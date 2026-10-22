//
//  m3_api_libc.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#అన్తర్భావ "m3_api_libc.h"

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_deterministic.h"
#అన్తర్భావ "m3_exception.h"

#అన్తర్భావ <time.h>
#అన్తర్భావ <errno.h>
#అన్తర్భావ <stdio.h>

రకనిర్వచనం uint32_t wasm_ptr_t;
రకనిర్వచనం uint32_t wasm_కొలత_t;

m3ApiRawFunction(m3_libc_abort)
{
    m3ApiTrap(m3Err_trapAbort);
}

m3ApiRawFunction(m3_libc_exit)
{
    m3ApiGetArg(int32_t, సంకేతం_2)
    (శూన్య)సంకేతం_2;

    m3ApiTrap(m3Err_trapExit);
}


m3ApiRawFunction(m3_libc_memset)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(శూన్య*, i_ptr)
    m3ApiGetArg(int32_t, i_విలువ)
    m3ApiGetArg(wasm_కొలత_t, i_కొలత)

    m3ApiCheckMem(i_ptr, i_కొలత);

    u32 ఫలితం = m3_Api_Ptr_గమ్యం_స్థానభ్రంశం(memset(i_ptr, i_విలువ, i_కొలత));
    m3ApiReturn(ఫలితం);
}

m3ApiRawFunction(m3_libc_memmove)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(శూన్య*, o_dst)
    m3ApiGetArgMem(శూన్య*, i_src)
    m3ApiGetArg(wasm_కొలత_t, i_కొలత)

    m3ApiCheckMem(o_dst, i_కొలత);
    m3ApiCheckMem(i_src, i_కొలత);

    u32 ఫలితం = m3_Api_Ptr_గమ్యం_స్థానభ్రంశం(memmove(o_dst, i_src, i_కొలత));
    m3ApiReturn(ఫలితం);
}

m3ApiRawFunction(m3_libc_print)
{
    m3ApiReturnType(uint32_t)

    m3ApiGetArgMem(శూన్య*, i_ptr)
    m3ApiGetArg(wasm_కొలత_t, i_కొలత)

    m3ApiCheckMem(i_ptr, i_కొలత);

    fwrite(i_ptr, i_కొలత, 1, stdout);
    fflush(stdout);

    m3ApiReturn(i_కొలత);
}

స్థిర
శూన్య అంతర్గత_itoa (పూర్ణాంకం n, అక్షరం s[], పూర్ణాంకం radix)
{
    స్థిర అక్షరం స్థిరరాశి HEXDIGITS[0x10] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    పూర్ణాంకం  i, j, sign;
    అక్షరం c;

    ఒకవేళ ((sign = n) < 0) {
        n = -n;
    }
    i = 0;
    చేయి {
        s[i++] = HEXDIGITS[n % radix];
    } అయితే ((n /= radix) > 0);

    ఒకవేళ (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';

    //తిరగేయి
    కొరకు (i = 0, j = (పూర్ణాంకం)అక్షరశ్రేణి_పొడవు(s) - 1; i < j; i++, j--) {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

m3ApiRawFunction(m3_libc_printf)
{
    m3ApiReturnType(int32_t)

    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, i_fmt)
    m3ApiGetArgMem(wasm_ptr_t*, i_వాదనలు)

    ఒకవేళ (m3ApiIsNullPtr(i_fmt)) {
        m3ApiReturn(0);
    }

    m3ApiCheckMem(i_fmt, 1);
    size_t fmt_len = strnlen(i_fmt, 1024);
    m3ApiCheckMem(i_fmt, fmt_len + 1); // include `\0`

    FILE*   file   = stdout;
    int32_t నిడివి = 0;

    అక్షరం ch;
    అయితే ((ch = *i_fmt++)) {
        ఒకవేళ ('%' != ch) {
            putc(ch, file);
            నిడివి++;
            కొనసాగు;
        }
        // A trailing '%' has no conversion character: the next byte is the
        // terminating NUL. Stop here rather than consuming it, which would
        // step the cursor past the end of the guest string.
        ఒకవేళ (not *i_fmt) {
            ఆపు;
        }
        ch = *i_fmt++;
        ఎంపిక (ch) {
        స్థితి 'c': {
            m3ApiCheckMem(i_వాదనలు, పరిమాణం(wasm_ptr_t));
            అక్షరం char_తాత్కాలిక = *i_వాదనలు++;
            fputc(char_తాత్కాలిక, file);
            నిడివి++;
            ఆపు;
        }
        స్థితి 'd':
        స్థితి 'x': {
            m3ApiCheckMem(i_వాదనలు, పరిమాణం(wasm_ptr_t));
            పూర్ణాంకం  int_తాత్కాలిక   = *i_వాదనలు++;
            అక్షరం బఫరు[32] = { 0 };
            అంతర్గత_itoa(int_తాత్కాలిక, బఫరు, (ch == 'x') ? 16 : 10);
            fputs(బఫరు, file);
            నిడివి += (int32_t)strnlen(బఫరు, పరిమాణం(బఫరు));
            ఆపు;
        }
        స్థితి 's': {
            m3ApiCheckMem(i_వాదనలు, పరిమాణం(wasm_ptr_t));
            స్థిరరాశి అక్షరం* string_తాత్కాలిక;
            size_t      string_len;

            string_తాత్కాలిక = (స్థిరరాశి అక్షరం*)m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(*i_వాదనలు++);
            ఒకవేళ (m3ApiIsNullPtr(string_తాత్కాలిక)) {
                string_తాత్కాలిక = "(null)";
                string_len  = 6;
            } లేకపోతే {
                string_len = strnlen(string_తాత్కాలిక, 1024);
                m3ApiCheckMem(string_తాత్కాలిక, string_len + 1);
            }

            fwrite(string_తాత్కాలిక, 1, string_len, file);
            నిడివి += (int32_t)string_len;
            ఆపు;
        అప్రమేయ:
            fputc(ch, file);
            నిడివి++;
            ఆపు;
        }
        }
    }

    m3ApiReturn(నిడివి);
}

స్థిర
uint64_t clock_ms (IM3Runtime i_runtime)
{
#ఒకవేళ d_m3DeterministicProfile
    తిరిగి i_runtime ? Deterministic_సమయం(i_runtime) / 1000000 : 0;
#లేకపోతే
    (శూన్య)i_runtime;

#  ifdef CLOCKS_PER_SEC
    స్థిరరాశి clock_t clock_divider = CLOCKS_PER_SEC / 1000;
    తిరిగి (uint64_t)(clock() / (clock_divider > 0 ? clock_divider : 1));
#  లేకపోతే
    తిరిగి (uint64_t)clock();
#  endif
#అయితే_ముగింపు
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

స్థిర
M3_ఫలితం SuppressLookupFailure (M3_ఫలితం i_ఫలితం)
{
    ఒకవేళ (i_ఫలితం == m3Err_functionLookupFailed or i_ఫలితం == m3Err_globalLookupFailed) {
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి i_ఫలితం;
    }
}

// wasm-coremark's minimal build imports `u64 env.clock_ms()`, while older ones
// - source/extra/coremark_minimal.wasm.h among them - import the i32 form.
// Bind whichever of the two the module actually declares.
స్థిర
M3_ఫలితం LinkClockMs (IM3Module module, ccstr_t i_moduleName)
{
    M3_ఫలితం ఫలితం = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "I()", &m3_libc_clock_ms_i64);

    ఒకవేళ (ఫలితం == m3Err_functionSignatureMismatch) {
        ఫలితం = m3_LinkRawFunction(module, i_moduleName, "clock_ms", "i()", &m3_libc_clock_ms);
    }

    తిరిగి SuppressLookupFailure(ఫలితం);
}

M3_ఫలితం m3_LinkLibC (IM3Module module)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    స్థిరరాశి అక్షరం* env = "env";
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
    తిరిగి ఫలితం;
}
