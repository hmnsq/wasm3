//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#ضمن "m3_api_wasi.h"

#ضمن "m3_env.h"
#ضمن "m3_exception.h"

#إذا defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
خارجي حرف** environ;
#  endif

ساكن m3_wasi_context_t* wasi_context = عدم;

تعريفنوع بنية wasi_iovec_t {
    uvwasi_سائيز_t buf;
    uvwasi_سائيز_t buf_len;
} wasi_iovec_t;

#  إذا d_m3EnableWasiTracing

ثابت حرف* wasi_errno2str (uvwasi_errno_t err)
{
    // clang-format off
    بدل (err) {
    حالة  0: أرجع "ESUCCESS";
    حالة  1: أرجع "E2BIG";
    حالة  2: أرجع "EACCES";
    حالة  3: أرجع "EADDRINUSE";
    حالة  4: أرجع "EADDRNOTAVAIL";
    حالة  5: أرجع "EAFNOSUPPORT";
    حالة  6: أرجع "EAGAIN";
    حالة  7: أرجع "EALREADY";
    حالة  8: أرجع "EBADF";
    حالة  9: أرجع "EBADMSG";
    حالة 10: أرجع "EBUSY";
    حالة 11: أرجع "ECANCELED";
    حالة 12: أرجع "ECHILD";
    حالة 13: أرجع "ECONNABORTED";
    حالة 14: أرجع "ECONNREFUSED";
    حالة 15: أرجع "ECONNRESET";
    حالة 16: أرجع "EDEADLK";
    حالة 17: أرجع "EDESTADDRREQ";
    حالة 18: أرجع "EDOM";
    حالة 19: أرجع "EDQUOT";
    حالة 20: أرجع "EEXIST";
    حالة 21: أرجع "EFAULT";
    حالة 22: أرجع "EFBIG";
    حالة 23: أرجع "EHOSTUNREACH";
    حالة 24: أرجع "EIDRM";
    حالة 25: أرجع "EILSEQ";
    حالة 26: أرجع "EINPROGRESS";
    حالة 27: أرجع "EINTR";
    حالة 28: أرجع "EINVAL";
    حالة 29: أرجع "EIO";
    حالة 30: أرجع "EISCONN";
    حالة 31: أرجع "EISDIR";
    حالة 32: أرجع "ELOOP";
    حالة 33: أرجع "EMFILE";
    حالة 34: أرجع "EMLINK";
    حالة 35: أرجع "EMSGSIZE";
    حالة 36: أرجع "EMULTIHOP";
    حالة 37: أرجع "ENAMETOOLONG";
    حالة 38: أرجع "ENETDOWN";
    حالة 39: أرجع "ENETRESET";
    حالة 40: أرجع "ENETUNREACH";
    حالة 41: أرجع "ENFILE";
    حالة 42: أرجع "ENOBUFS";
    حالة 43: أرجع "ENODEV";
    حالة 44: أرجع "ENOENT";
    حالة 45: أرجع "ENOEXEC";
    حالة 46: أرجع "ENOLCK";
    حالة 47: أرجع "ENOLINK";
    حالة 48: أرجع "ENOMEM";
    حالة 49: أرجع "ENOMSG";
    حالة 50: أرجع "ENOPROTOOPT";
    حالة 51: أرجع "ENOSPC";
    حالة 52: أرجع "ENOSYS";
    حالة 53: أرجع "ENOTCONN";
    حالة 54: أرجع "ENOTDIR";
    حالة 55: أرجع "ENOTEMPTY";
    حالة 56: أرجع "ENOTRECOVERABLE";
    حالة 57: أرجع "ENOTSOCK";
    حالة 58: أرجع "ENOTSUP";
    حالة 59: أرجع "ENOTTY";
    حالة 60: أرجع "ENXIO";
    حالة 61: أرجع "EOVERFLOW";
    حالة 62: أرجع "EOWNERDEAD";
    حالة 63: أرجع "EPERM";
    حالة 64: أرجع "EPIPE";
    حالة 65: أرجع "EPROTO";
    حالة 66: أرجع "EPROTONOSUPPORT";
    حالة 67: أرجع "EPROTOTYPE";
    حالة 68: أرجع "ERANGE";
    حالة 69: أرجع "EROFS";
    حالة 70: أرجع "ESPIPE";
    حالة 71: أرجع "ESRCH";
    حالة 72: أرجع "ESTALE";
    حالة 73: أرجع "ETIMEDOUT";
    حالة 74: أرجع "ETXTBSY";
    حالة 75: أرجع "EXDEV";
    حالة 76: أرجع "ENOTCAPABLE";
    افتراضي: أرجع "<unknown>";
    }
    // clang-format on
}

ثابت حرف* wasi_whence2str (uvwasi_whence_t whence)
{
    بدل (whence) {
    حالة UVWASI_WHENCE_سيٽ: أرجع "SET";
    حالة UVWASI_WHENCE_CUR: أرجع "CUR";
    حالة UVWASI_WHENCE_پڇاڙي: أرجع "END";
    افتراضي: أرجع "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  وإلا
#    define WASI_TRACE(fmt, ...)
#  endif

ساكن مضمن
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    أكد(context != عدم);
    أرجع &context->uvwasi;
}

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_دليلون_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(حرف*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    إذا (context == عدم) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    m3ApiCheckMem(argv, context->argc * حجم(uint32_t));

    لكل (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_منزل_وٿي(argv_buf));

        size_t len = لڙي_ڊيگهه(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        يادگيري_نقل(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_دليلون_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_سائيز_t*, argc)
    m3ApiGetArgMem(uvwasi_سائيز_t*, argv_buf_سائيز)

    m3ApiCheckMem(argc, حجم(uvwasi_سائيز_t));
    m3ApiCheckMem(argv_buf_سائيز, حجم(uvwasi_سائيز_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    إذا (context == عدم) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_سائيز_t buf_len = 0;
    لكل (u32 i = 0; i < context->argc; ++i) {
        buf_len += (uvwasi_سائيز_t)لڙي_ڊيگهه(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_سائيز, buf_len);

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(حرف*, env_buf)

    حرف**         environment;
    uvwasi_errno_t ret;
    uvwasi_سائيز_t  env_ڳڻپ, env_buf_سائيز;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_ڳڻپ, &env_buf_سائيز);
    إذا (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ڳڻپ * حجم(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_سائيز);

    environment = صفر_يادگيري_ڏيو(env_ڳڻپ, حجم(حرف*));
    إذا (environment == عدم) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    إذا (ret != UVWASI_ESUCCESS) {
        حرر(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_وٿي = m3_Api_Ptr_منزل_وٿي(env_buf);

    لكل (u32 i = 0; i < env_ڳڻپ; ++i) {
        uint32_t offset = environ_buf_وٿي +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    حرر(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_سائيز_t*, env_ڳڻپ)
    m3ApiGetArgMem(uvwasi_سائيز_t*, env_buf_سائيز)

    m3ApiCheckMem(env_ڳڻپ, حجم(uvwasi_سائيز_t));
    m3ApiCheckMem(env_buf_سائيز, حجم(uvwasi_سائيز_t));

    uvwasi_سائيز_t ڳڻپ;
    uvwasi_سائيز_t buf_سائيز;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &ڳڻپ, &buf_سائيز);

    m3ApiWriteMem32(env_ڳڻپ, ڳڻپ);
    m3ApiWriteMem32(env_buf_سائيز, buf_سائيز);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(حرف*, رستو)
    m3ApiGetArg(uvwasi_سائيز_t, رستو_len)

    m3ApiCheckMem(رستو, رستو_len);

    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(_get_uvwasi_p(_ctx), fd, رستو, رستو_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, رستو_len, رستو);

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

    إذا (ret != UVWASI_ESUCCESS) {
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

    إذا (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 24);
    m3ApiWriteMem8(buf + 0, stat.fs_filetype);
    m3ApiWriteMem16(buf + 2, stat.fs_flags);
    m3ApiWriteMem64(buf + 8, stat.fs_rights_بنياد);
    m3ApiWriteMem64(buf + 16, stat.fs_rights_inheriting);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_سيٽ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_fdflags_t, flags)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_سيٽ_flags(_get_uvwasi_p(_ctx), fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_سيٽ_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_rights_t, rights_بنياد)
    m3ApiGetArg(uvwasi_rights_t, rights_inheriting)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_سيٽ_rights(_get_uvwasi_p(_ctx), fd, rights_بنياد, rights_inheriting);

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_بنياد, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سيٽ_سائيز)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, سائيز)

    uvwasi_errno_t ret = uvwasi_fd_filestat_سيٽ_سائيز(_get_uvwasi_p(_ctx), fd, سائيز);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, سائيز);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سيٽ_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_timestamp_t, atim)
    m3ApiGetArg(uvwasi_timestamp_t, mtim)
    m3ApiGetArg(uvwasi_fstflags_t, fst_flags)

    uvwasi_errno_t ret = uvwasi_fd_filestat_سيٽ_times(_get_uvwasi_p(_ctx), fd, atim, mtim, fst_flags);

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

    إذا (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArgMem(uvwasi_filesize_t*, نتيجو)

    m3ApiCheckMem(نتيجو, حجم(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    بدل (wasi_whence) {
    حالة 0: whence = UVWASI_WHENCE_سيٽ; اكسر;
    حالة 1: whence = UVWASI_WHENCE_CUR; اكسر;
    حالة 2: whence = UVWASI_WHENCE_پڇاڙي; اكسر;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_seek(_get_uvwasi_p(_ctx), fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    m3ApiWriteMem64(نتيجو, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, from)
    m3ApiGetArg(uvwasi_fd_t, منزل)

    uvwasi_errno_t ret = uvwasi_fd_renumber(_get_uvwasi_p(_ctx), from, منزل);

    WASI_TRACE("from:%d, to:%d", from, منزل);

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
    m3ApiGetArgMem(uvwasi_filesize_t*, نتيجو)

    m3ApiCheckMem(نتيجو, حجم(uvwasi_filesize_t));

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_tell(_get_uvwasi_p(_ctx), fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    m3ApiWriteMem64(نتيجو, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, رستو)
    m3ApiGetArg(uvwasi_سائيز_t, رستو_len)

    m3ApiCheckMem(رستو, رستو_len);

    uvwasi_errno_t ret = uvwasi_رستو_create_directory(_get_uvwasi_p(_ctx), fd, رستو, رستو_len);

    WASI_TRACE("fd:%d, path:%s", fd, رستو);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, رستو)
    m3ApiGetArg(uvwasi_سائيز_t, رستو_len)
    m3ApiGetArgMem(حرف*, buf)
    m3ApiGetArg(uvwasi_سائيز_t, buf_len)
    m3ApiGetArgMem(uvwasi_سائيز_t*, bufused)

    m3ApiCheckMem(رستو, رستو_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, حجم(uvwasi_سائيز_t));

    uvwasi_سائيز_t uvbufused;

    uvwasi_errno_t ret = uvwasi_رستو_readlink(_get_uvwasi_p(_ctx), fd, رستو, رستو_len, buf, buf_len, &uvbufused);

    WASI_TRACE("fd:%d, path:%s | buf:%s, bufused:%d", fd, رستو, buf, uvbufused);

    m3ApiWriteMem32(bufused, uvbufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, رستو)
    m3ApiGetArg(uvwasi_سائيز_t, رستو_len)

    m3ApiCheckMem(رستو, رستو_len);

    uvwasi_errno_t ret = uvwasi_رستو_remove_directory(_get_uvwasi_p(_ctx), fd, رستو, رستو_len);

    WASI_TRACE("fd:%d, path:%s", fd, رستو);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, old_fd)
    m3ApiGetArgMem(ثابت حرف*, old_رستو)
    m3ApiGetArg(uvwasi_سائيز_t, old_رستو_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(ثابت حرف*, new_رستو)
    m3ApiGetArg(uvwasi_سائيز_t, new_رستو_len)

    m3ApiCheckMem(old_رستو, old_رستو_len);
    m3ApiCheckMem(new_رستو, new_رستو_len);

    uvwasi_errno_t ret = uvwasi_رستو_rename(_get_uvwasi_p(_ctx), old_fd, old_رستو, old_رستو_len,
                                            new_fd, new_رستو, new_رستو_len);

    WASI_TRACE("old_fd:%d, old_path:%s, new_fd:%d, new_path:%s", old_fd, old_رستو, new_fd, new_رستو);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت حرف*, old_رستو)
    m3ApiGetArg(uvwasi_سائيز_t, old_رستو_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, new_رستو)
    m3ApiGetArg(uvwasi_سائيز_t, new_رستو_len)

    m3ApiCheckMem(old_رستو, old_رستو_len);
    m3ApiCheckMem(new_رستو, new_رستو_len);

    uvwasi_errno_t ret = uvwasi_رستو_symlink(_get_uvwasi_p(_ctx), old_رستو, old_رستو_len,
                                             fd, new_رستو, new_رستو_len);

    WASI_TRACE("old_path:%s, fd:%d, new_path:%s", old_رستو, fd, new_رستو);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, رستو)
    m3ApiGetArg(uvwasi_سائيز_t, رستو_len)

    m3ApiCheckMem(رستو, رستو_len);

    uvwasi_errno_t ret = uvwasi_رستو_unlink_file(_get_uvwasi_p(_ctx), fd, رستو, رستو_len);

    WASI_TRACE("fd:%d, path:%s", fd, رستو);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, dirfd)
    m3ApiGetArg(uvwasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ثابت حرف*, رستو)
    m3ApiGetArg(uvwasi_سائيز_t, رستو_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_بنياد)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(رستو, رستو_len);
    m3ApiCheckMem(fd, حجم(uvwasi_fd_t));

    uvwasi_fd_t uvfd;

    uvwasi_errno_t ret = uvwasi_رستو_open(_get_uvwasi_p(_ctx),
                                          dirfd,
                                          dirflags,
                                          رستو,
                                          رستو_len,
                                          oflags,
                                          fs_rights_بنياد,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          &uvfd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%s, oflags:0x%x, fs_flags:0x%x | fd:%d", dirfd, dirflags, رستو, oflags, fs_flags, uvfd);

    m3ApiWriteMem32(fd, uvfd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_رستو_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_lookupflags_t, flags)
    m3ApiGetArgMem(ثابت حرف*, رستو)
    m3ApiGetArg(uint32_t, رستو_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(رستو, رستو_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_رستو_filestat_get(_get_uvwasi_p(_ctx), fd, flags, رستو, رستو_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, رستو, stat.st_size);

    إذا (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArg(uvwasi_سائيز_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_سائيز_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(uvwasi_سائيز_t));

#  إذا M3_HAS_VLA
    إذا (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  وإلا
    إذا (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    لكل (uvwasi_سائيز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_وٿي_منزل_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_سائيز_t انگ_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &انگ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, انگ_read);

    m3ApiWriteMem32(nread, انگ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_سائيز_t, iovs_len)
    m3ApiGetArgMem(uvwasi_سائيز_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(uvwasi_سائيز_t));

#  إذا M3_HAS_VLA
    إذا (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  وإلا
    إذا (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_سائيز_t  انگ_read;
    uvwasi_errno_t ret;

    لكل (uvwasi_سائيز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_وٿي_منزل_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &انگ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, انگ_read);

    m3ApiWriteMem32(nread, انگ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_سائيز_t, iovs_len)
    m3ApiGetArgMem(uvwasi_سائيز_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(uvwasi_سائيز_t));

#  إذا M3_HAS_VLA
    إذا (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  وإلا
    إذا (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_سائيز_t  انگ_written;
    uvwasi_errno_t ret;

    لكل (uvwasi_سائيز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_وٿي_منزل_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &انگ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, انگ_written);

    m3ApiWriteMem32(nwritten, انگ_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pwrite)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_سائيز_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_سائيز_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(uvwasi_سائيز_t));

#  إذا M3_HAS_VLA
    إذا (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  وإلا
    إذا (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_سائيز_t  انگ_written;
    uvwasi_errno_t ret;

    لكل (uvwasi_سائيز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_وٿي_منزل_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &انگ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, انگ_written);

    m3ApiWriteMem32(nwritten, انگ_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(فراغ*, buf)
    m3ApiGetArg(uvwasi_سائيز_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_سائيز_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, حجم(uvwasi_سائيز_t));

    uvwasi_سائيز_t  uvbufused;
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
    m3ApiGetArg(uvwasi_filesize_t, ڊيگهائي)
    m3ApiGetArg(uvwasi_advice_t, advice)

    uvwasi_errno_t ret = uvwasi_fd_advise(_get_uvwasi_p(_ctx), fd, offset, ڊيگهائي, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, ڊيگهائي, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArg(uvwasi_filesize_t, ڊيگهائي)

    uvwasi_errno_t ret = uvwasi_fd_allocate(_get_uvwasi_p(_ctx), fd, offset, ڊيگهائي);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, ڊيگهائي);

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
    m3ApiGetArg(uvwasi_سائيز_t, buf_len)

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

    m3ApiCheckMem(resolution, حجم(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_res_get(_get_uvwasi_p(_ctx), wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    m3ApiWriteMem64(resolution, t);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_وقت_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(uvwasi_timestamp_t, precision)
    m3ApiGetArgMem(uvwasi_timestamp_t*, time)

    m3ApiCheckMem(time, حجم(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_وقت_get(_get_uvwasi_p(_ctx), wasi_clk_id, precision, &t);

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
#  define d_m3_Wasi_Subscription_سائيز            48

#  define d_m3_Wasi_Event_سائيز                   32

// Reads the guest's subscription list into host structs. The caller has already
// bounds-checked i_in for i_count records. 48 bytes a record: userdata 0, type 8,
// fd 16, and a clock's fields from 16.
ساكن
فراغ read_subscriptions (uvwasi_subscription_t* o_subs, ثابت uint8_t* i_اندريون,
                         uvwasi_سائيز_t i_ڳڻپ)
{
    لكل (uvwasi_سائيز_t i = 0; i < i_ڳڻپ; ++i) {
        ثابت uint8_t* s = i_اندريون + (size_t)i * d_m3_Wasi_Subscription_سائيز;

        memset(&o_subs[i], 0, حجم(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        إذا (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } وإلا {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ساكن
فراغ write_event (uint8_t* o_rec, ثابت uvwasi_event_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_سائيز);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->u.fd_readwrite.nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->u.fd_readwrite.flags);
}

// Waits on the subscriptions, once whichever reader above has decoded them, and
// writes the events back. Knows nothing about revisions: the event record is the
// same in both. i_subs is NULL when there are none, which uvwasi answers for itself.
ساكن
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, ثابت uvwasi_subscription_t* i_subs,
                                        uint8_t* o_ٻاهريون, uvwasi_سائيز_t i_ڳڻپ,
                                        uvwasi_سائيز_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_ڳڻپ ? m3_AllocArray(uvwasi_event_t, i_ڳڻپ) : عدم;

    إذا (i_ڳڻپ and not events) {
        أرجع UVWASI_ENOMEM;
    }

    uvwasi_سائيز_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_ڳڻپ, &written);

    إذا (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_ڳڻپ);

        لكل (uvwasi_سائيز_t i = 0; i < written; ++i) {
            write_event(o_ٻاهريون + (size_t)i * d_m3_Wasi_Event_سائيز, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    أرجع ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت uint8_t*, اندريون)
    m3ApiGetArgMem(uint8_t*, ٻاهريون)
    m3ApiGetArg(uvwasi_سائيز_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_سائيز_t*, nevents)

    m3ApiCheckMem(اندريون, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_سائيز);
    m3ApiCheckMem(ٻاهريون, (uint64_t)nsubscriptions * d_m3_Wasi_Event_سائيز);
    m3ApiCheckMem(nevents, حجم(uvwasi_سائيز_t));

    uvwasi_subscription_t* subs = عدم;

    إذا (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        إذا (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, اندريون, nsubscriptions);
    }

    uvwasi_سائيز_t ڳڻپ = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, ٻاهريون, nsubscriptions, &ڳڻپ);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, ڳڻپ);

    m3ApiWriteMem32(nevents, ڳڻپ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, ڪوڊ)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    إذا (context) {
        context->exit_ڪوڊ = ڪوڊ;
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


ساكن
M3_نتيجو SuppressLookupFailure (M3_نتيجو i_نتيجو)
{
    إذا (i_نتيجو == m3Err_functionLookupFailed) {
        أرجع m3Err_none;
    } وإلا {
        أرجع i_نتيجو;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    أرجع wasi_context;
}

ساكن ثابت حرف* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    عدم,
};

ساكن uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_رستو = "/",  .real_رستو = "." },
    { .mapped_رستو = "./", .real_رستو = "." },
};

ساكن مضمن
فراغ m3_سيٽ_مٿي_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = حجم(DEFAULT_PREOPENS) / حجم(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

ساكن
M3_نتيجو _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_نتيجو نتيجو = m3Err_none;

    ساكن ثابت حرف* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    لكل (size_t i = 0; i < M3_ڳڻپ_جو(namespaces); i++) {
        ثابت حرف* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_دليلون_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_دليلون_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_وقت_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_سيٽ_flags, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_سيٽ_rights, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_سيٽ_سائيز, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_سيٽ_times, wasi_context)));
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

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_رستو_create_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_رستو_filestat_get, wasi_context)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_رستو_open, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_رستو_readlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_رستو_remove_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_رستو_rename, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_رستو_symlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_رستو_unlink_file, wasi_context)));

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
    إذا (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 يادگيري_اشاريو;

        إذا (m3_ڳوليو_Exported_يادگيري(module, "memory", &يادگيري_اشاريو)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_يادگيري(module, "wasi_snapshot_preview1", يادگيري_اشاريو));
    }

_catch:
    أرجع نتيجو;
}

M3_نتيجو m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    أكد(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    إذا (ret != UVWASI_ESUCCESS) {
        أرجع "uvwasi_init failed";
    }

    أرجع m3Err_none;
}

M3_نتيجو m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    m3_سيٽ_مٿي_default_uvwasi_opts(&opts);

    أرجع m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3_نتيجو m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    أرجع _linkWASI(module, wasi);
}

فراغ m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3_نتيجو m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    m3_سيٽ_مٿي_default_uvwasi_opts(&opts);

    أرجع m3_LinkWASIWithOptions(module, opts);
}

M3_نتيجو m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    إذا (عدم == wasi_context) {
        M3_نتيجو res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        إذا (res) {
            أرجع res;
        }
    }

    أرجع _linkWASI(module, wasi_context);
}

#جيڪڏهن_پورو // d_m3HasUVWASI
