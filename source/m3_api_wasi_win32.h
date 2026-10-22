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

#ако !defined(_WIN32)
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
#ако !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#endif

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#ако !defined(FSCTL_GET_REPARSE_POINT)
#  define FSCTL_GET_REPARSE_POINT  0x000900A8
#endif
#ако !defined(MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
#  define MAXIMUM_REPARSE_DATA_BUFFER_SIZE  (16 * 1024)
#endif

typedef struct ReparseDataBuffer {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    USHORT SubstituteNameOffset;
    USHORT SubstituteNameLength;
    USHORT PrintNameOffset;
    USHORT PrintNameLength;
    union {
        struct {
            ULONG Flags;
            WCHAR PathBuffer[1];
        } symlink;
        struct {
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
__wasi_errno_t errno_from_crt (цяло err)
{
    // clang-format off
    switch (err) {
    case 0:       върни __WASI_ERRNO_SUCCESS;
    case EACCES:  върни __WASI_ERRNO_ACCES;
    case EBADF:   върни __WASI_ERRNO_BADF;
    case EEXIST:  върни __WASI_ERRNO_EXIST;
    case EMFILE:  върни __WASI_ERRNO_MFILE;
    case ENOENT:  върни __WASI_ERRNO_NOENT;
    case ENOSPC:  върни __WASI_ERRNO_NOSPC;
    case EINVAL:  върни __WASI_ERRNO_INVAL;
    }
    // clang-format on
    върни __WASI_ERRNO_IO;
}

static
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    switch (err) {
    case ERROR_SUCCESS:               върни __WASI_ERRNO_SUCCESS;
    case ERROR_INVALID_FUNCTION:      върни __WASI_ERRNO_NOTSUP;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
    case ERROR_BAD_NETPATH:
    case ERROR_BAD_PATHNAME:          върни __WASI_ERRNO_NOENT;
    case ERROR_TOO_MANY_OPEN_FILES:   върни __WASI_ERRNO_MFILE;
    case ERROR_ACCESS_DENIED:
    case ERROR_WRITE_PROTECT:         върни __WASI_ERRNO_ACCES;
    case ERROR_INVALID_HANDLE:        върни __WASI_ERRNO_BADF;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:           върни __WASI_ERRNO_NOMEM;
    case ERROR_NOT_SAME_DEVICE:       върни __WASI_ERRNO_XDEV;
    case ERROR_WRITE_FAULT:
    case ERROR_READ_FAULT:            върни __WASI_ERRNO_IO;
    case ERROR_SHARING_VIOLATION:
    case ERROR_LOCK_VIOLATION:        върни __WASI_ERRNO_BUSY;
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:             върни __WASI_ERRNO_NOSPC;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:        върни __WASI_ERRNO_EXIST;
    case ERROR_BUFFER_OVERFLOW:
    case ERROR_INSUFFICIENT_BUFFER:
    case ERROR_FILENAME_EXCED_RANGE:  върни __WASI_ERRNO_NAMETOOLONG;
    case ERROR_DIR_NOT_EMPTY:         върни __WASI_ERRNO_NOTEMPTY;
    case ERROR_DIRECTORY:             върни __WASI_ERRNO_NOTDIR;
    case ERROR_PRIVILEGE_NOT_HELD:    върни __WASI_ERRNO_PERM;
    case ERROR_NOT_SUPPORTED:         върни __WASI_ERRNO_NOTSUP;
    case ERROR_CANT_RESOLVE_FILENAME: върни __WASI_ERRNO_LOOP;
    case ERROR_NEGATIVE_SEEK:
    case ERROR_INVALID_NAME:
    case ERROR_INVALID_PARAMETER:
    case ERROR_NOT_A_REPARSE_POINT:   върни __WASI_ERRNO_INVAL;
    }
    // clang-format on
    върни __WASI_ERRNO_IO;
}

static
__wasi_timestamp_t convert_filetime (FILETIME i_time)
{
    uint64_t ticks = ((uint64_t)i_time.dwHighDateTime << 32) | i_time.dwLowDateTime;
    ако (ticks < d_m3WinEpochOffset) {
        върни 0;
    }
    върни (ticks - d_m3WinEpochOffset) * 100;
}

static
FILETIME convert_timestamp (__wasi_timestamp_t i_time)
{
    uint64_t ticks = i_time / 100 + d_m3WinEpochOffset;
    FILETIME out;
    out.dwLowDateTime = (DWORD)ticks;
    out.dwHighDateTime = (DWORD)(ticks >> 32);
    върни out;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
static
bool is_preopen (__wasi_fd_t i_fd)
{
    върни i_fd >= 3 && i_fd < d_m3WasiPreopenCount && m3_wasi_preopen[i_fd].fd == (цяло)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
static
__wasi_errno_t resolve_path (char* o_path, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    ако (!is_preopen(i_dirfd)) {
        върни __WASI_ERRNO_BADF;
    }

    цяло len = snprintf(o_path, d_m3WasiMaxHostPath, "%s/%s", m3_wasi_preopen[i_dirfd].real_path,
                       i_path);
    ако (len < 0 || len >= d_m3WasiMaxHostPath) {
        върни __WASI_ERRNO_NAMETOOLONG;
    }
    върни __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
static
__wasi_errno_t open_path_handle (ccstr_t i_path, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    ако (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_POINT;
    }

    HANDLE handle = CreateFileA(i_path, FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                OPEN_EXISTING, flags, NULL);
    ако (handle == INVALID_HANDLE_VALUE) {
        върни errno_from_win32(GetLastError());
    }

    *o_handle = handle;
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    ако (handle == INVALID_HANDLE_VALUE) {
        върни __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    върни __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
static
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, sizeof(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    ако (!GetFileInformationByHandle(i_handle, &info)) {
        switch (GetFileType(i_handle)) {
        case FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; break;
        case FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; break;
        default: върни errno_from_win32(GetLastError());
        }
        върни __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dwVolumeSerialNumber;
    o_stat->ino = ((uint64_t)info.nFileIndexHigh << 32) | info.nFileIndexLow;
    o_stat->nlink = info.nNumberOfLinks;
    o_stat->size = ((uint64_t)info.nFileSizeHigh << 32) | info.nFileSizeLow;
    o_stat->atim = convert_filetime(info.ftLastAccessTime);
    o_stat->mtim = convert_filetime(info.ftLastWriteTime);
    o_stat->ctim = convert_filetime(info.ftCreationTime);

    ако (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } иначе ако (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } иначе {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    върни __WASI_ERRNO_SUCCESS;
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
    за (цяло i = 3; i < d_m3WasiPreopenCount; i++) {
        ако (m3_wasi_preopen[i].fd < 0) {
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
    ако (_close(i_fd) != 0) {
        върни errno_from_crt(errno);
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    ако (_commit(i_fd) != 0) {
        върни errno_from_crt(errno);
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    ако (i_advice > __WASI_ADVICE_NOREUSE) {
        върни __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    върни handle_for_fd(i_fd, &handle);
}

static
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, sizeof(*o_stat));

    ако (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        върни __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    върни filestat_from_handle(handle, o_stat);
}

static
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    върни __WASI_ERRNO_SUCCESS;
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
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    върни (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

static
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    върни errno_from_crt(_chsize_s(i_fd, (__int64)i_size));
}

static
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    GetSystemTimeAsFileTime(&now);

    const FILETIME* atime = NULL;
    const FILETIME* mtime = NULL;

    ако (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } иначе ако (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    ако (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } иначе ако (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    ако (!SetFileTime(handle, NULL, atime, mtime)) {
        върни errno_from_win32(GetLastError());
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, цяло i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_offset, i_whence);
    ако (ret < 0) {
        върни errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    върни __WASI_ERRNO_SUCCESS;
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

    ако (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        ако (saved < 0 || _lseeki64(i_fd, (__int64)i_offset, SEEK_SET) < 0) {
            върни errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         done = 0;

    за (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        void*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ако (len == 0) {
            continue;
        }

        цяло ret = i_write ? _write(i_fd, addr, (unsigned)len) : _read(i_fd, addr, (unsigned)len);
        ако (ret < 0) {
            err = errno_from_crt(errno);
            break;
        }

        done += (size_t)ret;
        ако ((size_t)ret < len) {
            break;
        }
    }

    ако (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    *o_done = (__wasi_size_t)done;
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    върни transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    върни transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    върни transfer_iovs(i_fd, i_iovs, i_iovsLen, i_offset, true, false, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    върни transfer_iovs(i_fd, i_iovs, i_iovsLen, i_offset, true, true, o_done);
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

    char           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    ако (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_path_handle(full_path, true, &handle);
        ако (err != __WASI_ERRNO_SUCCESS) {
            върни err;
        }

        цяло dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        ако (dir_fd < 0) {
            CloseHandle(handle);
            върни __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        върни __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    цяло flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                _O_BINARY;

    ако ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } иначе ако ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } иначе ако ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    цяло host_fd = _open(full_path, flags, _S_IREAD | _S_IWRITE);
    ако (host_fd < 0) {
        върни errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, sizeof(*o_stat));

    char           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    HANDLE handle;
    err = open_path_handle(full_path, i_follow, &handle);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    върни err;
}

static
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    char           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    ако (!CreateDirectoryA(full_path, NULL)) {
        върни errno_from_win32(GetLastError());
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    char           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    ако (!RemoveDirectoryA(full_path)) {
        върни errno_from_win32(GetLastError());
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    char           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = GetFileAttributesA(full_path);
    bool  isDirLink =
      attrs != INVALID_FILE_ATTRIBUTES &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT);

    ако (!(isDirLink ? RemoveDirectoryA(full_path) : DeleteFileA(full_path))) {
        върни errno_from_win32(GetLastError());
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    char           full_old[d_m3WasiMaxHostPath];
    char           full_new[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_old, i_oldDirfd, i_oldPath);
    ако (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_path(full_new, i_newDirfd, i_newPath);
    }
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    ако (!MoveFileExA(full_old, full_new, MOVEFILE_REPLACE_EXISTING)) {
        върни errno_from_win32(GetLastError());
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    char           full_old[d_m3WasiMaxHostPath];
    char           full_new[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_old, i_oldDirfd, i_oldPath);
    ако (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_path(full_new, i_newDirfd, i_newPath);
    }
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    ако (!CreateHardLinkA(full_new, full_old, NULL)) {
        върни errno_from_win32(GetLastError());
    }
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    char           full_new[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_new, i_dirfd, i_path);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    char   target[d_m3WasiMaxHostPath];
    size_t dir_len = 0;
    за (size_t i = 0; full_new[i]; i++) {
        ако (full_new[i] == '/' || full_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    цяло target_len = snprintf(target, sizeof(target), "%.*s%s", (цяло)dir_len, full_new, i_target);
    ако (target_len < 0 || target_len >= (цяло)sizeof(target)) {
        върни __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = GetFileAttributesA(target);
    ако (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    ако (!CreateSymbolicLinkA(full_new, i_target,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        ако (GetLastError() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(full_new, i_target, flags)) {
            върни errno_from_win32(GetLastError());
        }
    }
    върни __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
static
__wasi_errno_t read_link_target (ccstr_t i_path, char* o_target, цяло i_size, цяло* o_len)
{
    HANDLE handle = CreateFileA(i_path, FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    ако (handle == INVALID_HANDLE_VALUE) {
        върни errno_from_win32(GetLastError());
    }

    ReparseDataBuffer* reparse =
      (ReparseDataBuffer*)m3_Malloc("ReparseDataBuffer", MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    ако (!reparse) {
        CloseHandle(handle);
        върни __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    ако (!DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &returned, NULL)) {
        err = errno_from_win32(GetLastError());
    }
    CloseHandle(handle);

    ако (err == __WASI_ERRNO_SUCCESS) {
        const WCHAR* names;
        ако (reparse->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
            names = reparse->u.symlink.PathBuffer;
        } иначе ако (reparse->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
            names = reparse->u.mount_point.PathBuffer;
        } иначе {
            names = NULL;
            err = __WASI_ERRNO_INVAL;
        }

        ако (names) {
            const WCHAR* name = names + reparse->PrintNameOffset / sizeof(WCHAR);
            цяло          len = reparse->PrintNameLength / sizeof(WCHAR);

            ако (len == 0) {
                name = names + reparse->SubstituteNameOffset / sizeof(WCHAR);
                len = reparse->SubstituteNameLength / sizeof(WCHAR);
                ако (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = WideCharToMultiByte(CP_UTF8, 0, name, len, o_target, i_size, NULL, NULL);
            ако (*o_len <= 0 && len != 0) {
                err = errno_from_win32(GetLastError());
            }
        }
    }

    m3_Free(reparse);
    върни err;
}

static
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, char* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    char           full_path[d_m3WasiMaxHostPath];
    __wasi_errno_t err = resolve_path(full_path, i_dirfd, i_path);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    char target[d_m3WasiMaxHostPath];
    цяло  target_len = 0;

    err = read_link_target(full_path, target, (цяло)sizeof(target), &target_len);
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)target_len, i_bufLen);
    memcpy(o_buf, target, copied);
    *o_bufUsed = copied;
    върни __WASI_ERRNO_SUCCESS;
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
    върни __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
static
__wasi_timestamp_t now_ns (void)
{
    FILETIME now;
    GetSystemTimeAsFileTime(&now);

    върни convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = now_ns();
    върни __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_random_get (void* o_buf, __wasi_size_t i_len)
{
    ако (!RtlGenRandom(o_buf, i_len)) {
        върни __WASI_ERRNO_IO;
    }
    върни __WASI_ERRNO_SUCCESS;
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
    ако (err != __WASI_ERRNO_SUCCESS) {
        върни err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    ако (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        върни __WASI_ERRNO_SUCCESS;
    }

    ако (type == FILE_TYPE_PIPE) {
        DWORD waiting = 0;

        ако (!PeekNamedPipe(handle, NULL, 0, NULL, &waiting, NULL)) {
            DWORD last = GetLastError();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            ако (last == ERROR_BROKEN_PIPE or last == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                върни __WASI_ERRNO_SUCCESS;
            }
            върни errno_from_win32(last);
        }

        *o_ready = (waiting != 0);
        *o_nbytes = waiting;
        върни __WASI_ERRNO_SUCCESS;
    }

    ако (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        върни __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    върни __WASI_ERRNO_SUCCESS;
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
    ако (!i_sub->isAbsolute) {
        върни (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    върни (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

static
__wasi_errno_t m3_wasi_host_poll_oneoff (const m3_wasi_pollsub_t* i_subs, __wasi_size_t i_count,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_eventCount)
{
    *o_eventCount = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    за (__wasi_size_t i = 0; i < i_count; i++) {
        ако (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_remaining_ns(&i_subs[i], 0));
            hasClock = true;
        } иначе {
            hasFd = true;
        }
    }

    ако (!hasClock and !hasFd) {
        върни __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t startNs = now_ns();

    за (;;) {
        __wasi_size_t out = 0;

        за (__wasi_size_t i = 0; i < i_count; i++) {
            const m3_wasi_pollsub_t* sub = &i_subs[i];

            ако (sub->type == __WASI_EVENTTYPE_CLOCK) {
                continue;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            ако (err == __WASI_ERRNO_SUCCESS and !ready) {
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

        ако (t < startNs) {
            startNs = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        ако (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            за (__wasi_size_t i = 0; i < i_count; i++) {
                const m3_wasi_pollsub_t* sub = &i_subs[i];

                ако (sub->type == __WASI_EVENTTYPE_CLOCK and
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

        ако (out) {
            *o_eventCount = out;
            върни __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        ако (hasClock) {
            uint64_t remainingNs = soonestNs - elapsedNs;
            uint64_t remainingMs = (remainingNs + 999999) / 1000000;

            ако (not hasFd or remainingMs < sleepMs) {
                sleepMs = (remainingMs > MAXDWORD) ? MAXDWORD : (DWORD)remainingMs;
            }
        }

        Sleep(sleepMs);
    }
}

#endif // m3_api_wasi_win32_h
