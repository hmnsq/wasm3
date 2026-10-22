//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#અન્તર્ભાવ "m3_api_libc.h"

#અન્તર્ભાવ "m3_env.h"
#અન્તર્ભાવ "m3_deterministic.h"
#અન્તર્ભાવ "m3_exception.h"

#અન્તર્ભાવ <time.h>
#અન્તર્ભાવ <errno.h>
#અન્તર્ભાવ <stdio.h>

પ્રકારવ્યાખ્યા uint32_t wasm_ptr_t;
પ્રકારવ્યાખ્યા uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (શૂન્ય)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(શૂન્ય*, i_ptr)
    m3ApiGetArg(int32_t, i_કિંમત)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_કિંમત, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(શૂન્ય*, o_dst)
    m3ApiGetArgMem(શૂન્ય*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(શૂન્ય*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

સ્થિર
શૂન્ય internal_itoa (પૂર્ણાંક n, અક્ષર s[], પૂર્ણાંક radix)
{
    સ્થિર અક્ષર અચલ HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    પૂર્ણાંક  i, j, sign;
    અક્ષર c;

    જો ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    કરો {
        s[i++] = HEXDIGITS[n % radix];
    } જ્યારે ((n /= radix) > 0);

    જો (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    માટે (i = 0, j = (પૂર્ણાંક)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(અચલ અક્ષર*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    જો (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    અક્ષર ch;
    જ્યારે ((ch = *i_fmt++)) {
        જો ('%' != ch) {
            putc(ch, file);
            length++;
            ચાલુ;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        જો (not *i_fmt) {
            તોડો;
        }
        ch = *i_fmt++;
        પસંદગી (ch) {
        સ્થિતિ 'c': {
            m3ApiCheckMem(i_args, કદ(wasm_ptr_t));
            અક્ષર char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            તોડો;
        }
        સ્થિતિ 'd':
        સ્થિતિ 'x': {
            m3ApiCheckMem(i_args, કદ(wasm_ptr_t));
            પૂર્ણાંક  int_temp   = *i_args++;
            અક્ષર buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, કદ(buffer));
            તોડો;
        }
        સ્થિતિ 's': {
            m3ApiCheckMem(i_args, કદ(wasm_ptr_t));
            અચલ અક્ષર* string_temp;
            size_t      string_len;

            string_temp = (અચલ અક્ષર*)m3ApiOffsetToPtr(*i_args++);
            જો (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } નહીંતર {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            તોડો;
        મૂળભૂત:
            fputc(ch, file);
            length++;
            તોડો;
        }
        }
    }

    m3ApiReturn(length);
}

સ્થિર
uint64_t clock_ms (IM3Runtime i_runtime)
{
#જો d_m3DeterministicProfile
    પાછા i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#નહીંતર
    (શૂન્ય)i_runtime;

#  ifdef CLOCKS_PER_SEC
    અચલ clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    પાછા (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  નહીંતર
    પાછા (uint64_t)clock();
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

સ્થિર
M3Result SuppressLookupFailure (M3Result i_result)
{
    જો (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        પાછા m3Err_none;
    } નહીંતર {
        પાછા i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
સ્થિર
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    જો (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    પાછા SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    અચલ અક્ષર* env = "env";
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
    પાછા result;
}
