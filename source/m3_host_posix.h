//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#اگر_غیر_تعریف_شدہ m3_host_posix_h
#تعریف_کرو m3_host_posix_h

#شامل "m3_core.h"

#اگر defined(_WIN32)
#  error "This file is not for Windows"
#اگر_ختم

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
#اگر_غیر_تعریف_شدہ d_m3_Has_Thread_ڈھیر_Probe
#  اگر defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_ڈھیر_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    اگر __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_ڈھیر_Probe          1
#    ورنہ
#      define d_m3_Has_Thread_ڈھیر_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_ڈھیر_Probe            1    // musl keeps it in libc
#  ورنہ
#    define d_m3_Has_Thread_ڈھیر_Probe            0
#  endif
#اگر_ختم

#اگر d_m3_Has_Thread_ڈھیر_Probe
#  include <pthread.h>
#  اگر defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#اگر_ختم

#اگر d_m3_Has_Thread_ڈھیر_Probe && defined(__linux__)

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
ساکن
خالی* linux_main_ڈھیر_بنیاد (خالی)
{
    اگر ((طویل)syscall(SYS_gettid) != (طویل)getpid()) {
        واپس تہی;
    }

    ساخت rlimit حد_بندی;

    اگر (getrlimit(RLIMIT_STACK, &حد_بندی) != 0 or
        حد_بندی.rlim_cur == RLIM_INFINITY or حد_بندی.rlim_cur == 0) {
        واپس تہی;
    }

    صحیح fd = open("/proc/self/stat", O_RDONLY);

    اگر (fd < 0) {
        واپس تہی;
    }

    حرف    سطر_2[512];
    ssize_t got = اقرأ(fd, سطر_2, حجم(سطر_2) - 1);

    close(fd);

    اگر (got <= 0) {
        واپس تہی;
    }

    سطر_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    ثابت حرف* میدان = strrchr(سطر_2, ')');

    برائے (صحیح نمبر = 2; میدان and نمبر < 28; ++نمبر) {
        میدان = strchr(میدان + 1, ' ');
    }

    اگر (میدان == تہی) {
        واپس تہی;
    }

    غیرعلامتی طویل طویل چوٹی = strtoull(میدان + 1, تہی, 10);

    // A limit reaching below nothing is not a stack this can describe
    اگر (چوٹی == 0 or (غیرعلامتی طویل طویل) حد_بندی.rlim_cur > چوٹی) {
        واپس تہی;
    }

    واپس (u8*)(uintptr_t)چوٹی - (size_t)حد_بندی.rlim_cur;
}

#اگر_ختم

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
خالی* m3_Host_ڈھیر_بنیاد (خالی)
{
#اگر !d_m3_Has_Thread_ڈھیر_Probe

    واپس تہی;

#وگرنہ_اگر defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       بلند = (u8*)pthread_get_stackaddr_np(self);
    size_t    سائز = pthread_get_stacksize_np(self);

    اگر (بلند == تہی or سائز == 0) {
        واپس تہی;
    }

    واپس بلند - سائز;

#ورنہ

#  اگر defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_بنیاد = (u8*)linux_main_ڈھیر_بنیاد();

    اگر (main_بنیاد) {
        واپس main_بنیاد;
    }

#  endif

    pthread_attr_t attr;

    اگر (pthread_getattr_np(pthread_self(), &attr) != 0) {
        واپس تہی;
    }

    خالی*  بنیاد = تہی;
    size_t سائز = 0;

    اگر (pthread_attr_getstack(&attr, &بنیاد, &سائز) != 0 or سائز == 0) {
        بنیاد = تہی;
    }

    pthread_attr_destroy(&attr);

    واپس بنیاد;

#اگر_ختم
}

ساکن IM3Runtime       s_posixSuspendRuntime = تہی;
ساکن ساخت sigaction s_oldSigTstp;
ساکن ساخت sigaction s_oldSigInt;
ساکن bool             s_suspendHandlersInstalled = false;

ساکن
خالی m3_PosixSignalHandler (صحیح sig)
{
    (خالی)sig;
    اگر (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

خالی m3_Host_Install_مداخلت_Handler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    اگر (!s_suspendHandlersInstalled) {
        ساخت sigaction sa;
        memset(&sa, 0, حجم(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

خالی m3_Host_Remove_مداخلت_Handler (خالی)
{
    اگر (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, تہی);
        sigaction(SIGINT, &s_oldSigInt, تہی);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = تہی;
}

u64 m3_Host_وقت_Ms (خالی)
{
    ساخت timespec now;

    اگر (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        واپس 0;
    }

    واپس (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

خالی m3_HostSleepNs (u64 i_ns)
{
    ساخت timespec بایاں;
    بایاں.tv_sec = (time_t)(i_ns / 1000000000u);
    بایاں.tv_nsec = (طویل)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    جب (nanosleep(&بایاں, &بایاں) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (ثابت حرف* i_راستہ, size_t i_max_بائٹیں, M3HostFile* o_file)
{
    صحیح fd = open(i_راستہ, O_RDONLY);
    اگر (fd < 0) {
        واپس false;
    }

    ساخت stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    اگر (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_max_بائٹیں and (uint64_t) info.st_size > (uint64_t)i_max_بائٹیں)) {
        close(fd);
        واپس m3_HostReadFile(i_راستہ, i_max_بائٹیں, o_file);
    }

    size_t سائز = (size_t)info.st_size;
    خالی*  بنیاد = mmap(تہی, سائز, PROT_READ, MAP_PRIVATE, fd, 0);

    اگر (بنیاد == MAP_FAILED) {
        close(fd);
        واپس m3_HostReadFile(i_راستہ, i_max_بائٹیں, o_file);
    }

#اگر defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (خالی)flock(fd, LOCK_SH | LOCK_NB);
#اگر_ختم

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->ڈیٹا = بنیاد;
    o_file->سائز = سائز;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    واپس true;
}

خالی m3_HostUnmapFile (M3HostFile* io_file)
{
    اگر (io_file->mapped) {
        munmap(io_file->ڈیٹا, io_file->سائز);

        اگر (io_file->handle != d_m3HostNoHandle) {
            close((صحیح)io_file->handle);     // and with it the lock
        }
    } ورنہ {
        m3_Free(io_file->ڈیٹا);
    }

    io_file->ڈیٹا = تہی;
    io_file->سائز = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_مقصد)
{
    واپس rename(i_from, i_مقصد) == 0;
}

#اگر d_m3_Guarded_حافظہ

#  include <setjmp.h>
#  include <signal.h>

size_t m3_Host_صفحہ_سائز (خالی)
{
    طویل سائز = sysconf(_SC_PAGESIZE);

    واپس (سائز > 0) ? (size_t)سائز : 4096;
}

خالی* m3_HostReserve (size_t i_بائٹیں)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    خالی* بنیاد = mmap(تہی, i_بائٹیں, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    واپس (بنیاد == MAP_FAILED) ? تہی : بنیاد;
}

bool m3_HostCommit (خالی* i_پتہ, size_t i_بائٹیں)
{
    اگر (i_بائٹیں == 0) {
        واپس true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    واپس mprotect(i_پتہ, i_بائٹیں, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (خالی* i_پتہ, size_t i_بائٹیں)
{
    اگر (i_بائٹیں == 0) {
        واپس true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    خالی* again = mmap(i_پتہ, i_بائٹیں, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    واپس again == i_پتہ;
}

خالی m3_HostRelease (خالی* i_پتہ, size_t i_بائٹیں)
{
    munmap(i_پتہ, i_بائٹیں);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
قسمتعریف ساخت M3_Guard_چوکھٹا {
    sigjmp_buf           jmp;
    ثابت u8*            کم;
    size_t               بائٹیں;
    ساخت M3_Guard_چوکھٹا* پچھلا_2;
} M3_Guard_چوکھٹا;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
قسمتعریف شمار M3GuardState {
    guards_initial = 0,
    guards_فعال,
    guards_unavailable
} M3GuardState;

ساکن M3_THREAD_LOCAL M3_Guard_چوکھٹا* g_guard_چوکھٹا;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
ساکن ساخت sigaction              g_پچھلا_Segv;
ساکن ساخت sigaction              g_پچھلا_Bus;
ساکن M3GuardState                  g_guards;

ساکن
خالی guard_signal (صحیح i_signal, siginfo_t* i_info, خالی* i_ucontext)
{
    ثابت M3_Guard_چوکھٹا* چوکھٹا = g_guard_چوکھٹا;

    اگر (چوکھٹا and i_info) {
        ثابت u8* پتہ = (ثابت u8*)i_info->si_addr;

        اگر (پتہ >= چوکھٹا->کم and پتہ < چوکھٹا->کم + چوکھٹا->بائٹیں) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3_Guard_چوکھٹا*)چوکھٹا)->jmp, 1);
        }
    }

    ثابت ساخت sigaction* پچھلا_2 = (i_signal == SIGBUS) ? &g_پچھلا_Bus : &g_پچھلا_Segv;

    اگر ((پچھلا_2->sa_flags & SA_SIGINFO) and پچھلا_2->sa_sigaction) {
        پچھلا_2->sa_sigaction(i_signal, i_info, i_ucontext);
    } ورنہ اگر (پچھلا_2->sa_handler != SIG_DFL and پچھلا_2->sa_handler != SIG_IGN) {
        پچھلا_2->sa_handler(i_signal);
    } ورنہ {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        ساخت sigaction dfl;
        memset(&dfl, 0, حجم(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, تہی);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
ساکن
bool guard_handler_took (صحیح i_signal)
{
    ساخت sigaction installed;
    memset(&installed, 0, حجم(installed));

    اگر (sigaction(i_signal, تہی, &installed) != 0) {
        واپس false;
    }

    واپس (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

ساکن
bool install_guard_handlers (خالی)
{
    اگر (g_guards != guards_initial) {
        واپس g_guards == guards_فعال;
    }

    ساخت sigaction action;
    memset(&action, 0, حجم(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &g_پچھلا_Segv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_پچھلا_Bus);

    g_guards = (guard_handler_took(SIGSEGV) and guard_handler_took(SIGBUS))
                 ? guards_فعال
                 : guards_unavailable;

    واپس g_guards == guards_فعال;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
ساکن
خالی install_alt_ڈھیر (خالی)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    ساکن M3_THREAD_LOCAL u8   ڈھیر[64 * 1024];
    ساکن M3_THREAD_LOCAL bool installed;

    اگر (installed) {
        واپس;
    }
    installed = true;

    stack_t current;
    اگر (sigaltstack(تہی, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        واپس;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = ڈھیر;
    alt.ss_size = حجم(ڈھیر);
    alt.ss_flags = 0;

    sigaltstack(&alt, تہی);
}

bool m3_Host_Guards_فعال (خالی)
{
    واپس install_guard_handlers();
}

bool m3_HostProtectedCall (خالی (*i_body)(خالی*), خالی* i_context,
                           خالی* i_guard_کم, size_t i_guard_بائٹیں)
{
    install_guard_handlers();
    install_alt_ڈھیر();

    M3_Guard_چوکھٹا چوکھٹا;
    چوکھٹا.کم = (ثابت u8*)i_guard_کم;
    چوکھٹا.بائٹیں = i_guard_بائٹیں;
    چوکھٹا.پچھلا_2 = g_guard_چوکھٹا;

    g_guard_چوکھٹا = &چوکھٹا;

    bool ختم_شدہ;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    اگر (sigsetjmp(چوکھٹا.jmp, 0) == 0) {
        i_body(i_context);
        ختم_شدہ = true;
    } ورنہ {
        ختم_شدہ = false;
    }

    g_guard_چوکھٹا = چوکھٹا.پچھلا_2;

    واپس ختم_شدہ;
}

#اگر_ختم // d_m3GuardedMemory

#اگر_ختم // m3_host_posix_h
