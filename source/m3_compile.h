//
//  m3_compile.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ifndef m3_compile_h
#define m3_compile_h

#अन्तर्भाव "m3_code.h"
#अन्तर्भाव "m3_exec_defs.h"
#अन्तर्भाव "m3_function.h"

d_m3BeginExternC

गणना {
    c_waOp_block              = 0x02,
    c_waOp_loop               = 0x03,
    c_waOp_if                 = 0x04,
    c_waOp_else               = 0x05,
    c_waOp_throw              = 0x08,
    c_waOp_throwRef           = 0x0a,
    c_waOp_end                = 0x0b,
    c_waOp_branch             = 0x0c,
    c_waOp_branchTable        = 0x0e,
    c_waOp_branchIf           = 0x0d,
    c_waOp_return             = 0x0f,
    c_waOp_call               = 0x10,
    c_waOp_callIndirect       = 0x11,
    c_waOp_returnCall         = 0x12,
    c_waOp_returnCallIndirect = 0x13,
    c_waOp_callRef            = 0x14,
    c_waOp_returnCallRef      = 0x15,
    c_waOp_getLocal           = 0x20,
    c_waOp_setLocal           = 0x21,
    c_waOp_teeLocal           = 0x22,

    c_waOp_selectTyped        = 0x1c,
    c_waOp_tryTable           = 0x1f,

    c_waOp_getGlobal          = 0x23,
    c_waOp_tableGet           = 0x25,
    c_waOp_tableSet           = 0x26,

    c_waOp_store_f32          = 0x38,
    c_waOp_store_f64          = 0x39,

    c_waOp_i32_const          = 0x41,
    c_waOp_i64_const          = 0x42,
    c_waOp_f32_const          = 0x43,
    c_waOp_f64_const          = 0x44,

    // the arithmetic the extended-const proposal admits into constant expressions
    c_waOp_i32_add            = 0x6a,
    c_waOp_i32_sub            = 0x6b,
    c_waOp_i32_mul            = 0x6c,
    c_waOp_i64_add            = 0x7c,
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

    c_waOp_memoryInit         = 0xfc08,
    c_waOp_dataDrop           = 0xfc09,
    c_waOp_memoryCopy         = 0xfc0a,
    c_waOp_memoryFill         = 0xfc0b,
    c_waOp_tableInit          = 0xfc0c,
    c_waOp_elemDrop           = 0xfc0d,
    c_waOp_tableCopy          = 0xfc0e,
    c_waOp_tableGrow          = 0xfc0f,
    c_waOp_tableSize          = 0xfc10,
    c_waOp_tableFill          = 0xfc11,
    c_waOp_i64Add128          = 0xfc13,
    c_waOp_i64Sub128          = 0xfc14,
    c_waOp_i64MulWideS        = 0xfc15,
    c_waOp_i64MulWideU        = 0xfc16,

    c_waOp_atomicNotify       = 0xfe00,
    c_waOp_atomicWait32       = 0xfe01,
    c_waOp_atomicWait64       = 0xfe02,
    c_waOp_atomicFence        = 0xfe03,
    c_waOp_atomicFirstAccess  = 0xfe10,     // i32.atomic.load

    // Highest opcode each operation table actually defines below the reference
    // instructions. The tables run past these: with internal ops in DEBUG
    // builds, and with the designated 0xd0..0xd2 and 0xfc entries.
    c_waOp_lastCore           = 0xc4,     // i64.extend32_s
#सचे d_m3HasWideArithmetic
    c_waOp_lastExtended = 0x16,     // i64.mul_wide_u
#अञ्ञथा
    c_waOp_lastExtended = 0x11,     // table.fill
#endif
    c_waOp_lastAtomic = 0x4e      // i64.atomic.rmw32.cmpxchg_u
};


#define d_FuncRetType(ftype,i)  ((ftype)->types[(i)])
#define d_FuncArgType(ftype,i)  ((ftype)->types[(ftype)->अङ्कः_Rets + (i)])

//-----------------------------------------------------------------------------------------------------------------------------------

प्रकारनिर्देश संरचना M3CompilationScope {
    संरचना M3CompilationScope* outer;

    pc_t                       pc;                 // used by ContinueLoop's
    pc_t                       patches;
    i32                        depth;
    u16                        exit_राशिः_Index;
    u16                        block_राशिः_Index;
    //    u16                             topSlot;
    IM3FuncType                type;
    m3opcode_t                 opcode;
    bool                       isPolymorphic;
    u32                        snapshotBlock;      // its M3BlockStart in the snapshot map, plus one; 0 for none
} M3CompilationScope;

प्रकारनिर्देश M3CompilationScope* IM3CompilationScope;

#सचे d_m3HasExceptionHandling

// One catch clause of a try_table, as Compile_TryTable reads it out of the
// immediates. The stub that CompileBlock later emits for this clause writes its
// own address back into stubSlot, which points at the pointer op_TryTable
// reserved for it in the clause table.
संरचना M3Tag;

प्रकारनिर्देश संरचना M3CatchClause {
    संरचना M3Tag* tag;            // NULL for catch_all / catch_all_ref
    शून्य*         stubSlot;
    u32           labelDepth;     // counted from outside the try block
    bool          hasRef;         // the _ref forms also hand over an exnref
} M3CatchClause;

#endif


प्रकारनिर्देश संरचना
{
    IM3Runtime         runtime;
    IM3Module          module;

    bytes_t            wasm;
    bytes_t            wasmEnd;
    bytes_t            lastOpcodeStart;

    M3CompilationScope block;

    IM3Function        function;

    IM3CodePage        page;

#ifdef DEBUG
    u32 अङ्कः_Emits;
    u32 अङ्कः_Opcodes;
#endif

    u16        राशिः_First_Dynamic_Index;     // args and locals are pushed to the stack so that their slot locations can be tracked. the wasm model itself doesn't
                                                    // treat these values as being on the stack, so stackFirstDynamicIndex marks the start of the real Wasm stack
    u16        राशिः_Index;                 // current stack top

    u16        slotFirstConstIndex;
    u16        slotMaxConstIndex;          // as const's are encountered during compilation this tracks their location in the "real" stack

    u16        slotFirstLocalIndex;
    u16        slotFirstDynamicIndex;      // numArgs + numLocals + numReservedConstants. the first mutable slot available to the compiler.

    u16        max_राशिः_Slots;

    m3slot_t   constants[d_m3MaxConstantTableSize];

    // 'wasmStack' holds slot locations
    u16        wasm_राशिः[d_m3_Max_Function_राशिः_Height];
    m3type_t   type_राशिः[d_m3_Max_Function_राशिः_Height];

    // 'm3Slots' contains allocation usage counts
    u8         m3Slots[d_m3MaxFunctionSlots];

    u16        slotMaxAllocatedIndexPlusOne;

    u16        reg_राशिः_Index_Plus_One[2];

    m3opcode_t previousOpcode;

#सचे d_m3HasExceptionHandling
    // handed from Compile_TryTable to the CompileBlock it is about to start.
    // The stubs are emitted from in there because a catch entry has to be
    // compiled against the try block's entry state, which is what CompileBlock
    // sets up
    M3CatchClause* tryClauses;
    u32            अङ्कः_Try_Clauses;
#endif

    bool is_Init_व्यञ्जकम्;                 // walking a constant expression, not a function body

#सचे d_m3HasGasMetering
    // The metering segment being compiled, the cost accumulated so far, and
    // where its op_UseGas left an immediate for the cost to be written back
    // into once the segment's extent is known
    शून्य* gasPatch;
    u32   gasCost;
    bool  isGasSegmentOpen;
#endif

#सचे d_m3HasSnapshots
    // The function's snapshot map as it is being built, or NULL when this
    // compilation records none, and where the body the offsets in it count
    // from begins.
    M3SnapshotMap* snapshotMap;
    u32*           localInitDepth; // lexical scope that initialized each non-defaultable local
    u32            safePointsCapacity;
    u32            मानानि_Capacity;
    u32            blocksCapacity;
    bytes_t        bodyStart;
#endif

#सचे d_m3FoldSetLocal
    // the last emitted op, when it is a candidate for local.set destination folding.
    // foldPatchPC is NULL whenever anything was emitted (or a branch target captured) since.
    pc_t       foldPatchPC;                // address of the candidate's op word in the code stream
    m3opcode_t foldOpcode;
    u16        fold_राशिः_Index;             // stack index of the candidate's pushed result
    u8         foldForm;                   // operand-form index the candidate was emitted with
#endif
} M3Compilation;

प्रकारनिर्देश M3Compilation* IM3Compilation;

प्रकारनिर्देश M3Result (*M3Compiler)(IM3Compilation, m3opcode_t);


//-----------------------------------------------------------------------------------------------------------------------------------


प्रकारनिर्देश संरचना M3OpInfo {
#ifdef DEBUG
    ध्रुव वर्ण* ध्रुव name;
#endif

    i8           राशिः_Offset;
    u8           type;

    // index into c_compilers [], 0 for an opcode with no compiler. An index
    // rather than a pointer: it costs a byte instead of a word, and it keeps
    // the whole table free of relocations.
    u8           compiler;

    // for most operations:
    // [0]= top operand in register, [1]= top operand in stack, [2]= both operands in stack
    IM3Operation operations[4];
} M3OpInfo;

प्रकारनिर्देश ध्रुव M3OpInfo* IM3OpInfo;

IM3OpInfo               GetOpInfo (m3opcode_t opcode);

// The raw tables, for the DEBUG-only reverse lookup in m3_info.c. Past the last
// Wasm opcode c_operations also holds internal operations, which GetOpInfo hides.
बाह्य ध्रुव M3OpInfo   c_operations[];
बाह्य ध्रुव M3OpInfo   c_operationsFC[];
बाह्य ध्रुव M3OpInfo   c_operationsFE[];
बाह्य ध्रुव u32        c_अङ्कः_Operations;
बाह्य ध्रुव u32        c_अङ्कः_Operations_FC;
बाह्य ध्रुव u32        c_अङ्कः_Operations_FE;

स्थिर ध्रुव u16        c_m3RegisterUnallocated = 0;
स्थिर ध्रुव u16        c_slotUnused = 0xffff;

स्थिर अन्तःस्थ
bool IsRegisterAllocated (IM3Compilation o, u32 i_register)
{
    पटिनिवत्त (o->reg_राशिः_Index_Plus_One[i_register] != c_m3RegisterUnallocated);
}

स्थिर अन्तःस्थ
bool Is_राशिः_Polymorphic (IM3Compilation o)
{
    पटिनिवत्त o->block.isPolymorphic;
}

स्थिर अन्तःस्थ
bool IsRegisterSlotAlias (u16 i_slot)
{
    पटिनिवत्त (i_slot >= d_m3Reg0SlotAlias and i_slot != c_slotUnused);
}
स्थिर अन्तःस्थ
bool IsFpRegisterSlotAlias (u16 i_slot)
{
    पटिनिवत्त (i_slot == d_m3Fp0SlotAlias);
}
स्थिर अन्तःस्थ
bool IsIntRegisterSlotAlias (u16 i_slot)
{
    पटिनिवत्त (i_slot == d_m3Reg0SlotAlias);
}


// Every field is named, and named in declaration order: a mixture of designated
// and positional initializers is not valid C++, which the maintenance builds
// compile this as.
#ifdef DEBUG
#  define M3OP(_name, _offset, _type, ...)  { .name = _name, .राशिः_Offset = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { .name = "reserved" }
#अञ्ञथा
// Strip-off name
#  define M3OP(_name, _offset, _type, ...)  { .राशिः_Offset = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { 0 }
#endif

#सचे d_m3HasFloat
#  define M3OP_F          M3OP
#नोचेद्यदि d_m3NoFloatDynamic
#  define M3OP_F(n,o,t,cc,...)        M3OP(n, o, t, cc, .operations = { op_Unsupported, op_Unsupported, op_Unsupported, op_Unsupported })
#अञ्ञथा
#  define M3OP_F(...)     { 0 }
#endif

//-----------------------------------------------------------------------------------------------------------------------------------

u16      GetMaxUsedSlotPlusOne (IM3Compilation o);

M3Result CompileBlock (IM3Compilation io, IM3FuncType i_blockType, m3opcode_t i_blockOpcode);

M3Result CompileBlockStatements (IM3Compilation io);
M3Result CompileExpression (IM3Compilation io, IM3FuncType i_resultType);
M3Result CompileFunction (IM3Function io_function);

M3Result CompileRawFunction (IM3Module io_module, IM3Function io_function, ध्रुव शून्य* i_function, ध्रुव शून्य* i_userdata);

संरचना M3Continuation;

#सचे d_m3_Has_राशिः_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, संरचना M3Continuation* i_cont);
#endif

d_m3EndExternC

#endif // m3_compile_h
