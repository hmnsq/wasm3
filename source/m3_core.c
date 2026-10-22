//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#నిర్వచితం_కాకపోతే GNU_ఆధారం
#  define GNU_ఆధారం 1
#అయితే_ముగింపు

#నిర్వచించు M3_IMPLEMENT_ERROR_అక్షరశ్రేణులు
#అన్తర్భావ "m3_config.h"
#అన్తర్భావ "wasm3.h"

#అన్తర్భావ "m3_core.h"
#అన్తర్భావ "m3_env.h"

// The one place any of the m3_host.h implementations is built - see that header
#అన్తర్భావ "m3_host.h"

#ఒకవేళ d_m3HasWin32Host
#  include "m3_host_win32.h"
#లేదావేళ d_m3HasPosixHost
#  include "m3_host_posix.h"
#లేకపోతే
#  include "m3_host_none.h"
#అయితే_ముగింపు

శూన్య m3_Abort (స్థిరరాశి అక్షరం* సందేశం)
{
#నిర్వచితమైతే DEBUG
    fprintf(stderr, "Error: %s\n", సందేశం);
#అయితే_ముగింపు
    abort();
}

M3_WEAK
M3_ఫలితం m3_Yield ()
{
    తిరిగి m3Err_none;
}

#ఒకవేళ d_m3LogTimestamps

#  include <time.h>

#  define SEC_గమ్యం_US(sec) ((sec)*1000000)
#  define NS_గమ్యం_US(ns)    ((ns)/1000)

స్థిర uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    ఒకవేళ (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    నిర్మాణం timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_గమ్యం_US((uint64_t)ts.tv_sec) + NS_గమ్యం_US((uint64_t)ts.tv_nsec);
    తిరిగి us - initial_ts;
}

#అయితే_ముగింపు

#ఒకవేళ d_m3_Fixed_కుప్ప

స్థిర u8        fixed_కుప్ప[d_m3_Fixed_కుప్ప];
స్థిర u8*       fixed_కుప్ప_Ptr  = fixed_కుప్ప;
స్థిర u8* స్థిరరాశి fixed_కుప్ప_చివర  = fixed_కుప్ప + d_m3_Fixed_కుప్ప;
స్థిర u8*       fixed_కుప్ప_చివరి = ఖాళీ;

#  ఒకవేళ d_m3_Fixed_కుప్ప_Align > 1
#    define కుప్ప_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_కుప్ప_Align-1)) & ~ (d_m3_Fixed_కుప్ప_Align-1));
#  లేకపోతే
#    define కుప్ప_ALIGN_PTR(P)
#  endif

శూన్య* m3_Malloc_Impl (size_t i_కొలత)
{
    u8* ptr = fixed_కుప్ప_Ptr;

    fixed_కుప్ప_Ptr += i_కొలత;
    కుప్ప_ALIGN_PTR(fixed_కుప్ప_Ptr);

    ఒకవేళ (fixed_కుప్ప_Ptr >= fixed_కుప్ప_చివర) {
        తిరిగి ఖాళీ;
    }

    memset(ptr, 0x0, i_కొలత);
    fixed_కుప్ప_చివరి = ptr;

    తిరిగి ptr;
}

శూన్య m3_Free_Impl (శూన్య* i_ptr)
{
    // Handle the last chunk
    ఒకవేళ (i_ptr && i_ptr == fixed_కుప్ప_చివరి) {
        fixed_కుప్ప_Ptr  = fixed_కుప్ప_చివరి;
        fixed_కుప్ప_చివరి = ఖాళీ;
    } లేకపోతే {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

శూన్య* m3_Realloc_Impl (శూన్య* i_ptr, size_t i_new_కొలత, size_t i_old_కొలత)
{
    ఒకవేళ (M3_UNLIKELY(i_new_కొలత == i_old_కొలత)) {
        తిరిగి i_ptr;
    }

    శూన్య* newPtr;

    // Handle the last chunk
    ఒకవేళ (i_ptr && i_ptr == fixed_కుప్ప_చివరి) {
        fixed_కుప్ప_Ptr = fixed_కుప్ప_చివరి + i_new_కొలత;
        కుప్ప_ALIGN_PTR(fixed_కుప్ప_Ptr);
        ఒకవేళ (fixed_కుప్ప_Ptr >= fixed_కుప్ప_చివర) {
            తిరిగి ఖాళీ;
        }
        newPtr = i_ptr;
    } లేకపోతే {
        newPtr = m3_Malloc_Impl(i_new_కొలత);
        ఒకవేళ (!newPtr) {
            తిరిగి ఖాళీ;
        }
        ఒకవేళ (i_ptr) {
            జ్ఞాపకం_నకలు(newPtr, i_ptr, i_old_కొలత);
        }
    }

    ఒకవేళ (i_new_కొలత > i_old_కొలత) {
        memset((u8*)newPtr + i_old_కొలత, 0x0, i_new_కొలత - i_old_కొలత);
    }

    తిరిగి newPtr;
}

#లేకపోతే

శూన్య* m3_Malloc_Impl (size_t i_కొలత)
{
    తిరిగి సున్నా_జ్ఞాపకం_ఇవ్వు(i_కొలత, 1);
}

శూన్య m3_Free_Impl (శూన్య* io_ptr)
{
    విడుదల(io_ptr);
}

శూన్య* m3_Realloc_Impl (శూన్య* i_ptr, size_t i_new_కొలత, size_t i_old_కొలత)
{
    ఒకవేళ (M3_UNLIKELY(i_new_కొలత == i_old_కొలత)) {
        తిరిగి i_ptr;
    }

    శూన్య* newPtr = realloc(i_ptr, i_new_కొలత);

    ఒకవేళ (M3_LIKELY(newPtr)) {
        ఒకవేళ (i_new_కొలత > i_old_కొలత) {
            memset((u8*)newPtr + i_old_కొలత, 0x0, i_new_కొలత - i_old_కొలత);
        }
        తిరిగి newPtr;
    }
    తిరిగి ఖాళీ;
}

#అయితే_ముగింపు

శూన్య* m3_CopyMem (స్థిరరాశి శూన్య* i_from, size_t i_కొలత)
{
    శూన్య* ptr = m3_Malloc("CopyMem", i_కొలత);
    ఒకవేళ (ptr) {
        జ్ఞాపకం_నకలు(ptr, i_from, i_కొలత);
    }
    తిరిగి ptr;
}

//--------------------------------------------------------------------------------------------

#ఒకవేళ d_m3_Guarded_జ్ఞాపకం

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
స్థిర u8*    g_guardArena;
స్థిర size_t g_guard_Arena_బైట్లు;
స్థిర size_t g_guard_స్థానం_బైట్లు;
స్థిర size_t g_guard_పుట_కొలత;
స్థిర u32    g_guard_స్థానం_లెక్క;
స్థిర bool   g_guard_స్థానం_తీసుకున్న[d_m3GuardedArenaSlots];

// slot: [ header page ][ data: d_m3GuardedDataBytes ][ a page of slack ]
//
// The slack is what makes the last address an access can name still land inside the
// arena, so the handler recognizes it rather than letting it through as somebody
// else's fault.
స్థిర
bool Guard_Reserve (శూన్య)
{
    ఒకవేళ (g_guardArena) {
        తిరిగి true;
    }

    size_t పుట_కొలత = m3_Host_పుట_కొలత();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t స్థానం_బైట్లు = 2 * పుట_కొలత + (size_t)d_m3_Guarded_దత్తాంశం_బైట్లు;

    కొరకు (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        శూన్య* పునాది = m3_HostReserve(స్థానం_బైట్లు * slots);

        ఒకవేళ (పునాది) {
            g_guardArena      = (u8*)పునాది;
            g_guard_Arena_బైట్లు = స్థానం_బైట్లు * slots;
            g_guard_స్థానం_బైట్లు  = స్థానం_బైట్లు;
            g_guard_పుట_కొలత   = పుట_కొలత;
            g_guard_స్థానం_లెక్క  = slots;
            తిరిగి true;
        }
    }

    తిరిగి false;
}

శూన్య* Guard_Take_స్థానం (శూన్య)
{
    ఒకవేళ (not Guard_Reserve()) {
        తిరిగి ఖాళీ;
    }

    కొరకు (u32 i = 0; i < g_guard_స్థానం_లెక్క; ++i) {
        ఒకవేళ (g_guard_స్థానం_తీసుకున్న[i]) {
            కొనసాగు;
        }

        u8* స్థానం = g_guardArena + (size_t)i * g_guard_స్థానం_బైట్లు;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        ఒకవేళ (not m3_HostCommit(స్థానం, g_guard_పుట_కొలత)) {
            తిరిగి ఖాళీ;
        }

        g_guard_స్థానం_తీసుకున్న[i] = true;
        తిరిగి స్థానం;
    }

    తిరిగి ఖాళీ;
}

M3_జ్ఞాపకం_శీర్షిక* Guard_స్థానం_శీర్షిక (శూన్య* i_స్థానం)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* దత్తాంశం = (u8*)i_స్థానం + g_guard_పుట_కొలత;

    తిరిగి (M3_జ్ఞాపకం_శీర్షిక*)(దత్తాంశం - పరిమాణం(M3_జ్ఞాపకం_శీర్షిక));
}

bool Guard_Commit_స్థానం (శూన్య* i_స్థానం, size_t i_దత్తాంశం_బైట్లు)
{
    size_t rounded = (i_దత్తాంశం_బైట్లు + g_guard_పుట_కొలత - 1) / g_guard_పుట_కొలత * g_guard_పుట_కొలత;

    తిరిగి m3_HostCommit((u8*)i_స్థానం + g_guard_పుట_కొలత, rounded);
}

శూన్య Guard_Give_స్థానం (శూన్య* i_స్థానం)
{
    ఒకవేళ (i_స్థానం == ఖాళీ) {
        తిరిగి;
    }

    size_t index = (size_t)((u8*)i_స్థానం - g_guardArena) / g_guard_స్థానం_బైట్లు;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    ఒకవేళ (m3_HostDecommit(i_స్థానం, g_guard_స్థానం_బైట్లు) and index < g_guard_స్థానం_లెక్క) {
        g_guard_స్థానం_తీసుకున్న[index] = false;
    }
}

శూన్య Guard_ArenaRange (శూన్య** o_దిగువ, size_t* o_బైట్లు)
{
    *o_దిగువ   = g_guardArena;
    *o_బైట్లు = g_guard_Arena_బైట్లు;
}

#అయితే_ముగింపు // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#ఒకవేళ d_m3_Max_Native_దొంతర > 0

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
శూన్య* m3_Native_దొంతర_హద్దు (శూన్య* i_దొంతర_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_దొంతర_Ptr;

#  ఒకవేళ M3_HAS_THREAD_LOCAL
    స్థిర M3_THREAD_LOCAL u8* పునాది = ఖాళీ;
    ఒకవేళ (పునాది == ఖాళీ) {
        పునాది = (u8*)m3_Host_దొంతర_పునాది();
    }
#  లేకపోతే
    u8* పునాది = (u8*)m3_Host_దొంతర_పునాది();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    ఒకవేళ (పునాది and పునాది < sp) {
        size_t అందుబాటు = (size_t)(sp - పునాది);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_దొంతర_Margin), అందుబాటు / 4);

        i_budget = M3_MIN(i_budget, అందుబాటు - margin);
    }

    తిరిగి sp - i_budget;
}

#అయితే_ముగింపు // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#ఒకవేళ d_m3_Log_Native_దొంతర

స్థిర size_t దొంతర_ప్రారంభం;
స్థిర size_t దొంతర_చివర;

శూన్య m3_దొంతర_Check_Init ()
{
    అక్షరం దొంతర;
    దొంతర_చివర = దొంతర_ప్రారంభం = (size_t)&దొంతర;
}

శూన్య m3_దొంతర_Check ()
{
    అక్షరం   దొంతర;
    size_t addr = (size_t)&దొంతర;

    size_t దొంతర_చివర_2 = దొంతర_చివర;

    దొంతర_చివర = M3_MIN(దొంతర_చివర, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

పూర్ణాంకం m3_దొంతర_Get_Max ()
{
    తిరిగి దొంతర_ప్రారంభం - దొంతర_చివర;
}

#అయితే_ముగింపు

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_ఫలితం NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u8 type = -i_convolutedWasmType;

    ఒకవేళ (type == 0x40) {
        type = c_m3Type_none;
    }
    // always recognised: funcref names the element type of an MVP table even
    // when the reference types proposal is compiled out
    లేకపోతే ఒకవేళ (type == d_waType_funcref) {
        type = c_m3Type_funcref;
    } లేకపోతే ఒకవేళ (type == d_waType_externref) {
        type = c_m3Type_externref;
    }
#ఒకవేళ d_m3HasExceptionHandling
    లేకపోతే ఒకవేళ (type == d_waType_exnref) {
        type = c_m3Type_exnref;
    }
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
    లేకపోతే ఒకవేళ (type == d_wa_Type_కుప్ప_cont or type == d_wa_Type_కుప్ప_nocont) {
        type = c_m3Type_contref;
    }
#అయితే_ముగింపు
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    లేకపోతే ఒకవేళ (type < c_m3Type_i32 or type > c_m3Type_v128) {
        ఫలితం = m3Err_invalidTypeId;
    }

    *o_type = type;

    తిరిగి ఫలితం;
}


#ఒకవేళ d_m3HasTypedRefs

u8 పునాది_Type_యొక్క (m3type_t i_type)
{
    ఒకవేళ (IsSpelledRefType(i_type)) {
        ఒకవేళ (i_type & d_m3Type_refCont) {
            తిరిగి c_m3Type_contref;
        }
        తిరిగి (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    తిరిగి (u8)i_type;
}


bool Is_Sub_Type_యొక్క (m3type_t i_sub, m3type_t i_super)
{
    ఒకవేళ (i_sub == i_super) {
        తిరిగి true;
    }

    u8 పునాది = పునాది_Type_యొక్క(i_sub);

    // only references have a subtype relation; every other type is invariant
    ఒకవేళ (పునాది != పునాది_Type_యొక్క(i_super) or not IsRefType(పునాది)) {
        తిరిగి false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    ఒకవేళ (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        తిరిగి false;
    }

#  ఒకవేళ d_m3_Has_దొంతర_Switching
    // nocont sits under every continuation type
    ఒకవేళ (కుప్ప_Type_యొక్క(i_sub) == d_m3_Type_కుప్ప_None) {
        తిరిగి true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    తిరిగి (కుప్ప_Type_యొక్క(i_super) == d_m3_Type_కుప్ప_Abstract) or
           (కుప్ప_Type_యొక్క(i_sub) == కుప్ప_Type_యొక్క(i_super));
}

#అయితే_ముగింపు // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = పునాది_Type_యొక్క(i_type);
    తిరిగి (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = పునాది_Type_యొక్క(i_type);
    తిరిగి (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#ఒకవేళ d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
            or i_m3Type == c_m3Type_contref
#అయితే_ముగింపు
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = పునాది_Type_యొక్క(i_type);
    తిరిగి (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_బిట్టు_Type (m3type_t i_type)
{
    u8 i_m3Type = పునాది_Type_యొక్క(i_type);

    ఒకవేళ (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        తిరిగి true;
    } లేకపోతే ఒకవేళ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        తిరిగి false;
    } లేకపోతే {
        తిరిగి (పరిమాణం(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 కొలత_యొక్క_Type (m3type_t i_type)
{
    u8 i_m3Type = పునాది_Type_యొక్క(i_type);

    ఒకవేళ (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        తిరిగి పరిమాణం(i32);
    }

    ఒకవేళ (IsRefType(i_m3Type)) {
        తిరిగి పరిమాణం(శూన్య*);
    }

    తిరిగి పరిమాణం(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_ఫలితం Read_u64 (u64* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    స్థిరరాశి u8* ptr = *io_బైట్లు;
    ptr += పరిమాణం(u64);

    ఒకవేళ (M3_LIKELY(ptr <= i_చివర)) {
        జ్ఞాపకం_నకలు(o_విలువ, *io_బైట్లు, పరిమాణం(u64));
        M3_BSWAP_u64(*o_విలువ);
        *io_బైట్లు = ptr;
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3Err_wasmUnderrun;
    }
}


M3_ఫలితం Read_u32 (u32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    స్థిరరాశి u8* ptr = *io_బైట్లు;
    ptr += పరిమాణం(u32);

    ఒకవేళ (M3_LIKELY(ptr <= i_చివర)) {
        జ్ఞాపకం_నకలు(o_విలువ, *io_బైట్లు, పరిమాణం(u32));
        M3_BSWAP_u32(*o_విలువ);
        *io_బైట్లు = ptr;
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3Err_wasmUnderrun;
    }
}

#ఒకవేళ d_m3ImplementFloat

M3_ఫలితం Read_f64 (f64* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    స్థిరరాశి u8* ptr = *io_బైట్లు;
    ptr += పరిమాణం(f64);

    ఒకవేళ (M3_LIKELY(ptr <= i_చివర)) {
        జ్ఞాపకం_నకలు(o_విలువ, *io_బైట్లు, పరిమాణం(f64));
        M3_BSWAP_f64(*o_విలువ);
        *io_బైట్లు = ptr;
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3Err_wasmUnderrun;
    }
}


M3_ఫలితం Read_f32 (f32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    స్థిరరాశి u8* ptr = *io_బైట్లు;
    ptr += పరిమాణం(f32);

    ఒకవేళ (M3_LIKELY(ptr <= i_చివర)) {
        జ్ఞాపకం_నకలు(o_విలువ, *io_బైట్లు, పరిమాణం(f32));
        M3_BSWAP_f32(*o_విలువ);
        *io_బైట్లు = ptr;
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3Err_wasmUnderrun;
    }
}

#అయితే_ముగింపు

M3_ఫలితం Read_u8 (u8* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    స్థిరరాశి u8* ptr = *io_బైట్లు;

    ఒకవేళ (M3_LIKELY(ptr < i_చివర)) {
        *o_విలువ  = *ptr;
        *io_బైట్లు = ptr + 1;

        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3Err_wasmUnderrun;
    }
}

M3_ఫలితం Read_opcode (m3opcode_t* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    స్థిరరాశి u8* ptr = *io_బైట్లు;

    ఒకవేళ (M3_LIKELY(ptr < i_చివర)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_విలువ  = opcode;
        *io_బైట్లు = ptr;

        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి m3Err_wasmUnderrun;
    }
}


M3_ఫలితం ReadLebUnsigned (u64* o_విలువ, u32 i_max_అంకె_బిట్లు, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_wasmUnderrun;

    u64 విలువ = 0;

    u32       shift = 0;
    స్థిరరాశి u8* ptr   = *io_బైట్లు;

    అయితే (ptr < i_చివర) {
        u64 byte = *(ptr++);

        విలువ |= ((byte & 0x7f) << shift);
        shift += 7;

        ఒకవేళ (M3_LIKELY((byte & 0x80) == 0)) {
            ఫలితం = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            ఒకవేళ (M3_UNLIKELY(shift > i_max_అంకె_బిట్లు)) {
                u32 అంకె_వాడిన_బిట్లు = i_max_అంకె_బిట్లు + 7 - shift;

                ఒకవేళ (byte >> అంకె_వాడిన_బిట్లు) {
                    ఫలితం = m3_Err_leb_అధికప్రవాహం;
                }
            }
            ఆపు;
        }

        ఒకవేళ (M3_UNLIKELY(shift >= i_max_అంకె_బిట్లు)) {
            ఫలితం = m3_Err_leb_అధికప్రవాహం;
            ఆపు;
        }
    }

    *o_విలువ  = విలువ;
    *io_బైట్లు = ptr;

    తిరిగి ఫలితం;
}


M3_ఫలితం ReadLebSigned (i64* o_విలువ, u32 i_max_అంకె_బిట్లు, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_wasmUnderrun;

    i64 విలువ = 0;

    u32       shift = 0;
    స్థిరరాశి u8* ptr   = *io_బైట్లు;

    అయితే (ptr < i_చివర) {
        u64 byte = *(ptr++);

        విలువ |= ((byte & 0x7f) << shift);
        shift += 7;

        ఒకవేళ (M3_LIKELY((byte & 0x80) == 0)) {
            ఫలితం = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            ఒకవేళ (M3_UNLIKELY(shift > i_max_అంకె_బిట్లు)) {
                u32 అంకె_వాడిన_బిట్లు = i_max_అంకె_బిట్లు + 7 - shift;
                u8  sign_బిట్లు    = (u8)((0x7f << (అంకె_వాడిన_బిట్లు - 1)) & 0x7f);
                u8  బిట్లు        = (u8)(byte & sign_బిట్లు);

                ఒకవేళ (బిట్లు != 0 and బిట్లు != sign_బిట్లు) {
                    ఫలితం = m3_Err_leb_అధికప్రవాహం;
                }
            }
            ఒకవేళ ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                విలువ |= (~extend << shift);
            }

            ఆపు;
        }

        ఒకవేళ (M3_UNLIKELY(shift >= i_max_అంకె_బిట్లు)) {
            ఫలితం = m3_Err_leb_అధికప్రవాహం;
            ఆపు;
        }
    }

    *o_విలువ  = విలువ;
    *io_బైట్లు = ptr;

    తిరిగి ఫలితం;
}


M3_ఫలితం ReadLEB_u32 (u32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    u64      విలువ;
    M3_ఫలితం ఫలితం = ReadLebUnsigned(&విలువ, 32, io_బైట్లు, i_చివర);

    *o_విలువ = (u32)విలువ;

    తిరిగి ఫలితం;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#నిర్వచించు d_mem_Arg_Has_జ్ఞాపకం_Idx    0x40u

M3_ఫలితం Read_జ్ఞాపకం_Arg (u32* o_align, u32* o_జ్ఞాపకం_Idx, u64* o_స్థానభ్రంశం, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం;

    u32 align;
    ఫలితం = ReadLEB_u32(&align, io_బైట్లు, i_చివర);
    ఒకవేళ (ఫలితం) {
        తిరిగి ఫలితం;
    }

    *o_జ్ఞాపకం_Idx = 0;

#ఒకవేళ d_m3_Has_Multi_జ్ఞాపకం
    ఒకవేళ (align & d_mem_Arg_Has_జ్ఞాపకం_Idx) {
        align &= ~d_mem_Arg_Has_జ్ఞాపకం_Idx;

        ఫలితం = ReadLEB_u32(o_జ్ఞాపకం_Idx, io_బైట్లు, i_చివర);
        ఒకవేళ (ఫలితం) {
            తిరిగి ఫలితం;
        }
    }
#అయితే_ముగింపు

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    తిరిగి ReadLebUnsigned(o_స్థానభ్రంశం, 64, io_బైట్లు, i_చివర);
}


M3_ఫలితం ReadLEB_u7 (u8* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    u64      విలువ;
    M3_ఫలితం ఫలితం = ReadLebUnsigned(&విలువ, 7, io_బైట్లు, i_చివర);

    *o_విలువ = (u8)విలువ;

    తిరిగి ఫలితం;
}


M3_ఫలితం ReadLEB_i7 (i8* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    i64      విలువ;
    M3_ఫలితం ఫలితం = ReadLebSigned(&విలువ, 7, io_బైట్లు, i_చివర);

    *o_విలువ = (i8)విలువ;

    తిరిగి ఫలితం;
}


M3_ఫలితం ReadLEB_i32 (i32* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    i64      విలువ;
    M3_ఫలితం ఫలితం = ReadLebSigned(&విలువ, 32, io_బైట్లు, i_చివర);

    *o_విలువ = (i32)విలువ;

    తిరిగి ఫలితం;
}


M3_ఫలితం ReadLEB_i64 (i64* o_విలువ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    i64      విలువ;
    M3_ఫలితం ఫలితం = ReadLebSigned(&విలువ, 64, io_బైట్లు, i_చివర);

    *o_విలువ = విలువ;

    తిరిగి ఫలితం;
}

#ఒకవేళ d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
స్థిర
bool IsValidUtf8 (స్థిరరాశి u8* i_దత్తాంశం, u32 i_నిడివి)
{
    స్థిరరాశి u8* ptr = i_దత్తాంశం;
    స్థిరరాశి u8* చివర = i_దత్తాంశం + i_నిడివి;

    అయితే (ptr < చివర) {
        u8 b0 = *ptr++;

        ఒకవేళ (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            కొనసాగు;
        } లేకపోతే ఒకవేళ ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            ఒకవేళ (b0 < 0xC2) {
                తిరిగి false;       // overlong
            }
            ఒకవేళ (ptr >= చివర) {
                తిరిగి false;
            }
            u8 b1 = *ptr++;
            ఒకవేళ ((b1 & 0xC0) != 0x80) {
                తిరిగి false;
            }
        } లేకపోతే ఒకవేళ ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            ఒకవేళ (ptr + 1 >= చివర) {
                తిరిగి false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            ఒకవేళ ((b1 & 0xC0) != 0x80) {
                తిరిగి false;
            }
            ఒకవేళ ((b2 & 0xC0) != 0x80) {
                తిరిగి false;
            }
            // reject overlong
            ఒకవేళ (b0 == 0xE0 && b1 < 0xA0) {
                తిరిగి false;
            }
            // reject surrogates U+D800..U+DFFF
            ఒకవేళ (b0 == 0xED && b1 >= 0xA0) {
                తిరిగి false;
            }
        } లేకపోతే ఒకవేళ ((b0 & 0xF8) == 0xF0) {
            // four-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            ఒకవేళ (b0 > 0xF4) {
                తిరిగి false;       // above U+10FFFF
            }
            ఒకవేళ (ptr + 2 >= చివర) {
                తిరిగి false;
            }
            u8 b1 = *ptr++;
            u8 b2 = *ptr++;
            u8 b3 = *ptr++;
            ఒకవేళ ((b1 & 0xC0) != 0x80) {
                తిరిగి false;
            }
            ఒకవేళ ((b2 & 0xC0) != 0x80) {
                తిరిగి false;
            }
            ఒకవేళ ((b3 & 0xC0) != 0x80) {
                తిరిగి false;
            }
            // reject overlong
            ఒకవేళ (b0 == 0xF0 && b1 < 0x90) {
                తిరిగి false;
            }
            // reject above U+10FFFF
            ఒకవేళ (b0 == 0xF4 && b1 > 0x8F) {
                తిరిగి false;
            }
        } లేకపోతే {
            // invalid leading byte (0x80..0xBF or 0xF5..0xFF)
            తిరిగి false;
        }
    }

    తిరిగి true;
}
#అయితే_ముగింపు // d_m3EnableValidation


M3_ఫలితం Read_utf8 (cstr_t* o_utf8, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    *o_utf8 = ఖాళీ;

    u32      utf8_నిడివి;
    M3_ఫలితం ఫలితం = ReadLEB_u32(&utf8_నిడివి, io_బైట్లు, i_చివర);

    ఒకవేళ (not ఫలితం) {
        ఒకవేళ (utf8_నిడివి <= d_m3_Max_Sane_Utf8_నిడివి) {
            స్థిరరాశి u8* ptr = *io_బైట్లు;
            ఒకవేళ (utf8_నిడివి <= (size_t)(i_చివర - ptr)) {
                స్థిరరాశి u8* చివర = ptr + utf8_నిడివి;
#ఒకవేళ d_m3EnableValidation
                ఒకవేళ (not IsValidUtf8(ptr, utf8_నిడివి)) {
                    *io_బైట్లు = చివర;
                    తిరిగి m3Err_wasmMalformed;
                }
#అయితే_ముగింపు // d_m3EnableValidation

                అక్షరం* utf8 = (అక్షరం*)m3_Malloc("UTF8", utf8_నిడివి + 1);

                ఒకవేళ (utf8) {
                    జ్ఞాపకం_నకలు(utf8, ptr, utf8_నిడివి);
                    utf8[utf8_నిడివి] = 0;

                    *o_utf8 = utf8;
                } లేకపోతే {
                    ఫలితం = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_బైట్లు = చివర;
            } లేకపోతే {
                ఫలితం = m3Err_wasmUnderrun;
            }
        } లేకపోతే {
            ఫలితం = m3Err_missingUTF8;
        }
    }

    తిరిగి ఫలితం;
}

#ఒకవేళ d_m3_రికార్డు_Backtraces
u32 కనుగొను_Module_స్థానభ్రంశం (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_సంకేతం_పుట curr = i_runtime->పుటలు_Open;

    bool పుట_దొరికింది = false;

    అయితే (curr) {
        ఒకవేళ (కలిగియుంది_PC(curr, i_pc)) {
            పుట_దొరికింది = true;
            ఆపు;
        }
        curr = curr->info.తదుపరి;
    }

    ఒకవేళ (!పుట_దొరికింది) {
        curr = i_runtime->పుటలు_నిండిన;
        అయితే (curr) {
            ఒకవేళ (కలిగియుంది_PC(curr, i_pc)) {
                పుట_దొరికింది = true;
                ఆపు;
            }
            curr = curr->info.తదుపరి;
        }
    }

    ఒకవేళ (పుట_దొరికింది) {
        u32 ఫలితం = 0;

        bool pc_దొరికింది = Map_PCTo_స్థానభ్రంశం(curr, i_pc, &ఫలితం);
        (శూన్య)pc_దొరికింది;                                                          d_m3Assert (pc_దొరికింది);

        తిరిగి ఫలితం;
    } లేకపోతే {
        తిరిగి 0;
    }
}


శూన్య నెట్టు_Backtrace_చట్రం (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    ఒకవేళ (M3_UNLIKELY(io_runtime->backtrace.చివరి_చట్రం == M3_BACKTRACE_TRUNCATED)) {
        తిరిగి;
    }

    M3_Backtrace_చట్రం* new_చట్రం = m3_AllocStruct(M3_Backtrace_చట్రం);

    ఒకవేళ (!new_చట్రం) {
        io_runtime->backtrace.చివరి_చట్రం = M3_BACKTRACE_TRUNCATED;
        తిరిగి;
    }

    new_చట్రం->module_స్థానభ్రంశం = కనుగొను_Module_స్థానభ్రంశం(io_runtime, i_pc);

    ఒకవేళ (!io_runtime->backtrace.చట్రాలు || !io_runtime->backtrace.చివరి_చట్రం) {
        io_runtime->backtrace.చట్రాలు = new_చట్రం;
    } లేకపోతే {
        io_runtime->backtrace.చివరి_చట్రం->తదుపరి = new_చట్రం;
    }
    io_runtime->backtrace.చివరి_చట్రం = new_చట్రం;
}


శూన్య FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    ఒకవేళ (M3_UNLIKELY(io_runtime->backtrace.చివరి_చట్రం == M3_BACKTRACE_TRUNCATED)) {
        తిరిగి;
    }

    ఒకవేళ (!io_runtime->backtrace.చివరి_చట్రం) {
        తిరిగి;
    }

    io_runtime->backtrace.చివరి_చట్రం->function = i_function;
}


శూన్య ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_చట్రం* current_చట్రం = io_runtime->backtrace.చట్రాలు;
    అయితే (current_చట్రం) {
        M3_Backtrace_చట్రం* తదుపరి_చట్రం = current_చట్రం->తదుపరి;
        m3_Free(current_చట్రం);
        current_చట్రం = తదుపరి_చట్రం;
    }

    io_runtime->backtrace.చట్రాలు    = ఖాళీ;
    io_runtime->backtrace.చివరి_చట్రం = ఖాళీ;
}
#అయితే_ముగింపు // d_m3RecordBacktraces
