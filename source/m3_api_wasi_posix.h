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

#include "m3_core.h"

#ᬬᬾᬦ᭄ defined(_WIN32)
#  error "This file is not for Windows"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <string.h>
#include <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#ᬬᬾᬦ᭄ !defined(__wasi__)
#  include <sys/ioctl.h>
#endif

#ᬬᬾᬦ᭄ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  ᬬᬾᬦ᭄ defined(__APPLE__)
#    include <TargetConditionals.h>
#    ᬬᬾᬦ᭄ TARGET_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    ᬮᬶᬬ // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  ᬮᬶᬬ
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#endif

#include "m3_api_wasi_host.h"


/*
 * Plumbing
 */

static
__wasi_errno_t errno_to_wasi (puur_naa_nka errnum)
{
    // clang-format off
    switch (errnum) {
    pak_sa EPERM:    ᬯᬮᬶ __WASI_ERRNO_PERM;
    pak_sa ENOENT:   ᬯᬮᬶ __WASI_ERRNO_NOENT;
    pak_sa ESRCH:    ᬯᬮᬶ __WASI_ERRNO_SRCH;
    pak_sa EINTR:    ᬯᬮᬶ __WASI_ERRNO_INTR;
    pak_sa EIO:      ᬯᬮᬶ __WASI_ERRNO_IO;
    pak_sa ENXIO:    ᬯᬮᬶ __WASI_ERRNO_NXIO;
    pak_sa E2BIG:    ᬯᬮᬶ __WASI_ERRNO_2BIG;
    pak_sa ENOEXEC:  ᬯᬮᬶ __WASI_ERRNO_NOEXEC;
    pak_sa EBADF:    ᬯᬮᬶ __WASI_ERRNO_BADF;
    pak_sa ECHILD:   ᬯᬮᬶ __WASI_ERRNO_CHILD;
    pak_sa EAGAIN:   ᬯᬮᬶ __WASI_ERRNO_AGAIN;
    pak_sa ENOMEM:   ᬯᬮᬶ __WASI_ERRNO_NOMEM;
    pak_sa EACCES:   ᬯᬮᬶ __WASI_ERRNO_ACCES;
    pak_sa EFAULT:   ᬯᬮᬶ __WASI_ERRNO_FAULT;
    pak_sa EBUSY:    ᬯᬮᬶ __WASI_ERRNO_BUSY;
    pak_sa EEXIST:   ᬯᬮᬶ __WASI_ERRNO_EXIST;
    pak_sa EXDEV:    ᬯᬮᬶ __WASI_ERRNO_XDEV;
    pak_sa ENODEV:   ᬯᬮᬶ __WASI_ERRNO_NODEV;
    pak_sa ENOTDIR:  ᬯᬮᬶ __WASI_ERRNO_NOTDIR;
    pak_sa EISDIR:   ᬯᬮᬶ __WASI_ERRNO_ISDIR;
    pak_sa EINVAL:   ᬯᬮᬶ __WASI_ERRNO_INVAL;
    pak_sa ENFILE:   ᬯᬮᬶ __WASI_ERRNO_NFILE;
    pak_sa EMFILE:   ᬯᬮᬶ __WASI_ERRNO_MFILE;
    pak_sa ENOTTY:   ᬯᬮᬶ __WASI_ERRNO_NOTTY;
    pak_sa ETXTBSY:  ᬯᬮᬶ __WASI_ERRNO_TXTBSY;
    pak_sa EFBIG:    ᬯᬮᬶ __WASI_ERRNO_FBIG;
    pak_sa ENOSPC:   ᬯᬮᬶ __WASI_ERRNO_NOSPC;
    pak_sa ESPIPE:   ᬯᬮᬶ __WASI_ERRNO_SPIPE;
    pak_sa EROFS:    ᬯᬮᬶ __WASI_ERRNO_ROFS;
    pak_sa EMLINK:   ᬯᬮᬶ __WASI_ERRNO_MLINK;
    pak_sa EPIPE:    ᬯᬮᬶ __WASI_ERRNO_PIPE;
    pak_sa EDOM:     ᬯᬮᬶ __WASI_ERRNO_DOM;
    pak_sa ERANGE:   ᬯᬮᬶ __WASI_ERRNO_RANGE;
    }
    // clang-format on
    ᬯᬮᬶ __WASI_ERRNO_INVAL;
}

static
__wasi_filetype_t filetype_from_stat_mode (puur_naa_nka mode)
{
    ᬯᬮᬶ (S_ISBLK(mode) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(mode) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(mode) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(mode) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(mode) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

static
void filestat_from_stat (const sa_nracanaa stat* i_stat, m3_wasi_filestat_t* o_stat)
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
static
puur_naa_nka preopen_fd (__wasi_fd_t i_fd)
{
    ᬬᬾᬦ᭄ (i_fd >= d_m3WasiPreopenCount) {
        ᬯᬮᬶ -1;
    }
    ᬯᬮᬶ m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#ᬬᬾᬦ᭄ defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) NULL)
#ᬮᬶᬬ
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#endif

static
clockid_t convert_clockid (__wasi_clockid_t in)
{
    switch (in) {
    pak_sa __WASI_CLOCKID_MONOTONIC: ᬯᬮᬶ CLOCK_MONOTONIC;
    pak_sa __WASI_CLOCKID_REALTIME: ᬯᬮᬶ CLOCK_REALTIME;
#ᬬᬾᬦ᭄ defined(CLOCK_PROCESS_CPUTIME_ID)
    pak_sa __WASI_CLOCKID_PROCESS_CPUTIME_ID: ᬯᬮᬶ CLOCK_PROCESS_CPUTIME_ID;
#endif
#ᬬᬾᬦ᭄ defined(CLOCK_THREAD_CPUTIME_ID)
    pak_sa __WASI_CLOCKID_THREAD_CPUTIME_ID: ᬯᬮᬶ CLOCK_THREAD_CPUTIME_ID;
#endif
    default: ᬯᬮᬶ d_m3ClockIdInvalid;
    }
}

static
__wasi_timestamp_t convert_timespec (const sa_nracanaa timespec* ts)
{
    ᬬᬾᬦ᭄ (ts->tv_sec < 0) {
        ᬯᬮᬶ 0;
    }
    ᬬᬾᬦ᭄ ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        ᬯᬮᬶ UINT64_MAX;
    }
    ᬯᬮᬶ (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

static
void m3_wasi_host_init ()
{
    for (puur_naa_nka i = 3; i < d_m3WasiPreopenCount; i++) {
        ᬬᬾᬦ᭄ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_path, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

static
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ᬬᬾᬦ᭄ (close(i_fd) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#ᬬᬾᬦ᭄ defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#elif defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#endif

static
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#ᬬᬾᬦ᭄ defined(d_m3WasiDatasync)
    ᬬᬾᬦ᭄ (d_m3WasiDatasync(i_fd) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
#ᬮᬶᬬ
    ᬯᬮᬶ __WASI_ERRNO_NOSYS;
#endif
}

static
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#ᬬᬾᬦ᭄ defined(POSIX_FADV_NORMAL)
    puur_naa_nka adv;
    switch (i_advice) {
    pak_sa __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; bha_nga;
    pak_sa __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; bha_nga;
    pak_sa __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; bha_nga;
    pak_sa __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; bha_nga;
    pak_sa __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; bha_nga;
    pak_sa __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; bha_nga;
    default: ᬯᬮᬶ __WASI_ERRNO_INVAL;
    }

    puur_naa_nka ret = posix_fadvise(i_fd, i_offset, i_length, adv);
    ᬬᬾᬦ᭄ (ret != 0) {
        ᬯᬮᬶ errno_to_wasi(ret);
    }
#ᬮᬶᬬ
    ᬬᬾᬦ᭄ (i_advice > __WASI_ADVICE_NOREUSE) {
        ᬯᬮᬶ __WASI_ERRNO_INVAL;
    }

    sa_nracanaa stat fd_stat;
    ᬬᬾᬦ᭄ (fstat(i_fd, &fd_stat) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
#endif

    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    puur_naa_nka fl = fcntl(i_fd, F_GETFL);
    ᬬᬾᬦ᭄ (fl < 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    sa_nracanaa stat fd_stat;
    ᬬᬾᬦ᭄ (fstat(i_fd, &fd_stat) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    *o_filetype = filetype_from_stat_mode(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
static
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    puur_naa_nka fl = fcntl(i_fd, F_GETFL);
    ᬬᬾᬦ᭄ (fl < 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    const __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    ᬬᬾᬦ᭄ ((i_flags & fixed) != (current & fixed)) {
        ᬯᬮᬶ __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    ᬬᬾᬦ᭄ (fcntl(i_fd, F_SETFL, fl) < 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, parimaa_na(*o_stat));

    sa_nracanaa stat fd_stat;
    ᬬᬾᬦ᭄ (fstat(i_fd, &fd_stat) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    ᬬᬾᬦ᭄ (ftruncate(i_fd, (off_t)i_size) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    sa_nracanaa timespec times[2];

    ᬬᬾᬦ᭄ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } ᬮᬶᬬ ᬬᬾᬦ᭄ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } ᬮᬶᬬ {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    ᬬᬾᬦ᭄ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } ᬮᬶᬬ ᬬᬾᬦ᭄ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } ᬮᬶᬬ {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    ᬬᬾᬦ᭄ (futimens(i_fd, times) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, puur_naa_nka i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_offset, i_whence);
    ᬬᬾᬦ᭄ (ret < 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#ᬬᬾᬦ᭄ defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
static
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    sa_nracanaa iovec iovs[d_m3WasiMaxIovs];
    for (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (puur_naa_nka)i_iovsLen) : readv(i_fd, iovs, (puur_naa_nka)i_iovsLen);
    ᬬᬾᬦ᭄ (ret < 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    *o_done = (__wasi_size_t)ret;
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

#ᬮᬶᬬ

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
static
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    for (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        void*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ᬬᬾᬦ᭄ (len == 0) {
            continue;
        }

        ssize_t ret = i_write ? write(i_fd, addr, len) : read(i_fd, addr, len);
        ᬬᬾᬦ᭄ (ret < 0) {
            ᬯᬮᬶ errno_to_wasi(errno);
        }

        done += (size_t)ret;
        ᬬᬾᬦ᭄ ((size_t)ret < len) {
            bha_nga;
        }
    }

    *o_done = (__wasi_size_t)done;
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

#endif // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
static
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset, bool i_write,
                            __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    for (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        void*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ᬬᬾᬦ᭄ (len == 0) {
            continue;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_offset + done))
                              : pread(i_fd, addr, len, (off_t)(i_offset + done));
        ᬬᬾᬦ᭄ (ret < 0) {
            ᬯᬮᬶ errno_to_wasi(errno);
        }

        done += (size_t)ret;
        ᬬᬾᬦ᭄ ((size_t)ret < len) {
            bha_nga;
        }
    }

    *o_done = (__wasi_size_t)done;
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    ᬯᬮᬶ transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    ᬯᬮᬶ transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    ᬯᬮᬶ transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, false, o_done);
}

static
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, const m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    ᬯᬮᬶ transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, true, o_done);
}


/*
 * Paths
 */

static
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    puur_naa_nka dirfd = preopen_fd(i_dirfd);
    ᬬᬾᬦ᭄ (dirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    puur_naa_nka flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    ᬬᬾᬦ᭄ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } ᬮᬶᬬ ᬬᬾᬦ᭄ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } ᬮᬶᬬ ᬬᬾᬦ᭄ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    puur_naa_nka host_fd = openat(dirfd, i_path, flags, 0644);
    ᬬᬾᬦ᭄ (host_fd < 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, parimaa_na(*o_stat));

    puur_naa_nka dirfd = preopen_fd(i_dirfd);
    ᬬᬾᬦ᭄ (dirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    sa_nracanaa stat path_stat;
    ᬬᬾᬦ᭄ (fstatat(dirfd, i_path, &path_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    filestat_from_stat(&path_stat, o_stat);
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    puur_naa_nka dirfd = preopen_fd(i_dirfd);
    ᬬᬾᬦ᭄ (dirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    ᬬᬾᬦ᭄ (mkdirat(dirfd, i_path, 0755) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    puur_naa_nka dirfd = preopen_fd(i_dirfd);
    ᬬᬾᬦ᭄ (dirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    ᬬᬾᬦ᭄ (unlinkat(dirfd, i_path, AT_REMOVEDIR) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    puur_naa_nka dirfd = preopen_fd(i_dirfd);
    ᬬᬾᬦ᭄ (dirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    ᬬᬾᬦ᭄ (unlinkat(dirfd, i_path, 0) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    puur_naa_nka oldDirfd = preopen_fd(i_oldDirfd);
    puur_naa_nka newDirfd = preopen_fd(i_newDirfd);
    ᬬᬾᬦ᭄ (oldDirfd < 0 || newDirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    ᬬᬾᬦ᭄ (renameat(oldDirfd, i_oldPath, newDirfd, i_newPath) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    puur_naa_nka oldDirfd = preopen_fd(i_oldDirfd);
    puur_naa_nka newDirfd = preopen_fd(i_newDirfd);
    ᬬᬾᬦ᭄ (oldDirfd < 0 || newDirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    ᬬᬾᬦ᭄ (linkat(oldDirfd, i_oldPath, newDirfd, i_newPath, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    puur_naa_nka dirfd = preopen_fd(i_dirfd);
    ᬬᬾᬦ᭄ (dirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    ᬬᬾᬦ᭄ (symlinkat(i_target, dirfd, i_path) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, var_na* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    puur_naa_nka dirfd = preopen_fd(i_dirfd);
    ᬬᬾᬦ᭄ (dirfd < 0) {
        ᬯᬮᬶ __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_path, o_buf, i_bufLen);
    ᬬᬾᬦ᭄ (ret < 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    *o_bufUsed = (__wasi_size_t)ret;
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

static
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ᬬᬾᬦ᭄ (clk == d_m3ClockIdInvalid) {
        ᬯᬮᬶ __WASI_ERRNO_INVAL;
    }

    sa_nracanaa timespec tp;
    ᬬᬾᬦ᭄ (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } ᬮᬶᬬ {
        *o_res = convert_timespec(&tp);
    }
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ᬬᬾᬦ᭄ (clk == d_m3ClockIdInvalid) {
        ᬯᬮᬶ __WASI_ERRNO_INVAL;
    }

    sa_nracanaa timespec tp;
    ᬬᬾᬦ᭄ (clock_gettime(clk, &tp) != 0) {
        ᬯᬮᬶ errno_to_wasi(errno);
    }

    *o_time = convert_timespec(&tp);
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

static
__wasi_errno_t m3_wasi_host_random_get (void* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    ᬲᭂᬤᭂᬮ (i_len) {
        ssize_t retlen;

#ᬬᬾᬦ᭄ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  ᬬᬾᬦ᭄ defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
        retlen = SecRandomCopyBytes(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  ᬮᬶᬬ
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#elif defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#ᬮᬶᬬ
        ᬯᬮᬶ __WASI_ERRNO_NOSYS;
#endif

        ᬬᬾᬦ᭄ (retlen < 0) {
            ᬬᬾᬦ᭄ (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            ᬯᬮᬶ errno_to_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
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
static
uint64_t poll_remaining_ns (const m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ᬬᬾᬦ᭄ (not i_sub->isAbsolute) {
        ᬯᬮᬶ (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    ᬬᬾᬦ᭄ (clk == d_m3ClockIdInvalid) {
        ᬯᬮᬶ UINT64_MAX;
    }

    sa_nracanaa timespec tp;
    ᬬᬾᬦ᭄ (clock_gettime(clk, &tp) != 0) {
        ᬯᬮᬶ 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    ᬯᬮᬶ (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
static
__wasi_filesize_t poll_readable_bytes (puur_naa_nka i_fd)
{
#ᬬᬾᬦ᭄ defined(FIONREAD)
    puur_naa_nka available = 0;
    ᬬᬾᬦ᭄ (ioctl(i_fd, FIONREAD, &available) == 0 and available > 0) {
        ᬯᬮᬶ (__wasi_filesize_t)available;
    }
#endif
    ᬯᬮᬶ 1;
}

static
__wasi_errno_t m3_wasi_host_poll_oneoff (const m3_wasi_pollsub_t* i_subs, __wasi_size_t i_count,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_eventCount)
{
    *o_eventCount = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t numFds = 0;

    for (__wasi_size_t i = 0; i < i_count; i++) {
        ᬬᬾᬦ᭄ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_remaining_ns(&i_subs[i], 0);
            ᬬᬾᬦ᭄ (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } ᬮᬶᬬ {
            numFds++;
        }
    }

    sa_nracanaa pollfd* pollfds = NULL;

    ᬬᬾᬦ᭄ (numFds) {
        pollfds = (sa_nracanaa pollfd*)m3_Malloc("WASI poll_oneoff", numFds * parimaa_na(sa_nracanaa pollfd));
        ᬬᬾᬦ᭄ (pollfds == NULL) {
            ᬯᬮᬶ __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        for (__wasi_size_t i = 0; i < i_count; i++) {
            ᬬᬾᬦ᭄ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                continue;
            }
            pollfds[n].fd = (puur_naa_nka)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    puur_naa_nka timeoutMs = -1;
    ᬬᬾᬦ᭄ (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)INT_MAX) ? INT_MAX : (puur_naa_nka)ms;
    }

    puur_naa_nka ready;
    ᬬᬾᬦ᭄ (numFds) {
        do {
            ready = poll(pollfds, (nfds_t)numFds, timeoutMs);
        } ᬲᭂᬤᭂᬮ (ready < 0 and errno == EINTR);
    } ᬮᬶᬬ ᬬᬾᬦ᭄ (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        sa_nracanaa timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (long)(soonest % 1000000000);

        ᬲᭂᬤᭂᬮ (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } ᬮᬶᬬ ᬬᬾᬦ᭄ (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } ᬮᬶᬬ {
        // no clocks and no descriptors: nothing that could ever become ready
        ᬯᬮᬶ __WASI_ERRNO_INVAL;
    }

    ᬬᬾᬦ᭄ (ready < 0) {
        __wasi_errno_t err = errno_to_wasi(errno);
        m3_Free(pollfds);
        ᬯᬮᬶ err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t out = 0;
    __wasi_size_t n = 0;

    for (__wasi_size_t i = 0; i < i_count; i++) {
        const m3_wasi_pollsub_t* sub = &i_subs[i];

        ᬬᬾᬦ᭄ (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            ᬬᬾᬦ᭄ (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_INVAL;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            } ᬮᬶᬬ ᬬᬾᬦ᭄ (poll_remaining_ns(sub, elapsedNs) == 0) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_SUCCESS;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            }
            continue;
        }

        short revents = pollfds[n].revents;
        n++;

        ᬬᬾᬦ᭄ (revents == 0) {
            continue;
        }

        o_events[out].userdata = sub->userdata;
        o_events[out].type = sub->type;
        o_events[out].nbytes = 0;
        o_events[out].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        ᬬᬾᬦ᭄ (revents & POLLNVAL) {
            o_events[out].error = __WASI_ERRNO_BADF;
        } ᬮᬶᬬ ᬬᬾᬦ᭄ (revents & POLLERR) {
            o_events[out].error = __WASI_ERRNO_IO;
        } ᬮᬶᬬ {
            o_events[out].error = __WASI_ERRNO_SUCCESS;
            ᬬᬾᬦ᭄ (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[out].nbytes = poll_readable_bytes(pollfds[n - 1].fd);
            }
        }

        out++;
    }

    m3_Free(pollfds);

    *o_eventCount = out;
    ᬯᬮᬶ __WASI_ERRNO_SUCCESS;
}

#endif // m3_api_wasi_posix_h
