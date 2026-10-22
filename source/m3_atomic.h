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

#अगर_अघोषित m3_atomic_h
#घोषणा m3_atomic_h

#समावेश "m3_core.h"

#अगर d_m3HasAtomics

d_m3BeginExternC

क्रमागत {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    अटल अंतरभूत T m3_AtomicLoad_##T (स्थिर u8* i_cell)                \
    {                                                                   \
        T मूल्य;                                                        \
        स_नकल (& मूल्य, i_cell, माप (मूल्य));                       \
        M3_BSWAP_##T (मूल्य);                                           \
        वापस मूल्य;                                                   \
    }                                                                   \
                                                                        \
    अटल अंतरभूत व्योम m3_AtomicStore_##T (u8* o_cell, T i_मूल्य)       \
    {                                                                   \
        M3_BSWAP_##T (i_मूल्य);                                         \
        स_नकल (o_cell, & i_मूल्य, माप (i_मूल्य));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

अटल अंतरभूत
u64 m3_AtomicLoadW (स्थिर u8* i_cell, u32 i_log2Width)
{
    चयन (i_log2Width) {
    हाल 0: वापस m3_AtomicLoad_u8(i_cell);
    हाल 1: वापस m3_AtomicLoad_u16(i_cell);
    हाल 2: वापस m3_AtomicLoad_u32(i_cell);
    शेष: वापस m3_AtomicLoad_u64(i_cell);
    }
}

अटल अंतरभूत
व्योम m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_मूल्य)
{
    चयन (i_log2Width) {
    हाल 0: m3_AtomicStore_u8(o_cell, (u8)i_मूल्य); अवरोध;
    हाल 1: m3_AtomicStore_u16(o_cell, (u16)i_मूल्य); अवरोध;
    हाल 2: m3_AtomicStore_u32(o_cell, (u32)i_मूल्य); अवरोध;
    शेष: m3_AtomicStore_u64(o_cell, i_मूल्य); अवरोध;
    }
}

अटल अंतरभूत
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_मूल्य)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    चयन (i_kind) {
    हाल c_atomicAdd: updated = old + i_मूल्य; अवरोध;
    हाल c_atomicSub: updated = old - i_मूल्य; अवरोध;
    हाल c_atomicAnd: updated = old & i_मूल्य; अवरोध;
    हाल c_atomicOr: updated = old | i_मूल्य; अवरोध;
    हाल c_atomicXor: updated = old ^ i_मूल्य; अवरोध;
    शेष: updated = i_मूल्य; अवरोध;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    वापस old;
}

अटल अंतरभूत
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    अगर (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    वापस old;
}

d_m3EndExternC

#पूर्ण_अगर // d_m3HasAtomics

#पूर्ण_अगर // m3_atomic_h
