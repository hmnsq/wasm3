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

#شامل "m3_core.h"

#اگر defined(_WIN32)
#  error "This file is not for Windows"
#endif

#شامل "m3_host.h"

#شامل <fcntl.h>
#شامل <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#شامل <sys/mman.h>
#شامل <sys/stat.h>
#شامل <unistd.h>
#شامل <signal.h>
#شامل <errno.h>
#شامل <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3_Has_Thread_پشته_Probe
#  اگر defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_پشته_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    اگر __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_پشته_Probe          1
#    وگرنه
#      define d_m3_Has_Thread_پشته_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_پشته_Probe            1    // musl keeps it in libc
#  وگرنه
#    define d_m3_Has_Thread_پشته_Probe            0
#  endif
#endif

#اگر d_m3_Has_Thread_پشته_Probe
#  include <pthread.h>
#  اگر defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#اگر d_m3_Has_Thread_پشته_Probe && defined(__linux__)

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
ایستا
تهی* linux_main_پشته_base (تهی)
{
    اگر ((long)syscall(SYS_gettid) != (long)getpid()) {
        بازگشت پوچ;
    }

    ساختار rlimit limit;

    اگر (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        بازگشت پوچ;
    }

    صحیح fd = open("/proc/self/stat", O_RDONLY);

    اگر (fd < 0) {
        بازگشت پوچ;
    }

    نویسه    سطر_2[512];
    ssize_t got = اقرأ(fd, سطر_2, اندازه(سطر_2) - 1);

    close(fd);

    اگر (got <= 0) {
        بازگشت پوچ;
    }

    سطر_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ثابت نویسه* field = strrchr(سطر_2, ')');

    برای (صحیح عدد = 2; field and عدد < 28; ++عدد) {
        field = strchr(field + 1, ' ');
    }

    اگر (field == پوچ) {
        بازگشت پوچ;
    }

    unsigned long long بالاترین = strtoull(field + 1, پوچ, 10);

    // A limit reaching below nothing is not a stack this can describe
    اگر (بالاترین == 0 or (unsigned long long) limit.rlim_cur > بالاترین) {
        بازگشت پوچ;
    }

    بازگشت (u8*)(uintptr_t)بالاترین - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
تهی* m3_Host_پشته_Base (تهی)
{
#اگر !d_m3_Has_Thread_پشته_Probe

    بازگشت پوچ;

#وگرنهاگر defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       بالا = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    اگر (بالا == پوچ or size == 0) {
        بازگشت پوچ;
    }

    بازگشت بالا - size;

#وگرنه

#  اگر defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_پشته_base();

    اگر (main_base) {
        بازگشت main_base;
    }

#  endif

    pthread_attr_t attr;

    اگر (pthread_getattr_np(pthread_self(), &attr) != 0) {
        بازگشت پوچ;
    }

    تهی*  base = پوچ;
    size_t size = 0;

    اگر (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = پوچ;
    }

    pthread_attr_destroy(&attr);

    بازگشت base;

#endif
}

ایستا IM3Runtime       s_posixSuspendRuntime = پوچ;
ایستا ساختار sigaction s_oldSigTstp;
ایستا ساختار sigaction s_oldSigInt;
ایستا bool             s_suspendHandlersInstalled = false;

ایستا
تهی m3_PosixSignalHandler (صحیح sig)
{
    (تهی)sig;
    اگر (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

تهی m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    اگر (!s_suspendHandlersInstalled) {
        ساختار sigaction sa;
        memset(&sa, 0, اندازه(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

تهی m3_HostRemoveInterruptHandler (تهی)
{
    اگر (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, پوچ);
        sigaction(SIGINT, &s_oldSigInt, پوچ);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = پوچ;
}

u64 m3_HostTimeMs (تهی)
{
    ساختار timespec now;

    اگر (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        بازگشت 0;
    }

    بازگشت (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

تهی m3_HostSleepNs (u64 i_ns)
{
    ساختار timespec چپ;
    چپ.tv_sec = (time_t)(i_ns / 1000000000u);
    چپ.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    وقتی (nanosleep(&چپ, &چپ) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ثابت نویسه* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    صحیح fd = open(i_path, O_RDONLY);
    اگر (fd < 0) {
        بازگشت false;
    }

    ساختار stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    اگر (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        بازگشت m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    تهی*  base = mmap(پوچ, size, PROT_READ, MAP_PRIVATE, fd, 0);

    اگر (base == MAP_FAILED) {
        close(fd);
        بازگشت m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#اگر defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (تهی)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    بازگشت true;
}

تهی m3_HostUnmapFile (M3HostFile* io_file)
{
    اگر (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        اگر (io_file->handle != d_m3HostNoHandle) {
            close((صحیح)io_file->handle);     // and with it the lock
        }
    } وگرنه {
        m3_Free(io_file->data);
    }

    io_file->data = پوچ;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ثابت نویسه* i_from, ثابت نویسه* i_to)
{
    بازگشت rename(i_from, i_to) == 0;
}

#اگر d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (تهی)
{
    long size = sysconf(_SC_PAGESIZE);

    بازگشت (size > 0) ? (size_t)size : 4096;
}

تهی* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    تهی* base = mmap(پوچ, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    بازگشت (base == MAP_FAILED) ? پوچ : base;
}

bool m3_HostCommit (تهی* i_address, size_t i_bytes)
{
    اگر (i_bytes == 0) {
        بازگشت true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    بازگشت mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (تهی* i_address, size_t i_bytes)
{
    اگر (i_bytes == 0) {
        بازگشت true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    تهی* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    بازگشت again == i_address;
}

تهی m3_HostRelease (تهی* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
تعریفنوع ساختار M3GuardFrame {
    sigjmp_buf           jmp;
    ثابت u8*            کم;
    size_t               bytes;
    ساختار M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
تعریفنوع شمارش M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

ایستا M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ایستا ساختار sigaction              g_previousSegv;
ایستا ساختار sigaction              g_previousBus;
ایستا M3GuardState                  g_guards;

ایستا
تهی guard_signal (صحیح i_signal, siginfo_t* i_info, تهی* i_ucontext)
{
    ثابت M3GuardFrame* frame = g_guardFrame;

    اگر (frame and i_info) {
        ثابت u8* address = (ثابت u8*)i_info->si_addr;

        اگر (address >= frame->کم and address < frame->کم + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    ثابت ساختار sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    اگر ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } وگرنه اگر (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } وگرنه {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        ساختار sigaction dfl;
        memset(&dfl, 0, اندازه(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, پوچ);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
ایستا
bool guard_handler_took (صحیح i_signal)
{
    ساختار sigaction installed;
    memset(&installed, 0, اندازه(installed));

    اگر (sigaction(i_signal, پوچ, &installed) != 0) {
        بازگشت false;
    }

    بازگشت (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ایستا
bool install_guard_handlers (تهی)
{
    اگر (g_guards != guards_initial) {
        بازگشت g_guards == guards_active;
    }

    ساختار sigaction action;
    memset(&action, 0, اندازه(action));

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

    بازگشت g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ایستا
تهی install_alt_پشته (تهی)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ایستا M3_THREAD_LOCAL u8   پشته[64 * 1024];
    ایستا M3_THREAD_LOCAL bool installed;

    اگر (installed) {
        بازگشت;
    }
    installed = true;

    stack_t current;
    اگر (sigaltstack(پوچ, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        بازگشت;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = پشته;
    alt.ss_size = اندازه(پشته);
    alt.ss_flags = 0;

    sigaltstack(&alt, پوچ);
}

bool m3_HostGuardsActive (تهی)
{
    بازگشت install_guard_handlers();
}

bool m3_HostProtectedCall (تهی (*i_body)(تهی*), تهی* i_context,
                           تهی* i_guard_کم, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_پشته();

    M3GuardFrame frame;
    frame.کم = (ثابت u8*)i_guard_کم;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    اگر (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } وگرنه {
        finished = false;
    }

    g_guardFrame = frame.previous;

    بازگشت finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
