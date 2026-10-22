//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#यदि_अपरिभाषितम् m3_exec_h
#परिभाषय m3_exec_h

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

#यदि_अपरिभाषितम् M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#यदि_अन्तः

#अन्तर्भाव "m3_math_utils.h"
#अन्तर्भाव "m3_compile.h"
#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_info.h"
#अन्तर्भाव "m3_exec_defs.h"
#अन्तर्भाव "m3_atomic.h"
#अन्तर्भाव "m3_host.h"

#अन्तर्भाव <limits.h>

d_m3BeginExternC

#परिभाषय rewrite_op(OP)             * ((शून्य **) (_pc-1)) = (शून्य*)(OP)

#परिभाषय तात्कालिकम्(TYPE)            * ((TYPE *) _pc++)
#परिभाषय skip_तात्कालिकम्(TYPE)       (_pc++)

#परिभाषय slot(TYPE)                 * (TYPE *) (_sp + तात्कालिकम् (i32))
#परिभाषय slot_ptr(TYPE)             (TYPE *) (_sp + तात्कालिकम् (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#परिभाषय d_m3_Wide_सङ्कार्यम्(IS64)      ((IS64) ? slot (u64) : (u64) slot (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#परिभाषय d_m3MemRangeOk(START, दीर्घता_2, MEM)  \
   ((START) <= (MEM)->दीर्घता and (दीर्घता_2) <= (MEM)->दीर्घता - (START))


#यदि d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define अग्र_Op()                 M3_MUSTTAIL प्रत्यागम profileOp (d_m3_Op_All_तर्काः, __FUNCTION__)
#नोचेद्यदि d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define अग्र_Op()                 M3_MUSTTAIL प्रत्यागम debugOp (d_m3_Op_All_तर्काः, __FUNCTION__)
#अन्यथा
#  define अग्र_Op()                 अग्र_Op_प्रत्यक्षम्()
#यदि_अन्तः

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#यदि d_m3_Preload_अग्र_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_अग्र()        ध्रुव IM3Operation अग्र_Op_2 = (IM3Operation) (* _pc)
#  define अग्र_Op_Preloaded()        M3_MUSTTAIL प्रत्यागम अग्र_Op_2 (_pc + 1, d_m3_Op_तर्काः)
#अन्यथा
#  define d_m3_Preload_अग्र()
#  define अग्र_Op_Preloaded()        अग्र_Op()
#यदि_अन्तः

#यदि d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL प्रत्यागम profileJumpOp ((pc_t)(PC), d_m3_Op_तर्काः, __FUNCTION__)
#अन्यथा
#  define jumpOp(PC)               jump_Op_प्रत्यक्षम्(PC)
#यदि_अन्तः

#यदि d_m3_लेखः_Backtraces
#  define प्रक्षिप_Backtrace_Frame_2()            (प्रक्षिप_Backtrace_Frame (_mem->runtime, _pc - 1))
#  define fillBacktraceFrame(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    प्रत्यागम (प्रक्षिप_Backtrace_Frame_2 (), err)
#  define forwardTrap(err)                प्रत्यागम err
#अन्यथा
#  define प्रक्षिप_Backtrace_Frame_2()            कुरु {} यावत् (0)
#  define fillBacktraceFrame(FUNCTION)    कुरु {} यावत् (0)

#  define newTrap(err)                    प्रत्यागम err
#  define forwardTrap(err)                प्रत्यागम err
#यदि_अन्तः

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#यदि d_m3_Max_Native_राशिः > 0
#  define d_m3_Check_Native_राशिः()                                               \
       कुरु {                                                                    \
           शून्य * m3_सीमा = m3MemRuntime (_mem)->राशिः_सीमा;                  \
           यदि (M3_UNLIKELY (m3_सीमा && m3_Native_राशिः_Ptr () < m3_सीमा))      \
               newTrap (m3_Err_trap_राशिः_अतिप्रवाहः);                              \
       } यावत् (0)
#अन्यथा
#  define d_m3_Check_Native_राशिः()           कुरु {} यावत् (0)
#यदि_अन्तः


#यदि d_m3_Has_राशिः_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  यदि d_m3HasFloat
#    define d_m3_Exp_Reg_तर्काः(R0, FP0)        (R0), (FP0)
#  अन्यथा
#    define d_m3_Exp_Reg_तर्काः(R0, FP0)        (R0)
#  endif

#  यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_तर्काः (R0, FP0), d_m3_आधारः_Cstr)
#  अन्यथा
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_तर्काः (R0, FP0))
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
स्थिर M3_NOINLINE
m3ret_t लेखः_Call_Frame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_स्मृतिः i_स्मृतिः,
                         IM3Function i_function, m3reg_t i_r0
#  यदि d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3Frame frame;

    frame.विधा = frame_call;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.स्मृतिः = i_स्मृतिः;
    frame.call.function = i_function;
    frame.call.r0 = i_r0;
#  यदि d_m3HasFloat
    frame.call.fp0 = i_fp0;
#  endif

    प्रत्यागम Continuation_लेखः_Frame(i_runtime, &frame);
}

#  define d_m3_लेखः_Call_Frame(स्मृतिः_2, FUNCTION)                                \
       यदि (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           प्रत्यागम लेखः_Call_Frame (m3MemRuntime (_mem), _pc, _sp, (स्मृतिः_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_तर्काः (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
स्थिर M3_NOINLINE
m3ret_t लेखः_Loop_Frame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_स्मृतिः i_स्मृतिः)
{
    M3Frame frame;

    frame.विधा = frame_loop;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.स्मृतिः = i_स्मृतिः;

    प्रत्यागम Continuation_लेखः_Frame(i_runtime, &frame);
}


#  यदि d_m3HasExceptionHandling
स्थिर M3_NOINLINE
m3ret_t लेखः_Try_Frame (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_स्मृतिः i_स्मृतिः,
                        u32 i_अङ्कः_Clauses, bool i_handlers_जीवितम्)
{
    M3Frame frame;

    frame.विधा = frame_try;
    frame.pc = i_clauses;
    frame.sp = i_sp;
    frame.स्मृतिः = i_स्मृतिः;
    frame.try_.अङ्कः_Clauses = i_अङ्कः_Clauses;
    frame.try_.handlers_जीवितम् = i_handlers_जीवितम्;

    प्रत्यागम Continuation_लेखः_Frame(i_runtime, &frame);
}
#  endif


#  यदि d_m3_प्रविष्टिः_Keeps_Frame
स्थिर M3_NOINLINE
m3ret_t लेखः_प्रविष्टिः_Frame (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_स्मृतिः i_स्मृतिः,
                          IM3Function i_function)
{
    M3Frame frame;

    frame.विधा = frame_प्रविष्टिः;
    frame.pc = i_pc;
    frame.sp = i_sp;
    frame.स्मृतिः = i_स्मृतिः;
    frame.प्रविष्टिः.function = i_function;

    प्रत्यागम Continuation_लेखः_Frame(i_runtime, &frame);
}
#  endif
#अन्यथा
#  define d_m3_लेखः_Call_Frame(स्मृतिः_2, FUNCTION)
#यदि_अन्तः


#यदि d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#नोचेद्यदि d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    ध्रुव IM3Runtime trace_rt = m3MemRuntime(_mem); (शून्य) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_गभीरता)*2, "", ##__VA_ARGS__)
#अन्यथा
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#यदि_अन्तः

#यदि d_m3EnableStrace >= 3
#  define d_m3_Trace_आरोपय(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#अन्यथा
#  define d_m3_Trace_आरोपय(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#यदि_अन्तः

#यदि_परिभाषितम् DEBUG
  #परिभाषय d_बहिर्गमनम्_सम्बन्धि_Bounds newTrap (ErrorRuntime (m3_Err_trap_बहिर्गमनम्_सम्बन्धि_Bounds_स्मृतिः_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->दीर्घता, सङ्कार्यम्))

#  define d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(विस्थापनम्, आकार_2) newTrap (ErrorRuntime (m3_Err_trap_बहिर्गमनम्_सम्बन्धि_Bounds_स्मृतिः_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->दीर्घता, विस्थापनम्, आकार_2))
#अन्यथा
  #परिभाषय d_बहिर्गमनम्_सम्बन्धि_Bounds newTrap (m3_Err_trap_बहिर्गमनम्_सम्बन्धि_Bounds_स्मृतिः_Access)

#  define d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(विस्थापनम्, आकार_2) newTrap (m3_Err_trap_बहिर्गमनम्_सम्बन्धि_Bounds_स्मृतिः_Access)

#यदि_अन्तः

#यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#अन्यथा
d_m3RetSig Call (d_m3OpSig)
#यदि_अन्तः
{
    m3ret_t possible_trap = m3_Yield();
    यदि (M3_UNLIKELY(possible_trap)) {
        प्रत्यागम possible_trap;
    }

    अग्र_Op_प्रत्यक्षम्();
}

#परिभाषय d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE सङ्कार्यम् = slot (TYPE);                         \
    d_m3_Preload_अग्र ();                                 \
    OP((RES), सङ्कार्यम्, ((TYPE) REG), ##__VA_ARGS__);    \
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

#परिभाषय d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE सङ्कार्यम् = slot (TYPE);                         \
    d_m3_Preload_अग्र ();                                 \
    OP((RES), ((TYPE) REG), सङ्कार्यम्, ##__VA_ARGS__);    \
    अग्र_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#परिभाषय d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#परिभाषय d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#परिभाषय d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#परिभाषय d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#परिभाषय M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#परिभाषय M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#परिभाषय M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#परिभाषय M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#परिभाषय d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#परिभाषय d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#परिभाषय d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#परिभाषय d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#परिभाषय d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#परिभाषय d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#परिभाषय d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#परिभाषय d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#परिभाषय d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#परिभाषय d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#परिभाषय d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#परिभाषय d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#परिभाषय d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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

#यदि d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#यदि_अन्तः

#परिभाषय OP_सङ्कलय_32(A,B) (i32)((u32)(A) + (u32)(B))
#परिभाषय OP_सङ्कलय_64(A,B) (i64)((u64)(A) + (u64)(B))
#परिभाषय OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#परिभाषय OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#परिभाषय OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#परिभाषय OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, सङ्कलय,      OP_सङ्कलय_32)  d_m3CommutativeOpFunc_i (i64, सङ्कलय,      OP_सङ्कलय_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, व्यवकलय,            OP_SUB_32)  d_m3OpFunc_i (i64, व्यवकलय,            OP_SUB_64)

#परिभाषय OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#परिभाषय OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#परिभाषय OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#परिभाषय OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, सरणम्_वामः,       OP_SHL_32)      d_m3OpFunc_i (u64, सरणम्_वामः,       OP_SHL_64)
d_m3OpFunc_i (i32, सरणम्_दक्षिणः,      OP_SHR_32)      d_m3OpFunc_i (i64, सरणम्_दक्षिणः,      OP_SHR_64)
d_m3OpFunc_i (u32, सरणम्_दक्षिणः,      OP_SHR_32)      d_m3OpFunc_i (u64, सरणम्_दक्षिणः,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#यदि d_m3_Fold_समुच्चयः_Local

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
    TYPE सङ्कार्यम् = slot (TYPE);                         \
    TYPE * dest = slot_ptr (TYPE);                      \
    d_m3_Preload_अग्र ();                                 \
    TYPE परिणाम;                                        \
    OP((परिणाम), सङ्कार्यम्, ((TYPE) REG), ##__VA_ARGS__); \
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
    TYPE सङ्कार्यम् = slot (TYPE);                         \
    TYPE * dest = slot_ptr (TYPE);                      \
    d_m3_Preload_अग्र ();                                 \
    TYPE परिणाम;                                        \
    OP((परिणाम), ((TYPE) REG), सङ्कार्यम्, ##__VA_ARGS__); \
    * dest = परिणाम;  REG = परिणाम;                     \
    अग्र_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#परिभाषय d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#परिभाषय d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#परिभाषय d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#परिभाषय d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, सङ्कलय,          M3_FUNC, OP_सङ्कलय_32)
d_m3FoldOp_i           (i32, व्यवकलय,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, सरणम्_वामः,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, सरणम्_दक्षिणः,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, सरणम्_दक्षिणः,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, सङ्कलय,          M3_FUNC, OP_सङ्कलय_64)
d_m3FoldOp_i           (i64, व्यवकलय,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, सरणम्_वामः,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, सरणम्_दक्षिणः,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, सरणम्_दक्षिणः,   M3_FUNC, OP_SHR_64)

#  यदि d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, सङ्कलय,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, व्यवकलय,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, सङ्कलय,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, व्यवकलय,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#यदि_अन्तः

#यदि_अन्तः // d_m3FoldSetLocal

#यदि d_m3HasFloat
d_m3CommutativeArithOp_f(f32, सङ्कलय,         +)      d_m3CommutativeArithOp_f(f64, सङ्कलय,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, व्यवकलय,               -)      d_m3ArithOp_f(f64, व्यवकलय,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#यदि_अन्तः

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

#यदि d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#यदि_अन्तः

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#परिभाषय d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    अग्र_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE सङ्कार्यम् = slot (TYPE);                     \
    OP((RES), सङ्कार्यम्, ##__VA_ARGS__);              \
    अग्र_Op ();                                      \
}

#परिभाषय M3_UNARY(RES, X, OP) (RES) = OP(X)
#परिभाषय M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#परिभाषय d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#परिभाषय d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#परिभाषय d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#यदि d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       वर्गमूलम्);
d_m3UnaryArithOp_f(f32, निकटतमः,    rintf);    d_m3UnaryArithOp_f(f64, निकटतमः,    rint);
#  यदि defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  अन्यथा
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#यदि_अन्तः

#परिभाषय OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_गन्तव्यम्_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_गन्तव्यम्_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#यदि (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#परिभाषय OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#परिभाषय OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#परिभाषय OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#परिभाषय OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#नोचेद्यदि defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#परिभाषय OP_CLZ_32(x) __builtin_clz(x)
#परिभाषय OP_CTZ_32(x) __builtin_ctz(x)
#परिभाषय OP_CLZ_64(x) __builtin_clzll(x)
#परिभाषय OP_CTZ_64(x) __builtin_ctzll(x)
#अन्यथा
#परिभाषय OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#परिभाषय OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#परिभाषय OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#परिभाषय OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#यदि_अन्तः

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#परिभाषय OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    अग्र_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 सङ्कार्यम् = slot(i64);
    _r0 = OP_WRAP_I64(सङ्कार्यम्);
    अग्र_Op();
}

// Integer sign extension operations
#परिभाषय OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#परिभाषय OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#परिभाषय OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#परिभाषय OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#परिभाषय OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#यदि d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_उच्चः = slot (u64);
    u64  rhs_नीचः  = slot (u64);
    u64  lhs_उच्चः = slot (u64);
    u64  lhs_नीचः  = slot (u64);
    u64* नीचः     = slot_ptr (u64);
    d_m3_Preload_अग्र();

    u64 योग_2 = lhs_नीचः + rhs_नीचः;

    *नीचः = योग_2;
    _r0  = (i64) (lhs_उच्चः + rhs_उच्चः + (योग_2 < lhs_नीचः));

    अग्र_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_उच्चः = slot (u64);
    u64  rhs_नीचः  = slot (u64);
    u64  lhs_उच्चः = slot (u64);
    u64  lhs_नीचः  = slot (u64);
    u64* नीचः     = slot_ptr (u64);
    d_m3_Preload_अग्र();

    *नीचः = lhs_नीचः - rhs_नीचः;
    _r0  = (i64) (lhs_उच्चः - rhs_उच्चः - (lhs_नीचः < rhs_नीचः));

    अग्र_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = slot (i64);
    i64  a   = slot (i64);
    u64* नीचः = slot_ptr (u64);
    d_m3_Preload_अग्र();

    u64 उच्चः;
    *नीचः = mul_wide_i64 (a, b, &उच्चः);
    _r0  = (i64) उच्चः;

    अग्र_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = slot (u64);
    u64  a   = slot (u64);
    u64* नीचः = slot_ptr (u64);
    d_m3_Preload_अग्र();

    u64 उच्चः;
    *नीचः = mul_wide_u64 (a, b, &उच्चः);
    _r0  = (i64) उच्चः;

    अग्र_Op_Preloaded();
}

#यदि_अन्तः // d_m3HasWideArithmetic

#परिभाषय d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    अग्र_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * राशिः = slot_ptr (FROM);                 \
    OP((DEST), (* राशिः), ##__VA_ARGS__);           \
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
    FROM * राशिः = slot_ptr (FROM);                 \
    TYPE * dest = slot_ptr (TYPE);                  \
    OP((* dest), (* राशिः), ##__VA_ARGS__);         \
    अग्र_Op ();                                      \
}

#यदि d_m3HasFloat
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
#यदि_अन्तः

#परिभाषय d_m3TypeModifyOp(REG_गन्तव्यम्, REG_FROM, गन्तव्यम्_2, NAME, FROM)  \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_गन्तव्यम् = (गन्तव्यम्_2) ((FROM) REG_FROM);                        \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_गन्तव्यम् = (गन्तव्यम्_2) (from);                                   \
    अग्र_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#परिभाषय d_m3FloatModifyOp(गन्तव्यम्_2, NAME, FROM)                   \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((गन्तव्यम्_2) ((FROM) _fp0));             \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = slot (FROM);                                \
    _fp0 = canon_nan_##TO ((गन्तव्यम्_2) (from));                    \
    अग्र_Op ();                                              \
}

#यदि d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#यदि_अन्तः

#परिभाषय d_m3TypeConvertOp(REG_गन्तव्यम्, REG_FROM, गन्तव्यम्_2, NAME, FROM) \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_गन्तव्यम् = (गन्तव्यम्_2) ((FROM) REG_FROM);                        \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    slot (गन्तव्यम्_2) = (गन्तव्यम्_2) ((FROM) REG_FROM);                     \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    REG_गन्तव्यम् = (गन्तव्यम्_2) (from);                                   \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = slot (FROM);                                \
    slot (गन्तव्यम्_2) = (गन्तव्यम्_2) (from);                                \
    अग्र_Op ();                                              \
}

// Int to float
#यदि d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#यदि_अन्तः

#परिभाषय d_m3ReinterpretOp(REG, गन्तव्यम्_2, SRC, FROM)               \
d_m3Op(गन्तव्यम्_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    सङ्घ { FROM c; गन्तव्यम्_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    सङ्घ { FROM c; गन्तव्यम्_2 t; } u;                              \
    u.c = slot (FROM);                                      \
    REG = u.t;                                              \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    सङ्घ { FROM c; गन्तव्यम्_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    slot (गन्तव्यम्_2) = u.t;                                        \
    अग्र_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(गन्तव्यम्_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    सङ्घ { FROM c; गन्तव्यम्_2 t; } u;                              \
    u.c = slot (FROM);                                      \
    slot (गन्तव्यम्_2) = u.t;                                        \
    अग्र_Op ();                                              \
}

#यदि d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#यदि_अन्तः

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = तात्कालिकम्(u32*);
    slot(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    अग्र_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = तात्कालिकम्(u64*);
    slot(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    अग्र_Op();
}


d_m3Op(समुच्चयः_Global_i32)
{
    u32* global = तात्कालिकम्(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    अग्र_Op();
}


d_m3Op(समुच्चयः_Global_i64)
{
    u64* global = तात्कालिकम्(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    अग्र_Op();
}


#यदि d_m3_Has_राशिः_Switching

d_m3Op(Branch);
d_m3Op(प्रविष्टिः_2);
#  यदि !d_m3_प्रविष्टिः_Keeps_Frame
d_m3Op(प्रविष्टिः_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
स्थिर M3_NOINLINE
IM3Function Function_सम्बन्धि_Compiled_संहिता (pc_t i_pc)
{
    यदि (not i_pc) {
        प्रत्यागम रिक्त;
    }

    यावत् ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation प्रविष्टिः = (IM3Operation)i_pc[0];

#  यदि !d_m3_प्रविष्टिः_Keeps_Frame
    यदि (प्रविष्टिः == op_प्रविष्टिः_Suspendable) {
        प्रत्यागम (IM3Function)i_pc[1];
    }
#  endif
    प्रत्यागम (प्रविष्टिः == op_प्रविष्टिः) ? (IM3Function)i_pc[1] : रिक्त;
}

#यदि_अन्तः

d_m3Op(Call)
{
    d_m3_Check_Native_राशिः();

    pc_t      callPC = तात्कालिकम्(pc_t);
    i32       राशिः_विस्थापनम् = तात्कालिकम्(i32);
    IM3_स्मृतिः स्मृतिः = m3MemInfo(_mem);

    m3stack_t sp = _sp + राशिः_विस्थापनम्;

#यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_तर्काः, d_m3_आधारः_Cstr);
#अन्यथा
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_तर्काः);
#यदि_अन्तः

    _mem = स्मृतिः->mallocated;

    यदि (M3_LIKELY(not r)) {
        अग्र_Op();
    } अन्यथा {
        d_m3_लेखः_Call_Frame(स्मृतिः, Function_सम्बन्धि_Compiled_संहिता(callPC));
        प्रक्षिप_Backtrace_Frame_2();
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
    IM3FuncType type = तात्कालिकम्(IM3FuncType);
    i32         राशिः_विस्थापनम् = तात्कालिकम्(i32);
    IM3_स्मृतिः   स्मृतिः = m3MemInfo(_mem);

    m3stack_t   sp = _sp + राशिः_विस्थापनम्;

    m3ret_t     r = m3Err_none;

    यदि (M3_LIKELY(function)) {
        यदि (M3_LIKELY(type == function->funcType)) {
            यदि (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            यदि (M3_LIKELY(not r)) {
#यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_तर्काः, d_m3_आधारः_Cstr);
#अन्यथा
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_तर्काः);
#यदि_अन्तः

                _mem = स्मृतिः->mallocated;

                यदि (M3_LIKELY(not r)) {
                    अग्र_Op_प्रत्यक्षम्();
                } अन्यथा {
                    d_m3_लेखः_Call_Frame(स्मृतिः, function);
                    प्रक्षिप_Backtrace_Frame_2();
                    forwardTrap(r);
                }
            }
        } अन्यथा {
            r = m3_Err_trap_परोक्षम्_Call_Type_Mismatch;
        }
    } अन्यथा {
        r = m3Err_trapNullFunctionRef;
    }

    यदि (M3_UNLIKELY(r)) {
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

    यदि (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    अग्र_Op();
}


d_m3Op(Call_परोक्षम्)
{
    d_m3_Check_Native_राशिः();

    M3_सारणी*    सारणी_2 = तात्कालिकम्(M3_सारणी*);
    u64         सारणी_सूचकांक = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);
    IM3FuncType type = तात्कालिकम्(IM3FuncType);
    i32         राशिः_विस्थापनम् = तात्कालिकम्(i32);
    IM3_स्मृतिः   स्मृतिः = m3MemInfo(_mem);

    m3stack_t   sp = _sp + राशिः_विस्थापनम्;

    m3ret_t     r = m3Err_none;

    यदि (M3_LIKELY(सारणी_सूचकांक < सारणी_2->आकार)) {
        IM3Function function = (IM3Function)सारणी_2->elements[सारणी_सूचकांक];

        यदि (M3_LIKELY(function)) {
            यदि (M3_LIKELY(type == function->funcType)) {
                यदि (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                यदि (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3_स्मृतिः_शीर्षकम्* calleeMem = Module_स्मृतिः_शीर्षकम्(function->module);

#यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_तर्काः, d_m3_आधारः_Cstr);
#अन्यथा
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_तर्काः);
#यदि_अन्तः

                    _mem = स्मृतिः->mallocated;

                    यदि (M3_LIKELY(not r)) {
                        अग्र_Op_प्रत्यक्षम्();
                    } अन्यथा {
                        d_m3_लेखः_Call_Frame(स्मृतिः, function);
                        प्रक्षिप_Backtrace_Frame_2();
                        forwardTrap(r);
                    }
                }
            } अन्यथा {
                r = m3_Err_trap_परोक्षम्_Call_Type_Mismatch;
            }
        } अन्यथा {
            r = m3_Err_trap_सारणी_Element_Is_Null;
        }
    } अन्यथा {
        r = m3_Err_trap_सारणी_सूचकांक_बहिर्गमनम्_सम्बन्धि_Range;
    }

    यदि (M3_UNLIKELY(r)) {
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
#परिभाषय d_m3_पुच्छ_Call_तर्काः(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((शून्य *) (_sp + (RETURNSLOTS)), (ध्रुव शून्य *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * परिमाण (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = तात्कालिकम्(pc_t);
    i32     राशिः_विस्थापनम् = तात्कालिकम्(i32);
    i32     returnSlots = तात्कालिकम्(i32);
    u32     अङ्कः_Arg_Slots = तात्कालिकम्(u32);

    m3ret_t possible_trap = m3_Yield();
    यदि (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_पुच्छ_Call_तर्काः(returnSlots, राशिः_विस्थापनम्, अङ्कः_Arg_Slots);

    jump_Op_प्रत्यक्षम्(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = slot(IM3Function);
    IM3FuncType type = तात्कालिकम्(IM3FuncType);
    i32         राशिः_विस्थापनम् = तात्कालिकम्(i32);
    i32         returnSlots = तात्कालिकम्(i32);
    u32         अङ्कः_Arg_Slots = तात्कालिकम्(u32);

    m3ret_t     r = m3Err_none;

    यदि (M3_LIKELY(function)) {
        यदि (M3_LIKELY(type == function->funcType)) {
            यदि (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            यदि (M3_LIKELY(not r)) {
                r = m3_Yield();

                यदि (M3_LIKELY(not r)) {
                    d_m3_पुच्छ_Call_तर्काः(returnSlots, राशिः_विस्थापनम्, अङ्कः_Arg_Slots);

                    jump_Op_प्रत्यक्षम्(function->compiled);
                }
            }
        } अन्यथा {
            r = m3_Err_trap_परोक्षम्_Call_Type_Mismatch;
        }
    } अन्यथा {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(Return_Call_परोक्षम्)
{
    M3_सारणी*    सारणी_2 = तात्कालिकम्(M3_सारणी*);
    u64         सारणी_सूचकांक = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);
    IM3FuncType type = तात्कालिकम्(IM3FuncType);
    i32         राशिः_विस्थापनम् = तात्कालिकम्(i32);
    i32         returnSlots = तात्कालिकम्(i32);
    u32         अङ्कः_Arg_Slots = तात्कालिकम्(u32);

    m3ret_t     r = m3Err_none;

    यदि (M3_LIKELY(सारणी_सूचकांक < सारणी_2->आकार)) {
        IM3Function function = (IM3Function)सारणी_2->elements[सारणी_सूचकांक];

        यदि (M3_LIKELY(function)) {
            यदि (M3_LIKELY(type == function->funcType)) {
                यदि (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                यदि (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    यदि (M3_LIKELY(not r)) {
                        d_m3_पुच्छ_Call_तर्काः(returnSlots, राशिः_विस्थापनम्, अङ्कः_Arg_Slots);

                        jump_Op_प्रत्यक्षम्(function->compiled);
                    }
                }
            } अन्यथा {
                r = m3_Err_trap_परोक्षम्_Call_Type_Mismatch;
            }
        } अन्यथा {
            r = m3_Err_trap_सारणी_Element_Is_Null;
        }
    } अन्यथा {
        r = m3_Err_trap_सारणी_सूचकांक_बहिर्गमनम्_सम्बन्धि_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = तात्कालिकम्(IM3Function);
    ctx.userdata = तात्कालिकम्(शून्य*);
    u64* ध्रुव sp = ((u64*)_sp);
    IM3_स्मृतिः  स्मृतिः = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#यदि d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    वर्ण        outbuff[1024];
    वर्ण*       outp = outbuff;
    वर्ण*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.क्षेत्रम्_Utf8);

    ध्रुव पूर्णाङ्क n_तर्काः = ftype->अङ्कः_तर्काः;
    ध्रुव पूर्णाङ्क nRets = ftype->अङ्कः_Rets;
    u64*      तर्काः = sp + nRets;
    प्रति (पूर्णाङ्क i = 0; i < n_तर्काः; i++) {
        ध्रुव पूर्णाङ्क type = ftype->types[nRets + i];
        विकल्प (type) {
        पक्ष c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(तर्काः + i)); भङ्ग;
        पक्ष c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(तर्काः + i)); भङ्ग;
        पक्ष c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(तर्काः + i)); भङ्ग;
        पक्ष c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(तर्काः + i)); भङ्ग;
        स्वभाव: outp += snprintf(outp, oute - outp, "<type %d>", type); भङ्ग;
        }
        यदि (i < n_तर्काः - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  यदि d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#यदि_अन्तः

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    शून्य* राशिः_backup = runtime->राशिः;
    runtime->राशिः = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_दत्त(ctx.function->host_स्मृतिः->mallocated));
    runtime->राशिः = राशिः_backup;

#यदि d_m3EnableStrace
    यदि (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (वर्ण*)possible_trap);
    } अन्यथा यदि (ftype and ftype->अङ्कः_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } अन्यथा {
        d_m3TracePrint("%s", outbuff);
    }
#यदि_अन्तः

    यदि (M3_UNLIKELY(possible_trap)) {
        _mem = स्मृतिः->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        यदि (possible_trap != m3Err_pendingException) {
            प्रक्षिप_Backtrace_Frame_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_आकार)
{
    IM3_स्मृतिः स्मृतिः = m3MemInfo(_mem);

    _r0 = स्मृतिः->अङ्कः_पृष्ठानि;

    अग्र_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_स्मृतिः  स्मृतिः = m3MemInfo(_mem);

    i32        अङ्कः_पृष्ठानि_गन्तव्यम्_Grow = (i32)_r0;
    यदि (अङ्कः_पृष्ठानि_गन्तव्यम्_Grow >= 0) {
        _r0 = (m3reg_t)स्मृतिः->अङ्कः_पृष्ठानि;

        यदि (M3_LIKELY(अङ्कः_पृष्ठानि_गन्तव्यम्_Grow)) {
            u64      required_पृष्ठानि = स्मृतिः->अङ्कः_पृष्ठानि + (u32)अङ्कः_पृष्ठानि_गन्तव्यम्_Grow;

            M3_परिणाम r = Resize_स्मृतिः(runtime, स्मृतिः, required_पृष्ठानि);
            यदि (r) {
                _r0 = -1;
            }

            _mem = स्मृतिः->mallocated;
        }
    } अन्यथा {
        _r0 = -1;
    }

    अग्र_Op();
}


#यदि d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_स्मृतिः  स्मृतिः = m3MemInfo(_mem);

    u64        अङ्कः_पृष्ठानि_गन्तव्यम्_Grow = (u64)_r0;
    u64        अङ्कः_पृष्ठानि = स्मृतिः->अङ्कः_पृष्ठानि;

    यदि (M3_LIKELY(अङ्कः_पृष्ठानि_गन्तव्यम्_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        यदि (अङ्कः_पृष्ठानि_गन्तव्यम्_Grow <= स्मृतिः->max_पृष्ठानि - अङ्कः_पृष्ठानि and
            not Resize_स्मृतिः(runtime, स्मृतिः, अङ्कः_पृष्ठानि + अङ्कः_पृष्ठानि_गन्तव्यम्_Grow)) {
            _r0 = (m3reg_t)अङ्कः_पृष्ठानि;
        } अन्यथा {
            _r0 = -1;
        }

        _mem = स्मृतिः->mallocated;
    } अन्यथा {
        _r0 = (m3reg_t)अङ्कः_पृष्ठानि;
    }

    अग्र_Op();
}

#यदि_अन्तः // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 आकार = (u32)_r0;
    u64 source = slot(u32);
    u64 destination = slot(u32);

    यदि (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        यदि (M3_LIKELY(d_m3MemRangeOk(source, आकार, _mem))) {
            u8* dst = m3_Mem_दत्त(_mem) + destination;
            u8* src = m3_Mem_दत्त(_mem) + source;
            memmove(dst, src, (size_t)आकार);

            अग्र_Op();
        } अन्यथा {
            d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(source, आकार);
        }
    } अन्यथा {
        d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(destination, आकार);
    }
}


#यदि d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 आकार = (u64)_r0;
    u64 source = slot(u64);
    u64 destination = slot(u64);

    यदि (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        यदि (M3_LIKELY(d_m3MemRangeOk(source, आकार, _mem))) {
            u8* dst = m3_Mem_दत्त(_mem) + destination;
            u8* src = m3_Mem_दत्त(_mem) + source;
            memmove(dst, src, (size_t)आकार);

            अग्र_Op();
        } अन्यथा {
            d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(source, आकार);
        }
    } अन्यथा {
        d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(destination, आकार);
    }
}

#यदि_अन्तः // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(समुच्चयः_स्मृतिः)
{
    IM3_स्मृतिः स्मृतिः = तात्कालिकम्(IM3_स्मृतिः);

    _mem = स्मृतिः->mallocated;

    अग्र_Op();
}


#यदि d_m3_Has_Multi_स्मृतिः

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3_स्मृतिः       dest_स्मृतिः = तात्कालिकम्(IM3_स्मृतिः);
    IM3_स्मृतिः       source_स्मृतिः = तात्कालिकम्(IM3_स्मृतिः);
    u32             widths = तात्कालिकम्(u32);

    M3_स्मृतिः_शीर्षकम्* destMem = dest_स्मृतिः->mallocated;
    M3_स्मृतिः_शीर्षकम्* sourceMem = source_स्मृतिः->mallocated;

    u64             आकार = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             source = d_m3_Wide_सङ्कार्यम्(widths & 0x2);
    u64             destination = d_m3_Wide_सङ्कार्यम्(widths & 0x1);

    यदि (M3_LIKELY(d_m3MemRangeOk(destination, आकार, destMem))) {
        यदि (M3_LIKELY(d_m3MemRangeOk(source, आकार, sourceMem))) {
            memmove(m3_Mem_दत्त(destMem) + destination, m3_Mem_दत्त(sourceMem) + source, (size_t)आकार);

            अग्र_Op();
        } अन्यथा {
            d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(source, आकार);
        }
    } अन्यथा {
        d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(destination, आकार);
    }
}

#यदि_अन्तः // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 आकार = (u32)_r0;
    u32 byte = slot(u32);
    u64 destination = slot(u32);

    यदि (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        u8* mem8 = m3_Mem_दत्त(_mem) + destination;
        स्मृतिं_पूरय(mem8, (u8)byte, आकार);
        अग्र_Op();
    } अन्यथा {
        d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(destination, आकार);
    }
}


#यदि d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 आकार = (u64)_r0;
    u32 byte = slot(u32);
    u64 destination = slot(u64);

    यदि (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        u8* mem8 = m3_Mem_दत्त(_mem) + destination;
        स्मृतिं_पूरय(mem8, (u8)byte, (size_t)आकार);
        अग्र_Op();
    } अन्यथा {
        d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(destination, आकार);
    }
}

#यदि_अन्तः // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_दत्त_Segment* segment = तात्कालिकम्(M3_दत्त_Segment*);

    u64            आकार = (u32)_r0;
    u64            source = slot(u32);
    u64            destination = slot(u32);

    u64            उपलब्धम् = segment->dropped ? 0 : segment->आकार;

    यदि (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        यदि (M3_LIKELY(source <= उपलब्धम् and आकार <= उपलब्धम् - source)) {
            स्मृतिं_प्रतिलिख(m3_Mem_दत्त(_mem) + destination, segment->दत्त + source, (size_t)आकार);
            अग्र_Op();
        } अन्यथा {
            d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(source, आकार);
        }
    } अन्यथा {
        d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(destination, आकार);
    }
}


#यदि d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_दत्त_Segment* segment = तात्कालिकम्(M3_दत्त_Segment*);

    u64            आकार = (u32)_r0;
    u64            source = slot(u32);
    u64            destination = slot(u64);

    u64            उपलब्धम् = segment->dropped ? 0 : segment->आकार;

    यदि (M3_LIKELY(d_m3MemRangeOk(destination, आकार, _mem))) {
        यदि (M3_LIKELY(source <= उपलब्धम् and आकार <= उपलब्धम् - source)) {
            स्मृतिं_प्रतिलिख(m3_Mem_दत्त(_mem) + destination, segment->दत्त + source, (size_t)आकार);
            अग्र_Op();
        } अन्यथा {
            d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(source, आकार);
        }
    } अन्यथा {
        d_बहिर्गमनम्_सम्बन्धि_Bounds_Mem_Op(destination, आकार);
    }
}

#यदि_अन्तः // d_m3HasMemory64


d_m3Op(दत्त_Drop)
{
    M3_दत्त_Segment* segment = तात्कालिकम्(M3_दत्त_Segment*);
    segment->dropped = true;

    अग्र_Op();
}


#यदि d_m3HasRefTypes

d_m3Op(सारणी_Get)
{
    M3_सारणी* सारणी_2 = तात्कालिकम्(M3_सारणी*);
    u64      index = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);

    यदि (M3_LIKELY(index < सारणी_2->आकार)) {
        _r0 = (u64)(uintptr_t)सारणी_2->elements[index];
        अग्र_Op();
    } अन्यथा {
        newTrap(m3_Err_trap_सारणी_बहिर्गमनम्_सम्बन्धि_Bounds);
    }
}


d_m3Op(सारणी_समुच्चयः)
{
    M3_सारणी* सारणी_2 = तात्कालिकम्(M3_सारणी*);
    शून्य*    मानम् = slot(शून्य*);
    u64      index = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);

    यदि (M3_LIKELY(index < सारणी_2->आकार)) {
        सारणी_2->elements[index] = मानम्;
        अग्र_Op();
    } अन्यथा {
        newTrap(m3_Err_trap_सारणी_बहिर्गमनम्_सम्बन्धि_Bounds);
    }
}


d_m3Op(सारणी_आकार)
{
    M3_सारणी* सारणी_2 = तात्कालिकम्(M3_सारणी*);

    _r0 = सारणी_2->आकार;

    अग्र_Op();
}


d_m3Op(सारणी_Grow)
{
    M3_सारणी* सारणी_2 = तात्कालिकम्(M3_सारणी*);
    u64      delta = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);
    शून्य*    मानम् = slot(शून्य*);

    u32      old_आकार = सारणी_2->आकार;
    u32      max_आकार = सारणी_2->max_आकार ? सारणी_2->max_आकार : d_m3_Max_Sane_सारणी_आकार;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_आकार = (delta <= max_आकार - old_आकार) ? (u64)old_आकार + delta
                                                    : (u64)max_आकार + 1;

    यदि (new_आकार == old_आकार)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_आकार;
        अग्र_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    यदि (new_आकार <= max_आकार and (not runtime->सारणी_Elements_सीमा or
                                delta <= runtime->सारणी_Elements_सीमा - runtime->सारणी_Elements_प्रयुक्तम्)) {
        शून्य** elements = m3_ReallocArray(शून्य*, सारणी_2->elements, (size_t)new_आकार, old_आकार);

        यदि (elements) {
            runtime->सारणी_Elements_प्रयुक्तम् += new_आकार - old_आकार;
            सारणी_2->elements = elements;
            सारणी_2->आकार = (u32)new_आकार;

            प्रति (u32 i = old_आकार; i < सारणी_2->आकार; ++i) {
                सारणी_2->elements[i] = मानम्;
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


d_m3Op(सारणी_Init)
{
    M3_सारणी*          सारणी_2 = तात्कालिकम्(M3_सारणी*);
    M3ElementSegment* segment = तात्कालिकम्(M3ElementSegment*);
    u32               गणनम् = slot(u32);
    u32               source = slot(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);

    u64               उपलब्धम् = segment->dropped ? 0 : segment->अङ्कः_Elements;

    यदि (M3_LIKELY(destination <= सारणी_2->आकार and गणनम् <= सारणी_2->आकार - destination and
                  (u64) source + गणनम् <= उपलब्धम्)) {
        प्रति (u32 i = 0; i < गणनम्; ++i) {
            सारणी_2->elements[destination + i] = segment->resolved[source + i];
        }

        अग्र_Op();
    } अन्यथा {
        newTrap(m3_Err_trap_सारणी_बहिर्गमनम्_सम्बन्धि_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = तात्कालिकम्(M3ElementSegment*);
    segment->dropped = true;

    अग्र_Op();
}


d_m3Op(सारणी_Copy)
{
    M3_सारणी* dst = तात्कालिकम्(M3_सारणी*);
    M3_सारणी* src = तात्कालिकम्(M3_सारणी*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      गणनम् = d_m3_Wide_सङ्कार्यम्(dst->isTable64 and src->isTable64);
    u64      source = d_m3_Wide_सङ्कार्यम्(src->isTable64);
    u64      destination = d_m3_Wide_सङ्कार्यम्(dst->isTable64);

    यदि (M3_LIKELY(destination <= dst->आकार and गणनम् <= dst->आकार - destination and
                  source <= src->आकार and गणनम् <= src->आकार - source)) {
        memmove(dst->elements + destination, src->elements + source, (size_t)गणनम् * परिमाण(शून्य*));

        अग्र_Op();
    } अन्यथा {
        newTrap(m3_Err_trap_सारणी_बहिर्गमनम्_सम्बन्धि_Bounds);
    }
}


d_m3Op(सारणी_Fill)
{
    M3_सारणी* सारणी_2 = तात्कालिकम्(M3_सारणी*);
    u64      गणनम् = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);
    शून्य*    मानम् = slot(शून्य*);
    u64      index = d_m3_Wide_सङ्कार्यम्(सारणी_2->isTable64);

    यदि (M3_LIKELY(index <= सारणी_2->आकार and गणनम् <= सारणी_2->आकार - index)) {
        प्रति (u64 i = 0; i < गणनम्; ++i) {
            सारणी_2->elements[index + i] = मानम्;
        }

        अग्र_Op();
    } अन्यथा {
        newTrap(m3_Err_trap_सारणी_बहिर्गमनम्_सम्बन्धि_Bounds);
    }
}

#यदि_अन्तः // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = तात्कालिकम्(IM3Function);

    m3ret_t     परिणाम = m3Err_none;

    यदि (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        परिणाम = CompileFunction(function);
    }

    यदि (not परिणाम) {
        // patch up compiled pc and call rewritten op_Call
        *((शून्य**)--_pc) = (शून्य*)(function->compiled);
        --_pc;
        अग्र_Op_प्रत्यक्षम्();
    }

    newTrap(परिणाम);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = तात्कालिकम्(IM3Function);

    m3ret_t     परिणाम = m3Err_none;

    यदि (M3_UNLIKELY(not function->compiled)) {
        परिणाम = CompileFunction(function);
    }

    यदि (not परिणाम) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((शून्य**)--_pc) = (शून्य*)(function->compiled);
        --_pc;
        अग्र_Op_प्रत्यक्षम्();
    }

    newTrap(परिणाम);
}


#यदि d_m3_Has_राशिः_Switching
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
स्थिर
शून्य Suspend_Without_चिह्नकम् (IM3Runtime io_runtime, IM3Continuation io_cont, M3_Safe_Point_विधा i_विधा,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  यदि d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_चिह्नकम् = रिक्त;
    io_runtime->suspendHandlerCont = रिक्त;
    io_runtime->suspendStubPC = रिक्त;
    io_runtime->switchTarget = रिक्त;
    io_runtime->अङ्कः_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspendPoint = (u8)i_विधा;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  यदि d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->अङ्कः_Suspend_Results = 0;
}
#यदि_अन्तः // d_m3HasStackSwitching


#यदि d_m3HasGasMetering

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
    u32        cost = तात्कालिकम्(u32);

    runtime->gasRemaining -= (i64)cost;

    यदि (M3_UNLIKELY(runtime->gasRemaining < 0)) {
#  यदि d_m3_Has_राशिः_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        यदि (runtime->isSuspendable and runtime->सक्रिय_Continuation) {
            runtime->suspendRequested = true;
            अग्र_Op();
        }
#  endif
        newTrap(m3_Err_trap_बहिर्गमनम्_सम्बन्धि_Gas);
    }

    अग्र_Op();
}

#यदि_अन्तः // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
स्थिर अन्तःस्थ
bool प्रविष्टिः_माति (IM3Function i_function, m3stack_t _sp, M3_स्मृतिः_शीर्षकम्* _mem)
{
#यदि d_m3_Skip_राशिः_Check
    (शून्य)i_function;
    (शून्य)_sp;
    (शून्य)_mem;
    प्रत्यागम true;
#नोचेद्यदि d_m3_Has_राशिः_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->सक्रिय_Continuation : रिक्त);
    शून्य*           max_राशिः = (_act && _act->val_राशिः)
                                 ? (शून्य*)(_act->val_राशिः + _act->अङ्कः_राशिः_Slots)
                                 : (_mem ? _mem->max_राशिः : (शून्य*)~(uintptr_t)0);
    प्रत्यागम M3_LIKELY((शून्य*)(_sp + i_function->max_राशिः_Slots) < max_राशिः);
#अन्यथा
    प्रत्यागम M3_LIKELY((शून्य*)(_sp + i_function->max_राशिः_Slots) < _mem->max_राशिः);
#यदि_अन्तः
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
स्थिर अन्तःस्थ
शून्य EnterFrame (IM3Function i_function, m3stack_t _sp)
{
#यदि defined(DEBUG)
    i_function->लब्धानि++;
#यदि_अन्तः
    u8* राशिः = (u8*)((m3slot_t*)_sp + i_function->अङ्कः_Ret_And_Arg_Slots);

    स्मृतिं_पूरय(राशिः, 0x0, i_function->अङ्कः_Local_अष्टकानि);
    राशिः += i_function->अङ्कः_Local_अष्टकानि;

    यदि (i_function->constants) {
        स्मृतिं_प्रतिलिख(राशिः, i_function->constants, i_function->अङ्कः_Constant_अष्टकानि);
    }
}


d_m3Op(प्रविष्टिः_2)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = तात्कालिकम्(IM3Function);
#यदि d_m3_प्रविष्टिः_Keeps_Frame
    IM3_स्मृतिः स्मृतिः = m3MemInfo(_mem);
#यदि_अन्तः

    यदि (प्रविष्टिः_माति(function, _sp, _mem)) {
        EnterFrame(function, _sp);

#यदि !d_m3_प्रविष्टिः_Keeps_Frame
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        अग्र_Op_प्रत्यक्षम्();
#अन्यथा

#  यदि d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->अङ्कः_Ret_Slots));
        trace_rt->call_गभीरता++;
#  endif

        m3ret_t r = अग्र_Op_Impl();

#  यदि d_m3EnableStrace >= 2
        trace_rt->call_गभीरता--;

        यदि (r) {
            d_m3TracePrint("} !trap = %s", (वर्ण*)r);
        } अन्यथा यदि (Get_Function_अङ्कः_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } अन्यथा {
            d_m3TracePrint("}");
        }
#  endif

        यदि (M3_UNLIKELY(r)) {
            _mem = स्मृतिः->mallocated;
#  यदि d_m3_Has_राशिः_Switching
            यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                प्रत्यागम लेखः_प्रविष्टिः_Frame(m3MemRuntime(_mem), _pc, _sp, स्मृतिः, function);
            }
#  endif
            fillBacktraceFrame();
        }
        forwardTrap(r);
#यदि_अन्तः // d_m3EntryKeepsFrame
    } अन्यथा {
        newTrap(m3_Err_trap_राशिः_अतिप्रवाहः);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_राशिः();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_स्मृतिः स्मृतिः = m3MemInfo(_mem);

    कुरु {
#यदि d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_गभीरता++;
#यदि_अन्तः
        r = अग्र_Op_Impl();

#यदि d_m3EnableStrace >= 3
        trace_rt->call_गभीरता--;
        d_m3TracePrint("}");
#यदि_अन्तः
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = स्मृतिः->mallocated;
    } यावत् (r == _pc);

#यदि d_m3_Has_राशिः_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        प्रत्यागम लेखः_Loop_Frame(m3MemRuntime(_mem), _pc, _sp, स्मृतिः);
    }
#यदि_अन्तः

    forwardTrap(r);
}


#यदि d_m3HasExceptionHandling

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
d_m3Op(Try_सारणी)
{
    d_m3_Check_Native_राशिः();

    IM3_स्मृतिः  स्मृतिः = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        अङ्कः_Clauses = तात्कालिकम्(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * अङ्कः_Clauses;

    u32        गभीरता = ++runtime->try_गभीरता;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = स्मृतिः->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_गभीरता == गभीरता);
    runtime->try_गभीरता = गभीरता - 1;

#  यदि d_m3_Has_राशिः_Switching
    यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        प्रत्यागम लेखः_Try_Frame(runtime, clauses, _sp, स्मृतिः, अङ्कः_Clauses, isInnermost);
    }
#  endif

    यदि (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        ध्रुव M3Exception* exception = runtime->pendingException;

        प्रति (u32 i = 0; i < अङ्कः_Clauses; ++i) {
            IM3_चिह्नकम् चिह्नकम् = (IM3_चिह्नकम्)clauses[2 * i];

            यदि (चिह्नकम् == रिक्त or चिह्नकम् == exception->चिह्नकम्) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jump_Op_प्रत्यक्षम्(handler);
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

    u32          अङ्कः_तर्काः = तात्कालिकम्(u32);

    प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
        i32 offset = तात्कालिकम्(i32);
        u32 is64 = तात्कालिकम्(u32);
        u64 मानम् = exception->तर्काः[i];

        यदि (is64) {
            *(u64*)(_sp + offset) = मानम्;
        } अन्यथा {
            *(u32*)(_sp + offset) = (u32)मानम्;
        }
    }

    i32 ref_विस्थापनम् = तात्कालिकम्(i32);

    यदि (ref_विस्थापनम् >= 0) {
        *(M3Exception**)(_sp + ref_विस्थापनम्) = exception;
        exception->reified = true;
    } अन्यथा यदि (not exception->reified) {
        FreeException(runtime, exception);
    }

    अग्र_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_चिह्नकम्       चिह्नकम् = तात्कालिकम्(IM3_चिह्नकम्);
    u32          अङ्कः_तर्काः = तात्कालिकम्(u32);

    M3Exception* exception = NewException(runtime, चिह्नकम्, अङ्कः_तर्काः);

    यदि (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
        i32 offset = तात्कालिकम्(i32);
        u32 is64 = तात्कालिकम्(u32);

        exception->तर्काः[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    प्रत्यागम m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = slot(M3Exception*);

    यदि (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    प्रत्यागम m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(निष्कासय_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        गणनम् = तात्कालिकम्(u32);

    runtime->try_गभीरता -= गणनम्;

    अग्र_Op();
}

#यदि_अन्तः // d_m3HasExceptionHandling

#यदि d_m3_Has_राशिः_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = तात्कालिकम्(IM3FuncType);
    i32         dstSlot = तात्कालिकम्(i32);
    i32         funcSlot = तात्कालिकम्(i32);

    IM3Function func = *(IM3Function*)(_sp + funcSlot);
    यदि (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    यदि (M3_UNLIKELY(not Continuation_Can_Acquire_राशिः(runtime))) {
        newTrap(m3_Err_continuation_सीमा_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    यदि (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_परिणाम आवण्टनम् = Continuation_Acquire_राशिः(runtime, cont);
    यदि (M3_UNLIKELY(आवण्टनम्)) {
        cont->state = cont_consumed;
        newTrap(आवण्टनम्);
    }
    *(IM3Continuation*)(_sp + dstSlot) = cont;
    अग्र_Op();
}


स्थिर
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = तात्कालिकम्(IM3FuncType);
    IM3FuncType     ct2 = तात्कालिकम्(IM3FuncType);
    i32             dstSlot = तात्कालिकम्(i32);
    i32             contSlot = तात्कालिकम्(i32);
    u32             अङ्कः_Bound = तात्कालिकम्(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + contSlot);
    यदि (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    यदि (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->प्रविष्टिः_Function);
    यदि (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->प्रविष्टिः_Function = srcCont->प्रविष्टिः_Function;
    dstCont->val_राशिः = srcCont->val_राशिः;
    dstCont->अङ्कः_राशिः_Slots = srcCont->अङ्कः_राशिः_Slots;
    dstCont->चौकटाः = srcCont->चौकटाः;
    dstCont->चौकटाः_Cap = srcCont->चौकटाः_Cap;
    dstCont->अङ्कः_चौकटाः = srcCont->अङ्कः_चौकटाः;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspendPoint = srcCont->suspendPoint;
    dstCont->r0 = srcCont->r0;
#  यदि d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_तर्काः_गणनम् = srcCont->bound_तर्काः_गणनम्;
    dstCont->state = srcCont->state;
#  यदि d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = रिक्त;
#  endif

    dstCont->अङ्कः_Suspend_Results = srcCont->अङ्कः_Suspend_Results;
    प्रति (u32 j = 0; j < srcCont->अङ्कः_Suspend_Results; ++j) {
        dstCont->suspend_परिणाम_Offsets[j] = srcCont->suspend_परिणाम_Offsets[j];
        dstCont->suspend_परिणाम_Is64[j] = srcCont->suspend_परिणाम_Is64[j];
    }

    यदि (dstCont->state == cont_allocated) {
        u16 अङ्कः_Rets = ct1->contFuncType ? Get_Func_Type_अङ्कः_Results(ct1->contFuncType) : 0;
        प्रति (u32 i = 0; i < अङ्कः_Bound; ++i) {
            i32 argSlot = तात्कालिकम्(i32);
            u32 is64 = तात्कालिकम्(u32);
            u32 targetIdx = (अङ्कः_Rets + dstCont->bound_तर्काः_गणनम्) * c_io_Slot_गणनम्;
            यदि (is64) {
                *(u64*)(dstCont->val_राशिः + targetIdx) = *(u64*)(_sp + argSlot);
            } अन्यथा {
                *(u32*)(dstCont->val_राशिः + targetIdx) = *(u32*)(_sp + argSlot);
            }
            dstCont->bound_तर्काः_गणनम्++;
        }
    } अन्यथा {
        IM3Continuation inner = InnermostSuspended(dstCont);
        प्रति (u32 i = 0; i < अङ्कः_Bound; ++i) {
            i32 argSlot = तात्कालिकम्(i32);
            u32 is64 = तात्कालिकम्(u32);
            u32 dstIdx = dstCont->bound_तर्काः_गणनम्;
            यदि (dstIdx < inner->अङ्कः_Suspend_Results) {
                i32 dst_विस्थापनम् = inner->suspend_परिणाम_Offsets[dstIdx];
                यदि (is64) {
                    *(u64*)(inner->sp + dst_विस्थापनम्) = *(u64*)(_sp + argSlot);
                } अन्यथा {
                    *(u32*)(inner->sp + dst_विस्थापनम्) = *(u32*)(_sp + argSlot);
                }
            }
            dstCont->bound_तर्काः_गणनम्++;
        }
    }

    srcCont->val_राशिः = रिक्त;
    srcCont->चौकटाः = रिक्त;
    srcCont->चौकटाः_Cap = 0;
    srcCont->अङ्कः_चौकटाः = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dstSlot) = dstCont;
    अग्र_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
स्थिर
IM3Continuation FindHandler (IM3Continuation i_cont, IM3_चिह्नकम् i_चिह्नकम्, u8 i_विधा, pc_t* o_stubPC)
{
    IM3_चिह्नकम् query_चिह्नकम् = (i_चिह्नकम् and i_चिह्नकम्->resolved) ? i_चिह्नकम्->resolved : i_चिह्नकम्;

    प्रति (IM3Continuation cont = i_cont; cont; cont = cont->parent) {
        प्रति (u32 h = 0; h < cont->अङ्कः_Handlers; ++h) {
            pc_t   प्रविष्टिः = cont->handlersPC + (h * 3);
            IM3_चिह्नकम् handler_चिह्नकम् = (IM3_चिह्नकम्)(*(प्रविष्टिः + 1));
            यदि (handler_चिह्नकम् and handler_चिह्नकम्->resolved) {
                handler_चिह्नकम् = handler_चिह्नकम्->resolved;
            }

            यदि ((u32)(uintptr_t)(*प्रविष्टिः) == i_विधा and handler_चिह्नकम् == query_चिह्नकम्) {
                यदि (o_stubPC) {
                    *o_stubPC = (pc_t)(*(प्रविष्टिः + 2));
                }
                प्रत्यागम cont;
            }
        }
    }

    प्रत्यागम रिक्त;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
स्थिर
शून्य ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    प्रति (u32 i = 0; i < io_cont->अङ्कः_Suspend_Results; ++i) {
        यदि (io_cont->suspend_परिणाम_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_परिणाम_Offsets[i]) = 0;
        } अन्यथा {
            *(m3slot_t*)(i_sp + io_cont->suspend_परिणाम_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->सक्रिय_Continuation;
    यदि (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_चिह्नकम्);
    }

    IM3_चिह्नकम् चिह्नकम् = तात्कालिकम्(IM3_चिह्नकम्);
    u32    अङ्कः_Params = तात्कालिकम्(u32);

    runtime->अङ्कः_Suspend_Payload = अङ्कः_Params;
    प्रति (u32 i = 0; i < अङ्कः_Params; ++i) {
        i32 paramSlot = तात्कालिकम्(i32);
        u32 is64 = तात्कालिकम्(u32);
        यदि (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + paramSlot);
        } अन्यथा {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + paramSlot);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 अङ्कः_Results = तात्कालिकम्(u32);
    cont->अङ्कः_Suspend_Results = अङ्कः_Results;
    प्रति (u32 i = 0; i < अङ्कः_Results; ++i) {
        cont->suspend_परिणाम_Offsets[i] = तात्कालिकम्(i32);
        cont->suspend_परिणाम_Is64[i] = तात्कालिकम्(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = रिक्त;
    IM3Continuation अन्वेषणम्_Cont = FindHandler(cont, चिह्नकम्, 0, &matchedStubPC);

    यदि (M3_UNLIKELY(not अन्वेषणम्_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_चिह्नकम्);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = अन्वेषणम्_Cont;
    runtime->switchTarget = रिक्त;
    runtime->suspend_चिह्नकम् = चिह्नकम्;

    cont->pc = _pc;
    cont->suspendPoint = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  यदि d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_तर्काः_गणनम् = 0;

    प्रत्यागम m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
स्थिर
m3ret_t सततम्_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_अङ्कः_Handlers, M3_स्मृतिः_शीर्षकम्** io_mem);
स्थिर
शून्य TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_अङ्कः_Results);
स्थिर
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_स्मृतिः_शीर्षकम्* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
स्थिर अन्तःस्थ
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    प्रत्यागम pass and (i_cont->suspendPoint == safepoint_op or i_cont->suspendPoint == safepoint_प्रविष्टिः);
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
स्थिर
m3ret_t Replay_चौकटाः (IM3Continuation i_cont, i32 i_गभीरता, M3_स्मृतिः_शीर्षकम्* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  यदि d_m3_Max_Native_राशिः > 0
    {
        शून्य* सीमा = runtime->राशिः_सीमा;

        यदि (M3_UNLIKELY(सीमा and m3_Native_राशिः_Ptr() < सीमा)) {
            प्रत्यागम m3_Err_trap_राशिः_अतिप्रवाहः;
        }
    }
#  endif

    यदि (i_गभीरता < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  यदि d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        यदि (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = रिक्त;

            प्रत्यागम m3Err_pendingException;
        }
#  endif
        यदि (pastCheck) {
            प्रत्यागम ResumePastCheck(i_cont, _mem);
        }

        प्रत्यागम d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3Frame frame = i_cont->चौकटाः[i_गभीरता];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_Frame_विधा)frame.विधा == frame_resume);

#  यदि d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_Frame_विधा)frame.विधा == frame_try);
#  endif

    यदि (not ownDescent) {
        r = Replay_चौकटाः(i_cont, i_गभीरता - 1, _mem);
        _mem = frame.स्मृतिः->mallocated;
    }

    विकल्प ((M3_Frame_विधा)frame.विधा) {
    पक्ष frame_call:
        यदि (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            प्रत्यागम d_m3CallWithRegs(frame.pc, frame.sp, _mem, frame.call.r0, frame.call.fp0);
        }

        यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            प्रत्यागम Continuation_लेखः_Frame(runtime, &frame);
        }
#  यदि d_m3_लेखः_Backtraces
        प्रक्षिप_Backtrace_Frame(runtime, frame.pc - 1);
#  endif
        प्रत्यागम r;

    पक्ष frame_loop: {
        m3stack_t _sp = frame.sp;
        m3reg_t   _r0 = 0;
#  यदि d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        यावत् (r == (m3ret_t)frame.pc) {
            r = jumpOpImpl(frame.pc);
            _mem = frame.स्मृतिः->mallocated;
        }

        यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            प्रत्यागम Continuation_लेखः_Frame(runtime, &frame);
        }
        प्रत्यागम r;
    }

#  यदि d_m3HasExceptionHandling
    पक्ष frame_try: {
        m3stack_t _sp = frame.sp;
        m3reg_t   _r0 = 0;
#    यदि d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (शून्य)_sp;
        (शून्य)_r0;
#    यदि d_m3HasFloat
        (शून्य)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 गभीरता = ++runtime->try_गभीरता;

        यदि (not frame.try_.handlers_जीवितम्) {
            runtime->try_गभीरता = गभीरता - 1;
        }

        r = Replay_चौकटाः(i_cont, i_गभीरता - 1, _mem);
        _mem = frame.स्मृतिः->mallocated;

        bool isInnermost = (runtime->try_गभीरता == गभीरता);
        runtime->try_गभीरता = गभीरता - 1;

        यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            frame.try_.handlers_जीवितम् = isInnermost;
            प्रत्यागम Continuation_लेखः_Frame(runtime, &frame);
        }

        यदि (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            ध्रुव M3Exception* exception = runtime->pendingException;

            प्रति (u32 i = 0; i < frame.try_.अङ्कः_Clauses; ++i) {
                IM3_चिह्नकम् चिह्नकम् = (IM3_चिह्नकम्)frame.pc[2 * i];

                यदि (चिह्नकम् == रिक्त or चिह्नकम् == exception->चिह्नकम्) {
                    pc_t handler = (pc_t)frame.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    प्रत्यागम jumpOpImpl(handler);
                }
            }
        }
        प्रत्यागम r;
    }
#  endif

    पक्ष frame_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = frame.resume.cont;

        r = सततम्_Under_Handlers(runtime, &inner, frame.resume.handlersPC,
                             frame.resume.अङ्कः_Handlers, &_mem);

        यदि (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, frame.sp, frame.resume.resultsPC,
                                    frame.resume.अङ्कः_Results);
            Continuation_Recycle_राशिः(runtime, inner);

            प्रत्यागम d_m3CallWithRegs(frame.pc, frame.sp, _mem, 0, 0.);
        }

        यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            यदि (runtime->suspendHandlerCont != inner) {
                frame.resume.cont = inner;
                प्रत्यागम Continuation_लेखः_Frame(runtime, &frame);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            यदि (M3_UNLIKELY(not suspended)) {
                प्रत्यागम m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            यदि (runtime->suspendStubPC) {
                प्रत्यागम d_m3CallWithRegs(runtime->suspendStubPC, frame.sp, _mem, 0, 0.);
            } अन्यथा {
                प्रत्यागम m3_Err_trap_Unhandled_Control_चिह्नकम्;
            }
        } अन्यथा {
            inner->state = cont_consumed;
            Continuation_Recycle_राशिः(runtime, inner);
        }

        प्रत्यागम r;
    }

#  यदि d_m3_प्रविष्टिः_Keeps_Frame
    पक्ष frame_प्रविष्टिः:
        यदि (M3_UNLIKELY(r)) {
            यदि (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                प्रत्यागम Continuation_लेखः_Frame(runtime, &frame);
            }
#    यदि d_m3_लेखः_Backtraces
            FillBacktraceFunctionInfo(runtime, frame.प्रविष्टिः.function);
#    अन्यथा
            (शून्य)frame.प्रविष्टिः.function;
#    endif
        }
        प्रत्यागम r;
#  endif
    }

    प्रत्यागम r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
स्थिर
शून्य* ContinuationArgDest (IM3Continuation i_cont, u32 i_सूचकांक)
{
    यदि (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : रिक्त;
        u16         अङ्कः_Rets = inner ? Get_Func_Type_अङ्कः_Results(inner) : 0;

        प्रत्यागम i_cont->val_राशिः + (अङ्कः_Rets + i_सूचकांक) * c_io_Slot_गणनम्;
    }

    यदि (i_सूचकांक < i_cont->अङ्कः_Suspend_Results) {
        प्रत्यागम i_cont->sp + i_cont->suspend_परिणाम_Offsets[i_सूचकांक];
    }

    प्रत्यागम रिक्त;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
स्थिर
शून्य Hand_Over_गन्तव्यम्_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 अङ्कः_तर्काः = i_runtime->अङ्कः_Suspend_Payload;
    u32 आधारः = (io_peer->state == cont_allocated) ? io_peer->bound_तर्काः_गणनम् : 0;

    प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
        शून्य* dest = ContinuationArgDest(io_peer, आधारः + i);

        यदि (dest) {
            यदि (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } अन्यथा {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    शून्य* refDest = ContinuationArgDest(io_peer, आधारः + अङ्कः_तर्काः);

    यदि (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    यदि (io_peer->state == cont_allocated) {
        io_peer->bound_तर्काः_गणनम् += अङ्कः_तर्काः + 1;
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
स्थिर
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    यावत् (i_cont->अङ्कः_चौकटाः and (M3_Frame_विधा) i_cont->चौकटाः[0].विधा == frame_resume) {
        i_cont = i_cont->चौकटाः[0].resume.cont;
    }

    प्रत्यागम i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
स्थिर
m3ret_t सततम्_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_अङ्कः_Handlers, M3_स्मृतिः_शीर्षकम्** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation parent = i_runtime->सक्रिय_Continuation;
    IM3_स्मृतिः       स्मृतिः = m3MemInfo(*io_mem);
    m3ret_t         r;

    प्रति (;;) {
#  यदि d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        यदि (M3_UNLIKELY(cont->resumeThrow and cont->अङ्कः_चौकटाः == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = रिक्त;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            भङ्ग;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        यदि (isStarting) {
            यदि (M3_UNLIKELY(not cont->प्रविष्टिः_Function->compiled)) {
                r = CompileFunction(cont->प्रविष्टिः_Function);

                यदि (M3_UNLIKELY(r)) {
                    भङ्ग;
                }
            }
            cont->pc = cont->प्रविष्टिः_Function->compiled;
            cont->sp = cont->val_राशिः;
            cont->अङ्कः_चौकटाः = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->अङ्कः_Handlers = i_अङ्कः_Handlers;
        cont->parent = parent;
        cont->state = cont_running;
        cont->bound_तर्काः_गणनम् = 0;

        i_runtime->सक्रिय_Continuation = cont;

        यदि (cont->अङ्कः_चौकटाः) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->अङ्कः_चौकटाः - 1;
            cont->अङ्कः_चौकटाः = 0;

            r = Replay_चौकटाः(cont, outermost, *io_mem);
        } अन्यथा यदि (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } अन्यथा {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = स्मृतिः->mallocated;
        i_runtime->सक्रिय_Continuation = parent;

        यदि (r != m3Err_continuationSuspended) {
            भङ्ग;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        यदि (not i_runtime->switchTarget or i_runtime->suspendHandlerCont != cont) {
            भङ्ग;
        }

        IM3Continuation peer = i_runtime->switchTarget;
        i_runtime->switchTarget = रिक्त;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        यदि (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            भङ्ग;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_Cont_Type_सम्बन्धि_Ref(peer->प्रविष्टिः_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_अङ्कः_Params(peerType) - 1)));
        Hand_Over_गन्तव्यम्_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    प्रत्यागम r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
स्थिर
शून्य TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_अङ्कः_Results)
{
    प्रति (u32 i = 0; i < i_अङ्कः_Results; ++i) {
        i32 resSlot = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_Slot_गणनम्;

        यदि (is64) {
            *(u64*)(i_sp + resSlot) = *(u64*)(i_cont->val_राशिः + srcIdx);
        } अन्यथा {
            *(u32*)(i_sp + resSlot) = *(u32*)(i_cont->val_राशिः + srcIdx);
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
    IM3FuncType     contFuncType = तात्कालिकम्(IM3FuncType);
    i32             contSlot = तात्कालिकम्(i32);
    u32             बहुलकम् = तात्कालिकम्(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + contSlot);
    यदि (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    यदि (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         अङ्कः_Rets = Get_Func_Type_अङ्कः_Results(innerType);

    यदि (बहुलकम् == d_m3ResumeThrowRef) {
        i32 exnSlot = तात्कालिकम्(i32);

#  यदि d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exnSlot);

        यदि (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  अन्यथा
        (शून्य)exnSlot;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } अन्यथा यदि (बहुलकम् == d_m3ResumeThrow) {
        IM3_चिह्नकम् exn_चिह्नकम् = तात्कालिकम्(IM3_चिह्नकम्);
        u32    अङ्कः_तर्काः = तात्कालिकम्(u32);

#  यदि d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_चिह्नकम्, अङ्कः_तर्काः);

        यदि (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
            i32 offset = तात्कालिकम्(i32);
            u32 is64 = तात्कालिकम्(u32);

            exception->तर्काः[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  अन्यथा
        (शून्य)exn_चिह्नकम्;
        (शून्य)अङ्कः_तर्काः;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } अन्यथा {
        u32 अङ्कः_तर्काः = तात्कालिकम्(u32);

        यदि (cont->state == cont_allocated) {
            प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
                i32 argSlot = तात्कालिकम्(i32);
                u32 is64 = तात्कालिकम्(u32);
                u32 targetIdx = (अङ्कः_Rets + cont->bound_तर्काः_गणनम् + i) * c_io_Slot_गणनम्;
                यदि (is64) {
                    *(u64*)(cont->val_राशिः + targetIdx) = *(u64*)(_sp + argSlot);
                } अन्यथा {
                    *(u32*)(cont->val_राशिः + targetIdx) = *(u32*)(_sp + argSlot);
                }
            }
            cont->bound_तर्काः_गणनम् = 0;
        } अन्यथा {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
                i32 argSlot = तात्कालिकम्(i32);
                u32 is64 = तात्कालिकम्(u32);
                u32 dstIdx = cont->bound_तर्काः_गणनम् + i;
                यदि (dstIdx < inner->अङ्कः_Suspend_Results) {
                    i32 dst_विस्थापनम् = inner->suspend_परिणाम_Offsets[dstIdx];
                    यदि (is64) {
                        *(u64*)(inner->sp + dst_विस्थापनम्) = *(u64*)(_sp + argSlot);
                    } अन्यथा {
                        *(u32*)(inner->sp + dst_विस्थापनम्) = *(u32*)(_sp + argSlot);
                    }
                }
            }
        }
    }

    u32  अङ्कः_Handlers = तात्कालिकम्(u32);
    pc_t handlersPC = _pc;
    _pc += अङ्कः_Handlers * 3;

    u32       अङ्कः_Results = तात्कालिकम्(u32);
    pc_t      resultsPC = _pc;

    IM3_स्मृतिः स्मृतिः = m3MemInfo(_mem);

    m3ret_t   r = सततम्_Under_Handlers(runtime, &cont, handlersPC, अङ्कः_Handlers, &_mem);

    यदि (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, अङ्कः_Results);
        Continuation_Recycle_राशिः(runtime, cont);

        _pc += अङ्कः_Results * 2;
        अग्र_Op();
    } अन्यथा यदि (r == m3Err_continuationSuspended) {
        यदि (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            यदि (M3_UNLIKELY(not runtime->सक्रिय_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_चिह्नकम्);
            }
            M3Frame frame;
            frame.विधा = frame_resume;
            frame.pc = resultsPC + अङ्कः_Results * 2;
            frame.sp = _sp;
            frame.स्मृतिः = स्मृतिः;
            frame.resume.cont = cont;
            frame.resume.handlersPC = handlersPC;
            frame.resume.अङ्कः_Handlers = अङ्कः_Handlers;
            frame.resume.resultsPC = resultsPC;
            frame.resume.अङ्कः_Results = अङ्कः_Results;

            प्रत्यागम Continuation_लेखः_Frame(runtime, &frame);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        यदि (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        यदि (runtime->suspendStubPC) {
            jump_Op_प्रत्यक्षम्(runtime->suspendStubPC);
        } अन्यथा {
            newTrap(m3_Err_trap_Unhandled_Control_चिह्नकम्);
        }
    } अन्यथा {
        cont->state = cont_consumed;
        Continuation_Recycle_राशिः(runtime, cont);
        प्रक्षिप_Backtrace_Frame_2();
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
    cont->type = तात्कालिकम्(IM3FuncType);

    u32 अङ्कः_तर्काः = तात्कालिकम्(u32);
    प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
        i32 offset = तात्कालिकम्(i32);
        u32 is64 = तात्कालिकम्(u32);
        u64 val = (i < runtime->अङ्कः_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        यदि (is64) {
            *(u64*)(_sp + offset) = val;
        } अन्यथा {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_विस्थापनम् = तात्कालिकम्(i32);
    यदि (cont_विस्थापनम् >= 0) {
        *(IM3Continuation*)(_sp + cont_विस्थापनम्) = cont;
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
    IM3Continuation cont = runtime->सक्रिय_Continuation;

    यदि (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_चिह्नकम्);
    }

    skip_तात्कालिकम्(IM3FuncType);

    IM3_चिह्नकम्          चिह्नकम् = तात्कालिकम्(IM3_चिह्नकम्);
    i32             targetSlot = तात्कालिकम्(i32);

    IM3Continuation target = *(IM3Continuation*)(_sp + targetSlot);

    यदि (M3_UNLIKELY(not target)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    यदि (M3_UNLIKELY(target->state == cont_consumed or target->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = FindHandler(cont, चिह्नकम्, 1, रिक्त);

    यदि (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_चिह्नकम्);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 अङ्कः_तर्काः = तात्कालिकम्(u32);

    runtime->अङ्कः_Suspend_Payload = अङ्कः_तर्काः;

    प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
        i32 argSlot = तात्कालिकम्(i32);
        u32 is64 = तात्कालिकम्(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + argSlot) : (u64) * (u32*)(_sp + argSlot);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 अङ्कः_Results = तात्कालिकम्(u32);

    cont->अङ्कः_Suspend_Results = अङ्कः_Results;

    प्रति (u32 i = 0; i < अङ्कः_Results; ++i) {
        cont->suspend_परिणाम_Offsets[i] = तात्कालिकम्(i32);
        cont->suspend_परिणाम_Is64[i] = तात्कालिकम्(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = रिक्त;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switchTarget = target;
    runtime->suspend_चिह्नकम् = चिह्नकम्;

    cont->pc = _pc;
    cont->suspendPoint = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  यदि d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    प्रत्यागम m3Err_continuationSuspended;
}


#यदि_अन्तः // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = तात्कालिकम्(pc_t);

    यदि (condition) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = slot(i32);

    pc_t elsePC = तात्कालिकम्(pc_t);

    यदि (condition) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_सारणी)
{
    u32   branch_सूचकांक = slot(u32);           // branch index is always in a slot
    u32   अङ्कः_Targets = तात्कालिकम्(u32);

    pc_t* branches = (pc_t*)_pc;

    यदि (branch_सूचकांक > अङ्कः_Targets) {
        branch_सूचकांक = अङ्कः_Targets; // the default index
    }

    jumpOp(branches[branch_सूचकांक]);
}


#परिभाषय d_m3_समुच्चयः_पञ्जिकाकोशः_समुच्चयः_Slot(TYPE, REG) \
d_m3Op  (समुच्चयः_पञ्जिकाकोशः_2##TYPE)            \
{                                       \
    TYPE * src = slot_ptr (TYPE);       \
    d_m3_Preload_अग्र ();                 \
    REG = * src;                        \
    अग्र_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (समुच्चयः_Slot_2##TYPE)                 \
{                                       \
    TYPE * dst = slot_ptr (TYPE);       \
    d_m3_Preload_अग्र ();                 \
    * dst = (TYPE) REG;                 \
    अग्र_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_समुच्चयः_Slot_2##TYPE)         \
{                                       \
    TYPE * राशिः     = slot_ptr (TYPE); \
    TYPE * preserve  = slot_ptr (TYPE); \
    d_m3_Preload_अग्र ();                 \
                                        \
    * preserve = * राशिः;               \
    * राशिः = (TYPE) REG;               \
                                        \
    अग्र_Op_Preloaded ();                 \
}

d_m3_समुच्चयः_पञ्जिकाकोशः_समुच्चयः_Slot(i32, _r0)
d_m3_समुच्चयः_पञ्जिकाकोशः_समुच्चयः_Slot(i64, _r0)
#यदि d_m3HasFloat
d_m3_समुच्चयः_पञ्जिकाकोशः_समुच्चयः_Slot(f32, _fp0)
d_m3_समुच्चयः_पञ्जिकाकोशः_समुच्चयः_Slot(f64, _fp0)
#यदि_अन्तः

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


#यदि d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_राशिः)
{
    u32         opcode_सूचकांक = तात्कालिकम्(u32);
    u32         राशिः_Height = तात्कालिकम्(u32);
    IM3Function function = तात्कालिकम्(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    प्रारूप_लिख(" %4d ", opcode_सूचकांक);
    प्रारूप_लिख(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  यदि d_m3HasFloat
    प्रारूप_लिख("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    प्रति (u32 i = 0; i < राशिः_Height; ++i) {
        cstr_t विधा = "";

        प्रारूप_लिख("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, विधा, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    प्रारूप_लिख("---------------------------------------------------------------------------------------------------------\n");

    अग्र_Op_प्रत्यक्षम्();
}
#यदि_अन्तः


#परिभाषय d_m3Select_i(TYPE, REG)                 \
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


#परिभाषय d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
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

#यदि d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, slot(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, slot(i32))
#यदि_अन्तः

d_m3Op(Return)
{
    m3_राशिः_Check();
    प्रत्यागम m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = तात्कालिकम्(pc_t);

    यदि (condition) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = slot(i32);
    pc_t branch = तात्कालिकम्(pc_t);

    यदि (condition) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}

#यदि d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE सङ्कार्यम् = slot (TYPE);                             \
      pc_t branch  = तात्कालिकम् (pc_t);                        \
      यदि (सङ्कार्यम् OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      अन्यथा अग्र_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE सङ्कार्यम् = slot (TYPE);                             \
      pc_t branch  = तात्कालिकम् (pc_t);                        \
      यदि (((TYPE) _r0) OPER सङ्कार्यम्)                          \
          jumpOp (branch);                                    \
      अन्यथा अग्र_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = slot (TYPE);                            \
      TYPE operand1 = slot (TYPE);                            \
      pc_t branch   = तात्कालिकम् (pc_t);                       \
      यदि (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      अन्यथा अग्र_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE सङ्कार्यम् = slot (TYPE);                             \
      pc_t elsePC  = तात्कालिकम् (pc_t);                        \
      यदि (सङ्कार्यम् OPER ((TYPE) _r0))                          \
          अग्र_Op ();                                          \
      अन्यथा jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE सङ्कार्यम् = slot (TYPE);                             \
      pc_t elsePC  = तात्कालिकम् (pc_t);                        \
      यदि (((TYPE) _r0) OPER सङ्कार्यम्)                          \
          अग्र_Op ();                                          \
      अन्यथा jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = slot (TYPE);                            \
      TYPE operand1 = slot (TYPE);                            \
      pc_t elsePC   = तात्कालिकम् (pc_t);                       \
      यदि (operand1 OPER operand2)                             \
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
    pc_t branch = तात्कालिकम्(pc_t);
    यदि (((i32)_r0) == 0) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  सङ्कार्यम् = slot(i32);
    pc_t branch = तात्कालिकम्(pc_t);
    यदि (सङ्कार्यम् == 0) {
        jumpOp(branch);
    } अन्यथा {
        अग्र_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = तात्कालिकम्(pc_t);
    यदि (((i32)_r0) == 0) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  सङ्कार्यम् = slot(i32);
    pc_t elsePC = तात्कालिकम्(pc_t);
    यदि (सङ्कार्यम् == 0) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(elsePC);
    }
}

#यदि_अन्तः // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = तात्कालिकम्(pc_t);

    यदि (condition) {
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
    pc_t branch = तात्कालिकम्(pc_t);

    यदि (condition) {
        अग्र_Op();
    } अन्यथा {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_राशिः_Check();

    शून्य* loopId = तात्कालिकम्(शून्य*);
    प्रत्यागम loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    शून्य* loopId = तात्कालिकम्(शून्य*);

    यदि (condition) {
        प्रत्यागम loopId;
    } अन्यथा {
        अग्र_Op();
    }
}


#यदि d_m3_Has_राशिः_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_राशिः_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    यदि (M3_UNLIKELY(runtime->suspendRequested and runtime->सक्रिय_Continuation)) {
        Suspend_Without_चिह्नकम्(runtime, runtime->सक्रिय_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_तर्काः(_r0, _fp0));

        प्रत्यागम m3Err_continuationSuspended;
    }

    शून्य* loopId = तात्कालिकम्(शून्य*);
    प्रत्यागम loopId;
}


#  यदि !d_m3_प्रविष्टिः_Keeps_Frame
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(प्रविष्टिः_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = तात्कालिकम्(IM3Function);

    यदि (प्रविष्टिः_माति(function, _sp, _mem)) {
        EnterFrame(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        यदि (M3_UNLIKELY(runtime->suspendRequested and runtime->सक्रिय_Continuation)) {
            Suspend_Without_चिह्नकम्(runtime, runtime->सक्रिय_Continuation, safepoint_प्रविष्टिः, _pc, _sp,
                              d_m3_Exp_Reg_तर्काः(_r0, _fp0));

            प्रत्यागम m3Err_continuationSuspended;
        }

        अग्र_Op_प्रत्यक्षम्();
    } अन्यथा {
        newTrap(m3_Err_trap_राशिः_अतिप्रवाहः);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(प्रविष्टिः_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    यदि (M3_UNLIKELY(runtime->suspendRequested and runtime->सक्रिय_Continuation)) {
        Suspend_Without_चिह्नकम्(runtime, runtime->सक्रिय_Continuation, safepoint_प्रविष्टिः, _pc - 1, _sp,
                          d_m3_Exp_Reg_तर्काः(_r0, _fp0));

        प्रत्यागम m3Err_continuationSuspended;
    }

    अग्र_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    शून्य* loopId = तात्कालिकम्(शून्य*);

    यदि (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        यदि (M3_UNLIKELY(runtime->suspendRequested and runtime->सक्रिय_Continuation)) {
            Suspend_Without_चिह्नकम्(runtime, runtime->सक्रिय_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_तर्काः(_r0, _fp0));

            प्रत्यागम m3Err_continuationSuspended;
        }
        प्रत्यागम loopId;
    } अन्यथा {
        अग्र_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
स्थिर
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_स्मृतिः_शीर्षकम्* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  यदि d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    यदि (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } अन्यथा यदि (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } अन्यथा यदि (op == op_प्रविष्टिः_Check) {
        प्रत्यागम अग्र_Op_Impl();
    } अन्यथा {
        प्रत्यागम d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  यदि (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    प्रत्यागम op(_pc, d_m3_Op_तर्काः, d_m3_आधारः_Cstr);
#  अन्यथा
    प्रत्यागम op(_pc, d_m3_Op_तर्काः);
#  endif
}
#यदि_अन्तः // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 मानम् = *(u32*)_pc++;
    slot(u32) = मानम्;
    अग्र_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 मानम्;
    स्मृतिं_प्रतिलिख(&मानम्, _pc, परिमाण(मानम्));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    slot(u64) = मानम्;
    अग्र_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_राशिः_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(अन्तः_2)
{
    m3_राशिः_Check();
    प्रत्यागम m3Err_none;
}


d_m3Op(समुच्चयः_Global_s32)
{
    u32* global = तात्कालिकम्(u32*);
    *global = slot(u32);

    अग्र_Op();
}


d_m3Op(समुच्चयः_Global_s64)
{
    u64* global = तात्कालिकम्(u64*);
    *global = slot(u64);

    अग्र_Op();
}

#यदि d_m3HasFloat
d_m3Op(समुच्चयः_Global_f32)
{
    f32* global = तात्कालिकम्(f32*);
    *global = (f32)_fp0;

    अग्र_Op();
}


d_m3Op(समुच्चयः_Global_f64)
{
    f64* global = तात्कालिकम्(f64*);
    *global = _fp0;

    अग्र_Op();
}
#यदि_अन्तः


#यदि d_m3HasMemory64

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
    u64  सङ्कार्यम् = slot(u64);
    u32* स्थानाङ्कः = slot_ptr(u32);

    u64  offset;
    स्मृतिं_प्रतिलिख(&offset, _pc, परिमाण(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    यदि (M3_LIKELY(सङ्कार्यम् < d_m3_स्थानाङ्कः_सीमा)) {
        u64 effective = सङ्कार्यम् + offset;

        // the access itself checks its own size against what is left
        यदि (M3_LIKELY(effective < _mem->दीर्घता)) {
            *स्थानाङ्कः = (u32)effective;
            अग्र_Op();
        }
    }

    d_बहिर्गमनम्_सम्बन्धि_Bounds;
}

#यदि_अन्तः // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#यदि d_m3_Skip_स्मृतिः_Bounds_Check || d_m3_Guarded_स्मृतिः
#  define m3MemCheck(x) true
#अन्यथा
#  define m3MemCheck(x) M3_LIKELY(x)
#यदि_अन्तः

// memcpy here is to support non-aligned access on some platforms.

#परिभाषय d_m3_आरोपय(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = तात्कालिकम् (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    u64 सङ्कार्यम् = (u32) _r0;                            \
    सङ्कार्यम् += offset;                                  \
                                                        \
    यदि (m3MemCheck(                                     \
        सङ्कार्यम् + परिमाण (SRC_TYPE) <= _mem->दीर्घता     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;       \
            SRC_TYPE मानम्;                             \
            स्मृतिं_प्रतिलिख(&मानम्, src8, परिमाण(मानम्));        \
            M3_BSWAP_##SRC_TYPE(मानम्);                 \
            REG = (DEST_TYPE)मानम्;                     \
            d_m3_Trace_आरोपय(DEST_TYPE, सङ्कार्यम्, REG);     \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 सङ्कार्यम् = slot (u32);                           \
    u32 offset = तात्कालिकम् (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    सङ्कार्यम् += offset;                                  \
                                                        \
    यदि (m3MemCheck(                                     \
        सङ्कार्यम् + परिमाण (SRC_TYPE) <= _mem->दीर्घता     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;       \
            SRC_TYPE मानम्;                             \
            स्मृतिं_प्रतिलिख(&मानम्, src8, परिमाण(मानम्));        \
            M3_BSWAP_##SRC_TYPE(मानम्);                 \
            REG = (DEST_TYPE)मानम्;                     \
            d_m3_Trace_आरोपय(DEST_TYPE, सङ्कार्यम्, REG);     \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#परिभाषय d_m3_आरोपय_i(DEST_TYPE, SRC_TYPE) d_m3_आरोपय(_r0, DEST_TYPE, SRC_TYPE)
#परिभाषय d_m3_आरोपय_f(DEST_TYPE, SRC_TYPE) d_m3_आरोपय(_fp0, DEST_TYPE, SRC_TYPE)

#यदि d_m3HasFloat
d_m3_आरोपय_f(f32, f32);
d_m3_आरोपय_f(f64, f64);
#यदि_अन्तः

d_m3_आरोपय_i(i32, i8);
d_m3_आरोपय_i(i32, u8);
d_m3_आरोपय_i(i32, i16);
d_m3_आरोपय_i(i32, u16);
d_m3_आरोपय_i(i32, i32);

d_m3_आरोपय_i(i64, i8);
d_m3_आरोपय_i(i64, u8);
d_m3_आरोपय_i(i64, i16);
d_m3_आरोपय_i(i64, u16);
d_m3_आरोपय_i(i64, i32);
d_m3_आरोपय_i(i64, u32);
d_m3_आरोपय_i(i64, i64);

#यदि d_m3_Fold_समुच्चयः_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_आरोपय_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = तात्कालिकम् (u32);                       \
      DEST_TYPE * dest = slot_ptr (DEST_TYPE);            \
      d_m3_Preload_अग्र ();                                 \
      u64 सङ्कार्यम् = (u32) _r0;                            \
      सङ्कार्यम् += offset;                                  \
                                                          \
      यदि (m3MemCheck(                                     \
          सङ्कार्यम् + परिमाण (SRC_TYPE) <= _mem->दीर्घता     \
      )) {                                                \
          u8* src8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;           \
          SRC_TYPE मानम्;                                 \
          स्मृतिं_प्रतिलिख(&मानम्, src8, परिमाण(मानम्));            \
          M3_BSWAP_##SRC_TYPE(मानम्);                     \
          DEST_TYPE परिणाम = (DEST_TYPE)मानम्;            \
          * dest = परिणाम;  REG = परिणाम;                 \
          d_m3_Trace_आरोपय(DEST_TYPE, सङ्कार्यम्, परिणाम);      \
          अग्र_Op_Preloaded ();                             \
      } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 सङ्कार्यम् = slot (u32);                           \
      u32 offset = तात्कालिकम् (u32);                       \
      DEST_TYPE * dest = slot_ptr (DEST_TYPE);            \
      d_m3_Preload_अग्र ();                                 \
      सङ्कार्यम् += offset;                                  \
                                                          \
      यदि (m3MemCheck(                                     \
          सङ्कार्यम् + परिमाण (SRC_TYPE) <= _mem->दीर्घता     \
      )) {                                                \
          u8* src8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;           \
          SRC_TYPE मानम्;                                 \
          स्मृतिं_प्रतिलिख(&मानम्, src8, परिमाण(मानम्));            \
          M3_BSWAP_##SRC_TYPE(मानम्);                     \
          DEST_TYPE परिणाम = (DEST_TYPE)मानम्;            \
          * dest = परिणाम;  REG = परिणाम;                 \
          d_m3_Trace_आरोपय(DEST_TYPE, सङ्कार्यम्, परिणाम);      \
          अग्र_Op_Preloaded ();                             \
      } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
  }

d_m3_आरोपय_Fold(_r0, i32, i8)
d_m3_आरोपय_Fold(_r0, i32, u8)
d_m3_आरोपय_Fold(_r0, i32, i16)
d_m3_आरोपय_Fold(_r0, i32, u16)
d_m3_आरोपय_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_आरोपय_Fold(_r0, i64, i8)
d_m3_आरोपय_Fold(_r0, i64, u8)
d_m3_आरोपय_Fold(_r0, i64, i16)
d_m3_आरोपय_Fold(_r0, i64, u16)
d_m3_आरोपय_Fold(_r0, i64, i32)
d_m3_आरोपय_Fold(_r0, i64, u32)
d_m3_आरोपय_Fold(_r0, i64, i64)
#  यदि d_m3HasFloat
d_m3_आरोपय_Fold(_fp0, f32, f32)
d_m3_आरोपय_Fold(_fp0, f64, f64)
#  endif

#यदि_अन्तः // d_m3FoldSetLocal

#परिभाषय d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 सङ्कार्यम् = slot (u32);                           \
    u32 offset = तात्कालिकम् (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    सङ्कार्यम् += offset;                                  \
                                                        \
    यदि (m3MemCheck(                                     \
        सङ्कार्यम् + परिमाण (DEST_TYPE) <= _mem->दीर्घता    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, सङ्कार्यम्, REG);     \
            u8* mem8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            स्मृतिं_प्रतिलिख(mem8, &val, परिमाण(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    ध्रुव SRC_TYPE मानम् = slot (SRC_TYPE);             \
    u64 सङ्कार्यम् = (u32) _r0;                            \
    u32 offset = तात्कालिकम् (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    सङ्कार्यम् += offset;                                  \
                                                        \
    यदि (m3MemCheck(                                     \
        सङ्कार्यम् + परिमाण (DEST_TYPE) <= _mem->दीर्घता    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, सङ्कार्यम्, मानम्);   \
            u8* mem8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;       \
            DEST_TYPE val = (DEST_TYPE) मानम्;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            स्मृतिं_प्रतिलिख(mem8, &val, परिमाण(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    ध्रुव SRC_TYPE मानम् = slot (SRC_TYPE);             \
    u64 सङ्कार्यम् = slot (u32);                           \
    u32 offset = तात्कालिकम् (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    सङ्कार्यम् += offset;                                  \
                                                        \
    यदि (m3MemCheck(                                     \
        सङ्कार्यम् + परिमाण (DEST_TYPE) <= _mem->दीर्घता    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, सङ्कार्यम्, मानम्);   \
            u8* mem8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;       \
            DEST_TYPE val = (DEST_TYPE) मानम्;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            स्मृतिं_प्रतिलिख(mem8, &val, परिमाण(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
}

// both operands can be in regs when storing a float
#परिभाषय d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 सङ्कार्यम् = (u32) _r0;                            \
    u32 offset = तात्कालिकम् (u32);                       \
    d_m3_Preload_अग्र ();                                 \
    सङ्कार्यम् += offset;                                  \
                                                        \
    यदि (m3MemCheck(                                     \
        सङ्कार्यम् + परिमाण (TYPE) <= _mem->दीर्घता         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, सङ्कार्यम्, REG);         \
            u8* mem8 = m3_Mem_दत्त(_mem) + सङ्कार्यम्;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            स्मृतिं_प्रतिलिख(mem8, &val, परिमाण(val));            \
        }                                               \
        अग्र_Op_Preloaded ();                             \
    } अन्यथा d_बहिर्गमनम्_सम्बन्धि_Bounds;                               \
}


#परिभाषय d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#परिभाषय d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#यदि d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#यदि_अन्तः

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#अन्तर्भाव "m3_exec_atomic.h"

#अपरिभाषय m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#यदि d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    वर्ण name[100];
    वाक्यं_प्रतिलिख(name, वाक्ये_अन्विष(i_opcode, "op_") + 3);
    वर्ण* bracket = वाक्ये_अन्विष(name, "(");
    यदि (bracket) {
        *bracket = 0;
    }

    पङ्क्तिं_लिख(name);
    अग्र_Op_प्रत्यक्षम्();
}
#यदि_अन्तः

#यदि d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_लब्धम्(i_operationName);

    अग्र_Op_प्रत्यक्षम्();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_लब्धम्(i_operationName);

    अग्र_Op_प्रत्यक्षम्();
}
#यदि_अन्तः

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_exec_h
