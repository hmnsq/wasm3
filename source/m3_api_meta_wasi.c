//
//  m3_api_meta_wasi.c
//
//  Created by Volodymyr Shymanskyy on 01/08/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_wasi.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"

#ਜੇ defined(d_m3HasMetaWASI)

// NOTE: MetaWASI mostly redirects WASI calls to the host WASI environment

#  ਜੇ !defined(__wasi__)
#    error "MetaWASI is only supported on WASI target"
#  endif

#  ਜੇ __has_include("wasi/api.h")
#    include <wasi/api.h>
#    define USE_NEW_WASI
#    define WASI_STAT_ਖੇਤਰ(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_ਖੇਤਰ(f) st_##f

#  ਨਹੀਂਤਾਂ
#    error "Missing WASI headers"
#  endif

ਸਥਿਰ m3_wasi_context_t* wasi_context;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ size_t __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3_Wasi_ਰਸਤਾ_Max   PATH_MAX
#  ਨਹੀਂਤਾਂ
#    define d_m3_Wasi_ਰਸਤਾ_Max   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3_Wasi_ਰਸਤਾ(NAME, PTR, LEN)                                   \
      ਅੱਖਰ NAME [d_m3_Wasi_ਰਸਤਾ_Max];                                       \
      ਜੇ ((LEN) >= ਆਕਾਰ (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      ਸਮ੍ਰਿਤੀ_ਨਕਲ (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਅਚਲ ਖਾਲੀ* copy_iov_ਮੰਜ਼ਿਲ_host (IM3Runtime runtime, ਖਾਲੀ* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    ਲਈ (ਪੂਰਨ i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  ਜੇ d_m3EnableWasiTracing

ਅਚਲ ਅੱਖਰ* wasi_errno2str (__wasi_errno_t err)
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

ਅਚਲ ਅੱਖਰ* wasi_whence2str (__wasi_whence_t whence)
{
    ਚੋਣ (whence) {
    ਹਾਲਤ __WASI_WHENCE_SET: ਵਾਪਸ "SET";
    ਹਾਲਤ __WASI_WHENCE_CUR: ਵਾਪਸ "CUR";
    ਹਾਲਤ __WASI_WHENCE_END: ਵਾਪਸ "END";
    ਮੂਲ: ਵਾਪਸ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ਨਹੀਂਤਾਂ
#    define WASI_TRACE(fmt, ...)
#  endif

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
        m3ApiReturn(__WASI_ERRNO_INVAL);
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

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_ਦਲੀਲਾਂ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_ਮਾਪ)

    m3ApiCheckMem(argc, ਆਕਾਰ(__wasi_size_t));
    m3ApiCheckMem(argv_buf_ਮਾਪ, ਆਕਾਰ(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ਜੇ (context == ਨਿਰਾ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    ਲਈ (u32 i = 0; i < context->argc; ++i) {
        buf_len += ਲੜੀ_ਲੰਬਾਈ(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_ਮਾਪ, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ਅੱਖਰ*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_ਗਿਣਤੀ, env_buf_ਮਾਪ;

    ret = __wasi_environ_sizes_get(&env_ਗਿਣਤੀ, &env_buf_ਮਾਪ);
    ਜੇ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ਗਿਣਤੀ * ਆਕਾਰ(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_ਮਾਪ);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    ਜੇ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    ਲਈ (u32 i = 0; i < env_ਗਿਣਤੀ; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_ਗਿਣਤੀ)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_ਮਾਪ)

    m3ApiCheckMem(env_ਗਿਣਤੀ, ਆਕਾਰ(__wasi_size_t));
    m3ApiCheckMem(env_buf_ਮਾਪ, ਆਕਾਰ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_ਗਿਣਤੀ, env_buf_ਮਾਪ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    __wasi_errno_t ret = __wasi_fd_prestat_dir_name(fd, (uint8_t*)ਰਸਤਾ, ਰਸਤਾ_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, ਰਸਤਾ_len, ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_prestat_t*, buf)

    m3ApiCheckMem(buf, ਆਕਾਰ(__wasi_prestat_t));

    __wasi_errno_t ret = __wasi_fd_prestat_get(fd, buf);

    WASI_TRACE("fd:%d | type:%d, name_len:%d", fd, buf->pr_type, buf->u.dir.pr_name_len);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, fdstat)

    m3ApiCheckMem(fdstat, 24); // wasi_fdstat_t

    // The guest's buffer lands wherever its linear memory happens to sit, so it can't be
    // handed to the host directly: fdstat_t is 8-aligned and strict runtimes reject a
    // misaligned pointer. Bounce it through a local, as with filestat below.
    __wasi_fdstat_t stat;

    __wasi_errno_t ret = __wasi_fd_fdstat_get(fd, &stat);

    WASI_TRACE("fd:%d", fd);

    ਜੇ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(fdstat, 0, 24);
    m3ApiWriteMem8(fdstat + 0, stat.fs_filetype);
    m3ApiWriteMem16(fdstat + 2, stat.fs_flags);
    m3ApiWriteMem64(fdstat + 8, stat.fs_rights_ਅਧਾਰ);
    m3ApiWriteMem64(fdstat + 16, stat.fs_rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ਸੈੱਟ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    __wasi_errno_t ret = __wasi_fd_fdstat_set_flags(fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ਸੈੱਟ_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_rights_t, rights_ਅਧਾਰ)
    m3ApiGetArg(__wasi_rights_t, rights_inheriting)

#  ਜੇ 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_ਅਧਾਰ, rights_inheriting);
#  ਨਹੀਂਤਾਂ
    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  endif

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_ਅਧਾਰ, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ਸੈੱਟ_ਮਾਪ)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, ਮਾਪ)

    __wasi_errno_t ret = __wasi_fd_filestat_set_size(fd, ਮਾਪ);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, ਮਾਪ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ਸੈੱਟ_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_timestamp_t, atim)
    m3ApiGetArg(__wasi_timestamp_t, mtim)
    m3ApiGetArg(__wasi_fstflags_t, fst_flags)

    __wasi_errno_t ret = __wasi_fd_filestat_set_times(fd, atim, mtim, fst_flags);

    WASI_TRACE("fd:%d, atim:%" PRIu64 ", mtim:%" PRIu64 ", flags:%d", fd, atim, mtim, fst_flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    __wasi_errno_t ret = __wasi_fd_filestat_get(fd, &stat);

    WASI_TRACE("fd:%d | fs.size:%" PRIu64, fd, stat.WASI_STAT_ਖੇਤਰ(ਮਾਪ));

    ਜੇ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_ਖੇਤਰ(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_ਖੇਤਰ(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_ਖੇਤਰ(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_ਖੇਤਰ(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_ਖੇਤਰ(ਮਾਪ));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_ਖੇਤਰ(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_ਖੇਤਰ(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_ਖੇਤਰ(ctim));

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, ਨਤੀਜਾ)

    m3ApiCheckMem(ਨਤੀਜਾ, ਆਕਾਰ(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    ਚੋਣ (wasi_whence) {
    ਹਾਲਤ 0: whence = __WASI_WHENCE_SET; ਰੋਕੋ;
    ਹਾਲਤ 1: whence = __WASI_WHENCE_CUR; ਰੋਕੋ;
    ਹਾਲਤ 2: whence = __WASI_WHENCE_END; ਰੋਕੋ;
    }

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_seek(fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    *ਨਤੀਜਾ = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, from)
    m3ApiGetArg(__wasi_fd_t, ਮੰਜ਼ਿਲ)

    __wasi_errno_t ret = __wasi_fd_renumber(from, ਮੰਜ਼ਿਲ);

    WASI_TRACE("from:%d, to:%d", from, ਮੰਜ਼ਿਲ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_sync)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)

    __wasi_errno_t ret = __wasi_fd_sync(fd);

    WASI_TRACE("fd:%d", fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_tell)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_filesize_t*, ਨਤੀਜਾ)

    m3ApiCheckMem(ਨਤੀਜਾ, ਆਕਾਰ(__wasi_filesize_t));

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_tell(fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    *ਨਤੀਜਾ = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    d_m3_Wasi_ਰਸਤਾ(p, ਰਸਤਾ, ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ਪੂਰਨ)ਰਸਤਾ_len, ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)
    m3ApiGetArgMem(ਅੱਖਰ*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ਆਕਾਰ(__wasi_size_t));

    d_m3_Wasi_ਰਸਤਾ(p, ਰਸਤਾ, ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (ਪੂਰਨ)ਰਸਤਾ_len, ਰਸਤਾ, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    d_m3_Wasi_ਰਸਤਾ(p, ਰਸਤਾ, ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ਪੂਰਨ)ਰਸਤਾ_len, ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, old_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, old_ਰਸਤਾ_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, new_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, new_ਰਸਤਾ_len)

    m3ApiCheckMem(old_ਰਸਤਾ, old_ਰਸਤਾ_len);
    m3ApiCheckMem(new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    d_m3_Wasi_ਰਸਤਾ(op, old_ਰਸਤਾ, old_ਰਸਤਾ_len)
    d_m3_Wasi_ਰਸਤਾ(np, new_ਰਸਤਾ, new_ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (ਪੂਰਨ)old_ਰਸਤਾ_len, old_ਰਸਤਾ, new_fd, (ਪੂਰਨ)new_ਰਸਤਾ_len, new_ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, old_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, old_ਰਸਤਾ_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, new_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, new_ਰਸਤਾ_len)

    m3ApiCheckMem(old_ਰਸਤਾ, old_ਰਸਤਾ_len);
    m3ApiCheckMem(new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    d_m3_Wasi_ਰਸਤਾ(op, old_ਰਸਤਾ, old_ਰਸਤਾ_len)
    d_m3_Wasi_ਰਸਤਾ(np, new_ਰਸਤਾ, new_ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (ਪੂਰਨ)old_ਰਸਤਾ_len, old_ਰਸਤਾ, fd, (ਪੂਰਨ)new_ਰਸਤਾ_len, new_ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    d_m3_Wasi_ਰਸਤਾ(p, ਰਸਤਾ, ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ਪੂਰਨ)ਰਸਤਾ_len, ਰਸਤਾ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_ਅਧਾਰ)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(fd, ਆਕਾਰ(__wasi_fd_t));

    d_m3_Wasi_ਰਸਤਾ(p, ਰਸਤਾ, ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_open(dirfd,
                                          dirflags,
                                          p,
                                          oflags,
                                          fs_rights_ਅਧਾਰ,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          fd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%.*s, oflags:0x%x, fs_flags:0x%x | fd:%d",
               dirfd, dirflags, (ਪੂਰਨ)ਰਸਤਾ_len, ਰਸਤਾ, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(uint32_t, ਰਸਤਾ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3_Wasi_ਰਸਤਾ(p, ਰਸਤਾ, ਰਸਤਾ_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (ਪੂਰਨ)ਰਸਤਾ_len, ਰਸਤਾ, stat.WASI_STAT_ਖੇਤਰ(ਮਾਪ));

    ਜੇ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_ਖੇਤਰ(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_ਖੇਤਰ(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_ਖੇਤਰ(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_ਖੇਤਰ(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_ਖੇਤਰ(ਮਾਪ));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_ਖੇਤਰ(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_ਖੇਤਰ(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_ਖੇਤਰ(ctim));

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pread)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArgMem(__wasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(__wasi_iovec_t));
    m3ApiCheckMem(nread, ਆਕਾਰ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ਅਚਲ ਖਾਲੀ*    mem_check = copy_iov_ਮੰਜ਼ਿਲ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ਜੇ (mem_check != m3Err_none) {
        ਵਾਪਸ mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pread(fd, iovs, iovs_len, offset, nread);

    WASI_TRACE("fd:%d | nread:%d", fd, *nread);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(__wasi_iovec_t));
    m3ApiCheckMem(nread, ਆਕਾਰ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ਅਚਲ ਖਾਲੀ*    mem_check = copy_iov_ਮੰਜ਼ਿਲ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ਜੇ (mem_check != m3Err_none) {
        ਵਾਪਸ mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_read(fd, iovs, iovs_len, nread);

    WASI_TRACE("fd:%d | nread:%d", fd, *nread);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, ਆਕਾਰ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ਅਚਲ ਖਾਲੀ*    mem_check = copy_iov_ਮੰਜ਼ਿਲ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ਜੇ (mem_check != m3Err_none) {
        ਵਾਪਸ mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_write(fd, (__wasi_ciovec_t*)iovs, iovs_len, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pwrite)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, ਆਕਾਰ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ਅਚਲ ਖਾਲੀ*    mem_check = copy_iov_ਮੰਜ਼ਿਲ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ਜੇ (mem_check != m3Err_none) {
        ਵਾਪਸ mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pwrite(fd, (__wasi_ciovec_t*)iovs, iovs_len, offset, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਖਾਲੀ*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArg(__wasi_dircookie_t, cookie)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ਆਕਾਰ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_fd_readdir(fd, buf, buf_len, cookie, bufused);

    WASI_TRACE("fd:%d | bufused:%d", fd, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, ਲੰਮਾਈ)
    m3ApiGetArg(__wasi_advice_t, advice)

    __wasi_errno_t ret = __wasi_fd_advise(fd, offset, ਲੰਮਾਈ, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, ਲੰਮਾਈ, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, ਲੰਮਾਈ)

    __wasi_errno_t ret = __wasi_fd_allocate(fd, offset, ਲੰਮਾਈ);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, ਲੰਮਾਈ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_close)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)

    __wasi_errno_t ret = __wasi_fd_close(fd);

    WASI_TRACE("fd:%d", fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_datasync)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)

    __wasi_errno_t ret = __wasi_fd_datasync(fd);

    WASI_TRACE("fd:%d", fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_random_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint8_t*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)

    m3ApiCheckMem(buf, buf_len);

    __wasi_errno_t ret = __wasi_random_get(buf, buf_len);

    WASI_TRACE("len:%d", buf_len);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_res_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArgMem(__wasi_timestamp_t*, resolution)

    m3ApiCheckMem(resolution, ਆਕਾਰ(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_res_get(wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *resolution = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_ਸਮਾਂ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, ਆਕਾਰ(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *time = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ਅਚਲ __wasi_subscription_t*, ਵਿੱਚ)
    m3ApiGetArgMem(__wasi_event_t*, ਬਾਹਰਲਾ)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(ਵਿੱਚ, nsubscriptions * ਆਕਾਰ(__wasi_subscription_t));
    m3ApiCheckMem(ਬਾਹਰਲਾ, nsubscriptions * ਆਕਾਰ(__wasi_event_t));
    m3ApiCheckMem(nevents, ਆਕਾਰ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(ਵਿੱਚ, ਬਾਹਰਲਾ, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

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
    // proc_raise was dropped from preview1, so newer wasi-libc no longer has
    // __wasi_signal_t; the import signature is "i(i)" either way
    m3ApiGetArg(uint32_t, sig)

    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  ਜੇ 0
    ret = __wasi_proc_raise(sig);
#  endif

    WASI_TRACE("sig:%d", sig);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_sched_yield)
{
    m3ApiReturnType(uint32_t)
    __wasi_errno_t ret = __wasi_sched_yield();

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

ਖਾਲੀ m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ਲਈ (size_t i = 0; i < M3_ਗਿਣਤੀ_ਦਾ(namespaces); i++) {
        ਅਚਲ ਅੱਖਰ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_ਦਲੀਲਾਂ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_ਦਲੀਲਾਂ_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_ਸਮਾਂ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_ਸੈੱਟ_flags)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_ਸੈੱਟ_rights)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_ਸੈੱਟ_ਮਾਪ)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_ਸੈੱਟ_times)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_pread",             "i(i*iI*)",&m3_wasi_p1_fd_pread)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_prestat_get",       "i(i*)",   &m3_wasi_p1_fd_prestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_prestat_dir_name",  "i(i*i)",  &m3_wasi_p1_fd_prestat_dir_name)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_pwrite",            "i(i*iI*)",&m3_wasi_p1_fd_pwrite)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_read",              "i(i*i*)", &m3_wasi_p1_fd_read)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_readdir",           "i(i*iI*)",&m3_wasi_p1_fd_readdir)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_renumber",          "i(ii)",   &m3_wasi_p1_fd_renumber)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_sync",              "i(i)",    &m3_wasi_p1_fd_sync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_seek",               "i(iIi*)",     &m3_wasi_p1_fd_seek)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_tell",              "i(i*)",   &m3_wasi_p1_fd_tell)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_write",             "i(i*i*)", &m3_wasi_p1_fd_write)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_ਰਸਤਾ_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_ਰਸਤਾ_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_ਰਸਤਾ_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_ਰਸਤਾ_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_ਰਸਤਾ_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_unlink_file)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "poll_oneoff",           "i(**i*)",     &m3_wasi_p1_poll_oneoff)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "proc_exit",          "v(i)",    &m3_wasi_p1_proc_exit, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "proc_raise",           "i(i)",    &m3_wasi_p1_proc_raise)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "random_get",           "i(*i)",   &m3_wasi_p1_random_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sched_yield",          "i()",     &m3_wasi_p1_sched_yield)));

//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sock_recv",            "i(i*ii**)",        )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sock_send",            "i(i*ii*)",         )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sock_shutdown",        "i(ii)",            )));
    }
    // clang-format off

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

M3_ਨਤੀਜਾ m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    ਵਾਪਸ m3Err_none;
}

ਖਾਲੀ m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_ਨਤੀਜਾ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ਵਾਪਸ _linkWASI(module, wasi);
}

M3_ਨਤੀਜਾ m3_LinkWASI (IM3Module module)
{
    ਜੇ (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    ਵਾਪਸ _linkWASI(module, wasi_context);
}

#ਜੇ_ਅੰਤ // d_m3HasMetaWASI
