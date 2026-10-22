//
//  m3_api_wasi.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ضمن "m3_api_wasi.h"

#ضمن "m3_api_wasi_host.h"
#ضمن "m3_env.h"
#ضمن "m3_deterministic.h"
#ضمن "m3_exception.h"

#إذا defined(d_m3HasWASI)

#  include <string.h>

// The system underneath, of which exactly one is ever built. They are headers rather
// than translation units of their own so that neither has to be added to a source
// list - and most of those are build scripts outside this repository.
#  إذا defined(_WIN32)
#    include "m3_api_wasi_win32.h"
#  وإلا
#    include "m3_api_wasi_posix.h"
#  endif

// This file is the WASI interface and nothing below it: it decodes the guest's
// arguments, bounds-checks every pointer against guest memory, and writes results
// back in the layout the calling revision asks for. The system underneath is reached
// through m3_api_wasi_host.h, which m3_api_wasi_posix.h and m3_api_wasi_win32.h
// implement independently of each other.

ساكن m3_wasi_context_t* wasi_context;

// A scatter/gather list as the guest lays one out
تعريفنوع بنية wasi_iovec_t {
    __wasi_size_t buf;
    __wasi_size_t buf_len;
} wasi_iovec_t;

// The records the guest passes by address, sized as the ABI lays them out rather
// than as this host happens to - see write_filestat, read_subscriptions, write_event.
#  define d_m3_Wasi_Filestat_ناپ      64
#  define d_m3_Wasi_Subscription_ناپ  48
#  define d_m3_Wasi_Event_ناپ         32

ساكن ثابت حرف* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    عدم,
};

// Copies a path out of guest memory so it can be handed to the host NUL-terminated.
// The caller has already bounds-checked (i_path, i_pathLen) against guest memory.
ساكن
صحيح copy_رستہ (حرف* o_رستہ, ثابت حرف* i_رستہ, __wasi_size_t i_رستہ_Len)
{
    إذا (i_رستہ_Len >= d_m3_Wasi_Max_رستہ) {
        أرجع 0;
    }
    حافظہ_نقل(o_رستہ, i_رستہ, i_رستہ_Len);
    o_رستہ[i_رستہ_Len] = '\0';
    أرجع 1;
}

// Writes a filestat out: dev 0, ino 8, filetype 16, nlink 24, size 32, atim 40,
// mtim 48, ctim 56. The caller has checked the buffer for d_m3WasiFilestatSize.
ساكن
فراغ write_filestat (uint8_t* o_buf, ثابت m3_wasi_filestat_t* i_stat)
{
    memset(o_buf, 0, d_m3_Wasi_Filestat_ناپ);

    m3ApiWriteMem64(o_buf + 0, i_stat->dev);
    m3ApiWriteMem64(o_buf + 8, i_stat->ino);
    m3ApiWriteMem8(o_buf + 16, i_stat->filetype);
    m3ApiWriteMem64(o_buf + 24, i_stat->nlink);
    m3ApiWriteMem64(o_buf + 32, i_stat->ناپ);
    m3ApiWriteMem64(o_buf + 40, i_stat->atim);
    m3ApiWriteMem64(o_buf + 48, i_stat->mtim);
    m3ApiWriteMem64(o_buf + 56, i_stat->ctim);
}

// Resolves the guest's scatter/gather list to host addresses, checking each buffer
// against guest memory, and adds up how much they ask for. The host layer is handed
// the result and never sees a guest offset.
ساكن
ثابت فراغ* resolve_iovs (IM3Runtime runtime, فراغ* _mem, m3_wasi_iovec_t* o_iovs,
                          ثابت wasi_iovec_t* i_wasiIovs, __wasi_size_t i_گنتی,
                          __wasi_size_t* o_want)
{
    *o_want = 0;

    __wasi_size_t want = 0;

    لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
        o_iovs[i].buf = m3_Api_وتھ_منزل_Ptr(m3ApiReadMem32(&i_wasiIovs[i].buf));
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

m3ApiRawFunction(m3_wasi_p1_دلیلاں_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(حرف*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    إذا (context == عدم) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * حجم(uint32_t));

    لكل (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_منزل_وتھ(argv_buf));

        size_t len = لڑی_لمبائی(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        حافظہ_نقل(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_دلیلاں_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_ناپ)

    m3ApiCheckMem(argc, حجم(__wasi_size_t));
    m3ApiCheckMem(argv_buf_ناپ, حجم(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    إذا (context == عدم) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    لكل (u32 i = 0; i < context->argc; ++i) {
        buf_len += (__wasi_size_t)لڑی_لمبائی(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_ناپ, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(حرف*, env_buf)

    __wasi_size_t گنتی = 0;
    لكل (ثابت حرف** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        گنتی++;
    }

    m3ApiCheckMem(env, گنتی * حجم(uint32_t));

    لكل (u32 i = 0; i < گنتی; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_منزل_وتھ(env_buf));

        size_t len = لڑی_لمبائی(DEFAULT_ENVIRONMENT[i]);

        m3ApiCheckMem(env_buf, len + 1);        // include the terminating `\0`
        حافظہ_نقل(env_buf, DEFAULT_ENVIRONMENT[i], len);
        env_buf += len;
        *env_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_گنتی)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_ناپ)

    m3ApiCheckMem(env_گنتی, حجم(__wasi_size_t));
    m3ApiCheckMem(env_buf_ناپ, حجم(__wasi_size_t));

    __wasi_size_t گنتی = 0, buf_len = 0;
    لكل (ثابت حرف** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        گنتی++;
        buf_len += (__wasi_size_t)لڑی_لمبائی(*e) + 1;
    }

    m3ApiWriteMem32(env_گنتی, گنتی);
    m3ApiWriteMem32(env_buf_ناپ, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(حرف*, رستہ)
    m3ApiGetArg(__wasi_size_t, رستہ_len)

    m3ApiCheckMem(رستہ, رستہ_len);

    إذا (fd < 3 || fd >= d_m3_Wasi_Preopen_گنتی) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }
    size_t slen = لڑی_لمبائی(m3_wasi_preopen[fd].رستہ) + 1;
    حافظہ_نقل(رستہ, m3_wasi_preopen[fd].رستہ, M3_MIN(slen, رستہ_len));
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 8);

    إذا (fd < 3 || fd >= d_m3_Wasi_Preopen_گنتی) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }

    m3ApiWriteMem32(buf + 0, __WASI_PREOPENTYPE_DIR);
    m3ApiWriteMem32(buf + 4, (u32)لڑی_لمبائی(m3_wasi_preopen[fd].رستہ) + 1);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_fdstat_t*, fdstat)

    m3ApiCheckMem(fdstat, حجم(__wasi_fdstat_t));

    __wasi_filetype_t filetype;
    __wasi_fdflags_t  flags;

    __wasi_errno_t err = m3_wasi_host_fd_fdstat(fd, &filetype, &flags);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    fdstat->fs_filetype = filetype;
    m3ApiWriteMem16(&fdstat->fs_flags, flags);

    fdstat->fs_rights_ادھار = (uint64_t)-1; // all rights

    // Make descriptors 0,1,2 look like a TTY
    إذا (fd <= 2) {
        fdstat->fs_rights_ادھار &= ~(__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
    }

    fdstat->fs_rights_inheriting = (uint64_t)-1; // all rights
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_سیٹ_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_fdstat_سیٹ_flags(fd, flags));
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, لمائی)
    m3ApiGetArg(__wasi_advice_t, advice)

    m3ApiReturn(m3_wasi_host_fd_advise(fd, offset, لمائی, advice));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_ناپ);

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t err = m3_wasi_host_fd_filestat(fd, &fd_stat);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &fd_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_رستہ_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ثابت حرف*, رستہ)
    m3ApiGetArg(__wasi_size_t, رستہ_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(رستہ, رستہ_len);
    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_ناپ);

    حرف host_رستہ[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_رستہ, رستہ, رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3_wasi_filestat_t رستہ_stat;
    bool               follow = (dirflags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    __wasi_errno_t err = m3_wasi_host_رستہ_filestat(dirfd, host_رستہ, follow, &رستہ_stat);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &رستہ_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سیٹ_ناپ)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, ناپ)

    m3ApiReturn(m3_wasi_host_fd_سیٹ_ناپ(fd, ناپ));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_سیٹ_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_timestamp_t, atim)
    m3ApiGetArg(__wasi_timestamp_t, mtim)
    m3ApiGetArg(__wasi_fstflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_سیٹ_times(fd, atim, mtim, flags));
}

m3ApiRawFunction(m3_wasi_p1_رستہ_link)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, old_flags)
    m3ApiGetArgMem(ثابت حرف*, old_رستہ)
    m3ApiGetArg(__wasi_size_t, old_رستہ_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ثابت حرف*, new_رستہ)
    m3ApiGetArg(__wasi_size_t, new_رستہ_len)

    m3ApiCheckMem(old_رستہ, old_رستہ_len);
    m3ApiCheckMem(new_رستہ, new_رستہ_len);

    حرف host_old[d_m3_Wasi_Max_رستہ];
    حرف host_new[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_old, old_رستہ, old_رستہ_len) ||
        !copy_رستہ(host_new, new_رستہ, new_رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    bool follow = (old_flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    m3ApiReturn(m3_wasi_host_رستہ_link(old_dirfd, host_old, follow, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_رستہ_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ثابت حرف*, رستہ)
    m3ApiGetArg(__wasi_size_t, رستہ_len)

    m3ApiCheckMem(رستہ, رستہ_len);

    حرف host_رستہ[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_رستہ, رستہ, رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_رستہ_create_directory(dirfd, host_رستہ));
}

m3ApiRawFunction(m3_wasi_p1_رستہ_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ثابت حرف*, رستہ)
    m3ApiGetArg(__wasi_size_t, رستہ_len)

    m3ApiCheckMem(رستہ, رستہ_len);

    حرف host_رستہ[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_رستہ, رستہ, رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_رستہ_remove_directory(dirfd, host_رستہ));
}

m3ApiRawFunction(m3_wasi_p1_رستہ_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ثابت حرف*, رستہ)
    m3ApiGetArg(__wasi_size_t, رستہ_len)

    m3ApiCheckMem(رستہ, رستہ_len);

    حرف host_رستہ[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_رستہ, رستہ, رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_رستہ_unlink_file(dirfd, host_رستہ));
}

m3ApiRawFunction(m3_wasi_p1_رستہ_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArgMem(ثابت حرف*, old_رستہ)
    m3ApiGetArg(__wasi_size_t, old_رستہ_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ثابت حرف*, new_رستہ)
    m3ApiGetArg(__wasi_size_t, new_رستہ_len)

    m3ApiCheckMem(old_رستہ, old_رستہ_len);
    m3ApiCheckMem(new_رستہ, new_رستہ_len);

    حرف host_old[d_m3_Wasi_Max_رستہ];
    حرف host_new[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_old, old_رستہ, old_رستہ_len) ||
        !copy_رستہ(host_new, new_رستہ, new_رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_رستہ_rename(old_dirfd, host_old, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_رستہ_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت حرف*, old_رستہ)
    m3ApiGetArg(__wasi_size_t, old_رستہ_len)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ثابت حرف*, new_رستہ)
    m3ApiGetArg(__wasi_size_t, new_رستہ_len)

    m3ApiCheckMem(old_رستہ, old_رستہ_len);
    m3ApiCheckMem(new_رستہ, new_رستہ_len);

    // the target is stored as given, so it is a string rather than a path to resolve
    حرف host_old[d_m3_Wasi_Max_رستہ];
    حرف host_new[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_old, old_رستہ, old_رستہ_len) ||
        !copy_رستہ(host_new, new_رستہ, new_رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_رستہ_symlink(host_old, dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_رستہ_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ثابت حرف*, رستہ)
    m3ApiGetArg(__wasi_size_t, رستہ_len)
    m3ApiGetArgMem(حرف*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(رستہ, رستہ_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, حجم(__wasi_size_t));

    حرف host_رستہ[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_رستہ, رستہ, رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_size_t ورتیا;

    __wasi_errno_t err = m3_wasi_host_رستہ_readlink(dirfd, host_رستہ, buf, buf_len, &ورتیا);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(bufused, ورتیا);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_tell)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_filesize_t*, نتیجہ)

    m3ApiCheckMem(نتیجہ, حجم(__wasi_filesize_t));

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, 0, SEEK_CUR, &pos);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(نتیجہ, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, نتیجہ)

    m3ApiCheckMem(نتیجہ, حجم(__wasi_filesize_t));

    صحيح whence;

    بدل (wasi_whence) {
    حالة 0: whence = SEEK_SET; اكسر;
    حالة 1: whence = SEEK_CUR; اكسر;
    حالة 2: whence = SEEK_END; اكسر;
    افتراضي: m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, offset, whence, &pos);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(نتیجہ, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


m3ApiRawFunction(m3_wasi_p1_رستہ_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ثابت حرف*, رستہ)
    m3ApiGetArg(__wasi_size_t, رستہ_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_ادھار)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(رستہ, رستہ_len);
    m3ApiCheckMem(fd, حجم(__wasi_fd_t));

    حرف host_رستہ[d_m3_Wasi_Max_رستہ];
    إذا (!copy_رستہ(host_رستہ, رستہ, رستہ_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_fd_t host_fd;

    __wasi_errno_t err = m3_wasi_host_رستہ_open(dirfd, host_رستہ, oflags, fs_rights_ادھار,
                                                fs_flags, &host_fd);
    إذا (err != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   کل = 0;

    لكل (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t گنتی = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, پورا;

        ثابت فراغ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, گنتی, &want);
        إذا (mem_check != m3Err_none) {
            أرجع mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_read(fd, iovs, گنتی, &پورا);
        إذا (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        کل += پورا;
        إذا (پورا < want) {
            اكسر;
        }
    }

    m3ApiWriteMem32(nread, کل);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   کل = 0;

    لكل (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t گنتی = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, پورا;

        ثابت فراغ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, گنتی, &want);
        إذا (mem_check != m3Err_none) {
            أرجع mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_write(fd, iovs, گنتی, &پورا);
        إذا (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        کل += پورا;
        إذا (پورا < want) {
            اكسر;
        }
    }

    m3ApiWriteMem32(nwritten, کل);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nread, حجم(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   کل = 0;

    لكل (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t گنتی = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, پورا;

        ثابت فراغ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, گنتی, &want);
        إذا (mem_check != m3Err_none) {
            أرجع mem_check;
        }

        // the offset walks with the data, so a short batch leaves no hole behind it
        __wasi_errno_t err = m3_wasi_host_fd_pread(fd, iovs, گنتی, offset + کل, &پورا);
        إذا (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        کل += پورا;
        إذا (پورا < want) {
            اكسر;
        }
    }

    m3ApiWriteMem32(nread, کل);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * حجم(wasi_iovec_t));
    m3ApiCheckMem(nwritten, حجم(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   کل = 0;

    لكل (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t گنتی = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, پورا;

        ثابت فراغ* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, گنتی, &want);
        إذا (mem_check != m3Err_none) {
            أرجع mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_pwrite(fd, iovs, گنتی, offset + کل, &پورا);
        إذا (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        کل += پورا;
        إذا (پورا < want) {
            اكسر;
        }
    }

    m3ApiWriteMem32(nwritten, کل);
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

#  إذا d_m3DeterministicProfile
    Deterministic_Random(runtime, buf, buf_len);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  وإلا
    m3ApiReturn(m3_wasi_host_random_get(buf, buf_len));
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_res_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArgMem(__wasi_timestamp_t*, resolution)

    m3ApiCheckMem(resolution, حجم(__wasi_timestamp_t));

#  إذا d_m3DeterministicProfile
    // the deterministic clock's resolution is the step it moves in
    m3ApiWriteMem64(resolution, d_m3DeterministicTickNs);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  وإلا
    __wasi_timestamp_t res;

    __wasi_errno_t err = m3_wasi_host_clock_res_get(wasi_clk_id, &res);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(resolution, res);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_ویلا_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArg(__wasi_timestamp_t, precision)
    m3ApiGetArgMem(__wasi_timestamp_t*, time)

    m3ApiCheckMem(time, حجم(__wasi_timestamp_t));

#  إذا d_m3DeterministicProfile
    // Every clock the guest can name is the same underlying count - a runtime that is
    // not measuring the machine has nothing to tell process time and thread time apart
    // with - and only where it is counted from differs
    __wasi_timestamp_t now = Deterministic_ویلا(runtime);

    إذا (wasi_clk_id == __WASI_CLOCKID_REALTIME) {
        now += d_m3WasiDeterministicEpochNs;
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  وإلا
    __wasi_timestamp_t now;

    __wasi_errno_t err = m3_wasi_host_clock_ویلا_get(wasi_clk_id, &now);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, کوڈ)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    إذا (context) {
        context->exit_کوڈ = کوڈ;
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
ساكن
m3_wasi_pollsub_t* read_subscriptions (ثابت uint8_t* i_subs, __wasi_size_t i_گنتی)
{
    m3_wasi_pollsub_t* subs =
      (m3_wasi_pollsub_t*)m3_Malloc("WASI subscriptions", i_گنتی * حجم(m3_wasi_pollsub_t));

    إذا (subs == عدم) {
        أرجع عدم;
    }

    لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
        ثابت uint8_t* s = i_subs + (size_t)i * d_m3_Wasi_Subscription_ناپ;

        memset(&subs[i], 0, حجم(subs[i]));

        subs[i].userdata = m3ApiReadMem64(s + 0);
        subs[i].type     = m3ApiReadMem8(s + 8);

        إذا (subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            subs[i].clockId = m3ApiReadMem32(s + 16);
            subs[i].timeout = m3ApiReadMem64(s + 24);
            subs[i].isAbsolute =
              (m3ApiReadMem16(s + 40) & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) != 0;
        } وإلا {
            subs[i].fd = m3ApiReadMem32(s + 16);
        }
    }

    أرجع subs;
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
ساكن
فراغ write_event (uint8_t* o_rec, ثابت m3_wasi_pollevent_t* i_event)
{
    memset(o_rec, 0, d_m3_Wasi_Event_ناپ);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->flags);
}

#  إذا d_m3DeterministicProfile

// poll_oneoff under the deterministic profile, which cannot wait on anything real. A
// descriptor is reported ready at once - which is what the spec says a regular file
// always is, and the only answer here that does not depend on how fast something
// else is going - and the clock is moved forward to the soonest deadline, so that a
// guest sleeping still sees the time it slept for.
ساكن
فراغ poll_deterministically (IM3Runtime io_runtime, ثابت m3_wasi_pollsub_t* i_subs,
                             __wasi_size_t i_گنتی, m3_wasi_pollevent_t* o_events,
                             __wasi_size_t* o_عدد_Events)
{
    __wasi_size_t باہرلا     = 0;
    uint64_t      soonest = UINT64_MAX;
    uint64_t      now     = io_runtime->virtual_ویلا_Ns;

    لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
        ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

        إذا (sub->type != __WASI_EVENTTYPE_CLOCK) {
            o_events[باہرلا].userdata = sub->userdata;
            o_events[باہرلا].error    = __WASI_ERRNO_SUCCESS;
            o_events[باہرلا].type     = sub->type;
            o_events[باہرلا].nbytes   = 1;
            o_events[باہرلا].flags    = 0;
            باہرلا++;
            استمر;
        }

        // an absolute deadline is against the same virtual clock the guest reads
        uint64_t deadline = sub->isAbsolute ? sub->timeout : now + sub->timeout;
        soonest           = M3_MIN(soonest, deadline);
    }

    // Something is ready already, so no time passes - which is what poll() would
    // have done too
    إذا (باہرلا) {
        *o_عدد_Events = باہرلا;
        أرجع;
    }

    إذا (soonest > now) {
        Deterministic_Advance(io_runtime, soonest - now);
    }

    now = io_runtime->virtual_ویلا_Ns;

    لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
        ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

        إذا (sub->type != __WASI_EVENTTYPE_CLOCK) {
            استمر;
        }

        uint64_t deadline = sub->isAbsolute ? sub->timeout : soonest;

        إذا (deadline <= now) {
            o_events[باہرلا].userdata = sub->userdata;
            o_events[باہرلا].error    = __WASI_ERRNO_SUCCESS;
            o_events[باہرلا].type     = __WASI_EVENTTYPE_CLOCK;
            o_events[باہرلا].nbytes   = 0;
            o_events[باہرلا].flags    = 0;
            باہرلا++;
        }
    }

    *o_عدد_Events = باہرلا;
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
ساكن
__wasi_errno_t poll_subscriptions (IM3Runtime i_runtime, ثابت m3_wasi_pollsub_t* i_subs,
                                   uint8_t* o_events, __wasi_size_t i_گنتی,
                                   __wasi_size_t* o_عدد_Events)
{
    *o_عدد_Events = 0;

    m3_wasi_pollevent_t* evts =
      (m3_wasi_pollevent_t*)m3_Malloc("WASI events", i_گنتی * حجم(m3_wasi_pollevent_t));

    إذا (evts == عدم) {
        أرجع __WASI_ERRNO_NOMEM;
    }

    __wasi_size_t  عدد_Events = 0;
    __wasi_errno_t err       = __WASI_ERRNO_SUCCESS;

#  إذا d_m3DeterministicProfile
    poll_deterministically(i_runtime, i_subs, i_گنتی, evts, &عدد_Events);
#  وإلا
    err = m3_wasi_host_poll_oneoff(i_subs, i_گنتی, evts, &عدد_Events);
#  endif

    إذا (err == __WASI_ERRNO_SUCCESS) {
        لكل (__wasi_size_t i = 0; i < عدد_Events; i++) {
            write_event(o_events + (size_t)i * d_m3_Wasi_Event_ناپ, &evts[i]);
        }
        *o_عدد_Events = عدد_Events;
    }

    m3_Free(evts);

    أرجع err;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ثابت uint8_t*, وچ)
    m3ApiGetArgMem(uint8_t*, باہرلا)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(وچ, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_ناپ);
    m3ApiCheckMem(باہرلا, (uint64_t)nsubscriptions * d_m3_Wasi_Event_ناپ);
    m3ApiCheckMem(nevents, حجم(__wasi_size_t));

    إذا (nsubscriptions == 0) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3_wasi_pollsub_t* subs = read_subscriptions(وچ, nsubscriptions);

    إذا (subs == عدم) {
        m3ApiReturn(__WASI_ERRNO_NOMEM);
    }

    __wasi_size_t  عدد_Events = 0;
    __wasi_errno_t err       = poll_subscriptions(runtime, subs, باہرلا, nsubscriptions, &عدد_Events);

    m3_Free(subs);

    إذا (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(nevents, عدد_Events);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


ساكن
M3_نتیجہ SuppressLookupFailure (M3_نتیجہ i_نتیجہ)
{
    إذا (i_نتیجہ == m3Err_functionLookupFailed) {
        أرجع m3Err_none;
    } وإلا {
        أرجع i_نتیجہ;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    أرجع wasi_context;
}

فراغ m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

ساكن مضمن
M3_نتیجہ _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    m3_wasi_host_init();

    ساكن ثابت حرف* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    لكل (size_t i = 0; i < M3_گنتی_دا(namespaces); i++) {
        ثابت حرف* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_دلیلاں_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_دلیلاں_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_ویلا_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_سیٹ_flags)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",      "i(i*)",   &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_سیٹ_ناپ)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_سیٹ_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_رستہ_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",        "i(ii*i*)",     &m3_wasi_p1_رستہ_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   &m3_wasi_p1_رستہ_link)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_رستہ_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_رستہ_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_رستہ_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_رستہ_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_رستہ_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_رستہ_unlink_file)));

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
    إذا (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 حافظہ_اشاریہ;

        إذا (m3_لبھو_Exported_حافظہ(module, "memory", &حافظہ_اشاریہ)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_حافظہ(module, "wasi_snapshot_preview1", حافظہ_اشاریہ));
    }

_catch:
    أرجع نتیجہ;
}

M3_نتیجہ m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    أرجع m3Err_none;
}

فراغ m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_نتیجہ m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    أرجع _linkWASI(module, wasi);
}

M3_نتیجہ m3_LinkWASI (IM3Module module)
{
    إذا (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    أرجع _linkWASI(module, wasi_context);
}

#جے_انت // d_m3HasWASI
