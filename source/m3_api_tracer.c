//
//  m3_api_tracer.c
//
//  Created by Volodymyr Shymanskyy on 02/18/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_tracer.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"

#ਜੇ defined(d_m3HasTracer)


ਸਥਿਰ FILE* trace = ਨਿਰਾ;

m3ApiRawFunction(m3_env_log_execution)
{
    m3ApiGetArg(uint32_t, id)
    fprintf(trace, "exec;%d\n", id);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_env_log_exec_enter)
{
    m3ApiGetArg(uint32_t, id)
    m3ApiGetArg(uint32_t, func)
    fprintf(trace, "enter;%d;%d\n", id, func);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_env_log_exec_exit)
{
    m3ApiGetArg(uint32_t, id)
    m3ApiGetArg(uint32_t, func)
    fprintf(trace, "exit;%d;%d\n", id, func);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_env_log_exec_loop)
{
    m3ApiGetArg(uint32_t, id)
    fprintf(trace, "loop;%d\n", id);
    m3ApiSuccess();
}

m3ApiRawFunction(m3_env_ਲੱਦੋ_ptr)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uint32_t, id)
    m3ApiGetArg(uint32_t, align)
    m3ApiGetArg(uint32_t, offset)
    m3ApiGetArg(uint32_t, ਪਤਾ)
    fprintf(trace, "load ptr;%d;%d;%d;%d\n", id, align, offset, ਪਤਾ);
    m3ApiReturn(ਪਤਾ);
}

m3ApiRawFunction(m3_env_store_ptr)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uint32_t, id)
    m3ApiGetArg(uint32_t, align)
    m3ApiGetArg(uint32_t, offset)
    m3ApiGetArg(uint32_t, ਪਤਾ)
    fprintf(trace, "store ptr;%d;%d;%d;%d\n", id, align, offset, ਪਤਾ);
    m3ApiReturn(ਪਤਾ);
}

// clang-format off

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Trace_ਸਮ੍ਰਿਤੀ(FUNC, NAME, TYPE, FMT)                \
m3ApiRawFunction(m3_env_##FUNC)                               \
{                                                             \
    m3ApiReturnType (TYPE)                                    \
    m3ApiGetArg      (uint32_t, id)                           \
    m3ApiGetArg      (TYPE,     val)                          \
    fprintf(trace, NAME ";%d;" FMT "\n", id, val);            \
    m3ApiReturn(val);                                         \
}

d_m3_Trace_ਸਮ੍ਰਿਤੀ( ਲੱਦੋ_val_i32,  "load i32", int32_t, "%" PRIi32)
d_m3_Trace_ਸਮ੍ਰਿਤੀ(store_val_i32, "store i32", int32_t, "%" PRIi32)
d_m3_Trace_ਸਮ੍ਰਿਤੀ( ਲੱਦੋ_val_i64,  "load i64", int64_t, "%" PRIi64)
d_m3_Trace_ਸਮ੍ਰਿਤੀ(store_val_i64, "store i64", int64_t, "%" PRIi64)
d_m3_Trace_ਸਮ੍ਰਿਤੀ( ਲੱਦੋ_val_f32,  "load f32", ਦਸ਼ਮਲਵ,   "%" PRIf32)
d_m3_Trace_ਸਮ੍ਰਿਤੀ(store_val_f32, "store f32", ਦਸ਼ਮਲਵ,   "%" PRIf32)
d_m3_Trace_ਸਮ੍ਰਿਤੀ( ਲੱਦੋ_val_f64,  "load f64", ਦੁੱਗਣਾ,  "%" PRIf64)
d_m3_Trace_ਸਮ੍ਰਿਤੀ(store_val_f64, "store f64", ਦੁੱਗਣਾ,  "%" PRIf64)


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3TraceLocal(FUNC, NAME, TYPE, FMT)                 \
m3ApiRawFunction(m3_env_##FUNC)                               \
{                                                             \
    m3ApiReturnType (TYPE)                                    \
    m3ApiGetArg      (uint32_t, id)                           \
    m3ApiGetArg      (uint32_t, local)                        \
    m3ApiGetArg      (TYPE,     val)                          \
    fprintf(trace, NAME ";%d;%d;" FMT "\n", id, local, val); \
    m3ApiReturn(val);                                         \
}


d_m3TraceLocal(get_i32, "get i32", int32_t, "%" PRIi32)
d_m3TraceLocal(ਸੈੱਟ_i32, "set i32", int32_t, "%" PRIi32)
d_m3TraceLocal(get_i64, "get i64", int64_t, "%" PRIi64)
d_m3TraceLocal(ਸੈੱਟ_i64, "set i64", int64_t, "%" PRIi64)
d_m3TraceLocal(get_f32, "get f32", ਦਸ਼ਮਲਵ,   "%" PRIf32)
d_m3TraceLocal(ਸੈੱਟ_f32, "set f32", ਦਸ਼ਮਲਵ,   "%" PRIf32)
d_m3TraceLocal(get_f64, "get f64", ਦੁੱਗਣਾ,  "%" PRIf64)
d_m3TraceLocal(ਸੈੱਟ_f64, "set f64", ਦੁੱਗਣਾ,  "%" PRIf64)

// clang-format on

ਸਥਿਰ
M3_ਨਤੀਜਾ SuppressLookupFailure (M3_ਨਤੀਜਾ i_ਨਤੀਜਾ)
{
    ਜੇ (i_ਨਤੀਜਾ == m3Err_none) {
        // If any trace function is found in the module, open the trace file
        ਜੇ (!trace) {
            trace = fopen("wasm3_trace.csv", "w");
        }
    } ਨਹੀਂਤਾਂ ਜੇ (i_ਨਤੀਜਾ == m3Err_functionLookupFailed) {
        i_ਨਤੀਜਾ = m3Err_none;
    }
    ਵਾਪਸ i_ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ m3_LinkTracer (IM3Module module)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਅਚਲ ਅੱਖਰ* env = "env";

    // clang-format off

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "log_execution",       "v(i)",     &m3_env_log_execution)));

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "log_exec_enter",      "v(ii)",    &m3_env_log_exec_enter)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "log_exec_exit",       "v(ii)",    &m3_env_log_exec_exit)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "log_exec_loop",       "v(i)",     &m3_env_log_exec_loop)));

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "load_ptr",            "i(iiii)",  &m3_env_ਲੱਦੋ_ptr)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "store_ptr",           "i(iiii)",  &m3_env_store_ptr)));

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "load_val_i32",        "i(ii)",    &m3_env_ਲੱਦੋ_val_i32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "load_val_i64",        "I(iI)",    &m3_env_ਲੱਦੋ_val_i64)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "load_val_f32",        "f(if)",    &m3_env_ਲੱਦੋ_val_f32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "load_val_f64",        "F(iF)",    &m3_env_ਲੱਦੋ_val_f64)));

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "store_val_i32",       "i(ii)",    &m3_env_store_val_i32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "store_val_i64",       "I(iI)",    &m3_env_store_val_i64)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "store_val_f32",       "f(if)",    &m3_env_store_val_f32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "store_val_f64",       "F(iF)",    &m3_env_store_val_f64)));

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "get_i32",             "i(iii)",   &m3_env_get_i32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "get_i64",             "I(iiI)",   &m3_env_get_i64)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "get_f32",             "f(iif)",   &m3_env_get_f32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "get_f64",             "F(iiF)",   &m3_env_get_f64)));

_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "set_i32",             "i(iii)",   &m3_env_ਸੈੱਟ_i32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "set_i64",             "I(iiI)",   &m3_env_ਸੈੱਟ_i64)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "set_f32",             "f(iif)",   &m3_env_ਸੈੱਟ_f32)));
_   (SuppressLookupFailure(m3_LinkRawFunction(module, env, "set_f64",             "F(iiF)",   &m3_env_ਸੈੱਟ_f64)));

    // clang-format on

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasTracer
