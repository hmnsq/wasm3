//
//  m3_core.h
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_غیر_تعریف_شدہ m3_core_h
#تعریف_کرو m3_core_h

#شامل <stdio.h>
#شامل <stdlib.h>
#شامل <stdbool.h>
#شامل <string.h>
#شامل <assert.h>

#شامل "wasm3.h"
#شامل "m3_config.h"

#اگر defined(__cplusplus)
#  define d_m3BeginExternC     خارجی "C" {
#  define d_m3_انتہا_Extern_C       }
#ورنہ
#  define d_m3BeginExternC
#  define d_m3_انتہا_Extern_C
#اگر_ختم

d_m3BeginExternC

#تعریف_کرو d_m3ImplementFloat (d_m3HasFloat || d_m3NoFloatDynamic)

#اگر !defined(d_m3ShortTypesDefined)

قسمتعریف uint64_t u64;
قسمتعریف int64_t  i64;
قسمتعریف uint32_t u32;
قسمتعریف int32_t  i32;
قسمتعریف uint16_t u16;
قسمتعریف int16_t  i16;
قسمتعریف uint8_t  u8;
قسمتعریف int8_t   i8;

#  اگر d_m3ImplementFloat
قسمتعریف دوہرا f64;
قسمتعریف اعشاری  f32;
#  endif

#اگر_ختم // d_m3ShortTypesDefined

#تعریف_کرو PRIf32          "f"
#تعریف_کرو PRIf64          "lf"

قسمتعریف ثابت خالی*       m3ret_t;
قسمتعریف ثابت خالی*       voidptr_t;
قسمتعریف ثابت حرف*       cstr_t;
قسمتعریف ثابت حرف* ثابت ccstr_t;
قسمتعریف ثابت u8*         بائٹیں_t;
قسمتعریف ثابت u8* ثابت   cbytes_t;

قسمتعریف u16               m3opcode_t;
#اگر d_m3HasTypedRefs
قسمتعریف u16 m3type_t;       // a value type, see the d_m3Type_* bits below
#ورنہ
قسمتعریف u8 m3type_t;       // only plain M3ValueType values exist
#اگر_ختم

قسمتعریف i64 m3reg_t;

#اگر d_m3_Use32_ثنائی_ہندسہ_Slots
قسمتعریف u32 m3slot_t;
#ورنہ
قسمتعریف u64 m3slot_t;
#اگر_ختم

قسمتعریف m3slot_t* m3stack_t;

#تعریف_کرو c_io_خانہ_گنتی ((u16)(حجم(u64) / حجم(m3slot_t)))

قسمتعریف ثابت خالی* ثابت cvptr_t;

#اگر defined(DEBUG)

#  define d_m3Log(CATEGORY, FMT, ...)                لکھو (" %8s  |  " FMT, #CATEGORY, ##__VA_ARGS__);

#  اگر d_m3LogParse
#    define m3log_parse(CATEGORY, FMT, ...)          d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ورنہ
#    define m3log_parse(...) {}
#  endif

#  اگر d_m3LogCompile
#    define m3log_compile(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ورنہ
#    define m3log_compile(...) {}
#  endif

#  اگر d_m3LogEmit
#    define m3log_emit(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ورنہ
#    define m3log_emit(...) {}
#  endif

#  اگر d_m3_Log_رمز_صفحات
#    define m3log_رمز(CATEGORY, FMT, ...)           d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ورنہ
#    define m3log_رمز(...) {}
#  endif

#  اگر d_m3LogModule
#    define m3log_module(CATEGORY, FMT, ...)         d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ورنہ
#    define m3log_module(...) {}
#  endif

#  اگر d_m3LogRuntime
#    define m3log_runtime(CATEGORY, FMT, ...)        d_m3Log(CATEGORY, FMT, ##__VA_ARGS__)
#  ورنہ
#    define m3log_runtime(...) {}
#  endif

#  define m3log(CATEGORY, FMT, ...)                  m3log_##CATEGORY (CATEGORY, FMT "\n", ##__VA_ARGS__)
#ورنہ
#  define d_m3Log(CATEGORY, FMT, ...)                {}
#  define m3log(CATEGORY, FMT, ...)                  {}
#اگر_ختم


#اگر defined(ASSERTS) || (defined(DEBUG) && !defined(NASSERTS))
#  define d_m3Assert(ASS)  اگر (!(ASS)) { لکھو("Assertion failed at %s:%d : %s\n", __FILE__, __LINE__, #ASS); fflush(stdout); abort(); }
#ورنہ
#  define d_m3Assert(ASS)
#اگر_ختم

قسمتعریف خالی /*const*/*                رمز_t;
قسمتعریف رمز_t ثابت* /*__restrict__*/ pc_t;


// Sits immediately before the linear memory it describes, so m3MemData() is
// just (header + 1). The interpreter carries a pointer to one of these in the
// _mem pseudo-register, and reaches everything else about the memory - its page
// counts, its owning runtime - through the back-pointers here. 'memory' is what
// makes that possible without going through the runtime, which is the
// prerequisite for a runtime holding more than one memory.
قسمتعریف ساخت M3_حافظہ_سرخی {
    IM3Runtime       runtime;
    خالی*            max_ڈھیر;
    size_t           لمبائی;
    ساخت M3_حافظہ* حافظہ;         // the M3Memory this block belongs to
} M3_حافظہ_سرخی;

ساخت M3_رمز_Mapping_صفحہ;

قسمتعریف ساخت M3_رمز_صفحہ_سرخی {
    ساخت M3_رمز_صفحہ* اگلا;

    u32                سطر_اشاریہ;
    u32                عدد_سطریں;
    u32                sequence;       // this is just used for debugging; could be removed
    u32                usage_گنتی;

#اگر d_m3_ریکارڈ_Backtraces
    ساخت M3_رمز_Mapping_صفحہ* mapping;
#اگر_ختم // d_m3RecordBacktraces
} M3_رمز_صفحہ_سرخی;


#تعریف_کرو d_m3_رمز_صفحہ_Free_سطریں_حد      7+2       // max is: ReturnCallIndirect (op + 6 immediates) + 2 for bridge

#تعریف_کرو d_m3_Default_Mem_صفحہ_سائز              65536

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
#تعریف_کرو d_m3_پتہ_حد_بندی_بٹ                52
#تعریف_کرو d_m3_پتہ_حد_بندی                    (UINT64_C(1) << d_m3_پتہ_حد_بندی_بٹ)

#تعریف_کرو d_m3_Reg0_خانہ_Alias                   60000
#تعریف_کرو d_m3_Fp0_خانہ_Alias                    (d_m3_Reg0_خانہ_Alias + 2)

#اگر d_m3HasTypedRefs
// a heap type index has to fit in the spare bits of an m3type_t, see below
#  define d_m3_Max_Sane_Types_گنتی             4094
#ورنہ
// M3Environment.numFuncTypes is the u16 that hands out M3FuncType.canonicalIndex,
// so the count has to stay below 65536 here as well
#  define d_m3_Max_Sane_Types_گنتی             65500
#اگر_ختم
#تعریف_کرو d_m3_Max_Sane_Functions_گنتی           1000000
#تعریف_کرو d_m3_Max_Sane_Imports_گنتی             100000
#تعریف_کرو d_m3_Max_Sane_Exports_گنتی             100000
#تعریف_کرو d_m3_Max_Sane_Globals_گنتی             1000000
#تعریف_کرو d_m3_Max_Sane_لیبل_فہرست_گنتی                100000
#تعریف_کرو d_m3MaxSaneElementSegments          10000000
#تعریف_کرو d_m3_Max_Sane_ڈیٹا_Segments             100000
#تعریف_کرو d_m3_Max_Sane_جدول_سائز                10000000
#تعریف_کرو d_m3_Max_Sane_Memories_گنتی            10000
#اگر d_m3HasRefTypes
#  define d_m3_Max_Sane_جدول_گنتی             1000
#ورنہ
#  define d_m3_Max_Sane_جدول_گنتی             1           // MVP: a single table
#اگر_ختم
#تعریف_کرو d_m3_Max_Sane_Utf8_لمبائی               10000
#تعریف_کرو d_m3_Max_Sane_Function_Arg_Ret_گنتی      1000    // still insane, but whatever

#تعریف_کرو d_external_قسم_function             0
#تعریف_کرو d_external_قسم_جدول                1
#تعریف_کرو d_external_قسم_حافظہ               2
#تعریف_کرو d_external_قسم_global               3
#تعریف_کرو d_external_قسم_لیبل                  4

// compact-import-section: these stand where an externtype would, after an empty
// item name, and introduce a run of imports sharing one module name. 0x7f gives
// each item its own externtype, 0x7e shares a single one across the whole run.
#تعریف_کرو d_compact_Imports_فی_شے_Type        0x7f
#تعریف_کرو d_compactImports_sharedType         0x7e

// the negated wasm encodings of the abstract reference types, as NormalizeType()
// stores them
#تعریف_کرو d_waType_funcref                    16
#تعریف_کرو d_waType_externref                  17

// exception handling: exnref is wasm-encoded as -0x17
#تعریف_کرو d_waType_exnref                     23

// stack switching: cont type form is -0x23 (35); heap types: cont is -0x18 (24), nocont is -0x0b (11)
#تعریف_کرو d_waType_cont                       35
#تعریف_کرو d_wa_Type_ڈھیر_cont                  24
#تعریف_کرو d_wa_Type_ڈھیر_nocont                11

// function-references: a reference type spelled out as (ref ht) / (ref null ht),
// where the abstract heap types reuse the funcref and externref encodings
#تعریف_کرو d_waEncode_ref                      0x64
#تعریف_کرو d_waEncode_refNull                  0x63

// indexed by M3ValueType: c_m3Type_count entries for the concrete types, plus
// one more for c_m3Type_unknown, which sits right after them
ساکن ثابت حرف* ثابت c_waTypes[] = { "nil", "i32", "i64", "f32", "f64", "v128", "funcref", "externref", "exnref", "contref", "unknown" };
ساکن ثابت حرف* ثابت c_waCompactTypes[] = { "_", "i", "I", "f", "F", "V", "r", "R", "e", "c", "?" };


#اگر d_m3VerboseErrorMessages

M3_نتیجہ m3Error (M3_نتیجہ i_نتیجہ, IM3Runtime i_runtime, IM3Module i_module, IM3Function i_function,
                  ثابت حرف* ثابت i_file, u32 i_سطر_عدد, ثابت حرف* ثابت i_error_پیغام, ...);

#  define _m3Error(نتیجہ_2, RT, MOD, FUN, FILE, سطر, FORMAT, ...) \
            m3Error (نتیجہ_2, RT, MOD, FUN, FILE, سطر, FORMAT, ##__VA_ARGS__)

#ورنہ
#  define _m3Error(نتیجہ_2, RT, MOD, FUN, FILE, سطر, FORMAT, ...) (نتیجہ_2)
#اگر_ختم

#تعریف_کرو ErrorRuntime(نتیجہ_2, RUNTIME, FORMAT, ...)      _m3Error (نتیجہ_2, RUNTIME, تہی, تہی,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#تعریف_کرو ErrorModule(نتیجہ_2, MOD, FORMAT, ...)           _m3Error (نتیجہ_2, MOD->runtime, MOD, تہی,  __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)
#تعریف_کرو ErrorCompile(نتیجہ_2, COMP, FORMAT, ...)         _m3Error (نتیجہ_2, COMP->runtime, COMP->module, تہی, __FILE__, __LINE__, FORMAT, ##__VA_ARGS__)

#اگر d_m3_Guarded_حافظہ

// How much address space a linear memory's bytes are given: everything a Wasm access
// can name. A 32-bit address plus a 32-bit memarg offset reaches just under 2^33, and
// the access itself a few bytes further, which the page of slack in a slot covers.
#  define d_m3_Guarded_ڈیٹا_بائٹیں  (UINT64_C(1) << 33)

// Take one slot of the guarded arena, or NULL when the arena has none left or could
// not be reserved at all. What comes back is the base of the slot: the memory's
// M3MemoryHeader sits at the end of its first page, and the bytes the guest addresses
// begin on the page after that.
خالی*           Guard_Take_خانہ (خالی);

// The header of a slot, which is also what the guest's memory starts right after
M3_حافظہ_سرخی* Guard_خانہ_سرخی (خالی* i_خانہ);

// Make the first i_dataBytes of the slot's data reachable. Only ever asked to grow,
// and asking for what is already there costs nothing.
bool            Guard_Commit_خانہ (خالی* i_خانہ, size_t i_ڈیٹا_بائٹیں);

// Give a slot back, dropping everything committed in it. The next taker gets it
// zeroed, as a fresh linear memory has to be.
خالی            Guard_Give_خانہ (خالی* i_خانہ);

// The whole arena, for a fault to be held against - see m3_HostProtectedCall. Answers
// (NULL, 0) before anything has asked for a slot.
خالی            Guard_ArenaRange (خالی** o_کم, size_t* o_بائٹیں);

#اگر_ختم // d_m3GuardedMemory

#اگر d_m3_Max_Native_ڈھیر > 0
// The native-stack low-water mark for a top-level call that came in at i_stackPtr,
// clamped to what the calling thread's stack really is where that can be measured.
// See d_m3MaxNativeStack and d_m3StackLimitEnter.
خالی* m3_Native_ڈھیر_حد_بندی (خالی* i_ڈھیر_Ptr, size_t i_budget);
#اگر_ختم

#اگر d_m3_Log_Native_ڈھیر
خالی m3_ڈھیر_Check_Init ();
خالی m3_ڈھیر_Check ();
صحیح  m3_ڈھیر_Get_Max ();
#ورنہ
#  define     m3_ڈھیر_Check_Init()
#  define     m3_ڈھیر_Check()
#  define     m3_ڈھیر_Get_Max()         0
#اگر_ختم

#اگر d_m3LogTimestamps
#  define     PRIts                   "%llu"
uint64_t m3_GetTimestamp ();
#ورنہ
#  define     PRIts                   "%s"
#  define     m3_GetTimestamp()       ""
#اگر_ختم

خالی  m3_Abort (ثابت حرف* پیغام);
خالی* m3_Malloc_Impl (size_t i_سائز);
خالی* m3_Realloc_Impl (خالی* i_ptr, size_t i_new_سائز, size_t i_old_سائز);
خالی  m3_Free_Impl (خالی* i_ptr);
خالی* m3_CopyMem (ثابت خالی* i_from, size_t i_سائز);

#اگر d_m3_Log_ڈھیر_Ops

// Tracing format: timestamp;heap:OpCode;name;size(bytes);new items;new ptr;old items;old ptr

ساکن اندرونی
خالی* m3_AllocStruct_Impl (ccstr_t name, size_t i_سائز)
{
    خالی* نتیجہ = m3_Malloc_Impl(i_سائز);
    fprintf(stderr, PRIts ";heap:AllocStruct;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_سائز, نتیجہ);
    واپس نتیجہ;
}

ساکن اندرونی
خالی* m3_AllocArray_Impl (ccstr_t name, size_t i_عدد, size_t i_سائز)
{
    خالی* نتیجہ = m3_Malloc_Impl(i_سائز * i_عدد);
    fprintf(stderr, PRIts ";heap:AllocArr;%s;%zu;%zu;%p;;\n", m3_GetTimestamp(), name, i_سائز, i_عدد, نتیجہ);
    واپس نتیجہ;
}

ساکن اندرونی
خالی* m3_ReallocArray_Impl (ccstr_t name, خالی* i_ptr_old, size_t i_عدد_new, size_t i_عدد_old, size_t i_سائز)
{
    خالی* نتیجہ = m3_Realloc_Impl(i_ptr_old, i_سائز * i_عدد_new, i_سائز * i_عدد_old);
    fprintf(stderr, PRIts ";heap:ReallocArr;%s;%zu;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_سائز, i_عدد_new, نتیجہ, i_عدد_old, i_ptr_old);
    واپس نتیجہ;
}

ساکن اندرونی
خالی* m3_Malloc (ccstr_t name, size_t i_سائز)
{
    خالی* نتیجہ = m3_Malloc_Impl(i_سائز);
    fprintf(stderr, PRIts ";heap:AllocMem;%s;%zu;;%p;;\n", m3_GetTimestamp(), name, i_سائز, نتیجہ);
    واپس نتیجہ;
}
ساکن اندرونی
خالی* m3_Realloc (ccstr_t name, خالی* i_ptr, size_t i_new_سائز, size_t i_old_سائز)
{
    خالی* نتیجہ = m3_Realloc_Impl(i_ptr, i_new_سائز, i_old_سائز);
    fprintf(stderr, PRIts ";heap:ReallocMem;%s;;%zu;%p;%zu;%p\n", m3_GetTimestamp(), name, i_new_سائز, نتیجہ, i_old_سائز, i_ptr);
    واپس نتیجہ;
}

#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_AllocStruct_Impl  (#STRUCT, حجم (STRUCT))
#  define     m3_AllocArray(STRUCT, عدد)              (STRUCT *)m3_AllocArray_Impl   (#STRUCT, عدد, حجم (STRUCT))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_ReallocArray_Impl (#STRUCT, (خالی *)(PTR), (NEW), (OLD), حجم (STRUCT))
#  define     m3_Free(P)                              کرو { خالی* p = (خالی*)(P);                                  \
                                                          اگر (p) { fprintf(stderr, PRIts ";heap:FreeMem;;;;%p;\n", m3_GetTimestamp(), p); }     \
                                                          m3_Free_Impl (p); (P) = تہی; } جب(0)
#ورنہ
#  define     m3_Malloc(NAME, سائز_2)                   m3_Malloc_Impl(سائز_2)
#  define     m3_Realloc(NAME, PTR, NEW, OLD)         m3_Realloc_Impl(PTR, NEW, OLD)
#  define     m3_AllocStruct(STRUCT)                  (STRUCT *)m3_Malloc_Impl (حجم (STRUCT))
#  define     m3_AllocArray(STRUCT, عدد)              (STRUCT *)m3_Malloc_Impl (حجم (STRUCT) * (عدد))
#  define     m3_ReallocArray(STRUCT, PTR, NEW, OLD)  (STRUCT *)m3_Realloc_Impl ((خالی *)(PTR), حجم (STRUCT) * (NEW), حجم (STRUCT) * (OLD))
#  define     m3_Free(P)                              کرو { m3_Free_Impl ((خالی*)(P)); (P) = تہی; } جب(0)
#اگر_ختم

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

#اگر d_m3HasTypedRefs

#  define d_m3Type_ref                0x8000u
#  define d_m3Type_refNonNull         0x4000u
#  define d_m3Type_refExtern          0x2000u
#  define d_m3Type_refCont            0x1000u
#  define d_m3_Type_ڈھیر_نقاب           0x0FFFu
#  define d_m3_Type_ڈھیر_Abstract       0x0FFFu     // plain 'func', 'extern', or 'cont'
// 'nocont', the bottom of the continuation types: a reference to it is a
// reference to every one of them, which is what makes ref.null nocont usable
// wherever a particular continuation is expected. One below heapAbstract, and
// so out of reach of d_m3MaxSaneTypesCount.
#  define d_m3_Type_ڈھیر_None           0x0FFEu

// The storage type a value of this type occupies: a reference of any shape is
// just a funcref or an externref once the type checking is done with.
u8   بنیاد_Type_کا (m3type_t i_type);
bool Is_Sub_Type_کا (m3type_t i_sub, m3type_t i_super);

ساکن اندرونی
bool IsSpelledRefType (m3type_t i_type)
{
    واپس (i_type & d_m3Type_ref) != 0;
}
ساکن اندرونی
bool IsNullableRef (m3type_t i_type)
{
    واپس (i_type & d_m3Type_refNonNull) == 0;
}
ساکن اندرونی
u32 ڈھیر_Type_کا (m3type_t i_type)
{
    واپس IsSpelledRefType(i_type) ? (i_type & d_m3_Type_ڈھیر_نقاب) : d_m3_Type_ڈھیر_Abstract;
}

#ورنہ // no spelled-out reference types, so every value type is its own storage
      // type and subtyping collapses back to equality

ساکن اندرونی
u8 بنیاد_Type_کا (m3type_t i_type)
{
    واپس (u8)i_type;
}
ساکن اندرونی
bool Is_Sub_Type_کا (m3type_t i_sub, m3type_t i_super)
{
    واپس i_sub == i_super;
}
ساکن اندرونی
bool IsSpelledRefType (m3type_t i_type)
{
    (خالی)i_type;
    واپس false;
}
ساکن اندرونی
bool IsNullableRef (m3type_t i_type)
{
    (خالی)i_type;
    واپس true;
}

#اگر_ختم // d_m3HasTypedRefs

M3_نتیجہ NormalizeType (u8* o_type, i8 i_convolutedWasmType);

// These all speak in terms of how a value is stored, so they reduce a
// spelled-out reference type to its funcref / externref base first.
bool     IsIntType (m3type_t i_wasmType);
bool     IsFpType (m3type_t i_wasmType);
bool     IsRefType (m3type_t i_m3Type);
bool     Is64_ثنائی_ہندسہ_Type (m3type_t i_m3Type);
u32      سائز_کا_Type (m3type_t i_m3Type);

M3_نتیجہ Read_u64 (u64* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ Read_u32 (u32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
#اگر d_m3ImplementFloat
M3_نتیجہ Read_f64 (f64* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ Read_f32 (f32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
#اگر_ختم
M3_نتیجہ Read_u8 (u8* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ Read_opcode (m3opcode_t* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);

M3_نتیجہ ReadLebUnsigned (u64* o_قدر, u32 i_max_عدد_بٹ, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ ReadLebSigned (i64* o_قدر, u32 i_max_عدد_بٹ, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ ReadLEB_u32 (u32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);

// The memarg immediate of a load or store: an alignment hint, then a static
// offset. Multi-memory reinterprets the alignment as a bitfield -- bit 6 says a
// memory index sits between the two -- so the hint is returned with that bit
// masked off and the index defaults to 0 when it is clear.
M3_نتیجہ Read_حافظہ_Arg (u32* o_align, u32* o_حافظہ_Idx, u64* o_انحراف, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ ReadLEB_u7 (u8* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ ReadLEB_i7 (i8* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ ReadLEB_i32 (i32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ ReadLEB_i64 (i64* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);
M3_نتیجہ Read_utf8 (cstr_t* o_utf8, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا);

#اگر d_m3HasSnapshots
// XXH64, in m3_xxh64.c. Initialize a state, feed it the input in any number of
// pieces, then read the digest out. The digest depends only on the bytes fed in
// - not on how they were split up, nor on the host's byte order, pointer width
// or compiler - and matches the reference xxHash implementation, so anything
// else that speaks XXH64 computes the same value. Reading the digest does not
// consume the state.
قسمتعریف ساخت Xxh64 {
    u64 acc[4];              // the four accumulator lanes; acc[2] still holds the seed
    u64 کل_لمبائی;         // input bytes seen so far
    u8  بفر[32];          // input past the last stripe the accumulators took
    u32 بفر_لمبائی;
} Xxh64;

خالی Xxh64_Init (Xxh64* o_state, u64 i_بیج);
خالی Xxh64_Update (Xxh64* io_state, ثابت خالی* i_ڈیٹا, size_t i_سائز);
u64  Xxh64_Digest (ثابت Xxh64* i_state);
#اگر_ختم // d_m3HasSnapshots

cstr_t SPrint_قدر (خالی* i_قدر, m3type_t i_type);
size_t SPrintArg (حرف* o_string, size_t i_string_بفر_سائز, voidptr_t i_sp, m3type_t i_type);

خالی   ReportError (IM3Runtime io_runtime, IM3Module i_module, IM3Function i_function, ccstr_t i_error_پیغام, ccstr_t i_file, u32 i_سطر_عدد);

#اگر d_m3_ریکارڈ_Backtraces
خالی ڈالو_Backtrace_چوکھٹا (IM3Runtime io_runtime, pc_t i_pc);
خالی FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function);
خالی ClearBacktrace (IM3Runtime io_runtime);
#اگر_ختم

d_m3_انتہا_Extern_C

#اگر_ختم // m3_core_h
