//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif

#define M3_IMPLEMENT_ERROR_STRINGS
#ಅನ್ತರ್ಭಾವ "m3_config.h"
#ಅನ್ತರ್ಭಾವ "wasm3.h"

#ಅನ್ತರ್ಭಾವ "m3_core.h"
#ಅನ್ತರ್ಭಾವ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#ಅನ್ತರ್ಭಾವ "m3_host.h"

#ಯದಿ d_m3HasWin32Host
#  include "m3_host_win32.h"
#ನೋಚೇದ್ಯದಿ d_m3HasPosixHost
#  include "m3_host_posix.h"
#ಅನ್ಯಥಾ
#  include "m3_host_none.h"
#endif

ಶೂನ್ಯ m3_Abort (ಧ್ರುವ ವರ್ಣ* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

#ಯದಿ d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

ಸ್ಥಿರ uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    ಯದಿ (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    ಸಂರಚನಾ timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    ಪ್ರತ್ಯಾಗಮ us - initial_ts;
}

#endif

#ಯದಿ d_m3FixedHeap

ಸ್ಥಿರ u8        fixedHeap[d_m3FixedHeap];
ಸ್ಥಿರ u8*       fixedHeapPtr  = fixedHeap;
ಸ್ಥಿರ u8* ಧ್ರುವ fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
ಸ್ಥಿರ u8*       fixedHeapLast = ರಿಕ್ತ;

#  ಯದಿ d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  ಅನ್ಯಥಾ
#    define HEAP_ALIGN_PTR(P)
#  endif

ಶೂನ್ಯ* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    ಯದಿ (fixedHeapPtr >= fixedHeapEnd) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    ಪ್ರತ್ಯಾಗಮ ptr;
}

ಶೂನ್ಯ m3_Free_Impl (ಶೂನ್ಯ* i_ptr)
{
    // Handle the last chunk
    ಯದಿ (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = ರಿಕ್ತ;
    } ಅನ್ಯಥಾ {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

ಶೂನ್ಯ* m3_Realloc_Impl (ಶೂನ್ಯ* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    ಯದಿ (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ಪ್ರತ್ಯಾಗಮ i_ptr;
    }

    ಶೂನ್ಯ* newPtr;

    // Handle the last chunk
    ಯದಿ (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        ಯದಿ (fixedHeapPtr >= fixedHeapEnd) {
            ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
        }
        newPtr = i_ptr;
    } ಅನ್ಯಥಾ {
        newPtr = m3_Malloc_Impl(i_newSize);
        ಯದಿ (!newPtr) {
            ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
        }
        ಯದಿ (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    ಯದಿ (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    ಪ್ರತ್ಯಾಗಮ newPtr;
}

#ಅನ್ಯಥಾ

ಶೂನ್ಯ* m3_Malloc_Impl (size_t i_size)
{
    ಪ್ರತ್ಯಾಗಮ calloc(i_size, 1);
}

ಶೂನ್ಯ m3_Free_Impl (ಶೂನ್ಯ* io_ptr)
{
    ಮುಕ್ತ(io_ptr);
}

ಶೂನ್ಯ* m3_Realloc_Impl (ಶೂನ್ಯ* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    ಯದಿ (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ಪ್ರತ್ಯಾಗಮ i_ptr;
    }

    ಶೂನ್ಯ* newPtr = realloc(i_ptr, i_newSize);

    ಯದಿ (M3_LIKELY(newPtr)) {
        ಯದಿ (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        ಪ್ರತ್ಯಾಗಮ newPtr;
    }
    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

#endif

ಶೂನ್ಯ* m3_CopyMem (ಧ್ರುವ ಶೂನ್ಯ* i_from, size_t i_size)
{
    ಶೂನ್ಯ* ptr = m3_Malloc("CopyMem", i_size);
    ಯದಿ (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    ಪ್ರತ್ಯಾಗಮ ptr;
}

//--------------------------------------------------------------------------------------------

#ಯದಿ d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ಸ್ಥಿರ u8*    g_guardArena;
ಸ್ಥಿರ size_t g_guardArenaBytes;
ಸ್ಥಿರ size_t g_guardSlotBytes;
ಸ್ಥಿರ size_t g_guardPageSize;
ಸ್ಥಿರ u32    g_guard_Slot_ಎಣಿಕೆ;
ಸ್ಥಿರ bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
ಸ್ಥಿರ
bool Guard_Reserve (ಶೂನ್ಯ)
{
    ಯದಿ (g_guardArena) {
        ಪ್ರತ್ಯಾಗಮ true;
    }

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    ಪ್ರತಿ (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        ಶೂನ್ಯ* base = m3_HostReserve(slotBytes * slots);

        ಯದಿ (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_ಎಣಿಕೆ  = slots;
            ಪ್ರತ್ಯಾಗಮ true;
        }
    }

    ಪ್ರತ್ಯಾಗಮ false;
}

ಶೂನ್ಯ* Guard_TakeSlot (ಶೂನ್ಯ)
{
    ಯದಿ (not Guard_Reserve()) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪ್ರತಿ (u32 i = 0; i < g_guard_Slot_ಎಣಿಕೆ; ++i) {
        ಯದಿ (g_guardSlotTaken[i]) {
            ಅನುವರ್ತ;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        ಯದಿ (not m3_HostCommit(slot, g_guardPageSize)) {
            ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
        }

        g_guardSlotTaken[i] = true;
        ಪ್ರತ್ಯಾಗಮ slot;
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

M3MemoryHeader* Guard_SlotHeader (ಶೂನ್ಯ* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    ಪ್ರತ್ಯಾಗಮ (M3MemoryHeader*)(data - ಪರಿಮಾಣ(M3MemoryHeader));
}

bool Guard_CommitSlot (ಶೂನ್ಯ* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    ಪ್ರತ್ಯಾಗಮ m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

ಶೂನ್ಯ Guard_GiveSlot (ಶೂನ್ಯ* i_slot)
{
    ಯದಿ (i_slot == ರಿಕ್ತ) {
        ಪ್ರತ್ಯಾಗಮ;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    ಯದಿ (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_ಎಣಿಕೆ) {
        g_guardSlotTaken[index] = false;
    }
}

ಶೂನ್ಯ Guard_ArenaRange (ಶೂನ್ಯ** o_ಕೆಳಗಿನ, size_t* o_bytes)
{
    *o_ಕೆಳಗಿನ   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#ಯದಿ d_m3_Max_Native_ರಾಶಿ > 0

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
ಶೂನ್ಯ* m3_Native_ರಾಶಿ_Limit (ಶೂನ್ಯ* i_ರಾಶಿ_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ರಾಶಿ_Ptr;

#  ಯದಿ M3_HAS_THREAD_LOCAL
    ಸ್ಥಿರ M3_THREAD_LOCAL u8* base = ರಿಕ್ತ;
    ಯದಿ (base == ರಿಕ್ತ) {
        base = (u8*)m3_Host_ರಾಶಿ_Base();
    }
#  ಅನ್ಯಥಾ
    u8* base = (u8*)m3_Host_ರಾಶಿ_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    ಯದಿ (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ರಾಶಿ_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    ಪ್ರತ್ಯಾಗಮ sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#ಯದಿ d_m3_Log_Native_ರಾಶಿ

ಸ್ಥಿರ size_t ರಾಶಿ_start;
ಸ್ಥಿರ size_t ರಾಶಿ_end;

ಶೂನ್ಯ m3_ರಾಶಿ_Check_Init ()
{
    ವರ್ಣ ರಾಶಿ;
    ರಾಶಿ_end = ರಾಶಿ_start = (size_t)&ರಾಶಿ;
}

ಶೂನ್ಯ m3_ರಾಶಿ_Check ()
{
    ವರ್ಣ   ರಾಶಿ;
    size_t addr = (size_t)&ರಾಶಿ;

    size_t ರಾಶಿ_End = ರಾಶಿ_end;

    ರಾಶಿ_end = M3_MIN(ರಾಶಿ_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

ಪೂರ್ಣಾಙ್ಕ m3_ರಾಶಿ_Get_Max ()
{
    ಪ್ರತ್ಯಾಗಮ ರಾಶಿ_start - ರಾಶಿ_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    ಯದಿ (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    ಅನ್ಯಥಾ ಯದಿ (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } ಅನ್ಯಥಾ ಯದಿ (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#ಯದಿ d_m3HasExceptionHandling
    ಅನ್ಯಥಾ ಯದಿ (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    ಅನ್ಯಥಾ ಯದಿ (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    ಅನ್ಯಥಾ ಯದಿ (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    ಪ್ರತ್ಯಾಗಮ result;
}


#ಯದಿ d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    ಯದಿ (IsSpelledRefType(i_type)) {
        ಯದಿ (i_type & d_m3Type_refCont) {
            ಪ್ರತ್ಯಾಗಮ c_m3Type_contref;
        }
        ಪ್ರತ್ಯಾಗಮ (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    ಪ್ರತ್ಯಾಗಮ (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    ಯದಿ (i_sub == i_super) {
        ಪ್ರತ್ಯಾಗಮ true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    ಯದಿ (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    ಯದಿ (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

#  ಯದಿ d_m3_Has_ರಾಶಿ_Switching
    // nocont sits under every continuation type
    ಯದಿ (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        ಪ್ರತ್ಯಾಗಮ true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    ಪ್ರತ್ಯಾಗಮ (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ಪ್ರತ್ಯಾಗಮ (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ಪ್ರತ್ಯಾಗಮ (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#ಯದಿ d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ಪ್ರತ್ಯಾಗಮ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    ಯದಿ (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        ಪ್ರತ್ಯಾಗಮ true;
    } ಅನ್ಯಥಾ ಯದಿ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        ಪ್ರತ್ಯಾಗಮ false;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ (ಪರಿಮಾಣ(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    ಯದಿ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(i32);
    }

    ಯದಿ (IsRefType(i_m3Type)) {
        ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(ಶೂನ್ಯ*);
    }

    ಪ್ರತ್ಯಾಗಮ ಪರಿಮಾಣ(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    ಧ್ರುವ u8* ptr = *io_bytes;
    ptr += ಪರಿಮಾಣ(u64);

    ಯದಿ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ಮೌಲ್ಯ, *io_bytes, ಪರಿಮಾಣ(u64));
        M3_BSWAP_u64(*o_ಮೌಲ್ಯ);
        *io_bytes = ptr;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    ಧ್ರುವ u8* ptr = *io_bytes;
    ptr += ಪರಿಮಾಣ(u32);

    ಯದಿ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ಮೌಲ್ಯ, *io_bytes, ಪರಿಮಾಣ(u32));
        M3_BSWAP_u32(*o_ಮೌಲ್ಯ);
        *io_bytes = ptr;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
}

#ಯದಿ d_m3ImplementFloat

M3Result Read_f64 (f64* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    ಧ್ರುವ u8* ptr = *io_bytes;
    ptr += ಪರಿಮಾಣ(f64);

    ಯದಿ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ಮೌಲ್ಯ, *io_bytes, ಪರಿಮಾಣ(f64));
        M3_BSWAP_f64(*o_ಮೌಲ್ಯ);
        *io_bytes = ptr;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    ಧ್ರುವ u8* ptr = *io_bytes;
    ptr += ಪರಿಮಾಣ(f32);

    ಯದಿ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ಮೌಲ್ಯ, *io_bytes, ಪರಿಮಾಣ(f32));
        M3_BSWAP_f32(*o_ಮೌಲ್ಯ);
        *io_bytes = ptr;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    ಧ್ರುವ u8* ptr = *io_bytes;

    ಯದಿ (M3_LIKELY(ptr < i_end)) {
        *o_ಮೌಲ್ಯ  = *ptr;
        *io_bytes = ptr + 1;

        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    ಧ್ರುವ u8* ptr = *io_bytes;

    ಯದಿ (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_ಮೌಲ್ಯ  = opcode;
        *io_bytes = ptr;

        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_ಮೌಲ್ಯ, u32 i_max_ಅಂಕೆ_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 ಮೌಲ್ಯ = 0;

    u32       shift = 0;
    ಧ್ರುವ u8* ptr   = *io_bytes;

    ಯಾವತ್ (ptr < i_end) {
        u64 byte = *(ptr++);

        ಮೌಲ್ಯ |= ((byte & 0x7f) << shift);
        shift += 7;

        ಯದಿ (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            ಯದಿ (M3_UNLIKELY(shift > i_max_ಅಂಕೆ_Bits)) {
                u32 ಅಂಕೆ_Used_Bits = i_max_ಅಂಕೆ_Bits + 7 - shift;

                ಯದಿ (byte >> ಅಂಕೆ_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            ಭಙ್ಗ;
        }

        ಯದಿ (M3_UNLIKELY(shift >= i_max_ಅಂಕೆ_Bits)) {
            result = m3Err_lebOverflow;
            ಭಙ್ಗ;
        }
    }

    *o_ಮೌಲ್ಯ  = ಮೌಲ್ಯ;
    *io_bytes = ptr;

    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result ReadLebSigned (i64* o_ಮೌಲ್ಯ, u32 i_max_ಅಂಕೆ_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 ಮೌಲ್ಯ = 0;

    u32       shift = 0;
    ಧ್ರುವ u8* ptr   = *io_bytes;

    ಯಾವತ್ (ptr < i_end) {
        u64 byte = *(ptr++);

        ಮೌಲ್ಯ |= ((byte & 0x7f) << shift);
        shift += 7;

        ಯದಿ (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            ಯದಿ (M3_UNLIKELY(shift > i_max_ಅಂಕೆ_Bits)) {
                u32 ಅಂಕೆ_Used_Bits = i_max_ಅಂಕೆ_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (ಅಂಕೆ_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                ಯದಿ (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            ಯದಿ ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                ಮೌಲ್ಯ |= (~extend << shift);
            }

            ಭಙ್ಗ;
        }

        ಯದಿ (M3_UNLIKELY(shift >= i_max_ಅಂಕೆ_Bits)) {
            result = m3Err_lebOverflow;
            ಭಙ್ಗ;
        }
    }

    *o_ಮೌಲ್ಯ  = ಮೌಲ್ಯ;
    *io_bytes = ptr;

    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result ReadLEB_u32 (u32* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      ಮೌಲ್ಯ;
    M3Result result = ReadLebUnsigned(&ಮೌಲ್ಯ, 32, io_bytes, i_end);

    *o_ಮೌಲ್ಯ = (u32)ಮೌಲ್ಯ;

    ಪ್ರತ್ಯಾಗಮ result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    ಯದಿ (result) {
        ಪ್ರತ್ಯಾಗಮ result;
    }

    *o_memoryIdx = 0;

#ಯದಿ d_m3HasMultiMemory
    ಯದಿ (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        ಯದಿ (result) {
            ಪ್ರತ್ಯಾಗಮ result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    ಪ್ರತ್ಯಾಗಮ ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      ಮೌಲ್ಯ;
    M3Result result = ReadLebUnsigned(&ಮೌಲ್ಯ, 7, io_bytes, i_end);

    *o_ಮೌಲ್ಯ = (u8)ಮೌಲ್ಯ;

    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result ReadLEB_i7 (i8* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      ಮೌಲ್ಯ;
    M3Result result = ReadLebSigned(&ಮೌಲ್ಯ, 7, io_bytes, i_end);

    *o_ಮೌಲ್ಯ = (i8)ಮೌಲ್ಯ;

    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result ReadLEB_i32 (i32* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      ಮೌಲ್ಯ;
    M3Result result = ReadLebSigned(&ಮೌಲ್ಯ, 32, io_bytes, i_end);

    *o_ಮೌಲ್ಯ = (i32)ಮೌಲ್ಯ;

    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result ReadLEB_i64 (i64* o_ಮೌಲ್ಯ, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      ಮೌಲ್ಯ;
    M3Result result = ReadLebSigned(&ಮೌಲ್ಯ, 64, io_bytes, i_end);

    *o_ಮೌಲ್ಯ = ಮೌಲ್ಯ;

    ಪ್ರತ್ಯಾಗಮ result;
}

#ಯದಿ d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ಸ್ಥಿರ
bool IsValidUtf8 (ಧ್ರುವ u8* i_data, u32 i_length)
{
    ಧ್ರುವ u8* ptr = i_data;
    ಧ್ರುವ u8* end = i_data + i_length;

    ಯಾವತ್ (ptr < end) {
        u8 b0 = *ptr++;

        ಯದಿ (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            ಅನುವರ್ತ;
        } ಅನ್ಯಥಾ ಯದಿ ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            ಯದಿ (b0 < 0xC2) {
                ಪ್ರತ್ಯಾಗಮ false;       // overlong
            }
            ಯದಿ (ptr >= end) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            u8 b1 = *ptr++;
            ಯದಿ ((b1 & 0xC0) != 0x80) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
        } ಅನ್ಯಥಾ ಯದಿ ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            ಯದಿ (ptr + 1 >= end) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            ಯದಿ ((b1 & 0xC0) != 0x80) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            ಯದಿ ((b2 & 0xC0) != 0x80) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            // reject overlong
            ಯದಿ (b0 == 0xE0 && b1 < 0xA0) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            // reject surrogates U+D800..U+DFFF
            ಯದಿ (b0 == 0xED && b1 >= 0xA0) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
        } ಅನ್ಯಥಾ ಯದಿ ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            ಯದಿ (b0 > 0xF4) {
                ಪ್ರತ್ಯಾಗಮ false;       // above U+10FFFF
            }
            ಯದಿ (ptr + 2 >= end) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            ಯದಿ ((b1 & 0xC0) != 0x80) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            ಯದಿ ((b2 & 0xC0) != 0x80) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            ಯದಿ ((b3 & 0xC0) != 0x80) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            // reject overlong
            ಯದಿ (b0 == 0xF0 && b1 < 0x90) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
            // reject above U+10FFFF
            ಯದಿ (b0 == 0xF4 && b1 > 0x8F) {
                ಪ್ರತ್ಯಾಗಮ false;
            }
        } ಅನ್ಯಥಾ {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            ಪ್ರತ್ಯಾಗಮ false;
        }
    }

    ಪ್ರತ್ಯಾಗಮ true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = ರಿಕ್ತ;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    ಯದಿ (not result) {
        ಯದಿ (utf8Length <= d_m3MaxSaneUtf8Length) {
            ಧ್ರುವ u8* ptr = *io_bytes;
            ಯದಿ (utf8Length <= (size_t)(i_end - ptr)) {
                ಧ್ರುವ u8* end = ptr + utf8Length;
#ಯದಿ d_m3EnableValidation
                ಯದಿ (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                ವರ್ಣ* utf8 = (ವರ್ಣ*)m3_Malloc("UTF8", utf8Length + 1);

                ಯದಿ (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } ಅನ್ಯಥಾ {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } ಅನ್ಯಥಾ {
                result = m3Err_wasmUnderrun;
            }
        } ಅನ್ಯಥಾ {
            result = m3Err_missingUTF8;
        }
    }

    ಪ್ರತ್ಯಾಗಮ result;
}

#ಯದಿ d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    ಯಾವತ್ (curr) {
        ಯದಿ (ContainsPC(curr, i_pc)) {
            pageFound = true;
            ಭಙ್ಗ;
        }
        curr = curr->info.next;
    }

    ಯದಿ (!pageFound) {
        curr = i_runtime->pagesFull;
        ಯಾವತ್ (curr) {
            ಯದಿ (ContainsPC(curr, i_pc)) {
                pageFound = true;
                ಭಙ್ಗ;
            }
            curr = curr->info.next;
        }
    }

    ಯದಿ (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (ಶೂನ್ಯ)pcFound;                                                          d_m3Assert (pcFound);

        ಪ್ರತ್ಯಾಗಮ result;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ 0;
    }
}


ಶೂನ್ಯ ತಳ್ಳು_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    ಯದಿ (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ಪ್ರತ್ಯಾಗಮ;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    ಯದಿ (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        ಪ್ರತ್ಯಾಗಮ;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    ಯದಿ (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } ಅನ್ಯಥಾ {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


ಶೂನ್ಯ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    ಯದಿ (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (!io_runtime->backtrace.lastFrame) {
        ಪ್ರತ್ಯಾಗಮ;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


ಶೂನ್ಯ ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    ಯಾವತ್ (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = ರಿಕ್ತ;
    io_runtime->backtrace.lastFrame = ರಿಕ್ತ;
}
#endif // d_m3RecordBacktraces
