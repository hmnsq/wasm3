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

#यदि_अपरिभाषितम् m3_api_wasi_win32_h
#परिभाषय m3_api_wasi_win32_h

#अन्तर्भाव "m3_core.h"

#यदि !defined(_WIN32)
#  error "This file is for Windows only"
#यदि_अन्तः

#अन्तर्भाव <Windows.h>
#अन्तर्भाव <errno.h>
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <io.h>
#अन्तर्भाव <stdio.h>
#अन्तर्भाव <string.h>
#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <wchar.h>

// See http://msdn.microsoft.com/en-us/library/windows/desktop/aa387694.aspx
#परिभाषय SystemFunction036 NTAPI SystemFunction036
#अन्तर्भाव <NTSecAPI.h>
#अपरिभाषय SystemFunction036

#अन्तर्भाव "m3_api_wasi_host.h"

// Room for a preopen's real path in front of the guest's
#परिभाषय d_m3_Wasi_Max_Host_मार्गः  (d_m3_Wasi_Max_मार्गः + 16)

// 1601-01-01, where a FILETIME counts its 100ns ticks from, in those same ticks
#परिभाषय d_m3_Win_Epoch_विस्थापनम्   116444736000000000ULL

// Only the Windows 10 SDK and later declare it
#यदि !defined(SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)
#  define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE  0x2
#यदि_अन्तः

// A reparse point is read through an ioctl whose reply is described in ntifs.h,
// which does not ship with the compiler, so the layout is spelled out below. The two
// tags that name a path differ only in the symbolic link's extra Flags word.
#यदि !defined(FSCTL_GET_REPARSE_POINT)
#  define FSCTL_GET_REPARSE_POINT  0x000900A8
#यदि_अन्तः
#यदि !defined(अधिकतमम्_REPARSE_दत्त_बफर_आकार)
#  define अधिकतमम्_REPARSE_दत्त_बफर_आकार  (16 * 1024)
#यदि_अन्तः

प्रकारनिर्देश संरचना Reparse_दत्त_बफर {
    ULONG  Reparse_चिह्नकम्;
    USHORT Reparse_दत्त_दीर्घता;
    USHORT Reserved;
    USHORT Substitute_Name_विस्थापनम्;
    USHORT Substitute_Name_दीर्घता;
    USHORT Print_Name_विस्थापनम्;
    USHORT Print_Name_दीर्घता;
    सङ्घ {
        संरचना {
            ULONG Flags;
            WCHAR मार्गः_बफर[1];
        } symlink;
        संरचना {
            WCHAR मार्गः_बफर[1];
        } mount_point;
    } u;
} Reparse_दत्त_बफर;


/*
 * Plumbing
 */

// What the CRT calls used here leave in errno. Everything the Win32 API reports goes
// through errno_from_win32 instead.
स्थिर
__wasi_errno_t errno_from_crt (पूर्णाङ्क err)
{
    // clang-format off
    विकल्प (err) {
    पक्ष 0:       प्रत्यागम __WASI_ERRNO_SUCCESS;
    पक्ष EACCES:  प्रत्यागम __WASI_ERRNO_ACCES;
    पक्ष EBADF:   प्रत्यागम __WASI_ERRNO_BADF;
    पक्ष EEXIST:  प्रत्यागम __WASI_ERRNO_EXIST;
    पक्ष EMFILE:  प्रत्यागम __WASI_ERRNO_MFILE;
    पक्ष ENOENT:  प्रत्यागम __WASI_ERRNO_NOENT;
    पक्ष ENOSPC:  प्रत्यागम __WASI_ERRNO_NOSPC;
    पक्ष EINVAL:  प्रत्यागम __WASI_ERRNO_INVAL;
    }
    // clang-format on
    प्रत्यागम __WASI_ERRNO_IO;
}

स्थिर
__wasi_errno_t errno_from_win32 (DWORD err)
{
    // clang-format off
    विकल्प (err) {
    पक्ष ERROR_SUCCESS:               प्रत्यागम __WASI_ERRNO_SUCCESS;
    पक्ष ERROR_INVALID_FUNCTION:      प्रत्यागम __WASI_ERRNO_NOTSUP;
    पक्ष ERROR_FILE_NOT_प्राप्तम्:
    पक्ष ERROR_मार्गः_NOT_प्राप्तम्:
    पक्ष ERROR_INVALID_DRIVE:
    पक्ष ERROR_BAD_NETPATH:
    पक्ष ERROR_BAD_PATHNAME:          प्रत्यागम __WASI_ERRNO_NOENT;
    पक्ष ERROR_TOO_MANY_OPEN_FILES:   प्रत्यागम __WASI_ERRNO_MFILE;
    पक्ष ERROR_ACCESS_DENIED:
    पक्ष ERROR_WRITE_PROTECT:         प्रत्यागम __WASI_ERRNO_ACCES;
    पक्ष ERROR_INVALID_HANDLE:        प्रत्यागम __WASI_ERRNO_BADF;
    पक्ष ERROR_NOT_ENOUGH_स्मृतिः:
    पक्ष ERROR_OUTOFMEMORY:           प्रत्यागम __WASI_ERRNO_NOMEM;
    पक्ष ERROR_NOT_SAME_DEVICE:       प्रत्यागम __WASI_ERRNO_XDEV;
    पक्ष ERROR_WRITE_FAULT:
    पक्ष ERROR_READ_FAULT:            प्रत्यागम __WASI_ERRNO_IO;
    पक्ष ERROR_SHARING_VIOLATION:
    पक्ष ERROR_तालकम्_VIOLATION:        प्रत्यागम __WASI_ERRNO_BUSY;
    पक्ष ERROR_HANDLE_DISK_पूर्णम्:
    पक्ष ERROR_DISK_पूर्णम्:             प्रत्यागम __WASI_ERRNO_NOSPC;
    पक्ष ERROR_FILE_EXISTS:
    पक्ष ERROR_ALREADY_EXISTS:        प्रत्यागम __WASI_ERRNO_EXIST;
    पक्ष ERROR_बफर_अतिप्रवाहः:
    पक्ष ERROR_INSUFFICIENT_बफर:
    पक्ष ERROR_FILENAME_EXCED_RANGE:  प्रत्यागम __WASI_ERRNO_NAMETOOLONG;
    पक्ष ERROR_DIR_NOT_रिक्तानि:         प्रत्यागम __WASI_ERRNO_NOTEMPTY;
    पक्ष ERROR_DIRECTORY:             प्रत्यागम __WASI_ERRNO_NOTDIR;
    पक्ष ERROR_PRIVILEGE_NOT_HELD:    प्रत्यागम __WASI_ERRNO_PERM;
    पक्ष ERROR_NOT_SUPPORTED:         प्रत्यागम __WASI_ERRNO_NOTSUP;
    पक्ष ERROR_CANT_RESOLVE_FILENAME: प्रत्यागम __WASI_ERRNO_LOOP;
    पक्ष ERROR_NEGATIVE_SEEK:
    पक्ष ERROR_INVALID_NAME:
    पक्ष ERROR_INVALID_PARAMETER:
    पक्ष ERROR_NOT_A_REPARSE_POINT:   प्रत्यागम __WASI_ERRNO_INVAL;
    }
    // clang-format on
    प्रत्यागम __WASI_ERRNO_IO;
}

स्थिर
__wasi_timestamp_t convert_filetime (FILETIME i_time)
{
    uint64_t ticks = ((uint64_t)i_time.dw_उच्चः_Date_Time << 32) | i_time.dw_नीचः_Date_Time;
    यदि (ticks < d_m3_Win_Epoch_विस्थापनम्) {
        प्रत्यागम 0;
    }
    प्रत्यागम (ticks - d_m3_Win_Epoch_विस्थापनम्) * 100;
}

स्थिर
FILETIME convert_timestamp (__wasi_timestamp_t i_time)
{
    uint64_t ticks = i_time / 100 + d_m3_Win_Epoch_विस्थापनम्;
    FILETIME बहिर्गमनम्;
    बहिर्गमनम्.dw_नीचः_Date_Time = (DWORD)ticks;
    बहिर्गमनम्.dw_उच्चः_Date_Time = (DWORD)(ticks >> 32);
    प्रत्यागम बहिर्गमनम्;
}

// Whether i_fd is one of the preopened directories. A preopen holds a placeholder
// descriptor rather than an open directory, so it has to answer for itself wherever
// a stat is asked for, and it names a directory only while the placeholder opened
// for it still holds that number.
स्थिर
bool is_preopen (__wasi_fd_t i_fd)
{
    प्रत्यागम i_fd >= 3 && i_fd < d_m3_Wasi_Preopen_गणनम् && m3_wasi_preopen[i_fd].fd == (पूर्णाङ्क)i_fd;
}

// A preopened directory is resolved by prefixing its real path onto the guest's,
// which is what openat() would have resolved the pair to.
स्थिर
__wasi_errno_t resolve_मार्गः (वर्ण* o_मार्गः, __wasi_fd_t i_dirfd, ccstr_t i_मार्गः)
{
    यदि (!is_preopen(i_dirfd)) {
        प्रत्यागम __WASI_ERRNO_BADF;
    }

    पूर्णाङ्क len = snprintf(o_मार्गः, d_m3_Wasi_Max_Host_मार्गः, "%s/%s", m3_wasi_preopen[i_dirfd].real_मार्गः,
                       i_मार्गः);
    यदि (len < 0 || len >= d_m3_Wasi_Max_Host_मार्गः) {
        प्रत्यागम __WASI_ERRNO_NAMETOOLONG;
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

// FILE_FLAG_BACKUP_SEMANTICS is what lets a directory be opened at all, and
// FILE_READ_ATTRIBUTES is all the metadata calls need, so this opens whatever the
// path names without asking for access that could be refused.
स्थिर
__wasi_errno_t open_मार्गः_handle (ccstr_t i_मार्गः, bool i_follow, HANDLE* o_handle)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    यदि (!i_follow) {
        flags |= FILE_FLAG_OPEN_REPARSE_POINT;
    }

    HANDLE handle = CreateFileA(i_मार्गः, FILE_READ_गुणाः,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, रिक्त,
                                OPEN_EXISTING, flags, रिक्त);
    यदि (handle == INVALID_HANDLE_मानम्) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }

    *o_handle = handle;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t handle_for_fd (__wasi_fd_t i_fd, HANDLE* o_handle)
{
    HANDLE handle = (HANDLE)_get_osfhandle(i_fd);
    यदि (handle == INVALID_HANDLE_मानम्) {
        प्रत्यागम __WASI_ERRNO_BADF;
    }

    *o_handle = handle;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

// A handle that is not a file - a console, a pipe - carries none of this and answers
// with its type alone. Windows records no change time, so ctim is the creation time,
// the nearest thing it has.
स्थिर
__wasi_errno_t filestat_from_handle (HANDLE i_handle, m3_wasi_filestat_t* o_stat)
{
    स्मृतिं_पूरय(o_stat, 0, परिमाण(*o_stat));

    BY_HANDLE_FILE_INFORMATION info;
    यदि (!GetFileInformationByHandle(i_handle, &info)) {
        विकल्प (GetFileType(i_handle)) {
        पक्ष FILE_TYPE_CHAR: o_stat->filetype = __WASI_FILETYPE_CHARACTER_DEVICE; भङ्ग;
        पक्ष FILE_TYPE_PIPE: o_stat->filetype = __WASI_FILETYPE_UNKNOWN; भङ्ग;
        स्वभाव: प्रत्यागम errno_from_win32(Get_चरमः_Error());
        }
        प्रत्यागम __WASI_ERRNO_SUCCESS;
    }

    o_stat->dev = info.dw_Volume_Serial_सङ्ख्या;
    o_stat->ino = ((uint64_t)info.n_File_सूचकांक_उच्चः << 32) | info.n_File_सूचकांक_नीचः;
    o_stat->nlink = info.n_सङ्ख्या_सम्बन्धि_Links;
    o_stat->आकार = ((uint64_t)info.n_File_आकार_उच्चः << 32) | info.n_File_आकार_नीचः;
    o_stat->atim = convert_filetime(info.ft_चरमः_Access_Time);
    o_stat->mtim = convert_filetime(info.ft_चरमः_Write_Time);
    o_stat->ctim = convert_filetime(info.ftCreationTime);

    यदि (info.dw_File_गुणाः & FILE_ATTRIBUTE_REPARSE_POINT) {
        o_stat->filetype = __WASI_FILETYPE_SYMBOLIC_LINK;
    } अन्यथा यदि (info.dw_File_गुणाः & FILE_ATTRIBUTE_DIRECTORY) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
    } अन्यथा {
        o_stat->filetype = __WASI_FILETYPE_REGULAR_FILE;
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}


/*
 * Setup
 */

स्थिर
शून्य m3_wasi_host_init ()
{
    _setmode(_fileno(stdin), O_द्विमान);
    _setmode(_fileno(stdout), O_द्विमान);
    _setmode(_fileno(stderr), O_द्विमान);

    // Windows cannot open a directory as a descriptor, so a preopen is resolved by
    // path instead. It still has to hold a descriptor number: the guest is handed
    // one, and without something occupying it the next file opened would be given
    // the same number and the two would be indistinguishable.
    प्रति (पूर्णाङ्क i = 3; i < d_m3_Wasi_Preopen_गणनम्; i++) {
        यदि (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = _open("NUL", _O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

स्थिर
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    यदि (_close(i_fd) != 0) {
        प्रत्यागम errno_from_crt(errno);
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
    यदि (_commit(i_fd) != 0) {
        प्रत्यागम errno_from_crt(errno);
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_विस्थापनम्,
                                       __wasi_filesize_t i_दीर्घता, __wasi_advice_t i_advice)
{
    // Purely advisory, and Windows has nowhere to pass it on to, so dropping it is
    // still a conforming implementation. Only the fd has to be valid.
    यदि (i_advice > __WASI_ADVICE_NOREUSE) {
        प्रत्यागम __WASI_ERRNO_INVAL;
    }

    HANDLE handle;
    प्रत्यागम handle_for_fd(i_fd, &handle);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    स्मृतिं_पूरय(o_stat, 0, परिमाण(*o_stat));

    यदि (is_preopen(i_fd)) {
        o_stat->filetype = __WASI_FILETYPE_DIRECTORY;
        प्रत्यागम __WASI_ERRNO_SUCCESS;
    }

    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    प्रत्यागम filestat_from_handle(handle, o_stat);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    m3_wasi_filestat_t fd_stat;

    __wasi_errno_t     err = m3_wasi_host_fd_filestat(i_fd, &fd_stat);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    *o_filetype = fd_stat.filetype;
    *o_flags = 0; // the CRT does not report back the flags a descriptor was opened with
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

// Nothing here can be changed after the fact: append and non-blocking are fixed when
// CreateFile runs, and the CRT hands back no way to revisit either. fd_fdstat above
// reports no flags at all for the same reason, so the only request that can be
// honoured is the one asking for exactly that.
स्थिर
__wasi_errno_t m3_wasi_host_fd_fdstat_समुच्चयः_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    प्रत्यागम (i_flags == 0) ? __WASI_ERRNO_SUCCESS : __WASI_ERRNO_NOTSUP;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_समुच्चयः_आकार (__wasi_fd_t i_fd, __wasi_filesize_t i_आकार)
{
    // _chsize_s hands the error back rather than only leaving it in errno
    प्रत्यागम errno_from_crt(_chsize_s(i_fd, (__int64)i_आकार));
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_समुच्चयः_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    HANDLE         handle;
    __wasi_errno_t err = handle_for_fd(i_fd, &handle);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    // a time is either given, asked for as now, or left alone - and SetFileTime
    // leaves alone whatever it is handed no time for
    FILETIME now, times[2];
    GetSystemTimeAsFileTime(&now);

    ध्रुव FILETIME* atime = रिक्त;
    ध्रुव FILETIME* mtime = रिक्त;

    यदि (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        atime = &now;
    } अन्यथा यदि (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0] = convert_timestamp(i_atim);
        atime = &times[0];
    }

    यदि (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        mtime = &now;
    } अन्यथा यदि (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1] = convert_timestamp(i_mtim);
        mtime = &times[1];
    }

    यदि (!समुच्चयः_File_Time(handle, रिक्त, atime, mtime)) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_विस्थापनम्, पूर्णाङ्क i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    int64_t ret = _lseeki64(i_fd, i_विस्थापनम्, i_whence);
    यदि (ret < 0) {
        प्रत्यागम errno_from_crt(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

// Windows has neither a vectored nor a positional transfer, so the list is walked
// one buffer at a time and a short transfer ends it, as it would for readv. A
// positional one moves the file pointer and puts it back where it was.
स्थिर
__wasi_errno_t transfer_iovs (__wasi_fd_t i_fd, ध्रुव m3_wasi_iovec_t* i_iovs,
                              __wasi_size_t i_iovsLen, __wasi_filesize_t i_विस्थापनम्,
                              bool i_positional, bool i_write, __wasi_size_t* o_समाप्तम्)
{
    *o_समाप्तम् = 0;

    int64_t saved = 0;

    यदि (i_positional) {
        saved = _lseeki64(i_fd, 0, SEEK_CUR);
        यदि (saved < 0 || _lseeki64(i_fd, (__int64)i_विस्थापनम्, SEEK_SET) < 0) {
            प्रत्यागम errno_from_crt(errno);
        }
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    size_t         समाप्तम् = 0;

    प्रति (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        शून्य*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        यदि (len == 0) {
            अनुवर्त;
        }

        पूर्णाङ्क ret = i_write ? _write(i_fd, addr, (अचिह्नित)len) : _read(i_fd, addr, (अचिह्नित)len);
        यदि (ret < 0) {
            err = errno_from_crt(errno);
            भङ्ग;
        }

        समाप्तम् += (size_t)ret;
        यदि ((size_t)ret < len) {
            भङ्ग;
        }
    }

    यदि (i_positional) {
        _lseeki64(i_fd, saved, SEEK_SET);
    }

    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    *o_समाप्तम् = (__wasi_size_t)समाप्तम्;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ध्रुव m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_समाप्तम्)
{
    प्रत्यागम transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, false, o_समाप्तम्);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ध्रुव m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_समाप्तम्)
{
    प्रत्यागम transfer_iovs(i_fd, i_iovs, i_iovsLen, 0, false, true, o_समाप्तम्);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ध्रुव m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_विस्थापनम्,
                                      __wasi_size_t* o_समाप्तम्)
{
    प्रत्यागम transfer_iovs(i_fd, i_iovs, i_iovsLen, i_विस्थापनम्, true, false, o_समाप्तम्);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ध्रुव m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_विस्थापनम्,
                                       __wasi_size_t* o_समाप्तम्)
{
    प्रत्यागम transfer_iovs(i_fd, i_iovs, i_iovsLen, i_विस्थापनम्, true, true, o_समाप्तम्);
}


/*
 * Paths
 */

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_open (__wasi_fd_t i_dirfd, ccstr_t i_मार्गः,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    वर्ण           पूर्णम्_मार्गः[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_मार्गः, i_dirfd, i_मार्गः);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    // _open refuses a directory, so one is opened through the Win32 call that can
    // and handed to the CRT afterwards. The descriptor is good for what WASI does
    // with a directory - naming it in a later path call - and for nothing else.
    यदि (i_oflags & __WASI_OFLAGS_DIRECTORY) {
        HANDLE handle;
        err = open_मार्गः_handle(पूर्णम्_मार्गः, true, &handle);
        यदि (err != __WASI_ERRNO_SUCCESS) {
            प्रत्यागम err;
        }

        पूर्णाङ्क dir_fd = _open_osfhandle((intptr_t)handle, _O_RDONLY);
        यदि (dir_fd < 0) {
            CloseHandle(handle);
            प्रत्यागम __WASI_ERRNO_MFILE;
        }

        *o_fd = (__wasi_fd_t)dir_fd;
        प्रत्यागम __WASI_ERRNO_SUCCESS;
    }

    // Windows has no O_DSYNC/O_RSYNC/O_SYNC and no non-blocking file descriptor, so
    // those fs_flags are dropped rather than translated.
    पूर्णाङ्क flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? _O_CREAT : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? _O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? _O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? _O_APPEND : 0) |
                O_द्विमान;

    यदि ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_RDWR;
    } अन्यथा यदि ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= _O_WRONLY;
    } अन्यथा यदि ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= _O_RDONLY; // no-op because O_RDONLY is 0
    }

    पूर्णाङ्क host_fd = _open(पूर्णम्_मार्गः, flags, _S_IREAD | _S_IWRITE);
    यदि (host_fd < 0) {
        प्रत्यागम errno_from_crt(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_filestat (__wasi_fd_t i_dirfd, ccstr_t i_मार्गः, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    स्मृतिं_पूरय(o_stat, 0, परिमाण(*o_stat));

    वर्ण           पूर्णम्_मार्गः[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_मार्गः, i_dirfd, i_मार्गः);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    HANDLE handle;
    err = open_मार्गः_handle(पूर्णम्_मार्गः, i_follow, &handle);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    err = filestat_from_handle(handle, o_stat);
    CloseHandle(handle);
    प्रत्यागम err;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_मार्गः)
{
    वर्ण           पूर्णम्_मार्गः[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_मार्गः, i_dirfd, i_मार्गः);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    यदि (!CreateDirectoryA(पूर्णम्_मार्गः, रिक्त)) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_मार्गः)
{
    वर्ण           पूर्णम्_मार्गः[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_मार्गः, i_dirfd, i_मार्गः);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    यदि (!RemoveDirectoryA(पूर्णम्_मार्गः)) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_मार्गः)
{
    वर्ण           पूर्णम्_मार्गः[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_मार्गः, i_dirfd, i_मार्गः);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    // A symbolic link to a directory is a directory entry as far as Windows is
    // concerned, so removing the link itself goes through RemoveDirectory. Every
    // other kind of directory is DeleteFile's to refuse.
    DWORD attrs = Get_File_गुणाः_A(पूर्णम्_मार्गः);
    bool  isDirLink =
      attrs != INVALID_FILE_गुणाः &&
      (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ==
        (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT);

    यदि (!(isDirLink ? RemoveDirectoryA(पूर्णम्_मार्गः) : DeleteFileA(पूर्णम्_मार्गः))) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_मार्गः,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_मार्गः)
{
    वर्ण           पूर्णम्_old[d_m3_Wasi_Max_Host_मार्गः];
    वर्ण           पूर्णम्_new[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_old, i_oldDirfd, i_old_मार्गः);
    यदि (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_मार्गः(पूर्णम्_new, i_newDirfd, i_new_मार्गः);
    }
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    // rename() would refuse an existing destination; renameat() replaces it
    यदि (!MoveFileExA(पूर्णम्_old, पूर्णम्_new, MOVEFILE_REPLACE_EXISTING)) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_मार्गः, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_मार्गः)
{
    वर्ण           पूर्णम्_old[d_m3_Wasi_Max_Host_मार्गः];
    वर्ण           पूर्णम्_new[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_old, i_oldDirfd, i_old_मार्गः);
    यदि (err == __WASI_ERRNO_SUCCESS) {
        err = resolve_मार्गः(पूर्णम्_new, i_newDirfd, i_new_मार्गः);
    }
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    // CreateHardLink resolves the target the way AT_SYMLINK_FOLLOW asks for and has
    // no way of not doing so, so i_follow cannot be honoured here
    यदि (!CreateHardLinkA(पूर्णम्_new, पूर्णम्_old, रिक्त)) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_मार्गः)
{
    वर्ण           पूर्णम्_new[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_new, i_dirfd, i_मार्गः);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    // Windows settles at creation time whether a link names a file or a directory,
    // so the target has to be looked at first. It is stored as given, which puts it
    // beside the link rather than under the preopen.
    वर्ण   target[d_m3_Wasi_Max_Host_मार्गः];
    size_t dir_len = 0;
    प्रति (size_t i = 0; पूर्णम्_new[i]; i++) {
        यदि (पूर्णम्_new[i] == '/' || पूर्णम्_new[i] == '\\') {
            dir_len = i + 1;
        }
    }

    पूर्णाङ्क target_len = snprintf(target, परिमाण(target), "%.*s%s", (पूर्णाङ्क)dir_len, पूर्णम्_new, i_target);
    यदि (target_len < 0 || target_len >= (पूर्णाङ्क)परिमाण(target)) {
        प्रत्यागम __WASI_ERRNO_NAMETOOLONG;
    }

    DWORD flags = 0;
    DWORD attrs = Get_File_गुणाः_A(target);
    यदि (attrs != INVALID_FILE_गुणाः && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }

    // Creating a link is a privilege, waived only for a machine in Developer Mode
    // and only for callers that ask; Windows before 1703 rejects the asking.
    यदि (!CreateSymbolicLinkA(पूर्णम्_new, i_target,
                             flags | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        यदि (Get_चरमः_Error() != ERROR_INVALID_PARAMETER ||
            !CreateSymbolicLinkA(पूर्णम्_new, i_target, flags)) {
            प्रत्यागम errno_from_win32(Get_चरमः_Error());
        }
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

// The link's own target, as UTF-8. The print name is the form the link was written
// with; the substitute name is the resolved one, kept as the fallback for a link that
// carries no print name, minus the object-namespace prefix Windows puts on it.
स्थिर
__wasi_errno_t read_link_target (ccstr_t i_मार्गः, वर्ण* o_target, पूर्णाङ्क i_आकार, पूर्णाङ्क* o_len)
{
    HANDLE handle = CreateFileA(i_मार्गः, FILE_READ_गुणाः,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, रिक्त,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, रिक्त);
    यदि (handle == INVALID_HANDLE_मानम्) {
        प्रत्यागम errno_from_win32(Get_चरमः_Error());
    }

    Reparse_दत्त_बफर* reparse =
      (Reparse_दत्त_बफर*)m3_Malloc("ReparseDataBuffer", अधिकतमम्_REPARSE_दत्त_बफर_आकार);
    यदि (!reparse) {
        CloseHandle(handle);
        प्रत्यागम __WASI_ERRNO_NOMEM;
    }

    __wasi_errno_t err = __WASI_ERRNO_SUCCESS;
    DWORD          returned;
    यदि (!DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, रिक्त, 0, reparse,
                         अधिकतमम्_REPARSE_दत्त_बफर_आकार, &returned, रिक्त)) {
        err = errno_from_win32(Get_चरमः_Error());
    }
    CloseHandle(handle);

    यदि (err == __WASI_ERRNO_SUCCESS) {
        ध्रुव WCHAR* नामानि;
        यदि (reparse->Reparse_चिह्नकम् == IO_REPARSE_चिह्नकम्_SYMLINK) {
            नामानि = reparse->u.symlink.मार्गः_बफर;
        } अन्यथा यदि (reparse->Reparse_चिह्नकम् == IO_REPARSE_चिह्नकम्_MOUNT_POINT) {
            नामानि = reparse->u.mount_point.मार्गः_बफर;
        } अन्यथा {
            नामानि = रिक्त;
            err = __WASI_ERRNO_INVAL;
        }

        यदि (नामानि) {
            ध्रुव WCHAR* name = नामानि + reparse->Print_Name_विस्थापनम् / परिमाण(WCHAR);
            पूर्णाङ्क          len = reparse->Print_Name_दीर्घता / परिमाण(WCHAR);

            यदि (len == 0) {
                name = नामानि + reparse->Substitute_Name_विस्थापनम् / परिमाण(WCHAR);
                len = reparse->Substitute_Name_दीर्घता / परिमाण(WCHAR);
                यदि (len >= 4 && wcsncmp(name, L"\\??\\", 4) == 0) {
                    name += 4;
                    len -= 4;
                }
            }

            *o_len = Wide_Char_गन्तव्यम्_Multi_Byte(CP_UTF8, 0, name, len, o_target, i_आकार, रिक्त, रिक्त);
            यदि (*o_len <= 0 && len != 0) {
                err = errno_from_win32(Get_चरमः_Error());
            }
        }
    }

    m3_Free(reparse);
    प्रत्यागम err;
}

स्थिर
__wasi_errno_t m3_wasi_host_मार्गः_readlink (__wasi_fd_t i_dirfd, ccstr_t i_मार्गः, वर्ण* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_प्रयुक्तम्)
{
    *o_buf_प्रयुक्तम् = 0;

    वर्ण           पूर्णम्_मार्गः[d_m3_Wasi_Max_Host_मार्गः];
    __wasi_errno_t err = resolve_मार्गः(पूर्णम्_मार्गः, i_dirfd, i_मार्गः);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    वर्ण target[d_m3_Wasi_Max_Host_मार्गः];
    पूर्णाङ्क  target_len = 0;

    err = read_link_target(पूर्णम्_मार्गः, target, (पूर्णाङ्क)परिमाण(target), &target_len);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    __wasi_size_t copied = M3_MIN((__wasi_size_t)target_len, i_bufLen);
    स्मृतिं_प्रतिलिख(o_buf, target, copied);
    *o_buf_प्रयुक्तम् = copied;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

// Windows offers the one clock, so every WASI clock maps onto it and none of them is
// rejected
स्थिर
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    // a FILETIME counts in 100ns, but the system clock behind it moves on the timer
    // tick, so this says a millisecond rather than what it can spell
    *o_res = 1000000;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

// The one reading of the clock. Every WASI clock is this one here - Windows offers
// no other - and poll_oneoff waits on it too, so that the time a guest is made to
// wait and the time it can then measure are the same time.
स्थिर
__wasi_timestamp_t now_ns (शून्य)
{
    FILETIME now;
    GetSystemTimeAsFileTime(&now);

    प्रत्यागम convert_filetime(now);
}

__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = now_ns();
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_random_get (शून्य* o_buf, __wasi_size_t i_len)
{
    यदि (!RtlGenRandom(o_buf, i_len)) {
        प्रत्यागम __WASI_ERRNO_IO;
    }
    प्रत्यागम __WASI_ERRNO_SUCCESS;
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
#परिभाषय d_m3WasiPollIntervalMs  5

// Whether a descriptor has what its subscription is waiting for. Everything that is
// not a stream is ready by definition: the spec says a regular file always triggers
// both read and write, and so does anything whose kind Windows will not describe.
स्थिर
__wasi_errno_t poll_fd_ready (ध्रुव m3_wasi_pollsub_t* i_sub, bool* o_ready,
                              __wasi_filesize_t* o_nbytes, __wasi_eventrwflags_t* o_flags)
{
    *o_ready = false;
    *o_nbytes = 0;
    *o_flags = 0;

    HANDLE         handle;

    __wasi_errno_t err = handle_for_fd(i_sub->fd, &handle);
    यदि (err != __WASI_ERRNO_SUCCESS) {
        प्रत्यागम err;
    }

    DWORD type = GetFileType(handle);

    // A write is only ever waiting on the other end of a pipe, and Windows will not
    // say whether that has room, so writing is always reported as ready
    यदि (i_sub->type != __WASI_EVENTTYPE_FD_READ or type == FILE_TYPE_DISK) {
        *o_ready = true;
        *o_nbytes = 1;
        प्रत्यागम __WASI_ERRNO_SUCCESS;
    }

    यदि (type == FILE_TYPE_PIPE) {
        DWORD प्रतीक्षा = 0;

        यदि (!PeekNamedPipe(handle, रिक्त, 0, रिक्त, &प्रतीक्षा, रिक्त)) {
            DWORD चरमः = Get_चरमः_Error();

            // the writer is gone: a read would return end of file, which counts as
            // ready and is what the hangup flag is for
            यदि (चरमः == ERROR_BROKEN_PIPE or चरमः == ERROR_PIPE_NOT_CONNECTED) {
                *o_ready = true;
                *o_flags = __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP;
                प्रत्यागम __WASI_ERRNO_SUCCESS;
            }
            प्रत्यागम errno_from_win32(चरमः);
        }

        *o_ready = (प्रतीक्षा != 0);
        *o_nbytes = प्रतीक्षा;
        प्रत्यागम __WASI_ERRNO_SUCCESS;
    }

    यदि (type == FILE_TYPE_CHAR) {
        // a console has input pending when waiting on it succeeds without a wait
        *o_ready = (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0);
        *o_nbytes = *o_ready ? 1 : 0;
        प्रत्यागम __WASI_ERRNO_SUCCESS;
    }

    *o_ready = true;
    *o_nbytes = 1;
    प्रत्यागम __WASI_ERRNO_SUCCESS;
}

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock and ignores i_elapsedNs, the clock having moved on by itself.
// Every WASI clock is the system clock here - see clock_time_get - so an absolute
// deadline is held against that one whichever clock it names.
स्थिर
uint64_t poll_remaining_ns (ध्रुव m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    यदि (!i_sub->isAbsolute) {
        प्रत्यागम (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    __wasi_timestamp_t now = now_ns();
    प्रत्यागम (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

स्थिर
__wasi_errno_t m3_wasi_host_poll_oneoff (ध्रुव m3_wasi_pollsub_t* i_subs, __wasi_size_t i_गणनम्,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_गणनम्)
{
    *o_event_गणनम् = 0;

    uint64_t soonestNs = UINT64_MAX;
    bool     hasClock = false;
    bool     hasFd = false;

    प्रति (__wasi_size_t i = 0; i < i_गणनम्; i++) {
        यदि (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            soonestNs = M3_MIN(soonestNs, poll_remaining_ns(&i_subs[i], 0));
            hasClock = true;
        } अन्यथा {
            hasFd = true;
        }
    }

    यदि (!hasClock and !hasFd) {
        प्रत्यागम __WASI_ERRNO_INVAL;
    }

    // The wait is measured on the very clock clock_time_get reports, so that a guest
    // which sleeps and then reads the clock cannot see less time than it asked for.
    // GetTickCount64 is the obvious alternative and is wrong for exactly that
    // reason: it is a different clock quantized to the same coarse tick, so the two
    // can disagree by a whole one, and a 20ms sleep reads back as 15.
    uint64_t startNs = now_ns();

    प्रति (;;) {
        __wasi_size_t बहिर्गमनम् = 0;

        प्रति (__wasi_size_t i = 0; i < i_गणनम्; i++) {
            ध्रुव m3_wasi_pollsub_t* sub = &i_subs[i];

            यदि (sub->type == __WASI_EVENTTYPE_CLOCK) {
                अनुवर्त;
            }

            bool                  ready;
            __wasi_filesize_t     nbytes;
            __wasi_eventrwflags_t flags;

            __wasi_errno_t        err = poll_fd_ready(sub, &ready, &nbytes, &flags);

            // a descriptor that cannot be asked is ready with an error on it, which
            // is what tells the guest to stop waiting for it
            यदि (err == __WASI_ERRNO_SUCCESS and !ready) {
                अनुवर्त;
            }

            o_events[बहिर्गमनम्].userdata = sub->userdata;
            o_events[बहिर्गमनम्].error = err;
            o_events[बहिर्गमनम्].type = sub->type;
            o_events[बहिर्गमनम्].nbytes = nbytes;
            o_events[बहिर्गमनम्].flags = flags;
            बहिर्गमनम्++;
        }

        // This is a wall clock and can be set backwards under us, which is not
        // elapsed time going negative but the measurement being no longer usable;
        // starting the count again from here is the honest reading of it.
        ध्रुव uint64_t t = now_ns();
        ध्रुव uint64_t elapsedNs = (t > startNs) ? t - startNs : 0;

        यदि (t < startNs) {
            startNs = t;
        }

        bool expired = hasClock and (elapsedNs >= soonestNs);

        यदि (expired) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too
            प्रति (__wasi_size_t i = 0; i < i_गणनम्; i++) {
                ध्रुव m3_wasi_pollsub_t* sub = &i_subs[i];

                यदि (sub->type == __WASI_EVENTTYPE_CLOCK and
                    poll_remaining_ns(sub, elapsedNs) == 0) {
                    o_events[बहिर्गमनम्].userdata = sub->userdata;
                    o_events[बहिर्गमनम्].error = __WASI_ERRNO_SUCCESS;
                    o_events[बहिर्गमनम्].type = __WASI_EVENTTYPE_CLOCK;
                    o_events[बहिर्गमनम्].nbytes = 0;
                    o_events[बहिर्गमनम्].flags = 0;
                    बहिर्गमनम्++;
                }
            }
        }

        यदि (बहिर्गमनम्) {
            *o_event_गणनम् = बहिर्गमनम्;
            प्रत्यागम __WASI_ERRNO_SUCCESS;
        }

        // Nothing yet. With no descriptor to watch there is nothing to come back
        // for, so sleep out the whole remainder; with one, sleep to the next round
        // or to the deadline, whichever comes first. Rounding up is what keeps a
        // sub-millisecond remainder from becoming a spin.
        DWORD sleepMs = d_m3WasiPollIntervalMs;

        यदि (hasClock) {
            uint64_t remainingNs = soonestNs - elapsedNs;
            uint64_t remainingMs = (remainingNs + 999999) / 1000000;

            यदि (not hasFd or remainingMs < sleepMs) {
                sleepMs = (remainingMs > MAXDWORD) ? MAXDWORD : (DWORD)remainingMs;
            }
        }

        Sleep(sleepMs);
    }
}

#यदि_अन्तः // m3_api_wasi_win32_h
