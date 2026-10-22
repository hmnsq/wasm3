//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#அந்தர்பாவ "m3_api_libc.h"

#அந்தர்பாவ "m3_env.h"
#அந்தர்பாவ "m3_deterministic.h"
#அந்தர்பாவ "m3_exception.h"

#அந்தர்பாவ <time.h>
#அந்தர்பாவ <errno.h>
#அந்தர்பாவ <stdio.h>

வகைவரை uint32_t wasm_ptr_t;
வகைவரை uint32_t wasm_size_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (வெறுமை)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(வெறுமை*, i_ptr)
    m3ApiGetArg(int32_t, i_மதிப்பு)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    u32 result = m3ApiPtrToOffset(memset(i_ptr, i_மதிப்பு, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(வெறுமை*, o_dst)
    m3ApiGetArgMem(வெறுமை*, i_src)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(o_dst, i_size);
    m3ApiCheckMem(i_src, i_size);

    u32 result = m3ApiPtrToOffset(memmove(o_dst, i_src, i_size));
    m3ApiReturn(result);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(வெறுமை*, i_ptr)
    m3ApiGetArg(wasm_size_t, i_size)

    m3ApiCheckMem(i_ptr, i_size);

    fwrite(i_ptr, i_size, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_size);
}

நிலையான
வெறுமை internal_itoa (முழு n, எழுத்து s[], முழு radix)
{
    நிலையான எழுத்து மாறிலி HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    முழு  i, j, sign;
    எழுத்து c;

    எனில் ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    செய் {
        s[i++] = HEXDIGITS[n % radix];
    } போது ((n /= radix) > 0);

    எனில் (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    க்கு (i = 0, j = (முழு)strlen(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(மாறிலி எழுத்து*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    எனில் (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t length = 0;

    எழுத்து ch;
    போது ((ch = *i_fmt++)) {
        எனில் ('%' != ch) {
            putc(ch, file);
            length++;
            தொடர்;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        எனில் (not *i_fmt) {
            நிறுத்து;
        }
        ch = *i_fmt++;
        தேர்வு (ch) {
        நிலை 'c': {
            m3ApiCheckMem(i_args, அளவு(wasm_ptr_t));
            எழுத்து char_temp = *i_args++;
            fputc(char_temp, file);
            length++;
            நிறுத்து;
        }
        நிலை 'd':
        நிலை 'x': {
            m3ApiCheckMem(i_args, அளவு(wasm_ptr_t));
            முழு  int_temp   = *i_args++;
            எழுத்து buffer[32] = { 0 };
            internal_itoa(int_temp, buffer, (ch == 'x') ? 16 : 10);
            fputs(buffer, file);
            length += (int32_t)strnlen(buffer, அளவு(buffer));
            நிறுத்து;
        }
        நிலை 's': {
            m3ApiCheckMem(i_args, அளவு(wasm_ptr_t));
            மாறிலி எழுத்து* string_temp;
            size_t      string_len;

            string_temp = (மாறிலி எழுத்து*)m3ApiOffsetToPtr(*i_args++);
            எனில் (m3ApiIsNullPtr(string_temp)) {
                string_temp = "(null)";
                string_len  = 6;
            } இல்லையெனில் {
                string_len = strnlen(string_temp, 1024);
                m3ApiCheckMem(string_temp, string_len + 1);
            }

            fwrite(string_temp, 1, string_len, file);
            length += (int32_t)string_len;
            நிறுத்து;
        இயல்பு:
            fputc(ch, file);
            length++;
            நிறுத்து;
        }
        }
    }

    m3ApiReturn(length);
}

நிலையான
uint64_t clock_ms (IM3Runtime i_runtime)
{
#எனில் d_m3DeterministicProfile
    திரும்பு i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#இல்லையெனில்
    (வெறுமை)i_runtime;

#  ifdef CLOCKS_PER_SEC
    மாறிலி clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    திரும்பு (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  இல்லையெனில்
    திரும்பு (uint64_t)clock();
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

நிலையான
M3Result SuppressLookupFailure (M3Result i_result)
{
    எனில் (i_result == m3Err_functionLookupFailed or i_result == m3Err_globalLookupFailed) {
        திரும்பு m3Err_none;
    } இல்லையெனில் {
        திரும்பு i_result;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
நிலையான
M3Result LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3Result result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    எனில் (result == m3Err_functionSignatureMismatch) {
        result = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    திரும்பு SuppressLookupFailure(result);
}

M3Result m3_LinkLibC (IM3Module module)
{
    M3Result result = m3Err_none;

    மாறிலி எழுத்து* env = "env";
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
    திரும்பு result;
}
