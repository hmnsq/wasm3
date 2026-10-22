//
//  m3_api_wasi.c
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#अन्तर्भाव "m3_api_wasi.h"

#अन्तर्भाव "m3_api_wasi_host.h"
#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_deterministic.h"
#अन्तर्भाव "m3_exception.h"

#यदि defined(d_m3HasWASI)

#  include <string.h>

// The system underneath, of which exactly one is ever built. They are headers rather
// than translation units of their own so that neither has to be added to a source
// list - and most of those are build scripts outside this repository.
#  यदि defined(_WIN32)
#    include "m3_api_wasi_win32.h"
#  अन्यथा
#    include "m3_api_wasi_posix.h"
#  endif

// This file is the WASI interface and nothing below it: it decodes the guest's
// arguments, bounds-checks every pointer against guest memory, and writes results
// back in the layout the calling revision asks for. The system underneath is reached
// through m3_api_wasi_host.h, which m3_api_wasi_posix.h and m3_api_wasi_win32.h
// implement independently of each other.

स्थिर m3_wasi_context_t* wasi_context;

// A scatter/gather list as the guest lays one out
प्रकारनिर्देश संरचना wasi_iovec_t {
    __wasi_size_t buf;
    __wasi_size_t buf_len;
} wasi_iovec_t;

// The records the guest passes by address, sized as the ABI lays them out rather
// than as this host happens to - see write_filestat, read_subscriptions, write_event.
#  define d_m3_Wasi_Filestat_आकार      64
#  define d_m3_Wasi_Subscription_आकार  48
#  define d_m3_Wasi_Event_आकार         32

स्थिर ध्रुव वर्ण* DEFAULT_ENVIRONMENT[] = {
    d_m3WasiDefaultEnvironment,
    रिक्त,
};

// Copies a path out of guest memory so it can be handed to the host NUL-terminated.
// The caller has already bounds-checked (i_path, i_pathLen) against guest memory.
स्थिर
पूर्णाङ्क copy_मार्गः (वर्ण* o_मार्गः, ध्रुव वर्ण* i_मार्गः, __wasi_size_t i_मार्गः_Len)
{
    यदि (i_मार्गः_Len >= d_m3_Wasi_Max_मार्गः) {
        प्रत्यागम 0;
    }
    स्मृतिं_प्रतिलिख(o_मार्गः, i_मार्गः, i_मार्गः_Len);
    o_मार्गः[i_मार्गः_Len] = '\0';
    प्रत्यागम 1;
}

// Writes a filestat out: dev 0, ino 8, filetype 16, nlink 24, size 32, atim 40,
// mtim 48, ctim 56. The caller has checked the buffer for d_m3WasiFilestatSize.
स्थिर
शून्य write_filestat (uint8_t* o_buf, ध्रुव m3_wasi_filestat_t* i_stat)
{
    स्मृतिं_पूरय(o_buf, 0, d_m3_Wasi_Filestat_आकार);

    m3ApiWriteMem64(o_buf + 0, i_stat->dev);
    m3ApiWriteMem64(o_buf + 8, i_stat->ino);
    m3ApiWriteMem8(o_buf + 16, i_stat->filetype);
    m3ApiWriteMem64(o_buf + 24, i_stat->nlink);
    m3ApiWriteMem64(o_buf + 32, i_stat->आकार);
    m3ApiWriteMem64(o_buf + 40, i_stat->atim);
    m3ApiWriteMem64(o_buf + 48, i_stat->mtim);
    m3ApiWriteMem64(o_buf + 56, i_stat->ctim);
}

// Resolves the guest's scatter/gather list to host addresses, checking each buffer
// against guest memory, and adds up how much they ask for. The host layer is handed
// the result and never sees a guest offset.
स्थिर
ध्रुव शून्य* resolve_iovs (IM3Runtime runtime, शून्य* _mem, m3_wasi_iovec_t* o_iovs,
                          ध्रुव wasi_iovec_t* i_wasiIovs, __wasi_size_t i_गणनम्,
                          __wasi_size_t* o_want)
{
    *o_want = 0;

    __wasi_size_t want = 0;

    प्रति (__wasi_size_t i = 0; i < i_गणनम्; i++) {
        o_iovs[i].buf = m3_Api_विस्थापनम्_गन्तव्यम्_Ptr(m3ApiReadMem32(&i_wasiIovs[i].buf));
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

m3ApiRawFunction(m3_wasi_p1_तर्काः_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, argv)
    m3ApiGetArgMem(वर्ण*, argv_buf)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    यदि (context == रिक्त) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3ApiCheckMem(argv, context->argc * परिमाण(uint32_t));

    प्रति (u32 i = 0; i < context->argc; ++i) {
        m3ApiWriteMem32(&argv[i], m3_Api_Ptr_गन्तव्यम्_विस्थापनम्(argv_buf));

        size_t len = वाक्यदैर्घ्यम्(context->argv[i]);

        m3ApiCheckMem(argv_buf, len + 1);       // include the terminating `\0`
        स्मृतिं_प्रतिलिख(argv_buf, context->argv[i], len);
        argv_buf += len;
        *argv_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_तर्काः_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, argc)
    m3ApiGetArgMem(__wasi_size_t*, argv_buf_आकार)

    m3ApiCheckMem(argc, परिमाण(__wasi_size_t));
    m3ApiCheckMem(argv_buf_आकार, परिमाण(__wasi_size_t));

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    यदि (context == रिक्त) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_size_t buf_len = 0;
    प्रति (u32 i = 0; i < context->argc; ++i) {
        buf_len += (__wasi_size_t)वाक्यदैर्घ्यम्(context->argv[i]) + 1;
    }

    m3ApiWriteMem32(argc, context->argc);
    m3ApiWriteMem32(argv_buf_आकार, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(uint32_t*, env)
    m3ApiGetArgMem(वर्ण*, env_buf)

    __wasi_size_t गणनम् = 0;
    प्रति (ध्रुव वर्ण** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        गणनम्++;
    }

    m3ApiCheckMem(env, गणनम् * परिमाण(uint32_t));

    प्रति (u32 i = 0; i < गणनम्; ++i) {
        m3ApiWriteMem32(&env[i], m3_Api_Ptr_गन्तव्यम्_विस्थापनम्(env_buf));

        size_t len = वाक्यदैर्घ्यम्(DEFAULT_ENVIRONMENT[i]);

        m3ApiCheckMem(env_buf, len + 1);        // include the terminating `\0`
        स्मृतिं_प्रतिलिख(env_buf, DEFAULT_ENVIRONMENT[i], len);
        env_buf += len;
        *env_buf++ = 0;
    }

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_environ_sizes_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(__wasi_size_t*, env_गणनम्)
    m3ApiGetArgMem(__wasi_size_t*, env_buf_आकार)

    m3ApiCheckMem(env_गणनम्, परिमाण(__wasi_size_t));
    m3ApiCheckMem(env_buf_आकार, परिमाण(__wasi_size_t));

    __wasi_size_t गणनम् = 0, buf_len = 0;
    प्रति (ध्रुव वर्ण** e = DEFAULT_ENVIRONMENT; *e; ++e) {
        गणनम्++;
        buf_len += (__wasi_size_t)वाक्यदैर्घ्यम्(*e) + 1;
    }

    m3ApiWriteMem32(env_गणनम्, गणनम्);
    m3ApiWriteMem32(env_buf_आकार, buf_len);

    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_dir_name)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(वर्ण*, मार्गः)
    m3ApiGetArg(__wasi_size_t, मार्गः_len)

    m3ApiCheckMem(मार्गः, मार्गः_len);

    यदि (fd < 3 || fd >= d_m3_Wasi_Preopen_गणनम्) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }
    size_t slen = वाक्यदैर्घ्यम्(m3_wasi_preopen[fd].मार्गः) + 1;
    स्मृतिं_प्रतिलिख(मार्गः, m3_wasi_preopen[fd].मार्गः, M3_MIN(slen, मार्गः_len));
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_prestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, 8);

    यदि (fd < 3 || fd >= d_m3_Wasi_Preopen_गणनम्) {
        m3ApiReturn(__WASI_ERRNO_BADF);
    }

    m3ApiWriteMem32(buf + 0, __WASI_PREOPENTYPE_DIR);
    m3ApiWriteMem32(buf + 4, (u32)वाक्यदैर्घ्यम्(m3_wasi_preopen[fd].मार्गः) + 1);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_fdstat_t*, fdstat)

    m3ApiCheckMem(fdstat, परिमाण(__wasi_fdstat_t));

    __wasi_filetype_t filetype;
    __wasi_fdflags_t  flags;

    __wasi_errno_t err = m3_wasi_host_fd_fdstat(fd, &filetype, &flags);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    fdstat->fs_filetype = filetype;
    m3ApiWriteMem16(&fdstat->fs_flags, flags);

    fdstat->fs_rights_आधारः = (uint64_t)-1; // all rights

    // Make descriptors 0,1,2 look like a TTY
    यदि (fd <= 2) {
        fdstat->fs_rights_आधारः &= ~(__WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_TELL);
    }

    fdstat->fs_rights_inheriting = (uint64_t)-1; // all rights
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_fdstat_समुच्चयः_flags)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_fdflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_fdstat_समुच्चयः_flags(fd, flags));
}

m3ApiRawFunction(m3_wasi_p1_fd_advise)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, offset)
    m3ApiGetArg(__wasi_filesize_t, दीर्घता)
    m3ApiGetArg(__wasi_advice_t, advice)

    m3ApiReturn(m3_wasi_host_fd_advise(fd, offset, दीर्घता, advice));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_आकार);

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t err = m3_wasi_host_fd_filestat(fd, &fd_stat);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &fd_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_filestat_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ध्रुव वर्ण*, मार्गः)
    m3ApiGetArg(__wasi_size_t, मार्गः_len)
    m3ApiGetArgMem(uint8_t*, buf)

    m3ApiCheckMem(मार्गः, मार्गः_len);
    m3ApiCheckMem(buf, d_m3_Wasi_Filestat_आकार);

    वर्ण host_मार्गः[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_मार्गः, मार्गः, मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3_wasi_filestat_t मार्गः_stat;
    bool               follow = (dirflags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    __wasi_errno_t err = m3_wasi_host_मार्गः_filestat(dirfd, host_मार्गः, follow, &मार्गः_stat);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    write_filestat(buf, &मार्गः_stat);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_समुच्चयः_आकार)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filesize_t, आकार)

    m3ApiReturn(m3_wasi_host_fd_समुच्चयः_आकार(fd, आकार));
}

m3ApiRawFunction(m3_wasi_p1_fd_filestat_समुच्चयः_times)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_timestamp_t, atim)
    m3ApiGetArg(__wasi_timestamp_t, mtim)
    m3ApiGetArg(__wasi_fstflags_t, flags)

    m3ApiReturn(m3_wasi_host_fd_समुच्चयः_times(fd, atim, mtim, flags));
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_link)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, old_flags)
    m3ApiGetArgMem(ध्रुव वर्ण*, old_मार्गः)
    m3ApiGetArg(__wasi_size_t, old_मार्गः_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, new_मार्गः)
    m3ApiGetArg(__wasi_size_t, new_मार्गः_len)

    m3ApiCheckMem(old_मार्गः, old_मार्गः_len);
    m3ApiCheckMem(new_मार्गः, new_मार्गः_len);

    वर्ण host_old[d_m3_Wasi_Max_मार्गः];
    वर्ण host_new[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_old, old_मार्गः, old_मार्गः_len) ||
        !copy_मार्गः(host_new, new_मार्गः, new_मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    bool follow = (old_flags & __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW) != 0;

    m3ApiReturn(m3_wasi_host_मार्गः_link(old_dirfd, host_old, follow, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_create_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, मार्गः)
    m3ApiGetArg(__wasi_size_t, मार्गः_len)

    m3ApiCheckMem(मार्गः, मार्गः_len);

    वर्ण host_मार्गः[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_मार्गः, मार्गः, मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_मार्गः_create_directory(dirfd, host_मार्गः));
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_remove_directory)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, मार्गः)
    m3ApiGetArg(__wasi_size_t, मार्गः_len)

    m3ApiCheckMem(मार्गः, मार्गः_len);

    वर्ण host_मार्गः[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_मार्गः, मार्गः, मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_मार्गः_remove_directory(dirfd, host_मार्गः));
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_unlink_file)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, मार्गः)
    m3ApiGetArg(__wasi_size_t, मार्गः_len)

    m3ApiCheckMem(मार्गः, मार्गः_len);

    वर्ण host_मार्गः[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_मार्गः, मार्गः, मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_मार्गः_unlink_file(dirfd, host_मार्गः));
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_rename)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, old_dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, old_मार्गः)
    m3ApiGetArg(__wasi_size_t, old_मार्गः_len)
    m3ApiGetArg(__wasi_fd_t, new_dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, new_मार्गः)
    m3ApiGetArg(__wasi_size_t, new_मार्गः_len)

    m3ApiCheckMem(old_मार्गः, old_मार्गः_len);
    m3ApiCheckMem(new_मार्गः, new_मार्गः_len);

    वर्ण host_old[d_m3_Wasi_Max_मार्गः];
    वर्ण host_new[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_old, old_मार्गः, old_मार्गः_len) ||
        !copy_मार्गः(host_new, new_मार्गः, new_मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_मार्गः_rename(old_dirfd, host_old, new_dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_symlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ध्रुव वर्ण*, old_मार्गः)
    m3ApiGetArg(__wasi_size_t, old_मार्गः_len)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, new_मार्गः)
    m3ApiGetArg(__wasi_size_t, new_मार्गः_len)

    m3ApiCheckMem(old_मार्गः, old_मार्गः_len);
    m3ApiCheckMem(new_मार्गः, new_मार्गः_len);

    // the target is stored as given, so it is a string rather than a path to resolve
    वर्ण host_old[d_m3_Wasi_Max_मार्गः];
    वर्ण host_new[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_old, old_मार्गः, old_मार्गः_len) ||
        !copy_मार्गः(host_new, new_मार्गः, new_मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    m3ApiReturn(m3_wasi_host_मार्गः_symlink(host_old, dirfd, host_new));
}

m3ApiRawFunction(m3_wasi_p1_मार्गः_readlink)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArgMem(ध्रुव वर्ण*, मार्गः)
    m3ApiGetArg(__wasi_size_t, मार्गः_len)
    m3ApiGetArgMem(वर्ण*, buf)
    m3ApiGetArg(__wasi_size_t, buf_len)
    m3ApiGetArgMem(__wasi_size_t*, bufused)

    m3ApiCheckMem(मार्गः, मार्गः_len);
    m3ApiCheckMem(buf, buf_len);
    m3ApiCheckMem(bufused, परिमाण(__wasi_size_t));

    वर्ण host_मार्गः[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_मार्गः, मार्गः, मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_size_t प्रयुक्तम्;

    __wasi_errno_t err = m3_wasi_host_मार्गः_readlink(dirfd, host_मार्गः, buf, buf_len, &प्रयुक्तम्);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(bufused, प्रयुक्तम्);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_tell)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(__wasi_filesize_t*, परिणाम)

    m3ApiCheckMem(परिणाम, परिमाण(__wasi_filesize_t));

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, 0, SEEK_CUR, &pos);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(परिणाम, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_seek)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArg(__wasi_filedelta_t, offset)
    m3ApiGetArg(uint32_t, wasi_whence)
    m3ApiGetArgMem(__wasi_filesize_t*, परिणाम)

    m3ApiCheckMem(परिणाम, परिमाण(__wasi_filesize_t));

    पूर्णाङ्क whence;

    विकल्प (wasi_whence) {
    पक्ष 0: whence = SEEK_SET; भङ्ग;
    पक्ष 1: whence = SEEK_CUR; भङ्ग;
    पक्ष 2: whence = SEEK_END; भङ्ग;
    स्वभाव: m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    __wasi_filesize_t pos;

    __wasi_errno_t err = m3_wasi_host_fd_seek(fd, offset, whence, &pos);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(परिणाम, pos);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


m3ApiRawFunction(m3_wasi_p1_मार्गः_open)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, dirfd)
    m3ApiGetArg(__wasi_lookupflags_t, dirflags)
    m3ApiGetArgMem(ध्रुव वर्ण*, मार्गः)
    m3ApiGetArg(__wasi_size_t, मार्गः_len)
    m3ApiGetArg(__wasi_oflags_t, oflags)
    m3ApiGetArg(__wasi_rights_t, fs_rights_आधारः)
    m3ApiGetArg(__wasi_rights_t, fs_rights_inheriting)
    m3ApiGetArg(__wasi_fdflags_t, fs_flags)
    m3ApiGetArgMem(__wasi_fd_t*, fd)

    m3ApiCheckMem(मार्गः, मार्गः_len);
    m3ApiCheckMem(fd, परिमाण(__wasi_fd_t));

    वर्ण host_मार्गः[d_m3_Wasi_Max_मार्गः];
    यदि (!copy_मार्गः(host_मार्गः, मार्गः, मार्गः_len)) {
        m3ApiReturn(__WASI_ERRNO_NAMETOOLONG);
    }

    __wasi_fd_t host_fd;

    __wasi_errno_t err = m3_wasi_host_मार्गः_open(dirfd, host_मार्गः, oflags, fs_rights_आधारः,
                                                fs_flags, &host_fd);
    यदि (err != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(wasi_iovs, iovs_len * परिमाण(wasi_iovec_t));
    m3ApiCheckMem(nread, परिमाण(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   योग = 0;

    प्रति (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t गणनम् = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, समाप्तम्;

        ध्रुव शून्य* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, गणनम्, &want);
        यदि (mem_check != m3Err_none) {
            प्रत्यागम mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_read(fd, iovs, गणनम्, &समाप्तम्);
        यदि (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        योग += समाप्तम्;
        यदि (समाप्तम् < want) {
            भङ्ग;
        }
    }

    m3ApiWriteMem32(nread, योग);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}

m3ApiRawFunction(m3_wasi_p1_fd_write)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_fd_t, fd)
    m3ApiGetArgMem(wasi_iovec_t*, wasi_iovs)
    m3ApiGetArg(__wasi_size_t, iovs_len)
    m3ApiGetArgMem(__wasi_size_t*, nwritten)

    m3ApiCheckMem(wasi_iovs, iovs_len * परिमाण(wasi_iovec_t));
    m3ApiCheckMem(nwritten, परिमाण(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   योग = 0;

    प्रति (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t गणनम् = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, समाप्तम्;

        ध्रुव शून्य* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, गणनम्, &want);
        यदि (mem_check != m3Err_none) {
            प्रत्यागम mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_write(fd, iovs, गणनम्, &समाप्तम्);
        यदि (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        योग += समाप्तम्;
        यदि (समाप्तम् < want) {
            भङ्ग;
        }
    }

    m3ApiWriteMem32(nwritten, योग);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * परिमाण(wasi_iovec_t));
    m3ApiCheckMem(nread, परिमाण(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   योग = 0;

    प्रति (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t गणनम् = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, समाप्तम्;

        ध्रुव शून्य* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, गणनम्, &want);
        यदि (mem_check != m3Err_none) {
            प्रत्यागम mem_check;
        }

        // the offset walks with the data, so a short batch leaves no hole behind it
        __wasi_errno_t err = m3_wasi_host_fd_pread(fd, iovs, गणनम्, offset + योग, &समाप्तम्);
        यदि (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        योग += समाप्तम्;
        यदि (समाप्तम् < want) {
            भङ्ग;
        }
    }

    m3ApiWriteMem32(nread, योग);
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

    m3ApiCheckMem(wasi_iovs, iovs_len * परिमाण(wasi_iovec_t));
    m3ApiCheckMem(nwritten, परिमाण(__wasi_size_t));

    m3_wasi_iovec_t iovs[d_m3WasiMaxIovs];
    __wasi_size_t   योग = 0;

    प्रति (__wasi_size_t i = 0; i < iovs_len; i += d_m3WasiMaxIovs) {
        __wasi_size_t गणनम् = M3_MIN(iovs_len - i, (__wasi_size_t)d_m3WasiMaxIovs);
        __wasi_size_t want, समाप्तम्;

        ध्रुव शून्य* mem_check = resolve_iovs(runtime, _mem, iovs, wasi_iovs + i, गणनम्, &want);
        यदि (mem_check != m3Err_none) {
            प्रत्यागम mem_check;
        }

        __wasi_errno_t err = m3_wasi_host_fd_pwrite(fd, iovs, गणनम्, offset + योग, &समाप्तम्);
        यदि (err != __WASI_ERRNO_SUCCESS) {
            m3ApiReturn(err);
        }

        योग += समाप्तम्;
        यदि (समाप्तम् < want) {
            भङ्ग;
        }
    }

    m3ApiWriteMem32(nwritten, योग);
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

#  यदि d_m3DeterministicProfile
    Deterministic_Random(runtime, buf, buf_len);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  अन्यथा
    m3ApiReturn(m3_wasi_host_random_get(buf, buf_len));
#  endif
}

m3ApiRawFunction(m3_wasi_p1_clock_res_get)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArg(__wasi_clockid_t, wasi_clk_id)
    m3ApiGetArgMem(__wasi_timestamp_t*, resolution)

    m3ApiCheckMem(resolution, परिमाण(__wasi_timestamp_t));

#  यदि d_m3DeterministicProfile
    // the deterministic clock's resolution is the step it moves in
    m3ApiWriteMem64(resolution, d_m3DeterministicTickNs);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  अन्यथा
    __wasi_timestamp_t res;

    __wasi_errno_t err = m3_wasi_host_clock_res_get(wasi_clk_id, &res);
    यदि (err != __WASI_ERRNO_SUCCESS) {
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

    m3ApiCheckMem(time, परिमाण(__wasi_timestamp_t));

#  यदि d_m3DeterministicProfile
    // Every clock the guest can name is the same underlying count - a runtime that is
    // not measuring the machine has nothing to tell process time and thread time apart
    // with - and only where it is counted from differs
    __wasi_timestamp_t now = Deterministic_Time(runtime);

    यदि (wasi_clk_id == __WASI_CLOCKID_REALTIME) {
        now += d_m3WasiDeterministicEpochNs;
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  अन्यथा
    __wasi_timestamp_t now;

    __wasi_errno_t err = m3_wasi_host_clock_time_get(wasi_clk_id, &now);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem64(time, now);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
#  endif
}

m3ApiRawFunction(m3_wasi_p1_proc_exit)
{
    m3ApiGetArg(uint32_t, संहिता)

    m3_wasi_context_t* context = (m3_wasi_context_t*)(_ctx->userdata);

    यदि (context) {
        context->exit_संहिता = संहिता;
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
स्थिर
m3_wasi_pollsub_t* read_subscriptions (ध्रुव uint8_t* i_subs, __wasi_size_t i_गणनम्)
{
    m3_wasi_pollsub_t* subs =
      (m3_wasi_pollsub_t*)m3_Malloc("WASI subscriptions", i_गणनम् * परिमाण(m3_wasi_pollsub_t));

    यदि (subs == रिक्त) {
        प्रत्यागम रिक्त;
    }

    प्रति (__wasi_size_t i = 0; i < i_गणनम्; i++) {
        ध्रुव uint8_t* s = i_subs + (size_t)i * d_m3_Wasi_Subscription_आकार;

        स्मृतिं_पूरय(&subs[i], 0, परिमाण(subs[i]));

        subs[i].userdata = m3ApiReadMem64(s + 0);
        subs[i].type     = m3ApiReadMem8(s + 8);

        यदि (subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            subs[i].clockId = m3ApiReadMem32(s + 16);
            subs[i].timeout = m3ApiReadMem64(s + 24);
            subs[i].isAbsolute =
              (m3ApiReadMem16(s + 40) & __WASI_SUBCLOCKFLAGS_SUBSCRIPTION_CLOCK_ABSTIME) != 0;
        } अन्यथा {
            subs[i].fd = m3ApiReadMem32(s + 16);
        }
    }

    प्रत्यागम subs;
}

// Writes one event back. This record is the same in both revisions: userdata 0,
// error 8, type 10, nbytes 16, flags 24. What the union does not cover is zeroed
// rather than left as whatever the guest had there - an event for a clock says
// nothing about nbytes, and the guest should not read its own stale bytes back as
// one.
स्थिर
शून्य write_event (uint8_t* o_rec, ध्रुव m3_wasi_pollevent_t* i_event)
{
    स्मृतिं_पूरय(o_rec, 0, d_m3_Wasi_Event_आकार);

    m3ApiWriteMem64(o_rec + 0, i_event->userdata);
    m3ApiWriteMem16(o_rec + 8, i_event->error);
    m3ApiWriteMem8(o_rec + 10, i_event->type);
    m3ApiWriteMem64(o_rec + 16, i_event->nbytes);
    m3ApiWriteMem16(o_rec + 24, i_event->flags);
}

#  यदि d_m3DeterministicProfile

// poll_oneoff under the deterministic profile, which cannot wait on anything real. A
// descriptor is reported ready at once - which is what the spec says a regular file
// always is, and the only answer here that does not depend on how fast something
// else is going - and the clock is moved forward to the soonest deadline, so that a
// guest sleeping still sees the time it slept for.
स्थिर
शून्य poll_deterministically (IM3Runtime io_runtime, ध्रुव m3_wasi_pollsub_t* i_subs,
                             __wasi_size_t i_गणनम्, m3_wasi_pollevent_t* o_events,
                             __wasi_size_t* o_अङ्कः_Events)
{
    __wasi_size_t बहिर्गमनम्     = 0;
    uint64_t      soonest = UINT64_MAX;
    uint64_t      now     = io_runtime->virtualTimeNs;

    प्रति (__wasi_size_t i = 0; i < i_गणनम्; i++) {
        ध्रुव m3_wasi_pollsub_t* sub = &i_subs[i];

        यदि (sub->type != __WASI_EVENTTYPE_CLOCK) {
            o_events[बहिर्गमनम्].userdata = sub->userdata;
            o_events[बहिर्गमनम्].error    = __WASI_ERRNO_SUCCESS;
            o_events[बहिर्गमनम्].type     = sub->type;
            o_events[बहिर्गमनम्].nbytes   = 1;
            o_events[बहिर्गमनम्].flags    = 0;
            बहिर्गमनम्++;
            अनुवर्त;
        }

        // an absolute deadline is against the same virtual clock the guest reads
        uint64_t deadline = sub->isAbsolute ? sub->timeout : now + sub->timeout;
        soonest           = M3_MIN(soonest, deadline);
    }

    // Something is ready already, so no time passes - which is what poll() would
    // have done too
    यदि (बहिर्गमनम्) {
        *o_अङ्कः_Events = बहिर्गमनम्;
        प्रत्यागम;
    }

    यदि (soonest > now) {
        Deterministic_Advance(io_runtime, soonest - now);
    }

    now = io_runtime->virtualTimeNs;

    प्रति (__wasi_size_t i = 0; i < i_गणनम्; i++) {
        ध्रुव m3_wasi_pollsub_t* sub = &i_subs[i];

        यदि (sub->type != __WASI_EVENTTYPE_CLOCK) {
            अनुवर्त;
        }

        uint64_t deadline = sub->isAbsolute ? sub->timeout : soonest;

        यदि (deadline <= now) {
            o_events[बहिर्गमनम्].userdata = sub->userdata;
            o_events[बहिर्गमनम्].error    = __WASI_ERRNO_SUCCESS;
            o_events[बहिर्गमनम्].type     = __WASI_EVENTTYPE_CLOCK;
            o_events[बहिर्गमनम्].nbytes   = 0;
            o_events[बहिर्गमनम्].flags    = 0;
            बहिर्गमनम्++;
        }
    }

    *o_अङ्कः_Events = बहिर्गमनम्;
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
स्थिर
__wasi_errno_t poll_subscriptions (IM3Runtime i_runtime, ध्रुव m3_wasi_pollsub_t* i_subs,
                                   uint8_t* o_events, __wasi_size_t i_गणनम्,
                                   __wasi_size_t* o_अङ्कः_Events)
{
    *o_अङ्कः_Events = 0;

    m3_wasi_pollevent_t* evts =
      (m3_wasi_pollevent_t*)m3_Malloc("WASI events", i_गणनम् * परिमाण(m3_wasi_pollevent_t));

    यदि (evts == रिक्त) {
        प्रत्यागम __WASI_ERRNO_NOMEM;
    }

    __wasi_size_t  अङ्कः_Events = 0;
    __wasi_errno_t err       = __WASI_ERRNO_SUCCESS;

#  यदि d_m3DeterministicProfile
    poll_deterministically(i_runtime, i_subs, i_गणनम्, evts, &अङ्कः_Events);
#  अन्यथा
    err = m3_wasi_host_poll_oneoff(i_subs, i_गणनम्, evts, &अङ्कः_Events);
#  endif

    यदि (err == __WASI_ERRNO_SUCCESS) {
        प्रति (__wasi_size_t i = 0; i < अङ्कः_Events; i++) {
            write_event(o_events + (size_t)i * d_m3_Wasi_Event_आकार, &evts[i]);
        }
        *o_अङ्कः_Events = अङ्कः_Events;
    }

    m3_Free(evts);

    प्रत्यागम err;
}

m3ApiRawFunction(m3_wasi_p1_poll_oneoff)
{
    m3ApiReturnType(uint32_t)
    m3ApiGetArgMem(ध्रुव uint8_t*, अन्तर्)
    m3ApiGetArgMem(uint8_t*, बहिर्गमनम्)
    m3ApiGetArg(__wasi_size_t, nsubscriptions)
    m3ApiGetArgMem(__wasi_size_t*, nevents)

    m3ApiCheckMem(अन्तर्, (uint64_t)nsubscriptions * d_m3_Wasi_Subscription_आकार);
    m3ApiCheckMem(बहिर्गमनम्, (uint64_t)nsubscriptions * d_m3_Wasi_Event_आकार);
    m3ApiCheckMem(nevents, परिमाण(__wasi_size_t));

    यदि (nsubscriptions == 0) {
        m3ApiReturn(__WASI_ERRNO_INVAL);
    }

    m3_wasi_pollsub_t* subs = read_subscriptions(अन्तर्, nsubscriptions);

    यदि (subs == रिक्त) {
        m3ApiReturn(__WASI_ERRNO_NOMEM);
    }

    __wasi_size_t  अङ्कः_Events = 0;
    __wasi_errno_t err       = poll_subscriptions(runtime, subs, बहिर्गमनम्, nsubscriptions, &अङ्कः_Events);

    m3_Free(subs);

    यदि (err != __WASI_ERRNO_SUCCESS) {
        m3ApiReturn(err);
    }

    m3ApiWriteMem32(nevents, अङ्कः_Events);
    m3ApiReturn(__WASI_ERRNO_SUCCESS);
}


स्थिर
M3_परिणाम SuppressLookupFailure (M3_परिणाम i_परिणाम)
{
    यदि (i_परिणाम == m3Err_functionLookupFailed) {
        प्रत्यागम m3Err_none;
    } अन्यथा {
        प्रत्यागम i_परिणाम;
    }
}

m3_wasi_context_t* m3_GetWasiContext ()
{
    प्रत्यागम wasi_context;
}

शून्य m3_FreeWasi (m3_wasi_context_t* wasi)
{
    m3_Free(wasi);
}

स्थिर अन्तःस्थ
M3_परिणाम _linkWASI (IM3Module module, m3_wasi_context_t* wasi_context)
{
    M3_परिणाम परिणाम = m3Err_none;

    m3_wasi_host_init();

    स्थिर ध्रुव वर्ण* namespaces[] = { "wasi_snapshot_preview1" };

    // clang-format off

    प्रति (size_t i = 0; i < M3_गणनम्_सम्बन्धि(namespaces); i++) {
        ध्रुव वर्ण* wasi = namespaces[i];

_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_get",           "i(**)",   &m3_wasi_p1_तर्काः_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunctionEx(module, wasi, "args_sizes_get",     "i(**)",   &m3_wasi_p1_तर्काः_sizes_get, wasi_context)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_res_get",        "i(i*)",   &m3_wasi_p1_clock_res_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "clock_time_get",       "i(iI*)",  &m3_wasi_p1_clock_time_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_get",          "i(**)",   &m3_wasi_p1_environ_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "environ_sizes_get",    "i(**)",   &m3_wasi_p1_environ_sizes_get)));

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_advise",            "i(iIIi)", &m3_wasi_p1_fd_advise)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_allocate",          "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_close",             "i(i)",    &m3_wasi_p1_fd_close)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_datasync",          "i(i)",    &m3_wasi_p1_fd_datasync)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_get",        "i(i*)",   &m3_wasi_p1_fd_fdstat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_flags",  "i(ii)",   &m3_wasi_p1_fd_fdstat_समुच्चयः_flags)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_fdstat_set_rights", "i(iII)",  )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_get",      "i(i*)",   &m3_wasi_p1_fd_filestat_get)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_size", "i(iI)",   &m3_wasi_p1_fd_filestat_समुच्चयः_आकार)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "fd_filestat_set_times","i(iIIi)", &m3_wasi_p1_fd_filestat_समुच्चयः_times)));
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

_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_create_directory",    "i(i*i)",       &m3_wasi_p1_मार्गः_create_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_get",        "i(ii*i*)",     &m3_wasi_p1_मार्गः_filestat_get)));
//_     (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_filestat_set_times",  "i(ii*iIIi)",   )));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_link",                "i(ii*ii*i)",   &m3_wasi_p1_मार्गः_link)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_open",                "i(ii*iiIIi*)", &m3_wasi_p1_मार्गः_open)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_readlink",            "i(i*i*i*)",    &m3_wasi_p1_मार्गः_readlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_remove_directory",    "i(i*i)",       &m3_wasi_p1_मार्गः_remove_directory)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_rename",              "i(i*ii*i)",    &m3_wasi_p1_मार्गः_rename)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_symlink",             "i(*ii*i)",     &m3_wasi_p1_मार्गः_symlink)));
_       (SuppressLookupFailure(m3_LinkRawFunction(module, wasi, "path_unlink_file",         "i(i*i)",       &m3_wasi_p1_मार्गः_unlink_file)));

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
    यदि (Module_HasLinkedHostImport(module, "wasi_snapshot_preview1")) {
        u32 स्मृतिः_सूचकांक;

        यदि (m3_Find_Exported_स्मृतिः(module, "memory", &स्मृतिः_सूचकांक)) {
            _throw("WASI requires the module to export its memory as \"memory\"");
        }

_       (m3_Bind_Import_स्मृतिः(module, "wasi_snapshot_preview1", स्मृतिः_सूचकांक));
    }

_catch:
    प्रत्यागम परिणाम;
}

M3_परिणाम m3_NewCustomWASI (M3WASI* wasi_p)
{
    *wasi_p = m3_AllocStruct(m3_wasi_context_t);

    प्रत्यागम m3Err_none;
}

शून्य m3_FreeCustomWASI (M3WASI wasi)
{
    m3_Free(wasi);
}

M3_परिणाम m3_LinkCustomWASI (IM3Module module, M3WASI wasi)
{
    प्रत्यागम _linkWASI(module, wasi);
}

M3_परिणाम m3_LinkWASI (IM3Module module)
{
    यदि (!wasi_context) {
        wasi_context = m3_AllocStruct(m3_wasi_context_t);
    }

    प्रत्यागम _linkWASI(module, wasi_context);
}

#यदि_अन्तः // d_m3HasWASI
