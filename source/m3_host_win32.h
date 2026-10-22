//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#جيڪڏهن_وصف_نه_ٿيل m3_host_win32_h
#وصف_ڪريو m3_host_win32_h

#ضمن "m3_core.h"

#إذا !defined(_WIN32)
#  error "This file is for Windows only"
#جيڪڏهن_پورو

#ضمن "m3_host.h"

#جيڪڏهن_وصف_نه_ٿيل WIN32_LEAN_AND_سراسري
#  define WIN32_LEAN_AND_سراسري
#جيڪڏهن_پورو
#ضمن <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
فراغ* m3_Host_ڍير_بنياد (فراغ)
{
    يادگيري_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    متطاير حرف            هتي = 0;

    إذا (VirtualQuery((LPCVOID)&هتي, &mbi, حجم(mbi)) == 0) {
        أرجع عدم;
    }

    أرجع mbi.ورهاست_بنياد;
}

ساكن IM3Runtime s_win32SuspendRuntime = عدم;

ساكن
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    إذا (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        إذا (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            أرجع TRUE;
        }
    }
    أرجع FALSE;
}

فراغ m3_Host_Install_مداخلت_Handler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    سيٽ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, TRUE);
}

فراغ m3_Host_Remove_مداخلت_Handler (فراغ)
{
    سيٽ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = عدم;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_Host_وقت_Ms (فراغ)
{
    ساكن ثابت u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    Get_System_وقت_As_File_وقت(&now);

    u64 ticks = ((u64)now.dw_مٿاهون_Date_وقت << 32) | now.dw_هيٺيون_Date_وقت;

    أرجع (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

فراغ m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    طالما (ms > 0) {
        DWORD قدم = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(قدم);
        ms -= قدم;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ثابت حرف* i_رستو, size_t i_max_بائيٽون, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_رستو, GENERIC_READ, FILE_SHARE_READ, عدم, OPEN_EXISTING,
                              FILE_ATTRIBUTE_عام, عدم);
    إذا (file == INVALID_HANDLE_قدر) {
        أرجع false;
    }

    LARGE_INTEGER سائيز;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    إذا (not Get_File_سائيز_Ex(file, &سائيز) or سائيز.QuadPart <= 0 or
        (ULONGLONG) سائيز.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_max_بائيٽون and (ULONGLONG) سائيز.QuadPart > (ULONGLONG)i_max_بائيٽون)) {
        CloseHandle(file);
        أرجع m3_HostReadFile(i_رستو, i_max_بائيٽون, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, عدم, صفحو_READONLY, 0, 0, عدم);
    CloseHandle(file);                   // the mapping holds the file open

    إذا (mapping == عدم) {
        أرجع m3_HostReadFile(i_رستو, i_max_بائيٽون, o_file);
    }

    فراغ* بنياد = Map_View_جو_File(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    إذا (بنياد == عدم) {
        أرجع m3_HostReadFile(i_رستو, i_max_بائيٽون, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->معلومات = بنياد;
    o_file->سائيز = (size_t)سائيز.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    أرجع true;
}

فراغ m3_HostUnmapFile (M3HostFile* io_file)
{
    إذا (io_file->mapped) {
        Unmap_View_جو_File(io_file->معلومات);  // the view knows how big it is
    } وإلا {
        m3_Free(io_file->معلومات);
    }

    io_file->معلومات = عدم;
    io_file->سائيز = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_منزل)
{
    أرجع MoveFileExA(i_from, i_منزل, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#إذا d_m3_Guarded_يادگيري

size_t m3_Host_صفحو_سائيز (فراغ)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    أرجع info.dw_ورهاست_Granularity ? info.dw_ورهاست_Granularity : 65536;
}

فراغ* m3_HostReserve (size_t i_بائيٽون)
{
    أرجع VirtualAlloc(عدم, i_بائيٽون, MEM_RESERVE, صفحو_NOACCESS);
}

bool m3_HostCommit (فراغ* i_پتو, size_t i_بائيٽون)
{
    إذا (i_بائيٽون == 0) {
        أرجع true;
    }
    أرجع VirtualAlloc(i_پتو, i_بائيٽون, MEM_COMMIT, صفحو_READWRITE) != عدم;
}

bool m3_HostDecommit (فراغ* i_پتو, size_t i_بائيٽون)
{
    إذا (i_بائيٽون == 0) {
        أرجع true;
    }
    أرجع VirtualFree(i_پتو, i_بائيٽون, MEM_DECOMMIT) != 0;
}

فراغ m3_HostRelease (فراغ* i_پتو, size_t i_بائيٽون)
{
    (فراغ)i_بائيٽون;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_پتو, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ساكن
bool guard_fault (EXCEPTION_اشارا* i_info, فراغ* i_هيٺيون, size_t i_بائيٽون)
{
    إذا (i_info == عدم or i_info->Exception_رڪارڊ == عدم) {
        أرجع false;
    }

    ثابت EXCEPTION_رڪارڊ* رڪارڊ = i_info->Exception_رڪارڊ;

    إذا (رڪارڊ->Exception_ڪوڊ != EXCEPTION_ACCESS_VIOLATION or
        رڪارڊ->عدد_Parameters < 2) {
        أرجع false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ثابت u8* پتو = (ثابت u8*)رڪارڊ->ExceptionInformation[1];

    أرجع پتو >= (ثابت u8*)i_هيٺيون and پتو < (ثابت u8*)i_هيٺيون + i_بائيٽون;
}

bool m3_Host_Guards_سرگرم (فراغ)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    أرجع true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (فراغ (*i_body)(فراغ*), فراغ* i_context,
                           فراغ* i_guard_هيٺيون, size_t i_guard_بائيٽون)
{
    __try {
        i_body(i_context);
        أرجع true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_هيٺيون, i_guard_بائيٽون)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_ڳولا) {
        أرجع false;
    }
}

#جيڪڏهن_پورو // d_m3GuardedMemory

#جيڪڏهن_پورو // m3_host_win32_h
