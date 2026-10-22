//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#నిర్వచితం_కాకపోతే m3_math_utils_h
#నిర్వచించు m3_math_utils_h

#అన్తర్భావ "m3_core.h"

#అన్తర్భావ <limits.h>

#ఒకవేళ defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_ctz (uint32_t x)
{
    గుర్తులేని దీర్ఘ ret;
    బిట్టు_పరిశీలన_Forward(&ret, x);
    తిరిగి (పూర్ణాంకం)ret;
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_clz (uint32_t x)
{
    గుర్తులేని దీర్ఘ ret;
    బిట్టు_పరిశీలన_తిరగేయి(&ret, x);
    తిరిగి (పూర్ణాంకం)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_ctzll (uint64_t విలువ)
{
    గుర్తులేని దీర్ఘ ret;
    బిట్టు_పరిశీలన_Forward64(&ret, విలువ);
    తిరిగి (పూర్ణాంకం)ret;
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_clzll (uint64_t విలువ)
{
    గుర్తులేని దీర్ఘ ret;
    బిట్టు_పరిశీలన_Reverse64(&ret, విలువ);
    తిరిగి (పూర్ణాంకం)(63 ^ ret);
}

#  లేకపోతే // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_ctzll (uint64_t విలువ)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(విలువ >> 32);
    uint32_t lsh = (uint32_t)(విలువ & 0xFFFFFFFF);
    ఒకవేళ (lsh != 0) {
        తిరిగి __builtin_ctz(lsh);
    }
    తిరిగి 32 + __builtin_ctz(msh);
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_clzll (uint64_t విలువ)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(విలువ >> 32);
    uint32_t lsh = (uint32_t)(విలువ & 0xFFFFFFFF);
    ఒకవేళ (msh != 0) {
        తిరిగి __builtin_clz(msh);
    }
    తిరిగి 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#అయితే_ముగింపు // defined(M3_COMPILER_MSVC)

#ఒకవేళ defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    తిరిగి (పూర్ణాంకం)((x * 0x01010101u) >> 24);
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    తిరిగి __builtin_popcount((x & (~x + 1u)) - 1);
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    తిరిగి 32 - __builtin_popcount(x);
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_popcountll (uint64_t విలువ)
{
    తిరిగి __builtin_popcount((uint32_t)(విలువ & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(విలువ >> 32));
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_ctzll (uint64_t విలువ)
{
    uint32_t msh = (uint32_t)(విలువ >> 32);
    uint32_t lsh = (uint32_t)(విలువ & 0xFFFFFFFF);
    ఒకవేళ (lsh != 0) {
        తిరిగి __builtin_ctz(lsh);
    }
    తిరిగి 32 + __builtin_ctz(msh);
}

స్థిర అంతఃవరుస
పూర్ణాంకం __builtin_clzll (uint64_t విలువ)
{
    uint32_t msh = (uint32_t)(విలువ >> 32);
    uint32_t lsh = (uint32_t)(విలువ & 0xFFFFFFFF);
    ఒకవేళ (msh != 0) {
        తిరిగి __builtin_clz(msh);
    }
    తిరిగి 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

స్థిర అంతఃవరుస
f32 m3_negf (f32 x)
{
    u32 బిట్లు;
    జ్ఞాపకం_నకలు(&బిట్లు, &x, పరిమాణం(బిట్లు));
    బిట్లు ^= 0x80000000u;
    జ్ఞాపకం_నకలు(&x, &బిట్లు, పరిమాణం(x));
    తిరిగి x;
}

స్థిర అంతఃవరుస
f64 m3_neg (f64 x)
{
    u64 బిట్లు;
    జ్ఞాపకం_నకలు(&బిట్లు, &x, పరిమాణం(బిట్లు));
    బిట్లు ^= 0x8000000000000000ull;
    జ్ఞాపకం_నకలు(&x, &బిట్లు, పరిమాణం(x));
    తిరిగి x;
}

#అయితే_ముగింపు // defined(M3_COMPILER_TCC)

#ఒకవేళ defined(__AVR__)

స్థిర అంతఃవరుస
దశాంశం rintf (దశాంశం arg)
{
    సంఘం {
        దశాంశం    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    ఒకవేళ (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        తిరిగి arg;
    }
    తిరిగి (దశాంశం)lrint(arg);
}

స్థిర అంతఃవరుస
ద్విగుణ rint (ద్విగుణ arg)
{
    సంఘం {
        ద్విగుణ   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    ఒకవేళ (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        తిరిగి arg;
    }
    తిరిగి (ద్విగుణ)lrint(arg);
}

స్థిర అంతఃవరుస
uint64_t strtoull (స్థిరరాశి అక్షరం* str, అక్షరం** endptr, పూర్ణాంకం పునాది)
{
    uint64_t    ఫలితం = 0;
    స్థిరరాశి అక్షరం* p = str;

    అయితే (*p == ' ' || *p == '\t') {
        p++;
    }

    ఒకవేళ (పునాది == 0) {
        ఒకవేళ (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            పునాది = 16;
            p += 2;
        } లేకపోతే ఒకవేళ (*p == '0') {
            పునాది = 8;
            p++;
        } లేకపోతే {
            పునాది = 10;
        }
    } లేకపోతే ఒకవేళ (పునాది == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    అయితే (*p) {
        పూర్ణాంకం digit;
        ఒకవేళ (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } లేకపోతే ఒకవేళ (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } లేకపోతే ఒకవేళ (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } లేకపోతే {
            ఆపు;
        }
        ఒకవేళ (digit >= పునాది) {
            ఆపు;
        }
        ఫలితం = ఫలితం * పునాది + digit;
        p++;
    }

    ఒకవేళ (endptr) {
        *endptr = (అక్షరం*)p;
    }
    తిరిగి ఫలితం;
}

#అయితే_ముగింపు

/*
 * Rotr, Rotl
 */

స్థిర అంతఃవరుస
u32 rotl32 (u32 n, గుర్తులేని c)
{
    స్థిరరాశి గుర్తులేని ముసుగు = CHAR_BIT * పరిమాణం(n) - 1;
    c &= ముసుగు & 31;
    తిరిగి (n << c) | (n >> ((32 - c) & ముసుగు));
}

స్థిర అంతఃవరుస
u32 rotr32 (u32 n, గుర్తులేని c)
{
    స్థిరరాశి గుర్తులేని ముసుగు = CHAR_BIT * పరిమాణం(n) - 1;
    c &= ముసుగు & 31;
    తిరిగి (n >> c) | (n << ((32 - c) & ముసుగు));
}

స్థిర అంతఃవరుస
u64 rotl64 (u64 n, u64 c)
{
    స్థిరరాశి గుర్తులేని ముసుగు = CHAR_BIT * పరిమాణం(n) - 1;
    c &= ముసుగు & 63;
    తిరిగి (n << c) | (n >> ((64 - c) & ముసుగు));
}

స్థిర అంతఃవరుస
u64 rotr64 (u64 n, u64 c)
{
    స్థిరరాశి గుర్తులేని ముసుగు = CHAR_BIT * పరిమాణం(n) - 1;
    c &= ముసుగు & 63;
    తిరిగి (n >> c) | (n << ((64 - c) & ముసుగు));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

స్థిర అంతఃవరుస
u64 mul_wide_u64 (u64 a, u64 b, u64* o_ఎగువ)
{
#ఒకవేళ defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    గుర్తులేని __int128 లబ్ధం = (గుర్తులేని __int128)a * b;

    *o_ఎగువ = (u64)(లబ్ధం >> 64);
    తిరిగి (u64)లబ్ధం;
#లేకపోతే
    // four 32 x 32 partial products of the halves, a1:a0 times b1:b0. The middle
    // column collects the carry out of the low one and the low halves of the two
    // cross terms, at most 3 * (2^32 - 1).
    u64 a0 = (u32)a, a1 = a >> 32;
    u64 b0 = (u32)b, b1 = b >> 32;

    u64 ll = a0 * b0;
    u64 lh = a0 * b1;
    u64 hl = a1 * b0;
    u64 hh = a1 * b1;

    u64 మధ్య = (ll >> 32) + (u32)lh + (u32)hl;

    *o_ఎగువ = hh + (lh >> 32) + (hl >> 32) + (మధ్య >> 32);
    తిరిగి (మధ్య << 32) | (u32)ll;
#అయితే_ముగింపు
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
స్థిర అంతఃవరుస
u64 mul_wide_i64 (i64 a, i64 b, u64* o_ఎగువ)
{
    u64 దిగువ = mul_wide_u64((u64)a, (u64)b, o_ఎగువ);

    ఒకవేళ (a < 0) {
        *o_ఎగువ -= (u64)b;
    }
    ఒకవేళ (b < 0) {
        *o_ఎగువ -= (u64)a;
    }

    తిరిగి దిగువ;
}

/*
 * Integer Div, Rem
 */

#నిర్వచించు OP_DIV_U(RES, A, B)                                         \
    ఒకవేళ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#నిర్వచించు OP_REM_U(RES, A, B)                                         \
    ఒకవేళ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#ఒకవేళ (INT_MIN != -గరిష్ఠ_పూర్ణాంకం)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      ఒకవేళ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      ఒకవేళ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3_Err_trap_Integer_అధికప్రవాహం);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      ఒకవేళ (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      ఒకవేళ (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      లేకపోతే RES = A % B;

#లేకపోతే

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#అయితే_ముగింపు

/*
 * Trunc
 */

#నిర్వచించు OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    ఒకవేళ (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    ఒకవేళ (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
        newTrap (m3_Err_trap_Integer_అధికప్రవాహం);                \
    }                                                       \
    RES = (TYPE)A;


#నిర్వచించు OP_I32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i32, -2147483904.0f, 2147483648.0f)
#నిర్వచించు OP_U32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u32,          -1.0f, 4294967296.0f)
#నిర్వచించు OP_I32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i32, -2147483649.0 , 2147483648.0 )
#నిర్వచించు OP_U32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u32,          -1.0 , 4294967296.0 )

#నిర్వచించు OP_I64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f)
#నిర్వచించు OP_U64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0f, 18446744073709551616.0f)
#నిర్వచించు OP_I64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0 )
#నిర్వచించు OP_U64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0 , 18446744073709551616.0 )

#నిర్వచించు OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX)  \
    ఒకవేళ (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } లేకపోతే ఒకవేళ (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } లేకపోతే ఒకవేళ (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } లేకపోతే {                                                \
        RES = (TYPE)A;                                      \
    }

#నిర్వచించు OP_I32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483904.0f, 2147483648.0f,   INT32_MIN,  INT32_MAX)
#నిర్వచించు OP_U32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0f, 4294967296.0f,         0UL, UINT32_MAX)
#నిర్వచించు OP_I32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483649.0 , 2147483648.0,    INT32_MIN,  INT32_MAX)
#నిర్వచించు OP_U32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0 , 4294967296.0,          0UL, UINT32_MAX)

#నిర్వచించు OP_I64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f, INT64_MIN,  INT64_MAX)
#నిర్వచించు OP_U64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0f, 18446744073709551616.0f,      0ULL, UINT64_MAX)
#నిర్వచించు OP_I64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0,  INT64_MIN,  INT64_MAX)
#నిర్వచించు OP_U64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0 , 18446744073709551616.0,       0ULL, UINT64_MAX)

/*
 * Min, Max
 */

#ఒకవేళ d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

స్థిర అంతఃవరుస
f32 quiet_nan_f32 (f32 arg)
{
    సంఘం {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    తిరిగి u.f;
}

స్థిర అంతఃవరుస
f64 quiet_nan_f64 (f64 arg)
{
    సంఘం {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    తిరిగి u.f;
}

స్థిర అంతఃవరుస
f32 min_f32 (f32 a, f32 b)
{
    ఒకవేళ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        తిరిగి quiet_nan_f32(isnan(a) ? a : b);
    }
    ఒకవేళ (M3_UNLIKELY(a == 0 and a == b)) {
        తిరిగి signbit(a) ? a : b;
    }
    తిరిగి a > b ? b : a;
}

స్థిర అంతఃవరుస
f32 max_f32 (f32 a, f32 b)
{
    ఒకవేళ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        తిరిగి quiet_nan_f32(isnan(a) ? a : b);
    }
    ఒకవేళ (M3_UNLIKELY(a == 0 and a == b)) {
        తిరిగి signbit(a) ? b : a;
    }
    తిరిగి a > b ? a : b;
}

స్థిర అంతఃవరుస
f64 min_f64 (f64 a, f64 b)
{
    ఒకవేళ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        తిరిగి quiet_nan_f64(isnan(a) ? a : b);
    }
    ఒకవేళ (M3_UNLIKELY(a == 0 and a == b)) {
        తిరిగి signbit(a) ? a : b;
    }
    తిరిగి a > b ? b : a;
}

స్థిర అంతఃవరుస
f64 max_f64 (f64 a, f64 b)
{
    ఒకవేళ (M3_UNLIKELY(isnan(a) or isnan(b))) {
        తిరిగి quiet_nan_f64(isnan(a) ? a : b);
    }
    ఒకవేళ (M3_UNLIKELY(a == 0 and a == b)) {
        తిరిగి signbit(a) ? b : a;
    }
    తిరిగి a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  ఒకవేళ d_m3CanonicalNaN

స్థిర అంతఃవరుస
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    ఒకవేళ (M3_UNLIKELY(arg != arg)) {
        సంఘం {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        తిరిగి u.f;
    }
    తిరిగి arg;
}

స్థిర అంతఃవరుస
f64 canon_nan_f64 (f64 arg)
{
    ఒకవేళ (M3_UNLIKELY(arg != arg)) {
        సంఘం {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        తిరిగి u.f;
    }
    తిరిగి arg;
}

#  లేకపోతే

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#అయితే_ముగింపు

#అయితే_ముగింపు // m3_math_utils_h
