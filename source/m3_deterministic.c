//
//  m3_deterministic.c
//
//  Copyright © 2026 Volodymyr Shymanskyy. All rights reserved.
//

#شامل "m3_deterministic.h"

#اگر d_m3DeterministicProfile

خالی Deterministic_InitRuntime (IM3Runtime io_runtime)
{
    io_runtime->virtual_وقت_Ns = 0;
    io_runtime->randomState   = d_m3_Deterministic_بیج;
}

u64 Deterministic_وقت (IM3Runtime io_runtime)
{
    u64 now = io_runtime->virtual_وقت_Ns;

    Deterministic_Advance(io_runtime, d_m3DeterministicTickNs);

    واپس now;
}

خالی Deterministic_Advance (IM3Runtime io_runtime, u64 i_nanoseconds)
{
    // Saturating: a guest may ask to wait longer than the clock can express, and
    // going backwards is the one thing this clock must never do
    اگر (io_runtime->virtual_وقت_Ns > UINT64_MAX - i_nanoseconds) {
        io_runtime->virtual_وقت_Ns = UINT64_MAX;
    } ورنہ {
        io_runtime->virtual_وقت_Ns += i_nanoseconds;
    }
}

// SplitMix64, which is a few lines and passes the usual test suites. Nothing here
// wants unpredictability - the point is the opposite - so the only property that
// matters is that the same seed gives the same bytes on every machine, which fixed
// width arithmetic and no floating point are enough for.
خالی Deterministic_Random (IM3Runtime io_runtime, خالی* o_buf, size_t i_لمبائی)
{
    u8* برآمد = (u8*)o_buf;

    جب (i_لمبائی) {
        io_runtime->randomState += UINT64_C(0x9E3779B97F4A7C15);

        u64 z = io_runtime->randomState;
        z     = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z     = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        z     = z ^ (z >> 31);

        size_t chunk = M3_MIN(i_لمبائی, حجم(z));

        // one byte at a time, so the stream does not depend on the host's byte order
        برائے (size_t i = 0; i < chunk; ++i) {
            برآمد[i] = (u8)(z >> (8 * i));
        }

        برآمد += chunk;
        i_لمبائی -= chunk;
    }
}

#اگر_ختم // d_m3DeterministicProfile
