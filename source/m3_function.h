//
//  m3_function.h
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ifndef m3_function_h
#define m3_function_h

#অন্তর্ভাব "m3_core.h"

d_m3BeginExternC

//---------------------------------------------------------------------------------------------------------------------------------

প্রকারলেখ কাঠামো M3FuncType {
    কাঠামো M3FuncType* পরবর্তী;

    u16                অঙ্ক_Rets;
    u16                অঙ্ক_Args;

    // Position in the environment's list of distinct function types. Equal
    // types share one M3FuncType, so this doubles as the canonical heap type
    // index a (ref $t) carries, and comparing two of them is exactly the
    // structural equivalence the spec asks for.
    u16                canonical_সূচক;

    bool               isContinuation;
    কাঠামো M3FuncType* contFuncType;

    // A member of a recursive group. The group is its identity, so it never
    // shares an entry with a type that merely matches it field for field -
    // not one outside any group, and not one in a group spelled the same way.
    bool               inRecGroup;

    m3type_t           types[];        // returns, then args
} M3FuncType;

প্রকারলেখ M3FuncType* IM3FuncType;


M3_ফলাফল            AllocFuncType (IM3FuncType* o_functionType, u32 i_অঙ্ক_Types);
bool                AreFuncTypesEqual (ধ্রুবক IM3FuncType i_typeA, ধ্রুবক IM3FuncType i_typeB);

u16                 Get_Func_Type_অঙ্ক_Params (ধ্রুবক IM3FuncType i_funcType);
m3type_t            GetFuncTypeParamType (ধ্রুবক IM3FuncType i_funcType, u16 i_সূচক);

u16                 Get_Func_Type_অঙ্ক_Results (ধ্রুবক IM3FuncType i_funcType);
m3type_t            Get_Func_Type_ফলাফল_Type (ধ্রুবক IM3FuncType i_funcType, u16 i_সূচক);

#যদি d_m3HasTypedRefs
// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t RefTypeOfFuncType (ধ্রুবক IM3FuncType i_funcType, bool i_nonNull);
#endif

//---------------------------------------------------------------------------------------------------------------------------------

// The ways a function's frame can be left standing while it waits to go on.
// Two of them can share a pc - a call's return address can be the very next
// operation, which is itself one that suspends - and they do not agree on
// what the frame holds there, so a pc alone does not name one.
//
// A snapshot writes these down, so the numbering is part of the DMP format.
প্রকারলেখ গণনা M3SafePointKind {
    safepoint_op,           // at a loop back edge, which suspends in place
    safepoint_suspend,      // just past a suspend or a switch
    safepoint_call,         // a call waiting on its callee
    safepoint_resume,       // a resume waiting on the continuation it runs
    safepoint_entry,        // at the start of a function's body, its locals set
} M3SafePointKind;

#যদি d_m3HasSnapshots

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
প্রকারলেখ কাঠামো M3_Slot_মান {
    u16      slot;            // relative to the frame, or a register alias (d_m3Reg0SlotAlias, d_m3Fp0SlotAlias)
    m3type_t type;       // full Wasm type, including reference heap type and nullability
} M3_Slot_মান;

// Values that go with a safepoint
#  define d_m3SafePointTakenBranch    0x1     // resumes a br_if that was taken: _r0 has to hold its condition
#  define d_m3_Safe_Point_লেজ_Call       0x2     // a return_call compiled as a call: saving walks past it, and it names no frame

// A place a function can be suspended at, or be waiting on something that was:
// a loop back edge, a function's entry, a call, a suspend, a switch or a resume.
//
// Its identity in a snapshot is the instruction's offset and its kind, and for
// a back edge - since one br_table can branch back to several loops - the loop
// it goes to. Safepoints are recorded in the order the body is read, so they
// are sorted by that offset.
প্রকারলেখ কাঠামো M3SafePoint {
    pc_t     pc;
    u32      wasmOffset;     // the instruction's, from the start of the function's body
    u32      first_মান;     // into M3SnapshotMap.values
    u32      first_Local_মান; // local types at this point, or UINT32_MAX for the map's shared locals
    u16      অঙ্ক_মানসমূহ;      // the live operand stack, bottom up; the locals are the map's own
    u16      অঙ্ক_Results;     // suspend: the top values are the slots it waits in | call, resume: results not yet written
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
প্রকারলেখ কাঠামো M3BlockStart {
    pc_t pc;             // what the frame records: a loop's body, a try_table's clause table
    u32  wasmOffset;
    u32  wasmEnd;        // the offset of its end
    u16  অঙ্ক_Clauses;     // try_table
    u8   opcode;         // c_waOp_loop or c_waOp_tryTable
} M3BlockStart;

প্রকারলেখ কাঠামো M3SnapshotMap {
    M3_Slot_মান*  locals;            // the arguments, then the declared locals
    u32           অঙ্ক_Locals;

    M3SafePoint*  safePoints;
    u32           অঙ্ক_Safe_Points;

    M3_Slot_মান*  মানসমূহ;
    u32           অঙ্ক_মানসমূহ;

    M3BlockStart* blocks;
    u32           অঙ্ক_Blocks;
} M3SnapshotMap;

শূন্য SnapshotMap_Free (M3SnapshotMap* i_map);

#endif // d_m3HasSnapshots

প্রকারলেখ কাঠামো M3Function {
    কাঠামো M3Module*   module;

    M3ImportInfo       import;

    // The linear memory a host function bound to this import addresses. Set to
    // the module's memory 0 when the host function is bound - what a bare i32
    // guest pointer means when nothing says otherwise - and repointed by
    // m3_BindImportMemory for a host module whose ABI names a memory instead.
    // Holds the memory rather than its index: a slot can alias another module's
    // memory, and growing one reallocates behind it. NULL for anything that is
    // not an import bound to a host function.
    কাঠামো M3Memory*   hostMemory;

    // An import linked to another module's export: the function that actually
    // implements it. Everything that calls through an import resolves to this
    // first, so the call carries the defining module with it - which is what
    // says whose linear memory the body runs against. NULL for a function with
    // a body of its own, and for an import bound to a host function (that one
    // runs against the importing module's memory).
    কাঠামো M3Function* resolved;

    bytes_t            wasm;
    bytes_t            wasmEnd;

    // What traces and errors call the function - its import name, or its first
    // few export names, or else the name section's - and what m3_FindFunction
    // falls back to once no export matches. Not a list of exports: those are
    // M3Module.exports, which keeps every one.
    cstr_t             names[d_m3MaxDuplicateFunctionImpl];
    u16                অঙ্ক_Names;                               // maximum of d_m3MaxDuplicateFunctionImpl

    IM3FuncType        funcType;

    pc_t               compiled;

#যদি (d_m3EnableCodePageRefCounting)
    IM3CodePage* codePageRefs;                           // array of all pages used
    u32          অঙ্ক_Code_Page_Refs;
#endif

#যদি defined(DEBUG)
    u32 hits;
    u32 index;
#endif

    u16   max_স্তূপ_Slots;

    u16   অঙ্ক_Ret_Slots;
    u16   অঙ্ক_Ret_And_Arg_Slots;

    u16   অঙ্ক_Locals;                              // not including args
    u32   অঙ্ক_Local_Bytes;

    bool  ownsWasmCode;

    u16   অঙ্ক_Constant_Bytes;
    শূন্য* constants;

#যদি d_m3HasSnapshots
    M3SnapshotMap* snapshotMap;                   // NULL unless compiled suspendable
#endif
} M3Function;


// The function that actually runs when this one is called: an import linked to
// another module resolves to that module's function, everything else to itself.
স্থির অন্তঃরেখ
IM3Function Function_Implementation (IM3Function i_function)
{
    ফেরত i_function->resolved ? i_function->resolved : i_function;
}

শূন্য     Function_Release (IM3Function i_function);
শূন্য     Function_FreeCompiledCode (IM3Function i_function);

cstr_t   GetFunctionImportModuleName (IM3Function i_function);
cstr_t*  GetFunctionNames (IM3Function i_function, u16* o_অঙ্ক_Names);
u16      Get_Function_অঙ্ক_Args (IM3Function i_function);
m3type_t GetFunctionArgType (IM3Function i_function, u32 i_সূচক);

u16      Get_Function_অঙ্ক_Returns (IM3Function i_function);
u8       GetFunctionReturnType (ধ্রুবক IM3Function i_function, u16 i_সূচক);

u32      Get_Function_অঙ্ক_Args_And_Locals (IM3Function i_function);

cstr_t   SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp);
cstr_t   SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp);

//---------------------------------------------------------------------------------------------------------------------------------


d_m3EndExternC

#endif /* m3_function_h */
