//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#अगर_अघोषित _GNU_SOURCE
#  define _GNU_SOURCE 1
#पूर्ण_अगर

#घोषणा M3_IMPLEMENT_ERROR_STRINGS
#समावेश "m3_config.h"
#समावेश "wasm3.h"

#समावेश "m3_core.h"
#समावेश "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#समावेश "m3_host.h"

#अगर d_m3HasWin32Host
#  include "m3_host_win32.h"
#या_अगर d_m3HasPosixHost
#  include "m3_host_posix.h"
#अन्यथा
#  include "m3_host_none.h"
#पूर्ण_अगर

व्योम m3_Abort (स्थिर अक्षर* message)
{
#अगर_घोषित DEBUG
    ख_लिखो(मानक_त्रुटि, "Error: %s\n", message);
#पूर्ण_अगर
    पात();
}

M3_WEAK
M3_परिणाम m3_Yield ()
{
    वापस m3Err_none;
}

#अगर d_m3LogTimestamps

#  include <time.h>

#  define SEC_TO_US(sec) ((sec)*1000000)
#  define NS_TO_US(ns)    ((ns)/1000)

अटल uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    अगर (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    काष्ठा timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    वापस us - initial_ts;
}

#पूर्ण_अगर

#अगर d_m3FixedHeap

अटल u8        fixedHeap[d_m3FixedHeap];
अटल u8*       fixedHeapPtr  = fixedHeap;
अटल u8* स्थिर fixedHeapEnd  = fixedHeap + d_m3FixedHeap;
अटल u8*       fixedHeapLast = शून्य;

#  अगर d_m3FixedHeapAlign > 1
#    define HEAP_ALIGN_PTR(P) P = (u8*)(((माप_प्रकार)(P)+(d_m3FixedHeapAlign-1)) & ~ (d_m3FixedHeapAlign-1));
#  अन्यथा
#    define HEAP_ALIGN_PTR(P)
#  endif

व्योम* m3_Malloc_Impl (माप_प्रकार i_आकार)
{
    u8* ptr = fixedHeapPtr;

    fixedHeapPtr += i_आकार;
    HEAP_ALIGN_PTR(fixedHeapPtr);

    अगर (fixedHeapPtr >= fixedHeapEnd) {
        वापस शून्य;
    }

    स_रखो(ptr, 0x0, i_आकार);
    fixedHeapLast = ptr;

    वापस ptr;
}

व्योम m3_Free_Impl (व्योम* i_ptr)
{
    // Handle the last chunk
    अगर (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr  = fixedHeapLast;
        fixedHeapLast = शून्य;
    } अन्यथा {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

व्योम* m3_Realloc_Impl (व्योम* i_ptr, माप_प्रकार i_new_आकार, माप_प्रकार i_old_आकार)
{
    अगर (M3_UNLIKELY(i_new_आकार == i_old_आकार)) {
        वापस i_ptr;
    }

    व्योम* newPtr;

    // Handle the last chunk
    अगर (i_ptr && i_ptr == fixedHeapLast) {
        fixedHeapPtr = fixedHeapLast + i_new_आकार;
        HEAP_ALIGN_PTR(fixedHeapPtr);
        अगर (fixedHeapPtr >= fixedHeapEnd) {
            वापस शून्य;
        }
        newPtr = i_ptr;
    } अन्यथा {
        newPtr = m3_Malloc_Impl(i_new_आकार);
        अगर (!newPtr) {
            वापस शून्य;
        }
        अगर (i_ptr) {
            स_नकल(newPtr, i_ptr, i_old_आकार);
        }
    }

    अगर (i_new_आकार > i_old_आकार) {
        स_रखो((u8*)newPtr + i_old_आकार, 0x0, i_new_आकार - i_old_आकार);
    }

    वापस newPtr;
}

#अन्यथा

व्योम* m3_Malloc_Impl (माप_प्रकार i_आकार)
{
    वापस सुस्मृति(i_आकार, 1);
}

व्योम m3_Free_Impl (व्योम* io_ptr)
{
    मुक्त(io_ptr);
}

व्योम* m3_Realloc_Impl (व्योम* i_ptr, माप_प्रकार i_new_आकार, माप_प्रकार i_old_आकार)
{
    अगर (M3_UNLIKELY(i_new_आकार == i_old_आकार)) {
        वापस i_ptr;
    }

    व्योम* newPtr = पुनः_स्मृति(i_ptr, i_new_आकार);

    अगर (M3_LIKELY(newPtr)) {
        अगर (i_new_आकार > i_old_आकार) {
            स_रखो((u8*)newPtr + i_old_आकार, 0x0, i_new_आकार - i_old_आकार);
        }
        वापस newPtr;
    }
    वापस शून्य;
}

#पूर्ण_अगर

व्योम* m3_CopyMem (स्थिर व्योम* i_from, माप_प्रकार i_आकार)
{
    व्योम* ptr = m3_Malloc("CopyMem", i_आकार);
    अगर (ptr) {
        स_नकल(ptr, i_from, i_आकार);
    }
    वापस ptr;
}

//--------------------------------------------------------------------------------------------

#अगर d_m3GuardedMemory

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
अटल u8*    g_guardArena;
अटल माप_प्रकार g_guardArenaBytes;
अटल माप_प्रकार g_guardSlotBytes;
अटल माप_प्रकार g_guard_Page_आकार;
अटल u32    g_guard_Slot_गिनती;
अटल bool   g_guardSlotTaken[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
अटल
bool Guard_Reserve (व्योम)
{
    अगर (g_guardArena) {
        वापस true;
    }

    माप_प्रकार page_आकार = m3_Host_Page_आकार();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    माप_प्रकार slotBytes = 2 * page_आकार + (माप_प्रकार)d_m3_Guarded_तथ्य_Bytes;

    क्रम (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        व्योम* base = m3_HostReserve(slotBytes * slots);

        अगर (base) {
            g_guardArena      = (u8*)base;
            g_guardArenaBytes = slotBytes * slots;
            g_guardSlotBytes  = slotBytes;
            g_guard_Page_आकार   = page_आकार;
            g_guard_Slot_गिनती  = slots;
            वापस true;
        }
    }

    वापस false;
}

व्योम* Guard_TakeSlot (व्योम)
{
    अगर (not Guard_Reserve()) {
        वापस शून्य;
    }

    क्रम (u32 i = 0; i < g_guard_Slot_गिनती; ++i) {
        अगर (g_guardSlotTaken[i]) {
            जारी;
        }

        u8* slot = g_guardArena + (माप_प्रकार)i * g_guardSlotBytes;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        अगर (not m3_HostCommit(slot, g_guard_Page_आकार)) {
            वापस शून्य;
        }

        g_guardSlotTaken[i] = true;
        वापस slot;
    }

    वापस शून्य;
}

M3MemoryHeader* Guard_SlotHeader (व्योम* i_slot)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* तथ्य = (u8*)i_slot + g_guard_Page_आकार;

    वापस (M3MemoryHeader*)(तथ्य - माप(M3MemoryHeader));
}

bool Guard_CommitSlot (व्योम* i_slot, माप_प्रकार i_तथ्य_Bytes)
{
    माप_प्रकार rounded = (i_तथ्य_Bytes + g_guard_Page_आकार - 1) / g_guard_Page_आकार * g_guard_Page_आकार;

    वापस m3_HostCommit((u8*)i_slot + g_guard_Page_आकार, rounded);
}

व्योम Guard_GiveSlot (व्योम* i_slot)
{
    अगर (i_slot == शून्य) {
        वापस;
    }

    माप_प्रकार index = (माप_प्रकार)((u8*)i_slot - g_guardArena) / g_guardSlotBytes;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    अगर (m3_HostDecommit(i_slot, g_guardSlotBytes) and index < g_guard_Slot_गिनती) {
        g_guardSlotTaken[index] = false;
    }
}

व्योम Guard_ArenaRange (व्योम** o_निचला, माप_प्रकार* o_bytes)
{
    *o_निचला   = g_guardArena;
    *o_bytes = g_guardArenaBytes;
}

#पूर्ण_अगर // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#अगर d_m3_Max_Native_ढेर > 0

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
व्योम* m3_Native_ढेर_Limit (व्योम* i_ढेर_Ptr, माप_प्रकार i_budget)
{
    u8* sp = (u8*)i_ढेर_Ptr;

#  अगर M3_HAS_THREAD_LOCAL
    अटल M3_THREAD_LOCAL u8* base = शून्य;
    अगर (base == शून्य) {
        base = (u8*)m3_Host_ढेर_Base();
    }
#  अन्यथा
    u8* base = (u8*)m3_Host_ढेर_Base();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    अगर (base and base < sp) {
        माप_प्रकार available = (माप_प्रकार)(sp - base);
        माप_प्रकार margin    = M3_MIN((माप_प्रकार)(d_m3_Native_ढेर_Margin), available / 4);

        i_budget = M3_MIN(i_budget, available - margin);
    }

    वापस sp - i_budget;
}

#पूर्ण_अगर // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#अगर d_m3_Log_Native_ढेर

अटल माप_प्रकार ढेर_start;
अटल माप_प्रकार ढेर_end;

व्योम m3_ढेर_Check_Init ()
{
    अक्षर ढेर;
    ढेर_end = ढेर_start = (माप_प्रकार)&ढेर;
}

व्योम m3_ढेर_Check ()
{
    अक्षर   ढेर;
    माप_प्रकार addr = (माप_प्रकार)&ढेर;

    माप_प्रकार ढेर_End = ढेर_end;

    ढेर_end = M3_MIN(ढेर_end, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

पूर्णांक m3_ढेर_Get_Max ()
{
    वापस ढेर_start - ढेर_end;
}

#पूर्ण_अगर

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_परिणाम NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_परिणाम परिणाम = m3Err_none;

    u8 type = -i_convolutedWasmType;

    अगर (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    अन्यथा अगर (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } अन्यथा अगर (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#अगर d_m3HasExceptionHandling
    अन्यथा अगर (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
    अन्यथा अगर (type == d_waType_heap_cont or type == d_waType_heap_nocont) {
        type = c_m3Type_contref;
    }
#पूर्ण_अगर
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    अन्यथा अगर (type < c_m3Type_i32 or type > c_m3Type_v128) {
        परिणाम = m3Err_invalidTypeId;
    }

    *o_type = type;

    वापस परिणाम;
}


#अगर d_m3HasTypedRefs

u8 BaseTypeOf (m3type_t i_type)
{
    अगर (IsSpelledRefType(i_type)) {
        अगर (i_type & d_m3Type_refCont) {
            वापस c_m3Type_contref;
        }
        वापस (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    वापस (u8)i_type;
}


bool IsSubTypeOf (m3type_t i_sub, m3type_t i_super)
{
    अगर (i_sub == i_super) {
        वापस true;
    }

    u8 base = BaseTypeOf(i_sub);

    // only references have a subtype relation; every other type is invariant
    अगर (base != BaseTypeOf(i_super) or not IsRefType(base)) {
        वापस false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    अगर (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        वापस false;
    }

#  अगर d_m3_Has_ढेर_Switching
    // nocont sits under every continuation type
    अगर (HeapTypeOf(i_sub) == d_m3Type_heapNone) {
        वापस true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    वापस (HeapTypeOf(i_super) == d_m3Type_heapAbstract) or
           (HeapTypeOf(i_sub) == HeapTypeOf(i_super));
}

#पूर्ण_अगर // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    वापस (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    वापस (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#अगर d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#पूर्ण_अगर
#अगर d_m3_Has_ढेर_Switching
            or i_m3Type == c_m3Type_contref
#पूर्ण_अगर
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);
    वापस (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64BitType (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    अगर (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        वापस true;
    } अन्यथा अगर (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        वापस false;
    } अन्यथा {
        वापस (माप(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 आकार_Of_Type (m3type_t i_type)
{
    u8 i_m3Type = BaseTypeOf(i_type);

    अगर (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        वापस माप(i32);
    }

    अगर (IsRefType(i_m3Type)) {
        वापस माप(व्योम*);
    }

    वापस माप(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_परिणाम Read_u64 (u64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    स्थिर u8* ptr = *io_bytes;
    ptr += माप(u64);

    अगर (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, माप(u64));
        M3_BSWAP_u64(*o_मूल्य);
        *io_bytes = ptr;
        वापस m3Err_none;
    } अन्यथा {
        वापस m3Err_wasmUnderrun;
    }
}


M3_परिणाम Read_u32 (u32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    स्थिर u8* ptr = *io_bytes;
    ptr += माप(u32);

    अगर (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, माप(u32));
        M3_BSWAP_u32(*o_मूल्य);
        *io_bytes = ptr;
        वापस m3Err_none;
    } अन्यथा {
        वापस m3Err_wasmUnderrun;
    }
}

#अगर d_m3ImplementFloat

M3_परिणाम Read_f64 (f64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    स्थिर u8* ptr = *io_bytes;
    ptr += माप(f64);

    अगर (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, माप(f64));
        M3_BSWAP_f64(*o_मूल्य);
        *io_bytes = ptr;
        वापस m3Err_none;
    } अन्यथा {
        वापस m3Err_wasmUnderrun;
    }
}


M3_परिणाम Read_f32 (f32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    स्थिर u8* ptr = *io_bytes;
    ptr += माप(f32);

    अगर (M3_LIKELY(ptr <= i_end)) {
        स_नकल(o_मूल्य, *io_bytes, माप(f32));
        M3_BSWAP_f32(*o_मूल्य);
        *io_bytes = ptr;
        वापस m3Err_none;
    } अन्यथा {
        वापस m3Err_wasmUnderrun;
    }
}

#पूर्ण_अगर

M3_परिणाम Read_u8 (u8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    स्थिर u8* ptr = *io_bytes;

    अगर (M3_LIKELY(ptr < i_end)) {
        *o_मूल्य  = *ptr;
        *io_bytes = ptr + 1;

        वापस m3Err_none;
    } अन्यथा {
        वापस m3Err_wasmUnderrun;
    }
}

M3_परिणाम Read_opcode (m3opcode_t* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    स्थिर u8* ptr = *io_bytes;

    अगर (M3_LIKELY(ptr < i_end)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_मूल्य  = opcode;
        *io_bytes = ptr;

        वापस m3Err_none;
    } अन्यथा {
        वापस m3Err_wasmUnderrun;
    }
}


M3_परिणाम ReadLebUnsigned (u64* o_मूल्य, u32 i_max_अंक_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_wasmUnderrun;

    u64 मूल्य = 0;

    u32       shift = 0;
    स्थिर u8* ptr   = *io_bytes;

    जबतक (ptr < i_end) {
        u64 byte = *(ptr++);

        मूल्य |= ((byte & 0x7f) << shift);
        shift += 7;

        अगर (M3_LIKELY((byte & 0x80) == 0)) {
            परिणाम = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            अगर (M3_UNLIKELY(shift > i_max_अंक_Bits)) {
                u32 अंक_Used_Bits = i_max_अंक_Bits + 7 - shift;

                अगर (byte >> अंक_Used_Bits) {
                    परिणाम = m3Err_lebOverflow;
                }
            }
            अवरोध;
        }

        अगर (M3_UNLIKELY(shift >= i_max_अंक_Bits)) {
            परिणाम = m3Err_lebOverflow;
            अवरोध;
        }
    }

    *o_मूल्य  = मूल्य;
    *io_bytes = ptr;

    वापस परिणाम;
}


M3_परिणाम ReadLebSigned (i64* o_मूल्य, u32 i_max_अंक_Bits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_wasmUnderrun;

    i64 मूल्य = 0;

    u32       shift = 0;
    स्थिर u8* ptr   = *io_bytes;

    जबतक (ptr < i_end) {
        u64 byte = *(ptr++);

        मूल्य |= ((byte & 0x7f) << shift);
        shift += 7;

        अगर (M3_LIKELY((byte & 0x80) == 0)) {
            परिणाम = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            अगर (M3_UNLIKELY(shift > i_max_अंक_Bits)) {
                u32 अंक_Used_Bits = i_max_अंक_Bits + 7 - shift;
                u8  signBits    = (u8)((0x7f << (अंक_Used_Bits - 1)) & 0x7f);
                u8  bits        = (u8)(byte & signBits);

                अगर (bits != 0 and bits != signBits) {
                    परिणाम = m3Err_lebOverflow;
                }
            }
            अगर ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                मूल्य |= (~extend << shift);
            }

            अवरोध;
        }

        अगर (M3_UNLIKELY(shift >= i_max_अंक_Bits)) {
            परिणाम = m3Err_lebOverflow;
            अवरोध;
        }
    }

    *o_मूल्य  = मूल्य;
    *io_bytes = ptr;

    वापस परिणाम;
}


M3_परिणाम ReadLEB_u32 (u32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      मूल्य;
    M3_परिणाम परिणाम = ReadLebUnsigned(&मूल्य, 32, io_bytes, i_end);

    *o_मूल्य = (u32)मूल्य;

    वापस परिणाम;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#घोषणा d_memArgHasMemoryIdx    0x40u

M3_परिणाम ReadMemoryArg (u32* o_align, u32* o_memoryIdx, u64* o_offset, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम;

    u32 align;
    परिणाम = ReadLEB_u32(&align, io_bytes, i_end);
    अगर (परिणाम) {
        वापस परिणाम;
    }

    *o_memoryIdx = 0;

#अगर d_m3HasMultiMemory
    अगर (align & d_memArgHasMemoryIdx) {
        align &= ~d_memArgHasMemoryIdx;

        परिणाम = ReadLEB_u32(o_memoryIdx, io_bytes, i_end);
        अगर (परिणाम) {
            वापस परिणाम;
        }
    }
#पूर्ण_अगर

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    वापस ReadLebUnsigned(o_offset, 64, io_bytes, i_end);
}


M3_परिणाम ReadLEB_u7 (u8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    u64      मूल्य;
    M3_परिणाम परिणाम = ReadLebUnsigned(&मूल्य, 7, io_bytes, i_end);

    *o_मूल्य = (u8)मूल्य;

    वापस परिणाम;
}


M3_परिणाम ReadLEB_i7 (i8* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मूल्य;
    M3_परिणाम परिणाम = ReadLebSigned(&मूल्य, 7, io_bytes, i_end);

    *o_मूल्य = (i8)मूल्य;

    वापस परिणाम;
}


M3_परिणाम ReadLEB_i32 (i32* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मूल्य;
    M3_परिणाम परिणाम = ReadLebSigned(&मूल्य, 32, io_bytes, i_end);

    *o_मूल्य = (i32)मूल्य;

    वापस परिणाम;
}


M3_परिणाम ReadLEB_i64 (i64* o_मूल्य, bytes_t* io_bytes, cbytes_t i_end)
{
    i64      मूल्य;
    M3_परिणाम परिणाम = ReadLebSigned(&मूल्य, 64, io_bytes, i_end);

    *o_मूल्य = मूल्य;

    वापस परिणाम;
}

#अगर d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
अटल
bool IsValidUtf8 (स्थिर u8* i_तथ्य, u32 i_लंबाई)
{
    स्थिर u8* ptr = i_तथ्य;
    स्थिर u8* end = i_तथ्य + i_लंबाई;

    जबतक (ptr < end) {
        u8 b0 = *ptr++;

        अगर (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            जारी;
        } अन्यथा अगर ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            अगर (b0 < 0xC2) {
                वापस false;       // overlong
            }
            अगर (ptr >= end) {
                वापस false;
            }
            u8 b1 = *ptr++;
            अगर ((b1 & 0xC0) != 0x80) {
                वापस false;
            }
        } अन्यथा अगर ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            अगर (ptr + 1 >= end) {
                वापस false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            अगर ((b1 & 0xC0) != 0x80) {
                वापस false;
            }
            अगर ((b2 & 0xC0) != 0x80) {
                वापस false;
            }
            // reject overlong
            अगर (b0 == 0xE0 && b1 < 0xA0) {
                वापस false;
            }
            // reject surrogates U+D800..U+DFFF
            अगर (b0 == 0xED && b1 >= 0xA0) {
                वापस false;
            }
        } अन्यथा अगर ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            अगर (b0 > 0xF4) {
                वापस false;       // above U+10FFFF
            }
            अगर (ptr + 2 >= end) {
                वापस false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            अगर ((b1 & 0xC0) != 0x80) {
                वापस false;
            }
            अगर ((b2 & 0xC0) != 0x80) {
                वापस false;
            }
            अगर ((b3 & 0xC0) != 0x80) {
                वापस false;
            }
            // reject overlong
            अगर (b0 == 0xF0 && b1 < 0x90) {
                वापस false;
            }
            // reject above U+10FFFF
            अगर (b0 == 0xF4 && b1 > 0x8F) {
                वापस false;
            }
        } अन्यथा {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            वापस false;
        }
    }

    वापस true;
}
#पूर्ण_अगर // d_m3EnableValidation


M3_परिणाम Read_utf8 (cstr_t* o_utf8, bytes_t* io_bytes, cbytes_t i_end)
{
    *o_utf8 = शून्य;

    u32      utf8_लंबाई;
    M3_परिणाम परिणाम = ReadLEB_u32(&utf8_लंबाई, io_bytes, i_end);

    अगर (not परिणाम) {
        अगर (utf8_लंबाई <= d_m3_Max_Sane_Utf8_लंबाई) {
            स्थिर u8* ptr = *io_bytes;
            अगर (utf8_लंबाई <= (माप_प्रकार)(i_end - ptr)) {
                स्थिर u8* end = ptr + utf8_लंबाई;
#अगर d_m3EnableValidation
                अगर (not IsValidUtf8(ptr, utf8_लंबाई)) {
                    *io_bytes = end;
                    वापस m3Err_wasmMalformed;
                }
#पूर्ण_अगर // d_m3EnableValidation

                अक्षर* utf8 = (अक्षर*)m3_Malloc("UTF8", utf8_लंबाई + 1);

                अगर (utf8) {
                    स_नकल(utf8, ptr, utf8_लंबाई);
                    utf8[utf8_लंबाई] = 0;

                    *o_utf8 = utf8;
                } अन्यथा {
                    परिणाम = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_bytes = end;
            } अन्यथा {
                परिणाम = m3Err_wasmUnderrun;
            }
        } अन्यथा {
            परिणाम = m3Err_missingUTF8;
        }
    }

    वापस परिणाम;
}

#अगर d_m3RecordBacktraces
u32 FindModuleOffset (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3CodePage curr = i_runtime->pagesOpen;

    bool pageFound = false;

    जबतक (curr) {
        अगर (ContainsPC(curr, i_pc)) {
            pageFound = true;
            अवरोध;
        }
        curr = curr->info.अग्र;
    }

    अगर (!pageFound) {
        curr = i_runtime->pagesFull;
        जबतक (curr) {
            अगर (ContainsPC(curr, i_pc)) {
                pageFound = true;
                अवरोध;
            }
            curr = curr->info.अग्र;
        }
    }

    अगर (pageFound) {
        u32 परिणाम = 0;

        bool pcFound = MapPCToOffset(curr, i_pc, &परिणाम);
        (व्योम)pcFound;                                                          d_m3Assert (pcFound);

        वापस परिणाम;
    } अन्यथा {
        वापस 0;
    }
}


व्योम धकेलो_Backtrace_Frame (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    अगर (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        वापस;
    }

    M3BacktraceFrame* newFrame = m3_AllocStruct(M3BacktraceFrame);

    अगर (!newFrame) {
        io_runtime->backtrace.lastFrame = M3_BACKTRACE_TRUNCATED;
        वापस;
    }

    newFrame->moduleOffset = FindModuleOffset(io_runtime, i_pc);

    अगर (!io_runtime->backtrace.frames || !io_runtime->backtrace.lastFrame) {
        io_runtime->backtrace.frames = newFrame;
    } अन्यथा {
        io_runtime->backtrace.lastFrame->अग्र = newFrame;
    }
    io_runtime->backtrace.lastFrame = newFrame;
}


व्योम FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    अगर (M3_UNLIKELY(io_runtime->backtrace.lastFrame == M3_BACKTRACE_TRUNCATED)) {
        वापस;
    }

    अगर (!io_runtime->backtrace.lastFrame) {
        वापस;
    }

    io_runtime->backtrace.lastFrame->function = i_function;
}


व्योम ClearBacktrace (IM3Runtime io_runtime)
{
    M3BacktraceFrame* currentFrame = io_runtime->backtrace.frames;
    जबतक (currentFrame) {
        M3BacktraceFrame* अग्र_Frame = currentFrame->अग्र;
        m3_Free(currentFrame);
        currentFrame = अग्र_Frame;
    }

    io_runtime->backtrace.frames    = शून्य;
    io_runtime->backtrace.lastFrame = शून्य;
}
#पूर्ण_अगर // d_m3RecordBacktraces
