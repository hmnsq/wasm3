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

#ضمن "m3_core.h"

#ܐܢ defined(_WIN32)
#  error "This file is not for Windows"
#endif

#ضمن "m3_host.h"

#ضمن <fcntl.h>
#ضمن <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#ضمن <sys/mman.h>
#ضمن <sys/stat.h>
#ضمن <unistd.h>
#ضمن <signal.h>
#ضمن <errno.h>
#ضمن <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3HasThreadStackProbe
#  ܐܢ defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3HasThreadStackProbe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    ܐܢ __GLIBC_PREREQ(2, 34)
#      define d_m3HasThreadStackProbe          1
#    ܐܚܪܢܐ
#      define d_m3HasThreadStackProbe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3HasThreadStackProbe            1    // musl keeps it in libc
#  ܐܚܪܢܐ
#    define d_m3HasThreadStackProbe            0
#  endif
#endif

#ܐܢ d_m3HasThreadStackProbe
#  include <pthread.h>
#  ܐܢ defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#ܐܢ d_m3HasThreadStackProbe && defined(__linux__)

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
ساكن
فراغ* linux_main_stack_base (فراغ)
{
    ܐܢ ((long)syscall(SYS_gettid) != (long)getpid()) {
        ܦܢܐ عدم;
    }

    بنية rlimit limit;

    ܐܢ (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        ܦܢܐ عدم;
    }

    صحيح fd = open("/proc/self/stat", O_RDONLY);

    ܐܢ (fd < 0) {
        ܦܢܐ عدم;
    }

    حرف    line[512];
    ssize_t got = اقرأ(fd, line, حجم(line) - 1);

    close(fd);

    ܐܢ (got <= 0) {
        ܦܢܐ عدم;
    }

    line[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ثابت حرف* field = strrchr(line, ')');

    لكل (صحيح number = 2; field and number < 28; ++number) {
        field = strchr(field + 1, ' ');
    }

    ܐܢ (field == عدم) {
        ܦܢܐ عدم;
    }

    unsigned long long top = strtoull(field + 1, عدم, 10);

    // A limit reaching below nothing is not a stack this can describe
    ܐܢ (top == 0 or (unsigned long long) limit.rlim_cur > top) {
        ܦܢܐ عدم;
    }

    ܦܢܐ (u8*)(uintptr_t)top - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
فراغ* m3_HostStackBase (فراغ)
{
#ܐܢ !d_m3HasThreadStackProbe

    ܦܢܐ عدم;

#وإلاإذا defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       high = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    ܐܢ (high == عدم or size == 0) {
        ܦܢܐ عدم;
    }

    ܦܢܐ high - size;

#ܐܚܪܢܐ

#  ܐܢ defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_stack_base();

    ܐܢ (main_base) {
        ܦܢܐ main_base;
    }

#  endif

    pthread_attr_t attr;

    ܐܢ (pthread_getattr_np(pthread_self(), &attr) != 0) {
        ܦܢܐ عدم;
    }

    فراغ*  base = عدم;
    size_t size = 0;

    ܐܢ (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = عدم;
    }

    pthread_attr_destroy(&attr);

    ܦܢܐ base;

#endif
}

ساكن IM3Runtime       s_posixSuspendRuntime = عدم;
ساكن بنية sigaction s_oldSigTstp;
ساكن بنية sigaction s_oldSigInt;
ساكن bool             s_suspendHandlersInstalled = false;

ساكن
فراغ m3_PosixSignalHandler (صحيح sig)
{
    (فراغ)sig;
    ܐܢ (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

فراغ m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    ܐܢ (!s_suspendHandlersInstalled) {
        بنية sigaction sa;
        memset(&sa, 0, حجم(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

فراغ m3_HostRemoveInterruptHandler (فراغ)
{
    ܐܢ (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, عدم);
        sigaction(SIGINT, &s_oldSigInt, عدم);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = عدم;
}

u64 m3_HostTimeMs (فراغ)
{
    بنية timespec now;

    ܐܢ (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        ܦܢܐ 0;
    }

    ܦܢܐ (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

فراغ m3_HostSleepNs (u64 i_ns)
{
    بنية timespec left;
    left.tv_sec = (time_t)(i_ns / 1000000000u);
    left.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    ܟܡܐ (nanosleep(&left, &left) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ثابت حرف* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    صحيح fd = open(i_path, O_RDONLY);
    ܐܢ (fd < 0) {
        ܦܢܐ false;
    }

    بنية stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    ܐܢ (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        ܦܢܐ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    فراغ*  base = mmap(عدم, size, PROT_READ, MAP_PRIVATE, fd, 0);

    ܐܢ (base == MAP_FAILED) {
        close(fd);
        ܦܢܐ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#ܐܢ defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (فراغ)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    ܦܢܐ true;
}

فراغ m3_HostUnmapFile (M3HostFile* io_file)
{
    ܐܢ (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        ܐܢ (io_file->handle != d_m3HostNoHandle) {
            close((صحيح)io_file->handle);     // and with it the lock
        }
    } ܐܚܪܢܐ {
        m3_Free(io_file->data);
    }

    io_file->data = عدم;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_to)
{
    ܦܢܐ rename(i_from, i_to) == 0;
}

#ܐܢ d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (فراغ)
{
    long size = sysconf(_SC_PAGESIZE);

    ܦܢܐ (size > 0) ? (size_t)size : 4096;
}

فراغ* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    فراغ* base = mmap(عدم, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    ܦܢܐ (base == MAP_FAILED) ? عدم : base;
}

bool m3_HostCommit (فراغ* i_address, size_t i_bytes)
{
    ܐܢ (i_bytes == 0) {
        ܦܢܐ true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    ܦܢܐ mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (فراغ* i_address, size_t i_bytes)
{
    ܐܢ (i_bytes == 0) {
        ܦܢܐ true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    فراغ* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    ܦܢܐ again == i_address;
}

فراغ m3_HostRelease (فراغ* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
تعريفنوع بنية M3GuardFrame {
    sigjmp_buf           jmp;
    ثابت u8*            low;
    size_t               bytes;
    بنية M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
تعريفنوع تعداد M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

ساكن M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ساكن بنية sigaction              g_previousSegv;
ساكن بنية sigaction              g_previousBus;
ساكن M3GuardState                  g_guards;

ساكن
فراغ guard_signal (صحيح i_signal, siginfo_t* i_info, فراغ* i_ucontext)
{
    ثابت M3GuardFrame* frame = g_guardFrame;

    ܐܢ (frame and i_info) {
        ثابت u8* address = (ثابت u8*)i_info->si_addr;

        ܐܢ (address >= frame->low and address < frame->low + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    ثابت بنية sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    ܐܢ ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } ܐܚܪܢܐ ܐܢ (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } ܐܚܪܢܐ {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        بنية sigaction dfl;
        memset(&dfl, 0, حجم(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, عدم);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
ساكن
bool guard_handler_took (صحيح i_signal)
{
    بنية sigaction installed;
    memset(&installed, 0, حجم(installed));

    ܐܢ (sigaction(i_signal, عدم, &installed) != 0) {
        ܦܢܐ false;
    }

    ܦܢܐ (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ساكن
bool install_guard_handlers (فراغ)
{
    ܐܢ (g_guards != guards_initial) {
        ܦܢܐ g_guards == guards_active;
    }

    بنية sigaction action;
    memset(&action, 0, حجم(action));

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

    ܦܢܐ g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ساكن
فراغ install_alt_stack (فراغ)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ساكن M3_THREAD_LOCAL u8   stack[64 * 1024];
    ساكن M3_THREAD_LOCAL bool installed;

    ܐܢ (installed) {
        ܦܢܐ;
    }
    installed = true;

    stack_t current;
    ܐܢ (sigaltstack(عدم, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        ܦܢܐ;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = stack;
    alt.ss_size = حجم(stack);
    alt.ss_flags = 0;

    sigaltstack(&alt, عدم);
}

bool m3_HostGuardsActive (فراغ)
{
    ܦܢܐ install_guard_handlers();
}

bool m3_HostProtectedCall (فراغ (*i_body)(فراغ*), فراغ* i_context,
                           فراغ* i_guardLow, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_stack();

    M3GuardFrame frame;
    frame.low = (ثابت u8*)i_guardLow;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    ܐܢ (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } ܐܚܪܢܐ {
        finished = false;
    }

    g_guardFrame = frame.previous;

    ܦܢܐ finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
