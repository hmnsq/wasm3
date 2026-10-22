//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#அந்தர்பாவ "m3_validate.h"
#அந்தர்பாவ "m3_exception.h"
#அந்தர்பாவ "m3_info.h"

#எனில் d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

வகைவரை கட்டமைப்பு {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_மொத்தம்;
    u16         result_மொத்தம்;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

கட்டமைப்பு ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_அடுக்கு];
    u16         opd_உச்சி;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_உச்சி;

    m3type_t    localTypes [d_m3_Val_அடுக்கு];
    u16         இலக்கம்_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
நிலையான bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    திரும்பு v->module and i_memoryIdx < v->module->இலக்கம்_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
நிலையான m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    திரும்பு Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
நிலையான bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    திரும்பு v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
நிலையான m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    திரும்பு Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
நிலையான u32 v_max_align (m3opcode_t opcode)
{
    தேர்வு (opcode) {
    நிலை 0x2c: நிலை 0x2d:   // i32.load8_s, i32.load8_u
    நிலை 0x30: நிலை 0x31:   // i64.load8_s, i64.load8_u
    நிலை 0x3a:              // i32.store8
    நிலை 0x3c:              // i64.store8
        திரும்பு 0;
    நிலை 0x2e: நிலை 0x2f:   // i32.load16_s, i32.load16_u
    நிலை 0x32: நிலை 0x33:   // i64.load16_s, i64.load16_u
    நிலை 0x3b:              // i32.store16
    நிலை 0x3d:              // i64.store16
        திரும்பு 1;
    நிலை 0x29:              // i64.load
    நிலை 0x2b:              // f64.load
    நிலை 0x37:              // i64.store
    நிலை 0x39:              // f64.store
        திரும்பு 3;
    இயல்பு:                // 32-bit accesses, and a safe fallback
        திரும்பு 2;
    }
}

#எனில் d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
நிலையான bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_இலக்கம்_Operands, m3type_t * o_மதிப்பு_Type, m3type_t * o_resultType)
{
    நிலையான மாறிலி u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    நிலையான மாறிலி m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    எனில் (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_இலக்கம்_Operands = (sub == 0x00) ? 2 : 3;
        *o_மதிப்பு_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        திரும்பு true;
    }

    எனில் (sub < 0x10 or sub > c_waOp_lastAtomic) திரும்பு false;

    எனில் (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_இலக்கம்_Operands = 1;
    } இல்லையெனில் எனில் (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_இலக்கம்_Operands = 2;
    } இல்லையெனில் எனில் (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_இலக்கம்_Operands = 2;
    } இல்லையெனில் {                        // cmpxchg
        variant = sub - 0x48;       *o_இலக்கம்_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_மதிப்பு_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    திரும்பு true;
}

#endif

// ---------- Operand stack ----------

நிலையான M3Result v_தள்ளு (ValCtx * v, m3type_t type)
{
    எனில் (M3_UNLIKELY(v->opd_உச்சி >= d_m3_Val_அடுக்கு))
        திரும்பு m3_Err_function_அடுக்கு_Overflow;
    v->opd[v->opd_உச்சி++] = type;
    திரும்பு m3Err_none;
}

நிலையான M3Result v_எடு (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_உச்சி - 1];
    எனில் (M3_UNLIKELY(v->opd_உச்சி == f->height)) {
        எனில் (f->is_unreachable) { *o_type = c_valBottom; திரும்பு m3Err_none; }
        திரும்பு m3_Err_function_அடுக்கு_Underrun;
    }
    *o_type = v->opd[--v->opd_உச்சி];
    திரும்பு m3Err_none;
}

நிலையான M3Result v_எடு_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_எடு(v, &actual);
    எனில் (M3_UNLIKELY(r)) திரும்பு r;
    எனில் (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        திரும்பு m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    திரும்பு m3Err_none;
}

// ---------- Control stack ----------

நிலையான M3Result v_தள்ளு_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    எனில் (M3_UNLIKELY(v->ctrl_உச்சி >= d_m3ValCtrlDepth))
        திரும்பு m3_Err_function_அடுக்கு_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_உச்சி++];
    f->opcode       = op;
    f->type         = type;
    f->param_மொத்தம்  = type ? type->இலக்கம்_Args : 0;
    f->result_மொத்தம் = type ? type->இலக்கம்_Rets : 0;
    f->height       = v->opd_உச்சி;
    f->is_unreachable = false;
    திரும்பு m3Err_none;
}

நிலையான M3Result v_எடு_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    எனில் (M3_UNLIKELY(v->ctrl_உச்சி == 0))
        திரும்பு m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_உச்சி - 1];
    // pop result types
    எனில் (f->type) {
        க்கு (u16 i = f->result_மொத்தம்; i > 0; i--) {
            m3type_t a;
            M3Result r = v_எடு_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            எனில் (M3_UNLIKELY(r)) திரும்பு r;
        }
    }
    எனில் (M3_UNLIKELY(v->opd_உச்சி != f->height))
        திரும்பு m3_Err_type_மொத்தம்_Mismatch;
    எனில் (o_frame) *o_frame = *f;
    v->ctrl_உச்சி--;
    திரும்பு m3Err_none;
}

நிலையான வெறுமை v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_உச்சி - 1];
    v->opd_உச்சி = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
நிலையான u16 v_label_n (ValCtrlFrame * f)
{
    திரும்பு (f->opcode == 0x03) ? f->param_மொத்தம் : f->result_மொத்தம்;
}

நிலையான m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    எனில் (!f->type) திரும்பு c_m3Type_none;
    எனில் (f->opcode == 0x03)
        திரும்பு BaseTypeOf(f->type->types[f->type->இலக்கம்_Rets + i]); // params
    திரும்பு BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
நிலையான m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_உச்சி - 1];

    எனில் (v->opd_உச்சி <= i_depth || (u16)(v->opd_உச்சி - i_depth - 1) < f->height)
        திரும்பு c_valBottom;

    திரும்பு v->opd[v->opd_உச்சி - i_depth - 1];
}

// Pop label types for branch target
நிலையான M3Result v_எடு_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    க்கு (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_எடு_expect(v, v_label_t(tgt, i - 1), &a);
        எனில் (M3_UNLIKELY(r)) திரும்பு r;
    }
    திரும்பு m3Err_none;
}

// Push label types back
நிலையான M3Result v_தள்ளு_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    க்கு (u16 i = 0; i < n; i++) {
        M3Result r = v_தள்ளு(v, v_label_t(tgt, i));
        எனில் (M3_UNLIKELY(r)) திரும்பு r;
    }
    திரும்பு m3Err_none;
}

// ---------- Block type resolution ----------

நிலையான M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    எனில் (v->wasm >= v->wasmEnd)
        திரும்பு m3Err_wasmUnderrun;

#எனில் d_m3HasTypedRefs
    எனில் (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_மதிப்பு_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        எனில் (rr) திரும்பு rr;
        IM3FuncType ftype = வெற்று;
        rr = AllocFuncType(&ftype, 1);
        எனில் (rr) திரும்பு rr;
        ftype->இலக்கம்_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        எனில் (rr) திரும்பு rr;
        *o_type = ftype;
        திரும்பு m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    எனில் (r) திரும்பு r;

    எனில் (type < 0) {
        எனில் (type < -64) திரும்பு m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        எனில் (r) திரும்பு r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } இல்லையெனில் {
        எனில் ((u32)type >= v->module->இலக்கம்_Func_Types)
            திரும்பு m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    திரும்பு m3Err_none;
}

// ---------- Convenience ----------

நிலையான M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_எடு_expect(v, in, &a);
    எனில் (M3_UNLIKELY(r)) திரும்பு r;
    திரும்பு v_தள்ளு(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
நிலையான M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : வெற்று;

    u16 இலக்கம்_Callee_Rets = i_calleeType ? i_calleeType->இலக்கம்_Rets : 0;
    u16 இலக்கம்_Func_Rets   = ft ? ft->இலக்கம்_Rets : 0;

    எனில் (இலக்கம்_Callee_Rets != இலக்கம்_Func_Rets)
        திரும்பு m3Err_typeMismatch;

    க்கு (u16 i = 0; i < இலக்கம்_Callee_Rets; i++) {
        எனில் (i_calleeType->types[i] != ft->types[i])
            திரும்பு m3Err_typeMismatch;
    }

    திரும்பு m3Err_none;
}

நிலையான M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_எடு_expect(v, t, &a); எனில் (M3_UNLIKELY(r)) திரும்பு r;
    r = v_எடு_expect(v, t, &a); எனில் (M3_UNLIKELY(r)) திரும்பு r;
    திரும்பு v_தள்ளு(v, t);
}

நிலையான M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_எடு_expect(v, t, &a); எனில் (M3_UNLIKELY(r)) திரும்பு r;
    r = v_எடு_expect(v, t, &a); எனில் (M3_UNLIKELY(r)) திரும்பு r;
    திரும்பு v_தள்ளு(v, c_m3Type_i32);
}

நிலையான M3Result v_testop (ValCtx * v, m3type_t t)
{
    திரும்பு v_unop(v, t, c_m3Type_i32);
}

நிலையான M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    திரும்பு v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#எனில் d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
நிலையான M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    எனில் (r) திரும்பு r;
    எனில் (kind > 0x03) திரும்பு m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = வெற்று;
    எனில் (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        எனில் (r) திரும்பு r;
        எனில் (!v->module || tagIndex >= v->module->இலக்கம்_Tags) திரும்பு m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    எனில் (r) திரும்பு r;
    எனில் (depth >= v->ctrl_உச்சி) திரும்பு m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_உச்சி - 1 - depth];

    u16 இலக்கம்_Payload = tagType ? tagType->இலக்கம்_Args : 0;
    u16 இலக்கம்_Label   = v_label_n(tgt);

    எனில் (இலக்கம்_Label != இலக்கம்_Payload + (hasRef ? 1u : 0u))
        திரும்பு m3_Err_type_மொத்தம்_Mismatch;

    க்கு (u16 i = 0; i < இலக்கம்_Payload; i++) {
        எனில் (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->இலக்கம்_Rets + i]))
            திரும்பு m3Err_typeMismatch;
    }

    எனில் (hasRef and v_label_t(tgt, இலக்கம்_Payload) != c_m3Type_exnref)
        திரும்பு m3Err_typeMismatch;

    திரும்பு m3Err_none;
}

#endif // d_m3HasExceptionHandling

#எனில் d_m3_Has_அடுக்கு_Switching

நிலையான M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    எனில் (kind > 0x01) திரும்பு m3Err_wasmMalformed;
    எனில் (!v->module || tagIdx >= v->module->இலக்கம்_Tags) திரும்பு m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    எனில் (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
        எனில் (depth >= v->ctrl_உச்சி) திரும்பு m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_உச்சி - 1 - depth];
        u16 இலக்கம்_Payload = tagType ? tagType->இலக்கம்_Args : 0;
        u16 இலக்கம்_Label   = v_label_n(tgt);

        எனில் (இலக்கம்_Label != இலக்கம்_Payload + 1u) திரும்பு m3_Err_type_மொத்தம்_Mismatch;

        க்கு (u16 j = 0; j < இலக்கம்_Payload; j++) {
            எனில் (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->இலக்கம்_Rets + j]))
                திரும்பு m3Err_typeMismatch;
        }
        எனில் (v_label_t(tgt, இலக்கம்_Payload) != c_m3Type_contref)
            திரும்பு m3Err_typeMismatch;

#எனில் d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->இலக்கம்_Rets + இலக்கம்_Payload]
            : tgt->type->types[இலக்கம்_Payload];

        எனில் (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            திரும்பு m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = வெற்று;
        க்கு (u32 idx = 0; idx < v->module->இலக்கம்_Func_Types; ++idx) {
            எனில் (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                நிறுத்து;
            }
        }
        எனில் (!labelCt) திரும்பு m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_இலக்கம்_Params = tagType ? tagType->இலக்கம்_Rets : 0;
        எனில் (labelFt->இலக்கம்_Args != expected_இலக்கம்_Params) திரும்பு m3Err_typeMismatch;
        க்கு (u16 p = 0; p < expected_இலக்கம்_Params; ++p) {
            எனில் (labelFt->types[labelFt->இலக்கம்_Rets + p] != tagType->types[p])
                திரும்பு m3Err_typeMismatch;
        }

        u16 expected_இலக்கம்_Rets = ft ? ft->இலக்கம்_Rets : 0;
        எனில் (labelFt->இலக்கம்_Rets != expected_இலக்கம்_Rets) திரும்பு m3Err_typeMismatch;
        க்கு (u16 r_idx = 0; r_idx < expected_இலக்கம்_Rets; ++r_idx) {
            எனில் (labelFt->types[r_idx] != ft->types[r_idx])
                திரும்பு m3Err_typeMismatch;
        }
#endif
    } இல்லையெனில் {
        எனில் (tagType && tagType->இலக்கம்_Args != 0) திரும்பு m3Err_typeMismatch;
    }
    திரும்பு m3Err_none;
}

#endif // d_m3HasStackSwitching

நிலையான M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    போது (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        எனில் (M3_UNLIKELY(r)) திரும்பு r;

        தேர்வு (opcode)
        {
        // ---- Control ----
        நிலை 0x00: // unreachable
            v_unreachable(v);
            நிறுத்து;

        நிலை 0x01: // nop
            நிறுத்து;

        நிலை 0x02: // block
        நிலை 0x03: // loop
        நிலை 0x04: // if
        {
            IM3FuncType bt = வெற்று;
            r = v_read_blocktype(v, &bt);
            எனில் (r) திரும்பு r;
            எனில் (opcode == 0x04) {
                r = v_எடு_expect(v, c_m3Type_i32, &a);
                எனில் (r) திரும்பு r;
            }
            // Pop block params from caller stack
            எனில் (bt) {
                க்கு (u16 i = bt->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(bt->types[bt->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
            }
            r = v_தள்ளு_ctrl(v, opcode, bt);
            எனில் (r) திரும்பு r;
            // Push params inside block
            எனில் (bt) {
                க்கு (u16 i = 0; i < bt->இலக்கம்_Args; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(bt->types[bt->இலக்கம்_Rets + i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

#எனில் d_m3HasExceptionHandling
        நிலை 0x1f: // try_table
        {
            IM3FuncType bt = வெற்று;
            r = v_read_blocktype(v, &bt);
            எனில் (r) திரும்பு r;
            எனில் (bt) {
                க்கு (u16 i = bt->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(bt->types[bt->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 இலக்கம்_Catch;
            r = ReadLEB_u32(&இலக்கம்_Catch, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            க்கு (u32 i = 0; i < இலக்கம்_Catch; i++) {
                r = v_catch_clause(v);
                எனில் (r) திரும்பு r;
            }

            r = v_தள்ளு_ctrl(v, opcode, bt);
            எனில் (r) திரும்பு r;

            எனில் (bt) {
                க்கு (u16 i = 0; i < bt->இலக்கம்_Args; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(bt->types[bt->இலக்கம்_Rets + i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (!v->module || tagIndex >= v->module->இலக்கம்_Tags) திரும்பு m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            எனில் (tt) {
                க்கு (u16 i = tt->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(tt->types[tt->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
            }
            v_unreachable(v);
            நிறுத்து;
        }

        நிலை 0x0a: // throw_ref
            r = v_எடு_expect(v, c_m3Type_exnref, &a);
            எனில் (r) திரும்பு r;
            v_unreachable(v);
            நிறுத்து;
#endif // d_m3HasExceptionHandling

        நிலை 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_எடு_ctrl(v, &frame);
            எனில் (r) திரும்பு r;
            எனில் (frame.opcode != 0x04)
                திரும்பு m3Err_wasmMalformed;
            r = v_தள்ளு_ctrl(v, 0x05, frame.type);
            எனில் (r) திரும்பு r;
            எனில் (frame.type) {
                க்கு (u16 i = 0; i < frame.type->இலக்கம்_Args; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(frame.type->types[frame.type->இலக்கம்_Rets + i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_எடு_ctrl(v, &frame);
            எனில் (r) திரும்பு r;
            // Push results
            எனில் (frame.type) {
                க்கு (u16 i = 0; i < frame.result_மொத்தம்; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(frame.type->types[i]));
                    எனில் (r) திரும்பு r;
                }
            }
            // If this was the outermost frame, we're done
            எனில் (v->ctrl_உச்சி == 0)
                திரும்பு m3Err_none;
            நிறுத்து;
        }

        நிலை 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (depth >= v->ctrl_உச்சி) திரும்பு m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_உச்சி - 1 - depth];
            r = v_எடு_labels(v, tgt);
            எனில் (r) திரும்பு r;
            v_unreachable(v);
            நிறுத்து;
        }

        நிலை 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (depth >= v->ctrl_உச்சி) திரும்பு m3Err_unknownLabel;
            r = v_எடு_expect(v, c_m3Type_i32, &a);
            எனில் (r) திரும்பு r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_உச்சி - 1 - depth];
            r = v_எடு_labels(v, tgt);
            எனில் (r) திரும்பு r;
            r = v_தள்ளு_labels(v, tgt);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0x0e: // br_table
        {
            u32 மொத்தம்;
            r = ReadLEB_u32(&மொத்தம், &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            க்கு (u32 i = 0; i <= மொத்தம்; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                எனில் (r) திரும்பு r;
                எனில் (d >= v->ctrl_உச்சி) திரும்பு m3Err_unknownLabel;
                எனில் (i == மொத்தம்) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_உச்சி - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_எடு_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            எனில் (r) திரும்பு r;

            க்கு (u32 i = 0; i <= மொத்தம்; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                எனில் (r) திரும்பு r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_உச்சி - 1 - d];
                u16 n = v_label_n(t);
                எனில் (n != arity) திரும்பு m3_Err_type_மொத்தம்_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                க்கு (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    எனில் (want != c_valBottom && have != c_valBottom && want != have)
                        திரும்பு m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_உச்சி - 1 - defDepth];
            r = v_எடு_labels(v, dt);
            எனில் (r) திரும்பு r;
            v_unreachable(v);
            நிறுத்து;
        }

        நிலை 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            எனில் (ft) {
                க்கு (u16 i = ft->இலக்கம்_Rets; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
            }
            v_unreachable(v);
            நிறுத்து;
        }

        // ---- Call ----
        நிலை 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (idx >= v->module->இலக்கம்_Functions) திரும்பு m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            எனில் (ft) {
                க்கு (u16 i = ft->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(ft->types[ft->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
                க்கு (u16 i = 0; i < ft->இலக்கம்_Rets; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(ft->types[i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            // the table must exist and hold funcrefs
            எனில் (tableIdx >= v->module->இலக்கம்_Tables) திரும்பு m3Err_unknownTable;
            எனில் (v->module->tables[tableIdx]->type != c_m3Type_funcref) திரும்பு m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_எடு_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            எனில் (r) திரும்பு r;
            எனில் (ft) {
                க்கு (u16 i = ft->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(ft->types[ft->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
                க்கு (u16 i = 0; i < ft->இலக்கம்_Rets; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(ft->types[i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (idx >= v->module->இலக்கம்_Functions) திரும்பு m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            எனில் (r) திரும்பு r;
            எனில் (ft) {
                க்கு (u16 i = ft->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(ft->types[ft->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
            }
            v_unreachable(v);
            நிறுத்து;
        }

        நிலை 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            // the table must exist and hold funcrefs
            எனில் (tableIdx >= v->module->இலக்கம்_Tables) திரும்பு m3Err_unknownTable;
            எனில் (v->module->tables[tableIdx]->type != c_m3Type_funcref) திரும்பு m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            எனில் (r) திரும்பு r;
            r = v_எடு_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            எனில் (r) திரும்பு r;
            எனில் (ft) {
                க்கு (u16 i = ft->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(ft->types[ft->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
            }
            v_unreachable(v);
            நிறுத்து;
        }

#எனில் d_m3HasTypedRefs
        நிலை 0x14: // call_ref
        நிலை 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            எனில் (isTail) {
                r = v_check_tail_results(v, ft);
                எனில் (r) திரும்பு r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_எடு_expect(v, c_m3Type_funcref, &a);
            எனில் (r) திரும்பு r;
            எனில் (ft) {
                க்கு (u16 i = ft->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(ft->types[ft->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
                எனில் (not isTail) {
                    க்கு (u16 i = 0; i < ft->இலக்கம்_Rets; i++) {
                        r = v_தள்ளு(v, BaseTypeOf(ft->types[i]));
                        எனில் (r) திரும்பு r;
                    }
                }
            }
            எனில் (isTail) v_unreachable(v);
            நிறுத்து;
        }

        நிலை 0xd4: // ref.as_non_null
        {
            r = v_எடு_expect(v, c_m3Type_funcref, &a);
            எனில் (r) திரும்பு r;
            r = v_தள்ளு(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        நிலை 0x1a: // drop
            r = v_எடு(v, &a);
            எனில் (r) திரும்பு r;
            நிறுத்து;

        நிலை 0x1b: // select
        {
            r = v_எடு_expect(v, c_m3Type_i32, &a);
            எனில் (r) திரும்பு r;
            m3type_t t2 = c_valBottom;
            r = v_எடு(v, &t2);
            எனில் (r) திரும்பு r;
            m3type_t t1;
            r = v_எடு_expect(v, t2, &t1);
            எனில் (r) திரும்பு r;
            // untyped select is numeric only; references need the 0x1c form
            எனில் (t2 != c_valBottom && IsRefType(t2)) திரும்பு m3Err_typeMismatch;
            r = v_தள்ளு(v, (t2 == c_valBottom) ? t1 : t2);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

#எனில் d_m3HasRefTypes
        நிலை 0x1c: // select with an explicit result type
        {
            u32 இலக்கம்_Types;
            r = ReadLEB_u32(&இலக்கம்_Types, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (இலக்கம்_Types != 1) திரும்பு m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_மதிப்பு_Type(v->module, &selType, &v->wasm, v->wasmEnd);  எனில் (r) திரும்பு r;
            u8 t = BaseTypeOf(selType);

            r = v_எடு_expect(v, c_m3Type_i32, &a); எனில் (r) திரும்பு r;
            r = v_எடு_expect(v, t, &a);            எனில் (r) திரும்பு r;
            r = v_எடு_expect(v, t, &a);            எனில் (r) திரும்பு r;
            r = v_தள்ளு(v, t);                      எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0x25: // table.get
        நிலை 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (tableIdx >= v->module->இலக்கம்_Tables) திரும்பு m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            எனில் (opcode == 0x26) {
                r = v_எடு_expect(v, t, &a);   எனில் (r) திரும்பு r;
                r = v_எடு_expect(v, at, &a);  எனில் (r) திரும்பு r;
            } இல்லையெனில் {
                r = v_எடு_expect(v, at, &a);  எனில் (r) திரும்பு r;
                r = v_தள்ளு(v, t);             எனில் (r) திரும்பு r;
            }
            நிறுத்து;
        }

        நிலை 0xd0: // ref.null
        {
#எனில் d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  எனில் (r) திரும்பு r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# எனில் d_m3_Has_அடுக்கு_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#இல்லையெனில்
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   எனில் (r) திரும்பு r;
            r = NormalizeType(&t, waType);                   எனில் (r) திரும்பு r;
            எனில் (!IsRefType(t)) திரும்பு m3Err_wasmMalformed;
#endif
            r = v_தள்ளு(v, t);                                எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_எடு(v, &t);
            எனில் (r) திரும்பு r;
            எனில் (t != c_valBottom && !IsRefType(t)) திரும்பு m3Err_typeMismatch;
            r = v_தள்ளு(v, c_m3Type_i32);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (funcIdx >= v->module->இலக்கம்_Functions) திரும்பு m3Err_unknownFunction;
            எனில் (!Module_IsFunctionDeclared(v->module, funcIdx)) திரும்பு m3Err_undeclaredFuncRef;
            r = v_தள்ளு(v, c_m3Type_funcref);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }
#endif

        // ---- Variable ----
        நிலை 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (idx >= v->இலக்கம்_Locals) திரும்பு m3Err_unknownLocal;
            r = v_தள்ளு(v, v->localTypes[idx]);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (idx >= v->இலக்கம்_Locals) திரும்பு m3Err_unknownLocal;
            r = v_எடு_expect(v, v->localTypes[idx], &a);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (idx >= v->இலக்கம்_Locals) திரும்பு m3Err_unknownLocal;
            r = v_எடு_expect(v, v->localTypes[idx], &a);
            எனில் (r) திரும்பு r;
            r = v_தள்ளு(v, v->localTypes[idx]);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (idx >= v->module->இலக்கம்_Globals) திரும்பு m3Err_unknownGlobal;
            r = v_தள்ளு(v, BaseTypeOf(v->module->globals[idx].type));
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            எனில் (idx >= v->module->இலக்கம்_Globals) திரும்பு m3Err_unknownGlobal;
            r = v_எடு_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        // ---- Memory load ----
        நிலை 0x28: நிலை 0x29: நிலை 0x2a: நிலை 0x2b: // i32/i64/f32/f64.load
        நிலை 0x2c: நிலை 0x2d: நிலை 0x2e: நிலை 0x2f: // i32.load8/16 s/u
        நிலை 0x30: நிலை 0x31: நிலை 0x32: நிலை 0x33: // i64.load8/16 s/u
        நிலை 0x34: நிலை 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (align > v_max_align(opcode)) திரும்பு m3Err_invalidAlignment;
            எனில் (not v_has_memory_idx(v, memidx)) திரும்பு m3Err_unknownMemory;
            எனில் (not v_offset_in_range(v, memidx, offset)) திரும்பு m3Err_wasmMalformed;
            r = v_எடு_expect(v, v_memory_addrtype(v, memidx), &a); எனில் (r) திரும்பு r;
            u8 result;
            எனில்      (opcode == 0x28) result = c_m3Type_i32;
            இல்லையெனில் எனில் (opcode == 0x29) result = c_m3Type_i64;
            இல்லையெனில் எனில் (opcode == 0x2a) result = c_m3Type_f32;
            இல்லையெனில் எனில் (opcode == 0x2b) result = c_m3Type_f64;
            இல்லையெனில் எனில் (opcode <= 0x2f) result = c_m3Type_i32;
            இல்லையெனில்                     result = c_m3Type_i64;
            r = v_தள்ளு(v, result);
            எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        // ---- Memory store ----
        நிலை 0x36: நிலை 0x37: நிலை 0x38: நிலை 0x39: // i32/i64/f32/f64.store
        நிலை 0x3a: நிலை 0x3b:                         // i32.store8/16
        நிலை 0x3c: நிலை 0x3d: நிலை 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (align > v_max_align(opcode)) திரும்பு m3Err_invalidAlignment;
            எனில் (not v_has_memory_idx(v, memidx)) திரும்பு m3Err_unknownMemory;
            எனில் (not v_offset_in_range(v, memidx, offset)) திரும்பு m3Err_wasmMalformed;
            u8 valtype;
            எனில்      (opcode == 0x36) valtype = c_m3Type_i32;
            இல்லையெனில் எனில் (opcode == 0x37) valtype = c_m3Type_i64;
            இல்லையெனில் எனில் (opcode == 0x38) valtype = c_m3Type_f32;
            இல்லையெனில் எனில் (opcode == 0x39) valtype = c_m3Type_f64;
            இல்லையெனில் எனில் (opcode <= 0x3b) valtype = c_m3Type_i32;
            இல்லையெனில்                     valtype = c_m3Type_i64;
            r = v_எடு_expect(v, valtype, &a); எனில் (r) திரும்பு r;
            r = v_எடு_expect(v, v_memory_addrtype(v, memidx), &a); எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        // ---- Memory size/grow ----
        நிலை 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (not v_has_memory_idx(v, memidx)) திரும்பு m3Err_unknownMemory;
            r = v_தள்ளு(v, v_memory_addrtype(v, memidx)); எனில் (r) திரும்பு r;
            நிறுத்து;
        }
        நிலை 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (not v_has_memory_idx(v, memidx)) திரும்பு m3Err_unknownMemory;
            r = v_எடு_expect(v, v_memory_addrtype(v, memidx), &a); எனில் (r) திரும்பு r;
            r = v_தள்ளு(v, v_memory_addrtype(v, memidx)); எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        // ---- Constants ----
        நிலை 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            r = v_தள்ளு(v, c_m3Type_i32); எனில் (r) திரும்பு r;
            நிறுத்து;
        }
        நிலை 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            r = v_தள்ளு(v, c_m3Type_i64); எனில் (r) திரும்பு r;
            நிறுத்து;
        }
        நிலை 0x43: { // f32.const
            எனில் (v->wasm + 4 > v->wasmEnd) திரும்பு m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_தள்ளு(v, c_m3Type_f32); எனில் (r) திரும்பு r;
            நிறுத்து;
        }
        நிலை 0x44: { // f64.const
            எனில் (v->wasm + 8 > v->wasmEnd) திரும்பு m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_தள்ளு(v, c_m3Type_f64); எனில் (r) திரும்பு r;
            நிறுத்து;
        }


        // ---- i32 comparison ----
        நிலை 0x45: r = v_testop(v, c_m3Type_i32); நிறுத்து; // i32.eqz
        நிலை 0x46: நிலை 0x47: நிலை 0x48: நிலை 0x49: நிலை 0x4a:
        நிலை 0x4b: நிலை 0x4c: நிலை 0x4d: நிலை 0x4e: நிலை 0x4f:
            r = v_relop(v, c_m3Type_i32); நிறுத்து;

        // ---- i64 comparison ----
        நிலை 0x50: r = v_testop(v, c_m3Type_i64); நிறுத்து; // i64.eqz
        நிலை 0x51: நிலை 0x52: நிலை 0x53: நிலை 0x54: நிலை 0x55:
        நிலை 0x56: நிலை 0x57: நிலை 0x58: நிலை 0x59: நிலை 0x5a:
            r = v_relop(v, c_m3Type_i64); நிறுத்து;

        // ---- f32 comparison ----
        நிலை 0x5b: நிலை 0x5c: நிலை 0x5d: நிலை 0x5e: நிலை 0x5f: நிலை 0x60:
            r = v_relop(v, c_m3Type_f32); நிறுத்து;

        // ---- f64 comparison ----
        நிலை 0x61: நிலை 0x62: நிலை 0x63: நிலை 0x64: நிலை 0x65: நிலை 0x66:
            r = v_relop(v, c_m3Type_f64); நிறுத்து;

        // ---- i32 unary ----
        நிலை 0x67: நிலை 0x68: நிலை 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); நிறுத்து;

        // ---- i32 binary ----
        நிலை 0x6a: நிலை 0x6b: நிலை 0x6c: நிலை 0x6d: நிலை 0x6e: நிலை 0x6f:
        நிலை 0x70: நிலை 0x71: நிலை 0x72: நிலை 0x73: நிலை 0x74: நிலை 0x75:
        நிலை 0x76: நிலை 0x77: நிலை 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); நிறுத்து;

        // ---- i64 unary ----
        நிலை 0x79: நிலை 0x7a: நிலை 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); நிறுத்து;

        // ---- i64 binary ----
        நிலை 0x7c: நிலை 0x7d: நிலை 0x7e: நிலை 0x7f: நிலை 0x80: நிலை 0x81:
        நிலை 0x82: நிலை 0x83: நிலை 0x84: நிலை 0x85: நிலை 0x86: நிலை 0x87:
        நிலை 0x88: நிலை 0x89: நிலை 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); நிறுத்து;

        // ---- f32 unary ----
        நிலை 0x8b: நிலை 0x8c: நிலை 0x8d: நிலை 0x8e: நிலை 0x8f:
        நிலை 0x90: நிலை 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); நிறுத்து;

        // ---- f32 binary ----
        நிலை 0x92: நிலை 0x93: நிலை 0x94: நிலை 0x95: நிலை 0x96:
        நிலை 0x97: நிலை 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); நிறுத்து;

        // ---- f64 unary ----
        நிலை 0x99: நிலை 0x9a: நிலை 0x9b: நிலை 0x9c: நிலை 0x9d:
        நிலை 0x9e: நிலை 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); நிறுத்து;

        // ---- f64 binary ----
        நிலை 0xa0: நிலை 0xa1: நிலை 0xa2: நிலை 0xa3: நிலை 0xa4:
        நிலை 0xa5: நிலை 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); நிறுத்து;

        // ---- Conversions ----
        நிலை 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); நிறுத்து; // i32.wrap/i64
        நிலை 0xa8: நிலை 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); நிறுத்து;
        நிலை 0xaa: நிலை 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); நிறுத்து;
        நிலை 0xac: நிலை 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); நிறுத்து;
        நிலை 0xae: நிலை 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); நிறுத்து;
        நிலை 0xb0: நிலை 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); நிறுத்து;
        நிலை 0xb2: நிலை 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); நிறுத்து;
        நிலை 0xb4: நிலை 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); நிறுத்து;
        நிலை 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); நிறுத்து;
        நிலை 0xb7: நிலை 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); நிறுத்து;
        நிலை 0xb9: நிலை 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); நிறுத்து;
        நிலை 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); நிறுத்து;
        நிலை 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); நிறுத்து;
        நிலை 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); நிறுத்து;
        நிலை 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); நிறுத்து;
        நிலை 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); நிறுத்து;

        // ---- Sign-extension (MVP post) ----
        நிலை 0xc0: நிலை 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); நிறுத்து;
        நிலை 0xc2: நிலை 0xc3: நிலை 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); நிறுத்து;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        நிலை 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;
            தேர்வு (sub) {
            நிலை 0x00: நிலை 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); நிறுத்து;
            நிலை 0x02: நிலை 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); நிறுத்து;
            நிலை 0x04: நிலை 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); நிறுத்து;
            நிலை 0x06: நிலை 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); நிறுத்து;
            நிலை 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                எனில் (not v_has_memory_idx(v, memidx)) திரும்பு m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                எனில் (not v->module->has_Data_மொத்தம்) திரும்பு m3_Err_data_மொத்தம்_Required;
                எனில் (dataidx >= v->module->இலக்கம்_Data_Segments) திரும்பு m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_எடு_expect(v, c_m3Type_i32, &a); எனில் (r) திரும்பு r; // n
                r = v_எடு_expect(v, c_m3Type_i32, &a); எனில் (r) திரும்பு r; // src
                r = v_எடு_expect(v, v_memory_addrtype(v, memidx), &a); எனில் (r) திரும்பு r; // dst
                நிறுத்து;
            }
            நிலை 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                // needs the segment, but not a memory
                எனில் (not v->module->has_Data_மொத்தம்) திரும்பு m3_Err_data_மொத்தம்_Required;
                எனில் (dataidx >= v->module->இலக்கம்_Data_Segments) திரும்பு m3Err_unknownDataSegment;
                நிறுத்து;
            }
#எனில் d_m3HasRefTypes
            நிலை 0x0c: // table.init
            நிலை 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;

                எனில் (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    எனில் (a2 >= v->module->இலக்கம்_Tables) திரும்பு m3Err_unknownTable;
                    எனில் (a1 >= v->module->இலக்கம்_Element_Segments) திரும்பு m3Err_unknownElemSegment;
                    எனில் (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        திரும்பு m3Err_typeMismatch;
                } இல்லையெனில் {                                                    // dst, src
                    எனில் (a1 >= v->module->இலக்கம்_Tables) திரும்பு m3Err_unknownTable;
                    எனில் (a2 >= v->module->இலக்கம்_Tables) திரும்பு m3Err_unknownTable;
                    எனில் (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        திரும்பு m3Err_typeMismatch;
                }

                எனில் (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_எடு_expect(v, c_m3Type_i32, &a); எனில் (r) திரும்பு r;             // n
                    r = v_எடு_expect(v, c_m3Type_i32, &a); எனில் (r) திரும்பு r;             // s
                    r = v_எடு_expect(v, v_table_addrtype(v, a2), &a); எனில் (r) திரும்பு r;  // d
                } இல்லையெனில் {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_எடு_expect(v, nType, &a); எனில் (r) திரும்பு r;  // n
                    r = v_எடு_expect(v, sType, &a); எனில் (r) திரும்பு r;  // s
                    r = v_எடு_expect(v, dType, &a); எனில் (r) திரும்பு r;  // d
                }
                நிறுத்து;
            }
            நிலை 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                எனில் (elemIdx >= v->module->இலக்கம்_Element_Segments) திரும்பு m3Err_unknownElemSegment;
                நிறுத்து;
            }
            நிலை 0x0f: // table.grow
            நிலை 0x10: // table.size
            நிலை 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                எனில் (tableIdx >= v->module->இலக்கம்_Tables) திரும்பு m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                எனில் (sub == 0x10) {                                          // table.size
                    r = v_தள்ளு(v, at); எனில் (r) திரும்பு r;
                } இல்லையெனில் எனில் (sub == 0x0f) {                                   // table.grow
                    r = v_எடு_expect(v, at, &a); எனில் (r) திரும்பு r;  // n
                    r = v_எடு_expect(v, t, &a);  எனில் (r) திரும்பு r;  // init
                    r = v_தள்ளு(v, at);           எனில் (r) திரும்பு r;
                } இல்லையெனில் {                                                    // table.fill
                    r = v_எடு_expect(v, at, &a); எனில் (r) திரும்பு r;  // n
                    r = v_எடு_expect(v, t, &a);  எனில் (r) திரும்பு r;  // val
                    r = v_எடு_expect(v, at, &a); எனில் (r) திரும்பு r;  // i
                }
                நிறுத்து;
            }
#endif
            நிலை 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                எனில் (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) திரும்பு m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_எடு_expect(v, lenType, &a); எனில் (r) திரும்பு r; // n
                r = v_எடு_expect(v, srcType, &a); எனில் (r) திரும்பு r; // src
                r = v_எடு_expect(v, dstType, &a); எனில் (r) திரும்பு r; // dst
                நிறுத்து;
            }
            நிலை 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                எனில் (not v_has_memory_idx(v, memidx)) திரும்பு m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_எடு_expect(v, addrType, &a); எனில் (r) திரும்பு r;      // n
                r = v_எடு_expect(v, c_m3Type_i32, &a); எனில் (r) திரும்பு r;  // val
                r = v_எடு_expect(v, addrType, &a); எனில் (r) திரும்பு r;      // dst
                நிறுத்து;
            }
#எனில் d_m3HasWideArithmetic
            நிலை 0x13: நிலை 0x14: // i64.add128, i64.sub128
            நிலை 0x15: நிலை 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 இலக்கம்_Operands = (sub <= 0x14) ? 4 : 2;
                க்கு (u32 i = 0; i < இலக்கம்_Operands; ++i) {
                    r = v_எடு_expect(v, c_m3Type_i64, &a); எனில் (r) திரும்பு r;
                }
                r = v_தள்ளு(v, c_m3Type_i64); எனில் (r) திரும்பு r;
                r = v_தள்ளு(v, c_m3Type_i64); எனில் (r) திரும்பு r;
                நிறுத்து;
            }
#endif
            இயல்பு:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                நிறுத்து;
            }
            நிறுத்து;
        }

#எனில் d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        நிலை 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            எனில் (r) திரும்பு r;

            எனில் (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                எனில் (reserved != 0) திரும்பு m3Err_wasmMalformed;
                நிறுத்து;
            }

            u32 width, இலக்கம்_Operands; m3type_t மதிப்பு_Type, resultType;
            எனில் (not v_atomic_shape(sub, &width, &இலக்கம்_Operands, &மதிப்பு_Type, &resultType)) திரும்பு m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            எனில் ((1u << align) != width) திரும்பு m3Err_invalidAtomicAlignment;
            எனில் (not v_has_memory_idx(v, memidx)) திரும்பு m3Err_unknownMemory;
            எனில் (not v_offset_in_range(v, memidx, offset)) திரும்பு m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            எனில் (sub == 0x01 or sub == 0x02) {
                r = v_எடு_expect(v, c_m3Type_i64, &a); எனில் (r) திரும்பு r;
                இலக்கம்_Operands--;
            }
            க்கு (u32 i = 1; i < இலக்கம்_Operands; ++i) {
                r = v_எடு_expect(v, மதிப்பு_Type, &a); எனில் (r) திரும்பு r;
            }
            r = v_எடு_expect(v, v_memory_addrtype(v, memidx), &a); எனில் (r) திரும்பு r;
            எனில் (resultType != c_m3Type_none) {
                r = v_தள்ளு(v, resultType); எனில் (r) திரும்பு r;
            }
            நிறுத்து;
        }
#endif

#எனில் d_m3_Has_அடுக்கு_Switching
        // ---- Stack Switching ----
        நிலை 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            எனில் (!ft || !ft->isContinuation) திரும்பு m3Err_typeMismatch;
            r = v_எடு_expect(v, c_m3Type_funcref, &a); எனில் (r) திரும்பு r;
            r = v_தள்ளு(v, c_m3Type_contref); எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (typeIdx1 >= v->module->இலக்கம்_Func_Types || typeIdx2 >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            எனில் (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) திரும்பு m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 இலக்கம்_Params1 = ft1->இலக்கம்_Args;
            u16 இலக்கம்_Params2 = ft2->இலக்கம்_Args;
            எனில் (இலக்கம்_Params1 < இலக்கம்_Params2) திரும்பு m3Err_typeMismatch;
            u32 இலக்கம்_Bound = இலக்கம்_Params1 - இலக்கம்_Params2;

            எனில் (ft1->இலக்கம்_Rets != ft2->இலக்கம்_Rets) திரும்பு m3Err_typeMismatch;
            க்கு (u16 i = 0; i < ft1->இலக்கம்_Rets; ++i) {
                எனில் (ft1->types[i] != ft2->types[i]) திரும்பு m3Err_typeMismatch;
            }
            க்கு (u16 i = 0; i < இலக்கம்_Params2; ++i) {
                எனில் (ft1->types[ft1->இலக்கம்_Rets + இலக்கம்_Bound + i] != ft2->types[ft2->இலக்கம்_Rets + i]) திரும்பு m3Err_typeMismatch;
            }

            r = v_எடு_expect(v, c_m3Type_contref, &a); எனில் (r) திரும்பு r;

            க்கு (u32 i = இலக்கம்_Bound; i > 0; i--) {
                r = v_எடு_expect(v, BaseTypeOf(ft1->types[ft1->இலக்கம்_Rets + i - 1]), &a);
                எனில் (r) திரும்பு r;
            }

            r = v_தள்ளு(v, c_m3Type_contref); எனில் (r) திரும்பு r;
            நிறுத்து;
        }

        நிலை 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (!v->module || tagIdx >= v->module->இலக்கம்_Tags) திரும்பு m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            எனில் (tt) {
                க்கு (u16 i = tt->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(tt->types[tt->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
                க்கு (u16 i = 0; i < tt->இலக்கம்_Rets; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(tt->types[i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            எனில் (!ct || !ct->isContinuation) திரும்பு m3Err_typeMismatch;

            u32 இலக்கம்_Clauses;
            r = ReadLEB_u32(&இலக்கம்_Clauses, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;

            க்கு (u32 i = 0; i < இலக்கம்_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                r = v_resume_clause(v, kind, tagIdx, ct); எனில் (r) திரும்பு r;
            }

            r = v_எடு_expect(v, c_m3Type_contref, &a); எனில் (r) திரும்பு r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            எனில் (ft) {
                க்கு (u16 i = ft->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(ft->types[ft->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
                க்கு (u16 i = 0; i < ft->இலக்கம்_Rets; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(ft->types[i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, இலக்கம்_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            r = ReadLEB_u32(&இலக்கம்_Clauses, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;

            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            எனில் (!v->module || tagIdx >= v->module->இலக்கம்_Tags) திரும்பு m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            எனில் (!ct || !ct->isContinuation) திரும்பு m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            எனில் (tagType && tagType->இலக்கம்_Rets != 0) திரும்பு m3Err_typeMismatch;

            க்கு (u32 i = 0; i < இலக்கம்_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                r = v_resume_clause(v, kind, cTagIdx, ct); எனில் (r) திரும்பு r;
            }

            r = v_எடு_expect(v, c_m3Type_contref, &a); எனில் (r) திரும்பு r;

            எனில் (tagType) {
                க்கு (u16 i = tagType->இலக்கம்_Args; i > 0; i--) {
                    r = v_எடு_expect(v, BaseTypeOf(tagType->types[tagType->இலக்கம்_Rets + i - 1]), &a);
                    எனில் (r) திரும்பு r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            எனில் (ft) {
                க்கு (u16 i = 0; i < ft->இலக்கம்_Rets; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(ft->types[i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0xe5: // resume_throw_ref
        {
            u32 typeIdx, இலக்கம்_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            r = ReadLEB_u32(&இலக்கம்_Clauses, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;

            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            எனில் (!ct || !ct->isContinuation) திரும்பு m3Err_typeMismatch;

            க்கு (u32 i = 0; i < இலக்கம்_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
                r = v_resume_clause(v, kind, cTagIdx, ct); எனில் (r) திரும்பு r;
            }

            r = v_எடு_expect(v, c_m3Type_contref, &a); எனில் (r) திரும்பு r;
            r = v_எடு_expect(v, c_m3Type_exnref, &a); எனில் (r) திரும்பு r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            எனில் (ft) {
                க்கு (u16 i = 0; i < ft->இலக்கம்_Rets; i++) {
                    r = v_தள்ளு(v, BaseTypeOf(ft->types[i]));
                    எனில் (r) திரும்பு r;
                }
            }
            நிறுத்து;
        }

        நிலை 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); எனில் (r) திரும்பு r;

            எனில் (typeIdx >= v->module->இலக்கம்_Func_Types) திரும்பு m3Err_unknownType;
            எனில் (!v->module || tagIdx >= v->module->இலக்கம்_Tags) திரும்பு m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            எனில் (!ct || !ct->isContinuation) திரும்பு m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            எனில் (tagType && tagType->இலக்கம்_Args != 0) திரும்பு m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            எனில் (!ft || ft->இலக்கம்_Args == 0) திரும்பு m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->இலக்கம்_Rets + ft->இலக்கம்_Args - 1]);
            எனில் (!peer || !peer->contFuncType) திரும்பு m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_எடு_expect(v, c_m3Type_contref, &a); எனில் (r) திரும்பு r;

            க்கு (u16 i = (u16)(ft->இலக்கம்_Args - 1); i > 0; i--) {
                r = v_எடு_expect(v, BaseTypeOf(ft->types[ft->இலக்கம்_Rets + i - 1]), &a);
                எனில் (r) திரும்பு r;
            }

            க்கு (u16 i = 0; i < self->இலக்கம்_Args; i++) {
                r = v_தள்ளு(v, BaseTypeOf(self->types[self->இலக்கம்_Rets + i]));
                எனில் (r) திரும்பு r;
            }
            நிறுத்து;
        }

#endif // d_m3HasStackSwitching

        இயல்பு:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            நிறுத்து;

        } // switch

        எனில் (r) திரும்பு r;

    } // while

    // If we ran out of bytes without hitting the final end
    திரும்பு m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    எனில் (!i_function->wasm) திரும்பு m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    எனில் (!v) {
        v = m3_AllocStruct (ValCtx);
        எனில் (!v) திரும்பு m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, அளவு(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    எனில் (r) திரும்பு r;

    // Parse locals
    u32 இலக்கம்_Local_Blocks;
    r = ReadLEB_u32(&இலக்கம்_Local_Blocks, &v->wasm, v->wasmEnd);
    எனில் (r) திரும்பு r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 இலக்கம்_Params = funcType ? funcType->இலக்கம்_Args : 0;
    எனில் (இலக்கம்_Params > d_m3_Val_அடுக்கு) திரும்பு m3_Err_function_அடுக்கு_Overflow;
    க்கு (u16 i = 0; i < இலக்கம்_Params; i++) {
        v->localTypes[v->இலக்கம்_Locals++] = BaseTypeOf(funcType->types[funcType->இலக்கம்_Rets + i]);
    }

    // Then: declared locals
    க்கு (u32 b = 0; b < இலக்கம்_Local_Blocks; b++) {
        u32 மொத்தம்;
        r = ReadLEB_u32(&மொத்தம், &v->wasm, v->wasmEnd);
        எனில் (r) திரும்பு r;
        m3type_t localType;
        r = Parse_மதிப்பு_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        எனில் (r) திரும்பு r;
        u8 normalized = BaseTypeOf(localType);
        எனில் (மொத்தம் > (u32) (d_m3_Val_அடுக்கு - v->இலக்கம்_Locals)) திரும்பு m3_Err_function_அடுக்கு_Overflow;
        க்கு (u32 c = 0; c < மொத்தம்; c++) {
            v->localTypes[v->இலக்கம்_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_தள்ளு_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    எனில் (r) திரும்பு r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    எனில் (r) திரும்பு r;

    // After validation, control stack should be empty and all declared bytes consumed
    எனில் (v->ctrl_உச்சி != 0)
        திரும்பு m3Err_wasmMalformed;

    எனில் (v->wasm != v->wasmEnd)
        திரும்பு m3Err_wasmSectionUnderrun;

    திரும்பு m3Err_none;
}

#இல்லையெனில் // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (வெறுமை)i_function;
    திரும்பு m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
