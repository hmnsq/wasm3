//
//  m3_api_wasi.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#అన్తర్భావ "m3_api_wasi.h"

#అన్తర్భావ "m3_api_wasi_host.h"
#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_deterministic.h"
#అన్తర్భావ "m3_exception.h"

#ఒకవేళ defined(d_m3HasWASI)

#  include <string.h>

// The system underneath, of which exactly one is ever built. They are headers rather
// than translation units of their own so that neither has to be added to a source
// list - and most of those are build scripts outside this repository.
#  ఒకవేళ defined(_WIN32)
#    include "m3_api_wasi_win32.h"
#  లేకపోతే
#    include "m3_api_wasi_posix.h"
#  endif

// This file is the WASI interface and nothing below it: it decodes the guest's
// arguments, bounds-checks every pointer against guest memory, and writes results
// back in the layout the calling revision asks for. The system underneath is reached
// through m3_api_wasi_host.h, which m3_api_wasi_posix.h and m3_api_wasi_win32.h
// implement independently of each other.

స్థిర m3_wasi_context_t* wasi_context;

// A scatter/gather list as the guest lays one out
రకనిర్వచనం నిర్మాణం wasi_iovec_t {
    __wasi_size_t buf;
    __wasi_size_t buf_len;
} wasi_iovec_t;

// The records the guest passes by address, sized as the ABI lays them out rather
// than as this host happens to - see write_filestat, read_subscriptions, write_event.
#  define d_m3_Wasi_Filestat_కొలత      64
#  define d_m3_Wasi_Subscription_కొలత  48
#  define d_m3_Wasi_Event_కొలత         32

స్థిర స్థిరరాశి అక్షరం* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ఖాళీ,
};

// Copies a path out of guest memory so it can be handed to the host NUL-terminated.
// The caller has already bounds-checked (i_path, i_pathLen) against guest memory.
స్థిర
పూర్ణాంకం copy_మార్గం (అక్షరం* o_మార్గం, స్థిరరాశి అక్షరం* i_మార్గం, __wasi_size_t i_మార్గం_Len)
{
    ఒకవేళ (i_మార్గం_Len >= d_m3_Wasi_Max_మార్గం) {
        తిరిగి 0;
    }
    జ్ఞాపకం_నకలు(o_మార్గం, i_మార్గం, i_మార్గం_Len);
    o_మార్గం[i_మార్గం_Len] = '\0';
    తిరిగి 1;
}

// Writes a filestat out: dev 0, ino 8, filetype 16, nlink 24, size 32, atim 40,
// mtim 48, ctim 56. The caller has checked the buffer for d_m3WasiFilestatSize.
స్థిర
శూన్య write_filestat (uint8_t* o_buf, స్థిరరాశి m3_wasi_filestat_t* i_stat)
{
    memset(o_buf, 0, d_m3_Wasi_Filestat_కొలత);

    m3ApiWriteMem64(o_buf + 0, i_stat->dev);
    m3ApiWriteMem64(o_buf + 8, i_stat->ino);
    m3ApiWriteMem8(o_buf + 16, i_stat->filetype);
    m3ApiWriteMem64(o_buf + 24, i_stat->nlink);
    m3ApiWriteMem64(o_buf + 32, i_stat->కొలత);
    m3ApiWriteMem64(o_buf + 40, i_stat->atim);
    m3ApiWriteMem64(o_buf + 48, i_stat->mtim);
    m3ApiWriteMem64(o_buf + 56, i_stat->ctim);
}

// Resolves the guest's scatter/gather list to host addresses, checking each buffer
// against guest memory, and adds up how much they ask for. The host layer is handed
// the result and never sees a guest offset.
స్థిర
స్థిరరాశి శూన్య* resolve_iovs (IM3Runtime runtime, శూన్య* _mem, m3_wasi_iovec_t* o_iovs,
                          స్థిరరాశి wasi_iovec_t* i_wasiIovs, __wasi_size_t i_లెక్క,
                          __wasi_size_t* o_want)
{
    *o_want = 0;

    __wasi_size_t want = 0;

    కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
        o_iovs[i].buf = m3_Api_స్థానభ్రంశం_గమ్యం_Ptr(m3ApiReadMem32(&i_wasiIovs[i].buf));
        o_iovs[i].len = m3ApiReadMem32(&i_wasiIovs[i].buf_len);
        m3ApiCheckMem(o_iovs[i].buf, o_iovs[i].len);
        want += (__wasi_size_t)o_iovs[i].len;
    }

    *o_want = want;
    m3ApiSuccess();
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
        buf_len += (__wasi_size_t)అక్షరశ్రేణి_పొడవు(context->argv[i]) + 1;
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

    __wasi_size_t లెక్క = 0;
    కొరకు (స్థిరరాశి అక్షరం** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        లెక్క++;
    }

    m3ApiCheckMem(env, లెక్క * పరిమాణం(uint32_t));

    కొరకు (u32 i = 0; i < లెక్క; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_గమ్యం_స్థానభ్రంశం(env_buf));

        size_t len = అక్షరశ్రేణి_పొడవు(DEFAULT_ENVIRONMENT[i]);

        m3ApiCheckMem(env_buf, len + 1);        // include the terminating `\0`
        జ్ఞాపకం_నకలు(env_buf, DEFAULT_ENVIRONMENT[i], len);
        env_buf += len;
        *env_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_లెక్క)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_కొలత)

    m3ApiCheckMem(env_లెక్క, పరిమాణం(__wasi_size_t));
    m3ApiCheckMem(env_buf_కొలత, పరిమాణం(__wasi_size_t));

    __wasi_size_t లెక్క = 0, buf_len = 0;
    కొరకు (స్థిరరాశి అక్షరం** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        లెక్క++;
        buf_len += (__wasi_size_t)అక్షరశ్రేణి_పొడవు(*e) + 1;
    }

    m3ApiWriteMem32(env_లెక్క, లెక్క);
    m3ApiWriteMem32(env_buf_కొలత, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    ఒకవేళ (fd < 3 || fd >= d_m3_Wasi_Preopen_లెక్క) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }
    size_t slen = అక్షరశ్రేణి_పొడవు(m3_wasi_preopen[fd].మార్గం) + 1;
    జ్ఞాపకం_నకలు(మార్గం, m3_wasi_preopen[fd].మార్గం, M3_MIN(slen, మార్గం_len));
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 8);

    ఒకవేళ (fd < 3 || fd >= d_m3_Wasi_Preopen_లెక్క) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }

    m3ApiWriteMem32(buf + 0, __WASI_PREOPENTYPE_DIR);
    m3ApiWriteMem32(buf + 4, (u32)అక్షరశ్రేణి_పొడవు(m3_wasi_preopen[fd].మార్గం) + 1);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_fdstat_t*, fdstat)

    m3ApiCheckMem(fdstat, పరిమాణం(__wasi_fdstat_t));

    __wasi_filetype_t filetype;
    __wasi_fdflags_t  flags;

    __wasi_errno_t err = m3_wasi_host_fd_fdstat(fd, &filetype, &flags);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    fdstat->fs_filetype = filetype;
    m3ApiWriteMem16(&fdstat->fs_flags, flags);

    fdstat->fs_rights_పునాది = (uint64_t)-1; // all rights

    // Make descriptors 0,1,2 look like a TTY
    ఒకవేళ (fd <= 2) {
        fdstat->fs_rights_పునాది &= ~(__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
    }

    fdstat->fs_rights_inheriting = (uint64_t)-1; // all rights
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_సమితి_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_fdstat_సమితి_flags(fd, flags));
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, నిడివి)
    m3ApiGetArg(__wasi_advice_t, advice)

    m3ApiReturn(m3_wasi_host_fd_advise(fd, offset, నిడివి, advice));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_కొలత);

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t err = m3_wasi_host_fd_filestat(fd, &fd_stat);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &fd_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_కొలత);

    అక్షరం host_మార్గం[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_మార్గం, మార్గం, మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3_wasi_filestat_t మార్గం_stat;
    bool               follow = (dirflags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    __wasi_errno_t err = m3_wasi_host_మార్గం_filestat(dirfd, host_మార్గం, follow, &మార్గం_stat);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &మార్గం_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_సమితి_కొలత)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, కొలత)

    m3ApiReturn(m3_wasi_host_fd_సమితి_కొలత(fd, కొలత));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_సమితి_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_timestamp_t, atim)
    m3ApiGetArg(__wasi_timestamp_t, mtim)
    m3ApiGetArg(__wasi_fstflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_సమితి_times(fd, atim, mtim, flags));
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_link)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, old_flags)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, old_మార్గం)
    m3ApiGetArg(__wasi_size_t, old_మార్గం_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, new_మార్గం)
    m3ApiGetArg(__wasi_size_t, new_మార్గం_len)

    m3ApiCheckMem(old_మార్గం, old_మార్గం_len);
    m3ApiCheckMem(new_మార్గం, new_మార్గం_len);

    అక్షరం host_old[d_m3_Wasi_Max_మార్గం];
    అక్షరం host_new[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_old, old_మార్గం, old_మార్గం_len) ||
        !copy_మార్గం(host_new, new_మార్గం, new_మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    bool follow = (old_flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    m3ApiReturn(m3_wasi_host_మార్గం_link(old_dirfd, host_old, follow, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    అక్షరం host_మార్గం[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_మార్గం, మార్గం, మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_మార్గం_create_directory(dirfd, host_మార్గం));
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    అక్షరం host_మార్గం[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_మార్గం, మార్గం, మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_మార్గం_remove_directory(dirfd, host_మార్గం));
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)

    m3ApiCheckMem(మార్గం, మార్గం_len);

    అక్షరం host_మార్గం[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_మార్గం, మార్గం, మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_మార్గం_unlink_file(dirfd, host_మార్గం));
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, old_మార్గం)
    m3ApiGetArg(__wasi_size_t, old_మార్గం_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, new_మార్గం)
    m3ApiGetArg(__wasi_size_t, new_మార్గం_len)

    m3ApiCheckMem(old_మార్గం, old_మార్గం_len);
    m3ApiCheckMem(new_మార్గం, new_మార్గం_len);

    అక్షరం host_old[d_m3_Wasi_Max_మార్గం];
    అక్షరం host_new[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_old, old_మార్గం, old_మార్గం_len) ||
        !copy_మార్గం(host_new, new_మార్గం, new_మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_మార్గం_rename(old_dirfd, host_old, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, old_మార్గం)
    m3ApiGetArg(__wasi_size_t, old_మార్గం_len)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, new_మార్గం)
    m3ApiGetArg(__wasi_size_t, new_మార్గం_len)

    m3ApiCheckMem(old_మార్గం, old_మార్గం_len);
    m3ApiCheckMem(new_మార్గం, new_మార్గం_len);

    // the target is stored as given, so it is a string rather than a path to resolve
    అక్షరం host_old[d_m3_Wasi_Max_మార్గం];
    అక్షరం host_new[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_old, old_మార్గం, old_మార్గం_len) ||
        !copy_మార్గం(host_new, new_మార్గం, new_మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_మార్గం_symlink(host_old, dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_మార్గం_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(స్థిరరాశి అక్షరం*, మార్గం)
    m3ApiGetArg(__wasi_size_t, మార్గం_len)
    m3ApiGetArgMem(అక్షరం*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(మార్గం, మార్గం_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, పరిమాణం(__wasi_size_t));

    అక్షరం host_మార్గం[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_మార్గం, మార్గం, మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_size_t వాడిన;

    __wasi_errno_t err = m3_wasi_host_మార్గం_readlink(dirfd, host_మార్గం, buf, buf_len, &వాడిన);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(bufused, వాడిన);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_tell)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_filesize_t*, ఫలితం)

    m3ApiCheckMem(ఫలితం, పరిమాణం(__wasi_filesize_t));

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, 0, SEEK_CUR, &pos);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(ఫలితం, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, ఫలితం)

    m3ApiCheckMem(ఫలితం, పరిమాణం(__wasi_filesize_t));

    పూర్ణాంకం whence;

    ఎంపిక (wasi_whence) {
    స్థితి 0: whence = SEEK_SET; ఆపు;
    స్థితి 1: whence = SEEK_CUR; ఆపు;
    స్థితి 2: whence = SEEK_END; ఆపు;
    అప్రమేయ: m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, offset, whence, &pos);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(ఫలితం, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
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

    అక్షరం host_మార్గం[d_m3_Wasi_Max_మార్గం];
    ఒకవేళ (!copy_మార్గం(host_మార్గం, మార్గం, మార్గం_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_fd_t host_fd;

    __wasi_errno_t err = m3_wasi_host_మార్గం_open(dirfd, host_మార్గం, oflags, fs_rights_పునాది,
                                                fs_flags, &host_fd);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(fd, host_fd);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

// The four transfer calls all walk the guest's list a batch at a time - see
// d_m3WasiMaxIovs - and stop where a batch comes back short, as a short readv would
// have stopped the whole call.

m3ApiRawFunction(m3_wasi_p1_fd_read)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nread, పరిమాణం(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   మొత్తం = 0;

    కొరకు (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t లెక్క = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, పూర్తి;

        స్థిరరాశి శూన్య* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, లెక్క, &want);
        ఒకవేళ (mem_check != m3Err_none) {
            తిరిగి mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_read(fd, iovs, లెక్క, &పూర్తి);
        ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        మొత్తం += పూర్తి;
        ఒకవేళ (పూర్తి < want) {
            ఆపు;
        }
    }

    m3ApiWriteMem32(nread, మొత్తం);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nwritten, పరిమాణం(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   మొత్తం = 0;

    కొరకు (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t లెక్క = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, పూర్తి;

        స్థిరరాశి శూన్య* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, లెక్క, &want);
        ఒకవేళ (mem_check != m3Err_none) {
            తిరిగి mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_write(fd, iovs, లెక్క, &పూర్తి);
        ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        మొత్తం += పూర్తి;
        ఒకవేళ (పూర్తి < want) {
            ఆపు;
        }
    }

    m3ApiWriteMem32(nwritten, మొత్తం);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_pread)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArgMem(__wasi_size_t*, nread)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nread, పరిమాణం(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   మొత్తం = 0;

    కొరకు (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t లెక్క = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, పూర్తి;

        స్థిరరాశి శూన్య* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, లెక్క, &want);
        ఒకవేళ (mem_check != m3Err_none) {
            తిరిగి mem_check;
        }

        // the offset walks with the data, so a short batch leaves no hole behind it
        __wasi_errno_t err = m3_wasi_host_fd_pread(fd, iovs, లెక్క, offset + మొత్తం, &పూర్తి);
        ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        మొత్తం += పూర్తి;
        ఒకవేళ (పూర్తి < want) {
            ఆపు;
        }
    }

    m3ApiWriteMem32(nread, మొత్తం);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_pwrite)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * పరిమాణం(wasi_iovec_t));
    m3ApiCheckMem(nwritten, పరిమాణం(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   మొత్తం = 0;

    కొరకు (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t లెక్క = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, పూర్తి;

        స్థిరరాశి శూన్య* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, లెక్క, &want);
        ఒకవేళ (mem_check != m3Err_none) {
            తిరిగి mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_pwrite(fd, iovs, లెక్క, offset + మొత్తం, &పూర్తి);
        ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        మొత్తం += పూర్తి;
        ఒకవేళ (పూర్తి < want) {
            ఆపు;
        }
    }

    m3ApiWriteMem32(nwritten, మొత్తం);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_close)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)

    m3ApiReturn(m3_wasi_host_fd_close(fd));
}

m3ApiRawFunction(m3_wasi_p1_fd_datasync)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)

    m3ApiReturn(m3_wasi_host_fd_datasync(fd));
}

m3ApiRawFunction(m3_wasi_p1_random_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint8_t*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)

    m3ApiCheckMem(buf, buf_len);

#  ఒకవేళ d_m3DeterministicProfile
    Deterministic_Random(runtime, buf, buf_len);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  లేకపోతే
    m3ApiReturn(m3_wasi_host_random_get(buf, buf_len));
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_res_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArgMem(__wasi_timestamp_t*, resolution)

    m3ApiCheckMem(resolution, పరిమాణం(__wasi_timestamp_t));

#  ఒకవేళ d_m3DeterministicProfile
    // the deterministic clock's resolution is the step it moves in
    m3ApiWriteMem64(resolution, d_m3DeterministicTickNs);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  లేకపోతే
    __wasi_timestamp_t res;

    __wasi_errno_t err = m3_wasi_host_clock_res_get(wasi_clk_id, &res);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(resolution, res);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_సమయం_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, పరిమాణం(__wasi_timestamp_t));

#  ఒకవేళ d_m3DeterministicProfile
    // Every clock the guest can name is the same underlying count - a runtime that is
    // not measuring the machine has nothing to tell process time and thread time apart
    // with - and only where it is counted from differs
    __wasi_timestamp_t now = Deterministic_సమయం(runtime);

    ఒకవేళ (wasi_clk_id == __WASI_CLOCKID_REALTIME) {
        now += d_m3WasiDeterministicEpochNs;
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  లేకపోతే
    __wasi_timestamp_t now;

    __wasi_errno_t err = m3_wasi_host_clock_సమయం_get(wasi_clk_id, &now);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
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

m3ApiRawFunction(m3_wasi_p1_sched_yield)
{
    m3ApiReturnType(uint32_t)

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

// Reads the guest's subscription list, allocating the host form of it. NULL if there
// is no memory for that; the caller frees what comes back.
//
// 48 bytes a record: userdata 0, type 8, fd 16, and a clock's fields from 16.
// $precision at 32 says how much longer the host may wait to coalesce this with
// another event, and waiting exactly as long as asked is always a valid answer to
// that, so it is not read.
స్థిర
m3_wasi_pollsub_t* read_subscriptions (స్థిరరాశి uint8_t* i_subs, __wasi_size_t i_లెక్క)
{
    m3_wasi_pollsub_t* subs =
      (m3_wasi_pollsub_t*)m3_Malloc("WASI subscriptions", i_లెక్క * పరిమాణం(m3_wasi_pollsub_t));

    ఒకవేళ (subs == ఖాళీ) {
        తిరిగి ఖాళీ;
    }

    కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
        స్థిరరాశి uint8_t* s = i_subs + (size_t)i * d_m3_Wasi_Subscription_కొలత;

        memset(&subs[i], 0, పరిమాణం(subs[i]));

        subs[i].userdata = m3ApiReadMem64(s + 0);
        subs[i].type     = m3ApiReadMem8(s + 8);

        ఒకవేళ (subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            subs[i].clockId = m3ApiReadMem32(s + 16);
            subs[i].timeout = m3ApiReadMem64(s + 24);
            subs[i].isAbsolute =
              (m3ApiReadMem16(s + 40) & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) != 0;
        } లేకపోతే {
            subs[i].fd = m3ApiReadMem32(s + 16);
        }
    }

    తిరిగి subs;
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
స్థిర
శూన్య write_event (uint8_t* o_rec, స్థిరరాశి m3_wasi_pollevent_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_కొలత);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->flags);
}

#  ఒకవేళ d_m3DeterministicProfile

// poll_oneoff under the deterministic profile, which cannot wait on anything real. A
// descriptor is reported ready at once - which is what the spec says a regular file
// always is, and the only answer here that does not depend on how fast something
// else is going - and the clock is moved forward to the soonest deadline, so that a
// guest sleeping still sees the time it slept for.
స్థిర
శూన్య poll_deterministically (IM3Runtime io_runtime, స్థిరరాశి m3_wasi_pollsub_t* i_subs,
                             __wasi_size_t i_లెక్క, m3_wasi_pollevent_t* o_events,
                             __wasi_size_t* o_అంకె_Events)
{
    __wasi_size_t బహిర్గతం     = 0;
    uint64_t      soonest = UINT64_MAX;
    uint64_t      now     = io_runtime->virtual_సమయం_Ns;

    కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
        స్థిరరాశి m3_wasi_pollsub_t* sub = &i_subs[i];

        ఒకవేళ (sub->type != __WASI_EVENTTYPE_CLOCK) {
            o_events[బహిర్గతం].userdata = sub->userdata;
            o_events[బహిర్గతం].error    = __WASI_ERRNO_SUCCESS;
            o_events[బహిర్గతం].type     = sub->type;
            o_events[బహిర్గతం].nbytes   = 1;
            o_events[బహిర్గతం].flags    = 0;
            బహిర్గతం++;
            కొనసాగు;
        }

        // an absolute deadline is against the same virtual clock the guest reads
        uint64_t deadline = sub->isAbsolute ? sub->timeout : now + sub->timeout;
        soonest           = M3_MIN(soonest, deadline);
    }

    // Something is ready already, so no time passes - which is what poll() would
    // have done too
    ఒకవేళ (బహిర్గతం) {
        *o_అంకె_Events = బహిర్గతం;
        తిరిగి;
    }

    ఒకవేళ (soonest > now) {
        Deterministic_Advance(io_runtime, soonest - now);
    }

    now = io_runtime->virtual_సమయం_Ns;

    కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
        స్థిరరాశి m3_wasi_pollsub_t* sub = &i_subs[i];

        ఒకవేళ (sub->type != __WASI_EVENTTYPE_CLOCK) {
            కొనసాగు;
        }

        uint64_t deadline = sub->isAbsolute ? sub->timeout : soonest;

        ఒకవేళ (deadline <= now) {
            o_events[బహిర్గతం].userdata = sub->userdata;
            o_events[బహిర్గతం].error    = __WASI_ERRNO_SUCCESS;
            o_events[బహిర్గతం].type     = __WASI_EVENTTYPE_CLOCK;
            o_events[బహిర్గతం].nbytes   = 0;
            o_events[బహిర్గతం].flags    = 0;
            బహిర్గతం++;
        }
    }

    *o_అంకె_Events = బహిర్గతం;
}

#  endif // d_m3DeterministicProfile

// Waits on the subscriptions, once whichever reader above has decoded them, and
// writes the events back. Knows nothing about revisions: the event record is the
// same in both, and the caller has bounds-checked o_events for i_count of them.
//
// The host layer is handed the whole list at once rather than a batch at a time -
// unlike the transfer calls - because waiting on half of a list is not waiting on
// the list: an event on a descriptor in the second half would go unnoticed for as
// long as the first half took.
స్థిర
__wasi_errno_t poll_subscriptions (IM3Runtime i_runtime, స్థిరరాశి m3_wasi_pollsub_t* i_subs,
                                   uint8_t* o_events, __wasi_size_t i_లెక్క,
                                   __wasi_size_t* o_అంకె_Events)
{
    *o_అంకె_Events = 0;

    m3_wasi_pollevent_t* evts =
      (m3_wasi_pollevent_t*)m3_Malloc("WASI events", i_లెక్క * పరిమాణం(m3_wasi_pollevent_t));

    ఒకవేళ (evts == ఖాళీ) {
        తిరిగి __WASI_ERRNO_NOMEM;
    }

    __wasi_size_t  అంకె_Events = 0;
    __wasi_errno_t err       = __WASI_ERRNO_SUCCESS;

#  ఒకవేళ d_m3DeterministicProfile
    poll_deterministically(i_runtime, i_subs, i_లెక్క, evts, &అంకె_Events);
#  లేకపోతే
    err = m3_wasi_host_poll_oneoff(i_subs, i_లెక్క, evts, &అంకె_Events);
#  endif

    ఒకవేళ (err == __WASI_ERRNO_SUCCESS) {
        కొరకు (__wasi_size_t i = 0; i < అంకె_Events; i++) {
            write_event(o_events + (size_t)i * d_m3_Wasi_Event_కొలత, &evts[i]);
        }
        *o_అంకె_Events = అంకె_Events;
    }

    m3_Free(evts);

    తిరిగి err;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(స్థిరరాశి uint8_t*, లోపల)
    m3ApiGetArgMem(uint8_t*, బహిర్గతం)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(లోపల, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_కొలత);
    m3ApiCheckMem(బహిర్గతం, (uint64_t)nsubscriptions * d_m3_Wasi_Event_కొలత);
    m3ApiCheckMem(nevents, పరిమాణం(__wasi_size_t));

    ఒకవేళ (nsubscriptions == 0) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3_wasi_pollsub_t* subs = read_subscriptions(లోపల, nsubscriptions);

    ఒకవేళ (subs == ఖాళీ) {
        m3ApiReturn(__WASI_ERRNO_NOMEM);
    }

    __wasi_size_t  అంకె_Events = 0;
    __wasi_errno_t err       = poll_subscriptions(runtime, subs, బహిర్గతం, nsubscriptions, &అంకె_Events);

    m3_Free(subs);

    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(nevents, అంకె_Events);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
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

    m3_wasi_host_init();

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
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_సమితి_flags)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",      "i(i*)",   &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_సమితి_కొలత)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_సమితి_times)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_pread",             "i(i*iI*)", &m3_wasi_p1_fd_pread)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_prestat_get",       "i(i*)",   &m3_wasi_p1_fd_prestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_prestat_dir_name",  "i(i*i)",  &m3_wasi_p1_fd_prestat_dir_name)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_pwrite",            "i(i*iI*)", &m3_wasi_p1_fd_pwrite)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_read",              "i(i*i*)", &m3_wasi_p1_fd_read)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_readdir",           "i(i*iI*)",)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_renumber",          "i(ii)",   )));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_sync",              "i(i)",    )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_seek",              "i(iIi*)", &m3_wasi_p1_fd_seek)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_tell",              "i(i*)",   &m3_wasi_p1_fd_tell)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_write",             "i(i*i*)", &m3_wasi_p1_fd_write)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_మార్గం_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",        "i(ii*i*)",     &m3_wasi_p1_మార్గం_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   &m3_wasi_p1_మార్గం_link)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_మార్గం_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_మార్గం_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_మార్గం_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_మార్గం_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_మార్గం_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_మార్గం_unlink_file)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "poll_oneoff",          "i(**i*)", &m3_wasi_p1_poll_oneoff)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "proc_exit",          "v(i)",    &m3_wasi_p1_proc_exit, wasi_context)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "proc_raise",           "i(i)",    )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "random_get",           "i(*i)",   &m3_wasi_p1_random_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "sched_yield",          "i()",     &m3_wasi_p1_sched_yield)));

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

#అయితే_ముగింపు // d_m3HasWASI
