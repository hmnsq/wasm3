//
//  m3_api_wasi.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_wasi.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_wasi_host.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_deterministic.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"

#ਜੇ defined(d_m3HasWASI)

#  include <string.h>

// The system underneath, of which exactly one is ever built. They are headers rather
// than translation units of their own so that neither has to be added to a source
// list - and most of those are build scripts outside this repository.
#  ਜੇ defined(_WIN32)
#    include "m3_api_wasi_win32.h"
#  ਨਹੀਂਤਾਂ
#    include "m3_api_wasi_posix.h"
#  endif

// This file is the WASI interface and nothing below it: it decodes the guest's
// arguments, bounds-checks every pointer against guest memory, and writes results
// back in the layout the calling revision asks for. The system underneath is reached
// through m3_api_wasi_host.h, which m3_api_wasi_posix.h and m3_api_wasi_win32.h
// implement independently of each other.

ਸਥਿਰ m3_wasi_context_t* wasi_context;

// A scatter/gather list as the guest lays one out
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ wasi_iovec_t {
    __wasi_size_t buf;
    __wasi_size_t buf_len;
} wasi_iovec_t;

// The records the guest passes by address, sized as the ABI lays them out rather
// than as this host happens to - see write_filestat, read_subscriptions, write_event.
#  define d_m3_Wasi_Filestat_ਮਾਪ      64
#  define d_m3_Wasi_Subscription_ਮਾਪ  48
#  define d_m3_Wasi_Event_ਮਾਪ         32

ਸਥਿਰ ਅਚਲ ਅੱਖਰ* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    ਨਿਰਾ,
};

// Copies a path out of guest memory so it can be handed to the host NUL-terminated.
// The caller has already bounds-checked (i_path, i_pathLen) against guest memory.
ਸਥਿਰ
ਪੂਰਨ copy_ਰਸਤਾ (ਅੱਖਰ* o_ਰਸਤਾ, ਅਚਲ ਅੱਖਰ* i_ਰਸਤਾ, __wasi_size_t i_ਰਸਤਾ_Len)
{
    ਜੇ (i_ਰਸਤਾ_Len >= d_m3_Wasi_Max_ਰਸਤਾ) {
        ਵਾਪਸ 0;
    }
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਰਸਤਾ, i_ਰਸਤਾ, i_ਰਸਤਾ_Len);
    o_ਰਸਤਾ[i_ਰਸਤਾ_Len] = '\0';
    ਵਾਪਸ 1;
}

// Writes a filestat out: dev 0, ino 8, filetype 16, nlink 24, size 32, atim 40,
// mtim 48, ctim 56. The caller has checked the buffer for d_m3WasiFilestatSize.
ਸਥਿਰ
ਖਾਲੀ write_filestat (uint8_t* o_buf, ਅਚਲ m3_wasi_filestat_t* i_stat)
{
    memset(o_buf, 0, d_m3_Wasi_Filestat_ਮਾਪ);

    m3ApiWriteMem64(o_buf + 0, i_stat->dev);
    m3ApiWriteMem64(o_buf + 8, i_stat->ino);
    m3ApiWriteMem8(o_buf + 16, i_stat->filetype);
    m3ApiWriteMem64(o_buf + 24, i_stat->nlink);
    m3ApiWriteMem64(o_buf + 32, i_stat->ਮਾਪ);
    m3ApiWriteMem64(o_buf + 40, i_stat->atim);
    m3ApiWriteMem64(o_buf + 48, i_stat->mtim);
    m3ApiWriteMem64(o_buf + 56, i_stat->ctim);
}

// Resolves the guest's scatter/gather list to host addresses, checking each buffer
// against guest memory, and adds up how much they ask for. The host layer is handed
// the result and never sees a guest offset.
ਸਥਿਰ
ਅਚਲ ਖਾਲੀ* resolve_iovs (IM3Runtime runtime, ਖਾਲੀ* _mem, m3_wasi_iovec_t* o_iovs,
                          ਅਚਲ wasi_iovec_t* i_wasiIovs, __wasi_size_t i_ਗਿਣਤੀ,
                          __wasi_size_t* o_want)
{
    *o_want = 0;

    __wasi_size_t want = 0;

    ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
        o_iovs[i].buf = m3_Api_ਵਿੱਥ_ਮੰਜ਼ਿਲ_Ptr(m3ApiReadMem32(&i_wasiIovs[i].buf));
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

m3ApiRawFunction(m3_wasi_p1_ਦਲੀਲਾਂ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(ਅੱਖਰ*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ਜੇ (context == ਨਿਰਾ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * ਆਕਾਰ(uint32_t));

    ਲਈ (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(argv_buf));

        size_t len = ਲੜੀ_ਲੰਬਾਈ(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_ਦਲੀਲਾਂ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_ਮਾਪ)

    m3ApiCheckMem(argc, ਆਕਾਰ(__wasi_size_t));
    m3ApiCheckMem(argv_buf_ਮਾਪ, ਆਕਾਰ(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ਜੇ (context == ਨਿਰਾ) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    ਲਈ (u32 i = 0; i < context->argc; ++i) {
        buf_len += (__wasi_size_t)ਲੜੀ_ਲੰਬਾਈ(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_ਮਾਪ, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(ਅੱਖਰ*, env_buf)

    __wasi_size_t ਗਿਣਤੀ_2 = 0;
    ਲਈ (ਅਚਲ ਅੱਖਰ** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        ਗਿਣਤੀ_2++;
    }

    m3ApiCheckMem(env, ਗਿਣਤੀ_2 * ਆਕਾਰ(uint32_t));

    ਲਈ (u32 i = 0; i < ਗਿਣਤੀ_2; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_ਮੰਜ਼ਿਲ_ਵਿੱਥ(env_buf));

        size_t len = ਲੜੀ_ਲੰਬਾਈ(DEFAULT_ENVIRONMENT[i]);

        m3ApiCheckMem(env_buf, len + 1);        // include the terminating `\0`
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(env_buf, DEFAULT_ENVIRONMENT[i], len);
        env_buf += len;
        *env_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_ਗਿਣਤੀ)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_ਮਾਪ)

    m3ApiCheckMem(env_ਗਿਣਤੀ, ਆਕਾਰ(__wasi_size_t));
    m3ApiCheckMem(env_buf_ਮਾਪ, ਆਕਾਰ(__wasi_size_t));

    __wasi_size_t ਗਿਣਤੀ_2 = 0, buf_len = 0;
    ਲਈ (ਅਚਲ ਅੱਖਰ** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        ਗਿਣਤੀ_2++;
        buf_len += (__wasi_size_t)ਲੜੀ_ਲੰਬਾਈ(*e) + 1;
    }

    m3ApiWriteMem32(env_ਗਿਣਤੀ, ਗਿਣਤੀ_2);
    m3ApiWriteMem32(env_buf_ਮਾਪ, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    ਜੇ (fd < 3 || fd >= d_m3_Wasi_Preopen_ਗਿਣਤੀ) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }
    size_t slen = ਲੜੀ_ਲੰਬਾਈ(m3_wasi_preopen[fd].ਰਸਤਾ) + 1;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(ਰਸਤਾ, m3_wasi_preopen[fd].ਰਸਤਾ, M3_MIN(slen, ਰਸਤਾ_len));
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 8);

    ਜੇ (fd < 3 || fd >= d_m3_Wasi_Preopen_ਗਿਣਤੀ) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }

    m3ApiWriteMem32(buf + 0, __WASI_PREOPENTYPE_DIR);
    m3ApiWriteMem32(buf + 4, (u32)ਲੜੀ_ਲੰਬਾਈ(m3_wasi_preopen[fd].ਰਸਤਾ) + 1);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_fdstat_t*, fdstat)

    m3ApiCheckMem(fdstat, ਆਕਾਰ(__wasi_fdstat_t));

    __wasi_filetype_t filetype;
    __wasi_fdflags_t  flags;

    __wasi_errno_t err = m3_wasi_host_fd_fdstat(fd, &filetype, &flags);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    fdstat->fs_filetype = filetype;
    m3ApiWriteMem16(&fdstat->fs_flags, flags);

    fdstat->fs_rights_ਅਧਾਰ = (uint64_t)-1; // all rights

    // Make descriptors 0,1,2 look like a TTY
    ਜੇ (fd <= 2) {
        fdstat->fs_rights_ਅਧਾਰ &= ~(__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
    }

    fdstat->fs_rights_inheriting = (uint64_t)-1; // all rights
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_ਸੈੱਟ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_fdstat_ਸੈੱਟ_flags(fd, flags));
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, ਲੰਮਾਈ)
    m3ApiGetArg(__wasi_advice_t, advice)

    m3ApiReturn(m3_wasi_host_fd_advise(fd, offset, ਲੰਮਾਈ, advice));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_ਮਾਪ);

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t err = m3_wasi_host_fd_filestat(fd, &fd_stat);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &fd_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_ਮਾਪ);

    ਅੱਖਰ host_ਰਸਤਾ[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_ਰਸਤਾ, ਰਸਤਾ, ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3_wasi_filestat_t ਰਸਤਾ_stat;
    bool               follow = (dirflags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    __wasi_errno_t err = m3_wasi_host_ਰਸਤਾ_filestat(dirfd, host_ਰਸਤਾ, follow, &ਰਸਤਾ_stat);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &ਰਸਤਾ_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ਸੈੱਟ_ਮਾਪ)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, ਮਾਪ)

    m3ApiReturn(m3_wasi_host_fd_ਸੈੱਟ_ਮਾਪ(fd, ਮਾਪ));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_ਸੈੱਟ_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_timestamp_t, atim)
    m3ApiGetArg(__wasi_timestamp_t, mtim)
    m3ApiGetArg(__wasi_fstflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_ਸੈੱਟ_times(fd, atim, mtim, flags));
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_link)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, old_flags)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, old_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, old_ਰਸਤਾ_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, new_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, new_ਰਸਤਾ_len)

    m3ApiCheckMem(old_ਰਸਤਾ, old_ਰਸਤਾ_len);
    m3ApiCheckMem(new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    ਅੱਖਰ host_old[d_m3_Wasi_Max_ਰਸਤਾ];
    ਅੱਖਰ host_new[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_old, old_ਰਸਤਾ, old_ਰਸਤਾ_len) ||
        !copy_ਰਸਤਾ(host_new, new_ਰਸਤਾ, new_ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    bool follow = (old_flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    m3ApiReturn(m3_wasi_host_ਰਸਤਾ_link(old_dirfd, host_old, follow, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    ਅੱਖਰ host_ਰਸਤਾ[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_ਰਸਤਾ, ਰਸਤਾ, ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_ਰਸਤਾ_create_directory(dirfd, host_ਰਸਤਾ));
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    ਅੱਖਰ host_ਰਸਤਾ[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_ਰਸਤਾ, ਰਸਤਾ, ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_ਰਸਤਾ_remove_directory(dirfd, host_ਰਸਤਾ));
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);

    ਅੱਖਰ host_ਰਸਤਾ[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_ਰਸਤਾ, ਰਸਤਾ, ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_ਰਸਤਾ_unlink_file(dirfd, host_ਰਸਤਾ));
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, old_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, old_ਰਸਤਾ_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, new_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, new_ਰਸਤਾ_len)

    m3ApiCheckMem(old_ਰਸਤਾ, old_ਰਸਤਾ_len);
    m3ApiCheckMem(new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    ਅੱਖਰ host_old[d_m3_Wasi_Max_ਰਸਤਾ];
    ਅੱਖਰ host_new[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_old, old_ਰਸਤਾ, old_ਰਸਤਾ_len) ||
        !copy_ਰਸਤਾ(host_new, new_ਰਸਤਾ, new_ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_ਰਸਤਾ_rename(old_dirfd, host_old, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, old_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, old_ਰਸਤਾ_len)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, new_ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, new_ਰਸਤਾ_len)

    m3ApiCheckMem(old_ਰਸਤਾ, old_ਰਸਤਾ_len);
    m3ApiCheckMem(new_ਰਸਤਾ, new_ਰਸਤਾ_len);

    // the target is stored as given, so it is a string rather than a path to resolve
    ਅੱਖਰ host_old[d_m3_Wasi_Max_ਰਸਤਾ];
    ਅੱਖਰ host_new[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_old, old_ਰਸਤਾ, old_ਰਸਤਾ_len) ||
        !copy_ਰਸਤਾ(host_new, new_ਰਸਤਾ, new_ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_ਰਸਤਾ_symlink(host_old, dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)
    m3ApiGetArgMem(ਅੱਖਰ*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, ਆਕਾਰ(__wasi_size_t));

    ਅੱਖਰ host_ਰਸਤਾ[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_ਰਸਤਾ, ਰਸਤਾ, ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_size_t ਵਰਤਿਆ;

    __wasi_errno_t err = m3_wasi_host_ਰਸਤਾ_readlink(dirfd, host_ਰਸਤਾ, buf, buf_len, &ਵਰਤਿਆ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(bufused, ਵਰਤਿਆ);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_tell)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_filesize_t*, ਨਤੀਜਾ)

    m3ApiCheckMem(ਨਤੀਜਾ, ਆਕਾਰ(__wasi_filesize_t));

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, 0, SEEK_CUR, &pos);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(ਨਤੀਜਾ, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, ਨਤੀਜਾ)

    m3ApiCheckMem(ਨਤੀਜਾ, ਆਕਾਰ(__wasi_filesize_t));

    ਪੂਰਨ whence;

    ਚੋਣ (wasi_whence) {
    ਹਾਲਤ 0: whence = SEEK_SET; ਰੋਕੋ;
    ਹਾਲਤ 1: whence = SEEK_CUR; ਰੋਕੋ;
    ਹਾਲਤ 2: whence = SEEK_END; ਰੋਕੋ;
    ਮੂਲ: m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, offset, whence, &pos);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(ਨਤੀਜਾ, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


m3ApiRawFunction(m3_wasi_p1_ਰਸਤਾ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ਅਚਲ ਅੱਖਰ*, ਰਸਤਾ)
    m3ApiGetArg(__wasi_size_t, ਰਸਤਾ_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_ਅਧਾਰ)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(ਰਸਤਾ, ਰਸਤਾ_len);
    m3ApiCheckMem(fd, ਆਕਾਰ(__wasi_fd_t));

    ਅੱਖਰ host_ਰਸਤਾ[d_m3_Wasi_Max_ਰਸਤਾ];
    ਜੇ (!copy_ਰਸਤਾ(host_ਰਸਤਾ, ਰਸਤਾ, ਰਸਤਾ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_fd_t host_fd;

    __wasi_errno_t err = m3_wasi_host_ਰਸਤਾ_open(dirfd, host_ਰਸਤਾ, oflags, fs_rights_ਅਧਾਰ,
                                                fs_flags, &host_fd);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nread, ਆਕਾਰ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   ਕੁੱਲ = 0;

    ਲਈ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ਗਿਣਤੀ_2 = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, ਪੂਰਾ;

        ਅਚਲ ਖਾਲੀ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ਗਿਣਤੀ_2, &want);
        ਜੇ (mem_check != m3Err_none) {
            ਵਾਪਸ mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_read(fd, iovs, ਗਿਣਤੀ_2, &ਪੂਰਾ);
        ਜੇ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        ਕੁੱਲ += ਪੂਰਾ;
        ਜੇ (ਪੂਰਾ < want) {
            ਰੋਕੋ;
        }
    }

    m3ApiWriteMem32(nread, ਕੁੱਲ);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ਆਕਾਰ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   ਕੁੱਲ = 0;

    ਲਈ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ਗਿਣਤੀ_2 = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, ਪੂਰਾ;

        ਅਚਲ ਖਾਲੀ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ਗਿਣਤੀ_2, &want);
        ਜੇ (mem_check != m3Err_none) {
            ਵਾਪਸ mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_write(fd, iovs, ਗਿਣਤੀ_2, &ਪੂਰਾ);
        ਜੇ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        ਕੁੱਲ += ਪੂਰਾ;
        ਜੇ (ਪੂਰਾ < want) {
            ਰੋਕੋ;
        }
    }

    m3ApiWriteMem32(nwritten, ਕੁੱਲ);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nread, ਆਕਾਰ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   ਕੁੱਲ = 0;

    ਲਈ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ਗਿਣਤੀ_2 = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, ਪੂਰਾ;

        ਅਚਲ ਖਾਲੀ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ਗਿਣਤੀ_2, &want);
        ਜੇ (mem_check != m3Err_none) {
            ਵਾਪਸ mem_check;
        }

        // the offset walks with the data, so a short batch leaves no hole behind it
        __wasi_errno_t err = m3_wasi_host_fd_pread(fd, iovs, ਗਿਣਤੀ_2, offset + ਕੁੱਲ, &ਪੂਰਾ);
        ਜੇ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        ਕੁੱਲ += ਪੂਰਾ;
        ਜੇ (ਪੂਰਾ < want) {
            ਰੋਕੋ;
        }
    }

    m3ApiWriteMem32(nread, ਕੁੱਲ);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * ਆਕਾਰ(wasi_iovec_t));
    m3ApiCheckMem(nwritten, ਆਕਾਰ(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   ਕੁੱਲ = 0;

    ਲਈ (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t ਗਿਣਤੀ_2 = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, ਪੂਰਾ;

        ਅਚਲ ਖਾਲੀ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, ਗਿਣਤੀ_2, &want);
        ਜੇ (mem_check != m3Err_none) {
            ਵਾਪਸ mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_pwrite(fd, iovs, ਗਿਣਤੀ_2, offset + ਕੁੱਲ, &ਪੂਰਾ);
        ਜੇ (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        ਕੁੱਲ += ਪੂਰਾ;
        ਜੇ (ਪੂਰਾ < want) {
            ਰੋਕੋ;
        }
    }

    m3ApiWriteMem32(nwritten, ਕੁੱਲ);
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

#  ਜੇ d_m3DeterministicProfile
    Deterministic_Random(runtime, buf, buf_len);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  ਨਹੀਂਤਾਂ
    m3ApiReturn(m3_wasi_host_random_get(buf, buf_len));
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_res_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArgMem(__wasi_timestamp_t*, resolution)

    m3ApiCheckMem(resolution, ਆਕਾਰ(__wasi_timestamp_t));

#  ਜੇ d_m3DeterministicProfile
    // the deterministic clock's resolution is the step it moves in
    m3ApiWriteMem64(resolution, d_m3DeterministicTickNs);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  ਨਹੀਂਤਾਂ
    __wasi_timestamp_t res;

    __wasi_errno_t err = m3_wasi_host_clock_res_get(wasi_clk_id, &res);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(resolution, res);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_ਸਮਾਂ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, ਆਕਾਰ(__wasi_timestamp_t));

#  ਜੇ d_m3DeterministicProfile
    // Every clock the guest can name is the same underlying count - a runtime that is
    // not measuring the machine has nothing to tell process time and thread time apart
    // with - and only where it is counted from differs
    __wasi_timestamp_t now = Deterministic_ਸਮਾਂ(runtime);

    ਜੇ (wasi_clk_id == __WASI_CLOCKID_REALTIME) {
        now += d_m3WasiDeterministicEpochNs;
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  ਨਹੀਂਤਾਂ
    __wasi_timestamp_t now;

    __wasi_errno_t err = m3_wasi_host_clock_ਸਮਾਂ_get(wasi_clk_id, &now);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, ਕੋਡ)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    ਜੇ (context) {
        context->exit_ਕੋਡ = ਕੋਡ;
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
ਸਥਿਰ
m3_wasi_pollsub_t* read_subscriptions (ਅਚਲ uint8_t* i_subs, __wasi_size_t i_ਗਿਣਤੀ)
{
    m3_wasi_pollsub_t* subs =
      (m3_wasi_pollsub_t*)m3_Malloc("WASI subscriptions", i_ਗਿਣਤੀ * ਆਕਾਰ(m3_wasi_pollsub_t));

    ਜੇ (subs == ਨਿਰਾ) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
        ਅਚਲ uint8_t* s = i_subs + (size_t)i * d_m3_Wasi_Subscription_ਮਾਪ;

        memset(&subs[i], 0, ਆਕਾਰ(subs[i]));

        subs[i].userdata = m3ApiReadMem64(s + 0);
        subs[i].type     = m3ApiReadMem8(s + 8);

        ਜੇ (subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            subs[i].clockId = m3ApiReadMem32(s + 16);
            subs[i].timeout = m3ApiReadMem64(s + 24);
            subs[i].isAbsolute =
              (m3ApiReadMem16(s + 40) & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) != 0;
        } ਨਹੀਂਤਾਂ {
            subs[i].fd = m3ApiReadMem32(s + 16);
        }
    }

    ਵਾਪਸ subs;
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ਸਥਿਰ
ਖਾਲੀ write_event (uint8_t* o_rec, ਅਚਲ m3_wasi_pollevent_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_ਮਾਪ);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->flags);
}

#  ਜੇ d_m3DeterministicProfile

// poll_oneoff under the deterministic profile, which cannot wait on anything real. A
// descriptor is reported ready at once - which is what the spec says a regular file
// always is, and the only answer here that does not depend on how fast something
// else is going - and the clock is moved forward to the soonest deadline, so that a
// guest sleeping still sees the time it slept for.
ਸਥਿਰ
ਖਾਲੀ poll_deterministically (IM3Runtime io_runtime, ਅਚਲ m3_wasi_pollsub_t* i_subs,
                             __wasi_size_t i_ਗਿਣਤੀ, m3_wasi_pollevent_t* o_events,
                             __wasi_size_t* o_ਅੰਕ_Events)
{
    __wasi_size_t ਬਾਹਰਲਾ     = 0;
    uint64_t      soonest = UINT64_MAX;
    uint64_t      now     = io_runtime->virtual_ਸਮਾਂ_Ns;

    ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
        ਅਚਲ m3_wasi_pollsub_t* sub = &i_subs[i];

        ਜੇ (sub->type != __WASI_EVENTTYPE_CLOCK) {
            o_events[ਬਾਹਰਲਾ].userdata = sub->userdata;
            o_events[ਬਾਹਰਲਾ].error    = __WASI_ERRNO_SUCCESS;
            o_events[ਬਾਹਰਲਾ].type     = sub->type;
            o_events[ਬਾਹਰਲਾ].nbytes   = 1;
            o_events[ਬਾਹਰਲਾ].flags    = 0;
            ਬਾਹਰਲਾ++;
            ਜਾਰੀ;
        }

        // an absolute deadline is against the same virtual clock the guest reads
        uint64_t deadline = sub->isAbsolute ? sub->timeout : now + sub->timeout;
        soonest           = M3_MIN(soonest, deadline);
    }

    // Something is ready already, so no time passes - which is what poll() would
    // have done too
    ਜੇ (ਬਾਹਰਲਾ) {
        *o_ਅੰਕ_Events = ਬਾਹਰਲਾ;
        ਵਾਪਸ;
    }

    ਜੇ (soonest > now) {
        Deterministic_Advance(io_runtime, soonest - now);
    }

    now = io_runtime->virtual_ਸਮਾਂ_Ns;

    ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
        ਅਚਲ m3_wasi_pollsub_t* sub = &i_subs[i];

        ਜੇ (sub->type != __WASI_EVENTTYPE_CLOCK) {
            ਜਾਰੀ;
        }

        uint64_t deadline = sub->isAbsolute ? sub->timeout : soonest;

        ਜੇ (deadline <= now) {
            o_events[ਬਾਹਰਲਾ].userdata = sub->userdata;
            o_events[ਬਾਹਰਲਾ].error    = __WASI_ERRNO_SUCCESS;
            o_events[ਬਾਹਰਲਾ].type     = __WASI_EVENTTYPE_CLOCK;
            o_events[ਬਾਹਰਲਾ].nbytes   = 0;
            o_events[ਬਾਹਰਲਾ].flags    = 0;
            ਬਾਹਰਲਾ++;
        }
    }

    *o_ਅੰਕ_Events = ਬਾਹਰਲਾ;
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
ਸਥਿਰ
__wasi_errno_t poll_subscriptions (IM3Runtime i_runtime, ਅਚਲ m3_wasi_pollsub_t* i_subs,
                                   uint8_t* o_events, __wasi_size_t i_ਗਿਣਤੀ,
                                   __wasi_size_t* o_ਅੰਕ_Events)
{
    *o_ਅੰਕ_Events = 0;

    m3_wasi_pollevent_t* evts =
      (m3_wasi_pollevent_t*)m3_Malloc("WASI events", i_ਗਿਣਤੀ * ਆਕਾਰ(m3_wasi_pollevent_t));

    ਜੇ (evts == ਨਿਰਾ) {
        ਵਾਪਸ __WASI_ERRNO_NOMEM;
    }

    __wasi_size_t  ਅੰਕ_Events = 0;
    __wasi_errno_t err       = __WASI_ERRNO_SUCCESS;

#  ਜੇ d_m3DeterministicProfile
    poll_deterministically(i_runtime, i_subs, i_ਗਿਣਤੀ, evts, &ਅੰਕ_Events);
#  ਨਹੀਂਤਾਂ
    err = m3_wasi_host_poll_oneoff(i_subs, i_ਗਿਣਤੀ, evts, &ਅੰਕ_Events);
#  endif

    ਜੇ (err == __WASI_ERRNO_SUCCESS) {
        ਲਈ (__wasi_size_t i = 0; i < ਅੰਕ_Events; i++) {
            write_event(o_events + (size_t)i * d_m3_Wasi_Event_ਮਾਪ, &evts[i]);
        }
        *o_ਅੰਕ_Events = ਅੰਕ_Events;
    }

    m3_Free(evts);

    ਵਾਪਸ err;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ਅਚਲ uint8_t*, ਵਿੱਚ)
    m3ApiGetArgMem(uint8_t*, ਬਾਹਰਲਾ)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(ਵਿੱਚ, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_ਮਾਪ);
    m3ApiCheckMem(ਬਾਹਰਲਾ, (uint64_t)nsubscriptions * d_m3_Wasi_Event_ਮਾਪ);
    m3ApiCheckMem(nevents, ਆਕਾਰ(__wasi_size_t));

    ਜੇ (nsubscriptions == 0) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3_wasi_pollsub_t* subs = read_subscriptions(ਵਿੱਚ, nsubscriptions);

    ਜੇ (subs == ਨਿਰਾ) {
        m3ApiReturn(__WASI_ERRNO_NOMEM);
    }

    __wasi_size_t  ਅੰਕ_Events = 0;
    __wasi_errno_t err       = poll_subscriptions(runtime, subs, ਬਾਹਰਲਾ, nsubscriptions, &ਅੰਕ_Events);

    m3_Free(subs);

    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(nevents, ਅੰਕ_Events);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


ਸਥਿਰ
M3_ਨਤੀਜਾ SuppressLookupFailure (M3_ਨਤੀਜਾ i_ਨਤੀਜਾ)
{
    ਜੇ (i_ਨਤੀਜਾ == m3Err_functionLookupFailed) {
        ਵਾਪਸ m3Err_none;
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ i_ਨਤੀਜਾ;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    ਵਾਪਸ wasi_context;
}

ਖਾਲੀ m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਨਤੀਜਾ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    m3_wasi_host_init();

    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    ਲਈ (size_t i = 0; i < M3_ਗਿਣਤੀ_ਦਾ(namespaces); i++) {
        ਅਚਲ ਅੱਖਰ* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_ਦਲੀਲਾਂ_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_ਦਲੀਲਾਂ_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_ਸਮਾਂ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_ਸੈੱਟ_flags)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",      "i(i*)",   &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_ਸੈੱਟ_ਮਾਪ)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_ਸੈੱਟ_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",        "i(ii*i*)",     &m3_wasi_p1_ਰਸਤਾ_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   &m3_wasi_p1_ਰਸਤਾ_link)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_ਰਸਤਾ_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_ਰਸਤਾ_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_ਰਸਤਾ_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_ਰਸਤਾ_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_ਰਸਤਾ_unlink_file)));

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
    ਜੇ (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ;

        ਜੇ (m3_ਲੱਭੋ_Exported_ਸਮ੍ਰਿਤੀ(module, "memory", &ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_ਸਮ੍ਰਿਤੀ(module, "wasi_snapshot_preview1", ਸਮ੍ਰਿਤੀ_ਸੂਚਕਾਂਕ));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

M3_ਨਤੀਜਾ m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    ਵਾਪਸ m3Err_none;
}

ਖਾਲੀ m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_ਨਤੀਜਾ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    ਵਾਪਸ _linkWASI(module, wasi);
}

M3_ਨਤੀਜਾ m3_LinkWASI (IM3Module module)
{
    ਜੇ (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    ਵਾਪਸ _linkWASI(module, wasi_context);
}

#ਜੇ_ਅੰਤ // d_m3HasWASI
