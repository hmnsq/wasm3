//
//  m3_exec.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.


#నిర్వచితం_కాకపోతే m3_exec_h
#నిర్వచించు m3_exec_h

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

#నిర్వచితం_కాకపోతే M3_COMPILE_OPCODES
#  error "Opcodes should only be included in one compilation unit"
#అయితే_ముగింపు

#అన్తర్భావ "m3_math_utils.h"
#అన్తర్భావ "m3_compile.h"
#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_info.h"
#అన్తర్భావ "m3_exec_defs.h"
#అన్తర్భావ "m3_atomic.h"
#అన్తర్భావ "m3_host.h"

#అన్తర్భావ <limits.h>

d_m3BeginExternC

#నిర్వచించు rewrite_op(OP)             * ((శూన్య **) (_pc-1)) = (శూన్య*)(OP)

#నిర్వచించు తక్షణ(TYPE)            * ((TYPE *) _pc++)
#నిర్వచించు skip_తక్షణ(TYPE)       (_pc++)

#నిర్వచించు స్థానం(TYPE)                 * (TYPE *) (_sp + తక్షణ (i32))
#నిర్వచించు స్థానం_ptr(TYPE)             (TYPE *) (_sp + తక్షణ (i32))

// Reads one address, index or length operand from a slot, at the width the
// memory or table it belongs to is addressed by. Either way exactly one slot
// immediate is consumed; only how much of the slot is read differs.
#నిర్వచించు d_m3_Wide_నిర్వాహ్యం(IS64)      ((IS64) ? స్థానం (u64) : (u64) స్థానం (u32))

// Whether a range fits a memory, written so that it holds for operands of any
// magnitude: the subtraction cannot wrap once the start is known to be within
// the memory, where start + length could.
#నిర్వచించు d_m3MemRangeOk(ప్రారంభం_2, నిడివి_2, MEM)  \
   ((ప్రారంభం_2) <= (MEM)->నిడివి and (నిడివి_2) <= (MEM)->నిడివి - (ప్రారంభం_2))


#ఒకవేళ d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName);
#  define తదుపరి_Op()                 M3_MUSTTAIL తిరిగి profileOp (d_m3_Op_All_వాదనలు, __FUNCTION__)
#లేదావేళ d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_operationName);
#  define తదుపరి_Op()                 M3_MUSTTAIL తిరిగి debugOp (d_m3_Op_All_వాదనలు, __FUNCTION__)
#లేకపోతే
#  define తదుపరి_Op()                 తదుపరి_Op_ప్రత్యక్ష()
#అయితే_ముగింపు

// Next-operation preloading. The dispatch load and the indirect jump that consumes it
// sit next to each other, so the jump cannot resolve until the load returns. An
// operation's _pc is final as soon as it has taken its immediates -- the offsets are
// compile-time constants -- so the load can be issued there instead, leaving the
// operation's own work as cover for its latency.
//
// Preloading is dropped in profiling and tracing builds, where dispatch goes through a
// hook instead and the loaded pointer would be the wrong thing to call.
#ఒకవేళ d_m3_Preload_తదుపరి_Op && !(d_m3EnableOpProfiling || d_m3EnableOpTracing)
#  define d_m3_Preload_తదుపరి()        స్థిరరాశి IM3Operation తదుపరి_Op_2 = (IM3Operation) (* _pc)
#  define తదుపరి_Op_Preloaded()        M3_MUSTTAIL తిరిగి తదుపరి_Op_2 (_pc + 1, d_m3_Op_వాదనలు)
#లేకపోతే
#  define d_m3_Preload_తదుపరి()
#  define తదుపరి_Op_Preloaded()        తదుపరి_Op()
#అయితే_ముగింపు

#ఒకవేళ d_m3EnableOpProfiling
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName);
// An operation that leaves by a branch never reaches nextOp, so without this it is
// absent from the profile entirely and its outgoing edges are attributed to whatever
// ran before it. Has to stay one statement: jumpOp appears in brace-less else arms.
#  define jumpOp(PC)               M3_MUSTTAIL తిరిగి profileJumpOp ((pc_t)(PC), d_m3_Op_వాదనలు, __FUNCTION__)
#లేకపోతే
#  define jumpOp(PC)               jump_Op_ప్రత్యక్ష(PC)
#అయితే_ముగింపు

#ఒకవేళ d_m3_రికార్డు_Backtraces
#  define నెట్టు_Backtrace_చట్రం_2()            (నెట్టు_Backtrace_చట్రం (_mem->runtime, _pc - 1))
#  define fill_Backtrace_చట్రం(FUNCTION)    (FillBacktraceFunctionInfo (_mem->runtime, function))

#  define newTrap(err)                    తిరిగి (నెట్టు_Backtrace_చట్రం_2 (), err)
#  define forwardTrap(err)                తిరిగి err
#లేకపోతే
#  define నెట్టు_Backtrace_చట్రం_2()            చేయి {} అయితే (0)
#  define fill_Backtrace_చట్రం(FUNCTION)    చేయి {} అయితే (0)

#  define newTrap(err)                    తిరిగి err
#  define forwardTrap(err)                తిరిగి err
#అయితే_ముగింపు

// Trap before an operation that claims a native frame of its own would drive
// the native C stack past runtime->stackLimit. Reads the current stack pointer
// via a frame-address probe and compares against the low-water mark set by
// d_m3StackLimitEnter. Every op that keeps a frame across an inner dispatch
// probes here first -- op_Call and op_CallIndirect, and also op_Loop and
// op_TryTable, whose frames outlive the body they run. Guarding the frame
// claim rather than the call is what keeps op_ReturnCall bounded: the tail
// call itself adds nothing, but the frames it leaves standing behind it do.
#ఒకవేళ d_m3_Max_Native_దొంతర > 0
#  define d_m3_Check_Native_దొంతర()                                               \
       చేయి {                                                                    \
           శూన్య * m3_హద్దు = m3MemRuntime (_mem)->దొంతర_హద్దు;                  \
           ఒకవేళ (M3_UNLIKELY (m3_హద్దు && m3_Native_దొంతర_Ptr () < m3_హద్దు))      \
               newTrap (m3_Err_trap_దొంతర_అధికప్రవాహం);                              \
       } అయితే (0)
#లేకపోతే
#  define d_m3_Check_Native_దొంతర()           చేయి {} అయితే (0)
#అయితే_ముగింపు


#ఒకవేళ d_m3_Has_దొంతర_Switching

// Calling into compiled code with an explicit register pair, which the replay
// needs and the d_m3OpDefaultArgs spelling cannot express.
#  ఒకవేళ d_m3HasFloat
#    define d_m3_Exp_Reg_వాదనలు(R0, FP0)        (R0), (FP0)
#  లేకపోతే
#    define d_m3_Exp_Reg_వాదనలు(R0, FP0)        (R0)
#  endif

#  ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_వాదనలు (R0, FP0), d_m3_పునాది_Cstr)
#  లేకపోతే
#    define d_m3CallWithRegs(PC, SP, MEM, R0, FP0)                             \
         Call ((PC), (SP), (MEM), d_m3_Exp_Reg_వాదనలు (R0, FP0))
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
స్థిర M3_NOINLINE
m3ret_t రికార్డు_Call_చట్రం (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_జ్ఞాపకం i_జ్ఞాపకం,
                         IM3Function i_function, m3reg_t i_r0
#  ఒకవేళ d_m3HasFloat
                         ,
                         f64 i_fp0
#  endif
)
{
    M3_చట్రం చట్రం;

    చట్రం.రకం = చట్రం_call;
    చట్రం.pc = i_pc;
    చట్రం.sp = i_sp;
    చట్రం.జ్ఞాపకం = i_జ్ఞాపకం;
    చట్రం.call.function = i_function;
    చట్రం.call.r0 = i_r0;
#  ఒకవేళ d_m3HasFloat
    చట్రం.call.fp0 = i_fp0;
#  endif

    తిరిగి Continuation_రికార్డు_చట్రం(i_runtime, &చట్రం);
}

#  define d_m3_రికార్డు_Call_చట్రం(జ్ఞాపకం_2, FUNCTION)                                \
       ఒకవేళ (M3_UNLIKELY (r == m3Err_continuationSuspended)) {                   \
           తిరిగి రికార్డు_Call_చట్రం (m3MemRuntime (_mem), _pc, _sp, (జ్ఞాపకం_2),    \
                                   (FUNCTION), d_m3_Exp_Reg_వాదనలు (_r0, _fp0));    \
       }


// op_Loop and op_TryTable hold native frames across their bodies and nest as
// deeply as the Wasm does, so they keep their frames out of this too.
స్థిర M3_NOINLINE
m3ret_t రికార్డు_Loop_చట్రం (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_జ్ఞాపకం i_జ్ఞాపకం)
{
    M3_చట్రం చట్రం;

    చట్రం.రకం = చట్రం_loop;
    చట్రం.pc = i_pc;
    చట్రం.sp = i_sp;
    చట్రం.జ్ఞాపకం = i_జ్ఞాపకం;

    తిరిగి Continuation_రికార్డు_చట్రం(i_runtime, &చట్రం);
}


#  ఒకవేళ d_m3HasExceptionHandling
స్థిర M3_NOINLINE
m3ret_t రికార్డు_Try_చట్రం (IM3Runtime i_runtime, pc_t i_clauses, m3stack_t i_sp, IM3_జ్ఞాపకం i_జ్ఞాపకం,
                        u32 i_అంకె_Clauses, bool i_handlers_సజీవ)
{
    M3_చట్రం చట్రం;

    చట్రం.రకం = చట్రం_try;
    చట్రం.pc = i_clauses;
    చట్రం.sp = i_sp;
    చట్రం.జ్ఞాపకం = i_జ్ఞాపకం;
    చట్రం.try_.అంకె_Clauses = i_అంకె_Clauses;
    చట్రం.try_.handlers_సజీవ = i_handlers_సజీవ;

    తిరిగి Continuation_రికార్డు_చట్రం(i_runtime, &చట్రం);
}
#  endif


#  ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
స్థిర M3_NOINLINE
m3ret_t రికార్డు_నమోదు_చట్రం (IM3Runtime i_runtime, pc_t i_pc, m3stack_t i_sp, IM3_జ్ఞాపకం i_జ్ఞాపకం,
                          IM3Function i_function)
{
    M3_చట్రం చట్రం;

    చట్రం.రకం = చట్రం_నమోదు;
    చట్రం.pc = i_pc;
    చట్రం.sp = i_sp;
    చట్రం.జ్ఞాపకం = i_జ్ఞాపకం;
    చట్రం.నమోదు_2.function = i_function;

    తిరిగి Continuation_రికార్డు_చట్రం(i_runtime, &చట్రం);
}
#  endif
#లేకపోతే
#  define d_m3_రికార్డు_Call_చట్రం(జ్ఞాపకం_2, FUNCTION)
#అయితే_ముగింపు


#ఒకవేళ d_m3EnableStrace == 1
// Flat trace
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#లేదావేళ d_m3EnableStrace >= 2
// Structured trace
#  define d_m3TracePrepare                    స్థిరరాశి IM3Runtime trace_rt = m3MemRuntime(_mem); (శూన్య) trace_rt;
#  define d_m3TracePrint(fmt, ...)            fprintf(stderr, "%*s" fmt "\n", (trace_rt->call_లోతు)*2, "", ##__VA_ARGS__)
#లేకపోతే
#  define d_m3TracePrepare
#  define d_m3TracePrint(fmt, ...)
#అయితే_ముగింపు

#ఒకవేళ d_m3EnableStrace >= 3
#  define d_m3_Trace_ఎక్కించు(TYPE,offset,val)      d_m3TracePrint("load." #TYPE "  0x%x = %" PRI##TYPE, offset, val)
#  define d_m3TraceStore(TYPE,offset,val)     d_m3TracePrint("store." #TYPE " 0x%x , %" PRI##TYPE, offset, val)
#లేకపోతే
#  define d_m3_Trace_ఎక్కించు(TYPE,offset,val)
#  define d_m3TraceStore(TYPE,offset,val)
#అయితే_ముగింపు

#నిర్వచితమైతే DEBUG
  #నిర్వచించు d_బహిర్గతం_యొక్క_Bounds newTrap (ErrorRuntime (m3_Err_trap_బహిర్గతం_యొక్క_Bounds_జ్ఞాపకం_Access,   \
                        _mem->runtime, "memory size: %zu; access offset: %zu",      \
                        _mem->నిడివి, నిర్వాహ్యం))

#  define d_బహిర్గతం_యొక్క_Bounds_Mem_Op(స్థానభ్రంశం, కొలత_2) newTrap (ErrorRuntime (m3_Err_trap_బహిర్గతం_యొక్క_Bounds_జ్ఞాపకం_Access,   \
                     _mem->runtime, "memory size: %zu; access offset: %zu; size: %u",     \
                     _mem->నిడివి, స్థానభ్రంశం, కొలత_2))
#లేకపోతే
  #నిర్వచించు d_బహిర్గతం_యొక్క_Bounds newTrap (m3_Err_trap_బహిర్గతం_యొక్క_Bounds_జ్ఞాపకం_Access)

#  define d_బహిర్గతం_యొక్క_Bounds_Mem_Op(స్థానభ్రంశం, కొలత_2) newTrap (m3_Err_trap_బహిర్గతం_యొక్క_Bounds_జ్ఞాపకం_Access)

#అయితే_ముగింపు

#ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
d_m3RetSig Call (d_m3OpSig, cstr_t i_operationName)
#లేకపోతే
d_m3RetSig Call (d_m3OpSig)
#అయితే_ముగింపు
{
    m3ret_t possible_trap = m3_Yield();
    ఒకవేళ (M3_UNLIKELY(possible_trap)) {
        తిరిగి possible_trap;
    }

    తదుపరి_Op_ప్రత్యక్ష();
}

#నిర్వచించు d_m3CommutativeOpMacro(RES, REG, TYPE, NAME, OP, ...) \
d_m3Op(TYPE##_##NAME##_rs)                              \
{                                                       \
    TYPE నిర్వాహ్యం = స్థానం (TYPE);                         \
    d_m3_Preload_తదుపరి ();                                 \
    OP((RES), నిర్వాహ్యం, ((TYPE) REG), ##__VA_ARGS__);    \
    తదుపరి_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss)                              \
{                                                       \
    TYPE operand2 = స్థానం (TYPE);                        \
    TYPE operand1 = స్థానం (TYPE);                        \
    d_m3_Preload_తదుపరి ();                                 \
    OP((RES), operand1, operand2, ##__VA_ARGS__);       \
    తదుపరి_Op_Preloaded ();                                 \
}

#నిర్వచించు d_m3OpMacro(RES, REG, TYPE, NAME, OP, ...)      \
d_m3Op(TYPE##_##NAME##_sr)                              \
{                                                       \
    TYPE నిర్వాహ్యం = స్థానం (TYPE);                         \
    d_m3_Preload_తదుపరి ();                                 \
    OP((RES), ((TYPE) REG), నిర్వాహ్యం, ##__VA_ARGS__);    \
    తదుపరి_Op_Preloaded ();                                 \
}                                                       \
d_m3CommutativeOpMacro(RES, REG, TYPE,NAME, OP, ##__VA_ARGS__)

// Accept macros
#నిర్వచించు d_m3CommutativeOpMacro_i(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#నిర్వచించు d_m3OpMacro_i(TYPE, NAME, MACRO, ...)               d_m3OpMacro             ( _r0,  _r0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#నిర్వచించు d_m3CommutativeOpMacro_f(TYPE, NAME, MACRO, ...)    d_m3CommutativeOpMacro  (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)
#నిర్వచించు d_m3OpMacro_f(TYPE, NAME, MACRO, ...)               d_m3OpMacro             (_fp0, _fp0, TYPE, NAME, MACRO, ##__VA_ARGS__)

#నిర్వచించు M3_FUNC(RES, A, B, OP)  (RES) = OP((A), (B))        // Accept functions: res = OP(a,b)
#నిర్వచించు M3_OPER(RES, A, B, OP)  (RES) = ((A) OP (B))        // Accept operators: res = a OP b

// The same two, for a float operation whose NaN result the spec leaves to the host:
// CANON is the canonicalizer for the operation's width (only in effect when d_m3CanonicalNaN is set).
#నిర్వచించు M3_FUNC_A(RES, A, B, OP, CANON)  (RES) = CANON(OP((A), (B)))
#నిర్వచించు M3_OPER_A(RES, A, B, OP, CANON)  (RES) = CANON((A) OP (B))

#నిర్వచించు d_m3CommutativeOpFunc_i(TYPE, NAME, OP)     d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_FUNC, OP)
#నిర్వచించు d_m3OpFunc_i(TYPE, NAME, OP)                d_m3OpMacro_i               (TYPE, NAME, M3_FUNC, OP)
#నిర్వచించు d_m3CommutativeOpFunc_f(TYPE, NAME, OP)     d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_FUNC, OP)
#నిర్వచించు d_m3OpFunc_f(TYPE, NAME, OP)                d_m3OpMacro_f               (TYPE, NAME, M3_FUNC, OP)

#నిర్వచించు d_m3CommutativeOp_i(TYPE, NAME, OP)         d_m3CommutativeOpMacro_i    (TYPE, NAME, M3_OPER, OP)
#నిర్వచించు d_m3Op_i(TYPE, NAME, OP)                    d_m3OpMacro_i               (TYPE, NAME, M3_OPER, OP)
#నిర్వచించు d_m3CommutativeOp_f(TYPE, NAME, OP)         d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER, OP)
#నిర్వచించు d_m3Op_f(TYPE, NAME, OP)                    d_m3OpMacro_f               (TYPE, NAME, M3_OPER, OP)

// ...and the arithmetic ones, which name the canonicalizer for their own width
#నిర్వచించు d_m3CommutativeArithOp_f(TYPE, NAME, OP)    d_m3CommutativeOpMacro_f    (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#నిర్వచించు d_m3ArithOp_f(TYPE, NAME, OP)               d_m3OpMacro_f               (TYPE, NAME, M3_OPER_A, OP, canon_nan_##TYPE)
#నిర్వచించు d_m3ArithOpFunc_f(TYPE, NAME, OP)           d_m3OpMacro_f               (TYPE, NAME, M3_FUNC_A, OP, canon_nan_##TYPE)

// compare needs to be distinct for fp 'cause the result must be _r0
#నిర్వచించు d_m3CompareOp_f(TYPE, NAME, OP)             d_m3OpMacro                 (_r0, _fp0, TYPE, NAME, M3_OPER, OP)
#నిర్వచించు d_m3CommutativeCmpOp_f(TYPE, NAME, OP)      d_m3CommutativeOpMacro      (_r0, _fp0, TYPE, NAME, M3_OPER, OP)


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

#ఒకవేళ d_m3HasFloat
d_m3CommutativeCmpOp_f(f32, Equal,         ==)     d_m3CommutativeCmpOp_f(f64, Equal,         ==)
d_m3CommutativeCmpOp_f(f32, NotEqual,      !=)     d_m3CommutativeCmpOp_f(f64, NotEqual,      !=)
d_m3CompareOp_f(f32, LessThan,             < )     d_m3CompareOp_f(f64, LessThan,             < )
d_m3CompareOp_f(f32, GreaterThan,          > )     d_m3CompareOp_f(f64, GreaterThan,          > )
d_m3CompareOp_f(f32, LessThanOrEqual,      <=)     d_m3CompareOp_f(f64, LessThanOrEqual,      <=)
d_m3CompareOp_f(f32, GreaterThanOrEqual,   >=)     d_m3CompareOp_f(f64, GreaterThanOrEqual,   >=)
#అయితే_ముగింపు

#నిర్వచించు OP_కూడు_32(A,B) (i32)((u32)(A) + (u32)(B))
#నిర్వచించు OP_కూడు_64(A,B) (i64)((u64)(A) + (u64)(B))
#నిర్వచించు OP_SUB_32(A,B) (i32)((u32)(A) - (u32)(B))
#నిర్వచించు OP_SUB_64(A,B) (i64)((u64)(A) - (u64)(B))
#నిర్వచించు OP_MUL_32(A,B) (i32)((u32)(A) * (u32)(B))
#నిర్వచించు OP_MUL_64(A,B) (i64)((u64)(A) * (u64)(B))

d_m3CommutativeOpFunc_i (i32, కూడు,      OP_కూడు_32)  d_m3CommutativeOpFunc_i (i64, కూడు,      OP_కూడు_64)
d_m3CommutativeOpFunc_i (i32, Multiply, OP_MUL_32)  d_m3CommutativeOpFunc_i (i64, Multiply, OP_MUL_64)

d_m3OpFunc_i (i32, వ్యవకలనం,            OP_SUB_32)  d_m3OpFunc_i (i64, వ్యవకలనం,            OP_SUB_64)

#నిర్వచించు OP_SHL_32(X,N) ((X) << ((u32)(N) % 32))
#నిర్వచించు OP_SHL_64(X,N) ((X) << ((u64)(N) % 64))
#నిర్వచించు OP_SHR_32(X,N) ((X) >> ((u32)(N) % 32))
#నిర్వచించు OP_SHR_64(X,N) ((X) >> ((u64)(N) % 64))

d_m3OpFunc_i (u32, జరుపు_ఎడమ,       OP_SHL_32)      d_m3OpFunc_i (u64, జరుపు_ఎడమ,       OP_SHL_64)
d_m3OpFunc_i (i32, జరుపు_కుడి,      OP_SHR_32)      d_m3OpFunc_i (i64, జరుపు_కుడి,      OP_SHR_64)
d_m3OpFunc_i (u32, జరుపు_కుడి,      OP_SHR_32)      d_m3OpFunc_i (u64, జరుపు_కుడి,      OP_SHR_64)

d_m3CommutativeOp_i(u32, And,              &)
d_m3CommutativeOp_i(u32, Or,               |)
d_m3CommutativeOp_i(u32, Xor,              ^)

d_m3CommutativeOp_i(u64, And,              &)
d_m3CommutativeOp_i(u64, Or,               |)
d_m3CommutativeOp_i(u64, Xor,              ^)

#ఒకవేళ d_m3_Fold_సమితి_Local

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
    TYPE నిర్వాహ్యం = స్థానం (TYPE);                         \
    TYPE * dest = స్థానం_ptr (TYPE);                      \
    d_m3_Preload_తదుపరి ();                                 \
    TYPE ఫలితం;                                        \
    OP((ఫలితం), నిర్వాహ్యం, ((TYPE) REG), ##__VA_ARGS__); \
    * dest = ఫలితం;  REG = ఫలితం;                     \
    తదుపరి_Op_Preloaded ();                                 \
}                                                       \
d_m3Op(TYPE##_##NAME##_ss_f)                            \
{                                                       \
    TYPE operand2 = స్థానం (TYPE);                        \
    TYPE operand1 = స్థానం (TYPE);                        \
    TYPE * dest = స్థానం_ptr (TYPE);                      \
    d_m3_Preload_తదుపరి ();                                 \
    TYPE ఫలితం;                                        \
    OP((ఫలితం), operand1, operand2, ##__VA_ARGS__);    \
    * dest = ఫలితం;  REG = ఫలితం;                     \
    తదుపరి_Op_Preloaded ();                                 \
}

#  define d_m3FoldOpMacro(REG, TYPE, NAME, OP, ...)     \
d_m3Op(TYPE##_##NAME##_sr_f)                            \
{                                                       \
    TYPE నిర్వాహ్యం = స్థానం (TYPE);                         \
    TYPE * dest = స్థానం_ptr (TYPE);                      \
    d_m3_Preload_తదుపరి ();                                 \
    TYPE ఫలితం;                                        \
    OP((ఫలితం), ((TYPE) REG), నిర్వాహ్యం, ##__VA_ARGS__); \
    * dest = ఫలితం;  REG = ఫలితం;                     \
    తదుపరి_Op_Preloaded ();                                 \
}                                                       \
d_m3FoldCommutativeOpMacro(REG, TYPE, NAME, OP, ##__VA_ARGS__)

// A commutative operator has no _sr form in its variant table, so the fold set skips
// the _sr_f handler it could never be patched to.
#నిర్వచించు d_m3FoldOp_i(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#నిర్వచించు d_m3FoldOp_f(TYPE, NAME, OP, ...)               d_m3FoldOpMacro             (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)
#నిర్వచించు d_m3FoldCommutativeOp_i(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  ( _r0, TYPE, NAME, OP, ##__VA_ARGS__)
#నిర్వచించు d_m3FoldCommutativeOp_f(TYPE, NAME, OP, ...)    d_m3FoldCommutativeOpMacro  (_fp0, TYPE, NAME, OP, ##__VA_ARGS__)

d_m3FoldCommutativeOp_i(i32, కూడు,          M3_FUNC, OP_కూడు_32)
d_m3FoldOp_i           (i32, వ్యవకలనం,     M3_FUNC, OP_SUB_32)
d_m3FoldCommutativeOp_i(i32, Multiply,     M3_FUNC, OP_MUL_32)
d_m3FoldCommutativeOp_i(u32, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u32, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u32, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u32, జరుపు_ఎడమ,    M3_FUNC, OP_SHL_32)
d_m3FoldOp_i           (i32, జరుపు_కుడి,   M3_FUNC, OP_SHR_32)
d_m3FoldOp_i           (u32, జరుపు_కుడి,   M3_FUNC, OP_SHR_32)

d_m3FoldCommutativeOp_i(i64, కూడు,          M3_FUNC, OP_కూడు_64)
d_m3FoldOp_i           (i64, వ్యవకలనం,     M3_FUNC, OP_SUB_64)
d_m3FoldCommutativeOp_i(i64, Multiply,     M3_FUNC, OP_MUL_64)
d_m3FoldCommutativeOp_i(u64, And,          M3_OPER, &)
d_m3FoldCommutativeOp_i(u64, Or,           M3_OPER, |)
d_m3FoldCommutativeOp_i(u64, Xor,          M3_OPER, ^)
d_m3FoldOp_i           (u64, జరుపు_ఎడమ,    M3_FUNC, OP_SHL_64)
d_m3FoldOp_i           (i64, జరుపు_కుడి,   M3_FUNC, OP_SHR_64)
d_m3FoldOp_i           (u64, జరుపు_కుడి,   M3_FUNC, OP_SHR_64)

#  ఒకవేళ d_m3HasFloat
// Without these a float result round-trips through _fp0 and a separate SetSlot, which
// on float-heavy code is about one dispatch in five.
d_m3FoldCommutativeOp_f(f32, కూడు,          M3_OPER_A, +, canon_nan_f32)
d_m3FoldOp_f           (f32, వ్యవకలనం,     M3_OPER_A, -, canon_nan_f32)
d_m3FoldCommutativeOp_f(f32, Multiply,     M3_OPER_A, *, canon_nan_f32)
d_m3FoldOp_f           (f32, Divide,       M3_OPER_A, /, canon_nan_f32)
d_m3FoldCommutativeOp_f(f64, కూడు,          M3_OPER_A, +, canon_nan_f64)
d_m3FoldOp_f           (f64, వ్యవకలనం,     M3_OPER_A, -, canon_nan_f64)
d_m3FoldCommutativeOp_f(f64, Multiply,     M3_OPER_A, *, canon_nan_f64)
d_m3FoldOp_f           (f64, Divide,       M3_OPER_A, /, canon_nan_f64)
#అయితే_ముగింపు

#అయితే_ముగింపు // d_m3FoldSetLocal

#ఒకవేళ d_m3HasFloat
d_m3CommutativeArithOp_f(f32, కూడు,         +)      d_m3CommutativeArithOp_f(f64, కూడు,         +)
d_m3CommutativeArithOp_f(f32, Multiply,    *)      d_m3CommutativeArithOp_f(f64, Multiply,    *)
d_m3ArithOp_f(f32, వ్యవకలనం,               -)      d_m3ArithOp_f(f64, వ్యవకలనం,               -)
d_m3ArithOp_f(f32, Divide,                 /)      d_m3ArithOp_f(f64, Divide,                 /)
#అయితే_ముగింపు

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

#ఒకవేళ d_m3HasFloat
d_m3ArithOpFunc_f(f32, Min, min_f32);
d_m3ArithOpFunc_f(f32, Max, max_f32);
d_m3ArithOpFunc_f(f64, Min, min_f64);
d_m3ArithOpFunc_f(f64, Max, max_f64);

// copysign is defined bitwise and keeps its operand's payload - see canon_nan_f32
d_m3OpFunc_f(f32, CopySign, copysignf);
d_m3OpFunc_f(f64, CopySign, copysign);
#అయితే_ముగింపు

// Unary operations
// Note: This macro follows the principle of d_m3OpMacro

#నిర్వచించు d_m3UnaryMacro(RES, REG, TYPE, NAME, OP, ...)   \
d_m3Op(TYPE##_##NAME##_r)                           \
{                                                   \
    OP((RES), (TYPE) REG, ##__VA_ARGS__);           \
    తదుపరి_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_s)                           \
{                                                   \
    TYPE నిర్వాహ్యం = స్థానం (TYPE);                     \
    OP((RES), నిర్వాహ్యం, ##__VA_ARGS__);              \
    తదుపరి_Op ();                                      \
}

#నిర్వచించు M3_UNARY(RES, X, OP) (RES) = OP(X)
#నిర్వచించు M3_UNARY_A(RES, X, OP, CANON) (RES) = CANON(OP(X))

#నిర్వచించు d_m3UnaryOp_i(TYPE, NAME, OPERATION)        d_m3UnaryMacro( _r0,  _r0, TYPE, NAME, M3_UNARY, OPERATION)
#నిర్వచించు d_m3UnaryOp_f(TYPE, NAME, OPERATION)        d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY, OPERATION)
#నిర్వచించు d_m3UnaryArithOp_f(TYPE, NAME, OPERATION)   d_m3UnaryMacro(_fp0, _fp0, TYPE, NAME, M3_UNARY_A, OPERATION, canon_nan_##TYPE)

#ఒకవేళ d_m3HasFloat
// abs and neg are defined bitwise and keep their operand's payload - see canon_nan_f32
d_m3UnaryOp_f(f32, Abs,             fabsf);    d_m3UnaryOp_f(f64, Abs,             fabs);
d_m3UnaryArithOp_f(f32, Ceil,       ceilf);    d_m3UnaryArithOp_f(f64, Ceil,       ceil);
d_m3UnaryArithOp_f(f32, Floor,      floorf);   d_m3UnaryArithOp_f(f64, Floor,      floor);
d_m3UnaryArithOp_f(f32, Trunc,      truncf);   d_m3UnaryArithOp_f(f64, Trunc,      trunc);
d_m3UnaryArithOp_f(f32, Sqrt,       sqrtf);    d_m3UnaryArithOp_f(f64, Sqrt,       వర్గమూలం);
d_m3UnaryArithOp_f(f32, సమీప,    rintf);    d_m3UnaryArithOp_f(f64, సమీప,    rint);
#  ఒకవేళ defined(M3_COMPILER_TCC)
d_m3UnaryOp_f(f32, Negate,     m3_negf);       d_m3UnaryOp_f(f64, Negate,     m3_neg);
#  లేకపోతే
d_m3UnaryOp_f(f32, Negate,     -);             d_m3UnaryOp_f(f64, Negate,     -);
#  endif
#అయితే_ముగింపు

#నిర్వచించు OP_EQZ(x) ((x) == 0)

d_m3UnaryOp_i(i32, Equal_గమ్యం_Zero, OP_EQZ)
d_m3UnaryOp_i(i64, Equal_గమ్యం_Zero, OP_EQZ)

// clz(0), ctz(0) results are undefined for rest platforms, fix it
#ఒకవేళ (defined(__i386__) || defined(__x86_64__)) && !(defined(__AVX2__) || (defined(__ABM__) && defined(__BMI__)))
#నిర్వచించు OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#నిర్వచించు OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
// for 64-bit instructions branchless approach more preferable
#నిర్వచించు OP_CLZ_64(x) (__builtin_clzll((x) | (1ULL <<  0)) + OP_EQZ(x))
#నిర్వచించు OP_CTZ_64(x) (__builtin_ctzll((x) | (1ULL << 63)) + OP_EQZ(x))
#లేదావేళ defined(__ppc__) || defined(__ppc64__)
// PowerPC is defined for __builtin_clz(0) and __builtin_ctz(0).
// See (https://github.com/aquynh/capstone/blob/master/MathExtras.h#L99)
#నిర్వచించు OP_CLZ_32(x) __builtin_clz(x)
#నిర్వచించు OP_CTZ_32(x) __builtin_ctz(x)
#నిర్వచించు OP_CLZ_64(x) __builtin_clzll(x)
#నిర్వచించు OP_CTZ_64(x) __builtin_ctzll(x)
#లేకపోతే
#నిర్వచించు OP_CLZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_clz(x))
#నిర్వచించు OP_CTZ_32(x) (M3_UNLIKELY((x) == 0) ? 32 : __builtin_ctz(x))
#నిర్వచించు OP_CLZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_clzll(x))
#నిర్వచించు OP_CTZ_64(x) (M3_UNLIKELY((x) == 0) ? 64 : __builtin_ctzll(x))
#అయితే_ముగింపు

d_m3UnaryOp_i(u32, Clz, OP_CLZ_32)
d_m3UnaryOp_i(u64, Clz, OP_CLZ_64)

d_m3UnaryOp_i(u32, Ctz, OP_CTZ_32)
d_m3UnaryOp_i(u64, Ctz, OP_CTZ_64)

d_m3UnaryOp_i(u32, Popcnt, __builtin_popcount)
d_m3UnaryOp_i(u64, Popcnt, __builtin_popcountll)

#నిర్వచించు OP_WRAP_I64(X) ((X) & 0x00000000ffffffff)

d_m3Op(i32_Wrap_i64_r)
{
    _r0 = OP_WRAP_I64((i64)_r0);
    తదుపరి_Op();
}

d_m3Op(i32_Wrap_i64_s)
{
    i64 నిర్వాహ్యం = స్థానం(i64);
    _r0 = OP_WRAP_I64(నిర్వాహ్యం);
    తదుపరి_Op();
}

// Integer sign extension operations
#నిర్వచించు OP_EXTEND8_S_I32(X)  ((int32_t)(int8_t)(X))
#నిర్వచించు OP_EXTEND16_S_I32(X) ((int32_t)(int16_t)(X))
#నిర్వచించు OP_EXTEND8_S_I64(X)  ((int64_t)(int8_t)(X))
#నిర్వచించు OP_EXTEND16_S_I64(X) ((int64_t)(int16_t)(X))
#నిర్వచించు OP_EXTEND32_S_I64(X) ((int64_t)(int32_t)(X))

d_m3UnaryOp_i(i32, Extend8_s,  OP_EXTEND8_S_I32)
d_m3UnaryOp_i(i32, Extend16_s, OP_EXTEND16_S_I32)
d_m3UnaryOp_i(i64, Extend8_s,  OP_EXTEND8_S_I64)
d_m3UnaryOp_i(i64, Extend16_s, OP_EXTEND16_S_I64)
d_m3UnaryOp_i(i64, Extend32_s, OP_EXTEND32_S_I64)

#ఒకవేళ d_m3HasWideArithmetic

// Wide arithmetic. Every operand is in a slot, the top of the stack first, and
// the last immediate is the slot the low half of the result goes to; the high
// half goes to _r0. That slot may be one an operand was just read from, so it is
// only written once all of them are.
d_m3Op(i64_Add128)
{
    u64  rhs_ఎగువ = స్థానం (u64);
    u64  rhs_దిగువ  = స్థానం (u64);
    u64  lhs_ఎగువ = స్థానం (u64);
    u64  lhs_దిగువ  = స్థానం (u64);
    u64* దిగువ     = స్థానం_ptr (u64);
    d_m3_Preload_తదుపరి();

    u64 కూడిక = lhs_దిగువ + rhs_దిగువ;

    *దిగువ = కూడిక;
    _r0  = (i64) (lhs_ఎగువ + rhs_ఎగువ + (కూడిక < lhs_దిగువ));

    తదుపరి_Op_Preloaded();
}

d_m3Op(i64_Sub128)
{
    u64  rhs_ఎగువ = స్థానం (u64);
    u64  rhs_దిగువ  = స్థానం (u64);
    u64  lhs_ఎగువ = స్థానం (u64);
    u64  lhs_దిగువ  = స్థానం (u64);
    u64* దిగువ     = స్థానం_ptr (u64);
    d_m3_Preload_తదుపరి();

    *దిగువ = lhs_దిగువ - rhs_దిగువ;
    _r0  = (i64) (lhs_ఎగువ - rhs_ఎగువ - (lhs_దిగువ < rhs_దిగువ));

    తదుపరి_Op_Preloaded();
}

d_m3Op(i64_MultiplyWide)
{
    i64  b   = స్థానం (i64);
    i64  a   = స్థానం (i64);
    u64* దిగువ = స్థానం_ptr (u64);
    d_m3_Preload_తదుపరి();

    u64 ఎగువ;
    *దిగువ = mul_wide_i64 (a, b, &ఎగువ);
    _r0  = (i64) ఎగువ;

    తదుపరి_Op_Preloaded();
}

d_m3Op(u64_MultiplyWide)
{
    u64  b   = స్థానం (u64);
    u64  a   = స్థానం (u64);
    u64* దిగువ = స్థానం_ptr (u64);
    d_m3_Preload_తదుపరి();

    u64 ఎగువ;
    *దిగువ = mul_wide_u64 (a, b, &ఎగువ);
    _r0  = (i64) ఎగువ;

    తదుపరి_Op_Preloaded();
}

#అయితే_ముగింపు // d_m3HasWideArithmetic

#నిర్వచించు d_m3TruncMacro(DEST, SRC, TYPE, NAME, FROM, OP, ...)   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_r)                \
{                                                   \
    OP((DEST), (FROM) SRC, ##__VA_ARGS__);          \
    తదుపరి_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_r_s)                \
{                                                   \
    FROM * దొంతర = స్థానం_ptr (FROM);                 \
    OP((DEST), (* దొంతర), ##__VA_ARGS__);           \
    తదుపరి_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_r)                \
{                                                   \
    TYPE * dest = స్థానం_ptr (TYPE);                  \
    OP((* dest), (FROM) SRC, ##__VA_ARGS__);        \
    తదుపరి_Op ();                                      \
}                                                   \
d_m3Op(TYPE##_##NAME##_##FROM##_s_s)                \
{                                                   \
    FROM * దొంతర = స్థానం_ptr (FROM);                 \
    TYPE * dest = స్థానం_ptr (TYPE);                  \
    OP((* dest), (* దొంతర), ##__VA_ARGS__);         \
    తదుపరి_Op ();                                      \
}

#ఒకవేళ d_m3HasFloat
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
#అయితే_ముగింపు

#నిర్వచించు d_m3TypeModifyOp(REG_గమ్యం, REG_FROM, గమ్యం_2, NAME, FROM)  \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    REG_గమ్యం = (గమ్యం_2) ((FROM) REG_FROM);                        \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = స్థానం (FROM);                                \
    REG_గమ్యం = (గమ్యం_2) (from);                                   \
    తదుపరి_Op ();                                              \
}

// Int to int
d_m3TypeModifyOp(_r0, _r0, i64, Extend, i32);
d_m3TypeModifyOp(_r0, _r0, i64, Extend, u32);

// Float to float. Narrowing or widening a NaN produces an arithmetic one, so these
// two go through the canonicalizer where the int conversions have nothing to do.
#నిర్వచించు d_m3FloatModifyOp(గమ్యం_2, NAME, FROM)                   \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_r)                            \
{                                                           \
    _fp0 = canon_nan_##TO ((గమ్యం_2) ((FROM) _fp0));             \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_s)                            \
{                                                           \
    FROM from = స్థానం (FROM);                                \
    _fp0 = canon_nan_##TO ((గమ్యం_2) (from));                    \
    తదుపరి_Op ();                                              \
}

#ఒకవేళ d_m3HasFloat
d_m3FloatModifyOp(f32, Demote, f64);
d_m3FloatModifyOp(f64, Promote, f32);
#అయితే_ముగింపు

#నిర్వచించు d_m3TypeConvertOp(REG_గమ్యం, REG_FROM, గమ్యం_2, NAME, FROM) \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_r_r)                          \
{                                                           \
    REG_గమ్యం = (గమ్యం_2) ((FROM) REG_FROM);                        \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_s_r)                          \
{                                                           \
    స్థానం (గమ్యం_2) = (గమ్యం_2) ((FROM) REG_FROM);                     \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_r_s)                          \
{                                                           \
    FROM from = స్థానం (FROM);                                \
    REG_గమ్యం = (గమ్యం_2) (from);                                   \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_##NAME##_##FROM##_s_s)                          \
{                                                           \
    FROM from = స్థానం (FROM);                                \
    స్థానం (గమ్యం_2) = (గమ్యం_2) (from);                                \
    తదుపరి_Op ();                                              \
}

// Int to float
#ఒకవేళ d_m3HasFloat
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f64, Convert, u64);

d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u32);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, i64);
d_m3TypeConvertOp(_fp0, _r0, f32, Convert, u64);
#అయితే_ముగింపు

#నిర్వచించు d_m3ReinterpretOp(REG, గమ్యం_2, SRC, FROM)               \
d_m3Op(గమ్యం_2##_Reinterpret_##FROM##_r_r)                       \
{                                                           \
    సంఘం { FROM c; గమ్యం_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    REG = u.t;                                              \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_Reinterpret_##FROM##_r_s)                       \
{                                                           \
    సంఘం { FROM c; గమ్యం_2 t; } u;                              \
    u.c = స్థానం (FROM);                                      \
    REG = u.t;                                              \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_Reinterpret_##FROM##_s_r)                       \
{                                                           \
    సంఘం { FROM c; గమ్యం_2 t; } u;                              \
    u.c = (FROM) SRC;                                       \
    స్థానం (గమ్యం_2) = u.t;                                        \
    తదుపరి_Op ();                                              \
}                                                           \
                                                            \
d_m3Op(గమ్యం_2##_Reinterpret_##FROM##_s_s)                       \
{                                                           \
    సంఘం { FROM c; గమ్యం_2 t; } u;                              \
    u.c = స్థానం (FROM);                                      \
    స్థానం (గమ్యం_2) = u.t;                                        \
    తదుపరి_Op ();                                              \
}

#ఒకవేళ d_m3HasFloat
d_m3ReinterpretOp(_r0, i32, _fp0, f32)
d_m3ReinterpretOp(_r0, i64, _fp0, f64)
d_m3ReinterpretOp(_fp0, f32, _r0, i32)
d_m3ReinterpretOp(_fp0, f64, _r0, i64)
#అయితే_ముగింపు

// clang-format on

d_m3Op(GetGlobal_s32)
{
    u32* global = తక్షణ(u32*);
    స్థానం(u32) = *global;                        //  printf ("get global: %p %" PRIi64 "\n", global, *global);

    తదుపరి_Op();
}


d_m3Op(GetGlobal_s64)
{
    u64* global = తక్షణ(u64*);
    స్థానం(u64) = *global;                        // printf ("get global: %p %" PRIi64 "\n", global, *global);

    తదుపరి_Op();
}


d_m3Op(సమితి_Global_i32)
{
    u32* global = తక్షణ(u32*);
    *global = (u32)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    తదుపరి_Op();
}


d_m3Op(సమితి_Global_i64)
{
    u64* global = తక్షణ(u64*);
    *global = (u64)_r0;                         //  printf ("set global: %p %" PRIi64 "\n", global, _r0);

    తదుపరి_Op();
}


#ఒకవేళ d_m3_Has_దొంతర_Switching

d_m3Op(Branch);
d_m3Op(నమోదు_3);
#  ఒకవేళ !d_m3_నమోదు_Keeps_చట్రం
d_m3Op(నమోదు_Suspendable);
#  endif

// The function a body of compiled code belongs to. op_Call is handed the code
// rather than the function, but every body opens with op_Entry and the function
// it enters - past a bridge, where the page ran out before the first op.
స్థిర M3_NOINLINE
IM3Function Function_యొక్క_Compiled_సంకేతం (pc_t i_pc)
{
    ఒకవేళ (not i_pc) {
        తిరిగి ఖాళీ;
    }

    అయితే ((IM3Operation)i_pc[0] == op_Branch) {
        i_pc = (pc_t)i_pc[1];
    }

    IM3Operation నమోదు_2 = (IM3Operation)i_pc[0];

#  ఒకవేళ !d_m3_నమోదు_Keeps_చట్రం
    ఒకవేళ (నమోదు_2 == op_నమోదు_Suspendable) {
        తిరిగి (IM3Function)i_pc[1];
    }
#  endif
    తిరిగి (నమోదు_2 == op_నమోదు) ? (IM3Function)i_pc[1] : ఖాళీ;
}

#అయితే_ముగింపు

d_m3Op(Call)
{
    d_m3_Check_Native_దొంతర();

    pc_t      callPC = తక్షణ(pc_t);
    i32       దొంతర_స్థానభ్రంశం = తక్షణ(i32);
    IM3_జ్ఞాపకం జ్ఞాపకం = m3MemInfo(_mem);

    m3stack_t sp = _sp + దొంతర_స్థానభ్రంశం;

#ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_వాదనలు, d_m3_పునాది_Cstr);
#లేకపోతే
    m3ret_t r = Call(callPC, sp, _mem, d_m3_Op_Default_వాదనలు);
#అయితే_ముగింపు

    _mem = జ్ఞాపకం->mallocated;

    ఒకవేళ (M3_LIKELY(not r)) {
        తదుపరి_Op();
    } లేకపోతే {
        d_m3_రికార్డు_Call_చట్రం(జ్ఞాపకం, Function_యొక్క_Compiled_సంకేతం(callPC));
        నెట్టు_Backtrace_చట్రం_2();
        forwardTrap(r);
    }
}


// call_ref / return_call_ref: like the indirect calls, except the callee comes
// straight off the stack instead of out of a table, so there is no bounds check
// and no index. The type still has to match: a (ref null $t) can hold a
// reference the validator only knows as $t, and null has to trap.
d_m3Op(CallRef)
{
    IM3Function function = స్థానం(IM3Function);
    IM3FuncType type = తక్షణ(IM3FuncType);
    i32         దొంతర_స్థానభ్రంశం = తక్షణ(i32);
    IM3_జ్ఞాపకం   జ్ఞాపకం = m3MemInfo(_mem);

    m3stack_t   sp = _sp + దొంతర_స్థానభ్రంశం;

    m3ret_t     r = m3Err_none;

    ఒకవేళ (M3_LIKELY(function)) {
        ఒకవేళ (M3_LIKELY(type == function->funcType)) {
            ఒకవేళ (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            ఒకవేళ (M3_LIKELY(not r)) {
#ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_వాదనలు, d_m3_పునాది_Cstr);
#లేకపోతే
                r = Call(function->compiled, sp, _mem, d_m3_Op_Default_వాదనలు);
#అయితే_ముగింపు

                _mem = జ్ఞాపకం->mallocated;

                ఒకవేళ (M3_LIKELY(not r)) {
                    తదుపరి_Op_ప్రత్యక్ష();
                } లేకపోతే {
                    d_m3_రికార్డు_Call_చట్రం(జ్ఞాపకం, function);
                    నెట్టు_Backtrace_చట్రం_2();
                    forwardTrap(r);
                }
            }
        } లేకపోతే {
            r = m3_Err_trap_పరోక్ష_Call_Type_Mismatch;
        }
    } లేకపోతే {
        r = m3Err_trapNullFunctionRef;
    }

    ఒకవేళ (M3_UNLIKELY(r)) {
        newTrap(r);
    } లేకపోతే {
        forwardTrap(r);
    }
}


// ref.as_non_null is a null check and nothing else: the value stays put, so
// there is no destination slot, only the trap.
d_m3Op(RefAsNonNull)
{
    IM3Function reference = స్థానం(IM3Function);

    ఒకవేళ (M3_UNLIKELY(not reference)) {
        newTrap(m3Err_trapNullReference);
    }

    తదుపరి_Op();
}


d_m3Op(Call_పరోక్ష)
{
    d_m3_Check_Native_దొంతర();

    M3_పట్టిక*    పట్టిక_2 = తక్షణ(M3_పట్టిక*);
    u64         పట్టిక_సూచీ = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);
    IM3FuncType type = తక్షణ(IM3FuncType);
    i32         దొంతర_స్థానభ్రంశం = తక్షణ(i32);
    IM3_జ్ఞాపకం   జ్ఞాపకం = m3MemInfo(_mem);

    m3stack_t   sp = _sp + దొంతర_స్థానభ్రంశం;

    m3ret_t     r = m3Err_none;

    ఒకవేళ (M3_LIKELY(పట్టిక_సూచీ < పట్టిక_2->కొలత)) {
        IM3Function function = (IM3Function)పట్టిక_2->elements[పట్టిక_సూచీ];

        ఒకవేళ (M3_LIKELY(function)) {
            ఒకవేళ (M3_LIKELY(type == function->funcType)) {
                ఒకవేళ (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                ఒకవేళ (M3_LIKELY(not r)) {
                    // a table may hold functions from another module, and those
                    // run against their own module's memory
                    M3_జ్ఞాపకం_శీర్షిక* calleeMem = Module_జ్ఞాపకం_శీర్షిక(function->module);

#ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_వాదనలు, d_m3_పునాది_Cstr);
#లేకపోతే
                    r = Call(function->compiled, sp, calleeMem, d_m3_Op_Default_వాదనలు);
#అయితే_ముగింపు

                    _mem = జ్ఞాపకం->mallocated;

                    ఒకవేళ (M3_LIKELY(not r)) {
                        తదుపరి_Op_ప్రత్యక్ష();
                    } లేకపోతే {
                        d_m3_రికార్డు_Call_చట్రం(జ్ఞాపకం, function);
                        నెట్టు_Backtrace_చట్రం_2();
                        forwardTrap(r);
                    }
                }
            } లేకపోతే {
                r = m3_Err_trap_పరోక్ష_Call_Type_Mismatch;
            }
        } లేకపోతే {
            r = m3_Err_trap_పట్టిక_Element_Is_Null;
        }
    } లేకపోతే {
        r = m3_Err_trap_పట్టిక_సూచీ_బహిర్గతం_యొక్క_Range;
    }

    ఒకవేళ (M3_UNLIKELY(r)) {
        newTrap(r);
    } లేకపోతే {
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
#నిర్వచించు d_m3_తోక_Call_వాదనలు(RETURNSLOTS, STACKOFFSET, NUMARGSLOTS)                     \
   memmove ((శూన్య *) (_sp + (RETURNSLOTS)), (స్థిరరాశి శూన్య *) (_sp + (STACKOFFSET)),   \
            (size_t) (NUMARGSLOTS) * పరిమాణం (m3slot_t))

d_m3Op(ReturnCall)
{
    pc_t    callPC = తక్షణ(pc_t);
    i32     దొంతర_స్థానభ్రంశం = తక్షణ(i32);
    i32     returnSlots = తక్షణ(i32);
    u32     అంకె_Arg_Slots = తక్షణ(u32);

    m3ret_t possible_trap = m3_Yield();
    ఒకవేళ (M3_UNLIKELY(possible_trap)) {
        newTrap(possible_trap);
    }

    d_m3_తోక_Call_వాదనలు(returnSlots, దొంతర_స్థానభ్రంశం, అంకె_Arg_Slots);

    jump_Op_ప్రత్యక్ష(callPC);
}


d_m3Op(ReturnCallRef)
{
    IM3Function function = స్థానం(IM3Function);
    IM3FuncType type = తక్షణ(IM3FuncType);
    i32         దొంతర_స్థానభ్రంశం = తక్షణ(i32);
    i32         returnSlots = తక్షణ(i32);
    u32         అంకె_Arg_Slots = తక్షణ(u32);

    m3ret_t     r = m3Err_none;

    ఒకవేళ (M3_LIKELY(function)) {
        ఒకవేళ (M3_LIKELY(type == function->funcType)) {
            ఒకవేళ (M3_UNLIKELY(not function->compiled)) {
                r = CompileFunction(function);
            }

            ఒకవేళ (M3_LIKELY(not r)) {
                r = m3_Yield();

                ఒకవేళ (M3_LIKELY(not r)) {
                    d_m3_తోక_Call_వాదనలు(returnSlots, దొంతర_స్థానభ్రంశం, అంకె_Arg_Slots);

                    jump_Op_ప్రత్యక్ష(function->compiled);
                }
            }
        } లేకపోతే {
            r = m3_Err_trap_పరోక్ష_Call_Type_Mismatch;
        }
    } లేకపోతే {
        r = m3Err_trapNullFunctionRef;
    }

    newTrap(r);
}


d_m3Op(Return_Call_పరోక్ష)
{
    M3_పట్టిక*    పట్టిక_2 = తక్షణ(M3_పట్టిక*);
    u64         పట్టిక_సూచీ = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);
    IM3FuncType type = తక్షణ(IM3FuncType);
    i32         దొంతర_స్థానభ్రంశం = తక్షణ(i32);
    i32         returnSlots = తక్షణ(i32);
    u32         అంకె_Arg_Slots = తక్షణ(u32);

    m3ret_t     r = m3Err_none;

    ఒకవేళ (M3_LIKELY(పట్టిక_సూచీ < పట్టిక_2->కొలత)) {
        IM3Function function = (IM3Function)పట్టిక_2->elements[పట్టిక_సూచీ];

        ఒకవేళ (M3_LIKELY(function)) {
            ఒకవేళ (M3_LIKELY(type == function->funcType)) {
                ఒకవేళ (M3_UNLIKELY(not function->compiled)) {
                    r = CompileFunction(function);
                }

                ఒకవేళ (M3_LIKELY(not r)) {
                    r = m3_Yield();

                    ఒకవేళ (M3_LIKELY(not r)) {
                        d_m3_తోక_Call_వాదనలు(returnSlots, దొంతర_స్థానభ్రంశం, అంకె_Arg_Slots);

                        jump_Op_ప్రత్యక్ష(function->compiled);
                    }
                }
            } లేకపోతే {
                r = m3_Err_trap_పరోక్ష_Call_Type_Mismatch;
            }
        } లేకపోతే {
            r = m3_Err_trap_పట్టిక_Element_Is_Null;
        }
    } లేకపోతే {
        r = m3_Err_trap_పట్టిక_సూచీ_బహిర్గతం_యొక్క_Range;
    }

    newTrap(r);
}


d_m3Op(CallRawFunction)
{
    d_m3TracePrepare

    M3ImportContext ctx;

    M3RawCall       call = (M3RawCall)(*_pc++);
    ctx.function = తక్షణ(IM3Function);
    ctx.userdata = తక్షణ(శూన్య*);
    u64* స్థిరరాశి sp = ((u64*)_sp);
    IM3_జ్ఞాపకం  జ్ఞాపకం = m3MemInfo(_mem);

    IM3Runtime runtime = m3MemRuntime(_mem);

#ఒకవేళ d_m3EnableStrace
    IM3FuncType ftype = ctx.function->funcType;

    అక్షరం        outbuff[1024];
    అక్షరం*       outp = outbuff;
    అక్షరం*       oute = outbuff + 1024;

    outp += snprintf(outp, oute - outp, "%s!%s(", ctx.function->import.moduleUtf8, ctx.function->import.క్షేత్రం_Utf8);

    స్థిరరాశి పూర్ణాంకం n_వాదనలు = ftype->అంకె_వాదనలు;
    స్థిరరాశి పూర్ణాంకం nRets = ftype->అంకె_Rets;
    u64*      వాదనలు = sp + nRets;
    కొరకు (పూర్ణాంకం i = 0; i < n_వాదనలు; i++) {
        స్థిరరాశి పూర్ణాంకం type = ftype->types[nRets + i];
        ఎంపిక (type) {
        స్థితి c_m3Type_i32: outp += snprintf(outp, oute - outp, "%" PRIi32, *(i32*)(వాదనలు + i)); ఆపు;
        స్థితి c_m3Type_i64: outp += snprintf(outp, oute - outp, "%" PRIi64, *(i64*)(వాదనలు + i)); ఆపు;
        స్థితి c_m3Type_f32: outp += snprintf(outp, oute - outp, "%" PRIf32, *(f32*)(వాదనలు + i)); ఆపు;
        స్థితి c_m3Type_f64: outp += snprintf(outp, oute - outp, "%" PRIf64, *(f64*)(వాదనలు + i)); ఆపు;
        అప్రమేయ: outp += snprintf(outp, oute - outp, "<type %d>", type); ఆపు;
        }
        ఒకవేళ (i < n_వాదనలు - 1) {
            outp += snprintf(outp, oute - outp, ", ");
        }
    }
    // closed here rather than after the last argument: an import taking none has
    // no last argument to close it
    outp += snprintf(outp, oute - outp, ")");
#  ఒకవేళ d_m3EnableStrace >= 2
    outp += snprintf(outp, oute - outp, " { <native> }");
#  endif
#అయితే_ముగింపు

    // m3_Call uses runtime->stack to set-up initial exported function stack.
    // Reconfigure the stack to enable recursive invocations of m3_Call.
    // I.e. exported/table function can be called from an impoted function.
    శూన్య* దొంతర_backup = runtime->దొంతర;
    runtime->దొంతర = sp;
    // the memory this import was bound to, which is memory 0 unless the host
    // module named another one. _mem itself stays on the caller's memory:
    // the trap path below and the backtrace recorder still read it.
    m3ret_t possible_trap = call(runtime, &ctx, sp, m3_Mem_దత్తాంశం(ctx.function->host_జ్ఞాపకం->mallocated));
    runtime->దొంతర = దొంతర_backup;

#ఒకవేళ d_m3EnableStrace
    ఒకవేళ (M3_UNLIKELY(possible_trap)) {
        d_m3TracePrint("%s -> %s", outbuff, (అక్షరం*)possible_trap);
    } లేకపోతే ఒకవేళ (ftype and ftype->అంకె_Rets) {
        d_m3TracePrint("%s = %s", outbuff, SPrintFunctionRetList(ftype, (m3stack_t)sp));
    } లేకపోతే {
        d_m3TracePrint("%s", outbuff);
    }
#అయితే_ముగింపు

    ఒకవేళ (M3_UNLIKELY(possible_trap)) {
        _mem = జ్ఞాపకం->mallocated;
        // an exception the host threw (m3_ThrowException) is not a trap yet, and
        // may well be caught: it records no frame, as a Wasm throw doesn't
        ఒకవేళ (possible_trap != m3Err_pendingException) {
            నెట్టు_Backtrace_చట్రం_2();
        }
    }
    forwardTrap(possible_trap);
}


d_m3Op(Mem_కొలత)
{
    IM3_జ్ఞాపకం జ్ఞాపకం = m3MemInfo(_mem);

    _r0 = జ్ఞాపకం->అంకె_పుటలు;

    తదుపరి_Op();
}


d_m3Op(MemGrow)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_జ్ఞాపకం  జ్ఞాపకం = m3MemInfo(_mem);

    i32        అంకె_పుటలు_గమ్యం_Grow = (i32)_r0;
    ఒకవేళ (అంకె_పుటలు_గమ్యం_Grow >= 0) {
        _r0 = (m3reg_t)జ్ఞాపకం->అంకె_పుటలు;

        ఒకవేళ (M3_LIKELY(అంకె_పుటలు_గమ్యం_Grow)) {
            u64      required_పుటలు = జ్ఞాపకం->అంకె_పుటలు + (u32)అంకె_పుటలు_గమ్యం_Grow;

            M3_ఫలితం r = Resize_జ్ఞాపకం(runtime, జ్ఞాపకం, required_పుటలు);
            ఒకవేళ (r) {
                _r0 = -1;
            }

            _mem = జ్ఞాపకం->mallocated;
        }
    } లేకపోతే {
        _r0 = -1;
    }

    తదుపరి_Op();
}


#ఒకవేళ d_m3HasMemory64

// memory.grow on a 64-bit memory. The delta is a full u64 rather than a value
// the sign of which can stand in for "too big", and failure answers 2^64-1
// instead of 2^32-1 - which is the same -1 in the register, sized by the slot
// it is written to.
d_m3Op(MemGrow64)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    IM3_జ్ఞాపకం  జ్ఞాపకం = m3MemInfo(_mem);

    u64        అంకె_పుటలు_గమ్యం_Grow = (u64)_r0;
    u64        అంకె_పుటలు = జ్ఞాపకం->అంకె_పుటలు;

    ఒకవేళ (M3_LIKELY(అంకె_పుటలు_గమ్యం_Grow)) {
        // maxPages is never below the current size, so the difference cannot
        // wrap; asking for more than it is a request nothing could satisfy
        ఒకవేళ (అంకె_పుటలు_గమ్యం_Grow <= జ్ఞాపకం->max_పుటలు - అంకె_పుటలు and
            not Resize_జ్ఞాపకం(runtime, జ్ఞాపకం, అంకె_పుటలు + అంకె_పుటలు_గమ్యం_Grow)) {
            _r0 = (m3reg_t)అంకె_పుటలు;
        } లేకపోతే {
            _r0 = -1;
        }

        _mem = జ్ఞాపకం->mallocated;
    } లేకపోతే {
        _r0 = (m3reg_t)అంకె_పుటలు;
    }

    తదుపరి_Op();
}

#అయితే_ముగింపు // d_m3HasMemory64


d_m3Op(MemCopy)
{
    u64 కొలత = (u32)_r0;
    u64 ఆధారం = స్థానం(u32);
    u64 destination = స్థానం(u32);

    ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(destination, కొలత, _mem))) {
        ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(ఆధారం, కొలత, _mem))) {
            u8* dst = m3_Mem_దత్తాంశం(_mem) + destination;
            u8* src = m3_Mem_దత్తాంశం(_mem) + ఆధారం;
            memmove(dst, src, (size_t)కొలత);

            తదుపరి_Op();
        } లేకపోతే {
            d_బహిర్గతం_యొక్క_Bounds_Mem_Op(ఆధారం, కొలత);
        }
    } లేకపోతే {
        d_బహిర్గతం_యొక్క_Bounds_Mem_Op(destination, కొలత);
    }
}


#ఒకవేళ d_m3HasMemory64

// memory.copy within a single 64-bit memory: every operand is a full u64.
d_m3Op(MemCopy64)
{
    u64 కొలత = (u64)_r0;
    u64 ఆధారం = స్థానం(u64);
    u64 destination = స్థానం(u64);

    ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(destination, కొలత, _mem))) {
        ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(ఆధారం, కొలత, _mem))) {
            u8* dst = m3_Mem_దత్తాంశం(_mem) + destination;
            u8* src = m3_Mem_దత్తాంశం(_mem) + ఆధారం;
            memmove(dst, src, (size_t)కొలత);

            తదుపరి_Op();
        } లేకపోతే {
            d_బహిర్గతం_యొక్క_Bounds_Mem_Op(ఆధారం, కొలత);
        }
    } లేకపోతే {
        d_బహిర్గతం_యొక్క_Bounds_Mem_Op(destination, కొలత);
    }
}

#అయితే_ముగింపు // d_m3HasMemory64


// Points the _mem register at another memory: one of the module's own, or
// memory 0 of a module being called into. The compiler emits these in pairs,
// around a single access or a single cross-module call, so _mem is back where
// it belongs before anything that reads it for another purpose - a backtrace,
// the stack-limit check - can run.
d_m3Op(సమితి_జ్ఞాపకం)
{
    IM3_జ్ఞాపకం జ్ఞాపకం = తక్షణ(IM3_జ్ఞాపకం);

    _mem = జ్ఞాపకం->mallocated;

    తదుపరి_Op();
}


#ఒకవేళ d_m3_Has_Multi_జ్ఞాపకం

// memory.copy between two different memories. The pair-of-SetMemory trick only
// reaches one memory at a time, so this one names both outright.
//
// The two need not be addressed the same way, so each operand's width comes
// from the memory it belongs to: bit 0 of the widths immediate for the
// destination, bit 1 for the source. The length follows the narrower of the
// two, which is 64-bit only when both are.
d_m3Op(MemCopy_x)
{
    IM3_జ్ఞాపకం       dest_జ్ఞాపకం = తక్షణ(IM3_జ్ఞాపకం);
    IM3_జ్ఞాపకం       ఆధారం_జ్ఞాపకం = తక్షణ(IM3_జ్ఞాపకం);
    u32             widths = తక్షణ(u32);

    M3_జ్ఞాపకం_శీర్షిక* destMem = dest_జ్ఞాపకం->mallocated;
    M3_జ్ఞాపకం_శీర్షిక* ఆధారం_Mem = ఆధారం_జ్ఞాపకం->mallocated;

    u64             కొలత = (widths == 0x3) ? (u64)_r0 : (u64)(u32)_r0;
    u64             ఆధారం = d_m3_Wide_నిర్వాహ్యం(widths & 0x2);
    u64             destination = d_m3_Wide_నిర్వాహ్యం(widths & 0x1);

    ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(destination, కొలత, destMem))) {
        ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(ఆధారం, కొలత, ఆధారం_Mem))) {
            memmove(m3_Mem_దత్తాంశం(destMem) + destination, m3_Mem_దత్తాంశం(ఆధారం_Mem) + ఆధారం, (size_t)కొలత);

            తదుపరి_Op();
        } లేకపోతే {
            d_బహిర్గతం_యొక్క_Bounds_Mem_Op(ఆధారం, కొలత);
        }
    } లేకపోతే {
        d_బహిర్గతం_యొక్క_Bounds_Mem_Op(destination, కొలత);
    }
}

#అయితే_ముగింపు // d_m3HasMultiMemory


d_m3Op(MemFill)
{
    u32 కొలత = (u32)_r0;
    u32 byte = స్థానం(u32);
    u64 destination = స్థానం(u32);

    ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(destination, కొలత, _mem))) {
        u8* mem8 = m3_Mem_దత్తాంశం(_mem) + destination;
        memset(mem8, (u8)byte, కొలత);
        తదుపరి_Op();
    } లేకపోతే {
        d_బహిర్గతం_యొక్క_Bounds_Mem_Op(destination, కొలత);
    }
}


#ఒకవేళ d_m3HasMemory64

// memory.fill on a 64-bit memory. The byte to write stays an i32 - it is a
// value, not an address - so only the destination and the length widen.
d_m3Op(MemFill64)
{
    u64 కొలత = (u64)_r0;
    u32 byte = స్థానం(u32);
    u64 destination = స్థానం(u64);

    ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(destination, కొలత, _mem))) {
        u8* mem8 = m3_Mem_దత్తాంశం(_mem) + destination;
        memset(mem8, (u8)byte, (size_t)కొలత);
        తదుపరి_Op();
    } లేకపోతే {
        d_బహిర్గతం_యొక్క_Bounds_Mem_Op(destination, కొలత);
    }
}

#అయితే_ముగింపు // d_m3HasMemory64


d_m3Op(MemInit)
{
    M3_దత్తాంశం_Segment* segment = తక్షణ(M3_దత్తాంశం_Segment*);

    u64            కొలత = (u32)_r0;
    u64            ఆధారం = స్థానం(u32);
    u64            destination = స్థానం(u32);

    u64            అందుబాటు = segment->dropped ? 0 : segment->కొలత;

    ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(destination, కొలత, _mem))) {
        ఒకవేళ (M3_LIKELY(ఆధారం <= అందుబాటు and కొలత <= అందుబాటు - ఆధారం)) {
            జ్ఞాపకం_నకలు(m3_Mem_దత్తాంశం(_mem) + destination, segment->దత్తాంశం + ఆధారం, (size_t)కొలత);
            తదుపరి_Op();
        } లేకపోతే {
            d_బహిర్గతం_యొక్క_Bounds_Mem_Op(ఆధారం, కొలత);
        }
    } లేకపోతే {
        d_బహిర్గతం_యొక్క_Bounds_Mem_Op(destination, కొలత);
    }
}


#ఒకవేళ d_m3HasMemory64

// memory.init into a 64-bit memory. The segment is indexed as an i32 whatever
// the memory is, so only the destination widens.
d_m3Op(MemInit64)
{
    M3_దత్తాంశం_Segment* segment = తక్షణ(M3_దత్తాంశం_Segment*);

    u64            కొలత = (u32)_r0;
    u64            ఆధారం = స్థానం(u32);
    u64            destination = స్థానం(u64);

    u64            అందుబాటు = segment->dropped ? 0 : segment->కొలత;

    ఒకవేళ (M3_LIKELY(d_m3MemRangeOk(destination, కొలత, _mem))) {
        ఒకవేళ (M3_LIKELY(ఆధారం <= అందుబాటు and కొలత <= అందుబాటు - ఆధారం)) {
            జ్ఞాపకం_నకలు(m3_Mem_దత్తాంశం(_mem) + destination, segment->దత్తాంశం + ఆధారం, (size_t)కొలత);
            తదుపరి_Op();
        } లేకపోతే {
            d_బహిర్గతం_యొక్క_Bounds_Mem_Op(ఆధారం, కొలత);
        }
    } లేకపోతే {
        d_బహిర్గతం_యొక్క_Bounds_Mem_Op(destination, కొలత);
    }
}

#అయితే_ముగింపు // d_m3HasMemory64


d_m3Op(దత్తాంశం_Drop)
{
    M3_దత్తాంశం_Segment* segment = తక్షణ(M3_దత్తాంశం_Segment*);
    segment->dropped = true;

    తదుపరి_Op();
}


#ఒకవేళ d_m3HasRefTypes

d_m3Op(పట్టిక_Get)
{
    M3_పట్టిక* పట్టిక_2 = తక్షణ(M3_పట్టిక*);
    u64      index = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);

    ఒకవేళ (M3_LIKELY(index < పట్టిక_2->కొలత)) {
        _r0 = (u64)(uintptr_t)పట్టిక_2->elements[index];
        తదుపరి_Op();
    } లేకపోతే {
        newTrap(m3_Err_trap_పట్టిక_బహిర్గతం_యొక్క_Bounds);
    }
}


d_m3Op(పట్టిక_సమితి)
{
    M3_పట్టిక* పట్టిక_2 = తక్షణ(M3_పట్టిక*);
    శూన్య*    విలువ = స్థానం(శూన్య*);
    u64      index = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);

    ఒకవేళ (M3_LIKELY(index < పట్టిక_2->కొలత)) {
        పట్టిక_2->elements[index] = విలువ;
        తదుపరి_Op();
    } లేకపోతే {
        newTrap(m3_Err_trap_పట్టిక_బహిర్గతం_యొక్క_Bounds);
    }
}


d_m3Op(పట్టిక_కొలత)
{
    M3_పట్టిక* పట్టిక_2 = తక్షణ(M3_పట్టిక*);

    _r0 = పట్టిక_2->కొలత;

    తదుపరి_Op();
}


d_m3Op(పట్టిక_Grow)
{
    M3_పట్టిక* పట్టిక_2 = తక్షణ(M3_పట్టిక*);
    u64      delta = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);
    శూన్య*    విలువ = స్థానం(శూన్య*);

    u32      old_కొలత = పట్టిక_2->కొలత;
    u32      max_కొలత = పట్టిక_2->max_కొలత ? పట్టిక_2->max_కొలత : d_m3_Max_Sane_పట్టిక_కొలత;

    // a table64 delta is a whole u64, so the sum is formed only once it is
    // known to fit
    u64      new_కొలత = (delta <= max_కొలత - old_కొలత) ? (u64)old_కొలత + delta
                                                    : (u64)max_కొలత + 1;

    ఒకవేళ (new_కొలత == old_కొలత)         // a zero delta never reallocates, and the table may have no storage yet
    {
        _r0 = old_కొలత;
        తదుపరి_Op();
    }

    IM3Runtime runtime = m3MemRuntime(_mem);
    ఒకవేళ (new_కొలత <= max_కొలత and (not runtime->పట్టిక_Elements_హద్దు or
                                delta <= runtime->పట్టిక_Elements_హద్దు - runtime->పట్టిక_Elements_వాడిన)) {
        శూన్య** elements = m3_ReallocArray(శూన్య*, పట్టిక_2->elements, (size_t)new_కొలత, old_కొలత);

        ఒకవేళ (elements) {
            runtime->పట్టిక_Elements_వాడిన += new_కొలత - old_కొలత;
            పట్టిక_2->elements = elements;
            పట్టిక_2->కొలత = (u32)new_కొలత;

            కొరకు (u32 i = old_కొలత; i < పట్టిక_2->కొలత; ++i) {
                పట్టిక_2->elements[i] = విలువ;
            }

            _r0 = old_కొలత;
            తదుపరి_Op();
        }
    }

    // spec: a failed grow returns -1 - which for a table64 is 2^64-1, and the
    // slot it is pushed to is what decides how much of the register is seen
    _r0 = -1;

    తదుపరి_Op();
}


d_m3Op(పట్టిక_Init)
{
    M3_పట్టిక*          పట్టిక_2 = తక్షణ(M3_పట్టిక*);
    M3ElementSegment* segment = తక్షణ(M3ElementSegment*);
    u32               లెక్క = స్థానం(u32);
    u32               ఆధారం = స్థానం(u32);
    // an element segment is indexed as an i32 whatever the table is; only the
    // destination follows the table's index type
    u64               destination = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);

    u64               అందుబాటు = segment->dropped ? 0 : segment->అంకె_Elements;

    ఒకవేళ (M3_LIKELY(destination <= పట్టిక_2->కొలత and లెక్క <= పట్టిక_2->కొలత - destination and
                  (u64) ఆధారం + లెక్క <= అందుబాటు)) {
        కొరకు (u32 i = 0; i < లెక్క; ++i) {
            పట్టిక_2->elements[destination + i] = segment->resolved[ఆధారం + i];
        }

        తదుపరి_Op();
    } లేకపోతే {
        newTrap(m3_Err_trap_పట్టిక_బహిర్గతం_యొక్క_Bounds);
    }
}


d_m3Op(ElemDrop)
{
    M3ElementSegment* segment = తక్షణ(M3ElementSegment*);
    segment->dropped = true;

    తదుపరి_Op();
}


d_m3Op(పట్టిక_Copy)
{
    M3_పట్టిక* dst = తక్షణ(M3_పట్టిక*);
    M3_పట్టిక* src = తక్షణ(M3_పట్టిక*);
    // the length follows the narrower of the two tables, so it is 64-bit only
    // when both are
    u64      లెక్క = d_m3_Wide_నిర్వాహ్యం(dst->isTable64 and src->isTable64);
    u64      ఆధారం = d_m3_Wide_నిర్వాహ్యం(src->isTable64);
    u64      destination = d_m3_Wide_నిర్వాహ్యం(dst->isTable64);

    ఒకవేళ (M3_LIKELY(destination <= dst->కొలత and లెక్క <= dst->కొలత - destination and
                  ఆధారం <= src->కొలత and లెక్క <= src->కొలత - ఆధారం)) {
        memmove(dst->elements + destination, src->elements + ఆధారం, (size_t)లెక్క * పరిమాణం(శూన్య*));

        తదుపరి_Op();
    } లేకపోతే {
        newTrap(m3_Err_trap_పట్టిక_బహిర్గతం_యొక్క_Bounds);
    }
}


d_m3Op(పట్టిక_Fill)
{
    M3_పట్టిక* పట్టిక_2 = తక్షణ(M3_పట్టిక*);
    u64      లెక్క = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);
    శూన్య*    విలువ = స్థానం(శూన్య*);
    u64      index = d_m3_Wide_నిర్వాహ్యం(పట్టిక_2->isTable64);

    ఒకవేళ (M3_LIKELY(index <= పట్టిక_2->కొలత and లెక్క <= పట్టిక_2->కొలత - index)) {
        కొరకు (u64 i = 0; i < లెక్క; ++i) {
            పట్టిక_2->elements[index + i] = విలువ;
        }

        తదుపరి_Op();
    } లేకపోతే {
        newTrap(m3_Err_trap_పట్టిక_బహిర్గతం_యొక్క_Bounds);
    }
}

#అయితే_ముగింపు // d_m3HasRefTypes


// it's a debate: should the compilation be trigger be the caller or callee page.
// it's a much easier to put it in the caller pager. if it's in the callee, either the entire page
// has be left dangling or it's just a stub that jumps to a newly acquired page.  In Gestalt, I opted
// for the stub approach. Stubbing makes it easier to dynamically free the compilation. You can also
// do both.
d_m3Op(Compile)
{
    rewrite_op(op_Call);

    IM3Function function = తక్షణ(IM3Function);

    m3ret_t     ఫలితం = m3Err_none;

    ఒకవేళ (M3_UNLIKELY(not function->compiled)) { // check to see if function was compiled since this operation was emitted.
        ఫలితం = CompileFunction(function);
    }

    ఒకవేళ (not ఫలితం) {
        // patch up compiled pc and call rewritten op_Call
        *((శూన్య**)--_pc) = (శూన్య*)(function->compiled);
        --_pc;
        తదుపరి_Op_ప్రత్యక్ష();
    }

    newTrap(ఫలితం);
}


// same as op_Compile, for a call site that was emitted as a tail call
d_m3Op(CompileReturnCall)
{
    rewrite_op(op_ReturnCall);

    IM3Function function = తక్షణ(IM3Function);

    m3ret_t     ఫలితం = m3Err_none;

    ఒకవేళ (M3_UNLIKELY(not function->compiled)) {
        ఫలితం = CompileFunction(function);
    }

    ఒకవేళ (not ఫలితం) {
        // patch up compiled pc and call rewritten op_ReturnCall
        *((శూన్య**)--_pc) = (శూన్య*)(function->compiled);
        --_pc;
        తదుపరి_Op_ప్రత్యక్ష();
    }

    newTrap(ఫలితం);
}


#ఒకవేళ d_m3_Has_దొంతర_Switching
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
స్థిర
శూన్య Suspend_Without_ట్యాగ్ (IM3Runtime io_runtime, IM3Continuation io_cont, M3_సురక్షిత_బిందువు_రకం i_రకం,
                        pc_t i_pc, m3stack_t i_sp, m3reg_t i_r0
#  ఒకవేళ d_m3HasFloat
                        ,
                        f64 i_fp0
#  endif
)
{
    io_runtime->suspendRequested = false;

    io_runtime->suspend_ట్యాగ్ = ఖాళీ;
    io_runtime->suspendHandlerCont = ఖాళీ;
    io_runtime->suspendStubPC = ఖాళీ;
    io_runtime->switch_లక్ష్యం = ఖాళీ;
    io_runtime->అంకె_Suspend_Payload = 0;

    io_cont->pc = i_pc;
    io_cont->suspend_బిందువు = (u8)i_రకం;
    io_cont->sp = i_sp;
    io_cont->r0 = i_r0;
#  ఒకవేళ d_m3HasFloat
    io_cont->fp0 = i_fp0;
#  endif

    io_cont->అంకె_Suspend_Results = 0;
}
#అయితే_ముగింపు // d_m3HasStackSwitching


#ఒకవేళ d_m3HasGasMetering

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
    u32        ఖర్చు = తక్షణ(u32);

    runtime->gas_మిగిలిన -= (i64)ఖర్చు;

    ఒకవేళ (M3_UNLIKELY(runtime->gas_మిగిలిన < 0)) {
#  ఒకవేళ d_m3_Has_దొంతర_Switching
        // A runtime that can pause does, at the next pause point: running out
        // is a request to stop, like the host's, and the segment runs on to
        // it. The counter goes below zero by what it takes to get there.
        ఒకవేళ (runtime->isSuspendable and runtime->చురుకైన_Continuation) {
            runtime->suspendRequested = true;
            తదుపరి_Op();
        }
#  endif
        newTrap(m3_Err_trap_బహిర్గతం_యొక్క_Gas);
    }

    తదుపరి_Op();
}

#అయితే_ముగింపు // d_m3HasGasMetering


// Whether a function's frame fits the stack it is about to be entered on
స్థిర అంతఃవరుస
bool నమోదు_సరిపోతుంది (IM3Function i_function, m3stack_t _sp, M3_జ్ఞాపకం_శీర్షిక* _mem)
{
#ఒకవేళ d_m3_Skip_దొంతర_Check
    (శూన్య)i_function;
    (శూన్య)_sp;
    (శూన్య)_mem;
    తిరిగి true;
#లేదావేళ d_m3_Has_దొంతర_Switching
    IM3Continuation _act = (_mem ? m3MemRuntime(_mem)->చురుకైన_Continuation : ఖాళీ);
    శూన్య*           max_దొంతర = (_act && _act->val_దొంతర)
                                 ? (శూన్య*)(_act->val_దొంతర + _act->అంకె_దొంతర_Slots)
                                 : (_mem ? _mem->max_దొంతర : (శూన్య*)~(uintptr_t)0);
    తిరిగి M3_LIKELY((శూన్య*)(_sp + i_function->max_దొంతర_Slots) < max_దొంతర);
#లేకపోతే
    తిరిగి M3_LIKELY((శూన్య*)(_sp + i_function->max_దొంతర_Slots) < _mem->max_దొంతర);
#అయితే_ముగింపు
}

// Sets up a function's frame: its locals zeroed, and its constants copied in
స్థిర అంతఃవరుస
శూన్య Enter_చట్రం (IM3Function i_function, m3stack_t _sp)
{
#ఒకవేళ defined(DEBUG)
    i_function->తగిలినవి++;
#అయితే_ముగింపు
    u8* దొంతర = (u8*)((m3slot_t*)_sp + i_function->అంకె_Ret_And_Arg_Slots);

    memset(దొంతర, 0x0, i_function->అంకె_Local_బైట్లు);
    దొంతర += i_function->అంకె_Local_బైట్లు;

    ఒకవేళ (i_function->constants) {
        జ్ఞాపకం_నకలు(దొంతర, i_function->constants, i_function->అంకె_Constant_బైట్లు);
    }
}


d_m3Op(నమోదు_3)
{
    d_m3ClearRegisters

    d_m3TracePrepare

    IM3Function function = తక్షణ(IM3Function);
#ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
    IM3_జ్ఞాపకం జ్ఞాపకం = m3MemInfo(_mem);
#అయితే_ముగింపు

    ఒకవేళ (నమోదు_సరిపోతుంది(function, _sp, _mem)) {
        Enter_చట్రం(function, _sp);

#ఒకవేళ !d_m3_నమోదు_Keeps_చట్రం
        // there's nothing left to do once the body returns, so hand this native frame
        // over to it. That's what lets op_ReturnCall tail-call in constant native stack.
        తదుపరి_Op_ప్రత్యక్ష();
#లేకపోతే

#  ఒకవేళ d_m3EnableStrace >= 2
        d_m3TracePrint("%s %s {", m3_GetFunctionName(function), SPrintFunctionArgList(function, _sp + function->అంకె_Ret_Slots));
        trace_rt->call_లోతు++;
#  endif

        m3ret_t r = తదుపరి_Op_Impl();

#  ఒకవేళ d_m3EnableStrace >= 2
        trace_rt->call_లోతు--;

        ఒకవేళ (r) {
            d_m3TracePrint("} !trap = %s", (అక్షరం*)r);
        } లేకపోతే ఒకవేళ (Get_Function_అంకె_Returns(function)) {
            d_m3TracePrint("} = %s", SPrintFunctionRetList(function->funcType, _sp));
        } లేకపోతే {
            d_m3TracePrint("}");
        }
#  endif

        ఒకవేళ (M3_UNLIKELY(r)) {
            _mem = జ్ఞాపకం->mallocated;
#  ఒకవేళ d_m3_Has_దొంతర_Switching
            ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                తిరిగి రికార్డు_నమోదు_చట్రం(m3MemRuntime(_mem), _pc, _sp, జ్ఞాపకం, function);
            }
#  endif
            fill_Backtrace_చట్రం();
        }
        forwardTrap(r);
#అయితే_ముగింపు // d_m3EntryKeepsFrame
    } లేకపోతే {
        newTrap(m3_Err_trap_దొంతర_అధికప్రవాహం);
    }
}


d_m3Op(Loop)
{
    d_m3_Check_Native_దొంతర();

    d_m3TracePrepare

    // regs are unused coming into a loop anyway
    // this reduces code size & stack usage
    d_m3ClearRegisters

    m3ret_t   r;

    IM3_జ్ఞాపకం జ్ఞాపకం = m3MemInfo(_mem);

    చేయి {
#ఒకవేళ d_m3EnableStrace >= 3
        d_m3TracePrint("iter {");
        trace_rt->call_లోతు++;
#అయితే_ముగింపు
        r = తదుపరి_Op_Impl();

#ఒకవేళ d_m3EnableStrace >= 3
        trace_rt->call_లోతు--;
        d_m3TracePrint("}");
#అయితే_ముగింపు
        // linear memory pointer needs refreshed here because the block it's looping over
        // can potentially invoke the grow operation.
        _mem = జ్ఞాపకం->mallocated;
    } అయితే (r == _pc);

#ఒకవేళ d_m3_Has_దొంతర_Switching
    // The loop's identity is its own pc, which is also where its body starts,
    // so replaying it needs nothing else. Without this the continue sentinel
    // has no frame left to land in after a resume and gets mistaken for a trap.
    ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        తిరిగి రికార్డు_Loop_చట్రం(m3MemRuntime(_mem), _pc, _sp, జ్ఞాపకం);
    }
#అయితే_ముగింపు

    forwardTrap(r);
}


#ఒకవేళ d_m3HasExceptionHandling

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
d_m3Op(Try_పట్టిక)
{
    d_m3_Check_Native_దొంతర();

    IM3_జ్ఞాపకం  జ్ఞాపకం = m3MemInfo(_mem);
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        అంకె_Clauses = తక్షణ(u32);

    pc_t       clauses = _pc;
    pc_t       body = _pc + 2 * అంకె_Clauses;

    u32        లోతు = ++runtime->try_లోతు;

    m3ret_t    r = jumpOpImpl(body);

    // the body can have grown linear memory out from under us
    _mem = జ్ఞాపకం->mallocated;

    // control is leaving this frame for good, so this try is done either way
    bool isInnermost = (runtime->try_లోతు == లోతు);
    runtime->try_లోతు = లోతు - 1;

#  ఒకవేళ d_m3_Has_దొంతర_Switching
    ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
        తిరిగి రికార్డు_Try_చట్రం(runtime, clauses, _sp, జ్ఞాపకం, అంకె_Clauses, isInnermost);
    }
#  endif

    ఒకవేళ (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
        స్థిరరాశి M3Exception* exception = runtime->pendingException;

        కొరకు (u32 i = 0; i < అంకె_Clauses; ++i) {
            IM3_ట్యాగ్ ట్యాగ్ = (IM3_ట్యాగ్)clauses[2 * i];

            ఒకవేళ (ట్యాగ్ == ఖాళీ or ట్యాగ్ == exception->ట్యాగ్) {
                pc_t handler = (pc_t)clauses[2 * i + 1];

                // an unwind leaves nothing meaningful in the registers
                d_m3ClearRegisters

                jump_Op_ప్రత్యక్ష(handler);
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

    u32          అంకె_వాదనలు = తక్షణ(u32);

    కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
        i32 offset = తక్షణ(i32);
        u32 is64 = తక్షణ(u32);
        u64 విలువ = exception->వాదనలు[i];

        ఒకవేళ (is64) {
            *(u64*)(_sp + offset) = విలువ;
        } లేకపోతే {
            *(u32*)(_sp + offset) = (u32)విలువ;
        }
    }

    i32 ref_స్థానభ్రంశం = తక్షణ(i32);

    ఒకవేళ (ref_స్థానభ్రంశం >= 0) {
        *(M3Exception**)(_sp + ref_స్థానభ్రంశం) = exception;
        exception->reified = true;
    } లేకపోతే ఒకవేళ (not exception->reified) {
        FreeException(runtime, exception);
    }

    తదుపరి_Op();
}


d_m3Op(Throw)
{
    IM3Runtime   runtime = m3MemRuntime(_mem);

    IM3_ట్యాగ్       ట్యాగ్ = తక్షణ(IM3_ట్యాగ్);
    u32          అంకె_వాదనలు = తక్షణ(u32);

    M3Exception* exception = NewException(runtime, ట్యాగ్, అంకె_వాదనలు);

    ఒకవేళ (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_mallocFailed);
    }

    కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
        i32 offset = తక్షణ(i32);
        u32 is64 = తక్షణ(u32);

        exception->వాదనలు[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
    }

    runtime->pendingException = exception;

    తిరిగి m3Err_pendingException;
}


d_m3Op(ThrowRef)
{
    M3Exception* exception = స్థానం(M3Exception*);

    ఒకవేళ (M3_UNLIKELY(not exception)) {
        newTrap(m3Err_trapNullReference);
    }

    m3MemRuntime(_mem)->pendingException = exception;

    తిరిగి m3Err_pendingException;
}


// Emitted where control leaves a try region without leaving op_TryTable's native
// frame: falling out of the block's end, or branching past it. The count is how
// many try regions the exit crosses.
d_m3Op(తీయి_Handlers)
{
    IM3Runtime runtime = m3MemRuntime(_mem);

    u32        లెక్క = తక్షణ(u32);

    runtime->try_లోతు -= లెక్క;

    తదుపరి_Op();
}

#అయితే_ముగింపు // d_m3HasExceptionHandling

#ఒకవేళ d_m3_Has_దొంతర_Switching

d_m3Op(ContNew)
{
    IM3Runtime  runtime = m3MemRuntime(_mem);
    IM3FuncType funcType = తక్షణ(IM3FuncType);
    i32         dst_స్థానం = తక్షణ(i32);
    i32         func_స్థానం = తక్షణ(i32);

    IM3Function func = *(IM3Function*)(_sp + func_స్థానం);
    ఒకవేళ (M3_UNLIKELY(not func)) {
        newTrap(m3Err_trapNullFunctionRef);
    }

    // refused before a record exists, so a trap here leaves nothing behind
    ఒకవేళ (M3_UNLIKELY(not Continuation_Can_Acquire_దొంతర(runtime))) {
        newTrap(m3_Err_continuation_హద్దు_Exceeded);
    }
    IM3Continuation cont = Continuation_New(runtime, funcType, func);
    ఒకవేళ (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_mallocFailed);
    }

    M3_ఫలితం కేటాయింపు = Continuation_Acquire_దొంతర(runtime, cont);
    ఒకవేళ (M3_UNLIKELY(కేటాయింపు)) {
        cont->state = cont_consumed;
        newTrap(కేటాయింపు);
    }
    *(IM3Continuation*)(_sp + dst_స్థానం) = cont;
    తదుపరి_Op();
}


స్థిర
IM3Continuation InnermostSuspended (IM3Continuation i_cont);

d_m3Op(ContBind)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3FuncType     ct1 = తక్షణ(IM3FuncType);
    IM3FuncType     ct2 = తక్షణ(IM3FuncType);
    i32             dst_స్థానం = తక్షణ(i32);
    i32             cont_స్థానం = తక్షణ(i32);
    u32             అంకె_Bound = తక్షణ(u32);

    IM3Continuation srcCont = *(IM3Continuation*)(_sp + cont_స్థానం);
    ఒకవేళ (M3_UNLIKELY(not srcCont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ఒకవేళ (M3_UNLIKELY(srcCont->state == cont_consumed || srcCont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation dstCont = Continuation_New(runtime, ct2, srcCont->నమోదు_Function);
    ఒకవేళ (M3_UNLIKELY(not dstCont)) {
        newTrap(m3Err_mallocFailed);
    }

    // the bound continuation takes over the original's execution state whole,
    // recorded native frames included: binding a suspended continuation must
    // not cost it the frames it has to be resumed through
    dstCont->నమోదు_Function = srcCont->నమోదు_Function;
    dstCont->val_దొంతర = srcCont->val_దొంతర;
    dstCont->అంకె_దొంతర_Slots = srcCont->అంకె_దొంతర_Slots;
    dstCont->చట్రాలు = srcCont->చట్రాలు;
    dstCont->చట్రాలు_Cap = srcCont->చట్రాలు_Cap;
    dstCont->అంకె_చట్రాలు = srcCont->అంకె_చట్రాలు;
    dstCont->sp = srcCont->sp;
    dstCont->pc = srcCont->pc;
    dstCont->suspend_బిందువు = srcCont->suspend_బిందువు;
    dstCont->r0 = srcCont->r0;
#  ఒకవేళ d_m3HasFloat
    dstCont->fp0 = srcCont->fp0;
#  endif
    dstCont->bound_వాదనలు_లెక్క = srcCont->bound_వాదనలు_లెక్క;
    dstCont->state = srcCont->state;
#  ఒకవేళ d_m3HasExceptionHandling
    dstCont->resumeThrow = srcCont->resumeThrow;
    srcCont->resumeThrow = ఖాళీ;
#  endif

    dstCont->అంకె_Suspend_Results = srcCont->అంకె_Suspend_Results;
    కొరకు (u32 j = 0; j < srcCont->అంకె_Suspend_Results; ++j) {
        dstCont->suspend_ఫలితం_Offsets[j] = srcCont->suspend_ఫలితం_Offsets[j];
        dstCont->suspend_ఫలితం_Is64[j] = srcCont->suspend_ఫలితం_Is64[j];
    }

    ఒకవేళ (dstCont->state == cont_allocated) {
        u16 అంకె_Rets = ct1->contFuncType ? Get_Func_Type_అంకె_Results(ct1->contFuncType) : 0;
        కొరకు (u32 i = 0; i < అంకె_Bound; ++i) {
            i32 arg_స్థానం = తక్షణ(i32);
            u32 is64 = తక్షణ(u32);
            u32 లక్ష్యం_Idx = (అంకె_Rets + dstCont->bound_వాదనలు_లెక్క) * c_io_స్థానం_లెక్క;
            ఒకవేళ (is64) {
                *(u64*)(dstCont->val_దొంతర + లక్ష్యం_Idx) = *(u64*)(_sp + arg_స్థానం);
            } లేకపోతే {
                *(u32*)(dstCont->val_దొంతర + లక్ష్యం_Idx) = *(u32*)(_sp + arg_స్థానం);
            }
            dstCont->bound_వాదనలు_లెక్క++;
        }
    } లేకపోతే {
        IM3Continuation inner = InnermostSuspended(dstCont);
        కొరకు (u32 i = 0; i < అంకె_Bound; ++i) {
            i32 arg_స్థానం = తక్షణ(i32);
            u32 is64 = తక్షణ(u32);
            u32 dstIdx = dstCont->bound_వాదనలు_లెక్క;
            ఒకవేళ (dstIdx < inner->అంకె_Suspend_Results) {
                i32 dst_స్థానభ్రంశం = inner->suspend_ఫలితం_Offsets[dstIdx];
                ఒకవేళ (is64) {
                    *(u64*)(inner->sp + dst_స్థానభ్రంశం) = *(u64*)(_sp + arg_స్థానం);
                } లేకపోతే {
                    *(u32*)(inner->sp + dst_స్థానభ్రంశం) = *(u32*)(_sp + arg_స్థానం);
                }
            }
            dstCont->bound_వాదనలు_లెక్క++;
        }
    }

    srcCont->val_దొంతర = ఖాళీ;
    srcCont->చట్రాలు = ఖాళీ;
    srcCont->చట్రాలు_Cap = 0;
    srcCont->అంకె_చట్రాలు = 0;
    srcCont->state = cont_consumed;

    *(IM3Continuation*)(_sp + dst_స్థానం) = dstCont;
    తదుపరి_Op();
}


// Finds the resume whose handler answers this tag, walking out through the
// continuations that resumed us. i_kind picks which sort of handler counts -
// 0 for (on $e $l), 1 for (on $e switch). The spec is explicit that the search
// walks past a handler of the wrong sort even when the tag matches, so the
// kind is part of the match rather than something checked afterwards.
స్థిర
IM3Continuation కనుగొను_Handler (IM3Continuation i_cont, IM3_ట్యాగ్ i_ట్యాగ్, u8 i_రకం, pc_t* o_stubPC)
{
    IM3_ట్యాగ్ query_ట్యాగ్ = (i_ట్యాగ్ and i_ట్యాగ్->resolved) ? i_ట్యాగ్->resolved : i_ట్యాగ్;

    కొరకు (IM3Continuation cont = i_cont; cont; cont = cont->మాతృ) {
        కొరకు (u32 h = 0; h < cont->అంకె_Handlers; ++h) {
            pc_t   నమోదు_2 = cont->handlersPC + (h * 3);
            IM3_ట్యాగ్ handler_ట్యాగ్ = (IM3_ట్యాగ్)(*(నమోదు_2 + 1));
            ఒకవేళ (handler_ట్యాగ్ and handler_ట్యాగ్->resolved) {
                handler_ట్యాగ్ = handler_ట్యాగ్->resolved;
            }

            ఒకవేళ ((u32)(uintptr_t)(*నమోదు_2) == i_రకం and handler_ట్యాగ్ == query_ట్యాగ్) {
                ఒకవేళ (o_stubPC) {
                    *o_stubPC = (pc_t)(*(నమోదు_2 + 2));
                }
                తిరిగి cont;
            }
        }
    }

    తిరిగి ఖాళీ;
}


// Nulls the slots a suspend or switch leaves waiting for whatever resumes it.
// Until something writes them they are part of the suspended frame holding
// nothing, and a snapshot taken meanwhile reads each one as the type it will
// have: a reference there has to be null rather than whatever the slot held.
స్థిర
శూన్య ClearSuspendResults (IM3Continuation io_cont, m3stack_t i_sp)
{
    కొరకు (u32 i = 0; i < io_cont->అంకె_Suspend_Results; ++i) {
        ఒకవేళ (io_cont->suspend_ఫలితం_Is64[i]) {
            *(u64*)(i_sp + io_cont->suspend_ఫలితం_Offsets[i]) = 0;
        } లేకపోతే {
            *(m3slot_t*)(i_sp + io_cont->suspend_ఫలితం_Offsets[i]) = 0;
        }
    }
}


d_m3Op(Suspend)
{
    IM3Runtime      runtime = m3MemRuntime(_mem);
    IM3Continuation cont = runtime->చురుకైన_Continuation;
    ఒకవేళ (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ట్యాగ్);
    }

    IM3_ట్యాగ్ ట్యాగ్ = తక్షణ(IM3_ట్యాగ్);
    u32    అంకె_Params = తక్షణ(u32);

    runtime->అంకె_Suspend_Payload = అంకె_Params;
    కొరకు (u32 i = 0; i < అంకె_Params; ++i) {
        i32 param_స్థానం = తక్షణ(i32);
        u32 is64 = తక్షణ(u32);
        ఒకవేళ (is64) {
            runtime->suspendPayload[i] = *(u64*)(_sp + param_స్థానం);
        } లేకపోతే {
            runtime->suspendPayload[i] = (u64) * (u32*)(_sp + param_స్థానం);
        }
        runtime->suspendPayloadIs64[i] = is64;
    }

    u32 అంకె_Results = తక్షణ(u32);
    cont->అంకె_Suspend_Results = అంకె_Results;
    కొరకు (u32 i = 0; i < అంకె_Results; ++i) {
        cont->suspend_ఫలితం_Offsets[i] = తక్షణ(i32);
        cont->suspend_ఫలితం_Is64[i] = తక్షణ(u32);
    }

    ClearSuspendResults(cont, _sp);

    pc_t            matchedStubPC = ఖాళీ;
    IM3Continuation శోధన_Cont = కనుగొను_Handler(cont, ట్యాగ్, 0, &matchedStubPC);

    ఒకవేళ (M3_UNLIKELY(not శోధన_Cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ట్యాగ్);
    }

    runtime->suspendStubPC = matchedStubPC;
    runtime->suspendHandlerCont = శోధన_Cont;
    runtime->switch_లక్ష్యం = ఖాళీ;
    runtime->suspend_ట్యాగ్ = ట్యాగ్;

    cont->pc = _pc;
    cont->suspend_బిందువు = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  ఒకవేళ d_m3HasFloat
    cont->fp0 = _fp0;
#  endif
    cont->bound_వాదనలు_లెక్క = 0;

    తిరిగి m3Err_continuationSuspended;
}


// ReplayFrames and RunUnderHandlers call each other: a captured resume is a
// frame like any other, and what it stands back up is a continuation with
// frames of its own.
స్థిర
m3ret_t నిరంతరం_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_అంకె_Handlers, M3_జ్ఞాపకం_శీర్షిక** io_mem);
స్థిర
శూన్య TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp,
                              pc_t i_resultsPC, u32 i_అంకె_Results);
స్థిర
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_జ్ఞాపకం_శీర్షిక* _mem);

// Whether the suspension point a replay has come down to is resumed past its
// check. Only the first one reached answers to m3_ResumeRuntime's request: that
// is the one the runtime paused at, and whatever it is, the request is spent.
స్థిర అంతఃవరుస
bool ResumesPastCheck (IM3Runtime io_runtime, IM3Continuation i_cont)
{
    bool pass = io_runtime->resumePastCheck;

    io_runtime->resumePastCheck = false;

    తిరిగి pass and (i_cont->suspend_బిందువు == safepoint_op or i_cont->suspend_బిందువు == safepoint_నమోదు);
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
స్థిర
m3ret_t Replay_చట్రాలు (IM3Continuation i_cont, i32 i_లోతు, M3_జ్ఞాపకం_శీర్షిక* _mem)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    m3ret_t    r = m3Err_none;

#  ఒకవేళ d_m3_Max_Native_దొంతర > 0
    {
        శూన్య* హద్దు = runtime->దొంతర_హద్దు;

        ఒకవేళ (M3_UNLIKELY(హద్దు and m3_Native_దొంతర_Ptr() < హద్దు)) {
            తిరిగి m3_Err_trap_దొంతర_అధికప్రవాహం;
        }
    }
#  endif

    ఒకవేళ (i_లోతు < 0) {
        bool pastCheck = ResumesPastCheck(runtime, i_cont);

#  ఒకవేళ d_m3HasExceptionHandling
        // resume_throw stops here: rather than carrying on from the suspension
        // point, the exception is raised at it and travels back out through the
        // try regions this walk has just rebuilt above
        ఒకవేళ (M3_UNLIKELY(i_cont->resumeThrow)) {
            runtime->pendingException = i_cont->resumeThrow;
            i_cont->resumeThrow = ఖాళీ;

            తిరిగి m3Err_pendingException;
        }
#  endif
        ఒకవేళ (pastCheck) {
            తిరిగి ResumePastCheck(i_cont, _mem);
        }

        తిరిగి d_m3CallWithRegs(i_cont->pc, i_cont->sp, _mem, i_cont->r0, i_cont->fp0);
    }

    M3_చట్రం చట్రం = i_cont->చట్రాలు[i_లోతు];

    // frame_try brackets the recursion with its own bookkeeping, and
    // frame_resume descends into another continuation entirely, so neither
    // shares this one
    bool    ownDescent = ((M3_చట్రం_రకం)చట్రం.రకం == చట్రం_resume);

#  ఒకవేళ d_m3HasExceptionHandling
    ownDescent = ownDescent or ((M3_చట్రం_రకం)చట్రం.రకం == చట్రం_try);
#  endif

    ఒకవేళ (not ownDescent) {
        r = Replay_చట్రాలు(i_cont, i_లోతు - 1, _mem);
        _mem = చట్రం.జ్ఞాపకం->mallocated;
    }

    ఎంపిక ((M3_చట్రం_రకం)చట్రం.రకం) {
    స్థితి చట్రం_call:
        ఒకవేళ (M3_LIKELY(not r)) {
            // the callee returned: carry on in the caller, with the registers
            // op_Call's own frame had been holding across the call
            తిరిగి d_m3CallWithRegs(చట్రం.pc, చట్రం.sp, _mem, చట్రం.call.r0, చట్రం.call.fp0);
        }

        ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            తిరిగి Continuation_రికార్డు_చట్రం(runtime, &చట్రం);
        }
#  ఒకవేళ d_m3_రికార్డు_Backtraces
        నెట్టు_Backtrace_చట్రం(runtime, చట్రం.pc - 1);
#  endif
        తిరిగి r;

    స్థితి చట్రం_loop: {
        m3stack_t _sp = చట్రం.sp;
        m3reg_t   _r0 = 0;
#  ఒకవేళ d_m3HasFloat
        f64 _fp0 = 0.;
#  endif
        // op_Loop's protocol: the body hands its own pc back to ask for
        // another lap. Dispatched the way op_Loop dispatches it, so a yield
        // hook sees the same back edges it always did.
        అయితే (r == (m3ret_t)చట్రం.pc) {
            r = jumpOpImpl(చట్రం.pc);
            _mem = చట్రం.జ్ఞాపకం->mallocated;
        }

        ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            తిరిగి Continuation_రికార్డు_చట్రం(runtime, &చట్రం);
        }
        తిరిగి r;
    }

#  ఒకవేళ d_m3HasExceptionHandling
    స్థితి చట్రం_try: {
        m3stack_t _sp = చట్రం.sp;
        m3reg_t   _r0 = 0;
#    ఒకవేళ d_m3HasFloat
        f64 _fp0 = 0.;
#    endif
        (శూన్య)_sp;
        (శూన్య)_r0;
#    ఒకవేళ d_m3HasFloat
        (శూన్య)_fp0;
#    endif

        // this try region is standing again, so it counts towards the depth
        // once more - unless op_PopHandlers had already retired it before the
        // suspend, in which case it is only here to be walked past
        u32 లోతు = ++runtime->try_లోతు;

        ఒకవేళ (not చట్రం.try_.handlers_సజీవ) {
            runtime->try_లోతు = లోతు - 1;
        }

        r = Replay_చట్రాలు(i_cont, i_లోతు - 1, _mem);
        _mem = చట్రం.జ్ఞాపకం->mallocated;

        bool isInnermost = (runtime->try_లోతు == లోతు);
        runtime->try_లోతు = లోతు - 1;

        ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            చట్రం.try_.handlers_సజీవ = isInnermost;
            తిరిగి Continuation_రికార్డు_చట్రం(runtime, &చట్రం);
        }

        ఒకవేళ (M3_UNLIKELY(r == m3Err_pendingException) and isInnermost) {
            స్థిరరాశి M3Exception* exception = runtime->pendingException;

            కొరకు (u32 i = 0; i < చట్రం.try_.అంకె_Clauses; ++i) {
                IM3_ట్యాగ్ ట్యాగ్ = (IM3_ట్యాగ్)చట్రం.pc[2 * i];

                ఒకవేళ (ట్యాగ్ == ఖాళీ or ట్యాగ్ == exception->ట్యాగ్) {
                    pc_t handler = (pc_t)చట్రం.pc[2 * i + 1];

                    d_m3ClearRegisters

                    // op_TryTable hands its frame to the handler, but this is
                    // not an operation and cannot: the replay stands in for
                    // that frame, so it keeps it and passes the answer on.
                    తిరిగి jumpOpImpl(handler);
                }
            }
        }
        తిరిగి r;
    }
#  endif

    స్థితి చట్రం_resume: {
        // A resume that the suspend passed straight through. Standing it up
        // again means running the continuation it had under it, under the same
        // handlers - and that continuation carries the suspension point, so it
        // is where the rest of the replay happens.
        IM3Continuation inner = చట్రం.resume.cont;

        r = నిరంతరం_Under_Handlers(runtime, &inner, చట్రం.resume.handlersPC,
                             చట్రం.resume.అంకె_Handlers, &_mem);

        ఒకవేళ (r == m3Err_none) {
            inner->state = cont_consumed;

            TakeContinuationResults(inner, చట్రం.sp, చట్రం.resume.resultsPC,
                                    చట్రం.resume.అంకె_Results);
            Continuation_Recycle_దొంతర(runtime, inner);

            తిరిగి d_m3CallWithRegs(చట్రం.pc, చట్రం.sp, _mem, 0, 0.);
        }

        ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
            ఒకవేళ (runtime->suspendHandlerCont != inner) {
                చట్రం.resume.cont = inner;
                తిరిగి Continuation_రికార్డు_చట్రం(runtime, &చట్రం);
            }

            IM3Continuation suspended = Continuation_ForkSuspended(runtime, inner);
            ఒకవేళ (M3_UNLIKELY(not suspended)) {
                తిరిగి m3Err_mallocFailed;
            }
            runtime->suspendedContinuation = suspended;
            ఒకవేళ (runtime->suspendStubPC) {
                తిరిగి d_m3CallWithRegs(runtime->suspendStubPC, చట్రం.sp, _mem, 0, 0.);
            } లేకపోతే {
                తిరిగి m3_Err_trap_Unhandled_Control_ట్యాగ్;
            }
        } లేకపోతే {
            inner->state = cont_consumed;
            Continuation_Recycle_దొంతర(runtime, inner);
        }

        తిరిగి r;
    }

#  ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
    స్థితి చట్రం_నమోదు:
        ఒకవేళ (M3_UNLIKELY(r)) {
            ఒకవేళ (M3_UNLIKELY(r == m3Err_continuationSuspended)) {
                తిరిగి Continuation_రికార్డు_చట్రం(runtime, &చట్రం);
            }
#    ఒకవేళ d_m3_రికార్డు_Backtraces
            FillBacktraceFunctionInfo(runtime, చట్రం.నమోదు_2.function);
#    లేకపోతే
            (శూన్య)చట్రం.నమోదు_2.function;
#    endif
        }
        తిరిగి r;
#  endif
    }

    తిరిగి r;
}


// Where a continuation's i-th incoming value goes. A continuation that has not
// started yet takes them as the arguments of its entry frame, laid out above
// its return slots; one that is suspended takes them in the slots its own
// suspend or switch left waiting.
స్థిర
శూన్య* ContinuationArgDest (IM3Continuation i_cont, u32 i_సూచీ)
{
    ఒకవేళ (i_cont->state == cont_allocated) {
        IM3FuncType inner = i_cont->type ? i_cont->type->contFuncType : ఖాళీ;
        u16         అంకె_Rets = inner ? Get_Func_Type_అంకె_Results(inner) : 0;

        తిరిగి i_cont->val_దొంతర + (అంకె_Rets + i_సూచీ) * c_io_స్థానం_లెక్క;
    }

    ఒకవేళ (i_సూచీ < i_cont->అంకె_Suspend_Results) {
        తిరిగి i_cont->sp + i_cont->suspend_ఫలితం_Offsets[i_సూచీ];
    }

    తిరిగి ఖాళీ;
}


// Hands a switch's payload to the peer, and after it the continuation that
// switched away - the reference that lets the peer come back this way.
స్థిర
శూన్య Hand_Over_గమ్యం_Peer (IM3Runtime i_runtime, IM3Continuation io_peer, IM3Continuation i_from)
{
    u32 అంకె_వాదనలు = i_runtime->అంకె_Suspend_Payload;
    u32 పునాది = (io_peer->state == cont_allocated) ? io_peer->bound_వాదనలు_లెక్క : 0;

    కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
        శూన్య* dest = ContinuationArgDest(io_peer, పునాది + i);

        ఒకవేళ (dest) {
            ఒకవేళ (i_runtime->suspendPayloadIs64[i]) {
                *(u64*)dest = i_runtime->suspendPayload[i];
            } లేకపోతే {
                *(u32*)dest = (u32)i_runtime->suspendPayload[i];
            }
        }
    }

    శూన్య* refDest = ContinuationArgDest(io_peer, పునాది + అంకె_వాదనలు);

    ఒకవేళ (refDest) {
        *(IM3Continuation*)refDest = i_from;
    }

    ఒకవేళ (io_peer->state == cont_allocated) {
        io_peer->bound_వాదనలు_లెక్క += అంకె_వాదనలు + 1;
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
స్థిర
IM3Continuation InnermostSuspended (IM3Continuation i_cont)
{
    అయితే (i_cont->అంకె_చట్రాలు and (M3_చట్రం_రకం) i_cont->చట్రాలు[0].రకం == చట్రం_resume) {
        i_cont = i_cont->చట్రాలు[0].resume.cont;
    }

    తిరిగి i_cont;
}


// Runs a continuation under one handler table until something comes back that
// is not a switch, and reports which continuation was running when it did: a
// switch under this table takes the place of the one that switched away, and
// whatever is running is what the handler table belongs to.
స్థిర
m3ret_t నిరంతరం_Under_Handlers (IM3Runtime i_runtime, IM3Continuation* io_cont,
                          pc_t i_handlersPC, u32 i_అంకె_Handlers, M3_జ్ఞాపకం_శీర్షిక** io_mem)
{
    IM3Continuation cont = *io_cont;
    IM3Continuation మాతృ = i_runtime->చురుకైన_Continuation;
    IM3_జ్ఞాపకం       జ్ఞాపకం = m3MemInfo(*io_mem);
    m3ret_t         r;

    కొరకు (;;) {
#  ఒకవేళ d_m3HasExceptionHandling
        // aborting a continuation that never started: there is no suspension
        // point to raise at, so the exception simply comes straight back out
        ఒకవేళ (M3_UNLIKELY(cont->resumeThrow and cont->అంకె_చట్రాలు == 0)) {
            i_runtime->pendingException = cont->resumeThrow;
            cont->resumeThrow = ఖాళీ;
            cont->state = cont_consumed;
            r = m3Err_pendingException;
            ఆపు;
        }
#  endif
        bool isStarting = (cont->state == cont_allocated);

        ఒకవేళ (isStarting) {
            ఒకవేళ (M3_UNLIKELY(not cont->నమోదు_Function->compiled)) {
                r = CompileFunction(cont->నమోదు_Function);

                ఒకవేళ (M3_UNLIKELY(r)) {
                    ఆపు;
                }
            }
            cont->pc = cont->నమోదు_Function->compiled;
            cont->sp = cont->val_దొంతర;
            cont->అంకె_చట్రాలు = 0;
        }

        cont->handlersPC = i_handlersPC;
        cont->అంకె_Handlers = i_అంకె_Handlers;
        cont->మాతృ = మాతృ;
        cont->state = cont_running;
        cont->bound_వాదనలు_లెక్క = 0;

        i_runtime->చురుకైన_Continuation = cont;

        ఒకవేళ (cont->అంకె_చట్రాలు) {
            // a suspended continuation: put back the native frames it unwound
            // through before landing on the suspension point. The count is
            // cleared first because a second suspend records into the same
            // array, starting over at index 0.
            i32 outermost = (i32)cont->అంకె_చట్రాలు - 1;
            cont->అంకె_చట్రాలు = 0;

            r = Replay_చట్రాలు(cont, outermost, *io_mem);
        } లేకపోతే ఒకవేళ (not isStarting and ResumesPastCheck(i_runtime, cont)) {
            r = ResumePastCheck(cont, *io_mem);
        } లేకపోతే {
            r = d_m3CallWithRegs(cont->pc, cont->sp, *io_mem, 0, 0.);
        }

        *io_mem = జ్ఞాపకం->mallocated;
        i_runtime->చురుకైన_Continuation = మాతృ;

        ఒకవేళ (r != m3Err_continuationSuspended) {
            ఆపు;
        }

        cont->state = cont_suspended;

        // not a switch, so it is a suspend and this is as far as we take it
        ఒకవేళ (not i_runtime->switch_లక్ష్యం or i_runtime->suspendHandlerCont != cont) {
            ఆపు;
        }

        IM3Continuation peer = i_runtime->switch_లక్ష్యం;
        i_runtime->switch_లక్ష్యం = ఖాళీ;

        IM3Continuation suspended = Continuation_ForkSuspended(i_runtime, cont);
        ఒకవేళ (M3_UNLIKELY(not suspended)) {
            r = m3Err_mallocFailed;
            ఆపు;
        }
        IM3FuncType peerType = peer->type->contFuncType;
        suspended->type = Module_Cont_Type_యొక్క_Ref(peer->నమోదు_Function->module,
                                               GetFuncTypeParamType(peerType, (u16)(Get_Func_Type_అంకె_Params(peerType) - 1)));
        Hand_Over_గమ్యం_Peer(i_runtime, peer, suspended);

        cont = peer;
    }

    *io_cont = cont;

    తిరిగి r;
}


// Copies what a finished continuation left in its return slots into the slots
// the resume site set aside for them.
స్థిర
శూన్య TakeContinuationResults (IM3Continuation i_cont, m3stack_t i_sp, pc_t i_resultsPC, u32 i_అంకె_Results)
{
    కొరకు (u32 i = 0; i < i_అంకె_Results; ++i) {
        i32 res_స్థానం = *(i32*)(i_resultsPC + 2 * i);
        u32 is64 = *(u32*)(i_resultsPC + 2 * i + 1);
        u32 srcIdx = i * c_io_స్థానం_లెక్క;

        ఒకవేళ (is64) {
            *(u64*)(i_sp + res_స్థానం) = *(u64*)(i_cont->val_దొంతర + srcIdx);
        } లేకపోతే {
            *(u32*)(i_sp + res_స్థానం) = *(u32*)(i_cont->val_దొంతర + srcIdx);
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
    IM3FuncType     contFuncType = తక్షణ(IM3FuncType);
    i32             cont_స్థానం = తక్షణ(i32);
    u32             బహుళకం = తక్షణ(u32);

    IM3Continuation cont = *(IM3Continuation*)(_sp + cont_స్థానం);
    ఒకవేళ (M3_UNLIKELY(not cont)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ఒకవేళ (M3_UNLIKELY(cont->state == cont_consumed or cont->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3FuncType innerType = contFuncType->contFuncType ? contFuncType->contFuncType : contFuncType;
    u16         అంకె_Rets = Get_Func_Type_అంకె_Results(innerType);

    ఒకవేళ (బహుళకం == d_m3ResumeThrowRef) {
        i32 exn_స్థానం = తక్షణ(i32);

#  ఒకవేళ d_m3HasExceptionHandling
        M3Exception* exception = *(M3Exception**)(_sp + exn_స్థానం);

        ఒకవేళ (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_trapNullReference);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  లేకపోతే
        (శూన్య)exn_స్థానం;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } లేకపోతే ఒకవేళ (బహుళకం == d_m3ResumeThrow) {
        IM3_ట్యాగ్ exn_ట్యాగ్ = తక్షణ(IM3_ట్యాగ్);
        u32    అంకె_వాదనలు = తక్షణ(u32);

#  ఒకవేళ d_m3HasExceptionHandling
        M3Exception* exception = NewException(runtime, exn_ట్యాగ్, అంకె_వాదనలు);

        ఒకవేళ (M3_UNLIKELY(not exception)) {
            newTrap(m3Err_mallocFailed);
        }

        కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
            i32 offset = తక్షణ(i32);
            u32 is64 = తక్షణ(u32);

            exception->వాదనలు[i] = is64 ? *(u64*)(_sp + offset) : *(u32*)(_sp + offset);
        }

        InnermostSuspended(cont)->resumeThrow = exception;
#  లేకపోతే
        (శూన్య)exn_ట్యాగ్;
        (శూన్య)అంకె_వాదనలు;
        newTrap(m3Err_trapUnsupportedInstruction);
#  endif
    } లేకపోతే {
        u32 అంకె_వాదనలు = తక్షణ(u32);

        ఒకవేళ (cont->state == cont_allocated) {
            కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
                i32 arg_స్థానం = తక్షణ(i32);
                u32 is64 = తక్షణ(u32);
                u32 లక్ష్యం_Idx = (అంకె_Rets + cont->bound_వాదనలు_లెక్క + i) * c_io_స్థానం_లెక్క;
                ఒకవేళ (is64) {
                    *(u64*)(cont->val_దొంతర + లక్ష్యం_Idx) = *(u64*)(_sp + arg_స్థానం);
                } లేకపోతే {
                    *(u32*)(cont->val_దొంతర + లక్ష్యం_Idx) = *(u32*)(_sp + arg_స్థానం);
                }
            }
            cont->bound_వాదనలు_లెక్క = 0;
        } లేకపోతే {
            // the waiting slots belong to whichever continuation holds the
            // suspension point, which is not this one if a resume was captured
            // along with it
            IM3Continuation inner = InnermostSuspended(cont);

            కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
                i32 arg_స్థానం = తక్షణ(i32);
                u32 is64 = తక్షణ(u32);
                u32 dstIdx = cont->bound_వాదనలు_లెక్క + i;
                ఒకవేళ (dstIdx < inner->అంకె_Suspend_Results) {
                    i32 dst_స్థానభ్రంశం = inner->suspend_ఫలితం_Offsets[dstIdx];
                    ఒకవేళ (is64) {
                        *(u64*)(inner->sp + dst_స్థానభ్రంశం) = *(u64*)(_sp + arg_స్థానం);
                    } లేకపోతే {
                        *(u32*)(inner->sp + dst_స్థానభ్రంశం) = *(u32*)(_sp + arg_స్థానం);
                    }
                }
            }
        }
    }

    u32  అంకె_Handlers = తక్షణ(u32);
    pc_t handlersPC = _pc;
    _pc += అంకె_Handlers * 3;

    u32       అంకె_Results = తక్షణ(u32);
    pc_t      resultsPC = _pc;

    IM3_జ్ఞాపకం జ్ఞాపకం = m3MemInfo(_mem);

    m3ret_t   r = నిరంతరం_Under_Handlers(runtime, &cont, handlersPC, అంకె_Handlers, &_mem);

    ఒకవేళ (r == m3Err_none) {
        cont->state = cont_consumed;

        TakeContinuationResults(cont, _sp, resultsPC, అంకె_Results);
        Continuation_Recycle_దొంతర(runtime, cont);

        _pc += అంకె_Results * 2;
        తదుపరి_Op();
    } లేకపోతే ఒకవేళ (r == m3Err_continuationSuspended) {
        ఒకవేళ (M3_UNLIKELY(runtime->suspendHandlerCont != cont)) {
            ఒకవేళ (M3_UNLIKELY(not runtime->చురుకైన_Continuation)) {
                newTrap(m3_Err_trap_Unhandled_Control_ట్యాగ్);
            }
            M3_చట్రం చట్రం;
            చట్రం.రకం = చట్రం_resume;
            చట్రం.pc = resultsPC + అంకె_Results * 2;
            చట్రం.sp = _sp;
            చట్రం.జ్ఞాపకం = జ్ఞాపకం;
            చట్రం.resume.cont = cont;
            చట్రం.resume.handlersPC = handlersPC;
            చట్రం.resume.అంకె_Handlers = అంకె_Handlers;
            చట్రం.resume.resultsPC = resultsPC;
            చట్రం.resume.అంకె_Results = అంకె_Results;

            తిరిగి Continuation_రికార్డు_చట్రం(runtime, &చట్రం);
        }

        IM3Continuation suspended = Continuation_ForkSuspended(runtime, cont);
        ఒకవేళ (M3_UNLIKELY(not suspended)) {
            newTrap(m3Err_mallocFailed);
        }
        runtime->suspendedContinuation = suspended;

        ఒకవేళ (runtime->suspendStubPC) {
            jump_Op_ప్రత్యక్ష(runtime->suspendStubPC);
        } లేకపోతే {
            newTrap(m3_Err_trap_Unhandled_Control_ట్యాగ్);
        }
    } లేకపోతే {
        cont->state = cont_consumed;
        Continuation_Recycle_దొంతర(runtime, cont);
        నెట్టు_Backtrace_చట్రం_2();
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
    cont->type = తక్షణ(IM3FuncType);

    u32 అంకె_వాదనలు = తక్షణ(u32);
    కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
        i32 offset = తక్షణ(i32);
        u32 is64 = తక్షణ(u32);
        u64 val = (i < runtime->అంకె_Suspend_Payload) ? runtime->suspendPayload[i] : 0;
        ఒకవేళ (is64) {
            *(u64*)(_sp + offset) = val;
        } లేకపోతే {
            *(u32*)(_sp + offset) = (u32)val;
        }
    }

    i32 cont_స్థానభ్రంశం = తక్షణ(i32);
    ఒకవేళ (cont_స్థానభ్రంశం >= 0) {
        *(IM3Continuation*)(_sp + cont_స్థానభ్రంశం) = cont;
    }

    తదుపరి_Op();
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
    IM3Continuation cont = runtime->చురుకైన_Continuation;

    ఒకవేళ (M3_UNLIKELY(not cont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ట్యాగ్);
    }

    skip_తక్షణ(IM3FuncType);

    IM3_ట్యాగ్          ట్యాగ్ = తక్షణ(IM3_ట్యాగ్);
    i32             లక్ష్యం_స్థానం = తక్షణ(i32);

    IM3Continuation లక్ష్యం = *(IM3Continuation*)(_sp + లక్ష్యం_స్థానం);

    ఒకవేళ (M3_UNLIKELY(not లక్ష్యం)) {
        newTrap(m3Err_trapNullContinuationRef);
    }
    ఒకవేళ (M3_UNLIKELY(లక్ష్యం->state == cont_consumed or లక్ష్యం->state == cont_running)) {
        newTrap(m3Err_trapContinuationConsumed);
    }

    IM3Continuation handlerCont = కనుగొను_Handler(cont, ట్యాగ్, 1, ఖాళీ);

    ఒకవేళ (M3_UNLIKELY(not handlerCont)) {
        newTrap(m3_Err_trap_Unhandled_Control_ట్యాగ్);
    }

    // the peer's arguments, and after them the room for the continuation
    // reference the resume will write once this one is safely suspended
    u32 అంకె_వాదనలు = తక్షణ(u32);

    runtime->అంకె_Suspend_Payload = అంకె_వాదనలు;

    కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
        i32 arg_స్థానం = తక్షణ(i32);
        u32 is64 = తక్షణ(u32);

        runtime->suspendPayload[i] = is64 ? *(u64*)(_sp + arg_స్థానం) : (u64) * (u32*)(_sp + arg_స్థానం);
        runtime->suspendPayloadIs64[i] = is64;
    }

    // where the values come back to, when something switches into this one
    u32 అంకె_Results = తక్షణ(u32);

    cont->అంకె_Suspend_Results = అంకె_Results;

    కొరకు (u32 i = 0; i < అంకె_Results; ++i) {
        cont->suspend_ఫలితం_Offsets[i] = తక్షణ(i32);
        cont->suspend_ఫలితం_Is64[i] = తక్షణ(u32);
    }

    ClearSuspendResults(cont, _sp);

    runtime->suspendStubPC = ఖాళీ;
    runtime->suspendHandlerCont = handlerCont;
    runtime->switch_లక్ష్యం = లక్ష్యం;
    runtime->suspend_ట్యాగ్ = ట్యాగ్;

    cont->pc = _pc;
    cont->suspend_బిందువు = safepoint_suspend;
    cont->sp = _sp;
    cont->r0 = _r0;
#  ఒకవేళ d_m3HasFloat
    cont->fp0 = _fp0;
#  endif

    తిరిగి m3Err_continuationSuspended;
}


#అయితే_ముగింపు // d_m3HasStackSwitching


d_m3Op(Branch)
{
    jumpOp(*_pc);
}


d_m3Op(If_r)
{
    i32  condition = (i32)_r0;

    pc_t elsePC = తక్షణ(pc_t);

    ఒకవేళ (condition) {
        తదుపరి_Op();
    } లేకపోతే {
        jumpOp(elsePC);
    }
}


d_m3Op(If_s)
{
    i32  condition = స్థానం(i32);

    pc_t elsePC = తక్షణ(pc_t);

    ఒకవేళ (condition) {
        తదుపరి_Op();
    } లేకపోతే {
        jumpOp(elsePC);
    }
}


d_m3Op(Branch_పట్టిక)
{
    u32   branch_సూచీ = స్థానం(u32);           // branch index is always in a slot
    u32   అంకె_Targets = తక్షణ(u32);

    pc_t* branches = (pc_t*)_pc;

    ఒకవేళ (branch_సూచీ > అంకె_Targets) {
        branch_సూచీ = అంకె_Targets; // the default index
    }

    jumpOp(branches[branch_సూచీ]);
}


#నిర్వచించు d_m3_సమితి_రిజిస్టరు_సమితి_స్థానం(TYPE, REG) \
d_m3Op  (సమితి_రిజిస్టరు_2##TYPE)            \
{                                       \
    TYPE * src = స్థానం_ptr (TYPE);       \
    d_m3_Preload_తదుపరి ();                 \
    REG = * src;                        \
    తదుపరి_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (సమితి_స్థానం_2##TYPE)                 \
{                                       \
    TYPE * dst = స్థానం_ptr (TYPE);       \
    d_m3_Preload_తదుపరి ();                 \
    * dst = (TYPE) REG;                 \
    తదుపరి_Op_Preloaded ();                 \
}                                       \
                                        \
d_m3Op (Preserve_సమితి_స్థానం_2##TYPE)         \
{                                       \
    TYPE * దొంతర     = స్థానం_ptr (TYPE); \
    TYPE * preserve  = స్థానం_ptr (TYPE); \
    d_m3_Preload_తదుపరి ();                 \
                                        \
    * preserve = * దొంతర;               \
    * దొంతర = (TYPE) REG;               \
                                        \
    తదుపరి_Op_Preloaded ();                 \
}

d_m3_సమితి_రిజిస్టరు_సమితి_స్థానం(i32, _r0)
d_m3_సమితి_రిజిస్టరు_సమితి_స్థానం(i64, _r0)
#ఒకవేళ d_m3HasFloat
d_m3_సమితి_రిజిస్టరు_సమితి_స్థానం(f32, _fp0)
d_m3_సమితి_రిజిస్టరు_సమితి_స్థానం(f64, _fp0)
#అయితే_ముగింపు

d_m3Op(Copy_స్థానం_32)
{
    u32* dst = స్థానం_ptr(u32);
    u32* src = స్థానం_ptr(u32);
    d_m3_Preload_తదుపరి();

    *dst = *src;

    తదుపరి_Op_Preloaded();
}


d_m3Op(Preserve_Copy_స్థానం_32)
{
    u32* dest = స్థానం_ptr(u32);
    u32* src = స్థానం_ptr(u32);
    u32* preserve = స్థానం_ptr(u32);
    d_m3_Preload_తదుపరి();

    *preserve = *dest;
    *dest = *src;

    తదుపరి_Op_Preloaded();
}


d_m3Op(Copy_స్థానం_64)
{
    u64* dst = స్థానం_ptr(u64);
    u64* src = స్థానం_ptr(u64);
    d_m3_Preload_తదుపరి();

    *dst = *src;                  // printf ("copy: %p <- %" PRIi64 " <- %p\n", dst, * dst, src);

    తదుపరి_Op_Preloaded();
}


d_m3Op(Preserve_Copy_స్థానం_64)
{
    u64* dest = స్థానం_ptr(u64);
    u64* src = స్థానం_ptr(u64);
    u64* preserve = స్థానం_ptr(u64);
    d_m3_Preload_తదుపరి();

    *preserve = *dest;
    *dest = *src;

    తదుపరి_Op_Preloaded();
}


#ఒకవేళ d_m3EnableOpTracing
//--------------------------------------------------------------------------------------------------------
d_m3Op(Dump_దొంతర)
{
    u32         opcode_సూచీ = తక్షణ(u32);
    u32         దొంతర_ఎత్తు = తక్షణ(u32);
    IM3Function function = తక్షణ(IM3Function);

    cstr_t      funcName = (function) ? m3_GetFunctionName(function) : "";

    రాయి(" %4d ", opcode_సూచీ);
    రాయి(" %-25s     r0: 0x%016" PRIx64 "  i:%" PRIi64 "  u:%" PRIu64 "\n", funcName, _r0, _r0, _r0);
#  ఒకవేళ d_m3HasFloat
    రాయి("                                    fp0: %" PRIf64 "\n", _fp0);
#  endif
    m3stack_t sp = _sp;

    కొరకు (u32 i = 0; i < దొంతర_ఎత్తు; ++i) {
        cstr_t రకం = "";

        రాయి("%p  %5s  %2d: 0x%" PRIx64 "  i:%" PRIi64 "\n", sp, రకం, i, (u64) * (sp), (i64) * (sp));

        ++sp;
    }
    రాయి("---------------------------------------------------------------------------------------------------------\n");

    తదుపరి_Op_ప్రత్యక్ష();
}
#అయితే_ముగింపు


#నిర్వచించు d_m3Select_i(TYPE, REG)                 \
d_m3Op  (Select_##TYPE##_rss)                   \
{                                               \
    i32 condition = (i32) _r0;                  \
                                                \
    TYPE operand2 = స్థానం (TYPE);                \
    TYPE operand1 = స్థానం (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    తదుపరి_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_srs)                   \
{                                               \
    i32 condition = స్థానం (i32);                 \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = స్థానం (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    తదుపరి_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_ssr)                   \
{                                               \
    i32 condition = స్థానం (i32);                 \
                                                \
    TYPE operand2 = స్థానం (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    తదుపరి_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_sss)                   \
{                                               \
    i32 condition = స్థానం (i32);                 \
                                                \
    TYPE operand2 = స్థానం (TYPE);                \
    TYPE operand1 = స్థానం (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    తదుపరి_Op ();                                  \
}


d_m3Select_i(i32, _r0)
d_m3Select_i(i64, _r0)


#నిర్వచించు d_m3Select_f(TYPE, REG, LABEL, SELECTOR)  \
d_m3Op  (Select_##TYPE##_##LABEL##ss)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = స్థానం (TYPE);                \
    TYPE operand1 = స్థానం (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    తదుపరి_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##rs)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = (TYPE) REG;                 \
    TYPE operand1 = స్థానం (TYPE);                \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    తదుపరి_Op ();                                  \
}                                               \
                                                \
d_m3Op  (Select_##TYPE##_##LABEL##sr)           \
{                                               \
    i32 condition = (i32) SELECTOR;             \
                                                \
    TYPE operand2 = స్థానం (TYPE);                \
    TYPE operand1 = (TYPE) REG;                 \
                                                \
    REG = (condition) ? operand1 : operand2;    \
                                                \
    తదుపరి_Op ();                                  \
}

#ఒకవేళ d_m3HasFloat
d_m3Select_f(f32, _fp0, r, _r0)
d_m3Select_f(f32, _fp0, s, స్థానం(i32))

d_m3Select_f(f64, _fp0, r, _r0)
d_m3Select_f(f64, _fp0, s, స్థానం(i32))
#అయితే_ముగింపు

d_m3Op(Return)
{
    m3_దొంతర_Check();
    తిరిగి m3Err_none;
}


d_m3Op(BranchIf_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = తక్షణ(pc_t);

    ఒకవేళ (condition) {
        jumpOp(branch);
    } లేకపోతే {
        తదుపరి_Op();
    }
}


d_m3Op(BranchIf_s)
{
    i32  condition = స్థానం(i32);
    pc_t branch = తక్షణ(pc_t);

    ఒకవేళ (condition) {
        jumpOp(branch);
    } లేకపోతే {
        తదుపరి_Op();
    }
}

#ఒకవేళ d_m3FuseBranch

// fused compare+branch and compare+if: a compare retro-patched by the compiler when a br_if or if
// immediately consumes its result. operand shapes and immediate order match the plain compare
// (_rs/_sr/_ss); the branch/else target pointer is the last immediate.
#  define d_m3FusedCmpBranch(TYPE, NAME, OPER)                \
  d_m3Op(TYPE##_BranchIf_##NAME##_rs)                         \
  {                                                           \
      TYPE నిర్వాహ్యం = స్థానం (TYPE);                             \
      pc_t branch  = తక్షణ (pc_t);                        \
      ఒకవేళ (నిర్వాహ్యం OPER ((TYPE) _r0))                          \
          jumpOp (branch);                                    \
      లేకపోతే తదుపరి_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_sr)                         \
  {                                                           \
      TYPE నిర్వాహ్యం = స్థానం (TYPE);                             \
      pc_t branch  = తక్షణ (pc_t);                        \
      ఒకవేళ (((TYPE) _r0) OPER నిర్వాహ్యం)                          \
          jumpOp (branch);                                    \
      లేకపోతే తదుపరి_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_BranchIf_##NAME##_ss)                         \
  {                                                           \
      TYPE operand2 = స్థానం (TYPE);                            \
      TYPE operand1 = స్థానం (TYPE);                            \
      pc_t branch   = తక్షణ (pc_t);                       \
      ఒకవేళ (operand1 OPER operand2)                             \
          jumpOp (branch);                                    \
      లేకపోతే తదుపరి_Op ();                                         \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_rs)                               \
  {                                                           \
      TYPE నిర్వాహ్యం = స్థానం (TYPE);                             \
      pc_t elsePC  = తక్షణ (pc_t);                        \
      ఒకవేళ (నిర్వాహ్యం OPER ((TYPE) _r0))                          \
          తదుపరి_Op ();                                          \
      లేకపోతే jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_sr)                               \
  {                                                           \
      TYPE నిర్వాహ్యం = స్థానం (TYPE);                             \
      pc_t elsePC  = తక్షణ (pc_t);                        \
      ఒకవేళ (((TYPE) _r0) OPER నిర్వాహ్యం)                          \
          తదుపరి_Op ();                                          \
      లేకపోతే jumpOp (elsePC);                                   \
  }                                                           \
  d_m3Op(TYPE##_If_##NAME##_ss)                               \
  {                                                           \
      TYPE operand2 = స్థానం (TYPE);                            \
      TYPE operand1 = స్థానం (TYPE);                            \
      pc_t elsePC   = తక్షణ (pc_t);                       \
      ఒకవేళ (operand1 OPER operand2)                             \
          తదుపరి_Op ();                                          \
      లేకపోతే jumpOp (elsePC);                                   \
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
    pc_t branch = తక్షణ(pc_t);
    ఒకవేళ (((i32)_r0) == 0) {
        jumpOp(branch);
    } లేకపోతే {
        తదుపరి_Op();
    }
}

d_m3Op(i32_BranchIfEqz_s)
{
    i32  నిర్వాహ్యం = స్థానం(i32);
    pc_t branch = తక్షణ(pc_t);
    ఒకవేళ (నిర్వాహ్యం == 0) {
        jumpOp(branch);
    } లేకపోతే {
        తదుపరి_Op();
    }
}

d_m3Op(i32_IfEqz_r)
{
    pc_t elsePC = తక్షణ(pc_t);
    ఒకవేళ (((i32)_r0) == 0) {
        తదుపరి_Op();
    } లేకపోతే {
        jumpOp(elsePC);
    }
}

d_m3Op(i32_IfEqz_s)
{
    i32  నిర్వాహ్యం = స్థానం(i32);
    pc_t elsePC = తక్షణ(pc_t);
    ఒకవేళ (నిర్వాహ్యం == 0) {
        తదుపరి_Op();
    } లేకపోతే {
        jumpOp(elsePC);
    }
}

#అయితే_ముగింపు // d_m3FuseBranch


d_m3Op(BranchIfPrologue_r)
{
    i32  condition = (i32)_r0;
    pc_t branch = తక్షణ(pc_t);

    ఒకవేళ (condition) {
        // this is the "prologue" that ends with
        // a plain branch to the actual target
        తదుపరి_Op();
    } లేకపోతే {
        jumpOp(branch); // jump over the prologue
    }
}


d_m3Op(BranchIfPrologue_s)
{
    i32  condition = స్థానం(i32);
    pc_t branch = తక్షణ(pc_t);

    ఒకవేళ (condition) {
        తదుపరి_Op();
    } లేకపోతే {
        jumpOp(branch);
    }
}


d_m3Op(ContinueLoop)
{
    m3_దొంతర_Check();

    శూన్య* loopId = తక్షణ(శూన్య*);
    తిరిగి loopId;
}


d_m3Op(ContinueLoopIf)
{
    i32   condition = (i32)_r0;
    శూన్య* loopId = తక్షణ(శూన్య*);

    ఒకవేళ (condition) {
        తిరిగి loopId;
    } లేకపోతే {
        తదుపరి_Op();
    }
}


#ఒకవేళ d_m3_Has_దొంతర_Switching

// This is where execution can "escape" the M3 code and callback to the client / fiber switch
// OR it can go in the Loop operation. I think it's best to do here. adding code to the loop operation
// has the potential to increase its native-stack usage.

d_m3Op(ContinueLoop_Suspendable)
{
    m3_దొంతర_Check();

    IM3Runtime runtime = m3MemRuntime(_mem);
    ఒకవేళ (M3_UNLIKELY(runtime->suspendRequested and runtime->చురుకైన_Continuation)) {
        Suspend_Without_ట్యాగ్(runtime, runtime->చురుకైన_Continuation, safepoint_op, _pc - 1, _sp,
                          d_m3_Exp_Reg_వాదనలు(_r0, _fp0));

        తిరిగి m3Err_continuationSuspended;
    }

    శూన్య* loopId = తక్షణ(శూన్య*);
    తిరిగి loopId;
}


#  ఒకవేళ !d_m3_నమోదు_Keeps_చట్రం
// op_Entry, in code compiled while the runtime is suspendable: once the frame is
// set up, it checks for a pause request. Recursion passes one of these on every
// lap, as a loop passes its back edge, so a pause the host requests reaches code
// that has no loop. A pause here stops at the body's first operation, so going
// on from it is going on past the check.
d_m3Op(నమోదు_Suspendable)
{
    d_m3ClearRegisters

    IM3Function function = తక్షణ(IM3Function);

    ఒకవేళ (నమోదు_సరిపోతుంది(function, _sp, _mem)) {
        Enter_చట్రం(function, _sp);

        IM3Runtime runtime = m3MemRuntime(_mem);
        ఒకవేళ (M3_UNLIKELY(runtime->suspendRequested and runtime->చురుకైన_Continuation)) {
            Suspend_Without_ట్యాగ్(runtime, runtime->చురుకైన_Continuation, safepoint_నమోదు, _pc, _sp,
                              d_m3_Exp_Reg_వాదనలు(_r0, _fp0));

            తిరిగి m3Err_continuationSuspended;
        }

        తదుపరి_Op_ప్రత్యక్ష();
    } లేకపోతే {
        newTrap(m3_Err_trap_దొంతర_అధికప్రవాహం);
    }
}
#  endif


// The check op_Entry_Suspendable makes, as an operation of its own for the
// builds whose op_Entry keeps a frame and so cannot make it: the start of a
// function's body, in code compiled while the runtime is suspendable
d_m3Op(నమోదు_Check)
{
    IM3Runtime runtime = m3MemRuntime(_mem);
    ఒకవేళ (M3_UNLIKELY(runtime->suspendRequested and runtime->చురుకైన_Continuation)) {
        Suspend_Without_ట్యాగ్(runtime, runtime->చురుకైన_Continuation, safepoint_నమోదు, _pc - 1, _sp,
                          d_m3_Exp_Reg_వాదనలు(_r0, _fp0));

        తిరిగి m3Err_continuationSuspended;
    }

    తదుపరి_Op();
}


d_m3Op(ContinueLoopIf_Suspendable)
{
    i32   condition = (i32)_r0;
    శూన్య* loopId = తక్షణ(శూన్య*);

    ఒకవేళ (condition) {
        IM3Runtime runtime = m3MemRuntime(_mem);
        ఒకవేళ (M3_UNLIKELY(runtime->suspendRequested and runtime->చురుకైన_Continuation)) {
            Suspend_Without_ట్యాగ్(runtime, runtime->చురుకైన_Continuation, safepoint_op, _pc - 2, _sp,
                              d_m3_Exp_Reg_వాదనలు(_r0, _fp0));

            తిరిగి m3Err_continuationSuspended;
        }
        తిరిగి loopId;
    } లేకపోతే {
        తదుపరి_Op();
    }
}


// Carries a resumed back edge or function entry on past the check it paused in
// (see M3Runtime.resumePastCheck), by running the operation the check stands
// in front of: the plain back edge on the same immediates, or whatever follows
// the entry check.
స్థిర
m3ret_t ResumePastCheck (IM3Continuation i_cont, M3_జ్ఞాపకం_శీర్షిక* _mem)
{
    pc_t         check = i_cont->pc;
    IM3Operation op = (IM3Operation)(*check);
    pc_t         _pc = check + 1;
    m3stack_t    _sp = i_cont->sp;
    m3reg_t      _r0 = i_cont->r0;
#  ఒకవేళ d_m3HasFloat
    f64 _fp0 = i_cont->fp0;
#  endif

    ఒకవేళ (op == op_ContinueLoop_Suspendable) {
        op = op_ContinueLoop;
    } లేకపోతే ఒకవేళ (op == op_ContinueLoopIf_Suspendable) {
        op = op_ContinueLoopIf;
    } లేకపోతే ఒకవేళ (op == op_నమోదు_Check) {
        తిరిగి తదుపరి_Op_Impl();
    } లేకపోతే {
        తిరిగి d_m3CallWithRegs(check, _sp, _mem, _r0, _fp0);
    }

#  ఒకవేళ (d_m3EnableOpProfiling || d_m3EnableOpTracing)
    తిరిగి op(_pc, d_m3_Op_వాదనలు, d_m3_పునాది_Cstr);
#  లేకపోతే
    తిరిగి op(_pc, d_m3_Op_వాదనలు);
#  endif
}
#అయితే_ముగింపు // d_m3HasStackSwitching


d_m3Op(Const32)
{
    u32 విలువ = *(u32*)_pc++;
    స్థానం(u32) = విలువ;
    తదుపరి_Op();
}


d_m3Op(Const64)
{
    // where a pointer is 32 bits the two lines are only 4-byte aligned
    u64 విలువ;
    జ్ఞాపకం_నకలు(&విలువ, _pc, పరిమాణం(విలువ));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;
    స్థానం(u64) = విలువ;
    తదుపరి_Op();
}

d_m3Op(Unsupported)
{
    newTrap(m3Err_trapUnsupportedInstruction);
}

d_m3Op(Unreachable)
{
    m3_దొంతర_Check();
    newTrap(m3Err_trapUnreachable);
}


d_m3Op(చివర_2)
{
    m3_దొంతర_Check();
    తిరిగి m3Err_none;
}


d_m3Op(సమితి_Global_s32)
{
    u32* global = తక్షణ(u32*);
    *global = స్థానం(u32);

    తదుపరి_Op();
}


d_m3Op(సమితి_Global_s64)
{
    u64* global = తక్షణ(u64*);
    *global = స్థానం(u64);

    తదుపరి_Op();
}

#ఒకవేళ d_m3HasFloat
d_m3Op(సమితి_Global_f32)
{
    f32* global = తక్షణ(f32*);
    *global = (f32)_fp0;

    తదుపరి_Op();
}


d_m3Op(సమితి_Global_f64)
{
    f64* global = తక్షణ(f64*);
    *global = _fp0;

    తదుపరి_Op();
}
#అయితే_ముగింపు


#ఒకవేళ d_m3HasMemory64

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
    u64  నిర్వాహ్యం = స్థానం(u64);
    u32* చిరునామా = స్థానం_ptr(u32);

    u64  offset;
    జ్ఞాపకం_నకలు(&offset, _pc, పరిమాణం(offset));
    _pc += (M3_SIZEOF_PTR == 4) ? 2 : 1;

    ఒకవేళ (M3_LIKELY(నిర్వాహ్యం < d_m3_చిరునామా_హద్దు)) {
        u64 effective = నిర్వాహ్యం + offset;

        // the access itself checks its own size against what is left
        ఒకవేళ (M3_LIKELY(effective < _mem->నిడివి)) {
            *చిరునామా = (u32)effective;
            తదుపరి_Op();
        }
    }

    d_బహిర్గతం_యొక్క_Bounds;
}

#అయితే_ముగింపు // d_m3HasMemory64


// Under d_m3GuardedMemory there is nothing to check: every address one of these
// operations can name lands inside the memory's reservation, and the part of it the
// memory does not have is not committed, so the access faults and RunCodeChecked
// turns that into the trap the spec asks for.
#ఒకవేళ d_m3_Skip_జ్ఞాపకం_Bounds_Check || d_m3_Guarded_జ్ఞాపకం
#  define m3MemCheck(x) true
#లేకపోతే
#  define m3MemCheck(x) M3_LIKELY(x)
#అయితే_ముగింపు

// memcpy here is to support non-aligned access on some platforms.

#నిర్వచించు d_m3_ఎక్కించు(REG,DEST_TYPE,SRC_TYPE)                \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u32 offset = తక్షణ (u32);                       \
    d_m3_Preload_తదుపరి ();                                 \
    u64 నిర్వాహ్యం = (u32) _r0;                            \
    నిర్వాహ్యం += offset;                                  \
                                                        \
    ఒకవేళ (m3MemCheck(                                     \
        నిర్వాహ్యం + పరిమాణం (SRC_TYPE) <= _mem->నిడివి     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;       \
            SRC_TYPE విలువ;                             \
            జ్ఞాపకం_నకలు(&విలువ, src8, పరిమాణం(విలువ));        \
            M3_BSWAP_##SRC_TYPE(విలువ);                 \
            REG = (DEST_TYPE)విలువ;                     \
            d_m3_Trace_ఎక్కించు(DEST_TYPE, నిర్వాహ్యం, REG);     \
        }                                               \
        తదుపరి_Op_Preloaded ();                             \
    } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
}                                                       \
d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s)                 \
{                                                       \
    d_m3TracePrepare                                    \
    u64 నిర్వాహ్యం = స్థానం (u32);                           \
    u32 offset = తక్షణ (u32);                       \
    d_m3_Preload_తదుపరి ();                                 \
    నిర్వాహ్యం += offset;                                  \
                                                        \
    ఒకవేళ (m3MemCheck(                                     \
        నిర్వాహ్యం + పరిమాణం (SRC_TYPE) <= _mem->నిడివి     \
    )) {                                                \
        {                                               \
            u8* src8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;       \
            SRC_TYPE విలువ;                             \
            జ్ఞాపకం_నకలు(&విలువ, src8, పరిమాణం(విలువ));        \
            M3_BSWAP_##SRC_TYPE(విలువ);                 \
            REG = (DEST_TYPE)విలువ;                     \
            d_m3_Trace_ఎక్కించు(DEST_TYPE, నిర్వాహ్యం, REG);     \
        }                                               \
        తదుపరి_Op_Preloaded ();                             \
    } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
}

//  printf ("get: %d -> %d\n", operand + offset, (i64) REG);


#నిర్వచించు d_m3_ఎక్కించు_i(DEST_TYPE, SRC_TYPE) d_m3_ఎక్కించు(_r0, DEST_TYPE, SRC_TYPE)
#నిర్వచించు d_m3_ఎక్కించు_f(DEST_TYPE, SRC_TYPE) d_m3_ఎక్కించు(_fp0, DEST_TYPE, SRC_TYPE)

#ఒకవేళ d_m3HasFloat
d_m3_ఎక్కించు_f(f32, f32);
d_m3_ఎక్కించు_f(f64, f64);
#అయితే_ముగింపు

d_m3_ఎక్కించు_i(i32, i8);
d_m3_ఎక్కించు_i(i32, u8);
d_m3_ఎక్కించు_i(i32, i16);
d_m3_ఎక్కించు_i(i32, u16);
d_m3_ఎక్కించు_i(i32, i32);

d_m3_ఎక్కించు_i(i64, i8);
d_m3_ఎక్కించు_i(i64, u8);
d_m3_ఎక్కించు_i(i64, i16);
d_m3_ఎక్కించు_i(i64, u16);
d_m3_ఎక్కించు_i(i64, i32);
d_m3_ఎక్కించు_i(i64, u32);
d_m3_ఎక్కించు_i(i64, i64);

#ఒకవేళ d_m3_Fold_సమితి_Local

// destination-folded loads: the value goes to a trailing destination slot instead of _r0.
// operand immediates keep d_m3Load's order; the destination slot is the last immediate
#  define d_m3_ఎక్కించు_Fold(REG, DEST_TYPE, SRC_TYPE)          \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_r_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u32 offset = తక్షణ (u32);                       \
      DEST_TYPE * dest = స్థానం_ptr (DEST_TYPE);            \
      d_m3_Preload_తదుపరి ();                                 \
      u64 నిర్వాహ్యం = (u32) _r0;                            \
      నిర్వాహ్యం += offset;                                  \
                                                          \
      ఒకవేళ (m3MemCheck(                                     \
          నిర్వాహ్యం + పరిమాణం (SRC_TYPE) <= _mem->నిడివి     \
      )) {                                                \
          u8* src8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;           \
          SRC_TYPE విలువ;                                 \
          జ్ఞాపకం_నకలు(&విలువ, src8, పరిమాణం(విలువ));            \
          M3_BSWAP_##SRC_TYPE(విలువ);                     \
          DEST_TYPE ఫలితం = (DEST_TYPE)విలువ;            \
          * dest = ఫలితం;  REG = ఫలితం;                 \
          d_m3_Trace_ఎక్కించు(DEST_TYPE, నిర్వాహ్యం, ఫలితం);      \
          తదుపరి_Op_Preloaded ();                             \
      } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
  }                                                       \
  d_m3Op(DEST_TYPE##_Load_##SRC_TYPE##_s_f)               \
  {                                                       \
      d_m3TracePrepare                                    \
      u64 నిర్వాహ్యం = స్థానం (u32);                           \
      u32 offset = తక్షణ (u32);                       \
      DEST_TYPE * dest = స్థానం_ptr (DEST_TYPE);            \
      d_m3_Preload_తదుపరి ();                                 \
      నిర్వాహ్యం += offset;                                  \
                                                          \
      ఒకవేళ (m3MemCheck(                                     \
          నిర్వాహ్యం + పరిమాణం (SRC_TYPE) <= _mem->నిడివి     \
      )) {                                                \
          u8* src8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;           \
          SRC_TYPE విలువ;                                 \
          జ్ఞాపకం_నకలు(&విలువ, src8, పరిమాణం(విలువ));            \
          M3_BSWAP_##SRC_TYPE(విలువ);                     \
          DEST_TYPE ఫలితం = (DEST_TYPE)విలువ;            \
          * dest = ఫలితం;  REG = ఫలితం;                 \
          d_m3_Trace_ఎక్కించు(DEST_TYPE, నిర్వాహ్యం, ఫలితం);      \
          తదుపరి_Op_Preloaded ();                             \
      } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
  }

d_m3_ఎక్కించు_Fold(_r0, i32, i8)
d_m3_ఎక్కించు_Fold(_r0, i32, u8)
d_m3_ఎక్కించు_Fold(_r0, i32, i16)
d_m3_ఎక్కించు_Fold(_r0, i32, u16)
d_m3_ఎక్కించు_Fold(_r0, i32, i32)

// The fold forms take the address from _r0 and write the result straight to the
// destination slot, so they never touch _fp0 and the float widths need no register form.
d_m3_ఎక్కించు_Fold(_r0, i64, i8)
d_m3_ఎక్కించు_Fold(_r0, i64, u8)
d_m3_ఎక్కించు_Fold(_r0, i64, i16)
d_m3_ఎక్కించు_Fold(_r0, i64, u16)
d_m3_ఎక్కించు_Fold(_r0, i64, i32)
d_m3_ఎక్కించు_Fold(_r0, i64, u32)
d_m3_ఎక్కించు_Fold(_r0, i64, i64)
#  ఒకవేళ d_m3HasFloat
d_m3_ఎక్కించు_Fold(_fp0, f32, f32)
d_m3_ఎక్కించు_Fold(_fp0, f64, f64)
#  endif

#అయితే_ముగింపు // d_m3FoldSetLocal

#నిర్వచించు d_m3Store(REG, SRC_TYPE, DEST_TYPE)             \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_rs)             \
{                                                       \
    d_m3TracePrepare                                    \
    u64 నిర్వాహ్యం = స్థానం (u32);                           \
    u32 offset = తక్షణ (u32);                       \
    d_m3_Preload_తదుపరి ();                                 \
    నిర్వాహ్యం += offset;                                  \
                                                        \
    ఒకవేళ (m3MemCheck(                                     \
        నిర్వాహ్యం + పరిమాణం (DEST_TYPE) <= _mem->నిడివి    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, నిర్వాహ్యం, REG);     \
            u8* mem8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;       \
            DEST_TYPE val = (DEST_TYPE) REG;            \
            M3_BSWAP_##DEST_TYPE(val);                  \
            జ్ఞాపకం_నకలు(mem8, &val, పరిమాణం(val));            \
        }                                               \
        తదుపరి_Op_Preloaded ();                             \
    } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_sr)             \
{                                                       \
    d_m3TracePrepare                                    \
    స్థిరరాశి SRC_TYPE విలువ = స్థానం (SRC_TYPE);             \
    u64 నిర్వాహ్యం = (u32) _r0;                            \
    u32 offset = తక్షణ (u32);                       \
    d_m3_Preload_తదుపరి ();                                 \
    నిర్వాహ్యం += offset;                                  \
                                                        \
    ఒకవేళ (m3MemCheck(                                     \
        నిర్వాహ్యం + పరిమాణం (DEST_TYPE) <= _mem->నిడివి    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, నిర్వాహ్యం, విలువ);   \
            u8* mem8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;       \
            DEST_TYPE val = (DEST_TYPE) విలువ;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            జ్ఞాపకం_నకలు(mem8, &val, పరిమాణం(val));            \
        }                                               \
        తదుపరి_Op_Preloaded ();                             \
    } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
}                                                       \
d_m3Op  (SRC_TYPE##_Store_##DEST_TYPE##_ss)             \
{                                                       \
    d_m3TracePrepare                                    \
    స్థిరరాశి SRC_TYPE విలువ = స్థానం (SRC_TYPE);             \
    u64 నిర్వాహ్యం = స్థానం (u32);                           \
    u32 offset = తక్షణ (u32);                       \
    d_m3_Preload_తదుపరి ();                                 \
    నిర్వాహ్యం += offset;                                  \
                                                        \
    ఒకవేళ (m3MemCheck(                                     \
        నిర్వాహ్యం + పరిమాణం (DEST_TYPE) <= _mem->నిడివి    \
    )) {                                                \
        {                                               \
            d_m3TraceStore(SRC_TYPE, నిర్వాహ్యం, విలువ);   \
            u8* mem8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;       \
            DEST_TYPE val = (DEST_TYPE) విలువ;          \
            M3_BSWAP_##DEST_TYPE(val);                  \
            జ్ఞాపకం_నకలు(mem8, &val, పరిమాణం(val));            \
        }                                               \
        తదుపరి_Op_Preloaded ();                             \
    } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
}

// both operands can be in regs when storing a float
#నిర్వచించు d_m3StoreFp(REG, TYPE)                          \
d_m3Op  (TYPE##_Store_##TYPE##_rr)                      \
{                                                       \
    d_m3TracePrepare                                    \
    u64 నిర్వాహ్యం = (u32) _r0;                            \
    u32 offset = తక్షణ (u32);                       \
    d_m3_Preload_తదుపరి ();                                 \
    నిర్వాహ్యం += offset;                                  \
                                                        \
    ఒకవేళ (m3MemCheck(                                     \
        నిర్వాహ్యం + పరిమాణం (TYPE) <= _mem->నిడివి         \
    )) {                                                \
        {                                               \
            d_m3TraceStore(TYPE, నిర్వాహ్యం, REG);         \
            u8* mem8 = m3_Mem_దత్తాంశం(_mem) + నిర్వాహ్యం;       \
            TYPE val = (TYPE) REG;                      \
            M3_BSWAP_##TYPE(val);                       \
            జ్ఞాపకం_నకలు(mem8, &val, పరిమాణం(val));            \
        }                                               \
        తదుపరి_Op_Preloaded ();                             \
    } లేకపోతే d_బహిర్గతం_యొక్క_Bounds;                               \
}


#నిర్వచించు d_m3Store_i(SRC_TYPE, DEST_TYPE) d_m3Store(_r0, SRC_TYPE, DEST_TYPE)
#నిర్వచించు d_m3Store_f(SRC_TYPE, DEST_TYPE) d_m3Store(_fp0, SRC_TYPE, DEST_TYPE) d_m3StoreFp (_fp0, SRC_TYPE);

#ఒకవేళ d_m3HasFloat
d_m3Store_f(f32, f32)
d_m3Store_f(f64, f64)
#అయితే_ముగింపు

d_m3Store_i(i32, u8)
d_m3Store_i(i32, i16)
d_m3Store_i(i32, i32)

d_m3Store_i(i64, u8)
d_m3Store_i(i64, i16)
d_m3Store_i(i64, i32)
d_m3Store_i(i64, i64)

#అన్తర్భావ "m3_exec_atomic.h"

#నిర్వచనం_రద్దు m3MemCheck


//---------------------------------------------------------------------------------------------------------------------
// debug/profiling
//---------------------------------------------------------------------------------------------------------------------
#ఒకవేళ d_m3EnableOpTracing
d_m3RetSig debugOp (d_m3OpSig, cstr_t i_opcode)
{
    అక్షరం name[100];
    strcpy(name, అక్షరశ్రేణి_శోధన(i_opcode, "op_") + 3);
    అక్షరం* bracket = అక్షరశ్రేణి_శోధన(name, "(");
    ఒకవేళ (bracket) {
        *bracket = 0;
    }

    puts(name);
    తదుపరి_Op_ప్రత్యక్ష();
}
#అయితే_ముగింపు

#ఒకవేళ d_m3EnableOpProfiling
d_m3RetSig profileOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_తగిలింది(i_operationName);

    తదుపరి_Op_ప్రత్యక్ష();
}

// Same, for an operation leaving by a branch: jumpOp hands the target pc in as _pc,
// so dispatching from here lands exactly where jumpOpDirect would have.
d_m3RetSig profileJumpOp (d_m3OpSig, cstr_t i_operationName)
{
    Profile_తగిలింది(i_operationName);

    తదుపరి_Op_ప్రత్యక్ష();
}
#అయితే_ముగింపు

d_m3_చివర_Extern_C

#అయితే_ముగింపు // m3_exec_h
