//
//  Wasm3, high performance WebAssembly interpreter
//
//  Copyright © 2019 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#నిర్వచితం_కాకపోతే wasm3_h
#నిర్వచించు wasm3_h

#నిర్వచించు M3_సంస్కరణ_MAJOR 0
#నిర్వచించు M3_సంస్కరణ_MINOR 9
#నిర్వచించు M3_సంస్కరణ_REV   2
#నిర్వచించు M3_సంస్కరణ       "0.9.2"

#అన్తర్భావ <stddef.h>
#అన్తర్భావ <stdlib.h>
#అన్తర్భావ <stdint.h>
#అన్తర్భావ <inttypes.h>
#అన్తర్భావ <stdarg.h>
#అన్తర్భావ <stdbool.h>
#అన్తర్భావ <stddef.h>

#అన్తర్భావ "wasm3_defs.h"

// Constants
#నిర్వచించు M3_BACKTRACE_TRUNCATED      (IM3_Backtrace_చట్రం)(SIZE_MAX)
// Gas units are ten-thousandths of a gas in ewasm's cost table.
#నిర్వచించు M3_GAS_UNITS_చొప్పున_GAS        10000

#ఒకవేళ defined(__cplusplus)
బాహ్య "C" {
#అయితే_ముగింపు

రకనిర్వచనం స్థిరరాశి అక్షరం* M3_ఫలితం;

నిర్మాణం M3Environment;
రకనిర్వచనం నిర్మాణం M3Environment* IM3Environment;
నిర్మాణం M3Runtime;
రకనిర్వచనం నిర్మాణం M3Runtime* IM3Runtime;
నిర్మాణం M3Module;
రకనిర్వచనం నిర్మాణం M3Module* IM3Module;
నిర్మాణం M3Function;
రకనిర్వచనం నిర్మాణం M3Function* IM3Function;
నిర్మాణం M3Global;
రకనిర్వచనం నిర్మాణం M3Global* IM3Global;
నిర్మాణం M3_ట్యాగ్;
రకనిర్వచనం నిర్మాణం M3_ట్యాగ్* IM3_ట్యాగ్;

రకనిర్వచనం నిర్మాణం M3ErrorInfo {
    M3_ఫలితం    ఫలితం;

    IM3Runtime  runtime;
    IM3Module   module;
    IM3Function function;

    స్థిరరాశి అక్షరం* file;
    uint32_t    పంక్తి_2;

    స్థిరరాశి అక్షరం* సందేశం;
} M3ErrorInfo;

రకనిర్వచనం నిర్మాణం M3_Backtrace_చట్రం {
    uint32_t                 module_స్థానభ్రంశం;
    IM3Function              function;

    నిర్మాణం M3_Backtrace_చట్రం* తదుపరి;
} M3_Backtrace_చట్రం, *IM3_Backtrace_చట్రం;

రకనిర్వచనం నిర్మాణం M3BacktraceInfo {
    IM3_Backtrace_చట్రం చట్రాలు;
    IM3_Backtrace_చట్రం చివరి_చట్రం;    // can be M3_BACKTRACE_TRUNCATED
} M3BacktraceInfo, *IM3BacktraceInfo;

రకనిర్వచనం గణన M3_Resource_హద్దు {
    c_m3_హద్దు_జ్ఞాపకం_బైట్లు,       // Actual backing bytes across the runtime's memories.
    c_m3_హద్దు_పట్టిక_Elements,     // Elements across the runtime's tables; imports count once.
    // Arm before compiling: only subsequently compiled bodies are instrumented.
    // Setting gas re-arms the full budget; usage may exceed it by one segment.
    // Exhaustion traps, or requests a pause in a suspendable runtime.
    c_m3_హద్దు_Gas_Units,
    c_m3_హద్దు_Continuations,     // Concurrent active cont.new stacks, recycled on completion.
} M3_Resource_హద్దు;

రకనిర్వచనం గణన M3_విలువ_Type {
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
    c_m3_Type_లెక్క,

    // "not a valid type" - returned where a lookup or a mapping failed.
    c_m3Type_unknown = c_m3_Type_లెక్క
} M3_విలువ_Type;

రకనిర్వచనం నిర్మాణం M3_Tagged_విలువ {
    M3_విలువ_Type type;
    సంఘం M3_విలువ_Union {
        uint32_t    i32;
        uint64_t    i64;
        దశాంశం       f32;
        ద్విగుణ      f64;
        స్థిరరాశి శూన్య* ref;        // funcref / externref; NULL is the null reference
    } విలువ;
} M3_Tagged_విలువ, *IM3_Tagged_విలువ;

రకనిర్వచనం నిర్మాణం M3ImportInfo {
    స్థిరరాశి అక్షరం* moduleUtf8;
    స్థిరరాశి అక్షరం* క్షేత్రం_Utf8;
} M3ImportInfo, *IM3ImportInfo;

రకనిర్వచనం నిర్మాణం M3ImportContext {
    శూన్య*       userdata;
    IM3Function function;
} M3ImportContext, *IM3ImportContext;

రకనిర్వచనం స్థిరరాశి శూన్య* (*M3RawCall)(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, శూన్య* _mem);

రకనిర్వచనం M3_ఫలితం (*M3SectionHandler)(IM3Module i_module, స్థిరరాశి అక్షరం* name, స్థిరరాశి uint8_t* ప్రారంభం, స్థిరరాశి uint8_t* చివర);


// -------------------------------------------------------------------------------------------------------------------------------
//  error codes
// -------------------------------------------------------------------------------------------------------------------------------

#ఒకవేళ defined(M3_IMPLEMENT_ERROR_అక్షరశ్రేణులు)
#  ఒకవేళ defined(__cplusplus)
#    define d_m3ErrorConst(LABEL, STRING)      బాహ్య స్థిరరాశి M3_ఫలితం m3Err_##LABEL = { STRING };
#  లేకపోతే
#    define d_m3ErrorConst(LABEL, STRING)      స్థిరరాశి M3_ఫలితం m3Err_##LABEL = { STRING };
#  endif
#లేకపోతే
#  define d_m3ErrorConst(LABEL, STRING)        బాహ్య స్థిరరాశి M3_ఫలితం m3Err_##LABEL;
#అయితే_ముగింపు

// -------------------------------------------------------------------------------------------------------------------------------
// clang-format off

d_m3ErrorConst(none,                           ఖాళీ)

// general errors
d_m3ErrorConst(mallocFailed,                   "memory allocation failed")

// parse errors
d_m3ErrorConst(incompatible_Wasm_సంస్కరణ,        "incompatible Wasm binary version")
d_m3ErrorConst(wasmMalformed,                  "malformed Wasm binary")
d_m3ErrorConst(misorderedWasmSection,          "out of order Wasm section")
d_m3ErrorConst(wasmUnderrun,                   "underrun while parsing Wasm binary")
d_m3ErrorConst(wasmOverrun,                    "overrun while parsing Wasm binary")
d_m3ErrorConst(wasm_Missing_Init_సమాసం,            "missing init_expr in Wasm binary")
d_m3ErrorConst(leb_అధికప్రవాహం,                    "LEB encoded value overflow")
d_m3ErrorConst(missingUTF8,                    "invalid length UTF-8 string")
d_m3ErrorConst(wasmSectionUnderrun,            "section underrun while parsing Wasm binary")
d_m3ErrorConst(wasmSectionOverrun,             "section overrun while parsing Wasm binary")
d_m3ErrorConst(invalidTypeId,                  "unknown value_type")
d_m3ErrorConst(too_Many_జ్ఞాపకం_Sections,          "only one memory per module is supported")
d_m3ErrorConst(too_Many_వాదనలు_Rets,                "too many arguments or return values")

// link errors
d_m3ErrorConst(moduleNotLinked,                "attempting to use module that is not loaded")
d_m3ErrorConst(moduleAlreadyLinked,            "attempting to bind module to multiple runtimes")
d_m3ErrorConst(functionLookupFailed,           "function lookup failed")
d_m3ErrorConst(ట్యాగ్_Lookup_Failed,                "tag lookup failed")
d_m3ErrorConst(functionImportMissing,          "missing imported function")
d_m3ErrorConst(unknownImport,                  "unknown import")
d_m3ErrorConst(incompatibleImportType,         "incompatible import type")

d_m3ErrorConst(malformedFunctionSignature,     "malformed function signature")
d_m3ErrorConst(functionSignatureMismatch,      "function signature mismatch")

// compilation errors
d_m3ErrorConst(noCompiler,                     "no compiler found for opcode")
d_m3ErrorConst(unknownOpcode,                  "unknown opcode")
d_m3ErrorConst(restrictedOpcode,               "restricted opcode")
d_m3ErrorConst(function_దొంతర_అధికప్రవాహం,          "compiling function overran its stack height limit")
d_m3ErrorConst(function_దొంతర_Underrun,          "compiling function underran the stack")
d_m3ErrorConst(malloc_Failed_సంకేతం_పుట,           "memory allocation failed when acquiring a new M3 code page")
d_m3ErrorConst(settingImmutableGlobal,         "attempting to set an immutable global")
d_m3ErrorConst(typeMismatch,                   "incorrect type on stack")
d_m3ErrorConst(type_లెక్క_Mismatch,              "incorrect value count on stack")

// validation errors. The wording follows the spec's own assert_invalid failure
d_m3ErrorConst(unknownType,                    "unknown type")
d_m3ErrorConst(unknownLabel,                   "unknown label")
d_m3ErrorConst(unknownLocal,                   "unknown local")
d_m3ErrorConst(unknownGlobal,                  "unknown global")
d_m3ErrorConst(unknownFunction,                "unknown function")
d_m3ErrorConst(unknown_పట్టిక,                   "unknown table")
d_m3ErrorConst(unknown_ట్యాగ్,                     "unknown tag")
d_m3ErrorConst(unknown_జ్ఞాపకం,                  "unknown memory")
d_m3ErrorConst(unknown_దత్తాంశం_Segment,             "unknown data segment")
d_m3ErrorConst(unknownElemSegment,             "unknown elem segment")
d_m3ErrorConst(దత్తాంశం_లెక్క_Required,              "data count section required")
d_m3ErrorConst(invalidAlignment,               "alignment must not be larger than natural")
d_m3ErrorConst(invalidAtomicAlignment,         "atomic alignment must be natural")
d_m3ErrorConst(undeclaredFuncRef,              "undeclared function reference")

// runtime errors
d_m3ErrorConst(missing_Compiled_సంకేతం,            "function is missing compiled m3 code")
d_m3ErrorConst(wasm_జ్ఞాపకం_అధికప్రవాహం,             "runtime ran out of memory")
d_m3ErrorConst(global_జ్ఞాపకం_Not_Allocated,       "global memory is missing from a module")
d_m3ErrorConst(globa_సూచీ_బహిర్గతం_యొక్క_Bounds,          "global index is too large")
d_m3ErrorConst(వాదం_లెక్క_Mismatch,          "argument count mismatch")
d_m3ErrorConst(వాదం_Type_Mismatch,           "argument type mismatch")
d_m3ErrorConst(globalLookupFailed,             "global lookup failed")
d_m3ErrorConst(globalTypeMismatch,             "global type mismatch")
d_m3ErrorConst(globalNotMutable,               "global is not mutable")
d_m3ErrorConst(జ్ఞాపకం_Guards_Unavailable,        "memory guards could not be installed")

// resource limit errors
d_m3ErrorConst(జ్ఞాపకం_హద్దు_Exceeded,            "runtime memory limit exceeded")
d_m3ErrorConst(పట్టిక_హద్దు_Exceeded,             "table elements limit exceeded")
d_m3ErrorConst(continuation_హద్దు_Exceeded,      "continuation limit exceeded")
d_m3ErrorConst(resource_హద్దు_క్రింద_Usage,        "resource limit below current usage")
d_m3ErrorConst(resource_హద్దు_Not_Supported,      "resource limit not supported in this build")
d_m3ErrorConst(unknown_Resource_హద్దు,           "unknown resource limit")

// traps
d_m3ErrorConst(trap_బహిర్గతం_యొక్క_Bounds_జ్ఞాపకం_Access,    "[trap] out of bounds memory access")
d_m3ErrorConst(trapUnalignedAtomic,            "[trap] unaligned atomic")
d_m3ErrorConst(trap_Expected_Shared_జ్ఞాపకం,       "[trap] expected shared memory")
d_m3ErrorConst(trapWaitForever,                "[trap] wait would block forever")
d_m3ErrorConst(trapDivisionByZero,             "[trap] integer divide by zero")
d_m3ErrorConst(trap_Integer_అధికప్రవాహం,            "[trap] integer overflow")
d_m3ErrorConst(trapIntegerConversion,          "[trap] invalid conversion to integer")
d_m3ErrorConst(trap_పరోక్ష_Call_Type_Mismatch,   "[trap] indirect call type mismatch")
d_m3ErrorConst(trap_పట్టిక_సూచీ_బహిర్గతం_యొక్క_Range,       "[trap] undefined element")
d_m3ErrorConst(trap_పట్టిక_Element_Is_Null,         "[trap] uninitialized element")
d_m3ErrorConst(trapNullReference,              "[trap] null reference")
d_m3ErrorConst(trapNullFunctionRef,            "[trap] null function reference")
// call_indirect past the end of the table is "undefined element"; the table
// access instructions report an out of bounds access instead
d_m3ErrorConst(trap_పట్టిక_బహిర్గతం_యొక్క_Bounds,           "[trap] out of bounds table access")
d_m3ErrorConst(trapWasiExit,                   "[trap] wasi program called exit")
d_m3ErrorConst(trapExit,                       "[trap] program called exit")
d_m3ErrorConst(trapAbort,                      "[trap] program called abort")
d_m3ErrorConst(trapUnreachable,                "[trap] unreachable")
d_m3ErrorConst(trapUnsupportedInstruction,     "[trap] unsupported instruction")
d_m3ErrorConst(trap_దొంతర_అధికప్రవాహం,              "[trap] stack overflow")
d_m3ErrorConst(trap_బహిర్గతం_యొక్క_Gas,                   "[trap] out of gas")
d_m3ErrorConst(trapUncaughtException,          "[trap] uncaught exception")
d_m3ErrorConst(trapContinuationConsumed,       "[trap] continuation already consumed")
d_m3ErrorConst(trapNullContinuationRef,        "[trap] null continuation reference")
d_m3ErrorConst(trap_Unhandled_Control_ట్యాగ్,        "[trap] unhandled control tag")

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
IM3Environment m3_NewEnvironment (శూన్య);

శూన్య           m3_FreeEnvironment (IM3Environment i_environment);

శూన్య           m3_సమితి_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler);


//-------------------------------------------------------------------------------------------------------------------------------
//  execution context
//-------------------------------------------------------------------------------------------------------------------------------

IM3Runtime     m3_NewRuntime (IM3Environment io_environment,
                              uint32_t       i_దొంతర_కొలత_లోపల_బైట్లు,
                              శూన్య*          i_userdata);

శూన్య           m3_FreeRuntime (IM3Runtime i_runtime);

// The validator runs as a pre-pass of compiling a function body, so it
// follows lazy compilation: a body is checked the first time it is compiled.
// Turning it off means trusting the module - nothing then checks a body's
// types before it runs. On by default (a no-op in a build without validation).
శూన్య           m3_సమితి_Validation (IM3Runtime i_runtime, bool i_enable);

// Zero means unlimited for allocation caps (memory, tables, continuations).
// Allocation limits below current usage are refused. Unsupported limits are
// refused even when the requested value is zero.
// Gas configures an execution budget (zero sets an empty/unmetered budget); arm
// a positive budget before compilation to instrument metered code. Gas saturates
// at INT64_MAX, continuations at UINT32_MAX. Setting an allocation limit leaves
// usage unchanged; setting gas starts a fresh execution budget.
M3_ఫలితం       m3_సమితి_Resource_హద్దు (IM3Runtime i_runtime, M3_Resource_హద్దు i_హద్దు, uint64_t i_విలువ);
// Returns the effective (possibly saturated) limit; unsupported/unset limits are zero.
uint64_t       m3_Get_Resource_హద్దు (IM3Runtime i_runtime, M3_Resource_హద్దు i_హద్దు);
// Usage is tracked even without a limit (for gas, requires arming before compilation).
// Unsupported and unknown keys return zero.
uint64_t       m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_హద్దు i_హద్దు);

// A memory belongs to the module that declares it, so these take the module
// rather than the runtime - a runtime can hold several modules, each with
// its own memories. Returns NULL when the module has no memory at that
// index; o_memorySizeInBytes is set either way.
//
// Sizes are size_t, not uint32_t: a linear memory may be a full 4 GiB, which
// is one byte too many to count in 32 bits.
uint8_t*       m3_Get_జ్ఞాపకం (IM3Module i_module,
                             size_t*   o_జ్ఞాపకం_కొలత_లోపల_బైట్లు,
                             uint32_t  i_జ్ఞాపకం_సూచీ);

size_t         m3_Get_జ్ఞాపకం_కొలత (IM3Module i_module,
                                 uint32_t  i_జ్ఞాపకం_సూచీ);

// Size of the memory a pointer addresses into - specifically the _mem a raw
// function is handed, which is the memory of whichever module is calling,
// and need not be any particular module's. Used by m3ApiCheckMem.
size_t         m3_Get_జ్ఞాపకం_కొలత_At (స్థిరరాశి శూన్య* i_జ్ఞాపకం);

శూన్య*          m3_Get_User_దత్తాంశం (IM3Runtime i_runtime);

// The index of the memory the module exports under i_name. A host module
// whose ABI names the memory it addresses looks it up this way - WASI names
// it "memory". Returns m3Err_unknownMemory when no memory is exported
// under that name.
M3_ఫలితం       m3_కనుగొను_Exported_జ్ఞాపకం (IM3Module         i_module,
                                      స్థిరరాశి అక్షరం* స్థిరరాశి i_name,
                                      uint32_t*         o_జ్ఞాపకం_సూచీ);

// Pin which of the module's memories the host functions imported from
// i_importModule address ("*" for every namespace, as in
// m3_LinkRawFunction). Applies to imports already bound to a host function,
// so call it after linking them. Left alone, a host function addresses
// memory 0 - what a bare i32 guest pointer means when nothing says
// otherwise - so this is only needed by a host module that names a memory.
M3_ఫలితం       m3_Bind_Import_జ్ఞాపకం (IM3Module         io_module,
                                    స్థిరరాశి అక్షరం* స్థిరరాశి i_importModule,
                                    uint32_t          i_జ్ఞాపకం_సూచీ);

//-------------------------------------------------------------------------------------------------------------------------------
//  modules
//-------------------------------------------------------------------------------------------------------------------------------

// i_wasmBytes data must be persistent during the lifetime of the module
M3_ఫలితం       m3_ParseModule (IM3Environment       i_environment,
                               IM3Module*           o_module,
                               స్థిరరాశి uint8_t* స్థిరరాశి i_wasm_బైట్లు,
                               uint32_t             i_అంకె_Wasm_బైట్లు);

// Only a module that was never handed to m3_LoadModule needs to be freed.
శూన్య           m3_FreeModule (IM3Module i_module);

//  Transfers ownership of the module to the runtime - whether or not it
//  succeeds. A failed instantiation can already have written this module's
//  functions into a table another module owns, and the spec keeps whatever
//  it managed to do, so those entries stay callable; the module has to
//  outlive the failure for them not to dangle. m3_FreeRuntime releases it.
//  Do not call m3_FreeModule on a module after passing it here.
M3_ఫలితం       m3_ఎక్కించు_Module (IM3Runtime io_runtime, IM3Module io_module);

// Optional, compiles all functions in the module
M3_ఫలితం       m3_CompileModule (IM3Module io_module);

// Calling m3_RunStart is optional
M3_ఫలితం       m3_నిరంతరం_ప్రారంభం (IM3Module i_module);

// Arguments and return values are passed in and out through the stack pointer _sp.
// Placeholder return value slots are first and arguments after. So, the first argument is at _sp [numReturns]
// Return values should be written into _sp [0] to _sp [num_returns - 1]
M3_ఫలితం       m3_LinkRawFunction (IM3Module         io_module,
                                   స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName,
                                   స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName,
                                   స్థిరరాశి అక్షరం* స్థిరరాశి i_signature,
                                   M3RawCall         i_function);

M3_ఫలితం       m3_LinkRawFunctionEx (IM3Module         io_module,
                                     స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName,
                                     స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName,
                                     స్థిరరాశి అక్షరం* స్థిరరాశి i_signature,
                                     M3RawCall         i_function,
                                     స్థిరరాశి శూన్య*       i_userdata);

// supplies the value of an imported global, regardless of its mutability
M3_ఫలితం       m3_LinkGlobal (IM3Module            io_module,
                              స్థిరరాశి అక్షరం* స్థిరరాశి    i_moduleName,
                              స్థిరరాశి అక్షరం* స్థిరరాశి    i_globalName,
                              స్థిరరాశి IM3_Tagged_విలువ i_విలువ);

స్థిరరాశి అక్షరం*    m3_GetModuleName (IM3Module i_module);
శూన్య           m3_సమితి_Module_Name (IM3Module i_module, స్థిరరాశి అక్షరం* name);
IM3Runtime     m3_GetModuleRuntime (IM3Module i_module);

// The module registered under i_moduleName, or NULL. Most recently loaded
// first, so a name registered twice names the newer module.
IM3Module      m3_కనుగొను_Module (IM3Runtime        i_runtime,
                              స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName);

//-------------------------------------------------------------------------------------------------------------------------------
//  globals
//-------------------------------------------------------------------------------------------------------------------------------
IM3Global      m3_కనుగొను_Global (IM3Module         io_module,
                              స్థిరరాశి అక్షరం* స్థిరరాశి i_globalName);

M3_ఫలితం       m3_GetGlobal (IM3Global      i_global,
                             IM3_Tagged_విలువ o_విలువ);

M3_ఫలితం       m3_సమితి_Global (IM3Global            i_global,
                             స్థిరరాశి IM3_Tagged_విలువ i_విలువ);

M3_విలువ_Type    m3_GetGlobalType (IM3Global i_global);

//-------------------------------------------------------------------------------------------------------------------------------
//  functions
//-------------------------------------------------------------------------------------------------------------------------------
M3_ఫలితం       m3_Yield (శూన్య);

// o_function is valid during the lifetime of the originating runtime.
// m3_FindFunction searches every module loaded into the runtime, most
// recently loaded first; m3_FindFunctionIn searches just the one, which is
// what naming a module's export means.
M3_ఫలితం       m3_కనుగొను_Function (IM3Function*      o_function,
                                IM3Runtime        i_runtime,
                                స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName);
M3_ఫలితం       m3_కనుగొను_Function_లోపల (IM3Function*      o_function,
                                  IM3Module         i_module,
                                  స్థిరరాశి అక్షరం* స్థిరరాశి i_functionName);
M3_ఫలితం       m3_Get_పట్టిక_Function (IM3Function* o_function,
                                    IM3Module    i_module,
                                    uint32_t     i_సూచీ);

uint32_t       m3_Get_Arg_లెక్క (IM3Function i_function);
uint32_t       m3_Get_Ret_లెక్క (IM3Function i_function);
M3_విలువ_Type    m3_GetArgType (IM3Function i_function, uint32_t i_సూచీ);
M3_విలువ_Type    m3_GetRetType (IM3Function i_function, uint32_t i_సూచీ);

M3_ఫలితం       m3_CallV (IM3Function i_function, ...);
M3_ఫలితం       m3_CallVL (IM3Function i_function, va_list i_వాదనలు);
M3_ఫలితం       m3_Call (IM3Function i_function, uint32_t i_argc, స్థిరరాశి శూన్య* i_argptrs[]);
M3_ఫలితం       m3_CallArgv (IM3Function i_function, uint32_t i_argc, స్థిరరాశి అక్షరం* i_argv[]);

M3_ఫలితం       m3_GetResultsV (IM3Function i_function, ...);
M3_ఫలితం       m3_GetResultsVL (IM3Function i_function, va_list o_rets);
M3_ఫలితం       m3_GetResults (IM3Function i_function, uint32_t i_retc, స్థిరరాశి శూన్య* o_retptrs[]);


శూన్య           m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info);
శూన్య           m3_ResetErrorInfo (IM3Runtime i_runtime);

స్థిరరాశి అక్షరం*    m3_GetFunctionName (IM3Function i_function);
IM3Module      m3_GetFunctionModule (IM3Function i_function);

//-------------------------------------------------------------------------------------------------------------------------------
//  exceptions
//-------------------------------------------------------------------------------------------------------------------------------
// A tag is the identity of an exception: a catch clause names one, and catches only
// what was thrown with it. Its parameters are the types of the payload the exception
// carries. A tag lives as long as the runtime its module was loaded into, or the
// runtime m3_NewTag made it in.

// The tag i_module exports as i_tagName, or NULL.
IM3_ట్యాగ్         m3_కనుగొను_ట్యాగ్ (IM3Module         i_module,
                           స్థిరరాశి అక్షరం* స్థిరరాశి i_ట్యాగ్_Name);

// A tag of the host's own, typed by a signature in m3_LinkRawFunction's notation:
// "v(iI)" carries an i32 and an i64. No module can name it, so only catch_all and
// catch_all_ref catch what is thrown with it - until m3_LinkTag hands it to one.
M3_ఫలితం       m3_New_ట్యాగ్ (IM3Runtime        io_runtime,
                          IM3_ట్యాగ్*           o_ట్యాగ్,
                          స్థిరరాశి అక్షరం* స్థిరరాశి i_signature);

// Satisfies a tag import with i_tag, which must be of the same type. Code refers
// to a tag as whatever it resolved to when it was compiled, so link before
// calling anything that throws or catches it. "*" matches any module name.
M3_ఫలితం       m3_Link_ట్యాగ్ (IM3Module         io_module,
                           స్థిరరాశి అక్షరం* స్థిరరాశి i_moduleName,
                           స్థిరరాశి అక్షరం* స్థిరరాశి i_ట్యాగ్_Name,
                           IM3_ట్యాగ్            i_ట్యాగ్);

uint32_t       m3_Get_ట్యాగ్_Arg_లెక్క (IM3_ట్యాగ్ i_ట్యాగ్);
M3_విలువ_Type    m3_Get_ట్యాగ్_Arg_Type (IM3_ట్యాగ్ i_ట్యాగ్, uint32_t i_సూచీ);

// Throws an exception from inside a raw host function, which returns what this
// returns: the exception, on its way to the nearest try_table that catches it, or
// a trap if it could not be thrown. i_argptrs point at the payload values, as
// m3_Call's arguments do. Only a tag without results can be thrown.
M3_ఫలితం       m3_ThrowException (IM3Runtime  io_runtime,
                                  IM3_ట్యాగ్      i_ట్యాగ్,
                                  uint32_t    i_argc,
                                  స్థిరరాశి శూన్య* i_argptrs[]);

// The exception that ended the most recent call or resume on this runtime, when
// it ended with m3Err_trapUncaughtException; NULL after any other outcome. The
// payload is a copy, so it outlives the call, but an exnref within it does not.
IM3_ట్యాగ్         m3_Get_Exception_ట్యాగ్ (IM3Runtime i_runtime);
M3_ఫలితం       m3_Get_Exception_వాదనలు (IM3Runtime  i_runtime,
                                    uint32_t    i_argc,
                                    స్థిరరాశి శూన్య* o_argptrs[]);

//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

రకనిర్వచనం M3_ఫలితం (*M3SnapshotWriter)(స్థిరరాశి శూన్య* i_దత్తాంశం, size_t i_కొలత, శూన్య* i_userdata);
రకనిర్వచనం M3_ఫలితం (*M3SnapshotReader)(శూన్య* o_బఫరు, size_t i_కొలత, శూన్య* i_userdata);

// A suspendable runtime can pause a call part way and pick it up again with
// m3_ResumeRuntime - in this process, or through a snapshot in another. Make it
// suspendable before anything compiles: only code compiled that way has the
// checks that pause.
//
// m3_RequestSuspend asks for a pause, which takes effect at the next pause
// point: a loop's back edge, or the start of a function's body. Every loop and
// every recursion passes one. A resume goes on past the point it paused at, so
// asking again before each m3_ResumeRuntime steps from one to the next.
శూన్య     m3_సమితి_Suspendable (IM3Runtime io_runtime, bool i_suspendable);
శూన్య     m3_RequestSuspend (IM3Runtime io_runtime);
bool     m3_IsSuspended (IM3Runtime i_runtime);

// Saves the paused invocation, or a postmortem of the module the last call
// entered when nothing is paused; with neither, there is nothing to save.
M3_ఫలితం m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, శూన్య* i_userdata);
// Restores into a freshly instantiated module: loaded and linked, with none of
// its code run yet - its start function included. A restore that fails part way
// leaves a module that refuses to run, resume or be saved again, and has to be
// thrown away with its runtime.
M3_ఫలితం m3_ఎక్కించు_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, శూన్య* i_userdata);

M3_ఫలితం m3_Save_Snapshot_గమ్యం_బఫరు (IM3Runtime io_runtime, శూన్య** o_బైట్లు, size_t* o_కొలత);
// Accepts a standalone container, a Wasm module with a default snapshot, or
// the bare section stream returned by m3_GetEmbeddedSnapshot.
M3_ఫలితం m3_ఎక్కించు_Snapshot_From_బఫరు (IM3Runtime io_runtime, IM3Module i_module, స్థిరరాశి శూన్య* i_బైట్లు, size_t i_కొలత);

// A snapshot can also ride inside the module it belongs to, as a custom
// section named "snapshot" or "snapshot.<name>". i_name selects one; NULL or
// "" means the unnamed default. Two sections of the same name are refused
// when that name is selected, not when the module is parsed.
bool     m3_HasSnapshot (IM3Module i_module, స్థిరరాశి అక్షరం* i_name);

// Points o_bytes at the embedded snapshot, which belongs to the module and
// lives as long as the bytes it was parsed from. The payload has no standalone header.
M3_ఫలితం m3_GetEmbeddedSnapshot (IM3Module i_module, స్థిరరాశి అక్షరం* i_name,
                                 స్థిరరాశి శూన్య** o_బైట్లు, size_t* o_కొలత);

M3_ఫలితం m3_ఎక్కించు_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, స్థిరరాశి అక్షరం* i_name);

// Writes the module's bytes with the paused invocation embedded in them,
// replacing any snapshot of the same name already there. A postmortem is not
// embedded: with nothing paused, this fails. Release o_bytes with free().
M3_ఫలితం m3_Save_Snapshot_గమ్యం_Module (IM3Runtime io_runtime, IM3Module i_module, స్థిరరాశి అక్షరం* i_name,
                                  శూన్య** o_బైట్లు, size_t* o_కొలత);

// What a snapshot cannot name on its own belongs to the embedder: an externref,
// and whatever the program depends on outside its Wasm state - open files,
// clocks, anything its imports keep. Any of these may be left NULL.
రకనిర్వచనం నిర్మాణం M3SnapshotHooks {
    // An externref as a number the embedder can turn back into the same
    // reference in another process. Without nameExternRef a non-null externref
    // refuses the save; without bindExternRef a snapshot holding one is refused.
    // UINT64_MAX is not a name: it is what a null reference is written as.
    M3_ఫలితం (*nameExternRef)(శూన్య* i_userdata, శూన్య* i_reference, uint64_t* o_name);
    M3_ఫలితం (*bindExternRef)(శూన్య* i_userdata, uint64_t i_name, శూన్య** o_reference);

    // The embedder's own state, carried after the program's. saveHostState
    // writes it through i_writer; loadHostState reads exactly those i_size bytes
    // back through i_reader. A snapshot that carries some is refused by a
    // runtime with no loadHostState.
    M3_ఫలితం (*saveHostState)(శూన్య* i_userdata, M3SnapshotWriter i_writer, శూన్య* i_writer_దత్తాంశం);
    M3_ఫలితం (*ఎక్కించు_Host_State_2)(శూన్య* i_userdata, M3SnapshotReader i_reader, శూన్య* i_reader_దత్తాంశం, size_t i_కొలత);

    శూన్య* userdata;
} M3SnapshotHooks;

శూన్య             m3_సమితి_Snapshot_Hooks (IM3Runtime io_runtime, స్థిరరాశి M3SnapshotHooks* i_hooks);

M3_ఫలితం         m3_ResumeRuntime (IM3Runtime io_runtime);


//-------------------------------------------------------------------------------------------------------------------------------
//  debug info
//-------------------------------------------------------------------------------------------------------------------------------

శూన్య             m3_PrintRuntimeInfo (IM3Runtime i_runtime);
శూన్య             m3_PrintM3Info (శూన్య);
శూన్య             m3_PrintProfilerInfo (శూన్య);

// The runtime owns the backtrace, do not free the backtrace you obtain. Returns NULL if there's no backtrace.
IM3BacktraceInfo m3_GetBacktrace (IM3Runtime i_runtime);

//-------------------------------------------------------------------------------------------------------------------------------
//  raw function definition helpers
//-------------------------------------------------------------------------------------------------------------------------------

#నిర్వచించు m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(offset)   (శూన్య*)((uint8_t*)_mem + (uint32_t)(offset))
#నిర్వచించు m3_Api_Ptr_గమ్యం_స్థానభ్రంశం(ptr)      (uint32_t)((uint8_t*)ptr - (uint8_t*)_mem)

// Each of these walks _sp as it goes, so an entry point has to name every
// argument it is handed whether or not its implementation has a use for the
// value - which is why the declarations carry M3_UNUSED rather than the build
// turning the warning off for everything.
#నిర్వచించు m3ApiReturnType(TYPE)                 TYPE* raw_return M3_వాడని = ((TYPE*) (_sp++));
#నిర్వచించు m3_Api_Multi_విలువ_Return_Type(TYPE, NAME) TYPE* NAME M3_వాడని = ((TYPE*) (_sp++));
#నిర్వచించు m3ApiGetArg(TYPE, NAME)               TYPE NAME M3_వాడని = \
   (పరిమాణం(TYPE) >= పరిమాణం(uint32_t)) ? \
   (*((TYPE *)(_sp++))) : \
   ((TYPE)(uintptr_t)(*((uint32_t *)(_sp++))));
#నిర్వచించు m3ApiGetArgMem(TYPE, NAME)            TYPE NAME M3_వాడని = (TYPE)m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(* ((uint32_t *) (_sp++)));

#నిర్వచించు m3ApiIsNullPtr(addr)       ((శూన్య*)(addr) <= _mem)

// Whether [addr, addr+len) lies inside the memory _mem points at. Written as
// two subtractions rather than base + size and addr + len, so that neither a
// memory occupying the top of the address space nor a wild length can carry the
// comparison past where it wraps. Its own block, so the locals go out of scope
// with it.
#నిర్వచించు m3ApiCheckMem(addr, len)                                                   \
   {   uintptr_t m3_పునాది = (uintptr_t)(_mem);                                     \
       uintptr_t _m3_addr = (uintptr_t)(శూన్య*)(addr);                              \
       size_t    m3_కొలత = m3_Get_జ్ఞాపకం_కొలత_At(_mem);                              \
       ఒకవేళ (M3_UNLIKELY(_m3_addr < m3_పునాది ||                                      \
                       (size_t)(_m3_addr - m3_పునాది) > m3_కొలత ||                 \
                       (uint64_t)(len) > (uint64_t)(m3_కొలత - (size_t)(_m3_addr - m3_పునాది)))) \
           m3ApiTrap(m3_Err_trap_బహిర్గతం_యొక్క_Bounds_జ్ఞాపకం_Access);                           \
   }

#నిర్వచించు m3ApiRawFunction(NAME)               స్థిరరాశి శూన్య * NAME (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, శూన్య * _mem)
#నిర్వచించు m3ApiReturn(విలువ_2)                   { *raw_return = (విలువ_2); తిరిగి m3Err_none;}
#నిర్వచించు m3_Api_Multi_విలువ_Return(NAME, విలువ_2)   { *NAME = (విలువ_2); }
#నిర్వచించు m3ApiTrap(విలువ_2)                     { తిరిగి విలువ_2; }
#నిర్వచించు m3ApiThrow(ట్యాగ్_2, ARGC, ARGPTRS)       { తిరిగి m3_ThrowException(runtime, ట్యాగ్_2, ARGC, ARGPTRS); }
#నిర్వచించు m3ApiSuccess()                       { తిరిగి m3Err_none; }

#ఒకవేళ defined(M3_BIG_ENDIAN)
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        m3_bswap16((* (uint16_t *)(ptr)))
#  define m3ApiReadMem32(ptr)        m3_bswap32((* (uint32_t *)(ptr)))
#  define m3ApiReadMem64(ptr)        m3_bswap64((* (uint64_t *)(ptr)))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr)  = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = m3_bswap16((val)); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = m3_bswap32((val)); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = m3_bswap64((val)); }
#లేకపోతే
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        (* (uint16_t *)(ptr))
#  define m3ApiReadMem32(ptr)        (* (uint32_t *)(ptr))
#  define m3ApiReadMem64(ptr)        (* (uint64_t *)(ptr))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr) = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = (val); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = (val); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = (val); }
#అయితే_ముగింపు

#ఒకవేళ defined(__cplusplus)
}
#అయితే_ముగింపు

#అయితే_ముగింపు // wasm3_h
