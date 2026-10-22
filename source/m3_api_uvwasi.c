//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#ଅନ୍ତର୍ଭାଵ "m3_api_wasi.h"

#ଅନ୍ତର୍ଭାଵ "m3_env.h"
#ଅନ୍ତର୍ଭାଵ "m3_exception.h"

#ଯଦି defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
ବାହ୍ୟ ଅକ୍ଷର** environ;
#  endif

ସ୍ଥିର m3_wasi_context_t* wasi_context = ଖାଲି;

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା wasi_iovec_t {
    uvwasi_size_t buf;
    uvwasi_size_t buf_len;
} wasi_iovec_t;

#  ଯଦି d_m3EnableWasiTracing

ଅଚଳ ଅକ୍ଷର* wasi_errno2str (uvwasi_errno_t err)
{
    // clang-format off
    ବାଛ (err) {
    ସ୍ଥିତି  0: ଫେରନ୍ତୁ "ESUCCESS";
    ସ୍ଥିତି  1: ଫେରନ୍ତୁ "E2BIG";
    ସ୍ଥିତି  2: ଫେରନ୍ତୁ "EACCES";
    ସ୍ଥିତି  3: ଫେରନ୍ତୁ "EADDRINUSE";
    ସ୍ଥିତି  4: ଫେରନ୍ତୁ "EADDRNOTAVAIL";
    ସ୍ଥିତି  5: ଫେରନ୍ତୁ "EAFNOSUPPORT";
    ସ୍ଥିତି  6: ଫେରନ୍ତୁ "EAGAIN";
    ସ୍ଥିତି  7: ଫେରନ୍ତୁ "EALREADY";
    ସ୍ଥିତି  8: ଫେରନ୍ତୁ "EBADF";
    ସ୍ଥିତି  9: ଫେରନ୍ତୁ "EBADMSG";
    ସ୍ଥିତି 10: ଫେରନ୍ତୁ "EBUSY";
    ସ୍ଥିତି 11: ଫେରନ୍ତୁ "ECANCELED";
    ସ୍ଥିତି 12: ଫେରନ୍ତୁ "ECHILD";
    ସ୍ଥିତି 13: ଫେରନ୍ତୁ "ECONNABORTED";
    ସ୍ଥିତି 14: ଫେରନ୍ତୁ "ECONNREFUSED";
    ସ୍ଥିତି 15: ଫେରନ୍ତୁ "ECONNRESET";
    ସ୍ଥିତି 16: ଫେରନ୍ତୁ "EDEADLK";
    ସ୍ଥିତି 17: ଫେରନ୍ତୁ "EDESTADDRREQ";
    ସ୍ଥିତି 18: ଫେରନ୍ତୁ "EDOM";
    ସ୍ଥିତି 19: ଫେରନ୍ତୁ "EDQUOT";
    ସ୍ଥିତି 20: ଫେରନ୍ତୁ "EEXIST";
    ସ୍ଥିତି 21: ଫେରନ୍ତୁ "EFAULT";
    ସ୍ଥିତି 22: ଫେରନ୍ତୁ "EFBIG";
    ସ୍ଥିତି 23: ଫେରନ୍ତୁ "EHOSTUNREACH";
    ସ୍ଥିତି 24: ଫେରନ୍ତୁ "EIDRM";
    ସ୍ଥିତି 25: ଫେରନ୍ତୁ "EILSEQ";
    ସ୍ଥିତି 26: ଫେରନ୍ତୁ "EINPROGRESS";
    ସ୍ଥିତି 27: ଫେରନ୍ତୁ "EINTR";
    ସ୍ଥିତି 28: ଫେରନ୍ତୁ "EINVAL";
    ସ୍ଥିତି 29: ଫେରନ୍ତୁ "EIO";
    ସ୍ଥିତି 30: ଫେରନ୍ତୁ "EISCONN";
    ସ୍ଥିତି 31: ଫେରନ୍ତୁ "EISDIR";
    ସ୍ଥିତି 32: ଫେରନ୍ତୁ "ELOOP";
    ସ୍ଥିତି 33: ଫେରନ୍ତୁ "EMFILE";
    ସ୍ଥିତି 34: ଫେରନ୍ତୁ "EMLINK";
    ସ୍ଥିତି 35: ଫେରନ୍ତୁ "EMSGSIZE";
    ସ୍ଥିତି 36: ଫେରନ୍ତୁ "EMULTIHOP";
    ସ୍ଥିତି 37: ଫେରନ୍ତୁ "ENAMETOOLONG";
    ସ୍ଥିତି 38: ଫେରନ୍ତୁ "ENETDOWN";
    ସ୍ଥିତି 39: ଫେରନ୍ତୁ "ENETRESET";
    ସ୍ଥିତି 40: ଫେରନ୍ତୁ "ENETUNREACH";
    ସ୍ଥିତି 41: ଫେରନ୍ତୁ "ENFILE";
    ସ୍ଥିତି 42: ଫେରନ୍ତୁ "ENOBUFS";
    ସ୍ଥିତି 43: ଫେରନ୍ତୁ "ENODEV";
    ସ୍ଥିତି 44: ଫେରନ୍ତୁ "ENOENT";
    ସ୍ଥିତି 45: ଫେରନ୍ତୁ "ENOEXEC";
    ସ୍ଥିତି 46: ଫେରନ୍ତୁ "ENOLCK";
    ସ୍ଥିତି 47: ଫେରନ୍ତୁ "ENOLINK";
    ସ୍ଥିତି 48: ଫେରନ୍ତୁ "ENOMEM";
    ସ୍ଥିତି 49: ଫେରନ୍ତୁ "ENOMSG";
    ସ୍ଥିତି 50: ଫେରନ୍ତୁ "ENOPROTOOPT";
    ସ୍ଥିତି 51: ଫେରନ୍ତୁ "ENOSPC";
    ସ୍ଥିତି 52: ଫେରନ୍ତୁ "ENOSYS";
    ସ୍ଥିତି 53: ଫେରନ୍ତୁ "ENOTCONN";
    ସ୍ଥିତି 54: ଫେରନ୍ତୁ "ENOTDIR";
    ସ୍ଥିତି 55: ଫେରନ୍ତୁ "ENOTEMPTY";
    ସ୍ଥିତି 56: ଫେରନ୍ତୁ "ENOTRECOVERABLE";
    ସ୍ଥିତି 57: ଫେରନ୍ତୁ "ENOTSOCK";
    ସ୍ଥିତି 58: ଫେରନ୍ତୁ "ENOTSUP";
    ସ୍ଥିତି 59: ଫେରନ୍ତୁ "ENOTTY";
    ସ୍ଥିତି 60: ଫେରନ୍ତୁ "ENXIO";
    ସ୍ଥିତି 61: ଫେରନ୍ତୁ "EOVERFLOW";
    ସ୍ଥିତି 62: ଫେରନ୍ତୁ "EOWNERDEAD";
    ସ୍ଥିତି 63: ଫେରନ୍ତୁ "EPERM";
    ସ୍ଥିତି 64: ଫେରନ୍ତୁ "EPIPE";
    ସ୍ଥିତି 65: ଫେରନ୍ତୁ "EPROTO";
    ସ୍ଥିତି 66: ଫେରନ୍ତୁ "EPROTONOSUPPORT";
    ସ୍ଥିତି 67: ଫେରନ୍ତୁ "EPROTOTYPE";
    ସ୍ଥିତି 68: ଫେରନ୍ତୁ "ERANGE";
    ସ୍ଥିତି 69: ଫେରନ୍ତୁ "EROFS";
    ସ୍ଥିତି 70: ଫେରନ୍ତୁ "ESPIPE";
    ସ୍ଥିତି 71: ଫେରନ୍ତୁ "ESRCH";
    ସ୍ଥିତି 72: ଫେରନ୍ତୁ "ESTALE";
    ସ୍ଥିତି 73: ଫେରନ୍ତୁ "ETIMEDOUT";
    ସ୍ଥିତି 74: ଫେରନ୍ତୁ "ETXTBSY";
    ସ୍ଥିତି 75: ଫେରନ୍ତୁ "EXDEV";
    ସ୍ଥିତି 76: ଫେରନ୍ତୁ "ENOTCAPABLE";
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "<unknown>";
    }
    // clang-format on
}

ଅଚଳ ଅକ୍ଷର* wasi_whence2str (uvwasi_whence_t whence)
{
    ବାଛ (whence) {
    ସ୍ଥିତି UVWASI_WHENCE_SET: ଫେରନ୍ତୁ "SET";
    ସ୍ଥିତି UVWASI_WHENCE_CUR: ଫେରନ୍ତୁ "CUR";
    ସ୍ଥିତି UVWASI_WHENCE_END: ଫେରନ୍ତୁ "END";
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  ଅନ୍ୟଥା
#    define WASI_TRACE(fmt, ...)
#  endif

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    ପ୍ରତିଜ୍ଞା(context != ଖାଲି);
    ଫେରନ୍ତୁ &context->uvwasi;
}

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_args_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(ଅକ୍ଷର*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ଯଦି (context == ଖାଲି) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    m3ApiCheckMem(argv, context->argc * ଆକାର(uint32_t));

    ପାଇଁ (u32 i = 0; i < context->argc; ++i) {
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

    m3ApiCheckMem(argc, ଆକାର(uvwasi_size_t));
    m3ApiCheckMem(argv_buf_size, ଆକାର(uvwasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ଯଦି (context == ଖାଲି) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_size_t buf_len = 0;
    ପାଇଁ (u32 i = 0; i < context->argc; ++i) {
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
    m3ApiGetArgMem(ଅକ୍ଷର*, env_buf)

    ଅକ୍ଷର**         environment;
    uvwasi_errno_t ret;
    uvwasi_size_t  env_ଗଣନା, env_buf_size;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_ଗଣନା, &env_buf_size);
    ଯଦି (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_ଗଣନା * ଆକାର(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_size);

    environment = calloc(env_ଗଣନା, ଆକାର(ଅକ୍ଷର*));
    ଯଦି (environment == ଖାଲି) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    ଯଦି (ret != UVWASI_ESUCCESS) {
        ମୁକ୍ତ(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_offset = m3ApiPtrToOffset(env_buf);

    ପାଇଁ (u32 i = 0; i < env_ଗଣନା; ++i) {
        uint32_t offset = environ_buf_offset +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    ମୁକ୍ତ(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_size_t*, env_ଗଣନା)
    m3ApiGetArgMem(uvwasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_ଗଣନା, ଆକାର(uvwasi_size_t));
    m3ApiCheckMem(env_buf_size, ଆକାର(uvwasi_size_t));

    uvwasi_size_t ଗଣନା_2;
    uvwasi_size_t buf_size;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &ଗଣନା_2, &buf_size);

    m3ApiWriteMem32(env_ଗଣନା, ଗଣନା_2);
    m3ApiWriteMem32(env_buf_size, buf_size);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ଅକ୍ଷର*, path)
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

    ଯଦି (ret != UVWASI_ESUCCESS) {
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

    ଯଦି (ret != UVWASI_ESUCCESS) {
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

    ଯଦି (ret != UVWASI_ESUCCESS) {
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

    m3ApiCheckMem(result, ଆକାର(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    ବାଛ (wasi_whence) {
    ସ୍ଥିତି 0: whence = UVWASI_WHENCE_SET; ଭାଙ୍ଗ;
    ସ୍ଥିତି 1: whence = UVWASI_WHENCE_CUR; ଭାଙ୍ଗ;
    ସ୍ଥିତି 2: whence = UVWASI_WHENCE_END; ଭାଙ୍ଗ;
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

    m3ApiCheckMem(result, ଆକାର(uvwasi_filesize_t));

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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, path)
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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArgMem(ଅକ୍ଷର*, buf)
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ଆକାର(uvwasi_size_t));

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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, path)
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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, new_path)
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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, old_path)
    m3ApiGetArg(uvwasi_size_t, old_path_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, new_path)
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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, path)
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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, path)
    m3ApiGetArg(uvwasi_size_t, path_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_base)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, ଆକାର(uvwasi_fd_t));

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
    m3ApiGetArgMem(ଅଚଳ ଅକ୍ଷର*, path)
    m3ApiGetArg(uint32_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_path_filestat_get(_get_uvwasi_p(_ctx), fd, flags, path, path_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, path, stat.st_size);

    ଯଦି (ret != UVWASI_ESUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ଆକାର(wasi_iovec_t));
    m3ApiCheckMem(nread, ଆକାର(uvwasi_size_t));

#  ଯଦି M3_HAS_VLA
    ଯଦି (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ଅନ୍ୟଥା
    ଯଦି (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    ପାଇଁ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_size_t ଅଙ୍କ_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &ଅଙ୍କ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, ଅଙ୍କ_read);

    m3ApiWriteMem32(nread, ଅଙ୍କ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * ଆକାର(wasi_iovec_t));
    m3ApiCheckMem(nread, ଆକାର(uvwasi_size_t));

#  ଯଦି M3_HAS_VLA
    ଯଦି (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  ଅନ୍ୟଥା
    ଯଦି (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_size_t  ଅଙ୍କ_read;
    uvwasi_errno_t ret;

    ପାଇଁ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &ଅଙ୍କ_read);

    WASI_TRACE("fd:%d | nread:%d", fd, ଅଙ୍କ_read);

    m3ApiWriteMem32(nread, ଅଙ୍କ_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_size_t, iovs_len)
    m3ApiGetArgMem(uvwasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ଆକାର(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ଆକାର(uvwasi_size_t));

#  ଯଦି M3_HAS_VLA
    ଯଦି (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ଅନ୍ୟଥା
    ଯଦି (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  ଅଙ୍କ_written;
    uvwasi_errno_t ret;

    ପାଇଁ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &ଅଙ୍କ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, ଅଙ୍କ_written);

    m3ApiWriteMem32(nwritten, ଅଙ୍କ_written);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ଆକାର(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ଆକାର(uvwasi_size_t));

#  ଯଦି M3_HAS_VLA
    ଯଦି (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  ଅନ୍ୟଥା
    ଯଦି (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_size_t  ଅଙ୍କ_written;
    uvwasi_errno_t ret;

    ପାଇଁ (uvwasi_size_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3ApiOffsetToPtr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &ଅଙ୍କ_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, ଅଙ୍କ_written);

    m3ApiWriteMem32(nwritten, ଅଙ୍କ_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(ଶୂନ୍ୟ*, buf)
    m3ApiGetArg(uvwasi_size_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ଆକାର(uvwasi_size_t));

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

    m3ApiCheckMem(resolution, ଆକାର(uvwasi_timestamp_t));

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

    m3ApiCheckMem(time, ଆକାର(uvwasi_timestamp_t));

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
ସ୍ଥିର
ଶୂନ୍ୟ read_subscriptions (uvwasi_subscription_t* o_subs, ଅଚଳ uint8_t* i_in,
                         uvwasi_size_t i_ଗଣନା)
{
    ପାଇଁ (uvwasi_size_t i = 0; i < i_ଗଣନା; ++i) {
        ଅଚଳ uint8_t* s = i_in + (size_t)i * d_m3WasiSubscriptionSize;

        memset(&o_subs[i], 0, ଆକାର(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        ଯଦି (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } ଅନ୍ୟଥା {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ସ୍ଥିର
ଶୂନ୍ୟ write_event (uint8_t* o_rec, ଅଚଳ uvwasi_event_t* i_event)
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
ସ୍ଥିର
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, ଅଚଳ uvwasi_subscription_t* i_subs,
                                        uint8_t* o_out, uvwasi_size_t i_ଗଣନା,
                                        uvwasi_size_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_ଗଣନା ? m3_AllocArray(uvwasi_event_t, i_ଗଣନା) : ଖାଲି;

    ଯଦି (i_ଗଣନା and not events) {
        ଫେରନ୍ତୁ UVWASI_ENOMEM;
    }

    uvwasi_size_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_ଗଣନା, &written);

    ଯଦି (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_ଗଣନା);

        ପାଇଁ (uvwasi_size_t i = 0; i < written; ++i) {
            write_event(o_out + (size_t)i * d_m3WasiEventSize, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    ଫେରନ୍ତୁ ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ଅଚଳ uint8_t*, in)
    m3ApiGetArgMem(uint8_t*, out)
    m3ApiGetArg(uvwasi_size_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_size_t*, nevents)

    m3ApiCheckMem(in, (uint64_t)nsubscriptions * d_m3WasiSubscriptionSize);
    m3ApiCheckMem(out, (uint64_t)nsubscriptions * d_m3WasiEventSize);
    m3ApiCheckMem(nevents, ଆକାର(uvwasi_size_t));

    uvwasi_subscription_t* subs = ଖାଲି;

    ଯଦି (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        ଯଦି (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, in, nsubscriptions);
    }

    uvwasi_size_t ଗଣନା_2 = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, out, nsubscriptions, &ଗଣନା_2);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, ଗଣନା_2);

    m3ApiWriteMem32(nevents, ଗଣନା_2);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, code)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ଯଦି (context) {
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


ସ୍ଥିର
M3Result SuppressLookupFailure (M3Result i_result)
{
    ଯଦି (i_result == m3Err_functionLookupFailed) {
        ଫେରନ୍ତୁ m3Err_none;
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ i_result;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    ଫେରନ୍ତୁ wasi_context;
}

ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ଖାଲି,
};

ସ୍ଥିର uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_path = "/",  .real_path = "." },
    { .mapped_path = "./", .real_path = "." },
};

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
ଶୂନ୍ୟ _m3_set_up_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = ଆକାର(DEFAULT_PREOPENS) / ଆକାର(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

ସ୍ଥିର
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ପାଇଁ (size_t i = 0; i < M3_ଗଣନା_OF(namespaces); i++) {
        ଅଚଳ ଅକ୍ଷର* wasi = namespaces[i];

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
    ଯଦି (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 memoryIndex;

        ଯଦି (m3_FindExportedMemory(module, "memory", &memoryIndex)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_BindImportMemory(module, "wasi_snapshot_preview1", memoryIndex));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

M3Result m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    ପ୍ରତିଜ୍ଞା(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    ଯଦି (ret != UVWASI_ESUCCESS) {
        ଫେରନ୍ତୁ "uvwasi_init failed";
    }

    ଫେରନ୍ତୁ m3Err_none;
}

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    ଫେରନ୍ତୁ m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ଫେରନ୍ତୁ _linkWASI(module, wasi);
}

ଶୂନ୍ୟ m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    _m3_set_up_default_uvwasi_opts(&opts);

    ଫେରନ୍ତୁ m3_LinkWASIWithOptions(module, opts);
}

M3Result m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    ଯଦି (ଖାଲି == wasi_context) {
        M3Result res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        ଯଦି (res) {
            ଫେରନ୍ତୁ res;
        }
    }

    ଫେରନ୍ତୁ _linkWASI(module, wasi_context);
}

#endif // d_m3HasUVWASI
