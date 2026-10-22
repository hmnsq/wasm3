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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_host_none_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_host_none_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_host.h"

ਖਾਲੀ* m3_Host_ਢੇਰ_ਅਧਾਰ (ਖਾਲੀ)
{
    ਵਾਪਸ ਨਿਰਾ;
}

ਖਾਲੀ m3_Host_Install_ਰੁਕਾਵਟ_Handler (IM3Runtime io_runtime)
{
    (ਖਾਲੀ)io_runtime;
}

ਖਾਲੀ m3_Host_Remove_ਰੁਕਾਵਟ_Handler (ਖਾਲੀ)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_Host_ਸਮਾਂ_Ms (ਖਾਲੀ)
{
    ਵਾਪਸ 0;
}

ਖਾਲੀ m3_HostSleepNs (u64 i_ns)
{
    (ਖਾਲੀ)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (ਅਚਲ ਅੱਖਰ* i_ਰਸਤਾ, size_t i_max_ਬਾਈਟਾਂ, M3HostFile* o_file)
{
    ਵਾਪਸ m3_HostReadFile(i_ਰਸਤਾ, i_max_ਬਾਈਟਾਂ, o_file);
}

ਖਾਲੀ m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->ਅੰਕੜੇ);

    io_file->ਮਾਪ = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (ਅਚਲ ਅੱਖਰ* i_from, ਅਚਲ ਅੱਖਰ* i_ਮੰਜ਼ਿਲ)
{
    remove(i_ਮੰਜ਼ਿਲ);
    ਵਾਪਸ rename(i_from, i_ਮੰਜ਼ਿਲ) == 0;
}

#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#ਜੇ_ਅੰਤ

#ਜੇ_ਅੰਤ // m3_host_none_h
