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

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಆದರೆ d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u32 XxhRead32 (ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ptr)
{
    u32 ಮೌಲ್ಯ;
    ಸ್ಮರಣೆ_ನಕಲು(&ಮೌಲ್ಯ, i_ptr, ಗಾತ್ರ(ಮೌಲ್ಯ));
    M3_BSWAP_u32(ಮೌಲ್ಯ);
    ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 XxhRead64 (ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ptr)
{
    u64 ಮೌಲ್ಯ;
    ಸ್ಮರಣೆ_ನಕಲು(&ಮೌಲ್ಯ, i_ptr, ಗಾತ್ರ(ಮೌಲ್ಯ));
    M3_BSWAP_u64(ಮೌಲ್ಯ);
    ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 XxhRotl64 (u64 i_ಮೌಲ್ಯ, u32 i_ಎಣಿಕೆ)
{
    ಹಿಂತಿರುಗಿ (i_ಮೌಲ್ಯ << i_ಎಣಿಕೆ) | (i_ಮೌಲ್ಯ >> (64 - i_ಎಣಿಕೆ));
}

// One lane taking one 8-byte word
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 Xxh_ಸುತ್ತು (u64 i_acc, u64 i_ಒಳಹರಿವು)
{
    i_acc += i_ಒಳಹರಿವು * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    ಹಿಂತಿರುಗಿ i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 Xxh_ವಿಲೀನ_ಸುತ್ತು (u64 i_acc, u64 i_ಮೌಲ್ಯ)
{
    i_acc ^= Xxh_ಸುತ್ತು(0, i_ಮೌಲ್ಯ);
    ಹಿಂತಿರುಗಿ i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    ಹಿಂತಿರುಗಿ i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ u8* XxhConsumeStripes (u64* io_acc, ಸ್ಥಿರಾಂಕ u8* i_ಒಳಹರಿವು, size_t i_ಗಾತ್ರ)
{
    ಸ್ಥಿರಾಂಕ u8* ಒಳಹರಿವು = i_ಒಳಹರಿವು;
    ಸ್ಥಿರಾಂಕ u8* ಎಲ್ಲೆ = i_ಒಳಹರಿವು + i_ಗಾತ್ರ - (XXH_STRIPE_LEN - 1);

    ಮಾಡು {
        ಗಾಗಿ (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = Xxh_ಸುತ್ತು(io_acc[lane], XxhRead64(ಒಳಹರಿವು));
            ಒಳಹರಿವು += 8;
        }
    } ಆಗ (ಒಳಹರಿವು < ಎಲ್ಲೆ);

    ಹಿಂತಿರುಗಿ ಒಳಹರಿವು;
}

ಸ್ಥಿರ
u64 Xxh_ವಿಲೀನ_Accs (ಸ್ಥಿರಾಂಕ u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    ಗಾಗಿ (size_t lane = 0; lane < 4; ++lane) {
        hash = Xxh_ವಿಲೀನ_ಸುತ್ತು(hash, i_acc[lane]);
    }

    ಹಿಂತಿರುಗಿ hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
ಸ್ಥಿರ
u64 XxhFinalize (u64 i_hash, ಸ್ಥಿರಾಂಕ u8* i_ಒಳಹರಿವು, size_t i_ಗಾತ್ರ)
{
    size_t ಗಾತ್ರ_2 = i_ಗಾತ್ರ & (XXH_STRIPE_LEN - 1);

    ಆಗ (ಗಾತ್ರ_2 >= 8) {
        i_hash ^= Xxh_ಸುತ್ತು(0, XxhRead64(i_ಒಳಹರಿವು));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_ಒಳಹರಿವು += 8;
        ಗಾತ್ರ_2 -= 8;
    }

    ಆದರೆ (ಗಾತ್ರ_2 >= 4) {
        i_hash ^= (u64)XxhRead32(i_ಒಳಹರಿವು) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_ಒಳಹರಿವು += 4;
        ಗಾತ್ರ_2 -= 4;
    }

    ಆಗ (ಗಾತ್ರ_2 > 0) {
        i_hash ^= (*i_ಒಳಹರಿವು++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --ಗಾತ್ರ_2;
    }

    ಹಿಂತಿರುಗಿ XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

ಶೂನ್ಯ Xxh64_Init (Xxh64* o_state, u64 i_ಬೀಜ)
{
    o_state->acc[0] = i_ಬೀಜ + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_ಬೀಜ + XXH_PRIME64_2;
    o_state->acc[2] = i_ಬೀಜ;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_ಬೀಜ - XXH_PRIME64_1;

    o_state->ಒಟ್ಟು_ಉದ್ದಳತೆ  = 0;
    o_state->ಬಫರ್_ಉದ್ದಳತೆ = 0;
}

ಶೂನ್ಯ Xxh64_Update (Xxh64* io_state, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ದತ್ತಾಂಶ, size_t i_ಗಾತ್ರ)
{
    ಆದರೆ (not i_ದತ್ತಾಂಶ) {
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿರಾಂಕ u8* ಒಳಹರಿವು = (ಸ್ಥಿರಾಂಕ u8*)i_ದತ್ತಾಂಶ;
    ಸ್ಥಿರಾಂಕ u8* ಕೊನೆ   = ಒಳಹರಿವು + i_ಗಾತ್ರ;

    io_state->ಒಟ್ಟು_ಉದ್ದಳತೆ += i_ಗಾತ್ರ;

    ಆದರೆ (i_ಗಾತ್ರ < (size_t)(XXH_STRIPE_LEN - io_state->ಬಫರ್_ಉದ್ದಳತೆ)) {
        ಸ್ಮರಣೆ_ನಕಲು(io_state->ಬಫರ್ + io_state->ಬಫರ್_ಉದ್ದಳತೆ, ಒಳಹರಿವು, i_ಗಾತ್ರ);
        io_state->ಬಫರ್_ಉದ್ದಳತೆ += (u32)i_ಗಾತ್ರ;
        ಹಿಂತಿರುಗಿ;
    }

    // fill up what the buffer already holds, and consume that
    ಆದರೆ (io_state->ಬಫರ್_ಉದ್ದಳತೆ) {
        size_t ತುಂಬು_ಗಾತ್ರ = XXH_STRIPE_LEN - io_state->ಬಫರ್_ಉದ್ದಳತೆ;

        ಸ್ಮರಣೆ_ನಕಲು(io_state->ಬಫರ್ + io_state->ಬಫರ್_ಉದ್ದಳತೆ, ಒಳಹರಿವು, ತುಂಬು_ಗಾತ್ರ);
        ಒಳಹರಿವು += ತುಂಬು_ಗಾತ್ರ;

        XxhConsumeStripes(io_state->acc, io_state->ಬಫರ್, XXH_STRIPE_LEN);
        io_state->ಬಫರ್_ಉದ್ದಳತೆ = 0;
    }

    ಆದರೆ ((size_t)(ಕೊನೆ - ಒಳಹರಿವು) >= XXH_STRIPE_LEN) {
        ಒಳಹರಿವು = XxhConsumeStripes(io_state->acc, ಒಳಹರಿವು, (size_t)(ಕೊನೆ - ಒಳಹರಿವು));
    }

    ಆದರೆ (ಒಳಹರಿವು < ಕೊನೆ) {
        ಸ್ಮರಣೆ_ನಕಲು(io_state->ಬಫರ್, ಒಳಹರಿವು, (size_t)(ಕೊನೆ - ಒಳಹರಿವು));
        io_state->ಬಫರ್_ಉದ್ದಳತೆ = (u32)(ಕೊನೆ - ಒಳಹರಿವು);
    }
}

u64 Xxh64_Digest (ಸ್ಥಿರಾಂಕ Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->ಒಟ್ಟು_ಉದ್ದಳತೆ >= XXH_STRIPE_LEN) ? Xxh_ವಿಲೀನ_Accs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->ಒಟ್ಟು_ಉದ್ದಳತೆ;

    ಹಿಂತಿರುಗಿ XxhFinalize(hash, i_state->ಬಫರ್, (size_t)i_state->ಒಟ್ಟು_ಉದ್ದಳತೆ);
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasSnapshots
