//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#నిర్వచితం_కాకపోతే m3_core_h
#నిర్వచించు m3_core_h

#అన్తర్భావ <stdio.h>
#అన్తర్భావ <stdlib.h>
#అన్తర్భావ <stdbool.h>
#అన్తర్భావ <string.h>
#అన్తర్భావ <assert.h>

#అన్తర్భావ "wasm3.h"
#అన్తర్భావ "m3_config.h"

#ఒకవేళ defined(__cplusplus)
#  define d_m3BeginExternC     బాహ్య "C" {
#  define d_m3_చివర_Extern_C       }
#లేకపోతే
#  define d_m3BeginExternC
#  define d_m3_చివర_Extern_C
#అయితే_ముగింపు

d_m3BeginExternC

#నిర్వచించు d_m3ImplementFloat (d_m3HasFloat || d_m3NoFloatDynamic)

#ఒకవేళ !defined(d_m3ShortTypesDefined)

రకనిర్వచనం uint64_t u64;
రకనిర్వచనం int64_t  i64;
రకనిర్వచనం uint32_t u32;
రకనిర్వచనం int32_t  i32;
రకనిర్వచనం uint16_t u16;
రకనిర్వచనం int16_t  i16;
రకనిర్వచనం uint8_t  u8;
రకనిర్వచనం int8_t   i8;

#  ఒకవేళ d_m3ImplementFloat
రకనిర్వచనం ద్విగుణ f64;
రకనిర్వచనం దశాంశం  f32;
#  endif

#అయితే_ముగింపు // d_m3ShortTypesDefined

#నిర్వచించు PRIf32          "f"
#నిర్వచించు PRIf64          "lf"

రకనిర్వచనం స్థిరరాశి శూన్య*       m3ret_t;
రకనిర్వచనం స్థిరరాశి శూన్య*       voidptr_t;
రకనిర్వచనం స్థిరరాశి అక్షరం*       cstr_t;
రకనిర్వచనం స్థిరరాశి అక్షరం* స్థిరరాశి ccstr_t;
రకనిర్వచనం స్థిరరాశి u8*         బైట్లు_t;
రకనిర్వచనం స్థిరరాశి u8* స్థిరరాశి   cbytes_t;

రకనిర్వచనం u16               m3opcode_t;
#ఒకవేళ d_m3HasTypedRefs
రకనిర్వచనం u16 m3type_t;       // a value type, see the d_m3Type_* bits below
#లేకపోతే
రకనిర్వచనం u8 m3type_t;       // only plain M3ValueType values exist
#అయితే_ముగింపు

రకనిర్వచనం i64 m3reg_t;

#ఒకవేళ d_m3_Use32_బిట్టు_Slots
రకనిర్వచనం u32 m3slot_t;
#లేకపోతే
రకనిర్వచనం u64 m3slot_t;
#అయితే_ముగింపు

రకనిర్వచనం m3slot_t* m3stack_t;

#నిర్వచించు c_io_స్థానం_లెక్క ((u16)(పరిమాణం(u64) / పరిమాణం(m3slot_t)))

రకనిర్వచనం స్థిరరాశి శూన్య* స్థిరరాశి cvptr_t;

#ఒకవేళ defined(DEBUG)

#  define d_m3Log(CATEGORY, FMT, ...)                రాయి (" %8s  |  " FMT, #CATEGORY, ##__VA_ARGS__);

#  ఒకవేళ d_m3LogParse
#    define m3log_parse(CATEGORY, FMT, ...)          d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  లేకపోతే
#    define m3log_parse(...) {}
#  endif

#  ఒకవేళ d_m3LogCompile
#    define m3log_compile(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  లేకపోతే
#    define m3log_compile(...) {}
#  endif

#  ఒకవేళ d_m3LogEmit
#    define m3log_emit(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  లేకపోతే
#    define m3log_emit(...) {}
#  endif

#  ఒకవేళ d_m3_Log_సంకేతం_పుటలు
#    define m3log_సంకేతం(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  లేకపోతే
#    define m3log_సంకేతం(...) {}
#  endif

#  ఒకవేళ d_m3LogModule
#    define m3log_module(CATEGORY, FMT, ...)         d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  లేకపోతే
#    define m3log_module(...) {}
#  endif

#  ఒకవేళ d_m3LogRuntime
#    define m3log_runtime(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  లేకపోతే
#    define m3log_runtime(...) {}
#  endif

#  define m3log(CATEGORY, FMT, ...)                  m3log_##CATEGORY (CATEGORY, FMT "\n", ##__VA_ARGS__)
#లేకపోతే
#  define d_m3Log(CATEGORY, FMT, ...)                {}
#  define m3log(CATEGORY, FMT, ...)                  {}
#అయితే_ముగింపు


#ఒకవేళ defined(ASSERTS) || (defined(DEBUG) && !defined(NASSERTS))
#  define d_m3Assert(ASS)  ఒకవేళ (!(ASS)) { రాయి("Assertion failed at %s:%d : %s\n", __FILE__, __LINE__, #ASS); fflush(stdout); abort(); }
#లేకపోతే
#  define d_m3Assert(ASS)
#అయితే_ముగింపు

రకనిర్వచనం శూన్య /*const*/*                సంకేతం_t;
రకనిర్వచనం సంకేతం_t స్థిరరాశి* /*__restrict__*/ pc_t;


// Sits immediately before the linear memory it describes, so m3MemData() is
// just (header + 1). The interpreter carries a pointer to one of these in the
// _mem pseudo-register, and reaches everything else about the memory - its page
// counts, its owning runtime - through the back-pointers here. 'memory' is what
// makes that possible without going through the runtime, which is the
// prerequisite for a runtime holding more than one memory.
రకనిర్వచనం నిర్మాణం M3_జ్ఞాపకం_శీర్షిక {
    IM3Runtime       runtime;
    శూన్య*            max_దొంతర;
    size_t           నిడివి;
    నిర్మాణం M3_జ్ఞాపకం* జ్ఞాపకం;         // the M3Memory this block belongs to
} M3_జ్ఞాపకం_శీర్షిక;

నిర్మాణం M3_సంకేతం_Mapping_పుట;

రకనిర్వచనం నిర్మాణం M3_సంకేతం_పుట_శీర్షిక {
    నిర్మాణం M3_సంకేతం_పుట* తదుపరి;

    u32                పంక్తి_సూచీ;
    u32                అంకె_పంక్తులు;
    u32                sequence;       // this is just used for debugging; could be removed
    u32                usage_లెక్క;

#ఒకవేళ d_m3_రికార్డు_Backtraces
    నిర్మాణం M3_సంకేతం_Mapping_పుట* mapping;
#అయితే_ముగింపు // d_m3RecordBacktraces
} M3_సంకేతం_పుట_శీర్షిక;


#నిర్వచించు d_m3_సంకేతం_పుట_Free_పంక్తులు_పరిమితి      7+2       // max is: ReturnCallIndirect (op + 6 immediates) + 2 for bridge

#నిర్వచించు d_m3_Default_Mem_పుట_కొలత              65536

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
#నిర్వచించు d_m3_చిరునామా_హద్దు_బిట్లు                52
#నిర్వచించు d_m3_చిరునామా_హద్దు                    (UINT64_C(1) << d_m3_చిరునామా_హద్దు_బిట్లు)

#నిర్వచించు d_m3_Reg0_స్థానం_Alias                   60000
#నిర్వచించు d_m3_Fp0_స్థానం_Alias                    (d_m3_Reg0_స్థానం_Alias + 2)

#ఒకవేళ d_m3HasTypedRefs
// a heap type index has to fit in the spare bits of an m3type_t, see below
#  define d_m3_Max_Sane_Types_లెక్క             4094
#లేకపోతే
// M3Environment.numFuncTypes is the u16 that hands out M3FuncType.canonicalIndex,
// so the count has to stay below 65536 here as well
#  define d_m3_Max_Sane_Types_లెక్క             65500
#అయితే_ముగింపు
#నిర్వచించు d_m3_Max_Sane_Functions_లెక్క           1000000
#నిర్వచించు d_m3_Max_Sane_Imports_లెక్క             100000
#నిర్వచించు d_m3_Max_Sane_Exports_లెక్క             100000
#నిర్వచించు d_m3_Max_Sane_Globals_లెక్క             1000000
#నిర్వచించు d_m3_Max_Sane_ట్యాగులు_లెక్క                100000
#నిర్వచించు d_m3MaxSaneElementSegments          10000000
#నిర్వచించు d_m3_Max_Sane_దత్తాంశం_Segments             100000
#నిర్వచించు d_m3_Max_Sane_పట్టిక_కొలత                10000000
#నిర్వచించు d_m3_Max_Sane_Memories_లెక్క            10000
#ఒకవేళ d_m3HasRefTypes
#  define d_m3_Max_Sane_పట్టిక_లెక్క             1000
#లేకపోతే
#  define d_m3_Max_Sane_పట్టిక_లెక్క             1           // MVP: a single table
#అయితే_ముగింపు
#నిర్వచించు d_m3_Max_Sane_Utf8_నిడివి               10000
#నిర్వచించు d_m3_Max_Sane_Function_Arg_Ret_లెక్క      1000    // still insane, but whatever

#నిర్వచించు d_external_రకం_function             0
#నిర్వచించు d_external_రకం_పట్టిక                1
#నిర్వచించు d_external_రకం_జ్ఞాపకం               2
#నిర్వచించు d_external_రకం_global               3
#నిర్వచించు d_external_రకం_ట్యాగ్                  4

// compact-import-section: these stand where an externtype would, after an empty
// item name, and introduce a run of imports sharing one module name. 0x7f gives
// each item its own externtype, 0x7e shares a single one across the whole run.
#నిర్వచించు d_compact_Imports_చొప్పున_అంశం_Type        0x7f
#నిర్వచించు d_compactImports_sharedType         0x7e

// the negated wasm encodings of the abstract reference types, as NormalizeType()
// stores them
#నిర్వచించు d_waType_funcref                    16
#నిర్వచించు d_waType_externref                  17

// exception handling: exnref is wasm-encoded as -0x17
#నిర్వచించు d_waType_exnref                     23

// stack switching: cont type form is -0x23 (35); heap types: cont is -0x18 (24), nocont is -0x0b (11)
#నిర్వచించు d_waType_cont                       35
#నిర్వచించు d_wa_Type_కుప్ప_cont                  24
#నిర్వచించు d_wa_Type_కుప్ప_nocont                11

// function-references: a reference type spelled out as (ref ht) / (ref null ht),
// where the abstract heap types reuse the funcref and externref encodings
#నిర్వచించు d_waEncode_ref                      0x64
#నిర్వచించు d_waEncode_refNull                  0x63

// indexed by M3ValueType: c_m3Type_count entries for the concrete types, plus
// one more for c_m3Type_unknown, which sits right after them
స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి c_waTypes[] = { "nil", "i32", "i64", "f32", "f64", "v128", "funcref", "externref", "exnref", "contref", "unknown" };
స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి c_waCompactTypes[] = { "_", "i", "I", "f", "F", "V", "r", "R", "e", "c", "?" };


#ఒకవేళ d_m3VerboseErrorMessages

M3_ఫలితం m3Error (M3_ఫలితం i_ఫలితం, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  స్థిరరాశి అక్షరం* స్థిరరాశి i_file, u32 i_పంక్తి_అంకె, స్థిరరాశి అక్షరం* స్థిరరాశి i_error_సందేశం, ...);

#  define _m3Error(ఫలితం_2, RT, MOD, FUN, FILE, పంక్తి, FORMAT, ...) \
            m3Error (ఫలితం_2, RT, MOD, FUN, FILE, పంక్తి, FORMAT, ##__VA_ARGS__)

#లేకపోతే
#  define _m3Error(ఫలితం_2, RT, MOD, FUN, FILE, పంక్తి, FORMAT, ...) (ఫలితం_2)
#అయితే_ముగింపు

#నిర్వచించు ErrorRuntime(ఫలితం_2, RUNTIME, FORMAT, ...)      _m3Error (ఫలితం_2, RUNTIME, ఖాళీ, ఖాళీ,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#నిర్వచించు ErrorModule(ఫలితం_2, MOD, FORMAT, ...)           _m3Error (ఫలితం_2, MOD->runtime, MOD, ఖాళీ,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#నిర్వచించు ErrorCompile(ఫలితం_2, COMP, FORMAT, ...)         _m3Error (ఫలితం_2, COMP->runtime, COMP->module, ఖాళీ, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)

#ఒకవేళ d_m3_Guarded_జ్ఞాపకం

// How much address space a linear memory's bytes are given: everything a Wasm access
// can name. A 32-bit address plus a 32-bit memarg offset reaches just under 2^33, and
// the access itself a few bytes further, which the page of slack in a slot covers.
#  define d_m3_Guarded_దత్తాంశం_బైట్లు  (UINT64_C(1) << 33)

// Take one slot of the guarded arena, or NULL when the arena has none left or could
// not be reserved at all. What comes back is the base of the slot: the memory's
// M3MemoryHeader sits at the end of its first page, and the bytes the guest addresses
// begin on the page after that.
శూన్య*           Guard_Take_స్థానం (శూన్య);

// The header of a slot, which is also what the guest's memory starts right after
M3_జ్ఞాపకం_శీర్షిక* Guard_స్థానం_శీర్షిక (శూన్య* i_స్థానం);

// Make the first i_dataBytes of the slot's data reachable. Only ever asked to grow,
// and asking for what is already there costs nothing.
bool            Guard_Commit_స్థానం (శూన్య* i_స్థానం, size_t i_దత్తాంశం_బైట్లు);

// Give a slot back, dropping everything committed in it. The next taker gets it
// zeroed, as a fresh linear memory has to be.
శూన్య            Guard_Give_స్థానం (శూన్య* i_స్థానం);

// The whole arena, for a fault to be held against - see m3_HostProtectedCall. Answers
// (NULL, 0) before anything has asked for a slot.
శూన్య            Guard_ArenaRange (శూన్య** o_దిగువ, size_t* o_బైట్లు);

#అయితే_ముగింపు // d_m3GuardedMemory

#ఒకవేళ d_m3_Max_Native_దొంతర > 0
// The native-stack low-water mark for a top-level call that came in at i_stackPtr,
// clamped to what the calling thread's stack really is where that can be measured.
// See d_m3MaxNativeStack and d_m3StackLimitEnter.
శూన్య* m3_Native_దొంతర_హద్దు (శూన్య* i_దొంతర_Ptr, size_t i_budget);
#అయితే_ముగింపు

#ఒకవేళ d_m3_Log_Native_దొంతర
శూన్య m3_దొంతర_Check_Init ();
శూన్య m3_దొంతర_Check ();
పూర్ణాంకం  m3_దొంతర_Get_Max ();
#లేకపోతే
#  define     m3_దొంతర_Check_Init()
#  define     m3_దొంతర_Check()
#  define     m3_దొంతర_Get_Max()         0
#అయితే_ముగింపు

#ఒకవేళ d_m3LogTimestamps
#  define     PRIts                   "%llu"
uint64_t m3_GetTimestamp ();
#లేకపోతే
#  define     PRIts                   "%s"
#  define     m3_GetTimestamp()       ""
#అయితే_ముగింపు

శూన్య  m3_Abort (స్థిరరాశి అక్షరం* సందేశం);
శూన్య* m3_Malloc_Impl (size_t i_కొలత);
శూన్య* m3_Realloc_Impl (శూన్య* i_ptr, size_t i_new_కొలత, size_t i_old_కొలత);
శూన్య  m3_Free_Impl (శూన్య* i_ptr);
శూన్య* m3_CopyMem (స్థిరరాశి శూన్య* i_from, size_t i_కొలత);

#ఒకవేళ d_m3_Log_కుప్ప_Ops

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

స్థిర అంతఃవరుస
శూన్య* m3_AllocStruct_Impl (ccstr_t name, size_t i_కొలత)
{
    శూన్య* ఫలితం = m3_Malloc_Impl(i_కొలత);
    fprintf(stderr, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_కొలత, ఫలితం);
    తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
శూన్య* m3_AllocArray_Impl (ccstr_t name, size_t i_అంకె, size_t i_కొలత)
{
    శూన్య* ఫలితం = m3_Malloc_Impl(i_కొలత * i_అంకె);
    fprintf(stderr, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_కొలత, i_అంకె, ఫలితం);
    తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
శూన్య* m3_ReallocArray_Impl (ccstr_t name, శూన్య* i_ptr_old, size_t i_అంకె_new, size_t i_అంకె_old, size_t i_కొలత)
{
    శూన్య* ఫలితం = m3_Realloc_Impl(i_ptr_old, i_కొలత * i_అంకె_new, i_కొలత * i_అంకె_old);
    fprintf(stderr, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_కొలత, i_అంకె_new, ఫలితం, i_అంకె_old, i_ptr_old);
    తిరిగి ఫలితం;
}

స్థిర అంతఃవరుస
శూన్య* m3_Malloc (ccstr_t name, size_t i_కొలత)
{
    శూన్య* ఫలితం = m3_Malloc_Impl(i_కొలత);
    fprintf(stderr, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_కొలత, ఫలితం);
    తిరిగి ఫలితం;
}
స్థిర అంతఃవరుస
శూన్య* m3_Realloc (ccstr_t name, శూన్య* i_ptr, size_t i_new_కొలత, size_t i_old_కొలత)
{
    శూన్య* ఫలితం = m3_Realloc_Impl(i_ptr, i_new_కొలత, i_old_కొలత);
    fprintf(stderr, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_new_కొలత, ఫలితం, i_old_కొలత, i_ptr);
    తిరిగి ఫలితం;
}

#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, పరిమాణం (STRUCT))
#  define     m3_AllocArray(STRUCT, అంకె)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, అంకె, పరిమాణం (STRUCT))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (శూన్య *)(PTR), (NEW), (OLD), పరిమాణం (STRUCT))
#  define     m3_Free(P)                              చేయి { శూన్య* p = (శూన్య*)(P);                                  \
                                                          ఒకవేళ (p) { fprintf(stderr, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                          m3_Free_Impl (p); (P) = ఖాళీ; } అయితే(0)
#లేకపోతే
#  define     m3_Malloc(NAME, కొలత_2)                   m3_Malloc_Impl(కొలత_2)
#  define     m3_Realloc(NAME, PTR, NEW, OLD)         m3_Realloc_Impl(PTR, NEW, OLD)
#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_Malloc_Impl (పరిమాణం (STRUCT))
#  define     m3_AllocArray(STRUCT, అంకె)              (STRUCT *)m3_Malloc_Impl (పరిమాణం (STRUCT) * (అంకె))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_Realloc_Impl ((శూన్య *)(PTR), పరిమాణం (STRUCT) * (NEW), పరిమాణం (STRUCT) * (OLD))
#  define     m3_Free(P)                              చేయి { m3_Free_Impl ((శూన్య*)(P)); (P) = ఖాళీ; } అయితే(0)
#అయితే_ముగింపు

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

#ఒకవేళ d_m3HasTypedRefs

#  define d_m3Type_ref                0x8000u
#  define d_m3Type_refNonNull         0x4000u
#  define d_m3Type_refExtern          0x2000u
#  define d_m3Type_refCont            0x1000u
#  define d_m3_Type_కుప్ప_ముసుగు           0x0FFFu
#  define d_m3_Type_కుప్ప_Abstract       0x0FFFu     // plain 'func', 'extern', or 'cont'
// 'nocont', the bottom of the continuation types: a reference to it is a
// reference to every one of them, which is what makes ref.null nocont usable
// wherever a particular continuation is expected. One below heapAbstract, and
// so out of reach of d_m3MaxSaneTypesCount.
#  define d_m3_Type_కుప్ప_None           0x0FFEu

// The storage type a value of this type occupies: a reference of any shape is
// just a funcref or an externref once the type checking is done with.
u8   పునాది_Type_యొక్క (m3type_t i_type);
bool Is_Sub_Type_యొక్క (m3type_t i_sub, m3type_t i_super);

స్థిర అంతఃవరుస
bool IsSpelledRefType (m3type_t i_type)
{
    తిరిగి (i_type & d_m3Type_ref) != 0;
}
స్థిర అంతఃవరుస
bool IsNullableRef (m3type_t i_type)
{
    తిరిగి (i_type & d_m3Type_refNonNull) == 0;
}
స్థిర అంతఃవరుస
u32 కుప్ప_Type_యొక్క (m3type_t i_type)
{
    తిరిగి IsSpelledRefType(i_type) ? (i_type & d_m3_Type_కుప్ప_ముసుగు) : d_m3_Type_కుప్ప_Abstract;
}

#లేకపోతే // no spelled-out reference types, so every value type is its own storage
      // type and subtyping collapses back to equality

స్థిర అంతఃవరుస
u8 పునాది_Type_యొక్క (m3type_t i_type)
{
    తిరిగి (u8)i_type;
}
స్థిర అంతఃవరుస
bool Is_Sub_Type_యొక్క (m3type_t i_sub, m3type_t i_super)
{
    తిరిగి i_sub == i_super;
}
స్థిర అంతఃవరుస
bool IsSpelledRefType (m3type_t i_type)
{
    (శూన్య)i_type;
    తిరిగి false;
}
స్థిర అంతఃవరుస
bool IsNullableRef (m3type_t i_type)
{
    (శూన్య)i_type;
    తిరిగి true;
}

#అయితే_ముగింపు // d_m3HasTypedRefs

M3_ఫలితం NormalizeType (u8* o_type, i8 i_convolutedWasmType);

// These all speak in terms of how a value is stored, so they reduce a
// spelled-out reference type to its funcref / externref base first.
bool     IsIntType (m3type_t i_wasmType);
bool     IsFpType (m3type_t i_wasmType);
bool     IsRefType (m3type_t i_m3Type);
bool     Is64_బిట్టు_Type (m3type_t i_m3Type);
u32      కొలత_యొక్క_Type (m3type_t i_m3Type);

M3_ఫలితం Read_u64 (u64* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం Read_u32 (u32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
#ఒకవేళ d_m3ImplementFloat
M3_ఫలితం Read_f64 (f64* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం Read_f32 (f32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
#అయితే_ముగింపు
M3_ఫలితం Read_u8 (u8* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం Read_opcode (m3opcode_t* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);

M3_ఫలితం ReadLebUnsigned (u64* o_విలువ, u32 i_max_అంకె_బిట్లు, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం ReadLebSigned (i64* o_విలువ, u32 i_max_అంకె_బిట్లు, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం ReadLEB_u32 (u32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);

// The memarg immediate of a load or store: an alignment hint, then a static
// offset. Multi-memory reinterprets the alignment as a bitfield -- bit 6 says a
// memory index sits between the two -- so the hint is returned with that bit
// masked off and the index defaults to 0 when it is clear.
M3_ఫలితం Read_జ్ఞాపకం_Arg (u32* o_align, u32* o_జ్ఞాపకం_Idx, u64* o_స్థానభ్రంశం, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం ReadLEB_u7 (u8* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం ReadLEB_i7 (i8* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం ReadLEB_i32 (i32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం ReadLEB_i64 (i64* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
M3_ఫలితం Read_utf8 (cstr_t* o_utf8, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);

#ఒకవేళ d_m3HasSnapshots
// XXH64, in m3_xxh64.c. Initialize a state, feed it the input in any number of
// pieces, then read the digest out. The digest depends only on the bytes fed in
// - not on how they were split up, nor on the host's byte order, pointer width
// or compiler - and matches the reference xxHash implementation, so anything
// else that speaks XXH64 computes the same value. Reading the digest does not
// consume the state.
రకనిర్వచనం నిర్మాణం Xxh64 {
    u64 acc[4];              // the four accumulator lanes; acc[2] still holds the seed
    u64 మొత్తం_నిడివి;         // input bytes seen so far
    u8  బఫరు[32];          // input past the last stripe the accumulators took
    u32 బఫరు_నిడివి;
} Xxh64;

శూన్య Xxh64_Init (Xxh64* o_state, u64 i_విత్తనం);
శూన్య Xxh64_Update (Xxh64* io_state, స్థిరరాశి శూన్య* i_దత్తాంశం, size_t i_కొలత);
u64  Xxh64_Digest (స్థిరరాశి Xxh64* i_state);
#అయితే_ముగింపు // d_m3HasSnapshots

cstr_t SPrint_విలువ (శూన్య* i_విలువ, m3type_t i_type);
size_t SPrintArg (అక్షరం* o_string, size_t i_string_బఫరు_కొలత, voidptr_t i_sp, m3type_t i_type);

శూన్య   ReportError (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_error_సందేశం, ccstr_t i_file, u32 i_పంక్తి_అంకె);

#ఒకవేళ d_m3_రికార్డు_Backtraces
శూన్య నెట్టు_Backtrace_చట్రం (IM3Runtime io_runtime, pc_t i_pc);
శూన్య FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function);
శూన్య ClearBacktrace (IM3Runtime io_runtime);
#అయితే_ముగింపు

d_m3_చివర_Extern_C

#అయితే_ముగింపు // m3_core_h
