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

#অন্তর্ভাৱ "m3_core.h"

#যদি d_m3HasAtomics

d_m3BeginExternC

গণনা {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    স্থির অন্তঃস্থ T m3_AtomicLoad_##T (ধ্রুৱ u8* i_cell)                \
    {                                                                   \
        T মান;                                                        \
        memcpy (& মান, i_cell, পরিমাণ (মান));                       \
        M3_BSWAP_##T (মান);                                           \
        ঘূৰাই মান;                                                   \
    }                                                                   \
                                                                        \
    স্থির অন্তঃস্থ শূন্য m3_AtomicStore_##T (u8* o_cell, T i_মান)       \
    {                                                                   \
        M3_BSWAP_##T (i_মান);                                         \
        memcpy (o_cell, & i_মান, পরিমাণ (i_মান));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

স্থির অন্তঃস্থ
u64 m3_AtomicLoadW (ধ্রুৱ u8* i_cell, u32 i_log2Width)
{
    ৱিকল্প (i_log2Width) {
    পক্ষ 0: ঘূৰাই m3_AtomicLoad_u8(i_cell);
    পক্ষ 1: ঘূৰাই m3_AtomicLoad_u16(i_cell);
    পক্ষ 2: ঘূৰাই m3_AtomicLoad_u32(i_cell);
    স্ৱভাৱ: ঘূৰাই m3_AtomicLoad_u64(i_cell);
    }
}

স্থির অন্তঃস্থ
শূন্য m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_মান)
{
    ৱিকল্প (i_log2Width) {
    পক্ষ 0: m3_AtomicStore_u8(o_cell, (u8)i_মান); ভঙ্গ;
    পক্ষ 1: m3_AtomicStore_u16(o_cell, (u16)i_মান); ভঙ্গ;
    পক্ষ 2: m3_AtomicStore_u32(o_cell, (u32)i_মান); ভঙ্গ;
    স্ৱভাৱ: m3_AtomicStore_u64(o_cell, i_মান); ভঙ্গ;
    }
}

স্থির অন্তঃস্থ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_মান)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    ৱিকল্প (i_kind) {
    পক্ষ c_atomicAdd: updated = old + i_মান; ভঙ্গ;
    পক্ষ c_atomicSub: updated = old - i_মান; ভঙ্গ;
    পক্ষ c_atomicAnd: updated = old & i_মান; ভঙ্গ;
    পক্ষ c_atomicOr: updated = old | i_মান; ভঙ্গ;
    পক্ষ c_atomicXor: updated = old ^ i_মান; ভঙ্গ;
    স্ৱভাৱ: updated = i_মান; ভঙ্গ;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    ঘূৰাই old;
}

স্থির অন্তঃস্থ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    যদি (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    ঘূৰাই old;
}

d_m3EndExternC

#endif // d_m3HasAtomics

#endif // m3_atomic_h
