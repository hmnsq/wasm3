//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_core_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_core_h

#ಅನ್ತರ್ಭಾವ <stdio.h>
#ಅನ್ತರ್ಭಾವ <stdlib.h>
#ಅನ್ತರ್ಭಾವ <stdbool.h>
#ಅನ್ತರ್ಭಾವ <string.h>
#ಅನ್ತರ್ಭಾವ <assert.h>

#ಅನ್ತರ್ಭಾವ "wasm3.h"
#ಅನ್ತರ್ಭಾವ "m3_config.h"

#ಆದರೆ defined(__cplusplus)
#  define d_m3BeginExternC     ಬಾಹ್ಯ "C" {
#  define d_m3_ಕೊನೆ_Extern_C       }
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3BeginExternC
#  define d_m3_ಕೊನೆ_Extern_C
#ಆದರೆ_ಅಂತ್ಯ

d_m3BeginExternC

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3ImplementFloat (d_m3HasFloat || d_m3NoFloatDynamic)

#ಆದರೆ !defined(d_m3ShortTypesDefined)

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ uint64_t u64;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ int64_t  i64;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ uint32_t u32;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ int32_t  i32;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ uint16_t u16;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ int16_t  i16;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ uint8_t  u8;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ int8_t   i8;

#  ಆದರೆ d_m3ImplementFloat
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ದ್ವಿಗುಣ f64;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ದಶಮಾಂಶ  f32;
#  endif

#ಆದರೆ_ಅಂತ್ಯ // d_m3ShortTypesDefined

#ವ್ಯಾಖ್ಯಾನಿಸು PRIf32          "f"
#ವ್ಯಾಖ್ಯಾನಿಸು PRIf64          "lf"

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*       m3ret_t;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*       voidptr_t;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*       cstr_t;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ccstr_t;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸ್ಥಿರಾಂಕ u8*         ಬೈಟುಗಳು_t;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸ್ಥಿರಾಂಕ u8* ಸ್ಥಿರಾಂಕ   cbytes_t;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ u16               m3opcode_t;
#ಆದರೆ d_m3HasTypedRefs
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ u16 m3type_t;       // a value type, see the d_m3Type_* bits below
#ಇಲ್ಲದಿದ್ದರೆ
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ u8 m3type_t;       // only plain M3ValueType values exist
#ಆದರೆ_ಅಂತ್ಯ

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ i64 m3reg_t;

#ಆದರೆ d_m3_Use32_ಬಿಟ್_Slots
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ u32 m3slot_t;
#ಇಲ್ಲದಿದ್ದರೆ
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ u64 m3slot_t;
#ಆದರೆ_ಅಂತ್ಯ

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ m3slot_t* m3stack_t;

#ವ್ಯಾಖ್ಯಾನಿಸು c_io_ಸ್ಥಾನ_ಎಣಿಕೆ ((u16)(ಗಾತ್ರ(u64) / ಗಾತ್ರ(m3slot_t)))

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* ಸ್ಥಿರಾಂಕ cvptr_t;

#ಆದರೆ defined(DEBUG)

#  define d_m3Log(CATEGORY, FMT, ...)                ಬರೆ (" %8s  |  " FMT, #CATEGORY, ##__VA_ARGS__);

#  ಆದರೆ d_m3LogParse
#    define m3log_parse(CATEGORY, FMT, ...)          d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define m3log_parse(...) {}
#  endif

#  ಆದರೆ d_m3LogCompile
#    define m3log_compile(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define m3log_compile(...) {}
#  endif

#  ಆದರೆ d_m3LogEmit
#    define m3log_emit(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define m3log_emit(...) {}
#  endif

#  ಆದರೆ d_m3_Log_ಸಂಕೇತ_ಪುಟಗಳು
#    define m3log_ಸಂಕೇತ(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define m3log_ಸಂಕೇತ(...) {}
#  endif

#  ಆದರೆ d_m3LogModule
#    define m3log_module(CATEGORY, FMT, ...)         d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define m3log_module(...) {}
#  endif

#  ಆದರೆ d_m3LogRuntime
#    define m3log_runtime(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ಇಲ್ಲದಿದ್ದರೆ
#    define m3log_runtime(...) {}
#  endif

#  define m3log(CATEGORY, FMT, ...)                  m3log_##CATEGORY (CATEGORY, FMT "\n", ##__VA_ARGS__)
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3Log(CATEGORY, FMT, ...)                {}
#  define m3log(CATEGORY, FMT, ...)                  {}
#ಆದರೆ_ಅಂತ್ಯ


#ಆದರೆ defined(ASSERTS) || (defined(DEBUG) && !defined(NASSERTS))
#  define d_m3Assert(ASS)  ಆದರೆ (!(ASS)) { ಬರೆ("Assertion failed at %s:%d : %s\n", __FILE__, __LINE__, #ASS); fflush(stdout); abort(); }
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3Assert(ASS)
#ಆದರೆ_ಅಂತ್ಯ

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಶೂನ್ಯ /*const*/*                ಸಂಕೇತ_t;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಸಂಕೇತ_t ಸ್ಥಿರಾಂಕ* /*__restrict__*/ pc_t;


// Sits immediately before the linear memory it describes, so m3MemData() is
// just (header + 1). The interpreter carries a pointer to one of these in the
// _mem pseudo-register, and reaches everything else about the memory - its page
// counts, its owning runtime - through the back-pointers here. 'memory' is what
// makes that possible without going through the runtime, which is the
// prerequisite for a runtime holding more than one memory.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ {
    IM3Runtime       runtime;
    ಶೂನ್ಯ*            max_ರಾಶಿ;
    size_t           ಉದ್ದಳತೆ;
    ರಚನೆ M3_ಸ್ಮರಣೆ* ಸ್ಮರಣೆ;         // the M3Memory this block belongs to
} M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ;

ರಚನೆ M3_ಸಂಕೇತ_Mapping_ಪುಟ;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಸಂಕೇತ_ಪುಟ_ಶೀರ್ಷಿಕೆ {
    ರಚನೆ M3_ಸಂಕೇತ_ಪುಟ* ಮುಂದಿನ;

    u32                ಸಾಲು_ಸೂಚ್ಯಂಕ;
    u32                ಅಂಕೆ_ಸಾಲುಗಳು;
    u32                sequence;       // this is just used for debugging; could be removed
    u32                usage_ಎಣಿಕೆ;

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
    ರಚನೆ M3_ಸಂಕೇತ_Mapping_ಪುಟ* mapping;
#ಆದರೆ_ಅಂತ್ಯ // d_m3RecordBacktraces
} M3_ಸಂಕೇತ_ಪುಟ_ಶೀರ್ಷಿಕೆ;


#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ಸಂಕೇತ_ಪುಟ_Free_ಸಾಲುಗಳು_ಮಿತಿ      7+2       // max is: ReturnCallIndirect (op + 6 immediates) + 2 for bridge

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Default_Mem_ಪುಟ_ಗಾತ್ರ              65536

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
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ_ಬಿಟ್ಟುಗಳು                52
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ                    (UINT64_C(1) << d_m3_ವಿಳಾಸ_ಎಲ್ಲೆ_ಬಿಟ್ಟುಗಳು)

#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Reg0_ಸ್ಥಾನ_Alias                   60000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Fp0_ಸ್ಥಾನ_Alias                    (d_m3_Reg0_ಸ್ಥಾನ_Alias + 2)

#ಆದರೆ d_m3HasTypedRefs
// a heap type index has to fit in the spare bits of an m3type_t, see below
#  define d_m3_Max_Sane_Types_ಎಣಿಕೆ             4094
#ಇಲ್ಲದಿದ್ದರೆ
// M3Environment.numFuncTypes is the u16 that hands out M3FuncType.canonicalIndex,
// so the count has to stay below 65536 here as well
#  define d_m3_Max_Sane_Types_ಎಣಿಕೆ             65500
#ಆದರೆ_ಅಂತ್ಯ
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_Functions_ಎಣಿಕೆ           1000000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_Imports_ಎಣಿಕೆ             100000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_Exports_ಎಣಿಕೆ             100000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_Globals_ಎಣಿಕೆ             1000000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_ಟ್ಯಾಗುಗಳು_ಎಣಿಕೆ                100000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3MaxSaneElementSegments          10000000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_ದತ್ತಾಂಶ_Segments             100000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಗಾತ್ರ                10000000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_Memories_ಎಣಿಕೆ            10000
#ಆದರೆ d_m3HasRefTypes
#  define d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಎಣಿಕೆ             1000
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಎಣಿಕೆ             1           // MVP: a single table
#ಆದರೆ_ಅಂತ್ಯ
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_Utf8_ಉದ್ದಳತೆ               10000
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Max_Sane_Function_Arg_Ret_ಎಣಿಕೆ      1000    // still insane, but whatever

#ವ್ಯಾಖ್ಯಾನಿಸು d_external_ಬಗೆ_function             0
#ವ್ಯಾಖ್ಯಾನಿಸು d_external_ಬಗೆ_ಕೋಷ್ಟಕ                1
#ವ್ಯಾಖ್ಯಾನಿಸು d_external_ಬಗೆ_ಸ್ಮರಣೆ               2
#ವ್ಯಾಖ್ಯಾನಿಸು d_external_ಬಗೆ_global               3
#ವ್ಯಾಖ್ಯಾನಿಸು d_external_ಬಗೆ_ಟ್ಯಾಗ್                  4

// compact-import-section: these stand where an externtype would, after an empty
// item name, and introduce a run of imports sharing one module name. 0x7f gives
// each item its own externtype, 0x7e shares a single one across the whole run.
#ವ್ಯಾಖ್ಯಾನಿಸು d_compact_Imports_ಪ್ರತಿ_ಅಂಶ_Type        0x7f
#ವ್ಯಾಖ್ಯಾನಿಸು d_compactImports_sharedType         0x7e

// the negated wasm encodings of the abstract reference types, as NormalizeType()
// stores them
#ವ್ಯಾಖ್ಯಾನಿಸು d_waType_funcref                    16
#ವ್ಯಾಖ್ಯಾನಿಸು d_waType_externref                  17

// exception handling: exnref is wasm-encoded as -0x17
#ವ್ಯಾಖ್ಯಾನಿಸು d_waType_exnref                     23

// stack switching: cont type form is -0x23 (35); heap types: cont is -0x18 (24), nocont is -0x0b (11)
#ವ್ಯಾಖ್ಯಾನಿಸು d_waType_cont                       35
#ವ್ಯಾಖ್ಯಾನಿಸು d_wa_Type_ರಾಶಿ_cont                  24
#ವ್ಯಾಖ್ಯಾನಿಸು d_wa_Type_ರಾಶಿ_nocont                11

// function-references: a reference type spelled out as (ref ht) / (ref null ht),
// where the abstract heap types reuse the funcref and externref encodings
#ವ್ಯಾಖ್ಯಾನಿಸು d_waEncode_ref                      0x64
#ವ್ಯಾಖ್ಯಾನಿಸು d_waEncode_refNull                  0x63

// indexed by M3ValueType: c_m3Type_count entries for the concrete types, plus
// one more for c_m3Type_unknown, which sits right after them
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ c_waTypes[] = { "nil", "i32", "i64", "f32", "f64", "v128", "funcref", "externref", "exnref", "contref", "unknown" };
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ c_waCompactTypes[] = { "_", "i", "I", "f", "F", "V", "r", "R", "e", "c", "?" };


#ಆದರೆ d_m3VerboseErrorMessages

M3_ಫಲಿತಾಂಶ m3Error (M3_ಫಲಿತಾಂಶ i_ಫಲಿತಾಂಶ, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_file, u32 i_ಸಾಲು_ಅಂಕೆ, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ i_error_ಸಂದೇಶ, ...);

#  define _m3Error(ಫಲಿತಾಂಶ_2, RT, MOD, FUN, FILE, ಸಾಲು, FORMAT, ...) \
            m3Error (ಫಲಿತಾಂಶ_2, RT, MOD, FUN, FILE, ಸಾಲು, FORMAT, ##__VA_ARGS__)

#ಇಲ್ಲದಿದ್ದರೆ
#  define _m3Error(ಫಲಿತಾಂಶ_2, RT, MOD, FUN, FILE, ಸಾಲು, FORMAT, ...) (ಫಲಿತಾಂಶ_2)
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು ErrorRuntime(ಫಲಿತಾಂಶ_2, RUNTIME, FORMAT, ...)      _m3Error (ಫಲಿತಾಂಶ_2, RUNTIME, ಖಾಲಿ, ಖಾಲಿ,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು ErrorModule(ಫಲಿತಾಂಶ_2, MOD, FORMAT, ...)           _m3Error (ಫಲಿತಾಂಶ_2, MOD->runtime, MOD, ಖಾಲಿ,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು ErrorCompile(ಫಲಿತಾಂಶ_2, COMP, FORMAT, ...)         _m3Error (ಫಲಿತಾಂಶ_2, COMP->runtime, COMP->module, ಖಾಲಿ, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)

#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ

// How much address space a linear memory's bytes are given: everything a Wasm access
// can name. A 32-bit address plus a 32-bit memarg offset reaches just under 2^33, and
// the access itself a few bytes further, which the page of slack in a slot covers.
#  define d_m3_Guarded_ದತ್ತಾಂಶ_ಬೈಟುಗಳು  (UINT64_C(1) << 33)

// Take one slot of the guarded arena, or NULL when the arena has none left or could
// not be reserved at all. What comes back is the base of the slot: the memory's
// M3MemoryHeader sits at the end of its first page, and the bytes the guest addresses
// begin on the page after that.
ಶೂನ್ಯ*           Guard_Take_ಸ್ಥಾನ (ಶೂನ್ಯ);

// The header of a slot, which is also what the guest's memory starts right after
M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* Guard_ಸ್ಥಾನ_ಶೀರ್ಷಿಕೆ (ಶೂನ್ಯ* i_ಸ್ಥಾನ);

// Make the first i_dataBytes of the slot's data reachable. Only ever asked to grow,
// and asking for what is already there costs nothing.
bool            Guard_Commit_ಸ್ಥಾನ (ಶೂನ್ಯ* i_ಸ್ಥಾನ, size_t i_ದತ್ತಾಂಶ_ಬೈಟುಗಳು);

// Give a slot back, dropping everything committed in it. The next taker gets it
// zeroed, as a fresh linear memory has to be.
ಶೂನ್ಯ            Guard_Give_ಸ್ಥಾನ (ಶೂನ್ಯ* i_ಸ್ಥಾನ);

// The whole arena, for a fault to be held against - see m3_HostProtectedCall. Answers
// (NULL, 0) before anything has asked for a slot.
ಶೂನ್ಯ            Guard_ArenaRange (ಶೂನ್ಯ** o_ಕೆಳಗಿನ, size_t* o_ಬೈಟುಗಳು);

#ಆದರೆ_ಅಂತ್ಯ // d_m3GuardedMemory

#ಆದರೆ d_m3_Max_Native_ರಾಶಿ > 0
// The native-stack low-water mark for a top-level call that came in at i_stackPtr,
// clamped to what the calling thread's stack really is where that can be measured.
// See d_m3MaxNativeStack and d_m3StackLimitEnter.
ಶೂನ್ಯ* m3_Native_ರಾಶಿ_ಎಲ್ಲೆ (ಶೂನ್ಯ* i_ರಾಶಿ_Ptr, size_t i_budget);
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_Log_Native_ರಾಶಿ
ಶೂನ್ಯ m3_ರಾಶಿ_Check_Init ();
ಶೂನ್ಯ m3_ರಾಶಿ_Check ();
ಪೂರ್ಣಾಂಕ  m3_ರಾಶಿ_Get_Max ();
#ಇಲ್ಲದಿದ್ದರೆ
#  define     m3_ರಾಶಿ_Check_Init()
#  define     m3_ರಾಶಿ_Check()
#  define     m3_ರಾಶಿ_Get_Max()         0
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3LogTimestamps
#  define     PRIts                   "%llu"
uint64_t m3_GetTimestamp ();
#ಇಲ್ಲದಿದ್ದರೆ
#  define     PRIts                   "%s"
#  define     m3_GetTimestamp()       ""
#ಆದರೆ_ಅಂತ್ಯ

ಶೂನ್ಯ  m3_Abort (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸಂದೇಶ);
ಶೂನ್ಯ* m3_Malloc_Impl (size_t i_ಗಾತ್ರ);
ಶೂನ್ಯ* m3_Realloc_Impl (ಶೂನ್ಯ* i_ptr, size_t i_new_ಗಾತ್ರ, size_t i_old_ಗಾತ್ರ);
ಶೂನ್ಯ  m3_Free_Impl (ಶೂನ್ಯ* i_ptr);
ಶೂನ್ಯ* m3_CopyMem (ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_from, size_t i_ಗಾತ್ರ);

#ಆದರೆ d_m3_Log_ರಾಶಿ_Ops

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ* m3_AllocStruct_Impl (ccstr_t name, size_t i_ಗಾತ್ರ)
{
    ಶೂನ್ಯ* ಫಲಿತಾಂಶ = m3_Malloc_Impl(i_ಗಾತ್ರ);
    fprintf(stderr, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_ಗಾತ್ರ, ಫಲಿತಾಂಶ);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ* m3_AllocArray_Impl (ccstr_t name, size_t i_ಅಂಕೆ, size_t i_ಗಾತ್ರ)
{
    ಶೂನ್ಯ* ಫಲಿತಾಂಶ = m3_Malloc_Impl(i_ಗಾತ್ರ * i_ಅಂಕೆ);
    fprintf(stderr, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_ಗಾತ್ರ, i_ಅಂಕೆ, ಫಲಿತಾಂಶ);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ* m3_ReallocArray_Impl (ccstr_t name, ಶೂನ್ಯ* i_ptr_old, size_t i_ಅಂಕೆ_new, size_t i_ಅಂಕೆ_old, size_t i_ಗಾತ್ರ)
{
    ಶೂನ್ಯ* ಫಲಿತಾಂಶ = m3_Realloc_Impl(i_ptr_old, i_ಗಾತ್ರ * i_ಅಂಕೆ_new, i_ಗಾತ್ರ * i_ಅಂಕೆ_old);
    fprintf(stderr, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_ಗಾತ್ರ, i_ಅಂಕೆ_new, ಫಲಿತಾಂಶ, i_ಅಂಕೆ_old, i_ptr_old);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ* m3_Malloc (ccstr_t name, size_t i_ಗಾತ್ರ)
{
    ಶೂನ್ಯ* ಫಲಿತಾಂಶ = m3_Malloc_Impl(i_ಗಾತ್ರ);
    fprintf(stderr, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_ಗಾತ್ರ, ಫಲಿತಾಂಶ);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ* m3_Realloc (ccstr_t name, ಶೂನ್ಯ* i_ptr, size_t i_new_ಗಾತ್ರ, size_t i_old_ಗಾತ್ರ)
{
    ಶೂನ್ಯ* ಫಲಿತಾಂಶ = m3_Realloc_Impl(i_ptr, i_new_ಗಾತ್ರ, i_old_ಗಾತ್ರ);
    fprintf(stderr, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_new_ಗಾತ್ರ, ಫಲಿತಾಂಶ, i_old_ಗಾತ್ರ, i_ptr);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, ಗಾತ್ರ (STRUCT))
#  define     m3_AllocArray(STRUCT, ಅಂಕೆ)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, ಅಂಕೆ, ಗಾತ್ರ (STRUCT))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (ಶೂನ್ಯ *)(PTR), (NEW), (OLD), ಗಾತ್ರ (STRUCT))
#  define     m3_Free(P)                              ಮಾಡು { ಶೂನ್ಯ* p = (ಶೂನ್ಯ*)(P);                                  \
                                                          ಆದರೆ (p) { fprintf(stderr, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                          m3_Free_Impl (p); (P) = ಖಾಲಿ; } ಆಗ(0)
#ಇಲ್ಲದಿದ್ದರೆ
#  define     m3_Malloc(NAME, ಗಾತ್ರ_3)                   m3_Malloc_Impl(ಗಾತ್ರ_3)
#  define     m3_Realloc(NAME, PTR, NEW, OLD)         m3_Realloc_Impl(PTR, NEW, OLD)
#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_Malloc_Impl (ಗಾತ್ರ (STRUCT))
#  define     m3_AllocArray(STRUCT, ಅಂಕೆ)              (STRUCT *)m3_Malloc_Impl (ಗಾತ್ರ (STRUCT) * (ಅಂಕೆ))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_Realloc_Impl ((ಶೂನ್ಯ *)(PTR), ಗಾತ್ರ (STRUCT) * (NEW), ಗಾತ್ರ (STRUCT) * (OLD))
#  define     m3_Free(P)                              ಮಾಡು { m3_Free_Impl ((ಶೂನ್ಯ*)(P)); (P) = ಖಾಲಿ; } ಆಗ(0)
#ಆದರೆ_ಅಂತ್ಯ

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

#ಆದರೆ d_m3HasTypedRefs

#  define d_m3Type_ref                0x8000u
#  define d_m3Type_refNonNull         0x4000u
#  define d_m3Type_refExtern          0x2000u
#  define d_m3Type_refCont            0x1000u
#  define d_m3_Type_ರಾಶಿ_ಮುಖವಾಡ           0x0FFFu
#  define d_m3_Type_ರಾಶಿ_Abstract       0x0FFFu     // plain 'func', 'extern', or 'cont'
// 'nocont', the bottom of the continuation types: a reference to it is a
// reference to every one of them, which is what makes ref.null nocont usable
// wherever a particular continuation is expected. One below heapAbstract, and
// so out of reach of d_m3MaxSaneTypesCount.
#  define d_m3_Type_ರಾಶಿ_None           0x0FFEu

// The storage type a value of this type occupies: a reference of any shape is
// just a funcref or an externref once the type checking is done with.
u8   ತಳ_Type_ದ (m3type_t i_type);
bool Is_Sub_Type_ದ (m3type_t i_sub, m3type_t i_super);

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool IsSpelledRefType (m3type_t i_type)
{
    ಹಿಂತಿರುಗಿ (i_type & d_m3Type_ref) != 0;
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool IsNullableRef (m3type_t i_type)
{
    ಹಿಂತಿರುಗಿ (i_type & d_m3Type_refNonNull) == 0;
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u32 ರಾಶಿ_Type_ದ (m3type_t i_type)
{
    ಹಿಂತಿರುಗಿ IsSpelledRefType(i_type) ? (i_type & d_m3_Type_ರಾಶಿ_ಮುಖವಾಡ) : d_m3_Type_ರಾಶಿ_Abstract;
}

#ಇಲ್ಲದಿದ್ದರೆ // no spelled-out reference types, so every value type is its own storage
      // type and subtyping collapses back to equality

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u8 ತಳ_Type_ದ (m3type_t i_type)
{
    ಹಿಂತಿರುಗಿ (u8)i_type;
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Is_Sub_Type_ದ (m3type_t i_sub, m3type_t i_super)
{
    ಹಿಂತಿರುಗಿ i_sub == i_super;
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool IsSpelledRefType (m3type_t i_type)
{
    (ಶೂನ್ಯ)i_type;
    ಹಿಂತಿರುಗಿ false;
}
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool IsNullableRef (m3type_t i_type)
{
    (ಶೂನ್ಯ)i_type;
    ಹಿಂತಿರುಗಿ true;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasTypedRefs

M3_ಫಲಿತಾಂಶ NormalizeType (u8* o_type, i8 i_convolutedWasmType);

// These all speak in terms of how a value is stored, so they reduce a
// spelled-out reference type to its funcref / externref base first.
bool     IsIntType (m3type_t i_wasmType);
bool     IsFpType (m3type_t i_wasmType);
bool     IsRefType (m3type_t i_m3Type);
bool     Is64_ಬಿಟ್_Type (m3type_t i_m3Type);
u32      ಗಾತ್ರ_ದ_Type (m3type_t i_m3Type);

M3_ಫಲಿತಾಂಶ Read_u64 (u64* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ Read_u32 (u32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
#ಆದರೆ d_m3ImplementFloat
M3_ಫಲಿತಾಂಶ Read_f64 (f64* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ Read_f32 (f32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
#ಆದರೆ_ಅಂತ್ಯ
M3_ಫಲಿತಾಂಶ Read_u8 (u8* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ Read_opcode (m3opcode_t* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);

M3_ಫಲಿತಾಂಶ ReadLebUnsigned (u64* o_ಮೌಲ್ಯ, u32 i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ ReadLebSigned (i64* o_ಮೌಲ್ಯ, u32 i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ ReadLEB_u32 (u32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);

// The memarg immediate of a load or store: an alignment hint, then a static
// offset. Multi-memory reinterprets the alignment as a bitfield -- bit 6 says a
// memory index sits between the two -- so the hint is returned with that bit
// masked off and the index defaults to 0 when it is clear.
M3_ಫಲಿತಾಂಶ Read_ಸ್ಮರಣೆ_Arg (u32* o_align, u32* o_ಸ್ಮರಣೆ_Idx, u64* o_ಸ್ಥಾನಾಂತರ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ ReadLEB_u7 (u8* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ ReadLEB_i7 (i8* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ ReadLEB_i32 (i32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ ReadLEB_i64 (i64* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
M3_ಫಲಿತಾಂಶ Read_utf8 (cstr_t* o_utf8, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);

#ಆದರೆ d_m3HasSnapshots
// XXH64, in m3_xxh64.c. Initialize a state, feed it the input in any number of
// pieces, then read the digest out. The digest depends only on the bytes fed in
// - not on how they were split up, nor on the host's byte order, pointer width
// or compiler - and matches the reference xxHash implementation, so anything
// else that speaks XXH64 computes the same value. Reading the digest does not
// consume the state.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ Xxh64 {
    u64 acc[4];              // the four accumulator lanes; acc[2] still holds the seed
    u64 ಒಟ್ಟು_ಉದ್ದಳತೆ;         // input bytes seen so far
    u8  ಬಫರ್[32];          // input past the last stripe the accumulators took
    u32 ಬಫರ್_ಉದ್ದಳತೆ;
} Xxh64;

ಶೂನ್ಯ Xxh64_Init (Xxh64* o_state, u64 i_ಬೀಜ);
ಶೂನ್ಯ Xxh64_Update (Xxh64* io_state, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ದತ್ತಾಂಶ, size_t i_ಗಾತ್ರ);
u64  Xxh64_Digest (ಸ್ಥಿರಾಂಕ Xxh64* i_state);
#ಆದರೆ_ಅಂತ್ಯ // d_m3HasSnapshots

cstr_t SPrint_ಮೌಲ್ಯ (ಶೂನ್ಯ* i_ಮೌಲ್ಯ, m3type_t i_type);
size_t SPrintArg (ಅಕ್ಷರ* o_string, size_t i_string_ಬಫರ್_ಗಾತ್ರ, voidptr_t i_sp, m3type_t i_type);

ಶೂನ್ಯ   ReportError (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_error_ಸಂದೇಶ, ccstr_t i_file, u32 i_ಸಾಲು_ಅಂಕೆ);

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
ಶೂನ್ಯ ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು (IM3Runtime io_runtime, pc_t i_pc);
ಶೂನ್ಯ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function);
ಶೂನ್ಯ ClearBacktrace (IM3Runtime io_runtime);
#ಆದರೆ_ಅಂತ್ಯ

d_m3_ಕೊನೆ_Extern_C

#ಆದರೆ_ಅಂತ್ಯ // m3_core_h
