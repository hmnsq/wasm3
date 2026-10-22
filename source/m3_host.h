//
//  m3_host.h
//
//  What the engine asks of the system it is hosted on.
//
//  Everything here is something only the operating system can answer: how much native
//  stack this thread still has, how to get at a file's bytes and replace a file whole,
//  and - where the build
//  asks for guarded memories - how to reserve address space, commit part of it, and
//  turn the fault from a read past the committed part back into a Wasm trap.
//
//  Exactly one implementation is ever built:
//
//    m3_host_win32.h    VirtualAlloc/VirtualQuery, file mappings, and structured
//                       exception handling
//    m3_host_posix.h    mmap/mprotect, file mappings, and a SIGSEGV handler
//    m3_host_none.h     a system with none of that, which is most of platforms/:
//                       it reads a file rather than mapping one, and says it cannot
//                       measure a stack
//
//  All three are headers, and m3_core.c includes whichever one it is built for: a new
//  .c would have to be added to every source list that names its sources one by one,
//  and most of those are build scripts outside this repository. Unlike the WASI host
//  split, the callers here are in other translation units, so these are real
//  definitions rather than static ones - this header is the only declaration of them
//  there is.
//
//  Every call is allowed to fail, and failing is not an error: m3_HostStackBase
//  answers NULL on a system that cannot be asked, and the engine falls back to the
//  compile-time budget. Only the reservation calls report failure the caller has to
//  act on, and only a build with d_m3GuardedMemory has any.
//

#अगर_अघोषित m3_host_h
#घोषणा m3_host_h

#समावेश "m3_core.h"

d_m3BeginExternC

// The lowest address the calling thread's stack can reach, or NULL when this
// build cannot ask (see d_m3HasThreadStackProbe). What sits at the very bottom
// is the system's business - a guard page usually - so the answer bounds the
// stack rather than describing what is safe to touch; d_m3NativeStackMargin is
// what keeps the difference.
व्योम* m3_Host_ढेर_Base (व्योम);

// Suspension interrupt handlers (Ctrl+Z / SIGTSTP / SIGINT / Console Ctrl)
व्योम  m3_HostInstallInterruptHandler (IM3Runtime io_runtime);
व्योम  m3_HostRemoveInterruptHandler (व्योम);

// The wall clock, in milliseconds since the Unix epoch, UTC. 0 when there is no
// clock to ask. Only ever written down - a snapshot says when it was taken - so
// nothing depends on it being right.
u64   m3_HostTimeMs (व्योम);

// Blocks the calling thread for at least the given time. A host with no clock to
// wait on returns at once, as though the time had passed.
व्योम  m3_HostSleepNs (u64 i_ns);


// A file's bytes and how they were come by.
//
// 'mapped' is the one that says who gives the bytes back, and it is not the same
// question as whether there is a handle: a Win32 mapping keeps nothing open, because
// the view holds the section which holds the file object and the share mode with it,
// while POSIX has to hold the descriptor its lock lives on. So a mapped file may have
// no handle, and d_m3HostNoHandle is what that looks like.
#घोषणा d_m3HostNoHandle    ((intptr_t) -1)

प्रकार काष्ठा M3HostFile {
    व्योम*    तथ्य;
    माप_प्रकार   आकार;
    intptr_t handle;     // the host layer's, and d_m3HostNoHandle when there is none
    bool     mapped;
} M3HostFile;

// The module's bytes, however this system can best get at them: mapped where it can
// map, read onto the heap where it cannot - a system with no mapping at all, a
// filesystem that will not, or something that is not a file, which is what a pipe
// from a process substitution is. The caller is told which only by 'mapped', and
// needs to know only because m3_HostUnmapFile is what gives the bytes back either
// way.
//
// i_maxBytes refuses a file larger than the caller is willing to take, before
// anything is allocated for it; 0 accepts any size the system will hand over. False
// is that, or the file not being readable at all, and o_file is written on success
// only.
bool m3_HostMapFile (स्थिर अक्षर* i_path, माप_प्रकार i_maxBytes, M3HostFile* o_file);

// Give the bytes back, releasing the mapping and its lock, or the heap block
व्योम m3_HostUnmapFile (M3HostFile* io_file);

// Move the file at i_from over i_to, replacing whatever is there. Where the system
// can, in one step: a reader of i_to sees the old file or the new one, never one
// half written, and a writer that dies part way leaves i_to as it was. False when
// the move failed, with i_from left where it is.
bool m3_HostReplaceFile (स्थिर अक्षर* i_from, स्थिर अक्षर* i_to);

// The reading half on its own: what each m3_HostMapFile falls back to, and the whole
// of what a system with no mapping does. The same on every system, so it is here
// rather than in any of them - and inline, so that a translation unit which never
// calls it is not the one that drags stdio's FILE machinery into the link.
अटल अंतरभूत
bool m3_HostReadFile (स्थिर अक्षर* i_path, माप_प्रकार i_maxBytes, M3HostFile* o_file)
{
    खाता* f = ख_खोलो(i_path, "rb");
    अगर (f == शून्य) {
        वापस false;
    }

    // The size is taken before anything is allocated, so that a file too big to be
    // wanted is refused rather than read
    ख_जाओ(f, 0, अंत_से);
    दीर्घ tell = ख_बताओ(f);
    ख_जाओ(f, 0, शुरू_से);

    अगर (tell < 0 or (i_maxBytes and (माप_प्रकार) tell > i_maxBytes)) {
        ख_बंद(f);
        वापस false;
    }

    माप_प्रकार आकार = (माप_प्रकार)tell;
    व्योम*  तथ्य = m3_Malloc("Host File", आकार ? आकार : 1);

    अगर (तथ्य == शून्य) {
        ख_बंद(f);
        वापस false;
    }

    bool पढ़ो = (ख_पढ़ो(तथ्य, 1, आकार, f) == आकार);
    ख_बंद(f);

    अगर (not पढ़ो) {
        m3_Free(तथ्य);
        वापस false;
    }

    o_file->तथ्य = तथ्य;
    o_file->आकार = आकार;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = false;
    वापस true;
}

// A module is parsed in place and keeps pointing into the mapped bytes for as long
// as it is loaded, so this is where a module's own size stops being paid for twice:
// what is mapped stays in the page cache, shared with anything else running the same
// file, and is only ever read as far as the module is walked.
//
// What is mapped is the file itself, so a file that changes underneath the mapping
// changes under a running module. Each system is asked to prevent that as far as it
// can, and the two can do very different amounts:
//
//   Win32   a share mode the kernel enforces. Another process opening the file for
//           writing is refused outright while the mapping lives, so this is a real
//           guarantee. Other readers - another wasm3 on the same module - are fine.
//   POSIX   a shared advisory lock, which stops only a writer that asks for a lock
//           of its own. Nothing stops a plain open-and-write, because POSIX has no
//           portable mandatory locking: Linux removed what it had in 5.15, and
//           O_EXLOCK is a BSD extension that is advisory as well.
//
// So on POSIX this narrows the window rather than closing it.

#अगर d_m3GuardedMemory

#  अगर M3_SIZEOF_PTR < 8
#    error "d_m3GuardedMemory needs a 64-bit address space"
#  endif
#  अगर d_m3FixedHeap
#    error "d_m3GuardedMemory and d_m3FixedHeap are two different places for a linear memory to come from"
#  endif
#  अगर defined(_WIN32) && !defined(_MSC_VER)
#    error "d_m3GuardedMemory on Windows needs __try/__except - build with MSVC or clang-cl"
#  endif

// The system's allocation granularity: what a reservation's size and a commit's
// bounds are rounded to. Never zero.
माप_प्रकार m3_Host_Page_आकार (व्योम);

// Reserve i_bytes of address space without backing any of it. Reading or writing it
// faults until m3_HostCommit says otherwise, which is the whole point: the fault is
// what a Wasm access past the end of its memory turns into. NULL if the system will
// not.
व्योम*  m3_HostReserve (माप_प्रकार i_bytes);

// Back [i_address, i_address + i_bytes) with pages, which read as zero. The range
// has to be inside a reservation from m3_HostReserve; committing what is already
// committed is not an error and changes nothing.
bool   m3_HostCommit (व्योम* i_address, माप_प्रकार i_bytes);

// Undo m3_HostCommit for [i_address, i_address + i_bytes), giving the pages back to
// the system and leaving the range reserved and unreachable again. What was written
// there is gone: committing it once more reads as zero.
bool   m3_HostDecommit (व्योम* i_address, माप_प्रकार i_bytes);

// Give back a whole reservation. i_bytes is what was reserved.
व्योम   m3_HostRelease (व्योम* i_address, माप_प्रकार i_bytes);

// Run i_body(i_context) with a fault inside [i_guardLow, i_guardLow + i_guardBytes)
// caught and reported rather than killing the process: false means the body touched
// that range and did not finish. Any other fault is left to whatever would have
// handled it, so an engine bug still looks like an engine bug.
//
// Nests, so a host function calling back into Wasm gets a region of its own, and is
// per-thread. What the body was doing when it faulted is abandoned where it stands -
// the caller has to be able to carry on without it, which for the interpreter means
// the Wasm stack and the memory, both of which are just data.
bool   m3_HostProtectedCall (व्योम (*i_body)(व्योम*), व्योम* i_context,
                             व्योम* i_guard_निचला, माप_प्रकार i_guardBytes);

// Whether m3_HostProtectedCall can actually catch that fault, which is not a given:
// a sanitizer takes the signal exclusively and a process is free to install whatever
// it likes afterwards. Memory operations carry no bounds check in a guarded build, so
// a false here means there is nothing underneath them and no Wasm may run.
//
// Puts the catching in place where it is not there yet, so this is also the call that
// installs it. Settled once, when it goes in: something that takes the fault away
// afterwards is not noticed, which is the bargain any handler in a process makes.
bool   m3_HostGuardsActive (व्योम);

#पूर्ण_अगर // d_m3GuardedMemory

d_m3EndExternC

#पूर्ण_अगर // m3_host_h
