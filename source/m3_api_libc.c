//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#समावेश "m3_api_libc.h"

#समावेश "m3_env.h"
#समावेश "m3_deterministic.h"
#समावेश "m3_exception.h"

#समावेश <time.h>
#समावेश <errno.h>
#समावेश <stdio.h>

प्रकार uint32_t wasm_ptr_t;
प्रकार uint32_t wasm_आकार_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, code)
    (व्योम)code;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(व्योम*, i_ptr)
    m3ApiGetArg(int32_t, i_मूल्य)
    m3ApiGetArg(wasm_आकार_t, i_आकार)

    m3ApiCheckMem(i_ptr, i_आकार);

    u32 परिणाम = m3ApiPtrToOffset(स_रखो(i_ptr, i_मूल्य, i_आकार));
    m3ApiReturn(परिणाम);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(व्योम*, o_dst)
    m3ApiGetArgMem(व्योम*, i_src)
    m3ApiGetArg(wasm_आकार_t, i_आकार)

    m3ApiCheckMem(o_dst, i_आकार);
    m3ApiCheckMem(i_src, i_आकार);

    u32 परिणाम = m3ApiPtrToOffset(स_हटाओ(o_dst, i_src, i_आकार));
    m3ApiReturn(परिणाम);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(व्योम*, i_ptr)
    m3ApiGetArg(wasm_आकार_t, i_आकार)

    m3ApiCheckMem(i_ptr, i_आकार);

    ख_डालो(i_ptr, i_आकार, 1, मानक_निकास);
    ख_साफ(मानक_निकास);

    m3ApiReturn(i_आकार);
}

अटल
व्योम internal_itoa (पूर्णांक n, अक्षर s[], पूर्णांक radix)
{
    अटल अक्षर स्थिर HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    पूर्णांक  i, j, sign;
    अक्षर c;

    अगर ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    करो {
        s[i++] = HEXDIGITS[n % radix];
    } जबतक ((n /= radix) > 0);

    अगर (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    // reverse
    क्रम (i = 0, j = (पूर्णांक)म_माप(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(स्थिर अक्षर*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_args)

    अगर (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    माप_प्रकार fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    खाता*   file   = मानक_निकास;
    int32_t लंबाई = 0;

    अक्षर ch;
    जबतक ((ch = *i_fmt++)) {
        अगर ('%' != ch) {
            अ_दो(ch, file);
            लंबाई++;
            जारी;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        अगर (not *i_fmt) {
            अवरोध;
        }
        ch = *i_fmt++;
        चयन (ch) {
        हाल 'c': {
            m3ApiCheckMem(i_args, माप(wasm_ptr_t));
            अक्षर char_अस्थायी = *i_args++;
            ख_अक्षर_दो(char_अस्थायी, file);
            लंबाई++;
            अवरोध;
        }
        हाल 'd':
        हाल 'x': {
            m3ApiCheckMem(i_args, माप(wasm_ptr_t));
            पूर्णांक  int_अस्थायी   = *i_args++;
            अक्षर बफर[32] = { 0 };
            internal_itoa(int_अस्थायी, बफर, (ch == 'x') ? 16 : 10);
            ख_माला_दो(बफर, file);
            लंबाई += (int32_t)strnlen(बफर, माप(बफर));
            अवरोध;
        }
        हाल 's': {
            m3ApiCheckMem(i_args, माप(wasm_ptr_t));
            स्थिर अक्षर* string_अस्थायी;
            माप_प्रकार      string_len;

            string_अस्थायी = (स्थिर अक्षर*)m3ApiOffsetToPtr(*i_args++);
            अगर (m3ApiIsNullPtr(string_अस्थायी)) {
                string_अस्थायी = "(null)";
                string_len  = 6;
            } अन्यथा {
                string_len = strnlen(string_अस्थायी, 1024);
                m3ApiCheckMem(string_अस्थायी, string_len + 1);
            }

            ख_डालो(string_अस्थायी, 1, string_len, file);
            लंबाई += (int32_t)string_len;
            अवरोध;
        शेष:
            ख_अक्षर_दो(ch, file);
            लंबाई++;
            अवरोध;
        }
        }
    }

    m3ApiReturn(लंबाई);
}

अटल
uint64_t clock_ms (IM3Runtime i_runtime)
{
#अगर d_m3DeterministicProfile
    वापस i_runtime ? Deterministic_Time(i_runtime) / 1000000 : 0;
#अन्यथा
    (व्योम)i_runtime;

#  ifdef CLOCKS_PER_SEC
    स्थिर घड़ी_प्रकार clock_divider = CLOCKS_PER_SEC / 1000;
    वापस (uint64_t)(घड़ी() / (clock_divider > 0 ? clock_divider : 1));
#  अन्यथा
    वापस (uint64_t)घड़ी();
#  endif
#पूर्ण_अगर
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

अटल
M3_परिणाम SuppressLookupFailure (M3_परिणाम i_परिणाम)
{
    अगर (i_परिणाम == m3Err_functionLookupFailed or i_परिणाम == m3Err_globalLookupFailed) {
        वापस m3Err_none;
    } अन्यथा {
        वापस i_परिणाम;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
अटल
M3_परिणाम LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3_परिणाम परिणाम = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    अगर (परिणाम == m3Err_functionSignatureMismatch) {
        परिणाम = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    वापस SuppressLookupFailure(परिणाम);
}

M3_परिणाम m3_LinkLibC (IM3Module module)
{
    M3_परिणाम परिणाम = m3Err_none;

    स्थिर अक्षर* env = "env";
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
    वापस परिणाम;
}
