//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_exec_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_exec_h

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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "m3_math_utils.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"
#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"
#ಅನ್ತರ್ಭಾವ "m3_exec_defs.h"
#ಅನ್ತರ್ಭಾವ "m3_atomic.h"
#ಅನ್ತರ್ಭಾವ "m3_host.h"

#ಅನ್ತರ್ಭಾವ <limits.h>

d_m3BeginExternC

#ವ್ಯಾಖ್ಯಾನಿಸು rewrite_op(OP)             * ((ಶೂನ್ಯ **) (_pc-1)) = (ಶೂನ್ಯ*)(OP)

#ವ್ಯಾಖ್ಯಾನಿಸು ತಕ್ಷಣ(TYPE)            * ((TYPE *) _pc++)
#ವ್ಯಾಖ್ಯಾನಿಸು skip_ತಕ್ಷಣ(TYPE)       (_pc++)

#ವ್ಯಾಖ್ಯಾನಿಸು ಸ್ಥಾನ(TYPE)                 * (TYPE *) (_sp + ತಕ್ಷಣ (i32))
#ವ್ಯಾಖ್ಯಾನಿಸು ಸ್ಥಾನ_ptr(TYPE)             (TYPE *) (_sp + ತಕ್ಷಣ (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Wide_ಕಾರ್ಯಾಂಶ(IS64)      ((IS64) ? ಸ್ಥಾನ (u64) : (u64) ಸ್ಥಾನ (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3MemRangeOk(ಆರಂಭ_2, ಉದ್ದಳತೆ_2, MEM)  \
   ((ಆರಂಭ_2) <= (MEM)->ಉದ್ದಳತೆ and (ಉದ್ದಳತೆ_2) <= (MEM)->ಉದ್ದಳತೆ - (ಆರಂಭ_2))


#ಆದರೆ d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define ಮುಂದಿನ_Op()                 M3_MUSTTAIL ಹಿಂತಿರುಗಿ profileOp (d_m3_Op_All_ವಾದಗಳು, __FUNCTION__)
#ಅಥವಾಆದರೆ d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define ಮುಂದಿನ_Op()                 M3_MUSTTAIL ಹಿಂತಿರುಗಿ debugOp (d_m3_Op_All_ವಾದಗಳು, __FUNCTION__)
#ಇಲ್ಲದಿದ್ದರೆ
#  define ಮುಂದಿನ_Op()                 ಮುಂದಿನ_Op_ನೇರ()
#ಆದರೆ_ಅಂತ್ಯ

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#ಆದರೆ d_m3_Preload_ಮುಂದಿನ_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_ಮುಂದಿನ()        ಸ್ಥಿರಾಂಕ IM3Operation ಮುಂದಿನ_Op_2 = (IM3Operation) (* _pc)
#  define ಮುಂದಿನ_Op_Preloaded()        M3_MUSTTAIL ಹಿಂತಿರುಗಿ ಮುಂದಿನ_Op_2 (_pc + 1, d_m3_Op_ವಾದಗಳು)
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3_Preload_ಮುಂದಿನ()
#  define ಮುಂದಿನ_Op_Preloaded()        ಮುಂದಿನ_Op()
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL ಹಿಂತಿರುಗಿ profileJumpOp ((pc_t)(PC), d_m3_Op_ವಾದಗಳು, __FUNCTION__)
#ಇಲ್ಲದಿದ್ದರೆ
#  define jumpOp(PC)               jump_Op_ನೇರ(PC)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
#  define ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2()            (ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು (_mem->runtime, _pc - 1))
#  define fill_Backtrace_ಚೌಕಟ್ಟು(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    ಹಿಂತಿರುಗಿ (ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2 (), err)
#  define forwardTrap(err)                ಹಿಂತಿರುಗಿ err
#ಇಲ್ಲದಿದ್ದರೆ
#  define ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2()            ಮಾಡು {} ಆಗ (0)
#  define fill_Backtrace_ಚೌಕಟ್ಟು(FUNCTION)    ಮಾಡು {} ಆಗ (0)

#  define newTrap(err)                    ಹಿಂತಿರುಗಿ err
#  define forwardTrap(err)                ಹಿಂತಿರುಗಿ err
#ಆದರೆ_ಅಂತ್ಯ

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#ಆದರೆ d_m3_Max_Native_ರಾಶಿ > 0
#  define d_m3_Check_Native_ರಾಶಿ()                                               \
       ಮಾಡು {                                                                    \
           ಶೂನ್ಯ * m3_ಎಲ್ಲೆ = m3MemRuntime (_mem)->ರಾಶಿ_ಎಲ್ಲೆ;                  \
           ಆದರೆ (M3_UNLIKELY (m3_ಎಲ್ಲೆ && m3_Native_ರಾಶಿ_Ptr () < m3_ಎಲ್ಲೆ))      \
               newTrap (m3_Err_trap_ರಾಶಿ_ಅತಿಪ್ರವಾಹ);                              \
       } ಆಗ (0)
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3_Check_Native_ರಾಶಿ()           ಮಾಡು {} ಆಗ (0)
#ಆದರೆ_ಅಂತ್ಯ


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  ಆದರೆ d_m3HasFloat
#    define d_m3_Exp_Reg_ವಾದಗಳು(R0, FP0)        (R0), (FP0)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define d_m3_Exp_Reg_ವಾದಗಳು(R0, FP0)        (R0)
#  endif

#  ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_ವಾದಗಳು (R0, FP0), d_m3_ತಳ_Cstr)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_ವಾದಗಳು (R0, FP0))
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
ಸ್ಥಿರ M3_NOINLINE
m3ret_t ದಾಖಲೆ_Call_ಚೌಕಟ್ಟು (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_ಸ್ಮರಣೆ i_ಸ್ಮರಣೆ,
                         IM3Function i_function, m3reg_t i_r0
#  ಆದರೆ d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;

    ಚೌಕಟ್ಟು.ಬಗೆ = ಚೌಕಟ್ಟು_call;
    ಚೌಕಟ್ಟು.pc = i_pc;
    ಚೌಕಟ್ಟು.sp = i_sp;
    ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ = i_ಸ್ಮರಣೆ;
    ಚೌಕಟ್ಟು.call.function = i_function;
    ಚೌಕಟ್ಟು.call.r0 = i_r0;
#  ಆದರೆ d_m3HasFloat
    ಚೌಕಟ್ಟು.call.fp0 = i_fp0;
#  endif

    ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(i_runtime, &ಚೌಕಟ್ಟು);
}

#  define d_m3_ದಾಖಲೆ_Call_ಚೌಕಟ್ಟು(ಸ್ಮರಣೆ_2, FUNCTION)                                \
       ಆದರೆ (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           ಹಿಂತಿರುಗಿ ದಾಖಲೆ_Call_ಚೌಕಟ್ಟು (m3MemRuntime (_mem), _pc, _sp, (ಸ್ಮರಣೆ_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_ವಾದಗಳು (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
ಸ್ಥಿರ M3_NOINLINE
m3ret_t ದಾಖಲೆ_Loop_ಚೌಕಟ್ಟು (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_ಸ್ಮರಣೆ i_ಸ್ಮರಣೆ)
{
    M3_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;

    ಚೌಕಟ್ಟು.ಬಗೆ = ಚೌಕಟ್ಟು_loop;
    ಚೌಕಟ್ಟು.pc = i_pc;
    ಚೌಕಟ್ಟು.sp = i_sp;
    ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ = i_ಸ್ಮರಣೆ;

    ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(i_runtime, &ಚೌಕಟ್ಟು);
}


#  ಆದರೆ d_m3HasExceptionHandling
ಸ್ಥಿರ M3_NOINLINE
m3ret_t ದಾಖಲೆ_Try_ಚೌಕಟ್ಟು (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_ಸ್ಮರಣೆ i_ಸ್ಮರಣೆ,
                        u32 i_ಅಂಕೆ_Clauses, bool i_handlers_ಜೀವಂತ)
{
    M3_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;

    ಚೌಕಟ್ಟು.ಬಗೆ = ಚೌಕಟ್ಟು_try;
    ಚೌಕಟ್ಟು.pc = i_clauses;
    ಚೌಕಟ್ಟು.sp = i_sp;
    ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ = i_ಸ್ಮರಣೆ;
    ಚೌಕಟ್ಟು.try_.ಅಂಕೆ_Clauses = i_ಅಂಕೆ_Clauses;
    ಚೌಕಟ್ಟು.try_.handlers_ಜೀವಂತ = i_handlers_ಜೀವಂತ;

    ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(i_runtime, &ಚೌಕಟ್ಟು);
}
#  endif


#  ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
ಸ್ಥಿರ M3_NOINLINE
m3ret_t ದಾಖಲೆ_ನಮೂದು_ಚೌಕಟ್ಟು (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_ಸ್ಮರಣೆ i_ಸ್ಮರಣೆ,
                          IM3Function i_function)
{
    M3_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;

    ಚೌಕಟ್ಟು.ಬಗೆ = ಚೌಕಟ್ಟು_ನಮೂದು;
    ಚೌಕಟ್ಟು.pc = i_pc;
    ಚೌಕಟ್ಟು.sp = i_sp;
    ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ = i_ಸ್ಮರಣೆ;
    ಚೌಕಟ್ಟು.ನಮೂದು.function = i_function;

    ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(i_runtime, &ಚೌಕಟ್ಟು);
}
#  endif
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3_ದಾಖಲೆ_Call_ಚೌಕಟ್ಟು(ಸ್ಮರಣೆ_2, FUNCTION)
#ಆದರೆ_ಅಂತ್ಯ


#ಆದರೆ d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#ಅಥವಾಆದರೆ d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ಸ್ಥಿರಾಂಕ IM3Runtime trace_rt = m3MemRuntime(_mem); (ಶೂನ್ಯ) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_ಆಳ)*2, "", ##__VA_ARGS__)
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3EnableStrace >= 3
#  define d_m3_Trace_ತುಂಬು(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3_Trace_ತುಂಬು(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ DEBUG
  #ವ್ಯಾಖ್ಯಾನಿಸು d_ಹೊರಹೋಗು_ದ_Bounds newTrap (ErrorRuntime (m3_Err_trap_ಹೊರಹೋಗು_ದ_Bounds_ಸ್ಮರಣೆ_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->ಉದ್ದಳತೆ, ಕಾರ್ಯಾಂಶ))

#  define d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(ಸ್ಥಾನಾಂತರ, ಗಾತ್ರ_3) newTrap (ErrorRuntime (m3_Err_trap_ಹೊರಹೋಗು_ದ_Bounds_ಸ್ಮರಣೆ_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->ಉದ್ದಳತೆ, ಸ್ಥಾನಾಂತರ, ಗಾತ್ರ_3))
#ಇಲ್ಲದಿದ್ದರೆ
  #ವ್ಯಾಖ್ಯಾನಿಸು d_ಹೊರಹೋಗು_ದ_Bounds newTrap (m3_Err_trap_ಹೊರಹೋಗು_ದ_Bounds_ಸ್ಮರಣೆ_Access)

#  define d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(ಸ್ಥಾನಾಂತರ, ಗಾತ್ರ_3) newTrap (m3_Err_trap_ಹೊರಹೋಗು_ದ_Bounds_ಸ್ಮರಣೆ_Access)

#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#ಇಲ್ಲದಿದ್ದರೆ
d_m3RetSig Call (d_m3OpSig)
#ಆದರೆ_ಅಂತ್ಯ
{
    m3ret_t possible_trap = m3_Yield();
    ಆದರೆ (M3_UNLIKELY(possible_trap)) {
        ಹಿಂತಿರುಗಿ possible_trap;
    }

    ಮುಂದಿನ_Op_ನೇರ();
}

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                         \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    OP((RES), ಕಾರ್ಯಾಂಶ, ((TYPE) REG), ##__VA_ARGS__);    \
    ಮುಂದಿನ_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = ಸ್ಥಾನ (TYPE);                        \
    TYPE operand1 = ಸ್ಥಾನ (TYPE);                        \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    ಮುಂದಿನ_Op_Preloaded ();                                 \
}

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                         \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    OP((RES), ((TYPE) REG), ಕಾರ್ಯಾಂಶ, ##__VA_ARGS__);    \
    ಮುಂದಿನ_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#ವ್ಯಾಖ್ಯಾನಿಸು M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#ವ್ಯಾಖ್ಯಾನಿಸು M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#ವ್ಯಾಖ್ಯಾನಿಸು M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#ವ್ಯಾಖ್ಯಾನಿಸು M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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

#ಆದರೆ d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು OP_ಕೂಡಿಸು_32(A,B) (i32)((u32)(A) + (u32)(B))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_ಕೂಡಿಸು_64(A,B) (i64)((u64)(A) + (u64)(B))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, ಕೂಡಿಸು,      OP_ಕೂಡಿಸು_32)  d_m3CommutativeOpFunc_i (i64, ಕೂಡಿಸು,      OP_ಕೂಡಿಸು_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, ಕಳೆ,            OP_SUB_32)  d_m3OpFunc_i (i64, ಕಳೆ,            OP_SUB_64)

#ವ್ಯಾಖ್ಯಾನಿಸು OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, ಸರಿಸು_ಎಡ,       OP_SHL_32)      d_m3OpFunc_i (u64, ಸರಿಸು_ಎಡ,       OP_SHL_64)
d_m3OpFunc_i (i32, ಸರಿಸು_ಬಲ,      OP_SHR_32)      d_m3OpFunc_i (i64, ಸರಿಸು_ಬಲ,      OP_SHR_64)
d_m3OpFunc_i (u32, ಸರಿಸು_ಬಲ,      OP_SHR_32)      d_m3OpFunc_i (u64, ಸರಿಸು_ಬಲ,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#ಆದರೆ d_m3_Fold_ಗಣ_Local

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
    TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                         \
    TYPE * dest = ಸ್ಥಾನ_ptr (TYPE);                      \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    TYPE ಫಲಿತಾಂಶ;                                        \
    OP((ಫಲಿತಾಂಶ), ಕಾರ್ಯಾಂಶ, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = ಫಲಿತಾಂಶ;  REG = ಫಲಿತಾಂಶ;                     \
    ಮುಂದಿನ_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = ಸ್ಥಾನ (TYPE);                        \
    TYPE operand1 = ಸ್ಥಾನ (TYPE);                        \
    TYPE * dest = ಸ್ಥಾನ_ptr (TYPE);                      \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    TYPE ಫಲಿತಾಂಶ;                                        \
    OP((ಫಲಿತಾಂಶ), operand1, operand2, ##__VA_ARGS__);    \
    * dest = ಫಲಿತಾಂಶ;  REG = ಫಲಿತಾಂಶ;                     \
    ಮುಂದಿನ_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                         \
    TYPE * dest = ಸ್ಥಾನ_ptr (TYPE);                      \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    TYPE ಫಲಿತಾಂಶ;                                        \
    OP((ಫಲಿತಾಂಶ), ((TYPE) REG), ಕಾರ್ಯಾಂಶ, ##__VA_ARGS__); \
    * dest = ಫಲಿತಾಂಶ;  REG = ಫಲಿತಾಂಶ;                     \
    ಮುಂದಿನ_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, ಕೂಡಿಸು,          M3_FUNC, OP_ಕೂಡಿಸು_32)
d_m3FoldOp_i           (i32, ಕಳೆ,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, ಸರಿಸು_ಎಡ,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, ಸರಿಸು_ಬಲ,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, ಸರಿಸು_ಬಲ,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, ಕೂಡಿಸು,          M3_FUNC, OP_ಕೂಡಿಸು_64)
d_m3FoldOp_i           (i64, ಕಳೆ,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, ಸರಿಸು_ಎಡ,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, ಸರಿಸು_ಬಲ,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, ಸರಿಸು_ಬಲ,   M3_FUNC, OP_SHR_64)

#  ಆದರೆ d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, ಕೂಡಿಸು,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, ಕಳೆ,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, ಕೂಡಿಸು,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, ಕಳೆ,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ_ಅಂತ್ಯ // d_m3FoldSetLocal

#ಆದರೆ d_m3HasFloat
d_m3CommutativeArithOp_f(f32, ಕೂಡಿಸು,         +)      d_m3CommutativeArithOp_f(f64, ಕೂಡಿಸು,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, ಕಳೆ,               -)      d_m3ArithOp_f(f64, ಕಳೆ,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#ಆದರೆ_ಅಂತ್ಯ

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

#ಆದರೆ d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#ಆದರೆ_ಅಂತ್ಯ

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    ಮುಂದಿನ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                     \
    OP((RES), ಕಾರ್ಯಾಂಶ, ##__VA_ARGS__);              \
    ಮುಂದಿನ_Op ();                                      \
}

#ವ್ಯಾಖ್ಯಾನಿಸು M3_UNARY(RES, X, OP) (RES) = OP(X)
#ವ್ಯಾಖ್ಯಾನಿಸು M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#ಆದರೆ d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       ವರ್ಗಮೂಲ);
d_m3UnaryArithOp_f(f32, ಹತ್ತಿರದ,    rintf);    d_m3UnaryArithOp_f(f64, ಹತ್ತಿರದ,    rint);
#  ಆದರೆ defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  ಇಲ್ಲದಿದ್ದರೆ
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_ಗಮ್ಯ_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_ಗಮ್ಯ_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#ಆದರೆ (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#ಅಥವಾಆದರೆ defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CLZ_32(x) __builtin_clz(x)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CTZ_32(x) __builtin_ctz(x)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CLZ_64(x) __builtin_clzll(x)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CTZ_64(x) __builtin_ctzll(x)
#ಇಲ್ಲದಿದ್ದರೆ
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#ಆದರೆ_ಅಂತ್ಯ

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#ವ್ಯಾಖ್ಯಾನಿಸು OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    ಮುಂದಿನ_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ(i64);
    _r0 = OP_WRAP_I64(ಕಾರ್ಯಾಂಶ);
    ಮುಂದಿನ_Op();
}

// Integer sign extension operations
#ವ್ಯಾಖ್ಯಾನಿಸು OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#ವ್ಯಾಖ್ಯಾನಿಸು OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#ಆದರೆ d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_ಮೇಲಿನ = ಸ್ಥಾನ (u64);
    u64  rhs_ಕೆಳಗಿನ  = ಸ್ಥಾನ (u64);
    u64  lhs_ಮೇಲಿನ = ಸ್ಥಾನ (u64);
    u64  lhs_ಕೆಳಗಿನ  = ಸ್ಥಾನ (u64);
    u64* ಕೆಳಗಿನ     = ಸ್ಥಾನ_ptr (u64);
    d_m3_Preload_ಮುಂದಿನ();

    u64 ಮೊತ್ತ = lhs_ಕೆಳಗಿನ + rhs_ಕೆಳಗಿನ;

    *ಕೆಳಗಿನ = ಮೊತ್ತ;
    _r0  = (i64) (lhs_ಮೇಲಿನ + rhs_ಮೇಲಿನ + (ಮೊತ್ತ < lhs_ಕೆಳಗಿನ));

    ಮುಂದಿನ_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_ಮೇಲಿನ = ಸ್ಥಾನ (u64);
    u64  rhs_ಕೆಳಗಿನ  = ಸ್ಥಾನ (u64);
    u64  lhs_ಮೇಲಿನ = ಸ್ಥಾನ (u64);
    u64  lhs_ಕೆಳಗಿನ  = ಸ್ಥಾನ (u64);
    u64* ಕೆಳಗಿನ     = ಸ್ಥಾನ_ptr (u64);
    d_m3_Preload_ಮುಂದಿನ();

    *ಕೆಳಗಿನ = lhs_ಕೆಳಗಿನ - rhs_ಕೆಳಗಿನ;
    _r0  = (i64) (lhs_ಮೇಲಿನ - rhs_ಮೇಲಿನ - (lhs_ಕೆಳಗಿನ < rhs_ಕೆಳಗಿನ));

    ಮುಂದಿನ_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = ಸ್ಥಾನ (i64);
    i64  a   = ಸ್ಥಾನ (i64);
    u64* ಕೆಳಗಿನ = ಸ್ಥಾನ_ptr (u64);
    d_m3_Preload_ಮುಂದಿನ();

    u64 ಮೇಲಿನ;
    *ಕೆಳಗಿನ = mul_wide_i64 (a, b, &ಮೇಲಿನ);
    _r0  = (i64) ಮೇಲಿನ;

    ಮುಂದಿನ_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = ಸ್ಥಾನ (u64);
    u64  a   = ಸ್ಥಾನ (u64);
    u64* ಕೆಳಗಿನ = ಸ್ಥಾನ_ptr (u64);
    d_m3_Preload_ಮುಂದಿನ();

    u64 ಮೇಲಿನ;
    *ಕೆಳಗಿನ = mul_wide_u64 (a, b, &ಮೇಲಿನ);
    _r0  = (i64) ಮೇಲಿನ;

    ಮುಂದಿನ_Op_Preloaded();
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasWideArithmetic

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    ಮುಂದಿನ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * ರಾಶಿ = ಸ್ಥಾನ_ptr (FROM);                 \
    OP((DEST), (* ರಾಶಿ), ##__VA_ARGS__);           \
    ಮುಂದಿನ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = ಸ್ಥಾನ_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    ಮುಂದಿನ_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * ರಾಶಿ = ಸ್ಥಾನ_ptr (FROM);                 \
    TYPE * dest = ಸ್ಥಾನ_ptr (TYPE);                  \
    OP((* dest), (* ರಾಶಿ), ##__VA_ARGS__);         \
    ಮುಂದಿನ_Op ();                                      \
}

#ಆದರೆ d_m3HasFloat
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
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3TypeModifyOp(REG_ಗಮ್ಯ, REG_FROM, ಗಮ್ಯ_2, NAME, FROM)  \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_ಗಮ್ಯ = (ಗಮ್ಯ_2) ((FROM) REG_FROM);                        \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = ಸ್ಥಾನ (FROM);                                \
    REG_ಗಮ್ಯ = (ಗಮ್ಯ_2) (from);                                   \
    ಮುಂದಿನ_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3FloatModifyOp(ಗಮ್ಯ_2, NAME, FROM)                   \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((ಗಮ್ಯ_2) ((FROM) _fp0));             \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = ಸ್ಥಾನ (FROM);                                \
    _fp0 = canon_nan_##TO ((ಗಮ್ಯ_2) (from));                    \
    ಮುಂದಿನ_Op ();                                              \
}

#ಆದರೆ d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3TypeConvertOp(REG_ಗಮ್ಯ, REG_FROM, ಗಮ್ಯ_2, NAME, FROM) \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_ಗಮ್ಯ = (ಗಮ್ಯ_2) ((FROM) REG_FROM);                        \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    ಸ್ಥಾನ (ಗಮ್ಯ_2) = (ಗಮ್ಯ_2) ((FROM) REG_FROM);                     \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = ಸ್ಥಾನ (FROM);                                \
    REG_ಗಮ್ಯ = (ಗಮ್ಯ_2) (from);                                   \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = ಸ್ಥಾನ (FROM);                                \
    ಸ್ಥಾನ (ಗಮ್ಯ_2) = (ಗಮ್ಯ_2) (from);                                \
    ಮುಂದಿನ_Op ();                                              \
}

// Int to float
#ಆದರೆ d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3ReinterpretOp(REG, ಗಮ್ಯ_2, SRC, FROM)               \
d_m3Op(ಗಮ್ಯ_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    ಒಕ್ಕೂಟ { FROM c; ಗಮ್ಯ_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    ಒಕ್ಕೂಟ { FROM c; ಗಮ್ಯ_2 t; } u;                              \
    u.c = ಸ್ಥಾನ (FROM);                                      \
    REG = u.t;                                              \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    ಒಕ್ಕೂಟ { FROM c; ಗಮ್ಯ_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    ಸ್ಥಾನ (ಗಮ್ಯ_2) = u.t;                                        \
    ಮುಂದಿನ_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(ಗಮ್ಯ_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    ಒಕ್ಕೂಟ { FROM c; ಗಮ್ಯ_2 t; } u;                              \
    u.c = ಸ್ಥಾನ (FROM);                                      \
    ಸ್ಥಾನ (ಗಮ್ಯ_2) = u.t;                                        \
    ಮುಂದಿನ_Op ();                                              \
}

#ಆದರೆ d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#ಆದರೆ_ಅಂತ್ಯ

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = ತಕ್ಷಣ(u32*);
    ಸ್ಥಾನ(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    ಮುಂದಿನ_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = ತಕ್ಷಣ(u64*);
    ಸ್ಥಾನ(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    ಮುಂದಿನ_Op();
}


d_m3Op(ಗಣ_Global_i32)
{
    u32* global = ತಕ್ಷಣ(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    ಮುಂದಿನ_Op();
}


d_m3Op(ಗಣ_Global_i64)
{
    u64* global = ತಕ್ಷಣ(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    ಮುಂದಿನ_Op();
}


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

d_m3Op(Branch);
d_m3Op(ನಮೂದು_2);
#  ಆದರೆ !d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
d_m3Op(ನಮೂದು_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
ಸ್ಥಿರ M3_NOINLINE
IM3Function Function_ದ_Compiled_ಸಂಕೇತ (pc_t i_pc)
{
    ಆದರೆ (not i_pc) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಆಗ ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation ನಮೂದು = (IM3Operation)i_pc[0];

#  ಆದರೆ !d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
    ಆದರೆ (ನಮೂದು == op_ನಮೂದು_Suspendable) {
        ಹಿಂತಿರುಗಿ (IM3Function)i_pc[1];
    }
#  endif
    ಹಿಂತಿರುಗಿ (ನಮೂದು == op_ನಮೂದು) ? (IM3Function)i_pc[1] : ಖಾಲಿ;
}

#ಆದರೆ_ಅಂತ್ಯ

d_m3Op(Call)
{
    d_m3_Check_Native_ರಾಶಿ();

    pc_t      callPC = ತಕ್ಷಣ(pc_t);
    i32       ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);
    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = m3MemInfo(_mem);

    m3stack_t sp = _sp + ರಾಶಿ_ಸ್ಥಾನಾಂತರ;

#ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_ವಾದಗಳು, d_m3_ತಳ_Cstr);
#ಇಲ್ಲದಿದ್ದರೆ
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_ವಾದಗಳು);
#ಆದರೆ_ಅಂತ್ಯ

    _mem = ಸ್ಮರಣೆ->mallocated;

    ಆದರೆ (M3_LIKELY(not r)) {
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_m3_ದಾಖಲೆ_Call_ಚೌಕಟ್ಟು(ಸ್ಮರಣೆ, Function_ದ_Compiled_ಸಂಕೇತ(callPC));
        ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = ಸ್ಥಾನ(IM3Function);
    IM3FuncType type = ತಕ್ಷಣ(IM3FuncType);
    i32         ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);
    IM3_ಸ್ಮರಣೆ   ಸ್ಮರಣೆ = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ರಾಶಿ_ಸ್ಥಾನಾಂತರ;

    m3ret_t     r = m3Err_none;

    ಆದರೆ (M3_LIKELY(function)) {
        ಆದರೆ (M3_LIKELY(type == function->funcType)) {
            ಆದರೆ (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            ಆದರೆ (M3_LIKELY(not r)) {
#ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_ವಾದಗಳು, d_m3_ತಳ_Cstr);
#ಇಲ್ಲದಿದ್ದರೆ
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_ವಾದಗಳು);
#ಆದರೆ_ಅಂತ್ಯ

                _mem = ಸ್ಮರಣೆ->mallocated;

                ಆದರೆ (M3_LIKELY(not r)) {
                    ಮುಂದಿನ_Op_ನೇರ();
                } ಇಲ್ಲದಿದ್ದರೆ {
                    d_m3_ದಾಖಲೆ_Call_ಚೌಕಟ್ಟು(ಸ್ಮರಣೆ, function);
                    ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2();
                    forwardTrap(r);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            r = m3_Err_trap_ಪರೋಕ್ಷ_Call_Type_Mismatch;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        r = m3Err_trapNullFunctionRef;
    }

    ಆದರೆ (M3_UNLIKELY(r)) {
        newTrap(r);
    } ಇಲ್ಲದಿದ್ದರೆ {
        forwardTrap(r);
    }
}


// ref.as_non_null is a null check and nothing else: the value stays put, so
// there is no destination slot, only the trap.
d_m3Op(RefAsNonNull)
{
    IM3Function reference = ಸ್ಥಾನ(IM3Function);

    ಆದರೆ (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    ಮುಂದಿನ_Op();
}


d_m3Op(Call_ಪರೋಕ್ಷ)
{
    d_m3_Check_Native_ರಾಶಿ();

    M3_ಕೋಷ್ಟಕ*    ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    u64         ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);
    IM3FuncType type = ತಕ್ಷಣ(IM3FuncType);
    i32         ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);
    IM3_ಸ್ಮರಣೆ   ಸ್ಮರಣೆ = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ರಾಶಿ_ಸ್ಥಾನಾಂತರ;

    m3ret_t     r = m3Err_none;

    ಆದರೆ (M3_LIKELY(ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2)) {
        IM3Function function = (IM3Function)ಕೋಷ್ಟಕ_2->elements[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ];

        ಆದರೆ (M3_LIKELY(function)) {
            ಆದರೆ (M3_LIKELY(type == function->funcType)) {
                ಆದರೆ (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                ಆದರೆ (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* calleeMem = Module_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ(function->module);

#ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_ವಾದಗಳು, d_m3_ತಳ_Cstr);
#ಇಲ್ಲದಿದ್ದರೆ
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_ವಾದಗಳು);
#ಆದರೆ_ಅಂತ್ಯ

                    _mem = ಸ್ಮರಣೆ->mallocated;

                    ಆದರೆ (M3_LIKELY(not r)) {
                        ಮುಂದಿನ_Op_ನೇರ();
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        d_m3_ದಾಖಲೆ_Call_ಚೌಕಟ್ಟು(ಸ್ಮರಣೆ, function);
                        ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2();
                        forwardTrap(r);
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                r = m3_Err_trap_ಪರೋಕ್ಷ_Call_Type_Mismatch;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            r = m3_Err_trap_ಕೋಷ್ಟಕ_Element_Is_Null;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        r = m3_Err_trap_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ_ಹೊರಹೋಗು_ದ_Range;
    }

    ಆದರೆ (M3_UNLIKELY(r)) {
        newTrap(r);
    } ಇಲ್ಲದಿದ್ದರೆ {
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
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ಬಾಲ_Call_ವಾದಗಳು(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((ಶೂನ್ಯ *) (_sp + (RETURNSLOTS)), (ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * ಗಾತ್ರ (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = ತಕ್ಷಣ(pc_t);
    i32     ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);
    i32     returnSlots = ತಕ್ಷಣ(i32);
    u32     ಅಂಕೆ_Arg_Slots = ತಕ್ಷಣ(u32);

    m3ret_t possible_trap = m3_Yield();
    ಆದರೆ (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_ಬಾಲ_Call_ವಾದಗಳು(returnSlots, ರಾಶಿ_ಸ್ಥಾನಾಂತರ, ಅಂಕೆ_Arg_Slots);

    jump_Op_ನೇರ(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = ಸ್ಥಾನ(IM3Function);
    IM3FuncType type = ತಕ್ಷಣ(IM3FuncType);
    i32         ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);
    i32         returnSlots = ತಕ್ಷಣ(i32);
    u32         ಅಂಕೆ_Arg_Slots = ತಕ್ಷಣ(u32);

    m3ret_t     r = m3Err_none;

    ಆದರೆ (M3_LIKELY(function)) {
        ಆದರೆ (M3_LIKELY(type == function->funcType)) {
            ಆದರೆ (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            ಆದರೆ (M3_LIKELY(not r)) {
                r = m3_Yield();

                ಆದರೆ (M3_LIKELY(not r)) {
                    d_m3_ಬಾಲ_Call_ವಾದಗಳು(returnSlots, ರಾಶಿ_ಸ್ಥಾನಾಂತರ, ಅಂಕೆ_Arg_Slots);

                    jump_Op_ನೇರ(function->compiled);
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            r = m3_Err_trap_ಪರೋಕ್ಷ_Call_Type_Mismatch;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(Return_Call_ಪರೋಕ್ಷ)
{
    M3_ಕೋಷ್ಟಕ*    ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    u64         ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);
    IM3FuncType type = ತಕ್ಷಣ(IM3FuncType);
    i32         ರಾಶಿ_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);
    i32         returnSlots = ತಕ್ಷಣ(i32);
    u32         ಅಂಕೆ_Arg_Slots = ತಕ್ಷಣ(u32);

    m3ret_t     r = m3Err_none;

    ಆದರೆ (M3_LIKELY(ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2)) {
        IM3Function function = (IM3Function)ಕೋಷ್ಟಕ_2->elements[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ];

        ಆದರೆ (M3_LIKELY(function)) {
            ಆದರೆ (M3_LIKELY(type == function->funcType)) {
                ಆದರೆ (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                ಆದರೆ (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    ಆದರೆ (M3_LIKELY(not r)) {
                        d_m3_ಬಾಲ_Call_ವಾದಗಳು(returnSlots, ರಾಶಿ_ಸ್ಥಾನಾಂತರ, ಅಂಕೆ_Arg_Slots);

                        jump_Op_ನೇರ(function->compiled);
                    }
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                r = m3_Err_trap_ಪರೋಕ್ಷ_Call_Type_Mismatch;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            r = m3_Err_trap_ಕೋಷ್ಟಕ_Element_Is_Null;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        r = m3_Err_trap_ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ_ಹೊರಹೋಗು_ದ_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = ತಕ್ಷಣ(IM3Function);
    ctx.userdata = ತಕ್ಷಣ(ಶೂನ್ಯ*);
    u64* ಸ್ಥಿರಾಂಕ sp = ((u64*)_sp);
    IM3_ಸ್ಮರಣೆ  ಸ್ಮರಣೆ = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#ಆದರೆ d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    ಅಕ್ಷರ        outbuff[1024];
    ಅಕ್ಷರ*       outp = outbuff;
    ಅಕ್ಷರ*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.ಕ್ಷೇತ್ರ_Utf8);

    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ n_ವಾದಗಳು = ftype->ಅಂಕೆ_ವಾದಗಳು;
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ nRets = ftype->ಅಂಕೆ_Rets;
    u64*      ವಾದಗಳು = sp + nRets;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < n_ವಾದಗಳು; i++) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ type = ftype->types[nRets + i];
        ಆಯ್ಕೆ (type) {
        ಸ್ಥಿತಿ c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(ವಾದಗಳು + i)); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(ವಾದಗಳು + i)); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(ವಾದಗಳು + i)); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(ವಾದಗಳು + i)); ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: outp += snprintf(outp, oute - outp, "<type %d>", type); ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (i < n_ವಾದಗಳು - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  ಆದರೆ d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#ಆದರೆ_ಅಂತ್ಯ

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    ಶೂನ್ಯ* ರಾಶಿ_backup = runtime->ರಾಶಿ;
    runtime->ರಾಶಿ = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_ದತ್ತಾಂಶ(ctx.function->host_ಸ್ಮರಣೆ->mallocated));
    runtime->ರಾಶಿ = ರಾಶಿ_backup;

#ಆದರೆ d_m3EnableStrace
    ಆದರೆ (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (ಅಕ್ಷರ*)possible_trap);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ftype and ftype->ಅಂಕೆ_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_m3TracePrint("%s", outbuff);
    }
#ಆದರೆ_ಅಂತ್ಯ

    ಆದರೆ (M3_UNLIKELY(possible_trap)) {
        _mem = ಸ್ಮರಣೆ->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        ಆದರೆ (possible_trap != m3Err_pendingException) {
            ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_ಗಾತ್ರ)
{
    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = m3MemInfo(_mem);

    _r0 = ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು;

    ಮುಂದಿನ_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_ಸ್ಮರಣೆ  ಸ್ಮರಣೆ = m3MemInfo(_mem);

    i32        ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow = (i32)_r0;
    ಆದರೆ (ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow >= 0) {
        _r0 = (m3reg_t)ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು;

        ಆದರೆ (M3_LIKELY(ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow)) {
            u64      required_ಪುಟಗಳು = ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು + (u32)ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow;

            M3_ಫಲಿತಾಂಶ r = Resize_ಸ್ಮರಣೆ(runtime, ಸ್ಮರಣೆ, required_ಪುಟಗಳು);
            ಆದರೆ (r) {
                _r0 = -1;
            }

            _mem = ಸ್ಮರಣೆ->mallocated;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        _r0 = -1;
    }

    ಮುಂದಿನ_Op();
}


#ಆದರೆ d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_ಸ್ಮರಣೆ  ಸ್ಮರಣೆ = m3MemInfo(_mem);

    u64        ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow = (u64)_r0;
    u64        ಅಂಕೆ_ಪುಟಗಳು = ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು;

    ಆದರೆ (M3_LIKELY(ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        ಆದರೆ (ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow <= ಸ್ಮರಣೆ->max_ಪುಟಗಳು - ಅಂಕೆ_ಪುಟಗಳು and
            not Resize_ಸ್ಮರಣೆ(runtime, ಸ್ಮರಣೆ, ಅಂಕೆ_ಪುಟಗಳು + ಅಂಕೆ_ಪುಟಗಳು_ಗಮ್ಯ_Grow)) {
            _r0 = (m3reg_t)ಅಂಕೆ_ಪುಟಗಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            _r0 = -1;
        }

        _mem = ಸ್ಮರಣೆ->mallocated;
    } ಇಲ್ಲದಿದ್ದರೆ {
        _r0 = (m3reg_t)ಅಂಕೆ_ಪುಟಗಳು;
    }

    ಮುಂದಿನ_Op();
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 ಗಾತ್ರ_2 = (u32)_r0;
    u64 ಮೂಲ = ಸ್ಥಾನ(u32);
    u64 destination = ಸ್ಥಾನ(u32);

    ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(destination, ಗಾತ್ರ_2, _mem))) {
        ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(ಮೂಲ, ಗಾತ್ರ_2, _mem))) {
            u8* dst = m3_Mem_ದತ್ತಾಂಶ(_mem) + destination;
            u8* src = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಮೂಲ;
            memmove(dst, src, (size_t)ಗಾತ್ರ_2);

            ಮುಂದಿನ_Op();
        } ಇಲ್ಲದಿದ್ದರೆ {
            d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(ಮೂಲ, ಗಾತ್ರ_2);
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(destination, ಗಾತ್ರ_2);
    }
}


#ಆದರೆ d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 ಗಾತ್ರ_2 = (u64)_r0;
    u64 ಮೂಲ = ಸ್ಥಾನ(u64);
    u64 destination = ಸ್ಥಾನ(u64);

    ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(destination, ಗಾತ್ರ_2, _mem))) {
        ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(ಮೂಲ, ಗಾತ್ರ_2, _mem))) {
            u8* dst = m3_Mem_ದತ್ತಾಂಶ(_mem) + destination;
            u8* src = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಮೂಲ;
            memmove(dst, src, (size_t)ಗಾತ್ರ_2);

            ಮುಂದಿನ_Op();
        } ಇಲ್ಲದಿದ್ದರೆ {
            d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(ಮೂಲ, ಗಾತ್ರ_2);
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(destination, ಗಾತ್ರ_2);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(ಗಣ_ಸ್ಮರಣೆ)
{
    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = ತಕ್ಷಣ(IM3_ಸ್ಮರಣೆ);

    _mem = ಸ್ಮರಣೆ->mallocated;

    ಮುಂದಿನ_Op();
}


#ಆದರೆ d_m3_Has_Multi_ಸ್ಮರಣೆ

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3_ಸ್ಮರಣೆ       dest_ಸ್ಮರಣೆ = ತಕ್ಷಣ(IM3_ಸ್ಮರಣೆ);
    IM3_ಸ್ಮರಣೆ       ಮೂಲ_ಸ್ಮರಣೆ = ತಕ್ಷಣ(IM3_ಸ್ಮರಣೆ);
    u32             widths = ತಕ್ಷಣ(u32);

    M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* destMem = dest_ಸ್ಮರಣೆ->mallocated;
    M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* ಮೂಲ_Mem = ಮೂಲ_ಸ್ಮರಣೆ->mallocated;

    u64             ಗಾತ್ರ_2 = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             ಮೂಲ = d_m3_Wide_ಕಾರ್ಯಾಂಶ(widths & 0x2);
    u64             destination = d_m3_Wide_ಕಾರ್ಯಾಂಶ(widths & 0x1);

    ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(destination, ಗಾತ್ರ_2, destMem))) {
        ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(ಮೂಲ, ಗಾತ್ರ_2, ಮೂಲ_Mem))) {
            memmove(m3_Mem_ದತ್ತಾಂಶ(destMem) + destination, m3_Mem_ದತ್ತಾಂಶ(ಮೂಲ_Mem) + ಮೂಲ, (size_t)ಗಾತ್ರ_2);

            ಮುಂದಿನ_Op();
        } ಇಲ್ಲದಿದ್ದರೆ {
            d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(ಮೂಲ, ಗಾತ್ರ_2);
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(destination, ಗಾತ್ರ_2);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 ಗಾತ್ರ_2 = (u32)_r0;
    u32 byte = ಸ್ಥಾನ(u32);
    u64 destination = ಸ್ಥಾನ(u32);

    ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(destination, ಗಾತ್ರ_2, _mem))) {
        u8* mem8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + destination;
        memset(mem8, (u8)byte, ಗಾತ್ರ_2);
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(destination, ಗಾತ್ರ_2);
    }
}


#ಆದರೆ d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 ಗಾತ್ರ_2 = (u64)_r0;
    u32 byte = ಸ್ಥಾನ(u32);
    u64 destination = ಸ್ಥಾನ(u64);

    ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(destination, ಗಾತ್ರ_2, _mem))) {
        u8* mem8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)ಗಾತ್ರ_2);
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(destination, ಗಾತ್ರ_2);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_ದತ್ತಾಂಶ_Segment* segment = ತಕ್ಷಣ(M3_ದತ್ತಾಂಶ_Segment*);

    u64            ಗಾತ್ರ_2 = (u32)_r0;
    u64            ಮೂಲ = ಸ್ಥಾನ(u32);
    u64            destination = ಸ್ಥಾನ(u32);

    u64            ಲಭ್ಯ = segment->dropped ? 0 : segment->ಗಾತ್ರ_2;

    ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(destination, ಗಾತ್ರ_2, _mem))) {
        ಆದರೆ (M3_LIKELY(ಮೂಲ <= ಲಭ್ಯ and ಗಾತ್ರ_2 <= ಲಭ್ಯ - ಮೂಲ)) {
            ಸ್ಮರಣೆ_ನಕಲು(m3_Mem_ದತ್ತಾಂಶ(_mem) + destination, segment->ದತ್ತಾಂಶ + ಮೂಲ, (size_t)ಗಾತ್ರ_2);
            ಮುಂದಿನ_Op();
        } ಇಲ್ಲದಿದ್ದರೆ {
            d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(ಮೂಲ, ಗಾತ್ರ_2);
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(destination, ಗಾತ್ರ_2);
    }
}


#ಆದರೆ d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_ದತ್ತಾಂಶ_Segment* segment = ತಕ್ಷಣ(M3_ದತ್ತಾಂಶ_Segment*);

    u64            ಗಾತ್ರ_2 = (u32)_r0;
    u64            ಮೂಲ = ಸ್ಥಾನ(u32);
    u64            destination = ಸ್ಥಾನ(u64);

    u64            ಲಭ್ಯ = segment->dropped ? 0 : segment->ಗಾತ್ರ_2;

    ಆದರೆ (M3_LIKELY(d_m3MemRangeOk(destination, ಗಾತ್ರ_2, _mem))) {
        ಆದರೆ (M3_LIKELY(ಮೂಲ <= ಲಭ್ಯ and ಗಾತ್ರ_2 <= ಲಭ್ಯ - ಮೂಲ)) {
            ಸ್ಮರಣೆ_ನಕಲು(m3_Mem_ದತ್ತಾಂಶ(_mem) + destination, segment->ದತ್ತಾಂಶ + ಮೂಲ, (size_t)ಗಾತ್ರ_2);
            ಮುಂದಿನ_Op();
        } ಇಲ್ಲದಿದ್ದರೆ {
            d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(ಮೂಲ, ಗಾತ್ರ_2);
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        d_ಹೊರಹೋಗು_ದ_Bounds_Mem_Op(destination, ಗಾತ್ರ_2);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMemory64


d_m3Op(ದತ್ತಾಂಶ_Drop)
{
    M3_ದತ್ತಾಂಶ_Segment* segment = ತಕ್ಷಣ(M3_ದತ್ತಾಂಶ_Segment*);
    segment->dropped = true;

    ಮುಂದಿನ_Op();
}


#ಆದರೆ d_m3HasRefTypes

d_m3Op(ಕೋಷ್ಟಕ_Get)
{
    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    u64      index = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);

    ಆದರೆ (M3_LIKELY(index < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2)) {
        _r0 = (u64)(uintptr_t)ಕೋಷ್ಟಕ_2->elements[index];
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        newTrap(m3_Err_trap_ಕೋಷ್ಟಕ_ಹೊರಹೋಗು_ದ_Bounds);
    }
}


d_m3Op(ಕೋಷ್ಟಕ_ಗಣ)
{
    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    ಶೂನ್ಯ*    ಮೌಲ್ಯ = ಸ್ಥಾನ(ಶೂನ್ಯ*);
    u64      index = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);

    ಆದರೆ (M3_LIKELY(index < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2)) {
        ಕೋಷ್ಟಕ_2->elements[index] = ಮೌಲ್ಯ;
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        newTrap(m3_Err_trap_ಕೋಷ್ಟಕ_ಹೊರಹೋಗು_ದ_Bounds);
    }
}


d_m3Op(ಕೋಷ್ಟಕ_ಗಾತ್ರ)
{
    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);

    _r0 = ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2;

    ಮುಂದಿನ_Op();
}


d_m3Op(ಕೋಷ್ಟಕ_Grow)
{
    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    u64      delta = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);
    ಶೂನ್ಯ*    ಮೌಲ್ಯ = ಸ್ಥಾನ(ಶೂನ್ಯ*);

    u32      old_ಗಾತ್ರ = ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2;
    u32      max_ಗಾತ್ರ = ಕೋಷ್ಟಕ_2->max_ಗಾತ್ರ ? ಕೋಷ್ಟಕ_2->max_ಗಾತ್ರ : d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಗಾತ್ರ;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_ಗಾತ್ರ = (delta <= max_ಗಾತ್ರ - old_ಗಾತ್ರ) ? (u64)old_ಗಾತ್ರ + delta
                                                    : (u64)max_ಗಾತ್ರ + 1;

    ಆದರೆ (new_ಗಾತ್ರ == old_ಗಾತ್ರ)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_ಗಾತ್ರ;
        ಮುಂದಿನ_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    ಆದರೆ (new_ಗಾತ್ರ <= max_ಗಾತ್ರ and (not runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ or
                                delta <= runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ - runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ)) {
        ಶೂನ್ಯ** elements = m3_ReallocArray(ಶೂನ್ಯ*, ಕೋಷ್ಟಕ_2->elements, (size_t)new_ಗಾತ್ರ, old_ಗಾತ್ರ);

        ಆದರೆ (elements) {
            runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ += new_ಗಾತ್ರ - old_ಗಾತ್ರ;
            ಕೋಷ್ಟಕ_2->elements = elements;
            ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 = (u32)new_ಗಾತ್ರ;

            ಗಾಗಿ (u32 i = old_ಗಾತ್ರ; i < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2; ++i) {
                ಕೋಷ್ಟಕ_2->elements[i] = ಮೌಲ್ಯ;
            }

            _r0 = old_ಗಾತ್ರ;
            ಮುಂದಿನ_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    ಮುಂದಿನ_Op();
}


d_m3Op(ಕೋಷ್ಟಕ_Init)
{
    M3_ಕೋಷ್ಟಕ*          ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    M3ElementSegment* segment = ತಕ್ಷಣ(M3ElementSegment*);
    u32               ಎಣಿಕೆ_2 = ಸ್ಥಾನ(u32);
    u32               ಮೂಲ = ಸ್ಥಾನ(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);

    u64               ಲಭ್ಯ = segment->dropped ? 0 : segment->ಅಂಕೆ_Elements;

    ಆದರೆ (M3_LIKELY(destination <= ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 and ಎಣಿಕೆ_2 <= ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 - destination and
                  (u64) ಮೂಲ + ಎಣಿಕೆ_2 <= ಲಭ್ಯ)) {
        ಗಾಗಿ (u32 i = 0; i < ಎಣಿಕೆ_2; ++i) {
            ಕೋಷ್ಟಕ_2->elements[destination + i] = segment->resolved[ಮೂಲ + i];
        }

        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        newTrap(m3_Err_trap_ಕೋಷ್ಟಕ_ಹೊರಹೋಗು_ದ_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = ತಕ್ಷಣ(M3ElementSegment*);
    segment->dropped = true;

    ಮುಂದಿನ_Op();
}


d_m3Op(ಕೋಷ್ಟಕ_Copy)
{
    M3_ಕೋಷ್ಟಕ* dst = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    M3_ಕೋಷ್ಟಕ* src = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      ಎಣಿಕೆ_2 = d_m3_Wide_ಕಾರ್ಯಾಂಶ(dst->isTable64 and src->isTable64);
    u64      ಮೂಲ = d_m3_Wide_ಕಾರ್ಯಾಂಶ(src->isTable64);
    u64      destination = d_m3_Wide_ಕಾರ್ಯಾಂಶ(dst->isTable64);

    ಆದರೆ (M3_LIKELY(destination <= dst->ಗಾತ್ರ_2 and ಎಣಿಕೆ_2 <= dst->ಗಾತ್ರ_2 - destination and
                  ಮೂಲ <= src->ಗಾತ್ರ_2 and ಎಣಿಕೆ_2 <= src->ಗಾತ್ರ_2 - ಮೂಲ)) {
        memmove(dst->elements + destination, src->elements + ಮೂಲ, (size_t)ಎಣಿಕೆ_2 * ಗಾತ್ರ(ಶೂನ್ಯ*));

        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        newTrap(m3_Err_trap_ಕೋಷ್ಟಕ_ಹೊರಹೋಗು_ದ_Bounds);
    }
}


d_m3Op(ಕೋಷ್ಟಕ_Fill)
{
    M3_ಕೋಷ್ಟಕ* ಕೋಷ್ಟಕ_2 = ತಕ್ಷಣ(M3_ಕೋಷ್ಟಕ*);
    u64      ಎಣಿಕೆ_2 = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);
    ಶೂನ್ಯ*    ಮೌಲ್ಯ = ಸ್ಥಾನ(ಶೂನ್ಯ*);
    u64      index = d_m3_Wide_ಕಾರ್ಯಾಂಶ(ಕೋಷ್ಟಕ_2->isTable64);

    ಆದರೆ (M3_LIKELY(index <= ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 and ಎಣಿಕೆ_2 <= ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 - index)) {
        ಗಾಗಿ (u64 i = 0; i < ಎಣಿಕೆ_2; ++i) {
            ಕೋಷ್ಟಕ_2->elements[index + i] = ಮೌಲ್ಯ;
        }

        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        newTrap(m3_Err_trap_ಕೋಷ್ಟಕ_ಹೊರಹೋಗು_ದ_Bounds);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = ತಕ್ಷಣ(IM3Function);

    m3ret_t     ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        ಫಲಿತಾಂಶ = CompileFunction(function);
    }

    ಆದರೆ (not ಫಲಿತಾಂಶ) {
        // patch up compiled pc and call rewritten op_Call
        *((ಶೂನ್ಯ**)--_pc) = (ಶೂನ್ಯ*)(function->compiled);
        --_pc;
        ಮುಂದಿನ_Op_ನೇರ();
    }

    newTrap(ಫಲಿತಾಂಶ);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = ತಕ್ಷಣ(IM3Function);

    m3ret_t     ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (M3_UNLIKELY(not function->compiled)) {
        ಫಲಿತಾಂಶ = CompileFunction(function);
    }

    ಆದರೆ (not ಫಲಿತಾಂಶ) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((ಶೂನ್ಯ**)--_pc) = (ಶೂನ್ಯ*)(function->compiled);
        --_pc;
        ಮುಂದಿನ_Op_ನೇರ();
    }

    newTrap(ಫಲಿತಾಂಶ);
}


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
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
ಸ್ಥಿರ
ಶೂನ್ಯ Suspend_Without_ಟ್ಯಾಗ್ (IM3Runtime io_runtime, IM3Continuation io_cont, M3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಬಗೆ i_ಬಗೆ,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  ಆದರೆ d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_ಟ್ಯಾಗ್ = ಖಾಲಿ;
    io_runtime->suspendHandlerCont = ಖಾಲಿ;
    io_runtime->suspendStubPC = ಖಾಲಿ;
    io_runtime->switch_ಗುರಿ = ಖಾಲಿ;
    io_runtime->ಅಂಕೆ_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspend_ಬಿಂದು = (u8)i_ಬಗೆ;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  ಆದರೆ d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->ಅಂಕೆ_Suspend_Results = 0;
}
#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


#ಆದರೆ d_m3HasGasMetering

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
    u32        ವೆಚ್ಚ = ತಕ್ಷಣ(u32);

    runtime->gas_ಉಳಿದ -= (i64)ವೆಚ್ಚ;

    ಆದರೆ (M3_UNLIKELY(runtime->gas_ಉಳಿದ < 0)) {
#  ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        ಆದರೆ (runtime->isSuspendable and runtime->ಸಕ್ರಿಯ_Continuation) {
            runtime->suspendRequested = true;
            ಮುಂದಿನ_Op();
        }
#  endif
        newTrap(m3_Err_trap_ಹೊರಹೋಗು_ದ_Gas);
    }

    ಮುಂದಿನ_Op();
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool ನಮೂದು_ಹೊಂದುತ್ತದೆ (IM3Function i_function, m3stack_t _sp, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* _mem)
{
#ಆದರೆ d_m3_Skip_ರಾಶಿ_Check
    (ಶೂನ್ಯ)i_function;
    (ಶೂನ್ಯ)_sp;
    (ಶೂನ್ಯ)_mem;
    ಹಿಂತಿರುಗಿ true;
#ಅಥವಾಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->ಸಕ್ರಿಯ_Continuation : ಖಾಲಿ);
    ಶೂನ್ಯ*           max_ರಾಶಿ = (_act && _act->val_ರಾಶಿ)
                                 ? (ಶೂನ್ಯ*)(_act->val_ರಾಶಿ + _act->ಅಂಕೆ_ರಾಶಿ_Slots)
                                 : (_mem ? _mem->max_ರಾಶಿ : (ಶೂನ್ಯ*)~(uintptr_t)0);
    ಹಿಂತಿರುಗಿ M3_LIKELY((ಶೂನ್ಯ*)(_sp + i_function->max_ರಾಶಿ_Slots) < max_ರಾಶಿ);
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ M3_LIKELY((ಶೂನ್ಯ*)(_sp + i_function->max_ರಾಶಿ_Slots) < _mem->max_ರಾಶಿ);
#ಆದರೆ_ಅಂತ್ಯ
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ Enter_ಚೌಕಟ್ಟು (IM3Function i_function, m3stack_t _sp)
{
#ಆದರೆ defined(DEBUG)
    i_function->ದೊರಕಿದವು++;
#ಆದರೆ_ಅಂತ್ಯ
    u8* ರಾಶಿ = (u8*)((m3slot_t*)_sp + i_function->ಅಂಕೆ_Ret_And_Arg_Slots);

    memset(ರಾಶಿ, 0x0, i_function->ಅಂಕೆ_Local_ಬೈಟುಗಳು);
    ರಾಶಿ += i_function->ಅಂಕೆ_Local_ಬೈಟುಗಳು;

    ಆದರೆ (i_function->constants) {
        ಸ್ಮರಣೆ_ನಕಲು(ರಾಶಿ, i_function->constants, i_function->ಅಂಕೆ_Constant_ಬೈಟುಗಳು);
    }
}


d_m3Op(ನಮೂದು_2)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = ತಕ್ಷಣ(IM3Function);
#ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = m3MemInfo(_mem);
#ಆದರೆ_ಅಂತ್ಯ

    ಆದರೆ (ನಮೂದು_ಹೊಂದುತ್ತದೆ(function, _sp, _mem)) {
        Enter_ಚೌಕಟ್ಟು(function, _sp);

#ಆದರೆ !d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        ಮುಂದಿನ_Op_ನೇರ();
#ಇಲ್ಲದಿದ್ದರೆ

#  ಆದರೆ d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->ಅಂಕೆ_Ret_Slots));
        trace_rt->call_ಆಳ++;
#  endif

        m3ret_t r = ಮುಂದಿನ_Op_Impl();

#  ಆದರೆ d_m3EnableStrace >= 2
        trace_rt->call_ಆಳ--;

        ಆದರೆ (r) {
            d_m3TracePrint("} !trap = %s", (ಅಕ್ಷರ*)r);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Get_Function_ಅಂಕೆ_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } ಇಲ್ಲದಿದ್ದರೆ {
            d_m3TracePrint("}");
        }
#  endif

        ಆದರೆ (M3_UNLIKELY(r)) {
            _mem = ಸ್ಮರಣೆ->mallocated;
#  ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
            ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                ಹಿಂತಿರುಗಿ ದಾಖಲೆ_ನಮೂದು_ಚೌಕಟ್ಟು(m3MemRuntime(_mem), _pc, _sp, ಸ್ಮರಣೆ, function);
            }
#  endif
            fill_Backtrace_ಚೌಕಟ್ಟು();
        }
        forwardTrap(r);
#ಆದರೆ_ಅಂತ್ಯ // d_m3EntryKeepsFrame
    } ಇಲ್ಲದಿದ್ದರೆ {
        newTrap(m3_Err_trap_ರಾಶಿ_ಅತಿಪ್ರವಾಹ);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_ರಾಶಿ();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = m3MemInfo(_mem);

    ಮಾಡು {
#ಆದರೆ d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_ಆಳ++;
#ಆದರೆ_ಅಂತ್ಯ
        r = ಮುಂದಿನ_Op_Impl();

#ಆದರೆ d_m3EnableStrace >= 3
        trace_rt->call_ಆಳ--;
        d_m3TracePrint("}");
#ಆದರೆ_ಅಂತ್ಯ
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = ಸ್ಮರಣೆ->mallocated;
    } ಆಗ (r == _pc);

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        ಹಿಂತಿರುಗಿ ದಾಖಲೆ_Loop_ಚೌಕಟ್ಟು(m3MemRuntime(_mem), _pc, _sp, ಸ್ಮರಣೆ);
    }
#ಆದರೆ_ಅಂತ್ಯ

    forwardTrap(r);
}


#ಆದರೆ d_m3HasExceptionHandling

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
d_m3Op(Try_ಕೋಷ್ಟಕ)
{
    d_m3_Check_Native_ರಾಶಿ();

    IM3_ಸ್ಮರಣೆ  ಸ್ಮರಣೆ = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        ಅಂಕೆ_Clauses = ತಕ್ಷಣ(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * ಅಂಕೆ_Clauses;

    u32        ಆಳ = ++runtime->try_ಆಳ;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = ಸ್ಮರಣೆ->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_ಆಳ == ಆಳ);
    runtime->try_ಆಳ = ಆಳ - 1;

#  ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        ಹಿಂತಿರುಗಿ ದಾಖಲೆ_Try_ಚೌಕಟ್ಟು(runtime, clauses, _sp, ಸ್ಮರಣೆ, ಅಂಕೆ_Clauses, isInnermost);
    }
#  endif

    ಆದರೆ (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ಸ್ಥಿರಾಂಕ M3Exception* exception = runtime->pendingException;

        ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; ++i) {
            IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = (IM3_ಟ್ಯಾಗ್)clauses[2 * i];

            ಆದರೆ (ಟ್ಯಾಗ್ == ಖಾಲಿ or ಟ್ಯಾಗ್ == exception->ಟ್ಯಾಗ್) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jump_Op_ನೇರ(handler);
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

    u32          ಅಂಕೆ_ವಾದಗಳು = ತಕ್ಷಣ(u32);

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        i32 offset = ತಕ್ಷಣ(i32);
        u32 is64 = ತಕ್ಷಣ(u32);
        u64 ಮೌಲ್ಯ = exception->ವಾದಗಳು[i];

        ಆದರೆ (is64) {
            *(u64*)(_sp + offset) = ಮೌಲ್ಯ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            *(u32*)(_sp + offset) = (u32)ಮೌಲ್ಯ;
        }
    }

    i32 ref_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);

    ಆದರೆ (ref_ಸ್ಥಾನಾಂತರ >= 0) {
        *(M3Exception**)(_sp + ref_ಸ್ಥಾನಾಂತರ) = exception;
        exception->reified = true;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (not exception->reified) {
        FreeException(runtime, exception);
    }

    ಮುಂದಿನ_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_ಟ್ಯಾಗ್       ಟ್ಯಾಗ್ = ತಕ್ಷಣ(IM3_ಟ್ಯಾಗ್);
    u32          ಅಂಕೆ_ವಾದಗಳು = ತಕ್ಷಣ(u32);

    M3Exception* exception = NewException(runtime, ಟ್ಯಾಗ್, ಅಂಕೆ_ವಾದಗಳು);

    ಆದರೆ (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        i32 offset = ತಕ್ಷಣ(i32);
        u32 is64 = ತಕ್ಷಣ(u32);

        exception->ವಾದಗಳು[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    ಹಿಂತಿರುಗಿ m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = ಸ್ಥಾನ(M3Exception*);

    ಆದರೆ (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    ಹಿಂತಿರುಗಿ m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(ತೆಗೆ_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        ಎಣಿಕೆ_2 = ತಕ್ಷಣ(u32);

    runtime->try_ಆಳ -= ಎಣಿಕೆ_2;

    ಮುಂದಿನ_Op();
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = ತಕ್ಷಣ(IM3FuncType);
    i32         dst_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
    i32         func_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);

    IM3Function func = *(IM3Function*)(_sp + func_ಸ್ಥಾನ);
    ಆದರೆ (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    ಆದರೆ (M3_UNLIKELY(not Continuation_Can_Acquire_ರಾಶಿ(runtime))) {
        newTrap(m3_Err_continuation_ಎಲ್ಲೆ_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    ಆದರೆ (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_ಫಲಿತಾಂಶ ಹಂಚಿಕೆ = Continuation_Acquire_ರಾಶಿ(runtime, cont);
    ಆದರೆ (M3_UNLIKELY(ಹಂಚಿಕೆ)) {
        cont->state = cont_consumed;
        newTrap(ಹಂಚಿಕೆ);
    }
    *(IM3Continuation*)(_sp + dst_ಸ್ಥಾನ) = cont;
    ಮುಂದಿನ_Op();
}


ಸ್ಥಿರ
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = ತಕ್ಷಣ(IM3FuncType);
    IM3FuncType     ct2 = ತಕ್ಷಣ(IM3FuncType);
    i32             dst_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
    i32             cont_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
    u32             ಅಂಕೆ_Bound = ತಕ್ಷಣ(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + cont_ಸ್ಥಾನ);
    ಆದರೆ (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ಆದರೆ (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->ನಮೂದು_Function);
    ಆದರೆ (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->ನಮೂದು_Function = srcCont->ನಮೂದು_Function;
    dstCont->val_ರಾಶಿ = srcCont->val_ರಾಶಿ;
    dstCont->ಅಂಕೆ_ರಾಶಿ_Slots = srcCont->ಅಂಕೆ_ರಾಶಿ_Slots;
    dstCont->ಚೌಕಟ್ಟುಗಳು = srcCont->ಚೌಕಟ್ಟುಗಳು;
    dstCont->ಚೌಕಟ್ಟುಗಳು_Cap = srcCont->ಚೌಕಟ್ಟುಗಳು_Cap;
    dstCont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = srcCont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspend_ಬಿಂದು = srcCont->suspend_ಬಿಂದು;
    dstCont->r0 = srcCont->r0;
#  ಆದರೆ d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_ವಾದಗಳು_ಎಣಿಕೆ = srcCont->bound_ವಾದಗಳು_ಎಣಿಕೆ;
    dstCont->state = srcCont->state;
#  ಆದರೆ d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = ಖಾಲಿ;
#  endif

    dstCont->ಅಂಕೆ_Suspend_Results = srcCont->ಅಂಕೆ_Suspend_Results;
    ಗಾಗಿ (u32 j = 0; j < srcCont->ಅಂಕೆ_Suspend_Results; ++j) {
        dstCont->suspend_ಫಲಿತಾಂಶ_Offsets[j] = srcCont->suspend_ಫಲಿತಾಂಶ_Offsets[j];
        dstCont->suspend_ಫಲಿತಾಂಶ_Is64[j] = srcCont->suspend_ಫಲಿತಾಂಶ_Is64[j];
    }

    ಆದರೆ (dstCont->state == cont_allocated) {
        u16 ಅಂಕೆ_Rets = ct1->contFuncType ? Get_Func_Type_ಅಂಕೆ_Results(ct1->contFuncType) : 0;
        ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Bound; ++i) {
            i32 arg_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
            u32 is64 = ತಕ್ಷಣ(u32);
            u32 ಗುರಿ_Idx = (ಅಂಕೆ_Rets + dstCont->bound_ವಾದಗಳು_ಎಣಿಕೆ) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;
            ಆದರೆ (is64) {
                *(u64*)(dstCont->val_ರಾಶಿ + ಗುರಿ_Idx) = *(u64*)(_sp + arg_ಸ್ಥಾನ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                *(u32*)(dstCont->val_ರಾಶಿ + ಗುರಿ_Idx) = *(u32*)(_sp + arg_ಸ್ಥಾನ);
            }
            dstCont->bound_ವಾದಗಳು_ಎಣಿಕೆ++;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        IM3Continuation inner = InnermostSuspended(dstCont);
        ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Bound; ++i) {
            i32 arg_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
            u32 is64 = ತಕ್ಷಣ(u32);
            u32 dstIdx = dstCont->bound_ವಾದಗಳು_ಎಣಿಕೆ;
            ಆದರೆ (dstIdx < inner->ಅಂಕೆ_Suspend_Results) {
                i32 dst_ಸ್ಥಾನಾಂತರ = inner->suspend_ಫಲಿತಾಂಶ_Offsets[dstIdx];
                ಆದರೆ (is64) {
                    *(u64*)(inner->sp + dst_ಸ್ಥಾನಾಂತರ) = *(u64*)(_sp + arg_ಸ್ಥಾನ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    *(u32*)(inner->sp + dst_ಸ್ಥಾನಾಂತರ) = *(u32*)(_sp + arg_ಸ್ಥಾನ);
                }
            }
            dstCont->bound_ವಾದಗಳು_ಎಣಿಕೆ++;
        }
    }

    srcCont->val_ರಾಶಿ = ಖಾಲಿ;
    srcCont->ಚೌಕಟ್ಟುಗಳು = ಖಾಲಿ;
    srcCont->ಚೌಕಟ್ಟುಗಳು_Cap = 0;
    srcCont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dst_ಸ್ಥಾನ) = dstCont;
    ಮುಂದಿನ_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
ಸ್ಥಿರ
IM3Continuation ಕಂಡುಹಿಡಿ_Handler (IM3Continuation i_cont, IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್, u8 i_ಬಗೆ, pc_t* o_stubPC)
{
    IM3_ಟ್ಯಾಗ್ query_ಟ್ಯಾಗ್ = (i_ಟ್ಯಾಗ್ and i_ಟ್ಯಾಗ್->resolved) ? i_ಟ್ಯಾಗ್->resolved : i_ಟ್ಯಾಗ್;

    ಗಾಗಿ (IM3Continuation cont = i_cont; cont; cont = cont->ಪೋಷಕ) {
        ಗಾಗಿ (u32 h = 0; h < cont->ಅಂಕೆ_Handlers; ++h) {
            pc_t   ನಮೂದು = cont->handlersPC + (h * 3);
            IM3_ಟ್ಯಾಗ್ handler_ಟ್ಯಾಗ್ = (IM3_ಟ್ಯಾಗ್)(*(ನಮೂದು + 1));
            ಆದರೆ (handler_ಟ್ಯಾಗ್ and handler_ಟ್ಯಾಗ್->resolved) {
                handler_ಟ್ಯಾಗ್ = handler_ಟ್ಯಾಗ್->resolved;
            }

            ಆದರೆ ((u32)(uintptr_t)(*ನಮೂದು) == i_ಬಗೆ and handler_ಟ್ಯಾಗ್ == query_ಟ್ಯಾಗ್) {
                ಆದರೆ (o_stubPC) {
                    *o_stubPC = (pc_t)(*(ನಮೂದು + 2));
                }
                ಹಿಂತಿರುಗಿ cont;
            }
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
ಸ್ಥಿರ
ಶೂನ್ಯ ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    ಗಾಗಿ (u32 i = 0; i < io_cont->ಅಂಕೆ_Suspend_Results; ++i) {
        ಆದರೆ (io_cont->suspend_ಫಲಿತಾಂಶ_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_ಫಲಿತಾಂಶ_Offsets[i]) = 0;
        } ಇಲ್ಲದಿದ್ದರೆ {
            *(m3slot_t*)(i_sp + io_cont->suspend_ಫಲಿತಾಂಶ_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->ಸಕ್ರಿಯ_Continuation;
    ಆದರೆ (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ಟ್ಯಾಗ್);
    }

    IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = ತಕ್ಷಣ(IM3_ಟ್ಯಾಗ್);
    u32    ಅಂಕೆ_Params = ತಕ್ಷಣ(u32);

    runtime->ಅಂಕೆ_Suspend_Payload = ಅಂಕೆ_Params;
    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Params; ++i) {
        i32 param_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
        u32 is64 = ತಕ್ಷಣ(u32);
        ಆದರೆ (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + param_ಸ್ಥಾನ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + param_ಸ್ಥಾನ);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 ಅಂಕೆ_Results = ತಕ್ಷಣ(u32);
    cont->ಅಂಕೆ_Suspend_Results = ಅಂಕೆ_Results;
    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Results; ++i) {
        cont->suspend_ಫಲಿತಾಂಶ_Offsets[i] = ತಕ್ಷಣ(i32);
        cont->suspend_ಫಲಿತಾಂಶ_Is64[i] = ತಕ್ಷಣ(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = ಖಾಲಿ;
    IM3Continuation ಹುಡುಕು_Cont = ಕಂಡುಹಿಡಿ_Handler(cont, ಟ್ಯಾಗ್, 0, &matchedStubPC);

    ಆದರೆ (M3_UNLIKELY(not ಹುಡುಕು_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ಟ್ಯಾಗ್);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = ಹುಡುಕು_Cont;
    runtime->switch_ಗುರಿ = ಖಾಲಿ;
    runtime->suspend_ಟ್ಯಾಗ್ = ಟ್ಯಾಗ್;

    cont->pc = _pc;
    cont->suspend_ಬಿಂದು = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  ಆದರೆ d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_ವಾದಗಳು_ಎಣಿಕೆ = 0;

    ಹಿಂತಿರುಗಿ m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
ಸ್ಥಿರ
m3ret_t ಸತತ_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_ಅಂಕೆ_Handlers, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ** io_mem);
ಸ್ಥಿರ
ಶೂನ್ಯ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_ಅಂಕೆ_Results);
ಸ್ಥಿರ
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    ಹಿಂತಿರುಗಿ pass and (i_cont->suspend_ಬಿಂದು == safepoint_op or i_cont->suspend_ಬಿಂದು == safepoint_ನಮೂದು);
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
ಸ್ಥಿರ
m3ret_t Replay_ಚೌಕಟ್ಟುಗಳು (IM3Continuation i_cont, i32 i_ಆಳ, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  ಆದರೆ d_m3_Max_Native_ರಾಶಿ > 0
    {
        ಶೂನ್ಯ* ಎಲ್ಲೆ = runtime->ರಾಶಿ_ಎಲ್ಲೆ;

        ಆದರೆ (M3_UNLIKELY(ಎಲ್ಲೆ and m3_Native_ರಾಶಿ_Ptr() < ಎಲ್ಲೆ)) {
            ಹಿಂತಿರುಗಿ m3_Err_trap_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
        }
    }
#  endif

    ಆದರೆ (i_ಆಳ < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  ಆದರೆ d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        ಆದರೆ (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = ಖಾಲಿ;

            ಹಿಂತಿರುಗಿ m3Err_pendingException;
        }
#  endif
        ಆದರೆ (pastCheck) {
            ಹಿಂತಿರುಗಿ ResumePastCheck(i_cont, _mem);
        }

        ಹಿಂತಿರುಗಿ d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು = i_cont->ಚೌಕಟ್ಟುಗಳು[i_ಆಳ];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_ಚೌಕಟ್ಟು_ಬಗೆ)ಚೌಕಟ್ಟು.ಬಗೆ == ಚೌಕಟ್ಟು_resume);

#  ಆದರೆ d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_ಚೌಕಟ್ಟು_ಬಗೆ)ಚೌಕಟ್ಟು.ಬಗೆ == ಚೌಕಟ್ಟು_try);
#  endif

    ಆದರೆ (not ownDescent) {
        r = Replay_ಚೌಕಟ್ಟುಗಳು(i_cont, i_ಆಳ - 1, _mem);
        _mem = ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ->mallocated;
    }

    ಆಯ್ಕೆ ((M3_ಚೌಕಟ್ಟು_ಬಗೆ)ಚೌಕಟ್ಟು.ಬಗೆ) {
    ಸ್ಥಿತಿ ಚೌಕಟ್ಟು_call:
        ಆದರೆ (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            ಹಿಂತಿರುಗಿ d_m3CallWithRegs(ಚೌಕಟ್ಟು.pc, ಚೌಕಟ್ಟು.sp, _mem, ಚೌಕಟ್ಟು.call.r0, ಚೌಕಟ್ಟು.call.fp0);
        }

        ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(runtime, &ಚೌಕಟ್ಟು);
        }
#  ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
        ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು(runtime, ಚೌಕಟ್ಟು.pc - 1);
#  endif
        ಹಿಂತಿರುಗಿ r;

    ಸ್ಥಿತಿ ಚೌಕಟ್ಟು_loop: {
        m3stack_t _sp = ಚೌಕಟ್ಟು.sp;
        m3reg_t   _r0 = 0;
#  ಆದರೆ d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        ಆಗ (r == (m3ret_t)ಚೌಕಟ್ಟು.pc) {
            r = jumpOpImpl(ಚೌಕಟ್ಟು.pc);
            _mem = ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ->mallocated;
        }

        ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(runtime, &ಚೌಕಟ್ಟು);
        }
        ಹಿಂತಿರುಗಿ r;
    }

#  ಆದರೆ d_m3HasExceptionHandling
    ಸ್ಥಿತಿ ಚೌಕಟ್ಟು_try: {
        m3stack_t _sp = ಚೌಕಟ್ಟು.sp;
        m3reg_t   _r0 = 0;
#    ಆದರೆ d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (ಶೂನ್ಯ)_sp;
        (ಶೂನ್ಯ)_r0;
#    ಆದರೆ d_m3HasFloat
        (ಶೂನ್ಯ)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 ಆಳ = ++runtime->try_ಆಳ;

        ಆದರೆ (not ಚೌಕಟ್ಟು.try_.handlers_ಜೀವಂತ) {
            runtime->try_ಆಳ = ಆಳ - 1;
        }

        r = Replay_ಚೌಕಟ್ಟುಗಳು(i_cont, i_ಆಳ - 1, _mem);
        _mem = ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ->mallocated;

        bool isInnermost = (runtime->try_ಆಳ == ಆಳ);
        runtime->try_ಆಳ = ಆಳ - 1;

        ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ಚೌಕಟ್ಟು.try_.handlers_ಜೀವಂತ = isInnermost;
            ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(runtime, &ಚೌಕಟ್ಟು);
        }

        ಆದರೆ (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ಸ್ಥಿರಾಂಕ M3Exception* exception = runtime->pendingException;

            ಗಾಗಿ (u32 i = 0; i < ಚೌಕಟ್ಟು.try_.ಅಂಕೆ_Clauses; ++i) {
                IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = (IM3_ಟ್ಯಾಗ್)ಚೌಕಟ್ಟು.pc[2 * i];

                ಆದರೆ (ಟ್ಯಾಗ್ == ಖಾಲಿ or ಟ್ಯಾಗ್ == exception->ಟ್ಯಾಗ್) {
                    pc_t handler = (pc_t)ಚೌಕಟ್ಟು.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    ಹಿಂತಿರುಗಿ jumpOpImpl(handler);
                }
            }
        }
        ಹಿಂತಿರುಗಿ r;
    }
#  endif

    ಸ್ಥಿತಿ ಚೌಕಟ್ಟು_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = ಚೌಕಟ್ಟು.resume.cont;

        r = ಸತತ_Under_Handlers(runtime, &inner, ಚೌಕಟ್ಟು.resume.handlersPC,
                             ಚೌಕಟ್ಟು.resume.ಅಂಕೆ_Handlers, &_mem);

        ಆದರೆ (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, ಚೌಕಟ್ಟು.sp, ಚೌಕಟ್ಟು.resume.resultsPC,
                                    ಚೌಕಟ್ಟು.resume.ಅಂಕೆ_Results);
            Continuation_Recycle_ರಾಶಿ(runtime, inner);

            ಹಿಂತಿರುಗಿ d_m3CallWithRegs(ಚೌಕಟ್ಟು.pc, ಚೌಕಟ್ಟು.sp, _mem, 0, 0.);
        }

        ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ಆದರೆ (runtime->suspendHandlerCont != inner) {
                ಚೌಕಟ್ಟು.resume.cont = inner;
                ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(runtime, &ಚೌಕಟ್ಟು);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            ಆದರೆ (M3_UNLIKELY(not suspended)) {
                ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            ಆದರೆ (runtime->suspendStubPC) {
                ಹಿಂತಿರುಗಿ d_m3CallWithRegs(runtime->suspendStubPC, ಚೌಕಟ್ಟು.sp, _mem, 0, 0.);
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಹಿಂತಿರುಗಿ m3_Err_trap_Unhandled_Control_ಟ್ಯಾಗ್;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            inner->state = cont_consumed;
            Continuation_Recycle_ರಾಶಿ(runtime, inner);
        }

        ಹಿಂತಿರುಗಿ r;
    }

#  ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
    ಸ್ಥಿತಿ ಚೌಕಟ್ಟು_ನಮೂದು:
        ಆದರೆ (M3_UNLIKELY(r)) {
            ಆದರೆ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(runtime, &ಚೌಕಟ್ಟು);
            }
#    ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
            FillBacktraceFunctionInfo(runtime, ಚೌಕಟ್ಟು.ನಮೂದು.function);
#    ಇಲ್ಲದಿದ್ದರೆ
            (ಶೂನ್ಯ)ಚೌಕಟ್ಟು.ನಮೂದು.function;
#    endif
        }
        ಹಿಂತಿರುಗಿ r;
#  endif
    }

    ಹಿಂತಿರುಗಿ r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
ಸ್ಥಿರ
ಶೂನ್ಯ* ContinuationArgDest (IM3Continuation i_cont, u32 i_ಸೂಚ್ಯಂಕ)
{
    ಆದರೆ (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : ಖಾಲಿ;
        u16         ಅಂಕೆ_Rets = inner ? Get_Func_Type_ಅಂಕೆ_Results(inner) : 0;

        ಹಿಂತಿರುಗಿ i_cont->val_ರಾಶಿ + (ಅಂಕೆ_Rets + i_ಸೂಚ್ಯಂಕ) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;
    }

    ಆದರೆ (i_ಸೂಚ್ಯಂಕ < i_cont->ಅಂಕೆ_Suspend_Results) {
        ಹಿಂತಿರುಗಿ i_cont->sp + i_cont->suspend_ಫಲಿತಾಂಶ_Offsets[i_ಸೂಚ್ಯಂಕ];
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
ಸ್ಥಿರ
ಶೂನ್ಯ Hand_Over_ಗಮ್ಯ_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 ಅಂಕೆ_ವಾದಗಳು = i_runtime->ಅಂಕೆ_Suspend_Payload;
    u32 ತಳ = (io_peer->state == cont_allocated) ? io_peer->bound_ವಾದಗಳು_ಎಣಿಕೆ : 0;

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        ಶೂನ್ಯ* dest = ContinuationArgDest(io_peer, ತಳ + i);

        ಆದರೆ (dest) {
            ಆದರೆ (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } ಇಲ್ಲದಿದ್ದರೆ {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    ಶೂನ್ಯ* refDest = ContinuationArgDest(io_peer, ತಳ + ಅಂಕೆ_ವಾದಗಳು);

    ಆದರೆ (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    ಆದರೆ (io_peer->state == cont_allocated) {
        io_peer->bound_ವಾದಗಳು_ಎಣಿಕೆ += ಅಂಕೆ_ವಾದಗಳು + 1;
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
ಸ್ಥಿರ
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    ಆಗ (i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು and (M3_ಚೌಕಟ್ಟು_ಬಗೆ) i_cont->ಚೌಕಟ್ಟುಗಳು[0].ಬಗೆ == ಚೌಕಟ್ಟು_resume) {
        i_cont = i_cont->ಚೌಕಟ್ಟುಗಳು[0].resume.cont;
    }

    ಹಿಂತಿರುಗಿ i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
ಸ್ಥಿರ
m3ret_t ಸತತ_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_ಅಂಕೆ_Handlers, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation ಪೋಷಕ = i_runtime->ಸಕ್ರಿಯ_Continuation;
    IM3_ಸ್ಮರಣೆ       ಸ್ಮರಣೆ = m3MemInfo(*io_mem);
    m3ret_t         r;

    ಗಾಗಿ (;;) {
#  ಆದರೆ d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        ಆದರೆ (M3_UNLIKELY(cont->resumeThrow and cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = ಖಾಲಿ;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            ನಿಲ್ಲಿಸು;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        ಆದರೆ (isStarting) {
            ಆದರೆ (M3_UNLIKELY(not cont->ನಮೂದು_Function->compiled)) {
                r = CompileFunction(cont->ನಮೂದು_Function);

                ಆದರೆ (M3_UNLIKELY(r)) {
                    ನಿಲ್ಲಿಸು;
                }
            }
            cont->pc = cont->ನಮೂದು_Function->compiled;
            cont->sp = cont->val_ರಾಶಿ;
            cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->ಅಂಕೆ_Handlers = i_ಅಂಕೆ_Handlers;
        cont->ಪೋಷಕ = ಪೋಷಕ;
        cont->state = cont_running;
        cont->bound_ವಾದಗಳು_ಎಣಿಕೆ = 0;

        i_runtime->ಸಕ್ರಿಯ_Continuation = cont;

        ಆದರೆ (cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು - 1;
            cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;

            r = Replay_ಚೌಕಟ್ಟುಗಳು(cont, outermost, *io_mem);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } ಇಲ್ಲದಿದ್ದರೆ {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = ಸ್ಮರಣೆ->mallocated;
        i_runtime->ಸಕ್ರಿಯ_Continuation = ಪೋಷಕ;

        ಆದರೆ (r != m3Err_continuationSuspended) {
            ನಿಲ್ಲಿಸು;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        ಆದರೆ (not i_runtime->switch_ಗುರಿ or i_runtime->suspendHandlerCont != cont) {
            ನಿಲ್ಲಿಸು;
        }

        IM3Continuation peer = i_runtime->switch_ಗುರಿ;
        i_runtime->switch_ಗುರಿ = ಖಾಲಿ;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        ಆದರೆ (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            ನಿಲ್ಲಿಸು;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_Cont_Type_ದ_Ref(peer->ನಮೂದು_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_ಅಂಕೆ_Params(peerType) - 1)));
        Hand_Over_ಗಮ್ಯ_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    ಹಿಂತಿರುಗಿ r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
ಸ್ಥಿರ
ಶೂನ್ಯ TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_ಅಂಕೆ_Results)
{
    ಗಾಗಿ (u32 i = 0; i < i_ಅಂಕೆ_Results; ++i) {
        i32 res_ಸ್ಥಾನ = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;

        ಆದರೆ (is64) {
            *(u64*)(i_sp + res_ಸ್ಥಾನ) = *(u64*)(i_cont->val_ರಾಶಿ + srcIdx);
        } ಇಲ್ಲದಿದ್ದರೆ {
            *(u32*)(i_sp + res_ಸ್ಥಾನ) = *(u32*)(i_cont->val_ರಾಶಿ + srcIdx);
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
    IM3FuncType     contFuncType = ತಕ್ಷಣ(IM3FuncType);
    i32             cont_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
    u32             ಬಹುಲಕ = ತಕ್ಷಣ(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + cont_ಸ್ಥಾನ);
    ಆದರೆ (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ಆದರೆ (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         ಅಂಕೆ_Rets = Get_Func_Type_ಅಂಕೆ_Results(innerType);

    ಆದರೆ (ಬಹುಲಕ == d_m3ResumeThrowRef) {
        i32 exn_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);

#  ಆದರೆ d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exn_ಸ್ಥಾನ);

        ಆದರೆ (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  ಇಲ್ಲದಿದ್ದರೆ
        (ಶೂನ್ಯ)exn_ಸ್ಥಾನ;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಹುಲಕ == d_m3ResumeThrow) {
        IM3_ಟ್ಯಾಗ್ exn_ಟ್ಯಾಗ್ = ತಕ್ಷಣ(IM3_ಟ್ಯಾಗ್);
        u32    ಅಂಕೆ_ವಾದಗಳು = ತಕ್ಷಣ(u32);

#  ಆದರೆ d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_ಟ್ಯಾಗ್, ಅಂಕೆ_ವಾದಗಳು);

        ಆದರೆ (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
            i32 offset = ತಕ್ಷಣ(i32);
            u32 is64 = ತಕ್ಷಣ(u32);

            exception->ವಾದಗಳು[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  ಇಲ್ಲದಿದ್ದರೆ
        (ಶೂನ್ಯ)exn_ಟ್ಯಾಗ್;
        (ಶೂನ್ಯ)ಅಂಕೆ_ವಾದಗಳು;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } ಇಲ್ಲದಿದ್ದರೆ {
        u32 ಅಂಕೆ_ವಾದಗಳು = ತಕ್ಷಣ(u32);

        ಆದರೆ (cont->state == cont_allocated) {
            ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
                i32 arg_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
                u32 is64 = ತಕ್ಷಣ(u32);
                u32 ಗುರಿ_Idx = (ಅಂಕೆ_Rets + cont->bound_ವಾದಗಳು_ಎಣಿಕೆ + i) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ;
                ಆದರೆ (is64) {
                    *(u64*)(cont->val_ರಾಶಿ + ಗುರಿ_Idx) = *(u64*)(_sp + arg_ಸ್ಥಾನ);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    *(u32*)(cont->val_ರಾಶಿ + ಗುರಿ_Idx) = *(u32*)(_sp + arg_ಸ್ಥಾನ);
                }
            }
            cont->bound_ವಾದಗಳು_ಎಣಿಕೆ = 0;
        } ಇಲ್ಲದಿದ್ದರೆ {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
                i32 arg_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
                u32 is64 = ತಕ್ಷಣ(u32);
                u32 dstIdx = cont->bound_ವಾದಗಳು_ಎಣಿಕೆ + i;
                ಆದರೆ (dstIdx < inner->ಅಂಕೆ_Suspend_Results) {
                    i32 dst_ಸ್ಥಾನಾಂತರ = inner->suspend_ಫಲಿತಾಂಶ_Offsets[dstIdx];
                    ಆದರೆ (is64) {
                        *(u64*)(inner->sp + dst_ಸ್ಥಾನಾಂತರ) = *(u64*)(_sp + arg_ಸ್ಥಾನ);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        *(u32*)(inner->sp + dst_ಸ್ಥಾನಾಂತರ) = *(u32*)(_sp + arg_ಸ್ಥಾನ);
                    }
                }
            }
        }
    }

    u32  ಅಂಕೆ_Handlers = ತಕ್ಷಣ(u32);
    pc_t handlersPC = _pc;
    _pc += ಅಂಕೆ_Handlers * 3;

    u32       ಅಂಕೆ_Results = ತಕ್ಷಣ(u32);
    pc_t      resultsPC = _pc;

    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = m3MemInfo(_mem);

    m3ret_t   r = ಸತತ_Under_Handlers(runtime, &cont, handlersPC, ಅಂಕೆ_Handlers, &_mem);

    ಆದರೆ (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, ಅಂಕೆ_Results);
        Continuation_Recycle_ರಾಶಿ(runtime, cont);

        _pc += ಅಂಕೆ_Results * 2;
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (r == m3Err_continuationSuspended) {
        ಆದರೆ (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            ಆದರೆ (M3_UNLIKELY(not runtime->ಸಕ್ರಿಯ_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_ಟ್ಯಾಗ್);
            }
            M3_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;
            ಚೌಕಟ್ಟು.ಬಗೆ = ಚೌಕಟ್ಟು_resume;
            ಚೌಕಟ್ಟು.pc = resultsPC + ಅಂಕೆ_Results * 2;
            ಚೌಕಟ್ಟು.sp = _sp;
            ಚೌಕಟ್ಟು.ಸ್ಮರಣೆ = ಸ್ಮರಣೆ;
            ಚೌಕಟ್ಟು.resume.cont = cont;
            ಚೌಕಟ್ಟು.resume.handlersPC = handlersPC;
            ಚೌಕಟ್ಟು.resume.ಅಂಕೆ_Handlers = ಅಂಕೆ_Handlers;
            ಚೌಕಟ್ಟು.resume.resultsPC = resultsPC;
            ಚೌಕಟ್ಟು.resume.ಅಂಕೆ_Results = ಅಂಕೆ_Results;

            ಹಿಂತಿರುಗಿ Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು(runtime, &ಚೌಕಟ್ಟು);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        ಆದರೆ (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        ಆದರೆ (runtime->suspendStubPC) {
            jump_Op_ನೇರ(runtime->suspendStubPC);
        } ಇಲ್ಲದಿದ್ದರೆ {
            newTrap(m3_Err_trap_Unhandled_Control_ಟ್ಯಾಗ್);
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        cont->state = cont_consumed;
        Continuation_Recycle_ರಾಶಿ(runtime, cont);
        ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು_2();
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
    cont->type = ತಕ್ಷಣ(IM3FuncType);

    u32 ಅಂಕೆ_ವಾದಗಳು = ತಕ್ಷಣ(u32);
    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        i32 offset = ತಕ್ಷಣ(i32);
        u32 is64 = ತಕ್ಷಣ(u32);
        u64 val = (i < runtime->ಅಂಕೆ_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        ಆದರೆ (is64) {
            *(u64*)(_sp + offset) = val;
        } ಇಲ್ಲದಿದ್ದರೆ {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_ಸ್ಥಾನಾಂತರ = ತಕ್ಷಣ(i32);
    ಆದರೆ (cont_ಸ್ಥಾನಾಂತರ >= 0) {
        *(IM3Continuation*)(_sp + cont_ಸ್ಥಾನಾಂತರ) = cont;
    }

    ಮುಂದಿನ_Op();
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
    IM3Continuation cont = runtime->ಸಕ್ರಿಯ_Continuation;

    ಆದರೆ (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ಟ್ಯಾಗ್);
    }

    skip_ತಕ್ಷಣ(IM3FuncType);

    IM3_ಟ್ಯಾಗ್          ಟ್ಯಾಗ್ = ತಕ್ಷಣ(IM3_ಟ್ಯಾಗ್);
    i32             ಗುರಿ_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);

    IM3Continuation ಗುರಿ = *(IM3Continuation*)(_sp + ಗುರಿ_ಸ್ಥಾನ);

    ಆದರೆ (M3_UNLIKELY(not ಗುರಿ)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ಆದರೆ (M3_UNLIKELY(ಗುರಿ->state == cont_consumed or ಗುರಿ->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = ಕಂಡುಹಿಡಿ_Handler(cont, ಟ್ಯಾಗ್, 1, ಖಾಲಿ);

    ಆದರೆ (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ಟ್ಯಾಗ್);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 ಅಂಕೆ_ವಾದಗಳು = ತಕ್ಷಣ(u32);

    runtime->ಅಂಕೆ_Suspend_Payload = ಅಂಕೆ_ವಾದಗಳು;

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
        i32 arg_ಸ್ಥಾನ = ತಕ್ಷಣ(i32);
        u32 is64 = ತಕ್ಷಣ(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + arg_ಸ್ಥಾನ) : (u64) * (u32*)(_sp + arg_ಸ್ಥಾನ);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 ಅಂಕೆ_Results = ತಕ್ಷಣ(u32);

    cont->ಅಂಕೆ_Suspend_Results = ಅಂಕೆ_Results;

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Results; ++i) {
        cont->suspend_ಫಲಿತಾಂಶ_Offsets[i] = ತಕ್ಷಣ(i32);
        cont->suspend_ಫಲಿತಾಂಶ_Is64[i] = ತಕ್ಷಣ(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = ಖಾಲಿ;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switch_ಗುರಿ = ಗುರಿ;
    runtime->suspend_ಟ್ಯಾಗ್ = ಟ್ಯಾಗ್;

    cont->pc = _pc;
    cont->suspend_ಬಿಂದು = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  ಆದರೆ d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    ಹಿಂತಿರುಗಿ m3Err_continuationSuspended;
}


#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = ತಕ್ಷಣ(pc_t);

    ಆದರೆ (condition) {
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = ಸ್ಥಾನ(i32);

    pc_t elsePC = ತಕ್ಷಣ(pc_t);

    ಆದರೆ (condition) {
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_ಕೋಷ್ಟಕ)
{
    u32   branch_ಸೂಚ್ಯಂಕ = ಸ್ಥಾನ(u32);           // branch index is always in a slot
    u32   ಅಂಕೆ_Targets = ತಕ್ಷಣ(u32);

    pc_t* branches = (pc_t*)_pc;

    ಆದರೆ (branch_ಸೂಚ್ಯಂಕ > ಅಂಕೆ_Targets) {
        branch_ಸೂಚ್ಯಂಕ = ಅಂಕೆ_Targets; // the default index
    }

    jumpOp(branches[branch_ಸೂಚ್ಯಂಕ]);
}


#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ಗಣ_ರಿಜಿಸ್ಟರ್_ಗಣ_ಸ್ಥಾನ(TYPE, REG) \
d_m3Op  (ಗಣ_ರಿಜಿಸ್ಟರ್_2##TYPE)            \
{                                       \
    TYPE * src = ಸ್ಥಾನ_ptr (TYPE);       \
    d_m3_Preload_ಮುಂದಿನ ();                 \
    REG = * src;                        \
    ಮುಂದಿನ_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (ಗಣ_ಸ್ಥಾನ_2##TYPE)                 \
{                                       \
    TYPE * dst = ಸ್ಥಾನ_ptr (TYPE);       \
    d_m3_Preload_ಮುಂದಿನ ();                 \
    * dst = (TYPE) REG;                 \
    ಮುಂದಿನ_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_ಗಣ_ಸ್ಥಾನ_2##TYPE)         \
{                                       \
    TYPE * ರಾಶಿ     = ಸ್ಥಾನ_ptr (TYPE); \
    TYPE * preserve  = ಸ್ಥಾನ_ptr (TYPE); \
    d_m3_Preload_ಮುಂದಿನ ();                 \
                                        \
    * preserve = * ರಾಶಿ;               \
    * ರಾಶಿ = (TYPE) REG;               \
                                        \
    ಮುಂದಿನ_Op_Preloaded ();                 \
}

d_m3_ಗಣ_ರಿಜಿಸ್ಟರ್_ಗಣ_ಸ್ಥಾನ(i32, _r0)
d_m3_ಗಣ_ರಿಜಿಸ್ಟರ್_ಗಣ_ಸ್ಥಾನ(i64, _r0)
#ಆದರೆ d_m3HasFloat
d_m3_ಗಣ_ರಿಜಿಸ್ಟರ್_ಗಣ_ಸ್ಥಾನ(f32, _fp0)
d_m3_ಗಣ_ರಿಜಿಸ್ಟರ್_ಗಣ_ಸ್ಥಾನ(f64, _fp0)
#ಆದರೆ_ಅಂತ್ಯ

d_m3Op(Copy_ಸ್ಥಾನ_32)
{
    u32* dst = ಸ್ಥಾನ_ptr(u32);
    u32* src = ಸ್ಥಾನ_ptr(u32);
    d_m3_Preload_ಮುಂದಿನ();

    *dst = *src;

    ಮುಂದಿನ_Op_Preloaded();
}


d_m3Op(Preserve_Copy_ಸ್ಥಾನ_32)
{
    u32* dest = ಸ್ಥಾನ_ptr(u32);
    u32* src = ಸ್ಥಾನ_ptr(u32);
    u32* preserve = ಸ್ಥಾನ_ptr(u32);
    d_m3_Preload_ಮುಂದಿನ();

    *preserve = *dest;
    *dest = *src;

    ಮುಂದಿನ_Op_Preloaded();
}


d_m3Op(Copy_ಸ್ಥಾನ_64)
{
    u64* dst = ಸ್ಥಾನ_ptr(u64);
    u64* src = ಸ್ಥಾನ_ptr(u64);
    d_m3_Preload_ಮುಂದಿನ();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    ಮುಂದಿನ_Op_Preloaded();
}


d_m3Op(Preserve_Copy_ಸ್ಥಾನ_64)
{
    u64* dest = ಸ್ಥಾನ_ptr(u64);
    u64* src = ಸ್ಥಾನ_ptr(u64);
    u64* preserve = ಸ್ಥಾನ_ptr(u64);
    d_m3_Preload_ಮುಂದಿನ();

    *preserve = *dest;
    *dest = *src;

    ಮುಂದಿನ_Op_Preloaded();
}


#ಆದರೆ d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_ರಾಶಿ)
{
    u32         opcode_ಸೂಚ್ಯಂಕ = ತಕ್ಷಣ(u32);
    u32         ರಾಶಿ_ಎತ್ತರ = ತಕ್ಷಣ(u32);
    IM3Function function = ತಕ್ಷಣ(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    ಬರೆ(" %4d ", opcode_ಸೂಚ್ಯಂಕ);
    ಬರೆ(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  ಆದರೆ d_m3HasFloat
    ಬರೆ("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    ಗಾಗಿ (u32 i = 0; i < ರಾಶಿ_ಎತ್ತರ; ++i) {
        cstr_t ಬಗೆ = "";

        ಬರೆ("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, ಬಗೆ, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    ಬರೆ("---------------------------------------------------------------------------------------------------------\n");

    ಮುಂದಿನ_Op_ನೇರ();
}
#ಆದರೆ_ಅಂತ್ಯ


#ವ್ಯಾಖ್ಯಾನಿಸು d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = ಸ್ಥಾನ (TYPE);                \
    TYPE operand1 = ಸ್ಥಾನ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ಮುಂದಿನ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = ಸ್ಥಾನ (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = ಸ್ಥಾನ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ಮುಂದಿನ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = ಸ್ಥಾನ (i32);                 \
                                                \
    TYPE operand2 = ಸ್ಥಾನ (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ಮುಂದಿನ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = ಸ್ಥಾನ (i32);                 \
                                                \
    TYPE operand2 = ಸ್ಥಾನ (TYPE);                \
    TYPE operand1 = ಸ್ಥಾನ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ಮುಂದಿನ_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#ವ್ಯಾಖ್ಯಾನಿಸು d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = ಸ್ಥಾನ (TYPE);                \
    TYPE operand1 = ಸ್ಥಾನ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ಮುಂದಿನ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = ಸ್ಥಾನ (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ಮುಂದಿನ_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = ಸ್ಥಾನ (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    ಮುಂದಿನ_Op ();                                  \
}

#ಆದರೆ d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, ಸ್ಥಾನ(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, ಸ್ಥಾನ(i32))
#ಆದರೆ_ಅಂತ್ಯ

d_m3Op(Return)
{
    m3_ರಾಶಿ_Check();
    ಹಿಂತಿರುಗಿ m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = ತಕ್ಷಣ(pc_t);

    ಆದರೆ (condition) {
        jumpOp(branch);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಮುಂದಿನ_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = ಸ್ಥಾನ(i32);
    pc_t branch = ತಕ್ಷಣ(pc_t);

    ಆದರೆ (condition) {
        jumpOp(branch);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಮುಂದಿನ_Op();
    }
}

#ಆದರೆ d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                             \
      pc_t branch  = ತಕ್ಷಣ (pc_t);                        \
      ಆದರೆ (ಕಾರ್ಯಾಂಶ OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      ಇಲ್ಲದಿದ್ದರೆ ಮುಂದಿನ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                             \
      pc_t branch  = ತಕ್ಷಣ (pc_t);                        \
      ಆದರೆ (((TYPE) _r0) OPER ಕಾರ್ಯಾಂಶ)                          \
          jumpOp (branch);                                    \
      ಇಲ್ಲದಿದ್ದರೆ ಮುಂದಿನ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = ಸ್ಥಾನ (TYPE);                            \
      TYPE operand1 = ಸ್ಥಾನ (TYPE);                            \
      pc_t branch   = ತಕ್ಷಣ (pc_t);                       \
      ಆದರೆ (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      ಇಲ್ಲದಿದ್ದರೆ ಮುಂದಿನ_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                             \
      pc_t elsePC  = ತಕ್ಷಣ (pc_t);                        \
      ಆದರೆ (ಕಾರ್ಯಾಂಶ OPER ((TYPE) _r0))                          \
          ಮುಂದಿನ_Op ();                                          \
      ಇಲ್ಲದಿದ್ದರೆ jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (TYPE);                             \
      pc_t elsePC  = ತಕ್ಷಣ (pc_t);                        \
      ಆದರೆ (((TYPE) _r0) OPER ಕಾರ್ಯಾಂಶ)                          \
          ಮುಂದಿನ_Op ();                                          \
      ಇಲ್ಲದಿದ್ದರೆ jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = ಸ್ಥಾನ (TYPE);                            \
      TYPE operand1 = ಸ್ಥಾನ (TYPE);                            \
      pc_t elsePC   = ತಕ್ಷಣ (pc_t);                       \
      ಆದರೆ (operand1 OPER operand2)                             \
          ಮುಂದಿನ_Op ();                                          \
      ಇಲ್ಲದಿದ್ದರೆ jumpOp (elsePC);                                   \
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
    pc_t branch = ತಕ್ಷಣ(pc_t);
    ಆದರೆ (((i32)_r0) == 0) {
        jumpOp(branch);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಮುಂದಿನ_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ(i32);
    pc_t branch = ತಕ್ಷಣ(pc_t);
    ಆದರೆ (ಕಾರ್ಯಾಂಶ == 0) {
        jumpOp(branch);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಮುಂದಿನ_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = ತಕ್ಷಣ(pc_t);
    ಆದರೆ (((i32)_r0) == 0) {
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ(i32);
    pc_t elsePC = ತಕ್ಷಣ(pc_t);
    ಆದರೆ (ಕಾರ್ಯಾಂಶ == 0) {
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        jumpOp(elsePC);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = ತಕ್ಷಣ(pc_t);

    ಆದರೆ (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = ಸ್ಥಾನ(i32);
    pc_t branch = ತಕ್ಷಣ(pc_t);

    ಆದರೆ (condition) {
        ಮುಂದಿನ_Op();
    } ಇಲ್ಲದಿದ್ದರೆ {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_ರಾಶಿ_Check();

    ಶೂನ್ಯ* loopId = ತಕ್ಷಣ(ಶೂನ್ಯ*);
    ಹಿಂತಿರುಗಿ loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    ಶೂನ್ಯ* loopId = ತಕ್ಷಣ(ಶೂನ್ಯ*);

    ಆದರೆ (condition) {
        ಹಿಂತಿರುಗಿ loopId;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಮುಂದಿನ_Op();
    }
}


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_ರಾಶಿ_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    ಆದರೆ (M3_UNLIKELY(runtime->suspendRequested and runtime->ಸಕ್ರಿಯ_Continuation)) {
        Suspend_Without_ಟ್ಯಾಗ್(runtime, runtime->ಸಕ್ರಿಯ_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_ವಾದಗಳು(_r0, _fp0));

        ಹಿಂತಿರುಗಿ m3Err_continuationSuspended;
    }

    ಶೂನ್ಯ* loopId = ತಕ್ಷಣ(ಶೂನ್ಯ*);
    ಹಿಂತಿರುಗಿ loopId;
}


#  ಆದರೆ !d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(ನಮೂದು_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = ತಕ್ಷಣ(IM3Function);

    ಆದರೆ (ನಮೂದು_ಹೊಂದುತ್ತದೆ(function, _sp, _mem)) {
        Enter_ಚೌಕಟ್ಟು(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        ಆದರೆ (M3_UNLIKELY(runtime->suspendRequested and runtime->ಸಕ್ರಿಯ_Continuation)) {
            Suspend_Without_ಟ್ಯಾಗ್(runtime, runtime->ಸಕ್ರಿಯ_Continuation, safepoint_ನಮೂದು, _pc, _sp,
                              d_m3_Exp_Reg_ವಾದಗಳು(_r0, _fp0));

            ಹಿಂತಿರುಗಿ m3Err_continuationSuspended;
        }

        ಮುಂದಿನ_Op_ನೇರ();
    } ಇಲ್ಲದಿದ್ದರೆ {
        newTrap(m3_Err_trap_ರಾಶಿ_ಅತಿಪ್ರವಾಹ);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(ನಮೂದು_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    ಆದರೆ (M3_UNLIKELY(runtime->suspendRequested and runtime->ಸಕ್ರಿಯ_Continuation)) {
        Suspend_Without_ಟ್ಯಾಗ್(runtime, runtime->ಸಕ್ರಿಯ_Continuation, safepoint_ನಮೂದು, _pc - 1, _sp,
                          d_m3_Exp_Reg_ವಾದಗಳು(_r0, _fp0));

        ಹಿಂತಿರುಗಿ m3Err_continuationSuspended;
    }

    ಮುಂದಿನ_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    ಶೂನ್ಯ* loopId = ತಕ್ಷಣ(ಶೂನ್ಯ*);

    ಆದರೆ (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        ಆದರೆ (M3_UNLIKELY(runtime->suspendRequested and runtime->ಸಕ್ರಿಯ_Continuation)) {
            Suspend_Without_ಟ್ಯಾಗ್(runtime, runtime->ಸಕ್ರಿಯ_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_ವಾದಗಳು(_r0, _fp0));

            ಹಿಂತಿರುಗಿ m3Err_continuationSuspended;
        }
        ಹಿಂತಿರುಗಿ loopId;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಮುಂದಿನ_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
ಸ್ಥಿರ
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  ಆದರೆ d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    ಆದರೆ (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (op == op_ನಮೂದು_Check) {
        ಹಿಂತಿರುಗಿ ಮುಂದಿನ_Op_Impl();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  ಆದರೆ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    ಹಿಂತಿರುಗಿ op(_pc, d_m3_Op_ವಾದಗಳು, d_m3_ತಳ_Cstr);
#  ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ op(_pc, d_m3_Op_ವಾದಗಳು);
#  endif
}
#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 ಮೌಲ್ಯ = *(u32*)_pc++;
    ಸ್ಥಾನ(u32) = ಮೌಲ್ಯ;
    ಮುಂದಿನ_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 ಮೌಲ್ಯ;
    ಸ್ಮರಣೆ_ನಕಲು(&ಮೌಲ್ಯ, _pc, ಗಾತ್ರ(ಮೌಲ್ಯ));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    ಸ್ಥಾನ(u64) = ಮೌಲ್ಯ;
    ಮುಂದಿನ_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_ರಾಶಿ_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(ಕೊನೆ_2)
{
    m3_ರಾಶಿ_Check();
    ಹಿಂತಿರುಗಿ m3Err_none;
}


d_m3Op(ಗಣ_Global_s32)
{
    u32* global = ತಕ್ಷಣ(u32*);
    *global = ಸ್ಥಾನ(u32);

    ಮುಂದಿನ_Op();
}


d_m3Op(ಗಣ_Global_s64)
{
    u64* global = ತಕ್ಷಣ(u64*);
    *global = ಸ್ಥಾನ(u64);

    ಮುಂದಿನ_Op();
}

#ಆದರೆ d_m3HasFloat
d_m3Op(ಗಣ_Global_f32)
{
    f32* global = ತಕ್ಷಣ(f32*);
    *global = (f32)_fp0;

    ಮುಂದಿನ_Op();
}


d_m3Op(ಗಣ_Global_f64)
{
    f64* global = ತಕ್ಷಣ(f64*);
    *global = _fp0;

    ಮುಂದಿನ_Op();
}
#ಆದರೆ_ಅಂತ್ಯ


#ಆದರೆ d_m3HasMemory64

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
    u64  ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ(u64);
    u32* ವಿಳಾಸ = ಸ್ಥಾನ_ptr(u32);

    u64  offset;
    ಸ್ಮರಣೆ_ನಕಲು(&offset, _pc, ಗಾತ್ರ(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    ಆದರೆ (M3_LIKELY(ಕಾರ್ಯಾಂಶ < d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ)) {
        u64 effective = ಕಾರ್ಯಾಂಶ + offset;

        // the access itself checks its own size against what is left
        ಆದರೆ (M3_LIKELY(effective < _mem->ಉದ್ದಳತೆ)) {
            *ವಿಳಾಸ = (u32)effective;
            ಮುಂದಿನ_Op();
        }
    }

    d_ಹೊರಹೋಗು_ದ_Bounds;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#ಆದರೆ d_m3_Skip_ಸ್ಮರಣೆ_Bounds_Check || d_m3_Guarded_ಸ್ಮರಣೆ
#  define m3MemCheck(x) true
#ಇಲ್ಲದಿದ್ದರೆ
#  define m3MemCheck(x) M3_LIKELY(x)
#ಆದರೆ_ಅಂತ್ಯ

// memcpy here is to support non-aligned access on some platforms.

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತುಂಬು(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = ತಕ್ಷಣ (u32);                       \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    u64 ಕಾರ್ಯಾಂಶ = (u32) _r0;                            \
    ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                        \
    ಆದರೆ (m3MemCheck(                                     \
        ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (SRC_TYPE) <= _mem->ಉದ್ದಳತೆ     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;       \
            SRC_TYPE ಮೌಲ್ಯ;                             \
            ಸ್ಮರಣೆ_ನಕಲು(&ಮೌಲ್ಯ, src8, ಗಾತ್ರ(ಮೌಲ್ಯ));        \
            M3_BSWAP_##SRC_TYPE(ಮೌಲ್ಯ);                 \
            REG = (DEST_TYPE)ಮೌಲ್ಯ;                     \
            d_m3_Trace_ತುಂಬು(DEST_TYPE, ಕಾರ್ಯಾಂಶ, REG);     \
        }                                               \
        ಮುಂದಿನ_Op_Preloaded ();                             \
    } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (u32);                           \
    u32 offset = ತಕ್ಷಣ (u32);                       \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                        \
    ಆದರೆ (m3MemCheck(                                     \
        ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (SRC_TYPE) <= _mem->ಉದ್ದಳತೆ     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;       \
            SRC_TYPE ಮೌಲ್ಯ;                             \
            ಸ್ಮರಣೆ_ನಕಲು(&ಮೌಲ್ಯ, src8, ಗಾತ್ರ(ಮೌಲ್ಯ));        \
            M3_BSWAP_##SRC_TYPE(ಮೌಲ್ಯ);                 \
            REG = (DEST_TYPE)ಮೌಲ್ಯ;                     \
            d_m3_Trace_ತುಂಬು(DEST_TYPE, ಕಾರ್ಯಾಂಶ, REG);     \
        }                                               \
        ಮುಂದಿನ_Op_Preloaded ();                             \
    } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತುಂಬು_i(DEST_TYPE, SRC_TYPE) d_m3_ತುಂಬು(_r0, DEST_TYPE, SRC_TYPE)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ತುಂಬು_f(DEST_TYPE, SRC_TYPE) d_m3_ತುಂಬು(_fp0, DEST_TYPE, SRC_TYPE)

#ಆದರೆ d_m3HasFloat
d_m3_ತುಂಬು_f(f32, f32);
d_m3_ತುಂಬು_f(f64, f64);
#ಆದರೆ_ಅಂತ್ಯ

d_m3_ತುಂಬು_i(i32, i8);
d_m3_ತುಂಬು_i(i32, u8);
d_m3_ತುಂಬು_i(i32, i16);
d_m3_ತುಂಬು_i(i32, u16);
d_m3_ತುಂಬು_i(i32, i32);

d_m3_ತುಂಬು_i(i64, i8);
d_m3_ತುಂಬು_i(i64, u8);
d_m3_ತುಂಬು_i(i64, i16);
d_m3_ತುಂಬು_i(i64, u16);
d_m3_ತುಂಬು_i(i64, i32);
d_m3_ತುಂಬು_i(i64, u32);
d_m3_ತುಂಬು_i(i64, i64);

#ಆದರೆ d_m3_Fold_ಗಣ_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_ತುಂಬು_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = ತಕ್ಷಣ (u32);                       \
      DEST_TYPE * dest = ಸ್ಥಾನ_ptr (DEST_TYPE);            \
      d_m3_Preload_ಮುಂದಿನ ();                                 \
      u64 ಕಾರ್ಯಾಂಶ = (u32) _r0;                            \
      ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                          \
      ಆದರೆ (m3MemCheck(                                     \
          ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (SRC_TYPE) <= _mem->ಉದ್ದಳತೆ     \
      )) {                                                \
          u8* src8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;           \
          SRC_TYPE ಮೌಲ್ಯ;                                 \
          ಸ್ಮರಣೆ_ನಕಲು(&ಮೌಲ್ಯ, src8, ಗಾತ್ರ(ಮೌಲ್ಯ));            \
          M3_BSWAP_##SRC_TYPE(ಮೌಲ್ಯ);                     \
          DEST_TYPE ಫಲಿತಾಂಶ = (DEST_TYPE)ಮೌಲ್ಯ;            \
          * dest = ಫಲಿತಾಂಶ;  REG = ಫಲಿತಾಂಶ;                 \
          d_m3_Trace_ತುಂಬು(DEST_TYPE, ಕಾರ್ಯಾಂಶ, ಫಲಿತಾಂಶ);      \
          ಮುಂದಿನ_Op_Preloaded ();                             \
      } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (u32);                           \
      u32 offset = ತಕ್ಷಣ (u32);                       \
      DEST_TYPE * dest = ಸ್ಥಾನ_ptr (DEST_TYPE);            \
      d_m3_Preload_ಮುಂದಿನ ();                                 \
      ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                          \
      ಆದರೆ (m3MemCheck(                                     \
          ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (SRC_TYPE) <= _mem->ಉದ್ದಳತೆ     \
      )) {                                                \
          u8* src8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;           \
          SRC_TYPE ಮೌಲ್ಯ;                                 \
          ಸ್ಮರಣೆ_ನಕಲು(&ಮೌಲ್ಯ, src8, ಗಾತ್ರ(ಮೌಲ್ಯ));            \
          M3_BSWAP_##SRC_TYPE(ಮೌಲ್ಯ);                     \
          DEST_TYPE ಫಲಿತಾಂಶ = (DEST_TYPE)ಮೌಲ್ಯ;            \
          * dest = ಫಲಿತಾಂಶ;  REG = ಫಲಿತಾಂಶ;                 \
          d_m3_Trace_ತುಂಬು(DEST_TYPE, ಕಾರ್ಯಾಂಶ, ಫಲಿತಾಂಶ);      \
          ಮುಂದಿನ_Op_Preloaded ();                             \
      } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
  }

d_m3_ತುಂಬು_Fold(_r0, i32, i8)
d_m3_ತುಂಬು_Fold(_r0, i32, u8)
d_m3_ತುಂಬು_Fold(_r0, i32, i16)
d_m3_ತುಂಬು_Fold(_r0, i32, u16)
d_m3_ತುಂಬು_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_ತುಂಬು_Fold(_r0, i64, i8)
d_m3_ತುಂಬು_Fold(_r0, i64, u8)
d_m3_ತುಂಬು_Fold(_r0, i64, i16)
d_m3_ತುಂಬು_Fold(_r0, i64, u16)
d_m3_ತುಂಬು_Fold(_r0, i64, i32)
d_m3_ತುಂಬು_Fold(_r0, i64, u32)
d_m3_ತುಂಬು_Fold(_r0, i64, i64)
#  ಆದರೆ d_m3HasFloat
d_m3_ತುಂಬು_Fold(_fp0, f32, f32)
d_m3_ತುಂಬು_Fold(_fp0, f64, f64)
#  endif

#ಆದರೆ_ಅಂತ್ಯ // d_m3FoldSetLocal

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (u32);                           \
    u32 offset = ತಕ್ಷಣ (u32);                       \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                        \
    ಆದರೆ (m3MemCheck(                                     \
        ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (DEST_TYPE) <= _mem->ಉದ್ದಳತೆ    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, ಕಾರ್ಯಾಂಶ, REG);     \
            u8* mem8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            ಸ್ಮರಣೆ_ನಕಲು(mem8, &val, ಗಾತ್ರ(val));            \
        }                                               \
        ಮುಂದಿನ_Op_Preloaded ();                             \
    } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ಸ್ಥಿರಾಂಕ SRC_TYPE ಮೌಲ್ಯ = ಸ್ಥಾನ (SRC_TYPE);             \
    u64 ಕಾರ್ಯಾಂಶ = (u32) _r0;                            \
    u32 offset = ತಕ್ಷಣ (u32);                       \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                        \
    ಆದರೆ (m3MemCheck(                                     \
        ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (DEST_TYPE) <= _mem->ಉದ್ದಳತೆ    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, ಕಾರ್ಯಾಂಶ, ಮೌಲ್ಯ);   \
            u8* mem8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;       \
            DEST_TYPE val = (DEST_TYPE) ಮೌಲ್ಯ;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            ಸ್ಮರಣೆ_ನಕಲು(mem8, &val, ಗಾತ್ರ(val));            \
        }                                               \
        ಮುಂದಿನ_Op_Preloaded ();                             \
    } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ಸ್ಥಿರಾಂಕ SRC_TYPE ಮೌಲ್ಯ = ಸ್ಥಾನ (SRC_TYPE);             \
    u64 ಕಾರ್ಯಾಂಶ = ಸ್ಥಾನ (u32);                           \
    u32 offset = ತಕ್ಷಣ (u32);                       \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                        \
    ಆದರೆ (m3MemCheck(                                     \
        ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (DEST_TYPE) <= _mem->ಉದ್ದಳತೆ    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, ಕಾರ್ಯಾಂಶ, ಮೌಲ್ಯ);   \
            u8* mem8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;       \
            DEST_TYPE val = (DEST_TYPE) ಮೌಲ್ಯ;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            ಸ್ಮರಣೆ_ನಕಲು(mem8, &val, ಗಾತ್ರ(val));            \
        }                                               \
        ಮುಂದಿನ_Op_Preloaded ();                             \
    } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
}

// both operands can be in regs when storing a float
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 ಕಾರ್ಯಾಂಶ = (u32) _r0;                            \
    u32 offset = ತಕ್ಷಣ (u32);                       \
    d_m3_Preload_ಮುಂದಿನ ();                                 \
    ಕಾರ್ಯಾಂಶ += offset;                                  \
                                                        \
    ಆದರೆ (m3MemCheck(                                     \
        ಕಾರ್ಯಾಂಶ + ಗಾತ್ರ (TYPE) <= _mem->ಉದ್ದಳತೆ         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, ಕಾರ್ಯಾಂಶ, REG);         \
            u8* mem8 = m3_Mem_ದತ್ತಾಂಶ(_mem) + ಕಾರ್ಯಾಂಶ;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            ಸ್ಮರಣೆ_ನಕಲು(mem8, &val, ಗಾತ್ರ(val));            \
        }                                               \
        ಮುಂದಿನ_Op_Preloaded ();                             \
    } ಇಲ್ಲದಿದ್ದರೆ d_ಹೊರಹೋಗು_ದ_Bounds;                               \
}


#ವ್ಯಾಖ್ಯಾನಿಸು d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#ಆದರೆ d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#ಆದರೆ_ಅಂತ್ಯ

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#ಅನ್ತರ್ಭಾವ "m3_exec_atomic.h"

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#ಆದರೆ d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    ಅಕ್ಷರ name[100];
    strcpy(name, ಅಕ್ಷರಸರಣಿ_ಹುಡುಕು(i_opcode, "op_") + 3);
    ಅಕ್ಷರ* bracket = ಅಕ್ಷರಸರಣಿ_ಹುಡುಕು(name, "(");
    ಆದರೆ (bracket) {
        *bracket = 0;
    }

    puts(name);
    ಮುಂದಿನ_Op_ನೇರ();
}
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_ದೊರಕಿತು(i_operationName);

    ಮುಂದಿನ_Op_ನೇರ();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_ದೊರಕಿತು(i_operationName);

    ಮುಂದಿನ_Op_ನೇರ();
}
#ಆದರೆ_ಅಂತ್ಯ

d_m3_ಕೊನೆ_Extern_C

#ಆದರೆ_ಅಂತ್ಯ // m3_exec_h
