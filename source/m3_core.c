//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#که_نه_تعریف_شوی GNU_سرچینه
#  define GNU_سرچینه 1
#د_که_پای

#تعریف_کړه M3_IMPLEMENT_ERROR_مزي_ګانې
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
#د_که_پای

فراغ m3_Abort (ثابت حرف* پیغام)
{
#که_تعریف_شوی DEBUG
    fprintf(stderr, "Error: %s\n", پیغام);
#د_که_پای
    abort();
}

M3_WEAK
M3_پایله m3_Yield ()
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

#د_که_پای

#إذا d_m3_Fixed_ډېرۍ

ساكن u8        fixed_ډېرۍ[d_m3_Fixed_ډېرۍ];
ساكن u8*       fixed_ډېرۍ_Ptr  = fixed_ډېرۍ;
ساكن u8* ثابت fixed_ډېرۍ_پای_ټکی  = fixed_ډېرۍ + d_m3_Fixed_ډېرۍ;
ساكن u8*       fixed_ډېرۍ_وروستی = عدم;

#  إذا d_m3_Fixed_ډېرۍ_Align > 1
#    define ډېرۍ_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_ډېرۍ_Align-1)) & ~ (d_m3_Fixed_ډېرۍ_Align-1));
#  وإلا
#    define ډېرۍ_ALIGN_PTR(P)
#  endif

فراغ* m3_Malloc_Impl (size_t i_اندازه)
{
    u8* ptr = fixed_ډېرۍ_Ptr;

    fixed_ډېرۍ_Ptr += i_اندازه;
    ډېرۍ_ALIGN_PTR(fixed_ډېرۍ_Ptr);

    إذا (fixed_ډېرۍ_Ptr >= fixed_ډېرۍ_پای_ټکی) {
        أرجع عدم;
    }

    memset(ptr, 0x0, i_اندازه);
    fixed_ډېرۍ_وروستی = ptr;

    أرجع ptr;
}

فراغ m3_Free_Impl (فراغ* i_ptr)
{
    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_ډېرۍ_وروستی) {
        fixed_ډېرۍ_Ptr  = fixed_ډېرۍ_وروستی;
        fixed_ډېرۍ_وروستی = عدم;
    } وإلا {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_اندازه, size_t i_old_اندازه)
{
    إذا (M3_UNLIKELY(i_new_اندازه == i_old_اندازه)) {
        أرجع i_ptr;
    }

    فراغ* newPtr;

    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_ډېرۍ_وروستی) {
        fixed_ډېرۍ_Ptr = fixed_ډېرۍ_وروستی + i_new_اندازه;
        ډېرۍ_ALIGN_PTR(fixed_ډېرۍ_Ptr);
        إذا (fixed_ډېرۍ_Ptr >= fixed_ډېرۍ_پای_ټکی) {
            أرجع عدم;
        }
        newPtr = i_ptr;
    } وإلا {
        newPtr = m3_Malloc_Impl(i_new_اندازه);
        إذا (!newPtr) {
            أرجع عدم;
        }
        إذا (i_ptr) {
            حافظه_نقل(newPtr, i_ptr, i_old_اندازه);
        }
    }

    إذا (i_new_اندازه > i_old_اندازه) {
        memset((u8*)newPtr + i_old_اندازه, 0x0, i_new_اندازه - i_old_اندازه);
    }

    أرجع newPtr;
}

#وإلا

فراغ* m3_Malloc_Impl (size_t i_اندازه)
{
    أرجع حافظه_صفر_ورکړه(i_اندازه, 1);
}

فراغ m3_Free_Impl (فراغ* io_ptr)
{
    حرر(io_ptr);
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_اندازه, size_t i_old_اندازه)
{
    إذا (M3_UNLIKELY(i_new_اندازه == i_old_اندازه)) {
        أرجع i_ptr;
    }

    فراغ* newPtr = realloc(i_ptr, i_new_اندازه);

    إذا (M3_LIKELY(newPtr)) {
        إذا (i_new_اندازه > i_old_اندازه) {
            memset((u8*)newPtr + i_old_اندازه, 0x0, i_new_اندازه - i_old_اندازه);
        }
        أرجع newPtr;
    }
    أرجع عدم;
}

#د_که_پای

فراغ* m3_CopyMem (ثابت فراغ* i_from, size_t i_اندازه)
{
    فراغ* ptr = m3_Malloc("CopyMem", i_اندازه);
    إذا (ptr) {
        حافظه_نقل(ptr, i_from, i_اندازه);
    }
    أرجع ptr;
}

//--------------------------------------------------------------------------------------------

#إذا d_m3_Guarded_حافظه

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ساكن u8*    g_guardArena;
ساكن size_t g_guard_Arena_بایټونه;
ساكن size_t g_guard_خونه_بایټونه;
ساكن size_t g_guard_پاڼه_اندازه;
ساكن u32    g_guard_خونه_شمېر;
ساكن bool   g_guard_خونه_اخیستل_شوی[d_m3GuardedArenaSlots];

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

    size_t پاڼه_اندازه = m3_Host_پاڼه_اندازه();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t خونه_بایټونه = 2 * پاڼه_اندازه + (size_t)d_m3_Guarded_معلومات_بایټونه;

    لكل (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        فراغ* بنسټ = m3_HostReserve(خونه_بایټونه * slots);

        إذا (بنسټ) {
            g_guardArena      = (u8*)بنسټ;
            g_guard_Arena_بایټونه = خونه_بایټونه * slots;
            g_guard_خونه_بایټونه  = خونه_بایټونه;
            g_guard_پاڼه_اندازه   = پاڼه_اندازه;
            g_guard_خونه_شمېر  = slots;
            أرجع true;
        }
    }

    أرجع false;
}

فراغ* Guard_Take_خونه (فراغ)
{
    إذا (not Guard_Reserve()) {
        أرجع عدم;
    }

    لكل (u32 i = 0; i < g_guard_خونه_شمېر; ++i) {
        إذا (g_guard_خونه_اخیستل_شوی[i]) {
            استمر;
        }

        u8* خونه = g_guardArena + (size_t)i * g_guard_خونه_بایټونه;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        إذا (not m3_HostCommit(خونه, g_guard_پاڼه_اندازه)) {
            أرجع عدم;
        }

        g_guard_خونه_اخیستل_شوی[i] = true;
        أرجع خونه;
    }

    أرجع عدم;
}

M3_حافظه_سرلیک* Guard_خونه_سرلیک (فراغ* i_خونه)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* معلومات = (u8*)i_خونه + g_guard_پاڼه_اندازه;

    أرجع (M3_حافظه_سرلیک*)(معلومات - حجم(M3_حافظه_سرلیک));
}

bool Guard_Commit_خونه (فراغ* i_خونه, size_t i_معلومات_بایټونه)
{
    size_t rounded = (i_معلومات_بایټونه + g_guard_پاڼه_اندازه - 1) / g_guard_پاڼه_اندازه * g_guard_پاڼه_اندازه;

    أرجع m3_HostCommit((u8*)i_خونه + g_guard_پاڼه_اندازه, rounded);
}

فراغ Guard_Give_خونه (فراغ* i_خونه)
{
    إذا (i_خونه == عدم) {
        أرجع;
    }

    size_t index = (size_t)((u8*)i_خونه - g_guardArena) / g_guard_خونه_بایټونه;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    إذا (m3_HostDecommit(i_خونه, g_guard_خونه_بایټونه) and index < g_guard_خونه_شمېر) {
        g_guard_خونه_اخیستل_شوی[index] = false;
    }
}

فراغ Guard_ArenaRange (فراغ** o_ټیټ, size_t* o_بایټونه)
{
    *o_ټیټ   = g_guardArena;
    *o_بایټونه = g_guard_Arena_بایټونه;
}

#د_که_پای // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#إذا d_m3_Max_Native_ډېری > 0

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
فراغ* m3_Native_ډېری_حد (فراغ* i_ډېری_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ډېری_Ptr;

#  إذا M3_HAS_THREAD_LOCAL
    ساكن M3_THREAD_LOCAL u8* بنسټ = عدم;
    إذا (بنسټ == عدم) {
        بنسټ = (u8*)m3_Host_ډېری_بنسټ();
    }
#  وإلا
    u8* بنسټ = (u8*)m3_Host_ډېری_بنسټ();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    إذا (بنسټ and بنسټ < sp) {
        size_t شته = (size_t)(sp - بنسټ);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ډېری_Margin), شته / 4);

        i_budget = M3_MIN(i_budget, شته - margin);
    }

    أرجع sp - i_budget;
}

#د_که_پای // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#إذا d_m3_Log_Native_ډېری

ساكن size_t ډېری_پیل;
ساكن size_t ډېری_پای_ټکی;

فراغ m3_ډېری_Check_Init ()
{
    حرف ډېری;
    ډېری_پای_ټکی = ډېری_پیل = (size_t)&ډېری;
}

فراغ m3_ډېری_Check ()
{
    حرف   ډېری;
    size_t addr = (size_t)&ډېری;

    size_t ډېری_پای_ټکی_2 = ډېری_پای_ټکی;

    ډېری_پای_ټکی = M3_MIN(ډېری_پای_ټکی, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

صحيح m3_ډېری_Get_Max ()
{
    أرجع ډېری_پیل - ډېری_پای_ټکی;
}

#د_که_پای

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_پایله NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_پایله پایله = m3Err_none;

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
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
    وإلا إذا (type == d_wa_Type_ډېرۍ_cont or type == d_wa_Type_ډېرۍ_nocont) {
        type = c_m3Type_contref;
    }
#د_که_پای
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    وإلا إذا (type < c_m3Type_i32 or type > c_m3Type_v128) {
        پایله = m3Err_invalidTypeId;
    }

    *o_type = type;

    أرجع پایله;
}


#إذا d_m3HasTypedRefs

u8 بنسټ_Type_د (m3type_t i_type)
{
    إذا (IsSpelledRefType(i_type)) {
        إذا (i_type & d_m3Type_refCont) {
            أرجع c_m3Type_contref;
        }
        أرجع (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    أرجع (u8)i_type;
}


bool Is_Sub_Type_د (m3type_t i_sub, m3type_t i_super)
{
    إذا (i_sub == i_super) {
        أرجع true;
    }

    u8 بنسټ = بنسټ_Type_د(i_sub);

    // only references have a subtype relation; every other type is invariant
    إذا (بنسټ != بنسټ_Type_د(i_super) or not IsRefType(بنسټ)) {
        أرجع false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    إذا (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        أرجع false;
    }

#  إذا d_m3_Has_ډېری_Switching
    // nocont sits under every continuation type
    إذا (ډېرۍ_Type_د(i_sub) == d_m3_Type_ډېرۍ_None) {
        أرجع true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    أرجع (ډېرۍ_Type_د(i_super) == d_m3_Type_ډېرۍ_Abstract) or
           (ډېرۍ_Type_د(i_sub) == ډېرۍ_Type_د(i_super));
}

#د_که_پای // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = بنسټ_Type_د(i_type);
    أرجع (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = بنسټ_Type_د(i_type);
    أرجع (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#إذا d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
            or i_m3Type == c_m3Type_contref
#د_که_پای
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = بنسټ_Type_د(i_type);
    أرجع (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_بټ_Type (m3type_t i_type)
{
    u8 i_m3Type = بنسټ_Type_د(i_type);

    إذا (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        أرجع true;
    } وإلا إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        أرجع false;
    } وإلا {
        أرجع (حجم(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 اندازه_د_Type (m3type_t i_type)
{
    u8 i_m3Type = بنسټ_Type_د(i_type);

    إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        أرجع حجم(i32);
    }

    إذا (IsRefType(i_m3Type)) {
        أرجع حجم(فراغ*);
    }

    أرجع حجم(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_پایله Read_u64 (u64* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    ثابت u8* ptr = *io_بایټونه;
    ptr += حجم(u64);

    إذا (M3_LIKELY(ptr <= i_پای_ټکی)) {
        حافظه_نقل(o_ارزښت, *io_بایټونه, حجم(u64));
        M3_BSWAP_u64(*o_ارزښت);
        *io_بایټونه = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_پایله Read_u32 (u32* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    ثابت u8* ptr = *io_بایټونه;
    ptr += حجم(u32);

    إذا (M3_LIKELY(ptr <= i_پای_ټکی)) {
        حافظه_نقل(o_ارزښت, *io_بایټونه, حجم(u32));
        M3_BSWAP_u32(*o_ارزښت);
        *io_بایټونه = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#إذا d_m3ImplementFloat

M3_پایله Read_f64 (f64* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    ثابت u8* ptr = *io_بایټونه;
    ptr += حجم(f64);

    إذا (M3_LIKELY(ptr <= i_پای_ټکی)) {
        حافظه_نقل(o_ارزښت, *io_بایټونه, حجم(f64));
        M3_BSWAP_f64(*o_ارزښت);
        *io_بایټونه = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_پایله Read_f32 (f32* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    ثابت u8* ptr = *io_بایټونه;
    ptr += حجم(f32);

    إذا (M3_LIKELY(ptr <= i_پای_ټکی)) {
        حافظه_نقل(o_ارزښت, *io_بایټونه, حجم(f32));
        M3_BSWAP_f32(*o_ارزښت);
        *io_بایټونه = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#د_که_پای

M3_پایله Read_u8 (u8* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    ثابت u8* ptr = *io_بایټونه;

    إذا (M3_LIKELY(ptr < i_پای_ټکی)) {
        *o_ارزښت  = *ptr;
        *io_بایټونه = ptr + 1;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

M3_پایله Read_opcode (m3opcode_t* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    ثابت u8* ptr = *io_بایټونه;

    إذا (M3_LIKELY(ptr < i_پای_ټکی)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_ارزښت  = opcode;
        *io_بایټونه = ptr;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_پایله ReadLebUnsigned (u64* o_ارزښت, u32 i_max_عدد_بټونه, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_wasmUnderrun;

    u64 ارزښت = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بایټونه;

    طالما (ptr < i_پای_ټکی) {
        u64 byte = *(ptr++);

        ارزښت |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            پایله = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            إذا (M3_UNLIKELY(shift > i_max_عدد_بټونه)) {
                u32 عدد_کارول_شوی_بټونه = i_max_عدد_بټونه + 7 - shift;

                إذا (byte >> عدد_کارول_شوی_بټونه) {
                    پایله = m3_Err_leb_ډکېدنه;
                }
            }
            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_عدد_بټونه)) {
            پایله = m3_Err_leb_ډکېدنه;
            اكسر;
        }
    }

    *o_ارزښت  = ارزښت;
    *io_بایټونه = ptr;

    أرجع پایله;
}


M3_پایله ReadLebSigned (i64* o_ارزښت, u32 i_max_عدد_بټونه, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_wasmUnderrun;

    i64 ارزښت = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بایټونه;

    طالما (ptr < i_پای_ټکی) {
        u64 byte = *(ptr++);

        ارزښت |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            پایله = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            إذا (M3_UNLIKELY(shift > i_max_عدد_بټونه)) {
                u32 عدد_کارول_شوی_بټونه = i_max_عدد_بټونه + 7 - shift;
                u8  sign_بټونه    = (u8)((0x7f << (عدد_کارول_شوی_بټونه - 1)) & 0x7f);
                u8  بټونه        = (u8)(byte & sign_بټونه);

                إذا (بټونه != 0 and بټونه != sign_بټونه) {
                    پایله = m3_Err_leb_ډکېدنه;
                }
            }
            إذا ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                ارزښت |= (~extend << shift);
            }

            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_عدد_بټونه)) {
            پایله = m3_Err_leb_ډکېدنه;
            اكسر;
        }
    }

    *o_ارزښت  = ارزښت;
    *io_بایټونه = ptr;

    أرجع پایله;
}


M3_پایله ReadLEB_u32 (u32* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    u64      ارزښت;
    M3_پایله پایله = ReadLebUnsigned(&ارزښت, 32, io_بایټونه, i_پای_ټکی);

    *o_ارزښت = (u32)ارزښت;

    أرجع پایله;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#تعریف_کړه d_mem_Arg_Has_حافظه_Idx    0x40u

M3_پایله Read_حافظه_Arg (u32* o_align, u32* o_حافظه_Idx, u64* o_دننه_واټن, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله;

    u32 align;
    پایله = ReadLEB_u32(&align, io_بایټونه, i_پای_ټکی);
    إذا (پایله) {
        أرجع پایله;
    }

    *o_حافظه_Idx = 0;

#إذا d_m3_Has_Multi_حافظه
    إذا (align & d_mem_Arg_Has_حافظه_Idx) {
        align &= ~d_mem_Arg_Has_حافظه_Idx;

        پایله = ReadLEB_u32(o_حافظه_Idx, io_بایټونه, i_پای_ټکی);
        إذا (پایله) {
            أرجع پایله;
        }
    }
#د_که_پای

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    أرجع ReadLebUnsigned(o_دننه_واټن, 64, io_بایټونه, i_پای_ټکی);
}


M3_پایله ReadLEB_u7 (u8* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    u64      ارزښت;
    M3_پایله پایله = ReadLebUnsigned(&ارزښت, 7, io_بایټونه, i_پای_ټکی);

    *o_ارزښت = (u8)ارزښت;

    أرجع پایله;
}


M3_پایله ReadLEB_i7 (i8* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    i64      ارزښت;
    M3_پایله پایله = ReadLebSigned(&ارزښت, 7, io_بایټونه, i_پای_ټکی);

    *o_ارزښت = (i8)ارزښت;

    أرجع پایله;
}


M3_پایله ReadLEB_i32 (i32* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    i64      ارزښت;
    M3_پایله پایله = ReadLebSigned(&ارزښت, 32, io_بایټونه, i_پای_ټکی);

    *o_ارزښت = (i32)ارزښت;

    أرجع پایله;
}


M3_پایله ReadLEB_i64 (i64* o_ارزښت, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    i64      ارزښت;
    M3_پایله پایله = ReadLebSigned(&ارزښت, 64, io_بایټونه, i_پای_ټکی);

    *o_ارزښت = ارزښت;

    أرجع پایله;
}

#إذا d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ساكن
bool IsValidUtf8 (ثابت u8* i_معلومات, u32 i_اوږدتیا)
{
    ثابت u8* ptr = i_معلومات;
    ثابت u8* پای_ټکی = i_معلومات + i_اوږدتیا;

    طالما (ptr < پای_ټکی) {
        u8 b0 = *ptr++;

        إذا (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            استمر;
        } وإلا إذا ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            إذا (b0 < 0xC2) {
                أرجع false;       // overlong
            }
            إذا (ptr >= پای_ټکی) {
                أرجع false;
            }
            u8 b1 = *ptr++;
            إذا ((b1 & 0xC0) != 0x80) {
                أرجع false;
            }
        } وإلا إذا ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            إذا (ptr + 1 >= پای_ټکی) {
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
            إذا (ptr + 2 >= پای_ټکی) {
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
#د_که_پای // d_m3EnableValidation


M3_پایله Read_utf8 (cstr_t* o_utf8, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    *o_utf8 = عدم;

    u32      utf8_اوږدتیا;
    M3_پایله پایله = ReadLEB_u32(&utf8_اوږدتیا, io_بایټونه, i_پای_ټکی);

    إذا (not پایله) {
        إذا (utf8_اوږدتیا <= d_m3_Max_Sane_Utf8_اوږدتیا) {
            ثابت u8* ptr = *io_بایټونه;
            إذا (utf8_اوږدتیا <= (size_t)(i_پای_ټکی - ptr)) {
                ثابت u8* پای_ټکی = ptr + utf8_اوږدتیا;
#إذا d_m3EnableValidation
                إذا (not IsValidUtf8(ptr, utf8_اوږدتیا)) {
                    *io_بایټونه = پای_ټکی;
                    أرجع m3Err_wasmMalformed;
                }
#د_که_پای // d_m3EnableValidation

                حرف* utf8 = (حرف*)m3_Malloc("UTF8", utf8_اوږدتیا + 1);

                إذا (utf8) {
                    حافظه_نقل(utf8, ptr, utf8_اوږدتیا);
                    utf8[utf8_اوږدتیا] = 0;

                    *o_utf8 = utf8;
                } وإلا {
                    پایله = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_بایټونه = پای_ټکی;
            } وإلا {
                پایله = m3Err_wasmUnderrun;
            }
        } وإلا {
            پایله = m3Err_missingUTF8;
        }
    }

    أرجع پایله;
}

#إذا d_m3_ریکارډ_Backtraces
u32 پیدا_کړه_Module_دننه_واټن (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_کوډ_پاڼه curr = i_runtime->پاڼې_Open;

    bool پاڼه_وموندل = false;

    طالما (curr) {
        إذا (لري_PC(curr, i_pc)) {
            پاڼه_وموندل = true;
            اكسر;
        }
        curr = curr->info.بل;
    }

    إذا (!پاڼه_وموندل) {
        curr = i_runtime->پاڼې_ډک;
        طالما (curr) {
            إذا (لري_PC(curr, i_pc)) {
                پاڼه_وموندل = true;
                اكسر;
            }
            curr = curr->info.بل;
        }
    }

    إذا (پاڼه_وموندل) {
        u32 پایله = 0;

        bool pc_وموندل = Map_PCTo_دننه_واټن(curr, i_pc, &پایله);
        (فراغ)pc_وموندل;                                                          d_m3Assert (pc_وموندل);

        أرجع پایله;
    } وإلا {
        أرجع 0;
    }
}


فراغ ټېل_Backtrace_چوکاټ (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    إذا (M3_UNLIKELY(io_runtime->backtrace.وروستی_چوکاټ == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    M3_Backtrace_چوکاټ* new_چوکاټ = m3_AllocStruct(M3_Backtrace_چوکاټ);

    إذا (!new_چوکاټ) {
        io_runtime->backtrace.وروستی_چوکاټ = M3_BACKTRACE_TRUNCATED;
        أرجع;
    }

    new_چوکاټ->module_دننه_واټن = پیدا_کړه_Module_دننه_واټن(io_runtime, i_pc);

    إذا (!io_runtime->backtrace.چوکاټونه || !io_runtime->backtrace.وروستی_چوکاټ) {
        io_runtime->backtrace.چوکاټونه = new_چوکاټ;
    } وإلا {
        io_runtime->backtrace.وروستی_چوکاټ->بل = new_چوکاټ;
    }
    io_runtime->backtrace.وروستی_چوکاټ = new_چوکاټ;
}


فراغ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    إذا (M3_UNLIKELY(io_runtime->backtrace.وروستی_چوکاټ == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    إذا (!io_runtime->backtrace.وروستی_چوکاټ) {
        أرجع;
    }

    io_runtime->backtrace.وروستی_چوکاټ->function = i_function;
}


فراغ ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_چوکاټ* current_چوکاټ = io_runtime->backtrace.چوکاټونه;
    طالما (current_چوکاټ) {
        M3_Backtrace_چوکاټ* بل_چوکاټ = current_چوکاټ->بل;
        m3_Free(current_چوکاټ);
        current_چوکاټ = بل_چوکاټ;
    }

    io_runtime->backtrace.چوکاټونه    = عدم;
    io_runtime->backtrace.وروستی_چوکاټ = عدم;
}
#د_که_پای // d_m3RecordBacktraces
