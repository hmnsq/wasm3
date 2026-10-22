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

#اگر_تعریف_نشده m3_atomic_h
#تعریف_کن m3_atomic_h

#ضمن "m3_core.h"

#إذا d_m3HasAtomics

d_m3BeginExternC

تعداد {
    c_atomic_جمع_کن,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ساكن مضمن T m3_Atomic_بارگیری##T (ثابت u8* i_cell)                \
    {                                                                   \
        T مقدار;                                                        \
        کپی_حافظه (& مقدار, i_cell, حجم (مقدار));                       \
        M3_BSWAP_##T (مقدار);                                           \
        أرجع مقدار;                                                   \
    }                                                                   \
                                                                        \
    ساكن مضمن فراغ m3_AtomicStore_##T (u8* o_cell, T i_مقدار)       \
    {                                                                   \
        M3_BSWAP_##T (i_مقدار);                                         \
        کپی_حافظه (o_cell, & i_مقدار, حجم (i_مقدار));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ساكن مضمن
u64 m3_Atomic_بارگیری_W (ثابت u8* i_cell, u32 i_log2_عرض)
{
    بدل (i_log2_عرض) {
    حالة 0: أرجع m3_Atomic_بارگیری_u8(i_cell);
    حالة 1: أرجع m3_Atomic_بارگیری_u16(i_cell);
    حالة 2: أرجع m3_Atomic_بارگیری_u32(i_cell);
    افتراضي: أرجع m3_Atomic_بارگیری_u64(i_cell);
    }
}

ساكن مضمن
فراغ m3_AtomicStoreW (u8* o_cell, u32 i_log2_عرض, u64 i_مقدار)
{
    بدل (i_log2_عرض) {
    حالة 0: m3_AtomicStore_u8(o_cell, (u8)i_مقدار); اكسر;
    حالة 1: m3_AtomicStore_u16(o_cell, (u16)i_مقدار); اكسر;
    حالة 2: m3_AtomicStore_u32(o_cell, (u32)i_مقدار); اكسر;
    افتراضي: m3_AtomicStore_u64(o_cell, i_مقدار); اكسر;
    }
}

ساكن مضمن
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_عرض, u32 i_نوع, u64 i_مقدار)
{
    u64 old = m3_Atomic_بارگیری_W(io_cell, i_log2_عرض);
    u64 updated;

    بدل (i_نوع) {
    حالة c_atomic_جمع_کن: updated = old + i_مقدار; اكسر;
    حالة c_atomicSub: updated = old - i_مقدار; اكسر;
    حالة c_atomicAnd: updated = old & i_مقدار; اكسر;
    حالة c_atomicOr: updated = old | i_مقدار; اكسر;
    حالة c_atomicXor: updated = old ^ i_مقدار; اكسر;
    افتراضي: updated = i_مقدار; اكسر;
    }

    m3_AtomicStoreW(io_cell, i_log2_عرض, updated);
    أرجع old;
}

ساكن مضمن
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_عرض, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_بارگیری_W(io_cell, i_log2_عرض);
    u64 ماسک = (i_log2_عرض == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_عرض)) - 1);

    إذا (old == (i_expected & ماسک)) {
        m3_AtomicStoreW(io_cell, i_log2_عرض, i_replacement);
    }
    أرجع old;
}

d_m3_انتها_Extern_C

#پایان_اگر // d_m3HasAtomics

#پایان_اگر // m3_atomic_h
