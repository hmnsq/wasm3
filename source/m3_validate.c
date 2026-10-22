//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#समावेश "m3_validate.h"
#समावेश "m3_exception.h"
#समावेश "m3_info.h"

#अगर d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#घोषणा c_valBottom         0xFF

// ---------- Control frame ----------

प्रकार काष्ठा {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_गिनती;
    u16         परिणाम_गिनती;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

काष्ठा ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ढेर];
    u16         opd_शीर्ष;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_शीर्ष;

    m3type_t    localTypes [d_m3_Val_ढेर];
    u16         अंक_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
अटल bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    वापस v->module and i_memoryIdx < v->module->अंक_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
अटल m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    वापस Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
अटल bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    वापस v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
अटल m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    वापस Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
अटल u32 v_max_align (m3opcode_t opcode)
{
    चयन (opcode) {
    हाल 0x2c: हाल 0x2d:   // i32.load8_s, i32.load8_u
    हाल 0x30: हाल 0x31:   // i64.load8_s, i64.load8_u
    हाल 0x3a:              // i32.store8
    हाल 0x3c:              // i64.store8
        वापस 0;
    हाल 0x2e: हाल 0x2f:   // i32.load16_s, i32.load16_u
    हाल 0x32: हाल 0x33:   // i64.load16_s, i64.load16_u
    हाल 0x3b:              // i32.store16
    हाल 0x3d:              // i64.store16
        वापस 1;
    हाल 0x29:              // i64.load
    हाल 0x2b:              // f64.load
    हाल 0x37:              // i64.store
    हाल 0x39:              // f64.store
        वापस 3;
    शेष:                // 32-bit accesses, and a safe fallback
        वापस 2;
    }
}

#अगर d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
अटल bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_अंक_Operands, m3type_t * o_मूल्य_Type, m3type_t * o_परिणाम_Type)
{
    अटल स्थिर u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    अटल स्थिर m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    अगर (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_अंक_Operands = (sub == 0x00) ? 2 : 3;
        *o_मूल्य_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_परिणाम_Type  = c_m3Type_i32;
        वापस true;
    }

    अगर (sub < 0x10 or sub > c_waOp_lastAtomic) वापस false;

    अगर (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_अंक_Operands = 1;
    } अन्यथा अगर (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_अंक_Operands = 2;
    } अन्यथा अगर (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_अंक_Operands = 2;
    } अन्यथा {                        // cmpxchg
        variant = sub - 0x48;       *o_अंक_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_मूल्य_Type  = c_types [variant];
    *o_परिणाम_Type = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    वापस true;
}

#पूर्ण_अगर

// ---------- Operand stack ----------

अटल M3_परिणाम v_धकेलो (ValCtx * v, m3type_t type)
{
    अगर (M3_UNLIKELY(v->opd_शीर्ष >= d_m3_Val_ढेर))
        वापस m3_Err_function_ढेर_Overflow;
    v->opd[v->opd_शीर्ष++] = type;
    वापस m3Err_none;
}

अटल M3_परिणाम v_निकालो (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शीर्ष - 1];
    अगर (M3_UNLIKELY(v->opd_शीर्ष == f->height)) {
        अगर (f->is_unreachable) { *o_type = c_valBottom; वापस m3Err_none; }
        वापस m3_Err_function_ढेर_Underrun;
    }
    *o_type = v->opd[--v->opd_शीर्ष];
    वापस m3Err_none;
}

अटल M3_परिणाम v_निकालो_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3_परिणाम r = v_निकालो(v, &actual);
    अगर (M3_UNLIKELY(r)) वापस r;
    अगर (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        वापस m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    वापस m3Err_none;
}

// ---------- Control stack ----------

अटल M3_परिणाम v_धकेलो_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    अगर (M3_UNLIKELY(v->ctrl_शीर्ष >= d_m3ValCtrlDepth))
        वापस m3_Err_function_ढेर_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शीर्ष++];
    f->opcode       = op;
    f->type         = type;
    f->param_गिनती  = type ? type->अंक_Args : 0;
    f->परिणाम_गिनती = type ? type->अंक_Rets : 0;
    f->height       = v->opd_शीर्ष;
    f->is_unreachable = false;
    वापस m3Err_none;
}

अटल M3_परिणाम v_निकालो_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    अगर (M3_UNLIKELY(v->ctrl_शीर्ष == 0))
        वापस m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शीर्ष - 1];
    // pop result types
    अगर (f->type) {
        क्रम (u16 i = f->परिणाम_गिनती; i > 0; i--) {
            m3type_t a;
            M3_परिणाम r = v_निकालो_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            अगर (M3_UNLIKELY(r)) वापस r;
        }
    }
    अगर (M3_UNLIKELY(v->opd_शीर्ष != f->height))
        वापस m3_Err_type_गिनती_Mismatch;
    अगर (o_frame) *o_frame = *f;
    v->ctrl_शीर्ष--;
    वापस m3Err_none;
}

अटल व्योम v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शीर्ष - 1];
    v->opd_शीर्ष = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
अटल u16 v_label_n (ValCtrlFrame * f)
{
    वापस (f->opcode == 0x03) ? f->param_गिनती : f->परिणाम_गिनती;
}

अटल m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    अगर (!f->type) वापस c_m3Type_none;
    अगर (f->opcode == 0x03)
        वापस BaseTypeOf(f->type->types[f->type->अंक_Rets + i]); // params
    वापस BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
अटल m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_शीर्ष - 1];

    अगर (v->opd_शीर्ष <= i_depth || (u16)(v->opd_शीर्ष - i_depth - 1) < f->height)
        वापस c_valBottom;

    वापस v->opd[v->opd_शीर्ष - i_depth - 1];
}

// Pop label types for branch target
अटल M3_परिणाम v_निकालो_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    क्रम (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3_परिणाम r = v_निकालो_expect(v, v_label_t(tgt, i - 1), &a);
        अगर (M3_UNLIKELY(r)) वापस r;
    }
    वापस m3Err_none;
}

// Push label types back
अटल M3_परिणाम v_धकेलो_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    क्रम (u16 i = 0; i < n; i++) {
        M3_परिणाम r = v_धकेलो(v, v_label_t(tgt, i));
        अगर (M3_UNLIKELY(r)) वापस r;
    }
    वापस m3Err_none;
}

// ---------- Block type resolution ----------

अटल M3_परिणाम v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    अगर (v->wasm >= v->wasmEnd)
        वापस m3Err_wasmUnderrun;

#अगर d_m3HasTypedRefs
    अगर (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3_परिणाम rr = Parse_मूल्य_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        अगर (rr) वापस rr;
        IM3FuncType ftype = शून्य;
        rr = AllocFuncType(&ftype, 1);
        अगर (rr) वापस rr;
        ftype->अंक_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        अगर (rr) वापस rr;
        *o_type = ftype;
        वापस m3Err_none;
    }
#पूर्ण_अगर

    i64 type;
    M3_परिणाम r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    अगर (r) वापस r;

    अगर (type < 0) {
        अगर (type < -64) वापस m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        अगर (r) वापस r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } अन्यथा {
        अगर ((u32)type >= v->module->अंक_Func_Types)
            वापस m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    वापस m3Err_none;
}

// ---------- Convenience ----------

अटल M3_परिणाम v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3_परिणाम r = v_निकालो_expect(v, in, &a);
    अगर (M3_UNLIKELY(r)) वापस r;
    वापस v_धकेलो(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
अटल M3_परिणाम v_check_पुच्छ_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : शून्य;

    u16 अंक_Callee_Rets = i_calleeType ? i_calleeType->अंक_Rets : 0;
    u16 अंक_Func_Rets   = ft ? ft->अंक_Rets : 0;

    अगर (अंक_Callee_Rets != अंक_Func_Rets)
        वापस m3Err_typeMismatch;

    क्रम (u16 i = 0; i < अंक_Callee_Rets; i++) {
        अगर (i_calleeType->types[i] != ft->types[i])
            वापस m3Err_typeMismatch;
    }

    वापस m3Err_none;
}

अटल M3_परिणाम v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_परिणाम r;
    r = v_निकालो_expect(v, t, &a); अगर (M3_UNLIKELY(r)) वापस r;
    r = v_निकालो_expect(v, t, &a); अगर (M3_UNLIKELY(r)) वापस r;
    वापस v_धकेलो(v, t);
}

अटल M3_परिणाम v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_परिणाम r;
    r = v_निकालो_expect(v, t, &a); अगर (M3_UNLIKELY(r)) वापस r;
    r = v_निकालो_expect(v, t, &a); अगर (M3_UNLIKELY(r)) वापस r;
    वापस v_धकेलो(v, c_m3Type_i32);
}

अटल M3_परिणाम v_testop (ValCtx * v, m3type_t t)
{
    वापस v_unop(v, t, c_m3Type_i32);
}

अटल M3_परिणाम v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    वापस v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#अगर d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
अटल M3_परिणाम v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3_परिणाम r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    अगर (r) वापस r;
    अगर (kind > 0x03) वापस m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = शून्य;
    अगर (hasTag) {
        u32 tag_सूचकांक;
        r = ReadLEB_u32(&tag_सूचकांक, &v->wasm, v->wasmEnd);
        अगर (r) वापस r;
        अगर (!v->module || tag_सूचकांक >= v->module->अंक_Tags) वापस m3Err_unknownTag;
        tagType = v->module->tags[tag_सूचकांक].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    अगर (r) वापस r;
    अगर (depth >= v->ctrl_शीर्ष) वापस m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शीर्ष - 1 - depth];

    u16 अंक_Payload = tagType ? tagType->अंक_Args : 0;
    u16 अंक_Label   = v_label_n(tgt);

    अगर (अंक_Label != अंक_Payload + (hasRef ? 1u : 0u))
        वापस m3_Err_type_गिनती_Mismatch;

    क्रम (u16 i = 0; i < अंक_Payload; i++) {
        अगर (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->अंक_Rets + i]))
            वापस m3Err_typeMismatch;
    }

    अगर (hasRef and v_label_t(tgt, अंक_Payload) != c_m3Type_exnref)
        वापस m3Err_typeMismatch;

    वापस m3Err_none;
}

#पूर्ण_अगर // d_m3HasExceptionHandling

#अगर d_m3_Has_ढेर_Switching

अटल M3_परिणाम v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    अगर (kind > 0x01) वापस m3Err_wasmMalformed;
    अगर (!v->module || tagIdx >= v->module->अंक_Tags) वापस m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    अगर (kind == 0x00) {
        u32 depth;
        M3_परिणाम r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); अगर (r) वापस r;
        अगर (depth >= v->ctrl_शीर्ष) वापस m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शीर्ष - 1 - depth];
        u16 अंक_Payload = tagType ? tagType->अंक_Args : 0;
        u16 अंक_Label   = v_label_n(tgt);

        अगर (अंक_Label != अंक_Payload + 1u) वापस m3_Err_type_गिनती_Mismatch;

        क्रम (u16 j = 0; j < अंक_Payload; j++) {
            अगर (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->अंक_Rets + j]))
                वापस m3Err_typeMismatch;
        }
        अगर (v_label_t(tgt, अंक_Payload) != c_m3Type_contref)
            वापस m3Err_typeMismatch;

#अगर d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->अंक_Rets + अंक_Payload]
            : tgt->type->types[अंक_Payload];

        अगर (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            वापस m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = शून्य;
        क्रम (u32 idx = 0; idx < v->module->अंक_Func_Types; ++idx) {
            अगर (v->module->funcTypes[idx]->canonical_सूचकांक == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                अवरोध;
            }
        }
        अगर (!labelCt) वापस m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_अंक_Params = tagType ? tagType->अंक_Rets : 0;
        अगर (labelFt->अंक_Args != expected_अंक_Params) वापस m3Err_typeMismatch;
        क्रम (u16 p = 0; p < expected_अंक_Params; ++p) {
            अगर (labelFt->types[labelFt->अंक_Rets + p] != tagType->types[p])
                वापस m3Err_typeMismatch;
        }

        u16 expected_अंक_Rets = ft ? ft->अंक_Rets : 0;
        अगर (labelFt->अंक_Rets != expected_अंक_Rets) वापस m3Err_typeMismatch;
        क्रम (u16 r_idx = 0; r_idx < expected_अंक_Rets; ++r_idx) {
            अगर (labelFt->types[r_idx] != ft->types[r_idx])
                वापस m3Err_typeMismatch;
        }
#पूर्ण_अगर
    } अन्यथा {
        अगर (tagType && tagType->अंक_Args != 0) वापस m3Err_typeMismatch;
    }
    वापस m3Err_none;
}

#पूर्ण_अगर // d_m3HasStackSwitching

अटल M3_परिणाम v_validate_body (ValCtx * v)
{
    M3_परिणाम r = m3Err_none;
    m3type_t a = c_valBottom;

    जबतक (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        अगर (M3_UNLIKELY(r)) वापस r;

        चयन (opcode)
        {
        // ---- Control ----
        हाल 0x00: // unreachable
            v_unreachable(v);
            अवरोध;

        हाल 0x01: // nop
            अवरोध;

        हाल 0x02: // block
        हाल 0x03: // loop
        हाल 0x04: // if
        {
            IM3FuncType bt = शून्य;
            r = v_read_blocktype(v, &bt);
            अगर (r) वापस r;
            अगर (opcode == 0x04) {
                r = v_निकालो_expect(v, c_m3Type_i32, &a);
                अगर (r) वापस r;
            }
            // Pop block params from caller stack
            अगर (bt) {
                क्रम (u16 i = bt->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(bt->types[bt->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
            }
            r = v_धकेलो_ctrl(v, opcode, bt);
            अगर (r) वापस r;
            // Push params inside block
            अगर (bt) {
                क्रम (u16 i = 0; i < bt->अंक_Args; i++) {
                    r = v_धकेलो(v, BaseTypeOf(bt->types[bt->अंक_Rets + i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

#अगर d_m3HasExceptionHandling
        हाल 0x1f: // try_table
        {
            IM3FuncType bt = शून्य;
            r = v_read_blocktype(v, &bt);
            अगर (r) वापस r;
            अगर (bt) {
                क्रम (u16 i = bt->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(bt->types[bt->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 अंक_Catch;
            r = ReadLEB_u32(&अंक_Catch, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            क्रम (u32 i = 0; i < अंक_Catch; i++) {
                r = v_catch_clause(v);
                अगर (r) वापस r;
            }

            r = v_धकेलो_ctrl(v, opcode, bt);
            अगर (r) वापस r;

            अगर (bt) {
                क्रम (u16 i = 0; i < bt->अंक_Args; i++) {
                    r = v_धकेलो(v, BaseTypeOf(bt->types[bt->अंक_Rets + i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0x08: // throw
        {
            u32 tag_सूचकांक;
            r = ReadLEB_u32(&tag_सूचकांक, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (!v->module || tag_सूचकांक >= v->module->अंक_Tags) वापस m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tag_सूचकांक].type;
            अगर (tt) {
                क्रम (u16 i = tt->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(tt->types[tt->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
            }
            v_unreachable(v);
            अवरोध;
        }

        हाल 0x0a: // throw_ref
            r = v_निकालो_expect(v, c_m3Type_exnref, &a);
            अगर (r) वापस r;
            v_unreachable(v);
            अवरोध;
#पूर्ण_अगर // d_m3HasExceptionHandling

        हाल 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_निकालो_ctrl(v, &frame);
            अगर (r) वापस r;
            अगर (frame.opcode != 0x04)
                वापस m3Err_wasmMalformed;
            r = v_धकेलो_ctrl(v, 0x05, frame.type);
            अगर (r) वापस r;
            अगर (frame.type) {
                क्रम (u16 i = 0; i < frame.type->अंक_Args; i++) {
                    r = v_धकेलो(v, BaseTypeOf(frame.type->types[frame.type->अंक_Rets + i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_निकालो_ctrl(v, &frame);
            अगर (r) वापस r;
            // Push results
            अगर (frame.type) {
                क्रम (u16 i = 0; i < frame.परिणाम_गिनती; i++) {
                    r = v_धकेलो(v, BaseTypeOf(frame.type->types[i]));
                    अगर (r) वापस r;
                }
            }
            // If this was the outermost frame, we're done
            अगर (v->ctrl_शीर्ष == 0)
                वापस m3Err_none;
            अवरोध;
        }

        हाल 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (depth >= v->ctrl_शीर्ष) वापस m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शीर्ष - 1 - depth];
            r = v_निकालो_labels(v, tgt);
            अगर (r) वापस r;
            v_unreachable(v);
            अवरोध;
        }

        हाल 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (depth >= v->ctrl_शीर्ष) वापस m3Err_unknownLabel;
            r = v_निकालो_expect(v, c_m3Type_i32, &a);
            अगर (r) वापस r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_शीर्ष - 1 - depth];
            r = v_निकालो_labels(v, tgt);
            अगर (r) वापस r;
            r = v_धकेलो_labels(v, tgt);
            अगर (r) वापस r;
            अवरोध;
        }

        हाल 0x0e: // br_table
        {
            u32 गिनती;
            r = ReadLEB_u32(&गिनती, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            क्रम (u32 i = 0; i <= गिनती; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                अगर (r) वापस r;
                अगर (d >= v->ctrl_शीर्ष) वापस m3Err_unknownLabel;
                अगर (i == गिनती) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_शीर्ष - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_निकालो_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            अगर (r) वापस r;

            क्रम (u32 i = 0; i <= गिनती; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                अगर (r) वापस r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_शीर्ष - 1 - d];
                u16 n = v_label_n(t);
                अगर (n != arity) वापस m3_Err_type_गिनती_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                क्रम (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    अगर (want != c_valBottom && have != c_valBottom && want != have)
                        वापस m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_शीर्ष - 1 - defDepth];
            r = v_निकालो_labels(v, dt);
            अगर (r) वापस r;
            v_unreachable(v);
            अवरोध;
        }

        हाल 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            अगर (ft) {
                क्रम (u16 i = ft->अंक_Rets; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    अगर (r) वापस r;
                }
            }
            v_unreachable(v);
            अवरोध;
        }

        // ---- Call ----
        हाल 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (idx >= v->module->अंक_Functions) वापस m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            अगर (ft) {
                क्रम (u16 i = ft->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(ft->types[ft->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
                क्रम (u16 i = 0; i < ft->अंक_Rets; i++) {
                    r = v_धकेलो(v, BaseTypeOf(ft->types[i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            // the table must exist and hold funcrefs
            अगर (tableIdx >= v->module->अंक_Tables) वापस m3Err_unknownTable;
            अगर (v->module->tables[tableIdx]->type != c_m3Type_funcref) वापस m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_निकालो_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            अगर (r) वापस r;
            अगर (ft) {
                क्रम (u16 i = ft->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(ft->types[ft->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
                क्रम (u16 i = 0; i < ft->अंक_Rets; i++) {
                    r = v_धकेलो(v, BaseTypeOf(ft->types[i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (idx >= v->module->अंक_Functions) वापस m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_पुच्छ_results(v, ft);
            अगर (r) वापस r;
            अगर (ft) {
                क्रम (u16 i = ft->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(ft->types[ft->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
            }
            v_unreachable(v);
            अवरोध;
        }

        हाल 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            // the table must exist and hold funcrefs
            अगर (tableIdx >= v->module->अंक_Tables) वापस m3Err_unknownTable;
            अगर (v->module->tables[tableIdx]->type != c_m3Type_funcref) वापस m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_पुच्छ_results(v, ft);
            अगर (r) वापस r;
            r = v_निकालो_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            अगर (r) वापस r;
            अगर (ft) {
                क्रम (u16 i = ft->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(ft->types[ft->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
            }
            v_unreachable(v);
            अवरोध;
        }

#अगर d_m3HasTypedRefs
        हाल 0x14: // call_ref
        हाल 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool is_पुच्छ = (opcode == 0x15);
            अगर (is_पुच्छ) {
                r = v_check_पुच्छ_results(v, ft);
                अगर (r) वापस r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_निकालो_expect(v, c_m3Type_funcref, &a);
            अगर (r) वापस r;
            अगर (ft) {
                क्रम (u16 i = ft->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(ft->types[ft->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
                अगर (not is_पुच्छ) {
                    क्रम (u16 i = 0; i < ft->अंक_Rets; i++) {
                        r = v_धकेलो(v, BaseTypeOf(ft->types[i]));
                        अगर (r) वापस r;
                    }
                }
            }
            अगर (is_पुच्छ) v_unreachable(v);
            अवरोध;
        }

        हाल 0xd4: // ref.as_non_null
        {
            r = v_निकालो_expect(v, c_m3Type_funcref, &a);
            अगर (r) वापस r;
            r = v_धकेलो(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            अगर (r) वापस r;
            अवरोध;
        }
#पूर्ण_अगर // d_m3HasTypedRefs

        // ---- Parametric ----
        हाल 0x1a: // drop
            r = v_निकालो(v, &a);
            अगर (r) वापस r;
            अवरोध;

        हाल 0x1b: // select
        {
            r = v_निकालो_expect(v, c_m3Type_i32, &a);
            अगर (r) वापस r;
            m3type_t t2 = c_valBottom;
            r = v_निकालो(v, &t2);
            अगर (r) वापस r;
            m3type_t t1;
            r = v_निकालो_expect(v, t2, &t1);
            अगर (r) वापस r;
            // untyped select is numeric only; references need the 0x1c form
            अगर (t2 != c_valBottom && IsRefType(t2)) वापस m3Err_typeMismatch;
            r = v_धकेलो(v, (t2 == c_valBottom) ? t1 : t2);
            अगर (r) वापस r;
            अवरोध;
        }

#अगर d_m3HasRefTypes
        हाल 0x1c: // select with an explicit result type
        {
            u32 अंक_Types;
            r = ReadLEB_u32(&अंक_Types, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (अंक_Types != 1) वापस m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_मूल्य_Type(v->module, &selType, &v->wasm, v->wasmEnd);  अगर (r) वापस r;
            u8 t = BaseTypeOf(selType);

            r = v_निकालो_expect(v, c_m3Type_i32, &a); अगर (r) वापस r;
            r = v_निकालो_expect(v, t, &a);            अगर (r) वापस r;
            r = v_निकालो_expect(v, t, &a);            अगर (r) वापस r;
            r = v_धकेलो(v, t);                      अगर (r) वापस r;
            अवरोध;
        }

        हाल 0x25: // table.get
        हाल 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (tableIdx >= v->module->अंक_Tables) वापस m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            अगर (opcode == 0x26) {
                r = v_निकालो_expect(v, t, &a);   अगर (r) वापस r;
                r = v_निकालो_expect(v, at, &a);  अगर (r) वापस r;
            } अन्यथा {
                r = v_निकालो_expect(v, at, &a);  अगर (r) वापस r;
                r = v_धकेलो(v, t);             अगर (r) वापस r;
            }
            अवरोध;
        }

        हाल 0xd0: // ref.null
        {
#अगर d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  अगर (r) वापस r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# अगर d_m3_Has_ढेर_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#अन्यथा
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   अगर (r) वापस r;
            r = NormalizeType(&t, waType);                   अगर (r) वापस r;
            अगर (!IsRefType(t)) वापस m3Err_wasmMalformed;
#पूर्ण_अगर
            r = v_धकेलो(v, t);                                अगर (r) वापस r;
            अवरोध;
        }

        हाल 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_निकालो(v, &t);
            अगर (r) वापस r;
            अगर (t != c_valBottom && !IsRefType(t)) वापस m3Err_typeMismatch;
            r = v_धकेलो(v, c_m3Type_i32);
            अगर (r) वापस r;
            अवरोध;
        }

        हाल 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (funcIdx >= v->module->अंक_Functions) वापस m3Err_unknownFunction;
            अगर (!Module_IsFunctionDeclared(v->module, funcIdx)) वापस m3Err_undeclaredFuncRef;
            r = v_धकेलो(v, c_m3Type_funcref);
            अगर (r) वापस r;
            अवरोध;
        }
#पूर्ण_अगर

        // ---- Variable ----
        हाल 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (idx >= v->अंक_Locals) वापस m3Err_unknownLocal;
            r = v_धकेलो(v, v->localTypes[idx]);
            अगर (r) वापस r;
            अवरोध;
        }

        हाल 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (idx >= v->अंक_Locals) वापस m3Err_unknownLocal;
            r = v_निकालो_expect(v, v->localTypes[idx], &a);
            अगर (r) वापस r;
            अवरोध;
        }

        हाल 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (idx >= v->अंक_Locals) वापस m3Err_unknownLocal;
            r = v_निकालो_expect(v, v->localTypes[idx], &a);
            अगर (r) वापस r;
            r = v_धकेलो(v, v->localTypes[idx]);
            अगर (r) वापस r;
            अवरोध;
        }

        हाल 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (idx >= v->module->अंक_Globals) वापस m3Err_unknownGlobal;
            r = v_धकेलो(v, BaseTypeOf(v->module->globals[idx].type));
            अगर (r) वापस r;
            अवरोध;
        }

        हाल 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            अगर (idx >= v->module->अंक_Globals) वापस m3Err_unknownGlobal;
            r = v_निकालो_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            अगर (r) वापस r;
            अवरोध;
        }

        // ---- Memory load ----
        हाल 0x28: हाल 0x29: हाल 0x2a: हाल 0x2b: // i32/i64/f32/f64.load
        हाल 0x2c: हाल 0x2d: हाल 0x2e: हाल 0x2f: // i32.load8/16 s/u
        हाल 0x30: हाल 0x31: हाल 0x32: हाल 0x33: // i64.load8/16 s/u
        हाल 0x34: हाल 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (align > v_max_align(opcode)) वापस m3Err_invalidAlignment;
            अगर (not v_has_memory_idx(v, memidx)) वापस m3Err_unknownMemory;
            अगर (not v_offset_in_range(v, memidx, offset)) वापस m3Err_wasmMalformed;
            r = v_निकालो_expect(v, v_memory_addrtype(v, memidx), &a); अगर (r) वापस r;
            u8 परिणाम;
            अगर      (opcode == 0x28) परिणाम = c_m3Type_i32;
            अन्यथा अगर (opcode == 0x29) परिणाम = c_m3Type_i64;
            अन्यथा अगर (opcode == 0x2a) परिणाम = c_m3Type_f32;
            अन्यथा अगर (opcode == 0x2b) परिणाम = c_m3Type_f64;
            अन्यथा अगर (opcode <= 0x2f) परिणाम = c_m3Type_i32;
            अन्यथा                     परिणाम = c_m3Type_i64;
            r = v_धकेलो(v, परिणाम);
            अगर (r) वापस r;
            अवरोध;
        }

        // ---- Memory store ----
        हाल 0x36: हाल 0x37: हाल 0x38: हाल 0x39: // i32/i64/f32/f64.store
        हाल 0x3a: हाल 0x3b:                         // i32.store8/16
        हाल 0x3c: हाल 0x3d: हाल 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (align > v_max_align(opcode)) वापस m3Err_invalidAlignment;
            अगर (not v_has_memory_idx(v, memidx)) वापस m3Err_unknownMemory;
            अगर (not v_offset_in_range(v, memidx, offset)) वापस m3Err_wasmMalformed;
            u8 valtype;
            अगर      (opcode == 0x36) valtype = c_m3Type_i32;
            अन्यथा अगर (opcode == 0x37) valtype = c_m3Type_i64;
            अन्यथा अगर (opcode == 0x38) valtype = c_m3Type_f32;
            अन्यथा अगर (opcode == 0x39) valtype = c_m3Type_f64;
            अन्यथा अगर (opcode <= 0x3b) valtype = c_m3Type_i32;
            अन्यथा                     valtype = c_m3Type_i64;
            r = v_निकालो_expect(v, valtype, &a); अगर (r) वापस r;
            r = v_निकालो_expect(v, v_memory_addrtype(v, memidx), &a); अगर (r) वापस r;
            अवरोध;
        }

        // ---- Memory size/grow ----
        हाल 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (not v_has_memory_idx(v, memidx)) वापस m3Err_unknownMemory;
            r = v_धकेलो(v, v_memory_addrtype(v, memidx)); अगर (r) वापस r;
            अवरोध;
        }
        हाल 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (not v_has_memory_idx(v, memidx)) वापस m3Err_unknownMemory;
            r = v_निकालो_expect(v, v_memory_addrtype(v, memidx), &a); अगर (r) वापस r;
            r = v_धकेलो(v, v_memory_addrtype(v, memidx)); अगर (r) वापस r;
            अवरोध;
        }

        // ---- Constants ----
        हाल 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            r = v_धकेलो(v, c_m3Type_i32); अगर (r) वापस r;
            अवरोध;
        }
        हाल 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            r = v_धकेलो(v, c_m3Type_i64); अगर (r) वापस r;
            अवरोध;
        }
        हाल 0x43: { // f32.const
            अगर (v->wasm + 4 > v->wasmEnd) वापस m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_धकेलो(v, c_m3Type_f32); अगर (r) वापस r;
            अवरोध;
        }
        हाल 0x44: { // f64.const
            अगर (v->wasm + 8 > v->wasmEnd) वापस m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_धकेलो(v, c_m3Type_f64); अगर (r) वापस r;
            अवरोध;
        }


        // ---- i32 comparison ----
        हाल 0x45: r = v_testop(v, c_m3Type_i32); अवरोध; // i32.eqz
        हाल 0x46: हाल 0x47: हाल 0x48: हाल 0x49: हाल 0x4a:
        हाल 0x4b: हाल 0x4c: हाल 0x4d: हाल 0x4e: हाल 0x4f:
            r = v_relop(v, c_m3Type_i32); अवरोध;

        // ---- i64 comparison ----
        हाल 0x50: r = v_testop(v, c_m3Type_i64); अवरोध; // i64.eqz
        हाल 0x51: हाल 0x52: हाल 0x53: हाल 0x54: हाल 0x55:
        हाल 0x56: हाल 0x57: हाल 0x58: हाल 0x59: हाल 0x5a:
            r = v_relop(v, c_m3Type_i64); अवरोध;

        // ---- f32 comparison ----
        हाल 0x5b: हाल 0x5c: हाल 0x5d: हाल 0x5e: हाल 0x5f: हाल 0x60:
            r = v_relop(v, c_m3Type_f32); अवरोध;

        // ---- f64 comparison ----
        हाल 0x61: हाल 0x62: हाल 0x63: हाल 0x64: हाल 0x65: हाल 0x66:
            r = v_relop(v, c_m3Type_f64); अवरोध;

        // ---- i32 unary ----
        हाल 0x67: हाल 0x68: हाल 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); अवरोध;

        // ---- i32 binary ----
        हाल 0x6a: हाल 0x6b: हाल 0x6c: हाल 0x6d: हाल 0x6e: हाल 0x6f:
        हाल 0x70: हाल 0x71: हाल 0x72: हाल 0x73: हाल 0x74: हाल 0x75:
        हाल 0x76: हाल 0x77: हाल 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); अवरोध;

        // ---- i64 unary ----
        हाल 0x79: हाल 0x7a: हाल 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); अवरोध;

        // ---- i64 binary ----
        हाल 0x7c: हाल 0x7d: हाल 0x7e: हाल 0x7f: हाल 0x80: हाल 0x81:
        हाल 0x82: हाल 0x83: हाल 0x84: हाल 0x85: हाल 0x86: हाल 0x87:
        हाल 0x88: हाल 0x89: हाल 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); अवरोध;

        // ---- f32 unary ----
        हाल 0x8b: हाल 0x8c: हाल 0x8d: हाल 0x8e: हाल 0x8f:
        हाल 0x90: हाल 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); अवरोध;

        // ---- f32 binary ----
        हाल 0x92: हाल 0x93: हाल 0x94: हाल 0x95: हाल 0x96:
        हाल 0x97: हाल 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); अवरोध;

        // ---- f64 unary ----
        हाल 0x99: हाल 0x9a: हाल 0x9b: हाल 0x9c: हाल 0x9d:
        हाल 0x9e: हाल 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); अवरोध;

        // ---- f64 binary ----
        हाल 0xa0: हाल 0xa1: हाल 0xa2: हाल 0xa3: हाल 0xa4:
        हाल 0xa5: हाल 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); अवरोध;

        // ---- Conversions ----
        हाल 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); अवरोध; // i32.wrap/i64
        हाल 0xa8: हाल 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); अवरोध;
        हाल 0xaa: हाल 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); अवरोध;
        हाल 0xac: हाल 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); अवरोध;
        हाल 0xae: हाल 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); अवरोध;
        हाल 0xb0: हाल 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); अवरोध;
        हाल 0xb2: हाल 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); अवरोध;
        हाल 0xb4: हाल 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); अवरोध;
        हाल 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); अवरोध;
        हाल 0xb7: हाल 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); अवरोध;
        हाल 0xb9: हाल 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); अवरोध;
        हाल 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); अवरोध;
        हाल 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); अवरोध;
        हाल 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); अवरोध;
        हाल 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); अवरोध;
        हाल 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); अवरोध;

        // ---- Sign-extension (MVP post) ----
        हाल 0xc0: हाल 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); अवरोध;
        हाल 0xc2: हाल 0xc3: हाल 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); अवरोध;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        हाल 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;
            चयन (sub) {
            हाल 0x00: हाल 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); अवरोध;
            हाल 0x02: हाल 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); अवरोध;
            हाल 0x04: हाल 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); अवरोध;
            हाल 0x06: हाल 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); अवरोध;
            हाल 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                अगर (not v_has_memory_idx(v, memidx)) वापस m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                अगर (not v->module->has_तथ्य_गिनती) वापस m3_Err_तथ्य_गिनती_Required;
                अगर (dataidx >= v->module->अंक_तथ्य_Segments) वापस m3_Err_unknown_तथ्य_Segment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_निकालो_expect(v, c_m3Type_i32, &a); अगर (r) वापस r; // n
                r = v_निकालो_expect(v, c_m3Type_i32, &a); अगर (r) वापस r; // src
                r = v_निकालो_expect(v, v_memory_addrtype(v, memidx), &a); अगर (r) वापस r; // dst
                अवरोध;
            }
            हाल 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                // needs the segment, but not a memory
                अगर (not v->module->has_तथ्य_गिनती) वापस m3_Err_तथ्य_गिनती_Required;
                अगर (dataidx >= v->module->अंक_तथ्य_Segments) वापस m3_Err_unknown_तथ्य_Segment;
                अवरोध;
            }
#अगर d_m3HasRefTypes
            हाल 0x0c: // table.init
            हाल 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); अगर (r) वापस r;

                अगर (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    अगर (a2 >= v->module->अंक_Tables) वापस m3Err_unknownTable;
                    अगर (a1 >= v->module->अंक_Element_Segments) वापस m3Err_unknownElemSegment;
                    अगर (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        वापस m3Err_typeMismatch;
                } अन्यथा {                                                    // dst, src
                    अगर (a1 >= v->module->अंक_Tables) वापस m3Err_unknownTable;
                    अगर (a2 >= v->module->अंक_Tables) वापस m3Err_unknownTable;
                    अगर (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        वापस m3Err_typeMismatch;
                }

                अगर (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_निकालो_expect(v, c_m3Type_i32, &a); अगर (r) वापस r;             // n
                    r = v_निकालो_expect(v, c_m3Type_i32, &a); अगर (r) वापस r;             // s
                    r = v_निकालो_expect(v, v_table_addrtype(v, a2), &a); अगर (r) वापस r;  // d
                } अन्यथा {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_निकालो_expect(v, nType, &a); अगर (r) वापस r;  // n
                    r = v_निकालो_expect(v, sType, &a); अगर (r) वापस r;  // s
                    r = v_निकालो_expect(v, dType, &a); अगर (r) वापस r;  // d
                }
                अवरोध;
            }
            हाल 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                अगर (elemIdx >= v->module->अंक_Element_Segments) वापस m3Err_unknownElemSegment;
                अवरोध;
            }
            हाल 0x0f: // table.grow
            हाल 0x10: // table.size
            हाल 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                अगर (tableIdx >= v->module->अंक_Tables) वापस m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                अगर (sub == 0x10) {                                          // table.size
                    r = v_धकेलो(v, at); अगर (r) वापस r;
                } अन्यथा अगर (sub == 0x0f) {                                   // table.grow
                    r = v_निकालो_expect(v, at, &a); अगर (r) वापस r;  // n
                    r = v_निकालो_expect(v, t, &a);  अगर (r) वापस r;  // init
                    r = v_धकेलो(v, at);           अगर (r) वापस r;
                } अन्यथा {                                                    // table.fill
                    r = v_निकालो_expect(v, at, &a); अगर (r) वापस r;  // n
                    r = v_निकालो_expect(v, t, &a);  अगर (r) वापस r;  // val
                    r = v_निकालो_expect(v, at, &a); अगर (r) वापस r;  // i
                }
                अवरोध;
            }
#पूर्ण_अगर
            हाल 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                अगर (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) वापस m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_निकालो_expect(v, lenType, &a); अगर (r) वापस r; // n
                r = v_निकालो_expect(v, srcType, &a); अगर (r) वापस r; // src
                r = v_निकालो_expect(v, dstType, &a); अगर (r) वापस r; // dst
                अवरोध;
            }
            हाल 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                अगर (not v_has_memory_idx(v, memidx)) वापस m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_निकालो_expect(v, addrType, &a); अगर (r) वापस r;      // n
                r = v_निकालो_expect(v, c_m3Type_i32, &a); अगर (r) वापस r;  // val
                r = v_निकालो_expect(v, addrType, &a); अगर (r) वापस r;      // dst
                अवरोध;
            }
#अगर d_m3HasWideArithmetic
            हाल 0x13: हाल 0x14: // i64.add128, i64.sub128
            हाल 0x15: हाल 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 अंक_Operands = (sub <= 0x14) ? 4 : 2;
                क्रम (u32 i = 0; i < अंक_Operands; ++i) {
                    r = v_निकालो_expect(v, c_m3Type_i64, &a); अगर (r) वापस r;
                }
                r = v_धकेलो(v, c_m3Type_i64); अगर (r) वापस r;
                r = v_धकेलो(v, c_m3Type_i64); अगर (r) वापस r;
                अवरोध;
            }
#पूर्ण_अगर
            शेष:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                अवरोध;
            }
            अवरोध;
        }

#अगर d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        हाल 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            अगर (r) वापस r;

            अगर (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                अगर (reserved != 0) वापस m3Err_wasmMalformed;
                अवरोध;
            }

            u32 width, अंक_Operands; m3type_t मूल्य_Type, परिणाम_Type;
            अगर (not v_atomic_shape(sub, &width, &अंक_Operands, &मूल्य_Type, &परिणाम_Type)) वापस m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            अगर ((1u << align) != width) वापस m3Err_invalidAtomicAlignment;
            अगर (not v_has_memory_idx(v, memidx)) वापस m3Err_unknownMemory;
            अगर (not v_offset_in_range(v, memidx, offset)) वापस m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            अगर (sub == 0x01 or sub == 0x02) {
                r = v_निकालो_expect(v, c_m3Type_i64, &a); अगर (r) वापस r;
                अंक_Operands--;
            }
            क्रम (u32 i = 1; i < अंक_Operands; ++i) {
                r = v_निकालो_expect(v, मूल्य_Type, &a); अगर (r) वापस r;
            }
            r = v_निकालो_expect(v, v_memory_addrtype(v, memidx), &a); अगर (r) वापस r;
            अगर (परिणाम_Type != c_m3Type_none) {
                r = v_धकेलो(v, परिणाम_Type); अगर (r) वापस r;
            }
            अवरोध;
        }
#पूर्ण_अगर

#अगर d_m3_Has_ढेर_Switching
        // ---- Stack Switching ----
        हाल 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            अगर (!ft || !ft->isContinuation) वापस m3Err_typeMismatch;
            r = v_निकालो_expect(v, c_m3Type_funcref, &a); अगर (r) वापस r;
            r = v_धकेलो(v, c_m3Type_contref); अगर (r) वापस r;
            अवरोध;
        }

        हाल 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (typeIdx1 >= v->module->अंक_Func_Types || typeIdx2 >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            अगर (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) वापस m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 अंक_Params1 = ft1->अंक_Args;
            u16 अंक_Params2 = ft2->अंक_Args;
            अगर (अंक_Params1 < अंक_Params2) वापस m3Err_typeMismatch;
            u32 अंक_Bound = अंक_Params1 - अंक_Params2;

            अगर (ft1->अंक_Rets != ft2->अंक_Rets) वापस m3Err_typeMismatch;
            क्रम (u16 i = 0; i < ft1->अंक_Rets; ++i) {
                अगर (ft1->types[i] != ft2->types[i]) वापस m3Err_typeMismatch;
            }
            क्रम (u16 i = 0; i < अंक_Params2; ++i) {
                अगर (ft1->types[ft1->अंक_Rets + अंक_Bound + i] != ft2->types[ft2->अंक_Rets + i]) वापस m3Err_typeMismatch;
            }

            r = v_निकालो_expect(v, c_m3Type_contref, &a); अगर (r) वापस r;

            क्रम (u32 i = अंक_Bound; i > 0; i--) {
                r = v_निकालो_expect(v, BaseTypeOf(ft1->types[ft1->अंक_Rets + i - 1]), &a);
                अगर (r) वापस r;
            }

            r = v_धकेलो(v, c_m3Type_contref); अगर (r) वापस r;
            अवरोध;
        }

        हाल 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (!v->module || tagIdx >= v->module->अंक_Tags) वापस m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            अगर (tt) {
                क्रम (u16 i = tt->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(tt->types[tt->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
                क्रम (u16 i = 0; i < tt->अंक_Rets; i++) {
                    r = v_धकेलो(v, BaseTypeOf(tt->types[i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            अगर (!ct || !ct->isContinuation) वापस m3Err_typeMismatch;

            u32 अंक_Clauses;
            r = ReadLEB_u32(&अंक_Clauses, &v->wasm, v->wasmEnd); अगर (r) वापस r;

            क्रम (u32 i = 0; i < अंक_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                r = v_resume_clause(v, kind, tagIdx, ct); अगर (r) वापस r;
            }

            r = v_निकालो_expect(v, c_m3Type_contref, &a); अगर (r) वापस r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            अगर (ft) {
                क्रम (u16 i = ft->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(ft->types[ft->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
                क्रम (u16 i = 0; i < ft->अंक_Rets; i++) {
                    r = v_धकेलो(v, BaseTypeOf(ft->types[i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, अंक_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            r = ReadLEB_u32(&अंक_Clauses, &v->wasm, v->wasmEnd); अगर (r) वापस r;

            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            अगर (!v->module || tagIdx >= v->module->अंक_Tags) वापस m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            अगर (!ct || !ct->isContinuation) वापस m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            अगर (tagType && tagType->अंक_Rets != 0) वापस m3Err_typeMismatch;

            क्रम (u32 i = 0; i < अंक_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                r = v_resume_clause(v, kind, cTagIdx, ct); अगर (r) वापस r;
            }

            r = v_निकालो_expect(v, c_m3Type_contref, &a); अगर (r) वापस r;

            अगर (tagType) {
                क्रम (u16 i = tagType->अंक_Args; i > 0; i--) {
                    r = v_निकालो_expect(v, BaseTypeOf(tagType->types[tagType->अंक_Rets + i - 1]), &a);
                    अगर (r) वापस r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            अगर (ft) {
                क्रम (u16 i = 0; i < ft->अंक_Rets; i++) {
                    r = v_धकेलो(v, BaseTypeOf(ft->types[i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0xe5: // resume_throw_ref
        {
            u32 typeIdx, अंक_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            r = ReadLEB_u32(&अंक_Clauses, &v->wasm, v->wasmEnd); अगर (r) वापस r;

            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            अगर (!ct || !ct->isContinuation) वापस m3Err_typeMismatch;

            क्रम (u32 i = 0; i < अंक_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
                r = v_resume_clause(v, kind, cTagIdx, ct); अगर (r) वापस r;
            }

            r = v_निकालो_expect(v, c_m3Type_contref, &a); अगर (r) वापस r;
            r = v_निकालो_expect(v, c_m3Type_exnref, &a); अगर (r) वापस r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            अगर (ft) {
                क्रम (u16 i = 0; i < ft->अंक_Rets; i++) {
                    r = v_धकेलो(v, BaseTypeOf(ft->types[i]));
                    अगर (r) वापस r;
                }
            }
            अवरोध;
        }

        हाल 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); अगर (r) वापस r;

            अगर (typeIdx >= v->module->अंक_Func_Types) वापस m3Err_unknownType;
            अगर (!v->module || tagIdx >= v->module->अंक_Tags) वापस m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            अगर (!ct || !ct->isContinuation) वापस m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            अगर (tagType && tagType->अंक_Args != 0) वापस m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            अगर (!ft || ft->अंक_Args == 0) वापस m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->अंक_Rets + ft->अंक_Args - 1]);
            अगर (!peer || !peer->contFuncType) वापस m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_निकालो_expect(v, c_m3Type_contref, &a); अगर (r) वापस r;

            क्रम (u16 i = (u16)(ft->अंक_Args - 1); i > 0; i--) {
                r = v_निकालो_expect(v, BaseTypeOf(ft->types[ft->अंक_Rets + i - 1]), &a);
                अगर (r) वापस r;
            }

            क्रम (u16 i = 0; i < self->अंक_Args; i++) {
                r = v_धकेलो(v, BaseTypeOf(self->types[self->अंक_Rets + i]));
                अगर (r) वापस r;
            }
            अवरोध;
        }

#पूर्ण_अगर // d_m3HasStackSwitching

        शेष:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            अवरोध;

        } // switch

        अगर (r) वापस r;

    } // while

    // If we ran out of bytes without hitting the final end
    वापस m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3_परिणाम  ValidateFunction  (IM3Function i_function)
{
    अगर (!i_function->wasm) वापस m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    अगर (!v) {
        v = m3_AllocStruct (ValCtx);
        अगर (!v) वापस m3Err_mallocFailed;
        runtime->validator = v;
    }

    स_रखो(v, 0, माप(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 आकार;
    M3_परिणाम r = ReadLEB_u32(&आकार, &v->wasm, v->wasmEnd);
    अगर (r) वापस r;

    // Parse locals
    u32 अंक_Local_Blocks;
    r = ReadLEB_u32(&अंक_Local_Blocks, &v->wasm, v->wasmEnd);
    अगर (r) वापस r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 अंक_Params = funcType ? funcType->अंक_Args : 0;
    अगर (अंक_Params > d_m3_Val_ढेर) वापस m3_Err_function_ढेर_Overflow;
    क्रम (u16 i = 0; i < अंक_Params; i++) {
        v->localTypes[v->अंक_Locals++] = BaseTypeOf(funcType->types[funcType->अंक_Rets + i]);
    }

    // Then: declared locals
    क्रम (u32 b = 0; b < अंक_Local_Blocks; b++) {
        u32 गिनती;
        r = ReadLEB_u32(&गिनती, &v->wasm, v->wasmEnd);
        अगर (r) वापस r;
        m3type_t localType;
        r = Parse_मूल्य_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        अगर (r) वापस r;
        u8 normalized = BaseTypeOf(localType);
        अगर (गिनती > (u32) (d_m3_Val_ढेर - v->अंक_Locals)) वापस m3_Err_function_ढेर_Overflow;
        क्रम (u32 c = 0; c < गिनती; c++) {
            v->localTypes[v->अंक_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_धकेलो_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    अगर (r) वापस r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    अगर (r) वापस r;

    // After validation, control stack should be empty and all declared bytes consumed
    अगर (v->ctrl_शीर्ष != 0)
        वापस m3Err_wasmMalformed;

    अगर (v->wasm != v->wasmEnd)
        वापस m3Err_wasmSectionUnderrun;

    वापस m3Err_none;
}

#अन्यथा // !d_m3EnableValidation

M3_परिणाम  ValidateFunction  (IM3Function i_function)
{
    (व्योम)i_function;
    वापस m3Err_none;
}

// clang-format on

#पूर्ण_अगर // d_m3EnableValidation
