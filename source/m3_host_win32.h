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

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಯದಿ !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#ಅನ್ತರ್ಭಾವ "m3_host.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ಅನ್ತರ್ಭಾವ <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
ಶೂನ್ಯ* m3_Host_ರಾಶಿ_Base (ಶೂನ್ಯ)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    ಚಞ್ಚಲ ವರ್ಣ            here = 0;

    ಯದಿ (VirtualQuery((LPCVOID)&here, &mbi, ಪರಿಮಾಣ(mbi)) == 0) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪ್ರತ್ಯಾಗಮ mbi.AllocationBase;
}

ಸ್ಥಿರ IM3Runtime s_win32SuspendRuntime = ರಿಕ್ತ;

ಸ್ಥಿರ
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    ಯದಿ (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        ಯದಿ (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            ಪ್ರತ್ಯಾಗಮ TRUE;
        }
    }
    ಪ್ರತ್ಯಾಗಮ FALSE;
}

ಶೂನ್ಯ m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

ಶೂನ್ಯ m3_HostRemoveInterruptHandler (ಶೂನ್ಯ)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = ರಿಕ್ತ;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (ಶೂನ್ಯ)
{
    ಸ್ಥಿರ ಧ್ರುವ u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_ಮೇಲಿನ_Date_Time << 32) | now.dw_ಕೆಳಗಿನ_Date_Time;

    ಪ್ರತ್ಯಾಗಮ (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

ಶೂನ್ಯ m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    ಯಾವತ್ (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ಧ್ರುವ ವರ್ಣ* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, ರಿಕ್ತ, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, ರಿಕ್ತ);
    ಯದಿ (file == INVALID_HANDLE_ಮೌಲ್ಯ) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    LARGE_INTEGER size;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    ಯದಿ (not GetFileSizeEx(file, &size) or size.QuadPart <= 0 or
        (ULONGLONG) size.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) size.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        ಪ್ರತ್ಯಾಗಮ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, ರಿಕ್ತ, PAGE_READONLY, 0, 0, ರಿಕ್ತ);
    CloseHandle(file);                   // the mapping holds the file open

    ಯದಿ (mapping == ರಿಕ್ತ) {
        ಪ್ರತ್ಯಾಗಮ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    ಶೂನ್ಯ* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    ಯದಿ (base == ರಿಕ್ತ) {
        ಪ್ರತ್ಯಾಗಮ m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->data = base;
    o_file->size = (size_t)size.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    ಪ್ರತ್ಯಾಗಮ true;
}

ಶೂನ್ಯ m3_HostUnmapFile (M3HostFile* io_file)
{
    ಯದಿ (io_file->mapped) {
        UnmapViewOfFile(io_file->data);  // the view knows how big it is
    } ಅನ್ಯಥಾ {
        m3_Free(io_file->data);
    }

    io_file->data = ರಿಕ್ತ;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ಧ್ರುವ ವರ್ಣ* i_from, ಧ್ರುವ ವರ್ಣ* i_to)
{
    ಪ್ರತ್ಯಾಗಮ MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#ಯದಿ d_m3GuardedMemory

size_t m3_HostPageSize (ಶೂನ್ಯ)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    ಪ್ರತ್ಯಾಗಮ info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

ಶೂನ್ಯ* m3_HostReserve (size_t i_bytes)
{
    ಪ್ರತ್ಯಾಗಮ VirtualAlloc(ರಿಕ್ತ, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (ಶೂನ್ಯ* i_address, size_t i_bytes)
{
    ಯದಿ (i_bytes == 0) {
        ಪ್ರತ್ಯಾಗಮ true;
    }
    ಪ್ರತ್ಯಾಗಮ VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != ರಿಕ್ತ;
}

bool m3_HostDecommit (ಶೂನ್ಯ* i_address, size_t i_bytes)
{
    ಯದಿ (i_bytes == 0) {
        ಪ್ರತ್ಯಾಗಮ true;
    }
    ಪ್ರತ್ಯಾಗಮ VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

ಶೂನ್ಯ m3_HostRelease (ಶೂನ್ಯ* i_address, size_t i_bytes)
{
    (ಶೂನ್ಯ)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ಸ್ಥಿರ
bool guard_fault (EXCEPTION_POINTERS* i_info, ಶೂನ್ಯ* i_ಕೆಳಗಿನ, size_t i_bytes)
{
    ಯದಿ (i_info == ರಿಕ್ತ or i_info->ExceptionRecord == ರಿಕ್ತ) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಧ್ರುವ EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    ಯದಿ (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->ಸಂಖ್ಯೆ_Parameters < 2) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ಧ್ರುವ u8* address = (ಧ್ರುವ u8*)record->ExceptionInformation[1];

    ಪ್ರತ್ಯಾಗಮ address >= (ಧ್ರುವ u8*)i_ಕೆಳಗಿನ and address < (ಧ್ರುವ u8*)i_ಕೆಳಗಿನ + i_bytes;
}

bool m3_HostGuardsActive (ಶೂನ್ಯ)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    ಪ್ರತ್ಯಾಗಮ true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (ಶೂನ್ಯ (*i_body)(ಶೂನ್ಯ*), ಶೂನ್ಯ* i_context,
                           ಶೂನ್ಯ* i_guard_ಕೆಳಗಿನ, size_t i_guardBytes)
{
    __try {
        i_body(i_context);
        ಪ್ರತ್ಯಾಗಮ true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_ಕೆಳಗಿನ, i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_ಹುಡುಕು) {
        ಪ್ರತ್ಯಾಗಮ false;
    }
}

#endif // d_m3GuardedMemory

#endif // m3_host_win32_h
