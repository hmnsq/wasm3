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

#నిర్వచితం_కాకపోతే m3_api_wasi_win32_h
#నిర్వచించు m3_api_wasi_win32_h

#అన్తర్భావ "m3_core.h"

#ఒకవేళ !defined(_WIN32)
#  error "This file is for Windows only"
#అయితే_ముగింపు

#అన్తర్భావ <Windows.h>
#అన్తర్భావ <errno.h>
#అన్తర్భావ <fcntl.h>
#అన్తర్భావ <io.h>
#అన్తర్భావ <stdio.h>
#అన్తర్భావ <string.h>
#అన్తర్భావ <sys/stat.h>
#అన్తర్భావ <wchar.h>

// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#నిర్వచించు SystemFunction036 NTAPI SystemFunction036
#అన్తర్భావ <NTSecAPI.h>
#నిర్వచనం_రద్దు SystemFunction036

#అన్తర్భావ "m3_api_wasi_host.h"

// Room for a preopen's real path in front of the guest's
#నిర్వచించు d_m3_Wasi_Max_Host_మార్గం  (d_m3_Wasi_Max_మార్గం + 16)

// 1601-01-01, where a FILETIME counts its 100ns ticks from, in those same ticks
#నిర్వచించు d_m3_Win_Epoch_స్థానభ్రంశం   116444736000000000ULL

// Only the Windows 10 SDK and later declare it
#ఒకవేళ !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#అయితే_ముగింపు

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#ఒకవేళ !defined(FSCTL_GET_REPARSE_బిందువు)
#  define FSCTL_GET_REPARSE_బిందువు  0x000900A8
#అయితే_ముగింపు
#ఒకవేళ !defined(గరిష్ఠ_REPARSE_దత్తాంశం_బఫరు_కొలత)
#  define గరిష్ఠ_REPARSE_దత్తాంశం_బఫరు_కొలత  (16 * 1024)
#అయితే_ముగింపు

రకనిర్వచనం నిర్మాణం Reparse_దత్తాంశం_బఫరు {
    ULONG  Reparse_ట్యాగ్;
    USHORT Reparse_దత్తాంశం_నిడివి;
    USHORT Reserved;
    USHORT Substitute_Name_స్థానభ్రంశం;
    USHORT Substitute_Name_నిడివి;
    USHORT Print_Name_స్థానభ్రంశం;
    USHORT Print_Name_నిడివి;
    సంఘం {
        నిర్మాణం {
            ULONG Flags;
            WCHAR మార్గం_బఫరు[1];
        } symlink;
        నిర్మాణం {
            WCHAR మార్గం_బఫరు[1];
        } mount_బిందువు;
    } u;
} Reparse_దత్తాంశం_బఫరు;


/*
 * Plumbing
 */

// What the CRT calls used here leave in errno. Everything the Win32 API reports goes
// through errno_from_win32 instead.
స్థిర
__wasi_errno_t errno_from_crt (పూర్ణాంకం err)
{
    // clang-format off
    ఎంపిక (err) {
    స్థితి 0:       తిరిగి __WASI_ERRNO_SUCCESS;
    స్థితి EACCES:  తిరిగి __WASI_ERRNO_ACCES;
    స్థితి EBADF:   తిరిగి __WASI_ERRNO_BADF;
    స్థితి EEXIST:  తిరిగి __WASI_ERRNO_EXIST;
    స్థితి EMFILE:  తిరిగి __WASI_ERRNO_MFILE;
    స్థితి ENOENT:  తిరిగి __WASI_ERRNO_NOENT;
    స్థితి ENOSPC:  తిరిగి __WASI_ERRNO_NOSPC;
    స్థితి EINVAL:  తిరిగి __WASI_ERRNO_INVAL;
    }
    // clang-format on
    తిరిగి __WASI_ERRNO_IO;
}

స్థిర
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    ఎంపిక (err) {
    స్థితి ERROR_SUCCESS:               తిరిగి __WASI_ERRNO_SUCCESS;
    స్థితి ERROR_INVALID_FUNCTION:      తిరిగి __WASI_ERRNO_NOTSUP;
    స్థితి ERROR_FILE_NOT_దొరికింది:
    స్థితి ERROR_మార్గం_NOT_దొరికింది:
    స్థితి ERROR_INVALID_DRIVE:
    స్థితి ERROR_BAD_NETPATH:
    స్థితి ERROR_BAD_PATHNAME:          తిరిగి __WASI_ERRNO_NOENT;
    స్థితి ERROR_TOO_MANY_OPEN_FILES:   తిరిగి __WASI_ERRNO_MFILE;
    స్థితి ERROR_ACCESS_DENIED:
    స్థితి ERROR_WRITE_PROTECT:         తిరిగి __WASI_ERRNO_ACCES;
    స్థితి ERROR_INVALID_HANDLE:        తిరిగి __WASI_ERRNO_BADF;
    స్థితి ERROR_NOT_ENOUGH_జ్ఞాపకం:
    స్థితి ERROR_OUTOFMEMORY:           తిరిగి __WASI_ERRNO_NOMEM;
    స్థితి ERROR_NOT_SAME_DEVICE:       తిరిగి __WASI_ERRNO_XDEV;
    స్థితి ERROR_WRITE_FAULT:
    స్థితి ERROR_READ_FAULT:            తిరిగి __WASI_ERRNO_IO;
    స్థితి ERROR_SHARING_VIOLATION:
    స్థితి ERROR_తాళం_VIOLATION:        తిరిగి __WASI_ERRNO_BUSY;
    స్థితి ERROR_HANDLE_DISK_నిండిన:
    స్థితి ERROR_DISK_నిండిన:             తిరిగి __WASI_ERRNO_NOSPC;
    స్థితి ERROR_FILE_EXISTS:
    స్థితి ERROR_ALREADY_EXISTS:        తిరిగి __WASI_ERRNO_EXIST;
    స్థితి ERROR_బఫరు_అధికప్రవాహం:
    స్థితి ERROR_INSUFFICIENT_బఫరు:
    స్థితి ERROR_FILENAME_EXCED_RANGE:  తిరిగి __WASI_ERRNO_NAMETOOLONG;
    స్థితి ERROR_DIR_NOT_ఖాళీలు:         తిరిగి __WASI_ERRNO_NOTEMPTY;
    స్థితి ERROR_DIRECTORY:             తిరిగి __WASI_ERRNO_NOTDIR;
    స్థితి ERROR_PRIVILEGE_NOT_HELD:    తిరిగి __WASI_ERRNO_PERM;
    స్థితి ERROR_NOT_SUPPORTED:         తిరిగి __WASI_ERRNO_NOTSUP;
    స్థితి ERROR_CANT_RESOLVE_FILENAME: తిరిగి __WASI_ERRNO_LOOP;
    స్థితి ERROR_NEGATIVE_SEEK:
    స్థితి ERROR_INVALID_NAME:
    స్థితి ERROR_INVALID_PARAMETER:
    స్థితి ERROR_NOT_A_REPARSE_బిందువు:   తిరిగి __WASI_ERRNO_INVAL;
    }
    // clang-format on
    తిరిగి __WASI_ERRNO_IO;
}

స్థిర
__wasi_timestamp_t convert_filetime (FILETIME i_సమయం)
{
    uint64_t ticks = ((uint64_t)i_సమయం.dw_ఎగువ_Date_సమయం << 32) | i_సమయం.dw_దిగువ_Date_సమయం;
    ఒకవేళ (ticks < d_m3_Win_Epoch_స్థానభ్రంశం) {
        తిరిగి 0;
    }
    తిరిగి (ticks - d_m3_Win_Epoch_స్థానభ్రంశం) * 100;
}

స్థిర
FILETIME convert_timestamp (__wasi_timestamp_t i_సమయం)
{
    uint64_t ticks = i_సమయం / 100 + d_m3_Win_Epoch_స్థానభ్రంశం;
    FILETIME బహిర్గతం;
    బహిర్గతం.dw_దిగువ_Date_సమయం = (DWORD)ticks;
    బహిర్గతం.dw_ఎగువ_Date_సమయం = (DWORD)(ticks >> 32);
    తిరిగి బహిర్గతం;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
స్థిర
bool is_preopen (__wasi_fd_t i_fd)
{
    తిరిగి i_fd >= 3 && i_fd < d_m3_Wasi_Preopen_లెక్క && m3_wasi_preopen[i_fd].fd == (పూర్ణాంకం)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
స్థిర
__wasi_errno_t resolve_మార్గం (అక్షరం* o_మార్గం, __wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    ఒకవేళ (!is_preopen(i_dirfd)) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    పూర్ణాంకం len = snprintf(o_మార్గం, d_m3_Wasi_Max_Host_మార్గం, "%s/%s", m3_wasi_preopen[i_dirfd].real_మార్గం,
                       i_మార్గం);
    ఒకవేళ (len < 0 || len >= d_m3_Wasi_Max_Host_మార్గం) {
        తిరిగి __WASI_ERRNO_NAMETOOLONG;
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
స్థిర
__wasi_errno_t open_మార్గం_handle (ccstr_t i_మార్గం, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    ఒకవేళ (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_బిందువు;
    }

    HANDLE handle = CreateFileA(i_మార్గం, FILE_READ_లక్షణాలు,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ఖాళీ,
                                OPEN_EXISTING, flags, ఖాళీ);
    ఒకవేళ (handle == INVALID_HANDLE_విలువ) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }

    *o_handle = handle;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    ఒకవేళ (handle == INVALID_HANDLE_విలువ) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
స్థిర
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, పరిమాణం(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    ఒకవేళ (!GetFileInformationByHandle(i_handle, &info)) {
        ఎంపిక (GetFileType(i_handle)) {
        స్థితి FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; ఆపు;
        స్థితి FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; ఆపు;
        అప్రమేయ: తిరిగి errno_from_win32(Get_చివరి_Error());
        }
        తిరిగి __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dw_Volume_Serial_సంఖ్య;
    o_stat->ino = ((uint64_t)info.n_File_సూచీ_ఎగువ << 32) | info.n_File_సూచీ_దిగువ;
    o_stat->nlink = info.n_సంఖ్య_యొక్క_Links;
    o_stat->కొలత = ((uint64_t)info.n_File_కొలత_ఎగువ << 32) | info.n_File_కొలత_దిగువ;
    o_stat->atim = convert_filetime(info.ft_చివరి_Access_సమయం);
    o_stat->mtim = convert_filetime(info.ft_చివరి_Write_సమయం);
    o_stat->ctim = convert_filetime(info.ft_Creation_సమయం);

    ఒకవేళ (info.dw_File_లక్షణాలు & FILE_ATTRIBUTE_REPARSE_బిందువు) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } లేకపోతే ఒకవేళ (info.dw_File_లక్షణాలు & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } లేకపోతే {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}


/*
 * Setup
 */

స్థిర
శూన్య m3_wasi_host_init ()
{
    _setmode(_fileno(stdin), O_ద్విమాన);
    _setmode(_fileno(stdout), O_ద్విమాన);
    _setmode(_fileno(stderr), O_ద్విమాన);

    // Windows cannot open a directory as a descriptor, so a preopen is resolved by
    // path instead. It still has to hold a descriptor number: the guest is handed
    // one, and without something occupying it the next file opened would be given
    // the same number and the two would be indistinguishable.
    కొరకు (పూర్ణాంకం i = 3; i < d_m3_Wasi_Preopen_లెక్క; i++) {
        ఒకవేళ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = _open("NUL", _O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

స్థిర
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ఒకవేళ (_close(i_fd) != 0) {
        తిరిగి errno_from_crt(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    ఒకవేళ (_commit(i_fd) != 0) {
        తిరిగి errno_from_crt(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_స్థానభ్రంశం,
                                       __wasi_filesize_t i_నిడివి, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    ఒకవేళ (i_advice > __WASI_ADVICE_NOREUSE) {
        తిరిగి __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    తిరిగి handle_for_fd(i_fd, &handle);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, పరిమాణం(*o_stat));

    ఒకవేళ (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        తిరిగి __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    తిరిగి filestat_from_handle(handle, o_stat);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// Nothing here can be changed after the fact: append and non-blocking are fixed when
// CreateFile runs, and the CRT hands back no way to revisit either. fd_fdstat above
// reports no flags at all for the same reason, so the only request that can be
// honoured is the one asking for exactly that.
స్థిర
__wasi_errno_t m3_wasi_host_fd_fdstat_సమితి_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    తిరిగి (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_సమితి_కొలత (__wasi_fd_t i_fd, __wasi_filesize_t i_కొలత)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    తిరిగి errno_from_crt(_chsize_s(i_fd, (__int64)i_కొలత));
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_సమితి_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    Get_System_సమయం_As_File_సమయం(&now);

    స్థిరరాశి FILETIME* atime = ఖాళీ;
    స్థిరరాశి FILETIME* mtime = ఖాళీ;

    ఒకవేళ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } లేకపోతే ఒకవేళ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    ఒకవేళ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } లేకపోతే ఒకవేళ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    ఒకవేళ (!సమితి_File_సమయం(handle, ఖాళీ, atime, mtime)) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_స్థానభ్రంశం, పూర్ణాంకం i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_స్థానభ్రంశం, i_whence);
    ఒకవేళ (ret < 0) {
        తిరిగి errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    తిరిగి __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

// Windows has neither a vectored nor a positional transfer, so the list is walked
// one buffer at a time and a short transfer ends it, as it would for readv. A
// positional one moves the file pointer and puts it back where it was.
స్థిర
__wasi_errno_t transfer_iovs (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                              __wasi_size_t i_iovsLen, __wasi_filesize_t i_స్థానభ్రంశం,
                              bool i_positional, bool i_write, __wasi_size_t* o_పూర్తి)
{
    *o_పూర్తి = 0;

    int64_t saved = 0;

    ఒకవేళ (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        ఒకవేళ (saved < 0 || _lseeki64(i_fd, (__int64)i_స్థానభ్రంశం, SEEK_SET) < 0) {
            తిరిగి errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         పూర్తి = 0;

    కొరకు (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        శూన్య*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ఒకవేళ (len == 0) {
            కొనసాగు;
        }

        పూర్ణాంకం ret = i_write ? _write(i_fd, addr, (గుర్తులేని)len) : _read(i_fd, addr, (గుర్తులేని)len);
        ఒకవేళ (ret < 0) {
            err = errno_from_crt(errno);
            ఆపు;
        }

        పూర్తి += (size_t)ret;
        ఒకవేళ ((size_t)ret < len) {
            ఆపు;
        }
    }

    ఒకవేళ (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    *o_పూర్తి = (__wasi_size_t)పూర్తి;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_పూర్తి);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_పూర్తి);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_స్థానభ్రంశం,
                                      __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_iovs(i_fd, i_iovs, i_iovsLen, i_స్థానభ్రంశం, true, false, o_పూర్తి);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_స్థానభ్రంశం,
                                       __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_iovs(i_fd, i_iovs, i_iovsLen, i_స్థానభ్రంశం, true, true, o_పూర్తి);
}


/*
 * Paths
 */

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_open (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    అక్షరం           నిండిన_మార్గం[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_మార్గం, i_dirfd, i_మార్గం);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    ఒకవేళ (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_మార్గం_handle(నిండిన_మార్గం, true, &handle);
        ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
            తిరిగి err;
        }

        పూర్ణాంకం dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        ఒకవేళ (dir_fd < 0) {
            CloseHandle(handle);
            తిరిగి __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        తిరిగి __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    పూర్ణాంకం flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                O_ద్విమాన;

    ఒకవేళ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } లేకపోతే ఒకవేళ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } లేకపోతే ఒకవేళ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    పూర్ణాంకం host_fd = _open(నిండిన_మార్గం, flags, _S_IREAD | _S_IWRITE);
    ఒకవేళ (host_fd < 0) {
        తిరిగి errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_filestat (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, పరిమాణం(*o_stat));

    అక్షరం           నిండిన_మార్గం[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_మార్గం, i_dirfd, i_మార్గం);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    HANDLE handle;
    err = open_మార్గం_handle(నిండిన_మార్గం, i_follow, &handle);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    తిరిగి err;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    అక్షరం           నిండిన_మార్గం[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_మార్గం, i_dirfd, i_మార్గం);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    ఒకవేళ (!CreateDirectoryA(నిండిన_మార్గం, ఖాళీ)) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    అక్షరం           నిండిన_మార్గం[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_మార్గం, i_dirfd, i_మార్గం);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    ఒకవేళ (!RemoveDirectoryA(నిండిన_మార్గం)) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    అక్షరం           నిండిన_మార్గం[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_మార్గం, i_dirfd, i_మార్గం);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = Get_File_లక్షణాలు_A(నిండిన_మార్గం);
    bool  isDirLink =
      attrs != INVALID_FILE_లక్షణాలు &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_బిందువు)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_బిందువు);

    ఒకవేళ (!(isDirLink ? RemoveDirectoryA(నిండిన_మార్గం) : DeleteFileA(నిండిన_మార్గం))) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_మార్గం,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_మార్గం)
{
    అక్షరం           నిండిన_old[d_m3_Wasi_Max_Host_మార్గం];
    అక్షరం           నిండిన_new[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_old, i_oldDirfd, i_old_మార్గం);
    ఒకవేళ (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_మార్గం(నిండిన_new, i_newDirfd, i_new_మార్గం);
    }
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    ఒకవేళ (!MoveFileExA(నిండిన_old, నిండిన_new, MOVEFILE_REPLACE_EXISTING)) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_మార్గం, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_మార్గం)
{
    అక్షరం           నిండిన_old[d_m3_Wasi_Max_Host_మార్గం];
    అక్షరం           నిండిన_new[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_old, i_oldDirfd, i_old_మార్గం);
    ఒకవేళ (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_మార్గం(నిండిన_new, i_newDirfd, i_new_మార్గం);
    }
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    ఒకవేళ (!CreateHardLinkA(నిండిన_new, నిండిన_old, ఖాళీ)) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_symlink (ccstr_t i_లక్ష్యం, __wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    అక్షరం           నిండిన_new[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_new, i_dirfd, i_మార్గం);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    అక్షరం   లక్ష్యం[d_m3_Wasi_Max_Host_మార్గం];
    size_t dir_len = 0;
    కొరకు (size_t i = 0; నిండిన_new[i]; i++) {
        ఒకవేళ (నిండిన_new[i] == '/' || నిండిన_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    పూర్ణాంకం లక్ష్యం_len = snprintf(లక్ష్యం, పరిమాణం(లక్ష్యం), "%.*s%s", (పూర్ణాంకం)dir_len, నిండిన_new, i_లక్ష్యం);
    ఒకవేళ (లక్ష్యం_len < 0 || లక్ష్యం_len >= (పూర్ణాంకం)పరిమాణం(లక్ష్యం)) {
        తిరిగి __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = Get_File_లక్షణాలు_A(లక్ష్యం);
    ఒకవేళ (attrs != INVALID_FILE_లక్షణాలు && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    ఒకవేళ (!CreateSymbolicLinkA(నిండిన_new, i_లక్ష్యం,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        ఒకవేళ (Get_చివరి_Error() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(నిండిన_new, i_లక్ష్యం, flags)) {
            తిరిగి errno_from_win32(Get_చివరి_Error());
        }
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
స్థిర
__wasi_errno_t read_link_లక్ష్యం (ccstr_t i_మార్గం, అక్షరం* o_లక్ష్యం, పూర్ణాంకం i_కొలత, పూర్ణాంకం* o_len)
{
    HANDLE handle = CreateFileA(i_మార్గం, FILE_READ_లక్షణాలు,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ఖాళీ,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_బిందువు, ఖాళీ);
    ఒకవేళ (handle == INVALID_HANDLE_విలువ) {
        తిరిగి errno_from_win32(Get_చివరి_Error());
    }

    Reparse_దత్తాంశం_బఫరు* reparse =
      (Reparse_దత్తాంశం_బఫరు*)m3_Malloc("ReparseDataBuffer", గరిష్ఠ_REPARSE_దత్తాంశం_బఫరు_కొలత);
    ఒకవేళ (!reparse) {
        CloseHandle(handle);
        తిరిగి __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    ఒకవేళ (!DeviceIoControl(handle, FSCTL_GET_REPARSE_బిందువు, ఖాళీ, 0, reparse,
                         గరిష్ఠ_REPARSE_దత్తాంశం_బఫరు_కొలత, &returned, ఖాళీ)) {
        err = errno_from_win32(Get_చివరి_Error());
    }
    CloseHandle(handle);

    ఒకవేళ (err == __WASI_ERRNO_SUCCESS) {
        స్థిరరాశి WCHAR* పేర్లు;
        ఒకవేళ (reparse->Reparse_ట్యాగ్ == IO_REPARSE_ట్యాగ్_SYMLINK) {
            పేర్లు = reparse->u.symlink.మార్గం_బఫరు;
        } లేకపోతే ఒకవేళ (reparse->Reparse_ట్యాగ్ == IO_REPARSE_ట్యాగ్_MOUNT_బిందువు) {
            పేర్లు = reparse->u.mount_బిందువు.మార్గం_బఫరు;
        } లేకపోతే {
            పేర్లు = ఖాళీ;
            err = __WASI_ERRNO_INVAL;
        }

        ఒకవేళ (పేర్లు) {
            స్థిరరాశి WCHAR* name = పేర్లు + reparse->Print_Name_స్థానభ్రంశం / పరిమాణం(WCHAR);
            పూర్ణాంకం          len = reparse->Print_Name_నిడివి / పరిమాణం(WCHAR);

            ఒకవేళ (len == 0) {
                name = పేర్లు + reparse->Substitute_Name_స్థానభ్రంశం / పరిమాణం(WCHAR);
                len = reparse->Substitute_Name_నిడివి / పరిమాణం(WCHAR);
                ఒకవేళ (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = Wide_Char_గమ్యం_Multi_Byte(CP_UTF8, 0, name, len, o_లక్ష్యం, i_కొలత, ఖాళీ, ఖాళీ);
            ఒకవేళ (*o_len <= 0 && len != 0) {
                err = errno_from_win32(Get_చివరి_Error());
            }
        }
    }

    m3_Free(reparse);
    తిరిగి err;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_readlink (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం, అక్షరం* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_వాడిన)
{
    *o_buf_వాడిన = 0;

    అక్షరం           నిండిన_మార్గం[d_m3_Wasi_Max_Host_మార్గం];
    __wasi_errno_t err = resolve_మార్గం(నిండిన_మార్గం, i_dirfd, i_మార్గం);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    అక్షరం లక్ష్యం[d_m3_Wasi_Max_Host_మార్గం];
    పూర్ణాంకం  లక్ష్యం_len = 0;

    err = read_link_లక్ష్యం(నిండిన_మార్గం, లక్ష్యం, (పూర్ణాంకం)పరిమాణం(లక్ష్యం), &లక్ష్యం_len);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)లక్ష్యం_len, i_bufLen);
    జ్ఞాపకం_నకలు(o_buf, లక్ష్యం, copied);
    *o_buf_వాడిన = copied;
    తిరిగి __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

// Windows offers the one clock, so every WASI clock maps onto it and none of them is
// rejected
స్థిర
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    // a FILETIME counts in 100ns, but the system clock behind it moves on the timer
    // tick, so this says a millisecond rather than what it can spell
    *o_res = 1000000;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
స్థిర
__wasi_timestamp_t now_ns (శూన్య)
{
    FILETIME now;
    Get_System_సమయం_As_File_సమయం(&now);

    తిరిగి convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_సమయం_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_సమయం)
{
    *o_సమయం = now_ns();
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_random_get (శూన్య* o_buf, __wasi_size_t i_len)
{
    ఒకవేళ (!RtlGenRandom(o_buf, i_len)) {
        తిరిగి __WASI_ERRNO_IO;
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
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
#నిర్వచించు d_m3WasiPollIntervalMs  5

// Whether a descriptor has what its subscription is waiting for. Everything that is
// not a stream is ready by definition: the spec says a regular file always triggers
// both read and write, and so does anything whose kind Windows will not describe.
స్థిర
__wasi_errno_t poll_fd_ready (స్థిరరాశి m3_wasi_pollsub_t* i_sub, bool* o_ready,
                              __wasi_filesize_t* o_nbytes, __wasi_eventrwflags_t* o_flags)
{
    *o_ready = false;
    *o_nbytes = 0;
    *o_flags = 0;

    HANDLE         handle;

    __wasi_errno_t err = handle_for_fd(i_sub->fd, &handle);
    ఒకవేళ (err != __WASI_ERRNO_SUCCESS) {
        తిరిగి err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    ఒకవేళ (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        తిరిగి __WASI_ERRNO_SUCCESS;
    }

    ఒకవేళ (type == FILE_TYPE_PIPE) {
        DWORD నిరీక్షణ = 0;

        ఒకవేళ (!PeekNamedPipe(handle, ఖాళీ, 0, ఖాళీ, &నిరీక్షణ, ఖాళీ)) {
            DWORD చివరి = Get_చివరి_Error();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            ఒకవేళ (చివరి == ERROR_BROKEN_PIPE or చివరి == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                తిరిగి __WASI_ERRNO_SUCCESS;
            }
            తిరిగి errno_from_win32(చివరి);
        }

        *o_ready = (నిరీక్షణ != 0);
        *o_nbytes = నిరీక్షణ;
        తిరిగి __WASI_ERRNO_SUCCESS;
    }

    ఒకవేళ (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        తిరిగి __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock and ignores i_elapsedNs, the clock having moved on by itself.
// Every WASI clock is the system clock here - see clock_time_get - so an absolute
// deadline is held against that one whichever clock it names.
స్థిర
uint64_t poll_మిగిలిన_ns (స్థిరరాశి m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ఒకవేళ (!i_sub->isAbsolute) {
        తిరిగి (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    తిరిగి (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

స్థిర
__wasi_errno_t m3_wasi_host_poll_oneoff (స్థిరరాశి m3_wasi_pollsub_t* i_subs, __wasi_size_t i_లెక్క,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_లెక్క)
{
    *o_event_లెక్క = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
        ఒకవేళ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_మిగిలిన_ns(&i_subs[i], 0));
            hasClock = true;
        } లేకపోతే {
            hasFd = true;
        }
    }

    ఒకవేళ (!hasClock and !hasFd) {
        తిరిగి __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t ప్రారంభం_Ns = now_ns();

    కొరకు (;;) {
        __wasi_size_t బహిర్గతం = 0;

        కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
            స్థిరరాశి m3_wasi_pollsub_t* sub = &i_subs[i];

            ఒకవేళ (sub->type == __WASI_EVENTTYPE_CLOCK) {
                కొనసాగు;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            ఒకవేళ (err == __WASI_ERRNO_SUCCESS and !ready) {
                కొనసాగు;
            }

            o_events[బహిర్గతం].userdata = sub->userdata;
            o_events[బహిర్గతం].error = err;
            o_events[బహిర్గతం].type = sub->type;
            o_events[బహిర్గతం].nbytes = nbytes;
            o_events[బహిర్గతం].flags = flags;
            బహిర్గతం++;
        }

        // This is a wall clock and can be set backwards under us, which is not
        // elapsed time going negative but the measurement being no longer usable;
        // starting the count again from here is the honest reading of it.
        స్థిరరాశి uint64_t t = now_ns();
        స్థిరరాశి uint64_t elapsedNs = (t > ప్రారంభం_Ns) ? t - ప్రారంభం_Ns : 0;

        ఒకవేళ (t < ప్రారంభం_Ns) {
            ప్రారంభం_Ns = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        ఒకవేళ (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
                స్థిరరాశి m3_wasi_pollsub_t* sub = &i_subs[i];

                ఒకవేళ (sub->type == __WASI_EVENTTYPE_CLOCK and
                    poll_మిగిలిన_ns(sub, elapsedNs) == 0) {
                    o_events[బహిర్గతం].userdata = sub->userdata;
                    o_events[బహిర్గతం].error = __WASI_ERRNO_SUCCESS;
                    o_events[బహిర్గతం].type = __WASI_EVENTTYPE_CLOCK;
                    o_events[బహిర్గతం].nbytes = 0;
                    o_events[బహిర్గతం].flags = 0;
                    బహిర్గతం++;
                }
            }
        }

        ఒకవేళ (బహిర్గతం) {
            *o_event_లెక్క = బహిర్గతం;
            తిరిగి __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        ఒకవేళ (hasClock) {
            uint64_t మిగిలిన_Ns = soonestNs - elapsedNs;
            uint64_t మిగిలిన_Ms = (మిగిలిన_Ns + 999999) / 1000000;

            ఒకవేళ (not hasFd or మిగిలిన_Ms < sleepMs) {
                sleepMs = (మిగిలిన_Ms > MAXDWORD) ? MAXDWORD : (DWORD)మిగిలిన_Ms;
            }
        }

        Sleep(sleepMs);
    }
}

#అయితే_ముగింపు // m3_api_wasi_win32_h
