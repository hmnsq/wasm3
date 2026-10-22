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

#जर defined(_WIN32)
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
#ifndef d_m3_Has_Thread_चळत_Probe
#  जर defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_चळत_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    जर __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_चळत_Probe          1
#    अन्यथा
#      define d_m3_Has_Thread_चळत_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_चळत_Probe            1    // musl keeps it in libc
#  अन्यथा
#    define d_m3_Has_Thread_चळत_Probe            0
#  endif
#endif

#जर d_m3_Has_Thread_चळत_Probe
#  include <pthread.h>
#  जर defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#जर d_m3_Has_Thread_चळत_Probe && defined(__linux__)

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
शून्य* linux_main_चळत_base (शून्य)
{
    जर ((दीर्घ)syscall(SYS_gettid) != (दीर्घ)getpid()) {
        परत रिक्त;
    }

    संरचना rlimit limit;

    जर (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        परत रिक्त;
    }

    पूर्णांक fd = खोलो("/proc/self/stat", O_RDONLY);

    जर (fd < 0) {
        परत रिक्त;
    }

    अक्षर    ओळ_2[512];
    ssize_t got = पठ(fd, ओळ_2, आकार(ओळ_2) - 1);

    बंद(fd);

    जर (got <= 0) {
        परत रिक्त;
    }

    ओळ_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    स्थिरांक अक्षर* field = म_खोजप(ओळ_2, ')');

    साठी (पूर्णांक संख्या = 2; field and संख्या < 28; ++संख्या) {
        field = म_अक्षर(field + 1, ' ');
    }

    जर (field == रिक्त) {
        परत रिक्त;
    }

    अचिन्हित दीर्घ दीर्घ शीर्ष = strtoull(field + 1, रिक्त, 10);

    // A limit reaching below nothing is not a stack this can describe
    जर (शीर्ष == 0 or (अचिन्हित दीर्घ दीर्घ) limit.rlim_cur > शीर्ष) {
        परत रिक्त;
    }

    परत (u8*)(uintptr_t)शीर्ष - (माप_प्रकार)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
शून्य* m3_Host_चळत_Base (शून्य)
{
#जर !d_m3_Has_Thread_चळत_Probe

    परत रिक्त;

#किंवाजर defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       वरचा = (u8*)pthread_get_stackaddr_np(self);
    माप_प्रकार    size = pthread_get_stacksize_np(self);

    जर (वरचा == रिक्त or size == 0) {
        परत रिक्त;
    }

    परत वरचा - size;

#अन्यथा

#  जर defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_चळत_base();

    जर (main_base) {
        परत main_base;
    }

#  endif

    pthread_attr_t attr;

    जर (pthread_getattr_np(pthread_self(), &attr) != 0) {
        परत रिक्त;
    }

    शून्य*  base = रिक्त;
    माप_प्रकार size = 0;

    जर (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = रिक्त;
    }

    pthread_attr_destroy(&attr);

    परत base;

#endif
}

स्थिर IM3Runtime       s_posixSuspendRuntime = रिक्त;
स्थिर संरचना sigaction s_oldSigTstp;
स्थिर संरचना sigaction s_oldSigInt;
स्थिर bool             s_suspendHandlersInstalled = false;

स्थिर
शून्य m3_PosixSignalHandler (पूर्णांक sig)
{
    (शून्य)sig;
    जर (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

शून्य m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    जर (!s_suspendHandlersInstalled) {
        संरचना sigaction sa;
        स_रखो(&sa, 0, आकार(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(संक_विघ्न, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

शून्य m3_HostRemoveInterruptHandler (शून्य)
{
    जर (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, रिक्त);
        sigaction(संक_विघ्न, &s_oldSigInt, रिक्त);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = रिक्त;
}

u64 m3_HostTimeMs (शून्य)
{
    संरचना timespec now;

    जर (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        परत 0;
    }

    परत (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

शून्य m3_HostSleepNs (u64 i_ns)
{
    संरचना timespec डावा;
    डावा.tv_sec = (समय_प्रकार)(i_ns / 1000000000u);
    डावा.tv_nsec = (दीर्घ)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    पर्यंत (nanosleep(&डावा, &डावा) != 0 and त्रुटि_सं == EINTR) {
    }
}

bool m3_HostMapFile (स्थिरांक अक्षर* i_path, माप_प्रकार i_maxBytes, M3HostFile* o_file)
{
    पूर्णांक fd = खोलो(i_path, O_RDONLY);
    जर (fd < 0) {
        परत false;
    }

    संरचना stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    जर (ख_स्थिति(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(माप_प्रकार) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        बंद(fd);
        परत m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    माप_प्रकार size = (माप_प्रकार)info.st_size;
    शून्य*  base = mmap(रिक्त, size, PROT_READ, MAP_PRIVATE, fd, 0);

    जर (base == MAP_FAILED) {
        बंद(fd);
        परत m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#जर defined(LOCK_SH)
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
    परत true;
}

शून्य m3_HostUnmapFile (M3HostFile* io_file)
{
    जर (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        जर (io_file->handle != d_m3HostNoHandle) {
            बंद((पूर्णांक)io_file->handle);     // and with it the lock
        }
    } अन्यथा {
        m3_Free(io_file->data);
    }

    io_file->data = रिक्त;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (स्थिरांक अक्षर* i_from, स्थिरांक अक्षर* i_to)
{
    परत नाम(i_from, i_to) == 0;
}

#जर d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

माप_प्रकार m3_HostPageSize (शून्य)
{
    दीर्घ size = sysconf(_SC_PAGESIZE);

    परत (size > 0) ? (माप_प्रकार)size : 4096;
}

शून्य* m3_HostReserve (माप_प्रकार i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    शून्य* base = mmap(रिक्त, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    परत (base == MAP_FAILED) ? रिक्त : base;
}

bool m3_HostCommit (शून्य* i_address, माप_प्रकार i_bytes)
{
    जर (i_bytes == 0) {
        परत true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    परत mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (शून्य* i_address, माप_प्रकार i_bytes)
{
    जर (i_bytes == 0) {
        परत true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    शून्य* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    परत again == i_address;
}

शून्य m3_HostRelease (शून्य* i_address, माप_प्रकार i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
प्रकारव्याख्या संरचना M3GuardFrame {
    sigjmp_buf           jmp;
    स्थिरांक u8*            खालचा;
    माप_प्रकार               bytes;
    संरचना M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
प्रकारव्याख्या गणना M3GuardState {
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
शून्य guard_signal (पूर्णांक i_signal, siginfo_t* i_info, शून्य* i_ucontext)
{
    स्थिरांक M3GuardFrame* frame = g_guardFrame;

    जर (frame and i_info) {
        स्थिरांक u8* address = (स्थिरांक u8*)i_info->si_addr;

        जर (address >= frame->खालचा and address < frame->खालचा + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    स्थिरांक संरचना sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    जर ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } अन्यथा जर (previous->sa_handler != संक_पूर्व and previous->sa_handler != संक_छोड़ो) {
        previous->sa_handler(i_signal);
    } अन्यथा {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        संरचना sigaction dfl;
        स_रखो(&dfl, 0, आकार(dfl));
        dfl.sa_handler = संक_पूर्व;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, रिक्त);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
स्थिर
bool guard_handler_took (पूर्णांक i_signal)
{
    संरचना sigaction installed;
    स_रखो(&installed, 0, आकार(installed));

    जर (sigaction(i_signal, रिक्त, &installed) != 0) {
        परत false;
    }

    परत (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

स्थिर
bool install_guard_handlers (शून्य)
{
    जर (g_guards != guards_initial) {
        परत g_guards == guards_active;
    }

    संरचना sigaction action;
    स_रखो(&action, 0, आकार(action));

    action.sa_sigaction = guard_signal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&action.sa_mask);

    sigaction(संक_अंश, &action, &g_previousSegv);

    // Darwin reports a write to a PROT_NONE page as SIGBUS where Linux reports
    // SIGSEGV, so both have to be claimed
    sigaction(SIGBUS, &action, &g_previousBus);

    g_guards = (guard_handler_took(संक_अंश) and guard_handler_took(SIGBUS))
                 ? guards_active
                 : guards_unavailable;

    परत g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
स्थिर
शून्य install_alt_चळत (शून्य)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    स्थिर M3_THREAD_LOCAL u8   चळत[64 * 1024];
    स्थिर M3_THREAD_LOCAL bool installed;

    जर (installed) {
        परत;
    }
    installed = true;

    stack_t current;
    जर (sigaltstack(रिक्त, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        परत;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = चळत;
    alt.ss_size = आकार(चळत);
    alt.ss_flags = 0;

    sigaltstack(&alt, रिक्त);
}

bool m3_HostGuardsActive (शून्य)
{
    परत install_guard_handlers();
}

bool m3_HostProtectedCall (शून्य (*i_body)(शून्य*), शून्य* i_context,
                           शून्य* i_guard_खालचा, माप_प्रकार i_guardBytes)
{
    install_guard_handlers();
    install_alt_चळत();

    M3GuardFrame frame;
    frame.खालचा = (स्थिरांक u8*)i_guard_खालचा;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    जर (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } अन्यथा {
        finished = false;
    }

    g_guardFrame = frame.previous;

    परत finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
