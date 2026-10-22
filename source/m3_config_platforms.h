//
//  m3_config_platforms.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#నిర్వచితం_కాకపోతే m3_config_platforms_h
#నిర్వచించు m3_config_platforms_h

#అన్తర్భావ "wasm3_defs.h"

/*
 * Internal helpers
 */

#ఒకవేళ !defined(__cplusplus) || defined(_MSC_VER)
#  define not      !
#  define and      &&
#  define or       ||
#అయితే_ముగింపు

/*
 * Detect/define features
 */

#ఒకవేళ defined(M3_COMPILER_MSVC)
#  include <stdint.h>
#  ఒకవేళ UINTPTR_MAX == 0xFFFFFFFF
#    define M3_SIZEOF_PTR 4
#  elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#    define M3_SIZEOF_PTR 8
#  లేకపోతే
#    error "Pointer size not supported"
#  endif
#లేదావేళ defined(__SIZEOF_POINTER__)
#  define M3_SIZEOF_PTR __SIZEOF_POINTER__
#లేకపోతే
#  error "Pointer size not detected"
#అయితే_ముగింపు

#ఒకవేళ defined(M3_BIG_ENDIAN)
#  define M3_BSWAP_u8(X)  {}
#  define M3_BSWAP_u16(X) { (X)=m3_bswap16((X)); }
#  define M3_BSWAP_u32(X) { (X)=m3_bswap32((X)); }
#  define M3_BSWAP_u64(X) { (X)=m3_bswap64((X)); }
#  define M3_BSWAP_i8(X)  {}
#  define M3_BSWAP_i16(X) M3_BSWAP_u16(X)
#  define M3_BSWAP_i32(X) M3_BSWAP_u32(X)
#  define M3_BSWAP_i64(X) M3_BSWAP_u64(X)
#  define M3_BSWAP_f32(X) { సంఘం { f32 f; u32 i; } u; u.f = (X); M3_BSWAP_u32(u.i); (X) = u.f; }
#  define M3_BSWAP_f64(X) { సంఘం { f64 f; u64 i; } u; u.f = (X); M3_BSWAP_u64(u.i); (X) = u.f; }
#లేకపోతే
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
#అయితే_ముగింపు

#ఒకవేళ defined(M3_COMPILER_MSVC)
#  define M3_WEAK //__declspec(selectany)
#  define M3_NO_UBSAN
#  define M3_NOINLINE
#లేదావేళ defined(__MINGW32__) || defined(__CYGWIN__)
#  define M3_WEAK //__attribute__((selectany))
#  define M3_NO_UBSAN
#  define M3_NOINLINE   __attribute__((noinline))
#లేకపోతే
#  define M3_WEAK       __attribute__((weak))
#  define M3_NO_UBSAN   //__attribute__((no_sanitize("undefined")))
// Workaround for Cosmopolitan noinline conflict: https://github.com/jart/cosmopolitan/issues/310
#  ఒకవేళ defined(noinline)
#    define M3_NOINLINE   noinline
#  లేకపోతే
#    define M3_NOINLINE   __attribute__((noinline))
#  endif
#అయితే_ముగింపు

#ఒకవేళ !defined(M3_HAS_తోక_CALL)
#  ఒకవేళ defined(__m68k__) || defined(__microblaze__) || defined(__powerpc__) || defined(__sh__)
#    define M3_HAS_తోక_CALL 0
#  లేకపోతే
#    define M3_HAS_తోక_CALL 1
#  endif
#అయితే_ముగింపు

#ఒకవేళ M3_HAS_తోక_CALL && M3_COMPILER_HAS_ATTRIBUTE(musttail)
#  define M3_MUSTTAIL __attribute__((musttail))
#  define M3_GUARANTEED_తోక_CALL 1
#లేకపోతే
#  define M3_MUSTTAIL
// Without musttail a tail call happens only if the compiler feels like emitting one,
// and that is not something we can predict: GCC on x86-64 does it at -O3, but on
// ppc64 and alpha it does not (indirect sibling calls are restricted by those ABIs),
// and MSVC does not reliably anywhere. The interpreter merely runs slower when a
// dispatch isn't tail-called, but return_call would lose the frame it reuses -- so
// only claim a guarantee when the compiler actually gives one. GCC 15 has musttail.
// Builders who know their target sibling-calls can force it with -Dd_m3CanTailCall=1.
#  define M3_GUARANTEED_తోక_CALL 0
#అయితే_ముగింపు

#నిర్వచితం_కాకపోతే M3_THREAD_LOCAL
#  ఒకవేళ defined(M3_COMPILER_MSVC)
#    define M3_THREAD_LOCAL __declspec(thread)
#    define M3_HAS_THREAD_LOCAL 1
#  elif defined(__GNUC__) || defined(__clang__)
#    define M3_THREAD_LOCAL __thread
#    define M3_HAS_THREAD_LOCAL 1
#  లేకపోతే
#    define M3_THREAD_LOCAL
#    define M3_HAS_THREAD_LOCAL 0
#  endif
#అయితే_ముగింపు

// A cheap read of the current native stack pointer, used to bound recursion
// depth (see d_m3MaxNativeStack). Both the low-water mark and the per-call
// probe must use this same primitive so they measure the same stack -- notably
// __builtin_frame_address reads the real machine stack even when ASan relocates
// locals to its heap "fake stack".
#ఒకవేళ defined(__GNUC__) || defined(__clang__)
#  define m3_Native_దొంతర_Ptr()   ((శూన్య*) __builtin_frame_address (0))
#లేదావేళ defined(_MSC_VER)
#  include <intrin.h>
#  define m3_Native_దొంతర_Ptr()   (చిరునామా_యొక్క_Return_చిరునామా ())
#లేకపోతే
#  define m3_Native_దొంతర_Ptr()   ((శూన్య*) & (అస్థిర అక్షరం){ 0 })
#అయితే_ముగింపు

#నిర్వచితం_కాకపోతే M3_MIN
#  define M3_MIN(A,B) (((A) < (B)) ? (A) : (B))
#అయితే_ముగింపు
#నిర్వచితం_కాకపోతే M3_MAX
#  define M3_MAX(A,B) (((A) > (B)) ? (A) : (B))
#అయితే_ముగింపు

#నిర్వచించు M3_INIT(క్షేత్రం) memset(&క్షేత్రం, 0, పరిమాణం(క్షేత్రం))

#నిర్వచించు M3_లెక్క_యొక్క(x) ((పరిమాణం(x)/పరిమాణం(0[x])) / ((size_t)(!(పరిమాణం(x) % పరిమాణం(0[x])))))

#ఒకవేళ defined(__AVR__)

#  include <inttypes.h>

#  define PRIu64         "llu"
#  define PRIi64         "lli"

#  define d_m3ShortTypesDefined
రకనిర్వచనం ద్విగుణ         f64;
రకనిర్వచనం దశాంశం          f32;
రకనిర్వచనం uint64_t       u64;
రకనిర్వచనం int64_t        i64;
రకనిర్వచనం uint32_t       u32;
రకనిర్వచనం int32_t        i32;
రకనిర్వచనం పొట్టి గుర్తులేని u16;
రకనిర్వచనం పొట్టి          i16;
రకనిర్వచనం uint8_t        u8;
రకనిర్వచనం int8_t         i8;

#అయితే_ముగింపు

/*
 * Apply settings
 */

#ఒకవేళ defined(__SANITIZE_ADDRESS__) || M3_COMPILER_HAS_FEATURE(చిరునామా_sanitizer)
#  define M3_HAS_ASAN 1
#లేకపోతే
#  define M3_HAS_ASAN 0
#అయితే_ముగింపు

// preserve_none keeps nothing across a call, x19 included -- and x19 is the register
// AArch64 reserves for a frame base when a function both realigns its stack and has a
// dynamic alloca, which is what ASan's use-after-return check makes of every frame
// holding an address-taken local. The base pointer then does not survive a dispatch,
// and the frame it addressed is read back through whatever the callee left in x19.
// LLVM diagnoses the same conflict on x86-64 and refuses to compile it; on AArch64 it
// emits the code anyway. An instrumented build has no use for the convention regardless.
#ఒకవేళ M3_COMPILER_HAS_ATTRIBUTE(preserve_none) && M3_GUARANTEED_తోక_CALL && !M3_HAS_ASAN && \
  (defined(__x86_64__) || defined(__aarch64__))
#  define vectorcall   __attribute__((preserve_none))
#లేదావేళ defined(M3_COMPILER_MSVC)
#  define vectorcall   // For MSVC, better not to specify any call convention
#లేదావేళ defined(__x86_64__)
#  define vectorcall
//# elif defined(__riscv) && (__riscv_xlen == 64)
//#   define vectorcall
#లేదావేళ defined(__MINGW32__)
#  define vectorcall
#లేదావేళ defined(WIN32)
#  define vectorcall   __vectorcall
#లేదావేళ defined(ESP8266)
#  include <c_types.h>
#  define vectorcall   //ICACHE_FLASH_ATTR
#లేదావేళ defined(ESP32)
#  ఒకవేళ defined(M3_లోపల_IRAM)  // the interpreter is in IRAM, attribute not needed
#    define vectorcall
#  లేకపోతే
#    include "esp_system.h"
#    define vectorcall   IRAM_ATTR
#  endif
#లేదావేళ defined(FOMU)
#  define vectorcall   __attribute__((section(".ramtext")))
#అయితే_ముగింపు

#నిర్వచితం_కాకపోతే vectorcall
#  define vectorcall
#అయితే_ముగింపు


/*
 * Device-specific defaults
 */

#నిర్వచితం_కాకపోతే d_m3_Max_Function_దొంతర_ఎత్తు
#  ఒకవేళ defined(ESP8266) || defined(ESP32) || defined(ARDUINO_AMEBA) || defined(TEENSYDUINO)
#    define d_m3_Max_Function_దొంతర_ఎత్తు          256
#  endif
#అయితే_ముగింపు

#నిర్వచితం_కాకపోతే d_m3_Fixed_కుప్ప
#  ఒకవేళ defined(ARDUINO_AMEBA)
#    define d_m3_Fixed_కుప్ప                       (128*1024)
#  elif defined(BLUE_PILL) || defined(FOMU)
#    define d_m3_Fixed_కుప్ప                       (12*1024)
#  elif defined(ARDUINO_ARCH_ARC32) // Arduino 101
#    define d_m3_Fixed_కుప్ప                       (10*1024)
#  endif
#అయితే_ముగింపు

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
#ఒకవేళ defined(__COSMOPOLITAN__) || defined(__wasi__)
// Use m3_host_none
#లేదావేళ (defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__) ||    \
       defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
       defined(__DragonFly__) || defined(__sun) || defined(_AIX) ||           \
       defined(__QNX__) || defined(__HAIKU__) || defined(__CYGWIN__))
#  ifndef d_m3HasPosixHost
#    define d_m3HasPosixHost 1
#  endif
#లేదావేళ defined(_WIN32)
#  ifndef d_m3HasWin32Host
#    define d_m3HasWin32Host 1
#  endif
#అయితే_ముగింపు

#నిర్వచితం_కాకపోతే d_m3HasWin32Host
#  define d_m3HasWin32Host 0
#అయితే_ముగింపు
#నిర్వచితం_కాకపోతే d_m3HasPosixHost
#  define d_m3HasPosixHost 0
#అయితే_ముగింపు

// Guarded memories ask the system to catch an access past the end of a linear memory,
// which takes address space to reserve and a way to catch the fault.
//
// AddressSanitizer claims SIGSEGV exclusively, so the fault a guarded memory is built
// around never reaches wasm3 and kills the process instead of becoming a trap. Such a
// build takes the bounds checks, which is also the path worth instrumenting: a
// reservation is a plain mapping that ASan cannot see into. Setting d_m3GuardedMemory
// explicitly still wins, for a process that has arranged to keep the signal.
#ఒకవేళ M3_HAS_ASAN
#  ifndef d_m3_Guarded_జ్ఞాపకం
#    define d_m3_Guarded_జ్ఞాపకం                    0
#  endif
#అయితే_ముగింపు

#ఒకవేళ (d_m3HasPosixHost) || (d_m3HasWin32Host && defined(_MSC_VER))
#  ఒకవేళ !(defined(d_m3_Fixed_కుప్ప) && d_m3_Fixed_కుప్ప) && !defined(__CYGWIN__)
#    ఒకవేళ defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64) || defined(_M_ARM64)
#      ifndef d_m3_Guarded_జ్ఞాపకం
#        define d_m3_Guarded_జ్ఞాపకం                1
#      endif
#    endif
#  endif
#అయితే_ముగింపు

// Microcontrollers and other bare-metal targets, and any RTOS
#ఒకవేళ defined(ARDUINO) || defined(PARTICLE) || defined(PLATFORMIO) || defined(__MBED__) || \
  defined(ESP8266) || defined(ESP32) || defined(BLUE_PILL) || defined(WM_W600) || defined(FOMU)
#  ifndef d_m3VerboseErrorMessages
#    define d_m3VerboseErrorMessages             0
#  endif
#  ifndef d_m3_Max_Constant_పట్టిక_కొలత
#    define d_m3_Max_Constant_పట్టిక_కొలత             64
#  endif
#  ifndef d_m3_Max_Function_దొంతర_ఎత్తు
#    define d_m3_Max_Function_దొంతర_ఎత్తు           128
#  endif
#  ifndef d_m3_Max_Native_దొంతర
#    define d_m3_Max_Native_దొంతర                   0
#  endif
#  ifndef d_m3_సంకేతం_పుట_Align_కొలత
#    define d_m3_సంకేతం_పుట_Align_కొలత                1024
#  endif
#  ifndef d_m3HasGasMetering
#    define d_m3HasGasMetering                   0
#  endif
#  ifndef d_m3_Has_Multi_జ్ఞాపకం
#    define d_m3_Has_Multi_జ్ఞాపకం                   0
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
#  ifndef d_m3_Has_దొంతర_Switching
#    define d_m3_Has_దొంతర_Switching                0
#  endif
#అయితే_ముగింపు

/*
 * Arch-specific defaults
 */

#ఒకవేళ defined(__riscv) && (__riscv_xlen == 64)
#  ifndef d_m3_Use32_బిట్టు_Slots
#    define d_m3_Use32_బిట్టు_Slots                    0
#  endif
#లేదావేళ defined(__aarch64__)
#  ifndef d_m3_Preload_తదుపరి_Op
#    define d_m3_Preload_తదుపరి_Op                    1
#  endif
#అయితే_ముగింపు

#అయితే_ముగింపు // m3_config_platforms_h
