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

#അന്തര്ഭാവ "m3_core.h"

#എങ്കിൽ !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#അന്തര്ഭാവ "m3_host.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#അന്തര്ഭാവ <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
ശൂന്യം* m3_Host_അട്ടി_Base (ശൂന്യം)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    അസ്ഥിര അക്ഷരം            here = 0;

    എങ്കിൽ (VirtualQuery((LPCVOID)&here, &mbi, വലുപ്പം(mbi)) == 0) {
        തിരികെ ശൂന്യ;
    }

    തിരികെ mbi.AllocationBase;
}

സ്ഥിതം IM3Runtime s_win32SuspendRuntime = ശൂന്യ;

സ്ഥിതം
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    എങ്കിൽ (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        എങ്കിൽ (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            തിരികെ TRUE;
        }
    }
    തിരികെ FALSE;
}

ശൂന്യം m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

ശൂന്യം m3_HostRemoveInterruptHandler (ശൂന്യം)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = ശൂന്യ;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (ശൂന്യം)
{
    സ്ഥിതം സ്ഥിരം u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_മുകളിലെ_Date_Time << 32) | now.dw_താഴെയുള്ള_Date_Time;

    തിരികെ (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

ശൂന്യം m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    ആകുമ്പോൾ (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (സ്ഥിരം അക്ഷരം* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, ശൂന്യ, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, ശൂന്യ);
    എങ്കിൽ (file == INVALID_HANDLE_മൂല്യം) {
        തിരികെ false;
    }

    LARGE_INTEGER size;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    എങ്കിൽ (not GetFileSizeEx(file, &size) or size.QuadPart <= 0 or
        (ULONGLONG) size.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) size.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        തിരികെ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, ശൂന്യ, PAGE_READONLY, 0, 0, ശൂന്യ);
    CloseHandle(file);                   // the mapping holds the file open

    എങ്കിൽ (mapping == ശൂന്യ) {
        തിരികെ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    ശൂന്യം* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    എങ്കിൽ (base == ശൂന്യ) {
        തിരികെ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->data = base;
    o_file->size = (size_t)size.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    തിരികെ true;
}

ശൂന്യം m3_HostUnmapFile (M3HostFile* io_file)
{
    എങ്കിൽ (io_file->mapped) {
        UnmapViewOfFile(io_file->data);  // the view knows how big it is
    } അല്ലെങ്കിൽ {
        m3_Free(io_file->data);
    }

    io_file->data = ശൂന്യ;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (സ്ഥിരം അക്ഷരം* i_from, സ്ഥിരം അക്ഷരം* i_to)
{
    തിരികെ MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#എങ്കിൽ d_m3GuardedMemory

size_t m3_HostPageSize (ശൂന്യം)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    തിരികെ info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

ശൂന്യം* m3_HostReserve (size_t i_bytes)
{
    തിരികെ VirtualAlloc(ശൂന്യ, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (ശൂന്യം* i_address, size_t i_bytes)
{
    എങ്കിൽ (i_bytes == 0) {
        തിരികെ true;
    }
    തിരികെ VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != ശൂന്യ;
}

bool m3_HostDecommit (ശൂന്യം* i_address, size_t i_bytes)
{
    എങ്കിൽ (i_bytes == 0) {
        തിരികെ true;
    }
    തിരികെ VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

ശൂന്യം m3_HostRelease (ശൂന്യം* i_address, size_t i_bytes)
{
    (ശൂന്യം)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
സ്ഥിതം
bool guard_fault (EXCEPTION_POINTERS* i_info, ശൂന്യം* i_താഴെയുള്ള, size_t i_bytes)
{
    എങ്കിൽ (i_info == ശൂന്യ or i_info->ExceptionRecord == ശൂന്യ) {
        തിരികെ false;
    }

    സ്ഥിരം EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    എങ്കിൽ (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->സംഖ്യ_Parameters < 2) {
        തിരികെ false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    സ്ഥിരം u8* address = (സ്ഥിരം u8*)record->ExceptionInformation[1];

    തിരികെ address >= (സ്ഥിരം u8*)i_താഴെയുള്ള and address < (സ്ഥിരം u8*)i_താഴെയുള്ള + i_bytes;
}

bool m3_HostGuardsActive (ശൂന്യം)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    തിരികെ true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (ശൂന്യം (*i_body)(ശൂന്യം*), ശൂന്യം* i_context,
                           ശൂന്യം* i_guard_താഴെയുള്ള, size_t i_guardBytes)
{
    __try {
        i_body(i_context);
        തിരികെ true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_താഴെയുള്ള, i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_തിരയൽ) {
        തിരികെ false;
    }
}

#endif // d_m3GuardedMemory

#endif // m3_host_win32_h
