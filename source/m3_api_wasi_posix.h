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

#অন্তর্ভাব "m3_core.h"

#যদি defined(_WIN32)
#  error "This file is not for Windows"
#endif

#অন্তর্ভাব <sys/types.h>
#অন্তর্ভাব <sys/stat.h>
#অন্তর্ভাব <errno.h>
#অন্তর্ভাব <fcntl.h>
#অন্তর্ভাব <limits.h>
#অন্তর্ভাব <poll.h>
#অন্তর্ভাব <string.h>
#অন্তর্ভাব <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#যদি !defined(__wasi__)
#  include <sys/ioctl.h>
#endif

#যদি defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  যদি defined(__APPLE__)
#    include <TargetConditionals.h>
#    যদি TARGET_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    অন্যথা // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  অন্যথা
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#endif

#অন্তর্ভাব "m3_api_wasi_host.h"


/*
 * Plumbing
 */

স্থির
__wasi_errno_t errno_to_wasi (পূর্ণাঙ্ক errnum)
{
    // clang-format off
    বিকল্প (errnum) {
    পক্ষ EPERM:    প্রত্যাগম __WASI_ERRNO_PERM;
    পক্ষ ENOENT:   প্রত্যাগম __WASI_ERRNO_NOENT;
    পক্ষ ESRCH:    প্রত্যাগম __WASI_ERRNO_SRCH;
    পক্ষ EINTR:    প্রত্যাগম __WASI_ERRNO_INTR;
    পক্ষ EIO:      প্রত্যাগম __WASI_ERRNO_IO;
    পক্ষ ENXIO:    প্রত্যাগম __WASI_ERRNO_NXIO;
    পক্ষ E2BIG:    প্রত্যাগম __WASI_ERRNO_2BIG;
    পক্ষ ENOEXEC:  প্রত্যাগম __WASI_ERRNO_NOEXEC;
    পক্ষ EBADF:    প্রত্যাগম __WASI_ERRNO_BADF;
    পক্ষ ECHILD:   প্রত্যাগম __WASI_ERRNO_CHILD;
    পক্ষ EAGAIN:   প্রত্যাগম __WASI_ERRNO_AGAIN;
    পক্ষ ENOMEM:   প্রত্যাগম __WASI_ERRNO_NOMEM;
    পক্ষ EACCES:   প্রত্যাগম __WASI_ERRNO_ACCES;
    পক্ষ EFAULT:   প্রত্যাগম __WASI_ERRNO_FAULT;
    পক্ষ EBUSY:    প্রত্যাগম __WASI_ERRNO_BUSY;
    পক্ষ EEXIST:   প্রত্যাগম __WASI_ERRNO_EXIST;
    পক্ষ EXDEV:    প্রত্যাগম __WASI_ERRNO_XDEV;
    পক্ষ ENODEV:   প্রত্যাগম __WASI_ERRNO_NODEV;
    পক্ষ ENOTDIR:  প্রত্যাগম __WASI_ERRNO_NOTDIR;
    পক্ষ EISDIR:   প্রত্যাগম __WASI_ERRNO_ISDIR;
    পক্ষ EINVAL:   প্রত্যাগম __WASI_ERRNO_INVAL;
    পক্ষ ENFILE:   প্রত্যাগম __WASI_ERRNO_NFILE;
    পক্ষ EMFILE:   প্রত্যাগম __WASI_ERRNO_MFILE;
    পক্ষ ENOTTY:   প্রত্যাগম __WASI_ERRNO_NOTTY;
    পক্ষ ETXTBSY:  প্রত্যাগম __WASI_ERRNO_TXTBSY;
    পক্ষ EFBIG:    প্রত্যাগম __WASI_ERRNO_FBIG;
    পক্ষ ENOSPC:   প্রত্যাগম __WASI_ERRNO_NOSPC;
    পক্ষ ESPIPE:   প্রত্যাগম __WASI_ERRNO_SPIPE;
    পক্ষ EROFS:    প্রত্যাগম __WASI_ERRNO_ROFS;
    পক্ষ EMLINK:   প্রত্যাগম __WASI_ERRNO_MLINK;
    পক্ষ EPIPE:    প্রত্যাগম __WASI_ERRNO_PIPE;
    পক্ষ EDOM:     প্রত্যাগম __WASI_ERRNO_DOM;
    পক্ষ ERANGE:   প্রত্যাগম __WASI_ERRNO_RANGE;
    }
    // clang-format on
    প্রত্যাগম __WASI_ERRNO_INVAL;
}

স্থির
__wasi_filetype_t filetype_from_stat_mode (পূর্ণাঙ্ক mode)
{
    প্রত্যাগম (S_ISBLK(mode) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(mode) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(mode) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(mode) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(mode) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

স্থির
শূন্য filestat_from_stat (ধ্রুব সংরচনা stat* i_stat, m3_wasi_filestat_t* o_stat)
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
স্থির
পূর্ণাঙ্ক preopen_fd (__wasi_fd_t i_fd)
{
    যদি (i_fd >= d_m3_Wasi_Preopen_গুনতি) {
        প্রত্যাগম -1;
    }
    প্রত্যাগম m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#যদি defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) রিক্ত)
#অন্যথা
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#endif

স্থির
clockid_t convert_clockid (__wasi_clockid_t in)
{
    বিকল্প (in) {
    পক্ষ __WASI_CLOCKID_MONOTONIC: প্রত্যাগম CLOCK_MONOTONIC;
    পক্ষ __WASI_CLOCKID_REALTIME: প্রত্যাগম CLOCK_REALTIME;
#যদি defined(CLOCK_PROCESS_CPUTIME_ID)
    পক্ষ __WASI_CLOCKID_PROCESS_CPUTIME_ID: প্রত্যাগম CLOCK_PROCESS_CPUTIME_ID;
#endif
#যদি defined(CLOCK_THREAD_CPUTIME_ID)
    পক্ষ __WASI_CLOCKID_THREAD_CPUTIME_ID: প্রত্যাগম CLOCK_THREAD_CPUTIME_ID;
#endif
    স্বভাব: প্রত্যাগম d_m3ClockIdInvalid;
    }
}

স্থির
__wasi_timestamp_t convert_timespec (ধ্রুব সংরচনা timespec* ts)
{
    যদি (ts->tv_sec < 0) {
        প্রত্যাগম 0;
    }
    যদি ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        প্রত্যাগম UINT64_MAX;
    }
    প্রত্যাগম (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

স্থির
শূন্য m3_wasi_host_init ()
{
    প্রতি (পূর্ণাঙ্ক i = 3; i < d_m3_Wasi_Preopen_গুনতি; i++) {
        যদি (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_path, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

স্থির
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    যদি (close(i_fd) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#যদি defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#নোচেদ্যদি defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#endif

স্থির
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#যদি defined(d_m3WasiDatasync)
    যদি (d_m3WasiDatasync(i_fd) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
#অন্যথা
    প্রত্যাগম __WASI_ERRNO_NOSYS;
#endif
}

স্থির
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#যদি defined(POSIX_FADV_NORMAL)
    পূর্ণাঙ্ক adv;
    বিকল্প (i_advice) {
    পক্ষ __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; ভঙ্গ;
    পক্ষ __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; ভঙ্গ;
    পক্ষ __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; ভঙ্গ;
    পক্ষ __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; ভঙ্গ;
    পক্ষ __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; ভঙ্গ;
    পক্ষ __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; ভঙ্গ;
    স্বভাব: প্রত্যাগম __WASI_ERRNO_INVAL;
    }

    পূর্ণাঙ্ক ret = posix_fadvise(i_fd, i_offset, i_length, adv);
    যদি (ret != 0) {
        প্রত্যাগম errno_to_wasi(ret);
    }
#অন্যথা
    যদি (i_advice > __WASI_ADVICE_NOREUSE) {
        প্রত্যাগম __WASI_ERRNO_INVAL;
    }

    সংরচনা stat fd_stat;
    যদি (fstat(i_fd, &fd_stat) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
#endif

    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    পূর্ণাঙ্ক fl = fcntl(i_fd, F_GETFL);
    যদি (fl < 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    সংরচনা stat fd_stat;
    যদি (fstat(i_fd, &fd_stat) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    *o_filetype = filetype_from_stat_mode(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
স্থির
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    পূর্ণাঙ্ক fl = fcntl(i_fd, F_GETFL);
    যদি (fl < 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    ধ্রুব __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    যদি ((i_flags & fixed) != (current & fixed)) {
        প্রত্যাগম __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    যদি (fcntl(i_fd, F_SETFL, fl) < 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, পরিমাণ(*o_stat));

    সংরচনা stat fd_stat;
    যদি (fstat(i_fd, &fd_stat) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    যদি (ftruncate(i_fd, (off_t)i_size) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    সংরচনা timespec times[2];

    যদি (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } অন্যথা যদি (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } অন্যথা {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    যদি (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } অন্যথা যদি (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } অন্যথা {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    যদি (futimens(i_fd, times) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, পূর্ণাঙ্ক i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_offset, i_whence);
    যদি (ret < 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#যদি defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
স্থির
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ধ্রুব m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    সংরচনা iovec iovs[d_m3WasiMaxIovs];
    প্রতি (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (পূর্ণাঙ্ক)i_iovsLen) : readv(i_fd, iovs, (পূর্ণাঙ্ক)i_iovsLen);
    যদি (ret < 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    *o_done = (__wasi_size_t)ret;
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

#অন্যথা

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
স্থির
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ধ্রুব m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    প্রতি (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        শূন্য*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        যদি (len == 0) {
            অনুবর্ত;
        }

        ssize_t ret = i_write ? লিখ(i_fd, addr, len) : পঠ(i_fd, addr, len);
        যদি (ret < 0) {
            প্রত্যাগম errno_to_wasi(errno);
        }

        done += (size_t)ret;
        যদি ((size_t)ret < len) {
            ভঙ্গ;
        }
    }

    *o_done = (__wasi_size_t)done;
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

#endif // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
স্থির
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, ধ্রুব m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset, bool i_write,
                            __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    প্রতি (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        শূন্য*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        যদি (len == 0) {
            অনুবর্ত;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_offset + done))
                              : pread(i_fd, addr, len, (off_t)(i_offset + done));
        যদি (ret < 0) {
            প্রত্যাগম errno_to_wasi(errno);
        }

        done += (size_t)ret;
        যদি ((size_t)ret < len) {
            ভঙ্গ;
        }
    }

    *o_done = (__wasi_size_t)done;
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ধ্রুব m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    প্রত্যাগম transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_done);
}

স্থির
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ধ্রুব m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    প্রত্যাগম transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_done);
}

স্থির
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ধ্রুব m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    প্রত্যাগম transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, false, o_done);
}

স্থির
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ধ্রুব m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    প্রত্যাগম transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, true, o_done);
}


/*
 * Paths
 */

স্থির
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    পূর্ণাঙ্ক dirfd = preopen_fd(i_dirfd);
    যদি (dirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    পূর্ণাঙ্ক flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    যদি ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } অন্যথা যদি ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } অন্যথা যদি ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    পূর্ণাঙ্ক host_fd = openat(dirfd, i_path, flags, 0644);
    যদি (host_fd < 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, পরিমাণ(*o_stat));

    পূর্ণাঙ্ক dirfd = preopen_fd(i_dirfd);
    যদি (dirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    সংরচনা stat path_stat;
    যদি (fstatat(dirfd, i_path, &path_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    filestat_from_stat(&path_stat, o_stat);
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    পূর্ণাঙ্ক dirfd = preopen_fd(i_dirfd);
    যদি (dirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    যদি (mkdirat(dirfd, i_path, 0755) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    পূর্ণাঙ্ক dirfd = preopen_fd(i_dirfd);
    যদি (dirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    যদি (unlinkat(dirfd, i_path, AT_REMOVEDIR) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    পূর্ণাঙ্ক dirfd = preopen_fd(i_dirfd);
    যদি (dirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    যদি (unlinkat(dirfd, i_path, 0) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    পূর্ণাঙ্ক oldDirfd = preopen_fd(i_oldDirfd);
    পূর্ণাঙ্ক newDirfd = preopen_fd(i_newDirfd);
    যদি (oldDirfd < 0 || newDirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    যদি (renameat(oldDirfd, i_oldPath, newDirfd, i_newPath) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    পূর্ণাঙ্ক oldDirfd = preopen_fd(i_oldDirfd);
    পূর্ণাঙ্ক newDirfd = preopen_fd(i_newDirfd);
    যদি (oldDirfd < 0 || newDirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    যদি (linkat(oldDirfd, i_oldPath, newDirfd, i_newPath, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    পূর্ণাঙ্ক dirfd = preopen_fd(i_dirfd);
    যদি (dirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    যদি (symlinkat(i_target, dirfd, i_path) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, বর্ণ* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    পূর্ণাঙ্ক dirfd = preopen_fd(i_dirfd);
    যদি (dirfd < 0) {
        প্রত্যাগম __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_path, o_buf, i_bufLen);
    যদি (ret < 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    *o_bufUsed = (__wasi_size_t)ret;
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

স্থির
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    যদি (clk == d_m3ClockIdInvalid) {
        প্রত্যাগম __WASI_ERRNO_INVAL;
    }

    সংরচনা timespec tp;
    যদি (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } অন্যথা {
        *o_res = convert_timespec(&tp);
    }
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = 0;

    clockid_t clk = convert_clockid(i_clockId);
    যদি (clk == d_m3ClockIdInvalid) {
        প্রত্যাগম __WASI_ERRNO_INVAL;
    }

    সংরচনা timespec tp;
    যদি (clock_gettime(clk, &tp) != 0) {
        প্রত্যাগম errno_to_wasi(errno);
    }

    *o_time = convert_timespec(&tp);
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

স্থির
__wasi_errno_t m3_wasi_host_random_get (শূন্য* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    যাবত্ (i_len) {
        ssize_t retlen;

#যদি defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  যদি defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
        retlen = SecRandomCopyBytes(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  অন্যথা
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#নোচেদ্যদি defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#অন্যথা
        প্রত্যাগম __WASI_ERRNO_NOSYS;
#endif

        যদি (retlen < 0) {
            যদি (errno == EINTR || errno == EAGAIN) {
                অনুবর্ত;
            }
            প্রত্যাগম errno_to_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    প্রত্যাগম __WASI_ERRNO_SUCCESS;
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
স্থির
uint64_t poll_remaining_ns (ধ্রুব m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    যদি (not i_sub->isAbsolute) {
        প্রত্যাগম (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    যদি (clk == d_m3ClockIdInvalid) {
        প্রত্যাগম UINT64_MAX;
    }

    সংরচনা timespec tp;
    যদি (clock_gettime(clk, &tp) != 0) {
        প্রত্যাগম 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    প্রত্যাগম (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
স্থির
__wasi_filesize_t poll_readable_bytes (পূর্ণাঙ্ক i_fd)
{
#যদি defined(FIONREAD)
    পূর্ণাঙ্ক available = 0;
    যদি (ioctl(i_fd, FIONREAD, &available) == 0 and available > 0) {
        প্রত্যাগম (__wasi_filesize_t)available;
    }
#endif
    প্রত্যাগম 1;
}

স্থির
__wasi_errno_t m3_wasi_host_poll_oneoff (ধ্রুব m3_wasi_pollsub_t* i_subs, __wasi_size_t i_গুনতি,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_গুনতি)
{
    *o_event_গুনতি = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t অঙ্ক_Fds = 0;

    প্রতি (__wasi_size_t i = 0; i < i_গুনতি; i++) {
        যদি (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_remaining_ns(&i_subs[i], 0);
            যদি (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } অন্যথা {
            অঙ্ক_Fds++;
        }
    }

    সংরচনা pollfd* pollfds = রিক্ত;

    যদি (অঙ্ক_Fds) {
        pollfds = (সংরচনা pollfd*)m3_Malloc("WASI poll_oneoff", অঙ্ক_Fds * পরিমাণ(সংরচনা pollfd));
        যদি (pollfds == রিক্ত) {
            প্রত্যাগম __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        প্রতি (__wasi_size_t i = 0; i < i_গুনতি; i++) {
            যদি (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                অনুবর্ত;
            }
            pollfds[n].fd = (পূর্ণাঙ্ক)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    পূর্ণাঙ্ক timeoutMs = -1;
    যদি (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)INT_MAX) ? INT_MAX : (পূর্ণাঙ্ক)ms;
    }

    পূর্ণাঙ্ক ready;
    যদি (অঙ্ক_Fds) {
        কুরু {
            ready = poll(pollfds, (nfds_t)অঙ্ক_Fds, timeoutMs);
        } যাবত্ (ready < 0 and errno == EINTR);
    } অন্যথা যদি (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        সংরচনা timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (long)(soonest % 1000000000);

        যাবত্ (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } অন্যথা যদি (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } অন্যথা {
        // no clocks and no descriptors: nothing that could ever become ready
        প্রত্যাগম __WASI_ERRNO_INVAL;
    }

    যদি (ready < 0) {
        __wasi_errno_t err = errno_to_wasi(errno);
        m3_Free(pollfds);
        প্রত্যাগম err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t out = 0;
    __wasi_size_t n = 0;

    প্রতি (__wasi_size_t i = 0; i < i_গুনতি; i++) {
        ধ্রুব m3_wasi_pollsub_t* sub = &i_subs[i];

        যদি (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            যদি (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_INVAL;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            } অন্যথা যদি (poll_remaining_ns(sub, elapsedNs) == 0) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_SUCCESS;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            }
            অনুবর্ত;
        }

        short revents = pollfds[n].revents;
        n++;

        যদি (revents == 0) {
            অনুবর্ত;
        }

        o_events[out].userdata = sub->userdata;
        o_events[out].type = sub->type;
        o_events[out].nbytes = 0;
        o_events[out].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        যদি (revents & POLLNVAL) {
            o_events[out].error = __WASI_ERRNO_BADF;
        } অন্যথা যদি (revents & POLLERR) {
            o_events[out].error = __WASI_ERRNO_IO;
        } অন্যথা {
            o_events[out].error = __WASI_ERRNO_SUCCESS;
            যদি (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[out].nbytes = poll_readable_bytes(pollfds[n - 1].fd);
            }
        }

        out++;
    }

    m3_Free(pollfds);

    *o_event_গুনতি = out;
    প্রত্যাগম __WASI_ERRNO_SUCCESS;
}

#endif // m3_api_wasi_posix_h
