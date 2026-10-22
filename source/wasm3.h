//
//  Wasm3, high performance WebAssembly interpreter
//
//  Copyright © 2019 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#ifndef wasm3_h
#define wasm3_h

#define M3_VERSION_MAJOR 0
#define M3_VERSION_MINOR 9
#define M3_VERSION_REV   2
#define M3_VERSION       "0.9.2"

#അന്തര്ഭാവ <stddef.h>
#അന്തര്ഭാവ <stdlib.h>
#അന്തര്ഭാവ <stdint.h>
#അന്തര്ഭാവ <inttypes.h>
#അന്തര്ഭാവ <stdarg.h>
#അന്തര്ഭാവ <stdbool.h>
#അന്തര്ഭാവ <stddef.h>

#അന്തര്ഭാവ "wasm3_defs.h"

// Constants
#define M3_BACKTRACE_TRUNCATED      (IM3BacktraceFrame)(SIZE_MAX)
// Gas units are ten-thousandths of a gas in ewasm's cost table.
#define M3_GAS_UNITS_PER_GAS        10000

#എങ്കിൽ defined(__cplusplus)
ബാഹ്യ "C" {
#endif

തരനിർവചനം സ്ഥിരം അക്ഷരം* M3Result;

ഘടന M3Environment;
തരനിർവചനം ഘടന M3Environment* IM3Environment;
ഘടന M3Runtime;
തരനിർവചനം ഘടന M3Runtime* IM3Runtime;
ഘടന M3Module;
തരനിർവചനം ഘടന M3Module* IM3Module;
ഘടന M3Function;
തരനിർവചനം ഘടന M3Function* IM3Function;
ഘടന M3Global;
തരനിർവചനം ഘടന M3Global* IM3Global;
ഘടന M3Tag;
തരനിർവചനം ഘടന M3Tag* IM3Tag;

തരനിർവചനം ഘടന M3ErrorInfo {
    M3Result    result;

    IM3Runtime  runtime;
    IM3Module   module;
    IM3Function function;

    സ്ഥിരം അക്ഷരം* file;
    uint32_t    വരി_2;

    സ്ഥിരം അക്ഷരം* message;
} M3ErrorInfo;

തരനിർവചനം ഘടന M3BacktraceFrame {
    uint32_t                 moduleOffset;
    IM3Function              function;

    ഘടന M3BacktraceFrame* next;
} M3BacktraceFrame, *IM3BacktraceFrame;

തരനിർവചനം ഘടന M3BacktraceInfo {
    IM3BacktraceFrame frames;
    IM3BacktraceFrame lastFrame;    // can be M3_BACKTRACE_TRUNCATED
} M3BacktraceInfo, *IM3BacktraceInfo;

തരനിർവചനം എണ്ണൽ M3ResourceLimit {
    c_m3Limit_MemoryBytes,       // Actual backing bytes across the runtime's memories.
    c_m3Limit_TableElements,     // Elements across the runtime's tables; imports count once.
    // Arm before compiling: only subsequently compiled bodies are instrumented.
    // Setting gas re-arms the full budget; usage may exceed it by one segment.
    // Exhaustion traps, or requests a pause in a suspendable runtime.
    c_m3Limit_GasUnits,
    c_m3Limit_Continuations,     // Concurrent active cont.new stacks, recycled on completion.
} M3ResourceLimit;

തരനിർവചനം എണ്ണൽ M3_മൂല്യം_Type {
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
    c_m3_Type_എണ്ണം,

    // "not a valid type" - returned where a lookup or a mapping failed.
    c_m3Type_unknown = c_m3_Type_എണ്ണം
} M3_മൂല്യം_Type;

തരനിർവചനം ഘടന M3_Tagged_മൂല്യം {
    M3_മൂല്യം_Type type;
    സംഘം M3_മൂല്യം_Union {
        uint32_t    i32;
        uint64_t    i64;
        ദശാംശം       f32;
        ഇരട്ട      f64;
        സ്ഥിരം ശൂന്യം* ref;        // funcref / externref; NULL is the null reference
    } മൂല്യം;
} M3_Tagged_മൂല്യം, *IM3_Tagged_മൂല്യം;

തരനിർവചനം ഘടന M3ImportInfo {
    സ്ഥിരം അക്ഷരം* moduleUtf8;
    സ്ഥിരം അക്ഷരം* fieldUtf8;
} M3ImportInfo, *IM3ImportInfo;

തരനിർവചനം ഘടന M3ImportContext {
    ശൂന്യം*       userdata;
    IM3Function function;
} M3ImportContext, *IM3ImportContext;

തരനിർവചനം സ്ഥിരം ശൂന്യം* (*M3RawCall)(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, ശൂന്യം* _mem);

തരനിർവചനം M3Result (*M3SectionHandler)(IM3Module i_module, സ്ഥിരം അക്ഷരം* name, സ്ഥിരം uint8_t* start, സ്ഥിരം uint8_t* end);


// -------------------------------------------------------------------------------------------------------------------------------
//  error codes
// -------------------------------------------------------------------------------------------------------------------------------

#എങ്കിൽ defined(M3_IMPLEMENT_ERROR_STRINGS)
#  എങ്കിൽ defined(__cplusplus)
#    define d_m3ErrorConst(LABEL, STRING)      ബാഹ്യ സ്ഥിരം M3Result m3Err_##LABEL = { STRING };
#  അല്ലെങ്കിൽ
#    define d_m3ErrorConst(LABEL, STRING)      സ്ഥിരം M3Result m3Err_##LABEL = { STRING };
#  endif
#അല്ലെങ്കിൽ
#  define d_m3ErrorConst(LABEL, STRING)        ബാഹ്യ സ്ഥിരം M3Result m3Err_##LABEL;
#endif

// -------------------------------------------------------------------------------------------------------------------------------
// clang-format off

d_m3ErrorConst(none,                           ശൂന്യ)

// general errors
d_m3ErrorConst(mallocFailed,                   "memory allocation failed")

// parse errors
d_m3ErrorConst(incompatibleWasmVersion,        "incompatible Wasm binary version")
d_m3ErrorConst(wasmMalformed,                  "malformed Wasm binary")
d_m3ErrorConst(misorderedWasmSection,          "out of order Wasm section")
d_m3ErrorConst(wasmUnderrun,                   "underrun while parsing Wasm binary")
d_m3ErrorConst(wasmOverrun,                    "overrun while parsing Wasm binary")
d_m3ErrorConst(wasm_Missing_Init_വ്യഞ്ജകം,            "missing init_expr in Wasm binary")
d_m3ErrorConst(lebOverflow,                    "LEB encoded value overflow")
d_m3ErrorConst(missingUTF8,                    "invalid length UTF-8 string")
d_m3ErrorConst(wasmSectionUnderrun,            "section underrun while parsing Wasm binary")
d_m3ErrorConst(wasmSectionOverrun,             "section overrun while parsing Wasm binary")
d_m3ErrorConst(invalidTypeId,                  "unknown value_type")
d_m3ErrorConst(tooManyMemorySections,          "only one memory per module is supported")
d_m3ErrorConst(tooManyArgsRets,                "too many arguments or return values")

// link errors
d_m3ErrorConst(moduleNotLinked,                "attempting to use module that is not loaded")
d_m3ErrorConst(moduleAlreadyLinked,            "attempting to bind module to multiple runtimes")
d_m3ErrorConst(functionLookupFailed,           "function lookup failed")
d_m3ErrorConst(tagLookupFailed,                "tag lookup failed")
d_m3ErrorConst(functionImportMissing,          "missing imported function")
d_m3ErrorConst(unknownImport,                  "unknown import")
d_m3ErrorConst(incompatibleImportType,         "incompatible import type")

d_m3ErrorConst(malformedFunctionSignature,     "malformed function signature")
d_m3ErrorConst(functionSignatureMismatch,      "function signature mismatch")

// compilation errors
d_m3ErrorConst(noCompiler,                     "no compiler found for opcode")
d_m3ErrorConst(unknownOpcode,                  "unknown opcode")
d_m3ErrorConst(restrictedOpcode,               "restricted opcode")
d_m3ErrorConst(function_അട്ടി_Overflow,          "compiling function overran its stack height limit")
d_m3ErrorConst(function_അട്ടി_Underrun,          "compiling function underran the stack")
d_m3ErrorConst(mallocFailedCodePage,           "memory allocation failed when acquiring a new M3 code page")
d_m3ErrorConst(settingImmutableGlobal,         "attempting to set an immutable global")
d_m3ErrorConst(typeMismatch,                   "incorrect type on stack")
d_m3ErrorConst(type_എണ്ണം_Mismatch,              "incorrect value count on stack")

// validation errors. The wording follows the spec's own assert_invalid failure
d_m3ErrorConst(unknownType,                    "unknown type")
d_m3ErrorConst(unknownLabel,                   "unknown label")
d_m3ErrorConst(unknownLocal,                   "unknown local")
d_m3ErrorConst(unknownGlobal,                  "unknown global")
d_m3ErrorConst(unknownFunction,                "unknown function")
d_m3ErrorConst(unknownTable,                   "unknown table")
d_m3ErrorConst(unknownTag,                     "unknown tag")
d_m3ErrorConst(unknownMemory,                  "unknown memory")
d_m3ErrorConst(unknownDataSegment,             "unknown data segment")
d_m3ErrorConst(unknownElemSegment,             "unknown elem segment")
d_m3ErrorConst(data_എണ്ണം_Required,              "data count section required")
d_m3ErrorConst(invalidAlignment,               "alignment must not be larger than natural")
d_m3ErrorConst(invalidAtomicAlignment,         "atomic alignment must be natural")
d_m3ErrorConst(undeclaredFuncRef,              "undeclared function reference")

// runtime errors
d_m3ErrorConst(missingCompiledCode,            "function is missing compiled m3 code")
d_m3ErrorConst(wasmMemoryOverflow,             "runtime ran out of memory")
d_m3ErrorConst(globalMemoryNotAllocated,       "global memory is missing from a module")
d_m3ErrorConst(globaIndexOutOfBounds,          "global index is too large")
d_m3ErrorConst(argument_എണ്ണം_Mismatch,          "argument count mismatch")
d_m3ErrorConst(argumentTypeMismatch,           "argument type mismatch")
d_m3ErrorConst(globalLookupFailed,             "global lookup failed")
d_m3ErrorConst(globalTypeMismatch,             "global type mismatch")
d_m3ErrorConst(globalNotMutable,               "global is not mutable")
d_m3ErrorConst(memoryGuardsUnavailable,        "memory guards could not be installed")

// resource limit errors
d_m3ErrorConst(memoryLimitExceeded,            "runtime memory limit exceeded")
d_m3ErrorConst(tableLimitExceeded,             "table elements limit exceeded")
d_m3ErrorConst(continuationLimitExceeded,      "continuation limit exceeded")
d_m3ErrorConst(resourceLimitBelowUsage,        "resource limit below current usage")
d_m3ErrorConst(resourceLimitNotSupported,      "resource limit not supported in this build")
d_m3ErrorConst(unknownResourceLimit,           "unknown resource limit")

// traps
d_m3ErrorConst(trapOutOfBoundsMemoryAccess,    "[trap] out of bounds memory access")
d_m3ErrorConst(trapUnalignedAtomic,            "[trap] unaligned atomic")
d_m3ErrorConst(trapExpectedSharedMemory,       "[trap] expected shared memory")
d_m3ErrorConst(trapWaitForever,                "[trap] wait would block forever")
d_m3ErrorConst(trapDivisionByZero,             "[trap] integer divide by zero")
d_m3ErrorConst(trapIntegerOverflow,            "[trap] integer overflow")
d_m3ErrorConst(trapIntegerConversion,          "[trap] invalid conversion to integer")
d_m3ErrorConst(trapIndirectCallTypeMismatch,   "[trap] indirect call type mismatch")
d_m3ErrorConst(trapTableIndexOutOfRange,       "[trap] undefined element")
d_m3ErrorConst(trapTableElementIsNull,         "[trap] uninitialized element")
d_m3ErrorConst(trapNullReference,              "[trap] null reference")
d_m3ErrorConst(trapNullFunctionRef,            "[trap] null function reference")
// call_indirect past the end of the table is "undefined element"; the table
// access instructions report an out of bounds access instead
d_m3ErrorConst(trapTableOutOfBounds,           "[trap] out of bounds table access")
d_m3ErrorConst(trapWasiExit,                   "[trap] wasi program called exit")
d_m3ErrorConst(trapExit,                       "[trap] program called exit")
d_m3ErrorConst(trapAbort,                      "[trap] program called abort")
d_m3ErrorConst(trapUnreachable,                "[trap] unreachable")
d_m3ErrorConst(trapUnsupportedInstruction,     "[trap] unsupported instruction")
d_m3ErrorConst(trap_അട്ടി_Overflow,              "[trap] stack overflow")
d_m3ErrorConst(trapOutOfGas,                   "[trap] out of gas")
d_m3ErrorConst(trapUncaughtException,          "[trap] uncaught exception")
d_m3ErrorConst(trapContinuationConsumed,       "[trap] continuation already consumed")
d_m3ErrorConst(trapNullContinuationRef,        "[trap] null continuation reference")
d_m3ErrorConst(trapUnhandledControlTag,        "[trap] unhandled control tag")

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
IM3Environment m3_NewEnvironment (ശൂന്യം);

ശൂന്യം           m3_FreeEnvironment (IM3Environment i_environment);

ശൂന്യം           m3_SetCustomSectionHandler (IM3Environment i_environment, M3SectionHandler i_handler);


//-------------------------------------------------------------------------------------------------------------------------------
//  execution context
//-------------------------------------------------------------------------------------------------------------------------------

IM3Runtime     m3_NewRuntime (IM3Environment io_environment,
                              uint32_t       i_അട്ടി_Size_In_Bytes,
                              ശൂന്യം*          i_userdata);

ശൂന്യം           m3_FreeRuntime (IM3Runtime i_runtime);

// The validator runs as a pre-pass of compiling a function body, so it
// follows lazy compilation: a body is checked the first time it is compiled.
// Turning it off means trusting the module - nothing then checks a body's
// types before it runs. On by default (a no-op in a build without validation).
ശൂന്യം           m3_SetValidation (IM3Runtime i_runtime, bool i_enable);

// Zero means unlimited for allocation caps (memory, tables, continuations).
// Allocation limits below current usage are refused. Unsupported limits are
// refused even when the requested value is zero.
// Gas configures an execution budget (zero sets an empty/unmetered budget); arm
// a positive budget before compilation to instrument metered code. Gas saturates
// at INT64_MAX, continuations at UINT32_MAX. Setting an allocation limit leaves
// usage unchanged; setting gas starts a fresh execution budget.
M3Result       m3_SetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit, uint64_t i_മൂല്യം);
// Returns the effective (possibly saturated) limit; unsupported/unset limits are zero.
uint64_t       m3_GetResourceLimit (IM3Runtime i_runtime, M3ResourceLimit i_limit);
// Usage is tracked even without a limit (for gas, requires arming before compilation).
// Unsupported and unknown keys return zero.
uint64_t       m3_GetResourceUsage (IM3Runtime i_runtime, M3ResourceLimit i_limit);

// A memory belongs to the module that declares it, so these take the module
// rather than the runtime - a runtime can hold several modules, each with
// its own memories. Returns NULL when the module has no memory at that
// index; o_memorySizeInBytes is set either way.
//
// Sizes are size_t, not uint32_t: a linear memory may be a full 4 GiB, which
// is one byte too many to count in 32 bits.
uint8_t*       m3_GetMemory (IM3Module i_module,
                             size_t*   o_memorySizeInBytes,
                             uint32_t  i_memoryIndex);

size_t         m3_GetMemorySize (IM3Module i_module,
                                 uint32_t  i_memoryIndex);

// Size of the memory a pointer addresses into - specifically the _mem a raw
// function is handed, which is the memory of whichever module is calling,
// and need not be any particular module's. Used by m3ApiCheckMem.
size_t         m3_GetMemorySizeAt (സ്ഥിരം ശൂന്യം* i_memory);

ശൂന്യം*          m3_GetUserData (IM3Runtime i_runtime);

// The index of the memory the module exports under i_name. A host module
// whose ABI names the memory it addresses looks it up this way - WASI names
// it "memory". Returns m3Err_unknownMemory when no memory is exported
// under that name.
M3Result       m3_FindExportedMemory (IM3Module         i_module,
                                      സ്ഥിരം അക്ഷരം* സ്ഥിരം i_name,
                                      uint32_t*         o_memoryIndex);

// Pin which of the module's memories the host functions imported from
// i_importModule address ("*" for every namespace, as in
// m3_LinkRawFunction). Applies to imports already bound to a host function,
// so call it after linking them. Left alone, a host function addresses
// memory 0 - what a bare i32 guest pointer means when nothing says
// otherwise - so this is only needed by a host module that names a memory.
M3Result       m3_BindImportMemory (IM3Module         io_module,
                                    സ്ഥിരം അക്ഷരം* സ്ഥിരം i_importModule,
                                    uint32_t          i_memoryIndex);

//-------------------------------------------------------------------------------------------------------------------------------
//  modules
//-------------------------------------------------------------------------------------------------------------------------------

// i_wasmBytes data must be persistent during the lifetime of the module
M3Result       m3_ParseModule (IM3Environment       i_environment,
                               IM3Module*           o_module,
                               സ്ഥിരം uint8_t* സ്ഥിരം i_wasmBytes,
                               uint32_t             i_അക്കം_Wasm_Bytes);

// Only a module that was never handed to m3_LoadModule needs to be freed.
ശൂന്യം           m3_FreeModule (IM3Module i_module);

//  Transfers ownership of the module to the runtime - whether or not it
//  succeeds. A failed instantiation can already have written this module's
//  functions into a table another module owns, and the spec keeps whatever
//  it managed to do, so those entries stay callable; the module has to
//  outlive the failure for them not to dangle. m3_FreeRuntime releases it.
//  Do not call m3_FreeModule on a module after passing it here.
M3Result       m3_LoadModule (IM3Runtime io_runtime, IM3Module io_module);

// Optional, compiles all functions in the module
M3Result       m3_CompileModule (IM3Module io_module);

// Calling m3_RunStart is optional
M3Result       m3_RunStart (IM3Module i_module);

// Arguments and return values are passed in and out through the stack pointer _sp.
// Placeholder return value slots are first and arguments after. So, the first argument is at _sp [numReturns]
// Return values should be written into _sp [0] to _sp [num_returns - 1]
M3Result       m3_LinkRawFunction (IM3Module         io_module,
                                   സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName,
                                   സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName,
                                   സ്ഥിരം അക്ഷരം* സ്ഥിരം i_signature,
                                   M3RawCall         i_function);

M3Result       m3_LinkRawFunctionEx (IM3Module         io_module,
                                     സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName,
                                     സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName,
                                     സ്ഥിരം അക്ഷരം* സ്ഥിരം i_signature,
                                     M3RawCall         i_function,
                                     സ്ഥിരം ശൂന്യം*       i_userdata);

// supplies the value of an imported global, regardless of its mutability
M3Result       m3_LinkGlobal (IM3Module            io_module,
                              സ്ഥിരം അക്ഷരം* സ്ഥിരം    i_moduleName,
                              സ്ഥിരം അക്ഷരം* സ്ഥിരം    i_globalName,
                              സ്ഥിരം IM3_Tagged_മൂല്യം i_മൂല്യം);

സ്ഥിരം അക്ഷരം*    m3_GetModuleName (IM3Module i_module);
ശൂന്യം           m3_SetModuleName (IM3Module i_module, സ്ഥിരം അക്ഷരം* name);
IM3Runtime     m3_GetModuleRuntime (IM3Module i_module);

// The module registered under i_moduleName, or NULL. Most recently loaded
// first, so a name registered twice names the newer module.
IM3Module      m3_FindModule (IM3Runtime        i_runtime,
                              സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName);

//-------------------------------------------------------------------------------------------------------------------------------
//  globals
//-------------------------------------------------------------------------------------------------------------------------------
IM3Global      m3_FindGlobal (IM3Module         io_module,
                              സ്ഥിരം അക്ഷരം* സ്ഥിരം i_globalName);

M3Result       m3_GetGlobal (IM3Global      i_global,
                             IM3_Tagged_മൂല്യം o_മൂല്യം);

M3Result       m3_SetGlobal (IM3Global            i_global,
                             സ്ഥിരം IM3_Tagged_മൂല്യം i_മൂല്യം);

M3_മൂല്യം_Type    m3_GetGlobalType (IM3Global i_global);

//-------------------------------------------------------------------------------------------------------------------------------
//  functions
//-------------------------------------------------------------------------------------------------------------------------------
M3Result       m3_Yield (ശൂന്യം);

// o_function is valid during the lifetime of the originating runtime.
// m3_FindFunction searches every module loaded into the runtime, most
// recently loaded first; m3_FindFunctionIn searches just the one, which is
// what naming a module's export means.
M3Result       m3_FindFunction (IM3Function*      o_function,
                                IM3Runtime        i_runtime,
                                സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName);
M3Result       m3_FindFunctionIn (IM3Function*      o_function,
                                  IM3Module         i_module,
                                  സ്ഥിരം അക്ഷരം* സ്ഥിരം i_functionName);
M3Result       m3_GetTableFunction (IM3Function* o_function,
                                    IM3Module    i_module,
                                    uint32_t     i_index);

uint32_t       m3_Get_Arg_എണ്ണം (IM3Function i_function);
uint32_t       m3_Get_Ret_എണ്ണം (IM3Function i_function);
M3_മൂല്യം_Type    m3_GetArgType (IM3Function i_function, uint32_t i_index);
M3_മൂല്യം_Type    m3_GetRetType (IM3Function i_function, uint32_t i_index);

M3Result       m3_CallV (IM3Function i_function, ...);
M3Result       m3_CallVL (IM3Function i_function, va_list i_args);
M3Result       m3_Call (IM3Function i_function, uint32_t i_argc, സ്ഥിരം ശൂന്യം* i_argptrs[]);
M3Result       m3_CallArgv (IM3Function i_function, uint32_t i_argc, സ്ഥിരം അക്ഷരം* i_argv[]);

M3Result       m3_GetResultsV (IM3Function i_function, ...);
M3Result       m3_GetResultsVL (IM3Function i_function, va_list o_rets);
M3Result       m3_GetResults (IM3Function i_function, uint32_t i_retc, സ്ഥിരം ശൂന്യം* o_retptrs[]);


ശൂന്യം           m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info);
ശൂന്യം           m3_ResetErrorInfo (IM3Runtime i_runtime);

സ്ഥിരം അക്ഷരം*    m3_GetFunctionName (IM3Function i_function);
IM3Module      m3_GetFunctionModule (IM3Function i_function);

//-------------------------------------------------------------------------------------------------------------------------------
//  exceptions
//-------------------------------------------------------------------------------------------------------------------------------
// A tag is the identity of an exception: a catch clause names one, and catches only
// what was thrown with it. Its parameters are the types of the payload the exception
// carries. A tag lives as long as the runtime its module was loaded into, or the
// runtime m3_NewTag made it in.

// The tag i_module exports as i_tagName, or NULL.
IM3Tag         m3_FindTag (IM3Module         i_module,
                           സ്ഥിരം അക്ഷരം* സ്ഥിരം i_tagName);

// A tag of the host's own, typed by a signature in m3_LinkRawFunction's notation:
// "v(iI)" carries an i32 and an i64. No module can name it, so only catch_all and
// catch_all_ref catch what is thrown with it - until m3_LinkTag hands it to one.
M3Result       m3_NewTag (IM3Runtime        io_runtime,
                          IM3Tag*           o_tag,
                          സ്ഥിരം അക്ഷരം* സ്ഥിരം i_signature);

// Satisfies a tag import with i_tag, which must be of the same type. Code refers
// to a tag as whatever it resolved to when it was compiled, so link before
// calling anything that throws or catches it. "*" matches any module name.
M3Result       m3_LinkTag (IM3Module         io_module,
                           സ്ഥിരം അക്ഷരം* സ്ഥിരം i_moduleName,
                           സ്ഥിരം അക്ഷരം* സ്ഥിരം i_tagName,
                           IM3Tag            i_tag);

uint32_t       m3_Get_Tag_Arg_എണ്ണം (IM3Tag i_tag);
M3_മൂല്യം_Type    m3_GetTagArgType (IM3Tag i_tag, uint32_t i_index);

// Throws an exception from inside a raw host function, which returns what this
// returns: the exception, on its way to the nearest try_table that catches it, or
// a trap if it could not be thrown. i_argptrs point at the payload values, as
// m3_Call's arguments do. Only a tag without results can be thrown.
M3Result       m3_ThrowException (IM3Runtime  io_runtime,
                                  IM3Tag      i_tag,
                                  uint32_t    i_argc,
                                  സ്ഥിരം ശൂന്യം* i_argptrs[]);

// The exception that ended the most recent call or resume on this runtime, when
// it ended with m3Err_trapUncaughtException; NULL after any other outcome. The
// payload is a copy, so it outlives the call, but an exnref within it does not.
IM3Tag         m3_GetExceptionTag (IM3Runtime i_runtime);
M3Result       m3_GetExceptionArgs (IM3Runtime  i_runtime,
                                    uint32_t    i_argc,
                                    സ്ഥിരം ശൂന്യം* o_argptrs[]);

//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

തരനിർവചനം M3Result (*M3SnapshotWriter)(സ്ഥിരം ശൂന്യം* i_data, size_t i_size, ശൂന്യം* i_userdata);
തരനിർവചനം M3Result (*M3SnapshotReader)(ശൂന്യം* o_buffer, size_t i_size, ശൂന്യം* i_userdata);

// A suspendable runtime can pause a call part way and pick it up again with
// m3_ResumeRuntime - in this process, or through a snapshot in another. Make it
// suspendable before anything compiles: only code compiled that way has the
// checks that pause.
//
// m3_RequestSuspend asks for a pause, which takes effect at the next pause
// point: a loop's back edge, or the start of a function's body. Every loop and
// every recursion passes one. A resume goes on past the point it paused at, so
// asking again before each m3_ResumeRuntime steps from one to the next.
ശൂന്യം     m3_SetSuspendable (IM3Runtime io_runtime, bool i_suspendable);
ശൂന്യം     m3_RequestSuspend (IM3Runtime io_runtime);
bool     m3_IsSuspended (IM3Runtime i_runtime);

// Saves the paused invocation, or a postmortem of the module the last call
// entered when nothing is paused; with neither, there is nothing to save.
M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ശൂന്യം* i_userdata);
// Restores into a freshly instantiated module: loaded and linked, with none of
// its code run yet - its start function included. A restore that fails part way
// leaves a module that refuses to run, resume or be saved again, and has to be
// thrown away with its runtime.
M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ശൂന്യം* i_userdata);

M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, ശൂന്യം** o_bytes, size_t* o_size);
// Accepts a standalone container, a Wasm module with a default snapshot, or
// the bare section stream returned by m3_GetEmbeddedSnapshot.
M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, സ്ഥിരം ശൂന്യം* i_bytes, size_t i_size);

// A snapshot can also ride inside the module it belongs to, as a custom
// section named "snapshot" or "snapshot.<name>". i_name selects one; NULL or
// "" means the unnamed default. Two sections of the same name are refused
// when that name is selected, not when the module is parsed.
bool     m3_HasSnapshot (IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name);

// Points o_bytes at the embedded snapshot, which belongs to the module and
// lives as long as the bytes it was parsed from. The payload has no standalone header.
M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name,
                                 സ്ഥിരം ശൂന്യം** o_bytes, size_t* o_size);

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name);

// Writes the module's bytes with the paused invocation embedded in them,
// replacing any snapshot of the same name already there. A postmortem is not
// embedded: with nothing paused, this fails. Release o_bytes with free().
M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name,
                                  ശൂന്യം** o_bytes, size_t* o_size);

// What a snapshot cannot name on its own belongs to the embedder: an externref,
// and whatever the program depends on outside its Wasm state - open files,
// clocks, anything its imports keep. Any of these may be left NULL.
തരനിർവചനം ഘടന M3SnapshotHooks {
    // An externref as a number the embedder can turn back into the same
    // reference in another process. Without nameExternRef a non-null externref
    // refuses the save; without bindExternRef a snapshot holding one is refused.
    // UINT64_MAX is not a name: it is what a null reference is written as.
    M3Result (*nameExternRef)(ശൂന്യം* i_userdata, ശൂന്യം* i_reference, uint64_t* o_name);
    M3Result (*bindExternRef)(ശൂന്യം* i_userdata, uint64_t i_name, ശൂന്യം** o_reference);

    // The embedder's own state, carried after the program's. saveHostState
    // writes it through i_writer; loadHostState reads exactly those i_size bytes
    // back through i_reader. A snapshot that carries some is refused by a
    // runtime with no loadHostState.
    M3Result (*saveHostState)(ശൂന്യം* i_userdata, M3SnapshotWriter i_writer, ശൂന്യം* i_writerData);
    M3Result (*loadHostState)(ശൂന്യം* i_userdata, M3SnapshotReader i_reader, ശൂന്യം* i_readerData, size_t i_size);

    ശൂന്യം* userdata;
} M3SnapshotHooks;

ശൂന്യം             m3_SetSnapshotHooks (IM3Runtime io_runtime, സ്ഥിരം M3SnapshotHooks* i_hooks);

M3Result         m3_ResumeRuntime (IM3Runtime io_runtime);


//-------------------------------------------------------------------------------------------------------------------------------
//  debug info
//-------------------------------------------------------------------------------------------------------------------------------

ശൂന്യം             m3_PrintRuntimeInfo (IM3Runtime i_runtime);
ശൂന്യം             m3_PrintM3Info (ശൂന്യം);
ശൂന്യം             m3_PrintProfilerInfo (ശൂന്യം);

// The runtime owns the backtrace, do not free the backtrace you obtain. Returns NULL if there's no backtrace.
IM3BacktraceInfo m3_GetBacktrace (IM3Runtime i_runtime);

//-------------------------------------------------------------------------------------------------------------------------------
//  raw function definition helpers
//-------------------------------------------------------------------------------------------------------------------------------

#define m3ApiOffsetToPtr(offset)   (ശൂന്യം*)((uint8_t*)_mem + (uint32_t)(offset))
#define m3ApiPtrToOffset(ptr)      (uint32_t)((uint8_t*)ptr - (uint8_t*)_mem)

// Each of these walks _sp as it goes, so an entry point has to name every
// argument it is handed whether or not its implementation has a use for the
// value - which is why the declarations carry M3_UNUSED rather than the build
// turning the warning off for everything.
#define m3ApiReturnType(TYPE)                 TYPE* raw_return M3_UNUSED = ((TYPE*) (_sp++));
#define m3_Api_Multi_മൂല്യം_Return_Type(TYPE, NAME) TYPE* NAME M3_UNUSED = ((TYPE*) (_sp++));
#define m3ApiGetArg(TYPE, NAME)               TYPE NAME M3_UNUSED = \
   (വലുപ്പം(TYPE) >= വലുപ്പം(uint32_t)) ? \
   (*((TYPE *)(_sp++))) : \
   ((TYPE)(uintptr_t)(*((uint32_t *)(_sp++))));
#define m3ApiGetArgMem(TYPE, NAME)            TYPE NAME M3_UNUSED = (TYPE)m3ApiOffsetToPtr(* ((uint32_t *) (_sp++)));

#define m3ApiIsNullPtr(addr)       ((ശൂന്യം*)(addr) <= _mem)

// Whether [addr, addr+len) lies inside the memory _mem points at. Written as
// two subtractions rather than base + size and addr + len, so that neither a
// memory occupying the top of the address space nor a wild length can carry the
// comparison past where it wraps. Its own block, so the locals go out of scope
// with it.
#define m3ApiCheckMem(addr, len)                                                   \
   {   uintptr_t _m3_base = (uintptr_t)(_mem);                                     \
       uintptr_t _m3_addr = (uintptr_t)(ശൂന്യം*)(addr);                              \
       size_t    _m3_size = m3_GetMemorySizeAt(_mem);                              \
       എങ്കിൽ (M3_UNLIKELY(_m3_addr < _m3_base ||                                      \
                       (size_t)(_m3_addr - _m3_base) > _m3_size ||                 \
                       (uint64_t)(len) > (uint64_t)(_m3_size - (size_t)(_m3_addr - _m3_base)))) \
           m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);                           \
   }

#define m3ApiRawFunction(NAME)               സ്ഥിരം ശൂന്യം * NAME (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, ശൂന്യം * _mem)
#define m3ApiReturn(മൂല്യം_2)                   { *raw_return = (മൂല്യം_2); തിരികെ m3Err_none;}
#define m3_Api_Multi_മൂല്യം_Return(NAME, മൂല്യം_2)   { *NAME = (മൂല്യം_2); }
#define m3ApiTrap(മൂല്യം_2)                     { തിരികെ മൂല്യം_2; }
#define m3ApiThrow(TAG, ARGC, ARGPTRS)       { തിരികെ m3_ThrowException(runtime, TAG, ARGC, ARGPTRS); }
#define m3ApiSuccess()                       { തിരികെ m3Err_none; }

#എങ്കിൽ defined(M3_BIG_ENDIAN)
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        m3_bswap16((* (uint16_t *)(ptr)))
#  define m3ApiReadMem32(ptr)        m3_bswap32((* (uint32_t *)(ptr)))
#  define m3ApiReadMem64(ptr)        m3_bswap64((* (uint64_t *)(ptr)))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr)  = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = m3_bswap16((val)); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = m3_bswap32((val)); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = m3_bswap64((val)); }
#അല്ലെങ്കിൽ
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        (* (uint16_t *)(ptr))
#  define m3ApiReadMem32(ptr)        (* (uint32_t *)(ptr))
#  define m3ApiReadMem64(ptr)        (* (uint64_t *)(ptr))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr) = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = (val); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = (val); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = (val); }
#endif

#എങ്കിൽ defined(__cplusplus)
}
#endif

#endif // wasm3_h
