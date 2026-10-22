//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_math_utils_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_math_utils_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits.h>

#ਜੇ defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_ctz (uint32_t x)
{
    ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ret;
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Forward(&ret, x);
    ਵਾਪਸ (ਪੂਰਨ)ret;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_clz (uint32_t x)
{
    ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ret;
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_ਉਲਟ(&ret, x);
    ਵਾਪਸ (ਪੂਰਨ)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_ctzll (uint64_t ਮੁੱਲ)
{
    ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ret;
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Forward64(&ret, ਮੁੱਲ);
    ਵਾਪਸ (ਪੂਰਨ)ret;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_clzll (uint64_t ਮੁੱਲ)
{
    ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ret;
    ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Reverse64(&ret, ਮੁੱਲ);
    ਵਾਪਸ (ਪੂਰਨ)(63 ^ ret);
}

#  ਨਹੀਂਤਾਂ // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_ctzll (uint64_t ਮੁੱਲ)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(ਮੁੱਲ >> 32);
    uint32_t lsh = (uint32_t)(ਮੁੱਲ & 0xFFFFFFFF);
    ਜੇ (lsh != 0) {
        ਵਾਪਸ __builtin_ctz(lsh);
    }
    ਵਾਪਸ 32 + __builtin_ctz(msh);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_clzll (uint64_t ਮੁੱਲ)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(ਮੁੱਲ >> 32);
    uint32_t lsh = (uint32_t)(ਮੁੱਲ & 0xFFFFFFFF);
    ਜੇ (msh != 0) {
        ਵਾਪਸ __builtin_clz(msh);
    }
    ਵਾਪਸ 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#ਜੇ_ਅੰਤ // defined(M3_COMPILER_MSVC)

#ਜੇ defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    ਵਾਪਸ (ਪੂਰਨ)((x * 0x01010101u) >> 24);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    ਵਾਪਸ __builtin_popcount((x & (~x + 1u)) - 1);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ਵਾਪਸ 32 - __builtin_popcount(x);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_popcountll (uint64_t ਮੁੱਲ)
{
    ਵਾਪਸ __builtin_popcount((uint32_t)(ਮੁੱਲ & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(ਮੁੱਲ >> 32));
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_ctzll (uint64_t ਮੁੱਲ)
{
    uint32_t msh = (uint32_t)(ਮੁੱਲ >> 32);
    uint32_t lsh = (uint32_t)(ਮੁੱਲ & 0xFFFFFFFF);
    ਜੇ (lsh != 0) {
        ਵਾਪਸ __builtin_ctz(lsh);
    }
    ਵਾਪਸ 32 + __builtin_ctz(msh);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਪੂਰਨ __builtin_clzll (uint64_t ਮੁੱਲ)
{
    uint32_t msh = (uint32_t)(ਮੁੱਲ >> 32);
    uint32_t lsh = (uint32_t)(ਮੁੱਲ & 0xFFFFFFFF);
    ਜੇ (msh != 0) {
        ਵਾਪਸ __builtin_clz(msh);
    }
    ਵਾਪਸ 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

ਸਥਿਰ ਸਤਰਅੰਦਰ
f32 m3_negf (f32 x)
{
    u32 ਬਿੱਟ;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਬਿੱਟ, &x, ਆਕਾਰ(ਬਿੱਟ));
    ਬਿੱਟ ^= 0x80000000u;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&x, &ਬਿੱਟ, ਆਕਾਰ(x));
    ਵਾਪਸ x;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
f64 m3_neg (f64 x)
{
    u64 ਬਿੱਟ;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਬਿੱਟ, &x, ਆਕਾਰ(ਬਿੱਟ));
    ਬਿੱਟ ^= 0x8000000000000000ull;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&x, &ਬਿੱਟ, ਆਕਾਰ(x));
    ਵਾਪਸ x;
}

#ਜੇ_ਅੰਤ // defined(M3_COMPILER_TCC)

#ਜੇ defined(__AVR__)

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਦਸ਼ਮਲਵ rintf (ਦਸ਼ਮਲਵ arg)
{
    ਸੰਘ {
        ਦਸ਼ਮਲਵ    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    ਜੇ (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        ਵਾਪਸ arg;
    }
    ਵਾਪਸ (ਦਸ਼ਮਲਵ)lrint(arg);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਦੁੱਗਣਾ rint (ਦੁੱਗਣਾ arg)
{
    ਸੰਘ {
        ਦੁੱਗਣਾ   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    ਜੇ (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        ਵਾਪਸ arg;
    }
    ਵਾਪਸ (ਦੁੱਗਣਾ)lrint(arg);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
uint64_t strtoull (ਅਚਲ ਅੱਖਰ* str, ਅੱਖਰ** endptr, ਪੂਰਨ ਅਧਾਰ)
{
    uint64_t    ਨਤੀਜਾ = 0;
    ਅਚਲ ਅੱਖਰ* p = str;

    ਜਦੋਂ (*p == ' ' || *p == '\t') {
        p++;
    }

    ਜੇ (ਅਧਾਰ == 0) {
        ਜੇ (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            ਅਧਾਰ = 16;
            p += 2;
        } ਨਹੀਂਤਾਂ ਜੇ (*p == '0') {
            ਅਧਾਰ = 8;
            p++;
        } ਨਹੀਂਤਾਂ {
            ਅਧਾਰ = 10;
        }
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    ਜਦੋਂ (*p) {
        ਪੂਰਨ digit;
        ਜੇ (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } ਨਹੀਂਤਾਂ ਜੇ (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } ਨਹੀਂਤਾਂ ਜੇ (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } ਨਹੀਂਤਾਂ {
            ਰੋਕੋ;
        }
        ਜੇ (digit >= ਅਧਾਰ) {
            ਰੋਕੋ;
        }
        ਨਤੀਜਾ = ਨਤੀਜਾ * ਅਧਾਰ + digit;
        p++;
    }

    ਜੇ (endptr) {
        *endptr = (ਅੱਖਰ*)p;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ

/*
 * Rotr, Rotl
 */

ਸਥਿਰ ਸਤਰਅੰਦਰ
u32 rotl32 (u32 n, ਬਿਨਾਨਿਸ਼ਾਨੀ c)
{
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਨਕਾਬ = CHAR_BIT * ਆਕਾਰ(n) - 1;
    c &= ਨਕਾਬ & 31;
    ਵਾਪਸ (n << c) | (n >> ((32 - c) & ਨਕਾਬ));
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u32 rotr32 (u32 n, ਬਿਨਾਨਿਸ਼ਾਨੀ c)
{
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਨਕਾਬ = CHAR_BIT * ਆਕਾਰ(n) - 1;
    c &= ਨਕਾਬ & 31;
    ਵਾਪਸ (n >> c) | (n << ((32 - c) & ਨਕਾਬ));
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 rotl64 (u64 n, u64 c)
{
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਨਕਾਬ = CHAR_BIT * ਆਕਾਰ(n) - 1;
    c &= ਨਕਾਬ & 63;
    ਵਾਪਸ (n << c) | (n >> ((64 - c) & ਨਕਾਬ));
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 rotr64 (u64 n, u64 c)
{
    ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਨਕਾਬ = CHAR_BIT * ਆਕਾਰ(n) - 1;
    c &= ਨਕਾਬ & 63;
    ਵਾਪਸ (n >> c) | (n << ((64 - c) & ਨਕਾਬ));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 mul_wide_u64 (u64 a, u64 b, u64* o_ਉੱਚਾ)
{
#ਜੇ defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    ਬਿਨਾਨਿਸ਼ਾਨੀ __int128 ਗੁਣਨਫਲ = (ਬਿਨਾਨਿਸ਼ਾਨੀ __int128)a * b;

    *o_ਉੱਚਾ = (u64)(ਗੁਣਨਫਲ >> 64);
    ਵਾਪਸ (u64)ਗੁਣਨਫਲ;
#ਨਹੀਂਤਾਂ
    // four 32 x 32 partial products of the halves, a1:a0 times b1:b0. The middle
    // column collects the carry out of the low one and the low halves of the two
    // cross terms, at most 3 * (2^32 - 1).
    u64 a0 = (u32)a, a1 = a >> 32;
    u64 b0 = (u32)b, b1 = b >> 32;

    u64 ll = a0 * b0;
    u64 lh = a0 * b1;
    u64 hl = a1 * b0;
    u64 hh = a1 * b1;

    u64 ਵਿਚਕਾਰ = (ll >> 32) + (u32)lh + (u32)hl;

    *o_ਉੱਚਾ = hh + (lh >> 32) + (hl >> 32) + (ਵਿਚਕਾਰ >> 32);
    ਵਾਪਸ (ਵਿਚਕਾਰ << 32) | (u32)ll;
#ਜੇ_ਅੰਤ
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 mul_wide_i64 (i64 a, i64 b, u64* o_ਉੱਚਾ)
{
    u64 ਨੀਵਾਂ = mul_wide_u64((u64)a, (u64)b, o_ਉੱਚਾ);

    ਜੇ (a < 0) {
        *o_ਉੱਚਾ -= (u64)b;
    }
    ਜੇ (b < 0) {
        *o_ਉੱਚਾ -= (u64)a;
    }

    ਵਾਪਸ ਨੀਵਾਂ;
}

/*
 * Integer Div, Rem
 */

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_DIV_U(RES, A, B)                                         \
    ਜੇ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_REM_U(RES, A, B)                                         \
    ਜੇ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#ਜੇ (INT_MIN != -ਵੱਧ_ਤੋਂ_ਵੱਧ_ਅੰਕ)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      ਜੇ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      ਜੇ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3_Err_trap_Integer_ਉਛਾਲ);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      ਜੇ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      ਜੇ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      ਨਹੀਂਤਾਂ RES = A % B;

#ਨਹੀਂਤਾਂ

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#ਜੇ_ਅੰਤ

/*
 * Trunc
 */

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    ਜੇ (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    ਜੇ (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
        newTrap (m3_Err_trap_Integer_ਉਛਾਲ);                \
    }                                                       \
    RES = (TYPE)A;


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i32, -2147483904.0f, 2147483648.0f)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u32,          -1.0f, 4294967296.0f)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i32, -2147483649.0 , 2147483648.0 )
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u32,          -1.0 , 4294967296.0 )

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0f, 18446744073709551616.0f)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0 )
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0 , 18446744073709551616.0 )

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX)  \
    ਜੇ (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } ਨਹੀਂਤਾਂ ਜੇ (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } ਨਹੀਂਤਾਂ ਜੇ (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } ਨਹੀਂਤਾਂ {                                                \
        RES = (TYPE)A;                                      \
    }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483904.0f, 2147483648.0f,   INT32_MIN,  INT32_MAX)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0f, 4294967296.0f,         0UL, UINT32_MAX)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483649.0 , 2147483648.0,    INT32_MIN,  INT32_MAX)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0 , 4294967296.0,          0UL, UINT32_MAX)

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f, INT64_MIN,  INT64_MAX)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0f, 18446744073709551616.0f,      0ULL, UINT64_MAX)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_I64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0,  INT64_MIN,  INT64_MAX)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ OP_U64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0 , 18446744073709551616.0,       0ULL, UINT64_MAX)

/*
 * Min, Max
 */

#ਜੇ d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

ਸਥਿਰ ਸਤਰਅੰਦਰ
f32 quiet_nan_f32 (f32 arg)
{
    ਸੰਘ {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    ਵਾਪਸ u.f;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
f64 quiet_nan_f64 (f64 arg)
{
    ਸੰਘ {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    ਵਾਪਸ u.f;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
f32 min_f32 (f32 a, f32 b)
{
    ਜੇ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ਵਾਪਸ quiet_nan_f32(isnan(a) ? a : b);
    }
    ਜੇ (M3_UNLIKELY(a == 0 and a == b)) {
        ਵਾਪਸ signbit(a) ? a : b;
    }
    ਵਾਪਸ a > b ? b : a;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
f32 max_f32 (f32 a, f32 b)
{
    ਜੇ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ਵਾਪਸ quiet_nan_f32(isnan(a) ? a : b);
    }
    ਜੇ (M3_UNLIKELY(a == 0 and a == b)) {
        ਵਾਪਸ signbit(a) ? b : a;
    }
    ਵਾਪਸ a > b ? a : b;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
f64 min_f64 (f64 a, f64 b)
{
    ਜੇ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ਵਾਪਸ quiet_nan_f64(isnan(a) ? a : b);
    }
    ਜੇ (M3_UNLIKELY(a == 0 and a == b)) {
        ਵਾਪਸ signbit(a) ? a : b;
    }
    ਵਾਪਸ a > b ? b : a;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
f64 max_f64 (f64 a, f64 b)
{
    ਜੇ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ਵਾਪਸ quiet_nan_f64(isnan(a) ? a : b);
    }
    ਜੇ (M3_UNLIKELY(a == 0 and a == b)) {
        ਵਾਪਸ signbit(a) ? b : a;
    }
    ਵਾਪਸ a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  ਜੇ d_m3CanonicalNaN

ਸਥਿਰ ਸਤਰਅੰਦਰ
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    ਜੇ (M3_UNLIKELY(arg != arg)) {
        ਸੰਘ {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        ਵਾਪਸ u.f;
    }
    ਵਾਪਸ arg;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
f64 canon_nan_f64 (f64 arg)
{
    ਜੇ (M3_UNLIKELY(arg != arg)) {
        ਸੰਘ {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        ਵਾਪਸ u.f;
    }
    ਵਾਪਸ arg;
}

#  ਨਹੀਂਤਾਂ

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#ਜੇ_ਅੰਤ

#ਜੇ_ਅੰਤ // m3_math_utils_h
