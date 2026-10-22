//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अगर_अघोषित m3_core_h
#घोषणा m3_core_h

#समावेश <stdio.h>
#समावेश <stdlib.h>
#समावेश <stdbool.h>
#समावेश <string.h>
#समावेश <assert.h>

#समावेश "wasm3.h"
#समावेश "m3_config.h"

#अगर defined(__cplusplus)
#  define d_m3BeginExternC     बाह्य "C" {
#  define d_m3EndExternC       }
#अन्यथा
#  define d_m3BeginExternC
#  define d_m3EndExternC
#पूर्ण_अगर

d_m3BeginExternC

#घोषणा d_m3ImplementFloat (d_m3HasFloat || d_m3NoFloatDynamic)

#अगर !defined(d_m3ShortTypesDefined)

प्रकार uint64_t u64;
प्रकार int64_t  i64;
प्रकार uint32_t u32;
प्रकार int32_t  i32;
प्रकार uint16_t u16;
प्रकार int16_t  i16;
प्रकार uint8_t  u8;
प्रकार int8_t   i8;

#  अगर d_m3ImplementFloat
प्रकार द्विगुन f64;
प्रकार भग्न  f32;
#  endif

#पूर्ण_अगर // d_m3ShortTypesDefined

#घोषणा PRIf32          "f"
#घोषणा PRIf64          "lf"

प्रकार स्थिर व्योम*       m3ret_t;
प्रकार स्थिर व्योम*       voidptr_t;
प्रकार स्थिर अक्षर*       cstr_t;
प्रकार स्थिर अक्षर* स्थिर ccstr_t;
प्रकार स्थिर u8*         bytes_t;
प्रकार स्थिर u8* स्थिर   cbytes_t;

प्रकार u16               m3opcode_t;
#अगर d_m3HasTypedRefs
प्रकार u16 m3type_t;       // a value type, see the d_m3Type_* bits below
#अन्यथा
प्रकार u8 m3type_t;       // only plain M3ValueType values exist
#पूर्ण_अगर

प्रकार i64 m3reg_t;

#अगर d_m3Use32BitSlots
प्रकार u32 m3slot_t;
#अन्यथा
प्रकार u64 m3slot_t;
#पूर्ण_अगर

प्रकार m3slot_t* m3stack_t;

#घोषणा c_io_Slot_गिनती ((u16)(माप(u64) / माप(m3slot_t)))

प्रकार स्थिर व्योम* स्थिर cvptr_t;

#अगर defined(DEBUG)

#  define d_m3Log(CATEGORY, FMT, ...)                म_लिखो (" %8s  |  " FMT, #CATEGORY, ##__VA_ARGS__);

#  अगर d_m3LogParse
#    define m3log_parse(CATEGORY, FMT, ...)          d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_parse(...) {}
#  endif

#  अगर d_m3LogCompile
#    define m3log_compile(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_compile(...) {}
#  endif

#  अगर d_m3LogEmit
#    define m3log_emit(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_emit(...) {}
#  endif

#  अगर d_m3LogCodePages
#    define m3log_code(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_code(...) {}
#  endif

#  अगर d_m3LogModule
#    define m3log_module(CATEGORY, FMT, ...)         d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_module(...) {}
#  endif

#  अगर d_m3LogRuntime
#    define m3log_runtime(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  अन्यथा
#    define m3log_runtime(...) {}
#  endif

#  define m3log(CATEGORY, FMT, ...)                  m3log_##CATEGORY (CATEGORY, FMT "\n", ##__VA_ARGS__)
#अन्यथा
#  define d_m3Log(CATEGORY, FMT, ...)                {}
#  define m3log(CATEGORY, FMT, ...)                  {}
#पूर्ण_अगर


#अगर defined(ASSERTS) || (defined(DEBUG) && !defined(NASSERTS))
#  define d_m3Assert(ASS)  अगर (!(ASS)) { म_लिखो("Assertion failed at %s:%d : %s\n", __खाता__, __LINE__, #ASS); ख_साफ(मानक_निकास); पात(); }
#अन्यथा
#  define d_m3Assert(ASS)
#पूर्ण_अगर

प्रकार व्योम /*const*/*                code_t;
प्रकार code_t स्थिर* /*__restrict__*/ pc_t;


// Sits immediately before the linear memory it describes, so m3MemData() is
// just (header + 1). The interpreter carries a pointer to one of these in the
// _mem pseudo-register, and reaches everything else about the memory - its page
// counts, its owning runtime - through the back-pointers here. 'memory' is what
// makes that possible without going through the runtime, which is the
// prerequisite for a runtime holding more than one memory.
प्रकार काष्ठा M3MemoryHeader {
    IM3Runtime       runtime;
    व्योम*            max_ढेर;
    माप_प्रकार           लंबाई;
    काष्ठा M3Memory* memory;         // the M3Memory this block belongs to
} M3MemoryHeader;

काष्ठा M3CodeMappingPage;

प्रकार काष्ठा M3CodePageHeader {
    काष्ठा M3CodePage* अग्र;

    u32                पंक्ति_सूचकांक;
    u32                अंक_पंक्तियाँ;
    u32                sequence;       // this is just used for debugging; could be removed
    u32                usage_गिनती;

#अगर d_m3RecordBacktraces
    काष्ठा M3CodeMappingPage* mapping;
#पूर्ण_अगर // d_m3RecordBacktraces
} M3CodePageHeader;


#घोषणा d_m3_Code_Page_Free_पंक्तियाँ_Threshold      7+2       // max is: ReturnCallIndirect (op + 6 immediates) + 2 for bridge

#घोषणा d_m3_Default_Mem_Page_आकार              65536

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
#घोषणा d_m3AddressLimitBits                52
#घोषणा d_m3AddressLimit                    (UINT64_C(1) << d_m3AddressLimitBits)

#घोषणा d_m3Reg0SlotAlias                   60000
#घोषणा d_m3Fp0SlotAlias                    (d_m3Reg0SlotAlias + 2)

#अगर d_m3HasTypedRefs
// a heap type index has to fit in the spare bits of an m3type_t, see below
#  define d_m3_Max_Sane_Types_गिनती             4094
#अन्यथा
// M3Environment.numFuncTypes is the u16 that hands out M3FuncType.canonicalIndex,
// so the count has to stay below 65536 here as well
#  define d_m3_Max_Sane_Types_गिनती             65500
#पूर्ण_अगर
#घोषणा d_m3_Max_Sane_Functions_गिनती           1000000
#घोषणा d_m3_Max_Sane_Imports_गिनती             100000
#घोषणा d_m3_Max_Sane_Exports_गिनती             100000
#घोषणा d_m3_Max_Sane_Globals_गिनती             1000000
#घोषणा d_m3_Max_Sane_Tags_गिनती                100000
#घोषणा d_m3MaxSaneElementSegments          10000000
#घोषणा d_m3_Max_Sane_तथ्य_Segments             100000
#घोषणा d_m3_Max_Sane_Table_आकार                10000000
#घोषणा d_m3_Max_Sane_Memories_गिनती            10000
#अगर d_m3HasRefTypes
#  define d_m3_Max_Sane_Table_गिनती             1000
#अन्यथा
#  define d_m3_Max_Sane_Table_गिनती             1           // MVP: a single table
#पूर्ण_अगर
#घोषणा d_m3_Max_Sane_Utf8_लंबाई               10000
#घोषणा d_m3_Max_Sane_Function_Arg_Ret_गिनती      1000    // still insane, but whatever

#घोषणा d_externalKind_function             0
#घोषणा d_externalKind_table                1
#घोषणा d_externalKind_memory               2
#घोषणा d_externalKind_global               3
#घोषणा d_externalKind_tag                  4

// compact-import-section: these stand where an externtype would, after an empty
// item name, and introduce a run of imports sharing one module name. 0x7f gives
// each item its own externtype, 0x7e shares a single one across the whole run.
#घोषणा d_compactImports_perItemType        0x7f
#घोषणा d_compactImports_sharedType         0x7e

// the negated wasm encodings of the abstract reference types, as NormalizeType()
// stores them
#घोषणा d_waType_funcref                    16
#घोषणा d_waType_externref                  17

// exception handling: exnref is wasm-encoded as -0x17
#घोषणा d_waType_exnref                     23

// stack switching: cont type form is -0x23 (35); heap types: cont is -0x18 (24), nocont is -0x0b (11)
#घोषणा d_waType_cont                       35
#घोषणा d_waType_heap_cont                  24
#घोषणा d_waType_heap_nocont                11

// function-references: a reference type spelled out as (ref ht) / (ref null ht),
// where the abstract heap types reuse the funcref and externref encodings
#घोषणा d_waEncode_ref                      0x64
#घोषणा d_waEncode_refNull                  0x63

// indexed by M3ValueType: c_m3Type_count entries for the concrete types, plus
// one more for c_m3Type_unknown, which sits right after them
अटल स्थिर अक्षर* स्थिर c_waTypes[] = { "nil", "i32", "i64", "f32", "f64", "v128", "funcref", "externref", "exnref", "contref", "unknown" };
अटल स्थिर अक्षर* स्थिर c_waCompactTypes[] = { "_", "i", "I", "f", "F", "V", "r", "R", "e", "c", "?" };


#अगर d_m3VerboseErrorMessages

M3_परिणाम m3Error (M3_परिणाम i_परिणाम, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  स्थिर अक्षर* स्थिर i_file, u32 i_पंक्ति_अंक, स्थिर अक्षर* स्थिर i_errorMessage, ...);

#  define _m3Error(परिणाम_2, RT, MOD, FUN, खाता, पंक्ति, FORMAT, ...) \
            m3Error (परिणाम_2, RT, MOD, FUN, खाता, पंक्ति, FORMAT, ##__VA_ARGS__)

#अन्यथा
#  define _m3Error(परिणाम_2, RT, MOD, FUN, खाता, पंक्ति, FORMAT, ...) (परिणाम_2)
#पूर्ण_अगर

#घोषणा ErrorRuntime(परिणाम_2, RUNTIME, FORMAT, ...)      _m3Error (परिणाम_2, RUNTIME, शून्य, शून्य,  __खाता__, __LINE__, FORMAT, ##__VA_ARGS__)
#घोषणा ErrorModule(परिणाम_2, MOD, FORMAT, ...)           _m3Error (परिणाम_2, MOD->runtime, MOD, शून्य,  __खाता__, __LINE__, FORMAT, ##__VA_ARGS__)
#घोषणा ErrorCompile(परिणाम_2, COMP, FORMAT, ...)         _m3Error (परिणाम_2, COMP->runtime, COMP->module, शून्य, __खाता__, __LINE__, FORMAT, ##__VA_ARGS__)

#अगर d_m3GuardedMemory

// How much address space a linear memory's bytes are given: everything a Wasm access
// can name. A 32-bit address plus a 32-bit memarg offset reaches just under 2^33, and
// the access itself a few bytes further, which the page of slack in a slot covers.
#  define d_m3_Guarded_तथ्य_Bytes  (UINT64_C(1) << 33)

// Take one slot of the guarded arena, or NULL when the arena has none left or could
// not be reserved at all. What comes back is the base of the slot: the memory's
// M3MemoryHeader sits at the end of its first page, and the bytes the guest addresses
// begin on the page after that.
व्योम*           Guard_TakeSlot (व्योम);

// The header of a slot, which is also what the guest's memory starts right after
M3MemoryHeader* Guard_SlotHeader (व्योम* i_slot);

// Make the first i_dataBytes of the slot's data reachable. Only ever asked to grow,
// and asking for what is already there costs nothing.
bool            Guard_CommitSlot (व्योम* i_slot, माप_प्रकार i_तथ्य_Bytes);

// Give a slot back, dropping everything committed in it. The next taker gets it
// zeroed, as a fresh linear memory has to be.
व्योम            Guard_GiveSlot (व्योम* i_slot);

// The whole arena, for a fault to be held against - see m3_HostProtectedCall. Answers
// (NULL, 0) before anything has asked for a slot.
व्योम            Guard_ArenaRange (व्योम** o_निचला, माप_प्रकार* o_bytes);

#पूर्ण_अगर // d_m3GuardedMemory

#अगर d_m3_Max_Native_ढेर > 0
// The native-stack low-water mark for a top-level call that came in at i_stackPtr,
// clamped to what the calling thread's stack really is where that can be measured.
// See d_m3MaxNativeStack and d_m3StackLimitEnter.
व्योम* m3_Native_ढेर_Limit (व्योम* i_ढेर_Ptr, माप_प्रकार i_budget);
#पूर्ण_अगर

#अगर d_m3_Log_Native_ढेर
व्योम m3_ढेर_Check_Init ();
व्योम m3_ढेर_Check ();
पूर्णांक  m3_ढेर_Get_Max ();
#अन्यथा
#  define     m3_ढेर_Check_Init()
#  define     m3_ढेर_Check()
#  define     m3_ढेर_Get_Max()         0
#पूर्ण_अगर

#अगर d_m3LogTimestamps
#  define     PRIts                   "%llu"
uint64_t m3_GetTimestamp ();
#अन्यथा
#  define     PRIts                   "%s"
#  define     m3_GetTimestamp()       ""
#पूर्ण_अगर

व्योम  m3_Abort (स्थिर अक्षर* message);
व्योम* m3_Malloc_Impl (माप_प्रकार i_आकार);
व्योम* m3_Realloc_Impl (व्योम* i_ptr, माप_प्रकार i_new_आकार, माप_प्रकार i_old_आकार);
व्योम  m3_Free_Impl (व्योम* i_ptr);
व्योम* m3_CopyMem (स्थिर व्योम* i_from, माप_प्रकार i_आकार);

#अगर d_m3LogHeapOps

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

अटल अंतरभूत
व्योम* m3_AllocStruct_Impl (ccstr_t name, माप_प्रकार i_आकार)
{
    व्योम* परिणाम = m3_Malloc_Impl(i_आकार);
    ख_लिखो(मानक_त्रुटि, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_आकार, परिणाम);
    वापस परिणाम;
}

अटल अंतरभूत
व्योम* m3_AllocArray_Impl (ccstr_t name, माप_प्रकार i_अंक, माप_प्रकार i_आकार)
{
    व्योम* परिणाम = m3_Malloc_Impl(i_आकार * i_अंक);
    ख_लिखो(मानक_त्रुटि, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_आकार, i_अंक, परिणाम);
    वापस परिणाम;
}

अटल अंतरभूत
व्योम* m3_ReallocArray_Impl (ccstr_t name, व्योम* i_ptr_old, माप_प्रकार i_अंक_new, माप_प्रकार i_अंक_old, माप_प्रकार i_आकार)
{
    व्योम* परिणाम = m3_Realloc_Impl(i_ptr_old, i_आकार * i_अंक_new, i_आकार * i_अंक_old);
    ख_लिखो(मानक_त्रुटि, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_आकार, i_अंक_new, परिणाम, i_अंक_old, i_ptr_old);
    वापस परिणाम;
}

अटल अंतरभूत
व्योम* m3_Malloc (ccstr_t name, माप_प्रकार i_आकार)
{
    व्योम* परिणाम = m3_Malloc_Impl(i_आकार);
    ख_लिखो(मानक_त्रुटि, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_आकार, परिणाम);
    वापस परिणाम;
}
अटल अंतरभूत
व्योम* m3_Realloc (ccstr_t name, व्योम* i_ptr, माप_प्रकार i_new_आकार, माप_प्रकार i_old_आकार)
{
    व्योम* परिणाम = m3_Realloc_Impl(i_ptr, i_new_आकार, i_old_आकार);
    ख_लिखो(मानक_त्रुटि, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_new_आकार, परिणाम, i_old_आकार, i_ptr);
    वापस परिणाम;
}

#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, माप (STRUCT))
#  define     m3_AllocArray(STRUCT, अंक)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, अंक, माप (STRUCT))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (व्योम *)(PTR), (NEW), (OLD), माप (STRUCT))
#  define     m3_Free(P)                              करो { व्योम* p = (व्योम*)(P);                                  \
                                                          अगर (p) { ख_लिखो(मानक_त्रुटि, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                          m3_Free_Impl (p); (P) = शून्य; } जबतक(0)
#अन्यथा
#  define     m3_Malloc(NAME, आकार_2)                   m3_Malloc_Impl(आकार_2)
#  define     m3_Realloc(NAME, PTR, NEW, OLD)         m3_Realloc_Impl(PTR, NEW, OLD)
#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_Malloc_Impl (माप (STRUCT))
#  define     m3_AllocArray(STRUCT, अंक)              (STRUCT *)m3_Malloc_Impl (माप (STRUCT) * (अंक))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_Realloc_Impl ((व्योम *)(PTR), माप (STRUCT) * (NEW), माप (STRUCT) * (OLD))
#  define     m3_Free(P)                              करो { m3_Free_Impl ((व्योम*)(P)); (P) = शून्य; } जबतक(0)
#पूर्ण_अगर

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

#अगर d_m3HasTypedRefs

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

अटल अंतरभूत
bool IsSpelledRefType (m3type_t i_type)
{
    वापस (i_type & d_m3Type_ref) != 0;
}
अटल अंतरभूत
bool IsNullableRef (m3type_t i_type)
{
    वापस (i_type & d_m3Type_refNonNull) == 0;
}
अटल अंतरभूत
u32 HeapTypeOf (m3type_t i_type)
{
    वापस IsSpelledRefType(i_type) ? (i_type & d_m3Type_heapMask) : d_m3Type_heapAbstract;
}

#अन्यथा // no spelled-out reference types, so every value type is its own storage
      // type and subtyping collapses back to equality

अटल अंतरभूत
u8 BaseTypeOf (m3type_t i_type)
{
    वापस (u8)i_type;
}
अटल अंतरभूत
bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    वापस i_sub == i_super;
}
अटल अंतरभूत
bool IsSpelledRefType (m3type_t i_type)
{
    (व्योम)i_type;
    वापस false;
}
अटल अंतरभूत
bool IsNullableRef (m3type_t i_type)
{
    (व्योम)i_type;
    वापस true;
}

#पूर्ण_अगर // d_m3HasTypedRefs

M3_परिणाम NormalizeType (u8* o_type, i8 i_convolutedWasmType);

// These all speak in terms of how a value is stored, so they reduce a
// spelled-out reference type to its funcref / externref base first.
bool     IsIntType (m3type_t i_wasmType);
bool     IsFpType (m3type_t i_wasmType);
bool     IsRefType (m3type_t i_m3Type);
bool     Is64BitType (m3type_t i_m3Type);
u32      आकार_Of_Type (m3type_t i_m3Type);

M3_परिणाम Read_u64 (u64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम Read_u32 (u32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
#अगर d_m3ImplementFloat
M3_परिणाम Read_f64 (f64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम Read_f32 (f32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
#पूर्ण_अगर
M3_परिणाम Read_u8 (u8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम Read_opcode (m3opcode_t* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);

M3_परिणाम ReadLebUnsigned (u64* o_मूल्य, u32 i_max_अंक_Bits, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम ReadLebSigned (i64* o_मूल्य, u32 i_max_अंक_Bits, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम ReadLEB_u32 (u32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);

// The memarg immediate of a load or store: an alignment hint, then a static
// offset. Multi-memory reinterprets the alignment as a bitfield -- bit 6 says a
// memory index sits between the two -- so the hint is returned with that bit
// masked off and the index defaults to 0 when it is clear.
M3_परिणाम ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम ReadLEB_u7 (u8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम ReadLEB_i7 (i8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम ReadLEB_i32 (i32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम ReadLEB_i64 (i64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end);
M3_परिणाम Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end);

#अगर d_m3HasSnapshots
// XXH64, in m3_xxh64.c. Initialize a state, feed it the input in any number of
// pieces, then read the digest out. The digest depends only on the bytes fed in
// - not on how they were split up, nor on the host's byte order, pointer width
// or compiler - and matches the reference xxHash implementation, so anything
// else that speaks XXH64 computes the same value. Reading the digest does not
// consume the state.
प्रकार काष्ठा Xxh64 {
    u64 acc[4];              // the four accumulator lanes; acc[2] still holds the seed
    u64 योग_लंबाई;         // input bytes seen so far
    u8  बफर[32];          // input past the last stripe the accumulators took
    u32 बफर_लंबाई;
} Xxh64;

व्योम Xxh64_Init (Xxh64* o_state, u64 i_seed);
व्योम Xxh64_Update (Xxh64* io_state, स्थिर व्योम* i_तथ्य, माप_प्रकार i_आकार);
u64  Xxh64_Digest (स्थिर Xxh64* i_state);
#पूर्ण_अगर // d_m3HasSnapshots

cstr_t SPrint_मूल्य (व्योम* i_मूल्य, m3type_t i_type);
माप_प्रकार SPrintArg (अक्षर* o_string, माप_प्रकार i_string_बफर_आकार, voidptr_t i_sp, m3type_t i_type);

व्योम   ReportError (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_errorMessage, ccstr_t i_file, u32 i_पंक्ति_अंक);

#अगर d_m3RecordBacktraces
व्योम धकेलो_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc);
व्योम FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function);
व्योम ClearBacktrace (IM3Runtime io_runtime);
#पूर्ण_अगर

d_m3EndExternC

#पूर्ण_अगर // m3_core_h
