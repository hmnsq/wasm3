//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#ifndef m3_host_win32_h
#define m3_host_win32_h

#ଅନ୍ତର୍ଭାଵ "m3_core.h"

#ଯଦି !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#ଅନ୍ତର୍ଭାଵ "m3_host.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ଅନ୍ତର୍ଭାଵ <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
ଶୂନ୍ୟ* m3_Host_ଗଦା_Base (ଶୂନ୍ୟ)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    ଅସ୍ଥିର ଅକ୍ଷର            here = 0;

    ଯଦି (VirtualQuery((LPCVOID)&here, &mbi, ଆକାର(mbi)) == 0) {
        ଫେରନ୍ତୁ ଖାଲି;
    }

    ଫେରନ୍ତୁ mbi.AllocationBase;
}

ସ୍ଥିର IM3Runtime s_win32SuspendRuntime = ଖାଲି;

ସ୍ଥିର
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    ଯଦି (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        ଯଦି (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            ଫେରନ୍ତୁ TRUE;
        }
    }
    ଫେରନ୍ତୁ FALSE;
}

ଶୂନ୍ୟ m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

ଶୂନ୍ୟ m3_HostRemoveInterruptHandler (ଶୂନ୍ୟ)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = ଖାଲି;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (ଶୂନ୍ୟ)
{
    ସ୍ଥିର ଅଚଳ u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_ଉପର_Date_Time << 32) | now.dw_ତଳ_Date_Time;

    ଫେରନ୍ତୁ (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

ଶୂନ୍ୟ m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    ଯେତେବେଳେ (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ଅଚଳ ଅକ୍ଷର* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, ଖାଲି, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, ଖାଲି);
    ଯଦି (file == INVALID_HANDLE_ମୂଲ୍ୟ) {
        ଫେରନ୍ତୁ false;
    }

    LARGE_INTEGER size;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    ଯଦି (not GetFileSizeEx(file, &size) or size.QuadPart <= 0 or
        (ULONGLONG) size.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) size.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        ଫେରନ୍ତୁ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, ଖାଲି, PAGE_READONLY, 0, 0, ଖାଲି);
    CloseHandle(file);                   // the mapping holds the file open

    ଯଦି (mapping == ଖାଲି) {
        ଫେରନ୍ତୁ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    ଶୂନ୍ୟ* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    ଯଦି (base == ଖାଲି) {
        ଫେରନ୍ତୁ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->data = base;
    o_file->size = (size_t)size.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    ଫେରନ୍ତୁ true;
}

ଶୂନ୍ୟ m3_HostUnmapFile (M3HostFile* io_file)
{
    ଯଦି (io_file->mapped) {
        UnmapViewOfFile(io_file->data);  // the view knows how big it is
    } ଅନ୍ୟଥା {
        m3_Free(io_file->data);
    }

    io_file->data = ଖାଲି;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ଅଚଳ ଅକ୍ଷର* i_from, ଅଚଳ ଅକ୍ଷର* i_to)
{
    ଫେରନ୍ତୁ MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#ଯଦି d_m3GuardedMemory

size_t m3_HostPageSize (ଶୂନ୍ୟ)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    ଫେରନ୍ତୁ info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

ଶୂନ୍ୟ* m3_HostReserve (size_t i_bytes)
{
    ଫେରନ୍ତୁ VirtualAlloc(ଖାଲି, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (ଶୂନ୍ୟ* i_address, size_t i_bytes)
{
    ଯଦି (i_bytes == 0) {
        ଫେରନ୍ତୁ true;
    }
    ଫେରନ୍ତୁ VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != ଖାଲି;
}

bool m3_HostDecommit (ଶୂନ୍ୟ* i_address, size_t i_bytes)
{
    ଯଦି (i_bytes == 0) {
        ଫେରନ୍ତୁ true;
    }
    ଫେରନ୍ତୁ VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

ଶୂନ୍ୟ m3_HostRelease (ଶୂନ୍ୟ* i_address, size_t i_bytes)
{
    (ଶୂନ୍ୟ)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ସ୍ଥିର
bool guard_fault (EXCEPTION_POINTERS* i_info, ଶୂନ୍ୟ* i_ତଳ, size_t i_bytes)
{
    ଯଦି (i_info == ଖାଲି or i_info->ExceptionRecord == ଖାଲି) {
        ଫେରନ୍ତୁ false;
    }

    ଅଚଳ EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    ଯଦି (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->ସଂଖ୍ୟା_Parameters < 2) {
        ଫେରନ୍ତୁ false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ଅଚଳ u8* address = (ଅଚଳ u8*)record->ExceptionInformation[1];

    ଫେରନ୍ତୁ address >= (ଅଚଳ u8*)i_ତଳ and address < (ଅଚଳ u8*)i_ତଳ + i_bytes;
}

bool m3_HostGuardsActive (ଶୂନ୍ୟ)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    ଫେରନ୍ତୁ true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (ଶୂନ୍ୟ (*i_body)(ଶୂନ୍ୟ*), ଶୂନ୍ୟ* i_context,
                           ଶୂନ୍ୟ* i_guard_ତଳ, size_t i_guardBytes)
{
    __try {
        i_body(i_context);
        ଫେରନ୍ତୁ true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_ତଳ, i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_ସନ୍ଧାନ) {
        ଫେରନ୍ତୁ false;
    }
}

#endif // d_m3GuardedMemory

#endif // m3_host_win32_h
