//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#که_نه_تعریف_شوی m3_exec_h
#تعریف_کړه m3_exec_h

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

#که_نه_تعریف_شوی M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#د_که_پای

#ضمن "m3_math_utils.h"
#ضمن "m3_compile.h"
#ضمن "m3_env.h"
#ضمن "m3_info.h"
#ضمن "m3_exec_defs.h"
#ضمن "m3_atomic.h"
#ضمن "m3_host.h"

#ضمن <limits.h>

d_m3BeginExternC

#تعریف_کړه rewrite_op(OP)             * ((فراغ **) (_pc-1)) = (فراغ*)(OP)

#تعریف_کړه سمدستي(TYPE)            * ((TYPE *) _pc++)
#تعریف_کړه skip_سمدستي(TYPE)       (_pc++)

#تعریف_کړه خونه(TYPE)                 * (TYPE *) (_sp + سمدستي (i32))
#تعریف_کړه خونه_ptr(TYPE)             (TYPE *) (_sp + سمدستي (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#تعریف_کړه d_m3_Wide_عملوند(IS64)      ((IS64) ? خونه (u64) : (u64) خونه (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#تعریف_کړه d_m3MemRangeOk(پیل_2, اوږدتیا_2, MEM)  \
   ((پیل_2) <= (MEM)->اوږدتیا and (اوږدتیا_2) <= (MEM)->اوږدتیا - (پیل_2))


#إذا d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define بل_Op()                 M3_MUSTTAIL أرجع profileOp (d_m3_Op_All_دلیلونه, __FUNCTION__)
#وإلاإذا d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define بل_Op()                 M3_MUSTTAIL أرجع debugOp (d_m3_Op_All_دلیلونه, __FUNCTION__)
#وإلا
#  define بل_Op()                 بل_Op_مستقیم()
#د_که_پای

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#إذا d_m3_Preload_بل_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_بل()        ثابت IM3Operation بل_Op_2 = (IM3Operation) (* _pc)
#  define بل_Op_Preloaded()        M3_MUSTTAIL أرجع بل_Op_2 (_pc + 1, d_m3_Op_دلیلونه)
#وإلا
#  define d_m3_Preload_بل()
#  define بل_Op_Preloaded()        بل_Op()
#د_که_پای

#إذا d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL أرجع profileJumpOp ((pc_t)(PC), d_m3_Op_دلیلونه, __FUNCTION__)
#وإلا
#  define jumpOp(PC)               jump_Op_مستقیم(PC)
#د_که_پای

#إذا d_m3_ریکارډ_Backtraces
#  define ټېل_Backtrace_چوکاټ_2()            (ټېل_Backtrace_چوکاټ (_mem->runtime, _pc - 1))
#  define fill_Backtrace_چوکاټ(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    أرجع (ټېل_Backtrace_چوکاټ_2 (), err)
#  define forwardTrap(err)                أرجع err
#وإلا
#  define ټېل_Backtrace_چوکاټ_2()            افعل {} طالما (0)
#  define fill_Backtrace_چوکاټ(FUNCTION)    افعل {} طالما (0)

#  define newTrap(err)                    أرجع err
#  define forwardTrap(err)                أرجع err
#د_که_پای

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#إذا d_m3_Max_Native_ډېری > 0
#  define d_m3_Check_Native_ډېری()                                               \
       افعل {                                                                    \
           فراغ * m3_حد = m3MemRuntime (_mem)->ډېری_حد;                  \
           إذا (M3_UNLIKELY (m3_حد && m3_Native_ډېری_Ptr () < m3_حد))      \
               newTrap (m3_Err_trap_ډېری_ډکېدنه);                              \
       } طالما (0)
#وإلا
#  define d_m3_Check_Native_ډېری()           افعل {} طالما (0)
#د_که_پای


#إذا d_m3_Has_ډېری_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  إذا d_m3HasFloat
#    define d_m3_Exp_Reg_دلیلونه(R0, FP0)        (R0), (FP0)
#  وإلا
#    define d_m3_Exp_Reg_دلیلونه(R0, FP0)        (R0)
#  endif

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_دلیلونه (R0, FP0), d_m3_بنسټ_Cstr)
#  وإلا
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_دلیلونه (R0, FP0))
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
m3ret_t ریکارډ_Call_چوکاټ (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظه i_حافظه,
                         IM3Function i_function, m3reg_t i_r0
#  إذا d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3_چوکاټ چوکاټ;

    چوکاټ.ډول = چوکاټ_call;
    چوکاټ.pc = i_pc;
    چوکاټ.sp = i_sp;
    چوکاټ.حافظه = i_حافظه;
    چوکاټ.call.function = i_function;
    چوکاټ.call.r0 = i_r0;
#  إذا d_m3HasFloat
    چوکاټ.call.fp0 = i_fp0;
#  endif

    أرجع Continuation_ریکارډ_چوکاټ(i_runtime, &چوکاټ);
}

#  define d_m3_ریکارډ_Call_چوکاټ(حافظه_2, FUNCTION)                                \
       إذا (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           أرجع ریکارډ_Call_چوکاټ (m3MemRuntime (_mem), _pc, _sp, (حافظه_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_دلیلونه (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
ساكن M3_NOINLINE
m3ret_t ریکارډ_Loop_چوکاټ (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظه i_حافظه)
{
    M3_چوکاټ چوکاټ;

    چوکاټ.ډول = چوکاټ_loop;
    چوکاټ.pc = i_pc;
    چوکاټ.sp = i_sp;
    چوکاټ.حافظه = i_حافظه;

    أرجع Continuation_ریکارډ_چوکاټ(i_runtime, &چوکاټ);
}


#  إذا d_m3HasExceptionHandling
ساكن M3_NOINLINE
m3ret_t ریکارډ_Try_چوکاټ (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_حافظه i_حافظه,
                        u32 i_عدد_Clauses, bool i_handlers_ژوندی)
{
    M3_چوکاټ چوکاټ;

    چوکاټ.ډول = چوکاټ_try;
    چوکاټ.pc = i_clauses;
    چوکاټ.sp = i_sp;
    چوکاټ.حافظه = i_حافظه;
    چوکاټ.try_.عدد_Clauses = i_عدد_Clauses;
    چوکاټ.try_.handlers_ژوندی = i_handlers_ژوندی;

    أرجع Continuation_ریکارډ_چوکاټ(i_runtime, &چوکاټ);
}
#  endif


#  إذا d_m3_ثبت_Keeps_چوکاټ
ساكن M3_NOINLINE
m3ret_t ریکارډ_ثبت_چوکاټ (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظه i_حافظه,
                          IM3Function i_function)
{
    M3_چوکاټ چوکاټ;

    چوکاټ.ډول = چوکاټ_ثبت;
    چوکاټ.pc = i_pc;
    چوکاټ.sp = i_sp;
    چوکاټ.حافظه = i_حافظه;
    چوکاټ.ثبت.function = i_function;

    أرجع Continuation_ریکارډ_چوکاټ(i_runtime, &چوکاټ);
}
#  endif
#وإلا
#  define d_m3_ریکارډ_Call_چوکاټ(حافظه_2, FUNCTION)
#د_که_پای


#إذا d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#وإلاإذا d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ثابت IM3Runtime trace_rt = m3MemRuntime(_mem); (فراغ) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_ژوروالی)*2, "", ##__VA_ARGS__)
#وإلا
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#د_که_پای

#إذا d_m3EnableStrace >= 3
#  define d_m3_Trace_بارول(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#وإلا
#  define d_m3_Trace_بارول(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#د_که_پای

#که_تعریف_شوی DEBUG
  #تعریف_کړه d_بهر_د_Bounds newTrap (ErrorRuntime (m3_Err_trap_بهر_د_Bounds_حافظه_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->اوږدتیا, عملوند))

#  define d_بهر_د_Bounds_Mem_Op(دننه_واټن, اندازه_2) newTrap (ErrorRuntime (m3_Err_trap_بهر_د_Bounds_حافظه_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->اوږدتیا, دننه_واټن, اندازه_2))
#وإلا
  #تعریف_کړه d_بهر_د_Bounds newTrap (m3_Err_trap_بهر_د_Bounds_حافظه_Access)

#  define d_بهر_د_Bounds_Mem_Op(دننه_واټن, اندازه_2) newTrap (m3_Err_trap_بهر_د_Bounds_حافظه_Access)

#د_که_پای

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#وإلا
d_m3RetSig Call (d_m3OpSig)
#د_که_پای
{
    m3ret_t possible_trap = m3_Yield();
    إذا (M3_UNLIKELY(possible_trap)) {
        أرجع possible_trap;
    }

    بل_Op_مستقیم();
}

#تعریف_کړه d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE عملوند = خونه (TYPE);                         \
    d_m3_Preload_بل ();                                 \
    OP((RES), عملوند, ((TYPE) REG), ##__VA_ARGS__);    \
    بل_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = خونه (TYPE);                        \
    TYPE operand1 = خونه (TYPE);                        \
    d_m3_Preload_بل ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    بل_Op_Preloaded ();                                 \
}

#تعریف_کړه d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE عملوند = خونه (TYPE);                         \
    d_m3_Preload_بل ();                                 \
    OP((RES), ((TYPE) REG), عملوند, ##__VA_ARGS__);    \
    بل_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#تعریف_کړه d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کړه d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کړه d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کړه d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#تعریف_کړه M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#تعریف_کړه M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#تعریف_کړه M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#تعریف_کړه M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#تعریف_کړه d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#تعریف_کړه d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#تعریف_کړه d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#تعریف_کړه d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#تعریف_کړه d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#تعریف_کړه d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#تعریف_کړه d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#تعریف_کړه d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#تعریف_کړه d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#تعریف_کړه d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#تعریف_کړه d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#تعریف_کړه d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#تعریف_کړه d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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
#د_که_پای

#تعریف_کړه OP_جمع_کړه_32(A,B) (i32)((u32)(A) + (u32)(B))
#تعریف_کړه OP_جمع_کړه_64(A,B) (i64)((u64)(A) + (u64)(B))
#تعریف_کړه OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#تعریف_کړه OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#تعریف_کړه OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#تعریف_کړه OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, جمع_کړه,      OP_جمع_کړه_32)  d_m3CommutativeOpFunc_i (i64, جمع_کړه,      OP_جمع_کړه_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, کمول,            OP_SUB_32)  d_m3OpFunc_i (i64, کمول,            OP_SUB_64)

#تعریف_کړه OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#تعریف_کړه OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#تعریف_کړه OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#تعریف_کړه OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, ښويدنه_کیڼ,       OP_SHL_32)      d_m3OpFunc_i (u64, ښويدنه_کیڼ,       OP_SHL_64)
d_m3OpFunc_i (i32, ښويدنه_ښی,      OP_SHR_32)      d_m3OpFunc_i (i64, ښويدنه_ښی,      OP_SHR_64)
d_m3OpFunc_i (u32, ښويدنه_ښی,      OP_SHR_32)      d_m3OpFunc_i (u64, ښويدنه_ښی,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#إذا d_m3_Fold_سېټ_Local

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
    TYPE عملوند = خونه (TYPE);                         \
    TYPE * dest = خونه_ptr (TYPE);                      \
    d_m3_Preload_بل ();                                 \
    TYPE پایله;                                        \
    OP((پایله), عملوند, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = پایله;  REG = پایله;                     \
    بل_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = خونه (TYPE);                        \
    TYPE operand1 = خونه (TYPE);                        \
    TYPE * dest = خونه_ptr (TYPE);                      \
    d_m3_Preload_بل ();                                 \
    TYPE پایله;                                        \
    OP((پایله), operand1, operand2, ##__VA_ARGS__);    \
    * dest = پایله;  REG = پایله;                     \
    بل_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE عملوند = خونه (TYPE);                         \
    TYPE * dest = خونه_ptr (TYPE);                      \
    d_m3_Preload_بل ();                                 \
    TYPE پایله;                                        \
    OP((پایله), ((TYPE) REG), عملوند, ##__VA_ARGS__); \
    * dest = پایله;  REG = پایله;                     \
    بل_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#تعریف_کړه d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کړه d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کړه d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کړه d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, جمع_کړه,          M3_FUNC, OP_جمع_کړه_32)
d_m3FoldOp_i           (i32, کمول,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, ښويدنه_کیڼ,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, ښويدنه_ښی,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, ښويدنه_ښی,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, جمع_کړه,          M3_FUNC, OP_جمع_کړه_64)
d_m3FoldOp_i           (i64, کمول,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, ښويدنه_کیڼ,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, ښويدنه_ښی,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, ښويدنه_ښی,   M3_FUNC, OP_SHR_64)

#  إذا d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, جمع_کړه,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, کمول,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, جمع_کړه,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, کمول,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#د_که_پای

#د_که_پای // d_m3FoldSetLocal

#إذا d_m3HasFloat
d_m3CommutativeArithOp_f(f32, جمع_کړه,         +)      d_m3CommutativeArithOp_f(f64, جمع_کړه,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, کمول,               -)      d_m3ArithOp_f(f64, کمول,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#د_که_پای

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
#د_که_پای

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#تعریف_کړه d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    بل_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE عملوند = خونه (TYPE);                     \
    OP((RES), عملوند, ##__VA_ARGS__);              \
    بل_Op ();                                      \
}

#تعریف_کړه M3_UNARY(RES, X, OP) (RES) = OP(X)
#تعریف_کړه M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#تعریف_کړه d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#تعریف_کړه d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#تعریف_کړه d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#إذا d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       جذر);
d_m3UnaryArithOp_f(f32, تر_ټولو_نږدې,    rintf);    d_m3UnaryArithOp_f(f64, تر_ټولو_نږدې,    rint);
#  إذا defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  وإلا
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#د_که_پای

#تعریف_کړه OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_منزل_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_منزل_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#إذا (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#تعریف_کړه OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#تعریف_کړه OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#تعریف_کړه OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#تعریف_کړه OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#وإلاإذا defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#تعریف_کړه OP_CLZ_32(x) __builtin_clz(x)
#تعریف_کړه OP_CTZ_32(x) __builtin_ctz(x)
#تعریف_کړه OP_CLZ_64(x) __builtin_clzll(x)
#تعریف_کړه OP_CTZ_64(x) __builtin_ctzll(x)
#وإلا
#تعریف_کړه OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#تعریف_کړه OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#تعریف_کړه OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#تعریف_کړه OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#د_که_پای

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#تعریف_کړه OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    بل_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 عملوند = خونه(i64);
    _r0 = OP_WRAP_I64(عملوند);
    بل_Op();
}

// Integer sign extension operations
#تعریف_کړه OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#تعریف_کړه OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#تعریف_کړه OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#تعریف_کړه OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#تعریف_کړه OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

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
    u64  rhs_لوړ = خونه (u64);
    u64  rhs_ټیټ  = خونه (u64);
    u64  lhs_لوړ = خونه (u64);
    u64  lhs_ټیټ  = خونه (u64);
    u64* ټیټ     = خونه_ptr (u64);
    d_m3_Preload_بل();

    u64 مجموعه = lhs_ټیټ + rhs_ټیټ;

    *ټیټ = مجموعه;
    _r0  = (i64) (lhs_لوړ + rhs_لوړ + (مجموعه < lhs_ټیټ));

    بل_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_لوړ = خونه (u64);
    u64  rhs_ټیټ  = خونه (u64);
    u64  lhs_لوړ = خونه (u64);
    u64  lhs_ټیټ  = خونه (u64);
    u64* ټیټ     = خونه_ptr (u64);
    d_m3_Preload_بل();

    *ټیټ = lhs_ټیټ - rhs_ټیټ;
    _r0  = (i64) (lhs_لوړ - rhs_لوړ - (lhs_ټیټ < rhs_ټیټ));

    بل_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = خونه (i64);
    i64  a   = خونه (i64);
    u64* ټیټ = خونه_ptr (u64);
    d_m3_Preload_بل();

    u64 لوړ;
    *ټیټ = mul_wide_i64 (a, b, &لوړ);
    _r0  = (i64) لوړ;

    بل_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = خونه (u64);
    u64  a   = خونه (u64);
    u64* ټیټ = خونه_ptr (u64);
    d_m3_Preload_بل();

    u64 لوړ;
    *ټیټ = mul_wide_u64 (a, b, &لوړ);
    _r0  = (i64) لوړ;

    بل_Op_Preloaded();
}

#د_که_پای // d_m3HasWideArithmetic

#تعریف_کړه d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    بل_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * ډېری = خونه_ptr (FROM);                 \
    OP((DEST), (* ډېری), ##__VA_ARGS__);           \
    بل_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = خونه_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    بل_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * ډېری = خونه_ptr (FROM);                 \
    TYPE * dest = خونه_ptr (TYPE);                  \
    OP((* dest), (* ډېری), ##__VA_ARGS__);         \
    بل_Op ();                                      \
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
#د_که_پای

#تعریف_کړه d_m3TypeModifyOp(REG_منزل, REG_FROM, منزل_2, NAME, FROM)  \
d_m3Op(منزل_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_منزل = (منزل_2) ((FROM) REG_FROM);                        \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = خونه (FROM);                                \
    REG_منزل = (منزل_2) (from);                                   \
    بل_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#تعریف_کړه d_m3FloatModifyOp(منزل_2, NAME, FROM)                   \
d_m3Op(منزل_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((منزل_2) ((FROM) _fp0));             \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = خونه (FROM);                                \
    _fp0 = canon_nan_##TO ((منزل_2) (from));                    \
    بل_Op ();                                              \
}

#إذا d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#د_که_پای

#تعریف_کړه d_m3TypeConvertOp(REG_منزل, REG_FROM, منزل_2, NAME, FROM) \
d_m3Op(منزل_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_منزل = (منزل_2) ((FROM) REG_FROM);                        \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    خونه (منزل_2) = (منزل_2) ((FROM) REG_FROM);                     \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = خونه (FROM);                                \
    REG_منزل = (منزل_2) (from);                                   \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = خونه (FROM);                                \
    خونه (منزل_2) = (منزل_2) (from);                                \
    بل_Op ();                                              \
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
#د_که_پای

#تعریف_کړه d_m3ReinterpretOp(REG, منزل_2, SRC, FROM)               \
d_m3Op(منزل_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = خونه (FROM);                                      \
    REG = u.t;                                              \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    خونه (منزل_2) = u.t;                                        \
    بل_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = خونه (FROM);                                      \
    خونه (منزل_2) = u.t;                                        \
    بل_Op ();                                              \
}

#إذا d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#د_که_پای

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = سمدستي(u32*);
    خونه(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    بل_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = سمدستي(u64*);
    خونه(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    بل_Op();
}


d_m3Op(سېټ_Global_i32)
{
    u32* global = سمدستي(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    بل_Op();
}


d_m3Op(سېټ_Global_i64)
{
    u64* global = سمدستي(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    بل_Op();
}


#إذا d_m3_Has_ډېری_Switching

d_m3Op(Branch);
d_m3Op(ثبت_2);
#  إذا !d_m3_ثبت_Keeps_چوکاټ
d_m3Op(ثبت_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
ساكن M3_NOINLINE
IM3Function Function_د_Compiled_کوډ (pc_t i_pc)
{
    إذا (not i_pc) {
        أرجع عدم;
    }

    طالما ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation ثبت = (IM3Operation)i_pc[0];

#  إذا !d_m3_ثبت_Keeps_چوکاټ
    إذا (ثبت == op_ثبت_Suspendable) {
        أرجع (IM3Function)i_pc[1];
    }
#  endif
    أرجع (ثبت == op_ثبت) ? (IM3Function)i_pc[1] : عدم;
}

#د_که_پای

d_m3Op(Call)
{
    d_m3_Check_Native_ډېری();

    pc_t      callPC = سمدستي(pc_t);
    i32       ډېری_دننه_واټن = سمدستي(i32);
    IM3_حافظه حافظه = m3MemInfo(_mem);

    m3stack_t sp = _sp + ډېری_دننه_واټن;

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_دلیلونه, d_m3_بنسټ_Cstr);
#وإلا
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_دلیلونه);
#د_که_پای

    _mem = حافظه->mallocated;

    إذا (M3_LIKELY(not r)) {
        بل_Op();
    } وإلا {
        d_m3_ریکارډ_Call_چوکاټ(حافظه, Function_د_Compiled_کوډ(callPC));
        ټېل_Backtrace_چوکاټ_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = خونه(IM3Function);
    IM3FuncType type = سمدستي(IM3FuncType);
    i32         ډېری_دننه_واټن = سمدستي(i32);
    IM3_حافظه   حافظه = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ډېری_دننه_واټن;

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(function)) {
        إذا (M3_LIKELY(type == function->funcType)) {
            إذا (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            إذا (M3_LIKELY(not r)) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_دلیلونه, d_m3_بنسټ_Cstr);
#وإلا
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_دلیلونه);
#د_که_پای

                _mem = حافظه->mallocated;

                إذا (M3_LIKELY(not r)) {
                    بل_Op_مستقیم();
                } وإلا {
                    d_m3_ریکارډ_Call_چوکاټ(حافظه, function);
                    ټېل_Backtrace_چوکاټ_2();
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
    IM3Function reference = خونه(IM3Function);

    إذا (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    بل_Op();
}


d_m3Op(Call_غیرمستقیم)
{
    d_m3_Check_Native_ډېری();

    M3_جدول*    جدول_2 = سمدستي(M3_جدول*);
    u64         جدول_شاخص = d_m3_Wide_عملوند(جدول_2->isTable64);
    IM3FuncType type = سمدستي(IM3FuncType);
    i32         ډېری_دننه_واټن = سمدستي(i32);
    IM3_حافظه   حافظه = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ډېری_دننه_واټن;

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
                    M3_حافظه_سرلیک* calleeMem = Module_حافظه_سرلیک(function->module);

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_دلیلونه, d_m3_بنسټ_Cstr);
#وإلا
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_دلیلونه);
#د_که_پای

                    _mem = حافظه->mallocated;

                    إذا (M3_LIKELY(not r)) {
                        بل_Op_مستقیم();
                    } وإلا {
                        d_m3_ریکارډ_Call_چوکاټ(حافظه, function);
                        ټېل_Backtrace_چوکاټ_2();
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
        r = m3_Err_trap_جدول_شاخص_بهر_د_Range;
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
#تعریف_کړه d_m3_لکۍ_Call_دلیلونه(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((فراغ *) (_sp + (RETURNSLOTS)), (ثابت فراغ *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * حجم (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = سمدستي(pc_t);
    i32     ډېری_دننه_واټن = سمدستي(i32);
    i32     returnSlots = سمدستي(i32);
    u32     عدد_Arg_Slots = سمدستي(u32);

    m3ret_t possible_trap = m3_Yield();
    إذا (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_لکۍ_Call_دلیلونه(returnSlots, ډېری_دننه_واټن, عدد_Arg_Slots);

    jump_Op_مستقیم(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = خونه(IM3Function);
    IM3FuncType type = سمدستي(IM3FuncType);
    i32         ډېری_دننه_واټن = سمدستي(i32);
    i32         returnSlots = سمدستي(i32);
    u32         عدد_Arg_Slots = سمدستي(u32);

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(function)) {
        إذا (M3_LIKELY(type == function->funcType)) {
            إذا (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            إذا (M3_LIKELY(not r)) {
                r = m3_Yield();

                إذا (M3_LIKELY(not r)) {
                    d_m3_لکۍ_Call_دلیلونه(returnSlots, ډېری_دننه_واټن, عدد_Arg_Slots);

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
    M3_جدول*    جدول_2 = سمدستي(M3_جدول*);
    u64         جدول_شاخص = d_m3_Wide_عملوند(جدول_2->isTable64);
    IM3FuncType type = سمدستي(IM3FuncType);
    i32         ډېری_دننه_واټن = سمدستي(i32);
    i32         returnSlots = سمدستي(i32);
    u32         عدد_Arg_Slots = سمدستي(u32);

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
                        d_m3_لکۍ_Call_دلیلونه(returnSlots, ډېری_دننه_واټن, عدد_Arg_Slots);

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
        r = m3_Err_trap_جدول_شاخص_بهر_د_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = سمدستي(IM3Function);
    ctx.userdata = سمدستي(فراغ*);
    u64* ثابت sp = ((u64*)_sp);
    IM3_حافظه  حافظه = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#إذا d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    حرف        outbuff[1024];
    حرف*       outp = outbuff;
    حرف*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.ډګر_Utf8);

    ثابت صحيح n_دلیلونه = ftype->عدد_دلیلونه;
    ثابت صحيح nRets = ftype->عدد_Rets;
    u64*      دلیلونه = sp + nRets;
    لكل (صحيح i = 0; i < n_دلیلونه; i++) {
        ثابت صحيح type = ftype->types[nRets + i];
        بدل (type) {
        حالة c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(دلیلونه + i)); اكسر;
        حالة c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(دلیلونه + i)); اكسر;
        حالة c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(دلیلونه + i)); اكسر;
        حالة c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(دلیلونه + i)); اكسر;
        افتراضي: outp += snprintf(outp, oute - outp, "<type %d>", type); اكسر;
        }
        إذا (i < n_دلیلونه - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  إذا d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#د_که_پای

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    فراغ* ډېری_backup = runtime->ډېری;
    runtime->ډېری = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_معلومات(ctx.function->host_حافظه->mallocated));
    runtime->ډېری = ډېری_backup;

#إذا d_m3EnableStrace
    إذا (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (حرف*)possible_trap);
    } وإلا إذا (ftype and ftype->عدد_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } وإلا {
        d_m3TracePrint("%s", outbuff);
    }
#د_که_پای

    إذا (M3_UNLIKELY(possible_trap)) {
        _mem = حافظه->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        إذا (possible_trap != m3Err_pendingException) {
            ټېل_Backtrace_چوکاټ_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_اندازه)
{
    IM3_حافظه حافظه = m3MemInfo(_mem);

    _r0 = حافظه->عدد_پاڼې;

    بل_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_حافظه  حافظه = m3MemInfo(_mem);

    i32        عدد_پاڼې_منزل_Grow = (i32)_r0;
    إذا (عدد_پاڼې_منزل_Grow >= 0) {
        _r0 = (m3reg_t)حافظه->عدد_پاڼې;

        إذا (M3_LIKELY(عدد_پاڼې_منزل_Grow)) {
            u64      required_پاڼې = حافظه->عدد_پاڼې + (u32)عدد_پاڼې_منزل_Grow;

            M3_پایله r = Resize_حافظه(runtime, حافظه, required_پاڼې);
            إذا (r) {
                _r0 = -1;
            }

            _mem = حافظه->mallocated;
        }
    } وإلا {
        _r0 = -1;
    }

    بل_Op();
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

    u64        عدد_پاڼې_منزل_Grow = (u64)_r0;
    u64        عدد_پاڼې = حافظه->عدد_پاڼې;

    إذا (M3_LIKELY(عدد_پاڼې_منزل_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        إذا (عدد_پاڼې_منزل_Grow <= حافظه->max_پاڼې - عدد_پاڼې and
            not Resize_حافظه(runtime, حافظه, عدد_پاڼې + عدد_پاڼې_منزل_Grow)) {
            _r0 = (m3reg_t)عدد_پاڼې;
        } وإلا {
            _r0 = -1;
        }

        _mem = حافظه->mallocated;
    } وإلا {
        _r0 = (m3reg_t)عدد_پاڼې;
    }

    بل_Op();
}

#د_که_پای // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 اندازه = (u32)_r0;
    u64 سرچینه = خونه(u32);
    u64 destination = خونه(u32);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(سرچینه, اندازه, _mem))) {
            u8* dst = m3_Mem_معلومات(_mem) + destination;
            u8* src = m3_Mem_معلومات(_mem) + سرچینه;
            memmove(dst, src, (size_t)اندازه);

            بل_Op();
        } وإلا {
            d_بهر_د_Bounds_Mem_Op(سرچینه, اندازه);
        }
    } وإلا {
        d_بهر_د_Bounds_Mem_Op(destination, اندازه);
    }
}


#إذا d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 اندازه = (u64)_r0;
    u64 سرچینه = خونه(u64);
    u64 destination = خونه(u64);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(سرچینه, اندازه, _mem))) {
            u8* dst = m3_Mem_معلومات(_mem) + destination;
            u8* src = m3_Mem_معلومات(_mem) + سرچینه;
            memmove(dst, src, (size_t)اندازه);

            بل_Op();
        } وإلا {
            d_بهر_د_Bounds_Mem_Op(سرچینه, اندازه);
        }
    } وإلا {
        d_بهر_د_Bounds_Mem_Op(destination, اندازه);
    }
}

#د_که_پای // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(سېټ_حافظه)
{
    IM3_حافظه حافظه = سمدستي(IM3_حافظه);

    _mem = حافظه->mallocated;

    بل_Op();
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
    IM3_حافظه       dest_حافظه = سمدستي(IM3_حافظه);
    IM3_حافظه       سرچینه_حافظه = سمدستي(IM3_حافظه);
    u32             widths = سمدستي(u32);

    M3_حافظه_سرلیک* destMem = dest_حافظه->mallocated;
    M3_حافظه_سرلیک* سرچینه_Mem = سرچینه_حافظه->mallocated;

    u64             اندازه = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             سرچینه = d_m3_Wide_عملوند(widths & 0x2);
    u64             destination = d_m3_Wide_عملوند(widths & 0x1);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, destMem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(سرچینه, اندازه, سرچینه_Mem))) {
            memmove(m3_Mem_معلومات(destMem) + destination, m3_Mem_معلومات(سرچینه_Mem) + سرچینه, (size_t)اندازه);

            بل_Op();
        } وإلا {
            d_بهر_د_Bounds_Mem_Op(سرچینه, اندازه);
        }
    } وإلا {
        d_بهر_د_Bounds_Mem_Op(destination, اندازه);
    }
}

#د_که_پای // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 اندازه = (u32)_r0;
    u32 byte = خونه(u32);
    u64 destination = خونه(u32);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        u8* mem8 = m3_Mem_معلومات(_mem) + destination;
        memset(mem8, (u8)byte, اندازه);
        بل_Op();
    } وإلا {
        d_بهر_د_Bounds_Mem_Op(destination, اندازه);
    }
}


#إذا d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 اندازه = (u64)_r0;
    u32 byte = خونه(u32);
    u64 destination = خونه(u64);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        u8* mem8 = m3_Mem_معلومات(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)اندازه);
        بل_Op();
    } وإلا {
        d_بهر_د_Bounds_Mem_Op(destination, اندازه);
    }
}

#د_که_پای // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_معلومات_Segment* segment = سمدستي(M3_معلومات_Segment*);

    u64            اندازه = (u32)_r0;
    u64            سرچینه = خونه(u32);
    u64            destination = خونه(u32);

    u64            شته = segment->dropped ? 0 : segment->اندازه;

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(سرچینه <= شته and اندازه <= شته - سرچینه)) {
            حافظه_نقل(m3_Mem_معلومات(_mem) + destination, segment->معلومات + سرچینه, (size_t)اندازه);
            بل_Op();
        } وإلا {
            d_بهر_د_Bounds_Mem_Op(سرچینه, اندازه);
        }
    } وإلا {
        d_بهر_د_Bounds_Mem_Op(destination, اندازه);
    }
}


#إذا d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_معلومات_Segment* segment = سمدستي(M3_معلومات_Segment*);

    u64            اندازه = (u32)_r0;
    u64            سرچینه = خونه(u32);
    u64            destination = خونه(u64);

    u64            شته = segment->dropped ? 0 : segment->اندازه;

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, اندازه, _mem))) {
        إذا (M3_LIKELY(سرچینه <= شته and اندازه <= شته - سرچینه)) {
            حافظه_نقل(m3_Mem_معلومات(_mem) + destination, segment->معلومات + سرچینه, (size_t)اندازه);
            بل_Op();
        } وإلا {
            d_بهر_د_Bounds_Mem_Op(سرچینه, اندازه);
        }
    } وإلا {
        d_بهر_د_Bounds_Mem_Op(destination, اندازه);
    }
}

#د_که_پای // d_m3HasMemory64


d_m3Op(معلومات_Drop)
{
    M3_معلومات_Segment* segment = سمدستي(M3_معلومات_Segment*);
    segment->dropped = true;

    بل_Op();
}


#إذا d_m3HasRefTypes

d_m3Op(جدول_Get)
{
    M3_جدول* جدول_2 = سمدستي(M3_جدول*);
    u64      index = d_m3_Wide_عملوند(جدول_2->isTable64);

    إذا (M3_LIKELY(index < جدول_2->اندازه)) {
        _r0 = (u64)(uintptr_t)جدول_2->elements[index];
        بل_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بهر_د_Bounds);
    }
}


d_m3Op(جدول_سېټ)
{
    M3_جدول* جدول_2 = سمدستي(M3_جدول*);
    فراغ*    ارزښت = خونه(فراغ*);
    u64      index = d_m3_Wide_عملوند(جدول_2->isTable64);

    إذا (M3_LIKELY(index < جدول_2->اندازه)) {
        جدول_2->elements[index] = ارزښت;
        بل_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بهر_د_Bounds);
    }
}


d_m3Op(جدول_اندازه)
{
    M3_جدول* جدول_2 = سمدستي(M3_جدول*);

    _r0 = جدول_2->اندازه;

    بل_Op();
}


d_m3Op(جدول_Grow)
{
    M3_جدول* جدول_2 = سمدستي(M3_جدول*);
    u64      delta = d_m3_Wide_عملوند(جدول_2->isTable64);
    فراغ*    ارزښت = خونه(فراغ*);

    u32      old_اندازه = جدول_2->اندازه;
    u32      max_اندازه = جدول_2->max_اندازه ? جدول_2->max_اندازه : d_m3_Max_Sane_جدول_اندازه;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_اندازه = (delta <= max_اندازه - old_اندازه) ? (u64)old_اندازه + delta
                                                    : (u64)max_اندازه + 1;

    إذا (new_اندازه == old_اندازه)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_اندازه;
        بل_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (new_اندازه <= max_اندازه and (not runtime->جدول_Elements_حد or
                                delta <= runtime->جدول_Elements_حد - runtime->جدول_Elements_کارول_شوی)) {
        فراغ** elements = m3_ReallocArray(فراغ*, جدول_2->elements, (size_t)new_اندازه, old_اندازه);

        إذا (elements) {
            runtime->جدول_Elements_کارول_شوی += new_اندازه - old_اندازه;
            جدول_2->elements = elements;
            جدول_2->اندازه = (u32)new_اندازه;

            لكل (u32 i = old_اندازه; i < جدول_2->اندازه; ++i) {
                جدول_2->elements[i] = ارزښت;
            }

            _r0 = old_اندازه;
            بل_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    بل_Op();
}


d_m3Op(جدول_Init)
{
    M3_جدول*          جدول_2 = سمدستي(M3_جدول*);
    M3ElementSegment* segment = سمدستي(M3ElementSegment*);
    u32               شمېر = خونه(u32);
    u32               سرچینه = خونه(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_عملوند(جدول_2->isTable64);

    u64               شته = segment->dropped ? 0 : segment->عدد_Elements;

    إذا (M3_LIKELY(destination <= جدول_2->اندازه and شمېر <= جدول_2->اندازه - destination and
                  (u64) سرچینه + شمېر <= شته)) {
        لكل (u32 i = 0; i < شمېر; ++i) {
            جدول_2->elements[destination + i] = segment->resolved[سرچینه + i];
        }

        بل_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بهر_د_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = سمدستي(M3ElementSegment*);
    segment->dropped = true;

    بل_Op();
}


d_m3Op(جدول_Copy)
{
    M3_جدول* dst = سمدستي(M3_جدول*);
    M3_جدول* src = سمدستي(M3_جدول*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      شمېر = d_m3_Wide_عملوند(dst->isTable64 and src->isTable64);
    u64      سرچینه = d_m3_Wide_عملوند(src->isTable64);
    u64      destination = d_m3_Wide_عملوند(dst->isTable64);

    إذا (M3_LIKELY(destination <= dst->اندازه and شمېر <= dst->اندازه - destination and
                  سرچینه <= src->اندازه and شمېر <= src->اندازه - سرچینه)) {
        memmove(dst->elements + destination, src->elements + سرچینه, (size_t)شمېر * حجم(فراغ*));

        بل_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بهر_د_Bounds);
    }
}


d_m3Op(جدول_Fill)
{
    M3_جدول* جدول_2 = سمدستي(M3_جدول*);
    u64      شمېر = d_m3_Wide_عملوند(جدول_2->isTable64);
    فراغ*    ارزښت = خونه(فراغ*);
    u64      index = d_m3_Wide_عملوند(جدول_2->isTable64);

    إذا (M3_LIKELY(index <= جدول_2->اندازه and شمېر <= جدول_2->اندازه - index)) {
        لكل (u64 i = 0; i < شمېر; ++i) {
            جدول_2->elements[index + i] = ارزښت;
        }

        بل_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_بهر_د_Bounds);
    }
}

#د_که_پای // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = سمدستي(IM3Function);

    m3ret_t     پایله = m3Err_none;

    إذا (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        پایله = CompileFunction(function);
    }

    إذا (not پایله) {
        // patch up compiled pc and call rewritten op_Call
        *((فراغ**)--_pc) = (فراغ*)(function->compiled);
        --_pc;
        بل_Op_مستقیم();
    }

    newTrap(پایله);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = سمدستي(IM3Function);

    m3ret_t     پایله = m3Err_none;

    إذا (M3_UNLIKELY(not function->compiled)) {
        پایله = CompileFunction(function);
    }

    إذا (not پایله) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((فراغ**)--_pc) = (فراغ*)(function->compiled);
        --_pc;
        بل_Op_مستقیم();
    }

    newTrap(پایله);
}


#إذا d_m3_Has_ډېری_Switching
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
فراغ Suspend_Without_نښان (IM3Runtime io_runtime, IM3Continuation io_cont, M3_خوندي_ټکی_ډول i_ډول,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  إذا d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_نښان = عدم;
    io_runtime->suspendHandlerCont = عدم;
    io_runtime->suspendStubPC = عدم;
    io_runtime->switch_موخه = عدم;
    io_runtime->عدد_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspend_ټکی = (u8)i_ډول;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  إذا d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->عدد_Suspend_Results = 0;
}
#د_که_پای // d_m3HasStackSwitching


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
    u32        لګښت = سمدستي(u32);

    runtime->gas_پاتې_شوی -= (i64)لګښت;

    إذا (M3_UNLIKELY(runtime->gas_پاتې_شوی < 0)) {
#  إذا d_m3_Has_ډېری_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        إذا (runtime->isSuspendable and runtime->فعال_Continuation) {
            runtime->suspendRequested = true;
            بل_Op();
        }
#  endif
        newTrap(m3_Err_trap_بهر_د_Gas);
    }

    بل_Op();
}

#د_که_پای // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
ساكن مضمن
bool ثبت_ځاییږي (IM3Function i_function, m3stack_t _sp, M3_حافظه_سرلیک* _mem)
{
#إذا d_m3_Skip_ډېری_Check
    (فراغ)i_function;
    (فراغ)_sp;
    (فراغ)_mem;
    أرجع true;
#وإلاإذا d_m3_Has_ډېری_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->فعال_Continuation : عدم);
    فراغ*           max_ډېری = (_act && _act->val_ډېری)
                                 ? (فراغ*)(_act->val_ډېری + _act->عدد_ډېری_Slots)
                                 : (_mem ? _mem->max_ډېری : (فراغ*)~(uintptr_t)0);
    أرجع M3_LIKELY((فراغ*)(_sp + i_function->max_ډېری_Slots) < max_ډېری);
#وإلا
    أرجع M3_LIKELY((فراغ*)(_sp + i_function->max_ډېری_Slots) < _mem->max_ډېری);
#د_که_پای
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
ساكن مضمن
فراغ Enter_چوکاټ (IM3Function i_function, m3stack_t _sp)
{
#إذا defined(DEBUG)
    i_function->موندنې++;
#د_که_پای
    u8* ډېری = (u8*)((m3slot_t*)_sp + i_function->عدد_Ret_And_Arg_Slots);

    memset(ډېری, 0x0, i_function->عدد_Local_بایټونه);
    ډېری += i_function->عدد_Local_بایټونه;

    إذا (i_function->constants) {
        حافظه_نقل(ډېری, i_function->constants, i_function->عدد_Constant_بایټونه);
    }
}


d_m3Op(ثبت_2)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = سمدستي(IM3Function);
#إذا d_m3_ثبت_Keeps_چوکاټ
    IM3_حافظه حافظه = m3MemInfo(_mem);
#د_که_پای

    إذا (ثبت_ځاییږي(function, _sp, _mem)) {
        Enter_چوکاټ(function, _sp);

#إذا !d_m3_ثبت_Keeps_چوکاټ
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        بل_Op_مستقیم();
#وإلا

#  إذا d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->عدد_Ret_Slots));
        trace_rt->call_ژوروالی++;
#  endif

        m3ret_t r = بل_Op_Impl();

#  إذا d_m3EnableStrace >= 2
        trace_rt->call_ژوروالی--;

        إذا (r) {
            d_m3TracePrint("} !trap = %s", (حرف*)r);
        } وإلا إذا (Get_Function_عدد_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } وإلا {
            d_m3TracePrint("}");
        }
#  endif

        إذا (M3_UNLIKELY(r)) {
            _mem = حافظه->mallocated;
#  إذا d_m3_Has_ډېری_Switching
            إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                أرجع ریکارډ_ثبت_چوکاټ(m3MemRuntime(_mem), _pc, _sp, حافظه, function);
            }
#  endif
            fill_Backtrace_چوکاټ();
        }
        forwardTrap(r);
#د_که_پای // d_m3EntryKeepsFrame
    } وإلا {
        newTrap(m3_Err_trap_ډېری_ډکېدنه);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_ډېری();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_حافظه حافظه = m3MemInfo(_mem);

    افعل {
#إذا d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_ژوروالی++;
#د_که_پای
        r = بل_Op_Impl();

#إذا d_m3EnableStrace >= 3
        trace_rt->call_ژوروالی--;
        d_m3TracePrint("}");
#د_که_پای
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = حافظه->mallocated;
    } طالما (r == _pc);

#إذا d_m3_Has_ډېری_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        أرجع ریکارډ_Loop_چوکاټ(m3MemRuntime(_mem), _pc, _sp, حافظه);
    }
#د_که_پای

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
    d_m3_Check_Native_ډېری();

    IM3_حافظه  حافظه = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        عدد_Clauses = سمدستي(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * عدد_Clauses;

    u32        ژوروالی = ++runtime->try_ژوروالی;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = حافظه->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_ژوروالی == ژوروالی);
    runtime->try_ژوروالی = ژوروالی - 1;

#  إذا d_m3_Has_ډېری_Switching
    إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        أرجع ریکارډ_Try_چوکاټ(runtime, clauses, _sp, حافظه, عدد_Clauses, isInnermost);
    }
#  endif

    إذا (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ثابت M3Exception* exception = runtime->pendingException;

        لكل (u32 i = 0; i < عدد_Clauses; ++i) {
            IM3_نښان نښان = (IM3_نښان)clauses[2 * i];

            إذا (نښان == عدم or نښان == exception->نښان) {
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

    u32          عدد_دلیلونه = سمدستي(u32);

    لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
        i32 offset = سمدستي(i32);
        u32 is64 = سمدستي(u32);
        u64 ارزښت = exception->دلیلونه[i];

        إذا (is64) {
            *(u64*)(_sp + offset) = ارزښت;
        } وإلا {
            *(u32*)(_sp + offset) = (u32)ارزښت;
        }
    }

    i32 ref_دننه_واټن = سمدستي(i32);

    إذا (ref_دننه_واټن >= 0) {
        *(M3Exception**)(_sp + ref_دننه_واټن) = exception;
        exception->reified = true;
    } وإلا إذا (not exception->reified) {
        FreeException(runtime, exception);
    }

    بل_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_نښان       نښان = سمدستي(IM3_نښان);
    u32          عدد_دلیلونه = سمدستي(u32);

    M3Exception* exception = NewException(runtime, نښان, عدد_دلیلونه);

    إذا (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
        i32 offset = سمدستي(i32);
        u32 is64 = سمدستي(u32);

        exception->دلیلونه[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = خونه(M3Exception*);

    إذا (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    أرجع m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(راوباسه_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        شمېر = سمدستي(u32);

    runtime->try_ژوروالی -= شمېر;

    بل_Op();
}

#د_که_پای // d_m3HasExceptionHandling

#إذا d_m3_Has_ډېری_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = سمدستي(IM3FuncType);
    i32         dst_خونه = سمدستي(i32);
    i32         func_خونه = سمدستي(i32);

    IM3Function func = *(IM3Function*)(_sp + func_خونه);
    إذا (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    إذا (M3_UNLIKELY(not Continuation_Can_Acquire_ډېری(runtime))) {
        newTrap(m3_Err_continuation_حد_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_پایله ویش = Continuation_Acquire_ډېری(runtime, cont);
    إذا (M3_UNLIKELY(ویش)) {
        cont->state = cont_consumed;
        newTrap(ویش);
    }
    *(IM3Continuation*)(_sp + dst_خونه) = cont;
    بل_Op();
}


ساكن
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = سمدستي(IM3FuncType);
    IM3FuncType     ct2 = سمدستي(IM3FuncType);
    i32             dst_خونه = سمدستي(i32);
    i32             cont_خونه = سمدستي(i32);
    u32             عدد_Bound = سمدستي(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + cont_خونه);
    إذا (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->ثبت_Function);
    إذا (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->ثبت_Function = srcCont->ثبت_Function;
    dstCont->val_ډېری = srcCont->val_ډېری;
    dstCont->عدد_ډېری_Slots = srcCont->عدد_ډېری_Slots;
    dstCont->چوکاټونه = srcCont->چوکاټونه;
    dstCont->چوکاټونه_Cap = srcCont->چوکاټونه_Cap;
    dstCont->عدد_چوکاټونه = srcCont->عدد_چوکاټونه;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspend_ټکی = srcCont->suspend_ټکی;
    dstCont->r0 = srcCont->r0;
#  إذا d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_دلیلونه_شمېر = srcCont->bound_دلیلونه_شمېر;
    dstCont->state = srcCont->state;
#  إذا d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = عدم;
#  endif

    dstCont->عدد_Suspend_Results = srcCont->عدد_Suspend_Results;
    لكل (u32 j = 0; j < srcCont->عدد_Suspend_Results; ++j) {
        dstCont->suspend_پایله_Offsets[j] = srcCont->suspend_پایله_Offsets[j];
        dstCont->suspend_پایله_Is64[j] = srcCont->suspend_پایله_Is64[j];
    }

    إذا (dstCont->state == cont_allocated) {
        u16 عدد_Rets = ct1->contFuncType ? Get_Func_Type_عدد_Results(ct1->contFuncType) : 0;
        لكل (u32 i = 0; i < عدد_Bound; ++i) {
            i32 arg_خونه = سمدستي(i32);
            u32 is64 = سمدستي(u32);
            u32 موخه_Idx = (عدد_Rets + dstCont->bound_دلیلونه_شمېر) * c_io_خونه_شمېر;
            إذا (is64) {
                *(u64*)(dstCont->val_ډېری + موخه_Idx) = *(u64*)(_sp + arg_خونه);
            } وإلا {
                *(u32*)(dstCont->val_ډېری + موخه_Idx) = *(u32*)(_sp + arg_خونه);
            }
            dstCont->bound_دلیلونه_شمېر++;
        }
    } وإلا {
        IM3Continuation inner = InnermostSuspended(dstCont);
        لكل (u32 i = 0; i < عدد_Bound; ++i) {
            i32 arg_خونه = سمدستي(i32);
            u32 is64 = سمدستي(u32);
            u32 dstIdx = dstCont->bound_دلیلونه_شمېر;
            إذا (dstIdx < inner->عدد_Suspend_Results) {
                i32 dst_دننه_واټن = inner->suspend_پایله_Offsets[dstIdx];
                إذا (is64) {
                    *(u64*)(inner->sp + dst_دننه_واټن) = *(u64*)(_sp + arg_خونه);
                } وإلا {
                    *(u32*)(inner->sp + dst_دننه_واټن) = *(u32*)(_sp + arg_خونه);
                }
            }
            dstCont->bound_دلیلونه_شمېر++;
        }
    }

    srcCont->val_ډېری = عدم;
    srcCont->چوکاټونه = عدم;
    srcCont->چوکاټونه_Cap = 0;
    srcCont->عدد_چوکاټونه = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dst_خونه) = dstCont;
    بل_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
ساكن
IM3Continuation پیدا_کړه_Handler (IM3Continuation i_cont, IM3_نښان i_نښان, u8 i_ډول, pc_t* o_stubPC)
{
    IM3_نښان query_نښان = (i_نښان and i_نښان->resolved) ? i_نښان->resolved : i_نښان;

    لكل (IM3Continuation cont = i_cont; cont; cont = cont->پلار) {
        لكل (u32 h = 0; h < cont->عدد_Handlers; ++h) {
            pc_t   ثبت = cont->handlersPC + (h * 3);
            IM3_نښان handler_نښان = (IM3_نښان)(*(ثبت + 1));
            إذا (handler_نښان and handler_نښان->resolved) {
                handler_نښان = handler_نښان->resolved;
            }

            إذا ((u32)(uintptr_t)(*ثبت) == i_ډول and handler_نښان == query_نښان) {
                إذا (o_stubPC) {
                    *o_stubPC = (pc_t)(*(ثبت + 2));
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
    لكل (u32 i = 0; i < io_cont->عدد_Suspend_Results; ++i) {
        إذا (io_cont->suspend_پایله_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_پایله_Offsets[i]) = 0;
        } وإلا {
            *(m3slot_t*)(i_sp + io_cont->suspend_پایله_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->فعال_Continuation;
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_نښان);
    }

    IM3_نښان نښان = سمدستي(IM3_نښان);
    u32    عدد_Params = سمدستي(u32);

    runtime->عدد_Suspend_Payload = عدد_Params;
    لكل (u32 i = 0; i < عدد_Params; ++i) {
        i32 param_خونه = سمدستي(i32);
        u32 is64 = سمدستي(u32);
        إذا (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + param_خونه);
        } وإلا {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + param_خونه);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 عدد_Results = سمدستي(u32);
    cont->عدد_Suspend_Results = عدد_Results;
    لكل (u32 i = 0; i < عدد_Results; ++i) {
        cont->suspend_پایله_Offsets[i] = سمدستي(i32);
        cont->suspend_پایله_Is64[i] = سمدستي(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = عدم;
    IM3Continuation پلټنه_Cont = پیدا_کړه_Handler(cont, نښان, 0, &matchedStubPC);

    إذا (M3_UNLIKELY(not پلټنه_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_نښان);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = پلټنه_Cont;
    runtime->switch_موخه = عدم;
    runtime->suspend_نښان = نښان;

    cont->pc = _pc;
    cont->suspend_ټکی = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  إذا d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_دلیلونه_شمېر = 0;

    أرجع m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
ساكن
m3ret_t پرلپسې_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_عدد_Handlers, M3_حافظه_سرلیک** io_mem);
ساكن
فراغ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_عدد_Results);
ساكن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_حافظه_سرلیک* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
ساكن مضمن
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    أرجع pass and (i_cont->suspend_ټکی == safepoint_op or i_cont->suspend_ټکی == safepoint_ثبت);
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
m3ret_t Replay_چوکاټونه (IM3Continuation i_cont, i32 i_ژوروالی, M3_حافظه_سرلیک* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  إذا d_m3_Max_Native_ډېری > 0
    {
        فراغ* حد = runtime->ډېری_حد;

        إذا (M3_UNLIKELY(حد and m3_Native_ډېری_Ptr() < حد)) {
            أرجع m3_Err_trap_ډېری_ډکېدنه;
        }
    }
#  endif

    إذا (i_ژوروالی < 0) {
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

    M3_چوکاټ چوکاټ = i_cont->چوکاټونه[i_ژوروالی];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_چوکاټ_ډول)چوکاټ.ډول == چوکاټ_resume);

#  إذا d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_چوکاټ_ډول)چوکاټ.ډول == چوکاټ_try);
#  endif

    إذا (not ownDescent) {
        r = Replay_چوکاټونه(i_cont, i_ژوروالی - 1, _mem);
        _mem = چوکاټ.حافظه->mallocated;
    }

    بدل ((M3_چوکاټ_ډول)چوکاټ.ډول) {
    حالة چوکاټ_call:
        إذا (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            أرجع d_m3CallWithRegs(چوکاټ.pc, چوکاټ.sp, _mem, چوکاټ.call.r0, چوکاټ.call.fp0);
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            أرجع Continuation_ریکارډ_چوکاټ(runtime, &چوکاټ);
        }
#  إذا d_m3_ریکارډ_Backtraces
        ټېل_Backtrace_چوکاټ(runtime, چوکاټ.pc - 1);
#  endif
        أرجع r;

    حالة چوکاټ_loop: {
        m3stack_t _sp = چوکاټ.sp;
        m3reg_t   _r0 = 0;
#  إذا d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        طالما (r == (m3ret_t)چوکاټ.pc) {
            r = jumpOpImpl(چوکاټ.pc);
            _mem = چوکاټ.حافظه->mallocated;
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            أرجع Continuation_ریکارډ_چوکاټ(runtime, &چوکاټ);
        }
        أرجع r;
    }

#  إذا d_m3HasExceptionHandling
    حالة چوکاټ_try: {
        m3stack_t _sp = چوکاټ.sp;
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
        u32 ژوروالی = ++runtime->try_ژوروالی;

        إذا (not چوکاټ.try_.handlers_ژوندی) {
            runtime->try_ژوروالی = ژوروالی - 1;
        }

        r = Replay_چوکاټونه(i_cont, i_ژوروالی - 1, _mem);
        _mem = چوکاټ.حافظه->mallocated;

        bool isInnermost = (runtime->try_ژوروالی == ژوروالی);
        runtime->try_ژوروالی = ژوروالی - 1;

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            چوکاټ.try_.handlers_ژوندی = isInnermost;
            أرجع Continuation_ریکارډ_چوکاټ(runtime, &چوکاټ);
        }

        إذا (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ثابت M3Exception* exception = runtime->pendingException;

            لكل (u32 i = 0; i < چوکاټ.try_.عدد_Clauses; ++i) {
                IM3_نښان نښان = (IM3_نښان)چوکاټ.pc[2 * i];

                إذا (نښان == عدم or نښان == exception->نښان) {
                    pc_t handler = (pc_t)چوکاټ.pc[2 * i + 1];

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

    حالة چوکاټ_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = چوکاټ.resume.cont;

        r = پرلپسې_Under_Handlers(runtime, &inner, چوکاټ.resume.handlersPC,
                             چوکاټ.resume.عدد_Handlers, &_mem);

        إذا (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, چوکاټ.sp, چوکاټ.resume.resultsPC,
                                    چوکاټ.resume.عدد_Results);
            Continuation_Recycle_ډېری(runtime, inner);

            أرجع d_m3CallWithRegs(چوکاټ.pc, چوکاټ.sp, _mem, 0, 0.);
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            إذا (runtime->suspendHandlerCont != inner) {
                چوکاټ.resume.cont = inner;
                أرجع Continuation_ریکارډ_چوکاټ(runtime, &چوکاټ);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            إذا (M3_UNLIKELY(not suspended)) {
                أرجع m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            إذا (runtime->suspendStubPC) {
                أرجع d_m3CallWithRegs(runtime->suspendStubPC, چوکاټ.sp, _mem, 0, 0.);
            } وإلا {
                أرجع m3_Err_trap_Unhandled_Control_نښان;
            }
        } وإلا {
            inner->state = cont_consumed;
            Continuation_Recycle_ډېری(runtime, inner);
        }

        أرجع r;
    }

#  إذا d_m3_ثبت_Keeps_چوکاټ
    حالة چوکاټ_ثبت:
        إذا (M3_UNLIKELY(r)) {
            إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                أرجع Continuation_ریکارډ_چوکاټ(runtime, &چوکاټ);
            }
#    إذا d_m3_ریکارډ_Backtraces
            FillBacktraceFunctionInfo(runtime, چوکاټ.ثبت.function);
#    وإلا
            (فراغ)چوکاټ.ثبت.function;
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
        u16         عدد_Rets = inner ? Get_Func_Type_عدد_Results(inner) : 0;

        أرجع i_cont->val_ډېری + (عدد_Rets + i_شاخص) * c_io_خونه_شمېر;
    }

    إذا (i_شاخص < i_cont->عدد_Suspend_Results) {
        أرجع i_cont->sp + i_cont->suspend_پایله_Offsets[i_شاخص];
    }

    أرجع عدم;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
ساكن
فراغ Hand_Over_منزل_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 عدد_دلیلونه = i_runtime->عدد_Suspend_Payload;
    u32 بنسټ = (io_peer->state == cont_allocated) ? io_peer->bound_دلیلونه_شمېر : 0;

    لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
        فراغ* dest = ContinuationArgDest(io_peer, بنسټ + i);

        إذا (dest) {
            إذا (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } وإلا {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    فراغ* refDest = ContinuationArgDest(io_peer, بنسټ + عدد_دلیلونه);

    إذا (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    إذا (io_peer->state == cont_allocated) {
        io_peer->bound_دلیلونه_شمېر += عدد_دلیلونه + 1;
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
    طالما (i_cont->عدد_چوکاټونه and (M3_چوکاټ_ډول) i_cont->چوکاټونه[0].ډول == چوکاټ_resume) {
        i_cont = i_cont->چوکاټونه[0].resume.cont;
    }

    أرجع i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
ساكن
m3ret_t پرلپسې_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_عدد_Handlers, M3_حافظه_سرلیک** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation پلار = i_runtime->فعال_Continuation;
    IM3_حافظه       حافظه = m3MemInfo(*io_mem);
    m3ret_t         r;

    لكل (;;) {
#  إذا d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        إذا (M3_UNLIKELY(cont->resumeThrow and cont->عدد_چوکاټونه == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = عدم;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            اكسر;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        إذا (isStarting) {
            إذا (M3_UNLIKELY(not cont->ثبت_Function->compiled)) {
                r = CompileFunction(cont->ثبت_Function);

                إذا (M3_UNLIKELY(r)) {
                    اكسر;
                }
            }
            cont->pc = cont->ثبت_Function->compiled;
            cont->sp = cont->val_ډېری;
            cont->عدد_چوکاټونه = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->عدد_Handlers = i_عدد_Handlers;
        cont->پلار = پلار;
        cont->state = cont_running;
        cont->bound_دلیلونه_شمېر = 0;

        i_runtime->فعال_Continuation = cont;

        إذا (cont->عدد_چوکاټونه) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->عدد_چوکاټونه - 1;
            cont->عدد_چوکاټونه = 0;

            r = Replay_چوکاټونه(cont, outermost, *io_mem);
        } وإلا إذا (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } وإلا {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = حافظه->mallocated;
        i_runtime->فعال_Continuation = پلار;

        إذا (r != m3Err_continuationSuspended) {
            اكسر;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        إذا (not i_runtime->switch_موخه or i_runtime->suspendHandlerCont != cont) {
            اكسر;
        }

        IM3Continuation peer = i_runtime->switch_موخه;
        i_runtime->switch_موخه = عدم;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        إذا (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            اكسر;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_Cont_Type_د_Ref(peer->ثبت_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_عدد_Params(peerType) - 1)));
        Hand_Over_منزل_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    أرجع r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
ساكن
فراغ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_عدد_Results)
{
    لكل (u32 i = 0; i < i_عدد_Results; ++i) {
        i32 res_خونه = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_خونه_شمېر;

        إذا (is64) {
            *(u64*)(i_sp + res_خونه) = *(u64*)(i_cont->val_ډېری + srcIdx);
        } وإلا {
            *(u32*)(i_sp + res_خونه) = *(u32*)(i_cont->val_ډېری + srcIdx);
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
    IM3FuncType     contFuncType = سمدستي(IM3FuncType);
    i32             cont_خونه = سمدستي(i32);
    u32             ډېرپېښ = سمدستي(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + cont_خونه);
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         عدد_Rets = Get_Func_Type_عدد_Results(innerType);

    إذا (ډېرپېښ == d_m3ResumeThrowRef) {
        i32 exn_خونه = سمدستي(i32);

#  إذا d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exn_خونه);

        إذا (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  وإلا
        (فراغ)exn_خونه;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } وإلا إذا (ډېرپېښ == d_m3ResumeThrow) {
        IM3_نښان exn_نښان = سمدستي(IM3_نښان);
        u32    عدد_دلیلونه = سمدستي(u32);

#  إذا d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_نښان, عدد_دلیلونه);

        إذا (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
            i32 offset = سمدستي(i32);
            u32 is64 = سمدستي(u32);

            exception->دلیلونه[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  وإلا
        (فراغ)exn_نښان;
        (فراغ)عدد_دلیلونه;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } وإلا {
        u32 عدد_دلیلونه = سمدستي(u32);

        إذا (cont->state == cont_allocated) {
            لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
                i32 arg_خونه = سمدستي(i32);
                u32 is64 = سمدستي(u32);
                u32 موخه_Idx = (عدد_Rets + cont->bound_دلیلونه_شمېر + i) * c_io_خونه_شمېر;
                إذا (is64) {
                    *(u64*)(cont->val_ډېری + موخه_Idx) = *(u64*)(_sp + arg_خونه);
                } وإلا {
                    *(u32*)(cont->val_ډېری + موخه_Idx) = *(u32*)(_sp + arg_خونه);
                }
            }
            cont->bound_دلیلونه_شمېر = 0;
        } وإلا {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
                i32 arg_خونه = سمدستي(i32);
                u32 is64 = سمدستي(u32);
                u32 dstIdx = cont->bound_دلیلونه_شمېر + i;
                إذا (dstIdx < inner->عدد_Suspend_Results) {
                    i32 dst_دننه_واټن = inner->suspend_پایله_Offsets[dstIdx];
                    إذا (is64) {
                        *(u64*)(inner->sp + dst_دننه_واټن) = *(u64*)(_sp + arg_خونه);
                    } وإلا {
                        *(u32*)(inner->sp + dst_دننه_واټن) = *(u32*)(_sp + arg_خونه);
                    }
                }
            }
        }
    }

    u32  عدد_Handlers = سمدستي(u32);
    pc_t handlersPC = _pc;
    _pc += عدد_Handlers * 3;

    u32       عدد_Results = سمدستي(u32);
    pc_t      resultsPC = _pc;

    IM3_حافظه حافظه = m3MemInfo(_mem);

    m3ret_t   r = پرلپسې_Under_Handlers(runtime, &cont, handlersPC, عدد_Handlers, &_mem);

    إذا (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, عدد_Results);
        Continuation_Recycle_ډېری(runtime, cont);

        _pc += عدد_Results * 2;
        بل_Op();
    } وإلا إذا (r == m3Err_continuationSuspended) {
        إذا (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            إذا (M3_UNLIKELY(not runtime->فعال_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_نښان);
            }
            M3_چوکاټ چوکاټ;
            چوکاټ.ډول = چوکاټ_resume;
            چوکاټ.pc = resultsPC + عدد_Results * 2;
            چوکاټ.sp = _sp;
            چوکاټ.حافظه = حافظه;
            چوکاټ.resume.cont = cont;
            چوکاټ.resume.handlersPC = handlersPC;
            چوکاټ.resume.عدد_Handlers = عدد_Handlers;
            چوکاټ.resume.resultsPC = resultsPC;
            چوکاټ.resume.عدد_Results = عدد_Results;

            أرجع Continuation_ریکارډ_چوکاټ(runtime, &چوکاټ);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        إذا (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        إذا (runtime->suspendStubPC) {
            jump_Op_مستقیم(runtime->suspendStubPC);
        } وإلا {
            newTrap(m3_Err_trap_Unhandled_Control_نښان);
        }
    } وإلا {
        cont->state = cont_consumed;
        Continuation_Recycle_ډېری(runtime, cont);
        ټېل_Backtrace_چوکاټ_2();
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
    cont->type = سمدستي(IM3FuncType);

    u32 عدد_دلیلونه = سمدستي(u32);
    لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
        i32 offset = سمدستي(i32);
        u32 is64 = سمدستي(u32);
        u64 val = (i < runtime->عدد_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        إذا (is64) {
            *(u64*)(_sp + offset) = val;
        } وإلا {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_دننه_واټن = سمدستي(i32);
    إذا (cont_دننه_واټن >= 0) {
        *(IM3Continuation*)(_sp + cont_دننه_واټن) = cont;
    }

    بل_Op();
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
        newTrap(m3_Err_trap_Unhandled_Control_نښان);
    }

    skip_سمدستي(IM3FuncType);

    IM3_نښان          نښان = سمدستي(IM3_نښان);
    i32             موخه_خونه = سمدستي(i32);

    IM3Continuation موخه = *(IM3Continuation*)(_sp + موخه_خونه);

    إذا (M3_UNLIKELY(not موخه)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(موخه->state == cont_consumed or موخه->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = پیدا_کړه_Handler(cont, نښان, 1, عدم);

    إذا (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_نښان);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 عدد_دلیلونه = سمدستي(u32);

    runtime->عدد_Suspend_Payload = عدد_دلیلونه;

    لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
        i32 arg_خونه = سمدستي(i32);
        u32 is64 = سمدستي(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + arg_خونه) : (u64) * (u32*)(_sp + arg_خونه);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 عدد_Results = سمدستي(u32);

    cont->عدد_Suspend_Results = عدد_Results;

    لكل (u32 i = 0; i < عدد_Results; ++i) {
        cont->suspend_پایله_Offsets[i] = سمدستي(i32);
        cont->suspend_پایله_Is64[i] = سمدستي(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = عدم;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switch_موخه = موخه;
    runtime->suspend_نښان = نښان;

    cont->pc = _pc;
    cont->suspend_ټکی = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  إذا d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    أرجع m3Err_continuationSuspended;
}


#د_که_پای // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = سمدستي(pc_t);

    إذا (condition) {
        بل_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = خونه(i32);

    pc_t elsePC = سمدستي(pc_t);

    إذا (condition) {
        بل_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_جدول)
{
    u32   branch_شاخص = خونه(u32);           // branch index is always in a slot
    u32   عدد_Targets = سمدستي(u32);

    pc_t* branches = (pc_t*)_pc;

    إذا (branch_شاخص > عدد_Targets) {
        branch_شاخص = عدد_Targets; // the default index
    }

    jumpOp(branches[branch_شاخص]);
}


#تعریف_کړه d_m3_سېټ_ثبتوونکی_سېټ_خونه(TYPE, REG) \
d_m3Op  (سېټ_ثبتوونکی_2##TYPE)            \
{                                       \
    TYPE * src = خونه_ptr (TYPE);       \
    d_m3_Preload_بل ();                 \
    REG = * src;                        \
    بل_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (سېټ_خونه_2##TYPE)                 \
{                                       \
    TYPE * dst = خونه_ptr (TYPE);       \
    d_m3_Preload_بل ();                 \
    * dst = (TYPE) REG;                 \
    بل_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_سېټ_خونه_2##TYPE)         \
{                                       \
    TYPE * ډېری     = خونه_ptr (TYPE); \
    TYPE * preserve  = خونه_ptr (TYPE); \
    d_m3_Preload_بل ();                 \
                                        \
    * preserve = * ډېری;               \
    * ډېری = (TYPE) REG;               \
                                        \
    بل_Op_Preloaded ();                 \
}

d_m3_سېټ_ثبتوونکی_سېټ_خونه(i32, _r0)
d_m3_سېټ_ثبتوونکی_سېټ_خونه(i64, _r0)
#إذا d_m3HasFloat
d_m3_سېټ_ثبتوونکی_سېټ_خونه(f32, _fp0)
d_m3_سېټ_ثبتوونکی_سېټ_خونه(f64, _fp0)
#د_که_پای

d_m3Op(Copy_خونه_32)
{
    u32* dst = خونه_ptr(u32);
    u32* src = خونه_ptr(u32);
    d_m3_Preload_بل();

    *dst = *src;

    بل_Op_Preloaded();
}


d_m3Op(Preserve_Copy_خونه_32)
{
    u32* dest = خونه_ptr(u32);
    u32* src = خونه_ptr(u32);
    u32* preserve = خونه_ptr(u32);
    d_m3_Preload_بل();

    *preserve = *dest;
    *dest = *src;

    بل_Op_Preloaded();
}


d_m3Op(Copy_خونه_64)
{
    u64* dst = خونه_ptr(u64);
    u64* src = خونه_ptr(u64);
    d_m3_Preload_بل();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    بل_Op_Preloaded();
}


d_m3Op(Preserve_Copy_خونه_64)
{
    u64* dest = خونه_ptr(u64);
    u64* src = خونه_ptr(u64);
    u64* preserve = خونه_ptr(u64);
    d_m3_Preload_بل();

    *preserve = *dest;
    *dest = *src;

    بل_Op_Preloaded();
}


#إذا d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_ډېری)
{
    u32         opcode_شاخص = سمدستي(u32);
    u32         ډېری_لوړوالی = سمدستي(u32);
    IM3Function function = سمدستي(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    چاپ_کړه(" %4d ", opcode_شاخص);
    چاپ_کړه(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  إذا d_m3HasFloat
    چاپ_کړه("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    لكل (u32 i = 0; i < ډېری_لوړوالی; ++i) {
        cstr_t ډول = "";

        چاپ_کړه("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, ډول, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    چاپ_کړه("---------------------------------------------------------------------------------------------------------\n");

    بل_Op_مستقیم();
}
#د_که_پای


#تعریف_کړه d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = خونه (TYPE);                \
    TYPE operand1 = خونه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بل_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = خونه (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = خونه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بل_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = خونه (i32);                 \
                                                \
    TYPE operand2 = خونه (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بل_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = خونه (i32);                 \
                                                \
    TYPE operand2 = خونه (TYPE);                \
    TYPE operand1 = خونه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بل_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#تعریف_کړه d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = خونه (TYPE);                \
    TYPE operand1 = خونه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بل_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = خونه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بل_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = خونه (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    بل_Op ();                                  \
}

#إذا d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, خونه(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, خونه(i32))
#د_که_پای

d_m3Op(Return)
{
    m3_ډېری_Check();
    أرجع m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = سمدستي(pc_t);

    إذا (condition) {
        jumpOp(branch);
    } وإلا {
        بل_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = خونه(i32);
    pc_t branch = سمدستي(pc_t);

    إذا (condition) {
        jumpOp(branch);
    } وإلا {
        بل_Op();
    }
}

#إذا d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE عملوند = خونه (TYPE);                             \
      pc_t branch  = سمدستي (pc_t);                        \
      إذا (عملوند OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      وإلا بل_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE عملوند = خونه (TYPE);                             \
      pc_t branch  = سمدستي (pc_t);                        \
      إذا (((TYPE) _r0) OPER عملوند)                          \
          jumpOp (branch);                                    \
      وإلا بل_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = خونه (TYPE);                            \
      TYPE operand1 = خونه (TYPE);                            \
      pc_t branch   = سمدستي (pc_t);                       \
      إذا (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      وإلا بل_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE عملوند = خونه (TYPE);                             \
      pc_t elsePC  = سمدستي (pc_t);                        \
      إذا (عملوند OPER ((TYPE) _r0))                          \
          بل_Op ();                                          \
      وإلا jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE عملوند = خونه (TYPE);                             \
      pc_t elsePC  = سمدستي (pc_t);                        \
      إذا (((TYPE) _r0) OPER عملوند)                          \
          بل_Op ();                                          \
      وإلا jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = خونه (TYPE);                            \
      TYPE operand1 = خونه (TYPE);                            \
      pc_t elsePC   = سمدستي (pc_t);                       \
      إذا (operand1 OPER operand2)                             \
          بل_Op ();                                          \
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
    pc_t branch = سمدستي(pc_t);
    إذا (((i32)_r0) == 0) {
        jumpOp(branch);
    } وإلا {
        بل_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  عملوند = خونه(i32);
    pc_t branch = سمدستي(pc_t);
    إذا (عملوند == 0) {
        jumpOp(branch);
    } وإلا {
        بل_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = سمدستي(pc_t);
    إذا (((i32)_r0) == 0) {
        بل_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  عملوند = خونه(i32);
    pc_t elsePC = سمدستي(pc_t);
    إذا (عملوند == 0) {
        بل_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}

#د_که_پای // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = سمدستي(pc_t);

    إذا (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        بل_Op();
    } وإلا {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = خونه(i32);
    pc_t branch = سمدستي(pc_t);

    إذا (condition) {
        بل_Op();
    } وإلا {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_ډېری_Check();

    فراغ* loopId = سمدستي(فراغ*);
    أرجع loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    فراغ* loopId = سمدستي(فراغ*);

    إذا (condition) {
        أرجع loopId;
    } وإلا {
        بل_Op();
    }
}


#إذا d_m3_Has_ډېری_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_ډېری_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
        Suspend_Without_نښان(runtime, runtime->فعال_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_دلیلونه(_r0, _fp0));

        أرجع m3Err_continuationSuspended;
    }

    فراغ* loopId = سمدستي(فراغ*);
    أرجع loopId;
}


#  إذا !d_m3_ثبت_Keeps_چوکاټ
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(ثبت_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = سمدستي(IM3Function);

    إذا (ثبت_ځاییږي(function, _sp, _mem)) {
        Enter_چوکاټ(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
            Suspend_Without_نښان(runtime, runtime->فعال_Continuation, safepoint_ثبت, _pc, _sp,
                              d_m3_Exp_Reg_دلیلونه(_r0, _fp0));

            أرجع m3Err_continuationSuspended;
        }

        بل_Op_مستقیم();
    } وإلا {
        newTrap(m3_Err_trap_ډېری_ډکېدنه);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(ثبت_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
        Suspend_Without_نښان(runtime, runtime->فعال_Continuation, safepoint_ثبت, _pc - 1, _sp,
                          d_m3_Exp_Reg_دلیلونه(_r0, _fp0));

        أرجع m3Err_continuationSuspended;
    }

    بل_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    فراغ* loopId = سمدستي(فراغ*);

    إذا (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
            Suspend_Without_نښان(runtime, runtime->فعال_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_دلیلونه(_r0, _fp0));

            أرجع m3Err_continuationSuspended;
        }
        أرجع loopId;
    } وإلا {
        بل_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
ساكن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_حافظه_سرلیک* _mem)
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
    } وإلا إذا (op == op_ثبت_Check) {
        أرجع بل_Op_Impl();
    } وإلا {
        أرجع d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    أرجع op(_pc, d_m3_Op_دلیلونه, d_m3_بنسټ_Cstr);
#  وإلا
    أرجع op(_pc, d_m3_Op_دلیلونه);
#  endif
}
#د_که_پای // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 ارزښت = *(u32*)_pc++;
    خونه(u32) = ارزښت;
    بل_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 ارزښت;
    حافظه_نقل(&ارزښت, _pc, حجم(ارزښت));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    خونه(u64) = ارزښت;
    بل_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_ډېری_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(پای_ټکی_2)
{
    m3_ډېری_Check();
    أرجع m3Err_none;
}


d_m3Op(سېټ_Global_s32)
{
    u32* global = سمدستي(u32*);
    *global = خونه(u32);

    بل_Op();
}


d_m3Op(سېټ_Global_s64)
{
    u64* global = سمدستي(u64*);
    *global = خونه(u64);

    بل_Op();
}

#إذا d_m3HasFloat
d_m3Op(سېټ_Global_f32)
{
    f32* global = سمدستي(f32*);
    *global = (f32)_fp0;

    بل_Op();
}


d_m3Op(سېټ_Global_f64)
{
    f64* global = سمدستي(f64*);
    *global = _fp0;

    بل_Op();
}
#د_که_پای


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
    u64  عملوند = خونه(u64);
    u32* پته = خونه_ptr(u32);

    u64  offset;
    حافظه_نقل(&offset, _pc, حجم(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    إذا (M3_LIKELY(عملوند < d_m3_پته_حد)) {
        u64 effective = عملوند + offset;

        // the access itself checks its own size against what is left
        إذا (M3_LIKELY(effective < _mem->اوږدتیا)) {
            *پته = (u32)effective;
            بل_Op();
        }
    }

    d_بهر_د_Bounds;
}

#د_که_پای // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#إذا d_m3_Skip_حافظه_Bounds_Check || d_m3_Guarded_حافظه
#  define m3MemCheck(x) true
#وإلا
#  define m3MemCheck(x) M3_LIKELY(x)
#د_که_پای

// memcpy here is to support non-aligned access on some platforms.

#تعریف_کړه d_m3_بارول(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = سمدستي (u32);                       \
    d_m3_Preload_بل ();                                 \
    u64 عملوند = (u32) _r0;                            \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (SRC_TYPE) <= _mem->اوږدتیا     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_معلومات(_mem) + عملوند;       \
            SRC_TYPE ارزښت;                             \
            حافظه_نقل(&ارزښت, src8, حجم(ارزښت));        \
            M3_BSWAP_##SRC_TYPE(ارزښت);                 \
            REG = (DEST_TYPE)ارزښت;                     \
            d_m3_Trace_بارول(DEST_TYPE, عملوند, REG);     \
        }                                               \
        بل_Op_Preloaded ();                             \
    } وإلا d_بهر_د_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملوند = خونه (u32);                           \
    u32 offset = سمدستي (u32);                       \
    d_m3_Preload_بل ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (SRC_TYPE) <= _mem->اوږدتیا     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_معلومات(_mem) + عملوند;       \
            SRC_TYPE ارزښت;                             \
            حافظه_نقل(&ارزښت, src8, حجم(ارزښت));        \
            M3_BSWAP_##SRC_TYPE(ارزښت);                 \
            REG = (DEST_TYPE)ارزښت;                     \
            d_m3_Trace_بارول(DEST_TYPE, عملوند, REG);     \
        }                                               \
        بل_Op_Preloaded ();                             \
    } وإلا d_بهر_د_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#تعریف_کړه d_m3_بارول_i(DEST_TYPE, SRC_TYPE) d_m3_بارول(_r0, DEST_TYPE, SRC_TYPE)
#تعریف_کړه d_m3_بارول_f(DEST_TYPE, SRC_TYPE) d_m3_بارول(_fp0, DEST_TYPE, SRC_TYPE)

#إذا d_m3HasFloat
d_m3_بارول_f(f32, f32);
d_m3_بارول_f(f64, f64);
#د_که_پای

d_m3_بارول_i(i32, i8);
d_m3_بارول_i(i32, u8);
d_m3_بارول_i(i32, i16);
d_m3_بارول_i(i32, u16);
d_m3_بارول_i(i32, i32);

d_m3_بارول_i(i64, i8);
d_m3_بارول_i(i64, u8);
d_m3_بارول_i(i64, i16);
d_m3_بارول_i(i64, u16);
d_m3_بارول_i(i64, i32);
d_m3_بارول_i(i64, u32);
d_m3_بارول_i(i64, i64);

#إذا d_m3_Fold_سېټ_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_بارول_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = سمدستي (u32);                       \
      DEST_TYPE * dest = خونه_ptr (DEST_TYPE);            \
      d_m3_Preload_بل ();                                 \
      u64 عملوند = (u32) _r0;                            \
      عملوند += offset;                                  \
                                                          \
      إذا (m3MemCheck(                                     \
          عملوند + حجم (SRC_TYPE) <= _mem->اوږدتیا     \
      )) {                                                \
          u8* src8 = m3_Mem_معلومات(_mem) + عملوند;           \
          SRC_TYPE ارزښت;                                 \
          حافظه_نقل(&ارزښت, src8, حجم(ارزښت));            \
          M3_BSWAP_##SRC_TYPE(ارزښت);                     \
          DEST_TYPE پایله = (DEST_TYPE)ارزښت;            \
          * dest = پایله;  REG = پایله;                 \
          d_m3_Trace_بارول(DEST_TYPE, عملوند, پایله);      \
          بل_Op_Preloaded ();                             \
      } وإلا d_بهر_د_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 عملوند = خونه (u32);                           \
      u32 offset = سمدستي (u32);                       \
      DEST_TYPE * dest = خونه_ptr (DEST_TYPE);            \
      d_m3_Preload_بل ();                                 \
      عملوند += offset;                                  \
                                                          \
      إذا (m3MemCheck(                                     \
          عملوند + حجم (SRC_TYPE) <= _mem->اوږدتیا     \
      )) {                                                \
          u8* src8 = m3_Mem_معلومات(_mem) + عملوند;           \
          SRC_TYPE ارزښت;                                 \
          حافظه_نقل(&ارزښت, src8, حجم(ارزښت));            \
          M3_BSWAP_##SRC_TYPE(ارزښت);                     \
          DEST_TYPE پایله = (DEST_TYPE)ارزښت;            \
          * dest = پایله;  REG = پایله;                 \
          d_m3_Trace_بارول(DEST_TYPE, عملوند, پایله);      \
          بل_Op_Preloaded ();                             \
      } وإلا d_بهر_د_Bounds;                               \
  }

d_m3_بارول_Fold(_r0, i32, i8)
d_m3_بارول_Fold(_r0, i32, u8)
d_m3_بارول_Fold(_r0, i32, i16)
d_m3_بارول_Fold(_r0, i32, u16)
d_m3_بارول_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_بارول_Fold(_r0, i64, i8)
d_m3_بارول_Fold(_r0, i64, u8)
d_m3_بارول_Fold(_r0, i64, i16)
d_m3_بارول_Fold(_r0, i64, u16)
d_m3_بارول_Fold(_r0, i64, i32)
d_m3_بارول_Fold(_r0, i64, u32)
d_m3_بارول_Fold(_r0, i64, i64)
#  إذا d_m3HasFloat
d_m3_بارول_Fold(_fp0, f32, f32)
d_m3_بارول_Fold(_fp0, f64, f64)
#  endif

#د_که_پای // d_m3FoldSetLocal

#تعریف_کړه d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملوند = خونه (u32);                           \
    u32 offset = سمدستي (u32);                       \
    d_m3_Preload_بل ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (DEST_TYPE) <= _mem->اوږدتیا    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملوند, REG);     \
            u8* mem8 = m3_Mem_معلومات(_mem) + عملوند;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            حافظه_نقل(mem8, &val, حجم(val));            \
        }                                               \
        بل_Op_Preloaded ();                             \
    } وإلا d_بهر_د_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE ارزښت = خونه (SRC_TYPE);             \
    u64 عملوند = (u32) _r0;                            \
    u32 offset = سمدستي (u32);                       \
    d_m3_Preload_بل ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (DEST_TYPE) <= _mem->اوږدتیا    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملوند, ارزښت);   \
            u8* mem8 = m3_Mem_معلومات(_mem) + عملوند;       \
            DEST_TYPE val = (DEST_TYPE) ارزښت;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            حافظه_نقل(mem8, &val, حجم(val));            \
        }                                               \
        بل_Op_Preloaded ();                             \
    } وإلا d_بهر_د_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE ارزښت = خونه (SRC_TYPE);             \
    u64 عملوند = خونه (u32);                           \
    u32 offset = سمدستي (u32);                       \
    d_m3_Preload_بل ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (DEST_TYPE) <= _mem->اوږدتیا    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملوند, ارزښت);   \
            u8* mem8 = m3_Mem_معلومات(_mem) + عملوند;       \
            DEST_TYPE val = (DEST_TYPE) ارزښت;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            حافظه_نقل(mem8, &val, حجم(val));            \
        }                                               \
        بل_Op_Preloaded ();                             \
    } وإلا d_بهر_د_Bounds;                               \
}

// both operands can be in regs when storing a float
#تعریف_کړه d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملوند = (u32) _r0;                            \
    u32 offset = سمدستي (u32);                       \
    d_m3_Preload_بل ();                                 \
    عملوند += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عملوند + حجم (TYPE) <= _mem->اوږدتیا         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, عملوند, REG);         \
            u8* mem8 = m3_Mem_معلومات(_mem) + عملوند;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            حافظه_نقل(mem8, &val, حجم(val));            \
        }                                               \
        بل_Op_Preloaded ();                             \
    } وإلا d_بهر_د_Bounds;                               \
}


#تعریف_کړه d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#تعریف_کړه d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#إذا d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#د_که_پای

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#ضمن "m3_exec_atomic.h"

#تعریف_لغوه m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#إذا d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    حرف name[100];
    strcpy(name, مزي_پلټنه(i_opcode, "op_") + 3);
    حرف* bracket = مزي_پلټنه(name, "(");
    إذا (bracket) {
        *bracket = 0;
    }

    puts(name);
    بل_Op_مستقیم();
}
#د_که_پای

#إذا d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_موندنه(i_operationName);

    بل_Op_مستقیم();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_موندنه(i_operationName);

    بل_Op_مستقیم();
}
#د_که_پای

d_m3_پای_ټکی_Extern_C

#د_که_پای // m3_exec_h
