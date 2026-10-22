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
#অন্তর্ভাব "m3_config.h"
#অন্তর্ভাব "wasm3.h"

#অন্তর্ভাব "m3_core.h"
#অন্তর্ভাব "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#অন্তর্ভাব "m3_host.h"

#যদি d_m3HasWin32Host
#  include "m3_host_win32.h"
#নতুবা_যদি d_m3HasPosixHost
#  include "m3_host_posix.h"
#অন্যথা
#  include "m3_host_none.h"
#endif

শূন্য m3_Abort (ধ্রুবক অক্ষর* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3_ফলাফল m3_Yield ()
{
    ফেরত m3Err_none;
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
    কাঠামো timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    ফেরত us - initial_ts;
}

#endif

#যদি d_m3FixedHeap

স্থির u8        fixedHeap[d_m3FixedHeap];
স্থির u8*       fixedHeapPtr  = fixedHeap;
স্থির u8* ধ্রুবক fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
স্থির u8*       fixedHeapLast = ফাঁকা;

#  যদি d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  অন্যথা
#    define HEAP_ALIGN_PTR(P)
#  endif

শূন্য* m3_Malloc_Impl (size_t i_আকার)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_আকার;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    যদি (fixedHeapPtr >= fixedHeapEnd) {
        ফেরত ফাঁকা;
    }

    memset(ptr, 0x0, i_আকার);
    fixedHeapLast = ptr;

    ফেরত ptr;
}

শূন্য m3_Free_Impl (শূন্য* i_ptr)
{
    // Handle the last chunk
    যদি (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = ফাঁকা;
    } অন্যথা {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

শূন্য* m3_Realloc_Impl (শূন্য* i_ptr, size_t i_new_আকার, size_t i_old_আকার)
{
    যদি (M3_UNLIKELY(i_new_আকার == i_old_আকার)) {
        ফেরত i_ptr;
    }

    শূন্য* newPtr;

    // Handle the last chunk
    যদি (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_new_আকার;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        যদি (fixedHeapPtr >= fixedHeapEnd) {
            ফেরত ফাঁকা;
        }
        newPtr = i_ptr;
    } অন্যথা {
        newPtr = m3_Malloc_Impl(i_new_আকার);
        যদি (!newPtr) {
            ফেরত ফাঁকা;
        }
        যদি (i_ptr) {
            memcpy(newPtr, i_ptr, i_old_আকার);
        }
    }

    যদি (i_new_আকার > i_old_আকার) {
        memset((u8*)newPtr + i_old_আকার, 0x0, i_new_আকার - i_old_আকার);
    }

    ফেরত newPtr;
}

#অন্যথা

শূন্য* m3_Malloc_Impl (size_t i_আকার)
{
    ফেরত calloc(i_আকার, 1);
}

শূন্য m3_Free_Impl (শূন্য* io_ptr)
{
    মুক্ত(io_ptr);
}

শূন্য* m3_Realloc_Impl (শূন্য* i_ptr, size_t i_new_আকার, size_t i_old_আকার)
{
    যদি (M3_UNLIKELY(i_new_আকার == i_old_আকার)) {
        ফেরত i_ptr;
    }

    শূন্য* newPtr = realloc(i_ptr, i_new_আকার);

    যদি (M3_LIKELY(newPtr)) {
        যদি (i_new_আকার > i_old_আকার) {
            memset((u8*)newPtr + i_old_আকার, 0x0, i_new_আকার - i_old_আকার);
        }
        ফেরত newPtr;
    }
    ফেরত ফাঁকা;
}

#endif

শূন্য* m3_CopyMem (ধ্রুবক শূন্য* i_from, size_t i_আকার)
{
    শূন্য* ptr = m3_Malloc("CopyMem", i_আকার);
    যদি (ptr) {
        memcpy(ptr, i_from, i_আকার);
    }
    ফেরত ptr;
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
স্থির size_t g_guard_Page_আকার;
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
        ফেরত true;
    }

    size_t page_আকার = m3_Host_Page_আকার();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * page_আকার + (size_t)d_m3_Guarded_তথ্য_Bytes;

    জন্য (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        শূন্য* base = m3_HostReserve(slotBytes * slots);

        যদি (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guard_Page_আকার   = page_আকার;
            g_guard_Slot_গুনতি  = slots;
            ফেরত true;
        }
    }

    ফেরত false;
}

শূন্য* Guard_TakeSlot (শূন্য)
{
    যদি (not Guard_Reserve()) {
        ফেরত ফাঁকা;
    }

    জন্য (u32 i = 0; i < g_guard_Slot_গুনতি; ++i) {
        যদি (g_guardSlotTaken[i]) {
            চালু;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        যদি (not m3_HostCommit(slot, g_guard_Page_আকার)) {
            ফেরত ফাঁকা;
        }

        g_guardSlotTaken[i] = true;
        ফেরত slot;
    }

    ফেরত ফাঁকা;
}

M3MemoryHeader* Guard_SlotHeader (শূন্য* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* তথ্য = (u8*)i_slot + g_guard_Page_আকার;

    ফেরত (M3MemoryHeader*)(তথ্য - আকার(M3MemoryHeader));
}

bool Guard_CommitSlot (শূন্য* i_slot, size_t i_তথ্য_Bytes)
{
    size_t rounded = (i_তথ্য_Bytes + g_guard_Page_আকার - 1) / g_guard_Page_আকার * g_guard_Page_আকার;

    ফেরত m3_HostCommit((u8*)i_slot + g_guard_Page_আকার, rounded);
}

শূন্য Guard_GiveSlot (শূন্য* i_slot)
{
    যদি (i_slot == ফাঁকা) {
        ফেরত;
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

শূন্য Guard_ArenaRange (শূন্য** o_নিচের, size_t* o_bytes)
{
    *o_নিচের   = g_guardArena;
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
    স্থির M3_THREAD_LOCAL u8* base = ফাঁকা;
    যদি (base == ফাঁকা) {
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

    ফেরত sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#যদি d_m3_Log_Native_স্তূপ

স্থির size_t স্তূপ_start;
স্থির size_t স্তূপ_end;

শূন্য m3_স্তূপ_Check_Init ()
{
    অক্ষর স্তূপ;
    স্তূপ_end = স্তূপ_start = (size_t)&স্তূপ;
}

শূন্য m3_স্তূপ_Check ()
{
    অক্ষর   স্তূপ;
    size_t addr = (size_t)&স্তূপ;

    size_t স্তূপ_End = স্তূপ_end;

    স্তূপ_end = M3_MIN(স্তূপ_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

পূর্ণসংখ্যা m3_স্তূপ_Get_Max ()
{
    ফেরত স্তূপ_start - স্তূপ_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_ফলাফল NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_ফলাফল ফলাফল = m3Err_none;

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
        ফলাফল = m3Err_invalidTypeId;
    }

    *o_type = type;

    ফেরত ফলাফল;
}


#যদি d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    যদি (IsSpelledRefType(i_type)) {
        যদি (i_type & d_m3Type_refCont) {
            ফেরত c_m3Type_contref;
        }
        ফেরত (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    ফেরত (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    যদি (i_sub == i_super) {
        ফেরত true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    যদি (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        ফেরত false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    যদি (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        ফেরত false;
    }

#  যদি d_m3_Has_স্তূপ_Switching
    // nocont sits under every continuation type
    যদি (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        ফেরত true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    ফেরত (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ফেরত (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    ফেরত (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
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
    ফেরত (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    যদি (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        ফেরত true;
    } অন্যথা যদি (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        ফেরত false;
    } অন্যথা {
        ফেরত (আকার(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 আকার_Of_Type (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    যদি (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        ফেরত আকার(i32);
    }

    যদি (IsRefType(i_m3Type)) {
        ফেরত আকার(শূন্য*);
    }

    ফেরত আকার(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_ফলাফল Read_u64 (u64* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুবক u8* ptr = *io_bytes;
    ptr += আকার(u64);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, আকার(u64));
        M3_BSWAP_u64(*o_মান);
        *io_bytes = ptr;
        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3Err_wasmUnderrun;
    }
}


M3_ফলাফল Read_u32 (u32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুবক u8* ptr = *io_bytes;
    ptr += আকার(u32);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, আকার(u32));
        M3_BSWAP_u32(*o_মান);
        *io_bytes = ptr;
        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3Err_wasmUnderrun;
    }
}

#যদি d_m3ImplementFloat

M3_ফলাফল Read_f64 (f64* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুবক u8* ptr = *io_bytes;
    ptr += আকার(f64);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, আকার(f64));
        M3_BSWAP_f64(*o_মান);
        *io_bytes = ptr;
        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3Err_wasmUnderrun;
    }
}


M3_ফলাফল Read_f32 (f32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুবক u8* ptr = *io_bytes;
    ptr += আকার(f32);

    যদি (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_মান, *io_bytes, আকার(f32));
        M3_BSWAP_f32(*o_মান);
        *io_bytes = ptr;
        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3Err_wasmUnderrun;
    }
}

#endif

M3_ফলাফল Read_u8 (u8* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুবক u8* ptr = *io_bytes;

    যদি (M3_LIKELY(ptr < i_end)) {
        *o_মান  = *ptr;
        *io_bytes = ptr + 1;

        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3Err_wasmUnderrun;
    }
}

M3_ফলাফল Read_opcode (m3opcode_t* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    ধ্রুবক u8* ptr = *io_bytes;

    যদি (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_মান  = opcode;
        *io_bytes = ptr;

        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত m3Err_wasmUnderrun;
    }
}


M3_ফলাফল ReadLebUnsigned (u64* o_মান, u32 i_max_অঙ্ক_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_wasmUnderrun;

    u64 মান = 0;

    u32       shift = 0;
    ধ্রুবক u8* ptr   = *io_bytes;

    যখন (ptr < i_end) {
        u64 byte = *(ptr++);

        মান |= ((byte & 0x7f) << shift);
        shift += 7;

        যদি (M3_LIKELY((byte & 0x80) == 0)) {
            ফলাফল = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            যদি (M3_UNLIKELY(shift > i_max_অঙ্ক_Bits)) {
                u32 অঙ্ক_Used_Bits = i_max_অঙ্ক_Bits + 7 - shift;

                যদি (byte >> অঙ্ক_Used_Bits) {
                    ফলাফল = m3Err_lebOverflow;
                }
            }
            বিরতি;
        }

        যদি (M3_UNLIKELY(shift >= i_max_অঙ্ক_Bits)) {
            ফলাফল = m3Err_lebOverflow;
            বিরতি;
        }
    }

    *o_মান  = মান;
    *io_bytes = ptr;

    ফেরত ফলাফল;
}


M3_ফলাফল ReadLebSigned (i64* o_মান, u32 i_max_অঙ্ক_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_wasmUnderrun;

    i64 মান = 0;

    u32       shift = 0;
    ধ্রুবক u8* ptr   = *io_bytes;

    যখন (ptr < i_end) {
        u64 byte = *(ptr++);

        মান |= ((byte & 0x7f) << shift);
        shift += 7;

        যদি (M3_LIKELY((byte & 0x80) == 0)) {
            ফলাফল = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            যদি (M3_UNLIKELY(shift > i_max_অঙ্ক_Bits)) {
                u32 অঙ্ক_Used_Bits = i_max_অঙ্ক_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (অঙ্ক_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                যদি (bits != 0 and bits != signBits) {
                    ফলাফল = m3Err_lebOverflow;
                }
            }
            যদি ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                মান |= (~extend << shift);
            }

            বিরতি;
        }

        যদি (M3_UNLIKELY(shift >= i_max_অঙ্ক_Bits)) {
            ফলাফল = m3Err_lebOverflow;
            বিরতি;
        }
    }

    *o_মান  = মান;
    *io_bytes = ptr;

    ফেরত ফলাফল;
}


M3_ফলাফল ReadLEB_u32 (u32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      মান;
    M3_ফলাফল ফলাফল = ReadLebUnsigned(&মান, 32, io_bytes, i_end);

    *o_মান = (u32)মান;

    ফেরত ফলাফল;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3_ফলাফল ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল;

    u32 align;
    ফলাফল = ReadLEB_u32(&align, io_bytes, i_end);
    যদি (ফলাফল) {
        ফেরত ফলাফল;
    }

    *o_memoryIdx = 0;

#যদি d_m3HasMultiMemory
    যদি (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        ফলাফল = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        যদি (ফলাফল) {
            ফেরত ফলাফল;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    ফেরত ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3_ফলাফল ReadLEB_u7 (u8* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      মান;
    M3_ফলাফল ফলাফল = ReadLebUnsigned(&মান, 7, io_bytes, i_end);

    *o_মান = (u8)মান;

    ফেরত ফলাফল;
}


M3_ফলাফল ReadLEB_i7 (i8* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      মান;
    M3_ফলাফল ফলাফল = ReadLebSigned(&মান, 7, io_bytes, i_end);

    *o_মান = (i8)মান;

    ফেরত ফলাফল;
}


M3_ফলাফল ReadLEB_i32 (i32* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      মান;
    M3_ফলাফল ফলাফল = ReadLebSigned(&মান, 32, io_bytes, i_end);

    *o_মান = (i32)মান;

    ফেরত ফলাফল;
}


M3_ফলাফল ReadLEB_i64 (i64* o_মান, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      মান;
    M3_ফলাফল ফলাফল = ReadLebSigned(&মান, 64, io_bytes, i_end);

    *o_মান = মান;

    ফেরত ফলাফল;
}

#যদি d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
স্থির
bool IsValidUtf8 (ধ্রুবক u8* i_তথ্য, u32 i_দৈর্ঘ্য)
{
    ধ্রুবক u8* ptr = i_তথ্য;
    ধ্রুবক u8* end = i_তথ্য + i_দৈর্ঘ্য;

    যখন (ptr < end) {
        u8 b0 = *ptr++;

        যদি (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            চালু;
        } অন্যথা যদি ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            যদি (b0 < 0xC2) {
                ফেরত false;       // overlong
            }
            যদি (ptr >= end) {
                ফেরত false;
            }
            u8 b1 = *ptr++;
            যদি ((b1 & 0xC0) != 0x80) {
                ফেরত false;
            }
        } অন্যথা যদি ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            যদি (ptr + 1 >= end) {
                ফেরত false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            যদি ((b1 & 0xC0) != 0x80) {
                ফেরত false;
            }
            যদি ((b2 & 0xC0) != 0x80) {
                ফেরত false;
            }
            // reject overlong
            যদি (b0 == 0xE0 && b1 < 0xA0) {
                ফেরত false;
            }
            // reject surrogates U+D800..U+DFFF
            যদি (b0 == 0xED && b1 >= 0xA0) {
                ফেরত false;
            }
        } অন্যথা যদি ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            যদি (b0 > 0xF4) {
                ফেরত false;       // above U+10FFFF
            }
            যদি (ptr + 2 >= end) {
                ফেরত false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            যদি ((b1 & 0xC0) != 0x80) {
                ফেরত false;
            }
            যদি ((b2 & 0xC0) != 0x80) {
                ফেরত false;
            }
            যদি ((b3 & 0xC0) != 0x80) {
                ফেরত false;
            }
            // reject overlong
            যদি (b0 == 0xF0 && b1 < 0x90) {
                ফেরত false;
            }
            // reject above U+10FFFF
            যদি (b0 == 0xF4 && b1 > 0x8F) {
                ফেরত false;
            }
        } অন্যথা {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            ফেরত false;
        }
    }

    ফেরত true;
}
#endif // d_m3EnableValidation


M3_ফলাফল Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = ফাঁকা;

    u32      utf8_দৈর্ঘ্য;
    M3_ফলাফল ফলাফল = ReadLEB_u32(&utf8_দৈর্ঘ্য, io_bytes, i_end);

    যদি (not ফলাফল) {
        যদি (utf8_দৈর্ঘ্য <= d_m3_Max_Sane_Utf8_দৈর্ঘ্য) {
            ধ্রুবক u8* ptr = *io_bytes;
            যদি (utf8_দৈর্ঘ্য <= (size_t)(i_end - ptr)) {
                ধ্রুবক u8* end = ptr + utf8_দৈর্ঘ্য;
#যদি d_m3EnableValidation
                যদি (not IsValidUtf8(ptr, utf8_দৈর্ঘ্য)) {
                    *io_bytes = end;
                    ফেরত m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                অক্ষর* utf8 = (অক্ষর*)m3_Malloc("UTF8", utf8_দৈর্ঘ্য + 1);

                যদি (utf8) {
                    memcpy(utf8, ptr, utf8_দৈর্ঘ্য);
                    utf8[utf8_দৈর্ঘ্য] = 0;

                    *o_utf8 = utf8;
                } অন্যথা {
                    ফলাফল = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } অন্যথা {
                ফলাফল = m3Err_wasmUnderrun;
            }
        } অন্যথা {
            ফলাফল = m3Err_missingUTF8;
        }
    }

    ফেরত ফলাফল;
}

#যদি d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    যখন (curr) {
        যদি (ContainsPC(curr, i_pc)) {
            pageFound = true;
            বিরতি;
        }
        curr = curr->info.পরবর্তী;
    }

    যদি (!pageFound) {
        curr = i_runtime->pagesFull;
        যখন (curr) {
            যদি (ContainsPC(curr, i_pc)) {
                pageFound = true;
                বিরতি;
            }
            curr = curr->info.পরবর্তী;
        }
    }

    যদি (pageFound) {
        u32 ফলাফল = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &ফলাফল);
        (শূন্য)pcFound;                                                          d_m3Assert (pcFound);

        ফেরত ফলাফল;
    } অন্যথা {
        ফেরত 0;
    }
}


শূন্য ঠেলো_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    যদি (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ফেরত;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    যদি (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        ফেরত;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    যদি (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } অন্যথা {
        io_runtime->backtrace.lastFrame->পরবর্তী = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


শূন্য FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    যদি (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        ফেরত;
    }

    যদি (!io_runtime->backtrace.lastFrame) {
        ফেরত;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


শূন্য ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    যখন (currentFrame) {
        M3BacktraceFrame* পরবর্তী_Frame = currentFrame->পরবর্তী;
        m3_Free(currentFrame);
        currentFrame = পরবর্তী_Frame;
    }

    io_runtime->backtrace.frames    = ফাঁকা;
    io_runtime->backtrace.lastFrame = ফাঁকা;
}
#endif // d_m3RecordBacktraces
