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

#অন্তর্ভাব "m3_core.h"

#যদি defined(_WIN32)
#  error "This file is not for Windows"
#endif

#অন্তর্ভাব "m3_host.h"

#অন্তর্ভাব <fcntl.h>
#অন্তর্ভাব <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#অন্তর্ভাব <sys/mman.h>
#অন্তর্ভাব <sys/stat.h>
#অন্তর্ভাব <unistd.h>
#অন্তর্ভাব <signal.h>
#অন্তর্ভাব <errno.h>
#অন্তর্ভাব <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3_Has_Thread_স্তূপ_Probe
#  যদি defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_স্তূপ_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    যদি __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_স্তূপ_Probe          1
#    অন্যথা
#      define d_m3_Has_Thread_স্তূপ_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_স্তূপ_Probe            1    // musl keeps it in libc
#  অন্যথা
#    define d_m3_Has_Thread_স্তূপ_Probe            0
#  endif
#endif

#যদি d_m3_Has_Thread_স্তূপ_Probe
#  include <pthread.h>
#  যদি defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#যদি d_m3_Has_Thread_স্তূপ_Probe && defined(__linux__)

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
স্থির
শূন্য* linux_main_স্তূপ_base (শূন্য)
{
    যদি ((long)syscall(SYS_gettid) != (long)getpid()) {
        ফেরত ফাঁকা;
    }

    কাঠামো rlimit limit;

    যদি (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        ফেরত ফাঁকা;
    }

    পূর্ণসংখ্যা fd = open("/proc/self/stat", O_RDONLY);

    যদি (fd < 0) {
        ফেরত ফাঁকা;
    }

    অক্ষর    সারি_2[512];
    ssize_t got = পঠ(fd, সারি_2, আকার(সারি_2) - 1);

    close(fd);

    যদি (got <= 0) {
        ফেরত ফাঁকা;
    }

    সারি_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ধ্রুবক অক্ষর* field = strrchr(সারি_2, ')');

    জন্য (পূর্ণসংখ্যা সংখ্যা = 2; field and সংখ্যা < 28; ++সংখ্যা) {
        field = strchr(field + 1, ' ');
    }

    যদি (field == ফাঁকা) {
        ফেরত ফাঁকা;
    }

    unsigned long long শীর্ষ = strtoull(field + 1, ফাঁকা, 10);

    // A limit reaching below nothing is not a stack this can describe
    যদি (শীর্ষ == 0 or (unsigned long long) limit.rlim_cur > শীর্ষ) {
        ফেরত ফাঁকা;
    }

    ফেরত (u8*)(uintptr_t)শীর্ষ - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
শূন্য* m3_Host_স্তূপ_Base (শূন্য)
{
#যদি !d_m3_Has_Thread_স্তূপ_Probe

    ফেরত ফাঁকা;

#নতুবা_যদি defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       উপরের = (u8*)pthread_get_stackaddr_np(self);
    size_t    আকার_2 = pthread_get_stacksize_np(self);

    যদি (উপরের == ফাঁকা or আকার_2 == 0) {
        ফেরত ফাঁকা;
    }

    ফেরত উপরের - আকার_2;

#অন্যথা

#  যদি defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_স্তূপ_base();

    যদি (main_base) {
        ফেরত main_base;
    }

#  endif

    pthread_attr_t attr;

    যদি (pthread_getattr_np(pthread_self(), &attr) != 0) {
        ফেরত ফাঁকা;
    }

    শূন্য*  base = ফাঁকা;
    size_t আকার_2 = 0;

    যদি (pthread_attr_getstack(&attr, &base, &আকার_2) != 0 or আকার_2 == 0) {
        base = ফাঁকা;
    }

    pthread_attr_destroy(&attr);

    ফেরত base;

#endif
}

স্থির IM3Runtime       s_posixSuspendRuntime = ফাঁকা;
স্থির কাঠামো sigaction s_oldSigTstp;
স্থির কাঠামো sigaction s_oldSigInt;
স্থির bool             s_suspendHandlersInstalled = false;

স্থির
শূন্য m3_PosixSignalHandler (পূর্ণসংখ্যা sig)
{
    (শূন্য)sig;
    যদি (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

শূন্য m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    যদি (!s_suspendHandlersInstalled) {
        কাঠামো sigaction sa;
        memset(&sa, 0, আকার(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

শূন্য m3_HostRemoveInterruptHandler (শূন্য)
{
    যদি (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, ফাঁকা);
        sigaction(SIGINT, &s_oldSigInt, ফাঁকা);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = ফাঁকা;
}

u64 m3_HostTimeMs (শূন্য)
{
    কাঠামো timespec now;

    যদি (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        ফেরত 0;
    }

    ফেরত (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

শূন্য m3_HostSleepNs (u64 i_ns)
{
    কাঠামো timespec বাম;
    বাম.tv_sec = (time_t)(i_ns / 1000000000u);
    বাম.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    যখন (nanosleep(&বাম, &বাম) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ধ্রুবক অক্ষর* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    পূর্ণসংখ্যা fd = open(i_path, O_RDONLY);
    যদি (fd < 0) {
        ফেরত false;
    }

    কাঠামো stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    যদি (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        ফেরত m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t আকার_2 = (size_t)info.st_size;
    শূন্য*  base = mmap(ফাঁকা, আকার_2, PROT_READ, MAP_PRIVATE, fd, 0);

    যদি (base == MAP_FAILED) {
        close(fd);
        ফেরত m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#যদি defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (শূন্য)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->তথ্য = base;
    o_file->আকার_2 = আকার_2;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    ফেরত true;
}

শূন্য m3_HostUnmapFile (M3HostFile* io_file)
{
    যদি (io_file->mapped) {
        munmap(io_file->তথ্য, io_file->আকার_2);

        যদি (io_file->handle != d_m3HostNoHandle) {
            close((পূর্ণসংখ্যা)io_file->handle);     // and with it the lock
        }
    } অন্যথা {
        m3_Free(io_file->তথ্য);
    }

    io_file->তথ্য = ফাঁকা;
    io_file->আকার_2 = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ধ্রুবক অক্ষর* i_from, ধ্রুবক অক্ষর* i_to)
{
    ফেরত rename(i_from, i_to) == 0;
}

#যদি d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_Host_Page_আকার (শূন্য)
{
    long আকার_2 = sysconf(_SC_PAGESIZE);

    ফেরত (আকার_2 > 0) ? (size_t)আকার_2 : 4096;
}

শূন্য* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    শূন্য* base = mmap(ফাঁকা, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    ফেরত (base == MAP_FAILED) ? ফাঁকা : base;
}

bool m3_HostCommit (শূন্য* i_address, size_t i_bytes)
{
    যদি (i_bytes == 0) {
        ফেরত true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    ফেরত mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (শূন্য* i_address, size_t i_bytes)
{
    যদি (i_bytes == 0) {
        ফেরত true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    শূন্য* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    ফেরত again == i_address;
}

শূন্য m3_HostRelease (শূন্য* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
প্রকারলেখ কাঠামো M3GuardFrame {
    sigjmp_buf           jmp;
    ধ্রুবক u8*            নিচের;
    size_t               bytes;
    কাঠামো M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
প্রকারলেখ গণনা M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

স্থির M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
স্থির কাঠামো sigaction              g_previousSegv;
স্থির কাঠামো sigaction              g_previousBus;
স্থির M3GuardState                  g_guards;

স্থির
শূন্য guard_signal (পূর্ণসংখ্যা i_signal, siginfo_t* i_info, শূন্য* i_ucontext)
{
    ধ্রুবক M3GuardFrame* frame = g_guardFrame;

    যদি (frame and i_info) {
        ধ্রুবক u8* address = (ধ্রুবক u8*)i_info->si_addr;

        যদি (address >= frame->নিচের and address < frame->নিচের + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    ধ্রুবক কাঠামো sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    যদি ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } অন্যথা যদি (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } অন্যথা {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        কাঠামো sigaction dfl;
        memset(&dfl, 0, আকার(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, ফাঁকা);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
স্থির
bool guard_handler_took (পূর্ণসংখ্যা i_signal)
{
    কাঠামো sigaction installed;
    memset(&installed, 0, আকার(installed));

    যদি (sigaction(i_signal, ফাঁকা, &installed) != 0) {
        ফেরত false;
    }

    ফেরত (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

স্থির
bool install_guard_handlers (শূন্য)
{
    যদি (g_guards != guards_initial) {
        ফেরত g_guards == guards_active;
    }

    কাঠামো sigaction action;
    memset(&action, 0, আকার(action));

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

    ফেরত g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
স্থির
শূন্য install_alt_স্তূপ (শূন্য)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    স্থির M3_THREAD_LOCAL u8   স্তূপ[64 * 1024];
    স্থির M3_THREAD_LOCAL bool installed;

    যদি (installed) {
        ফেরত;
    }
    installed = true;

    stack_t current;
    যদি (sigaltstack(ফাঁকা, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        ফেরত;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = স্তূপ;
    alt.ss_size = আকার(স্তূপ);
    alt.ss_flags = 0;

    sigaltstack(&alt, ফাঁকা);
}

bool m3_HostGuardsActive (শূন্য)
{
    ফেরত install_guard_handlers();
}

bool m3_HostProtectedCall (শূন্য (*i_body)(শূন্য*), শূন্য* i_context,
                           শূন্য* i_guard_নিচের, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_স্তূপ();

    M3GuardFrame frame;
    frame.নিচের = (ধ্রুবক u8*)i_guard_নিচের;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    যদি (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } অন্যথা {
        finished = false;
    }

    g_guardFrame = frame.previous;

    ফেরত finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
