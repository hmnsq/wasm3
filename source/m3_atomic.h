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

#அந்தர்பாவ "m3_core.h"

#எனில் d_m3HasAtomics

d_m3BeginExternC

எண்ணிக்கை {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    நிலையான உள்வரி T m3_AtomicLoad_##T (மாறிலி u8* i_cell)                \
    {                                                                   \
        T மதிப்பு;                                                        \
        memcpy (& மதிப்பு, i_cell, அளவு (மதிப்பு));                       \
        M3_BSWAP_##T (மதிப்பு);                                           \
        திரும்பு மதிப்பு;                                                   \
    }                                                                   \
                                                                        \
    நிலையான உள்வரி வெறுமை m3_AtomicStore_##T (u8* o_cell, T i_மதிப்பு)       \
    {                                                                   \
        M3_BSWAP_##T (i_மதிப்பு);                                         \
        memcpy (o_cell, & i_மதிப்பு, அளவு (i_மதிப்பு));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

நிலையான உள்வரி
u64 m3_AtomicLoadW (மாறிலி u8* i_cell, u32 i_log2Width)
{
    தேர்வு (i_log2Width) {
    நிலை 0: திரும்பு m3_AtomicLoad_u8(i_cell);
    நிலை 1: திரும்பு m3_AtomicLoad_u16(i_cell);
    நிலை 2: திரும்பு m3_AtomicLoad_u32(i_cell);
    இயல்பு: திரும்பு m3_AtomicLoad_u64(i_cell);
    }
}

நிலையான உள்வரி
வெறுமை m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_மதிப்பு)
{
    தேர்வு (i_log2Width) {
    நிலை 0: m3_AtomicStore_u8(o_cell, (u8)i_மதிப்பு); நிறுத்து;
    நிலை 1: m3_AtomicStore_u16(o_cell, (u16)i_மதிப்பு); நிறுத்து;
    நிலை 2: m3_AtomicStore_u32(o_cell, (u32)i_மதிப்பு); நிறுத்து;
    இயல்பு: m3_AtomicStore_u64(o_cell, i_மதிப்பு); நிறுத்து;
    }
}

நிலையான உள்வரி
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_மதிப்பு)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    தேர்வு (i_kind) {
    நிலை c_atomicAdd: updated = old + i_மதிப்பு; நிறுத்து;
    நிலை c_atomicSub: updated = old - i_மதிப்பு; நிறுத்து;
    நிலை c_atomicAnd: updated = old & i_மதிப்பு; நிறுத்து;
    நிலை c_atomicOr: updated = old | i_மதிப்பு; நிறுத்து;
    நிலை c_atomicXor: updated = old ^ i_மதிப்பு; நிறுத்து;
    இயல்பு: updated = i_மதிப்பு; நிறுத்து;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    திரும்பு old;
}

நிலையான உள்வரி
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    எனில் (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    திரும்பு old;
}

d_m3EndExternC

#endif // d_m3HasAtomics

#endif // m3_atomic_h
