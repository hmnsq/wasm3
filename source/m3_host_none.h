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

#அந்தர்பாவ "m3_core.h"
#அந்தர்பாவ "m3_host.h"

வெறுமை* m3_Host_அடுக்கு_Base (வெறுமை)
{
    திரும்பு வெற்று;
}

வெறுமை m3_HostInstallInterruptHandler (IM3Runtime io_runtime)
{
    (வெறுமை)io_runtime;
}

வெறுமை m3_HostRemoveInterruptHandler (வெறுமை)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_HostTimeMs (வெறுமை)
{
    திரும்பு 0;
}

வெறுமை m3_HostSleepNs (u64 i_ns)
{
    (வெறுமை)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (மாறிலி எழுத்து* i_path, size_t i_maxBytes, M3HostFile* o_file)
{
    திரும்பு m3_HostReadFile(i_path, i_maxBytes, o_file);
}

வெறுமை m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->data);

    io_file->size = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (மாறிலி எழுத்து* i_from, மாறிலி எழுத்து* i_to)
{
    remove(i_to);
    திரும்பு rename(i_from, i_to) == 0;
}

#எனில் d_m3GuardedMemory
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#endif

#endif // m3_host_none_h
