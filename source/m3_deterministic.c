//
//  m3_deterministic.c
//
//  Copyright © 2026 Volodymyr Shymanskyy. All rights reserved.
//

#समावेश "m3_deterministic.h"

#अगर d_m3DeterministicProfile

व्योम Deterministic_InitRuntime (IM3Runtime io_runtime)
{
    io_runtime->virtualTimeNs = 0;
    io_runtime->randomState   = d_m3DeterministicSeed;
}

u64 Deterministic_Time (IM3Runtime io_runtime)
{
    u64 now = io_runtime->virtualTimeNs;

    Deterministic_Advance(io_runtime, d_m3DeterministicTickNs);

    वापस now;
}

व्योम Deterministic_Advance (IM3Runtime io_runtime, u64 i_nanoseconds)
{
    // Saturating: a guest may ask to wait longer than the clock can express, and
    // going backwards is the one thing this clock must never do
    अगर (io_runtime->virtualTimeNs > UINT64_MAX - i_nanoseconds) {
        io_runtime->virtualTimeNs = UINT64_MAX;
    } अन्यथा {
        io_runtime->virtualTimeNs += i_nanoseconds;
    }
}

// SplitMix64, which is a few lines and passes the usual test suites. Nothing here
// wants unpredictability - the point is the opposite - so the only property that
// matters is that the same seed gives the same bytes on every machine, which fixed
// width arithmetic and no floating point are enough for.
व्योम Deterministic_Random (IM3Runtime io_runtime, व्योम* o_buf, माप_प्रकार i_लंबाई)
{
    u8* out = (u8*)o_buf;

    जबतक (i_लंबाई) {
        io_runtime->randomState += UINT64_C(0x9E3779B97F4A7C15);

        u64 z = io_runtime->randomState;
        z     = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z     = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        z     = z ^ (z >> 31);

        माप_प्रकार chunk = M3_MIN(i_लंबाई, माप(z));

        // one byte at a time, so the stream does not depend on the host's byte order
        क्रम (माप_प्रकार i = 0; i < chunk; ++i) {
            out[i] = (u8)(z >> (8 * i));
        }

        out += chunk;
        i_लंबाई -= chunk;
    }
}

#पूर्ण_अगर // d_m3DeterministicProfile
