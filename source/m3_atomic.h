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

#include "m3_core.h"

#ᬬᬾᬦ᭄ d_m3HasAtomics

d_m3BeginExternC

ga_nanaa {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    static anta_hstha T m3_AtomicLoad_##T (const u8* i_cell)                \
    {                                                                   \
        T value;                                                        \
        memcpy (& value, i_cell, parimaa_na (value));                       \
        M3_BSWAP_##T (value);                                           \
        ᬯᬮᬶ value;                                                   \
    }                                                                   \
                                                                        \
    static anta_hstha void m3_AtomicStore_##T (u8* o_cell, T i_value)       \
    {                                                                   \
        M3_BSWAP_##T (i_value);                                         \
        memcpy (o_cell, & i_value, parimaa_na (i_value));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

static anta_hstha
u64 m3_AtomicLoadW (const u8* i_cell, u32 i_log2Width)
{
    switch (i_log2Width) {
    pak_sa 0: ᬯᬮᬶ m3_AtomicLoad_u8(i_cell);
    pak_sa 1: ᬯᬮᬶ m3_AtomicLoad_u16(i_cell);
    pak_sa 2: ᬯᬮᬶ m3_AtomicLoad_u32(i_cell);
    default: ᬯᬮᬶ m3_AtomicLoad_u64(i_cell);
    }
}

static anta_hstha
void m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_value)
{
    switch (i_log2Width) {
    pak_sa 0: m3_AtomicStore_u8(o_cell, (u8)i_value); bha_nga;
    pak_sa 1: m3_AtomicStore_u16(o_cell, (u16)i_value); bha_nga;
    pak_sa 2: m3_AtomicStore_u32(o_cell, (u32)i_value); bha_nga;
    default: m3_AtomicStore_u64(o_cell, i_value); bha_nga;
    }
}

static anta_hstha
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_value)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    switch (i_kind) {
    pak_sa c_atomicAdd: updated = old + i_value; bha_nga;
    pak_sa c_atomicSub: updated = old - i_value; bha_nga;
    pak_sa c_atomicAnd: updated = old & i_value; bha_nga;
    pak_sa c_atomicOr: updated = old | i_value; bha_nga;
    pak_sa c_atomicXor: updated = old ^ i_value; bha_nga;
    default: updated = i_value; bha_nga;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    ᬯᬮᬶ old;
}

static anta_hstha
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    ᬬᬾᬦ᭄ (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    ᬯᬮᬶ old;
}

d_m3EndExternC

#endif // d_m3HasAtomics

#endif // m3_atomic_h
