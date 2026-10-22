//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#अगर_अघोषित m3_host_win32_h
#घोषणा m3_host_win32_h

#समावेश "m3_core.h"

#अगर !defined(_WIN32)
#  error "This file is for Windows only"
#पूर्ण_अगर

#समावेश "m3_host.h"

#अगर_अघोषित WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#पूर्ण_अगर
#समावेश <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
व्योम* m3_Host_ढेर_Base (व्योम)
{
    MEMORY_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    अस्थिर अक्षर            here = 0;

    अगर (VirtualQuery((LPCVOID)&here, &mbi, माप(mbi)) == 0) {
        वापस शून्य;
    }

    वापस mbi.AllocationBase;
}

अटल IM3Runtime s_win32SuspendRuntime = शून्य;

अटल
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    अगर (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        अगर (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            वापस TRUE;
        }
    }
    वापस FALSE;
}

व्योम m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, TRUE);
}

व्योम m3_HostRemoveInterruptHandler (व्योम)
{
    SetConsoleCtrlHandler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = शून्य;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (व्योम)
{
    अटल स्थिर u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_ऊपरी_Date_Time << 32) | now.dw_निचला_Date_Time;

    वापस (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

व्योम m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    जबतक (ms > 0) {
        DWORD step = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(step);
        ms -= step;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (स्थिर अक्षर* i_path, माप_प्रकार i_maxBytes, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_path, GENERIC_READ, FILE_SHARE_READ, शून्य, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, शून्य);
    अगर (file == INVALID_HANDLE_मूल्य) {
        वापस false;
    }

    LARGE_INTEGER आकार;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    अगर (not Get_File_आकार_Ex(file, &आकार) or आकार.QuadPart <= 0 or
        (ULONGLONG) आकार.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_maxBytes and (ULONGLONG) आकार.QuadPart > (ULONGLONG)i_maxBytes)) {
        CloseHandle(file);
        वापस m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, शून्य, PAGE_READONLY, 0, 0, शून्य);
    CloseHandle(file);                   // the mapping holds the file open

    अगर (mapping == शून्य) {
        वापस m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    व्योम* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    अगर (base == शून्य) {
        वापस m3_HostReadFile(i_path, i_maxBytes, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->तथ्य = base;
    o_file->आकार = (माप_प्रकार)आकार.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    वापस true;
}

व्योम m3_HostUnmapFile (M3HostFile* io_file)
{
    अगर (io_file->mapped) {
        UnmapViewOfFile(io_file->तथ्य);  // the view knows how big it is
    } अन्यथा {
        m3_Free(io_file->तथ्य);
    }

    io_file->तथ्य = शून्य;
    io_file->आकार = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (स्थिर अक्षर* i_from, स्थिर अक्षर* i_to)
{
    वापस MoveFileExA(i_from, i_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#अगर d_m3GuardedMemory

माप_प्रकार m3_Host_Page_आकार (व्योम)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    वापस info.dwAllocationGranularity ? info.dwAllocationGranularity : 65536;
}

व्योम* m3_HostReserve (माप_प्रकार i_bytes)
{
    वापस VirtualAlloc(शून्य, i_bytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (व्योम* i_address, माप_प्रकार i_bytes)
{
    अगर (i_bytes == 0) {
        वापस true;
    }
    वापस VirtualAlloc(i_address, i_bytes, MEM_COMMIT, PAGE_READWRITE) != शून्य;
}

bool m3_HostDecommit (व्योम* i_address, माप_प्रकार i_bytes)
{
    अगर (i_bytes == 0) {
        वापस true;
    }
    वापस VirtualFree(i_address, i_bytes, MEM_DECOMMIT) != 0;
}

व्योम m3_HostRelease (व्योम* i_address, माप_प्रकार i_bytes)
{
    (व्योम)i_bytes;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_address, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
अटल
bool guard_fault (EXCEPTION_POINTERS* i_info, व्योम* i_निचला, माप_प्रकार i_bytes)
{
    अगर (i_info == शून्य or i_info->ExceptionRecord == शून्य) {
        वापस false;
    }

    स्थिर EXCEPTION_RECORD* record = i_info->ExceptionRecord;

    अगर (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION or
        record->संख्या_Parameters < 2) {
        वापस false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    स्थिर u8* address = (स्थिर u8*)record->ExceptionInformation[1];

    वापस address >= (स्थिर u8*)i_निचला and address < (स्थिर u8*)i_निचला + i_bytes;
}

bool m3_HostGuardsActive (व्योम)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    वापस true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (व्योम (*i_body)(व्योम*), व्योम* i_context,
                           व्योम* i_guard_निचला, माप_प्रकार i_guardBytes)
{
    __try {
        i_body(i_context);
        वापस true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_निचला, i_guardBytes)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_खोज) {
        वापस false;
    }
}

#पूर्ण_अगर // d_m3GuardedMemory

#पूर्ण_अगर // m3_host_win32_h
