//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ GNU_ಮೂಲ
#  define GNU_ಮೂಲ 1
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿಸು M3_IMPLEMENT_ERROR_ಅಕ್ಷರಸರಣಿಗಳು
#ಅನ್ತರ್ಭಾವ "m3_config.h"
#ಅನ್ತರ್ಭಾವ "wasm3.h"

#ಅನ್ತರ್ಭಾವ "m3_core.h"
#ಅನ್ತರ್ಭಾವ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#ಅನ್ತರ್ಭಾವ "m3_host.h"

#ಆದರೆ d_m3HasWin32Host
#  include "m3_host_win32.h"
#ಅಥವಾಆದರೆ d_m3HasPosixHost
#  include "m3_host_posix.h"
#ಇಲ್ಲದಿದ್ದರೆ
#  include "m3_host_none.h"
#ಆದರೆ_ಅಂತ್ಯ

ಶೂನ್ಯ m3_Abort (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸಂದೇಶ)
{
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ DEBUG
    fprintf(stderr, "Error: %s\n", ಸಂದೇಶ);
#ಆದರೆ_ಅಂತ್ಯ
    abort();
}

M3_WEAK
M3_ಫಲಿತಾಂಶ m3_Yield ()
{
    ಹಿಂತಿರುಗಿ m3Err_none;
}

#ಆದರೆ d_m3LogTimestamps

#  include <time.h>

#  define SEC_ಗಮ್ಯ_US(sec) ((sec)*1000000)
#  define NS_ಗಮ್ಯ_US(ns)    ((ns)/1000)

ಸ್ಥಿರ uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    ಆದರೆ (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    ರಚನೆ timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_ಗಮ್ಯ_US((uint64_t)ts.tv_sec) + NS_ಗಮ್ಯ_US((uint64_t)ts.tv_nsec);
    ಹಿಂತಿರುಗಿ us - initial_ts;
}

#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_Fixed_ರಾಶಿ

ಸ್ಥಿರ u8        fixed_ರಾಶಿ[d_m3_Fixed_ರಾಶಿ];
ಸ್ಥಿರ u8*       fixed_ರಾಶಿ_Ptr  = fixed_ರಾಶಿ;
ಸ್ಥಿರ u8* ಸ್ಥಿರಾಂಕ fixed_ರಾಶಿ_ಕೊನೆ  = fixed_ರಾಶಿ + d_m3_Fixed_ರಾಶಿ;
ಸ್ಥಿರ u8*       fixed_ರಾಶಿ_ಕೊನೆಯ = ಖಾಲಿ;

#  ಆದರೆ d_m3_Fixed_ರಾಶಿ_Align > 1
#    define ರಾಶಿ_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_ರಾಶಿ_Align-1)) & ~ (d_m3_Fixed_ರಾಶಿ_Align-1));
#  ಇಲ್ಲದಿದ್ದರೆ
#    define ರಾಶಿ_ALIGN_PTR(P)
#  endif

ಶೂನ್ಯ* m3_Malloc_Impl (size_t i_ಗಾತ್ರ)
{
    u8* ptr = fixed_ರಾಶಿ_Ptr;

    fixed_ರಾಶಿ_Ptr += i_ಗಾತ್ರ;
    ರಾಶಿ_ALIGN_PTR(fixed_ರಾಶಿ_Ptr);

    ಆದರೆ (fixed_ರಾಶಿ_Ptr >= fixed_ರಾಶಿ_ಕೊನೆ) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    memset(ptr, 0x0, i_ಗಾತ್ರ);
    fixed_ರಾಶಿ_ಕೊನೆಯ = ptr;

    ಹಿಂತಿರುಗಿ ptr;
}

ಶೂನ್ಯ m3_Free_Impl (ಶೂನ್ಯ* i_ptr)
{
    // Handle the last chunk
    ಆದರೆ (i_ptr && i_ptr == fixed_ರಾಶಿ_ಕೊನೆಯ) {
        fixed_ರಾಶಿ_Ptr  = fixed_ರಾಶಿ_ಕೊನೆಯ;
        fixed_ರಾಶಿ_ಕೊನೆಯ = ಖಾಲಿ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

ಶೂನ್ಯ* m3_Realloc_Impl (ಶೂನ್ಯ* i_ptr, size_t i_new_ಗಾತ್ರ, size_t i_old_ಗಾತ್ರ)
{
    ಆದರೆ (M3_UNLIKELY(i_new_ಗಾತ್ರ == i_old_ಗಾತ್ರ)) {
        ಹಿಂತಿರುಗಿ i_ptr;
    }

    ಶೂನ್ಯ* newPtr;

    // Handle the last chunk
    ಆದರೆ (i_ptr && i_ptr == fixed_ರಾಶಿ_ಕೊನೆಯ) {
        fixed_ರಾಶಿ_Ptr = fixed_ರಾಶಿ_ಕೊನೆಯ + i_new_ಗಾತ್ರ;
        ರಾಶಿ_ALIGN_PTR(fixed_ರಾಶಿ_Ptr);
        ಆದರೆ (fixed_ರಾಶಿ_Ptr >= fixed_ರಾಶಿ_ಕೊನೆ) {
            ಹಿಂತಿರುಗಿ ಖಾಲಿ;
        }
        newPtr = i_ptr;
    } ಇಲ್ಲದಿದ್ದರೆ {
        newPtr = m3_Malloc_Impl(i_new_ಗಾತ್ರ);
        ಆದರೆ (!newPtr) {
            ಹಿಂತಿರುಗಿ ಖಾಲಿ;
        }
        ಆದರೆ (i_ptr) {
            ಸ್ಮರಣೆ_ನಕಲು(newPtr, i_ptr, i_old_ಗಾತ್ರ);
        }
    }

    ಆದರೆ (i_new_ಗಾತ್ರ > i_old_ಗಾತ್ರ) {
        memset((u8*)newPtr + i_old_ಗಾತ್ರ, 0x0, i_new_ಗಾತ್ರ - i_old_ಗಾತ್ರ);
    }

    ಹಿಂತಿರುಗಿ newPtr;
}

#ಇಲ್ಲದಿದ್ದರೆ

ಶೂನ್ಯ* m3_Malloc_Impl (size_t i_ಗಾತ್ರ)
{
    ಹಿಂತಿರುಗಿ ಸೊನ್ನೆ_ಸ್ಮರಣೆ_ಕೊಡು(i_ಗಾತ್ರ, 1);
}

ಶೂನ್ಯ m3_Free_Impl (ಶೂನ್ಯ* io_ptr)
{
    ಬಿಡುಗಡೆ(io_ptr);
}

ಶೂನ್ಯ* m3_Realloc_Impl (ಶೂನ್ಯ* i_ptr, size_t i_new_ಗಾತ್ರ, size_t i_old_ಗಾತ್ರ)
{
    ಆದರೆ (M3_UNLIKELY(i_new_ಗಾತ್ರ == i_old_ಗಾತ್ರ)) {
        ಹಿಂತಿರುಗಿ i_ptr;
    }

    ಶೂನ್ಯ* newPtr = realloc(i_ptr, i_new_ಗಾತ್ರ);

    ಆದರೆ (M3_LIKELY(newPtr)) {
        ಆದರೆ (i_new_ಗಾತ್ರ > i_old_ಗಾತ್ರ) {
            memset((u8*)newPtr + i_old_ಗಾತ್ರ, 0x0, i_new_ಗಾತ್ರ - i_old_ಗಾತ್ರ);
        }
        ಹಿಂತಿರುಗಿ newPtr;
    }
    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

#ಆದರೆ_ಅಂತ್ಯ

ಶೂನ್ಯ* m3_CopyMem (ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_from, size_t i_ಗಾತ್ರ)
{
    ಶೂನ್ಯ* ptr = m3_Malloc("CopyMem", i_ಗಾತ್ರ);
    ಆದರೆ (ptr) {
        ಸ್ಮರಣೆ_ನಕಲು(ptr, i_from, i_ಗಾತ್ರ);
    }
    ಹಿಂತಿರುಗಿ ptr;
}

//--------------------------------------------------------------------------------------------

#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ಸ್ಥಿರ u8*    g_guardArena;
ಸ್ಥಿರ size_t g_guard_Arena_ಬೈಟುಗಳು;
ಸ್ಥಿರ size_t g_guard_ಸ್ಥಾನ_ಬೈಟುಗಳು;
ಸ್ಥಿರ size_t g_guard_ಪುಟ_ಗಾತ್ರ;
ಸ್ಥಿರ u32    g_guard_ಸ್ಥಾನ_ಎಣಿಕೆ;
ಸ್ಥಿರ bool   g_guard_ಸ್ಥಾನ_ತೆಗೆದುಕೊಂಡ[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
ಸ್ಥಿರ
bool Guard_Reserve (ಶೂನ್ಯ)
{
    ಆದರೆ (g_guardArena) {
        ಹಿಂತಿರುಗಿ true;
    }

    size_t ಪುಟ_ಗಾತ್ರ = m3_Host_ಪುಟ_ಗಾತ್ರ();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t ಸ್ಥಾನ_ಬೈಟುಗಳು = 2 * ಪುಟ_ಗಾತ್ರ + (size_t)d_m3_Guarded_ದತ್ತಾಂಶ_ಬೈಟುಗಳು;

    ಗಾಗಿ (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        ಶೂನ್ಯ* ತಳ = m3_HostReserve(ಸ್ಥಾನ_ಬೈಟುಗಳು * slots);

        ಆದರೆ (ತಳ) {
            g_guardArena      = (u8*)ತಳ;
            g_guard_Arena_ಬೈಟುಗಳು = ಸ್ಥಾನ_ಬೈಟುಗಳು * slots;
            g_guard_ಸ್ಥಾನ_ಬೈಟುಗಳು  = ಸ್ಥಾನ_ಬೈಟುಗಳು;
            g_guard_ಪುಟ_ಗಾತ್ರ   = ಪುಟ_ಗಾತ್ರ;
            g_guard_ಸ್ಥಾನ_ಎಣಿಕೆ  = slots;
            ಹಿಂತಿರುಗಿ true;
        }
    }

    ಹಿಂತಿರುಗಿ false;
}

ಶೂನ್ಯ* Guard_Take_ಸ್ಥಾನ (ಶೂನ್ಯ)
{
    ಆದರೆ (not Guard_Reserve()) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಗಾಗಿ (u32 i = 0; i < g_guard_ಸ್ಥಾನ_ಎಣಿಕೆ; ++i) {
        ಆದರೆ (g_guard_ಸ್ಥಾನ_ತೆಗೆದುಕೊಂಡ[i]) {
            ಮುಂದುವರಿ;
        }

        u8* ಸ್ಥಾನ = g_guardArena + (size_t)i * g_guard_ಸ್ಥಾನ_ಬೈಟುಗಳು;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        ಆದರೆ (not m3_HostCommit(ಸ್ಥಾನ, g_guard_ಪುಟ_ಗಾತ್ರ)) {
            ಹಿಂತಿರುಗಿ ಖಾಲಿ;
        }

        g_guard_ಸ್ಥಾನ_ತೆಗೆದುಕೊಂಡ[i] = true;
        ಹಿಂತಿರುಗಿ ಸ್ಥಾನ;
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* Guard_ಸ್ಥಾನ_ಶೀರ್ಷಿಕೆ (ಶೂನ್ಯ* i_ಸ್ಥಾನ)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* ದತ್ತಾಂಶ = (u8*)i_ಸ್ಥಾನ + g_guard_ಪುಟ_ಗಾತ್ರ;

    ಹಿಂತಿರುಗಿ (M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ*)(ದತ್ತಾಂಶ - ಗಾತ್ರ(M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ));
}

bool Guard_Commit_ಸ್ಥಾನ (ಶೂನ್ಯ* i_ಸ್ಥಾನ, size_t i_ದತ್ತಾಂಶ_ಬೈಟುಗಳು)
{
    size_t rounded = (i_ದತ್ತಾಂಶ_ಬೈಟುಗಳು + g_guard_ಪುಟ_ಗಾತ್ರ - 1) / g_guard_ಪುಟ_ಗಾತ್ರ * g_guard_ಪುಟ_ಗಾತ್ರ;

    ಹಿಂತಿರುಗಿ m3_HostCommit((u8*)i_ಸ್ಥಾನ + g_guard_ಪುಟ_ಗಾತ್ರ, rounded);
}

ಶೂನ್ಯ Guard_Give_ಸ್ಥಾನ (ಶೂನ್ಯ* i_ಸ್ಥಾನ)
{
    ಆದರೆ (i_ಸ್ಥಾನ == ಖಾಲಿ) {
        ಹಿಂತಿರುಗಿ;
    }

    size_t index = (size_t)((u8*)i_ಸ್ಥಾನ - g_guardArena) / g_guard_ಸ್ಥಾನ_ಬೈಟುಗಳು;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    ಆದರೆ (m3_HostDecommit(i_ಸ್ಥಾನ, g_guard_ಸ್ಥಾನ_ಬೈಟುಗಳು) and index < g_guard_ಸ್ಥಾನ_ಎಣಿಕೆ) {
        g_guard_ಸ್ಥಾನ_ತೆಗೆದುಕೊಂಡ[index] = false;
    }
}

ಶೂನ್ಯ Guard_ArenaRange (ಶೂನ್ಯ** o_ಕೆಳಗಿನ, size_t* o_ಬೈಟುಗಳು)
{
    *o_ಕೆಳಗಿನ   = g_guardArena;
    *o_ಬೈಟುಗಳು = g_guard_Arena_ಬೈಟುಗಳು;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#ಆದರೆ d_m3_Max_Native_ರಾಶಿ > 0

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
ಶೂನ್ಯ* m3_Native_ರಾಶಿ_ಎಲ್ಲೆ (ಶೂನ್ಯ* i_ರಾಶಿ_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ರಾಶಿ_Ptr;

#  ಆದರೆ M3_HAS_THREAD_LOCAL
    ಸ್ಥಿರ M3_THREAD_LOCAL u8* ತಳ = ಖಾಲಿ;
    ಆದರೆ (ತಳ == ಖಾಲಿ) {
        ತಳ = (u8*)m3_Host_ರಾಶಿ_ತಳ();
    }
#  ಇಲ್ಲದಿದ್ದರೆ
    u8* ತಳ = (u8*)m3_Host_ರಾಶಿ_ತಳ();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    ಆದರೆ (ತಳ and ತಳ < sp) {
        size_t ಲಭ್ಯ = (size_t)(sp - ತಳ);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ರಾಶಿ_Margin), ಲಭ್ಯ / 4);

        i_budget = M3_MIN(i_budget, ಲಭ್ಯ - margin);
    }

    ಹಿಂತಿರುಗಿ sp - i_budget;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#ಆದರೆ d_m3_Log_Native_ರಾಶಿ

ಸ್ಥಿರ size_t ರಾಶಿ_ಆರಂಭ;
ಸ್ಥಿರ size_t ರಾಶಿ_ಕೊನೆ;

ಶೂನ್ಯ m3_ರಾಶಿ_Check_Init ()
{
    ಅಕ್ಷರ ರಾಶಿ;
    ರಾಶಿ_ಕೊನೆ = ರಾಶಿ_ಆರಂಭ = (size_t)&ರಾಶಿ;
}

ಶೂನ್ಯ m3_ರಾಶಿ_Check ()
{
    ಅಕ್ಷರ   ರಾಶಿ;
    size_t addr = (size_t)&ರಾಶಿ;

    size_t ರಾಶಿ_ಕೊನೆ_2 = ರಾಶಿ_ಕೊನೆ;

    ರಾಶಿ_ಕೊನೆ = M3_MIN(ರಾಶಿ_ಕೊನೆ, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

ಪೂರ್ಣಾಂಕ m3_ರಾಶಿ_Get_Max ()
{
    ಹಿಂತಿರುಗಿ ರಾಶಿ_ಆರಂಭ - ರಾಶಿ_ಕೊನೆ;
}

#ಆದರೆ_ಅಂತ್ಯ

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_ಫಲಿತಾಂಶ NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u8 type = -i_convolutedWasmType;

    ಆದರೆ (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#ಆದರೆ d_m3HasExceptionHandling
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (type == d_wa_Type_ರಾಶಿ_cont or type == d_wa_Type_ರಾಶಿ_nocont) {
        type = c_m3Type_contref;
    }
#ಆದರೆ_ಅಂತ್ಯ
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (type < c_m3Type_i32 or type > c_m3Type_v128) {
        ಫಲಿತಾಂಶ = m3Err_invalidTypeId;
    }

    *o_type = type;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


#ಆದರೆ d_m3HasTypedRefs

u8 ತಳ_Type_ದ (m3type_t i_type)
{
    ಆದರೆ (IsSpelledRefType(i_type)) {
        ಆದರೆ (i_type & d_m3Type_refCont) {
            ಹಿಂತಿರುಗಿ c_m3Type_contref;
        }
        ಹಿಂತಿರುಗಿ (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    ಹಿಂತಿರುಗಿ (u8)i_type;
}


bool Is_Sub_Type_ದ (m3type_t i_sub, m3type_t i_super)
{
    ಆದರೆ (i_sub == i_super) {
        ಹಿಂತಿರುಗಿ true;
    }

    u8 ತಳ = ತಳ_Type_ದ(i_sub);

    // only references have a subtype relation; every other type is invariant
    ಆದರೆ (ತಳ != ತಳ_Type_ದ(i_super) or not IsRefType(ತಳ)) {
        ಹಿಂತಿರುಗಿ false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    ಆದರೆ (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        ಹಿಂತಿರುಗಿ false;
    }

#  ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    // nocont sits under every continuation type
    ಆದರೆ (ರಾಶಿ_Type_ದ(i_sub) == d_m3_Type_ರಾಶಿ_None) {
        ಹಿಂತಿರುಗಿ true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    ಹಿಂತಿರುಗಿ (ರಾಶಿ_Type_ದ(i_super) == d_m3_Type_ರಾಶಿ_Abstract) or
           (ರಾಶಿ_Type_ದ(i_sub) == ರಾಶಿ_Type_ದ(i_super));
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = ತಳ_Type_ದ(i_type);
    ಹಿಂತಿರುಗಿ (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = ತಳ_Type_ದ(i_type);
    ಹಿಂತಿರುಗಿ (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#ಆದರೆ d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
            or i_m3Type == c_m3Type_contref
#ಆದರೆ_ಅಂತ್ಯ
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = ತಳ_Type_ದ(i_type);
    ಹಿಂತಿರುಗಿ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_ಬಿಟ್_Type (m3type_t i_type)
{
    u8 i_m3Type = ತಳ_Type_ದ(i_type);

    ಆದರೆ (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        ಹಿಂತಿರುಗಿ true;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        ಹಿಂತಿರುಗಿ false;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ (ಗಾತ್ರ(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 ಗಾತ್ರ_ದ_Type (m3type_t i_type)
{
    u8 i_m3Type = ತಳ_Type_ದ(i_type);

    ಆದರೆ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        ಹಿಂತಿರುಗಿ ಗಾತ್ರ(i32);
    }

    ಆದರೆ (IsRefType(i_m3Type)) {
        ಹಿಂತಿರುಗಿ ಗಾತ್ರ(ಶೂನ್ಯ*);
    }

    ಹಿಂತಿರುಗಿ ಗಾತ್ರ(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_ಫಲಿತಾಂಶ Read_u64 (u64* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    ಸ್ಥಿರಾಂಕ u8* ptr = *io_ಬೈಟುಗಳು;
    ptr += ಗಾತ್ರ(u64);

    ಆದರೆ (M3_LIKELY(ptr <= i_ಕೊನೆ)) {
        ಸ್ಮರಣೆ_ನಕಲು(o_ಮೌಲ್ಯ, *io_ಬೈಟುಗಳು, ಗಾತ್ರ(u64));
        M3_BSWAP_u64(*o_ಮೌಲ್ಯ);
        *io_ಬೈಟುಗಳು = ptr;
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
}


M3_ಫಲಿತಾಂಶ Read_u32 (u32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    ಸ್ಥಿರಾಂಕ u8* ptr = *io_ಬೈಟುಗಳು;
    ptr += ಗಾತ್ರ(u32);

    ಆದರೆ (M3_LIKELY(ptr <= i_ಕೊನೆ)) {
        ಸ್ಮರಣೆ_ನಕಲು(o_ಮೌಲ್ಯ, *io_ಬೈಟುಗಳು, ಗಾತ್ರ(u32));
        M3_BSWAP_u32(*o_ಮೌಲ್ಯ);
        *io_ಬೈಟುಗಳು = ptr;
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
}

#ಆದರೆ d_m3ImplementFloat

M3_ಫಲಿತಾಂಶ Read_f64 (f64* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    ಸ್ಥಿರಾಂಕ u8* ptr = *io_ಬೈಟುಗಳು;
    ptr += ಗಾತ್ರ(f64);

    ಆದರೆ (M3_LIKELY(ptr <= i_ಕೊನೆ)) {
        ಸ್ಮರಣೆ_ನಕಲು(o_ಮೌಲ್ಯ, *io_ಬೈಟುಗಳು, ಗಾತ್ರ(f64));
        M3_BSWAP_f64(*o_ಮೌಲ್ಯ);
        *io_ಬೈಟುಗಳು = ptr;
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
}


M3_ಫಲಿತಾಂಶ Read_f32 (f32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    ಸ್ಥಿರಾಂಕ u8* ptr = *io_ಬೈಟುಗಳು;
    ptr += ಗಾತ್ರ(f32);

    ಆದರೆ (M3_LIKELY(ptr <= i_ಕೊನೆ)) {
        ಸ್ಮರಣೆ_ನಕಲು(o_ಮೌಲ್ಯ, *io_ಬೈಟುಗಳು, ಗಾತ್ರ(f32));
        M3_BSWAP_f32(*o_ಮೌಲ್ಯ);
        *io_ಬೈಟುಗಳು = ptr;
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
}

#ಆದರೆ_ಅಂತ್ಯ

M3_ಫಲಿತಾಂಶ Read_u8 (u8* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    ಸ್ಥಿರಾಂಕ u8* ptr = *io_ಬೈಟುಗಳು;

    ಆದರೆ (M3_LIKELY(ptr < i_ಕೊನೆ)) {
        *o_ಮೌಲ್ಯ  = *ptr;
        *io_ಬೈಟುಗಳು = ptr + 1;

        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
}

M3_ಫಲಿತಾಂಶ Read_opcode (m3opcode_t* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    ಸ್ಥಿರಾಂಕ u8* ptr = *io_ಬೈಟುಗಳು;

    ಆದರೆ (M3_LIKELY(ptr < i_ಕೊನೆ)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_ಮೌಲ್ಯ  = opcode;
        *io_ಬೈಟುಗಳು = ptr;

        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
}


M3_ಫಲಿತಾಂಶ ReadLebUnsigned (u64* o_ಮೌಲ್ಯ, u32 i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_wasmUnderrun;

    u64 ಮೌಲ್ಯ = 0;

    u32       shift = 0;
    ಸ್ಥಿರಾಂಕ u8* ptr   = *io_ಬೈಟುಗಳು;

    ಆಗ (ptr < i_ಕೊನೆ) {
        u64 byte = *(ptr++);

        ಮೌಲ್ಯ |= ((byte & 0x7f) << shift);
        shift += 7;

        ಆದರೆ (M3_LIKELY((byte & 0x80) == 0)) {
            ಫಲಿತಾಂಶ = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            ಆದರೆ (M3_UNLIKELY(shift > i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು)) {
                u32 ಅಂಕೆ_ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು = i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು + 7 - shift;

                ಆದರೆ (byte >> ಅಂಕೆ_ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು) {
                    ಫಲಿತಾಂಶ = m3_Err_leb_ಅತಿಪ್ರವಾಹ;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಆದರೆ (M3_UNLIKELY(shift >= i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು)) {
            ಫಲಿತಾಂಶ = m3_Err_leb_ಅತಿಪ್ರವಾಹ;
            ನಿಲ್ಲಿಸು;
        }
    }

    *o_ಮೌಲ್ಯ  = ಮೌಲ್ಯ;
    *io_ಬೈಟುಗಳು = ptr;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ReadLebSigned (i64* o_ಮೌಲ್ಯ, u32 i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_wasmUnderrun;

    i64 ಮೌಲ್ಯ = 0;

    u32       shift = 0;
    ಸ್ಥಿರಾಂಕ u8* ptr   = *io_ಬೈಟುಗಳು;

    ಆಗ (ptr < i_ಕೊನೆ) {
        u64 byte = *(ptr++);

        ಮೌಲ್ಯ |= ((byte & 0x7f) << shift);
        shift += 7;

        ಆದರೆ (M3_LIKELY((byte & 0x80) == 0)) {
            ಫಲಿತಾಂಶ = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            ಆದರೆ (M3_UNLIKELY(shift > i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು)) {
                u32 ಅಂಕೆ_ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು = i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು + 7 - shift;
                u8  sign_ಬಿಟ್ಟುಗಳು    = (u8)((0x7f << (ಅಂಕೆ_ಬಳಸಿದ_ಬಿಟ್ಟುಗಳು - 1)) & 0x7f);
                u8  ಬಿಟ್ಟುಗಳು        = (u8)(byte & sign_ಬಿಟ್ಟುಗಳು);

                ಆದರೆ (ಬಿಟ್ಟುಗಳು != 0 and ಬಿಟ್ಟುಗಳು != sign_ಬಿಟ್ಟುಗಳು) {
                    ಫಲಿತಾಂಶ = m3_Err_leb_ಅತಿಪ್ರವಾಹ;
                }
            }
            ಆದರೆ ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                ಮೌಲ್ಯ |= (~extend << shift);
            }

            ನಿಲ್ಲಿಸು;
        }

        ಆದರೆ (M3_UNLIKELY(shift >= i_max_ಅಂಕೆ_ಬಿಟ್ಟುಗಳು)) {
            ಫಲಿತಾಂಶ = m3_Err_leb_ಅತಿಪ್ರವಾಹ;
            ನಿಲ್ಲಿಸು;
        }
    }

    *o_ಮೌಲ್ಯ  = ಮೌಲ್ಯ;
    *io_ಬೈಟುಗಳು = ptr;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ReadLEB_u32 (u32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    u64      ಮೌಲ್ಯ;
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ReadLebUnsigned(&ಮೌಲ್ಯ, 32, io_ಬೈಟುಗಳು, i_ಕೊನೆ);

    *o_ಮೌಲ್ಯ = (u32)ಮೌಲ್ಯ;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#ವ್ಯಾಖ್ಯಾನಿಸು d_mem_Arg_Has_ಸ್ಮರಣೆ_Idx    0x40u

M3_ಫಲಿತಾಂಶ Read_ಸ್ಮರಣೆ_Arg (u32* o_align, u32* o_ಸ್ಮರಣೆ_Idx, u64* o_ಸ್ಥಾನಾಂತರ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    u32 align;
    ಫಲಿತಾಂಶ = ReadLEB_u32(&align, io_ಬೈಟುಗಳು, i_ಕೊನೆ);
    ಆದರೆ (ಫಲಿತಾಂಶ) {
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    *o_ಸ್ಮರಣೆ_Idx = 0;

#ಆದರೆ d_m3_Has_Multi_ಸ್ಮರಣೆ
    ಆದರೆ (align & d_mem_Arg_Has_ಸ್ಮರಣೆ_Idx) {
        align &= ~d_mem_Arg_Has_ಸ್ಮರಣೆ_Idx;

        ಫಲಿತಾಂಶ = ReadLEB_u32(o_ಸ್ಮರಣೆ_Idx, io_ಬೈಟುಗಳು, i_ಕೊನೆ);
        ಆದರೆ (ಫಲಿತಾಂಶ) {
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
    }
#ಆದರೆ_ಅಂತ್ಯ

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    ಹಿಂತಿರುಗಿ ReadLebUnsigned(o_ಸ್ಥಾನಾಂತರ, 64, io_ಬೈಟುಗಳು, i_ಕೊನೆ);
}


M3_ಫಲಿತಾಂಶ ReadLEB_u7 (u8* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    u64      ಮೌಲ್ಯ;
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ReadLebUnsigned(&ಮೌಲ್ಯ, 7, io_ಬೈಟುಗಳು, i_ಕೊನೆ);

    *o_ಮೌಲ್ಯ = (u8)ಮೌಲ್ಯ;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ReadLEB_i7 (i8* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    i64      ಮೌಲ್ಯ;
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ReadLebSigned(&ಮೌಲ್ಯ, 7, io_ಬೈಟುಗಳು, i_ಕೊನೆ);

    *o_ಮೌಲ್ಯ = (i8)ಮೌಲ್ಯ;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ReadLEB_i32 (i32* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    i64      ಮೌಲ್ಯ;
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ReadLebSigned(&ಮೌಲ್ಯ, 32, io_ಬೈಟುಗಳು, i_ಕೊನೆ);

    *o_ಮೌಲ್ಯ = (i32)ಮೌಲ್ಯ;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ReadLEB_i64 (i64* o_ಮೌಲ್ಯ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    i64      ಮೌಲ್ಯ;
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ReadLebSigned(&ಮೌಲ್ಯ, 64, io_ಬೈಟುಗಳು, i_ಕೊನೆ);

    *o_ಮೌಲ್ಯ = ಮೌಲ್ಯ;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ಸ್ಥಿರ
bool IsValidUtf8 (ಸ್ಥಿರಾಂಕ u8* i_ದತ್ತಾಂಶ, u32 i_ಉದ್ದಳತೆ)
{
    ಸ್ಥಿರಾಂಕ u8* ptr = i_ದತ್ತಾಂಶ;
    ಸ್ಥಿರಾಂಕ u8* ಕೊನೆ = i_ದತ್ತಾಂಶ + i_ಉದ್ದಳತೆ;

    ಆಗ (ptr < ಕೊನೆ) {
        u8 b0 = *ptr++;

        ಆದರೆ (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            ಮುಂದುವರಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            ಆದರೆ (b0 < 0xC2) {
                ಹಿಂತಿರುಗಿ false;       // overlong
            }
            ಆದರೆ (ptr >= ಕೊನೆ) {
                ಹಿಂತಿರುಗಿ false;
            }
            u8 b1 = *ptr++;
            ಆದರೆ ((b1 & 0xC0) != 0x80) {
                ಹಿಂತಿರುಗಿ false;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            ಆದರೆ (ptr + 1 >= ಕೊನೆ) {
                ಹಿಂತಿರುಗಿ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            ಆದರೆ ((b1 & 0xC0) != 0x80) {
                ಹಿಂತಿರುಗಿ false;
            }
            ಆದರೆ ((b2 & 0xC0) != 0x80) {
                ಹಿಂತಿರುಗಿ false;
            }
            // reject overlong
            ಆದರೆ (b0 == 0xE0 && b1 < 0xA0) {
                ಹಿಂತಿರುಗಿ false;
            }
            // reject surrogates U+D800..U+DFFF
            ಆದರೆ (b0 == 0xED && b1 >= 0xA0) {
                ಹಿಂತಿರುಗಿ false;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            ಆದರೆ (b0 > 0xF4) {
                ಹಿಂತಿರುಗಿ false;       // above U+10FFFF
            }
            ಆದರೆ (ptr + 2 >= ಕೊನೆ) {
                ಹಿಂತಿರುಗಿ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            ಆದರೆ ((b1 & 0xC0) != 0x80) {
                ಹಿಂತಿರುಗಿ false;
            }
            ಆದರೆ ((b2 & 0xC0) != 0x80) {
                ಹಿಂತಿರುಗಿ false;
            }
            ಆದರೆ ((b3 & 0xC0) != 0x80) {
                ಹಿಂತಿರುಗಿ false;
            }
            // reject overlong
            ಆದರೆ (b0 == 0xF0 && b1 < 0x90) {
                ಹಿಂತಿರುಗಿ false;
            }
            // reject above U+10FFFF
            ಆದರೆ (b0 == 0xF4 && b1 > 0x8F) {
                ಹಿಂತಿರುಗಿ false;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            ಹಿಂತಿರುಗಿ false;
        }
    }

    ಹಿಂತಿರುಗಿ true;
}
#ಆದರೆ_ಅಂತ್ಯ // d_m3EnableValidation


M3_ಫಲಿತಾಂಶ Read_utf8 (cstr_t* o_utf8, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    *o_utf8 = ಖಾಲಿ;

    u32      utf8_ಉದ್ದಳತೆ;
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = ReadLEB_u32(&utf8_ಉದ್ದಳತೆ, io_ಬೈಟುಗಳು, i_ಕೊನೆ);

    ಆದರೆ (not ಫಲಿತಾಂಶ) {
        ಆದರೆ (utf8_ಉದ್ದಳತೆ <= d_m3_Max_Sane_Utf8_ಉದ್ದಳತೆ) {
            ಸ್ಥಿರಾಂಕ u8* ptr = *io_ಬೈಟುಗಳು;
            ಆದರೆ (utf8_ಉದ್ದಳತೆ <= (size_t)(i_ಕೊನೆ - ptr)) {
                ಸ್ಥಿರಾಂಕ u8* ಕೊನೆ = ptr + utf8_ಉದ್ದಳತೆ;
#ಆದರೆ d_m3EnableValidation
                ಆದರೆ (not IsValidUtf8(ptr, utf8_ಉದ್ದಳತೆ)) {
                    *io_ಬೈಟುಗಳು = ಕೊನೆ;
                    ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
                }
#ಆದರೆ_ಅಂತ್ಯ // d_m3EnableValidation

                ಅಕ್ಷರ* utf8 = (ಅಕ್ಷರ*)m3_Malloc("UTF8", utf8_ಉದ್ದಳತೆ + 1);

                ಆದರೆ (utf8) {
                    ಸ್ಮರಣೆ_ನಕಲು(utf8, ptr, utf8_ಉದ್ದಳತೆ);
                    utf8[utf8_ಉದ್ದಳತೆ] = 0;

                    *o_utf8 = utf8;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಫಲಿತಾಂಶ = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_ಬೈಟುಗಳು = ಕೊನೆ;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಫಲಿತಾಂಶ = m3Err_wasmUnderrun;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಫಲಿತಾಂಶ = m3Err_missingUTF8;
        }
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
u32 ಕಂಡುಹಿಡಿ_Module_ಸ್ಥಾನಾಂತರ (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_ಸಂಕೇತ_ಪುಟ curr = i_runtime->ಪುಟಗಳು_Open;

    bool ಪುಟ_ಸಿಕ್ಕಿತು = false;

    ಆಗ (curr) {
        ಆದರೆ (ಒಳಗೊಂಡಿದೆ_PC(curr, i_pc)) {
            ಪುಟ_ಸಿಕ್ಕಿತು = true;
            ನಿಲ್ಲಿಸು;
        }
        curr = curr->info.ಮುಂದಿನ;
    }

    ಆದರೆ (!ಪುಟ_ಸಿಕ್ಕಿತು) {
        curr = i_runtime->ಪುಟಗಳು_ತುಂಬಿದ;
        ಆಗ (curr) {
            ಆದರೆ (ಒಳಗೊಂಡಿದೆ_PC(curr, i_pc)) {
                ಪುಟ_ಸಿಕ್ಕಿತು = true;
                ನಿಲ್ಲಿಸು;
            }
            curr = curr->info.ಮುಂದಿನ;
        }
    }

    ಆದರೆ (ಪುಟ_ಸಿಕ್ಕಿತು) {
        u32 ಫಲಿತಾಂಶ = 0;

        bool pc_ಸಿಕ್ಕಿತು = Map_PCTo_ಸ್ಥಾನಾಂತರ(curr, i_pc, &ಫಲಿತಾಂಶ);
        (ಶೂನ್ಯ)pc_ಸಿಕ್ಕಿತು;                                                          d_m3Assert (pc_ಸಿಕ್ಕಿತು);

        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ 0;
    }
}


ಶೂನ್ಯ ತಳ್ಳು_Backtrace_ಚೌಕಟ್ಟು (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    ಆದರೆ (M3_UNLIKELY(io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು == M3_BACKTRACE_TRUNCATED)) {
        ಹಿಂತಿರುಗಿ;
    }

    M3_Backtrace_ಚೌಕಟ್ಟು* new_ಚೌಕಟ್ಟು = m3_AllocStruct(M3_Backtrace_ಚೌಕಟ್ಟು);

    ಆದರೆ (!new_ಚೌಕಟ್ಟು) {
        io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು = M3_BACKTRACE_TRUNCATED;
        ಹಿಂತಿರುಗಿ;
    }

    new_ಚೌಕಟ್ಟು->module_ಸ್ಥಾನಾಂತರ = ಕಂಡುಹಿಡಿ_Module_ಸ್ಥಾನಾಂತರ(io_runtime, i_pc);

    ಆದರೆ (!io_runtime->backtrace.ಚೌಕಟ್ಟುಗಳು || !io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು) {
        io_runtime->backtrace.ಚೌಕಟ್ಟುಗಳು = new_ಚೌಕಟ್ಟು;
    } ಇಲ್ಲದಿದ್ದರೆ {
        io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು->ಮುಂದಿನ = new_ಚೌಕಟ್ಟು;
    }
    io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು = new_ಚೌಕಟ್ಟು;
}


ಶೂನ್ಯ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    ಆದರೆ (M3_UNLIKELY(io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು == M3_BACKTRACE_TRUNCATED)) {
        ಹಿಂತಿರುಗಿ;
    }

    ಆದರೆ (!io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು) {
        ಹಿಂತಿರುಗಿ;
    }

    io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು->function = i_function;
}


ಶೂನ್ಯ ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_ಚೌಕಟ್ಟು* current_ಚೌಕಟ್ಟು = io_runtime->backtrace.ಚೌಕಟ್ಟುಗಳು;
    ಆಗ (current_ಚೌಕಟ್ಟು) {
        M3_Backtrace_ಚೌಕಟ್ಟು* ಮುಂದಿನ_ಚೌಕಟ್ಟು = current_ಚೌಕಟ್ಟು->ಮುಂದಿನ;
        m3_Free(current_ಚೌಕಟ್ಟು);
        current_ಚೌಕಟ್ಟು = ಮುಂದಿನ_ಚೌಕಟ್ಟು;
    }

    io_runtime->backtrace.ಚೌಕಟ್ಟುಗಳು    = ಖಾಲಿ;
    io_runtime->backtrace.ಕೊನೆಯ_ಚೌಕಟ್ಟು = ಖಾಲಿ;
}
#ಆದರೆ_ಅಂತ್ಯ // d_m3RecordBacktraces
