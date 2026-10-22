//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#اگر_غیر_تعریف_شدہ m3_exec_h
#تعریف_کرو m3_exec_h

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

#اگر_غیر_تعریف_شدہ M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#اگر_ختم

#شامل "m3_math_utils.h"
#شامل "m3_compile.h"
#شامل "m3_env.h"
#شامل "m3_info.h"
#شامل "m3_exec_defs.h"
#شامل "m3_atomic.h"
#شامل "m3_host.h"

#شامل <limits.h>

d_m3BeginExternC

#تعریف_کرو rewrite_op(OP)             * ((خالی **) (_pc-1)) = (خالی*)(OP)

#تعریف_کرو فوری(TYPE)            * ((TYPE *) _pc++)
#تعریف_کرو skip_فوری(TYPE)       (_pc++)

#تعریف_کرو خانہ(TYPE)                 * (TYPE *) (_sp + فوری (i32))
#تعریف_کرو خانہ_ptr(TYPE)             (TYPE *) (_sp + فوری (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#تعریف_کرو d_m3_Wide_عملیہ(IS64)      ((IS64) ? خانہ (u64) : (u64) خانہ (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#تعریف_کرو d_m3MemRangeOk(آغاز_2, لمبائی_2, MEM)  \
   ((آغاز_2) <= (MEM)->لمبائی and (لمبائی_2) <= (MEM)->لمبائی - (آغاز_2))


#اگر d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define اگلا_Op()                 M3_MUSTTAIL واپس profileOp (d_m3_Op_All_دلائل, __FUNCTION__)
#وگرنہ_اگر d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define اگلا_Op()                 M3_MUSTTAIL واپس debugOp (d_m3_Op_All_دلائل, __FUNCTION__)
#ورنہ
#  define اگلا_Op()                 اگلا_Op_براہ_راست()
#اگر_ختم

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#اگر d_m3_Preload_اگلا_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_اگلا()        ثابت IM3Operation اگلا_Op_2 = (IM3Operation) (* _pc)
#  define اگلا_Op_Preloaded()        M3_MUSTTAIL واپس اگلا_Op_2 (_pc + 1, d_m3_Op_دلائل)
#ورنہ
#  define d_m3_Preload_اگلا()
#  define اگلا_Op_Preloaded()        اگلا_Op()
#اگر_ختم

#اگر d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL واپس profileJumpOp ((pc_t)(PC), d_m3_Op_دلائل, __FUNCTION__)
#ورنہ
#  define jumpOp(PC)               jump_Op_براہ_راست(PC)
#اگر_ختم

#اگر d_m3_ریکارڈ_Backtraces
#  define ڈالو_Backtrace_چوکھٹا_2()            (ڈالو_Backtrace_چوکھٹا (_mem->runtime, _pc - 1))
#  define fill_Backtrace_چوکھٹا(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    واپس (ڈالو_Backtrace_چوکھٹا_2 (), err)
#  define forwardTrap(err)                واپس err
#ورنہ
#  define ڈالو_Backtrace_چوکھٹا_2()            کرو {} جب (0)
#  define fill_Backtrace_چوکھٹا(FUNCTION)    کرو {} جب (0)

#  define newTrap(err)                    واپس err
#  define forwardTrap(err)                واپس err
#اگر_ختم

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#اگر d_m3_Max_Native_ڈھیر > 0
#  define d_m3_Check_Native_ڈھیر()                                               \
       کرو {                                                                    \
           خالی * m3_حد_بندی = m3MemRuntime (_mem)->ڈھیر_حد_بندی;                  \
           اگر (M3_UNLIKELY (m3_حد_بندی && m3_Native_ڈھیر_Ptr () < m3_حد_بندی))      \
               newTrap (m3_Err_trap_ڈھیر_حد_سے_تجاوز);                              \
       } جب (0)
#ورنہ
#  define d_m3_Check_Native_ڈھیر()           کرو {} جب (0)
#اگر_ختم


#اگر d_m3_Has_ڈھیر_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  اگر d_m3HasFloat
#    define d_m3_Exp_Reg_دلائل(R0, FP0)        (R0), (FP0)
#  ورنہ
#    define d_m3_Exp_Reg_دلائل(R0, FP0)        (R0)
#  endif

#  اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_دلائل (R0, FP0), d_m3_بنیاد_Cstr)
#  ورنہ
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_دلائل (R0, FP0))
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
ساکن M3_NOINLINE
m3ret_t ریکارڈ_Call_چوکھٹا (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظہ i_حافظہ,
                         IM3Function i_function, m3reg_t i_r0
#  اگر d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3_چوکھٹا چوکھٹا;

    چوکھٹا.قسم = چوکھٹا_call;
    چوکھٹا.pc = i_pc;
    چوکھٹا.sp = i_sp;
    چوکھٹا.حافظہ = i_حافظہ;
    چوکھٹا.call.function = i_function;
    چوکھٹا.call.r0 = i_r0;
#  اگر d_m3HasFloat
    چوکھٹا.call.fp0 = i_fp0;
#  endif

    واپس Continuation_ریکارڈ_چوکھٹا(i_runtime, &چوکھٹا);
}

#  define d_m3_ریکارڈ_Call_چوکھٹا(حافظہ_2, FUNCTION)                                \
       اگر (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           واپس ریکارڈ_Call_چوکھٹا (m3MemRuntime (_mem), _pc, _sp, (حافظہ_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_دلائل (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
ساکن M3_NOINLINE
m3ret_t ریکارڈ_Loop_چوکھٹا (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظہ i_حافظہ)
{
    M3_چوکھٹا چوکھٹا;

    چوکھٹا.قسم = چوکھٹا_loop;
    چوکھٹا.pc = i_pc;
    چوکھٹا.sp = i_sp;
    چوکھٹا.حافظہ = i_حافظہ;

    واپس Continuation_ریکارڈ_چوکھٹا(i_runtime, &چوکھٹا);
}


#  اگر d_m3HasExceptionHandling
ساکن M3_NOINLINE
m3ret_t ریکارڈ_Try_چوکھٹا (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_حافظہ i_حافظہ,
                        u32 i_عدد_Clauses, bool i_handlers_زندہ)
{
    M3_چوکھٹا چوکھٹا;

    چوکھٹا.قسم = چوکھٹا_try;
    چوکھٹا.pc = i_clauses;
    چوکھٹا.sp = i_sp;
    چوکھٹا.حافظہ = i_حافظہ;
    چوکھٹا.try_.عدد_Clauses = i_عدد_Clauses;
    چوکھٹا.try_.handlers_زندہ = i_handlers_زندہ;

    واپس Continuation_ریکارڈ_چوکھٹا(i_runtime, &چوکھٹا);
}
#  endif


#  اگر d_m3_اندراج_Keeps_چوکھٹا
ساکن M3_NOINLINE
m3ret_t ریکارڈ_اندراج_چوکھٹا (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_حافظہ i_حافظہ,
                          IM3Function i_function)
{
    M3_چوکھٹا چوکھٹا;

    چوکھٹا.قسم = چوکھٹا_اندراج;
    چوکھٹا.pc = i_pc;
    چوکھٹا.sp = i_sp;
    چوکھٹا.حافظہ = i_حافظہ;
    چوکھٹا.اندراج_2.function = i_function;

    واپس Continuation_ریکارڈ_چوکھٹا(i_runtime, &چوکھٹا);
}
#  endif
#ورنہ
#  define d_m3_ریکارڈ_Call_چوکھٹا(حافظہ_2, FUNCTION)
#اگر_ختم


#اگر d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#وگرنہ_اگر d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ثابت IM3Runtime trace_rt = m3MemRuntime(_mem); (خالی) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_گہرائی)*2, "", ##__VA_ARGS__)
#ورنہ
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#اگر_ختم

#اگر d_m3EnableStrace >= 3
#  define d_m3_Trace_بھرو(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#ورنہ
#  define d_m3_Trace_بھرو(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#اگر_ختم

#اگر_تعریف_شدہ DEBUG
  #تعریف_کرو d_برآمد_کا_Bounds newTrap (ErrorRuntime (m3_Err_trap_برآمد_کا_Bounds_حافظہ_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->لمبائی, عملیہ))

#  define d_برآمد_کا_Bounds_Mem_Op(انحراف, سائز_2) newTrap (ErrorRuntime (m3_Err_trap_برآمد_کا_Bounds_حافظہ_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->لمبائی, انحراف, سائز_2))
#ورنہ
  #تعریف_کرو d_برآمد_کا_Bounds newTrap (m3_Err_trap_برآمد_کا_Bounds_حافظہ_Access)

#  define d_برآمد_کا_Bounds_Mem_Op(انحراف, سائز_2) newTrap (m3_Err_trap_برآمد_کا_Bounds_حافظہ_Access)

#اگر_ختم

#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#ورنہ
d_m3RetSig Call (d_m3OpSig)
#اگر_ختم
{
    m3ret_t possible_trap = m3_Yield();
    اگر (M3_UNLIKELY(possible_trap)) {
        واپس possible_trap;
    }

    اگلا_Op_براہ_راست();
}

#تعریف_کرو d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE عملیہ = خانہ (TYPE);                         \
    d_m3_Preload_اگلا ();                                 \
    OP((RES), عملیہ, ((TYPE) REG), ##__VA_ARGS__);    \
    اگلا_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = خانہ (TYPE);                        \
    TYPE operand1 = خانہ (TYPE);                        \
    d_m3_Preload_اگلا ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    اگلا_Op_Preloaded ();                                 \
}

#تعریف_کرو d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE عملیہ = خانہ (TYPE);                         \
    d_m3_Preload_اگلا ();                                 \
    OP((RES), ((TYPE) REG), عملیہ, ##__VA_ARGS__);    \
    اگلا_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#تعریف_کرو d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کرو d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کرو d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#تعریف_کرو d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#تعریف_کرو M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#تعریف_کرو M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#تعریف_کرو M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#تعریف_کرو M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#تعریف_کرو d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#تعریف_کرو d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#تعریف_کرو d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#تعریف_کرو d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#تعریف_کرو d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#تعریف_کرو d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#تعریف_کرو d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#تعریف_کرو d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#تعریف_کرو d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#تعریف_کرو d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#تعریف_کرو d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#تعریف_کرو d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#تعریف_کرو d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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

#اگر d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#اگر_ختم

#تعریف_کرو OP_جوڑو_32(A,B) (i32)((u32)(A) + (u32)(B))
#تعریف_کرو OP_جوڑو_64(A,B) (i64)((u64)(A) + (u64)(B))
#تعریف_کرو OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#تعریف_کرو OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#تعریف_کرو OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#تعریف_کرو OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, جوڑو,      OP_جوڑو_32)  d_m3CommutativeOpFunc_i (i64, جوڑو,      OP_جوڑو_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, تفریق,            OP_SUB_32)  d_m3OpFunc_i (i64, تفریق,            OP_SUB_64)

#تعریف_کرو OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#تعریف_کرو OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#تعریف_کرو OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#تعریف_کرو OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, سرکاؤ_بایاں,       OP_SHL_32)      d_m3OpFunc_i (u64, سرکاؤ_بایاں,       OP_SHL_64)
d_m3OpFunc_i (i32, سرکاؤ_دایاں,      OP_SHR_32)      d_m3OpFunc_i (i64, سرکاؤ_دایاں,      OP_SHR_64)
d_m3OpFunc_i (u32, سرکاؤ_دایاں,      OP_SHR_32)      d_m3OpFunc_i (u64, سرکاؤ_دایاں,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#اگر d_m3_Fold_سیٹ_Local

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
    TYPE عملیہ = خانہ (TYPE);                         \
    TYPE * dest = خانہ_ptr (TYPE);                      \
    d_m3_Preload_اگلا ();                                 \
    TYPE نتیجہ;                                        \
    OP((نتیجہ), عملیہ, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = نتیجہ;  REG = نتیجہ;                     \
    اگلا_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = خانہ (TYPE);                        \
    TYPE operand1 = خانہ (TYPE);                        \
    TYPE * dest = خانہ_ptr (TYPE);                      \
    d_m3_Preload_اگلا ();                                 \
    TYPE نتیجہ;                                        \
    OP((نتیجہ), operand1, operand2, ##__VA_ARGS__);    \
    * dest = نتیجہ;  REG = نتیجہ;                     \
    اگلا_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE عملیہ = خانہ (TYPE);                         \
    TYPE * dest = خانہ_ptr (TYPE);                      \
    d_m3_Preload_اگلا ();                                 \
    TYPE نتیجہ;                                        \
    OP((نتیجہ), ((TYPE) REG), عملیہ, ##__VA_ARGS__); \
    * dest = نتیجہ;  REG = نتیجہ;                     \
    اگلا_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#تعریف_کرو d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کرو d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کرو d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#تعریف_کرو d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, جوڑو,          M3_FUNC, OP_جوڑو_32)
d_m3FoldOp_i           (i32, تفریق,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, سرکاؤ_بایاں,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, سرکاؤ_دایاں,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, سرکاؤ_دایاں,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, جوڑو,          M3_FUNC, OP_جوڑو_64)
d_m3FoldOp_i           (i64, تفریق,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, سرکاؤ_بایاں,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, سرکاؤ_دایاں,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, سرکاؤ_دایاں,   M3_FUNC, OP_SHR_64)

#  اگر d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, جوڑو,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, تفریق,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, جوڑو,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, تفریق,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#اگر_ختم

#اگر_ختم // d_m3FoldSetLocal

#اگر d_m3HasFloat
d_m3CommutativeArithOp_f(f32, جوڑو,         +)      d_m3CommutativeArithOp_f(f64, جوڑو,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, تفریق,               -)      d_m3ArithOp_f(f64, تفریق,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#اگر_ختم

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

#اگر d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#اگر_ختم

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#تعریف_کرو d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    اگلا_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE عملیہ = خانہ (TYPE);                     \
    OP((RES), عملیہ, ##__VA_ARGS__);              \
    اگلا_Op ();                                      \
}

#تعریف_کرو M3_UNARY(RES, X, OP) (RES) = OP(X)
#تعریف_کرو M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#تعریف_کرو d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#تعریف_کرو d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#تعریف_کرو d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#اگر d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       جذر);
d_m3UnaryArithOp_f(f32, قریب_ترین,    rintf);    d_m3UnaryArithOp_f(f64, قریب_ترین,    rint);
#  اگر defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  ورنہ
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#اگر_ختم

#تعریف_کرو OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_مقصد_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_مقصد_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#اگر (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#تعریف_کرو OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#تعریف_کرو OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#تعریف_کرو OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#تعریف_کرو OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#وگرنہ_اگر defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#تعریف_کرو OP_CLZ_32(x) __builtin_clz(x)
#تعریف_کرو OP_CTZ_32(x) __builtin_ctz(x)
#تعریف_کرو OP_CLZ_64(x) __builtin_clzll(x)
#تعریف_کرو OP_CTZ_64(x) __builtin_ctzll(x)
#ورنہ
#تعریف_کرو OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#تعریف_کرو OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#تعریف_کرو OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#تعریف_کرو OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#اگر_ختم

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#تعریف_کرو OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    اگلا_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 عملیہ = خانہ(i64);
    _r0 = OP_WRAP_I64(عملیہ);
    اگلا_Op();
}

// Integer sign extension operations
#تعریف_کرو OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#تعریف_کرو OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#تعریف_کرو OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#تعریف_کرو OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#تعریف_کرو OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#اگر d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_بلند = خانہ (u64);
    u64  rhs_کم  = خانہ (u64);
    u64  lhs_بلند = خانہ (u64);
    u64  lhs_کم  = خانہ (u64);
    u64* کم     = خانہ_ptr (u64);
    d_m3_Preload_اگلا();

    u64 مجموع = lhs_کم + rhs_کم;

    *کم = مجموع;
    _r0  = (i64) (lhs_بلند + rhs_بلند + (مجموع < lhs_کم));

    اگلا_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_بلند = خانہ (u64);
    u64  rhs_کم  = خانہ (u64);
    u64  lhs_بلند = خانہ (u64);
    u64  lhs_کم  = خانہ (u64);
    u64* کم     = خانہ_ptr (u64);
    d_m3_Preload_اگلا();

    *کم = lhs_کم - rhs_کم;
    _r0  = (i64) (lhs_بلند - rhs_بلند - (lhs_کم < rhs_کم));

    اگلا_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = خانہ (i64);
    i64  a   = خانہ (i64);
    u64* کم = خانہ_ptr (u64);
    d_m3_Preload_اگلا();

    u64 بلند;
    *کم = mul_wide_i64 (a, b, &بلند);
    _r0  = (i64) بلند;

    اگلا_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = خانہ (u64);
    u64  a   = خانہ (u64);
    u64* کم = خانہ_ptr (u64);
    d_m3_Preload_اگلا();

    u64 بلند;
    *کم = mul_wide_u64 (a, b, &بلند);
    _r0  = (i64) بلند;

    اگلا_Op_Preloaded();
}

#اگر_ختم // d_m3HasWideArithmetic

#تعریف_کرو d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    اگلا_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * ڈھیر = خانہ_ptr (FROM);                 \
    OP((DEST), (* ڈھیر), ##__VA_ARGS__);           \
    اگلا_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = خانہ_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    اگلا_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * ڈھیر = خانہ_ptr (FROM);                 \
    TYPE * dest = خانہ_ptr (TYPE);                  \
    OP((* dest), (* ڈھیر), ##__VA_ARGS__);         \
    اگلا_Op ();                                      \
}

#اگر d_m3HasFloat
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
#اگر_ختم

#تعریف_کرو d_m3TypeModifyOp(REG_مقصد, REG_FROM, مقصد_2, NAME, FROM)  \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_مقصد = (مقصد_2) ((FROM) REG_FROM);                        \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = خانہ (FROM);                                \
    REG_مقصد = (مقصد_2) (from);                                   \
    اگلا_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#تعریف_کرو d_m3FloatModifyOp(مقصد_2, NAME, FROM)                   \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((مقصد_2) ((FROM) _fp0));             \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = خانہ (FROM);                                \
    _fp0 = canon_nan_##TO ((مقصد_2) (from));                    \
    اگلا_Op ();                                              \
}

#اگر d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#اگر_ختم

#تعریف_کرو d_m3TypeConvertOp(REG_مقصد, REG_FROM, مقصد_2, NAME, FROM) \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_مقصد = (مقصد_2) ((FROM) REG_FROM);                        \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    خانہ (مقصد_2) = (مقصد_2) ((FROM) REG_FROM);                     \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = خانہ (FROM);                                \
    REG_مقصد = (مقصد_2) (from);                                   \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = خانہ (FROM);                                \
    خانہ (مقصد_2) = (مقصد_2) (from);                                \
    اگلا_Op ();                                              \
}

// Int to float
#اگر d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#اگر_ختم

#تعریف_کرو d_m3ReinterpretOp(REG, مقصد_2, SRC, FROM)               \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = خانہ (FROM);                                      \
    REG = u.t;                                              \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    خانہ (مقصد_2) = u.t;                                        \
    اگلا_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(مقصد_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    اتحاد { FROM c; مقصد_2 t; } u;                              \
    u.c = خانہ (FROM);                                      \
    خانہ (مقصد_2) = u.t;                                        \
    اگلا_Op ();                                              \
}

#اگر d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#اگر_ختم

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = فوری(u32*);
    خانہ(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    اگلا_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = فوری(u64*);
    خانہ(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    اگلا_Op();
}


d_m3Op(سیٹ_Global_i32)
{
    u32* global = فوری(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    اگلا_Op();
}


d_m3Op(سیٹ_Global_i64)
{
    u64* global = فوری(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    اگلا_Op();
}


#اگر d_m3_Has_ڈھیر_Switching

d_m3Op(Branch);
d_m3Op(اندراج_3);
#  اگر !d_m3_اندراج_Keeps_چوکھٹا
d_m3Op(اندراج_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
ساکن M3_NOINLINE
IM3Function Function_کا_Compiled_رمز (pc_t i_pc)
{
    اگر (not i_pc) {
        واپس تہی;
    }

    جب ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation اندراج_2 = (IM3Operation)i_pc[0];

#  اگر !d_m3_اندراج_Keeps_چوکھٹا
    اگر (اندراج_2 == op_اندراج_Suspendable) {
        واپس (IM3Function)i_pc[1];
    }
#  endif
    واپس (اندراج_2 == op_اندراج) ? (IM3Function)i_pc[1] : تہی;
}

#اگر_ختم

d_m3Op(Call)
{
    d_m3_Check_Native_ڈھیر();

    pc_t      callPC = فوری(pc_t);
    i32       ڈھیر_انحراف = فوری(i32);
    IM3_حافظہ حافظہ = m3MemInfo(_mem);

    m3stack_t sp = _sp + ڈھیر_انحراف;

#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_دلائل, d_m3_بنیاد_Cstr);
#ورنہ
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_دلائل);
#اگر_ختم

    _mem = حافظہ->mallocated;

    اگر (M3_LIKELY(not r)) {
        اگلا_Op();
    } ورنہ {
        d_m3_ریکارڈ_Call_چوکھٹا(حافظہ, Function_کا_Compiled_رمز(callPC));
        ڈالو_Backtrace_چوکھٹا_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = خانہ(IM3Function);
    IM3FuncType type = فوری(IM3FuncType);
    i32         ڈھیر_انحراف = فوری(i32);
    IM3_حافظہ   حافظہ = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ڈھیر_انحراف;

    m3ret_t     r = m3Err_none;

    اگر (M3_LIKELY(function)) {
        اگر (M3_LIKELY(type == function->funcType)) {
            اگر (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            اگر (M3_LIKELY(not r)) {
#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_دلائل, d_m3_بنیاد_Cstr);
#ورنہ
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_دلائل);
#اگر_ختم

                _mem = حافظہ->mallocated;

                اگر (M3_LIKELY(not r)) {
                    اگلا_Op_براہ_راست();
                } ورنہ {
                    d_m3_ریکارڈ_Call_چوکھٹا(حافظہ, function);
                    ڈالو_Backtrace_چوکھٹا_2();
                    forwardTrap(r);
                }
            }
        } ورنہ {
            r = m3_Err_trap_بالواسطہ_Call_Type_Mismatch;
        }
    } ورنہ {
        r = m3Err_trapNullFunctionRef;
    }

    اگر (M3_UNLIKELY(r)) {
        newTrap(r);
    } ورنہ {
        forwardTrap(r);
    }
}


// ref.as_non_null is a null check and nothing else: the value stays put, so
// there is no destination slot, only the trap.
d_m3Op(RefAsNonNull)
{
    IM3Function reference = خانہ(IM3Function);

    اگر (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    اگلا_Op();
}


d_m3Op(Call_بالواسطہ)
{
    d_m3_Check_Native_ڈھیر();

    M3_جدول*    جدول_2 = فوری(M3_جدول*);
    u64         جدول_اشاریہ = d_m3_Wide_عملیہ(جدول_2->isTable64);
    IM3FuncType type = فوری(IM3FuncType);
    i32         ڈھیر_انحراف = فوری(i32);
    IM3_حافظہ   حافظہ = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ڈھیر_انحراف;

    m3ret_t     r = m3Err_none;

    اگر (M3_LIKELY(جدول_اشاریہ < جدول_2->سائز)) {
        IM3Function function = (IM3Function)جدول_2->elements[جدول_اشاریہ];

        اگر (M3_LIKELY(function)) {
            اگر (M3_LIKELY(type == function->funcType)) {
                اگر (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                اگر (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3_حافظہ_سرخی* calleeMem = Module_حافظہ_سرخی(function->module);

#اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_دلائل, d_m3_بنیاد_Cstr);
#ورنہ
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_دلائل);
#اگر_ختم

                    _mem = حافظہ->mallocated;

                    اگر (M3_LIKELY(not r)) {
                        اگلا_Op_براہ_راست();
                    } ورنہ {
                        d_m3_ریکارڈ_Call_چوکھٹا(حافظہ, function);
                        ڈالو_Backtrace_چوکھٹا_2();
                        forwardTrap(r);
                    }
                }
            } ورنہ {
                r = m3_Err_trap_بالواسطہ_Call_Type_Mismatch;
            }
        } ورنہ {
            r = m3_Err_trap_جدول_Element_Is_Null;
        }
    } ورنہ {
        r = m3_Err_trap_جدول_اشاریہ_برآمد_کا_Range;
    }

    اگر (M3_UNLIKELY(r)) {
        newTrap(r);
    } ورنہ {
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
#تعریف_کرو d_m3_دم_Call_دلائل(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((خالی *) (_sp + (RETURNSLOTS)), (ثابت خالی *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * حجم (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = فوری(pc_t);
    i32     ڈھیر_انحراف = فوری(i32);
    i32     returnSlots = فوری(i32);
    u32     عدد_Arg_Slots = فوری(u32);

    m3ret_t possible_trap = m3_Yield();
    اگر (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_دم_Call_دلائل(returnSlots, ڈھیر_انحراف, عدد_Arg_Slots);

    jump_Op_براہ_راست(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = خانہ(IM3Function);
    IM3FuncType type = فوری(IM3FuncType);
    i32         ڈھیر_انحراف = فوری(i32);
    i32         returnSlots = فوری(i32);
    u32         عدد_Arg_Slots = فوری(u32);

    m3ret_t     r = m3Err_none;

    اگر (M3_LIKELY(function)) {
        اگر (M3_LIKELY(type == function->funcType)) {
            اگر (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            اگر (M3_LIKELY(not r)) {
                r = m3_Yield();

                اگر (M3_LIKELY(not r)) {
                    d_m3_دم_Call_دلائل(returnSlots, ڈھیر_انحراف, عدد_Arg_Slots);

                    jump_Op_براہ_راست(function->compiled);
                }
            }
        } ورنہ {
            r = m3_Err_trap_بالواسطہ_Call_Type_Mismatch;
        }
    } ورنہ {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(Return_Call_بالواسطہ)
{
    M3_جدول*    جدول_2 = فوری(M3_جدول*);
    u64         جدول_اشاریہ = d_m3_Wide_عملیہ(جدول_2->isTable64);
    IM3FuncType type = فوری(IM3FuncType);
    i32         ڈھیر_انحراف = فوری(i32);
    i32         returnSlots = فوری(i32);
    u32         عدد_Arg_Slots = فوری(u32);

    m3ret_t     r = m3Err_none;

    اگر (M3_LIKELY(جدول_اشاریہ < جدول_2->سائز)) {
        IM3Function function = (IM3Function)جدول_2->elements[جدول_اشاریہ];

        اگر (M3_LIKELY(function)) {
            اگر (M3_LIKELY(type == function->funcType)) {
                اگر (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                اگر (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    اگر (M3_LIKELY(not r)) {
                        d_m3_دم_Call_دلائل(returnSlots, ڈھیر_انحراف, عدد_Arg_Slots);

                        jump_Op_براہ_راست(function->compiled);
                    }
                }
            } ورنہ {
                r = m3_Err_trap_بالواسطہ_Call_Type_Mismatch;
            }
        } ورنہ {
            r = m3_Err_trap_جدول_Element_Is_Null;
        }
    } ورنہ {
        r = m3_Err_trap_جدول_اشاریہ_برآمد_کا_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = فوری(IM3Function);
    ctx.userdata = فوری(خالی*);
    u64* ثابت sp = ((u64*)_sp);
    IM3_حافظہ  حافظہ = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#اگر d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    حرف        outbuff[1024];
    حرف*       outp = outbuff;
    حرف*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.میدان_Utf8);

    ثابت صحیح n_دلائل = ftype->عدد_دلائل;
    ثابت صحیح nRets = ftype->عدد_Rets;
    u64*      دلائل = sp + nRets;
    برائے (صحیح i = 0; i < n_دلائل; i++) {
        ثابت صحیح type = ftype->types[nRets + i];
        انتخاب (type) {
        حالت c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(دلائل + i)); توڑو;
        حالت c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(دلائل + i)); توڑو;
        حالت c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(دلائل + i)); توڑو;
        حالت c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(دلائل + i)); توڑو;
        طےشدہ: outp += snprintf(outp, oute - outp, "<type %d>", type); توڑو;
        }
        اگر (i < n_دلائل - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  اگر d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#اگر_ختم

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    خالی* ڈھیر_backup = runtime->ڈھیر;
    runtime->ڈھیر = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_ڈیٹا(ctx.function->host_حافظہ->mallocated));
    runtime->ڈھیر = ڈھیر_backup;

#اگر d_m3EnableStrace
    اگر (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (حرف*)possible_trap);
    } ورنہ اگر (ftype and ftype->عدد_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } ورنہ {
        d_m3TracePrint("%s", outbuff);
    }
#اگر_ختم

    اگر (M3_UNLIKELY(possible_trap)) {
        _mem = حافظہ->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        اگر (possible_trap != m3Err_pendingException) {
            ڈالو_Backtrace_چوکھٹا_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_سائز)
{
    IM3_حافظہ حافظہ = m3MemInfo(_mem);

    _r0 = حافظہ->عدد_صفحات;

    اگلا_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_حافظہ  حافظہ = m3MemInfo(_mem);

    i32        عدد_صفحات_مقصد_Grow = (i32)_r0;
    اگر (عدد_صفحات_مقصد_Grow >= 0) {
        _r0 = (m3reg_t)حافظہ->عدد_صفحات;

        اگر (M3_LIKELY(عدد_صفحات_مقصد_Grow)) {
            u64      required_صفحات = حافظہ->عدد_صفحات + (u32)عدد_صفحات_مقصد_Grow;

            M3_نتیجہ r = Resize_حافظہ(runtime, حافظہ, required_صفحات);
            اگر (r) {
                _r0 = -1;
            }

            _mem = حافظہ->mallocated;
        }
    } ورنہ {
        _r0 = -1;
    }

    اگلا_Op();
}


#اگر d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_حافظہ  حافظہ = m3MemInfo(_mem);

    u64        عدد_صفحات_مقصد_Grow = (u64)_r0;
    u64        عدد_صفحات = حافظہ->عدد_صفحات;

    اگر (M3_LIKELY(عدد_صفحات_مقصد_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        اگر (عدد_صفحات_مقصد_Grow <= حافظہ->max_صفحات - عدد_صفحات and
            not Resize_حافظہ(runtime, حافظہ, عدد_صفحات + عدد_صفحات_مقصد_Grow)) {
            _r0 = (m3reg_t)عدد_صفحات;
        } ورنہ {
            _r0 = -1;
        }

        _mem = حافظہ->mallocated;
    } ورنہ {
        _r0 = (m3reg_t)عدد_صفحات;
    }

    اگلا_Op();
}

#اگر_ختم // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 سائز = (u32)_r0;
    u64 ماخذ = خانہ(u32);
    u64 destination = خانہ(u32);

    اگر (M3_LIKELY(d_m3MemRangeOk(destination, سائز, _mem))) {
        اگر (M3_LIKELY(d_m3MemRangeOk(ماخذ, سائز, _mem))) {
            u8* dst = m3_Mem_ڈیٹا(_mem) + destination;
            u8* src = m3_Mem_ڈیٹا(_mem) + ماخذ;
            memmove(dst, src, (size_t)سائز);

            اگلا_Op();
        } ورنہ {
            d_برآمد_کا_Bounds_Mem_Op(ماخذ, سائز);
        }
    } ورنہ {
        d_برآمد_کا_Bounds_Mem_Op(destination, سائز);
    }
}


#اگر d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 سائز = (u64)_r0;
    u64 ماخذ = خانہ(u64);
    u64 destination = خانہ(u64);

    اگر (M3_LIKELY(d_m3MemRangeOk(destination, سائز, _mem))) {
        اگر (M3_LIKELY(d_m3MemRangeOk(ماخذ, سائز, _mem))) {
            u8* dst = m3_Mem_ڈیٹا(_mem) + destination;
            u8* src = m3_Mem_ڈیٹا(_mem) + ماخذ;
            memmove(dst, src, (size_t)سائز);

            اگلا_Op();
        } ورنہ {
            d_برآمد_کا_Bounds_Mem_Op(ماخذ, سائز);
        }
    } ورنہ {
        d_برآمد_کا_Bounds_Mem_Op(destination, سائز);
    }
}

#اگر_ختم // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(سیٹ_حافظہ)
{
    IM3_حافظہ حافظہ = فوری(IM3_حافظہ);

    _mem = حافظہ->mallocated;

    اگلا_Op();
}


#اگر d_m3_Has_Multi_حافظہ

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3_حافظہ       dest_حافظہ = فوری(IM3_حافظہ);
    IM3_حافظہ       ماخذ_حافظہ = فوری(IM3_حافظہ);
    u32             widths = فوری(u32);

    M3_حافظہ_سرخی* destMem = dest_حافظہ->mallocated;
    M3_حافظہ_سرخی* ماخذ_Mem = ماخذ_حافظہ->mallocated;

    u64             سائز = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             ماخذ = d_m3_Wide_عملیہ(widths & 0x2);
    u64             destination = d_m3_Wide_عملیہ(widths & 0x1);

    اگر (M3_LIKELY(d_m3MemRangeOk(destination, سائز, destMem))) {
        اگر (M3_LIKELY(d_m3MemRangeOk(ماخذ, سائز, ماخذ_Mem))) {
            memmove(m3_Mem_ڈیٹا(destMem) + destination, m3_Mem_ڈیٹا(ماخذ_Mem) + ماخذ, (size_t)سائز);

            اگلا_Op();
        } ورنہ {
            d_برآمد_کا_Bounds_Mem_Op(ماخذ, سائز);
        }
    } ورنہ {
        d_برآمد_کا_Bounds_Mem_Op(destination, سائز);
    }
}

#اگر_ختم // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 سائز = (u32)_r0;
    u32 byte = خانہ(u32);
    u64 destination = خانہ(u32);

    اگر (M3_LIKELY(d_m3MemRangeOk(destination, سائز, _mem))) {
        u8* mem8 = m3_Mem_ڈیٹا(_mem) + destination;
        memset(mem8, (u8)byte, سائز);
        اگلا_Op();
    } ورنہ {
        d_برآمد_کا_Bounds_Mem_Op(destination, سائز);
    }
}


#اگر d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 سائز = (u64)_r0;
    u32 byte = خانہ(u32);
    u64 destination = خانہ(u64);

    اگر (M3_LIKELY(d_m3MemRangeOk(destination, سائز, _mem))) {
        u8* mem8 = m3_Mem_ڈیٹا(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)سائز);
        اگلا_Op();
    } ورنہ {
        d_برآمد_کا_Bounds_Mem_Op(destination, سائز);
    }
}

#اگر_ختم // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_ڈیٹا_Segment* segment = فوری(M3_ڈیٹا_Segment*);

    u64            سائز = (u32)_r0;
    u64            ماخذ = خانہ(u32);
    u64            destination = خانہ(u32);

    u64            دستیاب = segment->dropped ? 0 : segment->سائز;

    اگر (M3_LIKELY(d_m3MemRangeOk(destination, سائز, _mem))) {
        اگر (M3_LIKELY(ماخذ <= دستیاب and سائز <= دستیاب - ماخذ)) {
            حافظہ_نقل(m3_Mem_ڈیٹا(_mem) + destination, segment->ڈیٹا + ماخذ, (size_t)سائز);
            اگلا_Op();
        } ورنہ {
            d_برآمد_کا_Bounds_Mem_Op(ماخذ, سائز);
        }
    } ورنہ {
        d_برآمد_کا_Bounds_Mem_Op(destination, سائز);
    }
}


#اگر d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_ڈیٹا_Segment* segment = فوری(M3_ڈیٹا_Segment*);

    u64            سائز = (u32)_r0;
    u64            ماخذ = خانہ(u32);
    u64            destination = خانہ(u64);

    u64            دستیاب = segment->dropped ? 0 : segment->سائز;

    اگر (M3_LIKELY(d_m3MemRangeOk(destination, سائز, _mem))) {
        اگر (M3_LIKELY(ماخذ <= دستیاب and سائز <= دستیاب - ماخذ)) {
            حافظہ_نقل(m3_Mem_ڈیٹا(_mem) + destination, segment->ڈیٹا + ماخذ, (size_t)سائز);
            اگلا_Op();
        } ورنہ {
            d_برآمد_کا_Bounds_Mem_Op(ماخذ, سائز);
        }
    } ورنہ {
        d_برآمد_کا_Bounds_Mem_Op(destination, سائز);
    }
}

#اگر_ختم // d_m3HasMemory64


d_m3Op(ڈیٹا_Drop)
{
    M3_ڈیٹا_Segment* segment = فوری(M3_ڈیٹا_Segment*);
    segment->dropped = true;

    اگلا_Op();
}


#اگر d_m3HasRefTypes

d_m3Op(جدول_Get)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    u64      index = d_m3_Wide_عملیہ(جدول_2->isTable64);

    اگر (M3_LIKELY(index < جدول_2->سائز)) {
        _r0 = (u64)(uintptr_t)جدول_2->elements[index];
        اگلا_Op();
    } ورنہ {
        newTrap(m3_Err_trap_جدول_برآمد_کا_Bounds);
    }
}


d_m3Op(جدول_سیٹ)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    خالی*    قدر = خانہ(خالی*);
    u64      index = d_m3_Wide_عملیہ(جدول_2->isTable64);

    اگر (M3_LIKELY(index < جدول_2->سائز)) {
        جدول_2->elements[index] = قدر;
        اگلا_Op();
    } ورنہ {
        newTrap(m3_Err_trap_جدول_برآمد_کا_Bounds);
    }
}


d_m3Op(جدول_سائز)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);

    _r0 = جدول_2->سائز;

    اگلا_Op();
}


d_m3Op(جدول_Grow)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    u64      delta = d_m3_Wide_عملیہ(جدول_2->isTable64);
    خالی*    قدر = خانہ(خالی*);

    u32      old_سائز = جدول_2->سائز;
    u32      max_سائز = جدول_2->max_سائز ? جدول_2->max_سائز : d_m3_Max_Sane_جدول_سائز;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_سائز = (delta <= max_سائز - old_سائز) ? (u64)old_سائز + delta
                                                    : (u64)max_سائز + 1;

    اگر (new_سائز == old_سائز)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_سائز;
        اگلا_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    اگر (new_سائز <= max_سائز and (not runtime->جدول_Elements_حد_بندی or
                                delta <= runtime->جدول_Elements_حد_بندی - runtime->جدول_Elements_مستعمل)) {
        خالی** elements = m3_ReallocArray(خالی*, جدول_2->elements, (size_t)new_سائز, old_سائز);

        اگر (elements) {
            runtime->جدول_Elements_مستعمل += new_سائز - old_سائز;
            جدول_2->elements = elements;
            جدول_2->سائز = (u32)new_سائز;

            برائے (u32 i = old_سائز; i < جدول_2->سائز; ++i) {
                جدول_2->elements[i] = قدر;
            }

            _r0 = old_سائز;
            اگلا_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    اگلا_Op();
}


d_m3Op(جدول_Init)
{
    M3_جدول*          جدول_2 = فوری(M3_جدول*);
    M3ElementSegment* segment = فوری(M3ElementSegment*);
    u32               گنتی = خانہ(u32);
    u32               ماخذ = خانہ(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_عملیہ(جدول_2->isTable64);

    u64               دستیاب = segment->dropped ? 0 : segment->عدد_Elements;

    اگر (M3_LIKELY(destination <= جدول_2->سائز and گنتی <= جدول_2->سائز - destination and
                  (u64) ماخذ + گنتی <= دستیاب)) {
        برائے (u32 i = 0; i < گنتی; ++i) {
            جدول_2->elements[destination + i] = segment->resolved[ماخذ + i];
        }

        اگلا_Op();
    } ورنہ {
        newTrap(m3_Err_trap_جدول_برآمد_کا_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = فوری(M3ElementSegment*);
    segment->dropped = true;

    اگلا_Op();
}


d_m3Op(جدول_Copy)
{
    M3_جدول* dst = فوری(M3_جدول*);
    M3_جدول* src = فوری(M3_جدول*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      گنتی = d_m3_Wide_عملیہ(dst->isTable64 and src->isTable64);
    u64      ماخذ = d_m3_Wide_عملیہ(src->isTable64);
    u64      destination = d_m3_Wide_عملیہ(dst->isTable64);

    اگر (M3_LIKELY(destination <= dst->سائز and گنتی <= dst->سائز - destination and
                  ماخذ <= src->سائز and گنتی <= src->سائز - ماخذ)) {
        memmove(dst->elements + destination, src->elements + ماخذ, (size_t)گنتی * حجم(خالی*));

        اگلا_Op();
    } ورنہ {
        newTrap(m3_Err_trap_جدول_برآمد_کا_Bounds);
    }
}


d_m3Op(جدول_Fill)
{
    M3_جدول* جدول_2 = فوری(M3_جدول*);
    u64      گنتی = d_m3_Wide_عملیہ(جدول_2->isTable64);
    خالی*    قدر = خانہ(خالی*);
    u64      index = d_m3_Wide_عملیہ(جدول_2->isTable64);

    اگر (M3_LIKELY(index <= جدول_2->سائز and گنتی <= جدول_2->سائز - index)) {
        برائے (u64 i = 0; i < گنتی; ++i) {
            جدول_2->elements[index + i] = قدر;
        }

        اگلا_Op();
    } ورنہ {
        newTrap(m3_Err_trap_جدول_برآمد_کا_Bounds);
    }
}

#اگر_ختم // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = فوری(IM3Function);

    m3ret_t     نتیجہ = m3Err_none;

    اگر (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        نتیجہ = CompileFunction(function);
    }

    اگر (not نتیجہ) {
        // patch up compiled pc and call rewritten op_Call
        *((خالی**)--_pc) = (خالی*)(function->compiled);
        --_pc;
        اگلا_Op_براہ_راست();
    }

    newTrap(نتیجہ);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = فوری(IM3Function);

    m3ret_t     نتیجہ = m3Err_none;

    اگر (M3_UNLIKELY(not function->compiled)) {
        نتیجہ = CompileFunction(function);
    }

    اگر (not نتیجہ) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((خالی**)--_pc) = (خالی*)(function->compiled);
        --_pc;
        اگلا_Op_براہ_راست();
    }

    newTrap(نتیجہ);
}


#اگر d_m3_Has_ڈھیر_Switching
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
ساکن
خالی Suspend_Without_لیبل (IM3Runtime io_runtime, IM3Continuation io_cont, M3_محفوظ_نقطہ_قسم i_قسم,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  اگر d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_لیبل = تہی;
    io_runtime->suspendHandlerCont = تہی;
    io_runtime->suspendStubPC = تہی;
    io_runtime->switch_ہدف = تہی;
    io_runtime->عدد_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspend_نقطہ = (u8)i_قسم;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  اگر d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->عدد_Suspend_Results = 0;
}
#اگر_ختم // d_m3HasStackSwitching


#اگر d_m3HasGasMetering

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
    u32        لاگت = فوری(u32);

    runtime->gas_بقایا -= (i64)لاگت;

    اگر (M3_UNLIKELY(runtime->gas_بقایا < 0)) {
#  اگر d_m3_Has_ڈھیر_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        اگر (runtime->isSuspendable and runtime->فعال_Continuation) {
            runtime->suspendRequested = true;
            اگلا_Op();
        }
#  endif
        newTrap(m3_Err_trap_برآمد_کا_Gas);
    }

    اگلا_Op();
}

#اگر_ختم // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
ساکن اندرونی
bool اندراج_سماتا (IM3Function i_function, m3stack_t _sp, M3_حافظہ_سرخی* _mem)
{
#اگر d_m3_Skip_ڈھیر_Check
    (خالی)i_function;
    (خالی)_sp;
    (خالی)_mem;
    واپس true;
#وگرنہ_اگر d_m3_Has_ڈھیر_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->فعال_Continuation : تہی);
    خالی*           max_ڈھیر = (_act && _act->val_ڈھیر)
                                 ? (خالی*)(_act->val_ڈھیر + _act->عدد_ڈھیر_Slots)
                                 : (_mem ? _mem->max_ڈھیر : (خالی*)~(uintptr_t)0);
    واپس M3_LIKELY((خالی*)(_sp + i_function->max_ڈھیر_Slots) < max_ڈھیر);
#ورنہ
    واپس M3_LIKELY((خالی*)(_sp + i_function->max_ڈھیر_Slots) < _mem->max_ڈھیر);
#اگر_ختم
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
ساکن اندرونی
خالی Enter_چوکھٹا (IM3Function i_function, m3stack_t _sp)
{
#اگر defined(DEBUG)
    i_function->کامیابیاں++;
#اگر_ختم
    u8* ڈھیر = (u8*)((m3slot_t*)_sp + i_function->عدد_Ret_And_Arg_Slots);

    memset(ڈھیر, 0x0, i_function->عدد_Local_بائٹیں);
    ڈھیر += i_function->عدد_Local_بائٹیں;

    اگر (i_function->constants) {
        حافظہ_نقل(ڈھیر, i_function->constants, i_function->عدد_Constant_بائٹیں);
    }
}


d_m3Op(اندراج_3)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = فوری(IM3Function);
#اگر d_m3_اندراج_Keeps_چوکھٹا
    IM3_حافظہ حافظہ = m3MemInfo(_mem);
#اگر_ختم

    اگر (اندراج_سماتا(function, _sp, _mem)) {
        Enter_چوکھٹا(function, _sp);

#اگر !d_m3_اندراج_Keeps_چوکھٹا
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        اگلا_Op_براہ_راست();
#ورنہ

#  اگر d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->عدد_Ret_Slots));
        trace_rt->call_گہرائی++;
#  endif

        m3ret_t r = اگلا_Op_Impl();

#  اگر d_m3EnableStrace >= 2
        trace_rt->call_گہرائی--;

        اگر (r) {
            d_m3TracePrint("} !trap = %s", (حرف*)r);
        } ورنہ اگر (Get_Function_عدد_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } ورنہ {
            d_m3TracePrint("}");
        }
#  endif

        اگر (M3_UNLIKELY(r)) {
            _mem = حافظہ->mallocated;
#  اگر d_m3_Has_ڈھیر_Switching
            اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                واپس ریکارڈ_اندراج_چوکھٹا(m3MemRuntime(_mem), _pc, _sp, حافظہ, function);
            }
#  endif
            fill_Backtrace_چوکھٹا();
        }
        forwardTrap(r);
#اگر_ختم // d_m3EntryKeepsFrame
    } ورنہ {
        newTrap(m3_Err_trap_ڈھیر_حد_سے_تجاوز);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_ڈھیر();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_حافظہ حافظہ = m3MemInfo(_mem);

    کرو {
#اگر d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_گہرائی++;
#اگر_ختم
        r = اگلا_Op_Impl();

#اگر d_m3EnableStrace >= 3
        trace_rt->call_گہرائی--;
        d_m3TracePrint("}");
#اگر_ختم
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = حافظہ->mallocated;
    } جب (r == _pc);

#اگر d_m3_Has_ڈھیر_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        واپس ریکارڈ_Loop_چوکھٹا(m3MemRuntime(_mem), _pc, _sp, حافظہ);
    }
#اگر_ختم

    forwardTrap(r);
}


#اگر d_m3HasExceptionHandling

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
    d_m3_Check_Native_ڈھیر();

    IM3_حافظہ  حافظہ = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        عدد_Clauses = فوری(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * عدد_Clauses;

    u32        گہرائی = ++runtime->try_گہرائی;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = حافظہ->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_گہرائی == گہرائی);
    runtime->try_گہرائی = گہرائی - 1;

#  اگر d_m3_Has_ڈھیر_Switching
    اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        واپس ریکارڈ_Try_چوکھٹا(runtime, clauses, _sp, حافظہ, عدد_Clauses, isInnermost);
    }
#  endif

    اگر (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ثابت M3Exception* exception = runtime->pendingException;

        برائے (u32 i = 0; i < عدد_Clauses; ++i) {
            IM3_لیبل لیبل = (IM3_لیبل)clauses[2 * i];

            اگر (لیبل == تہی or لیبل == exception->لیبل) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jump_Op_براہ_راست(handler);
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

    u32          عدد_دلائل = فوری(u32);

    برائے (u32 i = 0; i < عدد_دلائل; ++i) {
        i32 offset = فوری(i32);
        u32 is64 = فوری(u32);
        u64 قدر = exception->دلائل[i];

        اگر (is64) {
            *(u64*)(_sp + offset) = قدر;
        } ورنہ {
            *(u32*)(_sp + offset) = (u32)قدر;
        }
    }

    i32 ref_انحراف = فوری(i32);

    اگر (ref_انحراف >= 0) {
        *(M3Exception**)(_sp + ref_انحراف) = exception;
        exception->reified = true;
    } ورنہ اگر (not exception->reified) {
        FreeException(runtime, exception);
    }

    اگلا_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_لیبل       لیبل = فوری(IM3_لیبل);
    u32          عدد_دلائل = فوری(u32);

    M3Exception* exception = NewException(runtime, لیبل, عدد_دلائل);

    اگر (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    برائے (u32 i = 0; i < عدد_دلائل; ++i) {
        i32 offset = فوری(i32);
        u32 is64 = فوری(u32);

        exception->دلائل[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    واپس m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = خانہ(M3Exception*);

    اگر (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    واپس m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(نکالو_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        گنتی = فوری(u32);

    runtime->try_گہرائی -= گنتی;

    اگلا_Op();
}

#اگر_ختم // d_m3HasExceptionHandling

#اگر d_m3_Has_ڈھیر_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = فوری(IM3FuncType);
    i32         dst_خانہ = فوری(i32);
    i32         func_خانہ = فوری(i32);

    IM3Function func = *(IM3Function*)(_sp + func_خانہ);
    اگر (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    اگر (M3_UNLIKELY(not Continuation_Can_Acquire_ڈھیر(runtime))) {
        newTrap(m3_Err_continuation_حد_بندی_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    اگر (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_نتیجہ تخصیص = Continuation_Acquire_ڈھیر(runtime, cont);
    اگر (M3_UNLIKELY(تخصیص)) {
        cont->state = cont_consumed;
        newTrap(تخصیص);
    }
    *(IM3Continuation*)(_sp + dst_خانہ) = cont;
    اگلا_Op();
}


ساکن
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = فوری(IM3FuncType);
    IM3FuncType     ct2 = فوری(IM3FuncType);
    i32             dst_خانہ = فوری(i32);
    i32             cont_خانہ = فوری(i32);
    u32             عدد_Bound = فوری(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + cont_خانہ);
    اگر (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    اگر (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->اندراج_Function);
    اگر (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->اندراج_Function = srcCont->اندراج_Function;
    dstCont->val_ڈھیر = srcCont->val_ڈھیر;
    dstCont->عدد_ڈھیر_Slots = srcCont->عدد_ڈھیر_Slots;
    dstCont->چوکھٹے = srcCont->چوکھٹے;
    dstCont->چوکھٹے_Cap = srcCont->چوکھٹے_Cap;
    dstCont->عدد_چوکھٹے = srcCont->عدد_چوکھٹے;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspend_نقطہ = srcCont->suspend_نقطہ;
    dstCont->r0 = srcCont->r0;
#  اگر d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_دلائل_گنتی = srcCont->bound_دلائل_گنتی;
    dstCont->state = srcCont->state;
#  اگر d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = تہی;
#  endif

    dstCont->عدد_Suspend_Results = srcCont->عدد_Suspend_Results;
    برائے (u32 j = 0; j < srcCont->عدد_Suspend_Results; ++j) {
        dstCont->suspend_نتیجہ_Offsets[j] = srcCont->suspend_نتیجہ_Offsets[j];
        dstCont->suspend_نتیجہ_Is64[j] = srcCont->suspend_نتیجہ_Is64[j];
    }

    اگر (dstCont->state == cont_allocated) {
        u16 عدد_Rets = ct1->contFuncType ? Get_Func_Type_عدد_Results(ct1->contFuncType) : 0;
        برائے (u32 i = 0; i < عدد_Bound; ++i) {
            i32 arg_خانہ = فوری(i32);
            u32 is64 = فوری(u32);
            u32 ہدف_Idx = (عدد_Rets + dstCont->bound_دلائل_گنتی) * c_io_خانہ_گنتی;
            اگر (is64) {
                *(u64*)(dstCont->val_ڈھیر + ہدف_Idx) = *(u64*)(_sp + arg_خانہ);
            } ورنہ {
                *(u32*)(dstCont->val_ڈھیر + ہدف_Idx) = *(u32*)(_sp + arg_خانہ);
            }
            dstCont->bound_دلائل_گنتی++;
        }
    } ورنہ {
        IM3Continuation inner = InnermostSuspended(dstCont);
        برائے (u32 i = 0; i < عدد_Bound; ++i) {
            i32 arg_خانہ = فوری(i32);
            u32 is64 = فوری(u32);
            u32 dstIdx = dstCont->bound_دلائل_گنتی;
            اگر (dstIdx < inner->عدد_Suspend_Results) {
                i32 dst_انحراف = inner->suspend_نتیجہ_Offsets[dstIdx];
                اگر (is64) {
                    *(u64*)(inner->sp + dst_انحراف) = *(u64*)(_sp + arg_خانہ);
                } ورنہ {
                    *(u32*)(inner->sp + dst_انحراف) = *(u32*)(_sp + arg_خانہ);
                }
            }
            dstCont->bound_دلائل_گنتی++;
        }
    }

    srcCont->val_ڈھیر = تہی;
    srcCont->چوکھٹے = تہی;
    srcCont->چوکھٹے_Cap = 0;
    srcCont->عدد_چوکھٹے = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dst_خانہ) = dstCont;
    اگلا_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
ساکن
IM3Continuation ڈھونڈو_Handler (IM3Continuation i_cont, IM3_لیبل i_لیبل, u8 i_قسم, pc_t* o_stubPC)
{
    IM3_لیبل query_لیبل = (i_لیبل and i_لیبل->resolved) ? i_لیبل->resolved : i_لیبل;

    برائے (IM3Continuation cont = i_cont; cont; cont = cont->والد) {
        برائے (u32 h = 0; h < cont->عدد_Handlers; ++h) {
            pc_t   اندراج_2 = cont->handlersPC + (h * 3);
            IM3_لیبل handler_لیبل = (IM3_لیبل)(*(اندراج_2 + 1));
            اگر (handler_لیبل and handler_لیبل->resolved) {
                handler_لیبل = handler_لیبل->resolved;
            }

            اگر ((u32)(uintptr_t)(*اندراج_2) == i_قسم and handler_لیبل == query_لیبل) {
                اگر (o_stubPC) {
                    *o_stubPC = (pc_t)(*(اندراج_2 + 2));
                }
                واپس cont;
            }
        }
    }

    واپس تہی;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
ساکن
خالی ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    برائے (u32 i = 0; i < io_cont->عدد_Suspend_Results; ++i) {
        اگر (io_cont->suspend_نتیجہ_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_نتیجہ_Offsets[i]) = 0;
        } ورنہ {
            *(m3slot_t*)(i_sp + io_cont->suspend_نتیجہ_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->فعال_Continuation;
    اگر (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_لیبل);
    }

    IM3_لیبل لیبل = فوری(IM3_لیبل);
    u32    عدد_Params = فوری(u32);

    runtime->عدد_Suspend_Payload = عدد_Params;
    برائے (u32 i = 0; i < عدد_Params; ++i) {
        i32 param_خانہ = فوری(i32);
        u32 is64 = فوری(u32);
        اگر (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + param_خانہ);
        } ورنہ {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + param_خانہ);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 عدد_Results = فوری(u32);
    cont->عدد_Suspend_Results = عدد_Results;
    برائے (u32 i = 0; i < عدد_Results; ++i) {
        cont->suspend_نتیجہ_Offsets[i] = فوری(i32);
        cont->suspend_نتیجہ_Is64[i] = فوری(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = تہی;
    IM3Continuation تلاش_Cont = ڈھونڈو_Handler(cont, لیبل, 0, &matchedStubPC);

    اگر (M3_UNLIKELY(not تلاش_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_لیبل);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = تلاش_Cont;
    runtime->switch_ہدف = تہی;
    runtime->suspend_لیبل = لیبل;

    cont->pc = _pc;
    cont->suspend_نقطہ = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  اگر d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_دلائل_گنتی = 0;

    واپس m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
ساکن
m3ret_t تسلسل_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_عدد_Handlers, M3_حافظہ_سرخی** io_mem);
ساکن
خالی TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_عدد_Results);
ساکن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_حافظہ_سرخی* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
ساکن اندرونی
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    واپس pass and (i_cont->suspend_نقطہ == safepoint_op or i_cont->suspend_نقطہ == safepoint_اندراج);
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
ساکن
m3ret_t Replay_چوکھٹے (IM3Continuation i_cont, i32 i_گہرائی, M3_حافظہ_سرخی* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  اگر d_m3_Max_Native_ڈھیر > 0
    {
        خالی* حد_بندی = runtime->ڈھیر_حد_بندی;

        اگر (M3_UNLIKELY(حد_بندی and m3_Native_ڈھیر_Ptr() < حد_بندی)) {
            واپس m3_Err_trap_ڈھیر_حد_سے_تجاوز;
        }
    }
#  endif

    اگر (i_گہرائی < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  اگر d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        اگر (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = تہی;

            واپس m3Err_pendingException;
        }
#  endif
        اگر (pastCheck) {
            واپس ResumePastCheck(i_cont, _mem);
        }

        واپس d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3_چوکھٹا چوکھٹا = i_cont->چوکھٹے[i_گہرائی];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_چوکھٹا_قسم)چوکھٹا.قسم == چوکھٹا_resume);

#  اگر d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_چوکھٹا_قسم)چوکھٹا.قسم == چوکھٹا_try);
#  endif

    اگر (not ownDescent) {
        r = Replay_چوکھٹے(i_cont, i_گہرائی - 1, _mem);
        _mem = چوکھٹا.حافظہ->mallocated;
    }

    انتخاب ((M3_چوکھٹا_قسم)چوکھٹا.قسم) {
    حالت چوکھٹا_call:
        اگر (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            واپس d_m3CallWithRegs(چوکھٹا.pc, چوکھٹا.sp, _mem, چوکھٹا.call.r0, چوکھٹا.call.fp0);
        }

        اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            واپس Continuation_ریکارڈ_چوکھٹا(runtime, &چوکھٹا);
        }
#  اگر d_m3_ریکارڈ_Backtraces
        ڈالو_Backtrace_چوکھٹا(runtime, چوکھٹا.pc - 1);
#  endif
        واپس r;

    حالت چوکھٹا_loop: {
        m3stack_t _sp = چوکھٹا.sp;
        m3reg_t   _r0 = 0;
#  اگر d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        جب (r == (m3ret_t)چوکھٹا.pc) {
            r = jumpOpImpl(چوکھٹا.pc);
            _mem = چوکھٹا.حافظہ->mallocated;
        }

        اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            واپس Continuation_ریکارڈ_چوکھٹا(runtime, &چوکھٹا);
        }
        واپس r;
    }

#  اگر d_m3HasExceptionHandling
    حالت چوکھٹا_try: {
        m3stack_t _sp = چوکھٹا.sp;
        m3reg_t   _r0 = 0;
#    اگر d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (خالی)_sp;
        (خالی)_r0;
#    اگر d_m3HasFloat
        (خالی)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 گہرائی = ++runtime->try_گہرائی;

        اگر (not چوکھٹا.try_.handlers_زندہ) {
            runtime->try_گہرائی = گہرائی - 1;
        }

        r = Replay_چوکھٹے(i_cont, i_گہرائی - 1, _mem);
        _mem = چوکھٹا.حافظہ->mallocated;

        bool isInnermost = (runtime->try_گہرائی == گہرائی);
        runtime->try_گہرائی = گہرائی - 1;

        اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            چوکھٹا.try_.handlers_زندہ = isInnermost;
            واپس Continuation_ریکارڈ_چوکھٹا(runtime, &چوکھٹا);
        }

        اگر (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ثابت M3Exception* exception = runtime->pendingException;

            برائے (u32 i = 0; i < چوکھٹا.try_.عدد_Clauses; ++i) {
                IM3_لیبل لیبل = (IM3_لیبل)چوکھٹا.pc[2 * i];

                اگر (لیبل == تہی or لیبل == exception->لیبل) {
                    pc_t handler = (pc_t)چوکھٹا.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    واپس jumpOpImpl(handler);
                }
            }
        }
        واپس r;
    }
#  endif

    حالت چوکھٹا_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = چوکھٹا.resume.cont;

        r = تسلسل_Under_Handlers(runtime, &inner, چوکھٹا.resume.handlersPC,
                             چوکھٹا.resume.عدد_Handlers, &_mem);

        اگر (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, چوکھٹا.sp, چوکھٹا.resume.resultsPC,
                                    چوکھٹا.resume.عدد_Results);
            Continuation_Recycle_ڈھیر(runtime, inner);

            واپس d_m3CallWithRegs(چوکھٹا.pc, چوکھٹا.sp, _mem, 0, 0.);
        }

        اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            اگر (runtime->suspendHandlerCont != inner) {
                چوکھٹا.resume.cont = inner;
                واپس Continuation_ریکارڈ_چوکھٹا(runtime, &چوکھٹا);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            اگر (M3_UNLIKELY(not suspended)) {
                واپس m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            اگر (runtime->suspendStubPC) {
                واپس d_m3CallWithRegs(runtime->suspendStubPC, چوکھٹا.sp, _mem, 0, 0.);
            } ورنہ {
                واپس m3_Err_trap_Unhandled_Control_لیبل;
            }
        } ورنہ {
            inner->state = cont_consumed;
            Continuation_Recycle_ڈھیر(runtime, inner);
        }

        واپس r;
    }

#  اگر d_m3_اندراج_Keeps_چوکھٹا
    حالت چوکھٹا_اندراج:
        اگر (M3_UNLIKELY(r)) {
            اگر (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                واپس Continuation_ریکارڈ_چوکھٹا(runtime, &چوکھٹا);
            }
#    اگر d_m3_ریکارڈ_Backtraces
            FillBacktraceFunctionInfo(runtime, چوکھٹا.اندراج_2.function);
#    ورنہ
            (خالی)چوکھٹا.اندراج_2.function;
#    endif
        }
        واپس r;
#  endif
    }

    واپس r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
ساکن
خالی* ContinuationArgDest (IM3Continuation i_cont, u32 i_اشاریہ)
{
    اگر (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : تہی;
        u16         عدد_Rets = inner ? Get_Func_Type_عدد_Results(inner) : 0;

        واپس i_cont->val_ڈھیر + (عدد_Rets + i_اشاریہ) * c_io_خانہ_گنتی;
    }

    اگر (i_اشاریہ < i_cont->عدد_Suspend_Results) {
        واپس i_cont->sp + i_cont->suspend_نتیجہ_Offsets[i_اشاریہ];
    }

    واپس تہی;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
ساکن
خالی Hand_Over_مقصد_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 عدد_دلائل = i_runtime->عدد_Suspend_Payload;
    u32 بنیاد = (io_peer->state == cont_allocated) ? io_peer->bound_دلائل_گنتی : 0;

    برائے (u32 i = 0; i < عدد_دلائل; ++i) {
        خالی* dest = ContinuationArgDest(io_peer, بنیاد + i);

        اگر (dest) {
            اگر (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } ورنہ {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    خالی* refDest = ContinuationArgDest(io_peer, بنیاد + عدد_دلائل);

    اگر (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    اگر (io_peer->state == cont_allocated) {
        io_peer->bound_دلائل_گنتی += عدد_دلائل + 1;
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
ساکن
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    جب (i_cont->عدد_چوکھٹے and (M3_چوکھٹا_قسم) i_cont->چوکھٹے[0].قسم == چوکھٹا_resume) {
        i_cont = i_cont->چوکھٹے[0].resume.cont;
    }

    واپس i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
ساکن
m3ret_t تسلسل_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_عدد_Handlers, M3_حافظہ_سرخی** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation والد = i_runtime->فعال_Continuation;
    IM3_حافظہ       حافظہ = m3MemInfo(*io_mem);
    m3ret_t         r;

    برائے (;;) {
#  اگر d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        اگر (M3_UNLIKELY(cont->resumeThrow and cont->عدد_چوکھٹے == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = تہی;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            توڑو;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        اگر (isStarting) {
            اگر (M3_UNLIKELY(not cont->اندراج_Function->compiled)) {
                r = CompileFunction(cont->اندراج_Function);

                اگر (M3_UNLIKELY(r)) {
                    توڑو;
                }
            }
            cont->pc = cont->اندراج_Function->compiled;
            cont->sp = cont->val_ڈھیر;
            cont->عدد_چوکھٹے = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->عدد_Handlers = i_عدد_Handlers;
        cont->والد = والد;
        cont->state = cont_running;
        cont->bound_دلائل_گنتی = 0;

        i_runtime->فعال_Continuation = cont;

        اگر (cont->عدد_چوکھٹے) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->عدد_چوکھٹے - 1;
            cont->عدد_چوکھٹے = 0;

            r = Replay_چوکھٹے(cont, outermost, *io_mem);
        } ورنہ اگر (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } ورنہ {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = حافظہ->mallocated;
        i_runtime->فعال_Continuation = والد;

        اگر (r != m3Err_continuationSuspended) {
            توڑو;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        اگر (not i_runtime->switch_ہدف or i_runtime->suspendHandlerCont != cont) {
            توڑو;
        }

        IM3Continuation peer = i_runtime->switch_ہدف;
        i_runtime->switch_ہدف = تہی;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        اگر (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            توڑو;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_Cont_Type_کا_Ref(peer->اندراج_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_عدد_Params(peerType) - 1)));
        Hand_Over_مقصد_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    واپس r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
ساکن
خالی TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_عدد_Results)
{
    برائے (u32 i = 0; i < i_عدد_Results; ++i) {
        i32 res_خانہ = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_خانہ_گنتی;

        اگر (is64) {
            *(u64*)(i_sp + res_خانہ) = *(u64*)(i_cont->val_ڈھیر + srcIdx);
        } ورنہ {
            *(u32*)(i_sp + res_خانہ) = *(u32*)(i_cont->val_ڈھیر + srcIdx);
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
    i32             cont_خانہ = فوری(i32);
    u32             منوال = فوری(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + cont_خانہ);
    اگر (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    اگر (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         عدد_Rets = Get_Func_Type_عدد_Results(innerType);

    اگر (منوال == d_m3ResumeThrowRef) {
        i32 exn_خانہ = فوری(i32);

#  اگر d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exn_خانہ);

        اگر (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  ورنہ
        (خالی)exn_خانہ;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } ورنہ اگر (منوال == d_m3ResumeThrow) {
        IM3_لیبل exn_لیبل = فوری(IM3_لیبل);
        u32    عدد_دلائل = فوری(u32);

#  اگر d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_لیبل, عدد_دلائل);

        اگر (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        برائے (u32 i = 0; i < عدد_دلائل; ++i) {
            i32 offset = فوری(i32);
            u32 is64 = فوری(u32);

            exception->دلائل[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  ورنہ
        (خالی)exn_لیبل;
        (خالی)عدد_دلائل;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } ورنہ {
        u32 عدد_دلائل = فوری(u32);

        اگر (cont->state == cont_allocated) {
            برائے (u32 i = 0; i < عدد_دلائل; ++i) {
                i32 arg_خانہ = فوری(i32);
                u32 is64 = فوری(u32);
                u32 ہدف_Idx = (عدد_Rets + cont->bound_دلائل_گنتی + i) * c_io_خانہ_گنتی;
                اگر (is64) {
                    *(u64*)(cont->val_ڈھیر + ہدف_Idx) = *(u64*)(_sp + arg_خانہ);
                } ورنہ {
                    *(u32*)(cont->val_ڈھیر + ہدف_Idx) = *(u32*)(_sp + arg_خانہ);
                }
            }
            cont->bound_دلائل_گنتی = 0;
        } ورنہ {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            برائے (u32 i = 0; i < عدد_دلائل; ++i) {
                i32 arg_خانہ = فوری(i32);
                u32 is64 = فوری(u32);
                u32 dstIdx = cont->bound_دلائل_گنتی + i;
                اگر (dstIdx < inner->عدد_Suspend_Results) {
                    i32 dst_انحراف = inner->suspend_نتیجہ_Offsets[dstIdx];
                    اگر (is64) {
                        *(u64*)(inner->sp + dst_انحراف) = *(u64*)(_sp + arg_خانہ);
                    } ورنہ {
                        *(u32*)(inner->sp + dst_انحراف) = *(u32*)(_sp + arg_خانہ);
                    }
                }
            }
        }
    }

    u32  عدد_Handlers = فوری(u32);
    pc_t handlersPC = _pc;
    _pc += عدد_Handlers * 3;

    u32       عدد_Results = فوری(u32);
    pc_t      resultsPC = _pc;

    IM3_حافظہ حافظہ = m3MemInfo(_mem);

    m3ret_t   r = تسلسل_Under_Handlers(runtime, &cont, handlersPC, عدد_Handlers, &_mem);

    اگر (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, عدد_Results);
        Continuation_Recycle_ڈھیر(runtime, cont);

        _pc += عدد_Results * 2;
        اگلا_Op();
    } ورنہ اگر (r == m3Err_continuationSuspended) {
        اگر (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            اگر (M3_UNLIKELY(not runtime->فعال_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_لیبل);
            }
            M3_چوکھٹا چوکھٹا;
            چوکھٹا.قسم = چوکھٹا_resume;
            چوکھٹا.pc = resultsPC + عدد_Results * 2;
            چوکھٹا.sp = _sp;
            چوکھٹا.حافظہ = حافظہ;
            چوکھٹا.resume.cont = cont;
            چوکھٹا.resume.handlersPC = handlersPC;
            چوکھٹا.resume.عدد_Handlers = عدد_Handlers;
            چوکھٹا.resume.resultsPC = resultsPC;
            چوکھٹا.resume.عدد_Results = عدد_Results;

            واپس Continuation_ریکارڈ_چوکھٹا(runtime, &چوکھٹا);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        اگر (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        اگر (runtime->suspendStubPC) {
            jump_Op_براہ_راست(runtime->suspendStubPC);
        } ورنہ {
            newTrap(m3_Err_trap_Unhandled_Control_لیبل);
        }
    } ورنہ {
        cont->state = cont_consumed;
        Continuation_Recycle_ڈھیر(runtime, cont);
        ڈالو_Backtrace_چوکھٹا_2();
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

    u32 عدد_دلائل = فوری(u32);
    برائے (u32 i = 0; i < عدد_دلائل; ++i) {
        i32 offset = فوری(i32);
        u32 is64 = فوری(u32);
        u64 val = (i < runtime->عدد_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        اگر (is64) {
            *(u64*)(_sp + offset) = val;
        } ورنہ {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_انحراف = فوری(i32);
    اگر (cont_انحراف >= 0) {
        *(IM3Continuation*)(_sp + cont_انحراف) = cont;
    }

    اگلا_Op();
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

    اگر (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_لیبل);
    }

    skip_فوری(IM3FuncType);

    IM3_لیبل          لیبل = فوری(IM3_لیبل);
    i32             ہدف_خانہ = فوری(i32);

    IM3Continuation ہدف = *(IM3Continuation*)(_sp + ہدف_خانہ);

    اگر (M3_UNLIKELY(not ہدف)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    اگر (M3_UNLIKELY(ہدف->state == cont_consumed or ہدف->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = ڈھونڈو_Handler(cont, لیبل, 1, تہی);

    اگر (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_لیبل);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 عدد_دلائل = فوری(u32);

    runtime->عدد_Suspend_Payload = عدد_دلائل;

    برائے (u32 i = 0; i < عدد_دلائل; ++i) {
        i32 arg_خانہ = فوری(i32);
        u32 is64 = فوری(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + arg_خانہ) : (u64) * (u32*)(_sp + arg_خانہ);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 عدد_Results = فوری(u32);

    cont->عدد_Suspend_Results = عدد_Results;

    برائے (u32 i = 0; i < عدد_Results; ++i) {
        cont->suspend_نتیجہ_Offsets[i] = فوری(i32);
        cont->suspend_نتیجہ_Is64[i] = فوری(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = تہی;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switch_ہدف = ہدف;
    runtime->suspend_لیبل = لیبل;

    cont->pc = _pc;
    cont->suspend_نقطہ = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  اگر d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    واپس m3Err_continuationSuspended;
}


#اگر_ختم // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = فوری(pc_t);

    اگر (condition) {
        اگلا_Op();
    } ورنہ {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = خانہ(i32);

    pc_t elsePC = فوری(pc_t);

    اگر (condition) {
        اگلا_Op();
    } ورنہ {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_جدول)
{
    u32   branch_اشاریہ = خانہ(u32);           // branch index is always in a slot
    u32   عدد_Targets = فوری(u32);

    pc_t* branches = (pc_t*)_pc;

    اگر (branch_اشاریہ > عدد_Targets) {
        branch_اشاریہ = عدد_Targets; // the default index
    }

    jumpOp(branches[branch_اشاریہ]);
}


#تعریف_کرو d_m3_سیٹ_رجسٹر_سیٹ_خانہ(TYPE, REG) \
d_m3Op  (سیٹ_رجسٹر_2##TYPE)            \
{                                       \
    TYPE * src = خانہ_ptr (TYPE);       \
    d_m3_Preload_اگلا ();                 \
    REG = * src;                        \
    اگلا_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (سیٹ_خانہ_2##TYPE)                 \
{                                       \
    TYPE * dst = خانہ_ptr (TYPE);       \
    d_m3_Preload_اگلا ();                 \
    * dst = (TYPE) REG;                 \
    اگلا_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_سیٹ_خانہ_2##TYPE)         \
{                                       \
    TYPE * ڈھیر     = خانہ_ptr (TYPE); \
    TYPE * preserve  = خانہ_ptr (TYPE); \
    d_m3_Preload_اگلا ();                 \
                                        \
    * preserve = * ڈھیر;               \
    * ڈھیر = (TYPE) REG;               \
                                        \
    اگلا_Op_Preloaded ();                 \
}

d_m3_سیٹ_رجسٹر_سیٹ_خانہ(i32, _r0)
d_m3_سیٹ_رجسٹر_سیٹ_خانہ(i64, _r0)
#اگر d_m3HasFloat
d_m3_سیٹ_رجسٹر_سیٹ_خانہ(f32, _fp0)
d_m3_سیٹ_رجسٹر_سیٹ_خانہ(f64, _fp0)
#اگر_ختم

d_m3Op(Copy_خانہ_32)
{
    u32* dst = خانہ_ptr(u32);
    u32* src = خانہ_ptr(u32);
    d_m3_Preload_اگلا();

    *dst = *src;

    اگلا_Op_Preloaded();
}


d_m3Op(Preserve_Copy_خانہ_32)
{
    u32* dest = خانہ_ptr(u32);
    u32* src = خانہ_ptr(u32);
    u32* preserve = خانہ_ptr(u32);
    d_m3_Preload_اگلا();

    *preserve = *dest;
    *dest = *src;

    اگلا_Op_Preloaded();
}


d_m3Op(Copy_خانہ_64)
{
    u64* dst = خانہ_ptr(u64);
    u64* src = خانہ_ptr(u64);
    d_m3_Preload_اگلا();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    اگلا_Op_Preloaded();
}


d_m3Op(Preserve_Copy_خانہ_64)
{
    u64* dest = خانہ_ptr(u64);
    u64* src = خانہ_ptr(u64);
    u64* preserve = خانہ_ptr(u64);
    d_m3_Preload_اگلا();

    *preserve = *dest;
    *dest = *src;

    اگلا_Op_Preloaded();
}


#اگر d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_ڈھیر)
{
    u32         opcode_اشاریہ = فوری(u32);
    u32         ڈھیر_اونچائی = فوری(u32);
    IM3Function function = فوری(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    لکھو(" %4d ", opcode_اشاریہ);
    لکھو(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  اگر d_m3HasFloat
    لکھو("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    برائے (u32 i = 0; i < ڈھیر_اونچائی; ++i) {
        cstr_t قسم = "";

        لکھو("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, قسم, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    لکھو("---------------------------------------------------------------------------------------------------------\n");

    اگلا_Op_براہ_راست();
}
#اگر_ختم


#تعریف_کرو d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = خانہ (TYPE);                \
    TYPE operand1 = خانہ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    اگلا_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = خانہ (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = خانہ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    اگلا_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = خانہ (i32);                 \
                                                \
    TYPE operand2 = خانہ (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    اگلا_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = خانہ (i32);                 \
                                                \
    TYPE operand2 = خانہ (TYPE);                \
    TYPE operand1 = خانہ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    اگلا_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#تعریف_کرو d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = خانہ (TYPE);                \
    TYPE operand1 = خانہ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    اگلا_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = خانہ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    اگلا_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = خانہ (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    اگلا_Op ();                                  \
}

#اگر d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, خانہ(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, خانہ(i32))
#اگر_ختم

d_m3Op(Return)
{
    m3_ڈھیر_Check();
    واپس m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = فوری(pc_t);

    اگر (condition) {
        jumpOp(branch);
    } ورنہ {
        اگلا_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = خانہ(i32);
    pc_t branch = فوری(pc_t);

    اگر (condition) {
        jumpOp(branch);
    } ورنہ {
        اگلا_Op();
    }
}

#اگر d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE عملیہ = خانہ (TYPE);                             \
      pc_t branch  = فوری (pc_t);                        \
      اگر (عملیہ OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      ورنہ اگلا_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE عملیہ = خانہ (TYPE);                             \
      pc_t branch  = فوری (pc_t);                        \
      اگر (((TYPE) _r0) OPER عملیہ)                          \
          jumpOp (branch);                                    \
      ورنہ اگلا_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = خانہ (TYPE);                            \
      TYPE operand1 = خانہ (TYPE);                            \
      pc_t branch   = فوری (pc_t);                       \
      اگر (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      ورنہ اگلا_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE عملیہ = خانہ (TYPE);                             \
      pc_t elsePC  = فوری (pc_t);                        \
      اگر (عملیہ OPER ((TYPE) _r0))                          \
          اگلا_Op ();                                          \
      ورنہ jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE عملیہ = خانہ (TYPE);                             \
      pc_t elsePC  = فوری (pc_t);                        \
      اگر (((TYPE) _r0) OPER عملیہ)                          \
          اگلا_Op ();                                          \
      ورنہ jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = خانہ (TYPE);                            \
      TYPE operand1 = خانہ (TYPE);                            \
      pc_t elsePC   = فوری (pc_t);                       \
      اگر (operand1 OPER operand2)                             \
          اگلا_Op ();                                          \
      ورنہ jumpOp (elsePC);                                   \
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
    اگر (((i32)_r0) == 0) {
        jumpOp(branch);
    } ورنہ {
        اگلا_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  عملیہ = خانہ(i32);
    pc_t branch = فوری(pc_t);
    اگر (عملیہ == 0) {
        jumpOp(branch);
    } ورنہ {
        اگلا_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = فوری(pc_t);
    اگر (((i32)_r0) == 0) {
        اگلا_Op();
    } ورنہ {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  عملیہ = خانہ(i32);
    pc_t elsePC = فوری(pc_t);
    اگر (عملیہ == 0) {
        اگلا_Op();
    } ورنہ {
        jumpOp(elsePC);
    }
}

#اگر_ختم // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = فوری(pc_t);

    اگر (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        اگلا_Op();
    } ورنہ {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = خانہ(i32);
    pc_t branch = فوری(pc_t);

    اگر (condition) {
        اگلا_Op();
    } ورنہ {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_ڈھیر_Check();

    خالی* loopId = فوری(خالی*);
    واپس loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    خالی* loopId = فوری(خالی*);

    اگر (condition) {
        واپس loopId;
    } ورنہ {
        اگلا_Op();
    }
}


#اگر d_m3_Has_ڈھیر_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_ڈھیر_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    اگر (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
        Suspend_Without_لیبل(runtime, runtime->فعال_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_دلائل(_r0, _fp0));

        واپس m3Err_continuationSuspended;
    }

    خالی* loopId = فوری(خالی*);
    واپس loopId;
}


#  اگر !d_m3_اندراج_Keeps_چوکھٹا
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(اندراج_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = فوری(IM3Function);

    اگر (اندراج_سماتا(function, _sp, _mem)) {
        Enter_چوکھٹا(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        اگر (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
            Suspend_Without_لیبل(runtime, runtime->فعال_Continuation, safepoint_اندراج, _pc, _sp,
                              d_m3_Exp_Reg_دلائل(_r0, _fp0));

            واپس m3Err_continuationSuspended;
        }

        اگلا_Op_براہ_راست();
    } ورنہ {
        newTrap(m3_Err_trap_ڈھیر_حد_سے_تجاوز);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(اندراج_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    اگر (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
        Suspend_Without_لیبل(runtime, runtime->فعال_Continuation, safepoint_اندراج, _pc - 1, _sp,
                          d_m3_Exp_Reg_دلائل(_r0, _fp0));

        واپس m3Err_continuationSuspended;
    }

    اگلا_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    خالی* loopId = فوری(خالی*);

    اگر (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        اگر (M3_UNLIKELY(runtime->suspendRequested and runtime->فعال_Continuation)) {
            Suspend_Without_لیبل(runtime, runtime->فعال_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_دلائل(_r0, _fp0));

            واپس m3Err_continuationSuspended;
        }
        واپس loopId;
    } ورنہ {
        اگلا_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
ساکن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_حافظہ_سرخی* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  اگر d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    اگر (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } ورنہ اگر (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } ورنہ اگر (op == op_اندراج_Check) {
        واپس اگلا_Op_Impl();
    } ورنہ {
        واپس d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  اگر (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    واپس op(_pc, d_m3_Op_دلائل, d_m3_بنیاد_Cstr);
#  ورنہ
    واپس op(_pc, d_m3_Op_دلائل);
#  endif
}
#اگر_ختم // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 قدر = *(u32*)_pc++;
    خانہ(u32) = قدر;
    اگلا_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 قدر;
    حافظہ_نقل(&قدر, _pc, حجم(قدر));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    خانہ(u64) = قدر;
    اگلا_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_ڈھیر_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(انتہا_2)
{
    m3_ڈھیر_Check();
    واپس m3Err_none;
}


d_m3Op(سیٹ_Global_s32)
{
    u32* global = فوری(u32*);
    *global = خانہ(u32);

    اگلا_Op();
}


d_m3Op(سیٹ_Global_s64)
{
    u64* global = فوری(u64*);
    *global = خانہ(u64);

    اگلا_Op();
}

#اگر d_m3HasFloat
d_m3Op(سیٹ_Global_f32)
{
    f32* global = فوری(f32*);
    *global = (f32)_fp0;

    اگلا_Op();
}


d_m3Op(سیٹ_Global_f64)
{
    f64* global = فوری(f64*);
    *global = _fp0;

    اگلا_Op();
}
#اگر_ختم


#اگر d_m3HasMemory64

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
    u64  عملیہ = خانہ(u64);
    u32* پتہ = خانہ_ptr(u32);

    u64  offset;
    حافظہ_نقل(&offset, _pc, حجم(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    اگر (M3_LIKELY(عملیہ < d_m3_پتہ_حد_بندی)) {
        u64 effective = عملیہ + offset;

        // the access itself checks its own size against what is left
        اگر (M3_LIKELY(effective < _mem->لمبائی)) {
            *پتہ = (u32)effective;
            اگلا_Op();
        }
    }

    d_برآمد_کا_Bounds;
}

#اگر_ختم // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#اگر d_m3_Skip_حافظہ_Bounds_Check || d_m3_Guarded_حافظہ
#  define m3MemCheck(x) true
#ورنہ
#  define m3MemCheck(x) M3_LIKELY(x)
#اگر_ختم

// memcpy here is to support non-aligned access on some platforms.

#تعریف_کرو d_m3_بھرو(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_اگلا ();                                 \
    u64 عملیہ = (u32) _r0;                            \
    عملیہ += offset;                                  \
                                                        \
    اگر (m3MemCheck(                                     \
        عملیہ + حجم (SRC_TYPE) <= _mem->لمبائی     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_ڈیٹا(_mem) + عملیہ;       \
            SRC_TYPE قدر;                             \
            حافظہ_نقل(&قدر, src8, حجم(قدر));        \
            M3_BSWAP_##SRC_TYPE(قدر);                 \
            REG = (DEST_TYPE)قدر;                     \
            d_m3_Trace_بھرو(DEST_TYPE, عملیہ, REG);     \
        }                                               \
        اگلا_Op_Preloaded ();                             \
    } ورنہ d_برآمد_کا_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملیہ = خانہ (u32);                           \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_اگلا ();                                 \
    عملیہ += offset;                                  \
                                                        \
    اگر (m3MemCheck(                                     \
        عملیہ + حجم (SRC_TYPE) <= _mem->لمبائی     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_ڈیٹا(_mem) + عملیہ;       \
            SRC_TYPE قدر;                             \
            حافظہ_نقل(&قدر, src8, حجم(قدر));        \
            M3_BSWAP_##SRC_TYPE(قدر);                 \
            REG = (DEST_TYPE)قدر;                     \
            d_m3_Trace_بھرو(DEST_TYPE, عملیہ, REG);     \
        }                                               \
        اگلا_Op_Preloaded ();                             \
    } ورنہ d_برآمد_کا_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#تعریف_کرو d_m3_بھرو_i(DEST_TYPE, SRC_TYPE) d_m3_بھرو(_r0, DEST_TYPE, SRC_TYPE)
#تعریف_کرو d_m3_بھرو_f(DEST_TYPE, SRC_TYPE) d_m3_بھرو(_fp0, DEST_TYPE, SRC_TYPE)

#اگر d_m3HasFloat
d_m3_بھرو_f(f32, f32);
d_m3_بھرو_f(f64, f64);
#اگر_ختم

d_m3_بھرو_i(i32, i8);
d_m3_بھرو_i(i32, u8);
d_m3_بھرو_i(i32, i16);
d_m3_بھرو_i(i32, u16);
d_m3_بھرو_i(i32, i32);

d_m3_بھرو_i(i64, i8);
d_m3_بھرو_i(i64, u8);
d_m3_بھرو_i(i64, i16);
d_m3_بھرو_i(i64, u16);
d_m3_بھرو_i(i64, i32);
d_m3_بھرو_i(i64, u32);
d_m3_بھرو_i(i64, i64);

#اگر d_m3_Fold_سیٹ_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_بھرو_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = فوری (u32);                       \
      DEST_TYPE * dest = خانہ_ptr (DEST_TYPE);            \
      d_m3_Preload_اگلا ();                                 \
      u64 عملیہ = (u32) _r0;                            \
      عملیہ += offset;                                  \
                                                          \
      اگر (m3MemCheck(                                     \
          عملیہ + حجم (SRC_TYPE) <= _mem->لمبائی     \
      )) {                                                \
          u8* src8 = m3_Mem_ڈیٹا(_mem) + عملیہ;           \
          SRC_TYPE قدر;                                 \
          حافظہ_نقل(&قدر, src8, حجم(قدر));            \
          M3_BSWAP_##SRC_TYPE(قدر);                     \
          DEST_TYPE نتیجہ = (DEST_TYPE)قدر;            \
          * dest = نتیجہ;  REG = نتیجہ;                 \
          d_m3_Trace_بھرو(DEST_TYPE, عملیہ, نتیجہ);      \
          اگلا_Op_Preloaded ();                             \
      } ورنہ d_برآمد_کا_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 عملیہ = خانہ (u32);                           \
      u32 offset = فوری (u32);                       \
      DEST_TYPE * dest = خانہ_ptr (DEST_TYPE);            \
      d_m3_Preload_اگلا ();                                 \
      عملیہ += offset;                                  \
                                                          \
      اگر (m3MemCheck(                                     \
          عملیہ + حجم (SRC_TYPE) <= _mem->لمبائی     \
      )) {                                                \
          u8* src8 = m3_Mem_ڈیٹا(_mem) + عملیہ;           \
          SRC_TYPE قدر;                                 \
          حافظہ_نقل(&قدر, src8, حجم(قدر));            \
          M3_BSWAP_##SRC_TYPE(قدر);                     \
          DEST_TYPE نتیجہ = (DEST_TYPE)قدر;            \
          * dest = نتیجہ;  REG = نتیجہ;                 \
          d_m3_Trace_بھرو(DEST_TYPE, عملیہ, نتیجہ);      \
          اگلا_Op_Preloaded ();                             \
      } ورنہ d_برآمد_کا_Bounds;                               \
  }

d_m3_بھرو_Fold(_r0, i32, i8)
d_m3_بھرو_Fold(_r0, i32, u8)
d_m3_بھرو_Fold(_r0, i32, i16)
d_m3_بھرو_Fold(_r0, i32, u16)
d_m3_بھرو_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_بھرو_Fold(_r0, i64, i8)
d_m3_بھرو_Fold(_r0, i64, u8)
d_m3_بھرو_Fold(_r0, i64, i16)
d_m3_بھرو_Fold(_r0, i64, u16)
d_m3_بھرو_Fold(_r0, i64, i32)
d_m3_بھرو_Fold(_r0, i64, u32)
d_m3_بھرو_Fold(_r0, i64, i64)
#  اگر d_m3HasFloat
d_m3_بھرو_Fold(_fp0, f32, f32)
d_m3_بھرو_Fold(_fp0, f64, f64)
#  endif

#اگر_ختم // d_m3FoldSetLocal

#تعریف_کرو d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملیہ = خانہ (u32);                           \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_اگلا ();                                 \
    عملیہ += offset;                                  \
                                                        \
    اگر (m3MemCheck(                                     \
        عملیہ + حجم (DEST_TYPE) <= _mem->لمبائی    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملیہ, REG);     \
            u8* mem8 = m3_Mem_ڈیٹا(_mem) + عملیہ;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            حافظہ_نقل(mem8, &val, حجم(val));            \
        }                                               \
        اگلا_Op_Preloaded ();                             \
    } ورنہ d_برآمد_کا_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE قدر = خانہ (SRC_TYPE);             \
    u64 عملیہ = (u32) _r0;                            \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_اگلا ();                                 \
    عملیہ += offset;                                  \
                                                        \
    اگر (m3MemCheck(                                     \
        عملیہ + حجم (DEST_TYPE) <= _mem->لمبائی    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملیہ, قدر);   \
            u8* mem8 = m3_Mem_ڈیٹا(_mem) + عملیہ;       \
            DEST_TYPE val = (DEST_TYPE) قدر;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            حافظہ_نقل(mem8, &val, حجم(val));            \
        }                                               \
        اگلا_Op_Preloaded ();                             \
    } ورنہ d_برآمد_کا_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE قدر = خانہ (SRC_TYPE);             \
    u64 عملیہ = خانہ (u32);                           \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_اگلا ();                                 \
    عملیہ += offset;                                  \
                                                        \
    اگر (m3MemCheck(                                     \
        عملیہ + حجم (DEST_TYPE) <= _mem->لمبائی    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عملیہ, قدر);   \
            u8* mem8 = m3_Mem_ڈیٹا(_mem) + عملیہ;       \
            DEST_TYPE val = (DEST_TYPE) قدر;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            حافظہ_نقل(mem8, &val, حجم(val));            \
        }                                               \
        اگلا_Op_Preloaded ();                             \
    } ورنہ d_برآمد_کا_Bounds;                               \
}

// both operands can be in regs when storing a float
#تعریف_کرو d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عملیہ = (u32) _r0;                            \
    u32 offset = فوری (u32);                       \
    d_m3_Preload_اگلا ();                                 \
    عملیہ += offset;                                  \
                                                        \
    اگر (m3MemCheck(                                     \
        عملیہ + حجم (TYPE) <= _mem->لمبائی         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, عملیہ, REG);         \
            u8* mem8 = m3_Mem_ڈیٹا(_mem) + عملیہ;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            حافظہ_نقل(mem8, &val, حجم(val));            \
        }                                               \
        اگلا_Op_Preloaded ();                             \
    } ورنہ d_برآمد_کا_Bounds;                               \
}


#تعریف_کرو d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#تعریف_کرو d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#اگر d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#اگر_ختم

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#شامل "m3_exec_atomic.h"

#تعریف_منسوخ m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#اگر d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    حرف name[100];
    strcpy(name, لڑی_تلاش(i_opcode, "op_") + 3);
    حرف* bracket = لڑی_تلاش(name, "(");
    اگر (bracket) {
        *bracket = 0;
    }

    puts(name);
    اگلا_Op_براہ_راست();
}
#اگر_ختم

#اگر d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_کامیابی(i_operationName);

    اگلا_Op_براہ_راست();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_کامیابی(i_operationName);

    اگلا_Op_براہ_راست();
}
#اگر_ختم

d_m3_انتہا_Extern_C

#اگر_ختم // m3_exec_h
