//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_api_wasi.h"

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"

#ಆದರೆ defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
ಬಾಹ್ಯ ಅಕ್ಷರ** environ;
#  endif

ಸ್ಥಿರ m3_wasi_context_t* wasi_context = ಖಾಲಿ;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ wasi_iovec_t {
    uvwasi_ಗಾತ್ರ_t buf;
    uvwasi_ಗಾತ್ರ_t buf_len;
} wasi_iovec_t;

#  ಆದರೆ d_m3EnableWasiTracing

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* wasi_errno2str (uvwasi_errno_t err)
{
    // clang-format off
    ಆಯ್ಕೆ (err) {
    ಸ್ಥಿತಿ  0: ಹಿಂತಿರುಗಿ "ESUCCESS";
    ಸ್ಥಿತಿ  1: ಹಿಂತಿರುಗಿ "E2BIG";
    ಸ್ಥಿತಿ  2: ಹಿಂತಿರುಗಿ "EACCES";
    ಸ್ಥಿತಿ  3: ಹಿಂತಿರುಗಿ "EADDRINUSE";
    ಸ್ಥಿತಿ  4: ಹಿಂತಿರುಗಿ "EADDRNOTAVAIL";
    ಸ್ಥಿತಿ  5: ಹಿಂತಿರುಗಿ "EAFNOSUPPORT";
    ಸ್ಥಿತಿ  6: ಹಿಂತಿರುಗಿ "EAGAIN";
    ಸ್ಥಿತಿ  7: ಹಿಂತಿರುಗಿ "EALREADY";
    ಸ್ಥಿತಿ  8: ಹಿಂತಿರುಗಿ "EBADF";
    ಸ್ಥಿತಿ  9: ಹಿಂತಿರುಗಿ "EBADMSG";
    ಸ್ಥಿತಿ 10: ಹಿಂತಿರುಗಿ "EBUSY";
    ಸ್ಥಿತಿ 11: ಹಿಂತಿರುಗಿ "ECANCELED";
    ಸ್ಥಿತಿ 12: ಹಿಂತಿರುಗಿ "ECHILD";
    ಸ್ಥಿತಿ 13: ಹಿಂತಿರುಗಿ "ECONNABORTED";
    ಸ್ಥಿತಿ 14: ಹಿಂತಿರುಗಿ "ECONNREFUSED";
    ಸ್ಥಿತಿ 15: ಹಿಂತಿರುಗಿ "ECONNRESET";
    ಸ್ಥಿತಿ 16: ಹಿಂತಿರುಗಿ "EDEADLK";
    ಸ್ಥಿತಿ 17: ಹಿಂತಿರುಗಿ "EDESTADDRREQ";
    ಸ್ಥಿತಿ 18: ಹಿಂತಿರುಗಿ "EDOM";
    ಸ್ಥಿತಿ 19: ಹಿಂತಿರುಗಿ "EDQUOT";
    ಸ್ಥಿತಿ 20: ಹಿಂತಿರುಗಿ "EEXIST";
    ಸ್ಥಿತಿ 21: ಹಿಂತಿರುಗಿ "EFAULT";
    ಸ್ಥಿತಿ 22: ಹಿಂತಿರುಗಿ "EFBIG";
    ಸ್ಥಿತಿ 23: ಹಿಂತಿರುಗಿ "EHOSTUNREACH";
    ಸ್ಥಿತಿ 24: ಹಿಂತಿರುಗಿ "EIDRM";
    ಸ್ಥಿತಿ 25: ಹಿಂತಿರುಗಿ "EILSEQ";
    ಸ್ಥಿತಿ 26: ಹಿಂತಿರುಗಿ "EINPROGRESS";
    ಸ್ಥಿತಿ 27: ಹಿಂತಿರುಗಿ "EINTR";
    ಸ್ಥಿತಿ 28: ಹಿಂತಿರುಗಿ "EINVAL";
    ಸ್ಥಿತಿ 29: ಹಿಂತಿರುಗಿ "EIO";
    ಸ್ಥಿತಿ 30: ಹಿಂತಿರುಗಿ "EISCONN";
    ಸ್ಥಿತಿ 31: ಹಿಂತಿರುಗಿ "EISDIR";
    ಸ್ಥಿತಿ 32: ಹಿಂತಿರುಗಿ "ELOOP";
    ಸ್ಥಿತಿ 33: ಹಿಂತಿರುಗಿ "EMFILE";
    ಸ್ಥಿತಿ 34: ಹಿಂತಿರುಗಿ "EMLINK";
    ಸ್ಥಿತಿ 35: ಹಿಂತಿರುಗಿ "EMSGSIZE";
    ಸ್ಥಿತಿ 36: ಹಿಂತಿರುಗಿ "EMULTIHOP";
    ಸ್ಥಿತಿ 37: ಹಿಂತಿರುಗಿ "ENAMETOOLONG";
    ಸ್ಥಿತಿ 38: ಹಿಂತಿರುಗಿ "ENETDOWN";
    ಸ್ಥಿತಿ 39: ಹಿಂತಿರುಗಿ "ENETRESET";
    ಸ್ಥಿತಿ 40: ಹಿಂತಿರುಗಿ "ENETUNREACH";
    ಸ್ಥಿತಿ 41: ಹಿಂತಿರುಗಿ "ENFILE";
    ಸ್ಥಿತಿ 42: ಹಿಂತಿರುಗಿ "ENOBUFS";
    ಸ್ಥಿತಿ 43: ಹಿಂತಿರುಗಿ "ENODEV";
    ಸ್ಥಿತಿ 44: ಹಿಂತಿರುಗಿ "ENOENT";
    ಸ್ಥಿತಿ 45: ಹಿಂತಿರುಗಿ "ENOEXEC";
    ಸ್ಥಿತಿ 46: ಹಿಂತಿರುಗಿ "ENOLCK";
    ಸ್ಥಿತಿ 47: ಹಿಂತಿರುಗಿ "ENOLINK";
    ಸ್ಥಿತಿ 48: ಹಿಂತಿರುಗಿ "ENOMEM";
    ಸ್ಥಿತಿ 49: ಹಿಂತಿರುಗಿ "ENOMSG";
    ಸ್ಥಿತಿ 50: ಹಿಂತಿರುಗಿ "ENOPROTOOPT";
    ಸ್ಥಿತಿ 51: ಹಿಂತಿರುಗಿ "ENOSPC";
    ಸ್ಥಿತಿ 52: ಹಿಂತಿರುಗಿ "ENOSYS";
    ಸ್ಥಿತಿ 53: ಹಿಂತಿರುಗಿ "ENOTCONN";
    ಸ್ಥಿತಿ 54: ಹಿಂತಿರುಗಿ "ENOTDIR";
    ಸ್ಥಿತಿ 55: ಹಿಂತಿರುಗಿ "ENOTEMPTY";
    ಸ್ಥಿತಿ 56: ಹಿಂತಿರುಗಿ "ENOTRECOVERABLE";
    ಸ್ಥಿತಿ 57: ಹಿಂತಿರುಗಿ "ENOTSOCK";
    ಸ್ಥಿತಿ 58: ಹಿಂತಿರುಗಿ "ENOTSUP";
    ಸ್ಥಿತಿ 59: ಹಿಂತಿರುಗಿ "ENOTTY";
    ಸ್ಥಿತಿ 60: ಹಿಂತಿರುಗಿ "ENXIO";
    ಸ್ಥಿತಿ 61: ಹಿಂತಿರುಗಿ "EOVERFLOW";
    ಸ್ಥಿತಿ 62: ಹಿಂತಿರುಗಿ "EOWNERDEAD";
    ಸ್ಥಿತಿ 63: ಹಿಂತಿರುಗಿ "EPERM";
    ಸ್ಥಿತಿ 64: ಹಿಂತಿರುಗಿ "EPIPE";
    ಸ್ಥಿತಿ 65: ಹಿಂತಿರುಗಿ "EPROTO";
    ಸ್ಥಿತಿ 66: ಹಿಂತಿರುಗಿ "EPROTONOSUPPORT";
    ಸ್ಥಿತಿ 67: ಹಿಂತಿರುಗಿ "EPROTOTYPE";
    ಸ್ಥಿತಿ 68: ಹಿಂತಿರುಗಿ "ERANGE";
    ಸ್ಥಿತಿ 69: ಹಿಂತಿರುಗಿ "EROFS";
    ಸ್ಥಿತಿ 70: ಹಿಂತಿರುಗಿ "ESPIPE";
    ಸ್ಥಿತಿ 71: ಹಿಂತಿರುಗಿ "ESRCH";
    ಸ್ಥಿತಿ 72: ಹಿಂತಿರುಗಿ "ESTALE";
    ಸ್ಥಿತಿ 73: ಹಿಂತಿರುಗಿ "ETIMEDOUT";
    ಸ್ಥಿತಿ 74: ಹಿಂತಿರುಗಿ "ETXTBSY";
    ಸ್ಥಿತಿ 75: ಹಿಂತಿರುಗಿ "EXDEV";
    ಸ್ಥಿತಿ 76: ಹಿಂತಿರುಗಿ "ENOTCAPABLE";
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "<unknown>";
    }
    // clang-format on
}

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* wasi_whence2str (uvwasi_whence_t whence)
{
    ಆಯ್ಕೆ (whence) {
    ಸ್ಥಿತಿ UVWASI_WHENCE_ಗಣ: ಹಿಂತಿರುಗಿ "SET";
    ಸ್ಥಿತಿ UVWASI_WHENCE_CUR: ಹಿಂತಿರುಗಿ "CUR";
    ಸ್ಥಿತಿ UVWASI_WHENCE_ಕೊನೆ: ಹಿಂತಿರುಗಿ "END";
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ಇಲ್ಲದಿದ್ದರೆ
#    define WASI_TRACE(fmt, ...)
#  endif

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    ಪ್ರತಿಜ್ಞಾ(context != ಖಾಲಿ);
    ಹಿಂತಿರುಗಿ &context->uvwasi;
}

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_ವಾದಗಳು_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(ಅಕ್ಷರ*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಆದರೆ (context == ಖಾಲಿ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    m3ApiCheckMem(argv, context->argc * ಗಾತ್ರ(uint32_t));

    ಗಾಗಿ (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_ಗಮ್ಯ_ಸ್ಥಾನಾಂತರ(argv_buf));

        size_t len = ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        ಸ್ಮರಣೆ_ನಕಲು(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_ವಾದಗಳು_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, argc)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, argv_buf_ಗಾತ್ರ)

    m3ApiCheckMem(argc, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));
    m3ApiCheckMem(argv_buf_ಗಾತ್ರ, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಆದರೆ (context == ಖಾಲಿ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_ಗಾತ್ರ_t buf_len = 0;
    ಗಾಗಿ (u32 i = 0; i < context->argc; ++i) {
        buf_len += (uvwasi_ಗಾತ್ರ_t)ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_ಗಾತ್ರ, buf_len);

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ಅಕ್ಷರ*, env_buf)

    ಅಕ್ಷರ**         environment;
    uvwasi_errno_t ret;
    uvwasi_ಗಾತ್ರ_t  env_ಎಣಿಕೆ, env_buf_ಗಾತ್ರ;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_ಎಣಿಕೆ, &env_buf_ಗಾತ್ರ);
    ಆದರೆ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ಎಣಿಕೆ * ಗಾತ್ರ(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_ಗಾತ್ರ);

    environment = ಸೊನ್ನೆ_ಸ್ಮರಣೆ_ಕೊಡು(env_ಎಣಿಕೆ, ಗಾತ್ರ(ಅಕ್ಷರ*));
    ಆದರೆ (environment == ಖಾಲಿ) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    ಆದರೆ (ret != UVWASI_ESUCCESS) {
        ಬಿಡುಗಡೆ(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_ಸ್ಥಾನಾಂತರ = m3_Api_Ptr_ಗಮ್ಯ_ಸ್ಥಾನಾಂತರ(env_buf);

    ಗಾಗಿ (u32 i = 0; i < env_ಎಣಿಕೆ; ++i) {
        uint32_t offset = environ_buf_ಸ್ಥಾನಾಂತರ +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    ಬಿಡುಗಡೆ(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, env_ಎಣಿಕೆ)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, env_buf_ಗಾತ್ರ)

    m3ApiCheckMem(env_ಎಣಿಕೆ, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));
    m3ApiCheckMem(env_buf_ಗಾತ್ರ, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

    uvwasi_ಗಾತ್ರ_t ಎಣಿಕೆ_2;
    uvwasi_ಗಾತ್ರ_t buf_ಗಾತ್ರ;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &ಎಣಿಕೆ_2, &buf_ಗಾತ್ರ);

    m3ApiWriteMem32(env_ಎಣಿಕೆ, ಎಣಿಕೆ_2);
    m3ApiWriteMem32(env_buf_ಗಾತ್ರ, buf_ಗಾತ್ರ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(_get_uvwasi_p(_ctx), fd, ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, ಮಾರ್ಗ_len, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 8);

    uvwasi_prestat_t prestat;

    uvwasi_errno_t ret = uvwasi_fd_prestat_get(_get_uvwasi_p(_ctx), fd, &prestat);

    WASI_TRACE("fd:%d | type:%d, name_len:%d", fd, prestat.pr_type, prestat.u.dir.pr_name_len);

    ಆದರೆ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiWriteMem32(buf + 0, prestat.pr_type);
    m3ApiWriteMem32(buf + 4, prestat.u.dir.pr_name_len);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 24);

    uvwasi_fdstat_t stat;
    uvwasi_errno_t  ret = uvwasi_fd_fdstat_get(_get_uvwasi_p(_ctx), fd, &stat);

    WASI_TRACE("fd:%d", fd);

    ಆದರೆ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 24);
    m3ApiWriteMem8(buf + 0, stat.fs_filetype);
    m3ApiWriteMem16(buf + 2, stat.fs_flags);
    m3ApiWriteMem64(buf + 8, stat.fs_rights_ತಳ);
    m3ApiWriteMem64(buf + 16, stat.fs_rights_inheriting);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ಗಣ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_fdflags_t, flags)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_ಗಣ_flags(_get_uvwasi_p(_ctx), fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ಗಣ_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_rights_t, rights_ತಳ)
    m3ApiGetArg(uvwasi_rights_t, rights_inheriting)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_ಗಣ_rights(_get_uvwasi_p(_ctx), fd, rights_ತಳ, rights_inheriting);

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_ತಳ, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ಗಣ_ಗಾತ್ರ)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, ಗಾತ್ರ_2)

    uvwasi_errno_t ret = uvwasi_fd_filestat_ಗಣ_ಗಾತ್ರ(_get_uvwasi_p(_ctx), fd, ಗಾತ್ರ_2);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, ಗಾತ್ರ_2);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ಗಣ_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_timestamp_t, atim)
    m3ApiGetArg(uvwasi_timestamp_t, mtim)
    m3ApiGetArg(uvwasi_fstflags_t, fst_flags)

    uvwasi_errno_t ret = uvwasi_fd_filestat_ಗಣ_times(_get_uvwasi_p(_ctx), fd, atim, mtim, fst_flags);

    WASI_TRACE("fd:%d, atim:%" PRIu64 ", mtim:%" PRIu64 ", flags:%d", fd, atim, mtim, fst_flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_fd_filestat_get(_get_uvwasi_p(_ctx), fd, &stat);

    WASI_TRACE("fd:%d | fs.size:%" PRIu64, fd, stat.st_size);

    ಆದರೆ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.st_dev);
    m3ApiWriteMem64(buf + 8, stat.st_ino);
    m3ApiWriteMem8(buf + 16, stat.st_filetype);
    m3ApiWriteMem64(buf + 24, stat.st_nlink);
    m3ApiWriteMem64(buf + 32, stat.st_size);
    m3ApiWriteMem64(buf + 40, stat.st_atim);
    m3ApiWriteMem64(buf + 48, stat.st_mtim);
    m3ApiWriteMem64(buf + 56, stat.st_ctim);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(uvwasi_filesize_t*, ಫಲಿತಾಂಶ)

    m3ApiCheckMem(ಫಲಿತಾಂಶ, ಗಾತ್ರ(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    ಆಯ್ಕೆ (wasi_whence) {
    ಸ್ಥಿತಿ 0: whence = UVWASI_WHENCE_ಗಣ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ 1: whence = UVWASI_WHENCE_CUR; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ 2: whence = UVWASI_WHENCE_ಕೊನೆ; ನಿಲ್ಲಿಸು;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_seek(_get_uvwasi_p(_ctx), fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    m3ApiWriteMem64(ಫಲಿತಾಂಶ, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, from)
    m3ApiGetArg(uvwasi_fd_t, ಗಮ್ಯ)

    uvwasi_errno_t ret = uvwasi_fd_renumber(_get_uvwasi_p(_ctx), from, ಗಮ್ಯ);

    WASI_TRACE("from:%d, to:%d", from, ಗಮ್ಯ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_sync)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)

    uvwasi_errno_t ret = uvwasi_fd_sync(_get_uvwasi_p(_ctx), fd);

    WASI_TRACE("fd:%d", fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_tell)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(uvwasi_filesize_t*, ಫಲಿತಾಂಶ)

    m3ApiCheckMem(ಫಲಿತಾಂಶ, ಗಾತ್ರ(uvwasi_filesize_t));

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_tell(_get_uvwasi_p(_ctx), fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    m3ApiWriteMem64(ಫಲಿತಾಂಶ, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_create_directory(_get_uvwasi_p(_ctx), fd, ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    WASI_TRACE("fd:%d, path:%s", fd, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, ಮಾರ್ಗ_len)
    m3ApiGetArgMem(ಅಕ್ಷರ*, buf)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, buf_len)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, bufused)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

    uvwasi_ಗಾತ್ರ_t uvbufused;

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_readlink(_get_uvwasi_p(_ctx), fd, ಮಾರ್ಗ, ಮಾರ್ಗ_len, buf, buf_len, &uvbufused);

    WASI_TRACE("fd:%d, path:%s | buf:%s, bufused:%d", fd, ಮಾರ್ಗ, buf, uvbufused);

    m3ApiWriteMem32(bufused, uvbufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_remove_directory(_get_uvwasi_p(_ctx), fd, ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    WASI_TRACE("fd:%d, path:%s", fd, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, old_fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, old_ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, old_ಮಾರ್ಗ_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, new_ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, new_ಮಾರ್ಗ_len)

    m3ApiCheckMem(old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len);
    m3ApiCheckMem(new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len);

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_rename(_get_uvwasi_p(_ctx), old_fd, old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len,
                                            new_fd, new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len);

    WASI_TRACE("old_fd:%d, old_path:%s, new_fd:%d, new_path:%s", old_fd, old_ಮಾರ್ಗ, new_fd, new_ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, old_ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, old_ಮಾರ್ಗ_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, new_ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, new_ಮಾರ್ಗ_len)

    m3ApiCheckMem(old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len);
    m3ApiCheckMem(new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len);

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_symlink(_get_uvwasi_p(_ctx), old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len,
                                             fd, new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len);

    WASI_TRACE("old_path:%s, fd:%d, new_path:%s", old_ಮಾರ್ಗ, fd, new_ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_unlink_file(_get_uvwasi_p(_ctx), fd, ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    WASI_TRACE("fd:%d, path:%s", fd, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, dirfd)
    m3ApiGetArg(uvwasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, ಮಾರ್ಗ_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_ತಳ)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);
    m3ApiCheckMem(fd, ಗಾತ್ರ(uvwasi_fd_t));

    uvwasi_fd_t uvfd;

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_open(_get_uvwasi_p(_ctx),
                                          dirfd,
                                          dirflags,
                                          ಮಾರ್ಗ,
                                          ಮಾರ್ಗ_len,
                                          oflags,
                                          fs_rights_ತಳ,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          &uvfd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%s, oflags:0x%x, fs_flags:0x%x | fd:%d", dirfd, dirflags, ಮಾರ್ಗ, oflags, fs_flags, uvfd);

    m3ApiWriteMem32(fd, uvfd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_lookupflags_t, flags)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uint32_t, ಮಾರ್ಗ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_ಮಾರ್ಗ_filestat_get(_get_uvwasi_p(_ctx), fd, flags, ಮಾರ್ಗ, ಮಾರ್ಗ_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, ಮಾರ್ಗ, stat.st_size);

    ಆದರೆ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.st_dev);
    m3ApiWriteMem64(buf + 8, stat.st_ino);
    m3ApiWriteMem8(buf + 16, stat.st_filetype);
    m3ApiWriteMem64(buf + 24, stat.st_nlink);
    m3ApiWriteMem64(buf + 32, stat.st_size);
    m3ApiWriteMem64(buf + 40, stat.st_atim);
    m3ApiWriteMem64(buf + 48, stat.st_mtim);
    m3ApiWriteMem64(buf + 56, stat.st_ctim);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pread)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(wasi_iovec_t));
    m3ApiCheckMem(nread, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

#  ಆದರೆ M3_HAS_VLA
    ಆದರೆ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ಇಲ್ಲದಿದ್ದರೆ
    ಆದರೆ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    ಗಾಗಿ (uvwasi_ಗಾತ್ರ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ಸ್ಥಾನಾಂತರ_ಗಮ್ಯ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_ಗಾತ್ರ_t ಅಂಕೆ_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &ಅಂಕೆ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, ಅಂಕೆ_read);

    m3ApiWriteMem32(nread, ಅಂಕೆ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, iovs_len)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(wasi_iovec_t));
    m3ApiCheckMem(nread, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

#  ಆದರೆ M3_HAS_VLA
    ಆದರೆ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ಇಲ್ಲದಿದ್ದರೆ
    ಆದರೆ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_ಗಾತ್ರ_t  ಅಂಕೆ_read;
    uvwasi_errno_t ret;

    ಗಾಗಿ (uvwasi_ಗಾತ್ರ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ಸ್ಥಾನಾಂತರ_ಗಮ್ಯ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &ಅಂಕೆ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, ಅಂಕೆ_read);

    m3ApiWriteMem32(nread, ಅಂಕೆ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, iovs_len)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

#  ಆದರೆ M3_HAS_VLA
    ಆದರೆ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ಇಲ್ಲದಿದ್ದರೆ
    ಆದರೆ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_ಗಾತ್ರ_t  ಅಂಕೆ_written;
    uvwasi_errno_t ret;

    ಗಾಗಿ (uvwasi_ಗಾತ್ರ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ಸ್ಥಾನಾಂತರ_ಗಮ್ಯ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &ಅಂಕೆ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, ಅಂಕೆ_written);

    m3ApiWriteMem32(nwritten, ಅಂಕೆ_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pwrite)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

#  ಆದರೆ M3_HAS_VLA
    ಆದರೆ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ಇಲ್ಲದಿದ್ದರೆ
    ಆದರೆ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_ಗಾತ್ರ_t  ಅಂಕೆ_written;
    uvwasi_errno_t ret;

    ಗಾಗಿ (uvwasi_ಗಾತ್ರ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ಸ್ಥಾನಾಂತರ_ಗಮ್ಯ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &ಅಂಕೆ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, ಅಂಕೆ_written);

    m3ApiWriteMem32(nwritten, ಅಂಕೆ_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಶೂನ್ಯ*, buf)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

    uvwasi_ಗಾತ್ರ_t  uvbufused;
    uvwasi_errno_t ret = uvwasi_fd_readdir(_get_uvwasi_p(_ctx), fd, buf, buf_len, cookie, &uvbufused);

    WASI_TRACE("fd:%d | bufused:%d", fd, uvbufused);

    m3ApiWriteMem32(bufused, uvbufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArg(uvwasi_filesize_t, ಉದ್ದಳತೆ)
    m3ApiGetArg(uvwasi_advice_t, advice)

    uvwasi_errno_t ret = uvwasi_fd_advise(_get_uvwasi_p(_ctx), fd, offset, ಉದ್ದಳತೆ, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, ಉದ್ದಳತೆ, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArg(uvwasi_filesize_t, ಉದ್ದಳತೆ)

    uvwasi_errno_t ret = uvwasi_fd_allocate(_get_uvwasi_p(_ctx), fd, offset, ಉದ್ದಳತೆ);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, ಉದ್ದಳತೆ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_close)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)

    uvwasi_errno_t ret = uvwasi_fd_close(_get_uvwasi_p(_ctx), fd);

    WASI_TRACE("fd:%d", fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_datasync)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)

    uvwasi_errno_t ret = uvwasi_fd_datasync(_get_uvwasi_p(_ctx), fd);

    WASI_TRACE("fd:%d", fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_random_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint8_t*, buf)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, buf_len)

    m3ApiCheckMem(buf, buf_len);

    uvwasi_errno_t ret = uvwasi_random_get(_get_uvwasi_p(_ctx), buf, buf_len);

    WASI_TRACE("len:%d", buf_len);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_res_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_clockid_t, wasi_clk_id)
    m3ApiGetArgMem(uvwasi_timestamp_t*, resolution)

    m3ApiCheckMem(resolution, ಗಾತ್ರ(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_res_get(_get_uvwasi_p(_ctx), wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    m3ApiWriteMem64(resolution, t);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_ಸಮಯ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(uvwasi_timestamp_t, precision)
    m3ApiGetArgMem(uvwasi_timestamp_t*, time)

    m3ApiCheckMem(time, ಗಾತ್ರ(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_ಸಮಯ_get(_get_uvwasi_p(_ctx), wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    m3ApiWriteMem64(time, t);

    m3ApiReturn(ret);
}

// poll_oneoff is the one call whose arguments are arrays of structs in both
// directions, and neither array can be handed to uvwasi where it lies: a guest
// pointer carries no alignment guarantee, and on a big-endian host every field in it
// is the wrong way round. So both are marshalled, the same way every other struct in
// this file is - see fd_filestat_get and friends.
//
// The records the guest passes by address, sized as the ABI lays them out rather
// than as uvwasi happens to - see read_subscriptions and write_event.
#  define d_m3_Wasi_Subscription_ಗಾತ್ರ            48

#  define d_m3_Wasi_Event_ಗಾತ್ರ                   32

// Reads the guest's subscription list into host structs. The caller has already
// bounds-checked i_in for i_count records. 48 bytes a record: userdata 0, type 8,
// fd 16, and a clock's fields from 16.
ಸ್ಥಿರ
ಶೂನ್ಯ read_subscriptions (uvwasi_subscription_t* o_subs, ಸ್ಥಿರಾಂಕ uint8_t* i_ಒಳ,
                         uvwasi_ಗಾತ್ರ_t i_ಎಣಿಕೆ)
{
    ಗಾಗಿ (uvwasi_ಗಾತ್ರ_t i = 0; i < i_ಎಣಿಕೆ; ++i) {
        ಸ್ಥಿರಾಂಕ uint8_t* s = i_ಒಳ + (size_t)i * d_m3_Wasi_Subscription_ಗಾತ್ರ;

        memset(&o_subs[i], 0, ಗಾತ್ರ(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        ಆದರೆ (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } ಇಲ್ಲದಿದ್ದರೆ {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ಸ್ಥಿರ
ಶೂನ್ಯ write_event (uint8_t* o_rec, ಸ್ಥಿರಾಂಕ uvwasi_event_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_ಗಾತ್ರ);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->u.fd_readwrite.nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->u.fd_readwrite.flags);
}

// Waits on the subscriptions, once whichever reader above has decoded them, and
// writes the events back. Knows nothing about revisions: the event record is the
// same in both. i_subs is NULL when there are none, which uvwasi answers for itself.
ಸ್ಥಿರ
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, ಸ್ಥಿರಾಂಕ uvwasi_subscription_t* i_subs,
                                        uint8_t* o_ಹೊರಹೋಗು, uvwasi_ಗಾತ್ರ_t i_ಎಣಿಕೆ,
                                        uvwasi_ಗಾತ್ರ_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_ಎಣಿಕೆ ? m3_AllocArray(uvwasi_event_t, i_ಎಣಿಕೆ) : ಖಾಲಿ;

    ಆದರೆ (i_ಎಣಿಕೆ and not events) {
        ಹಿಂತಿರುಗಿ UVWASI_ENOMEM;
    }

    uvwasi_ಗಾತ್ರ_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_ಎಣಿಕೆ, &written);

    ಆದರೆ (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_ಎಣಿಕೆ);

        ಗಾಗಿ (uvwasi_ಗಾತ್ರ_t i = 0; i < written; ++i) {
            write_event(o_ಹೊರಹೋಗು + (size_t)i * d_m3_Wasi_Event_ಗಾತ್ರ, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    ಹಿಂತಿರುಗಿ ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ uint8_t*, ಒಳ)
    m3ApiGetArgMem(uint8_t*, ಹೊರಹೋಗು)
    m3ApiGetArg(uvwasi_ಗಾತ್ರ_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_ಗಾತ್ರ_t*, nevents)

    m3ApiCheckMem(ಒಳ, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_ಗಾತ್ರ);
    m3ApiCheckMem(ಹೊರಹೋಗು, (uint64_t)nsubscriptions * d_m3_Wasi_Event_ಗಾತ್ರ);
    m3ApiCheckMem(nevents, ಗಾತ್ರ(uvwasi_ಗಾತ್ರ_t));

    uvwasi_subscription_t* subs = ಖಾಲಿ;

    ಆದರೆ (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        ಆದರೆ (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, ಒಳ, nsubscriptions);
    }

    uvwasi_ಗಾತ್ರ_t ಎಣಿಕೆ_2 = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, ಹೊರಹೋಗು, nsubscriptions, &ಎಣಿಕೆ_2);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, ಎಣಿಕೆ_2);

    m3ApiWriteMem32(nevents, ಎಣಿಕೆ_2);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, ಸಂಕೇತ_2)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಆದರೆ (context) {
        context->exit_ಸಂಕೇತ = ಸಂಕೇತ_2;
    }

    m3ApiTrap(m3Err_trapWasiExit);
}

m3ApiRawFunction(m3_wasi_p1_proc_raise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_signal_t, sig)

    uvwasi_errno_t ret = uvwasi_proc_raise(_get_uvwasi_p(_ctx), sig);

    WASI_TRACE("sig:%d", sig);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_sched_yield)
{
    m3ApiReturnType(uint32_t)
    uvwasi_errno_t ret = uvwasi_sched_yield(_get_uvwasi_p(_ctx));

    WASI_TRACE("");

    m3ApiReturn(ret);
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SuppressLookupFailure (M3_ಫಲಿತಾಂಶ i_ಫಲಿತಾಂಶ)
{
    ಆದರೆ (i_ಫಲಿತಾಂಶ == m3Err_functionLookupFailed) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ i_ಫಲಿತಾಂಶ;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    ಹಿಂತಿರುಗಿ wasi_context;
}

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ಖಾಲಿ,
};

ಸ್ಥಿರ uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_ಮಾರ್ಗ = "/",  .real_ಮಾರ್ಗ = "." },
    { .mapped_ಮಾರ್ಗ = "./", .real_ಮಾರ್ಗ = "." },
};

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ m3_ಗಣ_ಮೇಲೆ_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = ಗಾತ್ರ(DEFAULT_PREOPENS) / ಗಾತ್ರ(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ಗಾಗಿ (size_t i = 0; i < M3_ಎಣಿಕೆ_ದ(namespaces); i++) {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_ವಾದಗಳು_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_ವಾದಗಳು_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_ಸಮಯ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_ಗಣ_flags, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_ಗಣ_rights, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_ಗಣ_ಗಾತ್ರ, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_ಗಣ_times, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_pread",             "i(i*iI*)",&m3_wasi_p1_fd_pread, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_prestat_get",       "i(i*)",   &m3_wasi_p1_fd_prestat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_prestat_dir_name",  "i(i*i)",  &m3_wasi_p1_fd_prestat_dir_name, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_pwrite",            "i(i*iI*)",&m3_wasi_p1_fd_pwrite, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_read",              "i(i*i*)", &m3_wasi_p1_fd_read, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_readdir",           "i(i*iI*)",&m3_wasi_p1_fd_readdir, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_renumber",          "i(ii)",   &m3_wasi_p1_fd_renumber, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_sync",              "i(i)",    &m3_wasi_p1_fd_sync, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_seek",               "i(iIi*)",     &m3_wasi_p1_fd_seek, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_tell",              "i(i*)",   &m3_wasi_p1_fd_tell, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_write",             "i(i*i*)", &m3_wasi_p1_fd_write, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_ಮಾರ್ಗ_create_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_ಮಾರ್ಗ_filestat_get, wasi_context)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_ಮಾರ್ಗ_open, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_ಮಾರ್ಗ_readlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_ಮಾರ್ಗ_remove_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_ಮಾರ್ಗ_rename, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_ಮಾರ್ಗ_symlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_ಮಾರ್ಗ_unlink_file, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "poll_oneoff",           "i(**i*)",     &m3_wasi_p1_poll_oneoff, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "proc_exit",          "v(i)",    &m3_wasi_p1_proc_exit, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "proc_raise",           "i(i)",    &m3_wasi_p1_proc_raise, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "random_get",           "i(*i)",   &m3_wasi_p1_random_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "sched_yield",          "i()",     &m3_wasi_p1_sched_yield, wasi_context)));

//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sock_recv",            "i(i*ii**)",        )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sock_send",            "i(i*ii*)",         )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sock_shutdown",        "i(ii)",            )));
    }

    // clang-format on

    // WASI addresses the memory the module exports as "memory", not whichever
    // one the calling code happens to be running against. Resolve it once, here,
    // rather than per call. A module that linked none of the above imports has
    // no WASI to satisfy, so it is not asked for the export - link_all offers
    // WASI to every module, and most have neither the imports nor a memory.
    ಆದರೆ (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ;

        ಆದರೆ (m3_ಕಂಡುಹಿಡಿ_Exported_ಸ್ಮರಣೆ(module, "memory", &ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_ಸ್ಮರಣೆ(module, "wasi_snapshot_preview1", ಸ್ಮರಣೆ_ಸೂಚ್ಯಂಕ));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

M3_ಫಲಿತಾಂಶ m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    ಪ್ರತಿಜ್ಞಾ(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    ಆದರೆ (ret != UVWASI_ESUCCESS) {
        ಹಿಂತಿರುಗಿ "uvwasi_init failed";
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

M3_ಫಲಿತಾಂಶ m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    m3_ಗಣ_ಮೇಲೆ_default_uvwasi_opts(&opts);

    ಹಿಂತಿರುಗಿ m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3_ಫಲಿತಾಂಶ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ಹಿಂತಿರುಗಿ _linkWASI(module, wasi);
}

ಶೂನ್ಯ m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3_ಫಲಿತಾಂಶ m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    m3_ಗಣ_ಮೇಲೆ_default_uvwasi_opts(&opts);

    ಹಿಂತಿರುಗಿ m3_LinkWASIWithOptions(module, opts);
}

M3_ಫಲಿತಾಂಶ m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    ಆದರೆ (ಖಾಲಿ == wasi_context) {
        M3_ಫಲಿತಾಂಶ res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        ಆದರೆ (res) {
            ಹಿಂತಿರುಗಿ res;
        }
    }

    ಹಿಂತಿರುಗಿ _linkWASI(module, wasi_context);
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasUVWASI
