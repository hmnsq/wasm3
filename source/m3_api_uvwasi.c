//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_wasi.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"

#ਜੇ defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
ਬਾਹਰੀ ਅੱਖਰ** environ;
#  endif

ਸਥਿਰ m3_wasi_context_t* wasi_context = ਨਿਰਾ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ wasi_iovec_t {
    uvwasi_ਮਾਪ_t buf;
    uvwasi_ਮਾਪ_t buf_len;
} wasi_iovec_t;

#  ਜੇ d_m3EnableWasiTracing

ਅਚਲ ਅੱਖਰ* wasi_errno2str (uvwasi_errno_t err)
{
    // clang-format off
    ਚੋਣ (err) {
    ਹਾਲਤ  0: ਵਾਪਸ "ESUCCESS";
    ਹਾਲਤ  1: ਵਾਪਸ "E2BIG";
    ਹਾਲਤ  2: ਵਾਪਸ "EACCES";
    ਹਾਲਤ  3: ਵਾਪਸ "EADDRINUSE";
    ਹਾਲਤ  4: ਵਾਪਸ "EADDRNOTAVAIL";
    ਹਾਲਤ  5: ਵਾਪਸ "EAFNOSUPPORT";
    ਹਾਲਤ  6: ਵਾਪਸ "EAGAIN";
    ਹਾਲਤ  7: ਵਾਪਸ "EALREADY";
    ਹਾਲਤ  8: ਵਾਪਸ "EBADF";
    ਹਾਲਤ  9: ਵਾਪਸ "EBADMSG";
    ਹਾਲਤ 10: ਵਾਪਸ "EBUSY";
    ਹਾਲਤ 11: ਵਾਪਸ "ECANCELED";
    ਹਾਲਤ 12: ਵਾਪਸ "ECHILD";
    ਹਾਲਤ 13: ਵਾਪਸ "ECONNABORTED";
    ਹਾਲਤ 14: ਵਾਪਸ "ECONNREFUSED";
    ਹਾਲਤ 15: ਵਾਪਸ "ECONNRESET";
    ਹਾਲਤ 16: ਵਾਪਸ "EDEADLK";
    ਹਾਲਤ 17: ਵਾਪਸ "EDESTADDRREQ";
    ਹਾਲਤ 18: ਵਾਪਸ "EDOM";
    ਹਾਲਤ 19: ਵਾਪਸ "EDQUOT";
    ਹਾਲਤ 20: ਵਾਪਸ "EEXIST";
    ਹਾਲਤ 21: ਵਾਪਸ "EFAULT";
    ਹਾਲਤ 22: ਵਾਪਸ "EFBIG";
    ਹਾਲਤ 23: ਵਾਪਸ "EHOSTUNREACH";
    ਹਾਲਤ 24: ਵਾਪਸ "EIDRM";
    ਹਾਲਤ 25: ਵਾਪਸ "EILSEQ";
    ਹਾਲਤ 26: ਵਾਪਸ "EINPROGRESS";
    ਹਾਲਤ 27: ਵਾਪਸ "EINTR";
    ਹਾਲਤ 28: ਵਾਪਸ "EINVAL";
    ਹਾਲਤ 29: ਵਾਪਸ "EIO";
    ਹਾਲਤ 30: ਵਾਪਸ "EISCONN";
    ਹਾਲਤ 31: ਵਾਪਸ "EISDIR";
    ਹਾਲਤ 32: ਵਾਪਸ "ELOOP";
    ਹਾਲਤ 33: ਵਾਪਸ "EMFILE";
    ਹਾਲਤ 34: ਵਾਪਸ "EMLINK";
    ਹਾਲਤ 35: ਵਾਪਸ "EMSGSIZE";
    ਹਾਲਤ 36: ਵਾਪਸ "EMULTIHOP";
    ਹਾਲਤ 37: ਵਾਪਸ "ENAMETOOLONG";
    ਹਾਲਤ 38: ਵਾਪਸ "ENETDOWN";
    ਹਾਲਤ 39: ਵਾਪਸ "ENETRESET";
    ਹਾਲਤ 40: ਵਾਪਸ "ENETUNREACH";
    ਹਾਲਤ 41: ਵਾਪਸ "ENFILE";
    ਹਾਲਤ 42: ਵਾਪਸ "ENOBUFS";
    ਹਾਲਤ 43: ਵਾਪਸ "ENODEV";
    ਹਾਲਤ 44: ਵਾਪਸ "ENOENT";
    ਹਾਲਤ 45: ਵਾਪਸ "ENOEXEC";
    ਹਾਲਤ 46: ਵਾਪਸ "ENOLCK";
    ਹਾਲਤ 47: ਵਾਪਸ "ENOLINK";
    ਹਾਲਤ 48: ਵਾਪਸ "ENOMEM";
    ਹਾਲਤ 49: ਵਾਪਸ "ENOMSG";
    ਹਾਲਤ 50: ਵਾਪਸ "ENOPROTOOPT";
    ਹਾਲਤ 51: ਵਾਪਸ "ENOSPC";
    ਹਾਲਤ 52: ਵਾਪਸ "ENOSYS";
    ਹਾਲਤ 53: ਵਾਪਸ "ENOTCONN";
    ਹਾਲਤ 54: ਵਾਪਸ "ENOTDIR";
    ਹਾਲਤ 55: ਵਾਪਸ "ENOTEMPTY";
    ਹਾਲਤ 56: ਵਾਪਸ "ENOTRECOVERABLE";
    ਹਾਲਤ 57: ਵਾਪਸ "ENOTSOCK";
    ਹਾਲਤ 58: ਵਾਪਸ "ENOTSUP";
    ਹਾਲਤ 59: ਵਾਪਸ "ENOTTY";
    ਹਾਲਤ 60: ਵਾਪਸ "ENXIO";
    ਹਾਲਤ 61: ਵਾਪਸ "EOVERFLOW";
    ਹਾਲਤ 62: ਵਾਪਸ "EOWNERDEAD";
    ਹਾਲਤ 63: ਵਾਪਸ "EPERM";
    ਹਾਲਤ 64: ਵਾਪਸ "EPIPE";
    ਹਾਲਤ 65: ਵਾਪਸ "EPROTO";
    ਹਾਲਤ 66: ਵਾਪਸ "EPROTONOSUPPORT";
    ਹਾਲਤ 67: ਵਾਪਸ "EPROTOTYPE";
    ਹਾਲਤ 68: ਵਾਪਸ "ERANGE";
    ਹਾਲਤ 69: ਵਾਪਸ "EROFS";
    ਹਾਲਤ 70: ਵਾਪਸ "ESPIPE";
    ਹਾਲਤ 71: ਵਾਪਸ "ESRCH";
    ਹਾਲਤ 72: ਵਾਪਸ "ESTALE";
    ਹਾਲਤ 73: ਵਾਪਸ "ETIMEDOUT";
    ਹਾਲਤ 74: ਵਾਪਸ "ETXTBSY";
    ਹਾਲਤ 75: ਵਾਪਸ "EXDEV";
    ਹਾਲਤ 76: ਵਾਪਸ "ENOTCAPABLE";
    ਮੂਲ: ਵਾਪਸ "<unknown>";
    }
    // clang-format on
}

ਅਚਲ ਅੱਖਰ* wasi_whence2str (uvwasi_whence_t whence)
{
    ਚੋਣ (whence) {
    ਹਾਲਤ UVWASI_WHENCE_ਸੈੱਟ: ਵਾਪਸ "SET";
    ਹਾਲਤ UVWASI_WHENCE_CUR: ਵਾਪਸ "CUR";
    ਹਾਲਤ UVWASI_WHENCE_ਸਿਰਾ: ਵਾਪਸ "END";
    ਮੂਲ: ਵਾਪਸ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ਨਹੀਂਤਾਂ
#    define WASI_TRACE(fmt, ...)
#  endif

ਸਥਿਰ ਸਤਰਅੰਦਰ
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    ਪ੍ਰਤਿਜ੍ਞਾ(context != ਨਿਰਾ);
    ਵਾਪਸ &context->uvwasi;
}

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_ਦਲੀਲਾਂ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(ਅੱਖਰ*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ਜੇ (context == ਨਿਰਾ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    m3ApiCheckMem(argv, context->argc * ਆਕਾਰ(uint32_t));

    ਲਈ (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(argv_buf));

        size_t len = ਲੜੀ_ਲੰਬਾਈ(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_ਦਲੀਲਾਂ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, argc)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, argv_buf_ਮਾਪ)

    m3ApiCheckMem(argc, ਆਕਾਰ(uvwasi_ਮਾਪ_t));
    m3ApiCheckMem(argv_buf_ਮਾਪ, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ਜੇ (context == ਨਿਰਾ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_ਮਾਪ_t buf_len = 0;
    ਲਈ (u32 i = 0; i < context->argc; ++i) {
        buf_len += (uvwasi_ਮਾਪ_t)ਲੜੀ_ਲੰਬਾਈ(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_ਮਾਪ, buf_len);

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ਅੱਖਰ*, env_buf)

    ਅੱਖਰ**         environment;
    uvwasi_errno_t ret;
    uvwasi_ਮਾਪ_t  env_ਗਿਣਤੀ, env_buf_ਮਾਪ;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_ਗਿਣਤੀ, &env_buf_ਮਾਪ);
    ਜੇ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ਗਿਣਤੀ * ਆਕਾਰ(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_ਮਾਪ);

    environment = ਸਿਫਰ_ਸਮ੍ਰਿਤੀ_ਦਿਓ(env_ਗਿਣਤੀ, ਆਕਾਰ(ਅੱਖਰ*));
    ਜੇ (environment == ਨਿਰਾ) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    ਜੇ (ret != UVWASI_ESUCCESS) {
        ਮੁਕਤ(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_ਵਿੱਥ = m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(env_buf);

    ਲਈ (u32 i = 0; i < env_ਗਿਣਤੀ; ++i) {
        uint32_t offset = environ_buf_ਵਿੱਥ +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    ਮੁਕਤ(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, env_ਗਿਣਤੀ)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, env_buf_ਮਾਪ)

    m3ApiCheckMem(env_ਗਿਣਤੀ, ਆਕਾਰ(uvwasi_ਮਾਪ_t));
    m3ApiCheckMem(env_buf_ਮਾਪ, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

    uvwasi_ਮਾਪ_t ਗਿਣਤੀ_2;
    uvwasi_ਮਾਪ_t buf_ਮਾਪ;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &ਗਿਣਤੀ_2, &buf_ਮਾਪ);

    m3ApiWriteMem32(env_ਗਿਣਤੀ, ਗਿਣਤੀ_2);
    m3ApiWriteMem32(env_buf_ਮਾਪ, buf_ਮਾਪ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(_get_uvwasi_p(_ctx), fd, ਰਸਤਾ, ਰਸਤਾ_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, ਰਸਤਾ_len, ਰਸਤਾ);

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

    ਜੇ (ret != UVWASI_ESUCCESS) {
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

    ਜੇ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 24);
    m3ApiWriteMem8(buf + 0, stat.fs_filetype);
    m3ApiWriteMem16(buf + 2, stat.fs_flags);
    m3ApiWriteMem64(buf + 8, stat.fs_rights_ਅਧਾਰ);
    m3ApiWriteMem64(buf + 16, stat.fs_rights_inheriting);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ਸੈੱਟ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_fdflags_t, flags)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_ਸੈੱਟ_flags(_get_uvwasi_p(_ctx), fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ਸੈੱਟ_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_rights_t, rights_ਅਧਾਰ)
    m3ApiGetArg(uvwasi_rights_t, rights_inheriting)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_ਸੈੱਟ_rights(_get_uvwasi_p(_ctx), fd, rights_ਅਧਾਰ, rights_inheriting);

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_ਅਧਾਰ, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ਸੈੱਟ_ਮਾਪ)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, ਮਾਪ)

    uvwasi_errno_t ret = uvwasi_fd_filestat_ਸੈੱਟ_ਮਾਪ(_get_uvwasi_p(_ctx), fd, ਮਾਪ);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, ਮਾਪ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ਸੈੱਟ_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_timestamp_t, atim)
    m3ApiGetArg(uvwasi_timestamp_t, mtim)
    m3ApiGetArg(uvwasi_fstflags_t, fst_flags)

    uvwasi_errno_t ret = uvwasi_fd_filestat_ਸੈੱਟ_times(_get_uvwasi_p(_ctx), fd, atim, mtim, fst_flags);

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

    ਜੇ (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArgMem(uvwasi_filesize_t*, ਨਤੀਜਾ)

    m3ApiCheckMem(ਨਤੀਜਾ, ਆਕਾਰ(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    ਚੋਣ (wasi_whence) {
    ਹਾਲਤ 0: whence = UVWASI_WHENCE_ਸੈੱਟ; ਰੋਕੋ;
    ਹਾਲਤ 1: whence = UVWASI_WHENCE_CUR; ਰੋਕੋ;
    ਹਾਲਤ 2: whence = UVWASI_WHENCE_ਸਿਰਾ; ਰੋਕੋ;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_seek(_get_uvwasi_p(_ctx), fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    m3ApiWriteMem64(ਨਤੀਜਾ, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, from)
    m3ApiGetArg(uvwasi_fd_t, ਮੰਜ਼ਿਲ)

    uvwasi_errno_t ret = uvwasi_fd_renumber(_get_uvwasi_p(_ctx), from, ਮੰਜ਼ਿਲ);

    WASI_TRACE("from:%d, to:%d", from, ਮੰਜ਼ਿਲ);

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
    m3ApiGetArgMem(uvwasi_filesize_t*, ਨਤੀਜਾ)

    m3ApiCheckMem(ਨਤੀਜਾ, ਆਕਾਰ(uvwasi_filesize_t));

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_tell(_get_uvwasi_p(_ctx), fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    m3ApiWriteMem64(ਨਤੀਜਾ, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_create_directory(_get_uvwasi_p(_ctx), fd, ਰਸਤਾ, ਰਸਤਾ_len);

    WASI_TRACE("fd:%d, path:%s", fd, ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, ਰਸਤਾ_len)
    m3ApiGetArgMem(ਅੱਖਰ*, buf)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, buf_len)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, bufused)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

    uvwasi_ਮਾਪ_t uvbufused;

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_readlink(_get_uvwasi_p(_ctx), fd, ਰਸਤਾ, ਰਸਤਾ_len, buf, buf_len, &uvbufused);

    WASI_TRACE("fd:%d, path:%s | buf:%s, bufused:%d", fd, ਰਸਤਾ, buf, uvbufused);

    m3ApiWriteMem32(bufused, uvbufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_remove_directory(_get_uvwasi_p(_ctx), fd, ਰਸਤਾ, ਰਸਤਾ_len);

    WASI_TRACE("fd:%d, path:%s", fd, ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, old_fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, old_ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, old_ਰਸਤਾ_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, new_ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, new_ਰਸਤਾ_len)

    m3ApiCheckMem(old_ਰਸਤਾ, old_ਰਸਤਾ_len);
    m3ApiCheckMem(new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_rename(_get_uvwasi_p(_ctx), old_fd, old_ਰਸਤਾ, old_ਰਸਤਾ_len,
                                            new_fd, new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    WASI_TRACE("old_fd:%d, old_path:%s, new_fd:%d, new_path:%s", old_fd, old_ਰਸਤਾ, new_fd, new_ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, old_ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, old_ਰਸਤਾ_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, new_ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, new_ਰਸਤਾ_len)

    m3ApiCheckMem(old_ਰਸਤਾ, old_ਰਸਤਾ_len);
    m3ApiCheckMem(new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_symlink(_get_uvwasi_p(_ctx), old_ਰਸਤਾ, old_ਰਸਤਾ_len,
                                             fd, new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    WASI_TRACE("old_path:%s, fd:%d, new_path:%s", old_ਰਸਤਾ, fd, new_ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_unlink_file(_get_uvwasi_p(_ctx), fd, ਰਸਤਾ, ਰਸਤਾ_len);

    WASI_TRACE("fd:%d, path:%s", fd, ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, dirfd)
    m3ApiGetArg(uvwasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, ਰਸਤਾ_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_ਅਧਾਰ)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(fd, ਆਕਾਰ(uvwasi_fd_t));

    uvwasi_fd_t uvfd;

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_open(_get_uvwasi_p(_ctx),
                                          dirfd,
                                          dirflags,
                                          ਰਸਤਾ,
                                          ਰਸਤਾ_len,
                                          oflags,
                                          fs_rights_ਅਧਾਰ,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          &uvfd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%s, oflags:0x%x, fs_flags:0x%x | fd:%d", dirfd, dirflags, ਰਸਤਾ, oflags, fs_flags, uvfd);

    m3ApiWriteMem32(fd, uvfd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_lookupflags_t, flags)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uint32_t, ਰਸਤਾ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_ਰਸਤਾ_filestat_get(_get_uvwasi_p(_ctx), fd, flags, ਰਸਤਾ, ਰਸਤਾ_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, ਰਸਤਾ, stat.st_size);

    ਜੇ (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArg(uvwasi_ਮਾਪ_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nread, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

#  ਜੇ M3_HAS_VLA
    ਜੇ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ਨਹੀਂਤਾਂ
    ਜੇ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    ਲਈ (uvwasi_ਮਾਪ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_ਮਾਪ_t ਅੰਕ_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &ਅੰਕ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, ਅੰਕ_read);

    m3ApiWriteMem32(nread, ਅੰਕ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, iovs_len)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nread, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

#  ਜੇ M3_HAS_VLA
    ਜੇ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ਨਹੀਂਤਾਂ
    ਜੇ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_ਮਾਪ_t  ਅੰਕ_read;
    uvwasi_errno_t ret;

    ਲਈ (uvwasi_ਮਾਪ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &ਅੰਕ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, ਅੰਕ_read);

    m3ApiWriteMem32(nread, ਅੰਕ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, iovs_len)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

#  ਜੇ M3_HAS_VLA
    ਜੇ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ਨਹੀਂਤਾਂ
    ਜੇ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_ਮਾਪ_t  ਅੰਕ_written;
    uvwasi_errno_t ret;

    ਲਈ (uvwasi_ਮਾਪ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &ਅੰਕ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, ਅੰਕ_written);

    m3ApiWriteMem32(nwritten, ਅੰਕ_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pwrite)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

#  ਜੇ M3_HAS_VLA
    ਜੇ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ਨਹੀਂਤਾਂ
    ਜੇ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_ਮਾਪ_t  ਅੰਕ_written;
    uvwasi_errno_t ret;

    ਲਈ (uvwasi_ਮਾਪ_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &ਅੰਕ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, ਅੰਕ_written);

    m3ApiWriteMem32(nwritten, ਅੰਕ_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ਖਾਲੀ*, buf)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

    uvwasi_ਮਾਪ_t  uvbufused;
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
    m3ApiGetArg(uvwasi_filesize_t, ਲੰਮਾਈ)
    m3ApiGetArg(uvwasi_advice_t, advice)

    uvwasi_errno_t ret = uvwasi_fd_advise(_get_uvwasi_p(_ctx), fd, offset, ਲੰਮਾਈ, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, ਲੰਮਾਈ, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArg(uvwasi_filesize_t, ਲੰਮਾਈ)

    uvwasi_errno_t ret = uvwasi_fd_allocate(_get_uvwasi_p(_ctx), fd, offset, ਲੰਮਾਈ);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, ਲੰਮਾਈ);

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
    m3ApiGetArg(uvwasi_ਮਾਪ_t, buf_len)

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

    m3ApiCheckMem(resolution, ਆਕਾਰ(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_res_get(_get_uvwasi_p(_ctx), wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    m3ApiWriteMem64(resolution, t);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_ਸਮਾਂ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(uvwasi_timestamp_t, precision)
    m3ApiGetArgMem(uvwasi_timestamp_t*, time)

    m3ApiCheckMem(time, ਆਕਾਰ(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_ਸਮਾਂ_get(_get_uvwasi_p(_ctx), wasi_clk_id, precision, &t);

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
#  define d_m3_Wasi_Subscription_ਮਾਪ            48

#  define d_m3_Wasi_Event_ਮਾਪ                   32

// Reads the guest's subscription list into host structs. The caller has already
// bounds-checked i_in for i_count records. 48 bytes a record: userdata 0, type 8,
// fd 16, and a clock's fields from 16.
ਸਥਿਰ
ਖਾਲੀ read_subscriptions (uvwasi_subscription_t* o_subs, ਅਚਲ uint8_t* i_ਵਿੱਚ,
                         uvwasi_ਮਾਪ_t i_ਗਿਣਤੀ)
{
    ਲਈ (uvwasi_ਮਾਪ_t i = 0; i < i_ਗਿਣਤੀ; ++i) {
        ਅਚਲ uint8_t* s = i_ਵਿੱਚ + (size_t)i * d_m3_Wasi_Subscription_ਮਾਪ;

        memset(&o_subs[i], 0, ਆਕਾਰ(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        ਜੇ (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } ਨਹੀਂਤਾਂ {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ਸਥਿਰ
ਖਾਲੀ write_event (uint8_t* o_rec, ਅਚਲ uvwasi_event_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_ਮਾਪ);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->u.fd_readwrite.nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->u.fd_readwrite.flags);
}

// Waits on the subscriptions, once whichever reader above has decoded them, and
// writes the events back. Knows nothing about revisions: the event record is the
// same in both. i_subs is NULL when there are none, which uvwasi answers for itself.
ਸਥਿਰ
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, ਅਚਲ uvwasi_subscription_t* i_subs,
                                        uint8_t* o_ਬਾਹਰਲਾ, uvwasi_ਮਾਪ_t i_ਗਿਣਤੀ,
                                        uvwasi_ਮਾਪ_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_ਗਿਣਤੀ ? m3_AllocArray(uvwasi_event_t, i_ਗਿਣਤੀ) : ਨਿਰਾ;

    ਜੇ (i_ਗਿਣਤੀ and not events) {
        ਵਾਪਸ UVWASI_ENOMEM;
    }

    uvwasi_ਮਾਪ_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_ਗਿਣਤੀ, &written);

    ਜੇ (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_ਗਿਣਤੀ);

        ਲਈ (uvwasi_ਮਾਪ_t i = 0; i < written; ++i) {
            write_event(o_ਬਾਹਰਲਾ + (size_t)i * d_m3_Wasi_Event_ਮਾਪ, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    ਵਾਪਸ ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ਅਚਲ uint8_t*, ਵਿੱਚ)
    m3ApiGetArgMem(uint8_t*, ਬਾਹਰਲਾ)
    m3ApiGetArg(uvwasi_ਮਾਪ_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_ਮਾਪ_t*, nevents)

    m3ApiCheckMem(ਵਿੱਚ, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_ਮਾਪ);
    m3ApiCheckMem(ਬਾਹਰਲਾ, (uint64_t)nsubscriptions * d_m3_Wasi_Event_ਮਾਪ);
    m3ApiCheckMem(nevents, ਆਕਾਰ(uvwasi_ਮਾਪ_t));

    uvwasi_subscription_t* subs = ਨਿਰਾ;

    ਜੇ (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        ਜੇ (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, ਵਿੱਚ, nsubscriptions);
    }

    uvwasi_ਮਾਪ_t ਗਿਣਤੀ_2 = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, ਬਾਹਰਲਾ, nsubscriptions, &ਗਿਣਤੀ_2);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, ਗਿਣਤੀ_2);

    m3ApiWriteMem32(nevents, ਗਿਣਤੀ_2);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, ਕੋਡ)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ਜੇ (context) {
        context->exit_ਕੋਡ = ਕੋਡ;
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


ਸਥਿਰ
M3_ਨਤੀਜਾ SuppressLookupFailure (M3_ਨਤੀਜਾ i_ਨਤੀਜਾ)
{
    ਜੇ (i_ਨਤੀਜਾ == m3Err_functionLookupFailed) {
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ i_ਨਤੀਜਾ;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    ਵਾਪਸ wasi_context;
}

ਸਥਿਰ ਅਚਲ ਅੱਖਰ* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ਨਿਰਾ,
};

ਸਥਿਰ uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_ਰਸਤਾ = "/",  .real_ਰਸਤਾ = "." },
    { .mapped_ਰਸਤਾ = "./", .real_ਰਸਤਾ = "." },
};

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ m3_ਸੈੱਟ_ਉੱਪਰ_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = ਆਕਾਰ(DEFAULT_PREOPENS) / ਆਕਾਰ(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ਲਈ (size_t i = 0; i < M3_ਗਿਣਤੀ_ਦਾ(namespaces); i++) {
        ਅਚਲ ਅੱਖਰ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_ਦਲੀਲਾਂ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_ਦਲੀਲਾਂ_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_ਸਮਾਂ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_ਸੈੱਟ_flags, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_ਸੈੱਟ_rights, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_ਸੈੱਟ_ਮਾਪ, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_ਸੈੱਟ_times, wasi_context)));
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

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_create_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_ਰਸਤਾ_filestat_get, wasi_context)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_ਰਸਤਾ_open, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_ਰਸਤਾ_readlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_remove_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_ਰਸਤਾ_rename, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_ਰਸਤਾ_symlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_unlink_file, wasi_context)));

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
    ਜੇ (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ;

        ਜੇ (m3_ਲੱਭੋ_Exported_ਸਮ੍ਰਿਤੀ(module, "memory", &ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_ਸਮ੍ਰਿਤੀ(module, "wasi_snapshot_preview1", ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

M3_ਨਤੀਜਾ m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    ਪ੍ਰਤਿਜ੍ਞਾ(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    ਜੇ (ret != UVWASI_ESUCCESS) {
        ਵਾਪਸ "uvwasi_init failed";
    }

    ਵਾਪਸ m3Err_none;
}

M3_ਨਤੀਜਾ m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    m3_ਸੈੱਟ_ਉੱਪਰ_default_uvwasi_opts(&opts);

    ਵਾਪਸ m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3_ਨਤੀਜਾ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ਵਾਪਸ _linkWASI(module, wasi);
}

ਖਾਲੀ m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3_ਨਤੀਜਾ m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    m3_ਸੈੱਟ_ਉੱਪਰ_default_uvwasi_opts(&opts);

    ਵਾਪਸ m3_LinkWASIWithOptions(module, opts);
}

M3_ਨਤੀਜਾ m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    ਜੇ (ਨਿਰਾ == wasi_context) {
        M3_ਨਤੀਜਾ res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        ਜੇ (res) {
            ਵਾਪਸ res;
        }
    }

    ਵਾਪਸ _linkWASI(module, wasi_context);
}

#ਜੇ_ਅੰਤ // d_m3HasUVWASI
