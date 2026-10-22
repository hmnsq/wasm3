//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#నిర్వచితం_కాకపోతే m3_host_win32_h
#నిర్వచించు m3_host_win32_h

#అన్తర్భావ "m3_core.h"

#ఒకవేళ !defined(_WIN32)
#  error "This file is for Windows only"
#అయితే_ముగింపు

#అన్తర్భావ "m3_host.h"

#నిర్వచితం_కాకపోతే WIN32_LEAN_AND_సగటు
#  define WIN32_LEAN_AND_సగటు
#అయితే_ముగింపు
#అన్తర్భావ <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
శూన్య* m3_Host_దొంతర_పునాది (శూన్య)
{
    జ్ఞాపకం_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    అస్థిర అక్షరం            ఇక్కడ = 0;

    ఒకవేళ (VirtualQuery((LPCVOID)&ఇక్కడ, &mbi, పరిమాణం(mbi)) == 0) {
        తిరిగి ఖాళీ;
    }

    తిరిగి mbi.కేటాయింపు_పునాది;
}

స్థిర IM3Runtime s_win32SuspendRuntime = ఖాళీ;

స్థిర
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    ఒకవేళ (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        ఒకవేళ (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            తిరిగి TRUE;
        }
    }
    తిరిగి FALSE;
}

శూన్య m3_Host_Install_అంతరాయం_Handler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    సమితి_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, TRUE);
}

శూన్య m3_Host_Remove_అంతరాయం_Handler (శూన్య)
{
    సమితి_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = ఖాళీ;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_Host_సమయం_Ms (శూన్య)
{
    స్థిర స్థిరరాశి u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    Get_System_సమయం_As_File_సమయం(&now);

    u64 ticks = ((u64)now.dw_ఎగువ_Date_సమయం << 32) | now.dw_దిగువ_Date_సమయం;

    తిరిగి (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

శూన్య m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    అయితే (ms > 0) {
        DWORD అడుగు = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(అడుగు);
        ms -= అడుగు;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (స్థిరరాశి అక్షరం* i_మార్గం, size_t i_max_బైట్లు, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_మార్గం, GENERIC_READ, FILE_SHARE_READ, ఖాళీ, OPEN_EXISTING,
                              FILE_ATTRIBUTE_సాధారణ, ఖాళీ);
    ఒకవేళ (file == INVALID_HANDLE_విలువ) {
        తిరిగి false;
    }

    LARGE_INTEGER కొలత;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    ఒకవేళ (not Get_File_కొలత_Ex(file, &కొలత) or కొలత.QuadPart <= 0 or
        (ULONGLONG) కొలత.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_max_బైట్లు and (ULONGLONG) కొలత.QuadPart > (ULONGLONG)i_max_బైట్లు)) {
        CloseHandle(file);
        తిరిగి m3_HostReadFile(i_మార్గం, i_max_బైట్లు, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, ఖాళీ, పుట_READONLY, 0, 0, ఖాళీ);
    CloseHandle(file);                   // the mapping holds the file open

    ఒకవేళ (mapping == ఖాళీ) {
        తిరిగి m3_HostReadFile(i_మార్గం, i_max_బైట్లు, o_file);
    }

    శూన్య* పునాది = Map_View_యొక్క_File(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    ఒకవేళ (పునాది == ఖాళీ) {
        తిరిగి m3_HostReadFile(i_మార్గం, i_max_బైట్లు, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->దత్తాంశం = పునాది;
    o_file->కొలత = (size_t)కొలత.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    తిరిగి true;
}

శూన్య m3_HostUnmapFile (M3HostFile* io_file)
{
    ఒకవేళ (io_file->mapped) {
        Unmap_View_యొక్క_File(io_file->దత్తాంశం);  // the view knows how big it is
    } లేకపోతే {
        m3_Free(io_file->దత్తాంశం);
    }

    io_file->దత్తాంశం = ఖాళీ;
    io_file->కొలత = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (స్థిరరాశి అక్షరం* i_from, స్థిరరాశి అక్షరం* i_గమ్యం)
{
    తిరిగి MoveFileExA(i_from, i_గమ్యం, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#ఒకవేళ d_m3_Guarded_జ్ఞాపకం

size_t m3_Host_పుట_కొలత (శూన్య)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    తిరిగి info.dw_కేటాయింపు_Granularity ? info.dw_కేటాయింపు_Granularity : 65536;
}

శూన్య* m3_HostReserve (size_t i_బైట్లు)
{
    తిరిగి VirtualAlloc(ఖాళీ, i_బైట్లు, MEM_RESERVE, పుట_NOACCESS);
}

bool m3_HostCommit (శూన్య* i_చిరునామా, size_t i_బైట్లు)
{
    ఒకవేళ (i_బైట్లు == 0) {
        తిరిగి true;
    }
    తిరిగి VirtualAlloc(i_చిరునామా, i_బైట్లు, MEM_COMMIT, పుట_READWRITE) != ఖాళీ;
}

bool m3_HostDecommit (శూన్య* i_చిరునామా, size_t i_బైట్లు)
{
    ఒకవేళ (i_బైట్లు == 0) {
        తిరిగి true;
    }
    తిరిగి VirtualFree(i_చిరునామా, i_బైట్లు, MEM_DECOMMIT) != 0;
}

శూన్య m3_HostRelease (శూన్య* i_చిరునామా, size_t i_బైట్లు)
{
    (శూన్య)i_బైట్లు;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_చిరునామా, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
స్థిర
bool guard_fault (EXCEPTION_సూచికలు* i_info, శూన్య* i_దిగువ, size_t i_బైట్లు)
{
    ఒకవేళ (i_info == ఖాళీ or i_info->Exception_రికార్డు == ఖాళీ) {
        తిరిగి false;
    }

    స్థిరరాశి EXCEPTION_రికార్డు* రికార్డు = i_info->Exception_రికార్డు;

    ఒకవేళ (రికార్డు->Exception_సంకేతం != EXCEPTION_ACCESS_VIOLATION or
        రికార్డు->సంఖ్య_Parameters < 2) {
        తిరిగి false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    స్థిరరాశి u8* చిరునామా = (స్థిరరాశి u8*)రికార్డు->ExceptionInformation[1];

    తిరిగి చిరునామా >= (స్థిరరాశి u8*)i_దిగువ and చిరునామా < (స్థిరరాశి u8*)i_దిగువ + i_బైట్లు;
}

bool m3_Host_Guards_చురుకైన (శూన్య)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    తిరిగి true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (శూన్య (*i_body)(శూన్య*), శూన్య* i_context,
                           శూన్య* i_guard_దిగువ, size_t i_guard_బైట్లు)
{
    __try {
        i_body(i_context);
        తిరిగి true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_దిగువ, i_guard_బైట్లు)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_శోధన) {
        తిరిగి false;
    }
}

#అయితే_ముగింపు // d_m3GuardedMemory

#అయితే_ముగింపు // m3_host_win32_h
