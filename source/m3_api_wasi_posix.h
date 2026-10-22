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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_api_wasi_posix_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_api_wasi_posix_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಆದರೆ defined(_WIN32)
#  error "This file is not for Windows"
#ಆದರೆ_ಅಂತ್ಯ

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
#ಆದರೆ !defined(__wasi__)
#  include <sys/ioctl.h>
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  ಆದರೆ defined(__APPLE__)
#    include <TargetConditionals.h>
#    ಆದರೆ ಗುರಿ_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    ಇಲ್ಲದಿದ್ದರೆ // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  ಇಲ್ಲದಿದ್ದರೆ
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "m3_api_wasi_host.h"


/*
 * Plumbing
 */

ಸ್ಥಿರ
__wasi_errno_t errno_ಗಮ್ಯ_wasi (ಪೂರ್ಣಾಂಕ errnum)
{
    // clang-format off
    ಆಯ್ಕೆ (errnum) {
    ಸ್ಥಿತಿ EPERM:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_PERM;
    ಸ್ಥಿತಿ ENOENT:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOENT;
    ಸ್ಥಿತಿ ESRCH:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SRCH;
    ಸ್ಥಿತಿ EINTR:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_INTR;
    ಸ್ಥಿತಿ EIO:      ಹಿಂತಿರುಗಿ __WASI_ERRNO_IO;
    ಸ್ಥಿತಿ ENXIO:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_NXIO;
    ಸ್ಥಿತಿ E2BIG:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_2BIG;
    ಸ್ಥಿತಿ ENOEXEC:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOEXEC;
    ಸ್ಥಿತಿ EBADF:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    ಸ್ಥಿತಿ ECHILD:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_CHILD;
    ಸ್ಥಿತಿ EAGAIN:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_AGAIN;
    ಸ್ಥಿತಿ ENOMEM:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOMEM;
    ಸ್ಥಿತಿ EACCES:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_ACCES;
    ಸ್ಥಿತಿ EFAULT:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_FAULT;
    ಸ್ಥಿತಿ EBUSY:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_BUSY;
    ಸ್ಥಿತಿ EEXIST:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_EXIST;
    ಸ್ಥಿತಿ EXDEV:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_XDEV;
    ಸ್ಥಿತಿ ENODEV:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_NODEV;
    ಸ್ಥಿತಿ ENOTDIR:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOTDIR;
    ಸ್ಥಿತಿ EISDIR:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_ISDIR;
    ಸ್ಥಿತಿ EINVAL:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    ಸ್ಥಿತಿ ENFILE:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_NFILE;
    ಸ್ಥಿತಿ EMFILE:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_MFILE;
    ಸ್ಥಿತಿ ENOTTY:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOTTY;
    ಸ್ಥಿತಿ ETXTBSY:  ಹಿಂತಿರುಗಿ __WASI_ERRNO_TXTBSY;
    ಸ್ಥಿತಿ EFBIG:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_FBIG;
    ಸ್ಥಿತಿ ENOSPC:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOSPC;
    ಸ್ಥಿತಿ ESPIPE:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_SPIPE;
    ಸ್ಥಿತಿ EROFS:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_ROFS;
    ಸ್ಥಿತಿ EMLINK:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_MLINK;
    ಸ್ಥಿತಿ EPIPE:    ಹಿಂತಿರುಗಿ __WASI_ERRNO_PIPE;
    ಸ್ಥಿತಿ EDOM:     ಹಿಂತಿರುಗಿ __WASI_ERRNO_DOM;
    ಸ್ಥಿತಿ ERANGE:   ಹಿಂತಿರುಗಿ __WASI_ERRNO_RANGE;
    }
    // clang-format on
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
}

ಸ್ಥಿರ
__wasi_filetype_t filetype_from_stat_ಬಹುಲಕ (ಪೂರ್ಣಾಂಕ ಬಹುಲಕ)
{
    ಹಿಂತಿರುಗಿ (S_ISBLK(ಬಹುಲಕ) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(ಬಹುಲಕ) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(ಬಹುಲಕ) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(ಬಹುಲಕ) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(ಬಹುಲಕ) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

ಸ್ಥಿರ
ಶೂನ್ಯ filestat_from_stat (ಸ್ಥಿರಾಂಕ ರಚನೆ stat* i_stat, m3_wasi_filestat_t* o_stat)
{
    o_stat->dev = (uint64_t)i_stat->st_dev;
    o_stat->ino = (uint64_t)i_stat->st_ino;
    o_stat->nlink = (uint64_t)i_stat->st_nlink;
    o_stat->ಗಾತ್ರ_2 = (uint64_t)i_stat->st_size;
    o_stat->atim = (uint64_t)i_stat->st_atime * 1000000000ULL;
    o_stat->mtim = (uint64_t)i_stat->st_mtime * 1000000000ULL;
    o_stat->ctim = (uint64_t)i_stat->st_ctime * 1000000000ULL;
    o_stat->filetype = filetype_from_stat_ಬಹುಲಕ(i_stat->st_mode);
}

// The descriptor a preopened directory holds, or -1 where i_fd names no preopen
ಸ್ಥಿರ
ಪೂರ್ಣಾಂಕ preopen_fd (__wasi_fd_t i_fd)
{
    ಆದರೆ (i_fd >= d_m3_Wasi_Preopen_ಎಣಿಕೆ) {
        ಹಿಂತಿರುಗಿ -1;
    }
    ಹಿಂತಿರುಗಿ m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#ಆದರೆ defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) ಖಾಲಿ)
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#ಆದರೆ_ಅಂತ್ಯ

ಸ್ಥಿರ
clockid_t convert_clockid (__wasi_clockid_t ಒಳ)
{
    ಆಯ್ಕೆ (ಒಳ) {
    ಸ್ಥಿತಿ __WASI_CLOCKID_MONOTONIC: ಹಿಂತಿರುಗಿ CLOCK_MONOTONIC;
    ಸ್ಥಿತಿ __WASI_CLOCKID_REALTIME: ಹಿಂತಿರುಗಿ CLOCK_REALTIME;
#ಆದರೆ defined(CLOCK_PROCESS_CPUTIME_ID)
    ಸ್ಥಿತಿ __WASI_CLOCKID_PROCESS_CPUTIME_ID: ಹಿಂತಿರುಗಿ CLOCK_PROCESS_CPUTIME_ID;
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ defined(CLOCK_THREAD_CPUTIME_ID)
    ಸ್ಥಿತಿ __WASI_CLOCKID_THREAD_CPUTIME_ID: ಹಿಂತಿರುಗಿ CLOCK_THREAD_CPUTIME_ID;
#ಆದರೆ_ಅಂತ್ಯ
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ d_m3ClockIdInvalid;
    }
}

ಸ್ಥಿರ
__wasi_timestamp_t convert_timespec (ಸ್ಥಿರಾಂಕ ರಚನೆ timespec* ts)
{
    ಆದರೆ (ts->tv_sec < 0) {
        ಹಿಂತಿರುಗಿ 0;
    }
    ಆದರೆ ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        ಹಿಂತಿರುಗಿ UINT64_MAX;
    }
    ಹಿಂತಿರುಗಿ (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

ಸ್ಥಿರ
ಶೂನ್ಯ m3_wasi_host_init ()
{
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 3; i < d_m3_Wasi_Preopen_ಎಣಿಕೆ; i++) {
        ಆದರೆ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_ಮಾರ್ಗ, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ಆದರೆ (close(i_fd) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#ಆದರೆ defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#ಅಥವಾಆದರೆ defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#ಆದರೆ_ಅಂತ್ಯ

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#ಆದರೆ defined(d_m3WasiDatasync)
    ಆದರೆ (d_m3WasiDatasync(i_fd) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOSYS;
#ಆದರೆ_ಅಂತ್ಯ
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ,
                                       __wasi_filesize_t i_ಉದ್ದಳತೆ, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#ಆದರೆ defined(POSIX_FADV_NORMAL)
    ಪೂರ್ಣಾಂಕ adv;
    ಆಯ್ಕೆ (i_advice) {
    ಸ್ಥಿತಿ __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }

    ಪೂರ್ಣಾಂಕ ret = posix_fadvise(i_fd, i_ಸ್ಥಾನಾಂತರ, i_ಉದ್ದಳತೆ, adv);
    ಆದರೆ (ret != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(ret);
    }
#ಇಲ್ಲದಿದ್ದರೆ
    ಆದರೆ (i_advice > __WASI_ADVICE_NOREUSE) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }

    ರಚನೆ stat fd_stat;
    ಆದರೆ (fstat(i_fd, &fd_stat) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
#ಆದರೆ_ಅಂತ್ಯ

    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    ಪೂರ್ಣಾಂಕ fl = fcntl(i_fd, F_GETFL);
    ಆದರೆ (fl < 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    ರಚನೆ stat fd_stat;
    ಆದರೆ (fstat(i_fd, &fd_stat) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    *o_filetype = filetype_from_stat_ಬಹುಲಕ(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_fdstat_ಗಣ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    ಪೂರ್ಣಾಂಕ fl = fcntl(i_fd, F_GETFL);
    ಆದರೆ (fl < 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    ಸ್ಥಿರಾಂಕ __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    ಆದರೆ ((i_flags & fixed) != (current & fixed)) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    ಆದರೆ (fcntl(i_fd, F_SETFL, fl) < 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ಗಾತ್ರ(*o_stat));

    ರಚನೆ stat fd_stat;
    ಆದರೆ (fstat(i_fd, &fd_stat) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_ಗಣ_ಗಾತ್ರ (__wasi_fd_t i_fd, __wasi_filesize_t i_ಗಾತ್ರ)
{
    ಆದರೆ (ftruncate(i_fd, (off_t)i_ಗಾತ್ರ) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_ಗಣ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    ರಚನೆ timespec times[2];

    ಆದರೆ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } ಇಲ್ಲದಿದ್ದರೆ {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    ಆದರೆ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } ಇಲ್ಲದಿದ್ದರೆ {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    ಆದರೆ (futimens(i_fd, times) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_ಸ್ಥಾನಾಂತರ, ಪೂರ್ಣಾಂಕ i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_ಸ್ಥಾನಾಂತರ, i_whence);
    ಆದರೆ (ret < 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#ಆದರೆ defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
ಸ್ಥಿರ
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_ಮುಗಿದ)
{
    *o_ಮುಗಿದ = 0;

    ರಚನೆ iovec iovs[d_m3WasiMaxIovs];
    ಗಾಗಿ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (ಪೂರ್ಣಾಂಕ)i_iovsLen) : readv(i_fd, iovs, (ಪೂರ್ಣಾಂಕ)i_iovsLen);
    ಆದರೆ (ret < 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    *o_ಮುಗಿದ = (__wasi_size_t)ret;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

#ಇಲ್ಲದಿದ್ದರೆ

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
ಸ್ಥಿರ
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_ಮುಗಿದ)
{
    *o_ಮುಗಿದ = 0;

    size_t ಮುಗಿದ = 0;

    ಗಾಗಿ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ಶೂನ್ಯ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ಆದರೆ (len == 0) {
            ಮುಂದುವರಿ;
        }

        ssize_t ret = i_write ? ಲಿಖ(i_fd, addr, len) : ಪಠ(i_fd, addr, len);
        ಆದರೆ (ret < 0) {
            ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
        }

        ಮುಗಿದ += (size_t)ret;
        ಆದರೆ ((size_t)ret < len) {
            ನಿಲ್ಲಿಸು;
        }
    }

    *o_ಮುಗಿದ = (__wasi_size_t)ಮುಗಿದ;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
ಸ್ಥಿರ
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ, bool i_write,
                            __wasi_size_t* o_ಮುಗಿದ)
{
    *o_ಮುಗಿದ = 0;

    size_t ಮುಗಿದ = 0;

    ಗಾಗಿ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ಶೂನ್ಯ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ಆದರೆ (len == 0) {
            ಮುಂದುವರಿ;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_ಸ್ಥಾನಾಂತರ + ಮುಗಿದ))
                              : pread(i_fd, addr, len, (off_t)(i_ಸ್ಥಾನಾಂತರ + ಮುಗಿದ));
        ಆದರೆ (ret < 0) {
            ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
        }

        ಮುಗಿದ += (size_t)ret;
        ಆದರೆ ((size_t)ret < len) {
            ನಿಲ್ಲಿಸು;
        }
    }

    *o_ಮುಗಿದ = (__wasi_size_t)ಮುಗಿದ;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_ಮುಗಿದ);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_ಮುಗಿದ);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ,
                                      __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_at(i_fd, i_iovs, i_iovsLen, i_ಸ್ಥಾನಾಂತರ, false, o_ಮುಗಿದ);
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ಸ್ಥಿರಾಂಕ m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_ಸ್ಥಾನಾಂತರ,
                                       __wasi_size_t* o_ಮುಗಿದ)
{
    ಹಿಂತಿರುಗಿ transfer_at(i_fd, i_iovs, i_iovsLen, i_ಸ್ಥಾನಾಂತರ, true, o_ಮುಗಿದ);
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

    ಪೂರ್ಣಾಂಕ dirfd = preopen_fd(i_dirfd);
    ಆದರೆ (dirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    ಪೂರ್ಣಾಂಕ flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    ಆದರೆ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    ಪೂರ್ಣಾಂಕ host_fd = openat(dirfd, i_ಮಾರ್ಗ, flags, 0644);
    ಆದರೆ (host_fd < 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ಗಾತ್ರ(*o_stat));

    ಪೂರ್ಣಾಂಕ dirfd = preopen_fd(i_dirfd);
    ಆದರೆ (dirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ರಚನೆ stat ಮಾರ್ಗ_stat;
    ಆದರೆ (fstatat(dirfd, i_ಮಾರ್ಗ, &ಮಾರ್ಗ_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    filestat_from_stat(&ಮಾರ್ಗ_stat, o_stat);
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಪೂರ್ಣಾಂಕ dirfd = preopen_fd(i_dirfd);
    ಆದರೆ (dirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ಆದರೆ (mkdirat(dirfd, i_ಮಾರ್ಗ, 0755) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಪೂರ್ಣಾಂಕ dirfd = preopen_fd(i_dirfd);
    ಆದರೆ (dirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ಆದರೆ (unlinkat(dirfd, i_ಮಾರ್ಗ, AT_REMOVEDIR) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಪೂರ್ಣಾಂಕ dirfd = preopen_fd(i_dirfd);
    ಆದರೆ (dirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ಆದರೆ (unlinkat(dirfd, i_ಮಾರ್ಗ, 0) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ಮಾರ್ಗ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_ಮಾರ್ಗ)
{
    ಪೂರ್ಣಾಂಕ oldDirfd = preopen_fd(i_oldDirfd);
    ಪೂರ್ಣಾಂಕ newDirfd = preopen_fd(i_newDirfd);
    ಆದರೆ (oldDirfd < 0 || newDirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ಆದರೆ (renameat(oldDirfd, i_old_ಮಾರ್ಗ, newDirfd, i_new_ಮಾರ್ಗ) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ಮಾರ್ಗ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_ಮಾರ್ಗ)
{
    ಪೂರ್ಣಾಂಕ oldDirfd = preopen_fd(i_oldDirfd);
    ಪೂರ್ಣಾಂಕ newDirfd = preopen_fd(i_newDirfd);
    ಆದರೆ (oldDirfd < 0 || newDirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ಆದರೆ (linkat(oldDirfd, i_old_ಮಾರ್ಗ, newDirfd, i_new_ಮಾರ್ಗ, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_symlink (ccstr_t i_ಗುರಿ, __wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ)
{
    ಪೂರ್ಣಾಂಕ dirfd = preopen_fd(i_dirfd);
    ಆದರೆ (dirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ಆದರೆ (symlinkat(i_ಗುರಿ, dirfd, i_ಮಾರ್ಗ) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_ಮಾರ್ಗ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_ಮಾರ್ಗ, ಅಕ್ಷರ* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_ಬಳಸಿದ)
{
    *o_buf_ಬಳಸಿದ = 0;

    ಪೂರ್ಣಾಂಕ dirfd = preopen_fd(i_dirfd);
    ಆದರೆ (dirfd < 0) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_ಮಾರ್ಗ, o_buf, i_bufLen);
    ಆದರೆ (ret < 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    *o_buf_ಬಳಸಿದ = (__wasi_size_t)ret;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ಆದರೆ (clk == d_m3ClockIdInvalid) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }

    ರಚನೆ timespec tp;
    ಆದರೆ (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } ಇಲ್ಲದಿದ್ದರೆ {
        *o_res = convert_timespec(&tp);
    }
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_clock_ಸಮಯ_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_ಸಮಯ)
{
    *o_ಸಮಯ = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ಆದರೆ (clk == d_m3ClockIdInvalid) {
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }

    ರಚನೆ timespec tp;
    ಆದರೆ (clock_gettime(clk, &tp) != 0) {
        ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
    }

    *o_ಸಮಯ = convert_timespec(&tp);
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_random_get (ಶೂನ್ಯ* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    ಆಗ (i_len) {
        ssize_t retlen;

#ಆದರೆ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  ಆದರೆ defined(__APPLE__) && (ಗುರಿ_OS_IPHONE || ಗುರಿ_IPHONE_SIMULATOR)
        retlen = Sec_Random_Copy_ಬೈಟುಗಳು(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  ಇಲ್ಲದಿದ್ದರೆ
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#ಅಥವಾಆದರೆ defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#ಇಲ್ಲದಿದ್ದರೆ
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOSYS;
#ಆದರೆ_ಅಂತ್ಯ

        ಆದರೆ (retlen < 0) {
            ಆದರೆ (errno == EINTR || errno == EAGAIN) {
                ಮುಂದುವರಿ;
            }
            ಹಿಂತಿರುಗಿ errno_ಗಮ್ಯ_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
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
uint64_t poll_ಉಳಿದ_ns (ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ಆದರೆ (not i_sub->isAbsolute) {
        ಹಿಂತಿರುಗಿ (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    ಆದರೆ (clk == d_m3ClockIdInvalid) {
        ಹಿಂತಿರುಗಿ UINT64_MAX;
    }

    ರಚನೆ timespec tp;
    ಆದರೆ (clock_gettime(clk, &tp) != 0) {
        ಹಿಂತಿರುಗಿ 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    ಹಿಂತಿರುಗಿ (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
ಸ್ಥಿರ
__wasi_filesize_t poll_readable_ಬೈಟುಗಳು (ಪೂರ್ಣಾಂಕ i_fd)
{
#ಆದರೆ defined(FIONREAD)
    ಪೂರ್ಣಾಂಕ ಲಭ್ಯ = 0;
    ಆದರೆ (ioctl(i_fd, FIONREAD, &ಲಭ್ಯ) == 0 and ಲಭ್ಯ > 0) {
        ಹಿಂತಿರುಗಿ (__wasi_filesize_t)ಲಭ್ಯ;
    }
#ಆದರೆ_ಅಂತ್ಯ
    ಹಿಂತಿರುಗಿ 1;
}

ಸ್ಥಿರ
__wasi_errno_t m3_wasi_host_poll_oneoff (ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* i_subs, __wasi_size_t i_ಎಣಿಕೆ,
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

    ಗಾಗಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಆದರೆ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_ಉಳಿದ_ns(&i_subs[i], 0);
            ಆದರೆ (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಅಂಕೆ_Fds++;
        }
    }

    ರಚನೆ pollfd* pollfds = ಖಾಲಿ;

    ಆದರೆ (ಅಂಕೆ_Fds) {
        pollfds = (ರಚನೆ pollfd*)m3_Malloc("WASI poll_oneoff", ಅಂಕೆ_Fds * ಗಾತ್ರ(ರಚನೆ pollfd));
        ಆದರೆ (pollfds == ಖಾಲಿ) {
            ಹಿಂತಿರುಗಿ __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        ಗಾಗಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
            ಆದರೆ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                ಮುಂದುವರಿ;
            }
            pollfds[n].fd = (ಪೂರ್ಣಾಂಕ)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    ಪೂರ್ಣಾಂಕ timeoutMs = -1;
    ಆದರೆ (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)ಗರಿಷ್ಠ_ಪೂರ್ಣಾಂಕ) ? ಗರಿಷ್ಠ_ಪೂರ್ಣಾಂಕ : (ಪೂರ್ಣಾಂಕ)ms;
    }

    ಪೂರ್ಣಾಂಕ ready;
    ಆದರೆ (ಅಂಕೆ_Fds) {
        ಮಾಡು {
            ready = poll(pollfds, (nfds_t)ಅಂಕೆ_Fds, timeoutMs);
        } ಆಗ (ready < 0 and errno == EINTR);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        ರಚನೆ timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (ದೀರ್ಘ)(soonest % 1000000000);

        ಆಗ (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } ಇಲ್ಲದಿದ್ದರೆ {
        // no clocks and no descriptors: nothing that could ever become ready
        ಹಿಂತಿರುಗಿ __WASI_ERRNO_INVAL;
    }

    ಆದರೆ (ready < 0) {
        __wasi_errno_t err = errno_ಗಮ್ಯ_wasi(errno);
        m3_Free(pollfds);
        ಹಿಂತಿರುಗಿ err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t ಹೊರಹೋಗು = 0;
    __wasi_size_t n = 0;

    ಗಾಗಿ (__wasi_size_t i = 0; i < i_ಎಣಿಕೆ; i++) {
        ಸ್ಥಿರಾಂಕ m3_wasi_pollsub_t* sub = &i_subs[i];

        ಆದರೆ (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            ಆದರೆ (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[ಹೊರಹೋಗು].userdata = sub->userdata;
                o_events[ಹೊರಹೋಗು].error = __WASI_ERRNO_INVAL;
                o_events[ಹೊರಹೋಗು].type = __WASI_EVENTTYPE_CLOCK;
                o_events[ಹೊರಹೋಗು].nbytes = 0;
                o_events[ಹೊರಹೋಗು].flags = 0;
                ಹೊರಹೋಗು++;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (poll_ಉಳಿದ_ns(sub, elapsedNs) == 0) {
                o_events[ಹೊರಹೋಗು].userdata = sub->userdata;
                o_events[ಹೊರಹೋಗು].error = __WASI_ERRNO_SUCCESS;
                o_events[ಹೊರಹೋಗು].type = __WASI_EVENTTYPE_CLOCK;
                o_events[ಹೊರಹೋಗು].nbytes = 0;
                o_events[ಹೊರಹೋಗು].flags = 0;
                ಹೊರಹೋಗು++;
            }
            ಮುಂದುವರಿ;
        }

        ಗಿಡ್ಡ revents = pollfds[n].revents;
        n++;

        ಆದರೆ (revents == 0) {
            ಮುಂದುವರಿ;
        }

        o_events[ಹೊರಹೋಗು].userdata = sub->userdata;
        o_events[ಹೊರಹೋಗು].type = sub->type;
        o_events[ಹೊರಹೋಗು].nbytes = 0;
        o_events[ಹೊರಹೋಗು].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        ಆದರೆ (revents & POLLNVAL) {
            o_events[ಹೊರಹೋಗು].error = __WASI_ERRNO_BADF;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (revents & POLLERR) {
            o_events[ಹೊರಹೋಗು].error = __WASI_ERRNO_IO;
        } ಇಲ್ಲದಿದ್ದರೆ {
            o_events[ಹೊರಹೋಗು].error = __WASI_ERRNO_SUCCESS;
            ಆದರೆ (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[ಹೊರಹೋಗು].nbytes = poll_readable_ಬೈಟುಗಳು(pollfds[n - 1].fd);
            }
        }

        ಹೊರಹೋಗು++;
    }

    m3_Free(pollfds);

    *o_event_ಎಣಿಕೆ = ಹೊರಹೋಗು;
    ಹಿಂತಿರುಗಿ __WASI_ERRNO_SUCCESS;
}

#ಆದರೆ_ಅಂತ್ಯ // m3_api_wasi_posix_h
