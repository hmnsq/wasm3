//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ifndef m3_math_utils_h
#define m3_math_utils_h

#include "m3_core.h"

#include <limits.h>

#အကယ်၍ defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

static anta_hstha
ကိန်းပြည့် __builtin_ctz (uint32_t x)
{
    unsigned long ret;
    _BitScanForward(&ret, x);
    ပြန်ပေး (ကိန်းပြည့်)ret;
}

static anta_hstha
ကိန်းပြည့် __builtin_clz (uint32_t x)
{
    unsigned long ret;
    _BitScanReverse(&ret, x);
    ပြန်ပေး (ကိန်းပြည့်)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

static anta_hstha
ကိန်းပြည့် __builtin_ctzll (uint64_t value)
{
    unsigned long ret;
    _BitScanForward64(&ret, value);
    ပြန်ပေး (ကိန်းပြည့်)ret;
}

static anta_hstha
ကိန်းပြည့် __builtin_clzll (uint64_t value)
{
    unsigned long ret;
    _BitScanReverse64(&ret, value);
    ပြန်ပေး (ကိန်းပြည့်)(63 ^ ret);
}

#  မဟုတ်ပါက // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

static anta_hstha
ကိန်းပြည့် __builtin_ctzll (uint64_t value)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(value >> 32);
    uint32_t lsh = (uint32_t)(value & 0xFFFFFFFF);
    အကယ်၍ (lsh != 0) {
        ပြန်ပေး __builtin_ctz(lsh);
    }
    ပြန်ပေး 32 + __builtin_ctz(msh);
}

static anta_hstha
ကိန်းပြည့် __builtin_clzll (uint64_t value)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(value >> 32);
    uint32_t lsh = (uint32_t)(value & 0xFFFFFFFF);
    အကယ်၍ (msh != 0) {
        ပြန်ပေး __builtin_clz(msh);
    }
    ပြန်ပေး 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#endif // defined(M3_COMPILER_MSVC)

#အကယ်၍ defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

static anta_hstha
ကိန်းပြည့် __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    ပြန်ပေး (ကိန်းပြည့်)((x * 0x01010101u) >> 24);
}

static anta_hstha
ကိန်းပြည့် __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    ပြန်ပေး __builtin_popcount((x & (~x + 1u)) - 1);
}

static anta_hstha
ကိန်းပြည့် __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ပြန်ပေး 32 - __builtin_popcount(x);
}

static anta_hstha
ကိန်းပြည့် __builtin_popcountll (uint64_t value)
{
    ပြန်ပေး __builtin_popcount((uint32_t)(value & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(value >> 32));
}

static anta_hstha
ကိန်းပြည့် __builtin_ctzll (uint64_t value)
{
    uint32_t msh = (uint32_t)(value >> 32);
    uint32_t lsh = (uint32_t)(value & 0xFFFFFFFF);
    အကယ်၍ (lsh != 0) {
        ပြန်ပေး __builtin_ctz(lsh);
    }
    ပြန်ပေး 32 + __builtin_ctz(msh);
}

static anta_hstha
ကိန်းပြည့် __builtin_clzll (uint64_t value)
{
    uint32_t msh = (uint32_t)(value >> 32);
    uint32_t lsh = (uint32_t)(value & 0xFFFFFFFF);
    အကယ်၍ (msh != 0) {
        ပြန်ပေး __builtin_clz(msh);
    }
    ပြန်ပေး 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

static anta_hstha
f32 m3_negf (f32 x)
{
    u32 bits;
    memcpy(&bits, &x, parimaa_na(bits));
    bits ^= 0x80000000u;
    memcpy(&x, &bits, parimaa_na(x));
    ပြန်ပေး x;
}

static anta_hstha
f64 m3_neg (f64 x)
{
    u64 bits;
    memcpy(&bits, &x, parimaa_na(bits));
    bits ^= 0x8000000000000000ull;
    memcpy(&x, &bits, parimaa_na(x));
    ပြန်ပေး x;
}

#endif // defined(M3_COMPILER_TCC)

#အကယ်၍ defined(__AVR__)

static anta_hstha
float rintf (float arg)
{
    sa_ngha {
        float    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    အကယ်၍ (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        ပြန်ပေး arg;
    }
    ပြန်ပေး (float)lrint(arg);
}

static anta_hstha
dviguu_na rint (dviguu_na arg)
{
    sa_ngha {
        dviguu_na   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    အကယ်၍ (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        ပြန်ပေး arg;
    }
    ပြန်ပေး (dviguu_na)lrint(arg);
}

static anta_hstha
uint64_t strtoull (const var_na* str, var_na** endptr, ကိန်းပြည့် base)
{
    uint64_t    result = 0;
    const var_na* p = str;

    နေစဉ် (*p == ' ' || *p == '\t') {
        p++;
    }

    အကယ်၍ (base == 0) {
        အကယ်၍ (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            base = 16;
            p += 2;
        } မဟုတ်ပါက အကယ်၍ (*p == '0') {
            base = 8;
            p++;
        } မဟုတ်ပါက {
            base = 10;
        }
    } မဟုတ်ပါက အကယ်၍ (base == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    နေစဉ် (*p) {
        ကိန်းပြည့် digit;
        အကယ်၍ (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } မဟုတ်ပါက အကယ်၍ (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } မဟုတ်ပါက အကယ်၍ (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } မဟုတ်ပါက {
            bha_nga;
        }
        အကယ်၍ (digit >= base) {
            bha_nga;
        }
        result = result * base + digit;
        p++;
    }

    အကယ်၍ (endptr) {
        *endptr = (var_na*)p;
    }
    ပြန်ပေး result;
}

#endif

/*
 * Rotr, Rotl
 */

static anta_hstha
u32 rotl32 (u32 n, unsigned c)
{
    const unsigned mask = CHAR_BIT * parimaa_na(n) - 1;
    c &= mask & 31;
    ပြန်ပေး (n << c) | (n >> ((32 - c) & mask));
}

static anta_hstha
u32 rotr32 (u32 n, unsigned c)
{
    const unsigned mask = CHAR_BIT * parimaa_na(n) - 1;
    c &= mask & 31;
    ပြန်ပေး (n >> c) | (n << ((32 - c) & mask));
}

static anta_hstha
u64 rotl64 (u64 n, u64 c)
{
    const unsigned mask = CHAR_BIT * parimaa_na(n) - 1;
    c &= mask & 63;
    ပြန်ပေး (n << c) | (n >> ((64 - c) & mask));
}

static anta_hstha
u64 rotr64 (u64 n, u64 c)
{
    const unsigned mask = CHAR_BIT * parimaa_na(n) - 1;
    c &= mask & 63;
    ပြန်ပေး (n >> c) | (n << ((64 - c) & mask));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

static anta_hstha
u64 mul_wide_u64 (u64 a, u64 b, u64* o_high)
{
#အကယ်၍ defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    unsigned __int128 product = (unsigned __int128)a * b;

    *o_high = (u64)(product >> 64);
    ပြန်ပေး (u64)product;
#မဟုတ်ပါက
    // four 32 x 32 partial products of the halves, a1:a0 times b1:b0. The middle
    // column collects the carry out of the low one and the low halves of the two
    // cross terms, at most 3 * (2^32 - 1).
    u64 a0 = (u32)a, a1 = a >> 32;
    u64 b0 = (u32)b, b1 = b >> 32;

    u64 ll = a0 * b0;
    u64 lh = a0 * b1;
    u64 hl = a1 * b0;
    u64 hh = a1 * b1;

    u64 middle = (ll >> 32) + (u32)lh + (u32)hl;

    *o_high = hh + (lh >> 32) + (hl >> 32) + (middle >> 32);
    ပြန်ပေး (middle << 32) | (u32)ll;
#endif
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
static anta_hstha
u64 mul_wide_i64 (i64 a, i64 b, u64* o_high)
{
    u64 low = mul_wide_u64((u64)a, (u64)b, o_high);

    အကယ်၍ (a < 0) {
        *o_high -= (u64)b;
    }
    အကယ်၍ (b < 0) {
        *o_high -= (u64)a;
    }

    ပြန်ပေး low;
}

/*
 * Integer Div, Rem
 */

#define OP_DIV_U(RES, A, B)                                         \
    အကယ်၍ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#define OP_REM_U(RES, A, B)                                         \
    အကယ်၍ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#အကယ်၍ (INT_MIN != -INT_MAX)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      အကယ်၍ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      အကယ်၍ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3Err_trapIntegerOverflow);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      အကယ်၍ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      အကယ်၍ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      မဟုတ်ပါက RES = A % B;

#မဟုတ်ပါက

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#endif

/*
 * Trunc
 */

#define OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    အကယ်၍ (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    အကယ်၍ (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
        newTrap (m3Err_trapIntegerOverflow);                \
    }                                                       \
    RES = (TYPE)A;


#define OP_I32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i32, -2147483904.0f, 2147483648.0f)
#define OP_U32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u32,          -1.0f, 4294967296.0f)
#define OP_I32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i32, -2147483649.0 , 2147483648.0 )
#define OP_U32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u32,          -1.0 , 4294967296.0 )

#define OP_I64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f)
#define OP_U64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0f, 18446744073709551616.0f)
#define OP_I64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0 )
#define OP_U64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0 , 18446744073709551616.0 )

#define OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX)  \
    အကယ်၍ (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } မဟုတ်ပါက အကယ်၍ (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } မဟုတ်ပါက အကယ်၍ (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } မဟုတ်ပါက {                                                \
        RES = (TYPE)A;                                      \
    }

#define OP_I32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483904.0f, 2147483648.0f,   INT32_MIN,  INT32_MAX)
#define OP_U32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0f, 4294967296.0f,         0UL, UINT32_MAX)
#define OP_I32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483649.0 , 2147483648.0,    INT32_MIN,  INT32_MAX)
#define OP_U32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0 , 4294967296.0,          0UL, UINT32_MAX)

#define OP_I64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f, INT64_MIN,  INT64_MAX)
#define OP_U64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0f, 18446744073709551616.0f,      0ULL, UINT64_MAX)
#define OP_I64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0,  INT64_MIN,  INT64_MAX)
#define OP_U64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0 , 18446744073709551616.0,       0ULL, UINT64_MAX)

/*
 * Min, Max
 */

#အကယ်၍ d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

static anta_hstha
f32 quiet_nan_f32 (f32 arg)
{
    sa_ngha {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    ပြန်ပေး u.f;
}

static anta_hstha
f64 quiet_nan_f64 (f64 arg)
{
    sa_ngha {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    ပြန်ပေး u.f;
}

static anta_hstha
f32 min_f32 (f32 a, f32 b)
{
    အကယ်၍ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ပြန်ပေး quiet_nan_f32(isnan(a) ? a : b);
    }
    အကယ်၍ (M3_UNLIKELY(a == 0 and a == b)) {
        ပြန်ပေး signbit(a) ? a : b;
    }
    ပြန်ပေး a > b ? b : a;
}

static anta_hstha
f32 max_f32 (f32 a, f32 b)
{
    အကယ်၍ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ပြန်ပေး quiet_nan_f32(isnan(a) ? a : b);
    }
    အကယ်၍ (M3_UNLIKELY(a == 0 and a == b)) {
        ပြန်ပေး signbit(a) ? b : a;
    }
    ပြန်ပေး a > b ? a : b;
}

static anta_hstha
f64 min_f64 (f64 a, f64 b)
{
    အကယ်၍ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ပြန်ပေး quiet_nan_f64(isnan(a) ? a : b);
    }
    အကယ်၍ (M3_UNLIKELY(a == 0 and a == b)) {
        ပြန်ပေး signbit(a) ? a : b;
    }
    ပြန်ပေး a > b ? b : a;
}

static anta_hstha
f64 max_f64 (f64 a, f64 b)
{
    အကယ်၍ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ပြန်ပေး quiet_nan_f64(isnan(a) ? a : b);
    }
    အကယ်၍ (M3_UNLIKELY(a == 0 and a == b)) {
        ပြန်ပေး signbit(a) ? b : a;
    }
    ပြန်ပေး a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  အကယ်၍ d_m3CanonicalNaN

static anta_hstha
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    အကယ်၍ (M3_UNLIKELY(arg != arg)) {
        sa_ngha {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        ပြန်ပေး u.f;
    }
    ပြန်ပေး arg;
}

static anta_hstha
f64 canon_nan_f64 (f64 arg)
{
    အကယ်၍ (M3_UNLIKELY(arg != arg)) {
        sa_ngha {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        ပြန်ပေး u.f;
    }
    ပြန်ပေး arg;
}

#  မဟုတ်ပါက

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#endif

#endif // m3_math_utils_h
