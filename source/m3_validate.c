//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#شامل "m3_validate.h"
#شامل "m3_exception.h"
#شامل "m3_info.h"

#اگر d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

تعریفنوع ساختار {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_شمار;
    u16         result_شمار;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

ساختار ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_پشته];
    u16         opd_بالاترین;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_بالاترین;

    m3type_t    localTypes [d_m3_Val_پشته];
    u16         رقم_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ایستا bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    بازگشت v->module and i_memoryIdx < v->module->رقم_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ایستا m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    بازگشت Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ایستا bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    بازگشت v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ایستا m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    بازگشت Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
ایستا u32 v_max_align (m3opcode_t opcode)
{
    انتخاب (opcode) {
    حالت 0x2c: حالت 0x2d:   // i32.load8_s, i32.load8_u
    حالت 0x30: حالت 0x31:   // i64.load8_s, i64.load8_u
    حالت 0x3a:              // i32.store8
    حالت 0x3c:              // i64.store8
        بازگشت 0;
    حالت 0x2e: حالت 0x2f:   // i32.load16_s, i32.load16_u
    حالت 0x32: حالت 0x33:   // i64.load16_s, i64.load16_u
    حالت 0x3b:              // i32.store16
    حالت 0x3d:              // i64.store16
        بازگشت 1;
    حالت 0x29:              // i64.load
    حالت 0x2b:              // f64.load
    حالت 0x37:              // i64.store
    حالت 0x39:              // f64.store
        بازگشت 3;
    پیشفرض:                // 32-bit accesses, and a safe fallback
        بازگشت 2;
    }
}

#اگر d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
ایستا bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_رقم_Operands, m3type_t * o_مقدار_Type, m3type_t * o_resultType)
{
    ایستا ثابت u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ایستا ثابت m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    اگر (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_رقم_Operands = (sub == 0x00) ? 2 : 3;
        *o_مقدار_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        بازگشت true;
    }

    اگر (sub < 0x10 or sub > c_waOp_lastAtomic) بازگشت false;

    اگر (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_رقم_Operands = 1;
    } وگرنه اگر (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_رقم_Operands = 2;
    } وگرنه اگر (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_رقم_Operands = 2;
    } وگرنه {                        // cmpxchg
        variant = sub - 0x48;       *o_رقم_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_مقدار_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    بازگشت true;
}

#endif

// ---------- Operand stack ----------

ایستا M3Result v_افزودن (ValCtx * v, m3type_t type)
{
    اگر (M3_UNLIKELY(v->opd_بالاترین >= d_m3_Val_پشته))
        بازگشت m3_Err_function_پشته_Overflow;
    v->opd[v->opd_بالاترین++] = type;
    بازگشت m3Err_none;
}

ایستا M3Result v_برداشت (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_بالاترین - 1];
    اگر (M3_UNLIKELY(v->opd_بالاترین == f->height)) {
        اگر (f->is_unreachable) { *o_type = c_valBottom; بازگشت m3Err_none; }
        بازگشت m3_Err_function_پشته_Underrun;
    }
    *o_type = v->opd[--v->opd_بالاترین];
    بازگشت m3Err_none;
}

ایستا M3Result v_برداشت_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_برداشت(v, &actual);
    اگر (M3_UNLIKELY(r)) بازگشت r;
    اگر (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        بازگشت m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    بازگشت m3Err_none;
}

// ---------- Control stack ----------

ایستا M3Result v_افزودن_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    اگر (M3_UNLIKELY(v->ctrl_بالاترین >= d_m3ValCtrlDepth))
        بازگشت m3_Err_function_پشته_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_بالاترین++];
    f->opcode       = op;
    f->type         = type;
    f->param_شمار  = type ? type->رقم_Args : 0;
    f->result_شمار = type ? type->رقم_Rets : 0;
    f->height       = v->opd_بالاترین;
    f->is_unreachable = false;
    بازگشت m3Err_none;
}

ایستا M3Result v_برداشت_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    اگر (M3_UNLIKELY(v->ctrl_بالاترین == 0))
        بازگشت m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_بالاترین - 1];
    // pop result types
    اگر (f->type) {
        برای (u16 i = f->result_شمار; i > 0; i--) {
            m3type_t a;
            M3Result r = v_برداشت_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            اگر (M3_UNLIKELY(r)) بازگشت r;
        }
    }
    اگر (M3_UNLIKELY(v->opd_بالاترین != f->height))
        بازگشت m3_Err_type_شمار_Mismatch;
    اگر (o_frame) *o_frame = *f;
    v->ctrl_بالاترین--;
    بازگشت m3Err_none;
}

ایستا تهی v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_بالاترین - 1];
    v->opd_بالاترین = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ایستا u16 v_label_n (ValCtrlFrame * f)
{
    بازگشت (f->opcode == 0x03) ? f->param_شمار : f->result_شمار;
}

ایستا m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    اگر (!f->type) بازگشت c_m3Type_none;
    اگر (f->opcode == 0x03)
        بازگشت BaseTypeOf(f->type->types[f->type->رقم_Rets + i]); // params
    بازگشت BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ایستا m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_بالاترین - 1];

    اگر (v->opd_بالاترین <= i_depth || (u16)(v->opd_بالاترین - i_depth - 1) < f->height)
        بازگشت c_valBottom;

    بازگشت v->opd[v->opd_بالاترین - i_depth - 1];
}

// Pop label types for branch target
ایستا M3Result v_برداشت_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    برای (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_برداشت_expect(v, v_label_t(tgt, i - 1), &a);
        اگر (M3_UNLIKELY(r)) بازگشت r;
    }
    بازگشت m3Err_none;
}

// Push label types back
ایستا M3Result v_افزودن_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    برای (u16 i = 0; i < n; i++) {
        M3Result r = v_افزودن(v, v_label_t(tgt, i));
        اگر (M3_UNLIKELY(r)) بازگشت r;
    }
    بازگشت m3Err_none;
}

// ---------- Block type resolution ----------

ایستا M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    اگر (v->wasm >= v->wasmEnd)
        بازگشت m3Err_wasmUnderrun;

#اگر d_m3HasTypedRefs
    اگر (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_مقدار_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        اگر (rr) بازگشت rr;
        IM3FuncType ftype = پوچ;
        rr = AllocFuncType(&ftype, 1);
        اگر (rr) بازگشت rr;
        ftype->رقم_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        اگر (rr) بازگشت rr;
        *o_type = ftype;
        بازگشت m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    اگر (r) بازگشت r;

    اگر (type < 0) {
        اگر (type < -64) بازگشت m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        اگر (r) بازگشت r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } وگرنه {
        اگر ((u32)type >= v->module->رقم_Func_Types)
            بازگشت m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    بازگشت m3Err_none;
}

// ---------- Convenience ----------

ایستا M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_برداشت_expect(v, in, &a);
    اگر (M3_UNLIKELY(r)) بازگشت r;
    بازگشت v_افزودن(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ایستا M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : پوچ;

    u16 رقم_Callee_Rets = i_calleeType ? i_calleeType->رقم_Rets : 0;
    u16 رقم_Func_Rets   = ft ? ft->رقم_Rets : 0;

    اگر (رقم_Callee_Rets != رقم_Func_Rets)
        بازگشت m3Err_typeMismatch;

    برای (u16 i = 0; i < رقم_Callee_Rets; i++) {
        اگر (i_calleeType->types[i] != ft->types[i])
            بازگشت m3Err_typeMismatch;
    }

    بازگشت m3Err_none;
}

ایستا M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_برداشت_expect(v, t, &a); اگر (M3_UNLIKELY(r)) بازگشت r;
    r = v_برداشت_expect(v, t, &a); اگر (M3_UNLIKELY(r)) بازگشت r;
    بازگشت v_افزودن(v, t);
}

ایستا M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_برداشت_expect(v, t, &a); اگر (M3_UNLIKELY(r)) بازگشت r;
    r = v_برداشت_expect(v, t, &a); اگر (M3_UNLIKELY(r)) بازگشت r;
    بازگشت v_افزودن(v, c_m3Type_i32);
}

ایستا M3Result v_testop (ValCtx * v, m3type_t t)
{
    بازگشت v_unop(v, t, c_m3Type_i32);
}

ایستا M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    بازگشت v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#اگر d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ایستا M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    اگر (r) بازگشت r;
    اگر (kind > 0x03) بازگشت m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = پوچ;
    اگر (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        اگر (r) بازگشت r;
        اگر (!v->module || tagIndex >= v->module->رقم_Tags) بازگشت m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    اگر (r) بازگشت r;
    اگر (depth >= v->ctrl_بالاترین) بازگشت m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_بالاترین - 1 - depth];

    u16 رقم_Payload = tagType ? tagType->رقم_Args : 0;
    u16 رقم_Label   = v_label_n(tgt);

    اگر (رقم_Label != رقم_Payload + (hasRef ? 1u : 0u))
        بازگشت m3_Err_type_شمار_Mismatch;

    برای (u16 i = 0; i < رقم_Payload; i++) {
        اگر (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->رقم_Rets + i]))
            بازگشت m3Err_typeMismatch;
    }

    اگر (hasRef and v_label_t(tgt, رقم_Payload) != c_m3Type_exnref)
        بازگشت m3Err_typeMismatch;

    بازگشت m3Err_none;
}

#endif // d_m3HasExceptionHandling

#اگر d_m3_Has_پشته_Switching

ایستا M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    اگر (kind > 0x01) بازگشت m3Err_wasmMalformed;
    اگر (!v->module || tagIdx >= v->module->رقم_Tags) بازگشت m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    اگر (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
        اگر (depth >= v->ctrl_بالاترین) بازگشت m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_بالاترین - 1 - depth];
        u16 رقم_Payload = tagType ? tagType->رقم_Args : 0;
        u16 رقم_Label   = v_label_n(tgt);

        اگر (رقم_Label != رقم_Payload + 1u) بازگشت m3_Err_type_شمار_Mismatch;

        برای (u16 j = 0; j < رقم_Payload; j++) {
            اگر (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->رقم_Rets + j]))
                بازگشت m3Err_typeMismatch;
        }
        اگر (v_label_t(tgt, رقم_Payload) != c_m3Type_contref)
            بازگشت m3Err_typeMismatch;

#اگر d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->رقم_Rets + رقم_Payload]
            : tgt->type->types[رقم_Payload];

        اگر (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            بازگشت m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = پوچ;
        برای (u32 idx = 0; idx < v->module->رقم_Func_Types; ++idx) {
            اگر (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                بشکن;
            }
        }
        اگر (!labelCt) بازگشت m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_رقم_Params = tagType ? tagType->رقم_Rets : 0;
        اگر (labelFt->رقم_Args != expected_رقم_Params) بازگشت m3Err_typeMismatch;
        برای (u16 p = 0; p < expected_رقم_Params; ++p) {
            اگر (labelFt->types[labelFt->رقم_Rets + p] != tagType->types[p])
                بازگشت m3Err_typeMismatch;
        }

        u16 expected_رقم_Rets = ft ? ft->رقم_Rets : 0;
        اگر (labelFt->رقم_Rets != expected_رقم_Rets) بازگشت m3Err_typeMismatch;
        برای (u16 r_idx = 0; r_idx < expected_رقم_Rets; ++r_idx) {
            اگر (labelFt->types[r_idx] != ft->types[r_idx])
                بازگشت m3Err_typeMismatch;
        }
#endif
    } وگرنه {
        اگر (tagType && tagType->رقم_Args != 0) بازگشت m3Err_typeMismatch;
    }
    بازگشت m3Err_none;
}

#endif // d_m3HasStackSwitching

ایستا M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    وقتی (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        اگر (M3_UNLIKELY(r)) بازگشت r;

        انتخاب (opcode)
        {
        // ---- Control ----
        حالت 0x00: // unreachable
            v_unreachable(v);
            بشکن;

        حالت 0x01: // nop
            بشکن;

        حالت 0x02: // block
        حالت 0x03: // loop
        حالت 0x04: // if
        {
            IM3FuncType bt = پوچ;
            r = v_read_blocktype(v, &bt);
            اگر (r) بازگشت r;
            اگر (opcode == 0x04) {
                r = v_برداشت_expect(v, c_m3Type_i32, &a);
                اگر (r) بازگشت r;
            }
            // Pop block params from caller stack
            اگر (bt) {
                برای (u16 i = bt->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(bt->types[bt->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
            }
            r = v_افزودن_ctrl(v, opcode, bt);
            اگر (r) بازگشت r;
            // Push params inside block
            اگر (bt) {
                برای (u16 i = 0; i < bt->رقم_Args; i++) {
                    r = v_افزودن(v, BaseTypeOf(bt->types[bt->رقم_Rets + i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

#اگر d_m3HasExceptionHandling
        حالت 0x1f: // try_table
        {
            IM3FuncType bt = پوچ;
            r = v_read_blocktype(v, &bt);
            اگر (r) بازگشت r;
            اگر (bt) {
                برای (u16 i = bt->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(bt->types[bt->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 رقم_Catch;
            r = ReadLEB_u32(&رقم_Catch, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            برای (u32 i = 0; i < رقم_Catch; i++) {
                r = v_catch_clause(v);
                اگر (r) بازگشت r;
            }

            r = v_افزودن_ctrl(v, opcode, bt);
            اگر (r) بازگشت r;

            اگر (bt) {
                برای (u16 i = 0; i < bt->رقم_Args; i++) {
                    r = v_افزودن(v, BaseTypeOf(bt->types[bt->رقم_Rets + i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (!v->module || tagIndex >= v->module->رقم_Tags) بازگشت m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            اگر (tt) {
                برای (u16 i = tt->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(tt->types[tt->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
            }
            v_unreachable(v);
            بشکن;
        }

        حالت 0x0a: // throw_ref
            r = v_برداشت_expect(v, c_m3Type_exnref, &a);
            اگر (r) بازگشت r;
            v_unreachable(v);
            بشکن;
#endif // d_m3HasExceptionHandling

        حالت 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_برداشت_ctrl(v, &frame);
            اگر (r) بازگشت r;
            اگر (frame.opcode != 0x04)
                بازگشت m3Err_wasmMalformed;
            r = v_افزودن_ctrl(v, 0x05, frame.type);
            اگر (r) بازگشت r;
            اگر (frame.type) {
                برای (u16 i = 0; i < frame.type->رقم_Args; i++) {
                    r = v_افزودن(v, BaseTypeOf(frame.type->types[frame.type->رقم_Rets + i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_برداشت_ctrl(v, &frame);
            اگر (r) بازگشت r;
            // Push results
            اگر (frame.type) {
                برای (u16 i = 0; i < frame.result_شمار; i++) {
                    r = v_افزودن(v, BaseTypeOf(frame.type->types[i]));
                    اگر (r) بازگشت r;
                }
            }
            // If this was the outermost frame, we're done
            اگر (v->ctrl_بالاترین == 0)
                بازگشت m3Err_none;
            بشکن;
        }

        حالت 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (depth >= v->ctrl_بالاترین) بازگشت m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_بالاترین - 1 - depth];
            r = v_برداشت_labels(v, tgt);
            اگر (r) بازگشت r;
            v_unreachable(v);
            بشکن;
        }

        حالت 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (depth >= v->ctrl_بالاترین) بازگشت m3Err_unknownLabel;
            r = v_برداشت_expect(v, c_m3Type_i32, &a);
            اگر (r) بازگشت r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_بالاترین - 1 - depth];
            r = v_برداشت_labels(v, tgt);
            اگر (r) بازگشت r;
            r = v_افزودن_labels(v, tgt);
            اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0x0e: // br_table
        {
            u32 شمار;
            r = ReadLEB_u32(&شمار, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            برای (u32 i = 0; i <= شمار; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                اگر (r) بازگشت r;
                اگر (d >= v->ctrl_بالاترین) بازگشت m3Err_unknownLabel;
                اگر (i == شمار) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_بالاترین - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_برداشت_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            اگر (r) بازگشت r;

            برای (u32 i = 0; i <= شمار; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                اگر (r) بازگشت r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_بالاترین - 1 - d];
                u16 n = v_label_n(t);
                اگر (n != arity) بازگشت m3_Err_type_شمار_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                برای (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    اگر (want != c_valBottom && have != c_valBottom && want != have)
                        بازگشت m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_بالاترین - 1 - defDepth];
            r = v_برداشت_labels(v, dt);
            اگر (r) بازگشت r;
            v_unreachable(v);
            بشکن;
        }

        حالت 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            اگر (ft) {
                برای (u16 i = ft->رقم_Rets; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    اگر (r) بازگشت r;
                }
            }
            v_unreachable(v);
            بشکن;
        }

        // ---- Call ----
        حالت 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (idx >= v->module->رقم_Functions) بازگشت m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            اگر (ft) {
                برای (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
                برای (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_افزودن(v, BaseTypeOf(ft->types[i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            // the table must exist and hold funcrefs
            اگر (tableIdx >= v->module->رقم_Tables) بازگشت m3Err_unknownTable;
            اگر (v->module->tables[tableIdx]->type != c_m3Type_funcref) بازگشت m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_برداشت_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            اگر (r) بازگشت r;
            اگر (ft) {
                برای (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
                برای (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_افزودن(v, BaseTypeOf(ft->types[i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (idx >= v->module->رقم_Functions) بازگشت m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            اگر (r) بازگشت r;
            اگر (ft) {
                برای (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
            }
            v_unreachable(v);
            بشکن;
        }

        حالت 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            // the table must exist and hold funcrefs
            اگر (tableIdx >= v->module->رقم_Tables) بازگشت m3Err_unknownTable;
            اگر (v->module->tables[tableIdx]->type != c_m3Type_funcref) بازگشت m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            اگر (r) بازگشت r;
            r = v_برداشت_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            اگر (r) بازگشت r;
            اگر (ft) {
                برای (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
            }
            v_unreachable(v);
            بشکن;
        }

#اگر d_m3HasTypedRefs
        حالت 0x14: // call_ref
        حالت 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            اگر (isTail) {
                r = v_check_tail_results(v, ft);
                اگر (r) بازگشت r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_برداشت_expect(v, c_m3Type_funcref, &a);
            اگر (r) بازگشت r;
            اگر (ft) {
                برای (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
                اگر (not isTail) {
                    برای (u16 i = 0; i < ft->رقم_Rets; i++) {
                        r = v_افزودن(v, BaseTypeOf(ft->types[i]));
                        اگر (r) بازگشت r;
                    }
                }
            }
            اگر (isTail) v_unreachable(v);
            بشکن;
        }

        حالت 0xd4: // ref.as_non_null
        {
            r = v_برداشت_expect(v, c_m3Type_funcref, &a);
            اگر (r) بازگشت r;
            r = v_افزودن(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            اگر (r) بازگشت r;
            بشکن;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        حالت 0x1a: // drop
            r = v_برداشت(v, &a);
            اگر (r) بازگشت r;
            بشکن;

        حالت 0x1b: // select
        {
            r = v_برداشت_expect(v, c_m3Type_i32, &a);
            اگر (r) بازگشت r;
            m3type_t t2 = c_valBottom;
            r = v_برداشت(v, &t2);
            اگر (r) بازگشت r;
            m3type_t t1;
            r = v_برداشت_expect(v, t2, &t1);
            اگر (r) بازگشت r;
            // untyped select is numeric only; references need the 0x1c form
            اگر (t2 != c_valBottom && IsRefType(t2)) بازگشت m3Err_typeMismatch;
            r = v_افزودن(v, (t2 == c_valBottom) ? t1 : t2);
            اگر (r) بازگشت r;
            بشکن;
        }

#اگر d_m3HasRefTypes
        حالت 0x1c: // select with an explicit result type
        {
            u32 رقم_Types;
            r = ReadLEB_u32(&رقم_Types, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (رقم_Types != 1) بازگشت m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_مقدار_Type(v->module, &selType, &v->wasm, v->wasmEnd);  اگر (r) بازگشت r;
            u8 t = BaseTypeOf(selType);

            r = v_برداشت_expect(v, c_m3Type_i32, &a); اگر (r) بازگشت r;
            r = v_برداشت_expect(v, t, &a);            اگر (r) بازگشت r;
            r = v_برداشت_expect(v, t, &a);            اگر (r) بازگشت r;
            r = v_افزودن(v, t);                      اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0x25: // table.get
        حالت 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (tableIdx >= v->module->رقم_Tables) بازگشت m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            اگر (opcode == 0x26) {
                r = v_برداشت_expect(v, t, &a);   اگر (r) بازگشت r;
                r = v_برداشت_expect(v, at, &a);  اگر (r) بازگشت r;
            } وگرنه {
                r = v_برداشت_expect(v, at, &a);  اگر (r) بازگشت r;
                r = v_افزودن(v, t);             اگر (r) بازگشت r;
            }
            بشکن;
        }

        حالت 0xd0: // ref.null
        {
#اگر d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  اگر (r) بازگشت r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# اگر d_m3_Has_پشته_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#وگرنه
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   اگر (r) بازگشت r;
            r = NormalizeType(&t, waType);                   اگر (r) بازگشت r;
            اگر (!IsRefType(t)) بازگشت m3Err_wasmMalformed;
#endif
            r = v_افزودن(v, t);                                اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_برداشت(v, &t);
            اگر (r) بازگشت r;
            اگر (t != c_valBottom && !IsRefType(t)) بازگشت m3Err_typeMismatch;
            r = v_افزودن(v, c_m3Type_i32);
            اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (funcIdx >= v->module->رقم_Functions) بازگشت m3Err_unknownFunction;
            اگر (!Module_IsFunctionDeclared(v->module, funcIdx)) بازگشت m3Err_undeclaredFuncRef;
            r = v_افزودن(v, c_m3Type_funcref);
            اگر (r) بازگشت r;
            بشکن;
        }
#endif

        // ---- Variable ----
        حالت 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (idx >= v->رقم_Locals) بازگشت m3Err_unknownLocal;
            r = v_افزودن(v, v->localTypes[idx]);
            اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (idx >= v->رقم_Locals) بازگشت m3Err_unknownLocal;
            r = v_برداشت_expect(v, v->localTypes[idx], &a);
            اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (idx >= v->رقم_Locals) بازگشت m3Err_unknownLocal;
            r = v_برداشت_expect(v, v->localTypes[idx], &a);
            اگر (r) بازگشت r;
            r = v_افزودن(v, v->localTypes[idx]);
            اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (idx >= v->module->رقم_Globals) بازگشت m3Err_unknownGlobal;
            r = v_افزودن(v, BaseTypeOf(v->module->globals[idx].type));
            اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            اگر (idx >= v->module->رقم_Globals) بازگشت m3Err_unknownGlobal;
            r = v_برداشت_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            اگر (r) بازگشت r;
            بشکن;
        }

        // ---- Memory load ----
        حالت 0x28: حالت 0x29: حالت 0x2a: حالت 0x2b: // i32/i64/f32/f64.load
        حالت 0x2c: حالت 0x2d: حالت 0x2e: حالت 0x2f: // i32.load8/16 s/u
        حالت 0x30: حالت 0x31: حالت 0x32: حالت 0x33: // i64.load8/16 s/u
        حالت 0x34: حالت 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (align > v_max_align(opcode)) بازگشت m3Err_invalidAlignment;
            اگر (not v_has_memory_idx(v, memidx)) بازگشت m3Err_unknownMemory;
            اگر (not v_offset_in_range(v, memidx, offset)) بازگشت m3Err_wasmMalformed;
            r = v_برداشت_expect(v, v_memory_addrtype(v, memidx), &a); اگر (r) بازگشت r;
            u8 result;
            اگر      (opcode == 0x28) result = c_m3Type_i32;
            وگرنه اگر (opcode == 0x29) result = c_m3Type_i64;
            وگرنه اگر (opcode == 0x2a) result = c_m3Type_f32;
            وگرنه اگر (opcode == 0x2b) result = c_m3Type_f64;
            وگرنه اگر (opcode <= 0x2f) result = c_m3Type_i32;
            وگرنه                     result = c_m3Type_i64;
            r = v_افزودن(v, result);
            اگر (r) بازگشت r;
            بشکن;
        }

        // ---- Memory store ----
        حالت 0x36: حالت 0x37: حالت 0x38: حالت 0x39: // i32/i64/f32/f64.store
        حالت 0x3a: حالت 0x3b:                         // i32.store8/16
        حالت 0x3c: حالت 0x3d: حالت 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (align > v_max_align(opcode)) بازگشت m3Err_invalidAlignment;
            اگر (not v_has_memory_idx(v, memidx)) بازگشت m3Err_unknownMemory;
            اگر (not v_offset_in_range(v, memidx, offset)) بازگشت m3Err_wasmMalformed;
            u8 valtype;
            اگر      (opcode == 0x36) valtype = c_m3Type_i32;
            وگرنه اگر (opcode == 0x37) valtype = c_m3Type_i64;
            وگرنه اگر (opcode == 0x38) valtype = c_m3Type_f32;
            وگرنه اگر (opcode == 0x39) valtype = c_m3Type_f64;
            وگرنه اگر (opcode <= 0x3b) valtype = c_m3Type_i32;
            وگرنه                     valtype = c_m3Type_i64;
            r = v_برداشت_expect(v, valtype, &a); اگر (r) بازگشت r;
            r = v_برداشت_expect(v, v_memory_addrtype(v, memidx), &a); اگر (r) بازگشت r;
            بشکن;
        }

        // ---- Memory size/grow ----
        حالت 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (not v_has_memory_idx(v, memidx)) بازگشت m3Err_unknownMemory;
            r = v_افزودن(v, v_memory_addrtype(v, memidx)); اگر (r) بازگشت r;
            بشکن;
        }
        حالت 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (not v_has_memory_idx(v, memidx)) بازگشت m3Err_unknownMemory;
            r = v_برداشت_expect(v, v_memory_addrtype(v, memidx), &a); اگر (r) بازگشت r;
            r = v_افزودن(v, v_memory_addrtype(v, memidx)); اگر (r) بازگشت r;
            بشکن;
        }

        // ---- Constants ----
        حالت 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            r = v_افزودن(v, c_m3Type_i32); اگر (r) بازگشت r;
            بشکن;
        }
        حالت 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            r = v_افزودن(v, c_m3Type_i64); اگر (r) بازگشت r;
            بشکن;
        }
        حالت 0x43: { // f32.const
            اگر (v->wasm + 4 > v->wasmEnd) بازگشت m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_افزودن(v, c_m3Type_f32); اگر (r) بازگشت r;
            بشکن;
        }
        حالت 0x44: { // f64.const
            اگر (v->wasm + 8 > v->wasmEnd) بازگشت m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_افزودن(v, c_m3Type_f64); اگر (r) بازگشت r;
            بشکن;
        }


        // ---- i32 comparison ----
        حالت 0x45: r = v_testop(v, c_m3Type_i32); بشکن; // i32.eqz
        حالت 0x46: حالت 0x47: حالت 0x48: حالت 0x49: حالت 0x4a:
        حالت 0x4b: حالت 0x4c: حالت 0x4d: حالت 0x4e: حالت 0x4f:
            r = v_relop(v, c_m3Type_i32); بشکن;

        // ---- i64 comparison ----
        حالت 0x50: r = v_testop(v, c_m3Type_i64); بشکن; // i64.eqz
        حالت 0x51: حالت 0x52: حالت 0x53: حالت 0x54: حالت 0x55:
        حالت 0x56: حالت 0x57: حالت 0x58: حالت 0x59: حالت 0x5a:
            r = v_relop(v, c_m3Type_i64); بشکن;

        // ---- f32 comparison ----
        حالت 0x5b: حالت 0x5c: حالت 0x5d: حالت 0x5e: حالت 0x5f: حالت 0x60:
            r = v_relop(v, c_m3Type_f32); بشکن;

        // ---- f64 comparison ----
        حالت 0x61: حالت 0x62: حالت 0x63: حالت 0x64: حالت 0x65: حالت 0x66:
            r = v_relop(v, c_m3Type_f64); بشکن;

        // ---- i32 unary ----
        حالت 0x67: حالت 0x68: حالت 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); بشکن;

        // ---- i32 binary ----
        حالت 0x6a: حالت 0x6b: حالت 0x6c: حالت 0x6d: حالت 0x6e: حالت 0x6f:
        حالت 0x70: حالت 0x71: حالت 0x72: حالت 0x73: حالت 0x74: حالت 0x75:
        حالت 0x76: حالت 0x77: حالت 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); بشکن;

        // ---- i64 unary ----
        حالت 0x79: حالت 0x7a: حالت 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); بشکن;

        // ---- i64 binary ----
        حالت 0x7c: حالت 0x7d: حالت 0x7e: حالت 0x7f: حالت 0x80: حالت 0x81:
        حالت 0x82: حالت 0x83: حالت 0x84: حالت 0x85: حالت 0x86: حالت 0x87:
        حالت 0x88: حالت 0x89: حالت 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); بشکن;

        // ---- f32 unary ----
        حالت 0x8b: حالت 0x8c: حالت 0x8d: حالت 0x8e: حالت 0x8f:
        حالت 0x90: حالت 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); بشکن;

        // ---- f32 binary ----
        حالت 0x92: حالت 0x93: حالت 0x94: حالت 0x95: حالت 0x96:
        حالت 0x97: حالت 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); بشکن;

        // ---- f64 unary ----
        حالت 0x99: حالت 0x9a: حالت 0x9b: حالت 0x9c: حالت 0x9d:
        حالت 0x9e: حالت 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); بشکن;

        // ---- f64 binary ----
        حالت 0xa0: حالت 0xa1: حالت 0xa2: حالت 0xa3: حالت 0xa4:
        حالت 0xa5: حالت 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); بشکن;

        // ---- Conversions ----
        حالت 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); بشکن; // i32.wrap/i64
        حالت 0xa8: حالت 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); بشکن;
        حالت 0xaa: حالت 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); بشکن;
        حالت 0xac: حالت 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); بشکن;
        حالت 0xae: حالت 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); بشکن;
        حالت 0xb0: حالت 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); بشکن;
        حالت 0xb2: حالت 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); بشکن;
        حالت 0xb4: حالت 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); بشکن;
        حالت 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); بشکن;
        حالت 0xb7: حالت 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); بشکن;
        حالت 0xb9: حالت 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); بشکن;
        حالت 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); بشکن;
        حالت 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); بشکن;
        حالت 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); بشکن;
        حالت 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); بشکن;
        حالت 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); بشکن;

        // ---- Sign-extension (MVP post) ----
        حالت 0xc0: حالت 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); بشکن;
        حالت 0xc2: حالت 0xc3: حالت 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); بشکن;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        حالت 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;
            انتخاب (sub) {
            حالت 0x00: حالت 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); بشکن;
            حالت 0x02: حالت 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); بشکن;
            حالت 0x04: حالت 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); بشکن;
            حالت 0x06: حالت 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); بشکن;
            حالت 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                اگر (not v_has_memory_idx(v, memidx)) بازگشت m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                اگر (not v->module->has_Data_شمار) بازگشت m3_Err_data_شمار_Required;
                اگر (dataidx >= v->module->رقم_Data_Segments) بازگشت m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_برداشت_expect(v, c_m3Type_i32, &a); اگر (r) بازگشت r; // n
                r = v_برداشت_expect(v, c_m3Type_i32, &a); اگر (r) بازگشت r; // src
                r = v_برداشت_expect(v, v_memory_addrtype(v, memidx), &a); اگر (r) بازگشت r; // dst
                بشکن;
            }
            حالت 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                // needs the segment, but not a memory
                اگر (not v->module->has_Data_شمار) بازگشت m3_Err_data_شمار_Required;
                اگر (dataidx >= v->module->رقم_Data_Segments) بازگشت m3Err_unknownDataSegment;
                بشکن;
            }
#اگر d_m3HasRefTypes
            حالت 0x0c: // table.init
            حالت 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;

                اگر (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    اگر (a2 >= v->module->رقم_Tables) بازگشت m3Err_unknownTable;
                    اگر (a1 >= v->module->رقم_Element_Segments) بازگشت m3Err_unknownElemSegment;
                    اگر (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        بازگشت m3Err_typeMismatch;
                } وگرنه {                                                    // dst, src
                    اگر (a1 >= v->module->رقم_Tables) بازگشت m3Err_unknownTable;
                    اگر (a2 >= v->module->رقم_Tables) بازگشت m3Err_unknownTable;
                    اگر (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        بازگشت m3Err_typeMismatch;
                }

                اگر (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_برداشت_expect(v, c_m3Type_i32, &a); اگر (r) بازگشت r;             // n
                    r = v_برداشت_expect(v, c_m3Type_i32, &a); اگر (r) بازگشت r;             // s
                    r = v_برداشت_expect(v, v_table_addrtype(v, a2), &a); اگر (r) بازگشت r;  // d
                } وگرنه {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_برداشت_expect(v, nType, &a); اگر (r) بازگشت r;  // n
                    r = v_برداشت_expect(v, sType, &a); اگر (r) بازگشت r;  // s
                    r = v_برداشت_expect(v, dType, &a); اگر (r) بازگشت r;  // d
                }
                بشکن;
            }
            حالت 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                اگر (elemIdx >= v->module->رقم_Element_Segments) بازگشت m3Err_unknownElemSegment;
                بشکن;
            }
            حالت 0x0f: // table.grow
            حالت 0x10: // table.size
            حالت 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                اگر (tableIdx >= v->module->رقم_Tables) بازگشت m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                اگر (sub == 0x10) {                                          // table.size
                    r = v_افزودن(v, at); اگر (r) بازگشت r;
                } وگرنه اگر (sub == 0x0f) {                                   // table.grow
                    r = v_برداشت_expect(v, at, &a); اگر (r) بازگشت r;  // n
                    r = v_برداشت_expect(v, t, &a);  اگر (r) بازگشت r;  // init
                    r = v_افزودن(v, at);           اگر (r) بازگشت r;
                } وگرنه {                                                    // table.fill
                    r = v_برداشت_expect(v, at, &a); اگر (r) بازگشت r;  // n
                    r = v_برداشت_expect(v, t, &a);  اگر (r) بازگشت r;  // val
                    r = v_برداشت_expect(v, at, &a); اگر (r) بازگشت r;  // i
                }
                بشکن;
            }
#endif
            حالت 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                اگر (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) بازگشت m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_برداشت_expect(v, lenType, &a); اگر (r) بازگشت r; // n
                r = v_برداشت_expect(v, srcType, &a); اگر (r) بازگشت r; // src
                r = v_برداشت_expect(v, dstType, &a); اگر (r) بازگشت r; // dst
                بشکن;
            }
            حالت 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                اگر (not v_has_memory_idx(v, memidx)) بازگشت m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_برداشت_expect(v, addrType, &a); اگر (r) بازگشت r;      // n
                r = v_برداشت_expect(v, c_m3Type_i32, &a); اگر (r) بازگشت r;  // val
                r = v_برداشت_expect(v, addrType, &a); اگر (r) بازگشت r;      // dst
                بشکن;
            }
#اگر d_m3HasWideArithmetic
            حالت 0x13: حالت 0x14: // i64.add128, i64.sub128
            حالت 0x15: حالت 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 رقم_Operands = (sub <= 0x14) ? 4 : 2;
                برای (u32 i = 0; i < رقم_Operands; ++i) {
                    r = v_برداشت_expect(v, c_m3Type_i64, &a); اگر (r) بازگشت r;
                }
                r = v_افزودن(v, c_m3Type_i64); اگر (r) بازگشت r;
                r = v_افزودن(v, c_m3Type_i64); اگر (r) بازگشت r;
                بشکن;
            }
#endif
            پیشفرض:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                بشکن;
            }
            بشکن;
        }

#اگر d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        حالت 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            اگر (r) بازگشت r;

            اگر (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                اگر (reserved != 0) بازگشت m3Err_wasmMalformed;
                بشکن;
            }

            u32 width, رقم_Operands; m3type_t مقدار_Type, resultType;
            اگر (not v_atomic_shape(sub, &width, &رقم_Operands, &مقدار_Type, &resultType)) بازگشت m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            اگر ((1u << align) != width) بازگشت m3Err_invalidAtomicAlignment;
            اگر (not v_has_memory_idx(v, memidx)) بازگشت m3Err_unknownMemory;
            اگر (not v_offset_in_range(v, memidx, offset)) بازگشت m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            اگر (sub == 0x01 or sub == 0x02) {
                r = v_برداشت_expect(v, c_m3Type_i64, &a); اگر (r) بازگشت r;
                رقم_Operands--;
            }
            برای (u32 i = 1; i < رقم_Operands; ++i) {
                r = v_برداشت_expect(v, مقدار_Type, &a); اگر (r) بازگشت r;
            }
            r = v_برداشت_expect(v, v_memory_addrtype(v, memidx), &a); اگر (r) بازگشت r;
            اگر (resultType != c_m3Type_none) {
                r = v_افزودن(v, resultType); اگر (r) بازگشت r;
            }
            بشکن;
        }
#endif

#اگر d_m3_Has_پشته_Switching
        // ---- Stack Switching ----
        حالت 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            اگر (!ft || !ft->isContinuation) بازگشت m3Err_typeMismatch;
            r = v_برداشت_expect(v, c_m3Type_funcref, &a); اگر (r) بازگشت r;
            r = v_افزودن(v, c_m3Type_contref); اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (typeIdx1 >= v->module->رقم_Func_Types || typeIdx2 >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            اگر (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) بازگشت m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 رقم_Params1 = ft1->رقم_Args;
            u16 رقم_Params2 = ft2->رقم_Args;
            اگر (رقم_Params1 < رقم_Params2) بازگشت m3Err_typeMismatch;
            u32 رقم_Bound = رقم_Params1 - رقم_Params2;

            اگر (ft1->رقم_Rets != ft2->رقم_Rets) بازگشت m3Err_typeMismatch;
            برای (u16 i = 0; i < ft1->رقم_Rets; ++i) {
                اگر (ft1->types[i] != ft2->types[i]) بازگشت m3Err_typeMismatch;
            }
            برای (u16 i = 0; i < رقم_Params2; ++i) {
                اگر (ft1->types[ft1->رقم_Rets + رقم_Bound + i] != ft2->types[ft2->رقم_Rets + i]) بازگشت m3Err_typeMismatch;
            }

            r = v_برداشت_expect(v, c_m3Type_contref, &a); اگر (r) بازگشت r;

            برای (u32 i = رقم_Bound; i > 0; i--) {
                r = v_برداشت_expect(v, BaseTypeOf(ft1->types[ft1->رقم_Rets + i - 1]), &a);
                اگر (r) بازگشت r;
            }

            r = v_افزودن(v, c_m3Type_contref); اگر (r) بازگشت r;
            بشکن;
        }

        حالت 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (!v->module || tagIdx >= v->module->رقم_Tags) بازگشت m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            اگر (tt) {
                برای (u16 i = tt->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(tt->types[tt->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
                برای (u16 i = 0; i < tt->رقم_Rets; i++) {
                    r = v_افزودن(v, BaseTypeOf(tt->types[i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) بازگشت m3Err_typeMismatch;

            u32 رقم_Clauses;
            r = ReadLEB_u32(&رقم_Clauses, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;

            برای (u32 i = 0; i < رقم_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                r = v_resume_clause(v, kind, tagIdx, ct); اگر (r) بازگشت r;
            }

            r = v_برداشت_expect(v, c_m3Type_contref, &a); اگر (r) بازگشت r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            اگر (ft) {
                برای (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
                برای (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_افزودن(v, BaseTypeOf(ft->types[i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, رقم_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            r = ReadLEB_u32(&رقم_Clauses, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;

            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            اگر (!v->module || tagIdx >= v->module->رقم_Tags) بازگشت m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) بازگشت m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            اگر (tagType && tagType->رقم_Rets != 0) بازگشت m3Err_typeMismatch;

            برای (u32 i = 0; i < رقم_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                r = v_resume_clause(v, kind, cTagIdx, ct); اگر (r) بازگشت r;
            }

            r = v_برداشت_expect(v, c_m3Type_contref, &a); اگر (r) بازگشت r;

            اگر (tagType) {
                برای (u16 i = tagType->رقم_Args; i > 0; i--) {
                    r = v_برداشت_expect(v, BaseTypeOf(tagType->types[tagType->رقم_Rets + i - 1]), &a);
                    اگر (r) بازگشت r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            اگر (ft) {
                برای (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_افزودن(v, BaseTypeOf(ft->types[i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0xe5: // resume_throw_ref
        {
            u32 typeIdx, رقم_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            r = ReadLEB_u32(&رقم_Clauses, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;

            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) بازگشت m3Err_typeMismatch;

            برای (u32 i = 0; i < رقم_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
                r = v_resume_clause(v, kind, cTagIdx, ct); اگر (r) بازگشت r;
            }

            r = v_برداشت_expect(v, c_m3Type_contref, &a); اگر (r) بازگشت r;
            r = v_برداشت_expect(v, c_m3Type_exnref, &a); اگر (r) بازگشت r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            اگر (ft) {
                برای (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_افزودن(v, BaseTypeOf(ft->types[i]));
                    اگر (r) بازگشت r;
                }
            }
            بشکن;
        }

        حالت 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); اگر (r) بازگشت r;

            اگر (typeIdx >= v->module->رقم_Func_Types) بازگشت m3Err_unknownType;
            اگر (!v->module || tagIdx >= v->module->رقم_Tags) بازگشت m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) بازگشت m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            اگر (tagType && tagType->رقم_Args != 0) بازگشت m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            اگر (!ft || ft->رقم_Args == 0) بازگشت m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->رقم_Rets + ft->رقم_Args - 1]);
            اگر (!peer || !peer->contFuncType) بازگشت m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_برداشت_expect(v, c_m3Type_contref, &a); اگر (r) بازگشت r;

            برای (u16 i = (u16)(ft->رقم_Args - 1); i > 0; i--) {
                r = v_برداشت_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                اگر (r) بازگشت r;
            }

            برای (u16 i = 0; i < self->رقم_Args; i++) {
                r = v_افزودن(v, BaseTypeOf(self->types[self->رقم_Rets + i]));
                اگر (r) بازگشت r;
            }
            بشکن;
        }

#endif // d_m3HasStackSwitching

        پیشفرض:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            بشکن;

        } // switch

        اگر (r) بازگشت r;

    } // while

    // If we ran out of bytes without hitting the final end
    بازگشت m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    اگر (!i_function->wasm) بازگشت m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    اگر (!v) {
        v = m3_AllocStruct (ValCtx);
        اگر (!v) بازگشت m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, اندازه(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    اگر (r) بازگشت r;

    // Parse locals
    u32 رقم_Local_Blocks;
    r = ReadLEB_u32(&رقم_Local_Blocks, &v->wasm, v->wasmEnd);
    اگر (r) بازگشت r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 رقم_Params = funcType ? funcType->رقم_Args : 0;
    اگر (رقم_Params > d_m3_Val_پشته) بازگشت m3_Err_function_پشته_Overflow;
    برای (u16 i = 0; i < رقم_Params; i++) {
        v->localTypes[v->رقم_Locals++] = BaseTypeOf(funcType->types[funcType->رقم_Rets + i]);
    }

    // Then: declared locals
    برای (u32 b = 0; b < رقم_Local_Blocks; b++) {
        u32 شمار;
        r = ReadLEB_u32(&شمار, &v->wasm, v->wasmEnd);
        اگر (r) بازگشت r;
        m3type_t localType;
        r = Parse_مقدار_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        اگر (r) بازگشت r;
        u8 normalized = BaseTypeOf(localType);
        اگر (شمار > (u32) (d_m3_Val_پشته - v->رقم_Locals)) بازگشت m3_Err_function_پشته_Overflow;
        برای (u32 c = 0; c < شمار; c++) {
            v->localTypes[v->رقم_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_افزودن_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    اگر (r) بازگشت r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    اگر (r) بازگشت r;

    // After validation, control stack should be empty and all declared bytes consumed
    اگر (v->ctrl_بالاترین != 0)
        بازگشت m3Err_wasmMalformed;

    اگر (v->wasm != v->wasmEnd)
        بازگشت m3Err_wasmSectionUnderrun;

    بازگشت m3Err_none;
}

#وگرنه // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (تهی)i_function;
    بازگشت m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
