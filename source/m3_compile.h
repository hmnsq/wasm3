//
//  m3_compile.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_compile_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_compile_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_code.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exec_defs.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_function.h"

d_m3BeginExternC

ਗਿਣਤੀ {
    c_wa_Op_ਖੰਡ              = 0x02,
    c_waOp_loop               = 0x03,
    c_waOp_if                 = 0x04,
    c_waOp_else               = 0x05,
    c_waOp_throw              = 0x08,
    c_waOp_throwRef           = 0x0a,
    c_wa_Op_ਸਿਰਾ                = 0x0b,
    c_waOp_branch             = 0x0c,
    c_wa_Op_branch_ਸਾਰਣੀ        = 0x0e,
    c_waOp_branchIf           = 0x0d,
    c_waOp_return             = 0x0f,
    c_waOp_call               = 0x10,
    c_wa_Op_call_ਅਸਿੱਧਾ       = 0x11,
    c_waOp_returnCall         = 0x12,
    c_wa_Op_return_Call_ਅਸਿੱਧਾ = 0x13,
    c_waOp_callRef            = 0x14,
    c_waOp_returnCallRef      = 0x15,
    c_waOp_getLocal           = 0x20,
    c_wa_Op_ਸੈੱਟ_Local           = 0x21,
    c_waOp_teeLocal           = 0x22,

    c_waOp_selectTyped        = 0x1c,
    c_wa_Op_try_ਸਾਰਣੀ           = 0x1f,

    c_waOp_getGlobal          = 0x23,
    c_wa_Op_ਸਾਰਣੀ_Get           = 0x25,
    c_wa_Op_ਸਾਰਣੀ_ਸੈੱਟ           = 0x26,

    c_waOp_store_f32          = 0x38,
    c_waOp_store_f64          = 0x39,

    c_waOp_i32_const          = 0x41,
    c_waOp_i64_const          = 0x42,
    c_waOp_f32_const          = 0x43,
    c_waOp_f64_const          = 0x44,

    // the arithmetic the extended-const proposal admits into constant expressions
    c_wa_Op_i32_ਜੋੜੋ            = 0x6a,
    c_waOp_i32_sub            = 0x6b,
    c_waOp_i32_mul            = 0x6c,
    c_wa_Op_i64_ਜੋੜੋ            = 0x7c,
    c_waOp_i64_sub            = 0x7d,
    c_waOp_i64_mul            = 0x7e,

    c_waOp_refNull            = 0xd0,
    c_waOp_refIsNull          = 0xd1,
    c_waOp_refFunc            = 0xd2,
    c_waOp_refAsNonNull       = 0xd4,
    c_waOp_brOnNull           = 0xd5,
    c_waOp_brOnNonNull        = 0xd6,

    c_waOp_contNew            = 0xe0,
    c_waOp_contBind           = 0xe1,
    c_waOp_suspend            = 0xe2,
    c_waOp_resume             = 0xe3,
    c_waOp_resumeThrow        = 0xe4,
    c_waOp_resumeThrowRef     = 0xe5,
    c_waOp_switch             = 0xe6,

    c_waOp_extended           = 0xfc,
    c_waOp_atomic             = 0xfe,

    c_wa_Op_ਸਮ੍ਰਿਤੀ_Init         = 0xfc08,
    c_wa_Op_ਅੰਕੜੇ_Drop           = 0xfc09,
    c_wa_Op_ਸਮ੍ਰਿਤੀ_Copy         = 0xfc0a,
    c_wa_Op_ਸਮ੍ਰਿਤੀ_Fill         = 0xfc0b,
    c_wa_Op_ਸਾਰਣੀ_Init          = 0xfc0c,
    c_waOp_elemDrop           = 0xfc0d,
    c_wa_Op_ਸਾਰਣੀ_Copy          = 0xfc0e,
    c_wa_Op_ਸਾਰਣੀ_Grow          = 0xfc0f,
    c_wa_Op_ਸਾਰਣੀ_ਮਾਪ          = 0xfc10,
    c_wa_Op_ਸਾਰਣੀ_Fill          = 0xfc11,
    c_waOp_i64Add128          = 0xfc13,
    c_waOp_i64Sub128          = 0xfc14,
    c_waOp_i64MulWideS        = 0xfc15,
    c_waOp_i64MulWideU        = 0xfc16,

    c_waOp_atomicNotify       = 0xfe00,
    c_waOp_atomicWait32       = 0xfe01,
    c_waOp_atomicWait64       = 0xfe02,
    c_waOp_atomicFence        = 0xfe03,
    c_wa_Op_atomic_ਪਹਿਲਾ_Access  = 0xfe10,     // i32.atomic.load

    // Highest opcode each operation table actually defines below the reference
    // instructions. The tables run past these: with internal ops in DEBUG
    // builds, and with the designated 0xd0..0xd2 and 0xfc entries.
    c_wa_Op_ਆਖਰੀ_Core           = 0xc4,     // i64.extend32_s
#ਜੇ d_m3HasWideArithmetic
    c_wa_Op_ਆਖਰੀ_Extended = 0x16,     // i64.mul_wide_u
#ਨਹੀਂਤਾਂ
    c_wa_Op_ਆਖਰੀ_Extended = 0x11,     // table.fill
#ਜੇ_ਅੰਤ
    c_wa_Op_ਆਖਰੀ_Atomic = 0x4e      // i64.atomic.rmw32.cmpxchg_u
};


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_FuncRetType(ftype,i)  ((ftype)->types[(i)])
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ d_FuncArgType(ftype,i)  ((ftype)->types[(ftype)->ਅੰਕ_Rets + (i)])

//-----------------------------------------------------------------------------------------------------------------------------------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3CompilationScope {
    ਬਣਤਰ M3CompilationScope* outer;

    pc_t                       pc;                 // used by ContinueLoop's
    pc_t                       patches;
    i32                        ਡੂੰਘਾਈ;
    u16                        exit_ਢੇਰ_ਸੂਚਕਾਂਕ;
    u16                        ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ;
    //    u16                             topSlot;
    IM3FuncType                type;
    m3opcode_t                 opcode;
    bool                       isPolymorphic;
    u32                        snapshot_ਖੰਡ;      // its M3BlockStart in the snapshot map, plus one; 0 for none
} M3CompilationScope;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3CompilationScope* IM3CompilationScope;

#ਜੇ d_m3HasExceptionHandling

// One catch clause of a try_table, as Compile_TryTable reads it out of the
// immediates. The stub that CompileBlock later emits for this clause writes its
// own address back into stubSlot, which points at the pointer op_TryTable
// reserved for it in the clause table.
ਬਣਤਰ M3_ਟੈਗ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3CatchClause {
    ਬਣਤਰ M3_ਟੈਗ* ਟੈਗ;            // NULL for catch_all / catch_all_ref
    ਖਾਲੀ*         stub_ਥਾਂ;
    u32           label_ਡੂੰਘਾਈ;     // counted from outside the try block
    bool          hasRef;         // the _ref forms also hand over an exnref
} M3CatchClause;

#ਜੇ_ਅੰਤ


ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ
{
    IM3Runtime         runtime;
    IM3Module          module;

    ਬਾਈਟਾਂ_t            wasm;
    ਬਾਈਟਾਂ_t            wasm_ਸਿਰਾ;
    ਬਾਈਟਾਂ_t            ਆਖਰੀ_Opcode_ਸ਼ੁਰੂ;

    M3CompilationScope ਖੰਡ;

    IM3Function        function;

    IM3_ਕੋਡ_ਪੰਨਾ        ਪੰਨਾ;

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
    u32 ਅੰਕ_Emits;
    u32 ਅੰਕ_Opcodes;
#ਜੇ_ਅੰਤ

    u16        ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ;     // args and locals are pushed to the stack so that their slot locations can be tracked. the wasm model itself doesn't
                                                    // treat these values as being on the stack, so stackFirstDynamicIndex marks the start of the real Wasm stack
    u16        ਢੇਰ_ਸੂਚਕਾਂਕ;                 // current stack top

    u16        ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ;
    u16        ਥਾਂ_Max_Const_ਸੂਚਕਾਂਕ;          // as const's are encountered during compilation this tracks their location in the "real" stack

    u16        ਥਾਂ_ਪਹਿਲਾ_Local_ਸੂਚਕਾਂਕ;
    u16        ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ;      // numArgs + numLocals + numReservedConstants. the first mutable slot available to the compiler.

    u16        max_ਢੇਰ_Slots;

    m3slot_t   constants[d_m3_Max_Constant_ਸਾਰਣੀ_ਮਾਪ];

    // 'wasmStack' holds slot locations
    u16        wasm_ਢੇਰ[d_m3_Max_Function_ਢੇਰ_ਉਚਾਈ];
    m3type_t   type_ਢੇਰ[d_m3_Max_Function_ਢੇਰ_ਉਚਾਈ];

    // 'm3Slots' contains allocation usage counts
    u8         m3Slots[d_m3MaxFunctionSlots];

    u16        ਥਾਂ_Max_Allocated_ਸੂਚਕਾਂਕ_Plus_One;

    u16        reg_ਢੇਰ_ਸੂਚਕਾਂਕ_Plus_One[2];

    m3opcode_t ਪਿਛਲਾ_Opcode;

#ਜੇ d_m3HasExceptionHandling
    // handed from Compile_TryTable to the CompileBlock it is about to start.
    // The stubs are emitted from in there because a catch entry has to be
    // compiled against the try block's entry state, which is what CompileBlock
    // sets up
    M3CatchClause* tryClauses;
    u32            ਅੰਕ_Try_Clauses;
#ਜੇ_ਅੰਤ

    bool is_Init_ਵਿਅੰਜਕ;                 // walking a constant expression, not a function body

#ਜੇ d_m3HasGasMetering
    // The metering segment being compiled, the cost accumulated so far, and
    // where its op_UseGas left an immediate for the cost to be written back
    // into once the segment's extent is known
    ਖਾਲੀ* gasPatch;
    u32   gas_ਲਾਗਤ;
    bool  isGasSegmentOpen;
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasSnapshots
    // The function's snapshot map as it is being built, or NULL when this
    // compilation records none, and where the body the offsets in it count
    // from begins.
    M3SnapshotMap* snapshotMap;
    u32*           local_Init_ਡੂੰਘਾਈ; // lexical scope that initialized each non-defaultable local
    u32            ਸੁਰੱਖਿਅਤ_Points_ਸਮਰੱਥਾ;
    u32            ਮੁੱਲਾਂ_ਸਮਰੱਥਾ;
    u32            ਖੰਡਾਂ_ਸਮਰੱਥਾ;
    ਬਾਈਟਾਂ_t        body_ਸ਼ੁਰੂ;
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_Fold_ਸੈੱਟ_Local
    // the last emitted op, when it is a candidate for local.set destination folding.
    // foldPatchPC is NULL whenever anything was emitted (or a branch target captured) since.
    pc_t       foldPatchPC;                // address of the candidate's op word in the code stream
    m3opcode_t foldOpcode;
    u16        fold_ਢੇਰ_ਸੂਚਕਾਂਕ;             // stack index of the candidate's pushed result
    u8         foldForm;                   // operand-form index the candidate was emitted with
#ਜੇ_ਅੰਤ
} M3Compilation;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3Compilation* IM3Compilation;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਨਤੀਜਾ (*M3Compiler)(IM3Compilation, m3opcode_t);


//-----------------------------------------------------------------------------------------------------------------------------------


ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3OpInfo {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
    ਅਚਲ ਅੱਖਰ* ਅਚਲ name;
#ਜੇ_ਅੰਤ

    i8           ਢੇਰ_ਵਿੱਥ;
    u8           type;

    // index into c_compilers [], 0 for an opcode with no compiler. An index
    // rather than a pointer: it costs a byte instead of a word, and it keeps
    // the whole table free of relocations.
    u8           compiler;

    // for most operations:
    // [0]= top operand in register, [1]= top operand in stack, [2]= both operands in stack
    IM3Operation operations[4];
} M3OpInfo;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਅਚਲ M3OpInfo* IM3OpInfo;

IM3OpInfo               GetOpInfo (m3opcode_t opcode);

// The raw tables, for the DEBUG-only reverse lookup in m3_info.c. Past the last
// Wasm opcode c_operations also holds internal operations, which GetOpInfo hides.
ਬਾਹਰੀ ਅਚਲ M3OpInfo   c_operations[];
ਬਾਹਰੀ ਅਚਲ M3OpInfo   c_operationsFC[];
ਬਾਹਰੀ ਅਚਲ M3OpInfo   c_operationsFE[];
ਬਾਹਰੀ ਅਚਲ u32        c_ਅੰਕ_Operations;
ਬਾਹਰੀ ਅਚਲ u32        c_ਅੰਕ_Operations_FC;
ਬਾਹਰੀ ਅਚਲ u32        c_ਅੰਕ_Operations_FE;

ਸਥਿਰ ਅਚਲ u16        c_m3_ਰਜਿਸਟਰ_Unallocated = 0;
ਸਥਿਰ ਅਚਲ u16        c_ਥਾਂ_ਅਣਵਰਤਿਆ = 0xffff;

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਰਜਿਸਟਰ_Allocated (IM3Compilation o, u32 i_ਰਜਿਸਟਰ)
{
    ਵਾਪਸ (o->reg_ਢੇਰ_ਸੂਚਕਾਂਕ_Plus_One[i_ਰਜਿਸਟਰ] != c_m3_ਰਜਿਸਟਰ_Unallocated);
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਢੇਰ_Polymorphic (IM3Compilation o)
{
    ਵਾਪਸ o->ਖੰਡ.isPolymorphic;
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias (u16 i_ਥਾਂ)
{
    ਵਾਪਸ (i_ਥਾਂ >= d_m3_Reg0_ਥਾਂ_Alias and i_ਥਾਂ != c_ਥਾਂ_ਅਣਵਰਤਿਆ);
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_Fp_ਰਜਿਸਟਰ_ਥਾਂ_Alias (u16 i_ਥਾਂ)
{
    ਵਾਪਸ (i_ਥਾਂ == d_m3_Fp0_ਥਾਂ_Alias);
}
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Is_Int_ਰਜਿਸਟਰ_ਥਾਂ_Alias (u16 i_ਥਾਂ)
{
    ਵਾਪਸ (i_ਥਾਂ == d_m3_Reg0_ਥਾਂ_Alias);
}


// Every field is named, and named in declaration order: a mixture of designated
// and positional initializers is not valid C++, which the maintenance builds
// compile this as.
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
#  define M3OP(_name, _offset, _type, ...)  { .name = _name, .ਢੇਰ_ਵਿੱਥ = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { .name = "reserved" }
#ਨਹੀਂਤਾਂ
// Strip-off name
#  define M3OP(_name, _offset, _type, ...)  { .ਢੇਰ_ਵਿੱਥ = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { 0 }
#ਜੇ_ਅੰਤ

#ਜੇ d_m3HasFloat
#  define M3OP_F          M3OP
#ਜਾਂਜੇ d_m3NoFloatDynamic
#  define M3OP_F(n,o,t,cc,...)        M3OP(n, o, t, cc, .operations = { op_Unsupported, op_Unsupported, op_Unsupported, op_Unsupported })
#ਨਹੀਂਤਾਂ
#  define M3OP_F(...)     { 0 }
#ਜੇ_ਅੰਤ

//-----------------------------------------------------------------------------------------------------------------------------------

u16      Get_Max_ਵਰਤਿਆ_ਥਾਂ_Plus_One (IM3Compilation o);

M3_ਨਤੀਜਾ Compile_ਖੰਡ (IM3Compilation io, IM3FuncType i_ਖੰਡ_Type, m3opcode_t i_ਖੰਡ_Opcode);

M3_ਨਤੀਜਾ Compile_ਖੰਡ_Statements (IM3Compilation io);
M3_ਨਤੀਜਾ Compile_ਵਿਅੰਜਕ (IM3Compilation io, IM3FuncType i_ਨਤੀਜਾ_Type);
M3_ਨਤੀਜਾ CompileFunction (IM3Function io_function);

M3_ਨਤੀਜਾ CompileRawFunction (IM3Module io_module, IM3Function io_function, ਅਚਲ ਖਾਲੀ* i_function, ਅਚਲ ਖਾਲੀ* i_userdata);

ਬਣਤਰ M3Continuation;

#ਜੇ d_m3_Has_ਢੇਰ_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, ਬਣਤਰ M3Continuation* i_cont);
#ਜੇ_ਅੰਤ

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_compile_h
