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

#ضمن "m3_core.h"

#إذا d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

ساكن مضمن
u32 XxhRead32 (ثابت فراغ* i_ptr)
{
    u32 مقدار;
    کپی_حافظه(&مقدار, i_ptr, حجم(مقدار));
    M3_BSWAP_u32(مقدار);
    أرجع مقدار;
}

ساكن مضمن
u64 XxhRead64 (ثابت فراغ* i_ptr)
{
    u64 مقدار;
    کپی_حافظه(&مقدار, i_ptr, حجم(مقدار));
    M3_BSWAP_u64(مقدار);
    أرجع مقدار;
}

ساكن مضمن
u64 XxhRotl64 (u64 i_مقدار, u32 i_شمار)
{
    أرجع (i_مقدار << i_شمار) | (i_مقدار >> (64 - i_شمار));
}

// One lane taking one 8-byte word
ساكن مضمن
u64 Xxh_دور (u64 i_acc, u64 i_درون_داد)
{
    i_acc += i_درون_داد * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    أرجع i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
ساكن مضمن
u64 Xxh_ادغام_دور (u64 i_acc, u64 i_مقدار)
{
    i_acc ^= Xxh_دور(0, i_مقدار);
    أرجع i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

ساكن مضمن
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    أرجع i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
ساكن
ثابت u8* XxhConsumeStripes (u64* io_acc, ثابت u8* i_درون_داد, size_t i_اندازه)
{
    ثابت u8* درون_داد = i_درون_داد;
    ثابت u8* حد = i_درون_داد + i_اندازه - (XXH_STRIPE_LEN - 1);

    افعل {
        لكل (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = Xxh_دور(io_acc[lane], XxhRead64(درون_داد));
            درون_داد += 8;
        }
    } طالما (درون_داد < حد);

    أرجع درون_داد;
}

ساكن
u64 Xxh_ادغام_Accs (ثابت u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    لكل (size_t lane = 0; lane < 4; ++lane) {
        hash = Xxh_ادغام_دور(hash, i_acc[lane]);
    }

    أرجع hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
ساكن
u64 XxhFinalize (u64 i_hash, ثابت u8* i_درون_داد, size_t i_اندازه)
{
    size_t اندازه = i_اندازه & (XXH_STRIPE_LEN - 1);

    طالما (اندازه >= 8) {
        i_hash ^= Xxh_دور(0, XxhRead64(i_درون_داد));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_درون_داد += 8;
        اندازه -= 8;
    }

    إذا (اندازه >= 4) {
        i_hash ^= (u64)XxhRead32(i_درون_داد) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_درون_داد += 4;
        اندازه -= 4;
    }

    طالما (اندازه > 0) {
        i_hash ^= (*i_درون_داد++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --اندازه;
    }

    أرجع XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

فراغ Xxh64_Init (Xxh64* o_state, u64 i_دانه)
{
    o_state->acc[0] = i_دانه + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_دانه + XXH_PRIME64_2;
    o_state->acc[2] = i_دانه;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_دانه - XXH_PRIME64_1;

    o_state->مجموع_درازی  = 0;
    o_state->میانگیر_درازی = 0;
}

فراغ Xxh64_Update (Xxh64* io_state, ثابت فراغ* i_داده, size_t i_اندازه)
{
    إذا (not i_داده) {
        أرجع;
    }

    ثابت u8* درون_داد = (ثابت u8*)i_داده;
    ثابت u8* انتها   = درون_داد + i_اندازه;

    io_state->مجموع_درازی += i_اندازه;

    إذا (i_اندازه < (size_t)(XXH_STRIPE_LEN - io_state->میانگیر_درازی)) {
        کپی_حافظه(io_state->میانگیر + io_state->میانگیر_درازی, درون_داد, i_اندازه);
        io_state->میانگیر_درازی += (u32)i_اندازه;
        أرجع;
    }

    // fill up what the buffer already holds, and consume that
    إذا (io_state->میانگیر_درازی) {
        size_t بارگیری_اندازه = XXH_STRIPE_LEN - io_state->میانگیر_درازی;

        کپی_حافظه(io_state->میانگیر + io_state->میانگیر_درازی, درون_داد, بارگیری_اندازه);
        درون_داد += بارگیری_اندازه;

        XxhConsumeStripes(io_state->acc, io_state->میانگیر, XXH_STRIPE_LEN);
        io_state->میانگیر_درازی = 0;
    }

    إذا ((size_t)(انتها - درون_داد) >= XXH_STRIPE_LEN) {
        درون_داد = XxhConsumeStripes(io_state->acc, درون_داد, (size_t)(انتها - درون_داد));
    }

    إذا (درون_داد < انتها) {
        کپی_حافظه(io_state->میانگیر, درون_داد, (size_t)(انتها - درون_داد));
        io_state->میانگیر_درازی = (u32)(انتها - درون_داد);
    }
}

u64 Xxh64_Digest (ثابت Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->مجموع_درازی >= XXH_STRIPE_LEN) ? Xxh_ادغام_Accs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->مجموع_درازی;

    أرجع XxhFinalize(hash, i_state->میانگیر, (size_t)i_state->مجموع_درازی);
}

#پایان_اگر // d_m3HasSnapshots
