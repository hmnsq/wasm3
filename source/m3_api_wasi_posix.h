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

#جے_بنا_تعریف m3_api_wasi_posix_h
#تعریف_کرو m3_api_wasi_posix_h

#ضمن "m3_core.h"

#إذا defined(_WIN32)
#  error "This file is not for Windows"
#جے_انت

#ضمن <sys/types.h>
#ضمن <sys/stat.h>
#ضمن <errno.h>
#ضمن <fcntl.h>
#ضمن <limits.h>
#ضمن <poll.h>
#ضمن <string.h>
#ضمن <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#إذا !defined(__wasi__)
#  include <sys/ioctl.h>
#جے_انت

#إذا defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  إذا defined(__APPLE__)
#    include <TargetConditionals.h>
#    إذا نشانہ_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    وإلا // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  وإلا
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#جے_انت

#ضمن "m3_api_wasi_host.h"


/*
 * Plumbing
 */

ساكن
__wasi_errno_t errno_منزل_wasi (صحيح errnum)
{
    // clang-format off
    بدل (errnum) {
    حالة EPERM:    أرجع __WASI_ERRNO_PERM;
    حالة ENOENT:   أرجع __WASI_ERRNO_NOENT;
    حالة ESRCH:    أرجع __WASI_ERRNO_SRCH;
    حالة EINTR:    أرجع __WASI_ERRNO_INTR;
    حالة EIO:      أرجع __WASI_ERRNO_IO;
    حالة ENXIO:    أرجع __WASI_ERRNO_NXIO;
    حالة E2BIG:    أرجع __WASI_ERRNO_2BIG;
    حالة ENOEXEC:  أرجع __WASI_ERRNO_NOEXEC;
    حالة EBADF:    أرجع __WASI_ERRNO_BADF;
    حالة ECHILD:   أرجع __WASI_ERRNO_CHILD;
    حالة EAGAIN:   أرجع __WASI_ERRNO_AGAIN;
    حالة ENOMEM:   أرجع __WASI_ERRNO_NOMEM;
    حالة EACCES:   أرجع __WASI_ERRNO_ACCES;
    حالة EFAULT:   أرجع __WASI_ERRNO_FAULT;
    حالة EBUSY:    أرجع __WASI_ERRNO_BUSY;
    حالة EEXIST:   أرجع __WASI_ERRNO_EXIST;
    حالة EXDEV:    أرجع __WASI_ERRNO_XDEV;
    حالة ENODEV:   أرجع __WASI_ERRNO_NODEV;
    حالة ENOTDIR:  أرجع __WASI_ERRNO_NOTDIR;
    حالة EISDIR:   أرجع __WASI_ERRNO_ISDIR;
    حالة EINVAL:   أرجع __WASI_ERRNO_INVAL;
    حالة ENFILE:   أرجع __WASI_ERRNO_NFILE;
    حالة EMFILE:   أرجع __WASI_ERRNO_MFILE;
    حالة ENOTTY:   أرجع __WASI_ERRNO_NOTTY;
    حالة ETXTBSY:  أرجع __WASI_ERRNO_TXTBSY;
    حالة EFBIG:    أرجع __WASI_ERRNO_FBIG;
    حالة ENOSPC:   أرجع __WASI_ERRNO_NOSPC;
    حالة ESPIPE:   أرجع __WASI_ERRNO_SPIPE;
    حالة EROFS:    أرجع __WASI_ERRNO_ROFS;
    حالة EMLINK:   أرجع __WASI_ERRNO_MLINK;
    حالة EPIPE:    أرجع __WASI_ERRNO_PIPE;
    حالة EDOM:     أرجع __WASI_ERRNO_DOM;
    حالة ERANGE:   أرجع __WASI_ERRNO_RANGE;
    }
    // clang-format on
    أرجع __WASI_ERRNO_INVAL;
}

ساكن
__wasi_filetype_t filetype_from_stat_منوال (صحيح منوال)
{
    أرجع (S_ISBLK(منوال) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(منوال) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(منوال) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(منوال) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(منوال) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

ساكن
فراغ filestat_from_stat (ثابت بنية stat* i_stat, m3_wasi_filestat_t* o_stat)
{
    o_stat->dev = (uint64_t)i_stat->st_dev;
    o_stat->ino = (uint64_t)i_stat->st_ino;
    o_stat->nlink = (uint64_t)i_stat->st_nlink;
    o_stat->ناپ = (uint64_t)i_stat->st_size;
    o_stat->atim = (uint64_t)i_stat->st_atime * 1000000000ULL;
    o_stat->mtim = (uint64_t)i_stat->st_mtime * 1000000000ULL;
    o_stat->ctim = (uint64_t)i_stat->st_ctime * 1000000000ULL;
    o_stat->filetype = filetype_from_stat_منوال(i_stat->st_mode);
}

// The descriptor a preopened directory holds, or -1 where i_fd names no preopen
ساكن
صحيح preopen_fd (__wasi_fd_t i_fd)
{
    إذا (i_fd >= d_m3_Wasi_Preopen_گنتی) {
        أرجع -1;
    }
    أرجع m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#إذا defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) عدم)
#وإلا
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#جے_انت

ساكن
clockid_t convert_clockid (__wasi_clockid_t وچ)
{
    بدل (وچ) {
    حالة __WASI_CLOCKID_MONOTONIC: أرجع CLOCK_MONOTONIC;
    حالة __WASI_CLOCKID_REALTIME: أرجع CLOCK_REALTIME;
#إذا defined(CLOCK_PROCESS_CPUTIME_ID)
    حالة __WASI_CLOCKID_PROCESS_CPUTIME_ID: أرجع CLOCK_PROCESS_CPUTIME_ID;
#جے_انت
#إذا defined(CLOCK_THREAD_CPUTIME_ID)
    حالة __WASI_CLOCKID_THREAD_CPUTIME_ID: أرجع CLOCK_THREAD_CPUTIME_ID;
#جے_انت
    افتراضي: أرجع d_m3ClockIdInvalid;
    }
}

ساكن
__wasi_timestamp_t convert_timespec (ثابت بنية timespec* ts)
{
    إذا (ts->tv_sec < 0) {
        أرجع 0;
    }
    إذا ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        أرجع UINT64_MAX;
    }
    أرجع (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

ساكن
فراغ m3_wasi_host_init ()
{
    لكل (صحيح i = 3; i < d_m3_Wasi_Preopen_گنتی; i++) {
        إذا (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_رستہ, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

ساكن
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    إذا (close(i_fd) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#إذا defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#وإلاإذا defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#جے_انت

ساكن
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#إذا defined(d_m3WasiDatasync)
    إذا (d_m3WasiDatasync(i_fd) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
#وإلا
    أرجع __WASI_ERRNO_NOSYS;
#جے_انت
}

ساكن
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_وتھ,
                                       __wasi_filesize_t i_لمائی, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#إذا defined(POSIX_FADV_NORMAL)
    صحيح adv;
    بدل (i_advice) {
    حالة __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; اكسر;
    حالة __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; اكسر;
    حالة __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; اكسر;
    حالة __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; اكسر;
    حالة __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; اكسر;
    حالة __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; اكسر;
    افتراضي: أرجع __WASI_ERRNO_INVAL;
    }

    صحيح ret = posix_fadvise(i_fd, i_وتھ, i_لمائی, adv);
    إذا (ret != 0) {
        أرجع errno_منزل_wasi(ret);
    }
#وإلا
    إذا (i_advice > __WASI_ADVICE_NOREUSE) {
        أرجع __WASI_ERRNO_INVAL;
    }

    بنية stat fd_stat;
    إذا (fstat(i_fd, &fd_stat) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
#جے_انت

    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    صحيح fl = fcntl(i_fd, F_GETFL);
    إذا (fl < 0) {
        أرجع errno_منزل_wasi(errno);
    }

    بنية stat fd_stat;
    إذا (fstat(i_fd, &fd_stat) != 0) {
        أرجع errno_منزل_wasi(errno);
    }

    *o_filetype = filetype_from_stat_منوال(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    أرجع __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
ساكن
__wasi_errno_t m3_wasi_host_fd_fdstat_سیٹ_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    صحيح fl = fcntl(i_fd, F_GETFL);
    إذا (fl < 0) {
        أرجع errno_منزل_wasi(errno);
    }

    ثابت __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    إذا ((i_flags & fixed) != (current & fixed)) {
        أرجع __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    إذا (fcntl(i_fd, F_SETFL, fl) < 0) {
        أرجع errno_منزل_wasi(errno);
    }

    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    بنية stat fd_stat;
    إذا (fstat(i_fd, &fd_stat) != 0) {
        أرجع errno_منزل_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_سیٹ_ناپ (__wasi_fd_t i_fd, __wasi_filesize_t i_ناپ)
{
    إذا (ftruncate(i_fd, (off_t)i_ناپ) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_سیٹ_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    بنية timespec times[2];

    إذا (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } وإلا إذا (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } وإلا {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    إذا (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } وإلا إذا (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } وإلا {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    إذا (futimens(i_fd, times) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_وتھ, صحيح i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_وتھ, i_whence);
    إذا (ret < 0) {
        أرجع errno_منزل_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    أرجع __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#إذا defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
ساكن
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_پورا)
{
    *o_پورا = 0;

    بنية iovec iovs[d_m3WasiMaxIovs];
    لكل (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (صحيح)i_iovsLen) : readv(i_fd, iovs, (صحيح)i_iovsLen);
    إذا (ret < 0) {
        أرجع errno_منزل_wasi(errno);
    }

    *o_پورا = (__wasi_size_t)ret;
    أرجع __WASI_ERRNO_SUCCESS;
}

#وإلا

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
ساكن
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_پورا)
{
    *o_پورا = 0;

    size_t پورا = 0;

    لكل (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        فراغ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        إذا (len == 0) {
            استمر;
        }

        ssize_t ret = i_write ? اكتب(i_fd, addr, len) : اقرأ(i_fd, addr, len);
        إذا (ret < 0) {
            أرجع errno_منزل_wasi(errno);
        }

        پورا += (size_t)ret;
        إذا ((size_t)ret < len) {
            اكسر;
        }
    }

    *o_پورا = (__wasi_size_t)پورا;
    أرجع __WASI_ERRNO_SUCCESS;
}

#جے_انت // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
ساكن
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_وتھ, bool i_write,
                            __wasi_size_t* o_پورا)
{
    *o_پورا = 0;

    size_t پورا = 0;

    لكل (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        فراغ*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        إذا (len == 0) {
            استمر;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_وتھ + پورا))
                              : pread(i_fd, addr, len, (off_t)(i_وتھ + پورا));
        إذا (ret < 0) {
            أرجع errno_منزل_wasi(errno);
        }

        پورا += (size_t)ret;
        إذا ((size_t)ret < len) {
            اكسر;
        }
    }

    *o_پورا = (__wasi_size_t)پورا;
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_پورا)
{
    أرجع transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_پورا);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_پورا)
{
    أرجع transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_پورا);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_وتھ,
                                      __wasi_size_t* o_پورا)
{
    أرجع transfer_at(i_fd, i_iovs, i_iovsLen, i_وتھ, false, o_پورا);
}

ساكن
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, ثابت m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_وتھ,
                                       __wasi_size_t* o_پورا)
{
    أرجع transfer_at(i_fd, i_iovs, i_iovsLen, i_وتھ, true, o_پورا);
}


/*
 * Paths
 */

ساكن
__wasi_errno_t m3_wasi_host_رستہ_open (__wasi_fd_t i_dirfd, ccstr_t i_رستہ,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    صحيح dirfd = preopen_fd(i_dirfd);
    إذا (dirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    صحيح flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    إذا ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } وإلا إذا ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } وإلا إذا ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    صحيح host_fd = openat(dirfd, i_رستہ, flags, 0644);
    إذا (host_fd < 0) {
        أرجع errno_منزل_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_filestat (__wasi_fd_t i_dirfd, ccstr_t i_رستہ, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, حجم(*o_stat));

    صحيح dirfd = preopen_fd(i_dirfd);
    إذا (dirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    بنية stat رستہ_stat;
    إذا (fstatat(dirfd, i_رستہ, &رستہ_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        أرجع errno_منزل_wasi(errno);
    }

    filestat_from_stat(&رستہ_stat, o_stat);
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    صحيح dirfd = preopen_fd(i_dirfd);
    إذا (dirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    إذا (mkdirat(dirfd, i_رستہ, 0755) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    صحيح dirfd = preopen_fd(i_dirfd);
    إذا (dirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    إذا (unlinkat(dirfd, i_رستہ, AT_REMOVEDIR) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    صحيح dirfd = preopen_fd(i_dirfd);
    إذا (dirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    إذا (unlinkat(dirfd, i_رستہ, 0) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_old_رستہ,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_new_رستہ)
{
    صحيح oldDirfd = preopen_fd(i_oldDirfd);
    صحيح newDirfd = preopen_fd(i_newDirfd);
    إذا (oldDirfd < 0 || newDirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    إذا (renameat(oldDirfd, i_old_رستہ, newDirfd, i_new_رستہ) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_link (__wasi_fd_t i_oldDirfd, ccstr_t i_old_رستہ, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_new_رستہ)
{
    صحيح oldDirfd = preopen_fd(i_oldDirfd);
    صحيح newDirfd = preopen_fd(i_newDirfd);
    إذا (oldDirfd < 0 || newDirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    إذا (linkat(oldDirfd, i_old_رستہ, newDirfd, i_new_رستہ, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_symlink (ccstr_t i_نشانہ, __wasi_fd_t i_dirfd, ccstr_t i_رستہ)
{
    صحيح dirfd = preopen_fd(i_dirfd);
    إذا (dirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    إذا (symlinkat(i_نشانہ, dirfd, i_رستہ) != 0) {
        أرجع errno_منزل_wasi(errno);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_رستہ_readlink (__wasi_fd_t i_dirfd, ccstr_t i_رستہ, حرف* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_buf_ورتیا)
{
    *o_buf_ورتیا = 0;

    صحيح dirfd = preopen_fd(i_dirfd);
    إذا (dirfd < 0) {
        أرجع __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_رستہ, o_buf, i_bufLen);
    إذا (ret < 0) {
        أرجع errno_منزل_wasi(errno);
    }

    *o_buf_ورتیا = (__wasi_size_t)ret;
    أرجع __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

ساكن
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    إذا (clk == d_m3ClockIdInvalid) {
        أرجع __WASI_ERRNO_INVAL;
    }

    بنية timespec tp;
    إذا (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } وإلا {
        *o_res = convert_timespec(&tp);
    }
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_clock_ویلا_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_ویلا)
{
    *o_ویلا = 0;

    clockid_t clk = convert_clockid(i_clockId);
    إذا (clk == d_m3ClockIdInvalid) {
        أرجع __WASI_ERRNO_INVAL;
    }

    بنية timespec tp;
    إذا (clock_gettime(clk, &tp) != 0) {
        أرجع errno_منزل_wasi(errno);
    }

    *o_ویلا = convert_timespec(&tp);
    أرجع __WASI_ERRNO_SUCCESS;
}

ساكن
__wasi_errno_t m3_wasi_host_random_get (فراغ* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    طالما (i_len) {
        ssize_t retlen;

#إذا defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  إذا defined(__APPLE__) && (نشانہ_OS_IPHONE || نشانہ_IPHONE_SIMULATOR)
        retlen = Sec_Random_Copy_بائٹاں(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  وإلا
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#وإلاإذا defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#وإلا
        أرجع __WASI_ERRNO_NOSYS;
#جے_انت

        إذا (retlen < 0) {
            إذا (errno == EINTR || errno == EAGAIN) {
                استمر;
            }
            أرجع errno_منزل_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    أرجع __WASI_ERRNO_SUCCESS;
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
ساكن
uint64_t poll_بچیا_ns (ثابت m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    إذا (not i_sub->isAbsolute) {
        أرجع (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    إذا (clk == d_m3ClockIdInvalid) {
        أرجع UINT64_MAX;
    }

    بنية timespec tp;
    إذا (clock_gettime(clk, &tp) != 0) {
        أرجع 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    أرجع (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
ساكن
__wasi_filesize_t poll_readable_بائٹاں (صحيح i_fd)
{
#إذا defined(FIONREAD)
    صحيح دستیاب = 0;
    إذا (ioctl(i_fd, FIONREAD, &دستیاب) == 0 and دستیاب > 0) {
        أرجع (__wasi_filesize_t)دستیاب;
    }
#جے_انت
    أرجع 1;
}

ساكن
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

    لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
        إذا (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_بچیا_ns(&i_subs[i], 0);
            إذا (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } وإلا {
            عدد_Fds++;
        }
    }

    بنية pollfd* pollfds = عدم;

    إذا (عدد_Fds) {
        pollfds = (بنية pollfd*)m3_Malloc("WASI poll_oneoff", عدد_Fds * حجم(بنية pollfd));
        إذا (pollfds == عدم) {
            أرجع __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
            إذا (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                استمر;
            }
            pollfds[n].fd = (صحيح)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    صحيح timeoutMs = -1;
    إذا (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)ودھ_توں_ودھ_عدد) ? ودھ_توں_ودھ_عدد : (صحيح)ms;
    }

    صحيح ready;
    إذا (عدد_Fds) {
        افعل {
            ready = poll(pollfds, (nfds_t)عدد_Fds, timeoutMs);
        } طالما (ready < 0 and errno == EINTR);
    } وإلا إذا (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        بنية timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (لما)(soonest % 1000000000);

        طالما (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } وإلا إذا (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } وإلا {
        // no clocks and no descriptors: nothing that could ever become ready
        أرجع __WASI_ERRNO_INVAL;
    }

    إذا (ready < 0) {
        __wasi_errno_t err = errno_منزل_wasi(errno);
        m3_Free(pollfds);
        أرجع err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t باہرلا = 0;
    __wasi_size_t n = 0;

    لكل (__wasi_size_t i = 0; i < i_گنتی; i++) {
        ثابت m3_wasi_pollsub_t* sub = &i_subs[i];

        إذا (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            إذا (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[باہرلا].userdata = sub->userdata;
                o_events[باہرلا].error = __WASI_ERRNO_INVAL;
                o_events[باہرلا].type = __WASI_EVENTTYPE_CLOCK;
                o_events[باہرلا].nbytes = 0;
                o_events[باہرلا].flags = 0;
                باہرلا++;
            } وإلا إذا (poll_بچیا_ns(sub, elapsedNs) == 0) {
                o_events[باہرلا].userdata = sub->userdata;
                o_events[باہرلا].error = __WASI_ERRNO_SUCCESS;
                o_events[باہرلا].type = __WASI_EVENTTYPE_CLOCK;
                o_events[باہرلا].nbytes = 0;
                o_events[باہرلا].flags = 0;
                باہرلا++;
            }
            استمر;
        }

        چھوٹا revents = pollfds[n].revents;
        n++;

        إذا (revents == 0) {
            استمر;
        }

        o_events[باہرلا].userdata = sub->userdata;
        o_events[باہرلا].type = sub->type;
        o_events[باہرلا].nbytes = 0;
        o_events[باہرلا].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        إذا (revents & POLLNVAL) {
            o_events[باہرلا].error = __WASI_ERRNO_BADF;
        } وإلا إذا (revents & POLLERR) {
            o_events[باہرلا].error = __WASI_ERRNO_IO;
        } وإلا {
            o_events[باہرلا].error = __WASI_ERRNO_SUCCESS;
            إذا (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[باہرلا].nbytes = poll_readable_بائٹاں(pollfds[n - 1].fd);
            }
        }

        باہرلا++;
    }

    m3_Free(pollfds);

    *o_event_گنتی = باہرلا;
    أرجع __WASI_ERRNO_SUCCESS;
}

#جے_انت // m3_api_wasi_posix_h
