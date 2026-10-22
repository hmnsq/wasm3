//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#അന്തര്ഭാവ "m3_api_libc.h"

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_deterministic.h"
#അന്തര്ഭാവ "m3_exception.h"

#അന്തര്ഭാവ <time.h>
#അന്തര്ഭാവ <errno.h>
#അന്തര്ഭാവ <stdio.h>

തരനിർവചനം uint32_t wasm_ptr_t;
തരനിർവചനം uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (ശൂന്യം)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ശൂന്യം*, i_ptr)
    m3ApiGetArg(int32_t, i_മൂല്യം)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_മൂല്യം, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ശൂന്യം*, o_dst)
    m3ApiGetArgMem(ശൂന്യം*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(ശൂന്യം*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

സ്ഥിതം
ശൂന്യം internal_itoa (പൂർണ്ണസംഖ്യ n, അക്ഷരം s[], പൂർണ്ണസംഖ്യ radix)
{
    സ്ഥിതം അക്ഷരം സ്ഥിരം HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    പൂർണ്ണസംഖ്യ  i, j, sign;
    അക്ഷരം c;

    എങ്കിൽ ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    ചെയ്യുക {
        s[i++] = HEXDIGITS[n % radix];
    } ആകുമ്പോൾ ((n /= radix) > 0);

    എങ്കിൽ (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    വേണ്ടി (i = 0, j = (പൂർണ്ണസംഖ്യ)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    എങ്കിൽ (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    അക്ഷരം ch;
    ആകുമ്പോൾ ((ch = *i_fmt++)) {
        എങ്കിൽ ('%' != ch) {
            putc(ch, file);
            length++;
            തുടരുക;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        എങ്കിൽ (not *i_fmt) {
            നിർത്തുക;
        }
        ch = *i_fmt++;
        തിരഞ്ഞെടുപ്പ് (ch) {
        അവസ്ഥ 'c': {
            m3ApiCheckMem(i_args, വലുപ്പം(wasm_ptr_t));
            അക്ഷരം char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            നിർത്തുക;
        }
        അവസ്ഥ 'd':
        അവസ്ഥ 'x': {
            m3ApiCheckMem(i_args, വലുപ്പം(wasm_ptr_t));
            പൂർണ്ണസംഖ്യ  int_temp   = *i_args++;
            അക്ഷരം buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, വലുപ്പം(buffer));
            നിർത്തുക;
        }
        അവസ്ഥ 's': {
            m3ApiCheckMem(i_args, വലുപ്പം(wasm_ptr_t));
            സ്ഥിരം അക്ഷരം* string_temp;
            size_t      string_len;

            string_temp = (സ്ഥിരം അക്ഷരം*)m3ApiOffsetToPtr(*i_args++);
            എങ്കിൽ (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } അല്ലെങ്കിൽ {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി:
            fputc(ch, file);
            length++;
            നിർത്തുക;
        }
        }
    }

    m3ApiReturn(length);
}

സ്ഥിതം
uint64_t clock_ms (IM3Runtime i_runtime)
{
#എങ്കിൽ d_m3DeterministicProfile
    തിരികെ i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_runtime;

#  ifdef CLOCKS_PER_SEC
    സ്ഥിരം clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    തിരികെ (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  അല്ലെങ്കിൽ
    തിരികെ (uint64_t)clock();
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

സ്ഥിതം
M3Result SuppressLookupFailure (M3Result i_result)
{
    എങ്കിൽ (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
സ്ഥിതം
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    എങ്കിൽ (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    തിരികെ SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    സ്ഥിരം അക്ഷരം* env = "env";
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
    തിരികെ result;
}
