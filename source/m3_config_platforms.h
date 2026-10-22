//
//  m3_config_platforms.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#جے_بنا_تعریف m3_config_platforms_h
#تعریف_کرو m3_config_platforms_h

#ضمن "wasm3_defs.h"

/*
 * Internal helpers
 */

#إذا !defined(__cplusplus) || defined(_MSC_VER)
#  define not      !
#  define and      &&
#  define or       ||
#جے_انت

/*
 * Detect/define features
 */

#إذا defined(M3_COMPILER_MSVC)
#  include <stdint.h>
#  إذا UINTPTR_MAX == 0xFFFFFFFF
#    define M3_SIZEOF_PTR 4
#  elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#    define M3_SIZEOF_PTR 8
#  وإلا
#    error "Pointer size not supported"
#  endif
#وإلاإذا defined(__SIZEOF_POINTER__)
#  define M3_SIZEOF_PTR __SIZEOF_POINTER__
#وإلا
#  error "Pointer size not detected"
#جے_انت

#إذا defined(M3_BIG_ENDIAN)
#  define M3_BSWAP_u8(X)  {}
#  define M3_BSWAP_u16(X) { (X)=m3_bswap16((X)); }
#  define M3_BSWAP_u32(X) { (X)=m3_bswap32((X)); }
#  define M3_BSWAP_u64(X) { (X)=m3_bswap64((X)); }
#  define M3_BSWAP_i8(X)  {}
#  define M3_BSWAP_i16(X) M3_BSWAP_u16(X)
#  define M3_BSWAP_i32(X) M3_BSWAP_u32(X)
#  define M3_BSWAP_i64(X) M3_BSWAP_u64(X)
#  define M3_BSWAP_f32(X) { اتحاد { f32 f; u32 i; } u; u.f = (X); M3_BSWAP_u32(u.i); (X) = u.f; }
#  define M3_BSWAP_f64(X) { اتحاد { f64 f; u64 i; } u; u.f = (X); M3_BSWAP_u64(u.i); (X) = u.f; }
#وإلا
#  define M3_BSWAP_u8(X)  {}
#  define M3_BSWAP_u16(x) {}
#  define M3_BSWAP_u32(x) {}
#  define M3_BSWAP_u64(x) {}
#  define M3_BSWAP_i8(X)  {}
#  define M3_BSWAP_i16(X) {}
#  define M3_BSWAP_i32(X) {}
#  define M3_BSWAP_i64(X) {}
#  define M3_BSWAP_f32(X) {}
#  define M3_BSWAP_f64(X) {}
#جے_انت

#إذا defined(M3_COMPILER_MSVC)
#  define M3_WEAK //__declspec(selectany)
#  define M3_NO_UBSAN
#  define M3_NOINLINE
#وإلاإذا defined(__MINGW32__) || defined(__CYGWIN__)
#  define M3_WEAK //__attribute__((selectany))
#  define M3_NO_UBSAN
#  define M3_NOINLINE   __attribute__((noinline))
#وإلا
#  define M3_WEAK       __attribute__((weak))
#  define M3_NO_UBSAN   //__attribute__((no_sanitize("undefined")))
// Workaround for Cosmopolitan noinline conflict: https://github.com/jart/cosmopolitan/issues/310
#  إذا defined(noinline)
#    define M3_NOINLINE   noinline
#  وإلا
#    define M3_NOINLINE   __attribute__((noinline))
#  endif
#جے_انت

#إذا !defined(M3_HAS_پوچھ_CALL)
#  إذا defined(__m68k__) || defined(__microblaze__) || defined(__powerpc__) || defined(__sh__)
#    define M3_HAS_پوچھ_CALL 0
#  وإلا
#    define M3_HAS_پوچھ_CALL 1
#  endif
#جے_انت

#إذا M3_HAS_پوچھ_CALL && M3_COMPILER_HAS_ATTRIBUTE(musttail)
#  define M3_MUSTTAIL __attribute__((musttail))
#  define M3_GUARANTEED_پوچھ_CALL 1
#وإلا
#  define M3_MUSTTAIL
// Without musttail a tail call happens only if the compiler feels like emitting one,
// and that is not something we can predict: GCC on x86-64 does it at -O3, but on
// ppc64 and alpha it does not (indirect sibling calls are restricted by those ABIs),
// and MSVC does not reliably anywhere. The interpreter merely runs slower when a
// dispatch isn't tail-called, but return_call would lose the frame it reuses -- so
// only claim a guarantee when the compiler actually gives one. GCC 15 has musttail.
// Builders who know their target sibling-calls can force it with -Dd_m3CanTailCall=1.
#  define M3_GUARANTEED_پوچھ_CALL 0
#جے_انت

#جے_بنا_تعریف M3_THREAD_LOCAL
#  إذا defined(M3_COMPILER_MSVC)
#    define M3_THREAD_LOCAL __declspec(thread)
#    define M3_HAS_THREAD_LOCAL 1
#  elif defined(__GNUC__) || defined(__clang__)
#    define M3_THREAD_LOCAL __thread
#    define M3_HAS_THREAD_LOCAL 1
#  وإلا
#    define M3_THREAD_LOCAL
#    define M3_HAS_THREAD_LOCAL 0
#  endif
#جے_انت

// A cheap read of the current native stack pointer, used to bound recursion
// depth (see d_m3MaxNativeStack). Both the low-water mark and the per-call
// probe must use this same primitive so they measure the same stack -- notably
// __builtin_frame_address reads the real machine stack even when ASan relocates
// locals to its heap "fake stack".
#إذا defined(__GNUC__) || defined(__clang__)
#  define m3_Native_ڈھیر_Ptr()   ((فراغ*) __builtin_frame_address (0))
#وإلاإذا defined(_MSC_VER)
#  include <intrin.h>
#  define m3_Native_ڈھیر_Ptr()   (پتہ_دا_Return_پتہ ())
#وإلا
#  define m3_Native_ڈھیر_Ptr()   ((فراغ*) & (متطاير حرف){ 0 })
#جے_انت

#جے_بنا_تعریف M3_MIN
#  define M3_MIN(A,B) (((A) < (B)) ? (A) : (B))
#جے_انت
#جے_بنا_تعریف M3_MAX
#  define M3_MAX(A,B) (((A) > (B)) ? (A) : (B))
#جے_انت

#تعریف_کرو M3_INIT(میدان) memset(&میدان, 0, حجم(میدان))

#تعریف_کرو M3_گنتی_دا(x) ((حجم(x)/حجم(0[x])) / ((size_t)(!(حجم(x) % حجم(0[x])))))

#إذا defined(__AVR__)

#  include <inttypes.h>

#  define PRIu64         "llu"
#  define PRIi64         "lli"

#  define d_m3ShortTypesDefined
تعريفنوع مزدوج         f64;
تعريفنوع عائم          f32;
تعريفنوع uint64_t       u64;
تعريفنوع int64_t        i64;
تعريفنوع uint32_t       u32;
تعريفنوع int32_t        i32;
تعريفنوع چھوٹا بنانشانی u16;
تعريفنوع چھوٹا          i16;
تعريفنوع uint8_t        u8;
تعريفنوع int8_t         i8;

#جے_انت

/*
 * Apply settings
 */

#إذا defined(__SANITIZE_ADDRESS__) || M3_COMPILER_HAS_FEATURE(پتہ_sanitizer)
#  define M3_HAS_ASAN 1
#وإلا
#  define M3_HAS_ASAN 0
#جے_انت

// preserve_none keeps nothing across a call, x19 included -- and x19 is the register
// AArch64 reserves for a frame base when a function both realigns its stack and has a
// dynamic alloca, which is what ASan's use-after-return check makes of every frame
// holding an address-taken local. The base pointer then does not survive a dispatch,
// and the frame it addressed is read back through whatever the callee left in x19.
// LLVM diagnoses the same conflict on x86-64 and refuses to compile it; on AArch64 it
// emits the code anyway. An instrumented build has no use for the convention regardless.
#إذا M3_COMPILER_HAS_ATTRIBUTE(preserve_none) && M3_GUARANTEED_پوچھ_CALL && !M3_HAS_ASAN && \
  (defined(__x86_64__) || defined(__aarch64__))
#  define vectorcall   __attribute__((preserve_none))
#وإلاإذا defined(M3_COMPILER_MSVC)
#  define vectorcall   // For MSVC, better not to specify any call convention
#وإلاإذا defined(__x86_64__)
#  define vectorcall
//# elif defined(__riscv) && (__riscv_xlen == 64)
//#   define vectorcall
#وإلاإذا defined(__MINGW32__)
#  define vectorcall
#وإلاإذا defined(WIN32)
#  define vectorcall   __vectorcall
#وإلاإذا defined(ESP8266)
#  include <c_types.h>
#  define vectorcall   //ICACHE_FLASH_ATTR
#وإلاإذا defined(ESP32)
#  إذا defined(M3_وچ_IRAM)  // the interpreter is in IRAM, attribute not needed
#    define vectorcall
#  وإلا
#    include "esp_system.h"
#    define vectorcall   IRAM_ATTR
#  endif
#وإلاإذا defined(FOMU)
#  define vectorcall   __attribute__((section(".ramtext")))
#جے_انت

#جے_بنا_تعریف vectorcall
#  define vectorcall
#جے_انت


/*
 * Device-specific defaults
 */

#جے_بنا_تعریف d_m3_Max_Function_ڈھیر_اچائی
#  إذا defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AMEBA) || defined(TEENSYDUINO)
#    define d_m3_Max_Function_ڈھیر_اچائی          256
#  endif
#جے_انت

#جے_بنا_تعریف d_m3_Fixed_ڈھیر
#  إذا defined(ARDUINO_AMEBA)
#    define d_m3_Fixed_ڈھیر                       (128*1024)
#  elif defined(BLUE_PILL) || defined(FOMU)
#    define d_m3_Fixed_ڈھیر                       (12*1024)
#  elif defined(ARDUINO_ARCH_ARC32) // Arduino 101
#    define d_m3_Fixed_ڈھیر                       (10*1024)
#  endif
#جے_انت

/*
 * Platform-specific defaults
 */

// Which implementation of m3_host.h this build gets - see that header. A target
// counts as POSIX here only if it has the calls m3_host_posix.h makes, so the list
// is of operating systems rather than of "not Windows": a bare-metal or RTOS build,
// Emscripten and a Wasm self-hosting build all fall through to m3_host_none.h, which
// answers "cannot say" to everything and costs them nothing. Cosmopolitan is left
// out on purpose - one binary that starts on seven systems cannot pick one of these
// at compile time.
#إذا defined(__COSMOPOLITAN__) || defined(__wasi__)
// Use m3_host_none
#وإلاإذا (defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__) ||    \
       defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
       defined(__DragonFly__) || defined(__sun) || defined(_AIX) ||           \
       defined(__QNX__) || defined(__HAIKU__) || defined(__CYGWIN__))
#  ifndef d_m3HasPosixHost
#    define d_m3HasPosixHost 1
#  endif
#وإلاإذا defined(_WIN32)
#  ifndef d_m3HasWin32Host
#    define d_m3HasWin32Host 1
#  endif
#جے_انت

#جے_بنا_تعریف d_m3HasWin32Host
#  define d_m3HasWin32Host 0
#جے_انت
#جے_بنا_تعریف d_m3HasPosixHost
#  define d_m3HasPosixHost 0
#جے_انت

// Guarded memories ask the system to catch an access past the end of a linear memory,
// which takes address space to reserve and a way to catch the fault.
//
// AddressSanitizer claims SIGSEGV exclusively, so the fault a guarded memory is built
// around never reaches wasm3 and kills the process instead of becoming a trap. Such a
// build takes the bounds checks, which is also the path worth instrumenting: a
// reservation is a plain mapping that ASan cannot see into. Setting d_m3GuardedMemory
// explicitly still wins, for a process that has arranged to keep the signal.
#إذا M3_HAS_ASAN
#  ifndef d_m3_Guarded_حافظہ
#    define d_m3_Guarded_حافظہ                    0
#  endif
#جے_انت

#إذا (d_m3HasPosixHost) || (d_m3HasWin32Host && defined(_MSC_VER))
#  إذا !(defined(d_m3_Fixed_ڈھیر) && d_m3_Fixed_ڈھیر) && !defined(__CYGWIN__)
#    إذا defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64) || defined(_M_ARM64)
#      ifndef d_m3_Guarded_حافظہ
#        define d_m3_Guarded_حافظہ                1
#      endif
#    endif
#  endif
#جے_انت

// Microcontrollers and other bare-metal targets, and any RTOS
#إذا defined(ARDUINO) || defined(PARTICLE) || defined(PLATFORMIO) || defined(__MBED__) || \
  defined(ESP8266) || defined(ESP32) || defined(BLUE_PILL) || defined(WM_W600) || defined(FOMU)
#  ifndef d_m3VerboseErrorMessages
#    define d_m3VerboseErrorMessages             0
#  endif
#  ifndef d_m3_Max_Constant_جدول_ناپ
#    define d_m3_Max_Constant_جدول_ناپ             64
#  endif
#  ifndef d_m3_Max_Function_ڈھیر_اچائی
#    define d_m3_Max_Function_ڈھیر_اچائی           128
#  endif
#  ifndef d_m3_Max_Native_ڈھیر
#    define d_m3_Max_Native_ڈھیر                   0
#  endif
#  ifndef d_m3_کوڈ_ورقہ_Align_ناپ
#    define d_m3_کوڈ_ورقہ_Align_ناپ                1024
#  endif
#  ifndef d_m3HasGasMetering
#    define d_m3HasGasMetering                   0
#  endif
#  ifndef d_m3_Has_Multi_حافظہ
#    define d_m3_Has_Multi_حافظہ                   0
#  endif
#  ifndef d_m3HasMemory64
#    define d_m3HasMemory64                      0
#  endif
#  ifndef d_m3HasTypedRefs
#    define d_m3HasTypedRefs                     0
#  endif
#  ifndef d_m3HasAtomics
#    define d_m3HasAtomics                       0
#  endif
#  ifndef d_m3_Has_ڈھیر_Switching
#    define d_m3_Has_ڈھیر_Switching                0
#  endif
#جے_انت

/*
 * Arch-specific defaults
 */

#إذا defined(__riscv) && (__riscv_xlen == 64)
#  ifndef d_m3_Use32_ثنائی_ہندسہ_Slots
#    define d_m3_Use32_ثنائی_ہندسہ_Slots                    0
#  endif
#وإلاإذا defined(__aarch64__)
#  ifndef d_m3_Preload_اگلا_Op
#    define d_m3_Preload_اگلا_Op                    1
#  endif
#جے_انت

#جے_انت // m3_config_platforms_h
