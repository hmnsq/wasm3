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
#endif

فراغ m3_Abort (ثابت حرف* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    أرجع m3Err_none;
}

#إذا d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

ساكن uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    إذا (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    بنية timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    أرجع us - initial_ts;
}

#endif

#إذا d_m3FixedHeap

ساكن u8        fixedHeap[d_m3FixedHeap];
ساكن u8*       fixedHeapPtr  = fixedHeap;
ساكن u8* ثابت fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
ساكن u8*       fixedHeapLast = عدم;

#  إذا d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  وإلا
#    define HEAP_ALIGN_PTR(P)
#  endif

فراغ* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    إذا (fixedHeapPtr >= fixedHeapEnd) {
        أرجع عدم;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    أرجع ptr;
}

فراغ m3_Free_Impl (فراغ* i_ptr)
{
    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = عدم;
    } وإلا {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    إذا (M3_UNLIKELY(i_newSize == i_oldSize)) {
        أرجع i_ptr;
    }

    فراغ* newPtr;

    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        إذا (fixedHeapPtr >= fixedHeapEnd) {
            أرجع عدم;
        }
        newPtr = i_ptr;
    } وإلا {
        newPtr = m3_Malloc_Impl(i_newSize);
        إذا (!newPtr) {
            أرجع عدم;
        }
        إذا (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    إذا (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    أرجع newPtr;
}

#وإلا

فراغ* m3_Malloc_Impl (size_t i_size)
{
    أرجع calloc(i_size, 1);
}

فراغ m3_Free_Impl (فراغ* io_ptr)
{
    حرر(io_ptr);
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    إذا (M3_UNLIKELY(i_newSize == i_oldSize)) {
        أرجع i_ptr;
    }

    فراغ* newPtr = realloc(i_ptr, i_newSize);

    إذا (M3_LIKELY(newPtr)) {
        إذا (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        أرجع newPtr;
    }
    أرجع عدم;
}

#endif

فراغ* m3_CopyMem (ثابت فراغ* i_from, size_t i_size)
{
    فراغ* ptr = m3_Malloc("CopyMem", i_size);
    إذا (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    أرجع ptr;
}

//--------------------------------------------------------------------------------------------

#إذا d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ساكن u8*    g_guardArena;
ساكن size_t g_guardArenaBytes;
ساكن size_t g_guardSlotBytes;
ساكن size_t g_guardPageSize;
ساكن u32    g_guard_Slot_كدي;
ساكن bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

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

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    لكل (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        فراغ* base = m3_HostReserve(slotBytes * slots);

        إذا (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_كدي  = slots;
            أرجع true;
        }
    }

    أرجع false;
}

فراغ* Guard_TakeSlot (فراغ)
{
    إذا (not Guard_Reserve()) {
        أرجع عدم;
    }

    لكل (u32 i = 0; i < g_guard_Slot_كدي; ++i) {
        إذا (g_guardSlotTaken[i]) {
            استمر;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        إذا (not m3_HostCommit(slot, g_guardPageSize)) {
            أرجع عدم;
        }

        g_guardSlotTaken[i] = true;
        أرجع slot;
    }

    أرجع عدم;
}

M3MemoryHeader* Guard_SlotHeader (فراغ* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    أرجع (M3MemoryHeader*)(data - حجم(M3MemoryHeader));
}

bool Guard_CommitSlot (فراغ* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    أرجع m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

فراغ Guard_GiveSlot (فراغ* i_slot)
{
    إذا (i_slot == عدم) {
        أرجع;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    إذا (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_كدي) {
        g_guardSlotTaken[index] = false;
    }
}

فراغ Guard_ArenaRange (فراغ** o_كس, size_t* o_bytes)
{
    *o_كس   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#إذا d_m3_Max_Native_تر > 0

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
فراغ* m3_Native_تر_Limit (فراغ* i_تر_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_تر_Ptr;

#  إذا M3_HAS_THREAD_LOCAL
    ساكن M3_THREAD_LOCAL u8* base = عدم;
    إذا (base == عدم) {
        base = (u8*)m3_Host_تر_Base();
    }
#  وإلا
    u8* base = (u8*)m3_Host_تر_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    إذا (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_تر_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    أرجع sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#إذا d_m3_Log_Native_تر

ساكن size_t تر_start;
ساكن size_t تر_end;

فراغ m3_تر_Check_Init ()
{
    حرف تر;
    تر_end = تر_start = (size_t)&تر;
}

فراغ m3_تر_Check ()
{
    حرف   تر;
    size_t addr = (size_t)&تر;

    size_t تر_End = تر_end;

    تر_end = M3_MIN(تر_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

صحيح m3_تر_Get_Max ()
{
    أرجع تر_start - تر_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

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
#endif
#إذا d_m3_Has_تر_Switching
    وإلا إذا (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    وإلا إذا (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    أرجع result;
}


#إذا d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    إذا (IsSpelledRefType(i_type)) {
        إذا (i_type & d_m3Type_refCont) {
            أرجع c_m3Type_contref;
        }
        أرجع (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    أرجع (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    إذا (i_sub == i_super) {
        أرجع true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    إذا (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        أرجع false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    إذا (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        أرجع false;
    }

#  إذا d_m3_Has_تر_Switching
    // nocont sits under every continuation type
    إذا (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        أرجع true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    أرجع (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    أرجع (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    أرجع (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#إذا d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#إذا d_m3_Has_تر_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    أرجع (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    إذا (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        أرجع true;
    } وإلا إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        أرجع false;
    } وإلا {
        أرجع (حجم(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        أرجع حجم(i32);
    }

    إذا (IsRefType(i_m3Type)) {
        أرجع حجم(فراغ*);
    }

    أرجع حجم(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    ثابت u8* ptr = *io_bytes;
    ptr += حجم(u64);

    إذا (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_درج, *io_bytes, حجم(u64));
        M3_BSWAP_u64(*o_درج);
        *io_bytes = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    ثابت u8* ptr = *io_bytes;
    ptr += حجم(u32);

    إذا (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_درج, *io_bytes, حجم(u32));
        M3_BSWAP_u32(*o_درج);
        *io_bytes = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#إذا d_m3ImplementFloat

M3Result Read_f64 (f64* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    ثابت u8* ptr = *io_bytes;
    ptr += حجم(f64);

    إذا (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_درج, *io_bytes, حجم(f64));
        M3_BSWAP_f64(*o_درج);
        *io_bytes = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    ثابت u8* ptr = *io_bytes;
    ptr += حجم(f32);

    إذا (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_درج, *io_bytes, حجم(f32));
        M3_BSWAP_f32(*o_درج);
        *io_bytes = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    ثابت u8* ptr = *io_bytes;

    إذا (M3_LIKELY(ptr < i_end)) {
        *o_درج  = *ptr;
        *io_bytes = ptr + 1;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    ثابت u8* ptr = *io_bytes;

    إذا (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_درج  = opcode;
        *io_bytes = ptr;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_درج, u32 i_max_لمب_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 درج = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_bytes;

    طالما (ptr < i_end) {
        u64 byte = *(ptr++);

        درج |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            إذا (M3_UNLIKELY(shift > i_max_لمب_Bits)) {
                u32 لمب_Used_Bits = i_max_لمب_Bits + 7 - shift;

                إذا (byte >> لمب_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_لمب_Bits)) {
            result = m3Err_lebOverflow;
            اكسر;
        }
    }

    *o_درج  = درج;
    *io_bytes = ptr;

    أرجع result;
}


M3Result ReadLebSigned (i64* o_درج, u32 i_max_لمب_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 درج = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_bytes;

    طالما (ptr < i_end) {
        u64 byte = *(ptr++);

        درج |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            إذا (M3_UNLIKELY(shift > i_max_لمب_Bits)) {
                u32 لمب_Used_Bits = i_max_لمب_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (لمب_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                إذا (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            إذا ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                درج |= (~extend << shift);
            }

            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_لمب_Bits)) {
            result = m3Err_lebOverflow;
            اكسر;
        }
    }

    *o_درج  = درج;
    *io_bytes = ptr;

    أرجع result;
}


M3Result ReadLEB_u32 (u32* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      درج;
    M3Result result = ReadLebUnsigned(&درج, 32, io_bytes, i_end);

    *o_درج = (u32)درج;

    أرجع result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    إذا (result) {
        أرجع result;
    }

    *o_memoryIdx = 0;

#إذا d_m3HasMultiMemory
    إذا (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        إذا (result) {
            أرجع result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    أرجع ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      درج;
    M3Result result = ReadLebUnsigned(&درج, 7, io_bytes, i_end);

    *o_درج = (u8)درج;

    أرجع result;
}


M3Result ReadLEB_i7 (i8* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      درج;
    M3Result result = ReadLebSigned(&درج, 7, io_bytes, i_end);

    *o_درج = (i8)درج;

    أرجع result;
}


M3Result ReadLEB_i32 (i32* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      درج;
    M3Result result = ReadLebSigned(&درج, 32, io_bytes, i_end);

    *o_درج = (i32)درج;

    أرجع result;
}


M3Result ReadLEB_i64 (i64* o_درج, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      درج;
    M3Result result = ReadLebSigned(&درج, 64, io_bytes, i_end);

    *o_درج = درج;

    أرجع result;
}

#إذا d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ساكن
bool IsValidUtf8 (ثابت u8* i_data, u32 i_length)
{
    ثابت u8* ptr = i_data;
    ثابت u8* end = i_data + i_length;

    طالما (ptr < end) {
        u8 b0 = *ptr++;

        إذا (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            استمر;
        } وإلا إذا ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            إذا (b0 < 0xC2) {
                أرجع false;       // overlong
            }
            إذا (ptr >= end) {
                أرجع false;
            }
            u8 b1 = *ptr++;
            إذا ((b1 & 0xC0) != 0x80) {
                أرجع false;
            }
        } وإلا إذا ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            إذا (ptr + 1 >= end) {
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
            إذا (ptr + 2 >= end) {
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
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = عدم;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    إذا (not result) {
        إذا (utf8Length <= d_m3MaxSaneUtf8Length) {
            ثابت u8* ptr = *io_bytes;
            إذا (utf8Length <= (size_t)(i_end - ptr)) {
                ثابت u8* end = ptr + utf8Length;
#إذا d_m3EnableValidation
                إذا (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    أرجع m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                حرف* utf8 = (حرف*)m3_Malloc("UTF8", utf8Length + 1);

                إذا (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } وإلا {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } وإلا {
                result = m3Err_wasmUnderrun;
            }
        } وإلا {
            result = m3Err_missingUTF8;
        }
    }

    أرجع result;
}

#إذا d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    طالما (curr) {
        إذا (ContainsPC(curr, i_pc)) {
            pageFound = true;
            اكسر;
        }
        curr = curr->info.next;
    }

    إذا (!pageFound) {
        curr = i_runtime->pagesFull;
        طالما (curr) {
            إذا (ContainsPC(curr, i_pc)) {
                pageFound = true;
                اكسر;
            }
            curr = curr->info.next;
        }
    }

    إذا (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (فراغ)pcFound;                                                          d_m3Assert (pcFound);

        أرجع result;
    } وإلا {
        أرجع 0;
    }
}


فراغ تور_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    إذا (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    إذا (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        أرجع;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    إذا (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } وإلا {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


فراغ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    إذا (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    إذا (!io_runtime->backtrace.lastFrame) {
        أرجع;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


فراغ ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    طالما (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = عدم;
    io_runtime->backtrace.lastFrame = عدم;
}
#endif // d_m3RecordBacktraces
