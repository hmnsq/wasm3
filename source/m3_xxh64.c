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

#شامل "m3_core.h"

#اگر d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  constants and primitives
//---------------------------------------------------------------------------------------------------------------------------------

#  define XXH_PRIME64_1    0x9E3779B185EBCA87ULL
#  define XXH_PRIME64_2    0xC2B2AE3D27D4EB4FULL
#  define XXH_PRIME64_3    0x165667B19E3779F9ULL
#  define XXH_PRIME64_4    0x85EBCA77C2B2AE63ULL
#  define XXH_PRIME64_5    0x27D4EB2F165667C5ULL

#  define XXH_STRIPE_LEN   32     // must match Xxh64.buffer

ساکن اندرونی
u32 XxhRead32 (ثابت خالی* i_ptr)
{
    u32 قدر;
    حافظہ_نقل(&قدر, i_ptr, حجم(قدر));
    M3_BSWAP_u32(قدر);
    واپس قدر;
}

ساکن اندرونی
u64 XxhRead64 (ثابت خالی* i_ptr)
{
    u64 قدر;
    حافظہ_نقل(&قدر, i_ptr, حجم(قدر));
    M3_BSWAP_u64(قدر);
    واپس قدر;
}

ساکن اندرونی
u64 XxhRotl64 (u64 i_قدر, u32 i_گنتی)
{
    واپس (i_قدر << i_گنتی) | (i_قدر >> (64 - i_گنتی));
}

// One lane taking one 8-byte word
ساکن اندرونی
u64 Xxh_باری (u64 i_acc, u64 i_داخلہ)
{
    i_acc += i_داخلہ * XXH_PRIME64_2;
    i_acc = XxhRotl64(i_acc, 31);
    واپس i_acc * XXH_PRIME64_1;
}

// ...and one lane folded into the hash at the end
ساکن اندرونی
u64 Xxh_ملاؤ_باری (u64 i_acc, u64 i_قدر)
{
    i_acc ^= Xxh_باری(0, i_قدر);
    واپس i_acc * XXH_PRIME64_1 + XXH_PRIME64_4;
}

ساکن اندرونی
u64 XxhAvalanche (u64 i_hash)
{
    i_hash ^= i_hash >> 33;
    i_hash *= XXH_PRIME64_2;
    i_hash ^= i_hash >> 29;
    i_hash *= XXH_PRIME64_3;
    i_hash ^= i_hash >> 32;
    واپس i_hash;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  internals
//---------------------------------------------------------------------------------------------------------------------------------

// Takes whole 32-byte stripes and answers where in the input it stopped
ساکن
ثابت u8* XxhConsumeStripes (u64* io_acc, ثابت u8* i_داخلہ, size_t i_سائز)
{
    ثابت u8* داخلہ = i_داخلہ;
    ثابت u8* حد_بندی = i_داخلہ + i_سائز - (XXH_STRIPE_LEN - 1);

    کرو {
        برائے (size_t lane = 0; lane < 4; ++lane) {
            io_acc[lane] = Xxh_باری(io_acc[lane], XxhRead64(داخلہ));
            داخلہ += 8;
        }
    } جب (داخلہ < حد_بندی);

    واپس داخلہ;
}

ساکن
u64 Xxh_ملاؤ_Accs (ثابت u64* i_acc)
{
    u64 hash = XxhRotl64(i_acc[0], 1) + XxhRotl64(i_acc[1], 7) + XxhRotl64(i_acc[2], 12) + XxhRotl64(i_acc[3], 18);

    برائے (size_t lane = 0; lane < 4; ++lane) {
        hash = Xxh_ملاؤ_باری(hash, i_acc[lane]);
    }

    واپس hash;
}

// The tail: whatever is left over after the last whole stripe, down to
// single bytes
ساکن
u64 XxhFinalize (u64 i_hash, ثابت u8* i_داخلہ, size_t i_سائز)
{
    size_t سائز = i_سائز & (XXH_STRIPE_LEN - 1);

    جب (سائز >= 8) {
        i_hash ^= Xxh_باری(0, XxhRead64(i_داخلہ));
        i_hash = XxhRotl64(i_hash, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        i_داخلہ += 8;
        سائز -= 8;
    }

    اگر (سائز >= 4) {
        i_hash ^= (u64)XxhRead32(i_داخلہ) * XXH_PRIME64_1;
        i_hash = XxhRotl64(i_hash, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        i_داخلہ += 4;
        سائز -= 4;
    }

    جب (سائز > 0) {
        i_hash ^= (*i_داخلہ++) * XXH_PRIME64_5;
        i_hash = XxhRotl64(i_hash, 11) * XXH_PRIME64_1;
        --سائز;
    }

    واپس XxhAvalanche(i_hash);
}


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

خالی Xxh64_Init (Xxh64* o_state, u64 i_بیج)
{
    o_state->acc[0] = i_بیج + XXH_PRIME64_1 + XXH_PRIME64_2;
    o_state->acc[1] = i_بیج + XXH_PRIME64_2;
    o_state->acc[2] = i_بیج;                   // the digest reads the seed back out of this one
    o_state->acc[3] = i_بیج - XXH_PRIME64_1;

    o_state->کل_لمبائی  = 0;
    o_state->بفر_لمبائی = 0;
}

خالی Xxh64_Update (Xxh64* io_state, ثابت خالی* i_ڈیٹا, size_t i_سائز)
{
    اگر (not i_ڈیٹا) {
        واپس;
    }

    ثابت u8* داخلہ = (ثابت u8*)i_ڈیٹا;
    ثابت u8* انتہا   = داخلہ + i_سائز;

    io_state->کل_لمبائی += i_سائز;

    اگر (i_سائز < (size_t)(XXH_STRIPE_LEN - io_state->بفر_لمبائی)) {
        حافظہ_نقل(io_state->بفر + io_state->بفر_لمبائی, داخلہ, i_سائز);
        io_state->بفر_لمبائی += (u32)i_سائز;
        واپس;
    }

    // fill up what the buffer already holds, and consume that
    اگر (io_state->بفر_لمبائی) {
        size_t بھرو_سائز = XXH_STRIPE_LEN - io_state->بفر_لمبائی;

        حافظہ_نقل(io_state->بفر + io_state->بفر_لمبائی, داخلہ, بھرو_سائز);
        داخلہ += بھرو_سائز;

        XxhConsumeStripes(io_state->acc, io_state->بفر, XXH_STRIPE_LEN);
        io_state->بفر_لمبائی = 0;
    }

    اگر ((size_t)(انتہا - داخلہ) >= XXH_STRIPE_LEN) {
        داخلہ = XxhConsumeStripes(io_state->acc, داخلہ, (size_t)(انتہا - داخلہ));
    }

    اگر (داخلہ < انتہا) {
        حافظہ_نقل(io_state->بفر, داخلہ, (size_t)(انتہا - داخلہ));
        io_state->بفر_لمبائی = (u32)(انتہا - داخلہ);
    }
}

u64 Xxh64_Digest (ثابت Xxh64* i_state)
{
    // Short input never reached the accumulators, so it is hashed from the seed
    // alone - which acc[2] still holds, untouched by Xxh64_Init.
    u64 hash = (i_state->کل_لمبائی >= XXH_STRIPE_LEN) ? Xxh_ملاؤ_Accs(i_state->acc)
                                                        : i_state->acc[2] + XXH_PRIME64_5;

    hash += i_state->کل_لمبائی;

    واپس XxhFinalize(hash, i_state->بفر, (size_t)i_state->کل_لمبائی);
}

#اگر_ختم // d_m3HasSnapshots
