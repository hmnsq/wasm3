//
//  m3_host_posix.h
//
//  The POSIX implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#अगर_अघोषित m3_host_posix_h
#घोषणा m3_host_posix_h

#समावेश "m3_core.h"

#अगर defined(_WIN32)
#  error "This file is not for Windows"
#पूर्ण_अगर

#समावेश "m3_host.h"

#समावेश <fcntl.h>
#समावेश <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#समावेश <sys/mman.h>
#समावेश <sys/stat.h>
#समावेश <unistd.h>
#समावेश <signal.h>
#समावेश <errno.h>
#समावेश <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#अगर_अघोषित d_m3_Has_Thread_ढेर_Probe
#  अगर defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3_Has_Thread_ढेर_Probe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    अगर __GLIBC_PREREQ(2, 34)
#      define d_m3_Has_Thread_ढेर_Probe          1
#    अन्यथा
#      define d_m3_Has_Thread_ढेर_Probe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3_Has_Thread_ढेर_Probe            1    // musl keeps it in libc
#  अन्यथा
#    define d_m3_Has_Thread_ढेर_Probe            0
#  endif
#पूर्ण_अगर

#अगर d_m3_Has_Thread_ढेर_Probe
#  include <pthread.h>
#  अगर defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#पूर्ण_अगर

#अगर d_m3_Has_Thread_ढेर_Probe && defined(__linux__)

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
अटल
व्योम* linux_main_ढेर_base (व्योम)
{
    अगर ((दीर्घ)syscall(SYS_gettid) != (दीर्घ)getpid()) {
        वापस शून्य;
    }

    काष्ठा rlimit limit;

    अगर (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        वापस शून्य;
    }

    पूर्णांक fd = खोलो("/proc/self/stat", O_RDONLY);

    अगर (fd < 0) {
        वापस शून्य;
    }

    अक्षर    पंक्ति_2[512];
    ssize_t got = पढ़ो(fd, पंक्ति_2, माप(पंक्ति_2) - 1);

    बंद(fd);

    अगर (got <= 0) {
        वापस शून्य;
    }

    पंक्ति_2[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    स्थिर अक्षर* field = म_खोजप(पंक्ति_2, ')');

    क्रम (पूर्णांक संख्या = 2; field and संख्या < 28; ++संख्या) {
        field = म_अक्षर(field + 1, ' ');
    }

    अगर (field == शून्य) {
        वापस शून्य;
    }

    अचिन्हित दीर्घ दीर्घ शीर्ष = strtoull(field + 1, शून्य, 10);

    // A limit reaching below nothing is not a stack this can describe
    अगर (शीर्ष == 0 or (अचिन्हित दीर्घ दीर्घ) limit.rlim_cur > शीर्ष) {
        वापस शून्य;
    }

    वापस (u8*)(uintptr_t)शीर्ष - (माप_प्रकार)limit.rlim_cur;
}

#पूर्ण_अगर

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
व्योम* m3_Host_ढेर_Base (व्योम)
{
#अगर !d_m3_Has_Thread_ढेर_Probe

    वापस शून्य;

#या_अगर defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       ऊपरी = (u8*)pthread_get_stackaddr_np(self);
    माप_प्रकार    आकार = pthread_get_stacksize_np(self);

    अगर (ऊपरी == शून्य or आकार == 0) {
        वापस शून्य;
    }

    वापस ऊपरी - आकार;

#अन्यथा

#  अगर defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_ढेर_base();

    अगर (main_base) {
        वापस main_base;
    }

#  endif

    pthread_attr_t attr;

    अगर (pthread_getattr_np(pthread_self(), &attr) != 0) {
        वापस शून्य;
    }

    व्योम*  base = शून्य;
    माप_प्रकार आकार = 0;

    अगर (pthread_attr_getstack(&attr, &base, &आकार) != 0 or आकार == 0) {
        base = शून्य;
    }

    pthread_attr_destroy(&attr);

    वापस base;

#पूर्ण_अगर
}

अटल IM3Runtime       s_posixSuspendRuntime = शून्य;
अटल काष्ठा sigaction s_oldSigTstp;
अटल काष्ठा sigaction s_oldSigInt;
अटल bool             s_suspendHandlersInstalled = false;

अटल
व्योम m3_PosixSignalHandler (पूर्णांक sig)
{
    (व्योम)sig;
    अगर (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

व्योम m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    अगर (!s_suspendHandlersInstalled) {
        काष्ठा sigaction sa;
        स_रखो(&sa, 0, माप(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(संक_विघ्न, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

व्योम m3_HostRemoveInterruptHandler (व्योम)
{
    अगर (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, शून्य);
        sigaction(संक_विघ्न, &s_oldSigInt, शून्य);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = शून्य;
}

u64 m3_HostTimeMs (व्योम)
{
    काष्ठा timespec now;

    अगर (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        वापस 0;
    }

    वापस (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

व्योम m3_HostSleepNs (u64 i_ns)
{
    काष्ठा timespec बायाँ;
    बायाँ.tv_sec = (समय_प्रकार)(i_ns / 1000000000u);
    बायाँ.tv_nsec = (दीर्घ)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    जबतक (nanosleep(&बायाँ, &बायाँ) != 0 and त्रुटि_सं == EINTR) {
    }
}

bool m3_HostMapFile (स्थिर अक्षर* i_path, माप_प्रकार i_maxBytes, M3HostFile* o_file)
{
    पूर्णांक fd = खोलो(i_path, O_RDONLY);
    अगर (fd < 0) {
        वापस false;
    }

    काष्ठा stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    अगर (ख_स्थिति(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(माप_प्रकार) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        बंद(fd);
        वापस m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    माप_प्रकार आकार = (माप_प्रकार)info.st_size;
    व्योम*  base = mmap(शून्य, आकार, PROT_READ, MAP_PRIVATE, fd, 0);

    अगर (base == MAP_FAILED) {
        बंद(fd);
        वापस m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#अगर defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (व्योम)flock(fd, LOCK_SH | LOCK_NB);
#पूर्ण_अगर

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->तथ्य = base;
    o_file->आकार = आकार;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    वापस true;
}

व्योम m3_HostUnmapFile (M3HostFile* io_file)
{
    अगर (io_file->mapped) {
        munmap(io_file->तथ्य, io_file->आकार);

        अगर (io_file->handle != d_m3HostNoHandle) {
            बंद((पूर्णांक)io_file->handle);     // and with it the lock
        }
    } अन्यथा {
        m3_Free(io_file->तथ्य);
    }

    io_file->तथ्य = शून्य;
    io_file->आकार = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (स्थिर अक्षर* i_from, स्थिर अक्षर* i_to)
{
    वापस नाम(i_from, i_to) == 0;
}

#अगर d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

माप_प्रकार m3_Host_Page_आकार (व्योम)
{
    दीर्घ आकार = sysconf(_SC_PAGESIZE);

    वापस (आकार > 0) ? (माप_प्रकार)आकार : 4096;
}

व्योम* m3_HostReserve (माप_प्रकार i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    व्योम* base = mmap(शून्य, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    वापस (base == MAP_FAILED) ? शून्य : base;
}

bool m3_HostCommit (व्योम* i_address, माप_प्रकार i_bytes)
{
    अगर (i_bytes == 0) {
        वापस true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    वापस mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (व्योम* i_address, माप_प्रकार i_bytes)
{
    अगर (i_bytes == 0) {
        वापस true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    व्योम* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    वापस again == i_address;
}

व्योम m3_HostRelease (व्योम* i_address, माप_प्रकार i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
प्रकार काष्ठा M3GuardFrame {
    sigjmp_buf           jmp;
    स्थिर u8*            निचला;
    माप_प्रकार               bytes;
    काष्ठा M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
प्रकार क्रमागत M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

अटल M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
अटल काष्ठा sigaction              g_previousSegv;
अटल काष्ठा sigaction              g_previousBus;
अटल M3GuardState                  g_guards;

अटल
व्योम guard_signal (पूर्णांक i_signal, siginfo_t* i_info, व्योम* i_ucontext)
{
    स्थिर M3GuardFrame* frame = g_guardFrame;

    अगर (frame and i_info) {
        स्थिर u8* address = (स्थिर u8*)i_info->si_addr;

        अगर (address >= frame->निचला and address < frame->निचला + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    स्थिर काष्ठा sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    अगर ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } अन्यथा अगर (previous->sa_handler != संक_पूर्व and previous->sa_handler != संक_छोड़ो) {
        previous->sa_handler(i_signal);
    } अन्यथा {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        काष्ठा sigaction dfl;
        स_रखो(&dfl, 0, माप(dfl));
        dfl.sa_handler = संक_पूर्व;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, शून्य);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
अटल
bool guard_handler_took (पूर्णांक i_signal)
{
    काष्ठा sigaction installed;
    स_रखो(&installed, 0, माप(installed));

    अगर (sigaction(i_signal, शून्य, &installed) != 0) {
        वापस false;
    }

    वापस (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

अटल
bool install_guard_handlers (व्योम)
{
    अगर (g_guards != guards_initial) {
        वापस g_guards == guards_active;
    }

    काष्ठा sigaction action;
    स_रखो(&action, 0, माप(action));

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

    वापस g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
अटल
व्योम install_alt_ढेर (व्योम)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    अटल M3_THREAD_LOCAL u8   ढेर[64 * 1024];
    अटल M3_THREAD_LOCAL bool installed;

    अगर (installed) {
        वापस;
    }
    installed = true;

    stack_t current;
    अगर (sigaltstack(शून्य, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        वापस;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = ढेर;
    alt.ss_size = माप(ढेर);
    alt.ss_flags = 0;

    sigaltstack(&alt, शून्य);
}

bool m3_HostGuardsActive (व्योम)
{
    वापस install_guard_handlers();
}

bool m3_HostProtectedCall (व्योम (*i_body)(व्योम*), व्योम* i_context,
                           व्योम* i_guard_निचला, माप_प्रकार i_guardBytes)
{
    install_guard_handlers();
    install_alt_ढेर();

    M3GuardFrame frame;
    frame.निचला = (स्थिर u8*)i_guard_निचला;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    अगर (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } अन्यथा {
        finished = false;
    }

    g_guardFrame = frame.previous;

    वापस finished;
}

#पूर्ण_अगर // d_m3GuardedMemory

#पूर्ण_अगर // m3_host_posix_h
