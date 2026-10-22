//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#اگر_تعریف_نشده m3_host_win32_h
#تعریف_کن m3_host_win32_h

#ضمن "m3_core.h"

#إذا !defined(_WIN32)
#  error "This file is for Windows only"
#پایان_اگر

#ضمن "m3_host.h"

#اگر_تعریف_نشده WIN32_LEAN_AND_میانگین
#  define WIN32_LEAN_AND_میانگین
#پایان_اگر
#ضمن <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
فراغ* m3_Host_پشته_پایه (فراغ)
{
    حافظه_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    متطاير حرف            اینجا = 0;

    إذا (VirtualQuery((LPCVOID)&اینجا, &mbi, حجم(mbi)) == 0) {
        أرجع عدم;
    }

    أرجع mbi.تخصیص_پایه;
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

فراغ m3_Host_Install_وقفه_Handler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    مجموعه_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, TRUE);
}

فراغ m3_Host_Remove_وقفه_Handler (فراغ)
{
    مجموعه_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = عدم;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_Host_زمان_Ms (فراغ)
{
    ساكن ثابت u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    Get_System_زمان_As_File_زمان(&now);

    u64 ticks = ((u64)now.dw_بالا_Date_زمان << 32) | now.dw_کم_Date_زمان;

    أرجع (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

فراغ m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    طالما (ms > 0) {
        DWORD گام = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(گام);
        ms -= گام;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ثابت حرف* i_مسیر, size_t i_max_بایتها, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_مسیر, GENERIC_READ, FILE_SHARE_READ, عدم, OPEN_EXISTING,
                              FILE_ATTRIBUTE_نرمال, عدم);
    إذا (file == INVALID_HANDLE_مقدار) {
        أرجع false;
    }

    LARGE_INTEGER اندازه;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    إذا (not Get_File_اندازه_Ex(file, &اندازه) or اندازه.QuadPart <= 0 or
        (ULONGLONG) اندازه.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_max_بایتها and (ULONGLONG) اندازه.QuadPart > (ULONGLONG)i_max_بایتها)) {
        CloseHandle(file);
        أرجع m3_HostReadFile(i_مسیر, i_max_بایتها, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, عدم, صفحه_READONLY, 0, 0, عدم);
    CloseHandle(file);                   // the mapping holds the file open

    إذا (mapping == عدم) {
        أرجع m3_HostReadFile(i_مسیر, i_max_بایتها, o_file);
    }

    فراغ* پایه = Map_View_از_File(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    إذا (پایه == عدم) {
        أرجع m3_HostReadFile(i_مسیر, i_max_بایتها, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->داده = پایه;
    o_file->اندازه = (size_t)اندازه.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    أرجع true;
}

فراغ m3_HostUnmapFile (M3HostFile* io_file)
{
    إذا (io_file->mapped) {
        Unmap_View_از_File(io_file->داده);  // the view knows how big it is
    } وإلا {
        m3_Free(io_file->داده);
    }

    io_file->داده = عدم;
    io_file->اندازه = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_مقصد)
{
    أرجع MoveFileExA(i_from, i_مقصد, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#إذا d_m3_Guarded_حافظه

size_t m3_Host_صفحه_اندازه (فراغ)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    أرجع info.dw_تخصیص_Granularity ? info.dw_تخصیص_Granularity : 65536;
}

فراغ* m3_HostReserve (size_t i_بایتها)
{
    أرجع VirtualAlloc(عدم, i_بایتها, MEM_RESERVE, صفحه_NOACCESS);
}

bool m3_HostCommit (فراغ* i_آدرس, size_t i_بایتها)
{
    إذا (i_بایتها == 0) {
        أرجع true;
    }
    أرجع VirtualAlloc(i_آدرس, i_بایتها, MEM_COMMIT, صفحه_READWRITE) != عدم;
}

bool m3_HostDecommit (فراغ* i_آدرس, size_t i_بایتها)
{
    إذا (i_بایتها == 0) {
        أرجع true;
    }
    أرجع VirtualFree(i_آدرس, i_بایتها, MEM_DECOMMIT) != 0;
}

فراغ m3_HostRelease (فراغ* i_آدرس, size_t i_بایتها)
{
    (فراغ)i_بایتها;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_آدرس, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ساكن
bool guard_fault (EXCEPTION_اشارهگرها* i_info, فراغ* i_کم, size_t i_بایتها)
{
    إذا (i_info == عدم or i_info->Exception_رکورد == عدم) {
        أرجع false;
    }

    ثابت EXCEPTION_رکورد* رکورد = i_info->Exception_رکورد;

    إذا (رکورد->Exception_کد != EXCEPTION_ACCESS_VIOLATION or
        رکورد->عدد_Parameters < 2) {
        أرجع false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ثابت u8* آدرس = (ثابت u8*)رکورد->ExceptionInformation[1];

    أرجع آدرس >= (ثابت u8*)i_کم and آدرس < (ثابت u8*)i_کم + i_بایتها;
}

bool m3_Host_Guards_فعال (فراغ)
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
                           فراغ* i_guard_کم, size_t i_guard_بایتها)
{
    __try {
        i_body(i_context);
        أرجع true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_کم, i_guard_بایتها)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_جستجو) {
        أرجع false;
    }
}

#پایان_اگر // d_m3GuardedMemory

#پایان_اگر // m3_host_win32_h
