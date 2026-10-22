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

#اگر_غیر_تعریف_شدہ m3_host_none_h
#تعریف_کرو m3_host_none_h

#شامل "m3_core.h"
#شامل "m3_host.h"

خالی* m3_Host_ڈھیر_بنیاد (خالی)
{
    واپس تہی;
}

خالی m3_Host_Install_مداخلت_Handler (IM3Runtime io_runtime)
{
    (خالی)io_runtime;
}

خالی m3_Host_Remove_مداخلت_Handler (خالی)
{
}

// Plenty of these systems have no idea what time it is
u64 m3_Host_وقت_Ms (خالی)
{
    واپس 0;
}

خالی m3_HostSleepNs (u64 i_ns)
{
    (خالی)i_ns;
}

// No mmap here, so the module's bytes are read directly onto the heap
bool m3_HostMapFile (ثابت حرف* i_راستہ, size_t i_max_بائٹیں, M3HostFile* o_file)
{
    واپس m3_HostReadFile(i_راستہ, i_max_بائٹیں, o_file);
}

خالی m3_HostUnmapFile (M3HostFile* io_file)
{
    m3_Free(io_file->ڈیٹا);

    io_file->سائز = 0;
    io_file->handle = d_m3HostNoHandle;
    io_file->mapped = false;
}

// Standard C leaves it to the system whether rename replaces a file that
// exists, so the target goes first. Two steps: in between, there is no i_to.
bool m3_HostReplaceFile (ثابت حرف* i_from, ثابت حرف* i_مقصد)
{
    remove(i_مقصد);
    واپس rename(i_from, i_مقصد) == 0;
}

#اگر d_m3_Guarded_حافظہ
#  error "d_m3GuardedMemory needs a system that can reserve address space; this one has none"
#اگر_ختم

#اگر_ختم // m3_host_none_h
