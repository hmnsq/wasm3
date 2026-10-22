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

#ଅନ୍ତର୍ଭାଵ "m3_core.h"

#ଯଦି d_m3HasAtomics

d_m3BeginExternC

ଗଣନା {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ସ୍ଥିର ଧାଡ଼ିଭିତରେ T m3_AtomicLoad_##T (ଅଚଳ u8* i_cell)                \
    {                                                                   \
        T ମୂଲ୍ୟ;                                                        \
        memcpy (& ମୂଲ୍ୟ, i_cell, ଆକାର (ମୂଲ୍ୟ));                       \
        M3_BSWAP_##T (ମୂଲ୍ୟ);                                           \
        ଫେରନ୍ତୁ ମୂଲ୍ୟ;                                                   \
    }                                                                   \
                                                                        \
    ସ୍ଥିର ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ m3_AtomicStore_##T (u8* o_cell, T i_ମୂଲ୍ୟ)       \
    {                                                                   \
        M3_BSWAP_##T (i_ମୂଲ୍ୟ);                                         \
        memcpy (o_cell, & i_ମୂଲ୍ୟ, ଆକାର (i_ମୂଲ୍ୟ));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 m3_AtomicLoadW (ଅଚଳ u8* i_cell, u32 i_log2Width)
{
    ବାଛ (i_log2Width) {
    ସ୍ଥିତି 0: ଫେରନ୍ତୁ m3_AtomicLoad_u8(i_cell);
    ସ୍ଥିତି 1: ଫେରନ୍ତୁ m3_AtomicLoad_u16(i_cell);
    ସ୍ଥିତି 2: ଫେରନ୍ତୁ m3_AtomicLoad_u32(i_cell);
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ m3_AtomicLoad_u64(i_cell);
    }
}

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
ଶୂନ୍ୟ m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_ମୂଲ୍ୟ)
{
    ବାଛ (i_log2Width) {
    ସ୍ଥିତି 0: m3_AtomicStore_u8(o_cell, (u8)i_ମୂଲ୍ୟ); ଭାଙ୍ଗ;
    ସ୍ଥିତି 1: m3_AtomicStore_u16(o_cell, (u16)i_ମୂଲ୍ୟ); ଭାଙ୍ଗ;
    ସ୍ଥିତି 2: m3_AtomicStore_u32(o_cell, (u32)i_ମୂଲ୍ୟ); ଭାଙ୍ଗ;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: m3_AtomicStore_u64(o_cell, i_ମୂଲ୍ୟ); ଭାଙ୍ଗ;
    }
}

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_ମୂଲ୍ୟ)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    ବାଛ (i_kind) {
    ସ୍ଥିତି c_atomicAdd: updated = old + i_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_atomicSub: updated = old - i_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_atomicAnd: updated = old & i_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_atomicOr: updated = old | i_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    ସ୍ଥିତି c_atomicXor: updated = old ^ i_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: updated = i_ମୂଲ୍ୟ; ଭାଙ୍ଗ;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    ଫେରନ୍ତୁ old;
}

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    ଯଦି (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    ଫେରନ୍ତୁ old;
}

d_m3EndExternC

#endif // d_m3HasAtomics

#endif // m3_atomic_h
