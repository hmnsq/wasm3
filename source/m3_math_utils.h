//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#جيڪڏهن_وصف_نه_ٿيل m3_math_utils_h
#وصف_ڪريو m3_math_utils_h

#ضمن "m3_core.h"

#ضمن <limits.h>

#إذا defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

ساكن مضمن
صحيح __builtin_ctz (uint32_t x)
{
    بنانشان ڊگهو ret;
    ٻٽو_انگ_جاچ_Forward(&ret, x);
    أرجع (صحيح)ret;
}

ساكن مضمن
صحيح __builtin_clz (uint32_t x)
{
    بنانشان ڊگهو ret;
    ٻٽو_انگ_جاچ_ابتو(&ret, x);
    أرجع (صحيح)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

ساكن مضمن
صحيح __builtin_ctzll (uint64_t قدر)
{
    بنانشان ڊگهو ret;
    ٻٽو_انگ_جاچ_Forward64(&ret, قدر);
    أرجع (صحيح)ret;
}

ساكن مضمن
صحيح __builtin_clzll (uint64_t قدر)
{
    بنانشان ڊگهو ret;
    ٻٽو_انگ_جاچ_Reverse64(&ret, قدر);
    أرجع (صحيح)(63 ^ ret);
}

#  وإلا // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

ساكن مضمن
صحيح __builtin_ctzll (uint64_t قدر)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    إذا (lsh != 0) {
        أرجع __builtin_ctz(lsh);
    }
    أرجع 32 + __builtin_ctz(msh);
}

ساكن مضمن
صحيح __builtin_clzll (uint64_t قدر)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    إذا (msh != 0) {
        أرجع __builtin_clz(msh);
    }
    أرجع 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#جيڪڏهن_پورو // defined(M3_COMPILER_MSVC)

#إذا defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

ساكن مضمن
صحيح __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    أرجع (صحيح)((x * 0x01010101u) >> 24);
}

ساكن مضمن
صحيح __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    أرجع __builtin_popcount((x & (~x + 1u)) - 1);
}

ساكن مضمن
صحيح __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    أرجع 32 - __builtin_popcount(x);
}

ساكن مضمن
صحيح __builtin_popcountll (uint64_t قدر)
{
    أرجع __builtin_popcount((uint32_t)(قدر & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(قدر >> 32));
}

ساكن مضمن
صحيح __builtin_ctzll (uint64_t قدر)
{
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    إذا (lsh != 0) {
        أرجع __builtin_ctz(lsh);
    }
    أرجع 32 + __builtin_ctz(msh);
}

ساكن مضمن
صحيح __builtin_clzll (uint64_t قدر)
{
    uint32_t msh = (uint32_t)(قدر >> 32);
    uint32_t lsh = (uint32_t)(قدر & 0xFFFFFFFF);
    إذا (msh != 0) {
        أرجع __builtin_clz(msh);
    }
    أرجع 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

ساكن مضمن
f32 m3_negf (f32 x)
{
    u32 بٽ;
    يادگيري_نقل(&بٽ, &x, حجم(بٽ));
    بٽ ^= 0x80000000u;
    يادگيري_نقل(&x, &بٽ, حجم(x));
    أرجع x;
}

ساكن مضمن
f64 m3_neg (f64 x)
{
    u64 بٽ;
    يادگيري_نقل(&بٽ, &x, حجم(بٽ));
    بٽ ^= 0x8000000000000000ull;
    يادگيري_نقل(&x, &بٽ, حجم(x));
    أرجع x;
}

#جيڪڏهن_پورو // defined(M3_COMPILER_TCC)

#إذا defined(__AVR__)

ساكن مضمن
عائم rintf (عائم arg)
{
    اتحاد {
        عائم    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    إذا (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        أرجع arg;
    }
    أرجع (عائم)lrint(arg);
}

ساكن مضمن
مزدوج rint (مزدوج arg)
{
    اتحاد {
        مزدوج   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    إذا (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        أرجع arg;
    }
    أرجع (مزدوج)lrint(arg);
}

ساكن مضمن
uint64_t strtoull (ثابت حرف* str, حرف** endptr, صحيح بنياد)
{
    uint64_t    نتيجو = 0;
    ثابت حرف* p = str;

    طالما (*p == ' ' || *p == '\t') {
        p++;
    }

    إذا (بنياد == 0) {
        إذا (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            بنياد = 16;
            p += 2;
        } وإلا إذا (*p == '0') {
            بنياد = 8;
            p++;
        } وإلا {
            بنياد = 10;
        }
    } وإلا إذا (بنياد == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    طالما (*p) {
        صحيح digit;
        إذا (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } وإلا إذا (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } وإلا إذا (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } وإلا {
            اكسر;
        }
        إذا (digit >= بنياد) {
            اكسر;
        }
        نتيجو = نتيجو * بنياد + digit;
        p++;
    }

    إذا (endptr) {
        *endptr = (حرف*)p;
    }
    أرجع نتيجو;
}

#جيڪڏهن_پورو

/*
 * Rotr, Rotl
 */

ساكن مضمن
u32 rotl32 (u32 n, بنانشان c)
{
    ثابت بنانشان نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 31;
    أرجع (n << c) | (n >> ((32 - c) & نقاب));
}

ساكن مضمن
u32 rotr32 (u32 n, بنانشان c)
{
    ثابت بنانشان نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 31;
    أرجع (n >> c) | (n << ((32 - c) & نقاب));
}

ساكن مضمن
u64 rotl64 (u64 n, u64 c)
{
    ثابت بنانشان نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 63;
    أرجع (n << c) | (n >> ((64 - c) & نقاب));
}

ساكن مضمن
u64 rotr64 (u64 n, u64 c)
{
    ثابت بنانشان نقاب = CHAR_BIT * حجم(n) - 1;
    c &= نقاب & 63;
    أرجع (n >> c) | (n << ((64 - c) & نقاب));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

ساكن مضمن
u64 mul_wide_u64 (u64 a, u64 b, u64* o_مٿاهون)
{
#إذا defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    بنانشان __int128 حاصل_ضرب = (بنانشان __int128)a * b;

    *o_مٿاهون = (u64)(حاصل_ضرب >> 64);
    أرجع (u64)حاصل_ضرب;
#وإلا
    // four 32 x 32 partial products of the halves, a1:a0 times b1:b0. The middle
    // column collects the carry out of the low one and the low halves of the two
    // cross terms, at most 3 * (2^32 - 1).
    u64 a0 = (u32)a, a1 = a >> 32;
    u64 b0 = (u32)b, b1 = b >> 32;

    u64 ll = a0 * b0;
    u64 lh = a0 * b1;
    u64 hl = a1 * b0;
    u64 hh = a1 * b1;

    u64 وچ = (ll >> 32) + (u32)lh + (u32)hl;

    *o_مٿاهون = hh + (lh >> 32) + (hl >> 32) + (وچ >> 32);
    أرجع (وچ << 32) | (u32)ll;
#جيڪڏهن_پورو
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
ساكن مضمن
u64 mul_wide_i64 (i64 a, i64 b, u64* o_مٿاهون)
{
    u64 هيٺيون = mul_wide_u64((u64)a, (u64)b, o_مٿاهون);

    إذا (a < 0) {
        *o_مٿاهون -= (u64)b;
    }
    إذا (b < 0) {
        *o_مٿاهون -= (u64)a;
    }

    أرجع هيٺيون;
}

/*
 * Integer Div, Rem
 */

#وصف_ڪريو OP_DIV_U(RES, A, B)                                         \
    إذا (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#وصف_ڪريو OP_REM_U(RES, A, B)                                         \
    إذا (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#إذا (INT_MIN != -وڌ_کان_وڌ_عدد)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      إذا (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      إذا (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3_Err_trap_Integer_حد_کان_مٿي);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      إذا (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      إذا (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      وإلا RES = A % B;

#وإلا

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#جيڪڏهن_پورو

/*
 * Trunc
 */

#وصف_ڪريو OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    إذا (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    إذا (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
        newTrap (m3_Err_trap_Integer_حد_کان_مٿي);                \
    }                                                       \
    RES = (TYPE)A;


#وصف_ڪريو OP_I32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i32, -2147483904.0f, 2147483648.0f)
#وصف_ڪريو OP_U32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u32,          -1.0f, 4294967296.0f)
#وصف_ڪريو OP_I32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i32, -2147483649.0 , 2147483648.0 )
#وصف_ڪريو OP_U32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u32,          -1.0 , 4294967296.0 )

#وصف_ڪريو OP_I64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f)
#وصف_ڪريو OP_U64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0f, 18446744073709551616.0f)
#وصف_ڪريو OP_I64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0 )
#وصف_ڪريو OP_U64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0 , 18446744073709551616.0 )

#وصف_ڪريو OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX)  \
    إذا (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } وإلا إذا (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } وإلا إذا (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } وإلا {                                                \
        RES = (TYPE)A;                                      \
    }

#وصف_ڪريو OP_I32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483904.0f, 2147483648.0f,   INT32_MIN,  INT32_MAX)
#وصف_ڪريو OP_U32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0f, 4294967296.0f,         0UL, UINT32_MAX)
#وصف_ڪريو OP_I32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483649.0 , 2147483648.0,    INT32_MIN,  INT32_MAX)
#وصف_ڪريو OP_U32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0 , 4294967296.0,          0UL, UINT32_MAX)

#وصف_ڪريو OP_I64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f, INT64_MIN,  INT64_MAX)
#وصف_ڪريو OP_U64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0f, 18446744073709551616.0f,      0ULL, UINT64_MAX)
#وصف_ڪريو OP_I64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0,  INT64_MIN,  INT64_MAX)
#وصف_ڪريو OP_U64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0 , 18446744073709551616.0,       0ULL, UINT64_MAX)

/*
 * Min, Max
 */

#إذا d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

ساكن مضمن
f32 quiet_nan_f32 (f32 arg)
{
    اتحاد {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    أرجع u.f;
}

ساكن مضمن
f64 quiet_nan_f64 (f64 arg)
{
    اتحاد {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    أرجع u.f;
}

ساكن مضمن
f32 min_f32 (f32 a, f32 b)
{
    إذا (M3_UNLIKELY(isnan(a) or isnan(b))) {
        أرجع quiet_nan_f32(isnan(a) ? a : b);
    }
    إذا (M3_UNLIKELY(a == 0 and a == b)) {
        أرجع signbit(a) ? a : b;
    }
    أرجع a > b ? b : a;
}

ساكن مضمن
f32 max_f32 (f32 a, f32 b)
{
    إذا (M3_UNLIKELY(isnan(a) or isnan(b))) {
        أرجع quiet_nan_f32(isnan(a) ? a : b);
    }
    إذا (M3_UNLIKELY(a == 0 and a == b)) {
        أرجع signbit(a) ? b : a;
    }
    أرجع a > b ? a : b;
}

ساكن مضمن
f64 min_f64 (f64 a, f64 b)
{
    إذا (M3_UNLIKELY(isnan(a) or isnan(b))) {
        أرجع quiet_nan_f64(isnan(a) ? a : b);
    }
    إذا (M3_UNLIKELY(a == 0 and a == b)) {
        أرجع signbit(a) ? a : b;
    }
    أرجع a > b ? b : a;
}

ساكن مضمن
f64 max_f64 (f64 a, f64 b)
{
    إذا (M3_UNLIKELY(isnan(a) or isnan(b))) {
        أرجع quiet_nan_f64(isnan(a) ? a : b);
    }
    إذا (M3_UNLIKELY(a == 0 and a == b)) {
        أرجع signbit(a) ? b : a;
    }
    أرجع a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  إذا d_m3CanonicalNaN

ساكن مضمن
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    إذا (M3_UNLIKELY(arg != arg)) {
        اتحاد {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        أرجع u.f;
    }
    أرجع arg;
}

ساكن مضمن
f64 canon_nan_f64 (f64 arg)
{
    إذا (M3_UNLIKELY(arg != arg)) {
        اتحاد {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        أرجع u.f;
    }
    أرجع arg;
}

#  وإلا

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#جيڪڏهن_پورو

#جيڪڏهن_پورو // m3_math_utils_h
