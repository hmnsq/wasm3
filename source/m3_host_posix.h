//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#اگر_تعریف_نشده m3_host_posix_h
#تعریف_کن m3_host_posix_h

#ضمن "m3_core.h"

#إذا defined(_WIN32)
#  error "This file is not for Windows"
#پایان_اگر

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
#اگر_تعریف_نشده d_m3_Has_Thread_پشته_Probe
#  إذا defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_پشته_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    إذا __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_پشته_Probe          1
#    وإلا
#      define d_m3_Has_Thread_پشته_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_پشته_Probe            1    // musl keeps it in libc
#  وإلا
#    define d_m3_Has_Thread_پشته_Probe            0
#  endif
#پایان_اگر

#إذا d_m3_Has_Thread_پشته_Probe
#  include <pthread.h>
#  إذا defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#پایان_اگر

#إذا d_m3_Has_Thread_پشته_Probe && defined(__linux__)

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
فراغ* linux_main_پشته_پایه (فراغ)
{
    إذا ((دراز)syscall(SYS_gettid) != (دراز)getpid()) {
        أرجع عدم;
    }

    بنية rlimit حد;

    إذا (getrlimit(RLIMIT_STACK, &حد) != 0 or
        حد.rlim_cur == RLIM_INFINITY or حد.rlim_cur == 0) {
        أرجع عدم;
    }

    صحيح fd = open("/proc/self/stat", O_RDONLY);

    إذا (fd < 0) {
        أرجع عدم;
    }

    حرف    سطر_2[512];
    ssize_t got = اقرأ(fd, سطر_2, حجم(سطر_2) - 1);

    close(fd);

    إذا (got <= 0) {
        أرجع عدم;
    }

    سطر_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ثابت حرف* فیلد = strrchr(سطر_2, ')');

    لكل (صحيح عدد = 2; فیلد and عدد < 28; ++عدد) {
        فیلد = strchr(فیلد + 1, ' ');
    }

    إذا (فیلد == عدم) {
        أرجع عدم;
    }

    بیعلامت دراز دراز بالاترین = strtoull(فیلد + 1, عدم, 10);

    // A limit reaching below nothing is not a stack this can describe
    إذا (بالاترین == 0 or (بیعلامت دراز دراز) حد.rlim_cur > بالاترین) {
        أرجع عدم;
    }

    أرجع (u8*)(uintptr_t)بالاترین - (size_t)حد.rlim_cur;
}

#پایان_اگر

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
فراغ* m3_Host_پشته_پایه (فراغ)
{
#إذا !d_m3_Has_Thread_پشته_Probe

    أرجع عدم;

#وإلاإذا defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       بالا = (u8*)pthread_get_stackaddr_np(self);
    size_t    اندازه = pthread_get_stacksize_np(self);

    إذا (بالا == عدم or اندازه == 0) {
        أرجع عدم;
    }

    أرجع بالا - اندازه;

#وإلا

#  إذا defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_پایه = (u8*)linux_main_پشته_پایه();

    إذا (main_پایه) {
        أرجع main_پایه;
    }

#  endif

    pthread_attr_t attr;

    إذا (pthread_getattr_np(pthread_self(), &attr) != 0) {
        أرجع عدم;
    }

    فراغ*  پایه = عدم;
    size_t اندازه = 0;

    إذا (pthread_attr_getstack(&attr, &پایه, &اندازه) != 0 or اندازه == 0) {
        پایه = عدم;
    }

    pthread_attr_destroy(&attr);

    أرجع پایه;

#پایان_اگر
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

فراغ m3_Host_Install_وقفه_Handler (IM3Runtime io_runtime)
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

فراغ m3_Host_Remove_وقفه_Handler (فراغ)
{
    إذا (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, عدم);
        sigaction(SIGINT, &s_oldSigInt, عدم);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = عدم;
}

u64 m3_Host_زمان_Ms (فراغ)
{
    بنية timespec now;

    إذا (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        أرجع 0;
    }

    أرجع (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

فراغ m3_HostSleepNs (u64 i_ns)
{
    بنية timespec چپ;
    چپ.tv_sec = (time_t)(i_ns / 1000000000u);
    چپ.tv_nsec = (دراز)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    طالما (nanosleep(&چپ, &چپ) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ثابت حرف* i_مسیر, size_t i_max_بایتها, M3HostFile* o_file)
{
    صحيح fd = open(i_مسیر, O_RDONLY);
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
        (i_max_بایتها and (uint64_t) info.st_size > (uint64_t)i_max_بایتها)) {
        close(fd);
        أرجع m3_HostReadFile(i_مسیر, i_max_بایتها, o_file);
    }

    size_t اندازه = (size_t)info.st_size;
    فراغ*  پایه = mmap(عدم, اندازه, PROT_READ, MAP_PRIVATE, fd, 0);

    إذا (پایه == MAP_FAILED) {
        close(fd);
        أرجع m3_HostReadFile(i_مسیر, i_max_بایتها, o_file);
    }

#إذا defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (فراغ)flock(fd, LOCK_SH | LOCK_NB);
#پایان_اگر

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->داده = پایه;
    o_file->اندازه = اندازه;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    أرجع true;
}

فراغ m3_HostUnmapFile (M3HostFile* io_file)
{
    إذا (io_file->mapped) {
        munmap(io_file->داده, io_file->اندازه);

        إذا (io_file->handle != d_m3HostNoHandle) {
            close((صحيح)io_file->handle);     // and with it the lock
        }
    } وإلا {
        m3_Free(io_file->داده);
    }

    io_file->داده = عدم;
    io_file->اندازه = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_مقصد)
{
    أرجع rename(i_from, i_مقصد) == 0;
}

#إذا d_m3_Guarded_حافظه

#  include <setjmp.h>
#  include <signal.h>

size_t m3_Host_صفحه_اندازه (فراغ)
{
    دراز اندازه = sysconf(_SC_PAGESIZE);

    أرجع (اندازه > 0) ? (size_t)اندازه : 4096;
}

فراغ* m3_HostReserve (size_t i_بایتها)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    فراغ* پایه = mmap(عدم, i_بایتها, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    أرجع (پایه == MAP_FAILED) ? عدم : پایه;
}

bool m3_HostCommit (فراغ* i_آدرس, size_t i_بایتها)
{
    إذا (i_بایتها == 0) {
        أرجع true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    أرجع mprotect(i_آدرس, i_بایتها, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (فراغ* i_آدرس, size_t i_بایتها)
{
    إذا (i_بایتها == 0) {
        أرجع true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    فراغ* again = mmap(i_آدرس, i_بایتها, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    أرجع again == i_آدرس;
}

فراغ m3_HostRelease (فراغ* i_آدرس, size_t i_بایتها)
{
    munmap(i_آدرس, i_بایتها);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
تعريفنوع بنية M3_Guard_قاب {
    sigjmp_buf           jmp;
    ثابت u8*            کم;
    size_t               بایتها;
    بنية M3_Guard_قاب* قبلی;
} M3_Guard_قاب;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
تعريفنوع تعداد M3GuardState {
    guards_initial = 0,
    guards_فعال,
    guards_unavailable
} M3GuardState;

ساكن M3_THREAD_LOCAL M3_Guard_قاب* g_guard_قاب;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ساكن بنية sigaction              g_قبلی_Segv;
ساكن بنية sigaction              g_قبلی_Bus;
ساكن M3GuardState                  g_guards;

ساكن
فراغ guard_signal (صحيح i_signal, siginfo_t* i_info, فراغ* i_ucontext)
{
    ثابت M3_Guard_قاب* قاب = g_guard_قاب;

    إذا (قاب and i_info) {
        ثابت u8* آدرس = (ثابت u8*)i_info->si_addr;

        إذا (آدرس >= قاب->کم and آدرس < قاب->کم + قاب->بایتها) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3_Guard_قاب*)قاب)->jmp, 1);
        }
    }

    ثابت بنية sigaction* قبلی = (i_signal == SIGBUS) ? &g_قبلی_Bus : &g_قبلی_Segv;

    إذا ((قبلی->sa_flags & SA_SIGINFO) and قبلی->sa_sigaction) {
        قبلی->sa_sigaction(i_signal, i_info, i_ucontext);
    } وإلا إذا (قبلی->sa_handler != SIG_DFL and قبلی->sa_handler != SIG_IGN) {
        قبلی->sa_handler(i_signal);
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
        أرجع g_guards == guards_فعال;
    }

    بنية sigaction action;
    memset(&action, 0, حجم(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &g_قبلی_Segv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_قبلی_Bus);

    g_guards = (guard_handler_took(SIGSEGV) and guard_handler_took(SIGBUS))
                 ? guards_فعال
                 : guards_unavailable;

    أرجع g_guards == guards_فعال;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ساكن
فراغ install_alt_پشته (فراغ)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ساكن M3_THREAD_LOCAL u8   پشته[64 * 1024];
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
    alt.ss_sp = پشته;
    alt.ss_size = حجم(پشته);
    alt.ss_flags = 0;

    sigaltstack(&alt, عدم);
}

bool m3_Host_Guards_فعال (فراغ)
{
    أرجع install_guard_handlers();
}

bool m3_HostProtectedCall (فراغ (*i_body)(فراغ*), فراغ* i_context,
                           فراغ* i_guard_کم, size_t i_guard_بایتها)
{
    install_guard_handlers();
    install_alt_پشته();

    M3_Guard_قاب قاب;
    قاب.کم = (ثابت u8*)i_guard_کم;
    قاب.بایتها = i_guard_بایتها;
    قاب.قبلی = g_guard_قاب;

    g_guard_قاب = &قاب;

    bool پایانیافته;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    إذا (sigsetjmp(قاب.jmp, 0) == 0) {
        i_body(i_context);
        پایانیافته = true;
    } وإلا {
        پایانیافته = false;
    }

    g_guard_قاب = قاب.قبلی;

    أرجع پایانیافته;
}

#پایان_اگر // d_m3GuardedMemory

#پایان_اگر // m3_host_posix_h
