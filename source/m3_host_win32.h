//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#اگر_غیر_تعریف_شدہ m3_host_win32_h
#تعریف_کرو m3_host_win32_h

#شامل "m3_core.h"

#اگر !defined(_WIN32)
#  error "This file is for Windows only"
#اگر_ختم

#شامل "m3_host.h"

#اگر_غیر_تعریف_شدہ WIN32_LEAN_AND_اوسط
#  define WIN32_LEAN_AND_اوسط
#اگر_ختم
#شامل <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
خالی* m3_Host_ڈھیر_بنیاد (خالی)
{
    حافظہ_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    متلون حرف            یہاں = 0;

    اگر (VirtualQuery((LPCVOID)&یہاں, &mbi, حجم(mbi)) == 0) {
        واپس تہی;
    }

    واپس mbi.تخصیص_بنیاد;
}

ساکن IM3Runtime s_win32SuspendRuntime = تہی;

ساکن
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    اگر (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        اگر (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            واپس TRUE;
        }
    }
    واپس FALSE;
}

خالی m3_Host_Install_مداخلت_Handler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    سیٹ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, TRUE);
}

خالی m3_Host_Remove_مداخلت_Handler (خالی)
{
    سیٹ_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = تہی;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_Host_وقت_Ms (خالی)
{
    ساکن ثابت u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    Get_System_وقت_As_File_وقت(&now);

    u64 ticks = ((u64)now.dw_بلند_Date_وقت << 32) | now.dw_کم_Date_وقت;

    واپس (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

خالی m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    جب (ms > 0) {
        DWORD قدم = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(قدم);
        ms -= قدم;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ثابت حرف* i_راستہ, size_t i_max_بائٹیں, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_راستہ, GENERIC_READ, FILE_SHARE_READ, تہی, OPEN_EXISTING,
                              FILE_ATTRIBUTE_معمول, تہی);
    اگر (file == INVALID_HANDLE_قدر) {
        واپس false;
    }

    LARGE_INTEGER سائز;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    اگر (not Get_File_سائز_Ex(file, &سائز) or سائز.QuadPart <= 0 or
        (ULONGLONG) سائز.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_max_بائٹیں and (ULONGLONG) سائز.QuadPart > (ULONGLONG)i_max_بائٹیں)) {
        CloseHandle(file);
        واپس m3_HostReadFile(i_راستہ, i_max_بائٹیں, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, تہی, صفحہ_READONLY, 0, 0, تہی);
    CloseHandle(file);                   // the mapping holds the file open

    اگر (mapping == تہی) {
        واپس m3_HostReadFile(i_راستہ, i_max_بائٹیں, o_file);
    }

    خالی* بنیاد = Map_View_کا_File(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    اگر (بنیاد == تہی) {
        واپس m3_HostReadFile(i_راستہ, i_max_بائٹیں, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->ڈیٹا = بنیاد;
    o_file->سائز = (size_t)سائز.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    واپس true;
}

خالی m3_HostUnmapFile (M3HostFile* io_file)
{
    اگر (io_file->mapped) {
        Unmap_View_کا_File(io_file->ڈیٹا);  // the view knows how big it is
    } ورنہ {
        m3_Free(io_file->ڈیٹا);
    }

    io_file->ڈیٹا = تہی;
    io_file->سائز = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_مقصد)
{
    واپس MoveFileExA(i_from, i_مقصد, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#اگر d_m3_Guarded_حافظہ

size_t m3_Host_صفحہ_سائز (خالی)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    واپس info.dw_تخصیص_Granularity ? info.dw_تخصیص_Granularity : 65536;
}

خالی* m3_HostReserve (size_t i_بائٹیں)
{
    واپس VirtualAlloc(تہی, i_بائٹیں, MEM_RESERVE, صفحہ_NOACCESS);
}

bool m3_HostCommit (خالی* i_پتہ, size_t i_بائٹیں)
{
    اگر (i_بائٹیں == 0) {
        واپس true;
    }
    واپس VirtualAlloc(i_پتہ, i_بائٹیں, MEM_COMMIT, صفحہ_READWRITE) != تہی;
}

bool m3_HostDecommit (خالی* i_پتہ, size_t i_بائٹیں)
{
    اگر (i_بائٹیں == 0) {
        واپس true;
    }
    واپس VirtualFree(i_پتہ, i_بائٹیں, MEM_DECOMMIT) != 0;
}

خالی m3_HostRelease (خالی* i_پتہ, size_t i_بائٹیں)
{
    (خالی)i_بائٹیں;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_پتہ, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
ساکن
bool guard_fault (EXCEPTION_اشارے* i_info, خالی* i_کم, size_t i_بائٹیں)
{
    اگر (i_info == تہی or i_info->Exception_ریکارڈ == تہی) {
        واپس false;
    }

    ثابت EXCEPTION_ریکارڈ* ریکارڈ = i_info->Exception_ریکارڈ;

    اگر (ریکارڈ->Exception_رمز != EXCEPTION_ACCESS_VIOLATION or
        ریکارڈ->نمبر_Parameters < 2) {
        واپس false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ثابت u8* پتہ = (ثابت u8*)ریکارڈ->ExceptionInformation[1];

    واپس پتہ >= (ثابت u8*)i_کم and پتہ < (ثابت u8*)i_کم + i_بائٹیں;
}

bool m3_Host_Guards_فعال (خالی)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    واپس true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (خالی (*i_body)(خالی*), خالی* i_context,
                           خالی* i_guard_کم, size_t i_guard_بائٹیں)
{
    __try {
        i_body(i_context);
        واپس true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_کم, i_guard_بائٹیں)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_تلاش) {
        واپس false;
    }
}

#اگر_ختم // d_m3GuardedMemory

#اگر_ختم // m3_host_win32_h
