//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_exec_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_exec_h

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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_math_utils.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_info.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exec_defs.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_atomic.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_host.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits.h>

d_m3BeginExternC

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ rewrite_op(OP)             * ((ਖਾਲੀ **) (_pc-1)) = (ਖਾਲੀ*)(OP)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਤੁਰੰਤ(TYPE)            * ((TYPE *) _pc++)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ skip_ਤੁਰੰਤ(TYPE)       (_pc++)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਥਾਂ(TYPE)                 * (TYPE *) (_sp + ਤੁਰੰਤ (i32))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ਥਾਂ_ptr(TYPE)             (TYPE *) (_sp + ਤੁਰੰਤ (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Wide_ਸੰਕਾਰਜ(IS64)      ((IS64) ? ਥਾਂ (u64) : (u64) ਥਾਂ (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3MemRangeOk(ਸ਼ੁਰੂ_2, ਲੰਮਾਈ_2, MEM)  \
   ((ਸ਼ੁਰੂ_2) <= (MEM)->ਲੰਮਾਈ and (ਲੰਮਾਈ_2) <= (MEM)->ਲੰਮਾਈ - (ਸ਼ੁਰੂ_2))


#ਜੇ d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define ਅਗਲਾ_Op()                 M3_MUSTTAIL ਵਾਪਸ profileOp (d_m3_Op_All_ਦਲੀਲਾਂ, __FUNCTION__)
#ਜਾਂਜੇ d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define ਅਗਲਾ_Op()                 M3_MUSTTAIL ਵਾਪਸ debugOp (d_m3_Op_All_ਦਲੀਲਾਂ, __FUNCTION__)
#ਨਹੀਂਤਾਂ
#  define ਅਗਲਾ_Op()                 ਅਗਲਾ_Op_ਸਿੱਧਾ()
#ਜੇ_ਅੰਤ

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#ਜੇ d_m3_Preload_ਅਗਲਾ_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_ਅਗਲਾ()        ਅਚਲ IM3Operation ਅਗਲਾ_Op_2 = (IM3Operation) (* _pc)
#  define ਅਗਲਾ_Op_Preloaded()        M3_MUSTTAIL ਵਾਪਸ ਅਗਲਾ_Op_2 (_pc + 1, d_m3_Op_ਦਲੀਲਾਂ)
#ਨਹੀਂਤਾਂ
#  define d_m3_Preload_ਅਗਲਾ()
#  define ਅਗਲਾ_Op_Preloaded()        ਅਗਲਾ_Op()
#ਜੇ_ਅੰਤ

#ਜੇ d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL ਵਾਪਸ profileJumpOp ((pc_t)(PC), d_m3_Op_ਦਲੀਲਾਂ, __FUNCTION__)
#ਨਹੀਂਤਾਂ
#  define jumpOp(PC)               jump_Op_ਸਿੱਧਾ(PC)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
#  define ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2()            (ਧੱਕੋ_Backtrace_ਚੌਖਟਾ (_mem->runtime, _pc - 1))
#  define fill_Backtrace_ਚੌਖਟਾ(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    ਵਾਪਸ (ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2 (), err)
#  define forwardTrap(err)                ਵਾਪਸ err
#ਨਹੀਂਤਾਂ
#  define ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2()            ਕਰੋ {} ਜਦੋਂ (0)
#  define fill_Backtrace_ਚੌਖਟਾ(FUNCTION)    ਕਰੋ {} ਜਦੋਂ (0)

#  define newTrap(err)                    ਵਾਪਸ err
#  define forwardTrap(err)                ਵਾਪਸ err
#ਜੇ_ਅੰਤ

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#ਜੇ d_m3_Max_Native_ਢੇਰ > 0
#  define d_m3_Check_Native_ਢੇਰ()                                               \
       ਕਰੋ {                                                                    \
           ਖਾਲੀ * m3_ਹੱਦਬੰਦੀ = m3MemRuntime (_mem)->ਢੇਰ_ਹੱਦਬੰਦੀ;                  \
           ਜੇ (M3_UNLIKELY (m3_ਹੱਦਬੰਦੀ && m3_Native_ਢੇਰ_Ptr () < m3_ਹੱਦਬੰਦੀ))      \
               newTrap (m3_Err_trap_ਢੇਰ_ਉਛਾਲ);                              \
       } ਜਦੋਂ (0)
#ਨਹੀਂਤਾਂ
#  define d_m3_Check_Native_ਢੇਰ()           ਕਰੋ {} ਜਦੋਂ (0)
#ਜੇ_ਅੰਤ


#ਜੇ d_m3_Has_ਢੇਰ_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  ਜੇ d_m3HasFloat
#    define d_m3_Exp_Reg_ਦਲੀਲਾਂ(R0, FP0)        (R0), (FP0)
#  ਨਹੀਂਤਾਂ
#    define d_m3_Exp_Reg_ਦਲੀਲਾਂ(R0, FP0)        (R0)
#  endif

#  ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_ਦਲੀਲਾਂ (R0, FP0), d_m3_ਅਧਾਰ_Cstr)
#  ਨਹੀਂਤਾਂ
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_ਦਲੀਲਾਂ (R0, FP0))
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
ਸਥਿਰ M3_NOINLINE
m3ret_t ਰਿਕਾਰਡ_Call_ਚੌਖਟਾ (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_ਸਮ੍ਰਿਤੀ i_ਸਮ੍ਰਿਤੀ,
                         IM3Function i_function, m3reg_t i_r0
#  ਜੇ d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3_ਚੌਖਟਾ ਚੌਖਟਾ;

    ਚੌਖਟਾ.ਕਿਸਮ = ਚੌਖਟਾ_call;
    ਚੌਖਟਾ.pc = i_pc;
    ਚੌਖਟਾ.sp = i_sp;
    ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ = i_ਸਮ੍ਰਿਤੀ;
    ਚੌਖਟਾ.call.function = i_function;
    ਚੌਖਟਾ.call.r0 = i_r0;
#  ਜੇ d_m3HasFloat
    ਚੌਖਟਾ.call.fp0 = i_fp0;
#  endif

    ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(i_runtime, &ਚੌਖਟਾ);
}

#  define d_m3_ਰਿਕਾਰਡ_Call_ਚੌਖਟਾ(ਸਮ੍ਰਿਤੀ_2, FUNCTION)                                \
       ਜੇ (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           ਵਾਪਸ ਰਿਕਾਰਡ_Call_ਚੌਖਟਾ (m3MemRuntime (_mem), _pc, _sp, (ਸਮ੍ਰਿਤੀ_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_ਦਲੀਲਾਂ (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
ਸਥਿਰ M3_NOINLINE
m3ret_t ਰਿਕਾਰਡ_Loop_ਚੌਖਟਾ (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_ਸਮ੍ਰਿਤੀ i_ਸਮ੍ਰਿਤੀ)
{
    M3_ਚੌਖਟਾ ਚੌਖਟਾ;

    ਚੌਖਟਾ.ਕਿਸਮ = ਚੌਖਟਾ_loop;
    ਚੌਖਟਾ.pc = i_pc;
    ਚੌਖਟਾ.sp = i_sp;
    ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ = i_ਸਮ੍ਰਿਤੀ;

    ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(i_runtime, &ਚੌਖਟਾ);
}


#  ਜੇ d_m3HasExceptionHandling
ਸਥਿਰ M3_NOINLINE
m3ret_t ਰਿਕਾਰਡ_Try_ਚੌਖਟਾ (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_ਸਮ੍ਰਿਤੀ i_ਸਮ੍ਰਿਤੀ,
                        u32 i_ਅੰਕ_Clauses, bool i_handlers_ਜੀਵਤ)
{
    M3_ਚੌਖਟਾ ਚੌਖਟਾ;

    ਚੌਖਟਾ.ਕਿਸਮ = ਚੌਖਟਾ_try;
    ਚੌਖਟਾ.pc = i_clauses;
    ਚੌਖਟਾ.sp = i_sp;
    ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ = i_ਸਮ੍ਰਿਤੀ;
    ਚੌਖਟਾ.try_.ਅੰਕ_Clauses = i_ਅੰਕ_Clauses;
    ਚੌਖਟਾ.try_.handlers_ਜੀਵਤ = i_handlers_ਜੀਵਤ;

    ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(i_runtime, &ਚੌਖਟਾ);
}
#  endif


#  ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
ਸਥਿਰ M3_NOINLINE
m3ret_t ਰਿਕਾਰਡ_ਇੰਦਰਾਜ_ਚੌਖਟਾ (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_ਸਮ੍ਰਿਤੀ i_ਸਮ੍ਰਿਤੀ,
                          IM3Function i_function)
{
    M3_ਚੌਖਟਾ ਚੌਖਟਾ;

    ਚੌਖਟਾ.ਕਿਸਮ = ਚੌਖਟਾ_ਇੰਦਰਾਜ;
    ਚੌਖਟਾ.pc = i_pc;
    ਚੌਖਟਾ.sp = i_sp;
    ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ = i_ਸਮ੍ਰਿਤੀ;
    ਚੌਖਟਾ.ਇੰਦਰਾਜ.function = i_function;

    ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(i_runtime, &ਚੌਖਟਾ);
}
#  endif
#ਨਹੀਂਤਾਂ
#  define d_m3_ਰਿਕਾਰਡ_Call_ਚੌਖਟਾ(ਸਮ੍ਰਿਤੀ_2, FUNCTION)
#ਜੇ_ਅੰਤ


#ਜੇ d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#ਜਾਂਜੇ d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ਅਚਲ IM3Runtime trace_rt = m3MemRuntime(_mem); (ਖਾਲੀ) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_ਡੂੰਘਾਈ)*2, "", ##__VA_ARGS__)
#ਨਹੀਂਤਾਂ
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#ਜੇ_ਅੰਤ

#ਜੇ d_m3EnableStrace >= 3
#  define d_m3_Trace_ਲੱਦੋ(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#ਨਹੀਂਤਾਂ
#  define d_m3_Trace_ਲੱਦੋ(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
  #ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_ਬਾਹਰਲਾ_ਦਾ_Bounds newTrap (ErrorRuntime (m3_Err_trap_ਬਾਹਰਲਾ_ਦਾ_Bounds_ਸਮ੍ਰਿਤੀ_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->ਲੰਮਾਈ, ਸੰਕਾਰਜ))

#  define d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(ਵਿੱਥ, ਮਾਪ_2) newTrap (ErrorRuntime (m3_Err_trap_ਬਾਹਰਲਾ_ਦਾ_Bounds_ਸਮ੍ਰਿਤੀ_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->ਲੰਮਾਈ, ਵਿੱਥ, ਮਾਪ_2))
#ਨਹੀਂਤਾਂ
  #ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_ਬਾਹਰਲਾ_ਦਾ_Bounds newTrap (m3_Err_trap_ਬਾਹਰਲਾ_ਦਾ_Bounds_ਸਮ੍ਰਿਤੀ_Access)

#  define d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(ਵਿੱਥ, ਮਾਪ_2) newTrap (m3_Err_trap_ਬਾਹਰਲਾ_ਦਾ_Bounds_ਸਮ੍ਰਿਤੀ_Access)

#ਜੇ_ਅੰਤ

#ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#ਨਹੀਂਤਾਂ
d_m3RetSig Call (d_m3OpSig)
#ਜੇ_ਅੰਤ
{
    m3ret_t possible_trap = m3_Yield();
    ਜੇ (M3_UNLIKELY(possible_trap)) {
        ਵਾਪਸ possible_trap;
    }

    ਅਗਲਾ_Op_ਸਿੱਧਾ();
}

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                         \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    OP((RES), ਸੰਕਾਰਜ, ((TYPE) REG), ##__VA_ARGS__);    \
    ਅਗਲਾ_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = ਥਾਂ (TYPE);                        \
    TYPE operand1 = ਥਾਂ (TYPE);                        \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    ਅਗਲਾ_Op_Preloaded ();                                 \
}

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                         \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    OP((RES), ((TYPE) REG), ਸੰਕਾਰਜ, ##__VA_ARGS__);    \
    ਅਗਲਾ_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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

#ਜੇ d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_ਜੋੜੋ_32(A,B) (i32)((u32)(A) + (u32)(B))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_ਜੋੜੋ_64(A,B) (i64)((u64)(A) + (u64)(B))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, ਜੋੜੋ,      OP_ਜੋੜੋ_32)  d_m3CommutativeOpFunc_i (i64, ਜੋੜੋ,      OP_ਜੋੜੋ_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, ਘਟਾਓ,            OP_SUB_32)  d_m3OpFunc_i (i64, ਘਟਾਓ,            OP_SUB_64)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, ਖਿਸਕਾਓ_ਖੱਬਾ,       OP_SHL_32)      d_m3OpFunc_i (u64, ਖਿਸਕਾਓ_ਖੱਬਾ,       OP_SHL_64)
d_m3OpFunc_i (i32, ਖਿਸਕਾਓ_ਸੱਜਾ,      OP_SHR_32)      d_m3OpFunc_i (i64, ਖਿਸਕਾਓ_ਸੱਜਾ,      OP_SHR_64)
d_m3OpFunc_i (u32, ਖਿਸਕਾਓ_ਸੱਜਾ,      OP_SHR_32)      d_m3OpFunc_i (u64, ਖਿਸਕਾਓ_ਸੱਜਾ,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#ਜੇ d_m3_Fold_ਸੈੱਟ_Local

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
    TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                         \
    TYPE * dest = ਥਾਂ_ptr (TYPE);                      \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    TYPE ਨਤੀਜਾ;                                        \
    OP((ਨਤੀਜਾ), ਸੰਕਾਰਜ, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = ਨਤੀਜਾ;  REG = ਨਤੀਜਾ;                     \
    ਅਗਲਾ_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = ਥਾਂ (TYPE);                        \
    TYPE operand1 = ਥਾਂ (TYPE);                        \
    TYPE * dest = ਥਾਂ_ptr (TYPE);                      \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    TYPE ਨਤੀਜਾ;                                        \
    OP((ਨਤੀਜਾ), operand1, operand2, ##__VA_ARGS__);    \
    * dest = ਨਤੀਜਾ;  REG = ਨਤੀਜਾ;                     \
    ਅਗਲਾ_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                         \
    TYPE * dest = ਥਾਂ_ptr (TYPE);                      \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    TYPE ਨਤੀਜਾ;                                        \
    OP((ਨਤੀਜਾ), ((TYPE) REG), ਸੰਕਾਰਜ, ##__VA_ARGS__); \
    * dest = ਨਤੀਜਾ;  REG = ਨਤੀਜਾ;                     \
    ਅਗਲਾ_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, ਜੋੜੋ,          M3_FUNC, OP_ਜੋੜੋ_32)
d_m3FoldOp_i           (i32, ਘਟਾਓ,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, ਖਿਸਕਾਓ_ਖੱਬਾ,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, ਖਿਸਕਾਓ_ਸੱਜਾ,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, ਖਿਸਕਾਓ_ਸੱਜਾ,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, ਜੋੜੋ,          M3_FUNC, OP_ਜੋੜੋ_64)
d_m3FoldOp_i           (i64, ਘਟਾਓ,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, ਖਿਸਕਾਓ_ਖੱਬਾ,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, ਖਿਸਕਾਓ_ਸੱਜਾ,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, ਖਿਸਕਾਓ_ਸੱਜਾ,   M3_FUNC, OP_SHR_64)

#  ਜੇ d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, ਜੋੜੋ,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, ਘਟਾਓ,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, ਜੋੜੋ,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, ਘਟਾਓ,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#ਜੇ_ਅੰਤ

#ਜੇ_ਅੰਤ // d_m3FoldSetLocal

#ਜੇ d_m3HasFloat
d_m3CommutativeArithOp_f(f32, ਜੋੜੋ,         +)      d_m3CommutativeArithOp_f(f64, ਜੋੜੋ,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, ਘਟਾਓ,               -)      d_m3ArithOp_f(f64, ਘਟਾਓ,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#ਜੇ_ਅੰਤ

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

#ਜੇ d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#ਜੇ_ਅੰਤ

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    ਅਗਲਾ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                     \
    OP((RES), ਸੰਕਾਰਜ, ##__VA_ARGS__);              \
    ਅਗਲਾ_Op ();                                      \
}

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_UNARY(RES, X, OP) (RES) = OP(X)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#ਜੇ d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       ਵਰਗਮੂਲ);
d_m3UnaryArithOp_f(f32, ਸਭ_ਤੋਂ_ਨੇੜੇ,    rintf);    d_m3UnaryArithOp_f(f64, ਸਭ_ਤੋਂ_ਨੇੜੇ,    rint);
#  ਜੇ defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  ਨਹੀਂਤਾਂ
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_ਮੰਜ਼ਿਲ_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_ਮੰਜ਼ਿਲ_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#ਜੇ (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#ਜਾਂਜੇ defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CLZ_32(x) __builtin_clz(x)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CTZ_32(x) __builtin_ctz(x)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CLZ_64(x) __builtin_clzll(x)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CTZ_64(x) __builtin_ctzll(x)
#ਨਹੀਂਤਾਂ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#ਜੇ_ਅੰਤ

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    ਅਗਲਾ_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 ਸੰਕਾਰਜ = ਥਾਂ(i64);
    _r0 = OP_WRAP_I64(ਸੰਕਾਰਜ);
    ਅਗਲਾ_Op();
}

// Integer sign extension operations
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#ਜੇ d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_ਉੱਚਾ = ਥਾਂ (u64);
    u64  rhs_ਨੀਵਾਂ  = ਥਾਂ (u64);
    u64  lhs_ਉੱਚਾ = ਥਾਂ (u64);
    u64  lhs_ਨੀਵਾਂ  = ਥਾਂ (u64);
    u64* ਨੀਵਾਂ     = ਥਾਂ_ptr (u64);
    d_m3_Preload_ਅਗਲਾ();

    u64 ਜੋੜ = lhs_ਨੀਵਾਂ + rhs_ਨੀਵਾਂ;

    *ਨੀਵਾਂ = ਜੋੜ;
    _r0  = (i64) (lhs_ਉੱਚਾ + rhs_ਉੱਚਾ + (ਜੋੜ < lhs_ਨੀਵਾਂ));

    ਅਗਲਾ_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_ਉੱਚਾ = ਥਾਂ (u64);
    u64  rhs_ਨੀਵਾਂ  = ਥਾਂ (u64);
    u64  lhs_ਉੱਚਾ = ਥਾਂ (u64);
    u64  lhs_ਨੀਵਾਂ  = ਥਾਂ (u64);
    u64* ਨੀਵਾਂ     = ਥਾਂ_ptr (u64);
    d_m3_Preload_ਅਗਲਾ();

    *ਨੀਵਾਂ = lhs_ਨੀਵਾਂ - rhs_ਨੀਵਾਂ;
    _r0  = (i64) (lhs_ਉੱਚਾ - rhs_ਉੱਚਾ - (lhs_ਨੀਵਾਂ < rhs_ਨੀਵਾਂ));

    ਅਗਲਾ_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = ਥਾਂ (i64);
    i64  a   = ਥਾਂ (i64);
    u64* ਨੀਵਾਂ = ਥਾਂ_ptr (u64);
    d_m3_Preload_ਅਗਲਾ();

    u64 ਉੱਚਾ;
    *ਨੀਵਾਂ = mul_wide_i64 (a, b, &ਉੱਚਾ);
    _r0  = (i64) ਉੱਚਾ;

    ਅਗਲਾ_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = ਥਾਂ (u64);
    u64  a   = ਥਾਂ (u64);
    u64* ਨੀਵਾਂ = ਥਾਂ_ptr (u64);
    d_m3_Preload_ਅਗਲਾ();

    u64 ਉੱਚਾ;
    *ਨੀਵਾਂ = mul_wide_u64 (a, b, &ਉੱਚਾ);
    _r0  = (i64) ਉੱਚਾ;

    ਅਗਲਾ_Op_Preloaded();
}

#ਜੇ_ਅੰਤ // d_m3HasWideArithmetic

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    ਅਗਲਾ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * ਢੇਰ = ਥਾਂ_ptr (FROM);                 \
    OP((DEST), (* ਢੇਰ), ##__VA_ARGS__);           \
    ਅਗਲਾ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = ਥਾਂ_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    ਅਗਲਾ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * ਢੇਰ = ਥਾਂ_ptr (FROM);                 \
    TYPE * dest = ਥਾਂ_ptr (TYPE);                  \
    OP((* dest), (* ਢੇਰ), ##__VA_ARGS__);         \
    ਅਗਲਾ_Op ();                                      \
}

#ਜੇ d_m3HasFloat
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
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3TypeModifyOp(REG_ਮੰਜ਼ਿਲ, REG_FROM, ਮੰਜ਼ਿਲ_2, NAME, FROM)  \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_ਮੰਜ਼ਿਲ = (ਮੰਜ਼ਿਲ_2) ((FROM) REG_FROM);                        \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = ਥਾਂ (FROM);                                \
    REG_ਮੰਜ਼ਿਲ = (ਮੰਜ਼ਿਲ_2) (from);                                   \
    ਅਗਲਾ_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3FloatModifyOp(ਮੰਜ਼ਿਲ_2, NAME, FROM)                   \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((ਮੰਜ਼ਿਲ_2) ((FROM) _fp0));             \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = ਥਾਂ (FROM);                                \
    _fp0 = canon_nan_##TO ((ਮੰਜ਼ਿਲ_2) (from));                    \
    ਅਗਲਾ_Op ();                                              \
}

#ਜੇ d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3TypeConvertOp(REG_ਮੰਜ਼ਿਲ, REG_FROM, ਮੰਜ਼ਿਲ_2, NAME, FROM) \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_ਮੰਜ਼ਿਲ = (ਮੰਜ਼ਿਲ_2) ((FROM) REG_FROM);                        \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    ਥਾਂ (ਮੰਜ਼ਿਲ_2) = (ਮੰਜ਼ਿਲ_2) ((FROM) REG_FROM);                     \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = ਥਾਂ (FROM);                                \
    REG_ਮੰਜ਼ਿਲ = (ਮੰਜ਼ਿਲ_2) (from);                                   \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = ਥਾਂ (FROM);                                \
    ਥਾਂ (ਮੰਜ਼ਿਲ_2) = (ਮੰਜ਼ਿਲ_2) (from);                                \
    ਅਗਲਾ_Op ();                                              \
}

// Int to float
#ਜੇ d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3ReinterpretOp(REG, ਮੰਜ਼ਿਲ_2, SRC, FROM)               \
d_m3Op(ਮੰਜ਼ਿਲ_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    ਸੰਘ { FROM c; ਮੰਜ਼ਿਲ_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    ਸੰਘ { FROM c; ਮੰਜ਼ਿਲ_2 t; } u;                              \
    u.c = ਥਾਂ (FROM);                                      \
    REG = u.t;                                              \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    ਸੰਘ { FROM c; ਮੰਜ਼ਿਲ_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    ਥਾਂ (ਮੰਜ਼ਿਲ_2) = u.t;                                        \
    ਅਗਲਾ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ਮੰਜ਼ਿਲ_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    ਸੰਘ { FROM c; ਮੰਜ਼ਿਲ_2 t; } u;                              \
    u.c = ਥਾਂ (FROM);                                      \
    ਥਾਂ (ਮੰਜ਼ਿਲ_2) = u.t;                                        \
    ਅਗਲਾ_Op ();                                              \
}

#ਜੇ d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#ਜੇ_ਅੰਤ

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = ਤੁਰੰਤ(u32*);
    ਥਾਂ(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    ਅਗਲਾ_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = ਤੁਰੰਤ(u64*);
    ਥਾਂ(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    ਅਗਲਾ_Op();
}


d_m3Op(ਸੈੱਟ_Global_i32)
{
    u32* global = ਤੁਰੰਤ(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    ਅਗਲਾ_Op();
}


d_m3Op(ਸੈੱਟ_Global_i64)
{
    u64* global = ਤੁਰੰਤ(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    ਅਗਲਾ_Op();
}


#ਜੇ d_m3_Has_ਢੇਰ_Switching

d_m3Op(Branch);
d_m3Op(ਇੰਦਰਾਜ_2);
#  ਜੇ !d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
d_m3Op(ਇੰਦਰਾਜ_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
ਸਥਿਰ M3_NOINLINE
IM3Function Function_ਦਾ_Compiled_ਕੋਡ (pc_t i_pc)
{
    ਜੇ (not i_pc) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਜਦੋਂ ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation ਇੰਦਰਾਜ = (IM3Operation)i_pc[0];

#  ਜੇ !d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
    ਜੇ (ਇੰਦਰਾਜ == op_ਇੰਦਰਾਜ_Suspendable) {
        ਵਾਪਸ (IM3Function)i_pc[1];
    }
#  endif
    ਵਾਪਸ (ਇੰਦਰਾਜ == op_ਇੰਦਰਾਜ) ? (IM3Function)i_pc[1] : ਨਿਰਾ;
}

#ਜੇ_ਅੰਤ

d_m3Op(Call)
{
    d_m3_Check_Native_ਢੇਰ();

    pc_t      callPC = ਤੁਰੰਤ(pc_t);
    i32       ਢੇਰ_ਵਿੱਥ = ਤੁਰੰਤ(i32);
    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    m3stack_t sp = _sp + ਢੇਰ_ਵਿੱਥ;

#ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_ਦਲੀਲਾਂ, d_m3_ਅਧਾਰ_Cstr);
#ਨਹੀਂਤਾਂ
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_ਦਲੀਲਾਂ);
#ਜੇ_ਅੰਤ

    _mem = ਸਮ੍ਰਿਤੀ->mallocated;

    ਜੇ (M3_LIKELY(not r)) {
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        d_m3_ਰਿਕਾਰਡ_Call_ਚੌਖਟਾ(ਸਮ੍ਰਿਤੀ, Function_ਦਾ_Compiled_ਕੋਡ(callPC));
        ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = ਥਾਂ(IM3Function);
    IM3FuncType type = ਤੁਰੰਤ(IM3FuncType);
    i32         ਢੇਰ_ਵਿੱਥ = ਤੁਰੰਤ(i32);
    IM3_ਸਮ੍ਰਿਤੀ   ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ਢੇਰ_ਵਿੱਥ;

    m3ret_t     r = m3Err_none;

    ਜੇ (M3_LIKELY(function)) {
        ਜੇ (M3_LIKELY(type == function->funcType)) {
            ਜੇ (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            ਜੇ (M3_LIKELY(not r)) {
#ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_ਦਲੀਲਾਂ, d_m3_ਅਧਾਰ_Cstr);
#ਨਹੀਂਤਾਂ
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_ਦਲੀਲਾਂ);
#ਜੇ_ਅੰਤ

                _mem = ਸਮ੍ਰਿਤੀ->mallocated;

                ਜੇ (M3_LIKELY(not r)) {
                    ਅਗਲਾ_Op_ਸਿੱਧਾ();
                } ਨਹੀਂਤਾਂ {
                    d_m3_ਰਿਕਾਰਡ_Call_ਚੌਖਟਾ(ਸਮ੍ਰਿਤੀ, function);
                    ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2();
                    forwardTrap(r);
                }
            }
        } ਨਹੀਂਤਾਂ {
            r = m3_Err_trap_ਅਸਿੱਧਾ_Call_Type_Mismatch;
        }
    } ਨਹੀਂਤਾਂ {
        r = m3Err_trapNullFunctionRef;
    }

    ਜੇ (M3_UNLIKELY(r)) {
        newTrap(r);
    } ਨਹੀਂਤਾਂ {
        forwardTrap(r);
    }
}


// ref.as_non_null is a null check and nothing else: the value stays put, so
// there is no destination slot, only the trap.
d_m3Op(RefAsNonNull)
{
    IM3Function reference = ਥਾਂ(IM3Function);

    ਜੇ (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    ਅਗਲਾ_Op();
}


d_m3Op(Call_ਅਸਿੱਧਾ)
{
    d_m3_Check_Native_ਢੇਰ();

    M3_ਸਾਰਣੀ*    ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    u64         ਸਾਰਣੀ_ਸੂਚਕਾਂਕ = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);
    IM3FuncType type = ਤੁਰੰਤ(IM3FuncType);
    i32         ਢੇਰ_ਵਿੱਥ = ਤੁਰੰਤ(i32);
    IM3_ਸਮ੍ਰਿਤੀ   ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ਢੇਰ_ਵਿੱਥ;

    m3ret_t     r = m3Err_none;

    ਜੇ (M3_LIKELY(ਸਾਰਣੀ_ਸੂਚਕਾਂਕ < ਸਾਰਣੀ_2->ਮਾਪ)) {
        IM3Function function = (IM3Function)ਸਾਰਣੀ_2->elements[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ];

        ਜੇ (M3_LIKELY(function)) {
            ਜੇ (M3_LIKELY(type == function->funcType)) {
                ਜੇ (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                ਜੇ (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* calleeMem = Module_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ(function->module);

#ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_ਦਲੀਲਾਂ, d_m3_ਅਧਾਰ_Cstr);
#ਨਹੀਂਤਾਂ
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_ਦਲੀਲਾਂ);
#ਜੇ_ਅੰਤ

                    _mem = ਸਮ੍ਰਿਤੀ->mallocated;

                    ਜੇ (M3_LIKELY(not r)) {
                        ਅਗਲਾ_Op_ਸਿੱਧਾ();
                    } ਨਹੀਂਤਾਂ {
                        d_m3_ਰਿਕਾਰਡ_Call_ਚੌਖਟਾ(ਸਮ੍ਰਿਤੀ, function);
                        ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2();
                        forwardTrap(r);
                    }
                }
            } ਨਹੀਂਤਾਂ {
                r = m3_Err_trap_ਅਸਿੱਧਾ_Call_Type_Mismatch;
            }
        } ਨਹੀਂਤਾਂ {
            r = m3_Err_trap_ਸਾਰਣੀ_Element_Is_Null;
        }
    } ਨਹੀਂਤਾਂ {
        r = m3_Err_trap_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Range;
    }

    ਜੇ (M3_UNLIKELY(r)) {
        newTrap(r);
    } ਨਹੀਂਤਾਂ {
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
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਪੂਛ_Call_ਦਲੀਲਾਂ(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((ਖਾਲੀ *) (_sp + (RETURNSLOTS)), (ਅਚਲ ਖਾਲੀ *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * ਆਕਾਰ (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = ਤੁਰੰਤ(pc_t);
    i32     ਢੇਰ_ਵਿੱਥ = ਤੁਰੰਤ(i32);
    i32     returnSlots = ਤੁਰੰਤ(i32);
    u32     ਅੰਕ_Arg_Slots = ਤੁਰੰਤ(u32);

    m3ret_t possible_trap = m3_Yield();
    ਜੇ (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_ਪੂਛ_Call_ਦਲੀਲਾਂ(returnSlots, ਢੇਰ_ਵਿੱਥ, ਅੰਕ_Arg_Slots);

    jump_Op_ਸਿੱਧਾ(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = ਥਾਂ(IM3Function);
    IM3FuncType type = ਤੁਰੰਤ(IM3FuncType);
    i32         ਢੇਰ_ਵਿੱਥ = ਤੁਰੰਤ(i32);
    i32         returnSlots = ਤੁਰੰਤ(i32);
    u32         ਅੰਕ_Arg_Slots = ਤੁਰੰਤ(u32);

    m3ret_t     r = m3Err_none;

    ਜੇ (M3_LIKELY(function)) {
        ਜੇ (M3_LIKELY(type == function->funcType)) {
            ਜੇ (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            ਜੇ (M3_LIKELY(not r)) {
                r = m3_Yield();

                ਜੇ (M3_LIKELY(not r)) {
                    d_m3_ਪੂਛ_Call_ਦਲੀਲਾਂ(returnSlots, ਢੇਰ_ਵਿੱਥ, ਅੰਕ_Arg_Slots);

                    jump_Op_ਸਿੱਧਾ(function->compiled);
                }
            }
        } ਨਹੀਂਤਾਂ {
            r = m3_Err_trap_ਅਸਿੱਧਾ_Call_Type_Mismatch;
        }
    } ਨਹੀਂਤਾਂ {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(Return_Call_ਅਸਿੱਧਾ)
{
    M3_ਸਾਰਣੀ*    ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    u64         ਸਾਰਣੀ_ਸੂਚਕਾਂਕ = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);
    IM3FuncType type = ਤੁਰੰਤ(IM3FuncType);
    i32         ਢੇਰ_ਵਿੱਥ = ਤੁਰੰਤ(i32);
    i32         returnSlots = ਤੁਰੰਤ(i32);
    u32         ਅੰਕ_Arg_Slots = ਤੁਰੰਤ(u32);

    m3ret_t     r = m3Err_none;

    ਜੇ (M3_LIKELY(ਸਾਰਣੀ_ਸੂਚਕਾਂਕ < ਸਾਰਣੀ_2->ਮਾਪ)) {
        IM3Function function = (IM3Function)ਸਾਰਣੀ_2->elements[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ];

        ਜੇ (M3_LIKELY(function)) {
            ਜੇ (M3_LIKELY(type == function->funcType)) {
                ਜੇ (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                ਜੇ (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    ਜੇ (M3_LIKELY(not r)) {
                        d_m3_ਪੂਛ_Call_ਦਲੀਲਾਂ(returnSlots, ਢੇਰ_ਵਿੱਥ, ਅੰਕ_Arg_Slots);

                        jump_Op_ਸਿੱਧਾ(function->compiled);
                    }
                }
            } ਨਹੀਂਤਾਂ {
                r = m3_Err_trap_ਅਸਿੱਧਾ_Call_Type_Mismatch;
            }
        } ਨਹੀਂਤਾਂ {
            r = m3_Err_trap_ਸਾਰਣੀ_Element_Is_Null;
        }
    } ਨਹੀਂਤਾਂ {
        r = m3_Err_trap_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = ਤੁਰੰਤ(IM3Function);
    ctx.userdata = ਤੁਰੰਤ(ਖਾਲੀ*);
    u64* ਅਚਲ sp = ((u64*)_sp);
    IM3_ਸਮ੍ਰਿਤੀ  ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#ਜੇ d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    ਅੱਖਰ        outbuff[1024];
    ਅੱਖਰ*       outp = outbuff;
    ਅੱਖਰ*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.ਖੇਤਰ_Utf8);

    ਅਚਲ ਪੂਰਨ n_ਦਲੀਲਾਂ = ftype->ਅੰਕ_ਦਲੀਲਾਂ;
    ਅਚਲ ਪੂਰਨ nRets = ftype->ਅੰਕ_Rets;
    u64*      ਦਲੀਲਾਂ = sp + nRets;
    ਲਈ (ਪੂਰਨ i = 0; i < n_ਦਲੀਲਾਂ; i++) {
        ਅਚਲ ਪੂਰਨ type = ftype->types[nRets + i];
        ਚੋਣ (type) {
        ਹਾਲਤ c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(ਦਲੀਲਾਂ + i)); ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(ਦਲੀਲਾਂ + i)); ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(ਦਲੀਲਾਂ + i)); ਰੋਕੋ;
        ਹਾਲਤ c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(ਦਲੀਲਾਂ + i)); ਰੋਕੋ;
        ਮੂਲ: outp += snprintf(outp, oute - outp, "<type %d>", type); ਰੋਕੋ;
        }
        ਜੇ (i < n_ਦਲੀਲਾਂ - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  ਜੇ d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#ਜੇ_ਅੰਤ

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    ਖਾਲੀ* ਢੇਰ_backup = runtime->ਢੇਰ;
    runtime->ਢੇਰ = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_ਅੰਕੜੇ(ctx.function->host_ਸਮ੍ਰਿਤੀ->mallocated));
    runtime->ਢੇਰ = ਢੇਰ_backup;

#ਜੇ d_m3EnableStrace
    ਜੇ (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (ਅੱਖਰ*)possible_trap);
    } ਨਹੀਂਤਾਂ ਜੇ (ftype and ftype->ਅੰਕ_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } ਨਹੀਂਤਾਂ {
        d_m3TracePrint("%s", outbuff);
    }
#ਜੇ_ਅੰਤ

    ਜੇ (M3_UNLIKELY(possible_trap)) {
        _mem = ਸਮ੍ਰਿਤੀ->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        ਜੇ (possible_trap != m3Err_pendingException) {
            ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_ਮਾਪ)
{
    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    _r0 = ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ;

    ਅਗਲਾ_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_ਸਮ੍ਰਿਤੀ  ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    i32        ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow = (i32)_r0;
    ਜੇ (ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow >= 0) {
        _r0 = (m3reg_t)ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ;

        ਜੇ (M3_LIKELY(ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow)) {
            u64      required_ਪੰਨੇ = ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ + (u32)ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow;

            M3_ਨਤੀਜਾ r = Resize_ਸਮ੍ਰਿਤੀ(runtime, ਸਮ੍ਰਿਤੀ, required_ਪੰਨੇ);
            ਜੇ (r) {
                _r0 = -1;
            }

            _mem = ਸਮ੍ਰਿਤੀ->mallocated;
        }
    } ਨਹੀਂਤਾਂ {
        _r0 = -1;
    }

    ਅਗਲਾ_Op();
}


#ਜੇ d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_ਸਮ੍ਰਿਤੀ  ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    u64        ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow = (u64)_r0;
    u64        ਅੰਕ_ਪੰਨੇ = ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ;

    ਜੇ (M3_LIKELY(ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        ਜੇ (ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow <= ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ - ਅੰਕ_ਪੰਨੇ and
            not Resize_ਸਮ੍ਰਿਤੀ(runtime, ਸਮ੍ਰਿਤੀ, ਅੰਕ_ਪੰਨੇ + ਅੰਕ_ਪੰਨੇ_ਮੰਜ਼ਿਲ_Grow)) {
            _r0 = (m3reg_t)ਅੰਕ_ਪੰਨੇ;
        } ਨਹੀਂਤਾਂ {
            _r0 = -1;
        }

        _mem = ਸਮ੍ਰਿਤੀ->mallocated;
    } ਨਹੀਂਤਾਂ {
        _r0 = (m3reg_t)ਅੰਕ_ਪੰਨੇ;
    }

    ਅਗਲਾ_Op();
}

#ਜੇ_ਅੰਤ // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 ਮਾਪ = (u32)_r0;
    u64 ਸਰੋਤ = ਥਾਂ(u32);
    u64 destination = ਥਾਂ(u32);

    ਜੇ (M3_LIKELY(d_m3MemRangeOk(destination, ਮਾਪ, _mem))) {
        ਜੇ (M3_LIKELY(d_m3MemRangeOk(ਸਰੋਤ, ਮਾਪ, _mem))) {
            u8* dst = m3_Mem_ਅੰਕੜੇ(_mem) + destination;
            u8* src = m3_Mem_ਅੰਕੜੇ(_mem) + ਸਰੋਤ;
            memmove(dst, src, (size_t)ਮਾਪ);

            ਅਗਲਾ_Op();
        } ਨਹੀਂਤਾਂ {
            d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(ਸਰੋਤ, ਮਾਪ);
        }
    } ਨਹੀਂਤਾਂ {
        d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(destination, ਮਾਪ);
    }
}


#ਜੇ d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 ਮਾਪ = (u64)_r0;
    u64 ਸਰੋਤ = ਥਾਂ(u64);
    u64 destination = ਥਾਂ(u64);

    ਜੇ (M3_LIKELY(d_m3MemRangeOk(destination, ਮਾਪ, _mem))) {
        ਜੇ (M3_LIKELY(d_m3MemRangeOk(ਸਰੋਤ, ਮਾਪ, _mem))) {
            u8* dst = m3_Mem_ਅੰਕੜੇ(_mem) + destination;
            u8* src = m3_Mem_ਅੰਕੜੇ(_mem) + ਸਰੋਤ;
            memmove(dst, src, (size_t)ਮਾਪ);

            ਅਗਲਾ_Op();
        } ਨਹੀਂਤਾਂ {
            d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(ਸਰੋਤ, ਮਾਪ);
        }
    } ਨਹੀਂਤਾਂ {
        d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(destination, ਮਾਪ);
    }
}

#ਜੇ_ਅੰਤ // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(ਸੈੱਟ_ਸਮ੍ਰਿਤੀ)
{
    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = ਤੁਰੰਤ(IM3_ਸਮ੍ਰਿਤੀ);

    _mem = ਸਮ੍ਰਿਤੀ->mallocated;

    ਅਗਲਾ_Op();
}


#ਜੇ d_m3_Has_Multi_ਸਮ੍ਰਿਤੀ

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3_ਸਮ੍ਰਿਤੀ       dest_ਸਮ੍ਰਿਤੀ = ਤੁਰੰਤ(IM3_ਸਮ੍ਰਿਤੀ);
    IM3_ਸਮ੍ਰਿਤੀ       ਸਰੋਤ_ਸਮ੍ਰਿਤੀ = ਤੁਰੰਤ(IM3_ਸਮ੍ਰਿਤੀ);
    u32             widths = ਤੁਰੰਤ(u32);

    M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* destMem = dest_ਸਮ੍ਰਿਤੀ->mallocated;
    M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* ਸਰੋਤ_Mem = ਸਰੋਤ_ਸਮ੍ਰਿਤੀ->mallocated;

    u64             ਮਾਪ = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             ਸਰੋਤ = d_m3_Wide_ਸੰਕਾਰਜ(widths & 0x2);
    u64             destination = d_m3_Wide_ਸੰਕਾਰਜ(widths & 0x1);

    ਜੇ (M3_LIKELY(d_m3MemRangeOk(destination, ਮਾਪ, destMem))) {
        ਜੇ (M3_LIKELY(d_m3MemRangeOk(ਸਰੋਤ, ਮਾਪ, ਸਰੋਤ_Mem))) {
            memmove(m3_Mem_ਅੰਕੜੇ(destMem) + destination, m3_Mem_ਅੰਕੜੇ(ਸਰੋਤ_Mem) + ਸਰੋਤ, (size_t)ਮਾਪ);

            ਅਗਲਾ_Op();
        } ਨਹੀਂਤਾਂ {
            d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(ਸਰੋਤ, ਮਾਪ);
        }
    } ਨਹੀਂਤਾਂ {
        d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(destination, ਮਾਪ);
    }
}

#ਜੇ_ਅੰਤ // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 ਮਾਪ = (u32)_r0;
    u32 byte = ਥਾਂ(u32);
    u64 destination = ਥਾਂ(u32);

    ਜੇ (M3_LIKELY(d_m3MemRangeOk(destination, ਮਾਪ, _mem))) {
        u8* mem8 = m3_Mem_ਅੰਕੜੇ(_mem) + destination;
        memset(mem8, (u8)byte, ਮਾਪ);
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(destination, ਮਾਪ);
    }
}


#ਜੇ d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 ਮਾਪ = (u64)_r0;
    u32 byte = ਥਾਂ(u32);
    u64 destination = ਥਾਂ(u64);

    ਜੇ (M3_LIKELY(d_m3MemRangeOk(destination, ਮਾਪ, _mem))) {
        u8* mem8 = m3_Mem_ਅੰਕੜੇ(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)ਮਾਪ);
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(destination, ਮਾਪ);
    }
}

#ਜੇ_ਅੰਤ // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_ਅੰਕੜੇ_Segment* segment = ਤੁਰੰਤ(M3_ਅੰਕੜੇ_Segment*);

    u64            ਮਾਪ = (u32)_r0;
    u64            ਸਰੋਤ = ਥਾਂ(u32);
    u64            destination = ਥਾਂ(u32);

    u64            ਉਪਲਬਧ = segment->dropped ? 0 : segment->ਮਾਪ;

    ਜੇ (M3_LIKELY(d_m3MemRangeOk(destination, ਮਾਪ, _mem))) {
        ਜੇ (M3_LIKELY(ਸਰੋਤ <= ਉਪਲਬਧ and ਮਾਪ <= ਉਪਲਬਧ - ਸਰੋਤ)) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(m3_Mem_ਅੰਕੜੇ(_mem) + destination, segment->ਅੰਕੜੇ + ਸਰੋਤ, (size_t)ਮਾਪ);
            ਅਗਲਾ_Op();
        } ਨਹੀਂਤਾਂ {
            d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(ਸਰੋਤ, ਮਾਪ);
        }
    } ਨਹੀਂਤਾਂ {
        d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(destination, ਮਾਪ);
    }
}


#ਜੇ d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_ਅੰਕੜੇ_Segment* segment = ਤੁਰੰਤ(M3_ਅੰਕੜੇ_Segment*);

    u64            ਮਾਪ = (u32)_r0;
    u64            ਸਰੋਤ = ਥਾਂ(u32);
    u64            destination = ਥਾਂ(u64);

    u64            ਉਪਲਬਧ = segment->dropped ? 0 : segment->ਮਾਪ;

    ਜੇ (M3_LIKELY(d_m3MemRangeOk(destination, ਮਾਪ, _mem))) {
        ਜੇ (M3_LIKELY(ਸਰੋਤ <= ਉਪਲਬਧ and ਮਾਪ <= ਉਪਲਬਧ - ਸਰੋਤ)) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(m3_Mem_ਅੰਕੜੇ(_mem) + destination, segment->ਅੰਕੜੇ + ਸਰੋਤ, (size_t)ਮਾਪ);
            ਅਗਲਾ_Op();
        } ਨਹੀਂਤਾਂ {
            d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(ਸਰੋਤ, ਮਾਪ);
        }
    } ਨਹੀਂਤਾਂ {
        d_ਬਾਹਰਲਾ_ਦਾ_Bounds_Mem_Op(destination, ਮਾਪ);
    }
}

#ਜੇ_ਅੰਤ // d_m3HasMemory64


d_m3Op(ਅੰਕੜੇ_Drop)
{
    M3_ਅੰਕੜੇ_Segment* segment = ਤੁਰੰਤ(M3_ਅੰਕੜੇ_Segment*);
    segment->dropped = true;

    ਅਗਲਾ_Op();
}


#ਜੇ d_m3HasRefTypes

d_m3Op(ਸਾਰਣੀ_Get)
{
    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    u64      index = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);

    ਜੇ (M3_LIKELY(index < ਸਾਰਣੀ_2->ਮਾਪ)) {
        _r0 = (u64)(uintptr_t)ਸਾਰਣੀ_2->elements[index];
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        newTrap(m3_Err_trap_ਸਾਰਣੀ_ਬਾਹਰਲਾ_ਦਾ_Bounds);
    }
}


d_m3Op(ਸਾਰਣੀ_ਸੈੱਟ)
{
    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    ਖਾਲੀ*    ਮੁੱਲ = ਥਾਂ(ਖਾਲੀ*);
    u64      index = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);

    ਜੇ (M3_LIKELY(index < ਸਾਰਣੀ_2->ਮਾਪ)) {
        ਸਾਰਣੀ_2->elements[index] = ਮੁੱਲ;
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        newTrap(m3_Err_trap_ਸਾਰਣੀ_ਬਾਹਰਲਾ_ਦਾ_Bounds);
    }
}


d_m3Op(ਸਾਰਣੀ_ਮਾਪ)
{
    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);

    _r0 = ਸਾਰਣੀ_2->ਮਾਪ;

    ਅਗਲਾ_Op();
}


d_m3Op(ਸਾਰਣੀ_Grow)
{
    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    u64      delta = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);
    ਖਾਲੀ*    ਮੁੱਲ = ਥਾਂ(ਖਾਲੀ*);

    u32      old_ਮਾਪ = ਸਾਰਣੀ_2->ਮਾਪ;
    u32      max_ਮਾਪ = ਸਾਰਣੀ_2->max_ਮਾਪ ? ਸਾਰਣੀ_2->max_ਮਾਪ : d_m3_Max_Sane_ਸਾਰਣੀ_ਮਾਪ;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_ਮਾਪ = (delta <= max_ਮਾਪ - old_ਮਾਪ) ? (u64)old_ਮਾਪ + delta
                                                    : (u64)max_ਮਾਪ + 1;

    ਜੇ (new_ਮਾਪ == old_ਮਾਪ)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_ਮਾਪ;
        ਅਗਲਾ_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    ਜੇ (new_ਮਾਪ <= max_ਮਾਪ and (not runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ or
                                delta <= runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ - runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ)) {
        ਖਾਲੀ** elements = m3_ReallocArray(ਖਾਲੀ*, ਸਾਰਣੀ_2->elements, (size_t)new_ਮਾਪ, old_ਮਾਪ);

        ਜੇ (elements) {
            runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ += new_ਮਾਪ - old_ਮਾਪ;
            ਸਾਰਣੀ_2->elements = elements;
            ਸਾਰਣੀ_2->ਮਾਪ = (u32)new_ਮਾਪ;

            ਲਈ (u32 i = old_ਮਾਪ; i < ਸਾਰਣੀ_2->ਮਾਪ; ++i) {
                ਸਾਰਣੀ_2->elements[i] = ਮੁੱਲ;
            }

            _r0 = old_ਮਾਪ;
            ਅਗਲਾ_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    ਅਗਲਾ_Op();
}


d_m3Op(ਸਾਰਣੀ_Init)
{
    M3_ਸਾਰਣੀ*          ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    M3ElementSegment* segment = ਤੁਰੰਤ(M3ElementSegment*);
    u32               ਗਿਣਤੀ_2 = ਥਾਂ(u32);
    u32               ਸਰੋਤ = ਥਾਂ(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);

    u64               ਉਪਲਬਧ = segment->dropped ? 0 : segment->ਅੰਕ_Elements;

    ਜੇ (M3_LIKELY(destination <= ਸਾਰਣੀ_2->ਮਾਪ and ਗਿਣਤੀ_2 <= ਸਾਰਣੀ_2->ਮਾਪ - destination and
                  (u64) ਸਰੋਤ + ਗਿਣਤੀ_2 <= ਉਪਲਬਧ)) {
        ਲਈ (u32 i = 0; i < ਗਿਣਤੀ_2; ++i) {
            ਸਾਰਣੀ_2->elements[destination + i] = segment->resolved[ਸਰੋਤ + i];
        }

        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        newTrap(m3_Err_trap_ਸਾਰਣੀ_ਬਾਹਰਲਾ_ਦਾ_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = ਤੁਰੰਤ(M3ElementSegment*);
    segment->dropped = true;

    ਅਗਲਾ_Op();
}


d_m3Op(ਸਾਰਣੀ_Copy)
{
    M3_ਸਾਰਣੀ* dst = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    M3_ਸਾਰਣੀ* src = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      ਗਿਣਤੀ_2 = d_m3_Wide_ਸੰਕਾਰਜ(dst->isTable64 and src->isTable64);
    u64      ਸਰੋਤ = d_m3_Wide_ਸੰਕਾਰਜ(src->isTable64);
    u64      destination = d_m3_Wide_ਸੰਕਾਰਜ(dst->isTable64);

    ਜੇ (M3_LIKELY(destination <= dst->ਮਾਪ and ਗਿਣਤੀ_2 <= dst->ਮਾਪ - destination and
                  ਸਰੋਤ <= src->ਮਾਪ and ਗਿਣਤੀ_2 <= src->ਮਾਪ - ਸਰੋਤ)) {
        memmove(dst->elements + destination, src->elements + ਸਰੋਤ, (size_t)ਗਿਣਤੀ_2 * ਆਕਾਰ(ਖਾਲੀ*));

        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        newTrap(m3_Err_trap_ਸਾਰਣੀ_ਬਾਹਰਲਾ_ਦਾ_Bounds);
    }
}


d_m3Op(ਸਾਰਣੀ_Fill)
{
    M3_ਸਾਰਣੀ* ਸਾਰਣੀ_2 = ਤੁਰੰਤ(M3_ਸਾਰਣੀ*);
    u64      ਗਿਣਤੀ_2 = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);
    ਖਾਲੀ*    ਮੁੱਲ = ਥਾਂ(ਖਾਲੀ*);
    u64      index = d_m3_Wide_ਸੰਕਾਰਜ(ਸਾਰਣੀ_2->isTable64);

    ਜੇ (M3_LIKELY(index <= ਸਾਰਣੀ_2->ਮਾਪ and ਗਿਣਤੀ_2 <= ਸਾਰਣੀ_2->ਮਾਪ - index)) {
        ਲਈ (u64 i = 0; i < ਗਿਣਤੀ_2; ++i) {
            ਸਾਰਣੀ_2->elements[index + i] = ਮੁੱਲ;
        }

        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        newTrap(m3_Err_trap_ਸਾਰਣੀ_ਬਾਹਰਲਾ_ਦਾ_Bounds);
    }
}

#ਜੇ_ਅੰਤ // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = ਤੁਰੰਤ(IM3Function);

    m3ret_t     ਨਤੀਜਾ = m3Err_none;

    ਜੇ (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        ਨਤੀਜਾ = CompileFunction(function);
    }

    ਜੇ (not ਨਤੀਜਾ) {
        // patch up compiled pc and call rewritten op_Call
        *((ਖਾਲੀ**)--_pc) = (ਖਾਲੀ*)(function->compiled);
        --_pc;
        ਅਗਲਾ_Op_ਸਿੱਧਾ();
    }

    newTrap(ਨਤੀਜਾ);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = ਤੁਰੰਤ(IM3Function);

    m3ret_t     ਨਤੀਜਾ = m3Err_none;

    ਜੇ (M3_UNLIKELY(not function->compiled)) {
        ਨਤੀਜਾ = CompileFunction(function);
    }

    ਜੇ (not ਨਤੀਜਾ) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((ਖਾਲੀ**)--_pc) = (ਖਾਲੀ*)(function->compiled);
        --_pc;
        ਅਗਲਾ_Op_ਸਿੱਧਾ();
    }

    newTrap(ਨਤੀਜਾ);
}


#ਜੇ d_m3_Has_ਢੇਰ_Switching
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
ਸਥਿਰ
ਖਾਲੀ Suspend_Without_ਟੈਗ (IM3Runtime io_runtime, IM3Continuation io_cont, M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਕਿਸਮ i_ਕਿਸਮ,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  ਜੇ d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_ਟੈਗ = ਨਿਰਾ;
    io_runtime->suspendHandlerCont = ਨਿਰਾ;
    io_runtime->suspendStubPC = ਨਿਰਾ;
    io_runtime->switch_ਨਿਸ਼ਾਨਾ = ਨਿਰਾ;
    io_runtime->ਅੰਕ_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspend_ਬਿੰਦੂ = (u8)i_ਕਿਸਮ;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  ਜੇ d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->ਅੰਕ_Suspend_Results = 0;
}
#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


#ਜੇ d_m3HasGasMetering

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
    u32        ਲਾਗਤ = ਤੁਰੰਤ(u32);

    runtime->gas_ਬਚਿਆ -= (i64)ਲਾਗਤ;

    ਜੇ (M3_UNLIKELY(runtime->gas_ਬਚਿਆ < 0)) {
#  ਜੇ d_m3_Has_ਢੇਰ_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        ਜੇ (runtime->isSuspendable and runtime->ਸਰਗਰਮ_Continuation) {
            runtime->suspendRequested = true;
            ਅਗਲਾ_Op();
        }
#  endif
        newTrap(m3_Err_trap_ਬਾਹਰਲਾ_ਦਾ_Gas);
    }

    ਅਗਲਾ_Op();
}

#ਜੇ_ਅੰਤ // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool ਇੰਦਰਾਜ_ਸਮਾਉਂਦਾ (IM3Function i_function, m3stack_t _sp, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* _mem)
{
#ਜੇ d_m3_Skip_ਢੇਰ_Check
    (ਖਾਲੀ)i_function;
    (ਖਾਲੀ)_sp;
    (ਖਾਲੀ)_mem;
    ਵਾਪਸ true;
#ਜਾਂਜੇ d_m3_Has_ਢੇਰ_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->ਸਰਗਰਮ_Continuation : ਨਿਰਾ);
    ਖਾਲੀ*           max_ਢੇਰ = (_act && _act->val_ਢੇਰ)
                                 ? (ਖਾਲੀ*)(_act->val_ਢੇਰ + _act->ਅੰਕ_ਢੇਰ_Slots)
                                 : (_mem ? _mem->max_ਢੇਰ : (ਖਾਲੀ*)~(uintptr_t)0);
    ਵਾਪਸ M3_LIKELY((ਖਾਲੀ*)(_sp + i_function->max_ਢੇਰ_Slots) < max_ਢੇਰ);
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ M3_LIKELY((ਖਾਲੀ*)(_sp + i_function->max_ਢੇਰ_Slots) < _mem->max_ਢੇਰ);
#ਜੇ_ਅੰਤ
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ Enter_ਚੌਖਟਾ (IM3Function i_function, m3stack_t _sp)
{
#ਜੇ defined(DEBUG)
    i_function->ਪ੍ਰਾਪਤੀਆਂ++;
#ਜੇ_ਅੰਤ
    u8* ਢੇਰ = (u8*)((m3slot_t*)_sp + i_function->ਅੰਕ_Ret_And_Arg_Slots);

    memset(ਢੇਰ, 0x0, i_function->ਅੰਕ_Local_ਬਾਈਟਾਂ);
    ਢੇਰ += i_function->ਅੰਕ_Local_ਬਾਈਟਾਂ;

    ਜੇ (i_function->constants) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(ਢੇਰ, i_function->constants, i_function->ਅੰਕ_Constant_ਬਾਈਟਾਂ);
    }
}


d_m3Op(ਇੰਦਰਾਜ_2)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = ਤੁਰੰਤ(IM3Function);
#ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);
#ਜੇ_ਅੰਤ

    ਜੇ (ਇੰਦਰਾਜ_ਸਮਾਉਂਦਾ(function, _sp, _mem)) {
        Enter_ਚੌਖਟਾ(function, _sp);

#ਜੇ !d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        ਅਗਲਾ_Op_ਸਿੱਧਾ();
#ਨਹੀਂਤਾਂ

#  ਜੇ d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->ਅੰਕ_Ret_Slots));
        trace_rt->call_ਡੂੰਘਾਈ++;
#  endif

        m3ret_t r = ਅਗਲਾ_Op_Impl();

#  ਜੇ d_m3EnableStrace >= 2
        trace_rt->call_ਡੂੰਘਾਈ--;

        ਜੇ (r) {
            d_m3TracePrint("} !trap = %s", (ਅੱਖਰ*)r);
        } ਨਹੀਂਤਾਂ ਜੇ (Get_Function_ਅੰਕ_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } ਨਹੀਂਤਾਂ {
            d_m3TracePrint("}");
        }
#  endif

        ਜੇ (M3_UNLIKELY(r)) {
            _mem = ਸਮ੍ਰਿਤੀ->mallocated;
#  ਜੇ d_m3_Has_ਢੇਰ_Switching
            ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                ਵਾਪਸ ਰਿਕਾਰਡ_ਇੰਦਰਾਜ_ਚੌਖਟਾ(m3MemRuntime(_mem), _pc, _sp, ਸਮ੍ਰਿਤੀ, function);
            }
#  endif
            fill_Backtrace_ਚੌਖਟਾ();
        }
        forwardTrap(r);
#ਜੇ_ਅੰਤ // d_m3EntryKeepsFrame
    } ਨਹੀਂਤਾਂ {
        newTrap(m3_Err_trap_ਢੇਰ_ਉਛਾਲ);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_ਢੇਰ();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    ਕਰੋ {
#ਜੇ d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_ਡੂੰਘਾਈ++;
#ਜੇ_ਅੰਤ
        r = ਅਗਲਾ_Op_Impl();

#ਜੇ d_m3EnableStrace >= 3
        trace_rt->call_ਡੂੰਘਾਈ--;
        d_m3TracePrint("}");
#ਜੇ_ਅੰਤ
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = ਸਮ੍ਰਿਤੀ->mallocated;
    } ਜਦੋਂ (r == _pc);

#ਜੇ d_m3_Has_ਢੇਰ_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        ਵਾਪਸ ਰਿਕਾਰਡ_Loop_ਚੌਖਟਾ(m3MemRuntime(_mem), _pc, _sp, ਸਮ੍ਰਿਤੀ);
    }
#ਜੇ_ਅੰਤ

    forwardTrap(r);
}


#ਜੇ d_m3HasExceptionHandling

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
d_m3Op(Try_ਸਾਰਣੀ)
{
    d_m3_Check_Native_ਢੇਰ();

    IM3_ਸਮ੍ਰਿਤੀ  ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        ਅੰਕ_Clauses = ਤੁਰੰਤ(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * ਅੰਕ_Clauses;

    u32        ਡੂੰਘਾਈ = ++runtime->try_ਡੂੰਘਾਈ;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = ਸਮ੍ਰਿਤੀ->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_ਡੂੰਘਾਈ == ਡੂੰਘਾਈ);
    runtime->try_ਡੂੰਘਾਈ = ਡੂੰਘਾਈ - 1;

#  ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        ਵਾਪਸ ਰਿਕਾਰਡ_Try_ਚੌਖਟਾ(runtime, clauses, _sp, ਸਮ੍ਰਿਤੀ, ਅੰਕ_Clauses, isInnermost);
    }
#  endif

    ਜੇ (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ਅਚਲ M3Exception* exception = runtime->pendingException;

        ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; ++i) {
            IM3_ਟੈਗ ਟੈਗ = (IM3_ਟੈਗ)clauses[2 * i];

            ਜੇ (ਟੈਗ == ਨਿਰਾ or ਟੈਗ == exception->ਟੈਗ) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jump_Op_ਸਿੱਧਾ(handler);
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

    u32          ਅੰਕ_ਦਲੀਲਾਂ = ਤੁਰੰਤ(u32);

    ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        i32 offset = ਤੁਰੰਤ(i32);
        u32 is64 = ਤੁਰੰਤ(u32);
        u64 ਮੁੱਲ = exception->ਦਲੀਲਾਂ[i];

        ਜੇ (is64) {
            *(u64*)(_sp + offset) = ਮੁੱਲ;
        } ਨਹੀਂਤਾਂ {
            *(u32*)(_sp + offset) = (u32)ਮੁੱਲ;
        }
    }

    i32 ref_ਵਿੱਥ = ਤੁਰੰਤ(i32);

    ਜੇ (ref_ਵਿੱਥ >= 0) {
        *(M3Exception**)(_sp + ref_ਵਿੱਥ) = exception;
        exception->reified = true;
    } ਨਹੀਂਤਾਂ ਜੇ (not exception->reified) {
        FreeException(runtime, exception);
    }

    ਅਗਲਾ_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_ਟੈਗ       ਟੈਗ = ਤੁਰੰਤ(IM3_ਟੈਗ);
    u32          ਅੰਕ_ਦਲੀਲਾਂ = ਤੁਰੰਤ(u32);

    M3Exception* exception = NewException(runtime, ਟੈਗ, ਅੰਕ_ਦਲੀਲਾਂ);

    ਜੇ (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        i32 offset = ਤੁਰੰਤ(i32);
        u32 is64 = ਤੁਰੰਤ(u32);

        exception->ਦਲੀਲਾਂ[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    ਵਾਪਸ m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = ਥਾਂ(M3Exception*);

    ਜੇ (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    ਵਾਪਸ m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(ਕੱਢੋ_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        ਗਿਣਤੀ_2 = ਤੁਰੰਤ(u32);

    runtime->try_ਡੂੰਘਾਈ -= ਗਿਣਤੀ_2;

    ਅਗਲਾ_Op();
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling

#ਜੇ d_m3_Has_ਢੇਰ_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = ਤੁਰੰਤ(IM3FuncType);
    i32         dst_ਥਾਂ = ਤੁਰੰਤ(i32);
    i32         func_ਥਾਂ = ਤੁਰੰਤ(i32);

    IM3Function func = *(IM3Function*)(_sp + func_ਥਾਂ);
    ਜੇ (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    ਜੇ (M3_UNLIKELY(not Continuation_Can_Acquire_ਢੇਰ(runtime))) {
        newTrap(m3_Err_continuation_ਹੱਦਬੰਦੀ_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    ਜੇ (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_ਨਤੀਜਾ ਵੰਡ = Continuation_Acquire_ਢੇਰ(runtime, cont);
    ਜੇ (M3_UNLIKELY(ਵੰਡ)) {
        cont->state = cont_consumed;
        newTrap(ਵੰਡ);
    }
    *(IM3Continuation*)(_sp + dst_ਥਾਂ) = cont;
    ਅਗਲਾ_Op();
}


ਸਥਿਰ
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = ਤੁਰੰਤ(IM3FuncType);
    IM3FuncType     ct2 = ਤੁਰੰਤ(IM3FuncType);
    i32             dst_ਥਾਂ = ਤੁਰੰਤ(i32);
    i32             cont_ਥਾਂ = ਤੁਰੰਤ(i32);
    u32             ਅੰਕ_Bound = ਤੁਰੰਤ(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + cont_ਥਾਂ);
    ਜੇ (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ਜੇ (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->ਇੰਦਰਾਜ_Function);
    ਜੇ (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->ਇੰਦਰਾਜ_Function = srcCont->ਇੰਦਰਾਜ_Function;
    dstCont->val_ਢੇਰ = srcCont->val_ਢੇਰ;
    dstCont->ਅੰਕ_ਢੇਰ_Slots = srcCont->ਅੰਕ_ਢੇਰ_Slots;
    dstCont->ਚੌਖਟੇ = srcCont->ਚੌਖਟੇ;
    dstCont->ਚੌਖਟੇ_Cap = srcCont->ਚੌਖਟੇ_Cap;
    dstCont->ਅੰਕ_ਚੌਖਟੇ = srcCont->ਅੰਕ_ਚੌਖਟੇ;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspend_ਬਿੰਦੂ = srcCont->suspend_ਬਿੰਦੂ;
    dstCont->r0 = srcCont->r0;
#  ਜੇ d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = srcCont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ;
    dstCont->state = srcCont->state;
#  ਜੇ d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = ਨਿਰਾ;
#  endif

    dstCont->ਅੰਕ_Suspend_Results = srcCont->ਅੰਕ_Suspend_Results;
    ਲਈ (u32 j = 0; j < srcCont->ਅੰਕ_Suspend_Results; ++j) {
        dstCont->suspend_ਨਤੀਜਾ_Offsets[j] = srcCont->suspend_ਨਤੀਜਾ_Offsets[j];
        dstCont->suspend_ਨਤੀਜਾ_Is64[j] = srcCont->suspend_ਨਤੀਜਾ_Is64[j];
    }

    ਜੇ (dstCont->state == cont_allocated) {
        u16 ਅੰਕ_Rets = ct1->contFuncType ? Get_Func_Type_ਅੰਕ_Results(ct1->contFuncType) : 0;
        ਲਈ (u32 i = 0; i < ਅੰਕ_Bound; ++i) {
            i32 arg_ਥਾਂ = ਤੁਰੰਤ(i32);
            u32 is64 = ਤੁਰੰਤ(u32);
            u32 ਨਿਸ਼ਾਨਾ_Idx = (ਅੰਕ_Rets + dstCont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ) * c_io_ਥਾਂ_ਗਿਣਤੀ;
            ਜੇ (is64) {
                *(u64*)(dstCont->val_ਢੇਰ + ਨਿਸ਼ਾਨਾ_Idx) = *(u64*)(_sp + arg_ਥਾਂ);
            } ਨਹੀਂਤਾਂ {
                *(u32*)(dstCont->val_ਢੇਰ + ਨਿਸ਼ਾਨਾ_Idx) = *(u32*)(_sp + arg_ਥਾਂ);
            }
            dstCont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ++;
        }
    } ਨਹੀਂਤਾਂ {
        IM3Continuation inner = InnermostSuspended(dstCont);
        ਲਈ (u32 i = 0; i < ਅੰਕ_Bound; ++i) {
            i32 arg_ਥਾਂ = ਤੁਰੰਤ(i32);
            u32 is64 = ਤੁਰੰਤ(u32);
            u32 dstIdx = dstCont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ;
            ਜੇ (dstIdx < inner->ਅੰਕ_Suspend_Results) {
                i32 dst_ਵਿੱਥ = inner->suspend_ਨਤੀਜਾ_Offsets[dstIdx];
                ਜੇ (is64) {
                    *(u64*)(inner->sp + dst_ਵਿੱਥ) = *(u64*)(_sp + arg_ਥਾਂ);
                } ਨਹੀਂਤਾਂ {
                    *(u32*)(inner->sp + dst_ਵਿੱਥ) = *(u32*)(_sp + arg_ਥਾਂ);
                }
            }
            dstCont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ++;
        }
    }

    srcCont->val_ਢੇਰ = ਨਿਰਾ;
    srcCont->ਚੌਖਟੇ = ਨਿਰਾ;
    srcCont->ਚੌਖਟੇ_Cap = 0;
    srcCont->ਅੰਕ_ਚੌਖਟੇ = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dst_ਥਾਂ) = dstCont;
    ਅਗਲਾ_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
ਸਥਿਰ
IM3Continuation ਲੱਭੋ_Handler (IM3Continuation i_cont, IM3_ਟੈਗ i_ਟੈਗ, u8 i_ਕਿਸਮ, pc_t* o_stubPC)
{
    IM3_ਟੈਗ query_ਟੈਗ = (i_ਟੈਗ and i_ਟੈਗ->resolved) ? i_ਟੈਗ->resolved : i_ਟੈਗ;

    ਲਈ (IM3Continuation cont = i_cont; cont; cont = cont->ਮਾਪਾ) {
        ਲਈ (u32 h = 0; h < cont->ਅੰਕ_Handlers; ++h) {
            pc_t   ਇੰਦਰਾਜ = cont->handlersPC + (h * 3);
            IM3_ਟੈਗ handler_ਟੈਗ = (IM3_ਟੈਗ)(*(ਇੰਦਰਾਜ + 1));
            ਜੇ (handler_ਟੈਗ and handler_ਟੈਗ->resolved) {
                handler_ਟੈਗ = handler_ਟੈਗ->resolved;
            }

            ਜੇ ((u32)(uintptr_t)(*ਇੰਦਰਾਜ) == i_ਕਿਸਮ and handler_ਟੈਗ == query_ਟੈਗ) {
                ਜੇ (o_stubPC) {
                    *o_stubPC = (pc_t)(*(ਇੰਦਰਾਜ + 2));
                }
                ਵਾਪਸ cont;
            }
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
ਸਥਿਰ
ਖਾਲੀ ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    ਲਈ (u32 i = 0; i < io_cont->ਅੰਕ_Suspend_Results; ++i) {
        ਜੇ (io_cont->suspend_ਨਤੀਜਾ_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_ਨਤੀਜਾ_Offsets[i]) = 0;
        } ਨਹੀਂਤਾਂ {
            *(m3slot_t*)(i_sp + io_cont->suspend_ਨਤੀਜਾ_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->ਸਰਗਰਮ_Continuation;
    ਜੇ (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ਟੈਗ);
    }

    IM3_ਟੈਗ ਟੈਗ = ਤੁਰੰਤ(IM3_ਟੈਗ);
    u32    ਅੰਕ_Params = ਤੁਰੰਤ(u32);

    runtime->ਅੰਕ_Suspend_Payload = ਅੰਕ_Params;
    ਲਈ (u32 i = 0; i < ਅੰਕ_Params; ++i) {
        i32 param_ਥਾਂ = ਤੁਰੰਤ(i32);
        u32 is64 = ਤੁਰੰਤ(u32);
        ਜੇ (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + param_ਥਾਂ);
        } ਨਹੀਂਤਾਂ {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + param_ਥਾਂ);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 ਅੰਕ_Results = ਤੁਰੰਤ(u32);
    cont->ਅੰਕ_Suspend_Results = ਅੰਕ_Results;
    ਲਈ (u32 i = 0; i < ਅੰਕ_Results; ++i) {
        cont->suspend_ਨਤੀਜਾ_Offsets[i] = ਤੁਰੰਤ(i32);
        cont->suspend_ਨਤੀਜਾ_Is64[i] = ਤੁਰੰਤ(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = ਨਿਰਾ;
    IM3Continuation ਖੋਜ_Cont = ਲੱਭੋ_Handler(cont, ਟੈਗ, 0, &matchedStubPC);

    ਜੇ (M3_UNLIKELY(not ਖੋਜ_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ਟੈਗ);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = ਖੋਜ_Cont;
    runtime->switch_ਨਿਸ਼ਾਨਾ = ਨਿਰਾ;
    runtime->suspend_ਟੈਗ = ਟੈਗ;

    cont->pc = _pc;
    cont->suspend_ਬਿੰਦੂ = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  ਜੇ d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = 0;

    ਵਾਪਸ m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
ਸਥਿਰ
m3ret_t ਲੜੀਵਾਰ_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_ਅੰਕ_Handlers, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ** io_mem);
ਸਥਿਰ
ਖਾਲੀ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_ਅੰਕ_Results);
ਸਥਿਰ
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    ਵਾਪਸ pass and (i_cont->suspend_ਬਿੰਦੂ == safepoint_op or i_cont->suspend_ਬਿੰਦੂ == safepoint_ਇੰਦਰਾਜ);
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
ਸਥਿਰ
m3ret_t Replay_ਚੌਖਟੇ (IM3Continuation i_cont, i32 i_ਡੂੰਘਾਈ, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  ਜੇ d_m3_Max_Native_ਢੇਰ > 0
    {
        ਖਾਲੀ* ਹੱਦਬੰਦੀ = runtime->ਢੇਰ_ਹੱਦਬੰਦੀ;

        ਜੇ (M3_UNLIKELY(ਹੱਦਬੰਦੀ and m3_Native_ਢੇਰ_Ptr() < ਹੱਦਬੰਦੀ)) {
            ਵਾਪਸ m3_Err_trap_ਢੇਰ_ਉਛਾਲ;
        }
    }
#  endif

    ਜੇ (i_ਡੂੰਘਾਈ < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  ਜੇ d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        ਜੇ (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = ਨਿਰਾ;

            ਵਾਪਸ m3Err_pendingException;
        }
#  endif
        ਜੇ (pastCheck) {
            ਵਾਪਸ ResumePastCheck(i_cont, _mem);
        }

        ਵਾਪਸ d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3_ਚੌਖਟਾ ਚੌਖਟਾ = i_cont->ਚੌਖਟੇ[i_ਡੂੰਘਾਈ];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_ਚੌਖਟਾ_ਕਿਸਮ)ਚੌਖਟਾ.ਕਿਸਮ == ਚੌਖਟਾ_resume);

#  ਜੇ d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_ਚੌਖਟਾ_ਕਿਸਮ)ਚੌਖਟਾ.ਕਿਸਮ == ਚੌਖਟਾ_try);
#  endif

    ਜੇ (not ownDescent) {
        r = Replay_ਚੌਖਟੇ(i_cont, i_ਡੂੰਘਾਈ - 1, _mem);
        _mem = ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ->mallocated;
    }

    ਚੋਣ ((M3_ਚੌਖਟਾ_ਕਿਸਮ)ਚੌਖਟਾ.ਕਿਸਮ) {
    ਹਾਲਤ ਚੌਖਟਾ_call:
        ਜੇ (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            ਵਾਪਸ d_m3CallWithRegs(ਚੌਖਟਾ.pc, ਚੌਖਟਾ.sp, _mem, ਚੌਖਟਾ.call.r0, ਚੌਖਟਾ.call.fp0);
        }

        ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(runtime, &ਚੌਖਟਾ);
        }
#  ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
        ਧੱਕੋ_Backtrace_ਚੌਖਟਾ(runtime, ਚੌਖਟਾ.pc - 1);
#  endif
        ਵਾਪਸ r;

    ਹਾਲਤ ਚੌਖਟਾ_loop: {
        m3stack_t _sp = ਚੌਖਟਾ.sp;
        m3reg_t   _r0 = 0;
#  ਜੇ d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        ਜਦੋਂ (r == (m3ret_t)ਚੌਖਟਾ.pc) {
            r = jumpOpImpl(ਚੌਖਟਾ.pc);
            _mem = ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ->mallocated;
        }

        ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(runtime, &ਚੌਖਟਾ);
        }
        ਵਾਪਸ r;
    }

#  ਜੇ d_m3HasExceptionHandling
    ਹਾਲਤ ਚੌਖਟਾ_try: {
        m3stack_t _sp = ਚੌਖਟਾ.sp;
        m3reg_t   _r0 = 0;
#    ਜੇ d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (ਖਾਲੀ)_sp;
        (ਖਾਲੀ)_r0;
#    ਜੇ d_m3HasFloat
        (ਖਾਲੀ)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 ਡੂੰਘਾਈ = ++runtime->try_ਡੂੰਘਾਈ;

        ਜੇ (not ਚੌਖਟਾ.try_.handlers_ਜੀਵਤ) {
            runtime->try_ਡੂੰਘਾਈ = ਡੂੰਘਾਈ - 1;
        }

        r = Replay_ਚੌਖਟੇ(i_cont, i_ਡੂੰਘਾਈ - 1, _mem);
        _mem = ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ->mallocated;

        bool isInnermost = (runtime->try_ਡੂੰਘਾਈ == ਡੂੰਘਾਈ);
        runtime->try_ਡੂੰਘਾਈ = ਡੂੰਘਾਈ - 1;

        ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ਚੌਖਟਾ.try_.handlers_ਜੀਵਤ = isInnermost;
            ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(runtime, &ਚੌਖਟਾ);
        }

        ਜੇ (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ਅਚਲ M3Exception* exception = runtime->pendingException;

            ਲਈ (u32 i = 0; i < ਚੌਖਟਾ.try_.ਅੰਕ_Clauses; ++i) {
                IM3_ਟੈਗ ਟੈਗ = (IM3_ਟੈਗ)ਚੌਖਟਾ.pc[2 * i];

                ਜੇ (ਟੈਗ == ਨਿਰਾ or ਟੈਗ == exception->ਟੈਗ) {
                    pc_t handler = (pc_t)ਚੌਖਟਾ.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    ਵਾਪਸ jumpOpImpl(handler);
                }
            }
        }
        ਵਾਪਸ r;
    }
#  endif

    ਹਾਲਤ ਚੌਖਟਾ_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = ਚੌਖਟਾ.resume.cont;

        r = ਲੜੀਵਾਰ_Under_Handlers(runtime, &inner, ਚੌਖਟਾ.resume.handlersPC,
                             ਚੌਖਟਾ.resume.ਅੰਕ_Handlers, &_mem);

        ਜੇ (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, ਚੌਖਟਾ.sp, ਚੌਖਟਾ.resume.resultsPC,
                                    ਚੌਖਟਾ.resume.ਅੰਕ_Results);
            Continuation_Recycle_ਢੇਰ(runtime, inner);

            ਵਾਪਸ d_m3CallWithRegs(ਚੌਖਟਾ.pc, ਚੌਖਟਾ.sp, _mem, 0, 0.);
        }

        ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ਜੇ (runtime->suspendHandlerCont != inner) {
                ਚੌਖਟਾ.resume.cont = inner;
                ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(runtime, &ਚੌਖਟਾ);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            ਜੇ (M3_UNLIKELY(not suspended)) {
                ਵਾਪਸ m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            ਜੇ (runtime->suspendStubPC) {
                ਵਾਪਸ d_m3CallWithRegs(runtime->suspendStubPC, ਚੌਖਟਾ.sp, _mem, 0, 0.);
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ m3_Err_trap_Unhandled_Control_ਟੈਗ;
            }
        } ਨਹੀਂਤਾਂ {
            inner->state = cont_consumed;
            Continuation_Recycle_ਢੇਰ(runtime, inner);
        }

        ਵਾਪਸ r;
    }

#  ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
    ਹਾਲਤ ਚੌਖਟਾ_ਇੰਦਰਾਜ:
        ਜੇ (M3_UNLIKELY(r)) {
            ਜੇ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(runtime, &ਚੌਖਟਾ);
            }
#    ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
            FillBacktraceFunctionInfo(runtime, ਚੌਖਟਾ.ਇੰਦਰਾਜ.function);
#    ਨਹੀਂਤਾਂ
            (ਖਾਲੀ)ਚੌਖਟਾ.ਇੰਦਰਾਜ.function;
#    endif
        }
        ਵਾਪਸ r;
#  endif
    }

    ਵਾਪਸ r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
ਸਥਿਰ
ਖਾਲੀ* ContinuationArgDest (IM3Continuation i_cont, u32 i_ਸੂਚਕਾਂਕ)
{
    ਜੇ (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : ਨਿਰਾ;
        u16         ਅੰਕ_Rets = inner ? Get_Func_Type_ਅੰਕ_Results(inner) : 0;

        ਵਾਪਸ i_cont->val_ਢੇਰ + (ਅੰਕ_Rets + i_ਸੂਚਕਾਂਕ) * c_io_ਥਾਂ_ਗਿਣਤੀ;
    }

    ਜੇ (i_ਸੂਚਕਾਂਕ < i_cont->ਅੰਕ_Suspend_Results) {
        ਵਾਪਸ i_cont->sp + i_cont->suspend_ਨਤੀਜਾ_Offsets[i_ਸੂਚਕਾਂਕ];
    }

    ਵਾਪਸ ਨਿਰਾ;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
ਸਥਿਰ
ਖਾਲੀ Hand_Over_ਮੰਜ਼ਿਲ_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 ਅੰਕ_ਦਲੀਲਾਂ = i_runtime->ਅੰਕ_Suspend_Payload;
    u32 ਅਧਾਰ = (io_peer->state == cont_allocated) ? io_peer->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ : 0;

    ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        ਖਾਲੀ* dest = ContinuationArgDest(io_peer, ਅਧਾਰ + i);

        ਜੇ (dest) {
            ਜੇ (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } ਨਹੀਂਤਾਂ {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    ਖਾਲੀ* refDest = ContinuationArgDest(io_peer, ਅਧਾਰ + ਅੰਕ_ਦਲੀਲਾਂ);

    ਜੇ (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    ਜੇ (io_peer->state == cont_allocated) {
        io_peer->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ += ਅੰਕ_ਦਲੀਲਾਂ + 1;
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
ਸਥਿਰ
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    ਜਦੋਂ (i_cont->ਅੰਕ_ਚੌਖਟੇ and (M3_ਚੌਖਟਾ_ਕਿਸਮ) i_cont->ਚੌਖਟੇ[0].ਕਿਸਮ == ਚੌਖਟਾ_resume) {
        i_cont = i_cont->ਚੌਖਟੇ[0].resume.cont;
    }

    ਵਾਪਸ i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
ਸਥਿਰ
m3ret_t ਲੜੀਵਾਰ_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_ਅੰਕ_Handlers, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation ਮਾਪਾ = i_runtime->ਸਰਗਰਮ_Continuation;
    IM3_ਸਮ੍ਰਿਤੀ       ਸਮ੍ਰਿਤੀ = m3MemInfo(*io_mem);
    m3ret_t         r;

    ਲਈ (;;) {
#  ਜੇ d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        ਜੇ (M3_UNLIKELY(cont->resumeThrow and cont->ਅੰਕ_ਚੌਖਟੇ == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = ਨਿਰਾ;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            ਰੋਕੋ;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        ਜੇ (isStarting) {
            ਜੇ (M3_UNLIKELY(not cont->ਇੰਦਰਾਜ_Function->compiled)) {
                r = CompileFunction(cont->ਇੰਦਰਾਜ_Function);

                ਜੇ (M3_UNLIKELY(r)) {
                    ਰੋਕੋ;
                }
            }
            cont->pc = cont->ਇੰਦਰਾਜ_Function->compiled;
            cont->sp = cont->val_ਢੇਰ;
            cont->ਅੰਕ_ਚੌਖਟੇ = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->ਅੰਕ_Handlers = i_ਅੰਕ_Handlers;
        cont->ਮਾਪਾ = ਮਾਪਾ;
        cont->state = cont_running;
        cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = 0;

        i_runtime->ਸਰਗਰਮ_Continuation = cont;

        ਜੇ (cont->ਅੰਕ_ਚੌਖਟੇ) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->ਅੰਕ_ਚੌਖਟੇ - 1;
            cont->ਅੰਕ_ਚੌਖਟੇ = 0;

            r = Replay_ਚੌਖਟੇ(cont, outermost, *io_mem);
        } ਨਹੀਂਤਾਂ ਜੇ (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } ਨਹੀਂਤਾਂ {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = ਸਮ੍ਰਿਤੀ->mallocated;
        i_runtime->ਸਰਗਰਮ_Continuation = ਮਾਪਾ;

        ਜੇ (r != m3Err_continuationSuspended) {
            ਰੋਕੋ;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        ਜੇ (not i_runtime->switch_ਨਿਸ਼ਾਨਾ or i_runtime->suspendHandlerCont != cont) {
            ਰੋਕੋ;
        }

        IM3Continuation peer = i_runtime->switch_ਨਿਸ਼ਾਨਾ;
        i_runtime->switch_ਨਿਸ਼ਾਨਾ = ਨਿਰਾ;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        ਜੇ (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            ਰੋਕੋ;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_Cont_Type_ਦਾ_Ref(peer->ਇੰਦਰਾਜ_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_ਅੰਕ_Params(peerType) - 1)));
        Hand_Over_ਮੰਜ਼ਿਲ_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    ਵਾਪਸ r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
ਸਥਿਰ
ਖਾਲੀ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_ਅੰਕ_Results)
{
    ਲਈ (u32 i = 0; i < i_ਅੰਕ_Results; ++i) {
        i32 res_ਥਾਂ = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_ਥਾਂ_ਗਿਣਤੀ;

        ਜੇ (is64) {
            *(u64*)(i_sp + res_ਥਾਂ) = *(u64*)(i_cont->val_ਢੇਰ + srcIdx);
        } ਨਹੀਂਤਾਂ {
            *(u32*)(i_sp + res_ਥਾਂ) = *(u32*)(i_cont->val_ਢੇਰ + srcIdx);
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
    IM3FuncType     contFuncType = ਤੁਰੰਤ(IM3FuncType);
    i32             cont_ਥਾਂ = ਤੁਰੰਤ(i32);
    u32             ਬਹੁਲਕ = ਤੁਰੰਤ(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + cont_ਥਾਂ);
    ਜੇ (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ਜੇ (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         ਅੰਕ_Rets = Get_Func_Type_ਅੰਕ_Results(innerType);

    ਜੇ (ਬਹੁਲਕ == d_m3ResumeThrowRef) {
        i32 exn_ਥਾਂ = ਤੁਰੰਤ(i32);

#  ਜੇ d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exn_ਥਾਂ);

        ਜੇ (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  ਨਹੀਂਤਾਂ
        (ਖਾਲੀ)exn_ਥਾਂ;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } ਨਹੀਂਤਾਂ ਜੇ (ਬਹੁਲਕ == d_m3ResumeThrow) {
        IM3_ਟੈਗ exn_ਟੈਗ = ਤੁਰੰਤ(IM3_ਟੈਗ);
        u32    ਅੰਕ_ਦਲੀਲਾਂ = ਤੁਰੰਤ(u32);

#  ਜੇ d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_ਟੈਗ, ਅੰਕ_ਦਲੀਲਾਂ);

        ਜੇ (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
            i32 offset = ਤੁਰੰਤ(i32);
            u32 is64 = ਤੁਰੰਤ(u32);

            exception->ਦਲੀਲਾਂ[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  ਨਹੀਂਤਾਂ
        (ਖਾਲੀ)exn_ਟੈਗ;
        (ਖਾਲੀ)ਅੰਕ_ਦਲੀਲਾਂ;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } ਨਹੀਂਤਾਂ {
        u32 ਅੰਕ_ਦਲੀਲਾਂ = ਤੁਰੰਤ(u32);

        ਜੇ (cont->state == cont_allocated) {
            ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
                i32 arg_ਥਾਂ = ਤੁਰੰਤ(i32);
                u32 is64 = ਤੁਰੰਤ(u32);
                u32 ਨਿਸ਼ਾਨਾ_Idx = (ਅੰਕ_Rets + cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ + i) * c_io_ਥਾਂ_ਗਿਣਤੀ;
                ਜੇ (is64) {
                    *(u64*)(cont->val_ਢੇਰ + ਨਿਸ਼ਾਨਾ_Idx) = *(u64*)(_sp + arg_ਥਾਂ);
                } ਨਹੀਂਤਾਂ {
                    *(u32*)(cont->val_ਢੇਰ + ਨਿਸ਼ਾਨਾ_Idx) = *(u32*)(_sp + arg_ਥਾਂ);
                }
            }
            cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = 0;
        } ਨਹੀਂਤਾਂ {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
                i32 arg_ਥਾਂ = ਤੁਰੰਤ(i32);
                u32 is64 = ਤੁਰੰਤ(u32);
                u32 dstIdx = cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ + i;
                ਜੇ (dstIdx < inner->ਅੰਕ_Suspend_Results) {
                    i32 dst_ਵਿੱਥ = inner->suspend_ਨਤੀਜਾ_Offsets[dstIdx];
                    ਜੇ (is64) {
                        *(u64*)(inner->sp + dst_ਵਿੱਥ) = *(u64*)(_sp + arg_ਥਾਂ);
                    } ਨਹੀਂਤਾਂ {
                        *(u32*)(inner->sp + dst_ਵਿੱਥ) = *(u32*)(_sp + arg_ਥਾਂ);
                    }
                }
            }
        }
    }

    u32  ਅੰਕ_Handlers = ਤੁਰੰਤ(u32);
    pc_t handlersPC = _pc;
    _pc += ਅੰਕ_Handlers * 3;

    u32       ਅੰਕ_Results = ਤੁਰੰਤ(u32);
    pc_t      resultsPC = _pc;

    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = m3MemInfo(_mem);

    m3ret_t   r = ਲੜੀਵਾਰ_Under_Handlers(runtime, &cont, handlersPC, ਅੰਕ_Handlers, &_mem);

    ਜੇ (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, ਅੰਕ_Results);
        Continuation_Recycle_ਢੇਰ(runtime, cont);

        _pc += ਅੰਕ_Results * 2;
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ ਜੇ (r == m3Err_continuationSuspended) {
        ਜੇ (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            ਜੇ (M3_UNLIKELY(not runtime->ਸਰਗਰਮ_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_ਟੈਗ);
            }
            M3_ਚੌਖਟਾ ਚੌਖਟਾ;
            ਚੌਖਟਾ.ਕਿਸਮ = ਚੌਖਟਾ_resume;
            ਚੌਖਟਾ.pc = resultsPC + ਅੰਕ_Results * 2;
            ਚੌਖਟਾ.sp = _sp;
            ਚੌਖਟਾ.ਸਮ੍ਰਿਤੀ = ਸਮ੍ਰਿਤੀ;
            ਚੌਖਟਾ.resume.cont = cont;
            ਚੌਖਟਾ.resume.handlersPC = handlersPC;
            ਚੌਖਟਾ.resume.ਅੰਕ_Handlers = ਅੰਕ_Handlers;
            ਚੌਖਟਾ.resume.resultsPC = resultsPC;
            ਚੌਖਟਾ.resume.ਅੰਕ_Results = ਅੰਕ_Results;

            ਵਾਪਸ Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ(runtime, &ਚੌਖਟਾ);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        ਜੇ (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        ਜੇ (runtime->suspendStubPC) {
            jump_Op_ਸਿੱਧਾ(runtime->suspendStubPC);
        } ਨਹੀਂਤਾਂ {
            newTrap(m3_Err_trap_Unhandled_Control_ਟੈਗ);
        }
    } ਨਹੀਂਤਾਂ {
        cont->state = cont_consumed;
        Continuation_Recycle_ਢੇਰ(runtime, cont);
        ਧੱਕੋ_Backtrace_ਚੌਖਟਾ_2();
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
    cont->type = ਤੁਰੰਤ(IM3FuncType);

    u32 ਅੰਕ_ਦਲੀਲਾਂ = ਤੁਰੰਤ(u32);
    ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        i32 offset = ਤੁਰੰਤ(i32);
        u32 is64 = ਤੁਰੰਤ(u32);
        u64 val = (i < runtime->ਅੰਕ_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        ਜੇ (is64) {
            *(u64*)(_sp + offset) = val;
        } ਨਹੀਂਤਾਂ {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_ਵਿੱਥ = ਤੁਰੰਤ(i32);
    ਜੇ (cont_ਵਿੱਥ >= 0) {
        *(IM3Continuation*)(_sp + cont_ਵਿੱਥ) = cont;
    }

    ਅਗਲਾ_Op();
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
    IM3Continuation cont = runtime->ਸਰਗਰਮ_Continuation;

    ਜੇ (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ਟੈਗ);
    }

    skip_ਤੁਰੰਤ(IM3FuncType);

    IM3_ਟੈਗ          ਟੈਗ = ਤੁਰੰਤ(IM3_ਟੈਗ);
    i32             ਨਿਸ਼ਾਨਾ_ਥਾਂ = ਤੁਰੰਤ(i32);

    IM3Continuation ਨਿਸ਼ਾਨਾ = *(IM3Continuation*)(_sp + ਨਿਸ਼ਾਨਾ_ਥਾਂ);

    ਜੇ (M3_UNLIKELY(not ਨਿਸ਼ਾਨਾ)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ਜੇ (M3_UNLIKELY(ਨਿਸ਼ਾਨਾ->state == cont_consumed or ਨਿਸ਼ਾਨਾ->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = ਲੱਭੋ_Handler(cont, ਟੈਗ, 1, ਨਿਰਾ);

    ਜੇ (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ਟੈਗ);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 ਅੰਕ_ਦਲੀਲਾਂ = ਤੁਰੰਤ(u32);

    runtime->ਅੰਕ_Suspend_Payload = ਅੰਕ_ਦਲੀਲਾਂ;

    ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        i32 arg_ਥਾਂ = ਤੁਰੰਤ(i32);
        u32 is64 = ਤੁਰੰਤ(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + arg_ਥਾਂ) : (u64) * (u32*)(_sp + arg_ਥਾਂ);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 ਅੰਕ_Results = ਤੁਰੰਤ(u32);

    cont->ਅੰਕ_Suspend_Results = ਅੰਕ_Results;

    ਲਈ (u32 i = 0; i < ਅੰਕ_Results; ++i) {
        cont->suspend_ਨਤੀਜਾ_Offsets[i] = ਤੁਰੰਤ(i32);
        cont->suspend_ਨਤੀਜਾ_Is64[i] = ਤੁਰੰਤ(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = ਨਿਰਾ;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switch_ਨਿਸ਼ਾਨਾ = ਨਿਸ਼ਾਨਾ;
    runtime->suspend_ਟੈਗ = ਟੈਗ;

    cont->pc = _pc;
    cont->suspend_ਬਿੰਦੂ = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  ਜੇ d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    ਵਾਪਸ m3Err_continuationSuspended;
}


#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = ਤੁਰੰਤ(pc_t);

    ਜੇ (condition) {
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = ਥਾਂ(i32);

    pc_t elsePC = ਤੁਰੰਤ(pc_t);

    ਜੇ (condition) {
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_ਸਾਰਣੀ)
{
    u32   branch_ਸੂਚਕਾਂਕ = ਥਾਂ(u32);           // branch index is always in a slot
    u32   ਅੰਕ_Targets = ਤੁਰੰਤ(u32);

    pc_t* branches = (pc_t*)_pc;

    ਜੇ (branch_ਸੂਚਕਾਂਕ > ਅੰਕ_Targets) {
        branch_ਸੂਚਕਾਂਕ = ਅੰਕ_Targets; // the default index
    }

    jumpOp(branches[branch_ਸੂਚਕਾਂਕ]);
}


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਸੈੱਟ_ਰਜਿਸਟਰ_ਸੈੱਟ_ਥਾਂ(TYPE, REG) \
d_m3Op  (ਸੈੱਟ_ਰਜਿਸਟਰ_2##TYPE)            \
{                                       \
    TYPE * src = ਥਾਂ_ptr (TYPE);       \
    d_m3_Preload_ਅਗਲਾ ();                 \
    REG = * src;                        \
    ਅਗਲਾ_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (ਸੈੱਟ_ਥਾਂ_2##TYPE)                 \
{                                       \
    TYPE * dst = ਥਾਂ_ptr (TYPE);       \
    d_m3_Preload_ਅਗਲਾ ();                 \
    * dst = (TYPE) REG;                 \
    ਅਗਲਾ_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_ਸੈੱਟ_ਥਾਂ_2##TYPE)         \
{                                       \
    TYPE * ਢੇਰ     = ਥਾਂ_ptr (TYPE); \
    TYPE * preserve  = ਥਾਂ_ptr (TYPE); \
    d_m3_Preload_ਅਗਲਾ ();                 \
                                        \
    * preserve = * ਢੇਰ;               \
    * ਢੇਰ = (TYPE) REG;               \
                                        \
    ਅਗਲਾ_Op_Preloaded ();                 \
}

d_m3_ਸੈੱਟ_ਰਜਿਸਟਰ_ਸੈੱਟ_ਥਾਂ(i32, _r0)
d_m3_ਸੈੱਟ_ਰਜਿਸਟਰ_ਸੈੱਟ_ਥਾਂ(i64, _r0)
#ਜੇ d_m3HasFloat
d_m3_ਸੈੱਟ_ਰਜਿਸਟਰ_ਸੈੱਟ_ਥਾਂ(f32, _fp0)
d_m3_ਸੈੱਟ_ਰਜਿਸਟਰ_ਸੈੱਟ_ਥਾਂ(f64, _fp0)
#ਜੇ_ਅੰਤ

d_m3Op(Copy_ਥਾਂ_32)
{
    u32* dst = ਥਾਂ_ptr(u32);
    u32* src = ਥਾਂ_ptr(u32);
    d_m3_Preload_ਅਗਲਾ();

    *dst = *src;

    ਅਗਲਾ_Op_Preloaded();
}


d_m3Op(Preserve_Copy_ਥਾਂ_32)
{
    u32* dest = ਥਾਂ_ptr(u32);
    u32* src = ਥਾਂ_ptr(u32);
    u32* preserve = ਥਾਂ_ptr(u32);
    d_m3_Preload_ਅਗਲਾ();

    *preserve = *dest;
    *dest = *src;

    ਅਗਲਾ_Op_Preloaded();
}


d_m3Op(Copy_ਥਾਂ_64)
{
    u64* dst = ਥਾਂ_ptr(u64);
    u64* src = ਥਾਂ_ptr(u64);
    d_m3_Preload_ਅਗਲਾ();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    ਅਗਲਾ_Op_Preloaded();
}


d_m3Op(Preserve_Copy_ਥਾਂ_64)
{
    u64* dest = ਥਾਂ_ptr(u64);
    u64* src = ਥਾਂ_ptr(u64);
    u64* preserve = ਥਾਂ_ptr(u64);
    d_m3_Preload_ਅਗਲਾ();

    *preserve = *dest;
    *dest = *src;

    ਅਗਲਾ_Op_Preloaded();
}


#ਜੇ d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_ਢੇਰ)
{
    u32         opcode_ਸੂਚਕਾਂਕ = ਤੁਰੰਤ(u32);
    u32         ਢੇਰ_ਉਚਾਈ = ਤੁਰੰਤ(u32);
    IM3Function function = ਤੁਰੰਤ(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    ਲਿਖੋ(" %4d ", opcode_ਸੂਚਕਾਂਕ);
    ਲਿਖੋ(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  ਜੇ d_m3HasFloat
    ਲਿਖੋ("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    ਲਈ (u32 i = 0; i < ਢੇਰ_ਉਚਾਈ; ++i) {
        cstr_t ਕਿਸਮ = "";

        ਲਿਖੋ("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, ਕਿਸਮ, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    ਲਿਖੋ("---------------------------------------------------------------------------------------------------------\n");

    ਅਗਲਾ_Op_ਸਿੱਧਾ();
}
#ਜੇ_ਅੰਤ


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = ਥਾਂ (TYPE);                \
    TYPE operand1 = ਥਾਂ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ਅਗਲਾ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = ਥਾਂ (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = ਥਾਂ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ਅਗਲਾ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = ਥਾਂ (i32);                 \
                                                \
    TYPE operand2 = ਥਾਂ (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ਅਗਲਾ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = ਥਾਂ (i32);                 \
                                                \
    TYPE operand2 = ਥਾਂ (TYPE);                \
    TYPE operand1 = ਥਾਂ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ਅਗਲਾ_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = ਥਾਂ (TYPE);                \
    TYPE operand1 = ਥਾਂ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ਅਗਲਾ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = ਥਾਂ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ਅਗਲਾ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = ਥਾਂ (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ਅਗਲਾ_Op ();                                  \
}

#ਜੇ d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, ਥਾਂ(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, ਥਾਂ(i32))
#ਜੇ_ਅੰਤ

d_m3Op(Return)
{
    m3_ਢੇਰ_Check();
    ਵਾਪਸ m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = ਤੁਰੰਤ(pc_t);

    ਜੇ (condition) {
        jumpOp(branch);
    } ਨਹੀਂਤਾਂ {
        ਅਗਲਾ_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = ਥਾਂ(i32);
    pc_t branch = ਤੁਰੰਤ(pc_t);

    ਜੇ (condition) {
        jumpOp(branch);
    } ਨਹੀਂਤਾਂ {
        ਅਗਲਾ_Op();
    }
}

#ਜੇ d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                             \
      pc_t branch  = ਤੁਰੰਤ (pc_t);                        \
      ਜੇ (ਸੰਕਾਰਜ OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      ਨਹੀਂਤਾਂ ਅਗਲਾ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                             \
      pc_t branch  = ਤੁਰੰਤ (pc_t);                        \
      ਜੇ (((TYPE) _r0) OPER ਸੰਕਾਰਜ)                          \
          jumpOp (branch);                                    \
      ਨਹੀਂਤਾਂ ਅਗਲਾ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = ਥਾਂ (TYPE);                            \
      TYPE operand1 = ਥਾਂ (TYPE);                            \
      pc_t branch   = ਤੁਰੰਤ (pc_t);                       \
      ਜੇ (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      ਨਹੀਂਤਾਂ ਅਗਲਾ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                             \
      pc_t elsePC  = ਤੁਰੰਤ (pc_t);                        \
      ਜੇ (ਸੰਕਾਰਜ OPER ((TYPE) _r0))                          \
          ਅਗਲਾ_Op ();                                          \
      ਨਹੀਂਤਾਂ jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE ਸੰਕਾਰਜ = ਥਾਂ (TYPE);                             \
      pc_t elsePC  = ਤੁਰੰਤ (pc_t);                        \
      ਜੇ (((TYPE) _r0) OPER ਸੰਕਾਰਜ)                          \
          ਅਗਲਾ_Op ();                                          \
      ਨਹੀਂਤਾਂ jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = ਥਾਂ (TYPE);                            \
      TYPE operand1 = ਥਾਂ (TYPE);                            \
      pc_t elsePC   = ਤੁਰੰਤ (pc_t);                       \
      ਜੇ (operand1 OPER operand2)                             \
          ਅਗਲਾ_Op ();                                          \
      ਨਹੀਂਤਾਂ jumpOp (elsePC);                                   \
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
    pc_t branch = ਤੁਰੰਤ(pc_t);
    ਜੇ (((i32)_r0) == 0) {
        jumpOp(branch);
    } ਨਹੀਂਤਾਂ {
        ਅਗਲਾ_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  ਸੰਕਾਰਜ = ਥਾਂ(i32);
    pc_t branch = ਤੁਰੰਤ(pc_t);
    ਜੇ (ਸੰਕਾਰਜ == 0) {
        jumpOp(branch);
    } ਨਹੀਂਤਾਂ {
        ਅਗਲਾ_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = ਤੁਰੰਤ(pc_t);
    ਜੇ (((i32)_r0) == 0) {
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  ਸੰਕਾਰਜ = ਥਾਂ(i32);
    pc_t elsePC = ਤੁਰੰਤ(pc_t);
    ਜੇ (ਸੰਕਾਰਜ == 0) {
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        jumpOp(elsePC);
    }
}

#ਜੇ_ਅੰਤ // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = ਤੁਰੰਤ(pc_t);

    ਜੇ (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = ਥਾਂ(i32);
    pc_t branch = ਤੁਰੰਤ(pc_t);

    ਜੇ (condition) {
        ਅਗਲਾ_Op();
    } ਨਹੀਂਤਾਂ {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_ਢੇਰ_Check();

    ਖਾਲੀ* loopId = ਤੁਰੰਤ(ਖਾਲੀ*);
    ਵਾਪਸ loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    ਖਾਲੀ* loopId = ਤੁਰੰਤ(ਖਾਲੀ*);

    ਜੇ (condition) {
        ਵਾਪਸ loopId;
    } ਨਹੀਂਤਾਂ {
        ਅਗਲਾ_Op();
    }
}


#ਜੇ d_m3_Has_ਢੇਰ_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_ਢੇਰ_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    ਜੇ (M3_UNLIKELY(runtime->suspendRequested and runtime->ਸਰਗਰਮ_Continuation)) {
        Suspend_Without_ਟੈਗ(runtime, runtime->ਸਰਗਰਮ_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_ਦਲੀਲਾਂ(_r0, _fp0));

        ਵਾਪਸ m3Err_continuationSuspended;
    }

    ਖਾਲੀ* loopId = ਤੁਰੰਤ(ਖਾਲੀ*);
    ਵਾਪਸ loopId;
}


#  ਜੇ !d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(ਇੰਦਰਾਜ_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = ਤੁਰੰਤ(IM3Function);

    ਜੇ (ਇੰਦਰਾਜ_ਸਮਾਉਂਦਾ(function, _sp, _mem)) {
        Enter_ਚੌਖਟਾ(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        ਜੇ (M3_UNLIKELY(runtime->suspendRequested and runtime->ਸਰਗਰਮ_Continuation)) {
            Suspend_Without_ਟੈਗ(runtime, runtime->ਸਰਗਰਮ_Continuation, safepoint_ਇੰਦਰਾਜ, _pc, _sp,
                              d_m3_Exp_Reg_ਦਲੀਲਾਂ(_r0, _fp0));

            ਵਾਪਸ m3Err_continuationSuspended;
        }

        ਅਗਲਾ_Op_ਸਿੱਧਾ();
    } ਨਹੀਂਤਾਂ {
        newTrap(m3_Err_trap_ਢੇਰ_ਉਛਾਲ);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(ਇੰਦਰਾਜ_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    ਜੇ (M3_UNLIKELY(runtime->suspendRequested and runtime->ਸਰਗਰਮ_Continuation)) {
        Suspend_Without_ਟੈਗ(runtime, runtime->ਸਰਗਰਮ_Continuation, safepoint_ਇੰਦਰਾਜ, _pc - 1, _sp,
                          d_m3_Exp_Reg_ਦਲੀਲਾਂ(_r0, _fp0));

        ਵਾਪਸ m3Err_continuationSuspended;
    }

    ਅਗਲਾ_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    ਖਾਲੀ* loopId = ਤੁਰੰਤ(ਖਾਲੀ*);

    ਜੇ (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        ਜੇ (M3_UNLIKELY(runtime->suspendRequested and runtime->ਸਰਗਰਮ_Continuation)) {
            Suspend_Without_ਟੈਗ(runtime, runtime->ਸਰਗਰਮ_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_ਦਲੀਲਾਂ(_r0, _fp0));

            ਵਾਪਸ m3Err_continuationSuspended;
        }
        ਵਾਪਸ loopId;
    } ਨਹੀਂਤਾਂ {
        ਅਗਲਾ_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
ਸਥਿਰ
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  ਜੇ d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    ਜੇ (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } ਨਹੀਂਤਾਂ ਜੇ (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } ਨਹੀਂਤਾਂ ਜੇ (op == op_ਇੰਦਰਾਜ_Check) {
        ਵਾਪਸ ਅਗਲਾ_Op_Impl();
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  ਜੇ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    ਵਾਪਸ op(_pc, d_m3_Op_ਦਲੀਲਾਂ, d_m3_ਅਧਾਰ_Cstr);
#  ਨਹੀਂਤਾਂ
    ਵਾਪਸ op(_pc, d_m3_Op_ਦਲੀਲਾਂ);
#  endif
}
#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 ਮੁੱਲ = *(u32*)_pc++;
    ਥਾਂ(u32) = ਮੁੱਲ;
    ਅਗਲਾ_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 ਮੁੱਲ;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਮੁੱਲ, _pc, ਆਕਾਰ(ਮੁੱਲ));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    ਥਾਂ(u64) = ਮੁੱਲ;
    ਅਗਲਾ_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_ਢੇਰ_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(ਸਿਰਾ_2)
{
    m3_ਢੇਰ_Check();
    ਵਾਪਸ m3Err_none;
}


d_m3Op(ਸੈੱਟ_Global_s32)
{
    u32* global = ਤੁਰੰਤ(u32*);
    *global = ਥਾਂ(u32);

    ਅਗਲਾ_Op();
}


d_m3Op(ਸੈੱਟ_Global_s64)
{
    u64* global = ਤੁਰੰਤ(u64*);
    *global = ਥਾਂ(u64);

    ਅਗਲਾ_Op();
}

#ਜੇ d_m3HasFloat
d_m3Op(ਸੈੱਟ_Global_f32)
{
    f32* global = ਤੁਰੰਤ(f32*);
    *global = (f32)_fp0;

    ਅਗਲਾ_Op();
}


d_m3Op(ਸੈੱਟ_Global_f64)
{
    f64* global = ਤੁਰੰਤ(f64*);
    *global = _fp0;

    ਅਗਲਾ_Op();
}
#ਜੇ_ਅੰਤ


#ਜੇ d_m3HasMemory64

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
    u64  ਸੰਕਾਰਜ = ਥਾਂ(u64);
    u32* ਪਤਾ = ਥਾਂ_ptr(u32);

    u64  offset;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&offset, _pc, ਆਕਾਰ(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    ਜੇ (M3_LIKELY(ਸੰਕਾਰਜ < d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ)) {
        u64 effective = ਸੰਕਾਰਜ + offset;

        // the access itself checks its own size against what is left
        ਜੇ (M3_LIKELY(effective < _mem->ਲੰਮਾਈ)) {
            *ਪਤਾ = (u32)effective;
            ਅਗਲਾ_Op();
        }
    }

    d_ਬਾਹਰਲਾ_ਦਾ_Bounds;
}

#ਜੇ_ਅੰਤ // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#ਜੇ d_m3_Skip_ਸਮ੍ਰਿਤੀ_Bounds_Check || d_m3_Guarded_ਸਮ੍ਰਿਤੀ
#  define m3MemCheck(x) true
#ਨਹੀਂਤਾਂ
#  define m3MemCheck(x) M3_LIKELY(x)
#ਜੇ_ਅੰਤ

// memcpy here is to support non-aligned access on some platforms.

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਲੱਦੋ(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = ਤੁਰੰਤ (u32);                       \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    u64 ਸੰਕਾਰਜ = (u32) _r0;                            \
    ਸੰਕਾਰਜ += offset;                                  \
                                                        \
    ਜੇ (m3MemCheck(                                     \
        ਸੰਕਾਰਜ + ਆਕਾਰ (SRC_TYPE) <= _mem->ਲੰਮਾਈ     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;       \
            SRC_TYPE ਮੁੱਲ;                             \
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਮੁੱਲ, src8, ਆਕਾਰ(ਮੁੱਲ));        \
            M3_BSWAP_##SRC_TYPE(ਮੁੱਲ);                 \
            REG = (DEST_TYPE)ਮੁੱਲ;                     \
            d_m3_Trace_ਲੱਦੋ(DEST_TYPE, ਸੰਕਾਰਜ, REG);     \
        }                                               \
        ਅਗਲਾ_Op_Preloaded ();                             \
    } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 ਸੰਕਾਰਜ = ਥਾਂ (u32);                           \
    u32 offset = ਤੁਰੰਤ (u32);                       \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    ਸੰਕਾਰਜ += offset;                                  \
                                                        \
    ਜੇ (m3MemCheck(                                     \
        ਸੰਕਾਰਜ + ਆਕਾਰ (SRC_TYPE) <= _mem->ਲੰਮਾਈ     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;       \
            SRC_TYPE ਮੁੱਲ;                             \
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਮੁੱਲ, src8, ਆਕਾਰ(ਮੁੱਲ));        \
            M3_BSWAP_##SRC_TYPE(ਮੁੱਲ);                 \
            REG = (DEST_TYPE)ਮੁੱਲ;                     \
            d_m3_Trace_ਲੱਦੋ(DEST_TYPE, ਸੰਕਾਰਜ, REG);     \
        }                                               \
        ਅਗਲਾ_Op_Preloaded ();                             \
    } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਲੱਦੋ_i(DEST_TYPE, SRC_TYPE) d_m3_ਲੱਦੋ(_r0, DEST_TYPE, SRC_TYPE)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਲੱਦੋ_f(DEST_TYPE, SRC_TYPE) d_m3_ਲੱਦੋ(_fp0, DEST_TYPE, SRC_TYPE)

#ਜੇ d_m3HasFloat
d_m3_ਲੱਦੋ_f(f32, f32);
d_m3_ਲੱਦੋ_f(f64, f64);
#ਜੇ_ਅੰਤ

d_m3_ਲੱਦੋ_i(i32, i8);
d_m3_ਲੱਦੋ_i(i32, u8);
d_m3_ਲੱਦੋ_i(i32, i16);
d_m3_ਲੱਦੋ_i(i32, u16);
d_m3_ਲੱਦੋ_i(i32, i32);

d_m3_ਲੱਦੋ_i(i64, i8);
d_m3_ਲੱਦੋ_i(i64, u8);
d_m3_ਲੱਦੋ_i(i64, i16);
d_m3_ਲੱਦੋ_i(i64, u16);
d_m3_ਲੱਦੋ_i(i64, i32);
d_m3_ਲੱਦੋ_i(i64, u32);
d_m3_ਲੱਦੋ_i(i64, i64);

#ਜੇ d_m3_Fold_ਸੈੱਟ_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_ਲੱਦੋ_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = ਤੁਰੰਤ (u32);                       \
      DEST_TYPE * dest = ਥਾਂ_ptr (DEST_TYPE);            \
      d_m3_Preload_ਅਗਲਾ ();                                 \
      u64 ਸੰਕਾਰਜ = (u32) _r0;                            \
      ਸੰਕਾਰਜ += offset;                                  \
                                                          \
      ਜੇ (m3MemCheck(                                     \
          ਸੰਕਾਰਜ + ਆਕਾਰ (SRC_TYPE) <= _mem->ਲੰਮਾਈ     \
      )) {                                                \
          u8* src8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;           \
          SRC_TYPE ਮੁੱਲ;                                 \
          ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਮੁੱਲ, src8, ਆਕਾਰ(ਮੁੱਲ));            \
          M3_BSWAP_##SRC_TYPE(ਮੁੱਲ);                     \
          DEST_TYPE ਨਤੀਜਾ = (DEST_TYPE)ਮੁੱਲ;            \
          * dest = ਨਤੀਜਾ;  REG = ਨਤੀਜਾ;                 \
          d_m3_Trace_ਲੱਦੋ(DEST_TYPE, ਸੰਕਾਰਜ, ਨਤੀਜਾ);      \
          ਅਗਲਾ_Op_Preloaded ();                             \
      } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 ਸੰਕਾਰਜ = ਥਾਂ (u32);                           \
      u32 offset = ਤੁਰੰਤ (u32);                       \
      DEST_TYPE * dest = ਥਾਂ_ptr (DEST_TYPE);            \
      d_m3_Preload_ਅਗਲਾ ();                                 \
      ਸੰਕਾਰਜ += offset;                                  \
                                                          \
      ਜੇ (m3MemCheck(                                     \
          ਸੰਕਾਰਜ + ਆਕਾਰ (SRC_TYPE) <= _mem->ਲੰਮਾਈ     \
      )) {                                                \
          u8* src8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;           \
          SRC_TYPE ਮੁੱਲ;                                 \
          ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਮੁੱਲ, src8, ਆਕਾਰ(ਮੁੱਲ));            \
          M3_BSWAP_##SRC_TYPE(ਮੁੱਲ);                     \
          DEST_TYPE ਨਤੀਜਾ = (DEST_TYPE)ਮੁੱਲ;            \
          * dest = ਨਤੀਜਾ;  REG = ਨਤੀਜਾ;                 \
          d_m3_Trace_ਲੱਦੋ(DEST_TYPE, ਸੰਕਾਰਜ, ਨਤੀਜਾ);      \
          ਅਗਲਾ_Op_Preloaded ();                             \
      } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
  }

d_m3_ਲੱਦੋ_Fold(_r0, i32, i8)
d_m3_ਲੱਦੋ_Fold(_r0, i32, u8)
d_m3_ਲੱਦੋ_Fold(_r0, i32, i16)
d_m3_ਲੱਦੋ_Fold(_r0, i32, u16)
d_m3_ਲੱਦੋ_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_ਲੱਦੋ_Fold(_r0, i64, i8)
d_m3_ਲੱਦੋ_Fold(_r0, i64, u8)
d_m3_ਲੱਦੋ_Fold(_r0, i64, i16)
d_m3_ਲੱਦੋ_Fold(_r0, i64, u16)
d_m3_ਲੱਦੋ_Fold(_r0, i64, i32)
d_m3_ਲੱਦੋ_Fold(_r0, i64, u32)
d_m3_ਲੱਦੋ_Fold(_r0, i64, i64)
#  ਜੇ d_m3HasFloat
d_m3_ਲੱਦੋ_Fold(_fp0, f32, f32)
d_m3_ਲੱਦੋ_Fold(_fp0, f64, f64)
#  endif

#ਜੇ_ਅੰਤ // d_m3FoldSetLocal

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 ਸੰਕਾਰਜ = ਥਾਂ (u32);                           \
    u32 offset = ਤੁਰੰਤ (u32);                       \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    ਸੰਕਾਰਜ += offset;                                  \
                                                        \
    ਜੇ (m3MemCheck(                                     \
        ਸੰਕਾਰਜ + ਆਕਾਰ (DEST_TYPE) <= _mem->ਲੰਮਾਈ    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, ਸੰਕਾਰਜ, REG);     \
            u8* mem8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(mem8, &val, ਆਕਾਰ(val));            \
        }                                               \
        ਅਗਲਾ_Op_Preloaded ();                             \
    } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ਅਚਲ SRC_TYPE ਮੁੱਲ = ਥਾਂ (SRC_TYPE);             \
    u64 ਸੰਕਾਰਜ = (u32) _r0;                            \
    u32 offset = ਤੁਰੰਤ (u32);                       \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    ਸੰਕਾਰਜ += offset;                                  \
                                                        \
    ਜੇ (m3MemCheck(                                     \
        ਸੰਕਾਰਜ + ਆਕਾਰ (DEST_TYPE) <= _mem->ਲੰਮਾਈ    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, ਸੰਕਾਰਜ, ਮੁੱਲ);   \
            u8* mem8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;       \
            DEST_TYPE val = (DEST_TYPE) ਮੁੱਲ;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(mem8, &val, ਆਕਾਰ(val));            \
        }                                               \
        ਅਗਲਾ_Op_Preloaded ();                             \
    } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ਅਚਲ SRC_TYPE ਮੁੱਲ = ਥਾਂ (SRC_TYPE);             \
    u64 ਸੰਕਾਰਜ = ਥਾਂ (u32);                           \
    u32 offset = ਤੁਰੰਤ (u32);                       \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    ਸੰਕਾਰਜ += offset;                                  \
                                                        \
    ਜੇ (m3MemCheck(                                     \
        ਸੰਕਾਰਜ + ਆਕਾਰ (DEST_TYPE) <= _mem->ਲੰਮਾਈ    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, ਸੰਕਾਰਜ, ਮੁੱਲ);   \
            u8* mem8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;       \
            DEST_TYPE val = (DEST_TYPE) ਮੁੱਲ;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(mem8, &val, ਆਕਾਰ(val));            \
        }                                               \
        ਅਗਲਾ_Op_Preloaded ();                             \
    } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
}

// both operands can be in regs when storing a float
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 ਸੰਕਾਰਜ = (u32) _r0;                            \
    u32 offset = ਤੁਰੰਤ (u32);                       \
    d_m3_Preload_ਅਗਲਾ ();                                 \
    ਸੰਕਾਰਜ += offset;                                  \
                                                        \
    ਜੇ (m3MemCheck(                                     \
        ਸੰਕਾਰਜ + ਆਕਾਰ (TYPE) <= _mem->ਲੰਮਾਈ         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, ਸੰਕਾਰਜ, REG);         \
            u8* mem8 = m3_Mem_ਅੰਕੜੇ(_mem) + ਸੰਕਾਰਜ;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(mem8, &val, ਆਕਾਰ(val));            \
        }                                               \
        ਅਗਲਾ_Op_Preloaded ();                             \
    } ਨਹੀਂਤਾਂ d_ਬਾਹਰਲਾ_ਦਾ_Bounds;                               \
}


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#ਜੇ d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#ਜੇ_ਅੰਤ

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exec_atomic.h"

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#ਜੇ d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    ਅੱਖਰ name[100];
    strcpy(name, ਲੜੀ_ਖੋਜ(i_opcode, "op_") + 3);
    ਅੱਖਰ* bracket = ਲੜੀ_ਖੋਜ(name, "(");
    ਜੇ (bracket) {
        *bracket = 0;
    }

    puts(name);
    ਅਗਲਾ_Op_ਸਿੱਧਾ();
}
#ਜੇ_ਅੰਤ

#ਜੇ d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_ਪ੍ਰਾਪਤੀ(i_operationName);

    ਅਗਲਾ_Op_ਸਿੱਧਾ();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_ਪ੍ਰਾਪਤੀ(i_operationName);

    ਅਗਲਾ_Op_ਸਿੱਧਾ();
}
#ਜੇ_ਅੰਤ

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_exec_h
