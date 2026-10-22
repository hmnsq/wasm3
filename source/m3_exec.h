//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#اگر_تعریف_نشده m3_exec_h
#تعریف_کن m3_exec_h

// All these functions could move over to the .c at some point. normally, I'd say screw it,
// but it might prove useful to be able to compile m3_exec alone w/ optimizations while the remaining
// code is at debug O0


// About the naming convention of these operations/macros (_rs, _sr_, _ss, _srs, etc.)
//------------------------------------------------------------------------------------------------------
//   - 'r' means register and 's' means slot
//   - the first letter is the top of the stack
//
//  so, for example, _rs means the first operand (the first thing pushed to the stack) is in a slot
//  and the second operand (the top of the stack) is in a register
//------------------------------------------------------------------------------------------------------

#اگر_تعریف_نشده M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#پایان_اگر

#ضمن "m3_math_utils.h"
#ضمن "m3_compile.h"
#ضمن "m3_env.h"
#ضمن "m3_info.h"
#ضمن "m3_exec_defs.h"
#ضمن "m3_atomic.h"
#ضمن "m3_host.h"

#ضمن <limits.h>

d_m3BeginExternC

#تعریف_کن rewrite_op(OP)             * ((فراغ **) (_pc-1)) = (فراغ*)(OP)

#تعریف_کن فوری(TYPE)            * ((TYPE *) _pc++)
#تعریف_کن skip_فوری(TYPE)       (_pc++)

#تعریف_کن جایگاه(TYPE)                 * (TYPE *) (_sp + فوری (i32))
#تعریف_کن جایگاه_ptr(TYPE)             (TYPE *) (_sp + فوری (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#تعریف_کن d_m3_Wide_عملوند(IS64)      ((IS64) ? جایگاه (u64) : (u64) جایگاه (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#تعریف_کن d_m3MemRangeOk(شروع_2, درازی_2, MEM)  \
   ((شروع_2) <= (MEM)->درازی and (درازی_2) <= (MEM)->درازی - (شروع_2))


#إذا d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define بعدی_Op()                 M3_MUSTTAIL أرجع profileOp (d_m3_Op_All_آرگومانها, __FUNCTION__)
#وإلاإذا d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define بعدی_Op()                 M3_MUSTTAIL أرجع debugOp (d_m3_Op_All_آرگومانها, __FUNCTION__)
#وإلا
#  define بعدی_Op()                 بعدی_Op_مستقیم()
#پایان_اگر

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#إذا d_m3_Preload_بعدی_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_بعدی()        ثابت IM3Operation بعدی_Op_2 = (IM3Operation) (* _pc)
#  define بعدی_Op_Preloaded()        M3_MUSTTAIL أرجع بعدی_Op_2 (_pc + 1, d_m3_Op_آرگومانها)
#وإلا
#  define d_m3_Preload_بعدی()
#  define بعدی_Op_Preloaded()        بعدی_Op()
#پایان_اگر

#إذا d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL أرجع profileJumpOp ((pc_t)(PC), d_m3_Op_آرگومانها, __FUNCTION__)
#وإلا
#  define jumpOp(PC)               jump_Op_مستقیم(PC)
#پایان_اگر

#إذا d_m3_رکورد_Backtraces
#  define افزودن_Backtrace_قاب_2()            (افزودن_Backtrace_قاب (_mem->runtime, _pc - 1))
#  define fill_Backtrace_قاب(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    أرجع (افزودن_Backtrace_قاب_2 (), err)
#  define forwardTrap(err)                أرجع err
#وإلا
#  define افزودن_Backtrace_قاب_2()            افعل {} طالما (0)
#  define fill_Backtrace_قاب(FUNCTION)    افعل {} طالما (0)

#  define newTrap(err)                    أرجع err
#  define forwardTrap(err)                أرجع err
#پایان_اگر

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#إذا d_m3_Max_Native_پشته > 0
#  define d_m3_Check_Native_پشته()                                               \
       افعل {                                                                    \
           فراغ * m3_حد = m3MemRuntime (_mem)->پشته_حد;                  \
           إذا (M3_UNLIKELY (m3_حد && m3_Native_پشته_Ptr () < m3_حد))      \
               newTrap (m3_Err_trap_پشته_سرریز);                              \
       } طالما (0)
#وإلا
#  define d_m3_Check_Native_پشته()           افعل {} طالما (0)
#پایان_اگر


#إذا d_m3_Has_پشته_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  إذا d_m3HasFloat
#    define d_m3_Exp_Reg_آرگومانها(R0, FP0)        (R0), (FP0)
#  وإلا
#    define d_m3_Exp_Reg_آرگومانها(R0, FP0)        (R0)
#  endif

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_آرگومانها (R0, FP0), d_m3_پایه_Cstr)
#  وإلا
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_آرگومانها (R0, FP0))
#  endif

// A suspend is on its way up the native stack and this call was holding a
// frame: record what it takes to stand that frame back up, then keep unwinding
// with whatever the recording says. The registers go in because op_Call
// resumes the caller with the ones its own frame was holding.
//
// Out of line, and deliberately so: op_Call and its two siblings are the
// deepest-recursing functions in the interpreter, and a whole M3Frame of
// locals on every call is stack the recursion cannot spare - least of all in a
// sanitizer build, where each local carries a redzone.
//
// Called ahead of pushBacktraceFrame: a suspend is not a trap and has no
// business writing a backtrace.
ساكن M3_NOINLINE
m3ret_t رکورد_Call_قاب (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظه i_حافظه,
                         IM3Function i_function, m3reg_t i_r0
#  إذا d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3_قاب قاب;

    قاب.نوع = قاب_call;
    قاب.pc = i_pc;
    قاب.sp = i_sp;
    قاب.حافظه = i_حافظه;
    قاب.call.function = i_function;
    قاب.call.r0 = i_r0;
#  إذا d_m3HasFloat
    قاب.call.fp0 = i_fp0;
#  endif

    أرجع Continuation_رکورد_قاب(i_runtime, &قاب);
}

#  define d_m3_رکورد_Call_قاب(حافظه_2, FUNCTION)                                \
       إذا (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           أرجع رکورد_Call_قاب (m3MemRuntime (_mem), _pc, _sp, (حافظه_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_آرگومانها (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
ساكن M3_NOINLINE
m3ret_t رکورد_Loop_قاب (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظه i_حافظه)
{
    M3_قاب قاب;

    قاب.نوع = قاب_loop;
    قاب.pc = i_pc;
    قاب.sp = i_sp;
    قاب.حافظه = i_حافظه;

    أرجع Continuation_رکورد_قاب(i_runtime, &قاب);
}


#  إذا d_m3HasExceptionHandling
ساكن M3_NOINLINE
m3ret_t رکورد_Try_قاب (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_حافظه i_حافظه,
                        u32 i_رقم_Clauses, bool i_handlers_زنده)
{
    M3_قاب قاب;

    قاب.نوع = قاب_try;
    قاب.pc = i_clauses;
    قاب.sp = i_sp;
    قاب.حافظه = i_حافظه;
    قاب.try_.رقم_Clauses = i_رقم_Clauses;
    قاب.try_.handlers_زنده = i_handlers_زنده;

    أرجع Continuation_رکورد_قاب(i_runtime, &قاب);
}
#  endif


#  إذا d_m3_مدخل_Keeps_قاب
ساكن M3_NOINLINE
m3ret_t رکورد_مدخل_قاب (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظه i_حافظه,
                          IM3Function i_function)
{
    M3_قاب قاب;

    قاب.نوع = قاب_مدخل;
    قاب.pc = i_pc;
    قاب.sp = i_sp;
    قاب.حافظه = i_حافظه;
    قاب.مدخل.function = i_function;

    أرجع Continuation_رکورد_قاب(i_runtime, &قاب);
}
#  endif
#وإلا
#  define d_m3_رکورد_Call_قاب(حافظه_2, FUNCTION)
#پایان_اگر


#إذا d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#وإلاإذا d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ثابت IM3Runtime trace_rt = m3MemRuntime(_mem); (فراغ) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_عمق)*2, "", ##__VA_ARGS__)
#وإلا
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#پایان_اگر

#إذا d_m3EnableStrace >= 3
#  define d_m3_Trace_بارگیری(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#وإلا
#  define d_m3_Trace_بارگیری(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#پایان_اگر

#اگر_تعریف_شده DEBUG
  #تعریف_کن d_بیرون_از_Bounds newTrap (ErrorRuntime (m3_Err_trap_بیرون_از_Bounds_حافظه_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->درازی, عملوند))

#  define d_بیرون_از_Bounds_Mem_Op(جابه_جایی, اندازه_2) newTrap (ErrorRuntime (m3_Err_trap_بیرون_از_Bounds_حافظه_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->درازی, جابه_جایی, اندازه_2))
#وإلا
  #تعریف_کن d_بیرون_از_Bounds newTrap (m3_Err_trap_بیرون_از_Bounds_حافظه_Access)

#  define d_بیرون_از_Bounds_Mem_Op(جابه_جایی, اندازه_2) newTrap (m3_Err_trap_بیرون_از_Bounds_حافظه_Access)

#پایان_اگر

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#وإلا
d_m3RetSig Call (d_m3OpSig)
#پایان_اگر
{
    m3ret_t possible_trap = m3_Yield();
    إذا (M3_UNLIKELY(possible_trap)) {
        أرجع possible_trap;
    }

    بعدی_Op_مستقیم();
}

#تعریف_کن d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE عملوند = جایگاه (TYPE);                         \
    d_m3_Preload_بعدی ();                                 \
    OP((RES), عملوند, ((TYPE) REG), ##__VA_ARGS__);    \
    بعدی_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = جایگاه (TYPE);                        \
    TYPE operand1 = جایگاه (TYPE);                        \
    d_m3_Preload_بعدی ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    بعدی_Op_Preloaded ();                                 \
}

#تعریف_کن d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE عملوند = جایگاه (TYPE);                         \
    d_m3_Preload_بعدی ();                                 \
    OP((RES), ((TYPE) REG), عملوند, ##__VA_ARGS__);    \
    بعدی_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#تعریف_کن d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کن d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کن d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کن d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#تعریف_کن M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#تعریف_کن M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#تعریف_کن M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#تعریف_کن M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#تعریف_کن d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#تعریف_کن d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#تعریف_کن d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#تعریف_کن d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#تعریف_کن d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#تعریف_کن d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#تعریف_کن d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#تعریف_کن d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#تعریف_کن d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#تعریف_کن d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#تعریف_کن d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#تعریف_کن d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#تعریف_کن d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


//-----------------------
// clang-format off

// signed
d_m3CommutativeOp_i(i32, Equal,            ==)     d_m3CommutativeOp_i(i64, Equal,            ==)
d_m3CommutativeOp_i(i32, NotEqual,         !=)     d_m3CommutativeOp_i(i64, NotEqual,         !=)

d_m3Op_i(i32, LessThan,                    < )     d_m3Op_i(i64, LessThan,                    < )
d_m3Op_i(i32, GreaterThan,                 > )     d_m3Op_i(i64, GreaterThan,                 > )
d_m3Op_i(i32, LessThanOrEqual,             <=)     d_m3Op_i(i64, LessThanOrEqual,             <=)
d_m3Op_i(i32, GreaterThanOrEqual,          >=)     d_m3Op_i(i64, GreaterThanOrEqual,          >=)

// unsigned
d_m3Op_i(u32, LessThan,                    < )     d_m3Op_i(u64, LessThan,                    < )
d_m3Op_i(u32, GreaterThan,                 > )     d_m3Op_i(u64, GreaterThan,                 > )
d_m3Op_i(u32, LessThanOrEqual,             <=)     d_m3Op_i(u64, LessThanOrEqual,             <=)
d_m3Op_i(u32, GreaterThanOrEqual,          >=)     d_m3Op_i(u64, GreaterThanOrEqual,          >=)

#إذا d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#پایان_اگر

#تعریف_کن OP_جمع_کن_32(A,B) (i32)((u32)(A) + (u32)(B))
#تعریف_کن OP_جمع_کن_64(A,B) (i64)((u64)(A) + (u64)(B))
#تعریف_کن OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#تعریف_کن OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#تعریف_کن OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#تعریف_کن OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, جمع_کن,      OP_جمع_کن_32)  d_m3CommutativeOpFunc_i (i64, جمع_کن,      OP_جمع_کن_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, تفریق,            OP_SUB_32)  d_m3OpFunc_i (i64, تفریق,            OP_SUB_64)

#تعریف_کن OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#تعریف_کن OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#تعریف_کن OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#تعریف_کن OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, لغزش_چپ,       OP_SHL_32)      d_m3OpFunc_i (u64, لغزش_چپ,       OP_SHL_64)
d_m3OpFunc_i (i32, لغزش_راست,      OP_SHR_32)      d_m3OpFunc_i (i64, لغزش_راست,      OP_SHR_64)
d_m3OpFunc_i (u32, لغزش_راست,      OP_SHR_32)      d_m3OpFunc_i (u64, لغزش_راست,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#إذا d_m3_Fold_مجموعه_Local

// destination-folded ("_f") variants: same operand shapes as _rs/_sr/_ss, but the result is
// written to a trailing destination slot instead of _r0 - a producer fused with its local.set.
// operand immediates keep their normal order; the destination slot is the last immediate.
// The destination immediate is taken as a pointer before the operation runs, rather
// than written through at the end: that leaves _pc final early enough for the dispatch
// load to be issued ahead of the arithmetic instead of after it.
// The result is written through to the register as well as to the destination slot.
// Destination folding only ever replaces an operation that computed into the register,
// so the old value there is already dead and keeping the result costs nothing -- and it
// lets a local.tee carry on in the register instead of reloading the slot it just wrote.
#  define d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs_f)                            \
{                                                       \
    TYPE عملوند = جایگاه (TYPE);                         \
    TYPE * dest = جایگاه_ptr (TYPE);                      \
    d_m3_Preload_بعدی ();                                 \
    TYPE نتیجه;                                        \
    OP((نتیجه), عملوند, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = نتیجه;  REG = نتیجه;                     \
    بعدی_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = جایگاه (TYPE);                        \
    TYPE operand1 = جایگاه (TYPE);                        \
    TYPE * dest = جایگاه_ptr (TYPE);                      \
    d_m3_Preload_بعدی ();                                 \
    TYPE نتیجه;                                        \
    OP((نتیجه), operand1, operand2, ##__VA_ARGS__);    \
    * dest = نتیجه;  REG = نتیجه;                     \
    بعدی_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE عملوند = جایگاه (TYPE);                         \
    TYPE * dest = جایگاه_ptr (TYPE);                      \
    d_m3_Preload_بعدی ();                                 \
    TYPE نتیجه;                                        \
    OP((نتیجه), ((TYPE) REG), عملوند, ##__VA_ARGS__); \
    * dest = نتیجه;  REG = نتیجه;                     \
    بعدی_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#تعریف_کن d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کن d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کن d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کن d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, جمع_کن,          M3_FUNC, OP_جمع_کن_32)
d_m3FoldOp_i           (i32, تفریق,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, لغزش_چپ,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, لغزش_راست,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, لغزش_راست,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, جمع_کن,          M3_FUNC, OP_جمع_کن_64)
d_m3FoldOp_i           (i64, تفریق,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, لغزش_چپ,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, لغزش_راست,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, لغزش_راست,   M3_FUNC, OP_SHR_64)

#  إذا d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, جمع_کن,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, تفریق,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, جمع_کن,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, تفریق,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#پایان_اگر

#پایان_اگر // d_m3FoldSetLocal

#إذا d_m3HasFloat
d_m3CommutativeArithOp_f(f32, جمع_کن,         +)      d_m3CommutativeArithOp_f(f64, جمع_کن,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, تفریق,               -)      d_m3ArithOp_f(f64, تفریق,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#پایان_اگر

d_m3OpFunc_i(u32, Rotl, rotl32)
d_m3OpFunc_i(u32, Rotr, rotr32)
d_m3OpFunc_i(u64, Rotl, rotl64)
d_m3OpFunc_i(u64, Rotr, rotr64)

d_m3OpMacro_i(u32, Divide, OP_DIV_U);
d_m3OpMacro_i(i32, Divide, OP_DIV_S, INT32_MIN);
d_m3OpMacro_i(u64, Divide, OP_DIV_U);
d_m3OpMacro_i(i64, Divide, OP_DIV_S, INT64_MIN);

d_m3OpMacro_i(u32, Remainder, OP_REM_U);
d_m3OpMacro_i(i32, Remainder, OP_REM_S, INT32_MIN);
d_m3OpMacro_i(u64, Remainder, OP_REM_U);
d_m3OpMacro_i(i64, Remainder, OP_REM_S, INT64_MIN);

#إذا d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#پایان_اگر

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#تعریف_کن d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    بعدی_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE عملوند = جایگاه (TYPE);                     \
    OP((RES), عملوند, ##__VA_ARGS__);              \
    بعدی_Op ();                                      \
}

#تعریف_کن M3_UNARY(RES, X, OP) (RES) = OP(X)
#تعریف_کن M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#تعریف_کن d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#تعریف_کن d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#تعریف_کن d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#إذا d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       جذر);
d_m3UnaryArithOp_f(f32, نزدیکترین,    rintf);    d_m3UnaryArithOp_f(f64, نزدیکترین,    rint);
#  إذا defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  وإلا
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#پایان_اگر

#تعریف_کن OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_مقصد_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_مقصد_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#إذا (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#تعریف_کن OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#تعریف_کن OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#تعریف_کن OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#تعریف_کن OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#وإلاإذا defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#تعریف_کن OP_CLZ_32(x) __builtin_clz(x)
#تعریف_کن OP_CTZ_32(x) __builtin_ctz(x)
#تعریف_کن OP_CLZ_64(x) __builtin_clzll(x)
#تعریف_کن OP_CTZ_64(x) __builtin_ctzll(x)
#وإلا
#تعریف_کن OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#تعریف_کن OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#تعریف_کن OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#تعریف_کن OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#پایان_اگر

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#تعریف_کن OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    بعدی_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 عملوند = جایگاه(i64);
    _r0 = OP_WRAP_I64(عملوند);
    بعدی_Op();
}

// Integer sign extension operations
#تعریف_کن OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#تعریف_کن OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#تعریف_کن OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#تعریف_کن OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#تعریف_کن OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#إذا d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_بالا = جایگاه (u64);
    u64  rhs_کم  = جایگاه (u64);
    u64  lhs_بالا = جایگاه (u64);
    u64  lhs_کم  = جایگاه (u64);
    u64* کم     = جایگاه_ptr (u64);
    d_m3_Preload_بعدی();

    u64 جمع = lhs_کم + rhs_کم;

    *کم = جمع;
    _r0  = (i64) (lhs_بالا + rhs_بالا + (جمع < lhs_کم));

    بعدی_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_بالا = جایگاه (u64);
    u64  rhs_کم  = جایگاه (u64);
    u64  lhs_بالا = جایگاه (u64);
    u64  lhs_کم  = جایگاه (u64);
    u64* کم     = جایگاه_ptr (u64);
    d_m3_Preload_بعدی();

    *کم = lhs_کم - rhs_کم;
    _r0  = (i64) (lhs_بالا - rhs_بالا - (lhs_کم < rhs_کم));

    بعدی_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = جایگاه (i64);
    i64  a   = جایگاه (i64);
    u64* کم = جایگاه_ptr (u64);
    d_m3_Preload_بعدی();

    u64 بالا;
    *کم = mul_wide_i64 (a, b, &بالا);
    _r0  = (i64) بالا;

    بعدی_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = جایگاه (u64);
    u64  a   = جایگاه (u64);
    u64* کم = جایگاه_ptr (u64);
    d_m3_Preload_بعدی();

    u64 بالا;
    *کم = mul_wide_u64 (a, b, &بالا);
    _r0  = (i64) بالا;

    بعدی_Op_Preloaded();
}

#پایان_اگر // d_m3HasWideArithmetic

#تعریف_کن d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    بعدی_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * پشته = جایگاه_ptr (FROM);                 \
    OP((DEST), (* پشته), ##__VA_ARGS__);           \
    بعدی_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = جایگاه_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    بعدی_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * پشته = جایگاه_ptr (FROM);                 \
    TYPE * dest = جایگاه_ptr (TYPE);                  \
    OP((* dest), (* پشته), ##__VA_ARGS__);         \
    بعدی_Op ();                                      \
}

#إذا d_m3HasFloat
d_m3TruncMacro(_r0, _fp0, i32, Trunc, f32, OP_I32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f32, OP_U32_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i32, Trunc, f64, OP_I32_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u32, Trunc, f64, OP_U32_TRUNC_F64)

d_m3TruncMacro(_r0, _fp0, i64, Trunc, f32, OP_I64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f32, OP_U64_TRUNC_F32)
d_m3TruncMacro(_r0, _fp0, i64, Trunc, f64, OP_I64_TRUNC_F64)
d_m3TruncMacro(_r0, _fp0, u64, Trunc, f64, OP_U64_TRUNC_F64)

d_m3TruncMacro(_r0, _fp0, i32, TruncSat, f32, OP_I32_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, u32, TruncSat, f32, OP_U32_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, i32, TruncSat, f64, OP_I32_TRUNC_SAT_F64)
d_m3TruncMacro(_r0, _fp0, u32, TruncSat, f64, OP_U32_TRUNC_SAT_F64)

d_m3TruncMacro(_r0, _fp0, i64, TruncSat, f32, OP_I64_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, u64, TruncSat, f32, OP_U64_TRUNC_SAT_F32)
d_m3TruncMacro(_r0, _fp0, i64, TruncSat, f64, OP_I64_TRUNC_SAT_F64)
d_m3TruncMacro(_r0, _fp0, u64, TruncSat, f64, OP_U64_TRUNC_SAT_F64)
#پایان_اگر

#تعریف_کن d_m3TypeModifyOp(REG_مقصد, REG_FROM, مقصد_2, NAME, FROM)  \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_مقصد = (مقصد_2) ((FROM) REG_FROM);                        \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = جایگاه (FROM);                                \
    REG_مقصد = (مقصد_2) (from);                                   \
    بعدی_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#تعریف_کن d_m3FloatModifyOp(مقصد_2, NAME, FROM)                   \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((مقصد_2) ((FROM) _fp0));             \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = جایگاه (FROM);                                \
    _fp0 = canon_nan_##TO ((مقصد_2) (from));                    \
    بعدی_Op ();                                              \
}

#إذا d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#پایان_اگر

#تعریف_کن d_m3TypeConvertOp(REG_مقصد, REG_FROM, مقصد_2, NAME, FROM) \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_مقصد = (مقصد_2) ((FROM) REG_FROM);                        \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    جایگاه (مقصد_2) = (مقصد_2) ((FROM) REG_FROM);                     \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = جایگاه (FROM);                                \
    REG_مقصد = (مقصد_2) (from);                                   \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = جایگاه (FROM);                                \
    جایگاه (مقصد_2) = (مقصد_2) (from);                                \
    بعدی_Op ();                                              \
}

// Int to float
#إذا d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#پایان_اگر

#تعریف_کن d_m3ReinterpretOp(REG, مقصد_2, SRC, FROM)               \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = جایگاه (FROM);                                      \
    REG = u.t;                                              \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    جایگاه (مقصد_2) = u.t;                                        \
    بعدی_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = جایگاه (FROM);                                      \
    جایگاه (مقصد_2) = u.t;                                        \
    بعدی_Op ();                                              \
}

#إذا d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#پایان_اگر

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = فوری(u32*);
    جایگاه(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    بعدی_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = فوری(u64*);
    جایگاه(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    بعدی_Op();
}


d_m3Op(مجموعه_Global_i32)
{
    u32* global = فوری(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    بعدی_Op();
}


d_m3Op(مجموعه_Global_i64)
{
    u64* global = فوری(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    بعدی_Op();
}


#إذا d_m3_Has_پشته_Switching

d_m3Op(Branch);
d_m3Op(مدخل_2);
#  إذا !d_m3_مدخل_Keeps_قاب
d_m3Op(مدخل_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
ساكن M3_NOINLINE
IM3Function Function_از_Compiled_کد (pc_t i_pc)
{
    إذا (not i_pc) {
        أرجع عدم;
    }

    طالما ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation مدخل = (IM3Operation)i_pc[0];

#  إذا !d_m3_مدخل_Keeps_قاب
    إذا (مدخل == op_مدخل_Suspendable) {
        أرجع (IM3Function)i_pc[1];
    }
#  endif
    أرجع (مدخل == op_مدخل) ? (IM3Function)i_pc[1] : عدم;
}

#پایان_اگر

d_m3Op(Call)
{
    d_m3_Check_Native_پشته();

    pc_t      callPC = فوری(pc_t);
    i32       پشته_جابه_جایی = فوری(i32);
    IM3_حافظه حافظه = m3MemInfo(_mem);

    m3stack_t sp = _sp + پشته_جابه_جایی;

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_آرگومانها, d_m3_پایه_Cstr);
#وإلا
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_آرگومانها);
#پایان_اگر

    _mem = حافظه->mallocated;

    إذا (M3_LIKELY(not r)) {
        بعدی_Op();
    } وإلا {
        d_m3_رکورد_Call_قاب(حافظه, Function_از_Compiled_کد(callPC));
        افزودن_Backtrace_قاب_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = جایگاه(IM3Function);
    IM3FuncType type = فوری(IM3FuncType);
    i32         پشته_جابه_جایی = فوری(i32);
    IM3_حافظه   حافظه = m3MemInfo(_mem);

    m3stack_t   sp = _sp + پشته_جابه_جایی;

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(function)) {
        إذا (M3_LIKELY(type == function->funcType)) {
            إذا (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            إذا (M3_LIKELY(not r)) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_آرگومانها, d_m3_پایه_Cstr);
#وإلا
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_آرگومانها);
#پایان_اگر

                _mem = حافظه->mallocated;

                إذا (M3_LIKELY(not r)) {
                    بعدی_Op_مستقیم();
                } وإلا {
                    d_m3_رکورد_Call_قاب(حافظه, function);
                    افزودن_Backtrace_قاب_2();
                    forwardTrap(r);
                }
            }
        } وإلا {
            r = m3_Err_trap_غیرمستقیم_Call_Type_Mismatch;
        }
    } وإلا {
        r = m3Err_trapNullFunctionRef;
    }

    إذا (M3_UNLIKELY(r)) {
        newTrap(r);
    } وإلا {
        forwardTrap(r);
    }
}


// ref.as_non_null is a null check and nothing else: the value stays put, so
// there is no destination slot, only the trap.
d_m3Op(RefAsNonNull)
{
    IM3Function reference = جایگاه(IM3Function);

    إذا (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    بعدی_Op();
}


d_m3Op(Call_غیرمستقیم)
{
    d_m3_Check_Native_پشته();

    M3_جدول*    جدول_2 = فوری(M3_جدول*);
    u64         جدول_شاخص = d_m3_Wide_عملوند(جدول_2->isTable64);
    IM3FuncType type = فوری(IM3FuncType);
    i32         پشته_جابه_جایی = فوری(i32);
    IM3_حافظه   حافظه = m3MemInfo(_mem);

    m3stack_t   sp = _sp + پشته_جابه_جایی;

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(جدول_شاخص < جدول_2->اندازه)) {
        IM3Function function = (IM3Function)جدول_2->elements[جدول_شاخص];

        إذا (M3_LIKELY(function)) {
            إذا (M3_LIKELY(type == function->funcType)) {
                إذا (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                إذا (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3_حافظه_سرآیند* calleeMem = Module_حافظه_سرآیند(function->module);

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_آرگومانها, d_m3_پایه_Cstr);
#وإلا
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_آرگومانها);
#پایان_اگر

                    _mem = حافظه->mallocated;

                    إذا (M3_LIKELY(not r)) {
                        بعدی_Op_مستقیم();
                    } وإلا {
                        d_m3_رکورد_Call_قاب(حافظه, function);
                        افزودن_Backtrace_قاب_2();
                        forwardTrap(r);
                    }
                }
            } وإلا {
                r = m3_Err_trap_غیرمستقیم_Call_Type_Mismatch;
            }
        } وإلا {
            r = m3_Err_trap_جدول_Element_Is_Null;
        }
    } وإلا {
        r = m3_Err_trap_جدول_شاخص_بیرون_از_Range;
    }

    إذا (M3_UNLIKELY(r)) {
        newTrap(r);
    } وإلا {
        forwardTrap(r);
    }
}


// return_call / return_call_indirect: the callee takes over this function's stack frame
// instead of getting one of its own.  The spec guarantees the callee returns exactly what
// the enclosing function returns, so the two frames share their return slots and only the
// arguments have to be relocated: the compiler stages them above the caller's stack (they
// can still read the args/locals they're about to overwrite) and the op slides them down
// onto the frame base.  The jump is a tail call, so neither the m3 stack nor the native
// stack grows.  Tail recursion therefore runs in constant space, unless the return_call
// sits inside a loop or a try region: those hold a native frame that the tail call walks
// away from instead of unwinding, and d_m3CheckNativeStack bounds the pile they make.
#تعریف_کن d_m3_دم_Call_آرگومانها(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((فراغ *) (_sp + (RETURNSLOTS)), (ثابت فراغ *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * حجم (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = فوری(pc_t);
    i32     پشته_جابه_جایی = فوری(i32);
    i32     returnSlots = فوری(i32);
    u32     رقم_Arg_Slots = فوری(u32);

    m3ret_t possible_trap = m3_Yield();
    إذا (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_دم_Call_آرگومانها(returnSlots, پشته_جابه_جایی, رقم_Arg_Slots);

    jump_Op_مستقیم(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = جایگاه(IM3Function);
    IM3FuncType type = فوری(IM3FuncType);
    i32         پشته_جابه_جایی = فوری(i32);
    i32         returnSlots = فوری(i32);
    u32         رقم_Arg_Slots = فوری(u32);

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(function)) {
        إذا (M3_LIKELY(type == function->funcType)) {
            إذا (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            إذا (M3_LIKELY(not r)) {
                r = m3_Yield();

                إذا (M3_LIKELY(not r)) {
                    d_m3_دم_Call_آرگومانها(returnSlots, پشته_جابه_جایی, رقم_Arg_Slots);

                    jump_Op_مستقیم(function->compiled);
                }
            }
        } وإلا {
            r = m3_Err_trap_غیرمستقیم_Call_Type_Mismatch;
        }
    } وإلا {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(Return_Call_غیرمستقیم)
{
    M3_جدول*    جدول_2 = فوری(M3_جدول*);
    u64         جدول_شاخص = d_m3_Wide_عملوند(جدول_2->isTable64);
    IM3FuncType type = فوری(IM3FuncType);
    i32         پشته_جابه_جایی = فوری(i32);
    i32         returnSlots = فوری(i32);
    u32         رقم_Arg_Slots = فوری(u32);

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(جدول_شاخص < جدول_2->اندازه)) {
        IM3Function function = (IM3Function)جدول_2->elements[جدول_شاخص];

        إذا (M3_LIKELY(function)) {
            إذا (M3_LIKELY(type == function->funcType)) {
                إذا (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                إذا (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    إذا (M3_LIKELY(not r)) {
                        d_m3_دم_Call_آرگومانها(returnSlots, پشته_جابه_جایی, رقم_Arg_Slots);

                        jump_Op_مستقیم(function->compiled);
                    }
                }
            } وإلا {
                r = m3_Err_trap_غیرمستقیم_Call_Type_Mismatch;
            }
        } وإلا {
            r = m3_Err_trap_جدول_Element_Is_Null;
        }
    } وإلا {
        r = m3_Err_trap_جدول_شاخص_بیرون_از_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = فوری(IM3Function);
    ctx.userdata = فوری(فراغ*);
    u64* ثابت sp = ((u64*)_sp);
    IM3_حافظه  حافظه = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#إذا d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    حرف        outbuff[1024];
    حرف*       outp = outbuff;
    حرف*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.فیلد_Utf8);

    ثابت صحيح n_آرگومانها = ftype->رقم_آرگومانها;
    ثابت صحيح nRets = ftype->رقم_Rets;
    u64*      آرگومانها = sp + nRets;
    لكل (صحيح i = 0; i < n_آرگومانها; i++) {
        ثابت صحيح type = ftype->types[nRets + i];
        بدل (type) {
        حالة c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(آرگومانها + i)); اكسر;
        حالة c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(آرگومانها + i)); اكسر;
        حالة c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(آرگومانها + i)); اكسر;
        حالة c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(آرگومانها + i)); اكسر;
        افتراضي: outp += snprintf(outp, oute - outp, "<type %d>", type); اكسر;
        }
        إذا (i < n_آرگومانها - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  إذا d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#پایان_اگر

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    فراغ* پشته_backup = runtime->پشته;
    runtime->پشته = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_داده(ctx.function->host_حافظه->mallocated));
    runtime->پشته = پشته_backup;

#إذا d_m3EnableStrace
    إذا (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (حرف*)possible_trap);
    } وإلا إذا (ftype and ftype->رقم_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } وإلا {
        d_m3TracePrint("%s", outbuff);
    }
#پایان_اگر

    إذا (M3_UNLIKELY(possible_trap)) {
        _mem = حافظه->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        إذا (possible_trap != m3Err_pendingException) {
            افزودن_Backtrace_قاب_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_اندازه)
{
    IM3_حافظه حافظه = m3MemInfo(_mem);

    _r0 = حافظه->رقم_صفحات;

    بعدی_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_حافظه  حافظه = m3MemInfo(_mem);

    i32        رقم_صفحات_مقصد_Grow = (i32)_r0;
    إذا (رقم_صفحات_مقصد_Grow >= 0) {
        _r0 = (m3reg_t)حافظه->رقم_صفحات;

        إذا (M3_LIKELY(رقم_صفحات_مقصد_Grow)) {
            u64      required_صفحات = حافظه->رقم_صفحات + (u32)رقم_صفحات_مقصد_Grow;

            M3_نتیجه r = Resize_حافظه(runtime, حافظه, required_صفحات);
            إذا (r) {
                _r0 = -1;
            }

            _mem = حافظه->mallocated;
        }
    } وإلا {
        _r0 = -1;
    }

    بعدی_Op();
}


#إذا d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_حافظه  حافظه = m3MemInfo(_mem);

    u64        رقم_صفحات_مقصد_Grow = (u64)_r0;
    u64        رقم_صفحات = حافظه->رقم_صفحات;

    إذا (M3_LIKELY(رقم_صفحات_مقصد_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        إذا (رقم_صفحات_مقصد_Grow <= حافظه->max_صفحات - رقم_صفحات and
            not Resize_حافظه(runtime, حافظه, رقم_صفحات + رقم_صفحات_مقصد_Grow)) {
            _r0 = (m3reg_t)رقم_صفحات;
        } وإلا {
            _r0 = -1;
        }

        _mem = حافظه->mallocated;
    } وإلا {
        _r0 = (m3reg_t)رقم_صفحات;
    }

    بعدی_Op();
}

#پایان_اگر // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 اندازه = (u32)_r0;
    u64 منبع = جایگاه(u32);
    u64 destination = جایگاه(u32);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(منبع, اندازه, _mem))) {
            u8* dst = m3_Mem_داده(_mem) + destination;
            u8* src = m3_Mem_داده(_mem) + منبع;
            memmove(dst, src, (size_t)اندازه);

            بعدی_Op();
        } وإلا {
            d_بیرون_از_Bounds_Mem_Op(منبع, اندازه);
        }
    } وإلا {
        d_بیرون_از_Bounds_Mem_Op(destination, اندازه);
    }
}


#إذا d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 اندازه = (u64)_r0;
    u64 منبع = جایگاه(u64);
    u64 destination = جایگاه(u64);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(منبع, اندازه, _mem))) {
            u8* dst = m3_Mem_داده(_mem) + destination;
            u8* src = m3_Mem_داده(_mem) + منبع;
            memmove(dst, src, (size_t)اندازه);

            بعدی_Op();
        } وإلا {
            d_بیرون_از_Bounds_Mem_Op(منبع, اندازه);
        }
    } وإلا {
        d_بیرون_از_Bounds_Mem_Op(destination, اندازه);
    }
}

#پایان_اگر // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(مجموعه_حافظه)
{
    IM3_حافظه حافظه = فوری(IM3_حافظه);

    _mem = حافظه->mallocated;

    بعدی_Op();
}


#إذا d_m3_Has_Multi_حافظه

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3_حافظه       dest_حافظه = فوری(IM3_حافظه);
    IM3_حافظه       منبع_حافظه = فوری(IM3_حافظه);
    u32             widths = فوری(u32);

    M3_حافظه_سرآیند* destMem = dest_حافظه->mallocated;
    M3_حافظه_سرآیند* منبع_Mem = منبع_حافظه->mallocated;

    u64             اندازه = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             منبع = d_m3_Wide_عملوند(widths & 0x2);
    u64             destination = d_m3_Wide_عملوند(widths & 0x1);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, destMem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(منبع, اندازه, منبع_Mem))) {
            memmove(m3_Mem_داده(destMem) + destination, m3_Mem_داده(منبع_Mem) + منبع, (size_t)اندازه);

            بعدی_Op();
        } وإلا {
            d_بیرون_از_Bounds_Mem_Op(منبع, اندازه);
        }
    } وإلا {
        d_بیرون_از_Bounds_Mem_Op(destination, اندازه);
    }
}

#پایان_اگر // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 اندازه = (u32)_r0;
    u32 byte = جایگاه(u32);
    u64 destination = جایگاه(u32);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        u8* mem8 = m3_Mem_داده(_mem) + destination;
        memset(mem8, (u8)byte, اندازه);
        بعدی_Op();
    } وإلا {
        d_بیرون_از_Bounds_Mem_Op(destination, اندازه);
    }
}


#إذا d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 اندازه = (u64)_r0;
    u32 byte = جایگاه(u32);
    u64 destination = جایگاه(u64);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        u8* mem8 = m3_Mem_داده(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)اندازه);
        بعدی_Op();
    } وإلا {
        d_بیرون_از_Bounds_Mem_Op(destination, اندازه);
    }
}

#پایان_اگر // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_داده_Segment* segment = فوری(M3_داده_Segment*);

    u64            اندازه = (u32)_r0;
    u64            منبع = جایگاه(u32);
    u64            destination = جایگاه(u32);

    u64            موجود = segment->dropped ? 0 : segment->اندازه;

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(منبع <= موجود and اندازه <= موجود - منبع)) {
            کپی_حافظه(m3_Mem_داده(_mem) + destination, segment->داده + منبع, (size_t)اندازه);
            بعدی_Op();
        } وإلا {
            d_بیرون_از_Bounds_Mem_Op(منبع, اندازه);
        }
    } وإلا {
        d_بیرون_از_Bounds_Mem_Op(destination, اندازه);
    }
}


#إذا d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_داده_Segment* segment = فوری(M3_داده_Segment*);

    u64            اندازه = (u32)_r0;
    u64            منبع = جایگاه(u32);
    u64            destination = جایگاه(u64);

    u64            موجود = segment->dropped ? 0 : segment->اندازه;

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(منبع <= موجود and اندازه <= موجود - منبع)) {
            کپی_حافظه(m3_Mem_داده(_mem) + destination, segment->داده + منبع, (size_t)اندازه);
            بعدی_Op();
        } وإلا {
            d_بیرون_از_Bounds_Mem_Op(منبع, اندازه);
        }
    } وإلا {
        d_بیرون_از_Bounds_Mem_Op(destination, اندازه);
    }
}

#پایان_اگر // d_m3HasMemory64


d_m3Op(داده_Drop)
{
    M3_داده_Segment* segment = فوری(M3_داده_Segment*);
    segment->dropped = true;

    بعدی_Op();
}


#إذا d_m3HasRefTypes

d_m3Op(جدول_Get)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    u64      index = d_m3_Wide_عملوند(جدول_2->isTable64);

    إذا (M3_LIKELY(index < جدول_2->اندازه)) {
        _r0 = (u64)(uintptr_t)جدول_2->elements[index];
        بعدی_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بیرون_از_Bounds);
    }
}


d_m3Op(جدول_مجموعه)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    فراغ*    مقدار = جایگاه(فراغ*);
    u64      index = d_m3_Wide_عملوند(جدول_2->isTable64);

    إذا (M3_LIKELY(index < جدول_2->اندازه)) {
        جدول_2->elements[index] = مقدار;
        بعدی_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بیرون_از_Bounds);
    }
}


d_m3Op(جدول_اندازه)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);

    _r0 = جدول_2->اندازه;

    بعدی_Op();
}


d_m3Op(جدول_Grow)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    u64      delta = d_m3_Wide_عملوند(جدول_2->isTable64);
    فراغ*    مقدار = جایگاه(فراغ*);

    u32      old_اندازه = جدول_2->اندازه;
    u32      max_اندازه = جدول_2->max_اندازه ? جدول_2->max_اندازه : d_m3_Max_Sane_جدول_اندازه;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_اندازه = (delta <= max_اندازه - old_اندازه) ? (u64)old_اندازه + delta
                                                    : (u64)max_اندازه + 1;

    إذا (new_اندازه == old_اندازه)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_اندازه;
        بعدی_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (new_اندازه <= max_اندازه and (not runtime->جدول_Elements_حد or
                                delta <= runtime->جدول_Elements_حد - runtime->جدول_Elements_استفاده_شده)) {
        فراغ** elements = m3_ReallocArray(فراغ*, جدول_2->elements, (size_t)new_اندازه, old_اندازه);

        إذا (elements) {
            runtime->جدول_Elements_استفاده_شده += new_اندازه - old_اندازه;
            جدول_2->elements = elements;
            جدول_2->اندازه = (u32)new_اندازه;

            لكل (u32 i = old_اندازه; i < جدول_2->اندازه; ++i) {
                جدول_2->elements[i] = مقدار;
            }

            _r0 = old_اندازه;
            بعدی_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    بعدی_Op();
}


d_m3Op(جدول_Init)
{
    M3_جدول*          جدول_2 = فوری(M3_جدول*);
    M3ElementSegment* segment = فوری(M3ElementSegment*);
    u32               شمار = جایگاه(u32);
    u32               منبع = جایگاه(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_عملوند(جدول_2->isTable64);

    u64               موجود = segment->dropped ? 0 : segment->رقم_Elements;

    إذا (M3_LIKELY(destination <= جدول_2->اندازه and شمار <= جدول_2->اندازه - destination and
                  (u64) منبع + شمار <= موجود)) {
        لكل (u32 i = 0; i < شمار; ++i) {
            جدول_2->elements[destination + i] = segment->resolved[منبع + i];
        }

        بعدی_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بیرون_از_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = فوری(M3ElementSegment*);
    segment->dropped = true;

    بعدی_Op();
}


d_m3Op(جدول_Copy)
{
    M3_جدول* dst = فوری(M3_جدول*);
    M3_جدول* src = فوری(M3_جدول*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      شمار = d_m3_Wide_عملوند(dst->isTable64 and src->isTable64);
    u64      منبع = d_m3_Wide_عملوند(src->isTable64);
    u64      destination = d_m3_Wide_عملوند(dst->isTable64);

    إذا (M3_LIKELY(destination <= dst->اندازه and شمار <= dst->اندازه - destination and
                  منبع <= src->اندازه and شمار <= src->اندازه - منبع)) {
        memmove(dst->elements + destination, src->elements + منبع, (size_t)شمار * حجم(فراغ*));

        بعدی_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بیرون_از_Bounds);
    }
}


d_m3Op(جدول_Fill)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    u64      شمار = d_m3_Wide_عملوند(جدول_2->isTable64);
    فراغ*    مقدار = جایگاه(فراغ*);
    u64      index = d_m3_Wide_عملوند(جدول_2->isTable64);

    إذا (M3_LIKELY(index <= جدول_2->اندازه and شمار <= جدول_2->اندازه - index)) {
        لكل (u64 i = 0; i < شمار; ++i) {
            جدول_2->elements[index + i] = مقدار;
        }

        بعدی_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بیرون_از_Bounds);
    }
}

#پایان_اگر // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = فوری(IM3Function);

    m3ret_t     نتیجه = m3Err_none;

    إذا (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        نتیجه = CompileFunction(function);
    }

    إذا (not نتیجه) {
        // patch up compiled pc and call rewritten op_Call
        *((فراغ**)--_pc) = (فراغ*)(function->compiled);
        --_pc;
        بعدی_Op_مستقیم();
    }

    newTrap(نتیجه);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = فوری(IM3Function);

    m3ret_t     نتیجه = m3Err_none;

    إذا (M3_UNLIKELY(not function->compiled)) {
        نتیجه = CompileFunction(function);
    }

    إذا (not نتیجه) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((فراغ**)--_pc) = (فراغ*)(function->compiled);
        --_pc;
        بعدی_Op_مستقیم();
    }

    newTrap(نتیجه);
}


#إذا d_m3_Has_پشته_Switching
// Suspending for a reason that is not a control tag - the host asked, or the
// gas ran out. There is no handler to look for and no payload to carry, and
// clearing the routing a previous suspend left behind is what keeps this from
// being mistaken for one: a resume reads those fields to decide where to send
// the marker, and would otherwise send this one into whatever stub was last
// used, with whatever payload last went with it.
//
// No continuation handles it either. A resume that finds itself named as the
// handler takes the suspend as its own, so naming the suspending continuation
// would stop a suspend from inside a resumed one at that resume, with no stub
// to run. Naming none sends it through every resume up to the host.
ساكن
فراغ Suspend_Without_برچسب (IM3Runtime io_runtime, IM3Continuation io_cont, M3_امن_نقطه_نوع i_نوع,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  إذا d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_برچسب = عدم;
    io_runtime->suspendHandlerCont = عدم;
    io_runtime->suspendStubPC = عدم;
    io_runtime->switch_هدف = عدم;
    io_runtime->رقم_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspend_نقطه = (u8)i_نوع;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  إذا d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->رقم_Suspend_Results = 0;
}
#پایان_اگر // d_m3HasStackSwitching


#إذا d_m3HasGasMetering

// Pays for the straight-line segment of the function body that follows, before
// any of it runs. The compiler adds up the segment's instruction costs while it
// compiles them and leaves the total here as an immediate, so metering costs one
// subtract and one branch however long the segment is.
//
// The counter is allowed to go negative - the segment was charged in full - and
// nothing resets it, so the trap stands until the host arms the runtime again.
d_m3Op(UseGas)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    u32        هزینه = فوری(u32);

    runtime->gas_مانده -= (i64)هزینه;

    إذا (M3_UNLIKELY(runtime->gas_مانده < 0)) {
#  إذا d_m3_Has_پشته_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        إذا (runtime->isSuspendable and runtime->فعال_Continuation) {
            runtime->suspendRequested = true;
            بعدی_Op();
        }
#  endif
        newTrap(m3_Err_trap_بیرون_از_Gas);
    }

    بعدی_Op();
}

#پایان_اگر // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
ساكن مضمن
bool مدخل_میگنجد (IM3Function i_function, m3stack_t _sp, M3_حافظه_سرآیند* _mem)
{
#إذا d_m3_Skip_پشته_Check
    (فراغ)i_function;
    (فراغ)_sp;
    (فراغ)_mem;
    أرجع true;
#وإلاإذا d_m3_Has_پشته_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->فعال_Continuation : عدم);
    فراغ*           max_پشته = (_act && _act->val_پشته)
                                 ? (فراغ*)(_act->val_پشته + _act->رقم_پشته_Slots)
                                 : (_mem ? _mem->max_پشته : (فراغ*)~(uintptr_t)0);
    أرجع M3_LIKELY((فراغ*)(_sp + i_function->max_پشته_Slots) < max_پشته);
#وإلا
    أرجع M3_LIKELY((فراغ*)(_sp + i_function->max_پشته_Slots) < _mem->max_پشته);
#پایان_اگر
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
ساكن مضمن
فراغ Enter_قاب (IM3Function i_function, m3stack_t _sp)
{
#إذا defined(DEBUG)
    i_function->اصابتها++;
#پایان_اگر
    u8* پشته = (u8*)((m3slot_t*)_sp + i_function->رقم_Ret_And_Arg_Slots);

    memset(پشته, 0x0, i_function->رقم_Local_بایتها);
    پشته += i_function->رقم_Local_بایتها;

    إذا (i_function->constants) {
        کپی_حافظه(پشته, i_function->constants, i_function->رقم_Constant_بایتها);
    }
}


d_m3Op(مدخل_2)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = فوری(IM3Function);
#إذا d_m3_مدخل_Keeps_قاب
    IM3_حافظه حافظه = m3MemInfo(_mem);
#پایان_اگر

    إذا (مدخل_میگنجد(function, _sp, _mem)) {
        Enter_قاب(function, _sp);

#إذا !d_m3_مدخل_Keeps_قاب
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        بعدی_Op_مستقیم();
#وإلا

#  إذا d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->رقم_Ret_Slots));
        trace_rt->call_عمق++;
#  endif

        m3ret_t r = بعدی_Op_Impl();

#  إذا d_m3EnableStrace >= 2
        trace_rt->call_عمق--;

        إذا (r) {
            d_m3TracePrint("} !trap = %s", (حرف*)r);
        } وإلا إذا (Get_Function_رقم_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } وإلا {
            d_m3TracePrint("}");
        }
#  endif

        إذا (M3_UNLIKELY(r)) {
            _mem = حافظه->mallocated;
#  إذا d_m3_Has_پشته_Switching
            إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                أرجع رکورد_مدخل_قاب(m3MemRuntime(_mem), _pc, _sp, حافظه, function);
            }
#  endif
            fill_Backtrace_قاب();
        }
        forwardTrap(r);
#پایان_اگر // d_m3EntryKeepsFrame
    } وإلا {
        newTrap(m3_Err_trap_پشته_سرریز);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_پشته();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_حافظه حافظه = m3MemInfo(_mem);

    افعل {
#إذا d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_عمق++;
#پایان_اگر
        r = بعدی_Op_Impl();

#إذا d_m3EnableStrace >= 3
        trace_rt->call_عمق--;
        d_m3TracePrint("}");
#پایان_اگر
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = حافظه->mallocated;
    } طالما (r == _pc);

#إذا d_m3_Has_پشته_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        أرجع رکورد_Loop_قاب(m3MemRuntime(_mem), _pc, _sp, حافظه);
    }
#پایان_اگر

    forwardTrap(r);
}


#إذا d_m3HasExceptionHandling

// try_table. Like op_Loop this claims a native frame, and for the same reason:
// a throw unwinds by returning m3Err_pendingException up the native stack, so
// something has to be standing there to catch it.
//
// Immediates: the clause count, then two words per clause - the tag to match
// (NULL for catch_all / catch_all_ref) and the PC of the stub that moves the
// payload into place and branches to the clause's label. The body follows.
//
// The frame stays alive after the body falls through or branches out of the try
// region, but the handler record is popped at those points (op_PopHandlers), so
// a later throw sees this frame is no longer the innermost one and passes it by.
//
// The handler stack is just a depth counter: an unwind stops at whichever frame
// finds the count still standing at the value its own push produced, which is
// the innermost try region that hasn't been popped.
d_m3Op(Try_جدول)
{
    d_m3_Check_Native_پشته();

    IM3_حافظه  حافظه = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        رقم_Clauses = فوری(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * رقم_Clauses;

    u32        عمق = ++runtime->try_عمق;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = حافظه->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_عمق == عمق);
    runtime->try_عمق = عمق - 1;

#  إذا d_m3_Has_پشته_Switching
    إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        أرجع رکورد_Try_قاب(runtime, clauses, _sp, حافظه, رقم_Clauses, isInnermost);
    }
#  endif

    إذا (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ثابت M3Exception* exception = runtime->pendingException;

        لكل (u32 i = 0; i < رقم_Clauses; ++i) {
            IM3_برچسب برچسب = (IM3_برچسب)clauses[2 * i];

            إذا (برچسب == عدم or برچسب == exception->برچسب) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jump_Op_مستقیم(handler);
            }
        }

        // no clause matched: the exception carries on up
    }

    forwardTrap(r);
}


// Head of a catch clause's stub: copies the caught exception's payload into the
// slots the stub's branch expects, and the exnref itself for a _ref clause.
//
// A clause that takes no exnref is the end of the road for this exception, so
// it is released here rather than waiting for the call to finish - a module
// that throws inside a loop would otherwise pile up one object per lap. Not so
// once an exnref has been handed out: a local, a global or a table may still be
// holding it, ready for throw_ref.
d_m3Op(CatchPayload)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);
    M3Exception* exception = runtime->pendingException;

    u32          رقم_آرگومانها = فوری(u32);

    لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
        i32 offset = فوری(i32);
        u32 is64 = فوری(u32);
        u64 مقدار = exception->آرگومانها[i];

        إذا (is64) {
            *(u64*)(_sp + offset) = مقدار;
        } وإلا {
            *(u32*)(_sp + offset) = (u32)مقدار;
        }
    }

    i32 ref_جابه_جایی = فوری(i32);

    إذا (ref_جابه_جایی >= 0) {
        *(M3Exception**)(_sp + ref_جابه_جایی) = exception;
        exception->reified = true;
    } وإلا إذا (not exception->reified) {
        FreeException(runtime, exception);
    }

    بعدی_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_برچسب       برچسب = فوری(IM3_برچسب);
    u32          رقم_آرگومانها = فوری(u32);

    M3Exception* exception = NewException(runtime, برچسب, رقم_آرگومانها);

    إذا (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
        i32 offset = فوری(i32);
        u32 is64 = فوری(u32);

        exception->آرگومانها[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = جایگاه(M3Exception*);

    إذا (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    أرجع m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(برداشت_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        شمار = فوری(u32);

    runtime->try_عمق -= شمار;

    بعدی_Op();
}

#پایان_اگر // d_m3HasExceptionHandling

#إذا d_m3_Has_پشته_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = فوری(IM3FuncType);
    i32         dst_جایگاه = فوری(i32);
    i32         func_جایگاه = فوری(i32);

    IM3Function func = *(IM3Function*)(_sp + func_جایگاه);
    إذا (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    إذا (M3_UNLIKELY(not Continuation_Can_Acquire_پشته(runtime))) {
        newTrap(m3_Err_continuation_حد_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_نتیجه تخصیص = Continuation_Acquire_پشته(runtime, cont);
    إذا (M3_UNLIKELY(تخصیص)) {
        cont->state = cont_consumed;
        newTrap(تخصیص);
    }
    *(IM3Continuation*)(_sp + dst_جایگاه) = cont;
    بعدی_Op();
}


ساكن
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = فوری(IM3FuncType);
    IM3FuncType     ct2 = فوری(IM3FuncType);
    i32             dst_جایگاه = فوری(i32);
    i32             cont_جایگاه = فوری(i32);
    u32             رقم_Bound = فوری(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + cont_جایگاه);
    إذا (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->مدخل_Function);
    إذا (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->مدخل_Function = srcCont->مدخل_Function;
    dstCont->val_پشته = srcCont->val_پشته;
    dstCont->رقم_پشته_Slots = srcCont->رقم_پشته_Slots;
    dstCont->قابها = srcCont->قابها;
    dstCont->قابها_Cap = srcCont->قابها_Cap;
    dstCont->رقم_قابها = srcCont->رقم_قابها;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspend_نقطه = srcCont->suspend_نقطه;
    dstCont->r0 = srcCont->r0;
#  إذا d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_آرگومانها_شمار = srcCont->bound_آرگومانها_شمار;
    dstCont->state = srcCont->state;
#  إذا d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = عدم;
#  endif

    dstCont->رقم_Suspend_Results = srcCont->رقم_Suspend_Results;
    لكل (u32 j = 0; j < srcCont->رقم_Suspend_Results; ++j) {
        dstCont->suspend_نتیجه_Offsets[j] = srcCont->suspend_نتیجه_Offsets[j];
        dstCont->suspend_نتیجه_Is64[j] = srcCont->suspend_نتیجه_Is64[j];
    }

    إذا (dstCont->state == cont_allocated) {
        u16 رقم_Rets = ct1->contFuncType ? Get_Func_Type_رقم_Results(ct1->contFuncType) : 0;
        لكل (u32 i = 0; i < رقم_Bound; ++i) {
            i32 arg_جایگاه = فوری(i32);
            u32 is64 = فوری(u32);
            u32 هدف_Idx = (رقم_Rets + dstCont->bound_آرگومانها_شمار) * c_io_جایگاه_شمار;
            إذا (is64) {
                *(u64*)(dstCont->val_پشته + هدف_Idx) = *(u64*)(_sp + arg_جایگاه);
            } وإلا {
                *(u32*)(dstCont->val_پشته + هدف_Idx) = *(u32*)(_sp + arg_جایگاه);
            }
            dstCont->bound_آرگومانها_شمار++;
        }
    } وإلا {
        IM3Continuation inner = InnermostSuspended(dstCont);
        لكل (u32 i = 0; i < رقم_Bound; ++i) {
            i32 arg_جایگاه = فوری(i32);
            u32 is64 = فوری(u32);
            u32 dstIdx = dstCont->bound_آرگومانها_شمار;
            إذا (dstIdx < inner->رقم_Suspend_Results) {
                i32 dst_جابه_جایی = inner->suspend_نتیجه_Offsets[dstIdx];
                إذا (is64) {
                    *(u64*)(inner->sp + dst_جابه_جایی) = *(u64*)(_sp + arg_جایگاه);
                } وإلا {
                    *(u32*)(inner->sp + dst_جابه_جایی) = *(u32*)(_sp + arg_جایگاه);
                }
            }
            dstCont->bound_آرگومانها_شمار++;
        }
    }

    srcCont->val_پشته = عدم;
    srcCont->قابها = عدم;
    srcCont->قابها_Cap = 0;
    srcCont->رقم_قابها = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dst_جایگاه) = dstCont;
    بعدی_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
ساكن
IM3Continuation پیدا_کن_Handler (IM3Continuation i_cont, IM3_برچسب i_برچسب, u8 i_نوع, pc_t* o_stubPC)
{
    IM3_برچسب query_برچسب = (i_برچسب and i_برچسب->resolved) ? i_برچسب->resolved : i_برچسب;

    لكل (IM3Continuation cont = i_cont; cont; cont = cont->والد) {
        لكل (u32 h = 0; h < cont->رقم_Handlers; ++h) {
            pc_t   مدخل = cont->handlersPC + (h * 3);
            IM3_برچسب handler_برچسب = (IM3_برچسب)(*(مدخل + 1));
            إذا (handler_برچسب and handler_برچسب->resolved) {
                handler_برچسب = handler_برچسب->resolved;
            }

            إذا ((u32)(uintptr_t)(*مدخل) == i_نوع and handler_برچسب == query_برچسب) {
                إذا (o_stubPC) {
                    *o_stubPC = (pc_t)(*(مدخل + 2));
                }
                أرجع cont;
            }
        }
    }

    أرجع عدم;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
ساكن
فراغ ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    لكل (u32 i = 0; i < io_cont->رقم_Suspend_Results; ++i) {
        إذا (io_cont->suspend_نتیجه_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_نتیجه_Offsets[i]) = 0;
        } وإلا {
            *(m3slot_t*)(i_sp + io_cont->suspend_نتیجه_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->فعال_Continuation;
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_برچسب);
    }

    IM3_برچسب برچسب = فوری(IM3_برچسب);
    u32    رقم_Params = فوری(u32);

    runtime->رقم_Suspend_Payload = رقم_Params;
    لكل (u32 i = 0; i < رقم_Params; ++i) {
        i32 param_جایگاه = فوری(i32);
        u32 is64 = فوری(u32);
        إذا (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + param_جایگاه);
        } وإلا {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + param_جایگاه);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 رقم_Results = فوری(u32);
    cont->رقم_Suspend_Results = رقم_Results;
    لكل (u32 i = 0; i < رقم_Results; ++i) {
        cont->suspend_نتیجه_Offsets[i] = فوری(i32);
        cont->suspend_نتیجه_Is64[i] = فوری(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = عدم;
    IM3Continuation جستجو_Cont = پیدا_کن_Handler(cont, برچسب, 0, &matchedStubPC);

    إذا (M3_UNLIKELY(not جستجو_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_برچسب);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = جستجو_Cont;
    runtime->switch_هدف = عدم;
    runtime->suspend_برچسب = برچسب;

    cont->pc = _pc;
    cont->suspend_نقطه = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  إذا d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_آرگومانها_شمار = 0;

    أرجع m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
ساكن
m3ret_t پیاپی_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_رقم_Handlers, M3_حافظه_سرآیند** io_mem);
ساكن
فراغ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_رقم_Results);
ساكن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_حافظه_سرآیند* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
ساكن مضمن
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    أرجع pass and (i_cont->suspend_نقطه == safepoint_op or i_cont->suspend_نقطه == safepoint_مدخل);
}


// Rebuilds the native frames the suspend unwound through and lands on the
// suspension point at the bottom of them.
//
// Frames were recorded innermost first, which is the order the unwind met
// them, so this walks down from the outermost and recurses toward the
// suspension point: by the time anything runs again, the native stack looks
// the way it did when the suspend left.
//
// Each case has to stand in for one operation completely - claim the frame it
// claimed, and answer the return value the way it would - including recording
// itself again, because the continuation is free to suspend a second time from
// inside the chain being rebuilt here. That second suspend refills frames[]
// from index 0 while this walk is still in it, which is why every level takes
// its own copy of the frame on the way down, before anything can run.
ساكن
m3ret_t Replay_قابها (IM3Continuation i_cont, i32 i_عمق, M3_حافظه_سرآیند* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  إذا d_m3_Max_Native_پشته > 0
    {
        فراغ* حد = runtime->پشته_حد;

        إذا (M3_UNLIKELY(حد and m3_Native_پشته_Ptr() < حد)) {
            أرجع m3_Err_trap_پشته_سرریز;
        }
    }
#  endif

    إذا (i_عمق < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  إذا d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        إذا (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = عدم;

            أرجع m3Err_pendingException;
        }
#  endif
        إذا (pastCheck) {
            أرجع ResumePastCheck(i_cont, _mem);
        }

        أرجع d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3_قاب قاب = i_cont->قابها[i_عمق];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_قاب_نوع)قاب.نوع == قاب_resume);

#  إذا d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_قاب_نوع)قاب.نوع == قاب_try);
#  endif

    إذا (not ownDescent) {
        r = Replay_قابها(i_cont, i_عمق - 1, _mem);
        _mem = قاب.حافظه->mallocated;
    }

    بدل ((M3_قاب_نوع)قاب.نوع) {
    حالة قاب_call:
        إذا (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            أرجع d_m3CallWithRegs(قاب.pc, قاب.sp, _mem, قاب.call.r0, قاب.call.fp0);
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            أرجع Continuation_رکورد_قاب(runtime, &قاب);
        }
#  إذا d_m3_رکورد_Backtraces
        افزودن_Backtrace_قاب(runtime, قاب.pc - 1);
#  endif
        أرجع r;

    حالة قاب_loop: {
        m3stack_t _sp = قاب.sp;
        m3reg_t   _r0 = 0;
#  إذا d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        طالما (r == (m3ret_t)قاب.pc) {
            r = jumpOpImpl(قاب.pc);
            _mem = قاب.حافظه->mallocated;
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            أرجع Continuation_رکورد_قاب(runtime, &قاب);
        }
        أرجع r;
    }

#  إذا d_m3HasExceptionHandling
    حالة قاب_try: {
        m3stack_t _sp = قاب.sp;
        m3reg_t   _r0 = 0;
#    إذا d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (فراغ)_sp;
        (فراغ)_r0;
#    إذا d_m3HasFloat
        (فراغ)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 عمق = ++runtime->try_عمق;

        إذا (not قاب.try_.handlers_زنده) {
            runtime->try_عمق = عمق - 1;
        }

        r = Replay_قابها(i_cont, i_عمق - 1, _mem);
        _mem = قاب.حافظه->mallocated;

        bool isInnermost = (runtime->try_عمق == عمق);
        runtime->try_عمق = عمق - 1;

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            قاب.try_.handlers_زنده = isInnermost;
            أرجع Continuation_رکورد_قاب(runtime, &قاب);
        }

        إذا (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ثابت M3Exception* exception = runtime->pendingException;

            لكل (u32 i = 0; i < قاب.try_.رقم_Clauses; ++i) {
                IM3_برچسب برچسب = (IM3_برچسب)قاب.pc[2 * i];

                إذا (برچسب == عدم or برچسب == exception->برچسب) {
                    pc_t handler = (pc_t)قاب.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    أرجع jumpOpImpl(handler);
                }
            }
        }
        أرجع r;
    }
#  endif

    حالة قاب_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = قاب.resume.cont;

        r = پیاپی_Under_Handlers(runtime, &inner, قاب.resume.handlersPC,
                             قاب.resume.رقم_Handlers, &_mem);

        إذا (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, قاب.sp, قاب.resume.resultsPC,
                                    قاب.resume.رقم_Results);
            Continuation_Recycle_پشته(runtime, inner);

            أرجع d_m3CallWithRegs(قاب.pc, قاب.sp, _mem, 0, 0.);
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            إذا (runtime->suspendHandlerCont != inner) {
                قاب.resume.cont = inner;
                أرجع Continuation_رکورد_قاب(runtime, &قاب);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            إذا (M3_UNLIKELY(not suspended)) {
                أرجع m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            إذا (runtime->suspendStubPC) {
                أرجع d_m3CallWithRegs(runtime->suspendStubPC, قاب.sp, _mem, 0, 0.);
            } وإلا {
                أرجع m3_Err_trap_Unhandled_Control_برچسب;
            }
        } وإلا {
            inner->state = cont_consumed;
            Continuation_Recycle_پشته(runtime, inner);
        }

        أرجع r;
    }

#  إذا d_m3_مدخل_Keeps_قاب
    حالة قاب_مدخل:
        إذا (M3_UNLIKELY(r)) {
            إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                أرجع Continuation_رکورد_قاب(runtime, &قاب);
            }
#    إذا d_m3_رکورد_Backtraces
            FillBacktraceFunctionInfo(runtime, قاب.مدخل.function);
#    وإلا
            (فراغ)قاب.مدخل.function;
#    endif
        }
        أرجع r;
#  endif
    }

    أرجع r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
ساكن
فراغ* ContinuationArgDest (IM3Continuation i_cont, u32 i_شاخص)
{
    إذا (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : عدم;
        u16         رقم_Rets = inner ? Get_Func_Type_رقم_Results(inner) : 0;

        أرجع i_cont->val_پشته + (رقم_Rets + i_شاخص) * c_io_جایگاه_شمار;
    }

    إذا (i_شاخص < i_cont->رقم_Suspend_Results) {
        أرجع i_cont->sp + i_cont->suspend_نتیجه_Offsets[i_شاخص];
    }

    أرجع عدم;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
ساكن
فراغ Hand_Over_مقصد_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 رقم_آرگومانها = i_runtime->رقم_Suspend_Payload;
    u32 پایه = (io_peer->state == cont_allocated) ? io_peer->bound_آرگومانها_شمار : 0;

    لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
        فراغ* dest = ContinuationArgDest(io_peer, پایه + i);

        إذا (dest) {
            إذا (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } وإلا {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    فراغ* refDest = ContinuationArgDest(io_peer, پایه + رقم_آرگومانها);

    إذا (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    إذا (io_peer->state == cont_allocated) {
        io_peer->bound_آرگومانها_شمار += رقم_آرگومانها + 1;
    }
}


// resume, resume_throw and resume_throw_ref. The three differ only in what the
// continuation is handed to start with, which is what the mode immediate says,
// so everything after that - the handler table, the switch loop, the results -
// is written once.
// The innermost continuation of a capture: the one whose suspension point is
// at the bottom of it, and so the one whose waiting slots any values handed to
// the capture belong in. A resume the suspend travelled through leaves its
// record at the foot of the frame list, which is the link to follow.
ساكن
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    طالما (i_cont->رقم_قابها and (M3_قاب_نوع) i_cont->قابها[0].نوع == قاب_resume) {
        i_cont = i_cont->قابها[0].resume.cont;
    }

    أرجع i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
ساكن
m3ret_t پیاپی_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_رقم_Handlers, M3_حافظه_سرآیند** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation والد = i_runtime->فعال_Continuation;
    IM3_حافظه       حافظه = m3MemInfo(*io_mem);
    m3ret_t         r;

    لكل (;;) {
#  إذا d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        إذا (M3_UNLIKELY(cont->resumeThrow and cont->رقم_قابها == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = عدم;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            اكسر;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        إذا (isStarting) {
            إذا (M3_UNLIKELY(not cont->مدخل_Function->compiled)) {
                r = CompileFunction(cont->مدخل_Function);

                إذا (M3_UNLIKELY(r)) {
                    اكسر;
                }
            }
            cont->pc = cont->مدخل_Function->compiled;
            cont->sp = cont->val_پشته;
            cont->رقم_قابها = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->رقم_Handlers = i_رقم_Handlers;
        cont->والد = والد;
        cont->state = cont_running;
        cont->bound_آرگومانها_شمار = 0;

        i_runtime->فعال_Continuation = cont;

        إذا (cont->رقم_قابها) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->رقم_قابها - 1;
            cont->رقم_قابها = 0;

            r = Replay_قابها(cont, outermost, *io_mem);
        } وإلا إذا (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } وإلا {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = حافظه->mallocated;
        i_runtime->فعال_Continuation = والد;

        إذا (r != m3Err_continuationSuspended) {
            اكسر;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        إذا (not i_runtime->switch_هدف or i_runtime->suspendHandlerCont != cont) {
            اكسر;
        }

        IM3Continuation peer = i_runtime->switch_هدف;
        i_runtime->switch_هدف = عدم;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        إذا (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            اكسر;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_Cont_Type_از_Ref(peer->مدخل_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_رقم_Params(peerType) - 1)));
        Hand_Over_مقصد_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    أرجع r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
ساكن
فراغ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_رقم_Results)
{
    لكل (u32 i = 0; i < i_رقم_Results; ++i) {
        i32 res_جایگاه = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_جایگاه_شمار;

        إذا (is64) {
            *(u64*)(i_sp + res_جایگاه) = *(u64*)(i_cont->val_پشته + srcIdx);
        } وإلا {
            *(u32*)(i_sp + res_جایگاه) = *(u32*)(i_cont->val_پشته + srcIdx);
        }
    }
}


// resume, resume_throw and resume_throw_ref. The three differ only in what the
// continuation is handed to start with, which is what the mode immediate says,
// so everything after that - the handler table, the switch loop, the results -
// is written once.
d_m3Op(Resume)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     contFuncType = فوری(IM3FuncType);
    i32             cont_جایگاه = فوری(i32);
    u32             نما = فوری(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + cont_جایگاه);
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         رقم_Rets = Get_Func_Type_رقم_Results(innerType);

    إذا (نما == d_m3ResumeThrowRef) {
        i32 exn_جایگاه = فوری(i32);

#  إذا d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exn_جایگاه);

        إذا (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  وإلا
        (فراغ)exn_جایگاه;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } وإلا إذا (نما == d_m3ResumeThrow) {
        IM3_برچسب exn_برچسب = فوری(IM3_برچسب);
        u32    رقم_آرگومانها = فوری(u32);

#  إذا d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_برچسب, رقم_آرگومانها);

        إذا (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
            i32 offset = فوری(i32);
            u32 is64 = فوری(u32);

            exception->آرگومانها[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  وإلا
        (فراغ)exn_برچسب;
        (فراغ)رقم_آرگومانها;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } وإلا {
        u32 رقم_آرگومانها = فوری(u32);

        إذا (cont->state == cont_allocated) {
            لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
                i32 arg_جایگاه = فوری(i32);
                u32 is64 = فوری(u32);
                u32 هدف_Idx = (رقم_Rets + cont->bound_آرگومانها_شمار + i) * c_io_جایگاه_شمار;
                إذا (is64) {
                    *(u64*)(cont->val_پشته + هدف_Idx) = *(u64*)(_sp + arg_جایگاه);
                } وإلا {
                    *(u32*)(cont->val_پشته + هدف_Idx) = *(u32*)(_sp + arg_جایگاه);
                }
            }
            cont->bound_آرگومانها_شمار = 0;
        } وإلا {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
                i32 arg_جایگاه = فوری(i32);
                u32 is64 = فوری(u32);
                u32 dstIdx = cont->bound_آرگومانها_شمار + i;
                إذا (dstIdx < inner->رقم_Suspend_Results) {
                    i32 dst_جابه_جایی = inner->suspend_نتیجه_Offsets[dstIdx];
                    إذا (is64) {
                        *(u64*)(inner->sp + dst_جابه_جایی) = *(u64*)(_sp + arg_جایگاه);
                    } وإلا {
                        *(u32*)(inner->sp + dst_جابه_جایی) = *(u32*)(_sp + arg_جایگاه);
                    }
                }
            }
        }
    }

    u32  رقم_Handlers = فوری(u32);
    pc_t handlersPC = _pc;
    _pc += رقم_Handlers * 3;

    u32       رقم_Results = فوری(u32);
    pc_t      resultsPC = _pc;

    IM3_حافظه حافظه = m3MemInfo(_mem);

    m3ret_t   r = پیاپی_Under_Handlers(runtime, &cont, handlersPC, رقم_Handlers, &_mem);

    إذا (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, رقم_Results);
        Continuation_Recycle_پشته(runtime, cont);

        _pc += رقم_Results * 2;
        بعدی_Op();
    } وإلا إذا (r == m3Err_continuationSuspended) {
        إذا (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            إذا (M3_UNLIKELY(not runtime->فعال_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_برچسب);
            }
            M3_قاب قاب;
            قاب.نوع = قاب_resume;
            قاب.pc = resultsPC + رقم_Results * 2;
            قاب.sp = _sp;
            قاب.حافظه = حافظه;
            قاب.resume.cont = cont;
            قاب.resume.handlersPC = handlersPC;
            قاب.resume.رقم_Handlers = رقم_Handlers;
            قاب.resume.resultsPC = resultsPC;
            قاب.resume.رقم_Results = رقم_Results;

            أرجع Continuation_رکورد_قاب(runtime, &قاب);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        إذا (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        إذا (runtime->suspendStubPC) {
            jump_Op_مستقیم(runtime->suspendStubPC);
        } وإلا {
            newTrap(m3_Err_trap_Unhandled_Control_برچسب);
        }
    } وإلا {
        cont->state = cont_consumed;
        Continuation_Recycle_پشته(runtime, cont);
        افزودن_Backtrace_قاب_2();
        forwardTrap(r);
    }
}


d_m3Op(ResumePayload)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);

    // The payload comes from the suspend; the reference is to the whole of what
    // was captured, which is not the same continuation once the suspend has
    // travelled out through a resume that had nothing to say about its tag.
    IM3Continuation cont = runtime->suspendedContinuation;
    cont->type = فوری(IM3FuncType);

    u32 رقم_آرگومانها = فوری(u32);
    لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
        i32 offset = فوری(i32);
        u32 is64 = فوری(u32);
        u64 val = (i < runtime->رقم_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        إذا (is64) {
            *(u64*)(_sp + offset) = val;
        } وإلا {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_جابه_جایی = فوری(i32);
    إذا (cont_جابه_جایی >= 0) {
        *(IM3Continuation*)(_sp + cont_جابه_جایی) = cont;
    }

    بعدی_Op();
}


// switch is a suspend that names its own replacement. Rather than run the
// peer here - which would leave this native frame owning a continuation it is
// no longer running - it suspends the current continuation the ordinary way and
// hands the peer to the resume that installed the matching (on $e switch)
// handler. That resume is the delimiter the proposal asks for, and it is the
// frame that should be tracking whatever is running under it.
d_m3Op(Switch)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->فعال_Continuation;

    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_برچسب);
    }

    skip_فوری(IM3FuncType);

    IM3_برچسب          برچسب = فوری(IM3_برچسب);
    i32             هدف_جایگاه = فوری(i32);

    IM3Continuation هدف = *(IM3Continuation*)(_sp + هدف_جایگاه);

    إذا (M3_UNLIKELY(not هدف)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(هدف->state == cont_consumed or هدف->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = پیدا_کن_Handler(cont, برچسب, 1, عدم);

    إذا (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_برچسب);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 رقم_آرگومانها = فوری(u32);

    runtime->رقم_Suspend_Payload = رقم_آرگومانها;

    لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
        i32 arg_جایگاه = فوری(i32);
        u32 is64 = فوری(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + arg_جایگاه) : (u64) * (u32*)(_sp + arg_جایگاه);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 رقم_Results = فوری(u32);

    cont->رقم_Suspend_Results = رقم_Results;

    لكل (u32 i = 0; i < رقم_Results; ++i) {
        cont->suspend_نتیجه_Offsets[i] = فوری(i32);
        cont->suspend_نتیجه_Is64[i] = فوری(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = عدم;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switch_هدف = هدف;
    runtime->suspend_برچسب = برچسب;

    cont->pc = _pc;
    cont->suspend_نقطه = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  إذا d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    أرجع m3Err_continuationSuspended;
}


#پایان_اگر // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = فوری(pc_t);

    إذا (condition) {
        بعدی_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = جایگاه(i32);

    pc_t elsePC = فوری(pc_t);

    إذا (condition) {
        بعدی_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_جدول)
{
    u32   branch_شاخص = جایگاه(u32);           // branch index is always in a slot
    u32   رقم_Targets = فوری(u32);

    pc_t* branches = (pc_t*)_pc;

    إذا (branch_شاخص > رقم_Targets) {
        branch_شاخص = رقم_Targets; // the default index
    }

    jumpOp(branches[branch_شاخص]);
}


#تعریف_کن d_m3_مجموعه_ثبات_مجموعه_جایگاه(TYPE, REG) \
d_m3Op  (مجموعه_ثبات_2##TYPE)            \
{                                       \
    TYPE * src = جایگاه_ptr (TYPE);       \
    d_m3_Preload_بعدی ();                 \
    REG = * src;                        \
    بعدی_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (مجموعه_جایگاه_2##TYPE)                 \
{                                       \
    TYPE * dst = جایگاه_ptr (TYPE);       \
    d_m3_Preload_بعدی ();                 \
    * dst = (TYPE) REG;                 \
    بعدی_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_مجموعه_جایگاه_2##TYPE)         \
{                                       \
    TYPE * پشته     = جایگاه_ptr (TYPE); \
    TYPE * preserve  = جایگاه_ptr (TYPE); \
    d_m3_Preload_بعدی ();                 \
                                        \
    * preserve = * پشته;               \
    * پشته = (TYPE) REG;               \
                                        \
    بعدی_Op_Preloaded ();                 \
}

d_m3_مجموعه_ثبات_مجموعه_جایگاه(i32, _r0)
d_m3_مجموعه_ثبات_مجموعه_جایگاه(i64, _r0)
#إذا d_m3HasFloat
d_m3_مجموعه_ثبات_مجموعه_جایگاه(f32, _fp0)
d_m3_مجموعه_ثبات_مجموعه_جایگاه(f64, _fp0)
#پایان_اگر

d_m3Op(Copy_جایگاه_32)
{
    u32* dst = جایگاه_ptr(u32);
    u32* src = جایگاه_ptr(u32);
    d_m3_Preload_بعدی();

    *dst = *src;

    بعدی_Op_Preloaded();
}


d_m3Op(Preserve_Copy_جایگاه_32)
{
    u32* dest = جایگاه_ptr(u32);
    u32* src = جایگاه_ptr(u32);
    u32* preserve = جایگاه_ptr(u32);
    d_m3_Preload_بعدی();

    *preserve = *dest;
    *dest = *src;

    بعدی_Op_Preloaded();
}


d_m3Op(Copy_جایگاه_64)
{
    u64* dst = جایگاه_ptr(u64);
    u64* src = جایگاه_ptr(u64);
    d_m3_Preload_بعدی();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    بعدی_Op_Preloaded();
}


d_m3Op(Preserve_Copy_جایگاه_64)
{
    u64* dest = جایگاه_ptr(u64);
    u64* src = جایگاه_ptr(u64);
    u64* preserve = جایگاه_ptr(u64);
    d_m3_Preload_بعدی();

    *preserve = *dest;
    *dest = *src;

    بعدی_Op_Preloaded();
}


#إذا d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_پشته)
{
    u32         opcode_شاخص = فوری(u32);
    u32         پشته_ارتفاع = فوری(u32);
    IM3Function function = فوری(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    چاپ_کن(" %4d ", opcode_شاخص);
    چاپ_کن(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  إذا d_m3HasFloat
    چاپ_کن("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    لكل (u32 i = 0; i < پشته_ارتفاع; ++i) {
        cstr_t نوع = "";

        چاپ_کن("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, نوع, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    چاپ_کن("---------------------------------------------------------------------------------------------------------\n");

    بعدی_Op_مستقیم();
}
#پایان_اگر


#تعریف_کن d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = جایگاه (TYPE);                \
    TYPE operand1 = جایگاه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بعدی_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = جایگاه (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = جایگاه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بعدی_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = جایگاه (i32);                 \
                                                \
    TYPE operand2 = جایگاه (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بعدی_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = جایگاه (i32);                 \
                                                \
    TYPE operand2 = جایگاه (TYPE);                \
    TYPE operand1 = جایگاه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بعدی_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#تعریف_کن d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = جایگاه (TYPE);                \
    TYPE operand1 = جایگاه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بعدی_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = جایگاه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بعدی_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = جایگاه (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بعدی_Op ();                                  \
}

#إذا d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, جایگاه(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, جایگاه(i32))
#پایان_اگر

d_m3Op(Return)
{
    m3_پشته_Check();
    أرجع m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = فوری(pc_t);

    إذا (condition) {
        jumpOp(branch);
    } وإلا {
        بعدی_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = جایگاه(i32);
    pc_t branch = فوری(pc_t);

    إذا (condition) {
        jumpOp(branch);
    } وإلا {
        بعدی_Op();
    }
}

#إذا d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE عملوند = جایگاه (TYPE);                             \
      pc_t branch  = فوری (pc_t);                        \
      إذا (عملوند OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      وإلا بعدی_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE عملوند = جایگاه (TYPE);                             \
      pc_t branch  = فوری (pc_t);                        \
      إذا (((TYPE) _r0) OPER عملوند)                          \
          jumpOp (branch);                                    \
      وإلا بعدی_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = جایگاه (TYPE);                            \
      TYPE operand1 = جایگاه (TYPE);                            \
      pc_t branch   = فوری (pc_t);                       \
      إذا (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      وإلا بعدی_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE عملوند = جایگاه (TYPE);                             \
      pc_t elsePC  = فوری (pc_t);                        \
      إذا (عملوند OPER ((TYPE) _r0))                          \
          بعدی_Op ();                                          \
      وإلا jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE عملوند = جایگاه (TYPE);                             \
      pc_t elsePC  = فوری (pc_t);                        \
      إذا (((TYPE) _r0) OPER عملوند)                          \
          بعدی_Op ();                                          \
      وإلا jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = جایگاه (TYPE);                            \
      TYPE operand1 = جایگاه (TYPE);                            \
      pc_t elsePC   = فوری (pc_t);                       \
      إذا (operand1 OPER operand2)                             \
          بعدی_Op ();                                          \
      وإلا jumpOp (elsePC);                                   \
  }

d_m3FusedCmpBranch(i32, Equal, ==)
d_m3FusedCmpBranch(i32, NotEqual, !=)
d_m3FusedCmpBranch(i32, LessThan, <)
d_m3FusedCmpBranch(i32, GreaterThan, >)
d_m3FusedCmpBranch(i32, LessThanOrEqual, <=)
d_m3FusedCmpBranch(i32, GreaterThanOrEqual, >=)
d_m3FusedCmpBranch(u32, LessThan, <)
d_m3FusedCmpBranch(u32, GreaterThan, >)
d_m3FusedCmpBranch(u32, LessThanOrEqual, <=)
d_m3FusedCmpBranch(u32, GreaterThanOrEqual, >=)

// i32.eqz is unary: form _r has the operand in _r0 (no operand immediate), _s reads a slot
d_m3Op(i32_BranchIfEqz_r)
{
    pc_t branch = فوری(pc_t);
    إذا (((i32)_r0) == 0) {
        jumpOp(branch);
    } وإلا {
        بعدی_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  عملوند = جایگاه(i32);
    pc_t branch = فوری(pc_t);
    إذا (عملوند == 0) {
        jumpOp(branch);
    } وإلا {
        بعدی_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = فوری(pc_t);
    إذا (((i32)_r0) == 0) {
        بعدی_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  عملوند = جایگاه(i32);
    pc_t elsePC = فوری(pc_t);
    إذا (عملوند == 0) {
        بعدی_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}

#پایان_اگر // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = فوری(pc_t);

    إذا (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        بعدی_Op();
    } وإلا {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = جایگاه(i32);
    pc_t branch = فوری(pc_t);

    إذا (condition) {
        بعدی_Op();
    } وإلا {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_پشته_Check();

    فراغ* loopId = فوری(فراغ*);
    أرجع loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    فراغ* loopId = فوری(فراغ*);

    إذا (condition) {
        أرجع loopId;
    } وإلا {
        بعدی_Op();
    }
}


#إذا d_m3_Has_پشته_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_پشته_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
        Suspend_Without_برچسب(runtime, runtime->فعال_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_آرگومانها(_r0, _fp0));

        أرجع m3Err_continuationSuspended;
    }

    فراغ* loopId = فوری(فراغ*);
    أرجع loopId;
}


#  إذا !d_m3_مدخل_Keeps_قاب
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(مدخل_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = فوری(IM3Function);

    إذا (مدخل_میگنجد(function, _sp, _mem)) {
        Enter_قاب(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
            Suspend_Without_برچسب(runtime, runtime->فعال_Continuation, safepoint_مدخل, _pc, _sp,
                              d_m3_Exp_Reg_آرگومانها(_r0, _fp0));

            أرجع m3Err_continuationSuspended;
        }

        بعدی_Op_مستقیم();
    } وإلا {
        newTrap(m3_Err_trap_پشته_سرریز);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(مدخل_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
        Suspend_Without_برچسب(runtime, runtime->فعال_Continuation, safepoint_مدخل, _pc - 1, _sp,
                          d_m3_Exp_Reg_آرگومانها(_r0, _fp0));

        أرجع m3Err_continuationSuspended;
    }

    بعدی_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    فراغ* loopId = فوری(فراغ*);

    إذا (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
            Suspend_Without_برچسب(runtime, runtime->فعال_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_آرگومانها(_r0, _fp0));

            أرجع m3Err_continuationSuspended;
        }
        أرجع loopId;
    } وإلا {
        بعدی_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
ساكن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_حافظه_سرآیند* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  إذا d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    إذا (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } وإلا إذا (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } وإلا إذا (op == op_مدخل_Check) {
        أرجع بعدی_Op_Impl();
    } وإلا {
        أرجع d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    أرجع op(_pc, d_m3_Op_آرگومانها, d_m3_پایه_Cstr);
#  وإلا
    أرجع op(_pc, d_m3_Op_آرگومانها);
#  endif
}
#پایان_اگر // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 مقدار = *(u32*)_pc++;
    جایگاه(u32) = مقدار;
    بعدی_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 مقدار;
    کپی_حافظه(&مقدار, _pc, حجم(مقدار));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    جایگاه(u64) = مقدار;
    بعدی_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_پشته_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(انتها_2)
{
    m3_پشته_Check();
    أرجع m3Err_none;
}


d_m3Op(مجموعه_Global_s32)
{
    u32* global = فوری(u32*);
    *global = جایگاه(u32);

    بعدی_Op();
}


d_m3Op(مجموعه_Global_s64)
{
    u64* global = فوری(u64*);
    *global = جایگاه(u64);

    بعدی_Op();
}

#إذا d_m3HasFloat
d_m3Op(مجموعه_Global_f32)
{
    f32* global = فوری(f32*);
    *global = (f32)_fp0;

    بعدی_Op();
}


d_m3Op(مجموعه_Global_f64)
{
    f64* global = فوری(f64*);
    *global = _fp0;

    بعدی_Op();
}
#پایان_اگر


#إذا d_m3HasMemory64

// Checks a 64-bit address and folds the memarg offset into it, leaving the
// effective address in a slot the access that follows reads as an ordinary
// 32-bit one, with an offset of zero. That keeps one op between a 64-bit
// memory and the whole load/store table, rather than a second copy of it.
//
// The spec computes the effective address as a u65 and traps if it is out of
// range. Here the address is first held against d_m3AddressLimit and the
// compiler has already refused to emit this for an offset above it, so their
// sum is below 2^53 and no wider arithmetic is needed. An address that then
// passes the bounds check is below the memory's length, which no allocator
// lets past 32 bits, so the truncation below is exact.
d_m3Op(CheckAddr64)
{
    u64  عملوند = جایگاه(u64);
    u32* آدرس = جایگاه_ptr(u32);

    u64  offset;
    کپی_حافظه(&offset, _pc, حجم(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    إذا (M3_LIKELY(عملوند < d_m3_آدرس_حد)) {
        u64 effective = عملوند + offset;

        // the access itself checks its own size against what is left
        إذا (M3_LIKELY(effective < _mem->درازی)) {
            *آدرس = (u32)effective;
            بعدی_Op();
        }
    }

    d_بیرون_از_Bounds;
}

#پایان_اگر // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#إذا d_m3_Skip_حافظه_Bounds_Check || d_m3_Guarded_حافظه
#  define m3MemCheck(x) true
#وإلا
#  define m3MemCheck(x) M3_LIKELY(x)
#پایان_اگر

// memcpy here is to support non-aligned access on some platforms.

#تعریف_کن d_m3_بارگیری(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_بعدی ();                                 \
    u64 عملوند = (u32) _r0;                            \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (SRC_TYPE) <= _mem->درازی     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_داده(_mem) + عملوند;       \
            SRC_TYPE مقدار;                             \
            کپی_حافظه(&مقدار, src8, حجم(مقدار));        \
            M3_BSWAP_##SRC_TYPE(مقدار);                 \
            REG = (DEST_TYPE)مقدار;                     \
            d_m3_Trace_بارگیری(DEST_TYPE, عملوند, REG);     \
        }                                               \
        بعدی_Op_Preloaded ();                             \
    } وإلا d_بیرون_از_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملوند = جایگاه (u32);                           \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_بعدی ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (SRC_TYPE) <= _mem->درازی     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_داده(_mem) + عملوند;       \
            SRC_TYPE مقدار;                             \
            کپی_حافظه(&مقدار, src8, حجم(مقدار));        \
            M3_BSWAP_##SRC_TYPE(مقدار);                 \
            REG = (DEST_TYPE)مقدار;                     \
            d_m3_Trace_بارگیری(DEST_TYPE, عملوند, REG);     \
        }                                               \
        بعدی_Op_Preloaded ();                             \
    } وإلا d_بیرون_از_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#تعریف_کن d_m3_بارگیری_i(DEST_TYPE, SRC_TYPE) d_m3_بارگیری(_r0, DEST_TYPE, SRC_TYPE)
#تعریف_کن d_m3_بارگیری_f(DEST_TYPE, SRC_TYPE) d_m3_بارگیری(_fp0, DEST_TYPE, SRC_TYPE)

#إذا d_m3HasFloat
d_m3_بارگیری_f(f32, f32);
d_m3_بارگیری_f(f64, f64);
#پایان_اگر

d_m3_بارگیری_i(i32, i8);
d_m3_بارگیری_i(i32, u8);
d_m3_بارگیری_i(i32, i16);
d_m3_بارگیری_i(i32, u16);
d_m3_بارگیری_i(i32, i32);

d_m3_بارگیری_i(i64, i8);
d_m3_بارگیری_i(i64, u8);
d_m3_بارگیری_i(i64, i16);
d_m3_بارگیری_i(i64, u16);
d_m3_بارگیری_i(i64, i32);
d_m3_بارگیری_i(i64, u32);
d_m3_بارگیری_i(i64, i64);

#إذا d_m3_Fold_مجموعه_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_بارگیری_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = فوری (u32);                       \
      DEST_TYPE * dest = جایگاه_ptr (DEST_TYPE);            \
      d_m3_Preload_بعدی ();                                 \
      u64 عملوند = (u32) _r0;                            \
      عملوند += offset;                                  \
                                                          \
      إذا (m3MemCheck(                                     \
          عملوند + حجم (SRC_TYPE) <= _mem->درازی     \
      )) {                                                \
          u8* src8 = m3_Mem_داده(_mem) + عملوند;           \
          SRC_TYPE مقدار;                                 \
          کپی_حافظه(&مقدار, src8, حجم(مقدار));            \
          M3_BSWAP_##SRC_TYPE(مقدار);                     \
          DEST_TYPE نتیجه = (DEST_TYPE)مقدار;            \
          * dest = نتیجه;  REG = نتیجه;                 \
          d_m3_Trace_بارگیری(DEST_TYPE, عملوند, نتیجه);      \
          بعدی_Op_Preloaded ();                             \
      } وإلا d_بیرون_از_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 عملوند = جایگاه (u32);                           \
      u32 offset = فوری (u32);                       \
      DEST_TYPE * dest = جایگاه_ptr (DEST_TYPE);            \
      d_m3_Preload_بعدی ();                                 \
      عملوند += offset;                                  \
                                                          \
      إذا (m3MemCheck(                                     \
          عملوند + حجم (SRC_TYPE) <= _mem->درازی     \
      )) {                                                \
          u8* src8 = m3_Mem_داده(_mem) + عملوند;           \
          SRC_TYPE مقدار;                                 \
          کپی_حافظه(&مقدار, src8, حجم(مقدار));            \
          M3_BSWAP_##SRC_TYPE(مقدار);                     \
          DEST_TYPE نتیجه = (DEST_TYPE)مقدار;            \
          * dest = نتیجه;  REG = نتیجه;                 \
          d_m3_Trace_بارگیری(DEST_TYPE, عملوند, نتیجه);      \
          بعدی_Op_Preloaded ();                             \
      } وإلا d_بیرون_از_Bounds;                               \
  }

d_m3_بارگیری_Fold(_r0, i32, i8)
d_m3_بارگیری_Fold(_r0, i32, u8)
d_m3_بارگیری_Fold(_r0, i32, i16)
d_m3_بارگیری_Fold(_r0, i32, u16)
d_m3_بارگیری_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_بارگیری_Fold(_r0, i64, i8)
d_m3_بارگیری_Fold(_r0, i64, u8)
d_m3_بارگیری_Fold(_r0, i64, i16)
d_m3_بارگیری_Fold(_r0, i64, u16)
d_m3_بارگیری_Fold(_r0, i64, i32)
d_m3_بارگیری_Fold(_r0, i64, u32)
d_m3_بارگیری_Fold(_r0, i64, i64)
#  إذا d_m3HasFloat
d_m3_بارگیری_Fold(_fp0, f32, f32)
d_m3_بارگیری_Fold(_fp0, f64, f64)
#  endif

#پایان_اگر // d_m3FoldSetLocal

#تعریف_کن d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملوند = جایگاه (u32);                           \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_بعدی ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (DEST_TYPE) <= _mem->درازی    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملوند, REG);     \
            u8* mem8 = m3_Mem_داده(_mem) + عملوند;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            کپی_حافظه(mem8, &val, حجم(val));            \
        }                                               \
        بعدی_Op_Preloaded ();                             \
    } وإلا d_بیرون_از_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE مقدار = جایگاه (SRC_TYPE);             \
    u64 عملوند = (u32) _r0;                            \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_بعدی ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (DEST_TYPE) <= _mem->درازی    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملوند, مقدار);   \
            u8* mem8 = m3_Mem_داده(_mem) + عملوند;       \
            DEST_TYPE val = (DEST_TYPE) مقدار;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            کپی_حافظه(mem8, &val, حجم(val));            \
        }                                               \
        بعدی_Op_Preloaded ();                             \
    } وإلا d_بیرون_از_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE مقدار = جایگاه (SRC_TYPE);             \
    u64 عملوند = جایگاه (u32);                           \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_بعدی ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (DEST_TYPE) <= _mem->درازی    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملوند, مقدار);   \
            u8* mem8 = m3_Mem_داده(_mem) + عملوند;       \
            DEST_TYPE val = (DEST_TYPE) مقدار;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            کپی_حافظه(mem8, &val, حجم(val));            \
        }                                               \
        بعدی_Op_Preloaded ();                             \
    } وإلا d_بیرون_از_Bounds;                               \
}

// both operands can be in regs when storing a float
#تعریف_کن d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملوند = (u32) _r0;                            \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_بعدی ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (TYPE) <= _mem->درازی         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, عملوند, REG);         \
            u8* mem8 = m3_Mem_داده(_mem) + عملوند;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            کپی_حافظه(mem8, &val, حجم(val));            \
        }                                               \
        بعدی_Op_Preloaded ();                             \
    } وإلا d_بیرون_از_Bounds;                               \
}


#تعریف_کن d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#تعریف_کن d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#إذا d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#پایان_اگر

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#ضمن "m3_exec_atomic.h"

#لغو_تعریف m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#إذا d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    حرف name[100];
    strcpy(name, جستجوی_رشته(i_opcode, "op_") + 3);
    حرف* bracket = جستجوی_رشته(name, "(");
    إذا (bracket) {
        *bracket = 0;
    }

    puts(name);
    بعدی_Op_مستقیم();
}
#پایان_اگر

#إذا d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_اصابت(i_operationName);

    بعدی_Op_مستقیم();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_اصابت(i_operationName);

    بعدی_Op_مستقیم();
}
#پایان_اگر

d_m3_انتها_Extern_C

#پایان_اگر // m3_exec_h
