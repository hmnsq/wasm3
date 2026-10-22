//
//  m3_deterministic.c
//
//  Copyright © 2026 Volodymyr Shymanskyy. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_deterministic.h"

#ಆದರೆ d_m3DeterministicProfile

ಶೂನ್ಯ Deterministic_InitRuntime (IM3Runtime io_runtime)
{
    io_runtime->virtual_ಸಮಯ_Ns = 0;
    io_runtime->randomState   = d_m3_Deterministic_ಬೀಜ;
}

u64 Deterministic_ಸಮಯ (IM3Runtime io_runtime)
{
    u64 now = io_runtime->virtual_ಸಮಯ_Ns;

    Deterministic_Advance(io_runtime, d_m3DeterministicTickNs);

    ಹಿಂತಿರುಗಿ now;
}

ಶೂನ್ಯ Deterministic_Advance (IM3Runtime io_runtime, u64 i_nanoseconds)
{
    // Saturating: a guest may ask to wait longer than the clock can express, and
    // going backwards is the one thing this clock must never do
    ಆದರೆ (io_runtime->virtual_ಸಮಯ_Ns > UINT64_MAX - i_nanoseconds) {
        io_runtime->virtual_ಸಮಯ_Ns = UINT64_MAX;
    } ಇಲ್ಲದಿದ್ದರೆ {
        io_runtime->virtual_ಸಮಯ_Ns += i_nanoseconds;
    }
}

// SplitMix64, which is a few lines and passes the usual test suites. Nothing here
// wants unpredictability - the point is the opposite - so the only property that
// matters is that the same seed gives the same bytes on every machine, which fixed
// width arithmetic and no floating point are enough for.
ಶೂನ್ಯ Deterministic_Random (IM3Runtime io_runtime, ಶೂನ್ಯ* o_buf, size_t i_ಉದ್ದಳತೆ)
{
    u8* ಹೊರಹೋಗು = (u8*)o_buf;

    ಆಗ (i_ಉದ್ದಳತೆ) {
        io_runtime->randomState += UINT64_C(0x9E3779B97F4A7C15);

        u64 z = io_runtime->randomState;
        z     = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z     = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        z     = z ^ (z >> 31);

        size_t chunk = M3_MIN(i_ಉದ್ದಳತೆ, ಗಾತ್ರ(z));

        // one byte at a time, so the stream does not depend on the host's byte order
        ಗಾಗಿ (size_t i = 0; i < chunk; ++i) {
            ಹೊರಹೋಗು[i] = (u8)(z >> (8 * i));
        }

        ಹೊರಹೋಗು += chunk;
        i_ಉದ್ದಳತೆ -= chunk;
    }
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3DeterministicProfile
