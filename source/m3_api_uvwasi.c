//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#അന്തര്ഭാവ "m3_api_wasi.h"

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_exception.h"

#എങ്കിൽ defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
ബാഹ്യ അക്ഷരം** environ;
#  endif

സ്ഥിതം m3_wasi_context_t* wasi_context = ശൂന്യ;

തരനിർവചനം ഘടന wasi_iovec_t {
    uvwasi_size_t buf;
    uvwasi_size_t buf_len;
} wasi_iovec_t;

#  എങ്കിൽ d_m3EnableWasiTracing

സ്ഥിരം അക്ഷരം* wasi_errno2str (uvwasi_errno_t err)
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

സ്ഥിരം അക്ഷരം* wasi_whence2str (uvwasi_whence_t whence)
{
    തിരഞ്ഞെടുപ്പ് (whence) {
    അവസ്ഥ UVWASI_WHENCE_SET: തിരികെ "SET";
    അവസ്ഥ UVWASI_WHENCE_CUR: തിരികെ "CUR";
    അവസ്ഥ UVWASI_WHENCE_END: തിരികെ "END";
    സ്ഥിരസ്ഥിതി: തിരികെ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  അല്ലെങ്കിൽ
#    define WASI_TRACE(fmt, ...)
#  endif

സ്ഥിതം വരിയിൽ
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    പ്രതിജ്ഞാ(context != ശൂന്യ);
    തിരികെ &context->uvwasi;
}

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
        m3ApiReturn(UVWASI_EINVAL);
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

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_args_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_size_t*, argc)
    m3ApiGetArgMem(uvwasi_size_t*, argv_buf_size)

    m3ApiCheckMem(argc, വലുപ്പം(uvwasi_size_t));
    m3ApiCheckMem(argv_buf_size, വലുപ്പം(uvwasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    എങ്കിൽ (context == ശൂന്യ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_size_t buf_len = 0;
    വേണ്ടി (u32 i = 0; i < context->argc; ++i) {
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
    m3ApiGetArgMem(അക്ഷരം*, env_buf)

    അക്ഷരം**         environment;
    uvwasi_errno_t ret;
    uvwasi_size_t  env_എണ്ണം, env_buf_size;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_എണ്ണം, &env_buf_size);
    എങ്കിൽ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_എണ്ണം * വലുപ്പം(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_size);

    environment = calloc(env_എണ്ണം, വലുപ്പം(അക്ഷരം*));
    എങ്കിൽ (environment == ശൂന്യ) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    എങ്കിൽ (ret != UVWASI_ESUCCESS) {
        സ്വതന്ത്രം(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_offset = m3ApiPtrToOffset(env_buf);

    വേണ്ടി (u32 i = 0; i < env_എണ്ണം; ++i) {
        uint32_t offset = environ_buf_offset +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    സ്വതന്ത്രം(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_size_t*, env_എണ്ണം)
    m3ApiGetArgMem(uvwasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_എണ്ണം, വലുപ്പം(uvwasi_size_t));
    m3ApiCheckMem(env_buf_size, വലുപ്പം(uvwasi_size_t));

    uvwasi_size_t എണ്ണം;
    uvwasi_size_t buf_size;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &എണ്ണം, &buf_size);

    m3ApiWriteMem32(env_എണ്ണം, എണ്ണം);
    m3ApiWriteMem32(env_buf_size, buf_size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(അക്ഷരം*, path)
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

    എങ്കിൽ (ret != UVWASI_ESUCCESS) {
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

    എങ്കിൽ (ret != UVWASI_ESUCCESS) {
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

    എങ്കിൽ (ret != UVWASI_ESUCCESS) {
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

    m3ApiCheckMem(result, വലുപ്പം(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    തിരഞ്ഞെടുപ്പ് (wasi_whence) {
    അവസ്ഥ 0: whence = UVWASI_WHENCE_SET; നിർത്തുക;
    അവസ്ഥ 1: whence = UVWASI_WHENCE_CUR; നിർത്തുക;
    അവസ്ഥ 2: whence = UVWASI_WHENCE_END; നിർത്തുക;
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

    m3ApiCheckMem(result, വലുപ്പം(uvwasi_filesize_t));

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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArgMem(അക്ഷരം*, buf)
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, വലുപ്പം(uvwasi_size_t));

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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, new_path)
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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, new_path)
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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_base)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, വലുപ്പം(uvwasi_fd_t));

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
    m3ApiGetArgMem(സ്ഥിരം അക്ഷരം*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_path_filestat_get(_get_uvwasi_p(_ctx), fd, flags, path, path_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, path, stat.st_size);

    എങ്കിൽ (ret != UVWASI_ESUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(wasi_iovec_t));
    m3ApiCheckMem(nread, വലുപ്പം(uvwasi_size_t));

#  എങ്കിൽ M3_HAS_VLA
    എങ്കിൽ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  അല്ലെങ്കിൽ
    എങ്കിൽ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    വേണ്ടി (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_size_t അക്കം_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &അക്കം_read);

    WASI_TRACE("fd:%d | nread:%d", fd, അക്കം_read);

    m3ApiWriteMem32(nread, അക്കം_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(wasi_iovec_t));
    m3ApiCheckMem(nread, വലുപ്പം(uvwasi_size_t));

#  എങ്കിൽ M3_HAS_VLA
    എങ്കിൽ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  അല്ലെങ്കിൽ
    എങ്കിൽ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_size_t  അക്കം_read;
    uvwasi_errno_t ret;

    വേണ്ടി (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &അക്കം_read);

    WASI_TRACE("fd:%d | nread:%d", fd, അക്കം_read);

    m3ApiWriteMem32(nread, അക്കം_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(wasi_iovec_t));
    m3ApiCheckMem(nwritten, വലുപ്പം(uvwasi_size_t));

#  എങ്കിൽ M3_HAS_VLA
    എങ്കിൽ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  അല്ലെങ്കിൽ
    എങ്കിൽ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  അക്കം_written;
    uvwasi_errno_t ret;

    വേണ്ടി (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &അക്കം_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, അക്കം_written);

    m3ApiWriteMem32(nwritten, അക്കം_written);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * വലുപ്പം(wasi_iovec_t));
    m3ApiCheckMem(nwritten, വലുപ്പം(uvwasi_size_t));

#  എങ്കിൽ M3_HAS_VLA
    എങ്കിൽ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  അല്ലെങ്കിൽ
    എങ്കിൽ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  അക്കം_written;
    uvwasi_errno_t ret;

    വേണ്ടി (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &അക്കം_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, അക്കം_written);

    m3ApiWriteMem32(nwritten, അക്കം_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ശൂന്യം*, buf)
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, വലുപ്പം(uvwasi_size_t));

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

    m3ApiCheckMem(resolution, വലുപ്പം(uvwasi_timestamp_t));

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

    m3ApiCheckMem(time, വലുപ്പം(uvwasi_timestamp_t));

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
സ്ഥിതം
ശൂന്യം read_subscriptions (uvwasi_subscription_t* o_subs, സ്ഥിരം uint8_t* i_in,
                         uvwasi_size_t i_എണ്ണം)
{
    വേണ്ടി (uvwasi_size_t i = 0; i < i_എണ്ണം; ++i) {
        സ്ഥിരം uint8_t* s = i_in + (size_t)i * d_m3WasiSubscriptionSize;

        memset(&o_subs[i], 0, വലുപ്പം(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        എങ്കിൽ (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } അല്ലെങ്കിൽ {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
സ്ഥിതം
ശൂന്യം write_event (uint8_t* o_rec, സ്ഥിരം uvwasi_event_t* i_event)
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
സ്ഥിതം
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, സ്ഥിരം uvwasi_subscription_t* i_subs,
                                        uint8_t* o_out, uvwasi_size_t i_എണ്ണം,
                                        uvwasi_size_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_എണ്ണം ? m3_AllocArray(uvwasi_event_t, i_എണ്ണം) : ശൂന്യ;

    എങ്കിൽ (i_എണ്ണം and not events) {
        തിരികെ UVWASI_ENOMEM;
    }

    uvwasi_size_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_എണ്ണം, &written);

    എങ്കിൽ (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_എണ്ണം);

        വേണ്ടി (uvwasi_size_t i = 0; i < written; ++i) {
            write_event(o_out + (size_t)i * d_m3WasiEventSize, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    തിരികെ ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(സ്ഥിരം uint8_t*, in)
    m3ApiGetArgMem(uint8_t*, out)
    m3ApiGetArg(uvwasi_size_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_size_t*, nevents)

    m3ApiCheckMem(in, (uint64_t)nsubscriptions * d_m3WasiSubscriptionSize);
    m3ApiCheckMem(out, (uint64_t)nsubscriptions * d_m3WasiEventSize);
    m3ApiCheckMem(nevents, വലുപ്പം(uvwasi_size_t));

    uvwasi_subscription_t* subs = ശൂന്യ;

    എങ്കിൽ (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        എങ്കിൽ (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, in, nsubscriptions);
    }

    uvwasi_size_t എണ്ണം = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, out, nsubscriptions, &എണ്ണം);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, എണ്ണം);

    m3ApiWriteMem32(nevents, എണ്ണം);

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

സ്ഥിതം സ്ഥിരം അക്ഷരം* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ശൂന്യ,
};

സ്ഥിതം uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_path = "/",  .real_path = "." },
    { .mapped_path = "./", .real_path = "." },
};

സ്ഥിതം വരിയിൽ
ശൂന്യം _m3_set_up_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = വലുപ്പം(DEFAULT_PREOPENS) / വലുപ്പം(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

സ്ഥിതം
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    സ്ഥിതം സ്ഥിരം അക്ഷരം* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    വേണ്ടി (size_t i = 0; i < M3_എണ്ണം_OF(namespaces); i++) {
        സ്ഥിരം അക്ഷരം* wasi = namespaces[i];

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

M3Result m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    പ്രതിജ്ഞാ(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    എങ്കിൽ (ret != UVWASI_ESUCCESS) {
        തിരികെ "uvwasi_init failed";
    }

    തിരികെ m3Err_none;
}

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    തിരികെ m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    തിരികെ _linkWASI(module, wasi);
}

ശൂന്യം m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    തിരികെ m3_LinkWASIWithOptions(module, opts);
}

M3Result m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    എങ്കിൽ (ശൂന്യ == wasi_context) {
        M3Result res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        എങ്കിൽ (res) {
            തിരികെ res;
        }
    }

    തിരികെ _linkWASI(module, wasi_context);
}

#endif // d_m3HasUVWASI
