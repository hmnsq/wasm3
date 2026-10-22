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

#यदि d_m3HasWin32Host
#  include "m3_host_win32.h"
#नोचेद्यदि d_m3HasPosixHost
#  include "m3_host_posix.h"
#अन्यथा
#  include "m3_host_none.h"
#endif

शून्य m3_Abort (ध्रुव वर्ण* message)
{
#ifdef DEBUG
    ख_लिखो(मानक_त्रुटि, "Error: %s\n", message);
#endif
    पात();
}

M3_WEAK
M3Result m3_Yield ()
{
    प्रत्यागम m3Err_none;
}

#यदि d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

स्थिर uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    यदि (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    संरचना timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    प्रत्यागम us - initial_ts;
}

#endif

#यदि d_m3FixedHeap

स्थिर u8        fixedHeap[d_m3FixedHeap];
स्थिर u8*       fixedHeapPtr  = fixedHeap;
स्थिर u8* ध्रुव fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
स्थिर u8*       fixedHeapLast = रिक्त;

#  यदि d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((माप_प्रकार)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  अन्यथा
#    define HEAP_ALIGN_PTR(P)
#  endif

शून्य* m3_Malloc_Impl (माप_प्रकार i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    यदि (fixedHeapPtr >= fixedHeapEnd) {
        प्रत्यागम रिक्त;
    }

    स_रखो(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    प्रत्यागम ptr;
}

शून्य m3_Free_Impl (शून्य* i_ptr)
{
    // Handle the last chunk
    यदि (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = रिक्त;
    } अन्यथा {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

शून्य* m3_Realloc_Impl (शून्य* i_ptr, माप_प्रकार i_newSize, माप_प्रकार i_oldSize)
{
    यदि (M3_UNLIKELY(i_newSize == i_oldSize)) {
        प्रत्यागम i_ptr;
    }

    शून्य* newPtr;

    // Handle the last chunk
    यदि (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        यदि (fixedHeapPtr >= fixedHeapEnd) {
            प्रत्यागम रिक्त;
        }
        newPtr = i_ptr;
    } अन्यथा {
        newPtr = m3_Malloc_Impl(i_newSize);
        यदि (!newPtr) {
            प्रत्यागम रिक्त;
        }
        यदि (i_ptr) {
            स_नकल(newPtr, i_ptr, i_oldSize);
        }
    }

    यदि (i_newSize > i_oldSize) {
        स_रखो((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    प्रत्यागम newPtr;
}

#अन्यथा

शून्य* m3_Malloc_Impl (माप_प्रकार i_size)
{
    प्रत्यागम सुस्मृति(i_size, 1);
}

शून्य m3_Free_Impl (शून्य* io_ptr)
{
    मुक्त(io_ptr);
}

शून्य* m3_Realloc_Impl (शून्य* i_ptr, माप_प्रकार i_newSize, माप_प्रकार i_oldSize)
{
    यदि (M3_UNLIKELY(i_newSize == i_oldSize)) {
        प्रत्यागम i_ptr;
    }

    शून्य* newPtr = पुनः_स्मृति(i_ptr, i_newSize);

    यदि (M3_LIKELY(newPtr)) {
        यदि (i_newSize > i_oldSize) {
            स_रखो((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        प्रत्यागम newPtr;
    }
    प्रत्यागम रिक्त;
}

#endif

शून्य* m3_CopyMem (ध्रुव शून्य* i_from, माप_प्रकार i_size)
{
    शून्य* ptr = m3_Malloc("CopyMem", i_size);
    यदि (ptr) {
        स_नकल(ptr, i_from, i_size);
    }
    प्रत्यागम ptr;
}

//--------------------------------------------------------------------------------------------

#यदि d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
स्थिर u8*    g_guardArena;
स्थिर माप_प्रकार g_guardArenaBytes;
स्थिर माप_प्रकार g_guardSlotBytes;
स्थिर माप_प्रकार g_guardPageSize;
स्थिर u32    g_guard_Slot_गिनती;
स्थिर bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
स्थिर
bool Guard_Reserve (शून्य)
{
    यदि (g_guardArena) {
        प्रत्यागम true;
    }

    माप_प्रकार pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    माप_प्रकार slotBytes = 2 * pageSize + (माप_प्रकार)d_m3GuardedDataBytes;

    प्रति (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        शून्य* base = m3_HostReserve(slotBytes * slots);

        यदि (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_गिनती  = slots;
            प्रत्यागम true;
        }
    }

    प्रत्यागम false;
}

शून्य* Guard_TakeSlot (शून्य)
{
    यदि (not Guard_Reserve()) {
        प्रत्यागम रिक्त;
    }

    प्रति (u32 i = 0; i < g_guard_Slot_गिनती; ++i) {
        यदि (g_guardSlotTaken[i]) {
            अनुवर्त;
        }

        u8* slot = g_guardArena + (माप_प्रकार)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        यदि (not m3_HostCommit(slot, g_guardPageSize)) {
            प्रत्यागम रिक्त;
        }

        g_guardSlotTaken[i] = true;
        प्रत्यागम slot;
    }

    प्रत्यागम रिक्त;
}

M3MemoryHeader* Guard_SlotHeader (शून्य* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    प्रत्यागम (M3MemoryHeader*)(data - परिमाण(M3MemoryHeader));
}

bool Guard_CommitSlot (शून्य* i_slot, माप_प्रकार i_dataBytes)
{
    माप_प्रकार rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    प्रत्यागम m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

शून्य Guard_GiveSlot (शून्य* i_slot)
{
    यदि (i_slot == रिक्त) {
        प्रत्यागम;
    }

    माप_प्रकार index = (माप_प्रकार)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    यदि (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_गिनती) {
        g_guardSlotTaken[index] = false;
    }
}

शून्य Guard_ArenaRange (शून्य** o_निचला, माप_प्रकार* o_bytes)
{
    *o_निचला   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#यदि d_m3_Max_Native_ढेर > 0

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
शून्य* m3_Native_ढेर_Limit (शून्य* i_ढेर_Ptr, माप_प्रकार i_budget)
{
    u8* sp = (u8*)i_ढेर_Ptr;

#  यदि M3_HAS_THREAD_LOCAL
    स्थिर M3_THREAD_LOCAL u8* base = रिक्त;
    यदि (base == रिक्त) {
        base = (u8*)m3_Host_ढेर_Base();
    }
#  अन्यथा
    u8* base = (u8*)m3_Host_ढेर_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    यदि (base and base < sp) {
        माप_प्रकार available = (माप_प्रकार)(sp - base);
        माप_प्रकार margin    = M3_MIN((माप_प्रकार)(d_m3_Native_ढेर_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    प्रत्यागम sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#यदि d_m3_Log_Native_ढेर

स्थिर माप_प्रकार ढेर_start;
स्थिर माप_प्रकार ढेर_end;

शून्य m3_ढेर_Check_Init ()
{
    वर्ण ढेर;
    ढेर_end = ढेर_start = (माप_प्रकार)&ढेर;
}

शून्य m3_ढेर_Check ()
{
    वर्ण   ढेर;
    माप_प्रकार addr = (माप_प्रकार)&ढेर;

    माप_प्रकार ढेर_End = ढेर_end;

    ढेर_end = M3_MIN(ढेर_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

पूर्णाङ्क m3_ढेर_Get_Max ()
{
    प्रत्यागम ढेर_start - ढेर_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    यदि (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    अन्यथा यदि (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } अन्यथा यदि (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#यदि d_m3HasExceptionHandling
    अन्यथा यदि (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#यदि d_m3_Has_ढेर_Switching
    अन्यथा यदि (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    अन्यथा यदि (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    प्रत्यागम result;
}


#यदि d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    यदि (IsSpelledRefType(i_type)) {
        यदि (i_type & d_m3Type_refCont) {
            प्रत्यागम c_m3Type_contref;
        }
        प्रत्यागम (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    प्रत्यागम (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    यदि (i_sub == i_super) {
        प्रत्यागम true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    यदि (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        प्रत्यागम false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    यदि (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        प्रत्यागम false;
    }

#  यदि d_m3_Has_ढेर_Switching
    // nocont sits under every continuation type
    यदि (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        प्रत्यागम true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    प्रत्यागम (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    प्रत्यागम (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    प्रत्यागम (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#यदि d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#यदि d_m3_Has_ढेर_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    प्रत्यागम (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    यदि (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        प्रत्यागम true;
    } अन्यथा यदि (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        प्रत्यागम false;
    } अन्यथा {
        प्रत्यागम (परिमाण(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    यदि (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        प्रत्यागम परिमाण(i32);
    }

    यदि (IsRefType(i_m3Type)) {
        प्रत्यागम परिमाण(शून्य*);
    }

    प्रत्यागम परिमाण(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(u64);

    यदि (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, परिमाण(u64));
        M3_BSWAP_u64(*o_मूल्य);
        *io_bytes = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(u32);

    यदि (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, परिमाण(u32));
        M3_BSWAP_u32(*o_मूल्य);
        *io_bytes = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}

#यदि d_m3ImplementFloat

M3Result Read_f64 (f64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(f64);

    यदि (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, परिमाण(f64));
        M3_BSWAP_f64(*o_मूल्य);
        *io_bytes = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;
    ptr += परिमाण(f32);

    यदि (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, परिमाण(f32));
        M3_BSWAP_f32(*o_मूल्य);
        *io_bytes = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;

    यदि (M3_LIKELY(ptr < i_end)) {
        *o_मूल्य  = *ptr;
        *io_bytes = ptr + 1;

        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    ध्रुव u8* ptr = *io_bytes;

    यदि (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_मूल्य  = opcode;
        *io_bytes = ptr;

        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_मूल्य, u32 i_max_अंक_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 मूल्य = 0;

    u32       shift = 0;
    ध्रुव u8* ptr   = *io_bytes;

    यावत् (ptr < i_end) {
        u64 byte = *(ptr++);

        मूल्य |= ((byte & 0x7f) << shift);
        shift += 7;

        यदि (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            यदि (M3_UNLIKELY(shift > i_max_अंक_Bits)) {
                u32 अंक_Used_Bits = i_max_अंक_Bits + 7 - shift;

                यदि (byte >> अंक_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            भङ्ग;
        }

        यदि (M3_UNLIKELY(shift >= i_max_अंक_Bits)) {
            result = m3Err_lebOverflow;
            भङ्ग;
        }
    }

    *o_मूल्य  = मूल्य;
    *io_bytes = ptr;

    प्रत्यागम result;
}


M3Result ReadLebSigned (i64* o_मूल्य, u32 i_max_अंक_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 मूल्य = 0;

    u32       shift = 0;
    ध्रुव u8* ptr   = *io_bytes;

    यावत् (ptr < i_end) {
        u64 byte = *(ptr++);

        मूल्य |= ((byte & 0x7f) << shift);
        shift += 7;

        यदि (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            यदि (M3_UNLIKELY(shift > i_max_अंक_Bits)) {
                u32 अंक_Used_Bits = i_max_अंक_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (अंक_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                यदि (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            यदि ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                मूल्य |= (~extend << shift);
            }

            भङ्ग;
        }

        यदि (M3_UNLIKELY(shift >= i_max_अंक_Bits)) {
            result = m3Err_lebOverflow;
            भङ्ग;
        }
    }

    *o_मूल्य  = मूल्य;
    *io_bytes = ptr;

    प्रत्यागम result;
}


M3Result ReadLEB_u32 (u32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      मूल्य;
    M3Result result = ReadLebUnsigned(&मूल्य, 32, io_bytes, i_end);

    *o_मूल्य = (u32)मूल्य;

    प्रत्यागम result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    यदि (result) {
        प्रत्यागम result;
    }

    *o_memoryIdx = 0;

#यदि d_m3HasMultiMemory
    यदि (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        यदि (result) {
            प्रत्यागम result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    प्रत्यागम ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      मूल्य;
    M3Result result = ReadLebUnsigned(&मूल्य, 7, io_bytes, i_end);

    *o_मूल्य = (u8)मूल्य;

    प्रत्यागम result;
}


M3Result ReadLEB_i7 (i8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मूल्य;
    M3Result result = ReadLebSigned(&मूल्य, 7, io_bytes, i_end);

    *o_मूल्य = (i8)मूल्य;

    प्रत्यागम result;
}


M3Result ReadLEB_i32 (i32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मूल्य;
    M3Result result = ReadLebSigned(&मूल्य, 32, io_bytes, i_end);

    *o_मूल्य = (i32)मूल्य;

    प्रत्यागम result;
}


M3Result ReadLEB_i64 (i64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मूल्य;
    M3Result result = ReadLebSigned(&मूल्य, 64, io_bytes, i_end);

    *o_मूल्य = मूल्य;

    प्रत्यागम result;
}

#यदि d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
स्थिर
bool IsValidUtf8 (ध्रुव u8* i_data, u32 i_length)
{
    ध्रुव u8* ptr = i_data;
    ध्रुव u8* end = i_data + i_length;

    यावत् (ptr < end) {
        u8 b0 = *ptr++;

        यदि (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            अनुवर्त;
        } अन्यथा यदि ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            यदि (b0 < 0xC2) {
                प्रत्यागम false;       // overlong
            }
            यदि (ptr >= end) {
                प्रत्यागम false;
            }
            u8 b1 = *ptr++;
            यदि ((b1 & 0xC0) != 0x80) {
                प्रत्यागम false;
            }
        } अन्यथा यदि ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            यदि (ptr + 1 >= end) {
                प्रत्यागम false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            यदि ((b1 & 0xC0) != 0x80) {
                प्रत्यागम false;
            }
            यदि ((b2 & 0xC0) != 0x80) {
                प्रत्यागम false;
            }
            // reject overlong
            यदि (b0 == 0xE0 && b1 < 0xA0) {
                प्रत्यागम false;
            }
            // reject surrogates U+D800..U+DFFF
            यदि (b0 == 0xED && b1 >= 0xA0) {
                प्रत्यागम false;
            }
        } अन्यथा यदि ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            यदि (b0 > 0xF4) {
                प्रत्यागम false;       // above U+10FFFF
            }
            यदि (ptr + 2 >= end) {
                प्रत्यागम false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            यदि ((b1 & 0xC0) != 0x80) {
                प्रत्यागम false;
            }
            यदि ((b2 & 0xC0) != 0x80) {
                प्रत्यागम false;
            }
            यदि ((b3 & 0xC0) != 0x80) {
                प्रत्यागम false;
            }
            // reject overlong
            यदि (b0 == 0xF0 && b1 < 0x90) {
                प्रत्यागम false;
            }
            // reject above U+10FFFF
            यदि (b0 == 0xF4 && b1 > 0x8F) {
                प्रत्यागम false;
            }
        } अन्यथा {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            प्रत्यागम false;
        }
    }

    प्रत्यागम true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = रिक्त;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    यदि (not result) {
        यदि (utf8Length <= d_m3MaxSaneUtf8Length) {
            ध्रुव u8* ptr = *io_bytes;
            यदि (utf8Length <= (माप_प्रकार)(i_end - ptr)) {
                ध्रुव u8* end = ptr + utf8Length;
#यदि d_m3EnableValidation
                यदि (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    प्रत्यागम m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                वर्ण* utf8 = (वर्ण*)m3_Malloc("UTF8", utf8Length + 1);

                यदि (utf8) {
                    स_नकल(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } अन्यथा {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } अन्यथा {
                result = m3Err_wasmUnderrun;
            }
        } अन्यथा {
            result = m3Err_missingUTF8;
        }
    }

    प्रत्यागम result;
}

#यदि d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    यावत् (curr) {
        यदि (ContainsPC(curr, i_pc)) {
            pageFound = true;
            भङ्ग;
        }
        curr = curr->info.next;
    }

    यदि (!pageFound) {
        curr = i_runtime->pagesFull;
        यावत् (curr) {
            यदि (ContainsPC(curr, i_pc)) {
                pageFound = true;
                भङ्ग;
            }
            curr = curr->info.next;
        }
    }

    यदि (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (शून्य)pcFound;                                                          d_m3Assert (pcFound);

        प्रत्यागम result;
    } अन्यथा {
        प्रत्यागम 0;
    }
}


शून्य धकेलो_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    यदि (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        प्रत्यागम;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    यदि (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        प्रत्यागम;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    यदि (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } अन्यथा {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


शून्य FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    यदि (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        प्रत्यागम;
    }

    यदि (!io_runtime->backtrace.lastFrame) {
        प्रत्यागम;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


शून्य ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    यावत् (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = रिक्त;
    io_runtime->backtrace.lastFrame = रिक्त;
}
#endif // d_m3RecordBacktraces
