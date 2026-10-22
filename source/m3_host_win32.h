//
//  m3_host_win32.h
//
//  The Win32 implementation of m3_host.h.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#यदि_अपरिभाषितम् m3_host_win32_h
#परिभाषय m3_host_win32_h

#अन्तर्भाव "m3_core.h"

#यदि !defined(_WIN32)
#  error "This file is for Windows only"
#यदि_अन्तः

#अन्तर्भाव "m3_host.h"

#यदि_अपरिभाषितम् WIN32_LEAN_AND_माध्यम्
#  define WIN32_LEAN_AND_माध्यम्
#यदि_अन्तः
#अन्तर्भाव <windows.h>

// A thread's stack is one reservation, and any address inside it answers with the
// base of the whole thing - guard page included. That is what makes VirtualQuery
// enough here: no version check, no other library, and it describes a fibre's stack
// as readily as a thread's. GetCurrentThreadStackLimits would say the same, and only
// from Windows 8 on.
शून्य* m3_Host_राशिः_आधारः (शून्य)
{
    स्मृतिः_BASIC_INFORMATION mbi;

    // any address on the stack being asked about will do, and this one is on it
    चञ्चल वर्ण            अत्र = 0;

    यदि (VirtualQuery((LPCVOID)&अत्र, &mbi, परिमाण(mbi)) == 0) {
        प्रत्यागम रिक्त;
    }

    प्रत्यागम mbi.आवण्टनम्_आधारः;
}

स्थिर IM3Runtime s_win32SuspendRuntime = रिक्त;

स्थिर
BOOL WINAPI m3_ConsoleCtrlHandler (DWORD dwCtrlType)
{
    यदि (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        यदि (s_win32SuspendRuntime && !s_win32SuspendRuntime->suspendRequested) {
            s_win32SuspendRuntime->suspendRequested = true;
            प्रत्यागम TRUE;
        }
    }
    प्रत्यागम FALSE;
}

शून्य m3_Host_Install_व्यवधानम्_Handler (IM3Runtime io_runtime)
{
    s_win32SuspendRuntime = io_runtime;
    समुच्चयः_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, TRUE);
}

शून्य m3_Host_Remove_व्यवधानम्_Handler (शून्य)
{
    समुच्चयः_Console_Ctrl_Handler(m3_ConsoleCtrlHandler, FALSE);
    s_win32SuspendRuntime = रिक्त;
}

// FILETIME counts 100ns intervals from 1601
u64 m3_HostTimeMs (शून्य)
{
    स्थिर ध्रुव u64 c_unixEpoch = 116444736000000000ULL;

    FILETIME         now;
    GetSystemTimeAsFileTime(&now);

    u64 ticks = ((u64)now.dw_उच्चः_Date_Time << 32) | now.dw_नीचः_Date_Time;

    प्रत्यागम (ticks > c_unixEpoch) ? (ticks - c_unixEpoch) / 10000 : 0;
}

शून्य m3_HostSleepNs (u64 i_ns)
{
    // whole milliseconds, rounded up; the wait is never shorter than asked
    u64 ms = (i_ns + 999999u) / 1000000u;

    यावत् (ms > 0) {
        DWORD चरणः = (ms > 0x7FFFFFFFu) ? 0x7FFFFFFFu : (DWORD)ms;
        Sleep(चरणः);
        ms -= चरणः;
    }
}

// FILE_SHARE_READ and nothing else: another process may read the file, and one
// trying to write or delete it is refused for as long as the mapping lives. The
// kernel enforces that, so it is the real thing rather than the advisory lock POSIX
// has to make do with - see m3_host.h.
bool m3_HostMapFile (ध्रुव वर्ण* i_मार्गः, size_t i_max_अष्टकानि, M3HostFile* o_file)
{
    HANDLE file = CreateFileA(i_मार्गः, GENERIC_READ, FILE_SHARE_READ, रिक्त, OPEN_EXISTING,
                              FILE_ATTRIBUTE_प्रसामान्यम्, रिक्त);
    यदि (file == INVALID_HANDLE_मानम्) {
        प्रत्यागम false;
    }

    LARGE_INTEGER आकार;

    // Anything with no size to map - a pipe, a console - is read instead, and so is
    // a file too large to address or larger than the caller wants
    यदि (not Get_File_आकार_Ex(file, &आकार) or आकार.QuadPart <= 0 or
        (ULONGLONG) आकार.QuadPart > (ULONGLONG)SIZE_MAX or
        (i_max_अष्टकानि and (ULONGLONG) आकार.QuadPart > (ULONGLONG)i_max_अष्टकानि)) {
        CloseHandle(file);
        प्रत्यागम m3_HostReadFile(i_मार्गः, i_max_अष्टकानि, o_file);
    }

    HANDLE mapping = CreateFileMappingA(file, रिक्त, PAGE_READONLY, 0, 0, रिक्त);
    CloseHandle(file);                   // the mapping holds the file open

    यदि (mapping == रिक्त) {
        प्रत्यागम m3_HostReadFile(i_मार्गः, i_max_अष्टकानि, o_file);
    }

    शून्य* आधारः = Map_View_सम्बन्धि_File(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);                // and the view holds the mapping

    यदि (आधारः == रिक्त) {
        प्रत्यागम m3_HostReadFile(i_मार्गः, i_max_अष्टकानि, o_file);
    }

    // Nothing left to keep: the view holds the mapping, which holds the file, and
    // the share mode with it
    o_file->दत्त = आधारः;
    o_file->आकार = (size_t)आकार.QuadPart;
    o_file->handle = d_m3HostNoHandle;
    o_file->mapped = true;
    प्रत्यागम true;
}

शून्य m3_HostUnmapFile (M3HostFile* io_file)
{
    यदि (io_file->mapped) {
        Unmap_View_सम्बन्धि_File(io_file->दत्त);  // the view knows how big it is
    } अन्यथा {
        m3_Free(io_file->दत्त);
    }

    io_file->दत्त = रिक्त;
    io_file->आकार = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// rename refuses a target that exists; MoveFileEx replaces it, and within a
// volume does so by renaming, in one step
bool m3_HostReplaceFile (ध्रुव वर्ण* i_from, ध्रुव वर्ण* i_गन्तव्यम्)
{
    प्रत्यागम MoveFileExA(i_from, i_गन्तव्यम्, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}


#यदि d_m3_Guarded_स्मृतिः

size_t m3_Host_Page_आकार (शून्य)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    // dwAllocationGranularity, not dwPageSize: a reservation starts on the coarser
    // of the two, so that is the grain the whole scheme has to be cut in
    प्रत्यागम info.dw_आवण्टनम्_Granularity ? info.dw_आवण्टनम्_Granularity : 65536;
}

शून्य* m3_HostReserve (size_t i_अष्टकानि)
{
    प्रत्यागम VirtualAlloc(रिक्त, i_अष्टकानि, MEM_RESERVE, PAGE_NOACCESS);
}

bool m3_HostCommit (शून्य* i_स्थानाङ्कः, size_t i_अष्टकानि)
{
    यदि (i_अष्टकानि == 0) {
        प्रत्यागम true;
    }
    प्रत्यागम VirtualAlloc(i_स्थानाङ्कः, i_अष्टकानि, MEM_COMMIT, PAGE_READWRITE) != रिक्त;
}

bool m3_HostDecommit (शून्य* i_स्थानाङ्कः, size_t i_अष्टकानि)
{
    यदि (i_अष्टकानि == 0) {
        प्रत्यागम true;
    }
    प्रत्यागम VirtualFree(i_स्थानाङ्कः, i_अष्टकानि, MEM_DECOMMIT) != 0;
}

शून्य m3_HostRelease (शून्य* i_स्थानाङ्कः, size_t i_अष्टकानि)
{
    (शून्य)i_अष्टकानि;                       // MEM_RELEASE frees the whole reservation
    VirtualFree(i_स्थानाङ्कः, 0, MEM_RELEASE);
}

// Which faults are ours, for the filter below. Read from the exception record
// rather than from anything of ours, so nothing here depends on what the faulting
// code was in the middle of.
स्थिर
bool guard_fault (EXCEPTION_सूचकाः* i_info, शून्य* i_नीचः, size_t i_अष्टकानि)
{
    यदि (i_info == रिक्त or i_info->Exception_लेखः == रिक्त) {
        प्रत्यागम false;
    }

    ध्रुव EXCEPTION_लेखः* लेखः = i_info->Exception_लेखः;

    यदि (लेखः->Exception_संहिता != EXCEPTION_ACCESS_VIOLATION or
        लेखः->सङ्ख्या_Parameters < 2) {
        प्रत्यागम false;
    }

    // [0] says read or write, [1] is the address that could not be reached
    ध्रुव u8* स्थानाङ्कः = (ध्रुव u8*)लेखः->ExceptionInformation[1];

    प्रत्यागम स्थानाङ्कः >= (ध्रुव u8*)i_नीचः and स्थानाङ्कः < (ध्रुव u8*)i_नीचः + i_अष्टकानि;
}

bool m3_Host_Guards_सक्रिय (शून्य)
{
    // __try/__except is part of the frame m3_HostProtectedCall compiles to, so there is
    // nothing to install and nothing another part of the process can take away
    प्रत्यागम true;
}

// Structured exception handling rather than a vectored handler: the unwind is the
// system's to do, so the stack between here and the fault is given back properly
// instead of being jumped over. It is also why this needs a compiler that speaks
// __try - see d_m3GuardedMemory.
bool m3_HostProtectedCall (शून्य (*i_body)(शून्य*), शून्य* i_context,
                           शून्य* i_guard_नीचः, size_t i_guard_अष्टकानि)
{
    __try {
        i_body(i_context);
        प्रत्यागम true;
    } __except (guard_fault(GetExceptionInformation(), i_guard_नीचः, i_guard_अष्टकानि)
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_अन्वेषणम्) {
        प्रत्यागम false;
    }
}

#यदि_अन्तः // d_m3GuardedMemory

#यदि_अन्तः // m3_host_win32_h
