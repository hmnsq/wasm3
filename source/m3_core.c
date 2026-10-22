//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ GNU_ਸਰੋਤ
#  define GNU_ਸਰੋਤ 1
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_IMPLEMENT_ERROR_ਲੜੀਆਂ
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_config.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "wasm3.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_host.h"

#ਜੇ d_m3HasWin32Host
#  include "m3_host_win32.h"
#ਜਾਂਜੇ d_m3HasPosixHost
#  include "m3_host_posix.h"
#ਨਹੀਂਤਾਂ
#  include "m3_host_none.h"
#ਜੇ_ਅੰਤ

ਖਾਲੀ m3_Abort (ਅਚਲ ਅੱਖਰ* ਸੁਨੇਹਾ)
{
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
    fprintf(stderr, "Error: %s\n", ਸੁਨੇਹਾ);
#ਜੇ_ਅੰਤ
    abort();
}

M3_WEAK
M3_ਨਤੀਜਾ m3_Yield ()
{
    ਵਾਪਸ m3Err_none;
}

#ਜੇ d_m3LogTimestamps

#  include <time.h>

#  define SEC_ਮੰਜ਼ਿਲ_US(sec) ((sec)*1000000)
#  define NS_ਮੰਜ਼ਿਲ_US(ns)    ((ns)/1000)

ਸਥਿਰ uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    ਜੇ (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    ਬਣਤਰ timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_ਮੰਜ਼ਿਲ_US((uint64_t)ts.tv_sec) + NS_ਮੰਜ਼ਿਲ_US((uint64_t)ts.tv_nsec);
    ਵਾਪਸ us - initial_ts;
}

#ਜੇ_ਅੰਤ

#ਜੇ d_m3_Fixed_ਢੇਰ

ਸਥਿਰ u8        fixed_ਢੇਰ[d_m3_Fixed_ਢੇਰ];
ਸਥਿਰ u8*       fixed_ਢੇਰ_Ptr  = fixed_ਢੇਰ;
ਸਥਿਰ u8* ਅਚਲ fixed_ਢੇਰ_ਸਿਰਾ  = fixed_ਢੇਰ + d_m3_Fixed_ਢੇਰ;
ਸਥਿਰ u8*       fixed_ਢੇਰ_ਆਖਰੀ = ਨਿਰਾ;

#  ਜੇ d_m3_Fixed_ਢੇਰ_Align > 1
#    define ਢੇਰ_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_ਢੇਰ_Align-1)) & ~ (d_m3_Fixed_ਢੇਰ_Align-1));
#  ਨਹੀਂਤਾਂ
#    define ਢੇਰ_ALIGN_PTR(P)
#  endif

ਖਾਲੀ* m3_Malloc_Impl (size_t i_ਮਾਪ)
{
    u8* ptr = fixed_ਢੇਰ_Ptr;

    fixed_ਢੇਰ_Ptr += i_ਮਾਪ;
    ਢੇਰ_ALIGN_PTR(fixed_ਢੇਰ_Ptr);

    ਜੇ (fixed_ਢੇਰ_Ptr >= fixed_ਢੇਰ_ਸਿਰਾ) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    memset(ptr, 0x0, i_ਮਾਪ);
    fixed_ਢੇਰ_ਆਖਰੀ = ptr;

    ਵਾਪਸ ptr;
}

ਖਾਲੀ m3_Free_Impl (ਖਾਲੀ* i_ptr)
{
    // Handle the last chunk
    ਜੇ (i_ptr && i_ptr == fixed_ਢੇਰ_ਆਖਰੀ) {
        fixed_ਢੇਰ_Ptr  = fixed_ਢੇਰ_ਆਖਰੀ;
        fixed_ਢੇਰ_ਆਖਰੀ = ਨਿਰਾ;
    } ਨਹੀਂਤਾਂ {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

ਖਾਲੀ* m3_Realloc_Impl (ਖਾਲੀ* i_ptr, size_t i_new_ਮਾਪ, size_t i_old_ਮਾਪ)
{
    ਜੇ (M3_UNLIKELY(i_new_ਮਾਪ == i_old_ਮਾਪ)) {
        ਵਾਪਸ i_ptr;
    }

    ਖਾਲੀ* newPtr;

    // Handle the last chunk
    ਜੇ (i_ptr && i_ptr == fixed_ਢੇਰ_ਆਖਰੀ) {
        fixed_ਢੇਰ_Ptr = fixed_ਢੇਰ_ਆਖਰੀ + i_new_ਮਾਪ;
        ਢੇਰ_ALIGN_PTR(fixed_ਢੇਰ_Ptr);
        ਜੇ (fixed_ਢੇਰ_Ptr >= fixed_ਢੇਰ_ਸਿਰਾ) {
            ਵਾਪਸ ਨਿਰਾ;
        }
        newPtr = i_ptr;
    } ਨਹੀਂਤਾਂ {
        newPtr = m3_Malloc_Impl(i_new_ਮਾਪ);
        ਜੇ (!newPtr) {
            ਵਾਪਸ ਨਿਰਾ;
        }
        ਜੇ (i_ptr) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(newPtr, i_ptr, i_old_ਮਾਪ);
        }
    }

    ਜੇ (i_new_ਮਾਪ > i_old_ਮਾਪ) {
        memset((u8*)newPtr + i_old_ਮਾਪ, 0x0, i_new_ਮਾਪ - i_old_ਮਾਪ);
    }

    ਵਾਪਸ newPtr;
}

#ਨਹੀਂਤਾਂ

ਖਾਲੀ* m3_Malloc_Impl (size_t i_ਮਾਪ)
{
    ਵਾਪਸ ਸਿਫਰ_ਸਮ੍ਰਿਤੀ_ਦਿਓ(i_ਮਾਪ, 1);
}

ਖਾਲੀ m3_Free_Impl (ਖਾਲੀ* io_ptr)
{
    ਮੁਕਤ(io_ptr);
}

ਖਾਲੀ* m3_Realloc_Impl (ਖਾਲੀ* i_ptr, size_t i_new_ਮਾਪ, size_t i_old_ਮਾਪ)
{
    ਜੇ (M3_UNLIKELY(i_new_ਮਾਪ == i_old_ਮਾਪ)) {
        ਵਾਪਸ i_ptr;
    }

    ਖਾਲੀ* newPtr = realloc(i_ptr, i_new_ਮਾਪ);

    ਜੇ (M3_LIKELY(newPtr)) {
        ਜੇ (i_new_ਮਾਪ > i_old_ਮਾਪ) {
            memset((u8*)newPtr + i_old_ਮਾਪ, 0x0, i_new_ਮਾਪ - i_old_ਮਾਪ);
        }
        ਵਾਪਸ newPtr;
    }
    ਵਾਪਸ ਨਿਰਾ;
}

#ਜੇ_ਅੰਤ

ਖਾਲੀ* m3_CopyMem (ਅਚਲ ਖਾਲੀ* i_from, size_t i_ਮਾਪ)
{
    ਖਾਲੀ* ptr = m3_Malloc("CopyMem", i_ਮਾਪ);
    ਜੇ (ptr) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(ptr, i_from, i_ਮਾਪ);
    }
    ਵਾਪਸ ptr;
}

//--------------------------------------------------------------------------------------------

#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ਸਥਿਰ u8*    g_guardArena;
ਸਥਿਰ size_t g_guard_Arena_ਬਾਈਟਾਂ;
ਸਥਿਰ size_t g_guard_ਥਾਂ_ਬਾਈਟਾਂ;
ਸਥਿਰ size_t g_guard_ਪੰਨਾ_ਮਾਪ;
ਸਥਿਰ u32    g_guard_ਥਾਂ_ਗਿਣਤੀ;
ਸਥਿਰ bool   g_guard_ਥਾਂ_ਲਿਆ[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
ਸਥਿਰ
bool Guard_Reserve (ਖਾਲੀ)
{
    ਜੇ (g_guardArena) {
        ਵਾਪਸ true;
    }

    size_t ਪੰਨਾ_ਮਾਪ = m3_Host_ਪੰਨਾ_ਮਾਪ();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t ਥਾਂ_ਬਾਈਟਾਂ = 2 * ਪੰਨਾ_ਮਾਪ + (size_t)d_m3_Guarded_ਅੰਕੜੇ_ਬਾਈਟਾਂ;

    ਲਈ (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        ਖਾਲੀ* ਅਧਾਰ = m3_HostReserve(ਥਾਂ_ਬਾਈਟਾਂ * slots);

        ਜੇ (ਅਧਾਰ) {
            g_guardArena      = (u8*)ਅਧਾਰ;
            g_guard_Arena_ਬਾਈਟਾਂ = ਥਾਂ_ਬਾਈਟਾਂ * slots;
            g_guard_ਥਾਂ_ਬਾਈਟਾਂ  = ਥਾਂ_ਬਾਈਟਾਂ;
            g_guard_ਪੰਨਾ_ਮਾਪ   = ਪੰਨਾ_ਮਾਪ;
            g_guard_ਥਾਂ_ਗਿਣਤੀ  = slots;
            ਵਾਪਸ true;
        }
    }

    ਵਾਪਸ false;
}

ਖਾਲੀ* Guard_Take_ਥਾਂ (ਖਾਲੀ)
{
    ਜੇ (not Guard_Reserve()) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਲਈ (u32 i = 0; i < g_guard_ਥਾਂ_ਗਿਣਤੀ; ++i) {
        ਜੇ (g_guard_ਥਾਂ_ਲਿਆ[i]) {
            ਜਾਰੀ;
        }

        u8* ਥਾਂ = g_guardArena + (size_t)i * g_guard_ਥਾਂ_ਬਾਈਟਾਂ;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        ਜੇ (not m3_HostCommit(ਥਾਂ, g_guard_ਪੰਨਾ_ਮਾਪ)) {
            ਵਾਪਸ ਨਿਰਾ;
        }

        g_guard_ਥਾਂ_ਲਿਆ[i] = true;
        ਵਾਪਸ ਥਾਂ;
    }

    ਵਾਪਸ ਨਿਰਾ;
}

M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* Guard_ਥਾਂ_ਸਿਰਲੇਖ (ਖਾਲੀ* i_ਥਾਂ)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* ਅੰਕੜੇ = (u8*)i_ਥਾਂ + g_guard_ਪੰਨਾ_ਮਾਪ;

    ਵਾਪਸ (M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ*)(ਅੰਕੜੇ - ਆਕਾਰ(M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ));
}

bool Guard_Commit_ਥਾਂ (ਖਾਲੀ* i_ਥਾਂ, size_t i_ਅੰਕੜੇ_ਬਾਈਟਾਂ)
{
    size_t rounded = (i_ਅੰਕੜੇ_ਬਾਈਟਾਂ + g_guard_ਪੰਨਾ_ਮਾਪ - 1) / g_guard_ਪੰਨਾ_ਮਾਪ * g_guard_ਪੰਨਾ_ਮਾਪ;

    ਵਾਪਸ m3_HostCommit((u8*)i_ਥਾਂ + g_guard_ਪੰਨਾ_ਮਾਪ, rounded);
}

ਖਾਲੀ Guard_Give_ਥਾਂ (ਖਾਲੀ* i_ਥਾਂ)
{
    ਜੇ (i_ਥਾਂ == ਨਿਰਾ) {
        ਵਾਪਸ;
    }

    size_t index = (size_t)((u8*)i_ਥਾਂ - g_guardArena) / g_guard_ਥਾਂ_ਬਾਈਟਾਂ;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    ਜੇ (m3_HostDecommit(i_ਥਾਂ, g_guard_ਥਾਂ_ਬਾਈਟਾਂ) and index < g_guard_ਥਾਂ_ਗਿਣਤੀ) {
        g_guard_ਥਾਂ_ਲਿਆ[index] = false;
    }
}

ਖਾਲੀ Guard_ArenaRange (ਖਾਲੀ** o_ਨੀਵਾਂ, size_t* o_ਬਾਈਟਾਂ)
{
    *o_ਨੀਵਾਂ   = g_guardArena;
    *o_ਬਾਈਟਾਂ = g_guard_Arena_ਬਾਈਟਾਂ;
}

#ਜੇ_ਅੰਤ // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#ਜੇ d_m3_Max_Native_ਢੇਰ > 0

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
ਖਾਲੀ* m3_Native_ਢੇਰ_ਹੱਦਬੰਦੀ (ਖਾਲੀ* i_ਢੇਰ_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ਢੇਰ_Ptr;

#  ਜੇ M3_HAS_THREAD_LOCAL
    ਸਥਿਰ M3_THREAD_LOCAL u8* ਅਧਾਰ = ਨਿਰਾ;
    ਜੇ (ਅਧਾਰ == ਨਿਰਾ) {
        ਅਧਾਰ = (u8*)m3_Host_ਢੇਰ_ਅਧਾਰ();
    }
#  ਨਹੀਂਤਾਂ
    u8* ਅਧਾਰ = (u8*)m3_Host_ਢੇਰ_ਅਧਾਰ();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    ਜੇ (ਅਧਾਰ and ਅਧਾਰ < sp) {
        size_t ਉਪਲਬਧ = (size_t)(sp - ਅਧਾਰ);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ਢੇਰ_Margin), ਉਪਲਬਧ / 4);

        i_budget = M3_MIN(i_budget, ਉਪਲਬਧ - margin);
    }

    ਵਾਪਸ sp - i_budget;
}

#ਜੇ_ਅੰਤ // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#ਜੇ d_m3_Log_Native_ਢੇਰ

ਸਥਿਰ size_t ਢੇਰ_ਸ਼ੁਰੂ;
ਸਥਿਰ size_t ਢੇਰ_ਸਿਰਾ;

ਖਾਲੀ m3_ਢੇਰ_Check_Init ()
{
    ਅੱਖਰ ਢੇਰ;
    ਢੇਰ_ਸਿਰਾ = ਢੇਰ_ਸ਼ੁਰੂ = (size_t)&ਢੇਰ;
}

ਖਾਲੀ m3_ਢੇਰ_Check ()
{
    ਅੱਖਰ   ਢੇਰ;
    size_t addr = (size_t)&ਢੇਰ;

    size_t ਢੇਰ_ਸਿਰਾ_2 = ਢੇਰ_ਸਿਰਾ;

    ਢੇਰ_ਸਿਰਾ = M3_MIN(ਢੇਰ_ਸਿਰਾ, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

ਪੂਰਨ m3_ਢੇਰ_Get_Max ()
{
    ਵਾਪਸ ਢੇਰ_ਸ਼ੁਰੂ - ਢੇਰ_ਸਿਰਾ;
}

#ਜੇ_ਅੰਤ

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_ਨਤੀਜਾ NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u8 type = -i_convolutedWasmType;

    ਜੇ (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    ਨਹੀਂਤਾਂ ਜੇ (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } ਨਹੀਂਤਾਂ ਜੇ (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#ਜੇ d_m3HasExceptionHandling
    ਨਹੀਂਤਾਂ ਜੇ (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਨਹੀਂਤਾਂ ਜੇ (type == d_wa_Type_ਢੇਰ_cont or type == d_wa_Type_ਢੇਰ_nocont) {
        type = c_m3Type_contref;
    }
#ਜੇ_ਅੰਤ
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    ਨਹੀਂਤਾਂ ਜੇ (type < c_m3Type_i32 or type > c_m3Type_v128) {
        ਨਤੀਜਾ = m3Err_invalidTypeId;
    }

    *o_type = type;

    ਵਾਪਸ ਨਤੀਜਾ;
}


#ਜੇ d_m3HasTypedRefs

u8 ਅਧਾਰ_Type_ਦਾ (m3type_t i_type)
{
    ਜੇ (IsSpelledRefType(i_type)) {
        ਜੇ (i_type & d_m3Type_refCont) {
            ਵਾਪਸ c_m3Type_contref;
        }
        ਵਾਪਸ (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    ਵਾਪਸ (u8)i_type;
}


bool Is_Sub_Type_ਦਾ (m3type_t i_sub, m3type_t i_super)
{
    ਜੇ (i_sub == i_super) {
        ਵਾਪਸ true;
    }

    u8 ਅਧਾਰ = ਅਧਾਰ_Type_ਦਾ(i_sub);

    // only references have a subtype relation; every other type is invariant
    ਜੇ (ਅਧਾਰ != ਅਧਾਰ_Type_ਦਾ(i_super) or not IsRefType(ਅਧਾਰ)) {
        ਵਾਪਸ false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    ਜੇ (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        ਵਾਪਸ false;
    }

#  ਜੇ d_m3_Has_ਢੇਰ_Switching
    // nocont sits under every continuation type
    ਜੇ (ਢੇਰ_Type_ਦਾ(i_sub) == d_m3_Type_ਢੇਰ_None) {
        ਵਾਪਸ true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    ਵਾਪਸ (ਢੇਰ_Type_ਦਾ(i_super) == d_m3_Type_ਢੇਰ_Abstract) or
           (ਢੇਰ_Type_ਦਾ(i_sub) == ਢੇਰ_Type_ਦਾ(i_super));
}

#ਜੇ_ਅੰਤ // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = ਅਧਾਰ_Type_ਦਾ(i_type);
    ਵਾਪਸ (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = ਅਧਾਰ_Type_ਦਾ(i_type);
    ਵਾਪਸ (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#ਜੇ d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
            or i_m3Type == c_m3Type_contref
#ਜੇ_ਅੰਤ
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = ਅਧਾਰ_Type_ਦਾ(i_type);
    ਵਾਪਸ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type (m3type_t i_type)
{
    u8 i_m3Type = ਅਧਾਰ_Type_ਦਾ(i_type);

    ਜੇ (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        ਵਾਪਸ true;
    } ਨਹੀਂਤਾਂ ਜੇ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        ਵਾਪਸ false;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ (ਆਕਾਰ(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 ਮਾਪ_ਦਾ_Type (m3type_t i_type)
{
    u8 i_m3Type = ਅਧਾਰ_Type_ਦਾ(i_type);

    ਜੇ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        ਵਾਪਸ ਆਕਾਰ(i32);
    }

    ਜੇ (IsRefType(i_m3Type)) {
        ਵਾਪਸ ਆਕਾਰ(ਖਾਲੀ*);
    }

    ਵਾਪਸ ਆਕਾਰ(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_ਨਤੀਜਾ Read_u64 (u64* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    ਅਚਲ u8* ptr = *io_ਬਾਈਟਾਂ;
    ptr += ਆਕਾਰ(u64);

    ਜੇ (M3_LIKELY(ptr <= i_ਸਿਰਾ)) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਮੁੱਲ, *io_ਬਾਈਟਾਂ, ਆਕਾਰ(u64));
        M3_BSWAP_u64(*o_ਮੁੱਲ);
        *io_ਬਾਈਟਾਂ = ptr;
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
}


M3_ਨਤੀਜਾ Read_u32 (u32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    ਅਚਲ u8* ptr = *io_ਬਾਈਟਾਂ;
    ptr += ਆਕਾਰ(u32);

    ਜੇ (M3_LIKELY(ptr <= i_ਸਿਰਾ)) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਮੁੱਲ, *io_ਬਾਈਟਾਂ, ਆਕਾਰ(u32));
        M3_BSWAP_u32(*o_ਮੁੱਲ);
        *io_ਬਾਈਟਾਂ = ptr;
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
}

#ਜੇ d_m3ImplementFloat

M3_ਨਤੀਜਾ Read_f64 (f64* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    ਅਚਲ u8* ptr = *io_ਬਾਈਟਾਂ;
    ptr += ਆਕਾਰ(f64);

    ਜੇ (M3_LIKELY(ptr <= i_ਸਿਰਾ)) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਮੁੱਲ, *io_ਬਾਈਟਾਂ, ਆਕਾਰ(f64));
        M3_BSWAP_f64(*o_ਮੁੱਲ);
        *io_ਬਾਈਟਾਂ = ptr;
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
}


M3_ਨਤੀਜਾ Read_f32 (f32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    ਅਚਲ u8* ptr = *io_ਬਾਈਟਾਂ;
    ptr += ਆਕਾਰ(f32);

    ਜੇ (M3_LIKELY(ptr <= i_ਸਿਰਾ)) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਮੁੱਲ, *io_ਬਾਈਟਾਂ, ਆਕਾਰ(f32));
        M3_BSWAP_f32(*o_ਮੁੱਲ);
        *io_ਬਾਈਟਾਂ = ptr;
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
}

#ਜੇ_ਅੰਤ

M3_ਨਤੀਜਾ Read_u8 (u8* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    ਅਚਲ u8* ptr = *io_ਬਾਈਟਾਂ;

    ਜੇ (M3_LIKELY(ptr < i_ਸਿਰਾ)) {
        *o_ਮੁੱਲ  = *ptr;
        *io_ਬਾਈਟਾਂ = ptr + 1;

        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
}

M3_ਨਤੀਜਾ Read_opcode (m3opcode_t* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    ਅਚਲ u8* ptr = *io_ਬਾਈਟਾਂ;

    ਜੇ (M3_LIKELY(ptr < i_ਸਿਰਾ)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_ਮੁੱਲ  = opcode;
        *io_ਬਾਈਟਾਂ = ptr;

        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
}


M3_ਨਤੀਜਾ ReadLebUnsigned (u64* o_ਮੁੱਲ, u32 i_max_ਅੰਕ_ਬਿੱਟ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_wasmUnderrun;

    u64 ਮੁੱਲ = 0;

    u32       shift = 0;
    ਅਚਲ u8* ptr   = *io_ਬਾਈਟਾਂ;

    ਜਦੋਂ (ptr < i_ਸਿਰਾ) {
        u64 byte = *(ptr++);

        ਮੁੱਲ |= ((byte & 0x7f) << shift);
        shift += 7;

        ਜੇ (M3_LIKELY((byte & 0x80) == 0)) {
            ਨਤੀਜਾ = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            ਜੇ (M3_UNLIKELY(shift > i_max_ਅੰਕ_ਬਿੱਟ)) {
                u32 ਅੰਕ_ਵਰਤਿਆ_ਬਿੱਟ = i_max_ਅੰਕ_ਬਿੱਟ + 7 - shift;

                ਜੇ (byte >> ਅੰਕ_ਵਰਤਿਆ_ਬਿੱਟ) {
                    ਨਤੀਜਾ = m3_Err_leb_ਉਛਾਲ;
                }
            }
            ਰੋਕੋ;
        }

        ਜੇ (M3_UNLIKELY(shift >= i_max_ਅੰਕ_ਬਿੱਟ)) {
            ਨਤੀਜਾ = m3_Err_leb_ਉਛਾਲ;
            ਰੋਕੋ;
        }
    }

    *o_ਮੁੱਲ  = ਮੁੱਲ;
    *io_ਬਾਈਟਾਂ = ptr;

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ReadLebSigned (i64* o_ਮੁੱਲ, u32 i_max_ਅੰਕ_ਬਿੱਟ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_wasmUnderrun;

    i64 ਮੁੱਲ = 0;

    u32       shift = 0;
    ਅਚਲ u8* ptr   = *io_ਬਾਈਟਾਂ;

    ਜਦੋਂ (ptr < i_ਸਿਰਾ) {
        u64 byte = *(ptr++);

        ਮੁੱਲ |= ((byte & 0x7f) << shift);
        shift += 7;

        ਜੇ (M3_LIKELY((byte & 0x80) == 0)) {
            ਨਤੀਜਾ = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            ਜੇ (M3_UNLIKELY(shift > i_max_ਅੰਕ_ਬਿੱਟ)) {
                u32 ਅੰਕ_ਵਰਤਿਆ_ਬਿੱਟ = i_max_ਅੰਕ_ਬਿੱਟ + 7 - shift;
                u8  sign_ਬਿੱਟ    = (u8)((0x7f << (ਅੰਕ_ਵਰਤਿਆ_ਬਿੱਟ - 1)) & 0x7f);
                u8  ਬਿੱਟ        = (u8)(byte & sign_ਬਿੱਟ);

                ਜੇ (ਬਿੱਟ != 0 and ਬਿੱਟ != sign_ਬਿੱਟ) {
                    ਨਤੀਜਾ = m3_Err_leb_ਉਛਾਲ;
                }
            }
            ਜੇ ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                ਮੁੱਲ |= (~extend << shift);
            }

            ਰੋਕੋ;
        }

        ਜੇ (M3_UNLIKELY(shift >= i_max_ਅੰਕ_ਬਿੱਟ)) {
            ਨਤੀਜਾ = m3_Err_leb_ਉਛਾਲ;
            ਰੋਕੋ;
        }
    }

    *o_ਮੁੱਲ  = ਮੁੱਲ;
    *io_ਬਾਈਟਾਂ = ptr;

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ReadLEB_u32 (u32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    u64      ਮੁੱਲ;
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ReadLebUnsigned(&ਮੁੱਲ, 32, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);

    *o_ਮੁੱਲ = (u32)ਮੁੱਲ;

    ਵਾਪਸ ਨਤੀਜਾ;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_mem_Arg_Has_ਸਮ੍ਰਿਤੀ_Idx    0x40u

M3_ਨਤੀਜਾ Read_ਸਮ੍ਰਿਤੀ_Arg (u32* o_align, u32* o_ਸਮ੍ਰਿਤੀ_Idx, u64* o_ਵਿੱਥ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    u32 align;
    ਨਤੀਜਾ = ReadLEB_u32(&align, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);
    ਜੇ (ਨਤੀਜਾ) {
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    *o_ਸਮ੍ਰਿਤੀ_Idx = 0;

#ਜੇ d_m3_Has_Multi_ਸਮ੍ਰਿਤੀ
    ਜੇ (align & d_mem_Arg_Has_ਸਮ੍ਰਿਤੀ_Idx) {
        align &= ~d_mem_Arg_Has_ਸਮ੍ਰਿਤੀ_Idx;

        ਨਤੀਜਾ = ReadLEB_u32(o_ਸਮ੍ਰਿਤੀ_Idx, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);
        ਜੇ (ਨਤੀਜਾ) {
            ਵਾਪਸ ਨਤੀਜਾ;
        }
    }
#ਜੇ_ਅੰਤ

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    ਵਾਪਸ ReadLebUnsigned(o_ਵਿੱਥ, 64, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);
}


M3_ਨਤੀਜਾ ReadLEB_u7 (u8* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    u64      ਮੁੱਲ;
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ReadLebUnsigned(&ਮੁੱਲ, 7, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);

    *o_ਮੁੱਲ = (u8)ਮੁੱਲ;

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ReadLEB_i7 (i8* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    i64      ਮੁੱਲ;
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ReadLebSigned(&ਮੁੱਲ, 7, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);

    *o_ਮੁੱਲ = (i8)ਮੁੱਲ;

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ReadLEB_i32 (i32* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    i64      ਮੁੱਲ;
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ReadLebSigned(&ਮੁੱਲ, 32, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);

    *o_ਮੁੱਲ = (i32)ਮੁੱਲ;

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ReadLEB_i64 (i64* o_ਮੁੱਲ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    i64      ਮੁੱਲ;
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ReadLebSigned(&ਮੁੱਲ, 64, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);

    *o_ਮੁੱਲ = ਮੁੱਲ;

    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ਸਥਿਰ
bool IsValidUtf8 (ਅਚਲ u8* i_ਅੰਕੜੇ, u32 i_ਲੰਮਾਈ)
{
    ਅਚਲ u8* ptr = i_ਅੰਕੜੇ;
    ਅਚਲ u8* ਸਿਰਾ = i_ਅੰਕੜੇ + i_ਲੰਮਾਈ;

    ਜਦੋਂ (ptr < ਸਿਰਾ) {
        u8 b0 = *ptr++;

        ਜੇ (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            ਜਾਰੀ;
        } ਨਹੀਂਤਾਂ ਜੇ ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            ਜੇ (b0 < 0xC2) {
                ਵਾਪਸ false;       // overlong
            }
            ਜੇ (ptr >= ਸਿਰਾ) {
                ਵਾਪਸ false;
            }
            u8 b1 = *ptr++;
            ਜੇ ((b1 & 0xC0) != 0x80) {
                ਵਾਪਸ false;
            }
        } ਨਹੀਂਤਾਂ ਜੇ ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            ਜੇ (ptr + 1 >= ਸਿਰਾ) {
                ਵਾਪਸ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            ਜੇ ((b1 & 0xC0) != 0x80) {
                ਵਾਪਸ false;
            }
            ਜੇ ((b2 & 0xC0) != 0x80) {
                ਵਾਪਸ false;
            }
            // reject overlong
            ਜੇ (b0 == 0xE0 && b1 < 0xA0) {
                ਵਾਪਸ false;
            }
            // reject surrogates U+D800..U+DFFF
            ਜੇ (b0 == 0xED && b1 >= 0xA0) {
                ਵਾਪਸ false;
            }
        } ਨਹੀਂਤਾਂ ਜੇ ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            ਜੇ (b0 > 0xF4) {
                ਵਾਪਸ false;       // above U+10FFFF
            }
            ਜੇ (ptr + 2 >= ਸਿਰਾ) {
                ਵਾਪਸ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            ਜੇ ((b1 & 0xC0) != 0x80) {
                ਵਾਪਸ false;
            }
            ਜੇ ((b2 & 0xC0) != 0x80) {
                ਵਾਪਸ false;
            }
            ਜੇ ((b3 & 0xC0) != 0x80) {
                ਵਾਪਸ false;
            }
            // reject overlong
            ਜੇ (b0 == 0xF0 && b1 < 0x90) {
                ਵਾਪਸ false;
            }
            // reject above U+10FFFF
            ਜੇ (b0 == 0xF4 && b1 > 0x8F) {
                ਵਾਪਸ false;
            }
        } ਨਹੀਂਤਾਂ {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            ਵਾਪਸ false;
        }
    }

    ਵਾਪਸ true;
}
#ਜੇ_ਅੰਤ // d_m3EnableValidation


M3_ਨਤੀਜਾ Read_utf8 (cstr_t* o_utf8, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    *o_utf8 = ਨਿਰਾ;

    u32      utf8_ਲੰਮਾਈ;
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = ReadLEB_u32(&utf8_ਲੰਮਾਈ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);

    ਜੇ (not ਨਤੀਜਾ) {
        ਜੇ (utf8_ਲੰਮਾਈ <= d_m3_Max_Sane_Utf8_ਲੰਮਾਈ) {
            ਅਚਲ u8* ptr = *io_ਬਾਈਟਾਂ;
            ਜੇ (utf8_ਲੰਮਾਈ <= (size_t)(i_ਸਿਰਾ - ptr)) {
                ਅਚਲ u8* ਸਿਰਾ = ptr + utf8_ਲੰਮਾਈ;
#ਜੇ d_m3EnableValidation
                ਜੇ (not IsValidUtf8(ptr, utf8_ਲੰਮਾਈ)) {
                    *io_ਬਾਈਟਾਂ = ਸਿਰਾ;
                    ਵਾਪਸ m3Err_wasmMalformed;
                }
#ਜੇ_ਅੰਤ // d_m3EnableValidation

                ਅੱਖਰ* utf8 = (ਅੱਖਰ*)m3_Malloc("UTF8", utf8_ਲੰਮਾਈ + 1);

                ਜੇ (utf8) {
                    ਸਮ੍ਰਿਤੀ_ਨਕਲ(utf8, ptr, utf8_ਲੰਮਾਈ);
                    utf8[utf8_ਲੰਮਾਈ] = 0;

                    *o_utf8 = utf8;
                } ਨਹੀਂਤਾਂ {
                    ਨਤੀਜਾ = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_ਬਾਈਟਾਂ = ਸਿਰਾ;
            } ਨਹੀਂਤਾਂ {
                ਨਤੀਜਾ = m3Err_wasmUnderrun;
            }
        } ਨਹੀਂਤਾਂ {
            ਨਤੀਜਾ = m3Err_missingUTF8;
        }
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
u32 ਲੱਭੋ_Module_ਵਿੱਥ (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_ਕੋਡ_ਪੰਨਾ curr = i_runtime->ਪੰਨੇ_Open;

    bool ਪੰਨਾ_ਮਿਲਿਆ = false;

    ਜਦੋਂ (curr) {
        ਜੇ (ਸ਼ਾਮਲ_ਹੈ_PC(curr, i_pc)) {
            ਪੰਨਾ_ਮਿਲਿਆ = true;
            ਰੋਕੋ;
        }
        curr = curr->info.ਅਗਲਾ;
    }

    ਜੇ (!ਪੰਨਾ_ਮਿਲਿਆ) {
        curr = i_runtime->ਪੰਨੇ_ਭਰਿਆ;
        ਜਦੋਂ (curr) {
            ਜੇ (ਸ਼ਾਮਲ_ਹੈ_PC(curr, i_pc)) {
                ਪੰਨਾ_ਮਿਲਿਆ = true;
                ਰੋਕੋ;
            }
            curr = curr->info.ਅਗਲਾ;
        }
    }

    ਜੇ (ਪੰਨਾ_ਮਿਲਿਆ) {
        u32 ਨਤੀਜਾ = 0;

        bool pc_ਮਿਲਿਆ = Map_PCTo_ਵਿੱਥ(curr, i_pc, &ਨਤੀਜਾ);
        (ਖਾਲੀ)pc_ਮਿਲਿਆ;                                                          d_m3Assert (pc_ਮਿਲਿਆ);

        ਵਾਪਸ ਨਤੀਜਾ;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ 0;
    }
}


ਖਾਲੀ ਧੱਕੋ_Backtrace_ਚੌਖਟਾ (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    ਜੇ (M3_UNLIKELY(io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ == M3_BACKTRACE_TRUNCATED)) {
        ਵਾਪਸ;
    }

    M3_Backtrace_ਚੌਖਟਾ* new_ਚੌਖਟਾ = m3_AllocStruct(M3_Backtrace_ਚੌਖਟਾ);

    ਜੇ (!new_ਚੌਖਟਾ) {
        io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ = M3_BACKTRACE_TRUNCATED;
        ਵਾਪਸ;
    }

    new_ਚੌਖਟਾ->module_ਵਿੱਥ = ਲੱਭੋ_Module_ਵਿੱਥ(io_runtime, i_pc);

    ਜੇ (!io_runtime->backtrace.ਚੌਖਟੇ || !io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ) {
        io_runtime->backtrace.ਚੌਖਟੇ = new_ਚੌਖਟਾ;
    } ਨਹੀਂਤਾਂ {
        io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ->ਅਗਲਾ = new_ਚੌਖਟਾ;
    }
    io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ = new_ਚੌਖਟਾ;
}


ਖਾਲੀ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    ਜੇ (M3_UNLIKELY(io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ == M3_BACKTRACE_TRUNCATED)) {
        ਵਾਪਸ;
    }

    ਜੇ (!io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ) {
        ਵਾਪਸ;
    }

    io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ->function = i_function;
}


ਖਾਲੀ ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_ਚੌਖਟਾ* current_ਚੌਖਟਾ = io_runtime->backtrace.ਚੌਖਟੇ;
    ਜਦੋਂ (current_ਚੌਖਟਾ) {
        M3_Backtrace_ਚੌਖਟਾ* ਅਗਲਾ_ਚੌਖਟਾ = current_ਚੌਖਟਾ->ਅਗਲਾ;
        m3_Free(current_ਚੌਖਟਾ);
        current_ਚੌਖਟਾ = ਅਗਲਾ_ਚੌਖਟਾ;
    }

    io_runtime->backtrace.ਚੌਖਟੇ    = ਨਿਰਾ;
    io_runtime->backtrace.ਆਖਰੀ_ਚੌਖਟਾ = ਨਿਰਾ;
}
#ਜੇ_ਅੰਤ // d_m3RecordBacktraces
