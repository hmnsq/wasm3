//
//  m3_core.c
//
//  Created by Steven Massey on 4/15/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

// pthread_getattr_np, which m3_host_posix.h asks glibc for. Has to be set before any
// libc header is seen, which is why it is here and not in that file.
#اگر_تعریف_نشده GNU_منبع
#  define GNU_منبع 1
#پایان_اگر

#تعریف_کن M3_IMPLEMENT_ERROR_رشتهها
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
#پایان_اگر

فراغ m3_Abort (ثابت حرف* پیام)
{
#اگر_تعریف_شده DEBUG
    fprintf(stderr, "Error: %s\n", پیام);
#پایان_اگر
    abort();
}

M3_WEAK
M3_نتیجه m3_Yield ()
{
    أرجع m3Err_none;
}

#إذا d_m3LogTimestamps

#  include <time.h>

#  define SEC_مقصد_US(sec) ((sec)*1000000)
#  define NS_مقصد_US(ns)    ((ns)/1000)

ساكن uint64_t initial_ts = -1;

uint64_t m3_GetTimestamp ()
{
    إذا (initial_ts == -1) {
        initial_ts = 0;
        initial_ts = m3_GetTimestamp();
    }
    بنية timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t us = SEC_مقصد_US((uint64_t)ts.tv_sec) + NS_مقصد_US((uint64_t)ts.tv_nsec);
    أرجع us - initial_ts;
}

#پایان_اگر

#إذا d_m3_Fixed_انباشته

ساكن u8        fixed_انباشته[d_m3_Fixed_انباشته];
ساكن u8*       fixed_انباشته_Ptr  = fixed_انباشته;
ساكن u8* ثابت fixed_انباشته_انتها  = fixed_انباشته + d_m3_Fixed_انباشته;
ساكن u8*       fixed_انباشته_آخرین = عدم;

#  إذا d_m3_Fixed_انباشته_Align > 1
#    define انباشته_ALIGN_PTR(P) P = (u8*)(((size_t)(P)+(d_m3_Fixed_انباشته_Align-1)) & ~ (d_m3_Fixed_انباشته_Align-1));
#  وإلا
#    define انباشته_ALIGN_PTR(P)
#  endif

فراغ* m3_Malloc_Impl (size_t i_اندازه)
{
    u8* ptr = fixed_انباشته_Ptr;

    fixed_انباشته_Ptr += i_اندازه;
    انباشته_ALIGN_PTR(fixed_انباشته_Ptr);

    إذا (fixed_انباشته_Ptr >= fixed_انباشته_انتها) {
        أرجع عدم;
    }

    memset(ptr, 0x0, i_اندازه);
    fixed_انباشته_آخرین = ptr;

    أرجع ptr;
}

فراغ m3_Free_Impl (فراغ* i_ptr)
{
    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_انباشته_آخرین) {
        fixed_انباشته_Ptr  = fixed_انباشته_آخرین;
        fixed_انباشته_آخرین = عدم;
    } وإلا {
        //printf("== free %p [failed]\n", io_ptr);
    }
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_اندازه, size_t i_old_اندازه)
{
    إذا (M3_UNLIKELY(i_new_اندازه == i_old_اندازه)) {
        أرجع i_ptr;
    }

    فراغ* newPtr;

    // Handle the last chunk
    إذا (i_ptr && i_ptr == fixed_انباشته_آخرین) {
        fixed_انباشته_Ptr = fixed_انباشته_آخرین + i_new_اندازه;
        انباشته_ALIGN_PTR(fixed_انباشته_Ptr);
        إذا (fixed_انباشته_Ptr >= fixed_انباشته_انتها) {
            أرجع عدم;
        }
        newPtr = i_ptr;
    } وإلا {
        newPtr = m3_Malloc_Impl(i_new_اندازه);
        إذا (!newPtr) {
            أرجع عدم;
        }
        إذا (i_ptr) {
            کپی_حافظه(newPtr, i_ptr, i_old_اندازه);
        }
    }

    إذا (i_new_اندازه > i_old_اندازه) {
        memset((u8*)newPtr + i_old_اندازه, 0x0, i_new_اندازه - i_old_اندازه);
    }

    أرجع newPtr;
}

#وإلا

فراغ* m3_Malloc_Impl (size_t i_اندازه)
{
    أرجع حافظه_صفر_بده(i_اندازه, 1);
}

فراغ m3_Free_Impl (فراغ* io_ptr)
{
    حرر(io_ptr);
}

فراغ* m3_Realloc_Impl (فراغ* i_ptr, size_t i_new_اندازه, size_t i_old_اندازه)
{
    إذا (M3_UNLIKELY(i_new_اندازه == i_old_اندازه)) {
        أرجع i_ptr;
    }

    فراغ* newPtr = realloc(i_ptr, i_new_اندازه);

    إذا (M3_LIKELY(newPtr)) {
        إذا (i_new_اندازه > i_old_اندازه) {
            memset((u8*)newPtr + i_old_اندازه, 0x0, i_new_اندازه - i_old_اندازه);
        }
        أرجع newPtr;
    }
    أرجع عدم;
}

#پایان_اگر

فراغ* m3_CopyMem (ثابت فراغ* i_from, size_t i_اندازه)
{
    فراغ* ptr = m3_Malloc("CopyMem", i_اندازه);
    إذا (ptr) {
        کپی_حافظه(ptr, i_from, i_اندازه);
    }
    أرجع ptr;
}

//--------------------------------------------------------------------------------------------

#إذا d_m3_Guarded_حافظه

// One reservation, cut into equal slots, because a fault has to be recognized by its
// address alone: a signal handler cannot walk a list of scattered reservations, but
// it can compare against one pair of bounds. See m3_HostProtectedCall.
//
// The arena costs address space and nothing else - every page of it starts out
// uncommitted - which is why it is reserved whole on first use rather than grown.
// A system unwilling to hand over that much is met by halving the ask until it is.
ساكن u8*    g_guardArena;
ساكن size_t g_guard_Arena_بایتها;
ساكن size_t g_guard_جایگاه_بایتها;
ساكن size_t g_guard_صفحه_اندازه;
ساكن u32    g_guard_جایگاه_شمار;
ساكن bool   g_guard_جایگاه_گرفته[d_m3GuardedArenaSlots];

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

    size_t صفحه_اندازه = m3_Host_صفحه_اندازه();

    // d_m3GuardedDataBytes is 8GiB, which only fits in a size_t on the 64-bit
    // systems this feature is limited to - see d_m3GuardedMemory
    size_t جایگاه_بایتها = 2 * صفحه_اندازه + (size_t)d_m3_Guarded_داده_بایتها;

    لكل (u32 slots = d_m3GuardedArenaSlots; slots >= 1; slots /= 2) {
        فراغ* پایه = m3_HostReserve(جایگاه_بایتها * slots);

        إذا (پایه) {
            g_guardArena      = (u8*)پایه;
            g_guard_Arena_بایتها = جایگاه_بایتها * slots;
            g_guard_جایگاه_بایتها  = جایگاه_بایتها;
            g_guard_صفحه_اندازه   = صفحه_اندازه;
            g_guard_جایگاه_شمار  = slots;
            أرجع true;
        }
    }

    أرجع false;
}

فراغ* Guard_Take_جایگاه (فراغ)
{
    إذا (not Guard_Reserve()) {
        أرجع عدم;
    }

    لكل (u32 i = 0; i < g_guard_جایگاه_شمار; ++i) {
        إذا (g_guard_جایگاه_گرفته[i]) {
            استمر;
        }

        u8* جایگاه = g_guardArena + (size_t)i * g_guard_جایگاه_بایتها;

        // the header has to be reachable before anything writes it; the data pages
        // wait for Guard_CommitSlot
        إذا (not m3_HostCommit(جایگاه, g_guard_صفحه_اندازه)) {
            أرجع عدم;
        }

        g_guard_جایگاه_گرفته[i] = true;
        أرجع جایگاه;
    }

    أرجع عدم;
}

M3_حافظه_سرآیند* Guard_جایگاه_سرآیند (فراغ* i_جایگاه)
{
    // the guest's bytes begin on the second page, and the header is what sits
    // immediately before them - see M3MemoryHeader
    u8* داده = (u8*)i_جایگاه + g_guard_صفحه_اندازه;

    أرجع (M3_حافظه_سرآیند*)(داده - حجم(M3_حافظه_سرآیند));
}

bool Guard_Commit_جایگاه (فراغ* i_جایگاه, size_t i_داده_بایتها)
{
    size_t rounded = (i_داده_بایتها + g_guard_صفحه_اندازه - 1) / g_guard_صفحه_اندازه * g_guard_صفحه_اندازه;

    أرجع m3_HostCommit((u8*)i_جایگاه + g_guard_صفحه_اندازه, rounded);
}

فراغ Guard_Give_جایگاه (فراغ* i_جایگاه)
{
    إذا (i_جایگاه == عدم) {
        أرجع;
    }

    size_t index = (size_t)((u8*)i_جایگاه - g_guardArena) / g_guard_جایگاه_بایتها;

    // Everything committed in the slot goes, which is what makes the next memory to
    // land here read as zero. A slot that would not give its pages back stays taken:
    // handing it on still holding the last module's bytes is worse than never
    // handing it on at all.
    إذا (m3_HostDecommit(i_جایگاه, g_guard_جایگاه_بایتها) and index < g_guard_جایگاه_شمار) {
        g_guard_جایگاه_گرفته[index] = false;
    }
}

فراغ Guard_ArenaRange (فراغ** o_کم, size_t* o_بایتها)
{
    *o_کم   = g_guardArena;
    *o_بایتها = g_guard_Arena_بایتها;
}

#پایان_اگر // d_m3GuardedMemory

//--------------------------------------------------------------------------------------------

#إذا d_m3_Max_Native_پشته > 0

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
فراغ* m3_Native_پشته_حد (فراغ* i_پشته_Ptr, size_t i_budget)
{
    u8* sp = (u8*)i_پشته_Ptr;

#  إذا M3_HAS_THREAD_LOCAL
    ساكن M3_THREAD_LOCAL u8* پایه = عدم;
    إذا (پایه == عدم) {
        پایه = (u8*)m3_Host_پشته_پایه();
    }
#  وإلا
    u8* پایه = (u8*)m3_Host_پشته_پایه();
#  endif

    // A base at or above the stack pointer is not this thread's stack: an
    // unrecognized answer, not a stack with nothing left in it
    إذا (پایه and پایه < sp) {
        size_t موجود = (size_t)(sp - پایه);
        size_t margin    = M3_MIN((size_t)(d_m3_Native_پشته_Margin), موجود / 4);

        i_budget = M3_MIN(i_budget, موجود - margin);
    }

    أرجع sp - i_budget;
}

#پایان_اگر // d_m3MaxNativeStack > 0

//--------------------------------------------------------------------------------------------

#إذا d_m3_Log_Native_پشته

ساكن size_t پشته_شروع;
ساكن size_t پشته_انتها;

فراغ m3_پشته_Check_Init ()
{
    حرف پشته;
    پشته_انتها = پشته_شروع = (size_t)&پشته;
}

فراغ m3_پشته_Check ()
{
    حرف   پشته;
    size_t addr = (size_t)&پشته;

    size_t پشته_انتها_2 = پشته_انتها;

    پشته_انتها = M3_MIN(پشته_انتها, addr);

    //    if (stackEnd != stack_end)
    //        printf ("maxStack: %ld\n", m3StackGetMax ());
}

صحيح m3_پشته_Get_Max ()
{
    أرجع پشته_شروع - پشته_انتها;
}

#پایان_اگر

//--------------------------------------------------------------------------------------------

// Value types arrive as signed 7-bit LEBs, so the encoding negates: 0x7f (i32)
// reads as -1, 0x7b (v128) as -5, 0x70 (funcref) as -16, 0x6f (externref) as -17.
M3_نتیجه NormalizeType (u8* o_type, i8 i_convolutedWasmType)
{
    M3_نتیجه نتیجه = m3Err_none;

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
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
    وإلا إذا (type == d_wa_Type_انباشته_cont or type == d_wa_Type_انباشته_nocont) {
        type = c_m3Type_contref;
    }
#پایان_اگر
    // Accept v128 (wasm-encoded as 0x7b → -i_convolutedWasmType == 5)
    // as an opaque slot so modules with v128 in signatures or local
    // declarations parse. Actual v128 opcodes still hit
    // m3Err_unknownOpcode at compile time - we just stop refusing
    // unused SIMD slots that auto-vectorization emits.
    وإلا إذا (type < c_m3Type_i32 or type > c_m3Type_v128) {
        نتیجه = m3Err_invalidTypeId;
    }

    *o_type = type;

    أرجع نتیجه;
}


#إذا d_m3HasTypedRefs

u8 پایه_Type_از (m3type_t i_type)
{
    إذا (IsSpelledRefType(i_type)) {
        إذا (i_type & d_m3Type_refCont) {
            أرجع c_m3Type_contref;
        }
        أرجع (i_type & d_m3Type_refExtern) ? c_m3Type_externref : c_m3Type_funcref;
    }

    أرجع (u8)i_type;
}


bool Is_Sub_Type_از (m3type_t i_sub, m3type_t i_super)
{
    إذا (i_sub == i_super) {
        أرجع true;
    }

    u8 پایه = پایه_Type_از(i_sub);

    // only references have a subtype relation; every other type is invariant
    إذا (پایه != پایه_Type_از(i_super) or not IsRefType(پایه)) {
        أرجع false;
    }

    // (ref ht) <: (ref null ht), never the other way around
    إذا (IsNullableRef(i_sub) and not IsNullableRef(i_super)) {
        أرجع false;
    }

#  إذا d_m3_Has_پشته_Switching
    // nocont sits under every continuation type
    إذا (انباشته_Type_از(i_sub) == d_m3_Type_انباشته_None) {
        أرجع true;
    }
#  endif

    // $t <: func, and function types are invariant among themselves, so a
    // concrete heap type matches only itself or the abstract one. Indices are
    // canonical, so structurally equal types compare equal here.
    أرجع (انباشته_Type_از(i_super) == d_m3_Type_انباشته_Abstract) or
           (انباشته_Type_از(i_sub) == انباشته_Type_از(i_super));
}

#پایان_اگر // d_m3HasTypedRefs


bool IsFpType (m3type_t i_type)
{
    u8 i_m3Type = پایه_Type_از(i_type);
    أرجع (i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_f64);
}


bool IsRefType (m3type_t i_type)
{
    u8 i_m3Type = پایه_Type_از(i_type);
    أرجع (i_m3Type == c_m3Type_funcref or i_m3Type == c_m3Type_externref
#إذا d_m3HasExceptionHandling
            or i_m3Type == c_m3Type_exnref
#پایان_اگر
#إذا d_m3_Has_پشته_Switching
            or i_m3Type == c_m3Type_contref
#پایان_اگر
    );
}


bool IsIntType (m3type_t i_type)
{
    u8 i_m3Type = پایه_Type_از(i_type);
    أرجع (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_i64);
}


bool Is64_بیت_Type (m3type_t i_type)
{
    u8 i_m3Type = پایه_Type_از(i_type);

    إذا (i_m3Type == c_m3Type_i64 or i_m3Type == c_m3Type_f64) {
        أرجع true;
    } وإلا إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32 or i_m3Type == c_m3Type_none) {
        أرجع false;
    } وإلا {
        أرجع (حجم(voidptr_t) == 8); // all other cases are pointers
    }
}

u32 اندازه_از_Type (m3type_t i_type)
{
    u8 i_m3Type = پایه_Type_از(i_type);

    إذا (i_m3Type == c_m3Type_i32 or i_m3Type == c_m3Type_f32) {
        أرجع حجم(i32);
    }

    إذا (IsRefType(i_m3Type)) {
        أرجع حجم(فراغ*);
    }

    أرجع حجم(i64);
}


//-- Binary Wasm parsing utils  ------------------------------------------------------------------------------------------


M3_نتیجه Read_u64 (u64* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    ثابت u8* ptr = *io_بایتها;
    ptr += حجم(u64);

    إذا (M3_LIKELY(ptr <= i_انتها)) {
        کپی_حافظه(o_مقدار, *io_بایتها, حجم(u64));
        M3_BSWAP_u64(*o_مقدار);
        *io_بایتها = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتیجه Read_u32 (u32* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    ثابت u8* ptr = *io_بایتها;
    ptr += حجم(u32);

    إذا (M3_LIKELY(ptr <= i_انتها)) {
        کپی_حافظه(o_مقدار, *io_بایتها, حجم(u32));
        M3_BSWAP_u32(*o_مقدار);
        *io_بایتها = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#إذا d_m3ImplementFloat

M3_نتیجه Read_f64 (f64* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    ثابت u8* ptr = *io_بایتها;
    ptr += حجم(f64);

    إذا (M3_LIKELY(ptr <= i_انتها)) {
        کپی_حافظه(o_مقدار, *io_بایتها, حجم(f64));
        M3_BSWAP_f64(*o_مقدار);
        *io_بایتها = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتیجه Read_f32 (f32* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    ثابت u8* ptr = *io_بایتها;
    ptr += حجم(f32);

    إذا (M3_LIKELY(ptr <= i_انتها)) {
        کپی_حافظه(o_مقدار, *io_بایتها, حجم(f32));
        M3_BSWAP_f32(*o_مقدار);
        *io_بایتها = ptr;
        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

#پایان_اگر

M3_نتیجه Read_u8 (u8* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    ثابت u8* ptr = *io_بایتها;

    إذا (M3_LIKELY(ptr < i_انتها)) {
        *o_مقدار  = *ptr;
        *io_بایتها = ptr + 1;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}

M3_نتیجه Read_opcode (m3opcode_t* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    ثابت u8* ptr = *io_بایتها;

    إذا (M3_LIKELY(ptr < i_انتها)) {
        // 0xFC is returned as the bare prefix: its sub-opcode is a LEB128 u32, which
        // only the callers - Compile_ExtendedOpcode and the validator - can read.
        m3opcode_t opcode = *ptr++;

        *o_مقدار  = opcode;
        *io_بایتها = ptr;

        أرجع m3Err_none;
    } وإلا {
        أرجع m3Err_wasmUnderrun;
    }
}


M3_نتیجه ReadLebUnsigned (u64* o_مقدار, u32 i_max_رقم_بیتها, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_wasmUnderrun;

    u64 مقدار = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بایتها;

    طالما (ptr < i_انتها) {
        u64 byte = *(ptr++);

        مقدار |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            نتیجه = m3Err_none;

            // The last byte must not carry bits past i_maxNumBits
            إذا (M3_UNLIKELY(shift > i_max_رقم_بیتها)) {
                u32 رقم_استفاده_شده_بیتها = i_max_رقم_بیتها + 7 - shift;

                إذا (byte >> رقم_استفاده_شده_بیتها) {
                    نتیجه = m3_Err_leb_سرریز;
                }
            }
            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_رقم_بیتها)) {
            نتیجه = m3_Err_leb_سرریز;
            اكسر;
        }
    }

    *o_مقدار  = مقدار;
    *io_بایتها = ptr;

    أرجع نتیجه;
}


M3_نتیجه ReadLebSigned (i64* o_مقدار, u32 i_max_رقم_بیتها, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_wasmUnderrun;

    i64 مقدار = 0;

    u32       shift = 0;
    ثابت u8* ptr   = *io_بایتها;

    طالما (ptr < i_انتها) {
        u64 byte = *(ptr++);

        مقدار |= ((byte & 0x7f) << shift);
        shift += 7;

        إذا (M3_LIKELY((byte & 0x80) == 0)) {
            نتیجه = m3Err_none;

            // The bits of the last byte past i_maxNumBits must all repeat the
            // sign bit, otherwise the value doesn't fit
            إذا (M3_UNLIKELY(shift > i_max_رقم_بیتها)) {
                u32 رقم_استفاده_شده_بیتها = i_max_رقم_بیتها + 7 - shift;
                u8  sign_بیتها    = (u8)((0x7f << (رقم_استفاده_شده_بیتها - 1)) & 0x7f);
                u8  بیتها        = (u8)(byte & sign_بیتها);

                إذا (بیتها != 0 and بیتها != sign_بیتها) {
                    نتیجه = m3_Err_leb_سرریز;
                }
            }
            إذا ((byte & 0x40) and (shift < 64))    // do sign extension
            {
                u64 extend = 0;
                مقدار |= (~extend << shift);
            }

            اكسر;
        }

        إذا (M3_UNLIKELY(shift >= i_max_رقم_بیتها)) {
            نتیجه = m3_Err_leb_سرریز;
            اكسر;
        }
    }

    *o_مقدار  = مقدار;
    *io_بایتها = ptr;

    أرجع نتیجه;
}


M3_نتیجه ReadLEB_u32 (u32* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    u64      مقدار;
    M3_نتیجه نتیجه = ReadLebUnsigned(&مقدار, 32, io_بایتها, i_انتها);

    *o_مقدار = (u32)مقدار;

    أرجع نتیجه;
}


// Bit 6 of the alignment is the flag; it is a bit of the first LEB byte, so
// testing the decoded value for it is the same test.
#تعریف_کن d_mem_Arg_Has_حافظه_Idx    0x40u

M3_نتیجه Read_حافظه_Arg (u32* o_align, u32* o_حافظه_Idx, u64* o_جابه_جایی, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه;

    u32 align;
    نتیجه = ReadLEB_u32(&align, io_بایتها, i_انتها);
    إذا (نتیجه) {
        أرجع نتیجه;
    }

    *o_حافظه_Idx = 0;

#إذا d_m3_Has_Multi_حافظه
    إذا (align & d_mem_Arg_Has_حافظه_Idx) {
        align &= ~d_mem_Arg_Has_حافظه_Idx;

        نتیجه = ReadLEB_u32(o_حافظه_Idx, io_بایتها, i_انتها);
        إذا (نتیجه) {
            أرجع نتیجه;
        }
    }
#پایان_اگر

    *o_align = align;

    // memory64 widens the offset to a u64. Whether it is in range is a
    // question about the memory it addresses, so the caller checks it.
    أرجع ReadLebUnsigned(o_جابه_جایی, 64, io_بایتها, i_انتها);
}


M3_نتیجه ReadLEB_u7 (u8* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    u64      مقدار;
    M3_نتیجه نتیجه = ReadLebUnsigned(&مقدار, 7, io_بایتها, i_انتها);

    *o_مقدار = (u8)مقدار;

    أرجع نتیجه;
}


M3_نتیجه ReadLEB_i7 (i8* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    i64      مقدار;
    M3_نتیجه نتیجه = ReadLebSigned(&مقدار, 7, io_بایتها, i_انتها);

    *o_مقدار = (i8)مقدار;

    أرجع نتیجه;
}


M3_نتیجه ReadLEB_i32 (i32* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    i64      مقدار;
    M3_نتیجه نتیجه = ReadLebSigned(&مقدار, 32, io_بایتها, i_انتها);

    *o_مقدار = (i32)مقدار;

    أرجع نتیجه;
}


M3_نتیجه ReadLEB_i64 (i64* o_مقدار, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    i64      مقدار;
    M3_نتیجه نتیجه = ReadLebSigned(&مقدار, 64, io_بایتها, i_انتها);

    *o_مقدار = مقدار;

    أرجع نتیجه;
}

#إذا d_m3EnableValidation
// Validate that a byte sequence is well-formed UTF-8 per the Unicode spec.
// Returns true if valid, false otherwise.
ساكن
bool IsValidUtf8 (ثابت u8* i_داده, u32 i_درازی)
{
    ثابت u8* ptr = i_داده;
    ثابت u8* انتها = i_داده + i_درازی;

    طالما (ptr < انتها) {
        u8 b0 = *ptr++;

        إذا (b0 < 0x80) {
            // single-byte: 0xxxxxxx
            استمر;
        } وإلا إذا ((b0 & 0xE0) == 0xC0) {
            // two-byte: 110xxxxx 10xxxxxx
            إذا (b0 < 0xC2) {
                أرجع false;       // overlong
            }
            إذا (ptr >= انتها) {
                أرجع false;
            }
            u8 b1 = *ptr++;
            إذا ((b1 & 0xC0) != 0x80) {
                أرجع false;
            }
        } وإلا إذا ((b0 & 0xF0) == 0xE0) {
            // three-byte: 1110xxxx 10xxxxxx 10xxxxxx
            إذا (ptr + 1 >= انتها) {
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
            إذا (ptr + 2 >= انتها) {
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
#پایان_اگر // d_m3EnableValidation


M3_نتیجه Read_utf8 (cstr_t* o_utf8, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    *o_utf8 = عدم;

    u32      utf8_درازی;
    M3_نتیجه نتیجه = ReadLEB_u32(&utf8_درازی, io_بایتها, i_انتها);

    إذا (not نتیجه) {
        إذا (utf8_درازی <= d_m3_Max_Sane_Utf8_درازی) {
            ثابت u8* ptr = *io_بایتها;
            إذا (utf8_درازی <= (size_t)(i_انتها - ptr)) {
                ثابت u8* انتها = ptr + utf8_درازی;
#إذا d_m3EnableValidation
                إذا (not IsValidUtf8(ptr, utf8_درازی)) {
                    *io_بایتها = انتها;
                    أرجع m3Err_wasmMalformed;
                }
#پایان_اگر // d_m3EnableValidation

                حرف* utf8 = (حرف*)m3_Malloc("UTF8", utf8_درازی + 1);

                إذا (utf8) {
                    کپی_حافظه(utf8, ptr, utf8_درازی);
                    utf8[utf8_درازی] = 0;

                    *o_utf8 = utf8;
                } وإلا {
                    نتیجه = m3Err_mallocFailed;   // callers dereference the name; don't hand back NULL as success
                }

                *io_بایتها = انتها;
            } وإلا {
                نتیجه = m3Err_wasmUnderrun;
            }
        } وإلا {
            نتیجه = m3Err_missingUTF8;
        }
    }

    أرجع نتیجه;
}

#إذا d_m3_رکورد_Backtraces
u32 پیدا_کن_Module_جابه_جایی (IM3Runtime i_runtime, pc_t i_pc)
{
    // walk the code pages
    IM3_کد_صفحه curr = i_runtime->صفحات_Open;

    bool صفحه_یافت = false;

    طالما (curr) {
        إذا (شامل_است_PC(curr, i_pc)) {
            صفحه_یافت = true;
            اكسر;
        }
        curr = curr->info.بعدی;
    }

    إذا (!صفحه_یافت) {
        curr = i_runtime->صفحات_پر;
        طالما (curr) {
            إذا (شامل_است_PC(curr, i_pc)) {
                صفحه_یافت = true;
                اكسر;
            }
            curr = curr->info.بعدی;
        }
    }

    إذا (صفحه_یافت) {
        u32 نتیجه = 0;

        bool pc_یافت = Map_PCTo_جابه_جایی(curr, i_pc, &نتیجه);
        (فراغ)pc_یافت;                                                          d_m3Assert (pc_یافت);

        أرجع نتیجه;
    } وإلا {
        أرجع 0;
    }
}


فراغ افزودن_Backtrace_قاب (IM3Runtime io_runtime, pc_t i_pc)
{
    // don't try to push any more frames if we've already had an alloc failure
    إذا (M3_UNLIKELY(io_runtime->backtrace.آخرین_قاب == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    M3_Backtrace_قاب* new_قاب = m3_AllocStruct(M3_Backtrace_قاب);

    إذا (!new_قاب) {
        io_runtime->backtrace.آخرین_قاب = M3_BACKTRACE_TRUNCATED;
        أرجع;
    }

    new_قاب->module_جابه_جایی = پیدا_کن_Module_جابه_جایی(io_runtime, i_pc);

    إذا (!io_runtime->backtrace.قابها || !io_runtime->backtrace.آخرین_قاب) {
        io_runtime->backtrace.قابها = new_قاب;
    } وإلا {
        io_runtime->backtrace.آخرین_قاب->بعدی = new_قاب;
    }
    io_runtime->backtrace.آخرین_قاب = new_قاب;
}


فراغ FillBacktraceFunctionInfo (IM3Runtime io_runtime, IM3Function i_function)
{
    // If we've had an alloc failure then the last frame doesn't refer to the
    // frame we want to fill in the function info for.
    إذا (M3_UNLIKELY(io_runtime->backtrace.آخرین_قاب == M3_BACKTRACE_TRUNCATED)) {
        أرجع;
    }

    إذا (!io_runtime->backtrace.آخرین_قاب) {
        أرجع;
    }

    io_runtime->backtrace.آخرین_قاب->function = i_function;
}


فراغ ClearBacktrace (IM3Runtime io_runtime)
{
    M3_Backtrace_قاب* current_قاب = io_runtime->backtrace.قابها;
    طالما (current_قاب) {
        M3_Backtrace_قاب* بعدی_قاب = current_قاب->بعدی;
        m3_Free(current_قاب);
        current_قاب = بعدی_قاب;
    }

    io_runtime->backtrace.قابها    = عدم;
    io_runtime->backtrace.آخرین_قاب = عدم;
}
#پایان_اگر // d_m3RecordBacktraces
