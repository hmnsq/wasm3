//
//  Wasm3, high performance WebAssembly interpreter
//
//  Copyright © 2019 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ wasm3_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ wasm3_h

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_ਸੰਸਕਰਣ_MAJOR 0
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_ਸੰਸਕਰਣ_MINOR 9
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_ਸੰਸਕਰਣ_REV   2
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_ਸੰਸਕਰਣ       "0.9.2"

#ਅਨ੍ਤਰ੍ਭਾਵ <stddef.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdlib.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdint.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <inttypes.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdarg.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdbool.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stddef.h>

#ਅਨ੍ਤਰ੍ਭਾਵ "wasm3_defs.h"

// Constants
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_BACKTRACE_TRUNCATED      (IM3_Backtrace_ਚੌਖਟਾ)(SIZE_MAX)
// Gas units are ten-thousandths of a gas in ewasm's cost table.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_GAS_UNITS_ਪ੍ਰਤੀ_GAS        10000

#ਜੇ defined(__cplusplus)
ਬਾਹਰੀ "C" {
#ਜੇ_ਅੰਤ

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ ਅੱਖਰ* M3_ਨਤੀਜਾ;

ਬਣਤਰ M3Environment;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Environment* IM3Environment;
ਬਣਤਰ M3Runtime;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Runtime* IM3Runtime;
ਬਣਤਰ M3Module;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Module* IM3Module;
ਬਣਤਰ M3Function;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Function* IM3Function;
ਬਣਤਰ M3Global;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Global* IM3Global;
ਬਣਤਰ M3_ਟੈਗ;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਟੈਗ* IM3_ਟੈਗ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3ErrorInfo {
    M3_ਨਤੀਜਾ    ਨਤੀਜਾ;

    IM3Runtime  runtime;
    IM3Module   module;
    IM3Function function;

    ਅਚਲ ਅੱਖਰ* file;
    uint32_t    ਸਤਰ_2;

    ਅਚਲ ਅੱਖਰ* ਸੁਨੇਹਾ;
} M3ErrorInfo;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_Backtrace_ਚੌਖਟਾ {
    uint32_t                 module_ਵਿੱਥ;
    IM3Function              function;

    ਬਣਤਰ M3_Backtrace_ਚੌਖਟਾ* ਅਗਲਾ;
} M3_Backtrace_ਚੌਖਟਾ, *IM3_Backtrace_ਚੌਖਟਾ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3BacktraceInfo {
    IM3_Backtrace_ਚੌਖਟਾ ਚੌਖਟੇ;
    IM3_Backtrace_ਚੌਖਟਾ ਆਖਰੀ_ਚੌਖਟਾ;    // can be M3_BACKTRACE_TRUNCATED
} M3BacktraceInfo, *IM3BacktraceInfo;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਗਿਣਤੀ M3_Resource_ਹੱਦਬੰਦੀ {
    c_m3_ਹੱਦਬੰਦੀ_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ,       // Actual backing bytes across the runtime's memories.
    c_m3_ਹੱਦਬੰਦੀ_ਸਾਰਣੀ_Elements,     // Elements across the runtime's tables; imports count once.
    // Arm before compiling: only subsequently compiled bodies are instrumented.
    // Setting gas re-arms the full budget; usage may exceed it by one segment.
    // Exhaustion traps, or requests a pause in a suspendable runtime.
    c_m3_ਹੱਦਬੰਦੀ_Gas_Units,
    c_m3_ਹੱਦਬੰਦੀ_Continuations,     // Concurrent active cont.new stacks, recycled on completion.
} M3_Resource_ਹੱਦਬੰਦੀ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਗਿਣਤੀ M3_ਮੁੱਲ_Type {
    c_m3Type_none      = 0,
    c_m3Type_i32       = 1,
    c_m3Type_i64       = 2,
    c_m3Type_f32       = 3,
    c_m3Type_f64       = 4,

    // Opaque 16-byte slot used purely so wasm3 can PARSE modules
    // whose function signatures or local-variable declarations
    // mention v128 (the SIMD value type, wasm-encoded as 0x7B).
    // Actual v128 OPCODES still error at compile-time with
    // m3Err_unknownOpcode - we only avoid the parse-time
    // m3Err_invalidTypeId rejection. LLVM's auto-vectorizer emits
    // unused v128 locals into many `+simd128` modules even when no
    // SIMD op executes; without this slot wasm3 rejects every such
    // module before it ever sees a function body.
    c_m3Type_v128      = 5,

    // Reference values are opaque pointer-sized words and null is always 0.
    // A funcref holds an IM3Function; an externref holds a host-defined handle,
    // which the host is free to encode however it likes as long as 0 means null.
    c_m3Type_funcref   = 6,
    c_m3Type_externref = 7,

    // A caught exception, from the exception handling proposal. Holds a pointer
    // to a runtime-owned exception object, or 0 for the null reference. The
    // object belongs to the runtime and stays alive until the outermost
    // m3_Call() that produced it returns - a host that stashes one past that
    // point is holding a dangling reference.
    c_m3Type_exnref    = 8,

    // A first-class continuation reference, from the stack switching proposal.
    // Holds an IM3Continuation pointer, or 0 for null.
    c_m3Type_contref   = 9,

    // the number of concrete value types
    c_m3_Type_ਗਿਣਤੀ,

    // "not a valid type" - returned where a lookup or a mapping failed.
    c_m3Type_unknown = c_m3_Type_ਗਿਣਤੀ
} M3_ਮੁੱਲ_Type;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_Tagged_ਮੁੱਲ {
    M3_ਮੁੱਲ_Type type;
    ਸੰਘ M3_ਮੁੱਲ_Union {
        uint32_t    i32;
        uint64_t    i64;
        ਦਸ਼ਮਲਵ       f32;
        ਦੁੱਗਣਾ      f64;
        ਅਚਲ ਖਾਲੀ* ref;        // funcref / externref; NULL is the null reference
    } ਮੁੱਲ;
} M3_Tagged_ਮੁੱਲ, *IM3_Tagged_ਮੁੱਲ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3ImportInfo {
    ਅਚਲ ਅੱਖਰ* moduleUtf8;
    ਅਚਲ ਅੱਖਰ* ਖੇਤਰ_Utf8;
} M3ImportInfo, *IM3ImportInfo;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3ImportContext {
    ਖਾਲੀ*       userdata;
    IM3Function function;
} M3ImportContext, *IM3ImportContext;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ ਖਾਲੀ* (*M3RawCall)(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, ਖਾਲੀ* _mem);

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਨਤੀਜਾ (*M3SectionHandler)(IM3Module i_module, ਅਚਲ ਅੱਖਰ* name, ਅਚਲ uint8_t* ਸ਼ੁਰੂ, ਅਚਲ uint8_t* ਸਿਰਾ);


// -------------------------------------------------------------------------------------------------------------------------------
//  error codes
// -------------------------------------------------------------------------------------------------------------------------------

#ਜੇ defined(M3_IMPLEMENT_ERROR_ਲੜੀਆਂ)
#  ਜੇ defined(__cplusplus)
#    define d_m3ErrorConst(LABEL, STRING)      ਬਾਹਰੀ ਅਚਲ M3_ਨਤੀਜਾ m3Err_##LABEL = { STRING };
#  ਨਹੀਂਤਾਂ
#    define d_m3ErrorConst(LABEL, STRING)      ਅਚਲ M3_ਨਤੀਜਾ m3Err_##LABEL = { STRING };
#  endif
#ਨਹੀਂਤਾਂ
#  define d_m3ErrorConst(LABEL, STRING)        ਬਾਹਰੀ ਅਚਲ M3_ਨਤੀਜਾ m3Err_##LABEL;
#ਜੇ_ਅੰਤ

// -------------------------------------------------------------------------------------------------------------------------------
// clang-format off

d_m3ErrorConst(none,                           ਨਿਰਾ)

// general errors
d_m3ErrorConst(mallocFailed,                   "memory allocation failed")

// parse errors
d_m3ErrorConst(incompatible_Wasm_ਸੰਸਕਰਣ,        "incompatible Wasm binary version")
d_m3ErrorConst(wasmMalformed,                  "malformed Wasm binary")
d_m3ErrorConst(misorderedWasmSection,          "out of order Wasm section")
d_m3ErrorConst(wasmUnderrun,                   "underrun while parsing Wasm binary")
d_m3ErrorConst(wasmOverrun,                    "overrun while parsing Wasm binary")
d_m3ErrorConst(wasm_Missing_Init_ਵਿਅੰਜਕ,            "missing init_expr in Wasm binary")
d_m3ErrorConst(leb_ਉਛਾਲ,                    "LEB encoded value overflow")
d_m3ErrorConst(missingUTF8,                    "invalid length UTF-8 string")
d_m3ErrorConst(wasmSectionUnderrun,            "section underrun while parsing Wasm binary")
d_m3ErrorConst(wasmSectionOverrun,             "section overrun while parsing Wasm binary")
d_m3ErrorConst(invalidTypeId,                  "unknown value_type")
d_m3ErrorConst(too_Many_ਸਮ੍ਰਿਤੀ_Sections,          "only one memory per module is supported")
d_m3ErrorConst(too_Many_ਦਲੀਲਾਂ_Rets,                "too many arguments or return values")

// link errors
d_m3ErrorConst(moduleNotLinked,                "attempting to use module that is not loaded")
d_m3ErrorConst(moduleAlreadyLinked,            "attempting to bind module to multiple runtimes")
d_m3ErrorConst(functionLookupFailed,           "function lookup failed")
d_m3ErrorConst(ਟੈਗ_Lookup_Failed,                "tag lookup failed")
d_m3ErrorConst(functionImportMissing,          "missing imported function")
d_m3ErrorConst(unknownImport,                  "unknown import")
d_m3ErrorConst(incompatibleImportType,         "incompatible import type")

d_m3ErrorConst(malformedFunctionSignature,     "malformed function signature")
d_m3ErrorConst(functionSignatureMismatch,      "function signature mismatch")

// compilation errors
d_m3ErrorConst(noCompiler,                     "no compiler found for opcode")
d_m3ErrorConst(unknownOpcode,                  "unknown opcode")
d_m3ErrorConst(restrictedOpcode,               "restricted opcode")
d_m3ErrorConst(function_ਢੇਰ_ਉਛਾਲ,          "compiling function overran its stack height limit")
d_m3ErrorConst(function_ਢੇਰ_Underrun,          "compiling function underran the stack")
d_m3ErrorConst(malloc_Failed_ਕੋਡ_ਪੰਨਾ,           "memory allocation failed when acquiring a new M3 code page")
d_m3ErrorConst(settingImmutableGlobal,         "attempting to set an immutable global")
d_m3ErrorConst(typeMismatch,                   "incorrect type on stack")
d_m3ErrorConst(type_ਗਿਣਤੀ_Mismatch,              "incorrect value count on stack")

// validation errors. The wording follows the spec's own assert_invalid failure
d_m3ErrorConst(unknownType,                    "unknown type")
d_m3ErrorConst(unknownLabel,                   "unknown label")
d_m3ErrorConst(unknownLocal,                   "unknown local")
d_m3ErrorConst(unknownGlobal,                  "unknown global")
d_m3ErrorConst(unknownFunction,                "unknown function")
d_m3ErrorConst(unknown_ਸਾਰਣੀ,                   "unknown table")
d_m3ErrorConst(unknown_ਟੈਗ,                     "unknown tag")
d_m3ErrorConst(unknown_ਸਮ੍ਰਿਤੀ,                  "unknown memory")
d_m3ErrorConst(unknown_ਅੰਕੜੇ_Segment,             "unknown data segment")
d_m3ErrorConst(unknownElemSegment,             "unknown elem segment")
d_m3ErrorConst(ਅੰਕੜੇ_ਗਿਣਤੀ_Required,              "data count section required")
d_m3ErrorConst(invalidAlignment,               "alignment must not be larger than natural")
d_m3ErrorConst(invalidAtomicAlignment,         "atomic alignment must be natural")
d_m3ErrorConst(undeclaredFuncRef,              "undeclared function reference")

// runtime errors
d_m3ErrorConst(missing_Compiled_ਕੋਡ,            "function is missing compiled m3 code")
d_m3ErrorConst(wasm_ਸਮ੍ਰਿਤੀ_ਉਛਾਲ,             "runtime ran out of memory")
d_m3ErrorConst(global_ਸਮ੍ਰਿਤੀ_Not_Allocated,       "global memory is missing from a module")
d_m3ErrorConst(globa_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Bounds,          "global index is too large")
d_m3ErrorConst(ਦਲੀਲ_ਗਿਣਤੀ_Mismatch,          "argument count mismatch")
d_m3ErrorConst(ਦਲੀਲ_Type_Mismatch,           "argument type mismatch")
d_m3ErrorConst(globalLookupFailed,             "global lookup failed")
d_m3ErrorConst(globalTypeMismatch,             "global type mismatch")
d_m3ErrorConst(globalNotMutable,               "global is not mutable")
d_m3ErrorConst(ਸਮ੍ਰਿਤੀ_Guards_Unavailable,        "memory guards could not be installed")

// resource limit errors
d_m3ErrorConst(ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ_Exceeded,            "runtime memory limit exceeded")
d_m3ErrorConst(ਸਾਰਣੀ_ਹੱਦਬੰਦੀ_Exceeded,             "table elements limit exceeded")
d_m3ErrorConst(continuation_ਹੱਦਬੰਦੀ_Exceeded,      "continuation limit exceeded")
d_m3ErrorConst(resource_ਹੱਦਬੰਦੀ_ਹੇਠਾਂ_Usage,        "resource limit below current usage")
d_m3ErrorConst(resource_ਹੱਦਬੰਦੀ_Not_Supported,      "resource limit not supported in this build")
d_m3ErrorConst(unknown_Resource_ਹੱਦਬੰਦੀ,           "unknown resource limit")

// traps
d_m3ErrorConst(trap_ਬਾਹਰਲਾ_ਦਾ_Bounds_ਸਮ੍ਰਿਤੀ_Access,    "[trap] out of bounds memory access")
d_m3ErrorConst(trapUnalignedAtomic,            "[trap] unaligned atomic")
d_m3ErrorConst(trap_Expected_Shared_ਸਮ੍ਰਿਤੀ,       "[trap] expected shared memory")
d_m3ErrorConst(trapWaitForever,                "[trap] wait would block forever")
d_m3ErrorConst(trapDivisionByZero,             "[trap] integer divide by zero")
d_m3ErrorConst(trap_Integer_ਉਛਾਲ,            "[trap] integer overflow")
d_m3ErrorConst(trapIntegerConversion,          "[trap] invalid conversion to integer")
d_m3ErrorConst(trap_ਅਸਿੱਧਾ_Call_Type_Mismatch,   "[trap] indirect call type mismatch")
d_m3ErrorConst(trap_ਸਾਰਣੀ_ਸੂਚਕਾਂਕ_ਬਾਹਰਲਾ_ਦਾ_Range,       "[trap] undefined element")
d_m3ErrorConst(trap_ਸਾਰਣੀ_Element_Is_Null,         "[trap] uninitialized element")
d_m3ErrorConst(trapNullReference,              "[trap] null reference")
d_m3ErrorConst(trapNullFunctionRef,            "[trap] null function reference")
// call_indirect past the end of the table is "undefined element"; the table
// access instructions report an out of bounds access instead
d_m3ErrorConst(trap_ਸਾਰਣੀ_ਬਾਹਰਲਾ_ਦਾ_Bounds,           "[trap] out of bounds table access")
d_m3ErrorConst(trapWasiExit,                   "[trap] wasi program called exit")
d_m3ErrorConst(trapExit,                       "[trap] program called exit")
d_m3ErrorConst(trapAbort,                      "[trap] program called abort")
d_m3ErrorConst(trapUnreachable,                "[trap] unreachable")
d_m3ErrorConst(trapUnsupportedInstruction,     "[trap] unsupported instruction")
d_m3ErrorConst(trap_ਢੇਰ_ਉਛਾਲ,              "[trap] stack overflow")
d_m3ErrorConst(trap_ਬਾਹਰਲਾ_ਦਾ_Gas,                   "[trap] out of gas")
d_m3ErrorConst(trapUncaughtException,          "[trap] uncaught exception")
d_m3ErrorConst(trapContinuationConsumed,       "[trap] continuation already consumed")
d_m3ErrorConst(trapNullContinuationRef,        "[trap] null continuation reference")
d_m3ErrorConst(trap_Unhandled_Control_ਟੈਗ,        "[trap] unhandled control tag")

// Internal: the marker an in-flight exception rides back up the native stack
// on. Never escapes m3_Call - the outermost RunCodeChecked turns it into
// m3Err_trapUncaughtException.
d_m3ErrorConst(pendingException,               "[internal] exception in flight")
d_m3ErrorConst(continuationSuspended,          "[internal] continuation suspended")

// clang-format on

//-------------------------------------------------------------------------------------------------------------------------------
//  configuration, can be found in m3_config.h, m3_config_platforms.h, m3_core.h)
//-------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------------
//  global environment than can host multiple runtimes
//-------------------------------------------------------------------------------------------------------------------------------
IM3Environment m3_NewEnvironment (ਖਾਲੀ);

ਖਾਲੀ           m3_FreeEnvironment (IM3Environment i_environment);

ਖਾਲੀ           m3_ਸੈੱਟ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler);


//-------------------------------------------------------------------------------------------------------------------------------
//  execution context
//-------------------------------------------------------------------------------------------------------------------------------

IM3Runtime     m3_NewRuntime (IM3Environment io_environment,
                              uint32_t       i_ਢੇਰ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ,
                              ਖਾਲੀ*          i_userdata);

ਖਾਲੀ           m3_FreeRuntime (IM3Runtime i_runtime);

// The validator runs as a pre-pass of compiling a function body, so it
// follows lazy compilation: a body is checked the first time it is compiled.
// Turning it off means trusting the module - nothing then checks a body's
// types before it runs. On by default (a no-op in a build without validation).
ਖਾਲੀ           m3_ਸੈੱਟ_Validation (IM3Runtime i_runtime, bool i_enable);

// Zero means unlimited for allocation caps (memory, tables, continuations).
// Allocation limits below current usage are refused. Unsupported limits are
// refused even when the requested value is zero.
// Gas configures an execution budget (zero sets an empty/unmetered budget); arm
// a positive budget before compilation to instrument metered code. Gas saturates
// at INT64_MAX, continuations at UINT32_MAX. Setting an allocation limit leaves
// usage unchanged; setting gas starts a fresh execution budget.
M3_ਨਤੀਜਾ       m3_ਸੈੱਟ_Resource_ਹੱਦਬੰਦੀ (IM3Runtime i_runtime, M3_Resource_ਹੱਦਬੰਦੀ i_ਹੱਦਬੰਦੀ, uint64_t i_ਮੁੱਲ);
// Returns the effective (possibly saturated) limit; unsupported/unset limits are zero.
uint64_t       m3_Get_Resource_ਹੱਦਬੰਦੀ (IM3Runtime i_runtime, M3_Resource_ਹੱਦਬੰਦੀ i_ਹੱਦਬੰਦੀ);
// Usage is tracked even without a limit (for gas, requires arming before compilation).
// Unsupported and unknown keys return zero.
uint64_t       m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_ਹੱਦਬੰਦੀ i_ਹੱਦਬੰਦੀ);

// A memory belongs to the module that declares it, so these take the module
// rather than the runtime - a runtime can hold several modules, each with
// its own memories. Returns NULL when the module has no memory at that
// index; o_memorySizeInBytes is set either way.
//
// Sizes are size_t, not uint32_t: a linear memory may be a full 4 GiB, which
// is one byte too many to count in 32 bits.
uint8_t*       m3_Get_ਸਮ੍ਰਿਤੀ (IM3Module i_module,
                             size_t*   o_ਸਮ੍ਰਿਤੀ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ,
                             uint32_t  i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ);

size_t         m3_Get_ਸਮ੍ਰਿਤੀ_ਮਾਪ (IM3Module i_module,
                                 uint32_t  i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ);

// Size of the memory a pointer addresses into - specifically the _mem a raw
// function is handed, which is the memory of whichever module is calling,
// and need not be any particular module's. Used by m3ApiCheckMem.
size_t         m3_Get_ਸਮ੍ਰਿਤੀ_ਮਾਪ_At (ਅਚਲ ਖਾਲੀ* i_ਸਮ੍ਰਿਤੀ);

ਖਾਲੀ*          m3_Get_User_ਅੰਕੜੇ (IM3Runtime i_runtime);

// The index of the memory the module exports under i_name. A host module
// whose ABI names the memory it addresses looks it up this way - WASI names
// it "memory". Returns m3Err_unknownMemory when no memory is exported
// under that name.
M3_ਨਤੀਜਾ       m3_ਲੱਭੋ_Exported_ਸਮ੍ਰਿਤੀ (IM3Module         i_module,
                                      ਅਚਲ ਅੱਖਰ* ਅਚਲ i_name,
                                      uint32_t*         o_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ);

// Pin which of the module's memories the host functions imported from
// i_importModule address ("*" for every namespace, as in
// m3_LinkRawFunction). Applies to imports already bound to a host function,
// so call it after linking them. Left alone, a host function addresses
// memory 0 - what a bare i32 guest pointer means when nothing says
// otherwise - so this is only needed by a host module that names a memory.
M3_ਨਤੀਜਾ       m3_Bind_Import_ਸਮ੍ਰਿਤੀ (IM3Module         io_module,
                                    ਅਚਲ ਅੱਖਰ* ਅਚਲ i_importModule,
                                    uint32_t          i_ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ);

//-------------------------------------------------------------------------------------------------------------------------------
//  modules
//-------------------------------------------------------------------------------------------------------------------------------

// i_wasmBytes data must be persistent during the lifetime of the module
M3_ਨਤੀਜਾ       m3_ParseModule (IM3Environment       i_environment,
                               IM3Module*           o_module,
                               ਅਚਲ uint8_t* ਅਚਲ i_wasm_ਬਾਈਟਾਂ,
                               uint32_t             i_ਅੰਕ_Wasm_ਬਾਈਟਾਂ);

// Only a module that was never handed to m3_LoadModule needs to be freed.
ਖਾਲੀ           m3_FreeModule (IM3Module i_module);

//  Transfers ownership of the module to the runtime - whether or not it
//  succeeds. A failed instantiation can already have written this module's
//  functions into a table another module owns, and the spec keeps whatever
//  it managed to do, so those entries stay callable; the module has to
//  outlive the failure for them not to dangle. m3_FreeRuntime releases it.
//  Do not call m3_FreeModule on a module after passing it here.
M3_ਨਤੀਜਾ       m3_ਲੱਦੋ_Module (IM3Runtime io_runtime, IM3Module io_module);

// Optional, compiles all functions in the module
M3_ਨਤੀਜਾ       m3_CompileModule (IM3Module io_module);

// Calling m3_RunStart is optional
M3_ਨਤੀਜਾ       m3_ਲੜੀਵਾਰ_ਸ਼ੁਰੂ (IM3Module i_module);

// Arguments and return values are passed in and out through the stack pointer _sp.
// Placeholder return value slots are first and arguments after. So, the first argument is at _sp [numReturns]
// Return values should be written into _sp [0] to _sp [num_returns - 1]
M3_ਨਤੀਜਾ       m3_LinkRawFunction (IM3Module         io_module,
                                   ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName,
                                   ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName,
                                   ਅਚਲ ਅੱਖਰ* ਅਚਲ i_signature,
                                   M3RawCall         i_function);

M3_ਨਤੀਜਾ       m3_LinkRawFunctionEx (IM3Module         io_module,
                                     ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName,
                                     ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName,
                                     ਅਚਲ ਅੱਖਰ* ਅਚਲ i_signature,
                                     M3RawCall         i_function,
                                     ਅਚਲ ਖਾਲੀ*       i_userdata);

// supplies the value of an imported global, regardless of its mutability
M3_ਨਤੀਜਾ       m3_LinkGlobal (IM3Module            io_module,
                              ਅਚਲ ਅੱਖਰ* ਅਚਲ    i_moduleName,
                              ਅਚਲ ਅੱਖਰ* ਅਚਲ    i_globalName,
                              ਅਚਲ IM3_Tagged_ਮੁੱਲ i_ਮੁੱਲ);

ਅਚਲ ਅੱਖਰ*    m3_GetModuleName (IM3Module i_module);
ਖਾਲੀ           m3_ਸੈੱਟ_Module_Name (IM3Module i_module, ਅਚਲ ਅੱਖਰ* name);
IM3Runtime     m3_GetModuleRuntime (IM3Module i_module);

// The module registered under i_moduleName, or NULL. Most recently loaded
// first, so a name registered twice names the newer module.
IM3Module      m3_ਲੱਭੋ_Module (IM3Runtime        i_runtime,
                              ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName);

//-------------------------------------------------------------------------------------------------------------------------------
//  globals
//-------------------------------------------------------------------------------------------------------------------------------
IM3Global      m3_ਲੱਭੋ_Global (IM3Module         io_module,
                              ਅਚਲ ਅੱਖਰ* ਅਚਲ i_globalName);

M3_ਨਤੀਜਾ       m3_GetGlobal (IM3Global      i_global,
                             IM3_Tagged_ਮੁੱਲ o_ਮੁੱਲ);

M3_ਨਤੀਜਾ       m3_ਸੈੱਟ_Global (IM3Global            i_global,
                             ਅਚਲ IM3_Tagged_ਮੁੱਲ i_ਮੁੱਲ);

M3_ਮੁੱਲ_Type    m3_GetGlobalType (IM3Global i_global);

//-------------------------------------------------------------------------------------------------------------------------------
//  functions
//-------------------------------------------------------------------------------------------------------------------------------
M3_ਨਤੀਜਾ       m3_Yield (ਖਾਲੀ);

// o_function is valid during the lifetime of the originating runtime.
// m3_FindFunction searches every module loaded into the runtime, most
// recently loaded first; m3_FindFunctionIn searches just the one, which is
// what naming a module's export means.
M3_ਨਤੀਜਾ       m3_ਲੱਭੋ_Function (IM3Function*      o_function,
                                IM3Runtime        i_runtime,
                                ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName);
M3_ਨਤੀਜਾ       m3_ਲੱਭੋ_Function_ਵਿੱਚ (IM3Function*      o_function,
                                  IM3Module         i_module,
                                  ਅਚਲ ਅੱਖਰ* ਅਚਲ i_functionName);
M3_ਨਤੀਜਾ       m3_Get_ਸਾਰਣੀ_Function (IM3Function* o_function,
                                    IM3Module    i_module,
                                    uint32_t     i_ਸੂਚਕਾਂਕ);

uint32_t       m3_Get_Arg_ਗਿਣਤੀ (IM3Function i_function);
uint32_t       m3_Get_Ret_ਗਿਣਤੀ (IM3Function i_function);
M3_ਮੁੱਲ_Type    m3_GetArgType (IM3Function i_function, uint32_t i_ਸੂਚਕਾਂਕ);
M3_ਮੁੱਲ_Type    m3_GetRetType (IM3Function i_function, uint32_t i_ਸੂਚਕਾਂਕ);

M3_ਨਤੀਜਾ       m3_CallV (IM3Function i_function, ...);
M3_ਨਤੀਜਾ       m3_CallVL (IM3Function i_function, va_list i_ਦਲੀਲਾਂ);
M3_ਨਤੀਜਾ       m3_Call (IM3Function i_function, uint32_t i_argc, ਅਚਲ ਖਾਲੀ* i_argptrs[]);
M3_ਨਤੀਜਾ       m3_CallArgv (IM3Function i_function, uint32_t i_argc, ਅਚਲ ਅੱਖਰ* i_argv[]);

M3_ਨਤੀਜਾ       m3_GetResultsV (IM3Function i_function, ...);
M3_ਨਤੀਜਾ       m3_GetResultsVL (IM3Function i_function, va_list o_rets);
M3_ਨਤੀਜਾ       m3_GetResults (IM3Function i_function, uint32_t i_retc, ਅਚਲ ਖਾਲੀ* o_retptrs[]);


ਖਾਲੀ           m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info);
ਖਾਲੀ           m3_ResetErrorInfo (IM3Runtime i_runtime);

ਅਚਲ ਅੱਖਰ*    m3_GetFunctionName (IM3Function i_function);
IM3Module      m3_GetFunctionModule (IM3Function i_function);

//-------------------------------------------------------------------------------------------------------------------------------
//  exceptions
//-------------------------------------------------------------------------------------------------------------------------------
// A tag is the identity of an exception: a catch clause names one, and catches only
// what was thrown with it. Its parameters are the types of the payload the exception
// carries. A tag lives as long as the runtime its module was loaded into, or the
// runtime m3_NewTag made it in.

// The tag i_module exports as i_tagName, or NULL.
IM3_ਟੈਗ         m3_ਲੱਭੋ_ਟੈਗ (IM3Module         i_module,
                           ਅਚਲ ਅੱਖਰ* ਅਚਲ i_ਟੈਗ_Name);

// A tag of the host's own, typed by a signature in m3_LinkRawFunction's notation:
// "v(iI)" carries an i32 and an i64. No module can name it, so only catch_all and
// catch_all_ref catch what is thrown with it - until m3_LinkTag hands it to one.
M3_ਨਤੀਜਾ       m3_New_ਟੈਗ (IM3Runtime        io_runtime,
                          IM3_ਟੈਗ*           o_ਟੈਗ,
                          ਅਚਲ ਅੱਖਰ* ਅਚਲ i_signature);

// Satisfies a tag import with i_tag, which must be of the same type. Code refers
// to a tag as whatever it resolved to when it was compiled, so link before
// calling anything that throws or catches it. "*" matches any module name.
M3_ਨਤੀਜਾ       m3_Link_ਟੈਗ (IM3Module         io_module,
                           ਅਚਲ ਅੱਖਰ* ਅਚਲ i_moduleName,
                           ਅਚਲ ਅੱਖਰ* ਅਚਲ i_ਟੈਗ_Name,
                           IM3_ਟੈਗ            i_ਟੈਗ);

uint32_t       m3_Get_ਟੈਗ_Arg_ਗਿਣਤੀ (IM3_ਟੈਗ i_ਟੈਗ);
M3_ਮੁੱਲ_Type    m3_Get_ਟੈਗ_Arg_Type (IM3_ਟੈਗ i_ਟੈਗ, uint32_t i_ਸੂਚਕਾਂਕ);

// Throws an exception from inside a raw host function, which returns what this
// returns: the exception, on its way to the nearest try_table that catches it, or
// a trap if it could not be thrown. i_argptrs point at the payload values, as
// m3_Call's arguments do. Only a tag without results can be thrown.
M3_ਨਤੀਜਾ       m3_ThrowException (IM3Runtime  io_runtime,
                                  IM3_ਟੈਗ      i_ਟੈਗ,
                                  uint32_t    i_argc,
                                  ਅਚਲ ਖਾਲੀ* i_argptrs[]);

// The exception that ended the most recent call or resume on this runtime, when
// it ended with m3Err_trapUncaughtException; NULL after any other outcome. The
// payload is a copy, so it outlives the call, but an exnref within it does not.
IM3_ਟੈਗ         m3_Get_Exception_ਟੈਗ (IM3Runtime i_runtime);
M3_ਨਤੀਜਾ       m3_Get_Exception_ਦਲੀਲਾਂ (IM3Runtime  i_runtime,
                                    uint32_t    i_argc,
                                    ਅਚਲ ਖਾਲੀ* o_argptrs[]);

//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਨਤੀਜਾ (*M3SnapshotWriter)(ਅਚਲ ਖਾਲੀ* i_ਅੰਕੜੇ, size_t i_ਮਾਪ, ਖਾਲੀ* i_userdata);
ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਨਤੀਜਾ (*M3SnapshotReader)(ਖਾਲੀ* o_ਬਫਰ, size_t i_ਮਾਪ, ਖਾਲੀ* i_userdata);

// A suspendable runtime can pause a call part way and pick it up again with
// m3_ResumeRuntime - in this process, or through a snapshot in another. Make it
// suspendable before anything compiles: only code compiled that way has the
// checks that pause.
//
// m3_RequestSuspend asks for a pause, which takes effect at the next pause
// point: a loop's back edge, or the start of a function's body. Every loop and
// every recursion passes one. A resume goes on past the point it paused at, so
// asking again before each m3_ResumeRuntime steps from one to the next.
ਖਾਲੀ     m3_ਸੈੱਟ_Suspendable (IM3Runtime io_runtime, bool i_suspendable);
ਖਾਲੀ     m3_RequestSuspend (IM3Runtime io_runtime);
bool     m3_IsSuspended (IM3Runtime i_runtime);

// Saves the paused invocation, or a postmortem of the module the last call
// entered when nothing is paused; with neither, there is nothing to save.
M3_ਨਤੀਜਾ m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ਖਾਲੀ* i_userdata);
// Restores into a freshly instantiated module: loaded and linked, with none of
// its code run yet - its start function included. A restore that fails part way
// leaves a module that refuses to run, resume or be saved again, and has to be
// thrown away with its runtime.
M3_ਨਤੀਜਾ m3_ਲੱਦੋ_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ਖਾਲੀ* i_userdata);

M3_ਨਤੀਜਾ m3_Save_Snapshot_ਮੰਜ਼ਿਲ_ਬਫਰ (IM3Runtime io_runtime, ਖਾਲੀ** o_ਬਾਈਟਾਂ, size_t* o_ਮਾਪ);
// Accepts a standalone container, a Wasm module with a default snapshot, or
// the bare section stream returned by m3_GetEmbeddedSnapshot.
M3_ਨਤੀਜਾ m3_ਲੱਦੋ_Snapshot_From_ਬਫਰ (IM3Runtime io_runtime, IM3Module i_module, ਅਚਲ ਖਾਲੀ* i_ਬਾਈਟਾਂ, size_t i_ਮਾਪ);

// A snapshot can also ride inside the module it belongs to, as a custom
// section named "snapshot" or "snapshot.<name>". i_name selects one; NULL or
// "" means the unnamed default. Two sections of the same name are refused
// when that name is selected, not when the module is parsed.
bool     m3_HasSnapshot (IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name);

// Points o_bytes at the embedded snapshot, which belongs to the module and
// lives as long as the bytes it was parsed from. The payload has no standalone header.
M3_ਨਤੀਜਾ m3_GetEmbeddedSnapshot (IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name,
                                 ਅਚਲ ਖਾਲੀ** o_ਬਾਈਟਾਂ, size_t* o_ਮਾਪ);

M3_ਨਤੀਜਾ m3_ਲੱਦੋ_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name);

// Writes the module's bytes with the paused invocation embedded in them,
// replacing any snapshot of the same name already there. A postmortem is not
// embedded: with nothing paused, this fails. Release o_bytes with free().
M3_ਨਤੀਜਾ m3_Save_Snapshot_ਮੰਜ਼ਿਲ_Module (IM3Runtime io_runtime, IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name,
                                  ਖਾਲੀ** o_ਬਾਈਟਾਂ, size_t* o_ਮਾਪ);

// What a snapshot cannot name on its own belongs to the embedder: an externref,
// and whatever the program depends on outside its Wasm state - open files,
// clocks, anything its imports keep. Any of these may be left NULL.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3SnapshotHooks {
    // An externref as a number the embedder can turn back into the same
    // reference in another process. Without nameExternRef a non-null externref
    // refuses the save; without bindExternRef a snapshot holding one is refused.
    // UINT64_MAX is not a name: it is what a null reference is written as.
    M3_ਨਤੀਜਾ (*nameExternRef)(ਖਾਲੀ* i_userdata, ਖਾਲੀ* i_reference, uint64_t* o_name);
    M3_ਨਤੀਜਾ (*bindExternRef)(ਖਾਲੀ* i_userdata, uint64_t i_name, ਖਾਲੀ** o_reference);

    // The embedder's own state, carried after the program's. saveHostState
    // writes it through i_writer; loadHostState reads exactly those i_size bytes
    // back through i_reader. A snapshot that carries some is refused by a
    // runtime with no loadHostState.
    M3_ਨਤੀਜਾ (*saveHostState)(ਖਾਲੀ* i_userdata, M3SnapshotWriter i_writer, ਖਾਲੀ* i_writer_ਅੰਕੜੇ);
    M3_ਨਤੀਜਾ (*ਲੱਦੋ_Host_State_2)(ਖਾਲੀ* i_userdata, M3SnapshotReader i_reader, ਖਾਲੀ* i_reader_ਅੰਕੜੇ, size_t i_ਮਾਪ);

    ਖਾਲੀ* userdata;
} M3SnapshotHooks;

ਖਾਲੀ             m3_ਸੈੱਟ_Snapshot_Hooks (IM3Runtime io_runtime, ਅਚਲ M3SnapshotHooks* i_hooks);

M3_ਨਤੀਜਾ         m3_ResumeRuntime (IM3Runtime io_runtime);


//-------------------------------------------------------------------------------------------------------------------------------
//  debug info
//-------------------------------------------------------------------------------------------------------------------------------

ਖਾਲੀ             m3_PrintRuntimeInfo (IM3Runtime i_runtime);
ਖਾਲੀ             m3_PrintM3Info (ਖਾਲੀ);
ਖਾਲੀ             m3_PrintProfilerInfo (ਖਾਲੀ);

// The runtime owns the backtrace, do not free the backtrace you obtain. Returns NULL if there's no backtrace.
IM3BacktraceInfo m3_GetBacktrace (IM3Runtime i_runtime);

//-------------------------------------------------------------------------------------------------------------------------------
//  raw function definition helpers
//-------------------------------------------------------------------------------------------------------------------------------

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(offset)   (ਖਾਲੀ*)((uint8_t*)_mem + (uint32_t)(offset))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(ptr)      (uint32_t)((uint8_t*)ptr - (uint8_t*)_mem)

// Each of these walks _sp as it goes, so an entry point has to name every
// argument it is handed whether or not its implementation has a use for the
// value - which is why the declarations carry M3_UNUSED rather than the build
// turning the warning off for everything.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiReturnType(TYPE)                 TYPE* raw_return M3_ਅਣਵਰਤਿਆ = ((TYPE*) (_sp++));
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_Api_Multi_ਮੁੱਲ_Return_Type(TYPE, NAME) TYPE* NAME M3_ਅਣਵਰਤਿਆ = ((TYPE*) (_sp++));
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiGetArg(TYPE, NAME)               TYPE NAME M3_ਅਣਵਰਤਿਆ = \
   (ਆਕਾਰ(TYPE) >= ਆਕਾਰ(uint32_t)) ? \
   (*((TYPE *)(_sp++))) : \
   ((TYPE)(uintptr_t)(*((uint32_t *)(_sp++))));
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiGetArgMem(TYPE, NAME)            TYPE NAME M3_ਅਣਵਰਤਿਆ = (TYPE)m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(* ((uint32_t *) (_sp++)));

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiIsNullPtr(addr)       ((ਖਾਲੀ*)(addr) <= _mem)

// Whether [addr, addr+len) lies inside the memory _mem points at. Written as
// two subtractions rather than base + size and addr + len, so that neither a
// memory occupying the top of the address space nor a wild length can carry the
// comparison past where it wraps. Its own block, so the locals go out of scope
// with it.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiCheckMem(addr, len)                                                   \
   {   uintptr_t m3_ਅਧਾਰ = (uintptr_t)(_mem);                                     \
       uintptr_t _m3_addr = (uintptr_t)(ਖਾਲੀ*)(addr);                              \
       size_t    m3_ਮਾਪ = m3_Get_ਸਮ੍ਰਿਤੀ_ਮਾਪ_At(_mem);                              \
       ਜੇ (M3_UNLIKELY(_m3_addr < m3_ਅਧਾਰ ||                                      \
                       (size_t)(_m3_addr - m3_ਅਧਾਰ) > m3_ਮਾਪ ||                 \
                       (uint64_t)(len) > (uint64_t)(m3_ਮਾਪ - (size_t)(_m3_addr - m3_ਅਧਾਰ)))) \
           m3ApiTrap(m3_Err_trap_ਬਾਹਰਲਾ_ਦਾ_Bounds_ਸਮ੍ਰਿਤੀ_Access);                           \
   }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiRawFunction(NAME)               ਅਚਲ ਖਾਲੀ * NAME (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, ਖਾਲੀ * _mem)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiReturn(ਮੁੱਲ_2)                   { *raw_return = (ਮੁੱਲ_2); ਵਾਪਸ m3Err_none;}
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_Api_Multi_ਮੁੱਲ_Return(NAME, ਮੁੱਲ_2)   { *NAME = (ਮੁੱਲ_2); }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiTrap(ਮੁੱਲ_2)                     { ਵਾਪਸ ਮੁੱਲ_2; }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiThrow(ਟੈਗ_2, ARGC, ARGPTRS)       { ਵਾਪਸ m3_ThrowException(runtime, ਟੈਗ_2, ARGC, ARGPTRS); }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3ApiSuccess()                       { ਵਾਪਸ m3Err_none; }

#ਜੇ defined(M3_BIG_ENDIAN)
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        m3_bswap16((* (uint16_t *)(ptr)))
#  define m3ApiReadMem32(ptr)        m3_bswap32((* (uint32_t *)(ptr)))
#  define m3ApiReadMem64(ptr)        m3_bswap64((* (uint64_t *)(ptr)))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr)  = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = m3_bswap16((val)); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = m3_bswap32((val)); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = m3_bswap64((val)); }
#ਨਹੀਂਤਾਂ
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        (* (uint16_t *)(ptr))
#  define m3ApiReadMem32(ptr)        (* (uint32_t *)(ptr))
#  define m3ApiReadMem64(ptr)        (* (uint64_t *)(ptr))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr) = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = (val); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = (val); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = (val); }
#ਜੇ_ਅੰਤ

#ਜੇ defined(__cplusplus)
}
#ਜੇ_ਅੰਤ

#ਜੇ_ਅੰਤ // wasm3_h
