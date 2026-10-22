//
//  m3_exec_atomic.h
//
//  The atomic instructions of the threads proposal, included from m3_exec.h.
//
//  Operands follow Compile_Atomic: the topmost is in _r0, and the rest are read
//  from slots in the order the stack gives them, topmost first, with the address
//  always the last. The access itself is an immediate after them. A load has only
//  the address, so it is in _r0.
//
//  The trap order is the spec's: an address that is not aligned traps before one
//  that is out of bounds, and both before anything is read or written.
//

#ఒకవేళ d_m3HasAtomics

// WIDTH is the size of the cell in bytes, and EA a u64 already holding the effective
// address. An access leaves the bounds to m3MemCheck, which a guarded memory answers
// with the fault its reservation raises; notify and wait may never touch the cell, so
// they always check.
#  define d_m3AtomicCheckWith(లోపల_BOUNDS, వెడల్పు, EA)                         \
      ఒకవేళ (M3_UNLIKELY((EA) & ((వెడల్పు) - 1))) {                              \
          newTrap(m3Err_trapUnalignedAtomic);                               \
      }                                                                     \
      ఒకవేళ (not లోపల_BOUNDS((EA) + (వెడల్పు) <= _mem->నిడివి)) {                  \
          d_బహిర్గతం_యొక్క_Bounds_Mem_Op((size_t)(EA), (u32)(వెడల్పు));                   \
      }

#  define d_m3AtomicCheck(వెడల్పు, EA)        d_m3AtomicCheckWith(m3MemCheck, వెడల్పు, EA)
#  define d_m3AtomicCheckAlways(వెడల్పు, EA)  d_m3AtomicCheckWith(M3_LIKELY, వెడల్పు, EA)

#  define d_m3AtomicCell(EA)                (m3_Mem_దత్తాంశం(_mem) + (EA))


// One op per kind of access. The last immediate describes the instruction:
//
//   bits 0-1  log2 of the width of the cell
//   bit  2    the value type is i64 rather than i32
//   bits 3-5  for a read-modify-write, which one (c_atomicAdd and the rest)
#  define d_m3_Atomic_Log2_వెడల్పు(DESC)  ((DESC) & 3u)
#  define d_m3AtomicIs64(DESC)       (((DESC) >> 2) & 1u)
#  define d_m3_Atomic_రకం(DESC)       (((DESC) >> 3) & 7u)

// an i32 result is kept sign-extended in the register, as an i32 load leaves it
#  define d_m3_Atomic_ఫలితం(DESC, V)  (d_m3AtomicIs64(DESC) ? (m3reg_t)(V) : (m3reg_t)(i32)(V))

d_m3Op(Atomic_ఎక్కించు)
{
    u64 ea = (u64)(u32)_r0;
    ea += తక్షణ(u32);
    u32 desc = తక్షణ(u32);
    u32 lw = d_m3_Atomic_Log2_వెడల్పు(desc);
    d_m3AtomicCheck((u64)1 << lw, ea);

    _r0 = d_m3_Atomic_ఫలితం(desc, m3_Atomic_ఎక్కించు_W(d_m3AtomicCell(ea), lw));
    తదుపరి_Op();
}

d_m3Op(AtomicStore)
{
    u64 ea = స్థానం(u32);
    ea += తక్షణ(u32);
    u32 desc = తక్షణ(u32);
    u32 lw = d_m3_Atomic_Log2_వెడల్పు(desc);
    d_m3AtomicCheck((u64)1 << lw, ea);

    m3_AtomicStoreW(d_m3AtomicCell(ea), lw, (u64)_r0);
    తదుపరి_Op();
}

d_m3Op(AtomicRmw)
{
    u64 ea = స్థానం(u32);
    ea += తక్షణ(u32);
    u32 desc = తక్షణ(u32);
    u32 lw = d_m3_Atomic_Log2_వెడల్పు(desc);
    d_m3AtomicCheck((u64)1 << lw, ea);

    _r0 = d_m3_Atomic_ఫలితం(desc, m3_AtomicRmwW(d_m3AtomicCell(ea), lw, d_m3_Atomic_రకం(desc), (u64)_r0));
    తదుపరి_Op();
}

// the value compared against is cut to the width of the cell. Its slot comes before
// the description that says how wide it is, so it is taken by number first.
d_m3Op(AtomicCmpxchg)
{
    i32 expected_స్థానం = తక్షణ(i32);
    u64 ea = స్థానం(u32);
    ea += తక్షణ(u32);
    u32 desc = తక్షణ(u32);
    u32 lw = d_m3_Atomic_Log2_వెడల్పు(desc);
    d_m3AtomicCheck((u64)1 << lw, ea);

    u64 expected = d_m3AtomicIs64(desc) ? *(u64*)(_sp + expected_స్థానం) : (u64) * (u32*)(_sp + expected_స్థానం);

    _r0 = d_m3_Atomic_ఫలితం(desc, m3_AtomicCmpxchgW(d_m3AtomicCell(ea), lw, expected, (u64)_r0));
    తదుపరి_Op();
}


// memory.atomic.notify: nothing waits on a memory only one thread can reach
d_m3Op(AtomicNotify)
{
    u64 ea = స్థానం(u32);
    ea += తక్షణ(u32);
    d_m3AtomicCheckAlways(పరిమాణం(u32), ea);

    _r0 = 0;
    తదుపరి_Op();
}


// The wait itself, for a cell of type T. The result is 1 when the cell did not hold
// the expected value, and 2 when the timeout ran out. A negative timeout is
// infinite, which with one thread is a wait that nothing can end.
#  define d_m3AtomicWaitOp(NAME, T)                                          \
      d_m3Op(NAME)                                                           \
      {                                                                      \
          i64 timeout = (i64)_r0;                                            \
          T expected = (T)స్థానం(T);                                           \
          u64 ea = స్థానం(u32);                                                \
          ea += తక్షణ(u32);                                              \
          d_m3AtomicCheckAlways(పరిమాణం(T), ea);                              \
                                                                             \
          ఒకవేళ (M3_UNLIKELY(not m3MemInfo(_mem)->isShared)) {                  \
              newTrap(m3_Err_trap_Expected_Shared_జ్ఞాపకం);                       \
          }                                                                  \
                                                                             \
          ఒకవేళ (m3_Atomic_ఎక్కించు##T(d_m3AtomicCell(ea)) != expected) {           \
              _r0 = 1;                                                       \
              తదుపరి_Op();                                                      \
          }                                                                  \
                                                                             \
          ఒకవేళ (timeout < 0) {                                                 \
              newTrap(m3Err_trapWaitForever);                                \
          }                                                                  \
                                                                             \
          m3_HostSleepNs((u64)timeout);                                      \
          _r0 = 2;                                                           \
          తదుపరి_Op();                                                          \
      }

d_m3AtomicWaitOp(AtomicWait32, u32);
d_m3AtomicWaitOp(AtomicWait64, u64);

#అయితే_ముగింపు // d_m3HasAtomics
