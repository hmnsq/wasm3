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
#அந்தர்பாவ "m3_config.h"
#அந்தர்பாவ "wasm3.h"

#அந்தர்பாவ "m3_core.h"
#அந்தர்பாவ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#அந்தர்பாவ "m3_host.h"

#எனில் d_m3HasWin32Host
#  include "m3_host_win32.h"
#அல்லதெனில் d_m3HasPosixHost
#  include "m3_host_posix.h"
#இல்லையெனில்
#  include "m3_host_none.h"
#endif

வெறுமை m3_Abort (மாறிலி எழுத்து* message)
{
#ifdef DEBUG
    fprintf(stderr, "Error: %s\n", message);
#endif
    abort();
}

M3_WEAK
M3Result m3_Yield ()
{
    திரும்பு m3Err_none;
}

#எனில் d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

நிலையான uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    எனில் (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    கட்டமைப்பு timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    திரும்பு us - initial_ts;
}

#endif

#எனில் d_m3FixedHeap

நிலையான u8        fixedHeap[d_m3FixedHeap];
நிலையான u8*       fixedHeapPtr  = fixedHeap;
நிலையான u8* மாறிலி fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
நிலையான u8*       fixedHeapLast = வெற்று;

#  எனில் d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  இல்லையெனில்
#    define HEAP_ALIGN_PTR(P)
#  endif

வெறுமை* m3_Malloc_Impl (size_t i_size)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_size;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    எனில் (fixedHeapPtr >= fixedHeapEnd) {
        திரும்பு வெற்று;
    }

    memset(ptr, 0x0, i_size);
    fixedHeapLast = ptr;

    திரும்பு ptr;
}

வெறுமை m3_Free_Impl (வெறுமை* i_ptr)
{
    // Handle the last chunk
    எனில் (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = வெற்று;
    } இல்லையெனில் {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

வெறுமை* m3_Realloc_Impl (வெறுமை* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    எனில் (M3_UNLIKELY(i_newSize == i_oldSize)) {
        திரும்பு i_ptr;
    }

    வெறுமை* newPtr;

    // Handle the last chunk
    எனில் (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_newSize;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        எனில் (fixedHeapPtr >= fixedHeapEnd) {
            திரும்பு வெற்று;
        }
        newPtr = i_ptr;
    } இல்லையெனில் {
        newPtr = m3_Malloc_Impl(i_newSize);
        எனில் (!newPtr) {
            திரும்பு வெற்று;
        }
        எனில் (i_ptr) {
            memcpy(newPtr, i_ptr, i_oldSize);
        }
    }

    எனில் (i_newSize > i_oldSize) {
        memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
    }

    திரும்பு newPtr;
}

#இல்லையெனில்

வெறுமை* m3_Malloc_Impl (size_t i_size)
{
    திரும்பு calloc(i_size, 1);
}

வெறுமை m3_Free_Impl (வெறுமை* io_ptr)
{
    விடுவி(io_ptr);
}

வெறுமை* m3_Realloc_Impl (வெறுமை* i_ptr, size_t i_newSize, size_t i_oldSize)
{
    எனில் (M3_UNLIKELY(i_newSize == i_oldSize)) {
        திரும்பு i_ptr;
    }

    வெறுமை* newPtr = realloc(i_ptr, i_newSize);

    எனில் (M3_LIKELY(newPtr)) {
        எனில் (i_newSize > i_oldSize) {
            memset((u8*)newPtr + i_oldSize, 0x0, i_newSize - i_oldSize);
        }
        திரும்பு newPtr;
    }
    திரும்பு வெற்று;
}

#endif

வெறுமை* m3_CopyMem (மாறிலி வெறுமை* i_from, size_t i_size)
{
    வெறுமை* ptr = m3_Malloc("CopyMem", i_size);
    எனில் (ptr) {
        memcpy(ptr, i_from, i_size);
    }
    திரும்பு ptr;
}

//--------------------------------------------------------------------------------------------

#எனில் d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
நிலையான u8*    g_guardArena;
நிலையான size_t g_guardArenaBytes;
நிலையான size_t g_guardSlotBytes;
நிலையான size_t g_guardPageSize;
நிலையான u32    g_guard_Slot_மொத்தம்;
நிலையான bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
நிலையான
bool Guard_Reserve (வெறுமை)
{
    எனில் (g_guardArena) {
        திரும்பு true;
    }

    size_t pageSize = m3_HostPageSize();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slotBytes = 2 * pageSize + (size_t)d_m3GuardedDataBytes;

    க்கு (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        வெறுமை* base = m3_HostReserve(slotBytes * slots);

        எனில் (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guardPageSize   = pageSize;
            g_guard_Slot_மொத்தம்  = slots;
            திரும்பு true;
        }
    }

    திரும்பு false;
}

வெறுமை* Guard_TakeSlot (வெறுமை)
{
    எனில் (not Guard_Reserve()) {
        திரும்பு வெற்று;
    }

    க்கு (u32 i = 0; i < g_guard_Slot_மொத்தம்; ++i) {
        எனில் (g_guardSlotTaken[i]) {
            தொடர்;
        }

        u8* slot = g_guardArena + (size_t)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        எனில் (not m3_HostCommit(slot, g_guardPageSize)) {
            திரும்பு வெற்று;
        }

        g_guardSlotTaken[i] = true;
        திரும்பு slot;
    }

    திரும்பு வெற்று;
}

M3MemoryHeader* Guard_SlotHeader (வெறுமை* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* data = (u8*)i_slot + g_guardPageSize;

    திரும்பு (M3MemoryHeader*)(data - அளவு(M3MemoryHeader));
}

bool Guard_CommitSlot (வெறுமை* i_slot, size_t i_dataBytes)
{
    size_t rounded = (i_dataBytes + g_guardPageSize - 1) / g_guardPageSize * g_guardPageSize;

    திரும்பு m3_HostCommit((u8*)i_slot + g_guardPageSize, rounded);
}

வெறுமை Guard_GiveSlot (வெறுமை* i_slot)
{
    எனில் (i_slot == வெற்று) {
        திரும்பு;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    எனில் (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_மொத்தம்) {
        g_guardSlotTaken[index] = false;
    }
}

வெறுமை Guard_ArenaRange (வெறுமை** o_கீழ், size_t* o_bytes)
{
    *o_கீழ்   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#endif // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#எனில் d_m3_Max_Native_அடுக்கு > 0

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
வெறுமை* m3_Native_அடுக்கு_Limit (வெறுமை* i_அடுக்கு_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_அடுக்கு_Ptr;

#  எனில் M3_HAS_THREAD_LOCAL
    நிலையான M3_THREAD_LOCAL u8* base = வெற்று;
    எனில் (base == வெற்று) {
        base = (u8*)m3_Host_அடுக்கு_Base();
    }
#  இல்லையெனில்
    u8* base = (u8*)m3_Host_அடுக்கு_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    எனில் (base and base < sp) {
        size_t available = (size_t)(sp - base);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_அடுக்கு_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    திரும்பு sp - i_budget;
}

#endif // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#எனில் d_m3_Log_Native_அடுக்கு

நிலையான size_t அடுக்கு_start;
நிலையான size_t அடுக்கு_end;

வெறுமை m3_அடுக்கு_Check_Init ()
{
    எழுத்து அடுக்கு;
    அடுக்கு_end = அடுக்கு_start = (size_t)&அடுக்கு;
}

வெறுமை m3_அடுக்கு_Check ()
{
    எழுத்து   அடுக்கு;
    size_t addr = (size_t)&அடுக்கு;

    size_t அடுக்கு_End = அடுக்கு_end;

    அடுக்கு_end = M3_MIN(அடுக்கு_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

முழு m3_அடுக்கு_Get_Max ()
{
    திரும்பு அடுக்கு_start - அடுக்கு_end;
}

#endif

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3Result NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3Result result = m3Err_none;

    u8 type = -i_convolutedWasmType;

    எனில் (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    இல்லையெனில் எனில் (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } இல்லையெனில் எனில் (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#எனில் d_m3HasExceptionHandling
    இல்லையெனில் எனில் (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#endif
#எனில் d_m3_Has_அடுக்கு_Switching
    இல்லையெனில் எனில் (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#endif
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    இல்லையெனில் எனில் (type < c_m3Type_i32 or type > c_m3Type_v128) {
        result = m3Err_invalidTypeId;
    }

    *o_type = type;

    திரும்பு result;
}


#எனில் d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    எனில் (IsSpelledRefType(i_type)) {
        எனில் (i_type & d_m3Type_refCont) {
            திரும்பு c_m3Type_contref;
        }
        திரும்பு (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    திரும்பு (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    எனில் (i_sub == i_super) {
        திரும்பு true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    எனில் (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        திரும்பு false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    எனில் (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        திரும்பு false;
    }

#  எனில் d_m3_Has_அடுக்கு_Switching
    // nocont sits under every continuation type
    எனில் (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        திரும்பு true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    திரும்பு (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#endif // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    திரும்பு (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    திரும்பு (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#எனில் d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#endif
#எனில் d_m3_Has_அடுக்கு_Switching
            or i_m3Type == c_m3Type_contref
#endif
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    திரும்பு (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    எனில் (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        திரும்பு true;
    } இல்லையெனில் எனில் (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        திரும்பு false;
    } இல்லையெனில் {
        திரும்பு (அளவு(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 SizeOfType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    எனில் (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        திரும்பு அளவு(i32);
    }

    எனில் (IsRefType(i_m3Type)) {
        திரும்பு அளவு(வெறுமை*);
    }

    திரும்பு அளவு(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3Result Read_u64 (u64* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    மாறிலி u8* ptr = *io_bytes;
    ptr += அளவு(u64);

    எனில் (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_மதிப்பு, *io_bytes, அளவு(u64));
        M3_BSWAP_u64(*o_மதிப்பு);
        *io_bytes = ptr;
        திரும்பு m3Err_none;
    } இல்லையெனில் {
        திரும்பு m3Err_wasmUnderrun;
    }
}


M3Result Read_u32 (u32* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    மாறிலி u8* ptr = *io_bytes;
    ptr += அளவு(u32);

    எனில் (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_மதிப்பு, *io_bytes, அளவு(u32));
        M3_BSWAP_u32(*o_மதிப்பு);
        *io_bytes = ptr;
        திரும்பு m3Err_none;
    } இல்லையெனில் {
        திரும்பு m3Err_wasmUnderrun;
    }
}

#எனில் d_m3ImplementFloat

M3Result Read_f64 (f64* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    மாறிலி u8* ptr = *io_bytes;
    ptr += அளவு(f64);

    எனில் (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_மதிப்பு, *io_bytes, அளவு(f64));
        M3_BSWAP_f64(*o_மதிப்பு);
        *io_bytes = ptr;
        திரும்பு m3Err_none;
    } இல்லையெனில் {
        திரும்பு m3Err_wasmUnderrun;
    }
}


M3Result Read_f32 (f32* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    மாறிலி u8* ptr = *io_bytes;
    ptr += அளவு(f32);

    எனில் (M3_LIKELY(ptr <= i_end)) {
        memcpy(o_மதிப்பு, *io_bytes, அளவு(f32));
        M3_BSWAP_f32(*o_மதிப்பு);
        *io_bytes = ptr;
        திரும்பு m3Err_none;
    } இல்லையெனில் {
        திரும்பு m3Err_wasmUnderrun;
    }
}

#endif

M3Result Read_u8 (u8* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    மாறிலி u8* ptr = *io_bytes;

    எனில் (M3_LIKELY(ptr < i_end)) {
        *o_மதிப்பு  = *ptr;
        *io_bytes = ptr + 1;

        திரும்பு m3Err_none;
    } இல்லையெனில் {
        திரும்பு m3Err_wasmUnderrun;
    }
}

M3Result Read_opcode (m3opcode_t* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    மாறிலி u8* ptr = *io_bytes;

    எனில் (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_மதிப்பு  = opcode;
        *io_bytes = ptr;

        திரும்பு m3Err_none;
    } இல்லையெனில் {
        திரும்பு m3Err_wasmUnderrun;
    }
}


M3Result ReadLebUnsigned (u64* o_மதிப்பு, u32 i_max_இலக்கம்_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    u64 மதிப்பு = 0;

    u32       shift = 0;
    மாறிலி u8* ptr   = *io_bytes;

    போது (ptr < i_end) {
        u64 byte = *(ptr++);

        மதிப்பு |= ((byte & 0x7f) << shift);
        shift += 7;

        எனில் (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            எனில் (M3_UNLIKELY(shift > i_max_இலக்கம்_Bits)) {
                u32 இலக்கம்_Used_Bits = i_max_இலக்கம்_Bits + 7 - shift;

                எனில் (byte >> இலக்கம்_Used_Bits) {
                    result = m3Err_lebOverflow;
                }
            }
            நிறுத்து;
        }

        எனில் (M3_UNLIKELY(shift >= i_max_இலக்கம்_Bits)) {
            result = m3Err_lebOverflow;
            நிறுத்து;
        }
    }

    *o_மதிப்பு  = மதிப்பு;
    *io_bytes = ptr;

    திரும்பு result;
}


M3Result ReadLebSigned (i64* o_மதிப்பு, u32 i_max_இலக்கம்_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_wasmUnderrun;

    i64 மதிப்பு = 0;

    u32       shift = 0;
    மாறிலி u8* ptr   = *io_bytes;

    போது (ptr < i_end) {
        u64 byte = *(ptr++);

        மதிப்பு |= ((byte & 0x7f) << shift);
        shift += 7;

        எனில் (M3_LIKELY((byte & 0x80) == 0)) {
            result = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            எனில் (M3_UNLIKELY(shift > i_max_இலக்கம்_Bits)) {
                u32 இலக்கம்_Used_Bits = i_max_இலக்கம்_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (இலக்கம்_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                எனில் (bits != 0 and bits != signBits) {
                    result = m3Err_lebOverflow;
                }
            }
            எனில் ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                மதிப்பு |= (~extend << shift);
            }

            நிறுத்து;
        }

        எனில் (M3_UNLIKELY(shift >= i_max_இலக்கம்_Bits)) {
            result = m3Err_lebOverflow;
            நிறுத்து;
        }
    }

    *o_மதிப்பு  = மதிப்பு;
    *io_bytes = ptr;

    திரும்பு result;
}


M3Result ReadLEB_u32 (u32* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      மதிப்பு;
    M3Result result = ReadLebUnsigned(&மதிப்பு, 32, io_bytes, i_end);

    *o_மதிப்பு = (u32)மதிப்பு;

    திரும்பு result;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#define d_memArgHasMemoryIdx    0x40u

M3Result ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 align;
    result = ReadLEB_u32(&align, io_bytes, i_end);
    எனில் (result) {
        திரும்பு result;
    }

    *o_memoryIdx = 0;

#எனில் d_m3HasMultiMemory
    எனில் (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        result = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        எனில் (result) {
            திரும்பு result;
        }
    }
#endif

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    திரும்பு ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3Result ReadLEB_u7 (u8* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      மதிப்பு;
    M3Result result = ReadLebUnsigned(&மதிப்பு, 7, io_bytes, i_end);

    *o_மதிப்பு = (u8)மதிப்பு;

    திரும்பு result;
}


M3Result ReadLEB_i7 (i8* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      மதிப்பு;
    M3Result result = ReadLebSigned(&மதிப்பு, 7, io_bytes, i_end);

    *o_மதிப்பு = (i8)மதிப்பு;

    திரும்பு result;
}


M3Result ReadLEB_i32 (i32* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      மதிப்பு;
    M3Result result = ReadLebSigned(&மதிப்பு, 32, io_bytes, i_end);

    *o_மதிப்பு = (i32)மதிப்பு;

    திரும்பு result;
}


M3Result ReadLEB_i64 (i64* o_மதிப்பு, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      மதிப்பு;
    M3Result result = ReadLebSigned(&மதிப்பு, 64, io_bytes, i_end);

    *o_மதிப்பு = மதிப்பு;

    திரும்பு result;
}

#எனில் d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
நிலையான
bool IsValidUtf8 (மாறிலி u8* i_data, u32 i_length)
{
    மாறிலி u8* ptr = i_data;
    மாறிலி u8* end = i_data + i_length;

    போது (ptr < end) {
        u8 b0 = *ptr++;

        எனில் (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            தொடர்;
        } இல்லையெனில் எனில் ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            எனில் (b0 < 0xC2) {
                திரும்பு false;       // overlong
            }
            எனில் (ptr >= end) {
                திரும்பு false;
            }
            u8 b1 = *ptr++;
            எனில் ((b1 & 0xC0) != 0x80) {
                திரும்பு false;
            }
        } இல்லையெனில் எனில் ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            எனில் (ptr + 1 >= end) {
                திரும்பு false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            எனில் ((b1 & 0xC0) != 0x80) {
                திரும்பு false;
            }
            எனில் ((b2 & 0xC0) != 0x80) {
                திரும்பு false;
            }
            // reject overlong
            எனில் (b0 == 0xE0 && b1 < 0xA0) {
                திரும்பு false;
            }
            // reject surrogates U+D800..U+DFFF
            எனில் (b0 == 0xED && b1 >= 0xA0) {
                திரும்பு false;
            }
        } இல்லையெனில் எனில் ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            எனில் (b0 > 0xF4) {
                திரும்பு false;       // above U+10FFFF
            }
            எனில் (ptr + 2 >= end) {
                திரும்பு false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            எனில் ((b1 & 0xC0) != 0x80) {
                திரும்பு false;
            }
            எனில் ((b2 & 0xC0) != 0x80) {
                திரும்பு false;
            }
            எனில் ((b3 & 0xC0) != 0x80) {
                திரும்பு false;
            }
            // reject overlong
            எனில் (b0 == 0xF0 && b1 < 0x90) {
                திரும்பு false;
            }
            // reject above U+10FFFF
            எனில் (b0 == 0xF4 && b1 > 0x8F) {
                திரும்பு false;
            }
        } இல்லையெனில் {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            திரும்பு false;
        }
    }

    திரும்பு true;
}
#endif // d_m3EnableValidation


M3Result Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = வெற்று;

    u32      utf8Length;
    M3Result result = ReadLEB_u32(&utf8Length, io_bytes, i_end);

    எனில் (not result) {
        எனில் (utf8Length <= d_m3MaxSaneUtf8Length) {
            மாறிலி u8* ptr = *io_bytes;
            எனில் (utf8Length <= (size_t)(i_end - ptr)) {
                மாறிலி u8* end = ptr + utf8Length;
#எனில் d_m3EnableValidation
                எனில் (not IsValidUtf8(ptr, utf8Length)) {
                    *io_bytes = end;
                    திரும்பு m3Err_wasmMalformed;
                }
#endif // d_m3EnableValidation

                எழுத்து* utf8 = (எழுத்து*)m3_Malloc("UTF8", utf8Length + 1);

                எனில் (utf8) {
                    memcpy(utf8, ptr, utf8Length);
                    utf8[utf8Length] = 0;

                    *o_utf8 = utf8;
                } இல்லையெனில் {
                    result = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } இல்லையெனில் {
                result = m3Err_wasmUnderrun;
            }
        } இல்லையெனில் {
            result = m3Err_missingUTF8;
        }
    }

    திரும்பு result;
}

#எனில் d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    போது (curr) {
        எனில் (ContainsPC(curr, i_pc)) {
            pageFound = true;
            நிறுத்து;
        }
        curr = curr->info.next;
    }

    எனில் (!pageFound) {
        curr = i_runtime->pagesFull;
        போது (curr) {
            எனில் (ContainsPC(curr, i_pc)) {
                pageFound = true;
                நிறுத்து;
            }
            curr = curr->info.next;
        }
    }

    எனில் (pageFound) {
        u32 result = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &result);
        (வெறுமை)pcFound;                                                          d_m3Assert (pcFound);

        திரும்பு result;
    } இல்லையெனில் {
        திரும்பு 0;
    }
}


வெறுமை தள்ளு_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    எனில் (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        திரும்பு;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    எனில் (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        திரும்பு;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    எனில் (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } இல்லையெனில் {
        io_runtime->backtrace.lastFrame->next = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


வெறுமை FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    எனில் (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        திரும்பு;
    }

    எனில் (!io_runtime->backtrace.lastFrame) {
        திரும்பு;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


வெறுமை ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    போது (currentFrame) {
        M3BacktraceFrame* nextFrame = currentFrame->next;
        m3_Free(currentFrame);
        currentFrame = nextFrame;
    }

    io_runtime->backtrace.frames    = வெற்று;
    io_runtime->backtrace.lastFrame = வெற்று;
}
#endif // d_m3RecordBacktraces
