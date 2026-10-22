//
//  m3_env.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#నిర్వచితం_కాకపోతే m3_env_h
#నిర్వచించు m3_env_h

#అన్తర్భావ "wasm3.h"
#అన్తర్భావ "m3_code.h"
#అన్తర్భావ "m3_compile.h"

d_m3BeginExternC

రకనిర్వచనం నిర్మాణం ValCtx ValCtx;   // the validator's scratch, defined in m3_validate.c


//---------------------------------------------------------------------------------------------------------------------------------

// Page counts are u64 because a memory64 type may declare up to 2^48 pages.
// Nothing near that can be backed, but the module still has to parse and
// validate; it is instantiation that fails.
రకనిర్వచనం నిర్మాణం M3_జ్ఞాపకం_Info {
    u64  init_పుటలు;
    u64  max_పుటలు;
    u32  పుట_కొలత;
    bool hasMax;         // a declared maximum of 0 is not the same as none
    bool isMemory64;     // addressed by i64 rather than i32
    bool isShared;       // the threads proposal: a maximum is required
} M3_జ్ఞాపకం_Info;


// One linear memory. Memories belong to the module that declares them, not to
// the runtime: two modules loaded into the same runtime each get their own, and
// the interpreter reaches the one it is currently running against through the
// _mem pseudo-register (see M3MemoryHeader).
//
// Every entry is individually allocated so that a slot may later be pointed at
// a memory another module owns - growing one has to be visible through every
// name it has. 'owner' says which module allocated it, and so which module
// frees it.
రకనిర్వచనం నిర్మాణం M3_జ్ఞాపకం {
    M3_జ్ఞాపకం_శీర్షిక*  mallocated;

    u64              అంకె_పుటలు;
    u64              max_పుటలు;
    u64              init_పుటలు;
    u32              పుట_కొలత;
    bool             hasMax;         // see M3MemoryInfo
    bool             isMemory64;     // addressed by i64 rather than i32
    bool             isShared;       // see M3MemoryInfo

    నిర్మాణం M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;

#ఒకవేళ d_m3_Guarded_జ్ఞాపకం
    // The slot of the guarded arena this memory's bytes live in, and so what
    // 'mallocated' points into rather than the heap - see Guard_TakeSlot
    శూన్య* guard_స్థానం;
#అయితే_ముగింపు
} M3_జ్ఞాపకం;

రకనిర్వచనం M3_జ్ఞాపకం* IM3_జ్ఞాపకం;

// The value type a memory's addresses, page counts and lengths are expressed
// in. Every instruction naming the memory takes and returns that type.
స్థిర అంతఃవరుస
m3type_t జ్ఞాపకం_Addr_Type (స్థిరరాశి M3_జ్ఞాపకం* i_జ్ఞాపకం)
{
    తిరిగి i_జ్ఞాపకం->isMemory64 ? c_m3Type_i64 : c_m3Type_i32;
}


// A module that declares no custom page size means the default one. The parser
// leaves that as zero, and InitMemory only fills it in when it allocates, so a
// memory has to be asked rather than read directly until then.
స్థిర అంతఃవరుస
u32 జ్ఞాపకం_పుట_కొలత (స్థిరరాశి M3_జ్ఞాపకం* i_జ్ఞాపకం)
{
    తిరిగి i_జ్ఞాపకం->పుట_కొలత ? i_జ్ఞాపకం->పుట_కొలత : d_m3_Default_Mem_పుట_కొలత;
}


//---------------------------------------------------------------------------------------------------------------------------------

// A table type as the module declared it. Sizes stay u32 - a table64 may name
// far more entries than that, but d_m3MaxSaneTableSize refuses anything near
// the limit long before it matters.
రకనిర్వచనం నిర్మాణం M3_పట్టిక_Info {
    m3type_t elemType;
    u32      init_కొలత;
    u32      max_కొలత;
    bool     hasMax;         // a declared maximum of 0 is not the same as none
    bool     isTable64;      // indexed by i64 rather than i32
} M3_పట్టిక_Info;


// A table's elements are opaque pointer-sized references: IM3Function for a
// funcref table, a host handle for an externref one. NULL is the null reference.
రకనిర్వచనం నిర్మాణం M3_పట్టిక {
    శూన్య**           elements;
    u32              కొలత;
    u32              max_కొలత;            // 0 when the module declared no maximum
    m3type_t         type;

    // Every slot starts out holding this, rather than null. A table whose
    // element type is not nullable has to say what to fill itself with.
    బైట్లు_t          init_సమాసం;
    u32              init_సమాసం_కొలత;

    u32              init_కొలత;       // the declared minimum
    bool             hasMax;         // a declared maximum of 0 is not the same as none
    bool             isTable64;      // indexed by i64 rather than i32

    నిర్మాణం M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;
} M3_పట్టిక;

రకనిర్వచనం M3_పట్టిక* IM3_పట్టిక;

// The value type a table's indexes and sizes are expressed in - table64 is the
// same choice as memory64, made per table.
స్థిర అంతఃవరుస
m3type_t పట్టిక_Addr_Type (స్థిరరాశి M3_పట్టిక* i_పట్టిక)
{
    తిరిగి i_పట్టిక->isTable64 ? c_m3Type_i64 : c_m3Type_i32;
}

// Element segment modes, from the low two bits of the segment's flags
రకనిర్వచనం గణన {
    c_m3_Elem_చురుకైన      = 0,        // table 0
    c_m3Elem_passive     = 1,
    c_m3_Elem_చురుకైన_Idx   = 2,        // explicit table index
    c_m3Elem_declarative = 3
} M3_Element_బహుళకం;

రకనిర్వచనం నిర్మాణం M3ElementSegment {
    బైట్లు_t  init_సమాసం;       // active segments only: the offset
    బైట్లు_t  elements;       // funcidx list, or const exprs when isExpr
    శూన్య**   resolved;       // passive segments only, for table.init

    u32      init_సమాసం_కొలత;
    u32      అంకె_Elements;
    u32      పట్టిక_సూచీ;

    m3type_t type;
    u8       బహుళకం;
    bool     is_సమాసం;
    bool     dropped;
} M3ElementSegment;

రకనిర్వచనం నిర్మాణం M3_దత్తాంశం_Segment {
    స్థిరరాశి u8* init_సమాసం;           // wasm code
    స్థిరరాశి u8* దత్తాంశం;

    u32       init_సమాసం_కొలత;
    u32       జ్ఞాపకం_Region;
    u32       కొలత;

    bool      isPassive;
    bool      dropped;            // active segments are dropped once instantiated
} M3_దత్తాంశం_Segment;

//---------------------------------------------------------------------------------------------------------------------------------

రకనిర్వచనం నిర్మాణం M3Global {
    M3ImportInfo     import;

    // An import linked to another module's export: the global that actually
    // holds the value. Reads and writes go there, so a mutable global stays one
    // cell seen under two names. NULL for anything else, including an import a
    // host supplied a value for through m3_LinkGlobal.
    నిర్మాణం M3Global* resolved;

    సంఘం {
        i32 i32_విలువ;
        i64 i64_విలువ;
#ఒకవేళ d_m3HasFloat
        f64 f64_విలువ;
        f32 f32_విలువ;
#అయితే_ముగింపు
        శూన్య* ref_విలువ;
    };

    బైట్లు_t  init_సమాసం;       // wasm code
    u32      init_సమాసం_కొలత;
    m3type_t type;
    bool     imported;
    bool     isMutable;
} M3Global;


//---------------------------------------------------------------------------------------------------------------------------------

#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching

// An exception / control tag, as the tag section declares it. Tags are compared by
// identity, and this struct's address is that identity.
రకనిర్వచనం నిర్మాణం M3_ట్యాగ్ {
    M3ImportInfo  import;
    IM3FuncType   type;           // params are the payload; results are empty for EH, can be non-empty for stack switching
    నిర్మాణం M3_ట్యాగ్* resolved;
    bool          imported;
} M3_ట్యాగ్;

// A tag m3_NewTag made. It belongs to the runtime rather than to a module, and
// is linked in front of the runtime's others rather than kept in an array, which
// would move the tags - and with them their identities - whenever it grew.
రకనిర్వచనం నిర్మాణం M3_Host_ట్యాగ్ {
    నిర్మాణం M3_Host_ట్యాగ్* తదుపరి;
    M3_ట్యాగ్             ట్యాగ్;
} M3_Host_ట్యాగ్;

#అయితే_ముగింపు // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ఒకవేళ d_m3HasExceptionHandling

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
రకనిర్వచనం నిర్మాణం M3Exception {
    నిర్మాణం M3Exception* తదుపరి;           // the runtime's allocation list,
    నిర్మాణం M3Exception* prev;           //   doubly linked so one can leave early
    IM3_ట్యాగ్              ట్యాగ్;
    u32                 అంకె_వాదనలు;
    bool                reified;        // an exnref to this has been handed out
    u64                 వాదనలు[];
} M3Exception;


M3Exception* NewException (IM3Runtime io_runtime, IM3_ట్యాగ్ i_ట్యాగ్, u32 i_అంకె_వాదనలు);
శూన్య         FreeException (IM3Runtime io_runtime, M3Exception* i_exception);
శూన్య         FreeExceptions (IM3Runtime io_runtime);

#అయితే_ముగింపు // d_m3HasExceptionHandling


#ఒకవేళ d_m3_Has_దొంతర_Switching

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
రకనిర్వచనం గణన M3_చట్రం_రకం {
    చట్రం_call,          // op_Call / op_CallRef / op_CallIndirect
    చట్రం_loop,          // op_Loop
#  ఒకవేళ d_m3HasExceptionHandling
    చట్రం_try,           // op_TryTable
#  endif
#  ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
    చట్రం_నమోదు,         // op_Entry, in builds where it keeps a frame
#  endif
    చట్రం_resume,        // op_Resume, when the suspend is not its to answer
} M3_చట్రం_రకం;

రకనిర్వచనం నిర్మాణం M3_చట్రం {
    u8        రకం;      // M3FrameKind
    pc_t      pc;        // call: the return address | loop: the loop id, which is
                         // also its body | try: the clause table | entry: unused
    m3stack_t sp;
    IM3_జ్ఞాపకం జ్ఞాపకం;    // _mem is re-derived from this rather than stored:
                         // growing linear memory moves the header, so only the
                         // M3Memory stays valid across a suspend

    సంఘం {
        // op_Call resumes the caller with the registers its own native frame was
        // holding, so those belong to the frame, not to the suspension point
        నిర్మాణం {
            IM3Function function;
            m3reg_t     r0;
#  ఒకవేళ d_m3HasFloat
            f64 fp0;
#  endif
        } call;
#  ఒకవేళ d_m3HasExceptionHandling
        నిర్మాణం {
            u32  అంకె_Clauses;
            bool handlers_సజీవ;   // false once op_PopHandlers has retired the region
        } try_;
#  endif
#  ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
        నిర్మాణం {
            IM3Function function;
        } నమోదు_2;
#  endif
        // A resume that the suspend travelled straight through, because the
        // tag was not one its handlers named. The continuation it was running
        // is part of what got captured, so everything needed to set it going
        // again under the same handlers is kept here.
        నిర్మాణం {
            నిర్మాణం M3Continuation* cont;
            pc_t                   handlersPC;
            u32                    అంకె_Handlers;
            pc_t                   resultsPC;
            u32                    అంకె_Results;
        } resume;
    };
} M3_చట్రం;

// Which of the three resume forms an op_Resume was emitted for
గణన {
    d_m3_Resume_సాధారణ = 0,
    d_m3ResumeThrow,
    d_m3ResumeThrowRef,
};

రకనిర్వచనం గణన M3ContinuationState {
    cont_allocated = 0,
    cont_running,
    cont_suspended,
    cont_returned,
    cont_consumed
} M3ContinuationState;

రకనిర్వచనం నిర్మాణం M3ResumeHandler {
    u8     రకం;        // 0: (on $t $h), 1: (on $t switch)
    IM3_ట్యాగ్ ట్యాగ్;
    pc_t   stubPC;
} M3ResumeHandler;

రకనిర్వచనం నిర్మాణం M3Continuation {
    నిర్మాణం M3Continuation* తదుపరి;           // linked list in runtime for cleanup
    IM3FuncType            type;            // continuation type (cont $ft)
    IM3Function            నమోదు_Function;   // target function
    M3ContinuationState    state;

    m3slot_t*              val_దొంతర;        // allocated value stack buffer
    u32                    అంకె_దొంతర_Slots;
    m3stack_t              sp;              // current value stack pointer

    M3_చట్రం*               చట్రాలు;          // native frames recorded by the last suspend
    u32                    చట్రాలు_Cap;       // entries allocated
    u32                    అంకె_చట్రాలు;       // entries in use, innermost first

    pc_t                   pc;              // resume instruction pointer
    u8                     suspend_బిందువు;    // M3SafePointKind: what left pc where it is
    m3reg_t                r0;              // saved accumulator
#  ఒకవేళ d_m3HasFloat
    f64 fp0;
#  endif

    // Bound args count (for cont.bind)
    u32 bound_వాదనలు_లెక్క;

#  ఒకవేళ d_m3HasExceptionHandling
    // resume_throw: raised at the suspension point instead of carrying on from
    // it, so the try regions inside the continuation get their turn first
    M3Exception* resumeThrow;
#  endif

    // Where the values go when this continuation is next given some: the slots
    // its own suspend or switch is waiting on.
    u32                    అంకె_Suspend_Results;
    i32                    suspend_ఫలితం_Offsets[d_m3MaxContinuationPayload];
    u32                    suspend_ఫలితం_Is64[d_m3MaxContinuationPayload];

    // Active handler table from enclosing resume:
    u32                    అంకె_Handlers;
    pc_t                   handlersPC;
    నిర్మాణం M3Continuation* మాతృ;         // resumer continuation
} M3Continuation, *IM3Continuation;

M3_ఫలితం        Continuation_Acquire_దొంతర (IM3Runtime runtime, IM3Continuation cont);
శూన్య            Continuation_Recycle_దొంతర (IM3Runtime runtime, IM3Continuation cont);
IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function);
IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont);
శూన్య            Continuation_ReleaseAll (IM3Runtime io_runtime);

// Appends one frame to the suspending continuation. Returns the marker the
// caller should keep unwinding with: the suspend marker when the frame was
// recorded, or a trap when it could not be - a continuation that cannot record
// its frames cannot be resumed, so the suspend has to become a trap instead.
m3ret_t         Continuation_రికార్డు_చట్రం (IM3Runtime i_runtime, స్థిరరాశి M3_చట్రం* i_చట్రం);

// Points runtime->stack, where the host's next call puts its arguments, above
// whatever a suspended root continuation holds - or back at the bottom when
// there is none.
శూన్య            Runtime_ఉంచు_Call_దొంతర (IM3Runtime io_runtime);

#అయితే_ముగింపు // d_m3HasStackSwitching


#ఒకవేళ d_m3HasSnapshots
రకనిర్వచనం నిర్మాణం M3EmbeddedSnapshot {
    cstr_t                     name;
    బైట్లు_t                    దత్తాంశం;
    u32                        కొలత;
    నిర్మాణం M3EmbeddedSnapshot* తదుపరి;
} M3EmbeddedSnapshot;
#అయితే_ముగింపు


//---------------------------------------------------------------------------------------------------------------------------------

// One entry of the export section. Export names belong to the module rather than
// to what they name: one entity can be exported under any number of names, and an
// import re-exported under a name of the module's own, whose slot linking points
// at another module's entity. Every lookup by name goes through these, and
// resolves the index against the module's index space at the time it is asked.
రకనిర్వచనం నిర్మాణం M3Export {
    cstr_t name;
    u32    name_నిడివి;   // the name may hold a NUL, which ends the C string early
    u32    index;        // into the index space 'kind' names
    u8     రకం;         // d_externalKind_*
} M3Export;

రకనిర్వచనం నిర్మాణం M3Module {
    నిర్మాణం M3Runtime*     runtime;
    నిర్మాణం M3Environment* environment;

    బైట్లు_t               wasm_ప్రారంభం;
    బైట్లు_t               wasm_చివర;

    cstr_t                name;

    u32                   అంకె_Func_Types;
    IM3FuncType*          funcTypes;              // array of pointers to list of FuncTypes

    u32                   అంకె_Func_Imports;
    u32                   అంకె_Functions;
    u32                   allFunctions;           // allocated functions count
    M3Function*           functions;

    i32                   ప్రారంభం_Function;

    u32                   అంకె_దత్తాంశం_Segments;
    M3_దత్తాంశం_Segment*        దత్తాంశం_Segments;

    u32                   దత్తాంశం_లెక్క;          // from the data count section
    bool                  has_దత్తాంశం_లెక్క;

    // Bitset of the functions ref.func may name: those exported, or referenced
    // by an element segment or a global initializer. One bit per function.
    u8*                   declaredFuncs;

    //u32                     importedGlobals;
    u32                   అంకె_Globals;
    M3Global*             globals;

#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
    u32    అంకె_ట్యాగులు;
    M3_ట్యాగ్* ట్యాగులు;
#అయితే_ముగింపు

    u32               అంకె_Element_Segments;
    M3ElementSegment* elementSegments;
    బైట్లు_t           element_Section_చివర;

    // The module's table index space, imported entries first. Entries are
    // borrowed pointers, individually allocated so a slot can be pointed at a
    // table another module owns - see M3Table.owner, and M3Module.memories,
    // which works the same way.
    IM3_పట్టిక*         tables;
    u32               అంకె_Tables;

    // The module's memory index space: imported entries first, then declared
    // ones, exactly as the Wasm index space orders them. Entries are borrowed
    // pointers - see M3Memory.owner.
    IM3_జ్ఞాపకం*        memories;
    u32               అంకె_Memories;

    // The interpreter always carries a valid memory header in _mem - the
    // stack-limit check, the backtrace recorder and the call ops all reach the
    // runtime through it. A module that declares no memory still needs one, so
    // it gets this zero-length stand-in rather than a NULL _mem.
    M3_జ్ఞాపకం          ఖాళీలు_జ్ఞాపకం;

    // memories[0], or emptyMemory when there are none. Resolved once by
    // InitMemory - the index space is fixed by then and linking has already
    // repointed whatever it was going to - so the call ops can read it straight
    // instead of branching. NULL until the module is loaded.
    IM3_జ్ఞాపకం         memory0;

    M3Export*         exports;
    u32               అంకె_Exports;

#ఒకవేళ d_m3HasSnapshots
    నిర్మాణం M3EmbeddedSnapshot* snapshots;

    // A snapshot restores into a module none of whose code has run - start
    // function included - and a restore that fails part way leaves one that
    // can no longer run at all: its store is neither the snapshot's nor its own.
    bool                       has_నిరంతరం;
    bool                       isUnusable;
#అయితే_ముగింపు

    నిర్మాణం M3Module* తదుపరి;
} M3Module;

M3_ఫలితం Module_కూడు_జ్ఞాపకం (IM3Module io_module, IM3_జ్ఞాపకం* o_జ్ఞాపకం, స్థిరరాశి M3_జ్ఞాపకం_Info* i_info, bool i_isImported);

// Memory 0 of a module - the one the interpreter's _mem register tracks while
// that module's code is running. Never NULL once the module has been loaded.
స్థిర అంతఃవరుస
IM3_జ్ఞాపకం Module_Memory0 (IM3Module i_module)
{
    తిరిగి i_module->memory0;
}

స్థిర అంతఃవరుస
M3_జ్ఞాపకం_శీర్షిక* Module_జ్ఞాపకం_శీర్షిక (IM3Module i_module)
{
    తిరిగి i_module->memory0->mallocated;
}

M3_ఫలితం Module_కూడు_Global (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported);
M3_ఫలితం Module_కూడు_పట్టిక (IM3Module io_module, IM3_పట్టిక* o_పట్టిక, స్థిరరాశి M3_పట్టిక_Info* i_info, bool i_isImported);
#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
M3_ఫలితం Module_కూడు_ట్యాగ్ (IM3Module io_module, IM3_ట్యాగ్* o_ట్యాగ్, IM3FuncType i_type, bool i_isImported);
#అయితే_ముగింపు
#ఒకవేళ d_m3_Has_దొంతర_Switching
IM3FuncType Module_Cont_Type_యొక్క_Ref (IM3Module i_module, m3type_t i_refType);
#అయితే_ముగింపు
M3_ఫలితం    Module_DeclareFunction (IM3Module io_module, u32 i_సూచీ);
bool        Module_IsFunctionDeclared (IM3Module i_module, u32 i_సూచీ);

M3_ఫలితం    Module_PreallocFunctions (IM3Module io_module, u32 i_మొత్తం_Functions);
M3_ఫలితం    Module_కూడు_Function (IM3Module io_module, u32 i_type_సూచీ, IM3ImportInfo i_importInfo /* can be null */);
IM3Function Module_GetFunction (IM3Module i_module, u32 i_function_సూచీ);
bool        Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule);

శూన్య        Module_Generate_పేర్లు (IM3Module i_module);

శూన్య        FreeImportInfo (M3ImportInfo* i_info);

//---------------------------------------------------------------------------------------------------------------------------------

రకనిర్వచనం నిర్మాణం M3Environment {
    //    struct M3Runtime *      runtimes;

    IM3FuncType      funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType      retFuncTypes[c_m3_Type_లెక్క];      // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    u16              అంకె_Func_Types;                       // hands out M3FuncType.canonicalIndex
    M3_సంకేతం_పుట*      పుటలు_Released;

    M3SectionHandler customSectionHandler;
} M3Environment;

శూన్య     Environment_Release (IM3Environment i_environment);

// takes ownership of io_funcType and returns a pointer to the persistent version (could be same or different)
M3_ఫలితం Environment_కూడు_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType);
M3_ఫలితం Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_లెక్క, u16* o_మొదటి_సూచీ);
శూన్య     Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_సూచీ);

#ఒకవేళ d_m3HasTypedRefs
M3_ఫలితం Parse_కుప్ప_Type (IM3Module i_module, m3type_t* o_కుప్ప_బిట్లు, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);
#అయితే_ముగింపు
M3_ఫలితం Parse_విలువ_Type (IM3Module i_module, m3type_t* o_type, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర);

//---------------------------------------------------------------------------------------------------------------------------------


రకనిర్వచనం నిర్మాణం M3Runtime {
    M3Compilation  compilation;

    IM3Environment environment;

    M3_సంకేతం_పుట*    పుటలు_Open;      // linked list of code pages with writable space on them
    M3_సంకేతం_పుట*    పుటలు_నిండిన;      // linked list of at-capacity pages

    u32            అంకె_సంకేతం_పుటలు;
    u32            అంకె_చురుకైన_సంకేతం_పుటలు;

    IM3Module      modules;        // linked list of imported modules

    శూన్య*          దొంతర;
    శూన్య*          origin_దొంతర;
    u32            దొంతర_కొలత;
    u32            అంకె_దొంతర_Slots;
    శూన్య*          దొంతర_హద్దు;     // native C-stack low-water mark; Wasm calls trap past it (NULL = unset)
    IM3Function    చివరి_Called;     // last function that successfully executed
    IM3Function    entered;        // entry point of the outermost call in flight, or of the last one, trapped or not
    u32            callNesting;    // RunCodeChecked() recursion depth

    శూన్య*          userdata;

    u32            జ్ఞాపకం_హద్దు;
    u64            జ్ఞాపకం_బైట్లు_హద్దు;
    u64            జ్ఞాపకం_బైట్లు_వాడిన;
    u64            పట్టిక_Elements_హద్దు;
    u64            పట్టిక_Elements_వాడిన;

#ఒకవేళ d_m3DeterministicProfile
    // What the host answers about time and entropy out of, rather than out of the
    // machine it happens to be running on - see m3_deterministic.h, which is what
    // reads and moves them. Both are per-runtime, so two runtimes replay
    // independently and a fresh runtime replays the same way as the last one.
    u64 virtual_సమయం_Ns;
    u64 randomState;
#అయితే_ముగింపు

#ఒకవేళ d_m3EnableValidation
    bool    skipValidation; // m3_SetValidation: compile function bodies without type checking them first
    ValCtx* validator;      // created on first use
#అయితే_ముగింపు

#ఒకవేళ d_m3EnableStrace >= 2
    u32 call_లోతు;
#అయితే_ముగింపు

#ఒకవేళ d_m3HasGasMetering
    // Both counts are in gas units (see M3_GAS_UNITS_PER_GAS). gasRemaining goes
    // negative by the cost of the segment that ran out, which is what makes the
    // gas used come out slightly over the limit, and is deliberate: a segment
    // is paid for in full before any of it runs.
    i64 gas_హద్దు;
    i64 gas_మిగిలిన;
#అయితే_ముగింపు

#ఒకవేళ d_m3HasExceptionHandling
    u32          try_లోతు;           // number of try regions whose body is executing
    M3Exception* pendingException;   // the exception currently unwinding, if any
    M3Exception* exceptions;         // the ones it still holds
    M3Exception* uncaughtException;  // a copy of what ended the last call, off the list
#అయితే_ముగింపు
#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
    M3_Host_ట్యాగ్* host_ట్యాగులు;
#అయితే_ముగింపు

    // Set by the embedder and by the host's interrupt handler, so they are here
    // whether or not the interpreter was built able to act on them. Only the
    // suspendable operations read suspendRequested, and those exist only with
    // stack switching, so without it m3_RequestSuspend is simply never answered.
    bool          isSuspendable;
    అస్థిర bool suspendRequested;

#ఒకవేళ d_m3_Has_దొంతర_Switching
    // Set while m3_ResumeRuntime replays a paused invocation: a back edge or a
    // function entry it paused at goes on past its check, so a pause already
    // requested takes effect at the next one instead of on the spot again
    bool            resumePastCheck;

    u32             continuations_హద్దు;
    u32             continuationsAllocated;
    m3slot_t**      val_దొంతర_Pool;
    u32             val_దొంతర_Pool_Cap;
    u32             val_దొంతర_Pool_లెక్క;
    IM3Continuation చురుకైన_Continuation;
    IM3Continuation suspendedContinuation;
    IM3Continuation continuations;
    IM3Continuation మూలం_Continuation;

    // The suspend on its way out, if there is one. Only ever one at a time -
    // it travels up the native stack and is answered before anything else can
    // start - so this belongs to the runtime rather than to any continuation.
    IM3_ట్యాగ్          suspend_ట్యాగ్;
    IM3Continuation suspendHandlerCont;     // whose resume named the tag
    pc_t            suspendStubPC;          // the handler it is headed for
    IM3Continuation switch_లక్ష్యం;           // the peer to run instead, for a switch
    u64             suspendPayload[d_m3MaxContinuationPayload];
    u32             suspendPayloadIs64[d_m3MaxContinuationPayload];
    u32             అంకె_Suspend_Payload;
#అయితే_ముగింపు

#ఒకవేళ d_m3HasSnapshots
    M3SnapshotHooks snapshotHooks;
#అయితే_ముగింపు

    M3ErrorInfo error;
#ఒకవేళ d_m3VerboseErrorMessages
    అక్షరం error_సందేశం[256]; // the actual buffer. M3ErrorInfo can point to this
#అయితే_ముగింపు

#ఒకవేళ d_m3_రికార్డు_Backtraces
    M3BacktraceInfo backtrace;
#అయితే_ముగింపు

    u32 new_సంకేతం_పుట_Sequence;
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
#ఒకవేళ d_m3_Max_Native_దొంతర > 0
#  define d_m3_దొంతర_హద్దు_Enter(RT)                                               \
        శూన్య * m3_Saved_దొంతర_హద్దు = (RT)->దొంతర_హద్దు;                           \
        ఒకవేళ (not (RT)->దొంతర_హద్దు)                                               \
            (RT)->దొంతర_హద్దు = m3_Native_దొంతర_హద్దు(m3_Native_దొంతర_Ptr(), (d_m3_Max_Native_దొంతర));
#  define d_m3_దొంతర_హద్దు_Leave(RT)  (RT)->దొంతర_హద్దు = m3_Saved_దొంతర_హద్దు;
#లేకపోతే
#  define d_m3_దొంతర_హద్దు_Enter(RT)
#  define d_m3_దొంతర_హద్దు_Leave(RT)
#అయితే_ముగింపు

శూన్య     InitRuntime (IM3Runtime io_runtime, u32 i_దొంతర_కొలత_లోపల_బైట్లు);
శూన్య     Runtime_Release (IM3Runtime io_runtime);

M3_ఫలితం Resize_జ్ఞాపకం (IM3Runtime io_runtime, IM3_జ్ఞాపకం io_జ్ఞాపకం, u64 i_అంకె_పుటలు);

// Give back whatever is behind io_memory->mallocated, which is the heap or a slot of
// the guarded arena depending on the build. Leaves the M3Memory itself alone.
శూన్య     Free_జ్ఞాపకం_ఖండం (IM3_జ్ఞాపకం io_జ్ఞాపకం);

రకనిర్వచనం శూన్య* (*ModuleVisitor)(IM3Module i_module, శూన్య* i_info);
శూన్య*       ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, శూన్య* i_info);

శూన్య*       v_కనుగొను_Function (IM3Module i_module, శూన్య* i_info);

IM3_సంకేతం_పుట Acquire_సంకేతం_పుట (IM3Runtime io_runtime);
IM3_సంకేతం_పుట Acquire_సంకేతం_పుట_With_సామర్థ్యం (IM3Runtime io_runtime, u32 i_పంక్తి_లెక్క);
శూన్య        Release_సంకేతం_పుట (IM3Runtime io_runtime, IM3_సంకేతం_పుట i_సంకేతం_పుట);

#ఒకవేళ d_m3_Has_దొంతర_Switching
స్థిర అంతఃవరుస
bool Continuation_Can_Acquire_దొంతర (IM3Runtime runtime)
{
    తిరిగి not runtime->continuations_హద్దు or
           runtime->continuationsAllocated < runtime->continuations_హద్దు;
}
#అయితే_ముగింపు

d_m3_చివర_Extern_C

#అయితే_ముగింపు // m3_env_h
