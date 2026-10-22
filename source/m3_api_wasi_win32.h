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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_api_wasi_win32_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_api_wasi_win32_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

#ਜੇ !defined(_WIN32)
#  error "This file is for Windows only"
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ <Windows.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <errno.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <fcntl.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <io.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <stdio.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <string.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/stat.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <wchar.h>

// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ SystemFunction036 NTAPI SystemFunction036
#ਅਨ੍ਤਰ੍ਭਾਵ <NTSecAPI.h>
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ SystemFunction036

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_wasi_host.h"

// Room for a preopen's real path in front of the guest's
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Wasi_Max_Host_ਰਸਤਾ  (d_m3_Wasi_Max_ਰਸਤਾ + 16)

// 1601-01-01, where a FILETIME counts its 100ns ticks from, in those same ticks
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3_Win_Epoch_ਵਿੱਥ   116444736000000000ULL

// Only the Windows 10 SDK and later declare it
#ਜੇ !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#ਜੇ_ਅੰਤ

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#ਜੇ !defined(FSCTL_GET_REPARSE_ਬਿੰਦੂ)
#  define FSCTL_GET_REPARSE_ਬਿੰਦੂ  0x000900A8
#ਜੇ_ਅੰਤ
#ਜੇ !defined(ਵੱਧ_ਤੋਂ_ਵੱਧ_REPARSE_ਅੰਕੜੇ_ਬਫਰ_ਮਾਪ)
#  define ਵੱਧ_ਤੋਂ_ਵੱਧ_REPARSE_ਅੰਕੜੇ_ਬਫਰ_ਮਾਪ  (16 * 1024)
#ਜੇ_ਅੰਤ

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ Reparse_ਅੰਕੜੇ_ਬਫਰ {
    ULONG  Reparse_ਟੈਗ;
    USHORT Reparse_ਅੰਕੜੇ_ਲੰਮਾਈ;
    USHORT Reserved;
    USHORT Substitute_Name_ਵਿੱਥ;
    USHORT Substitute_Name_ਲੰਮਾਈ;
    USHORT Print_Name_ਵਿੱਥ;
    USHORT Print_Name_ਲੰਮਾਈ;
    ਸੰਘ {
        ਬਣਤਰ {
            ULONG Flags;
            WCHAR ਰਸਤਾ_ਬਫਰ[1];
        } symlink;
        ਬਣਤਰ {
            WCHAR ਰਸਤਾ_ਬਫਰ[1];
        } mount_ਬਿੰਦੂ;
    } u;
} Reparse_ਅੰਕੜੇ_ਬਫਰ;


/*
 * Plumbing
 */

// What the CRT calls used here leave in errno. Everything the Win32 API reports goes
// through errno_from_win32 instead.
ਸਥਿਰ
__wasi_errno_t errno_from_crt (ਪੂਰਨ err)
{
    // clang-format off
    ਚੋਣ (err) {
    ਹਾਲਤ 0:       ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    ਹਾਲਤ EACCES:  ਵਾਪਸ __WASI_ERRNO_ACCES;
    ਹਾਲਤ EBADF:   ਵਾਪਸ __WASI_ERRNO_BADF;
    ਹਾਲਤ EEXIST:  ਵਾਪਸ __WASI_ERRNO_EXIST;
    ਹਾਲਤ EMFILE:  ਵਾਪਸ __WASI_ERRNO_MFILE;
    ਹਾਲਤ ENOENT:  ਵਾਪਸ __WASI_ERRNO_NOENT;
    ਹਾਲਤ ENOSPC:  ਵਾਪਸ __WASI_ERRNO_NOSPC;
    ਹਾਲਤ EINVAL:  ਵਾਪਸ __WASI_ERRNO_INVAL;
    }
    // clang-format on
    ਵਾਪਸ __WASI_ERRNO_IO;
}

ਸਥਿਰ
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    ਚੋਣ (err) {
    ਹਾਲਤ ERROR_SUCCESS:               ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    ਹਾਲਤ ERROR_INVALID_FUNCTION:      ਵਾਪਸ __WASI_ERRNO_NOTSUP;
    ਹਾਲਤ ERROR_FILE_NOT_ਮਿਲਿਆ:
    ਹਾਲਤ ERROR_ਰਸਤਾ_NOT_ਮਿਲਿਆ:
    ਹਾਲਤ ERROR_INVALID_DRIVE:
    ਹਾਲਤ ERROR_BAD_NETPATH:
    ਹਾਲਤ ERROR_BAD_PATHNAME:          ਵਾਪਸ __WASI_ERRNO_NOENT;
    ਹਾਲਤ ERROR_TOO_MANY_OPEN_FILES:   ਵਾਪਸ __WASI_ERRNO_MFILE;
    ਹਾਲਤ ERROR_ACCESS_DENIED:
    ਹਾਲਤ ERROR_WRITE_PROTECT:         ਵਾਪਸ __WASI_ERRNO_ACCES;
    ਹਾਲਤ ERROR_INVALID_HANDLE:        ਵਾਪਸ __WASI_ERRNO_BADF;
    ਹਾਲਤ ERROR_NOT_ENOUGH_ਸਮ੍ਰਿਤੀ:
    ਹਾਲਤ ERROR_OUTOFMEMORY:           ਵਾਪਸ __WASI_ERRNO_NOMEM;
    ਹਾਲਤ ERROR_NOT_SAME_DEVICE:       ਵਾਪਸ __WASI_ERRNO_XDEV;
    ਹਾਲਤ ERROR_WRITE_FAULT:
    ਹਾਲਤ ERROR_READ_FAULT:            ਵਾਪਸ __WASI_ERRNO_IO;
    ਹਾਲਤ ERROR_SHARING_VIOLATION:
    ਹਾਲਤ ERROR_ਤਾਲਾ_VIOLATION:        ਵਾਪਸ __WASI_ERRNO_BUSY;
    ਹਾਲਤ ERROR_HANDLE_DISK_ਭਰਿਆ:
    ਹਾਲਤ ERROR_DISK_ਭਰਿਆ:             ਵਾਪਸ __WASI_ERRNO_NOSPC;
    ਹਾਲਤ ERROR_FILE_EXISTS:
    ਹਾਲਤ ERROR_ALREADY_EXISTS:        ਵਾਪਸ __WASI_ERRNO_EXIST;
    ਹਾਲਤ ERROR_ਬਫਰ_ਉਛਾਲ:
    ਹਾਲਤ ERROR_INSUFFICIENT_ਬਫਰ:
    ਹਾਲਤ ERROR_FILENAME_EXCED_RANGE:  ਵਾਪਸ __WASI_ERRNO_NAMETOOLONG;
    ਹਾਲਤ ERROR_DIR_NOT_ਖਾਲੀ_ਥਾਵਾਂ:         ਵਾਪਸ __WASI_ERRNO_NOTEMPTY;
    ਹਾਲਤ ERROR_DIRECTORY:             ਵਾਪਸ __WASI_ERRNO_NOTDIR;
    ਹਾਲਤ ERROR_PRIVILEGE_NOT_HELD:    ਵਾਪਸ __WASI_ERRNO_PERM;
    ਹਾਲਤ ERROR_NOT_SUPPORTED:         ਵਾਪਸ __WASI_ERRNO_NOTSUP;
    ਹਾਲਤ ERROR_CANT_RESOLVE_FILENAME: ਵਾਪਸ __WASI_ERRNO_LOOP;
    ਹਾਲਤ ERROR_NEGATIVE_SEEK:
    ਹਾਲਤ ERROR_INVALID_NAME:
    ਹਾਲਤ ERROR_INVALID_PARAMETER:
    ਹਾਲਤ ERROR_NOT_A_REPARSE_ਬਿੰਦੂ:   ਵਾਪਸ __WASI_ERRNO_INVAL;
    }
    // clang-format on
    ਵਾਪਸ __WASI_ERRNO_IO;
}

ਸਥਿਰ
__wasi_timestamp_t convert_filetime (FILETIME i_ਸਮਾਂ)
{
    uint64_t ticks = ((uint64_t)i_ਸਮਾਂ.dw_ਉੱਚਾ_Date_ਸਮਾਂ << 32) | i_ਸਮਾਂ.dw_ਨੀਵਾਂ_Date_ਸਮਾਂ;
    ਜੇ (ticks < d_m3_Win_Epoch_ਵਿੱਥ) {
        ਵਾਪਸ 0;
    }
    ਵਾਪਸ (ticks - d_m3_Win_Epoch_ਵਿੱਥ) * 100;
}

ਸਥਿਰ
FILETIME convert_timestamp (__wasi_timestamp_t i_ਸਮਾਂ)
{
    uint64_t ticks = i_ਸਮਾਂ / 100 + d_m3_Win_Epoch_ਵਿੱਥ;
    FILETIME ਬਾਹਰਲਾ;
    ਬਾਹਰਲਾ.dw_ਨੀਵਾਂ_Date_ਸਮਾਂ = (DWORD)ticks;
    ਬਾਹਰਲਾ.dw_ਉੱਚਾ_Date_ਸਮਾਂ = (DWORD)(ticks >> 32);
    ਵਾਪਸ ਬਾਹਰਲਾ;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
ਸਥਿਰ
bool is_preopen (__wasi_fd_t i_fd)
{
    ਵਾਪਸ i_fd >= 3 && i_fd < d_m3_Wasi_Preopen_ਗਿਣਤੀ && m3_wasi_preopen[i_fd].fd == (ਪੂਰਨ)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
ਸਥਿਰ
__wasi_errno_t resolve_ਰਸਤਾ (ਅੱਖਰ* o_ਰਸਤਾ, __wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਜੇ (!is_preopen(i_dirfd)) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਪੂਰਨ len = snprintf(o_ਰਸਤਾ, d_m3_Wasi_Max_Host_ਰਸਤਾ, "%s/%s", m3_wasi_preopen[i_dirfd].real_ਰਸਤਾ,
                       i_ਰਸਤਾ);
    ਜੇ (len < 0 || len >= d_m3_Wasi_Max_Host_ਰਸਤਾ) {
        ਵਾਪਸ __WASI_ERRNO_NAMETOOLONG;
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
ਸਥਿਰ
__wasi_errno_t open_ਰਸਤਾ_handle (ccstr_t i_ਰਸਤਾ, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    ਜੇ (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_ਬਿੰਦੂ;
    }

    HANDLE handle = CreateFileA(i_ਰਸਤਾ, FILE_READ_ਗੁਣ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ਨਿਰਾ,
                                OPEN_EXISTING, flags, ਨਿਰਾ);
    ਜੇ (handle == INVALID_HANDLE_ਮੁੱਲ) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }

    *o_handle = handle;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    ਜੇ (handle == INVALID_HANDLE_ਮੁੱਲ) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
ਸਥਿਰ
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ਆਕਾਰ(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    ਜੇ (!GetFileInformationByHandle(i_handle, &info)) {
        ਚੋਣ (GetFileType(i_handle)) {
        ਹਾਲਤ FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; ਰੋਕੋ;
        ਹਾਲਤ FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; ਰੋਕੋ;
        ਮੂਲ: ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
        }
        ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dw_Volume_Serial_ਸੰਖਿਆ;
    o_stat->ino = ((uint64_t)info.n_File_ਸੂਚਕਾਂਕ_ਉੱਚਾ << 32) | info.n_File_ਸੂਚਕਾਂਕ_ਨੀਵਾਂ;
    o_stat->nlink = info.n_ਸੰਖਿਆ_ਦਾ_Links;
    o_stat->ਮਾਪ = ((uint64_t)info.n_File_ਮਾਪ_ਉੱਚਾ << 32) | info.n_File_ਮਾਪ_ਨੀਵਾਂ;
    o_stat->atim = convert_filetime(info.ft_ਆਖਰੀ_Access_ਸਮਾਂ);
    o_stat->mtim = convert_filetime(info.ft_ਆਖਰੀ_Write_ਸਮਾਂ);
    o_stat->ctim = convert_filetime(info.ft_Creation_ਸਮਾਂ);

    ਜੇ (info.dw_File_ਗੁਣ & FILE_ATTRIBUTE_REPARSE_ਬਿੰਦੂ) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } ਨਹੀਂਤਾਂ ਜੇ (info.dw_File_ਗੁਣ & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } ਨਹੀਂਤਾਂ {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}


/*
 * Setup
 */

ਸਥਿਰ
ਖਾਲੀ m3_wasi_host_init ()
{
    _setmode(_fileno(stdin), O_ਦੁਆਧਾਰੀ);
    _setmode(_fileno(stdout), O_ਦੁਆਧਾਰੀ);
    _setmode(_fileno(stderr), O_ਦੁਆਧਾਰੀ);

    // Windows cannot open a directory as a descriptor, so a preopen is resolved by
    // path instead. It still has to hold a descriptor number: the guest is handed
    // one, and without something occupying it the next file opened would be given
    // the same number and the two would be indistinguishable.
    ਲਈ (ਪੂਰਨ i = 3; i < d_m3_Wasi_Preopen_ਗਿਣਤੀ; i++) {
        ਜੇ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = _open("NUL", _O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ਜੇ (_close(i_fd) != 0) {
        ਵਾਪਸ errno_from_crt(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    ਜੇ (_commit(i_fd) != 0) {
        ਵਾਪਸ errno_from_crt(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_ਵਿੱਥ,
                                       __wasi_filesize_t i_ਲੰਮਾਈ, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    ਜੇ (i_advice > __WASI_ADVICE_NOREUSE) {
        ਵਾਪਸ __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    ਵਾਪਸ handle_for_fd(i_fd, &handle);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ਆਕਾਰ(*o_stat));

    ਜੇ (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    ਵਾਪਸ filestat_from_handle(handle, o_stat);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// Nothing here can be changed after the fact: append and non-blocking are fixed when
// CreateFile runs, and the CRT hands back no way to revisit either. fd_fdstat above
// reports no flags at all for the same reason, so the only request that can be
// honoured is the one asking for exactly that.
ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_fdstat_ਸੈੱਟ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    ਵਾਪਸ (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_ਸੈੱਟ_ਮਾਪ (__wasi_fd_t i_fd, __wasi_filesize_t i_ਮਾਪ)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    ਵਾਪਸ errno_from_crt(_chsize_s(i_fd, (__int64)i_ਮਾਪ));
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_ਸੈੱਟ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    Get_System_ਸਮਾਂ_As_File_ਸਮਾਂ(&now);

    ਅਚਲ FILETIME* atime = ਨਿਰਾ;
    ਅਚਲ FILETIME* mtime = ਨਿਰਾ;

    ਜੇ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } ਨਹੀਂਤਾਂ ਜੇ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    ਜੇ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } ਨਹੀਂਤਾਂ ਜੇ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    ਜੇ (!ਸੈੱਟ_File_ਸਮਾਂ(handle, ਨਿਰਾ, atime, mtime)) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_ਵਿੱਥ, ਪੂਰਨ i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_ਵਿੱਥ, i_whence);
    ਜੇ (ret < 0) {
        ਵਾਪਸ errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

// Windows has neither a vectored nor a positional transfer, so the list is walked
// one buffer at a time and a short transfer ends it, as it would for readv. A
// positional one moves the file pointer and puts it back where it was.
ਸਥਿਰ
__wasi_errno_t transfer_iovs (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                              __wasi_size_t i_iovsLen, __wasi_filesize_t i_ਵਿੱਥ,
                              bool i_positional, bool i_write, __wasi_size_t* o_ਪੂਰਾ)
{
    *o_ਪੂਰਾ = 0;

    int64_t saved = 0;

    ਜੇ (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        ਜੇ (saved < 0 || _lseeki64(i_fd, (__int64)i_ਵਿੱਥ, SEEK_SET) < 0) {
            ਵਾਪਸ errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         ਪੂਰਾ = 0;

    ਲਈ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ਖਾਲੀ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ਜੇ (len == 0) {
            ਜਾਰੀ;
        }

        ਪੂਰਨ ret = i_write ? _write(i_fd, addr, (ਬਿਨਾਨਿਸ਼ਾਨੀ)len) : _read(i_fd, addr, (ਬਿਨਾਨਿਸ਼ਾਨੀ)len);
        ਜੇ (ret < 0) {
            err = errno_from_crt(errno);
            ਰੋਕੋ;
        }

        ਪੂਰਾ += (size_t)ret;
        ਜੇ ((size_t)ret < len) {
            ਰੋਕੋ;
        }
    }

    ਜੇ (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    *o_ਪੂਰਾ = (__wasi_size_t)ਪੂਰਾ;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_ਪੂਰਾ);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_ਪੂਰਾ);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_ਵਿੱਥ,
                                      __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_iovs(i_fd, i_iovs, i_iovsLen, i_ਵਿੱਥ, true, false, o_ਪੂਰਾ);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_ਵਿੱਥ,
                                       __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_iovs(i_fd, i_iovs, i_iovsLen, i_ਵਿੱਥ, true, true, o_ਪੂਰਾ);
}


/*
 * Paths
 */

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_open (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    ਅੱਖਰ           ਭਰਿਆ_ਰਸਤਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_ਰਸਤਾ, i_dirfd, i_ਰਸਤਾ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    ਜੇ (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_ਰਸਤਾ_handle(ਭਰਿਆ_ਰਸਤਾ, true, &handle);
        ਜੇ (err != __WASI_ERRNO_SUCCESS) {
            ਵਾਪਸ err;
        }

        ਪੂਰਨ dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        ਜੇ (dir_fd < 0) {
            CloseHandle(handle);
            ਵਾਪਸ __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    ਪੂਰਨ flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                O_ਦੁਆਧਾਰੀ;

    ਜੇ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } ਨਹੀਂਤਾਂ ਜੇ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } ਨਹੀਂਤਾਂ ਜੇ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    ਪੂਰਨ host_fd = _open(ਭਰਿਆ_ਰਸਤਾ, flags, _S_IREAD | _S_IWRITE);
    ਜੇ (host_fd < 0) {
        ਵਾਪਸ errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ਆਕਾਰ(*o_stat));

    ਅੱਖਰ           ਭਰਿਆ_ਰਸਤਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_ਰਸਤਾ, i_dirfd, i_ਰਸਤਾ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    HANDLE handle;
    err = open_ਰਸਤਾ_handle(ਭਰਿਆ_ਰਸਤਾ, i_follow, &handle);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    ਵਾਪਸ err;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਅੱਖਰ           ਭਰਿਆ_ਰਸਤਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_ਰਸਤਾ, i_dirfd, i_ਰਸਤਾ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    ਜੇ (!CreateDirectoryA(ਭਰਿਆ_ਰਸਤਾ, ਨਿਰਾ)) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਅੱਖਰ           ਭਰਿਆ_ਰਸਤਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_ਰਸਤਾ, i_dirfd, i_ਰਸਤਾ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    ਜੇ (!RemoveDirectoryA(ਭਰਿਆ_ਰਸਤਾ)) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਅੱਖਰ           ਭਰਿਆ_ਰਸਤਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_ਰਸਤਾ, i_dirfd, i_ਰਸਤਾ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = Get_File_ਗੁਣ_A(ਭਰਿਆ_ਰਸਤਾ);
    bool  isDirLink =
      attrs != INVALID_FILE_ਗੁਣ &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_ਬਿੰਦੂ)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_ਬਿੰਦੂ);

    ਜੇ (!(isDirLink ? RemoveDirectoryA(ਭਰਿਆ_ਰਸਤਾ) : DeleteFileA(ਭਰਿਆ_ਰਸਤਾ))) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ਰਸਤਾ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_ਰਸਤਾ)
{
    ਅੱਖਰ           ਭਰਿਆ_old[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    ਅੱਖਰ           ਭਰਿਆ_new[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_old, i_oldDirfd, i_old_ਰਸਤਾ);
    ਜੇ (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_ਰਸਤਾ(ਭਰਿਆ_new, i_newDirfd, i_new_ਰਸਤਾ);
    }
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    ਜੇ (!MoveFileExA(ਭਰਿਆ_old, ਭਰਿਆ_new, MOVEFILE_REPLACE_EXISTING)) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ਰਸਤਾ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_ਰਸਤਾ)
{
    ਅੱਖਰ           ਭਰਿਆ_old[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    ਅੱਖਰ           ਭਰਿਆ_new[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_old, i_oldDirfd, i_old_ਰਸਤਾ);
    ਜੇ (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_ਰਸਤਾ(ਭਰਿਆ_new, i_newDirfd, i_new_ਰਸਤਾ);
    }
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    ਜੇ (!CreateHardLinkA(ਭਰਿਆ_new, ਭਰਿਆ_old, ਨਿਰਾ)) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_symlink (ccstr_t i_ਨਿਸ਼ਾਨਾ, __wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਅੱਖਰ           ਭਰਿਆ_new[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_new, i_dirfd, i_ਰਸਤਾ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    ਅੱਖਰ   ਨਿਸ਼ਾਨਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    size_t dir_len = 0;
    ਲਈ (size_t i = 0; ਭਰਿਆ_new[i]; i++) {
        ਜੇ (ਭਰਿਆ_new[i] == '/' || ਭਰਿਆ_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    ਪੂਰਨ ਨਿਸ਼ਾਨਾ_len = snprintf(ਨਿਸ਼ਾਨਾ, ਆਕਾਰ(ਨਿਸ਼ਾਨਾ), "%.*s%s", (ਪੂਰਨ)dir_len, ਭਰਿਆ_new, i_ਨਿਸ਼ਾਨਾ);
    ਜੇ (ਨਿਸ਼ਾਨਾ_len < 0 || ਨਿਸ਼ਾਨਾ_len >= (ਪੂਰਨ)ਆਕਾਰ(ਨਿਸ਼ਾਨਾ)) {
        ਵਾਪਸ __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = Get_File_ਗੁਣ_A(ਨਿਸ਼ਾਨਾ);
    ਜੇ (attrs != INVALID_FILE_ਗੁਣ && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    ਜੇ (!CreateSymbolicLinkA(ਭਰਿਆ_new, i_ਨਿਸ਼ਾਨਾ,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        ਜੇ (Get_ਆਖਰੀ_Error() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(ਭਰਿਆ_new, i_ਨਿਸ਼ਾਨਾ, flags)) {
            ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
        }
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
ਸਥਿਰ
__wasi_errno_t read_link_ਨਿਸ਼ਾਨਾ (ccstr_t i_ਰਸਤਾ, ਅੱਖਰ* o_ਨਿਸ਼ਾਨਾ, ਪੂਰਨ i_ਮਾਪ, ਪੂਰਨ* o_len)
{
    HANDLE handle = CreateFileA(i_ਰਸਤਾ, FILE_READ_ਗੁਣ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ਨਿਰਾ,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_ਬਿੰਦੂ, ਨਿਰਾ);
    ਜੇ (handle == INVALID_HANDLE_ਮੁੱਲ) {
        ਵਾਪਸ errno_from_win32(Get_ਆਖਰੀ_Error());
    }

    Reparse_ਅੰਕੜੇ_ਬਫਰ* reparse =
      (Reparse_ਅੰਕੜੇ_ਬਫਰ*)m3_Malloc("ReparseDataBuffer", ਵੱਧ_ਤੋਂ_ਵੱਧ_REPARSE_ਅੰਕੜੇ_ਬਫਰ_ਮਾਪ);
    ਜੇ (!reparse) {
        CloseHandle(handle);
        ਵਾਪਸ __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    ਜੇ (!DeviceIoControl(handle, FSCTL_GET_REPARSE_ਬਿੰਦੂ, ਨਿਰਾ, 0, reparse,
                         ਵੱਧ_ਤੋਂ_ਵੱਧ_REPARSE_ਅੰਕੜੇ_ਬਫਰ_ਮਾਪ, &returned, ਨਿਰਾ)) {
        err = errno_from_win32(Get_ਆਖਰੀ_Error());
    }
    CloseHandle(handle);

    ਜੇ (err == __WASI_ERRNO_SUCCESS) {
        ਅਚਲ WCHAR* ਨਾਂ;
        ਜੇ (reparse->Reparse_ਟੈਗ == IO_REPARSE_ਟੈਗ_SYMLINK) {
            ਨਾਂ = reparse->u.symlink.ਰਸਤਾ_ਬਫਰ;
        } ਨਹੀਂਤਾਂ ਜੇ (reparse->Reparse_ਟੈਗ == IO_REPARSE_ਟੈਗ_MOUNT_ਬਿੰਦੂ) {
            ਨਾਂ = reparse->u.mount_ਬਿੰਦੂ.ਰਸਤਾ_ਬਫਰ;
        } ਨਹੀਂਤਾਂ {
            ਨਾਂ = ਨਿਰਾ;
            err = __WASI_ERRNO_INVAL;
        }

        ਜੇ (ਨਾਂ) {
            ਅਚਲ WCHAR* name = ਨਾਂ + reparse->Print_Name_ਵਿੱਥ / ਆਕਾਰ(WCHAR);
            ਪੂਰਨ          len = reparse->Print_Name_ਲੰਮਾਈ / ਆਕਾਰ(WCHAR);

            ਜੇ (len == 0) {
                name = ਨਾਂ + reparse->Substitute_Name_ਵਿੱਥ / ਆਕਾਰ(WCHAR);
                len = reparse->Substitute_Name_ਲੰਮਾਈ / ਆਕਾਰ(WCHAR);
                ਜੇ (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = Wide_Char_ਮੰਜ਼ਿਲ_Multi_Byte(CP_UTF8, 0, name, len, o_ਨਿਸ਼ਾਨਾ, i_ਮਾਪ, ਨਿਰਾ, ਨਿਰਾ);
            ਜੇ (*o_len <= 0 && len != 0) {
                err = errno_from_win32(Get_ਆਖਰੀ_Error());
            }
        }
    }

    m3_Free(reparse);
    ਵਾਪਸ err;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ, ਅੱਖਰ* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_ਵਰਤਿਆ)
{
    *o_buf_ਵਰਤਿਆ = 0;

    ਅੱਖਰ           ਭਰਿਆ_ਰਸਤਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    __wasi_errno_t err = resolve_ਰਸਤਾ(ਭਰਿਆ_ਰਸਤਾ, i_dirfd, i_ਰਸਤਾ);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    ਅੱਖਰ ਨਿਸ਼ਾਨਾ[d_m3_Wasi_Max_Host_ਰਸਤਾ];
    ਪੂਰਨ  ਨਿਸ਼ਾਨਾ_len = 0;

    err = read_link_ਨਿਸ਼ਾਨਾ(ਭਰਿਆ_ਰਸਤਾ, ਨਿਸ਼ਾਨਾ, (ਪੂਰਨ)ਆਕਾਰ(ਨਿਸ਼ਾਨਾ), &ਨਿਸ਼ਾਨਾ_len);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)ਨਿਸ਼ਾਨਾ_len, i_bufLen);
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_buf, ਨਿਸ਼ਾਨਾ, copied);
    *o_buf_ਵਰਤਿਆ = copied;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

// Windows offers the one clock, so every WASI clock maps onto it and none of them is
// rejected
ਸਥਿਰ
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    // a FILETIME counts in 100ns, but the system clock behind it moves on the timer
    // tick, so this says a millisecond rather than what it can spell
    *o_res = 1000000;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
ਸਥਿਰ
__wasi_timestamp_t now_ns (ਖਾਲੀ)
{
    FILETIME now;
    Get_System_ਸਮਾਂ_As_File_ਸਮਾਂ(&now);

    ਵਾਪਸ convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_ਸਮਾਂ_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_ਸਮਾਂ)
{
    *o_ਸਮਾਂ = now_ns();
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_random_get (ਖਾਲੀ* o_buf, __wasi_size_t i_len)
{
    ਜੇ (!RtlGenRandom(o_buf, i_len)) {
        ਵਾਪਸ __WASI_ERRNO_IO;
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
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
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_m3WasiPollIntervalMs  5

// Whether a descriptor has what its subscription is waiting for. Everything that is
// not a stream is ready by definition: the spec says a regular file always triggers
// both read and write, and so does anything whose kind Windows will not describe.
ਸਥਿਰ
__wasi_errno_t poll_fd_ready (ਅਚਲ m3_wasi_pollsub_t* i_sub, bool* o_ready,
                              __wasi_filesize_t* o_nbytes, __wasi_eventrwflags_t* o_flags)
{
    *o_ready = false;
    *o_nbytes = 0;
    *o_flags = 0;

    HANDLE         handle;

    __wasi_errno_t err = handle_for_fd(i_sub->fd, &handle);
    ਜੇ (err != __WASI_ERRNO_SUCCESS) {
        ਵਾਪਸ err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    ਜੇ (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    }

    ਜੇ (type == FILE_TYPE_PIPE) {
        DWORD ਉਡੀਕ = 0;

        ਜੇ (!PeekNamedPipe(handle, ਨਿਰਾ, 0, ਨਿਰਾ, &ਉਡੀਕ, ਨਿਰਾ)) {
            DWORD ਆਖਰੀ = Get_ਆਖਰੀ_Error();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            ਜੇ (ਆਖਰੀ == ERROR_BROKEN_PIPE or ਆਖਰੀ == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                ਵਾਪਸ __WASI_ERRNO_SUCCESS;
            }
            ਵਾਪਸ errno_from_win32(ਆਖਰੀ);
        }

        *o_ready = (ਉਡੀਕ != 0);
        *o_nbytes = ਉਡੀਕ;
        ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    }

    ਜੇ (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        ਵਾਪਸ __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock and ignores i_elapsedNs, the clock having moved on by itself.
// Every WASI clock is the system clock here - see clock_time_get - so an absolute
// deadline is held against that one whichever clock it names.
ਸਥਿਰ
uint64_t poll_ਬਚਿਆ_ns (ਅਚਲ m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ਜੇ (!i_sub->isAbsolute) {
        ਵਾਪਸ (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    ਵਾਪਸ (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_poll_oneoff (ਅਚਲ m3_wasi_pollsub_t* i_subs, __wasi_size_t i_ਗਿਣਤੀ,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_ਗਿਣਤੀ)
{
    *o_event_ਗਿਣਤੀ = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
        ਜੇ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_ਬਚਿਆ_ns(&i_subs[i], 0));
            hasClock = true;
        } ਨਹੀਂਤਾਂ {
            hasFd = true;
        }
    }

    ਜੇ (!hasClock and !hasFd) {
        ਵਾਪਸ __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t ਸ਼ੁਰੂ_Ns = now_ns();

    ਲਈ (;;) {
        __wasi_size_t ਬਾਹਰਲਾ = 0;

        ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
            ਅਚਲ m3_wasi_pollsub_t* sub = &i_subs[i];

            ਜੇ (sub->type == __WASI_EVENTTYPE_CLOCK) {
                ਜਾਰੀ;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            ਜੇ (err == __WASI_ERRNO_SUCCESS and !ready) {
                ਜਾਰੀ;
            }

            o_events[ਬਾਹਰਲਾ].userdata = sub->userdata;
            o_events[ਬਾਹਰਲਾ].error = err;
            o_events[ਬਾਹਰਲਾ].type = sub->type;
            o_events[ਬਾਹਰਲਾ].nbytes = nbytes;
            o_events[ਬਾਹਰਲਾ].flags = flags;
            ਬਾਹਰਲਾ++;
        }

        // This is a wall clock and can be set backwards under us, which is not
        // elapsed time going negative but the measurement being no longer usable;
        // starting the count again from here is the honest reading of it.
        ਅਚਲ uint64_t t = now_ns();
        ਅਚਲ uint64_t elapsedNs = (t > ਸ਼ੁਰੂ_Ns) ? t - ਸ਼ੁਰੂ_Ns : 0;

        ਜੇ (t < ਸ਼ੁਰੂ_Ns) {
            ਸ਼ੁਰੂ_Ns = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        ਜੇ (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
                ਅਚਲ m3_wasi_pollsub_t* sub = &i_subs[i];

                ਜੇ (sub->type == __WASI_EVENTTYPE_CLOCK and
                    poll_ਬਚਿਆ_ns(sub, elapsedNs) == 0) {
                    o_events[ਬਾਹਰਲਾ].userdata = sub->userdata;
                    o_events[ਬਾਹਰਲਾ].error = __WASI_ERRNO_SUCCESS;
                    o_events[ਬਾਹਰਲਾ].type = __WASI_EVENTTYPE_CLOCK;
                    o_events[ਬਾਹਰਲਾ].nbytes = 0;
                    o_events[ਬਾਹਰਲਾ].flags = 0;
                    ਬਾਹਰਲਾ++;
                }
            }
        }

        ਜੇ (ਬਾਹਰਲਾ) {
            *o_event_ਗਿਣਤੀ = ਬਾਹਰਲਾ;
            ਵਾਪਸ __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        ਜੇ (hasClock) {
            uint64_t ਬਚਿਆ_Ns = soonestNs - elapsedNs;
            uint64_t ਬਚਿਆ_Ms = (ਬਚਿਆ_Ns + 999999) / 1000000;

            ਜੇ (not hasFd or ਬਚਿਆ_Ms < sleepMs) {
                sleepMs = (ਬਚਿਆ_Ms > MAXDWORD) ? MAXDWORD : (DWORD)ਬਚਿਆ_Ms;
            }
        }

        Sleep(sleepMs);
    }
}

#ਜੇ_ਅੰਤ // m3_api_wasi_win32_h
