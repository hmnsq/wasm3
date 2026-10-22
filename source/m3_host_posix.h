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

#अन्तर्भाव "m3_core.h"

#सचे defined(_WIN32)
#  error "This file is not for Windows"
#endif

#अन्तर्भाव "m3_host.h"

#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#अन्तर्भाव <sys/mman.h>
#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <unistd.h>
#अन्तर्भाव <signal.h>
#अन्तर्भाव <errno.h>
#अन्तर्भाव <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3_Has_Thread_राशिः_Probe
#  सचे defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_राशिः_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    सचे __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_राशिः_Probe          1
#    अञ्ञथा
#      define d_m3_Has_Thread_राशिः_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_राशिः_Probe            1    // musl keeps it in libc
#  अञ्ञथा
#    define d_m3_Has_Thread_राशिः_Probe            0
#  endif
#endif

#सचे d_m3_Has_Thread_राशिः_Probe
#  include <pthread.h>
#  सचे defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#सचे d_m3_Has_Thread_राशिः_Probe && defined(__linux__)

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
स्थिर
शून्य* linux_main_राशिः_base (शून्य)
{
    सचे ((long)syscall(SYS_gettid) != (long)getpid()) {
        पटिनिवत्त रिक्त;
    }

    संरचना rlimit limit;

    सचे (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        पटिनिवत्त रिक्त;
    }

    पूर्णाङ्क fd = open("/proc/self/stat", O_RDONLY);

    सचे (fd < 0) {
        पटिनिवत्त रिक्त;
    }

    वर्ण    पङ्क्तिः_2[512];
    ssize_t got = पठ(fd, पङ्क्तिः_2, परिमाण(पङ्क्तिः_2) - 1);

    close(fd);

    सचे (got <= 0) {
        पटिनिवत्त रिक्त;
    }

    पङ्क्तिः_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ध्रुव वर्ण* field = strrchr(पङ्क्तिः_2, ')');

    अत्थाय (पूर्णाङ्क सङ्ख्या = 2; field and सङ्ख्या < 28; ++सङ्ख्या) {
        field = strchr(field + 1, ' ');
    }

    सचे (field == रिक्त) {
        पटिनिवत्त रिक्त;
    }

    unsigned long long शिखरम् = strtoull(field + 1, रिक्त, 10);

    // A limit reaching below nothing is not a stack this can describe
    सचे (शिखरम् == 0 or (unsigned long long) limit.rlim_cur > शिखरम्) {
        पटिनिवत्त रिक्त;
    }

    पटिनिवत्त (u8*)(uintptr_t)शिखरम् - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
शून्य* m3_Host_राशिः_Base (शून्य)
{
#सचे !d_m3_Has_Thread_राशिः_Probe

    पटिनिवत्त रिक्त;

#नोचेद्यदि defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       उच्चः = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    सचे (उच्चः == रिक्त or size == 0) {
        पटिनिवत्त रिक्त;
    }

    पटिनिवत्त उच्चः - size;

#अञ्ञथा

#  सचे defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_राशिः_base();

    सचे (main_base) {
        पटिनिवत्त main_base;
    }

#  endif

    pthread_attr_t attr;

    सचे (pthread_getattr_np(pthread_self(), &attr) != 0) {
        पटिनिवत्त रिक्त;
    }

    शून्य*  base = रिक्त;
    size_t size = 0;

    सचे (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = रिक्त;
    }

    pthread_attr_destroy(&attr);

    पटिनिवत्त base;

#endif
}

स्थिर IM3Runtime       s_posixSuspendRuntime = रिक्त;
स्थिर संरचना sigaction s_oldSigTstp;
स्थिर संरचना sigaction s_oldSigInt;
स्थिर bool             s_suspendHandlersInstalled = false;

स्थिर
शून्य m3_PosixSignalHandler (पूर्णाङ्क sig)
{
    (शून्य)sig;
    सचे (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

शून्य m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    सचे (!s_suspendHandlersInstalled) {
        संरचना sigaction sa;
        memset(&sa, 0, परिमाण(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

शून्य m3_HostRemoveInterruptHandler (शून्य)
{
    सचे (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, रिक्त);
        sigaction(SIGINT, &s_oldSigInt, रिक्त);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = रिक्त;
}

u64 m3_HostTimeMs (शून्य)
{
    संरचना timespec now;

    सचे (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        पटिनिवत्त 0;
    }

    पटिनिवत्त (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

शून्य m3_HostSleepNs (u64 i_ns)
{
    संरचना timespec वामः;
    वामः.tv_sec = (time_t)(i_ns / 1000000000u);
    वामः.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    याव (nanosleep(&वामः, &वामः) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ध्रुव वर्ण* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    पूर्णाङ्क fd = open(i_path, O_RDONLY);
    सचे (fd < 0) {
        पटिनिवत्त false;
    }

    संरचना stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    सचे (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        पटिनिवत्त m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    शून्य*  base = mmap(रिक्त, size, PROT_READ, MAP_PRIVATE, fd, 0);

    सचे (base == MAP_FAILED) {
        close(fd);
        पटिनिवत्त m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#सचे defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (शून्य)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    पटिनिवत्त true;
}

शून्य m3_HostUnmapFile (M3HostFile* io_file)
{
    सचे (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        सचे (io_file->handle != d_m3HostNoHandle) {
            close((पूर्णाङ्क)io_file->handle);     // and with it the lock
        }
    } अञ्ञथा {
        m3_Free(io_file->data);
    }

    io_file->data = रिक्त;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ध्रुव वर्ण* i_from, ध्रुव वर्ण* i_to)
{
    पटिनिवत्त rename(i_from, i_to) == 0;
}

#सचे d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (शून्य)
{
    long size = sysconf(_SC_PAGESIZE);

    पटिनिवत्त (size > 0) ? (size_t)size : 4096;
}

शून्य* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    शून्य* base = mmap(रिक्त, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    पटिनिवत्त (base == MAP_FAILED) ? रिक्त : base;
}

bool m3_HostCommit (शून्य* i_address, size_t i_bytes)
{
    सचे (i_bytes == 0) {
        पटिनिवत्त true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    पटिनिवत्त mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (शून्य* i_address, size_t i_bytes)
{
    सचे (i_bytes == 0) {
        पटिनिवत्त true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    शून्य* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    पटिनिवत्त again == i_address;
}

शून्य m3_HostRelease (शून्य* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
प्रकारनिर्देश संरचना M3GuardFrame {
    sigjmp_buf           jmp;
    ध्रुव u8*            नीचः;
    size_t               bytes;
    संरचना M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
प्रकारनिर्देश गणना M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

स्थिर M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
स्थिर संरचना sigaction              g_previousSegv;
स्थिर संरचना sigaction              g_previousBus;
स्थिर M3GuardState                  g_guards;

स्थिर
शून्य guard_signal (पूर्णाङ्क i_signal, siginfo_t* i_info, शून्य* i_ucontext)
{
    ध्रुव M3GuardFrame* frame = g_guardFrame;

    सचे (frame and i_info) {
        ध्रुव u8* address = (ध्रुव u8*)i_info->si_addr;

        सचे (address >= frame->नीचः and address < frame->नीचः + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    ध्रुव संरचना sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    सचे ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } अञ्ञथा सचे (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } अञ्ञथा {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        संरचना sigaction dfl;
        memset(&dfl, 0, परिमाण(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, रिक्त);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
स्थिर
bool guard_handler_took (पूर्णाङ्क i_signal)
{
    संरचना sigaction installed;
    memset(&installed, 0, परिमाण(installed));

    सचे (sigaction(i_signal, रिक्त, &installed) != 0) {
        पटिनिवत्त false;
    }

    पटिनिवत्त (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

स्थिर
bool install_guard_handlers (शून्य)
{
    सचे (g_guards != guards_initial) {
        पटिनिवत्त g_guards == guards_active;
    }

    संरचना sigaction action;
    memset(&action, 0, परिमाण(action));

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

    पटिनिवत्त g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
स्थिर
शून्य install_alt_राशिः (शून्य)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    स्थिर M3_THREAD_LOCAL u8   राशिः[64 * 1024];
    स्थिर M3_THREAD_LOCAL bool installed;

    सचे (installed) {
        पटिनिवत्त;
    }
    installed = true;

    stack_t current;
    सचे (sigaltstack(रिक्त, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        पटिनिवत्त;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = राशिः;
    alt.ss_size = परिमाण(राशिः);
    alt.ss_flags = 0;

    sigaltstack(&alt, रिक्त);
}

bool m3_HostGuardsActive (शून्य)
{
    पटिनिवत्त install_guard_handlers();
}

bool m3_HostProtectedCall (शून्य (*i_body)(शून्य*), शून्य* i_context,
                           शून्य* i_guard_नीचः, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_राशिः();

    M3GuardFrame frame;
    frame.नीचः = (ध्रुव u8*)i_guard_नीचः;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    सचे (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } अञ्ञथा {
        finished = false;
    }

    g_guardFrame = frame.previous;

    पटिनिवत्त finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
