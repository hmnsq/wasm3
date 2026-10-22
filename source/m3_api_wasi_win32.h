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

#اگر_غیر_تعریف_شدہ m3_api_wasi_win32_h
#تعریف_کرو m3_api_wasi_win32_h

#شامل "m3_core.h"

#اگر !defined(_WIN32)
#  error "This file is for Windows only"
#اگر_ختم

#شامل <Windows.h>
#شامل <errno.h>
#شامل <fcntl.h>
#شامل <io.h>
#شامل <stdio.h>
#شامل <string.h>
#شامل <sys/stat.h>
#شامل <wchar.h>

// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#تعریف_کرو SystemFunction036 NTAPI SystemFunction036
#شامل <NTSecAPI.h>
#تعریف_منسوخ SystemFunction036

#شامل "m3_api_wasi_host.h"

// Room for a preopen's real path in front of the guest's
#تعریف_کرو d_m3_Wasi_Max_Host_راستہ  (d_m3_Wasi_Max_راستہ + 16)

// 1601-01-01, where a FILETIME counts its 100ns ticks from, in those same ticks
#تعریف_کرو d_m3_Win_Epoch_انحراف   116444736000000000ULL

// Only the Windows 10 SDK and later declare it
#اگر !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#اگر_ختم

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#اگر !defined(FSCTL_GET_REPARSE_نقطہ)
#  define FSCTL_GET_REPARSE_نقطہ  0x000900A8
#اگر_ختم
#اگر !defined(زیادہ_سے_زیادہ_REPARSE_ڈیٹا_بفر_سائز)
#  define زیادہ_سے_زیادہ_REPARSE_ڈیٹا_بفر_سائز  (16 * 1024)
#اگر_ختم

قسمتعریف ساخت Reparse_ڈیٹا_بفر {
    ULONG  Reparse_لیبل;
    USHORT Reparse_ڈیٹا_لمبائی;
    USHORT Reserved;
    USHORT Substitute_Name_انحراف;
    USHORT Substitute_Name_لمبائی;
    USHORT Print_Name_انحراف;
    USHORT Print_Name_لمبائی;
    اتحاد {
        ساخت {
            ULONG Flags;
            WCHAR راستہ_بفر[1];
        } symlink;
        ساخت {
            WCHAR راستہ_بفر[1];
        } mount_نقطہ;
    } u;
} Reparse_ڈیٹا_بفر;


/*
 * Plumbing
 */

// What the CRT calls used here leave in errno. Everything the Win32 API reports goes
// through errno_from_win32 instead.
ساکن
__wasi_errno_t errno_from_crt (صحیح err)
{
    // clang-format off
    انتخاب (err) {
    حالت 0:       واپس __WASI_ERRNO_SUCCESS;
    حالت EACCES:  واپس __WASI_ERRNO_ACCES;
    حالت EBADF:   واپس __WASI_ERRNO_BADF;
    حالت EEXIST:  واپس __WASI_ERRNO_EXIST;
    حالت EMFILE:  واپس __WASI_ERRNO_MFILE;
    حالت ENOENT:  واپس __WASI_ERRNO_NOENT;
    حالت ENOSPC:  واپس __WASI_ERRNO_NOSPC;
    حالت EINVAL:  واپس __WASI_ERRNO_INVAL;
    }
    // clang-format on
    واپس __WASI_ERRNO_IO;
}

ساکن
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    انتخاب (err) {
    حالت ERROR_SUCCESS:               واپس __WASI_ERRNO_SUCCESS;
    حالت ERROR_INVALID_FUNCTION:      واپس __WASI_ERRNO_NOTSUP;
    حالت ERROR_FILE_NOT_ملا:
    حالت ERROR_راستہ_NOT_ملا:
    حالت ERROR_INVALID_DRIVE:
    حالت ERROR_BAD_NETPATH:
    حالت ERROR_BAD_PATHNAME:          واپس __WASI_ERRNO_NOENT;
    حالت ERROR_TOO_MANY_OPEN_FILES:   واپس __WASI_ERRNO_MFILE;
    حالت ERROR_ACCESS_DENIED:
    حالت ERROR_WRITE_PROTECT:         واپس __WASI_ERRNO_ACCES;
    حالت ERROR_INVALID_HANDLE:        واپس __WASI_ERRNO_BADF;
    حالت ERROR_NOT_ENOUGH_حافظہ:
    حالت ERROR_OUTOFMEMORY:           واپس __WASI_ERRNO_NOMEM;
    حالت ERROR_NOT_SAME_DEVICE:       واپس __WASI_ERRNO_XDEV;
    حالت ERROR_WRITE_FAULT:
    حالت ERROR_READ_FAULT:            واپس __WASI_ERRNO_IO;
    حالت ERROR_SHARING_VIOLATION:
    حالت ERROR_تالا_VIOLATION:        واپس __WASI_ERRNO_BUSY;
    حالت ERROR_HANDLE_DISK_بھرا:
    حالت ERROR_DISK_بھرا:             واپس __WASI_ERRNO_NOSPC;
    حالت ERROR_FILE_EXISTS:
    حالت ERROR_ALREADY_EXISTS:        واپس __WASI_ERRNO_EXIST;
    حالت ERROR_بفر_حد_سے_تجاوز:
    حالت ERROR_INSUFFICIENT_بفر:
    حالت ERROR_FILENAME_EXCED_RANGE:  واپس __WASI_ERRNO_NAMETOOLONG;
    حالت ERROR_DIR_NOT_خالی_خانے:         واپس __WASI_ERRNO_NOTEMPTY;
    حالت ERROR_DIRECTORY:             واپس __WASI_ERRNO_NOTDIR;
    حالت ERROR_PRIVILEGE_NOT_HELD:    واپس __WASI_ERRNO_PERM;
    حالت ERROR_NOT_SUPPORTED:         واپس __WASI_ERRNO_NOTSUP;
    حالت ERROR_CANT_RESOLVE_FILENAME: واپس __WASI_ERRNO_LOOP;
    حالت ERROR_NEGATIVE_SEEK:
    حالت ERROR_INVALID_NAME:
    حالت ERROR_INVALID_PARAMETER:
    حالت ERROR_NOT_A_REPARSE_نقطہ:   واپس __WASI_ERRNO_INVAL;
    }
    // clang-format on
    واپس __WASI_ERRNO_IO;
}

ساکن
__wasi_timestamp_t convert_filetime (FILETIME i_وقت)
{
    uint64_t ticks = ((uint64_t)i_وقت.dw_بلند_Date_وقت << 32) | i_وقت.dw_کم_Date_وقت;
    اگر (ticks < d_m3_Win_Epoch_انحراف) {
        واپس 0;
    }
    واپس (ticks - d_m3_Win_Epoch_انحراف) * 100;
}

ساکن
FILETIME convert_timestamp (__wasi_timestamp_t i_وقت)
{
    uint64_t ticks = i_وقت / 100 + d_m3_Win_Epoch_انحراف;
    FILETIME برآمد;
    برآمد.dw_کم_Date_وقت = (DWORD)ticks;
    برآمد.dw_بلند_Date_وقت = (DWORD)(ticks >> 32);
    واپس برآمد;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
ساکن
bool is_preopen (__wasi_fd_t i_fd)
{
    واپس i_fd >= 3 && i_fd < d_m3_Wasi_Preopen_گنتی && m3_wasi_preopen[i_fd].fd == (صحیح)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
ساکن
__wasi_errno_t resolve_راستہ (حرف* o_راستہ, __wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    اگر (!is_preopen(i_dirfd)) {
        واپس __WASI_ERRNO_BADF;
    }

    صحیح len = snprintf(o_راستہ, d_m3_Wasi_Max_Host_راستہ, "%s/%s", m3_wasi_preopen[i_dirfd].real_راستہ,
                       i_راستہ);
    اگر (len < 0 || len >= d_m3_Wasi_Max_Host_راستہ) {
        واپس __WASI_ERRNO_NAMETOOLONG;
    }
    واپس __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
ساکن
__wasi_errno_t open_راستہ_handle (ccstr_t i_راستہ, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    اگر (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_نقطہ;
    }

    HANDLE handle = CreateFileA(i_راستہ, FILE_READ_اوصاف,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, تہی,
                                OPEN_EXISTING, flags, تہی);
    اگر (handle == INVALID_HANDLE_قدر) {
        واپس errno_from_win32(Get_آخری_Error());
    }

    *o_handle = handle;
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    اگر (handle == INVALID_HANDLE_قدر) {
        واپس __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    واپس __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
ساکن
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    اگر (!GetFileInformationByHandle(i_handle, &info)) {
        انتخاب (GetFileType(i_handle)) {
        حالت FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; توڑو;
        حالت FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; توڑو;
        طےشدہ: واپس errno_from_win32(Get_آخری_Error());
        }
        واپس __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dw_Volume_Serial_نمبر;
    o_stat->ino = ((uint64_t)info.n_File_اشاریہ_بلند << 32) | info.n_File_اشاریہ_کم;
    o_stat->nlink = info.n_نمبر_کا_Links;
    o_stat->سائز = ((uint64_t)info.n_File_سائز_بلند << 32) | info.n_File_سائز_کم;
    o_stat->atim = convert_filetime(info.ft_آخری_Access_وقت);
    o_stat->mtim = convert_filetime(info.ft_آخری_Write_وقت);
    o_stat->ctim = convert_filetime(info.ft_Creation_وقت);

    اگر (info.dw_File_اوصاف & FILE_ATTRIBUTE_REPARSE_نقطہ) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } ورنہ اگر (info.dw_File_اوصاف & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } ورنہ {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    واپس __WASI_ERRNO_SUCCESS;
}


/*
 * Setup
 */

ساکن
خالی m3_wasi_host_init ()
{
    _setmode(_fileno(stdin), O_ثنائی);
    _setmode(_fileno(stdout), O_ثنائی);
    _setmode(_fileno(stderr), O_ثنائی);

    // Windows cannot open a directory as a descriptor, so a preopen is resolved by
    // path instead. It still has to hold a descriptor number: the guest is handed
    // one, and without something occupying it the next file opened would be given
    // the same number and the two would be indistinguishable.
    برائے (صحیح i = 3; i < d_m3_Wasi_Preopen_گنتی; i++) {
        اگر (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = _open("NUL", _O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ساکن
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    اگر (_close(i_fd) != 0) {
        واپس errno_from_crt(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    اگر (_commit(i_fd) != 0) {
        واپس errno_from_crt(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_انحراف,
                                       __wasi_filesize_t i_لمبائی, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    اگر (i_advice > __WASI_ADVICE_NOREUSE) {
        واپس __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    واپس handle_for_fd(i_fd, &handle);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    اگر (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        واپس __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    واپس filestat_from_handle(handle, o_stat);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    واپس __WASI_ERRNO_SUCCESS;
}

// Nothing here can be changed after the fact: append and non-blocking are fixed when
// CreateFile runs, and the CRT hands back no way to revisit either. fd_fdstat above
// reports no flags at all for the same reason, so the only request that can be
// honoured is the one asking for exactly that.
ساکن
__wasi_errno_t m3_wasi_host_fd_fdstat_سیٹ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    واپس (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_سیٹ_سائز (__wasi_fd_t i_fd, __wasi_filesize_t i_سائز)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    واپس errno_from_crt(_chsize_s(i_fd, (__int64)i_سائز));
}

ساکن
__wasi_errno_t m3_wasi_host_fd_سیٹ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    Get_System_وقت_As_File_وقت(&now);

    ثابت FILETIME* atime = تہی;
    ثابت FILETIME* mtime = تہی;

    اگر (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } ورنہ اگر (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    اگر (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } ورنہ اگر (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    اگر (!سیٹ_File_وقت(handle, تہی, atime, mtime)) {
        واپس errno_from_win32(Get_آخری_Error());
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_انحراف, صحیح i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_انحراف, i_whence);
    اگر (ret < 0) {
        واپس errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    واپس __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

// Windows has neither a vectored nor a positional transfer, so the list is walked
// one buffer at a time and a short transfer ends it, as it would for readv. A
// positional one moves the file pointer and puts it back where it was.
ساکن
__wasi_errno_t transfer_iovs (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                              __wasi_size_t i_iovsLen, __wasi_filesize_t i_انحراف,
                              bool i_positional, bool i_write, __wasi_size_t* o_مکمل)
{
    *o_مکمل = 0;

    int64_t saved = 0;

    اگر (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        اگر (saved < 0 || _lseeki64(i_fd, (__int64)i_انحراف, SEEK_SET) < 0) {
            واپس errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         مکمل = 0;

    برائے (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        خالی*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        اگر (len == 0) {
            جاری;
        }

        صحیح ret = i_write ? _write(i_fd, addr, (غیرعلامتی)len) : _read(i_fd, addr, (غیرعلامتی)len);
        اگر (ret < 0) {
            err = errno_from_crt(errno);
            توڑو;
        }

        مکمل += (size_t)ret;
        اگر ((size_t)ret < len) {
            توڑو;
        }
    }

    اگر (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    *o_مکمل = (__wasi_size_t)مکمل;
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_مکمل)
{
    واپس transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_مکمل);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_مکمل)
{
    واپس transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_مکمل);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_انحراف,
                                      __wasi_size_t* o_مکمل)
{
    واپس transfer_iovs(i_fd, i_iovs, i_iovsLen, i_انحراف, true, false, o_مکمل);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_انحراف,
                                       __wasi_size_t* o_مکمل)
{
    واپس transfer_iovs(i_fd, i_iovs, i_iovsLen, i_انحراف, true, true, o_مکمل);
}


/*
 * Paths
 */

ساکن
__wasi_errno_t m3_wasi_host_راستہ_open (__wasi_fd_t i_dirfd, ccstr_t i_راستہ,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    حرف           بھرا_راستہ[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_راستہ, i_dirfd, i_راستہ);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    اگر (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_راستہ_handle(بھرا_راستہ, true, &handle);
        اگر (err != __WASI_ERRNO_SUCCESS) {
            واپس err;
        }

        صحیح dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        اگر (dir_fd < 0) {
            CloseHandle(handle);
            واپس __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        واپس __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    صحیح flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                O_ثنائی;

    اگر ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } ورنہ اگر ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } ورنہ اگر ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    صحیح host_fd = _open(بھرا_راستہ, flags, _S_IREAD | _S_IWRITE);
    اگر (host_fd < 0) {
        واپس errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_راستہ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    حرف           بھرا_راستہ[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_راستہ, i_dirfd, i_راستہ);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    HANDLE handle;
    err = open_راستہ_handle(بھرا_راستہ, i_follow, &handle);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    واپس err;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    حرف           بھرا_راستہ[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_راستہ, i_dirfd, i_راستہ);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    اگر (!CreateDirectoryA(بھرا_راستہ, تہی)) {
        واپس errno_from_win32(Get_آخری_Error());
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    حرف           بھرا_راستہ[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_راستہ, i_dirfd, i_راستہ);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    اگر (!RemoveDirectoryA(بھرا_راستہ)) {
        واپس errno_from_win32(Get_آخری_Error());
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    حرف           بھرا_راستہ[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_راستہ, i_dirfd, i_راستہ);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = Get_File_اوصاف_A(بھرا_راستہ);
    bool  isDirLink =
      attrs != INVALID_FILE_اوصاف &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_نقطہ)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_نقطہ);

    اگر (!(isDirLink ? RemoveDirectoryA(بھرا_راستہ) : DeleteFileA(بھرا_راستہ))) {
        واپس errno_from_win32(Get_آخری_Error());
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_راستہ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_راستہ)
{
    حرف           بھرا_old[d_m3_Wasi_Max_Host_راستہ];
    حرف           بھرا_new[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_old, i_oldDirfd, i_old_راستہ);
    اگر (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_راستہ(بھرا_new, i_newDirfd, i_new_راستہ);
    }
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    اگر (!MoveFileExA(بھرا_old, بھرا_new, MOVEFILE_REPLACE_EXISTING)) {
        واپس errno_from_win32(Get_آخری_Error());
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_راستہ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_راستہ)
{
    حرف           بھرا_old[d_m3_Wasi_Max_Host_راستہ];
    حرف           بھرا_new[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_old, i_oldDirfd, i_old_راستہ);
    اگر (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_راستہ(بھرا_new, i_newDirfd, i_new_راستہ);
    }
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    اگر (!CreateHardLinkA(بھرا_new, بھرا_old, تہی)) {
        واپس errno_from_win32(Get_آخری_Error());
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_symlink (ccstr_t i_ہدف, __wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    حرف           بھرا_new[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_new, i_dirfd, i_راستہ);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    حرف   ہدف[d_m3_Wasi_Max_Host_راستہ];
    size_t dir_len = 0;
    برائے (size_t i = 0; بھرا_new[i]; i++) {
        اگر (بھرا_new[i] == '/' || بھرا_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    صحیح ہدف_len = snprintf(ہدف, حجم(ہدف), "%.*s%s", (صحیح)dir_len, بھرا_new, i_ہدف);
    اگر (ہدف_len < 0 || ہدف_len >= (صحیح)حجم(ہدف)) {
        واپس __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = Get_File_اوصاف_A(ہدف);
    اگر (attrs != INVALID_FILE_اوصاف && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    اگر (!CreateSymbolicLinkA(بھرا_new, i_ہدف,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        اگر (Get_آخری_Error() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(بھرا_new, i_ہدف, flags)) {
            واپس errno_from_win32(Get_آخری_Error());
        }
    }
    واپس __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
ساکن
__wasi_errno_t read_link_ہدف (ccstr_t i_راستہ, حرف* o_ہدف, صحیح i_سائز, صحیح* o_len)
{
    HANDLE handle = CreateFileA(i_راستہ, FILE_READ_اوصاف,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, تہی,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_نقطہ, تہی);
    اگر (handle == INVALID_HANDLE_قدر) {
        واپس errno_from_win32(Get_آخری_Error());
    }

    Reparse_ڈیٹا_بفر* reparse =
      (Reparse_ڈیٹا_بفر*)m3_Malloc("ReparseDataBuffer", زیادہ_سے_زیادہ_REPARSE_ڈیٹا_بفر_سائز);
    اگر (!reparse) {
        CloseHandle(handle);
        واپس __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    اگر (!DeviceIoControl(handle, FSCTL_GET_REPARSE_نقطہ, تہی, 0, reparse,
                         زیادہ_سے_زیادہ_REPARSE_ڈیٹا_بفر_سائز, &returned, تہی)) {
        err = errno_from_win32(Get_آخری_Error());
    }
    CloseHandle(handle);

    اگر (err == __WASI_ERRNO_SUCCESS) {
        ثابت WCHAR* نام;
        اگر (reparse->Reparse_لیبل == IO_REPARSE_لیبل_SYMLINK) {
            نام = reparse->u.symlink.راستہ_بفر;
        } ورنہ اگر (reparse->Reparse_لیبل == IO_REPARSE_لیبل_MOUNT_نقطہ) {
            نام = reparse->u.mount_نقطہ.راستہ_بفر;
        } ورنہ {
            نام = تہی;
            err = __WASI_ERRNO_INVAL;
        }

        اگر (نام) {
            ثابت WCHAR* name = نام + reparse->Print_Name_انحراف / حجم(WCHAR);
            صحیح          len = reparse->Print_Name_لمبائی / حجم(WCHAR);

            اگر (len == 0) {
                name = نام + reparse->Substitute_Name_انحراف / حجم(WCHAR);
                len = reparse->Substitute_Name_لمبائی / حجم(WCHAR);
                اگر (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = Wide_Char_مقصد_Multi_Byte(CP_UTF8, 0, name, len, o_ہدف, i_سائز, تہی, تہی);
            اگر (*o_len <= 0 && len != 0) {
                err = errno_from_win32(Get_آخری_Error());
            }
        }
    }

    m3_Free(reparse);
    واپس err;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_راستہ, حرف* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_مستعمل)
{
    *o_buf_مستعمل = 0;

    حرف           بھرا_راستہ[d_m3_Wasi_Max_Host_راستہ];
    __wasi_errno_t err = resolve_راستہ(بھرا_راستہ, i_dirfd, i_راستہ);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    حرف ہدف[d_m3_Wasi_Max_Host_راستہ];
    صحیح  ہدف_len = 0;

    err = read_link_ہدف(بھرا_راستہ, ہدف, (صحیح)حجم(ہدف), &ہدف_len);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)ہدف_len, i_bufLen);
    حافظہ_نقل(o_buf, ہدف, copied);
    *o_buf_مستعمل = copied;
    واپس __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

// Windows offers the one clock, so every WASI clock maps onto it and none of them is
// rejected
ساکن
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    // a FILETIME counts in 100ns, but the system clock behind it moves on the timer
    // tick, so this says a millisecond rather than what it can spell
    *o_res = 1000000;
    واپس __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
ساکن
__wasi_timestamp_t now_ns (خالی)
{
    FILETIME now;
    Get_System_وقت_As_File_وقت(&now);

    واپس convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_وقت_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_وقت)
{
    *o_وقت = now_ns();
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_random_get (خالی* o_buf, __wasi_size_t i_len)
{
    اگر (!RtlGenRandom(o_buf, i_len)) {
        واپس __WASI_ERRNO_IO;
    }
    واپس __WASI_ERRNO_SUCCESS;
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
#تعریف_کرو d_m3WasiPollIntervalMs  5

// Whether a descriptor has what its subscription is waiting for. Everything that is
// not a stream is ready by definition: the spec says a regular file always triggers
// both read and write, and so does anything whose kind Windows will not describe.
ساکن
__wasi_errno_t poll_fd_ready (ثابت m3_wasi_pollsub_t* i_sub, bool* o_ready,
                              __wasi_filesize_t* o_nbytes, __wasi_eventrwflags_t* o_flags)
{
    *o_ready = false;
    *o_nbytes = 0;
    *o_flags = 0;

    HANDLE         handle;

    __wasi_errno_t err = handle_for_fd(i_sub->fd, &handle);
    اگر (err != __WASI_ERRNO_SUCCESS) {
        واپس err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    اگر (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        واپس __WASI_ERRNO_SUCCESS;
    }

    اگر (type == FILE_TYPE_PIPE) {
        DWORD انتظار = 0;

        اگر (!PeekNamedPipe(handle, تہی, 0, تہی, &انتظار, تہی)) {
            DWORD آخری = Get_آخری_Error();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            اگر (آخری == ERROR_BROKEN_PIPE or آخری == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                واپس __WASI_ERRNO_SUCCESS;
            }
            واپس errno_from_win32(آخری);
        }

        *o_ready = (انتظار != 0);
        *o_nbytes = انتظار;
        واپس __WASI_ERRNO_SUCCESS;
    }

    اگر (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        واپس __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    واپس __WASI_ERRNO_SUCCESS;
}

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock and ignores i_elapsedNs, the clock having moved on by itself.
// Every WASI clock is the system clock here - see clock_time_get - so an absolute
// deadline is held against that one whichever clock it names.
ساکن
uint64_t poll_بقایا_ns (ثابت m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    اگر (!i_sub->isAbsolute) {
        واپس (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    واپس (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

ساکن
__wasi_errno_t m3_wasi_host_poll_oneoff (ثابت m3_wasi_pollsub_t* i_subs, __wasi_size_t i_گنتی,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_گنتی)
{
    *o_event_گنتی = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    برائے (__wasi_size_t i = 0; i < i_گنتی; i++) {
        اگر (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_بقایا_ns(&i_subs[i], 0));
            hasClock = true;
        } ورنہ {
            hasFd = true;
        }
    }

    اگر (!hasClock and !hasFd) {
        واپس __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t آغاز_Ns = now_ns();

    برائے (;;) {
        __wasi_size_t برآمد = 0;

        برائے (__wasi_size_t i = 0; i < i_گنتی; i++) {
            ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

            اگر (sub->type == __WASI_EVENTTYPE_CLOCK) {
                جاری;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            اگر (err == __WASI_ERRNO_SUCCESS and !ready) {
                جاری;
            }

            o_events[برآمد].userdata = sub->userdata;
            o_events[برآمد].error = err;
            o_events[برآمد].type = sub->type;
            o_events[برآمد].nbytes = nbytes;
            o_events[برآمد].flags = flags;
            برآمد++;
        }

        // This is a wall clock and can be set backwards under us, which is not
        // elapsed time going negative but the measurement being no longer usable;
        // starting the count again from here is the honest reading of it.
        ثابت uint64_t t = now_ns();
        ثابت uint64_t elapsedNs = (t > آغاز_Ns) ? t - آغاز_Ns : 0;

        اگر (t < آغاز_Ns) {
            آغاز_Ns = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        اگر (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            برائے (__wasi_size_t i = 0; i < i_گنتی; i++) {
                ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

                اگر (sub->type == __WASI_EVENTTYPE_CLOCK and
                    poll_بقایا_ns(sub, elapsedNs) == 0) {
                    o_events[برآمد].userdata = sub->userdata;
                    o_events[برآمد].error = __WASI_ERRNO_SUCCESS;
                    o_events[برآمد].type = __WASI_EVENTTYPE_CLOCK;
                    o_events[برآمد].nbytes = 0;
                    o_events[برآمد].flags = 0;
                    برآمد++;
                }
            }
        }

        اگر (برآمد) {
            *o_event_گنتی = برآمد;
            واپس __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        اگر (hasClock) {
            uint64_t بقایا_Ns = soonestNs - elapsedNs;
            uint64_t بقایا_Ms = (بقایا_Ns + 999999) / 1000000;

            اگر (not hasFd or بقایا_Ms < sleepMs) {
                sleepMs = (بقایا_Ms > MAXDWORD) ? MAXDWORD : (DWORD)بقایا_Ms;
            }
        }

        Sleep(sleepMs);
    }
}

#اگر_ختم // m3_api_wasi_win32_h
