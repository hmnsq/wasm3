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
#अन्तर्भाव "m3_config.h"
#अन्तर्भाव "wasm3.h"

#अन्तर्भाव "m3_core.h"
#अन्तर्भाव "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#अन्तर्भाव "m3_host.h"

#सचे d_m3HasWin32Host
#  include "m3_host_win32.h"
#नोचेद्यदि d_m3HasPosixHost
#  include "m3_host_posix.h"
#अञ्ञथा
#  include "m3_host_none.h"
#endif

शून्य m3_Abort (ध्रुव वर्ण* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    पटिनिवत्त m3Err_none;
}

#सचे d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

स्थिर uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    सचे (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    संरचना timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    पटिनिवत्त us - initial_ts;
}

#endif

#सचे d_m3FixedHeap

स्थिर u8        fixedHeap[d_m3FixedHeap];
स्थिर u8*       fixedHeapPtr  = fixedHeap;
स्थिर u8* ध्रुव fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
स्थिर u8*       fixedHeapLast = रिक्त;

#  सचे d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  अञ्ञथा
#    define HEAP_ALIGN_PTR(P)
#  endif

शून्य* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    सचे (fixedHeapPtr >= fixedHeapEnd) {
        पटिनिवत्त रिक्त;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    पटिनिवत्त ptr;
}

शून्य m3_Free_Impl (शून्य* i_ptr)
{
    // Handle the last chunk
    सचे (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = रिक्त;
    } अञ्ञथा {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

शून्य* m3_Realloc_Impl (शून्य* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    सचे (M3_UNLIKELY(i_newSize == i_oldSize)) {
        पटिनिवत्त i_ptr;
    }

    शून्य* newPtr;

    // Handle the last chunk
    सचे (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        सचे (fixedHeapPtr >= fixedHeapEnd) {
            पटिनिवत्त रिक्त;
        }
        newPtr = i_ptr;
    } अञ्ञथा {
        newPtr = m3_Malloc_Impl(i_newSize);
        सचे (!newPtr) {
            पटिनिवत्त रिक्त;
        }
        सचे (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    सचे (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    पटिनिवत्त newPtr;
}

#अञ्ञथा

शून्य* m3_Malloc_Impl (size_t i_size)
{
    पटिनिवत्त calloc(i_size, 1);
}

शून्य m3_Free_Impl (शून्य* io_ptr)
{
    मुक्त(io_ptr);
}

शून्य* m3_Realloc_Impl (शून्य* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    सचे (M3_UNLIKELY(i_newSize == i_oldSize)) {
        पटिनिवत्त i_ptr;
    }

    शून्य* newPtr = realloc(i_ptr, i_newSize);

    सचे (M3_LIKELY(newPtr)) {
        सचे (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        पटिनिवत्त newPtr;
    }
    पटिनिवत्त रिक्त;
}

#endif

शून्य* m3_CopyMem (ध्रुव शून्य* i_from, size_t i_size)
{
    शून्य* ptr = m3_Malloc("CopyMem", i_size);
    सचे (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    पटिनिवत्त ptr;
}

//--------------------------------------------------------------------------------------------

#सचे d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
स्थिर u8*    g_guardArena;
स्थिर size_t g_guardArenaBytes;
स्थिर size_t g_guardSlotBytes;
स्थिर size_t g_guardPageSize;
स्थिर u32    g_guard_Slot_गणनम्;
स्थिर bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
स्थिर
bool Guard_Reserve (शून्य)
{
    सचे (g_guardArena) {
        पटिनिवत्त true;
    }

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    अत्थाय (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        शून्य* base = m3_HostReserve(slotBytes * slots);

        सचे (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_गणनम्  = slots;
            पटिनिवत्त true;
        }
    }

    पटिनिवत्त false;
}

शून्य* Guard_TakeSlot (शून्य)
{
    सचे (not Guard_Reserve()) {
        पटिनिवत्त रिक्त;
    }

    अत्थाय (u32 i = 0; i < g_guard_Slot_गणनम्; ++i) {
        सचे (g_guardSlotTaken[i]) {
            अनुवर्त;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        सचे (not m3_HostCommit(slot, g_guardPageSize)) {
            पटिनिवत्त रिक्त;
        }

        g_guardSlotTaken[i] = true;
        पटिनिवत्त slot;
    }

    पटिनिवत्त रिक्त;
}

M3MemoryHeader* Guard_SlotHeader (शून्य* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    पटिनिवत्त (M3MemoryHeader*)(data - परिमाण(M3MemoryHeader));
}

bool Guard_CommitSlot (शून्य* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    पटिनिवत्त m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

शून्य Guard_GiveSlot (शून्य* i_slot)
{
    सचे (i_slot == रिक्त) {
        पटिनिवत्त;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    सचे (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_गणनम्) {
        g_guardSlotTaken[index] = false;
    }
}

शून्य Guard_ArenaRange (शून्य** o_नीचः, size_t* o_bytes)
{
    *o_नीचः   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#सचे d_m3_Max_Native_राशिः > 0

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
शून्य* m3_Native_राशिः_Limit (शून्य* i_राशिः_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_राशिः_Ptr;

#  सचे M3_HAS_THREAD_LOCAL
    स्थिर M3_THREAD_LOCAL u8* base = रिक्त;
    सचे (base == रिक्त) {
        base = (u8*)m3_Host_राशिः_Base();
    }
#  अञ्ञथा
    u8* base = (u8*)m3_Host_राशिः_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    सचे (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_राशिः_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    पटिनिवत्त sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#सचे d_m3_Log_Native_राशिः

स्थिर size_t राशिः_start;
स्थिर size_t राशिः_end;

शून्य m3_राशिः_Check_Init ()
{
    वर्ण राशिः;
    राशिः_end = राशिः_start = (size_t)&राशिः;
}

शून्य m3_राशिः_Check ()
{
    वर्ण   राशिः;
    size_t addr = (size_t)&राशिः;

    size_t राशिः_End = राशिः_end;

    राशिः_end = M3_MIN(राशिः_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

पूर्णाङ्क m3_राशिः_Get_Max ()
{
    पटिनिवत्त राशिः_start - राशिः_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    सचे (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    अञ्ञथा सचे (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } अञ्ञथा सचे (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#सचे d_m3HasExceptionHandling
    अञ्ञथा सचे (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#सचे d_m3_Has_राशिः_Switching
    अञ्ञथा सचे (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    अञ्ञथा सचे (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    पटिनिवत्त result;
}


#सचे d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    सचे (IsSpelledRefType(i_type)) {
        सचे (i_type & d_m3Type_refCont) {
            पटिनिवत्त c_m3Type_contref;
        }
        पटिनिवत्त (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    पटिनिवत्त (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    सचे (i_sub == i_super) {
        पटिनिवत्त true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    सचे (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        पटिनिवत्त false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    सचे (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        पटिनिवत्त false;
    }

#  सचे d_m3_Has_राशिः_Switching
    // nocont sits under every continuation type
    सचे (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        पटिनिवत्त true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    पटिनिवत्त (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    पटिनिवत्त (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    पटिनिवत्त (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#सचे d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#सचे d_m3_Has_राशिः_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    पटिनिवत्त (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    सचे (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        पटिनिवत्त true;
    } अञ्ञथा सचे (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        पटिनिवत्त false;
    } अञ्ञथा {
        पटिनिवत्त (परिमाण(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    सचे (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        पटिनिवत्त परिमाण(i32);
    }

    सचे (IsRefType(i_m3Type)) {
        पटिनिवत्त परिमाण(शून्य*);
    }

    पटिनिवत्त परिमाण(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(u64);

    सचे (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_मानम्, *io_bytes, परिमाण(u64));
        M3_BSWAP_u64(*o_मानम्);
        *io_bytes = ptr;
        पटिनिवत्त m3Err_none;
    } अञ्ञथा {
        पटिनिवत्त m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(u32);

    सचे (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_मानम्, *io_bytes, परिमाण(u32));
        M3_BSWAP_u32(*o_मानम्);
        *io_bytes = ptr;
        पटिनिवत्त m3Err_none;
    } अञ्ञथा {
        पटिनिवत्त m3Err_wasmUnderrun;
    }
}

#सचे d_m3ImplementFloat

M3Result Read_f64 (f64* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(f64);

    सचे (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_मानम्, *io_bytes, परिमाण(f64));
        M3_BSWAP_f64(*o_मानम्);
        *io_bytes = ptr;
        पटिनिवत्त m3Err_none;
    } अञ्ञथा {
        पटिनिवत्त m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(f32);

    सचे (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_मानम्, *io_bytes, परिमाण(f32));
        M3_BSWAP_f32(*o_मानम्);
        *io_bytes = ptr;
        पटिनिवत्त m3Err_none;
    } अञ्ञथा {
        पटिनिवत्त m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;

    सचे (M3_LIKELY(ptr < i_end)) {
        *o_मानम्  = *ptr;
        *io_bytes = ptr + 1;

        पटिनिवत्त m3Err_none;
    } अञ्ञथा {
        पटिनिवत्त m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;

    सचे (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_मानम्  = opcode;
        *io_bytes = ptr;

        पटिनिवत्त m3Err_none;
    } अञ्ञथा {
        पटिनिवत्त m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_मानम्, u32 i_max_अङ्कः_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 मानम् = 0;

    u32       shift = 0;
    ध्रुव u8* ptr   = *io_bytes;

    याव (ptr < i_end) {
        u64 byte = *(ptr++);

        मानम् |= ((byte & 0x7f) << shift);
        shift += 7;

        सचे (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            सचे (M3_UNLIKELY(shift > i_max_अङ्कः_Bits)) {
                u32 अङ्कः_Used_Bits = i_max_अङ्कः_Bits + 7 - shift;

                सचे (byte >> अङ्कः_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            भङ्ग;
        }

        सचे (M3_UNLIKELY(shift >= i_max_अङ्कः_Bits)) {
            result = m3Err_lebOverflow;
            भङ्ग;
        }
    }

    *o_मानम्  = मानम्;
    *io_bytes = ptr;

    पटिनिवत्त result;
}


M3Result ReadLebSigned (i64* o_मानम्, u32 i_max_अङ्कः_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 मानम् = 0;

    u32       shift = 0;
    ध्रुव u8* ptr   = *io_bytes;

    याव (ptr < i_end) {
        u64 byte = *(ptr++);

        मानम् |= ((byte & 0x7f) << shift);
        shift += 7;

        सचे (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            सचे (M3_UNLIKELY(shift > i_max_अङ्कः_Bits)) {
                u32 अङ्कः_Used_Bits = i_max_अङ्कः_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (अङ्कः_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                सचे (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            सचे ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                मानम् |= (~extend << shift);
            }

            भङ्ग;
        }

        सचे (M3_UNLIKELY(shift >= i_max_अङ्कः_Bits)) {
            result = m3Err_lebOverflow;
            भङ्ग;
        }
    }

    *o_मानम्  = मानम्;
    *io_bytes = ptr;

    पटिनिवत्त result;
}


M3Result ReadLEB_u32 (u32* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      मानम्;
    M3Result result = ReadLebUnsigned(&मानम्, 32, io_bytes, i_end);

    *o_मानम् = (u32)मानम्;

    पटिनिवत्त result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    सचे (result) {
        पटिनिवत्त result;
    }

    *o_memoryIdx = 0;

#सचे d_m3HasMultiMemory
    सचे (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        सचे (result) {
            पटिनिवत्त result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    पटिनिवत्त ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      मानम्;
    M3Result result = ReadLebUnsigned(&मानम्, 7, io_bytes, i_end);

    *o_मानम् = (u8)मानम्;

    पटिनिवत्त result;
}


M3Result ReadLEB_i7 (i8* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मानम्;
    M3Result result = ReadLebSigned(&मानम्, 7, io_bytes, i_end);

    *o_मानम् = (i8)मानम्;

    पटिनिवत्त result;
}


M3Result ReadLEB_i32 (i32* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मानम्;
    M3Result result = ReadLebSigned(&मानम्, 32, io_bytes, i_end);

    *o_मानम् = (i32)मानम्;

    पटिनिवत्त result;
}


M3Result ReadLEB_i64 (i64* o_मानम्, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मानम्;
    M3Result result = ReadLebSigned(&मानम्, 64, io_bytes, i_end);

    *o_मानम् = मानम्;

    पटिनिवत्त result;
}

#सचे d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
स्थिर
bool IsValidUtf8 (ध्रुव u8* i_data, u32 i_length)
{
    ध्रुव u8* ptr = i_data;
    ध्रुव u8* end = i_data + i_length;

    याव (ptr < end) {
        u8 b0 = *ptr++;

        सचे (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            अनुवर्त;
        } अञ्ञथा सचे ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            सचे (b0 < 0xC2) {
                पटिनिवत्त false;       // overlong
            }
            सचे (ptr >= end) {
                पटिनिवत्त false;
            }
            u8 b1 = *ptr++;
            सचे ((b1 & 0xC0) != 0x80) {
                पटिनिवत्त false;
            }
        } अञ्ञथा सचे ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            सचे (ptr + 1 >= end) {
                पटिनिवत्त false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            सचे ((b1 & 0xC0) != 0x80) {
                पटिनिवत्त false;
            }
            सचे ((b2 & 0xC0) != 0x80) {
                पटिनिवत्त false;
            }
            // reject overlong
            सचे (b0 == 0xE0 && b1 < 0xA0) {
                पटिनिवत्त false;
            }
            // reject surrogates U+D800..U+DFFF
            सचे (b0 == 0xED && b1 >= 0xA0) {
                पटिनिवत्त false;
            }
        } अञ्ञथा सचे ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            सचे (b0 > 0xF4) {
                पटिनिवत्त false;       // above U+10FFFF
            }
            सचे (ptr + 2 >= end) {
                पटिनिवत्त false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            सचे ((b1 & 0xC0) != 0x80) {
                पटिनिवत्त false;
            }
            सचे ((b2 & 0xC0) != 0x80) {
                पटिनिवत्त false;
            }
            सचे ((b3 & 0xC0) != 0x80) {
                पटिनिवत्त false;
            }
            // reject overlong
            सचे (b0 == 0xF0 && b1 < 0x90) {
                पटिनिवत्त false;
            }
            // reject above U+10FFFF
            सचे (b0 == 0xF4 && b1 > 0x8F) {
                पटिनिवत्त false;
            }
        } अञ्ञथा {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            पटिनिवत्त false;
        }
    }

    पटिनिवत्त true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = रिक्त;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    सचे (not result) {
        सचे (utf8Length <= d_m3MaxSaneUtf8Length) {
            ध्रुव u8* ptr = *io_bytes;
            सचे (utf8Length <= (size_t)(i_end - ptr)) {
                ध्रुव u8* end = ptr + utf8Length;
#सचे d_m3EnableValidation
                सचे (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    पटिनिवत्त m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                वर्ण* utf8 = (वर्ण*)m3_Malloc("UTF8", utf8Length + 1);

                सचे (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } अञ्ञथा {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } अञ्ञथा {
                result = m3Err_wasmUnderrun;
            }
        } अञ्ञथा {
            result = m3Err_missingUTF8;
        }
    }

    पटिनिवत्त result;
}

#सचे d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    याव (curr) {
        सचे (ContainsPC(curr, i_pc)) {
            pageFound = true;
            भङ्ग;
        }
        curr = curr->info.next;
    }

    सचे (!pageFound) {
        curr = i_runtime->pagesFull;
        याव (curr) {
            सचे (ContainsPC(curr, i_pc)) {
                pageFound = true;
                भङ्ग;
            }
            curr = curr->info.next;
        }
    }

    सचे (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (शून्य)pcFound;                                                          d_m3Assert (pcFound);

        पटिनिवत्त result;
    } अञ्ञथा {
        पटिनिवत्त 0;
    }
}


शून्य प्रक्षिप_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    सचे (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        पटिनिवत्त;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    सचे (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        पटिनिवत्त;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    सचे (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } अञ्ञथा {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


शून्य FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    सचे (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        पटिनिवत्त;
    }

    सचे (!io_runtime->backtrace.lastFrame) {
        पटिनिवत्त;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


शून्य ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    याव (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = रिक्त;
    io_runtime->backtrace.lastFrame = रिक्त;
}
#endif // d_m3RecordBacktraces
