//
//  m3_api_meta_wasi.c
//
//  Created by Volodymyr Shymanskyy on 01/08/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#شامل "m3_api_wasi.h"

#شامل "m3_env.h"
#شامل "m3_exception.h"

#اگر defined(d_m3HasMetaWASI)

// NOTE: MetaWASI mostly redirects WASI calls to the host WASI environment

#  اگر !defined(__wasi__)
#    error "MetaWASI is only supported on WASI target"
#  endif

#  اگر __has_include("wasi/api.h")
#    include <wasi/api.h>
#    define USE_NEW_WASI
#    define WASI_STAT_میدان(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_میدان(f) st_##f

#  ورنہ
#    error "Missing WASI headers"
#  endif

ساکن m3_wasi_context_t* wasi_context;

قسمتعریف size_t __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3_Wasi_راستہ_Max   PATH_MAX
#  ورنہ
#    define d_m3_Wasi_راستہ_Max   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3_Wasi_راستہ(NAME, PTR, LEN)                                   \
      حرف NAME [d_m3_Wasi_راستہ_Max];                                       \
      اگر ((LEN) >= حجم (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      حافظہ_نقل (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

ساکن اندرونی
ثابت خالی* copy_iov_مقصد_host (IM3Runtime runtime, خالی* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    برائے (صحیح i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3_Api_انحراف_مقصد_Ptr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  اگر d_m3EnableWasiTracing

ثابت حرف* wasi_errno2str (__wasi_errno_t err)
{
    // clang-format off
    انتخاب (err) {
    حالت  0: واپس "ESUCCESS";
    حالت  1: واپس "E2BIG";
    حالت  2: واپس "EACCES";
    حالت  3: واپس "EADDRINUSE";
    حالت  4: واپس "EADDRNOTAVAIL";
    حالت  5: واپس "EAFNOSUPPORT";
    حالت  6: واپس "EAGAIN";
    حالت  7: واپس "EALREADY";
    حالت  8: واپس "EBADF";
    حالت  9: واپس "EBADMSG";
    حالت 10: واپس "EBUSY";
    حالت 11: واپس "ECANCELED";
    حالت 12: واپس "ECHILD";
    حالت 13: واپس "ECONNABORTED";
    حالت 14: واپس "ECONNREFUSED";
    حالت 15: واپس "ECONNRESET";
    حالت 16: واپس "EDEADLK";
    حالت 17: واپس "EDESTADDRREQ";
    حالت 18: واپس "EDOM";
    حالت 19: واپس "EDQUOT";
    حالت 20: واپس "EEXIST";
    حالت 21: واپس "EFAULT";
    حالت 22: واپس "EFBIG";
    حالت 23: واپس "EHOSTUNREACH";
    حالت 24: واپس "EIDRM";
    حالت 25: واپس "EILSEQ";
    حالت 26: واپس "EINPROGRESS";
    حالت 27: واپس "EINTR";
    حالت 28: واپس "EINVAL";
    حالت 29: واپس "EIO";
    حالت 30: واپس "EISCONN";
    حالت 31: واپس "EISDIR";
    حالت 32: واپس "ELOOP";
    حالت 33: واپس "EMFILE";
    حالت 34: واپس "EMLINK";
    حالت 35: واپس "EMSGSIZE";
    حالت 36: واپس "EMULTIHOP";
    حالت 37: واپس "ENAMETOOLONG";
    حالت 38: واپس "ENETDOWN";
    حالت 39: واپس "ENETRESET";
    حالت 40: واپس "ENETUNREACH";
    حالت 41: واپس "ENFILE";
    حالت 42: واپس "ENOBUFS";
    حالت 43: واپس "ENODEV";
    حالت 44: واپس "ENOENT";
    حالت 45: واپس "ENOEXEC";
    حالت 46: واپس "ENOLCK";
    حالت 47: واپس "ENOLINK";
    حالت 48: واپس "ENOMEM";
    حالت 49: واپس "ENOMSG";
    حالت 50: واپس "ENOPROTOOPT";
    حالت 51: واپس "ENOSPC";
    حالت 52: واپس "ENOSYS";
    حالت 53: واپس "ENOTCONN";
    حالت 54: واپس "ENOTDIR";
    حالت 55: واپس "ENOTEMPTY";
    حالت 56: واپس "ENOTRECOVERABLE";
    حالت 57: واپس "ENOTSOCK";
    حالت 58: واپس "ENOTSUP";
    حالت 59: واپس "ENOTTY";
    حالت 60: واپس "ENXIO";
    حالت 61: واپس "EOVERFLOW";
    حالت 62: واپس "EOWNERDEAD";
    حالت 63: واپس "EPERM";
    حالت 64: واپس "EPIPE";
    حالت 65: واپس "EPROTO";
    حالت 66: واپس "EPROTONOSUPPORT";
    حالت 67: واپس "EPROTOTYPE";
    حالت 68: واپس "ERANGE";
    حالت 69: واپس "EROFS";
    حالت 70: واپس "ESPIPE";
    حالت 71: واپس "ESRCH";
    حالت 72: واپس "ESTALE";
    حالت 73: واپس "ETIMEDOUT";
    حالت 74: واپس "ETXTBSY";
    حالت 75: واپس "EXDEV";
    حالت 76: واپس "ENOTCAPABLE";
    طےشدہ: واپس "<unknown>";
    }
    // clang-format on
}

ثابت حرف* wasi_whence2str (__wasi_whence_t whence)
{
    انتخاب (whence) {
    حالت __WASI_WHENCE_SET: واپس "SET";
    حالت __WASI_WHENCE_CUR: واپس "CUR";
    حالت __WASI_WHENCE_END: واپس "END";
    طےشدہ: واپس "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ورنہ
#    define WASI_TRACE(fmt, ...)
#  endif

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_دلائل_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(حرف*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    اگر (context == تہی) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * حجم(uint32_t));

    برائے (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_مقصد_انحراف(argv_buf));

        size_t len = لڑی_لمبائی(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        حافظہ_نقل(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_دلائل_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_سائز)

    m3ApiCheckMem(argc, حجم(__wasi_size_t));
    m3ApiCheckMem(argv_buf_سائز, حجم(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    اگر (context == تہی) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    برائے (u32 i = 0; i < context->argc; ++i) {
        buf_len += لڑی_لمبائی(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_سائز, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(حرف*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_گنتی, env_buf_سائز;

    ret = __wasi_environ_sizes_get(&env_گنتی, &env_buf_سائز);
    اگر (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_گنتی * حجم(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_سائز);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    اگر (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    برائے (u32 i = 0; i < env_گنتی; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_مقصد_انحراف(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_گنتی)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_سائز)

    m3ApiCheckMem(env_گنتی, حجم(__wasi_size_t));
    m3ApiCheckMem(env_buf_سائز, حجم(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_گنتی, env_buf_سائز);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(حرف*, راستہ)
    m3ApiGetArg(__wasi_size_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    __wasi_errno_t ret = __wasi_fd_prestat_dir_name(fd, (uint8_t*)راستہ, راستہ_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, راستہ_len, راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_prestat_t*, buf)

    m3ApiCheckMem(buf, حجم(__wasi_prestat_t));

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

    اگر (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(fdstat, 0, 24);
    m3ApiWriteMem8(fdstat + 0, stat.fs_filetype);
    m3ApiWriteMem16(fdstat + 2, stat.fs_flags);
    m3ApiWriteMem64(fdstat + 8, stat.fs_rights_بنیاد);
    m3ApiWriteMem64(fdstat + 16, stat.fs_rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_سیٹ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    __wasi_errno_t ret = __wasi_fd_fdstat_set_flags(fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_سیٹ_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_rights_t, rights_بنیاد)
    m3ApiGetArg(__wasi_rights_t, rights_inheriting)

#  اگر 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_بنیاد, rights_inheriting);
#  ورنہ
    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  endif

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_بنیاد, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سیٹ_سائز)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, سائز)

    __wasi_errno_t ret = __wasi_fd_filestat_set_size(fd, سائز);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, سائز);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سیٹ_times)
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

    WASI_TRACE("fd:%d | fs.size:%" PRIu64, fd, stat.WASI_STAT_میدان(سائز));

    اگر (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_میدان(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_میدان(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_میدان(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_میدان(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_میدان(سائز));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_میدان(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_میدان(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_میدان(ctim));

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, نتیجہ)

    m3ApiCheckMem(نتیجہ, حجم(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    انتخاب (wasi_whence) {
    حالت 0: whence = __WASI_WHENCE_SET; توڑو;
    حالت 1: whence = __WASI_WHENCE_CUR; توڑو;
    حالت 2: whence = __WASI_WHENCE_END; توڑو;
    }

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_seek(fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    *نتیجہ = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, from)
    m3ApiGetArg(__wasi_fd_t, مقصد)

    __wasi_errno_t ret = __wasi_fd_renumber(from, مقصد);

    WASI_TRACE("from:%d, to:%d", from, مقصد);

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
    m3ApiGetArgMem(__wasi_filesize_t*, نتیجہ)

    m3ApiCheckMem(نتیجہ, حجم(__wasi_filesize_t));

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_tell(fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    *نتیجہ = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(__wasi_size_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    d_m3_Wasi_راستہ(p, راستہ, راستہ_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (صحیح)راستہ_len, راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(__wasi_size_t, راستہ_len)
    m3ApiGetArgMem(حرف*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(راستہ, راستہ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, حجم(__wasi_size_t));

    d_m3_Wasi_راستہ(p, راستہ, راستہ_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (صحیح)راستہ_len, راستہ, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(__wasi_size_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    d_m3_Wasi_راستہ(p, راستہ, راستہ_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (صحیح)راستہ_len, راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(ثابت حرف*, old_راستہ)
    m3ApiGetArg(__wasi_size_t, old_راستہ_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(ثابت حرف*, new_راستہ)
    m3ApiGetArg(__wasi_size_t, new_راستہ_len)

    m3ApiCheckMem(old_راستہ, old_راستہ_len);
    m3ApiCheckMem(new_راستہ, new_راستہ_len);

    d_m3_Wasi_راستہ(op, old_راستہ, old_راستہ_len)
    d_m3_Wasi_راستہ(np, new_راستہ, new_راستہ_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (صحیح)old_راستہ_len, old_راستہ, new_fd, (صحیح)new_راستہ_len, new_راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت حرف*, old_راستہ)
    m3ApiGetArg(__wasi_size_t, old_راستہ_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, new_راستہ)
    m3ApiGetArg(__wasi_size_t, new_راستہ_len)

    m3ApiCheckMem(old_راستہ, old_راستہ_len);
    m3ApiCheckMem(new_راستہ, new_راستہ_len);

    d_m3_Wasi_راستہ(op, old_راستہ, old_راستہ_len)
    d_m3_Wasi_راستہ(np, new_راستہ, new_راستہ_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (صحیح)old_راستہ_len, old_راستہ, fd, (صحیح)new_راستہ_len, new_راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(__wasi_size_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    d_m3_Wasi_راستہ(p, راستہ, راستہ_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (صحیح)راستہ_len, راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(__wasi_size_t, راستہ_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_بنیاد)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(راستہ, راستہ_len);
    m3ApiCheckMem(fd, حجم(__wasi_fd_t));

    d_m3_Wasi_راستہ(p, راستہ, راستہ_len)

    __wasi_errno_t ret = __wasi_path_open(dirfd,
                                          dirflags,
                                          p,
                                          oflags,
                                          fs_rights_بنیاد,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          fd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%.*s, oflags:0x%x, fs_flags:0x%x | fd:%d",
               dirfd, dirflags, (صحیح)راستہ_len, راستہ, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(uint32_t, راستہ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(راستہ, راستہ_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3_Wasi_راستہ(p, راستہ, راستہ_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (صحیح)راستہ_len, راستہ, stat.WASI_STAT_میدان(سائز));

    اگر (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_میدان(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_میدان(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_میدان(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_میدان(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_میدان(سائز));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_میدان(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_میدان(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_میدان(ctim));

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

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(__wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت خالی*    mem_check = copy_iov_مقصد_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        واپس mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(__wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت خالی*    mem_check = copy_iov_مقصد_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        واپس mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت خالی*    mem_check = copy_iov_مقصد_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        واپس mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت خالی*    mem_check = copy_iov_مقصد_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        واپس mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pwrite(fd, (__wasi_ciovec_t*)iovs, iovs_len, offset, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(خالی*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArg(__wasi_dircookie_t, cookie)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, حجم(__wasi_size_t));

    __wasi_errno_t ret = __wasi_fd_readdir(fd, buf, buf_len, cookie, bufused);

    WASI_TRACE("fd:%d | bufused:%d", fd, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, لمبائی)
    m3ApiGetArg(__wasi_advice_t, advice)

    __wasi_errno_t ret = __wasi_fd_advise(fd, offset, لمبائی, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, لمبائی, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, لمبائی)

    __wasi_errno_t ret = __wasi_fd_allocate(fd, offset, لمبائی);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, لمبائی);

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

    m3ApiCheckMem(resolution, حجم(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_res_get(wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *resolution = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_وقت_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, حجم(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *time = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت __wasi_subscription_t*, اندر)
    m3ApiGetArgMem(__wasi_event_t*, برآمد)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(اندر, nsubscriptions * حجم(__wasi_subscription_t));
    m3ApiCheckMem(برآمد, nsubscriptions * حجم(__wasi_event_t));
    m3ApiCheckMem(nevents, حجم(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(اندر, برآمد, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, رمز)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    اگر (context) {
        context->exit_رمز = رمز;
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
#  اگر 0
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


ساکن
M3_نتیجہ SuppressLookupFailure (M3_نتیجہ i_نتیجہ)
{
    اگر (i_نتیجہ == m3Err_functionLookupFailed) {
        واپس m3Err_none;
    } ورنہ {
        واپس i_نتیجہ;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    واپس wasi_context;
}

خالی m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ساکن اندرونی
M3_نتیجہ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    ساکن ثابت حرف* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    برائے (size_t i = 0; i < M3_گنتی_کا(namespaces); i++) {
        ثابت حرف* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_دلائل_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_دلائل_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_وقت_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_سیٹ_flags)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_سیٹ_rights)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_سیٹ_سائز)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_سیٹ_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_راستہ_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_راستہ_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_راستہ_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_راستہ_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_راستہ_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_راستہ_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_راستہ_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_راستہ_unlink_file)));

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
    اگر (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 حافظہ_اشاریہ;

        اگر (m3_ڈھونڈو_Exported_حافظہ(module, "memory", &حافظہ_اشاریہ)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_حافظہ(module, "wasi_snapshot_preview1", حافظہ_اشاریہ));
    }

_catch:
    واپس نتیجہ;
}

M3_نتیجہ m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    واپس m3Err_none;
}

خالی m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_نتیجہ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    واپس _linkWASI(module, wasi);
}

M3_نتیجہ m3_LinkWASI (IM3Module module)
{
    اگر (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    واپس _linkWASI(module, wasi_context);
}

#اگر_ختم // d_m3HasMetaWASI
