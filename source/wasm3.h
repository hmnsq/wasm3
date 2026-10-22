//
//  Wasm3, high performance WebAssembly interpreter
//
//  Copyright © 2019 Steven Massey, Volodymyr Shymanskyy.
//  All rights reserved.
//

#اگر_غیر_تعریف_شدہ wasm3_h
#تعریف_کرو wasm3_h

#تعریف_کرو M3_نسخہ_MAJOR 0
#تعریف_کرو M3_نسخہ_MINOR 9
#تعریف_کرو M3_نسخہ_REV   2
#تعریف_کرو M3_نسخہ       "0.9.2"

#شامل <stddef.h>
#شامل <stdlib.h>
#شامل <stdint.h>
#شامل <inttypes.h>
#شامل <stdarg.h>
#شامل <stdbool.h>
#شامل <stddef.h>

#شامل "wasm3_defs.h"

// Constants
#تعریف_کرو M3_BACKTRACE_TRUNCATED      (IM3_Backtrace_چوکھٹا)(SIZE_MAX)
// Gas units are ten-thousandths of a gas in ewasm's cost table.
#تعریف_کرو M3_GAS_UNITS_فی_GAS        10000

#اگر defined(__cplusplus)
خارجی "C" {
#اگر_ختم

قسمتعریف ثابت حرف* M3_نتیجہ;

ساخت M3Environment;
قسمتعریف ساخت M3Environment* IM3Environment;
ساخت M3Runtime;
قسمتعریف ساخت M3Runtime* IM3Runtime;
ساخت M3Module;
قسمتعریف ساخت M3Module* IM3Module;
ساخت M3Function;
قسمتعریف ساخت M3Function* IM3Function;
ساخت M3Global;
قسمتعریف ساخت M3Global* IM3Global;
ساخت M3_لیبل;
قسمتعریف ساخت M3_لیبل* IM3_لیبل;

قسمتعریف ساخت M3ErrorInfo {
    M3_نتیجہ    نتیجہ;

    IM3Runtime  runtime;
    IM3Module   module;
    IM3Function function;

    ثابت حرف* file;
    uint32_t    سطر_2;

    ثابت حرف* پیغام;
} M3ErrorInfo;

قسمتعریف ساخت M3_Backtrace_چوکھٹا {
    uint32_t                 module_انحراف;
    IM3Function              function;

    ساخت M3_Backtrace_چوکھٹا* اگلا;
} M3_Backtrace_چوکھٹا, *IM3_Backtrace_چوکھٹا;

قسمتعریف ساخت M3BacktraceInfo {
    IM3_Backtrace_چوکھٹا چوکھٹے;
    IM3_Backtrace_چوکھٹا آخری_چوکھٹا;    // can be M3_BACKTRACE_TRUNCATED
} M3BacktraceInfo, *IM3BacktraceInfo;

قسمتعریف شمار M3_Resource_حد_بندی {
    c_m3_حد_بندی_حافظہ_بائٹیں,       // Actual backing bytes across the runtime's memories.
    c_m3_حد_بندی_جدول_Elements,     // Elements across the runtime's tables; imports count once.
    // Arm before compiling: only subsequently compiled bodies are instrumented.
    // Setting gas re-arms the full budget; usage may exceed it by one segment.
    // Exhaustion traps, or requests a pause in a suspendable runtime.
    c_m3_حد_بندی_Gas_Units,
    c_m3_حد_بندی_Continuations,     // Concurrent active cont.new stacks, recycled on completion.
} M3_Resource_حد_بندی;

قسمتعریف شمار M3_قدر_Type {
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
    c_m3_Type_گنتی,

    // "not a valid type" - returned where a lookup or a mapping failed.
    c_m3Type_unknown = c_m3_Type_گنتی
} M3_قدر_Type;

قسمتعریف ساخت M3_Tagged_قدر {
    M3_قدر_Type type;
    اتحاد M3_قدر_Union {
        uint32_t    i32;
        uint64_t    i64;
        اعشاری       f32;
        دوہرا      f64;
        ثابت خالی* ref;        // funcref / externref; NULL is the null reference
    } قدر;
} M3_Tagged_قدر, *IM3_Tagged_قدر;

قسمتعریف ساخت M3ImportInfo {
    ثابت حرف* moduleUtf8;
    ثابت حرف* میدان_Utf8;
} M3ImportInfo, *IM3ImportInfo;

قسمتعریف ساخت M3ImportContext {
    خالی*       userdata;
    IM3Function function;
} M3ImportContext, *IM3ImportContext;

قسمتعریف ثابت خالی* (*M3RawCall)(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t* _sp, خالی* _mem);

قسمتعریف M3_نتیجہ (*M3SectionHandler)(IM3Module i_module, ثابت حرف* name, ثابت uint8_t* آغاز, ثابت uint8_t* انتہا);


// -------------------------------------------------------------------------------------------------------------------------------
//  error codes
// -------------------------------------------------------------------------------------------------------------------------------

#اگر defined(M3_IMPLEMENT_ERROR_لڑیاں)
#  اگر defined(__cplusplus)
#    define d_m3ErrorConst(LABEL, STRING)      خارجی ثابت M3_نتیجہ m3Err_##LABEL = { STRING };
#  ورنہ
#    define d_m3ErrorConst(LABEL, STRING)      ثابت M3_نتیجہ m3Err_##LABEL = { STRING };
#  endif
#ورنہ
#  define d_m3ErrorConst(LABEL, STRING)        خارجی ثابت M3_نتیجہ m3Err_##LABEL;
#اگر_ختم

// -------------------------------------------------------------------------------------------------------------------------------
// clang-format off

d_m3ErrorConst(none,                           تہی)

// general errors
d_m3ErrorConst(mallocFailed,                   "memory allocation failed")

// parse errors
d_m3ErrorConst(incompatible_Wasm_نسخہ,        "incompatible Wasm binary version")
d_m3ErrorConst(wasmMalformed,                  "malformed Wasm binary")
d_m3ErrorConst(misorderedWasmSection,          "out of order Wasm section")
d_m3ErrorConst(wasmUnderrun,                   "underrun while parsing Wasm binary")
d_m3ErrorConst(wasmOverrun,                    "overrun while parsing Wasm binary")
d_m3ErrorConst(wasm_Missing_Init_اظہاریہ,            "missing init_expr in Wasm binary")
d_m3ErrorConst(leb_حد_سے_تجاوز,                    "LEB encoded value overflow")
d_m3ErrorConst(missingUTF8,                    "invalid length UTF-8 string")
d_m3ErrorConst(wasmSectionUnderrun,            "section underrun while parsing Wasm binary")
d_m3ErrorConst(wasmSectionOverrun,             "section overrun while parsing Wasm binary")
d_m3ErrorConst(invalidTypeId,                  "unknown value_type")
d_m3ErrorConst(too_Many_حافظہ_Sections,          "only one memory per module is supported")
d_m3ErrorConst(too_Many_دلائل_Rets,                "too many arguments or return values")

// link errors
d_m3ErrorConst(moduleNotLinked,                "attempting to use module that is not loaded")
d_m3ErrorConst(moduleAlreadyLinked,            "attempting to bind module to multiple runtimes")
d_m3ErrorConst(functionLookupFailed,           "function lookup failed")
d_m3ErrorConst(لیبل_Lookup_Failed,                "tag lookup failed")
d_m3ErrorConst(functionImportMissing,          "missing imported function")
d_m3ErrorConst(unknownImport,                  "unknown import")
d_m3ErrorConst(incompatibleImportType,         "incompatible import type")

d_m3ErrorConst(malformedFunctionSignature,     "malformed function signature")
d_m3ErrorConst(functionSignatureMismatch,      "function signature mismatch")

// compilation errors
d_m3ErrorConst(noCompiler,                     "no compiler found for opcode")
d_m3ErrorConst(unknownOpcode,                  "unknown opcode")
d_m3ErrorConst(restrictedOpcode,               "restricted opcode")
d_m3ErrorConst(function_ڈھیر_حد_سے_تجاوز,          "compiling function overran its stack height limit")
d_m3ErrorConst(function_ڈھیر_Underrun,          "compiling function underran the stack")
d_m3ErrorConst(malloc_Failed_رمز_صفحہ,           "memory allocation failed when acquiring a new M3 code page")
d_m3ErrorConst(settingImmutableGlobal,         "attempting to set an immutable global")
d_m3ErrorConst(typeMismatch,                   "incorrect type on stack")
d_m3ErrorConst(type_گنتی_Mismatch,              "incorrect value count on stack")

// validation errors. The wording follows the spec's own assert_invalid failure
d_m3ErrorConst(unknownType,                    "unknown type")
d_m3ErrorConst(unknownLabel,                   "unknown label")
d_m3ErrorConst(unknownLocal,                   "unknown local")
d_m3ErrorConst(unknownGlobal,                  "unknown global")
d_m3ErrorConst(unknownFunction,                "unknown function")
d_m3ErrorConst(unknown_جدول,                   "unknown table")
d_m3ErrorConst(unknown_لیبل,                     "unknown tag")
d_m3ErrorConst(unknown_حافظہ,                  "unknown memory")
d_m3ErrorConst(unknown_ڈیٹا_Segment,             "unknown data segment")
d_m3ErrorConst(unknownElemSegment,             "unknown elem segment")
d_m3ErrorConst(ڈیٹا_گنتی_Required,              "data count section required")
d_m3ErrorConst(invalidAlignment,               "alignment must not be larger than natural")
d_m3ErrorConst(invalidAtomicAlignment,         "atomic alignment must be natural")
d_m3ErrorConst(undeclaredFuncRef,              "undeclared function reference")

// runtime errors
d_m3ErrorConst(missing_Compiled_رمز,            "function is missing compiled m3 code")
d_m3ErrorConst(wasm_حافظہ_حد_سے_تجاوز,             "runtime ran out of memory")
d_m3ErrorConst(global_حافظہ_Not_Allocated,       "global memory is missing from a module")
d_m3ErrorConst(globa_اشاریہ_برآمد_کا_Bounds,          "global index is too large")
d_m3ErrorConst(دلیل_گنتی_Mismatch,          "argument count mismatch")
d_m3ErrorConst(دلیل_Type_Mismatch,           "argument type mismatch")
d_m3ErrorConst(globalLookupFailed,             "global lookup failed")
d_m3ErrorConst(globalTypeMismatch,             "global type mismatch")
d_m3ErrorConst(globalNotMutable,               "global is not mutable")
d_m3ErrorConst(حافظہ_Guards_Unavailable,        "memory guards could not be installed")

// resource limit errors
d_m3ErrorConst(حافظہ_حد_بندی_Exceeded,            "runtime memory limit exceeded")
d_m3ErrorConst(جدول_حد_بندی_Exceeded,             "table elements limit exceeded")
d_m3ErrorConst(continuation_حد_بندی_Exceeded,      "continuation limit exceeded")
d_m3ErrorConst(resource_حد_بندی_نیچے_Usage,        "resource limit below current usage")
d_m3ErrorConst(resource_حد_بندی_Not_Supported,      "resource limit not supported in this build")
d_m3ErrorConst(unknown_Resource_حد_بندی,           "unknown resource limit")

// traps
d_m3ErrorConst(trap_برآمد_کا_Bounds_حافظہ_Access,    "[trap] out of bounds memory access")
d_m3ErrorConst(trapUnalignedAtomic,            "[trap] unaligned atomic")
d_m3ErrorConst(trap_Expected_Shared_حافظہ,       "[trap] expected shared memory")
d_m3ErrorConst(trapWaitForever,                "[trap] wait would block forever")
d_m3ErrorConst(trapDivisionByZero,             "[trap] integer divide by zero")
d_m3ErrorConst(trap_Integer_حد_سے_تجاوز,            "[trap] integer overflow")
d_m3ErrorConst(trapIntegerConversion,          "[trap] invalid conversion to integer")
d_m3ErrorConst(trap_بالواسطہ_Call_Type_Mismatch,   "[trap] indirect call type mismatch")
d_m3ErrorConst(trap_جدول_اشاریہ_برآمد_کا_Range,       "[trap] undefined element")
d_m3ErrorConst(trap_جدول_Element_Is_Null,         "[trap] uninitialized element")
d_m3ErrorConst(trapNullReference,              "[trap] null reference")
d_m3ErrorConst(trapNullFunctionRef,            "[trap] null function reference")
// call_indirect past the end of the table is "undefined element"; the table
// access instructions report an out of bounds access instead
d_m3ErrorConst(trap_جدول_برآمد_کا_Bounds,           "[trap] out of bounds table access")
d_m3ErrorConst(trapWasiExit,                   "[trap] wasi program called exit")
d_m3ErrorConst(trapExit,                       "[trap] program called exit")
d_m3ErrorConst(trapAbort,                      "[trap] program called abort")
d_m3ErrorConst(trapUnreachable,                "[trap] unreachable")
d_m3ErrorConst(trapUnsupportedInstruction,     "[trap] unsupported instruction")
d_m3ErrorConst(trap_ڈھیر_حد_سے_تجاوز,              "[trap] stack overflow")
d_m3ErrorConst(trap_برآمد_کا_Gas,                   "[trap] out of gas")
d_m3ErrorConst(trapUncaughtException,          "[trap] uncaught exception")
d_m3ErrorConst(trapContinuationConsumed,       "[trap] continuation already consumed")
d_m3ErrorConst(trapNullContinuationRef,        "[trap] null continuation reference")
d_m3ErrorConst(trap_Unhandled_Control_لیبل,        "[trap] unhandled control tag")

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
IM3Environment m3_NewEnvironment (خالی);

خالی           m3_FreeEnvironment (IM3Environment i_environment);

خالی           m3_سیٹ_Custom_Section_Handler (IM3Environment i_environment, M3SectionHandler i_handler);


//-------------------------------------------------------------------------------------------------------------------------------
//  execution context
//-------------------------------------------------------------------------------------------------------------------------------

IM3Runtime     m3_NewRuntime (IM3Environment io_environment,
                              uint32_t       i_ڈھیر_سائز_اندر_بائٹیں,
                              خالی*          i_userdata);

خالی           m3_FreeRuntime (IM3Runtime i_runtime);

// The validator runs as a pre-pass of compiling a function body, so it
// follows lazy compilation: a body is checked the first time it is compiled.
// Turning it off means trusting the module - nothing then checks a body's
// types before it runs. On by default (a no-op in a build without validation).
خالی           m3_سیٹ_Validation (IM3Runtime i_runtime, bool i_enable);

// Zero means unlimited for allocation caps (memory, tables, continuations).
// Allocation limits below current usage are refused. Unsupported limits are
// refused even when the requested value is zero.
// Gas configures an execution budget (zero sets an empty/unmetered budget); arm
// a positive budget before compilation to instrument metered code. Gas saturates
// at INT64_MAX, continuations at UINT32_MAX. Setting an allocation limit leaves
// usage unchanged; setting gas starts a fresh execution budget.
M3_نتیجہ       m3_سیٹ_Resource_حد_بندی (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی, uint64_t i_قدر);
// Returns the effective (possibly saturated) limit; unsupported/unset limits are zero.
uint64_t       m3_Get_Resource_حد_بندی (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی);
// Usage is tracked even without a limit (for gas, requires arming before compilation).
// Unsupported and unknown keys return zero.
uint64_t       m3_GetResourceUsage (IM3Runtime i_runtime, M3_Resource_حد_بندی i_حد_بندی);

// A memory belongs to the module that declares it, so these take the module
// rather than the runtime - a runtime can hold several modules, each with
// its own memories. Returns NULL when the module has no memory at that
// index; o_memorySizeInBytes is set either way.
//
// Sizes are size_t, not uint32_t: a linear memory may be a full 4 GiB, which
// is one byte too many to count in 32 bits.
uint8_t*       m3_Get_حافظہ (IM3Module i_module,
                             size_t*   o_حافظہ_سائز_اندر_بائٹیں,
                             uint32_t  i_حافظہ_اشاریہ);

size_t         m3_Get_حافظہ_سائز (IM3Module i_module,
                                 uint32_t  i_حافظہ_اشاریہ);

// Size of the memory a pointer addresses into - specifically the _mem a raw
// function is handed, which is the memory of whichever module is calling,
// and need not be any particular module's. Used by m3ApiCheckMem.
size_t         m3_Get_حافظہ_سائز_At (ثابت خالی* i_حافظہ);

خالی*          m3_Get_User_ڈیٹا (IM3Runtime i_runtime);

// The index of the memory the module exports under i_name. A host module
// whose ABI names the memory it addresses looks it up this way - WASI names
// it "memory". Returns m3Err_unknownMemory when no memory is exported
// under that name.
M3_نتیجہ       m3_ڈھونڈو_Exported_حافظہ (IM3Module         i_module,
                                      ثابت حرف* ثابت i_name,
                                      uint32_t*         o_حافظہ_اشاریہ);

// Pin which of the module's memories the host functions imported from
// i_importModule address ("*" for every namespace, as in
// m3_LinkRawFunction). Applies to imports already bound to a host function,
// so call it after linking them. Left alone, a host function addresses
// memory 0 - what a bare i32 guest pointer means when nothing says
// otherwise - so this is only needed by a host module that names a memory.
M3_نتیجہ       m3_Bind_Import_حافظہ (IM3Module         io_module,
                                    ثابت حرف* ثابت i_importModule,
                                    uint32_t          i_حافظہ_اشاریہ);

//-------------------------------------------------------------------------------------------------------------------------------
//  modules
//-------------------------------------------------------------------------------------------------------------------------------

// i_wasmBytes data must be persistent during the lifetime of the module
M3_نتیجہ       m3_ParseModule (IM3Environment       i_environment,
                               IM3Module*           o_module,
                               ثابت uint8_t* ثابت i_wasm_بائٹیں,
                               uint32_t             i_عدد_Wasm_بائٹیں);

// Only a module that was never handed to m3_LoadModule needs to be freed.
خالی           m3_FreeModule (IM3Module i_module);

//  Transfers ownership of the module to the runtime - whether or not it
//  succeeds. A failed instantiation can already have written this module's
//  functions into a table another module owns, and the spec keeps whatever
//  it managed to do, so those entries stay callable; the module has to
//  outlive the failure for them not to dangle. m3_FreeRuntime releases it.
//  Do not call m3_FreeModule on a module after passing it here.
M3_نتیجہ       m3_بھرو_Module (IM3Runtime io_runtime, IM3Module io_module);

// Optional, compiles all functions in the module
M3_نتیجہ       m3_CompileModule (IM3Module io_module);

// Calling m3_RunStart is optional
M3_نتیجہ       m3_تسلسل_آغاز (IM3Module i_module);

// Arguments and return values are passed in and out through the stack pointer _sp.
// Placeholder return value slots are first and arguments after. So, the first argument is at _sp [numReturns]
// Return values should be written into _sp [0] to _sp [num_returns - 1]
M3_نتیجہ       m3_LinkRawFunction (IM3Module         io_module,
                                   ثابت حرف* ثابت i_moduleName,
                                   ثابت حرف* ثابت i_functionName,
                                   ثابت حرف* ثابت i_signature,
                                   M3RawCall         i_function);

M3_نتیجہ       m3_LinkRawFunctionEx (IM3Module         io_module,
                                     ثابت حرف* ثابت i_moduleName,
                                     ثابت حرف* ثابت i_functionName,
                                     ثابت حرف* ثابت i_signature,
                                     M3RawCall         i_function,
                                     ثابت خالی*       i_userdata);

// supplies the value of an imported global, regardless of its mutability
M3_نتیجہ       m3_LinkGlobal (IM3Module            io_module,
                              ثابت حرف* ثابت    i_moduleName,
                              ثابت حرف* ثابت    i_globalName,
                              ثابت IM3_Tagged_قدر i_قدر);

ثابت حرف*    m3_GetModuleName (IM3Module i_module);
خالی           m3_سیٹ_Module_Name (IM3Module i_module, ثابت حرف* name);
IM3Runtime     m3_GetModuleRuntime (IM3Module i_module);

// The module registered under i_moduleName, or NULL. Most recently loaded
// first, so a name registered twice names the newer module.
IM3Module      m3_ڈھونڈو_Module (IM3Runtime        i_runtime,
                              ثابت حرف* ثابت i_moduleName);

//-------------------------------------------------------------------------------------------------------------------------------
//  globals
//-------------------------------------------------------------------------------------------------------------------------------
IM3Global      m3_ڈھونڈو_Global (IM3Module         io_module,
                              ثابت حرف* ثابت i_globalName);

M3_نتیجہ       m3_GetGlobal (IM3Global      i_global,
                             IM3_Tagged_قدر o_قدر);

M3_نتیجہ       m3_سیٹ_Global (IM3Global            i_global,
                             ثابت IM3_Tagged_قدر i_قدر);

M3_قدر_Type    m3_GetGlobalType (IM3Global i_global);

//-------------------------------------------------------------------------------------------------------------------------------
//  functions
//-------------------------------------------------------------------------------------------------------------------------------
M3_نتیجہ       m3_Yield (خالی);

// o_function is valid during the lifetime of the originating runtime.
// m3_FindFunction searches every module loaded into the runtime, most
// recently loaded first; m3_FindFunctionIn searches just the one, which is
// what naming a module's export means.
M3_نتیجہ       m3_ڈھونڈو_Function (IM3Function*      o_function,
                                IM3Runtime        i_runtime,
                                ثابت حرف* ثابت i_functionName);
M3_نتیجہ       m3_ڈھونڈو_Function_اندر (IM3Function*      o_function,
                                  IM3Module         i_module,
                                  ثابت حرف* ثابت i_functionName);
M3_نتیجہ       m3_Get_جدول_Function (IM3Function* o_function,
                                    IM3Module    i_module,
                                    uint32_t     i_اشاریہ);

uint32_t       m3_Get_Arg_گنتی (IM3Function i_function);
uint32_t       m3_Get_Ret_گنتی (IM3Function i_function);
M3_قدر_Type    m3_GetArgType (IM3Function i_function, uint32_t i_اشاریہ);
M3_قدر_Type    m3_GetRetType (IM3Function i_function, uint32_t i_اشاریہ);

M3_نتیجہ       m3_CallV (IM3Function i_function, ...);
M3_نتیجہ       m3_CallVL (IM3Function i_function, va_list i_دلائل);
M3_نتیجہ       m3_Call (IM3Function i_function, uint32_t i_argc, ثابت خالی* i_argptrs[]);
M3_نتیجہ       m3_CallArgv (IM3Function i_function, uint32_t i_argc, ثابت حرف* i_argv[]);

M3_نتیجہ       m3_GetResultsV (IM3Function i_function, ...);
M3_نتیجہ       m3_GetResultsVL (IM3Function i_function, va_list o_rets);
M3_نتیجہ       m3_GetResults (IM3Function i_function, uint32_t i_retc, ثابت خالی* o_retptrs[]);


خالی           m3_GetErrorInfo (IM3Runtime i_runtime, M3ErrorInfo* o_info);
خالی           m3_ResetErrorInfo (IM3Runtime i_runtime);

ثابت حرف*    m3_GetFunctionName (IM3Function i_function);
IM3Module      m3_GetFunctionModule (IM3Function i_function);

//-------------------------------------------------------------------------------------------------------------------------------
//  exceptions
//-------------------------------------------------------------------------------------------------------------------------------
// A tag is the identity of an exception: a catch clause names one, and catches only
// what was thrown with it. Its parameters are the types of the payload the exception
// carries. A tag lives as long as the runtime its module was loaded into, or the
// runtime m3_NewTag made it in.

// The tag i_module exports as i_tagName, or NULL.
IM3_لیبل         m3_ڈھونڈو_لیبل (IM3Module         i_module,
                           ثابت حرف* ثابت i_لیبل_Name);

// A tag of the host's own, typed by a signature in m3_LinkRawFunction's notation:
// "v(iI)" carries an i32 and an i64. No module can name it, so only catch_all and
// catch_all_ref catch what is thrown with it - until m3_LinkTag hands it to one.
M3_نتیجہ       m3_New_لیبل (IM3Runtime        io_runtime,
                          IM3_لیبل*           o_لیبل,
                          ثابت حرف* ثابت i_signature);

// Satisfies a tag import with i_tag, which must be of the same type. Code refers
// to a tag as whatever it resolved to when it was compiled, so link before
// calling anything that throws or catches it. "*" matches any module name.
M3_نتیجہ       m3_Link_لیبل (IM3Module         io_module,
                           ثابت حرف* ثابت i_moduleName,
                           ثابت حرف* ثابت i_لیبل_Name,
                           IM3_لیبل            i_لیبل);

uint32_t       m3_Get_لیبل_Arg_گنتی (IM3_لیبل i_لیبل);
M3_قدر_Type    m3_Get_لیبل_Arg_Type (IM3_لیبل i_لیبل, uint32_t i_اشاریہ);

// Throws an exception from inside a raw host function, which returns what this
// returns: the exception, on its way to the nearest try_table that catches it, or
// a trap if it could not be thrown. i_argptrs point at the payload values, as
// m3_Call's arguments do. Only a tag without results can be thrown.
M3_نتیجہ       m3_ThrowException (IM3Runtime  io_runtime,
                                  IM3_لیبل      i_لیبل,
                                  uint32_t    i_argc,
                                  ثابت خالی* i_argptrs[]);

// The exception that ended the most recent call or resume on this runtime, when
// it ended with m3Err_trapUncaughtException; NULL after any other outcome. The
// payload is a copy, so it outlives the call, but an exnref within it does not.
IM3_لیبل         m3_Get_Exception_لیبل (IM3Runtime i_runtime);
M3_نتیجہ       m3_Get_Exception_دلائل (IM3Runtime  i_runtime,
                                    uint32_t    i_argc,
                                    ثابت خالی* o_argptrs[]);

//-------------------------------------------------------------------------------------------------------------------------------
//  snapshots & suspendable execution
//-------------------------------------------------------------------------------------------------------------------------------

قسمتعریف M3_نتیجہ (*M3SnapshotWriter)(ثابت خالی* i_ڈیٹا, size_t i_سائز, خالی* i_userdata);
قسمتعریف M3_نتیجہ (*M3SnapshotReader)(خالی* o_بفر, size_t i_سائز, خالی* i_userdata);

// A suspendable runtime can pause a call part way and pick it up again with
// m3_ResumeRuntime - in this process, or through a snapshot in another. Make it
// suspendable before anything compiles: only code compiled that way has the
// checks that pause.
//
// m3_RequestSuspend asks for a pause, which takes effect at the next pause
// point: a loop's back edge, or the start of a function's body. Every loop and
// every recursion passes one. A resume goes on past the point it paused at, so
// asking again before each m3_ResumeRuntime steps from one to the next.
خالی     m3_سیٹ_Suspendable (IM3Runtime io_runtime, bool i_suspendable);
خالی     m3_RequestSuspend (IM3Runtime io_runtime);
bool     m3_IsSuspended (IM3Runtime i_runtime);

// Saves the paused invocation, or a postmortem of the module the last call
// entered when nothing is paused; with neither, there is nothing to save.
M3_نتیجہ m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, خالی* i_userdata);
// Restores into a freshly instantiated module: loaded and linked, with none of
// its code run yet - its start function included. A restore that fails part way
// leaves a module that refuses to run, resume or be saved again, and has to be
// thrown away with its runtime.
M3_نتیجہ m3_بھرو_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, خالی* i_userdata);

M3_نتیجہ m3_Save_Snapshot_مقصد_بفر (IM3Runtime io_runtime, خالی** o_بائٹیں, size_t* o_سائز);
// Accepts a standalone container, a Wasm module with a default snapshot, or
// the bare section stream returned by m3_GetEmbeddedSnapshot.
M3_نتیجہ m3_بھرو_Snapshot_From_بفر (IM3Runtime io_runtime, IM3Module i_module, ثابت خالی* i_بائٹیں, size_t i_سائز);

// A snapshot can also ride inside the module it belongs to, as a custom
// section named "snapshot" or "snapshot.<name>". i_name selects one; NULL or
// "" means the unnamed default. Two sections of the same name are refused
// when that name is selected, not when the module is parsed.
bool     m3_HasSnapshot (IM3Module i_module, ثابت حرف* i_name);

// Points o_bytes at the embedded snapshot, which belongs to the module and
// lives as long as the bytes it was parsed from. The payload has no standalone header.
M3_نتیجہ m3_GetEmbeddedSnapshot (IM3Module i_module, ثابت حرف* i_name,
                                 ثابت خالی** o_بائٹیں, size_t* o_سائز);

M3_نتیجہ m3_بھرو_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name);

// Writes the module's bytes with the paused invocation embedded in them,
// replacing any snapshot of the same name already there. A postmortem is not
// embedded: with nothing paused, this fails. Release o_bytes with free().
M3_نتیجہ m3_Save_Snapshot_مقصد_Module (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name,
                                  خالی** o_بائٹیں, size_t* o_سائز);

// What a snapshot cannot name on its own belongs to the embedder: an externref,
// and whatever the program depends on outside its Wasm state - open files,
// clocks, anything its imports keep. Any of these may be left NULL.
قسمتعریف ساخت M3SnapshotHooks {
    // An externref as a number the embedder can turn back into the same
    // reference in another process. Without nameExternRef a non-null externref
    // refuses the save; without bindExternRef a snapshot holding one is refused.
    // UINT64_MAX is not a name: it is what a null reference is written as.
    M3_نتیجہ (*nameExternRef)(خالی* i_userdata, خالی* i_reference, uint64_t* o_name);
    M3_نتیجہ (*bindExternRef)(خالی* i_userdata, uint64_t i_name, خالی** o_reference);

    // The embedder's own state, carried after the program's. saveHostState
    // writes it through i_writer; loadHostState reads exactly those i_size bytes
    // back through i_reader. A snapshot that carries some is refused by a
    // runtime with no loadHostState.
    M3_نتیجہ (*saveHostState)(خالی* i_userdata, M3SnapshotWriter i_writer, خالی* i_writer_ڈیٹا);
    M3_نتیجہ (*بھرو_Host_State_2)(خالی* i_userdata, M3SnapshotReader i_reader, خالی* i_reader_ڈیٹا, size_t i_سائز);

    خالی* userdata;
} M3SnapshotHooks;

خالی             m3_سیٹ_Snapshot_Hooks (IM3Runtime io_runtime, ثابت M3SnapshotHooks* i_hooks);

M3_نتیجہ         m3_ResumeRuntime (IM3Runtime io_runtime);


//-------------------------------------------------------------------------------------------------------------------------------
//  debug info
//-------------------------------------------------------------------------------------------------------------------------------

خالی             m3_PrintRuntimeInfo (IM3Runtime i_runtime);
خالی             m3_PrintM3Info (خالی);
خالی             m3_PrintProfilerInfo (خالی);

// The runtime owns the backtrace, do not free the backtrace you obtain. Returns NULL if there's no backtrace.
IM3BacktraceInfo m3_GetBacktrace (IM3Runtime i_runtime);

//-------------------------------------------------------------------------------------------------------------------------------
//  raw function definition helpers
//-------------------------------------------------------------------------------------------------------------------------------

#تعریف_کرو m3_Api_انحراف_مقصد_Ptr(offset)   (خالی*)((uint8_t*)_mem + (uint32_t)(offset))
#تعریف_کرو m3_Api_Ptr_مقصد_انحراف(ptr)      (uint32_t)((uint8_t*)ptr - (uint8_t*)_mem)

// Each of these walks _sp as it goes, so an entry point has to name every
// argument it is handed whether or not its implementation has a use for the
// value - which is why the declarations carry M3_UNUSED rather than the build
// turning the warning off for everything.
#تعریف_کرو m3ApiReturnType(TYPE)                 TYPE* raw_return M3_غیر_مستعمل = ((TYPE*) (_sp++));
#تعریف_کرو m3_Api_Multi_قدر_Return_Type(TYPE, NAME) TYPE* NAME M3_غیر_مستعمل = ((TYPE*) (_sp++));
#تعریف_کرو m3ApiGetArg(TYPE, NAME)               TYPE NAME M3_غیر_مستعمل = \
   (حجم(TYPE) >= حجم(uint32_t)) ? \
   (*((TYPE *)(_sp++))) : \
   ((TYPE)(uintptr_t)(*((uint32_t *)(_sp++))));
#تعریف_کرو m3ApiGetArgMem(TYPE, NAME)            TYPE NAME M3_غیر_مستعمل = (TYPE)m3_Api_انحراف_مقصد_Ptr(* ((uint32_t *) (_sp++)));

#تعریف_کرو m3ApiIsNullPtr(addr)       ((خالی*)(addr) <= _mem)

// Whether [addr, addr+len) lies inside the memory _mem points at. Written as
// two subtractions rather than base + size and addr + len, so that neither a
// memory occupying the top of the address space nor a wild length can carry the
// comparison past where it wraps. Its own block, so the locals go out of scope
// with it.
#تعریف_کرو m3ApiCheckMem(addr, len)                                                   \
   {   uintptr_t m3_بنیاد = (uintptr_t)(_mem);                                     \
       uintptr_t _m3_addr = (uintptr_t)(خالی*)(addr);                              \
       size_t    m3_سائز = m3_Get_حافظہ_سائز_At(_mem);                              \
       اگر (M3_UNLIKELY(_m3_addr < m3_بنیاد ||                                      \
                       (size_t)(_m3_addr - m3_بنیاد) > m3_سائز ||                 \
                       (uint64_t)(len) > (uint64_t)(m3_سائز - (size_t)(_m3_addr - m3_بنیاد)))) \
           m3ApiTrap(m3_Err_trap_برآمد_کا_Bounds_حافظہ_Access);                           \
   }

#تعریف_کرو m3ApiRawFunction(NAME)               ثابت خالی * NAME (IM3Runtime runtime, IM3ImportContext _ctx, uint64_t * _sp, خالی * _mem)
#تعریف_کرو m3ApiReturn(قدر_2)                   { *raw_return = (قدر_2); واپس m3Err_none;}
#تعریف_کرو m3_Api_Multi_قدر_Return(NAME, قدر_2)   { *NAME = (قدر_2); }
#تعریف_کرو m3ApiTrap(قدر_2)                     { واپس قدر_2; }
#تعریف_کرو m3ApiThrow(لیبل_2, ARGC, ARGPTRS)       { واپس m3_ThrowException(runtime, لیبل_2, ARGC, ARGPTRS); }
#تعریف_کرو m3ApiSuccess()                       { واپس m3Err_none; }

#اگر defined(M3_BIG_ENDIAN)
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        m3_bswap16((* (uint16_t *)(ptr)))
#  define m3ApiReadMem32(ptr)        m3_bswap32((* (uint32_t *)(ptr)))
#  define m3ApiReadMem64(ptr)        m3_bswap64((* (uint64_t *)(ptr)))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr)  = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = m3_bswap16((val)); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = m3_bswap32((val)); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = m3_bswap64((val)); }
#ورنہ
#  define m3ApiReadMem8(ptr)         (* (uint8_t *)(ptr))
#  define m3ApiReadMem16(ptr)        (* (uint16_t *)(ptr))
#  define m3ApiReadMem32(ptr)        (* (uint32_t *)(ptr))
#  define m3ApiReadMem64(ptr)        (* (uint64_t *)(ptr))
#  define m3ApiWriteMem8(ptr, val)   { * (uint8_t  *)(ptr) = (val); }
#  define m3ApiWriteMem16(ptr, val)  { * (uint16_t *)(ptr) = (val); }
#  define m3ApiWriteMem32(ptr, val)  { * (uint32_t *)(ptr) = (val); }
#  define m3ApiWriteMem64(ptr, val)  { * (uint64_t *)(ptr) = (val); }
#اگر_ختم

#اگر defined(__cplusplus)
}
#اگر_ختم

#اگر_ختم // wasm3_h
