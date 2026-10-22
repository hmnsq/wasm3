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

#నిర్వచితం_కాకపోతే m3_api_wasi_posix_h
#నిర్వచించు m3_api_wasi_posix_h

#అన్తర్భావ "m3_core.h"

#ఒకవేళ defined(_WIN32)
#  error "This file is not for Windows"
#అయితే_ముగింపు

#అన్తర్భావ <sys/types.h>
#అన్తర్భావ <sys/stat.h>
#అన్తర్భావ <errno.h>
#అన్తర్భావ <fcntl.h>
#అన్తర్భావ <limits.h>
#అన్తర్భావ <poll.h>
#అన్తర్భావ <string.h>
#అన్తర్భావ <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#ఒకవేళ !defined(__wasi__)
#  include <sys/ioctl.h>
#అయితే_ముగింపు

#ఒకవేళ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  ఒకవేళ defined(__APPLE__)
#    include <TargetConditionals.h>
#    ఒకవేళ లక్ష్యం_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    లేకపోతే // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  లేకపోతే
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#అయితే_ముగింపు

#అన్తర్భావ "m3_api_wasi_host.h"


/*
 * Plumbing
 */

స్థిర
__wasi_errno_t errno_గమ్యం_wasi (పూర్ణాంకం errnum)
{
    // clang-format off
    ఎంపిక (errnum) {
    స్థితి EPERM:    తిరిగి __WASI_ERRNO_PERM;
    స్థితి ENOENT:   తిరిగి __WASI_ERRNO_NOENT;
    స్థితి ESRCH:    తిరిగి __WASI_ERRNO_SRCH;
    స్థితి EINTR:    తిరిగి __WASI_ERRNO_INTR;
    స్థితి EIO:      తిరిగి __WASI_ERRNO_IO;
    స్థితి ENXIO:    తిరిగి __WASI_ERRNO_NXIO;
    స్థితి E2BIG:    తిరిగి __WASI_ERRNO_2BIG;
    స్థితి ENOEXEC:  తిరిగి __WASI_ERRNO_NOEXEC;
    స్థితి EBADF:    తిరిగి __WASI_ERRNO_BADF;
    స్థితి ECHILD:   తిరిగి __WASI_ERRNO_CHILD;
    స్థితి EAGAIN:   తిరిగి __WASI_ERRNO_AGAIN;
    స్థితి ENOMEM:   తిరిగి __WASI_ERRNO_NOMEM;
    స్థితి EACCES:   తిరిగి __WASI_ERRNO_ACCES;
    స్థితి EFAULT:   తిరిగి __WASI_ERRNO_FAULT;
    స్థితి EBUSY:    తిరిగి __WASI_ERRNO_BUSY;
    స్థితి EEXIST:   తిరిగి __WASI_ERRNO_EXIST;
    స్థితి EXDEV:    తిరిగి __WASI_ERRNO_XDEV;
    స్థితి ENODEV:   తిరిగి __WASI_ERRNO_NODEV;
    స్థితి ENOTDIR:  తిరిగి __WASI_ERRNO_NOTDIR;
    స్థితి EISDIR:   తిరిగి __WASI_ERRNO_ISDIR;
    స్థితి EINVAL:   తిరిగి __WASI_ERRNO_INVAL;
    స్థితి ENFILE:   తిరిగి __WASI_ERRNO_NFILE;
    స్థితి EMFILE:   తిరిగి __WASI_ERRNO_MFILE;
    స్థితి ENOTTY:   తిరిగి __WASI_ERRNO_NOTTY;
    స్థితి ETXTBSY:  తిరిగి __WASI_ERRNO_TXTBSY;
    స్థితి EFBIG:    తిరిగి __WASI_ERRNO_FBIG;
    స్థితి ENOSPC:   తిరిగి __WASI_ERRNO_NOSPC;
    స్థితి ESPIPE:   తిరిగి __WASI_ERRNO_SPIPE;
    స్థితి EROFS:    తిరిగి __WASI_ERRNO_ROFS;
    స్థితి EMLINK:   తిరిగి __WASI_ERRNO_MLINK;
    స్థితి EPIPE:    తిరిగి __WASI_ERRNO_PIPE;
    స్థితి EDOM:     తిరిగి __WASI_ERRNO_DOM;
    స్థితి ERANGE:   తిరిగి __WASI_ERRNO_RANGE;
    }
    // clang-format on
    తిరిగి __WASI_ERRNO_INVAL;
}

స్థిర
__wasi_filetype_t filetype_from_stat_బహుళకం (పూర్ణాంకం బహుళకం)
{
    తిరిగి (S_ISBLK(బహుళకం) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(బహుళకం) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(బహుళకం) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(బహుళకం) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(బహుళకం) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

స్థిర
శూన్య filestat_from_stat (స్థిరరాశి నిర్మాణం stat* i_stat, m3_wasi_filestat_t* o_stat)
{
    o_stat->dev = (uint64_t)i_stat->st_dev;
    o_stat->ino = (uint64_t)i_stat->st_ino;
    o_stat->nlink = (uint64_t)i_stat->st_nlink;
    o_stat->కొలత = (uint64_t)i_stat->st_size;
    o_stat->atim = (uint64_t)i_stat->st_atime * 1000000000ULL;
    o_stat->mtim = (uint64_t)i_stat->st_mtime * 1000000000ULL;
    o_stat->ctim = (uint64_t)i_stat->st_ctime * 1000000000ULL;
    o_stat->filetype = filetype_from_stat_బహుళకం(i_stat->st_mode);
}

// The descriptor a preopened directory holds, or -1 where i_fd names no preopen
స్థిర
పూర్ణాంకం preopen_fd (__wasi_fd_t i_fd)
{
    ఒకవేళ (i_fd >= d_m3_Wasi_Preopen_లెక్క) {
        తిరిగి -1;
    }
    తిరిగి m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#ఒకవేళ defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) ఖాళీ)
#లేకపోతే
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#అయితే_ముగింపు

స్థిర
clockid_t convert_clockid (__wasi_clockid_t లోపల)
{
    ఎంపిక (లోపల) {
    స్థితి __WASI_CLOCKID_MONOTONIC: తిరిగి CLOCK_MONOTONIC;
    స్థితి __WASI_CLOCKID_REALTIME: తిరిగి CLOCK_REALTIME;
#ఒకవేళ defined(CLOCK_PROCESS_CPUTIME_ID)
    స్థితి __WASI_CLOCKID_PROCESS_CPUTIME_ID: తిరిగి CLOCK_PROCESS_CPUTIME_ID;
#అయితే_ముగింపు
#ఒకవేళ defined(CLOCK_THREAD_CPUTIME_ID)
    స్థితి __WASI_CLOCKID_THREAD_CPUTIME_ID: తిరిగి CLOCK_THREAD_CPUTIME_ID;
#అయితే_ముగింపు
    అప్రమేయ: తిరిగి d_m3ClockIdInvalid;
    }
}

స్థిర
__wasi_timestamp_t convert_timespec (స్థిరరాశి నిర్మాణం timespec* ts)
{
    ఒకవేళ (ts->tv_sec < 0) {
        తిరిగి 0;
    }
    ఒకవేళ ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        తిరిగి UINT64_MAX;
    }
    తిరిగి (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

స్థిర
శూన్య m3_wasi_host_init ()
{
    కొరకు (పూర్ణాంకం i = 3; i < d_m3_Wasi_Preopen_లెక్క; i++) {
        ఒకవేళ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_మార్గం, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

స్థిర
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    ఒకవేళ (close(i_fd) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#ఒకవేళ defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#లేదావేళ defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#అయితే_ముగింపు

స్థిర
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#ఒకవేళ defined(d_m3WasiDatasync)
    ఒకవేళ (d_m3WasiDatasync(i_fd) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
#లేకపోతే
    తిరిగి __WASI_ERRNO_NOSYS;
#అయితే_ముగింపు
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_స్థానభ్రంశం,
                                       __wasi_filesize_t i_నిడివి, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#ఒకవేళ defined(POSIX_FADV_NORMAL)
    పూర్ణాంకం adv;
    ఎంపిక (i_advice) {
    స్థితి __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; ఆపు;
    స్థితి __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; ఆపు;
    స్థితి __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; ఆపు;
    స్థితి __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; ఆపు;
    స్థితి __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; ఆపు;
    స్థితి __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; ఆపు;
    అప్రమేయ: తిరిగి __WASI_ERRNO_INVAL;
    }

    పూర్ణాంకం ret = posix_fadvise(i_fd, i_స్థానభ్రంశం, i_నిడివి, adv);
    ఒకవేళ (ret != 0) {
        తిరిగి errno_గమ్యం_wasi(ret);
    }
#లేకపోతే
    ఒకవేళ (i_advice > __WASI_ADVICE_NOREUSE) {
        తిరిగి __WASI_ERRNO_INVAL;
    }

    నిర్మాణం stat fd_stat;
    ఒకవేళ (fstat(i_fd, &fd_stat) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
#అయితే_ముగింపు

    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    పూర్ణాంకం fl = fcntl(i_fd, F_GETFL);
    ఒకవేళ (fl < 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    నిర్మాణం stat fd_stat;
    ఒకవేళ (fstat(i_fd, &fd_stat) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    *o_filetype = filetype_from_stat_బహుళకం(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    తిరిగి __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
స్థిర
__wasi_errno_t m3_wasi_host_fd_fdstat_సమితి_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    పూర్ణాంకం fl = fcntl(i_fd, F_GETFL);
    ఒకవేళ (fl < 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    స్థిరరాశి __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    ఒకవేళ ((i_flags & fixed) != (current & fixed)) {
        తిరిగి __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    ఒకవేళ (fcntl(i_fd, F_SETFL, fl) < 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, పరిమాణం(*o_stat));

    నిర్మాణం stat fd_stat;
    ఒకవేళ (fstat(i_fd, &fd_stat) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_సమితి_కొలత (__wasi_fd_t i_fd, __wasi_filesize_t i_కొలత)
{
    ఒకవేళ (ftruncate(i_fd, (off_t)i_కొలత) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_సమితి_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    నిర్మాణం timespec times[2];

    ఒకవేళ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } లేకపోతే ఒకవేళ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } లేకపోతే {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    ఒకవేళ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } లేకపోతే ఒకవేళ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } లేకపోతే {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    ఒకవేళ (futimens(i_fd, times) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_స్థానభ్రంశం, పూర్ణాంకం i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_స్థానభ్రంశం, i_whence);
    ఒకవేళ (ret < 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    తిరిగి __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#ఒకవేళ defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
స్థిర
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_పూర్తి)
{
    *o_పూర్తి = 0;

    నిర్మాణం iovec iovs[d_m3WasiMaxIovs];
    కొరకు (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (పూర్ణాంకం)i_iovsLen) : readv(i_fd, iovs, (పూర్ణాంకం)i_iovsLen);
    ఒకవేళ (ret < 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    *o_పూర్తి = (__wasi_size_t)ret;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

#లేకపోతే

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
స్థిర
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_పూర్తి)
{
    *o_పూర్తి = 0;

    size_t పూర్తి = 0;

    కొరకు (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        శూన్య*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ఒకవేళ (len == 0) {
            కొనసాగు;
        }

        ssize_t ret = i_write ? లిఖ(i_fd, addr, len) : పఠ(i_fd, addr, len);
        ఒకవేళ (ret < 0) {
            తిరిగి errno_గమ్యం_wasi(errno);
        }

        పూర్తి += (size_t)ret;
        ఒకవేళ ((size_t)ret < len) {
            ఆపు;
        }
    }

    *o_పూర్తి = (__wasi_size_t)పూర్తి;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

#అయితే_ముగింపు // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
స్థిర
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_స్థానభ్రంశం, bool i_write,
                            __wasi_size_t* o_పూర్తి)
{
    *o_పూర్తి = 0;

    size_t పూర్తి = 0;

    కొరకు (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        శూన్య*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        ఒకవేళ (len == 0) {
            కొనసాగు;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_స్థానభ్రంశం + పూర్తి))
                              : pread(i_fd, addr, len, (off_t)(i_స్థానభ్రంశం + పూర్తి));
        ఒకవేళ (ret < 0) {
            తిరిగి errno_గమ్యం_wasi(errno);
        }

        పూర్తి += (size_t)ret;
        ఒకవేళ ((size_t)ret < len) {
            ఆపు;
        }
    }

    *o_పూర్తి = (__wasi_size_t)పూర్తి;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_పూర్తి);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_పూర్తి);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_స్థానభ్రంశం,
                                      __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_at(i_fd, i_iovs, i_iovsLen, i_స్థానభ్రంశం, false, o_పూర్తి);
}

స్థిర
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, స్థిరరాశి m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_స్థానభ్రంశం,
                                       __wasi_size_t* o_పూర్తి)
{
    తిరిగి transfer_at(i_fd, i_iovs, i_iovsLen, i_స్థానభ్రంశం, true, o_పూర్తి);
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

    పూర్ణాంకం dirfd = preopen_fd(i_dirfd);
    ఒకవేళ (dirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    పూర్ణాంకం flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    ఒకవేళ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } లేకపోతే ఒకవేళ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } లేకపోతే ఒకవేళ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    పూర్ణాంకం host_fd = openat(dirfd, i_మార్గం, flags, 0644);
    ఒకవేళ (host_fd < 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_filestat (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, పరిమాణం(*o_stat));

    పూర్ణాంకం dirfd = preopen_fd(i_dirfd);
    ఒకవేళ (dirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    నిర్మాణం stat మార్గం_stat;
    ఒకవేళ (fstatat(dirfd, i_మార్గం, &మార్గం_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    filestat_from_stat(&మార్గం_stat, o_stat);
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    పూర్ణాంకం dirfd = preopen_fd(i_dirfd);
    ఒకవేళ (dirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    ఒకవేళ (mkdirat(dirfd, i_మార్గం, 0755) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    పూర్ణాంకం dirfd = preopen_fd(i_dirfd);
    ఒకవేళ (dirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    ఒకవేళ (unlinkat(dirfd, i_మార్గం, AT_REMOVEDIR) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    పూర్ణాంకం dirfd = preopen_fd(i_dirfd);
    ఒకవేళ (dirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    ఒకవేళ (unlinkat(dirfd, i_మార్గం, 0) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_మార్గం,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_మార్గం)
{
    పూర్ణాంకం oldDirfd = preopen_fd(i_oldDirfd);
    పూర్ణాంకం newDirfd = preopen_fd(i_newDirfd);
    ఒకవేళ (oldDirfd < 0 || newDirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    ఒకవేళ (renameat(oldDirfd, i_old_మార్గం, newDirfd, i_new_మార్గం) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_మార్గం, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_మార్గం)
{
    పూర్ణాంకం oldDirfd = preopen_fd(i_oldDirfd);
    పూర్ణాంకం newDirfd = preopen_fd(i_newDirfd);
    ఒకవేళ (oldDirfd < 0 || newDirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    ఒకవేళ (linkat(oldDirfd, i_old_మార్గం, newDirfd, i_new_మార్గం, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_symlink (ccstr_t i_లక్ష్యం, __wasi_fd_t i_dirfd, ccstr_t i_మార్గం)
{
    పూర్ణాంకం dirfd = preopen_fd(i_dirfd);
    ఒకవేళ (dirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    ఒకవేళ (symlinkat(i_లక్ష్యం, dirfd, i_మార్గం) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_మార్గం_readlink (__wasi_fd_t i_dirfd, ccstr_t i_మార్గం, అక్షరం* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_వాడిన)
{
    *o_buf_వాడిన = 0;

    పూర్ణాంకం dirfd = preopen_fd(i_dirfd);
    ఒకవేళ (dirfd < 0) {
        తిరిగి __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_మార్గం, o_buf, i_bufLen);
    ఒకవేళ (ret < 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    *o_buf_వాడిన = (__wasi_size_t)ret;
    తిరిగి __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

స్థిర
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ఒకవేళ (clk == d_m3ClockIdInvalid) {
        తిరిగి __WASI_ERRNO_INVAL;
    }

    నిర్మాణం timespec tp;
    ఒకవేళ (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } లేకపోతే {
        *o_res = convert_timespec(&tp);
    }
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_clock_సమయం_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_సమయం)
{
    *o_సమయం = 0;

    clockid_t clk = convert_clockid(i_clockId);
    ఒకవేళ (clk == d_m3ClockIdInvalid) {
        తిరిగి __WASI_ERRNO_INVAL;
    }

    నిర్మాణం timespec tp;
    ఒకవేళ (clock_gettime(clk, &tp) != 0) {
        తిరిగి errno_గమ్యం_wasi(errno);
    }

    *o_సమయం = convert_timespec(&tp);
    తిరిగి __WASI_ERRNO_SUCCESS;
}

స్థిర
__wasi_errno_t m3_wasi_host_random_get (శూన్య* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    అయితే (i_len) {
        ssize_t retlen;

#ఒకవేళ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  ఒకవేళ defined(__APPLE__) && (లక్ష్యం_OS_IPHONE || లక్ష్యం_IPHONE_SIMULATOR)
        retlen = Sec_Random_Copy_బైట్లు(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  లేకపోతే
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#లేదావేళ defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#లేకపోతే
        తిరిగి __WASI_ERRNO_NOSYS;
#అయితే_ముగింపు

        ఒకవేళ (retlen < 0) {
            ఒకవేళ (errno == EINTR || errno == EAGAIN) {
                కొనసాగు;
            }
            తిరిగి errno_గమ్యం_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    తిరిగి __WASI_ERRNO_SUCCESS;
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
స్థిర
uint64_t poll_మిగిలిన_ns (స్థిరరాశి m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    ఒకవేళ (not i_sub->isAbsolute) {
        తిరిగి (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    ఒకవేళ (clk == d_m3ClockIdInvalid) {
        తిరిగి UINT64_MAX;
    }

    నిర్మాణం timespec tp;
    ఒకవేళ (clock_gettime(clk, &tp) != 0) {
        తిరిగి 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    తిరిగి (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
స్థిర
__wasi_filesize_t poll_readable_బైట్లు (పూర్ణాంకం i_fd)
{
#ఒకవేళ defined(FIONREAD)
    పూర్ణాంకం అందుబాటు = 0;
    ఒకవేళ (ioctl(i_fd, FIONREAD, &అందుబాటు) == 0 and అందుబాటు > 0) {
        తిరిగి (__wasi_filesize_t)అందుబాటు;
    }
#అయితే_ముగింపు
    తిరిగి 1;
}

స్థిర
__wasi_errno_t m3_wasi_host_poll_oneoff (స్థిరరాశి m3_wasi_pollsub_t* i_subs, __wasi_size_t i_లెక్క,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_లెక్క)
{
    *o_event_లెక్క = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t అంకె_Fds = 0;

    కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
        ఒకవేళ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_మిగిలిన_ns(&i_subs[i], 0);
            ఒకవేళ (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } లేకపోతే {
            అంకె_Fds++;
        }
    }

    నిర్మాణం pollfd* pollfds = ఖాళీ;

    ఒకవేళ (అంకె_Fds) {
        pollfds = (నిర్మాణం pollfd*)m3_Malloc("WASI poll_oneoff", అంకె_Fds * పరిమాణం(నిర్మాణం pollfd));
        ఒకవేళ (pollfds == ఖాళీ) {
            తిరిగి __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
            ఒకవేళ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                కొనసాగు;
            }
            pollfds[n].fd = (పూర్ణాంకం)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    పూర్ణాంకం timeoutMs = -1;
    ఒకవేళ (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)గరిష్ఠ_పూర్ణాంకం) ? గరిష్ఠ_పూర్ణాంకం : (పూర్ణాంకం)ms;
    }

    పూర్ణాంకం ready;
    ఒకవేళ (అంకె_Fds) {
        చేయి {
            ready = poll(pollfds, (nfds_t)అంకె_Fds, timeoutMs);
        } అయితే (ready < 0 and errno == EINTR);
    } లేకపోతే ఒకవేళ (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        నిర్మాణం timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (దీర్ఘ)(soonest % 1000000000);

        అయితే (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } లేకపోతే ఒకవేళ (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } లేకపోతే {
        // no clocks and no descriptors: nothing that could ever become ready
        తిరిగి __WASI_ERRNO_INVAL;
    }

    ఒకవేళ (ready < 0) {
        __wasi_errno_t err = errno_గమ్యం_wasi(errno);
        m3_Free(pollfds);
        తిరిగి err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t బహిర్గతం = 0;
    __wasi_size_t n = 0;

    కొరకు (__wasi_size_t i = 0; i < i_లెక్క; i++) {
        స్థిరరాశి m3_wasi_pollsub_t* sub = &i_subs[i];

        ఒకవేళ (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            ఒకవేళ (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[బహిర్గతం].userdata = sub->userdata;
                o_events[బహిర్గతం].error = __WASI_ERRNO_INVAL;
                o_events[బహిర్గతం].type = __WASI_EVENTTYPE_CLOCK;
                o_events[బహిర్గతం].nbytes = 0;
                o_events[బహిర్గతం].flags = 0;
                బహిర్గతం++;
            } లేకపోతే ఒకవేళ (poll_మిగిలిన_ns(sub, elapsedNs) == 0) {
                o_events[బహిర్గతం].userdata = sub->userdata;
                o_events[బహిర్గతం].error = __WASI_ERRNO_SUCCESS;
                o_events[బహిర్గతం].type = __WASI_EVENTTYPE_CLOCK;
                o_events[బహిర్గతం].nbytes = 0;
                o_events[బహిర్గతం].flags = 0;
                బహిర్గతం++;
            }
            కొనసాగు;
        }

        పొట్టి revents = pollfds[n].revents;
        n++;

        ఒకవేళ (revents == 0) {
            కొనసాగు;
        }

        o_events[బహిర్గతం].userdata = sub->userdata;
        o_events[బహిర్గతం].type = sub->type;
        o_events[బహిర్గతం].nbytes = 0;
        o_events[బహిర్గతం].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        ఒకవేళ (revents & POLLNVAL) {
            o_events[బహిర్గతం].error = __WASI_ERRNO_BADF;
        } లేకపోతే ఒకవేళ (revents & POLLERR) {
            o_events[బహిర్గతం].error = __WASI_ERRNO_IO;
        } లేకపోతే {
            o_events[బహిర్గతం].error = __WASI_ERRNO_SUCCESS;
            ఒకవేళ (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[బహిర్గతం].nbytes = poll_readable_బైట్లు(pollfds[n - 1].fd);
            }
        }

        బహిర్గతం++;
    }

    m3_Free(pollfds);

    *o_event_లెక్క = బహిర్గతం;
    తిరిగి __WASI_ERRNO_SUCCESS;
}

#అయితే_ముగింపు // m3_api_wasi_posix_h
