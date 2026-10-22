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

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

#ਜੇ d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

ਸਥਿਰ ਸਤਰਅੰਦਰ
u32 XxhRead32 (ਅਚਲ ਖਾਲੀ* i_ptr)
{
    u32 ਮੁੱਲ;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਮੁੱਲ, i_ptr, ਆਕਾਰ(ਮੁੱਲ));
    M3_BSWAP_u32(ਮੁੱਲ);
    ਵਾਪਸ ਮੁੱਲ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 XxhRead64 (ਅਚਲ ਖਾਲੀ* i_ptr)
{
    u64 ਮੁੱਲ;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਮੁੱਲ, i_ptr, ਆਕਾਰ(ਮੁੱਲ));
    M3_BSWAP_u64(ਮੁੱਲ);
    ਵਾਪਸ ਮੁੱਲ;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 XxhRotl64 (u64 i_ਮੁੱਲ, u32 i_ਗਿਣਤੀ)
{
    ਵਾਪਸ (i_ਮੁੱਲ << i_ਗਿਣਤੀ) | (i_ਮੁੱਲ >> (64 - i_ਗਿਣਤੀ));
}

// One lane taking one 8-byte word
ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 Xxh_ਗੇੜਾ (u64 i_acc, u64 i_ਨਿਵੇਸ਼)
{
    i_acc += i_ਨਿਵੇਸ਼ * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    ਵਾਪਸ i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 Xxh_ਮਿਲਾਓ_ਗੇੜਾ (u64 i_acc, u64 i_ਮੁੱਲ)
{
    i_acc ^= Xxh_ਗੇੜਾ(0, i_ਮੁੱਲ);
    ਵਾਪਸ i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    ਵਾਪਸ i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
ਸਥਿਰ
ਅਚਲ u8* XxhConsumeStripes (u64* io_acc, ਅਚਲ u8* i_ਨਿਵੇਸ਼, size_t i_ਮਾਪ)
{
    ਅਚਲ u8* ਨਿਵੇਸ਼ = i_ਨਿਵੇਸ਼;
    ਅਚਲ u8* ਹੱਦਬੰਦੀ = i_ਨਿਵੇਸ਼ + i_ਮਾਪ - (XXH_STRIPE_LEN - 1);

    ਕਰੋ {
        ਲਈ (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = Xxh_ਗੇੜਾ(io_acc[lane], XxhRead64(ਨਿਵੇਸ਼));
            ਨਿਵੇਸ਼ += 8;
        }
    } ਜਦੋਂ (ਨਿਵੇਸ਼ < ਹੱਦਬੰਦੀ);

    ਵਾਪਸ ਨਿਵੇਸ਼;
}

ਸਥਿਰ
u64 Xxh_ਮਿਲਾਓ_Accs (ਅਚਲ u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    ਲਈ (size_t lane = 0; lane < 4; ++lane) {
        hash = Xxh_ਮਿਲਾਓ_ਗੇੜਾ(hash, i_acc[lane]);
    }

    ਵਾਪਸ hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
ਸਥਿਰ
u64 XxhFinalize (u64 i_hash, ਅਚਲ u8* i_ਨਿਵੇਸ਼, size_t i_ਮਾਪ)
{
    size_t ਮਾਪ = i_ਮਾਪ & (XXH_STRIPE_LEN - 1);

    ਜਦੋਂ (ਮਾਪ >= 8) {
        i_hash ^= Xxh_ਗੇੜਾ(0, XxhRead64(i_ਨਿਵੇਸ਼));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_ਨਿਵੇਸ਼ += 8;
        ਮਾਪ -= 8;
    }

    ਜੇ (ਮਾਪ >= 4) {
        i_hash ^= (u64)XxhRead32(i_ਨਿਵੇਸ਼) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_ਨਿਵੇਸ਼ += 4;
        ਮਾਪ -= 4;
    }

    ਜਦੋਂ (ਮਾਪ > 0) {
        i_hash ^= (*i_ਨਿਵੇਸ਼++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --ਮਾਪ;
    }

    ਵਾਪਸ XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

ਖਾਲੀ Xxh64_Init (Xxh64* o_state, u64 i_ਬੀਜ)
{
    o_state->acc[0] = i_ਬੀਜ + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_ਬੀਜ + XXH_PRIME64_2;
    o_state->acc[2] = i_ਬੀਜ;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_ਬੀਜ - XXH_PRIME64_1;

    o_state->ਕੁੱਲ_ਲੰਮਾਈ  = 0;
    o_state->ਬਫਰ_ਲੰਮਾਈ = 0;
}

ਖਾਲੀ Xxh64_Update (Xxh64* io_state, ਅਚਲ ਖਾਲੀ* i_ਅੰਕੜੇ, size_t i_ਮਾਪ)
{
    ਜੇ (not i_ਅੰਕੜੇ) {
        ਵਾਪਸ;
    }

    ਅਚਲ u8* ਨਿਵੇਸ਼ = (ਅਚਲ u8*)i_ਅੰਕੜੇ;
    ਅਚਲ u8* ਸਿਰਾ   = ਨਿਵੇਸ਼ + i_ਮਾਪ;

    io_state->ਕੁੱਲ_ਲੰਮਾਈ += i_ਮਾਪ;

    ਜੇ (i_ਮਾਪ < (size_t)(XXH_STRIPE_LEN - io_state->ਬਫਰ_ਲੰਮਾਈ)) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(io_state->ਬਫਰ + io_state->ਬਫਰ_ਲੰਮਾਈ, ਨਿਵੇਸ਼, i_ਮਾਪ);
        io_state->ਬਫਰ_ਲੰਮਾਈ += (u32)i_ਮਾਪ;
        ਵਾਪਸ;
    }

    // fill up what the buffer already holds, and consume that
    ਜੇ (io_state->ਬਫਰ_ਲੰਮਾਈ) {
        size_t ਲੱਦੋ_ਮਾਪ = XXH_STRIPE_LEN - io_state->ਬਫਰ_ਲੰਮਾਈ;

        ਸਮ੍ਰਿਤੀ_ਨਕਲ(io_state->ਬਫਰ + io_state->ਬਫਰ_ਲੰਮਾਈ, ਨਿਵੇਸ਼, ਲੱਦੋ_ਮਾਪ);
        ਨਿਵੇਸ਼ += ਲੱਦੋ_ਮਾਪ;

        XxhConsumeStripes(io_state->acc, io_state->ਬਫਰ, XXH_STRIPE_LEN);
        io_state->ਬਫਰ_ਲੰਮਾਈ = 0;
    }

    ਜੇ ((size_t)(ਸਿਰਾ - ਨਿਵੇਸ਼) >= XXH_STRIPE_LEN) {
        ਨਿਵੇਸ਼ = XxhConsumeStripes(io_state->acc, ਨਿਵੇਸ਼, (size_t)(ਸਿਰਾ - ਨਿਵੇਸ਼));
    }

    ਜੇ (ਨਿਵੇਸ਼ < ਸਿਰਾ) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(io_state->ਬਫਰ, ਨਿਵੇਸ਼, (size_t)(ਸਿਰਾ - ਨਿਵੇਸ਼));
        io_state->ਬਫਰ_ਲੰਮਾਈ = (u32)(ਸਿਰਾ - ਨਿਵੇਸ਼);
    }
}

u64 Xxh64_Digest (ਅਚਲ Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->ਕੁੱਲ_ਲੰਮਾਈ >= XXH_STRIPE_LEN) ? Xxh_ਮਿਲਾਓ_Accs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->ਕੁੱਲ_ਲੰਮਾਈ;

    ਵਾਪਸ XxhFinalize(hash, i_state->ਬਫਰ, (size_t)i_state->ਕੁੱਲ_ਲੰਮਾਈ);
}

#ਜੇ_ਅੰਤ // d_m3HasSnapshots
