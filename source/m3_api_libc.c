//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_api_libc.h"

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_deterministic.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"

#ಅನ್ತರ್ಭಾವ <time.h>
#ಅನ್ತರ್ಭಾವ <errno.h>
#ಅನ್ತರ್ಭಾವ <stdio.h>

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ uint32_t wasm_ptr_t;
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ uint32_t wasm_ಗಾತ್ರ_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, ಸಂಕೇತ_2)
    (ಶೂನ್ಯ)ಸಂಕೇತ_2;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ಶೂನ್ಯ*, i_ptr)
    m3ApiGetArg(int32_t, i_ಮೌಲ್ಯ)
    m3ApiGetArg(wasm_ಗಾತ್ರ_t, i_ಗಾತ್ರ)

    m3ApiCheckMem(i_ptr, i_ಗಾತ್ರ);

    u32 ಫಲಿತಾಂಶ = m3_Api_Ptr_ಗಮ್ಯ_ಸ್ಥಾನಾಂತರ(memset(i_ptr, i_ಮೌಲ್ಯ, i_ಗಾತ್ರ));
    m3ApiReturn(ಫಲಿತಾಂಶ);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ಶೂನ್ಯ*, o_dst)
    m3ApiGetArgMem(ಶೂನ್ಯ*, i_src)
    m3ApiGetArg(wasm_ಗಾತ್ರ_t, i_ಗಾತ್ರ)

    m3ApiCheckMem(o_dst, i_ಗಾತ್ರ);
    m3ApiCheckMem(i_src, i_ಗಾತ್ರ);

    u32 ಫಲಿತಾಂಶ = m3_Api_Ptr_ಗಮ್ಯ_ಸ್ಥಾನಾಂತರ(memmove(o_dst, i_src, i_ಗಾತ್ರ));
    m3ApiReturn(ಫಲಿತಾಂಶ);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(ಶೂನ್ಯ*, i_ptr)
    m3ApiGetArg(wasm_ಗಾತ್ರ_t, i_ಗಾತ್ರ)

    m3ApiCheckMem(i_ptr, i_ಗಾತ್ರ);

    fwrite(i_ptr, i_ಗಾತ್ರ, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_ಗಾತ್ರ);
}

ಸ್ಥಿರ
ಶೂನ್ಯ ಆಂತರಿಕ_itoa (ಪೂರ್ಣಾಂಕ n, ಅಕ್ಷರ s[], ಪೂರ್ಣಾಂಕ radix)
{
    ಸ್ಥಿರ ಅಕ್ಷರ ಸ್ಥಿರಾಂಕ HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    ಪೂರ್ಣಾಂಕ  i, j, sign;
    ಅಕ್ಷರ c;

    ಆದರೆ ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    ಮಾಡು {
        s[i++] = HEXDIGITS[n % radix];
    } ಆಗ ((n /= radix) > 0);

    ಆದರೆ (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    //ಹಿಮ್ಮುಖ
    ಗಾಗಿ (i = 0, j = (ಪೂರ್ಣಾಂಕ)ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_ವಾದಗಳು)

    ಆದರೆ (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t ಉದ್ದಳತೆ = 0;

    ಅಕ್ಷರ ch;
    ಆಗ ((ch = *i_fmt++)) {
        ಆದರೆ ('%' != ch) {
            putc(ch, file);
            ಉದ್ದಳತೆ++;
            ಮುಂದುವರಿ;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        ಆದರೆ (not *i_fmt) {
            ನಿಲ್ಲಿಸು;
        }
        ch = *i_fmt++;
        ಆಯ್ಕೆ (ch) {
        ಸ್ಥಿತಿ 'c': {
            m3ApiCheckMem(i_ವಾದಗಳು, ಗಾತ್ರ(wasm_ptr_t));
            ಅಕ್ಷರ char_ತಾತ್ಕಾಲಿಕ = *i_ವಾದಗಳು++;
            fputc(char_ತಾತ್ಕಾಲಿಕ, file);
            ಉದ್ದಳತೆ++;
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 'd':
        ಸ್ಥಿತಿ 'x': {
            m3ApiCheckMem(i_ವಾದಗಳು, ಗಾತ್ರ(wasm_ptr_t));
            ಪೂರ್ಣಾಂಕ  int_ತಾತ್ಕಾಲಿಕ   = *i_ವಾದಗಳು++;
            ಅಕ್ಷರ ಬಫರ್[32] = { 0 };
            ಆಂತರಿಕ_itoa(int_ತಾತ್ಕಾಲಿಕ, ಬಫರ್, (ch == 'x') ? 16 : 10);
            fputs(ಬಫರ್, file);
            ಉದ್ದಳತೆ += (int32_t)strnlen(ಬಫರ್, ಗಾತ್ರ(ಬಫರ್));
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 's': {
            m3ApiCheckMem(i_ವಾದಗಳು, ಗಾತ್ರ(wasm_ptr_t));
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* string_ತಾತ್ಕಾಲಿಕ;
            size_t      string_len;

            string_ತಾತ್ಕಾಲಿಕ = (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*)m3_Api_ಸ್ಥಾನಾಂತರ_ಗಮ್ಯ_Ptr(*i_ವಾದಗಳು++);
            ಆದರೆ (m3ApiIsNullPtr(string_ತಾತ್ಕಾಲಿಕ)) {
                string_ತಾತ್ಕಾಲಿಕ = "(null)";
                string_len  = 6;
            } ಇಲ್ಲದಿದ್ದರೆ {
                string_len = strnlen(string_ತಾತ್ಕಾಲಿಕ, 1024);
                m3ApiCheckMem(string_ತಾತ್ಕಾಲಿಕ, string_len + 1);
            }

            fwrite(string_ತಾತ್ಕಾಲಿಕ, 1, string_len, file);
            ಉದ್ದಳತೆ += (int32_t)string_len;
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ:
            fputc(ch, file);
            ಉದ್ದಳತೆ++;
            ನಿಲ್ಲಿಸು;
        }
        }
    }

    m3ApiReturn(ಉದ್ದಳತೆ);
}

ಸ್ಥಿರ
uint64_t clock_ms (IM3Runtime i_runtime)
{
#ಆದರೆ d_m3DeterministicProfile
    ಹಿಂತಿರುಗಿ i_runtime ? Deterministic_ಸಮಯ(i_runtime) / 1000000 : 0;
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ಸ್ಥಿರಾಂಕ clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    ಹಿಂತಿರುಗಿ (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ (uint64_t)clock();
#  endif
#ಆದರೆ_ಅಂತ್ಯ
}

m3ApiRawFunction(m3_libc_clock_ms)
{
    m3ApiReturnType(uint32_t)

    m3ApiReturn((uint32_t)clock_ms(runtime));
}

m3ApiRawFunction(m3_libc_clock_ms_i64)
{
    m3ApiReturnType(uint64_t)

    m3ApiReturn(clock_ms(runtime));
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SuppressLookupFailure (M3_ಫಲಿತಾಂಶ i_ಫಲಿತಾಂಶ)
{
    ಆದರೆ (i_ಫಲಿತಾಂಶ == m3Err_functionLookupFailed or i_ಫಲಿತಾಂಶ == m3Err_globalLookupFailed) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ i_ಫಲಿತಾಂಶ;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    ಆದರೆ (ಫಲಿತಾಂಶ == m3Err_functionSignatureMismatch) {
        ಫಲಿತಾಂಶ = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    ಹಿಂತಿರುಗಿ SuppressLookupFailure(ಫಲಿತಾಂಶ);
}

M3_ಫಲಿತಾಂಶ m3_LinkLibC (IM3Module module)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* env = "env";
    // clang-format off

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "_debug",            "i(*i)",   &m3_libc_print)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "_memset",           "*(*ii)",  &m3_libc_memset)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "_memmove",          "*(**i)",  &m3_libc_memmove)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "_memcpy",           "*(**i)",  &m3_libc_memmove))); // just alias of memmove
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "_abort",            "v()",     &m3_libc_abort)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "_exit",             "v(i)",    &m3_libc_exit)));
_   (LinkClockMs(module, env));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "printf",            "i(**)",   &m3_libc_printf)));

    // clang-format on

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
