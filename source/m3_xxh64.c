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

#અન્તર્ભાવ "m3_core.h"

#જો d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

સ્થિર અંતઃરેખ
u32 XxhRead32 (અચલ શૂન્ય* i_ptr)
{
    u32 કિંમત;
    memcpy(&કિંમત, i_ptr, કદ(કિંમત));
    M3_BSWAP_u32(કિંમત);
    પાછા કિંમત;
}

સ્થિર અંતઃરેખ
u64 XxhRead64 (અચલ શૂન્ય* i_ptr)
{
    u64 કિંમત;
    memcpy(&કિંમત, i_ptr, કદ(કિંમત));
    M3_BSWAP_u64(કિંમત);
    પાછા કિંમત;
}

સ્થિર અંતઃરેખ
u64 XxhRotl64 (u64 i_કિંમત, u32 i_ગણતરી)
{
    પાછા (i_કિંમત << i_ગણતરી) | (i_કિંમત >> (64 - i_ગણતરી));
}

// One lane taking one 8-byte word
સ્થિર અંતઃરેખ
u64 XxhRound (u64 i_acc, u64 i_આગત)
{
    i_acc += i_આગત * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    પાછા i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
સ્થિર અંતઃરેખ
u64 XxhMergeRound (u64 i_acc, u64 i_કિંમત)
{
    i_acc ^= XxhRound(0, i_કિંમત);
    પાછા i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

સ્થિર અંતઃરેખ
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    પાછા i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
સ્થિર
અચલ u8* XxhConsumeStripes (u64* io_acc, અચલ u8* i_આગત, size_t i_size)
{
    અચલ u8* આગત = i_આગત;
    અચલ u8* limit = i_આગત + i_size - (XXH_STRIPE_LEN - 1);

    કરો {
        માટે (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = XxhRound(io_acc[lane], XxhRead64(આગત));
            આગત += 8;
        }
    } જ્યારે (આગત < limit);

    પાછા આગત;
}

સ્થિર
u64 XxhMergeAccs (અચલ u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    માટે (size_t lane = 0; lane < 4; ++lane) {
        hash = XxhMergeRound(hash, i_acc[lane]);
    }

    પાછા hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
સ્થિર
u64 XxhFinalize (u64 i_hash, અચલ u8* i_આગત, size_t i_size)
{
    size_t size = i_size & (XXH_STRIPE_LEN - 1);

    જ્યારે (size >= 8) {
        i_hash ^= XxhRound(0, XxhRead64(i_આગત));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_આગત += 8;
        size -= 8;
    }

    જો (size >= 4) {
        i_hash ^= (u64)XxhRead32(i_આગત) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_આગત += 4;
        size -= 4;
    }

    જ્યારે (size > 0) {
        i_hash ^= (*i_આગત++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --size;
    }

    પાછા XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

શૂન્ય Xxh64_Init (Xxh64* o_state, u64 i_seed)
{
    o_state->acc[0] = i_seed + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_seed + XXH_PRIME64_2;
    o_state->acc[2] = i_seed;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_seed - XXH_PRIME64_1;

    o_state->totalLength  = 0;
    o_state->bufferLength = 0;
}

શૂન્ય Xxh64_Update (Xxh64* io_state, અચલ શૂન્ય* i_data, size_t i_size)
{
    જો (not i_data) {
        પાછા;
    }

    અચલ u8* આગત = (અચલ u8*)i_data;
    અચલ u8* end   = આગત + i_size;

    io_state->totalLength += i_size;

    જો (i_size < (size_t)(XXH_STRIPE_LEN - io_state->bufferLength)) {
        memcpy(io_state->buffer + io_state->bufferLength, આગત, i_size);
        io_state->bufferLength += (u32)i_size;
        પાછા;
    }

    // fill up what the buffer already holds, and consume that
    જો (io_state->bufferLength) {
        size_t loadSize = XXH_STRIPE_LEN - io_state->bufferLength;

        memcpy(io_state->buffer + io_state->bufferLength, આગત, loadSize);
        આગત += loadSize;

        XxhConsumeStripes(io_state->acc, io_state->buffer, XXH_STRIPE_LEN);
        io_state->bufferLength = 0;
    }

    જો ((size_t)(end - આગત) >= XXH_STRIPE_LEN) {
        આગત = XxhConsumeStripes(io_state->acc, આગત, (size_t)(end - આગત));
    }

    જો (આગત < end) {
        memcpy(io_state->buffer, આગત, (size_t)(end - આગત));
        io_state->bufferLength = (u32)(end - આગત);
    }
}

u64 Xxh64_Digest (અચલ Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->totalLength >= XXH_STRIPE_LEN) ? XxhMergeAccs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->totalLength;

    પાછા XxhFinalize(hash, i_state->buffer, (size_t)i_state->totalLength);
}

#endif // d_m3HasSnapshots
