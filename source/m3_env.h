//
//  m3_env.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#جيڪڏهن_وصف_نه_ٿيل m3_env_h
#وصف_ڪريو m3_env_h

#ضمن "wasm3.h"
#ضمن "m3_code.h"
#ضمن "m3_compile.h"

d_m3BeginExternC

تعريفنوع بنية ValCtx ValCtx;   // the validator's scratch, defined in m3_validate.c


//---------------------------------------------------------------------------------------------------------------------------------

// Page counts are u64 because a memory64 type may declare up to 2^48 pages.
// Nothing near that can be backed, but the module still has to parse and
// validate; it is instantiation that fails.
تعريفنوع بنية M3_يادگيري_Info {
    u64  init_صفحا;
    u64  max_صفحا;
    u32  صفحو_سائيز;
    bool hasMax;         // a declared maximum of 0 is not the same as none
    bool isMemory64;     // addressed by i64 rather than i32
    bool isShared;       // the threads proposal: a maximum is required
} M3_يادگيري_Info;


// One linear memory. Memories belong to the module that declares them, not to
// the runtime: two modules loaded into the same runtime each get their own, and
// the interpreter reaches the one it is currently running against through the
// _mem pseudo-register (see M3MemoryHeader).
//
// Every entry is individually allocated so that a slot may later be pointed at
// a memory another module owns - growing one has to be visible through every
// name it has. 'owner' says which module allocated it, and so which module
// frees it.
تعريفنوع بنية M3_يادگيري {
    M3_يادگيري_سرو*  mallocated;

    u64              انگ_صفحا;
    u64              max_صفحا;
    u64              init_صفحا;
    u32              صفحو_سائيز;
    bool             hasMax;         // see M3MemoryInfo
    bool             isMemory64;     // addressed by i64 rather than i32
    bool             isShared;       // see M3MemoryInfo

    بنية M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;

#إذا d_m3_Guarded_يادگيري
    // The slot of the guarded arena this memory's bytes live in, and so what
    // 'mallocated' points into rather than the heap - see Guard_TakeSlot
    فراغ* guard_جڳهه;
#جيڪڏهن_پورو
} M3_يادگيري;

تعريفنوع M3_يادگيري* IM3_يادگيري;

// The value type a memory's addresses, page counts and lengths are expressed
// in. Every instruction naming the memory takes and returns that type.
ساكن مضمن
m3type_t يادگيري_Addr_Type (ثابت M3_يادگيري* i_يادگيري)
{
    أرجع i_يادگيري->isMemory64 ? c_m3Type_i64 : c_m3Type_i32;
}


// A module that declares no custom page size means the default one. The parser
// leaves that as zero, and InitMemory only fills it in when it allocates, so a
// memory has to be asked rather than read directly until then.
ساكن مضمن
u32 يادگيري_صفحو_سائيز (ثابت M3_يادگيري* i_يادگيري)
{
    أرجع i_يادگيري->صفحو_سائيز ? i_يادگيري->صفحو_سائيز : d_m3_Default_Mem_صفحو_سائيز;
}


//---------------------------------------------------------------------------------------------------------------------------------

// A table type as the module declared it. Sizes stay u32 - a table64 may name
// far more entries than that, but d_m3MaxSaneTableSize refuses anything near
// the limit long before it matters.
تعريفنوع بنية M3_جدول_Info {
    m3type_t elemType;
    u32      init_سائيز;
    u32      max_سائيز;
    bool     hasMax;         // a declared maximum of 0 is not the same as none
    bool     isTable64;      // indexed by i64 rather than i32
} M3_جدول_Info;


// A table's elements are opaque pointer-sized references: IM3Function for a
// funcref table, a host handle for an externref one. NULL is the null reference.
تعريفنوع بنية M3_جدول {
    فراغ**           elements;
    u32              سائيز;
    u32              max_سائيز;            // 0 when the module declared no maximum
    m3type_t         type;

    // Every slot starts out holding this, rather than null. A table whose
    // element type is not nullable has to say what to fill itself with.
    بائيٽون_t          init_اظهار;
    u32              init_اظهار_سائيز;

    u32              init_سائيز;       // the declared minimum
    bool             hasMax;         // a declared maximum of 0 is not the same as none
    bool             isTable64;      // indexed by i64 rather than i32

    بنية M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;
} M3_جدول;

تعريفنوع M3_جدول* IM3_جدول;

// The value type a table's indexes and sizes are expressed in - table64 is the
// same choice as memory64, made per table.
ساكن مضمن
m3type_t جدول_Addr_Type (ثابت M3_جدول* i_جدول)
{
    أرجع i_جدول->isTable64 ? c_m3Type_i64 : c_m3Type_i32;
}

// Element segment modes, from the low two bits of the segment's flags
تعريفنوع تعداد {
    c_m3_Elem_سرگرم      = 0,        // table 0
    c_m3Elem_passive     = 1,
    c_m3_Elem_سرگرم_Idx   = 2,        // explicit table index
    c_m3Elem_declarative = 3
} M3_Element_گهڻائي;

تعريفنوع بنية M3ElementSegment {
    بائيٽون_t  init_اظهار;       // active segments only: the offset
    بائيٽون_t  elements;       // funcidx list, or const exprs when isExpr
    فراغ**   resolved;       // passive segments only, for table.init

    u32      init_اظهار_سائيز;
    u32      انگ_Elements;
    u32      جدول_اشاريو;

    m3type_t type;
    u8       گهڻائي;
    bool     is_اظهار;
    bool     dropped;
} M3ElementSegment;

تعريفنوع بنية M3_معلومات_Segment {
    ثابت u8* init_اظهار;           // wasm code
    ثابت u8* معلومات;

    u32       init_اظهار_سائيز;
    u32       يادگيري_Region;
    u32       سائيز;

    bool      isPassive;
    bool      dropped;            // active segments are dropped once instantiated
} M3_معلومات_Segment;

//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3Global {
    M3ImportInfo     import;

    // An import linked to another module's export: the global that actually
    // holds the value. Reads and writes go there, so a mutable global stays one
    // cell seen under two names. NULL for anything else, including an import a
    // host supplied a value for through m3_LinkGlobal.
    بنية M3Global* resolved;

    اتحاد {
        i32 i32_قدر;
        i64 i64_قدر;
#إذا d_m3HasFloat
        f64 f64_قدر;
        f32 f32_قدر;
#جيڪڏهن_پورو
        فراغ* ref_قدر;
    };

    بائيٽون_t  init_اظهار;       // wasm code
    u32      init_اظهار_سائيز;
    m3type_t type;
    bool     imported;
    bool     isMutable;
} M3Global;


//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching

// An exception / control tag, as the tag section declares it. Tags are compared by
// identity, and this struct's address is that identity.
تعريفنوع بنية M3_ٽيگ {
    M3ImportInfo  import;
    IM3FuncType   type;           // params are the payload; results are empty for EH, can be non-empty for stack switching
    بنية M3_ٽيگ* resolved;
    bool          imported;
} M3_ٽيگ;

// A tag m3_NewTag made. It belongs to the runtime rather than to a module, and
// is linked in front of the runtime's others rather than kept in an array, which
// would move the tags - and with them their identities - whenever it grew.
تعريفنوع بنية M3_Host_ٽيگ {
    بنية M3_Host_ٽيگ* ايندڙ;
    M3_ٽيگ             ٽيگ;
} M3_Host_ٽيگ;

#جيڪڏهن_پورو // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3HasExceptionHandling

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
تعريفنوع بنية M3Exception {
    بنية M3Exception* ايندڙ;           // the runtime's allocation list,
    بنية M3Exception* prev;           //   doubly linked so one can leave early
    IM3_ٽيگ              ٽيگ;
    u32                 انگ_دليلون;
    bool                reified;        // an exnref to this has been handed out
    u64                 دليلون[];
} M3Exception;


M3Exception* NewException (IM3Runtime io_runtime, IM3_ٽيگ i_ٽيگ, u32 i_انگ_دليلون);
فراغ         FreeException (IM3Runtime io_runtime, M3Exception* i_exception);
فراغ         FreeExceptions (IM3Runtime io_runtime);

#جيڪڏهن_پورو // d_m3HasExceptionHandling


#إذا d_m3_Has_ڍير_Switching

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
تعريفنوع تعداد M3_چوکٺو_قسم {
    چوکٺو_call,          // op_Call / op_CallRef / op_CallIndirect
    چوکٺو_loop,          // op_Loop
#  إذا d_m3HasExceptionHandling
    چوکٺو_try,           // op_TryTable
#  endif
#  إذا d_m3_داخلا_Keeps_چوکٺو
    چوکٺو_داخلا,         // op_Entry, in builds where it keeps a frame
#  endif
    چوکٺو_resume,        // op_Resume, when the suspend is not its to answer
} M3_چوکٺو_قسم;

تعريفنوع بنية M3_چوکٺو {
    u8        قسم;      // M3FrameKind
    pc_t      pc;        // call: the return address | loop: the loop id, which is
                         // also its body | try: the clause table | entry: unused
    m3stack_t sp;
    IM3_يادگيري يادگيري;    // _mem is re-derived from this rather than stored:
                         // growing linear memory moves the header, so only the
                         // M3Memory stays valid across a suspend

    اتحاد {
        // op_Call resumes the caller with the registers its own native frame was
        // holding, so those belong to the frame, not to the suspension point
        بنية {
            IM3Function function;
            m3reg_t     r0;
#  إذا d_m3HasFloat
            f64 fp0;
#  endif
        } call;
#  إذا d_m3HasExceptionHandling
        بنية {
            u32  انگ_Clauses;
            bool handlers_جيئرو;   // false once op_PopHandlers has retired the region
        } try_;
#  endif
#  إذا d_m3_داخلا_Keeps_چوکٺو
        بنية {
            IM3Function function;
        } داخلا;
#  endif
        // A resume that the suspend travelled straight through, because the
        // tag was not one its handlers named. The continuation it was running
        // is part of what got captured, so everything needed to set it going
        // again under the same handlers is kept here.
        بنية {
            بنية M3Continuation* cont;
            pc_t                   handlersPC;
            u32                    انگ_Handlers;
            pc_t                   resultsPC;
            u32                    انگ_Results;
        } resume;
    };
} M3_چوکٺو;

// Which of the three resume forms an op_Resume was emitted for
تعداد {
    d_m3_Resume_عام = 0,
    d_m3ResumeThrow,
    d_m3ResumeThrowRef,
};

تعريفنوع تعداد M3ContinuationState {
    cont_allocated = 0,
    cont_running,
    cont_suspended,
    cont_returned,
    cont_consumed
} M3ContinuationState;

تعريفنوع بنية M3ResumeHandler {
    u8     قسم;        // 0: (on $t $h), 1: (on $t switch)
    IM3_ٽيگ ٽيگ;
    pc_t   stubPC;
} M3ResumeHandler;

تعريفنوع بنية M3Continuation {
    بنية M3Continuation* ايندڙ;           // linked list in runtime for cleanup
    IM3FuncType            type;            // continuation type (cont $ft)
    IM3Function            داخلا_Function;   // target function
    M3ContinuationState    state;

    m3slot_t*              val_ڍير;        // allocated value stack buffer
    u32                    انگ_ڍير_Slots;
    m3stack_t              sp;              // current value stack pointer

    M3_چوکٺو*               چوکٺا;          // native frames recorded by the last suspend
    u32                    چوکٺا_Cap;       // entries allocated
    u32                    انگ_چوکٺا;       // entries in use, innermost first

    pc_t                   pc;              // resume instruction pointer
    u8                     suspend_نقطو;    // M3SafePointKind: what left pc where it is
    m3reg_t                r0;              // saved accumulator
#  إذا d_m3HasFloat
    f64 fp0;
#  endif

    // Bound args count (for cont.bind)
    u32 bound_دليلون_ڳڻپ;

#  إذا d_m3HasExceptionHandling
    // resume_throw: raised at the suspension point instead of carrying on from
    // it, so the try regions inside the continuation get their turn first
    M3Exception* resumeThrow;
#  endif

    // Where the values go when this continuation is next given some: the slots
    // its own suspend or switch is waiting on.
    u32                    انگ_Suspend_Results;
    i32                    suspend_نتيجو_Offsets[d_m3MaxContinuationPayload];
    u32                    suspend_نتيجو_Is64[d_m3MaxContinuationPayload];

    // Active handler table from enclosing resume:
    u32                    انگ_Handlers;
    pc_t                   handlersPC;
    بنية M3Continuation* والد;         // resumer continuation
} M3Continuation, *IM3Continuation;

M3_نتيجو        Continuation_Acquire_ڍير (IM3Runtime runtime, IM3Continuation cont);
فراغ            Continuation_Recycle_ڍير (IM3Runtime runtime, IM3Continuation cont);
IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function);
IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont);
فراغ            Continuation_ReleaseAll (IM3Runtime io_runtime);

// Appends one frame to the suspending continuation. Returns the marker the
// caller should keep unwinding with: the suspend marker when the frame was
// recorded, or a trap when it could not be - a continuation that cannot record
// its frames cannot be resumed, so the suspend has to become a trap instead.
m3ret_t         Continuation_رڪارڊ_چوکٺو (IM3Runtime i_runtime, ثابت M3_چوکٺو* i_چوکٺو);

// Points runtime->stack, where the host's next call puts its arguments, above
// whatever a suspended root continuation holds - or back at the bottom when
// there is none.
فراغ            Runtime_رکو_Call_ڍير (IM3Runtime io_runtime);

#جيڪڏهن_پورو // d_m3HasStackSwitching


#إذا d_m3HasSnapshots
تعريفنوع بنية M3EmbeddedSnapshot {
    cstr_t                     name;
    بائيٽون_t                    معلومات;
    u32                        سائيز;
    بنية M3EmbeddedSnapshot* ايندڙ;
} M3EmbeddedSnapshot;
#جيڪڏهن_پورو


//---------------------------------------------------------------------------------------------------------------------------------

// One entry of the export section. Export names belong to the module rather than
// to what they name: one entity can be exported under any number of names, and an
// import re-exported under a name of the module's own, whose slot linking points
// at another module's entity. Every lookup by name goes through these, and
// resolves the index against the module's index space at the time it is asked.
تعريفنوع بنية M3Export {
    cstr_t name;
    u32    name_ڊيگهائي;   // the name may hold a NUL, which ends the C string early
    u32    index;        // into the index space 'kind' names
    u8     قسم;         // d_externalKind_*
} M3Export;

تعريفنوع بنية M3Module {
    بنية M3Runtime*     runtime;
    بنية M3Environment* environment;

    بائيٽون_t               wasm_شروعات;
    بائيٽون_t               wasm_پڇاڙي;

    cstr_t                name;

    u32                   انگ_Func_Types;
    IM3FuncType*          funcTypes;              // array of pointers to list of FuncTypes

    u32                   انگ_Func_Imports;
    u32                   انگ_Functions;
    u32                   allFunctions;           // allocated functions count
    M3Function*           functions;

    i32                   شروعات_Function;

    u32                   انگ_معلومات_Segments;
    M3_معلومات_Segment*        معلومات_Segments;

    u32                   معلومات_ڳڻپ;          // from the data count section
    bool                  has_معلومات_ڳڻپ;

    // Bitset of the functions ref.func may name: those exported, or referenced
    // by an element segment or a global initializer. One bit per function.
    u8*                   declaredFuncs;

    //u32                     importedGlobals;
    u32                   انگ_Globals;
    M3Global*             globals;

#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
    u32    انگ_ٽيگ_فهرست;
    M3_ٽيگ* ٽيگ_فهرست;
#جيڪڏهن_پورو

    u32               انگ_Element_Segments;
    M3ElementSegment* elementSegments;
    بائيٽون_t           element_Section_پڇاڙي;

    // The module's table index space, imported entries first. Entries are
    // borrowed pointers, individually allocated so a slot can be pointed at a
    // table another module owns - see M3Table.owner, and M3Module.memories,
    // which works the same way.
    IM3_جدول*         tables;
    u32               انگ_Tables;

    // The module's memory index space: imported entries first, then declared
    // ones, exactly as the Wasm index space orders them. Entries are borrowed
    // pointers - see M3Memory.owner.
    IM3_يادگيري*        memories;
    u32               انگ_Memories;

    // The interpreter always carries a valid memory header in _mem - the
    // stack-limit check, the backtrace recorder and the call ops all reach the
    // runtime through it. A module that declares no memory still needs one, so
    // it gets this zero-length stand-in rather than a NULL _mem.
    M3_يادگيري          خالي_جايون_يادگيري;

    // memories[0], or emptyMemory when there are none. Resolved once by
    // InitMemory - the index space is fixed by then and linking has already
    // repointed whatever it was going to - so the call ops can read it straight
    // instead of branching. NULL until the module is loaded.
    IM3_يادگيري         memory0;

    M3Export*         exports;
    u32               انگ_Exports;

#إذا d_m3HasSnapshots
    بنية M3EmbeddedSnapshot* snapshots;

    // A snapshot restores into a module none of whose code has run - start
    // function included - and a restore that fails part way leaves one that
    // can no longer run at all: its store is neither the snapshot's nor its own.
    bool                       has_لاڳيتو;
    bool                       isUnusable;
#جيڪڏهن_پورو

    بنية M3Module* ايندڙ;
} M3Module;

M3_نتيجو Module_جوڙيو_يادگيري (IM3Module io_module, IM3_يادگيري* o_يادگيري, ثابت M3_يادگيري_Info* i_info, bool i_isImported);

// Memory 0 of a module - the one the interpreter's _mem register tracks while
// that module's code is running. Never NULL once the module has been loaded.
ساكن مضمن
IM3_يادگيري Module_Memory0 (IM3Module i_module)
{
    أرجع i_module->memory0;
}

ساكن مضمن
M3_يادگيري_سرو* Module_يادگيري_سرو (IM3Module i_module)
{
    أرجع i_module->memory0->mallocated;
}

M3_نتيجو Module_جوڙيو_Global (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported);
M3_نتيجو Module_جوڙيو_جدول (IM3Module io_module, IM3_جدول* o_جدول, ثابت M3_جدول_Info* i_info, bool i_isImported);
#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
M3_نتيجو Module_جوڙيو_ٽيگ (IM3Module io_module, IM3_ٽيگ* o_ٽيگ, IM3FuncType i_type, bool i_isImported);
#جيڪڏهن_پورو
#إذا d_m3_Has_ڍير_Switching
IM3FuncType Module_Cont_Type_جو_Ref (IM3Module i_module, m3type_t i_refType);
#جيڪڏهن_پورو
M3_نتيجو    Module_DeclareFunction (IM3Module io_module, u32 i_اشاريو);
bool        Module_IsFunctionDeclared (IM3Module i_module, u32 i_اشاريو);

M3_نتيجو    Module_PreallocFunctions (IM3Module io_module, u32 i_ڪل_Functions);
M3_نتيجو    Module_جوڙيو_Function (IM3Module io_module, u32 i_type_اشاريو, IM3ImportInfo i_importInfo /* can be null */);
IM3Function Module_GetFunction (IM3Module i_module, u32 i_function_اشاريو);
bool        Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule);

فراغ        Module_Generate_نالا (IM3Module i_module);

فراغ        FreeImportInfo (M3ImportInfo* i_info);

//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3Environment {
    //    struct M3Runtime *      runtimes;

    IM3FuncType      funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType      retFuncTypes[c_m3_Type_ڳڻپ];      // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    u16              انگ_Func_Types;                       // hands out M3FuncType.canonicalIndex
    M3_ڪوڊ_صفحو*      صفحا_Released;

    M3SectionHandler customSectionHandler;
} M3Environment;

فراغ     Environment_Release (IM3Environment i_environment);

// takes ownership of io_funcType and returns a pointer to the persistent version (could be same or different)
M3_نتيجو Environment_جوڙيو_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType);
M3_نتيجو Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_ڳڻپ, u16* o_پهريون_اشاريو);
فراغ     Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_اشاريو);

#إذا d_m3HasTypedRefs
M3_نتيجو Parse_ڍير_Type (IM3Module i_module, m3type_t* o_ڍير_بٽ, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي);
#جيڪڏهن_پورو
M3_نتيجو Parse_قدر_Type (IM3Module i_module, m3type_t* o_type, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي);

//---------------------------------------------------------------------------------------------------------------------------------


تعريفنوع بنية M3Runtime {
    M3Compilation  compilation;

    IM3Environment environment;

    M3_ڪوڊ_صفحو*    صفحا_Open;      // linked list of code pages with writable space on them
    M3_ڪوڊ_صفحو*    صفحا_ڀريل;      // linked list of at-capacity pages

    u32            انگ_ڪوڊ_صفحا;
    u32            انگ_سرگرم_ڪوڊ_صفحا;

    IM3Module      modules;        // linked list of imported modules

    فراغ*          ڍير;
    فراغ*          origin_ڍير;
    u32            ڍير_سائيز;
    u32            انگ_ڍير_Slots;
    فراغ*          ڍير_حد_بندي;     // native C-stack low-water mark; Wasm calls trap past it (NULL = unset)
    IM3Function    آخري_Called;     // last function that successfully executed
    IM3Function    entered;        // entry point of the outermost call in flight, or of the last one, trapped or not
    u32            callNesting;    // RunCodeChecked() recursion depth

    فراغ*          userdata;

    u32            يادگيري_حد_بندي;
    u64            يادگيري_بائيٽون_حد_بندي;
    u64            يادگيري_بائيٽون_استعمال_ٿيل;
    u64            جدول_Elements_حد_بندي;
    u64            جدول_Elements_استعمال_ٿيل;

#إذا d_m3DeterministicProfile
    // What the host answers about time and entropy out of, rather than out of the
    // machine it happens to be running on - see m3_deterministic.h, which is what
    // reads and moves them. Both are per-runtime, so two runtimes replay
    // independently and a fresh runtime replays the same way as the last one.
    u64 virtual_وقت_Ns;
    u64 randomState;
#جيڪڏهن_پورو

#إذا d_m3EnableValidation
    bool    skipValidation; // m3_SetValidation: compile function bodies without type checking them first
    ValCtx* validator;      // created on first use
#جيڪڏهن_پورو

#إذا d_m3EnableStrace >= 2
    u32 call_اونهائي;
#جيڪڏهن_پورو

#إذا d_m3HasGasMetering
    // Both counts are in gas units (see M3_GAS_UNITS_PER_GAS). gasRemaining goes
    // negative by the cost of the segment that ran out, which is what makes the
    // gas used come out slightly over the limit, and is deliberate: a segment
    // is paid for in full before any of it runs.
    i64 gas_حد_بندي;
    i64 gas_بچيل;
#جيڪڏهن_پورو

#إذا d_m3HasExceptionHandling
    u32          try_اونهائي;           // number of try regions whose body is executing
    M3Exception* pendingException;   // the exception currently unwinding, if any
    M3Exception* exceptions;         // the ones it still holds
    M3Exception* uncaughtException;  // a copy of what ended the last call, off the list
#جيڪڏهن_پورو
#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
    M3_Host_ٽيگ* host_ٽيگ_فهرست;
#جيڪڏهن_پورو

    // Set by the embedder and by the host's interrupt handler, so they are here
    // whether or not the interpreter was built able to act on them. Only the
    // suspendable operations read suspendRequested, and those exist only with
    // stack switching, so without it m3_RequestSuspend is simply never answered.
    bool          isSuspendable;
    متطاير bool suspendRequested;

#إذا d_m3_Has_ڍير_Switching
    // Set while m3_ResumeRuntime replays a paused invocation: a back edge or a
    // function entry it paused at goes on past its check, so a pause already
    // requested takes effect at the next one instead of on the spot again
    bool            resumePastCheck;

    u32             continuations_حد_بندي;
    u32             continuationsAllocated;
    m3slot_t**      val_ڍير_Pool;
    u32             val_ڍير_Pool_Cap;
    u32             val_ڍير_Pool_ڳڻپ;
    IM3Continuation سرگرم_Continuation;
    IM3Continuation suspendedContinuation;
    IM3Continuation continuations;
    IM3Continuation پاڙ_Continuation;

    // The suspend on its way out, if there is one. Only ever one at a time -
    // it travels up the native stack and is answered before anything else can
    // start - so this belongs to the runtime rather than to any continuation.
    IM3_ٽيگ          suspend_ٽيگ;
    IM3Continuation suspendHandlerCont;     // whose resume named the tag
    pc_t            suspendStubPC;          // the handler it is headed for
    IM3Continuation switch_هدف;           // the peer to run instead, for a switch
    u64             suspendPayload[d_m3MaxContinuationPayload];
    u32             suspendPayloadIs64[d_m3MaxContinuationPayload];
    u32             انگ_Suspend_Payload;
#جيڪڏهن_پورو

#إذا d_m3HasSnapshots
    M3SnapshotHooks snapshotHooks;
#جيڪڏهن_پورو

    M3ErrorInfo error;
#إذا d_m3VerboseErrorMessages
    حرف error_پيغام[256]; // the actual buffer. M3ErrorInfo can point to this
#جيڪڏهن_پورو

#إذا d_m3_رڪارڊ_Backtraces
    M3BacktraceInfo backtrace;
#جيڪڏهن_پورو

    u32 new_ڪوڊ_صفحو_Sequence;
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
#إذا d_m3_Max_Native_ڍير > 0
#  define d_m3_ڍير_حد_بندي_Enter(RT)                                               \
        فراغ * m3_Saved_ڍير_حد_بندي = (RT)->ڍير_حد_بندي;                           \
        إذا (not (RT)->ڍير_حد_بندي)                                               \
            (RT)->ڍير_حد_بندي = m3_Native_ڍير_حد_بندي(m3_Native_ڍير_Ptr(), (d_m3_Max_Native_ڍير));
#  define d_m3_ڍير_حد_بندي_Leave(RT)  (RT)->ڍير_حد_بندي = m3_Saved_ڍير_حد_بندي;
#وإلا
#  define d_m3_ڍير_حد_بندي_Enter(RT)
#  define d_m3_ڍير_حد_بندي_Leave(RT)
#جيڪڏهن_پورو

فراغ     InitRuntime (IM3Runtime io_runtime, u32 i_ڍير_سائيز_اندريون_بائيٽون);
فراغ     Runtime_Release (IM3Runtime io_runtime);

M3_نتيجو Resize_يادگيري (IM3Runtime io_runtime, IM3_يادگيري io_يادگيري, u64 i_انگ_صفحا);

// Give back whatever is behind io_memory->mallocated, which is the heap or a slot of
// the guarded arena depending on the build. Leaves the M3Memory itself alone.
فراغ     Free_يادگيري_بلاڪ (IM3_يادگيري io_يادگيري);

تعريفنوع فراغ* (*ModuleVisitor)(IM3Module i_module, فراغ* i_info);
فراغ*       ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, فراغ* i_info);

فراغ*       v_ڳوليو_Function (IM3Module i_module, فراغ* i_info);

IM3_ڪوڊ_صفحو Acquire_ڪوڊ_صفحو (IM3Runtime io_runtime);
IM3_ڪوڊ_صفحو Acquire_ڪوڊ_صفحو_With_صلاحيت (IM3Runtime io_runtime, u32 i_سٽ_ڳڻپ);
فراغ        Release_ڪوڊ_صفحو (IM3Runtime io_runtime, IM3_ڪوڊ_صفحو i_ڪوڊ_صفحو);

#إذا d_m3_Has_ڍير_Switching
ساكن مضمن
bool Continuation_Can_Acquire_ڍير (IM3Runtime runtime)
{
    أرجع not runtime->continuations_حد_بندي or
           runtime->continuationsAllocated < runtime->continuations_حد_بندي;
}
#جيڪڏهن_پورو

d_m3_پڇاڙي_Extern_C

#جيڪڏهن_پورو // m3_env_h
