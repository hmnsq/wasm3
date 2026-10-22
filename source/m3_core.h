//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_core_h
#define m3_core_h

#અન્તર્ભાવ <stdio.h>
#અન્તર્ભાવ <stdlib.h>
#અન્તર્ભાવ <stdbool.h>
#અન્તર્ભાવ <string.h>
#અન્તર્ભાવ <assert.h>

#અન્તર્ભાવ "wasm3.h"
#અન્તર્ભાવ "m3_config.h"

#જો defined(__cplusplus)
#  define d_m3BeginExternC     બાહ્ય "C" {
#  define d_m3EndExternC       }
#નહીંતર
#  define d_m3BeginExternC
#  define d_m3EndExternC
#endif

d_m3BeginExternC

#define d_m3ImplementFloat (d_m3HasFloat || d_m3NoFloatDynamic)

#જો !defined(d_m3ShortTypesDefined)

પ્રકારવ્યાખ્યા uint64_t u64;
પ્રકારવ્યાખ્યા int64_t  i64;
પ્રકારવ્યાખ્યા uint32_t u32;
પ્રકારવ્યાખ્યા int32_t  i32;
પ્રકારવ્યાખ્યા uint16_t u16;
પ્રકારવ્યાખ્યા int16_t  i16;
પ્રકારવ્યાખ્યા uint8_t  u8;
પ્રકારવ્યાખ્યા int8_t   i8;

#  જો d_m3ImplementFloat
પ્રકારવ્યાખ્યા દ્વિગુણ f64;
પ્રકારવ્યાખ્યા દશાંશ  f32;
#  endif

#endif // d_m3ShortTypesDefined

#define PRIf32          "f"
#define PRIf64          "lf"

પ્રકારવ્યાખ્યા અચલ શૂન્ય*       m3ret_t;
પ્રકારવ્યાખ્યા અચલ શૂન્ય*       voidptr_t;
પ્રકારવ્યાખ્યા અચલ અક્ષર*       cstr_t;
પ્રકારવ્યાખ્યા અચલ અક્ષર* અચલ ccstr_t;
પ્રકારવ્યાખ્યા અચલ u8*         bytes_t;
પ્રકારવ્યાખ્યા અચલ u8* અચલ   cbytes_t;

પ્રકારવ્યાખ્યા u16               m3opcode_t;
#જો d_m3HasTypedRefs
પ્રકારવ્યાખ્યા u16 m3type_t;       // a value type, see the d_m3Type_* bits below
#નહીંતર
પ્રકારવ્યાખ્યા u8 m3type_t;       // only plain M3ValueType values exist
#endif

પ્રકારવ્યાખ્યા i64 m3reg_t;

#જો d_m3Use32BitSlots
પ્રકારવ્યાખ્યા u32 m3slot_t;
#નહીંતર
પ્રકારવ્યાખ્યા u64 m3slot_t;
#endif

પ્રકારવ્યાખ્યા m3slot_t* m3stack_t;

#define c_io_Slot_ગણતરી ((u16)(કદ(u64) / કદ(m3slot_t)))

પ્રકારવ્યાખ્યા અચલ શૂન્ય* અચલ cvptr_t;

#જો defined(DEBUG)

#  define d_m3Log(CATEGORY, FMT, ...)                printf (" %8s  |  " FMT, #CATEGORY, ##__VA_ARGS__);

#  જો d_m3LogParse
#    define m3log_parse(CATEGORY, FMT, ...)          d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  નહીંતર
#    define m3log_parse(...) {}
#  endif

#  જો d_m3LogCompile
#    define m3log_compile(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  નહીંતર
#    define m3log_compile(...) {}
#  endif

#  જો d_m3LogEmit
#    define m3log_emit(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  નહીંતર
#    define m3log_emit(...) {}
#  endif

#  જો d_m3LogCodePages
#    define m3log_code(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  નહીંતર
#    define m3log_code(...) {}
#  endif

#  જો d_m3LogModule
#    define m3log_module(CATEGORY, FMT, ...)         d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  નહીંતર
#    define m3log_module(...) {}
#  endif

#  જો d_m3LogRuntime
#    define m3log_runtime(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  નહીંતર
#    define m3log_runtime(...) {}
#  endif

#  define m3log(CATEGORY, FMT, ...)                  m3log_##CATEGORY (CATEGORY, FMT "\n", ##__VA_ARGS__)
#નહીંતર
#  define d_m3Log(CATEGORY, FMT, ...)                {}
#  define m3log(CATEGORY, FMT, ...)                  {}
#endif


#જો defined(ASSERTS) || (defined(DEBUG) && !defined(NASSERTS))
#  define d_m3Assert(ASS)  જો (!(ASS)) { printf("Assertion failed at %s:%d : %s\n", __FILE__, __LINE__, #ASS); fflush(stdout); abort(); }
#નહીંતર
#  define d_m3Assert(ASS)
#endif

પ્રકારવ્યાખ્યા શૂન્ય /*const*/*                code_t;
પ્રકારવ્યાખ્યા code_t અચલ* /*__restrict__*/ pc_t;


// Sits immediately before the linear memory it describes, so m3MemData() is
// just (header + 1). The interpreter carries a pointer to one of these in the
// _mem pseudo-register, and reaches everything else about the memory - its page
// counts, its owning runtime - through the back-pointers here. 'memory' is what
// makes that possible without going through the runtime, which is the
// prerequisite for a runtime holding more than one memory.
પ્રકારવ્યાખ્યા સંરચના M3MemoryHeader {
    IM3Runtime       runtime;
    શૂન્ય*            max_ઢગલો;
    size_t           length;
    સંરચના M3Memory* memory;         // the M3Memory this block belongs to
} M3MemoryHeader;

સંરચના M3CodeMappingPage;

પ્રકારવ્યાખ્યા સંરચના M3CodePageHeader {
    સંરચના M3CodePage* next;

    u32                લીટી_Index;
    u32                અંક_લીટીઓ;
    u32                sequence;       // this is just used for debugging; could be removed
    u32                usage_ગણતરી;

#જો d_m3RecordBacktraces
    સંરચના M3CodeMappingPage* mapping;
#endif // d_m3RecordBacktraces
} M3CodePageHeader;


#define d_m3_Code_Page_Free_લીટીઓ_Threshold      7+2       // max is: ReturnCallIndirect (op + 6 immediates) + 2 for bridge

#define d_m3DefaultMemPageSize              65536

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
#define d_m3AddressLimitBits                52
#define d_m3AddressLimit                    (UINT64_C(1) << d_m3AddressLimitBits)

#define d_m3Reg0SlotAlias                   60000
#define d_m3Fp0SlotAlias                    (d_m3Reg0SlotAlias + 2)

#જો d_m3HasTypedRefs
// a heap type index has to fit in the spare bits of an m3type_t, see below
#  define d_m3_Max_Sane_Types_ગણતરી             4094
#નહીંતર
// M3Environment.numFuncTypes is the u16 that hands out M3FuncType.canonicalIndex,
// so the count has to stay below 65536 here as well
#  define d_m3_Max_Sane_Types_ગણતરી             65500
#endif
#define d_m3_Max_Sane_Functions_ગણતરી           1000000
#define d_m3_Max_Sane_Imports_ગણતરી             100000
#define d_m3_Max_Sane_Exports_ગણતરી             100000
#define d_m3_Max_Sane_Globals_ગણતરી             1000000
#define d_m3_Max_Sane_Tags_ગણતરી                100000
#define d_m3MaxSaneElementSegments          10000000
#define d_m3MaxSaneDataSegments             100000
#define d_m3MaxSaneTableSize                10000000
#define d_m3_Max_Sane_Memories_ગણતરી            10000
#જો d_m3HasRefTypes
#  define d_m3_Max_Sane_Table_ગણતરી             1000
#નહીંતર
#  define d_m3_Max_Sane_Table_ગણતરી             1           // MVP: a single table
#endif
#define d_m3MaxSaneUtf8Length               10000
#define d_m3_Max_Sane_Function_Arg_Ret_ગણતરી      1000    // still insane, but whatever

#define d_externalKind_function             0
#define d_externalKind_table                1
#define d_externalKind_memory               2
#define d_externalKind_global               3
#define d_externalKind_tag                  4

// compact-import-section: these stand where an externtype would, after an empty
// item name, and introduce a run of imports sharing one module name. 0x7f gives
// each item its own externtype, 0x7e shares a single one across the whole run.
#define d_compactImports_perItemType        0x7f
#define d_compactImports_sharedType         0x7e

// the negated wasm encodings of the abstract reference types, as NormalizeType()
// stores them
#define d_waType_funcref                    16
#define d_waType_externref                  17

// exception handling: exnref is wasm-encoded as -0x17
#define d_waType_exnref                     23

// stack switching: cont type form is -0x23 (35); heap types: cont is -0x18 (24), nocont is -0x0b (11)
#define d_waType_cont                       35
#define d_waType_heap_cont                  24
#define d_waType_heap_nocont                11

// function-references: a reference type spelled out as (ref ht) / (ref null ht),
// where the abstract heap types reuse the funcref and externref encodings
#define d_waEncode_ref                      0x64
#define d_waEncode_refNull                  0x63

// indexed by M3ValueType: c_m3Type_count entries for the concrete types, plus
// one more for c_m3Type_unknown, which sits right after them
સ્થિર અચલ અક્ષર* અચલ c_waTypes[] = { "nil", "i32", "i64", "f32", "f64", "v128", "funcref", "externref", "exnref", "contref", "unknown" };
સ્થિર અચલ અક્ષર* અચલ c_waCompactTypes[] = { "_", "i", "I", "f", "F", "V", "r", "R", "e", "c", "?" };


#જો d_m3VerboseErrorMessages

M3Result m3Error (M3Result i_result, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  અચલ અક્ષર* અચલ i_file, u32 i_લીટી_અંક, અચલ અક્ષર* અચલ i_errorMessage, ...);

#  define _m3Error(RESULT, RT, MOD, FUN, FILE, લીટી, FORMAT, ...) \
            m3Error (RESULT, RT, MOD, FUN, FILE, લીટી, FORMAT, ##__VA_ARGS__)

#નહીંતર
#  define _m3Error(RESULT, RT, MOD, FUN, FILE, લીટી, FORMAT, ...) (RESULT)
#endif

#define ErrorRuntime(RESULT, RUNTIME, FORMAT, ...)      _m3Error (RESULT, RUNTIME, ખાલી, ખાલી,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#define ErrorModule(RESULT, MOD, FORMAT, ...)           _m3Error (RESULT, MOD->runtime, MOD, ખાલી,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#define ErrorCompile(RESULT, COMP, FORMAT, ...)         _m3Error (RESULT, COMP->runtime, COMP->module, ખાલી, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)

#જો d_m3GuardedMemory

// How much address space a linear memory's bytes are given: everything a Wasm access
// can name. A 32-bit address plus a 32-bit memarg offset reaches just under 2^33, and
// the access itself a few bytes further, which the page of slack in a slot covers.
#  define d_m3GuardedDataBytes  (UINT64_C(1) << 33)

// Take one slot of the guarded arena, or NULL when the arena has none left or could
// not be reserved at all. What comes back is the base of the slot: the memory's
// M3MemoryHeader sits at the end of its first page, and the bytes the guest addresses
// begin on the page after that.
શૂન્ય*           Guard_TakeSlot (શૂન્ય);

// The header of a slot, which is also what the guest's memory starts right after
M3MemoryHeader* Guard_SlotHeader (શૂન્ય* i_slot);

// Make the first i_dataBytes of the slot's data reachable. Only ever asked to grow,
// and asking for what is already there costs nothing.
bool            Guard_CommitSlot (શૂન્ય* i_slot, size_t i_dataBytes);

// Give a slot back, dropping everything committed in it. The next taker gets it
// zeroed, as a fresh linear memory has to be.
શૂન્ય            Guard_GiveSlot (શૂન્ય* i_slot);

// The whole arena, for a fault to be held against - see m3_HostProtectedCall. Answers
// (NULL, 0) before anything has asked for a slot.
શૂન્ય            Guard_ArenaRange (શૂન્ય** o_નીચલું, size_t* o_bytes);

#endif // d_m3GuardedMemory

#જો d_m3_Max_Native_ઢગલો > 0
// The native-stack low-water mark for a top-level call that came in at i_stackPtr,
// clamped to what the calling thread's stack really is where that can be measured.
// See d_m3MaxNativeStack and d_m3StackLimitEnter.
શૂન્ય* m3_Native_ઢગલો_Limit (શૂન્ય* i_ઢગલો_Ptr, size_t i_budget);
#endif

#જો d_m3_Log_Native_ઢગલો
શૂન્ય m3_ઢગલો_Check_Init ();
શૂન્ય m3_ઢગલો_Check ();
પૂર્ણાંક  m3_ઢગલો_Get_Max ();
#નહીંતર
#  define     m3_ઢગલો_Check_Init()
#  define     m3_ઢગલો_Check()
#  define     m3_ઢગલો_Get_Max()         0
#endif

#જો d_m3LogTimestamps
#  define     PRIts                   "%llu"
uint64_t m3_GetTimestamp ();
#નહીંતર
#  define     PRIts                   "%s"
#  define     m3_GetTimestamp()       ""
#endif

શૂન્ય  m3_Abort (અચલ અક્ષર* message);
શૂન્ય* m3_Malloc_Impl (size_t i_size);
શૂન્ય* m3_Realloc_Impl (શૂન્ય* i_ptr, size_t i_newSize, size_t i_oldSize);
શૂન્ય  m3_Free_Impl (શૂન્ય* i_ptr);
શૂન્ય* m3_CopyMem (અચલ શૂન્ય* i_from, size_t i_size);

#જો d_m3LogHeapOps

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

સ્થિર અંતઃરેખ
શૂન્ય* m3_AllocStruct_Impl (ccstr_t name, size_t i_size)
{
    શૂન્ય* result = m3_Malloc_Impl(i_size);
    fprintf(stderr, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_size, result);
    પાછા result;
}

સ્થિર અંતઃરેખ
શૂન્ય* m3_AllocArray_Impl (ccstr_t name, size_t i_અંક, size_t i_size)
{
    શૂન્ય* result = m3_Malloc_Impl(i_size * i_અંક);
    fprintf(stderr, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_size, i_અંક, result);
    પાછા result;
}

સ્થિર અંતઃરેખ
શૂન્ય* m3_ReallocArray_Impl (ccstr_t name, શૂન્ય* i_ptr_old, size_t i_અંક_new, size_t i_અંક_old, size_t i_size)
{
    શૂન્ય* result = m3_Realloc_Impl(i_ptr_old, i_size * i_અંક_new, i_size * i_અંક_old);
    fprintf(stderr, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_size, i_અંક_new, result, i_અંક_old, i_ptr_old);
    પાછા result;
}

સ્થિર અંતઃરેખ
શૂન્ય* m3_Malloc (ccstr_t name, size_t i_size)
{
    શૂન્ય* result = m3_Malloc_Impl(i_size);
    fprintf(stderr, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_size, result);
    પાછા result;
}
સ્થિર અંતઃરેખ
શૂન્ય* m3_Realloc (ccstr_t name, શૂન્ય* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    શૂન્ય* result = m3_Realloc_Impl(i_ptr, i_newSize, i_oldSize);
    fprintf(stderr, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_newSize, result, i_oldSize, i_ptr);
    પાછા result;
}

#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, કદ (STRUCT))
#  define     m3_AllocArray(STRUCT, અંક)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, અંક, કદ (STRUCT))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (શૂન્ય *)(PTR), (NEW), (OLD), કદ (STRUCT))
#  define     m3_Free(P)                              કરો { શૂન્ય* p = (શૂન્ય*)(P);                                  \
                                                          જો (p) { fprintf(stderr, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                          m3_Free_Impl (p); (P) = ખાલી; } જ્યારે(0)
#નહીંતર
#  define     m3_Malloc(NAME, SIZE)                   m3_Malloc_Impl(SIZE)
#  define     m3_Realloc(NAME, PTR, NEW, OLD)         m3_Realloc_Impl(PTR, NEW, OLD)
#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_Malloc_Impl (કદ (STRUCT))
#  define     m3_AllocArray(STRUCT, અંક)              (STRUCT *)m3_Malloc_Impl (કદ (STRUCT) * (અંક))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_Realloc_Impl ((શૂન્ય *)(PTR), કદ (STRUCT) * (NEW), કદ (STRUCT) * (OLD))
#  define     m3_Free(P)                              કરો { m3_Free_Impl ((શૂન્ય*)(P)); (P) = ખાલી; } જ્યારે(0)
#endif

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

#જો d_m3HasTypedRefs

#  define d_m3Type_ref                0x8000u
#  define d_m3Type_refNonNull         0x4000u
#  define d_m3Type_refExtern          0x2000u
#  define d_m3Type_refCont            0x1000u
#  define d_m3Type_heapMask           0x0FFFu
#  define d_m3Type_heapAbstract       0x0FFFu     // plain 'func', 'extern', or 'cont'
// 'nocont', the bottom of the continuation types: a reference to it is a
// reference to every one of them, which is what makes ref.null nocont usable
// wherever a particular continuation is expected. One below heapAbstract, and
// so out of reach of d_m3MaxSaneTypesCount.
#  define d_m3Type_heapNone           0x0FFEu

// The storage type a value of this type occupies: a reference of any shape is
// just a funcref or an externref once the type checking is done with.
u8   BaseTypeOf (m3type_t i_type);
bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super);

સ્થિર અંતઃરેખ
bool IsSpelledRefType (m3type_t i_type)
{
    પાછા (i_type & d_m3Type_ref) != 0;
}
સ્થિર અંતઃરેખ
bool IsNullableRef (m3type_t i_type)
{
    પાછા (i_type & d_m3Type_refNonNull) == 0;
}
સ્થિર અંતઃરેખ
u32 HeapTypeOf (m3type_t i_type)
{
    પાછા IsSpelledRefType(i_type) ? (i_type & d_m3Type_heapMask) : d_m3Type_heapAbstract;
}

#નહીંતર // no spelled-out reference types, so every value type is its own storage
      // type and subtyping collapses back to equality

સ્થિર અંતઃરેખ
u8 BaseTypeOf (m3type_t i_type)
{
    પાછા (u8)i_type;
}
સ્થિર અંતઃરેખ
bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    પાછા i_sub == i_super;
}
સ્થિર અંતઃરેખ
bool IsSpelledRefType (m3type_t i_type)
{
    (શૂન્ય)i_type;
    પાછા false;
}
સ્થિર અંતઃરેખ
bool IsNullableRef (m3type_t i_type)
{
    (શૂન્ય)i_type;
    પાછા true;
}

#endif // d_m3HasTypedRefs

M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType);

// These all speak in terms of how a value is stored, so they reduce a
// spelled-out reference type to its funcref / externref base first.
bool     IsIntType (m3type_t i_wasmType);
bool     IsFpType (m3type_t i_wasmType);
bool     IsRefType (m3type_t i_m3Type);
bool     Is64BitType (m3type_t i_m3Type);
u32      SizeOfType (m3type_t i_m3Type);

M3Result Read_u64 (u64* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
M3Result Read_u32 (u32* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
#જો d_m3ImplementFloat
M3Result Read_f64 (f64* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
M3Result Read_f32 (f32* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
#endif
M3Result Read_u8 (u8* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
M3Result Read_opcode (m3opcode_t* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);

M3Result ReadLebUnsigned (u64* o_કિંમત, u32 i_max_અંક_Bits, bytes_t* io_bytes, cbytes_t i_end);
M3Result ReadLebSigned (i64* o_કિંમત, u32 i_max_અંક_Bits, bytes_t* io_bytes, cbytes_t i_end);
M3Result ReadLEB_u32 (u32* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);

// The memarg immediate of a load or store: an alignment hint, then a static
// offset. Multi-memory reinterprets the alignment as a bitfield -- bit 6 says a
// memory index sits between the two -- so the hint is returned with that bit
// masked off and the index defaults to 0 when it is clear.
M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end);
M3Result ReadLEB_u7 (u8* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
M3Result ReadLEB_i7 (i8* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
M3Result ReadLEB_i32 (i32* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
M3Result ReadLEB_i64 (i64* o_કિંમત, bytes_t* io_bytes, cbytes_t i_end);
M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end);

#જો d_m3HasSnapshots
// XXH64, in m3_xxh64.c. Initialize a state, feed it the input in any number of
// pieces, then read the digest out. The digest depends only on the bytes fed in
// - not on how they were split up, nor on the host's byte order, pointer width
// or compiler - and matches the reference xxHash implementation, so anything
// else that speaks XXH64 computes the same value. Reading the digest does not
// consume the state.
પ્રકારવ્યાખ્યા સંરચના Xxh64 {
    u64 acc[4];              // the four accumulator lanes; acc[2] still holds the seed
    u64 totalLength;         // input bytes seen so far
    u8  buffer[32];          // input past the last stripe the accumulators took
    u32 bufferLength;
} Xxh64;

શૂન્ય Xxh64_Init (Xxh64* o_state, u64 i_seed);
શૂન્ય Xxh64_Update (Xxh64* io_state, અચલ શૂન્ય* i_data, size_t i_size);
u64  Xxh64_Digest (અચલ Xxh64* i_state);
#endif // d_m3HasSnapshots

cstr_t SPrint_કિંમત (શૂન્ય* i_કિંમત, m3type_t i_type);
size_t SPrintArg (અક્ષર* o_string, size_t i_stringBufferSize, voidptr_t i_sp, m3type_t i_type);

શૂન્ય   ReportError (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_errorMessage, ccstr_t i_file, u32 i_લીટી_અંક);

#જો d_m3RecordBacktraces
શૂન્ય ધકેલો_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc);
શૂન્ય FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function);
શૂન્ય ClearBacktrace (IM3Runtime io_runtime);
#endif

d_m3EndExternC

#endif // m3_core_h
