//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ifndef m3_math_utils_h
#define m3_math_utils_h

#अन्तर्भाव "m3_core.h"

#अन्तर्भाव <limits.h>

#यदि defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_ctz (uint32_t x)
{
    unsigned long ret;
    _BitScanForward(&ret, x);
    प्रत्यागम (पूर्णाङ्क)ret;
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_clz (uint32_t x)
{
    unsigned long ret;
    _BitScanReverse(&ret, x);
    प्रत्यागम (पूर्णाङ्क)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_ctzll (uint64_t मूल्य)
{
    unsigned long ret;
    _BitScanForward64(&ret, मूल्य);
    प्रत्यागम (पूर्णाङ्क)ret;
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_clzll (uint64_t मूल्य)
{
    unsigned long ret;
    _BitScanReverse64(&ret, मूल्य);
    प्रत्यागम (पूर्णाङ्क)(63 ^ ret);
}

#  अन्यथा // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_ctzll (uint64_t मूल्य)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    यदि (lsh != 0) {
        प्रत्यागम __builtin_ctz(lsh);
    }
    प्रत्यागम 32 + __builtin_ctz(msh);
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_clzll (uint64_t मूल्य)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    यदि (msh != 0) {
        प्रत्यागम __builtin_clz(msh);
    }
    प्रत्यागम 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#endif // defined(M3_COMPILER_MSVC)

#यदि defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    प्रत्यागम (पूर्णाङ्क)((x * 0x01010101u) >> 24);
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    प्रत्यागम __builtin_popcount((x & (~x + 1u)) - 1);
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    प्रत्यागम 32 - __builtin_popcount(x);
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_popcountll (uint64_t मूल्य)
{
    प्रत्यागम __builtin_popcount((uint32_t)(मूल्य & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(मूल्य >> 32));
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_ctzll (uint64_t मूल्य)
{
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    यदि (lsh != 0) {
        प्रत्यागम __builtin_ctz(lsh);
    }
    प्रत्यागम 32 + __builtin_ctz(msh);
}

स्थिर अन्तःस्थ
पूर्णाङ्क __builtin_clzll (uint64_t मूल्य)
{
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    यदि (msh != 0) {
        प्रत्यागम __builtin_clz(msh);
    }
    प्रत्यागम 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

स्थिर अन्तःस्थ
f32 m3_negf (f32 x)
{
    u32 bits;
    memcpy(&bits, &x, परिमाण(bits));
    bits ^= 0x80000000u;
    memcpy(&x, &bits, परिमाण(x));
    प्रत्यागम x;
}

स्थिर अन्तःस्थ
f64 m3_neg (f64 x)
{
    u64 bits;
    memcpy(&bits, &x, परिमाण(bits));
    bits ^= 0x8000000000000000ull;
    memcpy(&x, &bits, परिमाण(x));
    प्रत्यागम x;
}

#endif // defined(M3_COMPILER_TCC)

#यदि defined(__AVR__)

स्थिर अन्तःस्थ
भिन्न rintf (भिन्न arg)
{
    सङ्घ {
        भिन्न    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    यदि (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        प्रत्यागम arg;
    }
    प्रत्यागम (भिन्न)lrint(arg);
}

स्थिर अन्तःस्थ
द्विगुण rint (द्विगुण arg)
{
    सङ्घ {
        द्विगुण   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    यदि (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        प्रत्यागम arg;
    }
    प्रत्यागम (द्विगुण)lrint(arg);
}

स्थिर अन्तःस्थ
uint64_t strtoull (ध्रुव वर्ण* str, वर्ण** endptr, पूर्णाङ्क base)
{
    uint64_t    result = 0;
    ध्रुव वर्ण* p = str;

    यावत् (*p == ' ' || *p == '\t') {
        p++;
    }

    यदि (base == 0) {
        यदि (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            base = 16;
            p += 2;
        } अन्यथा यदि (*p == '0') {
            base = 8;
            p++;
        } अन्यथा {
            base = 10;
        }
    } अन्यथा यदि (base == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    यावत् (*p) {
        पूर्णाङ्क digit;
        यदि (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } अन्यथा यदि (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } अन्यथा यदि (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } अन्यथा {
            भङ्ग;
        }
        यदि (digit >= base) {
            भङ्ग;
        }
        result = result * base + digit;
        p++;
    }

    यदि (endptr) {
        *endptr = (वर्ण*)p;
    }
    प्रत्यागम result;
}

#endif

/*
 * Rotr, Rotl
 */

स्थिर अन्तःस्थ
u32 rotl32 (u32 n, unsigned c)
{
    ध्रुव unsigned mask = CHAR_BIT * परिमाण(n) - 1;
    c &= mask & 31;
    प्रत्यागम (n << c) | (n >> ((32 - c) & mask));
}

स्थिर अन्तःस्थ
u32 rotr32 (u32 n, unsigned c)
{
    ध्रुव unsigned mask = CHAR_BIT * परिमाण(n) - 1;
    c &= mask & 31;
    प्रत्यागम (n >> c) | (n << ((32 - c) & mask));
}

स्थिर अन्तःस्थ
u64 rotl64 (u64 n, u64 c)
{
    ध्रुव unsigned mask = CHAR_BIT * परिमाण(n) - 1;
    c &= mask & 63;
    प्रत्यागम (n << c) | (n >> ((64 - c) & mask));
}

स्थिर अन्तःस्थ
u64 rotr64 (u64 n, u64 c)
{
    ध्रुव unsigned mask = CHAR_BIT * परिमाण(n) - 1;
    c &= mask & 63;
    प्रत्यागम (n >> c) | (n << ((64 - c) & mask));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

स्थिर अन्तःस्थ
u64 mul_wide_u64 (u64 a, u64 b, u64* o_ऊपरी)
{
#यदि defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    unsigned __int128 product = (unsigned __int128)a * b;

    *o_ऊपरी = (u64)(product >> 64);
    प्रत्यागम (u64)product;
#अन्यथा
    // four 32 x 32 partial products of the halves, a1:a0 times b1:b0. The middle
    // column collects the carry out of the low one and the low halves of the two
    // cross terms, at most 3 * (2^32 - 1).
    u64 a0 = (u32)a, a1 = a >> 32;
    u64 b0 = (u32)b, b1 = b >> 32;

    u64 ll = a0 * b0;
    u64 lh = a0 * b1;
    u64 hl = a1 * b0;
    u64 hh = a1 * b1;

    u64 मध्य = (ll >> 32) + (u32)lh + (u32)hl;

    *o_ऊपरी = hh + (lh >> 32) + (hl >> 32) + (मध्य >> 32);
    प्रत्यागम (मध्य << 32) | (u32)ll;
#endif
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
स्थिर अन्तःस्थ
u64 mul_wide_i64 (i64 a, i64 b, u64* o_ऊपरी)
{
    u64 निचला = mul_wide_u64((u64)a, (u64)b, o_ऊपरी);

    यदि (a < 0) {
        *o_ऊपरी -= (u64)b;
    }
    यदि (b < 0) {
        *o_ऊपरी -= (u64)a;
    }

    प्रत्यागम निचला;
}

/*
 * Integer Div, Rem
 */

#define OP_DIV_U(RES, A, B)                                         \
    यदि (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#define OP_REM_U(RES, A, B)                                         \
    यदि (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#यदि (INT_MIN != -INT_MAX)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      यदि (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      यदि (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3Err_trapIntegerOverflow);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      यदि (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      यदि (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      अन्यथा RES = A % B;

#अन्यथा

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#endif

/*
 * Trunc
 */

#define OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    यदि (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    यदि (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
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
    यदि (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } अन्यथा यदि (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } अन्यथा यदि (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } अन्यथा {                                                \
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

#यदि d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

स्थिर अन्तःस्थ
f32 quiet_nan_f32 (f32 arg)
{
    सङ्घ {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    प्रत्यागम u.f;
}

स्थिर अन्तःस्थ
f64 quiet_nan_f64 (f64 arg)
{
    सङ्घ {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    प्रत्यागम u.f;
}

स्थिर अन्तःस्थ
f32 min_f32 (f32 a, f32 b)
{
    यदि (M3_UNLIKELY(isnan(a) or isnan(b))) {
        प्रत्यागम quiet_nan_f32(isnan(a) ? a : b);
    }
    यदि (M3_UNLIKELY(a == 0 and a == b)) {
        प्रत्यागम signbit(a) ? a : b;
    }
    प्रत्यागम a > b ? b : a;
}

स्थिर अन्तःस्थ
f32 max_f32 (f32 a, f32 b)
{
    यदि (M3_UNLIKELY(isnan(a) or isnan(b))) {
        प्रत्यागम quiet_nan_f32(isnan(a) ? a : b);
    }
    यदि (M3_UNLIKELY(a == 0 and a == b)) {
        प्रत्यागम signbit(a) ? b : a;
    }
    प्रत्यागम a > b ? a : b;
}

स्थिर अन्तःस्थ
f64 min_f64 (f64 a, f64 b)
{
    यदि (M3_UNLIKELY(isnan(a) or isnan(b))) {
        प्रत्यागम quiet_nan_f64(isnan(a) ? a : b);
    }
    यदि (M3_UNLIKELY(a == 0 and a == b)) {
        प्रत्यागम signbit(a) ? a : b;
    }
    प्रत्यागम a > b ? b : a;
}

स्थिर अन्तःस्थ
f64 max_f64 (f64 a, f64 b)
{
    यदि (M3_UNLIKELY(isnan(a) or isnan(b))) {
        प्रत्यागम quiet_nan_f64(isnan(a) ? a : b);
    }
    यदि (M3_UNLIKELY(a == 0 and a == b)) {
        प्रत्यागम signbit(a) ? b : a;
    }
    प्रत्यागम a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  यदि d_m3CanonicalNaN

स्थिर अन्तःस्थ
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    यदि (M3_UNLIKELY(arg != arg)) {
        सङ्घ {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        प्रत्यागम u.f;
    }
    प्रत्यागम arg;
}

स्थिर अन्तःस्थ
f64 canon_nan_f64 (f64 arg)
{
    यदि (M3_UNLIKELY(arg != arg)) {
        सङ्घ {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        प्रत्यागम u.f;
    }
    प्रत्यागम arg;
}

#  अन्यथा

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#endif

#endif // m3_math_utils_h
