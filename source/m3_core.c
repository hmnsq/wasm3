//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#جے_بنا_تعریف GNU_سروت
#  define GNU_سروت 1
#جے_انت

#تعریف_کرو M3_IMPLEMENT_ERROR_لڑیاں
#ضمن "m3_config.h"
#ضمن "wasm3.h"

#ضمن "m3_core.h"
#ضمن "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#ضمن "m3_host.h"

#إذا d_m3HasWin32Host
#  include "m3_host_win32.h"
#وإلاإذا d_m3HasPosixHost
#  include "m3_host_posix.h"
#وإلا
#  include "m3_host_none.h"
#جے_انت

فراغ m3_Abort (ثابت حرف* سنیہا)
{
#جے_تعریف_شدہ DEBUG
    fprintf(stderr, "Error: %s\n", سنیہا);
#جے_انت
    abort();
}

M3_WEAK
M3_نتیجہ m3_Yield ()
{
    أرجع m3Err_none;
}

#إذا d_m3LogTimestamps

#  include <time.h>

#  define SEC_منزل_US(sec) ((sec)*1000000)
#  define NS_منزل_US(ns)    ((ns)/1000)

ساكن uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    إذا (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    بنية timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_منزل_US((uint64_t)ts.tv_sec) + NS_منزل_US((uint64_t)ts.tv_nsec);
    أرجع us - initial_ts;
}

#جے_انت

#إذا d_m3_Fixed_ڈھیر

ساكن u8        fixed_ڈھیر[d_m3_Fixed_ڈھیر];
ساكن u8*       fixed_ڈھیر_Ptr  = fixed_ڈھیر;
ساكن u8* ثابت fixed_ڈھیر_سرا  = fixed_ڈھیر + d_m3_Fixed_ڈھیر;
ساكن u8*       fixed_ڈھیر_آخری = عدم;

#  إذا d_m3_Fixed_ڈھیر_Align > 1
#    define ڈھیر_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_ڈھیر_Align-1)) & ~ (d_m3_Fixed_ڈھیر_Align-1));
#  وإلا
#    define ڈھیر_ALIGN_PTR(P)
#  endif

فراغ* m3_Malloc_Impl (size_t i_ناپ)
{
    u8* ptr = fixed_ڈھیر_Ptr;

    fixed_ڈھیر_Ptr += i_ناپ;
    ڈھیر_ALIGN_PTR(fixed_ڈھیر_Ptr);

    إذا (fixed_ڈھیر_Ptr >= fixed_ڈھیر_سرا) {
        أرجع عدم;
    }

    memset(ptr, 0x0, i_ناپ);
    fixed_ڈھیر_آخری = ptr;

    أرجع ptr;
}

فراغ m3_Free_Impl (فراغ* i_ptr)
{
    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_ڈھیر_آخری) {
        fixed_ڈھیر_Ptr  = fixed_ڈھیر_آخری;
        fixed_ڈھیر_آخری = عدم;
    } وإلا {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_ناپ, size_t i_old_ناپ)
{
    إذا (M3_UNLIKELY(i_new_ناپ == i_old_ناپ)) {
        أرجع i_ptr;
    }

    فراغ* newPtr;

    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_ڈھیر_آخری) {
        fixed_ڈھیر_Ptr = fixed_ڈھیر_آخری + i_new_ناپ;
        ڈھیر_ALIGN_PTR(fixed_ڈھیر_Ptr);
        إذا (fixed_ڈھیر_Ptr >= fixed_ڈھیر_سرا) {
            أرجع عدم;
        }
        newPtr = i_ptr;
    } وإلا {
        newPtr = m3_Malloc_Impl(i_new_ناپ);
        إذا (!newPtr) {
            أرجع عدم;
        }
        إذا (i_ptr) {
            حافظہ_نقل(newPtr, i_ptr, i_old_ناپ);
        }
    }

    إذا (i_new_ناپ > i_old_ناپ) {
        memset((u8*)newPtr + i_old_ناپ, 0x0, i_new_ناپ - i_old_ناپ);
    }

    أرجع newPtr;
}

#وإلا

فراغ* m3_Malloc_Impl (size_t i_ناپ)
{
    أرجع صفر_حافظہ_دیو(i_ناپ, 1);
}

فراغ m3_Free_Impl (فراغ* io_ptr)
{
    حرر(io_ptr);
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_ناپ, size_t i_old_ناپ)
{
    إذا (M3_UNLIKELY(i_new_ناپ == i_old_ناپ)) {
        أرجع i_ptr;
    }

    فراغ* newPtr = realloc(i_ptr, i_new_ناپ);

    إذا (M3_LIKELY(newPtr)) {
        إذا (i_new_ناپ > i_old_ناپ) {
            memset((u8*)newPtr + i_old_ناپ, 0x0, i_new_ناپ - i_old_ناپ);
        }
        أرجع newPtr;
    }
    أرجع عدم;
}

#جے_انت

فراغ* m3_CopyMem (ثابت فراغ* i_from, size_t i_ناپ)
{
    فراغ* ptr = m3_Malloc("CopyMem", i_ناپ);
    إذا (ptr) {
        حافظہ_نقل(ptr, i_from, i_ناپ);
    }
    أرجع ptr;
}

//--------------------------------------------------------------------------------------------

#إذا d_m3_Guarded_حافظہ

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ساكن u8*    g_guardArena;
ساكن size_t g_guard_Arena_بائٹاں;
ساكن size_t g_guard_تھاں_بائٹاں;
ساكن size_t g_guard_ورقہ_ناپ;
ساكن u32    g_guard_تھاں_گنتی;
ساكن bool   g_guard_تھاں_لیا[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
ساكن
bool Guard_Reserve (فراغ)
{
    إذا (g_guardArena) {
        أرجع true;
    }

    size_t ورقہ_ناپ = m3_Host_ورقہ_ناپ();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t تھاں_بائٹاں = 2 * ورقہ_ناپ + (size_t)d_m3_Guarded_انکڑے_بائٹاں;

    لكل (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        فراغ* ادھار = m3_HostReserve(تھاں_بائٹاں * slots);

        إذا (ادھار) {
            g_guardArena      = (u8*)ادھار;
            g_guard_Arena_بائٹاں = تھاں_بائٹاں * slots;
            g_guard_تھاں_بائٹاں  = تھاں_بائٹاں;
            g_guard_ورقہ_ناپ   = ورقہ_ناپ;
            g_guard_تھاں_گنتی  = slots;
            أرجع true;
        }
    }

    أرجع false;
}

فراغ* Guard_Take_تھاں (فراغ)
{
    إذا (not Guard_Reserve()) {
        أرجع عدم;
    }

    لكل (u32 i = 0; i < g_guard_تھاں_گنتی; ++i) {
        إذا (g_guard_تھاں_لیا[i]) {
            استمر;
        }

        u8* تھاں = g_guardArena + (size_t)i * g_guard_تھاں_بائٹاں;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        إذا (not m3_HostCommit(تھاں, g_guard_ورقہ_ناپ)) {
            أرجع عدم;
        }

        g_guard_تھاں_لیا[i] = true;
        أرجع تھاں;
    }

    أرجع عدم;
}

M3_حافظہ_سرخی* Guard_تھاں_سرخی (فراغ* i_تھاں)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* انکڑے = (u8*)i_تھاں + g_guard_ورقہ_ناپ;

    أرجع (M3_حافظہ_سرخی*)(انکڑے - حجم(M3_حافظہ_سرخی));
}

bool Guard_Commit_تھاں (فراغ* i_تھاں, size_t i_انکڑے_بائٹاں)
{
    size_t rounded = (i_انکڑے_بائٹاں + g_guard_ورقہ_ناپ - 1) / g_guard_ورقہ_ناپ * g_guard_ورقہ_ناپ;

    أرجع m3_HostCommit((u8*)i_تھاں + g_guard_ورقہ_ناپ, rounded);
}

فراغ Guard_Give_تھاں (فراغ* i_تھاں)
{
    إذا (i_تھاں == عدم) {
        أرجع;
    }

    size_t index = (size_t)((u8*)i_تھاں - g_guardArena) / g_guard_تھاں_بائٹاں;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    إذا (m3_HostDecommit(i_تھاں, g_guard_تھاں_بائٹاں) and index < g_guard_تھاں_گنتی) {
        g_guard_تھاں_لیا[index] = false;
    }
}

فراغ Guard_ArenaRange (فراغ** o_کم, size_t* o_بائٹاں)
{
    *o_کم   = g_guardArena;
    *o_بائٹاں = g_guard_Arena_بائٹاں;
}

#جے_انت // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#إذا d_m3_Max_Native_ڈھیر > 0

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
فراغ* m3_Native_ڈھیر_حد_بندی (فراغ* i_ڈھیر_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ڈھیر_Ptr;

#  إذا M3_HAS_THREAD_LOCAL
    ساكن M3_THREAD_LOCAL u8* ادھار = عدم;
    إذا (ادھار == عدم) {
        ادھار = (u8*)m3_Host_ڈھیر_ادھار();
    }
#  وإلا
    u8* ادھار = (u8*)m3_Host_ڈھیر_ادھار();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    إذا (ادھار and ادھار < sp) {
        size_t دستیاب = (size_t)(sp - ادھار);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ڈھیر_Margin), دستیاب / 4);

        i_budget = M3_MIN(i_budget, دستیاب - margin);
    }

    أرجع sp - i_budget;
}

#جے_انت // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#إذا d_m3_Log_Native_ڈھیر

ساكن size_t ڈھیر_شروع;
ساكن size_t ڈھیر_سرا;

فراغ m3_ڈھیر_Check_Init ()
{
    حرف ڈھیر;
    ڈھیر_سرا = ڈھیر_شروع = (size_t)&ڈھیر;
}

فراغ m3_ڈھیر_Check ()
{
    حرف   ڈھیر;
    size_t addr = (size_t)&ڈھیر;

    size_t ڈھیر_سرا_2 = ڈھیر_سرا;

    ڈھیر_سرا = M3_MIN(ڈھیر_سرا, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

صحيح m3_ڈھیر_Get_Max ()
{
    أرجع ڈھیر_شروع - ڈھیر_سرا;
}

#جے_انت

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_نتیجہ NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u8 type = -i_convolutedWasmType;

    إذا (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    وإلا إذا (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } وإلا إذا (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#إذا d_m3HasExceptionHandling
    وإلا إذا (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
    وإلا إذا (type == d_wa_Type_ڈھیر_cont or type == d_wa_Type_ڈھیر_nocont) {
        type = c_m3Type_contref;
    }
#جے_انت
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    وإلا إذا (type < c_m3Type_i32 or type > c_m3Type_v128) {
        نتیجہ = m3Err_invalidTypeId;
    }

    *o_type = type;

    أرجع نتیجہ;
}


#إذا d_m3HasTypedRefs

u8 ادھار_Type_دا (m3type_t i_type)
{
    إذا (IsSpelledRefType(i_type)) {
        إذا (i_type & d_m3Type_refCont) {
            أرجع c_m3Type_contref;
        }
        أرجع (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    أرجع (u8)i_type;
}


bool Is_Sub_Type_دا (m3type_t i_sub, m3type_t i_super)
{
    إذا (i_sub == i_super) {
        أرجع true;
    }

    u8 ادھار = ادھار_Type_دا(i_sub);

    // only references have a subtype relation; every other type is invariant
    إذا (ادھار != ادھار_Type_دا(i_super) or not IsRefType(ادھار)) {
        أرجع false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    إذا (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        أرجع false;
    }

#  إذا d_m3_Has_ڈھیر_Switching
    // nocont sits under every continuation type
    إذا (ڈھیر_Type_دا(i_sub) == d_m3_Type_ڈھیر_None) {
        أرجع true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    أرجع (ڈھیر_Type_دا(i_super) == d_m3_Type_ڈھیر_Abstract) or
           (ڈھیر_Type_دا(i_sub) == ڈھیر_Type_دا(i_super));
}

#جے_انت // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = ادھار_Type_دا(i_type);
    أرجع (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = ادھار_Type_دا(i_type);
    أرجع (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#إذا d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#جے_انت
#إذا d_m3_Has_ڈھیر_Switching
            or i_m3Type == c_m3Type_contref
#جے_انت
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = ادھار_Type_دا(i_type);
    أرجع (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_ثنائی_ہندسہ_Type (m3type_t i_type)
{
    u8 i_m3Type = ادھار_Type_دا(i_type);

    إذا (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        أرجع true;
    } وإلا إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        أرجع false;
    } وإلا {
        أرجع (حجم(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 ناپ_دا_Type (m3type_t i_type)
{
    u8 i_m3Type = ادھار_Type_دا(i_type);

    إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        أرجع حجم(i32);
    }

    إذا (IsRefType(i_m3Type)) {
        أرجع حجم(فراغ*);
    }

    أرجع حجم(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_نتیجہ Read_u64 (u64* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    ثابت u8* ptr = *io_بائٹاں;
    ptr += حجم(u64);

    إذا (M3_LIKELY(ptr <= i_سرا)) {
        حافظہ_نقل(o_قدر, *io_بائٹاں, حجم(u64));
        M3_BSWAP_u64(*o_قدر);
        *io_بائٹاں = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتیجہ Read_u32 (u32* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    ثابت u8* ptr = *io_بائٹاں;
    ptr += حجم(u32);

    إذا (M3_LIKELY(ptr <= i_سرا)) {
        حافظہ_نقل(o_قدر, *io_بائٹاں, حجم(u32));
        M3_BSWAP_u32(*o_قدر);
        *io_بائٹاں = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#إذا d_m3ImplementFloat

M3_نتیجہ Read_f64 (f64* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    ثابت u8* ptr = *io_بائٹاں;
    ptr += حجم(f64);

    إذا (M3_LIKELY(ptr <= i_سرا)) {
        حافظہ_نقل(o_قدر, *io_بائٹاں, حجم(f64));
        M3_BSWAP_f64(*o_قدر);
        *io_بائٹاں = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتیجہ Read_f32 (f32* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    ثابت u8* ptr = *io_بائٹاں;
    ptr += حجم(f32);

    إذا (M3_LIKELY(ptr <= i_سرا)) {
        حافظہ_نقل(o_قدر, *io_بائٹاں, حجم(f32));
        M3_BSWAP_f32(*o_قدر);
        *io_بائٹاں = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#جے_انت

M3_نتیجہ Read_u8 (u8* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    ثابت u8* ptr = *io_بائٹاں;

    إذا (M3_LIKELY(ptr < i_سرا)) {
        *o_قدر  = *ptr;
        *io_بائٹاں = ptr + 1;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

M3_نتیجہ Read_opcode (m3opcode_t* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    ثابت u8* ptr = *io_بائٹاں;

    إذا (M3_LIKELY(ptr < i_سرا)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_قدر  = opcode;
        *io_بائٹاں = ptr;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتیجہ ReadLebUnsigned (u64* o_قدر, u32 i_max_عدد_بٹ, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_wasmUnderrun;

    u64 قدر = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بائٹاں;

    طالما (ptr < i_سرا) {
        u64 byte = *(ptr++);

        قدر |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            نتیجہ = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            إذا (M3_UNLIKELY(shift > i_max_عدد_بٹ)) {
                u32 عدد_ورتیا_بٹ = i_max_عدد_بٹ + 7 - shift;

                إذا (byte >> عدد_ورتیا_بٹ) {
                    نتیجہ = m3_Err_leb_حد_توں_ودھ;
                }
            }
            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_عدد_بٹ)) {
            نتیجہ = m3_Err_leb_حد_توں_ودھ;
            اكسر;
        }
    }

    *o_قدر  = قدر;
    *io_بائٹاں = ptr;

    أرجع نتیجہ;
}


M3_نتیجہ ReadLebSigned (i64* o_قدر, u32 i_max_عدد_بٹ, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_wasmUnderrun;

    i64 قدر = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بائٹاں;

    طالما (ptr < i_سرا) {
        u64 byte = *(ptr++);

        قدر |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            نتیجہ = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            إذا (M3_UNLIKELY(shift > i_max_عدد_بٹ)) {
                u32 عدد_ورتیا_بٹ = i_max_عدد_بٹ + 7 - shift;
                u8  sign_بٹ    = (u8)((0x7f << (عدد_ورتیا_بٹ - 1)) & 0x7f);
                u8  بٹ        = (u8)(byte & sign_بٹ);

                إذا (بٹ != 0 and بٹ != sign_بٹ) {
                    نتیجہ = m3_Err_leb_حد_توں_ودھ;
                }
            }
            إذا ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                قدر |= (~extend << shift);
            }

            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_عدد_بٹ)) {
            نتیجہ = m3_Err_leb_حد_توں_ودھ;
            اكسر;
        }
    }

    *o_قدر  = قدر;
    *io_بائٹاں = ptr;

    أرجع نتیجہ;
}


M3_نتیجہ ReadLEB_u32 (u32* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    u64      قدر;
    M3_نتیجہ نتیجہ = ReadLebUnsigned(&قدر, 32, io_بائٹاں, i_سرا);

    *o_قدر = (u32)قدر;

    أرجع نتیجہ;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#تعریف_کرو d_mem_Arg_Has_حافظہ_Idx    0x40u

M3_نتیجہ Read_حافظہ_Arg (u32* o_align, u32* o_حافظہ_Idx, u64* o_وتھ, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ;

    u32 align;
    نتیجہ = ReadLEB_u32(&align, io_بائٹاں, i_سرا);
    إذا (نتیجہ) {
        أرجع نتیجہ;
    }

    *o_حافظہ_Idx = 0;

#إذا d_m3_Has_Multi_حافظہ
    إذا (align & d_mem_Arg_Has_حافظہ_Idx) {
        align &= ~d_mem_Arg_Has_حافظہ_Idx;

        نتیجہ = ReadLEB_u32(o_حافظہ_Idx, io_بائٹاں, i_سرا);
        إذا (نتیجہ) {
            أرجع نتیجہ;
        }
    }
#جے_انت

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    أرجع ReadLebUnsigned(o_وتھ, 64, io_بائٹاں, i_سرا);
}


M3_نتیجہ ReadLEB_u7 (u8* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    u64      قدر;
    M3_نتیجہ نتیجہ = ReadLebUnsigned(&قدر, 7, io_بائٹاں, i_سرا);

    *o_قدر = (u8)قدر;

    أرجع نتیجہ;
}


M3_نتیجہ ReadLEB_i7 (i8* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    i64      قدر;
    M3_نتیجہ نتیجہ = ReadLebSigned(&قدر, 7, io_بائٹاں, i_سرا);

    *o_قدر = (i8)قدر;

    أرجع نتیجہ;
}


M3_نتیجہ ReadLEB_i32 (i32* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    i64      قدر;
    M3_نتیجہ نتیجہ = ReadLebSigned(&قدر, 32, io_بائٹاں, i_سرا);

    *o_قدر = (i32)قدر;

    أرجع نتیجہ;
}


M3_نتیجہ ReadLEB_i64 (i64* o_قدر, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    i64      قدر;
    M3_نتیجہ نتیجہ = ReadLebSigned(&قدر, 64, io_بائٹاں, i_سرا);

    *o_قدر = قدر;

    أرجع نتیجہ;
}

#إذا d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ساكن
bool IsValidUtf8 (ثابت u8* i_انکڑے, u32 i_لمائی)
{
    ثابت u8* ptr = i_انکڑے;
    ثابت u8* سرا = i_انکڑے + i_لمائی;

    طالما (ptr < سرا) {
        u8 b0 = *ptr++;

        إذا (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            استمر;
        } وإلا إذا ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            إذا (b0 < 0xC2) {
                أرجع false;       // overlong
            }
            إذا (ptr >= سرا) {
                أرجع false;
            }
            u8 b1 = *ptr++;
            إذا ((b1 & 0xC0) != 0x80) {
                أرجع false;
            }
        } وإلا إذا ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            إذا (ptr + 1 >= سرا) {
                أرجع false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            إذا ((b1 & 0xC0) != 0x80) {
                أرجع false;
            }
            إذا ((b2 & 0xC0) != 0x80) {
                أرجع false;
            }
            // reject overlong
            إذا (b0 == 0xE0 && b1 < 0xA0) {
                أرجع false;
            }
            // reject surrogates U+D800..U+DFFF
            إذا (b0 == 0xED && b1 >= 0xA0) {
                أرجع false;
            }
        } وإلا إذا ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            إذا (b0 > 0xF4) {
                أرجع false;       // above U+10FFFF
            }
            إذا (ptr + 2 >= سرا) {
                أرجع false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            إذا ((b1 & 0xC0) != 0x80) {
                أرجع false;
            }
            إذا ((b2 & 0xC0) != 0x80) {
                أرجع false;
            }
            إذا ((b3 & 0xC0) != 0x80) {
                أرجع false;
            }
            // reject overlong
            إذا (b0 == 0xF0 && b1 < 0x90) {
                أرجع false;
            }
            // reject above U+10FFFF
            إذا (b0 == 0xF4 && b1 > 0x8F) {
                أرجع false;
            }
        } وإلا {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            أرجع false;
        }
    }

    أرجع true;
}
#جے_انت // d_m3EnableValidation


M3_نتیجہ Read_utf8 (cstr_t* o_utf8, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    *o_utf8 = عدم;

    u32      utf8_لمائی;
    M3_نتیجہ نتیجہ = ReadLEB_u32(&utf8_لمائی, io_بائٹاں, i_سرا);

    إذا (not نتیجہ) {
        إذا (utf8_لمائی <= d_m3_Max_Sane_Utf8_لمائی) {
            ثابت u8* ptr = *io_بائٹاں;
            إذا (utf8_لمائی <= (size_t)(i_سرا - ptr)) {
                ثابت u8* سرا = ptr + utf8_لمائی;
#إذا d_m3EnableValidation
                إذا (not IsValidUtf8(ptr, utf8_لمائی)) {
                    *io_بائٹاں = سرا;
                    أرجع m3Err_wasmMalformed;
                }
#جے_انت // d_m3EnableValidation

                حرف* utf8 = (حرف*)m3_Malloc("UTF8", utf8_لمائی + 1);

                إذا (utf8) {
                    حافظہ_نقل(utf8, ptr, utf8_لمائی);
                    utf8[utf8_لمائی] = 0;

                    *o_utf8 = utf8;
                } وإلا {
                    نتیجہ = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_بائٹاں = سرا;
            } وإلا {
                نتیجہ = m3Err_wasmUnderrun;
            }
        } وإلا {
            نتیجہ = m3Err_missingUTF8;
        }
    }

    أرجع نتیجہ;
}

#إذا d_m3_ریکارڈ_Backtraces
u32 لبھو_Module_وتھ (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_کوڈ_ورقہ curr = i_runtime->ورقے_Open;

    bool ورقہ_لبھیا = false;

    طالما (curr) {
        إذا (شامل_اے_PC(curr, i_pc)) {
            ورقہ_لبھیا = true;
            اكسر;
        }
        curr = curr->info.اگلا;
    }

    إذا (!ورقہ_لبھیا) {
        curr = i_runtime->ورقے_بھریا;
        طالما (curr) {
            إذا (شامل_اے_PC(curr, i_pc)) {
                ورقہ_لبھیا = true;
                اكسر;
            }
            curr = curr->info.اگلا;
        }
    }

    إذا (ورقہ_لبھیا) {
        u32 نتیجہ = 0;

        bool pc_لبھیا = Map_PCTo_وتھ(curr, i_pc, &نتیجہ);
        (فراغ)pc_لبھیا;                                                          d_m3Assert (pc_لبھیا);

        أرجع نتیجہ;
    } وإلا {
        أرجع 0;
    }
}


فراغ ڈالو_Backtrace_چوکھٹا (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    إذا (M3_UNLIKELY(io_runtime->backtrace.آخری_چوکھٹا == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    M3_Backtrace_چوکھٹا* new_چوکھٹا = m3_AllocStruct(M3_Backtrace_چوکھٹا);

    إذا (!new_چوکھٹا) {
        io_runtime->backtrace.آخری_چوکھٹا = M3_BACKTRACE_TRUNCATED;
        أرجع;
    }

    new_چوکھٹا->module_وتھ = لبھو_Module_وتھ(io_runtime, i_pc);

    إذا (!io_runtime->backtrace.چوکھٹے || !io_runtime->backtrace.آخری_چوکھٹا) {
        io_runtime->backtrace.چوکھٹے = new_چوکھٹا;
    } وإلا {
        io_runtime->backtrace.آخری_چوکھٹا->اگلا = new_چوکھٹا;
    }
    io_runtime->backtrace.آخری_چوکھٹا = new_چوکھٹا;
}


فراغ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    إذا (M3_UNLIKELY(io_runtime->backtrace.آخری_چوکھٹا == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    إذا (!io_runtime->backtrace.آخری_چوکھٹا) {
        أرجع;
    }

    io_runtime->backtrace.آخری_چوکھٹا->function = i_function;
}


فراغ ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_چوکھٹا* current_چوکھٹا = io_runtime->backtrace.چوکھٹے;
    طالما (current_چوکھٹا) {
        M3_Backtrace_چوکھٹا* اگلا_چوکھٹا = current_چوکھٹا->اگلا;
        m3_Free(current_چوکھٹا);
        current_چوکھٹا = اگلا_چوکھٹا;
    }

    io_runtime->backtrace.چوکھٹے    = عدم;
    io_runtime->backtrace.آخری_چوکھٹا = عدم;
}
#جے_انت // d_m3RecordBacktraces
