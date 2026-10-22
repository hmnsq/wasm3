//
//  m3_xxh64.c
//
//  XXH64. A port of the reference implementation of xxHash 0.8.3 (BSD-2-Clause)
//  https://github.com/Cyan4973/xxHash
//
//  Digests match the reference on every host. The algorithm is defined over
//  little-endian reads, which is what the accessor below performs whatever the
//  host's byte order, and everything else is 64-bit arithmetic.
//

#అన్తర్భావ "m3_core.h"

#ఒకవేళ d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

స్థిర అంతఃవరుస
u32 XxhRead32 (స్థిరరాశి శూన్య* i_ptr)
{
    u32 విలువ;
    జ్ఞాపకం_నకలు(&విలువ, i_ptr, పరిమాణం(విలువ));
    M3_BSWAP_u32(విలువ);
    తిరిగి విలువ;
}

స్థిర అంతఃవరుస
u64 XxhRead64 (స్థిరరాశి శూన్య* i_ptr)
{
    u64 విలువ;
    జ్ఞాపకం_నకలు(&విలువ, i_ptr, పరిమాణం(విలువ));
    M3_BSWAP_u64(విలువ);
    తిరిగి విలువ;
}

స్థిర అంతఃవరుస
u64 XxhRotl64 (u64 i_విలువ, u32 i_లెక్క)
{
    తిరిగి (i_విలువ << i_లెక్క) | (i_విలువ >> (64 - i_లెక్క));
}

// One lane taking one 8-byte word
స్థిర అంతఃవరుస
u64 Xxh_విడత (u64 i_acc, u64 i_ఆదానం)
{
    i_acc += i_ఆదానం * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    తిరిగి i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
స్థిర అంతఃవరుస
u64 Xxh_విలీనం_విడత (u64 i_acc, u64 i_విలువ)
{
    i_acc ^= Xxh_విడత(0, i_విలువ);
    తిరిగి i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

స్థిర అంతఃవరుస
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    తిరిగి i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
స్థిర
స్థిరరాశి u8* XxhConsumeStripes (u64* io_acc, స్థిరరాశి u8* i_ఆదానం, size_t i_కొలత)
{
    స్థిరరాశి u8* ఆదానం = i_ఆదానం;
    స్థిరరాశి u8* హద్దు = i_ఆదానం + i_కొలత - (XXH_STRIPE_LEN - 1);

    చేయి {
        కొరకు (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = Xxh_విడత(io_acc[lane], XxhRead64(ఆదానం));
            ఆదానం += 8;
        }
    } అయితే (ఆదానం < హద్దు);

    తిరిగి ఆదానం;
}

స్థిర
u64 Xxh_విలీనం_Accs (స్థిరరాశి u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    కొరకు (size_t lane = 0; lane < 4; ++lane) {
        hash = Xxh_విలీనం_విడత(hash, i_acc[lane]);
    }

    తిరిగి hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
స్థిర
u64 XxhFinalize (u64 i_hash, స్థిరరాశి u8* i_ఆదానం, size_t i_కొలత)
{
    size_t కొలత = i_కొలత & (XXH_STRIPE_LEN - 1);

    అయితే (కొలత >= 8) {
        i_hash ^= Xxh_విడత(0, XxhRead64(i_ఆదానం));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_ఆదానం += 8;
        కొలత -= 8;
    }

    ఒకవేళ (కొలత >= 4) {
        i_hash ^= (u64)XxhRead32(i_ఆదానం) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_ఆదానం += 4;
        కొలత -= 4;
    }

    అయితే (కొలత > 0) {
        i_hash ^= (*i_ఆదానం++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --కొలత;
    }

    తిరిగి XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

శూన్య Xxh64_Init (Xxh64* o_state, u64 i_విత్తనం)
{
    o_state->acc[0] = i_విత్తనం + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_విత్తనం + XXH_PRIME64_2;
    o_state->acc[2] = i_విత్తనం;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_విత్తనం - XXH_PRIME64_1;

    o_state->మొత్తం_నిడివి  = 0;
    o_state->బఫరు_నిడివి = 0;
}

శూన్య Xxh64_Update (Xxh64* io_state, స్థిరరాశి శూన్య* i_దత్తాంశం, size_t i_కొలత)
{
    ఒకవేళ (not i_దత్తాంశం) {
        తిరిగి;
    }

    స్థిరరాశి u8* ఆదానం = (స్థిరరాశి u8*)i_దత్తాంశం;
    స్థిరరాశి u8* చివర   = ఆదానం + i_కొలత;

    io_state->మొత్తం_నిడివి += i_కొలత;

    ఒకవేళ (i_కొలత < (size_t)(XXH_STRIPE_LEN - io_state->బఫరు_నిడివి)) {
        జ్ఞాపకం_నకలు(io_state->బఫరు + io_state->బఫరు_నిడివి, ఆదానం, i_కొలత);
        io_state->బఫరు_నిడివి += (u32)i_కొలత;
        తిరిగి;
    }

    // fill up what the buffer already holds, and consume that
    ఒకవేళ (io_state->బఫరు_నిడివి) {
        size_t ఎక్కించు_కొలత = XXH_STRIPE_LEN - io_state->బఫరు_నిడివి;

        జ్ఞాపకం_నకలు(io_state->బఫరు + io_state->బఫరు_నిడివి, ఆదానం, ఎక్కించు_కొలత);
        ఆదానం += ఎక్కించు_కొలత;

        XxhConsumeStripes(io_state->acc, io_state->బఫరు, XXH_STRIPE_LEN);
        io_state->బఫరు_నిడివి = 0;
    }

    ఒకవేళ ((size_t)(చివర - ఆదానం) >= XXH_STRIPE_LEN) {
        ఆదానం = XxhConsumeStripes(io_state->acc, ఆదానం, (size_t)(చివర - ఆదానం));
    }

    ఒకవేళ (ఆదానం < చివర) {
        జ్ఞాపకం_నకలు(io_state->బఫరు, ఆదానం, (size_t)(చివర - ఆదానం));
        io_state->బఫరు_నిడివి = (u32)(చివర - ఆదానం);
    }
}

u64 Xxh64_Digest (స్థిరరాశి Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->మొత్తం_నిడివి >= XXH_STRIPE_LEN) ? Xxh_విలీనం_Accs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->మొత్తం_నిడివి;

    తిరిగి XxhFinalize(hash, i_state->బఫరు, (size_t)i_state->మొత్తం_నిడివి);
}

#అయితే_ముగింపు // d_m3HasSnapshots
