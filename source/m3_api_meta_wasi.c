//
//  m3_api_meta_wasi.c
//
//  Created by Volodymyr Shymanskyy on 01/08/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_api_wasi.h"

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"

#ಯದಿ defined(d_m3HasMetaWASI)

// NOTE: MetaWASI mostly redirects WASI calls to the host WASI environment

#  ಯದಿ !defined(__wasi__)
#    error "MetaWASI is only supported on WASI target"
#  endif

#  ಯದಿ __has_include("wasi/api.h")
#    include <wasi/api.h>
#    define USE_NEW_WASI
#    define WASI_STAT_FIELD(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_FIELD(f) st_##f

#  ಅನ್ಯಥಾ
#    error "Missing WASI headers"
#  endif

ಸ್ಥಿರ m3_wasi_context_t* wasi_context;

ಪ್ರಕಾರನಿರ್ದೇಶ size_t __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3WasiPathMax   PATH_MAX
#  ಅನ್ಯಥಾ
#    define d_m3WasiPathMax   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3WasiPath(NAME, PTR, LEN)                                   \
      ವರ್ಣ NAME [d_m3WasiPathMax];                                       \
      ಯದಿ ((LEN) >= ಪರಿಮಾಣ (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      memcpy (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
ಧ್ರುವ ಶೂನ್ಯ* copy_iov_to_host (IM3Runtime runtime, ಶೂನ್ಯ* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3ApiOffsetToPtr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  ಯದಿ d_m3EnableWasiTracing

ಧ್ರುವ ವರ್ಣ* wasi_errno2str (__wasi_errno_t err)
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

ಧ್ರುವ ವರ್ಣ* wasi_whence2str (__wasi_whence_t whence)
{
    ವಿಕಲ್ಪ (whence) {
    ಪಕ್ಷ __WASI_WHENCE_SET: ಪ್ರತ್ಯಾಗಮ "SET";
    ಪಕ್ಷ __WASI_WHENCE_CUR: ಪ್ರತ್ಯಾಗಮ "CUR";
    ಪಕ್ಷ __WASI_WHENCE_END: ಪ್ರತ್ಯಾಗಮ "END";
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ಅನ್ಯಥಾ
#    define WASI_TRACE(fmt, ...)
#  endif

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
        m3ApiReturn(__WASI_ERRNO_INVAL);
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

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_args_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_size)

    m3ApiCheckMem(argc, ಪರಿಮಾಣ(__wasi_size_t));
    m3ApiCheckMem(argv_buf_size, ಪರಿಮಾಣ(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಯದಿ (context == ರಿಕ್ತ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    ಪ್ರತಿ (u32 i = 0; i < context->argc; ++i) {
        buf_len += strlen(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_size, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ವರ್ಣ*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_ಎಣಿಕೆ, env_buf_size;

    ret = __wasi_environ_sizes_get(&env_ಎಣಿಕೆ, &env_buf_size);
    ಯದಿ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ಎಣಿಕೆ * ಪರಿಮಾಣ(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_size);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    ಯದಿ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    ಪ್ರತಿ (u32 i = 0; i < env_ಎಣಿಕೆ; ++i) {
        m3ApiWriteMem32(&env[i], m3ApiPtrToOffset(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_ಎಣಿಕೆ)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_ಎಣಿಕೆ, ಪರಿಮಾಣ(__wasi_size_t));
    m3ApiCheckMem(env_buf_size, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_ಎಣಿಕೆ, env_buf_size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    __wasi_errno_t ret = __wasi_fd_prestat_dir_name(fd, (uint8_t*)path, path_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_prestat_t*, buf)

    m3ApiCheckMem(buf, ಪರಿಮಾಣ(__wasi_prestat_t));

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

    ಯದಿ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(fdstat, 0, 24);
    m3ApiWriteMem8(fdstat + 0, stat.fs_filetype);
    m3ApiWriteMem16(fdstat + 2, stat.fs_flags);
    m3ApiWriteMem64(fdstat + 8, stat.fs_rights_base);
    m3ApiWriteMem64(fdstat + 16, stat.fs_rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_set_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    __wasi_errno_t ret = __wasi_fd_fdstat_set_flags(fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_set_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_rights_t, rights_base)
    m3ApiGetArg(__wasi_rights_t, rights_inheriting)

#  ಯದಿ 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_base, rights_inheriting);
#  ಅನ್ಯಥಾ
    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  endif

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_base, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_set_size)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, size)

    __wasi_errno_t ret = __wasi_fd_filestat_set_size(fd, size);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_set_times)
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

    WASI_TRACE("fd:%d | fs.size:%" PRIu64, fd, stat.WASI_STAT_FIELD(size));

    ಯದಿ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_FIELD(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_FIELD(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_FIELD(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_FIELD(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_FIELD(size));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_FIELD(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_FIELD(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_FIELD(ctim));

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, result)

    m3ApiCheckMem(result, ಪರಿಮಾಣ(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    ವಿಕಲ್ಪ (wasi_whence) {
    ಪಕ್ಷ 0: whence = __WASI_WHENCE_SET; ಭಙ್ಗ;
    ಪಕ್ಷ 1: whence = __WASI_WHENCE_CUR; ಭಙ್ಗ;
    ಪಕ್ಷ 2: whence = __WASI_WHENCE_END; ಭಙ್ಗ;
    }

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_seek(fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    *result = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, from)
    m3ApiGetArg(__wasi_fd_t, to)

    __wasi_errno_t ret = __wasi_fd_renumber(from, to);

    WASI_TRACE("from:%d, to:%d", from, to);

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
    m3ApiGetArgMem(__wasi_filesize_t*, result)

    m3ApiCheckMem(result, ಪರಿಮಾಣ(__wasi_filesize_t));

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_tell(fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    *result = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ಪೂರ್ಣಾಙ್ಕ)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArgMem(ವರ್ಣ*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಪರಿಮಾಣ(__wasi_size_t));

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (ಪೂರ್ಣಾಙ್ಕ)path_len, path, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ಪೂರ್ಣಾಙ್ಕ)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (ಪೂರ್ಣಾಙ್ಕ)old_path_len, old_path, new_fd, (ಪೂರ್ಣಾಙ್ಕ)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (ಪೂರ್ಣಾಙ್ಕ)old_path_len, old_path, fd, (ಪೂರ್ಣಾಙ್ಕ)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ಪೂರ್ಣಾಙ್ಕ)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_base)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, ಪರಿಮಾಣ(__wasi_fd_t));

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_open(dirfd,
                                          dirflags,
                                          p,
                                          oflags,
                                          fs_rights_base,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          fd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%.*s, oflags:0x%x, fs_flags:0x%x | fd:%d",
               dirfd, dirflags, (ಪೂರ್ಣಾಙ್ಕ)path_len, path, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (ಪೂರ್ಣಾಙ್ಕ)path_len, path, stat.WASI_STAT_FIELD(size));

    ಯದಿ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_FIELD(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_FIELD(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_FIELD(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_FIELD(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_FIELD(size));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_FIELD(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_FIELD(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_FIELD(ctim));

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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(__wasi_iovec_t));
    m3ApiCheckMem(nread, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಧ್ರುವ ಶೂನ್ಯ*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಯದಿ (mem_check != m3Err_none) {
        ಪ್ರತ್ಯಾಗಮ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(__wasi_iovec_t));
    m3ApiCheckMem(nread, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಧ್ರುವ ಶೂನ್ಯ*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಯದಿ (mem_check != m3Err_none) {
        ಪ್ರತ್ಯಾಗಮ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಧ್ರುವ ಶೂನ್ಯ*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಯದಿ (mem_check != m3Err_none) {
        ಪ್ರತ್ಯಾಗಮ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಧ್ರುವ ಶೂನ್ಯ*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಯದಿ (mem_check != m3Err_none) {
        ಪ್ರತ್ಯಾಗಮ mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pwrite(fd, (__wasi_ciovec_t*)iovs, iovs_len, offset, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಶೂನ್ಯ*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArg(__wasi_dircookie_t, cookie)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_fd_readdir(fd, buf, buf_len, cookie, bufused);

    WASI_TRACE("fd:%d | bufused:%d", fd, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, length)
    m3ApiGetArg(__wasi_advice_t, advice)

    __wasi_errno_t ret = __wasi_fd_advise(fd, offset, length, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, length, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, length)

    __wasi_errno_t ret = __wasi_fd_allocate(fd, offset, length);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, length);

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

    m3ApiCheckMem(resolution, ಪರಿಮಾಣ(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_res_get(wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *resolution = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_time_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, ಪರಿಮಾಣ(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *time = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಧ್ರುವ __wasi_subscription_t*, in)
    m3ApiGetArgMem(__wasi_event_t*, out)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(in, nsubscriptions * ಪರಿಮಾಣ(__wasi_subscription_t));
    m3ApiCheckMem(out, nsubscriptions * ಪರಿಮಾಣ(__wasi_event_t));
    m3ApiCheckMem(nevents, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(in, out, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

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
    // proc_raise was dropped from preview1, so newer wasi-libc no longer has
    // __wasi_signal_t; the import signature is "i(i)" either way
    m3ApiGetArg(uint32_t, sig)

    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  ಯದಿ 0
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

ಶೂನ್ಯ m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ಪ್ರತಿ (size_t i = 0; i < M3_ಎಣಿಕೆ_OF(namespaces); i++) {
        ಧ್ರುವ ವರ್ಣ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_args_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_args_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_time_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_set_flags)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_set_rights)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_set_size)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_set_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_path_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_path_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_path_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_path_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_path_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_path_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_path_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_path_unlink_file)));

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

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಶೂನ್ಯ m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ಪ್ರತ್ಯಾಗಮ _linkWASI(module, wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    ಯದಿ (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    ಪ್ರತ್ಯಾಗಮ _linkWASI(module, wasi_context);
}

#endif // d_m3HasMetaWASI
