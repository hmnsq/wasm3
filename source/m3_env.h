//
//  m3_env.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_env_h
#define m3_env_h

#അന്തര്ഭാവ "wasm3.h"
#അന്തര്ഭാവ "m3_code.h"
#അന്തര്ഭാവ "m3_compile.h"

d_m3BeginExternC

തരനിർവചനം ഘടന ValCtx ValCtx;   // the validator's scratch, defined in m3_validate.c


//---------------------------------------------------------------------------------------------------------------------------------

// Page counts are u64 because a memory64 type may declare up to 2^48 pages.
// Nothing near that can be backed, but the module still has to parse and
// validate; it is instantiation that fails.
തരനിർവചനം ഘടന M3MemoryInfo {
    u64  initPages;
    u64  maxPages;
    u32  pageSize;
    bool hasMax;         // a declared maximum of 0 is not the same as none
    bool isMemory64;     // addressed by i64 rather than i32
    bool isShared;       // the threads proposal: a maximum is required
} M3MemoryInfo;


// One linear memory. Memories belong to the module that declares them, not to
// the runtime: two modules loaded into the same runtime each get their own, and
// the interpreter reaches the one it is currently running against through the
// _mem pseudo-register (see M3MemoryHeader).
//
// Every entry is individually allocated so that a slot may later be pointed at
// a memory another module owns - growing one has to be visible through every
// name it has. 'owner' says which module allocated it, and so which module
// frees it.
തരനിർവചനം ഘടന M3Memory {
    M3MemoryHeader*  mallocated;

    u64              അക്കം_Pages;
    u64              maxPages;
    u64              initPages;
    u32              pageSize;
    bool             hasMax;         // see M3MemoryInfo
    bool             isMemory64;     // addressed by i64 rather than i32
    bool             isShared;       // see M3MemoryInfo

    ഘടന M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;

#എങ്കിൽ d_m3GuardedMemory
    // The slot of the guarded arena this memory's bytes live in, and so what
    // 'mallocated' points into rather than the heap - see Guard_TakeSlot
    ശൂന്യം* guardSlot;
#endif
} M3Memory;

തരനിർവചനം M3Memory* IM3Memory;

// The value type a memory's addresses, page counts and lengths are expressed
// in. Every instruction naming the memory takes and returns that type.
സ്ഥിതം വരിയിൽ
m3type_t Memory_AddrType (സ്ഥിരം M3Memory* i_memory)
{
    തിരികെ i_memory->isMemory64 ? c_m3Type_i64 : c_m3Type_i32;
}


// A module that declares no custom page size means the default one. The parser
// leaves that as zero, and InitMemory only fills it in when it allocates, so a
// memory has to be asked rather than read directly until then.
സ്ഥിതം വരിയിൽ
u32 Memory_PageSize (സ്ഥിരം M3Memory* i_memory)
{
    തിരികെ i_memory->pageSize ? i_memory->pageSize : d_m3DefaultMemPageSize;
}


//---------------------------------------------------------------------------------------------------------------------------------

// A table type as the module declared it. Sizes stay u32 - a table64 may name
// far more entries than that, but d_m3MaxSaneTableSize refuses anything near
// the limit long before it matters.
തരനിർവചനം ഘടന M3TableInfo {
    m3type_t elemType;
    u32      initSize;
    u32      maxSize;
    bool     hasMax;         // a declared maximum of 0 is not the same as none
    bool     isTable64;      // indexed by i64 rather than i32
} M3TableInfo;


// A table's elements are opaque pointer-sized references: IM3Function for a
// funcref table, a host handle for an externref one. NULL is the null reference.
തരനിർവചനം ഘടന M3Table {
    ശൂന്യം**           elements;
    u32              size;
    u32              maxSize;            // 0 when the module declared no maximum
    m3type_t         type;

    // Every slot starts out holding this, rather than null. A table whose
    // element type is not nullable has to say what to fill itself with.
    bytes_t          init_വ്യഞ്ജകം;
    u32              init_വ്യഞ്ജകം_Size;

    u32              initSize;       // the declared minimum
    bool             hasMax;         // a declared maximum of 0 is not the same as none
    bool             isTable64;      // indexed by i64 rather than i32

    ഘടന M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;
} M3Table;

തരനിർവചനം M3Table* IM3Table;

// The value type a table's indexes and sizes are expressed in - table64 is the
// same choice as memory64, made per table.
സ്ഥിതം വരിയിൽ
m3type_t Table_AddrType (സ്ഥിരം M3Table* i_table)
{
    തിരികെ i_table->isTable64 ? c_m3Type_i64 : c_m3Type_i32;
}

// Element segment modes, from the low two bits of the segment's flags
തരനിർവചനം എണ്ണൽ {
    c_m3Elem_active      = 0,        // table 0
    c_m3Elem_passive     = 1,
    c_m3Elem_activeIdx   = 2,        // explicit table index
    c_m3Elem_declarative = 3
} M3ElementMode;

തരനിർവചനം ഘടന M3ElementSegment {
    bytes_t  init_വ്യഞ്ജകം;       // active segments only: the offset
    bytes_t  elements;       // funcidx list, or const exprs when isExpr
    ശൂന്യം**   resolved;       // passive segments only, for table.init

    u32      init_വ്യഞ്ജകം_Size;
    u32      അക്കം_Elements;
    u32      tableIndex;

    m3type_t type;
    u8       mode;
    bool     is_വ്യഞ്ജകം;
    bool     dropped;
} M3ElementSegment;

തരനിർവചനം ഘടന M3DataSegment {
    സ്ഥിരം u8* init_വ്യഞ്ജകം;           // wasm code
    സ്ഥിരം u8* data;

    u32       init_വ്യഞ്ജകം_Size;
    u32       memoryRegion;
    u32       size;

    bool      isPassive;
    bool      dropped;            // active segments are dropped once instantiated
} M3DataSegment;

//---------------------------------------------------------------------------------------------------------------------------------

തരനിർവചനം ഘടന M3Global {
    M3ImportInfo     import;

    // An import linked to another module's export: the global that actually
    // holds the value. Reads and writes go there, so a mutable global stays one
    // cell seen under two names. NULL for anything else, including an import a
    // host supplied a value for through m3_LinkGlobal.
    ഘടന M3Global* resolved;

    സംഘം {
        i32 i32_മൂല്യം;
        i64 i64_മൂല്യം;
#എങ്കിൽ d_m3HasFloat
        f64 f64_മൂല്യം;
        f32 f32_മൂല്യം;
#endif
        ശൂന്യം* ref_മൂല്യം;
    };

    bytes_t  init_വ്യഞ്ജകം;       // wasm code
    u32      init_വ്യഞ്ജകം_Size;
    m3type_t type;
    bool     imported;
    bool     isMutable;
} M3Global;


//---------------------------------------------------------------------------------------------------------------------------------

#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching

// An exception / control tag, as the tag section declares it. Tags are compared by
// identity, and this struct's address is that identity.
തരനിർവചനം ഘടന M3Tag {
    M3ImportInfo  import;
    IM3FuncType   type;           // params are the payload; results are empty for EH, can be non-empty for stack switching
    ഘടന M3Tag* resolved;
    bool          imported;
} M3Tag;

// A tag m3_NewTag made. It belongs to the runtime rather than to a module, and
// is linked in front of the runtime's others rather than kept in an array, which
// would move the tags - and with them their identities - whenever it grew.
തരനിർവചനം ഘടന M3HostTag {
    ഘടന M3HostTag* next;
    M3Tag             tag;
} M3HostTag;

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#എങ്കിൽ d_m3HasExceptionHandling

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
തരനിർവചനം ഘടന M3Exception {
    ഘടന M3Exception* next;           // the runtime's allocation list,
    ഘടന M3Exception* prev;           //   doubly linked so one can leave early
    IM3Tag              tag;
    u32                 അക്കം_Args;
    bool                reified;        // an exnref to this has been handed out
    u64                 args[];
} M3Exception;


M3Exception* NewException (IM3Runtime io_runtime, IM3Tag i_tag, u32 i_അക്കം_Args);
ശൂന്യം         FreeException (IM3Runtime io_runtime, M3Exception* i_exception);
ശൂന്യം         FreeExceptions (IM3Runtime io_runtime);

#endif // d_m3HasExceptionHandling


#എങ്കിൽ d_m3_Has_അട്ടി_Switching

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
തരനിർവചനം എണ്ണൽ M3FrameKind {
    frame_call,          // op_Call / op_CallRef / op_CallIndirect
    frame_loop,          // op_Loop
#  എങ്കിൽ d_m3HasExceptionHandling
    frame_try,           // op_TryTable
#  endif
#  എങ്കിൽ d_m3EntryKeepsFrame
    frame_entry,         // op_Entry, in builds where it keeps a frame
#  endif
    frame_resume,        // op_Resume, when the suspend is not its to answer
} M3FrameKind;

തരനിർവചനം ഘടന M3Frame {
    u8        kind;      // M3FrameKind
    pc_t      pc;        // call: the return address | loop: the loop id, which is
                         // also its body | try: the clause table | entry: unused
    m3stack_t sp;
    IM3Memory memory;    // _mem is re-derived from this rather than stored:
                         // growing linear memory moves the header, so only the
                         // M3Memory stays valid across a suspend

    സംഘം {
        // op_Call resumes the caller with the registers its own native frame was
        // holding, so those belong to the frame, not to the suspension point
        ഘടന {
            IM3Function function;
            m3reg_t     r0;
#  എങ്കിൽ d_m3HasFloat
            f64 fp0;
#  endif
        } call;
#  എങ്കിൽ d_m3HasExceptionHandling
        ഘടന {
            u32  അക്കം_Clauses;
            bool handlersLive;   // false once op_PopHandlers has retired the region
        } try_;
#  endif
#  എങ്കിൽ d_m3EntryKeepsFrame
        ഘടന {
            IM3Function function;
        } entry;
#  endif
        // A resume that the suspend travelled straight through, because the
        // tag was not one its handlers named. The continuation it was running
        // is part of what got captured, so everything needed to set it going
        // again under the same handlers is kept here.
        ഘടന {
            ഘടന M3Continuation* cont;
            pc_t                   handlersPC;
            u32                    അക്കം_Handlers;
            pc_t                   resultsPC;
            u32                    അക്കം_Results;
        } resume;
    };
} M3Frame;

// Which of the three resume forms an op_Resume was emitted for
എണ്ണൽ {
    d_m3ResumeNormal = 0,
    d_m3ResumeThrow,
    d_m3ResumeThrowRef,
};

തരനിർവചനം എണ്ണൽ M3ContinuationState {
    cont_allocated = 0,
    cont_running,
    cont_suspended,
    cont_returned,
    cont_consumed
} M3ContinuationState;

തരനിർവചനം ഘടന M3ResumeHandler {
    u8     kind;        // 0: (on $t $h), 1: (on $t switch)
    IM3Tag tag;
    pc_t   stubPC;
} M3ResumeHandler;

തരനിർവചനം ഘടന M3Continuation {
    ഘടന M3Continuation* next;           // linked list in runtime for cleanup
    IM3FuncType            type;            // continuation type (cont $ft)
    IM3Function            entryFunction;   // target function
    M3ContinuationState    state;

    m3slot_t*              val_അട്ടി;        // allocated value stack buffer
    u32                    അക്കം_അട്ടി_Slots;
    m3stack_t              sp;              // current value stack pointer

    M3Frame*               frames;          // native frames recorded by the last suspend
    u32                    framesCap;       // entries allocated
    u32                    അക്കം_Frames;       // entries in use, innermost first

    pc_t                   pc;              // resume instruction pointer
    u8                     suspendPoint;    // M3SafePointKind: what left pc where it is
    m3reg_t                r0;              // saved accumulator
#  എങ്കിൽ d_m3HasFloat
    f64 fp0;
#  endif

    // Bound args count (for cont.bind)
    u32 bound_Args_എണ്ണം;

#  എങ്കിൽ d_m3HasExceptionHandling
    // resume_throw: raised at the suspension point instead of carrying on from
    // it, so the try regions inside the continuation get their turn first
    M3Exception* resumeThrow;
#  endif

    // Where the values go when this continuation is next given some: the slots
    // its own suspend or switch is waiting on.
    u32                    അക്കം_Suspend_Results;
    i32                    suspendResultOffsets[d_m3MaxContinuationPayload];
    u32                    suspendResultIs64[d_m3MaxContinuationPayload];

    // Active handler table from enclosing resume:
    u32                    അക്കം_Handlers;
    pc_t                   handlersPC;
    ഘടന M3Continuation* parent;         // resumer continuation
} M3Continuation, *IM3Continuation;

M3Result        Continuation_Acquire_അട്ടി (IM3Runtime runtime, IM3Continuation cont);
ശൂന്യം            Continuation_Recycle_അട്ടി (IM3Runtime runtime, IM3Continuation cont);
IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function);
IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont);
ശൂന്യം            Continuation_ReleaseAll (IM3Runtime io_runtime);

// Appends one frame to the suspending continuation. Returns the marker the
// caller should keep unwinding with: the suspend marker when the frame was
// recorded, or a trap when it could not be - a continuation that cannot record
// its frames cannot be resumed, so the suspend has to become a trap instead.
m3ret_t         Continuation_RecordFrame (IM3Runtime i_runtime, സ്ഥിരം M3Frame* i_frame);

// Points runtime->stack, where the host's next call puts its arguments, above
// whatever a suspended root continuation holds - or back at the bottom when
// there is none.
ശൂന്യം            Runtime_Place_Call_അട്ടി (IM3Runtime io_runtime);

#endif // d_m3HasStackSwitching


#എങ്കിൽ d_m3HasSnapshots
തരനിർവചനം ഘടന M3EmbeddedSnapshot {
    cstr_t                     name;
    bytes_t                    data;
    u32                        size;
    ഘടന M3EmbeddedSnapshot* next;
} M3EmbeddedSnapshot;
#endif


//---------------------------------------------------------------------------------------------------------------------------------

// One entry of the export section. Export names belong to the module rather than
// to what they name: one entity can be exported under any number of names, and an
// import re-exported under a name of the module's own, whose slot linking points
// at another module's entity. Every lookup by name goes through these, and
// resolves the index against the module's index space at the time it is asked.
തരനിർവചനം ഘടന M3Export {
    cstr_t name;
    u32    nameLength;   // the name may hold a NUL, which ends the C string early
    u32    index;        // into the index space 'kind' names
    u8     kind;         // d_externalKind_*
} M3Export;

തരനിർവചനം ഘടന M3Module {
    ഘടന M3Runtime*     runtime;
    ഘടന M3Environment* environment;

    bytes_t               wasmStart;
    bytes_t               wasmEnd;

    cstr_t                name;

    u32                   അക്കം_Func_Types;
    IM3FuncType*          funcTypes;              // array of pointers to list of FuncTypes

    u32                   അക്കം_Func_Imports;
    u32                   അക്കം_Functions;
    u32                   allFunctions;           // allocated functions count
    M3Function*           functions;

    i32                   startFunction;

    u32                   അക്കം_Data_Segments;
    M3DataSegment*        dataSegments;

    u32                   data_എണ്ണം;          // from the data count section
    bool                  has_Data_എണ്ണം;

    // Bitset of the functions ref.func may name: those exported, or referenced
    // by an element segment or a global initializer. One bit per function.
    u8*                   declaredFuncs;

    //u32                     importedGlobals;
    u32                   അക്കം_Globals;
    M3Global*             globals;

#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching
    u32    അക്കം_Tags;
    M3Tag* tags;
#endif

    u32               അക്കം_Element_Segments;
    M3ElementSegment* elementSegments;
    bytes_t           elementSectionEnd;

    // The module's table index space, imported entries first. Entries are
    // borrowed pointers, individually allocated so a slot can be pointed at a
    // table another module owns - see M3Table.owner, and M3Module.memories,
    // which works the same way.
    IM3Table*         tables;
    u32               അക്കം_Tables;

    // The module's memory index space: imported entries first, then declared
    // ones, exactly as the Wasm index space orders them. Entries are borrowed
    // pointers - see M3Memory.owner.
    IM3Memory*        memories;
    u32               അക്കം_Memories;

    // The interpreter always carries a valid memory header in _mem - the
    // stack-limit check, the backtrace recorder and the call ops all reach the
    // runtime through it. A module that declares no memory still needs one, so
    // it gets this zero-length stand-in rather than a NULL _mem.
    M3Memory          emptyMemory;

    // memories[0], or emptyMemory when there are none. Resolved once by
    // InitMemory - the index space is fixed by then and linking has already
    // repointed whatever it was going to - so the call ops can read it straight
    // instead of branching. NULL until the module is loaded.
    IM3Memory         memory0;

    M3Export*         exports;
    u32               അക്കം_Exports;

#എങ്കിൽ d_m3HasSnapshots
    ഘടന M3EmbeddedSnapshot* snapshots;

    // A snapshot restores into a module none of whose code has run - start
    // function included - and a restore that fails part way leaves one that
    // can no longer run at all: its store is neither the snapshot's nor its own.
    bool                       hasRun;
    bool                       isUnusable;
#endif

    ഘടന M3Module* next;
} M3Module;

M3Result Module_AddMemory (IM3Module io_module, IM3Memory* o_memory, സ്ഥിരം M3MemoryInfo* i_info, bool i_isImported);

// Memory 0 of a module - the one the interpreter's _mem register tracks while
// that module's code is running. Never NULL once the module has been loaded.
സ്ഥിതം വരിയിൽ
IM3Memory Module_Memory0 (IM3Module i_module)
{
    തിരികെ i_module->memory0;
}

സ്ഥിതം വരിയിൽ
M3MemoryHeader* Module_MemoryHeader (IM3Module i_module)
{
    തിരികെ i_module->memory0->mallocated;
}

M3Result Module_AddGlobal (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported);
M3Result Module_AddTable (IM3Module io_module, IM3Table* o_table, സ്ഥിരം M3TableInfo* i_info, bool i_isImported);
#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching
M3Result Module_AddTag (IM3Module io_module, IM3Tag* o_tag, IM3FuncType i_type, bool i_isImported);
#endif
#എങ്കിൽ d_m3_Has_അട്ടി_Switching
IM3FuncType Module_ContTypeOfRef (IM3Module i_module, m3type_t i_refType);
#endif
M3Result    Module_DeclareFunction (IM3Module io_module, u32 i_index);
bool        Module_IsFunctionDeclared (IM3Module i_module, u32 i_index);

M3Result    Module_PreallocFunctions (IM3Module io_module, u32 i_totalFunctions);
M3Result    Module_AddFunction (IM3Module io_module, u32 i_typeIndex, IM3ImportInfo i_importInfo /* can be null */);
IM3Function Module_GetFunction (IM3Module i_module, u32 i_functionIndex);
bool        Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule);

ശൂന്യം        Module_GenerateNames (IM3Module i_module);

ശൂന്യം        FreeImportInfo (M3ImportInfo* i_info);

//---------------------------------------------------------------------------------------------------------------------------------

തരനിർവചനം ഘടന M3Environment {
    //    struct M3Runtime *      runtimes;

    IM3FuncType      funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType      retFuncTypes[c_m3_Type_എണ്ണം];      // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    u16              അക്കം_Func_Types;                       // hands out M3FuncType.canonicalIndex
    M3CodePage*      pagesReleased;

    M3SectionHandler customSectionHandler;
} M3Environment;

ശൂന്യം     Environment_Release (IM3Environment i_environment);

// takes ownership of io_funcType and returns a pointer to the persistent version (could be same or different)
M3Result Environment_AddFuncType (IM3Environment i_environment, IM3FuncType* io_funcType);
M3Result Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_എണ്ണം, u16* o_firstIndex);
ശൂന്യം     Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonicalIndex);

#എങ്കിൽ d_m3HasTypedRefs
M3Result ParseHeapType (IM3Module i_module, m3type_t* o_heapBits, bytes_t* io_bytes, cbytes_t i_end);
#endif
M3Result Parse_മൂല്യം_Type (IM3Module i_module, m3type_t* o_type, bytes_t* io_bytes, cbytes_t i_end);

//---------------------------------------------------------------------------------------------------------------------------------


തരനിർവചനം ഘടന M3Runtime {
    M3Compilation  compilation;

    IM3Environment environment;

    M3CodePage*    pagesOpen;      // linked list of code pages with writable space on them
    M3CodePage*    pagesFull;      // linked list of at-capacity pages

    u32            അക്കം_Code_Pages;
    u32            അക്കം_Active_Code_Pages;

    IM3Module      modules;        // linked list of imported modules

    ശൂന്യം*          അട്ടി;
    ശൂന്യം*          origin_അട്ടി;
    u32            അട്ടി_Size;
    u32            അക്കം_അട്ടി_Slots;
    ശൂന്യം*          അട്ടി_Limit;     // native C-stack low-water mark; Wasm calls trap past it (NULL = unset)
    IM3Function    lastCalled;     // last function that successfully executed
    IM3Function    entered;        // entry point of the outermost call in flight, or of the last one, trapped or not
    u32            callNesting;    // RunCodeChecked() recursion depth

    ശൂന്യം*          userdata;

    u32            memoryLimit;
    u64            memoryBytesLimit;
    u64            memoryBytesUsed;
    u64            tableElementsLimit;
    u64            tableElementsUsed;

#എങ്കിൽ d_m3DeterministicProfile
    // What the host answers about time and entropy out of, rather than out of the
    // machine it happens to be running on - see m3_deterministic.h, which is what
    // reads and moves them. Both are per-runtime, so two runtimes replay
    // independently and a fresh runtime replays the same way as the last one.
    u64 virtualTimeNs;
    u64 randomState;
#endif

#എങ്കിൽ d_m3EnableValidation
    bool    skipValidation; // m3_SetValidation: compile function bodies without type checking them first
    ValCtx* validator;      // created on first use
#endif

#എങ്കിൽ d_m3EnableStrace >= 2
    u32 callDepth;
#endif

#എങ്കിൽ d_m3HasGasMetering
    // Both counts are in gas units (see M3_GAS_UNITS_PER_GAS). gasRemaining goes
    // negative by the cost of the segment that ran out, which is what makes the
    // gas used come out slightly over the limit, and is deliberate: a segment
    // is paid for in full before any of it runs.
    i64 gasLimit;
    i64 gasRemaining;
#endif

#എങ്കിൽ d_m3HasExceptionHandling
    u32          tryDepth;           // number of try regions whose body is executing
    M3Exception* pendingException;   // the exception currently unwinding, if any
    M3Exception* exceptions;         // the ones it still holds
    M3Exception* uncaughtException;  // a copy of what ended the last call, off the list
#endif
#എങ്കിൽ d_m3HasExceptionHandling || d_m3_Has_അട്ടി_Switching
    M3HostTag* hostTags;
#endif

    // Set by the embedder and by the host's interrupt handler, so they are here
    // whether or not the interpreter was built able to act on them. Only the
    // suspendable operations read suspendRequested, and those exist only with
    // stack switching, so without it m3_RequestSuspend is simply never answered.
    bool          isSuspendable;
    അസ്ഥിര bool suspendRequested;

#എങ്കിൽ d_m3_Has_അട്ടി_Switching
    // Set while m3_ResumeRuntime replays a paused invocation: a back edge or a
    // function entry it paused at goes on past its check, so a pause already
    // requested takes effect at the next one instead of on the spot again
    bool            resumePastCheck;

    u32             continuationsLimit;
    u32             continuationsAllocated;
    m3slot_t**      val_അട്ടി_Pool;
    u32             val_അട്ടി_Pool_Cap;
    u32             val_അട്ടി_Pool_എണ്ണം;
    IM3Continuation activeContinuation;
    IM3Continuation suspendedContinuation;
    IM3Continuation continuations;
    IM3Continuation വേര്_Continuation;

    // The suspend on its way out, if there is one. Only ever one at a time -
    // it travels up the native stack and is answered before anything else can
    // start - so this belongs to the runtime rather than to any continuation.
    IM3Tag          suspendTag;
    IM3Continuation suspendHandlerCont;     // whose resume named the tag
    pc_t            suspendStubPC;          // the handler it is headed for
    IM3Continuation switchTarget;           // the peer to run instead, for a switch
    u64             suspendPayload[d_m3MaxContinuationPayload];
    u32             suspendPayloadIs64[d_m3MaxContinuationPayload];
    u32             അക്കം_Suspend_Payload;
#endif

#എങ്കിൽ d_m3HasSnapshots
    M3SnapshotHooks snapshotHooks;
#endif

    M3ErrorInfo error;
#എങ്കിൽ d_m3VerboseErrorMessages
    അക്ഷരം error_message[256]; // the actual buffer. M3ErrorInfo can point to this
#endif

#എങ്കിൽ d_m3RecordBacktraces
    M3BacktraceInfo backtrace;
#endif

    u32 newCodePageSequence;
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
#എങ്കിൽ d_m3_Max_Native_അട്ടി > 0
#  define d_m3_അട്ടി_Limit_Enter(RT)                                               \
        ശൂന്യം * m3_Saved_അട്ടി_Limit = (RT)->അട്ടി_Limit;                           \
        എങ്കിൽ (not (RT)->അട്ടി_Limit)                                               \
            (RT)->അട്ടി_Limit = m3_Native_അട്ടി_Limit(m3_Native_അട്ടി_Ptr(), (d_m3_Max_Native_അട്ടി));
#  define d_m3_അട്ടി_Limit_Leave(RT)  (RT)->അട്ടി_Limit = m3_Saved_അട്ടി_Limit;
#അല്ലെങ്കിൽ
#  define d_m3_അട്ടി_Limit_Enter(RT)
#  define d_m3_അട്ടി_Limit_Leave(RT)
#endif

ശൂന്യം     InitRuntime (IM3Runtime io_runtime, u32 i_അട്ടി_Size_In_Bytes);
ശൂന്യം     Runtime_Release (IM3Runtime io_runtime);

M3Result ResizeMemory (IM3Runtime io_runtime, IM3Memory io_memory, u64 i_അക്കം_Pages);

// Give back whatever is behind io_memory->mallocated, which is the heap or a slot of
// the guarded arena depending on the build. Leaves the M3Memory itself alone.
ശൂന്യം     FreeMemoryBlock (IM3Memory io_memory);

തരനിർവചനം ശൂന്യം* (*ModuleVisitor)(IM3Module i_module, ശൂന്യം* i_info);
ശൂന്യം*       ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, ശൂന്യം* i_info);

ശൂന്യം*       v_FindFunction (IM3Module i_module, ശൂന്യം* i_info);

IM3CodePage AcquireCodePage (IM3Runtime io_runtime);
IM3CodePage AcquireCodePageWithCapacity (IM3Runtime io_runtime, u32 i_വരി_എണ്ണം);
ശൂന്യം        ReleaseCodePage (IM3Runtime io_runtime, IM3CodePage i_codePage);

#എങ്കിൽ d_m3_Has_അട്ടി_Switching
സ്ഥിതം വരിയിൽ
bool Continuation_Can_Acquire_അട്ടി (IM3Runtime runtime)
{
    തിരികെ not runtime->continuationsLimit or
           runtime->continuationsAllocated < runtime->continuationsLimit;
}
#endif

d_m3EndExternC

#endif // m3_env_h
