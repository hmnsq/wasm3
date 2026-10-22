//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#جيڪڏهن_وصف_نه_ٿيل m3_host_posix_h
#وصف_ڪريو m3_host_posix_h

#ضمن "m3_core.h"

#إذا defined(_WIN32)
#  error "This file is not for Windows"
#جيڪڏهن_پورو

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
#جيڪڏهن_وصف_نه_ٿيل d_m3_Has_Thread_ڍير_Probe
#  إذا defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_ڍير_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    إذا __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_ڍير_Probe          1
#    وإلا
#      define d_m3_Has_Thread_ڍير_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_ڍير_Probe            1    // musl keeps it in libc
#  وإلا
#    define d_m3_Has_Thread_ڍير_Probe            0
#  endif
#جيڪڏهن_پورو

#إذا d_m3_Has_Thread_ڍير_Probe
#  include <pthread.h>
#  إذا defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#جيڪڏهن_پورو

#إذا d_m3_Has_Thread_ڍير_Probe && defined(__linux__)

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
فراغ* linux_main_ڍير_بنياد (فراغ)
{
    إذا ((ڊگهو)syscall(SYS_gettid) != (ڊگهو)getpid()) {
        أرجع عدم;
    }

    بنية rlimit حد_بندي;

    إذا (getrlimit(RLIMIT_STACK, &حد_بندي) != 0 or
        حد_بندي.rlim_cur == RLIM_INFINITY or حد_بندي.rlim_cur == 0) {
        أرجع عدم;
    }

    صحيح fd = open("/proc/self/stat", O_RDONLY);

    إذا (fd < 0) {
        أرجع عدم;
    }

    حرف    سٽ_2[512];
    ssize_t got = اقرأ(fd, سٽ_2, حجم(سٽ_2) - 1);

    close(fd);

    إذا (got <= 0) {
        أرجع عدم;
    }

    سٽ_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ثابت حرف* ميدان = strrchr(سٽ_2, ')');

    لكل (صحيح عدد = 2; ميدان and عدد < 28; ++عدد) {
        ميدان = strchr(ميدان + 1, ' ');
    }

    إذا (ميدان == عدم) {
        أرجع عدم;
    }

    بنانشان ڊگهو ڊگهو چوٽي = strtoull(ميدان + 1, عدم, 10);

    // A limit reaching below nothing is not a stack this can describe
    إذا (چوٽي == 0 or (بنانشان ڊگهو ڊگهو) حد_بندي.rlim_cur > چوٽي) {
        أرجع عدم;
    }

    أرجع (u8*)(uintptr_t)چوٽي - (size_t)حد_بندي.rlim_cur;
}

#جيڪڏهن_پورو

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
فراغ* m3_Host_ڍير_بنياد (فراغ)
{
#إذا !d_m3_Has_Thread_ڍير_Probe

    أرجع عدم;

#وإلاإذا defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       مٿاهون = (u8*)pthread_get_stackaddr_np(self);
    size_t    سائيز = pthread_get_stacksize_np(self);

    إذا (مٿاهون == عدم or سائيز == 0) {
        أرجع عدم;
    }

    أرجع مٿاهون - سائيز;

#وإلا

#  إذا defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_بنياد = (u8*)linux_main_ڍير_بنياد();

    إذا (main_بنياد) {
        أرجع main_بنياد;
    }

#  endif

    pthread_attr_t attr;

    إذا (pthread_getattr_np(pthread_self(), &attr) != 0) {
        أرجع عدم;
    }

    فراغ*  بنياد = عدم;
    size_t سائيز = 0;

    إذا (pthread_attr_getstack(&attr, &بنياد, &سائيز) != 0 or سائيز == 0) {
        بنياد = عدم;
    }

    pthread_attr_destroy(&attr);

    أرجع بنياد;

#جيڪڏهن_پورو
}

ساكن IM3Runtime       s_posixSuspendRuntime = عدم;
ساكن بنية sigaction s_oldSigTstp;
ساكن بنية sigaction s_oldSigInt;
ساكن bool             s_suspendHandlersInstalled = false;

ساكن
فراغ m3_PosixSignalHandler (صحيح sig)
{
    (فراغ)sig;
    إذا (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

فراغ m3_Host_Install_مداخلت_Handler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    إذا (!s_suspendHandlersInstalled) {
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

فراغ m3_Host_Remove_مداخلت_Handler (فراغ)
{
    إذا (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, عدم);
        sigaction(SIGINT, &s_oldSigInt, عدم);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = عدم;
}

u64 m3_Host_وقت_Ms (فراغ)
{
    بنية timespec now;

    إذا (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        أرجع 0;
    }

    أرجع (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

فراغ m3_HostSleepNs (u64 i_ns)
{
    بنية timespec کاٻو;
    کاٻو.tv_sec = (time_t)(i_ns / 1000000000u);
    کاٻو.tv_nsec = (ڊگهو)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    طالما (nanosleep(&کاٻو, &کاٻو) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ثابت حرف* i_رستو, size_t i_max_بائيٽون, M3HostFile* o_file)
{
    صحيح fd = open(i_رستو, O_RDONLY);
    إذا (fd < 0) {
        أرجع false;
    }

    بنية stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    إذا (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_max_بائيٽون and (uint64_t) info.st_size > (uint64_t)i_max_بائيٽون)) {
        close(fd);
        أرجع m3_HostReadFile(i_رستو, i_max_بائيٽون, o_file);
    }

    size_t سائيز = (size_t)info.st_size;
    فراغ*  بنياد = mmap(عدم, سائيز, PROT_READ, MAP_PRIVATE, fd, 0);

    إذا (بنياد == MAP_FAILED) {
        close(fd);
        أرجع m3_HostReadFile(i_رستو, i_max_بائيٽون, o_file);
    }

#إذا defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (فراغ)flock(fd, LOCK_SH | LOCK_NB);
#جيڪڏهن_پورو

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->معلومات = بنياد;
    o_file->سائيز = سائيز;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    أرجع true;
}

فراغ m3_HostUnmapFile (M3HostFile* io_file)
{
    إذا (io_file->mapped) {
        munmap(io_file->معلومات, io_file->سائيز);

        إذا (io_file->handle != d_m3HostNoHandle) {
            close((صحيح)io_file->handle);     // and with it the lock
        }
    } وإلا {
        m3_Free(io_file->معلومات);
    }

    io_file->معلومات = عدم;
    io_file->سائيز = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_منزل)
{
    أرجع rename(i_from, i_منزل) == 0;
}

#إذا d_m3_Guarded_يادگيري

#  include <setjmp.h>
#  include <signal.h>

size_t m3_Host_صفحو_سائيز (فراغ)
{
    ڊگهو سائيز = sysconf(_SC_PAGESIZE);

    أرجع (سائيز > 0) ? (size_t)سائيز : 4096;
}

فراغ* m3_HostReserve (size_t i_بائيٽون)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    فراغ* بنياد = mmap(عدم, i_بائيٽون, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    أرجع (بنياد == MAP_FAILED) ? عدم : بنياد;
}

bool m3_HostCommit (فراغ* i_پتو, size_t i_بائيٽون)
{
    إذا (i_بائيٽون == 0) {
        أرجع true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    أرجع mprotect(i_پتو, i_بائيٽون, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (فراغ* i_پتو, size_t i_بائيٽون)
{
    إذا (i_بائيٽون == 0) {
        أرجع true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    فراغ* again = mmap(i_پتو, i_بائيٽون, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    أرجع again == i_پتو;
}

فراغ m3_HostRelease (فراغ* i_پتو, size_t i_بائيٽون)
{
    munmap(i_پتو, i_بائيٽون);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
تعريفنوع بنية M3_Guard_چوکٺو {
    sigjmp_buf           jmp;
    ثابت u8*            هيٺيون;
    size_t               بائيٽون;
    بنية M3_Guard_چوکٺو* اڳيون;
} M3_Guard_چوکٺو;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
تعريفنوع تعداد M3GuardState {
    guards_initial = 0,
    guards_سرگرم,
    guards_unavailable
} M3GuardState;

ساكن M3_THREAD_LOCAL M3_Guard_چوکٺو* g_guard_چوکٺو;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ساكن بنية sigaction              g_اڳيون_Segv;
ساكن بنية sigaction              g_اڳيون_Bus;
ساكن M3GuardState                  g_guards;

ساكن
فراغ guard_signal (صحيح i_signal, siginfo_t* i_info, فراغ* i_ucontext)
{
    ثابت M3_Guard_چوکٺو* چوکٺو = g_guard_چوکٺو;

    إذا (چوکٺو and i_info) {
        ثابت u8* پتو = (ثابت u8*)i_info->si_addr;

        إذا (پتو >= چوکٺو->هيٺيون and پتو < چوکٺو->هيٺيون + چوکٺو->بائيٽون) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3_Guard_چوکٺو*)چوکٺو)->jmp, 1);
        }
    }

    ثابت بنية sigaction* اڳيون = (i_signal == SIGBUS) ? &g_اڳيون_Bus : &g_اڳيون_Segv;

    إذا ((اڳيون->sa_flags & SA_SIGINFO) and اڳيون->sa_sigaction) {
        اڳيون->sa_sigaction(i_signal, i_info, i_ucontext);
    } وإلا إذا (اڳيون->sa_handler != SIG_DFL and اڳيون->sa_handler != SIG_IGN) {
        اڳيون->sa_handler(i_signal);
    } وإلا {
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

    إذا (sigaction(i_signal, عدم, &installed) != 0) {
        أرجع false;
    }

    أرجع (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ساكن
bool install_guard_handlers (فراغ)
{
    إذا (g_guards != guards_initial) {
        أرجع g_guards == guards_سرگرم;
    }

    بنية sigaction action;
    memset(&action, 0, حجم(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &g_اڳيون_Segv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_اڳيون_Bus);

    g_guards = (guard_handler_took(SIGSEGV) and guard_handler_took(SIGBUS))
                 ? guards_سرگرم
                 : guards_unavailable;

    أرجع g_guards == guards_سرگرم;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ساكن
فراغ install_alt_ڍير (فراغ)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ساكن M3_THREAD_LOCAL u8   ڍير[64 * 1024];
    ساكن M3_THREAD_LOCAL bool installed;

    إذا (installed) {
        أرجع;
    }
    installed = true;

    stack_t current;
    إذا (sigaltstack(عدم, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        أرجع;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = ڍير;
    alt.ss_size = حجم(ڍير);
    alt.ss_flags = 0;

    sigaltstack(&alt, عدم);
}

bool m3_Host_Guards_سرگرم (فراغ)
{
    أرجع install_guard_handlers();
}

bool m3_HostProtectedCall (فراغ (*i_body)(فراغ*), فراغ* i_context,
                           فراغ* i_guard_هيٺيون, size_t i_guard_بائيٽون)
{
    install_guard_handlers();
    install_alt_ڍير();

    M3_Guard_چوکٺو چوکٺو;
    چوکٺو.هيٺيون = (ثابت u8*)i_guard_هيٺيون;
    چوکٺو.بائيٽون = i_guard_بائيٽون;
    چوکٺو.اڳيون = g_guard_چوکٺو;

    g_guard_چوکٺو = &چوکٺو;

    bool ختم;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    إذا (sigsetjmp(چوکٺو.jmp, 0) == 0) {
        i_body(i_context);
        ختم = true;
    } وإلا {
        ختم = false;
    }

    g_guard_چوکٺو = چوکٺو.اڳيون;

    أرجع ختم;
}

#جيڪڏهن_پورو // d_m3GuardedMemory

#جيڪڏهن_پورو // m3_host_posix_h
