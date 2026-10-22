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
#অন্তর্ভাৱ "m3_config.h"
#অন্তর্ভাৱ "wasm3.h"

#অন্তর্ভাৱ "m3_core.h"
#অন্তর্ভাৱ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#অন্তর্ভাৱ "m3_host.h"

#যদি d_m3HasWin32Host
#  include "m3_host_win32.h"
#নোচেদ্যদি d_m3HasPosixHost
#  include "m3_host_posix.h"
#অন্যথা
#  include "m3_host_none.h"
#endif

শূন্য m3_Abort (ধ্রুৱ ৱর্ণ* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    ঘূৰাই m3Err_none;
}

#যদি d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

স্থির uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    যদি (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    সংরচনা timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    ঘূৰাই us - initial_ts;
}

#endif

#যদি d_m3FixedHeap

স্থির u8        fixedHeap[d_m3FixedHeap];
স্থির u8*       fixedHeapPtr  = fixedHeap;
স্থির u8* ধ্রুৱ fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
স্থির u8*       fixedHeapLast = রিক্ত;

#  যদি d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  অন্যথা
#    define HEAP_ALIGN_PTR(P)
#  endif

শূন্য* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    যদি (fixedHeapPtr >= fixedHeapEnd) {
        ঘূৰাই রিক্ত;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    ঘূৰাই ptr;
}

শূন্য m3_Free_Impl (শূন্য* i_ptr)
{
    // Handle the last chunk
    যদি (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = রিক্ত;
    } অন্যথা {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

শূন্য* m3_Realloc_Impl (শূন্য* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    যদি (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ঘূৰাই i_ptr;
    }

    শূন্য* newPtr;

    // Handle the last chunk
    যদি (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        যদি (fixedHeapPtr >= fixedHeapEnd) {
            ঘূৰাই রিক্ত;
        }
        newPtr = i_ptr;
    } অন্যথা {
        newPtr = m3_Malloc_Impl(i_newSize);
        যদি (!newPtr) {
            ঘূৰাই রিক্ত;
        }
        যদি (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    যদি (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    ঘূৰাই newPtr;
}

#অন্যথা

শূন্য* m3_Malloc_Impl (size_t i_size)
{
    ঘূৰাই calloc(i_size, 1);
}

শূন্য m3_Free_Impl (শূন্য* io_ptr)
{
    মুক্ত(io_ptr);
}

শূন্য* m3_Realloc_Impl (শূন্য* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    যদি (M3_UNLIKELY(i_newSize == i_oldSize)) {
        ঘূৰাই i_ptr;
    }

    শূন্য* newPtr = realloc(i_ptr, i_newSize);

    যদি (M3_LIKELY(newPtr)) {
        যদি (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        ঘূৰাই newPtr;
    }
    ঘূৰাই রিক্ত;
}

#endif

শূন্য* m3_CopyMem (ধ্রুৱ শূন্য* i_from, size_t i_size)
{
    শূন্য* ptr = m3_Malloc("CopyMem", i_size);
    যদি (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    ঘূৰাই ptr;
}

//--------------------------------------------------------------------------------------------

#যদি d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
স্থির u8*    g_guardArena;
স্থির size_t g_guardArenaBytes;
স্থির size_t g_guardSlotBytes;
স্থির size_t g_guardPageSize;
স্থির u32    g_guard_Slot_গুনতি;
স্থির bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
স্থির
bool Guard_Reserve (শূন্য)
{
    যদি (g_guardArena) {
        ঘূৰাই true;
    }

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    বাবে (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        শূন্য* base = m3_HostReserve(slotBytes * slots);

        যদি (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_গুনতি  = slots;
            ঘূৰাই true;
        }
    }

    ঘূৰাই false;
}

শূন্য* Guard_TakeSlot (শূন্য)
{
    যদি (not Guard_Reserve()) {
        ঘূৰাই রিক্ত;
    }

    বাবে (u32 i = 0; i < g_guard_Slot_গুনতি; ++i) {
        যদি (g_guardSlotTaken[i]) {
            অনুৱর্ত;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        যদি (not m3_HostCommit(slot, g_guardPageSize)) {
            ঘূৰাই রিক্ত;
        }

        g_guardSlotTaken[i] = true;
        ঘূৰাই slot;
    }

    ঘূৰাই রিক্ত;
}

M3MemoryHeader* Guard_SlotHeader (শূন্য* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    ঘূৰাই (M3MemoryHeader*)(data - পরিমাণ(M3MemoryHeader));
}

bool Guard_CommitSlot (শূন্য* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    ঘূৰাই m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

শূন্য Guard_GiveSlot (শূন্য* i_slot)
{
    যদি (i_slot == রিক্ত) {
        ঘূৰাই;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    যদি (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_গুনতি) {
        g_guardSlotTaken[index] = false;
    }
}

শূন্য Guard_ArenaRange (শূন্য** o_নিচেৰ, size_t* o_bytes)
{
    *o_নিচেৰ   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#যদি d_m3_Max_Native_স্তূপ > 0

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
শূন্য* m3_Native_স্তূপ_Limit (শূন্য* i_স্তূপ_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_স্তূপ_Ptr;

#  যদি M3_HAS_THREAD_LOCAL
    স্থির M3_THREAD_LOCAL u8* base = রিক্ত;
    যদি (base == রিক্ত) {
        base = (u8*)m3_Host_স্তূপ_Base();
    }
#  অন্যথা
    u8* base = (u8*)m3_Host_স্তূপ_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    যদি (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_স্তূপ_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    ঘূৰাই sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#যদি d_m3_Log_Native_স্তূপ

স্থির size_t স্তূপ_start;
স্থির size_t স্তূপ_end;

শূন্য m3_স্তূপ_Check_Init ()
{
    ৱর্ণ স্তূপ;
    স্তূপ_end = স্তূপ_start = (size_t)&স্তূপ;
}

শূন্য m3_স্তূপ_Check ()
{
    ৱর্ণ   স্তূপ;
    size_t addr = (size_t)&স্তূপ;

    size_t স্তূপ_End = স্তূপ_end;

    স্তূপ_end = M3_MIN(স্তূপ_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

পূর্ণাঙ্ক m3_স্তূপ_Get_Max ()
{
    ঘূৰাই স্তূপ_start - স্তূপ_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    যদি (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    অন্যথা যদি (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } অন্যথা যদি (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#যদি d_m3HasExceptionHandling
    অন্যথা যদি (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#যদি d_m3_Has_স্তূপ_Switching
    অন্যথা যদি (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    অন্যথা যদি (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    ঘূৰাই result;
}


#যদি d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    যদি (IsSpelledRefType(i_type)) {
        যদি (i_type & d_m3Type_refCont) {
            ঘূৰাই c_m3Type_contref;
        }
        ঘূৰাই (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    ঘূৰাই (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    যদি (i_sub == i_super) {
        ঘূৰাই true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    যদি (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        ঘূৰাই false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    যদি (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        ঘূৰাই false;
    }

#  যদি d_m3_Has_স্তূপ_Switching
    // nocont sits under every continuation type
    যদি (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        ঘূৰাই true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    ঘূৰাই (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ঘূৰাই (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ঘূৰাই (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#যদি d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#যদি d_m3_Has_স্তূপ_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ঘূৰাই (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    যদি (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        ঘূৰাই true;
    } অন্যথা যদি (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        ঘূৰাই false;
    } অন্যথা {
        ঘূৰাই (পরিমাণ(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    যদি (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        ঘূৰাই পরিমাণ(i32);
    }

    যদি (IsRefType(i_m3Type)) {
        ঘূৰাই পরিমাণ(শূন্য*);
    }

    ঘূৰাই পরিমাণ(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুৱ u8* ptr = *io_bytes;
    ptr += পরিমাণ(u64);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, পরিমাণ(u64));
        M3_BSWAP_u64(*o_মান);
        *io_bytes = ptr;
        ঘূৰাই m3Err_none;
    } অন্যথা {
        ঘূৰাই m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুৱ u8* ptr = *io_bytes;
    ptr += পরিমাণ(u32);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, পরিমাণ(u32));
        M3_BSWAP_u32(*o_মান);
        *io_bytes = ptr;
        ঘূৰাই m3Err_none;
    } অন্যথা {
        ঘূৰাই m3Err_wasmUnderrun;
    }
}

#যদি d_m3ImplementFloat

M3Result Read_f64 (f64* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুৱ u8* ptr = *io_bytes;
    ptr += পরিমাণ(f64);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, পরিমাণ(f64));
        M3_BSWAP_f64(*o_মান);
        *io_bytes = ptr;
        ঘূৰাই m3Err_none;
    } অন্যথা {
        ঘূৰাই m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুৱ u8* ptr = *io_bytes;
    ptr += পরিমাণ(f32);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, পরিমাণ(f32));
        M3_BSWAP_f32(*o_মান);
        *io_bytes = ptr;
        ঘূৰাই m3Err_none;
    } অন্যথা {
        ঘূৰাই m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুৱ u8* ptr = *io_bytes;

    যদি (M3_LIKELY(ptr < i_end)) {
        *o_মান  = *ptr;
        *io_bytes = ptr + 1;

        ঘূৰাই m3Err_none;
    } অন্যথা {
        ঘূৰাই m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুৱ u8* ptr = *io_bytes;

    যদি (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_মান  = opcode;
        *io_bytes = ptr;

        ঘূৰাই m3Err_none;
    } অন্যথা {
        ঘূৰাই m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_মান, u32 i_max_অঙ্ক_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 মান = 0;

    u32       shift = 0;
    ধ্রুৱ u8* ptr   = *io_bytes;

    যেতিয়ালৈ (ptr < i_end) {
        u64 byte = *(ptr++);

        মান |= ((byte & 0x7f) << shift);
        shift += 7;

        যদি (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            যদি (M3_UNLIKELY(shift > i_max_অঙ্ক_Bits)) {
                u32 অঙ্ক_Used_Bits = i_max_অঙ্ক_Bits + 7 - shift;

                যদি (byte >> অঙ্ক_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            ভঙ্গ;
        }

        যদি (M3_UNLIKELY(shift >= i_max_অঙ্ক_Bits)) {
            result = m3Err_lebOverflow;
            ভঙ্গ;
        }
    }

    *o_মান  = মান;
    *io_bytes = ptr;

    ঘূৰাই result;
}


M3Result ReadLebSigned (i64* o_মান, u32 i_max_অঙ্ক_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 মান = 0;

    u32       shift = 0;
    ধ্রুৱ u8* ptr   = *io_bytes;

    যেতিয়ালৈ (ptr < i_end) {
        u64 byte = *(ptr++);

        মান |= ((byte & 0x7f) << shift);
        shift += 7;

        যদি (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            যদি (M3_UNLIKELY(shift > i_max_অঙ্ক_Bits)) {
                u32 অঙ্ক_Used_Bits = i_max_অঙ্ক_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (অঙ্ক_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                যদি (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            যদি ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                মান |= (~extend << shift);
            }

            ভঙ্গ;
        }

        যদি (M3_UNLIKELY(shift >= i_max_অঙ্ক_Bits)) {
            result = m3Err_lebOverflow;
            ভঙ্গ;
        }
    }

    *o_মান  = মান;
    *io_bytes = ptr;

    ঘূৰাই result;
}


M3Result ReadLEB_u32 (u32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      মান;
    M3Result result = ReadLebUnsigned(&মান, 32, io_bytes, i_end);

    *o_মান = (u32)মান;

    ঘূৰাই result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    যদি (result) {
        ঘূৰাই result;
    }

    *o_memoryIdx = 0;

#যদি d_m3HasMultiMemory
    যদি (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        যদি (result) {
            ঘূৰাই result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    ঘূৰাই ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      মান;
    M3Result result = ReadLebUnsigned(&মান, 7, io_bytes, i_end);

    *o_মান = (u8)মান;

    ঘূৰাই result;
}


M3Result ReadLEB_i7 (i8* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      মান;
    M3Result result = ReadLebSigned(&মান, 7, io_bytes, i_end);

    *o_মান = (i8)মান;

    ঘূৰাই result;
}


M3Result ReadLEB_i32 (i32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      মান;
    M3Result result = ReadLebSigned(&মান, 32, io_bytes, i_end);

    *o_মান = (i32)মান;

    ঘূৰাই result;
}


M3Result ReadLEB_i64 (i64* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      মান;
    M3Result result = ReadLebSigned(&মান, 64, io_bytes, i_end);

    *o_মান = মান;

    ঘূৰাই result;
}

#যদি d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
স্থির
bool IsValidUtf8 (ধ্রুৱ u8* i_data, u32 i_length)
{
    ধ্রুৱ u8* ptr = i_data;
    ধ্রুৱ u8* end = i_data + i_length;

    যেতিয়ালৈ (ptr < end) {
        u8 b0 = *ptr++;

        যদি (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            অনুৱর্ত;
        } অন্যথা যদি ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            যদি (b0 < 0xC2) {
                ঘূৰাই false;       // overlong
            }
            যদি (ptr >= end) {
                ঘূৰাই false;
            }
            u8 b1 = *ptr++;
            যদি ((b1 & 0xC0) != 0x80) {
                ঘূৰাই false;
            }
        } অন্যথা যদি ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            যদি (ptr + 1 >= end) {
                ঘূৰাই false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            যদি ((b1 & 0xC0) != 0x80) {
                ঘূৰাই false;
            }
            যদি ((b2 & 0xC0) != 0x80) {
                ঘূৰাই false;
            }
            // reject overlong
            যদি (b0 == 0xE0 && b1 < 0xA0) {
                ঘূৰাই false;
            }
            // reject surrogates U+D800..U+DFFF
            যদি (b0 == 0xED && b1 >= 0xA0) {
                ঘূৰাই false;
            }
        } অন্যথা যদি ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            যদি (b0 > 0xF4) {
                ঘূৰাই false;       // above U+10FFFF
            }
            যদি (ptr + 2 >= end) {
                ঘূৰাই false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            যদি ((b1 & 0xC0) != 0x80) {
                ঘূৰাই false;
            }
            যদি ((b2 & 0xC0) != 0x80) {
                ঘূৰাই false;
            }
            যদি ((b3 & 0xC0) != 0x80) {
                ঘূৰাই false;
            }
            // reject overlong
            যদি (b0 == 0xF0 && b1 < 0x90) {
                ঘূৰাই false;
            }
            // reject above U+10FFFF
            যদি (b0 == 0xF4 && b1 > 0x8F) {
                ঘূৰাই false;
            }
        } অন্যথা {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            ঘূৰাই false;
        }
    }

    ঘূৰাই true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = রিক্ত;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    যদি (not result) {
        যদি (utf8Length <= d_m3MaxSaneUtf8Length) {
            ধ্রুৱ u8* ptr = *io_bytes;
            যদি (utf8Length <= (size_t)(i_end - ptr)) {
                ধ্রুৱ u8* end = ptr + utf8Length;
#যদি d_m3EnableValidation
                যদি (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    ঘূৰাই m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                ৱর্ণ* utf8 = (ৱর্ণ*)m3_Malloc("UTF8", utf8Length + 1);

                যদি (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } অন্যথা {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } অন্যথা {
                result = m3Err_wasmUnderrun;
            }
        } অন্যথা {
            result = m3Err_missingUTF8;
        }
    }

    ঘূৰাই result;
}

#যদি d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    যেতিয়ালৈ (curr) {
        যদি (ContainsPC(curr, i_pc)) {
            pageFound = true;
            ভঙ্গ;
        }
        curr = curr->info.next;
    }

    যদি (!pageFound) {
        curr = i_runtime->pagesFull;
        যেতিয়ালৈ (curr) {
            যদি (ContainsPC(curr, i_pc)) {
                pageFound = true;
                ভঙ্গ;
            }
            curr = curr->info.next;
        }
    }

    যদি (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (শূন্য)pcFound;                                                          d_m3Assert (pcFound);

        ঘূৰাই result;
    } অন্যথা {
        ঘূৰাই 0;
    }
}


শূন্য ঠেলো_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    যদি (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ঘূৰাই;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    যদি (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        ঘূৰাই;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    যদি (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } অন্যথা {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


শূন্য FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    যদি (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ঘূৰাই;
    }

    যদি (!io_runtime->backtrace.lastFrame) {
        ঘূৰাই;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


শূন্য ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    যেতিয়ালৈ (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = রিক্ত;
    io_runtime->backtrace.lastFrame = রিক্ত;
}
#endif // d_m3RecordBacktraces
