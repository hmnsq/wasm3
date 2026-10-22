//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#ifndef m3_host_posix_h
#define m3_host_posix_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಯದಿ defined(_WIN32)
#  error "This file is not for Windows"
#endif

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
#ifndef d_m3_Has_Thread_ರಾಶಿ_Probe
#  ಯದಿ defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_ರಾಶಿ_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    ಯದಿ __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_ರಾಶಿ_Probe          1
#    ಅನ್ಯಥಾ
#      define d_m3_Has_Thread_ರಾಶಿ_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_ರಾಶಿ_Probe            1    // musl keeps it in libc
#  ಅನ್ಯಥಾ
#    define d_m3_Has_Thread_ರಾಶಿ_Probe            0
#  endif
#endif

#ಯದಿ d_m3_Has_Thread_ರಾಶಿ_Probe
#  include <pthread.h>
#  ಯದಿ defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#ಯದಿ d_m3_Has_Thread_ರಾಶಿ_Probe && defined(__linux__)

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
ಶೂನ್ಯ* linux_main_ರಾಶಿ_base (ಶೂನ್ಯ)
{
    ಯದಿ ((long)syscall(SYS_gettid) != (long)getpid()) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಸಂರಚನಾ rlimit limit;

    ಯದಿ (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪೂರ್ಣಾಙ್ಕ fd = open("/proc/self/stat", O_RDONLY);

    ಯದಿ (fd < 0) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ವರ್ಣ    ಸಾಲು_2[512];
    ssize_t got = ಪಠ(fd, ಸಾಲು_2, ಪರಿಮಾಣ(ಸಾಲು_2) - 1);

    close(fd);

    ಯದಿ (got <= 0) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಸಾಲು_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ಧ್ರುವ ವರ್ಣ* field = strrchr(ಸಾಲು_2, ')');

    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ ಸಂಖ್ಯೆ = 2; field and ಸಂಖ್ಯೆ < 28; ++ಸಂಖ್ಯೆ) {
        field = strchr(field + 1, ' ');
    }

    ಯದಿ (field == ರಿಕ್ತ) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    unsigned long long ತುದಿ = strtoull(field + 1, ರಿಕ್ತ, 10);

    // A limit reaching below nothing is not a stack this can describe
    ಯದಿ (ತುದಿ == 0 or (unsigned long long) limit.rlim_cur > ತುದಿ) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪ್ರತ್ಯಾಗಮ (u8*)(uintptr_t)ತುದಿ - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
ಶೂನ್ಯ* m3_Host_ರಾಶಿ_Base (ಶೂನ್ಯ)
{
#ಯದಿ !d_m3_Has_Thread_ರಾಶಿ_Probe

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;

#ನೋಚೇದ್ಯದಿ defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       ಮೇಲಿನ = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    ಯದಿ (ಮೇಲಿನ == ರಿಕ್ತ or size == 0) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪ್ರತ್ಯಾಗಮ ಮೇಲಿನ - size;

#ಅನ್ಯಥಾ

#  ಯದಿ defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_ರಾಶಿ_base();

    ಯದಿ (main_base) {
        ಪ್ರತ್ಯಾಗಮ main_base;
    }

#  endif

    pthread_attr_t attr;

    ಯದಿ (pthread_getattr_np(pthread_self(), &attr) != 0) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಶೂನ್ಯ*  base = ರಿಕ್ತ;
    size_t size = 0;

    ಯದಿ (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = ರಿಕ್ತ;
    }

    pthread_attr_destroy(&attr);

    ಪ್ರತ್ಯಾಗಮ base;

#endif
}

ಸ್ಥಿರ IM3Runtime       s_posixSuspendRuntime = ರಿಕ್ತ;
ಸ್ಥಿರ ಸಂರಚನಾ sigaction s_oldSigTstp;
ಸ್ಥಿರ ಸಂರಚನಾ sigaction s_oldSigInt;
ಸ್ಥಿರ bool             s_suspendHandlersInstalled = false;

ಸ್ಥಿರ
ಶೂನ್ಯ m3_PosixSignalHandler (ಪೂರ್ಣಾಙ್ಕ sig)
{
    (ಶೂನ್ಯ)sig;
    ಯದಿ (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

ಶೂನ್ಯ m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    ಯದಿ (!s_suspendHandlersInstalled) {
        ಸಂರಚನಾ sigaction sa;
        memset(&sa, 0, ಪರಿಮಾಣ(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

ಶೂನ್ಯ m3_HostRemoveInterruptHandler (ಶೂನ್ಯ)
{
    ಯದಿ (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, ರಿಕ್ತ);
        sigaction(SIGINT, &s_oldSigInt, ರಿಕ್ತ);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = ರಿಕ್ತ;
}

u64 m3_HostTimeMs (ಶೂನ್ಯ)
{
    ಸಂರಚನಾ timespec now;

    ಯದಿ (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        ಪ್ರತ್ಯಾಗಮ 0;
    }

    ಪ್ರತ್ಯಾಗಮ (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

ಶೂನ್ಯ m3_HostSleepNs (u64 i_ns)
{
    ಸಂರಚನಾ timespec ಎಡ;
    ಎಡ.tv_sec = (time_t)(i_ns / 1000000000u);
    ಎಡ.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    ಯಾವತ್ (nanosleep(&ಎಡ, &ಎಡ) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ಧ್ರುವ ವರ್ಣ* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    ಪೂರ್ಣಾಙ್ಕ fd = open(i_path, O_RDONLY);
    ಯದಿ (fd < 0) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಸಂರಚನಾ stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    ಯದಿ (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        ಪ್ರತ್ಯಾಗಮ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    ಶೂನ್ಯ*  base = mmap(ರಿಕ್ತ, size, PROT_READ, MAP_PRIVATE, fd, 0);

    ಯದಿ (base == MAP_FAILED) {
        close(fd);
        ಪ್ರತ್ಯಾಗಮ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#ಯದಿ defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (ಶೂನ್ಯ)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    ಪ್ರತ್ಯಾಗಮ true;
}

ಶೂನ್ಯ m3_HostUnmapFile (M3HostFile* io_file)
{
    ಯದಿ (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        ಯದಿ (io_file->handle != d_m3HostNoHandle) {
            close((ಪೂರ್ಣಾಙ್ಕ)io_file->handle);     // and with it the lock
        }
    } ಅನ್ಯಥಾ {
        m3_Free(io_file->data);
    }

    io_file->data = ರಿಕ್ತ;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ಧ್ರುವ ವರ್ಣ* i_from, ಧ್ರುವ ವರ್ಣ* i_to)
{
    ಪ್ರತ್ಯಾಗಮ rename(i_from, i_to) == 0;
}

#ಯದಿ d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (ಶೂನ್ಯ)
{
    long size = sysconf(_SC_PAGESIZE);

    ಪ್ರತ್ಯಾಗಮ (size > 0) ? (size_t)size : 4096;
}

ಶೂನ್ಯ* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    ಶೂನ್ಯ* base = mmap(ರಿಕ್ತ, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    ಪ್ರತ್ಯಾಗಮ (base == MAP_FAILED) ? ರಿಕ್ತ : base;
}

bool m3_HostCommit (ಶೂನ್ಯ* i_address, size_t i_bytes)
{
    ಯದಿ (i_bytes == 0) {
        ಪ್ರತ್ಯಾಗಮ true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    ಪ್ರತ್ಯಾಗಮ mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (ಶೂನ್ಯ* i_address, size_t i_bytes)
{
    ಯದಿ (i_bytes == 0) {
        ಪ್ರತ್ಯಾಗಮ true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    ಶೂನ್ಯ* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    ಪ್ರತ್ಯಾಗಮ again == i_address;
}

ಶೂನ್ಯ m3_HostRelease (ಶೂನ್ಯ* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3GuardFrame {
    sigjmp_buf           jmp;
    ಧ್ರುವ u8*            ಕೆಳಗಿನ;
    size_t               bytes;
    ಸಂರಚನಾ M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
ಪ್ರಕಾರನಿರ್ದೇಶ ಗಣನಾ M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

ಸ್ಥಿರ M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ಸ್ಥಿರ ಸಂರಚನಾ sigaction              g_previousSegv;
ಸ್ಥಿರ ಸಂರಚನಾ sigaction              g_previousBus;
ಸ್ಥಿರ M3GuardState                  g_guards;

ಸ್ಥಿರ
ಶೂನ್ಯ guard_signal (ಪೂರ್ಣಾಙ್ಕ i_signal, siginfo_t* i_info, ಶೂನ್ಯ* i_ucontext)
{
    ಧ್ರುವ M3GuardFrame* frame = g_guardFrame;

    ಯದಿ (frame and i_info) {
        ಧ್ರುವ u8* address = (ಧ್ರುವ u8*)i_info->si_addr;

        ಯದಿ (address >= frame->ಕೆಳಗಿನ and address < frame->ಕೆಳಗಿನ + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    ಧ್ರುವ ಸಂರಚನಾ sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    ಯದಿ ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } ಅನ್ಯಥಾ ಯದಿ (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } ಅನ್ಯಥಾ {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        ಸಂರಚನಾ sigaction dfl;
        memset(&dfl, 0, ಪರಿಮಾಣ(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, ರಿಕ್ತ);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
ಸ್ಥಿರ
bool guard_handler_took (ಪೂರ್ಣಾಙ್ಕ i_signal)
{
    ಸಂರಚನಾ sigaction installed;
    memset(&installed, 0, ಪರಿಮಾಣ(installed));

    ಯದಿ (sigaction(i_signal, ರಿಕ್ತ, &installed) != 0) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಪ್ರತ್ಯಾಗಮ (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ಸ್ಥಿರ
bool install_guard_handlers (ಶೂನ್ಯ)
{
    ಯದಿ (g_guards != guards_initial) {
        ಪ್ರತ್ಯಾಗಮ g_guards == guards_active;
    }

    ಸಂರಚನಾ sigaction action;
    memset(&action, 0, ಪರಿಮಾಣ(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &g_previousSegv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_previousBus);

    g_guards = (guard_handler_took(SIGSEGV) and guard_handler_took(SIGBUS))
                 ? guards_active
                 : guards_unavailable;

    ಪ್ರತ್ಯಾಗಮ g_guards == guards_active;
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

    ಯದಿ (installed) {
        ಪ್ರತ್ಯಾಗಮ;
    }
    installed = true;

    stack_t current;
    ಯದಿ (sigaltstack(ರಿಕ್ತ, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        ಪ್ರತ್ಯಾಗಮ;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = ರಾಶಿ;
    alt.ss_size = ಪರಿಮಾಣ(ರಾಶಿ);
    alt.ss_flags = 0;

    sigaltstack(&alt, ರಿಕ್ತ);
}

bool m3_HostGuardsActive (ಶೂನ್ಯ)
{
    ಪ್ರತ್ಯಾಗಮ install_guard_handlers();
}

bool m3_HostProtectedCall (ಶೂನ್ಯ (*i_body)(ಶೂನ್ಯ*), ಶೂನ್ಯ* i_context,
                           ಶೂನ್ಯ* i_guard_ಕೆಳಗಿನ, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_ರಾಶಿ();

    M3GuardFrame frame;
    frame.ಕೆಳಗಿನ = (ಧ್ರುವ u8*)i_guard_ಕೆಳಗಿನ;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    ಯದಿ (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } ಅನ್ಯಥಾ {
        finished = false;
    }

    g_guardFrame = frame.previous;

    ಪ್ರತ್ಯಾಗಮ finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
