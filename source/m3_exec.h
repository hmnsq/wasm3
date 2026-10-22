//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#अगर_अघोषित m3_exec_h
#घोषणा m3_exec_h

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

#अगर_अघोषित M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#पूर्ण_अगर

#समावेश "m3_math_utils.h"
#समावेश "m3_compile.h"
#समावेश "m3_env.h"
#समावेश "m3_info.h"
#समावेश "m3_exec_defs.h"
#समावेश "m3_atomic.h"
#समावेश "m3_host.h"

#समावेश <limits.h>

d_m3BeginExternC

#घोषणा rewrite_op(OP)             * ((व्योम **) (_pc-1)) = (व्योम*)(OP)

#घोषणा immediate(TYPE)            * ((TYPE *) _pc++)
#घोषणा skip_immediate(TYPE)       (_pc++)

#घोषणा slot(TYPE)                 * (TYPE *) (_sp + immediate (i32))
#घोषणा slot_ptr(TYPE)             (TYPE *) (_sp + immediate (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#घोषणा d_m3WideOperand(IS64)      ((IS64) ? slot (u64) : (u64) slot (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#घोषणा d_m3MemRangeOk(START, लंबाई_2, MEM)  \
   ((START) <= (MEM)->लंबाई and (लंबाई_2) <= (MEM)->लंबाई - (START))


#अगर d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define अग्र_Op()                 M3_MUSTTAIL वापस profileOp (d_m3OpAllArgs, __FUNCTION__)
#या_अगर d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define अग्र_Op()                 M3_MUSTTAIL वापस debugOp (d_m3OpAllArgs, __FUNCTION__)
#अन्यथा
#  define अग्र_Op()                 अग्र_Op_Direct()
#पूर्ण_अगर

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#अगर d_m3_Preload_अग्र_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_अग्र()        स्थिर IM3Operation अग्र_Op_2 = (IM3Operation) (* _pc)
#  define अग्र_Op_Preloaded()        M3_MUSTTAIL वापस अग्र_Op_2 (_pc + 1, d_m3OpArgs)
#अन्यथा
#  define d_m3_Preload_अग्र()
#  define अग्र_Op_Preloaded()        अग्र_Op()
#पूर्ण_अगर

#अगर d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL वापस profileJumpOp ((pc_t)(PC), d_m3OpArgs, __FUNCTION__)
#अन्यथा
#  define jumpOp(PC)               jumpOpDirect(PC)
#पूर्ण_अगर

#अगर d_m3RecordBacktraces
#  define धकेलो_Backtrace_Frame_2()            (धकेलो_Backtrace_Frame (_mem->runtime, _pc - 1))
#  define fillBacktraceFrame(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    वापस (धकेलो_Backtrace_Frame_2 (), err)
#  define forwardTrap(err)                वापस err
#अन्यथा
#  define धकेलो_Backtrace_Frame_2()            करो {} जबतक (0)
#  define fillBacktraceFrame(FUNCTION)    करो {} जबतक (0)

#  define newTrap(err)                    वापस err
#  define forwardTrap(err)                वापस err
#पूर्ण_अगर

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#अगर d_m3_Max_Native_ढेर > 0
#  define d_m3_Check_Native_ढेर()                                               \
       करो {                                                                    \
           व्योम * _m3Limit = m3MemRuntime (_mem)->ढेर_Limit;                  \
           अगर (M3_UNLIKELY (_m3Limit && m3_Native_ढेर_Ptr () < _m3Limit))      \
               newTrap (m3_Err_trap_ढेर_Overflow);                              \
       } जबतक (0)
#अन्यथा
#  define d_m3_Check_Native_ढेर()           करो {} जबतक (0)
#पूर्ण_अगर


#अगर d_m3_Has_ढेर_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  अगर d_m3HasFloat
#    define d_m3ExpRegArgs(R0, FP0)        (R0), (FP0)
#  अन्यथा
#    define d_m3ExpRegArgs(R0, FP0)        (R0)
#  endif

#  अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3ExpRegArgs (R0, FP0), d_m3BaseCstr)
#  अन्यथा
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
अटल M3_NOINLINE
m3ret_t RecordCallFrame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3Memory i_memory,
                         IM3Function i_function, m3reg_t i_r0
#  अगर d_m3HasFloat
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
#  अगर d_m3HasFloat
    frame.call.fp0 = i_fp0;
#  endif

    वापस Continuation_RecordFrame(i_runtime, &frame);
}

#  define d_m3RecordCallFrame(MEMORY, FUNCTION)                                \
       अगर (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           वापस RecordCallFrame (m3MemRuntime (_mem), _pc, _sp, (MEMORY),    \
                                   (FUNCTION), d_m3ExpRegArgs (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
अटल M3_NOINLINE
m3ret_t RecordLoopFrame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3Memory i_memory)
{
    M3Frame frame;

    frame.kind = frame_loop;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.memory = i_memory;

    वापस Continuation_RecordFrame(i_runtime, &frame);
}


#  अगर d_m3HasExceptionHandling
अटल M3_NOINLINE
m3ret_t RecordTryFrame (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3Memory i_memory,
                        u32 i_अंक_Clauses, bool i_handlersLive)
{
    M3Frame frame;

    frame.kind = frame_try;
    frame.pc = i_clauses;
    frame.sp = i_sp;
    frame.memory = i_memory;
    frame.try_.अंक_Clauses = i_अंक_Clauses;
    frame.try_.handlersLive = i_handlersLive;

    वापस Continuation_RecordFrame(i_runtime, &frame);
}
#  endif


#  अगर d_m3EntryKeepsFrame
अटल M3_NOINLINE
m3ret_t RecordEntryFrame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3Memory i_memory,
                          IM3Function i_function)
{
    M3Frame frame;

    frame.kind = frame_entry;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.memory = i_memory;
    frame.entry.function = i_function;

    वापस Continuation_RecordFrame(i_runtime, &frame);
}
#  endif
#अन्यथा
#  define d_m3RecordCallFrame(MEMORY, FUNCTION)
#पूर्ण_अगर


#अगर d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            ख_लिखो(मानक_त्रुटि, fmt "\n", ##__VA_ARGS__)
#या_अगर d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    स्थिर IM3Runtime trace_rt = m3MemRuntime(_mem); (व्योम) trace_rt;
#  define d_m3TracePrint(fmt, ...)            ख_लिखो(मानक_त्रुटि, "%*s" fmt "\n", (trace_rt->callDepth)*2, "", ##__VA_ARGS__)
#अन्यथा
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#पूर्ण_अगर

#अगर d_m3EnableStrace >= 3
#  define d_m3TraceLoad(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#अन्यथा
#  define d_m3TraceLoad(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#पूर्ण_अगर

#अगर_घोषित DEBUG
  #घोषणा d_outOfBounds newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->लंबाई, operand))

#  define d_outOfBoundsMemOp(OFFSET, आकार_2) newTrap (ErrorRuntime (m3Err_trapOutOfBoundsMemoryAccess,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->लंबाई, OFFSET, आकार_2))
#अन्यथा
  #घोषणा d_outOfBounds newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#  define d_outOfBoundsMemOp(OFFSET, आकार_2) newTrap (m3Err_trapOutOfBoundsMemoryAccess)

#पूर्ण_अगर

#अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#अन्यथा
d_m3RetSig Call (d_m3OpSig)
#पूर्ण_अगर
{
    m3ret_t possible_trap = m3_Yield();
    अगर (M3_UNLIKELY(possible_trap)) {
        वापस possible_trap;
    }

    अग्र_Op_Direct();
}

#घोषणा d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    d_m3_Preload_अग्र ();                                 \
    OP((RES), operand, ((TYPE) REG), ##__VA_ARGS__);    \
    अग्र_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = slot (TYPE);                        \
    TYPE operand1 = slot (TYPE);                        \
    d_m3_Preload_अग्र ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    अग्र_Op_Preloaded ();                                 \
}

#घोषणा d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    d_m3_Preload_अग्र ();                                 \
    OP((RES), ((TYPE) REG), operand, ##__VA_ARGS__);    \
    अग्र_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#घोषणा d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#घोषणा d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#घोषणा d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#घोषणा d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#घोषणा M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#घोषणा M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#घोषणा M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#घोषणा M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#घोषणा d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#घोषणा d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#घोषणा d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#घोषणा d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#घोषणा d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#घोषणा d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#घोषणा d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#घोषणा d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#घोषणा d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#घोषणा d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#घोषणा d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#घोषणा d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#घोषणा d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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

#अगर d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#पूर्ण_अगर

#घोषणा OP_ADD_32(A,B) (i32)((u32)(A) + (u32)(B))
#घोषणा OP_ADD_64(A,B) (i64)((u64)(A) + (u64)(B))
#घोषणा OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#घोषणा OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#घोषणा OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#घोषणा OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, Add,      OP_ADD_32)  d_m3CommutativeOpFunc_i (i64, Add,      OP_ADD_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, Subtract,            OP_SUB_32)  d_m3OpFunc_i (i64, Subtract,            OP_SUB_64)

#घोषणा OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#घोषणा OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#घोषणा OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#घोषणा OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, Shift_बायाँ,       OP_SHL_32)      d_m3OpFunc_i (u64, Shift_बायाँ,       OP_SHL_64)
d_m3OpFunc_i (i32, Shift_दायाँ,      OP_SHR_32)      d_m3OpFunc_i (i64, Shift_दायाँ,      OP_SHR_64)
d_m3OpFunc_i (u32, Shift_दायाँ,      OP_SHR_32)      d_m3OpFunc_i (u64, Shift_दायाँ,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#अगर d_m3FoldSetLocal

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
    d_m3_Preload_अग्र ();                                 \
    TYPE परिणाम;                                        \
    OP((परिणाम), operand, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = परिणाम;  REG = परिणाम;                     \
    अग्र_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = slot (TYPE);                        \
    TYPE operand1 = slot (TYPE);                        \
    TYPE * dest = slot_ptr (TYPE);                      \
    d_m3_Preload_अग्र ();                                 \
    TYPE परिणाम;                                        \
    OP((परिणाम), operand1, operand2, ##__VA_ARGS__);    \
    * dest = परिणाम;  REG = परिणाम;                     \
    अग्र_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE operand = slot (TYPE);                         \
    TYPE * dest = slot_ptr (TYPE);                      \
    d_m3_Preload_अग्र ();                                 \
    TYPE परिणाम;                                        \
    OP((परिणाम), ((TYPE) REG), operand, ##__VA_ARGS__); \
    * dest = परिणाम;  REG = परिणाम;                     \
    अग्र_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#घोषणा d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#घोषणा d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#घोषणा d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#घोषणा d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, Add,          M3_FUNC, OP_ADD_32)
d_m3FoldOp_i           (i32, Subtract,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, Shift_बायाँ,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, Shift_दायाँ,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, Shift_दायाँ,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, Add,          M3_FUNC, OP_ADD_64)
d_m3FoldOp_i           (i64, Subtract,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, Shift_बायाँ,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, Shift_दायाँ,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, Shift_दायाँ,   M3_FUNC, OP_SHR_64)

#  अगर d_m3HasFloat
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
#पूर्ण_अगर

#पूर्ण_अगर // d_m3FoldSetLocal

#अगर d_m3HasFloat
d_m3CommutativeArithOp_f(f32, Add,         +)      d_m3CommutativeArithOp_f(f64, Add,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, Subtract,               -)      d_m3ArithOp_f(f64, Subtract,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#पूर्ण_अगर

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

#अगर d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#पूर्ण_अगर

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#घोषणा d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    अग्र_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE operand = slot (TYPE);                     \
    OP((RES), operand, ##__VA_ARGS__);              \
    अग्र_Op ();                                      \
}

#घोषणा M3_UNARY(RES, X, OP) (RES) = OP(X)
#घोषणा M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#घोषणा d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#घोषणा d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#घोषणा d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#अगर d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             भ_असल);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       उच्चमान);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      न्यूनमान);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       वर्ग_मूल);
d_m3UnaryArithOp_f(f32, Nearest,    rintf);    d_m3UnaryArithOp_f(f64, Nearest,    rint);
#  अगर defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  अन्यथा
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#पूर्ण_अगर

#घोषणा OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, EqualToZero, OP_EQZ)
d_m3UnaryOp_i(i64, EqualToZero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#अगर (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#घोषणा OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#घोषणा OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#घोषणा OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#घोषणा OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#या_अगर defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#घोषणा OP_CLZ_32(x) __builtin_clz(x)
#घोषणा OP_CTZ_32(x) __builtin_ctz(x)
#घोषणा OP_CLZ_64(x) __builtin_clzll(x)
#घोषणा OP_CTZ_64(x) __builtin_ctzll(x)
#अन्यथा
#घोषणा OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#घोषणा OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#घोषणा OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#घोषणा OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#पूर्ण_अगर

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#घोषणा OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    अग्र_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 operand = slot(i64);
    _r0 = OP_WRAP_I64(operand);
    अग्र_Op();
}

// Integer sign extension operations
#घोषणा OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#घोषणा OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#घोषणा OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#घोषणा OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#घोषणा OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#अगर d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_ऊपरी = slot (u64);
    u64  rhs_निचला  = slot (u64);
    u64  lhs_ऊपरी = slot (u64);
    u64  lhs_निचला  = slot (u64);
    u64* निचला     = slot_ptr (u64);
    d_m3_Preload_अग्र();

    u64 योग_2 = lhs_निचला + rhs_निचला;

    *निचला = योग_2;
    _r0  = (i64) (lhs_ऊपरी + rhs_ऊपरी + (योग_2 < lhs_निचला));

    अग्र_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_ऊपरी = slot (u64);
    u64  rhs_निचला  = slot (u64);
    u64  lhs_ऊपरी = slot (u64);
    u64  lhs_निचला  = slot (u64);
    u64* निचला     = slot_ptr (u64);
    d_m3_Preload_अग्र();

    *निचला = lhs_निचला - rhs_निचला;
    _r0  = (i64) (lhs_ऊपरी - rhs_ऊपरी - (lhs_निचला < rhs_निचला));

    अग्र_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = slot (i64);
    i64  a   = slot (i64);
    u64* निचला = slot_ptr (u64);
    d_m3_Preload_अग्र();

    u64 ऊपरी;
    *निचला = mul_wide_i64 (a, b, &ऊपरी);
    _r0  = (i64) ऊपरी;

    अग्र_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = slot (u64);
    u64  a   = slot (u64);
    u64* निचला = slot_ptr (u64);
    d_m3_Preload_अग्र();

    u64 ऊपरी;
    *निचला = mul_wide_u64 (a, b, &ऊपरी);
    _r0  = (i64) ऊपरी;

    अग्र_Op_Preloaded();
}

#पूर्ण_अगर // d_m3HasWideArithmetic

#घोषणा d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    अग्र_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * ढेर = slot_ptr (FROM);                 \
    OP((DEST), (* ढेर), ##__VA_ARGS__);           \
    अग्र_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    अग्र_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * ढेर = slot_ptr (FROM);                 \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (* ढेर), ##__VA_ARGS__);         \
    अग्र_Op ();                                      \
}

#अगर d_m3HasFloat
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
#पूर्ण_अगर

#घोषणा d_m3TypeModifyOp(REG_TO, REG_FROM, TO, NAME, FROM)  \
d_m3Op(TO##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
    अग्र_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#घोषणा d_m3FloatModifyOp(TO, NAME, FROM)                   \
d_m3Op(TO##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((TO) ((FROM) _fp0));             \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    _fp0 = canon_nan_##TO ((TO) (from));                    \
    अग्र_Op ();                                              \
}

#अगर d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#पूर्ण_अगर

#घोषणा d_m3TypeConvertOp(REG_TO, REG_FROM, TO, NAME, FROM) \
d_m3Op(TO##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_TO = (TO) ((FROM) REG_FROM);                        \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    slot (TO) = (TO) ((FROM) REG_FROM);                     \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_TO = (TO) (from);                                   \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    slot (TO) = (TO) (from);                                \
    अग्र_Op ();                                              \
}

// Int to float
#अगर d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#पूर्ण_अगर

#घोषणा d_m3ReinterpretOp(REG, TO, SRC, FROM)               \
d_m3Op(TO##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    जोड़ { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    जोड़ { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    REG = u.t;                                              \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    जोड़ { FROM c; TO t; } u;                              \
    u.c = (FROM) SRC;                                       \
    slot (TO) = u.t;                                        \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(TO##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    जोड़ { FROM c; TO t; } u;                              \
    u.c = slot (FROM);                                      \
    slot (TO) = u.t;                                        \
    अग्र_Op ();                                              \
}

#अगर d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#पूर्ण_अगर

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = immediate(u32*);
    slot(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    अग्र_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = immediate(u64*);
    slot(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    अग्र_Op();
}


d_m3Op(SetGlobal_i32)
{
    u32* global = immediate(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    अग्र_Op();
}


d_m3Op(SetGlobal_i64)
{
    u64* global = immediate(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    अग्र_Op();
}


#अगर d_m3_Has_ढेर_Switching

d_m3Op(Branch);
d_m3Op(Entry);
#  अगर !d_m3EntryKeepsFrame
d_m3Op(Entry_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
अटल M3_NOINLINE
IM3Function FunctionOfCompiledCode (pc_t i_pc)
{
    अगर (not i_pc) {
        वापस शून्य;
    }

    जबतक ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation entry = (IM3Operation)i_pc[0];

#  अगर !d_m3EntryKeepsFrame
    अगर (entry == op_Entry_Suspendable) {
        वापस (IM3Function)i_pc[1];
    }
#  endif
    वापस (entry == op_Entry) ? (IM3Function)i_pc[1] : शून्य;
}

#पूर्ण_अगर

d_m3Op(Call)
{
    d_m3_Check_Native_ढेर();

    pc_t      callPC = immediate(pc_t);
    i32       ढेर_Offset = immediate(i32);
    IM3Memory memory = m3MemInfo(_mem);

    m3stack_t sp = _sp + ढेर_Offset;

#अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#अन्यथा
    m3ret_t r = Call(callPC, sp, _mem, d_m3OpDefaultArgs);
#पूर्ण_अगर

    _mem = memory->mallocated;

    अगर (M3_LIKELY(not r)) {
        अग्र_Op();
    } अन्यथा {
        d_m3RecordCallFrame(memory, FunctionOfCompiledCode(callPC));
        धकेलो_Backtrace_Frame_2();
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
    i32         ढेर_Offset = immediate(i32);
    IM3Memory   memory = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ढेर_Offset;

    m3ret_t     r = m3Err_none;

    अगर (M3_LIKELY(function)) {
        अगर (M3_LIKELY(type == function->funcType)) {
            अगर (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            अगर (M3_LIKELY(not r)) {
#अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3OpDefaultArgs, d_m3BaseCstr);
#अन्यथा
                r = Call(function->compiled, sp, _mem, d_m3OpDefaultArgs);
#पूर्ण_अगर

                _mem = memory->mallocated;

                अगर (M3_LIKELY(not r)) {
                    अग्र_Op_Direct();
                } अन्यथा {
                    d_m3RecordCallFrame(memory, function);
                    धकेलो_Backtrace_Frame_2();
                    forwardTrap(r);
                }
            }
        } अन्यथा {
            r = m3Err_trapIndirectCallTypeMismatch;
        }
    } अन्यथा {
        r = m3Err_trapNullFunctionRef;
    }

    अगर (M3_UNLIKELY(r)) {
        newTrap(r);
    } अन्यथा {
        forwardTrap(r);
    }
}


// ref.as_non_null is a null check and nothing else: the value stays put, so
// there is no destination slot, only the trap.
d_m3Op(RefAsNonNull)
{
    IM3Function reference = slot(IM3Function);

    अगर (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    अग्र_Op();
}


d_m3Op(CallIndirect)
{
    d_m3_Check_Native_ढेर();

    M3Table*    table = immediate(M3Table*);
    u64         table_सूचकांक = d_m3WideOperand(table->isTable64);
    IM3FuncType type = immediate(IM3FuncType);
    i32         ढेर_Offset = immediate(i32);
    IM3Memory   memory = m3MemInfo(_mem);

    m3stack_t   sp = _sp + ढेर_Offset;

    m3ret_t     r = m3Err_none;

    अगर (M3_LIKELY(table_सूचकांक < table->आकार)) {
        IM3Function function = (IM3Function)table->elements[table_सूचकांक];

        अगर (M3_LIKELY(function)) {
            अगर (M3_LIKELY(type == function->funcType)) {
                अगर (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                अगर (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3MemoryHeader* calleeMem = Module_MemoryHeader(function->module);

#अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3OpDefaultArgs, d_m3BaseCstr);
#अन्यथा
                    r = Call(function->compiled, sp, calleeMem, d_m3OpDefaultArgs);
#पूर्ण_अगर

                    _mem = memory->mallocated;

                    अगर (M3_LIKELY(not r)) {
                        अग्र_Op_Direct();
                    } अन्यथा {
                        d_m3RecordCallFrame(memory, function);
                        धकेलो_Backtrace_Frame_2();
                        forwardTrap(r);
                    }
                }
            } अन्यथा {
                r = m3Err_trapIndirectCallTypeMismatch;
            }
        } अन्यथा {
            r = m3Err_trapTableElementIsNull;
        }
    } अन्यथा {
        r = m3_Err_trap_Table_सूचकांक_Out_Of_Range;
    }

    अगर (M3_UNLIKELY(r)) {
        newTrap(r);
    } अन्यथा {
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
#घोषणा d_m3_पुच्छ_Call_Args(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   स_हटाओ ((व्योम *) (_sp + (RETURNSLOTS)), (स्थिर व्योम *) (_sp + (STACKOFFSET)),   \
            (माप_प्रकार) (NUMARGSLOTS) * माप (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = immediate(pc_t);
    i32     ढेर_Offset = immediate(i32);
    i32     returnSlots = immediate(i32);
    u32     अंक_Arg_Slots = immediate(u32);

    m3ret_t possible_trap = m3_Yield();
    अगर (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_पुच्छ_Call_Args(returnSlots, ढेर_Offset, अंक_Arg_Slots);

    jumpOpDirect(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = slot(IM3Function);
    IM3FuncType type = immediate(IM3FuncType);
    i32         ढेर_Offset = immediate(i32);
    i32         returnSlots = immediate(i32);
    u32         अंक_Arg_Slots = immediate(u32);

    m3ret_t     r = m3Err_none;

    अगर (M3_LIKELY(function)) {
        अगर (M3_LIKELY(type == function->funcType)) {
            अगर (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            अगर (M3_LIKELY(not r)) {
                r = m3_Yield();

                अगर (M3_LIKELY(not r)) {
                    d_m3_पुच्छ_Call_Args(returnSlots, ढेर_Offset, अंक_Arg_Slots);

                    jumpOpDirect(function->compiled);
                }
            }
        } अन्यथा {
            r = m3Err_trapIndirectCallTypeMismatch;
        }
    } अन्यथा {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(ReturnCallIndirect)
{
    M3Table*    table = immediate(M3Table*);
    u64         table_सूचकांक = d_m3WideOperand(table->isTable64);
    IM3FuncType type = immediate(IM3FuncType);
    i32         ढेर_Offset = immediate(i32);
    i32         returnSlots = immediate(i32);
    u32         अंक_Arg_Slots = immediate(u32);

    m3ret_t     r = m3Err_none;

    अगर (M3_LIKELY(table_सूचकांक < table->आकार)) {
        IM3Function function = (IM3Function)table->elements[table_सूचकांक];

        अगर (M3_LIKELY(function)) {
            अगर (M3_LIKELY(type == function->funcType)) {
                अगर (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                अगर (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    अगर (M3_LIKELY(not r)) {
                        d_m3_पुच्छ_Call_Args(returnSlots, ढेर_Offset, अंक_Arg_Slots);

                        jumpOpDirect(function->compiled);
                    }
                }
            } अन्यथा {
                r = m3Err_trapIndirectCallTypeMismatch;
            }
        } अन्यथा {
            r = m3Err_trapTableElementIsNull;
        }
    } अन्यथा {
        r = m3_Err_trap_Table_सूचकांक_Out_Of_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = immediate(IM3Function);
    ctx.userdata = immediate(व्योम*);
    u64* स्थिर sp = ((u64*)_sp);
    IM3Memory  memory = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#अगर d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    अक्षर        outbuff[1024];
    अक्षर*       outp = outbuff;
    अक्षर*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.fieldUtf8);

    स्थिर पूर्णांक nArgs = ftype->अंक_Args;
    स्थिर पूर्णांक nRets = ftype->अंक_Rets;
    u64*      args = sp + nRets;
    क्रम (पूर्णांक i = 0; i < nArgs; i++) {
        स्थिर पूर्णांक type = ftype->types[nRets + i];
        चयन (type) {
        हाल c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(args + i)); अवरोध;
        हाल c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(args + i)); अवरोध;
        हाल c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(args + i)); अवरोध;
        हाल c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(args + i)); अवरोध;
        शेष: outp += snprintf(outp, oute - outp, "<type %d>", type); अवरोध;
        }
        अगर (i < nArgs - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  अगर d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#पूर्ण_अगर

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    व्योम* ढेर_backup = runtime->ढेर;
    runtime->ढेर = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_तथ्य(ctx.function->hostMemory->mallocated));
    runtime->ढेर = ढेर_backup;

#अगर d_m3EnableStrace
    अगर (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (अक्षर*)possible_trap);
    } अन्यथा अगर (ftype and ftype->अंक_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } अन्यथा {
        d_m3TracePrint("%s", outbuff);
    }
#पूर्ण_अगर

    अगर (M3_UNLIKELY(possible_trap)) {
        _mem = memory->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        अगर (possible_trap != m3Err_pendingException) {
            धकेलो_Backtrace_Frame_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_आकार)
{
    IM3Memory memory = m3MemInfo(_mem);

    _r0 = memory->अंक_Pages;

    अग्र_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3Memory  memory = m3MemInfo(_mem);

    i32        अंक_Pages_To_Grow = (i32)_r0;
    अगर (अंक_Pages_To_Grow >= 0) {
        _r0 = (m3reg_t)memory->अंक_Pages;

        अगर (M3_LIKELY(अंक_Pages_To_Grow)) {
            u64      requiredPages = memory->अंक_Pages + (u32)अंक_Pages_To_Grow;

            M3_परिणाम r = ResizeMemory(runtime, memory, requiredPages);
            अगर (r) {
                _r0 = -1;
            }

            _mem = memory->mallocated;
        }
    } अन्यथा {
        _r0 = -1;
    }

    अग्र_Op();
}


#अगर d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3Memory  memory = m3MemInfo(_mem);

    u64        अंक_Pages_To_Grow = (u64)_r0;
    u64        अंक_Pages = memory->अंक_Pages;

    अगर (M3_LIKELY(अंक_Pages_To_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        अगर (अंक_Pages_To_Grow <= memory->maxPages - अंक_Pages and
            not ResizeMemory(runtime, memory, अंक_Pages + अंक_Pages_To_Grow)) {
            _r0 = (m3reg_t)अंक_Pages;
        } अन्यथा {
            _r0 = -1;
        }

        _mem = memory->mallocated;
    } अन्यथा {
        _r0 = (m3reg_t)अंक_Pages;
    }

    अग्र_Op();
}

#पूर्ण_अगर // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 आकार = (u32)_r0;
    u64 source = slot(u32);
    u64 destination = slot(u32);

    अगर (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        अगर (M3_LIKELY(d_m3MemRangeOk(source, आकार, _mem))) {
            u8* dst = m3_Mem_तथ्य(_mem) + destination;
            u8* src = m3_Mem_तथ्य(_mem) + source;
            स_हटाओ(dst, src, (माप_प्रकार)आकार);

            अग्र_Op();
        } अन्यथा {
            d_outOfBoundsMemOp(source, आकार);
        }
    } अन्यथा {
        d_outOfBoundsMemOp(destination, आकार);
    }
}


#अगर d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 आकार = (u64)_r0;
    u64 source = slot(u64);
    u64 destination = slot(u64);

    अगर (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        अगर (M3_LIKELY(d_m3MemRangeOk(source, आकार, _mem))) {
            u8* dst = m3_Mem_तथ्य(_mem) + destination;
            u8* src = m3_Mem_तथ्य(_mem) + source;
            स_हटाओ(dst, src, (माप_प्रकार)आकार);

            अग्र_Op();
        } अन्यथा {
            d_outOfBoundsMemOp(source, आकार);
        }
    } अन्यथा {
        d_outOfBoundsMemOp(destination, आकार);
    }
}

#पूर्ण_अगर // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(SetMemory)
{
    IM3Memory memory = immediate(IM3Memory);

    _mem = memory->mallocated;

    अग्र_Op();
}


#अगर d_m3HasMultiMemory

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

    u64             आकार = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             source = d_m3WideOperand(widths & 0x2);
    u64             destination = d_m3WideOperand(widths & 0x1);

    अगर (M3_LIKELY(d_m3MemRangeOk(destination, आकार, destMem))) {
        अगर (M3_LIKELY(d_m3MemRangeOk(source, आकार, sourceMem))) {
            स_हटाओ(m3_Mem_तथ्य(destMem) + destination, m3_Mem_तथ्य(sourceMem) + source, (माप_प्रकार)आकार);

            अग्र_Op();
        } अन्यथा {
            d_outOfBoundsMemOp(source, आकार);
        }
    } अन्यथा {
        d_outOfBoundsMemOp(destination, आकार);
    }
}

#पूर्ण_अगर // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 आकार = (u32)_r0;
    u32 byte = slot(u32);
    u64 destination = slot(u32);

    अगर (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        u8* mem8 = m3_Mem_तथ्य(_mem) + destination;
        स_रखो(mem8, (u8)byte, आकार);
        अग्र_Op();
    } अन्यथा {
        d_outOfBoundsMemOp(destination, आकार);
    }
}


#अगर d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 आकार = (u64)_r0;
    u32 byte = slot(u32);
    u64 destination = slot(u64);

    अगर (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        u8* mem8 = m3_Mem_तथ्य(_mem) + destination;
        स_रखो(mem8, (u8)byte, (माप_प्रकार)आकार);
        अग्र_Op();
    } अन्यथा {
        d_outOfBoundsMemOp(destination, आकार);
    }
}

#पूर्ण_अगर // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_तथ्य_Segment* segment = immediate(M3_तथ्य_Segment*);

    u64            आकार = (u32)_r0;
    u64            source = slot(u32);
    u64            destination = slot(u32);

    u64            available = segment->dropped ? 0 : segment->आकार;

    अगर (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        अगर (M3_LIKELY(source <= available and आकार <= available - source)) {
            स_नकल(m3_Mem_तथ्य(_mem) + destination, segment->तथ्य + source, (माप_प्रकार)आकार);
            अग्र_Op();
        } अन्यथा {
            d_outOfBoundsMemOp(source, आकार);
        }
    } अन्यथा {
        d_outOfBoundsMemOp(destination, आकार);
    }
}


#अगर d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_तथ्य_Segment* segment = immediate(M3_तथ्य_Segment*);

    u64            आकार = (u32)_r0;
    u64            source = slot(u32);
    u64            destination = slot(u64);

    u64            available = segment->dropped ? 0 : segment->आकार;

    अगर (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        अगर (M3_LIKELY(source <= available and आकार <= available - source)) {
            स_नकल(m3_Mem_तथ्य(_mem) + destination, segment->तथ्य + source, (माप_प्रकार)आकार);
            अग्र_Op();
        } अन्यथा {
            d_outOfBoundsMemOp(source, आकार);
        }
    } अन्यथा {
        d_outOfBoundsMemOp(destination, आकार);
    }
}

#पूर्ण_अगर // d_m3HasMemory64


d_m3Op(तथ्य_Drop)
{
    M3_तथ्य_Segment* segment = immediate(M3_तथ्य_Segment*);
    segment->dropped = true;

    अग्र_Op();
}


#अगर d_m3HasRefTypes

d_m3Op(TableGet)
{
    M3Table* table = immediate(M3Table*);
    u64      index = d_m3WideOperand(table->isTable64);

    अगर (M3_LIKELY(index < table->आकार)) {
        _r0 = (u64)(uintptr_t)table->elements[index];
        अग्र_Op();
    } अन्यथा {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(TableSet)
{
    M3Table* table = immediate(M3Table*);
    व्योम*    मूल्य = slot(व्योम*);
    u64      index = d_m3WideOperand(table->isTable64);

    अगर (M3_LIKELY(index < table->आकार)) {
        table->elements[index] = मूल्य;
        अग्र_Op();
    } अन्यथा {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(Table_आकार)
{
    M3Table* table = immediate(M3Table*);

    _r0 = table->आकार;

    अग्र_Op();
}


d_m3Op(TableGrow)
{
    M3Table* table = immediate(M3Table*);
    u64      delta = d_m3WideOperand(table->isTable64);
    व्योम*    मूल्य = slot(व्योम*);

    u32      old_आकार = table->आकार;
    u32      max_आकार = table->max_आकार ? table->max_आकार : d_m3_Max_Sane_Table_आकार;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_आकार = (delta <= max_आकार - old_आकार) ? (u64)old_आकार + delta
                                                    : (u64)max_आकार + 1;

    अगर (new_आकार == old_आकार)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_आकार;
        अग्र_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    अगर (new_आकार <= max_आकार and (not runtime->tableElementsLimit or
                                delta <= runtime->tableElementsLimit - runtime->tableElementsUsed)) {
        व्योम** elements = m3_ReallocArray(व्योम*, table->elements, (माप_प्रकार)new_आकार, old_आकार);

        अगर (elements) {
            runtime->tableElementsUsed += new_आकार - old_आकार;
            table->elements = elements;
            table->आकार = (u32)new_आकार;

            क्रम (u32 i = old_आकार; i < table->आकार; ++i) {
                table->elements[i] = मूल्य;
            }

            _r0 = old_आकार;
            अग्र_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    अग्र_Op();
}


d_m3Op(TableInit)
{
    M3Table*          table = immediate(M3Table*);
    M3ElementSegment* segment = immediate(M3ElementSegment*);
    u32               गिनती = slot(u32);
    u32               source = slot(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3WideOperand(table->isTable64);

    u64               available = segment->dropped ? 0 : segment->अंक_Elements;

    अगर (M3_LIKELY(destination <= table->आकार and गिनती <= table->आकार - destination and
                  (u64) source + गिनती <= available)) {
        क्रम (u32 i = 0; i < गिनती; ++i) {
            table->elements[destination + i] = segment->resolved[source + i];
        }

        अग्र_Op();
    } अन्यथा {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = immediate(M3ElementSegment*);
    segment->dropped = true;

    अग्र_Op();
}


d_m3Op(TableCopy)
{
    M3Table* dst = immediate(M3Table*);
    M3Table* src = immediate(M3Table*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      गिनती = d_m3WideOperand(dst->isTable64 and src->isTable64);
    u64      source = d_m3WideOperand(src->isTable64);
    u64      destination = d_m3WideOperand(dst->isTable64);

    अगर (M3_LIKELY(destination <= dst->आकार and गिनती <= dst->आकार - destination and
                  source <= src->आकार and गिनती <= src->आकार - source)) {
        स_हटाओ(dst->elements + destination, src->elements + source, (माप_प्रकार)गिनती * माप(व्योम*));

        अग्र_Op();
    } अन्यथा {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}


d_m3Op(TableFill)
{
    M3Table* table = immediate(M3Table*);
    u64      गिनती = d_m3WideOperand(table->isTable64);
    व्योम*    मूल्य = slot(व्योम*);
    u64      index = d_m3WideOperand(table->isTable64);

    अगर (M3_LIKELY(index <= table->आकार and गिनती <= table->आकार - index)) {
        क्रम (u64 i = 0; i < गिनती; ++i) {
            table->elements[index + i] = मूल्य;
        }

        अग्र_Op();
    } अन्यथा {
        newTrap(m3Err_trapTableOutOfBounds);
    }
}

#पूर्ण_अगर // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = immediate(IM3Function);

    m3ret_t     परिणाम = m3Err_none;

    अगर (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        परिणाम = CompileFunction(function);
    }

    अगर (not परिणाम) {
        // patch up compiled pc and call rewritten op_Call
        *((व्योम**)--_pc) = (व्योम*)(function->compiled);
        --_pc;
        अग्र_Op_Direct();
    }

    newTrap(परिणाम);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = immediate(IM3Function);

    m3ret_t     परिणाम = m3Err_none;

    अगर (M3_UNLIKELY(not function->compiled)) {
        परिणाम = CompileFunction(function);
    }

    अगर (not परिणाम) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((व्योम**)--_pc) = (व्योम*)(function->compiled);
        --_pc;
        अग्र_Op_Direct();
    }

    newTrap(परिणाम);
}


#अगर d_m3_Has_ढेर_Switching
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
अटल
व्योम SuspendWithoutTag (IM3Runtime io_runtime, IM3Continuation io_cont, M3SafePointKind i_kind,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  अगर d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspendTag = शून्य;
    io_runtime->suspendHandlerCont = शून्य;
    io_runtime->suspendStubPC = शून्य;
    io_runtime->switchTarget = शून्य;
    io_runtime->अंक_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspendPoint = (u8)i_kind;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  अगर d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->अंक_Suspend_Results = 0;
}
#पूर्ण_अगर // d_m3HasStackSwitching


#अगर d_m3HasGasMetering

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

    अगर (M3_UNLIKELY(runtime->gasRemaining < 0)) {
#  अगर d_m3_Has_ढेर_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        अगर (runtime->isSuspendable and runtime->activeContinuation) {
            runtime->suspendRequested = true;
            अग्र_Op();
        }
#  endif
        newTrap(m3Err_trapOutOfGas);
    }

    अग्र_Op();
}

#पूर्ण_अगर // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
अटल अंतरभूत
bool EntryFits (IM3Function i_function, m3stack_t _sp, M3MemoryHeader* _mem)
{
#अगर d_m3_Skip_ढेर_Check
    (व्योम)i_function;
    (व्योम)_sp;
    (व्योम)_mem;
    वापस true;
#या_अगर d_m3_Has_ढेर_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->activeContinuation : शून्य);
    व्योम*           max_ढेर = (_act && _act->val_ढेर)
                                 ? (व्योम*)(_act->val_ढेर + _act->अंक_ढेर_Slots)
                                 : (_mem ? _mem->max_ढेर : (व्योम*)~(uintptr_t)0);
    वापस M3_LIKELY((व्योम*)(_sp + i_function->max_ढेर_Slots) < max_ढेर);
#अन्यथा
    वापस M3_LIKELY((व्योम*)(_sp + i_function->max_ढेर_Slots) < _mem->max_ढेर);
#पूर्ण_अगर
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
अटल अंतरभूत
व्योम EnterFrame (IM3Function i_function, m3stack_t _sp)
{
#अगर defined(DEBUG)
    i_function->hits++;
#पूर्ण_अगर
    u8* ढेर = (u8*)((m3slot_t*)_sp + i_function->अंक_Ret_And_Arg_Slots);

    स_रखो(ढेर, 0x0, i_function->अंक_Local_Bytes);
    ढेर += i_function->अंक_Local_Bytes;

    अगर (i_function->constants) {
        स_नकल(ढेर, i_function->constants, i_function->अंक_Constant_Bytes);
    }
}


d_m3Op(Entry)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = immediate(IM3Function);
#अगर d_m3EntryKeepsFrame
    IM3Memory memory = m3MemInfo(_mem);
#पूर्ण_अगर

    अगर (EntryFits(function, _sp, _mem)) {
        EnterFrame(function, _sp);

#अगर !d_m3EntryKeepsFrame
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        अग्र_Op_Direct();
#अन्यथा

#  अगर d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->अंक_Ret_Slots));
        trace_rt->callDepth++;
#  endif

        m3ret_t r = अग्र_Op_Impl();

#  अगर d_m3EnableStrace >= 2
        trace_rt->callDepth--;

        अगर (r) {
            d_m3TracePrint("} !trap = %s", (अक्षर*)r);
        } अन्यथा अगर (Get_Function_अंक_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } अन्यथा {
            d_m3TracePrint("}");
        }
#  endif

        अगर (M3_UNLIKELY(r)) {
            _mem = memory->mallocated;
#  अगर d_m3_Has_ढेर_Switching
            अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                वापस RecordEntryFrame(m3MemRuntime(_mem), _pc, _sp, memory, function);
            }
#  endif
            fillBacktraceFrame();
        }
        forwardTrap(r);
#पूर्ण_अगर // d_m3EntryKeepsFrame
    } अन्यथा {
        newTrap(m3_Err_trap_ढेर_Overflow);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_ढेर();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3Memory memory = m3MemInfo(_mem);

    करो {
#अगर d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->callDepth++;
#पूर्ण_अगर
        r = अग्र_Op_Impl();

#अगर d_m3EnableStrace >= 3
        trace_rt->callDepth--;
        d_m3TracePrint("}");
#पूर्ण_अगर
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = memory->mallocated;
    } जबतक (r == _pc);

#अगर d_m3_Has_ढेर_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        वापस RecordLoopFrame(m3MemRuntime(_mem), _pc, _sp, memory);
    }
#पूर्ण_अगर

    forwardTrap(r);
}


#अगर d_m3HasExceptionHandling

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
    d_m3_Check_Native_ढेर();

    IM3Memory  memory = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        अंक_Clauses = immediate(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * अंक_Clauses;

    u32        depth = ++runtime->tryDepth;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = memory->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->tryDepth == depth);
    runtime->tryDepth = depth - 1;

#  अगर d_m3_Has_ढेर_Switching
    अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        वापस RecordTryFrame(runtime, clauses, _sp, memory, अंक_Clauses, isInnermost);
    }
#  endif

    अगर (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        स्थिर M3Exception* exception = runtime->pendingException;

        क्रम (u32 i = 0; i < अंक_Clauses; ++i) {
            IM3Tag tag = (IM3Tag)clauses[2 * i];

            अगर (tag == शून्य or tag == exception->tag) {
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

    u32          अंक_Args = immediate(u32);

    क्रम (u32 i = 0; i < अंक_Args; ++i) {
        i32 offset = immediate(i32);
        u32 is64 = immediate(u32);
        u64 मूल्य = exception->args[i];

        अगर (is64) {
            *(u64*)(_sp + offset) = मूल्य;
        } अन्यथा {
            *(u32*)(_sp + offset) = (u32)मूल्य;
        }
    }

    i32 refOffset = immediate(i32);

    अगर (refOffset >= 0) {
        *(M3Exception**)(_sp + refOffset) = exception;
        exception->reified = true;
    } अन्यथा अगर (not exception->reified) {
        FreeException(runtime, exception);
    }

    अग्र_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3Tag       tag = immediate(IM3Tag);
    u32          अंक_Args = immediate(u32);

    M3Exception* exception = NewException(runtime, tag, अंक_Args);

    अगर (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    क्रम (u32 i = 0; i < अंक_Args; ++i) {
        i32 offset = immediate(i32);
        u32 is64 = immediate(u32);

        exception->args[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    वापस m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = slot(M3Exception*);

    अगर (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    वापस m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(निकालो_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        गिनती = immediate(u32);

    runtime->tryDepth -= गिनती;

    अग्र_Op();
}

#पूर्ण_अगर // d_m3HasExceptionHandling

#अगर d_m3_Has_ढेर_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = immediate(IM3FuncType);
    i32         dstSlot = immediate(i32);
    i32         funcSlot = immediate(i32);

    IM3Function func = *(IM3Function*)(_sp + funcSlot);
    अगर (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    अगर (M3_UNLIKELY(not Continuation_Can_Acquire_ढेर(runtime))) {
        newTrap(m3Err_continuationLimitExceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    अगर (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_परिणाम allocation = Continuation_Acquire_ढेर(runtime, cont);
    अगर (M3_UNLIKELY(allocation)) {
        cont->state = cont_consumed;
        newTrap(allocation);
    }
    *(IM3Continuation*)(_sp + dstSlot) = cont;
    अग्र_Op();
}


अटल
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = immediate(IM3FuncType);
    IM3FuncType     ct2 = immediate(IM3FuncType);
    i32             dstSlot = immediate(i32);
    i32             contSlot = immediate(i32);
    u32             अंक_Bound = immediate(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + contSlot);
    अगर (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    अगर (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->entryFunction);
    अगर (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->entryFunction = srcCont->entryFunction;
    dstCont->val_ढेर = srcCont->val_ढेर;
    dstCont->अंक_ढेर_Slots = srcCont->अंक_ढेर_Slots;
    dstCont->frames = srcCont->frames;
    dstCont->framesCap = srcCont->framesCap;
    dstCont->अंक_Frames = srcCont->अंक_Frames;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspendPoint = srcCont->suspendPoint;
    dstCont->r0 = srcCont->r0;
#  अगर d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_Args_गिनती = srcCont->bound_Args_गिनती;
    dstCont->state = srcCont->state;
#  अगर d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = शून्य;
#  endif

    dstCont->अंक_Suspend_Results = srcCont->अंक_Suspend_Results;
    क्रम (u32 j = 0; j < srcCont->अंक_Suspend_Results; ++j) {
        dstCont->suspend_परिणाम_Offsets[j] = srcCont->suspend_परिणाम_Offsets[j];
        dstCont->suspend_परिणाम_Is64[j] = srcCont->suspend_परिणाम_Is64[j];
    }

    अगर (dstCont->state == cont_allocated) {
        u16 अंक_Rets = ct1->contFuncType ? Get_Func_Type_अंक_Results(ct1->contFuncType) : 0;
        क्रम (u32 i = 0; i < अंक_Bound; ++i) {
            i32 argSlot = immediate(i32);
            u32 is64 = immediate(u32);
            u32 targetIdx = (अंक_Rets + dstCont->bound_Args_गिनती) * c_io_Slot_गिनती;
            अगर (is64) {
                *(u64*)(dstCont->val_ढेर + targetIdx) = *(u64*)(_sp + argSlot);
            } अन्यथा {
                *(u32*)(dstCont->val_ढेर + targetIdx) = *(u32*)(_sp + argSlot);
            }
            dstCont->bound_Args_गिनती++;
        }
    } अन्यथा {
        IM3Continuation inner = InnermostSuspended(dstCont);
        क्रम (u32 i = 0; i < अंक_Bound; ++i) {
            i32 argSlot = immediate(i32);
            u32 is64 = immediate(u32);
            u32 dstIdx = dstCont->bound_Args_गिनती;
            अगर (dstIdx < inner->अंक_Suspend_Results) {
                i32 dstOffset = inner->suspend_परिणाम_Offsets[dstIdx];
                अगर (is64) {
                    *(u64*)(inner->sp + dstOffset) = *(u64*)(_sp + argSlot);
                } अन्यथा {
                    *(u32*)(inner->sp + dstOffset) = *(u32*)(_sp + argSlot);
                }
            }
            dstCont->bound_Args_गिनती++;
        }
    }

    srcCont->val_ढेर = शून्य;
    srcCont->frames = शून्य;
    srcCont->framesCap = 0;
    srcCont->अंक_Frames = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dstSlot) = dstCont;
    अग्र_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
अटल
IM3Continuation FindHandler (IM3Continuation i_cont, IM3Tag i_tag, u8 i_kind, pc_t* o_stubPC)
{
    IM3Tag queryTag = (i_tag and i_tag->resolved) ? i_tag->resolved : i_tag;

    क्रम (IM3Continuation cont = i_cont; cont; cont = cont->parent) {
        क्रम (u32 h = 0; h < cont->अंक_Handlers; ++h) {
            pc_t   entry = cont->handlersPC + (h * 3);
            IM3Tag handlerTag = (IM3Tag)(*(entry + 1));
            अगर (handlerTag and handlerTag->resolved) {
                handlerTag = handlerTag->resolved;
            }

            अगर ((u32)(uintptr_t)(*entry) == i_kind and handlerTag == queryTag) {
                अगर (o_stubPC) {
                    *o_stubPC = (pc_t)(*(entry + 2));
                }
                वापस cont;
            }
        }
    }

    वापस शून्य;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
अटल
व्योम ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    क्रम (u32 i = 0; i < io_cont->अंक_Suspend_Results; ++i) {
        अगर (io_cont->suspend_परिणाम_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_परिणाम_Offsets[i]) = 0;
        } अन्यथा {
            *(m3slot_t*)(i_sp + io_cont->suspend_परिणाम_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->activeContinuation;
    अगर (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    IM3Tag tag = immediate(IM3Tag);
    u32    अंक_Params = immediate(u32);

    runtime->अंक_Suspend_Payload = अंक_Params;
    क्रम (u32 i = 0; i < अंक_Params; ++i) {
        i32 paramSlot = immediate(i32);
        u32 is64 = immediate(u32);
        अगर (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + paramSlot);
        } अन्यथा {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + paramSlot);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 अंक_Results = immediate(u32);
    cont->अंक_Suspend_Results = अंक_Results;
    क्रम (u32 i = 0; i < अंक_Results; ++i) {
        cont->suspend_परिणाम_Offsets[i] = immediate(i32);
        cont->suspend_परिणाम_Is64[i] = immediate(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = शून्य;
    IM3Continuation खोज_Cont = FindHandler(cont, tag, 0, &matchedStubPC);

    अगर (M3_UNLIKELY(not खोज_Cont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = खोज_Cont;
    runtime->switchTarget = शून्य;
    runtime->suspendTag = tag;

    cont->pc = _pc;
    cont->suspendPoint = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  अगर d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_Args_गिनती = 0;

    वापस m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
अटल
m3ret_t RunUnderHandlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_अंक_Handlers, M3MemoryHeader** io_mem);
अटल
व्योम TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_अंक_Results);
अटल
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3MemoryHeader* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
अटल अंतरभूत
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    वापस pass and (i_cont->suspendPoint == safepoint_op or i_cont->suspendPoint == safepoint_entry);
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
अटल
m3ret_t ReplayFrames (IM3Continuation i_cont, i32 i_depth, M3MemoryHeader* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  अगर d_m3_Max_Native_ढेर > 0
    {
        व्योम* limit = runtime->ढेर_Limit;

        अगर (M3_UNLIKELY(limit and m3_Native_ढेर_Ptr() < limit)) {
            वापस m3_Err_trap_ढेर_Overflow;
        }
    }
#  endif

    अगर (i_depth < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  अगर d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        अगर (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = शून्य;

            वापस m3Err_pendingException;
        }
#  endif
        अगर (pastCheck) {
            वापस ResumePastCheck(i_cont, _mem);
        }

        वापस d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3Frame frame = i_cont->frames[i_depth];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3FrameKind)frame.kind == frame_resume);

#  अगर d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3FrameKind)frame.kind == frame_try);
#  endif

    अगर (not ownDescent) {
        r = ReplayFrames(i_cont, i_depth - 1, _mem);
        _mem = frame.memory->mallocated;
    }

    चयन ((M3FrameKind)frame.kind) {
    हाल frame_call:
        अगर (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            वापस d_m3CallWithRegs(frame.pc, frame.sp, _mem, frame.call.r0, frame.call.fp0);
        }

        अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            वापस Continuation_RecordFrame(runtime, &frame);
        }
#  अगर d_m3RecordBacktraces
        धकेलो_Backtrace_Frame(runtime, frame.pc - 1);
#  endif
        वापस r;

    हाल frame_loop: {
        m3stack_t _sp = frame.sp;
        m3reg_t   _r0 = 0;
#  अगर d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        जबतक (r == (m3ret_t)frame.pc) {
            r = jumpOpImpl(frame.pc);
            _mem = frame.memory->mallocated;
        }

        अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            वापस Continuation_RecordFrame(runtime, &frame);
        }
        वापस r;
    }

#  अगर d_m3HasExceptionHandling
    हाल frame_try: {
        m3stack_t _sp = frame.sp;
        m3reg_t   _r0 = 0;
#    अगर d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (व्योम)_sp;
        (व्योम)_r0;
#    अगर d_m3HasFloat
        (व्योम)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 depth = ++runtime->tryDepth;

        अगर (not frame.try_.handlersLive) {
            runtime->tryDepth = depth - 1;
        }

        r = ReplayFrames(i_cont, i_depth - 1, _mem);
        _mem = frame.memory->mallocated;

        bool isInnermost = (runtime->tryDepth == depth);
        runtime->tryDepth = depth - 1;

        अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            frame.try_.handlersLive = isInnermost;
            वापस Continuation_RecordFrame(runtime, &frame);
        }

        अगर (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            स्थिर M3Exception* exception = runtime->pendingException;

            क्रम (u32 i = 0; i < frame.try_.अंक_Clauses; ++i) {
                IM3Tag tag = (IM3Tag)frame.pc[2 * i];

                अगर (tag == शून्य or tag == exception->tag) {
                    pc_t handler = (pc_t)frame.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    वापस jumpOpImpl(handler);
                }
            }
        }
        वापस r;
    }
#  endif

    हाल frame_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = frame.resume.cont;

        r = RunUnderHandlers(runtime, &inner, frame.resume.handlersPC,
                             frame.resume.अंक_Handlers, &_mem);

        अगर (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, frame.sp, frame.resume.resultsPC,
                                    frame.resume.अंक_Results);
            Continuation_Recycle_ढेर(runtime, inner);

            वापस d_m3CallWithRegs(frame.pc, frame.sp, _mem, 0, 0.);
        }

        अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            अगर (runtime->suspendHandlerCont != inner) {
                frame.resume.cont = inner;
                वापस Continuation_RecordFrame(runtime, &frame);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            अगर (M3_UNLIKELY(not suspended)) {
                वापस m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            अगर (runtime->suspendStubPC) {
                वापस d_m3CallWithRegs(runtime->suspendStubPC, frame.sp, _mem, 0, 0.);
            } अन्यथा {
                वापस m3Err_trapUnhandledControlTag;
            }
        } अन्यथा {
            inner->state = cont_consumed;
            Continuation_Recycle_ढेर(runtime, inner);
        }

        वापस r;
    }

#  अगर d_m3EntryKeepsFrame
    हाल frame_entry:
        अगर (M3_UNLIKELY(r)) {
            अगर (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                वापस Continuation_RecordFrame(runtime, &frame);
            }
#    अगर d_m3RecordBacktraces
            FillBacktraceFunctionInfo(runtime, frame.entry.function);
#    अन्यथा
            (व्योम)frame.entry.function;
#    endif
        }
        वापस r;
#  endif
    }

    वापस r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
अटल
व्योम* ContinuationArgDest (IM3Continuation i_cont, u32 i_सूचकांक)
{
    अगर (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : शून्य;
        u16         अंक_Rets = inner ? Get_Func_Type_अंक_Results(inner) : 0;

        वापस i_cont->val_ढेर + (अंक_Rets + i_सूचकांक) * c_io_Slot_गिनती;
    }

    अगर (i_सूचकांक < i_cont->अंक_Suspend_Results) {
        वापस i_cont->sp + i_cont->suspend_परिणाम_Offsets[i_सूचकांक];
    }

    वापस शून्य;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
अटल
व्योम HandOverToPeer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 अंक_Args = i_runtime->अंक_Suspend_Payload;
    u32 base = (io_peer->state == cont_allocated) ? io_peer->bound_Args_गिनती : 0;

    क्रम (u32 i = 0; i < अंक_Args; ++i) {
        व्योम* dest = ContinuationArgDest(io_peer, base + i);

        अगर (dest) {
            अगर (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } अन्यथा {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    व्योम* refDest = ContinuationArgDest(io_peer, base + अंक_Args);

    अगर (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    अगर (io_peer->state == cont_allocated) {
        io_peer->bound_Args_गिनती += अंक_Args + 1;
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
अटल
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    जबतक (i_cont->अंक_Frames and (M3FrameKind) i_cont->frames[0].kind == frame_resume) {
        i_cont = i_cont->frames[0].resume.cont;
    }

    वापस i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
अटल
m3ret_t RunUnderHandlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_अंक_Handlers, M3MemoryHeader** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation parent = i_runtime->activeContinuation;
    IM3Memory       memory = m3MemInfo(*io_mem);
    m3ret_t         r;

    क्रम (;;) {
#  अगर d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        अगर (M3_UNLIKELY(cont->resumeThrow and cont->अंक_Frames == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = शून्य;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            अवरोध;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        अगर (isStarting) {
            अगर (M3_UNLIKELY(not cont->entryFunction->compiled)) {
                r = CompileFunction(cont->entryFunction);

                अगर (M3_UNLIKELY(r)) {
                    अवरोध;
                }
            }
            cont->pc = cont->entryFunction->compiled;
            cont->sp = cont->val_ढेर;
            cont->अंक_Frames = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->अंक_Handlers = i_अंक_Handlers;
        cont->parent = parent;
        cont->state = cont_running;
        cont->bound_Args_गिनती = 0;

        i_runtime->activeContinuation = cont;

        अगर (cont->अंक_Frames) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->अंक_Frames - 1;
            cont->अंक_Frames = 0;

            r = ReplayFrames(cont, outermost, *io_mem);
        } अन्यथा अगर (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } अन्यथा {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = memory->mallocated;
        i_runtime->activeContinuation = parent;

        अगर (r != m3Err_continuationSuspended) {
            अवरोध;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        अगर (not i_runtime->switchTarget or i_runtime->suspendHandlerCont != cont) {
            अवरोध;
        }

        IM3Continuation peer = i_runtime->switchTarget;
        i_runtime->switchTarget = शून्य;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        अगर (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            अवरोध;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_ContTypeOfRef(peer->entryFunction->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_अंक_Params(peerType) - 1)));
        HandOverToPeer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    वापस r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
अटल
व्योम TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_अंक_Results)
{
    क्रम (u32 i = 0; i < i_अंक_Results; ++i) {
        i32 resSlot = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_Slot_गिनती;

        अगर (is64) {
            *(u64*)(i_sp + resSlot) = *(u64*)(i_cont->val_ढेर + srcIdx);
        } अन्यथा {
            *(u32*)(i_sp + resSlot) = *(u32*)(i_cont->val_ढेर + srcIdx);
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
    अगर (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    अगर (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         अंक_Rets = Get_Func_Type_अंक_Results(innerType);

    अगर (mode == d_m3ResumeThrowRef) {
        i32 exnSlot = immediate(i32);

#  अगर d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exnSlot);

        अगर (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  अन्यथा
        (व्योम)exnSlot;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } अन्यथा अगर (mode == d_m3ResumeThrow) {
        IM3Tag exnTag = immediate(IM3Tag);
        u32    अंक_Args = immediate(u32);

#  अगर d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exnTag, अंक_Args);

        अगर (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        क्रम (u32 i = 0; i < अंक_Args; ++i) {
            i32 offset = immediate(i32);
            u32 is64 = immediate(u32);

            exception->args[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  अन्यथा
        (व्योम)exnTag;
        (व्योम)अंक_Args;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } अन्यथा {
        u32 अंक_Args = immediate(u32);

        अगर (cont->state == cont_allocated) {
            क्रम (u32 i = 0; i < अंक_Args; ++i) {
                i32 argSlot = immediate(i32);
                u32 is64 = immediate(u32);
                u32 targetIdx = (अंक_Rets + cont->bound_Args_गिनती + i) * c_io_Slot_गिनती;
                अगर (is64) {
                    *(u64*)(cont->val_ढेर + targetIdx) = *(u64*)(_sp + argSlot);
                } अन्यथा {
                    *(u32*)(cont->val_ढेर + targetIdx) = *(u32*)(_sp + argSlot);
                }
            }
            cont->bound_Args_गिनती = 0;
        } अन्यथा {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            क्रम (u32 i = 0; i < अंक_Args; ++i) {
                i32 argSlot = immediate(i32);
                u32 is64 = immediate(u32);
                u32 dstIdx = cont->bound_Args_गिनती + i;
                अगर (dstIdx < inner->अंक_Suspend_Results) {
                    i32 dstOffset = inner->suspend_परिणाम_Offsets[dstIdx];
                    अगर (is64) {
                        *(u64*)(inner->sp + dstOffset) = *(u64*)(_sp + argSlot);
                    } अन्यथा {
                        *(u32*)(inner->sp + dstOffset) = *(u32*)(_sp + argSlot);
                    }
                }
            }
        }
    }

    u32  अंक_Handlers = immediate(u32);
    pc_t handlersPC = _pc;
    _pc += अंक_Handlers * 3;

    u32       अंक_Results = immediate(u32);
    pc_t      resultsPC = _pc;

    IM3Memory memory = m3MemInfo(_mem);

    m3ret_t   r = RunUnderHandlers(runtime, &cont, handlersPC, अंक_Handlers, &_mem);

    अगर (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, अंक_Results);
        Continuation_Recycle_ढेर(runtime, cont);

        _pc += अंक_Results * 2;
        अग्र_Op();
    } अन्यथा अगर (r == m3Err_continuationSuspended) {
        अगर (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            अगर (M3_UNLIKELY(not runtime->activeContinuation)) {
                newTrap(m3Err_trapUnhandledControlTag);
            }
            M3Frame frame;
            frame.kind = frame_resume;
            frame.pc = resultsPC + अंक_Results * 2;
            frame.sp = _sp;
            frame.memory = memory;
            frame.resume.cont = cont;
            frame.resume.handlersPC = handlersPC;
            frame.resume.अंक_Handlers = अंक_Handlers;
            frame.resume.resultsPC = resultsPC;
            frame.resume.अंक_Results = अंक_Results;

            वापस Continuation_RecordFrame(runtime, &frame);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        अगर (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        अगर (runtime->suspendStubPC) {
            jumpOpDirect(runtime->suspendStubPC);
        } अन्यथा {
            newTrap(m3Err_trapUnhandledControlTag);
        }
    } अन्यथा {
        cont->state = cont_consumed;
        Continuation_Recycle_ढेर(runtime, cont);
        धकेलो_Backtrace_Frame_2();
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

    u32 अंक_Args = immediate(u32);
    क्रम (u32 i = 0; i < अंक_Args; ++i) {
        i32 offset = immediate(i32);
        u32 is64 = immediate(u32);
        u64 val = (i < runtime->अंक_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        अगर (is64) {
            *(u64*)(_sp + offset) = val;
        } अन्यथा {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 contOffset = immediate(i32);
    अगर (contOffset >= 0) {
        *(IM3Continuation*)(_sp + contOffset) = cont;
    }

    अग्र_Op();
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

    अगर (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    skip_immediate(IM3FuncType);

    IM3Tag          tag = immediate(IM3Tag);
    i32             targetSlot = immediate(i32);

    IM3Continuation target = *(IM3Continuation*)(_sp + targetSlot);

    अगर (M3_UNLIKELY(not target)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    अगर (M3_UNLIKELY(target->state == cont_consumed or target->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = FindHandler(cont, tag, 1, शून्य);

    अगर (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3Err_trapUnhandledControlTag);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 अंक_Args = immediate(u32);

    runtime->अंक_Suspend_Payload = अंक_Args;

    क्रम (u32 i = 0; i < अंक_Args; ++i) {
        i32 argSlot = immediate(i32);
        u32 is64 = immediate(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + argSlot) : (u64) * (u32*)(_sp + argSlot);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 अंक_Results = immediate(u32);

    cont->अंक_Suspend_Results = अंक_Results;

    क्रम (u32 i = 0; i < अंक_Results; ++i) {
        cont->suspend_परिणाम_Offsets[i] = immediate(i32);
        cont->suspend_परिणाम_Is64[i] = immediate(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = शून्य;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switchTarget = target;
    runtime->suspendTag = tag;

    cont->pc = _pc;
    cont->suspendPoint = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  अगर d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    वापस m3Err_continuationSuspended;
}


#पूर्ण_अगर // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = immediate(pc_t);

    अगर (condition) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = slot(i32);

    pc_t elsePC = immediate(pc_t);

    अगर (condition) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}


d_m3Op(BranchTable)
{
    u32   branch_सूचकांक = slot(u32);           // branch index is always in a slot
    u32   अंक_Targets = immediate(u32);

    pc_t* branches = (pc_t*)_pc;

    अगर (branch_सूचकांक > अंक_Targets) {
        branch_सूचकांक = अंक_Targets; // the default index
    }

    jumpOp(branches[branch_सूचकांक]);
}


#घोषणा d_m3SetRegisterSetSlot(TYPE, REG) \
d_m3Op  (SetRegister_##TYPE)            \
{                                       \
    TYPE * src = slot_ptr (TYPE);       \
    d_m3_Preload_अग्र ();                 \
    REG = * src;                        \
    अग्र_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (SetSlot_##TYPE)                 \
{                                       \
    TYPE * dst = slot_ptr (TYPE);       \
    d_m3_Preload_अग्र ();                 \
    * dst = (TYPE) REG;                 \
    अग्र_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (PreserveSetSlot_##TYPE)         \
{                                       \
    TYPE * ढेर     = slot_ptr (TYPE); \
    TYPE * preserve  = slot_ptr (TYPE); \
    d_m3_Preload_अग्र ();                 \
                                        \
    * preserve = * ढेर;               \
    * ढेर = (TYPE) REG;               \
                                        \
    अग्र_Op_Preloaded ();                 \
}

d_m3SetRegisterSetSlot(i32, _r0)
d_m3SetRegisterSetSlot(i64, _r0)
#अगर d_m3HasFloat
d_m3SetRegisterSetSlot(f32, _fp0)
d_m3SetRegisterSetSlot(f64, _fp0)
#पूर्ण_अगर

d_m3Op(CopySlot_32)
{
    u32* dst = slot_ptr(u32);
    u32* src = slot_ptr(u32);
    d_m3_Preload_अग्र();

    *dst = *src;

    अग्र_Op_Preloaded();
}


d_m3Op(PreserveCopySlot_32)
{
    u32* dest = slot_ptr(u32);
    u32* src = slot_ptr(u32);
    u32* preserve = slot_ptr(u32);
    d_m3_Preload_अग्र();

    *preserve = *dest;
    *dest = *src;

    अग्र_Op_Preloaded();
}


d_m3Op(CopySlot_64)
{
    u64* dst = slot_ptr(u64);
    u64* src = slot_ptr(u64);
    d_m3_Preload_अग्र();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    अग्र_Op_Preloaded();
}


d_m3Op(PreserveCopySlot_64)
{
    u64* dest = slot_ptr(u64);
    u64* src = slot_ptr(u64);
    u64* preserve = slot_ptr(u64);
    d_m3_Preload_अग्र();

    *preserve = *dest;
    *dest = *src;

    अग्र_Op_Preloaded();
}


#अगर d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_ढेर)
{
    u32         opcode_सूचकांक = immediate(u32);
    u32         ढेर_Height = immediate(u32);
    IM3Function function = immediate(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    म_लिखो(" %4d ", opcode_सूचकांक);
    म_लिखो(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  अगर d_m3HasFloat
    म_लिखो("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    क्रम (u32 i = 0; i < ढेर_Height; ++i) {
        cstr_t kind = "";

        म_लिखो("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, kind, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    म_लिखो("---------------------------------------------------------------------------------------------------------\n");

    अग्र_Op_Direct();
}
#पूर्ण_अगर


#घोषणा d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    अग्र_Op ();                                  \
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
    अग्र_Op ();                                  \
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
    अग्र_Op ();                                  \
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
    अग्र_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#घोषणा d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = slot (TYPE);                \
    TYPE operand1 = slot (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    अग्र_Op ();                                  \
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
    अग्र_Op ();                                  \
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
    अग्र_Op ();                                  \
}

#अगर d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, slot(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, slot(i32))
#पूर्ण_अगर

d_m3Op(Return)
{
    m3_ढेर_Check();
    वापस m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = immediate(pc_t);

    अगर (condition) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = slot(i32);
    pc_t branch = immediate(pc_t);

    अगर (condition) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}

#अगर d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t branch  = immediate (pc_t);                        \
      अगर (operand OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      अन्यथा अग्र_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t branch  = immediate (pc_t);                        \
      अगर (((TYPE) _r0) OPER operand)                          \
          jumpOp (branch);                                    \
      अन्यथा अग्र_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = slot (TYPE);                            \
      TYPE operand1 = slot (TYPE);                            \
      pc_t branch   = immediate (pc_t);                       \
      अगर (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      अन्यथा अग्र_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t elsePC  = immediate (pc_t);                        \
      अगर (operand OPER ((TYPE) _r0))                          \
          अग्र_Op ();                                          \
      अन्यथा jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE operand = slot (TYPE);                             \
      pc_t elsePC  = immediate (pc_t);                        \
      अगर (((TYPE) _r0) OPER operand)                          \
          अग्र_Op ();                                          \
      अन्यथा jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = slot (TYPE);                            \
      TYPE operand1 = slot (TYPE);                            \
      pc_t elsePC   = immediate (pc_t);                       \
      अगर (operand1 OPER operand2)                             \
          अग्र_Op ();                                          \
      अन्यथा jumpOp (elsePC);                                   \
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
    अगर (((i32)_r0) == 0) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  operand = slot(i32);
    pc_t branch = immediate(pc_t);
    अगर (operand == 0) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = immediate(pc_t);
    अगर (((i32)_r0) == 0) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  operand = slot(i32);
    pc_t elsePC = immediate(pc_t);
    अगर (operand == 0) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}

#पूर्ण_अगर // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = immediate(pc_t);

    अगर (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        अग्र_Op();
    } अन्यथा {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = slot(i32);
    pc_t branch = immediate(pc_t);

    अगर (condition) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_ढेर_Check();

    व्योम* loopId = immediate(व्योम*);
    वापस loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    व्योम* loopId = immediate(व्योम*);

    अगर (condition) {
        वापस loopId;
    } अन्यथा {
        अग्र_Op();
    }
}


#अगर d_m3_Has_ढेर_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_ढेर_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    अगर (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
        SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_op, _pc - 1, _sp,
                          d_m3ExpRegArgs(_r0, _fp0));

        वापस m3Err_continuationSuspended;
    }

    व्योम* loopId = immediate(व्योम*);
    वापस loopId;
}


#  अगर !d_m3EntryKeepsFrame
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(Entry_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = immediate(IM3Function);

    अगर (EntryFits(function, _sp, _mem)) {
        EnterFrame(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        अगर (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
            SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_entry, _pc, _sp,
                              d_m3ExpRegArgs(_r0, _fp0));

            वापस m3Err_continuationSuspended;
        }

        अग्र_Op_Direct();
    } अन्यथा {
        newTrap(m3_Err_trap_ढेर_Overflow);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(EntryCheck)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    अगर (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
        SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_entry, _pc - 1, _sp,
                          d_m3ExpRegArgs(_r0, _fp0));

        वापस m3Err_continuationSuspended;
    }

    अग्र_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    व्योम* loopId = immediate(व्योम*);

    अगर (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        अगर (M3_UNLIKELY(runtime->suspendRequested and runtime->activeContinuation)) {
            SuspendWithoutTag(runtime, runtime->activeContinuation, safepoint_op, _pc - 2, _sp,
                              d_m3ExpRegArgs(_r0, _fp0));

            वापस m3Err_continuationSuspended;
        }
        वापस loopId;
    } अन्यथा {
        अग्र_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
अटल
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3MemoryHeader* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  अगर d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    अगर (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } अन्यथा अगर (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } अन्यथा अगर (op == op_EntryCheck) {
        वापस अग्र_Op_Impl();
    } अन्यथा {
        वापस d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  अगर (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    वापस op(_pc, d_m3OpArgs, d_m3BaseCstr);
#  अन्यथा
    वापस op(_pc, d_m3OpArgs);
#  endif
}
#पूर्ण_अगर // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 मूल्य = *(u32*)_pc++;
    slot(u32) = मूल्य;
    अग्र_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 मूल्य;
    स_नकल(&मूल्य, _pc, माप(मूल्य));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    slot(u64) = मूल्य;
    अग्र_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_ढेर_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(End)
{
    m3_ढेर_Check();
    वापस m3Err_none;
}


d_m3Op(SetGlobal_s32)
{
    u32* global = immediate(u32*);
    *global = slot(u32);

    अग्र_Op();
}


d_m3Op(SetGlobal_s64)
{
    u64* global = immediate(u64*);
    *global = slot(u64);

    अग्र_Op();
}

#अगर d_m3HasFloat
d_m3Op(SetGlobal_f32)
{
    f32* global = immediate(f32*);
    *global = (f32)_fp0;

    अग्र_Op();
}


d_m3Op(SetGlobal_f64)
{
    f64* global = immediate(f64*);
    *global = _fp0;

    अग्र_Op();
}
#पूर्ण_अगर


#अगर d_m3HasMemory64

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
    स_नकल(&offset, _pc, माप(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    अगर (M3_LIKELY(operand < d_m3AddressLimit)) {
        u64 effective = operand + offset;

        // the access itself checks its own size against what is left
        अगर (M3_LIKELY(effective < _mem->लंबाई)) {
            *address = (u32)effective;
            अग्र_Op();
        }
    }

    d_outOfBounds;
}

#पूर्ण_अगर // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#अगर d_m3SkipMemoryBoundsCheck || d_m3GuardedMemory
#  define m3MemCheck(x) true
#अन्यथा
#  define m3MemCheck(x) M3_LIKELY(x)
#पूर्ण_अगर

// memcpy here is to support non-aligned access on some platforms.

#घोषणा d_m3Load(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = immediate (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    u64 operand = (u32) _r0;                            \
    operand += offset;                                  \
                                                        \
    अगर (m3MemCheck(                                     \
        operand + माप (SRC_TYPE) <= _mem->लंबाई     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_तथ्य(_mem) + operand;       \
            SRC_TYPE मूल्य;                             \
            स_नकल(&मूल्य, src8, माप(मूल्य));        \
            M3_BSWAP_##SRC_TYPE(मूल्य);                 \
            REG = (DEST_TYPE)मूल्य;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_outOfBounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    operand += offset;                                  \
                                                        \
    अगर (m3MemCheck(                                     \
        operand + माप (SRC_TYPE) <= _mem->लंबाई     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_तथ्य(_mem) + operand;       \
            SRC_TYPE मूल्य;                             \
            स_नकल(&मूल्य, src8, माप(मूल्य));        \
            M3_BSWAP_##SRC_TYPE(मूल्य);                 \
            REG = (DEST_TYPE)मूल्य;                     \
            d_m3TraceLoad(DEST_TYPE, operand, REG);     \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_outOfBounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#घोषणा d_m3Load_i(DEST_TYPE, SRC_TYPE) d_m3Load(_r0, DEST_TYPE, SRC_TYPE)
#घोषणा d_m3Load_f(DEST_TYPE, SRC_TYPE) d_m3Load(_fp0, DEST_TYPE, SRC_TYPE)

#अगर d_m3HasFloat
d_m3Load_f(f32, f32);
d_m3Load_f(f64, f64);
#पूर्ण_अगर

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

#अगर d_m3FoldSetLocal

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3LoadFold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = immediate (u32);                       \
      DEST_TYPE * dest = slot_ptr (DEST_TYPE);            \
      d_m3_Preload_अग्र ();                                 \
      u64 operand = (u32) _r0;                            \
      operand += offset;                                  \
                                                          \
      अगर (m3MemCheck(                                     \
          operand + माप (SRC_TYPE) <= _mem->लंबाई     \
      )) {                                                \
          u8* src8 = m3_Mem_तथ्य(_mem) + operand;           \
          SRC_TYPE मूल्य;                                 \
          स_नकल(&मूल्य, src8, माप(मूल्य));            \
          M3_BSWAP_##SRC_TYPE(मूल्य);                     \
          DEST_TYPE परिणाम = (DEST_TYPE)मूल्य;            \
          * dest = परिणाम;  REG = परिणाम;                 \
          d_m3TraceLoad(DEST_TYPE, operand, परिणाम);      \
          अग्र_Op_Preloaded ();                             \
      } अन्यथा d_outOfBounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 operand = slot (u32);                           \
      u32 offset = immediate (u32);                       \
      DEST_TYPE * dest = slot_ptr (DEST_TYPE);            \
      d_m3_Preload_अग्र ();                                 \
      operand += offset;                                  \
                                                          \
      अगर (m3MemCheck(                                     \
          operand + माप (SRC_TYPE) <= _mem->लंबाई     \
      )) {                                                \
          u8* src8 = m3_Mem_तथ्य(_mem) + operand;           \
          SRC_TYPE मूल्य;                                 \
          स_नकल(&मूल्य, src8, माप(मूल्य));            \
          M3_BSWAP_##SRC_TYPE(मूल्य);                     \
          DEST_TYPE परिणाम = (DEST_TYPE)मूल्य;            \
          * dest = परिणाम;  REG = परिणाम;                 \
          d_m3TraceLoad(DEST_TYPE, operand, परिणाम);      \
          अग्र_Op_Preloaded ();                             \
      } अन्यथा d_outOfBounds;                               \
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
#  अगर d_m3HasFloat
d_m3LoadFold(_fp0, f32, f32)
d_m3LoadFold(_fp0, f64, f64)
#  endif

#पूर्ण_अगर // d_m3FoldSetLocal

#घोषणा d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    operand += offset;                                  \
                                                        \
    अगर (m3MemCheck(                                     \
        operand + माप (DEST_TYPE) <= _mem->लंबाई    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, REG);     \
            u8* mem8 = m3_Mem_तथ्य(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            स_नकल(mem8, &val, माप(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    स्थिर SRC_TYPE मूल्य = slot (SRC_TYPE);             \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    operand += offset;                                  \
                                                        \
    अगर (m3MemCheck(                                     \
        operand + माप (DEST_TYPE) <= _mem->लंबाई    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, मूल्य);   \
            u8* mem8 = m3_Mem_तथ्य(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) मूल्य;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            स_नकल(mem8, &val, माप(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_outOfBounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    स्थिर SRC_TYPE मूल्य = slot (SRC_TYPE);             \
    u64 operand = slot (u32);                           \
    u32 offset = immediate (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    operand += offset;                                  \
                                                        \
    अगर (m3MemCheck(                                     \
        operand + माप (DEST_TYPE) <= _mem->लंबाई    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, operand, मूल्य);   \
            u8* mem8 = m3_Mem_तथ्य(_mem) + operand;       \
            DEST_TYPE val = (DEST_TYPE) मूल्य;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            स_नकल(mem8, &val, माप(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_outOfBounds;                               \
}

// both operands can be in regs when storing a float
#घोषणा d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 operand = (u32) _r0;                            \
    u32 offset = immediate (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    operand += offset;                                  \
                                                        \
    अगर (m3MemCheck(                                     \
        operand + माप (TYPE) <= _mem->लंबाई         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, operand, REG);         \
            u8* mem8 = m3_Mem_तथ्य(_mem) + operand;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            स_नकल(mem8, &val, माप(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_outOfBounds;                               \
}


#घोषणा d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#घोषणा d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#अगर d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#पूर्ण_अगर

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#समावेश "m3_exec_atomic.h"

#अघोषित m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#अगर d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    अक्षर name[100];
    म_नकल(name, म_माला(i_opcode, "op_") + 3);
    अक्षर* bracket = म_माला(name, "(");
    अगर (bracket) {
        *bracket = 0;
    }

    माला_दो(name);
    अग्र_Op_Direct();
}
#पूर्ण_अगर

#अगर d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    ProfileHit(i_operationName);

    अग्र_Op_Direct();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    ProfileHit(i_operationName);

    अग्र_Op_Direct();
}
#पूर्ण_अगर

d_m3EndExternC

#पूर्ण_अगर // m3_exec_h
