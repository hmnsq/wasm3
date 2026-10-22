//
//  m3_api_meta_wasi.c
//
//  Created by Volodymyr Shymanskyy on 01/08/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#അന്തര്ഭാവ "m3_api_wasi.h"

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_exception.h"

#എങ്കിൽ defined(d_m3HasMetaWASI)

// NOTE: MetaWASI mostly redirects WASI calls to the host WASI environment

#  എങ്കിൽ !defined(__wasi__)
#    error "MetaWASI is only supported on WASI target"
#  endif

#  എങ്കിൽ __has_include("wasi/api.h")
#    include <wasi/api.h>
#    define USE_NEW_WASI
#    define WASI_STAT_FIELD(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_FIELD(f) st_##f

#  അല്ലെങ്കിൽ
#    error "Missing WASI headers"
#  endif

സ്ഥിതം m3_wasi_context_t* wasi_context;

തരനിർവചനം size_t __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3WasiPathMax   PATH_MAX
#  അല്ലെങ്കിൽ
#    define d_m3WasiPathMax   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3WasiPath(NAME, PTR, LEN)                                   \
      അക്ഷരം NAME [d_m3WasiPathMax];                                       \
      എങ്കിൽ ((LEN) >= വലുപ്പം (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      memcpy (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

സ്ഥിതം വരിയിൽ
സ്ഥിരം ശൂന്യം* copy_iov_to_host (IM3Runtime runtime, ശൂന്യം* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3ApiOffsetToPtr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  എങ്കിൽ d_m3EnableWasiTracing

സ്ഥിരം അക്ഷരം* wasi_errno2str (__wasi_errno_t err)
{
    // clang-format off
    തിരഞ്ഞെടുപ്പ് (err) {
    അവസ്ഥ  0: തിരികെ "ESUCCESS";
    അവസ്ഥ  1: തിരികെ "E2BIG";
    അവസ്ഥ  2: തിരികെ "EACCES";
    അവസ്ഥ  3: തിരികെ "EADDRINUSE";
    അവസ്ഥ  4: തിരികെ "EADDRNOTAVAIL";
    അവസ്ഥ  5: തിരികെ "EAFNOSUPPORT";
    അവസ്ഥ  6: തിരികെ "EAGAIN";
    അവസ്ഥ  7: തിരികെ "EALREADY";
    അവസ്ഥ  8: തിരികെ "EBADF";
    അവസ്ഥ  9: തിരികെ "EBADMSG";
    അവസ്ഥ 10: തിരികെ "EBUSY";
    അവസ്ഥ 11: തിരികെ "ECANCELED";
    അവസ്ഥ 12: തിരികെ "ECHILD";
    അവസ്ഥ 13: തിരികെ "ECONNABORTED";
    അവസ്ഥ 14: തിരികെ "ECONNREFUSED";
    അവസ്ഥ 15: തിരികെ "ECONNRESET";
    അവസ്ഥ 16: തിരികെ "EDEADLK";
    അവസ്ഥ 17: തിരികെ "EDESTADDRREQ";
    അവസ്ഥ 18: തിരികെ "EDOM";
    അവസ്ഥ 19: തിരികെ "EDQUOT";
    അവസ്ഥ 20: തിരികെ "EEXIST";
    അവസ്ഥ 21: തിരികെ "EFAULT";
    അവസ്ഥ 22: തിരികെ "EFBIG";
    അവസ്ഥ 23: തിരികെ "EHOSTUNREACH";
    അവസ്ഥ 24: തിരികെ "EIDRM";
    അവസ്ഥ 25: തിരികെ "EILSEQ";
    അവസ്ഥ 26: തിരികെ "EINPROGRESS";
    അവസ്ഥ 27: തിരികെ "EINTR";
    അവസ്ഥ 28: തിരികെ "EINVAL";
    അവസ്ഥ 29: തിരികെ "EIO";
    അവസ്ഥ 30: തിരികെ "EISCONN";
    അവസ്ഥ 31: തിരികെ "EISDIR";
    അവസ്ഥ 32: തിരികെ "ELOOP";
    അവസ്ഥ 33: തിരികെ "EMFILE";
    അവസ്ഥ 34: തിരികെ "EMLINK";
    അവസ്ഥ 35: തിരികെ "EMSGSIZE";
    അവസ്ഥ 36: തിരികെ "EMULTIHOP";
    അവസ്ഥ 37: തിരികെ "ENAMETOOLONG";
    അവസ്ഥ 38: തിരികെ "ENETDOWN";
    അവസ്ഥ 39: തിരികെ "ENETRESET";
    അവസ്ഥ 40: തിരികെ "ENETUNREACH";
    അവസ്ഥ 41: തിരികെ "ENFILE";
    അവസ്ഥ 42: തിരികെ "ENOBUFS";
    അവസ്ഥ 43: തിരികെ "ENODEV";
    അവസ്ഥ 44: തിരികെ "ENOENT";
    അവസ്ഥ 45: തിരികെ "ENOEXEC";
    അവസ്ഥ 46: തിരികെ "ENOLCK";
    അവസ്ഥ 47: തിരികെ "ENOLINK";
    അവസ്ഥ 48: തിരികെ "ENOMEM";
    അവസ്ഥ 49: തിരികെ "ENOMSG";
    അവസ്ഥ 50: തിരികെ "ENOPROTOOPT";
    അവസ്ഥ 51: തിരികെ "ENOSPC";
    അവസ്ഥ 52: തിരികെ "ENOSYS";
    അവസ്ഥ 53: തിരികെ "ENOTCONN";
    അവസ്ഥ 54: തിരികെ "ENOTDIR";
    അവസ്ഥ 55: തിരികെ "ENOTEMPTY";
    അവസ്ഥ 56: തിരികെ "ENOTRECOVERABLE";
    അവസ്ഥ 57: തിരികെ "ENOTSOCK";
    അവസ്ഥ 58: തിരികെ "ENOTSUP";
    അവസ്ഥ 59: തിരികെ "ENOTTY";
    അവസ്ഥ 60: തിരികെ "ENXIO";
    അവസ്ഥ 61: തിരികെ "EOVERFLOW";
    അവസ്ഥ 62: തിരികെ "EOWNERDEAD";
    അവസ്ഥ 63: തിരികെ "EPERM";
    അവസ്ഥ 64: തിരികെ "EPIPE";
    അവസ്ഥ 65: തിരികെ "EPROTO";
    അവസ്ഥ 66: തിരികെ "EPROTONOSUPPORT";
    അവസ്ഥ 67: തിരികെ "EPROTOTYPE";
    അവസ്ഥ 68: തിരികെ "ERANGE";
    അവസ്ഥ 69: തിരികെ "EROFS";
    അവസ്ഥ 70: തിരികെ "ESPIPE";
    അവസ്ഥ 71: തിരികെ "ESRCH";
    അവസ്ഥ 72: തിരികെ "ESTALE";
    അവസ്ഥ 73: തിരികെ "ETIMEDOUT";
    അവസ്ഥ 74: തിരികെ "ETXTBSY";
    അവസ്ഥ 75: തിരികെ "EXDEV";
    അവസ്ഥ 76: തിരികെ "ENOTCAPABLE";
    സ്ഥിരസ്ഥിതി: തിരികെ "<unknown>";
    }
    // clang-format on
}

സ്ഥിരം അക്ഷരം* wasi_whence2str (__wasi_whence_t whence)
{
    തിരഞ്ഞെടുപ്പ് (whence) {
    അവസ്ഥ __WASI_WHENCE_SET: തിരികെ "SET";
    അവസ്ഥ __WASI_WHENCE_CUR: തിരികെ "CUR";
    അവസ്ഥ __WASI_WHENCE_END: തിരികെ "END";
    സ്ഥിരസ്ഥിതി: തിരികെ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  അല്ലെങ്കിൽ
#    define WASI_TRACE(fmt, ...)
#  endif

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_args_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(അക്ഷരം*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    എങ്കിൽ (context == ശൂന്യ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * വലുപ്പം(uint32_t));

    വേണ്ടി (u32 i = 0; i < context->argc; ++i) {
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

    m3ApiCheckMem(argc, വലുപ്പം(__wasi_size_t));
    m3ApiCheckMem(argv_buf_size, വലുപ്പം(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    എങ്കിൽ (context == ശൂന്യ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    വേണ്ടി (u32 i = 0; i < context->argc; ++i) {
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
    m3ApiGetArgMem(അക്ഷരം*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_എണ്ണം, env_buf_size;

    ret = __wasi_environ_sizes_get(&env_എണ്ണം, &env_buf_size);
    എങ്കിൽ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_എണ്ണം * വലുപ്പം(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_size);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    എങ്കിൽ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    വേണ്ടി (u32 i = 0; i < env_എണ്ണം; ++i) {
        m3ApiWriteMem32(&env[i], m3ApiPtrToOffset(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_എണ്ണം)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_എണ്ണം, വലുപ്പം(__wasi_size_t));
    m3ApiCheckMem(env_buf_size, വലുപ്പം(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_എണ്ണം, env_buf_size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(അക്ഷരം*, path)
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

    m3ApiCheckMem(buf, വലുപ്പം(__wasi_prestat_t));

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

    എങ്കിൽ (ret != __WASI_ERRNO_SUCCESS) {
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

#  എങ്കിൽ 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_base, rights_inheriting);
#  അല്ലെങ്കിൽ
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

    എങ്കിൽ (ret != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(result, വലുപ്പം(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    തിരഞ്ഞെടുപ്പ് (wasi_whence) {
    അവസ്ഥ 0: whence = __WASI_WHENCE_SET; നിർത്തുക;
    അവസ്ഥ 1: whence = __WASI_WHENCE_CUR; നിർത്തുക;
    അവസ്ഥ 2: whence = __WASI_WHENCE_END; നിർത്തുക;
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

    m3ApiCheckMem(result, വലുപ്പം(__wasi_filesize_t));

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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (പൂർണ്ണസംഖ്യ)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArgMem(അക്ഷരം*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, വലുപ്പം(__wasi_size_t));

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (പൂർണ്ണസംഖ്യ)path_len, path, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (പൂർണ്ണസംഖ്യ)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (പൂർണ്ണസംഖ്യ)old_path_len, old_path, new_fd, (പൂർണ്ണസംഖ്യ)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (പൂർണ്ണസംഖ്യ)old_path_len, old_path, fd, (പൂർണ്ണസംഖ്യ)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (പൂർണ്ണസംഖ്യ)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_base)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, വലുപ്പം(__wasi_fd_t));

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
               dirfd, dirflags, (പൂർണ്ണസംഖ്യ)path_len, path, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (പൂർണ്ണസംഖ്യ)path_len, path, stat.WASI_STAT_FIELD(size));

    എങ്കിൽ (ret != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(__wasi_iovec_t));
    m3ApiCheckMem(nread, വലുപ്പം(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    സ്ഥിരം ശൂന്യം*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    എങ്കിൽ (mem_check != m3Err_none) {
        തിരികെ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(__wasi_iovec_t));
    m3ApiCheckMem(nread, വലുപ്പം(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    സ്ഥിരം ശൂന്യം*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    എങ്കിൽ (mem_check != m3Err_none) {
        തിരികെ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, വലുപ്പം(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    സ്ഥിരം ശൂന്യം*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    എങ്കിൽ (mem_check != m3Err_none) {
        തിരികെ mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, വലുപ്പം(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    സ്ഥിരം ശൂന്യം*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    എങ്കിൽ (mem_check != m3Err_none) {
        തിരികെ mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pwrite(fd, (__wasi_ciovec_t*)iovs, iovs_len, offset, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ശൂന്യം*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArg(__wasi_dircookie_t, cookie)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, വലുപ്പം(__wasi_size_t));

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

    m3ApiCheckMem(resolution, വലുപ്പം(__wasi_timestamp_t));

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

    m3ApiCheckMem(time, വലുപ്പം(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *time = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(സ്ഥിരം __wasi_subscription_t*, in)
    m3ApiGetArgMem(__wasi_event_t*, out)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(in, nsubscriptions * വലുപ്പം(__wasi_subscription_t));
    m3ApiCheckMem(out, nsubscriptions * വലുപ്പം(__wasi_event_t));
    m3ApiCheckMem(nevents, വലുപ്പം(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(in, out, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, code)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    എങ്കിൽ (context) {
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
#  എങ്കിൽ 0
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


സ്ഥിതം
M3Result SuppressLookupFailure (M3Result i_result)
{
    എങ്കിൽ (i_result == m3Err_functionLookupFailed) {
        തിരികെ m3Err_none;
    } അല്ലെങ്കിൽ {
        തിരികെ i_result;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    തിരികെ wasi_context;
}

ശൂന്യം m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

സ്ഥിതം വരിയിൽ
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    സ്ഥിതം സ്ഥിരം അക്ഷരം* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    വേണ്ടി (size_t i = 0; i < M3_എണ്ണം_OF(namespaces); i++) {
        സ്ഥിരം അക്ഷരം* wasi = namespaces[i];

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
    എങ്കിൽ (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 memoryIndex;

        എങ്കിൽ (m3_FindExportedMemory(module, "memory", &memoryIndex)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_BindImportMemory(module, "wasi_snapshot_preview1", memoryIndex));
    }

_catch:
    തിരികെ result;
}

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    തിരികെ m3Err_none;
}

ശൂന്യം m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    തിരികെ _linkWASI(module, wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    എങ്കിൽ (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    തിരികെ _linkWASI(module, wasi_context);
}

#endif // d_m3HasMetaWASI
