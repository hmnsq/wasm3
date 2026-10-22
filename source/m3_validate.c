//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#अन्तर्भाव "m3_validate.h"
#अन्तर्भाव "m3_exception.h"
#अन्तर्भाव "m3_info.h"

#जइ d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

प्रकारनिर्देश संरचना {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_गणनम्;
    u16         result_गणनम्;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

संरचना ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_राशिः];
    u16         opd_शिखरम्;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_शिखरम्;

    m3type_t    localTypes [d_m3_Val_राशिः];
    u16         अङ्कः_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
स्थिर bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    प्रत्यागम v->module and i_memoryIdx < v->module->अङ्कः_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
स्थिर m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    प्रत्यागम Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
स्थिर bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    प्रत्यागम v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
स्थिर m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    प्रत्यागम Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
स्थिर u32 v_max_align (m3opcode_t opcode)
{
    विकल्प (opcode) {
    पक्ष 0x2c: पक्ष 0x2d:   // i32.load8_s, i32.load8_u
    पक्ष 0x30: पक्ष 0x31:   // i64.load8_s, i64.load8_u
    पक्ष 0x3a:              // i32.store8
    पक्ष 0x3c:              // i64.store8
        प्रत्यागम 0;
    पक्ष 0x2e: पक्ष 0x2f:   // i32.load16_s, i32.load16_u
    पक्ष 0x32: पक्ष 0x33:   // i64.load16_s, i64.load16_u
    पक्ष 0x3b:              // i32.store16
    पक्ष 0x3d:              // i64.store16
        प्रत्यागम 1;
    पक्ष 0x29:              // i64.load
    पक्ष 0x2b:              // f64.load
    पक्ष 0x37:              // i64.store
    पक्ष 0x39:              // f64.store
        प्रत्यागम 3;
    स्वभाव:                // 32-bit accesses, and a safe fallback
        प्रत्यागम 2;
    }
}

#जइ d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
स्थिर bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_अङ्कः_Operands, m3type_t * o_मानम्_Type, m3type_t * o_resultType)
{
    स्थिर ध्रुव u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    स्थिर ध्रुव m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    जइ (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_अङ्कः_Operands = (sub == 0x00) ? 2 : 3;
        *o_मानम्_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        प्रत्यागम true;
    }

    जइ (sub < 0x10 or sub > c_waOp_lastAtomic) प्रत्यागम false;

    जइ (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_अङ्कः_Operands = 1;
    } अण्णहा जइ (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_अङ्कः_Operands = 2;
    } अण्णहा जइ (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_अङ्कः_Operands = 2;
    } अण्णहा {                        // cmpxchg
        variant = sub - 0x48;       *o_अङ्कः_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_मानम्_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    प्रत्यागम true;
}

#endif

// ---------- Operand stack ----------

स्थिर M3Result v_प्रक्षिप (ValCtx * v, m3type_t type)
{
    जइ (M3_UNLIKELY(v->opd_शिखरम् >= d_m3_Val_राशिः))
        प्रत्यागम m3_Err_function_राशिः_Overflow;
    v->opd[v->opd_शिखरम्++] = type;
    प्रत्यागम m3Err_none;
}

स्थिर M3Result v_निष्कासय (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शिखरम् - 1];
    जइ (M3_UNLIKELY(v->opd_शिखरम् == f->height)) {
        जइ (f->is_unreachable) { *o_type = c_valBottom; प्रत्यागम m3Err_none; }
        प्रत्यागम m3_Err_function_राशिः_Underrun;
    }
    *o_type = v->opd[--v->opd_शिखरम्];
    प्रत्यागम m3Err_none;
}

स्थिर M3Result v_निष्कासय_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_निष्कासय(v, &actual);
    जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    जइ (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        प्रत्यागम m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    प्रत्यागम m3Err_none;
}

// ---------- Control stack ----------

स्थिर M3Result v_प्रक्षिप_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    जइ (M3_UNLIKELY(v->ctrl_शिखरम् >= d_m3ValCtrlDepth))
        प्रत्यागम m3_Err_function_राशिः_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शिखरम्++];
    f->opcode       = op;
    f->type         = type;
    f->param_गणनम्  = type ? type->अङ्कः_Args : 0;
    f->result_गणनम् = type ? type->अङ्कः_Rets : 0;
    f->height       = v->opd_शिखरम्;
    f->is_unreachable = false;
    प्रत्यागम m3Err_none;
}

स्थिर M3Result v_निष्कासय_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    जइ (M3_UNLIKELY(v->ctrl_शिखरम् == 0))
        प्रत्यागम m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शिखरम् - 1];
    // pop result types
    जइ (f->type) {
        प्रति (u16 i = f->result_गणनम्; i > 0; i--) {
            m3type_t a;
            M3Result r = v_निष्कासय_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            जइ (M3_UNLIKELY(r)) प्रत्यागम r;
        }
    }
    जइ (M3_UNLIKELY(v->opd_शिखरम् != f->height))
        प्रत्यागम m3_Err_type_गणनम्_Mismatch;
    जइ (o_frame) *o_frame = *f;
    v->ctrl_शिखरम्--;
    प्रत्यागम m3Err_none;
}

स्थिर शून्य v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शिखरम् - 1];
    v->opd_शिखरम् = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
स्थिर u16 v_label_n (ValCtrlFrame * f)
{
    प्रत्यागम (f->opcode == 0x03) ? f->param_गणनम् : f->result_गणनम्;
}

स्थिर m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    जइ (!f->type) प्रत्यागम c_m3Type_none;
    जइ (f->opcode == 0x03)
        प्रत्यागम BaseTypeOf(f->type->types[f->type->अङ्कः_Rets + i]); // params
    प्रत्यागम BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
स्थिर m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शिखरम् - 1];

    जइ (v->opd_शिखरम् <= i_depth || (u16)(v->opd_शिखरम् - i_depth - 1) < f->height)
        प्रत्यागम c_valBottom;

    प्रत्यागम v->opd[v->opd_शिखरम् - i_depth - 1];
}

// Pop label types for branch target
स्थिर M3Result v_निष्कासय_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    प्रति (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_निष्कासय_expect(v, v_label_t(tgt, i - 1), &a);
        जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    }
    प्रत्यागम m3Err_none;
}

// Push label types back
स्थिर M3Result v_प्रक्षिप_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    प्रति (u16 i = 0; i < n; i++) {
        M3Result r = v_प्रक्षिप(v, v_label_t(tgt, i));
        जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    }
    प्रत्यागम m3Err_none;
}

// ---------- Block type resolution ----------

स्थिर M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    जइ (v->wasm >= v->wasmEnd)
        प्रत्यागम m3Err_wasmUnderrun;

#जइ d_m3HasTypedRefs
    जइ (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_मानम्_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        जइ (rr) प्रत्यागम rr;
        IM3FuncType ftype = रिक्त;
        rr = AllocFuncType(&ftype, 1);
        जइ (rr) प्रत्यागम rr;
        ftype->अङ्कः_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        जइ (rr) प्रत्यागम rr;
        *o_type = ftype;
        प्रत्यागम m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    जइ (r) प्रत्यागम r;

    जइ (type < 0) {
        जइ (type < -64) प्रत्यागम m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        जइ (r) प्रत्यागम r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } अण्णहा {
        जइ ((u32)type >= v->module->अङ्कः_Func_Types)
            प्रत्यागम m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    प्रत्यागम m3Err_none;
}

// ---------- Convenience ----------

स्थिर M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_निष्कासय_expect(v, in, &a);
    जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    प्रत्यागम v_प्रक्षिप(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
स्थिर M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : रिक्त;

    u16 अङ्कः_Callee_Rets = i_calleeType ? i_calleeType->अङ्कः_Rets : 0;
    u16 अङ्कः_Func_Rets   = ft ? ft->अङ्कः_Rets : 0;

    जइ (अङ्कः_Callee_Rets != अङ्कः_Func_Rets)
        प्रत्यागम m3Err_typeMismatch;

    प्रति (u16 i = 0; i < अङ्कः_Callee_Rets; i++) {
        जइ (i_calleeType->types[i] != ft->types[i])
            प्रत्यागम m3Err_typeMismatch;
    }

    प्रत्यागम m3Err_none;
}

स्थिर M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_निष्कासय_expect(v, t, &a); जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    r = v_निष्कासय_expect(v, t, &a); जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    प्रत्यागम v_प्रक्षिप(v, t);
}

स्थिर M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_निष्कासय_expect(v, t, &a); जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    r = v_निष्कासय_expect(v, t, &a); जइ (M3_UNLIKELY(r)) प्रत्यागम r;
    प्रत्यागम v_प्रक्षिप(v, c_m3Type_i32);
}

स्थिर M3Result v_testop (ValCtx * v, m3type_t t)
{
    प्रत्यागम v_unop(v, t, c_m3Type_i32);
}

स्थिर M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    प्रत्यागम v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#जइ d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
स्थिर M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    जइ (r) प्रत्यागम r;
    जइ (kind > 0x03) प्रत्यागम m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = रिक्त;
    जइ (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        जइ (r) प्रत्यागम r;
        जइ (!v->module || tagIndex >= v->module->अङ्कः_Tags) प्रत्यागम m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    जइ (r) प्रत्यागम r;
    जइ (depth >= v->ctrl_शिखरम्) प्रत्यागम m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शिखरम् - 1 - depth];

    u16 अङ्कः_Payload = tagType ? tagType->अङ्कः_Args : 0;
    u16 अङ्कः_Label   = v_label_n(tgt);

    जइ (अङ्कः_Label != अङ्कः_Payload + (hasRef ? 1u : 0u))
        प्रत्यागम m3_Err_type_गणनम्_Mismatch;

    प्रति (u16 i = 0; i < अङ्कः_Payload; i++) {
        जइ (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->अङ्कः_Rets + i]))
            प्रत्यागम m3Err_typeMismatch;
    }

    जइ (hasRef and v_label_t(tgt, अङ्कः_Payload) != c_m3Type_exnref)
        प्रत्यागम m3Err_typeMismatch;

    प्रत्यागम m3Err_none;
}

#endif // d_m3HasExceptionHandling

#जइ d_m3_Has_राशिः_Switching

स्थिर M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    जइ (kind > 0x01) प्रत्यागम m3Err_wasmMalformed;
    जइ (!v->module || tagIdx >= v->module->अङ्कः_Tags) प्रत्यागम m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    जइ (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
        जइ (depth >= v->ctrl_शिखरम्) प्रत्यागम m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शिखरम् - 1 - depth];
        u16 अङ्कः_Payload = tagType ? tagType->अङ्कः_Args : 0;
        u16 अङ्कः_Label   = v_label_n(tgt);

        जइ (अङ्कः_Label != अङ्कः_Payload + 1u) प्रत्यागम m3_Err_type_गणनम्_Mismatch;

        प्रति (u16 j = 0; j < अङ्कः_Payload; j++) {
            जइ (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->अङ्कः_Rets + j]))
                प्रत्यागम m3Err_typeMismatch;
        }
        जइ (v_label_t(tgt, अङ्कः_Payload) != c_m3Type_contref)
            प्रत्यागम m3Err_typeMismatch;

#जइ d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->अङ्कः_Rets + अङ्कः_Payload]
            : tgt->type->types[अङ्कः_Payload];

        जइ (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            प्रत्यागम m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = रिक्त;
        प्रति (u32 idx = 0; idx < v->module->अङ्कः_Func_Types; ++idx) {
            जइ (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                भङ्ग;
            }
        }
        जइ (!labelCt) प्रत्यागम m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_अङ्कः_Params = tagType ? tagType->अङ्कः_Rets : 0;
        जइ (labelFt->अङ्कः_Args != expected_अङ्कः_Params) प्रत्यागम m3Err_typeMismatch;
        प्रति (u16 p = 0; p < expected_अङ्कः_Params; ++p) {
            जइ (labelFt->types[labelFt->अङ्कः_Rets + p] != tagType->types[p])
                प्रत्यागम m3Err_typeMismatch;
        }

        u16 expected_अङ्कः_Rets = ft ? ft->अङ्कः_Rets : 0;
        जइ (labelFt->अङ्कः_Rets != expected_अङ्कः_Rets) प्रत्यागम m3Err_typeMismatch;
        प्रति (u16 r_idx = 0; r_idx < expected_अङ्कः_Rets; ++r_idx) {
            जइ (labelFt->types[r_idx] != ft->types[r_idx])
                प्रत्यागम m3Err_typeMismatch;
        }
#endif
    } अण्णहा {
        जइ (tagType && tagType->अङ्कः_Args != 0) प्रत्यागम m3Err_typeMismatch;
    }
    प्रत्यागम m3Err_none;
}

#endif // d_m3HasStackSwitching

स्थिर M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    जाव (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        जइ (M3_UNLIKELY(r)) प्रत्यागम r;

        विकल्प (opcode)
        {
        // ---- Control ----
        पक्ष 0x00: // unreachable
            v_unreachable(v);
            भङ्ग;

        पक्ष 0x01: // nop
            भङ्ग;

        पक्ष 0x02: // block
        पक्ष 0x03: // loop
        पक्ष 0x04: // if
        {
            IM3FuncType bt = रिक्त;
            r = v_read_blocktype(v, &bt);
            जइ (r) प्रत्यागम r;
            जइ (opcode == 0x04) {
                r = v_निष्कासय_expect(v, c_m3Type_i32, &a);
                जइ (r) प्रत्यागम r;
            }
            // Pop block params from caller stack
            जइ (bt) {
                प्रति (u16 i = bt->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(bt->types[bt->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
            }
            r = v_प्रक्षिप_ctrl(v, opcode, bt);
            जइ (r) प्रत्यागम r;
            // Push params inside block
            जइ (bt) {
                प्रति (u16 i = 0; i < bt->अङ्कः_Args; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(bt->types[bt->अङ्कः_Rets + i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

#जइ d_m3HasExceptionHandling
        पक्ष 0x1f: // try_table
        {
            IM3FuncType bt = रिक्त;
            r = v_read_blocktype(v, &bt);
            जइ (r) प्रत्यागम r;
            जइ (bt) {
                प्रति (u16 i = bt->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(bt->types[bt->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 अङ्कः_Catch;
            r = ReadLEB_u32(&अङ्कः_Catch, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            प्रति (u32 i = 0; i < अङ्कः_Catch; i++) {
                r = v_catch_clause(v);
                जइ (r) प्रत्यागम r;
            }

            r = v_प्रक्षिप_ctrl(v, opcode, bt);
            जइ (r) प्रत्यागम r;

            जइ (bt) {
                प्रति (u16 i = 0; i < bt->अङ्कः_Args; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(bt->types[bt->अङ्कः_Rets + i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (!v->module || tagIndex >= v->module->अङ्कः_Tags) प्रत्यागम m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            जइ (tt) {
                प्रति (u16 i = tt->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(tt->types[tt->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
            }
            v_unreachable(v);
            भङ्ग;
        }

        पक्ष 0x0a: // throw_ref
            r = v_निष्कासय_expect(v, c_m3Type_exnref, &a);
            जइ (r) प्रत्यागम r;
            v_unreachable(v);
            भङ्ग;
#endif // d_m3HasExceptionHandling

        पक्ष 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_निष्कासय_ctrl(v, &frame);
            जइ (r) प्रत्यागम r;
            जइ (frame.opcode != 0x04)
                प्रत्यागम m3Err_wasmMalformed;
            r = v_प्रक्षिप_ctrl(v, 0x05, frame.type);
            जइ (r) प्रत्यागम r;
            जइ (frame.type) {
                प्रति (u16 i = 0; i < frame.type->अङ्कः_Args; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(frame.type->types[frame.type->अङ्कः_Rets + i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_निष्कासय_ctrl(v, &frame);
            जइ (r) प्रत्यागम r;
            // Push results
            जइ (frame.type) {
                प्रति (u16 i = 0; i < frame.result_गणनम्; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(frame.type->types[i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            // If this was the outermost frame, we're done
            जइ (v->ctrl_शिखरम् == 0)
                प्रत्यागम m3Err_none;
            भङ्ग;
        }

        पक्ष 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (depth >= v->ctrl_शिखरम्) प्रत्यागम m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शिखरम् - 1 - depth];
            r = v_निष्कासय_labels(v, tgt);
            जइ (r) प्रत्यागम r;
            v_unreachable(v);
            भङ्ग;
        }

        पक्ष 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (depth >= v->ctrl_शिखरम्) प्रत्यागम m3Err_unknownLabel;
            r = v_निष्कासय_expect(v, c_m3Type_i32, &a);
            जइ (r) प्रत्यागम r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शिखरम् - 1 - depth];
            r = v_निष्कासय_labels(v, tgt);
            जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप_labels(v, tgt);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0x0e: // br_table
        {
            u32 गणनम्;
            r = ReadLEB_u32(&गणनम्, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            प्रति (u32 i = 0; i <= गणनम्; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                जइ (r) प्रत्यागम r;
                जइ (d >= v->ctrl_शिखरम्) प्रत्यागम m3Err_unknownLabel;
                जइ (i == गणनम्) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_शिखरम् - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_निष्कासय_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            जइ (r) प्रत्यागम r;

            प्रति (u32 i = 0; i <= गणनम्; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                जइ (r) प्रत्यागम r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_शिखरम् - 1 - d];
                u16 n = v_label_n(t);
                जइ (n != arity) प्रत्यागम m3_Err_type_गणनम्_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                प्रति (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    जइ (want != c_valBottom && have != c_valBottom && want != have)
                        प्रत्यागम m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_शिखरम् - 1 - defDepth];
            r = v_निष्कासय_labels(v, dt);
            जइ (r) प्रत्यागम r;
            v_unreachable(v);
            भङ्ग;
        }

        पक्ष 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            जइ (ft) {
                प्रति (u16 i = ft->अङ्कः_Rets; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
            }
            v_unreachable(v);
            भङ्ग;
        }

        // ---- Call ----
        पक्ष 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (idx >= v->module->अङ्कः_Functions) प्रत्यागम m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            जइ (ft) {
                प्रति (u16 i = ft->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[ft->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
                प्रति (u16 i = 0; i < ft->अङ्कः_Rets; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(ft->types[i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            // the table must exist and hold funcrefs
            जइ (tableIdx >= v->module->अङ्कः_Tables) प्रत्यागम m3Err_unknownTable;
            जइ (v->module->tables[tableIdx]->type != c_m3Type_funcref) प्रत्यागम m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_निष्कासय_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            जइ (r) प्रत्यागम r;
            जइ (ft) {
                प्रति (u16 i = ft->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[ft->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
                प्रति (u16 i = 0; i < ft->अङ्कः_Rets; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(ft->types[i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (idx >= v->module->अङ्कः_Functions) प्रत्यागम m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            जइ (r) प्रत्यागम r;
            जइ (ft) {
                प्रति (u16 i = ft->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[ft->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
            }
            v_unreachable(v);
            भङ्ग;
        }

        पक्ष 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            // the table must exist and hold funcrefs
            जइ (tableIdx >= v->module->अङ्कः_Tables) प्रत्यागम m3Err_unknownTable;
            जइ (v->module->tables[tableIdx]->type != c_m3Type_funcref) प्रत्यागम m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            जइ (r) प्रत्यागम r;
            r = v_निष्कासय_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            जइ (r) प्रत्यागम r;
            जइ (ft) {
                प्रति (u16 i = ft->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[ft->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
            }
            v_unreachable(v);
            भङ्ग;
        }

#जइ d_m3HasTypedRefs
        पक्ष 0x14: // call_ref
        पक्ष 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            जइ (isTail) {
                r = v_check_tail_results(v, ft);
                जइ (r) प्रत्यागम r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_निष्कासय_expect(v, c_m3Type_funcref, &a);
            जइ (r) प्रत्यागम r;
            जइ (ft) {
                प्रति (u16 i = ft->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[ft->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
                जइ (not isTail) {
                    प्रति (u16 i = 0; i < ft->अङ्कः_Rets; i++) {
                        r = v_प्रक्षिप(v, BaseTypeOf(ft->types[i]));
                        जइ (r) प्रत्यागम r;
                    }
                }
            }
            जइ (isTail) v_unreachable(v);
            भङ्ग;
        }

        पक्ष 0xd4: // ref.as_non_null
        {
            r = v_निष्कासय_expect(v, c_m3Type_funcref, &a);
            जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        पक्ष 0x1a: // drop
            r = v_निष्कासय(v, &a);
            जइ (r) प्रत्यागम r;
            भङ्ग;

        पक्ष 0x1b: // select
        {
            r = v_निष्कासय_expect(v, c_m3Type_i32, &a);
            जइ (r) प्रत्यागम r;
            m3type_t t2 = c_valBottom;
            r = v_निष्कासय(v, &t2);
            जइ (r) प्रत्यागम r;
            m3type_t t1;
            r = v_निष्कासय_expect(v, t2, &t1);
            जइ (r) प्रत्यागम r;
            // untyped select is numeric only; references need the 0x1c form
            जइ (t2 != c_valBottom && IsRefType(t2)) प्रत्यागम m3Err_typeMismatch;
            r = v_प्रक्षिप(v, (t2 == c_valBottom) ? t1 : t2);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

#जइ d_m3HasRefTypes
        पक्ष 0x1c: // select with an explicit result type
        {
            u32 अङ्कः_Types;
            r = ReadLEB_u32(&अङ्कः_Types, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (अङ्कः_Types != 1) प्रत्यागम m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_मानम्_Type(v->module, &selType, &v->wasm, v->wasmEnd);  जइ (r) प्रत्यागम r;
            u8 t = BaseTypeOf(selType);

            r = v_निष्कासय_expect(v, c_m3Type_i32, &a); जइ (r) प्रत्यागम r;
            r = v_निष्कासय_expect(v, t, &a);            जइ (r) प्रत्यागम r;
            r = v_निष्कासय_expect(v, t, &a);            जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप(v, t);                      जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0x25: // table.get
        पक्ष 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (tableIdx >= v->module->अङ्कः_Tables) प्रत्यागम m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            जइ (opcode == 0x26) {
                r = v_निष्कासय_expect(v, t, &a);   जइ (r) प्रत्यागम r;
                r = v_निष्कासय_expect(v, at, &a);  जइ (r) प्रत्यागम r;
            } अण्णहा {
                r = v_निष्कासय_expect(v, at, &a);  जइ (r) प्रत्यागम r;
                r = v_प्रक्षिप(v, t);             जइ (r) प्रत्यागम r;
            }
            भङ्ग;
        }

        पक्ष 0xd0: // ref.null
        {
#जइ d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  जइ (r) प्रत्यागम r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# जइ d_m3_Has_राशिः_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#अण्णहा
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   जइ (r) प्रत्यागम r;
            r = NormalizeType(&t, waType);                   जइ (r) प्रत्यागम r;
            जइ (!IsRefType(t)) प्रत्यागम m3Err_wasmMalformed;
#endif
            r = v_प्रक्षिप(v, t);                                जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_निष्कासय(v, &t);
            जइ (r) प्रत्यागम r;
            जइ (t != c_valBottom && !IsRefType(t)) प्रत्यागम m3Err_typeMismatch;
            r = v_प्रक्षिप(v, c_m3Type_i32);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (funcIdx >= v->module->अङ्कः_Functions) प्रत्यागम m3Err_unknownFunction;
            जइ (!Module_IsFunctionDeclared(v->module, funcIdx)) प्रत्यागम m3Err_undeclaredFuncRef;
            r = v_प्रक्षिप(v, c_m3Type_funcref);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }
#endif

        // ---- Variable ----
        पक्ष 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (idx >= v->अङ्कः_Locals) प्रत्यागम m3Err_unknownLocal;
            r = v_प्रक्षिप(v, v->localTypes[idx]);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (idx >= v->अङ्कः_Locals) प्रत्यागम m3Err_unknownLocal;
            r = v_निष्कासय_expect(v, v->localTypes[idx], &a);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (idx >= v->अङ्कः_Locals) प्रत्यागम m3Err_unknownLocal;
            r = v_निष्कासय_expect(v, v->localTypes[idx], &a);
            जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप(v, v->localTypes[idx]);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (idx >= v->module->अङ्कः_Globals) प्रत्यागम m3Err_unknownGlobal;
            r = v_प्रक्षिप(v, BaseTypeOf(v->module->globals[idx].type));
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            जइ (idx >= v->module->अङ्कः_Globals) प्रत्यागम m3Err_unknownGlobal;
            r = v_निष्कासय_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        // ---- Memory load ----
        पक्ष 0x28: पक्ष 0x29: पक्ष 0x2a: पक्ष 0x2b: // i32/i64/f32/f64.load
        पक्ष 0x2c: पक्ष 0x2d: पक्ष 0x2e: पक्ष 0x2f: // i32.load8/16 s/u
        पक्ष 0x30: पक्ष 0x31: पक्ष 0x32: पक्ष 0x33: // i64.load8/16 s/u
        पक्ष 0x34: पक्ष 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (align > v_max_align(opcode)) प्रत्यागम m3Err_invalidAlignment;
            जइ (not v_has_memory_idx(v, memidx)) प्रत्यागम m3Err_unknownMemory;
            जइ (not v_offset_in_range(v, memidx, offset)) प्रत्यागम m3Err_wasmMalformed;
            r = v_निष्कासय_expect(v, v_memory_addrtype(v, memidx), &a); जइ (r) प्रत्यागम r;
            u8 result;
            जइ      (opcode == 0x28) result = c_m3Type_i32;
            अण्णहा जइ (opcode == 0x29) result = c_m3Type_i64;
            अण्णहा जइ (opcode == 0x2a) result = c_m3Type_f32;
            अण्णहा जइ (opcode == 0x2b) result = c_m3Type_f64;
            अण्णहा जइ (opcode <= 0x2f) result = c_m3Type_i32;
            अण्णहा                     result = c_m3Type_i64;
            r = v_प्रक्षिप(v, result);
            जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        // ---- Memory store ----
        पक्ष 0x36: पक्ष 0x37: पक्ष 0x38: पक्ष 0x39: // i32/i64/f32/f64.store
        पक्ष 0x3a: पक्ष 0x3b:                         // i32.store8/16
        पक्ष 0x3c: पक्ष 0x3d: पक्ष 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (align > v_max_align(opcode)) प्रत्यागम m3Err_invalidAlignment;
            जइ (not v_has_memory_idx(v, memidx)) प्रत्यागम m3Err_unknownMemory;
            जइ (not v_offset_in_range(v, memidx, offset)) प्रत्यागम m3Err_wasmMalformed;
            u8 valtype;
            जइ      (opcode == 0x36) valtype = c_m3Type_i32;
            अण्णहा जइ (opcode == 0x37) valtype = c_m3Type_i64;
            अण्णहा जइ (opcode == 0x38) valtype = c_m3Type_f32;
            अण्णहा जइ (opcode == 0x39) valtype = c_m3Type_f64;
            अण्णहा जइ (opcode <= 0x3b) valtype = c_m3Type_i32;
            अण्णहा                     valtype = c_m3Type_i64;
            r = v_निष्कासय_expect(v, valtype, &a); जइ (r) प्रत्यागम r;
            r = v_निष्कासय_expect(v, v_memory_addrtype(v, memidx), &a); जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        // ---- Memory size/grow ----
        पक्ष 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (not v_has_memory_idx(v, memidx)) प्रत्यागम m3Err_unknownMemory;
            r = v_प्रक्षिप(v, v_memory_addrtype(v, memidx)); जइ (r) प्रत्यागम r;
            भङ्ग;
        }
        पक्ष 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (not v_has_memory_idx(v, memidx)) प्रत्यागम m3Err_unknownMemory;
            r = v_निष्कासय_expect(v, v_memory_addrtype(v, memidx), &a); जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप(v, v_memory_addrtype(v, memidx)); जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        // ---- Constants ----
        पक्ष 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप(v, c_m3Type_i32); जइ (r) प्रत्यागम r;
            भङ्ग;
        }
        पक्ष 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप(v, c_m3Type_i64); जइ (r) प्रत्यागम r;
            भङ्ग;
        }
        पक्ष 0x43: { // f32.const
            जइ (v->wasm + 4 > v->wasmEnd) प्रत्यागम m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_प्रक्षिप(v, c_m3Type_f32); जइ (r) प्रत्यागम r;
            भङ्ग;
        }
        पक्ष 0x44: { // f64.const
            जइ (v->wasm + 8 > v->wasmEnd) प्रत्यागम m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_प्रक्षिप(v, c_m3Type_f64); जइ (r) प्रत्यागम r;
            भङ्ग;
        }


        // ---- i32 comparison ----
        पक्ष 0x45: r = v_testop(v, c_m3Type_i32); भङ्ग; // i32.eqz
        पक्ष 0x46: पक्ष 0x47: पक्ष 0x48: पक्ष 0x49: पक्ष 0x4a:
        पक्ष 0x4b: पक्ष 0x4c: पक्ष 0x4d: पक्ष 0x4e: पक्ष 0x4f:
            r = v_relop(v, c_m3Type_i32); भङ्ग;

        // ---- i64 comparison ----
        पक्ष 0x50: r = v_testop(v, c_m3Type_i64); भङ्ग; // i64.eqz
        पक्ष 0x51: पक्ष 0x52: पक्ष 0x53: पक्ष 0x54: पक्ष 0x55:
        पक्ष 0x56: पक्ष 0x57: पक्ष 0x58: पक्ष 0x59: पक्ष 0x5a:
            r = v_relop(v, c_m3Type_i64); भङ्ग;

        // ---- f32 comparison ----
        पक्ष 0x5b: पक्ष 0x5c: पक्ष 0x5d: पक्ष 0x5e: पक्ष 0x5f: पक्ष 0x60:
            r = v_relop(v, c_m3Type_f32); भङ्ग;

        // ---- f64 comparison ----
        पक्ष 0x61: पक्ष 0x62: पक्ष 0x63: पक्ष 0x64: पक्ष 0x65: पक्ष 0x66:
            r = v_relop(v, c_m3Type_f64); भङ्ग;

        // ---- i32 unary ----
        पक्ष 0x67: पक्ष 0x68: पक्ष 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); भङ्ग;

        // ---- i32 binary ----
        पक्ष 0x6a: पक्ष 0x6b: पक्ष 0x6c: पक्ष 0x6d: पक्ष 0x6e: पक्ष 0x6f:
        पक्ष 0x70: पक्ष 0x71: पक्ष 0x72: पक्ष 0x73: पक्ष 0x74: पक्ष 0x75:
        पक्ष 0x76: पक्ष 0x77: पक्ष 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); भङ्ग;

        // ---- i64 unary ----
        पक्ष 0x79: पक्ष 0x7a: पक्ष 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); भङ्ग;

        // ---- i64 binary ----
        पक्ष 0x7c: पक्ष 0x7d: पक्ष 0x7e: पक्ष 0x7f: पक्ष 0x80: पक्ष 0x81:
        पक्ष 0x82: पक्ष 0x83: पक्ष 0x84: पक्ष 0x85: पक्ष 0x86: पक्ष 0x87:
        पक्ष 0x88: पक्ष 0x89: पक्ष 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); भङ्ग;

        // ---- f32 unary ----
        पक्ष 0x8b: पक्ष 0x8c: पक्ष 0x8d: पक्ष 0x8e: पक्ष 0x8f:
        पक्ष 0x90: पक्ष 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); भङ्ग;

        // ---- f32 binary ----
        पक्ष 0x92: पक्ष 0x93: पक्ष 0x94: पक्ष 0x95: पक्ष 0x96:
        पक्ष 0x97: पक्ष 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); भङ्ग;

        // ---- f64 unary ----
        पक्ष 0x99: पक्ष 0x9a: पक्ष 0x9b: पक्ष 0x9c: पक्ष 0x9d:
        पक्ष 0x9e: पक्ष 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); भङ्ग;

        // ---- f64 binary ----
        पक्ष 0xa0: पक्ष 0xa1: पक्ष 0xa2: पक्ष 0xa3: पक्ष 0xa4:
        पक्ष 0xa5: पक्ष 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); भङ्ग;

        // ---- Conversions ----
        पक्ष 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); भङ्ग; // i32.wrap/i64
        पक्ष 0xa8: पक्ष 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); भङ्ग;
        पक्ष 0xaa: पक्ष 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); भङ्ग;
        पक्ष 0xac: पक्ष 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); भङ्ग;
        पक्ष 0xae: पक्ष 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); भङ्ग;
        पक्ष 0xb0: पक्ष 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); भङ्ग;
        पक्ष 0xb2: पक्ष 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); भङ्ग;
        पक्ष 0xb4: पक्ष 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); भङ्ग;
        पक्ष 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); भङ्ग;
        पक्ष 0xb7: पक्ष 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); भङ्ग;
        पक्ष 0xb9: पक्ष 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); भङ्ग;
        पक्ष 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); भङ्ग;
        पक्ष 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); भङ्ग;
        पक्ष 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); भङ्ग;
        पक्ष 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); भङ्ग;
        पक्ष 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); भङ्ग;

        // ---- Sign-extension (MVP post) ----
        पक्ष 0xc0: पक्ष 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); भङ्ग;
        पक्ष 0xc2: पक्ष 0xc3: पक्ष 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); भङ्ग;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        पक्ष 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;
            विकल्प (sub) {
            पक्ष 0x00: पक्ष 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); भङ्ग;
            पक्ष 0x02: पक्ष 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); भङ्ग;
            पक्ष 0x04: पक्ष 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); भङ्ग;
            पक्ष 0x06: पक्ष 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); भङ्ग;
            पक्ष 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                जइ (not v_has_memory_idx(v, memidx)) प्रत्यागम m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                जइ (not v->module->has_Data_गणनम्) प्रत्यागम m3_Err_data_गणनम्_Required;
                जइ (dataidx >= v->module->अङ्कः_Data_Segments) प्रत्यागम m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_निष्कासय_expect(v, c_m3Type_i32, &a); जइ (r) प्रत्यागम r; // n
                r = v_निष्कासय_expect(v, c_m3Type_i32, &a); जइ (r) प्रत्यागम r; // src
                r = v_निष्कासय_expect(v, v_memory_addrtype(v, memidx), &a); जइ (r) प्रत्यागम r; // dst
                भङ्ग;
            }
            पक्ष 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                // needs the segment, but not a memory
                जइ (not v->module->has_Data_गणनम्) प्रत्यागम m3_Err_data_गणनम्_Required;
                जइ (dataidx >= v->module->अङ्कः_Data_Segments) प्रत्यागम m3Err_unknownDataSegment;
                भङ्ग;
            }
#जइ d_m3HasRefTypes
            पक्ष 0x0c: // table.init
            पक्ष 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;

                जइ (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    जइ (a2 >= v->module->अङ्कः_Tables) प्रत्यागम m3Err_unknownTable;
                    जइ (a1 >= v->module->अङ्कः_Element_Segments) प्रत्यागम m3Err_unknownElemSegment;
                    जइ (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        प्रत्यागम m3Err_typeMismatch;
                } अण्णहा {                                                    // dst, src
                    जइ (a1 >= v->module->अङ्कः_Tables) प्रत्यागम m3Err_unknownTable;
                    जइ (a2 >= v->module->अङ्कः_Tables) प्रत्यागम m3Err_unknownTable;
                    जइ (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        प्रत्यागम m3Err_typeMismatch;
                }

                जइ (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_निष्कासय_expect(v, c_m3Type_i32, &a); जइ (r) प्रत्यागम r;             // n
                    r = v_निष्कासय_expect(v, c_m3Type_i32, &a); जइ (r) प्रत्यागम r;             // s
                    r = v_निष्कासय_expect(v, v_table_addrtype(v, a2), &a); जइ (r) प्रत्यागम r;  // d
                } अण्णहा {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_निष्कासय_expect(v, nType, &a); जइ (r) प्रत्यागम r;  // n
                    r = v_निष्कासय_expect(v, sType, &a); जइ (r) प्रत्यागम r;  // s
                    r = v_निष्कासय_expect(v, dType, &a); जइ (r) प्रत्यागम r;  // d
                }
                भङ्ग;
            }
            पक्ष 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                जइ (elemIdx >= v->module->अङ्कः_Element_Segments) प्रत्यागम m3Err_unknownElemSegment;
                भङ्ग;
            }
            पक्ष 0x0f: // table.grow
            पक्ष 0x10: // table.size
            पक्ष 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                जइ (tableIdx >= v->module->अङ्कः_Tables) प्रत्यागम m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                जइ (sub == 0x10) {                                          // table.size
                    r = v_प्रक्षिप(v, at); जइ (r) प्रत्यागम r;
                } अण्णहा जइ (sub == 0x0f) {                                   // table.grow
                    r = v_निष्कासय_expect(v, at, &a); जइ (r) प्रत्यागम r;  // n
                    r = v_निष्कासय_expect(v, t, &a);  जइ (r) प्रत्यागम r;  // init
                    r = v_प्रक्षिप(v, at);           जइ (r) प्रत्यागम r;
                } अण्णहा {                                                    // table.fill
                    r = v_निष्कासय_expect(v, at, &a); जइ (r) प्रत्यागम r;  // n
                    r = v_निष्कासय_expect(v, t, &a);  जइ (r) प्रत्यागम r;  // val
                    r = v_निष्कासय_expect(v, at, &a); जइ (r) प्रत्यागम r;  // i
                }
                भङ्ग;
            }
#endif
            पक्ष 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                जइ (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) प्रत्यागम m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_निष्कासय_expect(v, lenType, &a); जइ (r) प्रत्यागम r; // n
                r = v_निष्कासय_expect(v, srcType, &a); जइ (r) प्रत्यागम r; // src
                r = v_निष्कासय_expect(v, dstType, &a); जइ (r) प्रत्यागम r; // dst
                भङ्ग;
            }
            पक्ष 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                जइ (not v_has_memory_idx(v, memidx)) प्रत्यागम m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_निष्कासय_expect(v, addrType, &a); जइ (r) प्रत्यागम r;      // n
                r = v_निष्कासय_expect(v, c_m3Type_i32, &a); जइ (r) प्रत्यागम r;  // val
                r = v_निष्कासय_expect(v, addrType, &a); जइ (r) प्रत्यागम r;      // dst
                भङ्ग;
            }
#जइ d_m3HasWideArithmetic
            पक्ष 0x13: पक्ष 0x14: // i64.add128, i64.sub128
            पक्ष 0x15: पक्ष 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 अङ्कः_Operands = (sub <= 0x14) ? 4 : 2;
                प्रति (u32 i = 0; i < अङ्कः_Operands; ++i) {
                    r = v_निष्कासय_expect(v, c_m3Type_i64, &a); जइ (r) प्रत्यागम r;
                }
                r = v_प्रक्षिप(v, c_m3Type_i64); जइ (r) प्रत्यागम r;
                r = v_प्रक्षिप(v, c_m3Type_i64); जइ (r) प्रत्यागम r;
                भङ्ग;
            }
#endif
            स्वभाव:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                भङ्ग;
            }
            भङ्ग;
        }

#जइ d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        पक्ष 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            जइ (r) प्रत्यागम r;

            जइ (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                जइ (reserved != 0) प्रत्यागम m3Err_wasmMalformed;
                भङ्ग;
            }

            u32 width, अङ्कः_Operands; m3type_t मानम्_Type, resultType;
            जइ (not v_atomic_shape(sub, &width, &अङ्कः_Operands, &मानम्_Type, &resultType)) प्रत्यागम m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            जइ ((1u << align) != width) प्रत्यागम m3Err_invalidAtomicAlignment;
            जइ (not v_has_memory_idx(v, memidx)) प्रत्यागम m3Err_unknownMemory;
            जइ (not v_offset_in_range(v, memidx, offset)) प्रत्यागम m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            जइ (sub == 0x01 or sub == 0x02) {
                r = v_निष्कासय_expect(v, c_m3Type_i64, &a); जइ (r) प्रत्यागम r;
                अङ्कः_Operands--;
            }
            प्रति (u32 i = 1; i < अङ्कः_Operands; ++i) {
                r = v_निष्कासय_expect(v, मानम्_Type, &a); जइ (r) प्रत्यागम r;
            }
            r = v_निष्कासय_expect(v, v_memory_addrtype(v, memidx), &a); जइ (r) प्रत्यागम r;
            जइ (resultType != c_m3Type_none) {
                r = v_प्रक्षिप(v, resultType); जइ (r) प्रत्यागम r;
            }
            भङ्ग;
        }
#endif

#जइ d_m3_Has_राशिः_Switching
        // ---- Stack Switching ----
        पक्ष 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            जइ (!ft || !ft->isContinuation) प्रत्यागम m3Err_typeMismatch;
            r = v_निष्कासय_expect(v, c_m3Type_funcref, &a); जइ (r) प्रत्यागम r;
            r = v_प्रक्षिप(v, c_m3Type_contref); जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (typeIdx1 >= v->module->अङ्कः_Func_Types || typeIdx2 >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            जइ (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) प्रत्यागम m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 अङ्कः_Params1 = ft1->अङ्कः_Args;
            u16 अङ्कः_Params2 = ft2->अङ्कः_Args;
            जइ (अङ्कः_Params1 < अङ्कः_Params2) प्रत्यागम m3Err_typeMismatch;
            u32 अङ्कः_Bound = अङ्कः_Params1 - अङ्कः_Params2;

            जइ (ft1->अङ्कः_Rets != ft2->अङ्कः_Rets) प्रत्यागम m3Err_typeMismatch;
            प्रति (u16 i = 0; i < ft1->अङ्कः_Rets; ++i) {
                जइ (ft1->types[i] != ft2->types[i]) प्रत्यागम m3Err_typeMismatch;
            }
            प्रति (u16 i = 0; i < अङ्कः_Params2; ++i) {
                जइ (ft1->types[ft1->अङ्कः_Rets + अङ्कः_Bound + i] != ft2->types[ft2->अङ्कः_Rets + i]) प्रत्यागम m3Err_typeMismatch;
            }

            r = v_निष्कासय_expect(v, c_m3Type_contref, &a); जइ (r) प्रत्यागम r;

            प्रति (u32 i = अङ्कः_Bound; i > 0; i--) {
                r = v_निष्कासय_expect(v, BaseTypeOf(ft1->types[ft1->अङ्कः_Rets + i - 1]), &a);
                जइ (r) प्रत्यागम r;
            }

            r = v_प्रक्षिप(v, c_m3Type_contref); जइ (r) प्रत्यागम r;
            भङ्ग;
        }

        पक्ष 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (!v->module || tagIdx >= v->module->अङ्कः_Tags) प्रत्यागम m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            जइ (tt) {
                प्रति (u16 i = tt->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(tt->types[tt->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
                प्रति (u16 i = 0; i < tt->अङ्कः_Rets; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(tt->types[i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            जइ (!ct || !ct->isContinuation) प्रत्यागम m3Err_typeMismatch;

            u32 अङ्कः_Clauses;
            r = ReadLEB_u32(&अङ्कः_Clauses, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;

            प्रति (u32 i = 0; i < अङ्कः_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                r = v_resume_clause(v, kind, tagIdx, ct); जइ (r) प्रत्यागम r;
            }

            r = v_निष्कासय_expect(v, c_m3Type_contref, &a); जइ (r) प्रत्यागम r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            जइ (ft) {
                प्रति (u16 i = ft->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[ft->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
                प्रति (u16 i = 0; i < ft->अङ्कः_Rets; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(ft->types[i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, अङ्कः_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            r = ReadLEB_u32(&अङ्कः_Clauses, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;

            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            जइ (!v->module || tagIdx >= v->module->अङ्कः_Tags) प्रत्यागम m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            जइ (!ct || !ct->isContinuation) प्रत्यागम m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            जइ (tagType && tagType->अङ्कः_Rets != 0) प्रत्यागम m3Err_typeMismatch;

            प्रति (u32 i = 0; i < अङ्कः_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                r = v_resume_clause(v, kind, cTagIdx, ct); जइ (r) प्रत्यागम r;
            }

            r = v_निष्कासय_expect(v, c_m3Type_contref, &a); जइ (r) प्रत्यागम r;

            जइ (tagType) {
                प्रति (u16 i = tagType->अङ्कः_Args; i > 0; i--) {
                    r = v_निष्कासय_expect(v, BaseTypeOf(tagType->types[tagType->अङ्कः_Rets + i - 1]), &a);
                    जइ (r) प्रत्यागम r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            जइ (ft) {
                प्रति (u16 i = 0; i < ft->अङ्कः_Rets; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(ft->types[i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0xe5: // resume_throw_ref
        {
            u32 typeIdx, अङ्कः_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            r = ReadLEB_u32(&अङ्कः_Clauses, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;

            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            जइ (!ct || !ct->isContinuation) प्रत्यागम m3Err_typeMismatch;

            प्रति (u32 i = 0; i < अङ्कः_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
                r = v_resume_clause(v, kind, cTagIdx, ct); जइ (r) प्रत्यागम r;
            }

            r = v_निष्कासय_expect(v, c_m3Type_contref, &a); जइ (r) प्रत्यागम r;
            r = v_निष्कासय_expect(v, c_m3Type_exnref, &a); जइ (r) प्रत्यागम r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            जइ (ft) {
                प्रति (u16 i = 0; i < ft->अङ्कः_Rets; i++) {
                    r = v_प्रक्षिप(v, BaseTypeOf(ft->types[i]));
                    जइ (r) प्रत्यागम r;
                }
            }
            भङ्ग;
        }

        पक्ष 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); जइ (r) प्रत्यागम r;

            जइ (typeIdx >= v->module->अङ्कः_Func_Types) प्रत्यागम m3Err_unknownType;
            जइ (!v->module || tagIdx >= v->module->अङ्कः_Tags) प्रत्यागम m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            जइ (!ct || !ct->isContinuation) प्रत्यागम m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            जइ (tagType && tagType->अङ्कः_Args != 0) प्रत्यागम m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            जइ (!ft || ft->अङ्कः_Args == 0) प्रत्यागम m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->अङ्कः_Rets + ft->अङ्कः_Args - 1]);
            जइ (!peer || !peer->contFuncType) प्रत्यागम m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_निष्कासय_expect(v, c_m3Type_contref, &a); जइ (r) प्रत्यागम r;

            प्रति (u16 i = (u16)(ft->अङ्कः_Args - 1); i > 0; i--) {
                r = v_निष्कासय_expect(v, BaseTypeOf(ft->types[ft->अङ्कः_Rets + i - 1]), &a);
                जइ (r) प्रत्यागम r;
            }

            प्रति (u16 i = 0; i < self->अङ्कः_Args; i++) {
                r = v_प्रक्षिप(v, BaseTypeOf(self->types[self->अङ्कः_Rets + i]));
                जइ (r) प्रत्यागम r;
            }
            भङ्ग;
        }

#endif // d_m3HasStackSwitching

        स्वभाव:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            भङ्ग;

        } // switch

        जइ (r) प्रत्यागम r;

    } // while

    // If we ran out of bytes without hitting the final end
    प्रत्यागम m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    जइ (!i_function->wasm) प्रत्यागम m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    जइ (!v) {
        v = m3_AllocStruct (ValCtx);
        जइ (!v) प्रत्यागम m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, परिमाण(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    जइ (r) प्रत्यागम r;

    // Parse locals
    u32 अङ्कः_Local_Blocks;
    r = ReadLEB_u32(&अङ्कः_Local_Blocks, &v->wasm, v->wasmEnd);
    जइ (r) प्रत्यागम r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 अङ्कः_Params = funcType ? funcType->अङ्कः_Args : 0;
    जइ (अङ्कः_Params > d_m3_Val_राशिः) प्रत्यागम m3_Err_function_राशिः_Overflow;
    प्रति (u16 i = 0; i < अङ्कः_Params; i++) {
        v->localTypes[v->अङ्कः_Locals++] = BaseTypeOf(funcType->types[funcType->अङ्कः_Rets + i]);
    }

    // Then: declared locals
    प्रति (u32 b = 0; b < अङ्कः_Local_Blocks; b++) {
        u32 गणनम्;
        r = ReadLEB_u32(&गणनम्, &v->wasm, v->wasmEnd);
        जइ (r) प्रत्यागम r;
        m3type_t localType;
        r = Parse_मानम्_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        जइ (r) प्रत्यागम r;
        u8 normalized = BaseTypeOf(localType);
        जइ (गणनम् > (u32) (d_m3_Val_राशिः - v->अङ्कः_Locals)) प्रत्यागम m3_Err_function_राशिः_Overflow;
        प्रति (u32 c = 0; c < गणनम्; c++) {
            v->localTypes[v->अङ्कः_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_प्रक्षिप_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    जइ (r) प्रत्यागम r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    जइ (r) प्रत्यागम r;

    // After validation, control stack should be empty and all declared bytes consumed
    जइ (v->ctrl_शिखरम् != 0)
        प्रत्यागम m3Err_wasmMalformed;

    जइ (v->wasm != v->wasmEnd)
        प्रत्यागम m3Err_wasmSectionUnderrun;

    प्रत्यागम m3Err_none;
}

#अण्णहा // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (शून्य)i_function;
    प्रत्यागम m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
