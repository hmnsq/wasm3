//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#جيڪڏهن_وصف_نه_ٿيل GNU_ذريعو
#  define GNU_ذريعو 1
#جيڪڏهن_پورو

#وصف_ڪريو M3_IMPLEMENT_ERROR_لڙيون
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
#جيڪڏهن_پورو

فراغ m3_Abort (ثابت حرف* پيغام)
{
#جيڪڏهن_وصف_ٿيل DEBUG
    fprintf(stderr, "Error: %s\n", پيغام);
#جيڪڏهن_پورو
    abort();
}

M3_WEAK
M3_نتيجو m3_Yield ()
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

#جيڪڏهن_پورو

#إذا d_m3_Fixed_ڍير

ساكن u8        fixed_ڍير[d_m3_Fixed_ڍير];
ساكن u8*       fixed_ڍير_Ptr  = fixed_ڍير;
ساكن u8* ثابت fixed_ڍير_پڇاڙي  = fixed_ڍير + d_m3_Fixed_ڍير;
ساكن u8*       fixed_ڍير_آخري = عدم;

#  إذا d_m3_Fixed_ڍير_Align > 1
#    define ڍير_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_ڍير_Align-1)) & ~ (d_m3_Fixed_ڍير_Align-1));
#  وإلا
#    define ڍير_ALIGN_PTR(P)
#  endif

فراغ* m3_Malloc_Impl (size_t i_سائيز)
{
    u8* ptr = fixed_ڍير_Ptr;

    fixed_ڍير_Ptr += i_سائيز;
    ڍير_ALIGN_PTR(fixed_ڍير_Ptr);

    إذا (fixed_ڍير_Ptr >= fixed_ڍير_پڇاڙي) {
        أرجع عدم;
    }

    memset(ptr, 0x0, i_سائيز);
    fixed_ڍير_آخري = ptr;

    أرجع ptr;
}

فراغ m3_Free_Impl (فراغ* i_ptr)
{
    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_ڍير_آخري) {
        fixed_ڍير_Ptr  = fixed_ڍير_آخري;
        fixed_ڍير_آخري = عدم;
    } وإلا {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_سائيز, size_t i_old_سائيز)
{
    إذا (M3_UNLIKELY(i_new_سائيز == i_old_سائيز)) {
        أرجع i_ptr;
    }

    فراغ* newPtr;

    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_ڍير_آخري) {
        fixed_ڍير_Ptr = fixed_ڍير_آخري + i_new_سائيز;
        ڍير_ALIGN_PTR(fixed_ڍير_Ptr);
        إذا (fixed_ڍير_Ptr >= fixed_ڍير_پڇاڙي) {
            أرجع عدم;
        }
        newPtr = i_ptr;
    } وإلا {
        newPtr = m3_Malloc_Impl(i_new_سائيز);
        إذا (!newPtr) {
            أرجع عدم;
        }
        إذا (i_ptr) {
            يادگيري_نقل(newPtr, i_ptr, i_old_سائيز);
        }
    }

    إذا (i_new_سائيز > i_old_سائيز) {
        memset((u8*)newPtr + i_old_سائيز, 0x0, i_new_سائيز - i_old_سائيز);
    }

    أرجع newPtr;
}

#وإلا

فراغ* m3_Malloc_Impl (size_t i_سائيز)
{
    أرجع صفر_يادگيري_ڏيو(i_سائيز, 1);
}

فراغ m3_Free_Impl (فراغ* io_ptr)
{
    حرر(io_ptr);
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_سائيز, size_t i_old_سائيز)
{
    إذا (M3_UNLIKELY(i_new_سائيز == i_old_سائيز)) {
        أرجع i_ptr;
    }

    فراغ* newPtr = realloc(i_ptr, i_new_سائيز);

    إذا (M3_LIKELY(newPtr)) {
        إذا (i_new_سائيز > i_old_سائيز) {
            memset((u8*)newPtr + i_old_سائيز, 0x0, i_new_سائيز - i_old_سائيز);
        }
        أرجع newPtr;
    }
    أرجع عدم;
}

#جيڪڏهن_پورو

فراغ* m3_CopyMem (ثابت فراغ* i_from, size_t i_سائيز)
{
    فراغ* ptr = m3_Malloc("CopyMem", i_سائيز);
    إذا (ptr) {
        يادگيري_نقل(ptr, i_from, i_سائيز);
    }
    أرجع ptr;
}

//--------------------------------------------------------------------------------------------

#إذا d_m3_Guarded_يادگيري

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ساكن u8*    g_guardArena;
ساكن size_t g_guard_Arena_بائيٽون;
ساكن size_t g_guard_جڳهه_بائيٽون;
ساكن size_t g_guard_صفحو_سائيز;
ساكن u32    g_guard_جڳهه_ڳڻپ;
ساكن bool   g_guard_جڳهه_ورتل[d_m3GuardedArenaSlots];

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

    size_t صفحو_سائيز = m3_Host_صفحو_سائيز();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t جڳهه_بائيٽون = 2 * صفحو_سائيز + (size_t)d_m3_Guarded_معلومات_بائيٽون;

    لكل (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        فراغ* بنياد = m3_HostReserve(جڳهه_بائيٽون * slots);

        إذا (بنياد) {
            g_guardArena      = (u8*)بنياد;
            g_guard_Arena_بائيٽون = جڳهه_بائيٽون * slots;
            g_guard_جڳهه_بائيٽون  = جڳهه_بائيٽون;
            g_guard_صفحو_سائيز   = صفحو_سائيز;
            g_guard_جڳهه_ڳڻپ  = slots;
            أرجع true;
        }
    }

    أرجع false;
}

فراغ* Guard_Take_جڳهه (فراغ)
{
    إذا (not Guard_Reserve()) {
        أرجع عدم;
    }

    لكل (u32 i = 0; i < g_guard_جڳهه_ڳڻپ; ++i) {
        إذا (g_guard_جڳهه_ورتل[i]) {
            استمر;
        }

        u8* جڳهه = g_guardArena + (size_t)i * g_guard_جڳهه_بائيٽون;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        إذا (not m3_HostCommit(جڳهه, g_guard_صفحو_سائيز)) {
            أرجع عدم;
        }

        g_guard_جڳهه_ورتل[i] = true;
        أرجع جڳهه;
    }

    أرجع عدم;
}

M3_يادگيري_سرو* Guard_جڳهه_سرو (فراغ* i_جڳهه)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* معلومات = (u8*)i_جڳهه + g_guard_صفحو_سائيز;

    أرجع (M3_يادگيري_سرو*)(معلومات - حجم(M3_يادگيري_سرو));
}

bool Guard_Commit_جڳهه (فراغ* i_جڳهه, size_t i_معلومات_بائيٽون)
{
    size_t rounded = (i_معلومات_بائيٽون + g_guard_صفحو_سائيز - 1) / g_guard_صفحو_سائيز * g_guard_صفحو_سائيز;

    أرجع m3_HostCommit((u8*)i_جڳهه + g_guard_صفحو_سائيز, rounded);
}

فراغ Guard_Give_جڳهه (فراغ* i_جڳهه)
{
    إذا (i_جڳهه == عدم) {
        أرجع;
    }

    size_t index = (size_t)((u8*)i_جڳهه - g_guardArena) / g_guard_جڳهه_بائيٽون;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    إذا (m3_HostDecommit(i_جڳهه, g_guard_جڳهه_بائيٽون) and index < g_guard_جڳهه_ڳڻپ) {
        g_guard_جڳهه_ورتل[index] = false;
    }
}

فراغ Guard_ArenaRange (فراغ** o_هيٺيون, size_t* o_بائيٽون)
{
    *o_هيٺيون   = g_guardArena;
    *o_بائيٽون = g_guard_Arena_بائيٽون;
}

#جيڪڏهن_پورو // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#إذا d_m3_Max_Native_ڍير > 0

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
فراغ* m3_Native_ڍير_حد_بندي (فراغ* i_ڍير_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ڍير_Ptr;

#  إذا M3_HAS_THREAD_LOCAL
    ساكن M3_THREAD_LOCAL u8* بنياد = عدم;
    إذا (بنياد == عدم) {
        بنياد = (u8*)m3_Host_ڍير_بنياد();
    }
#  وإلا
    u8* بنياد = (u8*)m3_Host_ڍير_بنياد();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    إذا (بنياد and بنياد < sp) {
        size_t موجود = (size_t)(sp - بنياد);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ڍير_Margin), موجود / 4);

        i_budget = M3_MIN(i_budget, موجود - margin);
    }

    أرجع sp - i_budget;
}

#جيڪڏهن_پورو // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#إذا d_m3_Log_Native_ڍير

ساكن size_t ڍير_شروعات;
ساكن size_t ڍير_پڇاڙي;

فراغ m3_ڍير_Check_Init ()
{
    حرف ڍير;
    ڍير_پڇاڙي = ڍير_شروعات = (size_t)&ڍير;
}

فراغ m3_ڍير_Check ()
{
    حرف   ڍير;
    size_t addr = (size_t)&ڍير;

    size_t ڍير_پڇاڙي_2 = ڍير_پڇاڙي;

    ڍير_پڇاڙي = M3_MIN(ڍير_پڇاڙي, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

صحيح m3_ڍير_Get_Max ()
{
    أرجع ڍير_شروعات - ڍير_پڇاڙي;
}

#جيڪڏهن_پورو

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_نتيجو NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_نتيجو نتيجو = m3Err_none;

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
#جيڪڏهن_پورو
#إذا d_m3_Has_ڍير_Switching
    وإلا إذا (type == d_wa_Type_ڍير_cont or type == d_wa_Type_ڍير_nocont) {
        type = c_m3Type_contref;
    }
#جيڪڏهن_پورو
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    وإلا إذا (type < c_m3Type_i32 or type > c_m3Type_v128) {
        نتيجو = m3Err_invalidTypeId;
    }

    *o_type = type;

    أرجع نتيجو;
}


#إذا d_m3HasTypedRefs

u8 بنياد_Type_جو (m3type_t i_type)
{
    إذا (IsSpelledRefType(i_type)) {
        إذا (i_type & d_m3Type_refCont) {
            أرجع c_m3Type_contref;
        }
        أرجع (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    أرجع (u8)i_type;
}


bool Is_Sub_Type_جو (m3type_t i_sub, m3type_t i_super)
{
    إذا (i_sub == i_super) {
        أرجع true;
    }

    u8 بنياد = بنياد_Type_جو(i_sub);

    // only references have a subtype relation; every other type is invariant
    إذا (بنياد != بنياد_Type_جو(i_super) or not IsRefType(بنياد)) {
        أرجع false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    إذا (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        أرجع false;
    }

#  إذا d_m3_Has_ڍير_Switching
    // nocont sits under every continuation type
    إذا (ڍير_Type_جو(i_sub) == d_m3_Type_ڍير_None) {
        أرجع true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    أرجع (ڍير_Type_جو(i_super) == d_m3_Type_ڍير_Abstract) or
           (ڍير_Type_جو(i_sub) == ڍير_Type_جو(i_super));
}

#جيڪڏهن_پورو // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = بنياد_Type_جو(i_type);
    أرجع (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = بنياد_Type_جو(i_type);
    أرجع (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#إذا d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#جيڪڏهن_پورو
#إذا d_m3_Has_ڍير_Switching
            or i_m3Type == c_m3Type_contref
#جيڪڏهن_پورو
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = بنياد_Type_جو(i_type);
    أرجع (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_ٻٽو_انگ_Type (m3type_t i_type)
{
    u8 i_m3Type = بنياد_Type_جو(i_type);

    إذا (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        أرجع true;
    } وإلا إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        أرجع false;
    } وإلا {
        أرجع (حجم(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 سائيز_جو_Type (m3type_t i_type)
{
    u8 i_m3Type = بنياد_Type_جو(i_type);

    إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        أرجع حجم(i32);
    }

    إذا (IsRefType(i_m3Type)) {
        أرجع حجم(فراغ*);
    }

    أرجع حجم(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_نتيجو Read_u64 (u64* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    ثابت u8* ptr = *io_بائيٽون;
    ptr += حجم(u64);

    إذا (M3_LIKELY(ptr <= i_پڇاڙي)) {
        يادگيري_نقل(o_قدر, *io_بائيٽون, حجم(u64));
        M3_BSWAP_u64(*o_قدر);
        *io_بائيٽون = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتيجو Read_u32 (u32* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    ثابت u8* ptr = *io_بائيٽون;
    ptr += حجم(u32);

    إذا (M3_LIKELY(ptr <= i_پڇاڙي)) {
        يادگيري_نقل(o_قدر, *io_بائيٽون, حجم(u32));
        M3_BSWAP_u32(*o_قدر);
        *io_بائيٽون = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#إذا d_m3ImplementFloat

M3_نتيجو Read_f64 (f64* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    ثابت u8* ptr = *io_بائيٽون;
    ptr += حجم(f64);

    إذا (M3_LIKELY(ptr <= i_پڇاڙي)) {
        يادگيري_نقل(o_قدر, *io_بائيٽون, حجم(f64));
        M3_BSWAP_f64(*o_قدر);
        *io_بائيٽون = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتيجو Read_f32 (f32* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    ثابت u8* ptr = *io_بائيٽون;
    ptr += حجم(f32);

    إذا (M3_LIKELY(ptr <= i_پڇاڙي)) {
        يادگيري_نقل(o_قدر, *io_بائيٽون, حجم(f32));
        M3_BSWAP_f32(*o_قدر);
        *io_بائيٽون = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#جيڪڏهن_پورو

M3_نتيجو Read_u8 (u8* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    ثابت u8* ptr = *io_بائيٽون;

    إذا (M3_LIKELY(ptr < i_پڇاڙي)) {
        *o_قدر  = *ptr;
        *io_بائيٽون = ptr + 1;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

M3_نتيجو Read_opcode (m3opcode_t* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    ثابت u8* ptr = *io_بائيٽون;

    إذا (M3_LIKELY(ptr < i_پڇاڙي)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_قدر  = opcode;
        *io_بائيٽون = ptr;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتيجو ReadLebUnsigned (u64* o_قدر, u32 i_max_انگ_بٽ, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_wasmUnderrun;

    u64 قدر = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بائيٽون;

    طالما (ptr < i_پڇاڙي) {
        u64 byte = *(ptr++);

        قدر |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            نتيجو = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            إذا (M3_UNLIKELY(shift > i_max_انگ_بٽ)) {
                u32 انگ_استعمال_ٿيل_بٽ = i_max_انگ_بٽ + 7 - shift;

                إذا (byte >> انگ_استعمال_ٿيل_بٽ) {
                    نتيجو = m3_Err_leb_حد_کان_مٿي;
                }
            }
            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_انگ_بٽ)) {
            نتيجو = m3_Err_leb_حد_کان_مٿي;
            اكسر;
        }
    }

    *o_قدر  = قدر;
    *io_بائيٽون = ptr;

    أرجع نتيجو;
}


M3_نتيجو ReadLebSigned (i64* o_قدر, u32 i_max_انگ_بٽ, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_wasmUnderrun;

    i64 قدر = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بائيٽون;

    طالما (ptr < i_پڇاڙي) {
        u64 byte = *(ptr++);

        قدر |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            نتيجو = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            إذا (M3_UNLIKELY(shift > i_max_انگ_بٽ)) {
                u32 انگ_استعمال_ٿيل_بٽ = i_max_انگ_بٽ + 7 - shift;
                u8  sign_بٽ    = (u8)((0x7f << (انگ_استعمال_ٿيل_بٽ - 1)) & 0x7f);
                u8  بٽ        = (u8)(byte & sign_بٽ);

                إذا (بٽ != 0 and بٽ != sign_بٽ) {
                    نتيجو = m3_Err_leb_حد_کان_مٿي;
                }
            }
            إذا ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                قدر |= (~extend << shift);
            }

            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_انگ_بٽ)) {
            نتيجو = m3_Err_leb_حد_کان_مٿي;
            اكسر;
        }
    }

    *o_قدر  = قدر;
    *io_بائيٽون = ptr;

    أرجع نتيجو;
}


M3_نتيجو ReadLEB_u32 (u32* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    u64      قدر;
    M3_نتيجو نتيجو = ReadLebUnsigned(&قدر, 32, io_بائيٽون, i_پڇاڙي);

    *o_قدر = (u32)قدر;

    أرجع نتيجو;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#وصف_ڪريو d_mem_Arg_Has_يادگيري_Idx    0x40u

M3_نتيجو Read_يادگيري_Arg (u32* o_align, u32* o_يادگيري_Idx, u64* o_وٿي, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو;

    u32 align;
    نتيجو = ReadLEB_u32(&align, io_بائيٽون, i_پڇاڙي);
    إذا (نتيجو) {
        أرجع نتيجو;
    }

    *o_يادگيري_Idx = 0;

#إذا d_m3_Has_Multi_يادگيري
    إذا (align & d_mem_Arg_Has_يادگيري_Idx) {
        align &= ~d_mem_Arg_Has_يادگيري_Idx;

        نتيجو = ReadLEB_u32(o_يادگيري_Idx, io_بائيٽون, i_پڇاڙي);
        إذا (نتيجو) {
            أرجع نتيجو;
        }
    }
#جيڪڏهن_پورو

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    أرجع ReadLebUnsigned(o_وٿي, 64, io_بائيٽون, i_پڇاڙي);
}


M3_نتيجو ReadLEB_u7 (u8* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    u64      قدر;
    M3_نتيجو نتيجو = ReadLebUnsigned(&قدر, 7, io_بائيٽون, i_پڇاڙي);

    *o_قدر = (u8)قدر;

    أرجع نتيجو;
}


M3_نتيجو ReadLEB_i7 (i8* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    i64      قدر;
    M3_نتيجو نتيجو = ReadLebSigned(&قدر, 7, io_بائيٽون, i_پڇاڙي);

    *o_قدر = (i8)قدر;

    أرجع نتيجو;
}


M3_نتيجو ReadLEB_i32 (i32* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    i64      قدر;
    M3_نتيجو نتيجو = ReadLebSigned(&قدر, 32, io_بائيٽون, i_پڇاڙي);

    *o_قدر = (i32)قدر;

    أرجع نتيجو;
}


M3_نتيجو ReadLEB_i64 (i64* o_قدر, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    i64      قدر;
    M3_نتيجو نتيجو = ReadLebSigned(&قدر, 64, io_بائيٽون, i_پڇاڙي);

    *o_قدر = قدر;

    أرجع نتيجو;
}

#إذا d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ساكن
bool IsValidUtf8 (ثابت u8* i_معلومات, u32 i_ڊيگهائي)
{
    ثابت u8* ptr = i_معلومات;
    ثابت u8* پڇاڙي = i_معلومات + i_ڊيگهائي;

    طالما (ptr < پڇاڙي) {
        u8 b0 = *ptr++;

        إذا (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            استمر;
        } وإلا إذا ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            إذا (b0 < 0xC2) {
                أرجع false;       // overlong
            }
            إذا (ptr >= پڇاڙي) {
                أرجع false;
            }
            u8 b1 = *ptr++;
            إذا ((b1 & 0xC0) != 0x80) {
                أرجع false;
            }
        } وإلا إذا ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            إذا (ptr + 1 >= پڇاڙي) {
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
            إذا (ptr + 2 >= پڇاڙي) {
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
#جيڪڏهن_پورو // d_m3EnableValidation


M3_نتيجو Read_utf8 (cstr_t* o_utf8, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    *o_utf8 = عدم;

    u32      utf8_ڊيگهائي;
    M3_نتيجو نتيجو = ReadLEB_u32(&utf8_ڊيگهائي, io_بائيٽون, i_پڇاڙي);

    إذا (not نتيجو) {
        إذا (utf8_ڊيگهائي <= d_m3_Max_Sane_Utf8_ڊيگهائي) {
            ثابت u8* ptr = *io_بائيٽون;
            إذا (utf8_ڊيگهائي <= (size_t)(i_پڇاڙي - ptr)) {
                ثابت u8* پڇاڙي = ptr + utf8_ڊيگهائي;
#إذا d_m3EnableValidation
                إذا (not IsValidUtf8(ptr, utf8_ڊيگهائي)) {
                    *io_بائيٽون = پڇاڙي;
                    أرجع m3Err_wasmMalformed;
                }
#جيڪڏهن_پورو // d_m3EnableValidation

                حرف* utf8 = (حرف*)m3_Malloc("UTF8", utf8_ڊيگهائي + 1);

                إذا (utf8) {
                    يادگيري_نقل(utf8, ptr, utf8_ڊيگهائي);
                    utf8[utf8_ڊيگهائي] = 0;

                    *o_utf8 = utf8;
                } وإلا {
                    نتيجو = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_بائيٽون = پڇاڙي;
            } وإلا {
                نتيجو = m3Err_wasmUnderrun;
            }
        } وإلا {
            نتيجو = m3Err_missingUTF8;
        }
    }

    أرجع نتيجو;
}

#إذا d_m3_رڪارڊ_Backtraces
u32 ڳوليو_Module_وٿي (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_ڪوڊ_صفحو curr = i_runtime->صفحا_Open;

    bool صفحو_مليو = false;

    طالما (curr) {
        إذا (شامل_آهي_PC(curr, i_pc)) {
            صفحو_مليو = true;
            اكسر;
        }
        curr = curr->info.ايندڙ;
    }

    إذا (!صفحو_مليو) {
        curr = i_runtime->صفحا_ڀريل;
        طالما (curr) {
            إذا (شامل_آهي_PC(curr, i_pc)) {
                صفحو_مليو = true;
                اكسر;
            }
            curr = curr->info.ايندڙ;
        }
    }

    إذا (صفحو_مليو) {
        u32 نتيجو = 0;

        bool pc_مليو = Map_PCTo_وٿي(curr, i_pc, &نتيجو);
        (فراغ)pc_مليو;                                                          d_m3Assert (pc_مليو);

        أرجع نتيجو;
    } وإلا {
        أرجع 0;
    }
}


فراغ ڌڪو_Backtrace_چوکٺو (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    إذا (M3_UNLIKELY(io_runtime->backtrace.آخري_چوکٺو == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    M3_Backtrace_چوکٺو* new_چوکٺو = m3_AllocStruct(M3_Backtrace_چوکٺو);

    إذا (!new_چوکٺو) {
        io_runtime->backtrace.آخري_چوکٺو = M3_BACKTRACE_TRUNCATED;
        أرجع;
    }

    new_چوکٺو->module_وٿي = ڳوليو_Module_وٿي(io_runtime, i_pc);

    إذا (!io_runtime->backtrace.چوکٺا || !io_runtime->backtrace.آخري_چوکٺو) {
        io_runtime->backtrace.چوکٺا = new_چوکٺو;
    } وإلا {
        io_runtime->backtrace.آخري_چوکٺو->ايندڙ = new_چوکٺو;
    }
    io_runtime->backtrace.آخري_چوکٺو = new_چوکٺو;
}


فراغ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    إذا (M3_UNLIKELY(io_runtime->backtrace.آخري_چوکٺو == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    إذا (!io_runtime->backtrace.آخري_چوکٺو) {
        أرجع;
    }

    io_runtime->backtrace.آخري_چوکٺو->function = i_function;
}


فراغ ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_چوکٺو* current_چوکٺو = io_runtime->backtrace.چوکٺا;
    طالما (current_چوکٺو) {
        M3_Backtrace_چوکٺو* ايندڙ_چوکٺو = current_چوکٺو->ايندڙ;
        m3_Free(current_چوکٺو);
        current_چوکٺو = ايندڙ_چوکٺو;
    }

    io_runtime->backtrace.چوکٺا    = عدم;
    io_runtime->backtrace.آخري_چوکٺو = عدم;
}
#جيڪڏهن_پورو // d_m3RecordBacktraces
