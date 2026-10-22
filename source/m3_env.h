//
//  m3_env.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#که_نه_تعریف_شوی m3_env_h
#تعریف_کړه m3_env_h

#ضمن "wasm3.h"
#ضمن "m3_code.h"
#ضمن "m3_compile.h"

d_m3BeginExternC

تعريفنوع بنية ValCtx ValCtx;   // the validator's scratch, defined in m3_validate.c


//---------------------------------------------------------------------------------------------------------------------------------

// Page counts are u64 because a memory64 type may declare up to 2^48 pages.
// Nothing near that can be backed, but the module still has to parse and
// validate; it is instantiation that fails.
تعريفنوع بنية M3_حافظه_Info {
    u64  init_پاڼې;
    u64  max_پاڼې;
    u32  پاڼه_اندازه;
    bool hasMax;         // a declared maximum of 0 is not the same as none
    bool isMemory64;     // addressed by i64 rather than i32
    bool isShared;       // the threads proposal: a maximum is required
} M3_حافظه_Info;


// One linear memory. Memories belong to the module that declares them, not to
// the runtime: two modules loaded into the same runtime each get their own, and
// the interpreter reaches the one it is currently running against through the
// _mem pseudo-register (see M3MemoryHeader).
//
// Every entry is individually allocated so that a slot may later be pointed at
// a memory another module owns - growing one has to be visible through every
// name it has. 'owner' says which module allocated it, and so which module
// frees it.
تعريفنوع بنية M3_حافظه {
    M3_حافظه_سرلیک*  mallocated;

    u64              عدد_پاڼې;
    u64              max_پاڼې;
    u64              init_پاڼې;
    u32              پاڼه_اندازه;
    bool             hasMax;         // see M3MemoryInfo
    bool             isMemory64;     // addressed by i64 rather than i32
    bool             isShared;       // see M3MemoryInfo

    بنية M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;

#إذا d_m3_Guarded_حافظه
    // The slot of the guarded arena this memory's bytes live in, and so what
    // 'mallocated' points into rather than the heap - see Guard_TakeSlot
    فراغ* guard_خونه;
#د_که_پای
} M3_حافظه;

تعريفنوع M3_حافظه* IM3_حافظه;

// The value type a memory's addresses, page counts and lengths are expressed
// in. Every instruction naming the memory takes and returns that type.
ساكن مضمن
m3type_t حافظه_Addr_Type (ثابت M3_حافظه* i_حافظه)
{
    أرجع i_حافظه->isMemory64 ? c_m3Type_i64 : c_m3Type_i32;
}


// A module that declares no custom page size means the default one. The parser
// leaves that as zero, and InitMemory only fills it in when it allocates, so a
// memory has to be asked rather than read directly until then.
ساكن مضمن
u32 حافظه_پاڼه_اندازه (ثابت M3_حافظه* i_حافظه)
{
    أرجع i_حافظه->پاڼه_اندازه ? i_حافظه->پاڼه_اندازه : d_m3_Default_Mem_پاڼه_اندازه;
}


//---------------------------------------------------------------------------------------------------------------------------------

// A table type as the module declared it. Sizes stay u32 - a table64 may name
// far more entries than that, but d_m3MaxSaneTableSize refuses anything near
// the limit long before it matters.
تعريفنوع بنية M3_جدول_Info {
    m3type_t elemType;
    u32      init_اندازه;
    u32      max_اندازه;
    bool     hasMax;         // a declared maximum of 0 is not the same as none
    bool     isTable64;      // indexed by i64 rather than i32
} M3_جدول_Info;


// A table's elements are opaque pointer-sized references: IM3Function for a
// funcref table, a host handle for an externref one. NULL is the null reference.
تعريفنوع بنية M3_جدول {
    فراغ**           elements;
    u32              اندازه;
    u32              max_اندازه;            // 0 when the module declared no maximum
    m3type_t         type;

    // Every slot starts out holding this, rather than null. A table whose
    // element type is not nullable has to say what to fill itself with.
    بایټونه_t          init_بیان;
    u32              init_بیان_اندازه;

    u32              init_اندازه;       // the declared minimum
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
    c_m3_Elem_فعال      = 0,        // table 0
    c_m3Elem_passive     = 1,
    c_m3_Elem_فعال_Idx   = 2,        // explicit table index
    c_m3Elem_declarative = 3
} M3_Element_ډېرپېښ;

تعريفنوع بنية M3ElementSegment {
    بایټونه_t  init_بیان;       // active segments only: the offset
    بایټونه_t  elements;       // funcidx list, or const exprs when isExpr
    فراغ**   resolved;       // passive segments only, for table.init

    u32      init_بیان_اندازه;
    u32      عدد_Elements;
    u32      جدول_شاخص;

    m3type_t type;
    u8       ډېرپېښ;
    bool     is_بیان;
    bool     dropped;
} M3ElementSegment;

تعريفنوع بنية M3_معلومات_Segment {
    ثابت u8* init_بیان;           // wasm code
    ثابت u8* معلومات;

    u32       init_بیان_اندازه;
    u32       حافظه_Region;
    u32       اندازه;

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
        i32 i32_ارزښت;
        i64 i64_ارزښت;
#إذا d_m3HasFloat
        f64 f64_ارزښت;
        f32 f32_ارزښت;
#د_که_پای
        فراغ* ref_ارزښت;
    };

    بایټونه_t  init_بیان;       // wasm code
    u32      init_بیان_اندازه;
    m3type_t type;
    bool     imported;
    bool     isMutable;
} M3Global;


//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching

// An exception / control tag, as the tag section declares it. Tags are compared by
// identity, and this struct's address is that identity.
تعريفنوع بنية M3_نښان {
    M3ImportInfo  import;
    IM3FuncType   type;           // params are the payload; results are empty for EH, can be non-empty for stack switching
    بنية M3_نښان* resolved;
    bool          imported;
} M3_نښان;

// A tag m3_NewTag made. It belongs to the runtime rather than to a module, and
// is linked in front of the runtime's others rather than kept in an array, which
// would move the tags - and with them their identities - whenever it grew.
تعريفنوع بنية M3_Host_نښان {
    بنية M3_Host_نښان* بل;
    M3_نښان             نښان;
} M3_Host_نښان;

#د_که_پای // d_m3HasExceptionHandling || d_m3HasStackSwitching


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
    بنية M3Exception* بل;           // the runtime's allocation list,
    بنية M3Exception* prev;           //   doubly linked so one can leave early
    IM3_نښان              نښان;
    u32                 عدد_دلیلونه;
    bool                reified;        // an exnref to this has been handed out
    u64                 دلیلونه[];
} M3Exception;


M3Exception* NewException (IM3Runtime io_runtime, IM3_نښان i_نښان, u32 i_عدد_دلیلونه);
فراغ         FreeException (IM3Runtime io_runtime, M3Exception* i_exception);
فراغ         FreeExceptions (IM3Runtime io_runtime);

#د_که_پای // d_m3HasExceptionHandling


#إذا d_m3_Has_ډېری_Switching

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
تعريفنوع تعداد M3_چوکاټ_ډول {
    چوکاټ_call,          // op_Call / op_CallRef / op_CallIndirect
    چوکاټ_loop,          // op_Loop
#  إذا d_m3HasExceptionHandling
    چوکاټ_try,           // op_TryTable
#  endif
#  إذا d_m3_ثبت_Keeps_چوکاټ
    چوکاټ_ثبت,         // op_Entry, in builds where it keeps a frame
#  endif
    چوکاټ_resume,        // op_Resume, when the suspend is not its to answer
} M3_چوکاټ_ډول;

تعريفنوع بنية M3_چوکاټ {
    u8        ډول;      // M3FrameKind
    pc_t      pc;        // call: the return address | loop: the loop id, which is
                         // also its body | try: the clause table | entry: unused
    m3stack_t sp;
    IM3_حافظه حافظه;    // _mem is re-derived from this rather than stored:
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
            u32  عدد_Clauses;
            bool handlers_ژوندی;   // false once op_PopHandlers has retired the region
        } try_;
#  endif
#  إذا d_m3_ثبت_Keeps_چوکاټ
        بنية {
            IM3Function function;
        } ثبت;
#  endif
        // A resume that the suspend travelled straight through, because the
        // tag was not one its handlers named. The continuation it was running
        // is part of what got captured, so everything needed to set it going
        // again under the same handlers is kept here.
        بنية {
            بنية M3Continuation* cont;
            pc_t                   handlersPC;
            u32                    عدد_Handlers;
            pc_t                   resultsPC;
            u32                    عدد_Results;
        } resume;
    };
} M3_چوکاټ;

// Which of the three resume forms an op_Resume was emitted for
تعداد {
    d_m3_Resume_نورمال = 0,
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
    u8     ډول;        // 0: (on $t $h), 1: (on $t switch)
    IM3_نښان نښان;
    pc_t   stubPC;
} M3ResumeHandler;

تعريفنوع بنية M3Continuation {
    بنية M3Continuation* بل;           // linked list in runtime for cleanup
    IM3FuncType            type;            // continuation type (cont $ft)
    IM3Function            ثبت_Function;   // target function
    M3ContinuationState    state;

    m3slot_t*              val_ډېری;        // allocated value stack buffer
    u32                    عدد_ډېری_Slots;
    m3stack_t              sp;              // current value stack pointer

    M3_چوکاټ*               چوکاټونه;          // native frames recorded by the last suspend
    u32                    چوکاټونه_Cap;       // entries allocated
    u32                    عدد_چوکاټونه;       // entries in use, innermost first

    pc_t                   pc;              // resume instruction pointer
    u8                     suspend_ټکی;    // M3SafePointKind: what left pc where it is
    m3reg_t                r0;              // saved accumulator
#  إذا d_m3HasFloat
    f64 fp0;
#  endif

    // Bound args count (for cont.bind)
    u32 bound_دلیلونه_شمېر;

#  إذا d_m3HasExceptionHandling
    // resume_throw: raised at the suspension point instead of carrying on from
    // it, so the try regions inside the continuation get their turn first
    M3Exception* resumeThrow;
#  endif

    // Where the values go when this continuation is next given some: the slots
    // its own suspend or switch is waiting on.
    u32                    عدد_Suspend_Results;
    i32                    suspend_پایله_Offsets[d_m3MaxContinuationPayload];
    u32                    suspend_پایله_Is64[d_m3MaxContinuationPayload];

    // Active handler table from enclosing resume:
    u32                    عدد_Handlers;
    pc_t                   handlersPC;
    بنية M3Continuation* پلار;         // resumer continuation
} M3Continuation, *IM3Continuation;

M3_پایله        Continuation_Acquire_ډېری (IM3Runtime runtime, IM3Continuation cont);
فراغ            Continuation_Recycle_ډېری (IM3Runtime runtime, IM3Continuation cont);
IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function);
IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont);
فراغ            Continuation_ReleaseAll (IM3Runtime io_runtime);

// Appends one frame to the suspending continuation. Returns the marker the
// caller should keep unwinding with: the suspend marker when the frame was
// recorded, or a trap when it could not be - a continuation that cannot record
// its frames cannot be resumed, so the suspend has to become a trap instead.
m3ret_t         Continuation_ریکارډ_چوکاټ (IM3Runtime i_runtime, ثابت M3_چوکاټ* i_چوکاټ);

// Points runtime->stack, where the host's next call puts its arguments, above
// whatever a suspended root continuation holds - or back at the bottom when
// there is none.
فراغ            Runtime_ځای_پرځای_کړه_Call_ډېری (IM3Runtime io_runtime);

#د_که_پای // d_m3HasStackSwitching


#إذا d_m3HasSnapshots
تعريفنوع بنية M3EmbeddedSnapshot {
    cstr_t                     name;
    بایټونه_t                    معلومات;
    u32                        اندازه;
    بنية M3EmbeddedSnapshot* بل;
} M3EmbeddedSnapshot;
#د_که_پای


//---------------------------------------------------------------------------------------------------------------------------------

// One entry of the export section. Export names belong to the module rather than
// to what they name: one entity can be exported under any number of names, and an
// import re-exported under a name of the module's own, whose slot linking points
// at another module's entity. Every lookup by name goes through these, and
// resolves the index against the module's index space at the time it is asked.
تعريفنوع بنية M3Export {
    cstr_t name;
    u32    name_اوږدتیا;   // the name may hold a NUL, which ends the C string early
    u32    index;        // into the index space 'kind' names
    u8     ډول;         // d_externalKind_*
} M3Export;

تعريفنوع بنية M3Module {
    بنية M3Runtime*     runtime;
    بنية M3Environment* environment;

    بایټونه_t               wasm_پیل;
    بایټونه_t               wasm_پای_ټکی;

    cstr_t                name;

    u32                   عدد_Func_Types;
    IM3FuncType*          funcTypes;              // array of pointers to list of FuncTypes

    u32                   عدد_Func_Imports;
    u32                   عدد_Functions;
    u32                   allFunctions;           // allocated functions count
    M3Function*           functions;

    i32                   پیل_Function;

    u32                   عدد_معلومات_Segments;
    M3_معلومات_Segment*        معلومات_Segments;

    u32                   معلومات_شمېر;          // from the data count section
    bool                  has_معلومات_شمېر;

    // Bitset of the functions ref.func may name: those exported, or referenced
    // by an element segment or a global initializer. One bit per function.
    u8*                   declaredFuncs;

    //u32                     importedGlobals;
    u32                   عدد_Globals;
    M3Global*             globals;

#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
    u32    عدد_نښانونه;
    M3_نښان* نښانونه;
#د_که_پای

    u32               عدد_Element_Segments;
    M3ElementSegment* elementSegments;
    بایټونه_t           element_Section_پای_ټکی;

    // The module's table index space, imported entries first. Entries are
    // borrowed pointers, individually allocated so a slot can be pointed at a
    // table another module owns - see M3Table.owner, and M3Module.memories,
    // which works the same way.
    IM3_جدول*         tables;
    u32               عدد_Tables;

    // The module's memory index space: imported entries first, then declared
    // ones, exactly as the Wasm index space orders them. Entries are borrowed
    // pointers - see M3Memory.owner.
    IM3_حافظه*        memories;
    u32               عدد_Memories;

    // The interpreter always carries a valid memory header in _mem - the
    // stack-limit check, the backtrace recorder and the call ops all reach the
    // runtime through it. A module that declares no memory still needs one, so
    // it gets this zero-length stand-in rather than a NULL _mem.
    M3_حافظه          تشه_حافظه;

    // memories[0], or emptyMemory when there are none. Resolved once by
    // InitMemory - the index space is fixed by then and linking has already
    // repointed whatever it was going to - so the call ops can read it straight
    // instead of branching. NULL until the module is loaded.
    IM3_حافظه         memory0;

    M3Export*         exports;
    u32               عدد_Exports;

#إذا d_m3HasSnapshots
    بنية M3EmbeddedSnapshot* snapshots;

    // A snapshot restores into a module none of whose code has run - start
    // function included - and a restore that fails part way leaves one that
    // can no longer run at all: its store is neither the snapshot's nor its own.
    bool                       has_پرلپسې;
    bool                       isUnusable;
#د_که_پای

    بنية M3Module* بل;
} M3Module;

M3_پایله Module_جمع_کړه_حافظه (IM3Module io_module, IM3_حافظه* o_حافظه, ثابت M3_حافظه_Info* i_info, bool i_isImported);

// Memory 0 of a module - the one the interpreter's _mem register tracks while
// that module's code is running. Never NULL once the module has been loaded.
ساكن مضمن
IM3_حافظه Module_Memory0 (IM3Module i_module)
{
    أرجع i_module->memory0;
}

ساكن مضمن
M3_حافظه_سرلیک* Module_حافظه_سرلیک (IM3Module i_module)
{
    أرجع i_module->memory0->mallocated;
}

M3_پایله Module_جمع_کړه_Global (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported);
M3_پایله Module_جمع_کړه_جدول (IM3Module io_module, IM3_جدول* o_جدول, ثابت M3_جدول_Info* i_info, bool i_isImported);
#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
M3_پایله Module_جمع_کړه_نښان (IM3Module io_module, IM3_نښان* o_نښان, IM3FuncType i_type, bool i_isImported);
#د_که_پای
#إذا d_m3_Has_ډېری_Switching
IM3FuncType Module_Cont_Type_د_Ref (IM3Module i_module, m3type_t i_refType);
#د_که_پای
M3_پایله    Module_DeclareFunction (IM3Module io_module, u32 i_شاخص);
bool        Module_IsFunctionDeclared (IM3Module i_module, u32 i_شاخص);

M3_پایله    Module_PreallocFunctions (IM3Module io_module, u32 i_ټول_Functions);
M3_پایله    Module_جمع_کړه_Function (IM3Module io_module, u32 i_type_شاخص, IM3ImportInfo i_importInfo /* can be null */);
IM3Function Module_GetFunction (IM3Module i_module, u32 i_function_شاخص);
bool        Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule);

فراغ        Module_Generate_نومونه (IM3Module i_module);

فراغ        FreeImportInfo (M3ImportInfo* i_info);

//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3Environment {
    //    struct M3Runtime *      runtimes;

    IM3FuncType      funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType      retFuncTypes[c_m3_Type_شمېر];      // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    u16              عدد_Func_Types;                       // hands out M3FuncType.canonicalIndex
    M3_کوډ_پاڼه*      پاڼې_Released;

    M3SectionHandler customSectionHandler;
} M3Environment;

فراغ     Environment_Release (IM3Environment i_environment);

// takes ownership of io_funcType and returns a pointer to the persistent version (could be same or different)
M3_پایله Environment_جمع_کړه_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType);
M3_پایله Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_شمېر, u16* o_لومړی_شاخص);
فراغ     Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_شاخص);

#إذا d_m3HasTypedRefs
M3_پایله Parse_ډېرۍ_Type (IM3Module i_module, m3type_t* o_ډېرۍ_بټونه, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی);
#د_که_پای
M3_پایله Parse_ارزښت_Type (IM3Module i_module, m3type_t* o_type, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی);

//---------------------------------------------------------------------------------------------------------------------------------


تعريفنوع بنية M3Runtime {
    M3Compilation  compilation;

    IM3Environment environment;

    M3_کوډ_پاڼه*    پاڼې_Open;      // linked list of code pages with writable space on them
    M3_کوډ_پاڼه*    پاڼې_ډک;      // linked list of at-capacity pages

    u32            عدد_کوډ_پاڼې;
    u32            عدد_فعال_کوډ_پاڼې;

    IM3Module      modules;        // linked list of imported modules

    فراغ*          ډېری;
    فراغ*          origin_ډېری;
    u32            ډېری_اندازه;
    u32            عدد_ډېری_Slots;
    فراغ*          ډېری_حد;     // native C-stack low-water mark; Wasm calls trap past it (NULL = unset)
    IM3Function    وروستی_Called;     // last function that successfully executed
    IM3Function    entered;        // entry point of the outermost call in flight, or of the last one, trapped or not
    u32            callNesting;    // RunCodeChecked() recursion depth

    فراغ*          userdata;

    u32            حافظه_حد;
    u64            حافظه_بایټونه_حد;
    u64            حافظه_بایټونه_کارول_شوی;
    u64            جدول_Elements_حد;
    u64            جدول_Elements_کارول_شوی;

#إذا d_m3DeterministicProfile
    // What the host answers about time and entropy out of, rather than out of the
    // machine it happens to be running on - see m3_deterministic.h, which is what
    // reads and moves them. Both are per-runtime, so two runtimes replay
    // independently and a fresh runtime replays the same way as the last one.
    u64 virtual_وخت_Ns;
    u64 randomState;
#د_که_پای

#إذا d_m3EnableValidation
    bool    skipValidation; // m3_SetValidation: compile function bodies without type checking them first
    ValCtx* validator;      // created on first use
#د_که_پای

#إذا d_m3EnableStrace >= 2
    u32 call_ژوروالی;
#د_که_پای

#إذا d_m3HasGasMetering
    // Both counts are in gas units (see M3_GAS_UNITS_PER_GAS). gasRemaining goes
    // negative by the cost of the segment that ran out, which is what makes the
    // gas used come out slightly over the limit, and is deliberate: a segment
    // is paid for in full before any of it runs.
    i64 gas_حد;
    i64 gas_پاتې_شوی;
#د_که_پای

#إذا d_m3HasExceptionHandling
    u32          try_ژوروالی;           // number of try regions whose body is executing
    M3Exception* pendingException;   // the exception currently unwinding, if any
    M3Exception* exceptions;         // the ones it still holds
    M3Exception* uncaughtException;  // a copy of what ended the last call, off the list
#د_که_پای
#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
    M3_Host_نښان* host_نښانونه;
#د_که_پای

    // Set by the embedder and by the host's interrupt handler, so they are here
    // whether or not the interpreter was built able to act on them. Only the
    // suspendable operations read suspendRequested, and those exist only with
    // stack switching, so without it m3_RequestSuspend is simply never answered.
    bool          isSuspendable;
    متطاير bool suspendRequested;

#إذا d_m3_Has_ډېری_Switching
    // Set while m3_ResumeRuntime replays a paused invocation: a back edge or a
    // function entry it paused at goes on past its check, so a pause already
    // requested takes effect at the next one instead of on the spot again
    bool            resumePastCheck;

    u32             continuations_حد;
    u32             continuationsAllocated;
    m3slot_t**      val_ډېری_Pool;
    u32             val_ډېری_Pool_Cap;
    u32             val_ډېری_Pool_شمېر;
    IM3Continuation فعال_Continuation;
    IM3Continuation suspendedContinuation;
    IM3Continuation continuations;
    IM3Continuation ریښه_Continuation;

    // The suspend on its way out, if there is one. Only ever one at a time -
    // it travels up the native stack and is answered before anything else can
    // start - so this belongs to the runtime rather than to any continuation.
    IM3_نښان          suspend_نښان;
    IM3Continuation suspendHandlerCont;     // whose resume named the tag
    pc_t            suspendStubPC;          // the handler it is headed for
    IM3Continuation switch_موخه;           // the peer to run instead, for a switch
    u64             suspendPayload[d_m3MaxContinuationPayload];
    u32             suspendPayloadIs64[d_m3MaxContinuationPayload];
    u32             عدد_Suspend_Payload;
#د_که_پای

#إذا d_m3HasSnapshots
    M3SnapshotHooks snapshotHooks;
#د_که_پای

    M3ErrorInfo error;
#إذا d_m3VerboseErrorMessages
    حرف error_پیغام[256]; // the actual buffer. M3ErrorInfo can point to this
#د_که_پای

#إذا d_m3_ریکارډ_Backtraces
    M3BacktraceInfo backtrace;
#د_که_پای

    u32 new_کوډ_پاڼه_Sequence;
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
#إذا d_m3_Max_Native_ډېری > 0
#  define d_m3_ډېری_حد_Enter(RT)                                               \
        فراغ * m3_Saved_ډېری_حد = (RT)->ډېری_حد;                           \
        إذا (not (RT)->ډېری_حد)                                               \
            (RT)->ډېری_حد = m3_Native_ډېری_حد(m3_Native_ډېری_Ptr(), (d_m3_Max_Native_ډېری));
#  define d_m3_ډېری_حد_Leave(RT)  (RT)->ډېری_حد = m3_Saved_ډېری_حد;
#وإلا
#  define d_m3_ډېری_حد_Enter(RT)
#  define d_m3_ډېری_حد_Leave(RT)
#د_که_پای

فراغ     InitRuntime (IM3Runtime io_runtime, u32 i_ډېری_اندازه_دننه_بایټونه);
فراغ     Runtime_Release (IM3Runtime io_runtime);

M3_پایله Resize_حافظه (IM3Runtime io_runtime, IM3_حافظه io_حافظه, u64 i_عدد_پاڼې);

// Give back whatever is behind io_memory->mallocated, which is the heap or a slot of
// the guarded arena depending on the build. Leaves the M3Memory itself alone.
فراغ     Free_حافظه_بلاک (IM3_حافظه io_حافظه);

تعريفنوع فراغ* (*ModuleVisitor)(IM3Module i_module, فراغ* i_info);
فراغ*       ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, فراغ* i_info);

فراغ*       v_پیدا_کړه_Function (IM3Module i_module, فراغ* i_info);

IM3_کوډ_پاڼه Acquire_کوډ_پاڼه (IM3Runtime io_runtime);
IM3_کوډ_پاڼه Acquire_کوډ_پاڼه_With_ظرفیت (IM3Runtime io_runtime, u32 i_کرښه_شمېر);
فراغ        Release_کوډ_پاڼه (IM3Runtime io_runtime, IM3_کوډ_پاڼه i_کوډ_پاڼه);

#إذا d_m3_Has_ډېری_Switching
ساكن مضمن
bool Continuation_Can_Acquire_ډېری (IM3Runtime runtime)
{
    أرجع not runtime->continuations_حد or
           runtime->continuationsAllocated < runtime->continuations_حد;
}
#د_که_پای

d_m3_پای_ټکی_Extern_C

#د_که_پای // m3_env_h
