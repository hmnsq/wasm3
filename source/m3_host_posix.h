//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#నిర్వచితం_కాకపోతే m3_host_posix_h
#నిర్వచించు m3_host_posix_h

#అన్తర్భావ "m3_core.h"

#ఒకవేళ defined(_WIN32)
#  error "This file is not for Windows"
#అయితే_ముగింపు

#అన్తర్భావ "m3_host.h"

#అన్తర్భావ <fcntl.h>
#అన్తర్భావ <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#అన్తర్భావ <sys/mman.h>
#అన్తర్భావ <sys/stat.h>
#అన్తర్భావ <unistd.h>
#అన్తర్భావ <signal.h>
#అన్తర్భావ <errno.h>
#అన్తర్భావ <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#నిర్వచితం_కాకపోతే d_m3_Has_Thread_దొంతర_Probe
#  ఒకవేళ defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_దొంతర_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    ఒకవేళ __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_దొంతర_Probe          1
#    లేకపోతే
#      define d_m3_Has_Thread_దొంతర_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_దొంతర_Probe            1    // musl keeps it in libc
#  లేకపోతే
#    define d_m3_Has_Thread_దొంతర_Probe            0
#  endif
#అయితే_ముగింపు

#ఒకవేళ d_m3_Has_Thread_దొంతర_Probe
#  include <pthread.h>
#  ఒకవేళ defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#అయితే_ముగింపు

#ఒకవేళ d_m3_Has_Thread_దొంతర_Probe && defined(__linux__)

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
స్థిర
శూన్య* linux_main_దొంతర_పునాది (శూన్య)
{
    ఒకవేళ ((దీర్ఘ)syscall(SYS_gettid) != (దీర్ఘ)getpid()) {
        తిరిగి ఖాళీ;
    }

    నిర్మాణం rlimit హద్దు;

    ఒకవేళ (getrlimit(RLIMIT_STACK, &హద్దు) != 0 or
        హద్దు.rlim_cur == RLIM_INFINITY or హద్దు.rlim_cur == 0) {
        తిరిగి ఖాళీ;
    }

    పూర్ణాంకం fd = open("/proc/self/stat", O_RDONLY);

    ఒకవేళ (fd < 0) {
        తిరిగి ఖాళీ;
    }

    అక్షరం    పంక్తి_2[512];
    ssize_t got = పఠ(fd, పంక్తి_2, పరిమాణం(పంక్తి_2) - 1);

    close(fd);

    ఒకవేళ (got <= 0) {
        తిరిగి ఖాళీ;
    }

    పంక్తి_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    స్థిరరాశి అక్షరం* క్షేత్రం = strrchr(పంక్తి_2, ')');

    కొరకు (పూర్ణాంకం సంఖ్య = 2; క్షేత్రం and సంఖ్య < 28; ++సంఖ్య) {
        క్షేత్రం = strchr(క్షేత్రం + 1, ' ');
    }

    ఒకవేళ (క్షేత్రం == ఖాళీ) {
        తిరిగి ఖాళీ;
    }

    గుర్తులేని దీర్ఘ దీర్ఘ శిఖరం = strtoull(క్షేత్రం + 1, ఖాళీ, 10);

    // A limit reaching below nothing is not a stack this can describe
    ఒకవేళ (శిఖరం == 0 or (గుర్తులేని దీర్ఘ దీర్ఘ) హద్దు.rlim_cur > శిఖరం) {
        తిరిగి ఖాళీ;
    }

    తిరిగి (u8*)(uintptr_t)శిఖరం - (size_t)హద్దు.rlim_cur;
}

#అయితే_ముగింపు

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
శూన్య* m3_Host_దొంతర_పునాది (శూన్య)
{
#ఒకవేళ !d_m3_Has_Thread_దొంతర_Probe

    తిరిగి ఖాళీ;

#లేదావేళ defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       ఎగువ = (u8*)pthread_get_stackaddr_np(self);
    size_t    కొలత = pthread_get_stacksize_np(self);

    ఒకవేళ (ఎగువ == ఖాళీ or కొలత == 0) {
        తిరిగి ఖాళీ;
    }

    తిరిగి ఎగువ - కొలత;

#లేకపోతే

#  ఒకవేళ defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_పునాది = (u8*)linux_main_దొంతర_పునాది();

    ఒకవేళ (main_పునాది) {
        తిరిగి main_పునాది;
    }

#  endif

    pthread_attr_t attr;

    ఒకవేళ (pthread_getattr_np(pthread_self(), &attr) != 0) {
        తిరిగి ఖాళీ;
    }

    శూన్య*  పునాది = ఖాళీ;
    size_t కొలత = 0;

    ఒకవేళ (pthread_attr_getstack(&attr, &పునాది, &కొలత) != 0 or కొలత == 0) {
        పునాది = ఖాళీ;
    }

    pthread_attr_destroy(&attr);

    తిరిగి పునాది;

#అయితే_ముగింపు
}

స్థిర IM3Runtime       s_posixSuspendRuntime = ఖాళీ;
స్థిర నిర్మాణం sigaction s_oldSigTstp;
స్థిర నిర్మాణం sigaction s_oldSigInt;
స్థిర bool             s_suspendHandlersInstalled = false;

స్థిర
శూన్య m3_PosixSignalHandler (పూర్ణాంకం sig)
{
    (శూన్య)sig;
    ఒకవేళ (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

శూన్య m3_Host_Install_అంతరాయం_Handler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    ఒకవేళ (!s_suspendHandlersInstalled) {
        నిర్మాణం sigaction sa;
        memset(&sa, 0, పరిమాణం(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

శూన్య m3_Host_Remove_అంతరాయం_Handler (శూన్య)
{
    ఒకవేళ (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, ఖాళీ);
        sigaction(SIGINT, &s_oldSigInt, ఖాళీ);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = ఖాళీ;
}

u64 m3_Host_సమయం_Ms (శూన్య)
{
    నిర్మాణం timespec now;

    ఒకవేళ (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        తిరిగి 0;
    }

    తిరిగి (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

శూన్య m3_HostSleepNs (u64 i_ns)
{
    నిర్మాణం timespec ఎడమ;
    ఎడమ.tv_sec = (time_t)(i_ns / 1000000000u);
    ఎడమ.tv_nsec = (దీర్ఘ)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    అయితే (nanosleep(&ఎడమ, &ఎడమ) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (స్థిరరాశి అక్షరం* i_మార్గం, size_t i_max_బైట్లు, M3HostFile* o_file)
{
    పూర్ణాంకం fd = open(i_మార్గం, O_RDONLY);
    ఒకవేళ (fd < 0) {
        తిరిగి false;
    }

    నిర్మాణం stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    ఒకవేళ (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_max_బైట్లు and (uint64_t) info.st_size > (uint64_t)i_max_బైట్లు)) {
        close(fd);
        తిరిగి m3_HostReadFile(i_మార్గం, i_max_బైట్లు, o_file);
    }

    size_t కొలత = (size_t)info.st_size;
    శూన్య*  పునాది = mmap(ఖాళీ, కొలత, PROT_READ, MAP_PRIVATE, fd, 0);

    ఒకవేళ (పునాది == MAP_FAILED) {
        close(fd);
        తిరిగి m3_HostReadFile(i_మార్గం, i_max_బైట్లు, o_file);
    }

#ఒకవేళ defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (శూన్య)flock(fd, LOCK_SH | LOCK_NB);
#అయితే_ముగింపు

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->దత్తాంశం = పునాది;
    o_file->కొలత = కొలత;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    తిరిగి true;
}

శూన్య m3_HostUnmapFile (M3HostFile* io_file)
{
    ఒకవేళ (io_file->mapped) {
        munmap(io_file->దత్తాంశం, io_file->కొలత);

        ఒకవేళ (io_file->handle != d_m3HostNoHandle) {
            close((పూర్ణాంకం)io_file->handle);     // and with it the lock
        }
    } లేకపోతే {
        m3_Free(io_file->దత్తాంశం);
    }

    io_file->దత్తాంశం = ఖాళీ;
    io_file->కొలత = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (స్థిరరాశి అక్షరం* i_from, స్థిరరాశి అక్షరం* i_గమ్యం)
{
    తిరిగి rename(i_from, i_గమ్యం) == 0;
}

#ఒకవేళ d_m3_Guarded_జ్ఞాపకం

#  include <setjmp.h>
#  include <signal.h>

size_t m3_Host_పుట_కొలత (శూన్య)
{
    దీర్ఘ కొలత = sysconf(_SC_PAGESIZE);

    తిరిగి (కొలత > 0) ? (size_t)కొలత : 4096;
}

శూన్య* m3_HostReserve (size_t i_బైట్లు)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    శూన్య* పునాది = mmap(ఖాళీ, i_బైట్లు, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    తిరిగి (పునాది == MAP_FAILED) ? ఖాళీ : పునాది;
}

bool m3_HostCommit (శూన్య* i_చిరునామా, size_t i_బైట్లు)
{
    ఒకవేళ (i_బైట్లు == 0) {
        తిరిగి true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    తిరిగి mprotect(i_చిరునామా, i_బైట్లు, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (శూన్య* i_చిరునామా, size_t i_బైట్లు)
{
    ఒకవేళ (i_బైట్లు == 0) {
        తిరిగి true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    శూన్య* again = mmap(i_చిరునామా, i_బైట్లు, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    తిరిగి again == i_చిరునామా;
}

శూన్య m3_HostRelease (శూన్య* i_చిరునామా, size_t i_బైట్లు)
{
    munmap(i_చిరునామా, i_బైట్లు);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
రకనిర్వచనం నిర్మాణం M3_Guard_చట్రం {
    sigjmp_buf           jmp;
    స్థిరరాశి u8*            దిగువ;
    size_t               బైట్లు;
    నిర్మాణం M3_Guard_చట్రం* మునుపటి;
} M3_Guard_చట్రం;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
రకనిర్వచనం గణన M3GuardState {
    guards_initial = 0,
    guards_చురుకైన,
    guards_unavailable
} M3GuardState;

స్థిర M3_THREAD_LOCAL M3_Guard_చట్రం* g_guard_చట్రం;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
స్థిర నిర్మాణం sigaction              g_మునుపటి_Segv;
స్థిర నిర్మాణం sigaction              g_మునుపటి_Bus;
స్థిర M3GuardState                  g_guards;

స్థిర
శూన్య guard_signal (పూర్ణాంకం i_signal, siginfo_t* i_info, శూన్య* i_ucontext)
{
    స్థిరరాశి M3_Guard_చట్రం* చట్రం = g_guard_చట్రం;

    ఒకవేళ (చట్రం and i_info) {
        స్థిరరాశి u8* చిరునామా = (స్థిరరాశి u8*)i_info->si_addr;

        ఒకవేళ (చిరునామా >= చట్రం->దిగువ and చిరునామా < చట్రం->దిగువ + చట్రం->బైట్లు) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3_Guard_చట్రం*)చట్రం)->jmp, 1);
        }
    }

    స్థిరరాశి నిర్మాణం sigaction* మునుపటి = (i_signal == SIGBUS) ? &g_మునుపటి_Bus : &g_మునుపటి_Segv;

    ఒకవేళ ((మునుపటి->sa_flags & SA_SIGINFO) and మునుపటి->sa_sigaction) {
        మునుపటి->sa_sigaction(i_signal, i_info, i_ucontext);
    } లేకపోతే ఒకవేళ (మునుపటి->sa_handler != SIG_DFL and మునుపటి->sa_handler != SIG_IGN) {
        మునుపటి->sa_handler(i_signal);
    } లేకపోతే {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        నిర్మాణం sigaction dfl;
        memset(&dfl, 0, పరిమాణం(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, ఖాళీ);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
స్థిర
bool guard_handler_took (పూర్ణాంకం i_signal)
{
    నిర్మాణం sigaction installed;
    memset(&installed, 0, పరిమాణం(installed));

    ఒకవేళ (sigaction(i_signal, ఖాళీ, &installed) != 0) {
        తిరిగి false;
    }

    తిరిగి (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

స్థిర
bool install_guard_handlers (శూన్య)
{
    ఒకవేళ (g_guards != guards_initial) {
        తిరిగి g_guards == guards_చురుకైన;
    }

    నిర్మాణం sigaction action;
    memset(&action, 0, పరిమాణం(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &g_మునుపటి_Segv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_మునుపటి_Bus);

    g_guards = (guard_handler_took(SIGSEGV) and guard_handler_took(SIGBUS))
                 ? guards_చురుకైన
                 : guards_unavailable;

    తిరిగి g_guards == guards_చురుకైన;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
స్థిర
శూన్య install_alt_దొంతర (శూన్య)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    స్థిర M3_THREAD_LOCAL u8   దొంతర[64 * 1024];
    స్థిర M3_THREAD_LOCAL bool installed;

    ఒకవేళ (installed) {
        తిరిగి;
    }
    installed = true;

    stack_t current;
    ఒకవేళ (sigaltstack(ఖాళీ, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        తిరిగి;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = దొంతర;
    alt.ss_size = పరిమాణం(దొంతర);
    alt.ss_flags = 0;

    sigaltstack(&alt, ఖాళీ);
}

bool m3_Host_Guards_చురుకైన (శూన్య)
{
    తిరిగి install_guard_handlers();
}

bool m3_HostProtectedCall (శూన్య (*i_body)(శూన్య*), శూన్య* i_context,
                           శూన్య* i_guard_దిగువ, size_t i_guard_బైట్లు)
{
    install_guard_handlers();
    install_alt_దొంతర();

    M3_Guard_చట్రం చట్రం;
    చట్రం.దిగువ = (స్థిరరాశి u8*)i_guard_దిగువ;
    చట్రం.బైట్లు = i_guard_బైట్లు;
    చట్రం.మునుపటి = g_guard_చట్రం;

    g_guard_చట్రం = &చట్రం;

    bool ముగిసిన;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    ఒకవేళ (sigsetjmp(చట్రం.jmp, 0) == 0) {
        i_body(i_context);
        ముగిసిన = true;
    } లేకపోతే {
        ముగిసిన = false;
    }

    g_guard_చట్రం = చట్రం.మునుపటి;

    తిరిగి ముగిసిన;
}

#అయితే_ముగింపు // d_m3GuardedMemory

#అయితే_ముగింపు // m3_host_posix_h
