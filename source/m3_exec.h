//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#جيڪڏهن_وصف_نه_ٿيل m3_exec_h
#وصف_ڪريو m3_exec_h

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

#جيڪڏهن_وصف_نه_ٿيل M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#جيڪڏهن_پورو

#ضمن "m3_math_utils.h"
#ضمن "m3_compile.h"
#ضمن "m3_env.h"
#ضمن "m3_info.h"
#ضمن "m3_exec_defs.h"
#ضمن "m3_atomic.h"
#ضمن "m3_host.h"

#ضمن <limits.h>

d_m3BeginExternC

#وصف_ڪريو rewrite_op(OP)             * ((فراغ **) (_pc-1)) = (فراغ*)(OP)

#وصف_ڪريو فوري(TYPE)            * ((TYPE *) _pc++)
#وصف_ڪريو skip_فوري(TYPE)       (_pc++)

#وصف_ڪريو جڳهه(TYPE)                 * (TYPE *) (_sp + فوري (i32))
#وصف_ڪريو جڳهه_ptr(TYPE)             (TYPE *) (_sp + فوري (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#وصف_ڪريو d_m3_Wide_عمليو(IS64)      ((IS64) ? جڳهه (u64) : (u64) جڳهه (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#وصف_ڪريو d_m3MemRangeOk(شروعات_2, ڊيگهائي_2, MEM)  \
   ((شروعات_2) <= (MEM)->ڊيگهائي and (ڊيگهائي_2) <= (MEM)->ڊيگهائي - (شروعات_2))


#إذا d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define ايندڙ_Op()                 M3_MUSTTAIL أرجع profileOp (d_m3_Op_All_دليلون, __FUNCTION__)
#وإلاإذا d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define ايندڙ_Op()                 M3_MUSTTAIL أرجع debugOp (d_m3_Op_All_دليلون, __FUNCTION__)
#وإلا
#  define ايندڙ_Op()                 ايندڙ_Op_سڌو()
#جيڪڏهن_پورو

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#إذا d_m3_Preload_ايندڙ_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_ايندڙ()        ثابت IM3Operation ايندڙ_Op_2 = (IM3Operation) (* _pc)
#  define ايندڙ_Op_Preloaded()        M3_MUSTTAIL أرجع ايندڙ_Op_2 (_pc + 1, d_m3_Op_دليلون)
#وإلا
#  define d_m3_Preload_ايندڙ()
#  define ايندڙ_Op_Preloaded()        ايندڙ_Op()
#جيڪڏهن_پورو

#إذا d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL أرجع profileJumpOp ((pc_t)(PC), d_m3_Op_دليلون, __FUNCTION__)
#وإلا
#  define jumpOp(PC)               jump_Op_سڌو(PC)
#جيڪڏهن_پورو

#إذا d_m3_رڪارڊ_Backtraces
#  define ڌڪو_Backtrace_چوکٺو_2()            (ڌڪو_Backtrace_چوکٺو (_mem->runtime, _pc - 1))
#  define fill_Backtrace_چوکٺو(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    أرجع (ڌڪو_Backtrace_چوکٺو_2 (), err)
#  define forwardTrap(err)                أرجع err
#وإلا
#  define ڌڪو_Backtrace_چوکٺو_2()            افعل {} طالما (0)
#  define fill_Backtrace_چوکٺو(FUNCTION)    افعل {} طالما (0)

#  define newTrap(err)                    أرجع err
#  define forwardTrap(err)                أرجع err
#جيڪڏهن_پورو

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#إذا d_m3_Max_Native_ڍير > 0
#  define d_m3_Check_Native_ڍير()                                               \
       افعل {                                                                    \
           فراغ * m3_حد_بندي = m3MemRuntime (_mem)->ڍير_حد_بندي;                  \
           إذا (M3_UNLIKELY (m3_حد_بندي && m3_Native_ڍير_Ptr () < m3_حد_بندي))      \
               newTrap (m3_Err_trap_ڍير_حد_کان_مٿي);                              \
       } طالما (0)
#وإلا
#  define d_m3_Check_Native_ڍير()           افعل {} طالما (0)
#جيڪڏهن_پورو


#إذا d_m3_Has_ڍير_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  إذا d_m3HasFloat
#    define d_m3_Exp_Reg_دليلون(R0, FP0)        (R0), (FP0)
#  وإلا
#    define d_m3_Exp_Reg_دليلون(R0, FP0)        (R0)
#  endif

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_دليلون (R0, FP0), d_m3_بنياد_Cstr)
#  وإلا
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_دليلون (R0, FP0))
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
m3ret_t رڪارڊ_Call_چوکٺو (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_يادگيري i_يادگيري,
                         IM3Function i_function, m3reg_t i_r0
#  إذا d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3_چوکٺو چوکٺو;

    چوکٺو.قسم = چوکٺو_call;
    چوکٺو.pc = i_pc;
    چوکٺو.sp = i_sp;
    چوکٺو.يادگيري = i_يادگيري;
    چوکٺو.call.function = i_function;
    چوکٺو.call.r0 = i_r0;
#  إذا d_m3HasFloat
    چوکٺو.call.fp0 = i_fp0;
#  endif

    أرجع Continuation_رڪارڊ_چوکٺو(i_runtime, &چوکٺو);
}

#  define d_m3_رڪارڊ_Call_چوکٺو(يادگيري_2, FUNCTION)                                \
       إذا (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           أرجع رڪارڊ_Call_چوکٺو (m3MemRuntime (_mem), _pc, _sp, (يادگيري_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_دليلون (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
ساكن M3_NOINLINE
m3ret_t رڪارڊ_Loop_چوکٺو (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_يادگيري i_يادگيري)
{
    M3_چوکٺو چوکٺو;

    چوکٺو.قسم = چوکٺو_loop;
    چوکٺو.pc = i_pc;
    چوکٺو.sp = i_sp;
    چوکٺو.يادگيري = i_يادگيري;

    أرجع Continuation_رڪارڊ_چوکٺو(i_runtime, &چوکٺو);
}


#  إذا d_m3HasExceptionHandling
ساكن M3_NOINLINE
m3ret_t رڪارڊ_Try_چوکٺو (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_يادگيري i_يادگيري,
                        u32 i_انگ_Clauses, bool i_handlers_جيئرو)
{
    M3_چوکٺو چوکٺو;

    چوکٺو.قسم = چوکٺو_try;
    چوکٺو.pc = i_clauses;
    چوکٺو.sp = i_sp;
    چوکٺو.يادگيري = i_يادگيري;
    چوکٺو.try_.انگ_Clauses = i_انگ_Clauses;
    چوکٺو.try_.handlers_جيئرو = i_handlers_جيئرو;

    أرجع Continuation_رڪارڊ_چوکٺو(i_runtime, &چوکٺو);
}
#  endif


#  إذا d_m3_داخلا_Keeps_چوکٺو
ساكن M3_NOINLINE
m3ret_t رڪارڊ_داخلا_چوکٺو (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_يادگيري i_يادگيري,
                          IM3Function i_function)
{
    M3_چوکٺو چوکٺو;

    چوکٺو.قسم = چوکٺو_داخلا;
    چوکٺو.pc = i_pc;
    چوکٺو.sp = i_sp;
    چوکٺو.يادگيري = i_يادگيري;
    چوکٺو.داخلا.function = i_function;

    أرجع Continuation_رڪارڊ_چوکٺو(i_runtime, &چوکٺو);
}
#  endif
#وإلا
#  define d_m3_رڪارڊ_Call_چوکٺو(يادگيري_2, FUNCTION)
#جيڪڏهن_پورو


#إذا d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#وإلاإذا d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ثابت IM3Runtime trace_rt = m3MemRuntime(_mem); (فراغ) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_اونهائي)*2, "", ##__VA_ARGS__)
#وإلا
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#جيڪڏهن_پورو

#إذا d_m3EnableStrace >= 3
#  define d_m3_Trace_ڀريو(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#وإلا
#  define d_m3_Trace_ڀريو(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#جيڪڏهن_پورو

#جيڪڏهن_وصف_ٿيل DEBUG
  #وصف_ڪريو d_ٻاهريون_جو_Bounds newTrap (ErrorRuntime (m3_Err_trap_ٻاهريون_جو_Bounds_يادگيري_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->ڊيگهائي, عمليو))

#  define d_ٻاهريون_جو_Bounds_Mem_Op(وٿي, سائيز_2) newTrap (ErrorRuntime (m3_Err_trap_ٻاهريون_جو_Bounds_يادگيري_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->ڊيگهائي, وٿي, سائيز_2))
#وإلا
  #وصف_ڪريو d_ٻاهريون_جو_Bounds newTrap (m3_Err_trap_ٻاهريون_جو_Bounds_يادگيري_Access)

#  define d_ٻاهريون_جو_Bounds_Mem_Op(وٿي, سائيز_2) newTrap (m3_Err_trap_ٻاهريون_جو_Bounds_يادگيري_Access)

#جيڪڏهن_پورو

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#وإلا
d_m3RetSig Call (d_m3OpSig)
#جيڪڏهن_پورو
{
    m3ret_t possible_trap = m3_Yield();
    إذا (M3_UNLIKELY(possible_trap)) {
        أرجع possible_trap;
    }

    ايندڙ_Op_سڌو();
}

#وصف_ڪريو d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE عمليو = جڳهه (TYPE);                         \
    d_m3_Preload_ايندڙ ();                                 \
    OP((RES), عمليو, ((TYPE) REG), ##__VA_ARGS__);    \
    ايندڙ_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = جڳهه (TYPE);                        \
    TYPE operand1 = جڳهه (TYPE);                        \
    d_m3_Preload_ايندڙ ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    ايندڙ_Op_Preloaded ();                                 \
}

#وصف_ڪريو d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE عمليو = جڳهه (TYPE);                         \
    d_m3_Preload_ايندڙ ();                                 \
    OP((RES), ((TYPE) REG), عمليو, ##__VA_ARGS__);    \
    ايندڙ_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#وصف_ڪريو d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#وصف_ڪريو d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#وصف_ڪريو d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#وصف_ڪريو d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#وصف_ڪريو M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#وصف_ڪريو M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#وصف_ڪريو M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#وصف_ڪريو M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#وصف_ڪريو d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#وصف_ڪريو d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#وصف_ڪريو d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#وصف_ڪريو d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#وصف_ڪريو d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#وصف_ڪريو d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#وصف_ڪريو d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#وصف_ڪريو d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#وصف_ڪريو d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#وصف_ڪريو d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#وصف_ڪريو d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#وصف_ڪريو d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#وصف_ڪريو d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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
#جيڪڏهن_پورو

#وصف_ڪريو OP_جوڙيو_32(A,B) (i32)((u32)(A) + (u32)(B))
#وصف_ڪريو OP_جوڙيو_64(A,B) (i64)((u64)(A) + (u64)(B))
#وصف_ڪريو OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#وصف_ڪريو OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#وصف_ڪريو OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#وصف_ڪريو OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, جوڙيو,      OP_جوڙيو_32)  d_m3CommutativeOpFunc_i (i64, جوڙيو,      OP_جوڙيو_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, ڪٽيو,            OP_SUB_32)  d_m3OpFunc_i (i64, ڪٽيو,            OP_SUB_64)

#وصف_ڪريو OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#وصف_ڪريو OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#وصف_ڪريو OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#وصف_ڪريو OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, سرڪايو_کاٻو,       OP_SHL_32)      d_m3OpFunc_i (u64, سرڪايو_کاٻو,       OP_SHL_64)
d_m3OpFunc_i (i32, سرڪايو_ساڄو,      OP_SHR_32)      d_m3OpFunc_i (i64, سرڪايو_ساڄو,      OP_SHR_64)
d_m3OpFunc_i (u32, سرڪايو_ساڄو,      OP_SHR_32)      d_m3OpFunc_i (u64, سرڪايو_ساڄو,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#إذا d_m3_Fold_سيٽ_Local

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
    TYPE عمليو = جڳهه (TYPE);                         \
    TYPE * dest = جڳهه_ptr (TYPE);                      \
    d_m3_Preload_ايندڙ ();                                 \
    TYPE نتيجو;                                        \
    OP((نتيجو), عمليو, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = نتيجو;  REG = نتيجو;                     \
    ايندڙ_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = جڳهه (TYPE);                        \
    TYPE operand1 = جڳهه (TYPE);                        \
    TYPE * dest = جڳهه_ptr (TYPE);                      \
    d_m3_Preload_ايندڙ ();                                 \
    TYPE نتيجو;                                        \
    OP((نتيجو), operand1, operand2, ##__VA_ARGS__);    \
    * dest = نتيجو;  REG = نتيجو;                     \
    ايندڙ_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE عمليو = جڳهه (TYPE);                         \
    TYPE * dest = جڳهه_ptr (TYPE);                      \
    d_m3_Preload_ايندڙ ();                                 \
    TYPE نتيجو;                                        \
    OP((نتيجو), ((TYPE) REG), عمليو, ##__VA_ARGS__); \
    * dest = نتيجو;  REG = نتيجو;                     \
    ايندڙ_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#وصف_ڪريو d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#وصف_ڪريو d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#وصف_ڪريو d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#وصف_ڪريو d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, جوڙيو,          M3_FUNC, OP_جوڙيو_32)
d_m3FoldOp_i           (i32, ڪٽيو,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, سرڪايو_کاٻو,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, سرڪايو_ساڄو,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, سرڪايو_ساڄو,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, جوڙيو,          M3_FUNC, OP_جوڙيو_64)
d_m3FoldOp_i           (i64, ڪٽيو,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, سرڪايو_کاٻو,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, سرڪايو_ساڄو,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, سرڪايو_ساڄو,   M3_FUNC, OP_SHR_64)

#  إذا d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, جوڙيو,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, ڪٽيو,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, جوڙيو,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, ڪٽيو,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#جيڪڏهن_پورو

#جيڪڏهن_پورو // d_m3FoldSetLocal

#إذا d_m3HasFloat
d_m3CommutativeArithOp_f(f32, جوڙيو,         +)      d_m3CommutativeArithOp_f(f64, جوڙيو,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, ڪٽيو,               -)      d_m3ArithOp_f(f64, ڪٽيو,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#جيڪڏهن_پورو

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
#جيڪڏهن_پورو

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#وصف_ڪريو d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    ايندڙ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE عمليو = جڳهه (TYPE);                     \
    OP((RES), عمليو, ##__VA_ARGS__);              \
    ايندڙ_Op ();                                      \
}

#وصف_ڪريو M3_UNARY(RES, X, OP) (RES) = OP(X)
#وصف_ڪريو M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#وصف_ڪريو d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#وصف_ڪريو d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#وصف_ڪريو d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#إذا d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       جذر);
d_m3UnaryArithOp_f(f32, سڀ_کان_ويجهو,    rintf);    d_m3UnaryArithOp_f(f64, سڀ_کان_ويجهو,    rint);
#  إذا defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  وإلا
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#جيڪڏهن_پورو

#وصف_ڪريو OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_منزل_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_منزل_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#إذا (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#وصف_ڪريو OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#وصف_ڪريو OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#وصف_ڪريو OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#وصف_ڪريو OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#وإلاإذا defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#وصف_ڪريو OP_CLZ_32(x) __builtin_clz(x)
#وصف_ڪريو OP_CTZ_32(x) __builtin_ctz(x)
#وصف_ڪريو OP_CLZ_64(x) __builtin_clzll(x)
#وصف_ڪريو OP_CTZ_64(x) __builtin_ctzll(x)
#وإلا
#وصف_ڪريو OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#وصف_ڪريو OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#وصف_ڪريو OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#وصف_ڪريو OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#جيڪڏهن_پورو

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#وصف_ڪريو OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    ايندڙ_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 عمليو = جڳهه(i64);
    _r0 = OP_WRAP_I64(عمليو);
    ايندڙ_Op();
}

// Integer sign extension operations
#وصف_ڪريو OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#وصف_ڪريو OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#وصف_ڪريو OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#وصف_ڪريو OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#وصف_ڪريو OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

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
    u64  rhs_مٿاهون = جڳهه (u64);
    u64  rhs_هيٺيون  = جڳهه (u64);
    u64  lhs_مٿاهون = جڳهه (u64);
    u64  lhs_هيٺيون  = جڳهه (u64);
    u64* هيٺيون     = جڳهه_ptr (u64);
    d_m3_Preload_ايندڙ();

    u64 جوڙ = lhs_هيٺيون + rhs_هيٺيون;

    *هيٺيون = جوڙ;
    _r0  = (i64) (lhs_مٿاهون + rhs_مٿاهون + (جوڙ < lhs_هيٺيون));

    ايندڙ_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_مٿاهون = جڳهه (u64);
    u64  rhs_هيٺيون  = جڳهه (u64);
    u64  lhs_مٿاهون = جڳهه (u64);
    u64  lhs_هيٺيون  = جڳهه (u64);
    u64* هيٺيون     = جڳهه_ptr (u64);
    d_m3_Preload_ايندڙ();

    *هيٺيون = lhs_هيٺيون - rhs_هيٺيون;
    _r0  = (i64) (lhs_مٿاهون - rhs_مٿاهون - (lhs_هيٺيون < rhs_هيٺيون));

    ايندڙ_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = جڳهه (i64);
    i64  a   = جڳهه (i64);
    u64* هيٺيون = جڳهه_ptr (u64);
    d_m3_Preload_ايندڙ();

    u64 مٿاهون;
    *هيٺيون = mul_wide_i64 (a, b, &مٿاهون);
    _r0  = (i64) مٿاهون;

    ايندڙ_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = جڳهه (u64);
    u64  a   = جڳهه (u64);
    u64* هيٺيون = جڳهه_ptr (u64);
    d_m3_Preload_ايندڙ();

    u64 مٿاهون;
    *هيٺيون = mul_wide_u64 (a, b, &مٿاهون);
    _r0  = (i64) مٿاهون;

    ايندڙ_Op_Preloaded();
}

#جيڪڏهن_پورو // d_m3HasWideArithmetic

#وصف_ڪريو d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    ايندڙ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * ڍير = جڳهه_ptr (FROM);                 \
    OP((DEST), (* ڍير), ##__VA_ARGS__);           \
    ايندڙ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = جڳهه_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    ايندڙ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * ڍير = جڳهه_ptr (FROM);                 \
    TYPE * dest = جڳهه_ptr (TYPE);                  \
    OP((* dest), (* ڍير), ##__VA_ARGS__);         \
    ايندڙ_Op ();                                      \
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
#جيڪڏهن_پورو

#وصف_ڪريو d_m3TypeModifyOp(REG_منزل, REG_FROM, منزل_2, NAME, FROM)  \
d_m3Op(منزل_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_منزل = (منزل_2) ((FROM) REG_FROM);                        \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = جڳهه (FROM);                                \
    REG_منزل = (منزل_2) (from);                                   \
    ايندڙ_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#وصف_ڪريو d_m3FloatModifyOp(منزل_2, NAME, FROM)                   \
d_m3Op(منزل_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((منزل_2) ((FROM) _fp0));             \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = جڳهه (FROM);                                \
    _fp0 = canon_nan_##TO ((منزل_2) (from));                    \
    ايندڙ_Op ();                                              \
}

#إذا d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#جيڪڏهن_پورو

#وصف_ڪريو d_m3TypeConvertOp(REG_منزل, REG_FROM, منزل_2, NAME, FROM) \
d_m3Op(منزل_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_منزل = (منزل_2) ((FROM) REG_FROM);                        \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    جڳهه (منزل_2) = (منزل_2) ((FROM) REG_FROM);                     \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = جڳهه (FROM);                                \
    REG_منزل = (منزل_2) (from);                                   \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = جڳهه (FROM);                                \
    جڳهه (منزل_2) = (منزل_2) (from);                                \
    ايندڙ_Op ();                                              \
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
#جيڪڏهن_پورو

#وصف_ڪريو d_m3ReinterpretOp(REG, منزل_2, SRC, FROM)               \
d_m3Op(منزل_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = جڳهه (FROM);                                      \
    REG = u.t;                                              \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    جڳهه (منزل_2) = u.t;                                        \
    ايندڙ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(منزل_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    اتحاد { FROM c; منزل_2 t; } u;                              \
    u.c = جڳهه (FROM);                                      \
    جڳهه (منزل_2) = u.t;                                        \
    ايندڙ_Op ();                                              \
}

#إذا d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#جيڪڏهن_پورو

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = فوري(u32*);
    جڳهه(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    ايندڙ_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = فوري(u64*);
    جڳهه(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    ايندڙ_Op();
}


d_m3Op(سيٽ_Global_i32)
{
    u32* global = فوري(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    ايندڙ_Op();
}


d_m3Op(سيٽ_Global_i64)
{
    u64* global = فوري(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    ايندڙ_Op();
}


#إذا d_m3_Has_ڍير_Switching

d_m3Op(Branch);
d_m3Op(داخلا_2);
#  إذا !d_m3_داخلا_Keeps_چوکٺو
d_m3Op(داخلا_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
ساكن M3_NOINLINE
IM3Function Function_جو_Compiled_ڪوڊ (pc_t i_pc)
{
    إذا (not i_pc) {
        أرجع عدم;
    }

    طالما ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation داخلا = (IM3Operation)i_pc[0];

#  إذا !d_m3_داخلا_Keeps_چوکٺو
    إذا (داخلا == op_داخلا_Suspendable) {
        أرجع (IM3Function)i_pc[1];
    }
#  endif
    أرجع (داخلا == op_داخلا) ? (IM3Function)i_pc[1] : عدم;
}

#جيڪڏهن_پورو

d_m3Op(Call)
{
    d_m3_Check_Native_ڍير();

    pc_t      callPC = فوري(pc_t);
    i32       ڍير_وٿي = فوري(i32);
    IM3_يادگيري يادگيري = m3MemInfo(_mem);

    m3stack_t sp = _sp + ڍير_وٿي;

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_دليلون, d_m3_بنياد_Cstr);
#وإلا
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_دليلون);
#جيڪڏهن_پورو

    _mem = يادگيري->mallocated;

    إذا (M3_LIKELY(not r)) {
        ايندڙ_Op();
    } وإلا {
        d_m3_رڪارڊ_Call_چوکٺو(يادگيري, Function_جو_Compiled_ڪوڊ(callPC));
        ڌڪو_Backtrace_چوکٺو_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = جڳهه(IM3Function);
    IM3FuncType type = فوري(IM3FuncType);
    i32         ڍير_وٿي = فوري(i32);
    IM3_يادگيري   يادگيري = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ڍير_وٿي;

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(function)) {
        إذا (M3_LIKELY(type == function->funcType)) {
            إذا (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            إذا (M3_LIKELY(not r)) {
#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_دليلون, d_m3_بنياد_Cstr);
#وإلا
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_دليلون);
#جيڪڏهن_پورو

                _mem = يادگيري->mallocated;

                إذا (M3_LIKELY(not r)) {
                    ايندڙ_Op_سڌو();
                } وإلا {
                    d_m3_رڪارڊ_Call_چوکٺو(يادگيري, function);
                    ڌڪو_Backtrace_چوکٺو_2();
                    forwardTrap(r);
                }
            }
        } وإلا {
            r = m3_Err_trap_اڻسڌو_Call_Type_Mismatch;
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
    IM3Function reference = جڳهه(IM3Function);

    إذا (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    ايندڙ_Op();
}


d_m3Op(Call_اڻسڌو)
{
    d_m3_Check_Native_ڍير();

    M3_جدول*    جدول_2 = فوري(M3_جدول*);
    u64         جدول_اشاريو = d_m3_Wide_عمليو(جدول_2->isTable64);
    IM3FuncType type = فوري(IM3FuncType);
    i32         ڍير_وٿي = فوري(i32);
    IM3_يادگيري   يادگيري = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ڍير_وٿي;

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(جدول_اشاريو < جدول_2->سائيز)) {
        IM3Function function = (IM3Function)جدول_2->elements[جدول_اشاريو];

        إذا (M3_LIKELY(function)) {
            إذا (M3_LIKELY(type == function->funcType)) {
                إذا (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                إذا (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3_يادگيري_سرو* calleeMem = Module_يادگيري_سرو(function->module);

#إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_دليلون, d_m3_بنياد_Cstr);
#وإلا
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_دليلون);
#جيڪڏهن_پورو

                    _mem = يادگيري->mallocated;

                    إذا (M3_LIKELY(not r)) {
                        ايندڙ_Op_سڌو();
                    } وإلا {
                        d_m3_رڪارڊ_Call_چوکٺو(يادگيري, function);
                        ڌڪو_Backtrace_چوکٺو_2();
                        forwardTrap(r);
                    }
                }
            } وإلا {
                r = m3_Err_trap_اڻسڌو_Call_Type_Mismatch;
            }
        } وإلا {
            r = m3_Err_trap_جدول_Element_Is_Null;
        }
    } وإلا {
        r = m3_Err_trap_جدول_اشاريو_ٻاهريون_جو_Range;
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
#وصف_ڪريو d_m3_پڇ_Call_دليلون(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((فراغ *) (_sp + (RETURNSLOTS)), (ثابت فراغ *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * حجم (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = فوري(pc_t);
    i32     ڍير_وٿي = فوري(i32);
    i32     returnSlots = فوري(i32);
    u32     انگ_Arg_Slots = فوري(u32);

    m3ret_t possible_trap = m3_Yield();
    إذا (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_پڇ_Call_دليلون(returnSlots, ڍير_وٿي, انگ_Arg_Slots);

    jump_Op_سڌو(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = جڳهه(IM3Function);
    IM3FuncType type = فوري(IM3FuncType);
    i32         ڍير_وٿي = فوري(i32);
    i32         returnSlots = فوري(i32);
    u32         انگ_Arg_Slots = فوري(u32);

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(function)) {
        إذا (M3_LIKELY(type == function->funcType)) {
            إذا (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            إذا (M3_LIKELY(not r)) {
                r = m3_Yield();

                إذا (M3_LIKELY(not r)) {
                    d_m3_پڇ_Call_دليلون(returnSlots, ڍير_وٿي, انگ_Arg_Slots);

                    jump_Op_سڌو(function->compiled);
                }
            }
        } وإلا {
            r = m3_Err_trap_اڻسڌو_Call_Type_Mismatch;
        }
    } وإلا {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(Return_Call_اڻسڌو)
{
    M3_جدول*    جدول_2 = فوري(M3_جدول*);
    u64         جدول_اشاريو = d_m3_Wide_عمليو(جدول_2->isTable64);
    IM3FuncType type = فوري(IM3FuncType);
    i32         ڍير_وٿي = فوري(i32);
    i32         returnSlots = فوري(i32);
    u32         انگ_Arg_Slots = فوري(u32);

    m3ret_t     r = m3Err_none;

    إذا (M3_LIKELY(جدول_اشاريو < جدول_2->سائيز)) {
        IM3Function function = (IM3Function)جدول_2->elements[جدول_اشاريو];

        إذا (M3_LIKELY(function)) {
            إذا (M3_LIKELY(type == function->funcType)) {
                إذا (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                إذا (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    إذا (M3_LIKELY(not r)) {
                        d_m3_پڇ_Call_دليلون(returnSlots, ڍير_وٿي, انگ_Arg_Slots);

                        jump_Op_سڌو(function->compiled);
                    }
                }
            } وإلا {
                r = m3_Err_trap_اڻسڌو_Call_Type_Mismatch;
            }
        } وإلا {
            r = m3_Err_trap_جدول_Element_Is_Null;
        }
    } وإلا {
        r = m3_Err_trap_جدول_اشاريو_ٻاهريون_جو_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = فوري(IM3Function);
    ctx.userdata = فوري(فراغ*);
    u64* ثابت sp = ((u64*)_sp);
    IM3_يادگيري  يادگيري = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#إذا d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    حرف        outbuff[1024];
    حرف*       outp = outbuff;
    حرف*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.ميدان_Utf8);

    ثابت صحيح n_دليلون = ftype->انگ_دليلون;
    ثابت صحيح nRets = ftype->انگ_Rets;
    u64*      دليلون = sp + nRets;
    لكل (صحيح i = 0; i < n_دليلون; i++) {
        ثابت صحيح type = ftype->types[nRets + i];
        بدل (type) {
        حالة c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(دليلون + i)); اكسر;
        حالة c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(دليلون + i)); اكسر;
        حالة c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(دليلون + i)); اكسر;
        حالة c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(دليلون + i)); اكسر;
        افتراضي: outp += snprintf(outp, oute - outp, "<type %d>", type); اكسر;
        }
        إذا (i < n_دليلون - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  إذا d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#جيڪڏهن_پورو

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    فراغ* ڍير_backup = runtime->ڍير;
    runtime->ڍير = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_معلومات(ctx.function->host_يادگيري->mallocated));
    runtime->ڍير = ڍير_backup;

#إذا d_m3EnableStrace
    إذا (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (حرف*)possible_trap);
    } وإلا إذا (ftype and ftype->انگ_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } وإلا {
        d_m3TracePrint("%s", outbuff);
    }
#جيڪڏهن_پورو

    إذا (M3_UNLIKELY(possible_trap)) {
        _mem = يادگيري->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        إذا (possible_trap != m3Err_pendingException) {
            ڌڪو_Backtrace_چوکٺو_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_سائيز)
{
    IM3_يادگيري يادگيري = m3MemInfo(_mem);

    _r0 = يادگيري->انگ_صفحا;

    ايندڙ_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_يادگيري  يادگيري = m3MemInfo(_mem);

    i32        انگ_صفحا_منزل_Grow = (i32)_r0;
    إذا (انگ_صفحا_منزل_Grow >= 0) {
        _r0 = (m3reg_t)يادگيري->انگ_صفحا;

        إذا (M3_LIKELY(انگ_صفحا_منزل_Grow)) {
            u64      required_صفحا = يادگيري->انگ_صفحا + (u32)انگ_صفحا_منزل_Grow;

            M3_نتيجو r = Resize_يادگيري(runtime, يادگيري, required_صفحا);
            إذا (r) {
                _r0 = -1;
            }

            _mem = يادگيري->mallocated;
        }
    } وإلا {
        _r0 = -1;
    }

    ايندڙ_Op();
}


#إذا d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_يادگيري  يادگيري = m3MemInfo(_mem);

    u64        انگ_صفحا_منزل_Grow = (u64)_r0;
    u64        انگ_صفحا = يادگيري->انگ_صفحا;

    إذا (M3_LIKELY(انگ_صفحا_منزل_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        إذا (انگ_صفحا_منزل_Grow <= يادگيري->max_صفحا - انگ_صفحا and
            not Resize_يادگيري(runtime, يادگيري, انگ_صفحا + انگ_صفحا_منزل_Grow)) {
            _r0 = (m3reg_t)انگ_صفحا;
        } وإلا {
            _r0 = -1;
        }

        _mem = يادگيري->mallocated;
    } وإلا {
        _r0 = (m3reg_t)انگ_صفحا;
    }

    ايندڙ_Op();
}

#جيڪڏهن_پورو // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 سائيز = (u32)_r0;
    u64 ذريعو = جڳهه(u32);
    u64 destination = جڳهه(u32);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, سائيز, _mem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(ذريعو, سائيز, _mem))) {
            u8* dst = m3_Mem_معلومات(_mem) + destination;
            u8* src = m3_Mem_معلومات(_mem) + ذريعو;
            memmove(dst, src, (size_t)سائيز);

            ايندڙ_Op();
        } وإلا {
            d_ٻاهريون_جو_Bounds_Mem_Op(ذريعو, سائيز);
        }
    } وإلا {
        d_ٻاهريون_جو_Bounds_Mem_Op(destination, سائيز);
    }
}


#إذا d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 سائيز = (u64)_r0;
    u64 ذريعو = جڳهه(u64);
    u64 destination = جڳهه(u64);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, سائيز, _mem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(ذريعو, سائيز, _mem))) {
            u8* dst = m3_Mem_معلومات(_mem) + destination;
            u8* src = m3_Mem_معلومات(_mem) + ذريعو;
            memmove(dst, src, (size_t)سائيز);

            ايندڙ_Op();
        } وإلا {
            d_ٻاهريون_جو_Bounds_Mem_Op(ذريعو, سائيز);
        }
    } وإلا {
        d_ٻاهريون_جو_Bounds_Mem_Op(destination, سائيز);
    }
}

#جيڪڏهن_پورو // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(سيٽ_يادگيري)
{
    IM3_يادگيري يادگيري = فوري(IM3_يادگيري);

    _mem = يادگيري->mallocated;

    ايندڙ_Op();
}


#إذا d_m3_Has_Multi_يادگيري

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3_يادگيري       dest_يادگيري = فوري(IM3_يادگيري);
    IM3_يادگيري       ذريعو_يادگيري = فوري(IM3_يادگيري);
    u32             widths = فوري(u32);

    M3_يادگيري_سرو* destMem = dest_يادگيري->mallocated;
    M3_يادگيري_سرو* ذريعو_Mem = ذريعو_يادگيري->mallocated;

    u64             سائيز = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             ذريعو = d_m3_Wide_عمليو(widths & 0x2);
    u64             destination = d_m3_Wide_عمليو(widths & 0x1);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, سائيز, destMem))) {
        إذا (M3_LIKELY(d_m3MemRangeOk(ذريعو, سائيز, ذريعو_Mem))) {
            memmove(m3_Mem_معلومات(destMem) + destination, m3_Mem_معلومات(ذريعو_Mem) + ذريعو, (size_t)سائيز);

            ايندڙ_Op();
        } وإلا {
            d_ٻاهريون_جو_Bounds_Mem_Op(ذريعو, سائيز);
        }
    } وإلا {
        d_ٻاهريون_جو_Bounds_Mem_Op(destination, سائيز);
    }
}

#جيڪڏهن_پورو // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 سائيز = (u32)_r0;
    u32 byte = جڳهه(u32);
    u64 destination = جڳهه(u32);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, سائيز, _mem))) {
        u8* mem8 = m3_Mem_معلومات(_mem) + destination;
        memset(mem8, (u8)byte, سائيز);
        ايندڙ_Op();
    } وإلا {
        d_ٻاهريون_جو_Bounds_Mem_Op(destination, سائيز);
    }
}


#إذا d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 سائيز = (u64)_r0;
    u32 byte = جڳهه(u32);
    u64 destination = جڳهه(u64);

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, سائيز, _mem))) {
        u8* mem8 = m3_Mem_معلومات(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)سائيز);
        ايندڙ_Op();
    } وإلا {
        d_ٻاهريون_جو_Bounds_Mem_Op(destination, سائيز);
    }
}

#جيڪڏهن_پورو // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_معلومات_Segment* segment = فوري(M3_معلومات_Segment*);

    u64            سائيز = (u32)_r0;
    u64            ذريعو = جڳهه(u32);
    u64            destination = جڳهه(u32);

    u64            موجود = segment->dropped ? 0 : segment->سائيز;

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, سائيز, _mem))) {
        إذا (M3_LIKELY(ذريعو <= موجود and سائيز <= موجود - ذريعو)) {
            يادگيري_نقل(m3_Mem_معلومات(_mem) + destination, segment->معلومات + ذريعو, (size_t)سائيز);
            ايندڙ_Op();
        } وإلا {
            d_ٻاهريون_جو_Bounds_Mem_Op(ذريعو, سائيز);
        }
    } وإلا {
        d_ٻاهريون_جو_Bounds_Mem_Op(destination, سائيز);
    }
}


#إذا d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_معلومات_Segment* segment = فوري(M3_معلومات_Segment*);

    u64            سائيز = (u32)_r0;
    u64            ذريعو = جڳهه(u32);
    u64            destination = جڳهه(u64);

    u64            موجود = segment->dropped ? 0 : segment->سائيز;

    إذا (M3_LIKELY(d_m3MemRangeOk(destination, سائيز, _mem))) {
        إذا (M3_LIKELY(ذريعو <= موجود and سائيز <= موجود - ذريعو)) {
            يادگيري_نقل(m3_Mem_معلومات(_mem) + destination, segment->معلومات + ذريعو, (size_t)سائيز);
            ايندڙ_Op();
        } وإلا {
            d_ٻاهريون_جو_Bounds_Mem_Op(ذريعو, سائيز);
        }
    } وإلا {
        d_ٻاهريون_جو_Bounds_Mem_Op(destination, سائيز);
    }
}

#جيڪڏهن_پورو // d_m3HasMemory64


d_m3Op(معلومات_Drop)
{
    M3_معلومات_Segment* segment = فوري(M3_معلومات_Segment*);
    segment->dropped = true;

    ايندڙ_Op();
}


#إذا d_m3HasRefTypes

d_m3Op(جدول_Get)
{
    M3_جدول* جدول_2 = فوري(M3_جدول*);
    u64      index = d_m3_Wide_عمليو(جدول_2->isTable64);

    إذا (M3_LIKELY(index < جدول_2->سائيز)) {
        _r0 = (u64)(uintptr_t)جدول_2->elements[index];
        ايندڙ_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_ٻاهريون_جو_Bounds);
    }
}


d_m3Op(جدول_سيٽ)
{
    M3_جدول* جدول_2 = فوري(M3_جدول*);
    فراغ*    قدر = جڳهه(فراغ*);
    u64      index = d_m3_Wide_عمليو(جدول_2->isTable64);

    إذا (M3_LIKELY(index < جدول_2->سائيز)) {
        جدول_2->elements[index] = قدر;
        ايندڙ_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_ٻاهريون_جو_Bounds);
    }
}


d_m3Op(جدول_سائيز)
{
    M3_جدول* جدول_2 = فوري(M3_جدول*);

    _r0 = جدول_2->سائيز;

    ايندڙ_Op();
}


d_m3Op(جدول_Grow)
{
    M3_جدول* جدول_2 = فوري(M3_جدول*);
    u64      delta = d_m3_Wide_عمليو(جدول_2->isTable64);
    فراغ*    قدر = جڳهه(فراغ*);

    u32      old_سائيز = جدول_2->سائيز;
    u32      max_سائيز = جدول_2->max_سائيز ? جدول_2->max_سائيز : d_m3_Max_Sane_جدول_سائيز;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_سائيز = (delta <= max_سائيز - old_سائيز) ? (u64)old_سائيز + delta
                                                    : (u64)max_سائيز + 1;

    إذا (new_سائيز == old_سائيز)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_سائيز;
        ايندڙ_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (new_سائيز <= max_سائيز and (not runtime->جدول_Elements_حد_بندي or
                                delta <= runtime->جدول_Elements_حد_بندي - runtime->جدول_Elements_استعمال_ٿيل)) {
        فراغ** elements = m3_ReallocArray(فراغ*, جدول_2->elements, (size_t)new_سائيز, old_سائيز);

        إذا (elements) {
            runtime->جدول_Elements_استعمال_ٿيل += new_سائيز - old_سائيز;
            جدول_2->elements = elements;
            جدول_2->سائيز = (u32)new_سائيز;

            لكل (u32 i = old_سائيز; i < جدول_2->سائيز; ++i) {
                جدول_2->elements[i] = قدر;
            }

            _r0 = old_سائيز;
            ايندڙ_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    ايندڙ_Op();
}


d_m3Op(جدول_Init)
{
    M3_جدول*          جدول_2 = فوري(M3_جدول*);
    M3ElementSegment* segment = فوري(M3ElementSegment*);
    u32               ڳڻپ = جڳهه(u32);
    u32               ذريعو = جڳهه(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_عمليو(جدول_2->isTable64);

    u64               موجود = segment->dropped ? 0 : segment->انگ_Elements;

    إذا (M3_LIKELY(destination <= جدول_2->سائيز and ڳڻپ <= جدول_2->سائيز - destination and
                  (u64) ذريعو + ڳڻپ <= موجود)) {
        لكل (u32 i = 0; i < ڳڻپ; ++i) {
            جدول_2->elements[destination + i] = segment->resolved[ذريعو + i];
        }

        ايندڙ_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_ٻاهريون_جو_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = فوري(M3ElementSegment*);
    segment->dropped = true;

    ايندڙ_Op();
}


d_m3Op(جدول_Copy)
{
    M3_جدول* dst = فوري(M3_جدول*);
    M3_جدول* src = فوري(M3_جدول*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      ڳڻپ = d_m3_Wide_عمليو(dst->isTable64 and src->isTable64);
    u64      ذريعو = d_m3_Wide_عمليو(src->isTable64);
    u64      destination = d_m3_Wide_عمليو(dst->isTable64);

    إذا (M3_LIKELY(destination <= dst->سائيز and ڳڻپ <= dst->سائيز - destination and
                  ذريعو <= src->سائيز and ڳڻپ <= src->سائيز - ذريعو)) {
        memmove(dst->elements + destination, src->elements + ذريعو, (size_t)ڳڻپ * حجم(فراغ*));

        ايندڙ_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_ٻاهريون_جو_Bounds);
    }
}


d_m3Op(جدول_Fill)
{
    M3_جدول* جدول_2 = فوري(M3_جدول*);
    u64      ڳڻپ = d_m3_Wide_عمليو(جدول_2->isTable64);
    فراغ*    قدر = جڳهه(فراغ*);
    u64      index = d_m3_Wide_عمليو(جدول_2->isTable64);

    إذا (M3_LIKELY(index <= جدول_2->سائيز and ڳڻپ <= جدول_2->سائيز - index)) {
        لكل (u64 i = 0; i < ڳڻپ; ++i) {
            جدول_2->elements[index + i] = قدر;
        }

        ايندڙ_Op();
    } وإلا {
        newTrap(m3_Err_trap_جدول_ٻاهريون_جو_Bounds);
    }
}

#جيڪڏهن_پورو // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = فوري(IM3Function);

    m3ret_t     نتيجو = m3Err_none;

    إذا (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        نتيجو = CompileFunction(function);
    }

    إذا (not نتيجو) {
        // patch up compiled pc and call rewritten op_Call
        *((فراغ**)--_pc) = (فراغ*)(function->compiled);
        --_pc;
        ايندڙ_Op_سڌو();
    }

    newTrap(نتيجو);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = فوري(IM3Function);

    m3ret_t     نتيجو = m3Err_none;

    إذا (M3_UNLIKELY(not function->compiled)) {
        نتيجو = CompileFunction(function);
    }

    إذا (not نتيجو) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((فراغ**)--_pc) = (فراغ*)(function->compiled);
        --_pc;
        ايندڙ_Op_سڌو();
    }

    newTrap(نتيجو);
}


#إذا d_m3_Has_ڍير_Switching
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
فراغ Suspend_Without_ٽيگ (IM3Runtime io_runtime, IM3Continuation io_cont, M3_محفوظ_نقطو_قسم i_قسم,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  إذا d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_ٽيگ = عدم;
    io_runtime->suspendHandlerCont = عدم;
    io_runtime->suspendStubPC = عدم;
    io_runtime->switch_هدف = عدم;
    io_runtime->انگ_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspend_نقطو = (u8)i_قسم;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  إذا d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->انگ_Suspend_Results = 0;
}
#جيڪڏهن_پورو // d_m3HasStackSwitching


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
    u32        لاڳت = فوري(u32);

    runtime->gas_بچيل -= (i64)لاڳت;

    إذا (M3_UNLIKELY(runtime->gas_بچيل < 0)) {
#  إذا d_m3_Has_ڍير_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        إذا (runtime->isSuspendable and runtime->سرگرم_Continuation) {
            runtime->suspendRequested = true;
            ايندڙ_Op();
        }
#  endif
        newTrap(m3_Err_trap_ٻاهريون_جو_Gas);
    }

    ايندڙ_Op();
}

#جيڪڏهن_پورو // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
ساكن مضمن
bool داخلا_سمائي (IM3Function i_function, m3stack_t _sp, M3_يادگيري_سرو* _mem)
{
#إذا d_m3_Skip_ڍير_Check
    (فراغ)i_function;
    (فراغ)_sp;
    (فراغ)_mem;
    أرجع true;
#وإلاإذا d_m3_Has_ڍير_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->سرگرم_Continuation : عدم);
    فراغ*           max_ڍير = (_act && _act->val_ڍير)
                                 ? (فراغ*)(_act->val_ڍير + _act->انگ_ڍير_Slots)
                                 : (_mem ? _mem->max_ڍير : (فراغ*)~(uintptr_t)0);
    أرجع M3_LIKELY((فراغ*)(_sp + i_function->max_ڍير_Slots) < max_ڍير);
#وإلا
    أرجع M3_LIKELY((فراغ*)(_sp + i_function->max_ڍير_Slots) < _mem->max_ڍير);
#جيڪڏهن_پورو
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
ساكن مضمن
فراغ Enter_چوکٺو (IM3Function i_function, m3stack_t _sp)
{
#إذا defined(DEBUG)
    i_function->ڪاميابيون++;
#جيڪڏهن_پورو
    u8* ڍير = (u8*)((m3slot_t*)_sp + i_function->انگ_Ret_And_Arg_Slots);

    memset(ڍير, 0x0, i_function->انگ_Local_بائيٽون);
    ڍير += i_function->انگ_Local_بائيٽون;

    إذا (i_function->constants) {
        يادگيري_نقل(ڍير, i_function->constants, i_function->انگ_Constant_بائيٽون);
    }
}


d_m3Op(داخلا_2)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = فوري(IM3Function);
#إذا d_m3_داخلا_Keeps_چوکٺو
    IM3_يادگيري يادگيري = m3MemInfo(_mem);
#جيڪڏهن_پورو

    إذا (داخلا_سمائي(function, _sp, _mem)) {
        Enter_چوکٺو(function, _sp);

#إذا !d_m3_داخلا_Keeps_چوکٺو
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        ايندڙ_Op_سڌو();
#وإلا

#  إذا d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->انگ_Ret_Slots));
        trace_rt->call_اونهائي++;
#  endif

        m3ret_t r = ايندڙ_Op_Impl();

#  إذا d_m3EnableStrace >= 2
        trace_rt->call_اونهائي--;

        إذا (r) {
            d_m3TracePrint("} !trap = %s", (حرف*)r);
        } وإلا إذا (Get_Function_انگ_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } وإلا {
            d_m3TracePrint("}");
        }
#  endif

        إذا (M3_UNLIKELY(r)) {
            _mem = يادگيري->mallocated;
#  إذا d_m3_Has_ڍير_Switching
            إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                أرجع رڪارڊ_داخلا_چوکٺو(m3MemRuntime(_mem), _pc, _sp, يادگيري, function);
            }
#  endif
            fill_Backtrace_چوکٺو();
        }
        forwardTrap(r);
#جيڪڏهن_پورو // d_m3EntryKeepsFrame
    } وإلا {
        newTrap(m3_Err_trap_ڍير_حد_کان_مٿي);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_ڍير();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_يادگيري يادگيري = m3MemInfo(_mem);

    افعل {
#إذا d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_اونهائي++;
#جيڪڏهن_پورو
        r = ايندڙ_Op_Impl();

#إذا d_m3EnableStrace >= 3
        trace_rt->call_اونهائي--;
        d_m3TracePrint("}");
#جيڪڏهن_پورو
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = يادگيري->mallocated;
    } طالما (r == _pc);

#إذا d_m3_Has_ڍير_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        أرجع رڪارڊ_Loop_چوکٺو(m3MemRuntime(_mem), _pc, _sp, يادگيري);
    }
#جيڪڏهن_پورو

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
    d_m3_Check_Native_ڍير();

    IM3_يادگيري  يادگيري = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        انگ_Clauses = فوري(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * انگ_Clauses;

    u32        اونهائي = ++runtime->try_اونهائي;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = يادگيري->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_اونهائي == اونهائي);
    runtime->try_اونهائي = اونهائي - 1;

#  إذا d_m3_Has_ڍير_Switching
    إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        أرجع رڪارڊ_Try_چوکٺو(runtime, clauses, _sp, يادگيري, انگ_Clauses, isInnermost);
    }
#  endif

    إذا (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ثابت M3Exception* exception = runtime->pendingException;

        لكل (u32 i = 0; i < انگ_Clauses; ++i) {
            IM3_ٽيگ ٽيگ = (IM3_ٽيگ)clauses[2 * i];

            إذا (ٽيگ == عدم or ٽيگ == exception->ٽيگ) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jump_Op_سڌو(handler);
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

    u32          انگ_دليلون = فوري(u32);

    لكل (u32 i = 0; i < انگ_دليلون; ++i) {
        i32 offset = فوري(i32);
        u32 is64 = فوري(u32);
        u64 قدر = exception->دليلون[i];

        إذا (is64) {
            *(u64*)(_sp + offset) = قدر;
        } وإلا {
            *(u32*)(_sp + offset) = (u32)قدر;
        }
    }

    i32 ref_وٿي = فوري(i32);

    إذا (ref_وٿي >= 0) {
        *(M3Exception**)(_sp + ref_وٿي) = exception;
        exception->reified = true;
    } وإلا إذا (not exception->reified) {
        FreeException(runtime, exception);
    }

    ايندڙ_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_ٽيگ       ٽيگ = فوري(IM3_ٽيگ);
    u32          انگ_دليلون = فوري(u32);

    M3Exception* exception = NewException(runtime, ٽيگ, انگ_دليلون);

    إذا (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    لكل (u32 i = 0; i < انگ_دليلون; ++i) {
        i32 offset = فوري(i32);
        u32 is64 = فوري(u32);

        exception->دليلون[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    أرجع m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = جڳهه(M3Exception*);

    إذا (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    أرجع m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(ڪڍو_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        ڳڻپ = فوري(u32);

    runtime->try_اونهائي -= ڳڻپ;

    ايندڙ_Op();
}

#جيڪڏهن_پورو // d_m3HasExceptionHandling

#إذا d_m3_Has_ڍير_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = فوري(IM3FuncType);
    i32         dst_جڳهه = فوري(i32);
    i32         func_جڳهه = فوري(i32);

    IM3Function func = *(IM3Function*)(_sp + func_جڳهه);
    إذا (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    إذا (M3_UNLIKELY(not Continuation_Can_Acquire_ڍير(runtime))) {
        newTrap(m3_Err_continuation_حد_بندي_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_نتيجو ورهاست = Continuation_Acquire_ڍير(runtime, cont);
    إذا (M3_UNLIKELY(ورهاست)) {
        cont->state = cont_consumed;
        newTrap(ورهاست);
    }
    *(IM3Continuation*)(_sp + dst_جڳهه) = cont;
    ايندڙ_Op();
}


ساكن
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = فوري(IM3FuncType);
    IM3FuncType     ct2 = فوري(IM3FuncType);
    i32             dst_جڳهه = فوري(i32);
    i32             cont_جڳهه = فوري(i32);
    u32             انگ_Bound = فوري(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + cont_جڳهه);
    إذا (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->داخلا_Function);
    إذا (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->داخلا_Function = srcCont->داخلا_Function;
    dstCont->val_ڍير = srcCont->val_ڍير;
    dstCont->انگ_ڍير_Slots = srcCont->انگ_ڍير_Slots;
    dstCont->چوکٺا = srcCont->چوکٺا;
    dstCont->چوکٺا_Cap = srcCont->چوکٺا_Cap;
    dstCont->انگ_چوکٺا = srcCont->انگ_چوکٺا;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspend_نقطو = srcCont->suspend_نقطو;
    dstCont->r0 = srcCont->r0;
#  إذا d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_دليلون_ڳڻپ = srcCont->bound_دليلون_ڳڻپ;
    dstCont->state = srcCont->state;
#  إذا d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = عدم;
#  endif

    dstCont->انگ_Suspend_Results = srcCont->انگ_Suspend_Results;
    لكل (u32 j = 0; j < srcCont->انگ_Suspend_Results; ++j) {
        dstCont->suspend_نتيجو_Offsets[j] = srcCont->suspend_نتيجو_Offsets[j];
        dstCont->suspend_نتيجو_Is64[j] = srcCont->suspend_نتيجو_Is64[j];
    }

    إذا (dstCont->state == cont_allocated) {
        u16 انگ_Rets = ct1->contFuncType ? Get_Func_Type_انگ_Results(ct1->contFuncType) : 0;
        لكل (u32 i = 0; i < انگ_Bound; ++i) {
            i32 arg_جڳهه = فوري(i32);
            u32 is64 = فوري(u32);
            u32 هدف_Idx = (انگ_Rets + dstCont->bound_دليلون_ڳڻپ) * c_io_جڳهه_ڳڻپ;
            إذا (is64) {
                *(u64*)(dstCont->val_ڍير + هدف_Idx) = *(u64*)(_sp + arg_جڳهه);
            } وإلا {
                *(u32*)(dstCont->val_ڍير + هدف_Idx) = *(u32*)(_sp + arg_جڳهه);
            }
            dstCont->bound_دليلون_ڳڻپ++;
        }
    } وإلا {
        IM3Continuation inner = InnermostSuspended(dstCont);
        لكل (u32 i = 0; i < انگ_Bound; ++i) {
            i32 arg_جڳهه = فوري(i32);
            u32 is64 = فوري(u32);
            u32 dstIdx = dstCont->bound_دليلون_ڳڻپ;
            إذا (dstIdx < inner->انگ_Suspend_Results) {
                i32 dst_وٿي = inner->suspend_نتيجو_Offsets[dstIdx];
                إذا (is64) {
                    *(u64*)(inner->sp + dst_وٿي) = *(u64*)(_sp + arg_جڳهه);
                } وإلا {
                    *(u32*)(inner->sp + dst_وٿي) = *(u32*)(_sp + arg_جڳهه);
                }
            }
            dstCont->bound_دليلون_ڳڻپ++;
        }
    }

    srcCont->val_ڍير = عدم;
    srcCont->چوکٺا = عدم;
    srcCont->چوکٺا_Cap = 0;
    srcCont->انگ_چوکٺا = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dst_جڳهه) = dstCont;
    ايندڙ_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
ساكن
IM3Continuation ڳوليو_Handler (IM3Continuation i_cont, IM3_ٽيگ i_ٽيگ, u8 i_قسم, pc_t* o_stubPC)
{
    IM3_ٽيگ query_ٽيگ = (i_ٽيگ and i_ٽيگ->resolved) ? i_ٽيگ->resolved : i_ٽيگ;

    لكل (IM3Continuation cont = i_cont; cont; cont = cont->والد) {
        لكل (u32 h = 0; h < cont->انگ_Handlers; ++h) {
            pc_t   داخلا = cont->handlersPC + (h * 3);
            IM3_ٽيگ handler_ٽيگ = (IM3_ٽيگ)(*(داخلا + 1));
            إذا (handler_ٽيگ and handler_ٽيگ->resolved) {
                handler_ٽيگ = handler_ٽيگ->resolved;
            }

            إذا ((u32)(uintptr_t)(*داخلا) == i_قسم and handler_ٽيگ == query_ٽيگ) {
                إذا (o_stubPC) {
                    *o_stubPC = (pc_t)(*(داخلا + 2));
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
    لكل (u32 i = 0; i < io_cont->انگ_Suspend_Results; ++i) {
        إذا (io_cont->suspend_نتيجو_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_نتيجو_Offsets[i]) = 0;
        } وإلا {
            *(m3slot_t*)(i_sp + io_cont->suspend_نتيجو_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->سرگرم_Continuation;
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ٽيگ);
    }

    IM3_ٽيگ ٽيگ = فوري(IM3_ٽيگ);
    u32    انگ_Params = فوري(u32);

    runtime->انگ_Suspend_Payload = انگ_Params;
    لكل (u32 i = 0; i < انگ_Params; ++i) {
        i32 param_جڳهه = فوري(i32);
        u32 is64 = فوري(u32);
        إذا (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + param_جڳهه);
        } وإلا {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + param_جڳهه);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 انگ_Results = فوري(u32);
    cont->انگ_Suspend_Results = انگ_Results;
    لكل (u32 i = 0; i < انگ_Results; ++i) {
        cont->suspend_نتيجو_Offsets[i] = فوري(i32);
        cont->suspend_نتيجو_Is64[i] = فوري(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = عدم;
    IM3Continuation ڳولا_Cont = ڳوليو_Handler(cont, ٽيگ, 0, &matchedStubPC);

    إذا (M3_UNLIKELY(not ڳولا_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ٽيگ);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = ڳولا_Cont;
    runtime->switch_هدف = عدم;
    runtime->suspend_ٽيگ = ٽيگ;

    cont->pc = _pc;
    cont->suspend_نقطو = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  إذا d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_دليلون_ڳڻپ = 0;

    أرجع m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
ساكن
m3ret_t لاڳيتو_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_انگ_Handlers, M3_يادگيري_سرو** io_mem);
ساكن
فراغ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_انگ_Results);
ساكن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_يادگيري_سرو* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
ساكن مضمن
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    أرجع pass and (i_cont->suspend_نقطو == safepoint_op or i_cont->suspend_نقطو == safepoint_داخلا);
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
m3ret_t Replay_چوکٺا (IM3Continuation i_cont, i32 i_اونهائي, M3_يادگيري_سرو* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  إذا d_m3_Max_Native_ڍير > 0
    {
        فراغ* حد_بندي = runtime->ڍير_حد_بندي;

        إذا (M3_UNLIKELY(حد_بندي and m3_Native_ڍير_Ptr() < حد_بندي)) {
            أرجع m3_Err_trap_ڍير_حد_کان_مٿي;
        }
    }
#  endif

    إذا (i_اونهائي < 0) {
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

    M3_چوکٺو چوکٺو = i_cont->چوکٺا[i_اونهائي];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_چوکٺو_قسم)چوکٺو.قسم == چوکٺو_resume);

#  إذا d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_چوکٺو_قسم)چوکٺو.قسم == چوکٺو_try);
#  endif

    إذا (not ownDescent) {
        r = Replay_چوکٺا(i_cont, i_اونهائي - 1, _mem);
        _mem = چوکٺو.يادگيري->mallocated;
    }

    بدل ((M3_چوکٺو_قسم)چوکٺو.قسم) {
    حالة چوکٺو_call:
        إذا (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            أرجع d_m3CallWithRegs(چوکٺو.pc, چوکٺو.sp, _mem, چوکٺو.call.r0, چوکٺو.call.fp0);
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            أرجع Continuation_رڪارڊ_چوکٺو(runtime, &چوکٺو);
        }
#  إذا d_m3_رڪارڊ_Backtraces
        ڌڪو_Backtrace_چوکٺو(runtime, چوکٺو.pc - 1);
#  endif
        أرجع r;

    حالة چوکٺو_loop: {
        m3stack_t _sp = چوکٺو.sp;
        m3reg_t   _r0 = 0;
#  إذا d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        طالما (r == (m3ret_t)چوکٺو.pc) {
            r = jumpOpImpl(چوکٺو.pc);
            _mem = چوکٺو.يادگيري->mallocated;
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            أرجع Continuation_رڪارڊ_چوکٺو(runtime, &چوکٺو);
        }
        أرجع r;
    }

#  إذا d_m3HasExceptionHandling
    حالة چوکٺو_try: {
        m3stack_t _sp = چوکٺو.sp;
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
        u32 اونهائي = ++runtime->try_اونهائي;

        إذا (not چوکٺو.try_.handlers_جيئرو) {
            runtime->try_اونهائي = اونهائي - 1;
        }

        r = Replay_چوکٺا(i_cont, i_اونهائي - 1, _mem);
        _mem = چوکٺو.يادگيري->mallocated;

        bool isInnermost = (runtime->try_اونهائي == اونهائي);
        runtime->try_اونهائي = اونهائي - 1;

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            چوکٺو.try_.handlers_جيئرو = isInnermost;
            أرجع Continuation_رڪارڊ_چوکٺو(runtime, &چوکٺو);
        }

        إذا (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ثابت M3Exception* exception = runtime->pendingException;

            لكل (u32 i = 0; i < چوکٺو.try_.انگ_Clauses; ++i) {
                IM3_ٽيگ ٽيگ = (IM3_ٽيگ)چوکٺو.pc[2 * i];

                إذا (ٽيگ == عدم or ٽيگ == exception->ٽيگ) {
                    pc_t handler = (pc_t)چوکٺو.pc[2 * i + 1];

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

    حالة چوکٺو_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = چوکٺو.resume.cont;

        r = لاڳيتو_Under_Handlers(runtime, &inner, چوکٺو.resume.handlersPC,
                             چوکٺو.resume.انگ_Handlers, &_mem);

        إذا (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, چوکٺو.sp, چوکٺو.resume.resultsPC,
                                    چوکٺو.resume.انگ_Results);
            Continuation_Recycle_ڍير(runtime, inner);

            أرجع d_m3CallWithRegs(چوکٺو.pc, چوکٺو.sp, _mem, 0, 0.);
        }

        إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            إذا (runtime->suspendHandlerCont != inner) {
                چوکٺو.resume.cont = inner;
                أرجع Continuation_رڪارڊ_چوکٺو(runtime, &چوکٺو);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            إذا (M3_UNLIKELY(not suspended)) {
                أرجع m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            إذا (runtime->suspendStubPC) {
                أرجع d_m3CallWithRegs(runtime->suspendStubPC, چوکٺو.sp, _mem, 0, 0.);
            } وإلا {
                أرجع m3_Err_trap_Unhandled_Control_ٽيگ;
            }
        } وإلا {
            inner->state = cont_consumed;
            Continuation_Recycle_ڍير(runtime, inner);
        }

        أرجع r;
    }

#  إذا d_m3_داخلا_Keeps_چوکٺو
    حالة چوکٺو_داخلا:
        إذا (M3_UNLIKELY(r)) {
            إذا (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                أرجع Continuation_رڪارڊ_چوکٺو(runtime, &چوکٺو);
            }
#    إذا d_m3_رڪارڊ_Backtraces
            FillBacktraceFunctionInfo(runtime, چوکٺو.داخلا.function);
#    وإلا
            (فراغ)چوکٺو.داخلا.function;
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
فراغ* ContinuationArgDest (IM3Continuation i_cont, u32 i_اشاريو)
{
    إذا (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : عدم;
        u16         انگ_Rets = inner ? Get_Func_Type_انگ_Results(inner) : 0;

        أرجع i_cont->val_ڍير + (انگ_Rets + i_اشاريو) * c_io_جڳهه_ڳڻپ;
    }

    إذا (i_اشاريو < i_cont->انگ_Suspend_Results) {
        أرجع i_cont->sp + i_cont->suspend_نتيجو_Offsets[i_اشاريو];
    }

    أرجع عدم;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
ساكن
فراغ Hand_Over_منزل_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 انگ_دليلون = i_runtime->انگ_Suspend_Payload;
    u32 بنياد = (io_peer->state == cont_allocated) ? io_peer->bound_دليلون_ڳڻپ : 0;

    لكل (u32 i = 0; i < انگ_دليلون; ++i) {
        فراغ* dest = ContinuationArgDest(io_peer, بنياد + i);

        إذا (dest) {
            إذا (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } وإلا {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    فراغ* refDest = ContinuationArgDest(io_peer, بنياد + انگ_دليلون);

    إذا (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    إذا (io_peer->state == cont_allocated) {
        io_peer->bound_دليلون_ڳڻپ += انگ_دليلون + 1;
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
    طالما (i_cont->انگ_چوکٺا and (M3_چوکٺو_قسم) i_cont->چوکٺا[0].قسم == چوکٺو_resume) {
        i_cont = i_cont->چوکٺا[0].resume.cont;
    }

    أرجع i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
ساكن
m3ret_t لاڳيتو_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_انگ_Handlers, M3_يادگيري_سرو** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation والد = i_runtime->سرگرم_Continuation;
    IM3_يادگيري       يادگيري = m3MemInfo(*io_mem);
    m3ret_t         r;

    لكل (;;) {
#  إذا d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        إذا (M3_UNLIKELY(cont->resumeThrow and cont->انگ_چوکٺا == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = عدم;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            اكسر;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        إذا (isStarting) {
            إذا (M3_UNLIKELY(not cont->داخلا_Function->compiled)) {
                r = CompileFunction(cont->داخلا_Function);

                إذا (M3_UNLIKELY(r)) {
                    اكسر;
                }
            }
            cont->pc = cont->داخلا_Function->compiled;
            cont->sp = cont->val_ڍير;
            cont->انگ_چوکٺا = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->انگ_Handlers = i_انگ_Handlers;
        cont->والد = والد;
        cont->state = cont_running;
        cont->bound_دليلون_ڳڻپ = 0;

        i_runtime->سرگرم_Continuation = cont;

        إذا (cont->انگ_چوکٺا) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->انگ_چوکٺا - 1;
            cont->انگ_چوکٺا = 0;

            r = Replay_چوکٺا(cont, outermost, *io_mem);
        } وإلا إذا (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } وإلا {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = يادگيري->mallocated;
        i_runtime->سرگرم_Continuation = والد;

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
        suspended->type = Module_Cont_Type_جو_Ref(peer->داخلا_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_انگ_Params(peerType) - 1)));
        Hand_Over_منزل_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    أرجع r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
ساكن
فراغ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_انگ_Results)
{
    لكل (u32 i = 0; i < i_انگ_Results; ++i) {
        i32 res_جڳهه = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_جڳهه_ڳڻپ;

        إذا (is64) {
            *(u64*)(i_sp + res_جڳهه) = *(u64*)(i_cont->val_ڍير + srcIdx);
        } وإلا {
            *(u32*)(i_sp + res_جڳهه) = *(u32*)(i_cont->val_ڍير + srcIdx);
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
    IM3FuncType     contFuncType = فوري(IM3FuncType);
    i32             cont_جڳهه = فوري(i32);
    u32             گهڻائي = فوري(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + cont_جڳهه);
    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         انگ_Rets = Get_Func_Type_انگ_Results(innerType);

    إذا (گهڻائي == d_m3ResumeThrowRef) {
        i32 exn_جڳهه = فوري(i32);

#  إذا d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exn_جڳهه);

        إذا (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  وإلا
        (فراغ)exn_جڳهه;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } وإلا إذا (گهڻائي == d_m3ResumeThrow) {
        IM3_ٽيگ exn_ٽيگ = فوري(IM3_ٽيگ);
        u32    انگ_دليلون = فوري(u32);

#  إذا d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_ٽيگ, انگ_دليلون);

        إذا (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        لكل (u32 i = 0; i < انگ_دليلون; ++i) {
            i32 offset = فوري(i32);
            u32 is64 = فوري(u32);

            exception->دليلون[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  وإلا
        (فراغ)exn_ٽيگ;
        (فراغ)انگ_دليلون;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } وإلا {
        u32 انگ_دليلون = فوري(u32);

        إذا (cont->state == cont_allocated) {
            لكل (u32 i = 0; i < انگ_دليلون; ++i) {
                i32 arg_جڳهه = فوري(i32);
                u32 is64 = فوري(u32);
                u32 هدف_Idx = (انگ_Rets + cont->bound_دليلون_ڳڻپ + i) * c_io_جڳهه_ڳڻپ;
                إذا (is64) {
                    *(u64*)(cont->val_ڍير + هدف_Idx) = *(u64*)(_sp + arg_جڳهه);
                } وإلا {
                    *(u32*)(cont->val_ڍير + هدف_Idx) = *(u32*)(_sp + arg_جڳهه);
                }
            }
            cont->bound_دليلون_ڳڻپ = 0;
        } وإلا {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            لكل (u32 i = 0; i < انگ_دليلون; ++i) {
                i32 arg_جڳهه = فوري(i32);
                u32 is64 = فوري(u32);
                u32 dstIdx = cont->bound_دليلون_ڳڻپ + i;
                إذا (dstIdx < inner->انگ_Suspend_Results) {
                    i32 dst_وٿي = inner->suspend_نتيجو_Offsets[dstIdx];
                    إذا (is64) {
                        *(u64*)(inner->sp + dst_وٿي) = *(u64*)(_sp + arg_جڳهه);
                    } وإلا {
                        *(u32*)(inner->sp + dst_وٿي) = *(u32*)(_sp + arg_جڳهه);
                    }
                }
            }
        }
    }

    u32  انگ_Handlers = فوري(u32);
    pc_t handlersPC = _pc;
    _pc += انگ_Handlers * 3;

    u32       انگ_Results = فوري(u32);
    pc_t      resultsPC = _pc;

    IM3_يادگيري يادگيري = m3MemInfo(_mem);

    m3ret_t   r = لاڳيتو_Under_Handlers(runtime, &cont, handlersPC, انگ_Handlers, &_mem);

    إذا (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, انگ_Results);
        Continuation_Recycle_ڍير(runtime, cont);

        _pc += انگ_Results * 2;
        ايندڙ_Op();
    } وإلا إذا (r == m3Err_continuationSuspended) {
        إذا (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            إذا (M3_UNLIKELY(not runtime->سرگرم_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_ٽيگ);
            }
            M3_چوکٺو چوکٺو;
            چوکٺو.قسم = چوکٺو_resume;
            چوکٺو.pc = resultsPC + انگ_Results * 2;
            چوکٺو.sp = _sp;
            چوکٺو.يادگيري = يادگيري;
            چوکٺو.resume.cont = cont;
            چوکٺو.resume.handlersPC = handlersPC;
            چوکٺو.resume.انگ_Handlers = انگ_Handlers;
            چوکٺو.resume.resultsPC = resultsPC;
            چوکٺو.resume.انگ_Results = انگ_Results;

            أرجع Continuation_رڪارڊ_چوکٺو(runtime, &چوکٺو);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        إذا (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        إذا (runtime->suspendStubPC) {
            jump_Op_سڌو(runtime->suspendStubPC);
        } وإلا {
            newTrap(m3_Err_trap_Unhandled_Control_ٽيگ);
        }
    } وإلا {
        cont->state = cont_consumed;
        Continuation_Recycle_ڍير(runtime, cont);
        ڌڪو_Backtrace_چوکٺو_2();
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
    cont->type = فوري(IM3FuncType);

    u32 انگ_دليلون = فوري(u32);
    لكل (u32 i = 0; i < انگ_دليلون; ++i) {
        i32 offset = فوري(i32);
        u32 is64 = فوري(u32);
        u64 val = (i < runtime->انگ_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        إذا (is64) {
            *(u64*)(_sp + offset) = val;
        } وإلا {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_وٿي = فوري(i32);
    إذا (cont_وٿي >= 0) {
        *(IM3Continuation*)(_sp + cont_وٿي) = cont;
    }

    ايندڙ_Op();
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
    IM3Continuation cont = runtime->سرگرم_Continuation;

    إذا (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ٽيگ);
    }

    skip_فوري(IM3FuncType);

    IM3_ٽيگ          ٽيگ = فوري(IM3_ٽيگ);
    i32             هدف_جڳهه = فوري(i32);

    IM3Continuation هدف = *(IM3Continuation*)(_sp + هدف_جڳهه);

    إذا (M3_UNLIKELY(not هدف)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    إذا (M3_UNLIKELY(هدف->state == cont_consumed or هدف->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = ڳوليو_Handler(cont, ٽيگ, 1, عدم);

    إذا (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ٽيگ);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 انگ_دليلون = فوري(u32);

    runtime->انگ_Suspend_Payload = انگ_دليلون;

    لكل (u32 i = 0; i < انگ_دليلون; ++i) {
        i32 arg_جڳهه = فوري(i32);
        u32 is64 = فوري(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + arg_جڳهه) : (u64) * (u32*)(_sp + arg_جڳهه);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 انگ_Results = فوري(u32);

    cont->انگ_Suspend_Results = انگ_Results;

    لكل (u32 i = 0; i < انگ_Results; ++i) {
        cont->suspend_نتيجو_Offsets[i] = فوري(i32);
        cont->suspend_نتيجو_Is64[i] = فوري(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = عدم;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switch_هدف = هدف;
    runtime->suspend_ٽيگ = ٽيگ;

    cont->pc = _pc;
    cont->suspend_نقطو = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  إذا d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    أرجع m3Err_continuationSuspended;
}


#جيڪڏهن_پورو // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = فوري(pc_t);

    إذا (condition) {
        ايندڙ_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = جڳهه(i32);

    pc_t elsePC = فوري(pc_t);

    إذا (condition) {
        ايندڙ_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_جدول)
{
    u32   branch_اشاريو = جڳهه(u32);           // branch index is always in a slot
    u32   انگ_Targets = فوري(u32);

    pc_t* branches = (pc_t*)_pc;

    إذا (branch_اشاريو > انگ_Targets) {
        branch_اشاريو = انگ_Targets; // the default index
    }

    jumpOp(branches[branch_اشاريو]);
}


#وصف_ڪريو d_m3_سيٽ_رجسٽر_سيٽ_جڳهه(TYPE, REG) \
d_m3Op  (سيٽ_رجسٽر_2##TYPE)            \
{                                       \
    TYPE * src = جڳهه_ptr (TYPE);       \
    d_m3_Preload_ايندڙ ();                 \
    REG = * src;                        \
    ايندڙ_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (سيٽ_جڳهه_2##TYPE)                 \
{                                       \
    TYPE * dst = جڳهه_ptr (TYPE);       \
    d_m3_Preload_ايندڙ ();                 \
    * dst = (TYPE) REG;                 \
    ايندڙ_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_سيٽ_جڳهه_2##TYPE)         \
{                                       \
    TYPE * ڍير     = جڳهه_ptr (TYPE); \
    TYPE * preserve  = جڳهه_ptr (TYPE); \
    d_m3_Preload_ايندڙ ();                 \
                                        \
    * preserve = * ڍير;               \
    * ڍير = (TYPE) REG;               \
                                        \
    ايندڙ_Op_Preloaded ();                 \
}

d_m3_سيٽ_رجسٽر_سيٽ_جڳهه(i32, _r0)
d_m3_سيٽ_رجسٽر_سيٽ_جڳهه(i64, _r0)
#إذا d_m3HasFloat
d_m3_سيٽ_رجسٽر_سيٽ_جڳهه(f32, _fp0)
d_m3_سيٽ_رجسٽر_سيٽ_جڳهه(f64, _fp0)
#جيڪڏهن_پورو

d_m3Op(Copy_جڳهه_32)
{
    u32* dst = جڳهه_ptr(u32);
    u32* src = جڳهه_ptr(u32);
    d_m3_Preload_ايندڙ();

    *dst = *src;

    ايندڙ_Op_Preloaded();
}


d_m3Op(Preserve_Copy_جڳهه_32)
{
    u32* dest = جڳهه_ptr(u32);
    u32* src = جڳهه_ptr(u32);
    u32* preserve = جڳهه_ptr(u32);
    d_m3_Preload_ايندڙ();

    *preserve = *dest;
    *dest = *src;

    ايندڙ_Op_Preloaded();
}


d_m3Op(Copy_جڳهه_64)
{
    u64* dst = جڳهه_ptr(u64);
    u64* src = جڳهه_ptr(u64);
    d_m3_Preload_ايندڙ();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    ايندڙ_Op_Preloaded();
}


d_m3Op(Preserve_Copy_جڳهه_64)
{
    u64* dest = جڳهه_ptr(u64);
    u64* src = جڳهه_ptr(u64);
    u64* preserve = جڳهه_ptr(u64);
    d_m3_Preload_ايندڙ();

    *preserve = *dest;
    *dest = *src;

    ايندڙ_Op_Preloaded();
}


#إذا d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_ڍير)
{
    u32         opcode_اشاريو = فوري(u32);
    u32         ڍير_اوچائي = فوري(u32);
    IM3Function function = فوري(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    لکو(" %4d ", opcode_اشاريو);
    لکو(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  إذا d_m3HasFloat
    لکو("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    لكل (u32 i = 0; i < ڍير_اوچائي; ++i) {
        cstr_t قسم = "";

        لکو("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, قسم, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    لکو("---------------------------------------------------------------------------------------------------------\n");

    ايندڙ_Op_سڌو();
}
#جيڪڏهن_پورو


#وصف_ڪريو d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = جڳهه (TYPE);                \
    TYPE operand1 = جڳهه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ايندڙ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = جڳهه (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = جڳهه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ايندڙ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = جڳهه (i32);                 \
                                                \
    TYPE operand2 = جڳهه (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ايندڙ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = جڳهه (i32);                 \
                                                \
    TYPE operand2 = جڳهه (TYPE);                \
    TYPE operand1 = جڳهه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ايندڙ_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#وصف_ڪريو d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = جڳهه (TYPE);                \
    TYPE operand1 = جڳهه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ايندڙ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = جڳهه (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ايندڙ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = جڳهه (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ايندڙ_Op ();                                  \
}

#إذا d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, جڳهه(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, جڳهه(i32))
#جيڪڏهن_پورو

d_m3Op(Return)
{
    m3_ڍير_Check();
    أرجع m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = فوري(pc_t);

    إذا (condition) {
        jumpOp(branch);
    } وإلا {
        ايندڙ_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = جڳهه(i32);
    pc_t branch = فوري(pc_t);

    إذا (condition) {
        jumpOp(branch);
    } وإلا {
        ايندڙ_Op();
    }
}

#إذا d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE عمليو = جڳهه (TYPE);                             \
      pc_t branch  = فوري (pc_t);                        \
      إذا (عمليو OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      وإلا ايندڙ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE عمليو = جڳهه (TYPE);                             \
      pc_t branch  = فوري (pc_t);                        \
      إذا (((TYPE) _r0) OPER عمليو)                          \
          jumpOp (branch);                                    \
      وإلا ايندڙ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = جڳهه (TYPE);                            \
      TYPE operand1 = جڳهه (TYPE);                            \
      pc_t branch   = فوري (pc_t);                       \
      إذا (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      وإلا ايندڙ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE عمليو = جڳهه (TYPE);                             \
      pc_t elsePC  = فوري (pc_t);                        \
      إذا (عمليو OPER ((TYPE) _r0))                          \
          ايندڙ_Op ();                                          \
      وإلا jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE عمليو = جڳهه (TYPE);                             \
      pc_t elsePC  = فوري (pc_t);                        \
      إذا (((TYPE) _r0) OPER عمليو)                          \
          ايندڙ_Op ();                                          \
      وإلا jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = جڳهه (TYPE);                            \
      TYPE operand1 = جڳهه (TYPE);                            \
      pc_t elsePC   = فوري (pc_t);                       \
      إذا (operand1 OPER operand2)                             \
          ايندڙ_Op ();                                          \
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
    pc_t branch = فوري(pc_t);
    إذا (((i32)_r0) == 0) {
        jumpOp(branch);
    } وإلا {
        ايندڙ_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  عمليو = جڳهه(i32);
    pc_t branch = فوري(pc_t);
    إذا (عمليو == 0) {
        jumpOp(branch);
    } وإلا {
        ايندڙ_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = فوري(pc_t);
    إذا (((i32)_r0) == 0) {
        ايندڙ_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  عمليو = جڳهه(i32);
    pc_t elsePC = فوري(pc_t);
    إذا (عمليو == 0) {
        ايندڙ_Op();
    } وإلا {
        jumpOp(elsePC);
    }
}

#جيڪڏهن_پورو // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = فوري(pc_t);

    إذا (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        ايندڙ_Op();
    } وإلا {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = جڳهه(i32);
    pc_t branch = فوري(pc_t);

    إذا (condition) {
        ايندڙ_Op();
    } وإلا {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_ڍير_Check();

    فراغ* loopId = فوري(فراغ*);
    أرجع loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    فراغ* loopId = فوري(فراغ*);

    إذا (condition) {
        أرجع loopId;
    } وإلا {
        ايندڙ_Op();
    }
}


#إذا d_m3_Has_ڍير_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_ڍير_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->سرگرم_Continuation)) {
        Suspend_Without_ٽيگ(runtime, runtime->سرگرم_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_دليلون(_r0, _fp0));

        أرجع m3Err_continuationSuspended;
    }

    فراغ* loopId = فوري(فراغ*);
    أرجع loopId;
}


#  إذا !d_m3_داخلا_Keeps_چوکٺو
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(داخلا_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = فوري(IM3Function);

    إذا (داخلا_سمائي(function, _sp, _mem)) {
        Enter_چوکٺو(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->سرگرم_Continuation)) {
            Suspend_Without_ٽيگ(runtime, runtime->سرگرم_Continuation, safepoint_داخلا, _pc, _sp,
                              d_m3_Exp_Reg_دليلون(_r0, _fp0));

            أرجع m3Err_continuationSuspended;
        }

        ايندڙ_Op_سڌو();
    } وإلا {
        newTrap(m3_Err_trap_ڍير_حد_کان_مٿي);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(داخلا_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->سرگرم_Continuation)) {
        Suspend_Without_ٽيگ(runtime, runtime->سرگرم_Continuation, safepoint_داخلا, _pc - 1, _sp,
                          d_m3_Exp_Reg_دليلون(_r0, _fp0));

        أرجع m3Err_continuationSuspended;
    }

    ايندڙ_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    فراغ* loopId = فوري(فراغ*);

    إذا (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        إذا (M3_UNLIKELY(runtime->suspendRequested and runtime->سرگرم_Continuation)) {
            Suspend_Without_ٽيگ(runtime, runtime->سرگرم_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_دليلون(_r0, _fp0));

            أرجع m3Err_continuationSuspended;
        }
        أرجع loopId;
    } وإلا {
        ايندڙ_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
ساكن
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_يادگيري_سرو* _mem)
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
    } وإلا إذا (op == op_داخلا_Check) {
        أرجع ايندڙ_Op_Impl();
    } وإلا {
        أرجع d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  إذا (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    أرجع op(_pc, d_m3_Op_دليلون, d_m3_بنياد_Cstr);
#  وإلا
    أرجع op(_pc, d_m3_Op_دليلون);
#  endif
}
#جيڪڏهن_پورو // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 قدر = *(u32*)_pc++;
    جڳهه(u32) = قدر;
    ايندڙ_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 قدر;
    يادگيري_نقل(&قدر, _pc, حجم(قدر));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    جڳهه(u64) = قدر;
    ايندڙ_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_ڍير_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(پڇاڙي_2)
{
    m3_ڍير_Check();
    أرجع m3Err_none;
}


d_m3Op(سيٽ_Global_s32)
{
    u32* global = فوري(u32*);
    *global = جڳهه(u32);

    ايندڙ_Op();
}


d_m3Op(سيٽ_Global_s64)
{
    u64* global = فوري(u64*);
    *global = جڳهه(u64);

    ايندڙ_Op();
}

#إذا d_m3HasFloat
d_m3Op(سيٽ_Global_f32)
{
    f32* global = فوري(f32*);
    *global = (f32)_fp0;

    ايندڙ_Op();
}


d_m3Op(سيٽ_Global_f64)
{
    f64* global = فوري(f64*);
    *global = _fp0;

    ايندڙ_Op();
}
#جيڪڏهن_پورو


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
    u64  عمليو = جڳهه(u64);
    u32* پتو = جڳهه_ptr(u32);

    u64  offset;
    يادگيري_نقل(&offset, _pc, حجم(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    إذا (M3_LIKELY(عمليو < d_m3_پتو_حد_بندي)) {
        u64 effective = عمليو + offset;

        // the access itself checks its own size against what is left
        إذا (M3_LIKELY(effective < _mem->ڊيگهائي)) {
            *پتو = (u32)effective;
            ايندڙ_Op();
        }
    }

    d_ٻاهريون_جو_Bounds;
}

#جيڪڏهن_پورو // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#إذا d_m3_Skip_يادگيري_Bounds_Check || d_m3_Guarded_يادگيري
#  define m3MemCheck(x) true
#وإلا
#  define m3MemCheck(x) M3_LIKELY(x)
#جيڪڏهن_پورو

// memcpy here is to support non-aligned access on some platforms.

#وصف_ڪريو d_m3_ڀريو(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = فوري (u32);                       \
    d_m3_Preload_ايندڙ ();                                 \
    u64 عمليو = (u32) _r0;                            \
    عمليو += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عمليو + حجم (SRC_TYPE) <= _mem->ڊيگهائي     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_معلومات(_mem) + عمليو;       \
            SRC_TYPE قدر;                             \
            يادگيري_نقل(&قدر, src8, حجم(قدر));        \
            M3_BSWAP_##SRC_TYPE(قدر);                 \
            REG = (DEST_TYPE)قدر;                     \
            d_m3_Trace_ڀريو(DEST_TYPE, عمليو, REG);     \
        }                                               \
        ايندڙ_Op_Preloaded ();                             \
    } وإلا d_ٻاهريون_جو_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عمليو = جڳهه (u32);                           \
    u32 offset = فوري (u32);                       \
    d_m3_Preload_ايندڙ ();                                 \
    عمليو += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عمليو + حجم (SRC_TYPE) <= _mem->ڊيگهائي     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_معلومات(_mem) + عمليو;       \
            SRC_TYPE قدر;                             \
            يادگيري_نقل(&قدر, src8, حجم(قدر));        \
            M3_BSWAP_##SRC_TYPE(قدر);                 \
            REG = (DEST_TYPE)قدر;                     \
            d_m3_Trace_ڀريو(DEST_TYPE, عمليو, REG);     \
        }                                               \
        ايندڙ_Op_Preloaded ();                             \
    } وإلا d_ٻاهريون_جو_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#وصف_ڪريو d_m3_ڀريو_i(DEST_TYPE, SRC_TYPE) d_m3_ڀريو(_r0, DEST_TYPE, SRC_TYPE)
#وصف_ڪريو d_m3_ڀريو_f(DEST_TYPE, SRC_TYPE) d_m3_ڀريو(_fp0, DEST_TYPE, SRC_TYPE)

#إذا d_m3HasFloat
d_m3_ڀريو_f(f32, f32);
d_m3_ڀريو_f(f64, f64);
#جيڪڏهن_پورو

d_m3_ڀريو_i(i32, i8);
d_m3_ڀريو_i(i32, u8);
d_m3_ڀريو_i(i32, i16);
d_m3_ڀريو_i(i32, u16);
d_m3_ڀريو_i(i32, i32);

d_m3_ڀريو_i(i64, i8);
d_m3_ڀريو_i(i64, u8);
d_m3_ڀريو_i(i64, i16);
d_m3_ڀريو_i(i64, u16);
d_m3_ڀريو_i(i64, i32);
d_m3_ڀريو_i(i64, u32);
d_m3_ڀريو_i(i64, i64);

#إذا d_m3_Fold_سيٽ_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_ڀريو_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = فوري (u32);                       \
      DEST_TYPE * dest = جڳهه_ptr (DEST_TYPE);            \
      d_m3_Preload_ايندڙ ();                                 \
      u64 عمليو = (u32) _r0;                            \
      عمليو += offset;                                  \
                                                          \
      إذا (m3MemCheck(                                     \
          عمليو + حجم (SRC_TYPE) <= _mem->ڊيگهائي     \
      )) {                                                \
          u8* src8 = m3_Mem_معلومات(_mem) + عمليو;           \
          SRC_TYPE قدر;                                 \
          يادگيري_نقل(&قدر, src8, حجم(قدر));            \
          M3_BSWAP_##SRC_TYPE(قدر);                     \
          DEST_TYPE نتيجو = (DEST_TYPE)قدر;            \
          * dest = نتيجو;  REG = نتيجو;                 \
          d_m3_Trace_ڀريو(DEST_TYPE, عمليو, نتيجو);      \
          ايندڙ_Op_Preloaded ();                             \
      } وإلا d_ٻاهريون_جو_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 عمليو = جڳهه (u32);                           \
      u32 offset = فوري (u32);                       \
      DEST_TYPE * dest = جڳهه_ptr (DEST_TYPE);            \
      d_m3_Preload_ايندڙ ();                                 \
      عمليو += offset;                                  \
                                                          \
      إذا (m3MemCheck(                                     \
          عمليو + حجم (SRC_TYPE) <= _mem->ڊيگهائي     \
      )) {                                                \
          u8* src8 = m3_Mem_معلومات(_mem) + عمليو;           \
          SRC_TYPE قدر;                                 \
          يادگيري_نقل(&قدر, src8, حجم(قدر));            \
          M3_BSWAP_##SRC_TYPE(قدر);                     \
          DEST_TYPE نتيجو = (DEST_TYPE)قدر;            \
          * dest = نتيجو;  REG = نتيجو;                 \
          d_m3_Trace_ڀريو(DEST_TYPE, عمليو, نتيجو);      \
          ايندڙ_Op_Preloaded ();                             \
      } وإلا d_ٻاهريون_جو_Bounds;                               \
  }

d_m3_ڀريو_Fold(_r0, i32, i8)
d_m3_ڀريو_Fold(_r0, i32, u8)
d_m3_ڀريو_Fold(_r0, i32, i16)
d_m3_ڀريو_Fold(_r0, i32, u16)
d_m3_ڀريو_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_ڀريو_Fold(_r0, i64, i8)
d_m3_ڀريو_Fold(_r0, i64, u8)
d_m3_ڀريو_Fold(_r0, i64, i16)
d_m3_ڀريو_Fold(_r0, i64, u16)
d_m3_ڀريو_Fold(_r0, i64, i32)
d_m3_ڀريو_Fold(_r0, i64, u32)
d_m3_ڀريو_Fold(_r0, i64, i64)
#  إذا d_m3HasFloat
d_m3_ڀريو_Fold(_fp0, f32, f32)
d_m3_ڀريو_Fold(_fp0, f64, f64)
#  endif

#جيڪڏهن_پورو // d_m3FoldSetLocal

#وصف_ڪريو d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عمليو = جڳهه (u32);                           \
    u32 offset = فوري (u32);                       \
    d_m3_Preload_ايندڙ ();                                 \
    عمليو += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عمليو + حجم (DEST_TYPE) <= _mem->ڊيگهائي    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عمليو, REG);     \
            u8* mem8 = m3_Mem_معلومات(_mem) + عمليو;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            يادگيري_نقل(mem8, &val, حجم(val));            \
        }                                               \
        ايندڙ_Op_Preloaded ();                             \
    } وإلا d_ٻاهريون_جو_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE قدر = جڳهه (SRC_TYPE);             \
    u64 عمليو = (u32) _r0;                            \
    u32 offset = فوري (u32);                       \
    d_m3_Preload_ايندڙ ();                                 \
    عمليو += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عمليو + حجم (DEST_TYPE) <= _mem->ڊيگهائي    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عمليو, قدر);   \
            u8* mem8 = m3_Mem_معلومات(_mem) + عمليو;       \
            DEST_TYPE val = (DEST_TYPE) قدر;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            يادگيري_نقل(mem8, &val, حجم(val));            \
        }                                               \
        ايندڙ_Op_Preloaded ();                             \
    } وإلا d_ٻاهريون_جو_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ثابت SRC_TYPE قدر = جڳهه (SRC_TYPE);             \
    u64 عمليو = جڳهه (u32);                           \
    u32 offset = فوري (u32);                       \
    d_m3_Preload_ايندڙ ();                                 \
    عمليو += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عمليو + حجم (DEST_TYPE) <= _mem->ڊيگهائي    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, عمليو, قدر);   \
            u8* mem8 = m3_Mem_معلومات(_mem) + عمليو;       \
            DEST_TYPE val = (DEST_TYPE) قدر;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            يادگيري_نقل(mem8, &val, حجم(val));            \
        }                                               \
        ايندڙ_Op_Preloaded ();                             \
    } وإلا d_ٻاهريون_جو_Bounds;                               \
}

// both operands can be in regs when storing a float
#وصف_ڪريو d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 عمليو = (u32) _r0;                            \
    u32 offset = فوري (u32);                       \
    d_m3_Preload_ايندڙ ();                                 \
    عمليو += offset;                                  \
                                                        \
    إذا (m3MemCheck(                                     \
        عمليو + حجم (TYPE) <= _mem->ڊيگهائي         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, عمليو, REG);         \
            u8* mem8 = m3_Mem_معلومات(_mem) + عمليو;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            يادگيري_نقل(mem8, &val, حجم(val));            \
        }                                               \
        ايندڙ_Op_Preloaded ();                             \
    } وإلا d_ٻاهريون_جو_Bounds;                               \
}


#وصف_ڪريو d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#وصف_ڪريو d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#إذا d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#جيڪڏهن_پورو

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#ضمن "m3_exec_atomic.h"

#وصف_رد m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#إذا d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    حرف name[100];
    strcpy(name, لڙي_ڳولا(i_opcode, "op_") + 3);
    حرف* bracket = لڙي_ڳولا(name, "(");
    إذا (bracket) {
        *bracket = 0;
    }

    puts(name);
    ايندڙ_Op_سڌو();
}
#جيڪڏهن_پورو

#إذا d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_ڪاميابي(i_operationName);

    ايندڙ_Op_سڌو();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_ڪاميابي(i_operationName);

    ايندڙ_Op_سڌو();
}
#جيڪڏهن_پورو

d_m3_پڇاڙي_Extern_C

#جيڪڏهن_پورو // m3_exec_h
