//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#अन्तर्भाव "m3_api_libc.h"

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_deterministic.h"
#अन्तर्भाव "m3_exception.h"

#अन्तर्भाव <time.h>
#अन्तर्भाव <errno.h>
#अन्तर्भाव <stdio.h>

प्रकारनिर्देश uint32_t wasm_ptr_t;
प्रकारनिर्देश uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (शून्य)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(शून्य*, i_ptr)
    m3ApiGetArg(int32_t, i_मूल्य)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_मूल्य, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(शून्य*, o_dst)
    m3ApiGetArgMem(शून्य*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(शून्य*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

स्थिर
शून्य internal_itoa (पूर्णाङ्क n, वर्ण s[], पूर्णाङ्क radix)
{
    स्थिर वर्ण ध्रुव HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    पूर्णाङ्क  i, j, sign;
    वर्ण c;

    यदि ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    कुरु {
        s[i++] = HEXDIGITS[n % radix];
    } यावत् ((n /= radix) > 0);

    यदि (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    प्रति (i = 0, j = (पूर्णाङ्क)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ध्रुव वर्ण*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    यदि (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    वर्ण ch;
    यावत् ((ch = *i_fmt++)) {
        यदि ('%' != ch) {
            putc(ch, file);
            length++;
            अनुवर्त;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        यदि (not *i_fmt) {
            भङ्ग;
        }
        ch = *i_fmt++;
        विकल्प (ch) {
        पक्ष 'c': {
            m3ApiCheckMem(i_args, परिमाण(wasm_ptr_t));
            वर्ण char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            भङ्ग;
        }
        पक्ष 'd':
        पक्ष 'x': {
            m3ApiCheckMem(i_args, परिमाण(wasm_ptr_t));
            पूर्णाङ्क  int_temp   = *i_args++;
            वर्ण buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, परिमाण(buffer));
            भङ्ग;
        }
        पक्ष 's': {
            m3ApiCheckMem(i_args, परिमाण(wasm_ptr_t));
            ध्रुव वर्ण* string_temp;
            size_t      string_len;

            string_temp = (ध्रुव वर्ण*)m3ApiOffsetToPtr(*i_args++);
            यदि (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } अन्यथा {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            भङ्ग;
        स्वभाव:
            fputc(ch, file);
            length++;
            भङ्ग;
        }
        }
    }

    m3ApiReturn(length);
}

स्थिर
uint64_t clock_ms (IM3Runtime i_runtime)
{
#यदि d_m3DeterministicProfile
    प्रत्यागम i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#अन्यथा
    (शून्य)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ध्रुव clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    प्रत्यागम (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  अन्यथा
    प्रत्यागम (uint64_t)clock();
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

स्थिर
M3Result SuppressLookupFailure (M3Result i_result)
{
    यदि (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
स्थिर
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    यदि (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    प्रत्यागम SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    ध्रुव वर्ण* env = "env";
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
    प्रत्यागम result;
}
