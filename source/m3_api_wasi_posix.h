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

#அந்தர்பாவ "m3_core.h"

#எனில் defined(_WIN32)
#  error "This file is not for Windows"
#endif

#அந்தர்பாவ <sys/types.h>
#அந்தர்பாவ <sys/stat.h>
#அந்தர்பாவ <errno.h>
#அந்தர்பாவ <fcntl.h>
#அந்தர்பாவ <limits.h>
#அந்தர்பாவ <poll.h>
#அந்தர்பாவ <string.h>
#அந்தர்பாவ <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#எனில் !defined(__wasi__)
#  include <sys/ioctl.h>
#endif

#எனில் defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  எனில் defined(__APPLE__)
#    include <TargetConditionals.h>
#    எனில் TARGET_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    இல்லையெனில் // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  இல்லையெனில்
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#endif

#அந்தர்பாவ "m3_api_wasi_host.h"


/*
 * Plumbing
 */

நிலையான
__wasi_errno_t errno_to_wasi (முழு errnum)
{
    // clang-format off
    தேர்வு (errnum) {
    நிலை EPERM:    திரும்பு __WASI_ERRNO_PERM;
    நிலை ENOENT:   திரும்பு __WASI_ERRNO_NOENT;
    நிலை ESRCH:    திரும்பு __WASI_ERRNO_SRCH;
    நிலை EINTR:    திரும்பு __WASI_ERRNO_INTR;
    நிலை EIO:      திரும்பு __WASI_ERRNO_IO;
    நிலை ENXIO:    திரும்பு __WASI_ERRNO_NXIO;
    நிலை E2BIG:    திரும்பு __WASI_ERRNO_2BIG;
    நிலை ENOEXEC:  திரும்பு __WASI_ERRNO_NOEXEC;
    நிலை EBADF:    திரும்பு __WASI_ERRNO_BADF;
    நிலை ECHILD:   திரும்பு __WASI_ERRNO_CHILD;
    நிலை EAGAIN:   திரும்பு __WASI_ERRNO_AGAIN;
    நிலை ENOMEM:   திரும்பு __WASI_ERRNO_NOMEM;
    நிலை EACCES:   திரும்பு __WASI_ERRNO_ACCES;
    நிலை EFAULT:   திரும்பு __WASI_ERRNO_FAULT;
    நிலை EBUSY:    திரும்பு __WASI_ERRNO_BUSY;
    நிலை EEXIST:   திரும்பு __WASI_ERRNO_EXIST;
    நிலை EXDEV:    திரும்பு __WASI_ERRNO_XDEV;
    நிலை ENODEV:   திரும்பு __WASI_ERRNO_NODEV;
    நிலை ENOTDIR:  திரும்பு __WASI_ERRNO_NOTDIR;
    நிலை EISDIR:   திரும்பு __WASI_ERRNO_ISDIR;
    நிலை EINVAL:   திரும்பு __WASI_ERRNO_INVAL;
    நிலை ENFILE:   திரும்பு __WASI_ERRNO_NFILE;
    நிலை EMFILE:   திரும்பு __WASI_ERRNO_MFILE;
    நிலை ENOTTY:   திரும்பு __WASI_ERRNO_NOTTY;
    நிலை ETXTBSY:  திரும்பு __WASI_ERRNO_TXTBSY;
    நிலை EFBIG:    திரும்பு __WASI_ERRNO_FBIG;
    நிலை ENOSPC:   திரும்பு __WASI_ERRNO_NOSPC;
    நிலை ESPIPE:   திரும்பு __WASI_ERRNO_SPIPE;
    நிலை EROFS:    திரும்பு __WASI_ERRNO_ROFS;
    நிலை EMLINK:   திரும்பு __WASI_ERRNO_MLINK;
    நிலை EPIPE:    திரும்பு __WASI_ERRNO_PIPE;
    நிலை EDOM:     திரும்பு __WASI_ERRNO_DOM;
    நிலை ERANGE:   திரும்பு __WASI_ERRNO_RANGE;
    }
    // clang-format on
    திரும்பு __WASI_ERRNO_INVAL;
}

நிலையான
__wasi_filetype_t filetype_from_stat_mode (முழு mode)
{
    திரும்பு (S_ISBLK(mode) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(mode) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(mode) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(mode) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(mode) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

நிலையான
வெறுமை filestat_from_stat (மாறிலி கட்டமைப்பு stat* i_stat, m3_wasi_filestat_t* o_stat)
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
நிலையான
முழு preopen_fd (__wasi_fd_t i_fd)
{
    எனில் (i_fd >= d_m3_Wasi_Preopen_மொத்தம்) {
        திரும்பு -1;
    }
    திரும்பு m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#எனில் defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) வெற்று)
#இல்லையெனில்
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#endif

நிலையான
clockid_t convert_clockid (__wasi_clockid_t in)
{
    தேர்வு (in) {
    நிலை __WASI_CLOCKID_MONOTONIC: திரும்பு CLOCK_MONOTONIC;
    நிலை __WASI_CLOCKID_REALTIME: திரும்பு CLOCK_REALTIME;
#எனில் defined(CLOCK_PROCESS_CPUTIME_ID)
    நிலை __WASI_CLOCKID_PROCESS_CPUTIME_ID: திரும்பு CLOCK_PROCESS_CPUTIME_ID;
#endif
#எனில் defined(CLOCK_THREAD_CPUTIME_ID)
    நிலை __WASI_CLOCKID_THREAD_CPUTIME_ID: திரும்பு CLOCK_THREAD_CPUTIME_ID;
#endif
    இயல்பு: திரும்பு d_m3ClockIdInvalid;
    }
}

நிலையான
__wasi_timestamp_t convert_timespec (மாறிலி கட்டமைப்பு timespec* ts)
{
    எனில் (ts->tv_sec < 0) {
        திரும்பு 0;
    }
    எனில் ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        திரும்பு UINT64_MAX;
    }
    திரும்பு (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

நிலையான
வெறுமை m3_wasi_host_init ()
{
    க்கு (முழு i = 3; i < d_m3_Wasi_Preopen_மொத்தம்; i++) {
        எனில் (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_path, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

நிலையான
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    எனில் (close(i_fd) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#எனில் defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#அல்லதெனில் defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#endif

நிலையான
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#எனில் defined(d_m3WasiDatasync)
    எனில் (d_m3WasiDatasync(i_fd) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
#இல்லையெனில்
    திரும்பு __WASI_ERRNO_NOSYS;
#endif
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#எனில் defined(POSIX_FADV_NORMAL)
    முழு adv;
    தேர்வு (i_advice) {
    நிலை __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; நிறுத்து;
    நிலை __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; நிறுத்து;
    நிலை __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; நிறுத்து;
    நிலை __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; நிறுத்து;
    நிலை __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; நிறுத்து;
    நிலை __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; நிறுத்து;
    இயல்பு: திரும்பு __WASI_ERRNO_INVAL;
    }

    முழு ret = posix_fadvise(i_fd, i_offset, i_length, adv);
    எனில் (ret != 0) {
        திரும்பு errno_to_wasi(ret);
    }
#இல்லையெனில்
    எனில் (i_advice > __WASI_ADVICE_NOREUSE) {
        திரும்பு __WASI_ERRNO_INVAL;
    }

    கட்டமைப்பு stat fd_stat;
    எனில் (fstat(i_fd, &fd_stat) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
#endif

    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    முழு fl = fcntl(i_fd, F_GETFL);
    எனில் (fl < 0) {
        திரும்பு errno_to_wasi(errno);
    }

    கட்டமைப்பு stat fd_stat;
    எனில் (fstat(i_fd, &fd_stat) != 0) {
        திரும்பு errno_to_wasi(errno);
    }

    *o_filetype = filetype_from_stat_mode(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    திரும்பு __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
நிலையான
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    முழு fl = fcntl(i_fd, F_GETFL);
    எனில் (fl < 0) {
        திரும்பு errno_to_wasi(errno);
    }

    மாறிலி __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    எனில் ((i_flags & fixed) != (current & fixed)) {
        திரும்பு __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    எனில் (fcntl(i_fd, F_SETFL, fl) < 0) {
        திரும்பு errno_to_wasi(errno);
    }

    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, அளவு(*o_stat));

    கட்டமைப்பு stat fd_stat;
    எனில் (fstat(i_fd, &fd_stat) != 0) {
        திரும்பு errno_to_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    எனில் (ftruncate(i_fd, (off_t)i_size) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    கட்டமைப்பு timespec times[2];

    எனில் (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } இல்லையெனில் எனில் (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } இல்லையெனில் {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    எனில் (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } இல்லையெனில் எனில் (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } இல்லையெனில் {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    எனில் (futimens(i_fd, times) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, முழு i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_offset, i_whence);
    எனில் (ret < 0) {
        திரும்பு errno_to_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    திரும்பு __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#எனில் defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
நிலையான
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, மாறிலி m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    கட்டமைப்பு iovec iovs[d_m3WasiMaxIovs];
    க்கு (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (முழு)i_iovsLen) : readv(i_fd, iovs, (முழு)i_iovsLen);
    எனில் (ret < 0) {
        திரும்பு errno_to_wasi(errno);
    }

    *o_done = (__wasi_size_t)ret;
    திரும்பு __WASI_ERRNO_SUCCESS;
}

#இல்லையெனில்

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
நிலையான
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, மாறிலி m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    க்கு (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        வெறுமை*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        எனில் (len == 0) {
            தொடர்;
        }

        ssize_t ret = i_write ? லிக(i_fd, addr, len) : பட(i_fd, addr, len);
        எனில் (ret < 0) {
            திரும்பு errno_to_wasi(errno);
        }

        done += (size_t)ret;
        எனில் ((size_t)ret < len) {
            நிறுத்து;
        }
    }

    *o_done = (__wasi_size_t)done;
    திரும்பு __WASI_ERRNO_SUCCESS;
}

#endif // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
நிலையான
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, மாறிலி m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset, bool i_write,
                            __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    க்கு (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        வெறுமை*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        எனில் (len == 0) {
            தொடர்;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_offset + done))
                              : pread(i_fd, addr, len, (off_t)(i_offset + done));
        எனில் (ret < 0) {
            திரும்பு errno_to_wasi(errno);
        }

        done += (size_t)ret;
        எனில் ((size_t)ret < len) {
            நிறுத்து;
        }
    }

    *o_done = (__wasi_size_t)done;
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, மாறிலி m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    திரும்பு transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_done);
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, மாறிலி m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    திரும்பு transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_done);
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, மாறிலி m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    திரும்பு transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, false, o_done);
}

நிலையான
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, மாறிலி m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    திரும்பு transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, true, o_done);
}


/*
 * Paths
 */

நிலையான
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    முழு dirfd = preopen_fd(i_dirfd);
    எனில் (dirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    முழு flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    எனில் ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } இல்லையெனில் எனில் ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } இல்லையெனில் எனில் ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    முழு host_fd = openat(dirfd, i_path, flags, 0644);
    எனில் (host_fd < 0) {
        திரும்பு errno_to_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, அளவு(*o_stat));

    முழு dirfd = preopen_fd(i_dirfd);
    எனில் (dirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    கட்டமைப்பு stat path_stat;
    எனில் (fstatat(dirfd, i_path, &path_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        திரும்பு errno_to_wasi(errno);
    }

    filestat_from_stat(&path_stat, o_stat);
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    முழு dirfd = preopen_fd(i_dirfd);
    எனில் (dirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    எனில் (mkdirat(dirfd, i_path, 0755) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    முழு dirfd = preopen_fd(i_dirfd);
    எனில் (dirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    எனில் (unlinkat(dirfd, i_path, AT_REMOVEDIR) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    முழு dirfd = preopen_fd(i_dirfd);
    எனில் (dirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    எனில் (unlinkat(dirfd, i_path, 0) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    முழு oldDirfd = preopen_fd(i_oldDirfd);
    முழு newDirfd = preopen_fd(i_newDirfd);
    எனில் (oldDirfd < 0 || newDirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    எனில் (renameat(oldDirfd, i_oldPath, newDirfd, i_newPath) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    முழு oldDirfd = preopen_fd(i_oldDirfd);
    முழு newDirfd = preopen_fd(i_newDirfd);
    எனில் (oldDirfd < 0 || newDirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    எனில் (linkat(oldDirfd, i_oldPath, newDirfd, i_newPath, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    முழு dirfd = preopen_fd(i_dirfd);
    எனில் (dirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    எனில் (symlinkat(i_target, dirfd, i_path) != 0) {
        திரும்பு errno_to_wasi(errno);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, எழுத்து* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    முழு dirfd = preopen_fd(i_dirfd);
    எனில் (dirfd < 0) {
        திரும்பு __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_path, o_buf, i_bufLen);
    எனில் (ret < 0) {
        திரும்பு errno_to_wasi(errno);
    }

    *o_bufUsed = (__wasi_size_t)ret;
    திரும்பு __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

நிலையான
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    எனில் (clk == d_m3ClockIdInvalid) {
        திரும்பு __WASI_ERRNO_INVAL;
    }

    கட்டமைப்பு timespec tp;
    எனில் (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } இல்லையெனில் {
        *o_res = convert_timespec(&tp);
    }
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = 0;

    clockid_t clk = convert_clockid(i_clockId);
    எனில் (clk == d_m3ClockIdInvalid) {
        திரும்பு __WASI_ERRNO_INVAL;
    }

    கட்டமைப்பு timespec tp;
    எனில் (clock_gettime(clk, &tp) != 0) {
        திரும்பு errno_to_wasi(errno);
    }

    *o_time = convert_timespec(&tp);
    திரும்பு __WASI_ERRNO_SUCCESS;
}

நிலையான
__wasi_errno_t m3_wasi_host_random_get (வெறுமை* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    போது (i_len) {
        ssize_t retlen;

#எனில் defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  எனில் defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
        retlen = SecRandomCopyBytes(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  இல்லையெனில்
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#அல்லதெனில் defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#இல்லையெனில்
        திரும்பு __WASI_ERRNO_NOSYS;
#endif

        எனில் (retlen < 0) {
            எனில் (errno == EINTR || errno == EAGAIN) {
                தொடர்;
            }
            திரும்பு errno_to_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    திரும்பு __WASI_ERRNO_SUCCESS;
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
நிலையான
uint64_t poll_remaining_ns (மாறிலி m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    எனில் (not i_sub->isAbsolute) {
        திரும்பு (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    எனில் (clk == d_m3ClockIdInvalid) {
        திரும்பு UINT64_MAX;
    }

    கட்டமைப்பு timespec tp;
    எனில் (clock_gettime(clk, &tp) != 0) {
        திரும்பு 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    திரும்பு (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
நிலையான
__wasi_filesize_t poll_readable_bytes (முழு i_fd)
{
#எனில் defined(FIONREAD)
    முழு available = 0;
    எனில் (ioctl(i_fd, FIONREAD, &available) == 0 and available > 0) {
        திரும்பு (__wasi_filesize_t)available;
    }
#endif
    திரும்பு 1;
}

நிலையான
__wasi_errno_t m3_wasi_host_poll_oneoff (மாறிலி m3_wasi_pollsub_t* i_subs, __wasi_size_t i_மொத்தம்,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_மொத்தம்)
{
    *o_event_மொத்தம் = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t இலக்கம்_Fds = 0;

    க்கு (__wasi_size_t i = 0; i < i_மொத்தம்; i++) {
        எனில் (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_remaining_ns(&i_subs[i], 0);
            எனில் (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } இல்லையெனில் {
            இலக்கம்_Fds++;
        }
    }

    கட்டமைப்பு pollfd* pollfds = வெற்று;

    எனில் (இலக்கம்_Fds) {
        pollfds = (கட்டமைப்பு pollfd*)m3_Malloc("WASI poll_oneoff", இலக்கம்_Fds * அளவு(கட்டமைப்பு pollfd));
        எனில் (pollfds == வெற்று) {
            திரும்பு __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        க்கு (__wasi_size_t i = 0; i < i_மொத்தம்; i++) {
            எனில் (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                தொடர்;
            }
            pollfds[n].fd = (முழு)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    முழு timeoutMs = -1;
    எனில் (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)INT_MAX) ? INT_MAX : (முழு)ms;
    }

    முழு ready;
    எனில் (இலக்கம்_Fds) {
        செய் {
            ready = poll(pollfds, (nfds_t)இலக்கம்_Fds, timeoutMs);
        } போது (ready < 0 and errno == EINTR);
    } இல்லையெனில் எனில் (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        கட்டமைப்பு timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (long)(soonest % 1000000000);

        போது (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } இல்லையெனில் எனில் (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } இல்லையெனில் {
        // no clocks and no descriptors: nothing that could ever become ready
        திரும்பு __WASI_ERRNO_INVAL;
    }

    எனில் (ready < 0) {
        __wasi_errno_t err = errno_to_wasi(errno);
        m3_Free(pollfds);
        திரும்பு err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t out = 0;
    __wasi_size_t n = 0;

    க்கு (__wasi_size_t i = 0; i < i_மொத்தம்; i++) {
        மாறிலி m3_wasi_pollsub_t* sub = &i_subs[i];

        எனில் (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            எனில் (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_INVAL;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            } இல்லையெனில் எனில் (poll_remaining_ns(sub, elapsedNs) == 0) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_SUCCESS;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            }
            தொடர்;
        }

        short revents = pollfds[n].revents;
        n++;

        எனில் (revents == 0) {
            தொடர்;
        }

        o_events[out].userdata = sub->userdata;
        o_events[out].type = sub->type;
        o_events[out].nbytes = 0;
        o_events[out].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        எனில் (revents & POLLNVAL) {
            o_events[out].error = __WASI_ERRNO_BADF;
        } இல்லையெனில் எனில் (revents & POLLERR) {
            o_events[out].error = __WASI_ERRNO_IO;
        } இல்லையெனில் {
            o_events[out].error = __WASI_ERRNO_SUCCESS;
            எனில் (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[out].nbytes = poll_readable_bytes(pollfds[n - 1].fd);
            }
        }

        out++;
    }

    m3_Free(pollfds);

    *o_event_மொத்தம் = out;
    திரும்பு __WASI_ERRNO_SUCCESS;
}

#endif // m3_api_wasi_posix_h
