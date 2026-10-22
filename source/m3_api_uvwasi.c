//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_api_wasi.h"

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"

#ಯದಿ defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
ಬಾಹ್ಯ ವರ್ಣ** environ;
#  endif

ಸ್ಥಿರ m3_wasi_context_t* wasi_context = ರಿಕ್ತ;

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ wasi_iovec_t {
    uvwasi_size_t buf;
    uvwasi_size_t buf_len;
} wasi_iovec_t;

#  ಯದಿ d_m3EnableWasiTracing

ಧ್ರುವ ವರ್ಣ* wasi_errno2str (uvwasi_errno_t err)
{
    // clang-format off
    ವಿಕಲ್ಪ (err) {
    ಪಕ್ಷ  0: ಪ್ರತ್ಯಾಗಮ "ESUCCESS";
    ಪಕ್ಷ  1: ಪ್ರತ್ಯಾಗಮ "E2BIG";
    ಪಕ್ಷ  2: ಪ್ರತ್ಯಾಗಮ "EACCES";
    ಪಕ್ಷ  3: ಪ್ರತ್ಯಾಗಮ "EADDRINUSE";
    ಪಕ್ಷ  4: ಪ್ರತ್ಯಾಗಮ "EADDRNOTAVAIL";
    ಪಕ್ಷ  5: ಪ್ರತ್ಯಾಗಮ "EAFNOSUPPORT";
    ಪಕ್ಷ  6: ಪ್ರತ್ಯಾಗಮ "EAGAIN";
    ಪಕ್ಷ  7: ಪ್ರತ್ಯಾಗಮ "EALREADY";
    ಪಕ್ಷ  8: ಪ್ರತ್ಯಾಗಮ "EBADF";
    ಪಕ್ಷ  9: ಪ್ರತ್ಯಾಗಮ "EBADMSG";
    ಪಕ್ಷ 10: ಪ್ರತ್ಯಾಗಮ "EBUSY";
    ಪಕ್ಷ 11: ಪ್ರತ್ಯಾಗಮ "ECANCELED";
    ಪಕ್ಷ 12: ಪ್ರತ್ಯಾಗಮ "ECHILD";
    ಪಕ್ಷ 13: ಪ್ರತ್ಯಾಗಮ "ECONNABORTED";
    ಪಕ್ಷ 14: ಪ್ರತ್ಯಾಗಮ "ECONNREFUSED";
    ಪಕ್ಷ 15: ಪ್ರತ್ಯಾಗಮ "ECONNRESET";
    ಪಕ್ಷ 16: ಪ್ರತ್ಯಾಗಮ "EDEADLK";
    ಪಕ್ಷ 17: ಪ್ರತ್ಯಾಗಮ "EDESTADDRREQ";
    ಪಕ್ಷ 18: ಪ್ರತ್ಯಾಗಮ "EDOM";
    ಪಕ್ಷ 19: ಪ್ರತ್ಯಾಗಮ "EDQUOT";
    ಪಕ್ಷ 20: ಪ್ರತ್ಯಾಗಮ "EEXIST";
    ಪಕ್ಷ 21: ಪ್ರತ್ಯಾಗಮ "EFAULT";
    ಪಕ್ಷ 22: ಪ್ರತ್ಯಾಗಮ "EFBIG";
    ಪಕ್ಷ 23: ಪ್ರತ್ಯಾಗಮ "EHOSTUNREACH";
    ಪಕ್ಷ 24: ಪ್ರತ್ಯಾಗಮ "EIDRM";
    ಪಕ್ಷ 25: ಪ್ರತ್ಯಾಗಮ "EILSEQ";
    ಪಕ್ಷ 26: ಪ್ರತ್ಯಾಗಮ "EINPROGRESS";
    ಪಕ್ಷ 27: ಪ್ರತ್ಯಾಗಮ "EINTR";
    ಪಕ್ಷ 28: ಪ್ರತ್ಯಾಗಮ "EINVAL";
    ಪಕ್ಷ 29: ಪ್ರತ್ಯಾಗಮ "EIO";
    ಪಕ್ಷ 30: ಪ್ರತ್ಯಾಗಮ "EISCONN";
    ಪಕ್ಷ 31: ಪ್ರತ್ಯಾಗಮ "EISDIR";
    ಪಕ್ಷ 32: ಪ್ರತ್ಯಾಗಮ "ELOOP";
    ಪಕ್ಷ 33: ಪ್ರತ್ಯಾಗಮ "EMFILE";
    ಪಕ್ಷ 34: ಪ್ರತ್ಯಾಗಮ "EMLINK";
    ಪಕ್ಷ 35: ಪ್ರತ್ಯಾಗಮ "EMSGSIZE";
    ಪಕ್ಷ 36: ಪ್ರತ್ಯಾಗಮ "EMULTIHOP";
    ಪಕ್ಷ 37: ಪ್ರತ್ಯಾಗಮ "ENAMETOOLONG";
    ಪಕ್ಷ 38: ಪ್ರತ್ಯಾಗಮ "ENETDOWN";
    ಪಕ್ಷ 39: ಪ್ರತ್ಯಾಗಮ "ENETRESET";
    ಪಕ್ಷ 40: ಪ್ರತ್ಯಾಗಮ "ENETUNREACH";
    ಪಕ್ಷ 41: ಪ್ರತ್ಯಾಗಮ "ENFILE";
    ಪಕ್ಷ 42: ಪ್ರತ್ಯಾಗಮ "ENOBUFS";
    ಪಕ್ಷ 43: ಪ್ರತ್ಯಾಗಮ "ENODEV";
    ಪಕ್ಷ 44: ಪ್ರತ್ಯಾಗಮ "ENOENT";
    ಪಕ್ಷ 45: ಪ್ರತ್ಯಾಗಮ "ENOEXEC";
    ಪಕ್ಷ 46: ಪ್ರತ್ಯಾಗಮ "ENOLCK";
    ಪಕ್ಷ 47: ಪ್ರತ್ಯಾಗಮ "ENOLINK";
    ಪಕ್ಷ 48: ಪ್ರತ್ಯಾಗಮ "ENOMEM";
    ಪಕ್ಷ 49: ಪ್ರತ್ಯಾಗಮ "ENOMSG";
    ಪಕ್ಷ 50: ಪ್ರತ್ಯಾಗಮ "ENOPROTOOPT";
    ಪಕ್ಷ 51: ಪ್ರತ್ಯಾಗಮ "ENOSPC";
    ಪಕ್ಷ 52: ಪ್ರತ್ಯಾಗಮ "ENOSYS";
    ಪಕ್ಷ 53: ಪ್ರತ್ಯಾಗಮ "ENOTCONN";
    ಪಕ್ಷ 54: ಪ್ರತ್ಯಾಗಮ "ENOTDIR";
    ಪಕ್ಷ 55: ಪ್ರತ್ಯಾಗಮ "ENOTEMPTY";
    ಪಕ್ಷ 56: ಪ್ರತ್ಯಾಗಮ "ENOTRECOVERABLE";
    ಪಕ್ಷ 57: ಪ್ರತ್ಯಾಗಮ "ENOTSOCK";
    ಪಕ್ಷ 58: ಪ್ರತ್ಯಾಗಮ "ENOTSUP";
    ಪಕ್ಷ 59: ಪ್ರತ್ಯಾಗಮ "ENOTTY";
    ಪಕ್ಷ 60: ಪ್ರತ್ಯಾಗಮ "ENXIO";
    ಪಕ್ಷ 61: ಪ್ರತ್ಯಾಗಮ "EOVERFLOW";
    ಪಕ್ಷ 62: ಪ್ರತ್ಯಾಗಮ "EOWNERDEAD";
    ಪಕ್ಷ 63: ಪ್ರತ್ಯಾಗಮ "EPERM";
    ಪಕ್ಷ 64: ಪ್ರತ್ಯಾಗಮ "EPIPE";
    ಪಕ್ಷ 65: ಪ್ರತ್ಯಾಗಮ "EPROTO";
    ಪಕ್ಷ 66: ಪ್ರತ್ಯಾಗಮ "EPROTONOSUPPORT";
    ಪಕ್ಷ 67: ಪ್ರತ್ಯಾಗಮ "EPROTOTYPE";
    ಪಕ್ಷ 68: ಪ್ರತ್ಯಾಗಮ "ERANGE";
    ಪಕ್ಷ 69: ಪ್ರತ್ಯಾಗಮ "EROFS";
    ಪಕ್ಷ 70: ಪ್ರತ್ಯಾಗಮ "ESPIPE";
    ಪಕ್ಷ 71: ಪ್ರತ್ಯಾಗಮ "ESRCH";
    ಪಕ್ಷ 72: ಪ್ರತ್ಯಾಗಮ "ESTALE";
    ಪಕ್ಷ 73: ಪ್ರತ್ಯಾಗಮ "ETIMEDOUT";
    ಪಕ್ಷ 74: ಪ್ರತ್ಯಾಗಮ "ETXTBSY";
    ಪಕ್ಷ 75: ಪ್ರತ್ಯಾಗಮ "EXDEV";
    ಪಕ್ಷ 76: ಪ್ರತ್ಯಾಗಮ "ENOTCAPABLE";
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "<unknown>";
    }
    // clang-format on
}

ಧ್ರುವ ವರ್ಣ* wasi_whence2str (uvwasi_whence_t whence)
{
    ವಿಕಲ್ಪ (whence) {
    ಪಕ್ಷ UVWASI_WHENCE_SET: ಪ್ರತ್ಯಾಗಮ "SET";
    ಪಕ್ಷ UVWASI_WHENCE_CUR: ಪ್ರತ್ಯಾಗಮ "CUR";
    ಪಕ್ಷ UVWASI_WHENCE_END: ಪ್ರತ್ಯಾಗಮ "END";
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ಅನ್ಯಥಾ
#    define WASI_TRACE(fmt, ...)
#  endif

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    ಪ್ರತಿಜ್ಞಾ(context != ರಿಕ್ತ);
    ಪ್ರತ್ಯಾಗಮ &context->uvwasi;
}

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_args_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(ವರ್ಣ*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಯದಿ (context == ರಿಕ್ತ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    m3ApiCheckMem(argv, context->argc * ಪರಿಮಾಣ(uint32_t));

    ಪ್ರತಿ (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3ApiPtrToOffset(argv_buf));

        size_t len = strlen(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        memcpy(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_args_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_size_t*, argc)
    m3ApiGetArgMem(uvwasi_size_t*, argv_buf_size)

    m3ApiCheckMem(argc, ಪರಿಮಾಣ(uvwasi_size_t));
    m3ApiCheckMem(argv_buf_size, ಪರಿಮಾಣ(uvwasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಯದಿ (context == ರಿಕ್ತ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_size_t buf_len = 0;
    ಪ್ರತಿ (u32 i = 0; i < context->argc; ++i) {
        buf_len += (uvwasi_size_t)strlen(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_size, buf_len);

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ವರ್ಣ*, env_buf)

    ವರ್ಣ**         environment;
    uvwasi_errno_t ret;
    uvwasi_size_t  env_ಎಣಿಕೆ, env_buf_size;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_ಎಣಿಕೆ, &env_buf_size);
    ಯದಿ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ಎಣಿಕೆ * ಪರಿಮಾಣ(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_size);

    environment = calloc(env_ಎಣಿಕೆ, ಪರಿಮಾಣ(ವರ್ಣ*));
    ಯದಿ (environment == ರಿಕ್ತ) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    ಯದಿ (ret != UVWASI_ESUCCESS) {
        ಮುಕ್ತ(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_offset = m3ApiPtrToOffset(env_buf);

    ಪ್ರತಿ (u32 i = 0; i < env_ಎಣಿಕೆ; ++i) {
        uint32_t offset = environ_buf_offset +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    ಮುಕ್ತ(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_size_t*, env_ಎಣಿಕೆ)
    m3ApiGetArgMem(uvwasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_ಎಣಿಕೆ, ಪರಿಮಾಣ(uvwasi_size_t));
    m3ApiCheckMem(env_buf_size, ಪರಿಮಾಣ(uvwasi_size_t));

    uvwasi_size_t ಎಣಿಕೆ;
    uvwasi_size_t buf_size;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &ಎಣಿಕೆ, &buf_size);

    m3ApiWriteMem32(env_ಎಣಿಕೆ, ಎಣಿಕೆ);
    m3ApiWriteMem32(env_buf_size, buf_size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ವರ್ಣ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(_get_uvwasi_p(_ctx), fd, path, path_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, path_len, path);

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

    ಯದಿ (ret != UVWASI_ESUCCESS) {
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

    ಯದಿ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 24);
    m3ApiWriteMem8(buf + 0, stat.fs_filetype);
    m3ApiWriteMem16(buf + 2, stat.fs_flags);
    m3ApiWriteMem64(buf + 8, stat.fs_rights_base);
    m3ApiWriteMem64(buf + 16, stat.fs_rights_inheriting);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_set_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_fdflags_t, flags)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_set_flags(_get_uvwasi_p(_ctx), fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_set_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_rights_t, rights_base)
    m3ApiGetArg(uvwasi_rights_t, rights_inheriting)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_set_rights(_get_uvwasi_p(_ctx), fd, rights_base, rights_inheriting);

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_base, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_set_size)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, size)

    uvwasi_errno_t ret = uvwasi_fd_filestat_set_size(_get_uvwasi_p(_ctx), fd, size);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_set_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_timestamp_t, atim)
    m3ApiGetArg(uvwasi_timestamp_t, mtim)
    m3ApiGetArg(uvwasi_fstflags_t, fst_flags)

    uvwasi_errno_t ret = uvwasi_fd_filestat_set_times(_get_uvwasi_p(_ctx), fd, atim, mtim, fst_flags);

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

    ಯದಿ (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArgMem(uvwasi_filesize_t*, result)

    m3ApiCheckMem(result, ಪರಿಮಾಣ(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    ವಿಕಲ್ಪ (wasi_whence) {
    ಪಕ್ಷ 0: whence = UVWASI_WHENCE_SET; ಭಙ್ಗ;
    ಪಕ್ಷ 1: whence = UVWASI_WHENCE_CUR; ಭಙ್ಗ;
    ಪಕ್ಷ 2: whence = UVWASI_WHENCE_END; ಭಙ್ಗ;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_seek(_get_uvwasi_p(_ctx), fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    m3ApiWriteMem64(result, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, from)
    m3ApiGetArg(uvwasi_fd_t, to)

    uvwasi_errno_t ret = uvwasi_fd_renumber(_get_uvwasi_p(_ctx), from, to);

    WASI_TRACE("from:%d, to:%d", from, to);

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
    m3ApiGetArgMem(uvwasi_filesize_t*, result)

    m3ApiCheckMem(result, ಪರಿಮಾಣ(uvwasi_filesize_t));

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_tell(_get_uvwasi_p(_ctx), fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    m3ApiWriteMem64(result, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    uvwasi_errno_t ret = uvwasi_path_create_directory(_get_uvwasi_p(_ctx), fd, path, path_len);

    WASI_TRACE("fd:%d, path:%s", fd, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArgMem(ವರ್ಣ*, buf)
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಪರಿಮಾಣ(uvwasi_size_t));

    uvwasi_size_t uvbufused;

    uvwasi_errno_t ret = uvwasi_path_readlink(_get_uvwasi_p(_ctx), fd, path, path_len, buf, buf_len, &uvbufused);

    WASI_TRACE("fd:%d, path:%s | buf:%s, bufused:%d", fd, path, buf, uvbufused);

    m3ApiWriteMem32(bufused, uvbufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    uvwasi_errno_t ret = uvwasi_path_remove_directory(_get_uvwasi_p(_ctx), fd, path, path_len);

    WASI_TRACE("fd:%d, path:%s", fd, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, old_fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, new_path)
    m3ApiGetArg(uvwasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    uvwasi_errno_t ret = uvwasi_path_rename(_get_uvwasi_p(_ctx), old_fd, old_path, old_path_len,
                                            new_fd, new_path, new_path_len);

    WASI_TRACE("old_fd:%d, old_path:%s, new_fd:%d, new_path:%s", old_fd, old_path, new_fd, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, new_path)
    m3ApiGetArg(uvwasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    uvwasi_errno_t ret = uvwasi_path_symlink(_get_uvwasi_p(_ctx), old_path, old_path_len,
                                             fd, new_path, new_path_len);

    WASI_TRACE("old_path:%s, fd:%d, new_path:%s", old_path, fd, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    uvwasi_errno_t ret = uvwasi_path_unlink_file(_get_uvwasi_p(_ctx), fd, path, path_len);

    WASI_TRACE("fd:%d, path:%s", fd, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, dirfd)
    m3ApiGetArg(uvwasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_base)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, ಪರಿಮಾಣ(uvwasi_fd_t));

    uvwasi_fd_t uvfd;

    uvwasi_errno_t ret = uvwasi_path_open(_get_uvwasi_p(_ctx),
                                          dirfd,
                                          dirflags,
                                          path,
                                          path_len,
                                          oflags,
                                          fs_rights_base,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          &uvfd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%s, oflags:0x%x, fs_flags:0x%x | fd:%d", dirfd, dirflags, path, oflags, fs_flags, uvfd);

    m3ApiWriteMem32(fd, uvfd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_lookupflags_t, flags)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_path_filestat_get(_get_uvwasi_p(_ctx), fd, flags, path, path_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, path, stat.st_size);

    ಯದಿ (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nread, ಪರಿಮಾಣ(uvwasi_size_t));

#  ಯದಿ M3_HAS_VLA
    ಯದಿ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ಅನ್ಯಥಾ
    ಯದಿ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    ಪ್ರತಿ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_size_t ಅಂಕೆ_read;

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
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nread, ಪರಿಮಾಣ(uvwasi_size_t));

#  ಯದಿ M3_HAS_VLA
    ಯದಿ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ಅನ್ಯಥಾ
    ಯದಿ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_size_t  ಅಂಕೆ_read;
    uvwasi_errno_t ret;

    ಪ್ರತಿ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
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
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಪರಿಮಾಣ(uvwasi_size_t));

#  ಯದಿ M3_HAS_VLA
    ಯದಿ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ಅನ್ಯಥಾ
    ಯದಿ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  ಅಂಕೆ_written;
    uvwasi_errno_t ret;

    ಪ್ರತಿ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
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
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಪರಿಮಾಣ(uvwasi_size_t));

#  ಯದಿ M3_HAS_VLA
    ಯದಿ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ಅನ್ಯಥಾ
    ಯದಿ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  ಅಂಕೆ_written;
    uvwasi_errno_t ret;

    ಪ್ರತಿ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
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
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಪರಿಮಾಣ(uvwasi_size_t));

    uvwasi_size_t  uvbufused;
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
    m3ApiGetArg(uvwasi_filesize_t, length)
    m3ApiGetArg(uvwasi_advice_t, advice)

    uvwasi_errno_t ret = uvwasi_fd_advise(_get_uvwasi_p(_ctx), fd, offset, length, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, length, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArg(uvwasi_filesize_t, length)

    uvwasi_errno_t ret = uvwasi_fd_allocate(_get_uvwasi_p(_ctx), fd, offset, length);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, length);

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
    m3ApiGetArg(uvwasi_size_t, buf_len)

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

    m3ApiCheckMem(resolution, ಪರಿಮಾಣ(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_res_get(_get_uvwasi_p(_ctx), wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    m3ApiWriteMem64(resolution, t);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_time_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(uvwasi_timestamp_t, precision)
    m3ApiGetArgMem(uvwasi_timestamp_t*, time)

    m3ApiCheckMem(time, ಪರಿಮಾಣ(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_time_get(_get_uvwasi_p(_ctx), wasi_clk_id, precision, &t);

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
#  define d_m3WasiSubscriptionSize            48

#  define d_m3WasiEventSize                   32

// Reads the guest's subscription list into host structs. The caller has already
// bounds-checked i_in for i_count records. 48 bytes a record: userdata 0, type 8,
// fd 16, and a clock's fields from 16.
ಸ್ಥಿರ
ಶೂನ್ಯ read_subscriptions (uvwasi_subscription_t* o_subs, ಧ್ರುವ uint8_t* i_in,
                         uvwasi_size_t i_ಎಣಿಕೆ)
{
    ಪ್ರತಿ (uvwasi_size_t i = 0; i < i_ಎಣಿಕೆ; ++i) {
        ಧ್ರುವ uint8_t* s = i_in + (size_t)i * d_m3WasiSubscriptionSize;

        memset(&o_subs[i], 0, ಪರಿಮಾಣ(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        ಯದಿ (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } ಅನ್ಯಥಾ {
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
ಶೂನ್ಯ write_event (uint8_t* o_rec, ಧ್ರುವ uvwasi_event_t* i_event)
{
    memset(o_rec, 0, d_m3WasiEventSize);

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
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, ಧ್ರುವ uvwasi_subscription_t* i_subs,
                                        uint8_t* o_out, uvwasi_size_t i_ಎಣಿಕೆ,
                                        uvwasi_size_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_ಎಣಿಕೆ ? m3_AllocArray(uvwasi_event_t, i_ಎಣಿಕೆ) : ರಿಕ್ತ;

    ಯದಿ (i_ಎಣಿಕೆ and not events) {
        ಪ್ರತ್ಯಾಗಮ UVWASI_ENOMEM;
    }

    uvwasi_size_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_ಎಣಿಕೆ, &written);

    ಯದಿ (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_ಎಣಿಕೆ);

        ಪ್ರತಿ (uvwasi_size_t i = 0; i < written; ++i) {
            write_event(o_out + (size_t)i * d_m3WasiEventSize, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    ಪ್ರತ್ಯಾಗಮ ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಧ್ರುವ uint8_t*, in)
    m3ApiGetArgMem(uint8_t*, out)
    m3ApiGetArg(uvwasi_size_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_size_t*, nevents)

    m3ApiCheckMem(in, (uint64_t)nsubscriptions * d_m3WasiSubscriptionSize);
    m3ApiCheckMem(out, (uint64_t)nsubscriptions * d_m3WasiEventSize);
    m3ApiCheckMem(nevents, ಪರಿಮಾಣ(uvwasi_size_t));

    uvwasi_subscription_t* subs = ರಿಕ್ತ;

    ಯದಿ (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        ಯದಿ (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, in, nsubscriptions);
    }

    uvwasi_size_t ಎಣಿಕೆ = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, out, nsubscriptions, &ಎಣಿಕೆ);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, ಎಣಿಕೆ);

    m3ApiWriteMem32(nevents, ಎಣಿಕೆ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, code)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಯದಿ (context) {
        context->exit_code = code;
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
M3Result SuppressLookupFailure (M3Result i_result)
{
    ಯದಿ (i_result == m3Err_functionLookupFailed) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ i_result;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    ಪ್ರತ್ಯಾಗಮ wasi_context;
}

ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ರಿಕ್ತ,
};

ಸ್ಥಿರ uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_path = "/",  .real_path = "." },
    { .mapped_path = "./", .real_path = "." },
};

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
ಶೂನ್ಯ _m3_set_up_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = ಪರಿಮಾಣ(DEFAULT_PREOPENS) / ಪರಿಮಾಣ(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

ಸ್ಥಿರ
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ಪ್ರತಿ (size_t i = 0; i < M3_ಎಣಿಕೆ_OF(namespaces); i++) {
        ಧ್ರುವ ವರ್ಣ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_args_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_args_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_time_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_set_flags, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_set_rights, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_set_size, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_set_times, wasi_context)));
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

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_path_create_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_path_filestat_get, wasi_context)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_path_open, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_path_readlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_path_remove_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_path_rename, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_path_symlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_path_unlink_file, wasi_context)));

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
    ಯದಿ (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 memoryIndex;

        ಯದಿ (m3_FindExportedMemory(module, "memory", &memoryIndex)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_BindImportMemory(module, "wasi_snapshot_preview1", memoryIndex));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

M3Result m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    ಪ್ರತಿಜ್ಞಾ(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    ಯದಿ (ret != UVWASI_ESUCCESS) {
        ಪ್ರತ್ಯಾಗಮ "uvwasi_init failed";
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    ಪ್ರತ್ಯಾಗಮ m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ಪ್ರತ್ಯಾಗಮ _linkWASI(module, wasi);
}

ಶೂನ್ಯ m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    ಪ್ರತ್ಯಾಗಮ m3_LinkWASIWithOptions(module, opts);
}

M3Result m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    ಯದಿ (ರಿಕ್ತ == wasi_context) {
        M3Result res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        ಯದಿ (res) {
            ಪ್ರತ್ಯಾಗಮ res;
        }
    }

    ಪ್ರತ್ಯಾಗಮ _linkWASI(module, wasi_context);
}

#endif // d_m3HasUVWASI
