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

#അന്തര്ഭാവ "m3_core.h"

#എങ്കിൽ defined(_WIN32)
#  error "This file is not for Windows"
#endif

#അന്തര്ഭാവ <sys/types.h>
#അന്തര്ഭാവ <sys/stat.h>
#അന്തര്ഭാവ <errno.h>
#അന്തര്ഭാവ <fcntl.h>
#അന്തര്ഭാവ <limits.h>
#അന്തര്ഭാവ <poll.h>
#അന്തര്ഭാവ <string.h>
#അന്തര്ഭാവ <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#എങ്കിൽ !defined(__wasi__)
#  include <sys/ioctl.h>
#endif

#എങ്കിൽ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  എങ്കിൽ defined(__APPLE__)
#    include <TargetConditionals.h>
#    എങ്കിൽ TARGET_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    അല്ലെങ്കിൽ // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  അല്ലെങ്കിൽ
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#endif

#അന്തര്ഭാവ "m3_api_wasi_host.h"


/*
 * Plumbing
 */

സ്ഥിതം
__wasi_errno_t errno_to_wasi (പൂർണ്ണസംഖ്യ errnum)
{
    // clang-format off
    തിരഞ്ഞെടുപ്പ് (errnum) {
    അവസ്ഥ EPERM:    തിരികെ __WASI_ERRNO_PERM;
    അവസ്ഥ ENOENT:   തിരികെ __WASI_ERRNO_NOENT;
    അവസ്ഥ ESRCH:    തിരികെ __WASI_ERRNO_SRCH;
    അവസ്ഥ EINTR:    തിരികെ __WASI_ERRNO_INTR;
    അവസ്ഥ EIO:      തിരികെ __WASI_ERRNO_IO;
    അവസ്ഥ ENXIO:    തിരികെ __WASI_ERRNO_NXIO;
    അവസ്ഥ E2BIG:    തിരികെ __WASI_ERRNO_2BIG;
    അവസ്ഥ ENOEXEC:  തിരികെ __WASI_ERRNO_NOEXEC;
    അവസ്ഥ EBADF:    തിരികെ __WASI_ERRNO_BADF;
    അവസ്ഥ ECHILD:   തിരികെ __WASI_ERRNO_CHILD;
    അവസ്ഥ EAGAIN:   തിരികെ __WASI_ERRNO_AGAIN;
    അവസ്ഥ ENOMEM:   തിരികെ __WASI_ERRNO_NOMEM;
    അവസ്ഥ EACCES:   തിരികെ __WASI_ERRNO_ACCES;
    അവസ്ഥ EFAULT:   തിരികെ __WASI_ERRNO_FAULT;
    അവസ്ഥ EBUSY:    തിരികെ __WASI_ERRNO_BUSY;
    അവസ്ഥ EEXIST:   തിരികെ __WASI_ERRNO_EXIST;
    അവസ്ഥ EXDEV:    തിരികെ __WASI_ERRNO_XDEV;
    അവസ്ഥ ENODEV:   തിരികെ __WASI_ERRNO_NODEV;
    അവസ്ഥ ENOTDIR:  തിരികെ __WASI_ERRNO_NOTDIR;
    അവസ്ഥ EISDIR:   തിരികെ __WASI_ERRNO_ISDIR;
    അവസ്ഥ EINVAL:   തിരികെ __WASI_ERRNO_INVAL;
    അവസ്ഥ ENFILE:   തിരികെ __WASI_ERRNO_NFILE;
    അവസ്ഥ EMFILE:   തിരികെ __WASI_ERRNO_MFILE;
    അവസ്ഥ ENOTTY:   തിരികെ __WASI_ERRNO_NOTTY;
    അവസ്ഥ ETXTBSY:  തിരികെ __WASI_ERRNO_TXTBSY;
    അവസ്ഥ EFBIG:    തിരികെ __WASI_ERRNO_FBIG;
    അവസ്ഥ ENOSPC:   തിരികെ __WASI_ERRNO_NOSPC;
    അവസ്ഥ ESPIPE:   തിരികെ __WASI_ERRNO_SPIPE;
    അവസ്ഥ EROFS:    തിരികെ __WASI_ERRNO_ROFS;
    അവസ്ഥ EMLINK:   തിരികെ __WASI_ERRNO_MLINK;
    അവസ്ഥ EPIPE:    തിരികെ __WASI_ERRNO_PIPE;
    അവസ്ഥ EDOM:     തിരികെ __WASI_ERRNO_DOM;
    അവസ്ഥ ERANGE:   തിരികെ __WASI_ERRNO_RANGE;
    }
    // clang-format on
    തിരികെ __WASI_ERRNO_INVAL;
}

സ്ഥിതം
__wasi_filetype_t filetype_from_stat_mode (പൂർണ്ണസംഖ്യ mode)
{
    തിരികെ (S_ISBLK(mode) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(mode) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(mode) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(mode) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(mode) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

സ്ഥിതം
ശൂന്യം filestat_from_stat (സ്ഥിരം ഘടന stat* i_stat, m3_wasi_filestat_t* o_stat)
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
സ്ഥിതം
പൂർണ്ണസംഖ്യ preopen_fd (__wasi_fd_t i_fd)
{
    എങ്കിൽ (i_fd >= d_m3_Wasi_Preopen_എണ്ണം) {
        തിരികെ -1;
    }
    തിരികെ m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#എങ്കിൽ defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) ശൂന്യ)
#അല്ലെങ്കിൽ
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#endif

സ്ഥിതം
clockid_t convert_clockid (__wasi_clockid_t in)
{
    തിരഞ്ഞെടുപ്പ് (in) {
    അവസ്ഥ __WASI_CLOCKID_MONOTONIC: തിരികെ CLOCK_MONOTONIC;
    അവസ്ഥ __WASI_CLOCKID_REALTIME: തിരികെ CLOCK_REALTIME;
#എങ്കിൽ defined(CLOCK_PROCESS_CPUTIME_ID)
    അവസ്ഥ __WASI_CLOCKID_PROCESS_CPUTIME_ID: തിരികെ CLOCK_PROCESS_CPUTIME_ID;
#endif
#എങ്കിൽ defined(CLOCK_THREAD_CPUTIME_ID)
    അവസ്ഥ __WASI_CLOCKID_THREAD_CPUTIME_ID: തിരികെ CLOCK_THREAD_CPUTIME_ID;
#endif
    സ്ഥിരസ്ഥിതി: തിരികെ d_m3ClockIdInvalid;
    }
}

സ്ഥിതം
__wasi_timestamp_t convert_timespec (സ്ഥിരം ഘടന timespec* ts)
{
    എങ്കിൽ (ts->tv_sec < 0) {
        തിരികെ 0;
    }
    എങ്കിൽ ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        തിരികെ UINT64_MAX;
    }
    തിരികെ (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

സ്ഥിതം
ശൂന്യം m3_wasi_host_init ()
{
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 3; i < d_m3_Wasi_Preopen_എണ്ണം; i++) {
        എങ്കിൽ (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = open(m3_wasi_preopen[i].real_path, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    എങ്കിൽ (close(i_fd) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#എങ്കിൽ defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#അല്ലെങ്കിലെങ്കിൽ defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#endif

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#എങ്കിൽ defined(d_m3WasiDatasync)
    എങ്കിൽ (d_m3WasiDatasync(i_fd) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
#അല്ലെങ്കിൽ
    തിരികെ __WASI_ERRNO_NOSYS;
#endif
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#എങ്കിൽ defined(POSIX_FADV_NORMAL)
    പൂർണ്ണസംഖ്യ adv;
    തിരഞ്ഞെടുപ്പ് (i_advice) {
    അവസ്ഥ __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; നിർത്തുക;
    അവസ്ഥ __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; നിർത്തുക;
    അവസ്ഥ __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; നിർത്തുക;
    അവസ്ഥ __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; നിർത്തുക;
    അവസ്ഥ __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; നിർത്തുക;
    അവസ്ഥ __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; നിർത്തുക;
    സ്ഥിരസ്ഥിതി: തിരികെ __WASI_ERRNO_INVAL;
    }

    പൂർണ്ണസംഖ്യ ret = posix_fadvise(i_fd, i_offset, i_length, adv);
    എങ്കിൽ (ret != 0) {
        തിരികെ errno_to_wasi(ret);
    }
#അല്ലെങ്കിൽ
    എങ്കിൽ (i_advice > __WASI_ADVICE_NOREUSE) {
        തിരികെ __WASI_ERRNO_INVAL;
    }

    ഘടന stat fd_stat;
    എങ്കിൽ (fstat(i_fd, &fd_stat) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
#endif

    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    പൂർണ്ണസംഖ്യ fl = fcntl(i_fd, F_GETFL);
    എങ്കിൽ (fl < 0) {
        തിരികെ errno_to_wasi(errno);
    }

    ഘടന stat fd_stat;
    എങ്കിൽ (fstat(i_fd, &fd_stat) != 0) {
        തിരികെ errno_to_wasi(errno);
    }

    *o_filetype = filetype_from_stat_mode(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    തിരികെ __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    പൂർണ്ണസംഖ്യ fl = fcntl(i_fd, F_GETFL);
    എങ്കിൽ (fl < 0) {
        തിരികെ errno_to_wasi(errno);
    }

    സ്ഥിരം __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    എങ്കിൽ ((i_flags & fixed) != (current & fixed)) {
        തിരികെ __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    എങ്കിൽ (fcntl(i_fd, F_SETFL, fl) < 0) {
        തിരികെ errno_to_wasi(errno);
    }

    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, വലുപ്പം(*o_stat));

    ഘടന stat fd_stat;
    എങ്കിൽ (fstat(i_fd, &fd_stat) != 0) {
        തിരികെ errno_to_wasi(errno);
    }

    filestat_from_stat(&fd_stat, o_stat);
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    എങ്കിൽ (ftruncate(i_fd, (off_t)i_size) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    ഘടന timespec times[2];

    എങ്കിൽ (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_NOW;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_flags & __WASI_FSTFLAGS_ATIM) {
        times[0].tv_sec = i_atim / 1000000000ULL;
        times[0].tv_nsec = i_atim % 1000000000ULL;
    } അല്ലെങ്കിൽ {
        times[0].tv_sec = 0;
        times[0].tv_nsec = UTIME_OMIT;
    }

    എങ്കിൽ (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_NOW;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_flags & __WASI_FSTFLAGS_MTIM) {
        times[1].tv_sec = i_mtim / 1000000000ULL;
        times[1].tv_nsec = i_mtim % 1000000000ULL;
    } അല്ലെങ്കിൽ {
        times[1].tv_sec = 0;
        times[1].tv_nsec = UTIME_OMIT;
    }

    എങ്കിൽ (futimens(i_fd, times) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, പൂർണ്ണസംഖ്യ i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_offset, i_whence);
    എങ്കിൽ (ret < 0) {
        തിരികെ errno_to_wasi(errno);
    }

    *o_pos = (__wasi_filesize_t)ret;
    തിരികെ __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#എങ്കിൽ defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
സ്ഥിതം
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, സ്ഥിരം m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    ഘടന iovec iovs[d_m3WasiMaxIovs];
    വേണ്ടി (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (പൂർണ്ണസംഖ്യ)i_iovsLen) : readv(i_fd, iovs, (പൂർണ്ണസംഖ്യ)i_iovsLen);
    എങ്കിൽ (ret < 0) {
        തിരികെ errno_to_wasi(errno);
    }

    *o_done = (__wasi_size_t)ret;
    തിരികെ __WASI_ERRNO_SUCCESS;
}

#അല്ലെങ്കിൽ

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
സ്ഥിതം
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, സ്ഥിരം m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    വേണ്ടി (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ശൂന്യം*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        എങ്കിൽ (len == 0) {
            തുടരുക;
        }

        ssize_t ret = i_write ? ലിഖ(i_fd, addr, len) : പഠ(i_fd, addr, len);
        എങ്കിൽ (ret < 0) {
            തിരികെ errno_to_wasi(errno);
        }

        done += (size_t)ret;
        എങ്കിൽ ((size_t)ret < len) {
            നിർത്തുക;
        }
    }

    *o_done = (__wasi_size_t)done;
    തിരികെ __WASI_ERRNO_SUCCESS;
}

#endif // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
സ്ഥിതം
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, സ്ഥിരം m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset, bool i_write,
                            __wasi_size_t* o_done)
{
    *o_done = 0;

    size_t done = 0;

    വേണ്ടി (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        ശൂന്യം*  addr = i_iovs[i].buf;
        size_t len = i_iovs[i].len;
        എങ്കിൽ (len == 0) {
            തുടരുക;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_offset + done))
                              : pread(i_fd, addr, len, (off_t)(i_offset + done));
        എങ്കിൽ (ret < 0) {
            തിരികെ errno_to_wasi(errno);
        }

        done += (size_t)ret;
        എങ്കിൽ ((size_t)ret < len) {
            നിർത്തുക;
        }
    }

    *o_done = (__wasi_size_t)done;
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, സ്ഥിരം m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    തിരികെ transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_done);
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, സ്ഥിരം m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    തിരികെ transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_done);
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, സ്ഥിരം m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    തിരികെ transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, false, o_done);
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, സ്ഥിരം m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    തിരികെ transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, true, o_done);
}


/*
 * Paths
 */

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    പൂർണ്ണസംഖ്യ dirfd = preopen_fd(i_dirfd);
    എങ്കിൽ (dirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    പൂർണ്ണസംഖ്യ flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    എങ്കിൽ ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } അല്ലെങ്കിൽ എങ്കിൽ ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } അല്ലെങ്കിൽ എങ്കിൽ ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    പൂർണ്ണസംഖ്യ host_fd = openat(dirfd, i_path, flags, 0644);
    എങ്കിൽ (host_fd < 0) {
        തിരികെ errno_to_wasi(errno);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    memset(o_stat, 0, വലുപ്പം(*o_stat));

    പൂർണ്ണസംഖ്യ dirfd = preopen_fd(i_dirfd);
    എങ്കിൽ (dirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    ഘടന stat path_stat;
    എങ്കിൽ (fstatat(dirfd, i_path, &path_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        തിരികെ errno_to_wasi(errno);
    }

    filestat_from_stat(&path_stat, o_stat);
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    പൂർണ്ണസംഖ്യ dirfd = preopen_fd(i_dirfd);
    എങ്കിൽ (dirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    എങ്കിൽ (mkdirat(dirfd, i_path, 0755) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    പൂർണ്ണസംഖ്യ dirfd = preopen_fd(i_dirfd);
    എങ്കിൽ (dirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    എങ്കിൽ (unlinkat(dirfd, i_path, AT_REMOVEDIR) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    പൂർണ്ണസംഖ്യ dirfd = preopen_fd(i_dirfd);
    എങ്കിൽ (dirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    എങ്കിൽ (unlinkat(dirfd, i_path, 0) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    പൂർണ്ണസംഖ്യ oldDirfd = preopen_fd(i_oldDirfd);
    പൂർണ്ണസംഖ്യ newDirfd = preopen_fd(i_newDirfd);
    എങ്കിൽ (oldDirfd < 0 || newDirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    എങ്കിൽ (renameat(oldDirfd, i_oldPath, newDirfd, i_newPath) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    പൂർണ്ണസംഖ്യ oldDirfd = preopen_fd(i_oldDirfd);
    പൂർണ്ണസംഖ്യ newDirfd = preopen_fd(i_newDirfd);
    എങ്കിൽ (oldDirfd < 0 || newDirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    എങ്കിൽ (linkat(oldDirfd, i_oldPath, newDirfd, i_newPath, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    പൂർണ്ണസംഖ്യ dirfd = preopen_fd(i_dirfd);
    എങ്കിൽ (dirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    എങ്കിൽ (symlinkat(i_target, dirfd, i_path) != 0) {
        തിരികെ errno_to_wasi(errno);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, അക്ഷരം* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    പൂർണ്ണസംഖ്യ dirfd = preopen_fd(i_dirfd);
    എങ്കിൽ (dirfd < 0) {
        തിരികെ __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_path, o_buf, i_bufLen);
    എങ്കിൽ (ret < 0) {
        തിരികെ errno_to_wasi(errno);
    }

    *o_bufUsed = (__wasi_size_t)ret;
    തിരികെ __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

സ്ഥിതം
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    എങ്കിൽ (clk == d_m3ClockIdInvalid) {
        തിരികെ __WASI_ERRNO_INVAL;
    }

    ഘടന timespec tp;
    എങ്കിൽ (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } അല്ലെങ്കിൽ {
        *o_res = convert_timespec(&tp);
    }
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = 0;

    clockid_t clk = convert_clockid(i_clockId);
    എങ്കിൽ (clk == d_m3ClockIdInvalid) {
        തിരികെ __WASI_ERRNO_INVAL;
    }

    ഘടന timespec tp;
    എങ്കിൽ (clock_gettime(clk, &tp) != 0) {
        തിരികെ errno_to_wasi(errno);
    }

    *o_time = convert_timespec(&tp);
    തിരികെ __WASI_ERRNO_SUCCESS;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_random_get (ശൂന്യം* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    ആകുമ്പോൾ (i_len) {
        ssize_t retlen;

#എങ്കിൽ defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        size_t reqlen = M3_MIN(i_len, 256);
#  എങ്കിൽ defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
        retlen = SecRandomCopyBytes(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  അല്ലെങ്കിൽ
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#അല്ലെങ്കിലെങ്കിൽ defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#അല്ലെങ്കിൽ
        തിരികെ __WASI_ERRNO_NOSYS;
#endif

        എങ്കിൽ (retlen < 0) {
            എങ്കിൽ (errno == EINTR || errno == EAGAIN) {
                തുടരുക;
            }
            തിരികെ errno_to_wasi(errno);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    തിരികെ __WASI_ERRNO_SUCCESS;
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
സ്ഥിതം
uint64_t poll_remaining_ns (സ്ഥിരം m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    എങ്കിൽ (not i_sub->isAbsolute) {
        തിരികെ (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    എങ്കിൽ (clk == d_m3ClockIdInvalid) {
        തിരികെ UINT64_MAX;
    }

    ഘടന timespec tp;
    എങ്കിൽ (clock_gettime(clk, &tp) != 0) {
        തിരികെ 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    തിരികെ (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
സ്ഥിതം
__wasi_filesize_t poll_readable_bytes (പൂർണ്ണസംഖ്യ i_fd)
{
#എങ്കിൽ defined(FIONREAD)
    പൂർണ്ണസംഖ്യ available = 0;
    എങ്കിൽ (ioctl(i_fd, FIONREAD, &available) == 0 and available > 0) {
        തിരികെ (__wasi_filesize_t)available;
    }
#endif
    തിരികെ 1;
}

സ്ഥിതം
__wasi_errno_t m3_wasi_host_poll_oneoff (സ്ഥിരം m3_wasi_pollsub_t* i_subs, __wasi_size_t i_എണ്ണം,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_എണ്ണം)
{
    *o_event_എണ്ണം = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t അക്കം_Fds = 0;

    വേണ്ടി (__wasi_size_t i = 0; i < i_എണ്ണം; i++) {
        എങ്കിൽ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_remaining_ns(&i_subs[i], 0);
            എങ്കിൽ (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } അല്ലെങ്കിൽ {
            അക്കം_Fds++;
        }
    }

    ഘടന pollfd* pollfds = ശൂന്യ;

    എങ്കിൽ (അക്കം_Fds) {
        pollfds = (ഘടന pollfd*)m3_Malloc("WASI poll_oneoff", അക്കം_Fds * വലുപ്പം(ഘടന pollfd));
        എങ്കിൽ (pollfds == ശൂന്യ) {
            തിരികെ __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        വേണ്ടി (__wasi_size_t i = 0; i < i_എണ്ണം; i++) {
            എങ്കിൽ (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                തുടരുക;
            }
            pollfds[n].fd = (പൂർണ്ണസംഖ്യ)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    പൂർണ്ണസംഖ്യ timeoutMs = -1;
    എങ്കിൽ (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)INT_MAX) ? INT_MAX : (പൂർണ്ണസംഖ്യ)ms;
    }

    പൂർണ്ണസംഖ്യ ready;
    എങ്കിൽ (അക്കം_Fds) {
        ചെയ്യുക {
            ready = poll(pollfds, (nfds_t)അക്കം_Fds, timeoutMs);
        } ആകുമ്പോൾ (ready < 0 and errno == EINTR);
    } അല്ലെങ്കിൽ എങ്കിൽ (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        ഘടന timespec req;
        req.tv_sec = (time_t)(soonest / 1000000000);
        req.tv_nsec = (long)(soonest % 1000000000);

        ആകുമ്പോൾ (nanosleep(&req, &req) != 0 and errno == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } അല്ലെങ്കിൽ എങ്കിൽ (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } അല്ലെങ്കിൽ {
        // no clocks and no descriptors: nothing that could ever become ready
        തിരികെ __WASI_ERRNO_INVAL;
    }

    എങ്കിൽ (ready < 0) {
        __wasi_errno_t err = errno_to_wasi(errno);
        m3_Free(pollfds);
        തിരികെ err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t out = 0;
    __wasi_size_t n = 0;

    വേണ്ടി (__wasi_size_t i = 0; i < i_എണ്ണം; i++) {
        സ്ഥിരം m3_wasi_pollsub_t* sub = &i_subs[i];

        എങ്കിൽ (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            എങ്കിൽ (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_INVAL;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            } അല്ലെങ്കിൽ എങ്കിൽ (poll_remaining_ns(sub, elapsedNs) == 0) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_SUCCESS;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            }
            തുടരുക;
        }

        short revents = pollfds[n].revents;
        n++;

        എങ്കിൽ (revents == 0) {
            തുടരുക;
        }

        o_events[out].userdata = sub->userdata;
        o_events[out].type = sub->type;
        o_events[out].nbytes = 0;
        o_events[out].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        എങ്കിൽ (revents & POLLNVAL) {
            o_events[out].error = __WASI_ERRNO_BADF;
        } അല്ലെങ്കിൽ എങ്കിൽ (revents & POLLERR) {
            o_events[out].error = __WASI_ERRNO_IO;
        } അല്ലെങ്കിൽ {
            o_events[out].error = __WASI_ERRNO_SUCCESS;
            എങ്കിൽ (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[out].nbytes = poll_readable_bytes(pollfds[n - 1].fd);
            }
        }

        out++;
    }

    m3_Free(pollfds);

    *o_event_എണ്ണം = out;
    തിരികെ __WASI_ERRNO_SUCCESS;
}

#endif // m3_api_wasi_posix_h
