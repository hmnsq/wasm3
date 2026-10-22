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

#اگر_غیر_تعریف_شدہ m3_api_wasi_posix_h
#تعریف_کرو m3_api_wasi_posix_h

#شامل "m3_core.h"

#اگر defined(_WIN32)
#  error "This file is not for Windows"
#اگر_ختم

#شامل <sys/types.h>
#شامل <sys/stat.h>
#شامل <errno.h>
#شامل <fcntl.h>
#شامل <limits.h>
#شامل <poll.h>
#شامل <string.h>
#شامل <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#اگر !defined(__wasi__)
#  include <sys/ioctl.h>
#اگر_ختم

#اگر defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  اگر defined(__APPLE__)
#    include <TargetConditionals.h>
#    اگر ہدف_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    ورنہ // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  ورنہ
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#اگر_ختم

#شامل "m3_api_wasi_host.h"


/*
 * Plumbing
 */

ساکن
__wasi_errno_t errno_مقصد_wasi (صحیح errnum)
{
    // clang-format off
    انتخاب (errnum) {
    حالت EPERM:    واپس __WASI_ERRNO_PERM;
    حالت ENOENT:   واپس __WASI_ERRNO_NOENT;
    حالت ESRCH:    واپس __WASI_ERRNO_SRCH;
    حالت EINTR:    واپس __WASI_ERRNO_INTR;
    حالت EIO:      واپس __WASI_ERRNO_IO;
    حالت ENXIO:    واپس __WASI_ERRNO_NXIO;
    حالت E2BIG:    واپس __WASI_ERRNO_2BIG;
    حالت ENOEXEC:  واپس __WASI_ERRNO_NOEXEC;
    حالت EBADF:    واپس __WASI_ERRNO_BADF;
    حالت ECHILD:   واپس __WASI_ERRNO_CHILD;
    حالت EAGAIN:   واپس __WASI_ERRNO_AGAIN;
    حالت ENOMEM:   واپس __WASI_ERRNO_NOMEM;
    حالت EACCES:   واپس __WASI_ERRNO_ACCES;
    حالت EFAULT:   واپس __WASI_ERRNO_FAULT;
    حالت EBUSY:    واپس __WASI_ERRNO_BUSY;
    حالت EEXIST:   واپس __WASI_ERRNO_EXIST;
    حالت EXDEV:    واپس __WASI_ERRNO_XDEV;
    حالت ENODEV:   واپس __WASI_ERRNO_NODEV;
    حالت ENOTDIR:  واپس __WASI_ERRNO_NOTDIR;
    حالت EISDIR:   واپس __WASI_ERRNO_ISDIR;
    حالت EINVAL:   واپس __WASI_ERRNO_INVAL;
    حالت ENFILE:   واپس __WASI_ERRNO_NFILE;
    حالت EMFILE:   واپس __WASI_ERRNO_MFILE;
    حالت ENOTTY:   واپس __WASI_ERRNO_NOTTY;
    حالت ETXTBSY:  واپس __WASI_ERRNO_TXTBSY;
    حالت EFBIG:    واپس __WASI_ERRNO_FBIG;
    حالت ENOSPC:   واپس __WASI_ERRNO_NOSPC;
    حالت ESPIPE:   واپس __WASI_ERRNO_SPIPE;
    حالت EROFS:    واپس __WASI_ERRNO_ROFS;
    حالت EMLINK:   واپس __WASI_ERRNO_MLINK;
    حالت EPIPE:    واپس __WASI_ERRNO_PIPE;
    حالت EDOM:     واپس __WASI_ERRNO_DOM;
    حالت ERANGE:   واپس __WASI_ERRNO_RANGE;
    }
    // clang-format on
    واپس __WASI_ERRNO_INVAL;
}

ساکن
__wasi_filetype_t filetype_from_stat_منوال (صحیح منوال)
{
    واپس (S_ISBLK(منوال) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(منوال) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(منوال) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(منوال) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(منوال) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

ساکن
خالی filestat_from_stat (ثابت ساخت stat* i_stat, m3_wasi_filestat_t* o_stat)
{
    o_stat->dev = (uint64_t)i_stat->st_dev;
    o_stat->ino = (uint64_t)i_stat->st_ino;
    o_stat->nlink = (uint64_t)i_stat->st_nlink;
    o_stat->سائز = (uint64_t)i_stat->st_size;
    o_stat->atim = (uint64_t)i_stat->st_atime * 1000000000ULL;
    o_stat->mtim = (uint64_t)i_stat->st_mtime * 1000000000ULL;
    o_stat->ctim = (uint64_t)i_stat->st_ctime * 1000000000ULL;
    o_stat->filetype = filetype_from_stat_منوال(i_stat->st_mode);
}

// The descriptor a preopened directory holds, or -1 where i_fd names no preopen
ساکن
صحیح preopen_fd (__wasi_fd_t i_fd)
{
    اگر (i_fd >= d_m3_Wasi_Preopen_گنتی) {
        واپس -1;
    }
    واپس m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#اگر defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) تہی)
#ورنہ
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#اگر_ختم

ساکن
clockid_t convert_clockid (__wasi_clockid_t اندر)
{
    انتخاب (اندر) {
    حالت __WASI_CLOCKID_MONOTONIC: واپس CLOCK_MONOTONIC;
    حالت __WASI_CLOCKID_REALTIME: واپس CLOCK_REALTIME;
#اگر defined(CLOCK_PROCESS_CPUTIME_ID)
    حالت __WASI_CLOCKID_PROCESS_CPUTIME_ID: واپس CLOCK_PROCESS_CPUTIME_ID;
#اگر_ختم
#اگر defined(CLOCK_THREAD_CPUTIME_ID)
    حالت __WASI_CLOCKID_THREAD_CPUTIME_ID: واپس CLOCK_THREAD_CPUTIME_ID;
#اگر_ختم
    طےشدہ: واپس d_m3ClockIdInvalid;
    }
}

ساکن
__wasi_timestamp_t convert_timespec (ثابت ساخت timespec* ts)
{
    اگر (ts->tv_sec < 0) {
        واپس 0;
    }
    اگر ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        واپس UINT64_MAX;
    }
    واپس (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

ساکن
خالی m3_wasi_host_init ()
{
    برائے (صحیح i = 3; i < d_m3_Wasi_Preopen_گنتی; i++) {
        اگر (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_راستہ, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ساکن
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    اگر (close(i_fd) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#اگر defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#وگرنہ_اگر defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#اگر_ختم

ساکن
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#اگر defined(d_m3WasiDatasync)
    اگر (d_m3WasiDatasync(i_fd) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
#ورنہ
    واپس __WASI_ERRNO_NOSYS;
#اگر_ختم
}

ساکن
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_انحراف,
                                       __wasi_filesize_t i_لمبائی, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#اگر defined(POSIX_FADV_NORMAL)
    صحیح adv;
    انتخاب (i_advice) {
    حالت __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; توڑو;
    حالت __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; توڑو;
    حالت __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; توڑو;
    حالت __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; توڑو;
    حالت __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; توڑو;
    حالت __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; توڑو;
    طےشدہ: واپس __WASI_ERRNO_INVAL;
    }

    صحیح ret = posix_fadvise(i_fd, i_انحراف, i_لمبائی, adv);
    اگر (ret != 0) {
        واپس errno_مقصد_wasi(ret);
    }
#ورنہ
    اگر (i_advice > __WASI_ADVICE_NOREUSE) {
        واپس __WASI_ERRNO_INVAL;
    }

    ساخت stat fd_stat;
    اگر (fstat(i_fd, &fd_stat) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
#اگر_ختم

    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    صحیح fl = fcntl(i_fd, F_GETFL);
    اگر (fl < 0) {
        واپس errno_مقصد_wasi(errno);
    }

    ساخت stat fd_stat;
    اگر (fstat(i_fd, &fd_stat) != 0) {
        واپس errno_مقصد_wasi(errno);
    }

    *o_filetype = filetype_from_stat_منوال(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    واپس __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
ساکن
__wasi_errno_t m3_wasi_host_fd_fdstat_سیٹ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    صحیح fl = fcntl(i_fd, F_GETFL);
    اگر (fl < 0) {
        واپس errno_مقصد_wasi(errno);
    }

    ثابت __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    اگر ((i_flags & fixed) != (current & fixed)) {
        واپس __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    اگر (fcntl(i_fd, F_SETFL, fl) < 0) {
        واپس errno_مقصد_wasi(errno);
    }

    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    ساخت stat fd_stat;
    اگر (fstat(i_fd, &fd_stat) != 0) {
        واپس errno_مقصد_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_سیٹ_سائز (__wasi_fd_t i_fd, __wasi_filesize_t i_سائز)
{
    اگر (ftruncate(i_fd, (off_t)i_سائز) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_سیٹ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    ساخت timespec times[2];

    اگر (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } ورنہ اگر (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } ورنہ {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    اگر (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } ورنہ اگر (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } ورنہ {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    اگر (futimens(i_fd, times) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_انحراف, صحیح i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_انحراف, i_whence);
    اگر (ret < 0) {
        واپس errno_مقصد_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    واپس __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#اگر defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
ساکن
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_مکمل)
{
    *o_مکمل = 0;

    ساخت iovec iovs[d_m3WasiMaxIovs];
    برائے (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (صحیح)i_iovsLen) : readv(i_fd, iovs, (صحیح)i_iovsLen);
    اگر (ret < 0) {
        واپس errno_مقصد_wasi(errno);
    }

    *o_مکمل = (__wasi_size_t)ret;
    واپس __WASI_ERRNO_SUCCESS;
}

#ورنہ

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
ساکن
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_مکمل)
{
    *o_مکمل = 0;

    size_t مکمل = 0;

    برائے (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        خالی*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        اگر (len == 0) {
            جاری;
        }

        ssize_t ret = i_write ? اكتب(i_fd, addr, len) : اقرأ(i_fd, addr, len);
        اگر (ret < 0) {
            واپس errno_مقصد_wasi(errno);
        }

        مکمل += (size_t)ret;
        اگر ((size_t)ret < len) {
            توڑو;
        }
    }

    *o_مکمل = (__wasi_size_t)مکمل;
    واپس __WASI_ERRNO_SUCCESS;
}

#اگر_ختم // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
ساکن
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_انحراف, bool i_write,
                            __wasi_size_t* o_مکمل)
{
    *o_مکمل = 0;

    size_t مکمل = 0;

    برائے (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        خالی*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        اگر (len == 0) {
            جاری;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_انحراف + مکمل))
                              : pread(i_fd, addr, len, (off_t)(i_انحراف + مکمل));
        اگر (ret < 0) {
            واپس errno_مقصد_wasi(errno);
        }

        مکمل += (size_t)ret;
        اگر ((size_t)ret < len) {
            توڑو;
        }
    }

    *o_مکمل = (__wasi_size_t)مکمل;
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_مکمل)
{
    واپس transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_مکمل);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_مکمل)
{
    واپس transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_مکمل);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_انحراف,
                                      __wasi_size_t* o_مکمل)
{
    واپس transfer_at(i_fd, i_iovs, i_iovsLen, i_انحراف, false, o_مکمل);
}

ساکن
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_انحراف,
                                       __wasi_size_t* o_مکمل)
{
    واپس transfer_at(i_fd, i_iovs, i_iovsLen, i_انحراف, true, o_مکمل);
}


/*
 * Paths
 */

ساکن
__wasi_errno_t m3_wasi_host_راستہ_open (__wasi_fd_t i_dirfd, ccstr_t i_راستہ,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    صحیح dirfd = preopen_fd(i_dirfd);
    اگر (dirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    صحیح flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    اگر ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } ورنہ اگر ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } ورنہ اگر ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    صحیح host_fd = openat(dirfd, i_راستہ, flags, 0644);
    اگر (host_fd < 0) {
        واپس errno_مقصد_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_راستہ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    صحیح dirfd = preopen_fd(i_dirfd);
    اگر (dirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    ساخت stat راستہ_stat;
    اگر (fstatat(dirfd, i_راستہ, &راستہ_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        واپس errno_مقصد_wasi(errno);
    }

    filestat_from_stat(&راستہ_stat, o_stat);
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    صحیح dirfd = preopen_fd(i_dirfd);
    اگر (dirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    اگر (mkdirat(dirfd, i_راستہ, 0755) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    صحیح dirfd = preopen_fd(i_dirfd);
    اگر (dirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    اگر (unlinkat(dirfd, i_راستہ, AT_REMOVEDIR) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    صحیح dirfd = preopen_fd(i_dirfd);
    اگر (dirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    اگر (unlinkat(dirfd, i_راستہ, 0) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_راستہ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_راستہ)
{
    صحیح oldDirfd = preopen_fd(i_oldDirfd);
    صحیح newDirfd = preopen_fd(i_newDirfd);
    اگر (oldDirfd < 0 || newDirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    اگر (renameat(oldDirfd, i_old_راستہ, newDirfd, i_new_راستہ) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_راستہ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_راستہ)
{
    صحیح oldDirfd = preopen_fd(i_oldDirfd);
    صحیح newDirfd = preopen_fd(i_newDirfd);
    اگر (oldDirfd < 0 || newDirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    اگر (linkat(oldDirfd, i_old_راستہ, newDirfd, i_new_راستہ, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_symlink (ccstr_t i_ہدف, __wasi_fd_t i_dirfd, ccstr_t i_راستہ)
{
    صحیح dirfd = preopen_fd(i_dirfd);
    اگر (dirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    اگر (symlinkat(i_ہدف, dirfd, i_راستہ) != 0) {
        واپس errno_مقصد_wasi(errno);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_راستہ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_راستہ, حرف* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_مستعمل)
{
    *o_buf_مستعمل = 0;

    صحیح dirfd = preopen_fd(i_dirfd);
    اگر (dirfd < 0) {
        واپس __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_راستہ, o_buf, i_bufLen);
    اگر (ret < 0) {
        واپس errno_مقصد_wasi(errno);
    }

    *o_buf_مستعمل = (__wasi_size_t)ret;
    واپس __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

ساکن
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    اگر (clk == d_m3ClockIdInvalid) {
        واپس __WASI_ERRNO_INVAL;
    }

    ساخت timespec tp;
    اگر (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } ورنہ {
        *o_res = convert_timespec(&tp);
    }
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_clock_وقت_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_وقت)
{
    *o_وقت = 0;

    clockid_t clk = convert_clockid(i_clockId);
    اگر (clk == d_m3ClockIdInvalid) {
        واپس __WASI_ERRNO_INVAL;
    }

    ساخت timespec tp;
    اگر (clock_gettime(clk, &tp) != 0) {
        واپس errno_مقصد_wasi(errno);
    }

    *o_وقت = convert_timespec(&tp);
    واپس __WASI_ERRNO_SUCCESS;
}

ساکن
__wasi_errno_t m3_wasi_host_random_get (خالی* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    جب (i_len) {
        ssize_t retlen;

#اگر defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  اگر defined(__APPLE__) && (ہدف_OS_IPHONE || ہدف_IPHONE_SIMULATOR)
        retlen = Sec_Random_Copy_بائٹیں(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  ورنہ
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#وگرنہ_اگر defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#ورنہ
        واپس __WASI_ERRNO_NOSYS;
#اگر_ختم

        اگر (retlen < 0) {
            اگر (errno == EINTR || errno == EAGAIN) {
                جاری;
            }
            واپس errno_مقصد_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    واپس __WASI_ERRNO_SUCCESS;
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
ساکن
uint64_t poll_بقایا_ns (ثابت m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    اگر (not i_sub->isAbsolute) {
        واپس (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    اگر (clk == d_m3ClockIdInvalid) {
        واپس UINT64_MAX;
    }

    ساخت timespec tp;
    اگر (clock_gettime(clk, &tp) != 0) {
        واپس 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    واپس (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
ساکن
__wasi_filesize_t poll_readable_بائٹیں (صحیح i_fd)
{
#اگر defined(FIONREAD)
    صحیح دستیاب = 0;
    اگر (ioctl(i_fd, FIONREAD, &دستیاب) == 0 and دستیاب > 0) {
        واپس (__wasi_filesize_t)دستیاب;
    }
#اگر_ختم
    واپس 1;
}

ساکن
__wasi_errno_t m3_wasi_host_poll_oneoff (ثابت m3_wasi_pollsub_t* i_subs, __wasi_size_t i_گنتی,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_گنتی)
{
    *o_event_گنتی = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t عدد_Fds = 0;

    برائے (__wasi_size_t i = 0; i < i_گنتی; i++) {
        اگر (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_بقایا_ns(&i_subs[i], 0);
            اگر (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } ورنہ {
            عدد_Fds++;
        }
    }

    ساخت pollfd* pollfds = تہی;

    اگر (عدد_Fds) {
        pollfds = (ساخت pollfd*)m3_Malloc("WASI poll_oneoff", عدد_Fds * حجم(ساخت pollfd));
        اگر (pollfds == تہی) {
            واپس __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        برائے (__wasi_size_t i = 0; i < i_گنتی; i++) {
            اگر (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                جاری;
            }
            pollfds[n].fd = (صحیح)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    صحیح timeoutMs = -1;
    اگر (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)زیادہ_سے_زیادہ_عدد) ? زیادہ_سے_زیادہ_عدد : (صحیح)ms;
    }

    صحیح ready;
    اگر (عدد_Fds) {
        کرو {
            ready = poll(pollfds, (nfds_t)عدد_Fds, timeoutMs);
        } جب (ready < 0 and errno == EINTR);
    } ورنہ اگر (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        ساخت timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (طویل)(soonest % 1000000000);

        جب (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } ورنہ اگر (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } ورنہ {
        // no clocks and no descriptors: nothing that could ever become ready
        واپس __WASI_ERRNO_INVAL;
    }

    اگر (ready < 0) {
        __wasi_errno_t err = errno_مقصد_wasi(errno);
        m3_Free(pollfds);
        واپس err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t برآمد = 0;
    __wasi_size_t n = 0;

    برائے (__wasi_size_t i = 0; i < i_گنتی; i++) {
        ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

        اگر (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            اگر (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[برآمد].userdata = sub->userdata;
                o_events[برآمد].error = __WASI_ERRNO_INVAL;
                o_events[برآمد].type = __WASI_EVENTTYPE_CLOCK;
                o_events[برآمد].nbytes = 0;
                o_events[برآمد].flags = 0;
                برآمد++;
            } ورنہ اگر (poll_بقایا_ns(sub, elapsedNs) == 0) {
                o_events[برآمد].userdata = sub->userdata;
                o_events[برآمد].error = __WASI_ERRNO_SUCCESS;
                o_events[برآمد].type = __WASI_EVENTTYPE_CLOCK;
                o_events[برآمد].nbytes = 0;
                o_events[برآمد].flags = 0;
                برآمد++;
            }
            جاری;
        }

        مختصر revents = pollfds[n].revents;
        n++;

        اگر (revents == 0) {
            جاری;
        }

        o_events[برآمد].userdata = sub->userdata;
        o_events[برآمد].type = sub->type;
        o_events[برآمد].nbytes = 0;
        o_events[برآمد].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        اگر (revents & POLLNVAL) {
            o_events[برآمد].error = __WASI_ERRNO_BADF;
        } ورنہ اگر (revents & POLLERR) {
            o_events[برآمد].error = __WASI_ERRNO_IO;
        } ورنہ {
            o_events[برآمد].error = __WASI_ERRNO_SUCCESS;
            اگر (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[برآمد].nbytes = poll_readable_بائٹیں(pollfds[n - 1].fd);
            }
        }

        برآمد++;
    }

    m3_Free(pollfds);

    *o_event_گنتی = برآمد;
    واپس __WASI_ERRNO_SUCCESS;
}

#اگر_ختم // m3_api_wasi_posix_h
