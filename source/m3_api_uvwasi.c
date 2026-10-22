//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#شامل "m3_api_wasi.h"

#شامل "m3_env.h"
#شامل "m3_exception.h"

#اگر defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
خارجی حرف** environ;
#  endif

ساکن m3_wasi_context_t* wasi_context = تہی;

قسمتعریف ساخت wasi_iovec_t {
    uvwasi_سائز_t buf;
    uvwasi_سائز_t buf_len;
} wasi_iovec_t;

#  اگر d_m3EnableWasiTracing

ثابت حرف* wasi_errno2str (uvwasi_errno_t err)
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

ثابت حرف* wasi_whence2str (uvwasi_whence_t whence)
{
    انتخاب (whence) {
    حالت UVWASI_WHENCE_سیٹ: واپس "SET";
    حالت UVWASI_WHENCE_CUR: واپس "CUR";
    حالت UVWASI_WHENCE_انتہا: واپس "END";
    طےشدہ: واپس "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ورنہ
#    define WASI_TRACE(fmt, ...)
#  endif

ساکن اندرونی
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    أكد(context != تہی);
    واپس &context->uvwasi;
}

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
        m3ApiReturn(UVWASI_EINVAL);
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

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_دلائل_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_سائز_t*, argc)
    m3ApiGetArgMem(uvwasi_سائز_t*, argv_buf_سائز)

    m3ApiCheckMem(argc, حجم(uvwasi_سائز_t));
    m3ApiCheckMem(argv_buf_سائز, حجم(uvwasi_سائز_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    اگر (context == تہی) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_سائز_t buf_len = 0;
    برائے (u32 i = 0; i < context->argc; ++i) {
        buf_len += (uvwasi_سائز_t)لڑی_لمبائی(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_سائز, buf_len);

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(حرف*, env_buf)

    حرف**         environment;
    uvwasi_errno_t ret;
    uvwasi_سائز_t  env_گنتی, env_buf_سائز;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_گنتی, &env_buf_سائز);
    اگر (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_گنتی * حجم(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_سائز);

    environment = صفر_حافظہ_دو(env_گنتی, حجم(حرف*));
    اگر (environment == تہی) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    اگر (ret != UVWASI_ESUCCESS) {
        آزاد(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_انحراف = m3_Api_Ptr_مقصد_انحراف(env_buf);

    برائے (u32 i = 0; i < env_گنتی; ++i) {
        uint32_t offset = environ_buf_انحراف +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    آزاد(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_سائز_t*, env_گنتی)
    m3ApiGetArgMem(uvwasi_سائز_t*, env_buf_سائز)

    m3ApiCheckMem(env_گنتی, حجم(uvwasi_سائز_t));
    m3ApiCheckMem(env_buf_سائز, حجم(uvwasi_سائز_t));

    uvwasi_سائز_t گنتی;
    uvwasi_سائز_t buf_سائز;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &گنتی, &buf_سائز);

    m3ApiWriteMem32(env_گنتی, گنتی);
    m3ApiWriteMem32(env_buf_سائز, buf_سائز);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(حرف*, راستہ)
    m3ApiGetArg(uvwasi_سائز_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(_get_uvwasi_p(_ctx), fd, راستہ, راستہ_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, راستہ_len, راستہ);

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

    اگر (ret != UVWASI_ESUCCESS) {
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

    اگر (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 24);
    m3ApiWriteMem8(buf + 0, stat.fs_filetype);
    m3ApiWriteMem16(buf + 2, stat.fs_flags);
    m3ApiWriteMem64(buf + 8, stat.fs_rights_بنیاد);
    m3ApiWriteMem64(buf + 16, stat.fs_rights_inheriting);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_سیٹ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_fdflags_t, flags)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_سیٹ_flags(_get_uvwasi_p(_ctx), fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_سیٹ_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_rights_t, rights_بنیاد)
    m3ApiGetArg(uvwasi_rights_t, rights_inheriting)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_سیٹ_rights(_get_uvwasi_p(_ctx), fd, rights_بنیاد, rights_inheriting);

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_بنیاد, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سیٹ_سائز)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, سائز)

    uvwasi_errno_t ret = uvwasi_fd_filestat_سیٹ_سائز(_get_uvwasi_p(_ctx), fd, سائز);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, سائز);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سیٹ_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_timestamp_t, atim)
    m3ApiGetArg(uvwasi_timestamp_t, mtim)
    m3ApiGetArg(uvwasi_fstflags_t, fst_flags)

    uvwasi_errno_t ret = uvwasi_fd_filestat_سیٹ_times(_get_uvwasi_p(_ctx), fd, atim, mtim, fst_flags);

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

    اگر (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArgMem(uvwasi_filesize_t*, نتیجہ)

    m3ApiCheckMem(نتیجہ, حجم(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    انتخاب (wasi_whence) {
    حالت 0: whence = UVWASI_WHENCE_سیٹ; توڑو;
    حالت 1: whence = UVWASI_WHENCE_CUR; توڑو;
    حالت 2: whence = UVWASI_WHENCE_انتہا; توڑو;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_seek(_get_uvwasi_p(_ctx), fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    m3ApiWriteMem64(نتیجہ, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, from)
    m3ApiGetArg(uvwasi_fd_t, مقصد)

    uvwasi_errno_t ret = uvwasi_fd_renumber(_get_uvwasi_p(_ctx), from, مقصد);

    WASI_TRACE("from:%d, to:%d", from, مقصد);

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
    m3ApiGetArgMem(uvwasi_filesize_t*, نتیجہ)

    m3ApiCheckMem(نتیجہ, حجم(uvwasi_filesize_t));

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_tell(_get_uvwasi_p(_ctx), fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    m3ApiWriteMem64(نتیجہ, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(uvwasi_سائز_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    uvwasi_errno_t ret = uvwasi_راستہ_create_directory(_get_uvwasi_p(_ctx), fd, راستہ, راستہ_len);

    WASI_TRACE("fd:%d, path:%s", fd, راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(uvwasi_سائز_t, راستہ_len)
    m3ApiGetArgMem(حرف*, buf)
    m3ApiGetArg(uvwasi_سائز_t, buf_len)
    m3ApiGetArgMem(uvwasi_سائز_t*, bufused)

    m3ApiCheckMem(راستہ, راستہ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, حجم(uvwasi_سائز_t));

    uvwasi_سائز_t uvbufused;

    uvwasi_errno_t ret = uvwasi_راستہ_readlink(_get_uvwasi_p(_ctx), fd, راستہ, راستہ_len, buf, buf_len, &uvbufused);

    WASI_TRACE("fd:%d, path:%s | buf:%s, bufused:%d", fd, راستہ, buf, uvbufused);

    m3ApiWriteMem32(bufused, uvbufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(uvwasi_سائز_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    uvwasi_errno_t ret = uvwasi_راستہ_remove_directory(_get_uvwasi_p(_ctx), fd, راستہ, راستہ_len);

    WASI_TRACE("fd:%d, path:%s", fd, راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, old_fd)
    m3ApiGetArgMem(ثابت حرف*, old_راستہ)
    m3ApiGetArg(uvwasi_سائز_t, old_راستہ_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(ثابت حرف*, new_راستہ)
    m3ApiGetArg(uvwasi_سائز_t, new_راستہ_len)

    m3ApiCheckMem(old_راستہ, old_راستہ_len);
    m3ApiCheckMem(new_راستہ, new_راستہ_len);

    uvwasi_errno_t ret = uvwasi_راستہ_rename(_get_uvwasi_p(_ctx), old_fd, old_راستہ, old_راستہ_len,
                                            new_fd, new_راستہ, new_راستہ_len);

    WASI_TRACE("old_fd:%d, old_path:%s, new_fd:%d, new_path:%s", old_fd, old_راستہ, new_fd, new_راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت حرف*, old_راستہ)
    m3ApiGetArg(uvwasi_سائز_t, old_راستہ_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, new_راستہ)
    m3ApiGetArg(uvwasi_سائز_t, new_راستہ_len)

    m3ApiCheckMem(old_راستہ, old_راستہ_len);
    m3ApiCheckMem(new_راستہ, new_راستہ_len);

    uvwasi_errno_t ret = uvwasi_راستہ_symlink(_get_uvwasi_p(_ctx), old_راستہ, old_راستہ_len,
                                             fd, new_راستہ, new_راستہ_len);

    WASI_TRACE("old_path:%s, fd:%d, new_path:%s", old_راستہ, fd, new_راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(uvwasi_سائز_t, راستہ_len)

    m3ApiCheckMem(راستہ, راستہ_len);

    uvwasi_errno_t ret = uvwasi_راستہ_unlink_file(_get_uvwasi_p(_ctx), fd, راستہ, راستہ_len);

    WASI_TRACE("fd:%d, path:%s", fd, راستہ);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, dirfd)
    m3ApiGetArg(uvwasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(uvwasi_سائز_t, راستہ_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_بنیاد)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(راستہ, راستہ_len);
    m3ApiCheckMem(fd, حجم(uvwasi_fd_t));

    uvwasi_fd_t uvfd;

    uvwasi_errno_t ret = uvwasi_راستہ_open(_get_uvwasi_p(_ctx),
                                          dirfd,
                                          dirflags,
                                          راستہ,
                                          راستہ_len,
                                          oflags,
                                          fs_rights_بنیاد,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          &uvfd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%s, oflags:0x%x, fs_flags:0x%x | fd:%d", dirfd, dirflags, راستہ, oflags, fs_flags, uvfd);

    m3ApiWriteMem32(fd, uvfd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_راستہ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_lookupflags_t, flags)
    m3ApiGetArgMem(ثابت حرف*, راستہ)
    m3ApiGetArg(uint32_t, راستہ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(راستہ, راستہ_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_راستہ_filestat_get(_get_uvwasi_p(_ctx), fd, flags, راستہ, راستہ_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, راستہ, stat.st_size);

    اگر (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArg(uvwasi_سائز_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_سائز_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(uvwasi_سائز_t));

#  اگر M3_HAS_VLA
    اگر (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ورنہ
    اگر (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    برائے (uvwasi_سائز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_انحراف_مقصد_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_سائز_t عدد_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &عدد_read);

    WASI_TRACE("fd:%d | nread:%d", fd, عدد_read);

    m3ApiWriteMem32(nread, عدد_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_سائز_t, iovs_len)
    m3ApiGetArgMem(uvwasi_سائز_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(uvwasi_سائز_t));

#  اگر M3_HAS_VLA
    اگر (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ورنہ
    اگر (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_سائز_t  عدد_read;
    uvwasi_errno_t ret;

    برائے (uvwasi_سائز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_انحراف_مقصد_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &عدد_read);

    WASI_TRACE("fd:%d | nread:%d", fd, عدد_read);

    m3ApiWriteMem32(nread, عدد_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_سائز_t, iovs_len)
    m3ApiGetArgMem(uvwasi_سائز_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(uvwasi_سائز_t));

#  اگر M3_HAS_VLA
    اگر (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ورنہ
    اگر (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_سائز_t  عدد_written;
    uvwasi_errno_t ret;

    برائے (uvwasi_سائز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_انحراف_مقصد_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &عدد_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, عدد_written);

    m3ApiWriteMem32(nwritten, عدد_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pwrite)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_سائز_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_سائز_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(uvwasi_سائز_t));

#  اگر M3_HAS_VLA
    اگر (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ورنہ
    اگر (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_سائز_t  عدد_written;
    uvwasi_errno_t ret;

    برائے (uvwasi_سائز_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_انحراف_مقصد_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &عدد_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, عدد_written);

    m3ApiWriteMem32(nwritten, عدد_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(خالی*, buf)
    m3ApiGetArg(uvwasi_سائز_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_سائز_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, حجم(uvwasi_سائز_t));

    uvwasi_سائز_t  uvbufused;
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
    m3ApiGetArg(uvwasi_filesize_t, لمبائی)
    m3ApiGetArg(uvwasi_advice_t, advice)

    uvwasi_errno_t ret = uvwasi_fd_advise(_get_uvwasi_p(_ctx), fd, offset, لمبائی, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, لمبائی, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArg(uvwasi_filesize_t, لمبائی)

    uvwasi_errno_t ret = uvwasi_fd_allocate(_get_uvwasi_p(_ctx), fd, offset, لمبائی);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, لمبائی);

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
    m3ApiGetArg(uvwasi_سائز_t, buf_len)

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
#  define d_m3_Wasi_Subscription_سائز            48

#  define d_m3_Wasi_Event_سائز                   32

// Reads the guest's subscription list into host structs. The caller has already
// bounds-checked i_in for i_count records. 48 bytes a record: userdata 0, type 8,
// fd 16, and a clock's fields from 16.
ساکن
خالی read_subscriptions (uvwasi_subscription_t* o_subs, ثابت uint8_t* i_اندر,
                         uvwasi_سائز_t i_گنتی)
{
    برائے (uvwasi_سائز_t i = 0; i < i_گنتی; ++i) {
        ثابت uint8_t* s = i_اندر + (size_t)i * d_m3_Wasi_Subscription_سائز;

        memset(&o_subs[i], 0, حجم(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        اگر (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } ورنہ {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ساکن
خالی write_event (uint8_t* o_rec, ثابت uvwasi_event_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_سائز);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->u.fd_readwrite.nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->u.fd_readwrite.flags);
}

// Waits on the subscriptions, once whichever reader above has decoded them, and
// writes the events back. Knows nothing about revisions: the event record is the
// same in both. i_subs is NULL when there are none, which uvwasi answers for itself.
ساکن
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, ثابت uvwasi_subscription_t* i_subs,
                                        uint8_t* o_برآمد, uvwasi_سائز_t i_گنتی,
                                        uvwasi_سائز_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_گنتی ? m3_AllocArray(uvwasi_event_t, i_گنتی) : تہی;

    اگر (i_گنتی and not events) {
        واپس UVWASI_ENOMEM;
    }

    uvwasi_سائز_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_گنتی, &written);

    اگر (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_گنتی);

        برائے (uvwasi_سائز_t i = 0; i < written; ++i) {
            write_event(o_برآمد + (size_t)i * d_m3_Wasi_Event_سائز, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    واپس ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت uint8_t*, اندر)
    m3ApiGetArgMem(uint8_t*, برآمد)
    m3ApiGetArg(uvwasi_سائز_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_سائز_t*, nevents)

    m3ApiCheckMem(اندر, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_سائز);
    m3ApiCheckMem(برآمد, (uint64_t)nsubscriptions * d_m3_Wasi_Event_سائز);
    m3ApiCheckMem(nevents, حجم(uvwasi_سائز_t));

    uvwasi_subscription_t* subs = تہی;

    اگر (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        اگر (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, اندر, nsubscriptions);
    }

    uvwasi_سائز_t گنتی = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, برآمد, nsubscriptions, &گنتی);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, گنتی);

    m3ApiWriteMem32(nevents, گنتی);

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

ساکن ثابت حرف* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    تہی,
};

ساکن uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_راستہ = "/",  .real_راستہ = "." },
    { .mapped_راستہ = "./", .real_راستہ = "." },
};

ساکن اندرونی
خالی m3_سیٹ_اوپر_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = حجم(DEFAULT_PREOPENS) / حجم(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

ساکن
M3_نتیجہ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    ساکن ثابت حرف* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    برائے (size_t i = 0; i < M3_گنتی_کا(namespaces); i++) {
        ثابت حرف* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_دلائل_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_دلائل_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_وقت_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_سیٹ_flags, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_سیٹ_rights, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_سیٹ_سائز, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_سیٹ_times, wasi_context)));
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

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_راستہ_create_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_راستہ_filestat_get, wasi_context)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_راستہ_open, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_راستہ_readlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_راستہ_remove_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_راستہ_rename, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_راستہ_symlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_راستہ_unlink_file, wasi_context)));

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

M3_نتیجہ m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    أكد(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    اگر (ret != UVWASI_ESUCCESS) {
        واپس "uvwasi_init failed";
    }

    واپس m3Err_none;
}

M3_نتیجہ m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    m3_سیٹ_اوپر_default_uvwasi_opts(&opts);

    واپس m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3_نتیجہ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    واپس _linkWASI(module, wasi);
}

خالی m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3_نتیجہ m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    m3_سیٹ_اوپر_default_uvwasi_opts(&opts);

    واپس m3_LinkWASIWithOptions(module, opts);
}

M3_نتیجہ m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    اگر (تہی == wasi_context) {
        M3_نتیجہ res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        اگر (res) {
            واپس res;
        }
    }

    واپس _linkWASI(module, wasi_context);
}

#اگر_ختم // d_m3HasUVWASI
