//
//  m3_function.h
//
//  Created by Steven Massey on 4/7/21.
//  Copyright © 2021 Steven Massey. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_function_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_function_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

d_m3BeginExternC

//---------------------------------------------------------------------------------------------------------------------------------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3FuncType {
    ਬਣਤਰ M3FuncType* ਅਗਲਾ;

    u16                ਅੰਕ_Rets;
    u16                ਅੰਕ_ਦਲੀਲਾਂ;

    // Position in the environment's list of distinct function types. Equal
    // types share one M3FuncType, so this doubles as the canonical heap type
    // index a (ref $t) carries, and comparing two of them is exactly the
    // structural equivalence the spec asks for.
    u16                canonical_ਸੂਚਕਾਂਕ;

    bool               isContinuation;
    ਬਣਤਰ M3FuncType* contFuncType;

    // A member of a recursive group. The group is its identity, so it never
    // shares an entry with a type that merely matches it field for field -
    // not one outside any group, and not one in a group spelled the same way.
    bool               ਵਿੱਚ_Rec_ਸਮੂਹ;

    m3type_t           types[];        // returns, then args
} M3FuncType;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3FuncType* IM3FuncType;


M3_ਨਤੀਜਾ            AllocFuncType (IM3FuncType* o_functionType, u32 i_ਅੰਕ_Types);
bool                AreFuncTypesEqual (ਅਚਲ IM3FuncType i_typeA, ਅਚਲ IM3FuncType i_typeB);

u16                 Get_Func_Type_ਅੰਕ_Params (ਅਚਲ IM3FuncType i_funcType);
m3type_t            GetFuncTypeParamType (ਅਚਲ IM3FuncType i_funcType, u16 i_ਸੂਚਕਾਂਕ);

u16                 Get_Func_Type_ਅੰਕ_Results (ਅਚਲ IM3FuncType i_funcType);
m3type_t            Get_Func_Type_ਨਤੀਜਾ_Type (ਅਚਲ IM3FuncType i_funcType, u16 i_ਸੂਚਕਾਂਕ);

#ਜੇ d_m3HasTypedRefs
// The type a (ref $t) / (ref null $t) naming this function type is spelled as
m3type_t Ref_Type_ਦਾ_Func_Type (ਅਚਲ IM3FuncType i_funcType, bool i_nonNull);
#ਜੇ_ਅੰਤ

//---------------------------------------------------------------------------------------------------------------------------------

// The ways a function's frame can be left standing while it waits to go on.
// Two of them can share a pc - a call's return address can be the very next
// operation, which is itself one that suspends - and they do not agree on
// what the frame holds there, so a pc alone does not name one.
//
// A snapshot writes these down, so the numbering is part of the DMP format.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਗਿਣਤੀ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਕਿਸਮ {
    safepoint_op,           // at a loop back edge, which suspends in place
    safepoint_suspend,      // just past a suspend or a switch
    safepoint_call,         // a call waiting on its callee
    safepoint_resume,       // a resume waiting on the continuation it runs
    safepoint_ਇੰਦਰਾਜ,        // at the start of a function's body, its locals set
} M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਕਿਸਮ;

#ਜੇ d_m3HasSnapshots

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
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਥਾਂ_ਮੁੱਲ {
    u16      ਥਾਂ;            // relative to the frame, or a register alias (d_m3Reg0SlotAlias, d_m3Fp0SlotAlias)
    m3type_t type;       // full Wasm type, including reference heap type and nullability
} M3_ਥਾਂ_ਮੁੱਲ;

// Values that go with a safepoint
#  define d_m3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਲਿਆ_Branch    0x1     // resumes a br_if that was taken: _r0 has to hold its condition
#  define d_m3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਪੂਛ_Call       0x2     // a return_call compiled as a call: saving walks past it, and it names no frame

// A place a function can be suspended at, or be waiting on something that was:
// a loop back edge, a function's entry, a call, a suspend, a switch or a resume.
//
// Its identity in a snapshot is the instruction's offset and its kind, and for
// a back edge - since one br_table can branch back to several loops - the loop
// it goes to. Safepoints are recorded in the order the body is read, so they
// are sorted by that offset.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ {
    pc_t     pc;
    u32      wasm_ਵਿੱਥ;     // the instruction's, from the start of the function's body
    u32      ਪਹਿਲਾ_ਮੁੱਲ;     // into M3SnapshotMap.values
    u32      ਪਹਿਲਾ_Local_ਮੁੱਲ; // local types at this point, or UINT32_MAX for the map's shared locals
    u16      ਅੰਕ_ਮੁੱਲਾਂ;      // the live operand stack, bottom up; the locals are the map's own
    u16      ਅੰਕ_Results;     // suspend: the top values are the slots it waits in | call, resume: results not yet written
    u16      aux;            // call: where the callee's frame starts, in slots above this one | resume: its handler count
                             // | back edge: the loop it goes to, in M3SnapshotMap.blocks
    m3type_t resumeType; // resume: expected continuation type, including the result contract
    u8       ਕਿਸਮ;           // M3SafePointKind
    u8       flags;
} M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ;

// A loop or a try_table, which the interpreter keeps a native frame for while
// its body runs, and which a snapshot names by the instruction's offset.
//
// The region runs from the opening instruction to its end, and the ones that
// enclose a safepoint are those whose region holds its offset, both ends
// excluded - so a catch clause, which records its safepoints at its try_table,
// is outside that try_table. Recorded in the order they open, so enclosing
// regions come outermost first.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਖੰਡ_ਸ਼ੁਰੂ {
    pc_t pc;             // what the frame records: a loop's body, a try_table's clause table
    u32  wasm_ਵਿੱਥ;
    u32  wasm_ਸਿਰਾ;        // the offset of its end
    u16  ਅੰਕ_Clauses;     // try_table
    u8   opcode;         // c_waOp_loop or c_waOp_tryTable
} M3_ਖੰਡ_ਸ਼ੁਰੂ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3SnapshotMap {
    M3_ਥਾਂ_ਮੁੱਲ*  locals;            // the arguments, then the declared locals
    u32           ਅੰਕ_Locals;

    M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ*  ਸੁਰੱਖਿਅਤ_Points;
    u32           ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points;

    M3_ਥਾਂ_ਮੁੱਲ*  ਮੁੱਲਾਂ;
    u32           ਅੰਕ_ਮੁੱਲਾਂ;

    M3_ਖੰਡ_ਸ਼ੁਰੂ* ਖੰਡਾਂ;
    u32           ਅੰਕ_ਖੰਡਾਂ;
} M3SnapshotMap;

ਖਾਲੀ SnapshotMap_Free (M3SnapshotMap* i_map);

#ਜੇ_ਅੰਤ // d_m3HasSnapshots

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Function {
    ਬਣਤਰ M3Module*   module;

    M3ImportInfo       import;

    // The linear memory a host function bound to this import addresses. Set to
    // the module's memory 0 when the host function is bound - what a bare i32
    // guest pointer means when nothing says otherwise - and repointed by
    // m3_BindImportMemory for a host module whose ABI names a memory instead.
    // Holds the memory rather than its index: a slot can alias another module's
    // memory, and growing one reallocates behind it. NULL for anything that is
    // not an import bound to a host function.
    ਬਣਤਰ M3_ਸਮ੍ਰਿਤੀ*   host_ਸਮ੍ਰਿਤੀ;

    // An import linked to another module's export: the function that actually
    // implements it. Everything that calls through an import resolves to this
    // first, so the call carries the defining module with it - which is what
    // says whose linear memory the body runs against. NULL for a function with
    // a body of its own, and for an import bound to a host function (that one
    // runs against the importing module's memory).
    ਬਣਤਰ M3Function* resolved;

    ਬਾਈਟਾਂ_t            wasm;
    ਬਾਈਟਾਂ_t            wasm_ਸਿਰਾ;

    // What traces and errors call the function - its import name, or its first
    // few export names, or else the name section's - and what m3_FindFunction
    // falls back to once no export matches. Not a list of exports: those are
    // M3Module.exports, which keeps every one.
    cstr_t             ਨਾਂ[d_m3MaxDuplicateFunctionImpl];
    u16                ਅੰਕ_ਨਾਂ;                               // maximum of d_m3MaxDuplicateFunctionImpl

    IM3FuncType        funcType;

    pc_t               compiled;

#ਜੇ (d_m3_Enable_ਕੋਡ_ਪੰਨਾ_Ref_Counting)
    IM3_ਕੋਡ_ਪੰਨਾ* ਕੋਡ_ਪੰਨਾ_Refs;                           // array of all pages used
    u32          ਅੰਕ_ਕੋਡ_ਪੰਨਾ_Refs;
#ਜੇ_ਅੰਤ

#ਜੇ defined(DEBUG)
    u32 ਪ੍ਰਾਪਤੀਆਂ;
    u32 index;
#ਜੇ_ਅੰਤ

    u16   max_ਢੇਰ_Slots;

    u16   ਅੰਕ_Ret_Slots;
    u16   ਅੰਕ_Ret_And_Arg_Slots;

    u16   ਅੰਕ_Locals;                              // not including args
    u32   ਅੰਕ_Local_ਬਾਈਟਾਂ;

    bool  owns_Wasm_ਕੋਡ;

    u16   ਅੰਕ_Constant_ਬਾਈਟਾਂ;
    ਖਾਲੀ* constants;

#ਜੇ d_m3HasSnapshots
    M3SnapshotMap* snapshotMap;                   // NULL unless compiled suspendable
#ਜੇ_ਅੰਤ
} M3Function;


// The function that actually runs when this one is called: an import linked to
// another module resolves to that module's function, everything else to itself.
ਸਥਿਰ ਸਤਰਅੰਦਰ
IM3Function Function_Implementation (IM3Function i_function)
{
    ਵਾਪਸ i_function->resolved ? i_function->resolved : i_function;
}

ਖਾਲੀ     Function_Release (IM3Function i_function);
ਖਾਲੀ     Function_Free_Compiled_ਕੋਡ (IM3Function i_function);

cstr_t   GetFunctionImportModuleName (IM3Function i_function);
cstr_t*  Get_Function_ਨਾਂ (IM3Function i_function, u16* o_ਅੰਕ_ਨਾਂ);
u16      Get_Function_ਅੰਕ_ਦਲੀਲਾਂ (IM3Function i_function);
m3type_t GetFunctionArgType (IM3Function i_function, u32 i_ਸੂਚਕਾਂਕ);

u16      Get_Function_ਅੰਕ_Returns (IM3Function i_function);
u8       GetFunctionReturnType (ਅਚਲ IM3Function i_function, u16 i_ਸੂਚਕਾਂਕ);

u32      Get_Function_ਅੰਕ_ਦਲੀਲਾਂ_And_Locals (IM3Function i_function);

cstr_t   SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp);
cstr_t   SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp);

//---------------------------------------------------------------------------------------------------------------------------------


d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ /* m3_function_h */
