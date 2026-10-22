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

#جے_بنا_تعریف m3_atomic_h
#تعریف_کرو m3_atomic_h

#ضمن "m3_core.h"

#إذا d_m3HasAtomics

d_m3BeginExternC

تعداد {
    c_atomic_جوڑو,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ساكن مضمن T m3_Atomic_لدو##T (ثابت u8* i_cell)                \
    {                                                                   \
        T قدر;                                                        \
        حافظہ_نقل (& قدر, i_cell, حجم (قدر));                       \
        M3_BSWAP_##T (قدر);                                           \
        أرجع قدر;                                                   \
    }                                                                   \
                                                                        \
    ساكن مضمن فراغ m3_AtomicStore_##T (u8* o_cell, T i_قدر)       \
    {                                                                   \
        M3_BSWAP_##T (i_قدر);                                         \
        حافظہ_نقل (o_cell, & i_قدر, حجم (i_قدر));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ساكن مضمن
u64 m3_Atomic_لدو_W (ثابت u8* i_cell, u32 i_log2_چوڑائی)
{
    بدل (i_log2_چوڑائی) {
    حالة 0: أرجع m3_Atomic_لدو_u8(i_cell);
    حالة 1: أرجع m3_Atomic_لدو_u16(i_cell);
    حالة 2: أرجع m3_Atomic_لدو_u32(i_cell);
    افتراضي: أرجع m3_Atomic_لدو_u64(i_cell);
    }
}

ساكن مضمن
فراغ m3_AtomicStoreW (u8* o_cell, u32 i_log2_چوڑائی, u64 i_قدر)
{
    بدل (i_log2_چوڑائی) {
    حالة 0: m3_AtomicStore_u8(o_cell, (u8)i_قدر); اكسر;
    حالة 1: m3_AtomicStore_u16(o_cell, (u16)i_قدر); اكسر;
    حالة 2: m3_AtomicStore_u32(o_cell, (u32)i_قدر); اكسر;
    افتراضي: m3_AtomicStore_u64(o_cell, i_قدر); اكسر;
    }
}

ساكن مضمن
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_چوڑائی, u32 i_قسم, u64 i_قدر)
{
    u64 old = m3_Atomic_لدو_W(io_cell, i_log2_چوڑائی);
    u64 updated;

    بدل (i_قسم) {
    حالة c_atomic_جوڑو: updated = old + i_قدر; اكسر;
    حالة c_atomicSub: updated = old - i_قدر; اكسر;
    حالة c_atomicAnd: updated = old & i_قدر; اكسر;
    حالة c_atomicOr: updated = old | i_قدر; اكسر;
    حالة c_atomicXor: updated = old ^ i_قدر; اكسر;
    افتراضي: updated = i_قدر; اكسر;
    }

    m3_AtomicStoreW(io_cell, i_log2_چوڑائی, updated);
    أرجع old;
}

ساكن مضمن
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_چوڑائی, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_لدو_W(io_cell, i_log2_چوڑائی);
    u64 نقاب = (i_log2_چوڑائی == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_چوڑائی)) - 1);

    إذا (old == (i_expected & نقاب)) {
        m3_AtomicStoreW(io_cell, i_log2_چوڑائی, i_replacement);
    }
    أرجع old;
}

d_m3_سرا_Extern_C

#جے_انت // d_m3HasAtomics

#جے_انت // m3_atomic_h
