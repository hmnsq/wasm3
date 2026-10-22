//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_host_posix_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_host_posix_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಆದರೆ defined(_WIN32)
#  error "This file is not for Windows"
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "m3_host.h"

#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#ಅನ್ತರ್ಭಾವ <sys/mman.h>
#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ಅನ್ತರ್ಭಾವ <unistd.h>
#ಅನ್ತರ್ಭಾವ <signal.h>
#ಅನ್ತರ್ಭಾವ <errno.h>
#ಅನ್ತರ್ಭಾವ <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ d_m3_Has_Thread_ರಾಶಿ_Probe
#  ಆದರೆ defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_ರಾಶಿ_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    ಆದರೆ __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_ರಾಶಿ_Probe          1
#    ಇಲ್ಲದಿದ್ದರೆ
#      define d_m3_Has_Thread_ರಾಶಿ_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_ರಾಶಿ_Probe            1    // musl keeps it in libc
#  ಇಲ್ಲದಿದ್ದರೆ
#    define d_m3_Has_Thread_ರಾಶಿ_Probe            0
#  endif
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_Has_Thread_ರಾಶಿ_Probe
#  include <pthread.h>
#  ಆದರೆ defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_Has_Thread_ರಾಶಿ_Probe && defined(__linux__)

// The first thread's stack, asked of the kernel rather than of pthreads, because
// pthreads is not always cheap about it: musl finds that stack by probing for its
// end a page at a time - a system call per page, and there can be a great many -
// where this is one line of text and a getrlimit. A thread given a stack of its own
// is left to pthreads, which has that one written down and answers at once.
//
// /proc says where the stack started, RLIMIT_STACK how far down it may grow. Another
// mapping can stop it growing that far, which this does not go looking for: the
// answer is then a stack larger than the real one, the budget stays what the build
// asked for, and that is what a build unable to measure gets anyway.
ಸ್ಥಿರ
ಶೂನ್ಯ* linux_main_ರಾಶಿ_ತಳ (ಶೂನ್ಯ)
{
    ಆದರೆ ((ದೀರ್ಘ)syscall(SYS_gettid) != (ದೀರ್ಘ)getpid()) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ರಚನೆ rlimit ಎಲ್ಲೆ;

    ಆದರೆ (getrlimit(RLIMIT_STACK, &ಎಲ್ಲೆ) != 0 or
        ಎಲ್ಲೆ.rlim_cur == RLIM_INFINITY or ಎಲ್ಲೆ.rlim_cur == 0) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಪೂರ್ಣಾಂಕ fd = open("/proc/self/stat", O_RDONLY);

    ಆದರೆ (fd < 0) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಅಕ್ಷರ    ಸಾಲು_2[512];
    ssize_t got = ಪಠ(fd, ಸಾಲು_2, ಗಾತ್ರ(ಸಾಲು_2) - 1);

    close(fd);

    ಆದರೆ (got <= 0) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಸಾಲು_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಕ್ಷೇತ್ರ = strrchr(ಸಾಲು_2, ')');

    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ ಸಂಖ್ಯೆ = 2; ಕ್ಷೇತ್ರ and ಸಂಖ್ಯೆ < 28; ++ಸಂಖ್ಯೆ) {
        ಕ್ಷೇತ್ರ = strchr(ಕ್ಷೇತ್ರ + 1, ' ');
    }

    ಆದರೆ (ಕ್ಷೇತ್ರ == ಖಾಲಿ) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ ತುದಿ = strtoull(ಕ್ಷೇತ್ರ + 1, ಖಾಲಿ, 10);

    // A limit reaching below nothing is not a stack this can describe
    ಆದರೆ (ತುದಿ == 0 or (ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ) ಎಲ್ಲೆ.rlim_cur > ತುದಿ) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಹಿಂತಿರುಗಿ (u8*)(uintptr_t)ತುದಿ - (size_t)ಎಲ್ಲೆ.rlim_cur;
}

#ಆದರೆ_ಅಂತ್ಯ

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
ಶೂನ್ಯ* m3_Host_ರಾಶಿ_ತಳ (ಶೂನ್ಯ)
{
#ಆದರೆ !d_m3_Has_Thread_ರಾಶಿ_Probe

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;

#ಅಥವಾಆದರೆ defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       ಮೇಲಿನ = (u8*)pthread_get_stackaddr_np(self);
    size_t    ಗಾತ್ರ_2 = pthread_get_stacksize_np(self);

    ಆದರೆ (ಮೇಲಿನ == ಖಾಲಿ or ಗಾತ್ರ_2 == 0) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಹಿಂತಿರುಗಿ ಮೇಲಿನ - ಗಾತ್ರ_2;

#ಇಲ್ಲದಿದ್ದರೆ

#  ಆದರೆ defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_ತಳ = (u8*)linux_main_ರಾಶಿ_ತಳ();

    ಆದರೆ (main_ತಳ) {
        ಹಿಂತಿರುಗಿ main_ತಳ;
    }

#  endif

    pthread_attr_t attr;

    ಆದರೆ (pthread_getattr_np(pthread_self(), &attr) != 0) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಶೂನ್ಯ*  ತಳ = ಖಾಲಿ;
    size_t ಗಾತ್ರ_2 = 0;

    ಆದರೆ (pthread_attr_getstack(&attr, &ತಳ, &ಗಾತ್ರ_2) != 0 or ಗಾತ್ರ_2 == 0) {
        ತಳ = ಖಾಲಿ;
    }

    pthread_attr_destroy(&attr);

    ಹಿಂತಿರುಗಿ ತಳ;

#ಆದರೆ_ಅಂತ್ಯ
}

ಸ್ಥಿರ IM3Runtime       s_posixSuspendRuntime = ಖಾಲಿ;
ಸ್ಥಿರ ರಚನೆ sigaction s_oldSigTstp;
ಸ್ಥಿರ ರಚನೆ sigaction s_oldSigInt;
ಸ್ಥಿರ bool             s_suspendHandlersInstalled = false;

ಸ್ಥಿರ
ಶೂನ್ಯ m3_PosixSignalHandler (ಪೂರ್ಣಾಂಕ sig)
{
    (ಶೂನ್ಯ)sig;
    ಆದರೆ (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

ಶೂನ್ಯ m3_Host_Install_ಅಡಚಣೆ_Handler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    ಆದರೆ (!s_suspendHandlersInstalled) {
        ರಚನೆ sigaction sa;
        memset(&sa, 0, ಗಾತ್ರ(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

ಶೂನ್ಯ m3_Host_Remove_ಅಡಚಣೆ_Handler (ಶೂನ್ಯ)
{
    ಆದರೆ (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, ಖಾಲಿ);
        sigaction(SIGINT, &s_oldSigInt, ಖಾಲಿ);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = ಖಾಲಿ;
}

u64 m3_Host_ಸಮಯ_Ms (ಶೂನ್ಯ)
{
    ರಚನೆ timespec now;

    ಆದರೆ (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        ಹಿಂತಿರುಗಿ 0;
    }

    ಹಿಂತಿರುಗಿ (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

ಶೂನ್ಯ m3_HostSleepNs (u64 i_ns)
{
    ರಚನೆ timespec ಎಡ;
    ಎಡ.tv_sec = (time_t)(i_ns / 1000000000u);
    ಎಡ.tv_nsec = (ದೀರ್ಘ)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    ಆಗ (nanosleep(&ಎಡ, &ಎಡ) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_ಮಾರ್ಗ, size_t i_max_ಬೈಟುಗಳು, M3HostFile* o_file)
{
    ಪೂರ್ಣಾಂಕ fd = open(i_ಮಾರ್ಗ, O_RDONLY);
    ಆದರೆ (fd < 0) {
        ಹಿಂತಿರುಗಿ false;
    }

    ರಚನೆ stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    ಆದರೆ (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_max_ಬೈಟುಗಳು and (uint64_t) info.st_size > (uint64_t)i_max_ಬೈಟುಗಳು)) {
        close(fd);
        ಹಿಂತಿರುಗಿ m3_HostReadFile(i_ಮಾರ್ಗ, i_max_ಬೈಟುಗಳು, o_file);
    }

    size_t ಗಾತ್ರ_2 = (size_t)info.st_size;
    ಶೂನ್ಯ*  ತಳ = mmap(ಖಾಲಿ, ಗಾತ್ರ_2, PROT_READ, MAP_PRIVATE, fd, 0);

    ಆದರೆ (ತಳ == MAP_FAILED) {
        close(fd);
        ಹಿಂತಿರುಗಿ m3_HostReadFile(i_ಮಾರ್ಗ, i_max_ಬೈಟುಗಳು, o_file);
    }

#ಆದರೆ defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (ಶೂನ್ಯ)flock(fd, LOCK_SH | LOCK_NB);
#ಆದರೆ_ಅಂತ್ಯ

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->ದತ್ತಾಂಶ = ತಳ;
    o_file->ಗಾತ್ರ_2 = ಗಾತ್ರ_2;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    ಹಿಂತಿರುಗಿ true;
}

ಶೂನ್ಯ m3_HostUnmapFile (M3HostFile* io_file)
{
    ಆದರೆ (io_file->mapped) {
        munmap(io_file->ದತ್ತಾಂಶ, io_file->ಗಾತ್ರ_2);

        ಆದರೆ (io_file->handle != d_m3HostNoHandle) {
            close((ಪೂರ್ಣಾಂಕ)io_file->handle);     // and with it the lock
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        m3_Free(io_file->ದತ್ತಾಂಶ);
    }

    io_file->ದತ್ತಾಂಶ = ಖಾಲಿ;
    io_file->ಗಾತ್ರ_2 = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_from, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_ಗಮ್ಯ)
{
    ಹಿಂತಿರುಗಿ rename(i_from, i_ಗಮ್ಯ) == 0;
}

#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ

#  include <setjmp.h>
#  include <signal.h>

size_t m3_Host_ಪುಟ_ಗಾತ್ರ (ಶೂನ್ಯ)
{
    ದೀರ್ಘ ಗಾತ್ರ_2 = sysconf(_SC_PAGESIZE);

    ಹಿಂತಿರುಗಿ (ಗಾತ್ರ_2 > 0) ? (size_t)ಗಾತ್ರ_2 : 4096;
}

ಶೂನ್ಯ* m3_HostReserve (size_t i_ಬೈಟುಗಳು)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    ಶೂನ್ಯ* ತಳ = mmap(ಖಾಲಿ, i_ಬೈಟುಗಳು, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    ಹಿಂತಿರುಗಿ (ತಳ == MAP_FAILED) ? ಖಾಲಿ : ತಳ;
}

bool m3_HostCommit (ಶೂನ್ಯ* i_ವಿಳಾಸ, size_t i_ಬೈಟುಗಳು)
{
    ಆದರೆ (i_ಬೈಟುಗಳು == 0) {
        ಹಿಂತಿರುಗಿ true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    ಹಿಂತಿರುಗಿ mprotect(i_ವಿಳಾಸ, i_ಬೈಟುಗಳು, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (ಶೂನ್ಯ* i_ವಿಳಾಸ, size_t i_ಬೈಟುಗಳು)
{
    ಆದರೆ (i_ಬೈಟುಗಳು == 0) {
        ಹಿಂತಿರುಗಿ true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    ಶೂನ್ಯ* again = mmap(i_ವಿಳಾಸ, i_ಬೈಟುಗಳು, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    ಹಿಂತಿರುಗಿ again == i_ವಿಳಾಸ;
}

ಶೂನ್ಯ m3_HostRelease (ಶೂನ್ಯ* i_ವಿಳಾಸ, size_t i_ಬೈಟುಗಳು)
{
    munmap(i_ವಿಳಾಸ, i_ಬೈಟುಗಳು);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_Guard_ಚೌಕಟ್ಟು {
    sigjmp_buf           jmp;
    ಸ್ಥಿರಾಂಕ u8*            ಕೆಳಗಿನ;
    size_t               ಬೈಟುಗಳು;
    ರಚನೆ M3_Guard_ಚೌಕಟ್ಟು* ಹಿಂದಿನ;
} M3_Guard_ಚೌಕಟ್ಟು;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಎಣಿಕೆ M3GuardState {
    guards_initial = 0,
    guards_ಸಕ್ರಿಯ,
    guards_unavailable
} M3GuardState;

ಸ್ಥಿರ M3_THREAD_LOCAL M3_Guard_ಚೌಕಟ್ಟು* g_guard_ಚೌಕಟ್ಟು;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ಸ್ಥಿರ ರಚನೆ sigaction              g_ಹಿಂದಿನ_Segv;
ಸ್ಥಿರ ರಚನೆ sigaction              g_ಹಿಂದಿನ_Bus;
ಸ್ಥಿರ M3GuardState                  g_guards;

ಸ್ಥಿರ
ಶೂನ್ಯ guard_signal (ಪೂರ್ಣಾಂಕ i_signal, siginfo_t* i_info, ಶೂನ್ಯ* i_ucontext)
{
    ಸ್ಥಿರಾಂಕ M3_Guard_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟು = g_guard_ಚೌಕಟ್ಟು;

    ಆದರೆ (ಚೌಕಟ್ಟು and i_info) {
        ಸ್ಥಿರಾಂಕ u8* ವಿಳಾಸ = (ಸ್ಥಿರಾಂಕ u8*)i_info->si_addr;

        ಆದರೆ (ವಿಳಾಸ >= ಚೌಕಟ್ಟು->ಕೆಳಗಿನ and ವಿಳಾಸ < ಚೌಕಟ್ಟು->ಕೆಳಗಿನ + ಚೌಕಟ್ಟು->ಬೈಟುಗಳು) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3_Guard_ಚೌಕಟ್ಟು*)ಚೌಕಟ್ಟು)->jmp, 1);
        }
    }

    ಸ್ಥಿರಾಂಕ ರಚನೆ sigaction* ಹಿಂದಿನ = (i_signal == SIGBUS) ? &g_ಹಿಂದಿನ_Bus : &g_ಹಿಂದಿನ_Segv;

    ಆದರೆ ((ಹಿಂದಿನ->sa_flags & SA_SIGINFO) and ಹಿಂದಿನ->sa_sigaction) {
        ಹಿಂದಿನ->sa_sigaction(i_signal, i_info, i_ucontext);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಹಿಂದಿನ->sa_handler != SIG_DFL and ಹಿಂದಿನ->sa_handler != SIG_IGN) {
        ಹಿಂದಿನ->sa_handler(i_signal);
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        ರಚನೆ sigaction dfl;
        memset(&dfl, 0, ಗಾತ್ರ(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, ಖಾಲಿ);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
ಸ್ಥಿರ
bool guard_handler_took (ಪೂರ್ಣಾಂಕ i_signal)
{
    ರಚನೆ sigaction installed;
    memset(&installed, 0, ಗಾತ್ರ(installed));

    ಆದರೆ (sigaction(i_signal, ಖಾಲಿ, &installed) != 0) {
        ಹಿಂತಿರುಗಿ false;
    }

    ಹಿಂತಿರುಗಿ (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ಸ್ಥಿರ
bool install_guard_handlers (ಶೂನ್ಯ)
{
    ಆದರೆ (g_guards != guards_initial) {
        ಹಿಂತಿರುಗಿ g_guards == guards_ಸಕ್ರಿಯ;
    }

    ರಚನೆ sigaction action;
    memset(&action, 0, ಗಾತ್ರ(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &g_ಹಿಂದಿನ_Segv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_ಹಿಂದಿನ_Bus);

    g_guards = (guard_handler_took(SIGSEGV) and guard_handler_took(SIGBUS))
                 ? guards_ಸಕ್ರಿಯ
                 : guards_unavailable;

    ಹಿಂತಿರುಗಿ g_guards == guards_ಸಕ್ರಿಯ;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ಸ್ಥಿರ
ಶೂನ್ಯ install_alt_ರಾಶಿ (ಶೂನ್ಯ)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ಸ್ಥಿರ M3_THREAD_LOCAL u8   ರಾಶಿ[64 * 1024];
    ಸ್ಥಿರ M3_THREAD_LOCAL bool installed;

    ಆದರೆ (installed) {
        ಹಿಂತಿರುಗಿ;
    }
    installed = true;

    stack_t current;
    ಆದರೆ (sigaltstack(ಖಾಲಿ, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        ಹಿಂತಿರುಗಿ;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = ರಾಶಿ;
    alt.ss_size = ಗಾತ್ರ(ರಾಶಿ);
    alt.ss_flags = 0;

    sigaltstack(&alt, ಖಾಲಿ);
}

bool m3_Host_Guards_ಸಕ್ರಿಯ (ಶೂನ್ಯ)
{
    ಹಿಂತಿರುಗಿ install_guard_handlers();
}

bool m3_HostProtectedCall (ಶೂನ್ಯ (*i_body)(ಶೂನ್ಯ*), ಶೂನ್ಯ* i_context,
                           ಶೂನ್ಯ* i_guard_ಕೆಳಗಿನ, size_t i_guard_ಬೈಟುಗಳು)
{
    install_guard_handlers();
    install_alt_ರಾಶಿ();

    M3_Guard_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;
    ಚೌಕಟ್ಟು.ಕೆಳಗಿನ = (ಸ್ಥಿರಾಂಕ u8*)i_guard_ಕೆಳಗಿನ;
    ಚೌಕಟ್ಟು.ಬೈಟುಗಳು = i_guard_ಬೈಟುಗಳು;
    ಚೌಕಟ್ಟು.ಹಿಂದಿನ = g_guard_ಚೌಕಟ್ಟು;

    g_guard_ಚೌಕಟ್ಟು = &ಚೌಕಟ್ಟು;

    bool ಪೂರ್ಣಗೊಂಡ;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    ಆದರೆ (sigsetjmp(ಚೌಕಟ್ಟು.jmp, 0) == 0) {
        i_body(i_context);
        ಪೂರ್ಣಗೊಂಡ = true;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಪೂರ್ಣಗೊಂಡ = false;
    }

    g_guard_ಚೌಕಟ್ಟು = ಚೌಕಟ್ಟು.ಹಿಂದಿನ;

    ಹಿಂತಿರುಗಿ ಪೂರ್ಣಗೊಂಡ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3GuardedMemory

#ಆದರೆ_ಅಂತ್ಯ // m3_host_posix_h
