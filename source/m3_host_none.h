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

#నిర్వచితం_కాకపోతే m3_host_none_h
#నిర్వచించు m3_host_none_h

#అన్తర్భావ "m3_core.h"
#అన్తర్భావ "m3_host.h"

శూన్య* m3_Host_దొంతర_పునాది (శూన్య)
{
    తిరిగి ఖాళీ;
}

శూన్య m3_Host_Install_అంతరాయం_Handler (IM3Runtime io_runtime)
{
    (శూన్య)io_runtime;
}

శూన్య m3_Host_Remove_అంతరాయం_Handler (శూన్య)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_Host_సమయం_Ms (శూన్య)
{
    తిరిగి 0;
}

శూన్య m3_HostSleepNs (u64 i_ns)
{
    (శూన్య)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (స్థిరరాశి అక్షరం* i_మార్గం, size_t i_max_బైట్లు, M3HostFile* o_file)
{
    తిరిగి m3_HostReadFile(i_మార్గం, i_max_బైట్లు, o_file);
}

శూన్య m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->దత్తాంశం);

    io_file->కొలత = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (స్థిరరాశి అక్షరం* i_from, స్థిరరాశి అక్షరం* i_గమ్యం)
{
    remove(i_గమ్యం);
    తిరిగి rename(i_from, i_గమ్యం) == 0;
}

#ఒకవేళ d_m3_Guarded_జ్ఞాపకం
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#అయితే_ముగింపు

#అయితే_ముగింపు // m3_host_none_h
