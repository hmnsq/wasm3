//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_host_win32_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_host_win32_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

#ਜੇ !defined(_WIN32)
#  error "This file is for Windows only"
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_host.h"

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ WIN32_LEAN_AND_ਔਸਤ
#  define WIN32_LEAN_AND_ਔਸਤ
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
ਖਾਲੀ* m3_Host_ਢੇਰ_ਅਧਾਰ (ਖਾਲੀ)
{
    ਸਮ੍ਰਿਤੀ_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    ਅਸਥਿਰ ਅੱਖਰ            ਇੱਥੇ = 0;

    ਜੇ (VirtualQuery((LPCVOID)&ਇੱਥੇ, &mbi, ਆਕਾਰ(mbi)) == 0) {
        ਵਾਪਸ ਨਿਰਾ;
    }

    ਵਾਪਸ mbi.ਵੰਡ_ਅਧਾਰ;
}

ਸਥਿਰ IM3Runtime s_win32SuspendRuntime = ਨਿਰਾ;

ਸਥਿਰ
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    ਜੇ (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        ਜੇ (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            ਵਾਪਸ TRUE;
        }
    }
    ਵਾਪਸ FALSE;
}

ਖਾਲੀ m3_Host_Install_ਰੁਕਾਵਟ_Handler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    ਸੈੱਟ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, TRUE);
}

ਖਾਲੀ m3_Host_Remove_ਰੁਕਾਵਟ_Handler (ਖਾਲੀ)
{
    ਸੈੱਟ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = ਨਿਰਾ;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_Host_ਸਮਾਂ_Ms (ਖਾਲੀ)
{
    ਸਥਿਰ ਅਚਲ u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    Get_System_ਸਮਾਂ_As_File_ਸਮਾਂ(&now);

    u64 ticks = ((u64)now.dw_ਉੱਚਾ_Date_ਸਮਾਂ << 32) | now.dw_ਨੀਵਾਂ_Date_ਸਮਾਂ;

    ਵਾਪਸ (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

ਖਾਲੀ m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    ਜਦੋਂ (ms > 0) {
        DWORD ਕਦਮ = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(ਕਦਮ);
        ms -= ਕਦਮ;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ਅਚਲ ਅੱਖਰ* i_ਰਸਤਾ, size_t i_max_ਬਾਈਟਾਂ, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_ਰਸਤਾ, GENERIC_READ, FILE_SHARE_READ, ਨਿਰਾ, OPEN_EXISTING,
                              FILE_ATTRIBUTE_ਸਾਧਾਰਨ, ਨਿਰਾ);
    ਜੇ (file == INVALID_HANDLE_ਮੁੱਲ) {
        ਵਾਪਸ false;
    }

    LARGE_INTEGER ਮਾਪ;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    ਜੇ (not Get_File_ਮਾਪ_Ex(file, &ਮਾਪ) or ਮਾਪ.QuadPart <= 0 or
        (ULONGLONG) ਮਾਪ.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_max_ਬਾਈਟਾਂ and (ULONGLONG) ਮਾਪ.QuadPart > (ULONGLONG)i_max_ਬਾਈਟਾਂ)) {
        CloseHandle(file);
        ਵਾਪਸ m3_HostReadFile(i_ਰਸਤਾ, i_max_ਬਾਈਟਾਂ, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, ਨਿਰਾ, ਪੰਨਾ_READONLY, 0, 0, ਨਿਰਾ);
    CloseHandle(file);                   // the mapping holds the file open

    ਜੇ (mapping == ਨਿਰਾ) {
        ਵਾਪਸ m3_HostReadFile(i_ਰਸਤਾ, i_max_ਬਾਈਟਾਂ, o_file);
    }

    ਖਾਲੀ* ਅਧਾਰ = Map_View_ਦਾ_File(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    ਜੇ (ਅਧਾਰ == ਨਿਰਾ) {
        ਵਾਪਸ m3_HostReadFile(i_ਰਸਤਾ, i_max_ਬਾਈਟਾਂ, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->ਅੰਕੜੇ = ਅਧਾਰ;
    o_file->ਮਾਪ = (size_t)ਮਾਪ.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    ਵਾਪਸ true;
}

ਖਾਲੀ m3_HostUnmapFile (M3HostFile* io_file)
{
    ਜੇ (io_file->mapped) {
        Unmap_View_ਦਾ_File(io_file->ਅੰਕੜੇ);  // the view knows how big it is
    } ਨਹੀਂਤਾਂ {
        m3_Free(io_file->ਅੰਕੜੇ);
    }

    io_file->ਅੰਕੜੇ = ਨਿਰਾ;
    io_file->ਮਾਪ = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ਅਚਲ ਅੱਖਰ* i_from, ਅਚਲ ਅੱਖਰ* i_ਮੰਜ਼ਿਲ)
{
    ਵਾਪਸ MoveFileExA(i_from, i_ਮੰਜ਼ਿਲ, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ

size_t m3_Host_ਪੰਨਾ_ਮਾਪ (ਖਾਲੀ)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    ਵਾਪਸ info.dw_ਵੰਡ_Granularity ? info.dw_ਵੰਡ_Granularity : 65536;
}

ਖਾਲੀ* m3_HostReserve (size_t i_ਬਾਈਟਾਂ)
{
    ਵਾਪਸ VirtualAlloc(ਨਿਰਾ, i_ਬਾਈਟਾਂ, MEM_RESERVE, ਪੰਨਾ_NOACCESS);
}

bool m3_HostCommit (ਖਾਲੀ* i_ਪਤਾ, size_t i_ਬਾਈਟਾਂ)
{
    ਜੇ (i_ਬਾਈਟਾਂ == 0) {
        ਵਾਪਸ true;
    }
    ਵਾਪਸ VirtualAlloc(i_ਪਤਾ, i_ਬਾਈਟਾਂ, MEM_COMMIT, ਪੰਨਾ_READWRITE) != ਨਿਰਾ;
}

bool m3_HostDecommit (ਖਾਲੀ* i_ਪਤਾ, size_t i_ਬਾਈਟਾਂ)
{
    ਜੇ (i_ਬਾਈਟਾਂ == 0) {
        ਵਾਪਸ true;
    }
    ਵਾਪਸ VirtualFree(i_ਪਤਾ, i_ਬਾਈਟਾਂ, MEM_DECOMMIT) != 0;
}

ਖਾਲੀ m3_HostRelease (ਖਾਲੀ* i_ਪਤਾ, size_t i_ਬਾਈਟਾਂ)
{
    (ਖਾਲੀ)i_ਬਾਈਟਾਂ;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_ਪਤਾ, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ਸਥਿਰ
bool guard_fault (EXCEPTION_ਸੂਚਕ_ਸਾਰੇ* i_info, ਖਾਲੀ* i_ਨੀਵਾਂ, size_t i_ਬਾਈਟਾਂ)
{
    ਜੇ (i_info == ਨਿਰਾ or i_info->Exception_ਰਿਕਾਰਡ == ਨਿਰਾ) {
        ਵਾਪਸ false;
    }

    ਅਚਲ EXCEPTION_ਰਿਕਾਰਡ* ਰਿਕਾਰਡ = i_info->Exception_ਰਿਕਾਰਡ;

    ਜੇ (ਰਿਕਾਰਡ->Exception_ਕੋਡ != EXCEPTION_ACCESS_VIOLATION or
        ਰਿਕਾਰਡ->ਸੰਖਿਆ_Parameters < 2) {
        ਵਾਪਸ false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ਅਚਲ u8* ਪਤਾ = (ਅਚਲ u8*)ਰਿਕਾਰਡ->ExceptionInformation[1];

    ਵਾਪਸ ਪਤਾ >= (ਅਚਲ u8*)i_ਨੀਵਾਂ and ਪਤਾ < (ਅਚਲ u8*)i_ਨੀਵਾਂ + i_ਬਾਈਟਾਂ;
}

bool m3_Host_Guards_ਸਰਗਰਮ (ਖਾਲੀ)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    ਵਾਪਸ true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (ਖਾਲੀ (*i_body)(ਖਾਲੀ*), ਖਾਲੀ* i_context,
                           ਖਾਲੀ* i_guard_ਨੀਵਾਂ, size_t i_guard_ਬਾਈਟਾਂ)
{
    __try {
        i_body(i_context);
        ਵਾਪਸ true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_ਨੀਵਾਂ, i_guard_ਬਾਈਟਾਂ)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_ਖੋਜ) {
        ਵਾਪਸ false;
    }
}

#ਜੇ_ਅੰਤ // d_m3GuardedMemory

#ਜੇ_ਅੰਤ // m3_host_win32_h
