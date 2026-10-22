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

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಯದಿ d_m3HasAtomics

d_m3BeginExternC

ಗಣನಾ {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ T m3_AtomicLoad_##T (ಧ್ರುವ u8* i_cell)                \
    {                                                                   \
        T ಮೌಲ್ಯ;                                                        \
        memcpy (& ಮೌಲ್ಯ, i_cell, ಪರಿಮಾಣ (ಮೌಲ್ಯ));                       \
        M3_BSWAP_##T (ಮೌಲ್ಯ);                                           \
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ;                                                   \
    }                                                                   \
                                                                        \
    ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ ಶೂನ್ಯ m3_AtomicStore_##T (u8* o_cell, T i_ಮೌಲ್ಯ)       \
    {                                                                   \
        M3_BSWAP_##T (i_ಮೌಲ್ಯ);                                         \
        memcpy (o_cell, & i_ಮೌಲ್ಯ, ಪರಿಮಾಣ (i_ಮೌಲ್ಯ));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
u64 m3_AtomicLoadW (ಧ್ರುವ u8* i_cell, u32 i_log2Width)
{
    ವಿಕಲ್ಪ (i_log2Width) {
    ಪಕ್ಷ 0: ಪ್ರತ್ಯಾಗಮ m3_AtomicLoad_u8(i_cell);
    ಪಕ್ಷ 1: ಪ್ರತ್ಯಾಗಮ m3_AtomicLoad_u16(i_cell);
    ಪಕ್ಷ 2: ಪ್ರತ್ಯಾಗಮ m3_AtomicLoad_u32(i_cell);
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ m3_AtomicLoad_u64(i_cell);
    }
}

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
ಶೂನ್ಯ m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_ಮೌಲ್ಯ)
{
    ವಿಕಲ್ಪ (i_log2Width) {
    ಪಕ್ಷ 0: m3_AtomicStore_u8(o_cell, (u8)i_ಮೌಲ್ಯ); ಭಙ್ಗ;
    ಪಕ್ಷ 1: m3_AtomicStore_u16(o_cell, (u16)i_ಮೌಲ್ಯ); ಭಙ್ಗ;
    ಪಕ್ಷ 2: m3_AtomicStore_u32(o_cell, (u32)i_ಮೌಲ್ಯ); ಭಙ್ಗ;
    ಸ್ವಭಾವ: m3_AtomicStore_u64(o_cell, i_ಮೌಲ್ಯ); ಭಙ್ಗ;
    }
}

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_ಮೌಲ್ಯ)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    ವಿಕಲ್ಪ (i_kind) {
    ಪಕ್ಷ c_atomicAdd: updated = old + i_ಮೌಲ್ಯ; ಭಙ್ಗ;
    ಪಕ್ಷ c_atomicSub: updated = old - i_ಮೌಲ್ಯ; ಭಙ್ಗ;
    ಪಕ್ಷ c_atomicAnd: updated = old & i_ಮೌಲ್ಯ; ಭಙ್ಗ;
    ಪಕ್ಷ c_atomicOr: updated = old | i_ಮೌಲ್ಯ; ಭಙ್ಗ;
    ಪಕ್ಷ c_atomicXor: updated = old ^ i_ಮೌಲ್ಯ; ಭಙ್ಗ;
    ಸ್ವಭಾವ: updated = i_ಮೌಲ್ಯ; ಭಙ್ಗ;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    ಪ್ರತ್ಯಾಗಮ old;
}

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    ಯದಿ (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    ಪ್ರತ್ಯಾಗಮ old;
}

d_m3EndExternC

#endif // d_m3HasAtomics

#endif // m3_atomic_h
