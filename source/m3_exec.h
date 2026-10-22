//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#ifndef m3_exec_h
#define m3_exec_h

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

#ifndef M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#endif

#অন্তর্ভাৱ "m3_math_utils.h"
#অন্তর্ভাৱ "m3_compile.h"
#অন্তর্ভাৱ "m3_env.h"
#অন্তর্ভাৱ "m3_info.h"
#অন্তর্ভাৱ "m3_exec_defs.h"
#অন্তর্ভাৱ "m3_atomic.h"
#অন্তর্ভাৱ "m3_host.h"

#অন্তর্ভাৱ <limits.h>

d_m3BeginExternC

#define rewrite_op(OP)             * ((শূন্য **) (_pc-1)) = (শূন্য*)(OP)

#define immediate(TYPE)            * ((TYPE *) _pc++)
#define skip_immediate(TYPE)       (_pc++)

#define slot(TYPE)                 * (TYPE *) (_sp + immediate (i32))
#define slot_ptr(TYPE)             (TYPE *) (_sp + immediate (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#define d_m3WideOperand(IS64)      ((IS64) ? slot (u64) : (u64) slot (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#define d_m3MemRangeOk(START, LENGTH, MEM)  \
   ((START) <= (MEM)->length and (LENGTH) <= (MEM)->length - (START))


#যদি d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define nextOp()                 M3_MUSTTAIL ঘূৰাই profileOp (d_m3OpAllArgs, __FUNCTION__)
#নোচেদ্যদি d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define nextOp()                 M3_MUSTTAIL ঘূৰাই debugOp (d_m3OpAllArgs, __FUNCTION__)
#অন্যথা
#  define nextOp()                 nextOpDirect()
#endif

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#যদি d_m3PreloadNextOp && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3PreloadNext()        ধ্রুৱ IM3Operation _nextOp = (IM3Operation) (* _pc)
#  define nextOpPreloaded()        M3_MUSTTAIL ঘূৰাই _nextOp (_pc + 1, d_m3OpArgs)
#অন্যথা
#  define d_m3PreloadNext()
#  define nextOpPreloaded()        nextOp()
#endif

#যদি d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL ঘূৰাই profileJumpOp ((pc_t)(PC), d_m3OpArgs, __FUNCTION__)
#অন্যথা
#  define jumpOp(PC)               jumpOpDirect(PC)
#endif

#যদি d_m3RecordBacktraces
#  define ঠেলো_Backtrace_Frame_2()            (ঠেলো_Backtrace_Frame (_mem->runtime, _pc - 1))
#  define fillBacktraceFrame(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    ঘূৰাই (ঠেলো_Backtrace_Frame_2 (), err)
#  define forwardTrap(err)                ঘূৰাই err
#অন্যথা
#  define ঠেলো_Backtrace_Frame_2()            কৰা {} যেতিয়ালৈ (0)
#  define fillBacktraceFrame(FUNCTION)    কৰা {} যেতিয়ালৈ (0)

#  define newTrap(err)                    ঘূৰাই err
#  define forwardTrap(err)                ঘূৰাই err
#endif

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#যদি d_m3_Max_Native_স্তূপ > 0
#  define d_m3_Check_Native_স্তূপ()                                               \
       কৰা {                                                                    \
           শূন্য * _m3Limit = m3MemRuntime (_mem)->স্তূপ_Limit;                  \
           যদি (M3_UNLIKELY (_m3Limit && m3_Native_স্তূপ_Ptr () < _m3Limit))      \
               newTrap (m3_Err_trap_স্তূপ_Overflow);                              \
       } যেতিয়ালৈ (0)
#অন্যথা
#  define d_m3_Check_Native_স্তূপ()           কৰা {} যেতিয়ালৈ (0)
#endif


#যদি d_m3_Has_স্তূপ_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  যদি d_m3HasFloat
#    define d_m3ExpRegArgs(R0, FP0)        (R0), (FP0)
#  অন্যথা
#    define d_m3ExpRegArgs(R0, FP0)        (R0)
#  endif

#  যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3ExpRegArgs (R0, FP0), d_m3BaseCstr)
#  অন্যথা
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3ExpRegArgs (R0, FP0))
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
স্থির M3_NOINLINE
m3ret_t RecordCallFrame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3Memory i_memory,
                         IM3Function i_function, m3reg_t i_r0
#  যদি d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3Frame frame;

    frame.kind = frame_call;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.memory = i_memory;
    frame.call.function = i_function;
    frame.call.r0 = i_r0;
#  যদি d_m3HasFloat
    frame.call.fp0 = i_fp0;
#  endif

    ঘূৰাই Continuation_RecordFrame(i_runtime, &frame);
}

#  define d_m3RecordCallFrame(MEMORY, FUNCTION)                                \
       যদি (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           ঘূৰাই RecordCallFrame (m3MemRuntime (_mem), _pc, _sp, (MEMORY),    \
                                   (FUNCTION), d_m3ExpRegArgs (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
স্থির M3_NOINLINE
m3ret_t RecordLoopFrame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3Memory i_memory)
{
    M3Frame frame;

    frame.kind = frame_loop;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.memory = i_memory;

    ঘূৰাই Continuation_RecordFrame(i_runtime, &frame);
}


#  যদি d_m3HasExceptionHandling
স্থির M3_NOINLINE
m3ret_t RecordTryFrame (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3Memory i_memory,
                        u32 i_অঙ্ক_Clauses, bool i_handlersLive)
{
    M3Frame frame;

    frame.kind = frame_try;
    frame.pc = i_clauses;
    frame.sp = i_sp;
    frame.memory = i_memory;
    frame.try_.অঙ্ক_Clauses = i_অঙ্ক_Clauses;
    frame.try_.handlersLive = i_handlersLive;

    ঘূৰাই Continuation_RecordFrame(i_runtime, &frame);
}
#  endif


#  যদি d_m3EntryKeepsFrame
স্থির M3_NOINLINE
m3ret_t RecordEntryFrame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3Memory i_memory,
                          IM3Function i_function)
{
    M3Frame frame;

    frame.kind = frame_entry;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.memory = i_memory;
    frame.entry.function = i_function;

    ঘূৰাই Continuation_RecordFrame(i_runtime, &frame);
}
#  endif
#অন্যথা
#  define d_m3RecordCallFrame(MEMORY, FUNCTION)
#endif


#যদি d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#নোচেদ্যদি d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ধ্রুৱ IM3Runtime trace_rt = m3MemRuntime(_mem); (শূন্য) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->callDepth)*2, "", ##__VA_ARGS__)
#অন্যথা
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#endif

#যদি d_m3EnableStrace >= 3
#  define d_m3TraceLoad(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#অন্যথা
#  define d_m3TraceLoad(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#endif

#ifdef DEBUG
  #define d_outOfBounds newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->length, operand))

#  define d_outOfBoundsMemOp(OFFSET, SIZE) newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->length, OFFSET, SIZE))
#অন্যথা
  #define d_outOfBounds newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#  define d_outOfBoundsMemOp(OFFSET, SIZE) newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#endif

#যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#অন্যথা
d_m3RetSig Call (d_m3OpSig)
#endif
{
    m3ret_t possible_trap = m3_Yield();
    যদি (M3_UNLIKELY(possible_trap)) {
        ঘূৰাই possible_trap;
    }

    nextOpDirect();
}

#define d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    d_m3PreloadNext ();                                 \
    OP((RES), operand, ((TYPE) REG), ##__VA_ARGS__);    \
    nextOpPreloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = slot (TYPE);                        \
    TYPE operand1 = slot (TYPE);                        \
    d_m3PreloadNext ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    nextOpPreloaded ();                                 \
}

#define d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    d_m3PreloadNext ();                                 \
    OP((RES), ((TYPE) REG), operand, ##__VA_ARGS__);    \
    nextOpPreloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#define d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#define d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#define M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#define M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#define M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#define M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#define d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#define d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#define d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#define d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#define d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#define d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#define d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#define d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#define d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#define d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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

#যদি d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#endif

#define OP_ADD_32(A,B) (i32)((u32)(A) + (u32)(B))
#define OP_ADD_64(A,B) (i64)((u64)(A) + (u64)(B))
#define OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#define OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#define OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#define OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, Add,      OP_ADD_32)  d_m3CommutativeOpFunc_i (i64, Add,      OP_ADD_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, Subtract,            OP_SUB_32)  d_m3OpFunc_i (i64, Subtract,            OP_SUB_64)

#define OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#define OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#define OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#define OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, Shift_বাম,       OP_SHL_32)      d_m3OpFunc_i (u64, Shift_বাম,       OP_SHL_64)
d_m3OpFunc_i (i32, Shift_ডান,      OP_SHR_32)      d_m3OpFunc_i (i64, Shift_ডান,      OP_SHR_64)
d_m3OpFunc_i (u32, Shift_ডান,      OP_SHR_32)      d_m3OpFunc_i (u64, Shift_ডান,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#যদি d_m3FoldSetLocal

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
    TYPE operand = slot (TYPE);                         \
    TYPE * dest = slot_ptr (TYPE);                      \
    d_m3PreloadNext ();                                 \
    TYPE result;                                        \
    OP((result), operand, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = result;  REG = result;                     \
    nextOpPreloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = slot (TYPE);                        \
    TYPE operand1 = slot (TYPE);                        \
    TYPE * dest = slot_ptr (TYPE);                      \
    d_m3PreloadNext ();                                 \
    TYPE result;                                        \
    OP((result), operand1, operand2, ##__VA_ARGS__);    \
    * dest = result;  REG = result;                     \
    nextOpPreloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    TYPE * dest = slot_ptr (TYPE);                      \
    d_m3PreloadNext ();                                 \
    TYPE result;                                        \
    OP((result), ((TYPE) REG), operand, ##__VA_ARGS__); \
    * dest = result;  REG = result;                     \
    nextOpPreloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#define d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#define d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#define d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#define d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, Add,          M3_FUNC, OP_ADD_32)
d_m3FoldOp_i           (i32, Subtract,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, Shift_বাম,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, Shift_ডান,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, Shift_ডান,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, Add,          M3_FUNC, OP_ADD_64)
d_m3FoldOp_i           (i64, Subtract,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, Shift_বাম,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, Shift_ডান,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, Shift_ডান,   M3_FUNC, OP_SHR_64)

#  যদি d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, Add,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, Subtract,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, Add,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, Subtract,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#endif

#endif // d_m3FoldSetLocal

#যদি d_m3HasFloat
d_m3CommutativeArithOp_f(f32, Add,         +)      d_m3CommutativeArithOp_f(f64, Add,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, Subtract,               -)      d_m3ArithOp_f(f64, Subtract,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#endif

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

#যদি d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#endif

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#define d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE operand = slot (TYPE);                     \
    OP((RES), operand, ##__VA_ARGS__);              \
    nextOp ();                                      \
}

#define M3_UNARY(RES, X, OP) (RES) = OP(X)
#define M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#define d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#define d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#define d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#যদি d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       sqrt);
d_m3UnaryArithOp_f(f32, Nearest,    rintf);    d_m3UnaryArithOp_f(f64, Nearest,    rint);
#  যদি defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  অন্যথা
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#endif

#define OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, EqualToZero, OP_EQZ)
d_m3UnaryOp_i(i64, EqualToZero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#যদি (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#define OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#define OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#define OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#define OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#নোচেদ্যদি defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#define OP_CLZ_32(x) __builtin_clz(x)
#define OP_CTZ_32(x) __builtin_ctz(x)
#define OP_CLZ_64(x) __builtin_clzll(x)
#define OP_CTZ_64(x) __builtin_ctzll(x)
#অন্যথা
#define OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#define OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#define OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#define OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#endif

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#define OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    nextOp();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 operand = slot(i64);
    _r0 = OP_WRAP_I64(operand);
    nextOp();
}

// Integer sign extension operations
#define OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#define OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#define OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#define OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#define OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#যদি d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_উপৰেৰ = slot (u64);
    u64  rhs_নিচেৰ  = slot (u64);
    u64  lhs_উপৰেৰ = slot (u64);
    u64  lhs_নিচেৰ  = slot (u64);
    u64* নিচেৰ     = slot_ptr (u64);
    d_m3PreloadNext();

    u64 sum = lhs_নিচেৰ + rhs_নিচেৰ;

    *নিচেৰ = sum;
    _r0  = (i64) (lhs_উপৰেৰ + rhs_উপৰেৰ + (sum < lhs_নিচেৰ));

    nextOpPreloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_উপৰেৰ = slot (u64);
    u64  rhs_নিচেৰ  = slot (u64);
    u64  lhs_উপৰেৰ = slot (u64);
    u64  lhs_নিচেৰ  = slot (u64);
    u64* নিচেৰ     = slot_ptr (u64);
    d_m3PreloadNext();

    *নিচেৰ = lhs_নিচেৰ - rhs_নিচেৰ;
    _r0  = (i64) (lhs_উপৰেৰ - rhs_উপৰেৰ - (lhs_নিচেৰ < rhs_নিচেৰ));

    nextOpPreloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = slot (i64);
    i64  a   = slot (i64);
    u64* নিচেৰ = slot_ptr (u64);
    d_m3PreloadNext();

    u64 উপৰেৰ;
    *নিচেৰ = mul_wide_i64 (a, b, &উপৰেৰ);
    _r0  = (i64) উপৰেৰ;

    nextOpPreloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = slot (u64);
    u64  a   = slot (u64);
    u64* নিচেৰ = slot_ptr (u64);
    d_m3PreloadNext();

    u64 উপৰেৰ;
    *নিচেৰ = mul_wide_u64 (a, b, &উপৰেৰ);
    _r0  = (i64) উপৰেৰ;

    nextOpPreloaded();
}

#endif // d_m3HasWideArithmetic

#define d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * স্তূপ = slot_ptr (FROM);                 \
    OP((DEST), (* স্তূপ), ##__VA_ARGS__);           \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    nextOp ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * স্তূপ = slot_ptr (FROM);                 \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (* স্তূপ), ##__VA_ARGS__);         \
    nextOp ();                                      \
}

#যদি d_m3HasFloat
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
#endif

#define d_m3TypeModifyOp(REG_TO, REG_FROM, TO, NAME, FROM)  \
d_m3Op(TO##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
    nextOp ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#define d_m3FloatModifyOp(TO, NAME, FROM)                   \
d_m3Op(TO##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((TO) ((FROM) _fp0));             \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    _fp0 = canon_nan_##TO ((TO) (from));                    \
    nextOp ();                                              \
}

#যদি d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#endif

#define d_m3TypeConvertOp(REG_TO, REG_FROM, TO, NAME, FROM) \
d_m3Op(TO##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    slot (TO) = (TO) ((FROM) REG_FROM);                     \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    slot (TO) = (TO) (from);                                \
    nextOp ();                                              \
}

// Int to float
#যদি d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#endif

#define d_m3ReinterpretOp(REG, TO, SRC, FROM)               \
d_m3Op(TO##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    সঙ্ঘ { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    সঙ্ঘ { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    REG = u.t;                                              \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    সঙ্ঘ { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    slot (TO) = u.t;                                        \
    nextOp ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    সঙ্ঘ { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    slot (TO) = u.t;                                        \
    nextOp ();                                              \
}

#যদি d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#endif

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = immediate(u32*);
    slot(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    nextOp();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = immediate(u64*);
    slot(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    nextOp();
}


d_m3Op(SetGlobal_i32)
{
    u32* global = immediate(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    nextOp();
}


d_m3Op(SetGlobal_i64)
{
    u64* global = immediate(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    nextOp();
}


#যদি d_m3_Has_স্তূপ_Switching

d_m3Op(Branch);
d_m3Op(Entry);
#  যদি !d_m3EntryKeepsFrame
d_m3Op(Entry_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
স্থির M3_NOINLINE
IM3Function FunctionOfCompiledCode (pc_t i_pc)
{
    যদি (not i_pc) {
        ঘূৰাই রিক্ত;
    }

    যেতিয়ালৈ ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation entry = (IM3Operation)i_pc[0];

#  যদি !d_m3EntryKeepsFrame
    যদি (entry == op_Entry_Suspendable) {
        ঘূৰাই (IM3Function)i_pc[1];
    }
#  endif
    ঘূৰাই (entry == op_Entry) ? (IM3Function)i_pc[1] : রিক্ত;
}

#endif

d_m3Op(Call)
{
    d_m3_Check_Native_স্তূপ();

    pc_t      callPC = immediate(pc_t);
    i32       স্তূপ_Offset = immediate(i32);
    IM3Memory memory = m3MemInfo(_mem);

    m3stack_t sp = _sp + স্তূপ_Offset;

#যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#অন্যথা
    m3ret_t r = Call(callPC, sp, _mem, d_m3OpDefaultArgs);
#endif

    _mem = memory->mallocated;

    যদি (M3_LIKELY(not r)) {
        nextOp();
    } অন্যথা {
        d_m3RecordCallFrame(memory, FunctionOfCompiledCode(callPC));
        ঠেলো_Backtrace_Frame_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = slot(IM3Function);
    IM3FuncType type = immediate(IM3FuncType);
    i32         স্তূপ_Offset = immediate(i32);
    IM3Memory   memory = m3MemInfo(_mem);

    m3stack_t   sp = _sp + স্তূপ_Offset;

    m3ret_t     r = m3Err_none;

    যদি (M3_LIKELY(function)) {
        যদি (M3_LIKELY(type == function->funcType)) {
            যদি (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            যদি (M3_LIKELY(not r)) {
#যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#অন্যথা
                r = Call(function->compiled, sp, _mem, d_m3OpDefaultArgs);
#endif

                _mem = memory->mallocated;

                যদি (M3_LIKELY(not r)) {
                    nextOpDirect();
                } অন্যথা {
                    d_m3RecordCallFrame(memory, function);
                    ঠেলো_Backtrace_Frame_2();
                    forwardTrap(r);
                }
            }
        } অন্যথা {
            r = m3Err_trapIndirectCallTypeMismatch;
        }
    } অন্যথা {
        r = m3Err_trapNullFunctionRef;
    }

    যদি (M3_UNLIKELY(r)) {
        newTrap(r);
    } অন্যথা {
        forwardTrap(r);
    }
}


// ref.as_non_null is a null check and nothing else: the value stays put, so
// there is no destination slot, only the trap.
d_m3Op(RefAsNonNull)
{
    IM3Function reference = slot(IM3Function);

    যদি (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    nextOp();
}


d_m3Op(CallIndirect)
{
    d_m3_Check_Native_স্তূপ();

    M3Table*    table = immediate(M3Table*);
    u64         tableIndex = d_m3WideOperand(table->isTable64);
    IM3FuncType type = immediate(IM3FuncType);
    i32         স্তূপ_Offset = immediate(i32);
    IM3Memory   memory = m3MemInfo(_mem);

    m3stack_t   sp = _sp + স্তূপ_Offset;

    m3ret_t     r = m3Err_none;

    যদি (M3_LIKELY(tableIndex < table->size)) {
        IM3Function function = (IM3Function)table->elements[tableIndex];

        যদি (M3_LIKELY(function)) {
            যদি (M3_LIKELY(type == function->funcType)) {
                যদি (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                যদি (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3MemoryHeader* calleeMem = Module_MemoryHeader(function->module);

#যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3OpDefaultArgs, d_m3BaseCstr);
#অন্যথা
                    r = Call(function->compiled, sp, calleeMem, d_m3OpDefaultArgs);
#endif

                    _mem = memory->mallocated;

                    যদি (M3_LIKELY(not r)) {
                        nextOpDirect();
                    } অন্যথা {
                        d_m3RecordCallFrame(memory, function);
                        ঠেলো_Backtrace_Frame_2();
                        forwardTrap(r);
                    }
                }
            } অন্যথা {
                r = m3Err_trapIndirectCallTypeMismatch;
            }
        } অন্যথা {
            r = m3Err_trapTableElementIsNull;
        }
    } অন্যথা {
        r = m3Err_trapTableIndexOutOfRange;
    }

    যদি (M3_UNLIKELY(r)) {
        newTrap(r);
    } অন্যথা {
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
#define d_m3TailCallArgs(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((শূন্য *) (_sp + (RETURNSLOTS)), (ধ্রুৱ শূন্য *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * পরিমাণ (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = immediate(pc_t);
    i32     স্তূপ_Offset = immediate(i32);
    i32     returnSlots = immediate(i32);
    u32     অঙ্ক_Arg_Slots = immediate(u32);

    m3ret_t possible_trap = m3_Yield();
    যদি (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3TailCallArgs(returnSlots, স্তূপ_Offset, অঙ্ক_Arg_Slots);

    jumpOpDirect(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = slot(IM3Function);
    IM3FuncType type = immediate(IM3FuncType);
    i32         স্তূপ_Offset = immediate(i32);
    i32         returnSlots = immediate(i32);
    u32         অঙ্ক_Arg_Slots = immediate(u32);

    m3ret_t     r = m3Err_none;

    যদি (M3_LIKELY(function)) {
        যদি (M3_LIKELY(type == function->funcType)) {
            যদি (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            যদি (M3_LIKELY(not r)) {
                r = m3_Yield();

                যদি (M3_LIKELY(not r)) {
                    d_m3TailCallArgs(returnSlots, স্তূপ_Offset, অঙ্ক_Arg_Slots);

                    jumpOpDirect(function->compiled);
                }
            }
        } অন্যথা {
            r = m3Err_trapIndirectCallTypeMismatch;
        }
    } অন্যথা {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(ReturnCallIndirect)
{
    M3Table*    table = immediate(M3Table*);
    u64         tableIndex = d_m3WideOperand(table->isTable64);
    IM3FuncType type = immediate(IM3FuncType);
    i32         স্তূপ_Offset = immediate(i32);
    i32         returnSlots = immediate(i32);
    u32         অঙ্ক_Arg_Slots = immediate(u32);

    m3ret_t     r = m3Err_none;

    যদি (M3_LIKELY(tableIndex < table->size)) {
        IM3Function function = (IM3Function)table->elements[tableIndex];

        যদি (M3_LIKELY(function)) {
            যদি (M3_LIKELY(type == function->funcType)) {
                যদি (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                যদি (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    যদি (M3_LIKELY(not r)) {
                        d_m3TailCallArgs(returnSlots, স্তূপ_Offset, অঙ্ক_Arg_Slots);

                        jumpOpDirect(function->compiled);
                    }
                }
            } অন্যথা {
                r = m3Err_trapIndirectCallTypeMismatch;
            }
        } অন্যথা {
            r = m3Err_trapTableElementIsNull;
        }
    } অন্যথা {
        r = m3Err_trapTableIndexOutOfRange;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = immediate(IM3Function);
    ctx.userdata = immediate(শূন্য*);
    u64* ধ্রুৱ sp = ((u64*)_sp);
    IM3Memory  memory = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#যদি d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    ৱর্ণ        outbuff[1024];
    ৱর্ণ*       outp = outbuff;
    ৱর্ণ*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.fieldUtf8);

    ধ্রুৱ পূর্ণাঙ্ক nArgs = ftype->অঙ্ক_Args;
    ধ্রুৱ পূর্ণাঙ্ক nRets = ftype->অঙ্ক_Rets;
    u64*      args = sp + nRets;
    বাবে (পূর্ণাঙ্ক i = 0; i < nArgs; i++) {
        ধ্রুৱ পূর্ণাঙ্ক type = ftype->types[nRets + i];
        ৱিকল্প (type) {
        পক্ষ c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(args + i)); ভঙ্গ;
        পক্ষ c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(args + i)); ভঙ্গ;
        পক্ষ c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(args + i)); ভঙ্গ;
        পক্ষ c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(args + i)); ভঙ্গ;
        স্ৱভাৱ: outp += snprintf(outp, oute - outp, "<type %d>", type); ভঙ্গ;
        }
        যদি (i < nArgs - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  যদি d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#endif

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    শূন্য* স্তূপ_backup = runtime->স্তূপ;
    runtime->স্তূপ = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3MemData(ctx.function->hostMemory->mallocated));
    runtime->স্তূপ = স্তূপ_backup;

#যদি d_m3EnableStrace
    যদি (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (ৱর্ণ*)possible_trap);
    } অন্যথা যদি (ftype and ftype->অঙ্ক_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } অন্যথা {
        d_m3TracePrint("%s", outbuff);
    }
#endif

    যদি (M3_UNLIKELY(possible_trap)) {
        _mem = memory->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        যদি (possible_trap != m3Err_pendingException) {
            ঠেলো_Backtrace_Frame_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(MemSize)
{
    IM3Memory memory = m3MemInfo(_mem);

    _r0 = memory->অঙ্ক_Pages;

    nextOp();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3Memory  memory = m3MemInfo(_mem);

    i32        অঙ্ক_Pages_To_Grow = (i32)_r0;
    যদি (অঙ্ক_Pages_To_Grow >= 0) {
        _r0 = (m3reg_t)memory->অঙ্ক_Pages;

        যদি (M3_LIKELY(অঙ্ক_Pages_To_Grow)) {
            u64      requiredPages = memory->অঙ্ক_Pages + (u32)অঙ্ক_Pages_To_Grow;

            M3Result r = ResizeMemory(runtime, memory, requiredPages);
            যদি (r) {
                _r0 = -1;
            }

            _mem = memory->mallocated;
        }
    } অন্যথা {
        _r0 = -1;
    }

    nextOp();
}


#যদি d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3Memory  memory = m3MemInfo(_mem);

    u64        অঙ্ক_Pages_To_Grow = (u64)_r0;
    u64        অঙ্ক_Pages = memory->অঙ্ক_Pages;

    যদি (M3_LIKELY(অঙ্ক_Pages_To_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        যদি (অঙ্ক_Pages_To_Grow <= memory->maxPages - অঙ্ক_Pages and
            not ResizeMemory(runtime, memory, অঙ্ক_Pages + অঙ্ক_Pages_To_Grow)) {
            _r0 = (m3reg_t)অঙ্ক_Pages;
        } অন্যথা {
            _r0 = -1;
        }

        _mem = memory->mallocated;
    } অন্যথা {
        _r0 = (m3reg_t)অঙ্ক_Pages;
    }

    nextOp();
}

#endif // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 size = (u32)_r0;
    u64 source = slot(u32);
    u64 destination = slot(u32);

    যদি (M3_LIKELY(d_m3MemRangeOk(destination, size, _mem))) {
        যদি (M3_LIKELY(d_m3MemRangeOk(source, size, _mem))) {
            u8* dst = m3MemData(_mem) + destination;
            u8* src = m3MemData(_mem) + source;
            memmove(dst, src, (size_t)size);

            nextOp();
        } অন্যথা {
            d_outOfBoundsMemOp(source, size);
        }
    } অন্যথা {
        d_outOfBoundsMemOp(destination, size);
    }
}


#যদি d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 size = (u64)_r0;
    u64 source = slot(u64);
    u64 destination = slot(u64);

    যদি (M3_LIKELY(d_m3MemRangeOk(destination, size, _mem))) {
        যদি (M3_LIKELY(d_m3MemRangeOk(source, size, _mem))) {
            u8* dst = m3MemData(_mem) + destination;
            u8* src = m3MemData(_mem) + source;
            memmove(dst, src, (size_t)size);

            nextOp();
        } অন্যথা {
            d_outOfBoundsMemOp(source, size);
        }
    } অন্যথা {
        d_outOfBoundsMemOp(destination, size);
    }
}

#endif // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(SetMemory)
{
    IM3Memory memory = immediate(IM3Memory);

    _mem = memory->mallocated;

    nextOp();
}


#যদি d_m3HasMultiMemory

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3Memory       destMemory = immediate(IM3Memory);
    IM3Memory       sourceMemory = immediate(IM3Memory);
    u32             widths = immediate(u32);

    M3MemoryHeader* destMem = destMemory->mallocated;
    M3MemoryHeader* sourceMem = sourceMemory->mallocated;

    u64             size = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             source = d_m3WideOperand(widths & 0x2);
    u64             destination = d_m3WideOperand(widths & 0x1);

    যদি (M3_LIKELY(d_m3MemRangeOk(destination, size, destMem))) {
        যদি (M3_LIKELY(d_m3MemRangeOk(source, size, sourceMem))) {
            memmove(m3MemData(destMem) + destination, m3MemData(sourceMem) + source, (size_t)size);

            nextOp();
        } অন্যথা {
            d_outOfBoundsMemOp(source, size);
        }
    } অন্যথা {
        d_outOfBoundsMemOp(destination, size);
    }
}

#endif // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 size = (u32)_r0;
    u32 byte = slot(u32);
    u64 destination = slot(u32);

    যদি (M3_LIKELY(d_m3MemRangeOk(destination, size, _mem))) {
        u8* mem8 = m3MemData(_mem) + destination;
        memset(mem8, (u8)byte, size);
        nextOp();
    } অন্যথা {
        d_outOfBoundsMemOp(destination, size);
    }
}


#যদি d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 size = (u64)_r0;
    u32 byte = slot(u32);
    u64 destination = slot(u64);

    যদি (M3_LIKELY(d_m3MemRangeOk(destination, size, _mem))) {
        u8* mem8 = m3MemData(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)size);
        nextOp();
    } অন্যথা {
        d_outOfBoundsMemOp(destination, size);
    }
}

#endif // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3DataSegment* segment = immediate(M3DataSegment*);

    u64            size = (u32)_r0;
    u64            source = slot(u32);
    u64            destination = slot(u32);

    u64            available = segment->dropped ? 0 : segment->size;

    যদি (M3_LIKELY(d_m3MemRangeOk(destination, size, _mem))) {
        যদি (M3_LIKELY(source <= available and size <= available - source)) {
            memcpy(m3MemData(_mem) + destination, segment->data + source, (size_t)size);
            nextOp();
        } অন্যথা {
            d_outOfBoundsMemOp(source, size);
        }
    } অন্যথা {
        d_outOfBoundsMemOp(destination, size);
    }
}


#যদি d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3DataSegment* segment = immediate(M3DataSegment*);

    u64            size = (u32)_r0;
    u64            source = slot(u32);
    u64            destination = slot(u64);

    u64            available = segment->dropped ? 0 : segment->size;

    যদি (M3_LIKELY(d_m3MemRangeOk(destination, size, _mem))) {
        যদি (M3_LIKELY(source <= available and size <= available - source)) {
            memcpy(m3MemData(_mem) + destination, segment->data + source, (size_t)size);
            nextOp();
        } অন্যথা {
            d_outOfBoundsMemOp(source, size);
        }
    } অন্যথা {
        d_outOfBoundsMemOp(destination, size);
    }
}

#endif // d_m3HasMemory64


d_m3Op(DataDrop)
{
    M3DataSegment* segment = immediate(M3DataSegment*);
    segment->dropped = true;

    nextOp();
}


#যদি d_m3HasRefTypes

d_m3Op(TableGet)
{
    M3Table* table = immediate(M3Table*);
    u64      index = d_m3WideOperand(table->isTable64);

    যদি (M3_LIKELY(index < table->size)) {
        _r0 = (u64)(uintptr_t)table->elements[index];
        nextOp();
    } অন্যথা {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(TableSet)
{
    M3Table* table = immediate(M3Table*);
    শূন্য*    মান = slot(শূন্য*);
    u64      index = d_m3WideOperand(table->isTable64);

    যদি (M3_LIKELY(index < table->size)) {
        table->elements[index] = মান;
        nextOp();
    } অন্যথা {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(TableSize)
{
    M3Table* table = immediate(M3Table*);

    _r0 = table->size;

    nextOp();
}


d_m3Op(TableGrow)
{
    M3Table* table = immediate(M3Table*);
    u64      delta = d_m3WideOperand(table->isTable64);
    শূন্য*    মান = slot(শূন্য*);

    u32      oldSize = table->size;
    u32      maxSize = table->maxSize ? table->maxSize : d_m3MaxSaneTableSize;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      newSize = (delta <= maxSize - oldSize) ? (u64)oldSize + delta
                                                    : (u64)maxSize + 1;

    যদি (newSize == oldSize)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = oldSize;
        nextOp();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    যদি (newSize <= maxSize and (not runtime->tableElementsLimit or
                                delta <= runtime->tableElementsLimit - runtime->tableElementsUsed)) {
        শূন্য** elements = m3_ReallocArray(শূন্য*, table->elements, (size_t)newSize, oldSize);

        যদি (elements) {
            runtime->tableElementsUsed += newSize - oldSize;
            table->elements = elements;
            table->size = (u32)newSize;

            বাবে (u32 i = oldSize; i < table->size; ++i) {
                table->elements[i] = মান;
            }

            _r0 = oldSize;
            nextOp();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    nextOp();
}


d_m3Op(TableInit)
{
    M3Table*          table = immediate(M3Table*);
    M3ElementSegment* segment = immediate(M3ElementSegment*);
    u32               গুনতি = slot(u32);
    u32               source = slot(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3WideOperand(table->isTable64);

    u64               available = segment->dropped ? 0 : segment->অঙ্ক_Elements;

    যদি (M3_LIKELY(destination <= table->size and গুনতি <= table->size - destination and
                  (u64) source + গুনতি <= available)) {
        বাবে (u32 i = 0; i < গুনতি; ++i) {
            table->elements[destination + i] = segment->resolved[source + i];
        }

        nextOp();
    } অন্যথা {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = immediate(M3ElementSegment*);
    segment->dropped = true;

    nextOp();
}


d_m3Op(TableCopy)
{
    M3Table* dst = immediate(M3Table*);
    M3Table* src = immediate(M3Table*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      গুনতি = d_m3WideOperand(dst->isTable64 and src->isTable64);
    u64      source = d_m3WideOperand(src->isTable64);
    u64      destination = d_m3WideOperand(dst->isTable64);

    যদি (M3_LIKELY(destination <= dst->size and গুনতি <= dst->size - destination and
                  source <= src->size and গুনতি <= src->size - source)) {
        memmove(dst->elements + destination, src->elements + source, (size_t)গুনতি * পরিমাণ(শূন্য*));

        nextOp();
    } অন্যথা {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(TableFill)
{
    M3Table* table = immediate(M3Table*);
    u64      গুনতি = d_m3WideOperand(table->isTable64);
    শূন্য*    মান = slot(শূন্য*);
    u64      index = d_m3WideOperand(table->isTable64);

    যদি (M3_LIKELY(index <= table->size and গুনতি <= table->size - index)) {
        বাবে (u64 i = 0; i < গুনতি; ++i) {
            table->elements[index + i] = মান;
        }

        nextOp();
    } অন্যথা {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}

#endif // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = immediate(IM3Function);

    m3ret_t     result = m3Err_none;

    যদি (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        result = CompileFunction(function);
    }

    যদি (not result) {
        // patch up compiled pc and call rewritten op_Call
        *((শূন্য**)--_pc) = (শূন্য*)(function->compiled);
        --_pc;
        nextOpDirect();
    }

    newTrap(result);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = immediate(IM3Function);

    m3ret_t     result = m3Err_none;

    যদি (M3_UNLIKELY(not function->compiled)) {
        result = CompileFunction(function);
    }

    যদি (not result) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((শূন্য**)--_pc) = (শূন্য*)(function->compiled);
        --_pc;
        nextOpDirect();
    }

    newTrap(result);
}


#যদি d_m3_Has_স্তূপ_Switching
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
স্থির
শূন্য SuspendWithoutTag (IM3Runtime io_runtime, IM3Continuation io_cont, M3SafePointKind i_kind,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  যদি d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspendTag = রিক্ত;
    io_runtime->suspendHandlerCont = রিক্ত;
    io_runtime->suspendStubPC = রিক্ত;
    io_runtime->switchTarget = রিক্ত;
    io_runtime->অঙ্ক_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspendPoint = (u8)i_kind;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  যদি d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->অঙ্ক_Suspend_Results = 0;
}
#endif // d_m3HasStackSwitching


#যদি d_m3HasGasMetering

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
    u32        cost = immediate(u32);

    runtime->gasRemaining -= (i64)cost;

    যদি (M3_UNLIKELY(runtime->gasRemaining < 0)) {
#  যদি d_m3_Has_স্তূপ_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        যদি (runtime->isSuspendable and runtime->activeContinuation) {
            runtime->suspendRequested = true;
            nextOp();
        }
#  endif
        newTrap(m3Err_trapOutOfGas);
    }

    nextOp();
}

#endif // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
স্থির অন্তঃস্থ
bool EntryFits (IM3Function i_function, m3stack_t _sp, M3MemoryHeader* _mem)
{
#যদি d_m3_Skip_স্তূপ_Check
    (শূন্য)i_function;
    (শূন্য)_sp;
    (শূন্য)_mem;
    ঘূৰাই true;
#নোচেদ্যদি d_m3_Has_স্তূপ_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->activeContinuation : রিক্ত);
    শূন্য*           max_স্তূপ = (_act && _act->val_স্তূপ)
                                 ? (শূন্য*)(_act->val_স্তূপ + _act->অঙ্ক_স্তূপ_Slots)
                                 : (_mem ? _mem->max_স্তূপ : (শূন্য*)~(uintptr_t)0);
    ঘূৰাই M3_LIKELY((শূন্য*)(_sp + i_function->max_স্তূপ_Slots) < max_স্তূপ);
#অন্যথা
    ঘূৰাই M3_LIKELY((শূন্য*)(_sp + i_function->max_স্তূপ_Slots) < _mem->max_স্তূপ);
#endif
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
স্থির অন্তঃস্থ
শূন্য EnterFrame (IM3Function i_function, m3stack_t _sp)
{
#যদি defined(DEBUG)
    i_function->hits++;
#endif
    u8* স্তূপ = (u8*)((m3slot_t*)_sp + i_function->অঙ্ক_Ret_And_Arg_Slots);

    memset(স্তূপ, 0x0, i_function->অঙ্ক_Local_Bytes);
    স্তূপ += i_function->অঙ্ক_Local_Bytes;

    যদি (i_function->constants) {
        memcpy(স্তূপ, i_function->constants, i_function->অঙ্ক_Constant_Bytes);
    }
}


d_m3Op(Entry)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = immediate(IM3Function);
#যদি d_m3EntryKeepsFrame
    IM3Memory memory = m3MemInfo(_mem);
#endif

    যদি (EntryFits(function, _sp, _mem)) {
        EnterFrame(function, _sp);

#যদি !d_m3EntryKeepsFrame
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        nextOpDirect();
#অন্যথা

#  যদি d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->অঙ্ক_Ret_Slots));
        trace_rt->callDepth++;
#  endif

        m3ret_t r = nextOpImpl();

#  যদি d_m3EnableStrace >= 2
        trace_rt->callDepth--;

        যদি (r) {
            d_m3TracePrint("} !trap = %s", (ৱর্ণ*)r);
        } অন্যথা যদি (Get_Function_অঙ্ক_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } অন্যথা {
            d_m3TracePrint("}");
        }
#  endif

        যদি (M3_UNLIKELY(r)) {
            _mem = memory->mallocated;
#  যদি d_m3_Has_স্তূপ_Switching
            যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                ঘূৰাই RecordEntryFrame(m3MemRuntime(_mem), _pc, _sp, memory, function);
            }
#  endif
            fillBacktraceFrame();
        }
        forwardTrap(r);
#endif // d_m3EntryKeepsFrame
    } অন্যথা {
        newTrap(m3_Err_trap_স্তূপ_Overflow);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_স্তূপ();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3Memory memory = m3MemInfo(_mem);

    কৰা {
#যদি d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->callDepth++;
#endif
        r = nextOpImpl();

#যদি d_m3EnableStrace >= 3
        trace_rt->callDepth--;
        d_m3TracePrint("}");
#endif
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = memory->mallocated;
    } যেতিয়ালৈ (r == _pc);

#যদি d_m3_Has_স্তূপ_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        ঘূৰাই RecordLoopFrame(m3MemRuntime(_mem), _pc, _sp, memory);
    }
#endif

    forwardTrap(r);
}


#যদি d_m3HasExceptionHandling

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
d_m3Op(TryTable)
{
    d_m3_Check_Native_স্তূপ();

    IM3Memory  memory = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        অঙ্ক_Clauses = immediate(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * অঙ্ক_Clauses;

    u32        depth = ++runtime->tryDepth;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = memory->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->tryDepth == depth);
    runtime->tryDepth = depth - 1;

#  যদি d_m3_Has_স্তূপ_Switching
    যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        ঘূৰাই RecordTryFrame(runtime, clauses, _sp, memory, অঙ্ক_Clauses, isInnermost);
    }
#  endif

    যদি (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ধ্রুৱ M3Exception* exception = runtime->pendingException;

        বাবে (u32 i = 0; i < অঙ্ক_Clauses; ++i) {
            IM3Tag tag = (IM3Tag)clauses[2 * i];

            যদি (tag == রিক্ত or tag == exception->tag) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jumpOpDirect(handler);
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

    u32          অঙ্ক_Args = immediate(u32);

    বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
        i32 offset = immediate(i32);
        u32 is64 = immediate(u32);
        u64 মান = exception->args[i];

        যদি (is64) {
            *(u64*)(_sp + offset) = মান;
        } অন্যথা {
            *(u32*)(_sp + offset) = (u32)মান;
        }
    }

    i32 refOffset = immediate(i32);

    যদি (refOffset >= 0) {
        *(M3Exception**)(_sp + refOffset) = exception;
        exception->reified = true;
    } অন্যথা যদি (not exception->reified) {
        FreeException(runtime, exception);
    }

    nextOp();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3Tag       tag = immediate(IM3Tag);
    u32          অঙ্ক_Args = immediate(u32);

    M3Exception* exception = NewException(runtime, tag, অঙ্ক_Args);

    যদি (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
        i32 offset = immediate(i32);
        u32 is64 = immediate(u32);

        exception->args[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    ঘূৰাই m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = slot(M3Exception*);

    যদি (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    ঘূৰাই m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(বেৰ_কৰো_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        গুনতি = immediate(u32);

    runtime->tryDepth -= গুনতি;

    nextOp();
}

#endif // d_m3HasExceptionHandling

#যদি d_m3_Has_স্তূপ_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = immediate(IM3FuncType);
    i32         dstSlot = immediate(i32);
    i32         funcSlot = immediate(i32);

    IM3Function func = *(IM3Function*)(_sp + funcSlot);
    যদি (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    যদি (M3_UNLIKELY(not Continuation_Can_Acquire_স্তূপ(runtime))) {
        newTrap(m3Err_continuationLimitExceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    যদি (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3Result allocation = Continuation_Acquire_স্তূপ(runtime, cont);
    যদি (M3_UNLIKELY(allocation)) {
        cont->state = cont_consumed;
        newTrap(allocation);
    }
    *(IM3Continuation*)(_sp + dstSlot) = cont;
    nextOp();
}


স্থির
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = immediate(IM3FuncType);
    IM3FuncType     ct2 = immediate(IM3FuncType);
    i32             dstSlot = immediate(i32);
    i32             contSlot = immediate(i32);
    u32             অঙ্ক_Bound = immediate(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + contSlot);
    যদি (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    যদি (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->entryFunction);
    যদি (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->entryFunction = srcCont->entryFunction;
    dstCont->val_স্তূপ = srcCont->val_স্তূপ;
    dstCont->অঙ্ক_স্তূপ_Slots = srcCont->অঙ্ক_স্তূপ_Slots;
    dstCont->frames = srcCont->frames;
    dstCont->framesCap = srcCont->framesCap;
    dstCont->অঙ্ক_Frames = srcCont->অঙ্ক_Frames;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspendPoint = srcCont->suspendPoint;
    dstCont->r0 = srcCont->r0;
#  যদি d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_Args_গুনতি = srcCont->bound_Args_গুনতি;
    dstCont->state = srcCont->state;
#  যদি d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = রিক্ত;
#  endif

    dstCont->অঙ্ক_Suspend_Results = srcCont->অঙ্ক_Suspend_Results;
    বাবে (u32 j = 0; j < srcCont->অঙ্ক_Suspend_Results; ++j) {
        dstCont->suspendResultOffsets[j] = srcCont->suspendResultOffsets[j];
        dstCont->suspendResultIs64[j] = srcCont->suspendResultIs64[j];
    }

    যদি (dstCont->state == cont_allocated) {
        u16 অঙ্ক_Rets = ct1->contFuncType ? Get_Func_Type_অঙ্ক_Results(ct1->contFuncType) : 0;
        বাবে (u32 i = 0; i < অঙ্ক_Bound; ++i) {
            i32 argSlot = immediate(i32);
            u32 is64 = immediate(u32);
            u32 targetIdx = (অঙ্ক_Rets + dstCont->bound_Args_গুনতি) * c_io_Slot_গুনতি;
            যদি (is64) {
                *(u64*)(dstCont->val_স্তূপ + targetIdx) = *(u64*)(_sp + argSlot);
            } অন্যথা {
                *(u32*)(dstCont->val_স্তূপ + targetIdx) = *(u32*)(_sp + argSlot);
            }
            dstCont->bound_Args_গুনতি++;
        }
    } অন্যথা {
        IM3Continuation inner = InnermostSuspended(dstCont);
        বাবে (u32 i = 0; i < অঙ্ক_Bound; ++i) {
            i32 argSlot = immediate(i32);
            u32 is64 = immediate(u32);
            u32 dstIdx = dstCont->bound_Args_গুনতি;
            যদি (dstIdx < inner->অঙ্ক_Suspend_Results) {
                i32 dstOffset = inner->suspendResultOffsets[dstIdx];
                যদি (is64) {
                    *(u64*)(inner->sp + dstOffset) = *(u64*)(_sp + argSlot);
                } অন্যথা {
                    *(u32*)(inner->sp + dstOffset) = *(u32*)(_sp + argSlot);
                }
            }
            dstCont->bound_Args_গুনতি++;
        }
    }

    srcCont->val_স্তূপ = রিক্ত;
    srcCont->frames = রিক্ত;
    srcCont->framesCap = 0;
    srcCont->অঙ্ক_Frames = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dstSlot) = dstCont;
    nextOp();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
স্থির
IM3Continuation FindHandler (IM3Continuation i_cont, IM3Tag i_tag, u8 i_kind, pc_t* o_stubPC)
{
    IM3Tag queryTag = (i_tag and i_tag->resolved) ? i_tag->resolved : i_tag;

    বাবে (IM3Continuation cont = i_cont; cont; cont = cont->parent) {
        বাবে (u32 h = 0; h < cont->অঙ্ক_Handlers; ++h) {
            pc_t   entry = cont->handlersPC + (h * 3);
            IM3Tag handlerTag = (IM3Tag)(*(entry + 1));
            যদি (handlerTag and handlerTag->resolved) {
                handlerTag = handlerTag->resolved;
            }

            যদি ((u32)(uintptr_t)(*entry) == i_kind and handlerTag == queryTag) {
                যদি (o_stubPC) {
                    *o_stubPC = (pc_t)(*(entry + 2));
                }
                ঘূৰাই cont;
            }
        }
    }

    ঘূৰাই রিক্ত;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
স্থির
শূন্য ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    বাবে (u32 i = 0; i < io_cont->অঙ্ক_Suspend_Results; ++i) {
        যদি (io_cont->suspendResultIs64[i]) {
            *(u64*)(i_sp + io_cont->suspendResultOffsets[i]) = 0;
        } অন্যথা {
            *(m3slot_t*)(i_sp + io_cont->suspendResultOffsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->activeContinuation;
    যদি (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    IM3Tag tag = immediate(IM3Tag);
    u32    অঙ্ক_Params = immediate(u32);

    runtime->অঙ্ক_Suspend_Payload = অঙ্ক_Params;
    বাবে (u32 i = 0; i < অঙ্ক_Params; ++i) {
        i32 paramSlot = immediate(i32);
        u32 is64 = immediate(u32);
        যদি (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + paramSlot);
        } অন্যথা {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + paramSlot);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 অঙ্ক_Results = immediate(u32);
    cont->অঙ্ক_Suspend_Results = অঙ্ক_Results;
    বাবে (u32 i = 0; i < অঙ্ক_Results; ++i) {
        cont->suspendResultOffsets[i] = immediate(i32);
        cont->suspendResultIs64[i] = immediate(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = রিক্ত;
    IM3Continuation অনুসন্ধান_Cont = FindHandler(cont, tag, 0, &matchedStubPC);

    যদি (M3_UNLIKELY(not অনুসন্ধান_Cont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = অনুসন্ধান_Cont;
    runtime->switchTarget = রিক্ত;
    runtime->suspendTag = tag;

    cont->pc = _pc;
    cont->suspendPoint = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  যদি d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_Args_গুনতি = 0;

    ঘূৰাই m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
স্থির
m3ret_t RunUnderHandlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_অঙ্ক_Handlers, M3MemoryHeader** io_mem);
স্থির
শূন্য TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_অঙ্ক_Results);
স্থির
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3MemoryHeader* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
স্থির অন্তঃস্থ
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    ঘূৰাই pass and (i_cont->suspendPoint == safepoint_op or i_cont->suspendPoint == safepoint_entry);
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
স্থির
m3ret_t ReplayFrames (IM3Continuation i_cont, i32 i_depth, M3MemoryHeader* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  যদি d_m3_Max_Native_স্তূপ > 0
    {
        শূন্য* limit = runtime->স্তূপ_Limit;

        যদি (M3_UNLIKELY(limit and m3_Native_স্তূপ_Ptr() < limit)) {
            ঘূৰাই m3_Err_trap_স্তূপ_Overflow;
        }
    }
#  endif

    যদি (i_depth < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  যদি d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        যদি (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = রিক্ত;

            ঘূৰাই m3Err_pendingException;
        }
#  endif
        যদি (pastCheck) {
            ঘূৰাই ResumePastCheck(i_cont, _mem);
        }

        ঘূৰাই d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3Frame frame = i_cont->frames[i_depth];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3FrameKind)frame.kind == frame_resume);

#  যদি d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3FrameKind)frame.kind == frame_try);
#  endif

    যদি (not ownDescent) {
        r = ReplayFrames(i_cont, i_depth - 1, _mem);
        _mem = frame.memory->mallocated;
    }

    ৱিকল্প ((M3FrameKind)frame.kind) {
    পক্ষ frame_call:
        যদি (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            ঘূৰাই d_m3CallWithRegs(frame.pc, frame.sp, _mem, frame.call.r0, frame.call.fp0);
        }

        যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ঘূৰাই Continuation_RecordFrame(runtime, &frame);
        }
#  যদি d_m3RecordBacktraces
        ঠেলো_Backtrace_Frame(runtime, frame.pc - 1);
#  endif
        ঘূৰাই r;

    পক্ষ frame_loop: {
        m3stack_t _sp = frame.sp;
        m3reg_t   _r0 = 0;
#  যদি d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        যেতিয়ালৈ (r == (m3ret_t)frame.pc) {
            r = jumpOpImpl(frame.pc);
            _mem = frame.memory->mallocated;
        }

        যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ঘূৰাই Continuation_RecordFrame(runtime, &frame);
        }
        ঘূৰাই r;
    }

#  যদি d_m3HasExceptionHandling
    পক্ষ frame_try: {
        m3stack_t _sp = frame.sp;
        m3reg_t   _r0 = 0;
#    যদি d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (শূন্য)_sp;
        (শূন্য)_r0;
#    যদি d_m3HasFloat
        (শূন্য)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 depth = ++runtime->tryDepth;

        যদি (not frame.try_.handlersLive) {
            runtime->tryDepth = depth - 1;
        }

        r = ReplayFrames(i_cont, i_depth - 1, _mem);
        _mem = frame.memory->mallocated;

        bool isInnermost = (runtime->tryDepth == depth);
        runtime->tryDepth = depth - 1;

        যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            frame.try_.handlersLive = isInnermost;
            ঘূৰাই Continuation_RecordFrame(runtime, &frame);
        }

        যদি (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ধ্রুৱ M3Exception* exception = runtime->pendingException;

            বাবে (u32 i = 0; i < frame.try_.অঙ্ক_Clauses; ++i) {
                IM3Tag tag = (IM3Tag)frame.pc[2 * i];

                যদি (tag == রিক্ত or tag == exception->tag) {
                    pc_t handler = (pc_t)frame.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    ঘূৰাই jumpOpImpl(handler);
                }
            }
        }
        ঘূৰাই r;
    }
#  endif

    পক্ষ frame_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = frame.resume.cont;

        r = RunUnderHandlers(runtime, &inner, frame.resume.handlersPC,
                             frame.resume.অঙ্ক_Handlers, &_mem);

        যদি (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, frame.sp, frame.resume.resultsPC,
                                    frame.resume.অঙ্ক_Results);
            Continuation_Recycle_স্তূপ(runtime, inner);

            ঘূৰাই d_m3CallWithRegs(frame.pc, frame.sp, _mem, 0, 0.);
        }

        যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            যদি (runtime->suspendHandlerCont != inner) {
                frame.resume.cont = inner;
                ঘূৰাই Continuation_RecordFrame(runtime, &frame);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            যদি (M3_UNLIKELY(not suspended)) {
                ঘূৰাই m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            যদি (runtime->suspendStubPC) {
                ঘূৰাই d_m3CallWithRegs(runtime->suspendStubPC, frame.sp, _mem, 0, 0.);
            } অন্যথা {
                ঘূৰাই m3Err_trapUnhandledControlTag;
            }
        } অন্যথা {
            inner->state = cont_consumed;
            Continuation_Recycle_স্তূপ(runtime, inner);
        }

        ঘূৰাই r;
    }

#  যদি d_m3EntryKeepsFrame
    পক্ষ frame_entry:
        যদি (M3_UNLIKELY(r)) {
            যদি (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                ঘূৰাই Continuation_RecordFrame(runtime, &frame);
            }
#    যদি d_m3RecordBacktraces
            FillBacktraceFunctionInfo(runtime, frame.entry.function);
#    অন্যথা
            (শূন্য)frame.entry.function;
#    endif
        }
        ঘূৰাই r;
#  endif
    }

    ঘূৰাই r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
স্থির
শূন্য* ContinuationArgDest (IM3Continuation i_cont, u32 i_index)
{
    যদি (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : রিক্ত;
        u16         অঙ্ক_Rets = inner ? Get_Func_Type_অঙ্ক_Results(inner) : 0;

        ঘূৰাই i_cont->val_স্তূপ + (অঙ্ক_Rets + i_index) * c_io_Slot_গুনতি;
    }

    যদি (i_index < i_cont->অঙ্ক_Suspend_Results) {
        ঘূৰাই i_cont->sp + i_cont->suspendResultOffsets[i_index];
    }

    ঘূৰাই রিক্ত;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
স্থির
শূন্য HandOverToPeer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 অঙ্ক_Args = i_runtime->অঙ্ক_Suspend_Payload;
    u32 base = (io_peer->state == cont_allocated) ? io_peer->bound_Args_গুনতি : 0;

    বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
        শূন্য* dest = ContinuationArgDest(io_peer, base + i);

        যদি (dest) {
            যদি (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } অন্যথা {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    শূন্য* refDest = ContinuationArgDest(io_peer, base + অঙ্ক_Args);

    যদি (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    যদি (io_peer->state == cont_allocated) {
        io_peer->bound_Args_গুনতি += অঙ্ক_Args + 1;
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
স্থির
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    যেতিয়ালৈ (i_cont->অঙ্ক_Frames and (M3FrameKind) i_cont->frames[0].kind == frame_resume) {
        i_cont = i_cont->frames[0].resume.cont;
    }

    ঘূৰাই i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
স্থির
m3ret_t RunUnderHandlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_অঙ্ক_Handlers, M3MemoryHeader** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation parent = i_runtime->activeContinuation;
    IM3Memory       memory = m3MemInfo(*io_mem);
    m3ret_t         r;

    বাবে (;;) {
#  যদি d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        যদি (M3_UNLIKELY(cont->resumeThrow and cont->অঙ্ক_Frames == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = রিক্ত;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            ভঙ্গ;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        যদি (isStarting) {
            যদি (M3_UNLIKELY(not cont->entryFunction->compiled)) {
                r = CompileFunction(cont->entryFunction);

                যদি (M3_UNLIKELY(r)) {
                    ভঙ্গ;
                }
            }
            cont->pc = cont->entryFunction->compiled;
            cont->sp = cont->val_স্তূপ;
            cont->অঙ্ক_Frames = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->অঙ্ক_Handlers = i_অঙ্ক_Handlers;
        cont->parent = parent;
        cont->state = cont_running;
        cont->bound_Args_গুনতি = 0;

        i_runtime->activeContinuation = cont;

        যদি (cont->অঙ্ক_Frames) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->অঙ্ক_Frames - 1;
            cont->অঙ্ক_Frames = 0;

            r = ReplayFrames(cont, outermost, *io_mem);
        } অন্যথা যদি (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } অন্যথা {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = memory->mallocated;
        i_runtime->activeContinuation = parent;

        যদি (r != m3Err_continuationSuspended) {
            ভঙ্গ;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        যদি (not i_runtime->switchTarget or i_runtime->suspendHandlerCont != cont) {
            ভঙ্গ;
        }

        IM3Continuation peer = i_runtime->switchTarget;
        i_runtime->switchTarget = রিক্ত;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        যদি (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            ভঙ্গ;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_ContTypeOfRef(peer->entryFunction->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_অঙ্ক_Params(peerType) - 1)));
        HandOverToPeer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    ঘূৰাই r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
স্থির
শূন্য TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_অঙ্ক_Results)
{
    বাবে (u32 i = 0; i < i_অঙ্ক_Results; ++i) {
        i32 resSlot = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_Slot_গুনতি;

        যদি (is64) {
            *(u64*)(i_sp + resSlot) = *(u64*)(i_cont->val_স্তূপ + srcIdx);
        } অন্যথা {
            *(u32*)(i_sp + resSlot) = *(u32*)(i_cont->val_স্তূপ + srcIdx);
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
    IM3FuncType     contFuncType = immediate(IM3FuncType);
    i32             contSlot = immediate(i32);
    u32             mode = immediate(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + contSlot);
    যদি (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    যদি (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         অঙ্ক_Rets = Get_Func_Type_অঙ্ক_Results(innerType);

    যদি (mode == d_m3ResumeThrowRef) {
        i32 exnSlot = immediate(i32);

#  যদি d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exnSlot);

        যদি (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  অন্যথা
        (শূন্য)exnSlot;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } অন্যথা যদি (mode == d_m3ResumeThrow) {
        IM3Tag exnTag = immediate(IM3Tag);
        u32    অঙ্ক_Args = immediate(u32);

#  যদি d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exnTag, অঙ্ক_Args);

        যদি (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
            i32 offset = immediate(i32);
            u32 is64 = immediate(u32);

            exception->args[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  অন্যথা
        (শূন্য)exnTag;
        (শূন্য)অঙ্ক_Args;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } অন্যথা {
        u32 অঙ্ক_Args = immediate(u32);

        যদি (cont->state == cont_allocated) {
            বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
                i32 argSlot = immediate(i32);
                u32 is64 = immediate(u32);
                u32 targetIdx = (অঙ্ক_Rets + cont->bound_Args_গুনতি + i) * c_io_Slot_গুনতি;
                যদি (is64) {
                    *(u64*)(cont->val_স্তূপ + targetIdx) = *(u64*)(_sp + argSlot);
                } অন্যথা {
                    *(u32*)(cont->val_স্তূপ + targetIdx) = *(u32*)(_sp + argSlot);
                }
            }
            cont->bound_Args_গুনতি = 0;
        } অন্যথা {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
                i32 argSlot = immediate(i32);
                u32 is64 = immediate(u32);
                u32 dstIdx = cont->bound_Args_গুনতি + i;
                যদি (dstIdx < inner->অঙ্ক_Suspend_Results) {
                    i32 dstOffset = inner->suspendResultOffsets[dstIdx];
                    যদি (is64) {
                        *(u64*)(inner->sp + dstOffset) = *(u64*)(_sp + argSlot);
                    } অন্যথা {
                        *(u32*)(inner->sp + dstOffset) = *(u32*)(_sp + argSlot);
                    }
                }
            }
        }
    }

    u32  অঙ্ক_Handlers = immediate(u32);
    pc_t handlersPC = _pc;
    _pc += অঙ্ক_Handlers * 3;

    u32       অঙ্ক_Results = immediate(u32);
    pc_t      resultsPC = _pc;

    IM3Memory memory = m3MemInfo(_mem);

    m3ret_t   r = RunUnderHandlers(runtime, &cont, handlersPC, অঙ্ক_Handlers, &_mem);

    যদি (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, অঙ্ক_Results);
        Continuation_Recycle_স্তূপ(runtime, cont);

        _pc += অঙ্ক_Results * 2;
        nextOp();
    } অন্যথা যদি (r == m3Err_continuationSuspended) {
        যদি (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            যদি (M3_UNLIKELY(not runtime->activeContinuation)) {
                newTrap(m3Err_trapUnhandledControlTag);
            }
            M3Frame frame;
            frame.kind = frame_resume;
            frame.pc = resultsPC + অঙ্ক_Results * 2;
            frame.sp = _sp;
            frame.memory = memory;
            frame.resume.cont = cont;
            frame.resume.handlersPC = handlersPC;
            frame.resume.অঙ্ক_Handlers = অঙ্ক_Handlers;
            frame.resume.resultsPC = resultsPC;
            frame.resume.অঙ্ক_Results = অঙ্ক_Results;

            ঘূৰাই Continuation_RecordFrame(runtime, &frame);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        যদি (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        যদি (runtime->suspendStubPC) {
            jumpOpDirect(runtime->suspendStubPC);
        } অন্যথা {
            newTrap(m3Err_trapUnhandledControlTag);
        }
    } অন্যথা {
        cont->state = cont_consumed;
        Continuation_Recycle_স্তূপ(runtime, cont);
        ঠেলো_Backtrace_Frame_2();
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
    cont->type = immediate(IM3FuncType);

    u32 অঙ্ক_Args = immediate(u32);
    বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
        i32 offset = immediate(i32);
        u32 is64 = immediate(u32);
        u64 val = (i < runtime->অঙ্ক_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        যদি (is64) {
            *(u64*)(_sp + offset) = val;
        } অন্যথা {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 contOffset = immediate(i32);
    যদি (contOffset >= 0) {
        *(IM3Continuation*)(_sp + contOffset) = cont;
    }

    nextOp();
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
    IM3Continuation cont = runtime->activeContinuation;

    যদি (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    skip_immediate(IM3FuncType);

    IM3Tag          tag = immediate(IM3Tag);
    i32             targetSlot = immediate(i32);

    IM3Continuation target = *(IM3Continuation*)(_sp + targetSlot);

    যদি (M3_UNLIKELY(not target)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    যদি (M3_UNLIKELY(target->state == cont_consumed or target->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = FindHandler(cont, tag, 1, রিক্ত);

    যদি (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 অঙ্ক_Args = immediate(u32);

    runtime->অঙ্ক_Suspend_Payload = অঙ্ক_Args;

    বাবে (u32 i = 0; i < অঙ্ক_Args; ++i) {
        i32 argSlot = immediate(i32);
        u32 is64 = immediate(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + argSlot) : (u64) * (u32*)(_sp + argSlot);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 অঙ্ক_Results = immediate(u32);

    cont->অঙ্ক_Suspend_Results = অঙ্ক_Results;

    বাবে (u32 i = 0; i < অঙ্ক_Results; ++i) {
        cont->suspendResultOffsets[i] = immediate(i32);
        cont->suspendResultIs64[i] = immediate(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = রিক্ত;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switchTarget = target;
    runtime->suspendTag = tag;

    cont->pc = _pc;
    cont->suspendPoint = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  যদি d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    ঘূৰাই m3Err_continuationSuspended;
}


#endif // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = immediate(pc_t);

    যদি (condition) {
        nextOp();
    } অন্যথা {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = slot(i32);

    pc_t elsePC = immediate(pc_t);

    যদি (condition) {
        nextOp();
    } অন্যথা {
        jumpOp(elsePC);
    }
}


d_m3Op(BranchTable)
{
    u32   branchIndex = slot(u32);           // branch index is always in a slot
    u32   অঙ্ক_Targets = immediate(u32);

    pc_t* branches = (pc_t*)_pc;

    যদি (branchIndex > অঙ্ক_Targets) {
        branchIndex = অঙ্ক_Targets; // the default index
    }

    jumpOp(branches[branchIndex]);
}


#define d_m3SetRegisterSetSlot(TYPE, REG) \
d_m3Op  (SetRegister_##TYPE)            \
{                                       \
    TYPE * src = slot_ptr (TYPE);       \
    d_m3PreloadNext ();                 \
    REG = * src;                        \
    nextOpPreloaded ();                 \
}                                       \
                                        \
d_m3Op (SetSlot_##TYPE)                 \
{                                       \
    TYPE * dst = slot_ptr (TYPE);       \
    d_m3PreloadNext ();                 \
    * dst = (TYPE) REG;                 \
    nextOpPreloaded ();                 \
}                                       \
                                        \
d_m3Op (PreserveSetSlot_##TYPE)         \
{                                       \
    TYPE * স্তূপ     = slot_ptr (TYPE); \
    TYPE * preserve  = slot_ptr (TYPE); \
    d_m3PreloadNext ();                 \
                                        \
    * preserve = * স্তূপ;               \
    * স্তূপ = (TYPE) REG;               \
                                        \
    nextOpPreloaded ();                 \
}

d_m3SetRegisterSetSlot(i32, _r0)
d_m3SetRegisterSetSlot(i64, _r0)
#যদি d_m3HasFloat
d_m3SetRegisterSetSlot(f32, _fp0)
d_m3SetRegisterSetSlot(f64, _fp0)
#endif

d_m3Op(CopySlot_32)
{
    u32* dst = slot_ptr(u32);
    u32* src = slot_ptr(u32);
    d_m3PreloadNext();

    *dst = *src;

    nextOpPreloaded();
}


d_m3Op(PreserveCopySlot_32)
{
    u32* dest = slot_ptr(u32);
    u32* src = slot_ptr(u32);
    u32* preserve = slot_ptr(u32);
    d_m3PreloadNext();

    *preserve = *dest;
    *dest = *src;

    nextOpPreloaded();
}


d_m3Op(CopySlot_64)
{
    u64* dst = slot_ptr(u64);
    u64* src = slot_ptr(u64);
    d_m3PreloadNext();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    nextOpPreloaded();
}


d_m3Op(PreserveCopySlot_64)
{
    u64* dest = slot_ptr(u64);
    u64* src = slot_ptr(u64);
    u64* preserve = slot_ptr(u64);
    d_m3PreloadNext();

    *preserve = *dest;
    *dest = *src;

    nextOpPreloaded();
}


#যদি d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_স্তূপ)
{
    u32         opcodeIndex = immediate(u32);
    u32         স্তূপ_Height = immediate(u32);
    IM3Function function = immediate(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    printf(" %4d ", opcodeIndex);
    printf(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  যদি d_m3HasFloat
    printf("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    বাবে (u32 i = 0; i < স্তূপ_Height; ++i) {
        cstr_t kind = "";

        printf("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, kind, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    printf("---------------------------------------------------------------------------------------------------------\n");

    nextOpDirect();
}
#endif


#define d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = slot (i32);                 \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#define d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    nextOp ();                                  \
}

#যদি d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, slot(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, slot(i32))
#endif

d_m3Op(Return)
{
    m3_স্তূপ_Check();
    ঘূৰাই m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = immediate(pc_t);

    যদি (condition) {
        jumpOp(branch);
    } অন্যথা {
        nextOp();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = slot(i32);
    pc_t branch = immediate(pc_t);

    যদি (condition) {
        jumpOp(branch);
    } অন্যথা {
        nextOp();
    }
}

#যদি d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t branch  = immediate (pc_t);                        \
      যদি (operand OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      অন্যথা nextOp ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t branch  = immediate (pc_t);                        \
      যদি (((TYPE) _r0) OPER operand)                          \
          jumpOp (branch);                                    \
      অন্যথা nextOp ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = slot (TYPE);                            \
      TYPE operand1 = slot (TYPE);                            \
      pc_t branch   = immediate (pc_t);                       \
      যদি (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      অন্যথা nextOp ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t elsePC  = immediate (pc_t);                        \
      যদি (operand OPER ((TYPE) _r0))                          \
          nextOp ();                                          \
      অন্যথা jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t elsePC  = immediate (pc_t);                        \
      যদি (((TYPE) _r0) OPER operand)                          \
          nextOp ();                                          \
      অন্যথা jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = slot (TYPE);                            \
      TYPE operand1 = slot (TYPE);                            \
      pc_t elsePC   = immediate (pc_t);                       \
      যদি (operand1 OPER operand2)                             \
          nextOp ();                                          \
      অন্যথা jumpOp (elsePC);                                   \
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
    pc_t branch = immediate(pc_t);
    যদি (((i32)_r0) == 0) {
        jumpOp(branch);
    } অন্যথা {
        nextOp();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  operand = slot(i32);
    pc_t branch = immediate(pc_t);
    যদি (operand == 0) {
        jumpOp(branch);
    } অন্যথা {
        nextOp();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = immediate(pc_t);
    যদি (((i32)_r0) == 0) {
        nextOp();
    } অন্যথা {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  operand = slot(i32);
    pc_t elsePC = immediate(pc_t);
    যদি (operand == 0) {
        nextOp();
    } অন্যথা {
        jumpOp(elsePC);
    }
}

#endif // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = immediate(pc_t);

    যদি (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        nextOp();
    } অন্যথা {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = slot(i32);
    pc_t branch = immediate(pc_t);

    যদি (condition) {
        nextOp();
    } অন্যথা {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_স্তূপ_Check();

    শূন্য* loopId = immediate(শূন্য*);
    ঘূৰাই loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    শূন্য* loopId = immediate(শূন্য*);

    যদি (condition) {
        ঘূৰাই loopId;
    } অন্যথা {
        nextOp();
    }
}


#যদি d_m3_Has_স্তূপ_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_স্তূপ_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    যদি (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
        SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_op, _pc - 1, _sp,
                          d_m3ExpRegArgs(_r0, _fp0));

        ঘূৰাই m3Err_continuationSuspended;
    }

    শূন্য* loopId = immediate(শূন্য*);
    ঘূৰাই loopId;
}


#  যদি !d_m3EntryKeepsFrame
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(Entry_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = immediate(IM3Function);

    যদি (EntryFits(function, _sp, _mem)) {
        EnterFrame(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        যদি (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
            SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_entry, _pc, _sp,
                              d_m3ExpRegArgs(_r0, _fp0));

            ঘূৰাই m3Err_continuationSuspended;
        }

        nextOpDirect();
    } অন্যথা {
        newTrap(m3_Err_trap_স্তূপ_Overflow);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(EntryCheck)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    যদি (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
        SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_entry, _pc - 1, _sp,
                          d_m3ExpRegArgs(_r0, _fp0));

        ঘূৰাই m3Err_continuationSuspended;
    }

    nextOp();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    শূন্য* loopId = immediate(শূন্য*);

    যদি (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        যদি (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
            SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_op, _pc - 2, _sp,
                              d_m3ExpRegArgs(_r0, _fp0));

            ঘূৰাই m3Err_continuationSuspended;
        }
        ঘূৰাই loopId;
    } অন্যথা {
        nextOp();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
স্থির
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3MemoryHeader* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  যদি d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    যদি (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } অন্যথা যদি (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } অন্যথা যদি (op == op_EntryCheck) {
        ঘূৰাই nextOpImpl();
    } অন্যথা {
        ঘূৰাই d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  যদি (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    ঘূৰাই op(_pc, d_m3OpArgs, d_m3BaseCstr);
#  অন্যথা
    ঘূৰাই op(_pc, d_m3OpArgs);
#  endif
}
#endif // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 মান = *(u32*)_pc++;
    slot(u32) = মান;
    nextOp();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 মান;
    memcpy(&মান, _pc, পরিমাণ(মান));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    slot(u64) = মান;
    nextOp();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_স্তূপ_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(End)
{
    m3_স্তূপ_Check();
    ঘূৰাই m3Err_none;
}


d_m3Op(SetGlobal_s32)
{
    u32* global = immediate(u32*);
    *global = slot(u32);

    nextOp();
}


d_m3Op(SetGlobal_s64)
{
    u64* global = immediate(u64*);
    *global = slot(u64);

    nextOp();
}

#যদি d_m3HasFloat
d_m3Op(SetGlobal_f32)
{
    f32* global = immediate(f32*);
    *global = (f32)_fp0;

    nextOp();
}


d_m3Op(SetGlobal_f64)
{
    f64* global = immediate(f64*);
    *global = _fp0;

    nextOp();
}
#endif


#যদি d_m3HasMemory64

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
    u64  operand = slot(u64);
    u32* address = slot_ptr(u32);

    u64  offset;
    memcpy(&offset, _pc, পরিমাণ(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    যদি (M3_LIKELY(operand < d_m3AddressLimit)) {
        u64 effective = operand + offset;

        // the access itself checks its own size against what is left
        যদি (M3_LIKELY(effective < _mem->length)) {
            *address = (u32)effective;
            nextOp();
        }
    }

    d_outOfBounds;
}

#endif // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#যদি d_m3SkipMemoryBoundsCheck || d_m3GuardedMemory
#  define m3MemCheck(x) true
#অন্যথা
#  define m3MemCheck(x) M3_LIKELY(x)
#endif

// memcpy here is to support non-aligned access on some platforms.

#define d_m3Load(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = immediate (u32);                       \
    d_m3PreloadNext ();                                 \
    u64 operand = (u32) _r0;                            \
    operand += offset;                                  \
                                                        \
    যদি (m3MemCheck(                                     \
        operand + পরিমাণ (SRC_TYPE) <= _mem->length     \
    )) {                                                \
        {                                               \
            u8* src8 = m3MemData(_mem) + operand;       \
            SRC_TYPE মান;                             \
            memcpy(&মান, src8, পরিমাণ(মান));        \
            M3_BSWAP_##SRC_TYPE(মান);                 \
            REG = (DEST_TYPE)মান;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        nextOpPreloaded ();                             \
    } অন্যথা d_outOfBounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    d_m3PreloadNext ();                                 \
    operand += offset;                                  \
                                                        \
    যদি (m3MemCheck(                                     \
        operand + পরিমাণ (SRC_TYPE) <= _mem->length     \
    )) {                                                \
        {                                               \
            u8* src8 = m3MemData(_mem) + operand;       \
            SRC_TYPE মান;                             \
            memcpy(&মান, src8, পরিমাণ(মান));        \
            M3_BSWAP_##SRC_TYPE(মান);                 \
            REG = (DEST_TYPE)মান;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        nextOpPreloaded ();                             \
    } অন্যথা d_outOfBounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#define d_m3Load_i(DEST_TYPE, SRC_TYPE) d_m3Load(_r0, DEST_TYPE, SRC_TYPE)
#define d_m3Load_f(DEST_TYPE, SRC_TYPE) d_m3Load(_fp0, DEST_TYPE, SRC_TYPE)

#যদি d_m3HasFloat
d_m3Load_f(f32, f32);
d_m3Load_f(f64, f64);
#endif

d_m3Load_i(i32, i8);
d_m3Load_i(i32, u8);
d_m3Load_i(i32, i16);
d_m3Load_i(i32, u16);
d_m3Load_i(i32, i32);

d_m3Load_i(i64, i8);
d_m3Load_i(i64, u8);
d_m3Load_i(i64, i16);
d_m3Load_i(i64, u16);
d_m3Load_i(i64, i32);
d_m3Load_i(i64, u32);
d_m3Load_i(i64, i64);

#যদি d_m3FoldSetLocal

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3LoadFold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = immediate (u32);                       \
      DEST_TYPE * dest = slot_ptr (DEST_TYPE);            \
      d_m3PreloadNext ();                                 \
      u64 operand = (u32) _r0;                            \
      operand += offset;                                  \
                                                          \
      যদি (m3MemCheck(                                     \
          operand + পরিমাণ (SRC_TYPE) <= _mem->length     \
      )) {                                                \
          u8* src8 = m3MemData(_mem) + operand;           \
          SRC_TYPE মান;                                 \
          memcpy(&মান, src8, পরিমাণ(মান));            \
          M3_BSWAP_##SRC_TYPE(মান);                     \
          DEST_TYPE result = (DEST_TYPE)মান;            \
          * dest = result;  REG = result;                 \
          d_m3TraceLoad(DEST_TYPE, operand, result);      \
          nextOpPreloaded ();                             \
      } অন্যথা d_outOfBounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 operand = slot (u32);                           \
      u32 offset = immediate (u32);                       \
      DEST_TYPE * dest = slot_ptr (DEST_TYPE);            \
      d_m3PreloadNext ();                                 \
      operand += offset;                                  \
                                                          \
      যদি (m3MemCheck(                                     \
          operand + পরিমাণ (SRC_TYPE) <= _mem->length     \
      )) {                                                \
          u8* src8 = m3MemData(_mem) + operand;           \
          SRC_TYPE মান;                                 \
          memcpy(&মান, src8, পরিমাণ(মান));            \
          M3_BSWAP_##SRC_TYPE(মান);                     \
          DEST_TYPE result = (DEST_TYPE)মান;            \
          * dest = result;  REG = result;                 \
          d_m3TraceLoad(DEST_TYPE, operand, result);      \
          nextOpPreloaded ();                             \
      } অন্যথা d_outOfBounds;                               \
  }

d_m3LoadFold(_r0, i32, i8)
d_m3LoadFold(_r0, i32, u8)
d_m3LoadFold(_r0, i32, i16)
d_m3LoadFold(_r0, i32, u16)
d_m3LoadFold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3LoadFold(_r0, i64, i8)
d_m3LoadFold(_r0, i64, u8)
d_m3LoadFold(_r0, i64, i16)
d_m3LoadFold(_r0, i64, u16)
d_m3LoadFold(_r0, i64, i32)
d_m3LoadFold(_r0, i64, u32)
d_m3LoadFold(_r0, i64, i64)
#  যদি d_m3HasFloat
d_m3LoadFold(_fp0, f32, f32)
d_m3LoadFold(_fp0, f64, f64)
#  endif

#endif // d_m3FoldSetLocal

#define d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    d_m3PreloadNext ();                                 \
    operand += offset;                                  \
                                                        \
    যদি (m3MemCheck(                                     \
        operand + পরিমাণ (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, REG);     \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            memcpy(mem8, &val, পরিমাণ(val));            \
        }                                               \
        nextOpPreloaded ();                             \
    } অন্যথা d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ধ্রুৱ SRC_TYPE মান = slot (SRC_TYPE);             \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    d_m3PreloadNext ();                                 \
    operand += offset;                                  \
                                                        \
    যদি (m3MemCheck(                                     \
        operand + পরিমাণ (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, মান);   \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) মান;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            memcpy(mem8, &val, পরিমাণ(val));            \
        }                                               \
        nextOpPreloaded ();                             \
    } অন্যথা d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ধ্রুৱ SRC_TYPE মান = slot (SRC_TYPE);             \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    d_m3PreloadNext ();                                 \
    operand += offset;                                  \
                                                        \
    যদি (m3MemCheck(                                     \
        operand + পরিমাণ (DEST_TYPE) <= _mem->length    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, মান);   \
            u8* mem8 = m3MemData(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) মান;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            memcpy(mem8, &val, পরিমাণ(val));            \
        }                                               \
        nextOpPreloaded ();                             \
    } অন্যথা d_outOfBounds;                               \
}

// both operands can be in regs when storing a float
#define d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    d_m3PreloadNext ();                                 \
    operand += offset;                                  \
                                                        \
    যদি (m3MemCheck(                                     \
        operand + পরিমাণ (TYPE) <= _mem->length         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, operand, REG);         \
            u8* mem8 = m3MemData(_mem) + operand;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            memcpy(mem8, &val, পরিমাণ(val));            \
        }                                               \
        nextOpPreloaded ();                             \
    } অন্যথা d_outOfBounds;                               \
}


#define d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#define d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#যদি d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#endif

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#অন্তর্ভাৱ "m3_exec_atomic.h"

#undef m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#যদি d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    ৱর্ণ name[100];
    strcpy(name, strstr(i_opcode, "op_") + 3);
    ৱর্ণ* bracket = strstr(name, "(");
    যদি (bracket) {
        *bracket = 0;
    }

    puts(name);
    nextOpDirect();
}
#endif

#যদি d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    ProfileHit(i_operationName);

    nextOpDirect();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    ProfileHit(i_operationName);

    nextOpDirect();
}
#endif

d_m3EndExternC

#endif // m3_exec_h
