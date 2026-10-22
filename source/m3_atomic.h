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

#اگر_غیر_تعریف_شدہ m3_atomic_h
#تعریف_کرو m3_atomic_h

#شامل "m3_core.h"

#اگر d_m3HasAtomics

d_m3BeginExternC

شمار {
    c_atomic_جوڑو,
    c_atomicSub,
    c_atomicAnd,
    c_atomicOr,
    c_atomicXor,
    c_atomicXchg
};

#  define d_m3AtomicTyped(T)                                            \
    ساکن اندرونی T m3_Atomic_بھرو##T (ثابت u8* i_cell)                \
    {                                                                   \
        T قدر;                                                        \
        حافظہ_نقل (& قدر, i_cell, حجم (قدر));                       \
        M3_BSWAP_##T (قدر);                                           \
        واپس قدر;                                                   \
    }                                                                   \
                                                                        \
    ساکن اندرونی خالی m3_AtomicStore_##T (u8* o_cell, T i_قدر)       \
    {                                                                   \
        M3_BSWAP_##T (i_قدر);                                         \
        حافظہ_نقل (o_cell, & i_قدر, حجم (i_قدر));                   \
    }

d_m3AtomicTyped(u8);
d_m3AtomicTyped(u16);
d_m3AtomicTyped(u32);
d_m3AtomicTyped(u64);

ساکن اندرونی
u64 m3_Atomic_بھرو_W (ثابت u8* i_cell, u32 i_log2_چوڑائی)
{
    انتخاب (i_log2_چوڑائی) {
    حالت 0: واپس m3_Atomic_بھرو_u8(i_cell);
    حالت 1: واپس m3_Atomic_بھرو_u16(i_cell);
    حالت 2: واپس m3_Atomic_بھرو_u32(i_cell);
    طےشدہ: واپس m3_Atomic_بھرو_u64(i_cell);
    }
}

ساکن اندرونی
خالی m3_AtomicStoreW (u8* o_cell, u32 i_log2_چوڑائی, u64 i_قدر)
{
    انتخاب (i_log2_چوڑائی) {
    حالت 0: m3_AtomicStore_u8(o_cell, (u8)i_قدر); توڑو;
    حالت 1: m3_AtomicStore_u16(o_cell, (u16)i_قدر); توڑو;
    حالت 2: m3_AtomicStore_u32(o_cell, (u32)i_قدر); توڑو;
    طےشدہ: m3_AtomicStore_u64(o_cell, i_قدر); توڑو;
    }
}

ساکن اندرونی
u64 m3_AtomicRmwW (u8* io_cell, u32 i_log2_چوڑائی, u32 i_قسم, u64 i_قدر)
{
    u64 old = m3_Atomic_بھرو_W(io_cell, i_log2_چوڑائی);
    u64 updated;

    انتخاب (i_قسم) {
    حالت c_atomic_جوڑو: updated = old + i_قدر; توڑو;
    حالت c_atomicSub: updated = old - i_قدر; توڑو;
    حالت c_atomicAnd: updated = old & i_قدر; توڑو;
    حالت c_atomicOr: updated = old | i_قدر; توڑو;
    حالت c_atomicXor: updated = old ^ i_قدر; توڑو;
    طےشدہ: updated = i_قدر; توڑو;
    }

    m3_AtomicStoreW(io_cell, i_log2_چوڑائی, updated);
    واپس old;
}

ساکن اندرونی
u64 m3_AtomicCmpxchgW (u8* io_cell, u32 i_log2_چوڑائی, u64 i_expected, u64 i_replacement)
{
    u64 old = m3_Atomic_بھرو_W(io_cell, i_log2_چوڑائی);
    u64 نقاب = (i_log2_چوڑائی == 3) ? ~(u64)0 : (((u64)1 << (8u << i_log2_چوڑائی)) - 1);

    اگر (old == (i_expected & نقاب)) {
        m3_AtomicStoreW(io_cell, i_log2_چوڑائی, i_replacement);
    }
    واپس old;
}

d_m3_انتہا_Extern_C

#اگر_ختم // d_m3HasAtomics

#اگر_ختم // m3_atomic_h
