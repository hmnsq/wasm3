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

#ifndef m3_host_none_h
#define m3_host_none_h

#अन्तर्भाव "m3_core.h"
#अन्तर्भाव "m3_host.h"

शून्य* m3_Host_ढेर_Base (शून्य)
{
    प्रत्यागम रिक्त;
}

शून्य m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    (शून्य)io_runtime;
}

शून्य m3_HostRemoveInterruptHandler (शून्य)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_HostTimeMs (शून्य)
{
    प्रत्यागम 0;
}

शून्य m3_HostSleepNs (u64 i_ns)
{
    (शून्य)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (ध्रुव वर्ण* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    प्रत्यागम m3_HostReadFile(i_path, i_maxBytes, o_file);
}

शून्य m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->data);

    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (ध्रुव वर्ण* i_from, ध्रुव वर्ण* i_to)
{
    remove(i_to);
    प्रत्यागम rename(i_from, i_to) == 0;
}

#यदि d_m3GuardedMemory
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#endif

#endif // m3_host_none_h
