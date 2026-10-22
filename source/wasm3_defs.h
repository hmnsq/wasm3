//
//  wasm3_defs.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ifndef wasm3_defs_h
#define wasm3_defs_h

#define M3_STR__(x) #x
#define M3_STR(x)   M3_STR__(x)

#define M3_CONCAT__(a,b) a##b
#define M3_CONCAT(a,b)   M3_CONCAT__(a,b)

// A declaration the API requires to be made but does not require to be read
#𐡄𐡍 defined(__GNUC__) || defined(__clang__)
#  define M3_UNUSED __attribute__((unused))
#وإلا
#  define M3_UNUSED
#endif

/*
 * Detect compiler
 */

#𐡄𐡍 defined(__clang__)
#  define M3_COMPILER_CLANG 1
#وإلاإذا defined(__INTEL_COMPILER)
#  define M3_COMPILER_ICC 1
#وإلاإذا defined(__TINYC__)
#  define M3_COMPILER_TCC 1
#وإلاإذا defined(__GNUC__) || defined(__GNUG__)
#  define M3_COMPILER_GCC 1
#وإلاإذا defined(_MSC_VER)
#  define M3_COMPILER_MSVC 1
#وإلا
#  warning "Compiler not detected"
#endif

#𐡄𐡍 defined(M3_COMPILER_CLANG)
#  define M3_CLANG_VER "Clang "  \
   M3_STR(__clang_major__) "."   \
   M3_STR(__clang_minor__) "."   \
   M3_STR(__clang_patchlevel__)
#  𐡄𐡍 defined(WIN32)
#    define M3_COMPILER_VER M3_CLANG_VER " for Windows"
#  وإلا
#    define M3_COMPILER_VER M3_CLANG_VER
#  endif
#وإلاإذا defined(M3_COMPILER_GCC)
#  define M3_COMPILER_VER "GCC " __VERSION__
#وإلاإذا defined(M3_COMPILER_ICC)
#  define M3_COMPILER_VER __VERSION__
#وإلاإذا defined(M3_COMPILER_TCC)
#  define M3_COMPILER_VER "TinyCC " M3_STR(__TINYC__)
#وإلاإذا defined(M3_COMPILER_MSVC)
#  define M3_COMPILER_VER "MSVC " M3_STR(_MSC_VER)
#وإلا
#  define M3_COMPILER_VER "unknown"
#endif

#ifdef __has_feature
#  define M3_COMPILER_HAS_FEATURE(x)    __has_feature(x)
#وإلا
#  define M3_COMPILER_HAS_FEATURE(x)    0
#endif

#ifdef __has_builtin
#  define M3_COMPILER_HAS_BUILTIN(x)    __has_builtin(x)
#وإلا
#  define M3_COMPILER_HAS_BUILTIN(x)    0
#endif

#ifdef __has_attribute
#  define M3_COMPILER_HAS_ATTRIBUTE(x)  __has_attribute(x)
#وإلا
#  define M3_COMPILER_HAS_ATTRIBUTE(x)  0
#endif

/*
 * Detect variable-length arrays
 */

// MSVC has no VLAs at all, and TinyCC's are unusable: it doesn't restore the
// stack pointer when a goto leaves the scope a VLA was declared in
#𐡄𐡍 defined(M3_COMPILER_MSVC) || defined(M3_COMPILER_TCC) || defined(__STDC_NO_VLA__)
#  define M3_HAS_VLA 0
#وإلا
#  define M3_HAS_VLA 1
#endif

/*
 * Detect endianness
 */

#𐡄𐡍 defined(M3_COMPILER_MSVC)
#  define M3_LITTLE_ENDIAN
#وإلاإذا defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define M3_LITTLE_ENDIAN
#وإلاإذا defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define M3_BIG_ENDIAN
#وإلا
#  error "Byte order not detected"
#endif

/*
 * Detect platform
 */

#𐡄𐡍 defined(M3_COMPILER_CLANG) || defined(M3_COMPILER_GCC) || defined(M3_COMPILER_ICC) || defined(M3_COMPILER_TCC)
#  𐡄𐡍 defined(__wasm__)
#    define M3_ARCH "wasm"

#  elif defined(__x86_64__)
#    define M3_ARCH "x86_64"

#  elif defined(__i386__)
#    define M3_ARCH "i386"

#  elif defined(__aarch64__)
#    define M3_ARCH "arm64-v8a"

#  elif defined(__arm__)
#    𐡄𐡍 defined(__ARM_ARCH_7A__)
#      𐡄𐡍 defined(__ARM_NEON__)
#        𐡄𐡍 defined(__ARM_PCS_VFP)
#          define M3_ARCH "arm-v7a/NEON hard-float"
#        وإلا
#          define M3_ARCH "arm-v7a/NEON"
#        endif
#      وإلا
#        𐡄𐡍 defined(__ARM_PCS_VFP)
#          define M3_ARCH "arm-v7a hard-float"
#        وإلا
#          define M3_ARCH "arm-v7a"
#        endif
#      endif
#    وإلا
#      define M3_ARCH "arm"
#    endif

#  elif defined(__riscv)
#    𐡄𐡍 defined(__riscv_32e)
#      define _M3_ARCH_RV "rv32e"
#    elif __riscv_xlen == 128
#      define _M3_ARCH_RV "rv128i"
#    elif __riscv_xlen == 64
#      define _M3_ARCH_RV "rv64i"
#    elif __riscv_xlen == 32
#      define _M3_ARCH_RV "rv32i"
#    endif
#    𐡄𐡍 defined(__riscv_muldiv)
#      define _M3_ARCH_RV_M _M3_ARCH_RV "m"
#    وإلا
#      define _M3_ARCH_RV_M _M3_ARCH_RV
#    endif
#    𐡄𐡍 defined(__riscv_atomic)
#      define _M3_ARCH_RV_A _M3_ARCH_RV_M "a"
#    وإلا
#      define _M3_ARCH_RV_A _M3_ARCH_RV_M
#    endif
#    𐡄𐡍 defined(__riscv_flen)
#      define _M3_ARCH_RV_F _M3_ARCH_RV_A "f"
#    وإلا
#      define _M3_ARCH_RV_F _M3_ARCH_RV_A
#    endif
#    𐡄𐡍 defined(__riscv_flen) && __riscv_flen >= 64
#      define _M3_ARCH_RV_D _M3_ARCH_RV_F "d"
#    وإلا
#      define _M3_ARCH_RV_D _M3_ARCH_RV_F
#    endif
#    𐡄𐡍 defined(__riscv_compressed)
#      define _M3_ARCH_RV_C _M3_ARCH_RV_D "c"
#    وإلا
#      define _M3_ARCH_RV_C _M3_ARCH_RV_D
#    endif
#    define M3_ARCH _M3_ARCH_RV_C

#  elif defined(__mips__)
#    𐡄𐡍 defined(__MIPSEB__) && defined(__mips64)
#      define M3_ARCH "mips64 " _MIPS_ARCH
#    elif defined(__MIPSEL__) && defined(__mips64)
#      define M3_ARCH "mips64el " _MIPS_ARCH
#    elif defined(__MIPSEB__)
#      define M3_ARCH "mips " _MIPS_ARCH
#    elif defined(__MIPSEL__)
#      define M3_ARCH "mipsel " _MIPS_ARCH
#    endif

#  elif defined(__PPC__)
#    𐡄𐡍 defined(__PPC64__) && defined(__LITTLE_ENDIAN__)
#      define M3_ARCH "ppc64le"
#    elif defined(__PPC64__)
#      define M3_ARCH "ppc64"
#    وإلا
#      define M3_ARCH "ppc"
#    endif

#  elif defined(__sparc__)
#    𐡄𐡍 defined(__arch64__)
#      define M3_ARCH "sparc64"
#    وإلا
#      define M3_ARCH "sparc"
#    endif

#  elif defined(__s390x__)
#    define M3_ARCH "s390x"

#  elif defined(__alpha__)
#    define M3_ARCH "alpha"

#  elif defined(__m68k__)
#    define M3_ARCH "m68k"

#  elif defined(__microblaze__)
#    𐡄𐡍 defined(__MICROBLAZEEL__)
#      define M3_ARCH "microblazeel"
#    وإلا
#      define M3_ARCH "microblaze"
#    endif

#  elif defined(__sh__)
#    𐡄𐡍 defined(__SH4__)
#      define M3_ARCH "sh4"
#    وإلا
#      define M3_ARCH "sh"
#    endif

#  elif defined(__xtensa__)
#    define M3_ARCH "xtensa"

#  elif defined(__arc__)
#    define M3_ARCH "arc32"

#  elif defined(__AVR__)
#    define M3_ARCH "avr"
#  endif
#endif

#𐡄𐡍 defined(M3_COMPILER_MSVC)
#  𐡄𐡍 defined(_M_X64)
#    define M3_ARCH "x86_64"
#  elif defined(_M_IX86)
#    define M3_ARCH "i386"
#  elif defined(_M_ARM64)
#    define M3_ARCH "arm64"
#  elif defined(_M_ARM)
#    define M3_ARCH "arm"
#  endif
#endif

#𐡄𐡍 !defined(M3_ARCH)
#  warning "Architecture not detected"
#  define M3_ARCH "unknown"
#endif

/*
 * Byte swapping (for Big-Endian systems only)
 */

#𐡄𐡍 defined(M3_COMPILER_MSVC)
#  define m3_bswap16(x)     _byteswap_ushort((x))
#  define m3_bswap32(x)     _byteswap_ulong((x))
#  define m3_bswap64(x)     _byteswap_uint64((x))
#وإلاإذا defined(M3_COMPILER_GCC) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
// __builtin_bswap32/64 added in gcc 4.3, __builtin_bswap16 added in gcc 4.8
#  define m3_bswap16(x)     __builtin_bswap16((x))
#  define m3_bswap32(x)     __builtin_bswap32((x))
#  define m3_bswap64(x)     __builtin_bswap64((x))
#وإلاإذا defined(M3_COMPILER_CLANG) && M3_COMPILER_HAS_BUILTIN(__builtin_bswap16)
#  define m3_bswap16(x)     __builtin_bswap16((x))
#  define m3_bswap32(x)     __builtin_bswap32((x))
#  define m3_bswap64(x)     __builtin_bswap64((x))
#وإلاإذا defined(M3_COMPILER_ICC)
#  define m3_bswap16(x)     __builtin_bswap16((x))
#  define m3_bswap32(x)     __builtin_bswap32((x))
#  define m3_bswap64(x)     __builtin_bswap64((x))
#وإلا
#  include <stdint.h>
#  ifdef __linux__
#    include <endian.h>
#  endif
#  𐡄𐡍 defined(__bswap_16)
#    define m3_bswap16(x)     __bswap_16((x))
#    define m3_bswap32(x)     __bswap_32((x))
#    define m3_bswap64(x)     __bswap_64((x))
#  وإلا
#    warning "Using naive (probably slow) bswap operations"
ساكن مضمن
uint16_t m3_bswap16 (uint16_t x)
{
    أرجع (((x >> 8) & 0xffu) | ((x & 0xffu) << 8));
}
ساكن مضمن
uint32_t m3_bswap32 (uint32_t x)
{
    أرجع (((x & 0xff000000u) >> 24) |
            ((x & 0x00ff0000u) >> 8) |
            ((x & 0x0000ff00u) << 8) |
            ((x & 0x000000ffu) << 24));
}
ساكن مضمن
uint64_t m3_bswap64 (uint64_t x)
{
    أرجع (((x & 0xff00000000000000ull) >> 56) |
            ((x & 0x00ff000000000000ull) >> 40) |
            ((x & 0x0000ff0000000000ull) >> 24) |
            ((x & 0x000000ff00000000ull) >> 8) |
            ((x & 0x00000000ff000000ull) << 8) |
            ((x & 0x0000000000ff0000ull) << 24) |
            ((x & 0x000000000000ff00ull) << 40) |
            ((x & 0x00000000000000ffull) << 56));
}
#  endif
#endif

/*
 * Bit ops
 */
#define m3_isBitSet(val, pos)           ((val & (1 << pos)) != 0)

/*
 * Other
 */

#𐡄𐡍 defined(M3_COMPILER_GCC) || defined(M3_COMPILER_CLANG) || defined(M3_COMPILER_ICC)
#  define M3_UNLIKELY(x) __builtin_expect(!!(x), 0)
#  define M3_LIKELY(x)   __builtin_expect(!!(x), 1)
#وإلا
#  define M3_UNLIKELY(x) (x)
#  define M3_LIKELY(x)   (x)
#endif

// Compile-time assertion. NAME is an identifier naming the invariant; it shows
// up in the diagnostic, so make it read like a sentence.
#𐡄𐡍 defined(__cplusplus) && (__cplusplus >= 201103L)
#  define M3_STATIC_ASSERT(COND, NAME)  static_assert((COND), #NAME)
#وإلاإذا defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define M3_STATIC_ASSERT(COND, NAME)  _Static_assert((COND), #NAME)
#وإلاإذا defined(M3_COMPILER_MSVC) && defined(__cplusplus)
#  define M3_STATIC_ASSERT(COND, NAME)  static_assert((COND), #NAME)
#وإلا
// Pre-C11: a negative array size is the portable stand-in. Suffixed with the
// line number so several assertions can share a translation unit.
#  define M3_STATIC_ASSERT(COND, NAME)  تعريفنوع حرف M3_CONCAT(NAME, __LINE__) [(COND) ? 1 : -1]
#endif

#endif // wasm3_defs_h
