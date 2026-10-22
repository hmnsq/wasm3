//
//  m3_api_meta_wasi.c
//
//  Created by Volodymyr Shymanskyy on 01/08/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_api_wasi.h"

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"

#ಆದರೆ defined(d_m3HasMetaWASI)

// NOTE: MetaWASI mostly redirects WASI calls to the host WASI environment

#  ಆದರೆ !defined(__wasi__)
#    error "MetaWASI is only supported on WASI target"
#  endif

#  ಆದರೆ __has_include("wasi/api.h")
#    include <wasi/api.h>
#    define USE_NEW_WASI
#    define WASI_STAT_ಕ್ಷೇತ್ರ(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_ಕ್ಷೇತ್ರ(f) st_##f

#  ಇಲ್ಲದಿದ್ದರೆ
#    error "Missing WASI headers"
#  endif

ಸ್ಥಿರ m3_wasi_context_t* wasi_context;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ size_t __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3_Wasi_ಮಾರ್ಗ_Max   PATH_MAX
#  ಇಲ್ಲದಿದ್ದರೆ
#    define d_m3_Wasi_ಮಾರ್ಗ_Max   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3_Wasi_ಮಾರ್ಗ(NAME, PTR, LEN)                                   \
      ಅಕ್ಷರ NAME [d_m3_Wasi_ಮಾರ್ಗ_Max];                                       \
      ಆದರೆ ((LEN) >= ಗಾತ್ರ (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      ಸ್ಮರಣೆ_ನಕಲು (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* copy_iov_ಗಮ್ಯ_host (IM3Runtime runtime, ಶೂನ್ಯ* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3_Api_ಸ್ಥಾನಾಂತರ_ಗಮ್ಯ_Ptr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  ಆದರೆ d_m3EnableWasiTracing

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* wasi_errno2str (__wasi_errno_t err)
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

ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* wasi_whence2str (__wasi_whence_t whence)
{
    ಆಯ್ಕೆ (whence) {
    ಸ್ಥಿತಿ __WASI_WHENCE_SET: ಹಿಂತಿರುಗಿ "SET";
    ಸ್ಥಿತಿ __WASI_WHENCE_CUR: ಹಿಂತಿರುಗಿ "CUR";
    ಸ್ಥಿತಿ __WASI_WHENCE_END: ಹಿಂತಿರುಗಿ "END";
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ಇಲ್ಲದಿದ್ದರೆ
#    define WASI_TRACE(fmt, ...)
#  endif

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
        m3ApiReturn(__WASI_ERRNO_INVAL);
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

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_ವಾದಗಳು_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_ಗಾತ್ರ)

    m3ApiCheckMem(argc, ಗಾತ್ರ(__wasi_size_t));
    m3ApiCheckMem(argv_buf_ಗಾತ್ರ, ಗಾತ್ರ(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಆದರೆ (context == ಖಾಲಿ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    ಗಾಗಿ (u32 i = 0; i < context->argc; ++i) {
        buf_len += ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_ಗಾತ್ರ, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ಅಕ್ಷರ*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_ಎಣಿಕೆ, env_buf_ಗಾತ್ರ;

    ret = __wasi_environ_sizes_get(&env_ಎಣಿಕೆ, &env_buf_ಗಾತ್ರ);
    ಆದರೆ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ಎಣಿಕೆ * ಗಾತ್ರ(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_ಗಾತ್ರ);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    ಆದರೆ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    ಗಾಗಿ (u32 i = 0; i < env_ಎಣಿಕೆ; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_ಗಮ್ಯ_ಸ್ಥಾನಾಂತರ(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_ಎಣಿಕೆ)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_ಗಾತ್ರ)

    m3ApiCheckMem(env_ಎಣಿಕೆ, ಗಾತ್ರ(__wasi_size_t));
    m3ApiCheckMem(env_buf_ಗಾತ್ರ, ಗಾತ್ರ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_ಎಣಿಕೆ, env_buf_ಗಾತ್ರ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    __wasi_errno_t ret = __wasi_fd_prestat_dir_name(fd, (uint8_t*)ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, ಮಾರ್ಗ_len, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_prestat_t*, buf)

    m3ApiCheckMem(buf, ಗಾತ್ರ(__wasi_prestat_t));

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

    ಆದರೆ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(fdstat, 0, 24);
    m3ApiWriteMem8(fdstat + 0, stat.fs_filetype);
    m3ApiWriteMem16(fdstat + 2, stat.fs_flags);
    m3ApiWriteMem64(fdstat + 8, stat.fs_rights_ತಳ);
    m3ApiWriteMem64(fdstat + 16, stat.fs_rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ಗಣ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    __wasi_errno_t ret = __wasi_fd_fdstat_set_flags(fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ಗಣ_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_rights_t, rights_ತಳ)
    m3ApiGetArg(__wasi_rights_t, rights_inheriting)

#  ಆದರೆ 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_ತಳ, rights_inheriting);
#  ಇಲ್ಲದಿದ್ದರೆ
    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  endif

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_ತಳ, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ಗಣ_ಗಾತ್ರ)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, ಗಾತ್ರ_2)

    __wasi_errno_t ret = __wasi_fd_filestat_set_size(fd, ಗಾತ್ರ_2);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, ಗಾತ್ರ_2);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ಗಣ_times)
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

    WASI_TRACE("fd:%d | fs.size:%" PRIu64, fd, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ಗಾತ್ರ_2));

    ಆದರೆ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_ಕ್ಷೇತ್ರ(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_ಕ್ಷೇತ್ರ(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_ಕ್ಷೇತ್ರ(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ಗಾತ್ರ_2));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_ಕ್ಷೇತ್ರ(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_ಕ್ಷೇತ್ರ(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ctim));

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, ಫಲಿತಾಂಶ)

    m3ApiCheckMem(ಫಲಿತಾಂಶ, ಗಾತ್ರ(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    ಆಯ್ಕೆ (wasi_whence) {
    ಸ್ಥಿತಿ 0: whence = __WASI_WHENCE_SET; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ 1: whence = __WASI_WHENCE_CUR; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ 2: whence = __WASI_WHENCE_END; ನಿಲ್ಲಿಸು;
    }

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_seek(fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    *ಫಲಿತಾಂಶ = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, from)
    m3ApiGetArg(__wasi_fd_t, ಗಮ್ಯ)

    __wasi_errno_t ret = __wasi_fd_renumber(from, ಗಮ್ಯ);

    WASI_TRACE("from:%d, to:%d", from, ಗಮ್ಯ);

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
    m3ApiGetArgMem(__wasi_filesize_t*, ಫಲಿತಾಂಶ)

    m3ApiCheckMem(ಫಲಿತಾಂಶ, ಗಾತ್ರ(__wasi_filesize_t));

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_tell(fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    *ಫಲಿತಾಂಶ = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    d_m3_Wasi_ಮಾರ್ಗ(p, ಮಾರ್ಗ, ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ಪೂರ್ಣಾಂಕ)ಮಾರ್ಗ_len, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, ಮಾರ್ಗ_len)
    m3ApiGetArgMem(ಅಕ್ಷರ*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಗಾತ್ರ(__wasi_size_t));

    d_m3_Wasi_ಮಾರ್ಗ(p, ಮಾರ್ಗ, ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (ಪೂರ್ಣಾಂಕ)ಮಾರ್ಗ_len, ಮಾರ್ಗ, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    d_m3_Wasi_ಮಾರ್ಗ(p, ಮಾರ್ಗ, ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ಪೂರ್ಣಾಂಕ)ಮಾರ್ಗ_len, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, old_ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, old_ಮಾರ್ಗ_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, new_ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, new_ಮಾರ್ಗ_len)

    m3ApiCheckMem(old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len);
    m3ApiCheckMem(new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len);

    d_m3_Wasi_ಮಾರ್ಗ(op, old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len)
    d_m3_Wasi_ಮಾರ್ಗ(np, new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (ಪೂರ್ಣಾಂಕ)old_ಮಾರ್ಗ_len, old_ಮಾರ್ಗ, new_fd, (ಪೂರ್ಣಾಂಕ)new_ಮಾರ್ಗ_len, new_ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, old_ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, old_ಮಾರ್ಗ_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, new_ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, new_ಮಾರ್ಗ_len)

    m3ApiCheckMem(old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len);
    m3ApiCheckMem(new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len);

    d_m3_Wasi_ಮಾರ್ಗ(op, old_ಮಾರ್ಗ, old_ಮಾರ್ಗ_len)
    d_m3_Wasi_ಮಾರ್ಗ(np, new_ಮಾರ್ಗ, new_ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (ಪೂರ್ಣಾಂಕ)old_ಮಾರ್ಗ_len, old_ಮಾರ್ಗ, fd, (ಪೂರ್ಣಾಂಕ)new_ಮಾರ್ಗ_len, new_ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, ಮಾರ್ಗ_len)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);

    d_m3_Wasi_ಮಾರ್ಗ(p, ಮಾರ್ಗ, ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (ಪೂರ್ಣಾಂಕ)ಮಾರ್ಗ_len, ಮಾರ್ಗ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(__wasi_size_t, ಮಾರ್ಗ_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_ತಳ)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);
    m3ApiCheckMem(fd, ಗಾತ್ರ(__wasi_fd_t));

    d_m3_Wasi_ಮಾರ್ಗ(p, ಮಾರ್ಗ, ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_open(dirfd,
                                          dirflags,
                                          p,
                                          oflags,
                                          fs_rights_ತಳ,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          fd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%.*s, oflags:0x%x, fs_flags:0x%x | fd:%d",
               dirfd, dirflags, (ಪೂರ್ಣಾಂಕ)ಮಾರ್ಗ_len, ಮಾರ್ಗ, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_ಮಾರ್ಗ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಮಾರ್ಗ)
    m3ApiGetArg(uint32_t, ಮಾರ್ಗ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(ಮಾರ್ಗ, ಮಾರ್ಗ_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3_Wasi_ಮಾರ್ಗ(p, ಮಾರ್ಗ, ಮಾರ್ಗ_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (ಪೂರ್ಣಾಂಕ)ಮಾರ್ಗ_len, ಮಾರ್ಗ, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ಗಾತ್ರ_2));

    ಆದರೆ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_ಕ್ಷೇತ್ರ(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_ಕ್ಷೇತ್ರ(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_ಕ್ಷೇತ್ರ(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ಗಾತ್ರ_2));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_ಕ್ಷೇತ್ರ(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_ಕ್ಷೇತ್ರ(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_ಕ್ಷೇತ್ರ(ctim));

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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(__wasi_iovec_t));
    m3ApiCheckMem(nread, ಗಾತ್ರ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*    mem_check = copy_iov_ಗಮ್ಯ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಆದರೆ (mem_check != m3Err_none) {
        ಹಿಂತಿರುಗಿ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(__wasi_iovec_t));
    m3ApiCheckMem(nread, ಗಾತ್ರ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*    mem_check = copy_iov_ಗಮ್ಯ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಆದರೆ (mem_check != m3Err_none) {
        ಹಿಂತಿರುಗಿ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಗಾತ್ರ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*    mem_check = copy_iov_ಗಮ್ಯ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಆದರೆ (mem_check != m3Err_none) {
        ಹಿಂತಿರುಗಿ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಗಾತ್ರ(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಗಾತ್ರ(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*    mem_check = copy_iov_ಗಮ್ಯ_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ಆದರೆ (mem_check != m3Err_none) {
        ಹಿಂತಿರುಗಿ mem_check;
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
    m3ApiCheckMem(bufused, ಗಾತ್ರ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_fd_readdir(fd, buf, buf_len, cookie, bufused);

    WASI_TRACE("fd:%d | bufused:%d", fd, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, ಉದ್ದಳತೆ)
    m3ApiGetArg(__wasi_advice_t, advice)

    __wasi_errno_t ret = __wasi_fd_advise(fd, offset, ಉದ್ದಳತೆ, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, ಉದ್ದಳತೆ, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, ಉದ್ದಳತೆ)

    __wasi_errno_t ret = __wasi_fd_allocate(fd, offset, ಉದ್ದಳತೆ);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, ಉದ್ದಳತೆ);

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

    m3ApiCheckMem(resolution, ಗಾತ್ರ(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_res_get(wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *resolution = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_ಸಮಯ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, ಗಾತ್ರ(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *time = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಸ್ಥಿರಾಂಕ __wasi_subscription_t*, ಒಳ)
    m3ApiGetArgMem(__wasi_event_t*, ಹೊರಹೋಗು)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(ಒಳ, nsubscriptions * ಗಾತ್ರ(__wasi_subscription_t));
    m3ApiCheckMem(ಹೊರಹೋಗು, nsubscriptions * ಗಾತ್ರ(__wasi_event_t));
    m3ApiCheckMem(nevents, ಗಾತ್ರ(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(ಒಳ, ಹೊರಹೋಗು, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

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
    // proc_raise was dropped from preview1, so newer wasi-libc no longer has
    // __wasi_signal_t; the import signature is "i(i)" either way
    m3ApiGetArg(uint32_t, sig)

    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  ಆದರೆ 0
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

ಶೂನ್ಯ m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಫಲಿತಾಂಶ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ಗಾಗಿ (size_t i = 0; i < M3_ಎಣಿಕೆ_ದ(namespaces); i++) {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_ವಾದಗಳು_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_ವಾದಗಳು_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_ಸಮಯ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_ಗಣ_flags)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_ಗಣ_rights)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_ಗಣ_ಗಾತ್ರ)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_ಗಣ_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_ಮಾರ್ಗ_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_ಮಾರ್ಗ_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_ಮಾರ್ಗ_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_ಮಾರ್ಗ_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_ಮಾರ್ಗ_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_ಮಾರ್ಗ_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_ಮಾರ್ಗ_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_ಮಾರ್ಗ_unlink_file)));

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

M3_ಫಲಿತಾಂಶ m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಶೂನ್ಯ m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_ಫಲಿತಾಂಶ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ಹಿಂತಿರುಗಿ _linkWASI(module, wasi);
}

M3_ಫಲಿತಾಂಶ m3_LinkWASI (IM3Module module)
{
    ಆದರೆ (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    ಹಿಂತಿರುಗಿ _linkWASI(module, wasi_context);
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasMetaWASI
