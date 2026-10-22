//
//  m3_compile.h
//
//  Created by Steven Massey on 4/17/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#اگر_تعریف_نشده m3_compile_h
#تعریف_کن m3_compile_h

#ضمن "m3_code.h"
#ضمن "m3_exec_defs.h"
#ضمن "m3_function.h"

d_m3BeginExternC

تعداد {
    c_wa_Op_بلوک              = 0x02,
    c_waOp_loop               = 0x03,
    c_waOp_if                 = 0x04,
    c_waOp_else               = 0x05,
    c_waOp_throw              = 0x08,
    c_waOp_throwRef           = 0x0a,
    c_wa_Op_انتها                = 0x0b,
    c_waOp_branch             = 0x0c,
    c_wa_Op_branch_جدول        = 0x0e,
    c_waOp_branchIf           = 0x0d,
    c_waOp_return             = 0x0f,
    c_waOp_call               = 0x10,
    c_wa_Op_call_غیرمستقیم       = 0x11,
    c_waOp_returnCall         = 0x12,
    c_wa_Op_return_Call_غیرمستقیم = 0x13,
    c_waOp_callRef            = 0x14,
    c_waOp_returnCallRef      = 0x15,
    c_waOp_getLocal           = 0x20,
    c_wa_Op_مجموعه_Local           = 0x21,
    c_waOp_teeLocal           = 0x22,

    c_waOp_selectTyped        = 0x1c,
    c_wa_Op_try_جدول           = 0x1f,

    c_waOp_getGlobal          = 0x23,
    c_wa_Op_جدول_Get           = 0x25,
    c_wa_Op_جدول_مجموعه           = 0x26,

    c_waOp_store_f32          = 0x38,
    c_waOp_store_f64          = 0x39,

    c_waOp_i32_const          = 0x41,
    c_waOp_i64_const          = 0x42,
    c_waOp_f32_const          = 0x43,
    c_waOp_f64_const          = 0x44,

    // the arithmetic the extended-const proposal admits into constant expressions
    c_wa_Op_i32_جمع_کن            = 0x6a,
    c_waOp_i32_sub            = 0x6b,
    c_waOp_i32_mul            = 0x6c,
    c_wa_Op_i64_جمع_کن            = 0x7c,
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

    c_wa_Op_حافظه_Init         = 0xfc08,
    c_wa_Op_داده_Drop           = 0xfc09,
    c_wa_Op_حافظه_Copy         = 0xfc0a,
    c_wa_Op_حافظه_Fill         = 0xfc0b,
    c_wa_Op_جدول_Init          = 0xfc0c,
    c_waOp_elemDrop           = 0xfc0d,
    c_wa_Op_جدول_Copy          = 0xfc0e,
    c_wa_Op_جدول_Grow          = 0xfc0f,
    c_wa_Op_جدول_اندازه          = 0xfc10,
    c_wa_Op_جدول_Fill          = 0xfc11,
    c_waOp_i64Add128          = 0xfc13,
    c_waOp_i64Sub128          = 0xfc14,
    c_waOp_i64MulWideS        = 0xfc15,
    c_waOp_i64MulWideU        = 0xfc16,

    c_waOp_atomicNotify       = 0xfe00,
    c_waOp_atomicWait32       = 0xfe01,
    c_waOp_atomicWait64       = 0xfe02,
    c_waOp_atomicFence        = 0xfe03,
    c_wa_Op_atomic_اولی_Access  = 0xfe10,     // i32.atomic.load

    // Highest opcode each operation table actually defines below the reference
    // instructions. The tables run past these: with internal ops in DEBUG
    // builds, and with the designated 0xd0..0xd2 and 0xfc entries.
    c_wa_Op_آخرین_Core           = 0xc4,     // i64.extend32_s
#إذا d_m3HasWideArithmetic
    c_wa_Op_آخرین_Extended = 0x16,     // i64.mul_wide_u
#وإلا
    c_wa_Op_آخرین_Extended = 0x11,     // table.fill
#پایان_اگر
    c_wa_Op_آخرین_Atomic = 0x4e      // i64.atomic.rmw32.cmpxchg_u
};


#تعریف_کن d_FuncRetType(ftype,i)  ((ftype)->types[(i)])
#تعریف_کن d_FuncArgType(ftype,i)  ((ftype)->types[(ftype)->رقم_Rets + (i)])

//-----------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3CompilationScope {
    بنية M3CompilationScope* outer;

    pc_t                       pc;                 // used by ContinueLoop's
    pc_t                       patches;
    i32                        عمق;
    u16                        exit_پشته_شاخص;
    u16                        بلوک_پشته_شاخص;
    //    u16                             topSlot;
    IM3FuncType                type;
    m3opcode_t                 opcode;
    bool                       isPolymorphic;
    u32                        snapshot_بلوک;      // its M3BlockStart in the snapshot map, plus one; 0 for none
} M3CompilationScope;

تعريفنوع M3CompilationScope* IM3CompilationScope;

#إذا d_m3HasExceptionHandling

// One catch clause of a try_table, as Compile_TryTable reads it out of the
// immediates. The stub that CompileBlock later emits for this clause writes its
// own address back into stubSlot, which points at the pointer op_TryTable
// reserved for it in the clause table.
بنية M3_برچسب;

تعريفنوع بنية M3CatchClause {
    بنية M3_برچسب* برچسب;            // NULL for catch_all / catch_all_ref
    فراغ*         stub_جایگاه;
    u32           label_عمق;     // counted from outside the try block
    bool          hasRef;         // the _ref forms also hand over an exnref
} M3CatchClause;

#پایان_اگر


تعريفنوع بنية
{
    IM3Runtime         runtime;
    IM3Module          module;

    بایتها_t            wasm;
    بایتها_t            wasm_انتها;
    بایتها_t            آخرین_Opcode_شروع;

    M3CompilationScope بلوک;

    IM3Function        function;

    IM3_کد_صفحه        صفحه;

#اگر_تعریف_شده DEBUG
    u32 رقم_Emits;
    u32 رقم_Opcodes;
#پایان_اگر

    u16        پشته_اولی_Dynamic_شاخص;     // args and locals are pushed to the stack so that their slot locations can be tracked. the wasm model itself doesn't
                                                    // treat these values as being on the stack, so stackFirstDynamicIndex marks the start of the real Wasm stack
    u16        پشته_شاخص;                 // current stack top

    u16        جایگاه_اولی_Const_شاخص;
    u16        جایگاه_Max_Const_شاخص;          // as const's are encountered during compilation this tracks their location in the "real" stack

    u16        جایگاه_اولی_Local_شاخص;
    u16        جایگاه_اولی_Dynamic_شاخص;      // numArgs + numLocals + numReservedConstants. the first mutable slot available to the compiler.

    u16        max_پشته_Slots;

    m3slot_t   constants[d_m3_Max_Constant_جدول_اندازه];

    // 'wasmStack' holds slot locations
    u16        wasm_پشته[d_m3_Max_Function_پشته_ارتفاع];
    m3type_t   type_پشته[d_m3_Max_Function_پشته_ارتفاع];

    // 'm3Slots' contains allocation usage counts
    u8         m3Slots[d_m3MaxFunctionSlots];

    u16        جایگاه_Max_Allocated_شاخص_Plus_One;

    u16        reg_پشته_شاخص_Plus_One[2];

    m3opcode_t قبلی_Opcode;

#إذا d_m3HasExceptionHandling
    // handed from Compile_TryTable to the CompileBlock it is about to start.
    // The stubs are emitted from in there because a catch entry has to be
    // compiled against the try block's entry state, which is what CompileBlock
    // sets up
    M3CatchClause* tryClauses;
    u32            رقم_Try_Clauses;
#پایان_اگر

    bool is_Init_عبارت;                 // walking a constant expression, not a function body

#إذا d_m3HasGasMetering
    // The metering segment being compiled, the cost accumulated so far, and
    // where its op_UseGas left an immediate for the cost to be written back
    // into once the segment's extent is known
    فراغ* gasPatch;
    u32   gas_هزینه;
    bool  isGasSegmentOpen;
#پایان_اگر

#إذا d_m3HasSnapshots
    // The function's snapshot map as it is being built, or NULL when this
    // compilation records none, and where the body the offsets in it count
    // from begins.
    M3SnapshotMap* snapshotMap;
    u32*           local_Init_عمق; // lexical scope that initialized each non-defaultable local
    u32            امن_Points_ظرفیت;
    u32            مقادیر_ظرفیت;
    u32            بلوکها_ظرفیت;
    بایتها_t        body_شروع;
#پایان_اگر

#إذا d_m3_Fold_مجموعه_Local
    // the last emitted op, when it is a candidate for local.set destination folding.
    // foldPatchPC is NULL whenever anything was emitted (or a branch target captured) since.
    pc_t       foldPatchPC;                // address of the candidate's op word in the code stream
    m3opcode_t foldOpcode;
    u16        fold_پشته_شاخص;             // stack index of the candidate's pushed result
    u8         foldForm;                   // operand-form index the candidate was emitted with
#پایان_اگر
} M3Compilation;

تعريفنوع M3Compilation* IM3Compilation;

تعريفنوع M3_نتیجه (*M3Compiler)(IM3Compilation, m3opcode_t);


//-----------------------------------------------------------------------------------------------------------------------------------


تعريفنوع بنية M3OpInfo {
#اگر_تعریف_شده DEBUG
    ثابت حرف* ثابت name;
#پایان_اگر

    i8           پشته_جابه_جایی;
    u8           type;

    // index into c_compilers [], 0 for an opcode with no compiler. An index
    // rather than a pointer: it costs a byte instead of a word, and it keeps
    // the whole table free of relocations.
    u8           compiler;

    // for most operations:
    // [0]= top operand in register, [1]= top operand in stack, [2]= both operands in stack
    IM3Operation operations[4];
} M3OpInfo;

تعريفنوع ثابت M3OpInfo* IM3OpInfo;

IM3OpInfo               GetOpInfo (m3opcode_t opcode);

// The raw tables, for the DEBUG-only reverse lookup in m3_info.c. Past the last
// Wasm opcode c_operations also holds internal operations, which GetOpInfo hides.
خارجي ثابت M3OpInfo   c_operations[];
خارجي ثابت M3OpInfo   c_operationsFC[];
خارجي ثابت M3OpInfo   c_operationsFE[];
خارجي ثابت u32        c_رقم_Operations;
خارجي ثابت u32        c_رقم_Operations_FC;
خارجي ثابت u32        c_رقم_Operations_FE;

ساكن ثابت u16        c_m3_ثبات_Unallocated = 0;
ساكن ثابت u16        c_جایگاه_استفادهنشده = 0xffff;

ساكن مضمن
bool Is_ثبات_Allocated (IM3Compilation o, u32 i_ثبات)
{
    أرجع (o->reg_پشته_شاخص_Plus_One[i_ثبات] != c_m3_ثبات_Unallocated);
}

ساكن مضمن
bool Is_پشته_Polymorphic (IM3Compilation o)
{
    أرجع o->بلوک.isPolymorphic;
}

ساكن مضمن
bool Is_ثبات_جایگاه_Alias (u16 i_جایگاه)
{
    أرجع (i_جایگاه >= d_m3_Reg0_جایگاه_Alias and i_جایگاه != c_جایگاه_استفادهنشده);
}
ساكن مضمن
bool Is_Fp_ثبات_جایگاه_Alias (u16 i_جایگاه)
{
    أرجع (i_جایگاه == d_m3_Fp0_جایگاه_Alias);
}
ساكن مضمن
bool Is_Int_ثبات_جایگاه_Alias (u16 i_جایگاه)
{
    أرجع (i_جایگاه == d_m3_Reg0_جایگاه_Alias);
}


// Every field is named, and named in declaration order: a mixture of designated
// and positional initializers is not valid C++, which the maintenance builds
// compile this as.
#اگر_تعریف_شده DEBUG
#  define M3OP(_name, _offset, _type, ...)  { .name = _name, .پشته_جابه_جایی = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { .name = "reserved" }
#وإلا
// Strip-off name
#  define M3OP(_name, _offset, _type, ...)  { .پشته_جابه_جایی = _offset, .type = _type, __VA_ARGS__ }
#  define M3OP_RESERVED                     { 0 }
#پایان_اگر

#إذا d_m3HasFloat
#  define M3OP_F          M3OP
#وإلاإذا d_m3NoFloatDynamic
#  define M3OP_F(n,o,t,cc,...)        M3OP(n, o, t, cc, .operations = { op_Unsupported, op_Unsupported, op_Unsupported, op_Unsupported })
#وإلا
#  define M3OP_F(...)     { 0 }
#پایان_اگر

//-----------------------------------------------------------------------------------------------------------------------------------

u16      Get_Max_استفاده_شده_جایگاه_Plus_One (IM3Compilation o);

M3_نتیجه Compile_بلوک (IM3Compilation io, IM3FuncType i_بلوک_Type, m3opcode_t i_بلوک_Opcode);

M3_نتیجه Compile_بلوک_Statements (IM3Compilation io);
M3_نتیجه Compile_عبارت (IM3Compilation io, IM3FuncType i_نتیجه_Type);
M3_نتیجه CompileFunction (IM3Function io_function);

M3_نتیجه CompileRawFunction (IM3Module io_module, IM3Function io_function, ثابت فراغ* i_function, ثابت فراغ* i_userdata);

بنية M3Continuation;

#إذا d_m3_Has_پشته_Switching
m3ret_t ResumeContinuation (IM3Runtime i_runtime, بنية M3Continuation* i_cont);
#پایان_اگر

d_m3_انتها_Extern_C

#پایان_اگر // m3_compile_h
