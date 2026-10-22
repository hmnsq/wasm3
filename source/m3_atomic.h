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

#അന്തര്ഭാവ "m3_core.h"

#എങ്കിൽ d_m3HasAtomics

d_m3BeginExternC

എണ്ണൽ {
    c_atomicAdd,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    സ്ഥിതം വരിയിൽ T m3_AtomicLoad_##T (സ്ഥിരം u8* i_cell)                \
    {                                                                   \
        T മൂല്യം;                                                        \
        memcpy (& മൂല്യം, i_cell, വലുപ്പം (മൂല്യം));                       \
        M3_BSWAP_##T (മൂല്യം);                                           \
        തിരികെ മൂല്യം;                                                   \
    }                                                                   \
                                                                        \
    സ്ഥിതം വരിയിൽ ശൂന്യം m3_AtomicStore_##T (u8* o_cell, T i_മൂല്യം)       \
    {                                                                   \
        M3_BSWAP_##T (i_മൂല്യം);                                         \
        memcpy (o_cell, & i_മൂല്യം, വലുപ്പം (i_മൂല്യം));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

സ്ഥിതം വരിയിൽ
u64 m3_AtomicLoadW (സ്ഥിരം u8* i_cell, u32 i_log2Width)
{
    തിരഞ്ഞെടുപ്പ് (i_log2Width) {
    അവസ്ഥ 0: തിരികെ m3_AtomicLoad_u8(i_cell);
    അവസ്ഥ 1: തിരികെ m3_AtomicLoad_u16(i_cell);
    അവസ്ഥ 2: തിരികെ m3_AtomicLoad_u32(i_cell);
    സ്ഥിരസ്ഥിതി: തിരികെ m3_AtomicLoad_u64(i_cell);
    }
}

സ്ഥിതം വരിയിൽ
ശൂന്യം m3_AtomicStoreW (u8* o_cell, u32 i_log2Width, u64 i_മൂല്യം)
{
    തിരഞ്ഞെടുപ്പ് (i_log2Width) {
    അവസ്ഥ 0: m3_AtomicStore_u8(o_cell, (u8)i_മൂല്യം); നിർത്തുക;
    അവസ്ഥ 1: m3_AtomicStore_u16(o_cell, (u16)i_മൂല്യം); നിർത്തുക;
    അവസ്ഥ 2: m3_AtomicStore_u32(o_cell, (u32)i_മൂല്യം); നിർത്തുക;
    സ്ഥിരസ്ഥിതി: m3_AtomicStore_u64(o_cell, i_മൂല്യം); നിർത്തുക;
    }
}

സ്ഥിതം വരിയിൽ
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2Width, u32 i_kind, u64 i_മൂല്യം)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 updated;

    തിരഞ്ഞെടുപ്പ് (i_kind) {
    അവസ്ഥ c_atomicAdd: updated = old + i_മൂല്യം; നിർത്തുക;
    അവസ്ഥ c_atomicSub: updated = old - i_മൂല്യം; നിർത്തുക;
    അവസ്ഥ c_atomicAnd: updated = old & i_മൂല്യം; നിർത്തുക;
    അവസ്ഥ c_atomicOr: updated = old | i_മൂല്യം; നിർത്തുക;
    അവസ്ഥ c_atomicXor: updated = old ^ i_മൂല്യം; നിർത്തുക;
    സ്ഥിരസ്ഥിതി: updated = i_മൂല്യം; നിർത്തുക;
    }

    m3_AtomicStoreW(io_cell, i_log2Width, updated);
    തിരികെ old;
}

സ്ഥിതം വരിയിൽ
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2Width, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_AtomicLoadW(io_cell, i_log2Width);
    u64 mask = (i_log2Width == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2Width)) - 1);

    എങ്കിൽ (old == (i_expected & mask)) {
        m3_AtomicStoreW(io_cell, i_log2Width, i_replacement);
    }
    തിരികെ old;
}

d_m3EndExternC

#endif // d_m3HasAtomics

#endif // m3_atomic_h
