//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ଅନ୍ତର୍ଭାଵ "m3_api_libc.h"

#ଅନ୍ତର୍ଭାଵ "m3_env.h"
#ଅନ୍ତର୍ଭାଵ "m3_deterministic.h"
#ଅନ୍ତର୍ଭାଵ "m3_exception.h"

#ଅନ୍ତର୍ଭାଵ <time.h>
#ଅନ୍ତର୍ଭାଵ <errno.h>
#ଅନ୍ତର୍ଭାଵ <stdio.h>

ପ୍ରକାରବ୍ୟାଖ୍ୟା uint32_t wasm_ptr_t;
ପ୍ରକାରବ୍ୟାଖ୍ୟା uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (ଶୂନ୍ୟ)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ଶୂନ୍ୟ*, i_ptr)
    m3ApiGetArg(int32_t, i_ମୂଲ୍ୟ)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_ମୂଲ୍ୟ, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ଶୂନ୍ୟ*, o_dst)
    m3ApiGetArgMem(ଶୂନ୍ୟ*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(ଶୂନ୍ୟ*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

ସ୍ଥିର
ଶୂନ୍ୟ internal_itoa (ପୂର୍ଣ୍ଣସଂଖ୍ୟା n, ଅକ୍ଷର s[], ପୂର୍ଣ୍ଣସଂଖ୍ୟା radix)
{
    ସ୍ଥିର ଅକ୍ଷର ଅଚଳ HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା  i, j, sign;
    ଅକ୍ଷର c;

    ଯଦି ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    କର {
        s[i++] = HEXDIGITS[n % radix];
    } ଯେତେବେଳେ ((n /= radix) > 0);

    ଯଦି (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    ପାଇଁ (i = 0, j = (ପୂର୍ଣ୍ଣସଂଖ୍ୟା)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    ଯଦି (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    ଅକ୍ଷର ch;
    ଯେତେବେଳେ ((ch = *i_fmt++)) {
        ଯଦି ('%' != ch) {
            putc(ch, file);
            length++;
            ଜାରି;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        ଯଦି (not *i_fmt) {
            ଭାଙ୍ଗ;
        }
        ch = *i_fmt++;
        ବାଛ (ch) {
        ସ୍ଥିତି 'c': {
            m3ApiCheckMem(i_args, ଆକାର(wasm_ptr_t));
            ଅକ୍ଷର char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି 'd':
        ସ୍ଥିତି 'x': {
            m3ApiCheckMem(i_args, ଆକାର(wasm_ptr_t));
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା  int_temp   = *i_args++;
            ଅକ୍ଷର buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, ଆକାର(buffer));
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି 's': {
            m3ApiCheckMem(i_args, ଆକାର(wasm_ptr_t));
            ଅଚଳ ଅକ୍ଷର* string_temp;
            size_t      string_len;

            string_temp = (ଅଚଳ ଅକ୍ଷର*)m3ApiOffsetToPtr(*i_args++);
            ଯଦି (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } ଅନ୍ୟଥା {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
            fputc(ch, file);
            length++;
            ଭାଙ୍ଗ;
        }
        }
    }

    m3ApiReturn(length);
}

ସ୍ଥିର
uint64_t clock_ms (IM3Runtime i_runtime)
{
#ଯଦି d_m3DeterministicProfile
    ଫେରନ୍ତୁ i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ଅଚଳ clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    ଫେରନ୍ତୁ (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  ଅନ୍ୟଥା
    ଫେରନ୍ତୁ (uint64_t)clock();
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

ସ୍ଥିର
M3Result SuppressLookupFailure (M3Result i_result)
{
    ଯଦି (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
ସ୍ଥିର
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    ଯଦି (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    ଫେରନ୍ତୁ SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    ଅଚଳ ଅକ୍ଷର* env = "env";
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
    ଫେରନ୍ତୁ result;
}
