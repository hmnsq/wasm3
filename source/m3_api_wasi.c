//
//  m3_api_wasi.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_api_wasi.h"

#ಅನ್ತರ್ಭಾವ "m3_api_wasi_host.h"
#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_deterministic.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"

#ಯದಿ defined(d_m3HasWASI)

#  include <string.h>

// The system underneath, of which exactly one is ever built. They are headers rather
// than translation units of their own so that neither has to be added to a source
// list - and most of those are build scripts outside this repository.
#  ಯದಿ defined(_WIN32)
#    include "m3_api_wasi_win32.h"
#  ಅನ್ಯಥಾ
#    include "m3_api_wasi_posix.h"
#  endif

// This file is the WASI interface and nothing below it: it decodes the guest's
// arguments, bounds-checks every pointer against guest memory, and writes results
// back in the layout the calling revision asks for. The system underneath is reached
// through m3_api_wasi_host.h, which m3_api_wasi_posix.h and m3_api_wasi_win32.h
// implement independently of each other.

ಸ್ಥಿರ m3_wasi_context_t* wasi_context;

// A scatter/gather list as the guest lays one out
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ wasi_iovec_t {
    __wasi_size_t buf;
    __wasi_size_t buf_len;
} wasi_iovec_t;

// The records the guest passes by address, sized as the ABI lays them out rather
// than as this host happens to - see write_filestat, read_subscriptions, write_event.
#  define d_m3WasiFilestatSize      64
#  define d_m3WasiSubscriptionSize  48
#  define d_m3WasiEventSize         32

ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ರಿಕ್ತ,
};

// Copies a path out of guest memory so it can be handed to the host NUL-terminated.
// The caller has already bounds-checked (i_path, i_pathLen) against guest memory.
ಸ್ಥಿರ
ಪೂರ್ಣಾಙ್ಕ copy_path (ವರ್ಣ* o_path, ಧ್ರುವ ವರ್ಣ* i_path, __wasi_size_t i_pathLen)
{
    ಯದಿ (i_pathLen >= d_m3WasiMaxPath) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }
    memcpy(o_path, i_path, i_pathLen);
    o_path[i_pathLen] = '\0';
    ಪ್ರತ್ಯಾಗಮ 1;
}

// Writes a filestat out: dev 0, ino 8, filetype 16, nlink 24, size 32, atim 40,
// mtim 48, ctim 56. The caller has checked the buffer for d_m3WasiFilestatSize.
ಸ್ಥಿರ
ಶೂನ್ಯ write_filestat (uint8_t* o_buf, ಧ್ರುವ m3_wasi_filestat_t* i_stat)
{
    memset(o_buf, 0, d_m3WasiFilestatSize);

    m3ApiWriteMem64(o_buf + 0, i_stat->dev);
    m3ApiWriteMem64(o_buf + 8, i_stat->ino);
    m3ApiWriteMem8(o_buf + 16, i_stat->filetype);
    m3ApiWriteMem64(o_buf + 24, i_stat->nlink);
    m3ApiWriteMem64(o_buf + 32, i_stat->size);
    m3ApiWriteMem64(o_buf + 40, i_stat->atim);
    m3ApiWriteMem64(o_buf + 48, i_stat->mtim);
    m3ApiWriteMem64(o_buf + 56, i_stat->ctim);
}

// Resolves the guest's scatter/gather list to host addresses, checking each buffer
// against guest memory, and adds up how much they ask for. The host layer is handed
// the result and never sees a guest offset.
ಸ್ಥಿರ
ಧ್ರುವ ಶೂನ್ಯ* resolve_iovs (IM3Runtime runtime, ಶೂನ್ಯ* _mem, m3_wasi_iovec_t* o_iovs,
                          ಧ್ರುವ wasi_iovec_t* i_wasiIovs, __wasi_size_t i_ಎಣಿಕೆ,
                          __wasi_size_t* o_want)
{
    *o_want = 0;

    __wasi_size_t want = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        o_iovs[i].buf = m3ApiOffsetToPtr(m3ApiReadMem32(&i_wasiIovs[i].buf));
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

m3ApiRawFunction(m3_wasi_p1_args_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(ವರ್ಣ*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಯದಿ (context == ರಿಕ್ತ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * ಪರಿಮಾಣ(uint32_t));

    ಪ್ರತಿ (u32 i = 0; i < context->argc; ++i) {
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

    m3ApiCheckMem(argc, ಪರಿಮಾಣ(__wasi_size_t));
    m3ApiCheckMem(argv_buf_size, ಪರಿಮಾಣ(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಯದಿ (context == ರಿಕ್ತ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    ಪ್ರತಿ (u32 i = 0; i < context->argc; ++i) {
        buf_len += (__wasi_size_t)strlen(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_size, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ವರ್ಣ*, env_buf)

    __wasi_size_t ಎಣಿಕೆ = 0;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        ಎಣಿಕೆ++;
    }

    m3ApiCheckMem(env, ಎಣಿಕೆ * ಪರಿಮಾಣ(uint32_t));

    ಪ್ರತಿ (u32 i = 0; i < ಎಣಿಕೆ; ++i) {
        m3ApiWriteMem32(&env[i], m3ApiPtrToOffset(env_buf));

        size_t len = strlen(DEFAULT_ENVIRONMENT[i]);

        m3ApiCheckMem(env_buf, len + 1);        // include the terminating `\0`
        memcpy(env_buf, DEFAULT_ENVIRONMENT[i], len);
        env_buf += len;
        *env_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_ಎಣಿಕೆ)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_size)

    m3ApiCheckMem(env_ಎಣಿಕೆ, ಪರಿಮಾಣ(__wasi_size_t));
    m3ApiCheckMem(env_buf_size, ಪರಿಮಾಣ(__wasi_size_t));

    __wasi_size_t ಎಣಿಕೆ = 0, buf_len = 0;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        ಎಣಿಕೆ++;
        buf_len += (__wasi_size_t)strlen(*e) + 1;
    }

    m3ApiWriteMem32(env_ಎಣಿಕೆ, ಎಣಿಕೆ);
    m3ApiWriteMem32(env_buf_size, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    ಯದಿ (fd < 3 || fd >= d_m3_Wasi_Preopen_ಎಣಿಕೆ) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }
    size_t slen = strlen(m3_wasi_preopen[fd].path) + 1;
    memcpy(path, m3_wasi_preopen[fd].path, M3_MIN(slen, path_len));
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 8);

    ಯದಿ (fd < 3 || fd >= d_m3_Wasi_Preopen_ಎಣಿಕೆ) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }

    m3ApiWriteMem32(buf + 0, __WASI_PREOPENTYPE_DIR);
    m3ApiWriteMem32(buf + 4, (u32)strlen(m3_wasi_preopen[fd].path) + 1);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_fdstat_t*, fdstat)

    m3ApiCheckMem(fdstat, ಪರಿಮಾಣ(__wasi_fdstat_t));

    __wasi_filetype_t filetype;
    __wasi_fdflags_t  flags;

    __wasi_errno_t err = m3_wasi_host_fd_fdstat(fd, &filetype, &flags);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    fdstat->fs_filetype = filetype;
    m3ApiWriteMem16(&fdstat->fs_flags, flags);

    fdstat->fs_rights_base = (uint64_t)-1; // all rights

    // Make descriptors 0,1,2 look like a TTY
    ಯದಿ (fd <= 2) {
        fdstat->fs_rights_base &= ~(__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
    }

    fdstat->fs_rights_inheriting = (uint64_t)-1; // all rights
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_set_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_fdstat_set_flags(fd, flags));
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, length)
    m3ApiGetArg(__wasi_advice_t, advice)

    m3ApiReturn(m3_wasi_host_fd_advise(fd, offset, length, advice));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, d_m3WasiFilestatSize);

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t err = m3_wasi_host_fd_filestat(fd, &fd_stat);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &fd_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_path_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, d_m3WasiFilestatSize);

    ವರ್ಣ host_path[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_path, path, path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3_wasi_filestat_t path_stat;
    bool               follow = (dirflags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    __wasi_errno_t err = m3_wasi_host_path_filestat(dirfd, host_path, follow, &path_stat);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &path_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_set_size)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, size)

    m3ApiReturn(m3_wasi_host_fd_set_size(fd, size));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_set_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_timestamp_t, atim)
    m3ApiGetArg(__wasi_timestamp_t, mtim)
    m3ApiGetArg(__wasi_fstflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_set_times(fd, atim, mtim, flags));
}

m3ApiRawFunction(m3_wasi_p1_path_link)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, old_flags)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    ವರ್ಣ host_old[d_m3WasiMaxPath];
    ವರ್ಣ host_new[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_old, old_path, old_path_len) ||
        !copy_path(host_new, new_path, new_path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    bool follow = (old_flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    m3ApiReturn(m3_wasi_host_path_link(old_dirfd, host_old, follow, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_path_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    ವರ್ಣ host_path[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_path, path, path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_path_create_directory(dirfd, host_path));
}

m3ApiRawFunction(m3_wasi_p1_path_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    ವರ್ಣ host_path[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_path, path, path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_path_remove_directory(dirfd, host_path));
}

m3ApiRawFunction(m3_wasi_p1_path_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)

    m3ApiCheckMem(path, path_len);

    ವರ್ಣ host_path[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_path, path, path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_path_unlink_file(dirfd, host_path));
}

m3ApiRawFunction(m3_wasi_p1_path_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    ವರ್ಣ host_old[d_m3WasiMaxPath];
    ವರ್ಣ host_new[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_old, old_path, old_path_len) ||
        !copy_path(host_new, new_path, new_path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_path_rename(old_dirfd, host_old, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_path_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, old_path)
    m3ApiGetArg(__wasi_size_t, old_path_len)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, new_path)
    m3ApiGetArg(__wasi_size_t, new_path_len)

    m3ApiCheckMem(old_path, old_path_len);
    m3ApiCheckMem(new_path, new_path_len);

    // the target is stored as given, so it is a string rather than a path to resolve
    ವರ್ಣ host_old[d_m3WasiMaxPath];
    ವರ್ಣ host_new[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_old, old_path, old_path_len) ||
        !copy_path(host_new, new_path, new_path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_path_symlink(host_old, dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_path_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArgMem(ವರ್ಣ*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ಪರಿಮಾಣ(__wasi_size_t));

    ವರ್ಣ host_path[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_path, path, path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_size_t used;

    __wasi_errno_t err = m3_wasi_host_path_readlink(dirfd, host_path, buf, buf_len, &used);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(bufused, used);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_tell)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_filesize_t*, result)

    m3ApiCheckMem(result, ಪರಿಮಾಣ(__wasi_filesize_t));

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, 0, SEEK_CUR, &pos);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(result, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, result)

    m3ApiCheckMem(result, ಪರಿಮಾಣ(__wasi_filesize_t));

    ಪೂರ್ಣಾಙ್ಕ whence;

    ವಿಕಲ್ಪ (wasi_whence) {
    ಪಕ್ಷ 0: whence = SEEK_SET; ಭಙ್ಗ;
    ಪಕ್ಷ 1: whence = SEEK_CUR; ಭಙ್ಗ;
    ಪಕ್ಷ 2: whence = SEEK_END; ಭಙ್ಗ;
    ಸ್ವಭಾವ: m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, offset, whence, &pos);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(result, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


m3ApiRawFunction(m3_wasi_p1_path_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ಧ್ರುವ ವರ್ಣ*, path)
    m3ApiGetArg(__wasi_size_t, path_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_base)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(path, path_len);
    m3ApiCheckMem(fd, ಪರಿಮಾಣ(__wasi_fd_t));

    ವರ್ಣ host_path[d_m3WasiMaxPath];
    ಯದಿ (!copy_path(host_path, path, path_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_fd_t host_fd;

    __wasi_errno_t err = m3_wasi_host_path_open(dirfd, host_path, oflags, fs_rights_base,
                                                fs_flags, &host_fd);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nread, ಪರಿಮಾಣ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   total = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ಎಣಿಕೆ = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, done;

        ಧ್ರುವ ಶೂನ್ಯ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ಎಣಿಕೆ, &want);
        ಯದಿ (mem_check != m3Err_none) {
            ಪ್ರತ್ಯಾಗಮ mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_read(fd, iovs, ಎಣಿಕೆ, &done);
        ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        total += done;
        ಯದಿ (done < want) {
            ಭಙ್ಗ;
        }
    }

    m3ApiWriteMem32(nread, total);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಪರಿಮಾಣ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   total = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ಎಣಿಕೆ = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, done;

        ಧ್ರುವ ಶೂನ್ಯ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ಎಣಿಕೆ, &want);
        ಯದಿ (mem_check != m3Err_none) {
            ಪ್ರತ್ಯಾಗಮ mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_write(fd, iovs, ಎಣಿಕೆ, &done);
        ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        total += done;
        ಯದಿ (done < want) {
            ಭಙ್ಗ;
        }
    }

    m3ApiWriteMem32(nwritten, total);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nread, ಪರಿಮಾಣ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   total = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ಎಣಿಕೆ = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, done;

        ಧ್ರುವ ಶೂನ್ಯ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ಎಣಿಕೆ, &want);
        ಯದಿ (mem_check != m3Err_none) {
            ಪ್ರತ್ಯಾಗಮ mem_check;
        }

        // the offset walks with the data, so a short batch leaves no hole behind it
        __wasi_errno_t err = m3_wasi_host_fd_pread(fd, iovs, ಎಣಿಕೆ, offset + total, &done);
        ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        total += done;
        ಯದಿ (done < want) {
            ಭಙ್ಗ;
        }
    }

    m3ApiWriteMem32(nread, total);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ಪರಿಮಾಣ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ಪರಿಮಾಣ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   total = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ಎಣಿಕೆ = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, done;

        ಧ್ರುವ ಶೂನ್ಯ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ಎಣಿಕೆ, &want);
        ಯದಿ (mem_check != m3Err_none) {
            ಪ್ರತ್ಯಾಗಮ mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_pwrite(fd, iovs, ಎಣಿಕೆ, offset + total, &done);
        ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        total += done;
        ಯದಿ (done < want) {
            ಭಙ್ಗ;
        }
    }

    m3ApiWriteMem32(nwritten, total);
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

#  ಯದಿ d_m3DeterministicProfile
    Deterministic_Random(runtime, buf, buf_len);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  ಅನ್ಯಥಾ
    m3ApiReturn(m3_wasi_host_random_get(buf, buf_len));
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_res_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArgMem(__wasi_timestamp_t*, resolution)

    m3ApiCheckMem(resolution, ಪರಿಮಾಣ(__wasi_timestamp_t));

#  ಯದಿ d_m3DeterministicProfile
    // the deterministic clock's resolution is the step it moves in
    m3ApiWriteMem64(resolution, d_m3DeterministicTickNs);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  ಅನ್ಯಥಾ
    __wasi_timestamp_t res;

    __wasi_errno_t err = m3_wasi_host_clock_res_get(wasi_clk_id, &res);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(resolution, res);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_time_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, ಪರಿಮಾಣ(__wasi_timestamp_t));

#  ಯದಿ d_m3DeterministicProfile
    // Every clock the guest can name is the same underlying count - a runtime that is
    // not measuring the machine has nothing to tell process time and thread time apart
    // with - and only where it is counted from differs
    __wasi_timestamp_t now = Deterministic_Time(runtime);

    ಯದಿ (wasi_clk_id == __WASI_CLOCKID_REALTIME) {
        now += d_m3WasiDeterministicEpochNs;
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  ಅನ್ಯಥಾ
    __wasi_timestamp_t now;

    __wasi_errno_t err = m3_wasi_host_clock_time_get(wasi_clk_id, &now);
    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, code)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ಯದಿ (context) {
        context->exit_code = code;
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
ಸ್ಥಿರ
m3_wasi_pollsub_t* read_subscriptions (ಧ್ರುವ uint8_t* i_subs, __wasi_size_t i_ಎಣಿಕೆ)
{
    m3_wasi_pollsub_t* subs =
      (m3_wasi_pollsub_t*)m3_Malloc("WASI subscriptions", i_ಎಣಿಕೆ * ಪರಿಮಾಣ(m3_wasi_pollsub_t));

    ಯದಿ (subs == ರಿಕ್ತ) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಧ್ರುವ uint8_t* s = i_subs + (size_t)i * d_m3WasiSubscriptionSize;

        memset(&subs[i], 0, ಪರಿಮಾಣ(subs[i]));

        subs[i].userdata = m3ApiReadMem64(s + 0);
        subs[i].type     = m3ApiReadMem8(s + 8);

        ಯದಿ (subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            subs[i].clockId = m3ApiReadMem32(s + 16);
            subs[i].timeout = m3ApiReadMem64(s + 24);
            subs[i].isAbsolute =
              (m3ApiReadMem16(s + 40) & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) != 0;
        } ಅನ್ಯಥಾ {
            subs[i].fd = m3ApiReadMem32(s + 16);
        }
    }

    ಪ್ರತ್ಯಾಗಮ subs;
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ಸ್ಥಿರ
ಶೂನ್ಯ write_event (uint8_t* o_rec, ಧ್ರುವ m3_wasi_pollevent_t* i_event)
{
    memset(o_rec, 0, d_m3WasiEventSize);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->flags);
}

#  ಯದಿ d_m3DeterministicProfile

// poll_oneoff under the deterministic profile, which cannot wait on anything real. A
// descriptor is reported ready at once - which is what the spec says a regular file
// always is, and the only answer here that does not depend on how fast something
// else is going - and the clock is moved forward to the soonest deadline, so that a
// guest sleeping still sees the time it slept for.
ಸ್ಥಿರ
ಶೂನ್ಯ poll_deterministically (IM3Runtime io_runtime, ಧ್ರುವ m3_wasi_pollsub_t* i_subs,
                             __wasi_size_t i_ಎಣಿಕೆ, m3_wasi_pollevent_t* o_events,
                             __wasi_size_t* o_ಅಂಕೆ_Events)
{
    __wasi_size_t out     = 0;
    uint64_t      soonest = UINT64_MAX;
    uint64_t      now     = io_runtime->virtualTimeNs;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಧ್ರುವ m3_wasi_pollsub_t* sub = &i_subs[i];

        ಯದಿ (sub->type != __WASI_EVENTTYPE_CLOCK) {
            o_events[out].userdata = sub->userdata;
            o_events[out].error    = __WASI_ERRNO_SUCCESS;
            o_events[out].type     = sub->type;
            o_events[out].nbytes   = 1;
            o_events[out].flags    = 0;
            out++;
            ಅನುವರ್ತ;
        }

        // an absolute deadline is against the same virtual clock the guest reads
        uint64_t deadline = sub->isAbsolute ? sub->timeout : now + sub->timeout;
        soonest           = M3_MIN(soonest, deadline);
    }

    // Something is ready already, so no time passes - which is what poll() would
    // have done too
    ಯದಿ (out) {
        *o_ಅಂಕೆ_Events = out;
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಯದಿ (soonest > now) {
        Deterministic_Advance(io_runtime, soonest - now);
    }

    now = io_runtime->virtualTimeNs;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಧ್ರುವ m3_wasi_pollsub_t* sub = &i_subs[i];

        ಯದಿ (sub->type != __WASI_EVENTTYPE_CLOCK) {
            ಅನುವರ್ತ;
        }

        uint64_t deadline = sub->isAbsolute ? sub->timeout : soonest;

        ಯದಿ (deadline <= now) {
            o_events[out].userdata = sub->userdata;
            o_events[out].error    = __WASI_ERRNO_SUCCESS;
            o_events[out].type     = __WASI_EVENTTYPE_CLOCK;
            o_events[out].nbytes   = 0;
            o_events[out].flags    = 0;
            out++;
        }
    }

    *o_ಅಂಕೆ_Events = out;
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
ಸ್ಥಿರ
__wasi_errno_t poll_subscriptions (IM3Runtime i_runtime, ಧ್ರುವ m3_wasi_pollsub_t* i_subs,
                                   uint8_t* o_events, __wasi_size_t i_ಎಣಿಕೆ,
                                   __wasi_size_t* o_ಅಂಕೆ_Events)
{
    *o_ಅಂಕೆ_Events = 0;

    m3_wasi_pollevent_t* evts =
      (m3_wasi_pollevent_t*)m3_Malloc("WASI events", i_ಎಣಿಕೆ * ಪರಿಮಾಣ(m3_wasi_pollevent_t));

    ಯದಿ (evts == ರಿಕ್ತ) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOMEM;
    }

    __wasi_size_t  ಅಂಕೆ_Events = 0;
    __wasi_errno_t err       = __WASI_ERRNO_SUCCESS;

#  ಯದಿ d_m3DeterministicProfile
    poll_deterministically(i_runtime, i_subs, i_ಎಣಿಕೆ, evts, &ಅಂಕೆ_Events);
#  ಅನ್ಯಥಾ
    err = m3_wasi_host_poll_oneoff(i_subs, i_ಎಣಿಕೆ, evts, &ಅಂಕೆ_Events);
#  endif

    ಯದಿ (err == __WASI_ERRNO_SUCCESS) {
        ಪ್ರತಿ (__wasi_size_t i = 0; i < ಅಂಕೆ_Events; i++) {
            write_event(o_events + (size_t)i * d_m3WasiEventSize, &evts[i]);
        }
        *o_ಅಂಕೆ_Events = ಅಂಕೆ_Events;
    }

    m3_Free(evts);

    ಪ್ರತ್ಯಾಗಮ err;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ಧ್ರುವ uint8_t*, in)
    m3ApiGetArgMem(uint8_t*, out)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(in, (uint64_t)nsubscriptions * d_m3WasiSubscriptionSize);
    m3ApiCheckMem(out, (uint64_t)nsubscriptions * d_m3WasiEventSize);
    m3ApiCheckMem(nevents, ಪರಿಮಾಣ(__wasi_size_t));

    ಯದಿ (nsubscriptions == 0) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3_wasi_pollsub_t* subs = read_subscriptions(in, nsubscriptions);

    ಯದಿ (subs == ರಿಕ್ತ) {
        m3ApiReturn(__WASI_ERRNO_NOMEM);
    }

    __wasi_size_t  ಅಂಕೆ_Events = 0;
    __wasi_errno_t err       = poll_subscriptions(runtime, subs, out, nsubscriptions, &ಅಂಕೆ_Events);

    m3_Free(subs);

    ಯದಿ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(nevents, ಅಂಕೆ_Events);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


ಸ್ಥಿರ
M3Result SuppressLookupFailure (M3Result i_result)
{
    ಯದಿ (i_result == m3Err_functionLookupFailed) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ i_result;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    ಪ್ರತ್ಯಾಗಮ wasi_context;
}

ಶೂನ್ಯ m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
M3Result _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3Result result = m3Err_none;

    m3_wasi_host_init();

    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ಪ್ರತಿ (size_t i = 0; i < M3_ಎಣಿಕೆ_OF(namespaces); i++) {
        ಧ್ರುವ ವರ್ಣ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_args_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_args_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_time_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_set_flags)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",      "i(i*)",   &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_set_size)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_set_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_path_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",        "i(ii*i*)",     &m3_wasi_p1_path_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   &m3_wasi_p1_path_link)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_path_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_path_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_path_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_path_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_path_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_path_unlink_file)));

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
    ಯದಿ (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 memoryIndex;

        ಯದಿ (m3_FindExportedMemory(module, "memory", &memoryIndex)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_BindImportMemory(module, "wasi_snapshot_preview1", memoryIndex));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

M3Result m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಶೂನ್ಯ m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3Result m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ಪ್ರತ್ಯಾಗಮ _linkWASI(module, wasi);
}

M3Result m3_LinkWASI (IM3Module module)
{
    ಯದಿ (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    ಪ್ರತ್ಯಾಗಮ _linkWASI(module, wasi_context);
}

#endif // d_m3HasWASI
