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

#جے_بنا_تعریف m3_api_wasi_win32_h
#تعریف_کرو m3_api_wasi_win32_h

#ضمن "m3_core.h"

#إذا !defined(_WIN32)
#  error "This file is for Windows only"
#جے_انت

#ضمن <Windows.h>
#ضمن <errno.h>
#ضمن <fcntl.h>
#ضمن <io.h>
#ضمن <stdio.h>
#ضمن <string.h>
#ضمن <sys/stat.h>
#ضمن <wchar.h>

// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#تعریف_کرو SystemFunction036 NTAPI SystemFunction036
#ضمن <NTSecAPI.h>
#تعریف_رد SystemFunction036

#ضمن "m3_api_wasi_host.h"

// Room for a preopen's real path in front of the guest's
#تعریف_کرو d_m3_Wasi_Max_Host_رستہ  (d_m3_Wasi_Max_رستہ + 16)

// 1601-01-01, where a FILETIME counts its 100ns ticks from, in those same ticks
#تعریف_کرو d_m3_Win_Epoch_وتھ   116444736000000000ULL

// Only the Windows 10 SDK and later declare it
#إذا !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#جے_انت

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#إذا !defined(FSCTL_GET_REPARSE_نقطہ)
#  define FSCTL_GET_REPARSE_نقطہ  0x000900A8
#جے_انت
#إذا !defined(ودھ_توں_ودھ_REPARSE_انکڑے_بفر_ناپ)
#  define ودھ_توں_ودھ_REPARSE_انکڑے_بفر_ناپ  (16 * 1024)
#جے_انت

تعريفنوع بنية Reparse_انکڑے_بفر {
    ULONG  Reparse_ٹیگ;
    USHORT Reparse_انکڑے_لمائی;
    USHORT Reserved;
    USHORT Substitute_Name_وتھ;
    USHORT Substitute_Name_لمائی;
    USHORT Print_Name_وتھ;
    USHORT Print_Name_لمائی;
    اتحاد {
        بنية {
            ULONG Flags;
            WCHAR رستہ_بفر[1];
        } symlink;
        بنية {
            WCHAR رستہ_بفر[1];
        } mount_نقطہ;
    } u;
} Reparse_انکڑے_بفر;


/*
 * Plumbing
 */

// What the CRT calls used here leave in errno. Everything the Win32 API reports goes
// through errno_from_win32 instead.
ساكن
__wasi_errno_t errno_from_crt (صحيح err)
{
    // clang-format off
    بدل (err) {
    حالة 0:       أرجع __WASI_ERRNO_SUCCESS;
    حالة EACCES:  أرجع __WASI_ERRNO_ACCES;
    حالة EBADF:   أرجع __WASI_ERRNO_BADF;
    حالة EEXIST:  أرجع __WASI_ERRNO_EXIST;
    حالة EMFILE:  أرجع __WASI_ERRNO_MFILE;
    حالة ENOENT:  أرجع __WASI_ERRNO_NOENT;
    حالة ENOSPC:  أرجع __WASI_ERRNO_NOSPC;
    حالة EINVAL:  أرجع __WASI_ERRNO_INVAL;
    }
    // clang-format on
    أرجع __WASI_ERRNO_IO;
}

ساكن
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    بدل (err) {
    حالة ERROR_SUCCESS:               أرجع __WASI_ERRNO_SUCCESS;
    حالة ERROR_INVALID_FUNCTION:      أرجع __WASI_ERRNO_NOTSUP;
    حالة ERROR_FILE_NOT_لبھیا:
    حالة ERROR_رستہ_NOT_لبھیا:
    حالة ERROR_INVALID_DRIVE:
    حالة ERROR_BAD_NETPATH:
    حالة ERROR_BAD_PATHNAME:          أرجع __WASI_ERRNO_NOENT;
    حالة ERROR_TOO_MANY_OPEN_FILES:   أرجع __WASI_ERRNO_MFILE;
    حالة ERROR_ACCESS_DENIED:
    حالة ERROR_WRITE_PROTECT:         أرجع __WASI_ERRNO_ACCES;
    حالة ERROR_INVALID_HANDLE:        أرجع __WASI_ERRNO_BADF;
    حالة ERROR_NOT_ENOUGH_حافظہ:
    حالة ERROR_OUTOFMEMORY:           أرجع __WASI_ERRNO_NOMEM;
    حالة ERROR_NOT_SAME_DEVICE:       أرجع __WASI_ERRNO_XDEV;
    حالة ERROR_WRITE_FAULT:
    حالة ERROR_READ_FAULT:            أرجع __WASI_ERRNO_IO;
    حالة ERROR_SHARING_VIOLATION:
    حالة ERROR_جندرا_VIOLATION:        أرجع __WASI_ERRNO_BUSY;
    حالة ERROR_HANDLE_DISK_بھریا:
    حالة ERROR_DISK_بھریا:             أرجع __WASI_ERRNO_NOSPC;
    حالة ERROR_FILE_EXISTS:
    حالة ERROR_ALREADY_EXISTS:        أرجع __WASI_ERRNO_EXIST;
    حالة ERROR_بفر_حد_توں_ودھ:
    حالة ERROR_INSUFFICIENT_بفر:
    حالة ERROR_FILENAME_EXCED_RANGE:  أرجع __WASI_ERRNO_NAMETOOLONG;
    حالة ERROR_DIR_NOT_خالی_تھاواں:         أرجع __WASI_ERRNO_NOTEMPTY;
    حالة ERROR_DIRECTORY:             أرجع __WASI_ERRNO_NOTDIR;
    حالة ERROR_PRIVILEGE_NOT_HELD:    أرجع __WASI_ERRNO_PERM;
    حالة ERROR_NOT_SUPPORTED:         أرجع __WASI_ERRNO_NOTSUP;
    حالة ERROR_CANT_RESOLVE_FILENAME: أرجع __WASI_ERRNO_LOOP;
    حالة ERROR_NEGATIVE_SEEK:
    حالة ERROR_INVALID_NAME:
    حالة ERROR_INVALID_PARAMETER:
    حالة ERROR_NOT_A_REPARSE_نقطہ:   أرجع __WASI_ERRNO_INVAL;
    }
    // clang-format on
    أرجع __WASI_ERRNO_IO;
}

ساكن
__wasi_timestamp_t convert_filetime (FILETIME i_ویلا)
{
    uint64_t ticks = ((uint64_t)i_ویلا.dw_بلند_Date_ویلا << 32) | i_ویلا.dw_کم_Date_ویلا;
    إذا (ticks < d_m3_Win_Epoch_وتھ) {
        أرجع 0;
    }
    أرجع (ticks - d_m3_Win_Epoch_وتھ) * 100;
}

ساكن
FILETIME convert_timestamp (__wasi_timestamp_t i_ویلا)
{
    uint64_t ticks = i_ویلا / 100 + d_m3_Win_Epoch_وتھ;
    FILETIME باہرلا;
    باہرلا.dw_کم_Date_ویلا = (DWORD)ticks;
    باہرلا.dw_بلند_Date_ویلا = (DWORD)(ticks >> 32);
    أرجع باہرلا;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
ساكن
bool is_preopen (__wasi_fd_t i_fd)
{
    أرجع i_fd >= 3 && i_fd < d_m3_Wasi_Preopen_گنتی && m3_wasi_preopen[i_fd].fd == (صحيح)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
ساكن
__wasi_errno_t resolve_رستہ (حرف* o_رستہ, __wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    إذا (!is_preopen(i_dirfd)) {
        أرجع __WASI_ERRNO_BADF;
    }

    صحيح len = snprintf(o_رستہ, d_m3_Wasi_Max_Host_رستہ, "%s/%s", m3_wasi_preopen[i_dirfd].real_رستہ,
                       i_رستہ);
    إذا (len < 0 || len >= d_m3_Wasi_Max_Host_رستہ) {
        أرجع __WASI_ERRNO_NAMETOOLONG;
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
ساكن
__wasi_errno_t open_رستہ_handle (ccstr_t i_رستہ, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    إذا (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_نقطہ;
    }

    HANDLE handle = CreateFileA(i_رستہ, FILE_READ_گن,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, عدم,
                                OPEN_EXISTING, flags, عدم);
    إذا (handle == INVALID_HANDLE_قدر) {
        أرجع errno_from_win32(Get_آخری_Error());
    }

    *o_handle = handle;
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    إذا (handle == INVALID_HANDLE_قدر) {
        أرجع __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    أرجع __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
ساكن
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    إذا (!GetFileInformationByHandle(i_handle, &info)) {
        بدل (GetFileType(i_handle)) {
        حالة FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; اكسر;
        حالة FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; اكسر;
        افتراضي: أرجع errno_from_win32(Get_آخری_Error());
        }
        أرجع __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dw_Volume_Serial_نمبر;
    o_stat->ino = ((uint64_t)info.n_File_اشاریہ_بلند << 32) | info.n_File_اشاریہ_کم;
    o_stat->nlink = info.n_نمبر_دا_Links;
    o_stat->ناپ = ((uint64_t)info.n_File_ناپ_بلند << 32) | info.n_File_ناپ_کم;
    o_stat->atim = convert_filetime(info.ft_آخری_Access_ویلا);
    o_stat->mtim = convert_filetime(info.ft_آخری_Write_ویلا);
    o_stat->ctim = convert_filetime(info.ft_Creation_ویلا);

    إذا (info.dw_File_گن & FILE_ATTRIBUTE_REPARSE_نقطہ) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } وإلا إذا (info.dw_File_گن & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } وإلا {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    أرجع __WASI_ERRNO_SUCCESS;
}


/*
 * Setup
 */

ساكن
فراغ m3_wasi_host_init ()
{
    _setmode(_fileno(stdin), O_ثنائی);
    _setmode(_fileno(stdout), O_ثنائی);
    _setmode(_fileno(stderr), O_ثنائی);

    // Windows cannot open a directory as a descriptor, so a preopen is resolved by
    // path instead. It still has to hold a descriptor number: the guest is handed
    // one, and without something occupying it the next file opened would be given
    // the same number and the two would be indistinguishable.
    لكل (صحيح i = 3; i < d_m3_Wasi_Preopen_گنتی; i++) {
        إذا (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = _open("NUL", _O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ساكن
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    إذا (_close(i_fd) != 0) {
        أرجع errno_from_crt(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    إذا (_commit(i_fd) != 0) {
        أرجع errno_from_crt(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_وتھ,
                                       __wasi_filesize_t i_لمائی, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    إذا (i_advice > __WASI_ADVICE_NOREUSE) {
        أرجع __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    أرجع handle_for_fd(i_fd, &handle);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    إذا (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        أرجع __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    أرجع filestat_from_handle(handle, o_stat);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    أرجع __WASI_ERRNO_SUCCESS;
}

// Nothing here can be changed after the fact: append and non-blocking are fixed when
// CreateFile runs, and the CRT hands back no way to revisit either. fd_fdstat above
// reports no flags at all for the same reason, so the only request that can be
// honoured is the one asking for exactly that.
ساكن
__wasi_errno_t m3_wasi_host_fd_fdstat_سیٹ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    أرجع (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_سیٹ_ناپ (__wasi_fd_t i_fd, __wasi_filesize_t i_ناپ)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    أرجع errno_from_crt(_chsize_s(i_fd, (__int64)i_ناپ));
}

ساكن
__wasi_errno_t m3_wasi_host_fd_سیٹ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    Get_System_ویلا_As_File_ویلا(&now);

    ثابت FILETIME* atime = عدم;
    ثابت FILETIME* mtime = عدم;

    إذا (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } وإلا إذا (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    إذا (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } وإلا إذا (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    إذا (!سیٹ_File_ویلا(handle, عدم, atime, mtime)) {
        أرجع errno_from_win32(Get_آخری_Error());
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_وتھ, صحيح i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_وتھ, i_whence);
    إذا (ret < 0) {
        أرجع errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    أرجع __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

// Windows has neither a vectored nor a positional transfer, so the list is walked
// one buffer at a time and a short transfer ends it, as it would for readv. A
// positional one moves the file pointer and puts it back where it was.
ساكن
__wasi_errno_t transfer_iovs (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                              __wasi_size_t i_iovsLen, __wasi_filesize_t i_وتھ,
                              bool i_positional, bool i_write, __wasi_size_t* o_پورا)
{
    *o_پورا = 0;

    int64_t saved = 0;

    إذا (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        إذا (saved < 0 || _lseeki64(i_fd, (__int64)i_وتھ, SEEK_SET) < 0) {
            أرجع errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         پورا = 0;

    لكل (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        فراغ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        إذا (len == 0) {
            استمر;
        }

        صحيح ret = i_write ? _write(i_fd, addr, (بنانشانی)len) : _read(i_fd, addr, (بنانشانی)len);
        إذا (ret < 0) {
            err = errno_from_crt(errno);
            اكسر;
        }

        پورا += (size_t)ret;
        إذا ((size_t)ret < len) {
            اكسر;
        }
    }

    إذا (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    *o_پورا = (__wasi_size_t)پورا;
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_پورا)
{
    أرجع transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_پورا);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_پورا)
{
    أرجع transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_پورا);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_وتھ,
                                      __wasi_size_t* o_پورا)
{
    أرجع transfer_iovs(i_fd, i_iovs, i_iovsLen, i_وتھ, true, false, o_پورا);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_وتھ,
                                       __wasi_size_t* o_پورا)
{
    أرجع transfer_iovs(i_fd, i_iovs, i_iovsLen, i_وتھ, true, true, o_پورا);
}


/*
 * Paths
 */

ساكن
__wasi_errno_t m3_wasi_host_رستہ_open (__wasi_fd_t i_dirfd, ccstr_t i_رستہ,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    حرف           بھریا_رستہ[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_رستہ, i_dirfd, i_رستہ);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    إذا (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_رستہ_handle(بھریا_رستہ, true, &handle);
        إذا (err != __WASI_ERRNO_SUCCESS) {
            أرجع err;
        }

        صحيح dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        إذا (dir_fd < 0) {
            CloseHandle(handle);
            أرجع __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        أرجع __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    صحيح flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                O_ثنائی;

    إذا ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } وإلا إذا ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } وإلا إذا ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    صحيح host_fd = _open(بھریا_رستہ, flags, _S_IREAD | _S_IWRITE);
    إذا (host_fd < 0) {
        أرجع errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_رستہ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    حرف           بھریا_رستہ[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_رستہ, i_dirfd, i_رستہ);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    HANDLE handle;
    err = open_رستہ_handle(بھریا_رستہ, i_follow, &handle);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    أرجع err;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    حرف           بھریا_رستہ[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_رستہ, i_dirfd, i_رستہ);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    إذا (!CreateDirectoryA(بھریا_رستہ, عدم)) {
        أرجع errno_from_win32(Get_آخری_Error());
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    حرف           بھریا_رستہ[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_رستہ, i_dirfd, i_رستہ);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    إذا (!RemoveDirectoryA(بھریا_رستہ)) {
        أرجع errno_from_win32(Get_آخری_Error());
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    حرف           بھریا_رستہ[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_رستہ, i_dirfd, i_رستہ);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = Get_File_گن_A(بھریا_رستہ);
    bool  isDirLink =
      attrs != INVALID_FILE_گن &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_نقطہ)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_نقطہ);

    إذا (!(isDirLink ? RemoveDirectoryA(بھریا_رستہ) : DeleteFileA(بھریا_رستہ))) {
        أرجع errno_from_win32(Get_آخری_Error());
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_رستہ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_رستہ)
{
    حرف           بھریا_old[d_m3_Wasi_Max_Host_رستہ];
    حرف           بھریا_new[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_old, i_oldDirfd, i_old_رستہ);
    إذا (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_رستہ(بھریا_new, i_newDirfd, i_new_رستہ);
    }
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    إذا (!MoveFileExA(بھریا_old, بھریا_new, MOVEFILE_REPLACE_EXISTING)) {
        أرجع errno_from_win32(Get_آخری_Error());
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_رستہ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_رستہ)
{
    حرف           بھریا_old[d_m3_Wasi_Max_Host_رستہ];
    حرف           بھریا_new[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_old, i_oldDirfd, i_old_رستہ);
    إذا (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_رستہ(بھریا_new, i_newDirfd, i_new_رستہ);
    }
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    إذا (!CreateHardLinkA(بھریا_new, بھریا_old, عدم)) {
        أرجع errno_from_win32(Get_آخری_Error());
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_symlink (ccstr_t i_نشانہ, __wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    حرف           بھریا_new[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_new, i_dirfd, i_رستہ);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    حرف   نشانہ[d_m3_Wasi_Max_Host_رستہ];
    size_t dir_len = 0;
    لكل (size_t i = 0; بھریا_new[i]; i++) {
        إذا (بھریا_new[i] == '/' || بھریا_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    صحيح نشانہ_len = snprintf(نشانہ, حجم(نشانہ), "%.*s%s", (صحيح)dir_len, بھریا_new, i_نشانہ);
    إذا (نشانہ_len < 0 || نشانہ_len >= (صحيح)حجم(نشانہ)) {
        أرجع __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = Get_File_گن_A(نشانہ);
    إذا (attrs != INVALID_FILE_گن && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    إذا (!CreateSymbolicLinkA(بھریا_new, i_نشانہ,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        إذا (Get_آخری_Error() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(بھریا_new, i_نشانہ, flags)) {
            أرجع errno_from_win32(Get_آخری_Error());
        }
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
ساكن
__wasi_errno_t read_link_نشانہ (ccstr_t i_رستہ, حرف* o_نشانہ, صحيح i_ناپ, صحيح* o_len)
{
    HANDLE handle = CreateFileA(i_رستہ, FILE_READ_گن,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, عدم,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_نقطہ, عدم);
    إذا (handle == INVALID_HANDLE_قدر) {
        أرجع errno_from_win32(Get_آخری_Error());
    }

    Reparse_انکڑے_بفر* reparse =
      (Reparse_انکڑے_بفر*)m3_Malloc("ReparseDataBuffer", ودھ_توں_ودھ_REPARSE_انکڑے_بفر_ناپ);
    إذا (!reparse) {
        CloseHandle(handle);
        أرجع __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    إذا (!DeviceIoControl(handle, FSCTL_GET_REPARSE_نقطہ, عدم, 0, reparse,
                         ودھ_توں_ودھ_REPARSE_انکڑے_بفر_ناپ, &returned, عدم)) {
        err = errno_from_win32(Get_آخری_Error());
    }
    CloseHandle(handle);

    إذا (err == __WASI_ERRNO_SUCCESS) {
        ثابت WCHAR* ناں;
        إذا (reparse->Reparse_ٹیگ == IO_REPARSE_ٹیگ_SYMLINK) {
            ناں = reparse->u.symlink.رستہ_بفر;
        } وإلا إذا (reparse->Reparse_ٹیگ == IO_REPARSE_ٹیگ_MOUNT_نقطہ) {
            ناں = reparse->u.mount_نقطہ.رستہ_بفر;
        } وإلا {
            ناں = عدم;
            err = __WASI_ERRNO_INVAL;
        }

        إذا (ناں) {
            ثابت WCHAR* name = ناں + reparse->Print_Name_وتھ / حجم(WCHAR);
            صحيح          len = reparse->Print_Name_لمائی / حجم(WCHAR);

            إذا (len == 0) {
                name = ناں + reparse->Substitute_Name_وتھ / حجم(WCHAR);
                len = reparse->Substitute_Name_لمائی / حجم(WCHAR);
                إذا (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = Wide_Char_منزل_Multi_Byte(CP_UTF8, 0, name, len, o_نشانہ, i_ناپ, عدم, عدم);
            إذا (*o_len <= 0 && len != 0) {
                err = errno_from_win32(Get_آخری_Error());
            }
        }
    }

    m3_Free(reparse);
    أرجع err;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_رستہ, حرف* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_ورتیا)
{
    *o_buf_ورتیا = 0;

    حرف           بھریا_رستہ[d_m3_Wasi_Max_Host_رستہ];
    __wasi_errno_t err = resolve_رستہ(بھریا_رستہ, i_dirfd, i_رستہ);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    حرف نشانہ[d_m3_Wasi_Max_Host_رستہ];
    صحيح  نشانہ_len = 0;

    err = read_link_نشانہ(بھریا_رستہ, نشانہ, (صحيح)حجم(نشانہ), &نشانہ_len);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)نشانہ_len, i_bufLen);
    حافظہ_نقل(o_buf, نشانہ, copied);
    *o_buf_ورتیا = copied;
    أرجع __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

// Windows offers the one clock, so every WASI clock maps onto it and none of them is
// rejected
ساكن
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    // a FILETIME counts in 100ns, but the system clock behind it moves on the timer
    // tick, so this says a millisecond rather than what it can spell
    *o_res = 1000000;
    أرجع __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
ساكن
__wasi_timestamp_t now_ns (فراغ)
{
    FILETIME now;
    Get_System_ویلا_As_File_ویلا(&now);

    أرجع convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_ویلا_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_ویلا)
{
    *o_ویلا = now_ns();
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_random_get (فراغ* o_buf, __wasi_size_t i_len)
{
    إذا (!RtlGenRandom(o_buf, i_len)) {
        أرجع __WASI_ERRNO_IO;
    }
    أرجع __WASI_ERRNO_SUCCESS;
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
ساكن
__wasi_errno_t poll_fd_ready (ثابت m3_wasi_pollsub_t* i_sub, bool* o_ready,
                              __wasi_filesize_t* o_nbytes, __wasi_eventrwflags_t* o_flags)
{
    *o_ready = false;
    *o_nbytes = 0;
    *o_flags = 0;

    HANDLE         handle;

    __wasi_errno_t err = handle_for_fd(i_sub->fd, &handle);
    إذا (err != __WASI_ERRNO_SUCCESS) {
        أرجع err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    إذا (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        أرجع __WASI_ERRNO_SUCCESS;
    }

    إذا (type == FILE_TYPE_PIPE) {
        DWORD اڈیک = 0;

        إذا (!PeekNamedPipe(handle, عدم, 0, عدم, &اڈیک, عدم)) {
            DWORD آخری = Get_آخری_Error();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            إذا (آخری == ERROR_BROKEN_PIPE or آخری == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                أرجع __WASI_ERRNO_SUCCESS;
            }
            أرجع errno_from_win32(آخری);
        }

        *o_ready = (اڈیک != 0);
        *o_nbytes = اڈیک;
        أرجع __WASI_ERRNO_SUCCESS;
    }

    إذا (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        أرجع __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    أرجع __WASI_ERRNO_SUCCESS;
}

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock and ignores i_elapsedNs, the clock having moved on by itself.
// Every WASI clock is the system clock here - see clock_time_get - so an absolute
// deadline is held against that one whichever clock it names.
ساكن
uint64_t poll_بچیا_ns (ثابت m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    إذا (!i_sub->isAbsolute) {
        أرجع (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    أرجع (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

ساكن
__wasi_errno_t m3_wasi_host_poll_oneoff (ثابت m3_wasi_pollsub_t* i_subs, __wasi_size_t i_گنتی,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_گنتی)
{
    *o_event_گنتی = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
        إذا (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_بچیا_ns(&i_subs[i], 0));
            hasClock = true;
        } وإلا {
            hasFd = true;
        }
    }

    إذا (!hasClock and !hasFd) {
        أرجع __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t شروع_Ns = now_ns();

    لكل (;;) {
        __wasi_size_t باہرلا = 0;

        لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
            ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

            إذا (sub->type == __WASI_EVENTTYPE_CLOCK) {
                استمر;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            إذا (err == __WASI_ERRNO_SUCCESS and !ready) {
                استمر;
            }

            o_events[باہرلا].userdata = sub->userdata;
            o_events[باہرلا].error = err;
            o_events[باہرلا].type = sub->type;
            o_events[باہرلا].nbytes = nbytes;
            o_events[باہرلا].flags = flags;
            باہرلا++;
        }

        // This is a wall clock and can be set backwards under us, which is not
        // elapsed time going negative but the measurement being no longer usable;
        // starting the count again from here is the honest reading of it.
        ثابت uint64_t t = now_ns();
        ثابت uint64_t elapsedNs = (t > شروع_Ns) ? t - شروع_Ns : 0;

        إذا (t < شروع_Ns) {
            شروع_Ns = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        إذا (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
                ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

                إذا (sub->type == __WASI_EVENTTYPE_CLOCK and
                    poll_بچیا_ns(sub, elapsedNs) == 0) {
                    o_events[باہرلا].userdata = sub->userdata;
                    o_events[باہرلا].error = __WASI_ERRNO_SUCCESS;
                    o_events[باہرلا].type = __WASI_EVENTTYPE_CLOCK;
                    o_events[باہرلا].nbytes = 0;
                    o_events[باہرلا].flags = 0;
                    باہرلا++;
                }
            }
        }

        إذا (باہرلا) {
            *o_event_گنتی = باہرلا;
            أرجع __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        إذا (hasClock) {
            uint64_t بچیا_Ns = soonestNs - elapsedNs;
            uint64_t بچیا_Ms = (بچیا_Ns + 999999) / 1000000;

            إذا (not hasFd or بچیا_Ms < sleepMs) {
                sleepMs = (بچیا_Ms > MAXDWORD) ? MAXDWORD : (DWORD)بچیا_Ms;
            }
        }

        Sleep(sleepMs);
    }
}

#جے_انت // m3_api_wasi_win32_h
