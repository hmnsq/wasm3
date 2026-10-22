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
#    define WASI_STAT_FIELD(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_FIELD(f) st_##f

#  وگرنه
#    error "Missing WASI headers"
#  endif

ایستا m3_wasi_context_t* wasi_context;

تعریفنوع size_t __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3WasiPathMax   PATH_MAX
#  وگرنه
#    define d_m3WasiPathMax   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3WasiPath(NAME, PTR, LEN)                                   \
      نویسه NAME [d_m3WasiPathMax];                                       \
      اگر ((LEN) >= اندازه (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      memcpy (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

ایستا درونخطی
ثابت تهی* copy_iov_to_host (IM3Runtime runtime, تهی* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    برای (صحیح i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3ApiOffsetToPtr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  اگر d_m3EnableWasiTracing

ثابت نویسه* wasi_errno2str (__wasi_errno_t err)
{
    // clang-format off
    انتخاب (err) {
    حالت  0: بازگشت "ESUCCESS";
    حالت  1: بازگشت "E2BIG";
    حالت  2: بازگشت "EACCES";
    حالت  3: بازگشت "EADDRINUSE";
    حالت  4: بازگشت "EADDRNOTAVAIL";
    حالت  5: بازگشت "EAFNOSUPPORT";
    حالت  6: بازگشت "EAGAIN";
    حالت  7: بازگشت "EALREADY";
    حالت  8: بازگشت "EBADF";
    حالت  9: بازگشت "EBADMSG";
    حالت 10: بازگشت "EBUSY";
    حالت 11: بازگشت "ECANCELED";
    حالت 12: بازگشت "ECHILD";
    حالت 13: بازگشت "ECONNABORTED";
    حالت 14: بازگشت "ECONNREFUSED";
    حالت 15: بازگشت "ECONNRESET";
    حالت 16: بازگشت "EDEADLK";
    حالت 17: بازگشت "EDESTADDRREQ";
    حالت 18: بازگشت "EDOM";
    حالت 19: بازگشت "EDQUOT";
    حالت 20: بازگشت "EEXIST";
    حالت 21: بازگشت "EFAULT";
    حالت 22: بازگشت "EFBIG";
    حالت 23: بازگشت "EHOSTUNREACH";
    حالت 24: بازگشت "EIDRM";
    حالت 25: بازگشت "EILSEQ";
    حالت 26: بازگشت "EINPROGRESS";
    حالت 27: بازگشت "EINTR";
    حالت 28: بازگشت "EINVAL";
    حالت 29: بازگشت "EIO";
    حالت 30: بازگشت "EISCONN";
    حالت 31: بازگشت "EISDIR";
    حالت 32: بازگشت "ELOOP";
    حالت 33: بازگشت "EMFILE";
    حالت 34: بازگشت "EMLINK";
    حالت 35: بازگشت "EMSGSIZE";
    حالت 36: بازگشت "EMULTIHOP";
    حالت 37: بازگشت "ENAMETOOLONG";
    حالت 38: بازگشت "ENETDOWN";
    حالت 39: بازگشت "ENETRESET";
    حالت 40: بازگشت "ENETUNREACH";
    حالت 41: بازگشت "ENFILE";
    حالت 42: بازگشت "ENOBUFS";
    حالت 43: بازگشت "ENODEV";
    حالت 44: بازگشت "ENOENT";
    حالت 45: بازگشت "ENOEXEC";
    حالت 46: بازگشت "ENOLCK";
    حالت 47: بازگشت "ENOLINK";
    حالت 48: بازگشت "ENOMEM";
    حالت 49: بازگشت "ENOMSG";
    حالت 50: بازگشت "ENOPROTOOPT";
    حالت 51: بازگشت "ENOSPC";
    حالت 52: بازگشت "ENOSYS";
    حالت 53: بازگشت "ENOTCONN";
    حالت 54: بازگشت "ENOTDIR";
    حالت 55: بازگشت "ENOTEMPTY";
    حالت 56: بازگشت "ENOTRECOVERABLE";
    حالت 57: بازگشت "ENOTSOCK";
    حالت 58: بازگشت "ENOTSUP";
    حالت 59: بازگشت "ENOTTY";
    حالت 60: بازگشت "ENXIO";
    حالت 61: بازگشت "EOVERFLOW";
    حالت 62: بازگشت "EOWNERDEAD";
    حالت 63: بازگشت "EPERM";
    حالت 64: بازگشت "EPIPE";
    حالت 65: بازگشت "EPROTO";
    حالت 66: بازگشت "EPROTONOSUPPORT";
    حالت 67: بازگشت "EPROTOTYPE";
    حالت 68: بازگشت "ERANGE";
    حالت 69: بازگشت "EROFS";
    حالت 70: بازگشت "ESPIPE";
    حالت 71: بازگشت "ESRCH";
    حالت 72: بازگشت "ESTALE";
    حالت 73: بازگشت "ETIMEDOUT";
    حالت 74: بازگشت "ETXTBSY";
    حالت 75: بازگشت "EXDEV";
    حالت 76: بازگشت "ENOTCAPABLE";
    پیشفرض: بازگشت "<unknown>";
    }
    // clang-format on
}

ثابت نویسه* wasi_whence2str (__wasi_whence_t whence)
{
    انتخاب (whence) {
    حالت __WASI_WHENCE_SET: بازگشت "SET";
    حالت __WASI_WHENCE_CUR: بازگشت "CUR";
    حالت __WASI_WHENCE_END: بازگشت "END";
    پیشفرض: بازگشت "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  وگرنه
#    define WASI_TRACE(fmt, ...)
#  endif

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_args_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(نویسه*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    اگر (context == پوچ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * اندازه(uint32_t));

    برای (u32 i = 0; i < context->argc; ++i) {
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

    m3ApiCheckMem(argc, اندازه(__wasi_size_t));
    m3ApiCheckMem(argv_buf_size, اندازه(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    اگر (context == پوچ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    برای (u32 i = 0; i < context->argc; ++i) {
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
    m3ApiGetArgMem(نویسه*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_شمار, env_buf_size;

    ret = __wasi_environ_sizes_get(&env_شمار, &env_buf_size);
    اگر (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_شمار * اندازه(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_size);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    اگر (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    برای (u32 i = 0; i < env_شمار; ++i) {
        m3ApiWriteMem32(&env[i], m3ApiPtrToOffset(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_شمار)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_شمار, اندازه(__wasi_size_t));
    m3ApiCheckMem(env_buf_size, اندازه(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_شمار, env_buf_size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(نویسه*, path)
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

    m3ApiCheckMem(buf, اندازه(__wasi_prestat_t));

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

#  اگر 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_base, rights_inheriting);
#  وگرنه
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

    اگر (ret != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(result, اندازه(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    انتخاب (wasi_whence) {
    حالت 0: whence = __WASI_WHENCE_SET; بشکن;
    حالت 1: whence = __WASI_WHENCE_CUR; بشکن;
    حالت 2: whence = __WASI_WHENCE_END; بشکن;
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

    m3ApiCheckMem(result, اندازه(__wasi_filesize_t));

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
    m3ApiGetArgMem(ثابت نویسه*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (صحیح)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت نویسه*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArgMem(نویسه*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, اندازه(__wasi_size_t));

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (صحیح)path_len, path, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت نویسه*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (صحیح)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(ثابت نویسه*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(ثابت نویسه*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (صحیح)old_path_len, old_path, new_fd, (صحیح)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت نویسه*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت نویسه*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (صحیح)old_path_len, old_path, fd, (صحیح)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ثابت نویسه*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (صحیح)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ثابت نویسه*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_base)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, اندازه(__wasi_fd_t));

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
               dirfd, dirflags, (صحیح)path_len, path, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(ثابت نویسه*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (صحیح)path_len, path, stat.WASI_STAT_FIELD(size));

    اگر (ret != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * اندازه(__wasi_iovec_t));
    m3ApiCheckMem(nread, اندازه(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت تهی*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        بازگشت mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * اندازه(__wasi_iovec_t));
    m3ApiCheckMem(nread, اندازه(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت تهی*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        بازگشت mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * اندازه(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, اندازه(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت تهی*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        بازگشت mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * اندازه(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, اندازه(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    ثابت تهی*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    اگر (mem_check != m3Err_none) {
        بازگشت mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pwrite(fd, (__wasi_ciovec_t*)iovs, iovs_len, offset, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(تهی*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArg(__wasi_dircookie_t, cookie)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, اندازه(__wasi_size_t));

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

    m3ApiCheckMem(resolution, اندازه(__wasi_timestamp_t));

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

    m3ApiCheckMem(time, اندازه(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *time = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت __wasi_subscription_t*, in)
    m3ApiGetArgMem(__wasi_event_t*, out)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(in, nsubscriptions * اندازه(__wasi_subscription_t));
    m3ApiCheckMem(out, nsubscriptions * اندازه(__wasi_event_t));
    m3ApiCheckMem(nevents, اندازه(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(in, out, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, code)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    اگر (context) {
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


ایستا
M3Result SuppressLookupFailure (M3Result i_result)
{
    اگر (i_result == m3Err_functionLookupFailed) {
        بازگشت m3Err_none;
    } وگرنه {
        بازگشت i_result;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    بازگشت wasi_context;
}

تهی m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ایستا درونخطی
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    ایستا ثابت نویسه* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    برای (size_t i = 0; i < M3_شمار_OF(namespaces); i++) {
        ثابت نویسه* wasi = namespaces[i];

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
    اگر (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 memoryIndex;

        اگر (m3_FindExportedMemory(module, "memory", &memoryIndex)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_BindImportMemory(module, "wasi_snapshot_preview1", memoryIndex));
    }

_catch:
    بازگشت result;
}

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    بازگشت m3Err_none;
}

تهی m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    بازگشت _linkWASI(module, wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    اگر (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    بازگشت _linkWASI(module, wasi_context);
}

#endif // d_m3HasMetaWASI
