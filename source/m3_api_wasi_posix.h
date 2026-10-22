//
//  m3_api_wasi_posix.h
//
//  The POSIX implementation of m3_api_wasi_host.h. Every path here is resolved with
//  an *at() call against the descriptor a preopen holds, which is what keeps the
//  guest inside the directories it was given.
//
//  A header rather than a translation unit of its own, and included by m3_api_wasi.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#अगर_अघोषित m3_api_wasi_posix_h
#घोषणा m3_api_wasi_posix_h

#समावेश "m3_core.h"

#अगर defined(_WIN32)
#  error "This file is not for Windows"
#पूर्ण_अगर

#समावेश <sys/types.h>
#समावेश <sys/stat.h>
#समावेश <errno.h>
#समावेश <fcntl.h>
#समावेश <limits.h>
#समावेश <poll.h>
#समावेश <string.h>
#समावेश <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#अगर !defined(__wasi__)
#  include <sys/ioctl.h>
#पूर्ण_अगर

#अगर defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  अगर defined(__APPLE__)
#    include <TargetConditionals.h>
#    अगर TARGET_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    अन्यथा // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  अन्यथा
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#पूर्ण_अगर

#समावेश "m3_api_wasi_host.h"


/*
 * Plumbing
 */

अटल
__wasi_errno_t errno_to_wasi (पूर्णांक errnum)
{
    // clang-format off
    चयन (errnum) {
    हाल EPERM:    वापस __WASI_ERRNO_PERM;
    हाल ENOENT:   वापस __WASI_ERRNO_NOENT;
    हाल ESRCH:    वापस __WASI_ERRNO_SRCH;
    हाल EINTR:    वापस __WASI_ERRNO_INTR;
    हाल EIO:      वापस __WASI_ERRNO_IO;
    हाल ENXIO:    वापस __WASI_ERRNO_NXIO;
    हाल E2BIG:    वापस __WASI_ERRNO_2BIG;
    हाल ENOEXEC:  वापस __WASI_ERRNO_NOEXEC;
    हाल EBADF:    वापस __WASI_ERRNO_BADF;
    हाल ECHILD:   वापस __WASI_ERRNO_CHILD;
    हाल EAGAIN:   वापस __WASI_ERRNO_AGAIN;
    हाल ENOMEM:   वापस __WASI_ERRNO_NOMEM;
    हाल EACCES:   वापस __WASI_ERRNO_ACCES;
    हाल EFAULT:   वापस __WASI_ERRNO_FAULT;
    हाल EBUSY:    वापस __WASI_ERRNO_BUSY;
    हाल EEXIST:   वापस __WASI_ERRNO_EXIST;
    हाल EXDEV:    वापस __WASI_ERRNO_XDEV;
    हाल ENODEV:   वापस __WASI_ERRNO_NODEV;
    हाल ENOTDIR:  वापस __WASI_ERRNO_NOTDIR;
    हाल EISDIR:   वापस __WASI_ERRNO_ISDIR;
    हाल EINVAL:   वापस __WASI_ERRNO_INVAL;
    हाल ENFILE:   वापस __WASI_ERRNO_NFILE;
    हाल EMFILE:   वापस __WASI_ERRNO_MFILE;
    हाल ENOTTY:   वापस __WASI_ERRNO_NOTTY;
    हाल ETXTBSY:  वापस __WASI_ERRNO_TXTBSY;
    हाल EFBIG:    वापस __WASI_ERRNO_FBIG;
    हाल ENOSPC:   वापस __WASI_ERRNO_NOSPC;
    हाल ESPIPE:   वापस __WASI_ERRNO_SPIPE;
    हाल EROFS:    वापस __WASI_ERRNO_ROFS;
    हाल EMLINK:   वापस __WASI_ERRNO_MLINK;
    हाल EPIPE:    वापस __WASI_ERRNO_PIPE;
    हाल गलत_तर्क:     वापस __WASI_ERRNO_DOM;
    हाल दुस्फल:   वापस __WASI_ERRNO_RANGE;
    }
    // clang-format on
    वापस __WASI_ERRNO_INVAL;
}

अटल
__wasi_filetype_t filetype_from_stat_mode (पूर्णांक mode)
{
    वापस (S_ISBLK(mode) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(mode) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(mode) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(mode) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(mode) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

अटल
व्योम filestat_from_stat (स्थिर काष्ठा stat* i_stat, m3_wasi_filestat_t* o_stat)
{
    o_stat->dev = (uint64_t)i_stat->st_dev;
    o_stat->ino = (uint64_t)i_stat->st_ino;
    o_stat->nlink = (uint64_t)i_stat->st_nlink;
    o_stat->आकार = (uint64_t)i_stat->st_size;
    o_stat->atim = (uint64_t)i_stat->st_atime * 1000000000ULL;
    o_stat->mtim = (uint64_t)i_stat->st_mtime * 1000000000ULL;
    o_stat->ctim = (uint64_t)i_stat->st_ctime * 1000000000ULL;
    o_stat->filetype = filetype_from_stat_mode(i_stat->st_mode);
}

// The descriptor a preopened directory holds, or -1 where i_fd names no preopen
अटल
पूर्णांक preopen_fd (__wasi_fd_t i_fd)
{
    अगर (i_fd >= d_m3_Wasi_Preopen_गिनती) {
        वापस -1;
    }
    वापस m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#अगर defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) शून्य)
#अन्यथा
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#पूर्ण_अगर

अटल
clockid_t convert_clockid (__wasi_clockid_t in)
{
    चयन (in) {
    हाल __WASI_CLOCKID_MONOTONIC: वापस CLOCK_MONOTONIC;
    हाल __WASI_CLOCKID_REALTIME: वापस CLOCK_REALTIME;
#अगर defined(CLOCK_PROCESS_CPUTIME_ID)
    हाल __WASI_CLOCKID_PROCESS_CPUTIME_ID: वापस CLOCK_PROCESS_CPUTIME_ID;
#पूर्ण_अगर
#अगर defined(CLOCK_THREAD_CPUTIME_ID)
    हाल __WASI_CLOCKID_THREAD_CPUTIME_ID: वापस CLOCK_THREAD_CPUTIME_ID;
#पूर्ण_अगर
    शेष: वापस d_m3ClockIdInvalid;
    }
}

अटल
__wasi_timestamp_t convert_timespec (स्थिर काष्ठा timespec* ts)
{
    अगर (ts->tv_sec < 0) {
        वापस 0;
    }
    अगर ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        वापस UINT64_MAX;
    }
    वापस (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

अटल
व्योम m3_wasi_host_init ()
{
    क्रम (पूर्णांक i = 3; i < d_m3_Wasi_Preopen_गिनती; i++) {
        अगर (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = खोलो(m3_wasi_preopen[i].real_path, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

अटल
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    अगर (बंद(i_fd) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#अगर defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#या_अगर defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#पूर्ण_अगर

अटल
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#अगर defined(d_m3WasiDatasync)
    अगर (d_m3WasiDatasync(i_fd) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
#अन्यथा
    वापस __WASI_ERRNO_NOSYS;
#पूर्ण_अगर
}

अटल
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_लंबाई, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#अगर defined(POSIX_FADV_NORMAL)
    पूर्णांक adv;
    चयन (i_advice) {
    हाल __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; अवरोध;
    हाल __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; अवरोध;
    हाल __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; अवरोध;
    हाल __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; अवरोध;
    हाल __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; अवरोध;
    हाल __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; अवरोध;
    शेष: वापस __WASI_ERRNO_INVAL;
    }

    पूर्णांक ret = posix_fadvise(i_fd, i_offset, i_लंबाई, adv);
    अगर (ret != 0) {
        वापस errno_to_wasi(ret);
    }
#अन्यथा
    अगर (i_advice > __WASI_ADVICE_NOREUSE) {
        वापस __WASI_ERRNO_INVAL;
    }

    काष्ठा stat fd_stat;
    अगर (ख_स्थिति(i_fd, &fd_stat) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
#पूर्ण_अगर

    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    पूर्णांक fl = fcntl(i_fd, F_GETFL);
    अगर (fl < 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    काष्ठा stat fd_stat;
    अगर (ख_स्थिति(i_fd, &fd_stat) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    *o_filetype = filetype_from_stat_mode(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    वापस __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
अटल
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    पूर्णांक fl = fcntl(i_fd, F_GETFL);
    अगर (fl < 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    स्थिर __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    अगर ((i_flags & fixed) != (current & fixed)) {
        वापस __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    अगर (fcntl(i_fd, F_SETFL, fl) < 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    स_रखो(o_stat, 0, माप(*o_stat));

    काष्ठा stat fd_stat;
    अगर (ख_स्थिति(i_fd, &fd_stat) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    filestat_from_stat(&fd_stat, o_stat);
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_fd_set_आकार (__wasi_fd_t i_fd, __wasi_filesize_t i_आकार)
{
    अगर (ftruncate(i_fd, (off_t)i_आकार) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    काष्ठा timespec बार[2];

    अगर (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        बार[0].tv_sec = 0;
        बार[0].tv_nsec = UTIME_NOW;
    } अन्यथा अगर (i_flags & __WASI_FSTFLAGS_ATIM) {
        बार[0].tv_sec = i_atim / 1000000000ULL;
        बार[0].tv_nsec = i_atim % 1000000000ULL;
    } अन्यथा {
        बार[0].tv_sec = 0;
        बार[0].tv_nsec = UTIME_OMIT;
    }

    अगर (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        बार[1].tv_sec = 0;
        बार[1].tv_nsec = UTIME_NOW;
    } अन्यथा अगर (i_flags & __WASI_FSTFLAGS_MTIM) {
        बार[1].tv_sec = i_mtim / 1000000000ULL;
        बार[1].tv_nsec = i_mtim % 1000000000ULL;
    } अन्यथा {
        बार[1].tv_sec = 0;
        बार[1].tv_nsec = UTIME_OMIT;
    }

    अगर (futimens(i_fd, बार) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, पूर्णांक i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_offset, i_whence);
    अगर (ret < 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    *o_pos = (__wasi_filesize_t)ret;
    वापस __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#अगर defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
अटल
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, स्थिर m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    काष्ठा iovec iovs[d_m3WasiMaxIovs];
    क्रम (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (पूर्णांक)i_iovsLen) : readv(i_fd, iovs, (पूर्णांक)i_iovsLen);
    अगर (ret < 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    *o_done = (__wasi_size_t)ret;
    वापस __WASI_ERRNO_SUCCESS;
}

#अन्यथा

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
अटल
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, स्थिर m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    माप_प्रकार done = 0;

    क्रम (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        व्योम*  addr = i_iovs[i].buf;
        माप_प्रकार len = i_iovs[i].len;
        अगर (len == 0) {
            जारी;
        }

        ssize_t ret = i_write ? ग_लिखो(i_fd, addr, len) : पढ़ो(i_fd, addr, len);
        अगर (ret < 0) {
            वापस errno_to_wasi(त्रुटि_सं);
        }

        done += (माप_प्रकार)ret;
        अगर ((माप_प्रकार)ret < len) {
            अवरोध;
        }
    }

    *o_done = (__wasi_size_t)done;
    वापस __WASI_ERRNO_SUCCESS;
}

#पूर्ण_अगर // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
अटल
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, स्थिर m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset, bool i_write,
                            __wasi_size_t* o_done)
{
    *o_done = 0;

    माप_प्रकार done = 0;

    क्रम (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        व्योम*  addr = i_iovs[i].buf;
        माप_प्रकार len = i_iovs[i].len;
        अगर (len == 0) {
            जारी;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_offset + done))
                              : pread(i_fd, addr, len, (off_t)(i_offset + done));
        अगर (ret < 0) {
            वापस errno_to_wasi(त्रुटि_सं);
        }

        done += (माप_प्रकार)ret;
        अगर ((माप_प्रकार)ret < len) {
            अवरोध;
        }
    }

    *o_done = (__wasi_size_t)done;
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, स्थिर m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    वापस transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_done);
}

अटल
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, स्थिर m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    वापस transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_done);
}

अटल
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, स्थिर m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    वापस transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, false, o_done);
}

अटल
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, स्थिर m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    वापस transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, true, o_done);
}


/*
 * Paths
 */

अटल
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    पूर्णांक dirfd = preopen_fd(i_dirfd);
    अगर (dirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    पूर्णांक flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    अगर ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } अन्यथा अगर ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } अन्यथा अगर ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    पूर्णांक host_fd = openat(dirfd, i_path, flags, 0644);
    अगर (host_fd < 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    स_रखो(o_stat, 0, माप(*o_stat));

    पूर्णांक dirfd = preopen_fd(i_dirfd);
    अगर (dirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    काष्ठा stat path_stat;
    अगर (fstatat(dirfd, i_path, &path_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    filestat_from_stat(&path_stat, o_stat);
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    अगर (dirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    अगर (mkdirat(dirfd, i_path, 0755) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    अगर (dirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    अगर (unlinkat(dirfd, i_path, AT_REMOVEDIR) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    अगर (dirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    अगर (unlinkat(dirfd, i_path, 0) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    पूर्णांक oldDirfd = preopen_fd(i_oldDirfd);
    पूर्णांक newDirfd = preopen_fd(i_newDirfd);
    अगर (oldDirfd < 0 || newDirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    अगर (renameat(oldDirfd, i_oldPath, newDirfd, i_newPath) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    पूर्णांक oldDirfd = preopen_fd(i_oldDirfd);
    पूर्णांक newDirfd = preopen_fd(i_newDirfd);
    अगर (oldDirfd < 0 || newDirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    अगर (linkat(oldDirfd, i_oldPath, newDirfd, i_newPath, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    अगर (dirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    अगर (symlinkat(i_target, dirfd, i_path) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, अक्षर* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    पूर्णांक dirfd = preopen_fd(i_dirfd);
    अगर (dirfd < 0) {
        वापस __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_path, o_buf, i_bufLen);
    अगर (ret < 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    *o_bufUsed = (__wasi_size_t)ret;
    वापस __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

अटल
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    अगर (clk == d_m3ClockIdInvalid) {
        वापस __WASI_ERRNO_INVAL;
    }

    काष्ठा timespec tp;
    अगर (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } अन्यथा {
        *o_res = convert_timespec(&tp);
    }
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = 0;

    clockid_t clk = convert_clockid(i_clockId);
    अगर (clk == d_m3ClockIdInvalid) {
        वापस __WASI_ERRNO_INVAL;
    }

    काष्ठा timespec tp;
    अगर (clock_gettime(clk, &tp) != 0) {
        वापस errno_to_wasi(त्रुटि_सं);
    }

    *o_time = convert_timespec(&tp);
    वापस __WASI_ERRNO_SUCCESS;
}

अटल
__wasi_errno_t m3_wasi_host_random_get (व्योम* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    जबतक (i_len) {
        ssize_t retlen;

#अगर defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        माप_प्रकार reqlen = M3_MIN(i_len, 256);
#  अगर defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
        retlen = SecRandomCopyBytes(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  अन्यथा
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#या_अगर defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#अन्यथा
        वापस __WASI_ERRNO_NOSYS;
#पूर्ण_अगर

        अगर (retlen < 0) {
            अगर (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN) {
                जारी;
            }
            वापस errno_to_wasi(त्रुटि_सं);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    वापस __WASI_ERRNO_SUCCESS;
}


/*
 * Waiting
 */

// How much longer a clock subscription has to wait, in nanoseconds, given that
// i_elapsedNs of this call have already gone by - so 0 means it is ready. A relative
// subscription counts down from the timeout it named; an absolute one is held
// against the clock it names and ignores i_elapsedNs, that clock having moved on by
// itself. A clock this system has not got answers UINT64_MAX so that it never
// becomes the soonest deadline - the caller reports EINVAL for it separately.
अटल
uint64_t poll_remaining_ns (स्थिर m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    अगर (not i_sub->isAbsolute) {
        वापस (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    अगर (clk == d_m3ClockIdInvalid) {
        वापस UINT64_MAX;
    }

    काष्ठा timespec tp;
    अगर (clock_gettime(clk, &tp) != 0) {
        वापस 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    वापस (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
अटल
__wasi_filesize_t poll_readable_bytes (पूर्णांक i_fd)
{
#अगर defined(FIONREAD)
    पूर्णांक available = 0;
    अगर (ioctl(i_fd, FIONREAD, &available) == 0 and available > 0) {
        वापस (__wasi_filesize_t)available;
    }
#पूर्ण_अगर
    वापस 1;
}

अटल
__wasi_errno_t m3_wasi_host_poll_oneoff (स्थिर m3_wasi_pollsub_t* i_subs, __wasi_size_t i_गिनती,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_गिनती)
{
    *o_event_गिनती = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t अंक_Fds = 0;

    क्रम (__wasi_size_t i = 0; i < i_गिनती; i++) {
        अगर (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_remaining_ns(&i_subs[i], 0);
            अगर (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } अन्यथा {
            अंक_Fds++;
        }
    }

    काष्ठा pollfd* pollfds = शून्य;

    अगर (अंक_Fds) {
        pollfds = (काष्ठा pollfd*)m3_Malloc("WASI poll_oneoff", अंक_Fds * माप(काष्ठा pollfd));
        अगर (pollfds == शून्य) {
            वापस __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        क्रम (__wasi_size_t i = 0; i < i_गिनती; i++) {
            अगर (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                जारी;
            }
            pollfds[n].fd = (पूर्णांक)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    पूर्णांक timeoutMs = -1;
    अगर (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)पूर्णांक_उच्च) ? पूर्णांक_उच्च : (पूर्णांक)ms;
    }

    पूर्णांक ready;
    अगर (अंक_Fds) {
        करो {
            ready = poll(pollfds, (nfds_t)अंक_Fds, timeoutMs);
        } जबतक (ready < 0 and त्रुटि_सं == EINTR);
    } अन्यथा अगर (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        काष्ठा timespec req;
        req.tv_sec = (समय_प्रकार)(soonest / 1000000000);
        req.tv_nsec = (दीर्घ)(soonest % 1000000000);

        जबतक (nanosleep(&req, &req) != 0 and त्रुटि_सं == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } अन्यथा अगर (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } अन्यथा {
        // no clocks and no descriptors: nothing that could ever become ready
        वापस __WASI_ERRNO_INVAL;
    }

    अगर (ready < 0) {
        __wasi_errno_t err = errno_to_wasi(त्रुटि_सं);
        m3_Free(pollfds);
        वापस err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t out = 0;
    __wasi_size_t n = 0;

    क्रम (__wasi_size_t i = 0; i < i_गिनती; i++) {
        स्थिर m3_wasi_pollsub_t* sub = &i_subs[i];

        अगर (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            अगर (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_INVAL;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            } अन्यथा अगर (poll_remaining_ns(sub, elapsedNs) == 0) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_SUCCESS;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            }
            जारी;
        }

        लघु revents = pollfds[n].revents;
        n++;

        अगर (revents == 0) {
            जारी;
        }

        o_events[out].userdata = sub->userdata;
        o_events[out].type = sub->type;
        o_events[out].nbytes = 0;
        o_events[out].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        अगर (revents & POLLNVAL) {
            o_events[out].error = __WASI_ERRNO_BADF;
        } अन्यथा अगर (revents & POLLERR) {
            o_events[out].error = __WASI_ERRNO_IO;
        } अन्यथा {
            o_events[out].error = __WASI_ERRNO_SUCCESS;
            अगर (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[out].nbytes = poll_readable_bytes(pollfds[n - 1].fd);
            }
        }

        out++;
    }

    m3_Free(pollfds);

    *o_event_गिनती = out;
    वापस __WASI_ERRNO_SUCCESS;
}

#पूर्ण_अगर // m3_api_wasi_posix_h
