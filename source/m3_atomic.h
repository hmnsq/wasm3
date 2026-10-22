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

#यदि_अपरिभाषितम् m3_atomic_h
#परिभाषय m3_atomic_h

#अन्तर्भाव "m3_core.h"

#यदि d_m3HasAtomics

d_m3BeginExternC

गणना {
    c_atomic_सङ्कलय,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    स्थिर अन्तःस्थ T m3_Atomic_आरोपय##T (ध्रुव u8* i_cell)                \
    {                                                                   \
        T मानम्;                                                        \
        स्मृतिं_प्रतिलिख (& मानम्, i_cell, परिमाण (मानम्));                       \
        M3_BSWAP_##T (मानम्);                                           \
        प्रत्यागम मानम्;                                                   \
    }                                                                   \
                                                                        \
    स्थिर अन्तःस्थ शून्य m3_AtomicStore_##T (u8* o_cell, T i_मानम्)       \
    {                                                                   \
        M3_BSWAP_##T (i_मानम्);                                         \
        स्मृतिं_प्रतिलिख (o_cell, & i_मानम्, परिमाण (i_मानम्));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

स्थिर अन्तःस्थ
u64 m3_Atomic_आरोपय_W (ध्रुव u8* i_cell, u32 i_log2_विस्तारः)
{
    विकल्प (i_log2_विस्तारः) {
    पक्ष 0: प्रत्यागम m3_Atomic_आरोपय_u8(i_cell);
    पक्ष 1: प्रत्यागम m3_Atomic_आरोपय_u16(i_cell);
    पक्ष 2: प्रत्यागम m3_Atomic_आरोपय_u32(i_cell);
    स्वभाव: प्रत्यागम m3_Atomic_आरोपय_u64(i_cell);
    }
}

स्थिर अन्तःस्थ
शून्य m3_AtomicStoreW (u8* o_cell, u32 i_log2_विस्तारः, u64 i_मानम्)
{
    विकल्प (i_log2_विस्तारः) {
    पक्ष 0: m3_AtomicStore_u8(o_cell, (u8)i_मानम्); भङ्ग;
    पक्ष 1: m3_AtomicStore_u16(o_cell, (u16)i_मानम्); भङ्ग;
    पक्ष 2: m3_AtomicStore_u32(o_cell, (u32)i_मानम्); भङ्ग;
    स्वभाव: m3_AtomicStore_u64(o_cell, i_मानम्); भङ्ग;
    }
}

स्थिर अन्तःस्थ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_विस्तारः, u32 i_विधा, u64 i_मानम्)
{
    u64 old = m3_Atomic_आरोपय_W(io_cell, i_log2_विस्तारः);
    u64 updated;

    विकल्प (i_विधा) {
    पक्ष c_atomic_सङ्कलय: updated = old + i_मानम्; भङ्ग;
    पक्ष c_atomicSub: updated = old - i_मानम्; भङ्ग;
    पक्ष c_atomicAnd: updated = old & i_मानम्; भङ्ग;
    पक्ष c_atomicOr: updated = old | i_मानम्; भङ्ग;
    पक्ष c_atomicXor: updated = old ^ i_मानम्; भङ्ग;
    स्वभाव: updated = i_मानम्; भङ्ग;
    }

    m3_AtomicStoreW(io_cell, i_log2_विस्तारः, updated);
    प्रत्यागम old;
}

स्थिर अन्तःस्थ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_विस्तारः, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_आरोपय_W(io_cell, i_log2_विस्तारः);
    u64 आवरणम् = (i_log2_विस्तारः == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_विस्तारः)) - 1);

    यदि (old == (i_expected & आवरणम्)) {
        m3_AtomicStoreW(io_cell, i_log2_विस्तारः, i_replacement);
    }
    प्रत्यागम old;
}

d_m3_अन्तः_Extern_C

#यदि_अन्तः // d_m3HasAtomics

#यदि_अन्तः // m3_atomic_h
