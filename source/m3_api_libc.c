//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#অন্তর্ভাব "m3_api_libc.h"

#অন্তর্ভাব "m3_env.h"
#অন্তর্ভাব "m3_deterministic.h"
#অন্তর্ভাব "m3_exception.h"

#অন্তর্ভাব <time.h>
#অন্তর্ভাব <errno.h>
#অন্তর্ভাব <stdio.h>

প্রকারনির্দেশ uint32_t wasm_ptr_t;
প্রকারনির্দেশ uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (শূন্য)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(শূন্য*, i_ptr)
    m3ApiGetArg(int32_t, i_মান)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_মান, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(শূন্য*, o_dst)
    m3ApiGetArgMem(শূন্য*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(শূন্য*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

স্থির
শূন্য internal_itoa (পূর্ণাঙ্ক n, বর্ণ s[], পূর্ণাঙ্ক radix)
{
    স্থির বর্ণ ধ্রুব HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    পূর্ণাঙ্ক  i, j, sign;
    বর্ণ c;

    যদি ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    কুরু {
        s[i++] = HEXDIGITS[n % radix];
    } যাবত্ ((n /= radix) > 0);

    যদি (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    প্রতি (i = 0, j = (পূর্ণাঙ্ক)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ধ্রুব বর্ণ*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    যদি (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    বর্ণ ch;
    যাবত্ ((ch = *i_fmt++)) {
        যদি ('%' != ch) {
            putc(ch, file);
            length++;
            অনুবর্ত;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        যদি (not *i_fmt) {
            ভঙ্গ;
        }
        ch = *i_fmt++;
        বিকল্প (ch) {
        পক্ষ 'c': {
            m3ApiCheckMem(i_args, পরিমাণ(wasm_ptr_t));
            বর্ণ char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            ভঙ্গ;
        }
        পক্ষ 'd':
        পক্ষ 'x': {
            m3ApiCheckMem(i_args, পরিমাণ(wasm_ptr_t));
            পূর্ণাঙ্ক  int_temp   = *i_args++;
            বর্ণ buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, পরিমাণ(buffer));
            ভঙ্গ;
        }
        পক্ষ 's': {
            m3ApiCheckMem(i_args, পরিমাণ(wasm_ptr_t));
            ধ্রুব বর্ণ* string_temp;
            size_t      string_len;

            string_temp = (ধ্রুব বর্ণ*)m3ApiOffsetToPtr(*i_args++);
            যদি (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } অন্যথা {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            ভঙ্গ;
        স্বভাব:
            fputc(ch, file);
            length++;
            ভঙ্গ;
        }
        }
    }

    m3ApiReturn(length);
}

স্থির
uint64_t clock_ms (IM3Runtime i_runtime)
{
#যদি d_m3DeterministicProfile
    প্রত্যাগম i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#অন্যথা
    (শূন্য)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ধ্রুব clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    প্রত্যাগম (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  অন্যথা
    প্রত্যাগম (uint64_t)clock();
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

স্থির
M3Result SuppressLookupFailure (M3Result i_result)
{
    যদি (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        প্রত্যাগম m3Err_none;
    } অন্যথা {
        প্রত্যাগম i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
স্থির
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    যদি (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    প্রত্যাগম SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    ধ্রুব বর্ণ* env = "env";
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
    প্রত্যাগম result;
}
