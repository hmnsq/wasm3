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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_api_wasi_posix_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_api_wasi_posix_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

#ਜੇ defined(_WIN32)
#  error "This file is not for Windows"
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ <sys/types.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/stat.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <errno.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <fcntl.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <poll.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <string.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#ਜੇ !defined(__wasi__)
#  include <sys/ioctl.h>
#ਜੇ_ਅੰਤ

#ਜੇ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  ਜੇ defined(__APPLE__)
#    include <TargetConditionals.h>
#    ਜੇ ਨਿਸ਼ਾਨਾ_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    ਨਹੀਂਤਾਂ // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  ਨਹੀਂਤਾਂ
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_api_wasi_host.h"


/*
 * Plumbing
 */

ਸਥਿਰ
__wasi_errno_t errno_ਮੰਜ਼ਿਲ_wasi (ਪੂਰਨ errnum)
{
    // clang-format off
    ਚੋਣ (errnum) {
    ਹਾਲਤ EPERM:    ਵਾਪਸ __WASI_ERRNO_PERM;
    ਹਾਲਤ ENOENT:   ਵਾਪਸ __WASI_ERRNO_NOENT;
    ਹਾਲਤ ESRCH:    ਵਾਪਸ __WASI_ERRNO_SRCH;
    ਹਾਲਤ EINTR:    ਵਾਪਸ __WASI_ERRNO_INTR;
    ਹਾਲਤ EIO:      ਵਾਪਸ __WASI_ERRNO_IO;
    ਹਾਲਤ ENXIO:    ਵਾਪਸ __WASI_ERRNO_NXIO;
    ਹਾਲਤ E2BIG:    ਵਾਪਸ __WASI_ERRNO_2BIG;
    ਹਾਲਤ ENOEXEC:  ਵਾਪਸ __WASI_ERRNO_NOEXEC;
    ਹਾਲਤ EBADF:    ਵਾਪਸ __WASI_ERRNO_BADF;
    ਹਾਲਤ ECHILD:   ਵਾਪਸ __WASI_ERRNO_CHILD;
    ਹਾਲਤ EAGAIN:   ਵਾਪਸ __WASI_ERRNO_AGAIN;
    ਹਾਲਤ ENOMEM:   ਵਾਪਸ __WASI_ERRNO_NOMEM;
    ਹਾਲਤ EACCES:   ਵਾਪਸ __WASI_ERRNO_ACCES;
    ਹਾਲਤ EFAULT:   ਵਾਪਸ __WASI_ERRNO_FAULT;
    ਹਾਲਤ EBUSY:    ਵਾਪਸ __WASI_ERRNO_BUSY;
    ਹਾਲਤ EEXIST:   ਵਾਪਸ __WASI_ERRNO_EXIST;
    ਹਾਲਤ EXDEV:    ਵਾਪਸ __WASI_ERRNO_XDEV;
    ਹਾਲਤ ENODEV:   ਵਾਪਸ __WASI_ERRNO_NODEV;
    ਹਾਲਤ ENOTDIR:  ਵਾਪਸ __WASI_ERRNO_NOTDIR;
    ਹਾਲਤ EISDIR:   ਵਾਪਸ __WASI_ERRNO_ISDIR;
    ਹਾਲਤ EINVAL:   ਵਾਪਸ __WASI_ERRNO_INVAL;
    ਹਾਲਤ ENFILE:   ਵਾਪਸ __WASI_ERRNO_NFILE;
    ਹਾਲਤ EMFILE:   ਵਾਪਸ __WASI_ERRNO_MFILE;
    ਹਾਲਤ ENOTTY:   ਵਾਪਸ __WASI_ERRNO_NOTTY;
    ਹਾਲਤ ETXTBSY:  ਵਾਪਸ __WASI_ERRNO_TXTBSY;
    ਹਾਲਤ EFBIG:    ਵਾਪਸ __WASI_ERRNO_FBIG;
    ਹਾਲਤ ENOSPC:   ਵਾਪਸ __WASI_ERRNO_NOSPC;
    ਹਾਲਤ ESPIPE:   ਵਾਪਸ __WASI_ERRNO_SPIPE;
    ਹਾਲਤ EROFS:    ਵਾਪਸ __WASI_ERRNO_ROFS;
    ਹਾਲਤ EMLINK:   ਵਾਪਸ __WASI_ERRNO_MLINK;
    ਹਾਲਤ EPIPE:    ਵਾਪਸ __WASI_ERRNO_PIPE;
    ਹਾਲਤ EDOM:     ਵਾਪਸ __WASI_ERRNO_DOM;
    ਹਾਲਤ ERANGE:   ਵਾਪਸ __WASI_ERRNO_RANGE;
    }
    // clang-format on
    ਵਾਪਸ __WASI_ERRNO_INVAL;
}

ਸਥਿਰ
__wasi_filetype_t filetype_from_stat_ਬਹੁਲਕ (ਪੂਰਨ ਬਹੁਲਕ)
{
    ਵਾਪਸ (S_ISBLK(ਬਹੁਲਕ) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(ਬਹੁਲਕ) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(ਬਹੁਲਕ) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(ਬਹੁਲਕ) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(ਬਹੁਲਕ) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

ਸਥਿਰ
ਖਾਲੀ filestat_from_stat (ਅਚਲ ਬਣਤਰ stat* i_stat, m3_wasi_filestat_t* o_stat)
{
    o_stat->dev = (uint64_t)i_stat->st_dev;
    o_stat->ino = (uint64_t)i_stat->st_ino;
    o_stat->nlink = (uint64_t)i_stat->st_nlink;
    o_stat->ਮਾਪ = (uint64_t)i_stat->st_size;
    o_stat->atim = (uint64_t)i_stat->st_atime * 1000000000ULL;
    o_stat->mtim = (uint64_t)i_stat->st_mtime * 1000000000ULL;
    o_stat->ctim = (uint64_t)i_stat->st_ctime * 1000000000ULL;
    o_stat->filetype = filetype_from_stat_ਬਹੁਲਕ(i_stat->st_mode);
}

// The descriptor a preopened directory holds, or -1 where i_fd names no preopen
ਸਥਿਰ
ਪੂਰਨ preopen_fd (__wasi_fd_t i_fd)
{
    ਜੇ (i_fd >= d_m3_Wasi_Preopen_ਗਿਣਤੀ) {
        ਵਾਪਸ -1;
    }
    ਵਾਪਸ m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#ਜੇ defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) ਨਿਰਾ)
#ਨਹੀਂਤਾਂ
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#ਜੇ_ਅੰਤ

ਸਥਿਰ
clockid_t convert_clockid (__wasi_clockid_t ਵਿੱਚ)
{
    ਚੋਣ (ਵਿੱਚ) {
    ਹਾਲਤ __WASI_CLOCKID_MONOTONIC: ਵਾਪਸ CLOCK_MONOTONIC;
    ਹਾਲਤ __WASI_CLOCKID_REALTIME: ਵਾਪਸ CLOCK_REALTIME;
#ਜੇ defined(CLOCK_PROCESS_CPUTIME_ID)
    ਹਾਲਤ __WASI_CLOCKID_PROCESS_CPUTIME_ID: ਵਾਪਸ CLOCK_PROCESS_CPUTIME_ID;
#ਜੇ_ਅੰਤ
#ਜੇ defined(CLOCK_THREAD_CPUTIME_ID)
    ਹਾਲਤ __WASI_CLOCKID_THREAD_CPUTIME_ID: ਵਾਪਸ CLOCK_THREAD_CPUTIME_ID;
#ਜੇ_ਅੰਤ
    ਮੂਲ: ਵਾਪਸ d_m3ClockIdInvalid;
    }
}

ਸਥਿਰ
__wasi_timestamp_t convert_timespec (ਅਚਲ ਬਣਤਰ timespec* ts)
{
    ਜੇ (ts->tv_sec < 0) {
        ਵਾਪਸ 0;
    }
    ਜੇ ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        ਵਾਪਸ UINT64_MAX;
    }
    ਵਾਪਸ (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

ਸਥਿਰ
ਖਾਲੀ m3_wasi_host_init ()
{
    ਲਈ (ਪੂਰਨ i = 3; i < d_m3_Wasi_Preopen_ਗਿਣਤੀ; i++) {
        ਜੇ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_ਰਸਤਾ, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ਜੇ (close(i_fd) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#ਜੇ defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#ਜਾਂਜੇ defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#ਜੇ_ਅੰਤ

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#ਜੇ defined(d_m3WasiDatasync)
    ਜੇ (d_m3WasiDatasync(i_fd) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ __WASI_ERRNO_NOSYS;
#ਜੇ_ਅੰਤ
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_ਵਿੱਥ,
                                       __wasi_filesize_t i_ਲੰਮਾਈ, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#ਜੇ defined(POSIX_FADV_NORMAL)
    ਪੂਰਨ adv;
    ਚੋਣ (i_advice) {
    ਹਾਲਤ __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; ਰੋਕੋ;
    ਹਾਲਤ __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; ਰੋਕੋ;
    ਹਾਲਤ __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; ਰੋਕੋ;
    ਹਾਲਤ __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; ਰੋਕੋ;
    ਹਾਲਤ __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; ਰੋਕੋ;
    ਹਾਲਤ __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; ਰੋਕੋ;
    ਮੂਲ: ਵਾਪਸ __WASI_ERRNO_INVAL;
    }

    ਪੂਰਨ ret = posix_fadvise(i_fd, i_ਵਿੱਥ, i_ਲੰਮਾਈ, adv);
    ਜੇ (ret != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(ret);
    }
#ਨਹੀਂਤਾਂ
    ਜੇ (i_advice > __WASI_ADVICE_NOREUSE) {
        ਵਾਪਸ __WASI_ERRNO_INVAL;
    }

    ਬਣਤਰ stat fd_stat;
    ਜੇ (fstat(i_fd, &fd_stat) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
#ਜੇ_ਅੰਤ

    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    ਪੂਰਨ fl = fcntl(i_fd, F_GETFL);
    ਜੇ (fl < 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    ਬਣਤਰ stat fd_stat;
    ਜੇ (fstat(i_fd, &fd_stat) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    *o_filetype = filetype_from_stat_ਬਹੁਲਕ(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_fdstat_ਸੈੱਟ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    ਪੂਰਨ fl = fcntl(i_fd, F_GETFL);
    ਜੇ (fl < 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    ਅਚਲ __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    ਜੇ ((i_flags & fixed) != (current & fixed)) {
        ਵਾਪਸ __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    ਜੇ (fcntl(i_fd, F_SETFL, fl) < 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ਆਕਾਰ(*o_stat));

    ਬਣਤਰ stat fd_stat;
    ਜੇ (fstat(i_fd, &fd_stat) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_ਸੈੱਟ_ਮਾਪ (__wasi_fd_t i_fd, __wasi_filesize_t i_ਮਾਪ)
{
    ਜੇ (ftruncate(i_fd, (off_t)i_ਮਾਪ) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_ਸੈੱਟ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    ਬਣਤਰ timespec times[2];

    ਜੇ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } ਨਹੀਂਤਾਂ ਜੇ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } ਨਹੀਂਤਾਂ {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    ਜੇ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } ਨਹੀਂਤਾਂ ਜੇ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } ਨਹੀਂਤਾਂ {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    ਜੇ (futimens(i_fd, times) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_ਵਿੱਥ, ਪੂਰਨ i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_ਵਿੱਥ, i_whence);
    ਜੇ (ret < 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#ਜੇ defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
ਸਥਿਰ
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_ਪੂਰਾ)
{
    *o_ਪੂਰਾ = 0;

    ਬਣਤਰ iovec iovs[d_m3WasiMaxIovs];
    ਲਈ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (ਪੂਰਨ)i_iovsLen) : readv(i_fd, iovs, (ਪੂਰਨ)i_iovsLen);
    ਜੇ (ret < 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    *o_ਪੂਰਾ = (__wasi_size_t)ret;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

#ਨਹੀਂਤਾਂ

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
ਸਥਿਰ
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_ਪੂਰਾ)
{
    *o_ਪੂਰਾ = 0;

    size_t ਪੂਰਾ = 0;

    ਲਈ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ਖਾਲੀ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ਜੇ (len == 0) {
            ਜਾਰੀ;
        }

        ssize_t ret = i_write ? ਲਿਖ(i_fd, addr, len) : ਪਠ(i_fd, addr, len);
        ਜੇ (ret < 0) {
            ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
        }

        ਪੂਰਾ += (size_t)ret;
        ਜੇ ((size_t)ret < len) {
            ਰੋਕੋ;
        }
    }

    *o_ਪੂਰਾ = (__wasi_size_t)ਪੂਰਾ;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

#ਜੇ_ਅੰਤ // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
ਸਥਿਰ
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_ਵਿੱਥ, bool i_write,
                            __wasi_size_t* o_ਪੂਰਾ)
{
    *o_ਪੂਰਾ = 0;

    size_t ਪੂਰਾ = 0;

    ਲਈ (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ਖਾਲੀ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ਜੇ (len == 0) {
            ਜਾਰੀ;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_ਵਿੱਥ + ਪੂਰਾ))
                              : pread(i_fd, addr, len, (off_t)(i_ਵਿੱਥ + ਪੂਰਾ));
        ਜੇ (ret < 0) {
            ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
        }

        ਪੂਰਾ += (size_t)ret;
        ਜੇ ((size_t)ret < len) {
            ਰੋਕੋ;
        }
    }

    *o_ਪੂਰਾ = (__wasi_size_t)ਪੂਰਾ;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_ਪੂਰਾ);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_ਪੂਰਾ);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_ਵਿੱਥ,
                                      __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_at(i_fd, i_iovs, i_iovsLen, i_ਵਿੱਥ, false, o_ਪੂਰਾ);
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ਅਚਲ m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_ਵਿੱਥ,
                                       __wasi_size_t* o_ਪੂਰਾ)
{
    ਵਾਪਸ transfer_at(i_fd, i_iovs, i_iovsLen, i_ਵਿੱਥ, true, o_ਪੂਰਾ);
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

    ਪੂਰਨ dirfd = preopen_fd(i_dirfd);
    ਜੇ (dirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    ਪੂਰਨ flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    ਜੇ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } ਨਹੀਂਤਾਂ ਜੇ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } ਨਹੀਂਤਾਂ ਜੇ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    ਪੂਰਨ host_fd = openat(dirfd, i_ਰਸਤਾ, flags, 0644);
    ਜੇ (host_fd < 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, ਆਕਾਰ(*o_stat));

    ਪੂਰਨ dirfd = preopen_fd(i_dirfd);
    ਜੇ (dirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਬਣਤਰ stat ਰਸਤਾ_stat;
    ਜੇ (fstatat(dirfd, i_ਰਸਤਾ, &ਰਸਤਾ_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    filestat_from_stat(&ਰਸਤਾ_stat, o_stat);
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਪੂਰਨ dirfd = preopen_fd(i_dirfd);
    ਜੇ (dirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਜੇ (mkdirat(dirfd, i_ਰਸਤਾ, 0755) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਪੂਰਨ dirfd = preopen_fd(i_dirfd);
    ਜੇ (dirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਜੇ (unlinkat(dirfd, i_ਰਸਤਾ, AT_REMOVEDIR) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਪੂਰਨ dirfd = preopen_fd(i_dirfd);
    ਜੇ (dirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਜੇ (unlinkat(dirfd, i_ਰਸਤਾ, 0) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ਰਸਤਾ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_ਰਸਤਾ)
{
    ਪੂਰਨ oldDirfd = preopen_fd(i_oldDirfd);
    ਪੂਰਨ newDirfd = preopen_fd(i_newDirfd);
    ਜੇ (oldDirfd < 0 || newDirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਜੇ (renameat(oldDirfd, i_old_ਰਸਤਾ, newDirfd, i_new_ਰਸਤਾ) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_ਰਸਤਾ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_ਰਸਤਾ)
{
    ਪੂਰਨ oldDirfd = preopen_fd(i_oldDirfd);
    ਪੂਰਨ newDirfd = preopen_fd(i_newDirfd);
    ਜੇ (oldDirfd < 0 || newDirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਜੇ (linkat(oldDirfd, i_old_ਰਸਤਾ, newDirfd, i_new_ਰਸਤਾ, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_symlink (ccstr_t i_ਨਿਸ਼ਾਨਾ, __wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ)
{
    ਪੂਰਨ dirfd = preopen_fd(i_dirfd);
    ਜੇ (dirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ਜੇ (symlinkat(i_ਨਿਸ਼ਾਨਾ, dirfd, i_ਰਸਤਾ) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_ਰਸਤਾ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_ਰਸਤਾ, ਅੱਖਰ* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_ਵਰਤਿਆ)
{
    *o_buf_ਵਰਤਿਆ = 0;

    ਪੂਰਨ dirfd = preopen_fd(i_dirfd);
    ਜੇ (dirfd < 0) {
        ਵਾਪਸ __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_ਰਸਤਾ, o_buf, i_bufLen);
    ਜੇ (ret < 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    *o_buf_ਵਰਤਿਆ = (__wasi_size_t)ret;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ਜੇ (clk == d_m3ClockIdInvalid) {
        ਵਾਪਸ __WASI_ERRNO_INVAL;
    }

    ਬਣਤਰ timespec tp;
    ਜੇ (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } ਨਹੀਂਤਾਂ {
        *o_res = convert_timespec(&tp);
    }
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_clock_ਸਮਾਂ_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_ਸਮਾਂ)
{
    *o_ਸਮਾਂ = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ਜੇ (clk == d_m3ClockIdInvalid) {
        ਵਾਪਸ __WASI_ERRNO_INVAL;
    }

    ਬਣਤਰ timespec tp;
    ਜੇ (clock_gettime(clk, &tp) != 0) {
        ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
    }

    *o_ਸਮਾਂ = convert_timespec(&tp);
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_random_get (ਖਾਲੀ* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    ਜਦੋਂ (i_len) {
        ssize_t retlen;

#ਜੇ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  ਜੇ defined(__APPLE__) && (ਨਿਸ਼ਾਨਾ_OS_IPHONE || ਨਿਸ਼ਾਨਾ_IPHONE_SIMULATOR)
        retlen = Sec_Random_Copy_ਬਾਈਟਾਂ(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  ਨਹੀਂਤਾਂ
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#ਜਾਂਜੇ defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#ਨਹੀਂਤਾਂ
        ਵਾਪਸ __WASI_ERRNO_NOSYS;
#ਜੇ_ਅੰਤ

        ਜੇ (retlen < 0) {
            ਜੇ (errno == EINTR || errno == EAGAIN) {
                ਜਾਰੀ;
            }
            ਵਾਪਸ errno_ਮੰਜ਼ਿਲ_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
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
ਸਥਿਰ
uint64_t poll_ਬਚਿਆ_ns (ਅਚਲ m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ਜੇ (not i_sub->isAbsolute) {
        ਵਾਪਸ (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    ਜੇ (clk == d_m3ClockIdInvalid) {
        ਵਾਪਸ UINT64_MAX;
    }

    ਬਣਤਰ timespec tp;
    ਜੇ (clock_gettime(clk, &tp) != 0) {
        ਵਾਪਸ 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    ਵਾਪਸ (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
ਸਥਿਰ
__wasi_filesize_t poll_readable_ਬਾਈਟਾਂ (ਪੂਰਨ i_fd)
{
#ਜੇ defined(FIONREAD)
    ਪੂਰਨ ਉਪਲਬਧ = 0;
    ਜੇ (ioctl(i_fd, FIONREAD, &ਉਪਲਬਧ) == 0 and ਉਪਲਬਧ > 0) {
        ਵਾਪਸ (__wasi_filesize_t)ਉਪਲਬਧ;
    }
#ਜੇ_ਅੰਤ
    ਵਾਪਸ 1;
}

ਸਥਿਰ
__wasi_errno_t m3_wasi_host_poll_oneoff (ਅਚਲ m3_wasi_pollsub_t* i_subs, __wasi_size_t i_ਗਿਣਤੀ,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_ਗਿਣਤੀ)
{
    *o_event_ਗਿਣਤੀ = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t ਅੰਕ_Fds = 0;

    ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
        ਜੇ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_ਬਚਿਆ_ns(&i_subs[i], 0);
            ਜੇ (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } ਨਹੀਂਤਾਂ {
            ਅੰਕ_Fds++;
        }
    }

    ਬਣਤਰ pollfd* pollfds = ਨਿਰਾ;

    ਜੇ (ਅੰਕ_Fds) {
        pollfds = (ਬਣਤਰ pollfd*)m3_Malloc("WASI poll_oneoff", ਅੰਕ_Fds * ਆਕਾਰ(ਬਣਤਰ pollfd));
        ਜੇ (pollfds == ਨਿਰਾ) {
            ਵਾਪਸ __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
            ਜੇ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                ਜਾਰੀ;
            }
            pollfds[n].fd = (ਪੂਰਨ)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    ਪੂਰਨ timeoutMs = -1;
    ਜੇ (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)ਵੱਧ_ਤੋਂ_ਵੱਧ_ਅੰਕ) ? ਵੱਧ_ਤੋਂ_ਵੱਧ_ਅੰਕ : (ਪੂਰਨ)ms;
    }

    ਪੂਰਨ ready;
    ਜੇ (ਅੰਕ_Fds) {
        ਕਰੋ {
            ready = poll(pollfds, (nfds_t)ਅੰਕ_Fds, timeoutMs);
        } ਜਦੋਂ (ready < 0 and errno == EINTR);
    } ਨਹੀਂਤਾਂ ਜੇ (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        ਬਣਤਰ timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (ਲੰਮਾ)(soonest % 1000000000);

        ਜਦੋਂ (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } ਨਹੀਂਤਾਂ ਜੇ (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } ਨਹੀਂਤਾਂ {
        // no clocks and no descriptors: nothing that could ever become ready
        ਵਾਪਸ __WASI_ERRNO_INVAL;
    }

    ਜੇ (ready < 0) {
        __wasi_errno_t err = errno_ਮੰਜ਼ਿਲ_wasi(errno);
        m3_Free(pollfds);
        ਵਾਪਸ err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t ਬਾਹਰਲਾ = 0;
    __wasi_size_t n = 0;

    ਲਈ (__wasi_size_t i = 0; i < i_ਗਿਣਤੀ; i++) {
        ਅਚਲ m3_wasi_pollsub_t* sub = &i_subs[i];

        ਜੇ (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            ਜੇ (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[ਬਾਹਰਲਾ].userdata = sub->userdata;
                o_events[ਬਾਹਰਲਾ].error = __WASI_ERRNO_INVAL;
                o_events[ਬਾਹਰਲਾ].type = __WASI_EVENTTYPE_CLOCK;
                o_events[ਬਾਹਰਲਾ].nbytes = 0;
                o_events[ਬਾਹਰਲਾ].flags = 0;
                ਬਾਹਰਲਾ++;
            } ਨਹੀਂਤਾਂ ਜੇ (poll_ਬਚਿਆ_ns(sub, elapsedNs) == 0) {
                o_events[ਬਾਹਰਲਾ].userdata = sub->userdata;
                o_events[ਬਾਹਰਲਾ].error = __WASI_ERRNO_SUCCESS;
                o_events[ਬਾਹਰਲਾ].type = __WASI_EVENTTYPE_CLOCK;
                o_events[ਬਾਹਰਲਾ].nbytes = 0;
                o_events[ਬਾਹਰਲਾ].flags = 0;
                ਬਾਹਰਲਾ++;
            }
            ਜਾਰੀ;
        }

        ਛੋਟਾ revents = pollfds[n].revents;
        n++;

        ਜੇ (revents == 0) {
            ਜਾਰੀ;
        }

        o_events[ਬਾਹਰਲਾ].userdata = sub->userdata;
        o_events[ਬਾਹਰਲਾ].type = sub->type;
        o_events[ਬਾਹਰਲਾ].nbytes = 0;
        o_events[ਬਾਹਰਲਾ].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        ਜੇ (revents & POLLNVAL) {
            o_events[ਬਾਹਰਲਾ].error = __WASI_ERRNO_BADF;
        } ਨਹੀਂਤਾਂ ਜੇ (revents & POLLERR) {
            o_events[ਬਾਹਰਲਾ].error = __WASI_ERRNO_IO;
        } ਨਹੀਂਤਾਂ {
            o_events[ਬਾਹਰਲਾ].error = __WASI_ERRNO_SUCCESS;
            ਜੇ (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[ਬਾਹਰਲਾ].nbytes = poll_readable_ਬਾਈਟਾਂ(pollfds[n - 1].fd);
            }
        }

        ਬਾਹਰਲਾ++;
    }

    m3_Free(pollfds);

    *o_event_ਗਿਣਤੀ = ਬਾਹਰਲਾ;
    ਵਾਪਸ __WASI_ERRNO_SUCCESS;
}

#ਜੇ_ਅੰਤ // m3_api_wasi_posix_h
