//
//  m3_config.h
//
//  Created by Steven Massey on 5/4/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#که_نه_تعریف_شوی m3_config_h
#تعریف_کړه m3_config_h

#ضمن "m3_config_platforms.h"

// general --------------------------------------------------------------------

#که_نه_تعریف_شوی d_m3_کوډ_پاڼه_Align_اندازه
#  define d_m3_کوډ_پاڼه_Align_اندازه                32*1024
#د_که_پای

#که_نه_تعریف_شوی d_m3_Max_Function_ډېری_لوړوالی
#  define d_m3_Max_Function_ډېری_لوړوالی           8000    // max: 32768
#د_که_پای

#که_نه_تعریف_شوی d_m3_Max_خطي_حافظه_پاڼې
#  define d_m3_Max_خطي_حافظه_پاڼې             65536
#د_که_پای

#که_نه_تعریف_شوی d_m3MaxFunctionSlots
#  define d_m3MaxFunctionSlots                 ((d_m3_Max_Function_ډېری_لوړوالی)*2)
#د_که_پای

#که_نه_تعریف_شوی d_m3_Val_ډېری                                   // validator operand and local type stacks:
#  define d_m3_Val_ډېری                         (d_m3_Max_Function_ډېری_لوړوالی)       // the same operand stack the compiler bounds
#د_که_پای

#که_نه_تعریف_شوی d_m3_Val_Ctrl_ژوروالی                               // validator block nesting depth. Each frame is bigger than an
#  define d_m3_Val_Ctrl_ژوروالی                     ((d_m3_Max_Function_ډېری_لوړوالی)/4)   // operand entry, so this dominates the validator's stack usage
#د_که_پای

#که_نه_تعریف_شوی d_m3_Max_Constant_جدول_اندازه
#  define d_m3_Max_Constant_جدول_اندازه             120
#د_که_پای

#که_نه_تعریف_شوی d_m3MaxDuplicateFunctionImpl
#  define d_m3MaxDuplicateFunctionImpl         3
#د_که_پای

#که_نه_تعریف_شوی d_m3VerboseErrorMessages
#  define d_m3VerboseErrorMessages             1
#د_که_پای

#که_نه_تعریف_شوی d_m3_Fixed_ډېرۍ
#  define d_m3_Fixed_ډېرۍ                        false
//# define d_m3FixedHeap                       (32*1024)
#د_که_پای

#که_نه_تعریف_شوی d_m3_Fixed_ډېرۍ_Align
#  define d_m3_Fixed_ډېرۍ_Align                   16
#د_که_پای

#که_نه_تعریف_شوی d_m3_Use32_بټ_Slots
#  define d_m3_Use32_بټ_Slots                    1
#د_که_پای

#که_نه_تعریف_شوی d_m3_Profiler_خونه_ماسک
#  define d_m3_Profiler_خونه_ماسک                 0xFFFF
#د_که_پای

#که_نه_تعریف_شوی d_m3_ریکارډ_Backtraces
#  define d_m3_ریکارډ_Backtraces                 0
#د_که_پای

#که_نه_تعریف_شوی d_m3EnableExceptionBreakpoint
#  define d_m3EnableExceptionBreakpoint        0       // see m3_exception.h
#د_که_پای

// Backtraces and structured traces need op_Entry to still be around when the function
// body returns, so it can't tail-call into it.  Everywhere else it can, which keeps the
// native stack flat across calls -- and is what makes return_call actually iterative.
#که_نه_تعریف_شوی d_m3_ثبت_Keeps_چوکاټ
#  define d_m3_ثبت_Keeps_چوکاټ                  (d_m3_ریکارډ_Backtraces || (d_m3EnableStrace >= 2))
#د_که_پای

// Whether return_call/return_call_indirect can reuse the caller's frame.  Reusing it stops
// the m3 stack from growing, so it's only safe where the native stack doesn't grow either
// -- otherwise runaway tail recursion would blow the native stack with nothing to trap it.
// Where that doesn't hold they compile to a plain call followed by a return: still
// correct, just not iterative, and the m3 stack keeps overflowing (and trapping) first.
#که_نه_تعریف_شوی d_m3_Can_لکۍ_Call
#  define d_m3_Can_لکۍ_Call                      (!d_m3_ثبت_Keeps_چوکاټ && M3_GUARANTEED_لکۍ_CALL)
#د_که_پای


// profiling and tracing ------------------------------------------------------

#که_نه_تعریف_شوی d_m3EnableOpProfiling
#  define d_m3EnableOpProfiling                0       // opcode usage counters
#د_که_پای

#که_نه_تعریف_شوی d_m3EnableOpTracing
#  define d_m3EnableOpTracing                  0       // only works with DEBUG
#د_که_پای

#که_نه_تعریف_شوی d_m3EnableWasiTracing
#  define d_m3EnableWasiTracing                0
#د_که_پای

#که_نه_تعریف_شوی d_m3EnableStrace
#  define d_m3EnableStrace                     0       // 1 - trace exported function calls
// 2 - trace all calls (structured)
// 3 - all calls + loops + memory operations
#د_که_پای


// logging --------------------------------------------------------------------

#که_نه_تعریف_شوی d_m3LogParse
#  define d_m3LogParse                         0       // .wasm binary decoding info
#د_که_پای

#که_نه_تعریف_شوی d_m3LogModule
#  define d_m3LogModule                        0       // wasm module info
#د_که_پای

#که_نه_تعریف_شوی d_m3LogCompile
#  define d_m3LogCompile                       0       // wasm -> metacode generation phase
#د_که_پای

#که_نه_تعریف_شوی d_m3_Log_Wasm_ډېری
#  define d_m3_Log_Wasm_ډېری                     0       // dump the wasm stack when pushed or popped
#د_که_پای

#که_نه_تعریف_شوی d_m3LogEmit
#  define d_m3LogEmit                          0       // metacode generation info
#د_که_پای

#که_نه_تعریف_شوی d_m3_Log_کوډ_پاڼې
#  define d_m3_Log_کوډ_پاڼې                     0       // dump metacode pages when released
#د_که_پای

#که_نه_تعریف_شوی d_m3LogRuntime
#  define d_m3LogRuntime                       0       // higher-level runtime information
#د_که_پای

#که_نه_تعریف_شوی d_m3_Log_Native_ډېری
#  define d_m3_Log_Native_ډېری                   0       // track the memory usage of the C-stack
#د_که_پای

#که_نه_تعریف_شوی d_m3_Log_ډېرۍ_Ops
#  define d_m3_Log_ډېرۍ_Ops                       0       // track heap usage
#د_که_پای

#که_نه_تعریف_شوی d_m3LogTimestamps
#  define d_m3LogTimestamps                    0       // track timestamps on heap logs
#د_که_پای

// other ----------------------------------------------------------------------

#که_نه_تعریف_شوی d_m3HasFloat
#  define d_m3HasFloat                         1       // implement floating point ops
#د_که_پای

// The deterministic profile: the same module fed the same input produces the same
// run, wherever and whenever it is run.
//
// A profile in the WebAssembly sense is a subset of the language that an ecosystem
// picks once, for all of its modules - not something a module or a call turns on -
// which is why this is a build option and not a runtime one. See
// https://github.com/WebAssembly/profiles. That proposal names the deterministic
// profile as the marker set {N,T}: it excludes the constructs whose behaviour is
// nondeterministic, and threading.
//
// What that comes to here:
//
//   N   the one rule the proposal spells out is that a NaN result is normalized to
//       the canonical NaN, which is d_m3CanonicalNaN below and is turned on by this.
//       The other N-marked constructs are relaxed SIMD, which wasm3 does not
//       implement at all, so there is nothing to exclude.
//   T   threads and atomics, which wasm3 does not implement either. Whoever adds
//       them has to keep them out of this profile.
//
// The proposal is about the language and stops at the module's edge. What a module
// reads through its imports - the clock, entropy, whether a wait actually waits - is
// nondeterministic in exactly the way that matters and is nobody's profile, so this
// option covers it too: the WASI clock counts from a fixed instant in fixed steps,
// random_get is a stream from a fixed seed, and poll_oneoff stops waiting on
// anything real. That half is wasm3's own, not the proposal's.
//
// Two limits belong beside it and stay separate because each is useful alone:
// m3_SetResourceLimit with c_m3Limit_GasUnits, so a runaway module stops after
// the same amount of work, and c_m3Limit_MemoryBytes (--max-memory in the CLI),
// so memory.grow fails at the same point rather than wherever the allocator gives out.
#که_نه_تعریف_شوی d_m3DeterministicProfile
#  define d_m3DeterministicProfile             0
#د_که_پای

// Where a float operation's result is a NaN, the spec lets the host choose its sign
// and payload, and different architectures choose differently: the same module
// produces different bits on x86, on ARM and on MIPS. With this on, every such
// result is replaced by the one canonical NaN, so those bits are the same
// everywhere.
//
// Follows the deterministic profile, which is the thing that needs it, and can be
// set on its own by a build that wants the float behaviour and nothing else. It
// costs a compare and a predictable branch on each of the float operations the spec
// calls arithmetic. It changes nothing else - a canonical NaN satisfies every
// assertion the spec suite makes about an arithmetic NaN - and abs, neg and copysign
// keep carrying their operand's payload through, as the spec defines them to.
#که_نه_تعریف_شوی d_m3CanonicalNaN
#  define d_m3CanonicalNaN                     d_m3DeterministicProfile
#د_که_پای

// Issue the dispatch load for the next operation as soon as this one's immediates are
// consumed, rather than at the point of the jump that consumes it, so the operation's
// own work covers the load latency. See the comment in m3_exec.h.
#که_نه_تعریف_شوی d_m3_Preload_بل_Op
#  define d_m3_Preload_بل_Op                    0
#د_که_پای

#که_نه_تعریف_شوی d_m3_Fold_سېټ_Local
#  define d_m3_Fold_سېټ_Local                     1       // fuse a value-producing op with an immediately
#د_که_پای                                                 // following local.set by retro-patching its destination

#که_نه_تعریف_شوی d_m3FuseBranch
#  define d_m3FuseBranch                       1       // fuse a compare with an immediately following
#د_که_پای                                                 // br_if/if into one op (needs d_m3FoldSetLocal)
#إذا d_m3FuseBranch && !d_m3_Fold_سېټ_Local
#  undef d_m3FuseBranch
#  define d_m3FuseBranch                       0       // the fusion candidate tracking lives in the fold machinery
#د_که_پای

#إذا !d_m3HasFloat && !defined(d_m3NoFloatDynamic)
#  define d_m3NoFloatDynamic                   1       // if no floats, do not fail until flops are actually executed
#د_که_پای

// funcref/externref values, the table instructions and multiple tables.
// Without it a module may still declare one funcref table and use call_indirect.
#که_نه_تعریف_شوی d_m3HasRefTypes
#  define d_m3HasRefTypes                      1       // implement the reference types proposal
#د_که_پای

// i32/i64 add, sub and mul inside constant expressions, so a global, data or
// element offset can be computed from an imported global instead of a literal.
#که_نه_تعریف_شوی d_m3HasExtendedConst
#  define d_m3HasExtendedConst                 1       // implement the extended constant expressions proposal
#د_که_پای

// More than one linear memory per module, each memory instruction naming the
// one it addresses. Memory 0 keeps the fast path -- the interpreter carries it
// in the _mem register -- and the others are reached by swapping that register
// around the access, so the cost lands only on modules that use them.
#که_نه_تعریف_شوی d_m3_Has_Multi_حافظه
#  define d_m3_Has_Multi_حافظه                   1       // implement the multiple memories proposal
#د_که_پای

// 64-bit linear memories: a memory declares whether it is addressed by i32 or
// i64, and every instruction that names it takes operands of that type. The
// address space wasm3 can actually back is far smaller than 2^64 either way --
// see d_m3AddressLimit -- so this buys the addressing, not the range.
#که_نه_تعریف_شوی d_m3HasMemory64
#  define d_m3HasMemory64                      1       // implement the memory64 proposal
#د_که_پای

// The import section's compact encodings: one module name shared by a run of
// imports, optionally with one externtype shared as well. Decoding only - a
// module means exactly what it would spelled out the long way.
#که_نه_تعریف_شوی d_m3HasCompactImports
#  define d_m3HasCompactImports                1       // implement the compact import section proposal
#د_که_پای

// 128-bit arithmetic over pairs of i64 halves
#که_نه_تعریف_شوی d_m3HasWideArithmetic
#  define d_m3HasWideArithmetic                1       // implement the wide arithmetic proposal
#د_که_پای

// The threads proposal's 0xFE atomic instructions, and the shared flag on a memory's
// limits. On their own they keep single-thread semantics: an atomic access is an
// ordinary one that also checks its alignment, and memory.atomic.wait can only
// time out. Sharing a memory between runtimes is d_m3HasThreads.
#که_نه_تعریف_شوی d_m3HasAtomics
#  define d_m3HasAtomics                       1       // implement the atomic instructions of the threads proposal
#د_که_پای

// The exception handling proposal: a tag section, the exnref value type, and
// the try_table / throw / throw_ref instructions. Exceptions unwind by riding
// the same m3ret_t return path traps already use, so the cost when no module
// throws is a handful of branches.
//
// An imported tag is the exporting module's when that module is already loaded
// into the runtime, or the host's when m3_LinkTag supplied one; otherwise it is
// a fresh tag of the importing module's own. The host throws with
// m3_ThrowException and reads what escaped with m3_GetExceptionTag.
//
// Three things it does not do:
//   - exception objects belong to the runtime. One caught without being reified
//     is released at the catch; the rest are held until the outermost m3_Call
//     returns, which is the last point an exnref can still be reached from the
//     Wasm stack. One parked in a global or a table outlives that and dangles;
//     there is no collector to say otherwise.
//   - only the exnref shorthand is understood, not the (ref exn) / (ref null
//     exn) spellings, which need the exn heap type alongside typed function
//     references.
//   - entering a try region costs a native frame that is only given back when
//     the enclosing function returns or an enclosing loop comes round, so a long
//     straight-line run of try blocks in one function trades native stack for it.
//     d_m3MaxNativeStack bounds this the same way it bounds call depth.
#که_نه_تعریف_شوی d_m3HasExceptionHandling
#  define d_m3HasExceptionHandling             1
#د_که_پای

// (ref $t) and (ref null $t), call_ref and the rest of the typed function
// references proposal. Off by default: it is not finished, and it widens the
// value type from one byte to two wherever the compiler carries one.
#که_نه_تعریف_شوی d_m3HasTypedRefs
#  define d_m3HasTypedRefs                     1
#د_که_پای

// The stack switching proposal: typed first-class continuations, control tags,
// suspend and resume. Built on the typed reference types, which is where a
// continuation type gets the identity that switch needs to name its peer, so
// it follows d_m3HasTypedRefs unless asked for explicitly.
#که_نه_تعریف_شوی d_m3_Has_ډېری_Switching
#  define d_m3_Has_ډېری_Switching                d_m3HasTypedRefs
#د_که_پای

#إذا d_m3_Has_ډېری_Switching && !d_m3HasTypedRefs
#  error "d_m3HasStackSwitching requires d_m3HasTypedRefs"
#د_که_پای

// The Wasm value stack a continuation runs on. It has to be its own buffer
// rather than a window onto the runtime stack, because it outlives the resume
// that started it.
#که_نه_تعریف_شوی d_m3_Continuation_ډېری_Slots
#  define d_m3_Continuation_ډېری_Slots           1024
#د_که_پای

// How many values a control tag may carry, and how many a suspended
// continuation may be handed when it is resumed or switched into. Bounds the
// fixed arrays a continuation keeps for them, so a module that asks for more
// is rejected at compile time.
#که_نه_تعریف_شوی d_m3MaxContinuationPayload
#  define d_m3MaxContinuationPayload           16
#د_که_پای

// How deep the interpreter may be in native frames when a continuation
// suspends. Only reached frames are recorded and the array grows to fit, so
// this is a ceiling rather than an allocation; past it, the suspend traps.
#که_نه_تعریف_شوی d_m3_Continuation_Max_چوکاټونه
#  define d_m3_Continuation_Max_چوکاټونه            256
#د_که_پای

// Snapshot serialization & deserialization. Allows saving suspended execution
// state (linear memory, globals, tables, value stack, and virtual frames) and
// restoring it to resume execution.
#که_نه_تعریف_شوی d_m3HasSnapshots
#  define d_m3HasSnapshots                     d_m3_Has_ډېری_Switching
#د_که_پای

// Minimum run length of consecutive 0x00 or 0xFF bytes to trigger sparse
// compression during snapshot linear memory encoding.
#که_نه_تعریف_شوی d_m3_Snapshot_پرلپسې_پوله
#  define d_m3_Snapshot_پرلپسې_پوله             128
#د_که_پای


// Gas metering. Once m3_SetResourceLimit has armed a runtime, the compiler
// instruments the function bodies it compiles from then on: each straight-line
// segment of a body is charged, before any of it runs, for the instructions it
// is about to execute, and execution traps once the budget is gone. The
// segmentation and the per-opcode costs follow ewasm's metering design.
#که_نه_تعریف_شوی d_m3HasGasMetering
#  define d_m3HasGasMetering                   1
#د_که_پای

#که_نه_تعریف_شوی d_m3EnableValidation
#  define d_m3EnableValidation                 1       // pre-pass bytecode type validation
#د_که_پای

#که_نه_تعریف_شوی d_m3_Skip_ډېری_Check
#  define d_m3_Skip_ډېری_Check                   0       // skip stack overrun checks
#د_که_پای

#که_نه_تعریف_شوی d_m3_Max_Native_ډېری
// native C-stack budget (bytes) available to Wasm execution. A recursive
// Wasm module builds up native call frames (op_Call -> op_Entry -> ...);
// once this budget is exhausted the interpreter traps instead of
// overflowing the real C stack. 0 disables.
//
// This is a ceiling, not a promise: where the system can be asked what the
// calling thread's stack actually is (m3_HostStackBase), a budget that would
// reach past the bottom of it is cut down to what is really there, less
// d_m3NativeStackMargin. That is what keeps the default safe on a thread
// smaller than it - a 256KiB worker, say - without every embedder having to
// know to lower it.
#  define d_m3_Max_Native_ډېری                   ((8 * 1024 * 1024) - (128 * 1024))
#د_که_پای

#که_نه_تعریف_شوی d_m3_Native_ډېری_Margin
// How much of a measured stack to leave below the mark. The mark is only tested
// between calls, so everything after the last test still has to fit: the rest of
// the op that traps, the interpreter frames unwinding behind it, and any host
// function already on the stack. Only used where the stack was measured rather
// than assumed; a stack too small to spare this much gives up a quarter of
// itself instead.
#  define d_m3_Native_ډېری_Margin                (64 * 1024)
#د_که_پای

#که_نه_تعریف_شوی d_m3_Skip_حافظه_Bounds_Check
#  define d_m3_Skip_حافظه_Bounds_Check            0       // skip memory bounds checks
#د_که_پای

// Back each linear memory with enough reserved address space that every address a
// Wasm instruction could name lands inside it, and commit only the part the memory
// actually has. An access past the end then reaches address space that is reserved
// and not committed, and the fault that follows is turned back into the trap the
// spec asks for - so the bounds check on every load and store goes away entirely.
//
// It asks a lot of the system, and m3_config_platforms.h turns it on for every target
// that answers - which leaves it off here for everything else:
//
//   - 64-bit only. The reservation is 8GiB per memory, because a 32-bit address
//     plus a 32-bit memarg offset reaches that far; there is no room for that in a
//     32-bit address space, and none to spare on anything embedded.
//   - address space to reserve, and a way to say so: mmap on POSIX, VirtualAlloc on
//     Windows. m3_host_none.h refuses to build with this on.
//   - a way to catch the fault. On Windows that means a compiler that speaks
//     __try/__except - MSVC or clang-cl, not MinGW.
//   - no d_m3FixedHeap: the memory comes from the system here, not from that heap.
//
// d_m3GuardedArenaSlots caps how many memories a process can have at once, since
// each one takes a slot of that reserved arena. It costs address space and nothing
// else - no memory is committed for a slot until a module asks for one. The arena is
// shared by every runtime in the process and is not itself locked, so two threads
// loading modules at the same time need the embedder to keep them apart, as the rest
// of loading already does.
#که_نه_تعریف_شوی d_m3_Guarded_حافظه
#  define d_m3_Guarded_حافظه                    0
#د_که_پای

#که_نه_تعریف_شوی d_m3GuardedArenaSlots
#  define d_m3GuardedArenaSlots                128
#د_که_پای

// What it is incompatible with is checked in m3_host.h, which is the first place
// that has seen enough of the configuration to say

#تعریف_کړه d_m3_Enable_کوډ_پاڼه_Ref_Counting          0       // not supported currently

#د_که_پای // m3_config_h
