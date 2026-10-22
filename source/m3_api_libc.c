//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ضمن "m3_api_libc.h"

#ضمن "m3_env.h"
#ضمن "m3_deterministic.h"
#ضمن "m3_exception.h"

#ضمن <time.h>
#ضمن <errno.h>
#ضمن <stdio.h>

تعريفنوع uint32_t wasm_ptr_t;
تعريفنوع uint32_t wasm_اندازه_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, کوډ)
    (فراغ)کوډ;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(فراغ*, i_ptr)
    m3ApiGetArg(int32_t, i_ارزښت)
    m3ApiGetArg(wasm_اندازه_t, i_اندازه)

    m3ApiCheckMem(i_ptr, i_اندازه);

    u32 پایله = m3_Api_Ptr_منزل_دننه_واټن(memset(i_ptr, i_ارزښت, i_اندازه));
    m3ApiReturn(پایله);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(فراغ*, o_dst)
    m3ApiGetArgMem(فراغ*, i_src)
    m3ApiGetArg(wasm_اندازه_t, i_اندازه)

    m3ApiCheckMem(o_dst, i_اندازه);
    m3ApiCheckMem(i_src, i_اندازه);

    u32 پایله = m3_Api_Ptr_منزل_دننه_واټن(memmove(o_dst, i_src, i_اندازه));
    m3ApiReturn(پایله);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(فراغ*, i_ptr)
    m3ApiGetArg(wasm_اندازه_t, i_اندازه)

    m3ApiCheckMem(i_ptr, i_اندازه);

    fwrite(i_ptr, i_اندازه, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_اندازه);
}

ساكن
فراغ دننی_itoa (صحيح n, حرف s[], صحيح radix)
{
    ساكن حرف ثابت HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    صحيح  i, j, sign;
    حرف c;

    إذا ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    افعل {
        s[i++] = HEXDIGITS[n % radix];
    } طالما ((n /= radix) > 0);

    إذا (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    //سرچپه
    لكل (i = 0, j = (صحيح)مزي_اوږدوالی(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ثابت حرف*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_دلیلونه)

    إذا (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t اوږدتیا = 0;

    حرف ch;
    طالما ((ch = *i_fmt++)) {
        إذا ('%' != ch) {
            putc(ch, file);
            اوږدتیا++;
            استمر;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        إذا (not *i_fmt) {
            اكسر;
        }
        ch = *i_fmt++;
        بدل (ch) {
        حالة 'c': {
            m3ApiCheckMem(i_دلیلونه, حجم(wasm_ptr_t));
            حرف char_لنډمهاله = *i_دلیلونه++;
            fputc(char_لنډمهاله, file);
            اوږدتیا++;
            اكسر;
        }
        حالة 'd':
        حالة 'x': {
            m3ApiCheckMem(i_دلیلونه, حجم(wasm_ptr_t));
            صحيح  int_لنډمهاله   = *i_دلیلونه++;
            حرف بفر[32] = { 0 };
            دننی_itoa(int_لنډمهاله, بفر, (ch == 'x') ? 16 : 10);
            fputs(بفر, file);
            اوږدتیا += (int32_t)strnlen(بفر, حجم(بفر));
            اكسر;
        }
        حالة 's': {
            m3ApiCheckMem(i_دلیلونه, حجم(wasm_ptr_t));
            ثابت حرف* string_لنډمهاله;
            size_t      string_len;

            string_لنډمهاله = (ثابت حرف*)m3_Api_دننه_واټن_منزل_Ptr(*i_دلیلونه++);
            إذا (m3ApiIsNullPtr(string_لنډمهاله)) {
                string_لنډمهاله = "(null)";
                string_len  = 6;
            } وإلا {
                string_len = strnlen(string_لنډمهاله, 1024);
                m3ApiCheckMem(string_لنډمهاله, string_len + 1);
            }

            fwrite(string_لنډمهاله, 1, string_len, file);
            اوږدتیا += (int32_t)string_len;
            اكسر;
        افتراضي:
            fputc(ch, file);
            اوږدتیا++;
            اكسر;
        }
        }
    }

    m3ApiReturn(اوږدتیا);
}

ساكن
uint64_t clock_ms (IM3Runtime i_runtime)
{
#إذا d_m3DeterministicProfile
    أرجع i_runtime ? Deterministic_وخت(i_runtime) / 1000000 : 0;
#وإلا
    (فراغ)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ثابت clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    أرجع (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  وإلا
    أرجع (uint64_t)clock();
#  endif
#د_که_پای
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

ساكن
M3_پایله SuppressLookupFailure (M3_پایله i_پایله)
{
    إذا (i_پایله == m3Err_functionLookupFailed or i_پایله == m3Err_globalLookupFailed) {
        أرجع m3Err_none;
    } وإلا {
        أرجع i_پایله;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
ساكن
M3_پایله LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3_پایله پایله = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    إذا (پایله == m3Err_functionSignatureMismatch) {
        پایله = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    أرجع SuppressLookupFailure(پایله);
}

M3_پایله m3_LinkLibC (IM3Module module)
{
    M3_پایله پایله = m3Err_none;

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
    أرجع پایله;
}
