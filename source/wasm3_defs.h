//
//  wasm3_defs.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ wasm3_defs_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ wasm3_defs_h

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_STR__(x) #x
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_STR(x)   M3_STR__(x)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_CONCAT__(a,b) a##b
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3_CONCAT(a,b)   M3_CONCAT__(a,b)

// A declaration the API requires to be made but does not require to be read
#ਜੇ defined(__GNUC__) || defined(__clang__)
#  define M3_ਅਣਵਰਤਿਆ __attribute__((ਅਣਵਰਤਿਆ))
#ਨਹੀਂਤਾਂ
#  define M3_ਅਣਵਰਤਿਆ
#ਜੇ_ਅੰਤ

/*
 * Detect compiler
 */

#ਜੇ defined(__clang__)
#  define M3_COMPILER_CLANG 1
#ਜਾਂਜੇ defined(__INTEL_COMPILER)
#  define M3_COMPILER_ICC 1
#ਜਾਂਜੇ defined(__TINYC__)
#  define M3_COMPILER_TCC 1
#ਜਾਂਜੇ defined(__GNUC__) || defined(__GNUG__)
#  define M3_COMPILER_GCC 1
#ਜਾਂਜੇ defined(_MSC_VER)
#  define M3_COMPILER_MSVC 1
#ਨਹੀਂਤਾਂ
#  warning "Compiler not detected"
#ਜੇ_ਅੰਤ

#ਜੇ defined(M3_COMPILER_CLANG)
#  define M3_CLANG_VER "Clang "  \
   M3_STR(__clang_major__) "."   \
   M3_STR(__clang_minor__) "."   \
   M3_STR(__clang_patchlevel__)
#  ਜੇ defined(WIN32)
#    define M3_COMPILER_VER M3_CLANG_VER " for Windows"
#  ਨਹੀਂਤਾਂ
#    define M3_COMPILER_VER M3_CLANG_VER
#  endif
#ਜਾਂਜੇ defined(M3_COMPILER_GCC)
#  define M3_COMPILER_VER "GCC " __VERSION__
#ਜਾਂਜੇ defined(M3_COMPILER_ICC)
#  define M3_COMPILER_VER __VERSION__
#ਜਾਂਜੇ defined(M3_COMPILER_TCC)
#  define M3_COMPILER_VER "TinyCC " M3_STR(__TINYC__)
#ਜਾਂਜੇ defined(M3_COMPILER_MSVC)
#  define M3_COMPILER_VER "MSVC " M3_STR(_MSC_VER)
#ਨਹੀਂਤਾਂ
#  define M3_COMPILER_VER "unknown"
#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ __has_feature
#  define M3_COMPILER_HAS_FEATURE(x)    __has_feature(x)
#ਨਹੀਂਤਾਂ
#  define M3_COMPILER_HAS_FEATURE(x)    0
#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ __has_builtin
#  define M3_COMPILER_HAS_BUILTIN(x)    __has_builtin(x)
#ਨਹੀਂਤਾਂ
#  define M3_COMPILER_HAS_BUILTIN(x)    0
#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ __has_attribute
#  define M3_COMPILER_HAS_ATTRIBUTE(x)  __has_attribute(x)
#ਨਹੀਂਤਾਂ
#  define M3_COMPILER_HAS_ATTRIBUTE(x)  0
#ਜੇ_ਅੰਤ

/*
 * Detect variable-length arrays
 */

// MSVC has no VLAs at all, and TinyCC's are unusable: it doesn't restore the
// stack pointer when a goto leaves the scope a VLA was declared in
#ਜੇ defined(M3_COMPILER_MSVC) || defined(M3_COMPILER_TCC) || defined(__STDC_NO_VLA__)
#  define M3_HAS_VLA 0
#ਨਹੀਂਤਾਂ
#  define M3_HAS_VLA 1
#ਜੇ_ਅੰਤ

/*
 * Detect endianness
 */

#ਜੇ defined(M3_COMPILER_MSVC)
#  define M3_LITTLE_ENDIAN
#ਜਾਂਜੇ defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define M3_LITTLE_ENDIAN
#ਜਾਂਜੇ defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define M3_BIG_ENDIAN
#ਨਹੀਂਤਾਂ
#  error "Byte order not detected"
#ਜੇ_ਅੰਤ

/*
 * Detect platform
 */

#ਜੇ defined(M3_COMPILER_CLANG) || defined(M3_COMPILER_GCC) || defined(M3_COMPILER_ICC) || defined(M3_COMPILER_TCC)
#  ਜੇ defined(__wasm__)
#    define M3_ARCH "wasm"

#  elif defined(__x86_64__)
#    define M3_ARCH "x86_64"

#  elif defined(__i386__)
#    define M3_ARCH "i386"

#  elif defined(__aarch64__)
#    define M3_ARCH "arm64-v8a"

#  elif defined(__arm__)
#    ਜੇ defined(__ARM_ARCH_7A__)
#      ਜੇ defined(__ARM_NEON__)
#        ਜੇ defined(__ARM_PCS_VFP)
#          define M3_ARCH "arm-v7a/NEON hard-float"
#        ਨਹੀਂਤਾਂ
#          define M3_ARCH "arm-v7a/NEON"
#        endif
#      ਨਹੀਂਤਾਂ
#        ਜੇ defined(__ARM_PCS_VFP)
#          define M3_ARCH "arm-v7a hard-float"
#        ਨਹੀਂਤਾਂ
#          define M3_ARCH "arm-v7a"
#        endif
#      endif
#    ਨਹੀਂਤਾਂ
#      define M3_ARCH "arm"
#    endif

#  elif defined(__riscv)
#    ਜੇ defined(__riscv_32e)
#      define _M3_ARCH_RV "rv32e"
#    elif __riscv_xlen == 128
#      define _M3_ARCH_RV "rv128i"
#    elif __riscv_xlen == 64
#      define _M3_ARCH_RV "rv64i"
#    elif __riscv_xlen == 32
#      define _M3_ARCH_RV "rv32i"
#    endif
#    ਜੇ defined(__riscv_muldiv)
#      define _M3_ARCH_RV_M _M3_ARCH_RV "m"
#    ਨਹੀਂਤਾਂ
#      define _M3_ARCH_RV_M _M3_ARCH_RV
#    endif
#    ਜੇ defined(__riscv_atomic)
#      define _M3_ARCH_RV_A _M3_ARCH_RV_M "a"
#    ਨਹੀਂਤਾਂ
#      define _M3_ARCH_RV_A _M3_ARCH_RV_M
#    endif
#    ਜੇ defined(__riscv_flen)
#      define _M3_ARCH_RV_F _M3_ARCH_RV_A "f"
#    ਨਹੀਂਤਾਂ
#      define _M3_ARCH_RV_F _M3_ARCH_RV_A
#    endif
#    ਜੇ defined(__riscv_flen) && __riscv_flen >= 64
#      define _M3_ARCH_RV_D _M3_ARCH_RV_F "d"
#    ਨਹੀਂਤਾਂ
#      define _M3_ARCH_RV_D _M3_ARCH_RV_F
#    endif
#    ਜੇ defined(__riscv_compressed)
#      define _M3_ARCH_RV_C _M3_ARCH_RV_D "c"
#    ਨਹੀਂਤਾਂ
#      define _M3_ARCH_RV_C _M3_ARCH_RV_D
#    endif
#    define M3_ARCH _M3_ARCH_RV_C

#  elif defined(__mips__)
#    ਜੇ defined(__MIPSEB__) && defined(__mips64)
#      define M3_ARCH "mips64 " _MIPS_ARCH
#    elif defined(__MIPSEL__) && defined(__mips64)
#      define M3_ARCH "mips64el " _MIPS_ARCH
#    elif defined(__MIPSEB__)
#      define M3_ARCH "mips " _MIPS_ARCH
#    elif defined(__MIPSEL__)
#      define M3_ARCH "mipsel " _MIPS_ARCH
#    endif

#  elif defined(__PPC__)
#    ਜੇ defined(__PPC64__) && defined(__LITTLE_ENDIAN__)
#      define M3_ARCH "ppc64le"
#    elif defined(__PPC64__)
#      define M3_ARCH "ppc64"
#    ਨਹੀਂਤਾਂ
#      define M3_ARCH "ppc"
#    endif

#  elif defined(__sparc__)
#    ਜੇ defined(__arch64__)
#      define M3_ARCH "sparc64"
#    ਨਹੀਂਤਾਂ
#      define M3_ARCH "sparc"
#    endif

#  elif defined(__s390x__)
#    define M3_ARCH "s390x"

#  elif defined(__alpha__)
#    define M3_ARCH "alpha"

#  elif defined(__m68k__)
#    define M3_ARCH "m68k"

#  elif defined(__microblaze__)
#    ਜੇ defined(__MICROBLAZEEL__)
#      define M3_ARCH "microblazeel"
#    ਨਹੀਂਤਾਂ
#      define M3_ARCH "microblaze"
#    endif

#  elif defined(__sh__)
#    ਜੇ defined(__SH4__)
#      define M3_ARCH "sh4"
#    ਨਹੀਂਤਾਂ
#      define M3_ARCH "sh"
#    endif

#  elif defined(__xtensa__)
#    define M3_ARCH "xtensa"

#  elif defined(__arc__)
#    define M3_ARCH "arc32"

#  elif defined(__AVR__)
#    define M3_ARCH "avr"
#  endif
#ਜੇ_ਅੰਤ

#ਜੇ defined(M3_COMPILER_MSVC)
#  ਜੇ defined(_M_X64)
#    define M3_ARCH "x86_64"
#  elif defined(_M_IX86)
#    define M3_ARCH "i386"
#  elif defined(_M_ARM64)
#    define M3_ARCH "arm64"
#  elif defined(_M_ARM)
#    define M3_ARCH "arm"
#  endif
#ਜੇ_ਅੰਤ

#ਜੇ !defined(M3_ARCH)
#  warning "Architecture not detected"
#  define M3_ARCH "unknown"
#ਜੇ_ਅੰਤ

/*
 * Byte swapping (for Big-Endian systems only)
 */

#ਜੇ defined(M3_COMPILER_MSVC)
#  define m3_bswap16(x)     _byteswap_ushort((x))
#  define m3_bswap32(x)     _byteswap_ulong((x))
#  define m3_bswap64(x)     _byteswap_uint64((x))
#ਜਾਂਜੇ defined(M3_COMPILER_GCC) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
// __builtin_bswap32/64 added in gcc 4.3, __builtin_bswap16 added in gcc 4.8
#  define m3_bswap16(x)     __builtin_bswap16((x))
#  define m3_bswap32(x)     __builtin_bswap32((x))
#  define m3_bswap64(x)     __builtin_bswap64((x))
#ਜਾਂਜੇ defined(M3_COMPILER_CLANG) && M3_COMPILER_HAS_BUILTIN(__builtin_bswap16)
#  define m3_bswap16(x)     __builtin_bswap16((x))
#  define m3_bswap32(x)     __builtin_bswap32((x))
#  define m3_bswap64(x)     __builtin_bswap64((x))
#ਜਾਂਜੇ defined(M3_COMPILER_ICC)
#  define m3_bswap16(x)     __builtin_bswap16((x))
#  define m3_bswap32(x)     __builtin_bswap32((x))
#  define m3_bswap64(x)     __builtin_bswap64((x))
#ਨਹੀਂਤਾਂ
#  include <stdint.h>
#  ifdef __linux__
#    include <endian.h>
#  endif
#  ਜੇ defined(__bswap_16)
#    define m3_bswap16(x)     __bswap_16((x))
#    define m3_bswap32(x)     __bswap_32((x))
#    define m3_bswap64(x)     __bswap_64((x))
#  ਨਹੀਂਤਾਂ
#    warning "Using naive (probably slow) bswap operations"
ਸਥਿਰ ਸਤਰਅੰਦਰ
uint16_t m3_bswap16 (uint16_t x)
{
    ਵਾਪਸ (((x >> 8) & 0xffu) | ((x & 0xffu) << 8));
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
uint32_t m3_bswap32 (uint32_t x)
{
    ਵਾਪਸ (((x & 0xff000000u) >> 24) |
            ((x & 0x00ff0000u) >> 8) |
            ((x & 0x0000ff00u) << 8) |
            ((x & 0x000000ffu) << 24));
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
uint64_t m3_bswap64 (uint64_t x)
{
    ਵਾਪਸ (((x & 0xff00000000000000ull) >> 56) |
            ((x & 0x00ff000000000000ull) >> 40) |
            ((x & 0x0000ff0000000000ull) >> 24) |
            ((x & 0x000000ff00000000ull) >> 8) |
            ((x & 0x00000000ff000000ull) << 8) |
            ((x & 0x0000000000ff0000ull) << 24) |
            ((x & 0x000000000000ff00ull) << 40) |
            ((x & 0x00000000000000ffull) << 56));
}
#  endif
#ਜੇ_ਅੰਤ

/*
 * Bit ops
 */
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_is_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(val, pos)           ((val & (1 << pos)) != 0)

/*
 * Other
 */

#ਜੇ defined(M3_COMPILER_GCC) || defined(M3_COMPILER_CLANG) || defined(M3_COMPILER_ICC)
#  define M3_UNLIKELY(x) __builtin_expect(!!(x), 0)
#  define M3_LIKELY(x)   __builtin_expect(!!(x), 1)
#ਨਹੀਂਤਾਂ
#  define M3_UNLIKELY(x) (x)
#  define M3_LIKELY(x)   (x)
#ਜੇ_ਅੰਤ

// Compile-time assertion. NAME is an identifier naming the invariant; it shows
// up in the diagnostic, so make it read like a sentence.
#ਜੇ defined(__cplusplus) && (__cplusplus >= 201103L)
#  define M3_STATIC_ASSERT(COND, NAME)  static_assert((COND), #NAME)
#ਜਾਂਜੇ defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define M3_STATIC_ASSERT(COND, NAME)  _Static_assert((COND), #NAME)
#ਜਾਂਜੇ defined(M3_COMPILER_MSVC) && defined(__cplusplus)
#  define M3_STATIC_ASSERT(COND, NAME)  static_assert((COND), #NAME)
#ਨਹੀਂਤਾਂ
// Pre-C11: a negative array size is the portable stand-in. Suffixed with the
// line number so several assertions can share a translation unit.
#  define M3_STATIC_ASSERT(COND, NAME)  ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅੱਖਰ M3_CONCAT(NAME, __LINE__) [(COND) ? 1 : -1]
#ਜੇ_ਅੰਤ

#ਜੇ_ਅੰਤ // wasm3_defs_h
