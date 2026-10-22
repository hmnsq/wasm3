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

ಪ್ರಕಾರನಿರ್ದೇಶ uint32_t wasm_ptr_t;
ಪ್ರಕಾರನಿರ್ದೇಶ uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (ಶೂನ್ಯ)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ಶೂನ್ಯ*, i_ptr)
    m3ApiGetArg(int32_t, i_ಮೌಲ್ಯ)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_ಮೌಲ್ಯ, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ಶೂನ್ಯ*, o_dst)
    m3ApiGetArgMem(ಶೂನ್ಯ*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(ಶೂನ್ಯ*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

ಸ್ಥಿರ
ಶೂನ್ಯ internal_itoa (ಪೂರ್ಣಾಙ್ಕ n, ವರ್ಣ s[], ಪೂರ್ಣಾಙ್ಕ radix)
{
    ಸ್ಥಿರ ವರ್ಣ ಧ್ರುವ HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    ಪೂರ್ಣಾಙ್ಕ  i, j, sign;
    ವರ್ಣ c;

    ಯದಿ ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    ಕುರು {
        s[i++] = HEXDIGITS[n % radix];
    } ಯಾವತ್ ((n /= radix) > 0);

    ಯದಿ (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    ಪ್ರತಿ (i = 0, j = (ಪೂರ್ಣಾಙ್ಕ)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    ಯದಿ (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    ವರ್ಣ ch;
    ಯಾವತ್ ((ch = *i_fmt++)) {
        ಯದಿ ('%' != ch) {
            putc(ch, file);
            length++;
            ಅನುವರ್ತ;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        ಯದಿ (not *i_fmt) {
            ಭಙ್ಗ;
        }
        ch = *i_fmt++;
        ವಿಕಲ್ಪ (ch) {
        ಪಕ್ಷ 'c': {
            m3ApiCheckMem(i_args, ಪರಿಮಾಣ(wasm_ptr_t));
            ವರ್ಣ char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 'd':
        ಪಕ್ಷ 'x': {
            m3ApiCheckMem(i_args, ಪರಿಮಾಣ(wasm_ptr_t));
            ಪೂರ್ಣಾಙ್ಕ  int_temp   = *i_args++;
            ವರ್ಣ buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, ಪರಿಮಾಣ(buffer));
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 's': {
            m3ApiCheckMem(i_args, ಪರಿಮಾಣ(wasm_ptr_t));
            ಧ್ರುವ ವರ್ಣ* string_temp;
            size_t      string_len;

            string_temp = (ಧ್ರುವ ವರ್ಣ*)m3ApiOffsetToPtr(*i_args++);
            ಯದಿ (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } ಅನ್ಯಥಾ {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            ಭಙ್ಗ;
        ಸ್ವಭಾವ:
            fputc(ch, file);
            length++;
            ಭಙ್ಗ;
        }
        }
    }

    m3ApiReturn(length);
}

ಸ್ಥಿರ
uint64_t clock_ms (IM3Runtime i_runtime)
{
#ಯದಿ d_m3DeterministicProfile
    ಪ್ರತ್ಯಾಗಮ i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ಧ್ರುವ clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    ಪ್ರತ್ಯಾಗಮ (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ (uint64_t)clock();
#  endif
#endif
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
M3Result SuppressLookupFailure (M3Result i_result)
{
    ಯದಿ (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
ಸ್ಥಿರ
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    ಯದಿ (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    ಪ್ರತ್ಯಾಗಮ SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    ಧ್ರುವ ವರ್ಣ* env = "env";
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
    ಪ್ರತ್ಯಾಗಮ result;
}
