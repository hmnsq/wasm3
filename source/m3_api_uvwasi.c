//
//  m3_api_uvwasi.c
//
//  Created by Colin J. Ihrig on 4/20/20.
//  Copyright © 2020 Colin J. Ihrig, Volodymyr Shymanskyy. All rights reserved.
//

#అన్తర్భావ "m3_api_wasi.h"

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_exception.h"

#ఒకవేళ defined(d_m3HasUVWASI)

#  include <stdio.h>
#  include <string.h>

#  ifdef __APPLE__
#    include <crt_externs.h>
#    define environ (*_NSGetEnviron())
#  elif !defined(_MSC_VER)
బాహ్య అక్షరం** environ;
#  endif

స్థిర m3_wasi_context_t* wasi_context = ఖాళీ;

రకనిర్వచనం నిర్మాణం wasi_iovec_t {
    uvwasi_కొలత_t buf;
    uvwasi_కొలత_t buf_len;
} wasi_iovec_t;

#  ఒకవేళ d_m3EnableWasiTracing

స్థిరరాశి అక్షరం* wasi_errno2str (uvwasi_errno_t err)
{
    // clang-format off
    ఎంపిక (err) {
    స్థితి  0: తిరిగి "ESUCCESS";
    స్థితి  1: తిరిగి "E2BIG";
    స్థితి  2: తిరిగి "EACCES";
    స్థితి  3: తిరిగి "EADDRINUSE";
    స్థితి  4: తిరిగి "EADDRNOTAVAIL";
    స్థితి  5: తిరిగి "EAFNOSUPPORT";
    స్థితి  6: తిరిగి "EAGAIN";
    స్థితి  7: తిరిగి "EALREADY";
    స్థితి  8: తిరిగి "EBADF";
    స్థితి  9: తిరిగి "EBADMSG";
    స్థితి 10: తిరిగి "EBUSY";
    స్థితి 11: తిరిగి "ECANCELED";
    స్థితి 12: తిరిగి "ECHILD";
    స్థితి 13: తిరిగి "ECONNABORTED";
    స్థితి 14: తిరిగి "ECONNREFUSED";
    స్థితి 15: తిరిగి "ECONNRESET";
    స్థితి 16: తిరిగి "EDEADLK";
    స్థితి 17: తిరిగి "EDESTADDRREQ";
    స్థితి 18: తిరిగి "EDOM";
    స్థితి 19: తిరిగి "EDQUOT";
    స్థితి 20: తిరిగి "EEXIST";
    స్థితి 21: తిరిగి "EFAULT";
    స్థితి 22: తిరిగి "EFBIG";
    స్థితి 23: తిరిగి "EHOSTUNREACH";
    స్థితి 24: తిరిగి "EIDRM";
    స్థితి 25: తిరిగి "EILSEQ";
    స్థితి 26: తిరిగి "EINPROGRESS";
    స్థితి 27: తిరిగి "EINTR";
    స్థితి 28: తిరిగి "EINVAL";
    స్థితి 29: తిరిగి "EIO";
    స్థితి 30: తిరిగి "EISCONN";
    స్థితి 31: తిరిగి "EISDIR";
    స్థితి 32: తిరిగి "ELOOP";
    స్థితి 33: తిరిగి "EMFILE";
    స్థితి 34: తిరిగి "EMLINK";
    స్థితి 35: తిరిగి "EMSGSIZE";
    స్థితి 36: తిరిగి "EMULTIHOP";
    స్థితి 37: తిరిగి "ENAMETOOLONG";
    స్థితి 38: తిరిగి "ENETDOWN";
    స్థితి 39: తిరిగి "ENETRESET";
    స్థితి 40: తిరిగి "ENETUNREACH";
    స్థితి 41: తిరిగి "ENFILE";
    స్థితి 42: తిరిగి "ENOBUFS";
    స్థితి 43: తిరిగి "ENODEV";
    స్థితి 44: తిరిగి "ENOENT";
    స్థితి 45: తిరిగి "ENOEXEC";
    స్థితి 46: తిరిగి "ENOLCK";
    స్థితి 47: తిరిగి "ENOLINK";
    స్థితి 48: తిరిగి "ENOMEM";
    స్థితి 49: తిరిగి "ENOMSG";
    స్థితి 50: తిరిగి "ENOPROTOOPT";
    స్థితి 51: తిరిగి "ENOSPC";
    స్థితి 52: తిరిగి "ENOSYS";
    స్థితి 53: తిరిగి "ENOTCONN";
    స్థితి 54: తిరిగి "ENOTDIR";
    స్థితి 55: తిరిగి "ENOTEMPTY";
    స్థితి 56: తిరిగి "ENOTRECOVERABLE";
    స్థితి 57: తిరిగి "ENOTSOCK";
    స్థితి 58: తిరిగి "ENOTSUP";
    స్థితి 59: తిరిగి "ENOTTY";
    స్థితి 60: తిరిగి "ENXIO";
    స్థితి 61: తిరిగి "EOVERFLOW";
    స్థితి 62: తిరిగి "EOWNERDEAD";
    స్థితి 63: తిరిగి "EPERM";
    స్థితి 64: తిరిగి "EPIPE";
    స్థితి 65: తిరిగి "EPROTO";
    స్థితి 66: తిరిగి "EPROTONOSUPPORT";
    స్థితి 67: తిరిగి "EPROTOTYPE";
    స్థితి 68: తిరిగి "ERANGE";
    స్థితి 69: తిరిగి "EROFS";
    స్థితి 70: తిరిగి "ESPIPE";
    స్థితి 71: తిరిగి "ESRCH";
    స్థితి 72: తిరిగి "ESTALE";
    స్థితి 73: తిరిగి "ETIMEDOUT";
    స్థితి 74: తిరిగి "ETXTBSY";
    స్థితి 75: తిరిగి "EXDEV";
    స్థితి 76: తిరిగి "ENOTCAPABLE";
    అప్రమేయ: తిరిగి "<unknown>";
    }
    // clang-format on
}

స్థిరరాశి అక్షరం* wasi_whence2str (uvwasi_whence_t whence)
{
    ఎంపిక (whence) {
    స్థితి UVWASI_WHENCE_సమితి: తిరిగి "SET";
    స్థితి UVWASI_WHENCE_CUR: తిరిగి "CUR";
    స్థితి UVWASI_WHENCE_చివర: తిరిగి "END";
    అప్రమేయ: తిరిగి "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  లేకపోతే
#    define WASI_TRACE(fmt, ...)
#  endif

స్థిర అంతఃవరుస
uvwasi_t* _get_uvwasi_p (IM3ImportContext _ctx)
{
    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);
    ప్రతిజ్ఞా(context != ఖాళీ);
    తిరిగి &context->uvwasi;
}

/*
 * WASI API implementation
 */

m3ApiRawFunction(m3_wasi_p1_వాదనలు_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(అక్షరం*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ఒకవేళ (context == ఖాళీ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    m3ApiCheckMem(argv, context->argc * పరిమాణం(uint32_t));

    కొరకు (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_గమ్యం_స్థానభ్రంశం(argv_buf));

        size_t len = అక్షరశ్రేణి_పొడవు(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        జ్ఞాపకం_నకలు(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_వాదనలు_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_కొలత_t*, argc)
    m3ApiGetArgMem(uvwasi_కొలత_t*, argv_buf_కొలత)

    m3ApiCheckMem(argc, పరిమాణం(uvwasi_కొలత_t));
    m3ApiCheckMem(argv_buf_కొలత, పరిమాణం(uvwasi_కొలత_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ఒకవేళ (context == ఖాళీ) {
        m3ApiReturn(UVWASI_EINVAL);
    }

    uvwasi_కొలత_t buf_len = 0;
    కొరకు (u32 i = 0; i < context->argc; ++i) {
        buf_len += (uvwasi_కొలత_t)అక్షరశ్రేణి_పొడవు(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_కొలత, buf_len);

    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(అక్షరం*, env_buf)

    అక్షరం**         environment;
    uvwasi_errno_t ret;
    uvwasi_కొలత_t  env_లెక్క, env_buf_కొలత;

    ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &env_లెక్క, &env_buf_కొలత);
    ఒకవేళ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_లెక్క * పరిమాణం(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_కొలత);

    environment = సున్నా_జ్ఞాపకం_ఇవ్వు(env_లెక్క, పరిమాణం(అక్షరం*));
    ఒకవేళ (environment == ఖాళీ) {
        m3ApiReturn(UVWASI_ENOMEM);
    }

    ret = uvwasi_environ_get(_get_uvwasi_p(_ctx), environment, env_buf);
    ఒకవేళ (ret != UVWASI_ESUCCESS) {
        విడుదల(environment);
        m3ApiReturn(ret);
    }

    uint32_t environ_buf_స్థానభ్రంశం = m3_Api_Ptr_గమ్యం_స్థానభ్రంశం(env_buf);

    కొరకు (u32 i = 0; i < env_లెక్క; ++i) {
        uint32_t offset = environ_buf_స్థానభ్రంశం +
                          (uint32_t)(environment[i] - environment[0]);
        m3ApiWriteMem32(&env[i], offset);
    }

    విడుదల(environment);
    m3ApiReturn(UVWASI_ESUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uvwasi_కొలత_t*, env_లెక్క)
    m3ApiGetArgMem(uvwasi_కొలత_t*, env_buf_కొలత)

    m3ApiCheckMem(env_లెక్క, పరిమాణం(uvwasi_కొలత_t));
    m3ApiCheckMem(env_buf_కొలత, పరిమాణం(uvwasi_కొలత_t));

    uvwasi_కొలత_t లెక్క;
    uvwasi_కొలత_t buf_కొలత;

    uvwasi_errno_t ret = uvwasi_environ_sizes_get(_get_uvwasi_p(_ctx), &లెక్క, &buf_కొలత);

    m3ApiWriteMem32(env_లెక్క, లెక్క);
    m3ApiWriteMem32(env_buf_కొలత, buf_కొలత);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(అక్షరం*, మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    uvwasi_errno_t ret = uvwasi_fd_prestat_dir_name(_get_uvwasi_p(_ctx), fd, మార్గం, మార్గం_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, మార్గం_len, మార్గం);

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

    ఒకవేళ (ret != UVWASI_ESUCCESS) {
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

    ఒకవేళ (ret != UVWASI_ESUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 24);
    m3ApiWriteMem8(buf + 0, stat.fs_filetype);
    m3ApiWriteMem16(buf + 2, stat.fs_flags);
    m3ApiWriteMem64(buf + 8, stat.fs_rights_పునాది);
    m3ApiWriteMem64(buf + 16, stat.fs_rights_inheriting);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_సమితి_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_fdflags_t, flags)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_సమితి_flags(_get_uvwasi_p(_ctx), fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_సమితి_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_rights_t, rights_పునాది)
    m3ApiGetArg(uvwasi_rights_t, rights_inheriting)

    uvwasi_errno_t ret = uvwasi_fd_fdstat_సమితి_rights(_get_uvwasi_p(_ctx), fd, rights_పునాది, rights_inheriting);

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_పునాది, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_సమితి_కొలత)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, కొలత)

    uvwasi_errno_t ret = uvwasi_fd_filestat_సమితి_కొలత(_get_uvwasi_p(_ctx), fd, కొలత);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, కొలత);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_సమితి_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_timestamp_t, atim)
    m3ApiGetArg(uvwasi_timestamp_t, mtim)
    m3ApiGetArg(uvwasi_fstflags_t, fst_flags)

    uvwasi_errno_t ret = uvwasi_fd_filestat_సమితి_times(_get_uvwasi_p(_ctx), fd, atim, mtim, fst_flags);

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

    ఒకవేళ (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArgMem(uvwasi_filesize_t*, ఫలితం)

    m3ApiCheckMem(ఫలితం, పరిమాణం(uvwasi_filesize_t));

    uvwasi_whence_t whence = -1;
    ఎంపిక (wasi_whence) {
    స్థితి 0: whence = UVWASI_WHENCE_సమితి; ఆపు;
    స్థితి 1: whence = UVWASI_WHENCE_CUR; ఆపు;
    స్థితి 2: whence = UVWASI_WHENCE_చివర; ఆపు;
    }

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_seek(_get_uvwasi_p(_ctx), fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    m3ApiWriteMem64(ఫలితం, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, from)
    m3ApiGetArg(uvwasi_fd_t, గమ్యం)

    uvwasi_errno_t ret = uvwasi_fd_renumber(_get_uvwasi_p(_ctx), from, గమ్యం);

    WASI_TRACE("from:%d, to:%d", from, గమ్యం);

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
    m3ApiGetArgMem(uvwasi_filesize_t*, ఫలితం)

    m3ApiCheckMem(ఫలితం, పరిమాణం(uvwasi_filesize_t));

    uvwasi_filesize_t pos;
    uvwasi_errno_t    ret = uvwasi_fd_tell(_get_uvwasi_p(_ctx), fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    m3ApiWriteMem64(ఫలితం, pos);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    uvwasi_errno_t ret = uvwasi_మార్గం_create_directory(_get_uvwasi_p(_ctx), fd, మార్గం, మార్గం_len);

    WASI_TRACE("fd:%d, path:%s", fd, మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, మార్గం_len)
    m3ApiGetArgMem(అక్షరం*, buf)
    m3ApiGetArg(uvwasi_కొలత_t, buf_len)
    m3ApiGetArgMem(uvwasi_కొలత_t*, bufused)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, పరిమాణం(uvwasi_కొలత_t));

    uvwasi_కొలత_t uvbufused;

    uvwasi_errno_t ret = uvwasi_మార్గం_readlink(_get_uvwasi_p(_ctx), fd, మార్గం, మార్గం_len, buf, buf_len, &uvbufused);

    WASI_TRACE("fd:%d, path:%s | buf:%s, bufused:%d", fd, మార్గం, buf, uvbufused);

    m3ApiWriteMem32(bufused, uvbufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    uvwasi_errno_t ret = uvwasi_మార్గం_remove_directory(_get_uvwasi_p(_ctx), fd, మార్గం, మార్గం_len);

    WASI_TRACE("fd:%d, path:%s", fd, మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, old_fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, old_మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, old_మార్గం_len)
    m3ApiGetArg(uvwasi_fd_t, new_fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, new_మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, new_మార్గం_len)

    m3ApiCheckMem(old_మార్గం, old_మార్గం_len);
    m3ApiCheckMem(new_మార్గం, new_మార్గం_len);

    uvwasi_errno_t ret = uvwasi_మార్గం_rename(_get_uvwasi_p(_ctx), old_fd, old_మార్గం, old_మార్గం_len,
                                            new_fd, new_మార్గం, new_మార్గం_len);

    WASI_TRACE("old_fd:%d, old_path:%s, new_fd:%d, new_path:%s", old_fd, old_మార్గం, new_fd, new_మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, old_మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, old_మార్గం_len)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, new_మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, new_మార్గం_len)

    m3ApiCheckMem(old_మార్గం, old_మార్గం_len);
    m3ApiCheckMem(new_మార్గం, new_మార్గం_len);

    uvwasi_errno_t ret = uvwasi_మార్గం_symlink(_get_uvwasi_p(_ctx), old_మార్గం, old_మార్గం_len,
                                             fd, new_మార్గం, new_మార్గం_len);

    WASI_TRACE("old_path:%s, fd:%d, new_path:%s", old_మార్గం, fd, new_మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    uvwasi_errno_t ret = uvwasi_మార్గం_unlink_file(_get_uvwasi_p(_ctx), fd, మార్గం, మార్గం_len);

    WASI_TRACE("fd:%d, path:%s", fd, మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, dirfd)
    m3ApiGetArg(uvwasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(uvwasi_కొలత_t, మార్గం_len)
    m3ApiGetArg(uvwasi_oflags_t, oflags)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_పునాది)
    m3ApiGetArg(uvwasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(uvwasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(uvwasi_fd_t*, fd)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(fd, పరిమాణం(uvwasi_fd_t));

    uvwasi_fd_t uvfd;

    uvwasi_errno_t ret = uvwasi_మార్గం_open(_get_uvwasi_p(_ctx),
                                          dirfd,
                                          dirflags,
                                          మార్గం,
                                          మార్గం_len,
                                          oflags,
                                          fs_rights_పునాది,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          &uvfd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%s, oflags:0x%x, fs_flags:0x%x | fd:%d", dirfd, dirflags, మార్గం, oflags, fs_flags, uvfd);

    m3ApiWriteMem32(fd, uvfd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_lookupflags_t, flags)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(uint32_t, మార్గం_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    uvwasi_filestat_t stat;

    uvwasi_errno_t ret = uvwasi_మార్గం_filestat_get(_get_uvwasi_p(_ctx), fd, flags, మార్గం, మార్గం_len, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%s | fs.size:%" PRIu64, fd, flags, మార్గం, stat.st_size);

    ఒకవేళ (ret != UVWASI_ESUCCESS) {
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
    m3ApiGetArg(uvwasi_కొలత_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_కొలత_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nread, పరిమాణం(uvwasi_కొలత_t));

#  ఒకవేళ M3_HAS_VLA
    ఒకవేళ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  లేకపోతే
    ఒకవేళ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif

    కొరకు (uvwasi_కొలత_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_pread fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    uvwasi_కొలత_t అంకె_read;

    uvwasi_errno_t ret = uvwasi_fd_pread(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &అంకె_read);

    WASI_TRACE("fd:%d | nread:%d", fd, అంకె_read);

    m3ApiWriteMem32(nread, అంకె_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_కొలత_t, iovs_len)
    m3ApiGetArgMem(uvwasi_కొలత_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nread, పరిమాణం(uvwasi_కొలత_t));

#  ఒకవేళ M3_HAS_VLA
    ఒకవేళ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[iovs_len];
#  లేకపోతే
    ఒకవేళ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_iovec_t iovs[32];
#  endif
    uvwasi_కొలత_t  అంకె_read;
    uvwasi_errno_t ret;

    కొరకు (uvwasi_కొలత_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
        //fprintf(stderr, "> fd_read fd:%d iov%d.len:%d\n", fd, i, iovs[i].buf_len);
    }

    ret = uvwasi_fd_read(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &అంకె_read);

    WASI_TRACE("fd:%d | nread:%d", fd, అంకె_read);

    m3ApiWriteMem32(nread, అంకె_read);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_కొలత_t, iovs_len)
    m3ApiGetArgMem(uvwasi_కొలత_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nwritten, పరిమాణం(uvwasi_కొలత_t));

#  ఒకవేళ M3_HAS_VLA
    ఒకవేళ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  లేకపోతే
    ఒకవేళ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_కొలత_t  అంకె_written;
    uvwasi_errno_t ret;

    కొరకు (uvwasi_కొలత_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_write(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, &అంకె_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, అంకె_written);

    m3ApiWriteMem32(nwritten, అంకె_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_pwrite)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(uvwasi_కొలత_t, iovs_len)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArgMem(uvwasi_కొలత_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nwritten, పరిమాణం(uvwasi_కొలత_t));

#  ఒకవేళ M3_HAS_VLA
    ఒకవేళ (iovs_len > 128) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[iovs_len];
#  లేకపోతే
    ఒకవేళ (iovs_len > 32) {
        m3ApiReturn(UVWASI_EINVAL);
    }
    uvwasi_ciovec_t iovs[32];
#  endif
    uvwasi_కొలత_t  అంకె_written;
    uvwasi_errno_t ret;

    కొరకు (uvwasi_కొలత_t i = 0; i < iovs_len; ++i) {
        iovs[i].buf     = m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(m3ApiReadMem32(&wasi_iovs[i].buf));
        iovs[i].buf_len = m3ApiReadMem32(&wasi_iovs[i].buf_len);
        m3ApiCheckMem(iovs[i].buf, iovs[i].buf_len);
    }

    ret = uvwasi_fd_pwrite(_get_uvwasi_p(_ctx), fd, iovs, iovs_len, offset, &అంకె_written);

    WASI_TRACE("fd:%d | nwritten:%d", fd, అంకె_written);

    m3ApiWriteMem32(nwritten, అంకె_written);
    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArgMem(శూన్య*, buf)
    m3ApiGetArg(uvwasi_కొలత_t, buf_len)
    m3ApiGetArg(uvwasi_dircookie_t, cookie)
    m3ApiGetArgMem(uvwasi_కొలత_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, పరిమాణం(uvwasi_కొలత_t));

    uvwasi_కొలత_t  uvbufused;
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
    m3ApiGetArg(uvwasi_filesize_t, నిడివి)
    m3ApiGetArg(uvwasi_advice_t, advice)

    uvwasi_errno_t ret = uvwasi_fd_advise(_get_uvwasi_p(_ctx), fd, offset, నిడివి, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, నిడివి, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_fd_t, fd)
    m3ApiGetArg(uvwasi_filesize_t, offset)
    m3ApiGetArg(uvwasi_filesize_t, నిడివి)

    uvwasi_errno_t ret = uvwasi_fd_allocate(_get_uvwasi_p(_ctx), fd, offset, నిడివి);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, నిడివి);

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
    m3ApiGetArg(uvwasi_కొలత_t, buf_len)

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

    m3ApiCheckMem(resolution, పరిమాణం(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_res_get(_get_uvwasi_p(_ctx), wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    m3ApiWriteMem64(resolution, t);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_సమయం_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(uvwasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(uvwasi_timestamp_t, precision)
    m3ApiGetArgMem(uvwasi_timestamp_t*, time)

    m3ApiCheckMem(time, పరిమాణం(uvwasi_timestamp_t));

    uvwasi_timestamp_t t;
    uvwasi_errno_t     ret = uvwasi_clock_సమయం_get(_get_uvwasi_p(_ctx), wasi_clk_id, precision, &t);

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
#  define d_m3_Wasi_Subscription_కొలత            48

#  define d_m3_Wasi_Event_కొలత                   32

// Reads the guest's subscription list into host structs. The caller has already
// bounds-checked i_in for i_count records. 48 bytes a record: userdata 0, type 8,
// fd 16, and a clock's fields from 16.
స్థిర
శూన్య read_subscriptions (uvwasi_subscription_t* o_subs, స్థిరరాశి uint8_t* i_లోపల,
                         uvwasi_కొలత_t i_లెక్క)
{
    కొరకు (uvwasi_కొలత_t i = 0; i < i_లెక్క; ++i) {
        స్థిరరాశి uint8_t* s = i_లోపల + (size_t)i * d_m3_Wasi_Subscription_కొలత;

        memset(&o_subs[i], 0, పరిమాణం(o_subs[i]));

        o_subs[i].userdata = m3ApiReadMem64(s + 0);
        o_subs[i].type     = m3ApiReadMem8(s + 8);

        ఒకవేళ (o_subs[i].type == UVWASI_EVENTTYPE_CLOCK) {
            o_subs[i].u.clock.clock_id  = m3ApiReadMem32(s + 16);
            o_subs[i].u.clock.timeout   = m3ApiReadMem64(s + 24);
            o_subs[i].u.clock.precision = m3ApiReadMem64(s + 32);
            o_subs[i].u.clock.flags     = m3ApiReadMem16(s + 40);
        } లేకపోతే {
            o_subs[i].u.fd_readwrite.fd = m3ApiReadMem32(s + 16);
        }
    }
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
స్థిర
శూన్య write_event (uint8_t* o_rec, స్థిరరాశి uvwasi_event_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_కొలత);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->u.fd_readwrite.nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->u.fd_readwrite.flags);
}

// Waits on the subscriptions, once whichever reader above has decoded them, and
// writes the events back. Knows nothing about revisions: the event record is the
// same in both. i_subs is NULL when there are none, which uvwasi answers for itself.
స్థిర
uvwasi_errno_t poll_wasi_subscriptions (uvwasi_t* i_uvwasi, స్థిరరాశి uvwasi_subscription_t* i_subs,
                                        uint8_t* o_బహిర్గతం, uvwasi_కొలత_t i_లెక్క,
                                        uvwasi_కొలత_t* o_written)
{
    *o_written = 0;

    uvwasi_event_t* events = i_లెక్క ? m3_AllocArray(uvwasi_event_t, i_లెక్క) : ఖాళీ;

    ఒకవేళ (i_లెక్క and not events) {
        తిరిగి UVWASI_ENOMEM;
    }

    uvwasi_కొలత_t  written = 0;
    uvwasi_errno_t ret     = uvwasi_poll_oneoff(i_uvwasi, i_subs, events, i_లెక్క, &written);

    ఒకవేళ (ret == UVWASI_ESUCCESS) {
        // One event per subscription is the most that was checked for, and the most
        // there can be; clamping keeps a surprise from reaching past that
        written = M3_MIN(written, i_లెక్క);

        కొరకు (uvwasi_కొలత_t i = 0; i < written; ++i) {
            write_event(o_బహిర్గతం + (size_t)i * d_m3_Wasi_Event_కొలత, &events[i]);
        }
        *o_written = written;
    }

    m3_Free(events);

    తిరిగి ret;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(స్థిరరాశి uint8_t*, లోపల)
    m3ApiGetArgMem(uint8_t*, బహిర్గతం)
    m3ApiGetArg(uvwasi_కొలత_t, nsubscriptions)
    m3ApiGetArgMem(uvwasi_కొలత_t*, nevents)

    m3ApiCheckMem(లోపల, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_కొలత);
    m3ApiCheckMem(బహిర్గతం, (uint64_t)nsubscriptions * d_m3_Wasi_Event_కొలత);
    m3ApiCheckMem(nevents, పరిమాణం(uvwasi_కొలత_t));

    uvwasi_subscription_t* subs = ఖాళీ;

    ఒకవేళ (nsubscriptions) {
        subs = m3_AllocArray(uvwasi_subscription_t, nsubscriptions);
        ఒకవేళ (not subs) {
            m3ApiReturn(UVWASI_ENOMEM);
        }
        read_subscriptions(subs, లోపల, nsubscriptions);
    }

    uvwasi_కొలత_t లెక్క = 0;

    uvwasi_errno_t ret =
      poll_wasi_subscriptions(_get_uvwasi_p(_ctx), subs, బహిర్గతం, nsubscriptions, &లెక్క);

    m3_Free(subs);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, లెక్క);

    m3ApiWriteMem32(nevents, లెక్క);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, సంకేతం_2)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ఒకవేళ (context) {
        context->exit_సంకేతం = సంకేతం_2;
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


స్థిర
M3_ఫలితం SuppressLookupFailure (M3_ఫలితం i_ఫలితం)
{
    ఒకవేళ (i_ఫలితం == m3Err_functionLookupFailed) {
        తిరిగి m3Err_none;
    } లేకపోతే {
        తిరిగి i_ఫలితం;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    తిరిగి wasi_context;
}

స్థిర స్థిరరాశి అక్షరం* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ఖాళీ,
};

స్థిర uvwasi_preopen_t DEFAULT_PREOPENS[] = {
    { .mapped_మార్గం = "/",  .real_మార్గం = "." },
    { .mapped_మార్గం = "./", .real_మార్గం = "." },
};

స్థిర అంతఃవరుస
శూన్య m3_సమితి_పైకి_default_uvwasi_opts (uvwasi_options_t* opts_p)
{

    uvwasi_options_init(opts_p);
    opts_p->argc     = 0;      // runtime->argc is not initialized at this point, so we implement args_get directly
    opts_p->envp     = DEFAULT_ENVIRONMENT;
    opts_p->preopenc = పరిమాణం(DEFAULT_PREOPENS) / పరిమాణం(uvwasi_preopen_t);
    opts_p->preopens = DEFAULT_PREOPENS;
}

స్థిర
M3_ఫలితం _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    స్థిర స్థిరరాశి అక్షరం* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    కొరకు (size_t i = 0; i < M3_లెక్క_యొక్క(namespaces); i++) {
        స్థిరరాశి అక్షరం* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_వాదనలు_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_వాదనలు_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_సమయం_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get, wasi_context)));

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_సమితి_flags, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_సమితి_rights, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_సమితి_కొలత, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_సమితి_times, wasi_context)));
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

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_మార్గం_create_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_మార్గం_filestat_get, wasi_context)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_మార్గం_open, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_మార్గం_readlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_మార్గం_remove_directory, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_మార్గం_rename, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_మార్గం_symlink, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_మార్గం_unlink_file, wasi_context)));

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
    ఒకవేళ (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 జ్ఞాపకం_సూచీ;

        ఒకవేళ (m3_కనుగొను_Exported_జ్ఞాపకం(module, "memory", &జ్ఞాపకం_సూచీ)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_జ్ఞాపకం(module, "wasi_snapshot_preview1", జ్ఞాపకం_సూచీ));
    }

_catch:
    తిరిగి ఫలితం;
}

M3_ఫలితం m3_NewCustomWASIWithOptions (M3WASI* ctx_p, uvwasi_options_t init_options)
{
    *ctx_p = m3_AllocStruct(m3_wasi_context_t);
    ప్రతిజ్ఞా(*ctx_p);

    uvwasi_errno_t ret = uvwasi_init(&(*ctx_p)->uvwasi, &init_options);

    ఒకవేళ (ret != UVWASI_ESUCCESS) {
        తిరిగి "uvwasi_init failed";
    }

    తిరిగి m3Err_none;
}

M3_ఫలితం m3_NewCustomWASI (M3WASI* wasi_p)
{
    uvwasi_options_t opts;
    m3_సమితి_పైకి_default_uvwasi_opts(&opts);

    తిరిగి m3_NewCustomWASIWithOptions(wasi_p, opts);
}

M3_ఫలితం m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    తిరిగి _linkWASI(module, wasi);
}

శూన్య m3_FreeWASI (m3_wasi_context_t* wasi)
{
    uvwasi_destroy(&wasi->uvwasi);
    m3_Free(wasi);
}

M3_ఫలితం m3_LinkWASI (IM3Module module)
{
    uvwasi_options_t opts;
    m3_సమితి_పైకి_default_uvwasi_opts(&opts);

    తిరిగి m3_LinkWASIWithOptions(module, opts);
}

M3_ఫలితం m3_LinkWASIWithOptions (IM3Module module, uvwasi_options_t init_options)
{
    ఒకవేళ (ఖాళీ == wasi_context) {
        M3_ఫలితం res = m3_NewCustomWASIWithOptions(&wasi_context, init_options);
        ఒకవేళ (res) {
            తిరిగి res;
        }
    }

    తిరిగి _linkWASI(module, wasi_context);
}

#అయితే_ముగింపు // d_m3HasUVWASI
