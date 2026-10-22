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

#که_نه_تعریف_شوی m3_atomic_h
#تعریف_کړه m3_atomic_h

#ضمن "m3_core.h"

#إذا d_m3HasAtomics

d_m3BeginExternC

تعداد {
    c_atomic_جمع_کړه,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ساكن مضمن T m3_Atomic_بارول##T (ثابت u8* i_cell)                \
    {                                                                   \
        T ارزښت;                                                        \
        حافظه_نقل (& ارزښت, i_cell, حجم (ارزښت));                       \
        M3_BSWAP_##T (ارزښت);                                           \
        أرجع ارزښت;                                                   \
    }                                                                   \
                                                                        \
    ساكن مضمن فراغ m3_AtomicStore_##T (u8* o_cell, T i_ارزښت)       \
    {                                                                   \
        M3_BSWAP_##T (i_ارزښت);                                         \
        حافظه_نقل (o_cell, & i_ارزښت, حجم (i_ارزښت));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ساكن مضمن
u64 m3_Atomic_بارول_W (ثابت u8* i_cell, u32 i_log2_پلنوالی)
{
    بدل (i_log2_پلنوالی) {
    حالة 0: أرجع m3_Atomic_بارول_u8(i_cell);
    حالة 1: أرجع m3_Atomic_بارول_u16(i_cell);
    حالة 2: أرجع m3_Atomic_بارول_u32(i_cell);
    افتراضي: أرجع m3_Atomic_بارول_u64(i_cell);
    }
}

ساكن مضمن
فراغ m3_AtomicStoreW (u8* o_cell, u32 i_log2_پلنوالی, u64 i_ارزښت)
{
    بدل (i_log2_پلنوالی) {
    حالة 0: m3_AtomicStore_u8(o_cell, (u8)i_ارزښت); اكسر;
    حالة 1: m3_AtomicStore_u16(o_cell, (u16)i_ارزښت); اكسر;
    حالة 2: m3_AtomicStore_u32(o_cell, (u32)i_ارزښت); اكسر;
    افتراضي: m3_AtomicStore_u64(o_cell, i_ارزښت); اكسر;
    }
}

ساكن مضمن
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_پلنوالی, u32 i_ډول, u64 i_ارزښت)
{
    u64 old = m3_Atomic_بارول_W(io_cell, i_log2_پلنوالی);
    u64 updated;

    بدل (i_ډول) {
    حالة c_atomic_جمع_کړه: updated = old + i_ارزښت; اكسر;
    حالة c_atomicSub: updated = old - i_ارزښت; اكسر;
    حالة c_atomicAnd: updated = old & i_ارزښت; اكسر;
    حالة c_atomicOr: updated = old | i_ارزښت; اكسر;
    حالة c_atomicXor: updated = old ^ i_ارزښت; اكسر;
    افتراضي: updated = i_ارزښت; اكسر;
    }

    m3_AtomicStoreW(io_cell, i_log2_پلنوالی, updated);
    أرجع old;
}

ساكن مضمن
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_پلنوالی, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_بارول_W(io_cell, i_log2_پلنوالی);
    u64 ماسک = (i_log2_پلنوالی == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_پلنوالی)) - 1);

    إذا (old == (i_expected & ماسک)) {
        m3_AtomicStoreW(io_cell, i_log2_پلنوالی, i_replacement);
    }
    أرجع old;
}

d_m3_پای_ټکی_Extern_C

#د_که_پای // d_m3HasAtomics

#د_که_پای // m3_atomic_h
