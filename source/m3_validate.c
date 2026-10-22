//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#অন্তর্ভাৱ "m3_validate.h"
#অন্তর্ভাৱ "m3_exception.h"
#অন্তর্ভাৱ "m3_info.h"

#যদি d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

প্রকারনির্দেশ সংরচনা {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_গুনতি;
    u16         result_গুনতি;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

সংরচনা ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_স্তূপ];
    u16         opd_শীৰ্ষ;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_শীৰ্ষ;

    m3type_t    localTypes [d_m3_Val_স্তূপ];
    u16         অঙ্ক_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
স্থির bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    ঘূৰাই v->module and i_memoryIdx < v->module->অঙ্ক_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
স্থির m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    ঘূৰাই Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
স্থির bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    ঘূৰাই v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
স্থির m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    ঘূৰাই Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
স্থির u32 v_max_align (m3opcode_t opcode)
{
    ৱিকল্প (opcode) {
    পক্ষ 0x2c: পক্ষ 0x2d:   // i32.load8_s, i32.load8_u
    পক্ষ 0x30: পক্ষ 0x31:   // i64.load8_s, i64.load8_u
    পক্ষ 0x3a:              // i32.store8
    পক্ষ 0x3c:              // i64.store8
        ঘূৰাই 0;
    পক্ষ 0x2e: পক্ষ 0x2f:   // i32.load16_s, i32.load16_u
    পক্ষ 0x32: পক্ষ 0x33:   // i64.load16_s, i64.load16_u
    পক্ষ 0x3b:              // i32.store16
    পক্ষ 0x3d:              // i64.store16
        ঘূৰাই 1;
    পক্ষ 0x29:              // i64.load
    পক্ষ 0x2b:              // f64.load
    পক্ষ 0x37:              // i64.store
    পক্ষ 0x39:              // f64.store
        ঘূৰাই 3;
    স্ৱভাৱ:                // 32-bit accesses, and a safe fallback
        ঘূৰাই 2;
    }
}

#যদি d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
স্থির bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_অঙ্ক_Operands, m3type_t * o_মান_Type, m3type_t * o_resultType)
{
    স্থির ধ্রুৱ u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    স্থির ধ্রুৱ m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    যদি (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_অঙ্ক_Operands = (sub == 0x00) ? 2 : 3;
        *o_মান_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        ঘূৰাই true;
    }

    যদি (sub < 0x10 or sub > c_waOp_lastAtomic) ঘূৰাই false;

    যদি (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_অঙ্ক_Operands = 1;
    } অন্যথা যদি (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_অঙ্ক_Operands = 2;
    } অন্যথা যদি (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_অঙ্ক_Operands = 2;
    } অন্যথা {                        // cmpxchg
        variant = sub - 0x48;       *o_অঙ্ক_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_মান_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    ঘূৰাই true;
}

#endif

// ---------- Operand stack ----------

স্থির M3Result v_ঠেলো (ValCtx * v, m3type_t type)
{
    যদি (M3_UNLIKELY(v->opd_শীৰ্ষ >= d_m3_Val_স্তূপ))
        ঘূৰাই m3_Err_function_স্তূপ_Overflow;
    v->opd[v->opd_শীৰ্ষ++] = type;
    ঘূৰাই m3Err_none;
}

স্থির M3Result v_বেৰ_কৰো (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীৰ্ষ - 1];
    যদি (M3_UNLIKELY(v->opd_শীৰ্ষ == f->height)) {
        যদি (f->is_unreachable) { *o_type = c_valBottom; ঘূৰাই m3Err_none; }
        ঘূৰাই m3_Err_function_স্তূপ_Underrun;
    }
    *o_type = v->opd[--v->opd_শীৰ্ষ];
    ঘূৰাই m3Err_none;
}

স্থির M3Result v_বেৰ_কৰো_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_বেৰ_কৰো(v, &actual);
    যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    যদি (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        ঘূৰাই m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    ঘূৰাই m3Err_none;
}

// ---------- Control stack ----------

স্থির M3Result v_ঠেলো_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    যদি (M3_UNLIKELY(v->ctrl_শীৰ্ষ >= d_m3ValCtrlDepth))
        ঘূৰাই m3_Err_function_স্তূপ_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীৰ্ষ++];
    f->opcode       = op;
    f->type         = type;
    f->param_গুনতি  = type ? type->অঙ্ক_Args : 0;
    f->result_গুনতি = type ? type->অঙ্ক_Rets : 0;
    f->height       = v->opd_শীৰ্ষ;
    f->is_unreachable = false;
    ঘূৰাই m3Err_none;
}

স্থির M3Result v_বেৰ_কৰো_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    যদি (M3_UNLIKELY(v->ctrl_শীৰ্ষ == 0))
        ঘূৰাই m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীৰ্ষ - 1];
    // pop result types
    যদি (f->type) {
        বাবে (u16 i = f->result_গুনতি; i > 0; i--) {
            m3type_t a;
            M3Result r = v_বেৰ_কৰো_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            যদি (M3_UNLIKELY(r)) ঘূৰাই r;
        }
    }
    যদি (M3_UNLIKELY(v->opd_শীৰ্ষ != f->height))
        ঘূৰাই m3_Err_type_গুনতি_Mismatch;
    যদি (o_frame) *o_frame = *f;
    v->ctrl_শীৰ্ষ--;
    ঘূৰাই m3Err_none;
}

স্থির শূন্য v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীৰ্ষ - 1];
    v->opd_শীৰ্ষ = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
স্থির u16 v_label_n (ValCtrlFrame * f)
{
    ঘূৰাই (f->opcode == 0x03) ? f->param_গুনতি : f->result_গুনতি;
}

স্থির m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    যদি (!f->type) ঘূৰাই c_m3Type_none;
    যদি (f->opcode == 0x03)
        ঘূৰাই BaseTypeOf(f->type->types[f->type->অঙ্ক_Rets + i]); // params
    ঘূৰাই BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
স্থির m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীৰ্ষ - 1];

    যদি (v->opd_শীৰ্ষ <= i_depth || (u16)(v->opd_শীৰ্ষ - i_depth - 1) < f->height)
        ঘূৰাই c_valBottom;

    ঘূৰাই v->opd[v->opd_শীৰ্ষ - i_depth - 1];
}

// Pop label types for branch target
স্থির M3Result v_বেৰ_কৰো_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    বাবে (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_বেৰ_কৰো_expect(v, v_label_t(tgt, i - 1), &a);
        যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    }
    ঘূৰাই m3Err_none;
}

// Push label types back
স্থির M3Result v_ঠেলো_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    বাবে (u16 i = 0; i < n; i++) {
        M3Result r = v_ঠেলো(v, v_label_t(tgt, i));
        যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    }
    ঘূৰাই m3Err_none;
}

// ---------- Block type resolution ----------

স্থির M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    যদি (v->wasm >= v->wasmEnd)
        ঘূৰাই m3Err_wasmUnderrun;

#যদি d_m3HasTypedRefs
    যদি (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_মান_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        যদি (rr) ঘূৰাই rr;
        IM3FuncType ftype = রিক্ত;
        rr = AllocFuncType(&ftype, 1);
        যদি (rr) ঘূৰাই rr;
        ftype->অঙ্ক_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        যদি (rr) ঘূৰাই rr;
        *o_type = ftype;
        ঘূৰাই m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    যদি (r) ঘূৰাই r;

    যদি (type < 0) {
        যদি (type < -64) ঘূৰাই m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        যদি (r) ঘূৰাই r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } অন্যথা {
        যদি ((u32)type >= v->module->অঙ্ক_Func_Types)
            ঘূৰাই m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    ঘূৰাই m3Err_none;
}

// ---------- Convenience ----------

স্থির M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_বেৰ_কৰো_expect(v, in, &a);
    যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    ঘূৰাই v_ঠেলো(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
স্থির M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : রিক্ত;

    u16 অঙ্ক_Callee_Rets = i_calleeType ? i_calleeType->অঙ্ক_Rets : 0;
    u16 অঙ্ক_Func_Rets   = ft ? ft->অঙ্ক_Rets : 0;

    যদি (অঙ্ক_Callee_Rets != অঙ্ক_Func_Rets)
        ঘূৰাই m3Err_typeMismatch;

    বাবে (u16 i = 0; i < অঙ্ক_Callee_Rets; i++) {
        যদি (i_calleeType->types[i] != ft->types[i])
            ঘূৰাই m3Err_typeMismatch;
    }

    ঘূৰাই m3Err_none;
}

স্থির M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_বেৰ_কৰো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    r = v_বেৰ_কৰো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    ঘূৰাই v_ঠেলো(v, t);
}

স্থির M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_বেৰ_কৰো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    r = v_বেৰ_কৰো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ঘূৰাই r;
    ঘূৰাই v_ঠেলো(v, c_m3Type_i32);
}

স্থির M3Result v_testop (ValCtx * v, m3type_t t)
{
    ঘূৰাই v_unop(v, t, c_m3Type_i32);
}

স্থির M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    ঘূৰাই v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#যদি d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
স্থির M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    যদি (r) ঘূৰাই r;
    যদি (kind > 0x03) ঘূৰাই m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = রিক্ত;
    যদি (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        যদি (r) ঘূৰাই r;
        যদি (!v->module || tagIndex >= v->module->অঙ্ক_Tags) ঘূৰাই m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    যদি (r) ঘূৰাই r;
    যদি (depth >= v->ctrl_শীৰ্ষ) ঘূৰাই m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীৰ্ষ - 1 - depth];

    u16 অঙ্ক_Payload = tagType ? tagType->অঙ্ক_Args : 0;
    u16 অঙ্ক_Label   = v_label_n(tgt);

    যদি (অঙ্ক_Label != অঙ্ক_Payload + (hasRef ? 1u : 0u))
        ঘূৰাই m3_Err_type_গুনতি_Mismatch;

    বাবে (u16 i = 0; i < অঙ্ক_Payload; i++) {
        যদি (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->অঙ্ক_Rets + i]))
            ঘূৰাই m3Err_typeMismatch;
    }

    যদি (hasRef and v_label_t(tgt, অঙ্ক_Payload) != c_m3Type_exnref)
        ঘূৰাই m3Err_typeMismatch;

    ঘূৰাই m3Err_none;
}

#endif // d_m3HasExceptionHandling

#যদি d_m3_Has_স্তূপ_Switching

স্থির M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    যদি (kind > 0x01) ঘূৰাই m3Err_wasmMalformed;
    যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ঘূৰাই m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    যদি (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
        যদি (depth >= v->ctrl_শীৰ্ষ) ঘূৰাই m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীৰ্ষ - 1 - depth];
        u16 অঙ্ক_Payload = tagType ? tagType->অঙ্ক_Args : 0;
        u16 অঙ্ক_Label   = v_label_n(tgt);

        যদি (অঙ্ক_Label != অঙ্ক_Payload + 1u) ঘূৰাই m3_Err_type_গুনতি_Mismatch;

        বাবে (u16 j = 0; j < অঙ্ক_Payload; j++) {
            যদি (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->অঙ্ক_Rets + j]))
                ঘূৰাই m3Err_typeMismatch;
        }
        যদি (v_label_t(tgt, অঙ্ক_Payload) != c_m3Type_contref)
            ঘূৰাই m3Err_typeMismatch;

#যদি d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->অঙ্ক_Rets + অঙ্ক_Payload]
            : tgt->type->types[অঙ্ক_Payload];

        যদি (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            ঘূৰাই m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = রিক্ত;
        বাবে (u32 idx = 0; idx < v->module->অঙ্ক_Func_Types; ++idx) {
            যদি (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                ভঙ্গ;
            }
        }
        যদি (!labelCt) ঘূৰাই m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_অঙ্ক_Params = tagType ? tagType->অঙ্ক_Rets : 0;
        যদি (labelFt->অঙ্ক_Args != expected_অঙ্ক_Params) ঘূৰাই m3Err_typeMismatch;
        বাবে (u16 p = 0; p < expected_অঙ্ক_Params; ++p) {
            যদি (labelFt->types[labelFt->অঙ্ক_Rets + p] != tagType->types[p])
                ঘূৰাই m3Err_typeMismatch;
        }

        u16 expected_অঙ্ক_Rets = ft ? ft->অঙ্ক_Rets : 0;
        যদি (labelFt->অঙ্ক_Rets != expected_অঙ্ক_Rets) ঘূৰাই m3Err_typeMismatch;
        বাবে (u16 r_idx = 0; r_idx < expected_অঙ্ক_Rets; ++r_idx) {
            যদি (labelFt->types[r_idx] != ft->types[r_idx])
                ঘূৰাই m3Err_typeMismatch;
        }
#endif
    } অন্যথা {
        যদি (tagType && tagType->অঙ্ক_Args != 0) ঘূৰাই m3Err_typeMismatch;
    }
    ঘূৰাই m3Err_none;
}

#endif // d_m3HasStackSwitching

স্থির M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    যেতিয়ালৈ (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        যদি (M3_UNLIKELY(r)) ঘূৰাই r;

        ৱিকল্প (opcode)
        {
        // ---- Control ----
        পক্ষ 0x00: // unreachable
            v_unreachable(v);
            ভঙ্গ;

        পক্ষ 0x01: // nop
            ভঙ্গ;

        পক্ষ 0x02: // block
        পক্ষ 0x03: // loop
        পক্ষ 0x04: // if
        {
            IM3FuncType bt = রিক্ত;
            r = v_read_blocktype(v, &bt);
            যদি (r) ঘূৰাই r;
            যদি (opcode == 0x04) {
                r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a);
                যদি (r) ঘূৰাই r;
            }
            // Pop block params from caller stack
            যদি (bt) {
                বাবে (u16 i = bt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
            }
            r = v_ঠেলো_ctrl(v, opcode, bt);
            যদি (r) ঘূৰাই r;
            // Push params inside block
            যদি (bt) {
                বাবে (u16 i = 0; i < bt->অঙ্ক_Args; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

#যদি d_m3HasExceptionHandling
        পক্ষ 0x1f: // try_table
        {
            IM3FuncType bt = রিক্ত;
            r = v_read_blocktype(v, &bt);
            যদি (r) ঘূৰাই r;
            যদি (bt) {
                বাবে (u16 i = bt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 অঙ্ক_Catch;
            r = ReadLEB_u32(&অঙ্ক_Catch, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            বাবে (u32 i = 0; i < অঙ্ক_Catch; i++) {
                r = v_catch_clause(v);
                যদি (r) ঘূৰাই r;
            }

            r = v_ঠেলো_ctrl(v, opcode, bt);
            যদি (r) ঘূৰাই r;

            যদি (bt) {
                বাবে (u16 i = 0; i < bt->অঙ্ক_Args; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (!v->module || tagIndex >= v->module->অঙ্ক_Tags) ঘূৰাই m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            যদি (tt) {
                বাবে (u16 i = tt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(tt->types[tt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
            }
            v_unreachable(v);
            ভঙ্গ;
        }

        পক্ষ 0x0a: // throw_ref
            r = v_বেৰ_কৰো_expect(v, c_m3Type_exnref, &a);
            যদি (r) ঘূৰাই r;
            v_unreachable(v);
            ভঙ্গ;
#endif // d_m3HasExceptionHandling

        পক্ষ 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_বেৰ_কৰো_ctrl(v, &frame);
            যদি (r) ঘূৰাই r;
            যদি (frame.opcode != 0x04)
                ঘূৰাই m3Err_wasmMalformed;
            r = v_ঠেলো_ctrl(v, 0x05, frame.type);
            যদি (r) ঘূৰাই r;
            যদি (frame.type) {
                বাবে (u16 i = 0; i < frame.type->অঙ্ক_Args; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(frame.type->types[frame.type->অঙ্ক_Rets + i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_বেৰ_কৰো_ctrl(v, &frame);
            যদি (r) ঘূৰাই r;
            // Push results
            যদি (frame.type) {
                বাবে (u16 i = 0; i < frame.result_গুনতি; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(frame.type->types[i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            // If this was the outermost frame, we're done
            যদি (v->ctrl_শীৰ্ষ == 0)
                ঘূৰাই m3Err_none;
            ভঙ্গ;
        }

        পক্ষ 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (depth >= v->ctrl_শীৰ্ষ) ঘূৰাই m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীৰ্ষ - 1 - depth];
            r = v_বেৰ_কৰো_labels(v, tgt);
            যদি (r) ঘূৰাই r;
            v_unreachable(v);
            ভঙ্গ;
        }

        পক্ষ 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (depth >= v->ctrl_শীৰ্ষ) ঘূৰাই m3Err_unknownLabel;
            r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a);
            যদি (r) ঘূৰাই r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীৰ্ষ - 1 - depth];
            r = v_বেৰ_কৰো_labels(v, tgt);
            যদি (r) ঘূৰাই r;
            r = v_ঠেলো_labels(v, tgt);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0x0e: // br_table
        {
            u32 গুনতি;
            r = ReadLEB_u32(&গুনতি, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            বাবে (u32 i = 0; i <= গুনতি; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                যদি (r) ঘূৰাই r;
                যদি (d >= v->ctrl_শীৰ্ষ) ঘূৰাই m3Err_unknownLabel;
                যদি (i == গুনতি) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_শীৰ্ষ - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            যদি (r) ঘূৰাই r;

            বাবে (u32 i = 0; i <= গুনতি; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                যদি (r) ঘূৰাই r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_শীৰ্ষ - 1 - d];
                u16 n = v_label_n(t);
                যদি (n != arity) ঘূৰাই m3_Err_type_গুনতি_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                বাবে (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    যদি (want != c_valBottom && have != c_valBottom && want != have)
                        ঘূৰাই m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_শীৰ্ষ - 1 - defDepth];
            r = v_বেৰ_কৰো_labels(v, dt);
            যদি (r) ঘূৰাই r;
            v_unreachable(v);
            ভঙ্গ;
        }

        পক্ষ 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            যদি (ft) {
                বাবে (u16 i = ft->অঙ্ক_Rets; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
            }
            v_unreachable(v);
            ভঙ্গ;
        }

        // ---- Call ----
        পক্ষ 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (idx >= v->module->অঙ্ক_Functions) ঘূৰাই m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            যদি (ft) {
                বাবে (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
                বাবে (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            // the table must exist and hold funcrefs
            যদি (tableIdx >= v->module->অঙ্ক_Tables) ঘূৰাই m3Err_unknownTable;
            যদি (v->module->tables[tableIdx]->type != c_m3Type_funcref) ঘূৰাই m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_বেৰ_কৰো_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            যদি (r) ঘূৰাই r;
            যদি (ft) {
                বাবে (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
                বাবে (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (idx >= v->module->অঙ্ক_Functions) ঘূৰাই m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            যদি (r) ঘূৰাই r;
            যদি (ft) {
                বাবে (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
            }
            v_unreachable(v);
            ভঙ্গ;
        }

        পক্ষ 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            // the table must exist and hold funcrefs
            যদি (tableIdx >= v->module->অঙ্ক_Tables) ঘূৰাই m3Err_unknownTable;
            যদি (v->module->tables[tableIdx]->type != c_m3Type_funcref) ঘূৰাই m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            যদি (r) ঘূৰাই r;
            r = v_বেৰ_কৰো_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            যদি (r) ঘূৰাই r;
            যদি (ft) {
                বাবে (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
            }
            v_unreachable(v);
            ভঙ্গ;
        }

#যদি d_m3HasTypedRefs
        পক্ষ 0x14: // call_ref
        পক্ষ 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            যদি (isTail) {
                r = v_check_tail_results(v, ft);
                যদি (r) ঘূৰাই r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_বেৰ_কৰো_expect(v, c_m3Type_funcref, &a);
            যদি (r) ঘূৰাই r;
            যদি (ft) {
                বাবে (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
                যদি (not isTail) {
                    বাবে (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                        r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                        যদি (r) ঘূৰাই r;
                    }
                }
            }
            যদি (isTail) v_unreachable(v);
            ভঙ্গ;
        }

        পক্ষ 0xd4: // ref.as_non_null
        {
            r = v_বেৰ_কৰো_expect(v, c_m3Type_funcref, &a);
            যদি (r) ঘূৰাই r;
            r = v_ঠেলো(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        পক্ষ 0x1a: // drop
            r = v_বেৰ_কৰো(v, &a);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;

        পক্ষ 0x1b: // select
        {
            r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a);
            যদি (r) ঘূৰাই r;
            m3type_t t2 = c_valBottom;
            r = v_বেৰ_কৰো(v, &t2);
            যদি (r) ঘূৰাই r;
            m3type_t t1;
            r = v_বেৰ_কৰো_expect(v, t2, &t1);
            যদি (r) ঘূৰাই r;
            // untyped select is numeric only; references need the 0x1c form
            যদি (t2 != c_valBottom && IsRefType(t2)) ঘূৰাই m3Err_typeMismatch;
            r = v_ঠেলো(v, (t2 == c_valBottom) ? t1 : t2);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

#যদি d_m3HasRefTypes
        পক্ষ 0x1c: // select with an explicit result type
        {
            u32 অঙ্ক_Types;
            r = ReadLEB_u32(&অঙ্ক_Types, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (অঙ্ক_Types != 1) ঘূৰাই m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_মান_Type(v->module, &selType, &v->wasm, v->wasmEnd);  যদি (r) ঘূৰাই r;
            u8 t = BaseTypeOf(selType);

            r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a); যদি (r) ঘূৰাই r;
            r = v_বেৰ_কৰো_expect(v, t, &a);            যদি (r) ঘূৰাই r;
            r = v_বেৰ_কৰো_expect(v, t, &a);            যদি (r) ঘূৰাই r;
            r = v_ঠেলো(v, t);                      যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0x25: // table.get
        পক্ষ 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (tableIdx >= v->module->অঙ্ক_Tables) ঘূৰাই m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            যদি (opcode == 0x26) {
                r = v_বেৰ_কৰো_expect(v, t, &a);   যদি (r) ঘূৰাই r;
                r = v_বেৰ_কৰো_expect(v, at, &a);  যদি (r) ঘূৰাই r;
            } অন্যথা {
                r = v_বেৰ_কৰো_expect(v, at, &a);  যদি (r) ঘূৰাই r;
                r = v_ঠেলো(v, t);             যদি (r) ঘূৰাই r;
            }
            ভঙ্গ;
        }

        পক্ষ 0xd0: // ref.null
        {
#যদি d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  যদি (r) ঘূৰাই r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# যদি d_m3_Has_স্তূপ_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#অন্যথা
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   যদি (r) ঘূৰাই r;
            r = NormalizeType(&t, waType);                   যদি (r) ঘূৰাই r;
            যদি (!IsRefType(t)) ঘূৰাই m3Err_wasmMalformed;
#endif
            r = v_ঠেলো(v, t);                                যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_বেৰ_কৰো(v, &t);
            যদি (r) ঘূৰাই r;
            যদি (t != c_valBottom && !IsRefType(t)) ঘূৰাই m3Err_typeMismatch;
            r = v_ঠেলো(v, c_m3Type_i32);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (funcIdx >= v->module->অঙ্ক_Functions) ঘূৰাই m3Err_unknownFunction;
            যদি (!Module_IsFunctionDeclared(v->module, funcIdx)) ঘূৰাই m3Err_undeclaredFuncRef;
            r = v_ঠেলো(v, c_m3Type_funcref);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }
#endif

        // ---- Variable ----
        পক্ষ 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (idx >= v->অঙ্ক_Locals) ঘূৰাই m3Err_unknownLocal;
            r = v_ঠেলো(v, v->localTypes[idx]);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (idx >= v->অঙ্ক_Locals) ঘূৰাই m3Err_unknownLocal;
            r = v_বেৰ_কৰো_expect(v, v->localTypes[idx], &a);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (idx >= v->অঙ্ক_Locals) ঘূৰাই m3Err_unknownLocal;
            r = v_বেৰ_কৰো_expect(v, v->localTypes[idx], &a);
            যদি (r) ঘূৰাই r;
            r = v_ঠেলো(v, v->localTypes[idx]);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (idx >= v->module->অঙ্ক_Globals) ঘূৰাই m3Err_unknownGlobal;
            r = v_ঠেলো(v, BaseTypeOf(v->module->globals[idx].type));
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            যদি (idx >= v->module->অঙ্ক_Globals) ঘূৰাই m3Err_unknownGlobal;
            r = v_বেৰ_কৰো_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        // ---- Memory load ----
        পক্ষ 0x28: পক্ষ 0x29: পক্ষ 0x2a: পক্ষ 0x2b: // i32/i64/f32/f64.load
        পক্ষ 0x2c: পক্ষ 0x2d: পক্ষ 0x2e: পক্ষ 0x2f: // i32.load8/16 s/u
        পক্ষ 0x30: পক্ষ 0x31: পক্ষ 0x32: পক্ষ 0x33: // i64.load8/16 s/u
        পক্ষ 0x34: পক্ষ 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (align > v_max_align(opcode)) ঘূৰাই m3Err_invalidAlignment;
            যদি (not v_has_memory_idx(v, memidx)) ঘূৰাই m3Err_unknownMemory;
            যদি (not v_offset_in_range(v, memidx, offset)) ঘূৰাই m3Err_wasmMalformed;
            r = v_বেৰ_কৰো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ঘূৰাই r;
            u8 result;
            যদি      (opcode == 0x28) result = c_m3Type_i32;
            অন্যথা যদি (opcode == 0x29) result = c_m3Type_i64;
            অন্যথা যদি (opcode == 0x2a) result = c_m3Type_f32;
            অন্যথা যদি (opcode == 0x2b) result = c_m3Type_f64;
            অন্যথা যদি (opcode <= 0x2f) result = c_m3Type_i32;
            অন্যথা                     result = c_m3Type_i64;
            r = v_ঠেলো(v, result);
            যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        // ---- Memory store ----
        পক্ষ 0x36: পক্ষ 0x37: পক্ষ 0x38: পক্ষ 0x39: // i32/i64/f32/f64.store
        পক্ষ 0x3a: পক্ষ 0x3b:                         // i32.store8/16
        পক্ষ 0x3c: পক্ষ 0x3d: পক্ষ 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (align > v_max_align(opcode)) ঘূৰাই m3Err_invalidAlignment;
            যদি (not v_has_memory_idx(v, memidx)) ঘূৰাই m3Err_unknownMemory;
            যদি (not v_offset_in_range(v, memidx, offset)) ঘূৰাই m3Err_wasmMalformed;
            u8 valtype;
            যদি      (opcode == 0x36) valtype = c_m3Type_i32;
            অন্যথা যদি (opcode == 0x37) valtype = c_m3Type_i64;
            অন্যথা যদি (opcode == 0x38) valtype = c_m3Type_f32;
            অন্যথা যদি (opcode == 0x39) valtype = c_m3Type_f64;
            অন্যথা যদি (opcode <= 0x3b) valtype = c_m3Type_i32;
            অন্যথা                     valtype = c_m3Type_i64;
            r = v_বেৰ_কৰো_expect(v, valtype, &a); যদি (r) ঘূৰাই r;
            r = v_বেৰ_কৰো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        // ---- Memory size/grow ----
        পক্ষ 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (not v_has_memory_idx(v, memidx)) ঘূৰাই m3Err_unknownMemory;
            r = v_ঠেলো(v, v_memory_addrtype(v, memidx)); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }
        পক্ষ 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (not v_has_memory_idx(v, memidx)) ঘূৰাই m3Err_unknownMemory;
            r = v_বেৰ_কৰো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ঘূৰাই r;
            r = v_ঠেলো(v, v_memory_addrtype(v, memidx)); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        // ---- Constants ----
        পক্ষ 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            r = v_ঠেলো(v, c_m3Type_i32); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }
        পক্ষ 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            r = v_ঠেলো(v, c_m3Type_i64); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }
        পক্ষ 0x43: { // f32.const
            যদি (v->wasm + 4 > v->wasmEnd) ঘূৰাই m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ঠেলো(v, c_m3Type_f32); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }
        পক্ষ 0x44: { // f64.const
            যদি (v->wasm + 8 > v->wasmEnd) ঘূৰাই m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ঠেলো(v, c_m3Type_f64); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }


        // ---- i32 comparison ----
        পক্ষ 0x45: r = v_testop(v, c_m3Type_i32); ভঙ্গ; // i32.eqz
        পক্ষ 0x46: পক্ষ 0x47: পক্ষ 0x48: পক্ষ 0x49: পক্ষ 0x4a:
        পক্ষ 0x4b: পক্ষ 0x4c: পক্ষ 0x4d: পক্ষ 0x4e: পক্ষ 0x4f:
            r = v_relop(v, c_m3Type_i32); ভঙ্গ;

        // ---- i64 comparison ----
        পক্ষ 0x50: r = v_testop(v, c_m3Type_i64); ভঙ্গ; // i64.eqz
        পক্ষ 0x51: পক্ষ 0x52: পক্ষ 0x53: পক্ষ 0x54: পক্ষ 0x55:
        পক্ষ 0x56: পক্ষ 0x57: পক্ষ 0x58: পক্ষ 0x59: পক্ষ 0x5a:
            r = v_relop(v, c_m3Type_i64); ভঙ্গ;

        // ---- f32 comparison ----
        পক্ষ 0x5b: পক্ষ 0x5c: পক্ষ 0x5d: পক্ষ 0x5e: পক্ষ 0x5f: পক্ষ 0x60:
            r = v_relop(v, c_m3Type_f32); ভঙ্গ;

        // ---- f64 comparison ----
        পক্ষ 0x61: পক্ষ 0x62: পক্ষ 0x63: পক্ষ 0x64: পক্ষ 0x65: পক্ষ 0x66:
            r = v_relop(v, c_m3Type_f64); ভঙ্গ;

        // ---- i32 unary ----
        পক্ষ 0x67: পক্ষ 0x68: পক্ষ 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ভঙ্গ;

        // ---- i32 binary ----
        পক্ষ 0x6a: পক্ষ 0x6b: পক্ষ 0x6c: পক্ষ 0x6d: পক্ষ 0x6e: পক্ষ 0x6f:
        পক্ষ 0x70: পক্ষ 0x71: পক্ষ 0x72: পক্ষ 0x73: পক্ষ 0x74: পক্ষ 0x75:
        পক্ষ 0x76: পক্ষ 0x77: পক্ষ 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); ভঙ্গ;

        // ---- i64 unary ----
        পক্ষ 0x79: পক্ষ 0x7a: পক্ষ 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ভঙ্গ;

        // ---- i64 binary ----
        পক্ষ 0x7c: পক্ষ 0x7d: পক্ষ 0x7e: পক্ষ 0x7f: পক্ষ 0x80: পক্ষ 0x81:
        পক্ষ 0x82: পক্ষ 0x83: পক্ষ 0x84: পক্ষ 0x85: পক্ষ 0x86: পক্ষ 0x87:
        পক্ষ 0x88: পক্ষ 0x89: পক্ষ 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); ভঙ্গ;

        // ---- f32 unary ----
        পক্ষ 0x8b: পক্ষ 0x8c: পক্ষ 0x8d: পক্ষ 0x8e: পক্ষ 0x8f:
        পক্ষ 0x90: পক্ষ 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); ভঙ্গ;

        // ---- f32 binary ----
        পক্ষ 0x92: পক্ষ 0x93: পক্ষ 0x94: পক্ষ 0x95: পক্ষ 0x96:
        পক্ষ 0x97: পক্ষ 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); ভঙ্গ;

        // ---- f64 unary ----
        পক্ষ 0x99: পক্ষ 0x9a: পক্ষ 0x9b: পক্ষ 0x9c: পক্ষ 0x9d:
        পক্ষ 0x9e: পক্ষ 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); ভঙ্গ;

        // ---- f64 binary ----
        পক্ষ 0xa0: পক্ষ 0xa1: পক্ষ 0xa2: পক্ষ 0xa3: পক্ষ 0xa4:
        পক্ষ 0xa5: পক্ষ 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); ভঙ্গ;

        // ---- Conversions ----
        পক্ষ 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); ভঙ্গ; // i32.wrap/i64
        পক্ষ 0xa8: পক্ষ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ভঙ্গ;
        পক্ষ 0xaa: পক্ষ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ভঙ্গ;
        পক্ষ 0xac: পক্ষ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); ভঙ্গ;
        পক্ষ 0xae: পক্ষ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ভঙ্গ;
        পক্ষ 0xb0: পক্ষ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ভঙ্গ;
        পক্ষ 0xb2: পক্ষ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ভঙ্গ;
        পক্ষ 0xb4: পক্ষ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); ভঙ্গ;
        পক্ষ 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); ভঙ্গ;
        পক্ষ 0xb7: পক্ষ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); ভঙ্গ;
        পক্ষ 0xb9: পক্ষ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ভঙ্গ;
        পক্ষ 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); ভঙ্গ;
        পক্ষ 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ভঙ্গ;
        পক্ষ 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ভঙ্গ;
        পক্ষ 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ভঙ্গ;
        পক্ষ 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ভঙ্গ;

        // ---- Sign-extension (MVP post) ----
        পক্ষ 0xc0: পক্ষ 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ভঙ্গ;
        পক্ষ 0xc2: পক্ষ 0xc3: পক্ষ 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ভঙ্গ;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        পক্ষ 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;
            ৱিকল্প (sub) {
            পক্ষ 0x00: পক্ষ 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ভঙ্গ;
            পক্ষ 0x02: পক্ষ 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ভঙ্গ;
            পক্ষ 0x04: পক্ষ 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ভঙ্গ;
            পক্ষ 0x06: পক্ষ 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ভঙ্গ;
            পক্ষ 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                যদি (not v_has_memory_idx(v, memidx)) ঘূৰাই m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                যদি (not v->module->has_Data_গুনতি) ঘূৰাই m3_Err_data_গুনতি_Required;
                যদি (dataidx >= v->module->অঙ্ক_Data_Segments) ঘূৰাই m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a); যদি (r) ঘূৰাই r; // n
                r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a); যদি (r) ঘূৰাই r; // src
                r = v_বেৰ_কৰো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ঘূৰাই r; // dst
                ভঙ্গ;
            }
            পক্ষ 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                // needs the segment, but not a memory
                যদি (not v->module->has_Data_গুনতি) ঘূৰাই m3_Err_data_গুনতি_Required;
                যদি (dataidx >= v->module->অঙ্ক_Data_Segments) ঘূৰাই m3Err_unknownDataSegment;
                ভঙ্গ;
            }
#যদি d_m3HasRefTypes
            পক্ষ 0x0c: // table.init
            পক্ষ 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;

                যদি (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    যদি (a2 >= v->module->অঙ্ক_Tables) ঘূৰাই m3Err_unknownTable;
                    যদি (a1 >= v->module->অঙ্ক_Element_Segments) ঘূৰাই m3Err_unknownElemSegment;
                    যদি (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        ঘূৰাই m3Err_typeMismatch;
                } অন্যথা {                                                    // dst, src
                    যদি (a1 >= v->module->অঙ্ক_Tables) ঘূৰাই m3Err_unknownTable;
                    যদি (a2 >= v->module->অঙ্ক_Tables) ঘূৰাই m3Err_unknownTable;
                    যদি (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        ঘূৰাই m3Err_typeMismatch;
                }

                যদি (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a); যদি (r) ঘূৰাই r;             // n
                    r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a); যদি (r) ঘূৰাই r;             // s
                    r = v_বেৰ_কৰো_expect(v, v_table_addrtype(v, a2), &a); যদি (r) ঘূৰাই r;  // d
                } অন্যথা {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_বেৰ_কৰো_expect(v, nType, &a); যদি (r) ঘূৰাই r;  // n
                    r = v_বেৰ_কৰো_expect(v, sType, &a); যদি (r) ঘূৰাই r;  // s
                    r = v_বেৰ_কৰো_expect(v, dType, &a); যদি (r) ঘূৰাই r;  // d
                }
                ভঙ্গ;
            }
            পক্ষ 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                যদি (elemIdx >= v->module->অঙ্ক_Element_Segments) ঘূৰাই m3Err_unknownElemSegment;
                ভঙ্গ;
            }
            পক্ষ 0x0f: // table.grow
            পক্ষ 0x10: // table.size
            পক্ষ 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                যদি (tableIdx >= v->module->অঙ্ক_Tables) ঘূৰাই m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                যদি (sub == 0x10) {                                          // table.size
                    r = v_ঠেলো(v, at); যদি (r) ঘূৰাই r;
                } অন্যথা যদি (sub == 0x0f) {                                   // table.grow
                    r = v_বেৰ_কৰো_expect(v, at, &a); যদি (r) ঘূৰাই r;  // n
                    r = v_বেৰ_কৰো_expect(v, t, &a);  যদি (r) ঘূৰাই r;  // init
                    r = v_ঠেলো(v, at);           যদি (r) ঘূৰাই r;
                } অন্যথা {                                                    // table.fill
                    r = v_বেৰ_কৰো_expect(v, at, &a); যদি (r) ঘূৰাই r;  // n
                    r = v_বেৰ_কৰো_expect(v, t, &a);  যদি (r) ঘূৰাই r;  // val
                    r = v_বেৰ_কৰো_expect(v, at, &a); যদি (r) ঘূৰাই r;  // i
                }
                ভঙ্গ;
            }
#endif
            পক্ষ 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                যদি (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) ঘূৰাই m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_বেৰ_কৰো_expect(v, lenType, &a); যদি (r) ঘূৰাই r; // n
                r = v_বেৰ_কৰো_expect(v, srcType, &a); যদি (r) ঘূৰাই r; // src
                r = v_বেৰ_কৰো_expect(v, dstType, &a); যদি (r) ঘূৰাই r; // dst
                ভঙ্গ;
            }
            পক্ষ 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                যদি (not v_has_memory_idx(v, memidx)) ঘূৰাই m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_বেৰ_কৰো_expect(v, addrType, &a); যদি (r) ঘূৰাই r;      // n
                r = v_বেৰ_কৰো_expect(v, c_m3Type_i32, &a); যদি (r) ঘূৰাই r;  // val
                r = v_বেৰ_কৰো_expect(v, addrType, &a); যদি (r) ঘূৰাই r;      // dst
                ভঙ্গ;
            }
#যদি d_m3HasWideArithmetic
            পক্ষ 0x13: পক্ষ 0x14: // i64.add128, i64.sub128
            পক্ষ 0x15: পক্ষ 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 অঙ্ক_Operands = (sub <= 0x14) ? 4 : 2;
                বাবে (u32 i = 0; i < অঙ্ক_Operands; ++i) {
                    r = v_বেৰ_কৰো_expect(v, c_m3Type_i64, &a); যদি (r) ঘূৰাই r;
                }
                r = v_ঠেলো(v, c_m3Type_i64); যদি (r) ঘূৰাই r;
                r = v_ঠেলো(v, c_m3Type_i64); যদি (r) ঘূৰাই r;
                ভঙ্গ;
            }
#endif
            স্ৱভাৱ:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                ভঙ্গ;
            }
            ভঙ্গ;
        }

#যদি d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        পক্ষ 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            যদি (r) ঘূৰাই r;

            যদি (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                যদি (reserved != 0) ঘূৰাই m3Err_wasmMalformed;
                ভঙ্গ;
            }

            u32 width, অঙ্ক_Operands; m3type_t মান_Type, resultType;
            যদি (not v_atomic_shape(sub, &width, &অঙ্ক_Operands, &মান_Type, &resultType)) ঘূৰাই m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            যদি ((1u << align) != width) ঘূৰাই m3Err_invalidAtomicAlignment;
            যদি (not v_has_memory_idx(v, memidx)) ঘূৰাই m3Err_unknownMemory;
            যদি (not v_offset_in_range(v, memidx, offset)) ঘূৰাই m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            যদি (sub == 0x01 or sub == 0x02) {
                r = v_বেৰ_কৰো_expect(v, c_m3Type_i64, &a); যদি (r) ঘূৰাই r;
                অঙ্ক_Operands--;
            }
            বাবে (u32 i = 1; i < অঙ্ক_Operands; ++i) {
                r = v_বেৰ_কৰো_expect(v, মান_Type, &a); যদি (r) ঘূৰাই r;
            }
            r = v_বেৰ_কৰো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ঘূৰাই r;
            যদি (resultType != c_m3Type_none) {
                r = v_ঠেলো(v, resultType); যদি (r) ঘূৰাই r;
            }
            ভঙ্গ;
        }
#endif

#যদি d_m3_Has_স্তূপ_Switching
        // ---- Stack Switching ----
        পক্ষ 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            যদি (!ft || !ft->isContinuation) ঘূৰাই m3Err_typeMismatch;
            r = v_বেৰ_কৰো_expect(v, c_m3Type_funcref, &a); যদি (r) ঘূৰাই r;
            r = v_ঠেলো(v, c_m3Type_contref); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (typeIdx1 >= v->module->অঙ্ক_Func_Types || typeIdx2 >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            যদি (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) ঘূৰাই m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 অঙ্ক_Params1 = ft1->অঙ্ক_Args;
            u16 অঙ্ক_Params2 = ft2->অঙ্ক_Args;
            যদি (অঙ্ক_Params1 < অঙ্ক_Params2) ঘূৰাই m3Err_typeMismatch;
            u32 অঙ্ক_Bound = অঙ্ক_Params1 - অঙ্ক_Params2;

            যদি (ft1->অঙ্ক_Rets != ft2->অঙ্ক_Rets) ঘূৰাই m3Err_typeMismatch;
            বাবে (u16 i = 0; i < ft1->অঙ্ক_Rets; ++i) {
                যদি (ft1->types[i] != ft2->types[i]) ঘূৰাই m3Err_typeMismatch;
            }
            বাবে (u16 i = 0; i < অঙ্ক_Params2; ++i) {
                যদি (ft1->types[ft1->অঙ্ক_Rets + অঙ্ক_Bound + i] != ft2->types[ft2->অঙ্ক_Rets + i]) ঘূৰাই m3Err_typeMismatch;
            }

            r = v_বেৰ_কৰো_expect(v, c_m3Type_contref, &a); যদি (r) ঘূৰাই r;

            বাবে (u32 i = অঙ্ক_Bound; i > 0; i--) {
                r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft1->types[ft1->অঙ্ক_Rets + i - 1]), &a);
                যদি (r) ঘূৰাই r;
            }

            r = v_ঠেলো(v, c_m3Type_contref); যদি (r) ঘূৰাই r;
            ভঙ্গ;
        }

        পক্ষ 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ঘূৰাই m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            যদি (tt) {
                বাবে (u16 i = tt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(tt->types[tt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
                বাবে (u16 i = 0; i < tt->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(tt->types[i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ঘূৰাই m3Err_typeMismatch;

            u32 অঙ্ক_Clauses;
            r = ReadLEB_u32(&অঙ্ক_Clauses, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;

            বাবে (u32 i = 0; i < অঙ্ক_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                r = v_resume_clause(v, kind, tagIdx, ct); যদি (r) ঘূৰাই r;
            }

            r = v_বেৰ_কৰো_expect(v, c_m3Type_contref, &a); যদি (r) ঘূৰাই r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            যদি (ft) {
                বাবে (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
                বাবে (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, অঙ্ক_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            r = ReadLEB_u32(&অঙ্ক_Clauses, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;

            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ঘূৰাই m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ঘূৰাই m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            যদি (tagType && tagType->অঙ্ক_Rets != 0) ঘূৰাই m3Err_typeMismatch;

            বাবে (u32 i = 0; i < অঙ্ক_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                r = v_resume_clause(v, kind, cTagIdx, ct); যদি (r) ঘূৰাই r;
            }

            r = v_বেৰ_কৰো_expect(v, c_m3Type_contref, &a); যদি (r) ঘূৰাই r;

            যদি (tagType) {
                বাবে (u16 i = tagType->অঙ্ক_Args; i > 0; i--) {
                    r = v_বেৰ_কৰো_expect(v, BaseTypeOf(tagType->types[tagType->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ঘূৰাই r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            যদি (ft) {
                বাবে (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0xe5: // resume_throw_ref
        {
            u32 typeIdx, অঙ্ক_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            r = ReadLEB_u32(&অঙ্ক_Clauses, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;

            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ঘূৰাই m3Err_typeMismatch;

            বাবে (u32 i = 0; i < অঙ্ক_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
                r = v_resume_clause(v, kind, cTagIdx, ct); যদি (r) ঘূৰাই r;
            }

            r = v_বেৰ_কৰো_expect(v, c_m3Type_contref, &a); যদি (r) ঘূৰাই r;
            r = v_বেৰ_কৰো_expect(v, c_m3Type_exnref, &a); যদি (r) ঘূৰাই r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            যদি (ft) {
                বাবে (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ঘূৰাই r;
                }
            }
            ভঙ্গ;
        }

        পক্ষ 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ঘূৰাই r;

            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ঘূৰাই m3Err_unknownType;
            যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ঘূৰাই m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ঘূৰাই m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            যদি (tagType && tagType->অঙ্ক_Args != 0) ঘূৰাই m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            যদি (!ft || ft->অঙ্ক_Args == 0) ঘূৰাই m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->অঙ্ক_Rets + ft->অঙ্ক_Args - 1]);
            যদি (!peer || !peer->contFuncType) ঘূৰাই m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_বেৰ_কৰো_expect(v, c_m3Type_contref, &a); যদি (r) ঘূৰাই r;

            বাবে (u16 i = (u16)(ft->অঙ্ক_Args - 1); i > 0; i--) {
                r = v_বেৰ_কৰো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                যদি (r) ঘূৰাই r;
            }

            বাবে (u16 i = 0; i < self->অঙ্ক_Args; i++) {
                r = v_ঠেলো(v, BaseTypeOf(self->types[self->অঙ্ক_Rets + i]));
                যদি (r) ঘূৰাই r;
            }
            ভঙ্গ;
        }

#endif // d_m3HasStackSwitching

        স্ৱভাৱ:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            ভঙ্গ;

        } // switch

        যদি (r) ঘূৰাই r;

    } // while

    // If we ran out of bytes without hitting the final end
    ঘূৰাই m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    যদি (!i_function->wasm) ঘূৰাই m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    যদি (!v) {
        v = m3_AllocStruct (ValCtx);
        যদি (!v) ঘূৰাই m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, পরিমাণ(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    যদি (r) ঘূৰাই r;

    // Parse locals
    u32 অঙ্ক_Local_Blocks;
    r = ReadLEB_u32(&অঙ্ক_Local_Blocks, &v->wasm, v->wasmEnd);
    যদি (r) ঘূৰাই r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 অঙ্ক_Params = funcType ? funcType->অঙ্ক_Args : 0;
    যদি (অঙ্ক_Params > d_m3_Val_স্তূপ) ঘূৰাই m3_Err_function_স্তূপ_Overflow;
    বাবে (u16 i = 0; i < অঙ্ক_Params; i++) {
        v->localTypes[v->অঙ্ক_Locals++] = BaseTypeOf(funcType->types[funcType->অঙ্ক_Rets + i]);
    }

    // Then: declared locals
    বাবে (u32 b = 0; b < অঙ্ক_Local_Blocks; b++) {
        u32 গুনতি;
        r = ReadLEB_u32(&গুনতি, &v->wasm, v->wasmEnd);
        যদি (r) ঘূৰাই r;
        m3type_t localType;
        r = Parse_মান_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        যদি (r) ঘূৰাই r;
        u8 normalized = BaseTypeOf(localType);
        যদি (গুনতি > (u32) (d_m3_Val_স্তূপ - v->অঙ্ক_Locals)) ঘূৰাই m3_Err_function_স্তূপ_Overflow;
        বাবে (u32 c = 0; c < গুনতি; c++) {
            v->localTypes[v->অঙ্ক_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ঠেলো_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    যদি (r) ঘূৰাই r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    যদি (r) ঘূৰাই r;

    // After validation, control stack should be empty and all declared bytes consumed
    যদি (v->ctrl_শীৰ্ষ != 0)
        ঘূৰাই m3Err_wasmMalformed;

    যদি (v->wasm != v->wasmEnd)
        ঘূৰাই m3Err_wasmSectionUnderrun;

    ঘূৰাই m3Err_none;
}

#অন্যথা // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (শূন্য)i_function;
    ঘূৰাই m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
