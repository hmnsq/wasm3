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

#അന്തര്ഭാവ "m3_core.h"

#എങ്കിൽ defined(_WIN32)
#  error "This file is not for Windows"
#endif

#അന്തര്ഭാവ "m3_host.h"

#അന്തര്ഭാവ <fcntl.h>
#അന്തര്ഭാവ <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#അന്തര്ഭാവ <sys/mman.h>
#അന്തര്ഭാവ <sys/stat.h>
#അന്തര്ഭാവ <unistd.h>
#അന്തര്ഭാവ <signal.h>
#അന്തര്ഭാവ <errno.h>
#അന്തര്ഭാവ <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3_Has_Thread_അട്ടി_Probe
#  എങ്കിൽ defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_അട്ടി_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    എങ്കിൽ __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_അട്ടി_Probe          1
#    അല്ലെങ്കിൽ
#      define d_m3_Has_Thread_അട്ടി_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_അട്ടി_Probe            1    // musl keeps it in libc
#  അല്ലെങ്കിൽ
#    define d_m3_Has_Thread_അട്ടി_Probe            0
#  endif
#endif

#എങ്കിൽ d_m3_Has_Thread_അട്ടി_Probe
#  include <pthread.h>
#  എങ്കിൽ defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#എങ്കിൽ d_m3_Has_Thread_അട്ടി_Probe && defined(__linux__)

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
സ്ഥിതം
ശൂന്യം* linux_main_അട്ടി_base (ശൂന്യം)
{
    എങ്കിൽ ((long)syscall(SYS_gettid) != (long)getpid()) {
        തിരികെ ശൂന്യ;
    }

    ഘടന rlimit limit;

    എങ്കിൽ (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        തിരികെ ശൂന്യ;
    }

    പൂർണ്ണസംഖ്യ fd = open("/proc/self/stat", O_RDONLY);

    എങ്കിൽ (fd < 0) {
        തിരികെ ശൂന്യ;
    }

    അക്ഷരം    വരി_2[512];
    ssize_t got = പഠ(fd, വരി_2, വലുപ്പം(വരി_2) - 1);

    close(fd);

    എങ്കിൽ (got <= 0) {
        തിരികെ ശൂന്യ;
    }

    വരി_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    സ്ഥിരം അക്ഷരം* field = strrchr(വരി_2, ')');

    വേണ്ടി (പൂർണ്ണസംഖ്യ സംഖ്യ = 2; field and സംഖ്യ < 28; ++സംഖ്യ) {
        field = strchr(field + 1, ' ');
    }

    എങ്കിൽ (field == ശൂന്യ) {
        തിരികെ ശൂന്യ;
    }

    unsigned long long മുകൾ = strtoull(field + 1, ശൂന്യ, 10);

    // A limit reaching below nothing is not a stack this can describe
    എങ്കിൽ (മുകൾ == 0 or (unsigned long long) limit.rlim_cur > മുകൾ) {
        തിരികെ ശൂന്യ;
    }

    തിരികെ (u8*)(uintptr_t)മുകൾ - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
ശൂന്യം* m3_Host_അട്ടി_Base (ശൂന്യം)
{
#എങ്കിൽ !d_m3_Has_Thread_അട്ടി_Probe

    തിരികെ ശൂന്യ;

#അല്ലെങ്കിലെങ്കിൽ defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       മുകളിലെ = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    എങ്കിൽ (മുകളിലെ == ശൂന്യ or size == 0) {
        തിരികെ ശൂന്യ;
    }

    തിരികെ മുകളിലെ - size;

#അല്ലെങ്കിൽ

#  എങ്കിൽ defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_അട്ടി_base();

    എങ്കിൽ (main_base) {
        തിരികെ main_base;
    }

#  endif

    pthread_attr_t attr;

    എങ്കിൽ (pthread_getattr_np(pthread_self(), &attr) != 0) {
        തിരികെ ശൂന്യ;
    }

    ശൂന്യം*  base = ശൂന്യ;
    size_t size = 0;

    എങ്കിൽ (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = ശൂന്യ;
    }

    pthread_attr_destroy(&attr);

    തിരികെ base;

#endif
}

സ്ഥിതം IM3Runtime       s_posixSuspendRuntime = ശൂന്യ;
സ്ഥിതം ഘടന sigaction s_oldSigTstp;
സ്ഥിതം ഘടന sigaction s_oldSigInt;
സ്ഥിതം bool             s_suspendHandlersInstalled = false;

സ്ഥിതം
ശൂന്യം m3_PosixSignalHandler (പൂർണ്ണസംഖ്യ sig)
{
    (ശൂന്യം)sig;
    എങ്കിൽ (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

ശൂന്യം m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    എങ്കിൽ (!s_suspendHandlersInstalled) {
        ഘടന sigaction sa;
        memset(&sa, 0, വലുപ്പം(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

ശൂന്യം m3_HostRemoveInterruptHandler (ശൂന്യം)
{
    എങ്കിൽ (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, ശൂന്യ);
        sigaction(SIGINT, &s_oldSigInt, ശൂന്യ);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = ശൂന്യ;
}

u64 m3_HostTimeMs (ശൂന്യം)
{
    ഘടന timespec now;

    എങ്കിൽ (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        തിരികെ 0;
    }

    തിരികെ (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

ശൂന്യം m3_HostSleepNs (u64 i_ns)
{
    ഘടന timespec ഇടത്;
    ഇടത്.tv_sec = (time_t)(i_ns / 1000000000u);
    ഇടത്.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    ആകുമ്പോൾ (nanosleep(&ഇടത്, &ഇടത്) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (സ്ഥിരം അക്ഷരം* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    പൂർണ്ണസംഖ്യ fd = open(i_path, O_RDONLY);
    എങ്കിൽ (fd < 0) {
        തിരികെ false;
    }

    ഘടന stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    എങ്കിൽ (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        തിരികെ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    ശൂന്യം*  base = mmap(ശൂന്യ, size, PROT_READ, MAP_PRIVATE, fd, 0);

    എങ്കിൽ (base == MAP_FAILED) {
        close(fd);
        തിരികെ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#എങ്കിൽ defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (ശൂന്യം)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    തിരികെ true;
}

ശൂന്യം m3_HostUnmapFile (M3HostFile* io_file)
{
    എങ്കിൽ (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        എങ്കിൽ (io_file->handle != d_m3HostNoHandle) {
            close((പൂർണ്ണസംഖ്യ)io_file->handle);     // and with it the lock
        }
    } അല്ലെങ്കിൽ {
        m3_Free(io_file->data);
    }

    io_file->data = ശൂന്യ;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (സ്ഥിരം അക്ഷരം* i_from, സ്ഥിരം അക്ഷരം* i_to)
{
    തിരികെ rename(i_from, i_to) == 0;
}

#എങ്കിൽ d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (ശൂന്യം)
{
    long size = sysconf(_SC_PAGESIZE);

    തിരികെ (size > 0) ? (size_t)size : 4096;
}

ശൂന്യം* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    ശൂന്യം* base = mmap(ശൂന്യ, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    തിരികെ (base == MAP_FAILED) ? ശൂന്യ : base;
}

bool m3_HostCommit (ശൂന്യം* i_address, size_t i_bytes)
{
    എങ്കിൽ (i_bytes == 0) {
        തിരികെ true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    തിരികെ mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (ശൂന്യം* i_address, size_t i_bytes)
{
    എങ്കിൽ (i_bytes == 0) {
        തിരികെ true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    ശൂന്യം* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    തിരികെ again == i_address;
}

ശൂന്യം m3_HostRelease (ശൂന്യം* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
തരനിർവചനം ഘടന M3GuardFrame {
    sigjmp_buf           jmp;
    സ്ഥിരം u8*            താഴെയുള്ള;
    size_t               bytes;
    ഘടന M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
തരനിർവചനം എണ്ണൽ M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

സ്ഥിതം M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
സ്ഥിതം ഘടന sigaction              g_previousSegv;
സ്ഥിതം ഘടന sigaction              g_previousBus;
സ്ഥിതം M3GuardState                  g_guards;

സ്ഥിതം
ശൂന്യം guard_signal (പൂർണ്ണസംഖ്യ i_signal, siginfo_t* i_info, ശൂന്യം* i_ucontext)
{
    സ്ഥിരം M3GuardFrame* frame = g_guardFrame;

    എങ്കിൽ (frame and i_info) {
        സ്ഥിരം u8* address = (സ്ഥിരം u8*)i_info->si_addr;

        എങ്കിൽ (address >= frame->താഴെയുള്ള and address < frame->താഴെയുള്ള + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    സ്ഥിരം ഘടന sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    എങ്കിൽ ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } അല്ലെങ്കിൽ എങ്കിൽ (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } അല്ലെങ്കിൽ {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        ഘടന sigaction dfl;
        memset(&dfl, 0, വലുപ്പം(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, ശൂന്യ);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
സ്ഥിതം
bool guard_handler_took (പൂർണ്ണസംഖ്യ i_signal)
{
    ഘടന sigaction installed;
    memset(&installed, 0, വലുപ്പം(installed));

    എങ്കിൽ (sigaction(i_signal, ശൂന്യ, &installed) != 0) {
        തിരികെ false;
    }

    തിരികെ (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

സ്ഥിതം
bool install_guard_handlers (ശൂന്യം)
{
    എങ്കിൽ (g_guards != guards_initial) {
        തിരികെ g_guards == guards_active;
    }

    ഘടന sigaction action;
    memset(&action, 0, വലുപ്പം(action));

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

    തിരികെ g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
സ്ഥിതം
ശൂന്യം install_alt_അട്ടി (ശൂന്യം)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    സ്ഥിതം M3_THREAD_LOCAL u8   അട്ടി[64 * 1024];
    സ്ഥിതം M3_THREAD_LOCAL bool installed;

    എങ്കിൽ (installed) {
        തിരികെ;
    }
    installed = true;

    stack_t current;
    എങ്കിൽ (sigaltstack(ശൂന്യ, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        തിരികെ;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = അട്ടി;
    alt.ss_size = വലുപ്പം(അട്ടി);
    alt.ss_flags = 0;

    sigaltstack(&alt, ശൂന്യ);
}

bool m3_HostGuardsActive (ശൂന്യം)
{
    തിരികെ install_guard_handlers();
}

bool m3_HostProtectedCall (ശൂന്യം (*i_body)(ശൂന്യം*), ശൂന്യം* i_context,
                           ശൂന്യം* i_guard_താഴെയുള്ള, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_അട്ടി();

    M3GuardFrame frame;
    frame.താഴെയുള്ള = (സ്ഥിരം u8*)i_guard_താഴെയുള്ള;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    എങ്കിൽ (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } അല്ലെങ്കിൽ {
        finished = false;
    }

    g_guardFrame = frame.previous;

    തിരികെ finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
