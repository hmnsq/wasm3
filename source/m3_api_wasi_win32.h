//
//  m3_api_wasi_win32.h
//
//  The Win32 implementation of m3_api_wasi_host.h. Windows has no *at() family, no
//  fstat that names a file, no positional read or write and no readlink, so a path
//  is resolved by writing the preopen's real path in front of it and the rest is
//  assembled out of the Win32 API.
//
//  A header rather than a translation unit of its own, and included by m3_api_wasi.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#ifndef m3_api_wasi_win32_h
#define m3_api_wasi_win32_h

#include "m3_core.h"

#නම් !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#include <Windows.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <wchar.h>

// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#define SystemFunction036 NTAPI SystemFunction036
#include <NTSecAPI.h>
#undef SystemFunction036

#include "m3_api_wasi_host.h"

// Room for a preopen's real path in front of the guest's
#define d_m3WasiMaxHostPath  (d_m3WasiMaxPath + 16)

// 1601-01-01, where a FILETIME counts its 100ns ticks from, in those same ticks
#define d_m3WinEpochOffset   116444736000000000ULL

// Only the Windows 10 SDK and later declare it
#නම් !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#endif

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#නම් !defined(FSCTL_GET_REPARSE_POINT)
#  define FSCTL_GET_REPARSE_POINT  0x000900A8
#endif
#නම් !defined(MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
#  define MAXIMUM_REPARSE_DATA_BUFFER_SIZE  (16 * 1024)
#endif

typedef sa_nracanaa ReparseDataBuffer {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    USHORT SubstituteNameOffset;
    USHORT SubstituteNameLength;
    USHORT PrintNameOffset;
    USHORT PrintNameLength;
    sa_ngha {
        sa_nracanaa {
            ULONG Flags;
            WCHAR PathBuffer[1];
        } symlink;
        sa_nracanaa {
            WCHAR PathBuffer[1];
        } mount_point;
    } u;
} ReparseDataBuffer;


/*
 * Plumbing
 */

// What the CRT calls used here leave in errno. Everything the Win32 API reports goes
// through errno_from_win32 instead.
static
__wasi_errno_t errno_from_crt (පූර්ණ_සංඛ්‍යා err)
{
    // clang-format off
    switch (err) {
    pak_sa 0:       ආපසු __WASI_ERRNO_SUCCESS;
    pak_sa EACCES:  ආපසු __WASI_ERRNO_ACCES;
    pak_sa EBADF:   ආපසු __WASI_ERRNO_BADF;
    pak_sa EEXIST:  ආපසු __WASI_ERRNO_EXIST;
    pak_sa EMFILE:  ආපසු __WASI_ERRNO_MFILE;
    pak_sa ENOENT:  ආපසු __WASI_ERRNO_NOENT;
    pak_sa ENOSPC:  ආපසු __WASI_ERRNO_NOSPC;
    pak_sa EINVAL:  ආපසු __WASI_ERRNO_INVAL;
    }
    // clang-format on
    ආපසු __WASI_ERRNO_IO;
}

static
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    switch (err) {
    pak_sa ERROR_SUCCESS:               ආපසු __WASI_ERRNO_SUCCESS;
    pak_sa ERROR_INVALID_FUNCTION:      ආපසු __WASI_ERRNO_NOTSUP;
    pak_sa ERROR_FILE_NOT_FOUND:
    pak_sa ERROR_PATH_NOT_FOUND:
    pak_sa ERROR_INVALID_DRIVE:
    pak_sa ERROR_BAD_NETPATH:
    pak_sa ERROR_BAD_PATHNAME:          ආපසු __WASI_ERRNO_NOENT;
    pak_sa ERROR_TOO_MANY_OPEN_FILES:   ආපසු __WASI_ERRNO_MFILE;
    pak_sa ERROR_ACCESS_DENIED:
    pak_sa ERROR_WRITE_PROTECT:         ආපසු __WASI_ERRNO_ACCES;
    pak_sa ERROR_INVALID_HANDLE:        ආපසු __WASI_ERRNO_BADF;
    pak_sa ERROR_NOT_ENOUGH_MEMORY:
    pak_sa ERROR_OUTOFMEMORY:           ආපසු __WASI_ERRNO_NOMEM;
    pak_sa ERROR_NOT_SAME_DEVICE:       ආපසු __WASI_ERRNO_XDEV;
    pak_sa ERROR_WRITE_FAULT:
    pak_sa ERROR_READ_FAULT:            ආපසු __WASI_ERRNO_IO;
    pak_sa ERROR_SHARING_VIOLATION:
    pak_sa ERROR_LOCK_VIOLATION:        ආපසු __WASI_ERRNO_BUSY;
    pak_sa ERROR_HANDLE_DISK_FULL:
    pak_sa ERROR_DISK_FULL:             ආපසු __WASI_ERRNO_NOSPC;
    pak_sa ERROR_FILE_EXISTS:
    pak_sa ERROR_ALREADY_EXISTS:        ආපසු __WASI_ERRNO_EXIST;
    pak_sa ERROR_BUFFER_OVERFLOW:
    pak_sa ERROR_INSUFFICIENT_BUFFER:
    pak_sa ERROR_FILENAME_EXCED_RANGE:  ආපසු __WASI_ERRNO_NAMETOOLONG;
    pak_sa ERROR_DIR_NOT_EMPTY:         ආපසු __WASI_ERRNO_NOTEMPTY;
    pak_sa ERROR_DIRECTORY:             ආපසු __WASI_ERRNO_NOTDIR;
    pak_sa ERROR_PRIVILEGE_NOT_HELD:    ආපසු __WASI_ERRNO_PERM;
    pak_sa ERROR_NOT_SUPPORTED:         ආපසු __WASI_ERRNO_NOTSUP;
    pak_sa ERROR_CANT_RESOLVE_FILENAME: ආපසු __WASI_ERRNO_LOOP;
    pak_sa ERROR_NEGATIVE_SEEK:
    pak_sa ERROR_INVALID_NAME:
    pak_sa ERROR_INVALID_PARAMETER:
    pak_sa ERROR_NOT_A_REPARSE_POINT:   ආපසු __WASI_ERRNO_INVAL;
    }
    // clang-format on
    ආපසු __WASI_ERRNO_IO;
}

static
__wasi_timestamp_t convert_filetime (FILETIME i_time)
{
    uint64_t ticks = ((uint64_t)i_time.dwHighDateTime << 32) | i_time.dwLowDateTime;
    නම් (ticks < d_m3WinEpochOffset) {
        ආපසු 0;
    }
    ආපසු (ticks - d_m3WinEpochOffset) * 100;
}

static
FILETIME convert_timestamp (__wasi_timestamp_t i_time)
{
    uint64_t ticks = i_time / 100 + d_m3WinEpochOffset;
    FILETIME out;
    out.dwLowDateTime = (DWORD)ticks;
    out.dwHighDateTime = (DWORD)(ticks >> 32);
    ආපසු out;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
static
bool is_preopen (__wasi_fd_t i_fd)
{
    ආපසු i_fd >= 3 && i_fd < d_m3WasiPreopenCount && m3_wasi_preopen[i_fd].fd == (පූර්ණ_සංඛ්‍යා)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
static
__wasi_errno_t resolve_path (var_na* o_path, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    නම් (!is_preopen(i_dirfd)) {
        ආපසු __WASI_ERRNO_BADF;
    }

    පූර්ණ_සංඛ්‍යා len = snprintf(o_path, d_m3WasiMaxHostPath, "%s/%s", m3_wasi_preopen[i_dirfd].real_path,
                       i_path);
    නම් (len < 0 || len >= d_m3WasiMaxHostPath) {
        ආපසු __WASI_ERRNO_NAMETOOLONG;
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
static
__wasi_errno_t open_path_handle (ccstr_t i_path, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    නම් (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_POINT;
    }

    HANDLE handle = CreateFileA(i_path, FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                OPEN_EXISTING, flags, NULL);
    නම් (handle == INVALID_HANDLE_VALUE) {
        ආපසු errno_from_win32(GetLastError());
    }

    *o_handle = handle;
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    නම් (handle == INVALID_HANDLE_VALUE) {
        ආපසු __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    ආපසු __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
static
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, parimaa_na(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    නම් (!GetFileInformationByHandle(i_handle, &info)) {
        switch (GetFileType(i_handle)) {
        pak_sa FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; bha_nga;
        pak_sa FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; bha_nga;
        default: ආපසු errno_from_win32(GetLastError());
        }
        ආපසු __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dwVolumeSerialNumber;
    o_stat->ino = ((uint64_t)info.nFileIndexHigh << 32) | info.nFileIndexLow;
    o_stat->nlink = info.nNumberOfLinks;
    o_stat->size = ((uint64_t)info.nFileSizeHigh << 32) | info.nFileSizeLow;
    o_stat->atim = convert_filetime(info.ftLastAccessTime);
    o_stat->mtim = convert_filetime(info.ftLastWriteTime);
    o_stat->ctim = convert_filetime(info.ftCreationTime);

    නම් (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } නැතහොත් නම් (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } නැතහොත් {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}


/*
 * Setup
 */

static
void m3_wasi_host_init ()
{
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);

    // Windows cannot open a directory as a descriptor, so a preopen is resolved by
    // path instead. It still has to hold a descriptor number: the guest is handed
    // one, and without something occupying it the next file opened would be given
    // the same number and the two would be indistinguishable.
    සඳහා (පූර්ණ_සංඛ්‍යා i = 3; i < d_m3WasiPreopenCount; i++) {
        නම් (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = _open("NUL", _O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

static
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    නම් (_close(i_fd) != 0) {
        ආපසු errno_from_crt(errno);
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    නම් (_commit(i_fd) != 0) {
        ආපසු errno_from_crt(errno);
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    නම් (i_advice > __WASI_ADVICE_NOREUSE) {
        ආපසු __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    ආපසු handle_for_fd(i_fd, &handle);
}

static
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, parimaa_na(*o_stat));

    නම් (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        ආපසු __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    ආපසු filestat_from_handle(handle, o_stat);
}

static
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    ආපසු __WASI_ERRNO_SUCCESS;
}

// Nothing here can be changed after the fact: append and non-blocking are fixed when
// CreateFile runs, and the CRT hands back no way to revisit either. fd_fdstat above
// reports no flags at all for the same reason, so the only request that can be
// honoured is the one asking for exactly that.
static
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    ආපසු (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

static
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    ආපසු errno_from_crt(_chsize_s(i_fd, (__int64)i_size));
}

static
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    GetSystemTimeAsFileTime(&now);

    const FILETIME* atime = NULL;
    const FILETIME* mtime = NULL;

    නම් (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } නැතහොත් නම් (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    නම් (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } නැතහොත් නම් (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    නම් (!SetFileTime(handle, NULL, atime, mtime)) {
        ආපසු errno_from_win32(GetLastError());
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, පූර්ණ_සංඛ්‍යා i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_offset, i_whence);
    නම් (ret < 0) {
        ආපසු errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    ආපසු __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

// Windows has neither a vectored nor a positional transfer, so the list is walked
// one buffer at a time and a short transfer ends it, as it would for readv. A
// positional one moves the file pointer and puts it back where it was.
static
__wasi_errno_t transfer_iovs (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                              __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                              bool i_positional, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    int64_t saved = 0;

    නම් (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        නම් (saved < 0 || _lseeki64(i_fd, (__int64)i_offset, SEEK_SET) < 0) {
            ආපසු errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         done = 0;

    සඳහා (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        void*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        නම් (len == 0) {
            continue;
        }

        පූර්ණ_සංඛ්‍යා ret = i_write ? _write(i_fd, addr, (unsigned)len) : _read(i_fd, addr, (unsigned)len);
        නම් (ret < 0) {
            err = errno_from_crt(errno);
            bha_nga;
        }

        done += (size_t)ret;
        නම් ((size_t)ret < len) {
            bha_nga;
        }
    }

    නම් (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    *o_done = (__wasi_size_t)done;
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    ආපසු transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    ආපසු transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    ආපසු transfer_iovs(i_fd, i_iovs, i_iovsLen, i_offset, true, false, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    ආපසු transfer_iovs(i_fd, i_iovs, i_iovsLen, i_offset, true, true, o_done);
}


/*
 * Paths
 */

static
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    var_na           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    නම් (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_path_handle(full_path, true, &handle);
        නම් (err != __WASI_ERRNO_SUCCESS) {
            ආපසු err;
        }

        පූර්ණ_සංඛ්‍යා dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        නම් (dir_fd < 0) {
            CloseHandle(handle);
            ආපසු __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        ආපසු __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    පූර්ණ_සංඛ්‍යා flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                _O_BINARY;

    නම් ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } නැතහොත් නම් ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } නැතහොත් නම් ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    පූර්ණ_සංඛ්‍යා host_fd = _open(full_path, flags, _S_IREAD | _S_IWRITE);
    නම් (host_fd < 0) {
        ආපසු errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, parimaa_na(*o_stat));

    var_na           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    HANDLE handle;
    err = open_path_handle(full_path, i_follow, &handle);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    ආපසු err;
}

static
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    var_na           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    නම් (!CreateDirectoryA(full_path, NULL)) {
        ආපසු errno_from_win32(GetLastError());
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    var_na           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    නම් (!RemoveDirectoryA(full_path)) {
        ආපසු errno_from_win32(GetLastError());
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    var_na           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = GetFileAttributesA(full_path);
    bool  isDirLink =
      attrs != INVALID_FILE_ATTRIBUTES &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT);

    නම් (!(isDirLink ? RemoveDirectoryA(full_path) : DeleteFileA(full_path))) {
        ආපසු errno_from_win32(GetLastError());
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    var_na           full_old[d_m3WasiMaxHostPath];
    var_na           full_new[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_old, i_oldDirfd, i_oldPath);
    නම් (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_path(full_new, i_newDirfd, i_newPath);
    }
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    නම් (!MoveFileExA(full_old, full_new, MOVEFILE_REPLACE_EXISTING)) {
        ආපසු errno_from_win32(GetLastError());
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    var_na           full_old[d_m3WasiMaxHostPath];
    var_na           full_new[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_old, i_oldDirfd, i_oldPath);
    නම් (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_path(full_new, i_newDirfd, i_newPath);
    }
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    නම් (!CreateHardLinkA(full_new, full_old, NULL)) {
        ආපසු errno_from_win32(GetLastError());
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    var_na           full_new[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_new, i_dirfd, i_path);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    var_na   target[d_m3WasiMaxHostPath];
    size_t dir_len = 0;
    සඳහා (size_t i = 0; full_new[i]; i++) {
        නම් (full_new[i] == '/' || full_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    පූර්ණ_සංඛ්‍යා target_len = snprintf(target, parimaa_na(target), "%.*s%s", (පූර්ණ_සංඛ්‍යා)dir_len, full_new, i_target);
    නම් (target_len < 0 || target_len >= (පූර්ණ_සංඛ්‍යා)parimaa_na(target)) {
        ආපසු __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = GetFileAttributesA(target);
    නම් (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    නම් (!CreateSymbolicLinkA(full_new, i_target,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        නම් (GetLastError() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(full_new, i_target, flags)) {
            ආපසු errno_from_win32(GetLastError());
        }
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
static
__wasi_errno_t read_link_target (ccstr_t i_path, var_na* o_target, පූර්ණ_සංඛ්‍යා i_size, පූර්ණ_සංඛ්‍යා* o_len)
{
    HANDLE handle = CreateFileA(i_path, FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    නම් (handle == INVALID_HANDLE_VALUE) {
        ආපසු errno_from_win32(GetLastError());
    }

    ReparseDataBuffer* reparse =
      (ReparseDataBuffer*)m3_Malloc("ReparseDataBuffer", MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    නම් (!reparse) {
        CloseHandle(handle);
        ආපසු __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    නම් (!DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &returned, NULL)) {
        err = errno_from_win32(GetLastError());
    }
    CloseHandle(handle);

    නම් (err == __WASI_ERRNO_SUCCESS) {
        const WCHAR* names;
        නම් (reparse->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
            names = reparse->u.symlink.PathBuffer;
        } නැතහොත් නම් (reparse->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
            names = reparse->u.mount_point.PathBuffer;
        } නැතහොත් {
            names = NULL;
            err = __WASI_ERRNO_INVAL;
        }

        නම් (names) {
            const WCHAR* name = names + reparse->PrintNameOffset / parimaa_na(WCHAR);
            පූර්ණ_සංඛ්‍යා          len = reparse->PrintNameLength / parimaa_na(WCHAR);

            නම් (len == 0) {
                name = names + reparse->SubstituteNameOffset / parimaa_na(WCHAR);
                len = reparse->SubstituteNameLength / parimaa_na(WCHAR);
                නම් (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = WideCharToMultiByte(CP_UTF8, 0, name, len, o_target, i_size, NULL, NULL);
            නම් (*o_len <= 0 && len != 0) {
                err = errno_from_win32(GetLastError());
            }
        }
    }

    m3_Free(reparse);
    ආපසු err;
}

static
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, var_na* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    var_na           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    var_na target[d_m3WasiMaxHostPath];
    පූර්ණ_සංඛ්‍යා  target_len = 0;

    err = read_link_target(full_path, target, (පූර්ණ_සංඛ්‍යා)parimaa_na(target), &target_len);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)target_len, i_bufLen);
    memcpy(o_buf, target, copied);
    *o_bufUsed = copied;
    ආපසු __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

// Windows offers the one clock, so every WASI clock maps onto it and none of them is
// rejected
static
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    // a FILETIME counts in 100ns, but the system clock behind it moves on the timer
    // tick, so this says a millisecond rather than what it can spell
    *o_res = 1000000;
    ආපසු __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
static
__wasi_timestamp_t now_ns (void)
{
    FILETIME now;
    GetSystemTimeAsFileTime(&now);

    ආපසු convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = now_ns();
    ආපසු __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_random_get (void* o_buf, __wasi_size_t i_len)
{
    නම් (!RtlGenRandom(o_buf, i_len)) {
        ආපසු __WASI_ERRNO_IO;
    }
    ආපසු __WASI_ERRNO_SUCCESS;
}


/*
 * Waiting
 */

// Windows has no one call that waits on a file, a pipe and a console together the
// way poll() does - the objects are of different kinds and only some of them are
// waitable at all - so this asks each of them in turn and sleeps between rounds.
// That costs a wakeup every d_m3WasiPollIntervalMs while a wait is outstanding, and
// nothing at all once something is ready. Rebuilding it around overlapped I/O and a
// completion port would take the whole descriptor layer with it, which is more than
// the simple WASI implementation is for - a build that needs to wait on many
// descriptors at once wants BUILD_WASI=uvwasi.
#define d_m3WasiPollIntervalMs  5

// Whether a descriptor has what its subscription is waiting for. Everything that is
// not a stream is ready by definition: the spec says a regular file always triggers
// both read and write, and so does anything whose kind Windows will not describe.
static
__wasi_errno_t poll_fd_ready (const m3_wasi_pollsub_t* i_sub, bool* o_ready,
                              __wasi_filesize_t* o_nbytes, __wasi_eventrwflags_t* o_flags)
{
    *o_ready = false;
    *o_nbytes = 0;
    *o_flags = 0;

    HANDLE         handle;

    __wasi_errno_t err = handle_for_fd(i_sub->fd, &handle);
    නම් (err != __WASI_ERRNO_SUCCESS) {
        ආපසු err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    නම් (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        ආපසු __WASI_ERRNO_SUCCESS;
    }

    නම් (type == FILE_TYPE_PIPE) {
        DWORD waiting = 0;

        නම් (!PeekNamedPipe(handle, NULL, 0, NULL, &waiting, NULL)) {
            DWORD last = GetLastError();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            නම් (last == ERROR_BROKEN_PIPE or last == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                ආපසු __WASI_ERRNO_SUCCESS;
            }
            ආපසු errno_from_win32(last);
        }

        *o_ready = (waiting != 0);
        *o_nbytes = waiting;
        ආපසු __WASI_ERRNO_SUCCESS;
    }

    නම් (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        ආපසු __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    ආපසු __WASI_ERRNO_SUCCESS;
}

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock and ignores i_elapsedNs, the clock having moved on by itself.
// Every WASI clock is the system clock here - see clock_time_get - so an absolute
// deadline is held against that one whichever clock it names.
static
uint64_t poll_remaining_ns (const m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    නම් (!i_sub->isAbsolute) {
        ආපසු (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    ආපසු (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

static
__wasi_errno_t m3_wasi_host_poll_oneoff (const m3_wasi_pollsub_t* i_subs, __wasi_size_t i_count,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_eventCount)
{
    *o_eventCount = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    සඳහා (__wasi_size_t i = 0; i < i_count; i++) {
        නම් (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_remaining_ns(&i_subs[i], 0));
            hasClock = true;
        } නැතහොත් {
            hasFd = true;
        }
    }

    නම් (!hasClock and !hasFd) {
        ආපසු __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t startNs = now_ns();

    සඳහා (;;) {
        __wasi_size_t out = 0;

        සඳහා (__wasi_size_t i = 0; i < i_count; i++) {
            const m3_wasi_pollsub_t* sub = &i_subs[i];

            නම් (sub->type == __WASI_EVENTTYPE_CLOCK) {
                continue;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            නම් (err == __WASI_ERRNO_SUCCESS and !ready) {
                continue;
            }

            o_events[out].userdata = sub->userdata;
            o_events[out].error = err;
            o_events[out].type = sub->type;
            o_events[out].nbytes = nbytes;
            o_events[out].flags = flags;
            out++;
        }

        // This is a wall clock and can be set backwards under us, which is not
        // elapsed time going negative but the measurement being no longer usable;
        // starting the count again from here is the honest reading of it.
        const uint64_t t = now_ns();
        const uint64_t elapsedNs = (t > startNs) ? t - startNs : 0;

        නම් (t < startNs) {
            startNs = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        නම් (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            සඳහා (__wasi_size_t i = 0; i < i_count; i++) {
                const m3_wasi_pollsub_t* sub = &i_subs[i];

                නම් (sub->type == __WASI_EVENTTYPE_CLOCK and
                    poll_remaining_ns(sub, elapsedNs) == 0) {
                    o_events[out].userdata = sub->userdata;
                    o_events[out].error = __WASI_ERRNO_SUCCESS;
                    o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                    o_events[out].nbytes = 0;
                    o_events[out].flags = 0;
                    out++;
                }
            }
        }

        නම් (out) {
            *o_eventCount = out;
            ආපසු __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        නම් (hasClock) {
            uint64_t remainingNs = soonestNs - elapsedNs;
            uint64_t remainingMs = (remainingNs + 999999) / 1000000;

            නම් (not hasFd or remainingMs < sleepMs) {
                sleepMs = (remainingMs > MAXDWORD) ? MAXDWORD : (DWORD)remainingMs;
            }
        }

        Sleep(sleepMs);
    }
}

#endif // m3_api_wasi_win32_h
