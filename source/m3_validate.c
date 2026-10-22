//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#ಅನ್ತರ್ಭಾವ "m3_validate.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"

#ಯದಿ d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_ಎಣಿಕೆ;
    u16         result_ಎಣಿಕೆ;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

ಸಂರಚನಾ ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ರಾಶಿ];
    u16         opd_ತುದಿ;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_ತುದಿ;

    m3type_t    localTypes [d_m3_Val_ರಾಶಿ];
    u16         ಅಂಕೆ_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ಸ್ಥಿರ bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    ಪ್ರತ್ಯಾಗಮ v->module and i_memoryIdx < v->module->ಅಂಕೆ_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ಸ್ಥಿರ m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    ಪ್ರತ್ಯಾಗಮ Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ಸ್ಥಿರ bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    ಪ್ರತ್ಯಾಗಮ v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ಸ್ಥಿರ m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    ಪ್ರತ್ಯಾಗಮ Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
ಸ್ಥಿರ u32 v_max_align (m3opcode_t opcode)
{
    ವಿಕಲ್ಪ (opcode) {
    ಪಕ್ಷ 0x2c: ಪಕ್ಷ 0x2d:   // i32.load8_s, i32.load8_u
    ಪಕ್ಷ 0x30: ಪಕ್ಷ 0x31:   // i64.load8_s, i64.load8_u
    ಪಕ್ಷ 0x3a:              // i32.store8
    ಪಕ್ಷ 0x3c:              // i64.store8
        ಪ್ರತ್ಯಾಗಮ 0;
    ಪಕ್ಷ 0x2e: ಪಕ್ಷ 0x2f:   // i32.load16_s, i32.load16_u
    ಪಕ್ಷ 0x32: ಪಕ್ಷ 0x33:   // i64.load16_s, i64.load16_u
    ಪಕ್ಷ 0x3b:              // i32.store16
    ಪಕ್ಷ 0x3d:              // i64.store16
        ಪ್ರತ್ಯಾಗಮ 1;
    ಪಕ್ಷ 0x29:              // i64.load
    ಪಕ್ಷ 0x2b:              // f64.load
    ಪಕ್ಷ 0x37:              // i64.store
    ಪಕ್ಷ 0x39:              // f64.store
        ಪ್ರತ್ಯಾಗಮ 3;
    ಸ್ವಭಾವ:                // 32-bit accesses, and a safe fallback
        ಪ್ರತ್ಯಾಗಮ 2;
    }
}

#ಯದಿ d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
ಸ್ಥಿರ bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_ಅಂಕೆ_Operands, m3type_t * o_ಮೌಲ್ಯ_Type, m3type_t * o_resultType)
{
    ಸ್ಥಿರ ಧ್ರುವ u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ಸ್ಥಿರ ಧ್ರುವ m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    ಯದಿ (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_ಅಂಕೆ_Operands = (sub == 0x00) ? 2 : 3;
        *o_ಮೌಲ್ಯ_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        ಪ್ರತ್ಯಾಗಮ true;
    }

    ಯದಿ (sub < 0x10 or sub > c_waOp_lastAtomic) ಪ್ರತ್ಯಾಗಮ false;

    ಯದಿ (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_ಅಂಕೆ_Operands = 1;
    } ಅನ್ಯಥಾ ಯದಿ (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_ಅಂಕೆ_Operands = 2;
    } ಅನ್ಯಥಾ ಯದಿ (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_ಅಂಕೆ_Operands = 2;
    } ಅನ್ಯಥಾ {                        // cmpxchg
        variant = sub - 0x48;       *o_ಅಂಕೆ_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_ಮೌಲ್ಯ_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    ಪ್ರತ್ಯಾಗಮ true;
}

#endif

// ---------- Operand stack ----------

ಸ್ಥಿರ M3Result v_ತಳ್ಳು (ValCtx * v, m3type_t type)
{
    ಯದಿ (M3_UNLIKELY(v->opd_ತುದಿ >= d_m3_Val_ರಾಶಿ))
        ಪ್ರತ್ಯಾಗಮ m3_Err_function_ರಾಶಿ_Overflow;
    v->opd[v->opd_ತುದಿ++] = type;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಸ್ಥಿರ M3Result v_ತೆಗೆ (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ತುದಿ - 1];
    ಯದಿ (M3_UNLIKELY(v->opd_ತುದಿ == f->height)) {
        ಯದಿ (f->is_unreachable) { *o_type = c_valBottom; ಪ್ರತ್ಯಾಗಮ m3Err_none; }
        ಪ್ರತ್ಯಾಗಮ m3_Err_function_ರಾಶಿ_Underrun;
    }
    *o_type = v->opd[--v->opd_ತುದಿ];
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಸ್ಥಿರ M3Result v_ತೆಗೆ_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_ತೆಗೆ(v, &actual);
    ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    ಯದಿ (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

// ---------- Control stack ----------

ಸ್ಥಿರ M3Result v_ತಳ್ಳು_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    ಯದಿ (M3_UNLIKELY(v->ctrl_ತುದಿ >= d_m3ValCtrlDepth))
        ಪ್ರತ್ಯಾಗಮ m3_Err_function_ರಾಶಿ_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ತುದಿ++];
    f->opcode       = op;
    f->type         = type;
    f->param_ಎಣಿಕೆ  = type ? type->ಅಂಕೆ_Args : 0;
    f->result_ಎಣಿಕೆ = type ? type->ಅಂಕೆ_Rets : 0;
    f->height       = v->opd_ತುದಿ;
    f->is_unreachable = false;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಸ್ಥಿರ M3Result v_ತೆಗೆ_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    ಯದಿ (M3_UNLIKELY(v->ctrl_ತುದಿ == 0))
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ತುದಿ - 1];
    // pop result types
    ಯದಿ (f->type) {
        ಪ್ರತಿ (u16 i = f->result_ಎಣಿಕೆ; i > 0; i--) {
            m3type_t a;
            M3Result r = v_ತೆಗೆ_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
        }
    }
    ಯದಿ (M3_UNLIKELY(v->opd_ತುದಿ != f->height))
        ಪ್ರತ್ಯಾಗಮ m3_Err_type_ಎಣಿಕೆ_Mismatch;
    ಯದಿ (o_frame) *o_frame = *f;
    v->ctrl_ತುದಿ--;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಸ್ಥಿರ ಶೂನ್ಯ v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ತುದಿ - 1];
    v->opd_ತುದಿ = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ಸ್ಥಿರ u16 v_label_n (ValCtrlFrame * f)
{
    ಪ್ರತ್ಯಾಗಮ (f->opcode == 0x03) ? f->param_ಎಣಿಕೆ : f->result_ಎಣಿಕೆ;
}

ಸ್ಥಿರ m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    ಯದಿ (!f->type) ಪ್ರತ್ಯಾಗಮ c_m3Type_none;
    ಯದಿ (f->opcode == 0x03)
        ಪ್ರತ್ಯಾಗಮ BaseTypeOf(f->type->types[f->type->ಅಂಕೆ_Rets + i]); // params
    ಪ್ರತ್ಯಾಗಮ BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ಸ್ಥಿರ m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ತುದಿ - 1];

    ಯದಿ (v->opd_ತುದಿ <= i_depth || (u16)(v->opd_ತುದಿ - i_depth - 1) < f->height)
        ಪ್ರತ್ಯಾಗಮ c_valBottom;

    ಪ್ರತ್ಯಾಗಮ v->opd[v->opd_ತುದಿ - i_depth - 1];
}

// Pop label types for branch target
ಸ್ಥಿರ M3Result v_ತೆಗೆ_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    ಪ್ರತಿ (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_ತೆಗೆ_expect(v, v_label_t(tgt, i - 1), &a);
        ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

// Push label types back
ಸ್ಥಿರ M3Result v_ತಳ್ಳು_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    ಪ್ರತಿ (u16 i = 0; i < n; i++) {
        M3Result r = v_ತಳ್ಳು(v, v_label_t(tgt, i));
        ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

// ---------- Block type resolution ----------

ಸ್ಥಿರ M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    ಯದಿ (v->wasm >= v->wasmEnd)
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;

#ಯದಿ d_m3HasTypedRefs
    ಯದಿ (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_ಮೌಲ್ಯ_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        ಯದಿ (rr) ಪ್ರತ್ಯಾಗಮ rr;
        IM3FuncType ftype = ರಿಕ್ತ;
        rr = AllocFuncType(&ftype, 1);
        ಯದಿ (rr) ಪ್ರತ್ಯಾಗಮ rr;
        ftype->ಅಂಕೆ_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        ಯದಿ (rr) ಪ್ರತ್ಯಾಗಮ rr;
        *o_type = ftype;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

    ಯದಿ (type < 0) {
        ಯದಿ (type < -64) ಪ್ರತ್ಯಾಗಮ m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } ಅನ್ಯಥಾ {
        ಯದಿ ((u32)type >= v->module->ಅಂಕೆ_Func_Types)
            ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

// ---------- Convenience ----------

ಸ್ಥಿರ M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_ತೆಗೆ_expect(v, in, &a);
    ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    ಪ್ರತ್ಯಾಗಮ v_ತಳ್ಳು(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ಸ್ಥಿರ M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ರಿಕ್ತ;

    u16 ಅಂಕೆ_Callee_Rets = i_calleeType ? i_calleeType->ಅಂಕೆ_Rets : 0;
    u16 ಅಂಕೆ_Func_Rets   = ft ? ft->ಅಂಕೆ_Rets : 0;

    ಯದಿ (ಅಂಕೆ_Callee_Rets != ಅಂಕೆ_Func_Rets)
        ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

    ಪ್ರತಿ (u16 i = 0; i < ಅಂಕೆ_Callee_Rets; i++) {
        ಯದಿ (i_calleeType->types[i] != ft->types[i])
            ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಸ್ಥಿರ M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    ಪ್ರತ್ಯಾಗಮ v_ತಳ್ಳು(v, t);
}

ಸ್ಥಿರ M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;
    ಪ್ರತ್ಯಾಗಮ v_ತಳ್ಳು(v, c_m3Type_i32);
}

ಸ್ಥಿರ M3Result v_testop (ValCtx * v, m3type_t t)
{
    ಪ್ರತ್ಯಾಗಮ v_unop(v, t, c_m3Type_i32);
}

ಸ್ಥಿರ M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    ಪ್ರತ್ಯಾಗಮ v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#ಯದಿ d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ಸ್ಥಿರ M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
    ಯದಿ (kind > 0x03) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = ರಿಕ್ತ;
    ಯದಿ (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
        ಯದಿ (!v->module || tagIndex >= v->module->ಅಂಕೆ_Tags) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
    ಯದಿ (depth >= v->ctrl_ತುದಿ) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - depth];

    u16 ಅಂಕೆ_Payload = tagType ? tagType->ಅಂಕೆ_Args : 0;
    u16 ಅಂಕೆ_Label   = v_label_n(tgt);

    ಯದಿ (ಅಂಕೆ_Label != ಅಂಕೆ_Payload + (hasRef ? 1u : 0u))
        ಪ್ರತ್ಯಾಗಮ m3_Err_type_ಎಣಿಕೆ_Mismatch;

    ಪ್ರತಿ (u16 i = 0; i < ಅಂಕೆ_Payload; i++) {
        ಯದಿ (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->ಅಂಕೆ_Rets + i]))
            ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
    }

    ಯದಿ (hasRef and v_label_t(tgt, ಅಂಕೆ_Payload) != c_m3Type_exnref)
        ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

#endif // d_m3HasExceptionHandling

#ಯದಿ d_m3_Has_ರಾಶಿ_Switching

ಸ್ಥಿರ M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    ಯದಿ (kind > 0x01) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
    ಯದಿ (!v->module || tagIdx >= v->module->ಅಂಕೆ_Tags) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    ಯದಿ (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
        ಯದಿ (depth >= v->ctrl_ತುದಿ) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - depth];
        u16 ಅಂಕೆ_Payload = tagType ? tagType->ಅಂಕೆ_Args : 0;
        u16 ಅಂಕೆ_Label   = v_label_n(tgt);

        ಯದಿ (ಅಂಕೆ_Label != ಅಂಕೆ_Payload + 1u) ಪ್ರತ್ಯಾಗಮ m3_Err_type_ಎಣಿಕೆ_Mismatch;

        ಪ್ರತಿ (u16 j = 0; j < ಅಂಕೆ_Payload; j++) {
            ಯದಿ (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->ಅಂಕೆ_Rets + j]))
                ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
        }
        ಯದಿ (v_label_t(tgt, ಅಂಕೆ_Payload) != c_m3Type_contref)
            ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

#ಯದಿ d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->ಅಂಕೆ_Rets + ಅಂಕೆ_Payload]
            : tgt->type->types[ಅಂಕೆ_Payload];

        ಯದಿ (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = ರಿಕ್ತ;
        ಪ್ರತಿ (u32 idx = 0; idx < v->module->ಅಂಕೆ_Func_Types; ++idx) {
            ಯದಿ (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (!labelCt) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_ಅಂಕೆ_Params = tagType ? tagType->ಅಂಕೆ_Rets : 0;
        ಯದಿ (labelFt->ಅಂಕೆ_Args != expected_ಅಂಕೆ_Params) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
        ಪ್ರತಿ (u16 p = 0; p < expected_ಅಂಕೆ_Params; ++p) {
            ಯದಿ (labelFt->types[labelFt->ಅಂಕೆ_Rets + p] != tagType->types[p])
                ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
        }

        u16 expected_ಅಂಕೆ_Rets = ft ? ft->ಅಂಕೆ_Rets : 0;
        ಯದಿ (labelFt->ಅಂಕೆ_Rets != expected_ಅಂಕೆ_Rets) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
        ಪ್ರತಿ (u16 r_idx = 0; r_idx < expected_ಅಂಕೆ_Rets; ++r_idx) {
            ಯದಿ (labelFt->types[r_idx] != ft->types[r_idx])
                ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
        }
#endif
    } ಅನ್ಯಥಾ {
        ಯದಿ (tagType && tagType->ಅಂಕೆ_Args != 0) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

#endif // d_m3HasStackSwitching

ಸ್ಥಿರ M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    ಯಾವತ್ (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        ಯದಿ (M3_UNLIKELY(r)) ಪ್ರತ್ಯಾಗಮ r;

        ವಿಕಲ್ಪ (opcode)
        {
        // ---- Control ----
        ಪಕ್ಷ 0x00: // unreachable
            v_unreachable(v);
            ಭಙ್ಗ;

        ಪಕ್ಷ 0x01: // nop
            ಭಙ್ಗ;

        ಪಕ್ಷ 0x02: // block
        ಪಕ್ಷ 0x03: // loop
        ಪಕ್ಷ 0x04: // if
        {
            IM3FuncType bt = ರಿಕ್ತ;
            r = v_read_blocktype(v, &bt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (opcode == 0x04) {
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }
            // Pop block params from caller stack
            ಯದಿ (bt) {
                ಪ್ರತಿ (u16 i = bt->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(bt->types[bt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            r = v_ತಳ್ಳು_ctrl(v, opcode, bt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            // Push params inside block
            ಯದಿ (bt) {
                ಪ್ರತಿ (u16 i = 0; i < bt->ಅಂಕೆ_Args; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(bt->types[bt->ಅಂಕೆ_Rets + i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

#ಯದಿ d_m3HasExceptionHandling
        ಪಕ್ಷ 0x1f: // try_table
        {
            IM3FuncType bt = ರಿಕ್ತ;
            r = v_read_blocktype(v, &bt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (bt) {
                ಪ್ರತಿ (u16 i = bt->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(bt->types[bt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 ಅಂಕೆ_Catch;
            r = ReadLEB_u32(&ಅಂಕೆ_Catch, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Catch; i++) {
                r = v_catch_clause(v);
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }

            r = v_ತಳ್ಳು_ctrl(v, opcode, bt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಯದಿ (bt) {
                ಪ್ರತಿ (u16 i = 0; i < bt->ಅಂಕೆ_Args; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(bt->types[bt->ಅಂಕೆ_Rets + i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (!v->module || tagIndex >= v->module->ಅಂಕೆ_Tags) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            ಯದಿ (tt) {
                ಪ್ರತಿ (u16 i = tt->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(tt->types[tt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            v_unreachable(v);
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x0a: // throw_ref
            r = v_ತೆಗೆ_expect(v, c_m3Type_exnref, &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            v_unreachable(v);
            ಭಙ್ಗ;
#endif // d_m3HasExceptionHandling

        ಪಕ್ಷ 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_ತೆಗೆ_ctrl(v, &frame);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (frame.opcode != 0x04)
                ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
            r = v_ತಳ್ಳು_ctrl(v, 0x05, frame.type);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (frame.type) {
                ಪ್ರತಿ (u16 i = 0; i < frame.type->ಅಂಕೆ_Args; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(frame.type->types[frame.type->ಅಂಕೆ_Rets + i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_ತೆಗೆ_ctrl(v, &frame);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            // Push results
            ಯದಿ (frame.type) {
                ಪ್ರತಿ (u16 i = 0; i < frame.result_ಎಣಿಕೆ; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(frame.type->types[i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            // If this was the outermost frame, we're done
            ಯದಿ (v->ctrl_ತುದಿ == 0)
                ಪ್ರತ್ಯಾಗಮ m3Err_none;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (depth >= v->ctrl_ತುದಿ) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - depth];
            r = v_ತೆಗೆ_labels(v, tgt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            v_unreachable(v);
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (depth >= v->ctrl_ತುದಿ) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLabel;
            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - depth];
            r = v_ತೆಗೆ_labels(v, tgt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು_labels(v, tgt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x0e: // br_table
        {
            u32 ಎಣಿಕೆ;
            r = ReadLEB_u32(&ಎಣಿಕೆ, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            ಪ್ರತಿ (u32 i = 0; i <= ಎಣಿಕೆ; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಯದಿ (d >= v->ctrl_ತುದಿ) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLabel;
                ಯದಿ (i == ಎಣಿಕೆ) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಪ್ರತಿ (u32 i = 0; i <= ಎಣಿಕೆ; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_ತುದಿ - 1 - d];
                u16 n = v_label_n(t);
                ಯದಿ (n != arity) ಪ್ರತ್ಯಾಗಮ m3_Err_type_ಎಣಿಕೆ_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                ಪ್ರತಿ (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    ಯದಿ (want != c_valBottom && have != c_valBottom && want != have)
                        ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_ತುದಿ - 1 - defDepth];
            r = v_ತೆಗೆ_labels(v, dt);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            v_unreachable(v);
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = ft->ಅಂಕೆ_Rets; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            v_unreachable(v);
            ಭಙ್ಗ;
        }

        // ---- Call ----
        ಪಕ್ಷ 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (idx >= v->module->ಅಂಕೆ_Functions) ಪ್ರತ್ಯಾಗಮ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = ft->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
                ಪ್ರತಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(ft->types[i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ಯದಿ (tableIdx >= v->module->ಅಂಕೆ_Tables) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTable;
            ಯದಿ (v->module->tables[tableIdx]->type != c_m3Type_funcref) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_ತೆಗೆ_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = ft->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
                ಪ್ರತಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(ft->types[i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (idx >= v->module->ಅಂಕೆ_Functions) ಪ್ರತ್ಯಾಗಮ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = ft->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            v_unreachable(v);
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ಯದಿ (tableIdx >= v->module->ಅಂಕೆ_Tables) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTable;
            ಯದಿ (v->module->tables[tableIdx]->type != c_m3Type_funcref) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತೆಗೆ_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = ft->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            v_unreachable(v);
            ಭಙ್ಗ;
        }

#ಯದಿ d_m3HasTypedRefs
        ಪಕ್ಷ 0x14: // call_ref
        ಪಕ್ಷ 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            ಯದಿ (isTail) {
                r = v_check_tail_results(v, ft);
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_ತೆಗೆ_expect(v, c_m3Type_funcref, &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = ft->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
                ಯದಿ (not isTail) {
                    ಪ್ರತಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                        r = v_ತಳ್ಳು(v, BaseTypeOf(ft->types[i]));
                        ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                    }
                }
            }
            ಯದಿ (isTail) v_unreachable(v);
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xd4: // ref.as_non_null
        {
            r = v_ತೆಗೆ_expect(v, c_m3Type_funcref, &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        ಪಕ್ಷ 0x1a: // drop
            r = v_ತೆಗೆ(v, &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;

        ಪಕ್ಷ 0x1b: // select
        {
            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            m3type_t t2 = c_valBottom;
            r = v_ತೆಗೆ(v, &t2);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            m3type_t t1;
            r = v_ತೆಗೆ_expect(v, t2, &t1);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            // untyped select is numeric only; references need the 0x1c form
            ಯದಿ (t2 != c_valBottom && IsRefType(t2)) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            r = v_ತಳ್ಳು(v, (t2 == c_valBottom) ? t1 : t2);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

#ಯದಿ d_m3HasRefTypes
        ಪಕ್ಷ 0x1c: // select with an explicit result type
        {
            u32 ಅಂಕೆ_Types;
            r = ReadLEB_u32(&ಅಂಕೆ_Types, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (ಅಂಕೆ_Types != 1) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_ಮೌಲ್ಯ_Type(v->module, &selType, &v->wasm, v->wasmEnd);  ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            u8 t = BaseTypeOf(selType);

            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತೆಗೆ_expect(v, t, &a);            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತೆಗೆ_expect(v, t, &a);            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು(v, t);                      ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x25: // table.get
        ಪಕ್ಷ 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (tableIdx >= v->module->ಅಂಕೆ_Tables) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            ಯದಿ (opcode == 0x26) {
                r = v_ತೆಗೆ_expect(v, t, &a);   ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = v_ತೆಗೆ_expect(v, at, &a);  ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            } ಅನ್ಯಥಾ {
                r = v_ತೆಗೆ_expect(v, at, &a);  ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = v_ತಳ್ಳು(v, t);             ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xd0: // ref.null
        {
#ಯದಿ d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# ಯದಿ d_m3_Has_ರಾಶಿ_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#ಅನ್ಯಥಾ
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = NormalizeType(&t, waType);                   ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (!IsRefType(t)) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
#endif
            r = v_ತಳ್ಳು(v, t);                                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_ತೆಗೆ(v, &t);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (t != c_valBottom && !IsRefType(t)) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            r = v_ತಳ್ಳು(v, c_m3Type_i32);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (funcIdx >= v->module->ಅಂಕೆ_Functions) ಪ್ರತ್ಯಾಗಮ m3Err_unknownFunction;
            ಯದಿ (!Module_IsFunctionDeclared(v->module, funcIdx)) ಪ್ರತ್ಯಾಗಮ m3Err_undeclaredFuncRef;
            r = v_ತಳ್ಳು(v, c_m3Type_funcref);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }
#endif

        // ---- Variable ----
        ಪಕ್ಷ 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (idx >= v->ಅಂಕೆ_Locals) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLocal;
            r = v_ತಳ್ಳು(v, v->localTypes[idx]);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (idx >= v->ಅಂಕೆ_Locals) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLocal;
            r = v_ತೆಗೆ_expect(v, v->localTypes[idx], &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (idx >= v->ಅಂಕೆ_Locals) ಪ್ರತ್ಯಾಗಮ m3Err_unknownLocal;
            r = v_ತೆಗೆ_expect(v, v->localTypes[idx], &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು(v, v->localTypes[idx]);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (idx >= v->module->ಅಂಕೆ_Globals) ಪ್ರತ್ಯಾಗಮ m3Err_unknownGlobal;
            r = v_ತಳ್ಳು(v, BaseTypeOf(v->module->globals[idx].type));
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (idx >= v->module->ಅಂಕೆ_Globals) ಪ್ರತ್ಯಾಗಮ m3Err_unknownGlobal;
            r = v_ತೆಗೆ_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        // ---- Memory load ----
        ಪಕ್ಷ 0x28: ಪಕ್ಷ 0x29: ಪಕ್ಷ 0x2a: ಪಕ್ಷ 0x2b: // i32/i64/f32/f64.load
        ಪಕ್ಷ 0x2c: ಪಕ್ಷ 0x2d: ಪಕ್ಷ 0x2e: ಪಕ್ಷ 0x2f: // i32.load8/16 s/u
        ಪಕ್ಷ 0x30: ಪಕ್ಷ 0x31: ಪಕ್ಷ 0x32: ಪಕ್ಷ 0x33: // i64.load8/16 s/u
        ಪಕ್ಷ 0x34: ಪಕ್ಷ 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (align > v_max_align(opcode)) ಪ್ರತ್ಯಾಗಮ m3Err_invalidAlignment;
            ಯದಿ (not v_has_memory_idx(v, memidx)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
            ಯದಿ (not v_offset_in_range(v, memidx, offset)) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
            r = v_ತೆಗೆ_expect(v, v_memory_addrtype(v, memidx), &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            u8 result;
            ಯದಿ      (opcode == 0x28) result = c_m3Type_i32;
            ಅನ್ಯಥಾ ಯದಿ (opcode == 0x29) result = c_m3Type_i64;
            ಅನ್ಯಥಾ ಯದಿ (opcode == 0x2a) result = c_m3Type_f32;
            ಅನ್ಯಥಾ ಯದಿ (opcode == 0x2b) result = c_m3Type_f64;
            ಅನ್ಯಥಾ ಯದಿ (opcode <= 0x2f) result = c_m3Type_i32;
            ಅನ್ಯಥಾ                     result = c_m3Type_i64;
            r = v_ತಳ್ಳು(v, result);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        // ---- Memory store ----
        ಪಕ್ಷ 0x36: ಪಕ್ಷ 0x37: ಪಕ್ಷ 0x38: ಪಕ್ಷ 0x39: // i32/i64/f32/f64.store
        ಪಕ್ಷ 0x3a: ಪಕ್ಷ 0x3b:                         // i32.store8/16
        ಪಕ್ಷ 0x3c: ಪಕ್ಷ 0x3d: ಪಕ್ಷ 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (align > v_max_align(opcode)) ಪ್ರತ್ಯಾಗಮ m3Err_invalidAlignment;
            ಯದಿ (not v_has_memory_idx(v, memidx)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
            ಯದಿ (not v_offset_in_range(v, memidx, offset)) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
            u8 valtype;
            ಯದಿ      (opcode == 0x36) valtype = c_m3Type_i32;
            ಅನ್ಯಥಾ ಯದಿ (opcode == 0x37) valtype = c_m3Type_i64;
            ಅನ್ಯಥಾ ಯದಿ (opcode == 0x38) valtype = c_m3Type_f32;
            ಅನ್ಯಥಾ ಯದಿ (opcode == 0x39) valtype = c_m3Type_f64;
            ಅನ್ಯಥಾ ಯದಿ (opcode <= 0x3b) valtype = c_m3Type_i32;
            ಅನ್ಯಥಾ                     valtype = c_m3Type_i64;
            r = v_ತೆಗೆ_expect(v, valtype, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತೆಗೆ_expect(v, v_memory_addrtype(v, memidx), &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        // ---- Memory size/grow ----
        ಪಕ್ಷ 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (not v_has_memory_idx(v, memidx)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
            r = v_ತಳ್ಳು(v, v_memory_addrtype(v, memidx)); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (not v_has_memory_idx(v, memidx)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
            r = v_ತೆಗೆ_expect(v, v_memory_addrtype(v, memidx), &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು(v, v_memory_addrtype(v, memidx)); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        // ---- Constants ----
        ಪಕ್ಷ 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು(v, c_m3Type_i32); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು(v, c_m3Type_i64); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 0x43: { // f32.const
            ಯದಿ (v->wasm + 4 > v->wasmEnd) ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ತಳ್ಳು(v, c_m3Type_f32); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ 0x44: { // f64.const
            ಯದಿ (v->wasm + 8 > v->wasmEnd) ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ತಳ್ಳು(v, c_m3Type_f64); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }


        // ---- i32 comparison ----
        ಪಕ್ಷ 0x45: r = v_testop(v, c_m3Type_i32); ಭಙ್ಗ; // i32.eqz
        ಪಕ್ಷ 0x46: ಪಕ್ಷ 0x47: ಪಕ್ಷ 0x48: ಪಕ್ಷ 0x49: ಪಕ್ಷ 0x4a:
        ಪಕ್ಷ 0x4b: ಪಕ್ಷ 0x4c: ಪಕ್ಷ 0x4d: ಪಕ್ಷ 0x4e: ಪಕ್ಷ 0x4f:
            r = v_relop(v, c_m3Type_i32); ಭಙ್ಗ;

        // ---- i64 comparison ----
        ಪಕ್ಷ 0x50: r = v_testop(v, c_m3Type_i64); ಭಙ್ಗ; // i64.eqz
        ಪಕ್ಷ 0x51: ಪಕ್ಷ 0x52: ಪಕ್ಷ 0x53: ಪಕ್ಷ 0x54: ಪಕ್ಷ 0x55:
        ಪಕ್ಷ 0x56: ಪಕ್ಷ 0x57: ಪಕ್ಷ 0x58: ಪಕ್ಷ 0x59: ಪಕ್ಷ 0x5a:
            r = v_relop(v, c_m3Type_i64); ಭಙ್ಗ;

        // ---- f32 comparison ----
        ಪಕ್ಷ 0x5b: ಪಕ್ಷ 0x5c: ಪಕ್ಷ 0x5d: ಪಕ್ಷ 0x5e: ಪಕ್ಷ 0x5f: ಪಕ್ಷ 0x60:
            r = v_relop(v, c_m3Type_f32); ಭಙ್ಗ;

        // ---- f64 comparison ----
        ಪಕ್ಷ 0x61: ಪಕ್ಷ 0x62: ಪಕ್ಷ 0x63: ಪಕ್ಷ 0x64: ಪಕ್ಷ 0x65: ಪಕ್ಷ 0x66:
            r = v_relop(v, c_m3Type_f64); ಭಙ್ಗ;

        // ---- i32 unary ----
        ಪಕ್ಷ 0x67: ಪಕ್ಷ 0x68: ಪಕ್ಷ 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ಭಙ್ಗ;

        // ---- i32 binary ----
        ಪಕ್ಷ 0x6a: ಪಕ್ಷ 0x6b: ಪಕ್ಷ 0x6c: ಪಕ್ಷ 0x6d: ಪಕ್ಷ 0x6e: ಪಕ್ಷ 0x6f:
        ಪಕ್ಷ 0x70: ಪಕ್ಷ 0x71: ಪಕ್ಷ 0x72: ಪಕ್ಷ 0x73: ಪಕ್ಷ 0x74: ಪಕ್ಷ 0x75:
        ಪಕ್ಷ 0x76: ಪಕ್ಷ 0x77: ಪಕ್ಷ 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); ಭಙ್ಗ;

        // ---- i64 unary ----
        ಪಕ್ಷ 0x79: ಪಕ್ಷ 0x7a: ಪಕ್ಷ 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ಭಙ್ಗ;

        // ---- i64 binary ----
        ಪಕ್ಷ 0x7c: ಪಕ್ಷ 0x7d: ಪಕ್ಷ 0x7e: ಪಕ್ಷ 0x7f: ಪಕ್ಷ 0x80: ಪಕ್ಷ 0x81:
        ಪಕ್ಷ 0x82: ಪಕ್ಷ 0x83: ಪಕ್ಷ 0x84: ಪಕ್ಷ 0x85: ಪಕ್ಷ 0x86: ಪಕ್ಷ 0x87:
        ಪಕ್ಷ 0x88: ಪಕ್ಷ 0x89: ಪಕ್ಷ 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); ಭಙ್ಗ;

        // ---- f32 unary ----
        ಪಕ್ಷ 0x8b: ಪಕ್ಷ 0x8c: ಪಕ್ಷ 0x8d: ಪಕ್ಷ 0x8e: ಪಕ್ಷ 0x8f:
        ಪಕ್ಷ 0x90: ಪಕ್ಷ 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); ಭಙ್ಗ;

        // ---- f32 binary ----
        ಪಕ್ಷ 0x92: ಪಕ್ಷ 0x93: ಪಕ್ಷ 0x94: ಪಕ್ಷ 0x95: ಪಕ್ಷ 0x96:
        ಪಕ್ಷ 0x97: ಪಕ್ಷ 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); ಭಙ್ಗ;

        // ---- f64 unary ----
        ಪಕ್ಷ 0x99: ಪಕ್ಷ 0x9a: ಪಕ್ಷ 0x9b: ಪಕ್ಷ 0x9c: ಪಕ್ಷ 0x9d:
        ಪಕ್ಷ 0x9e: ಪಕ್ಷ 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); ಭಙ್ಗ;

        // ---- f64 binary ----
        ಪಕ್ಷ 0xa0: ಪಕ್ಷ 0xa1: ಪಕ್ಷ 0xa2: ಪಕ್ಷ 0xa3: ಪಕ್ಷ 0xa4:
        ಪಕ್ಷ 0xa5: ಪಕ್ಷ 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); ಭಙ್ಗ;

        // ---- Conversions ----
        ಪಕ್ಷ 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); ಭಙ್ಗ; // i32.wrap/i64
        ಪಕ್ಷ 0xa8: ಪಕ್ಷ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xaa: ಪಕ್ಷ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xac: ಪಕ್ಷ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); ಭಙ್ಗ;
        ಪಕ್ಷ 0xae: ಪಕ್ಷ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ಭಙ್ಗ;
        ಪಕ್ಷ 0xb0: ಪಕ್ಷ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ಭಙ್ಗ;
        ಪಕ್ಷ 0xb2: ಪಕ್ಷ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xb4: ಪಕ್ಷ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xb7: ಪಕ್ಷ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); ಭಙ್ಗ;
        ಪಕ್ಷ 0xb9: ಪಕ್ಷ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ಭಙ್ಗ;
        ಪಕ್ಷ 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); ಭಙ್ಗ;
        ಪಕ್ಷ 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ಭಙ್ಗ;
        ಪಕ್ಷ 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ಭಙ್ಗ;

        // ---- Sign-extension (MVP post) ----
        ಪಕ್ಷ 0xc0: ಪಕ್ಷ 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ಭಙ್ಗ;
        ಪಕ್ಷ 0xc2: ಪಕ್ಷ 0xc3: ಪಕ್ಷ 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ಭಙ್ಗ;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        ಪಕ್ಷ 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ವಿಕಲ್ಪ (sub) {
            ಪಕ್ಷ 0x00: ಪಕ್ಷ 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ಭಙ್ಗ;
            ಪಕ್ಷ 0x02: ಪಕ್ಷ 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ಭಙ್ಗ;
            ಪಕ್ಷ 0x04: ಪಕ್ಷ 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ಭಙ್ಗ;
            ಪಕ್ಷ 0x06: ಪಕ್ಷ 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ಭಙ್ಗ;
            ಪಕ್ಷ 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಯದಿ (not v_has_memory_idx(v, memidx)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                ಯದಿ (not v->module->has_Data_ಎಣಿಕೆ) ಪ್ರತ್ಯಾಗಮ m3_Err_data_ಎಣಿಕೆ_Required;
                ಯದಿ (dataidx >= v->module->ಅಂಕೆ_Data_Segments) ಪ್ರತ್ಯಾಗಮ m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r; // n
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r; // src
                r = v_ತೆಗೆ_expect(v, v_memory_addrtype(v, memidx), &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r; // dst
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                // needs the segment, but not a memory
                ಯದಿ (not v->module->has_Data_ಎಣಿಕೆ) ಪ್ರತ್ಯಾಗಮ m3_Err_data_ಎಣಿಕೆ_Required;
                ಯದಿ (dataidx >= v->module->ಅಂಕೆ_Data_Segments) ಪ್ರತ್ಯಾಗಮ m3Err_unknownDataSegment;
                ಭಙ್ಗ;
            }
#ಯದಿ d_m3HasRefTypes
            ಪಕ್ಷ 0x0c: // table.init
            ಪಕ್ಷ 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

                ಯದಿ (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    ಯದಿ (a2 >= v->module->ಅಂಕೆ_Tables) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTable;
                    ಯದಿ (a1 >= v->module->ಅಂಕೆ_Element_Segments) ಪ್ರತ್ಯಾಗಮ m3Err_unknownElemSegment;
                    ಯದಿ (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
                } ಅನ್ಯಥಾ {                                                    // dst, src
                    ಯದಿ (a1 >= v->module->ಅಂಕೆ_Tables) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTable;
                    ಯದಿ (a2 >= v->module->ಅಂಕೆ_Tables) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTable;
                    ಯದಿ (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
                }

                ಯದಿ (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;             // n
                    r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;             // s
                    r = v_ತೆಗೆ_expect(v, v_table_addrtype(v, a2), &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // d
                } ಅನ್ಯಥಾ {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_ತೆಗೆ_expect(v, nType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // n
                    r = v_ತೆಗೆ_expect(v, sType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // s
                    r = v_ತೆಗೆ_expect(v, dType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // d
                }
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಯದಿ (elemIdx >= v->module->ಅಂಕೆ_Element_Segments) ಪ್ರತ್ಯಾಗಮ m3Err_unknownElemSegment;
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ 0x0f: // table.grow
            ಪಕ್ಷ 0x10: // table.size
            ಪಕ್ಷ 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಯದಿ (tableIdx >= v->module->ಅಂಕೆ_Tables) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                ಯದಿ (sub == 0x10) {                                          // table.size
                    r = v_ತಳ್ಳು(v, at); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                } ಅನ್ಯಥಾ ಯದಿ (sub == 0x0f) {                                   // table.grow
                    r = v_ತೆಗೆ_expect(v, at, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // n
                    r = v_ತೆಗೆ_expect(v, t, &a);  ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // init
                    r = v_ತಳ್ಳು(v, at);           ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                } ಅನ್ಯಥಾ {                                                    // table.fill
                    r = v_ತೆಗೆ_expect(v, at, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // n
                    r = v_ತೆಗೆ_expect(v, t, &a);  ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // val
                    r = v_ತೆಗೆ_expect(v, at, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // i
                }
                ಭಙ್ಗ;
            }
#endif
            ಪಕ್ಷ 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಯದಿ (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_ತೆಗೆ_expect(v, lenType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r; // n
                r = v_ತೆಗೆ_expect(v, srcType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r; // src
                r = v_ತೆಗೆ_expect(v, dstType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r; // dst
                ಭಙ್ಗ;
            }
            ಪಕ್ಷ 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಯದಿ (not v_has_memory_idx(v, memidx)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_ತೆಗೆ_expect(v, addrType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;      // n
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;  // val
                r = v_ತೆಗೆ_expect(v, addrType, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;      // dst
                ಭಙ್ಗ;
            }
#ಯದಿ d_m3HasWideArithmetic
            ಪಕ್ಷ 0x13: ಪಕ್ಷ 0x14: // i64.add128, i64.sub128
            ಪಕ್ಷ 0x15: ಪಕ್ಷ 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 ಅಂಕೆ_Operands = (sub <= 0x14) ? 4 : 2;
                ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Operands; ++i) {
                    r = v_ತೆಗೆ_expect(v, c_m3Type_i64, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
                r = v_ತಳ್ಳು(v, c_m3Type_i64); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = v_ತಳ್ಳು(v, c_m3Type_i64); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಭಙ್ಗ;
            }
#endif
            ಸ್ವಭಾವ:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                ಭಙ್ಗ;
            }
            ಭಙ್ಗ;
        }

#ಯದಿ d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        ಪಕ್ಷ 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಯದಿ (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಯದಿ (reserved != 0) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
                ಭಙ್ಗ;
            }

            u32 width, ಅಂಕೆ_Operands; m3type_t ಮೌಲ್ಯ_Type, resultType;
            ಯದಿ (not v_atomic_shape(sub, &width, &ಅಂಕೆ_Operands, &ಮೌಲ್ಯ_Type, &resultType)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            ಯದಿ ((1u << align) != width) ಪ್ರತ್ಯಾಗಮ m3Err_invalidAtomicAlignment;
            ಯದಿ (not v_has_memory_idx(v, memidx)) ಪ್ರತ್ಯಾಗಮ m3Err_unknownMemory;
            ಯದಿ (not v_offset_in_range(v, memidx, offset)) ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            ಯದಿ (sub == 0x01 or sub == 0x02) {
                r = v_ತೆಗೆ_expect(v, c_m3Type_i64, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                ಅಂಕೆ_Operands--;
            }
            ಪ್ರತಿ (u32 i = 1; i < ಅಂಕೆ_Operands; ++i) {
                r = v_ತೆಗೆ_expect(v, ಮೌಲ್ಯ_Type, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }
            r = v_ತೆಗೆ_expect(v, v_memory_addrtype(v, memidx), &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (resultType != c_m3Type_none) {
                r = v_ತಳ್ಳು(v, resultType); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }
            ಭಙ್ಗ;
        }
#endif

#ಯದಿ d_m3_Has_ರಾಶಿ_Switching
        // ---- Stack Switching ----
        ಪಕ್ಷ 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            ಯದಿ (!ft || !ft->isContinuation) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            r = v_ತೆಗೆ_expect(v, c_m3Type_funcref, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತಳ್ಳು(v, c_m3Type_contref); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (typeIdx1 >= v->module->ಅಂಕೆ_Func_Types || typeIdx2 >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            ಯದಿ (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 ಅಂಕೆ_Params1 = ft1->ಅಂಕೆ_Args;
            u16 ಅಂಕೆ_Params2 = ft2->ಅಂಕೆ_Args;
            ಯದಿ (ಅಂಕೆ_Params1 < ಅಂಕೆ_Params2) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            u32 ಅಂಕೆ_Bound = ಅಂಕೆ_Params1 - ಅಂಕೆ_Params2;

            ಯದಿ (ft1->ಅಂಕೆ_Rets != ft2->ಅಂಕೆ_Rets) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            ಪ್ರತಿ (u16 i = 0; i < ft1->ಅಂಕೆ_Rets; ++i) {
                ಯದಿ (ft1->types[i] != ft2->types[i]) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            }
            ಪ್ರತಿ (u16 i = 0; i < ಅಂಕೆ_Params2; ++i) {
                ಯದಿ (ft1->types[ft1->ಅಂಕೆ_Rets + ಅಂಕೆ_Bound + i] != ft2->types[ft2->ಅಂಕೆ_Rets + i]) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಪ್ರತಿ (u32 i = ಅಂಕೆ_Bound; i > 0; i--) {
                r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft1->types[ft1->ಅಂಕೆ_Rets + i - 1]), &a);
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }

            r = v_ತಳ್ಳು(v, c_m3Type_contref); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (!v->module || tagIdx >= v->module->ಅಂಕೆ_Tags) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            ಯದಿ (tt) {
                ಪ್ರತಿ (u16 i = tt->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(tt->types[tt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
                ಪ್ರತಿ (u16 i = 0; i < tt->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(tt->types[i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಯದಿ (!ct || !ct->isContinuation) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            u32 ಅಂಕೆ_Clauses;
            r = ReadLEB_u32(&ಅಂಕೆ_Clauses, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = v_resume_clause(v, kind, tagIdx, ct); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = ft->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
                ಪ್ರತಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(ft->types[i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, ಅಂಕೆ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = ReadLEB_u32(&ಅಂಕೆ_Clauses, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            ಯದಿ (!v->module || tagIdx >= v->module->ಅಂಕೆ_Tags) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಯದಿ (!ct || !ct->isContinuation) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            ಯದಿ (tagType && tagType->ಅಂಕೆ_Rets != 0) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = v_resume_clause(v, kind, cTagIdx, ct); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಯದಿ (tagType) {
                ಪ್ರತಿ (u16 i = tagType->ಅಂಕೆ_Args; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, BaseTypeOf(tagType->types[tagType->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(ft->types[i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xe5: // resume_throw_ref
        {
            u32 typeIdx, ಅಂಕೆ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = ReadLEB_u32(&ಅಂಕೆ_Clauses, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಯದಿ (!ct || !ct->isContinuation) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                r = v_resume_clause(v, kind, cTagIdx, ct); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = v_ತೆಗೆ_expect(v, c_m3Type_exnref, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ಯದಿ (ft) {
                ಪ್ರತಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, BaseTypeOf(ft->types[i]));
                    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
                }
            }
            ಭಙ್ಗ;
        }

        ಪಕ್ಷ 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಯದಿ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಪ್ರತ್ಯಾಗಮ m3Err_unknownType;
            ಯದಿ (!v->module || tagIdx >= v->module->ಅಂಕೆ_Tags) ಪ್ರತ್ಯಾಗಮ m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಯದಿ (!ct || !ct->isContinuation) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            ಯದಿ (tagType && tagType->ಅಂಕೆ_Args != 0) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            ಯದಿ (!ft || ft->ಅಂಕೆ_Args == 0) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->ಅಂಕೆ_Rets + ft->ಅಂಕೆ_Args - 1]);
            ಯದಿ (!peer || !peer->contFuncType) ಪ್ರತ್ಯಾಗಮ m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

            ಪ್ರತಿ (u16 i = (u16)(ft->ಅಂಕೆ_Args - 1); i > 0; i--) {
                r = v_ತೆಗೆ_expect(v, BaseTypeOf(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }

            ಪ್ರತಿ (u16 i = 0; i < self->ಅಂಕೆ_Args; i++) {
                r = v_ತಳ್ಳು(v, BaseTypeOf(self->types[self->ಅಂಕೆ_Rets + i]));
                ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
            }
            ಭಙ್ಗ;
        }

#endif // d_m3HasStackSwitching

        ಸ್ವಭಾವ:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            ಭಙ್ಗ;

        } // switch

        ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

    } // while

    // If we ran out of bytes without hitting the final end
    ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    ಯದಿ (!i_function->wasm) ಪ್ರತ್ಯಾಗಮ m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    ಯದಿ (!v) {
        v = m3_AllocStruct (ValCtx);
        ಯದಿ (!v) ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, ಪರಿಮಾಣ(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

    // Parse locals
    u32 ಅಂಕೆ_Local_Blocks;
    r = ReadLEB_u32(&ಅಂಕೆ_Local_Blocks, &v->wasm, v->wasmEnd);
    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 ಅಂಕೆ_Params = funcType ? funcType->ಅಂಕೆ_Args : 0;
    ಯದಿ (ಅಂಕೆ_Params > d_m3_Val_ರಾಶಿ) ಪ್ರತ್ಯಾಗಮ m3_Err_function_ರಾಶಿ_Overflow;
    ಪ್ರತಿ (u16 i = 0; i < ಅಂಕೆ_Params; i++) {
        v->localTypes[v->ಅಂಕೆ_Locals++] = BaseTypeOf(funcType->types[funcType->ಅಂಕೆ_Rets + i]);
    }

    // Then: declared locals
    ಪ್ರತಿ (u32 b = 0; b < ಅಂಕೆ_Local_Blocks; b++) {
        u32 ಎಣಿಕೆ;
        r = ReadLEB_u32(&ಎಣಿಕೆ, &v->wasm, v->wasmEnd);
        ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
        m3type_t localType;
        r = Parse_ಮೌಲ್ಯ_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;
        u8 normalized = BaseTypeOf(localType);
        ಯದಿ (ಎಣಿಕೆ > (u32) (d_m3_Val_ರಾಶಿ - v->ಅಂಕೆ_Locals)) ಪ್ರತ್ಯಾಗಮ m3_Err_function_ರಾಶಿ_Overflow;
        ಪ್ರತಿ (u32 c = 0; c < ಎಣಿಕೆ; c++) {
            v->localTypes[v->ಅಂಕೆ_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ತಳ್ಳು_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    ಯದಿ (r) ಪ್ರತ್ಯಾಗಮ r;

    // After validation, control stack should be empty and all declared bytes consumed
    ಯದಿ (v->ctrl_ತುದಿ != 0)
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;

    ಯದಿ (v->wasm != v->wasmEnd)
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmSectionUnderrun;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

#ಅನ್ಯಥಾ // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (ಶೂನ್ಯ)i_function;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
