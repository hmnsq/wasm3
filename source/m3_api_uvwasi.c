//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#অন্তর্ভাৱ "m3_api_wasi.h"

#অন্তর্ভাৱ "m3_env.h"
#অন্তর্ভাৱ "m3_exception.h"

#যদি defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
বাহ্য ৱর্ণ** environ;
#  endif

স্থির m3_wasi_context_t* wasi_context = রিক্ত;

প্রকারনির্দেশ সংরচনা wasi_iovec_t {
    uvwasi_size_t buf;
    uvwasi_size_t buf_len;
} wasi_iovec_t;

#  যদি d_m3EnableWasiTracing

ধ্রুৱ ৱর্ণ* wasi_errno2str (uvwasi_errno_t err)
{
    // clang-format off
    ৱিকল্প (err) {
    পক্ষ  0: ঘূৰাই "ESUCCESS";
    পক্ষ  1: ঘূৰাই "E2BIG";
    পক্ষ  2: ঘূৰাই "EACCES";
    পক্ষ  3: ঘূৰাই "EADDRINUSE";
    পক্ষ  4: ঘূৰাই "EADDRNOTAVAIL";
    পক্ষ  5: ঘূৰাই "EAFNOSUPPORT";
    পক্ষ  6: ঘূৰাই "EAGAIN";
    পক্ষ  7: ঘূৰাই "EALREADY";
    পক্ষ  8: ঘূৰাই "EBADF";
    পক্ষ  9: ঘূৰাই "EBADMSG";
    পক্ষ 10: ঘূৰাই "EBUSY";
    পক্ষ 11: ঘূৰাই "ECANCELED";
    পক্ষ 12: ঘূৰাই "ECHILD";
    পক্ষ 13: ঘূৰাই "ECONNABORTED";
    পক্ষ 14: ঘূৰাই "ECONNREFUSED";
    পক্ষ 15: ঘূৰাই "ECONNRESET";
    পক্ষ 16: ঘূৰাই "EDEADLK";
    পক্ষ 17: ঘূৰাই "EDESTADDRREQ";
    পক্ষ 18: ঘূৰাই "EDOM";
    পক্ষ 19: ঘূৰাই "EDQUOT";
    পক্ষ 20: ঘূৰাই "EEXIST";
    পক্ষ 21: ঘূৰাই "EFAULT";
    পক্ষ 22: ঘূৰাই "EFBIG";
    পক্ষ 23: ঘূৰাই "EHOSTUNREACH";
    পক্ষ 24: ঘূৰাই "EIDRM";
    পক্ষ 25: ঘূৰাই "EILSEQ";
    পক্ষ 26: ঘূৰাই "EINPROGRESS";
    পক্ষ 27: ঘূৰাই "EINTR";
    পক্ষ 28: ঘূৰাই "EINVAL";
    পক্ষ 29: ঘূৰাই "EIO";
    পক্ষ 30: ঘূৰাই "EISCONN";
    পক্ষ 31: ঘূৰাই "EISDIR";
    পক্ষ 32: ঘূৰাই "ELOOP";
    পক্ষ 33: ঘূৰাই "EMFILE";
    পক্ষ 34: ঘূৰাই "EMLINK";
    পক্ষ 35: ঘূৰাই "EMSGSIZE";
    পক্ষ 36: ঘূৰাই "EMULTIHOP";
    পক্ষ 37: ঘূৰাই "ENAMETOOLONG";
    পক্ষ 38: ঘূৰাই "ENETDOWN";
    পক্ষ 39: ঘূৰাই "ENETRESET";
    পক্ষ 40: ঘূৰাই "ENETUNREACH";
    পক্ষ 41: ঘূৰাই "ENFILE";
    পক্ষ 42: ঘূৰাই "ENOBUFS";
    পক্ষ 43: ঘূৰাই "ENODEV";
    পক্ষ 44: ঘূৰাই "ENOENT";
    পক্ষ 45: ঘূৰাই "ENOEXEC";
    পক্ষ 46: ঘূৰাই "ENOLCK";
    পক্ষ 47: ঘূৰাই "ENOLINK";
    পক্ষ 48: ঘূৰাই "ENOMEM";
    পক্ষ 49: ঘূৰাই "ENOMSG";
    পক্ষ 50: ঘূৰাই "ENOPROTOOPT";
    পক্ষ 51: ঘূৰাই "ENOSPC";
    পক্ষ 52: ঘূৰাই "ENOSYS";
    পক্ষ 53: ঘূৰাই "ENOTCONN";
    পক্ষ 54: ঘূৰাই "ENOTDIR";
    পক্ষ 55: ঘূৰাই "ENOTEMPTY";
    পক্ষ 56: ঘূৰাই "ENOTRECOVERABLE";
    পক্ষ 57: ঘূৰাই "ENOTSOCK";
    পক্ষ 58: ঘূৰাই "ENOTSUP";
    পক্ষ 59: ঘূৰাই "ENOTTY";
    পক্ষ 60: ঘূৰাই "ENXIO";
    পক্ষ 61: ঘূৰাই "EOVERFLOW";
    পক্ষ 62: ঘূৰাই "EOWNERDEAD";
    পক্ষ 63: ঘূৰাই "EPERM";
    পক্ষ 64: ঘূৰাই "EPIPE";
    পক্ষ 65: ঘূৰাই "EPROTO";
    পক্ষ 66: ঘূৰাই "EPROTONOSUPPORT";
    পক্ষ 67: ঘূৰাই "EPROTOTYPE";
    পক্ষ 68: ঘূৰাই "ERANGE";
    পক্ষ 69: ঘূৰাই "EROFS";
    পক্ষ 70: ঘূৰাই "ESPIPE";
    পক্ষ 71: ঘূৰাই "ESRCH";
    পক্ষ 72: ঘূৰাই "ESTALE";
    পক্ষ 73: ঘূৰাই "ETIMEDOUT";
    পক্ষ 74: ঘূৰাই "ETXTBSY";
    পক্ষ 75: ঘূৰাই "EXDEV";
    পক্ষ 76: ঘূৰাই "ENOTCAPABLE";
    স্ৱভাৱ: ঘূৰাই "<unknown>";
    }
    // clang-format on
}

ধ্রুৱ ৱর্ণ* wasi_whence2str (uvwasi_whence_t whence)
{
    ৱিকল্প (whence) {
    পক্ষ UVWASI_WHENCE_SET: ঘূৰাই "SET";
    পক্ষ UVWASI_WHENCE_CUR: ঘূৰাই "CUR";
    পক্ষ UVWASI_WHENCE_END: ঘূৰাই "END";
    স্ৱভাৱ: ঘূৰাই "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  অন্যথা
#    define WASI_TRACE(fmt, ...)
#  endif

স্থির অন্তঃস্থ
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    প্রতিজ্ঞা(context != রিক্ত);
    ঘূৰাই &context->uvwasi;
}

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_args_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(ৱর্ণ*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    যদি (context == রিক্ত) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    m3ApiCheckMem(argv, context->argc * পরিমাণ(uint32_t));

    বাবে (u32 i = 0; i < context->argc; ++i) {
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

    m3ApiCheckMem(argc, পরিমাণ(uvwasi_size_t));
    m3ApiCheckMem(argv_buf_size, পরিমাণ(uvwasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    যদি (context == রিক্ত) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_size_t buf_len = 0;
    বাবে (u32 i = 0; i < context->argc; ++i) {
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
    m3ApiGetArgMem(ৱর্ণ*, env_buf)

    ৱর্ণ**         environment;
    uvwasi_errno_t ret;
    uvwasi_size_t  env_গুনতি, env_buf_size;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_গুনতি, &env_buf_size);
    যদি (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_গুনতি * পরিমাণ(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_size);

    environment = calloc(env_গুনতি, পরিমাণ(ৱর্ণ*));
    যদি (environment == রিক্ত) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    যদি (ret != UVWASI_ESUCCESS) {
        মুক্ত(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_offset = m3ApiPtrToOffset(env_buf);

    বাবে (u32 i = 0; i < env_গুনতি; ++i) {
        uint32_t offset = environ_buf_offset +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    মুক্ত(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_size_t*, env_গুনতি)
    m3ApiGetArgMem(uvwasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_গুনতি, পরিমাণ(uvwasi_size_t));
    m3ApiCheckMem(env_buf_size, পরিমাণ(uvwasi_size_t));

    uvwasi_size_t গুনতি;
    uvwasi_size_t buf_size;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &গুনতি, &buf_size);

    m3ApiWriteMem32(env_গুনতি, গুনতি);
    m3ApiWriteMem32(env_buf_size, buf_size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ৱর্ণ*, path)
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

    যদি (ret != UVWASI_ESUCCESS) {
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

    যদি (ret != UVWASI_ESUCCESS) {
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

    যদি (ret != UVWASI_ESUCCESS) {
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

    m3ApiCheckMem(result, পরিমাণ(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    ৱিকল্প (wasi_whence) {
    পক্ষ 0: whence = UVWASI_WHENCE_SET; ভঙ্গ;
    পক্ষ 1: whence = UVWASI_WHENCE_CUR; ভঙ্গ;
    পক্ষ 2: whence = UVWASI_WHENCE_END; ভঙ্গ;
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

    m3ApiCheckMem(result, পরিমাণ(uvwasi_filesize_t));

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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, path)
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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArgMem(ৱর্ণ*, buf)
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, পরিমাণ(uvwasi_size_t));

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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, path)
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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, new_path)
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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, new_path)
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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, path)
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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_base)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, পরিমাণ(uvwasi_fd_t));

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
    m3ApiGetArgMem(ধ্রুৱ ৱর্ণ*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_path_filestat_get(_get_uvwasi_p(_ctx), fd, flags, path, path_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, path, stat.st_size);

    যদি (ret != UVWASI_ESUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * পরিমাণ(wasi_iovec_t));
    m3ApiCheckMem(nread, পরিমাণ(uvwasi_size_t));

#  যদি M3_HAS_VLA
    যদি (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  অন্যথা
    যদি (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    বাবে (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_size_t অঙ্ক_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &অঙ্ক_read);

    WASI_TRACE("fd:%d | nread:%d", fd, অঙ্ক_read);

    m3ApiWriteMem32(nread, অঙ্ক_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * পরিমাণ(wasi_iovec_t));
    m3ApiCheckMem(nread, পরিমাণ(uvwasi_size_t));

#  যদি M3_HAS_VLA
    যদি (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  অন্যথা
    যদি (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_size_t  অঙ্ক_read;
    uvwasi_errno_t ret;

    বাবে (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &অঙ্ক_read);

    WASI_TRACE("fd:%d | nread:%d", fd, অঙ্ক_read);

    m3ApiWriteMem32(nread, অঙ্ক_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * পরিমাণ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, পরিমাণ(uvwasi_size_t));

#  যদি M3_HAS_VLA
    যদি (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  অন্যথা
    যদি (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  অঙ্ক_written;
    uvwasi_errno_t ret;

    বাবে (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &অঙ্ক_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, অঙ্ক_written);

    m3ApiWriteMem32(nwritten, অঙ্ক_written);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * পরিমাণ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, পরিমাণ(uvwasi_size_t));

#  যদি M3_HAS_VLA
    যদি (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  অন্যথা
    যদি (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  অঙ্ক_written;
    uvwasi_errno_t ret;

    বাবে (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &অঙ্ক_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, অঙ্ক_written);

    m3ApiWriteMem32(nwritten, অঙ্ক_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(শূন্য*, buf)
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, পরিমাণ(uvwasi_size_t));

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

    m3ApiCheckMem(resolution, পরিমাণ(uvwasi_timestamp_t));

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

    m3ApiCheckMem(time, পরিমাণ(uvwasi_timestamp_t));

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
স্থির
শূন্য read_subscriptions (uvwasi_subscription_t* o_subs, ধ্রুৱ uint8_t* i_in,
                         uvwasi_size_t i_গুনতি)
{
    বাবে (uvwasi_size_t i = 0; i < i_গুনতি; ++i) {
        ধ্রুৱ uint8_t* s = i_in + (size_t)i * d_m3WasiSubscriptionSize;

        memset(&o_subs[i], 0, পরিমাণ(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        যদি (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } অন্যথা {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
স্থির
শূন্য write_event (uint8_t* o_rec, ধ্রুৱ uvwasi_event_t* i_event)
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
স্থির
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, ধ্রুৱ uvwasi_subscription_t* i_subs,
                                        uint8_t* o_out, uvwasi_size_t i_গুনতি,
                                        uvwasi_size_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_গুনতি ? m3_AllocArray(uvwasi_event_t, i_গুনতি) : রিক্ত;

    যদি (i_গুনতি and not events) {
        ঘূৰাই UVWASI_ENOMEM;
    }

    uvwasi_size_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_গুনতি, &written);

    যদি (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_গুনতি);

        বাবে (uvwasi_size_t i = 0; i < written; ++i) {
            write_event(o_out + (size_t)i * d_m3WasiEventSize, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    ঘূৰাই ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ধ্রুৱ uint8_t*, in)
    m3ApiGetArgMem(uint8_t*, out)
    m3ApiGetArg(uvwasi_size_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_size_t*, nevents)

    m3ApiCheckMem(in, (uint64_t)nsubscriptions * d_m3WasiSubscriptionSize);
    m3ApiCheckMem(out, (uint64_t)nsubscriptions * d_m3WasiEventSize);
    m3ApiCheckMem(nevents, পরিমাণ(uvwasi_size_t));

    uvwasi_subscription_t* subs = রিক্ত;

    যদি (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        যদি (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, in, nsubscriptions);
    }

    uvwasi_size_t গুনতি = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, out, nsubscriptions, &গুনতি);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, গুনতি);

    m3ApiWriteMem32(nevents, গুনতি);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, code)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    যদি (context) {
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


স্থির
M3Result SuppressLookupFailure (M3Result i_result)
{
    যদি (i_result == m3Err_functionLookupFailed) {
        ঘূৰাই m3Err_none;
    } অন্যথা {
        ঘূৰাই i_result;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    ঘূৰাই wasi_context;
}

স্থির ধ্রুৱ ৱর্ণ* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    রিক্ত,
};

স্থির uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_path = "/",  .real_path = "." },
    { .mapped_path = "./", .real_path = "." },
};

স্থির অন্তঃস্থ
শূন্য _m3_set_up_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = পরিমাণ(DEFAULT_PREOPENS) / পরিমাণ(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

স্থির
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    স্থির ধ্রুৱ ৱর্ণ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    বাবে (size_t i = 0; i < M3_গুনতি_OF(namespaces); i++) {
        ধ্রুৱ ৱর্ণ* wasi = namespaces[i];

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
    যদি (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 memoryIndex;

        যদি (m3_FindExportedMemory(module, "memory", &memoryIndex)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_BindImportMemory(module, "wasi_snapshot_preview1", memoryIndex));
    }

_catch:
    ঘূৰাই result;
}

M3Result m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    প্রতিজ্ঞা(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    যদি (ret != UVWASI_ESUCCESS) {
        ঘূৰাই "uvwasi_init failed";
    }

    ঘূৰাই m3Err_none;
}

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    ঘূৰাই m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ঘূৰাই _linkWASI(module, wasi);
}

শূন্য m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    ঘূৰাই m3_LinkWASIWithOptions(module, opts);
}

M3Result m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    যদি (রিক্ত == wasi_context) {
        M3Result res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        যদি (res) {
            ঘূৰাই res;
        }
    }

    ঘূৰাই _linkWASI(module, wasi_context);
}

#endif // d_m3HasUVWASI
