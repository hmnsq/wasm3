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

#अन्तर्भाव "m3_core.h"

#सचे !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#अन्तर्भाव "m3_host.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#अन्तर्भाव <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
शून्य* m3_Host_राशिः_Base (शून्य)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    चञ्चल वर्ण            here = 0;

    सचे (VirtualQuery((LPCVOID)&here, &mbi, परिमाण(mbi)) == 0) {
        पटिनिवत्त रिक्त;
    }

    पटिनिवत्त mbi.AllocationBase;
}

स्थिर IM3Runtime s_win32SuspendRuntime = रिक्त;

स्थिर
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    सचे (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        सचे (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            पटिनिवत्त TRUE;
        }
    }
    पटिनिवत्त FALSE;
}

शून्य m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

शून्य m3_HostRemoveInterruptHandler (शून्य)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = रिक्त;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (शून्य)
{
    स्थिर ध्रुव u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_उच्चः_Date_Time << 32) | now.dw_नीचः_Date_Time;

    पटिनिवत्त (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

शून्य m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    याव (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ध्रुव वर्ण* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, रिक्त, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, रिक्त);
    सचे (file == INVALID_HANDLE_मानम्) {
        पटिनिवत्त false;
    }

    LARGE_INTEGER size;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    सचे (not GetFileSizeEx(file, &size) or size.QuadPart <= 0 or
        (ULONGLONG) size.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) size.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        पटिनिवत्त m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, रिक्त, PAGE_READONLY, 0, 0, रिक्त);
    CloseHandle(file);                   // the mapping holds the file open

    सचे (mapping == रिक्त) {
        पटिनिवत्त m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    शून्य* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    सचे (base == रिक्त) {
        पटिनिवत्त m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->data = base;
    o_file->size = (size_t)size.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    पटिनिवत्त true;
}

शून्य m3_HostUnmapFile (M3HostFile* io_file)
{
    सचे (io_file->mapped) {
        UnmapViewOfFile(io_file->data);  // the view knows how big it is
    } अञ्ञथा {
        m3_Free(io_file->data);
    }

    io_file->data = रिक्त;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ध्रुव वर्ण* i_from, ध्रुव वर्ण* i_to)
{
    पटिनिवत्त MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#सचे d_m3GuardedMemory

size_t m3_HostPageSize (शून्य)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    पटिनिवत्त info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

शून्य* m3_HostReserve (size_t i_bytes)
{
    पटिनिवत्त VirtualAlloc(रिक्त, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (शून्य* i_address, size_t i_bytes)
{
    सचे (i_bytes == 0) {
        पटिनिवत्त true;
    }
    पटिनिवत्त VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != रिक्त;
}

bool m3_HostDecommit (शून्य* i_address, size_t i_bytes)
{
    सचे (i_bytes == 0) {
        पटिनिवत्त true;
    }
    पटिनिवत्त VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

शून्य m3_HostRelease (शून्य* i_address, size_t i_bytes)
{
    (शून्य)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
स्थिर
bool guard_fault (EXCEPTION_POINTERS* i_info, शून्य* i_नीचः, size_t i_bytes)
{
    सचे (i_info == रिक्त or i_info->ExceptionRecord == रिक्त) {
        पटिनिवत्त false;
    }

    ध्रुव EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    सचे (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->सङ्ख्या_Parameters < 2) {
        पटिनिवत्त false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ध्रुव u8* address = (ध्रुव u8*)record->ExceptionInformation[1];

    पटिनिवत्त address >= (ध्रुव u8*)i_नीचः and address < (ध्रुव u8*)i_नीचः + i_bytes;
}

bool m3_HostGuardsActive (शून्य)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    पटिनिवत्त true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (शून्य (*i_body)(शून्य*), शून्य* i_context,
                           शून्य* i_guard_नीचः, size_t i_guardBytes)
{
    __try {
        i_body(i_context);
        पटिनिवत्त true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_नीचः, i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_अन्वेषणम्) {
        पटिनिवत्त false;
    }
}

#endif // d_m3GuardedMemory

#endif // m3_host_win32_h
