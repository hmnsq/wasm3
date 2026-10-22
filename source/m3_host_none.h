//
//  m3_host_none.h
//
//  The implementation of m3_host.h for a system with neither a POSIX nor a Win32
//  face: a bare-metal target, an RTOS, or anything else. Most of the ports under
//  platforms/ land here.
//
//  There is no address space to reserve and no stack to measure, and "cannot say" is
//  a supported answer to both, so a build gets those for nothing. A file is the one
//  thing it still does, by reading it - which is the whole of what such a system can
//  do, and costs it stdio.
//
//  A header rather than a translation unit of its own, and included by m3_core.c
//  alone: a new .c would have to be added to every source list that names them one by
//  one, and most of those are in build scripts outside this repository.
//

#अगर_अघोषित m3_host_none_h
#घोषणा m3_host_none_h

#समावेश "m3_core.h"
#समावेश "m3_host.h"

व्योम* m3_Host_ढेर_Base (व्योम)
{
    वापस शून्य;
}

व्योम m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    (व्योम)io_runtime;
}

व्योम m3_HostRemoveInterruptHandler (व्योम)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_HostTimeMs (व्योम)
{
    वापस 0;
}

व्योम m3_HostSleepNs (u64 i_ns)
{
    (व्योम)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (स्थिर अक्षर* i_path, माप_प्रकार i_maxBytes, M3HostFile* o_file)
{
    वापस m3_HostReadFile(i_path, i_maxBytes, o_file);
}

व्योम m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->तथ्य);

    io_file->आकार = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (स्थिर अक्षर* i_from, स्थिर अक्षर* i_to)
{
    हटाओ(i_to);
    वापस नाम(i_from, i_to) == 0;
}

#अगर d_m3GuardedMemory
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#पूर्ण_अगर

#पूर्ण_अगर // m3_host_none_h
