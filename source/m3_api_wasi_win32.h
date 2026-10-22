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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_api_wasi_win32_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_api_wasi_win32_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಆದರೆ !defined(_WIN32)
#  error "This file is for Windows only"
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ <Windows.h>
#ಅನ್ತರ್ಭಾವ <errno.h>
#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <io.h>
#ಅನ್ತರ್ಭಾವ <stdio.h>
#ಅನ್ತರ್ಭಾವ <string.h>
#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ಅನ್ತರ್ಭಾವ <wchar.h>

// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#ವ್ಯಾಖ್ಯಾನಿಸು SystemFunction036 NTAPI SystemFunction036
#ಅನ್ತರ್ಭಾವ <NTSecAPI.h>
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು SystemFunction036

#ಅನ್ತರ್ಭಾವ "m3_api_wasi_host.h"

// Room for a preopen's real path in front of the guest's
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Wasi_Max_Host_ಮಾರ್ಗ  (d_m3_Wasi_Max_ಮಾರ್ಗ + 16)

// 1601-01-01, where a FILETIME counts its 100ns ticks from, in those same ticks
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3_Win_Epoch_ಸ್ಥಾನಾಂತರ   116444736000000000ULL

// Only the Windows 10 SDK and later declare it
#ಆದರೆ !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#ಆದರೆ_ಅಂತ್ಯ

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#ಆದರೆ !defined(FSCTL_GET_REPARSE_ಬಿಂದು)
#  define FSCTL_GET_REPARSE_ಬಿಂದು  0x000900A8
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ !defined(ಗರಿಷ್ಠ_REPARSE_ದತ್ತಾಂಶ_ಬಫರ್_ಗಾತ್ರ)
#  define ಗರಿಷ್ಠ_REPARSE_ದತ್ತಾಂಶ_ಬಫರ್_ಗಾತ್ರ  (16 * 1024)
#ಆದರೆ_ಅಂತ್ಯ

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ Reparse_ದತ್ತಾಂಶ_ಬಫರ್ {
    ULONG  Reparse_ಟ್ಯಾಗ್;
    USHORT Reparse_ದತ್ತಾಂಶ_ಉದ್ದಳತೆ;
    USHORT Reserved;
    USHORT Substitute_Name_ಸ್ಥಾನಾಂತರ;
    USHORT Substitute_Name_ಉದ್ದಳತೆ;
    USHORT Print_Name_ಸ್ಥಾನಾಂತರ;
    USHORT Print_Name_ಉದ್ದಳತೆ;
    ಒಕ್ಕೂಟ {
        ರಚನೆ {
            ULONG Flags;
            WCHAR ಮಾರ್ಗ_ಬಫರ್[1];
        } symlink;
        ರಚನೆ {
            WCHAR ಮಾರ್ಗ_ಬಫರ್[1];
        } mount_ಬಿಂದು;
    } u;
} Reparse_ದತ್ತಾಂಶ_ಬಫರ್;


/*
 * Plumbing
 */

// What the CRT calls used here leave in errno. Everything the Win32 API reports goes
// through errno_from_win32 instead.
ಸ್ಥಿರ
__wasi_errno_t errno_from_crt (ಪೂರ್ಣಾಂಕ err)
{
    // clang-format off
    ಆಯ್ಕೆ (err) {
    ಸ್ಥಿತಿ 0:       ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    ಸ್ಥಿತಿ EACCES:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_ACCES;
    ಸ್ಥಿತಿ EBADF:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    ಸ್ಥಿತಿ EEXIST:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_EXIST;
    ಸ್ಥಿತಿ EMFILE:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_MFILE;
    ಸ್ಥಿತಿ ENOENT:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOENT;
    ಸ್ಥಿತಿ ENOSPC:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOSPC;
    ಸ್ಥಿತಿ EINVAL:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }
    // clang-format on
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_IO;
}

ಸ್ಥಿರ
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    ಆಯ್ಕೆ (err) {
    ಸ್ಥಿತಿ ERROR_SUCCESS:               ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    ಸ್ಥಿತಿ ERROR_INVALID_FUNCTION:      ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOTSUP;
    ಸ್ಥಿತಿ ERROR_FILE_NOT_ಸಿಕ್ಕಿತು:
    ಸ್ಥಿತಿ ERROR_ಮಾರ್ಗ_NOT_ಸಿಕ್ಕಿತು:
    ಸ್ಥಿತಿ ERROR_INVALID_DRIVE:
    ಸ್ಥಿತಿ ERROR_BAD_NETPATH:
    ಸ್ಥಿತಿ ERROR_BAD_PATHNAME:          ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOENT;
    ಸ್ಥಿತಿ ERROR_TOO_MANY_OPEN_FILES:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_MFILE;
    ಸ್ಥಿತಿ ERROR_ACCESS_DENIED:
    ಸ್ಥಿತಿ ERROR_WRITE_PROTECT:         ಹಿಂತಿರುಗಿ __WASI_ERRNO_ACCES;
    ಸ್ಥಿತಿ ERROR_INVALID_HANDLE:        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    ಸ್ಥಿತಿ ERROR_NOT_ENOUGH_ಸ್ಮರಣೆ:
    ಸ್ಥಿತಿ ERROR_OUTOFMEMORY:           ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOMEM;
    ಸ್ಥಿತಿ ERROR_NOT_SAME_DEVICE:       ಹಿಂತಿರುಗಿ __WASI_ERRNO_XDEV;
    ಸ್ಥಿತಿ ERROR_WRITE_FAULT:
    ಸ್ಥಿತಿ ERROR_READ_FAULT:            ಹಿಂತಿರುಗಿ __WASI_ERRNO_IO;
    ಸ್ಥಿತಿ ERROR_SHARING_VIOLATION:
    ಸ್ಥಿತಿ ERROR_ಬೀಗ_VIOLATION:        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BUSY;
    ಸ್ಥಿತಿ ERROR_HANDLE_DISK_ತುಂಬಿದ:
    ಸ್ಥಿತಿ ERROR_DISK_ತುಂಬಿದ:             ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOSPC;
    ಸ್ಥಿತಿ ERROR_FILE_EXISTS:
    ಸ್ಥಿತಿ ERROR_ALREADY_EXISTS:        ಹಿಂತಿರುಗಿ __WASI_ERRNO_EXIST;
    ಸ್ಥಿತಿ ERROR_ಬಫರ್_ಅತಿಪ್ರವಾಹ:
    ಸ್ಥಿತಿ ERROR_INSUFFICIENT_ಬಫರ್:
    ಸ್ಥಿತಿ ERROR_FILENAME_EXCED_RANGE:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_NAMETOOLONG;
    ಸ್ಥಿತಿ ERROR_DIR_NOT_ಖಾಲಿಗಳು:         ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOTEMPTY;
    ಸ್ಥಿತಿ ERROR_DIRECTORY:             ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOTDIR;
    ಸ್ಥಿತಿ ERROR_PRIVILEGE_NOT_HELD:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_PERM;
    ಸ್ಥಿತಿ ERROR_NOT_SUPPORTED:         ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOTSUP;
    ಸ್ಥಿತಿ ERROR_CANT_RESOLVE_FILENAME: ಹಿಂತಿರುಗಿ __WASI_ERRNO_LOOP;
    ಸ್ಥಿತಿ ERROR_NEGATIVE_SEEK:
    ಸ್ಥಿತಿ ERROR_INVALID_NAME:
    ಸ್ಥಿತಿ ERROR_INVALID_PARAMETER:
    ಸ್ಥಿತಿ ERROR_NOT_A_REPARSE_ಬಿಂದು:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }
    // clang-format on
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_IO;
}

ಸ್ಥಿರ
__wasi_timestamp_t convert_filetime (FILETIME i_ಸಮಯ)
{
    uint64_t ticks = ((uint64_t)i_ಸಮಯ.dw_ಮೇಲಿನ_Date_ಸಮಯ << 32) | i_ಸಮಯ.dw_ಕೆಳಗಿನ_Date_ಸಮಯ;
    ಆದರೆ (ticks < d_m3_Win_Epoch_ಸ್ಥಾನಾಂತರ) {
        ಹಿಂತಿರುಗಿ 0;
    }
    ಹಿಂತಿರುಗಿ (ticks - d_m3_Win_Epoch_ಸ್ಥಾನಾಂತರ) * 100;
}

ಸ್ಥಿರ
FILETIME convert_timestamp (__wasi_timestamp_t i_ಸಮಯ)
{
    uint64_t ticks = i_ಸಮಯ / 100 + d_m3_Win_Epoch_ಸ್ಥಾನಾಂತರ;
    FILETIME ಹೊರಹೋಗು;
    ಹೊರಹೋಗು.dw_ಕೆಳಗಿನ_Date_ಸಮಯ = (DWORD)ticks;
    ಹೊರಹೋಗು.dw_ಮೇಲಿನ_Date_ಸಮಯ = (DWORD)(ticks >> 32);
    ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
ಸ್ಥಿರ
bool is_preopen (__wasi_fd_t i_fd)
{
    ಹಿಂತಿರುಗಿ i_fd >= 3 && i_fd < d_m3_Wasi_Preopen_ಎಣಿಕೆ && m3_wasi_preopen[i_fd].fd == (ಪೂರ್ಣಾಂಕ)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
ಸ್ಥಿರ
__wasi_errno_t resolve_ಮಾರ್ಗ (ಅಕ್ಷರ* o_ಮಾರ್ಗ, __wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಆದರೆ (!is_preopen(i_dirfd)) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ಪೂರ್ಣಾಂಕ len = snprintf(o_ಮಾರ್ಗ, d_m3_Wasi_Max_Host_ಮಾರ್ಗ, "%s/%s", m3_wasi_preopen[i_dirfd].real_ಮಾರ್ಗ,
                       i_ಮಾರ್ಗ);
    ಆದರೆ (len < 0 || len >= d_m3_Wasi_Max_Host_ಮಾರ್ಗ) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_NAMETOOLONG;
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
ಸ್ಥಿರ
__wasi_errno_t open_ಮಾರ್ಗ_handle (ccstr_t i_ಮಾರ್ಗ, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    ಆದರೆ (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_ಬಿಂದು;
    }

    HANDLE handle = CreateFileA(i_ಮಾರ್ಗ, FILE_READ_ಗುಣಲಕ್ಷಣಗಳು,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ಖಾಲಿ,
                                OPEN_EXISTING, flags, ಖಾಲಿ);
    ಆದರೆ (handle == INVALID_HANDLE_ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }

    *o_handle = handle;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    ಆದರೆ (handle == INVALID_HANDLE_ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
ಸ್ಥಿರ
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ಗಾತ್ರ(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    ಆದರೆ (!GetFileInformationByHandle(i_handle, &info)) {
        ಆಯ್ಕೆ (GetFileType(i_handle)) {
        ಸ್ಥಿತಿ FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
        }
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dw_Volume_Serial_ಸಂಖ್ಯೆ;
    o_stat->ino = ((uint64_t)info.n_File_ಸೂಚ್ಯಂಕ_ಮೇಲಿನ << 32) | info.n_File_ಸೂಚ್ಯಂಕ_ಕೆಳಗಿನ;
    o_stat->nlink = info.n_ಸಂಖ್ಯೆ_ದ_Links;
    o_stat->ಗಾತ್ರ_2 = ((uint64_t)info.n_File_ಗಾತ್ರ_ಮೇಲಿನ << 32) | info.n_File_ಗಾತ್ರ_ಕೆಳಗಿನ;
    o_stat->atim = convert_filetime(info.ft_ಕೊನೆಯ_Access_ಸಮಯ);
    o_stat->mtim = convert_filetime(info.ft_ಕೊನೆಯ_Write_ಸಮಯ);
    o_stat->ctim = convert_filetime(info.ft_Creation_ಸಮಯ);

    ಆದರೆ (info.dw_File_ಗುಣಲಕ್ಷಣಗಳು & FILE_ATTRIBUTE_REPARSE_ಬಿಂದು) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (info.dw_File_ಗುಣಲಕ್ಷಣಗಳು & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } ಇಲ್ಲದಿದ್ದರೆ {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}


/*
 * Setup
 */

ಸ್ಥಿರ
ಶೂನ್ಯ m3_wasi_host_init ()
{
    _setmode(_fileno(stdin), O_ದ್ವಿಮಾನ);
    _setmode(_fileno(stdout), O_ದ್ವಿಮಾನ);
    _setmode(_fileno(stderr), O_ದ್ವಿಮಾನ);

    // Windows cannot open a directory as a descriptor, so a preopen is resolved by
    // path instead. It still has to hold a descriptor number: the guest is handed
    // one, and without something occupying it the next file opened would be given
    // the same number and the two would be indistinguishable.
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 3; i < d_m3_Wasi_Preopen_ಎಣಿಕೆ; i++) {
        ಆದರೆ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = _open("NUL", _O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ಆದರೆ (_close(i_fd) != 0) {
        ಹಿಂತಿರುಗಿ errno_from_crt(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    ಆದರೆ (_commit(i_fd) != 0) {
        ಹಿಂತಿರುಗಿ errno_from_crt(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ,
                                       __wasi_filesize_t i_ಉದ್ದಳತೆ, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    ಆದರೆ (i_advice > __WASI_ADVICE_NOREUSE) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    ಹಿಂತಿರುಗಿ handle_for_fd(i_fd, &handle);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ಗಾತ್ರ(*o_stat));

    ಆದರೆ (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    ಹಿಂತಿರುಗಿ filestat_from_handle(handle, o_stat);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// Nothing here can be changed after the fact: append and non-blocking are fixed when
// CreateFile runs, and the CRT hands back no way to revisit either. fd_fdstat above
// reports no flags at all for the same reason, so the only request that can be
// honoured is the one asking for exactly that.
ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_fdstat_ಗಣ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    ಹಿಂತಿರುಗಿ (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_ಗಣ_ಗಾತ್ರ (__wasi_fd_t i_fd, __wasi_filesize_t i_ಗಾತ್ರ)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    ಹಿಂತಿರುಗಿ errno_from_crt(_chsize_s(i_fd, (__int64)i_ಗಾತ್ರ));
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_ಗಣ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    Get_System_ಸಮಯ_As_File_ಸಮಯ(&now);

    ಸ್ಥಿರಾಂಕ FILETIME* atime = ಖಾಲಿ;
    ಸ್ಥಿರಾಂಕ FILETIME* mtime = ಖಾಲಿ;

    ಆದರೆ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    ಆದರೆ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    ಆದರೆ (!ಗಣ_File_ಸಮಯ(handle, ಖಾಲಿ, atime, mtime)) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_ಸ್ಥಾನಾಂತರ, ಪೂರ್ಣಾಂಕ i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_ಸ್ಥಾನಾಂತರ, i_whence);
    ಆದರೆ (ret < 0) {
        ಹಿಂತಿರುಗಿ errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

// Windows has neither a vectored nor a positional transfer, so the list is walked
// one buffer at a time and a short transfer ends it, as it would for readv. A
// positional one moves the file pointer and puts it back where it was.
ಸ್ಥಿರ
__wasi_errno_t transfer_iovs (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                              __wasi_size_t i_iovsLen, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ,
                              bool i_positional, bool i_write, __wasi_size_t* o_ಮುಗಿದ)
{
    *o_ಮುಗಿದ = 0;

    int64_t saved = 0;

    ಆದರೆ (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        ಆದರೆ (saved < 0 || _lseeki64(i_fd, (__int64)i_ಸ್ಥಾನಾಂತರ, SEEK_SET) < 0) {
            ಹಿಂತಿರುಗಿ errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         ಮುಗಿದ = 0;

    ಗಾಗಿ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ಶೂನ್ಯ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ಆದರೆ (len == 0) {
            ಮುಂದುವರಿ;
        }

        ಪೂರ್ಣಾಂಕ ret = i_write ? _write(i_fd, addr, (ಚಿಹ್ನೆರಹಿತ)len) : _read(i_fd, addr, (ಚಿಹ್ನೆರಹಿತ)len);
        ಆದರೆ (ret < 0) {
            err = errno_from_crt(errno);
            ನಿಲ್ಲಿಸು;
        }

        ಮುಗಿದ += (size_t)ret;
        ಆದರೆ ((size_t)ret < len) {
            ನಿಲ್ಲಿಸು;
        }
    }

    ಆದರೆ (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    *o_ಮುಗಿದ = (__wasi_size_t)ಮುಗಿದ;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_ಮುಗಿದ);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_ಮುಗಿದ);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ,
                                      __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_iovs(i_fd, i_iovs, i_iovsLen, i_ಸ್ಥಾನಾಂತರ, true, false, o_ಮುಗಿದ);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ,
                                       __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_iovs(i_fd, i_iovs, i_iovsLen, i_ಸ್ಥಾನಾಂತರ, true, true, o_ಮುಗಿದ);
}


/*
 * Paths
 */

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_open (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    ಅಕ್ಷರ           ತುಂಬಿದ_ಮಾರ್ಗ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_ಮಾರ್ಗ, i_dirfd, i_ಮಾರ್ಗ);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    ಆದರೆ (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_ಮಾರ್ಗ_handle(ತುಂಬಿದ_ಮಾರ್ಗ, true, &handle);
        ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
            ಹಿಂತಿರುಗಿ err;
        }

        ಪೂರ್ಣಾಂಕ dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        ಆದರೆ (dir_fd < 0) {
            CloseHandle(handle);
            ಹಿಂತಿರುಗಿ __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    ಪೂರ್ಣಾಂಕ flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                O_ದ್ವಿಮಾನ;

    ಆದರೆ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    ಪೂರ್ಣಾಂಕ host_fd = _open(ತುಂಬಿದ_ಮಾರ್ಗ, flags, _S_IREAD | _S_IWRITE);
    ಆದರೆ (host_fd < 0) {
        ಹಿಂತಿರುಗಿ errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ಗಾತ್ರ(*o_stat));

    ಅಕ್ಷರ           ತುಂಬಿದ_ಮಾರ್ಗ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_ಮಾರ್ಗ, i_dirfd, i_ಮಾರ್ಗ);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    HANDLE handle;
    err = open_ಮಾರ್ಗ_handle(ತುಂಬಿದ_ಮಾರ್ಗ, i_follow, &handle);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    ಹಿಂತಿರುಗಿ err;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಅಕ್ಷರ           ತುಂಬಿದ_ಮಾರ್ಗ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_ಮಾರ್ಗ, i_dirfd, i_ಮಾರ್ಗ);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    ಆದರೆ (!CreateDirectoryA(ತುಂಬಿದ_ಮಾರ್ಗ, ಖಾಲಿ)) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಅಕ್ಷರ           ತುಂಬಿದ_ಮಾರ್ಗ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_ಮಾರ್ಗ, i_dirfd, i_ಮಾರ್ಗ);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    ಆದರೆ (!RemoveDirectoryA(ತುಂಬಿದ_ಮಾರ್ಗ)) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಅಕ್ಷರ           ತುಂಬಿದ_ಮಾರ್ಗ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_ಮಾರ್ಗ, i_dirfd, i_ಮಾರ್ಗ);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = Get_File_ಗುಣಲಕ್ಷಣಗಳು_A(ತುಂಬಿದ_ಮಾರ್ಗ);
    bool  isDirLink =
      attrs != INVALID_FILE_ಗುಣಲಕ್ಷಣಗಳು &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_ಬಿಂದು)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_ಬಿಂದು);

    ಆದರೆ (!(isDirLink ? RemoveDirectoryA(ತುಂಬಿದ_ಮಾರ್ಗ) : DeleteFileA(ತುಂಬಿದ_ಮಾರ್ಗ))) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ಮಾರ್ಗ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_ಮಾರ್ಗ)
{
    ಅಕ್ಷರ           ತುಂಬಿದ_old[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    ಅಕ್ಷರ           ತುಂಬಿದ_new[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_old, i_oldDirfd, i_old_ಮಾರ್ಗ);
    ಆದರೆ (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_new, i_newDirfd, i_new_ಮಾರ್ಗ);
    }
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    ಆದರೆ (!MoveFileExA(ತುಂಬಿದ_old, ತುಂಬಿದ_new, MOVEFILE_REPLACE_EXISTING)) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ಮಾರ್ಗ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_ಮಾರ್ಗ)
{
    ಅಕ್ಷರ           ತುಂಬಿದ_old[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    ಅಕ್ಷರ           ತುಂಬಿದ_new[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_old, i_oldDirfd, i_old_ಮಾರ್ಗ);
    ಆದರೆ (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_new, i_newDirfd, i_new_ಮಾರ್ಗ);
    }
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    ಆದರೆ (!CreateHardLinkA(ತುಂಬಿದ_new, ತುಂಬಿದ_old, ಖಾಲಿ)) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_symlink (ccstr_t i_ಗುರಿ, __wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಅಕ್ಷರ           ತುಂಬಿದ_new[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_new, i_dirfd, i_ಮಾರ್ಗ);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    ಅಕ್ಷರ   ಗುರಿ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    size_t dir_len = 0;
    ಗಾಗಿ (size_t i = 0; ತುಂಬಿದ_new[i]; i++) {
        ಆದರೆ (ತುಂಬಿದ_new[i] == '/' || ತುಂಬಿದ_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    ಪೂರ್ಣಾಂಕ ಗುರಿ_len = snprintf(ಗುರಿ, ಗಾತ್ರ(ಗುರಿ), "%.*s%s", (ಪೂರ್ಣಾಂಕ)dir_len, ತುಂಬಿದ_new, i_ಗುರಿ);
    ಆದರೆ (ಗುರಿ_len < 0 || ಗುರಿ_len >= (ಪೂರ್ಣಾಂಕ)ಗಾತ್ರ(ಗುರಿ)) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = Get_File_ಗುಣಲಕ್ಷಣಗಳು_A(ಗುರಿ);
    ಆದರೆ (attrs != INVALID_FILE_ಗುಣಲಕ್ಷಣಗಳು && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    ಆದರೆ (!CreateSymbolicLinkA(ತುಂಬಿದ_new, i_ಗುರಿ,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        ಆದರೆ (Get_ಕೊನೆಯ_Error() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(ತುಂಬಿದ_new, i_ಗುರಿ, flags)) {
            ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
        }
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
ಸ್ಥಿರ
__wasi_errno_t read_link_ಗುರಿ (ccstr_t i_ಮಾರ್ಗ, ಅಕ್ಷರ* o_ಗುರಿ, ಪೂರ್ಣಾಂಕ i_ಗಾತ್ರ, ಪೂರ್ಣಾಂಕ* o_len)
{
    HANDLE handle = CreateFileA(i_ಮಾರ್ಗ, FILE_READ_ಗುಣಲಕ್ಷಣಗಳು,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ಖಾಲಿ,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_ಬಿಂದು, ಖಾಲಿ);
    ಆದರೆ (handle == INVALID_HANDLE_ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ errno_from_win32(Get_ಕೊನೆಯ_Error());
    }

    Reparse_ದತ್ತಾಂಶ_ಬಫರ್* reparse =
      (Reparse_ದತ್ತಾಂಶ_ಬಫರ್*)m3_Malloc("ReparseDataBuffer", ಗರಿಷ್ಠ_REPARSE_ದತ್ತಾಂಶ_ಬಫರ್_ಗಾತ್ರ);
    ಆದರೆ (!reparse) {
        CloseHandle(handle);
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    ಆದರೆ (!DeviceIoControl(handle, FSCTL_GET_REPARSE_ಬಿಂದು, ಖಾಲಿ, 0, reparse,
                         ಗರಿಷ್ಠ_REPARSE_ದತ್ತಾಂಶ_ಬಫರ್_ಗಾತ್ರ, &returned, ಖಾಲಿ)) {
        err = errno_from_win32(Get_ಕೊನೆಯ_Error());
    }
    CloseHandle(handle);

    ಆದರೆ (err == __WASI_ERRNO_SUCCESS) {
        ಸ್ಥಿರಾಂಕ WCHAR* ಹೆಸರುಗಳು;
        ಆದರೆ (reparse->Reparse_ಟ್ಯಾಗ್ == IO_REPARSE_ಟ್ಯಾಗ್_SYMLINK) {
            ಹೆಸರುಗಳು = reparse->u.symlink.ಮಾರ್ಗ_ಬಫರ್;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (reparse->Reparse_ಟ್ಯಾಗ್ == IO_REPARSE_ಟ್ಯಾಗ್_MOUNT_ಬಿಂದು) {
            ಹೆಸರುಗಳು = reparse->u.mount_ಬಿಂದು.ಮಾರ್ಗ_ಬಫರ್;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹೆಸರುಗಳು = ಖಾಲಿ;
            err = __WASI_ERRNO_INVAL;
        }

        ಆದರೆ (ಹೆಸರುಗಳು) {
            ಸ್ಥಿರಾಂಕ WCHAR* name = ಹೆಸರುಗಳು + reparse->Print_Name_ಸ್ಥಾನಾಂತರ / ಗಾತ್ರ(WCHAR);
            ಪೂರ್ಣಾಂಕ          len = reparse->Print_Name_ಉದ್ದಳತೆ / ಗಾತ್ರ(WCHAR);

            ಆದರೆ (len == 0) {
                name = ಹೆಸರುಗಳು + reparse->Substitute_Name_ಸ್ಥಾನಾಂತರ / ಗಾತ್ರ(WCHAR);
                len = reparse->Substitute_Name_ಉದ್ದಳತೆ / ಗಾತ್ರ(WCHAR);
                ಆದರೆ (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = Wide_Char_ಗಮ್ಯ_Multi_Byte(CP_UTF8, 0, name, len, o_ಗುರಿ, i_ಗಾತ್ರ, ಖಾಲಿ, ಖಾಲಿ);
            ಆದರೆ (*o_len <= 0 && len != 0) {
                err = errno_from_win32(Get_ಕೊನೆಯ_Error());
            }
        }
    }

    m3_Free(reparse);
    ಹಿಂತಿರುಗಿ err;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ, ಅಕ್ಷರ* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_ಬಳಸಿದ)
{
    *o_buf_ಬಳಸಿದ = 0;

    ಅಕ್ಷರ           ತುಂಬಿದ_ಮಾರ್ಗ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    __wasi_errno_t err = resolve_ಮಾರ್ಗ(ತುಂಬಿದ_ಮಾರ್ಗ, i_dirfd, i_ಮಾರ್ಗ);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    ಅಕ್ಷರ ಗುರಿ[d_m3_Wasi_Max_Host_ಮಾರ್ಗ];
    ಪೂರ್ಣಾಂಕ  ಗುರಿ_len = 0;

    err = read_link_ಗುರಿ(ತುಂಬಿದ_ಮಾರ್ಗ, ಗುರಿ, (ಪೂರ್ಣಾಂಕ)ಗಾತ್ರ(ಗುರಿ), &ಗುರಿ_len);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)ಗುರಿ_len, i_bufLen);
    ಸ್ಮರಣೆ_ನಕಲು(o_buf, ಗುರಿ, copied);
    *o_buf_ಬಳಸಿದ = copied;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

// Windows offers the one clock, so every WASI clock maps onto it and none of them is
// rejected
ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    // a FILETIME counts in 100ns, but the system clock behind it moves on the timer
    // tick, so this says a millisecond rather than what it can spell
    *o_res = 1000000;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
ಸ್ಥಿರ
__wasi_timestamp_t now_ns (ಶೂನ್ಯ)
{
    FILETIME now;
    Get_System_ಸಮಯ_As_File_ಸಮಯ(&now);

    ಹಿಂತಿರುಗಿ convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_ಸಮಯ_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_ಸಮಯ)
{
    *o_ಸಮಯ = now_ns();
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_random_get (ಶೂನ್ಯ* o_buf, __wasi_size_t i_len)
{
    ಆದರೆ (!RtlGenRandom(o_buf, i_len)) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_IO;
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
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
#ವ್ಯಾಖ್ಯಾನಿಸು d_m3WasiPollIntervalMs  5

// Whether a descriptor has what its subscription is waiting for. Everything that is
// not a stream is ready by definition: the spec says a regular file always triggers
// both read and write, and so does anything whose kind Windows will not describe.
ಸ್ಥಿರ
__wasi_errno_t poll_fd_ready (ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* i_sub, bool* o_ready,
                              __wasi_filesize_t* o_nbytes, __wasi_eventrwflags_t* o_flags)
{
    *o_ready = false;
    *o_nbytes = 0;
    *o_flags = 0;

    HANDLE         handle;

    __wasi_errno_t err = handle_for_fd(i_sub->fd, &handle);
    ಆದರೆ (err != __WASI_ERRNO_SUCCESS) {
        ಹಿಂತಿರುಗಿ err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    ಆದರೆ (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    }

    ಆದರೆ (type == FILE_TYPE_PIPE) {
        DWORD ಕಾಯುವಿಕೆ = 0;

        ಆದರೆ (!PeekNamedPipe(handle, ಖಾಲಿ, 0, ಖಾಲಿ, &ಕಾಯುವಿಕೆ, ಖಾಲಿ)) {
            DWORD ಕೊನೆಯ = Get_ಕೊನೆಯ_Error();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            ಆದರೆ (ಕೊನೆಯ == ERROR_BROKEN_PIPE or ಕೊನೆಯ == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
            }
            ಹಿಂತಿರುಗಿ errno_from_win32(ಕೊನೆಯ);
        }

        *o_ready = (ಕಾಯುವಿಕೆ != 0);
        *o_nbytes = ಕಾಯುವಿಕೆ;
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    }

    ಆದರೆ (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock and ignores i_elapsedNs, the clock having moved on by itself.
// Every WASI clock is the system clock here - see clock_time_get - so an absolute
// deadline is held against that one whichever clock it names.
ಸ್ಥಿರ
uint64_t poll_ಉಳಿದ_ns (ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ಆದರೆ (!i_sub->isAbsolute) {
        ಹಿಂತಿರುಗಿ (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    ಹಿಂತಿರುಗಿ (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_poll_oneoff (ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* i_subs, __wasi_size_t i_ಎಣಿಕೆ,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_ಎಣಿಕೆ)
{
    *o_event_ಎಣಿಕೆ = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    ಗಾಗಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಆದರೆ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_ಉಳಿದ_ns(&i_subs[i], 0));
            hasClock = true;
        } ಇಲ್ಲದಿದ್ದರೆ {
            hasFd = true;
        }
    }

    ಆದರೆ (!hasClock and !hasFd) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t ಆರಂಭ_Ns = now_ns();

    ಗಾಗಿ (;;) {
        __wasi_size_t ಹೊರಹೋಗು = 0;

        ಗಾಗಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
            ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* sub = &i_subs[i];

            ಆದರೆ (sub->type == __WASI_EVENTTYPE_CLOCK) {
                ಮುಂದುವರಿ;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            ಆದರೆ (err == __WASI_ERRNO_SUCCESS and !ready) {
                ಮುಂದುವರಿ;
            }

            o_events[ಹೊರಹೋಗು].userdata = sub->userdata;
            o_events[ಹೊರಹೋಗು].error = err;
            o_events[ಹೊರಹೋಗು].type = sub->type;
            o_events[ಹೊರಹೋಗು].nbytes = nbytes;
            o_events[ಹೊರಹೋಗು].flags = flags;
            ಹೊರಹೋಗು++;
        }

        // This is a wall clock and can be set backwards under us, which is not
        // elapsed time going negative but the measurement being no longer usable;
        // starting the count again from here is the honest reading of it.
        ಸ್ಥಿರಾಂಕ uint64_t t = now_ns();
        ಸ್ಥಿರಾಂಕ uint64_t elapsedNs = (t > ಆರಂಭ_Ns) ? t - ಆರಂಭ_Ns : 0;

        ಆದರೆ (t < ಆರಂಭ_Ns) {
            ಆರಂಭ_Ns = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        ಆದರೆ (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            ಗಾಗಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
                ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* sub = &i_subs[i];

                ಆದರೆ (sub->type == __WASI_EVENTTYPE_CLOCK and
                    poll_ಉಳಿದ_ns(sub, elapsedNs) == 0) {
                    o_events[ಹೊರಹೋಗು].userdata = sub->userdata;
                    o_events[ಹೊರಹೋಗು].error = __WASI_ERRNO_SUCCESS;
                    o_events[ಹೊರಹೋಗು].type = __WASI_EVENTTYPE_CLOCK;
                    o_events[ಹೊರಹೋಗು].nbytes = 0;
                    o_events[ಹೊರಹೋಗು].flags = 0;
                    ಹೊರಹೋಗು++;
                }
            }
        }

        ಆದರೆ (ಹೊರಹೋಗು) {
            *o_event_ಎಣಿಕೆ = ಹೊರಹೋಗು;
            ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        ಆದರೆ (hasClock) {
            uint64_t ಉಳಿದ_Ns = soonestNs - elapsedNs;
            uint64_t ಉಳಿದ_Ms = (ಉಳಿದ_Ns + 999999) / 1000000;

            ಆದರೆ (not hasFd or ಉಳಿದ_Ms < sleepMs) {
                sleepMs = (ಉಳಿದ_Ms > MAXDWORD) ? MAXDWORD : (DWORD)ಉಳಿದ_Ms;
            }
        }

        Sleep(sleepMs);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // m3_api_wasi_win32_h
