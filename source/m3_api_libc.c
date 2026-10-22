//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#include "m3_api_libc.h"

#include "m3_env.h"
#include "m3_deterministic.h"
#include "m3_exception.h"

#include <time.h>
#include <errno.h>
#include <stdio.h>

typedef uint32_t wasm_ptr_t;
typedef uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (void)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(void*, i_ptr)
    m3ApiGetArg(int32_t, i_value)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_value, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(void*, o_dst)
    m3ApiGetArgMem(void*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(void*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

static
void internal_itoa (정수 n, char s[], 정수 radix)
{
    static char const HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    정수  i, j, sign;
    char c;

    만약 ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    do {
        s[i++] = HEXDIGITS[n % radix];
    } 동안 ((n /= radix) > 0);

    만약 (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    위해 (i = 0, j = (정수)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(const char*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    만약 (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    char ch;
    동안 ((ch = *i_fmt++)) {
        만약 ('%' != ch) {
            putc(ch, file);
            length++;
            continue;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        만약 (not *i_fmt) {
            break;
        }
        ch = *i_fmt++;
        switch (ch) {
        case 'c': {
            m3ApiCheckMem(i_args, sizeof(wasm_ptr_t));
            char char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            break;
        }
        case 'd':
        case 'x': {
            m3ApiCheckMem(i_args, sizeof(wasm_ptr_t));
            정수  int_temp   = *i_args++;
            char buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, sizeof(buffer));
            break;
        }
        case 's': {
            m3ApiCheckMem(i_args, sizeof(wasm_ptr_t));
            const char* string_temp;
            size_t      string_len;

            string_temp = (const char*)m3ApiOffsetToPtr(*i_args++);
            만약 (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } 아니면 {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            break;
        default:
            fputc(ch, file);
            length++;
            break;
        }
        }
    }

    m3ApiReturn(length);
}

static
uint64_t clock_ms (IM3Runtime i_runtime)
{
#만약 d_m3DeterministicProfile
    반환 i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#아니면
    (void)i_runtime;

#  ifdef CLOCKS_PER_SEC
    const clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    반환 (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  아니면
    반환 (uint64_t)clock();
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

static
M3Result SuppressLookupFailure (M3Result i_result)
{
    만약 (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        반환 m3Err_none;
    } 아니면 {
        반환 i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
static
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    만약 (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    반환 SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    const char* env = "env";
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
    반환 result;
}
