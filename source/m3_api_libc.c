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

قسمتعریف uint32_t wasm_ptr_t;
قسمتعریف uint32_t wasm_سائز_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, رمز)
    (خالی)رمز;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(خالی*, i_ptr)
    m3ApiGetArg(int32_t, i_قدر)
    m3ApiGetArg(wasm_سائز_t, i_سائز)

    m3ApiCheckMem(i_ptr, i_سائز);

    u32 نتیجہ = m3_Api_Ptr_مقصد_انحراف(memset(i_ptr, i_قدر, i_سائز));
    m3ApiReturn(نتیجہ);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(خالی*, o_dst)
    m3ApiGetArgMem(خالی*, i_src)
    m3ApiGetArg(wasm_سائز_t, i_سائز)

    m3ApiCheckMem(o_dst, i_سائز);
    m3ApiCheckMem(i_src, i_سائز);

    u32 نتیجہ = m3_Api_Ptr_مقصد_انحراف(memmove(o_dst, i_src, i_سائز));
    m3ApiReturn(نتیجہ);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(خالی*, i_ptr)
    m3ApiGetArg(wasm_سائز_t, i_سائز)

    m3ApiCheckMem(i_ptr, i_سائز);

    fwrite(i_ptr, i_سائز, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_سائز);
}

ساکن
خالی اندرونی_itoa (صحیح n, حرف s[], صحیح radix)
{
    ساکن حرف ثابت HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    صحیح  i, j, sign;
    حرف c;

    اگر ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    کرو {
        s[i++] = HEXDIGITS[n % radix];
    } جب ((n /= radix) > 0);

    اگر (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    //الٹ
    برائے (i = 0, j = (صحیح)لڑی_لمبائی(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ثابت حرف*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_دلائل)

    اگر (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t لمبائی = 0;

    حرف ch;
    جب ((ch = *i_fmt++)) {
        اگر ('%' != ch) {
            putc(ch, file);
            لمبائی++;
            جاری;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        اگر (not *i_fmt) {
            توڑو;
        }
        ch = *i_fmt++;
        انتخاب (ch) {
        حالت 'c': {
            m3ApiCheckMem(i_دلائل, حجم(wasm_ptr_t));
            حرف char_عارضی = *i_دلائل++;
            fputc(char_عارضی, file);
            لمبائی++;
            توڑو;
        }
        حالت 'd':
        حالت 'x': {
            m3ApiCheckMem(i_دلائل, حجم(wasm_ptr_t));
            صحیح  int_عارضی   = *i_دلائل++;
            حرف بفر[32] = { 0 };
            اندرونی_itoa(int_عارضی, بفر, (ch == 'x') ? 16 : 10);
            fputs(بفر, file);
            لمبائی += (int32_t)strnlen(بفر, حجم(بفر));
            توڑو;
        }
        حالت 's': {
            m3ApiCheckMem(i_دلائل, حجم(wasm_ptr_t));
            ثابت حرف* string_عارضی;
            size_t      string_len;

            string_عارضی = (ثابت حرف*)m3_Api_انحراف_مقصد_Ptr(*i_دلائل++);
            اگر (m3ApiIsNullPtr(string_عارضی)) {
                string_عارضی = "(null)";
                string_len  = 6;
            } ورنہ {
                string_len = strnlen(string_عارضی, 1024);
                m3ApiCheckMem(string_عارضی, string_len + 1);
            }

            fwrite(string_عارضی, 1, string_len, file);
            لمبائی += (int32_t)string_len;
            توڑو;
        طےشدہ:
            fputc(ch, file);
            لمبائی++;
            توڑو;
        }
        }
    }

    m3ApiReturn(لمبائی);
}

ساکن
uint64_t clock_ms (IM3Runtime i_runtime)
{
#اگر d_m3DeterministicProfile
    واپس i_runtime ? Deterministic_وقت(i_runtime) / 1000000 : 0;
#ورنہ
    (خالی)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ثابت clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    واپس (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  ورنہ
    واپس (uint64_t)clock();
#  endif
#اگر_ختم
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

ساکن
M3_نتیجہ SuppressLookupFailure (M3_نتیجہ i_نتیجہ)
{
    اگر (i_نتیجہ == m3Err_functionLookupFailed or i_نتیجہ == m3Err_globalLookupFailed) {
        واپس m3Err_none;
    } ورنہ {
        واپس i_نتیجہ;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
ساکن
M3_نتیجہ LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3_نتیجہ نتیجہ = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    اگر (نتیجہ == m3Err_functionSignatureMismatch) {
        نتیجہ = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    واپس SuppressLookupFailure(نتیجہ);
}

M3_نتیجہ m3_LinkLibC (IM3Module module)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    ثابت حرف* env = "env";
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
    واپس نتیجہ;
}
