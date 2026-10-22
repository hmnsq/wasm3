//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_math_utils_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_math_utils_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಅನ್ತರ್ಭಾವ <limits.h>

#ಆದರೆ defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_ctz (uint32_t x)
{
    ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ret;
    ಬಿಟ್_ಪರಿಶೀಲನೆ_Forward(&ret, x);
    ಹಿಂತಿರುಗಿ (ಪೂರ್ಣಾಂಕ)ret;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_clz (uint32_t x)
{
    ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ret;
    ಬಿಟ್_ಪರಿಶೀಲನೆ_ಹಿಮ್ಮುಖ(&ret, x);
    ಹಿಂತಿರುಗಿ (ಪೂರ್ಣಾಂಕ)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_ctzll (uint64_t ಮೌಲ್ಯ)
{
    ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ret;
    ಬಿಟ್_ಪರಿಶೀಲನೆ_Forward64(&ret, ಮೌಲ್ಯ);
    ಹಿಂತಿರುಗಿ (ಪೂರ್ಣಾಂಕ)ret;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_clzll (uint64_t ಮೌಲ್ಯ)
{
    ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ret;
    ಬಿಟ್_ಪರಿಶೀಲನೆ_Reverse64(&ret, ಮೌಲ್ಯ);
    ಹಿಂತಿರುಗಿ (ಪೂರ್ಣಾಂಕ)(63 ^ ret);
}

#  ಇಲ್ಲದಿದ್ದರೆ // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_ctzll (uint64_t ಮೌಲ್ಯ)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(ಮೌಲ್ಯ >> 32);
    uint32_t lsh = (uint32_t)(ಮೌಲ್ಯ & 0xFFFFFFFF);
    ಆದರೆ (lsh != 0) {
        ಹಿಂತಿರುಗಿ __builtin_ctz(lsh);
    }
    ಹಿಂತಿರುಗಿ 32 + __builtin_ctz(msh);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_clzll (uint64_t ಮೌಲ್ಯ)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(ಮೌಲ್ಯ >> 32);
    uint32_t lsh = (uint32_t)(ಮೌಲ್ಯ & 0xFFFFFFFF);
    ಆದರೆ (msh != 0) {
        ಹಿಂತಿರುಗಿ __builtin_clz(msh);
    }
    ಹಿಂತಿರುಗಿ 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#ಆದರೆ_ಅಂತ್ಯ // defined(M3_COMPILER_MSVC)

#ಆದರೆ defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    ಹಿಂತಿರುಗಿ (ಪೂರ್ಣಾಂಕ)((x * 0x01010101u) >> 24);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    ಹಿಂತಿರುಗಿ __builtin_popcount((x & (~x + 1u)) - 1);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ಹಿಂತಿರುಗಿ 32 - __builtin_popcount(x);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_popcountll (uint64_t ಮೌಲ್ಯ)
{
    ಹಿಂತಿರುಗಿ __builtin_popcount((uint32_t)(ಮೌಲ್ಯ & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(ಮೌಲ್ಯ >> 32));
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_ctzll (uint64_t ಮೌಲ್ಯ)
{
    uint32_t msh = (uint32_t)(ಮೌಲ್ಯ >> 32);
    uint32_t lsh = (uint32_t)(ಮೌಲ್ಯ & 0xFFFFFFFF);
    ಆದರೆ (lsh != 0) {
        ಹಿಂತಿರುಗಿ __builtin_ctz(lsh);
    }
    ಹಿಂತಿರುಗಿ 32 + __builtin_ctz(msh);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಪೂರ್ಣಾಂಕ __builtin_clzll (uint64_t ಮೌಲ್ಯ)
{
    uint32_t msh = (uint32_t)(ಮೌಲ್ಯ >> 32);
    uint32_t lsh = (uint32_t)(ಮೌಲ್ಯ & 0xFFFFFFFF);
    ಆದರೆ (msh != 0) {
        ಹಿಂತಿರುಗಿ __builtin_clz(msh);
    }
    ಹಿಂತಿರುಗಿ 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f32 m3_negf (f32 x)
{
    u32 ಬಿಟ್ಟುಗಳು;
    ಸ್ಮರಣೆ_ನಕಲು(&ಬಿಟ್ಟುಗಳು, &x, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
    ಬಿಟ್ಟುಗಳು ^= 0x80000000u;
    ಸ್ಮರಣೆ_ನಕಲು(&x, &ಬಿಟ್ಟುಗಳು, ಗಾತ್ರ(x));
    ಹಿಂತಿರುಗಿ x;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f64 m3_neg (f64 x)
{
    u64 ಬಿಟ್ಟುಗಳು;
    ಸ್ಮರಣೆ_ನಕಲು(&ಬಿಟ್ಟುಗಳು, &x, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
    ಬಿಟ್ಟುಗಳು ^= 0x8000000000000000ull;
    ಸ್ಮರಣೆ_ನಕಲು(&x, &ಬಿಟ್ಟುಗಳು, ಗಾತ್ರ(x));
    ಹಿಂತಿರುಗಿ x;
}

#ಆದರೆ_ಅಂತ್ಯ // defined(M3_COMPILER_TCC)

#ಆದರೆ defined(__AVR__)

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ದಶಮಾಂಶ rintf (ದಶಮಾಂಶ arg)
{
    ಒಕ್ಕೂಟ {
        ದಶಮಾಂಶ    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    ಆದರೆ (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        ಹಿಂತಿರುಗಿ arg;
    }
    ಹಿಂತಿರುಗಿ (ದಶಮಾಂಶ)lrint(arg);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ದ್ವಿಗುಣ rint (ದ್ವಿಗುಣ arg)
{
    ಒಕ್ಕೂಟ {
        ದ್ವಿಗುಣ   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    ಆದರೆ (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        ಹಿಂತಿರುಗಿ arg;
    }
    ಹಿಂತಿರುಗಿ (ದ್ವಿಗುಣ)lrint(arg);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
uint64_t strtoull (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str, ಅಕ್ಷರ** endptr, ಪೂರ್ಣಾಂಕ ತಳ)
{
    uint64_t    ಫಲಿತಾಂಶ = 0;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* p = str;

    ಆಗ (*p == ' ' || *p == '\t') {
        p++;
    }

    ಆದರೆ (ತಳ == 0) {
        ಆದರೆ (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            ತಳ = 16;
            p += 2;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*p == '0') {
            ತಳ = 8;
            p++;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ತಳ = 10;
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    ಆಗ (*p) {
        ಪೂರ್ಣಾಂಕ digit;
        ಆದರೆ (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (digit >= ತಳ) {
            ನಿಲ್ಲಿಸು;
        }
        ಫಲಿತಾಂಶ = ಫಲಿತಾಂಶ * ತಳ + digit;
        p++;
    }

    ಆದರೆ (endptr) {
        *endptr = (ಅಕ್ಷರ*)p;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ

/*
 * Rotr, Rotl
 */

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u32 rotl32 (u32 n, ಚಿಹ್ನೆರಹಿತ c)
{
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಮುಖವಾಡ = CHAR_BIT * ಗಾತ್ರ(n) - 1;
    c &= ಮುಖವಾಡ & 31;
    ಹಿಂತಿರುಗಿ (n << c) | (n >> ((32 - c) & ಮುಖವಾಡ));
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u32 rotr32 (u32 n, ಚಿಹ್ನೆರಹಿತ c)
{
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಮುಖವಾಡ = CHAR_BIT * ಗಾತ್ರ(n) - 1;
    c &= ಮುಖವಾಡ & 31;
    ಹಿಂತಿರುಗಿ (n >> c) | (n << ((32 - c) & ಮುಖವಾಡ));
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 rotl64 (u64 n, u64 c)
{
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಮುಖವಾಡ = CHAR_BIT * ಗಾತ್ರ(n) - 1;
    c &= ಮುಖವಾಡ & 63;
    ಹಿಂತಿರುಗಿ (n << c) | (n >> ((64 - c) & ಮುಖವಾಡ));
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 rotr64 (u64 n, u64 c)
{
    ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಮುಖವಾಡ = CHAR_BIT * ಗಾತ್ರ(n) - 1;
    c &= ಮುಖವಾಡ & 63;
    ಹಿಂತಿರುಗಿ (n >> c) | (n << ((64 - c) & ಮುಖವಾಡ));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 mul_wide_u64 (u64 a, u64 b, u64* o_ಮೇಲಿನ)
{
#ಆದರೆ defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    ಚಿಹ್ನೆರಹಿತ __int128 ಗುಣಲಬ್ಧ = (ಚಿಹ್ನೆರಹಿತ __int128)a * b;

    *o_ಮೇಲಿನ = (u64)(ಗುಣಲಬ್ಧ >> 64);
    ಹಿಂತಿರುಗಿ (u64)ಗುಣಲಬ್ಧ;
#ಇಲ್ಲದಿದ್ದರೆ
    // four 32 x 32 partial products of the halves, a1:a0 times b1:b0. The middle
    // column collects the carry out of the low one and the low halves of the two
    // cross terms, at most 3 * (2^32 - 1).
    u64 a0 = (u32)a, a1 = a >> 32;
    u64 b0 = (u32)b, b1 = b >> 32;

    u64 ll = a0 * b0;
    u64 lh = a0 * b1;
    u64 hl = a1 * b0;
    u64 hh = a1 * b1;

    u64 ಮಧ್ಯ = (ll >> 32) + (u32)lh + (u32)hl;

    *o_ಮೇಲಿನ = hh + (lh >> 32) + (hl >> 32) + (ಮಧ್ಯ >> 32);
    ಹಿಂತಿರುಗಿ (ಮಧ್ಯ << 32) | (u32)ll;
#ಆದರೆ_ಅಂತ್ಯ
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 mul_wide_i64 (i64 a, i64 b, u64* o_ಮೇಲಿನ)
{
    u64 ಕೆಳಗಿನ = mul_wide_u64((u64)a, (u64)b, o_ಮೇಲಿನ);

    ಆದರೆ (a < 0) {
        *o_ಮೇಲಿನ -= (u64)b;
    }
    ಆದರೆ (b < 0) {
        *o_ಮೇಲಿನ -= (u64)a;
    }

    ಹಿಂತಿರುಗಿ ಕೆಳಗಿನ;
}

/*
 * Integer Div, Rem
 */

#ವ್ಯಾಖ್ಯಾನಿಸು OP_DIV_U(RES, A, B)                                         \
    ಆದರೆ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#ವ್ಯಾಖ್ಯಾನಿಸು OP_REM_U(RES, A, B)                                         \
    ಆದರೆ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#ಆದರೆ (INT_MIN != -ಗರಿಷ್ಠ_ಪೂರ್ಣಾಂಕ)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      ಆದರೆ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      ಆದರೆ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3_Err_trap_Integer_ಅತಿಪ್ರವಾಹ);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      ಆದರೆ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      ಆದರೆ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      ಇಲ್ಲದಿದ್ದರೆ RES = A % B;

#ಇಲ್ಲದಿದ್ದರೆ

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#ಆದರೆ_ಅಂತ್ಯ

/*
 * Trunc
 */

#ವ್ಯಾಖ್ಯಾನಿಸು OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    ಆದರೆ (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    ಆದರೆ (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
        newTrap (m3_Err_trap_Integer_ಅತಿಪ್ರವಾಹ);                \
    }                                                       \
    RES = (TYPE)A;


#ವ್ಯಾಖ್ಯಾನಿಸು OP_I32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i32, -2147483904.0f, 2147483648.0f)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u32,          -1.0f, 4294967296.0f)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_I32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i32, -2147483649.0 , 2147483648.0 )
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u32,          -1.0 , 4294967296.0 )

#ವ್ಯಾಖ್ಯಾನಿಸು OP_I64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0f, 18446744073709551616.0f)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_I64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0 )
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0 , 18446744073709551616.0 )

#ವ್ಯಾಖ್ಯಾನಿಸು OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX)  \
    ಆದರೆ (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } ಇಲ್ಲದಿದ್ದರೆ {                                                \
        RES = (TYPE)A;                                      \
    }

#ವ್ಯಾಖ್ಯಾನಿಸು OP_I32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483904.0f, 2147483648.0f,   INT32_MIN,  INT32_MAX)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0f, 4294967296.0f,         0UL, UINT32_MAX)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_I32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483649.0 , 2147483648.0,    INT32_MIN,  INT32_MAX)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0 , 4294967296.0,          0UL, UINT32_MAX)

#ವ್ಯಾಖ್ಯಾನಿಸು OP_I64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f, INT64_MIN,  INT64_MAX)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0f, 18446744073709551616.0f,      0ULL, UINT64_MAX)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_I64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0,  INT64_MIN,  INT64_MAX)
#ವ್ಯಾಖ್ಯಾನಿಸು OP_U64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0 , 18446744073709551616.0,       0ULL, UINT64_MAX)

/*
 * Min, Max
 */

#ಆದರೆ d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f32 quiet_nan_f32 (f32 arg)
{
    ಒಕ್ಕೂಟ {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    ಹಿಂತಿರುಗಿ u.f;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f64 quiet_nan_f64 (f64 arg)
{
    ಒಕ್ಕೂಟ {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    ಹಿಂತಿರುಗಿ u.f;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f32 min_f32 (f32 a, f32 b)
{
    ಆದರೆ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ಹಿಂತಿರುಗಿ quiet_nan_f32(isnan(a) ? a : b);
    }
    ಆದರೆ (M3_UNLIKELY(a == 0 and a == b)) {
        ಹಿಂತಿರುಗಿ signbit(a) ? a : b;
    }
    ಹಿಂತಿರುಗಿ a > b ? b : a;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f32 max_f32 (f32 a, f32 b)
{
    ಆದರೆ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ಹಿಂತಿರುಗಿ quiet_nan_f32(isnan(a) ? a : b);
    }
    ಆದರೆ (M3_UNLIKELY(a == 0 and a == b)) {
        ಹಿಂತಿರುಗಿ signbit(a) ? b : a;
    }
    ಹಿಂತಿರುಗಿ a > b ? a : b;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f64 min_f64 (f64 a, f64 b)
{
    ಆದರೆ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ಹಿಂತಿರುಗಿ quiet_nan_f64(isnan(a) ? a : b);
    }
    ಆದರೆ (M3_UNLIKELY(a == 0 and a == b)) {
        ಹಿಂತಿರುಗಿ signbit(a) ? a : b;
    }
    ಹಿಂತಿರುಗಿ a > b ? b : a;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f64 max_f64 (f64 a, f64 b)
{
    ಆದರೆ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        ಹಿಂತಿರುಗಿ quiet_nan_f64(isnan(a) ? a : b);
    }
    ಆದರೆ (M3_UNLIKELY(a == 0 and a == b)) {
        ಹಿಂತಿರುಗಿ signbit(a) ? b : a;
    }
    ಹಿಂತಿರುಗಿ a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  ಆದರೆ d_m3CanonicalNaN

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    ಆದರೆ (M3_UNLIKELY(arg != arg)) {
        ಒಕ್ಕೂಟ {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        ಹಿಂತಿರುಗಿ u.f;
    }
    ಹಿಂತಿರುಗಿ arg;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
f64 canon_nan_f64 (f64 arg)
{
    ಆದರೆ (M3_UNLIKELY(arg != arg)) {
        ಒಕ್ಕೂಟ {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        ಹಿಂತಿರುಗಿ u.f;
    }
    ಹಿಂತಿರುಗಿ arg;
}

#  ಇಲ್ಲದಿದ್ದರೆ

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ_ಅಂತ್ಯ // m3_math_utils_h
