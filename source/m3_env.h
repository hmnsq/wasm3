//
//  m3_env.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#यदि_अपरिभाषितम् m3_env_h
#परिभाषय m3_env_h

#अन्तर्भाव "wasm3.h"
#अन्तर्भाव "m3_code.h"
#अन्तर्भाव "m3_compile.h"

d_m3BeginExternC

प्रकारनिर्देश संरचना ValCtx ValCtx;   // the validator's scratch, defined in m3_validate.c


//---------------------------------------------------------------------------------------------------------------------------------

// Page counts are u64 because a memory64 type may declare up to 2^48 pages.
// Nothing near that can be backed, but the module still has to parse and
// validate; it is instantiation that fails.
प्रकारनिर्देश संरचना M3_स्मृतिः_Info {
    u64  init_पृष्ठानि;
    u64  max_पृष्ठानि;
    u32  page_आकार;
    bool hasMax;         // a declared maximum of 0 is not the same as none
    bool isMemory64;     // addressed by i64 rather than i32
    bool isShared;       // the threads proposal: a maximum is required
} M3_स्मृतिः_Info;


// One linear memory. Memories belong to the module that declares them, not to
// the runtime: two modules loaded into the same runtime each get their own, and
// the interpreter reaches the one it is currently running against through the
// _mem pseudo-register (see M3MemoryHeader).
//
// Every entry is individually allocated so that a slot may later be pointed at
// a memory another module owns - growing one has to be visible through every
// name it has. 'owner' says which module allocated it, and so which module
// frees it.
प्रकारनिर्देश संरचना M3_स्मृतिः {
    M3_स्मृतिः_शीर्षकम्*  mallocated;

    u64              अङ्कः_पृष्ठानि;
    u64              max_पृष्ठानि;
    u64              init_पृष्ठानि;
    u32              page_आकार;
    bool             hasMax;         // see M3MemoryInfo
    bool             isMemory64;     // addressed by i64 rather than i32
    bool             isShared;       // see M3MemoryInfo

    संरचना M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;

#यदि d_m3_Guarded_स्मृतिः
    // The slot of the guarded arena this memory's bytes live in, and so what
    // 'mallocated' points into rather than the heap - see Guard_TakeSlot
    शून्य* guardSlot;
#यदि_अन्तः
} M3_स्मृतिः;

प्रकारनिर्देश M3_स्मृतिः* IM3_स्मृतिः;

// The value type a memory's addresses, page counts and lengths are expressed
// in. Every instruction naming the memory takes and returns that type.
स्थिर अन्तःस्थ
m3type_t स्मृतिः_Addr_Type (ध्रुव M3_स्मृतिः* i_स्मृतिः)
{
    प्रत्यागम i_स्मृतिः->isMemory64 ? c_m3Type_i64 : c_m3Type_i32;
}


// A module that declares no custom page size means the default one. The parser
// leaves that as zero, and InitMemory only fills it in when it allocates, so a
// memory has to be asked rather than read directly until then.
स्थिर अन्तःस्थ
u32 स्मृतिः_Page_आकार (ध्रुव M3_स्मृतिः* i_स्मृतिः)
{
    प्रत्यागम i_स्मृतिः->page_आकार ? i_स्मृतिः->page_आकार : d_m3_Default_Mem_Page_आकार;
}


//---------------------------------------------------------------------------------------------------------------------------------

// A table type as the module declared it. Sizes stay u32 - a table64 may name
// far more entries than that, but d_m3MaxSaneTableSize refuses anything near
// the limit long before it matters.
प्रकारनिर्देश संरचना M3_सारणी_Info {
    m3type_t elemType;
    u32      init_आकार;
    u32      max_आकार;
    bool     hasMax;         // a declared maximum of 0 is not the same as none
    bool     isTable64;      // indexed by i64 rather than i32
} M3_सारणी_Info;


// A table's elements are opaque pointer-sized references: IM3Function for a
// funcref table, a host handle for an externref one. NULL is the null reference.
प्रकारनिर्देश संरचना M3_सारणी {
    शून्य**           elements;
    u32              आकार;
    u32              max_आकार;            // 0 when the module declared no maximum
    m3type_t         type;

    // Every slot starts out holding this, rather than null. A table whose
    // element type is not nullable has to say what to fill itself with.
    अष्टकानि_t          init_व्यञ्जकम्;
    u32              init_व्यञ्जकम्_आकार;

    u32              init_आकार;       // the declared minimum
    bool             hasMax;         // a declared maximum of 0 is not the same as none
    bool             isTable64;      // indexed by i64 rather than i32

    संरचना M3Module* owner;          // the module that allocated it
    M3ImportInfo     import;         // when declared as an import
    bool             imported;
} M3_सारणी;

प्रकारनिर्देश M3_सारणी* IM3_सारणी;

// The value type a table's indexes and sizes are expressed in - table64 is the
// same choice as memory64, made per table.
स्थिर अन्तःस्थ
m3type_t सारणी_Addr_Type (ध्रुव M3_सारणी* i_सारणी)
{
    प्रत्यागम i_सारणी->isTable64 ? c_m3Type_i64 : c_m3Type_i32;
}

// Element segment modes, from the low two bits of the segment's flags
प्रकारनिर्देश गणना {
    c_m3_Elem_सक्रिय      = 0,        // table 0
    c_m3Elem_passive     = 1,
    c_m3_Elem_सक्रिय_Idx   = 2,        // explicit table index
    c_m3Elem_declarative = 3
} M3_Element_बहुलकम्;

प्रकारनिर्देश संरचना M3ElementSegment {
    अष्टकानि_t  init_व्यञ्जकम्;       // active segments only: the offset
    अष्टकानि_t  elements;       // funcidx list, or const exprs when isExpr
    शून्य**   resolved;       // passive segments only, for table.init

    u32      init_व्यञ्जकम्_आकार;
    u32      अङ्कः_Elements;
    u32      सारणी_सूचकांक;

    m3type_t type;
    u8       बहुलकम्;
    bool     is_व्यञ्जकम्;
    bool     dropped;
} M3ElementSegment;

प्रकारनिर्देश संरचना M3_दत्त_Segment {
    ध्रुव u8* init_व्यञ्जकम्;           // wasm code
    ध्रुव u8* दत्त;

    u32       init_व्यञ्जकम्_आकार;
    u32       स्मृतिः_Region;
    u32       आकार;

    bool      isPassive;
    bool      dropped;            // active segments are dropped once instantiated
} M3_दत्त_Segment;

//---------------------------------------------------------------------------------------------------------------------------------

प्रकारनिर्देश संरचना M3Global {
    M3ImportInfo     import;

    // An import linked to another module's export: the global that actually
    // holds the value. Reads and writes go there, so a mutable global stays one
    // cell seen under two names. NULL for anything else, including an import a
    // host supplied a value for through m3_LinkGlobal.
    संरचना M3Global* resolved;

    सङ्घ {
        i32 i32_मानम्;
        i64 i64_मानम्;
#यदि d_m3HasFloat
        f64 f64_मानम्;
        f32 f32_मानम्;
#यदि_अन्तः
        शून्य* ref_मानम्;
    };

    अष्टकानि_t  init_व्यञ्जकम्;       // wasm code
    u32      init_व्यञ्जकम्_आकार;
    m3type_t type;
    bool     imported;
    bool     isMutable;
} M3Global;


//---------------------------------------------------------------------------------------------------------------------------------

#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching

// An exception / control tag, as the tag section declares it. Tags are compared by
// identity, and this struct's address is that identity.
प्रकारनिर्देश संरचना M3_चिह्नकम् {
    M3ImportInfo  import;
    IM3FuncType   type;           // params are the payload; results are empty for EH, can be non-empty for stack switching
    संरचना M3_चिह्नकम्* resolved;
    bool          imported;
} M3_चिह्नकम्;

// A tag m3_NewTag made. It belongs to the runtime rather than to a module, and
// is linked in front of the runtime's others rather than kept in an array, which
// would move the tags - and with them their identities - whenever it grew.
प्रकारनिर्देश संरचना M3_Host_चिह्नकम् {
    संरचना M3_Host_चिह्नकम्* अग्र;
    M3_चिह्नकम्             चिह्नकम्;
} M3_Host_चिह्नकम्;

#यदि_अन्तः // d_m3HasExceptionHandling || d_m3HasStackSwitching


#यदि d_m3HasExceptionHandling

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
प्रकारनिर्देश संरचना M3Exception {
    संरचना M3Exception* अग्र;           // the runtime's allocation list,
    संरचना M3Exception* पूर्व;           //   doubly linked so one can leave early
    IM3_चिह्नकम्              चिह्नकम्;
    u32                 अङ्कः_तर्काः;
    bool                reified;        // an exnref to this has been handed out
    u64                 तर्काः[];
} M3Exception;


M3Exception* NewException (IM3Runtime io_runtime, IM3_चिह्नकम् i_चिह्नकम्, u32 i_अङ्कः_तर्काः);
शून्य         FreeException (IM3Runtime io_runtime, M3Exception* i_exception);
शून्य         FreeExceptions (IM3Runtime io_runtime);

#यदि_अन्तः // d_m3HasExceptionHandling


#यदि d_m3_Has_राशिः_Switching

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
प्रकारनिर्देश गणना M3_Frame_विधा {
    frame_call,          // op_Call / op_CallRef / op_CallIndirect
    frame_loop,          // op_Loop
#  यदि d_m3HasExceptionHandling
    frame_try,           // op_TryTable
#  endif
#  यदि d_m3_प्रविष्टिः_Keeps_Frame
    frame_प्रविष्टिः,         // op_Entry, in builds where it keeps a frame
#  endif
    frame_resume,        // op_Resume, when the suspend is not its to answer
} M3_Frame_विधा;

प्रकारनिर्देश संरचना M3Frame {
    u8        विधा;      // M3FrameKind
    pc_t      pc;        // call: the return address | loop: the loop id, which is
                         // also its body | try: the clause table | entry: unused
    m3stack_t sp;
    IM3_स्मृतिः स्मृतिः;    // _mem is re-derived from this rather than stored:
                         // growing linear memory moves the header, so only the
                         // M3Memory stays valid across a suspend

    सङ्घ {
        // op_Call resumes the caller with the registers its own native frame was
        // holding, so those belong to the frame, not to the suspension point
        संरचना {
            IM3Function function;
            m3reg_t     r0;
#  यदि d_m3HasFloat
            f64 fp0;
#  endif
        } call;
#  यदि d_m3HasExceptionHandling
        संरचना {
            u32  अङ्कः_Clauses;
            bool handlers_जीवितम्;   // false once op_PopHandlers has retired the region
        } try_;
#  endif
#  यदि d_m3_प्रविष्टिः_Keeps_Frame
        संरचना {
            IM3Function function;
        } प्रविष्टिः;
#  endif
        // A resume that the suspend travelled straight through, because the
        // tag was not one its handlers named. The continuation it was running
        // is part of what got captured, so everything needed to set it going
        // again under the same handlers is kept here.
        संरचना {
            संरचना M3Continuation* cont;
            pc_t                   handlersPC;
            u32                    अङ्कः_Handlers;
            pc_t                   resultsPC;
            u32                    अङ्कः_Results;
        } resume;
    };
} M3Frame;

// Which of the three resume forms an op_Resume was emitted for
गणना {
    d_m3_Resume_प्रसामान्यम् = 0,
    d_m3ResumeThrow,
    d_m3ResumeThrowRef,
};

प्रकारनिर्देश गणना M3ContinuationState {
    cont_allocated = 0,
    cont_running,
    cont_suspended,
    cont_returned,
    cont_consumed
} M3ContinuationState;

प्रकारनिर्देश संरचना M3ResumeHandler {
    u8     विधा;        // 0: (on $t $h), 1: (on $t switch)
    IM3_चिह्नकम् चिह्नकम्;
    pc_t   stubPC;
} M3ResumeHandler;

प्रकारनिर्देश संरचना M3Continuation {
    संरचना M3Continuation* अग्र;           // linked list in runtime for cleanup
    IM3FuncType            type;            // continuation type (cont $ft)
    IM3Function            प्रविष्टिः_Function;   // target function
    M3ContinuationState    state;

    m3slot_t*              val_राशिः;        // allocated value stack buffer
    u32                    अङ्कः_राशिः_Slots;
    m3stack_t              sp;              // current value stack pointer

    M3Frame*               चौकटाः;          // native frames recorded by the last suspend
    u32                    चौकटाः_Cap;       // entries allocated
    u32                    अङ्कः_चौकटाः;       // entries in use, innermost first

    pc_t                   pc;              // resume instruction pointer
    u8                     suspendPoint;    // M3SafePointKind: what left pc where it is
    m3reg_t                r0;              // saved accumulator
#  यदि d_m3HasFloat
    f64 fp0;
#  endif

    // Bound args count (for cont.bind)
    u32 bound_तर्काः_गणनम्;

#  यदि d_m3HasExceptionHandling
    // resume_throw: raised at the suspension point instead of carrying on from
    // it, so the try regions inside the continuation get their turn first
    M3Exception* resumeThrow;
#  endif

    // Where the values go when this continuation is next given some: the slots
    // its own suspend or switch is waiting on.
    u32                    अङ्कः_Suspend_Results;
    i32                    suspend_परिणाम_Offsets[d_m3MaxContinuationPayload];
    u32                    suspend_परिणाम_Is64[d_m3MaxContinuationPayload];

    // Active handler table from enclosing resume:
    u32                    अङ्कः_Handlers;
    pc_t                   handlersPC;
    संरचना M3Continuation* parent;         // resumer continuation
} M3Continuation, *IM3Continuation;

M3_परिणाम        Continuation_Acquire_राशिः (IM3Runtime runtime, IM3Continuation cont);
शून्य            Continuation_Recycle_राशिः (IM3Runtime runtime, IM3Continuation cont);
IM3Continuation Continuation_New (IM3Runtime i_runtime, IM3FuncType i_type, IM3Function i_function);
IM3Continuation Continuation_ForkSuspended (IM3Runtime i_runtime, IM3Continuation i_cont);
शून्य            Continuation_ReleaseAll (IM3Runtime io_runtime);

// Appends one frame to the suspending continuation. Returns the marker the
// caller should keep unwinding with: the suspend marker when the frame was
// recorded, or a trap when it could not be - a continuation that cannot record
// its frames cannot be resumed, so the suspend has to become a trap instead.
m3ret_t         Continuation_लेखः_Frame (IM3Runtime i_runtime, ध्रुव M3Frame* i_frame);

// Points runtime->stack, where the host's next call puts its arguments, above
// whatever a suspended root continuation holds - or back at the bottom when
// there is none.
शून्य            Runtime_Place_Call_राशिः (IM3Runtime io_runtime);

#यदि_अन्तः // d_m3HasStackSwitching


#यदि d_m3HasSnapshots
प्रकारनिर्देश संरचना M3EmbeddedSnapshot {
    cstr_t                     name;
    अष्टकानि_t                    दत्त;
    u32                        आकार;
    संरचना M3EmbeddedSnapshot* अग्र;
} M3EmbeddedSnapshot;
#यदि_अन्तः


//---------------------------------------------------------------------------------------------------------------------------------

// One entry of the export section. Export names belong to the module rather than
// to what they name: one entity can be exported under any number of names, and an
// import re-exported under a name of the module's own, whose slot linking points
// at another module's entity. Every lookup by name goes through these, and
// resolves the index against the module's index space at the time it is asked.
प्रकारनिर्देश संरचना M3Export {
    cstr_t name;
    u32    name_दीर्घता;   // the name may hold a NUL, which ends the C string early
    u32    index;        // into the index space 'kind' names
    u8     विधा;         // d_externalKind_*
} M3Export;

प्रकारनिर्देश संरचना M3Module {
    संरचना M3Runtime*     runtime;
    संरचना M3Environment* environment;

    अष्टकानि_t               wasmStart;
    अष्टकानि_t               wasm_अन्तः;

    cstr_t                name;

    u32                   अङ्कः_Func_Types;
    IM3FuncType*          funcTypes;              // array of pointers to list of FuncTypes

    u32                   अङ्कः_Func_Imports;
    u32                   अङ्कः_Functions;
    u32                   allFunctions;           // allocated functions count
    M3Function*           functions;

    i32                   startFunction;

    u32                   अङ्कः_दत्त_Segments;
    M3_दत्त_Segment*        दत्त_Segments;

    u32                   दत्त_गणनम्;          // from the data count section
    bool                  has_दत्त_गणनम्;

    // Bitset of the functions ref.func may name: those exported, or referenced
    // by an element segment or a global initializer. One bit per function.
    u8*                   declaredFuncs;

    //u32                     importedGlobals;
    u32                   अङ्कः_Globals;
    M3Global*             globals;

#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    u32    अङ्कः_चिह्नकानि;
    M3_चिह्नकम्* चिह्नकानि;
#यदि_अन्तः

    u32               अङ्कः_Element_Segments;
    M3ElementSegment* elementSegments;
    अष्टकानि_t           element_Section_अन्तः;

    // The module's table index space, imported entries first. Entries are
    // borrowed pointers, individually allocated so a slot can be pointed at a
    // table another module owns - see M3Table.owner, and M3Module.memories,
    // which works the same way.
    IM3_सारणी*         tables;
    u32               अङ्कः_Tables;

    // The module's memory index space: imported entries first, then declared
    // ones, exactly as the Wasm index space orders them. Entries are borrowed
    // pointers - see M3Memory.owner.
    IM3_स्मृतिः*        memories;
    u32               अङ्कः_Memories;

    // The interpreter always carries a valid memory header in _mem - the
    // stack-limit check, the backtrace recorder and the call ops all reach the
    // runtime through it. A module that declares no memory still needs one, so
    // it gets this zero-length stand-in rather than a NULL _mem.
    M3_स्मृतिः          रिक्तानि_स्मृतिः;

    // memories[0], or emptyMemory when there are none. Resolved once by
    // InitMemory - the index space is fixed by then and linking has already
    // repointed whatever it was going to - so the call ops can read it straight
    // instead of branching. NULL until the module is loaded.
    IM3_स्मृतिः         memory0;

    M3Export*         exports;
    u32               अङ्कः_Exports;

#यदि d_m3HasSnapshots
    संरचना M3EmbeddedSnapshot* snapshots;

    // A snapshot restores into a module none of whose code has run - start
    // function included - and a restore that fails part way leaves one that
    // can no longer run at all: its store is neither the snapshot's nor its own.
    bool                       has_सततम्;
    bool                       isUnusable;
#यदि_अन्तः

    संरचना M3Module* अग्र;
} M3Module;

M3_परिणाम Module_सङ्कलय_स्मृतिः (IM3Module io_module, IM3_स्मृतिः* o_स्मृतिः, ध्रुव M3_स्मृतिः_Info* i_info, bool i_isImported);

// Memory 0 of a module - the one the interpreter's _mem register tracks while
// that module's code is running. Never NULL once the module has been loaded.
स्थिर अन्तःस्थ
IM3_स्मृतिः Module_Memory0 (IM3Module i_module)
{
    प्रत्यागम i_module->memory0;
}

स्थिर अन्तःस्थ
M3_स्मृतिः_शीर्षकम्* Module_स्मृतिः_शीर्षकम् (IM3Module i_module)
{
    प्रत्यागम i_module->memory0->mallocated;
}

M3_परिणाम Module_सङ्कलय_Global (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported);
M3_परिणाम Module_सङ्कलय_सारणी (IM3Module io_module, IM3_सारणी* o_सारणी, ध्रुव M3_सारणी_Info* i_info, bool i_isImported);
#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
M3_परिणाम Module_सङ्कलय_चिह्नकम् (IM3Module io_module, IM3_चिह्नकम्* o_चिह्नकम्, IM3FuncType i_type, bool i_isImported);
#यदि_अन्तः
#यदि d_m3_Has_राशिः_Switching
IM3FuncType Module_Cont_Type_सम्बन्धि_Ref (IM3Module i_module, m3type_t i_refType);
#यदि_अन्तः
M3_परिणाम    Module_DeclareFunction (IM3Module io_module, u32 i_सूचकांक);
bool        Module_IsFunctionDeclared (IM3Module i_module, u32 i_सूचकांक);

M3_परिणाम    Module_PreallocFunctions (IM3Module io_module, u32 i_योग_Functions);
M3_परिणाम    Module_सङ्कलय_Function (IM3Module io_module, u32 i_type_सूचकांक, IM3ImportInfo i_importInfo /* can be null */);
IM3Function Module_GetFunction (IM3Module i_module, u32 i_function_सूचकांक);
bool        Module_HasLinkedHostImport (IM3Module i_module, cstr_t i_importModule);

शून्य        Module_Generate_नामानि (IM3Module i_module);

शून्य        FreeImportInfo (M3ImportInfo* i_info);

//---------------------------------------------------------------------------------------------------------------------------------

प्रकारनिर्देश संरचना M3Environment {
    //    struct M3Runtime *      runtimes;

    IM3FuncType      funcTypes;                          // linked list of unique M3FuncType structs that can be compared using pointer-equivalence

    IM3FuncType      retFuncTypes[c_m3_Type_गणनम्];      // these 'point' to elements in the linked list above.
                                                                // the number of elements must match the basic types as per M3ValueType
    u16              अङ्कः_Func_Types;                       // hands out M3FuncType.canonicalIndex
    M3_संहिता_Page*      पृष्ठानि_Released;

    M3SectionHandler customSectionHandler;
} M3Environment;

शून्य     Environment_Release (IM3Environment i_environment);

// takes ownership of io_funcType and returns a pointer to the persistent version (could be same or different)
M3_परिणाम Environment_सङ्कलय_Func_Type (IM3Environment i_environment, IM3FuncType* io_funcType);
M3_परिणाम Environment_ReserveFuncTypes (IM3Environment i_environment, u32 i_गणनम्, u16* o_first_सूचकांक);
शून्य     Environment_AdoptFuncType (IM3Environment i_environment, IM3FuncType i_funcType, u16 i_canonical_सूचकांक);

#यदि d_m3HasTypedRefs
M3_परिणाम ParseHeapType (IM3Module i_module, m3type_t* o_heap_द्वयङ्काः, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);
#यदि_अन्तः
M3_परिणाम Parse_मानम्_Type (IM3Module i_module, m3type_t* o_type, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः);

//---------------------------------------------------------------------------------------------------------------------------------


प्रकारनिर्देश संरचना M3Runtime {
    M3Compilation  compilation;

    IM3Environment environment;

    M3_संहिता_Page*    पृष्ठानि_Open;      // linked list of code pages with writable space on them
    M3_संहिता_Page*    पृष्ठानि_पूर्णम्;      // linked list of at-capacity pages

    u32            अङ्कः_संहिता_पृष्ठानि;
    u32            अङ्कः_सक्रिय_संहिता_पृष्ठानि;

    IM3Module      modules;        // linked list of imported modules

    शून्य*          राशिः;
    शून्य*          origin_राशिः;
    u32            राशिः_आकार;
    u32            अङ्कः_राशिः_Slots;
    शून्य*          राशिः_सीमा;     // native C-stack low-water mark; Wasm calls trap past it (NULL = unset)
    IM3Function    चरमः_Called;     // last function that successfully executed
    IM3Function    entered;        // entry point of the outermost call in flight, or of the last one, trapped or not
    u32            callNesting;    // RunCodeChecked() recursion depth

    शून्य*          userdata;

    u32            स्मृतिः_सीमा;
    u64            स्मृतिः_अष्टकानि_सीमा;
    u64            स्मृतिः_अष्टकानि_प्रयुक्तम्;
    u64            सारणी_Elements_सीमा;
    u64            सारणी_Elements_प्रयुक्तम्;

#यदि d_m3DeterministicProfile
    // What the host answers about time and entropy out of, rather than out of the
    // machine it happens to be running on - see m3_deterministic.h, which is what
    // reads and moves them. Both are per-runtime, so two runtimes replay
    // independently and a fresh runtime replays the same way as the last one.
    u64 virtualTimeNs;
    u64 randomState;
#यदि_अन्तः

#यदि d_m3EnableValidation
    bool    skipValidation; // m3_SetValidation: compile function bodies without type checking them first
    ValCtx* validator;      // created on first use
#यदि_अन्तः

#यदि d_m3EnableStrace >= 2
    u32 call_गभीरता;
#यदि_अन्तः

#यदि d_m3HasGasMetering
    // Both counts are in gas units (see M3_GAS_UNITS_PER_GAS). gasRemaining goes
    // negative by the cost of the segment that ran out, which is what makes the
    // gas used come out slightly over the limit, and is deliberate: a segment
    // is paid for in full before any of it runs.
    i64 gas_सीमा;
    i64 gasRemaining;
#यदि_अन्तः

#यदि d_m3HasExceptionHandling
    u32          try_गभीरता;           // number of try regions whose body is executing
    M3Exception* pendingException;   // the exception currently unwinding, if any
    M3Exception* exceptions;         // the ones it still holds
    M3Exception* uncaughtException;  // a copy of what ended the last call, off the list
#यदि_अन्तः
#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    M3_Host_चिह्नकम्* host_चिह्नकानि;
#यदि_अन्तः

    // Set by the embedder and by the host's interrupt handler, so they are here
    // whether or not the interpreter was built able to act on them. Only the
    // suspendable operations read suspendRequested, and those exist only with
    // stack switching, so without it m3_RequestSuspend is simply never answered.
    bool          isSuspendable;
    चञ्चल bool suspendRequested;

#यदि d_m3_Has_राशिः_Switching
    // Set while m3_ResumeRuntime replays a paused invocation: a back edge or a
    // function entry it paused at goes on past its check, so a pause already
    // requested takes effect at the next one instead of on the spot again
    bool            resumePastCheck;

    u32             continuations_सीमा;
    u32             continuationsAllocated;
    m3slot_t**      val_राशिः_Pool;
    u32             val_राशिः_Pool_Cap;
    u32             val_राशिः_Pool_गणनम्;
    IM3Continuation सक्रिय_Continuation;
    IM3Continuation suspendedContinuation;
    IM3Continuation continuations;
    IM3Continuation मूलम्_Continuation;

    // The suspend on its way out, if there is one. Only ever one at a time -
    // it travels up the native stack and is answered before anything else can
    // start - so this belongs to the runtime rather than to any continuation.
    IM3_चिह्नकम्          suspend_चिह्नकम्;
    IM3Continuation suspendHandlerCont;     // whose resume named the tag
    pc_t            suspendStubPC;          // the handler it is headed for
    IM3Continuation switchTarget;           // the peer to run instead, for a switch
    u64             suspendPayload[d_m3MaxContinuationPayload];
    u32             suspendPayloadIs64[d_m3MaxContinuationPayload];
    u32             अङ्कः_Suspend_Payload;
#यदि_अन्तः

#यदि d_m3HasSnapshots
    M3SnapshotHooks snapshotHooks;
#यदि_अन्तः

    M3ErrorInfo error;
#यदि d_m3VerboseErrorMessages
    वर्ण error_सन्देशः[256]; // the actual buffer. M3ErrorInfo can point to this
#यदि_अन्तः

#यदि d_m3_लेखः_Backtraces
    M3BacktraceInfo backtrace;
#यदि_अन्तः

    u32 new_संहिता_Page_Sequence;
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
#यदि d_m3_Max_Native_राशिः > 0
#  define d_m3_राशिः_सीमा_Enter(RT)                                               \
        शून्य * m3_Saved_राशिः_सीमा = (RT)->राशिः_सीमा;                           \
        यदि (not (RT)->राशिः_सीमा)                                               \
            (RT)->राशिः_सीमा = m3_Native_राशिः_सीमा(m3_Native_राशिः_Ptr(), (d_m3_Max_Native_राशिः));
#  define d_m3_राशिः_सीमा_Leave(RT)  (RT)->राशिः_सीमा = m3_Saved_राशिः_सीमा;
#अन्यथा
#  define d_m3_राशिः_सीमा_Enter(RT)
#  define d_m3_राशिः_सीमा_Leave(RT)
#यदि_अन्तः

शून्य     InitRuntime (IM3Runtime io_runtime, u32 i_राशिः_आकार_अन्तर्_अष्टकानि);
शून्य     Runtime_Release (IM3Runtime io_runtime);

M3_परिणाम Resize_स्मृतिः (IM3Runtime io_runtime, IM3_स्मृतिः io_स्मृतिः, u64 i_अङ्कः_पृष्ठानि);

// Give back whatever is behind io_memory->mallocated, which is the heap or a slot of
// the guarded arena depending on the build. Leaves the M3Memory itself alone.
शून्य     Free_स्मृतिः_खण्डम् (IM3_स्मृतिः io_स्मृतिः);

प्रकारनिर्देश शून्य* (*ModuleVisitor)(IM3Module i_module, शून्य* i_info);
शून्य*       ForEachModule (IM3Runtime i_runtime, ModuleVisitor i_visitor, शून्य* i_info);

शून्य*       v_FindFunction (IM3Module i_module, शून्य* i_info);

IM3_संहिता_Page Acquire_संहिता_Page (IM3Runtime io_runtime);
IM3_संहिता_Page Acquire_संहिता_Page_With_क्षमता (IM3Runtime io_runtime, u32 i_पङ्क्तिः_गणनम्);
शून्य        Release_संहिता_Page (IM3Runtime io_runtime, IM3_संहिता_Page i_संहिता_Page);

#यदि d_m3_Has_राशिः_Switching
स्थिर अन्तःस्थ
bool Continuation_Can_Acquire_राशिः (IM3Runtime runtime)
{
    प्रत्यागम not runtime->continuations_सीमा or
           runtime->continuationsAllocated < runtime->continuations_सीमा;
}
#यदि_अन्तः

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_env_h
