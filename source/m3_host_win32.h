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

#அந்தர்பாவ "m3_core.h"

#எனில் !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#அந்தர்பாவ "m3_host.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#அந்தர்பாவ <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
வெறுமை* m3_Host_அடுக்கு_Base (வெறுமை)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    நிலையற்ற எழுத்து            here = 0;

    எனில் (VirtualQuery((LPCVOID)&here, &mbi, அளவு(mbi)) == 0) {
        திரும்பு வெற்று;
    }

    திரும்பு mbi.AllocationBase;
}

நிலையான IM3Runtime s_win32SuspendRuntime = வெற்று;

நிலையான
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    எனில் (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        எனில் (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            திரும்பு TRUE;
        }
    }
    திரும்பு FALSE;
}

வெறுமை m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

வெறுமை m3_HostRemoveInterruptHandler (வெறுமை)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = வெற்று;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (வெறுமை)
{
    நிலையான மாறிலி u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_உயர்வு_Date_Time << 32) | now.dw_கீழ்_Date_Time;

    திரும்பு (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

வெறுமை m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    போது (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (மாறிலி எழுத்து* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, வெற்று, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, வெற்று);
    எனில் (file == INVALID_HANDLE_மதிப்பு) {
        திரும்பு false;
    }

    LARGE_INTEGER size;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    எனில் (not GetFileSizeEx(file, &size) or size.QuadPart <= 0 or
        (ULONGLONG) size.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) size.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        திரும்பு m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, வெற்று, PAGE_READONLY, 0, 0, வெற்று);
    CloseHandle(file);                   // the mapping holds the file open

    எனில் (mapping == வெற்று) {
        திரும்பு m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    வெறுமை* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    எனில் (base == வெற்று) {
        திரும்பு m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->data = base;
    o_file->size = (size_t)size.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    திரும்பு true;
}

வெறுமை m3_HostUnmapFile (M3HostFile* io_file)
{
    எனில் (io_file->mapped) {
        UnmapViewOfFile(io_file->data);  // the view knows how big it is
    } இல்லையெனில் {
        m3_Free(io_file->data);
    }

    io_file->data = வெற்று;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (மாறிலி எழுத்து* i_from, மாறிலி எழுத்து* i_to)
{
    திரும்பு MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#எனில் d_m3GuardedMemory

size_t m3_HostPageSize (வெறுமை)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    திரும்பு info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

வெறுமை* m3_HostReserve (size_t i_bytes)
{
    திரும்பு VirtualAlloc(வெற்று, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (வெறுமை* i_address, size_t i_bytes)
{
    எனில் (i_bytes == 0) {
        திரும்பு true;
    }
    திரும்பு VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != வெற்று;
}

bool m3_HostDecommit (வெறுமை* i_address, size_t i_bytes)
{
    எனில் (i_bytes == 0) {
        திரும்பு true;
    }
    திரும்பு VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

வெறுமை m3_HostRelease (வெறுமை* i_address, size_t i_bytes)
{
    (வெறுமை)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
நிலையான
bool guard_fault (EXCEPTION_POINTERS* i_info, வெறுமை* i_கீழ், size_t i_bytes)
{
    எனில் (i_info == வெற்று or i_info->ExceptionRecord == வெற்று) {
        திரும்பு false;
    }

    மாறிலி EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    எனில் (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->எண்_Parameters < 2) {
        திரும்பு false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    மாறிலி u8* address = (மாறிலி u8*)record->ExceptionInformation[1];

    திரும்பு address >= (மாறிலி u8*)i_கீழ் and address < (மாறிலி u8*)i_கீழ் + i_bytes;
}

bool m3_HostGuardsActive (வெறுமை)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    திரும்பு true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (வெறுமை (*i_body)(வெறுமை*), வெறுமை* i_context,
                           வெறுமை* i_guard_கீழ், size_t i_guardBytes)
{
    __try {
        i_body(i_context);
        திரும்பு true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_கீழ், i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_தேடல்) {
        திரும்பு false;
    }
}

#endif // d_m3GuardedMemory

#endif // m3_host_win32_h
