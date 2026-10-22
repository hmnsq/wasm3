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

#अन्तर्भाव "m3_core.h"

#जइ d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

स्थिर अन्तःस्थ
u32 XxhRead32 (ध्रुव शून्य* i_ptr)
{
    u32 मानम्;
    memcpy(&मानम्, i_ptr, परिमाण(मानम्));
    M3_BSWAP_u32(मानम्);
    प्रत्यागम मानम्;
}

स्थिर अन्तःस्थ
u64 XxhRead64 (ध्रुव शून्य* i_ptr)
{
    u64 मानम्;
    memcpy(&मानम्, i_ptr, परिमाण(मानम्));
    M3_BSWAP_u64(मानम्);
    प्रत्यागम मानम्;
}

स्थिर अन्तःस्थ
u64 XxhRotl64 (u64 i_मानम्, u32 i_गणनम्)
{
    प्रत्यागम (i_मानम् << i_गणनम्) | (i_मानम् >> (64 - i_गणनम्));
}

// One lane taking one 8-byte word
स्थिर अन्तःस्थ
u64 XxhRound (u64 i_acc, u64 i_निवेशः)
{
    i_acc += i_निवेशः * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    प्रत्यागम i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
स्थिर अन्तःस्थ
u64 XxhMergeRound (u64 i_acc, u64 i_मानम्)
{
    i_acc ^= XxhRound(0, i_मानम्);
    प्रत्यागम i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

स्थिर अन्तःस्थ
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    प्रत्यागम i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
स्थिर
ध्रुव u8* XxhConsumeStripes (u64* io_acc, ध्रुव u8* i_निवेशः, size_t i_size)
{
    ध्रुव u8* निवेशः = i_निवेशः;
    ध्रुव u8* limit = i_निवेशः + i_size - (XXH_STRIPE_LEN - 1);

    करेहि {
        प्रति (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = XxhRound(io_acc[lane], XxhRead64(निवेशः));
            निवेशः += 8;
        }
    } जाव (निवेशः < limit);

    प्रत्यागम निवेशः;
}

स्थिर
u64 XxhMergeAccs (ध्रुव u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    प्रति (size_t lane = 0; lane < 4; ++lane) {
        hash = XxhMergeRound(hash, i_acc[lane]);
    }

    प्रत्यागम hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
स्थिर
u64 XxhFinalize (u64 i_hash, ध्रुव u8* i_निवेशः, size_t i_size)
{
    size_t size = i_size & (XXH_STRIPE_LEN - 1);

    जाव (size >= 8) {
        i_hash ^= XxhRound(0, XxhRead64(i_निवेशः));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_निवेशः += 8;
        size -= 8;
    }

    जइ (size >= 4) {
        i_hash ^= (u64)XxhRead32(i_निवेशः) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_निवेशः += 4;
        size -= 4;
    }

    जाव (size > 0) {
        i_hash ^= (*i_निवेशः++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --size;
    }

    प्रत्यागम XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

शून्य Xxh64_Init (Xxh64* o_state, u64 i_seed)
{
    o_state->acc[0] = i_seed + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_seed + XXH_PRIME64_2;
    o_state->acc[2] = i_seed;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_seed - XXH_PRIME64_1;

    o_state->totalLength  = 0;
    o_state->bufferLength = 0;
}

शून्य Xxh64_Update (Xxh64* io_state, ध्रुव शून्य* i_data, size_t i_size)
{
    जइ (not i_data) {
        प्रत्यागम;
    }

    ध्रुव u8* निवेशः = (ध्रुव u8*)i_data;
    ध्रुव u8* end   = निवेशः + i_size;

    io_state->totalLength += i_size;

    जइ (i_size < (size_t)(XXH_STRIPE_LEN - io_state->bufferLength)) {
        memcpy(io_state->buffer + io_state->bufferLength, निवेशः, i_size);
        io_state->bufferLength += (u32)i_size;
        प्रत्यागम;
    }

    // fill up what the buffer already holds, and consume that
    जइ (io_state->bufferLength) {
        size_t loadSize = XXH_STRIPE_LEN - io_state->bufferLength;

        memcpy(io_state->buffer + io_state->bufferLength, निवेशः, loadSize);
        निवेशः += loadSize;

        XxhConsumeStripes(io_state->acc, io_state->buffer, XXH_STRIPE_LEN);
        io_state->bufferLength = 0;
    }

    जइ ((size_t)(end - निवेशः) >= XXH_STRIPE_LEN) {
        निवेशः = XxhConsumeStripes(io_state->acc, निवेशः, (size_t)(end - निवेशः));
    }

    जइ (निवेशः < end) {
        memcpy(io_state->buffer, निवेशः, (size_t)(end - निवेशः));
        io_state->bufferLength = (u32)(end - निवेशः);
    }
}

u64 Xxh64_Digest (ध्रुव Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->totalLength >= XXH_STRIPE_LEN) ? XxhMergeAccs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->totalLength;

    प्रत्यागम XxhFinalize(hash, i_state->buffer, (size_t)i_state->totalLength);
}

#endif // d_m3HasSnapshots
