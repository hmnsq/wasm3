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

#जर d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

स्थिर ओळीत
u32 XxhRead32 (स्थिरांक शून्य* i_ptr)
{
    u32 मूल्य;
    स_नकल(&मूल्य, i_ptr, आकार(मूल्य));
    M3_BSWAP_u32(मूल्य);
    परत मूल्य;
}

स्थिर ओळीत
u64 XxhRead64 (स्थिरांक शून्य* i_ptr)
{
    u64 मूल्य;
    स_नकल(&मूल्य, i_ptr, आकार(मूल्य));
    M3_BSWAP_u64(मूल्य);
    परत मूल्य;
}

स्थिर ओळीत
u64 XxhRotl64 (u64 i_मूल्य, u32 i_मोजणी)
{
    परत (i_मूल्य << i_मोजणी) | (i_मूल्य >> (64 - i_मोजणी));
}

// One lane taking one 8-byte word
स्थिर ओळीत
u64 XxhRound (u64 i_acc, u64 i_आदान)
{
    i_acc += i_आदान * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    परत i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
स्थिर ओळीत
u64 XxhMergeRound (u64 i_acc, u64 i_मूल्य)
{
    i_acc ^= XxhRound(0, i_मूल्य);
    परत i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

स्थिर ओळीत
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    परत i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
स्थिर
स्थिरांक u8* XxhConsumeStripes (u64* io_acc, स्थिरांक u8* i_आदान, माप_प्रकार i_size)
{
    स्थिरांक u8* आदान = i_आदान;
    स्थिरांक u8* limit = i_आदान + i_size - (XXH_STRIPE_LEN - 1);

    करा {
        साठी (माप_प्रकार lane = 0; lane < 4; ++lane) {
            io_acc[lane] = XxhRound(io_acc[lane], XxhRead64(आदान));
            आदान += 8;
        }
    } पर्यंत (आदान < limit);

    परत आदान;
}

स्थिर
u64 XxhMergeAccs (स्थिरांक u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    साठी (माप_प्रकार lane = 0; lane < 4; ++lane) {
        hash = XxhMergeRound(hash, i_acc[lane]);
    }

    परत hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
स्थिर
u64 XxhFinalize (u64 i_hash, स्थिरांक u8* i_आदान, माप_प्रकार i_size)
{
    माप_प्रकार size = i_size & (XXH_STRIPE_LEN - 1);

    पर्यंत (size >= 8) {
        i_hash ^= XxhRound(0, XxhRead64(i_आदान));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_आदान += 8;
        size -= 8;
    }

    जर (size >= 4) {
        i_hash ^= (u64)XxhRead32(i_आदान) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_आदान += 4;
        size -= 4;
    }

    पर्यंत (size > 0) {
        i_hash ^= (*i_आदान++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --size;
    }

    परत XxhAvalanche(i_hash);
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

शून्य Xxh64_Update (Xxh64* io_state, स्थिरांक शून्य* i_data, माप_प्रकार i_size)
{
    जर (not i_data) {
        परत;
    }

    स्थिरांक u8* आदान = (स्थिरांक u8*)i_data;
    स्थिरांक u8* end   = आदान + i_size;

    io_state->totalLength += i_size;

    जर (i_size < (माप_प्रकार)(XXH_STRIPE_LEN - io_state->bufferLength)) {
        स_नकल(io_state->buffer + io_state->bufferLength, आदान, i_size);
        io_state->bufferLength += (u32)i_size;
        परत;
    }

    // fill up what the buffer already holds, and consume that
    जर (io_state->bufferLength) {
        माप_प्रकार loadSize = XXH_STRIPE_LEN - io_state->bufferLength;

        स_नकल(io_state->buffer + io_state->bufferLength, आदान, loadSize);
        आदान += loadSize;

        XxhConsumeStripes(io_state->acc, io_state->buffer, XXH_STRIPE_LEN);
        io_state->bufferLength = 0;
    }

    जर ((माप_प्रकार)(end - आदान) >= XXH_STRIPE_LEN) {
        आदान = XxhConsumeStripes(io_state->acc, आदान, (माप_प्रकार)(end - आदान));
    }

    जर (आदान < end) {
        स_नकल(io_state->buffer, आदान, (माप_प्रकार)(end - आदान));
        io_state->bufferLength = (u32)(end - आदान);
    }
}

u64 Xxh64_Digest (स्थिरांक Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->totalLength >= XXH_STRIPE_LEN) ? XxhMergeAccs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->totalLength;

    परत XxhFinalize(hash, i_state->buffer, (माप_प्रकार)i_state->totalLength);
}

#endif // d_m3HasSnapshots
