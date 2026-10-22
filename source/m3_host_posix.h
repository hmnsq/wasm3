//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_host_posix_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_host_posix_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

#ਜੇ defined(_WIN32)
#  error "This file is not for Windows"
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_host.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <fcntl.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/mman.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/stat.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <unistd.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <signal.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <errno.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ d_m3_Has_Thread_ਢੇਰ_Probe
#  ਜੇ defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_ਢੇਰ_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    ਜੇ __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_ਢੇਰ_Probe          1
#    ਨਹੀਂਤਾਂ
#      define d_m3_Has_Thread_ਢੇਰ_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_ਢੇਰ_Probe            1    // musl keeps it in libc
#  ਨਹੀਂਤਾਂ
#    define d_m3_Has_Thread_ਢੇਰ_Probe            0
#  endif
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_Has_Thread_ਢੇਰ_Probe
#  include <pthread.h>
#  ਜੇ defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_Has_Thread_ਢੇਰ_Probe && defined(__linux__)

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
ਸਥਿਰ
ਖਾਲੀ* linux_main_ਢੇਰ_ਅਧਾਰ (ਖਾਲੀ)
{
    ਜੇ ((ਲੰਮਾ)syscall(SYS_gettid) != (ਲੰਮਾ)getpid()) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਬਣਤਰ rlimit ਹੱਦਬੰਦੀ;

    ਜੇ (getrlimit(RLIMIT_STACK, &ਹੱਦਬੰਦੀ) != 0 or
        ਹੱਦਬੰਦੀ.rlim_cur == RLIM_INFINITY or ਹੱਦਬੰਦੀ.rlim_cur == 0) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਪੂਰਨ fd = open("/proc/self/stat", O_RDONLY);

    ਜੇ (fd < 0) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਅੱਖਰ    ਸਤਰ_2[512];
    ssize_t got = ਪਠ(fd, ਸਤਰ_2, ਆਕਾਰ(ਸਤਰ_2) - 1);

    close(fd);

    ਜੇ (got <= 0) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਸਤਰ_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ਅਚਲ ਅੱਖਰ* ਖੇਤਰ = strrchr(ਸਤਰ_2, ')');

    ਲਈ (ਪੂਰਨ ਸੰਖਿਆ = 2; ਖੇਤਰ and ਸੰਖਿਆ < 28; ++ਸੰਖਿਆ) {
        ਖੇਤਰ = strchr(ਖੇਤਰ + 1, ' ');
    }

    ਜੇ (ਖੇਤਰ == ਨਿਰਾ) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ ਸਿਖਰ = strtoull(ਖੇਤਰ + 1, ਨਿਰਾ, 10);

    // A limit reaching below nothing is not a stack this can describe
    ਜੇ (ਸਿਖਰ == 0 or (ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ) ਹੱਦਬੰਦੀ.rlim_cur > ਸਿਖਰ) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਵਾਪਸ (u8*)(uintptr_t)ਸਿਖਰ - (size_t)ਹੱਦਬੰਦੀ.rlim_cur;
}

#ਜੇ_ਅੰਤ

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
ਖਾਲੀ* m3_Host_ਢੇਰ_ਅਧਾਰ (ਖਾਲੀ)
{
#ਜੇ !d_m3_Has_Thread_ਢੇਰ_Probe

    ਵਾਪਸ ਨਿਰਾ;

#ਜਾਂਜੇ defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       ਉੱਚਾ = (u8*)pthread_get_stackaddr_np(self);
    size_t    ਮਾਪ = pthread_get_stacksize_np(self);

    ਜੇ (ਉੱਚਾ == ਨਿਰਾ or ਮਾਪ == 0) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਵਾਪਸ ਉੱਚਾ - ਮਾਪ;

#ਨਹੀਂਤਾਂ

#  ਜੇ defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_ਅਧਾਰ = (u8*)linux_main_ਢੇਰ_ਅਧਾਰ();

    ਜੇ (main_ਅਧਾਰ) {
        ਵਾਪਸ main_ਅਧਾਰ;
    }

#  endif

    pthread_attr_t attr;

    ਜੇ (pthread_getattr_np(pthread_self(), &attr) != 0) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਖਾਲੀ*  ਅਧਾਰ = ਨਿਰਾ;
    size_t ਮਾਪ = 0;

    ਜੇ (pthread_attr_getstack(&attr, &ਅਧਾਰ, &ਮਾਪ) != 0 or ਮਾਪ == 0) {
        ਅਧਾਰ = ਨਿਰਾ;
    }

    pthread_attr_destroy(&attr);

    ਵਾਪਸ ਅਧਾਰ;

#ਜੇ_ਅੰਤ
}

ਸਥਿਰ IM3Runtime       s_posixSuspendRuntime = ਨਿਰਾ;
ਸਥਿਰ ਬਣਤਰ sigaction s_oldSigTstp;
ਸਥਿਰ ਬਣਤਰ sigaction s_oldSigInt;
ਸਥਿਰ bool             s_suspendHandlersInstalled = false;

ਸਥਿਰ
ਖਾਲੀ m3_PosixSignalHandler (ਪੂਰਨ sig)
{
    (ਖਾਲੀ)sig;
    ਜੇ (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

ਖਾਲੀ m3_Host_Install_ਰੁਕਾਵਟ_Handler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    ਜੇ (!s_suspendHandlersInstalled) {
        ਬਣਤਰ sigaction sa;
        memset(&sa, 0, ਆਕਾਰ(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

ਖਾਲੀ m3_Host_Remove_ਰੁਕਾਵਟ_Handler (ਖਾਲੀ)
{
    ਜੇ (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, ਨਿਰਾ);
        sigaction(SIGINT, &s_oldSigInt, ਨਿਰਾ);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = ਨਿਰਾ;
}

u64 m3_Host_ਸਮਾਂ_Ms (ਖਾਲੀ)
{
    ਬਣਤਰ timespec now;

    ਜੇ (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        ਵਾਪਸ 0;
    }

    ਵਾਪਸ (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

ਖਾਲੀ m3_HostSleepNs (u64 i_ns)
{
    ਬਣਤਰ timespec ਖੱਬਾ;
    ਖੱਬਾ.tv_sec = (time_t)(i_ns / 1000000000u);
    ਖੱਬਾ.tv_nsec = (ਲੰਮਾ)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    ਜਦੋਂ (nanosleep(&ਖੱਬਾ, &ਖੱਬਾ) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ਅਚਲ ਅੱਖਰ* i_ਰਸਤਾ, size_t i_max_ਬਾਈਟਾਂ, M3HostFile* o_file)
{
    ਪੂਰਨ fd = open(i_ਰਸਤਾ, O_RDONLY);
    ਜੇ (fd < 0) {
        ਵਾਪਸ false;
    }

    ਬਣਤਰ stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    ਜੇ (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_max_ਬਾਈਟਾਂ and (uint64_t) info.st_size > (uint64_t)i_max_ਬਾਈਟਾਂ)) {
        close(fd);
        ਵਾਪਸ m3_HostReadFile(i_ਰਸਤਾ, i_max_ਬਾਈਟਾਂ, o_file);
    }

    size_t ਮਾਪ = (size_t)info.st_size;
    ਖਾਲੀ*  ਅਧਾਰ = mmap(ਨਿਰਾ, ਮਾਪ, PROT_READ, MAP_PRIVATE, fd, 0);

    ਜੇ (ਅਧਾਰ == MAP_FAILED) {
        close(fd);
        ਵਾਪਸ m3_HostReadFile(i_ਰਸਤਾ, i_max_ਬਾਈਟਾਂ, o_file);
    }

#ਜੇ defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (ਖਾਲੀ)flock(fd, LOCK_SH | LOCK_NB);
#ਜੇ_ਅੰਤ

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->ਅੰਕੜੇ = ਅਧਾਰ;
    o_file->ਮਾਪ = ਮਾਪ;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    ਵਾਪਸ true;
}

ਖਾਲੀ m3_HostUnmapFile (M3HostFile* io_file)
{
    ਜੇ (io_file->mapped) {
        munmap(io_file->ਅੰਕੜੇ, io_file->ਮਾਪ);

        ਜੇ (io_file->handle != d_m3HostNoHandle) {
            close((ਪੂਰਨ)io_file->handle);     // and with it the lock
        }
    } ਨਹੀਂਤਾਂ {
        m3_Free(io_file->ਅੰਕੜੇ);
    }

    io_file->ਅੰਕੜੇ = ਨਿਰਾ;
    io_file->ਮਾਪ = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ਅਚਲ ਅੱਖਰ* i_from, ਅਚਲ ਅੱਖਰ* i_ਮੰਜ਼ਿਲ)
{
    ਵਾਪਸ rename(i_from, i_ਮੰਜ਼ਿਲ) == 0;
}

#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ

#  include <setjmp.h>
#  include <signal.h>

size_t m3_Host_ਪੰਨਾ_ਮਾਪ (ਖਾਲੀ)
{
    ਲੰਮਾ ਮਾਪ = sysconf(_SC_PAGESIZE);

    ਵਾਪਸ (ਮਾਪ > 0) ? (size_t)ਮਾਪ : 4096;
}

ਖਾਲੀ* m3_HostReserve (size_t i_ਬਾਈਟਾਂ)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    ਖਾਲੀ* ਅਧਾਰ = mmap(ਨਿਰਾ, i_ਬਾਈਟਾਂ, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    ਵਾਪਸ (ਅਧਾਰ == MAP_FAILED) ? ਨਿਰਾ : ਅਧਾਰ;
}

bool m3_HostCommit (ਖਾਲੀ* i_ਪਤਾ, size_t i_ਬਾਈਟਾਂ)
{
    ਜੇ (i_ਬਾਈਟਾਂ == 0) {
        ਵਾਪਸ true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    ਵਾਪਸ mprotect(i_ਪਤਾ, i_ਬਾਈਟਾਂ, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (ਖਾਲੀ* i_ਪਤਾ, size_t i_ਬਾਈਟਾਂ)
{
    ਜੇ (i_ਬਾਈਟਾਂ == 0) {
        ਵਾਪਸ true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    ਖਾਲੀ* again = mmap(i_ਪਤਾ, i_ਬਾਈਟਾਂ, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    ਵਾਪਸ again == i_ਪਤਾ;
}

ਖਾਲੀ m3_HostRelease (ਖਾਲੀ* i_ਪਤਾ, size_t i_ਬਾਈਟਾਂ)
{
    munmap(i_ਪਤਾ, i_ਬਾਈਟਾਂ);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_Guard_ਚੌਖਟਾ {
    sigjmp_buf           jmp;
    ਅਚਲ u8*            ਨੀਵਾਂ;
    size_t               ਬਾਈਟਾਂ;
    ਬਣਤਰ M3_Guard_ਚੌਖਟਾ* ਪਿਛਲਾ;
} M3_Guard_ਚੌਖਟਾ;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਗਿਣਤੀ M3GuardState {
    guards_initial = 0,
    guards_ਸਰਗਰਮ,
    guards_unavailable
} M3GuardState;

ਸਥਿਰ M3_THREAD_LOCAL M3_Guard_ਚੌਖਟਾ* g_guard_ਚੌਖਟਾ;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ਸਥਿਰ ਬਣਤਰ sigaction              g_ਪਿਛਲਾ_Segv;
ਸਥਿਰ ਬਣਤਰ sigaction              g_ਪਿਛਲਾ_Bus;
ਸਥਿਰ M3GuardState                  g_guards;

ਸਥਿਰ
ਖਾਲੀ guard_signal (ਪੂਰਨ i_signal, siginfo_t* i_info, ਖਾਲੀ* i_ucontext)
{
    ਅਚਲ M3_Guard_ਚੌਖਟਾ* ਚੌਖਟਾ = g_guard_ਚੌਖਟਾ;

    ਜੇ (ਚੌਖਟਾ and i_info) {
        ਅਚਲ u8* ਪਤਾ = (ਅਚਲ u8*)i_info->si_addr;

        ਜੇ (ਪਤਾ >= ਚੌਖਟਾ->ਨੀਵਾਂ and ਪਤਾ < ਚੌਖਟਾ->ਨੀਵਾਂ + ਚੌਖਟਾ->ਬਾਈਟਾਂ) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3_Guard_ਚੌਖਟਾ*)ਚੌਖਟਾ)->jmp, 1);
        }
    }

    ਅਚਲ ਬਣਤਰ sigaction* ਪਿਛਲਾ = (i_signal == SIGBUS) ? &g_ਪਿਛਲਾ_Bus : &g_ਪਿਛਲਾ_Segv;

    ਜੇ ((ਪਿਛਲਾ->sa_flags & SA_SIGINFO) and ਪਿਛਲਾ->sa_sigaction) {
        ਪਿਛਲਾ->sa_sigaction(i_signal, i_info, i_ucontext);
    } ਨਹੀਂਤਾਂ ਜੇ (ਪਿਛਲਾ->sa_handler != SIG_DFL and ਪਿਛਲਾ->sa_handler != SIG_IGN) {
        ਪਿਛਲਾ->sa_handler(i_signal);
    } ਨਹੀਂਤਾਂ {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        ਬਣਤਰ sigaction dfl;
        memset(&dfl, 0, ਆਕਾਰ(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, ਨਿਰਾ);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
ਸਥਿਰ
bool guard_handler_took (ਪੂਰਨ i_signal)
{
    ਬਣਤਰ sigaction installed;
    memset(&installed, 0, ਆਕਾਰ(installed));

    ਜੇ (sigaction(i_signal, ਨਿਰਾ, &installed) != 0) {
        ਵਾਪਸ false;
    }

    ਵਾਪਸ (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ਸਥਿਰ
bool install_guard_handlers (ਖਾਲੀ)
{
    ਜੇ (g_guards != guards_initial) {
        ਵਾਪਸ g_guards == guards_ਸਰਗਰਮ;
    }

    ਬਣਤਰ sigaction action;
    memset(&action, 0, ਆਕਾਰ(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &g_ਪਿਛਲਾ_Segv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_ਪਿਛਲਾ_Bus);

    g_guards = (guard_handler_took(SIGSEGV) and guard_handler_took(SIGBUS))
                 ? guards_ਸਰਗਰਮ
                 : guards_unavailable;

    ਵਾਪਸ g_guards == guards_ਸਰਗਰਮ;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ਸਥਿਰ
ਖਾਲੀ install_alt_ਢੇਰ (ਖਾਲੀ)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ਸਥਿਰ M3_THREAD_LOCAL u8   ਢੇਰ[64 * 1024];
    ਸਥਿਰ M3_THREAD_LOCAL bool installed;

    ਜੇ (installed) {
        ਵਾਪਸ;
    }
    installed = true;

    stack_t current;
    ਜੇ (sigaltstack(ਨਿਰਾ, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        ਵਾਪਸ;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = ਢੇਰ;
    alt.ss_size = ਆਕਾਰ(ਢੇਰ);
    alt.ss_flags = 0;

    sigaltstack(&alt, ਨਿਰਾ);
}

bool m3_Host_Guards_ਸਰਗਰਮ (ਖਾਲੀ)
{
    ਵਾਪਸ install_guard_handlers();
}

bool m3_HostProtectedCall (ਖਾਲੀ (*i_body)(ਖਾਲੀ*), ਖਾਲੀ* i_context,
                           ਖਾਲੀ* i_guard_ਨੀਵਾਂ, size_t i_guard_ਬਾਈਟਾਂ)
{
    install_guard_handlers();
    install_alt_ਢੇਰ();

    M3_Guard_ਚੌਖਟਾ ਚੌਖਟਾ;
    ਚੌਖਟਾ.ਨੀਵਾਂ = (ਅਚਲ u8*)i_guard_ਨੀਵਾਂ;
    ਚੌਖਟਾ.ਬਾਈਟਾਂ = i_guard_ਬਾਈਟਾਂ;
    ਚੌਖਟਾ.ਪਿਛਲਾ = g_guard_ਚੌਖਟਾ;

    g_guard_ਚੌਖਟਾ = &ਚੌਖਟਾ;

    bool ਮੁਕੰਮਲ;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    ਜੇ (sigsetjmp(ਚੌਖਟਾ.jmp, 0) == 0) {
        i_body(i_context);
        ਮੁਕੰਮਲ = true;
    } ਨਹੀਂਤਾਂ {
        ਮੁਕੰਮਲ = false;
    }

    g_guard_ਚੌਖਟਾ = ਚੌਖਟਾ.ਪਿਛਲਾ;

    ਵਾਪਸ ਮੁਕੰਮਲ;
}

#ਜੇ_ਅੰਤ // d_m3GuardedMemory

#ਜੇ_ਅੰਤ // m3_host_posix_h
