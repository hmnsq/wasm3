//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#অন্তর্ভাব "m3_validate.h"
#অন্তর্ভাব "m3_exception.h"
#অন্তর্ভাব "m3_info.h"

#যদি d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

প্রকারলেখ কাঠামো {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_গুনতি;
    u16         ফলাফল_গুনতি;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

কাঠামো ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_স্তূপ];
    u16         opd_শীর্ষ;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_শীর্ষ;

    m3type_t    localTypes [d_m3_Val_স্তূপ];
    u16         অঙ্ক_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
স্থির bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    ফেরত v->module and i_memoryIdx < v->module->অঙ্ক_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
স্থির m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    ফেরত Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
স্থির bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    ফেরত v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
স্থির m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    ফেরত Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
স্থির u32 v_max_align (m3opcode_t opcode)
{
    নির্বাচন (opcode) {
    ক্ষেত্র 0x2c: ক্ষেত্র 0x2d:   // i32.load8_s, i32.load8_u
    ক্ষেত্র 0x30: ক্ষেত্র 0x31:   // i64.load8_s, i64.load8_u
    ক্ষেত্র 0x3a:              // i32.store8
    ক্ষেত্র 0x3c:              // i64.store8
        ফেরত 0;
    ক্ষেত্র 0x2e: ক্ষেত্র 0x2f:   // i32.load16_s, i32.load16_u
    ক্ষেত্র 0x32: ক্ষেত্র 0x33:   // i64.load16_s, i64.load16_u
    ক্ষেত্র 0x3b:              // i32.store16
    ক্ষেত্র 0x3d:              // i64.store16
        ফেরত 1;
    ক্ষেত্র 0x29:              // i64.load
    ক্ষেত্র 0x2b:              // f64.load
    ক্ষেত্র 0x37:              // i64.store
    ক্ষেত্র 0x39:              // f64.store
        ফেরত 3;
    পূর্বনির্ধারিত:                // 32-bit accesses, and a safe fallback
        ফেরত 2;
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
স্থির bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_অঙ্ক_Operands, m3type_t * o_মান_Type, m3type_t * o_ফলাফল_Type)
{
    স্থির ধ্রুবক u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    স্থির ধ্রুবক m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    যদি (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_অঙ্ক_Operands = (sub == 0x00) ? 2 : 3;
        *o_মান_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_ফলাফল_Type  = c_m3Type_i32;
        ফেরত true;
    }

    যদি (sub < 0x10 or sub > c_waOp_lastAtomic) ফেরত false;

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
    *o_ফলাফল_Type = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    ফেরত true;
}

#endif

// ---------- Operand stack ----------

স্থির M3_ফলাফল v_ঠেলো (ValCtx * v, m3type_t type)
{
    যদি (M3_UNLIKELY(v->opd_শীর্ষ >= d_m3_Val_স্তূপ))
        ফেরত m3_Err_function_স্তূপ_Overflow;
    v->opd[v->opd_শীর্ষ++] = type;
    ফেরত m3Err_none;
}

স্থির M3_ফলাফল v_বের_করো (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীর্ষ - 1];
    যদি (M3_UNLIKELY(v->opd_শীর্ষ == f->height)) {
        যদি (f->is_unreachable) { *o_type = c_valBottom; ফেরত m3Err_none; }
        ফেরত m3_Err_function_স্তূপ_Underrun;
    }
    *o_type = v->opd[--v->opd_শীর্ষ];
    ফেরত m3Err_none;
}

স্থির M3_ফলাফল v_বের_করো_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3_ফলাফল r = v_বের_করো(v, &actual);
    যদি (M3_UNLIKELY(r)) ফেরত r;
    যদি (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        ফেরত m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    ফেরত m3Err_none;
}

// ---------- Control stack ----------

স্থির M3_ফলাফল v_ঠেলো_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    যদি (M3_UNLIKELY(v->ctrl_শীর্ষ >= d_m3ValCtrlDepth))
        ফেরত m3_Err_function_স্তূপ_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীর্ষ++];
    f->opcode       = op;
    f->type         = type;
    f->param_গুনতি  = type ? type->অঙ্ক_Args : 0;
    f->ফলাফল_গুনতি = type ? type->অঙ্ক_Rets : 0;
    f->height       = v->opd_শীর্ষ;
    f->is_unreachable = false;
    ফেরত m3Err_none;
}

স্থির M3_ফলাফল v_বের_করো_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    যদি (M3_UNLIKELY(v->ctrl_শীর্ষ == 0))
        ফেরত m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীর্ষ - 1];
    // pop result types
    যদি (f->type) {
        জন্য (u16 i = f->ফলাফল_গুনতি; i > 0; i--) {
            m3type_t a;
            M3_ফলাফল r = v_বের_করো_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            যদি (M3_UNLIKELY(r)) ফেরত r;
        }
    }
    যদি (M3_UNLIKELY(v->opd_শীর্ষ != f->height))
        ফেরত m3_Err_type_গুনতি_Mismatch;
    যদি (o_frame) *o_frame = *f;
    v->ctrl_শীর্ষ--;
    ফেরত m3Err_none;
}

স্থির শূন্য v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীর্ষ - 1];
    v->opd_শীর্ষ = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
স্থির u16 v_label_n (ValCtrlFrame * f)
{
    ফেরত (f->opcode == 0x03) ? f->param_গুনতি : f->ফলাফল_গুনতি;
}

স্থির m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    যদি (!f->type) ফেরত c_m3Type_none;
    যদি (f->opcode == 0x03)
        ফেরত BaseTypeOf(f->type->types[f->type->অঙ্ক_Rets + i]); // params
    ফেরত BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
স্থির m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_শীর্ষ - 1];

    যদি (v->opd_শীর্ষ <= i_depth || (u16)(v->opd_শীর্ষ - i_depth - 1) < f->height)
        ফেরত c_valBottom;

    ফেরত v->opd[v->opd_শীর্ষ - i_depth - 1];
}

// Pop label types for branch target
স্থির M3_ফলাফল v_বের_করো_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    জন্য (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3_ফলাফল r = v_বের_করো_expect(v, v_label_t(tgt, i - 1), &a);
        যদি (M3_UNLIKELY(r)) ফেরত r;
    }
    ফেরত m3Err_none;
}

// Push label types back
স্থির M3_ফলাফল v_ঠেলো_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    জন্য (u16 i = 0; i < n; i++) {
        M3_ফলাফল r = v_ঠেলো(v, v_label_t(tgt, i));
        যদি (M3_UNLIKELY(r)) ফেরত r;
    }
    ফেরত m3Err_none;
}

// ---------- Block type resolution ----------

স্থির M3_ফলাফল v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    যদি (v->wasm >= v->wasmEnd)
        ফেরত m3Err_wasmUnderrun;

#যদি d_m3HasTypedRefs
    যদি (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3_ফলাফল rr = Parse_মান_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        যদি (rr) ফেরত rr;
        IM3FuncType ftype = ফাঁকা;
        rr = AllocFuncType(&ftype, 1);
        যদি (rr) ফেরত rr;
        ftype->অঙ্ক_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        যদি (rr) ফেরত rr;
        *o_type = ftype;
        ফেরত m3Err_none;
    }
#endif

    i64 type;
    M3_ফলাফল r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    যদি (r) ফেরত r;

    যদি (type < 0) {
        যদি (type < -64) ফেরত m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        যদি (r) ফেরত r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } অন্যথা {
        যদি ((u32)type >= v->module->অঙ্ক_Func_Types)
            ফেরত m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    ফেরত m3Err_none;
}

// ---------- Convenience ----------

স্থির M3_ফলাফল v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3_ফলাফল r = v_বের_করো_expect(v, in, &a);
    যদি (M3_UNLIKELY(r)) ফেরত r;
    ফেরত v_ঠেলো(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
স্থির M3_ফলাফল v_check_লেজ_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ফাঁকা;

    u16 অঙ্ক_Callee_Rets = i_calleeType ? i_calleeType->অঙ্ক_Rets : 0;
    u16 অঙ্ক_Func_Rets   = ft ? ft->অঙ্ক_Rets : 0;

    যদি (অঙ্ক_Callee_Rets != অঙ্ক_Func_Rets)
        ফেরত m3Err_typeMismatch;

    জন্য (u16 i = 0; i < অঙ্ক_Callee_Rets; i++) {
        যদি (i_calleeType->types[i] != ft->types[i])
            ফেরত m3Err_typeMismatch;
    }

    ফেরত m3Err_none;
}

স্থির M3_ফলাফল v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ফলাফল r;
    r = v_বের_করো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ফেরত r;
    r = v_বের_করো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ফেরত r;
    ফেরত v_ঠেলো(v, t);
}

স্থির M3_ফলাফল v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ফলাফল r;
    r = v_বের_করো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ফেরত r;
    r = v_বের_করো_expect(v, t, &a); যদি (M3_UNLIKELY(r)) ফেরত r;
    ফেরত v_ঠেলো(v, c_m3Type_i32);
}

স্থির M3_ফলাফল v_testop (ValCtx * v, m3type_t t)
{
    ফেরত v_unop(v, t, c_m3Type_i32);
}

স্থির M3_ফলাফল v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    ফেরত v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#যদি d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
স্থির M3_ফলাফল v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3_ফলাফল r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    যদি (r) ফেরত r;
    যদি (kind > 0x03) ফেরত m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = ফাঁকা;
    যদি (hasTag) {
        u32 tag_সূচক;
        r = ReadLEB_u32(&tag_সূচক, &v->wasm, v->wasmEnd);
        যদি (r) ফেরত r;
        যদি (!v->module || tag_সূচক >= v->module->অঙ্ক_Tags) ফেরত m3Err_unknownTag;
        tagType = v->module->tags[tag_সূচক].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    যদি (r) ফেরত r;
    যদি (depth >= v->ctrl_শীর্ষ) ফেরত m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীর্ষ - 1 - depth];

    u16 অঙ্ক_Payload = tagType ? tagType->অঙ্ক_Args : 0;
    u16 অঙ্ক_Label   = v_label_n(tgt);

    যদি (অঙ্ক_Label != অঙ্ক_Payload + (hasRef ? 1u : 0u))
        ফেরত m3_Err_type_গুনতি_Mismatch;

    জন্য (u16 i = 0; i < অঙ্ক_Payload; i++) {
        যদি (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->অঙ্ক_Rets + i]))
            ফেরত m3Err_typeMismatch;
    }

    যদি (hasRef and v_label_t(tgt, অঙ্ক_Payload) != c_m3Type_exnref)
        ফেরত m3Err_typeMismatch;

    ফেরত m3Err_none;
}

#endif // d_m3HasExceptionHandling

#যদি d_m3_Has_স্তূপ_Switching

স্থির M3_ফলাফল v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    যদি (kind > 0x01) ফেরত m3Err_wasmMalformed;
    যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ফেরত m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    যদি (kind == 0x00) {
        u32 depth;
        M3_ফলাফল r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
        যদি (depth >= v->ctrl_শীর্ষ) ফেরত m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীর্ষ - 1 - depth];
        u16 অঙ্ক_Payload = tagType ? tagType->অঙ্ক_Args : 0;
        u16 অঙ্ক_Label   = v_label_n(tgt);

        যদি (অঙ্ক_Label != অঙ্ক_Payload + 1u) ফেরত m3_Err_type_গুনতি_Mismatch;

        জন্য (u16 j = 0; j < অঙ্ক_Payload; j++) {
            যদি (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->অঙ্ক_Rets + j]))
                ফেরত m3Err_typeMismatch;
        }
        যদি (v_label_t(tgt, অঙ্ক_Payload) != c_m3Type_contref)
            ফেরত m3Err_typeMismatch;

#যদি d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->অঙ্ক_Rets + অঙ্ক_Payload]
            : tgt->type->types[অঙ্ক_Payload];

        যদি (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            ফেরত m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = ফাঁকা;
        জন্য (u32 idx = 0; idx < v->module->অঙ্ক_Func_Types; ++idx) {
            যদি (v->module->funcTypes[idx]->canonical_সূচক == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                বিরতি;
            }
        }
        যদি (!labelCt) ফেরত m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_অঙ্ক_Params = tagType ? tagType->অঙ্ক_Rets : 0;
        যদি (labelFt->অঙ্ক_Args != expected_অঙ্ক_Params) ফেরত m3Err_typeMismatch;
        জন্য (u16 p = 0; p < expected_অঙ্ক_Params; ++p) {
            যদি (labelFt->types[labelFt->অঙ্ক_Rets + p] != tagType->types[p])
                ফেরত m3Err_typeMismatch;
        }

        u16 expected_অঙ্ক_Rets = ft ? ft->অঙ্ক_Rets : 0;
        যদি (labelFt->অঙ্ক_Rets != expected_অঙ্ক_Rets) ফেরত m3Err_typeMismatch;
        জন্য (u16 r_idx = 0; r_idx < expected_অঙ্ক_Rets; ++r_idx) {
            যদি (labelFt->types[r_idx] != ft->types[r_idx])
                ফেরত m3Err_typeMismatch;
        }
#endif
    } অন্যথা {
        যদি (tagType && tagType->অঙ্ক_Args != 0) ফেরত m3Err_typeMismatch;
    }
    ফেরত m3Err_none;
}

#endif // d_m3HasStackSwitching

স্থির M3_ফলাফল v_validate_body (ValCtx * v)
{
    M3_ফলাফল r = m3Err_none;
    m3type_t a = c_valBottom;

    যখন (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        যদি (M3_UNLIKELY(r)) ফেরত r;

        নির্বাচন (opcode)
        {
        // ---- Control ----
        ক্ষেত্র 0x00: // unreachable
            v_unreachable(v);
            বিরতি;

        ক্ষেত্র 0x01: // nop
            বিরতি;

        ক্ষেত্র 0x02: // block
        ক্ষেত্র 0x03: // loop
        ক্ষেত্র 0x04: // if
        {
            IM3FuncType bt = ফাঁকা;
            r = v_read_blocktype(v, &bt);
            যদি (r) ফেরত r;
            যদি (opcode == 0x04) {
                r = v_বের_করো_expect(v, c_m3Type_i32, &a);
                যদি (r) ফেরত r;
            }
            // Pop block params from caller stack
            যদি (bt) {
                জন্য (u16 i = bt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
            }
            r = v_ঠেলো_ctrl(v, opcode, bt);
            যদি (r) ফেরত r;
            // Push params inside block
            যদি (bt) {
                জন্য (u16 i = 0; i < bt->অঙ্ক_Args; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

#যদি d_m3HasExceptionHandling
        ক্ষেত্র 0x1f: // try_table
        {
            IM3FuncType bt = ফাঁকা;
            r = v_read_blocktype(v, &bt);
            যদি (r) ফেরত r;
            যদি (bt) {
                জন্য (u16 i = bt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 অঙ্ক_Catch;
            r = ReadLEB_u32(&অঙ্ক_Catch, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            জন্য (u32 i = 0; i < অঙ্ক_Catch; i++) {
                r = v_catch_clause(v);
                যদি (r) ফেরত r;
            }

            r = v_ঠেলো_ctrl(v, opcode, bt);
            যদি (r) ফেরত r;

            যদি (bt) {
                জন্য (u16 i = 0; i < bt->অঙ্ক_Args; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(bt->types[bt->অঙ্ক_Rets + i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0x08: // throw
        {
            u32 tag_সূচক;
            r = ReadLEB_u32(&tag_সূচক, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (!v->module || tag_সূচক >= v->module->অঙ্ক_Tags) ফেরত m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tag_সূচক].type;
            যদি (tt) {
                জন্য (u16 i = tt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(tt->types[tt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
            }
            v_unreachable(v);
            বিরতি;
        }

        ক্ষেত্র 0x0a: // throw_ref
            r = v_বের_করো_expect(v, c_m3Type_exnref, &a);
            যদি (r) ফেরত r;
            v_unreachable(v);
            বিরতি;
#endif // d_m3HasExceptionHandling

        ক্ষেত্র 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_বের_করো_ctrl(v, &frame);
            যদি (r) ফেরত r;
            যদি (frame.opcode != 0x04)
                ফেরত m3Err_wasmMalformed;
            r = v_ঠেলো_ctrl(v, 0x05, frame.type);
            যদি (r) ফেরত r;
            যদি (frame.type) {
                জন্য (u16 i = 0; i < frame.type->অঙ্ক_Args; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(frame.type->types[frame.type->অঙ্ক_Rets + i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_বের_করো_ctrl(v, &frame);
            যদি (r) ফেরত r;
            // Push results
            যদি (frame.type) {
                জন্য (u16 i = 0; i < frame.ফলাফল_গুনতি; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(frame.type->types[i]));
                    যদি (r) ফেরত r;
                }
            }
            // If this was the outermost frame, we're done
            যদি (v->ctrl_শীর্ষ == 0)
                ফেরত m3Err_none;
            বিরতি;
        }

        ক্ষেত্র 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (depth >= v->ctrl_শীর্ষ) ফেরত m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীর্ষ - 1 - depth];
            r = v_বের_করো_labels(v, tgt);
            যদি (r) ফেরত r;
            v_unreachable(v);
            বিরতি;
        }

        ক্ষেত্র 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (depth >= v->ctrl_শীর্ষ) ফেরত m3Err_unknownLabel;
            r = v_বের_করো_expect(v, c_m3Type_i32, &a);
            যদি (r) ফেরত r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_শীর্ষ - 1 - depth];
            r = v_বের_করো_labels(v, tgt);
            যদি (r) ফেরত r;
            r = v_ঠেলো_labels(v, tgt);
            যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0x0e: // br_table
        {
            u32 গুনতি;
            r = ReadLEB_u32(&গুনতি, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            জন্য (u32 i = 0; i <= গুনতি; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                যদি (r) ফেরত r;
                যদি (d >= v->ctrl_শীর্ষ) ফেরত m3Err_unknownLabel;
                যদি (i == গুনতি) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_শীর্ষ - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_বের_করো_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            যদি (r) ফেরত r;

            জন্য (u32 i = 0; i <= গুনতি; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                যদি (r) ফেরত r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_শীর্ষ - 1 - d];
                u16 n = v_label_n(t);
                যদি (n != arity) ফেরত m3_Err_type_গুনতি_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                জন্য (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    যদি (want != c_valBottom && have != c_valBottom && want != have)
                        ফেরত m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_শীর্ষ - 1 - defDepth];
            r = v_বের_করো_labels(v, dt);
            যদি (r) ফেরত r;
            v_unreachable(v);
            বিরতি;
        }

        ক্ষেত্র 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            যদি (ft) {
                জন্য (u16 i = ft->অঙ্ক_Rets; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    যদি (r) ফেরত r;
                }
            }
            v_unreachable(v);
            বিরতি;
        }

        // ---- Call ----
        ক্ষেত্র 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (idx >= v->module->অঙ্ক_Functions) ফেরত m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            যদি (ft) {
                জন্য (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
                জন্য (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            // the table must exist and hold funcrefs
            যদি (tableIdx >= v->module->অঙ্ক_Tables) ফেরত m3Err_unknownTable;
            যদি (v->module->tables[tableIdx]->type != c_m3Type_funcref) ফেরত m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_বের_করো_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            যদি (r) ফেরত r;
            যদি (ft) {
                জন্য (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
                জন্য (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (idx >= v->module->অঙ্ক_Functions) ফেরত m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_লেজ_results(v, ft);
            যদি (r) ফেরত r;
            যদি (ft) {
                জন্য (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
            }
            v_unreachable(v);
            বিরতি;
        }

        ক্ষেত্র 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            // the table must exist and hold funcrefs
            যদি (tableIdx >= v->module->অঙ্ক_Tables) ফেরত m3Err_unknownTable;
            যদি (v->module->tables[tableIdx]->type != c_m3Type_funcref) ফেরত m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_লেজ_results(v, ft);
            যদি (r) ফেরত r;
            r = v_বের_করো_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            যদি (r) ফেরত r;
            যদি (ft) {
                জন্য (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
            }
            v_unreachable(v);
            বিরতি;
        }

#যদি d_m3HasTypedRefs
        ক্ষেত্র 0x14: // call_ref
        ক্ষেত্র 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool is_লেজ = (opcode == 0x15);
            যদি (is_লেজ) {
                r = v_check_লেজ_results(v, ft);
                যদি (r) ফেরত r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_বের_করো_expect(v, c_m3Type_funcref, &a);
            যদি (r) ফেরত r;
            যদি (ft) {
                জন্য (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
                যদি (not is_লেজ) {
                    জন্য (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                        r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                        যদি (r) ফেরত r;
                    }
                }
            }
            যদি (is_লেজ) v_unreachable(v);
            বিরতি;
        }

        ক্ষেত্র 0xd4: // ref.as_non_null
        {
            r = v_বের_করো_expect(v, c_m3Type_funcref, &a);
            যদি (r) ফেরত r;
            r = v_ঠেলো(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            যদি (r) ফেরত r;
            বিরতি;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        ক্ষেত্র 0x1a: // drop
            r = v_বের_করো(v, &a);
            যদি (r) ফেরত r;
            বিরতি;

        ক্ষেত্র 0x1b: // select
        {
            r = v_বের_করো_expect(v, c_m3Type_i32, &a);
            যদি (r) ফেরত r;
            m3type_t t2 = c_valBottom;
            r = v_বের_করো(v, &t2);
            যদি (r) ফেরত r;
            m3type_t t1;
            r = v_বের_করো_expect(v, t2, &t1);
            যদি (r) ফেরত r;
            // untyped select is numeric only; references need the 0x1c form
            যদি (t2 != c_valBottom && IsRefType(t2)) ফেরত m3Err_typeMismatch;
            r = v_ঠেলো(v, (t2 == c_valBottom) ? t1 : t2);
            যদি (r) ফেরত r;
            বিরতি;
        }

#যদি d_m3HasRefTypes
        ক্ষেত্র 0x1c: // select with an explicit result type
        {
            u32 অঙ্ক_Types;
            r = ReadLEB_u32(&অঙ্ক_Types, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (অঙ্ক_Types != 1) ফেরত m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_মান_Type(v->module, &selType, &v->wasm, v->wasmEnd);  যদি (r) ফেরত r;
            u8 t = BaseTypeOf(selType);

            r = v_বের_করো_expect(v, c_m3Type_i32, &a); যদি (r) ফেরত r;
            r = v_বের_করো_expect(v, t, &a);            যদি (r) ফেরত r;
            r = v_বের_করো_expect(v, t, &a);            যদি (r) ফেরত r;
            r = v_ঠেলো(v, t);                      যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0x25: // table.get
        ক্ষেত্র 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (tableIdx >= v->module->অঙ্ক_Tables) ফেরত m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            যদি (opcode == 0x26) {
                r = v_বের_করো_expect(v, t, &a);   যদি (r) ফেরত r;
                r = v_বের_করো_expect(v, at, &a);  যদি (r) ফেরত r;
            } অন্যথা {
                r = v_বের_করো_expect(v, at, &a);  যদি (r) ফেরত r;
                r = v_ঠেলো(v, t);             যদি (r) ফেরত r;
            }
            বিরতি;
        }

        ক্ষেত্র 0xd0: // ref.null
        {
#যদি d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  যদি (r) ফেরত r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# যদি d_m3_Has_স্তূপ_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#অন্যথা
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   যদি (r) ফেরত r;
            r = NormalizeType(&t, waType);                   যদি (r) ফেরত r;
            যদি (!IsRefType(t)) ফেরত m3Err_wasmMalformed;
#endif
            r = v_ঠেলো(v, t);                                যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_বের_করো(v, &t);
            যদি (r) ফেরত r;
            যদি (t != c_valBottom && !IsRefType(t)) ফেরত m3Err_typeMismatch;
            r = v_ঠেলো(v, c_m3Type_i32);
            যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (funcIdx >= v->module->অঙ্ক_Functions) ফেরত m3Err_unknownFunction;
            যদি (!Module_IsFunctionDeclared(v->module, funcIdx)) ফেরত m3Err_undeclaredFuncRef;
            r = v_ঠেলো(v, c_m3Type_funcref);
            যদি (r) ফেরত r;
            বিরতি;
        }
#endif

        // ---- Variable ----
        ক্ষেত্র 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (idx >= v->অঙ্ক_Locals) ফেরত m3Err_unknownLocal;
            r = v_ঠেলো(v, v->localTypes[idx]);
            যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (idx >= v->অঙ্ক_Locals) ফেরত m3Err_unknownLocal;
            r = v_বের_করো_expect(v, v->localTypes[idx], &a);
            যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (idx >= v->অঙ্ক_Locals) ফেরত m3Err_unknownLocal;
            r = v_বের_করো_expect(v, v->localTypes[idx], &a);
            যদি (r) ফেরত r;
            r = v_ঠেলো(v, v->localTypes[idx]);
            যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (idx >= v->module->অঙ্ক_Globals) ফেরত m3Err_unknownGlobal;
            r = v_ঠেলো(v, BaseTypeOf(v->module->globals[idx].type));
            যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            যদি (idx >= v->module->অঙ্ক_Globals) ফেরত m3Err_unknownGlobal;
            r = v_বের_করো_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            যদি (r) ফেরত r;
            বিরতি;
        }

        // ---- Memory load ----
        ক্ষেত্র 0x28: ক্ষেত্র 0x29: ক্ষেত্র 0x2a: ক্ষেত্র 0x2b: // i32/i64/f32/f64.load
        ক্ষেত্র 0x2c: ক্ষেত্র 0x2d: ক্ষেত্র 0x2e: ক্ষেত্র 0x2f: // i32.load8/16 s/u
        ক্ষেত্র 0x30: ক্ষেত্র 0x31: ক্ষেত্র 0x32: ক্ষেত্র 0x33: // i64.load8/16 s/u
        ক্ষেত্র 0x34: ক্ষেত্র 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (align > v_max_align(opcode)) ফেরত m3Err_invalidAlignment;
            যদি (not v_has_memory_idx(v, memidx)) ফেরত m3Err_unknownMemory;
            যদি (not v_offset_in_range(v, memidx, offset)) ফেরত m3Err_wasmMalformed;
            r = v_বের_করো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ফেরত r;
            u8 ফলাফল;
            যদি      (opcode == 0x28) ফলাফল = c_m3Type_i32;
            অন্যথা যদি (opcode == 0x29) ফলাফল = c_m3Type_i64;
            অন্যথা যদি (opcode == 0x2a) ফলাফল = c_m3Type_f32;
            অন্যথা যদি (opcode == 0x2b) ফলাফল = c_m3Type_f64;
            অন্যথা যদি (opcode <= 0x2f) ফলাফল = c_m3Type_i32;
            অন্যথা                     ফলাফল = c_m3Type_i64;
            r = v_ঠেলো(v, ফলাফল);
            যদি (r) ফেরত r;
            বিরতি;
        }

        // ---- Memory store ----
        ক্ষেত্র 0x36: ক্ষেত্র 0x37: ক্ষেত্র 0x38: ক্ষেত্র 0x39: // i32/i64/f32/f64.store
        ক্ষেত্র 0x3a: ক্ষেত্র 0x3b:                         // i32.store8/16
        ক্ষেত্র 0x3c: ক্ষেত্র 0x3d: ক্ষেত্র 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (align > v_max_align(opcode)) ফেরত m3Err_invalidAlignment;
            যদি (not v_has_memory_idx(v, memidx)) ফেরত m3Err_unknownMemory;
            যদি (not v_offset_in_range(v, memidx, offset)) ফেরত m3Err_wasmMalformed;
            u8 valtype;
            যদি      (opcode == 0x36) valtype = c_m3Type_i32;
            অন্যথা যদি (opcode == 0x37) valtype = c_m3Type_i64;
            অন্যথা যদি (opcode == 0x38) valtype = c_m3Type_f32;
            অন্যথা যদি (opcode == 0x39) valtype = c_m3Type_f64;
            অন্যথা যদি (opcode <= 0x3b) valtype = c_m3Type_i32;
            অন্যথা                     valtype = c_m3Type_i64;
            r = v_বের_করো_expect(v, valtype, &a); যদি (r) ফেরত r;
            r = v_বের_করো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ফেরত r;
            বিরতি;
        }

        // ---- Memory size/grow ----
        ক্ষেত্র 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (not v_has_memory_idx(v, memidx)) ফেরত m3Err_unknownMemory;
            r = v_ঠেলো(v, v_memory_addrtype(v, memidx)); যদি (r) ফেরত r;
            বিরতি;
        }
        ক্ষেত্র 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (not v_has_memory_idx(v, memidx)) ফেরত m3Err_unknownMemory;
            r = v_বের_করো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ফেরত r;
            r = v_ঠেলো(v, v_memory_addrtype(v, memidx)); যদি (r) ফেরত r;
            বিরতি;
        }

        // ---- Constants ----
        ক্ষেত্র 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            r = v_ঠেলো(v, c_m3Type_i32); যদি (r) ফেরত r;
            বিরতি;
        }
        ক্ষেত্র 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            r = v_ঠেলো(v, c_m3Type_i64); যদি (r) ফেরত r;
            বিরতি;
        }
        ক্ষেত্র 0x43: { // f32.const
            যদি (v->wasm + 4 > v->wasmEnd) ফেরত m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ঠেলো(v, c_m3Type_f32); যদি (r) ফেরত r;
            বিরতি;
        }
        ক্ষেত্র 0x44: { // f64.const
            যদি (v->wasm + 8 > v->wasmEnd) ফেরত m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ঠেলো(v, c_m3Type_f64); যদি (r) ফেরত r;
            বিরতি;
        }


        // ---- i32 comparison ----
        ক্ষেত্র 0x45: r = v_testop(v, c_m3Type_i32); বিরতি; // i32.eqz
        ক্ষেত্র 0x46: ক্ষেত্র 0x47: ক্ষেত্র 0x48: ক্ষেত্র 0x49: ক্ষেত্র 0x4a:
        ক্ষেত্র 0x4b: ক্ষেত্র 0x4c: ক্ষেত্র 0x4d: ক্ষেত্র 0x4e: ক্ষেত্র 0x4f:
            r = v_relop(v, c_m3Type_i32); বিরতি;

        // ---- i64 comparison ----
        ক্ষেত্র 0x50: r = v_testop(v, c_m3Type_i64); বিরতি; // i64.eqz
        ক্ষেত্র 0x51: ক্ষেত্র 0x52: ক্ষেত্র 0x53: ক্ষেত্র 0x54: ক্ষেত্র 0x55:
        ক্ষেত্র 0x56: ক্ষেত্র 0x57: ক্ষেত্র 0x58: ক্ষেত্র 0x59: ক্ষেত্র 0x5a:
            r = v_relop(v, c_m3Type_i64); বিরতি;

        // ---- f32 comparison ----
        ক্ষেত্র 0x5b: ক্ষেত্র 0x5c: ক্ষেত্র 0x5d: ক্ষেত্র 0x5e: ক্ষেত্র 0x5f: ক্ষেত্র 0x60:
            r = v_relop(v, c_m3Type_f32); বিরতি;

        // ---- f64 comparison ----
        ক্ষেত্র 0x61: ক্ষেত্র 0x62: ক্ষেত্র 0x63: ক্ষেত্র 0x64: ক্ষেত্র 0x65: ক্ষেত্র 0x66:
            r = v_relop(v, c_m3Type_f64); বিরতি;

        // ---- i32 unary ----
        ক্ষেত্র 0x67: ক্ষেত্র 0x68: ক্ষেত্র 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); বিরতি;

        // ---- i32 binary ----
        ক্ষেত্র 0x6a: ক্ষেত্র 0x6b: ক্ষেত্র 0x6c: ক্ষেত্র 0x6d: ক্ষেত্র 0x6e: ক্ষেত্র 0x6f:
        ক্ষেত্র 0x70: ক্ষেত্র 0x71: ক্ষেত্র 0x72: ক্ষেত্র 0x73: ক্ষেত্র 0x74: ক্ষেত্র 0x75:
        ক্ষেত্র 0x76: ক্ষেত্র 0x77: ক্ষেত্র 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); বিরতি;

        // ---- i64 unary ----
        ক্ষেত্র 0x79: ক্ষেত্র 0x7a: ক্ষেত্র 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); বিরতি;

        // ---- i64 binary ----
        ক্ষেত্র 0x7c: ক্ষেত্র 0x7d: ক্ষেত্র 0x7e: ক্ষেত্র 0x7f: ক্ষেত্র 0x80: ক্ষেত্র 0x81:
        ক্ষেত্র 0x82: ক্ষেত্র 0x83: ক্ষেত্র 0x84: ক্ষেত্র 0x85: ক্ষেত্র 0x86: ক্ষেত্র 0x87:
        ক্ষেত্র 0x88: ক্ষেত্র 0x89: ক্ষেত্র 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); বিরতি;

        // ---- f32 unary ----
        ক্ষেত্র 0x8b: ক্ষেত্র 0x8c: ক্ষেত্র 0x8d: ক্ষেত্র 0x8e: ক্ষেত্র 0x8f:
        ক্ষেত্র 0x90: ক্ষেত্র 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); বিরতি;

        // ---- f32 binary ----
        ক্ষেত্র 0x92: ক্ষেত্র 0x93: ক্ষেত্র 0x94: ক্ষেত্র 0x95: ক্ষেত্র 0x96:
        ক্ষেত্র 0x97: ক্ষেত্র 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); বিরতি;

        // ---- f64 unary ----
        ক্ষেত্র 0x99: ক্ষেত্র 0x9a: ক্ষেত্র 0x9b: ক্ষেত্র 0x9c: ক্ষেত্র 0x9d:
        ক্ষেত্র 0x9e: ক্ষেত্র 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); বিরতি;

        // ---- f64 binary ----
        ক্ষেত্র 0xa0: ক্ষেত্র 0xa1: ক্ষেত্র 0xa2: ক্ষেত্র 0xa3: ক্ষেত্র 0xa4:
        ক্ষেত্র 0xa5: ক্ষেত্র 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); বিরতি;

        // ---- Conversions ----
        ক্ষেত্র 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); বিরতি; // i32.wrap/i64
        ক্ষেত্র 0xa8: ক্ষেত্র 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); বিরতি;
        ক্ষেত্র 0xaa: ক্ষেত্র 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); বিরতি;
        ক্ষেত্র 0xac: ক্ষেত্র 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); বিরতি;
        ক্ষেত্র 0xae: ক্ষেত্র 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); বিরতি;
        ক্ষেত্র 0xb0: ক্ষেত্র 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); বিরতি;
        ক্ষেত্র 0xb2: ক্ষেত্র 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); বিরতি;
        ক্ষেত্র 0xb4: ক্ষেত্র 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); বিরতি;
        ক্ষেত্র 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); বিরতি;
        ক্ষেত্র 0xb7: ক্ষেত্র 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); বিরতি;
        ক্ষেত্র 0xb9: ক্ষেত্র 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); বিরতি;
        ক্ষেত্র 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); বিরতি;
        ক্ষেত্র 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); বিরতি;
        ক্ষেত্র 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); বিরতি;
        ক্ষেত্র 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); বিরতি;
        ক্ষেত্র 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); বিরতি;

        // ---- Sign-extension (MVP post) ----
        ক্ষেত্র 0xc0: ক্ষেত্র 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); বিরতি;
        ক্ষেত্র 0xc2: ক্ষেত্র 0xc3: ক্ষেত্র 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); বিরতি;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        ক্ষেত্র 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;
            নির্বাচন (sub) {
            ক্ষেত্র 0x00: ক্ষেত্র 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); বিরতি;
            ক্ষেত্র 0x02: ক্ষেত্র 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); বিরতি;
            ক্ষেত্র 0x04: ক্ষেত্র 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); বিরতি;
            ক্ষেত্র 0x06: ক্ষেত্র 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); বিরতি;
            ক্ষেত্র 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                যদি (not v_has_memory_idx(v, memidx)) ফেরত m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                যদি (not v->module->has_তথ্য_গুনতি) ফেরত m3_Err_তথ্য_গুনতি_Required;
                যদি (dataidx >= v->module->অঙ্ক_তথ্য_Segments) ফেরত m3_Err_unknown_তথ্য_Segment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_বের_করো_expect(v, c_m3Type_i32, &a); যদি (r) ফেরত r; // n
                r = v_বের_করো_expect(v, c_m3Type_i32, &a); যদি (r) ফেরত r; // src
                r = v_বের_করো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ফেরত r; // dst
                বিরতি;
            }
            ক্ষেত্র 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                // needs the segment, but not a memory
                যদি (not v->module->has_তথ্য_গুনতি) ফেরত m3_Err_তথ্য_গুনতি_Required;
                যদি (dataidx >= v->module->অঙ্ক_তথ্য_Segments) ফেরত m3_Err_unknown_তথ্য_Segment;
                বিরতি;
            }
#যদি d_m3HasRefTypes
            ক্ষেত্র 0x0c: // table.init
            ক্ষেত্র 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;

                যদি (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    যদি (a2 >= v->module->অঙ্ক_Tables) ফেরত m3Err_unknownTable;
                    যদি (a1 >= v->module->অঙ্ক_Element_Segments) ফেরত m3Err_unknownElemSegment;
                    যদি (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        ফেরত m3Err_typeMismatch;
                } অন্যথা {                                                    // dst, src
                    যদি (a1 >= v->module->অঙ্ক_Tables) ফেরত m3Err_unknownTable;
                    যদি (a2 >= v->module->অঙ্ক_Tables) ফেরত m3Err_unknownTable;
                    যদি (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        ফেরত m3Err_typeMismatch;
                }

                যদি (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_বের_করো_expect(v, c_m3Type_i32, &a); যদি (r) ফেরত r;             // n
                    r = v_বের_করো_expect(v, c_m3Type_i32, &a); যদি (r) ফেরত r;             // s
                    r = v_বের_করো_expect(v, v_table_addrtype(v, a2), &a); যদি (r) ফেরত r;  // d
                } অন্যথা {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_বের_করো_expect(v, nType, &a); যদি (r) ফেরত r;  // n
                    r = v_বের_করো_expect(v, sType, &a); যদি (r) ফেরত r;  // s
                    r = v_বের_করো_expect(v, dType, &a); যদি (r) ফেরত r;  // d
                }
                বিরতি;
            }
            ক্ষেত্র 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                যদি (elemIdx >= v->module->অঙ্ক_Element_Segments) ফেরত m3Err_unknownElemSegment;
                বিরতি;
            }
            ক্ষেত্র 0x0f: // table.grow
            ক্ষেত্র 0x10: // table.size
            ক্ষেত্র 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                যদি (tableIdx >= v->module->অঙ্ক_Tables) ফেরত m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                যদি (sub == 0x10) {                                          // table.size
                    r = v_ঠেলো(v, at); যদি (r) ফেরত r;
                } অন্যথা যদি (sub == 0x0f) {                                   // table.grow
                    r = v_বের_করো_expect(v, at, &a); যদি (r) ফেরত r;  // n
                    r = v_বের_করো_expect(v, t, &a);  যদি (r) ফেরত r;  // init
                    r = v_ঠেলো(v, at);           যদি (r) ফেরত r;
                } অন্যথা {                                                    // table.fill
                    r = v_বের_করো_expect(v, at, &a); যদি (r) ফেরত r;  // n
                    r = v_বের_করো_expect(v, t, &a);  যদি (r) ফেরত r;  // val
                    r = v_বের_করো_expect(v, at, &a); যদি (r) ফেরত r;  // i
                }
                বিরতি;
            }
#endif
            ক্ষেত্র 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                যদি (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) ফেরত m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_বের_করো_expect(v, lenType, &a); যদি (r) ফেরত r; // n
                r = v_বের_করো_expect(v, srcType, &a); যদি (r) ফেরত r; // src
                r = v_বের_করো_expect(v, dstType, &a); যদি (r) ফেরত r; // dst
                বিরতি;
            }
            ক্ষেত্র 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                যদি (not v_has_memory_idx(v, memidx)) ফেরত m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_বের_করো_expect(v, addrType, &a); যদি (r) ফেরত r;      // n
                r = v_বের_করো_expect(v, c_m3Type_i32, &a); যদি (r) ফেরত r;  // val
                r = v_বের_করো_expect(v, addrType, &a); যদি (r) ফেরত r;      // dst
                বিরতি;
            }
#যদি d_m3HasWideArithmetic
            ক্ষেত্র 0x13: ক্ষেত্র 0x14: // i64.add128, i64.sub128
            ক্ষেত্র 0x15: ক্ষেত্র 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 অঙ্ক_Operands = (sub <= 0x14) ? 4 : 2;
                জন্য (u32 i = 0; i < অঙ্ক_Operands; ++i) {
                    r = v_বের_করো_expect(v, c_m3Type_i64, &a); যদি (r) ফেরত r;
                }
                r = v_ঠেলো(v, c_m3Type_i64); যদি (r) ফেরত r;
                r = v_ঠেলো(v, c_m3Type_i64); যদি (r) ফেরত r;
                বিরতি;
            }
#endif
            পূর্বনির্ধারিত:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                বিরতি;
            }
            বিরতি;
        }

#যদি d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        ক্ষেত্র 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            যদি (r) ফেরত r;

            যদি (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                যদি (reserved != 0) ফেরত m3Err_wasmMalformed;
                বিরতি;
            }

            u32 width, অঙ্ক_Operands; m3type_t মান_Type, ফলাফল_Type;
            যদি (not v_atomic_shape(sub, &width, &অঙ্ক_Operands, &মান_Type, &ফলাফল_Type)) ফেরত m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            যদি ((1u << align) != width) ফেরত m3Err_invalidAtomicAlignment;
            যদি (not v_has_memory_idx(v, memidx)) ফেরত m3Err_unknownMemory;
            যদি (not v_offset_in_range(v, memidx, offset)) ফেরত m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            যদি (sub == 0x01 or sub == 0x02) {
                r = v_বের_করো_expect(v, c_m3Type_i64, &a); যদি (r) ফেরত r;
                অঙ্ক_Operands--;
            }
            জন্য (u32 i = 1; i < অঙ্ক_Operands; ++i) {
                r = v_বের_করো_expect(v, মান_Type, &a); যদি (r) ফেরত r;
            }
            r = v_বের_করো_expect(v, v_memory_addrtype(v, memidx), &a); যদি (r) ফেরত r;
            যদি (ফলাফল_Type != c_m3Type_none) {
                r = v_ঠেলো(v, ফলাফল_Type); যদি (r) ফেরত r;
            }
            বিরতি;
        }
#endif

#যদি d_m3_Has_স্তূপ_Switching
        // ---- Stack Switching ----
        ক্ষেত্র 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            যদি (!ft || !ft->isContinuation) ফেরত m3Err_typeMismatch;
            r = v_বের_করো_expect(v, c_m3Type_funcref, &a); যদি (r) ফেরত r;
            r = v_ঠেলো(v, c_m3Type_contref); যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (typeIdx1 >= v->module->অঙ্ক_Func_Types || typeIdx2 >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            যদি (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) ফেরত m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 অঙ্ক_Params1 = ft1->অঙ্ক_Args;
            u16 অঙ্ক_Params2 = ft2->অঙ্ক_Args;
            যদি (অঙ্ক_Params1 < অঙ্ক_Params2) ফেরত m3Err_typeMismatch;
            u32 অঙ্ক_Bound = অঙ্ক_Params1 - অঙ্ক_Params2;

            যদি (ft1->অঙ্ক_Rets != ft2->অঙ্ক_Rets) ফেরত m3Err_typeMismatch;
            জন্য (u16 i = 0; i < ft1->অঙ্ক_Rets; ++i) {
                যদি (ft1->types[i] != ft2->types[i]) ফেরত m3Err_typeMismatch;
            }
            জন্য (u16 i = 0; i < অঙ্ক_Params2; ++i) {
                যদি (ft1->types[ft1->অঙ্ক_Rets + অঙ্ক_Bound + i] != ft2->types[ft2->অঙ্ক_Rets + i]) ফেরত m3Err_typeMismatch;
            }

            r = v_বের_করো_expect(v, c_m3Type_contref, &a); যদি (r) ফেরত r;

            জন্য (u32 i = অঙ্ক_Bound; i > 0; i--) {
                r = v_বের_করো_expect(v, BaseTypeOf(ft1->types[ft1->অঙ্ক_Rets + i - 1]), &a);
                যদি (r) ফেরত r;
            }

            r = v_ঠেলো(v, c_m3Type_contref); যদি (r) ফেরত r;
            বিরতি;
        }

        ক্ষেত্র 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ফেরত m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            যদি (tt) {
                জন্য (u16 i = tt->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(tt->types[tt->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
                জন্য (u16 i = 0; i < tt->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(tt->types[i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ফেরত m3Err_typeMismatch;

            u32 অঙ্ক_Clauses;
            r = ReadLEB_u32(&অঙ্ক_Clauses, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;

            জন্য (u32 i = 0; i < অঙ্ক_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                r = v_resume_clause(v, kind, tagIdx, ct); যদি (r) ফেরত r;
            }

            r = v_বের_করো_expect(v, c_m3Type_contref, &a); যদি (r) ফেরত r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            যদি (ft) {
                জন্য (u16 i = ft->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
                জন্য (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, অঙ্ক_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            r = ReadLEB_u32(&অঙ্ক_Clauses, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;

            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ফেরত m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ফেরত m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            যদি (tagType && tagType->অঙ্ক_Rets != 0) ফেরত m3Err_typeMismatch;

            জন্য (u32 i = 0; i < অঙ্ক_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                r = v_resume_clause(v, kind, cTagIdx, ct); যদি (r) ফেরত r;
            }

            r = v_বের_করো_expect(v, c_m3Type_contref, &a); যদি (r) ফেরত r;

            যদি (tagType) {
                জন্য (u16 i = tagType->অঙ্ক_Args; i > 0; i--) {
                    r = v_বের_করো_expect(v, BaseTypeOf(tagType->types[tagType->অঙ্ক_Rets + i - 1]), &a);
                    যদি (r) ফেরত r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            যদি (ft) {
                জন্য (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0xe5: // resume_throw_ref
        {
            u32 typeIdx, অঙ্ক_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            r = ReadLEB_u32(&অঙ্ক_Clauses, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;

            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ফেরত m3Err_typeMismatch;

            জন্য (u32 i = 0; i < অঙ্ক_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
                r = v_resume_clause(v, kind, cTagIdx, ct); যদি (r) ফেরত r;
            }

            r = v_বের_করো_expect(v, c_m3Type_contref, &a); যদি (r) ফেরত r;
            r = v_বের_করো_expect(v, c_m3Type_exnref, &a); যদি (r) ফেরত r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            যদি (ft) {
                জন্য (u16 i = 0; i < ft->অঙ্ক_Rets; i++) {
                    r = v_ঠেলো(v, BaseTypeOf(ft->types[i]));
                    যদি (r) ফেরত r;
                }
            }
            বিরতি;
        }

        ক্ষেত্র 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); যদি (r) ফেরত r;

            যদি (typeIdx >= v->module->অঙ্ক_Func_Types) ফেরত m3Err_unknownType;
            যদি (!v->module || tagIdx >= v->module->অঙ্ক_Tags) ফেরত m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            যদি (!ct || !ct->isContinuation) ফেরত m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            যদি (tagType && tagType->অঙ্ক_Args != 0) ফেরত m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            যদি (!ft || ft->অঙ্ক_Args == 0) ফেরত m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->অঙ্ক_Rets + ft->অঙ্ক_Args - 1]);
            যদি (!peer || !peer->contFuncType) ফেরত m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_বের_করো_expect(v, c_m3Type_contref, &a); যদি (r) ফেরত r;

            জন্য (u16 i = (u16)(ft->অঙ্ক_Args - 1); i > 0; i--) {
                r = v_বের_করো_expect(v, BaseTypeOf(ft->types[ft->অঙ্ক_Rets + i - 1]), &a);
                যদি (r) ফেরত r;
            }

            জন্য (u16 i = 0; i < self->অঙ্ক_Args; i++) {
                r = v_ঠেলো(v, BaseTypeOf(self->types[self->অঙ্ক_Rets + i]));
                যদি (r) ফেরত r;
            }
            বিরতি;
        }

#endif // d_m3HasStackSwitching

        পূর্বনির্ধারিত:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            বিরতি;

        } // switch

        যদি (r) ফেরত r;

    } // while

    // If we ran out of bytes without hitting the final end
    ফেরত m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3_ফলাফল  ValidateFunction  (IM3Function i_function)
{
    যদি (!i_function->wasm) ফেরত m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    যদি (!v) {
        v = m3_AllocStruct (ValCtx);
        যদি (!v) ফেরত m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, আকার(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 আকার_2;
    M3_ফলাফল r = ReadLEB_u32(&আকার_2, &v->wasm, v->wasmEnd);
    যদি (r) ফেরত r;

    // Parse locals
    u32 অঙ্ক_Local_Blocks;
    r = ReadLEB_u32(&অঙ্ক_Local_Blocks, &v->wasm, v->wasmEnd);
    যদি (r) ফেরত r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 অঙ্ক_Params = funcType ? funcType->অঙ্ক_Args : 0;
    যদি (অঙ্ক_Params > d_m3_Val_স্তূপ) ফেরত m3_Err_function_স্তূপ_Overflow;
    জন্য (u16 i = 0; i < অঙ্ক_Params; i++) {
        v->localTypes[v->অঙ্ক_Locals++] = BaseTypeOf(funcType->types[funcType->অঙ্ক_Rets + i]);
    }

    // Then: declared locals
    জন্য (u32 b = 0; b < অঙ্ক_Local_Blocks; b++) {
        u32 গুনতি;
        r = ReadLEB_u32(&গুনতি, &v->wasm, v->wasmEnd);
        যদি (r) ফেরত r;
        m3type_t localType;
        r = Parse_মান_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        যদি (r) ফেরত r;
        u8 normalized = BaseTypeOf(localType);
        যদি (গুনতি > (u32) (d_m3_Val_স্তূপ - v->অঙ্ক_Locals)) ফেরত m3_Err_function_স্তূপ_Overflow;
        জন্য (u32 c = 0; c < গুনতি; c++) {
            v->localTypes[v->অঙ্ক_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ঠেলো_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    যদি (r) ফেরত r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    যদি (r) ফেরত r;

    // After validation, control stack should be empty and all declared bytes consumed
    যদি (v->ctrl_শীর্ষ != 0)
        ফেরত m3Err_wasmMalformed;

    যদি (v->wasm != v->wasmEnd)
        ফেরত m3Err_wasmSectionUnderrun;

    ফেরত m3Err_none;
}

#অন্যথা // !d_m3EnableValidation

M3_ফলাফল  ValidateFunction  (IM3Function i_function)
{
    (শূন্য)i_function;
    ফেরত m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
