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
#ଅନ୍ତର୍ଭାଵ "m3_config.h"
#ଅନ୍ତର୍ଭାଵ "wasm3.h"

#ଅନ୍ତର୍ଭାଵ "m3_core.h"
#ଅନ୍ତର୍ଭାଵ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#ଅନ୍ତର୍ଭାଵ "m3_host.h"

#ଯଦି d_m3HasWin32Host
#  include "m3_host_win32.h"
#ନଚେତଯଦି d_m3HasPosixHost
#  include "m3_host_posix.h"
#ଅନ୍ୟଥା
#  include "m3_host_none.h"
#endif

ଶୂନ୍ୟ m3_Abort (ଅଚଳ ଅକ୍ଷର* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    ଫେରନ୍ତୁ m3Err_none;
}

#ଯଦି d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

ସ୍ଥିର uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    ଯଦି (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    ସଂରଚନା timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    ଫେରନ୍ତୁ us - initial_ts;
}

#endif

#ଯଦି d_m3FixedHeap

ସ୍ଥିର u8        fixedHeap[d_m3FixedHeap];
ସ୍ଥିର u8*       fixedHeapPtr  = fixedHeap;
ସ୍ଥିର u8* ଅଚଳ fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
ସ୍ଥିର u8*       fixedHeapLast = ଖାଲି;

#  ଯଦି d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  ଅନ୍ୟଥା
#    define HEAP_ALIGN_PTR(P)
#  endif

ଶୂନ୍ୟ* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    ଯଦି (fixedHeapPtr >= fixedHeapEnd) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    ଫେରନ୍ତୁ ptr;
}

ଶୂନ୍ୟ m3_Free_Impl (ଶୂନ୍ୟ* i_ptr)
{
    // Handle the last chunk
    ଯଦି (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = ଖାଲି;
    } ଅନ୍ୟଥା {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

ଶୂନ୍ୟ* m3_Realloc_Impl (ଶୂନ୍ୟ* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    ଯଦି (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ଫେରନ୍ତୁ i_ptr;
    }

    ଶୂନ୍ୟ* newPtr;

    // Handle the last chunk
    ଯଦି (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        ଯଦି (fixedHeapPtr >= fixedHeapEnd) {
            ଫେରନ୍ତୁ ଖାଲି;
        }
        newPtr = i_ptr;
    } ଅନ୍ୟଥା {
        newPtr = m3_Malloc_Impl(i_newSize);
        ଯଦି (!newPtr) {
            ଫେରନ୍ତୁ ଖାଲି;
        }
        ଯଦି (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    ଯଦି (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    ଫେରନ୍ତୁ newPtr;
}

#ଅନ୍ୟଥା

ଶୂନ୍ୟ* m3_Malloc_Impl (size_t i_size)
{
    ଫେରନ୍ତୁ calloc(i_size, 1);
}

ଶୂନ୍ୟ m3_Free_Impl (ଶୂନ୍ୟ* io_ptr)
{
    ମୁକ୍ତ(io_ptr);
}

ଶୂନ୍ୟ* m3_Realloc_Impl (ଶୂନ୍ୟ* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    ଯଦି (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ଫେରନ୍ତୁ i_ptr;
    }

    ଶୂନ୍ୟ* newPtr = realloc(i_ptr, i_newSize);

    ଯଦି (M3_LIKELY(newPtr)) {
        ଯଦି (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        ଫେରନ୍ତୁ newPtr;
    }
    ଫେରନ୍ତୁ ଖାଲି;
}

#endif

ଶୂନ୍ୟ* m3_CopyMem (ଅଚଳ ଶୂନ୍ୟ* i_from, size_t i_size)
{
    ଶୂନ୍ୟ* ptr = m3_Malloc("CopyMem", i_size);
    ଯଦି (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    ଫେରନ୍ତୁ ptr;
}

//--------------------------------------------------------------------------------------------

#ଯଦି d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ସ୍ଥିର u8*    g_guardArena;
ସ୍ଥିର size_t g_guardArenaBytes;
ସ୍ଥିର size_t g_guardSlotBytes;
ସ୍ଥିର size_t g_guardPageSize;
ସ୍ଥିର u32    g_guard_Slot_ଗଣନା;
ସ୍ଥିର bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
ସ୍ଥିର
bool Guard_Reserve (ଶୂନ୍ୟ)
{
    ଯଦି (g_guardArena) {
        ଫେରନ୍ତୁ true;
    }

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    ପାଇଁ (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        ଶୂନ୍ୟ* base = m3_HostReserve(slotBytes * slots);

        ଯଦି (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_ଗଣନା  = slots;
            ଫେରନ୍ତୁ true;
        }
    }

    ଫେରନ୍ତୁ false;
}

ଶୂନ୍ୟ* Guard_TakeSlot (ଶୂନ୍ୟ)
{
    ଯଦି (not Guard_Reserve()) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ପାଇଁ (u32 i = 0; i < g_guard_Slot_ଗଣନା; ++i) {
        ଯଦି (g_guardSlotTaken[i]) {
            ଜାରି;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        ଯଦି (not m3_HostCommit(slot, g_guardPageSize)) {
            ଫେରନ୍ତୁ ଖାଲି;
        }

        g_guardSlotTaken[i] = true;
        ଫେରନ୍ତୁ slot;
    }

    ଫେରନ୍ତୁ ଖାଲି;
}

M3MemoryHeader* Guard_SlotHeader (ଶୂନ୍ୟ* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    ଫେରନ୍ତୁ (M3MemoryHeader*)(data - ଆକାର(M3MemoryHeader));
}

bool Guard_CommitSlot (ଶୂନ୍ୟ* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    ଫେରନ୍ତୁ m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

ଶୂନ୍ୟ Guard_GiveSlot (ଶୂନ୍ୟ* i_slot)
{
    ଯଦି (i_slot == ଖାଲି) {
        ଫେରନ୍ତୁ;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    ଯଦି (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_ଗଣନା) {
        g_guardSlotTaken[index] = false;
    }
}

ଶୂନ୍ୟ Guard_ArenaRange (ଶୂନ୍ୟ** o_ତଳ, size_t* o_bytes)
{
    *o_ତଳ   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#ଯଦି d_m3_Max_Native_ଗଦା > 0

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
ଶୂନ୍ୟ* m3_Native_ଗଦା_Limit (ଶୂନ୍ୟ* i_ଗଦା_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_ଗଦା_Ptr;

#  ଯଦି M3_HAS_THREAD_LOCAL
    ସ୍ଥିର M3_THREAD_LOCAL u8* base = ଖାଲି;
    ଯଦି (base == ଖାଲି) {
        base = (u8*)m3_Host_ଗଦା_Base();
    }
#  ଅନ୍ୟଥା
    u8* base = (u8*)m3_Host_ଗଦା_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    ଯଦି (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_ଗଦା_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    ଫେରନ୍ତୁ sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#ଯଦି d_m3_Log_Native_ଗଦା

ସ୍ଥିର size_t ଗଦା_start;
ସ୍ଥିର size_t ଗଦା_end;

ଶୂନ୍ୟ m3_ଗଦା_Check_Init ()
{
    ଅକ୍ଷର ଗଦା;
    ଗଦା_end = ଗଦା_start = (size_t)&ଗଦା;
}

ଶୂନ୍ୟ m3_ଗଦା_Check ()
{
    ଅକ୍ଷର   ଗଦା;
    size_t addr = (size_t)&ଗଦା;

    size_t ଗଦା_End = ଗଦା_end;

    ଗଦା_end = M3_MIN(ଗଦା_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା m3_ଗଦା_Get_Max ()
{
    ଫେରନ୍ତୁ ଗଦା_start - ଗଦା_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    ଯଦି (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    ଅନ୍ୟଥା ଯଦି (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } ଅନ୍ୟଥା ଯଦି (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#ଯଦି d_m3HasExceptionHandling
    ଅନ୍ୟଥା ଯଦି (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#ଯଦି d_m3_Has_ଗଦା_Switching
    ଅନ୍ୟଥା ଯଦି (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    ଅନ୍ୟଥା ଯଦି (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    ଫେରନ୍ତୁ result;
}


#ଯଦି d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    ଯଦି (IsSpelledRefType(i_type)) {
        ଯଦି (i_type & d_m3Type_refCont) {
            ଫେରନ୍ତୁ c_m3Type_contref;
        }
        ଫେରନ୍ତୁ (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    ଫେରନ୍ତୁ (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    ଯଦି (i_sub == i_super) {
        ଫେରନ୍ତୁ true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    ଯଦି (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        ଫେରନ୍ତୁ false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    ଯଦି (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        ଫେରନ୍ତୁ false;
    }

#  ଯଦି d_m3_Has_ଗଦା_Switching
    // nocont sits under every continuation type
    ଯଦି (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        ଫେରନ୍ତୁ true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    ଫେରନ୍ତୁ (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ଫେରନ୍ତୁ (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ଫେରନ୍ତୁ (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#ଯଦି d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#ଯଦି d_m3_Has_ଗଦା_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ଫେରନ୍ତୁ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    ଯଦି (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        ଫେରନ୍ତୁ true;
    } ଅନ୍ୟଥା ଯଦି (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        ଫେରନ୍ତୁ false;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ (ଆକାର(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    ଯଦି (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        ଫେରନ୍ତୁ ଆକାର(i32);
    }

    ଯଦି (IsRefType(i_m3Type)) {
        ଫେରନ୍ତୁ ଆକାର(ଶୂନ୍ୟ*);
    }

    ଫେରନ୍ତୁ ଆକାର(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    ଅଚଳ u8* ptr = *io_bytes;
    ptr += ଆକାର(u64);

    ଯଦି (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ମୂଲ୍ୟ, *io_bytes, ଆକାର(u64));
        M3_BSWAP_u64(*o_ମୂଲ୍ୟ);
        *io_bytes = ptr;
        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    ଅଚଳ u8* ptr = *io_bytes;
    ptr += ଆକାର(u32);

    ଯଦି (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ମୂଲ୍ୟ, *io_bytes, ଆକାର(u32));
        M3_BSWAP_u32(*o_ମୂଲ୍ୟ);
        *io_bytes = ptr;
        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
}

#ଯଦି d_m3ImplementFloat

M3Result Read_f64 (f64* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    ଅଚଳ u8* ptr = *io_bytes;
    ptr += ଆକାର(f64);

    ଯଦି (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ମୂଲ୍ୟ, *io_bytes, ଆକାର(f64));
        M3_BSWAP_f64(*o_ମୂଲ୍ୟ);
        *io_bytes = ptr;
        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    ଅଚଳ u8* ptr = *io_bytes;
    ptr += ଆକାର(f32);

    ଯଦି (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_ମୂଲ୍ୟ, *io_bytes, ଆକାର(f32));
        M3_BSWAP_f32(*o_ମୂଲ୍ୟ);
        *io_bytes = ptr;
        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    ଅଚଳ u8* ptr = *io_bytes;

    ଯଦି (M3_LIKELY(ptr < i_end)) {
        *o_ମୂଲ୍ୟ  = *ptr;
        *io_bytes = ptr + 1;

        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    ଅଚଳ u8* ptr = *io_bytes;

    ଯଦି (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_ମୂଲ୍ୟ  = opcode;
        *io_bytes = ptr;

        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_ମୂଲ୍ୟ, u32 i_max_ଅଙ୍କ_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 ମୂଲ୍ୟ = 0;

    u32       shift = 0;
    ଅଚଳ u8* ptr   = *io_bytes;

    ଯେତେବେଳେ (ptr < i_end) {
        u64 byte = *(ptr++);

        ମୂଲ୍ୟ |= ((byte & 0x7f) << shift);
        shift += 7;

        ଯଦି (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            ଯଦି (M3_UNLIKELY(shift > i_max_ଅଙ୍କ_Bits)) {
                u32 ଅଙ୍କ_Used_Bits = i_max_ଅଙ୍କ_Bits + 7 - shift;

                ଯଦି (byte >> ଅଙ୍କ_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            ଭାଙ୍ଗ;
        }

        ଯଦି (M3_UNLIKELY(shift >= i_max_ଅଙ୍କ_Bits)) {
            result = m3Err_lebOverflow;
            ଭାଙ୍ଗ;
        }
    }

    *o_ମୂଲ୍ୟ  = ମୂଲ୍ୟ;
    *io_bytes = ptr;

    ଫେରନ୍ତୁ result;
}


M3Result ReadLebSigned (i64* o_ମୂଲ୍ୟ, u32 i_max_ଅଙ୍କ_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 ମୂଲ୍ୟ = 0;

    u32       shift = 0;
    ଅଚଳ u8* ptr   = *io_bytes;

    ଯେତେବେଳେ (ptr < i_end) {
        u64 byte = *(ptr++);

        ମୂଲ୍ୟ |= ((byte & 0x7f) << shift);
        shift += 7;

        ଯଦି (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            ଯଦି (M3_UNLIKELY(shift > i_max_ଅଙ୍କ_Bits)) {
                u32 ଅଙ୍କ_Used_Bits = i_max_ଅଙ୍କ_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (ଅଙ୍କ_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                ଯଦି (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            ଯଦି ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                ମୂଲ୍ୟ |= (~extend << shift);
            }

            ଭାଙ୍ଗ;
        }

        ଯଦି (M3_UNLIKELY(shift >= i_max_ଅଙ୍କ_Bits)) {
            result = m3Err_lebOverflow;
            ଭାଙ୍ଗ;
        }
    }

    *o_ମୂଲ୍ୟ  = ମୂଲ୍ୟ;
    *io_bytes = ptr;

    ଫେରନ୍ତୁ result;
}


M3Result ReadLEB_u32 (u32* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      ମୂଲ୍ୟ;
    M3Result result = ReadLebUnsigned(&ମୂଲ୍ୟ, 32, io_bytes, i_end);

    *o_ମୂଲ୍ୟ = (u32)ମୂଲ୍ୟ;

    ଫେରନ୍ତୁ result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    ଯଦି (result) {
        ଫେରନ୍ତୁ result;
    }

    *o_memoryIdx = 0;

#ଯଦି d_m3HasMultiMemory
    ଯଦି (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        ଯଦି (result) {
            ଫେରନ୍ତୁ result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    ଫେରନ୍ତୁ ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      ମୂଲ୍ୟ;
    M3Result result = ReadLebUnsigned(&ମୂଲ୍ୟ, 7, io_bytes, i_end);

    *o_ମୂଲ୍ୟ = (u8)ମୂଲ୍ୟ;

    ଫେରନ୍ତୁ result;
}


M3Result ReadLEB_i7 (i8* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      ମୂଲ୍ୟ;
    M3Result result = ReadLebSigned(&ମୂଲ୍ୟ, 7, io_bytes, i_end);

    *o_ମୂଲ୍ୟ = (i8)ମୂଲ୍ୟ;

    ଫେରନ୍ତୁ result;
}


M3Result ReadLEB_i32 (i32* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      ମୂଲ୍ୟ;
    M3Result result = ReadLebSigned(&ମୂଲ୍ୟ, 32, io_bytes, i_end);

    *o_ମୂଲ୍ୟ = (i32)ମୂଲ୍ୟ;

    ଫେରନ୍ତୁ result;
}


M3Result ReadLEB_i64 (i64* o_ମୂଲ୍ୟ, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      ମୂଲ୍ୟ;
    M3Result result = ReadLebSigned(&ମୂଲ୍ୟ, 64, io_bytes, i_end);

    *o_ମୂଲ୍ୟ = ମୂଲ୍ୟ;

    ଫେରନ୍ତୁ result;
}

#ଯଦି d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ସ୍ଥିର
bool IsValidUtf8 (ଅଚଳ u8* i_data, u32 i_length)
{
    ଅଚଳ u8* ptr = i_data;
    ଅଚଳ u8* end = i_data + i_length;

    ଯେତେବେଳେ (ptr < end) {
        u8 b0 = *ptr++;

        ଯଦି (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            ଜାରି;
        } ଅନ୍ୟଥା ଯଦି ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            ଯଦି (b0 < 0xC2) {
                ଫେରନ୍ତୁ false;       // overlong
            }
            ଯଦି (ptr >= end) {
                ଫେରନ୍ତୁ false;
            }
            u8 b1 = *ptr++;
            ଯଦି ((b1 & 0xC0) != 0x80) {
                ଫେରନ୍ତୁ false;
            }
        } ଅନ୍ୟଥା ଯଦି ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            ଯଦି (ptr + 1 >= end) {
                ଫେରନ୍ତୁ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            ଯଦି ((b1 & 0xC0) != 0x80) {
                ଫେରନ୍ତୁ false;
            }
            ଯଦି ((b2 & 0xC0) != 0x80) {
                ଫେରନ୍ତୁ false;
            }
            // reject overlong
            ଯଦି (b0 == 0xE0 && b1 < 0xA0) {
                ଫେରନ୍ତୁ false;
            }
            // reject surrogates U+D800..U+DFFF
            ଯଦି (b0 == 0xED && b1 >= 0xA0) {
                ଫେରନ୍ତୁ false;
            }
        } ଅନ୍ୟଥା ଯଦି ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            ଯଦି (b0 > 0xF4) {
                ଫେରନ୍ତୁ false;       // above U+10FFFF
            }
            ଯଦି (ptr + 2 >= end) {
                ଫେରନ୍ତୁ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            ଯଦି ((b1 & 0xC0) != 0x80) {
                ଫେରନ୍ତୁ false;
            }
            ଯଦି ((b2 & 0xC0) != 0x80) {
                ଫେରନ୍ତୁ false;
            }
            ଯଦି ((b3 & 0xC0) != 0x80) {
                ଫେରନ୍ତୁ false;
            }
            // reject overlong
            ଯଦି (b0 == 0xF0 && b1 < 0x90) {
                ଫେରନ୍ତୁ false;
            }
            // reject above U+10FFFF
            ଯଦି (b0 == 0xF4 && b1 > 0x8F) {
                ଫେରନ୍ତୁ false;
            }
        } ଅନ୍ୟଥା {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            ଫେରନ୍ତୁ false;
        }
    }

    ଫେରନ୍ତୁ true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = ଖାଲି;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    ଯଦି (not result) {
        ଯଦି (utf8Length <= d_m3MaxSaneUtf8Length) {
            ଅଚଳ u8* ptr = *io_bytes;
            ଯଦି (utf8Length <= (size_t)(i_end - ptr)) {
                ଅଚଳ u8* end = ptr + utf8Length;
#ଯଦି d_m3EnableValidation
                ଯଦି (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    ଫେରନ୍ତୁ m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                ଅକ୍ଷର* utf8 = (ଅକ୍ଷର*)m3_Malloc("UTF8", utf8Length + 1);

                ଯଦି (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } ଅନ୍ୟଥା {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } ଅନ୍ୟଥା {
                result = m3Err_wasmUnderrun;
            }
        } ଅନ୍ୟଥା {
            result = m3Err_missingUTF8;
        }
    }

    ଫେରନ୍ତୁ result;
}

#ଯଦି d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    ଯେତେବେଳେ (curr) {
        ଯଦି (ContainsPC(curr, i_pc)) {
            pageFound = true;
            ଭାଙ୍ଗ;
        }
        curr = curr->info.next;
    }

    ଯଦି (!pageFound) {
        curr = i_runtime->pagesFull;
        ଯେତେବେଳେ (curr) {
            ଯଦି (ContainsPC(curr, i_pc)) {
                pageFound = true;
                ଭାଙ୍ଗ;
            }
            curr = curr->info.next;
        }
    }

    ଯଦି (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (ଶୂନ୍ୟ)pcFound;                                                          d_m3Assert (pcFound);

        ଫେରନ୍ତୁ result;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ 0;
    }
}


ଶୂନ୍ୟ ଠେଲ_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    ଯଦି (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ଫେରନ୍ତୁ;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    ଯଦି (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        ଫେରନ୍ତୁ;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    ଯଦି (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } ଅନ୍ୟଥା {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


ଶୂନ୍ୟ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    ଯଦି (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ଫେରନ୍ତୁ;
    }

    ଯଦି (!io_runtime->backtrace.lastFrame) {
        ଫେରନ୍ତୁ;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


ଶୂନ୍ୟ ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    ଯେତେବେଳେ (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = ଖାଲି;
    io_runtime->backtrace.lastFrame = ଖାଲି;
}
#endif // d_m3RecordBacktraces
