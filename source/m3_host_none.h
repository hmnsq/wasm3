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

#اگر_تعریف_نشده m3_host_none_h
#تعریف_کن m3_host_none_h

#ضمن "m3_core.h"
#ضمن "m3_host.h"

فراغ* m3_Host_پشته_پایه (فراغ)
{
    أرجع عدم;
}

فراغ m3_Host_Install_وقفه_Handler (IM3Runtime io_runtime)
{
    (فراغ)io_runtime;
}

فراغ m3_Host_Remove_وقفه_Handler (فراغ)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_Host_زمان_Ms (فراغ)
{
    أرجع 0;
}

فراغ m3_HostSleepNs (u64 i_ns)
{
    (فراغ)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (ثابت حرف* i_مسیر, size_t i_max_بایتها, M3HostFile* o_file)
{
    أرجع m3_HostReadFile(i_مسیر, i_max_بایتها, o_file);
}

فراغ m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->داده);

    io_file->اندازه = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_مقصد)
{
    remove(i_مقصد);
    أرجع rename(i_from, i_مقصد) == 0;
}

#إذا d_m3_Guarded_حافظه
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#پایان_اگر

#پایان_اگر // m3_host_none_h
