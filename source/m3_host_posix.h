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

#அந்தர்பாவ "m3_core.h"

#எனில் defined(_WIN32)
#  error "This file is not for Windows"
#endif

#அந்தர்பாவ "m3_host.h"

#அந்தர்பாவ <fcntl.h>
#அந்தர்பாவ <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#அந்தர்பாவ <sys/mman.h>
#அந்தர்பாவ <sys/stat.h>
#அந்தர்பாவ <unistd.h>
#அந்தர்பாவ <signal.h>
#அந்தர்பாவ <errno.h>
#அந்தர்பாவ <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3_Has_Thread_அடுக்கு_Probe
#  எனில் defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_அடுக்கு_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    எனில் __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_அடுக்கு_Probe          1
#    இல்லையெனில்
#      define d_m3_Has_Thread_அடுக்கு_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_அடுக்கு_Probe            1    // musl keeps it in libc
#  இல்லையெனில்
#    define d_m3_Has_Thread_அடுக்கு_Probe            0
#  endif
#endif

#எனில் d_m3_Has_Thread_அடுக்கு_Probe
#  include <pthread.h>
#  எனில் defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#எனில் d_m3_Has_Thread_அடுக்கு_Probe && defined(__linux__)

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
நிலையான
வெறுமை* linux_main_அடுக்கு_base (வெறுமை)
{
    எனில் ((long)syscall(SYS_gettid) != (long)getpid()) {
        திரும்பு வெற்று;
    }

    கட்டமைப்பு rlimit limit;

    எனில் (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        திரும்பு வெற்று;
    }

    முழு fd = open("/proc/self/stat", O_RDONLY);

    எனில் (fd < 0) {
        திரும்பு வெற்று;
    }

    எழுத்து    வரி_2[512];
    ssize_t got = பட(fd, வரி_2, அளவு(வரி_2) - 1);

    close(fd);

    எனில் (got <= 0) {
        திரும்பு வெற்று;
    }

    வரி_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    மாறிலி எழுத்து* field = strrchr(வரி_2, ')');

    க்கு (முழு எண் = 2; field and எண் < 28; ++எண்) {
        field = strchr(field + 1, ' ');
    }

    எனில் (field == வெற்று) {
        திரும்பு வெற்று;
    }

    unsigned long long உச்சி = strtoull(field + 1, வெற்று, 10);

    // A limit reaching below nothing is not a stack this can describe
    எனில் (உச்சி == 0 or (unsigned long long) limit.rlim_cur > உச்சி) {
        திரும்பு வெற்று;
    }

    திரும்பு (u8*)(uintptr_t)உச்சி - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
வெறுமை* m3_Host_அடுக்கு_Base (வெறுமை)
{
#எனில் !d_m3_Has_Thread_அடுக்கு_Probe

    திரும்பு வெற்று;

#அல்லதெனில் defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       உயர்வு = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    எனில் (உயர்வு == வெற்று or size == 0) {
        திரும்பு வெற்று;
    }

    திரும்பு உயர்வு - size;

#இல்லையெனில்

#  எனில் defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_அடுக்கு_base();

    எனில் (main_base) {
        திரும்பு main_base;
    }

#  endif

    pthread_attr_t attr;

    எனில் (pthread_getattr_np(pthread_self(), &attr) != 0) {
        திரும்பு வெற்று;
    }

    வெறுமை*  base = வெற்று;
    size_t size = 0;

    எனில் (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = வெற்று;
    }

    pthread_attr_destroy(&attr);

    திரும்பு base;

#endif
}

நிலையான IM3Runtime       s_posixSuspendRuntime = வெற்று;
நிலையான கட்டமைப்பு sigaction s_oldSigTstp;
நிலையான கட்டமைப்பு sigaction s_oldSigInt;
நிலையான bool             s_suspendHandlersInstalled = false;

நிலையான
வெறுமை m3_PosixSignalHandler (முழு sig)
{
    (வெறுமை)sig;
    எனில் (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

வெறுமை m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    எனில் (!s_suspendHandlersInstalled) {
        கட்டமைப்பு sigaction sa;
        memset(&sa, 0, அளவு(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

வெறுமை m3_HostRemoveInterruptHandler (வெறுமை)
{
    எனில் (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, வெற்று);
        sigaction(SIGINT, &s_oldSigInt, வெற்று);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = வெற்று;
}

u64 m3_HostTimeMs (வெறுமை)
{
    கட்டமைப்பு timespec now;

    எனில் (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        திரும்பு 0;
    }

    திரும்பு (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

வெறுமை m3_HostSleepNs (u64 i_ns)
{
    கட்டமைப்பு timespec இடது;
    இடது.tv_sec = (time_t)(i_ns / 1000000000u);
    இடது.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    போது (nanosleep(&இடது, &இடது) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (மாறிலி எழுத்து* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    முழு fd = open(i_path, O_RDONLY);
    எனில் (fd < 0) {
        திரும்பு false;
    }

    கட்டமைப்பு stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    எனில் (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        திரும்பு m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    வெறுமை*  base = mmap(வெற்று, size, PROT_READ, MAP_PRIVATE, fd, 0);

    எனில் (base == MAP_FAILED) {
        close(fd);
        திரும்பு m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#எனில் defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (வெறுமை)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    திரும்பு true;
}

வெறுமை m3_HostUnmapFile (M3HostFile* io_file)
{
    எனில் (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        எனில் (io_file->handle != d_m3HostNoHandle) {
            close((முழு)io_file->handle);     // and with it the lock
        }
    } இல்லையெனில் {
        m3_Free(io_file->data);
    }

    io_file->data = வெற்று;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (மாறிலி எழுத்து* i_from, மாறிலி எழுத்து* i_to)
{
    திரும்பு rename(i_from, i_to) == 0;
}

#எனில் d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (வெறுமை)
{
    long size = sysconf(_SC_PAGESIZE);

    திரும்பு (size > 0) ? (size_t)size : 4096;
}

வெறுமை* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    வெறுமை* base = mmap(வெற்று, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    திரும்பு (base == MAP_FAILED) ? வெற்று : base;
}

bool m3_HostCommit (வெறுமை* i_address, size_t i_bytes)
{
    எனில் (i_bytes == 0) {
        திரும்பு true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    திரும்பு mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (வெறுமை* i_address, size_t i_bytes)
{
    எனில் (i_bytes == 0) {
        திரும்பு true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    வெறுமை* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    திரும்பு again == i_address;
}

வெறுமை m3_HostRelease (வெறுமை* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
வகைவரை கட்டமைப்பு M3GuardFrame {
    sigjmp_buf           jmp;
    மாறிலி u8*            கீழ்;
    size_t               bytes;
    கட்டமைப்பு M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
வகைவரை எண்ணிக்கை M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

நிலையான M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
நிலையான கட்டமைப்பு sigaction              g_previousSegv;
நிலையான கட்டமைப்பு sigaction              g_previousBus;
நிலையான M3GuardState                  g_guards;

நிலையான
வெறுமை guard_signal (முழு i_signal, siginfo_t* i_info, வெறுமை* i_ucontext)
{
    மாறிலி M3GuardFrame* frame = g_guardFrame;

    எனில் (frame and i_info) {
        மாறிலி u8* address = (மாறிலி u8*)i_info->si_addr;

        எனில் (address >= frame->கீழ் and address < frame->கீழ் + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    மாறிலி கட்டமைப்பு sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    எனில் ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } இல்லையெனில் எனில் (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } இல்லையெனில் {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        கட்டமைப்பு sigaction dfl;
        memset(&dfl, 0, அளவு(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, வெற்று);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
நிலையான
bool guard_handler_took (முழு i_signal)
{
    கட்டமைப்பு sigaction installed;
    memset(&installed, 0, அளவு(installed));

    எனில் (sigaction(i_signal, வெற்று, &installed) != 0) {
        திரும்பு false;
    }

    திரும்பு (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

நிலையான
bool install_guard_handlers (வெறுமை)
{
    எனில் (g_guards != guards_initial) {
        திரும்பு g_guards == guards_active;
    }

    கட்டமைப்பு sigaction action;
    memset(&action, 0, அளவு(action));

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

    திரும்பு g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
நிலையான
வெறுமை install_alt_அடுக்கு (வெறுமை)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    நிலையான M3_THREAD_LOCAL u8   அடுக்கு[64 * 1024];
    நிலையான M3_THREAD_LOCAL bool installed;

    எனில் (installed) {
        திரும்பு;
    }
    installed = true;

    stack_t current;
    எனில் (sigaltstack(வெற்று, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        திரும்பு;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = அடுக்கு;
    alt.ss_size = அளவு(அடுக்கு);
    alt.ss_flags = 0;

    sigaltstack(&alt, வெற்று);
}

bool m3_HostGuardsActive (வெறுமை)
{
    திரும்பு install_guard_handlers();
}

bool m3_HostProtectedCall (வெறுமை (*i_body)(வெறுமை*), வெறுமை* i_context,
                           வெறுமை* i_guard_கீழ், size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_அடுக்கு();

    M3GuardFrame frame;
    frame.கீழ் = (மாறிலி u8*)i_guard_கீழ்;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    எனில் (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } இல்லையெனில் {
        finished = false;
    }

    g_guardFrame = frame.previous;

    திரும்பு finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
