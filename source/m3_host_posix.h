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

#ଅନ୍ତର୍ଭାଵ "m3_core.h"

#ଯଦି defined(_WIN32)
#  error "This file is not for Windows"
#endif

#ଅନ୍ତର୍ଭାଵ "m3_host.h"

#ଅନ୍ତର୍ଭାଵ <fcntl.h>
#ଅନ୍ତର୍ଭାଵ <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#ଅନ୍ତର୍ଭାଵ <sys/mman.h>
#ଅନ୍ତର୍ଭାଵ <sys/stat.h>
#ଅନ୍ତର୍ଭାଵ <unistd.h>
#ଅନ୍ତର୍ଭାଵ <signal.h>
#ଅନ୍ତର୍ଭାଵ <errno.h>
#ଅନ୍ତର୍ଭାଵ <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3_Has_Thread_ଗଦା_Probe
#  ଯଦି defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_ଗଦା_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    ଯଦି __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_ଗଦା_Probe          1
#    ଅନ୍ୟଥା
#      define d_m3_Has_Thread_ଗଦା_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_ଗଦା_Probe            1    // musl keeps it in libc
#  ଅନ୍ୟଥା
#    define d_m3_Has_Thread_ଗଦା_Probe            0
#  endif
#endif

#ଯଦି d_m3_Has_Thread_ଗଦା_Probe
#  include <pthread.h>
#  ଯଦି defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#ଯଦି d_m3_Has_Thread_ଗଦା_Probe && defined(__linux__)

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
ସ୍ଥିର
ଶୂନ୍ୟ* linux_main_ଗଦା_base (ଶୂନ୍ୟ)
{
    ଯଦି ((long)syscall(SYS_gettid) != (long)getpid()) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ସଂରଚନା rlimit limit;

    ଯଦି (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd = open("/proc/self/stat", O_RDONLY);

    ଯଦି (fd < 0) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଅକ୍ଷର    ଧାଡ଼ି_2[512];
    ssize_t got = ପଠ(fd, ଧାଡ଼ି_2, ଆକାର(ଧାଡ଼ି_2) - 1);

    close(fd);

    ଯଦି (got <= 0) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଧାଡ଼ି_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ଅଚଳ ଅକ୍ଷର* field = strrchr(ଧାଡ଼ି_2, ')');

    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା ସଂଖ୍ୟା = 2; field and ସଂଖ୍ୟା < 28; ++ସଂଖ୍ୟା) {
        field = strchr(field + 1, ' ');
    }

    ଯଦି (field == ଖାଲି) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    unsigned long long ଶୀର୍ଷ = strtoull(field + 1, ଖାଲି, 10);

    // A limit reaching below nothing is not a stack this can describe
    ଯଦି (ଶୀର୍ଷ == 0 or (unsigned long long) limit.rlim_cur > ଶୀର୍ଷ) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଫେରନ୍ତୁ (u8*)(uintptr_t)ଶୀର୍ଷ - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
ଶୂନ୍ୟ* m3_Host_ଗଦା_Base (ଶୂନ୍ୟ)
{
#ଯଦି !d_m3_Has_Thread_ଗଦା_Probe

    ଫେରନ୍ତୁ ଖାଲି;

#ନଚେତଯଦି defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       ଉପର = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    ଯଦି (ଉପର == ଖାଲି or size == 0) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଫେରନ୍ତୁ ଉପର - size;

#ଅନ୍ୟଥା

#  ଯଦି defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_ଗଦା_base();

    ଯଦି (main_base) {
        ଫେରନ୍ତୁ main_base;
    }

#  endif

    pthread_attr_t attr;

    ଯଦି (pthread_getattr_np(pthread_self(), &attr) != 0) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଶୂନ୍ୟ*  base = ଖାଲି;
    size_t size = 0;

    ଯଦି (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = ଖାଲି;
    }

    pthread_attr_destroy(&attr);

    ଫେରନ୍ତୁ base;

#endif
}

ସ୍ଥିର IM3Runtime       s_posixSuspendRuntime = ଖାଲି;
ସ୍ଥିର ସଂରଚନା sigaction s_oldSigTstp;
ସ୍ଥିର ସଂରଚନା sigaction s_oldSigInt;
ସ୍ଥିର bool             s_suspendHandlersInstalled = false;

ସ୍ଥିର
ଶୂନ୍ୟ m3_PosixSignalHandler (ପୂର୍ଣ୍ଣସଂଖ୍ୟା sig)
{
    (ଶୂନ୍ୟ)sig;
    ଯଦି (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

ଶୂନ୍ୟ m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    ଯଦି (!s_suspendHandlersInstalled) {
        ସଂରଚନା sigaction sa;
        memset(&sa, 0, ଆକାର(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

ଶୂନ୍ୟ m3_HostRemoveInterruptHandler (ଶୂନ୍ୟ)
{
    ଯଦି (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, ଖାଲି);
        sigaction(SIGINT, &s_oldSigInt, ଖାଲି);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = ଖାଲି;
}

u64 m3_HostTimeMs (ଶୂନ୍ୟ)
{
    ସଂରଚନା timespec now;

    ଯଦି (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        ଫେରନ୍ତୁ 0;
    }

    ଫେରନ୍ତୁ (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

ଶୂନ୍ୟ m3_HostSleepNs (u64 i_ns)
{
    ସଂରଚନା timespec ବାମ;
    ବାମ.tv_sec = (time_t)(i_ns / 1000000000u);
    ବାମ.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    ଯେତେବେଳେ (nanosleep(&ବାମ, &ବାମ) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ଅଚଳ ଅକ୍ଷର* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା fd = open(i_path, O_RDONLY);
    ଯଦି (fd < 0) {
        ଫେରନ୍ତୁ false;
    }

    ସଂରଚନା stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    ଯଦି (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        ଫେରନ୍ତୁ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    ଶୂନ୍ୟ*  base = mmap(ଖାଲି, size, PROT_READ, MAP_PRIVATE, fd, 0);

    ଯଦି (base == MAP_FAILED) {
        close(fd);
        ଫେରନ୍ତୁ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#ଯଦି defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (ଶୂନ୍ୟ)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    ଫେରନ୍ତୁ true;
}

ଶୂନ୍ୟ m3_HostUnmapFile (M3HostFile* io_file)
{
    ଯଦି (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        ଯଦି (io_file->handle != d_m3HostNoHandle) {
            close((ପୂର୍ଣ୍ଣସଂଖ୍ୟା)io_file->handle);     // and with it the lock
        }
    } ଅନ୍ୟଥା {
        m3_Free(io_file->data);
    }

    io_file->data = ଖାଲି;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ଅଚଳ ଅକ୍ଷର* i_from, ଅଚଳ ଅକ୍ଷର* i_to)
{
    ଫେରନ୍ତୁ rename(i_from, i_to) == 0;
}

#ଯଦି d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (ଶୂନ୍ୟ)
{
    long size = sysconf(_SC_PAGESIZE);

    ଫେରନ୍ତୁ (size > 0) ? (size_t)size : 4096;
}

ଶୂନ୍ୟ* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    ଶୂନ୍ୟ* base = mmap(ଖାଲି, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    ଫେରନ୍ତୁ (base == MAP_FAILED) ? ଖାଲି : base;
}

bool m3_HostCommit (ଶୂନ୍ୟ* i_address, size_t i_bytes)
{
    ଯଦି (i_bytes == 0) {
        ଫେରନ୍ତୁ true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    ଫେରନ୍ତୁ mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (ଶୂନ୍ୟ* i_address, size_t i_bytes)
{
    ଯଦି (i_bytes == 0) {
        ଫେରନ୍ତୁ true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    ଶୂନ୍ୟ* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    ଫେରନ୍ତୁ again == i_address;
}

ଶୂନ୍ୟ m3_HostRelease (ଶୂନ୍ୟ* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3GuardFrame {
    sigjmp_buf           jmp;
    ଅଚଳ u8*            ତଳ;
    size_t               bytes;
    ସଂରଚନା M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
ପ୍ରକାରବ୍ୟାଖ୍ୟା ଗଣନା M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

ସ୍ଥିର M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ସ୍ଥିର ସଂରଚନା sigaction              g_previousSegv;
ସ୍ଥିର ସଂରଚନା sigaction              g_previousBus;
ସ୍ଥିର M3GuardState                  g_guards;

ସ୍ଥିର
ଶୂନ୍ୟ guard_signal (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i_signal, siginfo_t* i_info, ଶୂନ୍ୟ* i_ucontext)
{
    ଅଚଳ M3GuardFrame* frame = g_guardFrame;

    ଯଦି (frame and i_info) {
        ଅଚଳ u8* address = (ଅଚଳ u8*)i_info->si_addr;

        ଯଦି (address >= frame->ତଳ and address < frame->ତଳ + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    ଅଚଳ ସଂରଚନା sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    ଯଦି ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } ଅନ୍ୟଥା ଯଦି (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } ଅନ୍ୟଥା {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        ସଂରଚନା sigaction dfl;
        memset(&dfl, 0, ଆକାର(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, ଖାଲି);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
ସ୍ଥିର
bool guard_handler_took (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i_signal)
{
    ସଂରଚନା sigaction installed;
    memset(&installed, 0, ଆକାର(installed));

    ଯଦି (sigaction(i_signal, ଖାଲି, &installed) != 0) {
        ଫେରନ୍ତୁ false;
    }

    ଫେରନ୍ତୁ (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ସ୍ଥିର
bool install_guard_handlers (ଶୂନ୍ୟ)
{
    ଯଦି (g_guards != guards_initial) {
        ଫେରନ୍ତୁ g_guards == guards_active;
    }

    ସଂରଚନା sigaction action;
    memset(&action, 0, ଆକାର(action));

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

    ଫେରନ୍ତୁ g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ସ୍ଥିର
ଶୂନ୍ୟ install_alt_ଗଦା (ଶୂନ୍ୟ)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ସ୍ଥିର M3_THREAD_LOCAL u8   ଗଦା[64 * 1024];
    ସ୍ଥିର M3_THREAD_LOCAL bool installed;

    ଯଦି (installed) {
        ଫେରନ୍ତୁ;
    }
    installed = true;

    stack_t current;
    ଯଦି (sigaltstack(ଖାଲି, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        ଫେରନ୍ତୁ;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = ଗଦା;
    alt.ss_size = ଆକାର(ଗଦା);
    alt.ss_flags = 0;

    sigaltstack(&alt, ଖାଲି);
}

bool m3_HostGuardsActive (ଶୂନ୍ୟ)
{
    ଫେରନ୍ତୁ install_guard_handlers();
}

bool m3_HostProtectedCall (ଶୂନ୍ୟ (*i_body)(ଶୂନ୍ୟ*), ଶୂନ୍ୟ* i_context,
                           ଶୂନ୍ୟ* i_guard_ତଳ, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_ଗଦା();

    M3GuardFrame frame;
    frame.ତଳ = (ଅଚଳ u8*)i_guard_ତଳ;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    ଯଦି (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } ଅନ୍ୟଥା {
        finished = false;
    }

    g_guardFrame = frame.previous;

    ଫେରନ୍ତୁ finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
