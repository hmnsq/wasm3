//
//  m3_env.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_env_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_env_h

#ಅನ್ತರ್ಭಾವ "wasm3.h"
#ಅನ್ತರ್ಭಾವ "m3_code.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"

d_m3BeginExternC

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ ValCtx ValCtx;   // the validator's scratch, defined in m3_validate.c


//---------------------------------------------------------------------------------------------------------------------------------

// Page counts are u64 because a memory64 type may declare up to 2^48 pages.
// Nothing near that can be backed, but the module still has to parse and
// validate; it is instantiation that fails.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಸ್ಮರಣೆ_Info {
    u64  init_ಪುಟಗಳು;
    u64  max_ಪುಟಗಳು;
    u32  ಪುಟ_ಗಾತ್ರ;
    bool hasMax;         // a declared maximum of 0 is not the same as none
    bool isMemory64;     // addressed by i64 rather than i32
    bool isShared;       // the threads proposal: a maximum is required
} M3_ಸ್ಮರಣೆ_Info;


// One linear memory. Memories belong to the module that declares them, not to
// the runtime: two modules loaded into the same runtime each get their own, and
// the interpreter reaches the one it is currently running against through the
// _mem pseudo-register (see M3MemoryHeader).
//
// Every entry is individually allocated so that a slot may later be pointed at
// a memory another module owns - growing one has to be visible through every
// name it has. 'owner' says which module allocated it, and so which module
// frees it.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಸ್ಮರಣೆ {
    M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ*  mallocated;

    u64              ಅಂಕೆ_ಪುಟಗಳು;
    u64              max_ಪುಟಗಳು;
    u64              init_ಪುಟಗಳು;
    u32              ಪುಟ_ಗಾತ್ರ;
    bool             hasMax;         // see M3MemoryInfo
    bool             isMemory64;     // addressed by i64 rather than i32
    bool             isShared;       // see M3MemoryInfo

    ರಚನೆ M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;

#ಆದರೆ d_m3_Guarded_ಸ್ಮರಣೆ
    // The slot of the guarded arena this memory's bytes live in, and so what
    // 'mallocated' points into rather than the heap - see Guard_TakeSlot
    ಶೂನ್ಯ* guard_ಸ್ಥಾನ;
#ಆದರೆ_ಅಂತ್ಯ
} M3_ಸ್ಮರಣೆ;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ M3_ಸ್ಮರಣೆ* IM3_ಸ್ಮರಣೆ;

// The value type a memory's addresses, page counts and lengths are expressed
// in. Every instruction naming the memory takes and returns that type.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
m3type_t ಸ್ಮರಣೆ_Addr_Type (ಸ್ಥಿರಾಂಕ M3_ಸ್ಮರಣೆ* i_ಸ್ಮರಣೆ)
{
    ಹಿಂತಿರುಗಿ i_ಸ್ಮರಣೆ->isMemory64 ? c_m3Type_i64 : c_m3Type_i32;
}


// A module that declares no custom page size means the default one. The parser
// leaves that as zero, and InitMemory only fills it in when it allocates, so a
// memory has to be asked rather than read directly until then.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u32 ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ (ಸ್ಥಿರಾಂಕ M3_ಸ್ಮರಣೆ* i_ಸ್ಮರಣೆ)
{
    ಹಿಂತಿರುಗಿ i_ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ ? i_ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ : d_m3_Default_Mem_ಪುಟ_ಗಾತ್ರ;
}


//---------------------------------------------------------------------------------------------------------------------------------

// A table type as the module declared it. Sizes stay u32 - a table64 may name
// far more entries than that, but d_m3MaxSaneTableSize refuses anything near
// the limit long before it matters.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಕೋಷ್ಟಕ_Info {
    m3type_t elemType;
    u32      init_ಗಾತ್ರ;
    u32      max_ಗಾತ್ರ;
    bool     hasMax;         // a declared maximum of 0 is not the same as none
    bool     isTable64;      // indexed by i64 rather than i32
} M3_ಕೋಷ್ಟಕ_Info;


// A table's elements are opaque pointer-sized references: IM3Function for a
// funcref table, a host handle for an externref one. NULL is the null reference.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಕೋಷ್ಟಕ {
    ಶೂನ್ಯ**           elements;
    u32              ಗಾತ್ರ_2;
    u32              max_ಗಾತ್ರ;            // 0 when the module declared no maximum
    m3type_t         type;

    // Every slot starts out holding this, rather than null. A table whose
    // element type is not nullable has to say what to fill itself with.
    ಬೈಟುಗಳು_t          init_ಅಭಿವ್ಯಕ್ತಿ;
    u32              init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ;

    u32              init_ಗಾತ್ರ;       // the declared minimum
    bool             hasMax;         // a declared maximum of 0 is not the same as none
    bool             isTable64;      // indexed by i64 rather than i32

    ರಚನೆ M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;
} M3_ಕೋಷ್ಟಕ;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ M3_ಕೋಷ್ಟಕ* IM3_ಕೋಷ್ಟಕ;

// The value type a table's indexes and sizes are expressed in - table64 is the
// same choice as memory64, made per table.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
m3type_t ಕೋಷ್ಟಕ_Addr_Type (ಸ್ಥಿರಾಂಕ M3_ಕೋಷ್ಟಕ* i_ಕೋಷ್ಟಕ)
{
    ಹಿಂತಿರುಗಿ i_ಕೋಷ್ಟಕ->isTable64 ? c_m3Type_i64 : c_m3Type_i32;
}

// Element segment modes, from the low two bits of the segment's flags
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಎಣಿಕೆ {
    c_m3_Elem_ಸಕ್ರಿಯ      = 0,        // table 0
    c_m3Elem_passive     = 1,
    c_m3_Elem_ಸಕ್ರಿಯ_Idx   = 2,        // explicit table index
    c_m3Elem_declarative = 3
} M3_Element_ಬಹುಲಕ;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3ElementSegment {
    ಬೈಟುಗಳು_t  init_ಅಭಿವ್ಯಕ್ತಿ;       // active segments only: the offset
    ಬೈಟುಗಳು_t  elements;       // funcidx list, or const exprs when isExpr
    ಶೂನ್ಯ**   resolved;       // passive segments only, for table.init

    u32      init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ;
    u32      ಅಂಕೆ_Elements;
    u32      ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ;

    m3type_t type;
    u8       ಬಹುಲಕ;
    bool     is_ಅಭಿವ್ಯಕ್ತಿ;
    bool     dropped;
} M3ElementSegment;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ದತ್ತಾಂಶ_Segment {
    ಸ್ಥಿರಾಂಕ u8* init_ಅಭಿವ್ಯಕ್ತಿ;           // wasm code
    ಸ್ಥಿರಾಂಕ u8* ದತ್ತಾಂಶ;

    u32       init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ;
    u32       ಸ್ಮರಣೆ_Region;
    u32       ಗಾತ್ರ_2;

    bool      isPassive;
    bool      dropped;            // active segments are dropped once instantiated
} M3_ದತ್ತಾಂಶ_Segment;

//---------------------------------------------------------------------------------------------------------------------------------

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Global {
    M3ImportInfo     import;

    // An import linked to another module's export: the global that actually
    // holds the value. Reads and writes go there, so a mutable global stays one
    // cell seen under two names. NULL for anything else, including an import a
    // host supplied a value for through m3_LinkGlobal.
    ರಚನೆ M3Global* resolved;

    ಒಕ್ಕೂಟ {
        i32 i32_ಮೌಲ್ಯ;
        i64 i64_ಮೌಲ್ಯ;
#ಆದರೆ d_m3HasFloat
        f64 f64_ಮೌಲ್ಯ;
        f32 f32_ಮೌಲ್ಯ;
#ಆದರೆ_ಅಂತ್ಯ
        ಶೂನ್ಯ* ref_ಮೌಲ್ಯ;
    };

    ಬೈಟುಗಳು_t  init_ಅಭಿವ್ಯಕ್ತಿ;       // wasm code
    u32      init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ;
    m3type_t type;
    bool     imported;
    bool     isMutable;
} M3Global;


//---------------------------------------------------------------------------------------------------------------------------------

#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching

// An exception / control tag, as the tag section declares it. Tags are compared by
// identity, and this struct's address is that identity.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಟ್ಯಾಗ್ {
    M3ImportInfo  import;
    IM3FuncType   type;           // params are the payload; results are empty for EH, can be non-empty for stack switching
    ರಚನೆ M3_ಟ್ಯಾಗ್* resolved;
    bool          imported;
} M3_ಟ್ಯಾಗ್;

// A tag m3_NewTag made. It belongs to the runtime rather than to a module, and
// is linked in front of the runtime's others rather than kept in an array, which
// would move the tags - and with them their identities - whenever it grew.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_Host_ಟ್ಯಾಗ್ {
    ರಚನೆ M3_Host_ಟ್ಯಾಗ್* ಮುಂದಿನ;
    M3_ಟ್ಯಾಗ್             ಟ್ಯಾಗ್;
} M3_Host_ಟ್ಯಾಗ್;

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ಆದರೆ d_m3HasExceptionHandling

// A thrown exception: the tag that identifies it plus the payload the throw
// site popped off the stack, one u64 per value (32-bit values are stored
// zero-extended, floats as their bit pattern).
//
// Exception objects belong to the runtime, not to the code that caught them.
//
// One caught by a clause that does not reify it - catch or catch_all rather
// than their _ref forms - is unreachable the moment its payload has been copied
// out, and is released there. Anything else stays on the runtime's list until
// the outermost m3_Call() returns, which is the last moment an exnref can still
// be reached from the Wasm stack. An exnref parked in a global or a table
// outlives that and is left dangling - wasm3 has no collector to say otherwise.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Exception {
    ರಚನೆ M3Exception* ಮುಂದಿನ;           // the runtime's allocation list,
    ರಚನೆ M3Exception* prev;           //   doubly linked so one can leave early
    IM3_ಟ್ಯಾಗ್              ಟ್ಯಾಗ್;
    u32                 ಅಂಕೆ_ವಾದಗಳು;
    bool                reified;        // an exnref to this has been handed out
    u64                 ವಾದಗಳು[];
} M3Exception;


M3Exception* NewException (IM3Runtime io_runtime, IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್, u32 i_ಅಂಕೆ_ವಾದಗಳು);
ಶೂನ್ಯ         FreeException (IM3Runtime io_runtime, M3Exception* i_exception);
ಶೂನ್ಯ         FreeExceptions (IM3Runtime io_runtime);

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

// A native frame the interpreter had claimed at the moment a continuation
// suspended, recorded so that resuming can build the same frame again.
//
// Nothing pushes these during ordinary execution. They are written on the way
// out: a suspend travels up the native stack as m3Err_continuationSuspended,
// and each operation that was holding a frame recognises the marker on its way
// past and appends the record describing itself - innermost first, which is
// simply the order the unwind visits them in. Resuming walks the array back
// down, rebuilding one native frame per entry, and lands on the suspension
// point at the bottom.
//
// Every operation that keeps a native frame needs an entry here, because each
// one owns a return protocol that only exists while its frame is standing:
// op_Call and friends resume the caller, op_Loop reads the continue sentinel,
// op_TryTable catches a pending exception.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಎಣಿಕೆ M3_ಚೌಕಟ್ಟು_ಬಗೆ {
    ಚೌಕಟ್ಟು_call,          // op_Call / op_CallRef / op_CallIndirect
    ಚೌಕಟ್ಟು_loop,          // op_Loop
#  ಆದರೆ d_m3HasExceptionHandling
    ಚೌಕಟ್ಟು_try,           // op_TryTable
#  endif
#  ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
    ಚೌಕಟ್ಟು_ನಮೂದು,         // op_Entry, in builds where it keeps a frame
#  endif
    ಚೌಕಟ್ಟು_resume,        // op_Resume, when the suspend is not its to answer
} M3_ಚೌಕಟ್ಟು_ಬಗೆ;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಚೌಕಟ್ಟು {
    u8        ಬಗೆ;      // M3FrameKind
    pc_t      pc;        // call: the return address | loop: the loop id, which is
                         // also its body | try: the clause table | entry: unused
    m3stack_t sp;
    IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ;    // _mem is re-derived from this rather than stored:
                         // growing linear memory moves the header, so only the
                         // M3Memory stays valid across a suspend

    ಒಕ್ಕೂಟ {
        // op_Call resumes the caller with the registers its own native frame was
        // holding, so those belong to the frame, not to the suspension point
        ರಚನೆ {
            IM3Function function;
            m3reg_t     r0;
#  ಆದರೆ d_m3HasFloat
            f64 fp0;
#  endif
        } call;
#  ಆದರೆ d_m3HasExceptionHandling
        ರಚನೆ {
            u32  ಅಂಕೆ_Clauses;
            bool handlers_ಜೀವಂತ;   // false once op_PopHandlers has retired the region
        } try_;
#  endif
#  ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
        ರಚನೆ {
            IM3Function function;
        } ನಮೂದು;
#  endif
        // A resume that the suspend travelled straight through, because the
        // tag was not one its handlers named. The continuation it was running
        // is part of what got captured, so everything needed to set it going
        // again under the same handlers is kept here.
        ರಚನೆ {
            ರಚನೆ M3Continuation* cont;
            pc_t                   handlersPC;
            u32                    ಅಂಕೆ_Handlers;
            pc_t                   resultsPC;
            u32                    ಅಂಕೆ_Results;
        } resume;
    };
} M3_ಚೌಕಟ್ಟು;

// Which of the three resume forms an op_Resume was emitted for
ಎಣಿಕೆ {
    d_m3_Resume_ಸಾಮಾನ್ಯ = 0,
    d_m3ResumeThrow,
    d_m3ResumeThrowRef,
};

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಎಣಿಕೆ M3ContinuationState {
    cont_allocated = 0,
    cont_running,
    cont_suspended,
    cont_returned,
    cont_consumed
} M3ContinuationState;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3ResumeHandler {
    u8     ಬಗೆ;        // 0: (on $t $h), 1: (on $t switch)
    IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್;
    pc_t   stubPC;
} M3ResumeHandler;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Continuation {
    ರಚನೆ M3Continuation* ಮುಂದಿನ;           // linked list in runtime for cleanup
    IM3FuncType            type;            // continuation type (cont $ft)
    IM3Function            ನಮೂದು_Function;   // target function
    M3ContinuationState    state;

    m3slot_t*              val_ರಾಶಿ;        // allocated value stack buffer
    u32                    ಅಂಕೆ_ರಾಶಿ_Slots;
    m3stack_t              sp;              // current value stack pointer

    M3_ಚೌಕಟ್ಟು*               ಚೌಕಟ್ಟುಗಳು;          // native frames recorded by the last suspend
    u32                    ಚೌಕಟ್ಟುಗಳು_Cap;       // entries allocated
    u32                    ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು;       // entries in use, innermost first

    pc_t                   pc;              // resume instruction pointer
    u8                     suspend_ಬಿಂದು;    // M3SafePointKind: what left pc where it is
    m3reg_t                r0;              // saved accumulator
#  ಆದರೆ d_m3HasFloat
    f64 fp0;
#  endif

    // Bound args count (for cont.bind)
    u32 bound_ವಾದಗಳು_ಎಣಿಕೆ;

#  ಆದರೆ d_m3HasExceptionHandling
    // resume_throw: raised at the suspension point instead of carrying on from
    // it, so the try regions inside the continuation get their turn first
    M3Exception* resumeThrow;
#  endif

    // Where the values go when this continuation is next given some: the slots
    // its own suspend or switch is waiting on.
    u32                    ಅಂಕೆ_Suspend_Results;
    i32                    suspend_ಫಲಿತಾಂಶ_Offsets[d_m3MaxContinuationPayload];
    u32                    suspend_ಫಲಿತಾಂಶ_Is64[d_m3MaxContinuationPayload];

    // Active handler table from enclosing resume:
    u32                    ಅಂಕೆ_Handlers;
    pc_t                   handlersPC;
    ರಚನೆ M3Continuation* ಪೋಷಕ;         // resumer continuation
} M3Continuation, *IM3Continuation;

M3_ಫಲಿತಾಂಶ        Continuation_Acquire_ರಾಶಿ (IM3Runtime runtime, IM3Continuation cont);
ಶೂನ್ಯ            Continuation_Recycle_ರಾಶಿ (IM3Runtime runtime, IM3Continuation cont);
IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function);
IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont);
ಶೂನ್ಯ            Continuation_ReleaseAll (IM3Runtime io_runtime);

// Appends one frame to the suspending continuation. Returns the marker the
// caller should keep unwinding with: the suspend marker when the frame was
// recorded, or a trap when it could not be - a continuation that cannot record
// its frames cannot be resumed, so the suspend has to become a trap instead.
m3ret_t         Continuation_ದಾಖಲೆ_ಚೌಕಟ್ಟು (IM3Runtime i_runtime, ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು* i_ಚೌಕಟ್ಟು);

// Points runtime->stack, where the host's next call puts its arguments, above
// whatever a suspended root continuation holds - or back at the bottom when
// there is none.
ಶೂನ್ಯ            Runtime_ಇರಿಸು_Call_ರಾಶಿ (IM3Runtime io_runtime);

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


#ಆದರೆ d_m3HasSnapshots
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3EmbeddedSnapshot {
    cstr_t                     name;
    ಬೈಟುಗಳು_t                    ದತ್ತಾಂಶ;
    u32                        ಗಾತ್ರ_2;
    ರಚನೆ M3EmbeddedSnapshot* ಮುಂದಿನ;
} M3EmbeddedSnapshot;
#ಆದರೆ_ಅಂತ್ಯ


//---------------------------------------------------------------------------------------------------------------------------------

// One entry of the export section. Export names belong to the module rather than
// to what they name: one entity can be exported under any number of names, and an
// import re-exported under a name of the module's own, whose slot linking points
// at another module's entity. Every lookup by name goes through these, and
// resolves the index against the module's index space at the time it is asked.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Export {
    cstr_t name;
    u32    name_ಉದ್ದಳತೆ;   // the name may hold a NUL, which ends the C string early
    u32    index;        // into the index space 'kind' names
    u8     ಬಗೆ;         // d_externalKind_*
} M3Export;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Module {
    ರಚನೆ M3Runtime*     runtime;
    ರಚನೆ M3Environment* environment;

    ಬೈಟುಗಳು_t               wasm_ಆರಂಭ;
    ಬೈಟುಗಳು_t               wasm_ಕೊನೆ;

    cstr_t                name;

    u32                   ಅಂಕೆ_Func_Types;
    IM3FuncType*          funcTypes;              // array of pointers to list of FuncTypes

    u32                   ಅಂಕೆ_Func_Imports;
    u32                   ಅಂಕೆ_Functions;
    u32                   allFunctions;           // allocated functions count
    M3Function*           functions;

    i32                   ಆರಂಭ_Function;

    u32                   ಅಂಕೆ_ದತ್ತಾಂಶ_Segments;
    M3_ದತ್ತಾಂಶ_Segment*        ದತ್ತಾಂಶ_Segments;

    u32                   ದತ್ತಾಂಶ_ಎಣಿಕೆ;          // from the data count section
    bool                  has_ದತ್ತಾಂಶ_ಎಣಿಕೆ;

    // Bitset of the functions ref.func may name: those exported, or referenced
    // by an element segment or a global initializer. One bit per function.
    u8*                   declaredFuncs;

    //u32                     importedGlobals;
    u32                   ಅಂಕೆ_Globals;
    M3Global*             globals;

#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    u32    ಅಂಕೆ_ಟ್ಯಾಗುಗಳು;
    M3_ಟ್ಯಾಗ್* ಟ್ಯಾಗುಗಳು;
#ಆದರೆ_ಅಂತ್ಯ

    u32               ಅಂಕೆ_Element_Segments;
    M3ElementSegment* elementSegments;
    ಬೈಟುಗಳು_t           element_Section_ಕೊನೆ;

    // The module's table index space, imported entries first. Entries are
    // borrowed pointers, individually allocated so a slot can be pointed at a
    // table another module owns - see M3Table.owner, and M3Module.memories,
    // which works the same way.
    IM3_ಕೋಷ್ಟಕ*         tables;
    u32               ಅಂಕೆ_Tables;

    // The module's memory index space: imported entries first, then declared
    // ones, exactly as the Wasm index space orders them. Entries are borrowed
    // pointers - see M3Memory.owner.
    IM3_ಸ್ಮರಣೆ*        memories;
    u32               ಅಂಕೆ_Memories;

    // The interpreter always carries a valid memory header in _mem - the
    // stack-limit check, the backtrace recorder and the call ops all reach the
    // runtime through it. A module that declares no memory still needs one, so
    // it gets this zero-length stand-in rather than a NULL _mem.
    M3_ಸ್ಮರಣೆ          ಖಾಲಿಗಳು_ಸ್ಮರಣೆ;

    // memories[0], or emptyMemory when there are none. Resolved once by
    // InitMemory - the index space is fixed by then and linking has already
    // repointed whatever it was going to - so the call ops can read it straight
    // instead of branching. NULL until the module is loaded.
    IM3_ಸ್ಮರಣೆ         memory0;

    M3Export*         exports;
    u32               ಅಂಕೆ_Exports;

#ಆದರೆ d_m3HasSnapshots
    ರಚನೆ M3EmbeddedSnapshot* snapshots;

    // A snapshot restores into a module none of whose code has run - start
    // function included - and a restore that fails part way leaves one that
    // can no longer run at all: its store is neither the snapshot's nor its own.
    bool                       has_ಸತತ;
    bool                       isUnusable;
#ಆದರೆ_ಅಂತ್ಯ

    ರಚನೆ M3Module* ಮುಂದಿನ;
} M3Module;

M3_ಫಲಿತಾಂಶ Module_ಕೂಡಿಸು_ಸ್ಮರಣೆ (IM3Module io_module, IM3_ಸ್ಮರಣೆ* o_ಸ್ಮರಣೆ, ಸ್ಥಿರಾಂಕ M3_ಸ್ಮರಣೆ_Info* i_info, bool i_isImported);

// Memory 0 of a module - the one the interpreter's _mem register tracks while
// that module's code is running. Never NULL once the module has been loaded.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
IM3_ಸ್ಮರಣೆ Module_Memory0 (IM3Module i_module)
{
    ಹಿಂತಿರುಗಿ i_module->memory0;
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
M3_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ* Module_ಸ್ಮರಣೆ_ಶೀರ್ಷಿಕೆ (IM3Module i_module)
{
    ಹಿಂತಿರುಗಿ i_module->memory0->mallocated;
}

M3_ಫಲಿತಾಂಶ Module_ಕೂಡಿಸು_Global (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported);
M3_ಫಲಿತಾಂಶ Module_ಕೂಡಿಸು_ಕೋಷ್ಟಕ (IM3Module io_module, IM3_ಕೋಷ್ಟಕ* o_ಕೋಷ್ಟಕ, ಸ್ಥಿರಾಂಕ M3_ಕೋಷ್ಟಕ_Info* i_info, bool i_isImported);
#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
M3_ಫಲಿತಾಂಶ Module_ಕೂಡಿಸು_ಟ್ಯಾಗ್ (IM3Module io_module, IM3_ಟ್ಯಾಗ್* o_ಟ್ಯಾಗ್, IM3FuncType i_type, bool i_isImported);
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
IM3FuncType Module_Cont_Type_ದ_Ref (IM3Module i_module, m3type_t i_refType);
#ಆದರೆ_ಅಂತ್ಯ
M3_ಫಲಿತಾಂಶ    Module_DeclareFunction (IM3Module io_module, u32 i_ಸೂಚ್ಯಂಕ);
bool        Module_IsFunctionDeclared (IM3Module i_module, u32 i_ಸೂಚ್ಯಂಕ);

M3_ಫಲಿತಾಂಶ    Module_PreallocFunctions (IM3Module io_module, u32 i_ಒಟ್ಟು_Functions);
M3_ಫಲಿತಾಂಶ    Module_ಕೂಡಿಸು_Function (IM3Module io_module, u32 i_type_ಸೂಚ್ಯಂಕ, IM3ImportInfo i_importInfo /* can be null */);
IM3Function Module_GetFunction (IM3Module i_module, u32 i_function_ಸೂಚ್ಯಂಕ);
bool        Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule);

ಶೂನ್ಯ        Module_Generate_ಹೆಸರುಗಳು (IM3Module i_module);

ಶೂನ್ಯ        FreeImportInfo (M3ImportInfo* i_info);

//---------------------------------------------------------------------------------------------------------------------------------

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Environment {
    //    struct M3Runtime *      runtimes;

    IM3FuncType      funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType      retFuncTypes[c_m3_Type_ಎಣಿಕೆ];      // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    u16              ಅಂಕೆ_Func_Types;                       // hands out M3FuncType.canonicalIndex
    M3_ಸಂಕೇತ_ಪುಟ*      ಪುಟಗಳು_Released;

    M3SectionHandler customSectionHandler;
} M3Environment;

ಶೂನ್ಯ     Environment_Release (IM3Environment i_environment);

// takes ownership of io_funcType and returns a pointer to the persistent version (could be same or different)
M3_ಫಲಿತಾಂಶ Environment_ಕೂಡಿಸು_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType);
M3_ಫಲಿತಾಂಶ Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ಎಣಿಕೆ, u16* o_ಮೊದಲ_ಸೂಚ್ಯಂಕ);
ಶೂನ್ಯ     Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_ಸೂಚ್ಯಂಕ);

#ಆದರೆ d_m3HasTypedRefs
M3_ಫಲಿತಾಂಶ Parse_ರಾಶಿ_Type (IM3Module i_module, m3type_t* o_ರಾಶಿ_ಬಿಟ್ಟುಗಳು, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);
#ಆದರೆ_ಅಂತ್ಯ
M3_ಫಲಿತಾಂಶ Parse_ಮೌಲ್ಯ_Type (IM3Module i_module, m3type_t* o_type, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ);

//---------------------------------------------------------------------------------------------------------------------------------


ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Runtime {
    M3Compilation  compilation;

    IM3Environment environment;

    M3_ಸಂಕೇತ_ಪುಟ*    ಪುಟಗಳು_Open;      // linked list of code pages with writable space on them
    M3_ಸಂಕೇತ_ಪುಟ*    ಪುಟಗಳು_ತುಂಬಿದ;      // linked list of at-capacity pages

    u32            ಅಂಕೆ_ಸಂಕೇತ_ಪುಟಗಳು;
    u32            ಅಂಕೆ_ಸಕ್ರಿಯ_ಸಂಕೇತ_ಪುಟಗಳು;

    IM3Module      modules;        // linked list of imported modules

    ಶೂನ್ಯ*          ರಾಶಿ;
    ಶೂನ್ಯ*          origin_ರಾಶಿ;
    u32            ರಾಶಿ_ಗಾತ್ರ;
    u32            ಅಂಕೆ_ರಾಶಿ_Slots;
    ಶೂನ್ಯ*          ರಾಶಿ_ಎಲ್ಲೆ;     // native C-stack low-water mark; Wasm calls trap past it (NULL = unset)
    IM3Function    ಕೊನೆಯ_Called;     // last function that successfully executed
    IM3Function    entered;        // entry point of the outermost call in flight, or of the last one, trapped or not
    u32            callNesting;    // RunCodeChecked() recursion depth

    ಶೂನ್ಯ*          userdata;

    u32            ಸ್ಮರಣೆ_ಎಲ್ಲೆ;
    u64            ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ;
    u64            ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಬಳಸಿದ;
    u64            ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ;
    u64            ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ;

#ಆದರೆ d_m3DeterministicProfile
    // What the host answers about time and entropy out of, rather than out of the
    // machine it happens to be running on - see m3_deterministic.h, which is what
    // reads and moves them. Both are per-runtime, so two runtimes replay
    // independently and a fresh runtime replays the same way as the last one.
    u64 virtual_ಸಮಯ_Ns;
    u64 randomState;
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3EnableValidation
    bool    skipValidation; // m3_SetValidation: compile function bodies without type checking them first
    ValCtx* validator;      // created on first use
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3EnableStrace >= 2
    u32 call_ಆಳ;
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasGasMetering
    // Both counts are in gas units (see M3_GAS_UNITS_PER_GAS). gasRemaining goes
    // negative by the cost of the segment that ran out, which is what makes the
    // gas used come out slightly over the limit, and is deliberate: a segment
    // is paid for in full before any of it runs.
    i64 gas_ಎಲ್ಲೆ;
    i64 gas_ಉಳಿದ;
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasExceptionHandling
    u32          try_ಆಳ;           // number of try regions whose body is executing
    M3Exception* pendingException;   // the exception currently unwinding, if any
    M3Exception* exceptions;         // the ones it still holds
    M3Exception* uncaughtException;  // a copy of what ended the last call, off the list
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    M3_Host_ಟ್ಯಾಗ್* host_ಟ್ಯಾಗುಗಳು;
#ಆದರೆ_ಅಂತ್ಯ

    // Set by the embedder and by the host's interrupt handler, so they are here
    // whether or not the interpreter was built able to act on them. Only the
    // suspendable operations read suspendRequested, and those exist only with
    // stack switching, so without it m3_RequestSuspend is simply never answered.
    bool          isSuspendable;
    ಅಸ್ಥಿರ bool suspendRequested;

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    // Set while m3_ResumeRuntime replays a paused invocation: a back edge or a
    // function entry it paused at goes on past its check, so a pause already
    // requested takes effect at the next one instead of on the spot again
    bool            resumePastCheck;

    u32             continuations_ಎಲ್ಲೆ;
    u32             continuationsAllocated;
    m3slot_t**      val_ರಾಶಿ_Pool;
    u32             val_ರಾಶಿ_Pool_Cap;
    u32             val_ರಾಶಿ_Pool_ಎಣಿಕೆ;
    IM3Continuation ಸಕ್ರಿಯ_Continuation;
    IM3Continuation suspendedContinuation;
    IM3Continuation continuations;
    IM3Continuation ಬೇರು_Continuation;

    // The suspend on its way out, if there is one. Only ever one at a time -
    // it travels up the native stack and is answered before anything else can
    // start - so this belongs to the runtime rather than to any continuation.
    IM3_ಟ್ಯಾಗ್          suspend_ಟ್ಯಾಗ್;
    IM3Continuation suspendHandlerCont;     // whose resume named the tag
    pc_t            suspendStubPC;          // the handler it is headed for
    IM3Continuation switch_ಗುರಿ;           // the peer to run instead, for a switch
    u64             suspendPayload[d_m3MaxContinuationPayload];
    u32             suspendPayloadIs64[d_m3MaxContinuationPayload];
    u32             ಅಂಕೆ_Suspend_Payload;
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3HasSnapshots
    M3SnapshotHooks snapshotHooks;
#ಆದರೆ_ಅಂತ್ಯ

    M3ErrorInfo error;
#ಆದರೆ d_m3VerboseErrorMessages
    ಅಕ್ಷರ error_ಸಂದೇಶ[256]; // the actual buffer. M3ErrorInfo can point to this
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_ದಾಖಲೆ_Backtraces
    M3BacktraceInfo backtrace;
#ಆದರೆ_ಅಂತ್ಯ

    u32 new_ಸಂಕೇತ_ಪುಟ_Sequence;
} M3Runtime;

// Establish the native C-stack limit for a top-level invocation. The outermost
// call records a low-water mark at most d_m3MaxNativeStack bytes into the stack -
// less, where the thread's stack was measured and is smaller than that. Nested
// re-entrant calls (e.g. an imported function calling back into Wasm) keep the
// original mark. op_Call/op_CallIndirect trap once execution crosses it.
//
// The stack pointer is read here rather than inside m3_NativeStackLimit so that
// the mark measures the caller's frame, the same one d_m3CheckNativeStack tests
// against later.
#ಆದರೆ d_m3_Max_Native_ರಾಶಿ > 0
#  define d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Enter(RT)                                               \
        ಶೂನ್ಯ * m3_Saved_ರಾಶಿ_ಎಲ್ಲೆ = (RT)->ರಾಶಿ_ಎಲ್ಲೆ;                           \
        ಆದರೆ (not (RT)->ರಾಶಿ_ಎಲ್ಲೆ)                                               \
            (RT)->ರಾಶಿ_ಎಲ್ಲೆ = m3_Native_ರಾಶಿ_ಎಲ್ಲೆ(m3_Native_ರಾಶಿ_Ptr(), (d_m3_Max_Native_ರಾಶಿ));
#  define d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Leave(RT)  (RT)->ರಾಶಿ_ಎಲ್ಲೆ = m3_Saved_ರಾಶಿ_ಎಲ್ಲೆ;
#ಇಲ್ಲದಿದ್ದರೆ
#  define d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Enter(RT)
#  define d_m3_ರಾಶಿ_ಎಲ್ಲೆ_Leave(RT)
#ಆದರೆ_ಅಂತ್ಯ

ಶೂನ್ಯ     InitRuntime (IM3Runtime io_runtime, u32 i_ರಾಶಿ_ಗಾತ್ರ_ಒಳ_ಬೈಟುಗಳು);
ಶೂನ್ಯ     Runtime_Release (IM3Runtime io_runtime);

M3_ಫಲಿತಾಂಶ Resize_ಸ್ಮರಣೆ (IM3Runtime io_runtime, IM3_ಸ್ಮರಣೆ io_ಸ್ಮರಣೆ, u64 i_ಅಂಕೆ_ಪುಟಗಳು);

// Give back whatever is behind io_memory->mallocated, which is the heap or a slot of
// the guarded arena depending on the build. Leaves the M3Memory itself alone.
ಶೂನ್ಯ     Free_ಸ್ಮರಣೆ_ಖಂಡ (IM3_ಸ್ಮರಣೆ io_ಸ್ಮರಣೆ);

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ಶೂನ್ಯ* (*ModuleVisitor)(IM3Module i_module, ಶೂನ್ಯ* i_info);
ಶೂನ್ಯ*       ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ಶೂನ್ಯ* i_info);

ಶೂನ್ಯ*       v_ಕಂಡುಹಿಡಿ_Function (IM3Module i_module, ಶೂನ್ಯ* i_info);

IM3_ಸಂಕೇತ_ಪುಟ Acquire_ಸಂಕೇತ_ಪುಟ (IM3Runtime io_runtime);
IM3_ಸಂಕೇತ_ಪುಟ Acquire_ಸಂಕೇತ_ಪುಟ_With_ಸಾಮರ್ಥ್ಯ (IM3Runtime io_runtime, u32 i_ಸಾಲು_ಎಣಿಕೆ);
ಶೂನ್ಯ        Release_ಸಂಕೇತ_ಪುಟ (IM3Runtime io_runtime, IM3_ಸಂಕೇತ_ಪುಟ i_ಸಂಕೇತ_ಪುಟ);

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Continuation_Can_Acquire_ರಾಶಿ (IM3Runtime runtime)
{
    ಹಿಂತಿರುಗಿ not runtime->continuations_ಎಲ್ಲೆ or
           runtime->continuationsAllocated < runtime->continuations_ಎಲ್ಲೆ;
}
#ಆದರೆ_ಅಂತ್ಯ

d_m3_ಕೊನೆ_Extern_C

#ಆದರೆ_ಅಂತ್ಯ // m3_env_h
