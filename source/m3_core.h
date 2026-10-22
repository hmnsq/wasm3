//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#यदि_अपरिभाषितम् m3_core_h
#परिभाषय m3_core_h

#अन्तर्भाव <stdio.h>
#अन्तर्भाव <stdlib.h>
#अन्तर्भाव <stdbool.h>
#अन्तर्भाव <string.h>
#अन्तर्भाव <assert.h>

#अन्तर्भाव "wasm3.h"
#अन्तर्भाव "m3_config.h"

#यदि defined(__cplusplus)
#  define d_m3BeginExternC     बाह्य "C" {
#  define d_m3_अन्तः_Extern_C       }
#अन्यथा
#  define d_m3BeginExternC
#  define d_m3_अन्तः_Extern_C
#यदि_अन्तः

d_m3BeginExternC

#परिभाषय d_m3ImplementFloat (d_m3HasFloat || d_m3NoFloatDynamic)

#यदि !defined(d_m3ShortTypesDefined)

प्रकारनिर्देश uint64_t u64;
प्रकारनिर्देश int64_t  i64;
प्रकारनिर्देश uint32_t u32;
प्रकारनिर्देश int32_t  i32;
प्रकारनिर्देश uint16_t u16;
प्रकारनिर्देश int16_t  i16;
प्रकारनिर्देश uint8_t  u8;
प्रकारनिर्देश int8_t   i8;

#  यदि d_m3ImplementFloat
प्रकारनिर्देश द्विगुण f64;
प्रकारनिर्देश भिन्न  f32;
#  endif

#यदि_अन्तः // d_m3ShortTypesDefined

#परिभाषय PRIf32          "f"
#परिभाषय PRIf64          "lf"

प्रकारनिर्देश ध्रुव शून्य*       m3ret_t;
प्रकारनिर्देश ध्रुव शून्य*       voidptr_t;
प्रकारनिर्देश ध्रुव वर्ण*       cstr_t;
प्रकारनिर्देश ध्रुव वर्ण* ध्रुव ccstr_t;
प्रकारनिर्देश ध्रुव u8*         अष्टकानि_t;
प्रकारनिर्देश ध्रुव u8* ध्रुव   cbytes_t;

प्रकारनिर्देश u16               m3opcode_t;
#यदि d_m3HasTypedRefs
प्रकारनिर्देश u16 m3type_t;       // a value type, see the d_m3Type_* bits below
#अन्यथा
प्रकारनिर्देश u8 m3type_t;       // only plain M3ValueType values exist
#यदि_अन्तः

प्रकारनिर्देश i64 m3reg_t;

#यदि d_m3Use32BitSlots
प्रकारनिर्देश u32 m3slot_t;
#अन्यथा
प्रकारनिर्देश u64 m3slot_t;
#यदि_अन्तः

प्रकारनिर्देश m3slot_t* m3stack_t;

#परिभाषय c_io_Slot_गणनम् ((u16)(परिमाण(u64) / परिमाण(m3slot_t)))

प्रकारनिर्देश ध्रुव शून्य* ध्रुव cvptr_t;

#यदि defined(DEBUG)

#  define d_m3Log(CATEGORY, FMT, ...)                प्रारूप_लिख (" %8s  |  " FMT, #CATEGORY, ##__VA_ARGS__);

#  यदि d_m3LogParse
#    define m3log_parse(CATEGORY, FMT, ...)          d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_parse(...) {}
#  endif

#  यदि d_m3LogCompile
#    define m3log_compile(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_compile(...) {}
#  endif

#  यदि d_m3LogEmit
#    define m3log_emit(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_emit(...) {}
#  endif

#  यदि d_m3_Log_संहिता_पृष्ठानि
#    define m3log_संहिता(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_संहिता(...) {}
#  endif

#  यदि d_m3LogModule
#    define m3log_module(CATEGORY, FMT, ...)         d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_module(...) {}
#  endif

#  यदि d_m3LogRuntime
#    define m3log_runtime(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_runtime(...) {}
#  endif

#  define m3log(CATEGORY, FMT, ...)                  m3log_##CATEGORY (CATEGORY, FMT "\n", ##__VA_ARGS__)
#अन्यथा
#  define d_m3Log(CATEGORY, FMT, ...)                {}
#  define m3log(CATEGORY, FMT, ...)                  {}
#यदि_अन्तः


#यदि defined(ASSERTS) || (defined(DEBUG) && !defined(NASSERTS))
#  define d_m3Assert(ASS)  यदि (!(ASS)) { प्रारूप_लिख("Assertion failed at %s:%d : %s\n", __FILE__, __LINE__, #ASS); fflush(stdout); abort(); }
#अन्यथा
#  define d_m3Assert(ASS)
#यदि_अन्तः

प्रकारनिर्देश शून्य /*const*/*                संहिता_t;
प्रकारनिर्देश संहिता_t ध्रुव* /*__restrict__*/ pc_t;


// Sits immediately before the linear memory it describes, so m3MemData() is
// just (header + 1). The interpreter carries a pointer to one of these in the
// _mem pseudo-register, and reaches everything else about the memory - its page
// counts, its owning runtime - through the back-pointers here. 'memory' is what
// makes that possible without going through the runtime, which is the
// prerequisite for a runtime holding more than one memory.
प्रकारनिर्देश संरचना M3_स्मृतिः_शीर्षकम् {
    IM3Runtime       runtime;
    शून्य*            max_राशिः;
    size_t           दीर्घता;
    संरचना M3_स्मृतिः* स्मृतिः;         // the M3Memory this block belongs to
} M3_स्मृतिः_शीर्षकम्;

संरचना M3_संहिता_Mapping_Page;

प्रकारनिर्देश संरचना M3_संहिता_Page_शीर्षकम् {
    संरचना M3_संहिता_Page* अग्र;

    u32                पङ्क्तिः_सूचकांक;
    u32                अङ्कः_पङ्क्तयः;
    u32                sequence;       // this is just used for debugging; could be removed
    u32                usage_गणनम्;

#यदि d_m3_लेखः_Backtraces
    संरचना M3_संहिता_Mapping_Page* mapping;
#यदि_अन्तः // d_m3RecordBacktraces
} M3_संहिता_Page_शीर्षकम्;


#परिभाषय d_m3_संहिता_Page_Free_पङ्क्तयः_देहली      7+2       // max is: ReturnCallIndirect (op + 6 immediates) + 2 for bridge

#परिभाषय d_m3_Default_Mem_Page_आकार              65536

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
#परिभाषय d_m3_स्थानाङ्कः_सीमा_द्वयङ्काः                52
#परिभाषय d_m3_स्थानाङ्कः_सीमा                    (UINT64_C(1) << d_m3_स्थानाङ्कः_सीमा_द्वयङ्काः)

#परिभाषय d_m3Reg0SlotAlias                   60000
#परिभाषय d_m3Fp0SlotAlias                    (d_m3Reg0SlotAlias + 2)

#यदि d_m3HasTypedRefs
// a heap type index has to fit in the spare bits of an m3type_t, see below
#  define d_m3_Max_Sane_Types_गणनम्             4094
#अन्यथा
// M3Environment.numFuncTypes is the u16 that hands out M3FuncType.canonicalIndex,
// so the count has to stay below 65536 here as well
#  define d_m3_Max_Sane_Types_गणनम्             65500
#यदि_अन्तः
#परिभाषय d_m3_Max_Sane_Functions_गणनम्           1000000
#परिभाषय d_m3_Max_Sane_Imports_गणनम्             100000
#परिभाषय d_m3_Max_Sane_Exports_गणनम्             100000
#परिभाषय d_m3_Max_Sane_Globals_गणनम्             1000000
#परिभाषय d_m3_Max_Sane_चिह्नकानि_गणनम्                100000
#परिभाषय d_m3MaxSaneElementSegments          10000000
#परिभाषय d_m3_Max_Sane_दत्त_Segments             100000
#परिभाषय d_m3_Max_Sane_सारणी_आकार                10000000
#परिभाषय d_m3_Max_Sane_Memories_गणनम्            10000
#यदि d_m3HasRefTypes
#  define d_m3_Max_Sane_सारणी_गणनम्             1000
#अन्यथा
#  define d_m3_Max_Sane_सारणी_गणनम्             1           // MVP: a single table
#यदि_अन्तः
#परिभाषय d_m3_Max_Sane_Utf8_दीर्घता               10000
#परिभाषय d_m3_Max_Sane_Function_Arg_Ret_गणनम्      1000    // still insane, but whatever

#परिभाषय d_external_विधा_function             0
#परिभाषय d_external_विधा_सारणी                1
#परिभाषय d_external_विधा_स्मृतिः               2
#परिभाषय d_external_विधा_global               3
#परिभाषय d_external_विधा_चिह्नकम्                  4

// compact-import-section: these stand where an externtype would, after an empty
// item name, and introduce a run of imports sharing one module name. 0x7f gives
// each item its own externtype, 0x7e shares a single one across the whole run.
#परिभाषय d_compact_Imports_प्रत्येकम्_Item_Type        0x7f
#परिभाषय d_compactImports_sharedType         0x7e

// the negated wasm encodings of the abstract reference types, as NormalizeType()
// stores them
#परिभाषय d_waType_funcref                    16
#परिभाषय d_waType_externref                  17

// exception handling: exnref is wasm-encoded as -0x17
#परिभाषय d_waType_exnref                     23

// stack switching: cont type form is -0x23 (35); heap types: cont is -0x18 (24), nocont is -0x0b (11)
#परिभाषय d_waType_cont                       35
#परिभाषय d_waType_heap_cont                  24
#परिभाषय d_waType_heap_nocont                11

// function-references: a reference type spelled out as (ref ht) / (ref null ht),
// where the abstract heap types reuse the funcref and externref encodings
#परिभाषय d_waEncode_ref                      0x64
#परिभाषय d_waEncode_refNull                  0x63

// indexed by M3ValueType: c_m3Type_count entries for the concrete types, plus
// one more for c_m3Type_unknown, which sits right after them
स्थिर ध्रुव वर्ण* ध्रुव c_waTypes[] = { "nil", "i32", "i64", "f32", "f64", "v128", "funcref", "externref", "exnref", "contref", "unknown" };
स्थिर ध्रुव वर्ण* ध्रुव c_waCompactTypes[] = { "_", "i", "I", "f", "F", "V", "r", "R", "e", "c", "?" };


#यदि d_m3VerboseErrorMessages

M3_परिणाम m3Error (M3_परिणाम i_परिणाम, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ध्रुव वर्ण* ध्रुव i_file, u32 i_पङ्क्तिः_अङ्कः, ध्रुव वर्ण* ध्रुव i_error_सन्देशः, ...);

#  define _m3Error(परिणाम_2, RT, MOD, FUN, FILE, पङ्क्तिः, FORMAT, ...) \
            m3Error (परिणाम_2, RT, MOD, FUN, FILE, पङ्क्तिः, FORMAT, ##__VA_ARGS__)

#अन्यथा
#  define _m3Error(परिणाम_2, RT, MOD, FUN, FILE, पङ्क्तिः, FORMAT, ...) (परिणाम_2)
#यदि_अन्तः

#परिभाषय ErrorRuntime(परिणाम_2, RUNTIME, FORMAT, ...)      _m3Error (परिणाम_2, RUNTIME, रिक्त, रिक्त,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#परिभाषय ErrorModule(परिणाम_2, MOD, FORMAT, ...)           _m3Error (परिणाम_2, MOD->runtime, MOD, रिक्त,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#परिभाषय ErrorCompile(परिणाम_2, COMP, FORMAT, ...)         _m3Error (परिणाम_2, COMP->runtime, COMP->module, रिक्त, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)

#यदि d_m3_Guarded_स्मृतिः

// How much address space a linear memory's bytes are given: everything a Wasm access
// can name. A 32-bit address plus a 32-bit memarg offset reaches just under 2^33, and
// the access itself a few bytes further, which the page of slack in a slot covers.
#  define d_m3_Guarded_दत्त_अष्टकानि  (UINT64_C(1) << 33)

// Take one slot of the guarded arena, or NULL when the arena has none left or could
// not be reserved at all. What comes back is the base of the slot: the memory's
// M3MemoryHeader sits at the end of its first page, and the bytes the guest addresses
// begin on the page after that.
शून्य*           Guard_TakeSlot (शून्य);

// The header of a slot, which is also what the guest's memory starts right after
M3_स्मृतिः_शीर्षकम्* Guard_Slot_शीर्षकम् (शून्य* i_slot);

// Make the first i_dataBytes of the slot's data reachable. Only ever asked to grow,
// and asking for what is already there costs nothing.
bool            Guard_CommitSlot (शून्य* i_slot, size_t i_दत्त_अष्टकानि);

// Give a slot back, dropping everything committed in it. The next taker gets it
// zeroed, as a fresh linear memory has to be.
शून्य            Guard_GiveSlot (शून्य* i_slot);

// The whole arena, for a fault to be held against - see m3_HostProtectedCall. Answers
// (NULL, 0) before anything has asked for a slot.
शून्य            Guard_ArenaRange (शून्य** o_नीचः, size_t* o_अष्टकानि);

#यदि_अन्तः // d_m3GuardedMemory

#यदि d_m3_Max_Native_राशिः > 0
// The native-stack low-water mark for a top-level call that came in at i_stackPtr,
// clamped to what the calling thread's stack really is where that can be measured.
// See d_m3MaxNativeStack and d_m3StackLimitEnter.
शून्य* m3_Native_राशिः_सीमा (शून्य* i_राशिः_Ptr, size_t i_budget);
#यदि_अन्तः

#यदि d_m3_Log_Native_राशिः
शून्य m3_राशिः_Check_Init ();
शून्य m3_राशिः_Check ();
पूर्णाङ्क  m3_राशिः_Get_Max ();
#अन्यथा
#  define     m3_राशिः_Check_Init()
#  define     m3_राशिः_Check()
#  define     m3_राशिः_Get_Max()         0
#यदि_अन्तः

#यदि d_m3LogTimestamps
#  define     PRIts                   "%llu"
uint64_t m3_GetTimestamp ();
#अन्यथा
#  define     PRIts                   "%s"
#  define     m3_GetTimestamp()       ""
#यदि_अन्तः

शून्य  m3_Abort (ध्रुव वर्ण* सन्देशः);
शून्य* m3_Malloc_Impl (size_t i_आकार);
शून्य* m3_Realloc_Impl (शून्य* i_ptr, size_t i_new_आकार, size_t i_old_आकार);
शून्य  m3_Free_Impl (शून्य* i_ptr);
शून्य* m3_CopyMem (ध्रुव शून्य* i_from, size_t i_आकार);

#यदि d_m3LogHeapOps

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

स्थिर अन्तःस्थ
शून्य* m3_AllocStruct_Impl (ccstr_t name, size_t i_आकार)
{
    शून्य* परिणाम = m3_Malloc_Impl(i_आकार);
    fprintf(stderr, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_आकार, परिणाम);
    प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
शून्य* m3_AllocArray_Impl (ccstr_t name, size_t i_अङ्कः, size_t i_आकार)
{
    शून्य* परिणाम = m3_Malloc_Impl(i_आकार * i_अङ्कः);
    fprintf(stderr, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_आकार, i_अङ्कः, परिणाम);
    प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
शून्य* m3_ReallocArray_Impl (ccstr_t name, शून्य* i_ptr_old, size_t i_अङ्कः_new, size_t i_अङ्कः_old, size_t i_आकार)
{
    शून्य* परिणाम = m3_Realloc_Impl(i_ptr_old, i_आकार * i_अङ्कः_new, i_आकार * i_अङ्कः_old);
    fprintf(stderr, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_आकार, i_अङ्कः_new, परिणाम, i_अङ्कः_old, i_ptr_old);
    प्रत्यागम परिणाम;
}

स्थिर अन्तःस्थ
शून्य* m3_Malloc (ccstr_t name, size_t i_आकार)
{
    शून्य* परिणाम = m3_Malloc_Impl(i_आकार);
    fprintf(stderr, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_आकार, परिणाम);
    प्रत्यागम परिणाम;
}
स्थिर अन्तःस्थ
शून्य* m3_Realloc (ccstr_t name, शून्य* i_ptr, size_t i_new_आकार, size_t i_old_आकार)
{
    शून्य* परिणाम = m3_Realloc_Impl(i_ptr, i_new_आकार, i_old_आकार);
    fprintf(stderr, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_new_आकार, परिणाम, i_old_आकार, i_ptr);
    प्रत्यागम परिणाम;
}

#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, परिमाण (STRUCT))
#  define     m3_AllocArray(STRUCT, अङ्कः)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, अङ्कः, परिमाण (STRUCT))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (शून्य *)(PTR), (NEW), (OLD), परिमाण (STRUCT))
#  define     m3_Free(P)                              कुरु { शून्य* p = (शून्य*)(P);                                  \
                                                          यदि (p) { fprintf(stderr, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                          m3_Free_Impl (p); (P) = रिक्त; } यावत्(0)
#अन्यथा
#  define     m3_Malloc(NAME, आकार_2)                   m3_Malloc_Impl(आकार_2)
#  define     m3_Realloc(NAME, PTR, NEW, OLD)         m3_Realloc_Impl(PTR, NEW, OLD)
#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_Malloc_Impl (परिमाण (STRUCT))
#  define     m3_AllocArray(STRUCT, अङ्कः)              (STRUCT *)m3_Malloc_Impl (परिमाण (STRUCT) * (अङ्कः))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_Realloc_Impl ((शून्य *)(PTR), परिमाण (STRUCT) * (NEW), परिमाण (STRUCT) * (OLD))
#  define     m3_Free(P)                              कुरु { m3_Free_Impl ((शून्य*)(P)); (P) = रिक्त; } यावत्(0)
#यदि_अन्तः

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

#यदि d_m3HasTypedRefs

#  define d_m3Type_ref                0x8000u
#  define d_m3Type_refNonNull         0x4000u
#  define d_m3Type_refExtern          0x2000u
#  define d_m3Type_refCont            0x1000u
#  define d_m3_Type_heap_आवरणम्           0x0FFFu
#  define d_m3Type_heapAbstract       0x0FFFu     // plain 'func', 'extern', or 'cont'
// 'nocont', the bottom of the continuation types: a reference to it is a
// reference to every one of them, which is what makes ref.null nocont usable
// wherever a particular continuation is expected. One below heapAbstract, and
// so out of reach of d_m3MaxSaneTypesCount.
#  define d_m3Type_heapNone           0x0FFEu

// The storage type a value of this type occupies: a reference of any shape is
// just a funcref or an externref once the type checking is done with.
u8   आधारः_Type_सम्बन्धि (m3type_t i_type);
bool Is_Sub_Type_सम्बन्धि (m3type_t i_sub, m3type_t i_super);

स्थिर अन्तःस्थ
bool IsSpelledRefType (m3type_t i_type)
{
    प्रत्यागम (i_type & d_m3Type_ref) != 0;
}
स्थिर अन्तःस्थ
bool IsNullableRef (m3type_t i_type)
{
    प्रत्यागम (i_type & d_m3Type_refNonNull) == 0;
}
स्थिर अन्तःस्थ
u32 Heap_Type_सम्बन्धि (m3type_t i_type)
{
    प्रत्यागम IsSpelledRefType(i_type) ? (i_type & d_m3_Type_heap_आवरणम्) : d_m3Type_heapAbstract;
}

#अन्यथा // no spelled-out reference types, so every value type is its own storage
      // type and subtyping collapses back to equality

स्थिर अन्तःस्थ
u8 आधारः_Type_सम्बन्धि (m3type_t i_type)
{
    प्रत्यागम (u8)i_type;
}
स्थिर अन्तःस्थ
bool Is_Sub_Type_सम्बन्धि (m3type_t i_sub, m3type_t i_super)
{
    प्रत्यागम i_sub == i_super;
}
स्थिर अन्तःस्थ
bool IsSpelledRefType (m3type_t i_type)
{
    (शून्य)i_type;
    प्रत्यागम false;
}
स्थिर अन्तःस्थ
bool IsNullableRef (m3type_t i_type)
{
    (शून्य)i_type;
    प्रत्यागम true;
}

#यदि_अन्तः // d_m3HasTypedRefs

M3_परिणाम NormalizeType (u8* o_type, i8 i_convolutedWasmType);

// These all speak in terms of how a value is stored, so they reduce a
// spelled-out reference type to its funcref / externref base first.
bool     IsIntType (m3type_t i_wasmType);
bool     IsFpType (m3type_t i_wasmType);
bool     IsRefType (m3type_t i_m3Type);
bool     Is64BitType (m3type_t i_m3Type);
u32      आकार_सम्बन्धि_Type (m3type_t i_m3Type);

M3_परिणाम Read_u64 (u64* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम Read_u32 (u32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
#यदि d_m3ImplementFloat
M3_परिणाम Read_f64 (f64* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम Read_f32 (f32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
#यदि_अन्तः
M3_परिणाम Read_u8 (u8* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम Read_opcode (m3opcode_t* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);

M3_परिणाम ReadLebUnsigned (u64* o_मानम्, u32 i_max_अङ्कः_द्वयङ्काः, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम ReadLebSigned (i64* o_मानम्, u32 i_max_अङ्कः_द्वयङ्काः, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम ReadLEB_u32 (u32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);

// The memarg immediate of a load or store: an alignment hint, then a static
// offset. Multi-memory reinterprets the alignment as a bitfield -- bit 6 says a
// memory index sits between the two -- so the hint is returned with that bit
// masked off and the index defaults to 0 when it is clear.
M3_परिणाम Read_स्मृतिः_Arg (u32* o_align, u32* o_स्मृतिः_Idx, u64* o_विस्थापनम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम ReadLEB_u7 (u8* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम ReadLEB_i7 (i8* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम ReadLEB_i32 (i32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम ReadLEB_i64 (i64* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
M3_परिणाम Read_utf8 (cstr_t* o_utf8, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);

#यदि d_m3HasSnapshots
// XXH64, in m3_xxh64.c. Initialize a state, feed it the input in any number of
// pieces, then read the digest out. The digest depends only on the bytes fed in
// - not on how they were split up, nor on the host's byte order, pointer width
// or compiler - and matches the reference xxHash implementation, so anything
// else that speaks XXH64 computes the same value. Reading the digest does not
// consume the state.
प्रकारनिर्देश संरचना Xxh64 {
    u64 acc[4];              // the four accumulator lanes; acc[2] still holds the seed
    u64 योग_दीर्घता;         // input bytes seen so far
    u8  बफर[32];          // input past the last stripe the accumulators took
    u32 बफर_दीर्घता;
} Xxh64;

शून्य Xxh64_Init (Xxh64* o_state, u64 i_बीजम्);
शून्य Xxh64_Update (Xxh64* io_state, ध्रुव शून्य* i_दत्त, size_t i_आकार);
u64  Xxh64_Digest (ध्रुव Xxh64* i_state);
#यदि_अन्तः // d_m3HasSnapshots

cstr_t SPrint_मानम् (शून्य* i_मानम्, m3type_t i_type);
size_t SPrintArg (वर्ण* o_string, size_t i_string_बफर_आकार, voidptr_t i_sp, m3type_t i_type);

शून्य   ReportError (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_error_सन्देशः, ccstr_t i_file, u32 i_पङ्क्तिः_अङ्कः);

#यदि d_m3_लेखः_Backtraces
शून्य प्रक्षिप_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc);
शून्य FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function);
शून्य ClearBacktrace (IM3Runtime io_runtime);
#यदि_अन्तः

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_core_h
