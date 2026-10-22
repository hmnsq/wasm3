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

#অন্তর্ভাব "m3_core.h"

#যদি !defined(_WIN32)
#  error "This file is for Windows only"
#endif

#অন্তর্ভাব "m3_host.h"

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#অন্তর্ভাব <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
শূন্য* m3_Host_স্তূপ_Base (শূন্য)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    চঞ্চল বর্ণ            here = 0;

    যদি (VirtualQuery((LPCVOID)&here, &mbi, পরিমাণ(mbi)) == 0) {
        প্রত্যাগম রিক্ত;
    }

    প্রত্যাগম mbi.AllocationBase;
}

স্থির IM3Runtime s_win32SuspendRuntime = রিক্ত;

স্থির
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    যদি (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        যদি (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            প্রত্যাগম TRUE;
        }
    }
    প্রত্যাগম FALSE;
}

শূন্য m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

শূন্য m3_HostRemoveInterruptHandler (শূন্য)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = রিক্ত;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (শূন্য)
{
    স্থির ধ্রুব u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_উপরের_Date_Time << 32) | now.dw_নিচের_Date_Time;

    প্রত্যাগম (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

শূন্য m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    যাবত্ (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ধ্রুব বর্ণ* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, রিক্ত, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, রিক্ত);
    যদি (file == INVALID_HANDLE_মান) {
        প্রত্যাগম false;
    }

    LARGE_INTEGER size;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    যদি (not GetFileSizeEx(file, &size) or size.QuadPart <= 0 or
        (ULONGLONG) size.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) size.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        প্রত্যাগম m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, রিক্ত, PAGE_READONLY, 0, 0, রিক্ত);
    CloseHandle(file);                   // the mapping holds the file open

    যদি (mapping == রিক্ত) {
        প্রত্যাগম m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    শূন্য* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    যদি (base == রিক্ত) {
        প্রত্যাগম m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->data = base;
    o_file->size = (size_t)size.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    প্রত্যাগম true;
}

শূন্য m3_HostUnmapFile (M3HostFile* io_file)
{
    যদি (io_file->mapped) {
        UnmapViewOfFile(io_file->data);  // the view knows how big it is
    } অন্যথা {
        m3_Free(io_file->data);
    }

    io_file->data = রিক্ত;
    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ধ্রুব বর্ণ* i_from, ধ্রুব বর্ণ* i_to)
{
    প্রত্যাগম MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#যদি d_m3GuardedMemory

size_t m3_HostPageSize (শূন্য)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    প্রত্যাগম info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

শূন্য* m3_HostReserve (size_t i_bytes)
{
    প্রত্যাগম VirtualAlloc(রিক্ত, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (শূন্য* i_address, size_t i_bytes)
{
    যদি (i_bytes == 0) {
        প্রত্যাগম true;
    }
    প্রত্যাগম VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != রিক্ত;
}

bool m3_HostDecommit (শূন্য* i_address, size_t i_bytes)
{
    যদি (i_bytes == 0) {
        প্রত্যাগম true;
    }
    প্রত্যাগম VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

শূন্য m3_HostRelease (শূন্য* i_address, size_t i_bytes)
{
    (শূন্য)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
স্থির
bool guard_fault (EXCEPTION_POINTERS* i_info, শূন্য* i_নিচের, size_t i_bytes)
{
    যদি (i_info == রিক্ত or i_info->ExceptionRecord == রিক্ত) {
        প্রত্যাগম false;
    }

    ধ্রুব EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    যদি (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->সংখ্যা_Parameters < 2) {
        প্রত্যাগম false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ধ্রুব u8* address = (ধ্রুব u8*)record->ExceptionInformation[1];

    প্রত্যাগম address >= (ধ্রুব u8*)i_নিচের and address < (ধ্রুব u8*)i_নিচের + i_bytes;
}

bool m3_HostGuardsActive (শূন্য)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    প্রত্যাগম true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (শূন্য (*i_body)(শূন্য*), শূন্য* i_context,
                           শূন্য* i_guard_নিচের, size_t i_guardBytes)
{
    __try {
        i_body(i_context);
        প্রত্যাগম true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_নিচের, i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_অনুসন্ধান) {
        প্রত্যাগম false;
    }
}

#endif // d_m3GuardedMemory

#endif // m3_host_win32_h
