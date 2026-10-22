//
//  m3_api_meta_wasi.c
//
//  Created by Volodymyr Shymanskyy on 01/08/20.
//  Copyright © 2020 Volodymyr Shymanskyy. All rights reserved.
//

#అన్తర్భావ "m3_api_wasi.h"

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_exception.h"

#ఒకవేళ defined(d_m3HasMetaWASI)

// NOTE: MetaWASI mostly redirects WASI calls to the host WASI environment

#  ఒకవేళ !defined(__wasi__)
#    error "MetaWASI is only supported on WASI target"
#  endif

#  ఒకవేళ __has_include("wasi/api.h")
#    include <wasi/api.h>
#    define USE_NEW_WASI
#    define WASI_STAT_క్షేత్రం(f) f

#  elif __has_include("wasi/core.h")
#    warning "Using legacy WASI headers"
#    include <wasi/core.h>
#    define __WASI_ERRNO_SUCCESS   __WASI_ESUCCESS
#    define __WASI_ERRNO_INVAL     __WASI_EINVAL
#    define WASI_STAT_క్షేత్రం(f) st_##f

#  లేకపోతే
#    error "Missing WASI headers"
#  endif

స్థిర m3_wasi_context_t* wasi_context;

రకనిర్వచనం size_t __wasi_size_t;

// The WASI ABI passes paths as (pointer, length) with no terminator, but wasi-libc's
// __wasi_path_* wrappers take NUL-terminated strings, so every path has to be copied out
// of guest memory and terminated first.
#  ifdef PATH_MAX
#    define d_m3_Wasi_మార్గం_Max   PATH_MAX
#  లేకపోతే
#    define d_m3_Wasi_మార్గం_Max   1024
#  endif

// Declares NAME as a NUL-terminated copy of the guest's path, or returns ENAMETOOLONG
#  define d_m3_Wasi_మార్గం(NAME, PTR, LEN)                                   \
      అక్షరం NAME [d_m3_Wasi_మార్గం_Max];                                       \
      ఒకవేళ ((LEN) >= పరిమాణం (NAME)) m3ApiReturn (__WASI_ERRNO_NAMETOOLONG); \
      జ్ఞాపకం_నకలు (NAME, (PTR), (LEN));                                       \
      NAME [(LEN)] = 0;

స్థిర అంతఃవరుస
స్థిరరాశి శూన్య* copy_iov_గమ్యం_host (IM3Runtime runtime, శూన్య* _mem, __wasi_iovec_t* host_iov, __wasi_iovec_t* wasi_iov, int32_t iovs_len)
{
    // Convert wasi memory offsets to host addresses
    కొరకు (పూర్ణాంకం i = 0; i < iovs_len; i++) {
        host_iov[i].buf     = m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(wasi_iov[i].buf);
        host_iov[i].buf_len = wasi_iov[i].buf_len;
        m3ApiCheckMem(host_iov[i].buf, host_iov[i].buf_len);
    }
    m3ApiSuccess();
}

#  ఒకవేళ d_m3EnableWasiTracing

స్థిరరాశి అక్షరం* wasi_errno2str (__wasi_errno_t err)
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

స్థిరరాశి అక్షరం* wasi_whence2str (__wasi_whence_t whence)
{
    ఎంపిక (whence) {
    స్థితి __WASI_WHENCE_SET: తిరిగి "SET";
    స్థితి __WASI_WHENCE_CUR: తిరిగి "CUR";
    స్థితి __WASI_WHENCE_END: తిరిగి "END";
    అప్రమేయ: తిరిగి "<unknown>";
    }
}

#    define WASI_TRACE(fmt, ...)    { fprintf(stderr, "%s " fmt, __FUNCTION__+16, ##__VA_ARGS__); fprintf(stderr, " => %s\n", wasi_errno2str(ret)); }
#  లేకపోతే
#    define WASI_TRACE(fmt, ...)
#  endif

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
        m3ApiReturn(__WASI_ERRNO_INVAL);
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

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_వాదనలు_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_కొలత)

    m3ApiCheckMem(argc, పరిమాణం(__wasi_size_t));
    m3ApiCheckMem(argv_buf_కొలత, పరిమాణం(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ఒకవేళ (context == ఖాళీ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    కొరకు (u32 i = 0; i < context->argc; ++i) {
        buf_len += అక్షరశ్రేణి_పొడవు(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_కొలత, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(అక్షరం*, env_buf)

    __wasi_errno_t ret;
    __wasi_size_t  env_లెక్క, env_buf_కొలత;

    ret = __wasi_environ_sizes_get(&env_లెక్క, &env_buf_కొలత);
    ఒకవేళ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    m3ApiCheckMem(env, env_లెక్క * పరిమాణం(uint32_t));
    m3ApiCheckMem(env_buf, env_buf_కొలత);

    // the array is the guest's, but the host fills it with its own pointers, which
    // are what m3ApiPtrToOffset turns back into guest offsets below
    uint8_t** host_env = (uint8_t**)env;

    ret = __wasi_environ_get(host_env, (uint8_t*)env_buf);
    ఒకవేళ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    కొరకు (u32 i = 0; i < env_లెక్క; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_గమ్యం_స్థానభ్రంశం(host_env[i]));
    }

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_లెక్క)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_కొలత)

    m3ApiCheckMem(env_లెక్క, పరిమాణం(__wasi_size_t));
    m3ApiCheckMem(env_buf_కొలత, పరిమాణం(__wasi_size_t));

    __wasi_errno_t ret = __wasi_environ_sizes_get(env_లెక్క, env_buf_కొలత);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    __wasi_errno_t ret = __wasi_fd_prestat_dir_name(fd, (uint8_t*)మార్గం, మార్గం_len);

    WASI_TRACE("fd:%d, len:%d | path:%s", fd, మార్గం_len, మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_prestat_t*, buf)

    m3ApiCheckMem(buf, పరిమాణం(__wasi_prestat_t));

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

    ఒకవేళ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(fdstat, 0, 24);
    m3ApiWriteMem8(fdstat + 0, stat.fs_filetype);
    m3ApiWriteMem16(fdstat + 2, stat.fs_flags);
    m3ApiWriteMem64(fdstat + 8, stat.fs_rights_పునాది);
    m3ApiWriteMem64(fdstat + 16, stat.fs_rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_సమితి_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    __wasi_errno_t ret = __wasi_fd_fdstat_set_flags(fd, flags);

    WASI_TRACE("fd:%d, flags:0x%x", fd, flags);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_సమితి_rights)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_rights_t, rights_పునాది)
    m3ApiGetArg(__wasi_rights_t, rights_inheriting)

#  ఒకవేళ 0
    __wasi_errno_t ret = __wasi_fd_fdstat_set_rights(fd, rights_పునాది, rights_inheriting);
#  లేకపోతే
    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  endif

    WASI_TRACE("fd:%d, base:0x%" PRIx64 ", inheriting:0x%" PRIx64, fd, rights_పునాది, rights_inheriting);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_సమితి_కొలత)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, కొలత)

    __wasi_errno_t ret = __wasi_fd_filestat_set_size(fd, కొలత);

    WASI_TRACE("fd:%d, size:%" PRIu64, fd, కొలత);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_సమితి_times)
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

    WASI_TRACE("fd:%d | fs.size:%" PRIu64, fd, stat.WASI_STAT_క్షేత్రం(కొలత));

    ఒకవేళ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_క్షేత్రం(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_క్షేత్రం(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_క్షేత్రం(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_క్షేత్రం(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_క్షేత్రం(కొలత));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_క్షేత్రం(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_క్షేత్రం(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_క్షేత్రం(ctim));

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, ఫలితం)

    m3ApiCheckMem(ఫలితం, పరిమాణం(__wasi_filesize_t));

    __wasi_whence_t whence = -1;
    ఎంపిక (wasi_whence) {
    స్థితి 0: whence = __WASI_WHENCE_SET; ఆపు;
    స్థితి 1: whence = __WASI_WHENCE_CUR; ఆపు;
    స్థితి 2: whence = __WASI_WHENCE_END; ఆపు;
    }

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_seek(fd, offset, whence, &pos);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", whence:%s | result:%" PRIu64,
               fd, offset, wasi_whence2str(whence), pos);

    *ఫలితం = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_renumber)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, from)
    m3ApiGetArg(__wasi_fd_t, గమ్యం)

    __wasi_errno_t ret = __wasi_fd_renumber(from, గమ్యం);

    WASI_TRACE("from:%d, to:%d", from, గమ్యం);

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
    m3ApiGetArgMem(__wasi_filesize_t*, ఫలితం)

    m3ApiCheckMem(ఫలితం, పరిమాణం(__wasi_filesize_t));

    __wasi_filesize_t pos;
    __wasi_errno_t    ret = __wasi_fd_tell(fd, &pos);

    WASI_TRACE("fd:%d | result:%" PRIu64, fd, pos);

    *ఫలితం = pos;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    d_m3_Wasi_మార్గం(p, మార్గం, మార్గం_len)

    __wasi_errno_t ret = __wasi_path_create_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (పూర్ణాంకం)మార్గం_len, మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)
    m3ApiGetArgMem(అక్షరం*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, పరిమాణం(__wasi_size_t));

    d_m3_Wasi_మార్గం(p, మార్గం, మార్గం_len)

    __wasi_errno_t ret = __wasi_path_readlink(fd, p, (uint8_t*)buf, buf_len, bufused);

    WASI_TRACE("fd:%d, path:%.*s | buf:%s, bufused:%d", fd, (పూర్ణాంకం)మార్గం_len, మార్గం, buf, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    d_m3_Wasi_మార్గం(p, మార్గం, మార్గం_len)

    __wasi_errno_t ret = __wasi_path_remove_directory(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (పూర్ణాంకం)మార్గం_len, మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, old_మార్గం)
    m3ApiGetArg(__wasi_size_t, old_మార్గం_len)
    m3ApiGetArg(__wasi_fd_t, new_fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, new_మార్గం)
    m3ApiGetArg(__wasi_size_t, new_మార్గం_len)

    m3ApiCheckMem(old_మార్గం, old_మార్గం_len);
    m3ApiCheckMem(new_మార్గం, new_మార్గం_len);

    d_m3_Wasi_మార్గం(op, old_మార్గం, old_మార్గం_len)
    d_m3_Wasi_మార్గం(np, new_మార్గం, new_మార్గం_len)

    __wasi_errno_t ret = __wasi_path_rename(old_fd, op,
                                            new_fd, np);

    WASI_TRACE("old_fd:%d, old_path:%.*s, new_fd:%d, new_path:%.*s",
               old_fd, (పూర్ణాంకం)old_మార్గం_len, old_మార్గం, new_fd, (పూర్ణాంకం)new_మార్గం_len, new_మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, old_మార్గం)
    m3ApiGetArg(__wasi_size_t, old_మార్గం_len)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, new_మార్గం)
    m3ApiGetArg(__wasi_size_t, new_మార్గం_len)

    m3ApiCheckMem(old_మార్గం, old_మార్గం_len);
    m3ApiCheckMem(new_మార్గం, new_మార్గం_len);

    d_m3_Wasi_మార్గం(op, old_మార్గం, old_మార్గం_len)
    d_m3_Wasi_మార్గం(np, new_మార్గం, new_మార్గం_len)

    __wasi_errno_t ret = __wasi_path_symlink(op,
                                             fd, np);

    WASI_TRACE("old_path:%.*s, fd:%d, new_path:%.*s",
               (పూర్ణాంకం)old_మార్గం_len, old_మార్గం, fd, (పూర్ణాంకం)new_మార్గం_len, new_మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    d_m3_Wasi_మార్గం(p, మార్గం, మార్గం_len)

    __wasi_errno_t ret = __wasi_path_unlink_file(fd, p);

    WASI_TRACE("fd:%d, path:%.*s", fd, (పూర్ణాంకం)మార్గం_len, మార్గం);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_పునాది)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(fd, పరిమాణం(__wasi_fd_t));

    d_m3_Wasi_మార్గం(p, మార్గం, మార్గం_len)

    __wasi_errno_t ret = __wasi_path_open(dirfd,
                                          dirflags,
                                          p,
                                          oflags,
                                          fs_rights_పునాది,
                                          fs_rights_inheriting,
                                          fs_flags,
                                          fd);

    WASI_TRACE("dirfd:%d, dirflags:0x%x, path:%.*s, oflags:0x%x, fs_flags:0x%x | fd:%d",
               dirfd, dirflags, (పూర్ణాంకం)మార్గం_len, మార్గం, oflags, fs_flags, *fd);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_lookupflags_t, flags)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(uint32_t, మార్గం_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(buf, 64); // wasi_filestat_t

    __wasi_filestat_t stat;

    d_m3_Wasi_మార్గం(p, మార్గం, మార్గం_len)

    __wasi_errno_t ret = __wasi_path_filestat_get(fd, flags, p, &stat);

    WASI_TRACE("fd:%d, flags:0x%x, path:%.*s | fs.size:%" PRIu64,
               fd, flags, (పూర్ణాంకం)మార్గం_len, మార్గం, stat.WASI_STAT_క్షేత్రం(కొలత));

    ఒకవేళ (ret != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(ret);
    }

    memset(buf, 0, 64);
    m3ApiWriteMem64(buf + 0, stat.WASI_STAT_క్షేత్రం(dev));
    m3ApiWriteMem64(buf + 8, stat.WASI_STAT_క్షేత్రం(ino));
    m3ApiWriteMem8(buf + 16, stat.WASI_STAT_క్షేత్రం(filetype));
    m3ApiWriteMem64(buf + 24, stat.WASI_STAT_క్షేత్రం(nlink));
    m3ApiWriteMem64(buf + 32, stat.WASI_STAT_క్షేత్రం(కొలత));
    m3ApiWriteMem64(buf + 40, stat.WASI_STAT_క్షేత్రం(atim));
    m3ApiWriteMem64(buf + 48, stat.WASI_STAT_క్షేత్రం(mtim));
    m3ApiWriteMem64(buf + 56, stat.WASI_STAT_క్షేత్రం(ctim));

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

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(__wasi_iovec_t));
    m3ApiCheckMem(nread, పరిమాణం(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    స్థిరరాశి శూన్య*    mem_check = copy_iov_గమ్యం_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ఒకవేళ (mem_check != m3Err_none) {
        తిరిగి mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(__wasi_iovec_t));
    m3ApiCheckMem(nread, పరిమాణం(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    స్థిరరాశి శూన్య*    mem_check = copy_iov_గమ్యం_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ఒకవేళ (mem_check != m3Err_none) {
        తిరిగి mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, పరిమాణం(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    స్థిరరాశి శూన్య*    mem_check = copy_iov_గమ్యం_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ఒకవేళ (mem_check != m3Err_none) {
        తిరిగి mem_check;
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

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(__wasi_iovec_t));
    m3ApiCheckMem(nwritten, పరిమాణం(__wasi_size_t));

    __wasi_iovec_t iovs[iovs_len];
    స్థిరరాశి శూన్య*    mem_check = copy_iov_గమ్యం_host(runtime, _mem, iovs, wasi_iovs, iovs_len);
    ఒకవేళ (mem_check != m3Err_none) {
        తిరిగి mem_check;
    }

    __wasi_errno_t ret = __wasi_fd_pwrite(fd, (__wasi_ciovec_t*)iovs, iovs_len, offset, nwritten);

    WASI_TRACE("fd:%d | nwritten:%d", fd, *nwritten);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_readdir)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(శూన్య*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArg(__wasi_dircookie_t, cookie)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, పరిమాణం(__wasi_size_t));

    __wasi_errno_t ret = __wasi_fd_readdir(fd, buf, buf_len, cookie, bufused);

    WASI_TRACE("fd:%d | bufused:%d", fd, *bufused);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, నిడివి)
    m3ApiGetArg(__wasi_advice_t, advice)

    __wasi_errno_t ret = __wasi_fd_advise(fd, offset, నిడివి, advice);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64 ", advice:%d", fd, offset, నిడివి, advice);

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_fd_allocate)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, నిడివి)

    __wasi_errno_t ret = __wasi_fd_allocate(fd, offset, నిడివి);

    WASI_TRACE("fd:%d, offset:%" PRIu64 ", length:%" PRIu64, fd, offset, నిడివి);

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

    m3ApiCheckMem(resolution, పరిమాణం(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_res_get(wasi_clk_id, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *resolution = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_clock_సమయం_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, పరిమాణం(__wasi_timestamp_t));

    __wasi_timestamp_t t;
    __wasi_errno_t     ret = __wasi_clock_time_get(wasi_clk_id, precision, &t);

    WASI_TRACE("clk_id:%d | res:%" PRIu64, wasi_clk_id, t);

    *time = t;

    m3ApiReturn(ret);
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(స్థిరరాశి __wasi_subscription_t*, లోపల)
    m3ApiGetArgMem(__wasi_event_t*, బహిర్గతం)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(లోపల, nsubscriptions * పరిమాణం(__wasi_subscription_t));
    m3ApiCheckMem(బహిర్గతం, nsubscriptions * పరిమాణం(__wasi_event_t));
    m3ApiCheckMem(nevents, పరిమాణం(__wasi_size_t));

    __wasi_errno_t ret = __wasi_poll_oneoff(లోపల, బహిర్గతం, nsubscriptions, nevents);

    WASI_TRACE("nsubscriptions:%d | nevents:%d", nsubscriptions, *nevents);

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
    // proc_raise was dropped from preview1, so newer wasi-libc no longer has
    // __wasi_signal_t; the import signature is "i(i)" either way
    m3ApiGetArg(uint32_t, sig)

    __wasi_errno_t ret = __WASI_ERRNO_INVAL;
#  ఒకవేళ 0
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

శూన్య m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

స్థిర అంతఃవరుస
M3_ఫలితం _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    స్థిర స్థిరరాశి అక్షరం* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    కొరకు (size_t i = 0; i < M3_లెక్క_యొక్క(namespaces); i++) {
        స్థిరరాశి అక్షరం* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_వాదనలు_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_వాదనలు_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_సమయం_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  &m3_wasi_p1_fd_allocate)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_సమితి_flags)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  &m3_wasi_p1_fd_fdstat_సమితి_rights)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",       "i(i*)",       &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_సమితి_కొలత)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_సమితి_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_మార్గం_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",     "i(ii*i*)",    &m3_wasi_p1_మార్గం_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_మార్గం_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_మార్గం_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_మార్గం_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_మార్గం_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_మార్గం_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_మార్గం_unlink_file)));

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

M3_ఫలితం m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    తిరిగి m3Err_none;
}

శూన్య m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_ఫలితం m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    తిరిగి _linkWASI(module, wasi);
}

M3_ఫలితం m3_LinkWASI (IM3Module module)
{
    ఒకవేళ (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    తిరిగి _linkWASI(module, wasi_context);
}

#అయితే_ముగింపు // d_m3HasMetaWASI
