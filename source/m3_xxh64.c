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

#समावेश "m3_core.h"

#अगर d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

अटल अंतरभूत
u32 XxhRead32 (स्थिर व्योम* i_ptr)
{
    u32 मूल्य;
    स_नकल(&मूल्य, i_ptr, माप(मूल्य));
    M3_BSWAP_u32(मूल्य);
    वापस मूल्य;
}

अटल अंतरभूत
u64 XxhRead64 (स्थिर व्योम* i_ptr)
{
    u64 मूल्य;
    स_नकल(&मूल्य, i_ptr, माप(मूल्य));
    M3_BSWAP_u64(मूल्य);
    वापस मूल्य;
}

अटल अंतरभूत
u64 XxhRotl64 (u64 i_मूल्य, u32 i_गिनती)
{
    वापस (i_मूल्य << i_गिनती) | (i_मूल्य >> (64 - i_गिनती));
}

// One lane taking one 8-byte word
अटल अंतरभूत
u64 XxhRound (u64 i_acc, u64 i_निवेश)
{
    i_acc += i_निवेश * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    वापस i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
अटल अंतरभूत
u64 XxhMergeRound (u64 i_acc, u64 i_मूल्य)
{
    i_acc ^= XxhRound(0, i_मूल्य);
    वापस i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

अटल अंतरभूत
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    वापस i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
अटल
स्थिर u8* XxhConsumeStripes (u64* io_acc, स्थिर u8* i_निवेश, माप_प्रकार i_आकार)
{
    स्थिर u8* निवेश = i_निवेश;
    स्थिर u8* limit = i_निवेश + i_आकार - (XXH_STRIPE_LEN - 1);

    करो {
        क्रम (माप_प्रकार lane = 0; lane < 4; ++lane) {
            io_acc[lane] = XxhRound(io_acc[lane], XxhRead64(निवेश));
            निवेश += 8;
        }
    } जबतक (निवेश < limit);

    वापस निवेश;
}

अटल
u64 XxhMergeAccs (स्थिर u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    क्रम (माप_प्रकार lane = 0; lane < 4; ++lane) {
        hash = XxhMergeRound(hash, i_acc[lane]);
    }

    वापस hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
अटल
u64 XxhFinalize (u64 i_hash, स्थिर u8* i_निवेश, माप_प्रकार i_आकार)
{
    माप_प्रकार आकार = i_आकार & (XXH_STRIPE_LEN - 1);

    जबतक (आकार >= 8) {
        i_hash ^= XxhRound(0, XxhRead64(i_निवेश));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_निवेश += 8;
        आकार -= 8;
    }

    अगर (आकार >= 4) {
        i_hash ^= (u64)XxhRead32(i_निवेश) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_निवेश += 4;
        आकार -= 4;
    }

    जबतक (आकार > 0) {
        i_hash ^= (*i_निवेश++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --आकार;
    }

    वापस XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

व्योम Xxh64_Init (Xxh64* o_state, u64 i_seed)
{
    o_state->acc[0] = i_seed + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_seed + XXH_PRIME64_2;
    o_state->acc[2] = i_seed;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_seed - XXH_PRIME64_1;

    o_state->योग_लंबाई  = 0;
    o_state->बफर_लंबाई = 0;
}

व्योम Xxh64_Update (Xxh64* io_state, स्थिर व्योम* i_तथ्य, माप_प्रकार i_आकार)
{
    अगर (not i_तथ्य) {
        वापस;
    }

    स्थिर u8* निवेश = (स्थिर u8*)i_तथ्य;
    स्थिर u8* end   = निवेश + i_आकार;

    io_state->योग_लंबाई += i_आकार;

    अगर (i_आकार < (माप_प्रकार)(XXH_STRIPE_LEN - io_state->बफर_लंबाई)) {
        स_नकल(io_state->बफर + io_state->बफर_लंबाई, निवेश, i_आकार);
        io_state->बफर_लंबाई += (u32)i_आकार;
        वापस;
    }

    // fill up what the buffer already holds, and consume that
    अगर (io_state->बफर_लंबाई) {
        माप_प्रकार load_आकार = XXH_STRIPE_LEN - io_state->बफर_लंबाई;

        स_नकल(io_state->बफर + io_state->बफर_लंबाई, निवेश, load_आकार);
        निवेश += load_आकार;

        XxhConsumeStripes(io_state->acc, io_state->बफर, XXH_STRIPE_LEN);
        io_state->बफर_लंबाई = 0;
    }

    अगर ((माप_प्रकार)(end - निवेश) >= XXH_STRIPE_LEN) {
        निवेश = XxhConsumeStripes(io_state->acc, निवेश, (माप_प्रकार)(end - निवेश));
    }

    अगर (निवेश < end) {
        स_नकल(io_state->बफर, निवेश, (माप_प्रकार)(end - निवेश));
        io_state->बफर_लंबाई = (u32)(end - निवेश);
    }
}

u64 Xxh64_Digest (स्थिर Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->योग_लंबाई >= XXH_STRIPE_LEN) ? XxhMergeAccs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->योग_लंबाई;

    वापस XxhFinalize(hash, i_state->बफर, (माप_प्रकार)i_state->योग_लंबाई);
}

#पूर्ण_अगर // d_m3HasSnapshots
