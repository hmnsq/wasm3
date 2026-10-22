//
//  m3_api_meta_wasi.c
//
//  Created by Volodymyr Shymanskyy on 01/08/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#समावेश "m3_api_wasi.h"

#समावेश "m3_env.h"
#समावेश "m3_exception.h"

#अगर defined(d_m3HasMetaWASI)

// NOTE: MetaWASI mostly redirects WASI calls to the host WASI environment

#  अगर !defined(__wasi__)
#    error "MetaWASI is only supported on WASI target"
#  endif

#  अगर __has_include("wasi/api.h")
#    include <wasi/api.h>
#    define USE_NEW_WASI
#    define WASI_STAT_FIELD(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_FIELD(f) st_##f

#  अन्यथा
#    error "Missing WASI headers"
#  endif

अटल m3_wasi_context_t* wasi_context;

प्रकार माप_प्रकार __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3WasiPathMax   PATH_MAX
#  अन्यथा
#    define d_m3WasiPathMax   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3WasiPath(NAME, PTR, LEN)                                   \
      अक्षर NAME [d_m3WasiPathMax];                                       \
      अगर ((LEN) >= माप (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      स_नकल (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

अटल अंतरभूत
स्थिर व्योम* copy_iov_to_host (IM3Runtime runtime, व्योम* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    क्रम (पूर्णांक i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3ApiOffsetToPtr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  अगर d_m3EnableWasiTracing

स्थिर अक्षर* wasi_errno2str (__wasi_errno_t err)
{
    // clang-format off
    चयन (err) {
    हाल  0: वापस "ESUCCESS";
    हाल  1: वापस "E2BIG";
    हाल  2: वापस "EACCES";
    हाल  3: वापस "EADDRINUSE";
    हाल  4: वापस "EADDRNOTAVAIL";
    हाल  5: वापस "EAFNOSUPPORT";
    हाल  6: वापस "EAGAIN";
    हाल  7: वापस "EALREADY";
    हाल  8: वापस "EBADF";
    हाल  9: वापस "EBADMSG";
    हाल 10: वापस "EBUSY";
    हाल 11: वापस "ECANCELED";
    हाल 12: वापस "ECHILD";
    हाल 13: वापस "ECONNABORTED";
    हाल 14: वापस "ECONNREFUSED";
    हाल 15: वापस "ECONNRESET";
    हाल 16: वापस "EDEADLK";
    हाल 17: वापस "EDESTADDRREQ";
    हाल 18: वापस "EDOM";
    हाल 19: वापस "EDQUOT";
    हाल 20: वापस "EEXIST";
    हाल 21: वापस "EFAULT";
    हाल 22: वापस "EFBIG";
    हाल 23: वापस "EHOSTUNREACH";
    हाल 24: वापस "EIDRM";
    हाल 25: वापस "EILSEQ";
    हाल 26: वापस "EINPROGRESS";
    हाल 27: वापस "EINTR";
    हाल 28: वापस "EINVAL";
    हाल 29: वापस "EIO";
    हाल 30: वापस "EISCONN";
    हाल 31: वापस "EISDIR";
    हाल 32: वापस "ELOOP";
    हाल 33: वापस "EMFILE";
    हाल 34: वापस "EMLINK";
    हाल 35: वापस "EMSGSIZE";
    हाल 36: वापस "EMULTIHOP";
    हाल 37: वापस "ENAMETOOLONG";
    हाल 38: वापस "ENETDOWN";
    हाल 39: वापस "ENETRESET";
    हाल 40: वापस "ENETUNREACH";
    हाल 41: वापस "ENFILE";
    हाल 42: वापस "ENOBUFS";
    हाल 43: वापस "ENODEV";
    हाल 44: वापस "ENOENT";
    हाल 45: वापस "ENOEXEC";
    हाल 46: वापस "ENOLCK";
    हाल 47: वापस "ENOLINK";
    हाल 48: वापस "ENOMEM";
    हाल 49: वापस "ENOMSG";
    हाल 50: वापस "ENOPROTOOPT";
    हाल 51: वापस "ENOSPC";
    हाल 52: वापस "ENOSYS";
    हाल 53: वापस "ENOTCONN";
    हाल 54: वापस "ENOTDIR";
    हाल 55: वापस "ENOTEMPTY";
    हाल 56: वापस "ENOTRECOVERABLE";
    हाल 57: वापस "ENOTSOCK";
    हाल 58: वापस "ENOTSUP";
    हाल 59: वापस "ENOTTY";
    हाल 60: वापस "ENXIO";
    हाल 61: वापस "EOVERFLOW";
    हाल 62: वापस "EOWNERDEAD";
    हाल 63: वापस "EPERM";
    हाल 64: वापस "EPIPE";
    हाल 65: वापस "EPROTO";
    हाल 66: वापस "EPROTONOSUPPORT";
    हाल 67: वापस "EPROTOTYPE";
    हाल 68: वापस "ERANGE";
    हाल 69: वापस "EROFS";
    हाल 70: वापस "ESPIPE";
    हाल 71: वापस "ESRCH";
    हाल 72: वापस "ESTALE";
    हाल 73: वापस "ETIMEDOUT";
    हाल 74: वापस "ETXTBSY";
    हाल 75: वापस "EXDEV";
    हाल 76: वापस "ENOTCAPABLE";
    शेष: वापस "<unknown>";
    }
    // clang-format on
}

स्थिर अक्षर* wasi_whence2str (__wasi_whence_t whence)
{
    चयन (whence) {
    हाल __WASI_WHENCE_SET: वापस "SET";
    हाल __WASI_WHENCE_CUR: वापस "CUR";
    हाल __WASI_WHENCE_END: वापस "END";
    शेष: वापस "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { ख_लिखो(मानक_त्रुटि, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); ख_लिखो(मानक_त्रुटि, " => %s\n", wasi_errno2str(ret)); }
#  अन्यथा
#    define WASI_TRACE(fmt, ...)
#  endif

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_args_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(अक्षर*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    अगर (context == शून्य) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * माप(uint32_t));

    क्रम (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3ApiPtrToOffset(argv_buf));

        माप_प्रकार len = म_माप(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        स_नकल(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_args_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_आकार)

    m3ApiCheckMem(argc, माप(__wasi_size_t));
    m3ApiCheckMem(argv_buf_आकार, माप(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    अगर (context == शून्य) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    क्रम (u32 i = 0; i < context->argc; ++i) {
        buf_len += म_माप(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_आकार, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(अक्षर*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_गिनती, env_buf_आकार;

    ret = __wasi_environ_sizes_get(&env_गिनती, &env_buf_आकार);
    अगर (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_गिनती * माप(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_आकार);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    अगर (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    क्रम (u32 i = 0; i < env_गिनती; ++i) {
        m3ApiWriteMem32(&env[i], m3ApiPtrToOffset(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_गिनती)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_आकार)

    m3ApiCheckMem(env_गिनती, माप(__wasi_size_t));
    m3ApiCheckMem(env_buf_आकार, माप(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_गिनती, env_buf_आकार);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(अक्षर*, path)
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

    m3ApiCheckMem(buf, माप(__wasi_prestat_t));

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

    अगर (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    स_रखो(fdstat, 0, 24);
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

#  अगर 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_base, rights_inheriting);
#  अन्यथा
    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  endif

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_base, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_set_आकार)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, आकार)

    __wasi_errno_t ret = __wasi_fd_filestat_set_size(fd, आकार);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, आकार);

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

    WASI_TRACE("fd:%d | fs.size:%" PRIu64, fd, stat.WASI_STAT_FIELD(आकार));

    अगर (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    स_रखो(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_FIELD(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_FIELD(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_FIELD(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_FIELD(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_FIELD(आकार));
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
    m3ApiGetArgMem(__wasi_filesize_t*, परिणाम)

    m3ApiCheckMem(परिणाम, माप(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    चयन (wasi_whence) {
    हाल 0: whence = __WASI_WHENCE_SET; अवरोध;
    हाल 1: whence = __WASI_WHENCE_CUR; अवरोध;
    हाल 2: whence = __WASI_WHENCE_END; अवरोध;
    }

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_seek(fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    *परिणाम = pos;

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
    m3ApiGetArgMem(__wasi_filesize_t*, परिणाम)

    m3ApiCheckMem(परिणाम, माप(__wasi_filesize_t));

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_tell(fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    *परिणाम = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(स्थिर अक्षर*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (पूर्णांक)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(स्थिर अक्षर*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArgMem(अक्षर*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, माप(__wasi_size_t));

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (पूर्णांक)path_len, path, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(स्थिर अक्षर*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (पूर्णांक)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(स्थिर अक्षर*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(स्थिर अक्षर*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (पूर्णांक)old_path_len, old_path, new_fd, (पूर्णांक)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(स्थिर अक्षर*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(स्थिर अक्षर*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    d_m3WasiPath(op, old_path, old_path_len)
    d_m3WasiPath(np, new_path, new_path_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (पूर्णांक)old_path_len, old_path, fd, (पूर्णांक)new_path_len, new_path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(स्थिर अक्षर*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (पूर्णांक)path_len, path);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(स्थिर अक्षर*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_base)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, माप(__wasi_fd_t));

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
               dirfd, dirflags, (पूर्णांक)path_len, path, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_path_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(स्थिर अक्षर*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3WasiPath(p, path, path_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (पूर्णांक)path_len, path, stat.WASI_STAT_FIELD(आकार));

    अगर (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    स_रखो(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_FIELD(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_FIELD(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_FIELD(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_FIELD(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_FIELD(आकार));
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

    m3ApiCheckMem(wasi_iovs, iovs_len * माप(__wasi_iovec_t));
    m3ApiCheckMem(nread, माप(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    स्थिर व्योम*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    अगर (mem_check != m3Err_none) {
        वापस mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * माप(__wasi_iovec_t));
    m3ApiCheckMem(nread, माप(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    स्थिर व्योम*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    अगर (mem_check != m3Err_none) {
        वापस mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * माप(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, माप(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    स्थिर व्योम*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    अगर (mem_check != m3Err_none) {
        वापस mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * माप(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, माप(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    स्थिर व्योम*    mem_check = copy_iov_to_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    अगर (mem_check != m3Err_none) {
        वापस mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pwrite(fd, (__wasi_ciovec_t*)iovs, iovs_len, offset, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(व्योम*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArg(__wasi_dircookie_t, cookie)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, माप(__wasi_size_t));

    __wasi_errno_t ret = __wasi_fd_readdir(fd, buf, buf_len, cookie, bufused);

    WASI_TRACE("fd:%d | bufused:%d", fd, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, लंबाई)
    m3ApiGetArg(__wasi_advice_t, advice)

    __wasi_errno_t ret = __wasi_fd_advise(fd, offset, लंबाई, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, लंबाई, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, लंबाई)

    __wasi_errno_t ret = __wasi_fd_allocate(fd, offset, लंबाई);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, लंबाई);

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

    m3ApiCheckMem(resolution, माप(__wasi_timestamp_t));

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
    m3ApiGetArgMem(__wasi_timestamp_t*, समय)

    m3ApiCheckMem(समय, माप(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *समय = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(स्थिर __wasi_subscription_t*, in)
    m3ApiGetArgMem(__wasi_event_t*, out)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(in, nsubscriptions * माप(__wasi_subscription_t));
    m3ApiCheckMem(out, nsubscriptions * माप(__wasi_event_t));
    m3ApiCheckMem(nevents, माप(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(in, out, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, code)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    अगर (context) {
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
#  अगर 0
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


अटल
M3_परिणाम SuppressLookupFailure (M3_परिणाम i_परिणाम)
{
    अगर (i_परिणाम == m3Err_functionLookupFailed) {
        वापस m3Err_none;
    } अन्यथा {
        वापस i_परिणाम;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    वापस wasi_context;
}

व्योम m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

अटल अंतरभूत
M3_परिणाम _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_परिणाम परिणाम = m3Err_none;

    अटल स्थिर अक्षर* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    क्रम (माप_प्रकार i = 0; i < M3_गिनती_OF(namespaces); i++) {
        स्थिर अक्षर* wasi = namespaces[i];

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
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_set_आकार)));
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
    अगर (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 memory_सूचकांक;

        अगर (m3_FindExportedMemory(module, "memory", &memory_सूचकांक)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_BindImportMemory(module, "wasi_snapshot_preview1", memory_सूचकांक));
    }

_catch:
    वापस परिणाम;
}

M3_परिणाम m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    वापस m3Err_none;
}

व्योम m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_परिणाम m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    वापस _linkWASI(module, wasi);
}

M3_परिणाम m3_LinkWASI (IM3Module module)
{
    अगर (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    वापस _linkWASI(module, wasi_context);
}

#पूर्ण_अगर // d_m3HasMetaWASI
