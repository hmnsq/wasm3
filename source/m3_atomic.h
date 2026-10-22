//
//  m3_atomic.h
//
//  The primitives under the threads proposal's atomic accesses. Each takes the
//  address of a naturally aligned cell in linear memory - the interpreter has
//  checked that already - and works in host byte order; a big-endian host swaps
//  on the way in and out, as the ordinary loads and stores do.
//
//  The width of the cell is a value, its log2 from 0 to 3, and so is the operation of
//  a read-modify-write, so that one body serves every instruction of a kind. Values
//  travel as u64 and are cut to the width on the way into the cell.
//
//  Every read-modify-write returns the value the cell held before.
//
//  With one thread the accesses only have to be correct in sequence, so these are
//  plain reads and writes.
//

#ifndef m3_atomic_h
#define m3_atomic_h

#अन्तर्भाव "m3_core.h"

#यदि d_m3HasAtomics

d_m3BeginExternC

गणना {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    स्थिर अन्तःस्थ T m3_AtomicLoad_##T (ध्रुव u8* i_cell)                \
    {                                                                   \
        T मूल्य;                                                        \
        memcpy (& मूल्य, i_cell, परिमाण (मूल्य));                       \
        M3_BSWAP_##T (मूल्य);                                           \
        प्रत्यागम मूल्य;                                                   \
    }                                                                   \
                                                                        \
    स्थिर अन्तःस्थ शून्य m3_AtomicStore_##T (u8* o_cell, T i_मूल्य)       \
    {                                                                   \
        M3_BSWAP_##T (i_मूल्य);                                         \
        memcpy (o_cell, & i_मूल्य, परिमाण (i_मूल्य));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

स्थिर अन्तःस्थ
u64 m3_AtomicLoadW (ध्रुव u8* i_cell, u32 i_log2Width)
{
    विकल्प (i_log2Width) {
    पक्ष 0: प्रत्यागम m3_AtomicLoad_u8(i_cell);
    पक्ष 1: प्रत्यागम m3_AtomicLoad_u16(i_cell);
    पक्ष 2: प्रत्यागम m3_AtomicLoad_u32(i_cell);
    स्वभाव: प्रत्यागम m3_AtomicLoad_u64(i_cell);
    }
}

स्थिर अन्तःस्थ
शून्य m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_मूल्य)
{
    विकल्प (i_log2Width) {
    पक्ष 0: m3_AtomicStore_u8(o_cell, (u8)i_मूल्य); भङ्ग;
    पक्ष 1: m3_AtomicStore_u16(o_cell, (u16)i_मूल्य); भङ्ग;
    पक्ष 2: m3_AtomicStore_u32(o_cell, (u32)i_मूल्य); भङ्ग;
    स्वभाव: m3_AtomicStore_u64(o_cell, i_मूल्य); भङ्ग;
    }
}

स्थिर अन्तःस्थ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_मूल्य)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    विकल्प (i_kind) {
    पक्ष c_atomicAdd: updated = old + i_मूल्य; भङ्ग;
    पक्ष c_atomicSub: updated = old - i_मूल्य; भङ्ग;
    पक्ष c_atomicAnd: updated = old & i_मूल्य; भङ्ग;
    पक्ष c_atomicOr: updated = old | i_मूल्य; भङ्ग;
    पक्ष c_atomicXor: updated = old ^ i_मूल्य; भङ्ग;
    स्वभाव: updated = i_मूल्य; भङ्ग;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    प्रत्यागम old;
}

स्थिर अन्तःस्थ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    यदि (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    प्रत्यागम old;
}

d_m3EndExternC

#endif // d_m3HasAtomics

#endif // m3_atomic_h
