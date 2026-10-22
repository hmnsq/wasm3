//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#اگر_غیر_تعریف_شدہ GNU_ماخذ
#  define GNU_ماخذ 1
#اگر_ختم

#تعریف_کرو M3_IMPLEMENT_ERROR_لڑیاں
#شامل "m3_config.h"
#شامل "wasm3.h"

#شامل "m3_core.h"
#شامل "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#شامل "m3_host.h"

#اگر d_m3HasWin32Host
#  include "m3_host_win32.h"
#وگرنہ_اگر d_m3HasPosixHost
#  include "m3_host_posix.h"
#ورنہ
#  include "m3_host_none.h"
#اگر_ختم

خالی m3_Abort (ثابت حرف* پیغام)
{
#اگر_تعریف_شدہ DEBUG
    fprintf(stderr, "Error: %s\n", پیغام);
#اگر_ختم
    abort();
}

M3_WEAK
M3_نتیجہ m3_Yield ()
{
    واپس m3Err_none;
}

#اگر d_m3LogTimestamps

#  include <time.h>

#  define SEC_مقصد_US(sec) ((sec)*1000000)
#  define NS_مقصد_US(ns)    ((ns)/1000)

ساکن uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    اگر (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    ساخت timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_مقصد_US((uint64_t)ts.tv_sec) + NS_مقصد_US((uint64_t)ts.tv_nsec);
    واپس us - initial_ts;
}

#اگر_ختم

#اگر d_m3_Fixed_ڈھیر

ساکن u8        fixed_ڈھیر[d_m3_Fixed_ڈھیر];
ساکن u8*       fixed_ڈھیر_Ptr  = fixed_ڈھیر;
ساکن u8* ثابت fixed_ڈھیر_انتہا  = fixed_ڈھیر + d_m3_Fixed_ڈھیر;
ساکن u8*       fixed_ڈھیر_آخری = تہی;

#  اگر d_m3_Fixed_ڈھیر_Align > 1
#    define ڈھیر_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_ڈھیر_Align-1)) & ~ (d_m3_Fixed_ڈھیر_Align-1));
#  ورنہ
#    define ڈھیر_ALIGN_PTR(P)
#  endif

خالی* m3_Malloc_Impl (size_t i_سائز)
{
    u8* ptr = fixed_ڈھیر_Ptr;

    fixed_ڈھیر_Ptr += i_سائز;
    ڈھیر_ALIGN_PTR(fixed_ڈھیر_Ptr);

    اگر (fixed_ڈھیر_Ptr >= fixed_ڈھیر_انتہا) {
        واپس تہی;
    }

    memset(ptr, 0x0, i_سائز);
    fixed_ڈھیر_آخری = ptr;

    واپس ptr;
}

خالی m3_Free_Impl (خالی* i_ptr)
{
    // Handle the last chunk
    اگر (i_ptr && i_ptr == fixed_ڈھیر_آخری) {
        fixed_ڈھیر_Ptr  = fixed_ڈھیر_آخری;
        fixed_ڈھیر_آخری = تہی;
    } ورنہ {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

خالی* m3_Realloc_Impl (خالی* i_ptr, size_t i_new_سائز, size_t i_old_سائز)
{
    اگر (M3_UNLIKELY(i_new_سائز == i_old_سائز)) {
        واپس i_ptr;
    }

    خالی* newPtr;

    // Handle the last chunk
    اگر (i_ptr && i_ptr == fixed_ڈھیر_آخری) {
        fixed_ڈھیر_Ptr = fixed_ڈھیر_آخری + i_new_سائز;
        ڈھیر_ALIGN_PTR(fixed_ڈھیر_Ptr);
        اگر (fixed_ڈھیر_Ptr >= fixed_ڈھیر_انتہا) {
            واپس تہی;
        }
        newPtr = i_ptr;
    } ورنہ {
        newPtr = m3_Malloc_Impl(i_new_سائز);
        اگر (!newPtr) {
            واپس تہی;
        }
        اگر (i_ptr) {
            حافظہ_نقل(newPtr, i_ptr, i_old_سائز);
        }
    }

    اگر (i_new_سائز > i_old_سائز) {
        memset((u8*)newPtr + i_old_سائز, 0x0, i_new_سائز - i_old_سائز);
    }

    واپس newPtr;
}

#ورنہ

خالی* m3_Malloc_Impl (size_t i_سائز)
{
    واپس صفر_حافظہ_دو(i_سائز, 1);
}

خالی m3_Free_Impl (خالی* io_ptr)
{
    آزاد(io_ptr);
}

خالی* m3_Realloc_Impl (خالی* i_ptr, size_t i_new_سائز, size_t i_old_سائز)
{
    اگر (M3_UNLIKELY(i_new_سائز == i_old_سائز)) {
        واپس i_ptr;
    }

    خالی* newPtr = realloc(i_ptr, i_new_سائز);

    اگر (M3_LIKELY(newPtr)) {
        اگر (i_new_سائز > i_old_سائز) {
            memset((u8*)newPtr + i_old_سائز, 0x0, i_new_سائز - i_old_سائز);
        }
        واپس newPtr;
    }
    واپس تہی;
}

#اگر_ختم

خالی* m3_CopyMem (ثابت خالی* i_from, size_t i_سائز)
{
    خالی* ptr = m3_Malloc("CopyMem", i_سائز);
    اگر (ptr) {
        حافظہ_نقل(ptr, i_from, i_سائز);
    }
    واپس ptr;
}

//--------------------------------------------------------------------------------------------

#اگر d_m3_Guarded_حافظہ

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ساکن u8*    g_guardArena;
ساکن size_t g_guard_Arena_بائٹیں;
ساکن size_t g_guard_خانہ_بائٹیں;
ساکن size_t g_guard_صفحہ_سائز;
ساکن u32    g_guard_خانہ_گنتی;
ساکن bool   g_guard_خانہ_لیا_گیا[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
ساکن
bool Guard_Reserve (خالی)
{
    اگر (g_guardArena) {
        واپس true;
    }

    size_t صفحہ_سائز = m3_Host_صفحہ_سائز();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t خانہ_بائٹیں = 2 * صفحہ_سائز + (size_t)d_m3_Guarded_ڈیٹا_بائٹیں;

    برائے (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        خالی* بنیاد = m3_HostReserve(خانہ_بائٹیں * slots);

        اگر (بنیاد) {
            g_guardArena      = (u8*)بنیاد;
            g_guard_Arena_بائٹیں = خانہ_بائٹیں * slots;
            g_guard_خانہ_بائٹیں  = خانہ_بائٹیں;
            g_guard_صفحہ_سائز   = صفحہ_سائز;
            g_guard_خانہ_گنتی  = slots;
            واپس true;
        }
    }

    واپس false;
}

خالی* Guard_Take_خانہ (خالی)
{
    اگر (not Guard_Reserve()) {
        واپس تہی;
    }

    برائے (u32 i = 0; i < g_guard_خانہ_گنتی; ++i) {
        اگر (g_guard_خانہ_لیا_گیا[i]) {
            جاری;
        }

        u8* خانہ = g_guardArena + (size_t)i * g_guard_خانہ_بائٹیں;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        اگر (not m3_HostCommit(خانہ, g_guard_صفحہ_سائز)) {
            واپس تہی;
        }

        g_guard_خانہ_لیا_گیا[i] = true;
        واپس خانہ;
    }

    واپس تہی;
}

M3_حافظہ_سرخی* Guard_خانہ_سرخی (خالی* i_خانہ)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* ڈیٹا = (u8*)i_خانہ + g_guard_صفحہ_سائز;

    واپس (M3_حافظہ_سرخی*)(ڈیٹا - حجم(M3_حافظہ_سرخی));
}

bool Guard_Commit_خانہ (خالی* i_خانہ, size_t i_ڈیٹا_بائٹیں)
{
    size_t rounded = (i_ڈیٹا_بائٹیں + g_guard_صفحہ_سائز - 1) / g_guard_صفحہ_سائز * g_guard_صفحہ_سائز;

    واپس m3_HostCommit((u8*)i_خانہ + g_guard_صفحہ_سائز, rounded);
}

خالی Guard_Give_خانہ (خالی* i_خانہ)
{
    اگر (i_خانہ == تہی) {
        واپس;
    }

    size_t index = (size_t)((u8*)i_خانہ - g_guardArena) / g_guard_خانہ_بائٹیں;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    اگر (m3_HostDecommit(i_خانہ, g_guard_خانہ_بائٹیں) and index < g_guard_خانہ_گنتی) {
        g_guard_خانہ_لیا_گیا[index] = false;
    }
}

خالی Guard_ArenaRange (خالی** o_کم, size_t* o_بائٹیں)
{
    *o_کم   = g_guardArena;
    *o_بائٹیں = g_guard_Arena_بائٹیں;
}

#اگر_ختم // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#اگر d_m3_Max_Native_ڈھیر > 0

// Where Wasm execution has to stop to keep the native stack intact, given the stack
// pointer a top-level call came in on. d_m3MaxNativeStack is the budget asked for;
// what comes back is that budget or what the thread actually has, whichever runs out
// first.
//
// A build that cannot measure the stack gets the budget unchanged, which is what
// every build did before there was anything to measure. Where it can, the margin
// below the mark is d_m3NativeStackMargin, or a quarter of the stack when the stack
// is too small to spare that much - the point is to leave the trap path somewhere to
// run, and on a small stack a fixed 64KiB would be most of it.
خالی* m3_Native_ڈھیر_حد_بندی (خالی* i_ڈھیر_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ڈھیر_Ptr;

#  اگر M3_HAS_THREAD_LOCAL
    ساکن M3_THREAD_LOCAL u8* بنیاد = تہی;
    اگر (بنیاد == تہی) {
        بنیاد = (u8*)m3_Host_ڈھیر_بنیاد();
    }
#  ورنہ
    u8* بنیاد = (u8*)m3_Host_ڈھیر_بنیاد();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    اگر (بنیاد and بنیاد < sp) {
        size_t دستیاب = (size_t)(sp - بنیاد);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ڈھیر_Margin), دستیاب / 4);

        i_budget = M3_MIN(i_budget, دستیاب - margin);
    }

    واپس sp - i_budget;
}

#اگر_ختم // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#اگر d_m3_Log_Native_ڈھیر

ساکن size_t ڈھیر_آغاز;
ساکن size_t ڈھیر_انتہا;

خالی m3_ڈھیر_Check_Init ()
{
    حرف ڈھیر;
    ڈھیر_انتہا = ڈھیر_آغاز = (size_t)&ڈھیر;
}

خالی m3_ڈھیر_Check ()
{
    حرف   ڈھیر;
    size_t addr = (size_t)&ڈھیر;

    size_t ڈھیر_انتہا_2 = ڈھیر_انتہا;

    ڈھیر_انتہا = M3_MIN(ڈھیر_انتہا, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

صحیح m3_ڈھیر_Get_Max ()
{
    واپس ڈھیر_آغاز - ڈھیر_انتہا;
}

#اگر_ختم

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_نتیجہ NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u8 type = -i_convolutedWasmType;

    اگر (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    ورنہ اگر (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } ورنہ اگر (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#اگر d_m3HasExceptionHandling
    ورنہ اگر (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
    ورنہ اگر (type == d_wa_Type_ڈھیر_cont or type == d_wa_Type_ڈھیر_nocont) {
        type = c_m3Type_contref;
    }
#اگر_ختم
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    ورنہ اگر (type < c_m3Type_i32 or type > c_m3Type_v128) {
        نتیجہ = m3Err_invalidTypeId;
    }

    *o_type = type;

    واپس نتیجہ;
}


#اگر d_m3HasTypedRefs

u8 بنیاد_Type_کا (m3type_t i_type)
{
    اگر (IsSpelledRefType(i_type)) {
        اگر (i_type & d_m3Type_refCont) {
            واپس c_m3Type_contref;
        }
        واپس (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    واپس (u8)i_type;
}


bool Is_Sub_Type_کا (m3type_t i_sub, m3type_t i_super)
{
    اگر (i_sub == i_super) {
        واپس true;
    }

    u8 بنیاد = بنیاد_Type_کا(i_sub);

    // only references have a subtype relation; every other type is invariant
    اگر (بنیاد != بنیاد_Type_کا(i_super) or not IsRefType(بنیاد)) {
        واپس false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    اگر (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        واپس false;
    }

#  اگر d_m3_Has_ڈھیر_Switching
    // nocont sits under every continuation type
    اگر (ڈھیر_Type_کا(i_sub) == d_m3_Type_ڈھیر_None) {
        واپس true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    واپس (ڈھیر_Type_کا(i_super) == d_m3_Type_ڈھیر_Abstract) or
           (ڈھیر_Type_کا(i_sub) == ڈھیر_Type_کا(i_super));
}

#اگر_ختم // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = بنیاد_Type_کا(i_type);
    واپس (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = بنیاد_Type_کا(i_type);
    واپس (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#اگر d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#اگر_ختم
#اگر d_m3_Has_ڈھیر_Switching
            or i_m3Type == c_m3Type_contref
#اگر_ختم
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = بنیاد_Type_کا(i_type);
    واپس (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_ثنائی_ہندسہ_Type (m3type_t i_type)
{
    u8 i_m3Type = بنیاد_Type_کا(i_type);

    اگر (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        واپس true;
    } ورنہ اگر (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        واپس false;
    } ورنہ {
        واپس (حجم(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 سائز_کا_Type (m3type_t i_type)
{
    u8 i_m3Type = بنیاد_Type_کا(i_type);

    اگر (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        واپس حجم(i32);
    }

    اگر (IsRefType(i_m3Type)) {
        واپس حجم(خالی*);
    }

    واپس حجم(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_نتیجہ Read_u64 (u64* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    ثابت u8* ptr = *io_بائٹیں;
    ptr += حجم(u64);

    اگر (M3_LIKELY(ptr <= i_انتہا)) {
        حافظہ_نقل(o_قدر, *io_بائٹیں, حجم(u64));
        M3_BSWAP_u64(*o_قدر);
        *io_بائٹیں = ptr;
        واپس m3Err_none;
    } ورنہ {
        واپس m3Err_wasmUnderrun;
    }
}


M3_نتیجہ Read_u32 (u32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    ثابت u8* ptr = *io_بائٹیں;
    ptr += حجم(u32);

    اگر (M3_LIKELY(ptr <= i_انتہا)) {
        حافظہ_نقل(o_قدر, *io_بائٹیں, حجم(u32));
        M3_BSWAP_u32(*o_قدر);
        *io_بائٹیں = ptr;
        واپس m3Err_none;
    } ورنہ {
        واپس m3Err_wasmUnderrun;
    }
}

#اگر d_m3ImplementFloat

M3_نتیجہ Read_f64 (f64* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    ثابت u8* ptr = *io_بائٹیں;
    ptr += حجم(f64);

    اگر (M3_LIKELY(ptr <= i_انتہا)) {
        حافظہ_نقل(o_قدر, *io_بائٹیں, حجم(f64));
        M3_BSWAP_f64(*o_قدر);
        *io_بائٹیں = ptr;
        واپس m3Err_none;
    } ورنہ {
        واپس m3Err_wasmUnderrun;
    }
}


M3_نتیجہ Read_f32 (f32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    ثابت u8* ptr = *io_بائٹیں;
    ptr += حجم(f32);

    اگر (M3_LIKELY(ptr <= i_انتہا)) {
        حافظہ_نقل(o_قدر, *io_بائٹیں, حجم(f32));
        M3_BSWAP_f32(*o_قدر);
        *io_بائٹیں = ptr;
        واپس m3Err_none;
    } ورنہ {
        واپس m3Err_wasmUnderrun;
    }
}

#اگر_ختم

M3_نتیجہ Read_u8 (u8* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    ثابت u8* ptr = *io_بائٹیں;

    اگر (M3_LIKELY(ptr < i_انتہا)) {
        *o_قدر  = *ptr;
        *io_بائٹیں = ptr + 1;

        واپس m3Err_none;
    } ورنہ {
        واپس m3Err_wasmUnderrun;
    }
}

M3_نتیجہ Read_opcode (m3opcode_t* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    ثابت u8* ptr = *io_بائٹیں;

    اگر (M3_LIKELY(ptr < i_انتہا)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_قدر  = opcode;
        *io_بائٹیں = ptr;

        واپس m3Err_none;
    } ورنہ {
        واپس m3Err_wasmUnderrun;
    }
}


M3_نتیجہ ReadLebUnsigned (u64* o_قدر, u32 i_max_عدد_بٹ, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_wasmUnderrun;

    u64 قدر = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بائٹیں;

    جب (ptr < i_انتہا) {
        u64 byte = *(ptr++);

        قدر |= ((byte & 0x7f) << shift);
        shift += 7;

        اگر (M3_LIKELY((byte & 0x80) == 0)) {
            نتیجہ = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            اگر (M3_UNLIKELY(shift > i_max_عدد_بٹ)) {
                u32 عدد_مستعمل_بٹ = i_max_عدد_بٹ + 7 - shift;

                اگر (byte >> عدد_مستعمل_بٹ) {
                    نتیجہ = m3_Err_leb_حد_سے_تجاوز;
                }
            }
            توڑو;
        }

        اگر (M3_UNLIKELY(shift >= i_max_عدد_بٹ)) {
            نتیجہ = m3_Err_leb_حد_سے_تجاوز;
            توڑو;
        }
    }

    *o_قدر  = قدر;
    *io_بائٹیں = ptr;

    واپس نتیجہ;
}


M3_نتیجہ ReadLebSigned (i64* o_قدر, u32 i_max_عدد_بٹ, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_wasmUnderrun;

    i64 قدر = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بائٹیں;

    جب (ptr < i_انتہا) {
        u64 byte = *(ptr++);

        قدر |= ((byte & 0x7f) << shift);
        shift += 7;

        اگر (M3_LIKELY((byte & 0x80) == 0)) {
            نتیجہ = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            اگر (M3_UNLIKELY(shift > i_max_عدد_بٹ)) {
                u32 عدد_مستعمل_بٹ = i_max_عدد_بٹ + 7 - shift;
                u8  sign_بٹ    = (u8)((0x7f << (عدد_مستعمل_بٹ - 1)) & 0x7f);
                u8  بٹ        = (u8)(byte & sign_بٹ);

                اگر (بٹ != 0 and بٹ != sign_بٹ) {
                    نتیجہ = m3_Err_leb_حد_سے_تجاوز;
                }
            }
            اگر ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                قدر |= (~extend << shift);
            }

            توڑو;
        }

        اگر (M3_UNLIKELY(shift >= i_max_عدد_بٹ)) {
            نتیجہ = m3_Err_leb_حد_سے_تجاوز;
            توڑو;
        }
    }

    *o_قدر  = قدر;
    *io_بائٹیں = ptr;

    واپس نتیجہ;
}


M3_نتیجہ ReadLEB_u32 (u32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    u64      قدر;
    M3_نتیجہ نتیجہ = ReadLebUnsigned(&قدر, 32, io_بائٹیں, i_انتہا);

    *o_قدر = (u32)قدر;

    واپس نتیجہ;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#تعریف_کرو d_mem_Arg_Has_حافظہ_Idx    0x40u

M3_نتیجہ Read_حافظہ_Arg (u32* o_align, u32* o_حافظہ_Idx, u64* o_انحراف, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ;

    u32 align;
    نتیجہ = ReadLEB_u32(&align, io_بائٹیں, i_انتہا);
    اگر (نتیجہ) {
        واپس نتیجہ;
    }

    *o_حافظہ_Idx = 0;

#اگر d_m3_Has_Multi_حافظہ
    اگر (align & d_mem_Arg_Has_حافظہ_Idx) {
        align &= ~d_mem_Arg_Has_حافظہ_Idx;

        نتیجہ = ReadLEB_u32(o_حافظہ_Idx, io_بائٹیں, i_انتہا);
        اگر (نتیجہ) {
            واپس نتیجہ;
        }
    }
#اگر_ختم

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    واپس ReadLebUnsigned(o_انحراف, 64, io_بائٹیں, i_انتہا);
}


M3_نتیجہ ReadLEB_u7 (u8* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    u64      قدر;
    M3_نتیجہ نتیجہ = ReadLebUnsigned(&قدر, 7, io_بائٹیں, i_انتہا);

    *o_قدر = (u8)قدر;

    واپس نتیجہ;
}


M3_نتیجہ ReadLEB_i7 (i8* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    i64      قدر;
    M3_نتیجہ نتیجہ = ReadLebSigned(&قدر, 7, io_بائٹیں, i_انتہا);

    *o_قدر = (i8)قدر;

    واپس نتیجہ;
}


M3_نتیجہ ReadLEB_i32 (i32* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    i64      قدر;
    M3_نتیجہ نتیجہ = ReadLebSigned(&قدر, 32, io_بائٹیں, i_انتہا);

    *o_قدر = (i32)قدر;

    واپس نتیجہ;
}


M3_نتیجہ ReadLEB_i64 (i64* o_قدر, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    i64      قدر;
    M3_نتیجہ نتیجہ = ReadLebSigned(&قدر, 64, io_بائٹیں, i_انتہا);

    *o_قدر = قدر;

    واپس نتیجہ;
}

#اگر d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ساکن
bool IsValidUtf8 (ثابت u8* i_ڈیٹا, u32 i_لمبائی)
{
    ثابت u8* ptr = i_ڈیٹا;
    ثابت u8* انتہا = i_ڈیٹا + i_لمبائی;

    جب (ptr < انتہا) {
        u8 b0 = *ptr++;

        اگر (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            جاری;
        } ورنہ اگر ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            اگر (b0 < 0xC2) {
                واپس false;       // overlong
            }
            اگر (ptr >= انتہا) {
                واپس false;
            }
            u8 b1 = *ptr++;
            اگر ((b1 & 0xC0) != 0x80) {
                واپس false;
            }
        } ورنہ اگر ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            اگر (ptr + 1 >= انتہا) {
                واپس false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            اگر ((b1 & 0xC0) != 0x80) {
                واپس false;
            }
            اگر ((b2 & 0xC0) != 0x80) {
                واپس false;
            }
            // reject overlong
            اگر (b0 == 0xE0 && b1 < 0xA0) {
                واپس false;
            }
            // reject surrogates U+D800..U+DFFF
            اگر (b0 == 0xED && b1 >= 0xA0) {
                واپس false;
            }
        } ورنہ اگر ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            اگر (b0 > 0xF4) {
                واپس false;       // above U+10FFFF
            }
            اگر (ptr + 2 >= انتہا) {
                واپس false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            اگر ((b1 & 0xC0) != 0x80) {
                واپس false;
            }
            اگر ((b2 & 0xC0) != 0x80) {
                واپس false;
            }
            اگر ((b3 & 0xC0) != 0x80) {
                واپس false;
            }
            // reject overlong
            اگر (b0 == 0xF0 && b1 < 0x90) {
                واپس false;
            }
            // reject above U+10FFFF
            اگر (b0 == 0xF4 && b1 > 0x8F) {
                واپس false;
            }
        } ورنہ {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            واپس false;
        }
    }

    واپس true;
}
#اگر_ختم // d_m3EnableValidation


M3_نتیجہ Read_utf8 (cstr_t* o_utf8, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    *o_utf8 = تہی;

    u32      utf8_لمبائی;
    M3_نتیجہ نتیجہ = ReadLEB_u32(&utf8_لمبائی, io_بائٹیں, i_انتہا);

    اگر (not نتیجہ) {
        اگر (utf8_لمبائی <= d_m3_Max_Sane_Utf8_لمبائی) {
            ثابت u8* ptr = *io_بائٹیں;
            اگر (utf8_لمبائی <= (size_t)(i_انتہا - ptr)) {
                ثابت u8* انتہا = ptr + utf8_لمبائی;
#اگر d_m3EnableValidation
                اگر (not IsValidUtf8(ptr, utf8_لمبائی)) {
                    *io_بائٹیں = انتہا;
                    واپس m3Err_wasmMalformed;
                }
#اگر_ختم // d_m3EnableValidation

                حرف* utf8 = (حرف*)m3_Malloc("UTF8", utf8_لمبائی + 1);

                اگر (utf8) {
                    حافظہ_نقل(utf8, ptr, utf8_لمبائی);
                    utf8[utf8_لمبائی] = 0;

                    *o_utf8 = utf8;
                } ورنہ {
                    نتیجہ = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_بائٹیں = انتہا;
            } ورنہ {
                نتیجہ = m3Err_wasmUnderrun;
            }
        } ورنہ {
            نتیجہ = m3Err_missingUTF8;
        }
    }

    واپس نتیجہ;
}

#اگر d_m3_ریکارڈ_Backtraces
u32 ڈھونڈو_Module_انحراف (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_رمز_صفحہ curr = i_runtime->صفحات_Open;

    bool صفحہ_ملا = false;

    جب (curr) {
        اگر (شامل_ہے_PC(curr, i_pc)) {
            صفحہ_ملا = true;
            توڑو;
        }
        curr = curr->info.اگلا;
    }

    اگر (!صفحہ_ملا) {
        curr = i_runtime->صفحات_بھرا;
        جب (curr) {
            اگر (شامل_ہے_PC(curr, i_pc)) {
                صفحہ_ملا = true;
                توڑو;
            }
            curr = curr->info.اگلا;
        }
    }

    اگر (صفحہ_ملا) {
        u32 نتیجہ = 0;

        bool pc_ملا = Map_PCTo_انحراف(curr, i_pc, &نتیجہ);
        (خالی)pc_ملا;                                                          d_m3Assert (pc_ملا);

        واپس نتیجہ;
    } ورنہ {
        واپس 0;
    }
}


خالی ڈالو_Backtrace_چوکھٹا (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    اگر (M3_UNLIKELY(io_runtime->backtrace.آخری_چوکھٹا == M3_BACKTRACE_TRUNCATED)) {
        واپس;
    }

    M3_Backtrace_چوکھٹا* new_چوکھٹا = m3_AllocStruct(M3_Backtrace_چوکھٹا);

    اگر (!new_چوکھٹا) {
        io_runtime->backtrace.آخری_چوکھٹا = M3_BACKTRACE_TRUNCATED;
        واپس;
    }

    new_چوکھٹا->module_انحراف = ڈھونڈو_Module_انحراف(io_runtime, i_pc);

    اگر (!io_runtime->backtrace.چوکھٹے || !io_runtime->backtrace.آخری_چوکھٹا) {
        io_runtime->backtrace.چوکھٹے = new_چوکھٹا;
    } ورنہ {
        io_runtime->backtrace.آخری_چوکھٹا->اگلا = new_چوکھٹا;
    }
    io_runtime->backtrace.آخری_چوکھٹا = new_چوکھٹا;
}


خالی FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    اگر (M3_UNLIKELY(io_runtime->backtrace.آخری_چوکھٹا == M3_BACKTRACE_TRUNCATED)) {
        واپس;
    }

    اگر (!io_runtime->backtrace.آخری_چوکھٹا) {
        واپس;
    }

    io_runtime->backtrace.آخری_چوکھٹا->function = i_function;
}


خالی ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_چوکھٹا* current_چوکھٹا = io_runtime->backtrace.چوکھٹے;
    جب (current_چوکھٹا) {
        M3_Backtrace_چوکھٹا* اگلا_چوکھٹا = current_چوکھٹا->اگلا;
        m3_Free(current_چوکھٹا);
        current_چوکھٹا = اگلا_چوکھٹا;
    }

    io_runtime->backtrace.چوکھٹے    = تہی;
    io_runtime->backtrace.آخری_چوکھٹا = تہی;
}
#اگر_ختم // d_m3RecordBacktraces
