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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_atomic_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_atomic_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

#ಆದರೆ d_m3HasAtomics

d_m3BeginExternC

ಎಣಿಕೆ {
    c_atomic_ಕೂಡಿಸು,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ಸ್ಥಿರ ಸಾಲೊಳಗೆ T m3_Atomic_ತುಂಬು##T (ಸ್ಥಿರಾಂಕ u8* i_cell)                \
    {                                                                   \
        T ಮೌಲ್ಯ;                                                        \
        ಸ್ಮರಣೆ_ನಕಲು (& ಮೌಲ್ಯ, i_cell, ಗಾತ್ರ (ಮೌಲ್ಯ));                       \
        M3_BSWAP_##T (ಮೌಲ್ಯ);                                           \
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ;                                                   \
    }                                                                   \
                                                                        \
    ಸ್ಥಿರ ಸಾಲೊಳಗೆ ಶೂನ್ಯ m3_AtomicStore_##T (u8* o_cell, T i_ಮೌಲ್ಯ)       \
    {                                                                   \
        M3_BSWAP_##T (i_ಮೌಲ್ಯ);                                         \
        ಸ್ಮರಣೆ_ನಕಲು (o_cell, & i_ಮೌಲ್ಯ, ಗಾತ್ರ (i_ಮೌಲ್ಯ));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 m3_Atomic_ತುಂಬು_W (ಸ್ಥಿರಾಂಕ u8* i_cell, u32 i_log2_ಅಗಲ)
{
    ಆಯ್ಕೆ (i_log2_ಅಗಲ) {
    ಸ್ಥಿತಿ 0: ಹಿಂತಿರುಗಿ m3_Atomic_ತುಂಬು_u8(i_cell);
    ಸ್ಥಿತಿ 1: ಹಿಂತಿರುಗಿ m3_Atomic_ತುಂಬು_u16(i_cell);
    ಸ್ಥಿತಿ 2: ಹಿಂತಿರುಗಿ m3_Atomic_ತುಂಬು_u32(i_cell);
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ m3_Atomic_ತುಂಬು_u64(i_cell);
    }
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
ಶೂನ್ಯ m3_AtomicStoreW (u8* o_cell, u32 i_log2_ಅಗಲ, u64 i_ಮೌಲ್ಯ)
{
    ಆಯ್ಕೆ (i_log2_ಅಗಲ) {
    ಸ್ಥಿತಿ 0: m3_AtomicStore_u8(o_cell, (u8)i_ಮೌಲ್ಯ); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ 1: m3_AtomicStore_u16(o_cell, (u16)i_ಮೌಲ್ಯ); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ 2: m3_AtomicStore_u32(o_cell, (u32)i_ಮೌಲ್ಯ); ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ: m3_AtomicStore_u64(o_cell, i_ಮೌಲ್ಯ); ನಿಲ್ಲಿಸು;
    }
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_ಅಗಲ, u32 i_ಬಗೆ, u64 i_ಮೌಲ್ಯ)
{
    u64 old = m3_Atomic_ತುಂಬು_W(io_cell, i_log2_ಅಗಲ);
    u64 updated;

    ಆಯ್ಕೆ (i_ಬಗೆ) {
    ಸ್ಥಿತಿ c_atomic_ಕೂಡಿಸು: updated = old + i_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_atomicSub: updated = old - i_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_atomicAnd: updated = old & i_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_atomicOr: updated = old | i_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ c_atomicXor: updated = old ^ i_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ: updated = i_ಮೌಲ್ಯ; ನಿಲ್ಲಿಸು;
    }

    m3_AtomicStoreW(io_cell, i_log2_ಅಗಲ, updated);
    ಹಿಂತಿರುಗಿ old;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_ಅಗಲ, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_ತುಂಬು_W(io_cell, i_log2_ಅಗಲ);
    u64 ಮುಖವಾಡ = (i_log2_ಅಗಲ == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_ಅಗಲ)) - 1);

    ಆದರೆ (old == (i_expected & ಮುಖವಾಡ)) {
        m3_AtomicStoreW(io_cell, i_log2_ಅಗಲ, i_replacement);
    }
    ಹಿಂತಿರುಗಿ old;
}

d_m3_ಕೊನೆ_Extern_C

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasAtomics

#ಆದರೆ_ಅಂತ್ಯ // m3_atomic_h
