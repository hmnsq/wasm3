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

#include "m3_core.h"

#nếu defined(_WIN32)
#  error "This file is not for Windows"
#endif

#include "m3_host.h"

#include <fcntl.h>
#include <sys/file.h>       // flock, which m3_HostMapFile takes a shared one with
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

// Whether pthread_getattr_np - or Darwin's pair of calls - can be reached without
// asking the build to link a threading library it may not be linking. glibc moved
// libpthread into libc in 2.34; before that this call was the one thing here that
// would not link, so a build on an older glibc keeps the compile-time budget unless
// it says otherwise. Decided here rather than in m3_config_platforms.h because
// __GLIBC__ only exists once a libc header has been seen, which m3_core.h above is.
#ifndef d_m3HasThreadStackProbe
#  nếu defined(__APPLE__) || defined(__ANDROID__)
#    define d_m3HasThreadStackProbe            1    // libSystem, bionic
#  elif defined(__GLIBC__) && defined(__GLIBC_PREREQ)
#    nếu __GLIBC_PREREQ(2, 34)
#      define d_m3HasThreadStackProbe          1
#    không_thì
#      define d_m3HasThreadStackProbe          0    // still in libpthread here
#    endif
#  elif defined(__linux__)
#    define d_m3HasThreadStackProbe            1    // musl keeps it in libc
#  không_thì
#    define d_m3HasThreadStackProbe            0
#  endif
#endif

#nếu d_m3HasThreadStackProbe
#  include <pthread.h>
#  nếu defined(__linux__)
#    include <sys/resource.h>
#    include <sys/syscall.h>
#  endif
#endif

#nếu d_m3HasThreadStackProbe && defined(__linux__)

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
static
void* linux_main_stack_base (void)
{
    nếu ((long)syscall(SYS_gettid) != (long)getpid()) {
        trả_về NULL;
    }

    struct rlimit limit;

    nếu (getrlimit(RLIMIT_STACK, &limit) != 0 or
        limit.rlim_cur == RLIM_INFINITY or limit.rlim_cur == 0) {
        trả_về NULL;
    }

    số_nguyên fd = open("/proc/self/stat", O_RDONLY);

    nếu (fd < 0) {
        trả_về NULL;
    }

    char    line[512];
    ssize_t got = read(fd, line, sizeof(line) - 1);

    close(fd);

    nếu (got <= 0) {
        trả_về NULL;
    }

    line[got] = 0;

    // Field 28 is where the stack started. The fields before it include the
    // executable's name in parentheses, and that name may hold anything at all -
    // spaces and parentheses included - so the count begins after the last of those.
    const char* field = strrchr(line, ')');

    cho (số_nguyên number = 2; field and number < 28; ++number) {
        field = strchr(field + 1, ' ');
    }

    nếu (field == NULL) {
        trả_về NULL;
    }

    unsigned long long top = strtoull(field + 1, NULL, 10);

    // A limit reaching below nothing is not a stack this can describe
    nếu (top == 0 or (unsigned long long) limit.rlim_cur > top) {
        trả_về NULL;
    }

    trả_về (u8*)(uintptr_t)top - (size_t)limit.rlim_cur;
}

#endif

// Answers for every thread and not just the first, which is what asking pthreads is
// for: RLIMIT_STACK describes the stack the process started on, and a thread given a
// stack of its own is not it. Where the first thread can be recognized, it is asked
// about separately - see linux_main_stack_base.
void* m3_HostStackBase (void)
{
#nếu !d_m3HasThreadStackProbe

    trả_về NULL;

#elif defined(__APPLE__)

    // Darwin describes a stack by its high end and its size, the opposite way round
    pthread_t self = pthread_self();

    u8*       high = (u8*)pthread_get_stackaddr_np(self);
    size_t    size = pthread_get_stacksize_np(self);

    nếu (high == NULL or size == 0) {
        trả_về NULL;
    }

    trả_về high - size;

#không_thì

#  nếu defined(__linux__)

    // Only where it answers: a system with no /proc, or a stack with no limit on how
    // far it grows, falls through to pthreads like everything else
    u8* main_base = (u8*)linux_main_stack_base();

    nếu (main_base) {
        trả_về main_base;
    }

#  endif

    pthread_attr_t attr;

    nếu (pthread_getattr_np(pthread_self(), &attr) != 0) {
        trả_về NULL;
    }

    void*  base = NULL;
    size_t size = 0;

    nếu (pthread_attr_getstack(&attr, &base, &size) != 0 or size == 0) {
        base = NULL;
    }

    pthread_attr_destroy(&attr);

    trả_về base;

#endif
}

static IM3Runtime       s_posixSuspendRuntime = NULL;
static struct sigaction s_oldSigTstp;
static struct sigaction s_oldSigInt;
static bool             s_suspendHandlersInstalled = false;

static
void m3_PosixSignalHandler (số_nguyên sig)
{
    (void)sig;
    nếu (s_posixSuspendRuntime) {
        s_posixSuspendRuntime->suspendRequested = true;
    }
}

void m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_posixSuspendRuntime = io_runtime;
    nếu (!s_suspendHandlersInstalled) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = m3_PosixSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGTSTP, &sa, &s_oldSigTstp);
        sigaction(SIGINT, &sa, &s_oldSigInt);
        s_suspendHandlersInstalled = true;
    }
}

void m3_HostRemoveInterruptHandler (void)
{
    nếu (s_suspendHandlersInstalled) {
        sigaction(SIGTSTP, &s_oldSigTstp, NULL);
        sigaction(SIGINT, &s_oldSigInt, NULL);
        s_suspendHandlersInstalled = false;
    }
    s_posixSuspendRuntime = NULL;
}

u64 m3_HostTimeMs (void)
{
    struct timespec now;

    nếu (clock_gettime(CLOCK_REALTIME, &now) != 0 || now.tv_sec < 0) {
        trả_về 0;
    }

    trả_về (u64)now.tv_sec * 1000 + (u64)now.tv_nsec / 1000000;
}

void m3_HostSleepNs (u64 i_ns)
{
    struct timespec left;
    left.tv_sec = (time_t)(i_ns / 1000000000u);
    left.tv_nsec = (long)(i_ns % 1000000000u);

    // a signal cuts the sleep short and reports what remains
    trong_khi (nanosleep(&left, &left) != 0 and errno == EINTR) {
    }
}

bool m3_HostMapFile (const char* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    số_nguyên fd = open(i_path, O_RDONLY);
    nếu (fd < 0) {
        trả_về false;
    }

    struct stat info;

    // Only a regular file of a size worth having: a directory cannot be mapped, a
    // pipe or a device has no size to map, and an empty one is no module anyway.
    // Anything this turns down is read instead, which is what makes a module out of
    // a process substitution work.
    nếu (fstat(fd, &info) != 0 or not S_ISREG(info.st_mode) or info.st_size <= 0 or
        (off_t)(size_t) info.st_size != info.st_size or
        (i_maxBytes and (uint64_t) info.st_size > (uint64_t)i_maxBytes)) {
        close(fd);
        trả_về m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    size_t size = (size_t)info.st_size;
    void*  base = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

    nếu (base == MAP_FAILED) {
        close(fd);
        trả_về m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

#nếu defined(LOCK_SH)
    // Shared, so that two wasm3 processes can run the same module, and
    // non-blocking, so that a file somebody else has locked is mapped anyway rather
    // than waiting on them. Failing changes nothing: the lock is advisory either
    // way, and a filesystem that has no opinion about locks is not a reason to
    // refuse the module. See m3_host.h for how little this promises.
    (void)flock(fd, LOCK_SH | LOCK_NB);
#endif

    // Unlike the mapping, the lock lives on the open file description, so the
    // descriptor has to stay open for as long as the mapping does
    o_file->data = base;
    o_file->size = size;
    o_file->handle = (intptr_t)fd;
    o_file->mapped = true;
    trả_về true;
}

void m3_HostUnmapFile (M3HostFile* io_file)
{
    nếu (io_file->mapped) {
        munmap(io_file->data, io_file->size);

        nếu (io_file->handle != d_m3HostNoHandle) {
            close((số_nguyên)io_file->handle);     // and with it the lock
        }
    } không_thì {
        m3_Free(io_file->data);
    }

    io_file->data = NULL;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename is atomic within a filesystem, which a file and its neighbour share
bool m3_HostReplaceFile (const char* i_from, const char* i_to)
{
    trả_về rename(i_from, i_to) == 0;
}

#nếu d_m3GuardedMemory

#  include <setjmp.h>
#  include <signal.h>

size_t m3_HostPageSize (void)
{
    long size = sysconf(_SC_PAGESIZE);

    trả_về (size > 0) ? (size_t)size : 4096;
}

void* m3_HostReserve (size_t i_bytes)
{
    // MAP_NORESERVE so that a reservation this large is not charged against the
    // overcommit budget: almost none of it is ever going to be touched
    void* base = mmap(NULL, i_bytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    trả_về (base == MAP_FAILED) ? NULL : base;
}

bool m3_HostCommit (void* i_address, size_t i_bytes)
{
    nếu (i_bytes == 0) {
        trả_về true;
    }

    // The pages are already there and already zero - the reservation was anonymous -
    // so committing is only a matter of being allowed to reach them
    trả_về mprotect(i_address, i_bytes, PROT_READ | PROT_WRITE) == 0;
}

bool m3_HostDecommit (void* i_address, size_t i_bytes)
{
    nếu (i_bytes == 0) {
        trả_về true;
    }

    // Mapping fresh anonymous pages over the range is what drops the old ones -
    // mprotect alone would leave them allocated and still holding what was written
    void* again = mmap(i_address, i_bytes, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);

    trả_về again == i_address;
}

void m3_HostRelease (void* i_address, size_t i_bytes)
{
    munmap(i_address, i_bytes);
}


// What a protected call left behind for the signal handler to find. Per-thread,
// because the fault arrives on the thread that caused it and nowhere else, and a
// chain, because a host function may call back into Wasm from inside one.
typedef struct M3GuardFrame {
    sigjmp_buf           jmp;
    const u8*            low;
    size_t               bytes;
    struct M3GuardFrame* previous;
} M3GuardFrame;

// Installing happens once and is never retried: a second pass would capture guard_signal
// itself as the previous handler, and an unclaimed fault would then call it forever. So
// the attempt and what came of it are one piece of state, settled on the first ask.
typedef enum M3GuardState {
    guards_initial = 0,
    guards_active,
    guards_unavailable
} M3GuardState;

static M3_THREAD_LOCAL M3GuardFrame* g_guardFrame;

// The handlers that were there first. A fault this does not claim has to end the way
// it would have without any of this, so those are called rather than replaced.
static struct sigaction              g_previousSegv;
static struct sigaction              g_previousBus;
static M3GuardState                  g_guards;

static
void guard_signal (số_nguyên i_signal, siginfo_t* i_info, void* i_ucontext)
{
    const M3GuardFrame* frame = g_guardFrame;

    nếu (frame and i_info) {
        const u8* address = (const u8*)i_info->si_addr;

        nếu (address >= frame->low and address < frame->low + frame->bytes) {
            // Everything the interpreter was in the middle of is data on the heap,
            // so there is nothing here to unwind - see m3_HostProtectedCall
            siglongjmp(((M3GuardFrame*)frame)->jmp, 1);
        }
    }

    const struct sigaction* previous = (i_signal == SIGBUS) ? &g_previousBus : &g_previousSegv;

    nếu ((previous->sa_flags & SA_SIGINFO) and previous->sa_sigaction) {
        previous->sa_sigaction(i_signal, i_info, i_ucontext);
    } không_thì nếu (previous->sa_handler != SIG_DFL and previous->sa_handler != SIG_IGN) {
        previous->sa_handler(i_signal);
    } không_thì {
        // Put the default back and return: the instruction runs again, faults again,
        // and the process dies where it stood rather than somewhere in here
        struct sigaction dfl;
        memset(&dfl, 0, sizeof(dfl));
        dfl.sa_handler = SIG_DFL;
        sigemptyset(&dfl.sa_mask);
        sigaction(i_signal, &dfl, NULL);
    }
}

// Whether i_signal is being delivered to guard_signal. sigaction reporting success is
// not enough on its own: AddressSanitizer's interceptor returns 0 for a handler it has
// no intention of installing, so the only answer to trust is the one the system gives
// back when asked what is installed.
static
bool guard_handler_took (số_nguyên i_signal)
{
    struct sigaction installed;
    memset(&installed, 0, sizeof(installed));

    nếu (sigaction(i_signal, NULL, &installed) != 0) {
        trả_về false;
    }

    trả_về (installed.sa_flags & SA_SIGINFO) and installed.sa_sigaction == guard_signal;
}

static
bool install_guard_handlers (void)
{
    nếu (g_guards != guards_initial) {
        trả_về g_guards == guards_active;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));

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

    trả_về g_guards == guards_active;
}

// A stack for the handler to run on, so that a fault which happens because the
// thread's own stack ran out still has somewhere to be handled. One per thread, and
// only where the thread has not already been given one.
static
void install_alt_stack (void)
{
    // A fixed size rather than SIGSTKSZ, which recent glibc made a call to sysconf
    // and so no longer something an array can be declared with. 64KiB is well over
    // what any of these handlers need - the claimed path is a siglongjmp, and the
    // unclaimed one is a call to whatever was there before.
    static M3_THREAD_LOCAL u8   stack[64 * 1024];
    static M3_THREAD_LOCAL bool installed;

    nếu (installed) {
        trả_về;
    }
    installed = true;

    stack_t current;
    nếu (sigaltstack(NULL, &current) == 0 and not (current.ss_flags & SS_DISABLE)) {
        trả_về;                          // this thread already has one
    }

    stack_t alt;
    alt.ss_sp = stack;
    alt.ss_size = sizeof(stack);
    alt.ss_flags = 0;

    sigaltstack(&alt, NULL);
}

bool m3_HostGuardsActive (void)
{
    trả_về install_guard_handlers();
}

bool m3_HostProtectedCall (void (*i_body)(void*), void* i_context,
                           void* i_guardLow, size_t i_guardBytes)
{
    install_guard_handlers();
    install_alt_stack();

    M3GuardFrame frame;
    frame.low = (const u8*)i_guardLow;
    frame.bytes = i_guardBytes;
    frame.previous = g_guardFrame;

    g_guardFrame = &frame;

    bool finished;

    // The mask is deliberately not saved: doing so costs a system call on every
    // call into Wasm, and the one thing it would have restored - SIGSEGV unblocked -
    // SA_NODEFER already arranges by never blocking it in the first place.
    nếu (sigsetjmp(frame.jmp, 0) == 0) {
        i_body(i_context);
        finished = true;
    } không_thì {
        finished = false;
    }

    g_guardFrame = frame.previous;

    trả_về finished;
}

#endif // d_m3GuardedMemory

#endif // m3_host_posix_h
