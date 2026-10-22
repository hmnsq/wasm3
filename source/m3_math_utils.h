//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#اگر_غیر_تعریف_شدہ m3_math_utils_h
#تعریف_کرو m3_math_utils_h

#شامل "m3_core.h"

#شامل <limits.h>

#اگر defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

ساکن اندرونی
صحیح __builtin_ctz (uint32_t x)
{
    غیرعلامتی طویل ret;
    ثنائی_ہندسہ_جائزہ_Forward(&ret, x);
    واپس (صحیح)ret;
}

ساکن اندرونی
صحیح __builtin_clz (uint32_t x)
{
    غیرعلامتی طویل ret;
    ثنائی_ہندسہ_جائزہ_الٹ(&ret, x);
    واپس (صحیح)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

ساکن اندرونی
صحیح __builtin_ctzll (uint64_t قدر)
{
    غیرعلامتی طویل ret;
    ثنائی_ہندسہ_جائزہ_Forward64(&ret, قدر);
    واپس (صحیح)ret;
}

ساکن اندرونی
صحیح __builtin_clzll (uint64_t قدر)
{
    غیرعلامتی طویل ret;
    ثنائی_ہندسہ_جائزہ_Reverse64(&ret, قدر);
    واپس (صحیح)(63 ^ ret);
}

#  ورنہ // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

ساکن اندرونی
صحیح __builtin_ctzll (uint64_t قدر)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    اگر (lsh != 0) {
        واپس __builtin_ctz(lsh);
    }
    واپس 32 + __builtin_ctz(msh);
}

ساکن اندرونی
صحیح __builtin_clzll (uint64_t قدر)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    اگر (msh != 0) {
        واپس __builtin_clz(msh);
    }
    واپس 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#اگر_ختم // defined(M3_COMPILER_MSVC)

#اگر defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

ساکن اندرونی
صحیح __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    واپس (صحیح)((x * 0x01010101u) >> 24);
}

ساکن اندرونی
صحیح __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    واپس __builtin_popcount((x & (~x + 1u)) - 1);
}

ساکن اندرونی
صحیح __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    واپس 32 - __builtin_popcount(x);
}

ساکن اندرونی
صحیح __builtin_popcountll (uint64_t قدر)
{
    واپس __builtin_popcount((uint32_t)(قدر & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(قدر >> 32));
}

ساکن اندرونی
صحیح __builtin_ctzll (uint64_t قدر)
{
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    اگر (lsh != 0) {
        واپس __builtin_ctz(lsh);
    }
    واپس 32 + __builtin_ctz(msh);
}

ساکن اندرونی
صحیح __builtin_clzll (uint64_t قدر)
{
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    اگر (msh != 0) {
        واپس __builtin_clz(msh);
    }
    واپس 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

ساکن اندرونی
f32 m3_negf (f32 x)
{
    u32 بٹ;
    حافظہ_نقل(&بٹ, &x, حجم(بٹ));
    بٹ ^= 0x80000000u;
    حافظہ_نقل(&x, &بٹ, حجم(x));
    واپس x;
}

ساکن اندرونی
f64 m3_neg (f64 x)
{
    u64 بٹ;
    حافظہ_نقل(&بٹ, &x, حجم(بٹ));
    بٹ ^= 0x8000000000000000ull;
    حافظہ_نقل(&x, &بٹ, حجم(x));
    واپس x;
}

#اگر_ختم // defined(M3_COMPILER_TCC)

#اگر defined(__AVR__)

ساکن اندرونی
اعشاری rintf (اعشاری arg)
{
    اتحاد {
        اعشاری    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    اگر (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        واپس arg;
    }
    واپس (اعشاری)lrint(arg);
}

ساکن اندرونی
دوہرا rint (دوہرا arg)
{
    اتحاد {
        دوہرا   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    اگر (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        واپس arg;
    }
    واپس (دوہرا)lrint(arg);
}

ساکن اندرونی
uint64_t strtoull (ثابت حرف* str, حرف** endptr, صحیح بنیاد)
{
    uint64_t    نتیجہ = 0;
    ثابت حرف* p = str;

    جب (*p == ' ' || *p == '\t') {
        p++;
    }

    اگر (بنیاد == 0) {
        اگر (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            بنیاد = 16;
            p += 2;
        } ورنہ اگر (*p == '0') {
            بنیاد = 8;
            p++;
        } ورنہ {
            بنیاد = 10;
        }
    } ورنہ اگر (بنیاد == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    جب (*p) {
        صحیح digit;
        اگر (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } ورنہ اگر (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } ورنہ اگر (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } ورنہ {
            توڑو;
        }
        اگر (digit >= بنیاد) {
            توڑو;
        }
        نتیجہ = نتیجہ * بنیاد + digit;
        p++;
    }

    اگر (endptr) {
        *endptr = (حرف*)p;
    }
    واپس نتیجہ;
}

#اگر_ختم

/*
 * Rotr, Rotl
 */

ساکن اندرونی
u32 rotl32 (u32 n, غیرعلامتی c)
{
    ثابت غیرعلامتی نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 31;
    واپس (n << c) | (n >> ((32 - c) & نقاب));
}

ساکن اندرونی
u32 rotr32 (u32 n, غیرعلامتی c)
{
    ثابت غیرعلامتی نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 31;
    واپس (n >> c) | (n << ((32 - c) & نقاب));
}

ساکن اندرونی
u64 rotl64 (u64 n, u64 c)
{
    ثابت غیرعلامتی نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 63;
    واپس (n << c) | (n >> ((64 - c) & نقاب));
}

ساکن اندرونی
u64 rotr64 (u64 n, u64 c)
{
    ثابت غیرعلامتی نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 63;
    واپس (n >> c) | (n << ((64 - c) & نقاب));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

ساکن اندرونی
u64 mul_wide_u64 (u64 a, u64 b, u64* o_بلند)
{
#اگر defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    غیرعلامتی __int128 حاصل_ضرب = (غیرعلامتی __int128)a * b;

    *o_بلند = (u64)(حاصل_ضرب >> 64);
    واپس (u64)حاصل_ضرب;
#ورنہ
    // four 32 x 32 partial products of the halves, a1:a0 times b1:b0. The middle
    // column collects the carry out of the low one and the low halves of the two
    // cross terms, at most 3 * (2^32 - 1).
    u64 a0 = (u32)a, a1 = a >> 32;
    u64 b0 = (u32)b, b1 = b >> 32;

    u64 ll = a0 * b0;
    u64 lh = a0 * b1;
    u64 hl = a1 * b0;
    u64 hh = a1 * b1;

    u64 درمیان = (ll >> 32) + (u32)lh + (u32)hl;

    *o_بلند = hh + (lh >> 32) + (hl >> 32) + (درمیان >> 32);
    واپس (درمیان << 32) | (u32)ll;
#اگر_ختم
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
ساکن اندرونی
u64 mul_wide_i64 (i64 a, i64 b, u64* o_بلند)
{
    u64 کم = mul_wide_u64((u64)a, (u64)b, o_بلند);

    اگر (a < 0) {
        *o_بلند -= (u64)b;
    }
    اگر (b < 0) {
        *o_بلند -= (u64)a;
    }

    واپس کم;
}

/*
 * Integer Div, Rem
 */

#تعریف_کرو OP_DIV_U(RES, A, B)                                         \
    اگر (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#تعریف_کرو OP_REM_U(RES, A, B)                                         \
    اگر (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#اگر (INT_MIN != -زیادہ_سے_زیادہ_عدد)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      اگر (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      اگر (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3_Err_trap_Integer_حد_سے_تجاوز);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      اگر (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      اگر (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      ورنہ RES = A % B;

#ورنہ

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#اگر_ختم

/*
 * Trunc
 */

#تعریف_کرو OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    اگر (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    اگر (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
        newTrap (m3_Err_trap_Integer_حد_سے_تجاوز);                \
    }                                                       \
    RES = (TYPE)A;


#تعریف_کرو OP_I32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i32, -2147483904.0f, 2147483648.0f)
#تعریف_کرو OP_U32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u32,          -1.0f, 4294967296.0f)
#تعریف_کرو OP_I32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i32, -2147483649.0 , 2147483648.0 )
#تعریف_کرو OP_U32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u32,          -1.0 , 4294967296.0 )

#تعریف_کرو OP_I64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f)
#تعریف_کرو OP_U64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0f, 18446744073709551616.0f)
#تعریف_کرو OP_I64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0 )
#تعریف_کرو OP_U64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0 , 18446744073709551616.0 )

#تعریف_کرو OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX)  \
    اگر (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } ورنہ اگر (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } ورنہ اگر (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } ورنہ {                                                \
        RES = (TYPE)A;                                      \
    }

#تعریف_کرو OP_I32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483904.0f, 2147483648.0f,   INT32_MIN,  INT32_MAX)
#تعریف_کرو OP_U32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0f, 4294967296.0f,         0UL, UINT32_MAX)
#تعریف_کرو OP_I32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483649.0 , 2147483648.0,    INT32_MIN,  INT32_MAX)
#تعریف_کرو OP_U32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0 , 4294967296.0,          0UL, UINT32_MAX)

#تعریف_کرو OP_I64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f, INT64_MIN,  INT64_MAX)
#تعریف_کرو OP_U64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0f, 18446744073709551616.0f,      0ULL, UINT64_MAX)
#تعریف_کرو OP_I64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0,  INT64_MIN,  INT64_MAX)
#تعریف_کرو OP_U64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0 , 18446744073709551616.0,       0ULL, UINT64_MAX)

/*
 * Min, Max
 */

#اگر d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

ساکن اندرونی
f32 quiet_nan_f32 (f32 arg)
{
    اتحاد {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    واپس u.f;
}

ساکن اندرونی
f64 quiet_nan_f64 (f64 arg)
{
    اتحاد {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    واپس u.f;
}

ساکن اندرونی
f32 min_f32 (f32 a, f32 b)
{
    اگر (M3_UNLIKELY(isnan(a) or isnan(b))) {
        واپس quiet_nan_f32(isnan(a) ? a : b);
    }
    اگر (M3_UNLIKELY(a == 0 and a == b)) {
        واپس signbit(a) ? a : b;
    }
    واپس a > b ? b : a;
}

ساکن اندرونی
f32 max_f32 (f32 a, f32 b)
{
    اگر (M3_UNLIKELY(isnan(a) or isnan(b))) {
        واپس quiet_nan_f32(isnan(a) ? a : b);
    }
    اگر (M3_UNLIKELY(a == 0 and a == b)) {
        واپس signbit(a) ? b : a;
    }
    واپس a > b ? a : b;
}

ساکن اندرونی
f64 min_f64 (f64 a, f64 b)
{
    اگر (M3_UNLIKELY(isnan(a) or isnan(b))) {
        واپس quiet_nan_f64(isnan(a) ? a : b);
    }
    اگر (M3_UNLIKELY(a == 0 and a == b)) {
        واپس signbit(a) ? a : b;
    }
    واپس a > b ? b : a;
}

ساکن اندرونی
f64 max_f64 (f64 a, f64 b)
{
    اگر (M3_UNLIKELY(isnan(a) or isnan(b))) {
        واپس quiet_nan_f64(isnan(a) ? a : b);
    }
    اگر (M3_UNLIKELY(a == 0 and a == b)) {
        واپس signbit(a) ? b : a;
    }
    واپس a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  اگر d_m3CanonicalNaN

ساکن اندرونی
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    اگر (M3_UNLIKELY(arg != arg)) {
        اتحاد {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        واپس u.f;
    }
    واپس arg;
}

ساکن اندرونی
f64 canon_nan_f64 (f64 arg)
{
    اگر (M3_UNLIKELY(arg != arg)) {
        اتحاد {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        واپس u.f;
    }
    واپس arg;
}

#  ورنہ

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#اگر_ختم

#اگر_ختم // m3_math_utils_h
