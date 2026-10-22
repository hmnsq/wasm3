//
//  m3_function.h
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ifndef m3_function_h
#define m3_function_h

#ଅନ୍ତର୍ଭାଵ "m3_core.h"

d_m3BeginExternC

//---------------------------------------------------------------------------------------------------------------------------------

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3FuncType {
    ସଂରଚନା M3FuncType* next;

    u16                ଅଙ୍କ_Rets;
    u16                ଅଙ୍କ_Args;

    // Position in the environment's list of distinct function types. Equal
    // types share one M3FuncType, so this doubles as the canonical heap type
    // index a (ref $t) carries, and comparing two of them is exactly the
    // structural equivalence the spec asks for.
    u16                canonicalIndex;

    bool               isContinuation;
    ସଂରଚନା M3FuncType* contFuncType;

    // A member of a recursive group. The group is its identity, so it never
    // shares an entry with a type that merely matches it field for field -
    // not one outside any group, and not one in a group spelled the same way.
    bool               inRecGroup;

    m3type_t           types[];        // returns, then args
} M3FuncType;

ପ୍ରକାରବ୍ୟାଖ୍ୟା M3FuncType* IM3FuncType;


M3Result            AllocFuncType (IM3FuncType* o_functionType, u32 i_ଅଙ୍କ_Types);
bool                AreFuncTypesEqual (ଅଚଳ IM3FuncType i_typeA, ଅଚଳ IM3FuncType i_typeB);

u16                 Get_Func_Type_ଅଙ୍କ_Params (ଅଚଳ IM3FuncType i_funcType);
m3type_t            GetFuncTypeParamType (ଅଚଳ IM3FuncType i_funcType, u16 i_index);

u16                 Get_Func_Type_ଅଙ୍କ_Results (ଅଚଳ IM3FuncType i_funcType);
m3type_t            GetFuncTypeResultType (ଅଚଳ IM3FuncType i_funcType, u16 i_index);

#ଯଦି d_m3HasTypedRefs
// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ଅଚଳ IM3FuncType i_funcType, bool i_nonNull);
#endif

//---------------------------------------------------------------------------------------------------------------------------------

// The ways a function's frame can be left standing while it waits to go on.
// Two of them can share a pc - a call's return address can be the very next
// operation, which is itself one that suspends - and they do not agree on
// what the frame holds there, so a pc alone does not name one.
//
// A snapshot writes these down, so the numbering is part of the DMP format.
ପ୍ରକାରବ୍ୟାଖ୍ୟା ଗଣନା M3SafePointKind {
    safepoint_op,           // at a loop back edge, which suspends in place
    safepoint_suspend,      // just past a suspend or a switch
    safepoint_call,         // a call waiting on its callee
    safepoint_resume,       // a resume waiting on the continuation it runs
    safepoint_entry,        // at the start of a function's body, its locals set
} M3SafePointKind;

#ଯଦି d_m3HasSnapshots

// What a snapshot needs to know about a function's compiled code, recorded by
// the compiler when the runtime is suspendable and nowhere else.
//
// A snapshot is written in Wasm's terms, not this build's: a place in a
// function is the offset of a Wasm instruction, and the frame is the Wasm
// locals and operand stack as typed values. The map is what translates. It
// says, at every place a frame can be left standing, which slot or register
// this build keeps each of those values in - so the values can be read out of
// one build's frame and scattered into another's, whatever layout each chose.

// A value the frame holds: where this build keeps it, and its type
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3_Slot_ମୂଲ୍ୟ {
    u16      slot;            // relative to the frame, or a register alias (d_m3Reg0SlotAlias, d_m3Fp0SlotAlias)
    m3type_t type;       // full Wasm type, including reference heap type and nullability
} M3_Slot_ମୂଲ୍ୟ;

// Values that go with a safepoint
#  define d_m3SafePointTakenBranch    0x1     // resumes a br_if that was taken: _r0 has to hold its condition
#  define d_m3SafePointTailCall       0x2     // a return_call compiled as a call: saving walks past it, and it names no frame

// A place a function can be suspended at, or be waiting on something that was:
// a loop back edge, a function's entry, a call, a suspend, a switch or a resume.
//
// Its identity in a snapshot is the instruction's offset and its kind, and for
// a back edge - since one br_table can branch back to several loops - the loop
// it goes to. Safepoints are recorded in the order the body is read, so they
// are sorted by that offset.
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3SafePoint {
    pc_t     pc;
    u32      wasmOffset;     // the instruction's, from the start of the function's body
    u32      first_ମୂଲ୍ୟ;     // into M3SnapshotMap.values
    u32      first_Local_ମୂଲ୍ୟ; // local types at this point, or UINT32_MAX for the map's shared locals
    u16      ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ;      // the live operand stack, bottom up; the locals are the map's own
    u16      ଅଙ୍କ_Results;     // suspend: the top values are the slots it waits in | call, resume: results not yet written
    u16      aux;            // call: where the callee's frame starts, in slots above this one | resume: its handler count
                             // | back edge: the loop it goes to, in M3SnapshotMap.blocks
    m3type_t resumeType; // resume: expected continuation type, including the result contract
    u8       kind;           // M3SafePointKind
    u8       flags;
} M3SafePoint;

// A loop or a try_table, which the interpreter keeps a native frame for while
// its body runs, and which a snapshot names by the instruction's offset.
//
// The region runs from the opening instruction to its end, and the ones that
// enclose a safepoint are those whose region holds its offset, both ends
// excluded - so a catch clause, which records its safepoints at its try_table,
// is outside that try_table. Recorded in the order they open, so enclosing
// regions come outermost first.
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3BlockStart {
    pc_t pc;             // what the frame records: a loop's body, a try_table's clause table
    u32  wasmOffset;
    u32  wasmEnd;        // the offset of its end
    u16  ଅଙ୍କ_Clauses;     // try_table
    u8   opcode;         // c_waOp_loop or c_waOp_tryTable
} M3BlockStart;

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3SnapshotMap {
    M3_Slot_ମୂଲ୍ୟ*  locals;            // the arguments, then the declared locals
    u32           ଅଙ୍କ_Locals;

    M3SafePoint*  safePoints;
    u32           ଅଙ୍କ_Safe_Points;

    M3_Slot_ମୂଲ୍ୟ*  ମୂଲ୍ୟଗୁଡ଼ିକ;
    u32           ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ;

    M3BlockStart* blocks;
    u32           ଅଙ୍କ_Blocks;
} M3SnapshotMap;

ଶୂନ୍ୟ SnapshotMap_Free (M3SnapshotMap* i_map);

#endif // d_m3HasSnapshots

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3Function {
    ସଂରଚନା M3Module*   module;

    M3ImportInfo       import;

    // The linear memory a host function bound to this import addresses. Set to
    // the module's memory 0 when the host function is bound - what a bare i32
    // guest pointer means when nothing says otherwise - and repointed by
    // m3_BindImportMemory for a host module whose ABI names a memory instead.
    // Holds the memory rather than its index: a slot can alias another module's
    // memory, and growing one reallocates behind it. NULL for anything that is
    // not an import bound to a host function.
    ସଂରଚନା M3Memory*   hostMemory;

    // An import linked to another module's export: the function that actually
    // implements it. Everything that calls through an import resolves to this
    // first, so the call carries the defining module with it - which is what
    // says whose linear memory the body runs against. NULL for a function with
    // a body of its own, and for an import bound to a host function (that one
    // runs against the importing module's memory).
    ସଂରଚନା M3Function* resolved;

    bytes_t            wasm;
    bytes_t            wasmEnd;

    // What traces and errors call the function - its import name, or its first
    // few export names, or else the name section's - and what m3_FindFunction
    // falls back to once no export matches. Not a list of exports: those are
    // M3Module.exports, which keeps every one.
    cstr_t             names[d_m3MaxDuplicateFunctionImpl];
    u16                ଅଙ୍କ_Names;                               // maximum of d_m3MaxDuplicateFunctionImpl

    IM3FuncType        funcType;

    pc_t               compiled;

#ଯଦି (d_m3EnableCodePageRefCounting)
    IM3CodePage* codePageRefs;                           // array of all pages used
    u32          ଅଙ୍କ_Code_Page_Refs;
#endif

#ଯଦି defined(DEBUG)
    u32 hits;
    u32 index;
#endif

    u16   max_ଗଦା_Slots;

    u16   ଅଙ୍କ_Ret_Slots;
    u16   ଅଙ୍କ_Ret_And_Arg_Slots;

    u16   ଅଙ୍କ_Locals;                              // not including args
    u32   ଅଙ୍କ_Local_Bytes;

    bool  ownsWasmCode;

    u16   ଅଙ୍କ_Constant_Bytes;
    ଶୂନ୍ୟ* constants;

#ଯଦି d_m3HasSnapshots
    M3SnapshotMap* snapshotMap;                   // NULL unless compiled suspendable
#endif
} M3Function;


// The function that actually runs when this one is called: an import linked to
// another module resolves to that module's function, everything else to itself.
ସ୍ଥିର ଧାଡ଼ିଭିତରେ
IM3Function Function_Implementation (IM3Function i_function)
{
    ଫେରନ୍ତୁ i_function->resolved ? i_function->resolved : i_function;
}

ଶୂନ୍ୟ     Function_Release (IM3Function i_function);
ଶୂନ୍ୟ     Function_FreeCompiledCode (IM3Function i_function);

cstr_t   GetFunctionImportModuleName (IM3Function i_function);
cstr_t*  GetFunctionNames (IM3Function i_function, u16* o_ଅଙ୍କ_Names);
u16      Get_Function_ଅଙ୍କ_Args (IM3Function i_function);
m3type_t GetFunctionArgType (IM3Function i_function, u32 i_index);

u16      Get_Function_ଅଙ୍କ_Returns (IM3Function i_function);
u8       GetFunctionReturnType (ଅଚଳ IM3Function i_function, u16 i_index);

u32      Get_Function_ଅଙ୍କ_Args_And_Locals (IM3Function i_function);

cstr_t   SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp);
cstr_t   SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp);

//---------------------------------------------------------------------------------------------------------------------------------


d_m3EndExternC

#endif /* m3_function_h */
