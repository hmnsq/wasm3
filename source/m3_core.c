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
#അന്തര്ഭാവ "m3_config.h"
#അന്തര്ഭാവ "wasm3.h"

#അന്തര്ഭാവ "m3_core.h"
#അന്തര്ഭാവ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#അന്തര്ഭാവ "m3_host.h"

#എങ്കിൽ d_m3HasWin32Host
#  include "m3_host_win32.h"
#അല്ലെങ്കിലെങ്കിൽ d_m3HasPosixHost
#  include "m3_host_posix.h"
#അല്ലെങ്കിൽ
#  include "m3_host_none.h"
#endif

ശൂന്യം m3_Abort (സ്ഥിരം അക്ഷരം* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    തിരികെ m3Err_none;
}

#എങ്കിൽ d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

സ്ഥിതം uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    എങ്കിൽ (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    ഘടന timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    തിരികെ us - initial_ts;
}

#endif

#എങ്കിൽ d_m3FixedHeap

സ്ഥിതം u8        fixedHeap[d_m3FixedHeap];
സ്ഥിതം u8*       fixedHeapPtr  = fixedHeap;
സ്ഥിതം u8* സ്ഥിരം fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
സ്ഥിതം u8*       fixedHeapLast = ശൂന്യ;

#  എങ്കിൽ d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  അല്ലെങ്കിൽ
#    define HEAP_ALIGN_PTR(P)
#  endif

ശൂന്യം* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    എങ്കിൽ (fixedHeapPtr >= fixedHeapEnd) {
        തിരികെ ശൂന്യ;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    തിരികെ ptr;
}

ശൂന്യം m3_Free_Impl (ശൂന്യം* i_ptr)
{
    // Handle the last chunk
    എങ്കിൽ (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = ശൂന്യ;
    } അല്ലെങ്കിൽ {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

ശൂന്യം* m3_Realloc_Impl (ശൂന്യം* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    എങ്കിൽ (M3_UNLIKELY(i_newSize == i_oldSize)) {
        തിരികെ i_ptr;
    }

    ശൂന്യം* newPtr;

    // Handle the last chunk
    എങ്കിൽ (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        എങ്കിൽ (fixedHeapPtr >= fixedHeapEnd) {
            തിരികെ ശൂന്യ;
        }
        newPtr = i_ptr;
    } അല്ലെങ്കിൽ {
        newPtr = m3_Malloc_Impl(i_newSize);
        എങ്കിൽ (!newPtr) {
            തിരികെ ശൂന്യ;
        }
        എങ്കിൽ (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    എങ്കിൽ (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    തിരികെ newPtr;
}

#അല്ലെങ്കിൽ

ശൂന്യം* m3_Malloc_Impl (size_t i_size)
{
    തിരികെ calloc(i_size, 1);
}

ശൂന്യം m3_Free_Impl (ശൂന്യം* io_ptr)
{
    സ്വതന്ത്രം(io_ptr);
}

ശൂന്യം* m3_Realloc_Impl (ശൂന്യം* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    എങ്കിൽ (M3_UNLIKELY(i_newSize == i_oldSize)) {
        തിരികെ i_ptr;
    }

    ശൂന്യം* newPtr = realloc(i_ptr, i_newSize);

    എങ്കിൽ (M3_LIKELY(newPtr)) {
        എങ്കിൽ (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        തിരികെ newPtr;
    }
    തിരികെ ശൂന്യ;
}

#endif

ശൂന്യം* m3_CopyMem (സ്ഥിരം ശൂന്യം* i_from, size_t i_size)
{
    ശൂന്യം* ptr = m3_Malloc("CopyMem", i_size);
    എങ്കിൽ (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    തിരികെ ptr;
}

//--------------------------------------------------------------------------------------------

#എങ്കിൽ d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
സ്ഥിതം u8*    g_guardArena;
സ്ഥിതം size_t g_guardArenaBytes;
സ്ഥിതം size_t g_guardSlotBytes;
സ്ഥിതം size_t g_guardPageSize;
സ്ഥിതം u32    g_guard_Slot_എണ്ണം;
സ്ഥിതം bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
സ്ഥിതം
bool Guard_Reserve (ശൂന്യം)
{
    എങ്കിൽ (g_guardArena) {
        തിരികെ true;
    }

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    വേണ്ടി (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        ശൂന്യം* base = m3_HostReserve(slotBytes * slots);

        എങ്കിൽ (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_എണ്ണം  = slots;
            തിരികെ true;
        }
    }

    തിരികെ false;
}

ശൂന്യം* Guard_TakeSlot (ശൂന്യം)
{
    എങ്കിൽ (not Guard_Reserve()) {
        തിരികെ ശൂന്യ;
    }

    വേണ്ടി (u32 i = 0; i < g_guard_Slot_എണ്ണം; ++i) {
        എങ്കിൽ (g_guardSlotTaken[i]) {
            തുടരുക;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        എങ്കിൽ (not m3_HostCommit(slot, g_guardPageSize)) {
            തിരികെ ശൂന്യ;
        }

        g_guardSlotTaken[i] = true;
        തിരികെ slot;
    }

    തിരികെ ശൂന്യ;
}

M3MemoryHeader* Guard_SlotHeader (ശൂന്യം* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    തിരികെ (M3MemoryHeader*)(data - വലുപ്പം(M3MemoryHeader));
}

bool Guard_CommitSlot (ശൂന്യം* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    തിരികെ m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

ശൂന്യം Guard_GiveSlot (ശൂന്യം* i_slot)
{
    എങ്കിൽ (i_slot == ശൂന്യ) {
        തിരികെ;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    എങ്കിൽ (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_എണ്ണം) {
        g_guardSlotTaken[index] = false;
    }
}

ശൂന്യം Guard_ArenaRange (ശൂന്യം** o_താഴെയുള്ള, size_t* o_bytes)
{
    *o_താഴെയുള്ള   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#എങ്കിൽ d_m3_Max_Native_അട്ടി > 0

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
ശൂന്യം* m3_Native_അട്ടി_Limit (ശൂന്യം* i_അട്ടി_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_അട്ടി_Ptr;

#  എങ്കിൽ M3_HAS_THREAD_LOCAL
    സ്ഥിതം M3_THREAD_LOCAL u8* base = ശൂന്യ;
    എങ്കിൽ (base == ശൂന്യ) {
        base = (u8*)m3_Host_അട്ടി_Base();
    }
#  അല്ലെങ്കിൽ
    u8* base = (u8*)m3_Host_അട്ടി_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    എങ്കിൽ (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_അട്ടി_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    തിരികെ sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#എങ്കിൽ d_m3_Log_Native_അട്ടി

സ്ഥിതം size_t അട്ടി_start;
സ്ഥിതം size_t അട്ടി_end;

ശൂന്യം m3_അട്ടി_Check_Init ()
{
    അക്ഷരം അട്ടി;
    അട്ടി_end = അട്ടി_start = (size_t)&അട്ടി;
}

ശൂന്യം m3_അട്ടി_Check ()
{
    അക്ഷരം   അട്ടി;
    size_t addr = (size_t)&അട്ടി;

    size_t അട്ടി_End = അട്ടി_end;

    അട്ടി_end = M3_MIN(അട്ടി_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

പൂർണ്ണസംഖ്യ m3_അട്ടി_Get_Max ()
{
    തിരികെ അട്ടി_start - അട്ടി_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    എങ്കിൽ (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    അല്ലെങ്കിൽ എങ്കിൽ (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } അല്ലെങ്കിൽ എങ്കിൽ (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#എങ്കിൽ d_m3HasExceptionHandling
    അല്ലെങ്കിൽ എങ്കിൽ (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    അല്ലെങ്കിൽ എങ്കിൽ (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    അല്ലെങ്കിൽ എങ്കിൽ (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    തിരികെ result;
}


#എങ്കിൽ d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    എങ്കിൽ (IsSpelledRefType(i_type)) {
        എങ്കിൽ (i_type & d_m3Type_refCont) {
            തിരികെ c_m3Type_contref;
        }
        തിരികെ (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    തിരികെ (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    എങ്കിൽ (i_sub == i_super) {
        തിരികെ true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    എങ്കിൽ (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        തിരികെ false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    എങ്കിൽ (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        തിരികെ false;
    }

#  എങ്കിൽ d_m3_Has_അട്ടി_Switching
    // nocont sits under every continuation type
    എങ്കിൽ (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        തിരികെ true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    തിരികെ (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    തിരികെ (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    തിരികെ (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#എങ്കിൽ d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    തിരികെ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    എങ്കിൽ (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        തിരികെ true;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        തിരികെ false;
    } അല്ലെങ്കിൽ {
        തിരികെ (വലുപ്പം(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    എങ്കിൽ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        തിരികെ വലുപ്പം(i32);
    }

    എങ്കിൽ (IsRefType(i_m3Type)) {
        തിരികെ വലുപ്പം(ശൂന്യം*);
    }

    തിരികെ വലുപ്പം(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    സ്ഥിരം u8* ptr = *io_bytes;
    ptr += വലുപ്പം(u64);

    എങ്കിൽ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_മൂല്യം, *io_bytes, വലുപ്പം(u64));
        M3_BSWAP_u64(*o_മൂല്യം);
        *io_bytes = ptr;
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    സ്ഥിരം u8* ptr = *io_bytes;
    ptr += വലുപ്പം(u32);

    എങ്കിൽ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_മൂല്യം, *io_bytes, വലുപ്പം(u32));
        M3_BSWAP_u32(*o_മൂല്യം);
        *io_bytes = ptr;
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3Err_wasmUnderrun;
    }
}

#എങ്കിൽ d_m3ImplementFloat

M3Result Read_f64 (f64* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    സ്ഥിരം u8* ptr = *io_bytes;
    ptr += വലുപ്പം(f64);

    എങ്കിൽ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_മൂല്യം, *io_bytes, വലുപ്പം(f64));
        M3_BSWAP_f64(*o_മൂല്യം);
        *io_bytes = ptr;
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    സ്ഥിരം u8* ptr = *io_bytes;
    ptr += വലുപ്പം(f32);

    എങ്കിൽ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_മൂല്യം, *io_bytes, വലുപ്പം(f32));
        M3_BSWAP_f32(*o_മൂല്യം);
        *io_bytes = ptr;
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    സ്ഥിരം u8* ptr = *io_bytes;

    എങ്കിൽ (M3_LIKELY(ptr < i_end)) {
        *o_മൂല്യം  = *ptr;
        *io_bytes = ptr + 1;

        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    സ്ഥിരം u8* ptr = *io_bytes;

    എങ്കിൽ (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_മൂല്യം  = opcode;
        *io_bytes = ptr;

        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_മൂല്യം, u32 i_max_അക്കം_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 മൂല്യം = 0;

    u32       shift = 0;
    സ്ഥിരം u8* ptr   = *io_bytes;

    ആകുമ്പോൾ (ptr < i_end) {
        u64 byte = *(ptr++);

        മൂല്യം |= ((byte & 0x7f) << shift);
        shift += 7;

        എങ്കിൽ (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            എങ്കിൽ (M3_UNLIKELY(shift > i_max_അക്കം_Bits)) {
                u32 അക്കം_Used_Bits = i_max_അക്കം_Bits + 7 - shift;

                എങ്കിൽ (byte >> അക്കം_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            നിർത്തുക;
        }

        എങ്കിൽ (M3_UNLIKELY(shift >= i_max_അക്കം_Bits)) {
            result = m3Err_lebOverflow;
            നിർത്തുക;
        }
    }

    *o_മൂല്യം  = മൂല്യം;
    *io_bytes = ptr;

    തിരികെ result;
}


M3Result ReadLebSigned (i64* o_മൂല്യം, u32 i_max_അക്കം_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 മൂല്യം = 0;

    u32       shift = 0;
    സ്ഥിരം u8* ptr   = *io_bytes;

    ആകുമ്പോൾ (ptr < i_end) {
        u64 byte = *(ptr++);

        മൂല്യം |= ((byte & 0x7f) << shift);
        shift += 7;

        എങ്കിൽ (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            എങ്കിൽ (M3_UNLIKELY(shift > i_max_അക്കം_Bits)) {
                u32 അക്കം_Used_Bits = i_max_അക്കം_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (അക്കം_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                എങ്കിൽ (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            എങ്കിൽ ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                മൂല്യം |= (~extend << shift);
            }

            നിർത്തുക;
        }

        എങ്കിൽ (M3_UNLIKELY(shift >= i_max_അക്കം_Bits)) {
            result = m3Err_lebOverflow;
            നിർത്തുക;
        }
    }

    *o_മൂല്യം  = മൂല്യം;
    *io_bytes = ptr;

    തിരികെ result;
}


M3Result ReadLEB_u32 (u32* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      മൂല്യം;
    M3Result result = ReadLebUnsigned(&മൂല്യം, 32, io_bytes, i_end);

    *o_മൂല്യം = (u32)മൂല്യം;

    തിരികെ result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    എങ്കിൽ (result) {
        തിരികെ result;
    }

    *o_memoryIdx = 0;

#എങ്കിൽ d_m3HasMultiMemory
    എങ്കിൽ (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        എങ്കിൽ (result) {
            തിരികെ result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    തിരികെ ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      മൂല്യം;
    M3Result result = ReadLebUnsigned(&മൂല്യം, 7, io_bytes, i_end);

    *o_മൂല്യം = (u8)മൂല്യം;

    തിരികെ result;
}


M3Result ReadLEB_i7 (i8* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      മൂല്യം;
    M3Result result = ReadLebSigned(&മൂല്യം, 7, io_bytes, i_end);

    *o_മൂല്യം = (i8)മൂല്യം;

    തിരികെ result;
}


M3Result ReadLEB_i32 (i32* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      മൂല്യം;
    M3Result result = ReadLebSigned(&മൂല്യം, 32, io_bytes, i_end);

    *o_മൂല്യം = (i32)മൂല്യം;

    തിരികെ result;
}


M3Result ReadLEB_i64 (i64* o_മൂല്യം, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      മൂല്യം;
    M3Result result = ReadLebSigned(&മൂല്യം, 64, io_bytes, i_end);

    *o_മൂല്യം = മൂല്യം;

    തിരികെ result;
}

#എങ്കിൽ d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
സ്ഥിതം
bool IsValidUtf8 (സ്ഥിരം u8* i_data, u32 i_length)
{
    സ്ഥിരം u8* ptr = i_data;
    സ്ഥിരം u8* end = i_data + i_length;

    ആകുമ്പോൾ (ptr < end) {
        u8 b0 = *ptr++;

        എങ്കിൽ (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            തുടരുക;
        } അല്ലെങ്കിൽ എങ്കിൽ ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            എങ്കിൽ (b0 < 0xC2) {
                തിരികെ false;       // overlong
            }
            എങ്കിൽ (ptr >= end) {
                തിരികെ false;
            }
            u8 b1 = *ptr++;
            എങ്കിൽ ((b1 & 0xC0) != 0x80) {
                തിരികെ false;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            എങ്കിൽ (ptr + 1 >= end) {
                തിരികെ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            എങ്കിൽ ((b1 & 0xC0) != 0x80) {
                തിരികെ false;
            }
            എങ്കിൽ ((b2 & 0xC0) != 0x80) {
                തിരികെ false;
            }
            // reject overlong
            എങ്കിൽ (b0 == 0xE0 && b1 < 0xA0) {
                തിരികെ false;
            }
            // reject surrogates U+D800..U+DFFF
            എങ്കിൽ (b0 == 0xED && b1 >= 0xA0) {
                തിരികെ false;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            എങ്കിൽ (b0 > 0xF4) {
                തിരികെ false;       // above U+10FFFF
            }
            എങ്കിൽ (ptr + 2 >= end) {
                തിരികെ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            എങ്കിൽ ((b1 & 0xC0) != 0x80) {
                തിരികെ false;
            }
            എങ്കിൽ ((b2 & 0xC0) != 0x80) {
                തിരികെ false;
            }
            എങ്കിൽ ((b3 & 0xC0) != 0x80) {
                തിരികെ false;
            }
            // reject overlong
            എങ്കിൽ (b0 == 0xF0 && b1 < 0x90) {
                തിരികെ false;
            }
            // reject above U+10FFFF
            എങ്കിൽ (b0 == 0xF4 && b1 > 0x8F) {
                തിരികെ false;
            }
        } അല്ലെങ്കിൽ {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            തിരികെ false;
        }
    }

    തിരികെ true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = ശൂന്യ;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    എങ്കിൽ (not result) {
        എങ്കിൽ (utf8Length <= d_m3MaxSaneUtf8Length) {
            സ്ഥിരം u8* ptr = *io_bytes;
            എങ്കിൽ (utf8Length <= (size_t)(i_end - ptr)) {
                സ്ഥിരം u8* end = ptr + utf8Length;
#എങ്കിൽ d_m3EnableValidation
                എങ്കിൽ (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    തിരികെ m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                അക്ഷരം* utf8 = (അക്ഷരം*)m3_Malloc("UTF8", utf8Length + 1);

                എങ്കിൽ (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } അല്ലെങ്കിൽ {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } അല്ലെങ്കിൽ {
                result = m3Err_wasmUnderrun;
            }
        } അല്ലെങ്കിൽ {
            result = m3Err_missingUTF8;
        }
    }

    തിരികെ result;
}

#എങ്കിൽ d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    ആകുമ്പോൾ (curr) {
        എങ്കിൽ (ContainsPC(curr, i_pc)) {
            pageFound = true;
            നിർത്തുക;
        }
        curr = curr->info.next;
    }

    എങ്കിൽ (!pageFound) {
        curr = i_runtime->pagesFull;
        ആകുമ്പോൾ (curr) {
            എങ്കിൽ (ContainsPC(curr, i_pc)) {
                pageFound = true;
                നിർത്തുക;
            }
            curr = curr->info.next;
        }
    }

    എങ്കിൽ (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (ശൂന്യം)pcFound;                                                          d_m3Assert (pcFound);

        തിരികെ result;
    } അല്ലെങ്കിൽ {
        തിരികെ 0;
    }
}


ശൂന്യം തള്ളുക_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    എങ്കിൽ (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        തിരികെ;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    എങ്കിൽ (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        തിരികെ;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    എങ്കിൽ (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } അല്ലെങ്കിൽ {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


ശൂന്യം FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    എങ്കിൽ (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        തിരികെ;
    }

    എങ്കിൽ (!io_runtime->backtrace.lastFrame) {
        തിരികെ;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


ശൂന്യം ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    ആകുമ്പോൾ (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = ശൂന്യ;
    io_runtime->backtrace.lastFrame = ശൂന്യ;
}
#endif // d_m3RecordBacktraces
