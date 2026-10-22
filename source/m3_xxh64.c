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

#ଅନ୍ତର୍ଭାଵ "m3_core.h"

#ଯଦି d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u32 XxhRead32 (ଅଚଳ ଶୂନ୍ୟ* i_ptr)
{
    u32 ମୂଲ୍ୟ;
    memcpy(&ମୂଲ୍ୟ, i_ptr, ଆକାର(ମୂଲ୍ୟ));
    M3_BSWAP_u32(ମୂଲ୍ୟ);
    ଫେରନ୍ତୁ ମୂଲ୍ୟ;
}

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 XxhRead64 (ଅଚଳ ଶୂନ୍ୟ* i_ptr)
{
    u64 ମୂଲ୍ୟ;
    memcpy(&ମୂଲ୍ୟ, i_ptr, ଆକାର(ମୂଲ୍ୟ));
    M3_BSWAP_u64(ମୂଲ୍ୟ);
    ଫେରନ୍ତୁ ମୂଲ୍ୟ;
}

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 XxhRotl64 (u64 i_ମୂଲ୍ୟ, u32 i_ଗଣନା)
{
    ଫେରନ୍ତୁ (i_ମୂଲ୍ୟ << i_ଗଣନା) | (i_ମୂଲ୍ୟ >> (64 - i_ଗଣନା));
}

// One lane taking one 8-byte word
ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 XxhRound (u64 i_acc, u64 i_ନିବେଶ)
{
    i_acc += i_ନିବେଶ * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    ଫେରନ୍ତୁ i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 XxhMergeRound (u64 i_acc, u64 i_ମୂଲ୍ୟ)
{
    i_acc ^= XxhRound(0, i_ମୂଲ୍ୟ);
    ଫେରନ୍ତୁ i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    ଫେରନ୍ତୁ i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
ସ୍ଥିର
ଅଚଳ u8* XxhConsumeStripes (u64* io_acc, ଅଚଳ u8* i_ନିବେଶ, size_t i_size)
{
    ଅଚଳ u8* ନିବେଶ = i_ନିବେଶ;
    ଅଚଳ u8* limit = i_ନିବେଶ + i_size - (XXH_STRIPE_LEN - 1);

    କର {
        ପାଇଁ (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = XxhRound(io_acc[lane], XxhRead64(ନିବେଶ));
            ନିବେଶ += 8;
        }
    } ଯେତେବେଳେ (ନିବେଶ < limit);

    ଫେରନ୍ତୁ ନିବେଶ;
}

ସ୍ଥିର
u64 XxhMergeAccs (ଅଚଳ u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    ପାଇଁ (size_t lane = 0; lane < 4; ++lane) {
        hash = XxhMergeRound(hash, i_acc[lane]);
    }

    ଫେରନ୍ତୁ hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
ସ୍ଥିର
u64 XxhFinalize (u64 i_hash, ଅଚଳ u8* i_ନିବେଶ, size_t i_size)
{
    size_t size = i_size & (XXH_STRIPE_LEN - 1);

    ଯେତେବେଳେ (size >= 8) {
        i_hash ^= XxhRound(0, XxhRead64(i_ନିବେଶ));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_ନିବେଶ += 8;
        size -= 8;
    }

    ଯଦି (size >= 4) {
        i_hash ^= (u64)XxhRead32(i_ନିବେଶ) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_ନିବେଶ += 4;
        size -= 4;
    }

    ଯେତେବେଳେ (size > 0) {
        i_hash ^= (*i_ନିବେଶ++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --size;
    }

    ଫେରନ୍ତୁ XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

ଶୂନ୍ୟ Xxh64_Init (Xxh64* o_state, u64 i_seed)
{
    o_state->acc[0] = i_seed + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_seed + XXH_PRIME64_2;
    o_state->acc[2] = i_seed;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_seed - XXH_PRIME64_1;

    o_state->totalLength  = 0;
    o_state->bufferLength = 0;
}

ଶୂନ୍ୟ Xxh64_Update (Xxh64* io_state, ଅଚଳ ଶୂନ୍ୟ* i_data, size_t i_size)
{
    ଯଦି (not i_data) {
        ଫେରନ୍ତୁ;
    }

    ଅଚଳ u8* ନିବେଶ = (ଅଚଳ u8*)i_data;
    ଅଚଳ u8* end   = ନିବେଶ + i_size;

    io_state->totalLength += i_size;

    ଯଦି (i_size < (size_t)(XXH_STRIPE_LEN - io_state->bufferLength)) {
        memcpy(io_state->buffer + io_state->bufferLength, ନିବେଶ, i_size);
        io_state->bufferLength += (u32)i_size;
        ଫେରନ୍ତୁ;
    }

    // fill up what the buffer already holds, and consume that
    ଯଦି (io_state->bufferLength) {
        size_t loadSize = XXH_STRIPE_LEN - io_state->bufferLength;

        memcpy(io_state->buffer + io_state->bufferLength, ନିବେଶ, loadSize);
        ନିବେଶ += loadSize;

        XxhConsumeStripes(io_state->acc, io_state->buffer, XXH_STRIPE_LEN);
        io_state->bufferLength = 0;
    }

    ଯଦି ((size_t)(end - ନିବେଶ) >= XXH_STRIPE_LEN) {
        ନିବେଶ = XxhConsumeStripes(io_state->acc, ନିବେଶ, (size_t)(end - ନିବେଶ));
    }

    ଯଦି (ନିବେଶ < end) {
        memcpy(io_state->buffer, ନିବେଶ, (size_t)(end - ନିବେଶ));
        io_state->bufferLength = (u32)(end - ନିବେଶ);
    }
}

u64 Xxh64_Digest (ଅଚଳ Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->totalLength >= XXH_STRIPE_LEN) ? XxhMergeAccs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->totalLength;

    ଫେରନ୍ତୁ XxhFinalize(hash, i_state->buffer, (size_t)i_state->totalLength);
}

#endif // d_m3HasSnapshots
