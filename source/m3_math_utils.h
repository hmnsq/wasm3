//
//  m3_math_utils.h
//
//  Created by Volodymyr Shymanksyy on 8/10/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#अगर_अघोषित m3_math_utils_h
#घोषणा m3_math_utils_h

#समावेश "m3_core.h"

#समावेश <limits.h>

#अगर defined(M3_COMPILER_MSVC)

#  include <intrin.h>

#  define __builtin_popcount    __popcnt

अटल अंतरभूत
पूर्णांक __builtin_ctz (uint32_t x)
{
    अचिन्हित दीर्घ ret;
    _BitScanForward(&ret, x);
    वापस (पूर्णांक)ret;
}

अटल अंतरभूत
पूर्णांक __builtin_clz (uint32_t x)
{
    अचिन्हित दीर्घ ret;
    _BitScanReverse(&ret, x);
    वापस (पूर्णांक)(31 ^ ret);
}


#  ifdef _WIN64

#    define __builtin_popcountll  __popcnt64

अटल अंतरभूत
पूर्णांक __builtin_ctzll (uint64_t मूल्य)
{
    अचिन्हित दीर्घ ret;
    _BitScanForward64(&ret, मूल्य);
    वापस (पूर्णांक)ret;
}

अटल अंतरभूत
पूर्णांक __builtin_clzll (uint64_t मूल्य)
{
    अचिन्हित दीर्घ ret;
    _BitScanReverse64(&ret, मूल्य);
    वापस (पूर्णांक)(63 ^ ret);
}

#  अन्यथा // _WIN64

#    define __builtin_popcountll(x)  (__popcnt((x) & 0xFFFFFFFF) + __popcnt((x) >> 32))

अटल अंतरभूत
पूर्णांक __builtin_ctzll (uint64_t मूल्य)
{
    //if (value == 0) return 64; // Note: ctz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    अगर (lsh != 0) {
        वापस __builtin_ctz(lsh);
    }
    वापस 32 + __builtin_ctz(msh);
}

अटल अंतरभूत
पूर्णांक __builtin_clzll (uint64_t मूल्य)
{
    //if (value == 0) return 64; // Note: clz(0) result is undefined anyway
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    अगर (msh != 0) {
        वापस __builtin_clz(msh);
    }
    वापस 32 + __builtin_clz(lsh);
}

#  endif // _WIN64

#पूर्ण_अगर // defined(M3_COMPILER_MSVC)

#अगर defined(M3_COMPILER_TCC)

// TinyCC provides none of the bit-counting builtins the interpreter uses

अटल अंतरभूत
पूर्णांक __builtin_popcount (uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0f0f0f0fu;
    वापस (पूर्णांक)((x * 0x01010101u) >> 24);
}

अटल अंतरभूत
पूर्णांक __builtin_ctz (uint32_t x)
{
    //if (x == 0) return 32; // Note: ctz(0) result is undefined anyway
    वापस __builtin_popcount((x & (~x + 1u)) - 1);
}

अटल अंतरभूत
पूर्णांक __builtin_clz (uint32_t x)
{
    //if (x == 0) return 32; // Note: clz(0) result is undefined anyway
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    वापस 32 - __builtin_popcount(x);
}

अटल अंतरभूत
पूर्णांक __builtin_popcountll (uint64_t मूल्य)
{
    वापस __builtin_popcount((uint32_t)(मूल्य & 0xFFFFFFFF)) +
           __builtin_popcount((uint32_t)(मूल्य >> 32));
}

अटल अंतरभूत
पूर्णांक __builtin_ctzll (uint64_t मूल्य)
{
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    अगर (lsh != 0) {
        वापस __builtin_ctz(lsh);
    }
    वापस 32 + __builtin_ctz(msh);
}

अटल अंतरभूत
पूर्णांक __builtin_clzll (uint64_t मूल्य)
{
    uint32_t msh = (uint32_t)(मूल्य >> 32);
    uint32_t lsh = (uint32_t)(मूल्य & 0xFFFFFFFF);
    अगर (msh != 0) {
        वापस __builtin_clz(msh);
    }
    वापस 32 + __builtin_clz(lsh);
}


// TinyCC compiles -x on floats as an arithmetic negation, which quiets signaling
// NaNs; f32.neg/f64.neg must only flip the sign bit and leave the payload alone

अटल अंतरभूत
f32 m3_negf (f32 x)
{
    u32 bits;
    स_नकल(&bits, &x, माप(bits));
    bits ^= 0x80000000u;
    स_नकल(&x, &bits, माप(x));
    वापस x;
}

अटल अंतरभूत
f64 m3_neg (f64 x)
{
    u64 bits;
    स_नकल(&bits, &x, माप(bits));
    bits ^= 0x8000000000000000ull;
    स_नकल(&x, &bits, माप(x));
    वापस x;
}

#पूर्ण_अगर // defined(M3_COMPILER_TCC)

#अगर defined(__AVR__)

अटल अंतरभूत
भग्न rintf (भग्न arg)
{
    जोड़ {
        भग्न    f;
        uint32_t i;
    } u;
    u.f = arg;
    uint32_t ux = u.i & 0x7FFFFFFF;
    अगर (M3_UNLIKELY(ux == 0 || ux > 0x5A000000)) {
        वापस arg;
    }
    वापस (भग्न)lrint(arg);
}

अटल अंतरभूत
द्विगुन rint (द्विगुन arg)
{
    जोड़ {
        द्विगुन   f;
        uint32_t i[2];
    } u;
    u.f = arg;
    uint32_t ux = u.i[1] & 0x7FFFFFFF;
    अगर (M3_UNLIKELY((ux == 0 && u.i[0] == 0) || ux > 0x433FFFFF)) {
        वापस arg;
    }
    वापस (द्विगुन)lrint(arg);
}

अटल अंतरभूत
uint64_t strtoull (स्थिर अक्षर* str, अक्षर** endptr, पूर्णांक base)
{
    uint64_t    परिणाम = 0;
    स्थिर अक्षर* p = str;

    जबतक (*p == ' ' || *p == '\t') {
        p++;
    }

    अगर (base == 0) {
        अगर (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
            base = 16;
            p += 2;
        } अन्यथा अगर (*p == '0') {
            base = 8;
            p++;
        } अन्यथा {
            base = 10;
        }
    } अन्यथा अगर (base == 16 && *p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        p += 2;
    }

    जबतक (*p) {
        पूर्णांक digit;
        अगर (*p >= '0' && *p <= '9') {
            digit = *p - '0';
        } अन्यथा अगर (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } अन्यथा अगर (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } अन्यथा {
            अवरोध;
        }
        अगर (digit >= base) {
            अवरोध;
        }
        परिणाम = परिणाम * base + digit;
        p++;
    }

    अगर (endptr) {
        *endptr = (अक्षर*)p;
    }
    वापस परिणाम;
}

#पूर्ण_अगर

/*
 * Rotr, Rotl
 */

अटल अंतरभूत
u32 rotl32 (u32 n, अचिन्हित c)
{
    स्थिर अचिन्हित mask = अक्षर_बिट * माप(n) - 1;
    c &= mask & 31;
    वापस (n << c) | (n >> ((32 - c) & mask));
}

अटल अंतरभूत
u32 rotr32 (u32 n, अचिन्हित c)
{
    स्थिर अचिन्हित mask = अक्षर_बिट * माप(n) - 1;
    c &= mask & 31;
    वापस (n >> c) | (n << ((32 - c) & mask));
}

अटल अंतरभूत
u64 rotl64 (u64 n, u64 c)
{
    स्थिर अचिन्हित mask = अक्षर_बिट * माप(n) - 1;
    c &= mask & 63;
    वापस (n << c) | (n >> ((64 - c) & mask));
}

अटल अंतरभूत
u64 rotr64 (u64 n, u64 c)
{
    स्थिर अचिन्हित mask = अक्षर_बिट * माप(n) - 1;
    c &= mask & 63;
    वापस (n >> c) | (n << ((64 - c) & mask));
}

/*
 * Wide multiply: the full 128-bit product of two 64-bit integers, as its low
 * half returned and its high half written through o_high
 */

अटल अंतरभूत
u64 mul_wide_u64 (u64 a, u64 b, u64* o_ऊपरी)
{
#अगर defined(__SIZEOF_INT128__) && (UINTPTR_MAX > UINT32_MAX || defined(__wasm_wide_arithmetic__))
    // a 64-bit target multiplies into a register pair in one instruction. A 32-bit
    // one that has the type anyway, wasm32 among them, would make a library call,
    // unless the target itself provides wide arithmetic instructions.
    अचिन्हित __int128 product = (अचिन्हित __int128)a * b;

    *o_ऊपरी = (u64)(product >> 64);
    वापस (u64)product;
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
    वापस (मध्य << 32) | (u32)ll;
#पूर्ण_अगर
}

// A negative factor reads 2^64 too large as unsigned, which puts 2^64 times the
// other factor too much into the product - one other factor too much in the high
// half, which is where it is taken back out
अटल अंतरभूत
u64 mul_wide_i64 (i64 a, i64 b, u64* o_ऊपरी)
{
    u64 निचला = mul_wide_u64((u64)a, (u64)b, o_ऊपरी);

    अगर (a < 0) {
        *o_ऊपरी -= (u64)b;
    }
    अगर (b < 0) {
        *o_ऊपरी -= (u64)a;
    }

    वापस निचला;
}

/*
 * Integer Div, Rem
 */

#घोषणा OP_DIV_U(RES, A, B)                                         \
    अगर (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A / B;

#घोषणा OP_REM_U(RES, A, B)                                         \
    अगर (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);    \
    RES = A % B;

// 2's complement detection
#अगर (पूर्णांक_न्यून != -पूर्णांक_उच्च)

#  define OP_DIV_S(RES, A, B, TYPE_MIN)                             \
      अगर (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      अगर (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) {                 \
          newTrap (m3Err_trapIntegerOverflow);                      \
      }                                                             \
      RES = A / B;

#  define OP_REM_S(RES, A, B, TYPE_MIN)                             \
      अगर (M3_UNLIKELY(B == 0)) newTrap (m3Err_trapDivisionByZero);  \
      अगर (M3_UNLIKELY(B == -1 and A == TYPE_MIN)) RES = 0;          \
      अन्यथा RES = A % B;

#अन्यथा

#  define OP_DIV_S(RES, A, B, TYPE_MIN) OP_DIV_U(RES, A, B)
#  define OP_REM_S(RES, A, B, TYPE_MIN) OP_REM_U(RES, A, B)

#पूर्ण_अगर

/*
 * Trunc
 */

#घोषणा OP_TRUNC(RES, A, TYPE, RMIN, RMAX)                  \
    अगर (M3_UNLIKELY(isnan(A))) {                            \
        newTrap (m3Err_trapIntegerConversion);              \
    }                                                       \
    अगर (M3_UNLIKELY(A <= RMIN or A >= RMAX)) {              \
        newTrap (m3Err_trapIntegerOverflow);                \
    }                                                       \
    RES = (TYPE)A;


#घोषणा OP_I32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i32, -2147483904.0f, 2147483648.0f)
#घोषणा OP_U32_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u32,          -1.0f, 4294967296.0f)
#घोषणा OP_I32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i32, -2147483649.0 , 2147483648.0 )
#घोषणा OP_U32_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u32,          -1.0 , 4294967296.0 )

#घोषणा OP_I64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f)
#घोषणा OP_U64_TRUNC_F32(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0f, 18446744073709551616.0f)
#घोषणा OP_I64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0 )
#घोषणा OP_U64_TRUNC_F64(RES, A)    OP_TRUNC(RES, A, u64,                   -1.0 , 18446744073709551616.0 )

#घोषणा OP_TRUNC_SAT(RES, A, TYPE, RMIN, RMAX, IMIN, IMAX)  \
    अगर (M3_UNLIKELY(isnan(A))) {                            \
        RES = 0;                                            \
    } अन्यथा अगर (M3_UNLIKELY(A <= RMIN)) {                    \
        RES = IMIN;                                         \
    } अन्यथा अगर (M3_UNLIKELY(A >= RMAX)) {                    \
        RES = IMAX;                                         \
    } अन्यथा {                                                \
        RES = (TYPE)A;                                      \
    }

#घोषणा OP_I32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483904.0f, 2147483648.0f,   INT32_MIN,  INT32_MAX)
#घोषणा OP_U32_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0f, 4294967296.0f,         0UL, UINT32_MAX)
#घोषणा OP_I32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i32, -2147483649.0 , 2147483648.0,    INT32_MIN,  INT32_MAX)
#घोषणा OP_U32_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u32,          -1.0 , 4294967296.0,          0UL, UINT32_MAX)

#घोषणा OP_I64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223373136366403584.0f,  9223372036854775808.0f, INT64_MIN,  INT64_MAX)
#घोषणा OP_U64_TRUNC_SAT_F32(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0f, 18446744073709551616.0f,      0ULL, UINT64_MAX)
#घोषणा OP_I64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, i64, -9223372036854777856.0 ,  9223372036854775808.0,  INT64_MIN,  INT64_MAX)
#घोषणा OP_U64_TRUNC_SAT_F64(RES, A)    OP_TRUNC_SAT(RES, A, u64,                   -1.0 , 18446744073709551616.0,       0ULL, UINT64_MAX)

/*
 * Min, Max
 */

#अगर d_m3HasFloat

#  include <math.h>

// Propagate a NaN operand the way the arithmetic ops do.
// keep its sign and payload, but force the quiet bit, since the spec requires
// min/max to produce an arithmetic NaN.

अटल अंतरभूत
f32 quiet_nan_f32 (f32 arg)
{
    जोड़ {
        f32 f;
        u32 i;
    } u;
    u.f = arg;
    u.i |= 0x00400000;
    वापस u.f;
}

अटल अंतरभूत
f64 quiet_nan_f64 (f64 arg)
{
    जोड़ {
        f64 f;
        u64 i;
    } u;
    u.f = arg;
    u.i |= 0x0008000000000000ULL;
    वापस u.f;
}

अटल अंतरभूत
f32 min_f32 (f32 a, f32 b)
{
    अगर (M3_UNLIKELY(isnan(a) or isnan(b))) {
        वापस quiet_nan_f32(isnan(a) ? a : b);
    }
    अगर (M3_UNLIKELY(a == 0 and a == b)) {
        वापस signbit(a) ? a : b;
    }
    वापस a > b ? b : a;
}

अटल अंतरभूत
f32 max_f32 (f32 a, f32 b)
{
    अगर (M3_UNLIKELY(isnan(a) or isnan(b))) {
        वापस quiet_nan_f32(isnan(a) ? a : b);
    }
    अगर (M3_UNLIKELY(a == 0 and a == b)) {
        वापस signbit(a) ? b : a;
    }
    वापस a > b ? a : b;
}

अटल अंतरभूत
f64 min_f64 (f64 a, f64 b)
{
    अगर (M3_UNLIKELY(isnan(a) or isnan(b))) {
        वापस quiet_nan_f64(isnan(a) ? a : b);
    }
    अगर (M3_UNLIKELY(a == 0 and a == b)) {
        वापस signbit(a) ? a : b;
    }
    वापस a > b ? b : a;
}

अटल अंतरभूत
f64 max_f64 (f64 a, f64 b)
{
    अगर (M3_UNLIKELY(isnan(a) or isnan(b))) {
        वापस quiet_nan_f64(isnan(a) ? a : b);
    }
    अगर (M3_UNLIKELY(a == 0 and a == b)) {
        वापस signbit(a) ? b : a;
    }
    वापस a > b ? a : b;
}

// The NaN the spec calls canonical: quiet, positive, and with nothing in its
// payload. Where an operation's result is a NaN, the spec allows any payload and any
// sign, and the hardware picks - differently on different architectures. These pin
// that down, so that a module's results are the same wherever it runs.
//
// Only operations the spec says produce an *arithmetic* NaN go through this. abs,
// neg and copysign are defined bitwise and carry their operand's payload through by
// design; canonicalizing them would be wrong, not merely stricter.
#  अगर d_m3CanonicalNaN

अटल अंतरभूत
f32 canon_nan_f32 (f32 arg)
{
    // arg != arg is the NaN test that does not need <math.h> to have been told to
    // keep it, and it is the whole of the cost when the value is not a NaN
    अगर (M3_UNLIKELY(arg != arg)) {
        जोड़ {
            f32 f;
            u32 i;
        } u;
        u.i = 0x7FC00000u;
        वापस u.f;
    }
    वापस arg;
}

अटल अंतरभूत
f64 canon_nan_f64 (f64 arg)
{
    अगर (M3_UNLIKELY(arg != arg)) {
        जोड़ {
            f64 f;
            u64 i;
        } u;
        u.i = 0x7FF8000000000000ULL;
        वापस u.f;
    }
    वापस arg;
}

#  अन्यथा

#    define canon_nan_f32(ARG)  (ARG)
#    define canon_nan_f64(ARG)  (ARG)

#  endif // d_m3CanonicalNaN

#पूर्ण_अगर

#पूर्ण_अगर // m3_math_utils_h
