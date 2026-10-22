//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#यदि_अपरिभाषितम् _GNU_SOURCE
#  define _GNU_SOURCE 1
#यदि_अन्तः

#परिभाषय M3_IMPLEMENT_ERROR_वाक्यानि
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
#यदि_अन्तः

शून्य m3_Abort (ध्रुव वर्ण* सन्देशः)
{
#यदि_परिभाषितम् DEBUG
    fprintf(stderr, "Error: %s\n", सन्देशः);
#यदि_अन्तः
    abort();
}

M3_WEAK
M3_परिणाम m3_Yield ()
{
    प्रत्यागम m3Err_none;
}

#यदि d_m3LogTimestamps

#  include <time.h>

#  define SEC_गन्तव्यम्_US(sec) ((sec)*1000000)
#  define NS_गन्तव्यम्_US(ns)    ((ns)/1000)

स्थिर uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    यदि (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    संरचना timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_गन्तव्यम्_US((uint64_t)ts.tv_sec) + NS_गन्तव्यम्_US((uint64_t)ts.tv_nsec);
    प्रत्यागम us - initial_ts;
}

#यदि_अन्तः

#यदि d_m3FixedHeap

स्थिर u8        fixedHeap[d_m3FixedHeap];
स्थिर u8*       fixedHeapPtr  = fixedHeap;
स्थिर u8* ध्रुव fixed_Heap_अन्तः  = fixedHeap + d_m3FixedHeap;
स्थिर u8*       fixed_Heap_चरमः = रिक्त;

#  यदि d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  अन्यथा
#    define HEAP_ALIGN_PTR(P)
#  endif

शून्य* m3_Malloc_Impl (size_t i_आकार)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_आकार;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    यदि (fixedHeapPtr >= fixed_Heap_अन्तः) {
        प्रत्यागम रिक्त;
    }

    स्मृतिं_पूरय(ptr, 0x0, i_आकार);
    fixed_Heap_चरमः = ptr;

    प्रत्यागम ptr;
}

शून्य m3_Free_Impl (शून्य* i_ptr)
{
    // Handle the last chunk
    यदि (i_ptr && i_ptr == fixed_Heap_चरमः) {
        fixedHeapPtr  = fixed_Heap_चरमः;
        fixed_Heap_चरमः = रिक्त;
    } अन्यथा {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

शून्य* m3_Realloc_Impl (शून्य* i_ptr, size_t i_new_आकार, size_t i_old_आकार)
{
    यदि (M3_UNLIKELY(i_new_आकार == i_old_आकार)) {
        प्रत्यागम i_ptr;
    }

    शून्य* newPtr;

    // Handle the last chunk
    यदि (i_ptr && i_ptr == fixed_Heap_चरमः) {
        fixedHeapPtr = fixed_Heap_चरमः + i_new_आकार;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        यदि (fixedHeapPtr >= fixed_Heap_अन्तः) {
            प्रत्यागम रिक्त;
        }
        newPtr = i_ptr;
    } अन्यथा {
        newPtr = m3_Malloc_Impl(i_new_आकार);
        यदि (!newPtr) {
            प्रत्यागम रिक्त;
        }
        यदि (i_ptr) {
            स्मृतिं_प्रतिलिख(newPtr, i_ptr, i_old_आकार);
        }
    }

    यदि (i_new_आकार > i_old_आकार) {
        स्मृतिं_पूरय((u8*)newPtr + i_old_आकार, 0x0, i_new_आकार - i_old_आकार);
    }

    प्रत्यागम newPtr;
}

#अन्यथा

शून्य* m3_Malloc_Impl (size_t i_आकार)
{
    प्रत्यागम शून्यस्मृतिं_देहि(i_आकार, 1);
}

शून्य m3_Free_Impl (शून्य* io_ptr)
{
    मुक्त(io_ptr);
}

शून्य* m3_Realloc_Impl (शून्य* i_ptr, size_t i_new_आकार, size_t i_old_आकार)
{
    यदि (M3_UNLIKELY(i_new_आकार == i_old_आकार)) {
        प्रत्यागम i_ptr;
    }

    शून्य* newPtr = realloc(i_ptr, i_new_आकार);

    यदि (M3_LIKELY(newPtr)) {
        यदि (i_new_आकार > i_old_आकार) {
            स्मृतिं_पूरय((u8*)newPtr + i_old_आकार, 0x0, i_new_आकार - i_old_आकार);
        }
        प्रत्यागम newPtr;
    }
    प्रत्यागम रिक्त;
}

#यदि_अन्तः

शून्य* m3_CopyMem (ध्रुव शून्य* i_from, size_t i_आकार)
{
    शून्य* ptr = m3_Malloc("CopyMem", i_आकार);
    यदि (ptr) {
        स्मृतिं_प्रतिलिख(ptr, i_from, i_आकार);
    }
    प्रत्यागम ptr;
}

//--------------------------------------------------------------------------------------------

#यदि d_m3_Guarded_स्मृतिः

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
स्थिर u8*    g_guardArena;
स्थिर size_t g_guard_Arena_अष्टकानि;
स्थिर size_t g_guard_Slot_अष्टकानि;
स्थिर size_t g_guard_Page_आकार;
स्थिर u32    g_guard_Slot_गणनम्;
स्थिर bool   g_guard_Slot_गृहीतम्[d_m3GuardedArenaSlots];

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

    size_t page_आकार = m3_Host_Page_आकार();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t slot_अष्टकानि = 2 * page_आकार + (size_t)d_m3_Guarded_दत्त_अष्टकानि;

    प्रति (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        शून्य* आधारः = m3_HostReserve(slot_अष्टकानि * slots);

        यदि (आधारः) {
            g_guardArena      = (u8*)आधारः;
            g_guard_Arena_अष्टकानि = slot_अष्टकानि * slots;
            g_guard_Slot_अष्टकानि  = slot_अष्टकानि;
            g_guard_Page_आकार   = page_आकार;
            g_guard_Slot_गणनम्  = slots;
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

    प्रति (u32 i = 0; i < g_guard_Slot_गणनम्; ++i) {
        यदि (g_guard_Slot_गृहीतम्[i]) {
            अनुवर्त;
        }

        u8* slot = g_guardArena + (size_t)i * g_guard_Slot_अष्टकानि;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        यदि (not m3_HostCommit(slot, g_guard_Page_आकार)) {
            प्रत्यागम रिक्त;
        }

        g_guard_Slot_गृहीतम्[i] = true;
        प्रत्यागम slot;
    }

    प्रत्यागम रिक्त;
}

M3_स्मृतिः_शीर्षकम्* Guard_Slot_शीर्षकम् (शून्य* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* दत्त = (u8*)i_slot + g_guard_Page_आकार;

    प्रत्यागम (M3_स्मृतिः_शीर्षकम्*)(दत्त - परिमाण(M3_स्मृतिः_शीर्षकम्));
}

bool Guard_CommitSlot (शून्य* i_slot, size_t i_दत्त_अष्टकानि)
{
    size_t rounded = (i_दत्त_अष्टकानि + g_guard_Page_आकार - 1) / g_guard_Page_आकार * g_guard_Page_आकार;

    प्रत्यागम m3_HostCommit((u8*)i_slot + g_guard_Page_आकार, rounded);
}

शून्य Guard_GiveSlot (शून्य* i_slot)
{
    यदि (i_slot == रिक्त) {
        प्रत्यागम;
    }

    size_t index = (size_t)((u8*)i_slot - g_guardArena) / g_guard_Slot_अष्टकानि;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    यदि (m3_HostDecommit(i_slot, g_guard_Slot_अष्टकानि) and index < g_guard_Slot_गणनम्) {
        g_guard_Slot_गृहीतम्[index] = false;
    }
}

शून्य Guard_ArenaRange (शून्य** o_नीचः, size_t* o_अष्टकानि)
{
    *o_नीचः   = g_guardArena;
    *o_अष्टकानि = g_guard_Arena_अष्टकानि;
}

#यदि_अन्तः // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#यदि d_m3_Max_Native_राशिः > 0

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
शून्य* m3_Native_राशिः_सीमा (शून्य* i_राशिः_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_राशिः_Ptr;

#  यदि M3_HAS_THREAD_LOCAL
    स्थिर M3_THREAD_LOCAL u8* आधारः = रिक्त;
    यदि (आधारः == रिक्त) {
        आधारः = (u8*)m3_Host_राशिः_आधारः();
    }
#  अन्यथा
    u8* आधारः = (u8*)m3_Host_राशिः_आधारः();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    यदि (आधारः and आधारः < sp) {
        size_t उपलब्धम् = (size_t)(sp - आधारः);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_राशिः_Margin), उपलब्धम् / 4);

        i_budget = M3_MIN(i_budget, उपलब्धम् - margin);
    }

    प्रत्यागम sp - i_budget;
}

#यदि_अन्तः // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#यदि d_m3_Log_Native_राशिः

स्थिर size_t राशिः_start;
स्थिर size_t राशिः_अन्तः;

शून्य m3_राशिः_Check_Init ()
{
    वर्ण राशिः;
    राशिः_अन्तः = राशिः_start = (size_t)&राशिः;
}

शून्य m3_राशिः_Check ()
{
    वर्ण   राशिः;
    size_t addr = (size_t)&राशिः;

    size_t राशिः_अन्तः_2 = राशिः_अन्तः;

    राशिः_अन्तः = M3_MIN(राशिः_अन्तः, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

पूर्णाङ्क m3_राशिः_Get_Max ()
{
    प्रत्यागम राशिः_start - राशिः_अन्तः;
}

#यदि_अन्तः

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_परिणाम NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_परिणाम परिणाम = m3Err_none;

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
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
    अन्यथा यदि (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#यदि_अन्तः
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    अन्यथा यदि (type < c_m3Type_i32 or type > c_m3Type_v128) {
        परिणाम = m3Err_invalidTypeId;
    }

    *o_type = type;

    प्रत्यागम परिणाम;
}


#यदि d_m3HasTypedRefs

u8 आधारः_Type_सम्बन्धि (m3type_t i_type)
{
    यदि (IsSpelledRefType(i_type)) {
        यदि (i_type & d_m3Type_refCont) {
            प्रत्यागम c_m3Type_contref;
        }
        प्रत्यागम (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    प्रत्यागम (u8)i_type;
}


bool Is_Sub_Type_सम्बन्धि (m3type_t i_sub, m3type_t i_super)
{
    यदि (i_sub == i_super) {
        प्रत्यागम true;
    }

    u8 आधारः = आधारः_Type_सम्बन्धि(i_sub);

    // only references have a subtype relation; every other type is invariant
    यदि (आधारः != आधारः_Type_सम्बन्धि(i_super) or not IsRefType(आधारः)) {
        प्रत्यागम false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    यदि (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        प्रत्यागम false;
    }

#  यदि d_m3_Has_राशिः_Switching
    // nocont sits under every continuation type
    यदि (Heap_Type_सम्बन्धि(i_sub) == d_m3Type_heapNone) {
        प्रत्यागम true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    प्रत्यागम (Heap_Type_सम्बन्धि(i_super) == d_m3Type_heapAbstract) or
           (Heap_Type_सम्बन्धि(i_sub) == Heap_Type_सम्बन्धि(i_super));
}

#यदि_अन्तः // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = आधारः_Type_सम्बन्धि(i_type);
    प्रत्यागम (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = आधारः_Type_सम्बन्धि(i_type);
    प्रत्यागम (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#यदि d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
            or i_m3Type == c_m3Type_contref
#यदि_अन्तः
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = आधारः_Type_सम्बन्धि(i_type);
    प्रत्यागम (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = आधारः_Type_सम्बन्धि(i_type);

    यदि (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        प्रत्यागम true;
    } अन्यथा यदि (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        प्रत्यागम false;
    } अन्यथा {
        प्रत्यागम (परिमाण(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 आकार_सम्बन्धि_Type (m3type_t i_type)
{
    u8 i_m3Type = आधारः_Type_सम्बन्धि(i_type);

    यदि (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        प्रत्यागम परिमाण(i32);
    }

    यदि (IsRefType(i_m3Type)) {
        प्रत्यागम परिमाण(शून्य*);
    }

    प्रत्यागम परिमाण(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_परिणाम Read_u64 (u64* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    ध्रुव u8* ptr = *io_अष्टकानि;
    ptr += परिमाण(u64);

    यदि (M3_LIKELY(ptr <= i_अन्तः)) {
        स्मृतिं_प्रतिलिख(o_मानम्, *io_अष्टकानि, परिमाण(u64));
        M3_BSWAP_u64(*o_मानम्);
        *io_अष्टकानि = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}


M3_परिणाम Read_u32 (u32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    ध्रुव u8* ptr = *io_अष्टकानि;
    ptr += परिमाण(u32);

    यदि (M3_LIKELY(ptr <= i_अन्तः)) {
        स्मृतिं_प्रतिलिख(o_मानम्, *io_अष्टकानि, परिमाण(u32));
        M3_BSWAP_u32(*o_मानम्);
        *io_अष्टकानि = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}

#यदि d_m3ImplementFloat

M3_परिणाम Read_f64 (f64* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    ध्रुव u8* ptr = *io_अष्टकानि;
    ptr += परिमाण(f64);

    यदि (M3_LIKELY(ptr <= i_अन्तः)) {
        स्मृतिं_प्रतिलिख(o_मानम्, *io_अष्टकानि, परिमाण(f64));
        M3_BSWAP_f64(*o_मानम्);
        *io_अष्टकानि = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}


M3_परिणाम Read_f32 (f32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    ध्रुव u8* ptr = *io_अष्टकानि;
    ptr += परिमाण(f32);

    यदि (M3_LIKELY(ptr <= i_अन्तः)) {
        स्मृतिं_प्रतिलिख(o_मानम्, *io_अष्टकानि, परिमाण(f32));
        M3_BSWAP_f32(*o_मानम्);
        *io_अष्टकानि = ptr;
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}

#यदि_अन्तः

M3_परिणाम Read_u8 (u8* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    ध्रुव u8* ptr = *io_अष्टकानि;

    यदि (M3_LIKELY(ptr < i_अन्तः)) {
        *o_मानम्  = *ptr;
        *io_अष्टकानि = ptr + 1;

        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}

M3_परिणाम Read_opcode (m3opcode_t* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    ध्रुव u8* ptr = *io_अष्टकानि;

    यदि (M3_LIKELY(ptr < i_अन्तः)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_मानम्  = opcode;
        *io_अष्टकानि = ptr;

        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम m3Err_wasmUnderrun;
    }
}


M3_परिणाम ReadLebUnsigned (u64* o_मानम्, u32 i_max_अङ्कः_द्वयङ्काः, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_wasmUnderrun;

    u64 मानम् = 0;

    u32       shift = 0;
    ध्रुव u8* ptr   = *io_अष्टकानि;

    यावत् (ptr < i_अन्तः) {
        u64 byte = *(ptr++);

        मानम् |= ((byte & 0x7f) << shift);
        shift += 7;

        यदि (M3_LIKELY((byte & 0x80) == 0)) {
            परिणाम = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            यदि (M3_UNLIKELY(shift > i_max_अङ्कः_द्वयङ्काः)) {
                u32 अङ्कः_प्रयुक्तम्_द्वयङ्काः = i_max_अङ्कः_द्वयङ्काः + 7 - shift;

                यदि (byte >> अङ्कः_प्रयुक्तम्_द्वयङ्काः) {
                    परिणाम = m3_Err_leb_अतिप्रवाहः;
                }
            }
            भङ्ग;
        }

        यदि (M3_UNLIKELY(shift >= i_max_अङ्कः_द्वयङ्काः)) {
            परिणाम = m3_Err_leb_अतिप्रवाहः;
            भङ्ग;
        }
    }

    *o_मानम्  = मानम्;
    *io_अष्टकानि = ptr;

    प्रत्यागम परिणाम;
}


M3_परिणाम ReadLebSigned (i64* o_मानम्, u32 i_max_अङ्कः_द्वयङ्काः, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_wasmUnderrun;

    i64 मानम् = 0;

    u32       shift = 0;
    ध्रुव u8* ptr   = *io_अष्टकानि;

    यावत् (ptr < i_अन्तः) {
        u64 byte = *(ptr++);

        मानम् |= ((byte & 0x7f) << shift);
        shift += 7;

        यदि (M3_LIKELY((byte & 0x80) == 0)) {
            परिणाम = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            यदि (M3_UNLIKELY(shift > i_max_अङ्कः_द्वयङ्काः)) {
                u32 अङ्कः_प्रयुक्तम्_द्वयङ्काः = i_max_अङ्कः_द्वयङ्काः + 7 - shift;
                u8  sign_द्वयङ्काः    = (u8)((0x7f << (अङ्कः_प्रयुक्तम्_द्वयङ्काः - 1)) & 0x7f);
                u8  द्वयङ्काः        = (u8)(byte & sign_द्वयङ्काः);

                यदि (द्वयङ्काः != 0 and द्वयङ्काः != sign_द्वयङ्काः) {
                    परिणाम = m3_Err_leb_अतिप्रवाहः;
                }
            }
            यदि ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                मानम् |= (~extend << shift);
            }

            भङ्ग;
        }

        यदि (M3_UNLIKELY(shift >= i_max_अङ्कः_द्वयङ्काः)) {
            परिणाम = m3_Err_leb_अतिप्रवाहः;
            भङ्ग;
        }
    }

    *o_मानम्  = मानम्;
    *io_अष्टकानि = ptr;

    प्रत्यागम परिणाम;
}


M3_परिणाम ReadLEB_u32 (u32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    u64      मानम्;
    M3_परिणाम परिणाम = ReadLebUnsigned(&मानम्, 32, io_अष्टकानि, i_अन्तः);

    *o_मानम् = (u32)मानम्;

    प्रत्यागम परिणाम;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#परिभाषय d_mem_Arg_Has_स्मृतिः_Idx    0x40u

M3_परिणाम Read_स्मृतिः_Arg (u32* o_align, u32* o_स्मृतिः_Idx, u64* o_विस्थापनम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम;

    u32 align;
    परिणाम = ReadLEB_u32(&align, io_अष्टकानि, i_अन्तः);
    यदि (परिणाम) {
        प्रत्यागम परिणाम;
    }

    *o_स्मृतिः_Idx = 0;

#यदि d_m3_Has_Multi_स्मृतिः
    यदि (align & d_mem_Arg_Has_स्मृतिः_Idx) {
        align &= ~d_mem_Arg_Has_स्मृतिः_Idx;

        परिणाम = ReadLEB_u32(o_स्मृतिः_Idx, io_अष्टकानि, i_अन्तः);
        यदि (परिणाम) {
            प्रत्यागम परिणाम;
        }
    }
#यदि_अन्तः

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    प्रत्यागम ReadLebUnsigned(o_विस्थापनम्, 64, io_अष्टकानि, i_अन्तः);
}


M3_परिणाम ReadLEB_u7 (u8* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    u64      मानम्;
    M3_परिणाम परिणाम = ReadLebUnsigned(&मानम्, 7, io_अष्टकानि, i_अन्तः);

    *o_मानम् = (u8)मानम्;

    प्रत्यागम परिणाम;
}


M3_परिणाम ReadLEB_i7 (i8* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    i64      मानम्;
    M3_परिणाम परिणाम = ReadLebSigned(&मानम्, 7, io_अष्टकानि, i_अन्तः);

    *o_मानम् = (i8)मानम्;

    प्रत्यागम परिणाम;
}


M3_परिणाम ReadLEB_i32 (i32* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    i64      मानम्;
    M3_परिणाम परिणाम = ReadLebSigned(&मानम्, 32, io_अष्टकानि, i_अन्तः);

    *o_मानम् = (i32)मानम्;

    प्रत्यागम परिणाम;
}


M3_परिणाम ReadLEB_i64 (i64* o_मानम्, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    i64      मानम्;
    M3_परिणाम परिणाम = ReadLebSigned(&मानम्, 64, io_अष्टकानि, i_अन्तः);

    *o_मानम् = मानम्;

    प्रत्यागम परिणाम;
}

#यदि d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
स्थिर
bool IsValidUtf8 (ध्रुव u8* i_दत्त, u32 i_दीर्घता)
{
    ध्रुव u8* ptr = i_दत्त;
    ध्रुव u8* अन्तः = i_दत्त + i_दीर्घता;

    यावत् (ptr < अन्तः) {
        u8 b0 = *ptr++;

        यदि (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            अनुवर्त;
        } अन्यथा यदि ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            यदि (b0 < 0xC2) {
                प्रत्यागम false;       // overlong
            }
            यदि (ptr >= अन्तः) {
                प्रत्यागम false;
            }
            u8 b1 = *ptr++;
            यदि ((b1 & 0xC0) != 0x80) {
                प्रत्यागम false;
            }
        } अन्यथा यदि ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            यदि (ptr + 1 >= अन्तः) {
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
            यदि (ptr + 2 >= अन्तः) {
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
#यदि_अन्तः // d_m3EnableValidation


M3_परिणाम Read_utf8 (cstr_t* o_utf8, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    *o_utf8 = रिक्त;

    u32      utf8_दीर्घता;
    M3_परिणाम परिणाम = ReadLEB_u32(&utf8_दीर्घता, io_अष्टकानि, i_अन्तः);

    यदि (not परिणाम) {
        यदि (utf8_दीर्घता <= d_m3_Max_Sane_Utf8_दीर्घता) {
            ध्रुव u8* ptr = *io_अष्टकानि;
            यदि (utf8_दीर्घता <= (size_t)(i_अन्तः - ptr)) {
                ध्रुव u8* अन्तः = ptr + utf8_दीर्घता;
#यदि d_m3EnableValidation
                यदि (not IsValidUtf8(ptr, utf8_दीर्घता)) {
                    *io_अष्टकानि = अन्तः;
                    प्रत्यागम m3Err_wasmMalformed;
                }
#यदि_अन्तः // d_m3EnableValidation

                वर्ण* utf8 = (वर्ण*)m3_Malloc("UTF8", utf8_दीर्घता + 1);

                यदि (utf8) {
                    स्मृतिं_प्रतिलिख(utf8, ptr, utf8_दीर्घता);
                    utf8[utf8_दीर्घता] = 0;

                    *o_utf8 = utf8;
                } अन्यथा {
                    परिणाम = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_अष्टकानि = अन्तः;
            } अन्यथा {
                परिणाम = m3Err_wasmUnderrun;
            }
        } अन्यथा {
            परिणाम = m3Err_missingUTF8;
        }
    }

    प्रत्यागम परिणाम;
}

#यदि d_m3_लेखः_Backtraces
u32 Find_Module_विस्थापनम् (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_संहिता_Page curr = i_runtime->पृष्ठानि_Open;

    bool page_प्राप्तम् = false;

    यावत् (curr) {
        यदि (अन्तर्भवति_PC(curr, i_pc)) {
            page_प्राप्तम् = true;
            भङ्ग;
        }
        curr = curr->info.अग्र;
    }

    यदि (!page_प्राप्तम्) {
        curr = i_runtime->पृष्ठानि_पूर्णम्;
        यावत् (curr) {
            यदि (अन्तर्भवति_PC(curr, i_pc)) {
                page_प्राप्तम् = true;
                भङ्ग;
            }
            curr = curr->info.अग्र;
        }
    }

    यदि (page_प्राप्तम्) {
        u32 परिणाम = 0;

        bool pc_प्राप्तम् = Map_PCTo_विस्थापनम्(curr, i_pc, &परिणाम);
        (शून्य)pc_प्राप्तम्;                                                          d_m3Assert (pc_प्राप्तम्);

        प्रत्यागम परिणाम;
    } अन्यथा {
        प्रत्यागम 0;
    }
}


शून्य प्रक्षिप_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    यदि (M3_UNLIKELY(io_runtime->backtrace.चरमः_Frame == M3_BACKTRACE_TRUNCATED)) {
        प्रत्यागम;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    यदि (!newFrame) {
        io_runtime->backtrace.चरमः_Frame = M3_BACKTRACE_TRUNCATED;
        प्रत्यागम;
    }

    newFrame->module_विस्थापनम् = Find_Module_विस्थापनम्(io_runtime, i_pc);

    यदि (!io_runtime->backtrace.चौकटाः || !io_runtime->backtrace.चरमः_Frame) {
        io_runtime->backtrace.चौकटाः = newFrame;
    } अन्यथा {
        io_runtime->backtrace.चरमः_Frame->अग्र = newFrame;
    }
    io_runtime->backtrace.चरमः_Frame = newFrame;
}


शून्य FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    यदि (M3_UNLIKELY(io_runtime->backtrace.चरमः_Frame == M3_BACKTRACE_TRUNCATED)) {
        प्रत्यागम;
    }

    यदि (!io_runtime->backtrace.चरमः_Frame) {
        प्रत्यागम;
    }

    io_runtime->backtrace.चरमः_Frame->function = i_function;
}


शून्य ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.चौकटाः;
    यावत् (currentFrame) {
        M3BacktraceFrame* अग्र_Frame = currentFrame->अग्र;
        m3_Free(currentFrame);
        currentFrame = अग्र_Frame;
    }

    io_runtime->backtrace.चौकटाः    = रिक्त;
    io_runtime->backtrace.चरमः_Frame = रिक्त;
}
#यदि_अन्तः // d_m3RecordBacktraces
