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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_host_none_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_host_none_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"
#ಅನ್ತರ್ಭಾವ "m3_host.h"

ಶೂನ್ಯ* m3_Host_ರಾಶಿ_ತಳ (ಶೂನ್ಯ)
{
    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

ಶೂನ್ಯ m3_Host_Install_ಅಡಚಣೆ_Handler (IM3Runtime io_runtime)
{
    (ಶೂನ್ಯ)io_runtime;
}

ಶೂನ್ಯ m3_Host_Remove_ಅಡಚಣೆ_Handler (ಶೂನ್ಯ)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_Host_ಸಮಯ_Ms (ಶೂನ್ಯ)
{
    ಹಿಂತಿರುಗಿ 0;
}

ಶೂನ್ಯ m3_HostSleepNs (u64 i_ns)
{
    (ಶೂನ್ಯ)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_ಮಾರ್ಗ, size_t i_max_ಬೈಟುಗಳು, M3HostFile* o_file)
{
    ಹಿಂತಿರುಗಿ m3_HostReadFile(i_ಮಾರ್ಗ, i_max_ಬೈಟುಗಳು, o_file);
}

ಶೂನ್ಯ m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->ದತ್ತಾಂಶ);

    io_file->ಗಾತ್ರ_2 = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_from, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_ಗಮ್ಯ)
{
    remove(i_ಗಮ್ಯ);
    ಹಿಂತಿರುಗಿ rename(i_from, i_ಗಮ್ಯ) == 0;
}

#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ_ಅಂತ್ಯ // m3_host_none_h
