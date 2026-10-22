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

#ضمن "m3_core.h"

#ܐܢ !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#ضمن "m3_host.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ضمن <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
فراغ* m3_HostStackBase (فراغ)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    متطاير حرف            here = 0;

    ܐܢ (VirtualQuery((LPCVOID)&here, &mbi, حجم(mbi)) == 0) {
        ܦܢܐ عدم;
    }

    ܦܢܐ mbi.AllocationBase;
}

ساكن IM3Runtime s_win32SuspendRuntime = عدم;

ساكن
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    ܐܢ (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        ܐܢ (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            ܦܢܐ TRUE;
        }
    }
    ܦܢܐ FALSE;
}

فراغ m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

فراغ m3_HostRemoveInterruptHandler (فراغ)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = عدم;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (فراغ)
{
    ساكن ثابت u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dwHighDateTime << 32) | now.dwLowDateTime;

    ܦܢܐ (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

فراغ m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    ܟܡܐ (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ثابت حرف* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, عدم, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, عدم);
    ܐܢ (file == INVALID_HANDLE_VALUE) {
        ܦܢܐ false;
    }

    LARGE_INTEGER size;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    ܐܢ (not GetFileSizeEx(file, &size) or size.QuadPart <= 0 or
        (ULONGLONG) size.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) size.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        ܦܢܐ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, عدم, PAGE_READONLY, 0, 0, عدم);
    CloseHandle(file);                   // the mapping holds the file open

    ܐܢ (mapping == عدم) {
        ܦܢܐ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    فراغ* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    ܐܢ (base == عدم) {
        ܦܢܐ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->data = base;
    o_file->size = (size_t)size.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    ܦܢܐ true;
}

فراغ m3_HostUnmapFile (M3HostFile* io_file)
{
    ܐܢ (io_file->mapped) {
        UnmapViewOfFile(io_file->data);  // the view knows how big it is
    } ܐܚܪܢܐ {
        m3_Free(io_file->data);
    }

    io_file->data = عدم;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_to)
{
    ܦܢܐ MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#ܐܢ d_m3GuardedMemory

size_t m3_HostPageSize (فراغ)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    ܦܢܐ info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

فراغ* m3_HostReserve (size_t i_bytes)
{
    ܦܢܐ VirtualAlloc(عدم, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (فراغ* i_address, size_t i_bytes)
{
    ܐܢ (i_bytes == 0) {
        ܦܢܐ true;
    }
    ܦܢܐ VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != عدم;
}

bool m3_HostDecommit (فراغ* i_address, size_t i_bytes)
{
    ܐܢ (i_bytes == 0) {
        ܦܢܐ true;
    }
    ܦܢܐ VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

فراغ m3_HostRelease (فراغ* i_address, size_t i_bytes)
{
    (فراغ)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ساكن
bool guard_fault (EXCEPTION_POINTERS* i_info, فراغ* i_low, size_t i_bytes)
{
    ܐܢ (i_info == عدم or i_info->ExceptionRecord == عدم) {
        ܦܢܐ false;
    }

    ثابت EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    ܐܢ (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->NumberParameters < 2) {
        ܦܢܐ false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ثابت u8* address = (ثابت u8*)record->ExceptionInformation[1];

    ܦܢܐ address >= (ثابت u8*)i_low and address < (ثابت u8*)i_low + i_bytes;
}

bool m3_HostGuardsActive (فراغ)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    ܦܢܐ true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (فراغ (*i_body)(فراغ*), فراغ* i_context,
                           فراغ* i_guardLow, size_t i_guardBytes)
{
    __try {
        i_body(i_context);
        ܦܢܐ true;
    } __except (guard_fault(GetExceptionInformation(), i_guardLow, i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH) {
        ܦܢܐ false;
    }
}

#endif // d_m3GuardedMemory

#endif // m3_host_win32_h
