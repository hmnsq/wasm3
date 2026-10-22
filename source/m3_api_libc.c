//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#شامل "m3_api_libc.h"

#شامل "m3_env.h"
#شامل "m3_deterministic.h"
#شامل "m3_exception.h"

#شامل <time.h>
#شامل <errno.h>
#شامل <stdio.h>

تعریفنوع uint32_t wasm_ptr_t;
تعریفنوع uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (تهی)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(تهی*, i_ptr)
    m3ApiGetArg(int32_t, i_مقدار)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_مقدار, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(تهی*, o_dst)
    m3ApiGetArgMem(تهی*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(تهی*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

ایستا
تهی internal_itoa (صحیح n, نویسه s[], صحیح radix)
{
    ایستا نویسه ثابت HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    صحیح  i, j, sign;
    نویسه c;

    اگر ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    انجامبده {
        s[i++] = HEXDIGITS[n % radix];
    } وقتی ((n /= radix) > 0);

    اگر (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    برای (i = 0, j = (صحیح)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ثابت نویسه*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    اگر (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    نویسه ch;
    وقتی ((ch = *i_fmt++)) {
        اگر ('%' != ch) {
            putc(ch, file);
            length++;
            ادامه;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        اگر (not *i_fmt) {
            بشکن;
        }
        ch = *i_fmt++;
        انتخاب (ch) {
        حالت 'c': {
            m3ApiCheckMem(i_args, اندازه(wasm_ptr_t));
            نویسه char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            بشکن;
        }
        حالت 'd':
        حالت 'x': {
            m3ApiCheckMem(i_args, اندازه(wasm_ptr_t));
            صحیح  int_temp   = *i_args++;
            نویسه buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, اندازه(buffer));
            بشکن;
        }
        حالت 's': {
            m3ApiCheckMem(i_args, اندازه(wasm_ptr_t));
            ثابت نویسه* string_temp;
            size_t      string_len;

            string_temp = (ثابت نویسه*)m3ApiOffsetToPtr(*i_args++);
            اگر (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } وگرنه {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            بشکن;
        پیشفرض:
            fputc(ch, file);
            length++;
            بشکن;
        }
        }
    }

    m3ApiReturn(length);
}

ایستا
uint64_t clock_ms (IM3Runtime i_runtime)
{
#اگر d_m3DeterministicProfile
    بازگشت i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#وگرنه
    (تهی)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ثابت clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    بازگشت (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  وگرنه
    بازگشت (uint64_t)clock();
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

ایستا
M3Result SuppressLookupFailure (M3Result i_result)
{
    اگر (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        بازگشت m3Err_none;
    } وگرنه {
        بازگشت i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
ایستا
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    اگر (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    بازگشت SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    ثابت نویسه* env = "env";
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
    بازگشت result;
}
