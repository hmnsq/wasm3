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

প্রকারলেখ uint32_t wasm_ptr_t;
প্রকারলেখ uint32_t wasm_আকার_t;

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
    m3ApiGetArg(wasm_আকার_t, i_আকার)

    m3ApiCheckMem(i_ptr, i_আকার);

    u32 ফলাফল = m3ApiPtrToOffset(memset(i_ptr, i_মান, i_আকার));
    m3ApiReturn(ফলাফল);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(শূন্য*, o_dst)
    m3ApiGetArgMem(শূন্য*, i_src)
    m3ApiGetArg(wasm_আকার_t, i_আকার)

    m3ApiCheckMem(o_dst, i_আকার);
    m3ApiCheckMem(i_src, i_আকার);

    u32 ফলাফল = m3ApiPtrToOffset(memmove(o_dst, i_src, i_আকার));
    m3ApiReturn(ফলাফল);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(শূন্য*, i_ptr)
    m3ApiGetArg(wasm_আকার_t, i_আকার)

    m3ApiCheckMem(i_ptr, i_আকার);

    fwrite(i_ptr, i_আকার, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_আকার);
}

স্থির
শূন্য internal_itoa (পূর্ণসংখ্যা n, অক্ষর s[], পূর্ণসংখ্যা radix)
{
    স্থির অক্ষর ধ্রুবক HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    পূর্ণসংখ্যা  i, j, sign;
    অক্ষর c;

    যদি ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    করো {
        s[i++] = HEXDIGITS[n % radix];
    } যখন ((n /= radix) > 0);

    যদি (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    জন্য (i = 0, j = (পূর্ণসংখ্যা)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ধ্রুবক অক্ষর*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    যদি (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t দৈর্ঘ্য = 0;

    অক্ষর ch;
    যখন ((ch = *i_fmt++)) {
        যদি ('%' != ch) {
            putc(ch, file);
            দৈর্ঘ্য++;
            চালু;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        যদি (not *i_fmt) {
            বিরতি;
        }
        ch = *i_fmt++;
        নির্বাচন (ch) {
        ক্ষেত্র 'c': {
            m3ApiCheckMem(i_args, আকার(wasm_ptr_t));
            অক্ষর char_অস্থায়ী = *i_args++;
            fputc(char_অস্থায়ী, file);
            দৈর্ঘ্য++;
            বিরতি;
        }
        ক্ষেত্র 'd':
        ক্ষেত্র 'x': {
            m3ApiCheckMem(i_args, আকার(wasm_ptr_t));
            পূর্ণসংখ্যা  int_অস্থায়ী   = *i_args++;
            অক্ষর বাফার[32] = { 0 };
            internal_itoa(int_অস্থায়ী, বাফার, (ch == 'x') ? 16 : 10);
            fputs(বাফার, file);
            দৈর্ঘ্য += (int32_t)strnlen(বাফার, আকার(বাফার));
            বিরতি;
        }
        ক্ষেত্র 's': {
            m3ApiCheckMem(i_args, আকার(wasm_ptr_t));
            ধ্রুবক অক্ষর* string_অস্থায়ী;
            size_t      string_len;

            string_অস্থায়ী = (ধ্রুবক অক্ষর*)m3ApiOffsetToPtr(*i_args++);
            যদি (m3ApiIsNullPtr(string_অস্থায়ী)) {
                string_অস্থায়ী = "(null)";
                string_len  = 6;
            } অন্যথা {
                string_len = strnlen(string_অস্থায়ী, 1024);
                m3ApiCheckMem(string_অস্থায়ী, string_len + 1);
            }

            fwrite(string_অস্থায়ী, 1, string_len, file);
            দৈর্ঘ্য += (int32_t)string_len;
            বিরতি;
        পূর্বনির্ধারিত:
            fputc(ch, file);
            দৈর্ঘ্য++;
            বিরতি;
        }
        }
    }

    m3ApiReturn(দৈর্ঘ্য);
}

স্থির
uint64_t clock_ms (IM3Runtime i_runtime)
{
#যদি d_m3DeterministicProfile
    ফেরত i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#অন্যথা
    (শূন্য)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ধ্রুবক clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    ফেরত (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  অন্যথা
    ফেরত (uint64_t)clock();
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
M3_ফলাফল SuppressLookupFailure (M3_ফলাফল i_ফলাফল)
{
    যদি (i_ফলাফল == m3Err_functionLookupFailed or i_ফলাফল == m3Err_globalLookupFailed) {
        ফেরত m3Err_none;
    } অন্যথা {
        ফেরত i_ফলাফল;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
স্থির
M3_ফলাফল LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3_ফলাফল ফলাফল = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    যদি (ফলাফল == m3Err_functionSignatureMismatch) {
        ফলাফল = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    ফেরত SuppressLookupFailure(ফলাফল);
}

M3_ফলাফল m3_LinkLibC (IM3Module module)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    ধ্রুবক অক্ষর* env = "env";
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
    ফেরত ফলাফল;
}
