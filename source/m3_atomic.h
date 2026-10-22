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

#నిర్వచితం_కాకపోతే m3_atomic_h
#నిర్వచించు m3_atomic_h

#అన్తర్భావ "m3_core.h"

#ఒకవేళ d_m3HasAtomics

d_m3BeginExternC

గణన {
    c_atomic_కూడు,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    స్థిర అంతఃవరుస T m3_Atomic_ఎక్కించు##T (స్థిరరాశి u8* i_cell)                \
    {                                                                   \
        T విలువ;                                                        \
        జ్ఞాపకం_నకలు (& విలువ, i_cell, పరిమాణం (విలువ));                       \
        M3_BSWAP_##T (విలువ);                                           \
        తిరిగి విలువ;                                                   \
    }                                                                   \
                                                                        \
    స్థిర అంతఃవరుస శూన్య m3_AtomicStore_##T (u8* o_cell, T i_విలువ)       \
    {                                                                   \
        M3_BSWAP_##T (i_విలువ);                                         \
        జ్ఞాపకం_నకలు (o_cell, & i_విలువ, పరిమాణం (i_విలువ));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

స్థిర అంతఃవరుస
u64 m3_Atomic_ఎక్కించు_W (స్థిరరాశి u8* i_cell, u32 i_log2_వెడల్పు)
{
    ఎంపిక (i_log2_వెడల్పు) {
    స్థితి 0: తిరిగి m3_Atomic_ఎక్కించు_u8(i_cell);
    స్థితి 1: తిరిగి m3_Atomic_ఎక్కించు_u16(i_cell);
    స్థితి 2: తిరిగి m3_Atomic_ఎక్కించు_u32(i_cell);
    అప్రమేయ: తిరిగి m3_Atomic_ఎక్కించు_u64(i_cell);
    }
}

స్థిర అంతఃవరుస
శూన్య m3_AtomicStoreW (u8* o_cell, u32 i_log2_వెడల్పు, u64 i_విలువ)
{
    ఎంపిక (i_log2_వెడల్పు) {
    స్థితి 0: m3_AtomicStore_u8(o_cell, (u8)i_విలువ); ఆపు;
    స్థితి 1: m3_AtomicStore_u16(o_cell, (u16)i_విలువ); ఆపు;
    స్థితి 2: m3_AtomicStore_u32(o_cell, (u32)i_విలువ); ఆపు;
    అప్రమేయ: m3_AtomicStore_u64(o_cell, i_విలువ); ఆపు;
    }
}

స్థిర అంతఃవరుస
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_వెడల్పు, u32 i_రకం, u64 i_విలువ)
{
    u64 old = m3_Atomic_ఎక్కించు_W(io_cell, i_log2_వెడల్పు);
    u64 updated;

    ఎంపిక (i_రకం) {
    స్థితి c_atomic_కూడు: updated = old + i_విలువ; ఆపు;
    స్థితి c_atomicSub: updated = old - i_విలువ; ఆపు;
    స్థితి c_atomicAnd: updated = old & i_విలువ; ఆపు;
    స్థితి c_atomicOr: updated = old | i_విలువ; ఆపు;
    స్థితి c_atomicXor: updated = old ^ i_విలువ; ఆపు;
    అప్రమేయ: updated = i_విలువ; ఆపు;
    }

    m3_AtomicStoreW(io_cell, i_log2_వెడల్పు, updated);
    తిరిగి old;
}

స్థిర అంతఃవరుస
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_వెడల్పు, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_ఎక్కించు_W(io_cell, i_log2_వెడల్పు);
    u64 ముసుగు = (i_log2_వెడల్పు == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_వెడల్పు)) - 1);

    ఒకవేళ (old == (i_expected & ముసుగు)) {
        m3_AtomicStoreW(io_cell, i_log2_వెడల్పు, i_replacement);
    }
    తిరిగి old;
}

d_m3_చివర_Extern_C

#అయితే_ముగింపు // d_m3HasAtomics

#అయితే_ముగింపు // m3_atomic_h
