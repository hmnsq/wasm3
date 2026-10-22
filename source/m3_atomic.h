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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_atomic_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_atomic_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

#ਜੇ d_m3HasAtomics

d_m3BeginExternC

ਗਿਣਤੀ {
    c_atomic_ਜੋੜੋ,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ਸਥਿਰ ਸਤਰਅੰਦਰ T m3_Atomic_ਲੱਦੋ##T (ਅਚਲ u8* i_cell)                \
    {                                                                   \
        T ਮੁੱਲ;                                                        \
        ਸਮ੍ਰਿਤੀ_ਨਕਲ (& ਮੁੱਲ, i_cell, ਆਕਾਰ (ਮੁੱਲ));                       \
        M3_BSWAP_##T (ਮੁੱਲ);                                           \
        ਵਾਪਸ ਮੁੱਲ;                                                   \
    }                                                                   \
                                                                        \
    ਸਥਿਰ ਸਤਰਅੰਦਰ ਖਾਲੀ m3_AtomicStore_##T (u8* o_cell, T i_ਮੁੱਲ)       \
    {                                                                   \
        M3_BSWAP_##T (i_ਮੁੱਲ);                                         \
        ਸਮ੍ਰਿਤੀ_ਨਕਲ (o_cell, & i_ਮੁੱਲ, ਆਕਾਰ (i_ਮੁੱਲ));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 m3_Atomic_ਲੱਦੋ_W (ਅਚਲ u8* i_cell, u32 i_log2_ਚੌੜਾਈ)
{
    ਚੋਣ (i_log2_ਚੌੜਾਈ) {
    ਹਾਲਤ 0: ਵਾਪਸ m3_Atomic_ਲੱਦੋ_u8(i_cell);
    ਹਾਲਤ 1: ਵਾਪਸ m3_Atomic_ਲੱਦੋ_u16(i_cell);
    ਹਾਲਤ 2: ਵਾਪਸ m3_Atomic_ਲੱਦੋ_u32(i_cell);
    ਮੂਲ: ਵਾਪਸ m3_Atomic_ਲੱਦੋ_u64(i_cell);
    }
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
ਖਾਲੀ m3_AtomicStoreW (u8* o_cell, u32 i_log2_ਚੌੜਾਈ, u64 i_ਮੁੱਲ)
{
    ਚੋਣ (i_log2_ਚੌੜਾਈ) {
    ਹਾਲਤ 0: m3_AtomicStore_u8(o_cell, (u8)i_ਮੁੱਲ); ਰੋਕੋ;
    ਹਾਲਤ 1: m3_AtomicStore_u16(o_cell, (u16)i_ਮੁੱਲ); ਰੋਕੋ;
    ਹਾਲਤ 2: m3_AtomicStore_u32(o_cell, (u32)i_ਮੁੱਲ); ਰੋਕੋ;
    ਮੂਲ: m3_AtomicStore_u64(o_cell, i_ਮੁੱਲ); ਰੋਕੋ;
    }
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_ਚੌੜਾਈ, u32 i_ਕਿਸਮ, u64 i_ਮੁੱਲ)
{
    u64 old = m3_Atomic_ਲੱਦੋ_W(io_cell, i_log2_ਚੌੜਾਈ);
    u64 updated;

    ਚੋਣ (i_ਕਿਸਮ) {
    ਹਾਲਤ c_atomic_ਜੋੜੋ: updated = old + i_ਮੁੱਲ; ਰੋਕੋ;
    ਹਾਲਤ c_atomicSub: updated = old - i_ਮੁੱਲ; ਰੋਕੋ;
    ਹਾਲਤ c_atomicAnd: updated = old & i_ਮੁੱਲ; ਰੋਕੋ;
    ਹਾਲਤ c_atomicOr: updated = old | i_ਮੁੱਲ; ਰੋਕੋ;
    ਹਾਲਤ c_atomicXor: updated = old ^ i_ਮੁੱਲ; ਰੋਕੋ;
    ਮੂਲ: updated = i_ਮੁੱਲ; ਰੋਕੋ;
    }

    m3_AtomicStoreW(io_cell, i_log2_ਚੌੜਾਈ, updated);
    ਵਾਪਸ old;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_ਚੌੜਾਈ, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_ਲੱਦੋ_W(io_cell, i_log2_ਚੌੜਾਈ);
    u64 ਨਕਾਬ = (i_log2_ਚੌੜਾਈ == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_ਚੌੜਾਈ)) - 1);

    ਜੇ (old == (i_expected & ਨਕਾਬ)) {
        m3_AtomicStoreW(io_cell, i_log2_ਚੌੜਾਈ, i_replacement);
    }
    ਵਾਪਸ old;
}

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // d_m3HasAtomics

#ਜੇ_ਅੰਤ // m3_atomic_h
