//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_core_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_core_h

#ਅਨ੍ਤਰ੍ਭਾਵ <stdio.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdlib.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdbool.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <string.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <assert.h>

#ਅਨ੍ਤਰ੍ਭਾਵ "wasm3.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_config.h"

#ਜੇ defined(__cplusplus)
#  define d_m3BeginExternC     ਬਾਹਰੀ "C" {
#  define d_m3_ਸਿਰਾ_Extern_C       }
#ਨਹੀਂਤਾਂ
#  define d_m3BeginExternC
#  define d_m3_ਸਿਰਾ_Extern_C
#ਜੇ_ਅੰਤ

d_m3BeginExternC

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3ImplementFloat (d_m3HasFloat || d_m3NoFloatDynamic)

#ਜੇ !defined(d_m3ShortTypesDefined)

ਕਿਸਮਪਰਿਭਾਸ਼ਾ uint64_t u64;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ int64_t  i64;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ uint32_t u32;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ int32_t  i32;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ uint16_t u16;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ int16_t  i16;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ uint8_t  u8;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ int8_t   i8;

#  ਜੇ d_m3ImplementFloat
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਦੁੱਗਣਾ f64;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਦਸ਼ਮਲਵ  f32;
#  endif

#ਜੇ_ਅੰਤ // d_m3ShortTypesDefined

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ PRIf32          "f"
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ PRIf64          "lf"

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ ਖਾਲੀ*       m3ret_t;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ ਖਾਲੀ*       voidptr_t;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ ਅੱਖਰ*       cstr_t;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ ਅੱਖਰ* ਅਚਲ ccstr_t;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ u8*         ਬਾਈਟਾਂ_t;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ u8* ਅਚਲ   cbytes_t;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ u16               m3opcode_t;
#ਜੇ d_m3HasTypedRefs
ਕਿਸਮਪਰਿਭਾਸ਼ਾ u16 m3type_t;       // a value type, see the d_m3Type_* bits below
#ਨਹੀਂਤਾਂ
ਕਿਸਮਪਰਿਭਾਸ਼ਾ u8 m3type_t;       // only plain M3ValueType values exist
#ਜੇ_ਅੰਤ

ਕਿਸਮਪਰਿਭਾਸ਼ਾ i64 m3reg_t;

#ਜੇ d_m3_Use32_ਦੋਆਧਾਰੀ_ਅੰਕ_Slots
ਕਿਸਮਪਰਿਭਾਸ਼ਾ u32 m3slot_t;
#ਨਹੀਂਤਾਂ
ਕਿਸਮਪਰਿਭਾਸ਼ਾ u64 m3slot_t;
#ਜੇ_ਅੰਤ

ਕਿਸਮਪਰਿਭਾਸ਼ਾ m3slot_t* m3stack_t;

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ c_io_ਥਾਂ_ਗਿਣਤੀ ((u16)(ਆਕਾਰ(u64) / ਆਕਾਰ(m3slot_t)))

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ ਖਾਲੀ* ਅਚਲ cvptr_t;

#ਜੇ defined(DEBUG)

#  define d_m3Log(CATEGORY, FMT, ...)                ਲਿਖੋ (" %8s  |  " FMT, #CATEGORY, ##__VA_ARGS__);

#  ਜੇ d_m3LogParse
#    define m3log_parse(CATEGORY, FMT, ...)          d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ਨਹੀਂਤਾਂ
#    define m3log_parse(...) {}
#  endif

#  ਜੇ d_m3LogCompile
#    define m3log_compile(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ਨਹੀਂਤਾਂ
#    define m3log_compile(...) {}
#  endif

#  ਜੇ d_m3LogEmit
#    define m3log_emit(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ਨਹੀਂਤਾਂ
#    define m3log_emit(...) {}
#  endif

#  ਜੇ d_m3_Log_ਕੋਡ_ਪੰਨੇ
#    define m3log_ਕੋਡ(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ਨਹੀਂਤਾਂ
#    define m3log_ਕੋਡ(...) {}
#  endif

#  ਜੇ d_m3LogModule
#    define m3log_module(CATEGORY, FMT, ...)         d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ਨਹੀਂਤਾਂ
#    define m3log_module(...) {}
#  endif

#  ਜੇ d_m3LogRuntime
#    define m3log_runtime(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ਨਹੀਂਤਾਂ
#    define m3log_runtime(...) {}
#  endif

#  define m3log(CATEGORY, FMT, ...)                  m3log_##CATEGORY (CATEGORY, FMT "\n", ##__VA_ARGS__)
#ਨਹੀਂਤਾਂ
#  define d_m3Log(CATEGORY, FMT, ...)                {}
#  define m3log(CATEGORY, FMT, ...)                  {}
#ਜੇ_ਅੰਤ


#ਜੇ defined(ASSERTS) || (defined(DEBUG) && !defined(NASSERTS))
#  define d_m3Assert(ASS)  ਜੇ (!(ASS)) { ਲਿਖੋ("Assertion failed at %s:%d : %s\n", __FILE__, __LINE__, #ASS); fflush(stdout); abort(); }
#ਨਹੀਂਤਾਂ
#  define d_m3Assert(ASS)
#ਜੇ_ਅੰਤ

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਖਾਲੀ /*const*/*                ਕੋਡ_t;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਕੋਡ_t ਅਚਲ* /*__restrict__*/ pc_t;


// Sits immediately before the linear memory it describes, so m3MemData() is
// just (header + 1). The interpreter carries a pointer to one of these in the
// _mem pseudo-register, and reaches everything else about the memory - its page
// counts, its owning runtime - through the back-pointers here. 'memory' is what
// makes that possible without going through the runtime, which is the
// prerequisite for a runtime holding more than one memory.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ {
    IM3Runtime       runtime;
    ਖਾਲੀ*            max_ਢੇਰ;
    size_t           ਲੰਮਾਈ;
    ਬਣਤਰ M3_ਸਮ੍ਰਿਤੀ* ਸਮ੍ਰਿਤੀ;         // the M3Memory this block belongs to
} M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ;

ਬਣਤਰ M3_ਕੋਡ_Mapping_ਪੰਨਾ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਕੋਡ_ਪੰਨਾ_ਸਿਰਲੇਖ {
    ਬਣਤਰ M3_ਕੋਡ_ਪੰਨਾ* ਅਗਲਾ;

    u32                ਸਤਰ_ਸੂਚਕਾਂਕ;
    u32                ਅੰਕ_ਸਤਰਾਂ;
    u32                sequence;       // this is just used for debugging; could be removed
    u32                usage_ਗਿਣਤੀ;

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
    ਬਣਤਰ M3_ਕੋਡ_Mapping_ਪੰਨਾ* mapping;
#ਜੇ_ਅੰਤ // d_m3RecordBacktraces
} M3_ਕੋਡ_ਪੰਨਾ_ਸਿਰਲੇਖ;


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਕੋਡ_ਪੰਨਾ_Free_ਸਤਰਾਂ_ਹੱਦ      7+2       // max is: ReturnCallIndirect (op + 6 immediates) + 2 for bridge

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Default_Mem_ਪੰਨਾ_ਮਾਪ              65536

// The ceiling on a linear memory address, and so on the size of a linear
// memory. No allocator reaches anywhere near it - a memory is one contiguous
// realloc'd block, and d_m3MaxLinearMemoryPages caps it far lower - so an
// address at or above this is out of bounds no matter which memory it names.
//
// That is what a 64-bit address is checked against before anything is added to
// it. Once both the address and the memarg offset are known to be below 2^52,
// their sum is below 2^53 and cannot wrap, so the u65 effective address the
// spec calls for can be computed in a plain u64 - and stays well inside the
// positive range of m3reg_t, which is signed. Anything at or above the limit
// traps, which is what an address that far out would do anyway.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ_ਬਿੱਟ                52
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ                    (UINT64_C(1) << d_m3_ਪਤਾ_ਹੱਦਬੰਦੀ_ਬਿੱਟ)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Reg0_ਥਾਂ_Alias                   60000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Fp0_ਥਾਂ_Alias                    (d_m3_Reg0_ਥਾਂ_Alias + 2)

#ਜੇ d_m3HasTypedRefs
// a heap type index has to fit in the spare bits of an m3type_t, see below
#  define d_m3_Max_Sane_Types_ਗਿਣਤੀ             4094
#ਨਹੀਂਤਾਂ
// M3Environment.numFuncTypes is the u16 that hands out M3FuncType.canonicalIndex,
// so the count has to stay below 65536 here as well
#  define d_m3_Max_Sane_Types_ਗਿਣਤੀ             65500
#ਜੇ_ਅੰਤ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_Functions_ਗਿਣਤੀ           1000000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_Imports_ਗਿਣਤੀ             100000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_Exports_ਗਿਣਤੀ             100000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_Globals_ਗਿਣਤੀ             1000000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_ਟੈਗ_ਸੂਚੀ_ਗਿਣਤੀ                100000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3MaxSaneElementSegments          10000000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_ਅੰਕੜੇ_Segments             100000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_ਸਾਰਣੀ_ਮਾਪ                10000000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_Memories_ਗਿਣਤੀ            10000
#ਜੇ d_m3HasRefTypes
#  define d_m3_Max_Sane_ਸਾਰਣੀ_ਗਿਣਤੀ             1000
#ਨਹੀਂਤਾਂ
#  define d_m3_Max_Sane_ਸਾਰਣੀ_ਗਿਣਤੀ             1           // MVP: a single table
#ਜੇ_ਅੰਤ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_Utf8_ਲੰਮਾਈ               10000
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Max_Sane_Function_Arg_Ret_ਗਿਣਤੀ      1000    // still insane, but whatever

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_external_ਕਿਸਮ_function             0
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_external_ਕਿਸਮ_ਸਾਰਣੀ                1
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_external_ਕਿਸਮ_ਸਮ੍ਰਿਤੀ               2
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_external_ਕਿਸਮ_global               3
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_external_ਕਿਸਮ_ਟੈਗ                  4

// compact-import-section: these stand where an externtype would, after an empty
// item name, and introduce a run of imports sharing one module name. 0x7f gives
// each item its own externtype, 0x7e shares a single one across the whole run.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_compact_Imports_ਪ੍ਰਤੀ_ਵਸਤੂ_Type        0x7f
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_compactImports_sharedType         0x7e

// the negated wasm encodings of the abstract reference types, as NormalizeType()
// stores them
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_waType_funcref                    16
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_waType_externref                  17

// exception handling: exnref is wasm-encoded as -0x17
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_waType_exnref                     23

// stack switching: cont type form is -0x23 (35); heap types: cont is -0x18 (24), nocont is -0x0b (11)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_waType_cont                       35
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_wa_Type_ਢੇਰ_cont                  24
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_wa_Type_ਢੇਰ_nocont                11

// function-references: a reference type spelled out as (ref ht) / (ref null ht),
// where the abstract heap types reuse the funcref and externref encodings
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_waEncode_ref                      0x64
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_waEncode_refNull                  0x63

// indexed by M3ValueType: c_m3Type_count entries for the concrete types, plus
// one more for c_m3Type_unknown, which sits right after them
ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ c_waTypes[] = { "nil", "i32", "i64", "f32", "f64", "v128", "funcref", "externref", "exnref", "contref", "unknown" };
ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ c_waCompactTypes[] = { "_", "i", "I", "f", "F", "V", "r", "R", "e", "c", "?" };


#ਜੇ d_m3VerboseErrorMessages

M3_ਨਤੀਜਾ m3Error (M3_ਨਤੀਜਾ i_ਨਤੀਜਾ, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ਅਚਲ ਅੱਖਰ* ਅਚਲ i_file, u32 i_ਸਤਰ_ਅੰਕ, ਅਚਲ ਅੱਖਰ* ਅਚਲ i_error_ਸੁਨੇਹਾ, ...);

#  define _m3Error(ਨਤੀਜਾ_2, RT, MOD, FUN, FILE, ਸਤਰ, FORMAT, ...) \
            m3Error (ਨਤੀਜਾ_2, RT, MOD, FUN, FILE, ਸਤਰ, FORMAT, ##__VA_ARGS__)

#ਨਹੀਂਤਾਂ
#  define _m3Error(ਨਤੀਜਾ_2, RT, MOD, FUN, FILE, ਸਤਰ, FORMAT, ...) (ਨਤੀਜਾ_2)
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ErrorRuntime(ਨਤੀਜਾ_2, RUNTIME, FORMAT, ...)      _m3Error (ਨਤੀਜਾ_2, RUNTIME, ਨਿਰਾ, ਨਿਰਾ,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ErrorModule(ਨਤੀਜਾ_2, MOD, FORMAT, ...)           _m3Error (ਨਤੀਜਾ_2, MOD->runtime, MOD, ਨਿਰਾ,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ ErrorCompile(ਨਤੀਜਾ_2, COMP, FORMAT, ...)         _m3Error (ਨਤੀਜਾ_2, COMP->runtime, COMP->module, ਨਿਰਾ, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)

#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ

// How much address space a linear memory's bytes are given: everything a Wasm access
// can name. A 32-bit address plus a 32-bit memarg offset reaches just under 2^33, and
// the access itself a few bytes further, which the page of slack in a slot covers.
#  define d_m3_Guarded_ਅੰਕੜੇ_ਬਾਈਟਾਂ  (UINT64_C(1) << 33)

// Take one slot of the guarded arena, or NULL when the arena has none left or could
// not be reserved at all. What comes back is the base of the slot: the memory's
// M3MemoryHeader sits at the end of its first page, and the bytes the guest addresses
// begin on the page after that.
ਖਾਲੀ*           Guard_Take_ਥਾਂ (ਖਾਲੀ);

// The header of a slot, which is also what the guest's memory starts right after
M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* Guard_ਥਾਂ_ਸਿਰਲੇਖ (ਖਾਲੀ* i_ਥਾਂ);

// Make the first i_dataBytes of the slot's data reachable. Only ever asked to grow,
// and asking for what is already there costs nothing.
bool            Guard_Commit_ਥਾਂ (ਖਾਲੀ* i_ਥਾਂ, size_t i_ਅੰਕੜੇ_ਬਾਈਟਾਂ);

// Give a slot back, dropping everything committed in it. The next taker gets it
// zeroed, as a fresh linear memory has to be.
ਖਾਲੀ            Guard_Give_ਥਾਂ (ਖਾਲੀ* i_ਥਾਂ);

// The whole arena, for a fault to be held against - see m3_HostProtectedCall. Answers
// (NULL, 0) before anything has asked for a slot.
ਖਾਲੀ            Guard_ArenaRange (ਖਾਲੀ** o_ਨੀਵਾਂ, size_t* o_ਬਾਈਟਾਂ);

#ਜੇ_ਅੰਤ // d_m3GuardedMemory

#ਜੇ d_m3_Max_Native_ਢੇਰ > 0
// The native-stack low-water mark for a top-level call that came in at i_stackPtr,
// clamped to what the calling thread's stack really is where that can be measured.
// See d_m3MaxNativeStack and d_m3StackLimitEnter.
ਖਾਲੀ* m3_Native_ਢੇਰ_ਹੱਦਬੰਦੀ (ਖਾਲੀ* i_ਢੇਰ_Ptr, size_t i_budget);
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_Log_Native_ਢੇਰ
ਖਾਲੀ m3_ਢੇਰ_Check_Init ();
ਖਾਲੀ m3_ਢੇਰ_Check ();
ਪੂਰਨ  m3_ਢੇਰ_Get_Max ();
#ਨਹੀਂਤਾਂ
#  define     m3_ਢੇਰ_Check_Init()
#  define     m3_ਢੇਰ_Check()
#  define     m3_ਢੇਰ_Get_Max()         0
#ਜੇ_ਅੰਤ

#ਜੇ d_m3LogTimestamps
#  define     PRIts                   "%llu"
uint64_t m3_GetTimestamp ();
#ਨਹੀਂਤਾਂ
#  define     PRIts                   "%s"
#  define     m3_GetTimestamp()       ""
#ਜੇ_ਅੰਤ

ਖਾਲੀ  m3_Abort (ਅਚਲ ਅੱਖਰ* ਸੁਨੇਹਾ);
ਖਾਲੀ* m3_Malloc_Impl (size_t i_ਮਾਪ);
ਖਾਲੀ* m3_Realloc_Impl (ਖਾਲੀ* i_ptr, size_t i_new_ਮਾਪ, size_t i_old_ਮਾਪ);
ਖਾਲੀ  m3_Free_Impl (ਖਾਲੀ* i_ptr);
ਖਾਲੀ* m3_CopyMem (ਅਚਲ ਖਾਲੀ* i_from, size_t i_ਮਾਪ);

#ਜੇ d_m3_Log_ਢੇਰ_Ops

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ* m3_AllocStruct_Impl (ccstr_t name, size_t i_ਮਾਪ)
{
    ਖਾਲੀ* ਨਤੀਜਾ = m3_Malloc_Impl(i_ਮਾਪ);
    fprintf(stderr, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_ਮਾਪ, ਨਤੀਜਾ);
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ* m3_AllocArray_Impl (ccstr_t name, size_t i_ਅੰਕ, size_t i_ਮਾਪ)
{
    ਖਾਲੀ* ਨਤੀਜਾ = m3_Malloc_Impl(i_ਮਾਪ * i_ਅੰਕ);
    fprintf(stderr, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_ਮਾਪ, i_ਅੰਕ, ਨਤੀਜਾ);
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ* m3_ReallocArray_Impl (ccstr_t name, ਖਾਲੀ* i_ptr_old, size_t i_ਅੰਕ_new, size_t i_ਅੰਕ_old, size_t i_ਮਾਪ)
{
    ਖਾਲੀ* ਨਤੀਜਾ = m3_Realloc_Impl(i_ptr_old, i_ਮਾਪ * i_ਅੰਕ_new, i_ਮਾਪ * i_ਅੰਕ_old);
    fprintf(stderr, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_ਮਾਪ, i_ਅੰਕ_new, ਨਤੀਜਾ, i_ਅੰਕ_old, i_ptr_old);
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ* m3_Malloc (ccstr_t name, size_t i_ਮਾਪ)
{
    ਖਾਲੀ* ਨਤੀਜਾ = m3_Malloc_Impl(i_ਮਾਪ);
    fprintf(stderr, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_ਮਾਪ, ਨਤੀਜਾ);
    ਵਾਪਸ ਨਤੀਜਾ;
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ* m3_Realloc (ccstr_t name, ਖਾਲੀ* i_ptr, size_t i_new_ਮਾਪ, size_t i_old_ਮਾਪ)
{
    ਖਾਲੀ* ਨਤੀਜਾ = m3_Realloc_Impl(i_ptr, i_new_ਮਾਪ, i_old_ਮਾਪ);
    fprintf(stderr, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_new_ਮਾਪ, ਨਤੀਜਾ, i_old_ਮਾਪ, i_ptr);
    ਵਾਪਸ ਨਤੀਜਾ;
}

#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, ਆਕਾਰ (STRUCT))
#  define     m3_AllocArray(STRUCT, ਅੰਕ)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, ਅੰਕ, ਆਕਾਰ (STRUCT))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (ਖਾਲੀ *)(PTR), (NEW), (OLD), ਆਕਾਰ (STRUCT))
#  define     m3_Free(P)                              ਕਰੋ { ਖਾਲੀ* p = (ਖਾਲੀ*)(P);                                  \
                                                          ਜੇ (p) { fprintf(stderr, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                          m3_Free_Impl (p); (P) = ਨਿਰਾ; } ਜਦੋਂ(0)
#ਨਹੀਂਤਾਂ
#  define     m3_Malloc(NAME, ਮਾਪ_2)                   m3_Malloc_Impl(ਮਾਪ_2)
#  define     m3_Realloc(NAME, PTR, NEW, OLD)         m3_Realloc_Impl(PTR, NEW, OLD)
#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_Malloc_Impl (ਆਕਾਰ (STRUCT))
#  define     m3_AllocArray(STRUCT, ਅੰਕ)              (STRUCT *)m3_Malloc_Impl (ਆਕਾਰ (STRUCT) * (ਅੰਕ))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_Realloc_Impl ((ਖਾਲੀ *)(PTR), ਆਕਾਰ (STRUCT) * (NEW), ਆਕਾਰ (STRUCT) * (OLD))
#  define     m3_Free(P)                              ਕਰੋ { m3_Free_Impl ((ਖਾਲੀ*)(P)); (P) = ਨਿਰਾ; } ਜਦੋਂ(0)
#ਜੇ_ਅੰਤ

// A value type as the parser, compiler and validator carry it. The plain
// M3ValueType values keep their encoding, so every comparison against a
// c_m3Type_* constant still means exactly what it did. Only a typed function
// reference - (ref $t) or (ref null $t) - needs more room, and it spends it on
// the canonical index of the function type it points at. Fitting that index
// here is why d_m3MaxSaneTypesCount is 8190 rather than something rounder.
//
//   bit 15     spelled-out reference type rather than a plain M3ValueType
//   bit 14     non-null, i.e. (ref ht) rather than (ref null ht)
//   bit 13     heap type extern rather than func
//   bit 12     heap type cont rather than func
//   bits 11-0  canonical function type index, or d_m3Type_heapAbstract

#ਜੇ d_m3HasTypedRefs

#  define d_m3Type_ref                0x8000u
#  define d_m3Type_refNonNull         0x4000u
#  define d_m3Type_refExtern          0x2000u
#  define d_m3Type_refCont            0x1000u
#  define d_m3_Type_ਢੇਰ_ਨਕਾਬ           0x0FFFu
#  define d_m3_Type_ਢੇਰ_Abstract       0x0FFFu     // plain 'func', 'extern', or 'cont'
// 'nocont', the bottom of the continuation types: a reference to it is a
// reference to every one of them, which is what makes ref.null nocont usable
// wherever a particular continuation is expected. One below heapAbstract, and
// so out of reach of d_m3MaxSaneTypesCount.
#  define d_m3_Type_ਢੇਰ_None           0x0FFEu

// The storage type a value of this type occupies: a reference of any shape is
// just a funcref or an externref once the type checking is done with.
u8   ਅਧਾਰ_Type_ਦਾ (m3type_t i_type);
bool Is_Sub_Type_ਦਾ (m3type_t i_sub, m3type_t i_super);

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool IsSpelledRefType (m3type_t i_type)
{
    ਵਾਪਸ (i_type & d_m3Type_ref) != 0;
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool IsNullableRef (m3type_t i_type)
{
    ਵਾਪਸ (i_type & d_m3Type_refNonNull) == 0;
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
u32 ਢੇਰ_Type_ਦਾ (m3type_t i_type)
{
    ਵਾਪਸ IsSpelledRefType(i_type) ? (i_type & d_m3_Type_ਢੇਰ_ਨਕਾਬ) : d_m3_Type_ਢੇਰ_Abstract;
}

#ਨਹੀਂਤਾਂ // no spelled-out reference types, so every value type is its own storage
      // type and subtyping collapses back to equality

ਸਥਿਰ ਸਤਰਅੰਦਰ
u8 ਅਧਾਰ_Type_ਦਾ (m3type_t i_type)
{
    ਵਾਪਸ (u8)i_type;
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_Sub_Type_ਦਾ (m3type_t i_sub, m3type_t i_super)
{
    ਵਾਪਸ i_sub == i_super;
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool IsSpelledRefType (m3type_t i_type)
{
    (ਖਾਲੀ)i_type;
    ਵਾਪਸ false;
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool IsNullableRef (m3type_t i_type)
{
    (ਖਾਲੀ)i_type;
    ਵਾਪਸ true;
}

#ਜੇ_ਅੰਤ // d_m3HasTypedRefs

M3_ਨਤੀਜਾ NormalizeType (u8* o_type, i8 i_convolutedWasmType);

// These all speak in terms of how a value is stored, so they reduce a
// spelled-out reference type to its funcref / externref base first.
bool     IsIntType (m3type_t i_wasmType);
bool     IsFpType (m3type_t i_wasmType);
bool     IsRefType (m3type_t i_m3Type);
bool     Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type (m3type_t i_m3Type);
u32      ਮਾਪ_ਦਾ_Type (m3type_t i_m3Type);

M3_ਨਤੀਜਾ Read_u64 (u64* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ Read_u32 (u32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
#ਜੇ d_m3ImplementFloat
M3_ਨਤੀਜਾ Read_f64 (f64* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ Read_f32 (f32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
#ਜੇ_ਅੰਤ
M3_ਨਤੀਜਾ Read_u8 (u8* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ Read_opcode (m3opcode_t* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);

M3_ਨਤੀਜਾ ReadLebUnsigned (u64* o_ਮੁੱਲ, u32 i_max_ਅੰਕ_ਬਿੱਟ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ ReadLebSigned (i64* o_ਮੁੱਲ, u32 i_max_ਅੰਕ_ਬਿੱਟ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ ReadLEB_u32 (u32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);

// The memarg immediate of a load or store: an alignment hint, then a static
// offset. Multi-memory reinterprets the alignment as a bitfield -- bit 6 says a
// memory index sits between the two -- so the hint is returned with that bit
// masked off and the index defaults to 0 when it is clear.
M3_ਨਤੀਜਾ Read_ਸਮ੍ਰਿਤੀ_Arg (u32* o_align, u32* o_ਸਮ੍ਰਿਤੀ_Idx, u64* o_ਵਿੱਥ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ ReadLEB_u7 (u8* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ ReadLEB_i7 (i8* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ ReadLEB_i32 (i32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ ReadLEB_i64 (i64* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
M3_ਨਤੀਜਾ Read_utf8 (cstr_t* o_utf8, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);

#ਜੇ d_m3HasSnapshots
// XXH64, in m3_xxh64.c. Initialize a state, feed it the input in any number of
// pieces, then read the digest out. The digest depends only on the bytes fed in
// - not on how they were split up, nor on the host's byte order, pointer width
// or compiler - and matches the reference xxHash implementation, so anything
// else that speaks XXH64 computes the same value. Reading the digest does not
// consume the state.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ Xxh64 {
    u64 acc[4];              // the four accumulator lanes; acc[2] still holds the seed
    u64 ਕੁੱਲ_ਲੰਮਾਈ;         // input bytes seen so far
    u8  ਬਫਰ[32];          // input past the last stripe the accumulators took
    u32 ਬਫਰ_ਲੰਮਾਈ;
} Xxh64;

ਖਾਲੀ Xxh64_Init (Xxh64* o_state, u64 i_ਬੀਜ);
ਖਾਲੀ Xxh64_Update (Xxh64* io_state, ਅਚਲ ਖਾਲੀ* i_ਅੰਕੜੇ, size_t i_ਮਾਪ);
u64  Xxh64_Digest (ਅਚਲ Xxh64* i_state);
#ਜੇ_ਅੰਤ // d_m3HasSnapshots

cstr_t SPrint_ਮੁੱਲ (ਖਾਲੀ* i_ਮੁੱਲ, m3type_t i_type);
size_t SPrintArg (ਅੱਖਰ* o_string, size_t i_string_ਬਫਰ_ਮਾਪ, voidptr_t i_sp, m3type_t i_type);

ਖਾਲੀ   ReportError (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_error_ਸੁਨੇਹਾ, ccstr_t i_file, u32 i_ਸਤਰ_ਅੰਕ);

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
ਖਾਲੀ ਧੱਕੋ_Backtrace_ਚੌਖਟਾ (IM3Runtime io_runtime, pc_t i_pc);
ਖਾਲੀ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function);
ਖਾਲੀ ClearBacktrace (IM3Runtime io_runtime);
#ਜੇ_ਅੰਤ

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_core_h
