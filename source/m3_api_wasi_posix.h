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

#ifndef m3_api_wasi_posix_h
#define m3_api_wasi_posix_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಯದಿ defined(_WIN32)
#  error "This file is not for Windows"
#endif

#ಅನ್ತರ್ಭಾವ <sys/types.h>
#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ಅನ್ತರ್ಭಾವ <errno.h>
#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <limits.h>
#ಅನ್ತರ್ಭಾವ <poll.h>
#ಅನ್ತರ್ಭಾವ <string.h>
#ಅನ್ತರ್ಭಾವ <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#ಯದಿ !defined(__wasi__)
#  include <sys/ioctl.h>
#endif

#ಯದಿ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  ಯದಿ defined(__APPLE__)
#    include <TargetConditionals.h>
#    ಯದಿ TARGET_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    ಅನ್ಯಥಾ // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  ಅನ್ಯಥಾ
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#endif

#ಅನ್ತರ್ಭಾವ "m3_api_wasi_host.h"


/*
 * Plumbing
 */

ಸ್ಥಿರ
__wasi_errno_t errno_to_wasi (ಪೂರ್ಣಾಙ್ಕ errnum)
{
    // clang-format off
    ವಿಕಲ್ಪ (errnum) {
    ಪಕ್ಷ EPERM:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_PERM;
    ಪಕ್ಷ ENOENT:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOENT;
    ಪಕ್ಷ ESRCH:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SRCH;
    ಪಕ್ಷ EINTR:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INTR;
    ಪಕ್ಷ EIO:      ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_IO;
    ಪಕ್ಷ ENXIO:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NXIO;
    ಪಕ್ಷ E2BIG:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_2BIG;
    ಪಕ್ಷ ENOEXEC:  ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOEXEC;
    ಪಕ್ಷ EBADF:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    ಪಕ್ಷ ECHILD:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_CHILD;
    ಪಕ್ಷ EAGAIN:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_AGAIN;
    ಪಕ್ಷ ENOMEM:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOMEM;
    ಪಕ್ಷ EACCES:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_ACCES;
    ಪಕ್ಷ EFAULT:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_FAULT;
    ಪಕ್ಷ EBUSY:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BUSY;
    ಪಕ್ಷ EEXIST:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_EXIST;
    ಪಕ್ಷ EXDEV:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_XDEV;
    ಪಕ್ಷ ENODEV:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NODEV;
    ಪಕ್ಷ ENOTDIR:  ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOTDIR;
    ಪಕ್ಷ EISDIR:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_ISDIR;
    ಪಕ್ಷ EINVAL:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INVAL;
    ಪಕ್ಷ ENFILE:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NFILE;
    ಪಕ್ಷ EMFILE:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_MFILE;
    ಪಕ್ಷ ENOTTY:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOTTY;
    ಪಕ್ಷ ETXTBSY:  ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_TXTBSY;
    ಪಕ್ಷ EFBIG:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_FBIG;
    ಪಕ್ಷ ENOSPC:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOSPC;
    ಪಕ್ಷ ESPIPE:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SPIPE;
    ಪಕ್ಷ EROFS:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_ROFS;
    ಪಕ್ಷ EMLINK:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_MLINK;
    ಪಕ್ಷ EPIPE:    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_PIPE;
    ಪಕ್ಷ EDOM:     ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_DOM;
    ಪಕ್ಷ ERANGE:   ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_RANGE;
    }
    // clang-format on
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INVAL;
}

ಸ್ಥಿರ
__wasi_filetype_t filetype_from_stat_mode (ಪೂರ್ಣಾಙ್ಕ mode)
{
    ಪ್ರತ್ಯಾಗಮ (S_ISBLK(mode) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(mode) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(mode) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(mode) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(mode) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

ಸ್ಥಿರ
ಶೂನ್ಯ filestat_from_stat (ಧ್ರುವ ಸಂರಚನಾ stat* i_stat, m3_wasi_filestat_t* o_stat)
{
    o_stat->dev = (uint64_t)i_stat->st_dev;
    o_stat->ino = (uint64_t)i_stat->st_ino;
    o_stat->nlink = (uint64_t)i_stat->st_nlink;
    o_stat->size = (uint64_t)i_stat->st_size;
    o_stat->atim = (uint64_t)i_stat->st_atime * 1000000000ULL;
    o_stat->mtim = (uint64_t)i_stat->st_mtime * 1000000000ULL;
    o_stat->ctim = (uint64_t)i_stat->st_ctime * 1000000000ULL;
    o_stat->filetype = filetype_from_stat_mode(i_stat->st_mode);
}

// The descriptor a preopened directory holds, or -1 where i_fd names no preopen
ಸ್ಥಿರ
ಪೂರ್ಣಾಙ್ಕ preopen_fd (__wasi_fd_t i_fd)
{
    ಯದಿ (i_fd >= d_m3_Wasi_Preopen_ಎಣಿಕೆ) {
        ಪ್ರತ್ಯಾಗಮ -1;
    }
    ಪ್ರತ್ಯಾಗಮ m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#ಯದಿ defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) ರಿಕ್ತ)
#ಅನ್ಯಥಾ
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#endif

ಸ್ಥಿರ
clockid_t convert_clockid (__wasi_clockid_t in)
{
    ವಿಕಲ್ಪ (in) {
    ಪಕ್ಷ __WASI_CLOCKID_MONOTONIC: ಪ್ರತ್ಯಾಗಮ CLOCK_MONOTONIC;
    ಪಕ್ಷ __WASI_CLOCKID_REALTIME: ಪ್ರತ್ಯಾಗಮ CLOCK_REALTIME;
#ಯದಿ defined(CLOCK_PROCESS_CPUTIME_ID)
    ಪಕ್ಷ __WASI_CLOCKID_PROCESS_CPUTIME_ID: ಪ್ರತ್ಯಾಗಮ CLOCK_PROCESS_CPUTIME_ID;
#endif
#ಯದಿ defined(CLOCK_THREAD_CPUTIME_ID)
    ಪಕ್ಷ __WASI_CLOCKID_THREAD_CPUTIME_ID: ಪ್ರತ್ಯಾಗಮ CLOCK_THREAD_CPUTIME_ID;
#endif
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ d_m3ClockIdInvalid;
    }
}

ಸ್ಥಿರ
__wasi_timestamp_t convert_timespec (ಧ್ರುವ ಸಂರಚನಾ timespec* ts)
{
    ಯದಿ (ts->tv_sec < 0) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }
    ಯದಿ ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        ಪ್ರತ್ಯಾಗಮ UINT64_MAX;
    }
    ಪ್ರತ್ಯಾಗಮ (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

ಸ್ಥಿರ
ಶೂನ್ಯ m3_wasi_host_init ()
{
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 3; i < d_m3_Wasi_Preopen_ಎಣಿಕೆ; i++) {
        ಯದಿ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_path, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ಯದಿ (close(i_fd) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#ಯದಿ defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#ನೋಚೇದ್ಯದಿ defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#endif

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#ಯದಿ defined(d_m3WasiDatasync)
    ಯದಿ (d_m3WasiDatasync(i_fd) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOSYS;
#endif
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#ಯದಿ defined(POSIX_FADV_NORMAL)
    ಪೂರ್ಣಾಙ್ಕ adv;
    ವಿಕಲ್ಪ (i_advice) {
    ಪಕ್ಷ __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; ಭಙ್ಗ;
    ಪಕ್ಷ __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; ಭಙ್ಗ;
    ಪಕ್ಷ __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; ಭಙ್ಗ;
    ಪಕ್ಷ __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; ಭಙ್ಗ;
    ಪಕ್ಷ __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; ಭಙ್ಗ;
    ಪಕ್ಷ __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; ಭಙ್ಗ;
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INVAL;
    }

    ಪೂರ್ಣಾಙ್ಕ ret = posix_fadvise(i_fd, i_offset, i_length, adv);
    ಯದಿ (ret != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(ret);
    }
#ಅನ್ಯಥಾ
    ಯದಿ (i_advice > __WASI_ADVICE_NOREUSE) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INVAL;
    }

    ಸಂರಚನಾ stat fd_stat;
    ಯದಿ (fstat(i_fd, &fd_stat) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
#endif

    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    ಪೂರ್ಣಾಙ್ಕ fl = fcntl(i_fd, F_GETFL);
    ಯದಿ (fl < 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    ಸಂರಚನಾ stat fd_stat;
    ಯದಿ (fstat(i_fd, &fd_stat) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    *o_filetype = filetype_from_stat_mode(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    ಪೂರ್ಣಾಙ್ಕ fl = fcntl(i_fd, F_GETFL);
    ಯದಿ (fl < 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    ಧ್ರುವ __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    ಯದಿ ((i_flags & fixed) != (current & fixed)) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    ಯದಿ (fcntl(i_fd, F_SETFL, fl) < 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ಪರಿಮಾಣ(*o_stat));

    ಸಂರಚನಾ stat fd_stat;
    ಯದಿ (fstat(i_fd, &fd_stat) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    ಯದಿ (ftruncate(i_fd, (off_t)i_size) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    ಸಂರಚನಾ timespec times[2];

    ಯದಿ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } ಅನ್ಯಥಾ ಯದಿ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } ಅನ್ಯಥಾ {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    ಯದಿ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } ಅನ್ಯಥಾ ಯದಿ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } ಅನ್ಯಥಾ {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    ಯದಿ (futimens(i_fd, times) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, ಪೂರ್ಣಾಙ್ಕ i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_offset, i_whence);
    ಯದಿ (ret < 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#ಯದಿ defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
ಸ್ಥಿರ
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ಧ್ರುವ m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    ಸಂರಚನಾ iovec iovs[d_m3WasiMaxIovs];
    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (ಪೂರ್ಣಾಙ್ಕ)i_iovsLen) : readv(i_fd, iovs, (ಪೂರ್ಣಾಙ್ಕ)i_iovsLen);
    ಯದಿ (ret < 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    *o_done = (__wasi_size_t)ret;
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

#ಅನ್ಯಥಾ

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
ಸ್ಥಿರ
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ಧ್ರುವ m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ಶೂನ್ಯ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ಯದಿ (len == 0) {
            ಅನುವರ್ತ;
        }

        ssize_t ret = i_write ? ಲಿಖ(i_fd, addr, len) : ಪಠ(i_fd, addr, len);
        ಯದಿ (ret < 0) {
            ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
        }

        done += (size_t)ret;
        ಯದಿ ((size_t)ret < len) {
            ಭಙ್ಗ;
        }
    }

    *o_done = (__wasi_size_t)done;
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

#endif // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
ಸ್ಥಿರ
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, ಧ್ರುವ m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset, bool i_write,
                            __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ಶೂನ್ಯ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ಯದಿ (len == 0) {
            ಅನುವರ್ತ;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_offset + done))
                              : pread(i_fd, addr, len, (off_t)(i_offset + done));
        ಯದಿ (ret < 0) {
            ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
        }

        done += (size_t)ret;
        ಯದಿ ((size_t)ret < len) {
            ಭಙ್ಗ;
        }
    }

    *o_done = (__wasi_size_t)done;
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ಧ್ರುವ m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    ಪ್ರತ್ಯಾಗಮ transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_done);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ಧ್ರುವ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    ಪ್ರತ್ಯಾಗಮ transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_done);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ಧ್ರುವ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    ಪ್ರತ್ಯಾಗಮ transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, false, o_done);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ಧ್ರುವ m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    ಪ್ರತ್ಯಾಗಮ transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, true, o_done);
}


/*
 * Paths
 */

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    ಪೂರ್ಣಾಙ್ಕ dirfd = preopen_fd(i_dirfd);
    ಯದಿ (dirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    ಪೂರ್ಣಾಙ್ಕ flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    ಯದಿ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } ಅನ್ಯಥಾ ಯದಿ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } ಅನ್ಯಥಾ ಯದಿ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    ಪೂರ್ಣಾಙ್ಕ host_fd = openat(dirfd, i_path, flags, 0644);
    ಯದಿ (host_fd < 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ಪರಿಮಾಣ(*o_stat));

    ಪೂರ್ಣಾಙ್ಕ dirfd = preopen_fd(i_dirfd);
    ಯದಿ (dirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ಸಂರಚನಾ stat path_stat;
    ಯದಿ (fstatat(dirfd, i_path, &path_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    filestat_from_stat(&path_stat, o_stat);
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    ಪೂರ್ಣಾಙ್ಕ dirfd = preopen_fd(i_dirfd);
    ಯದಿ (dirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ಯದಿ (mkdirat(dirfd, i_path, 0755) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    ಪೂರ್ಣಾಙ್ಕ dirfd = preopen_fd(i_dirfd);
    ಯದಿ (dirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ಯದಿ (unlinkat(dirfd, i_path, AT_REMOVEDIR) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    ಪೂರ್ಣಾಙ್ಕ dirfd = preopen_fd(i_dirfd);
    ಯದಿ (dirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ಯದಿ (unlinkat(dirfd, i_path, 0) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    ಪೂರ್ಣಾಙ್ಕ oldDirfd = preopen_fd(i_oldDirfd);
    ಪೂರ್ಣಾಙ್ಕ newDirfd = preopen_fd(i_newDirfd);
    ಯದಿ (oldDirfd < 0 || newDirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ಯದಿ (renameat(oldDirfd, i_oldPath, newDirfd, i_newPath) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    ಪೂರ್ಣಾಙ್ಕ oldDirfd = preopen_fd(i_oldDirfd);
    ಪೂರ್ಣಾಙ್ಕ newDirfd = preopen_fd(i_newDirfd);
    ಯದಿ (oldDirfd < 0 || newDirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ಯದಿ (linkat(oldDirfd, i_oldPath, newDirfd, i_newPath, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    ಪೂರ್ಣಾಙ್ಕ dirfd = preopen_fd(i_dirfd);
    ಯದಿ (dirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ಯದಿ (symlinkat(i_target, dirfd, i_path) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, ವರ್ಣ* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    ಪೂರ್ಣಾಙ್ಕ dirfd = preopen_fd(i_dirfd);
    ಯದಿ (dirfd < 0) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_path, o_buf, i_bufLen);
    ಯದಿ (ret < 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    *o_bufUsed = (__wasi_size_t)ret;
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ಯದಿ (clk == d_m3ClockIdInvalid) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INVAL;
    }

    ಸಂರಚನಾ timespec tp;
    ಯದಿ (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } ಅನ್ಯಥಾ {
        *o_res = convert_timespec(&tp);
    }
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ಯದಿ (clk == d_m3ClockIdInvalid) {
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INVAL;
    }

    ಸಂರಚನಾ timespec tp;
    ಯದಿ (clock_gettime(clk, &tp) != 0) {
        ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
    }

    *o_time = convert_timespec(&tp);
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_random_get (ಶೂನ್ಯ* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    ಯಾವತ್ (i_len) {
        ssize_t retlen;

#ಯದಿ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  ಯದಿ defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
        retlen = SecRandomCopyBytes(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  ಅನ್ಯಥಾ
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#ನೋಚೇದ್ಯದಿ defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#ಅನ್ಯಥಾ
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOSYS;
#endif

        ಯದಿ (retlen < 0) {
            ಯದಿ (errno == EINTR || errno == EAGAIN) {
                ಅನುವರ್ತ;
            }
            ಪ್ರತ್ಯಾಗಮ errno_to_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
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
ಸ್ಥಿರ
uint64_t poll_remaining_ns (ಧ್ರುವ m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ಯದಿ (not i_sub->isAbsolute) {
        ಪ್ರತ್ಯಾಗಮ (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    ಯದಿ (clk == d_m3ClockIdInvalid) {
        ಪ್ರತ್ಯಾಗಮ UINT64_MAX;
    }

    ಸಂರಚನಾ timespec tp;
    ಯದಿ (clock_gettime(clk, &tp) != 0) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    ಪ್ರತ್ಯಾಗಮ (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
ಸ್ಥಿರ
__wasi_filesize_t poll_readable_bytes (ಪೂರ್ಣಾಙ್ಕ i_fd)
{
#ಯದಿ defined(FIONREAD)
    ಪೂರ್ಣಾಙ್ಕ available = 0;
    ಯದಿ (ioctl(i_fd, FIONREAD, &available) == 0 and available > 0) {
        ಪ್ರತ್ಯಾಗಮ (__wasi_filesize_t)available;
    }
#endif
    ಪ್ರತ್ಯಾಗಮ 1;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_poll_oneoff (ಧ್ರುವ m3_wasi_pollsub_t* i_subs, __wasi_size_t i_ಎಣಿಕೆ,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_ಎಣಿಕೆ)
{
    *o_event_ಎಣಿಕೆ = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t ಅಂಕೆ_Fds = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಯದಿ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_remaining_ns(&i_subs[i], 0);
            ಯದಿ (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } ಅನ್ಯಥಾ {
            ಅಂಕೆ_Fds++;
        }
    }

    ಸಂರಚನಾ pollfd* pollfds = ರಿಕ್ತ;

    ಯದಿ (ಅಂಕೆ_Fds) {
        pollfds = (ಸಂರಚನಾ pollfd*)m3_Malloc("WASI poll_oneoff", ಅಂಕೆ_Fds * ಪರಿಮಾಣ(ಸಂರಚನಾ pollfd));
        ಯದಿ (pollfds == ರಿಕ್ತ) {
            ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        ಪ್ರತಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
            ಯದಿ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                ಅನುವರ್ತ;
            }
            pollfds[n].fd = (ಪೂರ್ಣಾಙ್ಕ)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    ಪೂರ್ಣಾಙ್ಕ timeoutMs = -1;
    ಯದಿ (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)INT_MAX) ? INT_MAX : (ಪೂರ್ಣಾಙ್ಕ)ms;
    }

    ಪೂರ್ಣಾಙ್ಕ ready;
    ಯದಿ (ಅಂಕೆ_Fds) {
        ಕುರು {
            ready = poll(pollfds, (nfds_t)ಅಂಕೆ_Fds, timeoutMs);
        } ಯಾವತ್ (ready < 0 and errno == EINTR);
    } ಅನ್ಯಥಾ ಯದಿ (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        ಸಂರಚನಾ timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (long)(soonest % 1000000000);

        ಯಾವತ್ (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } ಅನ್ಯಥಾ ಯದಿ (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } ಅನ್ಯಥಾ {
        // no clocks and no descriptors: nothing that could ever become ready
        ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_INVAL;
    }

    ಯದಿ (ready < 0) {
        __wasi_errno_t err = errno_to_wasi(errno);
        m3_Free(pollfds);
        ಪ್ರತ್ಯಾಗಮ err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t out = 0;
    __wasi_size_t n = 0;

    ಪ್ರತಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಧ್ರುವ m3_wasi_pollsub_t* sub = &i_subs[i];

        ಯದಿ (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            ಯದಿ (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_INVAL;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            } ಅನ್ಯಥಾ ಯದಿ (poll_remaining_ns(sub, elapsedNs) == 0) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_SUCCESS;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            }
            ಅನುವರ್ತ;
        }

        short revents = pollfds[n].revents;
        n++;

        ಯದಿ (revents == 0) {
            ಅನುವರ್ತ;
        }

        o_events[out].userdata = sub->userdata;
        o_events[out].type = sub->type;
        o_events[out].nbytes = 0;
        o_events[out].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        ಯದಿ (revents & POLLNVAL) {
            o_events[out].error = __WASI_ERRNO_BADF;
        } ಅನ್ಯಥಾ ಯದಿ (revents & POLLERR) {
            o_events[out].error = __WASI_ERRNO_IO;
        } ಅನ್ಯಥಾ {
            o_events[out].error = __WASI_ERRNO_SUCCESS;
            ಯದಿ (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[out].nbytes = poll_readable_bytes(pollfds[n - 1].fd);
            }
        }

        out++;
    }

    m3_Free(pollfds);

    *o_event_ಎಣಿಕೆ = out;
    ಪ್ರತ್ಯಾಗಮ __WASI_ERRNO_SUCCESS;
}

#endif // m3_api_wasi_posix_h
