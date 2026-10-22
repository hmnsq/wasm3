//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_libc.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_deterministic.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <time.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <errno.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdio.h>

ਕਿਸਮਪਰਿਭਾਸ਼ਾ uint32_t wasm_ptr_t;
ਕਿਸਮਪਰਿਭਾਸ਼ਾ uint32_t wasm_ਮਾਪ_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, ਕੋਡ)
    (ਖਾਲੀ)ਕੋਡ;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ਖਾਲੀ*, i_ptr)
    m3ApiGetArg(int32_t, i_ਮੁੱਲ)
    m3ApiGetArg(wasm_ਮਾਪ_t, i_ਮਾਪ)

    m3ApiCheckMem(i_ptr, i_ਮਾਪ);

    u32 ਨਤੀਜਾ = m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(memset(i_ptr, i_ਮੁੱਲ, i_ਮਾਪ));
    m3ApiReturn(ਨਤੀਜਾ);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ਖਾਲੀ*, o_dst)
    m3ApiGetArgMem(ਖਾਲੀ*, i_src)
    m3ApiGetArg(wasm_ਮਾਪ_t, i_ਮਾਪ)

    m3ApiCheckMem(o_dst, i_ਮਾਪ);
    m3ApiCheckMem(i_src, i_ਮਾਪ);

    u32 ਨਤੀਜਾ = m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(memmove(o_dst, i_src, i_ਮਾਪ));
    m3ApiReturn(ਨਤੀਜਾ);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(ਖਾਲੀ*, i_ptr)
    m3ApiGetArg(wasm_ਮਾਪ_t, i_ਮਾਪ)

    m3ApiCheckMem(i_ptr, i_ਮਾਪ);

    fwrite(i_ptr, i_ਮਾਪ, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_ਮਾਪ);
}

ਸਥਿਰ
ਖਾਲੀ ਅੰਦਰੂਨੀ_itoa (ਪੂਰਨ n, ਅੱਖਰ s[], ਪੂਰਨ radix)
{
    ਸਥਿਰ ਅੱਖਰ ਅਚਲ HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    ਪੂਰਨ  i, j, sign;
    ਅੱਖਰ c;

    ਜੇ ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    ਕਰੋ {
        s[i++] = HEXDIGITS[n % radix];
    } ਜਦੋਂ ((n /= radix) > 0);

    ਜੇ (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    //ਉਲਟ
    ਲਈ (i = 0, j = (ਪੂਰਨ)ਲੜੀ_ਲੰਬਾਈ(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_ਦਲੀਲਾਂ)

    ਜੇ (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t ਲੰਮਾਈ = 0;

    ਅੱਖਰ ch;
    ਜਦੋਂ ((ch = *i_fmt++)) {
        ਜੇ ('%' != ch) {
            putc(ch, file);
            ਲੰਮਾਈ++;
            ਜਾਰੀ;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        ਜੇ (not *i_fmt) {
            ਰੋਕੋ;
        }
        ch = *i_fmt++;
        ਚੋਣ (ch) {
        ਹਾਲਤ 'c': {
            m3ApiCheckMem(i_ਦਲੀਲਾਂ, ਆਕਾਰ(wasm_ptr_t));
            ਅੱਖਰ char_ਆਰਜ਼ੀ = *i_ਦਲੀਲਾਂ++;
            fputc(char_ਆਰਜ਼ੀ, file);
            ਲੰਮਾਈ++;
            ਰੋਕੋ;
        }
        ਹਾਲਤ 'd':
        ਹਾਲਤ 'x': {
            m3ApiCheckMem(i_ਦਲੀਲਾਂ, ਆਕਾਰ(wasm_ptr_t));
            ਪੂਰਨ  int_ਆਰਜ਼ੀ   = *i_ਦਲੀਲਾਂ++;
            ਅੱਖਰ ਬਫਰ[32] = { 0 };
            ਅੰਦਰੂਨੀ_itoa(int_ਆਰਜ਼ੀ, ਬਫਰ, (ch == 'x') ? 16 : 10);
            fputs(ਬਫਰ, file);
            ਲੰਮਾਈ += (int32_t)strnlen(ਬਫਰ, ਆਕਾਰ(ਬਫਰ));
            ਰੋਕੋ;
        }
        ਹਾਲਤ 's': {
            m3ApiCheckMem(i_ਦਲੀਲਾਂ, ਆਕਾਰ(wasm_ptr_t));
            ਅਚਲ ਅੱਖਰ* string_ਆਰਜ਼ੀ;
            size_t      string_len;

            string_ਆਰਜ਼ੀ = (ਅਚਲ ਅੱਖਰ*)m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(*i_ਦਲੀਲਾਂ++);
            ਜੇ (m3ApiIsNullPtr(string_ਆਰਜ਼ੀ)) {
                string_ਆਰਜ਼ੀ = "(null)";
                string_len  = 6;
            } ਨਹੀਂਤਾਂ {
                string_len = strnlen(string_ਆਰਜ਼ੀ, 1024);
                m3ApiCheckMem(string_ਆਰਜ਼ੀ, string_len + 1);
            }

            fwrite(string_ਆਰਜ਼ੀ, 1, string_len, file);
            ਲੰਮਾਈ += (int32_t)string_len;
            ਰੋਕੋ;
        ਮੂਲ:
            fputc(ch, file);
            ਲੰਮਾਈ++;
            ਰੋਕੋ;
        }
        }
    }

    m3ApiReturn(ਲੰਮਾਈ);
}

ਸਥਿਰ
uint64_t clock_ms (IM3Runtime i_runtime)
{
#ਜੇ d_m3DeterministicProfile
    ਵਾਪਸ i_runtime ? Deterministic_ਸਮਾਂ(i_runtime) / 1000000 : 0;
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_runtime;

#  ifdef CLOCKS_PER_SEC
    ਅਚਲ clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    ਵਾਪਸ (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  ਨਹੀਂਤਾਂ
    ਵਾਪਸ (uint64_t)clock();
#  endif
#ਜੇ_ਅੰਤ
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

ਸਥਿਰ
M3_ਨਤੀਜਾ SuppressLookupFailure (M3_ਨਤੀਜਾ i_ਨਤੀਜਾ)
{
    ਜੇ (i_ਨਤੀਜਾ == m3Err_functionLookupFailed or i_ਨਤੀਜਾ == m3Err_globalLookupFailed) {
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ i_ਨਤੀਜਾ;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
ਸਥਿਰ
M3_ਨਤੀਜਾ LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    ਜੇ (ਨਤੀਜਾ == m3Err_functionSignatureMismatch) {
        ਨਤੀਜਾ = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    ਵਾਪਸ SuppressLookupFailure(ਨਤੀਜਾ);
}

M3_ਨਤੀਜਾ m3_LinkLibC (IM3Module module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਅਚਲ ਅੱਖਰ* env = "env";
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
    ਵਾਪਸ ਨਤੀਜਾ;
}
