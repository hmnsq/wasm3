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

#यदि d_m3HasSnapshots


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
u32 XxhRead32 (अचर शून्य* i_ptr)
{
    u32 मान;
    स_नकल(&मान, i_ptr, परिमाण(मान));
    M3_BSWAP_u32(मान);
    फर्काऊ मान;
}

स्थिर अन्तःस्थ
u64 XxhRead64 (अचर शून्य* i_ptr)
{
    u64 मान;
    स_नकल(&मान, i_ptr, परिमाण(मान));
    M3_BSWAP_u64(मान);
    फर्काऊ मान;
}

स्थिर अन्तःस्थ
u64 XxhRotl64 (u64 i_मान, u32 i_गन्ती)
{
    फर्काऊ (i_मान << i_गन्ती) | (i_मान >> (64 - i_गन्ती));
}

// One lane taking one 8-byte word
स्थिर अन्तःस्थ
u64 XxhRound (u64 i_acc, u64 i_आगत)
{
    i_acc += i_आगत * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    फर्काऊ i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
स्थिर अन्तःस्थ
u64 XxhMergeRound (u64 i_acc, u64 i_मान)
{
    i_acc ^= XxhRound(0, i_मान);
    फर्काऊ i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

स्थिर अन्तःस्थ
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    फर्काऊ i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
स्थिर
अचर u8* XxhConsumeStripes (u64* io_acc, अचर u8* i_आगत, माप_प्रकार i_size)
{
    अचर u8* आगत = i_आगत;
    अचर u8* limit = i_आगत + i_size - (XXH_STRIPE_LEN - 1);

    गर {
        लागि (माप_प्रकार lane = 0; lane < 4; ++lane) {
            io_acc[lane] = XxhRound(io_acc[lane], XxhRead64(आगत));
            आगत += 8;
        }
    } जबसम्म (आगत < limit);

    फर्काऊ आगत;
}

स्थिर
u64 XxhMergeAccs (अचर u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    लागि (माप_प्रकार lane = 0; lane < 4; ++lane) {
        hash = XxhMergeRound(hash, i_acc[lane]);
    }

    फर्काऊ hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
स्थिर
u64 XxhFinalize (u64 i_hash, अचर u8* i_आगत, माप_प्रकार i_size)
{
    माप_प्रकार size = i_size & (XXH_STRIPE_LEN - 1);

    जबसम्म (size >= 8) {
        i_hash ^= XxhRound(0, XxhRead64(i_आगत));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_आगत += 8;
        size -= 8;
    }

    यदि (size >= 4) {
        i_hash ^= (u64)XxhRead32(i_आगत) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_आगत += 4;
        size -= 4;
    }

    जबसम्म (size > 0) {
        i_hash ^= (*i_आगत++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --size;
    }

    फर्काऊ XxhAvalanche(i_hash);
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

शून्य Xxh64_Update (Xxh64* io_state, अचर शून्य* i_data, माप_प्रकार i_size)
{
    यदि (not i_data) {
        फर्काऊ;
    }

    अचर u8* आगत = (अचर u8*)i_data;
    अचर u8* end   = आगत + i_size;

    io_state->totalLength += i_size;

    यदि (i_size < (माप_प्रकार)(XXH_STRIPE_LEN - io_state->bufferLength)) {
        स_नकल(io_state->buffer + io_state->bufferLength, आगत, i_size);
        io_state->bufferLength += (u32)i_size;
        फर्काऊ;
    }

    // fill up what the buffer already holds, and consume that
    यदि (io_state->bufferLength) {
        माप_प्रकार loadSize = XXH_STRIPE_LEN - io_state->bufferLength;

        स_नकल(io_state->buffer + io_state->bufferLength, आगत, loadSize);
        आगत += loadSize;

        XxhConsumeStripes(io_state->acc, io_state->buffer, XXH_STRIPE_LEN);
        io_state->bufferLength = 0;
    }

    यदि ((माप_प्रकार)(end - आगत) >= XXH_STRIPE_LEN) {
        आगत = XxhConsumeStripes(io_state->acc, आगत, (माप_प्रकार)(end - आगत));
    }

    यदि (आगत < end) {
        स_नकल(io_state->buffer, आगत, (माप_प्रकार)(end - आगत));
        io_state->bufferLength = (u32)(end - आगत);
    }
}

u64 Xxh64_Digest (अचर Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->totalLength >= XXH_STRIPE_LEN) ? XxhMergeAccs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->totalLength;

    फर्काऊ XxhFinalize(hash, i_state->buffer, (माप_प्रकार)i_state->totalLength);
}

#endif // d_m3HasSnapshots
