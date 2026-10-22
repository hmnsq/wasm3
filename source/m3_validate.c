//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#અન્તર્ભાવ "m3_validate.h"
#અન્તર્ભાવ "m3_exception.h"
#અન્તર્ભાવ "m3_info.h"

#જો d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

પ્રકારવ્યાખ્યા સંરચના {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_ગણતરી;
    u16         result_ગણતરી;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

સંરચના ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ઢગલો];
    u16         opd_ટોચ;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_ટોચ;

    m3type_t    localTypes [d_m3_Val_ઢગલો];
    u16         અંક_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
સ્થિર bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    પાછા v->module and i_memoryIdx < v->module->અંક_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
સ્થિર m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    પાછા Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
સ્થિર bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    પાછા v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
સ્થિર m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    પાછા Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
સ્થિર u32 v_max_align (m3opcode_t opcode)
{
    પસંદગી (opcode) {
    સ્થિતિ 0x2c: સ્થિતિ 0x2d:   // i32.load8_s, i32.load8_u
    સ્થિતિ 0x30: સ્થિતિ 0x31:   // i64.load8_s, i64.load8_u
    સ્થિતિ 0x3a:              // i32.store8
    સ્થિતિ 0x3c:              // i64.store8
        પાછા 0;
    સ્થિતિ 0x2e: સ્થિતિ 0x2f:   // i32.load16_s, i32.load16_u
    સ્થિતિ 0x32: સ્થિતિ 0x33:   // i64.load16_s, i64.load16_u
    સ્થિતિ 0x3b:              // i32.store16
    સ્થિતિ 0x3d:              // i64.store16
        પાછા 1;
    સ્થિતિ 0x29:              // i64.load
    સ્થિતિ 0x2b:              // f64.load
    સ્થિતિ 0x37:              // i64.store
    સ્થિતિ 0x39:              // f64.store
        પાછા 3;
    મૂળભૂત:                // 32-bit accesses, and a safe fallback
        પાછા 2;
    }
}

#જો d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
સ્થિર bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_અંક_Operands, m3type_t * o_કિંમત_Type, m3type_t * o_resultType)
{
    સ્થિર અચલ u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    સ્થિર અચલ m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    જો (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_અંક_Operands = (sub == 0x00) ? 2 : 3;
        *o_કિંમત_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        પાછા true;
    }

    જો (sub < 0x10 or sub > c_waOp_lastAtomic) પાછા false;

    જો (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_અંક_Operands = 1;
    } નહીંતર જો (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_અંક_Operands = 2;
    } નહીંતર જો (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_અંક_Operands = 2;
    } નહીંતર {                        // cmpxchg
        variant = sub - 0x48;       *o_અંક_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_કિંમત_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    પાછા true;
}

#endif

// ---------- Operand stack ----------

સ્થિર M3Result v_ધકેલો (ValCtx * v, m3type_t type)
{
    જો (M3_UNLIKELY(v->opd_ટોચ >= d_m3_Val_ઢગલો))
        પાછા m3_Err_function_ઢગલો_Overflow;
    v->opd[v->opd_ટોચ++] = type;
    પાછા m3Err_none;
}

સ્થિર M3Result v_કાઢો (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ટોચ - 1];
    જો (M3_UNLIKELY(v->opd_ટોચ == f->height)) {
        જો (f->is_unreachable) { *o_type = c_valBottom; પાછા m3Err_none; }
        પાછા m3_Err_function_ઢગલો_Underrun;
    }
    *o_type = v->opd[--v->opd_ટોચ];
    પાછા m3Err_none;
}

સ્થિર M3Result v_કાઢો_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_કાઢો(v, &actual);
    જો (M3_UNLIKELY(r)) પાછા r;
    જો (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        પાછા m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    પાછા m3Err_none;
}

// ---------- Control stack ----------

સ્થિર M3Result v_ધકેલો_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    જો (M3_UNLIKELY(v->ctrl_ટોચ >= d_m3ValCtrlDepth))
        પાછા m3_Err_function_ઢગલો_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ટોચ++];
    f->opcode       = op;
    f->type         = type;
    f->param_ગણતરી  = type ? type->અંક_Args : 0;
    f->result_ગણતરી = type ? type->અંક_Rets : 0;
    f->height       = v->opd_ટોચ;
    f->is_unreachable = false;
    પાછા m3Err_none;
}

સ્થિર M3Result v_કાઢો_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    જો (M3_UNLIKELY(v->ctrl_ટોચ == 0))
        પાછા m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ટોચ - 1];
    // pop result types
    જો (f->type) {
        માટે (u16 i = f->result_ગણતરી; i > 0; i--) {
            m3type_t a;
            M3Result r = v_કાઢો_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            જો (M3_UNLIKELY(r)) પાછા r;
        }
    }
    જો (M3_UNLIKELY(v->opd_ટોચ != f->height))
        પાછા m3_Err_type_ગણતરી_Mismatch;
    જો (o_frame) *o_frame = *f;
    v->ctrl_ટોચ--;
    પાછા m3Err_none;
}

સ્થિર શૂન્ય v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ટોચ - 1];
    v->opd_ટોચ = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
સ્થિર u16 v_label_n (ValCtrlFrame * f)
{
    પાછા (f->opcode == 0x03) ? f->param_ગણતરી : f->result_ગણતરી;
}

સ્થિર m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    જો (!f->type) પાછા c_m3Type_none;
    જો (f->opcode == 0x03)
        પાછા BaseTypeOf(f->type->types[f->type->અંક_Rets + i]); // params
    પાછા BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
સ્થિર m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ટોચ - 1];

    જો (v->opd_ટોચ <= i_depth || (u16)(v->opd_ટોચ - i_depth - 1) < f->height)
        પાછા c_valBottom;

    પાછા v->opd[v->opd_ટોચ - i_depth - 1];
}

// Pop label types for branch target
સ્થિર M3Result v_કાઢો_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    માટે (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_કાઢો_expect(v, v_label_t(tgt, i - 1), &a);
        જો (M3_UNLIKELY(r)) પાછા r;
    }
    પાછા m3Err_none;
}

// Push label types back
સ્થિર M3Result v_ધકેલો_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    માટે (u16 i = 0; i < n; i++) {
        M3Result r = v_ધકેલો(v, v_label_t(tgt, i));
        જો (M3_UNLIKELY(r)) પાછા r;
    }
    પાછા m3Err_none;
}

// ---------- Block type resolution ----------

સ્થિર M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    જો (v->wasm >= v->wasmEnd)
        પાછા m3Err_wasmUnderrun;

#જો d_m3HasTypedRefs
    જો (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_કિંમત_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        જો (rr) પાછા rr;
        IM3FuncType ftype = ખાલી;
        rr = AllocFuncType(&ftype, 1);
        જો (rr) પાછા rr;
        ftype->અંક_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        જો (rr) પાછા rr;
        *o_type = ftype;
        પાછા m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    જો (r) પાછા r;

    જો (type < 0) {
        જો (type < -64) પાછા m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        જો (r) પાછા r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } નહીંતર {
        જો ((u32)type >= v->module->અંક_Func_Types)
            પાછા m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    પાછા m3Err_none;
}

// ---------- Convenience ----------

સ્થિર M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_કાઢો_expect(v, in, &a);
    જો (M3_UNLIKELY(r)) પાછા r;
    પાછા v_ધકેલો(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
સ્થિર M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ખાલી;

    u16 અંક_Callee_Rets = i_calleeType ? i_calleeType->અંક_Rets : 0;
    u16 અંક_Func_Rets   = ft ? ft->અંક_Rets : 0;

    જો (અંક_Callee_Rets != અંક_Func_Rets)
        પાછા m3Err_typeMismatch;

    માટે (u16 i = 0; i < અંક_Callee_Rets; i++) {
        જો (i_calleeType->types[i] != ft->types[i])
            પાછા m3Err_typeMismatch;
    }

    પાછા m3Err_none;
}

સ્થિર M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_કાઢો_expect(v, t, &a); જો (M3_UNLIKELY(r)) પાછા r;
    r = v_કાઢો_expect(v, t, &a); જો (M3_UNLIKELY(r)) પાછા r;
    પાછા v_ધકેલો(v, t);
}

સ્થિર M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_કાઢો_expect(v, t, &a); જો (M3_UNLIKELY(r)) પાછા r;
    r = v_કાઢો_expect(v, t, &a); જો (M3_UNLIKELY(r)) પાછા r;
    પાછા v_ધકેલો(v, c_m3Type_i32);
}

સ્થિર M3Result v_testop (ValCtx * v, m3type_t t)
{
    પાછા v_unop(v, t, c_m3Type_i32);
}

સ્થિર M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    પાછા v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#જો d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
સ્થિર M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    જો (r) પાછા r;
    જો (kind > 0x03) પાછા m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = ખાલી;
    જો (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        જો (r) પાછા r;
        જો (!v->module || tagIndex >= v->module->અંક_Tags) પાછા m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    જો (r) પાછા r;
    જો (depth >= v->ctrl_ટોચ) પાછા m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ટોચ - 1 - depth];

    u16 અંક_Payload = tagType ? tagType->અંક_Args : 0;
    u16 અંક_Label   = v_label_n(tgt);

    જો (અંક_Label != અંક_Payload + (hasRef ? 1u : 0u))
        પાછા m3_Err_type_ગણતરી_Mismatch;

    માટે (u16 i = 0; i < અંક_Payload; i++) {
        જો (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->અંક_Rets + i]))
            પાછા m3Err_typeMismatch;
    }

    જો (hasRef and v_label_t(tgt, અંક_Payload) != c_m3Type_exnref)
        પાછા m3Err_typeMismatch;

    પાછા m3Err_none;
}

#endif // d_m3HasExceptionHandling

#જો d_m3_Has_ઢગલો_Switching

સ્થિર M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    જો (kind > 0x01) પાછા m3Err_wasmMalformed;
    જો (!v->module || tagIdx >= v->module->અંક_Tags) પાછા m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    જો (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); જો (r) પાછા r;
        જો (depth >= v->ctrl_ટોચ) પાછા m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ટોચ - 1 - depth];
        u16 અંક_Payload = tagType ? tagType->અંક_Args : 0;
        u16 અંક_Label   = v_label_n(tgt);

        જો (અંક_Label != અંક_Payload + 1u) પાછા m3_Err_type_ગણતરી_Mismatch;

        માટે (u16 j = 0; j < અંક_Payload; j++) {
            જો (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->અંક_Rets + j]))
                પાછા m3Err_typeMismatch;
        }
        જો (v_label_t(tgt, અંક_Payload) != c_m3Type_contref)
            પાછા m3Err_typeMismatch;

#જો d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->અંક_Rets + અંક_Payload]
            : tgt->type->types[અંક_Payload];

        જો (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            પાછા m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = ખાલી;
        માટે (u32 idx = 0; idx < v->module->અંક_Func_Types; ++idx) {
            જો (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                તોડો;
            }
        }
        જો (!labelCt) પાછા m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_અંક_Params = tagType ? tagType->અંક_Rets : 0;
        જો (labelFt->અંક_Args != expected_અંક_Params) પાછા m3Err_typeMismatch;
        માટે (u16 p = 0; p < expected_અંક_Params; ++p) {
            જો (labelFt->types[labelFt->અંક_Rets + p] != tagType->types[p])
                પાછા m3Err_typeMismatch;
        }

        u16 expected_અંક_Rets = ft ? ft->અંક_Rets : 0;
        જો (labelFt->અંક_Rets != expected_અંક_Rets) પાછા m3Err_typeMismatch;
        માટે (u16 r_idx = 0; r_idx < expected_અંક_Rets; ++r_idx) {
            જો (labelFt->types[r_idx] != ft->types[r_idx])
                પાછા m3Err_typeMismatch;
        }
#endif
    } નહીંતર {
        જો (tagType && tagType->અંક_Args != 0) પાછા m3Err_typeMismatch;
    }
    પાછા m3Err_none;
}

#endif // d_m3HasStackSwitching

સ્થિર M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    જ્યારે (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        જો (M3_UNLIKELY(r)) પાછા r;

        પસંદગી (opcode)
        {
        // ---- Control ----
        સ્થિતિ 0x00: // unreachable
            v_unreachable(v);
            તોડો;

        સ્થિતિ 0x01: // nop
            તોડો;

        સ્થિતિ 0x02: // block
        સ્થિતિ 0x03: // loop
        સ્થિતિ 0x04: // if
        {
            IM3FuncType bt = ખાલી;
            r = v_read_blocktype(v, &bt);
            જો (r) પાછા r;
            જો (opcode == 0x04) {
                r = v_કાઢો_expect(v, c_m3Type_i32, &a);
                જો (r) પાછા r;
            }
            // Pop block params from caller stack
            જો (bt) {
                માટે (u16 i = bt->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(bt->types[bt->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
            }
            r = v_ધકેલો_ctrl(v, opcode, bt);
            જો (r) પાછા r;
            // Push params inside block
            જો (bt) {
                માટે (u16 i = 0; i < bt->અંક_Args; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(bt->types[bt->અંક_Rets + i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

#જો d_m3HasExceptionHandling
        સ્થિતિ 0x1f: // try_table
        {
            IM3FuncType bt = ખાલી;
            r = v_read_blocktype(v, &bt);
            જો (r) પાછા r;
            જો (bt) {
                માટે (u16 i = bt->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(bt->types[bt->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 અંક_Catch;
            r = ReadLEB_u32(&અંક_Catch, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            માટે (u32 i = 0; i < અંક_Catch; i++) {
                r = v_catch_clause(v);
                જો (r) પાછા r;
            }

            r = v_ધકેલો_ctrl(v, opcode, bt);
            જો (r) પાછા r;

            જો (bt) {
                માટે (u16 i = 0; i < bt->અંક_Args; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(bt->types[bt->અંક_Rets + i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (!v->module || tagIndex >= v->module->અંક_Tags) પાછા m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            જો (tt) {
                માટે (u16 i = tt->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(tt->types[tt->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
            }
            v_unreachable(v);
            તોડો;
        }

        સ્થિતિ 0x0a: // throw_ref
            r = v_કાઢો_expect(v, c_m3Type_exnref, &a);
            જો (r) પાછા r;
            v_unreachable(v);
            તોડો;
#endif // d_m3HasExceptionHandling

        સ્થિતિ 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_કાઢો_ctrl(v, &frame);
            જો (r) પાછા r;
            જો (frame.opcode != 0x04)
                પાછા m3Err_wasmMalformed;
            r = v_ધકેલો_ctrl(v, 0x05, frame.type);
            જો (r) પાછા r;
            જો (frame.type) {
                માટે (u16 i = 0; i < frame.type->અંક_Args; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(frame.type->types[frame.type->અંક_Rets + i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_કાઢો_ctrl(v, &frame);
            જો (r) પાછા r;
            // Push results
            જો (frame.type) {
                માટે (u16 i = 0; i < frame.result_ગણતરી; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(frame.type->types[i]));
                    જો (r) પાછા r;
                }
            }
            // If this was the outermost frame, we're done
            જો (v->ctrl_ટોચ == 0)
                પાછા m3Err_none;
            તોડો;
        }

        સ્થિતિ 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (depth >= v->ctrl_ટોચ) પાછા m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ટોચ - 1 - depth];
            r = v_કાઢો_labels(v, tgt);
            જો (r) પાછા r;
            v_unreachable(v);
            તોડો;
        }

        સ્થિતિ 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (depth >= v->ctrl_ટોચ) પાછા m3Err_unknownLabel;
            r = v_કાઢો_expect(v, c_m3Type_i32, &a);
            જો (r) પાછા r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ટોચ - 1 - depth];
            r = v_કાઢો_labels(v, tgt);
            જો (r) પાછા r;
            r = v_ધકેલો_labels(v, tgt);
            જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0x0e: // br_table
        {
            u32 ગણતરી;
            r = ReadLEB_u32(&ગણતરી, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            માટે (u32 i = 0; i <= ગણતરી; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                જો (r) પાછા r;
                જો (d >= v->ctrl_ટોચ) પાછા m3Err_unknownLabel;
                જો (i == ગણતરી) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_ટોચ - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_કાઢો_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            જો (r) પાછા r;

            માટે (u32 i = 0; i <= ગણતરી; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                જો (r) પાછા r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_ટોચ - 1 - d];
                u16 n = v_label_n(t);
                જો (n != arity) પાછા m3_Err_type_ગણતરી_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                માટે (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    જો (want != c_valBottom && have != c_valBottom && want != have)
                        પાછા m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_ટોચ - 1 - defDepth];
            r = v_કાઢો_labels(v, dt);
            જો (r) પાછા r;
            v_unreachable(v);
            તોડો;
        }

        સ્થિતિ 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            જો (ft) {
                માટે (u16 i = ft->અંક_Rets; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    જો (r) પાછા r;
                }
            }
            v_unreachable(v);
            તોડો;
        }

        // ---- Call ----
        સ્થિતિ 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (idx >= v->module->અંક_Functions) પાછા m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            જો (ft) {
                માટે (u16 i = ft->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(ft->types[ft->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
                માટે (u16 i = 0; i < ft->અંક_Rets; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(ft->types[i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            // the table must exist and hold funcrefs
            જો (tableIdx >= v->module->અંક_Tables) પાછા m3Err_unknownTable;
            જો (v->module->tables[tableIdx]->type != c_m3Type_funcref) પાછા m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_કાઢો_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            જો (r) પાછા r;
            જો (ft) {
                માટે (u16 i = ft->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(ft->types[ft->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
                માટે (u16 i = 0; i < ft->અંક_Rets; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(ft->types[i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (idx >= v->module->અંક_Functions) પાછા m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            જો (r) પાછા r;
            જો (ft) {
                માટે (u16 i = ft->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(ft->types[ft->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
            }
            v_unreachable(v);
            તોડો;
        }

        સ્થિતિ 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            // the table must exist and hold funcrefs
            જો (tableIdx >= v->module->અંક_Tables) પાછા m3Err_unknownTable;
            જો (v->module->tables[tableIdx]->type != c_m3Type_funcref) પાછા m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            જો (r) પાછા r;
            r = v_કાઢો_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            જો (r) પાછા r;
            જો (ft) {
                માટે (u16 i = ft->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(ft->types[ft->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
            }
            v_unreachable(v);
            તોડો;
        }

#જો d_m3HasTypedRefs
        સ્થિતિ 0x14: // call_ref
        સ્થિતિ 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            જો (isTail) {
                r = v_check_tail_results(v, ft);
                જો (r) પાછા r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_કાઢો_expect(v, c_m3Type_funcref, &a);
            જો (r) પાછા r;
            જો (ft) {
                માટે (u16 i = ft->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(ft->types[ft->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
                જો (not isTail) {
                    માટે (u16 i = 0; i < ft->અંક_Rets; i++) {
                        r = v_ધકેલો(v, BaseTypeOf(ft->types[i]));
                        જો (r) પાછા r;
                    }
                }
            }
            જો (isTail) v_unreachable(v);
            તોડો;
        }

        સ્થિતિ 0xd4: // ref.as_non_null
        {
            r = v_કાઢો_expect(v, c_m3Type_funcref, &a);
            જો (r) પાછા r;
            r = v_ધકેલો(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            જો (r) પાછા r;
            તોડો;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        સ્થિતિ 0x1a: // drop
            r = v_કાઢો(v, &a);
            જો (r) પાછા r;
            તોડો;

        સ્થિતિ 0x1b: // select
        {
            r = v_કાઢો_expect(v, c_m3Type_i32, &a);
            જો (r) પાછા r;
            m3type_t t2 = c_valBottom;
            r = v_કાઢો(v, &t2);
            જો (r) પાછા r;
            m3type_t t1;
            r = v_કાઢો_expect(v, t2, &t1);
            જો (r) પાછા r;
            // untyped select is numeric only; references need the 0x1c form
            જો (t2 != c_valBottom && IsRefType(t2)) પાછા m3Err_typeMismatch;
            r = v_ધકેલો(v, (t2 == c_valBottom) ? t1 : t2);
            જો (r) પાછા r;
            તોડો;
        }

#જો d_m3HasRefTypes
        સ્થિતિ 0x1c: // select with an explicit result type
        {
            u32 અંક_Types;
            r = ReadLEB_u32(&અંક_Types, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (અંક_Types != 1) પાછા m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_કિંમત_Type(v->module, &selType, &v->wasm, v->wasmEnd);  જો (r) પાછા r;
            u8 t = BaseTypeOf(selType);

            r = v_કાઢો_expect(v, c_m3Type_i32, &a); જો (r) પાછા r;
            r = v_કાઢો_expect(v, t, &a);            જો (r) પાછા r;
            r = v_કાઢો_expect(v, t, &a);            જો (r) પાછા r;
            r = v_ધકેલો(v, t);                      જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0x25: // table.get
        સ્થિતિ 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (tableIdx >= v->module->અંક_Tables) પાછા m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            જો (opcode == 0x26) {
                r = v_કાઢો_expect(v, t, &a);   જો (r) પાછા r;
                r = v_કાઢો_expect(v, at, &a);  જો (r) પાછા r;
            } નહીંતર {
                r = v_કાઢો_expect(v, at, &a);  જો (r) પાછા r;
                r = v_ધકેલો(v, t);             જો (r) પાછા r;
            }
            તોડો;
        }

        સ્થિતિ 0xd0: // ref.null
        {
#જો d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  જો (r) પાછા r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# જો d_m3_Has_ઢગલો_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#નહીંતર
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   જો (r) પાછા r;
            r = NormalizeType(&t, waType);                   જો (r) પાછા r;
            જો (!IsRefType(t)) પાછા m3Err_wasmMalformed;
#endif
            r = v_ધકેલો(v, t);                                જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_કાઢો(v, &t);
            જો (r) પાછા r;
            જો (t != c_valBottom && !IsRefType(t)) પાછા m3Err_typeMismatch;
            r = v_ધકેલો(v, c_m3Type_i32);
            જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (funcIdx >= v->module->અંક_Functions) પાછા m3Err_unknownFunction;
            જો (!Module_IsFunctionDeclared(v->module, funcIdx)) પાછા m3Err_undeclaredFuncRef;
            r = v_ધકેલો(v, c_m3Type_funcref);
            જો (r) પાછા r;
            તોડો;
        }
#endif

        // ---- Variable ----
        સ્થિતિ 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (idx >= v->અંક_Locals) પાછા m3Err_unknownLocal;
            r = v_ધકેલો(v, v->localTypes[idx]);
            જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (idx >= v->અંક_Locals) પાછા m3Err_unknownLocal;
            r = v_કાઢો_expect(v, v->localTypes[idx], &a);
            જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (idx >= v->અંક_Locals) પાછા m3Err_unknownLocal;
            r = v_કાઢો_expect(v, v->localTypes[idx], &a);
            જો (r) પાછા r;
            r = v_ધકેલો(v, v->localTypes[idx]);
            જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (idx >= v->module->અંક_Globals) પાછા m3Err_unknownGlobal;
            r = v_ધકેલો(v, BaseTypeOf(v->module->globals[idx].type));
            જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            જો (idx >= v->module->અંક_Globals) પાછા m3Err_unknownGlobal;
            r = v_કાઢો_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            જો (r) પાછા r;
            તોડો;
        }

        // ---- Memory load ----
        સ્થિતિ 0x28: સ્થિતિ 0x29: સ્થિતિ 0x2a: સ્થિતિ 0x2b: // i32/i64/f32/f64.load
        સ્થિતિ 0x2c: સ્થિતિ 0x2d: સ્થિતિ 0x2e: સ્થિતિ 0x2f: // i32.load8/16 s/u
        સ્થિતિ 0x30: સ્થિતિ 0x31: સ્થિતિ 0x32: સ્થિતિ 0x33: // i64.load8/16 s/u
        સ્થિતિ 0x34: સ્થિતિ 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (align > v_max_align(opcode)) પાછા m3Err_invalidAlignment;
            જો (not v_has_memory_idx(v, memidx)) પાછા m3Err_unknownMemory;
            જો (not v_offset_in_range(v, memidx, offset)) પાછા m3Err_wasmMalformed;
            r = v_કાઢો_expect(v, v_memory_addrtype(v, memidx), &a); જો (r) પાછા r;
            u8 result;
            જો      (opcode == 0x28) result = c_m3Type_i32;
            નહીંતર જો (opcode == 0x29) result = c_m3Type_i64;
            નહીંતર જો (opcode == 0x2a) result = c_m3Type_f32;
            નહીંતર જો (opcode == 0x2b) result = c_m3Type_f64;
            નહીંતર જો (opcode <= 0x2f) result = c_m3Type_i32;
            નહીંતર                     result = c_m3Type_i64;
            r = v_ધકેલો(v, result);
            જો (r) પાછા r;
            તોડો;
        }

        // ---- Memory store ----
        સ્થિતિ 0x36: સ્થિતિ 0x37: સ્થિતિ 0x38: સ્થિતિ 0x39: // i32/i64/f32/f64.store
        સ્થિતિ 0x3a: સ્થિતિ 0x3b:                         // i32.store8/16
        સ્થિતિ 0x3c: સ્થિતિ 0x3d: સ્થિતિ 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (align > v_max_align(opcode)) પાછા m3Err_invalidAlignment;
            જો (not v_has_memory_idx(v, memidx)) પાછા m3Err_unknownMemory;
            જો (not v_offset_in_range(v, memidx, offset)) પાછા m3Err_wasmMalformed;
            u8 valtype;
            જો      (opcode == 0x36) valtype = c_m3Type_i32;
            નહીંતર જો (opcode == 0x37) valtype = c_m3Type_i64;
            નહીંતર જો (opcode == 0x38) valtype = c_m3Type_f32;
            નહીંતર જો (opcode == 0x39) valtype = c_m3Type_f64;
            નહીંતર જો (opcode <= 0x3b) valtype = c_m3Type_i32;
            નહીંતર                     valtype = c_m3Type_i64;
            r = v_કાઢો_expect(v, valtype, &a); જો (r) પાછા r;
            r = v_કાઢો_expect(v, v_memory_addrtype(v, memidx), &a); જો (r) પાછા r;
            તોડો;
        }

        // ---- Memory size/grow ----
        સ્થિતિ 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (not v_has_memory_idx(v, memidx)) પાછા m3Err_unknownMemory;
            r = v_ધકેલો(v, v_memory_addrtype(v, memidx)); જો (r) પાછા r;
            તોડો;
        }
        સ્થિતિ 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (not v_has_memory_idx(v, memidx)) પાછા m3Err_unknownMemory;
            r = v_કાઢો_expect(v, v_memory_addrtype(v, memidx), &a); જો (r) પાછા r;
            r = v_ધકેલો(v, v_memory_addrtype(v, memidx)); જો (r) પાછા r;
            તોડો;
        }

        // ---- Constants ----
        સ્થિતિ 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            r = v_ધકેલો(v, c_m3Type_i32); જો (r) પાછા r;
            તોડો;
        }
        સ્થિતિ 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            r = v_ધકેલો(v, c_m3Type_i64); જો (r) પાછા r;
            તોડો;
        }
        સ્થિતિ 0x43: { // f32.const
            જો (v->wasm + 4 > v->wasmEnd) પાછા m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ધકેલો(v, c_m3Type_f32); જો (r) પાછા r;
            તોડો;
        }
        સ્થિતિ 0x44: { // f64.const
            જો (v->wasm + 8 > v->wasmEnd) પાછા m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ધકેલો(v, c_m3Type_f64); જો (r) પાછા r;
            તોડો;
        }


        // ---- i32 comparison ----
        સ્થિતિ 0x45: r = v_testop(v, c_m3Type_i32); તોડો; // i32.eqz
        સ્થિતિ 0x46: સ્થિતિ 0x47: સ્થિતિ 0x48: સ્થિતિ 0x49: સ્થિતિ 0x4a:
        સ્થિતિ 0x4b: સ્થિતિ 0x4c: સ્થિતિ 0x4d: સ્થિતિ 0x4e: સ્થિતિ 0x4f:
            r = v_relop(v, c_m3Type_i32); તોડો;

        // ---- i64 comparison ----
        સ્થિતિ 0x50: r = v_testop(v, c_m3Type_i64); તોડો; // i64.eqz
        સ્થિતિ 0x51: સ્થિતિ 0x52: સ્થિતિ 0x53: સ્થિતિ 0x54: સ્થિતિ 0x55:
        સ્થિતિ 0x56: સ્થિતિ 0x57: સ્થિતિ 0x58: સ્થિતિ 0x59: સ્થિતિ 0x5a:
            r = v_relop(v, c_m3Type_i64); તોડો;

        // ---- f32 comparison ----
        સ્થિતિ 0x5b: સ્થિતિ 0x5c: સ્થિતિ 0x5d: સ્થિતિ 0x5e: સ્થિતિ 0x5f: સ્થિતિ 0x60:
            r = v_relop(v, c_m3Type_f32); તોડો;

        // ---- f64 comparison ----
        સ્થિતિ 0x61: સ્થિતિ 0x62: સ્થિતિ 0x63: સ્થિતિ 0x64: સ્થિતિ 0x65: સ્થિતિ 0x66:
            r = v_relop(v, c_m3Type_f64); તોડો;

        // ---- i32 unary ----
        સ્થિતિ 0x67: સ્થિતિ 0x68: સ્થિતિ 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); તોડો;

        // ---- i32 binary ----
        સ્થિતિ 0x6a: સ્થિતિ 0x6b: સ્થિતિ 0x6c: સ્થિતિ 0x6d: સ્થિતિ 0x6e: સ્થિતિ 0x6f:
        સ્થિતિ 0x70: સ્થિતિ 0x71: સ્થિતિ 0x72: સ્થિતિ 0x73: સ્થિતિ 0x74: સ્થિતિ 0x75:
        સ્થિતિ 0x76: સ્થિતિ 0x77: સ્થિતિ 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); તોડો;

        // ---- i64 unary ----
        સ્થિતિ 0x79: સ્થિતિ 0x7a: સ્થિતિ 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); તોડો;

        // ---- i64 binary ----
        સ્થિતિ 0x7c: સ્થિતિ 0x7d: સ્થિતિ 0x7e: સ્થિતિ 0x7f: સ્થિતિ 0x80: સ્થિતિ 0x81:
        સ્થિતિ 0x82: સ્થિતિ 0x83: સ્થિતિ 0x84: સ્થિતિ 0x85: સ્થિતિ 0x86: સ્થિતિ 0x87:
        સ્થિતિ 0x88: સ્થિતિ 0x89: સ્થિતિ 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); તોડો;

        // ---- f32 unary ----
        સ્થિતિ 0x8b: સ્થિતિ 0x8c: સ્થિતિ 0x8d: સ્થિતિ 0x8e: સ્થિતિ 0x8f:
        સ્થિતિ 0x90: સ્થિતિ 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); તોડો;

        // ---- f32 binary ----
        સ્થિતિ 0x92: સ્થિતિ 0x93: સ્થિતિ 0x94: સ્થિતિ 0x95: સ્થિતિ 0x96:
        સ્થિતિ 0x97: સ્થિતિ 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); તોડો;

        // ---- f64 unary ----
        સ્થિતિ 0x99: સ્થિતિ 0x9a: સ્થિતિ 0x9b: સ્થિતિ 0x9c: સ્થિતિ 0x9d:
        સ્થિતિ 0x9e: સ્થિતિ 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); તોડો;

        // ---- f64 binary ----
        સ્થિતિ 0xa0: સ્થિતિ 0xa1: સ્થિતિ 0xa2: સ્થિતિ 0xa3: સ્થિતિ 0xa4:
        સ્થિતિ 0xa5: સ્થિતિ 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); તોડો;

        // ---- Conversions ----
        સ્થિતિ 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); તોડો; // i32.wrap/i64
        સ્થિતિ 0xa8: સ્થિતિ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); તોડો;
        સ્થિતિ 0xaa: સ્થિતિ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); તોડો;
        સ્થિતિ 0xac: સ્થિતિ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); તોડો;
        સ્થિતિ 0xae: સ્થિતિ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); તોડો;
        સ્થિતિ 0xb0: સ્થિતિ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); તોડો;
        સ્થિતિ 0xb2: સ્થિતિ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); તોડો;
        સ્થિતિ 0xb4: સ્થિતિ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); તોડો;
        સ્થિતિ 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); તોડો;
        સ્થિતિ 0xb7: સ્થિતિ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); તોડો;
        સ્થિતિ 0xb9: સ્થિતિ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); તોડો;
        સ્થિતિ 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); તોડો;
        સ્થિતિ 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); તોડો;
        સ્થિતિ 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); તોડો;
        સ્થિતિ 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); તોડો;
        સ્થિતિ 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); તોડો;

        // ---- Sign-extension (MVP post) ----
        સ્થિતિ 0xc0: સ્થિતિ 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); તોડો;
        સ્થિતિ 0xc2: સ્થિતિ 0xc3: સ્થિતિ 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); તોડો;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        સ્થિતિ 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;
            પસંદગી (sub) {
            સ્થિતિ 0x00: સ્થિતિ 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); તોડો;
            સ્થિતિ 0x02: સ્થિતિ 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); તોડો;
            સ્થિતિ 0x04: સ્થિતિ 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); તોડો;
            સ્થિતિ 0x06: સ્થિતિ 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); તોડો;
            સ્થિતિ 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                જો (not v_has_memory_idx(v, memidx)) પાછા m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                જો (not v->module->has_Data_ગણતરી) પાછા m3_Err_data_ગણતરી_Required;
                જો (dataidx >= v->module->અંક_Data_Segments) પાછા m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_કાઢો_expect(v, c_m3Type_i32, &a); જો (r) પાછા r; // n
                r = v_કાઢો_expect(v, c_m3Type_i32, &a); જો (r) પાછા r; // src
                r = v_કાઢો_expect(v, v_memory_addrtype(v, memidx), &a); જો (r) પાછા r; // dst
                તોડો;
            }
            સ્થિતિ 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                // needs the segment, but not a memory
                જો (not v->module->has_Data_ગણતરી) પાછા m3_Err_data_ગણતરી_Required;
                જો (dataidx >= v->module->અંક_Data_Segments) પાછા m3Err_unknownDataSegment;
                તોડો;
            }
#જો d_m3HasRefTypes
            સ્થિતિ 0x0c: // table.init
            સ્થિતિ 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); જો (r) પાછા r;

                જો (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    જો (a2 >= v->module->અંક_Tables) પાછા m3Err_unknownTable;
                    જો (a1 >= v->module->અંક_Element_Segments) પાછા m3Err_unknownElemSegment;
                    જો (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        પાછા m3Err_typeMismatch;
                } નહીંતર {                                                    // dst, src
                    જો (a1 >= v->module->અંક_Tables) પાછા m3Err_unknownTable;
                    જો (a2 >= v->module->અંક_Tables) પાછા m3Err_unknownTable;
                    જો (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        પાછા m3Err_typeMismatch;
                }

                જો (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_કાઢો_expect(v, c_m3Type_i32, &a); જો (r) પાછા r;             // n
                    r = v_કાઢો_expect(v, c_m3Type_i32, &a); જો (r) પાછા r;             // s
                    r = v_કાઢો_expect(v, v_table_addrtype(v, a2), &a); જો (r) પાછા r;  // d
                } નહીંતર {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_કાઢો_expect(v, nType, &a); જો (r) પાછા r;  // n
                    r = v_કાઢો_expect(v, sType, &a); જો (r) પાછા r;  // s
                    r = v_કાઢો_expect(v, dType, &a); જો (r) પાછા r;  // d
                }
                તોડો;
            }
            સ્થિતિ 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                જો (elemIdx >= v->module->અંક_Element_Segments) પાછા m3Err_unknownElemSegment;
                તોડો;
            }
            સ્થિતિ 0x0f: // table.grow
            સ્થિતિ 0x10: // table.size
            સ્થિતિ 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                જો (tableIdx >= v->module->અંક_Tables) પાછા m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                જો (sub == 0x10) {                                          // table.size
                    r = v_ધકેલો(v, at); જો (r) પાછા r;
                } નહીંતર જો (sub == 0x0f) {                                   // table.grow
                    r = v_કાઢો_expect(v, at, &a); જો (r) પાછા r;  // n
                    r = v_કાઢો_expect(v, t, &a);  જો (r) પાછા r;  // init
                    r = v_ધકેલો(v, at);           જો (r) પાછા r;
                } નહીંતર {                                                    // table.fill
                    r = v_કાઢો_expect(v, at, &a); જો (r) પાછા r;  // n
                    r = v_કાઢો_expect(v, t, &a);  જો (r) પાછા r;  // val
                    r = v_કાઢો_expect(v, at, &a); જો (r) પાછા r;  // i
                }
                તોડો;
            }
#endif
            સ્થિતિ 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                જો (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) પાછા m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_કાઢો_expect(v, lenType, &a); જો (r) પાછા r; // n
                r = v_કાઢો_expect(v, srcType, &a); જો (r) પાછા r; // src
                r = v_કાઢો_expect(v, dstType, &a); જો (r) પાછા r; // dst
                તોડો;
            }
            સ્થિતિ 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                જો (not v_has_memory_idx(v, memidx)) પાછા m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_કાઢો_expect(v, addrType, &a); જો (r) પાછા r;      // n
                r = v_કાઢો_expect(v, c_m3Type_i32, &a); જો (r) પાછા r;  // val
                r = v_કાઢો_expect(v, addrType, &a); જો (r) પાછા r;      // dst
                તોડો;
            }
#જો d_m3HasWideArithmetic
            સ્થિતિ 0x13: સ્થિતિ 0x14: // i64.add128, i64.sub128
            સ્થિતિ 0x15: સ્થિતિ 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 અંક_Operands = (sub <= 0x14) ? 4 : 2;
                માટે (u32 i = 0; i < અંક_Operands; ++i) {
                    r = v_કાઢો_expect(v, c_m3Type_i64, &a); જો (r) પાછા r;
                }
                r = v_ધકેલો(v, c_m3Type_i64); જો (r) પાછા r;
                r = v_ધકેલો(v, c_m3Type_i64); જો (r) પાછા r;
                તોડો;
            }
#endif
            મૂળભૂત:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                તોડો;
            }
            તોડો;
        }

#જો d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        સ્થિતિ 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            જો (r) પાછા r;

            જો (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                જો (reserved != 0) પાછા m3Err_wasmMalformed;
                તોડો;
            }

            u32 width, અંક_Operands; m3type_t કિંમત_Type, resultType;
            જો (not v_atomic_shape(sub, &width, &અંક_Operands, &કિંમત_Type, &resultType)) પાછા m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            જો ((1u << align) != width) પાછા m3Err_invalidAtomicAlignment;
            જો (not v_has_memory_idx(v, memidx)) પાછા m3Err_unknownMemory;
            જો (not v_offset_in_range(v, memidx, offset)) પાછા m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            જો (sub == 0x01 or sub == 0x02) {
                r = v_કાઢો_expect(v, c_m3Type_i64, &a); જો (r) પાછા r;
                અંક_Operands--;
            }
            માટે (u32 i = 1; i < અંક_Operands; ++i) {
                r = v_કાઢો_expect(v, કિંમત_Type, &a); જો (r) પાછા r;
            }
            r = v_કાઢો_expect(v, v_memory_addrtype(v, memidx), &a); જો (r) પાછા r;
            જો (resultType != c_m3Type_none) {
                r = v_ધકેલો(v, resultType); જો (r) પાછા r;
            }
            તોડો;
        }
#endif

#જો d_m3_Has_ઢગલો_Switching
        // ---- Stack Switching ----
        સ્થિતિ 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            જો (!ft || !ft->isContinuation) પાછા m3Err_typeMismatch;
            r = v_કાઢો_expect(v, c_m3Type_funcref, &a); જો (r) પાછા r;
            r = v_ધકેલો(v, c_m3Type_contref); જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (typeIdx1 >= v->module->અંક_Func_Types || typeIdx2 >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            જો (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) પાછા m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 અંક_Params1 = ft1->અંક_Args;
            u16 અંક_Params2 = ft2->અંક_Args;
            જો (અંક_Params1 < અંક_Params2) પાછા m3Err_typeMismatch;
            u32 અંક_Bound = અંક_Params1 - અંક_Params2;

            જો (ft1->અંક_Rets != ft2->અંક_Rets) પાછા m3Err_typeMismatch;
            માટે (u16 i = 0; i < ft1->અંક_Rets; ++i) {
                જો (ft1->types[i] != ft2->types[i]) પાછા m3Err_typeMismatch;
            }
            માટે (u16 i = 0; i < અંક_Params2; ++i) {
                જો (ft1->types[ft1->અંક_Rets + અંક_Bound + i] != ft2->types[ft2->અંક_Rets + i]) પાછા m3Err_typeMismatch;
            }

            r = v_કાઢો_expect(v, c_m3Type_contref, &a); જો (r) પાછા r;

            માટે (u32 i = અંક_Bound; i > 0; i--) {
                r = v_કાઢો_expect(v, BaseTypeOf(ft1->types[ft1->અંક_Rets + i - 1]), &a);
                જો (r) પાછા r;
            }

            r = v_ધકેલો(v, c_m3Type_contref); જો (r) પાછા r;
            તોડો;
        }

        સ્થિતિ 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (!v->module || tagIdx >= v->module->અંક_Tags) પાછા m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            જો (tt) {
                માટે (u16 i = tt->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(tt->types[tt->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
                માટે (u16 i = 0; i < tt->અંક_Rets; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(tt->types[i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            જો (!ct || !ct->isContinuation) પાછા m3Err_typeMismatch;

            u32 અંક_Clauses;
            r = ReadLEB_u32(&અંક_Clauses, &v->wasm, v->wasmEnd); જો (r) પાછા r;

            માટે (u32 i = 0; i < અંક_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                r = v_resume_clause(v, kind, tagIdx, ct); જો (r) પાછા r;
            }

            r = v_કાઢો_expect(v, c_m3Type_contref, &a); જો (r) પાછા r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            જો (ft) {
                માટે (u16 i = ft->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(ft->types[ft->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
                માટે (u16 i = 0; i < ft->અંક_Rets; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(ft->types[i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, અંક_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            r = ReadLEB_u32(&અંક_Clauses, &v->wasm, v->wasmEnd); જો (r) પાછા r;

            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            જો (!v->module || tagIdx >= v->module->અંક_Tags) પાછા m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            જો (!ct || !ct->isContinuation) પાછા m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            જો (tagType && tagType->અંક_Rets != 0) પાછા m3Err_typeMismatch;

            માટે (u32 i = 0; i < અંક_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                r = v_resume_clause(v, kind, cTagIdx, ct); જો (r) પાછા r;
            }

            r = v_કાઢો_expect(v, c_m3Type_contref, &a); જો (r) પાછા r;

            જો (tagType) {
                માટે (u16 i = tagType->અંક_Args; i > 0; i--) {
                    r = v_કાઢો_expect(v, BaseTypeOf(tagType->types[tagType->અંક_Rets + i - 1]), &a);
                    જો (r) પાછા r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            જો (ft) {
                માટે (u16 i = 0; i < ft->અંક_Rets; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(ft->types[i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0xe5: // resume_throw_ref
        {
            u32 typeIdx, અંક_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            r = ReadLEB_u32(&અંક_Clauses, &v->wasm, v->wasmEnd); જો (r) પાછા r;

            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            જો (!ct || !ct->isContinuation) પાછા m3Err_typeMismatch;

            માટે (u32 i = 0; i < અંક_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
                r = v_resume_clause(v, kind, cTagIdx, ct); જો (r) પાછા r;
            }

            r = v_કાઢો_expect(v, c_m3Type_contref, &a); જો (r) પાછા r;
            r = v_કાઢો_expect(v, c_m3Type_exnref, &a); જો (r) પાછા r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            જો (ft) {
                માટે (u16 i = 0; i < ft->અંક_Rets; i++) {
                    r = v_ધકેલો(v, BaseTypeOf(ft->types[i]));
                    જો (r) પાછા r;
                }
            }
            તોડો;
        }

        સ્થિતિ 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); જો (r) પાછા r;

            જો (typeIdx >= v->module->અંક_Func_Types) પાછા m3Err_unknownType;
            જો (!v->module || tagIdx >= v->module->અંક_Tags) પાછા m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            જો (!ct || !ct->isContinuation) પાછા m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            જો (tagType && tagType->અંક_Args != 0) પાછા m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            જો (!ft || ft->અંક_Args == 0) પાછા m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->અંક_Rets + ft->અંક_Args - 1]);
            જો (!peer || !peer->contFuncType) પાછા m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_કાઢો_expect(v, c_m3Type_contref, &a); જો (r) પાછા r;

            માટે (u16 i = (u16)(ft->અંક_Args - 1); i > 0; i--) {
                r = v_કાઢો_expect(v, BaseTypeOf(ft->types[ft->અંક_Rets + i - 1]), &a);
                જો (r) પાછા r;
            }

            માટે (u16 i = 0; i < self->અંક_Args; i++) {
                r = v_ધકેલો(v, BaseTypeOf(self->types[self->અંક_Rets + i]));
                જો (r) પાછા r;
            }
            તોડો;
        }

#endif // d_m3HasStackSwitching

        મૂળભૂત:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            તોડો;

        } // switch

        જો (r) પાછા r;

    } // while

    // If we ran out of bytes without hitting the final end
    પાછા m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    જો (!i_function->wasm) પાછા m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    જો (!v) {
        v = m3_AllocStruct (ValCtx);
        જો (!v) પાછા m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, કદ(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    જો (r) પાછા r;

    // Parse locals
    u32 અંક_Local_Blocks;
    r = ReadLEB_u32(&અંક_Local_Blocks, &v->wasm, v->wasmEnd);
    જો (r) પાછા r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 અંક_Params = funcType ? funcType->અંક_Args : 0;
    જો (અંક_Params > d_m3_Val_ઢગલો) પાછા m3_Err_function_ઢગલો_Overflow;
    માટે (u16 i = 0; i < અંક_Params; i++) {
        v->localTypes[v->અંક_Locals++] = BaseTypeOf(funcType->types[funcType->અંક_Rets + i]);
    }

    // Then: declared locals
    માટે (u32 b = 0; b < અંક_Local_Blocks; b++) {
        u32 ગણતરી;
        r = ReadLEB_u32(&ગણતરી, &v->wasm, v->wasmEnd);
        જો (r) પાછા r;
        m3type_t localType;
        r = Parse_કિંમત_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        જો (r) પાછા r;
        u8 normalized = BaseTypeOf(localType);
        જો (ગણતરી > (u32) (d_m3_Val_ઢગલો - v->અંક_Locals)) પાછા m3_Err_function_ઢગલો_Overflow;
        માટે (u32 c = 0; c < ગણતરી; c++) {
            v->localTypes[v->અંક_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ધકેલો_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    જો (r) પાછા r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    જો (r) પાછા r;

    // After validation, control stack should be empty and all declared bytes consumed
    જો (v->ctrl_ટોચ != 0)
        પાછા m3Err_wasmMalformed;

    જો (v->wasm != v->wasmEnd)
        પાછા m3Err_wasmSectionUnderrun;

    પાછા m3Err_none;
}

#નહીંતર // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (શૂન્ય)i_function;
    પાછા m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
