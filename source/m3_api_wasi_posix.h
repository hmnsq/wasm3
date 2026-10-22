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

#अन्तर्भाव "m3_core.h"

#जर defined(_WIN32)
#  error "This file is not for Windows"
#endif

#अन्तर्भाव <sys/types.h>
#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <errno.h>
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <limits.h>
#अन्तर्भाव <poll.h>
#अन्तर्भाव <string.h>
#अन्तर्भाव <time.h>

// FIONREAD, which poll_oneoff uses to tell the guest how much is waiting on a
// descriptor. Nothing here needs it - a descriptor whose count cannot be taken is
// reported as having something rather than nothing - so a system without the header
// simply does without.
#जर !defined(__wasi__)
#  include <sys/ioctl.h>
#endif

#जर defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__) || defined(__CYGWIN__)
#  include <unistd.h>
#  include <sys/uio.h>
#  जर defined(__APPLE__)
#    include <TargetConditionals.h>
#    जर TARGET_OS_OSX // TARGET_OS_MAC includes iOS
#      include <sys/random.h>
#    अन्यथा // iOS / Simulator
#      include <Security/Security.h>
#    endif
#  अन्यथा
#    include <sys/random.h>
#  endif
#  define d_m3WasiHasIovec
#endif

#अन्तर्भाव "m3_api_wasi_host.h"


/*
 * Plumbing
 */

स्थिर
__wasi_errno_t errno_to_wasi (पूर्णांक errnum)
{
    // clang-format off
    निवड (errnum) {
    स्थिती EPERM:    परत __WASI_ERRNO_PERM;
    स्थिती ENOENT:   परत __WASI_ERRNO_NOENT;
    स्थिती ESRCH:    परत __WASI_ERRNO_SRCH;
    स्थिती EINTR:    परत __WASI_ERRNO_INTR;
    स्थिती EIO:      परत __WASI_ERRNO_IO;
    स्थिती ENXIO:    परत __WASI_ERRNO_NXIO;
    स्थिती E2BIG:    परत __WASI_ERRNO_2BIG;
    स्थिती ENOEXEC:  परत __WASI_ERRNO_NOEXEC;
    स्थिती EBADF:    परत __WASI_ERRNO_BADF;
    स्थिती ECHILD:   परत __WASI_ERRNO_CHILD;
    स्थिती EAGAIN:   परत __WASI_ERRNO_AGAIN;
    स्थिती ENOMEM:   परत __WASI_ERRNO_NOMEM;
    स्थिती EACCES:   परत __WASI_ERRNO_ACCES;
    स्थिती EFAULT:   परत __WASI_ERRNO_FAULT;
    स्थिती EBUSY:    परत __WASI_ERRNO_BUSY;
    स्थिती EEXIST:   परत __WASI_ERRNO_EXIST;
    स्थिती EXDEV:    परत __WASI_ERRNO_XDEV;
    स्थिती ENODEV:   परत __WASI_ERRNO_NODEV;
    स्थिती ENOTDIR:  परत __WASI_ERRNO_NOTDIR;
    स्थिती EISDIR:   परत __WASI_ERRNO_ISDIR;
    स्थिती EINVAL:   परत __WASI_ERRNO_INVAL;
    स्थिती ENFILE:   परत __WASI_ERRNO_NFILE;
    स्थिती EMFILE:   परत __WASI_ERRNO_MFILE;
    स्थिती ENOTTY:   परत __WASI_ERRNO_NOTTY;
    स्थिती ETXTBSY:  परत __WASI_ERRNO_TXTBSY;
    स्थिती EFBIG:    परत __WASI_ERRNO_FBIG;
    स्थिती ENOSPC:   परत __WASI_ERRNO_NOSPC;
    स्थिती ESPIPE:   परत __WASI_ERRNO_SPIPE;
    स्थिती EROFS:    परत __WASI_ERRNO_ROFS;
    स्थिती EMLINK:   परत __WASI_ERRNO_MLINK;
    स्थिती EPIPE:    परत __WASI_ERRNO_PIPE;
    स्थिती गलत_तर्क:     परत __WASI_ERRNO_DOM;
    स्थिती दुस्फल:   परत __WASI_ERRNO_RANGE;
    }
    // clang-format on
    परत __WASI_ERRNO_INVAL;
}

स्थिर
__wasi_filetype_t filetype_from_stat_mode (पूर्णांक mode)
{
    परत (S_ISBLK(mode) ? __WASI_FILETYPE_BLOCK_DEVICE : 0) |
           (S_ISCHR(mode) ? __WASI_FILETYPE_CHARACTER_DEVICE : 0) |
           (S_ISDIR(mode) ? __WASI_FILETYPE_DIRECTORY : 0) |
           (S_ISREG(mode) ? __WASI_FILETYPE_REGULAR_FILE : 0) |
           //(S_ISSOCK(mode)  ? __WASI_FILETYPE_SOCKET_STREAM    : 0) |
           (S_ISLNK(mode) ? __WASI_FILETYPE_SYMBOLIC_LINK : 0);
}

स्थिर
शून्य filestat_from_stat (स्थिरांक संरचना stat* i_stat, m3_wasi_filestat_t* o_stat)
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
स्थिर
पूर्णांक preopen_fd (__wasi_fd_t i_fd)
{
    जर (i_fd >= d_m3_Wasi_Preopen_मोजणी) {
        परत -1;
    }
    परत m3_wasi_preopen[i_fd].fd;
}

// wasi-libc (>= wasi-sdk 12) makes clockid_t an opaque pointer rather than an int,
// so the "no such clock" sentinel has to be the null one
#जर defined(__wasi__)
#  define d_m3ClockIdInvalid  ((clockid_t) रिक्त)
#अन्यथा
#  define d_m3ClockIdInvalid  ((clockid_t) -1)
#endif

स्थिर
clockid_t convert_clockid (__wasi_clockid_t in)
{
    निवड (in) {
    स्थिती __WASI_CLOCKID_MONOTONIC: परत CLOCK_MONOTONIC;
    स्थिती __WASI_CLOCKID_REALTIME: परत CLOCK_REALTIME;
#जर defined(CLOCK_PROCESS_CPUTIME_ID)
    स्थिती __WASI_CLOCKID_PROCESS_CPUTIME_ID: परत CLOCK_PROCESS_CPUTIME_ID;
#endif
#जर defined(CLOCK_THREAD_CPUTIME_ID)
    स्थिती __WASI_CLOCKID_THREAD_CPUTIME_ID: परत CLOCK_THREAD_CPUTIME_ID;
#endif
    पूर्वनिर्धारित: परत d_m3ClockIdInvalid;
    }
}

स्थिर
__wasi_timestamp_t convert_timespec (स्थिरांक संरचना timespec* ts)
{
    जर (ts->tv_sec < 0) {
        परत 0;
    }
    जर ((__wasi_timestamp_t)ts->tv_sec >= UINT64_MAX / 1000000000) {
        परत UINT64_MAX;
    }
    परत (__wasi_timestamp_t)ts->tv_sec * 1000000000 + ts->tv_nsec;
}


/*
 * Setup
 */

स्थिर
शून्य m3_wasi_host_init ()
{
    साठी (पूर्णांक i = 3; i < d_m3_Wasi_Preopen_मोजणी; i++) {
        जर (m3_wasi_preopen[i].fd < 0) {
            m3_wasi_preopen[i].fd = खोलो(m3_wasi_preopen[i].real_path, O_RDONLY);
        }
    }
}


/*
 * Descriptors
 */

स्थिर
__wasi_errno_t m3_wasi_host_fd_close (__wasi_fd_t i_fd)
{
    जर (बंद(i_fd) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

// no fdatasync on Apple, where fsync is the stronger promise of the two, and neither
// on a platform this does not name
#जर defined(__APPLE__)
#  define d_m3WasiDatasync(FD)  fsync(FD)
#किंवाजर defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__linux__) || defined(__EMSCRIPTEN__)
#  define d_m3WasiDatasync(FD)  fdatasync(FD)
#endif

स्थिर
__wasi_errno_t m3_wasi_host_fd_datasync (__wasi_fd_t i_fd)
{
#जर defined(d_m3WasiDatasync)
    जर (d_m3WasiDatasync(i_fd) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
#अन्यथा
    परत __WASI_ERRNO_NOSYS;
#endif
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_advise (__wasi_fd_t i_fd, __wasi_filesize_t i_offset,
                                       __wasi_filesize_t i_length, __wasi_advice_t i_advice)
{
    // Purely advisory: passing it on is a best effort, and dropping it is
    // still a conforming implementation. Only the fd has to be valid.
#जर defined(POSIX_FADV_NORMAL)
    पूर्णांक adv;
    निवड (i_advice) {
    स्थिती __WASI_ADVICE_NORMAL: adv = POSIX_FADV_NORMAL; थांबा;
    स्थिती __WASI_ADVICE_SEQUENTIAL: adv = POSIX_FADV_SEQUENTIAL; थांबा;
    स्थिती __WASI_ADVICE_RANDOM: adv = POSIX_FADV_RANDOM; थांबा;
    स्थिती __WASI_ADVICE_WILLNEED: adv = POSIX_FADV_WILLNEED; थांबा;
    स्थिती __WASI_ADVICE_DONTNEED: adv = POSIX_FADV_DONTNEED; थांबा;
    स्थिती __WASI_ADVICE_NOREUSE: adv = POSIX_FADV_NOREUSE; थांबा;
    पूर्वनिर्धारित: परत __WASI_ERRNO_INVAL;
    }

    पूर्णांक ret = posix_fadvise(i_fd, i_offset, i_length, adv);
    जर (ret != 0) {
        परत errno_to_wasi(ret);
    }
#अन्यथा
    जर (i_advice > __WASI_ADVICE_NOREUSE) {
        परत __WASI_ERRNO_INVAL;
    }

    संरचना stat fd_stat;
    जर (ख_स्थिति(i_fd, &fd_stat) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
#endif

    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_fdstat (__wasi_fd_t i_fd, __wasi_filetype_t* o_filetype,
                                       __wasi_fdflags_t* o_flags)
{
    *o_filetype = __WASI_FILETYPE_UNKNOWN;
    *o_flags = 0;

    पूर्णांक fl = fcntl(i_fd, F_GETFL);
    जर (fl < 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    संरचना stat fd_stat;
    जर (ख_स्थिति(i_fd, &fd_stat) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    *o_filetype = filetype_from_stat_mode(fd_stat.st_mode);
    *o_flags = ((fl & O_APPEND) ? __WASI_FDFLAGS_APPEND : 0) |
               ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
               ((fl & O_NONBLOCK) ? __WASI_FDFLAGS_NONBLOCK : 0) |
               //((fl & O_RSYNC)     ? __WASI_FDFLAGS_RSYNC     : 0) |
               ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);
    परत __WASI_ERRNO_SUCCESS;
}

// F_SETFL changes only a handful of the bits a file was opened with: the sync ones
// are fixed at open time, so a request to turn one on is refused rather than
// accepted and quietly dropped. Turning one off is refused the same way - the guest
// asked for a state the descriptor cannot be put into either way.
स्थिर
__wasi_errno_t m3_wasi_host_fd_fdstat_set_flags (__wasi_fd_t i_fd, __wasi_fdflags_t i_flags)
{
    पूर्णांक fl = fcntl(i_fd, F_GETFL);
    जर (fl < 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    स्थिरांक __wasi_fdflags_t fixed = __WASI_FDFLAGS_DSYNC | __WASI_FDFLAGS_RSYNC | __WASI_FDFLAGS_SYNC;

    // RSYNC is left out of the readback for the same reason fd_fdstat does not
    // report it: it has no bit of its own everywhere this builds
    __wasi_fdflags_t       current = ((fl & O_DSYNC) ? __WASI_FDFLAGS_DSYNC : 0) |
                                     ((fl & O_SYNC) ? __WASI_FDFLAGS_SYNC : 0);

    जर ((i_flags & fixed) != (current & fixed)) {
        परत __WASI_ERRNO_NOTSUP;
    }

    fl &= ~(O_APPEND | O_NONBLOCK);
    fl |= (i_flags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0;
    fl |= (i_flags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0;

    जर (fcntl(i_fd, F_SETFL, fl) < 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_filestat (__wasi_fd_t i_fd, m3_wasi_filestat_t* o_stat)
{
    स_रखो(o_stat, 0, आकार(*o_stat));

    संरचना stat fd_stat;
    जर (ख_स्थिति(i_fd, &fd_stat) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    filestat_from_stat(&fd_stat, o_stat);
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_set_size (__wasi_fd_t i_fd, __wasi_filesize_t i_size)
{
    जर (ftruncate(i_fd, (off_t)i_size) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_set_times (__wasi_fd_t i_fd, __wasi_timestamp_t i_atim,
                                          __wasi_timestamp_t i_mtim, __wasi_fstflags_t i_flags)
{
    // a time is either given, asked for as now, or left alone
    संरचना timespec बार[2];

    जर (i_flags & __WASI_FSTFLAGS_ATIM_NOW) {
        बार[0].tv_sec = 0;
        बार[0].tv_nsec = UTIME_NOW;
    } अन्यथा जर (i_flags & __WASI_FSTFLAGS_ATIM) {
        बार[0].tv_sec = i_atim / 1000000000ULL;
        बार[0].tv_nsec = i_atim % 1000000000ULL;
    } अन्यथा {
        बार[0].tv_sec = 0;
        बार[0].tv_nsec = UTIME_OMIT;
    }

    जर (i_flags & __WASI_FSTFLAGS_MTIM_NOW) {
        बार[1].tv_sec = 0;
        बार[1].tv_nsec = UTIME_NOW;
    } अन्यथा जर (i_flags & __WASI_FSTFLAGS_MTIM) {
        बार[1].tv_sec = i_mtim / 1000000000ULL;
        बार[1].tv_nsec = i_mtim % 1000000000ULL;
    } अन्यथा {
        बार[1].tv_sec = 0;
        बार[1].tv_nsec = UTIME_OMIT;
    }

    जर (futimens(i_fd, बार) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_seek (__wasi_fd_t i_fd, int64_t i_offset, पूर्णांक i_whence,
                                     __wasi_filesize_t* o_pos)
{
    *o_pos = 0;

    off_t ret = lseek(i_fd, (off_t)i_offset, i_whence);
    जर (ret < 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    *o_pos = (__wasi_filesize_t)ret;
    परत __WASI_ERRNO_SUCCESS;
}


/*
 * Transfers
 */

#जर defined(d_m3WasiHasIovec)

// m3_wasi_iovec_t is laid out as struct iovec is, but they are still two types, so
// the list is copied across rather than cast.
स्थिर
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, स्थिरांक m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    संरचना iovec iovs[d_m3WasiMaxIovs];
    साठी (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        iovs[i].iov_base = i_iovs[i].buf;
        iovs[i].iov_len = i_iovs[i].len;
    }

    ssize_t ret = i_write ? writev(i_fd, iovs, (पूर्णांक)i_iovsLen) : readv(i_fd, iovs, (पूर्णांक)i_iovsLen);
    जर (ret < 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    *o_done = (__wasi_size_t)ret;
    परत __WASI_ERRNO_SUCCESS;
}

#अन्यथा

// No readv/writev here, so the list is walked one buffer at a time and a short
// transfer ends it, which is what readv would have done
स्थिर
__wasi_errno_t transfer_vectored (__wasi_fd_t i_fd, स्थिरांक m3_wasi_iovec_t* i_iovs,
                                  __wasi_size_t i_iovsLen, bool i_write, __wasi_size_t* o_done)
{
    *o_done = 0;

    माप_प्रकार done = 0;

    साठी (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        शून्य*  addr = i_iovs[i].buf;
        माप_प्रकार len = i_iovs[i].len;
        जर (len == 0) {
            सुरूठेवा;
        }

        ssize_t ret = i_write ? लिख(i_fd, addr, len) : पठ(i_fd, addr, len);
        जर (ret < 0) {
            परत errno_to_wasi(त्रुटि_सं);
        }

        done += (माप_प्रकार)ret;
        जर ((माप_प्रकार)ret < len) {
            थांबा;
        }
    }

    *o_done = (__wasi_size_t)done;
    परत __WASI_ERRNO_SUCCESS;
}

#endif // d_m3WasiHasIovec

// pread/pwrite over the list. The offset walks with the data, so a short transfer
// stops it rather than leaving a hole.
स्थिर
__wasi_errno_t transfer_at (__wasi_fd_t i_fd, स्थिरांक m3_wasi_iovec_t* i_iovs,
                            __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset, bool i_write,
                            __wasi_size_t* o_done)
{
    *o_done = 0;

    माप_प्रकार done = 0;

    साठी (__wasi_size_t i = 0; i < i_iovsLen; i++) {
        शून्य*  addr = i_iovs[i].buf;
        माप_प्रकार len = i_iovs[i].len;
        जर (len == 0) {
            सुरूठेवा;
        }

        ssize_t ret = i_write ? pwrite(i_fd, addr, len, (off_t)(i_offset + done))
                              : pread(i_fd, addr, len, (off_t)(i_offset + done));
        जर (ret < 0) {
            परत errno_to_wasi(त्रुटि_सं);
        }

        done += (माप_प्रकार)ret;
        जर ((माप_प्रकार)ret < len) {
            थांबा;
        }
    }

    *o_done = (__wasi_size_t)done;
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_read (__wasi_fd_t i_fd, स्थिरांक m3_wasi_iovec_t* i_iovs,
                                     __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    परत transfer_vectored(i_fd, i_iovs, i_iovsLen, false, o_done);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_write (__wasi_fd_t i_fd, स्थिरांक m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_size_t* o_done)
{
    परत transfer_vectored(i_fd, i_iovs, i_iovsLen, true, o_done);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_pread (__wasi_fd_t i_fd, स्थिरांक m3_wasi_iovec_t* i_iovs,
                                      __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                      __wasi_size_t* o_done)
{
    परत transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, false, o_done);
}

स्थिर
__wasi_errno_t m3_wasi_host_fd_pwrite (__wasi_fd_t i_fd, स्थिरांक m3_wasi_iovec_t* i_iovs,
                                       __wasi_size_t i_iovsLen, __wasi_filesize_t i_offset,
                                       __wasi_size_t* o_done)
{
    परत transfer_at(i_fd, i_iovs, i_iovsLen, i_offset, true, o_done);
}


/*
 * Paths
 */

स्थिर
__wasi_errno_t m3_wasi_host_path_open (__wasi_fd_t i_dirfd, ccstr_t i_path,
                                       __wasi_oflags_t i_oflags, __wasi_rights_t i_rights,
                                       __wasi_fdflags_t i_fdFlags, __wasi_fd_t* o_fd)
{
    *o_fd = 0;

    पूर्णांक dirfd = preopen_fd(i_dirfd);
    जर (dirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    // translate o_flags and fs_flags into flags and mode
    पूर्णांक flags = ((i_oflags & __WASI_OFLAGS_CREAT) ? O_CREAT : 0) |
                //((i_oflags & __WASI_OFLAGS_DIRECTORY)      ? O_DIRECTORY : 0) |
                ((i_oflags & __WASI_OFLAGS_EXCL) ? O_EXCL : 0) |
                ((i_oflags & __WASI_OFLAGS_TRUNC) ? O_TRUNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_APPEND) ? O_APPEND : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_DSYNC) ? O_DSYNC : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_NONBLOCK) ? O_NONBLOCK : 0) |
                //((i_fdFlags & __WASI_FDFLAGS_RSYNC)        ? O_RSYNC     : 0) |
                ((i_fdFlags & __WASI_FDFLAGS_SYNC) ? O_SYNC : 0);

    जर ((i_rights & __WASI_RIGHTS_FD_READ) && (i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_RDWR;
    } अन्यथा जर ((i_rights & __WASI_RIGHTS_FD_WRITE)) {
        flags |= O_WRONLY;
    } अन्यथा जर ((i_rights & __WASI_RIGHTS_FD_READ)) {
        flags |= O_RDONLY; // no-op because O_RDONLY is 0
    }

    पूर्णांक host_fd = openat(dirfd, i_path, flags, 0644);
    जर (host_fd < 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    *o_fd = (__wasi_fd_t)host_fd;
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_filestat (__wasi_fd_t i_dirfd, ccstr_t i_path, bool i_follow,
                                           m3_wasi_filestat_t* o_stat)
{
    स_रखो(o_stat, 0, आकार(*o_stat));

    पूर्णांक dirfd = preopen_fd(i_dirfd);
    जर (dirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    संरचना stat path_stat;
    जर (fstatat(dirfd, i_path, &path_stat, i_follow ? 0 : AT_SYMLINK_NOFOLLOW) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    filestat_from_stat(&path_stat, o_stat);
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_create_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    जर (dirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    जर (mkdirat(dirfd, i_path, 0755) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_remove_directory (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    जर (dirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    जर (unlinkat(dirfd, i_path, AT_REMOVEDIR) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_unlink_file (__wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    जर (dirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    जर (unlinkat(dirfd, i_path, 0) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_rename (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath,
                                         __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    पूर्णांक oldDirfd = preopen_fd(i_oldDirfd);
    पूर्णांक newDirfd = preopen_fd(i_newDirfd);
    जर (oldDirfd < 0 || newDirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    जर (renameat(oldDirfd, i_oldPath, newDirfd, i_newPath) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_link (__wasi_fd_t i_oldDirfd, ccstr_t i_oldPath, bool i_follow,
                                       __wasi_fd_t i_newDirfd, ccstr_t i_newPath)
{
    पूर्णांक oldDirfd = preopen_fd(i_oldDirfd);
    पूर्णांक newDirfd = preopen_fd(i_newDirfd);
    जर (oldDirfd < 0 || newDirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    जर (linkat(oldDirfd, i_oldPath, newDirfd, i_newPath, i_follow ? AT_SYMLINK_FOLLOW : 0) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_symlink (ccstr_t i_target, __wasi_fd_t i_dirfd, ccstr_t i_path)
{
    पूर्णांक dirfd = preopen_fd(i_dirfd);
    जर (dirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    जर (symlinkat(i_target, dirfd, i_path) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_path_readlink (__wasi_fd_t i_dirfd, ccstr_t i_path, अक्षर* o_buf,
                                           __wasi_size_t i_bufLen, __wasi_size_t* o_bufUsed)
{
    *o_bufUsed = 0;

    पूर्णांक dirfd = preopen_fd(i_dirfd);
    जर (dirfd < 0) {
        परत __WASI_ERRNO_BADF;
    }

    ssize_t ret = readlinkat(dirfd, i_path, o_buf, i_bufLen);
    जर (ret < 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    *o_bufUsed = (__wasi_size_t)ret;
    परत __WASI_ERRNO_SUCCESS;
}


/*
 * Clocks and entropy
 */

स्थिर
__wasi_errno_t m3_wasi_host_clock_res_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_res)
{
    *o_res = 0;

    clockid_t clk = convert_clockid(i_clockId);
    जर (clk == d_m3ClockIdInvalid) {
        परत __WASI_ERRNO_INVAL;
    }

    संरचना timespec tp;
    जर (clock_getres(clk, &tp) != 0) {
        *o_res = 1000000;
    } अन्यथा {
        *o_res = convert_timespec(&tp);
    }
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_clock_time_get (__wasi_clockid_t i_clockId, __wasi_timestamp_t* o_time)
{
    *o_time = 0;

    clockid_t clk = convert_clockid(i_clockId);
    जर (clk == d_m3ClockIdInvalid) {
        परत __WASI_ERRNO_INVAL;
    }

    संरचना timespec tp;
    जर (clock_gettime(clk, &tp) != 0) {
        परत errno_to_wasi(त्रुटि_सं);
    }

    *o_time = convert_timespec(&tp);
    परत __WASI_ERRNO_SUCCESS;
}

स्थिर
__wasi_errno_t m3_wasi_host_random_get (शून्य* o_buf, __wasi_size_t i_len)
{
    uint8_t* buf = (uint8_t*)o_buf;

    पर्यंत (i_len) {
        ssize_t retlen;

#जर defined(__wasi__) || defined(__APPLE__) || defined(__ANDROID_API__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        माप_प्रकार reqlen = M3_MIN(i_len, 256);
#  जर defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
        retlen = SecRandomCopyBytes(kSecRandomDefault, reqlen, buf) < 0 ? -1 : (ssize_t)reqlen;
#  अन्यथा
        retlen = getentropy(buf, reqlen) < 0 ? -1 : (ssize_t)reqlen;
#  endif
#किंवाजर defined(__FreeBSD__) || defined(__linux__)
        retlen = getrandom(buf, i_len, 0);
#अन्यथा
        परत __WASI_ERRNO_NOSYS;
#endif

        जर (retlen < 0) {
            जर (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN) {
                सुरूठेवा;
            }
            परत errno_to_wasi(त्रुटि_सं);
        }

        buf += retlen;
        i_len -= (__wasi_size_t)retlen;
    }

    परत __WASI_ERRNO_SUCCESS;
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
स्थिर
uint64_t poll_remaining_ns (स्थिरांक m3_wasi_pollsub_t* i_sub, uint64_t i_elapsedNs)
{
    जर (not i_sub->isAbsolute) {
        परत (i_sub->timeout > i_elapsedNs) ? i_sub->timeout - i_elapsedNs : 0;
    }

    clockid_t clk = convert_clockid(i_sub->clockId);
    जर (clk == d_m3ClockIdInvalid) {
        परत UINT64_MAX;
    }

    संरचना timespec tp;
    जर (clock_gettime(clk, &tp) != 0) {
        परत 0;
    }

    __wasi_timestamp_t now = convert_timespec(&tp);
    परत (i_sub->timeout > now) ? i_sub->timeout - now : 0;
}

// How much a descriptor ready for reading has waiting on it. The guest is told a
// count so that it can size the read that follows, and a wrong one only costs it a
// short read - so a descriptor that cannot answer says one byte rather than nothing,
// which would read as end of file.
स्थिर
__wasi_filesize_t poll_readable_bytes (पूर्णांक i_fd)
{
#जर defined(FIONREAD)
    पूर्णांक available = 0;
    जर (ioctl(i_fd, FIONREAD, &available) == 0 and available > 0) {
        परत (__wasi_filesize_t)available;
    }
#endif
    परत 1;
}

स्थिर
__wasi_errno_t m3_wasi_host_poll_oneoff (स्थिरांक m3_wasi_pollsub_t* i_subs, __wasi_size_t i_मोजणी,
                                         m3_wasi_pollevent_t* o_events, __wasi_size_t* o_event_मोजणी)
{
    *o_event_मोजणी = 0;

    // The soonest clock deadline is what bounds the wait; without one, poll() is
    // told to wait forever, which is what a subscription list of descriptors alone
    // asks for. A clock this system has not got sets no deadline but is still a
    // subscription, and is reported below as one event carrying EINVAL.
    uint64_t      soonest = UINT64_MAX;
    bool          hasDeadline = false;
    bool          hasClock = false;

    __wasi_size_t अंक_Fds = 0;

    साठी (__wasi_size_t i = 0; i < i_मोजणी; i++) {
        जर (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
            hasClock = true;

            uint64_t delay = poll_remaining_ns(&i_subs[i], 0);
            जर (delay != UINT64_MAX) {
                soonest = M3_MIN(soonest, delay);
                hasDeadline = true;
            }
        } अन्यथा {
            अंक_Fds++;
        }
    }

    संरचना pollfd* pollfds = रिक्त;

    जर (अंक_Fds) {
        pollfds = (संरचना pollfd*)m3_Malloc("WASI poll_oneoff", अंक_Fds * आकार(संरचना pollfd));
        जर (pollfds == रिक्त) {
            परत __WASI_ERRNO_NOMEM;
        }

        __wasi_size_t n = 0;
        साठी (__wasi_size_t i = 0; i < i_मोजणी; i++) {
            जर (i_subs[i].type == __WASI_EVENTTYPE_CLOCK) {
                सुरूठेवा;
            }
            pollfds[n].fd = (पूर्णांक)i_subs[i].fd;
            pollfds[n].events = (i_subs[i].type == __WASI_EVENTTYPE_FD_READ) ? POLLIN : POLLOUT;
            pollfds[n].revents = 0;
            n++;
        }
    }

    // poll() counts in milliseconds and cannot say "no wait at all" apart from a
    // wait that rounds down to nothing, which is the same thing here. Rounding up
    // is what keeps a sub-millisecond deadline from becoming a spin.
    पूर्णांक timeoutMs = -1;
    जर (hasDeadline) {
        uint64_t ms = (soonest + 999999) / 1000000;
        timeoutMs = (ms > (uint64_t)पूर्णांक_उच्च) ? पूर्णांक_उच्च : (पूर्णांक)ms;
    }

    पूर्णांक ready;
    जर (अंक_Fds) {
        करा {
            ready = poll(pollfds, (nfds_t)अंक_Fds, timeoutMs);
        } पर्यंत (ready < 0 and त्रुटि_सं == EINTR);
    } अन्यथा जर (hasDeadline) {
        // nothing to watch, so the deadline is the whole of the wait
        संरचना timespec req;
        req.tv_sec = (समय_प्रकार)(soonest / 1000000000);
        req.tv_nsec = (दीर्घ)(soonest % 1000000000);

        पर्यंत (nanosleep(&req, &req) != 0 and त्रुटि_सं == EINTR) {
            // interrupted: req holds what is left of it
        }
        ready = 0;
    } अन्यथा जर (hasClock) {
        // every clock named is one this system has not got, so there is nothing to
        // wait for and every one of them is about to be reported as a bad argument
        ready = 0;
    } अन्यथा {
        // no clocks and no descriptors: nothing that could ever become ready
        परत __WASI_ERRNO_INVAL;
    }

    जर (ready < 0) {
        __wasi_errno_t err = errno_to_wasi(त्रुटि_सं);
        m3_Free(pollfds);
        परत err;
    }

    // How much of the wait actually happened. poll() answers 0 only when it waited
    // the whole timeout out, and that timeout was the soonest deadline; anything
    // else came back on a descriptor, which by definition beat every deadline. So
    // the two cases are the full wait and none of it, and neither needs the clock
    // read again - an absolute deadline is held against its own clock regardless.
    uint64_t      elapsedNs = (ready == 0 and hasDeadline) ? soonest : 0;

    __wasi_size_t out = 0;
    __wasi_size_t n = 0;

    साठी (__wasi_size_t i = 0; i < i_मोजणी; i++) {
        स्थिरांक m3_wasi_pollsub_t* sub = &i_subs[i];

        जर (sub->type == __WASI_EVENTTYPE_CLOCK) {
            // Every deadline is asked again rather than just the soonest: waiting
            // that one out may have passed a later one too, and an absolute
            // deadline already in the past fires without any wait at all.
            जर (convert_clockid(sub->clockId) == d_m3ClockIdInvalid) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_INVAL;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            } अन्यथा जर (poll_remaining_ns(sub, elapsedNs) == 0) {
                o_events[out].userdata = sub->userdata;
                o_events[out].error = __WASI_ERRNO_SUCCESS;
                o_events[out].type = __WASI_EVENTTYPE_CLOCK;
                o_events[out].nbytes = 0;
                o_events[out].flags = 0;
                out++;
            }
            सुरूठेवा;
        }

        लघु revents = pollfds[n].revents;
        n++;

        जर (revents == 0) {
            सुरूठेवा;
        }

        o_events[out].userdata = sub->userdata;
        o_events[out].type = sub->type;
        o_events[out].nbytes = 0;
        o_events[out].flags = (revents & POLLHUP) ? __WASI_EVENTRWFLAGS_FD_READWRITE_HANGUP : 0;

        जर (revents & POLLNVAL) {
            o_events[out].error = __WASI_ERRNO_BADF;
        } अन्यथा जर (revents & POLLERR) {
            o_events[out].error = __WASI_ERRNO_IO;
        } अन्यथा {
            o_events[out].error = __WASI_ERRNO_SUCCESS;
            जर (sub->type == __WASI_EVENTTYPE_FD_READ and (revents & POLLIN)) {
                o_events[out].nbytes = poll_readable_bytes(pollfds[n - 1].fd);
            }
        }

        out++;
    }

    m3_Free(pollfds);

    *o_event_मोजणी = out;
    परत __WASI_ERRNO_SUCCESS;
}

#endif // m3_api_wasi_posix_h
