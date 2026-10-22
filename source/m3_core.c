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
#include "m3_config.h"
#include "wasm3.h"

#include "m3_core.h"
#include "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#include "m3_host.h"

#ᰀᰧ d_m3HasWin32Host
#  include "m3_host_win32.h"
#elif d_m3HasPosixHost
#  include "m3_host_posix.h"
#ᰀᰧᰀ
#  include "m3_host_none.h"
#endif

void m3_Abort (const var_na* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    ᰀᰧᰌ m3Err_none;
}

#ᰀᰧ d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

static uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    ᰀᰧ (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    sa_nracanaa timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    ᰀᰧᰌ us - initial_ts;
}

#endif

#ᰀᰧ d_m3FixedHeap

static u8        fixedHeap[d_m3FixedHeap];
static u8*       fixedHeapPtr  = fixedHeap;
static u8* const fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
static u8*       fixedHeapLast = NULL;

#  ᰀᰧ d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  ᰀᰧᰀ
#    define HEAP_ALIGN_PTR(P)
#  endif

void* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    ᰀᰧ (fixedHeapPtr >= fixedHeapEnd) {
        ᰀᰧᰌ NULL;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    ᰀᰧᰌ ptr;
}

void m3_Free_Impl (void* i_ptr)
{
    // Handle the last chunk
    ᰀᰧ (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = NULL;
    } ᰀᰧᰀ {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

void* m3_Realloc_Impl (void* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    ᰀᰧ (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ᰀᰧᰌ i_ptr;
    }

    void* newPtr;

    // Handle the last chunk
    ᰀᰧ (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        ᰀᰧ (fixedHeapPtr >= fixedHeapEnd) {
            ᰀᰧᰌ NULL;
        }
        newPtr = i_ptr;
    } ᰀᰧᰀ {
        newPtr = m3_Malloc_Impl(i_newSize);
        ᰀᰧ (!newPtr) {
            ᰀᰧᰌ NULL;
        }
        ᰀᰧ (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    ᰀᰧ (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    ᰀᰧᰌ newPtr;
}

#ᰀᰧᰀ

void* m3_Malloc_Impl (size_t i_size)
{
    ᰀᰧᰌ calloc(i_size, 1);
}

void m3_Free_Impl (void* io_ptr)
{
    free(io_ptr);
}

void* m3_Realloc_Impl (void* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    ᰀᰧ (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ᰀᰧᰌ i_ptr;
    }

    void* newPtr = realloc(i_ptr, i_newSize);

    ᰀᰧ (M3_LIKELY(newPtr)) {
        ᰀᰧ (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        ᰀᰧᰌ newPtr;
    }
    ᰀᰧᰌ NULL;
}

#endif

void* m3_CopyMem (const void* i_from, size_t i_size)
{
    void* ptr = m3_Malloc("CopyMem", i_size);
    ᰀᰧ (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    ᰀᰧᰌ ptr;
}

//--------------------------------------------------------------------------------------------

#ᰀᰧ d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
static u8*    g_guardArena;
static size_t g_guardArenaBytes;
static size_t g_guardSlotBytes;
static size_t g_guardPageSize;
static u32    g_guardSlotCount;
static bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
static
bool Guard_Reserve (void)
{
    ᰀᰧ (g_guardArena) {
        ᰀᰧᰌ true;
    }

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    for (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        void* base = m3_HostReserve(slotBytes * slots);

        ᰀᰧ (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guardSlotCount  = slots;
            ᰀᰧᰌ true;
        }
    }

    ᰀᰧᰌ false;
}

void* Guard_TakeSlot (void)
{
    ᰀᰧ (not Guard_Reserve()) {
        ᰀᰧᰌ NULL;
    }

    for (u32 i = 0; i < g_guardSlotCount; ++i) {
        ᰀᰧ (g_guardSlotTaken[i]) {
            continue;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        ᰀᰧ (not m3_HostCommit(slot, g_guardPageSize)) {
            ᰀᰧᰌ NULL;
        }

        g_guardSlotTaken[i] = true;
        ᰀᰧᰌ slot;
    }

    ᰀᰧᰌ NULL;
}

M3MemoryHeader* Guard_SlotHeader (void* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    ᰀᰧᰌ (M3MemoryHeader*)(data - parimaa_na(M3MemoryHeader));
}

bool Guard_CommitSlot (void* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    ᰀᰧᰌ m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

void Guard_GiveSlot (void* i_slot)
{
    ᰀᰧ (i_slot == NULL) {
        ᰀᰧᰌ;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    ᰀᰧ (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guardSlotCount) {
        g_guardSlotTaken[index] = false;
    }
}

void Guard_ArenaRange (void** o_low, size_t* o_bytes)
{
    *o_low   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#ᰀᰧ d_m3MaxNativeStack > 0

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
void* m3_NativeStackLimit (void* i_stackPtr, size_t i_budget)
{
    u8* sp = (u8*)i_stackPtr;

#  ᰀᰧ M3_HAS_THREAD_LOCAL
    static M3_THREAD_LOCAL u8* base = NULL;
    ᰀᰧ (base == NULL) {
        base = (u8*)m3_HostStackBase();
    }
#  ᰀᰧᰀ
    u8* base = (u8*)m3_HostStackBase();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    ᰀᰧ (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3NativeStackMargin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    ᰀᰧᰌ sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#ᰀᰧ d_m3LogNativeStack

static size_t stack_start;
static size_t stack_end;

void m3StackCheckInit ()
{
    var_na stack;
    stack_end = stack_start = (size_t)&stack;
}

void m3StackCheck ()
{
    var_na   stack;
    size_t addr = (size_t)&stack;

    size_t stackEnd = stack_end;

    stack_end = M3_MIN(stack_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

puur_naa_nka m3StackGetMax ()
{
    ᰀᰧᰌ stack_start - stack_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    ᰀᰧ (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    ᰀᰧᰀ ᰀᰧ (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } ᰀᰧᰀ ᰀᰧ (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#ᰀᰧ d_m3HasExceptionHandling
    ᰀᰧᰀ ᰀᰧ (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#ᰀᰧ d_m3HasStackSwitching
    ᰀᰧᰀ ᰀᰧ (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    ᰀᰧᰀ ᰀᰧ (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    ᰀᰧᰌ result;
}


#ᰀᰧ d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    ᰀᰧ (IsSpelledRefType(i_type)) {
        ᰀᰧ (i_type & d_m3Type_refCont) {
            ᰀᰧᰌ c_m3Type_contref;
        }
        ᰀᰧᰌ (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    ᰀᰧᰌ (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    ᰀᰧ (i_sub == i_super) {
        ᰀᰧᰌ true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    ᰀᰧ (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        ᰀᰧᰌ false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    ᰀᰧ (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        ᰀᰧᰌ false;
    }

#  ᰀᰧ d_m3HasStackSwitching
    // nocont sits under every continuation type
    ᰀᰧ (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        ᰀᰧᰌ true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    ᰀᰧᰌ (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ᰀᰧᰌ (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ᰀᰧᰌ (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#ᰀᰧ d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#ᰀᰧ d_m3HasStackSwitching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ᰀᰧᰌ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    ᰀᰧ (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        ᰀᰧᰌ true;
    } ᰀᰧᰀ ᰀᰧ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        ᰀᰧᰌ false;
    } ᰀᰧᰀ {
        ᰀᰧᰌ (parimaa_na(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    ᰀᰧ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        ᰀᰧᰌ parimaa_na(i32);
    }

    ᰀᰧ (IsRefType(i_m3Type)) {
        ᰀᰧᰌ parimaa_na(void*);
    }

    ᰀᰧᰌ parimaa_na(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    const u8* ptr = *io_bytes;
    ptr += parimaa_na(u64);

    ᰀᰧ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_value, *io_bytes, parimaa_na(u64));
        M3_BSWAP_u64(*o_value);
        *io_bytes = ptr;
        ᰀᰧᰌ m3Err_none;
    } ᰀᰧᰀ {
        ᰀᰧᰌ m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    const u8* ptr = *io_bytes;
    ptr += parimaa_na(u32);

    ᰀᰧ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_value, *io_bytes, parimaa_na(u32));
        M3_BSWAP_u32(*o_value);
        *io_bytes = ptr;
        ᰀᰧᰌ m3Err_none;
    } ᰀᰧᰀ {
        ᰀᰧᰌ m3Err_wasmUnderrun;
    }
}

#ᰀᰧ d_m3ImplementFloat

M3Result Read_f64 (f64* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    const u8* ptr = *io_bytes;
    ptr += parimaa_na(f64);

    ᰀᰧ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_value, *io_bytes, parimaa_na(f64));
        M3_BSWAP_f64(*o_value);
        *io_bytes = ptr;
        ᰀᰧᰌ m3Err_none;
    } ᰀᰧᰀ {
        ᰀᰧᰌ m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    const u8* ptr = *io_bytes;
    ptr += parimaa_na(f32);

    ᰀᰧ (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_value, *io_bytes, parimaa_na(f32));
        M3_BSWAP_f32(*o_value);
        *io_bytes = ptr;
        ᰀᰧᰌ m3Err_none;
    } ᰀᰧᰀ {
        ᰀᰧᰌ m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    const u8* ptr = *io_bytes;

    ᰀᰧ (M3_LIKELY(ptr < i_end)) {
        *o_value  = *ptr;
        *io_bytes = ptr + 1;

        ᰀᰧᰌ m3Err_none;
    } ᰀᰧᰀ {
        ᰀᰧᰌ m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    const u8* ptr = *io_bytes;

    ᰀᰧ (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_value  = opcode;
        *io_bytes = ptr;

        ᰀᰧᰌ m3Err_none;
    } ᰀᰧᰀ {
        ᰀᰧᰌ m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_value, u32 i_maxNumBits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 value = 0;

    u32       shift = 0;
    const u8* ptr   = *io_bytes;

    ᰀᰬ (ptr < i_end) {
        u64 byte = *(ptr++);

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        ᰀᰧ (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            ᰀᰧ (M3_UNLIKELY(shift > i_maxNumBits)) {
                u32 numUsedBits = i_maxNumBits + 7 - shift;

                ᰀᰧ (byte >> numUsedBits) {
                    result = m3Err_lebOverflow;
                }
            }
            bha_nga;
        }

        ᰀᰧ (M3_UNLIKELY(shift >= i_maxNumBits)) {
            result = m3Err_lebOverflow;
            bha_nga;
        }
    }

    *o_value  = value;
    *io_bytes = ptr;

    ᰀᰧᰌ result;
}


M3Result ReadLebSigned (i64* o_value, u32 i_maxNumBits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 value = 0;

    u32       shift = 0;
    const u8* ptr   = *io_bytes;

    ᰀᰬ (ptr < i_end) {
        u64 byte = *(ptr++);

        value |= ((byte & 0x7f) << shift);
        shift += 7;

        ᰀᰧ (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            ᰀᰧ (M3_UNLIKELY(shift > i_maxNumBits)) {
                u32 numUsedBits = i_maxNumBits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (numUsedBits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                ᰀᰧ (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            ᰀᰧ ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                value |= (~extend << shift);
            }

            bha_nga;
        }

        ᰀᰧ (M3_UNLIKELY(shift >= i_maxNumBits)) {
            result = m3Err_lebOverflow;
            bha_nga;
        }
    }

    *o_value  = value;
    *io_bytes = ptr;

    ᰀᰧᰌ result;
}


M3Result ReadLEB_u32 (u32* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      value;
    M3Result result = ReadLebUnsigned(&value, 32, io_bytes, i_end);

    *o_value = (u32)value;

    ᰀᰧᰌ result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    ᰀᰧ (result) {
        ᰀᰧᰌ result;
    }

    *o_memoryIdx = 0;

#ᰀᰧ d_m3HasMultiMemory
    ᰀᰧ (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        ᰀᰧ (result) {
            ᰀᰧᰌ result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    ᰀᰧᰌ ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      value;
    M3Result result = ReadLebUnsigned(&value, 7, io_bytes, i_end);

    *o_value = (u8)value;

    ᰀᰧᰌ result;
}


M3Result ReadLEB_i7 (i8* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      value;
    M3Result result = ReadLebSigned(&value, 7, io_bytes, i_end);

    *o_value = (i8)value;

    ᰀᰧᰌ result;
}


M3Result ReadLEB_i32 (i32* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      value;
    M3Result result = ReadLebSigned(&value, 32, io_bytes, i_end);

    *o_value = (i32)value;

    ᰀᰧᰌ result;
}


M3Result ReadLEB_i64 (i64* o_value, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      value;
    M3Result result = ReadLebSigned(&value, 64, io_bytes, i_end);

    *o_value = value;

    ᰀᰧᰌ result;
}

#ᰀᰧ d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
static
bool IsValidUtf8 (const u8* i_data, u32 i_length)
{
    const u8* ptr = i_data;
    const u8* end = i_data + i_length;

    ᰀᰬ (ptr < end) {
        u8 b0 = *ptr++;

        ᰀᰧ (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            continue;
        } ᰀᰧᰀ ᰀᰧ ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            ᰀᰧ (b0 < 0xC2) {
                ᰀᰧᰌ false;       // overlong
            }
            ᰀᰧ (ptr >= end) {
                ᰀᰧᰌ false;
            }
            u8 b1 = *ptr++;
            ᰀᰧ ((b1 & 0xC0) != 0x80) {
                ᰀᰧᰌ false;
            }
        } ᰀᰧᰀ ᰀᰧ ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            ᰀᰧ (ptr + 1 >= end) {
                ᰀᰧᰌ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            ᰀᰧ ((b1 & 0xC0) != 0x80) {
                ᰀᰧᰌ false;
            }
            ᰀᰧ ((b2 & 0xC0) != 0x80) {
                ᰀᰧᰌ false;
            }
            // reject overlong
            ᰀᰧ (b0 == 0xE0 && b1 < 0xA0) {
                ᰀᰧᰌ false;
            }
            // reject surrogates U+D800..U+DFFF
            ᰀᰧ (b0 == 0xED && b1 >= 0xA0) {
                ᰀᰧᰌ false;
            }
        } ᰀᰧᰀ ᰀᰧ ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            ᰀᰧ (b0 > 0xF4) {
                ᰀᰧᰌ false;       // above U+10FFFF
            }
            ᰀᰧ (ptr + 2 >= end) {
                ᰀᰧᰌ false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            ᰀᰧ ((b1 & 0xC0) != 0x80) {
                ᰀᰧᰌ false;
            }
            ᰀᰧ ((b2 & 0xC0) != 0x80) {
                ᰀᰧᰌ false;
            }
            ᰀᰧ ((b3 & 0xC0) != 0x80) {
                ᰀᰧᰌ false;
            }
            // reject overlong
            ᰀᰧ (b0 == 0xF0 && b1 < 0x90) {
                ᰀᰧᰌ false;
            }
            // reject above U+10FFFF
            ᰀᰧ (b0 == 0xF4 && b1 > 0x8F) {
                ᰀᰧᰌ false;
            }
        } ᰀᰧᰀ {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            ᰀᰧᰌ false;
        }
    }

    ᰀᰧᰌ true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = NULL;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    ᰀᰧ (not result) {
        ᰀᰧ (utf8Length <= d_m3MaxSaneUtf8Length) {
            const u8* ptr = *io_bytes;
            ᰀᰧ (utf8Length <= (size_t)(i_end - ptr)) {
                const u8* end = ptr + utf8Length;
#ᰀᰧ d_m3EnableValidation
                ᰀᰧ (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    ᰀᰧᰌ m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                var_na* utf8 = (var_na*)m3_Malloc("UTF8", utf8Length + 1);

                ᰀᰧ (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } ᰀᰧᰀ {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } ᰀᰧᰀ {
                result = m3Err_wasmUnderrun;
            }
        } ᰀᰧᰀ {
            result = m3Err_missingUTF8;
        }
    }

    ᰀᰧᰌ result;
}

#ᰀᰧ d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    ᰀᰬ (curr) {
        ᰀᰧ (ContainsPC(curr, i_pc)) {
            pageFound = true;
            bha_nga;
        }
        curr = curr->info.next;
    }

    ᰀᰧ (!pageFound) {
        curr = i_runtime->pagesFull;
        ᰀᰬ (curr) {
            ᰀᰧ (ContainsPC(curr, i_pc)) {
                pageFound = true;
                bha_nga;
            }
            curr = curr->info.next;
        }
    }

    ᰀᰧ (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (void)pcFound;                                                          d_m3Assert (pcFound);

        ᰀᰧᰌ result;
    } ᰀᰧᰀ {
        ᰀᰧᰌ 0;
    }
}


void PushBacktraceFrame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    ᰀᰧ (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ᰀᰧᰌ;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    ᰀᰧ (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        ᰀᰧᰌ;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    ᰀᰧ (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } ᰀᰧᰀ {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


void FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    ᰀᰧ (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ᰀᰧᰌ;
    }

    ᰀᰧ (!io_runtime->backtrace.lastFrame) {
        ᰀᰧᰌ;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


void ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    ᰀᰬ (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = NULL;
    io_runtime->backtrace.lastFrame = NULL;
}
#endif // d_m3RecordBacktraces
