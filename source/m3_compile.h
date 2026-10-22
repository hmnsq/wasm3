//
//  m3_compile.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#यदि_अपरिभाषितम् m3_compile_h
#परिभाषय m3_compile_h

#अन्तर्भाव "m3_code.h"
#अन्तर्भाव "m3_exec_defs.h"
#अन्तर्भाव "m3_function.h"

d_m3BeginExternC

गणना {
    c_wa_Op_खण्डम्              = 0x02,
    c_waOp_loop               = 0x03,
    c_waOp_if                 = 0x04,
    c_waOp_else               = 0x05,
    c_waOp_throw              = 0x08,
    c_waOp_throwRef           = 0x0a,
    c_wa_Op_अन्तः                = 0x0b,
    c_waOp_branch             = 0x0c,
    c_wa_Op_branch_सारणी        = 0x0e,
    c_waOp_branchIf           = 0x0d,
    c_waOp_return             = 0x0f,
    c_waOp_call               = 0x10,
    c_wa_Op_call_परोक्षम्       = 0x11,
    c_waOp_returnCall         = 0x12,
    c_wa_Op_return_Call_परोक्षम् = 0x13,
    c_waOp_callRef            = 0x14,
    c_waOp_returnCallRef      = 0x15,
    c_waOp_getLocal           = 0x20,
    c_wa_Op_समुच्चयः_Local           = 0x21,
    c_waOp_teeLocal           = 0x22,

    c_waOp_selectTyped        = 0x1c,
    c_wa_Op_try_सारणी           = 0x1f,

    c_waOp_getGlobal          = 0x23,
    c_wa_Op_सारणी_Get           = 0x25,
    c_wa_Op_सारणी_समुच्चयः           = 0x26,

    c_waOp_store_f32          = 0x38,
    c_waOp_store_f64          = 0x39,

    c_waOp_i32_const          = 0x41,
    c_waOp_i64_const          = 0x42,
    c_waOp_f32_const          = 0x43,
    c_waOp_f64_const          = 0x44,

    // the arithmetic the extended-const proposal admits into constant expressions
    c_wa_Op_i32_सङ्कलय            = 0x6a,
    c_waOp_i32_sub            = 0x6b,
    c_waOp_i32_mul            = 0x6c,
    c_wa_Op_i64_सङ्कलय            = 0x7c,
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

    c_wa_Op_स्मृतिः_Init         = 0xfc08,
    c_wa_Op_दत्त_Drop           = 0xfc09,
    c_wa_Op_स्मृतिः_Copy         = 0xfc0a,
    c_wa_Op_स्मृतिः_Fill         = 0xfc0b,
    c_wa_Op_सारणी_Init          = 0xfc0c,
    c_waOp_elemDrop           = 0xfc0d,
    c_wa_Op_सारणी_Copy          = 0xfc0e,
    c_wa_Op_सारणी_Grow          = 0xfc0f,
    c_wa_Op_सारणी_आकार          = 0xfc10,
    c_wa_Op_सारणी_Fill          = 0xfc11,
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
    c_wa_Op_चरमः_Core           = 0xc4,     // i64.extend32_s
#यदि d_m3HasWideArithmetic
    c_wa_Op_चरमः_Extended = 0x16,     // i64.mul_wide_u
#अन्यथा
    c_wa_Op_चरमः_Extended = 0x11,     // table.fill
#यदि_अन्तः
    c_wa_Op_चरमः_Atomic = 0x4e      // i64.atomic.rmw32.cmpxchg_u
};


#परिभाषय d_FuncRetType(ftype,i)  ((ftype)->types[(i)])
#परिभाषय d_FuncArgType(ftype,i)  ((ftype)->types[(ftype)->अङ्कः_Rets + (i)])

//-----------------------------------------------------------------------------------------------------------------------------------

प्रकारनिर्देश संरचना M3CompilationScope {
    संरचना M3CompilationScope* outer;

    pc_t                       pc;                 // used by ContinueLoop's
    pc_t                       patches;
    i32                        गभीरता;
    u16                        exit_राशिः_सूचकांक;
    u16                        खण्डम्_राशिः_सूचकांक;
    //    u16                             topSlot;
    IM3FuncType                type;
    m3opcode_t                 opcode;
    bool                       isPolymorphic;
    u32                        snapshot_खण्डम्;      // its M3BlockStart in the snapshot map, plus one; 0 for none
} M3CompilationScope;

प्रकारनिर्देश M3CompilationScope* IM3CompilationScope;

#यदि d_m3HasExceptionHandling

// One catch clause of a try_table, as Compile_TryTable reads it out of the
// immediates. The stub that CompileBlock later emits for this clause writes its
// own address back into stubSlot, which points at the pointer op_TryTable
// reserved for it in the clause table.
संरचना M3_चिह्नकम्;

प्रकारनिर्देश संरचना M3CatchClause {
    संरचना M3_चिह्नकम्* चिह्नकम्;            // NULL for catch_all / catch_all_ref
    शून्य*         stubSlot;
    u32           label_गभीरता;     // counted from outside the try block
    bool          hasRef;         // the _ref forms also hand over an exnref
} M3CatchClause;

#यदि_अन्तः


प्रकारनिर्देश संरचना
{
    IM3Runtime         runtime;
    IM3Module          module;

    अष्टकानि_t            wasm;
    अष्टकानि_t            wasm_अन्तः;
    अष्टकानि_t            चरमः_Opcode_Start;

    M3CompilationScope खण्डम्;

    IM3Function        function;

    IM3_संहिता_Page        page;

#यदि_परिभाषितम् DEBUG
    u32 अङ्कः_Emits;
    u32 अङ्कः_Opcodes;
#यदि_अन्तः

    u16        राशिः_First_Dynamic_सूचकांक;     // args and locals are pushed to the stack so that their slot locations can be tracked. the wasm model itself doesn't
                                                    // treat these values as being on the stack, so stackFirstDynamicIndex marks the start of the real Wasm stack
    u16        राशिः_सूचकांक;                 // current stack top

    u16        slot_First_Const_सूचकांक;
    u16        slot_Max_Const_सूचकांक;          // as const's are encountered during compilation this tracks their location in the "real" stack

    u16        slot_First_Local_सूचकांक;
    u16        slot_First_Dynamic_सूचकांक;      // numArgs + numLocals + numReservedConstants. the first mutable slot available to the compiler.

    u16        max_राशिः_Slots;

    m3slot_t   constants[d_m3_Max_Constant_सारणी_आकार];

    // 'wasmStack' holds slot locations
    u16        wasm_राशिः[d_m3_Max_Function_राशिः_Height];
    m3type_t   type_राशिः[d_m3_Max_Function_राशिः_Height];

    // 'm3Slots' contains allocation usage counts
    u8         m3Slots[d_m3MaxFunctionSlots];

    u16        slot_Max_Allocated_सूचकांक_Plus_One;

    u16        reg_राशिः_सूचकांक_Plus_One[2];

    m3opcode_t previousOpcode;

#यदि d_m3HasExceptionHandling
    // handed from Compile_TryTable to the CompileBlock it is about to start.
    // The stubs are emitted from in there because a catch entry has to be
    // compiled against the try block's entry state, which is what CompileBlock
    // sets up
    M3CatchClause* tryClauses;
    u32            अङ्कः_Try_Clauses;
#यदि_अन्तः

    bool is_Init_व्यञ्जकम्;                 // walking a constant expression, not a function body

#यदि d_m3HasGasMetering
    // The metering segment being compiled, the cost accumulated so far, and
    // where its op_UseGas left an immediate for the cost to be written back
    // into once the segment's extent is known
    शून्य* gasPatch;
    u32   gasCost;
    bool  isGasSegmentOpen;
#यदि_अन्तः

#यदि d_m3HasSnapshots
    // The function's snapshot map as it is being built, or NULL when this
    // compilation records none, and where the body the offsets in it count
    // from begins.
    M3SnapshotMap* snapshotMap;
    u32*           local_Init_गभीरता; // lexical scope that initialized each non-defaultable local
    u32            safe_Points_क्षमता;
    u32            मानानि_क्षमता;
    u32            खण्डानि_क्षमता;
    अष्टकानि_t        bodyStart;
#यदि_अन्तः

#यदि d_m3_Fold_समुच्चयः_Local
    // the last emitted op, when it is a candidate for local.set destination folding.
    // foldPatchPC is NULL whenever anything was emitted (or a branch target captured) since.
    pc_t       foldPatchPC;                // address of the candidate's op word in the code stream
    m3opcode_t foldOpcode;
    u16        fold_राशिः_सूचकांक;             // stack index of the candidate's pushed result
    u8         foldForm;                   // operand-form index the candidate was emitted with
#यदि_अन्तः
} M3Compilation;

प्रकारनिर्देश M3Compilation* IM3Compilation;

प्रकारनिर्देश M3_परिणाम (*M3Compiler)(IM3Compilation, m3opcode_t);


//-----------------------------------------------------------------------------------------------------------------------------------


प्रकारनिर्देश संरचना M3OpInfo {
#यदि_परिभाषितम् DEBUG
    ध्रुव वर्ण* ध्रुव name;
#यदि_अन्तः

    i8           राशिः_विस्थापनम्;
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

स्थिर ध्रुव u16        c_m3_पञ्जिकाकोशः_Unallocated = 0;
स्थिर ध्रुव u16        c_slot_अप्रयुक्तम् = 0xffff;

स्थिर अन्तःस्थ
bool Is_पञ्जिकाकोशः_Allocated (IM3Compilation o, u32 i_पञ्जिकाकोशः)
{
    प्रत्यागम (o->reg_राशिः_सूचकांक_Plus_One[i_पञ्जिकाकोशः] != c_m3_पञ्जिकाकोशः_Unallocated);
}

स्थिर अन्तःस्थ
bool Is_राशिः_Polymorphic (IM3Compilation o)
{
    प्रत्यागम o->खण्डम्.isPolymorphic;
}

स्थिर अन्तःस्थ
bool Is_पञ्जिकाकोशः_Slot_Alias (u16 i_slot)
{
    प्रत्यागम (i_slot >= d_m3Reg0SlotAlias and i_slot != c_slot_अप्रयुक्तम्);
}
स्थिर अन्तःस्थ
bool Is_Fp_पञ्जिकाकोशः_Slot_Alias (u16 i_slot)
{
    प्रत्यागम (i_slot == d_m3Fp0SlotAlias);
}
स्थिर अन्तःस्थ
bool Is_Int_पञ्जिकाकोशः_Slot_Alias (u16 i_slot)
{
    प्रत्यागम (i_slot == d_m3Reg0SlotAlias);
}


// Every field is named, and named in declaration order: a mixture of designated
// and positional initializers is not valid C++, which the maintenance builds
// compile this as.
#यदि_परिभाषितम् DEBUG
#  define M3OP(_name, _offset, _type, ...)  { .name = _name, .राशिः_विस्थापनम् = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { .name = "reserved" }
#अन्यथा
// Strip-off name
#  define M3OP(_name, _offset, _type, ...)  { .राशिः_विस्थापनम् = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { 0 }
#यदि_अन्तः

#यदि d_m3HasFloat
#  define M3OP_F          M3OP
#नोचेद्यदि d_m3NoFloatDynamic
#  define M3OP_F(n,o,t,cc,...)        M3OP(n, o, t, cc, .operations = { op_Unsupported, op_Unsupported, op_Unsupported, op_Unsupported })
#अन्यथा
#  define M3OP_F(...)     { 0 }
#यदि_अन्तः

//-----------------------------------------------------------------------------------------------------------------------------------

u16      Get_Max_प्रयुक्तम्_Slot_Plus_One (IM3Compilation o);

M3_परिणाम Compile_खण्डम् (IM3Compilation io, IM3FuncType i_खण्डम्_Type, m3opcode_t i_खण्डम्_Opcode);

M3_परिणाम Compile_खण्डम्_Statements (IM3Compilation io);
M3_परिणाम CompileExpression (IM3Compilation io, IM3FuncType i_परिणाम_Type);
M3_परिणाम CompileFunction (IM3Function io_function);

M3_परिणाम CompileRawFunction (IM3Module io_module, IM3Function io_function, ध्रुव शून्य* i_function, ध्रुव शून्य* i_userdata);

संरचना M3Continuation;

#यदि d_m3_Has_राशिः_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, संरचना M3Continuation* i_cont);
#यदि_अन्तः

d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_compile_h
