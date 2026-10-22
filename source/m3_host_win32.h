//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_host_win32_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_host_win32_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಆದರೆ !defined(_WIN32)
#  error "This file is for Windows only"
#ಆದರೆ_ಅಂತ್ಯ

#ಅನ್ತರ್ಭಾವ "m3_host.h"

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ WIN32_LEAN_AND_ಸರಾಸರಿ
#  define WIN32_LEAN_AND_ಸರಾಸರಿ
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
ಶೂನ್ಯ* m3_Host_ರಾಶಿ_ತಳ (ಶೂನ್ಯ)
{
    ಸ್ಮರಣೆ_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    ಅಸ್ಥಿರ ಅಕ್ಷರ            ಇಲ್ಲಿ = 0;

    ಆದರೆ (VirtualQuery((LPCVOID)&ಇಲ್ಲಿ, &mbi, ಗಾತ್ರ(mbi)) == 0) {
        ಹಿಂತಿರುಗಿ ಖಾಲಿ;
    }

    ಹಿಂತಿರುಗಿ mbi.ಹಂಚಿಕೆ_ತಳ;
}

ಸ್ಥಿರ IM3Runtime s_win32SuspendRuntime = ಖಾಲಿ;

ಸ್ಥಿರ
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    ಆದರೆ (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        ಆದರೆ (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            ಹಿಂತಿರುಗಿ TRUE;
        }
    }
    ಹಿಂತಿರುಗಿ FALSE;
}

ಶೂನ್ಯ m3_Host_Install_ಅಡಚಣೆ_Handler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    ಗಣ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, TRUE);
}

ಶೂನ್ಯ m3_Host_Remove_ಅಡಚಣೆ_Handler (ಶೂನ್ಯ)
{
    ಗಣ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = ಖಾಲಿ;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_Host_ಸಮಯ_Ms (ಶೂನ್ಯ)
{
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    Get_System_ಸಮಯ_As_File_ಸಮಯ(&now);

    u64 ticks = ((u64)now.dw_ಮೇಲಿನ_Date_ಸಮಯ << 32) | now.dw_ಕೆಳಗಿನ_Date_ಸಮಯ;

    ಹಿಂತಿರುಗಿ (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

ಶೂನ್ಯ m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    ಆಗ (ms > 0) {
        DWORD ಹೆಜ್ಜೆ = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(ಹೆಜ್ಜೆ);
        ms -= ಹೆಜ್ಜೆ;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_ಮಾರ್ಗ, size_t i_max_ಬೈಟುಗಳು, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_ಮಾರ್ಗ, GENERIC_READ, FILE_SHARE_READ, ಖಾಲಿ, OPEN_EXISTING,
                              FILE_ATTRIBUTE_ಸಾಮಾನ್ಯ, ಖಾಲಿ);
    ಆದರೆ (file == INVALID_HANDLE_ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ false;
    }

    LARGE_INTEGER ಗಾತ್ರ_2;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    ಆದರೆ (not Get_File_ಗಾತ್ರ_Ex(file, &ಗಾತ್ರ_2) or ಗಾತ್ರ_2.QuadPart <= 0 or
        (ULONGLONG) ಗಾತ್ರ_2.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_max_ಬೈಟುಗಳು and (ULONGLONG) ಗಾತ್ರ_2.QuadPart > (ULONGLONG)i_max_ಬೈಟುಗಳು)) {
        CloseHandle(file);
        ಹಿಂತಿರುಗಿ m3_HostReadFile(i_ಮಾರ್ಗ, i_max_ಬೈಟುಗಳು, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, ಖಾಲಿ, ಪುಟ_READONLY, 0, 0, ಖಾಲಿ);
    CloseHandle(file);                   // the mapping holds the file open

    ಆದರೆ (mapping == ಖಾಲಿ) {
        ಹಿಂತಿರುಗಿ m3_HostReadFile(i_ಮಾರ್ಗ, i_max_ಬೈಟುಗಳು, o_file);
    }

    ಶೂನ್ಯ* ತಳ = Map_View_ದ_File(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    ಆದರೆ (ತಳ == ಖಾಲಿ) {
        ಹಿಂತಿರುಗಿ m3_HostReadFile(i_ಮಾರ್ಗ, i_max_ಬೈಟುಗಳು, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->ದತ್ತಾಂಶ = ತಳ;
    o_file->ಗಾತ್ರ_2 = (size_t)ಗಾತ್ರ_2.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    ಹಿಂತಿರುಗಿ true;
}

ಶೂನ್ಯ m3_HostUnmapFile (M3HostFile* io_file)
{
    ಆದರೆ (io_file->mapped) {
        Unmap_View_ದ_File(io_file->ದತ್ತಾಂಶ);  // the view knows how big it is
    } ಇಲ್ಲದಿದ್ದರೆ {
        m3_Free(io_file->ದತ್ತಾಂಶ);
    }

    io_file->ದತ್ತಾಂಶ = ಖಾಲಿ;
    io_file->ಗಾತ್ರ_2 = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_from, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_ಗಮ್ಯ)
{
    ಹಿಂತಿರುಗಿ MoveFileExA(i_from, i_ಗಮ್ಯ, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ

size_t m3_Host_ಪುಟ_ಗಾತ್ರ (ಶೂನ್ಯ)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    ಹಿಂತಿರುಗಿ info.dw_ಹಂಚಿಕೆ_Granularity ? info.dw_ಹಂಚಿಕೆ_Granularity : 65536;
}

ಶೂನ್ಯ* m3_HostReserve (size_t i_ಬೈಟುಗಳು)
{
    ಹಿಂತಿರುಗಿ VirtualAlloc(ಖಾಲಿ, i_ಬೈಟುಗಳು, MEM_RESERVE, ಪುಟ_NOACCESS);
}

bool m3_HostCommit (ಶೂನ್ಯ* i_ವಿಳಾಸ, size_t i_ಬೈಟುಗಳು)
{
    ಆದರೆ (i_ಬೈಟುಗಳು == 0) {
        ಹಿಂತಿರುಗಿ true;
    }
    ಹಿಂತಿರುಗಿ VirtualAlloc(i_ವಿಳಾಸ, i_ಬೈಟುಗಳು, MEM_COMMIT, ಪುಟ_READWRITE) != ಖಾಲಿ;
}

bool m3_HostDecommit (ಶೂನ್ಯ* i_ವಿಳಾಸ, size_t i_ಬೈಟುಗಳು)
{
    ಆದರೆ (i_ಬೈಟುಗಳು == 0) {
        ಹಿಂತಿರುಗಿ true;
    }
    ಹಿಂತಿರುಗಿ VirtualFree(i_ವಿಳಾಸ, i_ಬೈಟುಗಳು, MEM_DECOMMIT) != 0;
}

ಶೂನ್ಯ m3_HostRelease (ಶೂನ್ಯ* i_ವಿಳಾಸ, size_t i_ಬೈಟುಗಳು)
{
    (ಶೂನ್ಯ)i_ಬೈಟುಗಳು;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_ವಿಳಾಸ, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ಸ್ಥಿರ
bool guard_fault (EXCEPTION_ಸೂಚಕಗಳು* i_info, ಶೂನ್ಯ* i_ಕೆಳಗಿನ, size_t i_ಬೈಟುಗಳು)
{
    ಆದರೆ (i_info == ಖಾಲಿ or i_info->Exception_ದಾಖಲೆ == ಖಾಲಿ) {
        ಹಿಂತಿರುಗಿ false;
    }

    ಸ್ಥಿರಾಂಕ EXCEPTION_ದಾಖಲೆ* ದಾಖಲೆ = i_info->Exception_ದಾಖಲೆ;

    ಆದರೆ (ದಾಖಲೆ->Exception_ಸಂಕೇತ != EXCEPTION_ACCESS_VIOLATION or
        ದಾಖಲೆ->ಸಂಖ್ಯೆ_Parameters < 2) {
        ಹಿಂತಿರುಗಿ false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ಸ್ಥಿರಾಂಕ u8* ವಿಳಾಸ = (ಸ್ಥಿರಾಂಕ u8*)ದಾಖಲೆ->ExceptionInformation[1];

    ಹಿಂತಿರುಗಿ ವಿಳಾಸ >= (ಸ್ಥಿರಾಂಕ u8*)i_ಕೆಳಗಿನ and ವಿಳಾಸ < (ಸ್ಥಿರಾಂಕ u8*)i_ಕೆಳಗಿನ + i_ಬೈಟುಗಳು;
}

bool m3_Host_Guards_ಸಕ್ರಿಯ (ಶೂನ್ಯ)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    ಹಿಂತಿರುಗಿ true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (ಶೂನ್ಯ (*i_body)(ಶೂನ್ಯ*), ಶೂನ್ಯ* i_context,
                           ಶೂನ್ಯ* i_guard_ಕೆಳಗಿನ, size_t i_guard_ಬೈಟುಗಳು)
{
    __try {
        i_body(i_context);
        ಹಿಂತಿರುಗಿ true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_ಕೆಳಗಿನ, i_guard_ಬೈಟುಗಳು)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_ಹುಡುಕು) {
        ಹಿಂತಿರುಗಿ false;
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3GuardedMemory

#ಆದರೆ_ಅಂತ್ಯ // m3_host_win32_h
