//
//  m3_env.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_env_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_env_h

#ਅਨ੍ਤਰ੍ਭਾਵ "wasm3.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_code.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"

d_m3BeginExternC

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ ValCtx ValCtx;   // the validator's scratch, defined in m3_validate.c


//---------------------------------------------------------------------------------------------------------------------------------

// Page counts are u64 because a memory64 type may declare up to 2^48 pages.
// Nothing near that can be backed, but the module still has to parse and
// validate; it is instantiation that fails.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਸਮ੍ਰਿਤੀ_Info {
    u64  init_ਪੰਨੇ;
    u64  max_ਪੰਨੇ;
    u32  ਪੰਨਾ_ਮਾਪ;
    bool hasMax;         // a declared maximum of 0 is not the same as none
    bool isMemory64;     // addressed by i64 rather than i32
    bool isShared;       // the threads proposal: a maximum is required
} M3_ਸਮ੍ਰਿਤੀ_Info;


// One linear memory. Memories belong to the module that declares them, not to
// the runtime: two modules loaded into the same runtime each get their own, and
// the interpreter reaches the one it is currently running against through the
// _mem pseudo-register (see M3MemoryHeader).
//
// Every entry is individually allocated so that a slot may later be pointed at
// a memory another module owns - growing one has to be visible through every
// name it has. 'owner' says which module allocated it, and so which module
// frees it.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਸਮ੍ਰਿਤੀ {
    M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ*  mallocated;

    u64              ਅੰਕ_ਪੰਨੇ;
    u64              max_ਪੰਨੇ;
    u64              init_ਪੰਨੇ;
    u32              ਪੰਨਾ_ਮਾਪ;
    bool             hasMax;         // see M3MemoryInfo
    bool             isMemory64;     // addressed by i64 rather than i32
    bool             isShared;       // see M3MemoryInfo

    ਬਣਤਰ M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;

#ਜੇ d_m3_Guarded_ਸਮ੍ਰਿਤੀ
    // The slot of the guarded arena this memory's bytes live in, and so what
    // 'mallocated' points into rather than the heap - see Guard_TakeSlot
    ਖਾਲੀ* guard_ਥਾਂ;
#ਜੇ_ਅੰਤ
} M3_ਸਮ੍ਰਿਤੀ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਸਮ੍ਰਿਤੀ* IM3_ਸਮ੍ਰਿਤੀ;

// The value type a memory's addresses, page counts and lengths are expressed
// in. Every instruction naming the memory takes and returns that type.
ਸਥਿਰ ਸਤਰਅੰਦਰ
m3type_t ਸਮ੍ਰਿਤੀ_Addr_Type (ਅਚਲ M3_ਸਮ੍ਰਿਤੀ* i_ਸਮ੍ਰਿਤੀ)
{
    ਵਾਪਸ i_ਸਮ੍ਰਿਤੀ->isMemory64 ? c_m3Type_i64 : c_m3Type_i32;
}


// A module that declares no custom page size means the default one. The parser
// leaves that as zero, and InitMemory only fills it in when it allocates, so a
// memory has to be asked rather than read directly until then.
ਸਥਿਰ ਸਤਰਅੰਦਰ
u32 ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ (ਅਚਲ M3_ਸਮ੍ਰਿਤੀ* i_ਸਮ੍ਰਿਤੀ)
{
    ਵਾਪਸ i_ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ ? i_ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ : d_m3_Default_Mem_ਪੰਨਾ_ਮਾਪ;
}


//---------------------------------------------------------------------------------------------------------------------------------

// A table type as the module declared it. Sizes stay u32 - a table64 may name
// far more entries than that, but d_m3MaxSaneTableSize refuses anything near
// the limit long before it matters.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਸਾਰਣੀ_Info {
    m3type_t elemType;
    u32      init_ਮਾਪ;
    u32      max_ਮਾਪ;
    bool     hasMax;         // a declared maximum of 0 is not the same as none
    bool     isTable64;      // indexed by i64 rather than i32
} M3_ਸਾਰਣੀ_Info;


// A table's elements are opaque pointer-sized references: IM3Function for a
// funcref table, a host handle for an externref one. NULL is the null reference.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਸਾਰਣੀ {
    ਖਾਲੀ**           elements;
    u32              ਮਾਪ;
    u32              max_ਮਾਪ;            // 0 when the module declared no maximum
    m3type_t         type;

    // Every slot starts out holding this, rather than null. A table whose
    // element type is not nullable has to say what to fill itself with.
    ਬਾਈਟਾਂ_t          init_ਵਿਅੰਜਕ;
    u32              init_ਵਿਅੰਜਕ_ਮਾਪ;

    u32              init_ਮਾਪ;       // the declared minimum
    bool             hasMax;         // a declared maximum of 0 is not the same as none
    bool             isTable64;      // indexed by i64 rather than i32

    ਬਣਤਰ M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;
} M3_ਸਾਰਣੀ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਸਾਰਣੀ* IM3_ਸਾਰਣੀ;

// The value type a table's indexes and sizes are expressed in - table64 is the
// same choice as memory64, made per table.
ਸਥਿਰ ਸਤਰਅੰਦਰ
m3type_t ਸਾਰਣੀ_Addr_Type (ਅਚਲ M3_ਸਾਰਣੀ* i_ਸਾਰਣੀ)
{
    ਵਾਪਸ i_ਸਾਰਣੀ->isTable64 ? c_m3Type_i64 : c_m3Type_i32;
}

// Element segment modes, from the low two bits of the segment's flags
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਗਿਣਤੀ {
    c_m3_Elem_ਸਰਗਰਮ      = 0,        // table 0
    c_m3Elem_passive     = 1,
    c_m3_Elem_ਸਰਗਰਮ_Idx   = 2,        // explicit table index
    c_m3Elem_declarative = 3
} M3_Element_ਬਹੁਲਕ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3ElementSegment {
    ਬਾਈਟਾਂ_t  init_ਵਿਅੰਜਕ;       // active segments only: the offset
    ਬਾਈਟਾਂ_t  elements;       // funcidx list, or const exprs when isExpr
    ਖਾਲੀ**   resolved;       // passive segments only, for table.init

    u32      init_ਵਿਅੰਜਕ_ਮਾਪ;
    u32      ਅੰਕ_Elements;
    u32      ਸਾਰਣੀ_ਸੂਚਕਾਂਕ;

    m3type_t type;
    u8       ਬਹੁਲਕ;
    bool     is_ਵਿਅੰਜਕ;
    bool     dropped;
} M3ElementSegment;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਅੰਕੜੇ_Segment {
    ਅਚਲ u8* init_ਵਿਅੰਜਕ;           // wasm code
    ਅਚਲ u8* ਅੰਕੜੇ;

    u32       init_ਵਿਅੰਜਕ_ਮਾਪ;
    u32       ਸਮ੍ਰਿਤੀ_Region;
    u32       ਮਾਪ;

    bool      isPassive;
    bool      dropped;            // active segments are dropped once instantiated
} M3_ਅੰਕੜੇ_Segment;

//---------------------------------------------------------------------------------------------------------------------------------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Global {
    M3ImportInfo     import;

    // An import linked to another module's export: the global that actually
    // holds the value. Reads and writes go there, so a mutable global stays one
    // cell seen under two names. NULL for anything else, including an import a
    // host supplied a value for through m3_LinkGlobal.
    ਬਣਤਰ M3Global* resolved;

    ਸੰਘ {
        i32 i32_ਮੁੱਲ;
        i64 i64_ਮੁੱਲ;
#ਜੇ d_m3HasFloat
        f64 f64_ਮੁੱਲ;
        f32 f32_ਮੁੱਲ;
#ਜੇ_ਅੰਤ
        ਖਾਲੀ* ref_ਮੁੱਲ;
    };

    ਬਾਈਟਾਂ_t  init_ਵਿਅੰਜਕ;       // wasm code
    u32      init_ਵਿਅੰਜਕ_ਮਾਪ;
    m3type_t type;
    bool     imported;
    bool     isMutable;
} M3Global;


//---------------------------------------------------------------------------------------------------------------------------------

#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching

// An exception / control tag, as the tag section declares it. Tags are compared by
// identity, and this struct's address is that identity.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਟੈਗ {
    M3ImportInfo  import;
    IM3FuncType   type;           // params are the payload; results are empty for EH, can be non-empty for stack switching
    ਬਣਤਰ M3_ਟੈਗ* resolved;
    bool          imported;
} M3_ਟੈਗ;

// A tag m3_NewTag made. It belongs to the runtime rather than to a module, and
// is linked in front of the runtime's others rather than kept in an array, which
// would move the tags - and with them their identities - whenever it grew.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_Host_ਟੈਗ {
    ਬਣਤਰ M3_Host_ਟੈਗ* ਅਗਲਾ;
    M3_ਟੈਗ             ਟੈਗ;
} M3_Host_ਟੈਗ;

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ਜੇ d_m3HasExceptionHandling

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
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Exception {
    ਬਣਤਰ M3Exception* ਅਗਲਾ;           // the runtime's allocation list,
    ਬਣਤਰ M3Exception* prev;           //   doubly linked so one can leave early
    IM3_ਟੈਗ              ਟੈਗ;
    u32                 ਅੰਕ_ਦਲੀਲਾਂ;
    bool                reified;        // an exnref to this has been handed out
    u64                 ਦਲੀਲਾਂ[];
} M3Exception;


M3Exception* NewException (IM3Runtime io_runtime, IM3_ਟੈਗ i_ਟੈਗ, u32 i_ਅੰਕ_ਦਲੀਲਾਂ);
ਖਾਲੀ         FreeException (IM3Runtime io_runtime, M3Exception* i_exception);
ਖਾਲੀ         FreeExceptions (IM3Runtime io_runtime);

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling


#ਜੇ d_m3_Has_ਢੇਰ_Switching

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
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਗਿਣਤੀ M3_ਚੌਖਟਾ_ਕਿਸਮ {
    ਚੌਖਟਾ_call,          // op_Call / op_CallRef / op_CallIndirect
    ਚੌਖਟਾ_loop,          // op_Loop
#  ਜੇ d_m3HasExceptionHandling
    ਚੌਖਟਾ_try,           // op_TryTable
#  endif
#  ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
    ਚੌਖਟਾ_ਇੰਦਰਾਜ,         // op_Entry, in builds where it keeps a frame
#  endif
    ਚੌਖਟਾ_resume,        // op_Resume, when the suspend is not its to answer
} M3_ਚੌਖਟਾ_ਕਿਸਮ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਚੌਖਟਾ {
    u8        ਕਿਸਮ;      // M3FrameKind
    pc_t      pc;        // call: the return address | loop: the loop id, which is
                         // also its body | try: the clause table | entry: unused
    m3stack_t sp;
    IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ;    // _mem is re-derived from this rather than stored:
                         // growing linear memory moves the header, so only the
                         // M3Memory stays valid across a suspend

    ਸੰਘ {
        // op_Call resumes the caller with the registers its own native frame was
        // holding, so those belong to the frame, not to the suspension point
        ਬਣਤਰ {
            IM3Function function;
            m3reg_t     r0;
#  ਜੇ d_m3HasFloat
            f64 fp0;
#  endif
        } call;
#  ਜੇ d_m3HasExceptionHandling
        ਬਣਤਰ {
            u32  ਅੰਕ_Clauses;
            bool handlers_ਜੀਵਤ;   // false once op_PopHandlers has retired the region
        } try_;
#  endif
#  ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
        ਬਣਤਰ {
            IM3Function function;
        } ਇੰਦਰਾਜ;
#  endif
        // A resume that the suspend travelled straight through, because the
        // tag was not one its handlers named. The continuation it was running
        // is part of what got captured, so everything needed to set it going
        // again under the same handlers is kept here.
        ਬਣਤਰ {
            ਬਣਤਰ M3Continuation* cont;
            pc_t                   handlersPC;
            u32                    ਅੰਕ_Handlers;
            pc_t                   resultsPC;
            u32                    ਅੰਕ_Results;
        } resume;
    };
} M3_ਚੌਖਟਾ;

// Which of the three resume forms an op_Resume was emitted for
ਗਿਣਤੀ {
    d_m3_Resume_ਸਾਧਾਰਨ = 0,
    d_m3ResumeThrow,
    d_m3ResumeThrowRef,
};

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਗਿਣਤੀ M3ContinuationState {
    cont_allocated = 0,
    cont_running,
    cont_suspended,
    cont_returned,
    cont_consumed
} M3ContinuationState;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3ResumeHandler {
    u8     ਕਿਸਮ;        // 0: (on $t $h), 1: (on $t switch)
    IM3_ਟੈਗ ਟੈਗ;
    pc_t   stubPC;
} M3ResumeHandler;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Continuation {
    ਬਣਤਰ M3Continuation* ਅਗਲਾ;           // linked list in runtime for cleanup
    IM3FuncType            type;            // continuation type (cont $ft)
    IM3Function            ਇੰਦਰਾਜ_Function;   // target function
    M3ContinuationState    state;

    m3slot_t*              val_ਢੇਰ;        // allocated value stack buffer
    u32                    ਅੰਕ_ਢੇਰ_Slots;
    m3stack_t              sp;              // current value stack pointer

    M3_ਚੌਖਟਾ*               ਚੌਖਟੇ;          // native frames recorded by the last suspend
    u32                    ਚੌਖਟੇ_Cap;       // entries allocated
    u32                    ਅੰਕ_ਚੌਖਟੇ;       // entries in use, innermost first

    pc_t                   pc;              // resume instruction pointer
    u8                     suspend_ਬਿੰਦੂ;    // M3SafePointKind: what left pc where it is
    m3reg_t                r0;              // saved accumulator
#  ਜੇ d_m3HasFloat
    f64 fp0;
#  endif

    // Bound args count (for cont.bind)
    u32 bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ;

#  ਜੇ d_m3HasExceptionHandling
    // resume_throw: raised at the suspension point instead of carrying on from
    // it, so the try regions inside the continuation get their turn first
    M3Exception* resumeThrow;
#  endif

    // Where the values go when this continuation is next given some: the slots
    // its own suspend or switch is waiting on.
    u32                    ਅੰਕ_Suspend_Results;
    i32                    suspend_ਨਤੀਜਾ_Offsets[d_m3MaxContinuationPayload];
    u32                    suspend_ਨਤੀਜਾ_Is64[d_m3MaxContinuationPayload];

    // Active handler table from enclosing resume:
    u32                    ਅੰਕ_Handlers;
    pc_t                   handlersPC;
    ਬਣਤਰ M3Continuation* ਮਾਪਾ;         // resumer continuation
} M3Continuation, *IM3Continuation;

M3_ਨਤੀਜਾ        Continuation_Acquire_ਢੇਰ (IM3Runtime runtime, IM3Continuation cont);
ਖਾਲੀ            Continuation_Recycle_ਢੇਰ (IM3Runtime runtime, IM3Continuation cont);
IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function);
IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont);
ਖਾਲੀ            Continuation_ReleaseAll (IM3Runtime io_runtime);

// Appends one frame to the suspending continuation. Returns the marker the
// caller should keep unwinding with: the suspend marker when the frame was
// recorded, or a trap when it could not be - a continuation that cannot record
// its frames cannot be resumed, so the suspend has to become a trap instead.
m3ret_t         Continuation_ਰਿਕਾਰਡ_ਚੌਖਟਾ (IM3Runtime i_runtime, ਅਚਲ M3_ਚੌਖਟਾ* i_ਚੌਖਟਾ);

// Points runtime->stack, where the host's next call puts its arguments, above
// whatever a suspended root continuation holds - or back at the bottom when
// there is none.
ਖਾਲੀ            Runtime_ਰੱਖੋ_Call_ਢੇਰ (IM3Runtime io_runtime);

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


#ਜੇ d_m3HasSnapshots
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3EmbeddedSnapshot {
    cstr_t                     name;
    ਬਾਈਟਾਂ_t                    ਅੰਕੜੇ;
    u32                        ਮਾਪ;
    ਬਣਤਰ M3EmbeddedSnapshot* ਅਗਲਾ;
} M3EmbeddedSnapshot;
#ਜੇ_ਅੰਤ


//---------------------------------------------------------------------------------------------------------------------------------

// One entry of the export section. Export names belong to the module rather than
// to what they name: one entity can be exported under any number of names, and an
// import re-exported under a name of the module's own, whose slot linking points
// at another module's entity. Every lookup by name goes through these, and
// resolves the index against the module's index space at the time it is asked.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Export {
    cstr_t name;
    u32    name_ਲੰਮਾਈ;   // the name may hold a NUL, which ends the C string early
    u32    index;        // into the index space 'kind' names
    u8     ਕਿਸਮ;         // d_externalKind_*
} M3Export;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Module {
    ਬਣਤਰ M3Runtime*     runtime;
    ਬਣਤਰ M3Environment* environment;

    ਬਾਈਟਾਂ_t               wasm_ਸ਼ੁਰੂ;
    ਬਾਈਟਾਂ_t               wasm_ਸਿਰਾ;

    cstr_t                name;

    u32                   ਅੰਕ_Func_Types;
    IM3FuncType*          funcTypes;              // array of pointers to list of FuncTypes

    u32                   ਅੰਕ_Func_Imports;
    u32                   ਅੰਕ_Functions;
    u32                   allFunctions;           // allocated functions count
    M3Function*           functions;

    i32                   ਸ਼ੁਰੂ_Function;

    u32                   ਅੰਕ_ਅੰਕੜੇ_Segments;
    M3_ਅੰਕੜੇ_Segment*        ਅੰਕੜੇ_Segments;

    u32                   ਅੰਕੜੇ_ਗਿਣਤੀ;          // from the data count section
    bool                  has_ਅੰਕੜੇ_ਗਿਣਤੀ;

    // Bitset of the functions ref.func may name: those exported, or referenced
    // by an element segment or a global initializer. One bit per function.
    u8*                   declaredFuncs;

    //u32                     importedGlobals;
    u32                   ਅੰਕ_Globals;
    M3Global*             globals;

#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
    u32    ਅੰਕ_ਟੈਗ_ਸੂਚੀ;
    M3_ਟੈਗ* ਟੈਗ_ਸੂਚੀ;
#ਜੇ_ਅੰਤ

    u32               ਅੰਕ_Element_Segments;
    M3ElementSegment* elementSegments;
    ਬਾਈਟਾਂ_t           element_Section_ਸਿਰਾ;

    // The module's table index space, imported entries first. Entries are
    // borrowed pointers, individually allocated so a slot can be pointed at a
    // table another module owns - see M3Table.owner, and M3Module.memories,
    // which works the same way.
    IM3_ਸਾਰਣੀ*         tables;
    u32               ਅੰਕ_Tables;

    // The module's memory index space: imported entries first, then declared
    // ones, exactly as the Wasm index space orders them. Entries are borrowed
    // pointers - see M3Memory.owner.
    IM3_ਸਮ੍ਰਿਤੀ*        memories;
    u32               ਅੰਕ_Memories;

    // The interpreter always carries a valid memory header in _mem - the
    // stack-limit check, the backtrace recorder and the call ops all reach the
    // runtime through it. A module that declares no memory still needs one, so
    // it gets this zero-length stand-in rather than a NULL _mem.
    M3_ਸਮ੍ਰਿਤੀ          ਖਾਲੀ_ਥਾਵਾਂ_ਸਮ੍ਰਿਤੀ;

    // memories[0], or emptyMemory when there are none. Resolved once by
    // InitMemory - the index space is fixed by then and linking has already
    // repointed whatever it was going to - so the call ops can read it straight
    // instead of branching. NULL until the module is loaded.
    IM3_ਸਮ੍ਰਿਤੀ         memory0;

    M3Export*         exports;
    u32               ਅੰਕ_Exports;

#ਜੇ d_m3HasSnapshots
    ਬਣਤਰ M3EmbeddedSnapshot* snapshots;

    // A snapshot restores into a module none of whose code has run - start
    // function included - and a restore that fails part way leaves one that
    // can no longer run at all: its store is neither the snapshot's nor its own.
    bool                       has_ਲੜੀਵਾਰ;
    bool                       isUnusable;
#ਜੇ_ਅੰਤ

    ਬਣਤਰ M3Module* ਅਗਲਾ;
} M3Module;

M3_ਨਤੀਜਾ Module_ਜੋੜੋ_ਸਮ੍ਰਿਤੀ (IM3Module io_module, IM3_ਸਮ੍ਰਿਤੀ* o_ਸਮ੍ਰਿਤੀ, ਅਚਲ M3_ਸਮ੍ਰਿਤੀ_Info* i_info, bool i_isImported);

// Memory 0 of a module - the one the interpreter's _mem register tracks while
// that module's code is running. Never NULL once the module has been loaded.
ਸਥਿਰ ਸਤਰਅੰਦਰ
IM3_ਸਮ੍ਰਿਤੀ Module_Memory0 (IM3Module i_module)
{
    ਵਾਪਸ i_module->memory0;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
M3_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ* Module_ਸਮ੍ਰਿਤੀ_ਸਿਰਲੇਖ (IM3Module i_module)
{
    ਵਾਪਸ i_module->memory0->mallocated;
}

M3_ਨਤੀਜਾ Module_ਜੋੜੋ_Global (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported);
M3_ਨਤੀਜਾ Module_ਜੋੜੋ_ਸਾਰਣੀ (IM3Module io_module, IM3_ਸਾਰਣੀ* o_ਸਾਰਣੀ, ਅਚਲ M3_ਸਾਰਣੀ_Info* i_info, bool i_isImported);
#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
M3_ਨਤੀਜਾ Module_ਜੋੜੋ_ਟੈਗ (IM3Module io_module, IM3_ਟੈਗ* o_ਟੈਗ, IM3FuncType i_type, bool i_isImported);
#ਜੇ_ਅੰਤ
#ਜੇ d_m3_Has_ਢੇਰ_Switching
IM3FuncType Module_Cont_Type_ਦਾ_Ref (IM3Module i_module, m3type_t i_refType);
#ਜੇ_ਅੰਤ
M3_ਨਤੀਜਾ    Module_DeclareFunction (IM3Module io_module, u32 i_ਸੂਚਕਾਂਕ);
bool        Module_IsFunctionDeclared (IM3Module i_module, u32 i_ਸੂਚਕਾਂਕ);

M3_ਨਤੀਜਾ    Module_PreallocFunctions (IM3Module io_module, u32 i_ਕੁੱਲ_Functions);
M3_ਨਤੀਜਾ    Module_ਜੋੜੋ_Function (IM3Module io_module, u32 i_type_ਸੂਚਕਾਂਕ, IM3ImportInfo i_importInfo /* can be null */);
IM3Function Module_GetFunction (IM3Module i_module, u32 i_function_ਸੂਚਕਾਂਕ);
bool        Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule);

ਖਾਲੀ        Module_Generate_ਨਾਂ (IM3Module i_module);

ਖਾਲੀ        FreeImportInfo (M3ImportInfo* i_info);

//---------------------------------------------------------------------------------------------------------------------------------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Environment {
    //    struct M3Runtime *      runtimes;

    IM3FuncType      funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType      retFuncTypes[c_m3_Type_ਗਿਣਤੀ];      // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    u16              ਅੰਕ_Func_Types;                       // hands out M3FuncType.canonicalIndex
    M3_ਕੋਡ_ਪੰਨਾ*      ਪੰਨੇ_Released;

    M3SectionHandler customSectionHandler;
} M3Environment;

ਖਾਲੀ     Environment_Release (IM3Environment i_environment);

// takes ownership of io_funcType and returns a pointer to the persistent version (could be same or different)
M3_ਨਤੀਜਾ Environment_ਜੋੜੋ_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType);
M3_ਨਤੀਜਾ Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ਗਿਣਤੀ, u16* o_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ);
ਖਾਲੀ     Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_ਸੂਚਕਾਂਕ);

#ਜੇ d_m3HasTypedRefs
M3_ਨਤੀਜਾ Parse_ਢੇਰ_Type (IM3Module i_module, m3type_t* o_ਢੇਰ_ਬਿੱਟ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);
#ਜੇ_ਅੰਤ
M3_ਨਤੀਜਾ Parse_ਮੁੱਲ_Type (IM3Module i_module, m3type_t* o_type, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ);

//---------------------------------------------------------------------------------------------------------------------------------


ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Runtime {
    M3Compilation  compilation;

    IM3Environment environment;

    M3_ਕੋਡ_ਪੰਨਾ*    ਪੰਨੇ_Open;      // linked list of code pages with writable space on them
    M3_ਕੋਡ_ਪੰਨਾ*    ਪੰਨੇ_ਭਰਿਆ;      // linked list of at-capacity pages

    u32            ਅੰਕ_ਕੋਡ_ਪੰਨੇ;
    u32            ਅੰਕ_ਸਰਗਰਮ_ਕੋਡ_ਪੰਨੇ;

    IM3Module      modules;        // linked list of imported modules

    ਖਾਲੀ*          ਢੇਰ;
    ਖਾਲੀ*          origin_ਢੇਰ;
    u32            ਢੇਰ_ਮਾਪ;
    u32            ਅੰਕ_ਢੇਰ_Slots;
    ਖਾਲੀ*          ਢੇਰ_ਹੱਦਬੰਦੀ;     // native C-stack low-water mark; Wasm calls trap past it (NULL = unset)
    IM3Function    ਆਖਰੀ_Called;     // last function that successfully executed
    IM3Function    entered;        // entry point of the outermost call in flight, or of the last one, trapped or not
    u32            callNesting;    // RunCodeChecked() recursion depth

    ਖਾਲੀ*          userdata;

    u32            ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ;
    u64            ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ;
    u64            ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਵਰਤਿਆ;
    u64            ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ;
    u64            ਸਾਰਣੀ_Elements_ਵਰਤਿਆ;

#ਜੇ d_m3DeterministicProfile
    // What the host answers about time and entropy out of, rather than out of the
    // machine it happens to be running on - see m3_deterministic.h, which is what
    // reads and moves them. Both are per-runtime, so two runtimes replay
    // independently and a fresh runtime replays the same way as the last one.
    u64 virtual_ਸਮਾਂ_Ns;
    u64 randomState;
#ਜੇ_ਅੰਤ

#ਜੇ d_m3EnableValidation
    bool    skipValidation; // m3_SetValidation: compile function bodies without type checking them first
    ValCtx* validator;      // created on first use
#ਜੇ_ਅੰਤ

#ਜੇ d_m3EnableStrace >= 2
    u32 call_ਡੂੰਘਾਈ;
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasGasMetering
    // Both counts are in gas units (see M3_GAS_UNITS_PER_GAS). gasRemaining goes
    // negative by the cost of the segment that ran out, which is what makes the
    // gas used come out slightly over the limit, and is deliberate: a segment
    // is paid for in full before any of it runs.
    i64 gas_ਹੱਦਬੰਦੀ;
    i64 gas_ਬਚਿਆ;
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasExceptionHandling
    u32          try_ਡੂੰਘਾਈ;           // number of try regions whose body is executing
    M3Exception* pendingException;   // the exception currently unwinding, if any
    M3Exception* exceptions;         // the ones it still holds
    M3Exception* uncaughtException;  // a copy of what ended the last call, off the list
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
    M3_Host_ਟੈਗ* host_ਟੈਗ_ਸੂਚੀ;
#ਜੇ_ਅੰਤ

    // Set by the embedder and by the host's interrupt handler, so they are here
    // whether or not the interpreter was built able to act on them. Only the
    // suspendable operations read suspendRequested, and those exist only with
    // stack switching, so without it m3_RequestSuspend is simply never answered.
    bool          isSuspendable;
    ਅਸਥਿਰ bool suspendRequested;

#ਜੇ d_m3_Has_ਢੇਰ_Switching
    // Set while m3_ResumeRuntime replays a paused invocation: a back edge or a
    // function entry it paused at goes on past its check, so a pause already
    // requested takes effect at the next one instead of on the spot again
    bool            resumePastCheck;

    u32             continuations_ਹੱਦਬੰਦੀ;
    u32             continuationsAllocated;
    m3slot_t**      val_ਢੇਰ_Pool;
    u32             val_ਢੇਰ_Pool_Cap;
    u32             val_ਢੇਰ_Pool_ਗਿਣਤੀ;
    IM3Continuation ਸਰਗਰਮ_Continuation;
    IM3Continuation suspendedContinuation;
    IM3Continuation continuations;
    IM3Continuation ਜੜ੍ਹ_Continuation;

    // The suspend on its way out, if there is one. Only ever one at a time -
    // it travels up the native stack and is answered before anything else can
    // start - so this belongs to the runtime rather than to any continuation.
    IM3_ਟੈਗ          suspend_ਟੈਗ;
    IM3Continuation suspendHandlerCont;     // whose resume named the tag
    pc_t            suspendStubPC;          // the handler it is headed for
    IM3Continuation switch_ਨਿਸ਼ਾਨਾ;           // the peer to run instead, for a switch
    u64             suspendPayload[d_m3MaxContinuationPayload];
    u32             suspendPayloadIs64[d_m3MaxContinuationPayload];
    u32             ਅੰਕ_Suspend_Payload;
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasSnapshots
    M3SnapshotHooks snapshotHooks;
#ਜੇ_ਅੰਤ

    M3ErrorInfo error;
#ਜੇ d_m3VerboseErrorMessages
    ਅੱਖਰ error_ਸੁਨੇਹਾ[256]; // the actual buffer. M3ErrorInfo can point to this
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
    M3BacktraceInfo backtrace;
#ਜੇ_ਅੰਤ

    u32 new_ਕੋਡ_ਪੰਨਾ_Sequence;
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
#ਜੇ d_m3_Max_Native_ਢੇਰ > 0
#  define d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Enter(RT)                                               \
        ਖਾਲੀ * m3_Saved_ਢੇਰ_ਹੱਦਬੰਦੀ = (RT)->ਢੇਰ_ਹੱਦਬੰਦੀ;                           \
        ਜੇ (not (RT)->ਢੇਰ_ਹੱਦਬੰਦੀ)                                               \
            (RT)->ਢੇਰ_ਹੱਦਬੰਦੀ = m3_Native_ਢੇਰ_ਹੱਦਬੰਦੀ(m3_Native_ਢੇਰ_Ptr(), (d_m3_Max_Native_ਢੇਰ));
#  define d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Leave(RT)  (RT)->ਢੇਰ_ਹੱਦਬੰਦੀ = m3_Saved_ਢੇਰ_ਹੱਦਬੰਦੀ;
#ਨਹੀਂਤਾਂ
#  define d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Enter(RT)
#  define d_m3_ਢੇਰ_ਹੱਦਬੰਦੀ_Leave(RT)
#ਜੇ_ਅੰਤ

ਖਾਲੀ     InitRuntime (IM3Runtime io_runtime, u32 i_ਢੇਰ_ਮਾਪ_ਵਿੱਚ_ਬਾਈਟਾਂ);
ਖਾਲੀ     Runtime_Release (IM3Runtime io_runtime);

M3_ਨਤੀਜਾ Resize_ਸਮ੍ਰਿਤੀ (IM3Runtime io_runtime, IM3_ਸਮ੍ਰਿਤੀ io_ਸਮ੍ਰਿਤੀ, u64 i_ਅੰਕ_ਪੰਨੇ);

// Give back whatever is behind io_memory->mallocated, which is the heap or a slot of
// the guarded arena depending on the build. Leaves the M3Memory itself alone.
ਖਾਲੀ     Free_ਸਮ੍ਰਿਤੀ_ਖੰਡ (IM3_ਸਮ੍ਰਿਤੀ io_ਸਮ੍ਰਿਤੀ);

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਖਾਲੀ* (*ModuleVisitor)(IM3Module i_module, ਖਾਲੀ* i_info);
ਖਾਲੀ*       ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ਖਾਲੀ* i_info);

ਖਾਲੀ*       v_ਲੱਭੋ_Function (IM3Module i_module, ਖਾਲੀ* i_info);

IM3_ਕੋਡ_ਪੰਨਾ Acquire_ਕੋਡ_ਪੰਨਾ (IM3Runtime io_runtime);
IM3_ਕੋਡ_ਪੰਨਾ Acquire_ਕੋਡ_ਪੰਨਾ_With_ਸਮਰੱਥਾ (IM3Runtime io_runtime, u32 i_ਸਤਰ_ਗਿਣਤੀ);
ਖਾਲੀ        Release_ਕੋਡ_ਪੰਨਾ (IM3Runtime io_runtime, IM3_ਕੋਡ_ਪੰਨਾ i_ਕੋਡ_ਪੰਨਾ);

#ਜੇ d_m3_Has_ਢੇਰ_Switching
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Continuation_Can_Acquire_ਢੇਰ (IM3Runtime runtime)
{
    ਵਾਪਸ not runtime->continuations_ਹੱਦਬੰਦੀ or
           runtime->continuationsAllocated < runtime->continuations_ਹੱਦਬੰਦੀ;
}
#ਜੇ_ਅੰਤ

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_env_h
