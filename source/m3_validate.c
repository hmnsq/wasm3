//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#ضمن "m3_validate.h"
#ضمن "m3_exception.h"
#ضمن "m3_info.h"

#إذا d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

تعريفنوع بنية {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_تعداد;
    u16         result_تعداد;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

بنية ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_مكدس];
    u16         opd_قمة;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_قمة;

    m3type_t    localTypes [d_m3_Val_مكدس];
    u16         رقم_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ساكن bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    إرجاع v->module and i_memoryIdx < v->module->رقم_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ساكن m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    إرجاع Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ساكن bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    إرجاع v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ساكن m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    إرجاع Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
ساكن u32 v_max_align (m3opcode_t opcode)
{
    بدل (opcode) {
    حالة 0x2c: حالة 0x2d:   // i32.load8_s, i32.load8_u
    حالة 0x30: حالة 0x31:   // i64.load8_s, i64.load8_u
    حالة 0x3a:              // i32.store8
    حالة 0x3c:              // i64.store8
        إرجاع 0;
    حالة 0x2e: حالة 0x2f:   // i32.load16_s, i32.load16_u
    حالة 0x32: حالة 0x33:   // i64.load16_s, i64.load16_u
    حالة 0x3b:              // i32.store16
    حالة 0x3d:              // i64.store16
        إرجاع 1;
    حالة 0x29:              // i64.load
    حالة 0x2b:              // f64.load
    حالة 0x37:              // i64.store
    حالة 0x39:              // f64.store
        إرجاع 3;
    افتراضي:                // 32-bit accesses, and a safe fallback
        إرجاع 2;
    }
}

#إذا d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
ساكن bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_رقم_Operands, m3type_t * o_قيمة_Type, m3type_t * o_resultType)
{
    ساكن ثابت u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ساكن ثابت m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    إذا (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_رقم_Operands = (sub == 0x00) ? 2 : 3;
        *o_قيمة_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        إرجاع true;
    }

    إذا (sub < 0x10 or sub > c_waOp_lastAtomic) إرجاع false;

    إذا (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_رقم_Operands = 1;
    } وإلا إذا (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_رقم_Operands = 2;
    } وإلا إذا (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_رقم_Operands = 2;
    } وإلا {                        // cmpxchg
        variant = sub - 0x48;       *o_رقم_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_قيمة_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    إرجاع true;
}

#endif

// ---------- Operand stack ----------

ساكن M3Result v_ادفع (ValCtx * v, m3type_t type)
{
    إذا (M3_UNLIKELY(v->opd_قمة >= d_m3_Val_مكدس))
        إرجاع m3_Err_function_مكدس_Overflow;
    v->opd[v->opd_قمة++] = type;
    إرجاع m3Err_none;
}

ساكن M3Result v_اسحب (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_قمة - 1];
    إذا (M3_UNLIKELY(v->opd_قمة == f->height)) {
        إذا (f->is_unreachable) { *o_type = c_valBottom; إرجاع m3Err_none; }
        إرجاع m3_Err_function_مكدس_Underrun;
    }
    *o_type = v->opd[--v->opd_قمة];
    إرجاع m3Err_none;
}

ساكن M3Result v_اسحب_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_اسحب(v, &actual);
    إذا (M3_UNLIKELY(r)) إرجاع r;
    إذا (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        إرجاع m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    إرجاع m3Err_none;
}

// ---------- Control stack ----------

ساكن M3Result v_ادفع_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    إذا (M3_UNLIKELY(v->ctrl_قمة >= d_m3ValCtrlDepth))
        إرجاع m3_Err_function_مكدس_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_قمة++];
    f->opcode       = op;
    f->type         = type;
    f->param_تعداد  = type ? type->رقم_Args : 0;
    f->result_تعداد = type ? type->رقم_Rets : 0;
    f->height       = v->opd_قمة;
    f->is_unreachable = false;
    إرجاع m3Err_none;
}

ساكن M3Result v_اسحب_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    إذا (M3_UNLIKELY(v->ctrl_قمة == 0))
        إرجاع m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_قمة - 1];
    // pop result types
    إذا (f->type) {
        لكل (u16 i = f->result_تعداد; i > 0; i--) {
            m3type_t a;
            M3Result r = v_اسحب_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            إذا (M3_UNLIKELY(r)) إرجاع r;
        }
    }
    إذا (M3_UNLIKELY(v->opd_قمة != f->height))
        إرجاع m3_Err_type_تعداد_Mismatch;
    إذا (o_frame) *o_frame = *f;
    v->ctrl_قمة--;
    إرجاع m3Err_none;
}

ساكن فراغ v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_قمة - 1];
    v->opd_قمة = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ساكن u16 v_label_n (ValCtrlFrame * f)
{
    إرجاع (f->opcode == 0x03) ? f->param_تعداد : f->result_تعداد;
}

ساكن m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    إذا (!f->type) إرجاع c_m3Type_none;
    إذا (f->opcode == 0x03)
        إرجاع BaseTypeOf(f->type->types[f->type->رقم_Rets + i]); // params
    إرجاع BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ساكن m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_قمة - 1];

    إذا (v->opd_قمة <= i_depth || (u16)(v->opd_قمة - i_depth - 1) < f->height)
        إرجاع c_valBottom;

    إرجاع v->opd[v->opd_قمة - i_depth - 1];
}

// Pop label types for branch target
ساكن M3Result v_اسحب_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    لكل (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_اسحب_expect(v, v_label_t(tgt, i - 1), &a);
        إذا (M3_UNLIKELY(r)) إرجاع r;
    }
    إرجاع m3Err_none;
}

// Push label types back
ساكن M3Result v_ادفع_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    لكل (u16 i = 0; i < n; i++) {
        M3Result r = v_ادفع(v, v_label_t(tgt, i));
        إذا (M3_UNLIKELY(r)) إرجاع r;
    }
    إرجاع m3Err_none;
}

// ---------- Block type resolution ----------

ساكن M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    إذا (v->wasm >= v->wasmEnd)
        إرجاع m3Err_wasmUnderrun;

#إذا d_m3HasTypedRefs
    إذا (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_قيمة_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        إذا (rr) إرجاع rr;
        IM3FuncType ftype = عدم;
        rr = AllocFuncType(&ftype, 1);
        إذا (rr) إرجاع rr;
        ftype->رقم_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        إذا (rr) إرجاع rr;
        *o_type = ftype;
        إرجاع m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    إذا (r) إرجاع r;

    إذا (type < 0) {
        إذا (type < -64) إرجاع m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        إذا (r) إرجاع r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } وإلا {
        إذا ((u32)type >= v->module->رقم_Func_Types)
            إرجاع m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    إرجاع m3Err_none;
}

// ---------- Convenience ----------

ساكن M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_اسحب_expect(v, in, &a);
    إذا (M3_UNLIKELY(r)) إرجاع r;
    إرجاع v_ادفع(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ساكن M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : عدم;

    u16 رقم_Callee_Rets = i_calleeType ? i_calleeType->رقم_Rets : 0;
    u16 رقم_Func_Rets   = ft ? ft->رقم_Rets : 0;

    إذا (رقم_Callee_Rets != رقم_Func_Rets)
        إرجاع m3Err_typeMismatch;

    لكل (u16 i = 0; i < رقم_Callee_Rets; i++) {
        إذا (i_calleeType->types[i] != ft->types[i])
            إرجاع m3Err_typeMismatch;
    }

    إرجاع m3Err_none;
}

ساكن M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_اسحب_expect(v, t, &a); إذا (M3_UNLIKELY(r)) إرجاع r;
    r = v_اسحب_expect(v, t, &a); إذا (M3_UNLIKELY(r)) إرجاع r;
    إرجاع v_ادفع(v, t);
}

ساكن M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_اسحب_expect(v, t, &a); إذا (M3_UNLIKELY(r)) إرجاع r;
    r = v_اسحب_expect(v, t, &a); إذا (M3_UNLIKELY(r)) إرجاع r;
    إرجاع v_ادفع(v, c_m3Type_i32);
}

ساكن M3Result v_testop (ValCtx * v, m3type_t t)
{
    إرجاع v_unop(v, t, c_m3Type_i32);
}

ساكن M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    إرجاع v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#إذا d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ساكن M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    إذا (r) إرجاع r;
    إذا (kind > 0x03) إرجاع m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = عدم;
    إذا (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        إذا (r) إرجاع r;
        إذا (!v->module || tagIndex >= v->module->رقم_Tags) إرجاع m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    إذا (r) إرجاع r;
    إذا (depth >= v->ctrl_قمة) إرجاع m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_قمة - 1 - depth];

    u16 رقم_Payload = tagType ? tagType->رقم_Args : 0;
    u16 رقم_Label   = v_label_n(tgt);

    إذا (رقم_Label != رقم_Payload + (hasRef ? 1u : 0u))
        إرجاع m3_Err_type_تعداد_Mismatch;

    لكل (u16 i = 0; i < رقم_Payload; i++) {
        إذا (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->رقم_Rets + i]))
            إرجاع m3Err_typeMismatch;
    }

    إذا (hasRef and v_label_t(tgt, رقم_Payload) != c_m3Type_exnref)
        إرجاع m3Err_typeMismatch;

    إرجاع m3Err_none;
}

#endif // d_m3HasExceptionHandling

#إذا d_m3_Has_مكدس_Switching

ساكن M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    إذا (kind > 0x01) إرجاع m3Err_wasmMalformed;
    إذا (!v->module || tagIdx >= v->module->رقم_Tags) إرجاع m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    إذا (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
        إذا (depth >= v->ctrl_قمة) إرجاع m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_قمة - 1 - depth];
        u16 رقم_Payload = tagType ? tagType->رقم_Args : 0;
        u16 رقم_Label   = v_label_n(tgt);

        إذا (رقم_Label != رقم_Payload + 1u) إرجاع m3_Err_type_تعداد_Mismatch;

        لكل (u16 j = 0; j < رقم_Payload; j++) {
            إذا (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->رقم_Rets + j]))
                إرجاع m3Err_typeMismatch;
        }
        إذا (v_label_t(tgt, رقم_Payload) != c_m3Type_contref)
            إرجاع m3Err_typeMismatch;

#إذا d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->رقم_Rets + رقم_Payload]
            : tgt->type->types[رقم_Payload];

        إذا (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            إرجاع m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = عدم;
        لكل (u32 idx = 0; idx < v->module->رقم_Func_Types; ++idx) {
            إذا (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                اكسر;
            }
        }
        إذا (!labelCt) إرجاع m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_رقم_Params = tagType ? tagType->رقم_Rets : 0;
        إذا (labelFt->رقم_Args != expected_رقم_Params) إرجاع m3Err_typeMismatch;
        لكل (u16 p = 0; p < expected_رقم_Params; ++p) {
            إذا (labelFt->types[labelFt->رقم_Rets + p] != tagType->types[p])
                إرجاع m3Err_typeMismatch;
        }

        u16 expected_رقم_Rets = ft ? ft->رقم_Rets : 0;
        إذا (labelFt->رقم_Rets != expected_رقم_Rets) إرجاع m3Err_typeMismatch;
        لكل (u16 r_idx = 0; r_idx < expected_رقم_Rets; ++r_idx) {
            إذا (labelFt->types[r_idx] != ft->types[r_idx])
                إرجاع m3Err_typeMismatch;
        }
#endif
    } وإلا {
        إذا (tagType && tagType->رقم_Args != 0) إرجاع m3Err_typeMismatch;
    }
    إرجاع m3Err_none;
}

#endif // d_m3HasStackSwitching

ساكن M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    بينما (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        إذا (M3_UNLIKELY(r)) إرجاع r;

        بدل (opcode)
        {
        // ---- Control ----
        حالة 0x00: // unreachable
            v_unreachable(v);
            اكسر;

        حالة 0x01: // nop
            اكسر;

        حالة 0x02: // block
        حالة 0x03: // loop
        حالة 0x04: // if
        {
            IM3FuncType bt = عدم;
            r = v_read_blocktype(v, &bt);
            إذا (r) إرجاع r;
            إذا (opcode == 0x04) {
                r = v_اسحب_expect(v, c_m3Type_i32, &a);
                إذا (r) إرجاع r;
            }
            // Pop block params from caller stack
            إذا (bt) {
                لكل (u16 i = bt->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(bt->types[bt->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
            }
            r = v_ادفع_ctrl(v, opcode, bt);
            إذا (r) إرجاع r;
            // Push params inside block
            إذا (bt) {
                لكل (u16 i = 0; i < bt->رقم_Args; i++) {
                    r = v_ادفع(v, BaseTypeOf(bt->types[bt->رقم_Rets + i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

#إذا d_m3HasExceptionHandling
        حالة 0x1f: // try_table
        {
            IM3FuncType bt = عدم;
            r = v_read_blocktype(v, &bt);
            إذا (r) إرجاع r;
            إذا (bt) {
                لكل (u16 i = bt->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(bt->types[bt->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 رقم_Catch;
            r = ReadLEB_u32(&رقم_Catch, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            لكل (u32 i = 0; i < رقم_Catch; i++) {
                r = v_catch_clause(v);
                إذا (r) إرجاع r;
            }

            r = v_ادفع_ctrl(v, opcode, bt);
            إذا (r) إرجاع r;

            إذا (bt) {
                لكل (u16 i = 0; i < bt->رقم_Args; i++) {
                    r = v_ادفع(v, BaseTypeOf(bt->types[bt->رقم_Rets + i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (!v->module || tagIndex >= v->module->رقم_Tags) إرجاع m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            إذا (tt) {
                لكل (u16 i = tt->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(tt->types[tt->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
            }
            v_unreachable(v);
            اكسر;
        }

        حالة 0x0a: // throw_ref
            r = v_اسحب_expect(v, c_m3Type_exnref, &a);
            إذا (r) إرجاع r;
            v_unreachable(v);
            اكسر;
#endif // d_m3HasExceptionHandling

        حالة 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_اسحب_ctrl(v, &frame);
            إذا (r) إرجاع r;
            إذا (frame.opcode != 0x04)
                إرجاع m3Err_wasmMalformed;
            r = v_ادفع_ctrl(v, 0x05, frame.type);
            إذا (r) إرجاع r;
            إذا (frame.type) {
                لكل (u16 i = 0; i < frame.type->رقم_Args; i++) {
                    r = v_ادفع(v, BaseTypeOf(frame.type->types[frame.type->رقم_Rets + i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_اسحب_ctrl(v, &frame);
            إذا (r) إرجاع r;
            // Push results
            إذا (frame.type) {
                لكل (u16 i = 0; i < frame.result_تعداد; i++) {
                    r = v_ادفع(v, BaseTypeOf(frame.type->types[i]));
                    إذا (r) إرجاع r;
                }
            }
            // If this was the outermost frame, we're done
            إذا (v->ctrl_قمة == 0)
                إرجاع m3Err_none;
            اكسر;
        }

        حالة 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (depth >= v->ctrl_قمة) إرجاع m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_قمة - 1 - depth];
            r = v_اسحب_labels(v, tgt);
            إذا (r) إرجاع r;
            v_unreachable(v);
            اكسر;
        }

        حالة 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (depth >= v->ctrl_قمة) إرجاع m3Err_unknownLabel;
            r = v_اسحب_expect(v, c_m3Type_i32, &a);
            إذا (r) إرجاع r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_قمة - 1 - depth];
            r = v_اسحب_labels(v, tgt);
            إذا (r) إرجاع r;
            r = v_ادفع_labels(v, tgt);
            إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0x0e: // br_table
        {
            u32 تعداد_2;
            r = ReadLEB_u32(&تعداد_2, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            لكل (u32 i = 0; i <= تعداد_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                إذا (r) إرجاع r;
                إذا (d >= v->ctrl_قمة) إرجاع m3Err_unknownLabel;
                إذا (i == تعداد_2) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_قمة - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_اسحب_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            إذا (r) إرجاع r;

            لكل (u32 i = 0; i <= تعداد_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                إذا (r) إرجاع r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_قمة - 1 - d];
                u16 n = v_label_n(t);
                إذا (n != arity) إرجاع m3_Err_type_تعداد_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                لكل (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    إذا (want != c_valBottom && have != c_valBottom && want != have)
                        إرجاع m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_قمة - 1 - defDepth];
            r = v_اسحب_labels(v, dt);
            إذا (r) إرجاع r;
            v_unreachable(v);
            اكسر;
        }

        حالة 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            إذا (ft) {
                لكل (u16 i = ft->رقم_Rets; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    إذا (r) إرجاع r;
                }
            }
            v_unreachable(v);
            اكسر;
        }

        // ---- Call ----
        حالة 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (idx >= v->module->رقم_Functions) إرجاع m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            إذا (ft) {
                لكل (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
                لكل (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_ادفع(v, BaseTypeOf(ft->types[i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            // the table must exist and hold funcrefs
            إذا (tableIdx >= v->module->رقم_Tables) إرجاع m3Err_unknownTable;
            إذا (v->module->tables[tableIdx]->type != c_m3Type_funcref) إرجاع m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_اسحب_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            إذا (r) إرجاع r;
            إذا (ft) {
                لكل (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
                لكل (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_ادفع(v, BaseTypeOf(ft->types[i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (idx >= v->module->رقم_Functions) إرجاع m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            إذا (r) إرجاع r;
            إذا (ft) {
                لكل (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
            }
            v_unreachable(v);
            اكسر;
        }

        حالة 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            // the table must exist and hold funcrefs
            إذا (tableIdx >= v->module->رقم_Tables) إرجاع m3Err_unknownTable;
            إذا (v->module->tables[tableIdx]->type != c_m3Type_funcref) إرجاع m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            إذا (r) إرجاع r;
            r = v_اسحب_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            إذا (r) إرجاع r;
            إذا (ft) {
                لكل (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
            }
            v_unreachable(v);
            اكسر;
        }

#إذا d_m3HasTypedRefs
        حالة 0x14: // call_ref
        حالة 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            إذا (isTail) {
                r = v_check_tail_results(v, ft);
                إذا (r) إرجاع r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_اسحب_expect(v, c_m3Type_funcref, &a);
            إذا (r) إرجاع r;
            إذا (ft) {
                لكل (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
                إذا (not isTail) {
                    لكل (u16 i = 0; i < ft->رقم_Rets; i++) {
                        r = v_ادفع(v, BaseTypeOf(ft->types[i]));
                        إذا (r) إرجاع r;
                    }
                }
            }
            إذا (isTail) v_unreachable(v);
            اكسر;
        }

        حالة 0xd4: // ref.as_non_null
        {
            r = v_اسحب_expect(v, c_m3Type_funcref, &a);
            إذا (r) إرجاع r;
            r = v_ادفع(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            إذا (r) إرجاع r;
            اكسر;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        حالة 0x1a: // drop
            r = v_اسحب(v, &a);
            إذا (r) إرجاع r;
            اكسر;

        حالة 0x1b: // select
        {
            r = v_اسحب_expect(v, c_m3Type_i32, &a);
            إذا (r) إرجاع r;
            m3type_t t2 = c_valBottom;
            r = v_اسحب(v, &t2);
            إذا (r) إرجاع r;
            m3type_t t1;
            r = v_اسحب_expect(v, t2, &t1);
            إذا (r) إرجاع r;
            // untyped select is numeric only; references need the 0x1c form
            إذا (t2 != c_valBottom && IsRefType(t2)) إرجاع m3Err_typeMismatch;
            r = v_ادفع(v, (t2 == c_valBottom) ? t1 : t2);
            إذا (r) إرجاع r;
            اكسر;
        }

#إذا d_m3HasRefTypes
        حالة 0x1c: // select with an explicit result type
        {
            u32 رقم_Types;
            r = ReadLEB_u32(&رقم_Types, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (رقم_Types != 1) إرجاع m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_قيمة_Type(v->module, &selType, &v->wasm, v->wasmEnd);  إذا (r) إرجاع r;
            u8 t = BaseTypeOf(selType);

            r = v_اسحب_expect(v, c_m3Type_i32, &a); إذا (r) إرجاع r;
            r = v_اسحب_expect(v, t, &a);            إذا (r) إرجاع r;
            r = v_اسحب_expect(v, t, &a);            إذا (r) إرجاع r;
            r = v_ادفع(v, t);                      إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0x25: // table.get
        حالة 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (tableIdx >= v->module->رقم_Tables) إرجاع m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            إذا (opcode == 0x26) {
                r = v_اسحب_expect(v, t, &a);   إذا (r) إرجاع r;
                r = v_اسحب_expect(v, at, &a);  إذا (r) إرجاع r;
            } وإلا {
                r = v_اسحب_expect(v, at, &a);  إذا (r) إرجاع r;
                r = v_ادفع(v, t);             إذا (r) إرجاع r;
            }
            اكسر;
        }

        حالة 0xd0: // ref.null
        {
#إذا d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  إذا (r) إرجاع r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# إذا d_m3_Has_مكدس_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#وإلا
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   إذا (r) إرجاع r;
            r = NormalizeType(&t, waType);                   إذا (r) إرجاع r;
            إذا (!IsRefType(t)) إرجاع m3Err_wasmMalformed;
#endif
            r = v_ادفع(v, t);                                إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_اسحب(v, &t);
            إذا (r) إرجاع r;
            إذا (t != c_valBottom && !IsRefType(t)) إرجاع m3Err_typeMismatch;
            r = v_ادفع(v, c_m3Type_i32);
            إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (funcIdx >= v->module->رقم_Functions) إرجاع m3Err_unknownFunction;
            إذا (!Module_IsFunctionDeclared(v->module, funcIdx)) إرجاع m3Err_undeclaredFuncRef;
            r = v_ادفع(v, c_m3Type_funcref);
            إذا (r) إرجاع r;
            اكسر;
        }
#endif

        // ---- Variable ----
        حالة 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (idx >= v->رقم_Locals) إرجاع m3Err_unknownLocal;
            r = v_ادفع(v, v->localTypes[idx]);
            إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (idx >= v->رقم_Locals) إرجاع m3Err_unknownLocal;
            r = v_اسحب_expect(v, v->localTypes[idx], &a);
            إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (idx >= v->رقم_Locals) إرجاع m3Err_unknownLocal;
            r = v_اسحب_expect(v, v->localTypes[idx], &a);
            إذا (r) إرجاع r;
            r = v_ادفع(v, v->localTypes[idx]);
            إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (idx >= v->module->رقم_Globals) إرجاع m3Err_unknownGlobal;
            r = v_ادفع(v, BaseTypeOf(v->module->globals[idx].type));
            إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            إذا (idx >= v->module->رقم_Globals) إرجاع m3Err_unknownGlobal;
            r = v_اسحب_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            إذا (r) إرجاع r;
            اكسر;
        }

        // ---- Memory load ----
        حالة 0x28: حالة 0x29: حالة 0x2a: حالة 0x2b: // i32/i64/f32/f64.load
        حالة 0x2c: حالة 0x2d: حالة 0x2e: حالة 0x2f: // i32.load8/16 s/u
        حالة 0x30: حالة 0x31: حالة 0x32: حالة 0x33: // i64.load8/16 s/u
        حالة 0x34: حالة 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (align > v_max_align(opcode)) إرجاع m3Err_invalidAlignment;
            إذا (not v_has_memory_idx(v, memidx)) إرجاع m3Err_unknownMemory;
            إذا (not v_offset_in_range(v, memidx, offset)) إرجاع m3Err_wasmMalformed;
            r = v_اسحب_expect(v, v_memory_addrtype(v, memidx), &a); إذا (r) إرجاع r;
            u8 result;
            إذا      (opcode == 0x28) result = c_m3Type_i32;
            وإلا إذا (opcode == 0x29) result = c_m3Type_i64;
            وإلا إذا (opcode == 0x2a) result = c_m3Type_f32;
            وإلا إذا (opcode == 0x2b) result = c_m3Type_f64;
            وإلا إذا (opcode <= 0x2f) result = c_m3Type_i32;
            وإلا                     result = c_m3Type_i64;
            r = v_ادفع(v, result);
            إذا (r) إرجاع r;
            اكسر;
        }

        // ---- Memory store ----
        حالة 0x36: حالة 0x37: حالة 0x38: حالة 0x39: // i32/i64/f32/f64.store
        حالة 0x3a: حالة 0x3b:                         // i32.store8/16
        حالة 0x3c: حالة 0x3d: حالة 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (align > v_max_align(opcode)) إرجاع m3Err_invalidAlignment;
            إذا (not v_has_memory_idx(v, memidx)) إرجاع m3Err_unknownMemory;
            إذا (not v_offset_in_range(v, memidx, offset)) إرجاع m3Err_wasmMalformed;
            u8 valtype;
            إذا      (opcode == 0x36) valtype = c_m3Type_i32;
            وإلا إذا (opcode == 0x37) valtype = c_m3Type_i64;
            وإلا إذا (opcode == 0x38) valtype = c_m3Type_f32;
            وإلا إذا (opcode == 0x39) valtype = c_m3Type_f64;
            وإلا إذا (opcode <= 0x3b) valtype = c_m3Type_i32;
            وإلا                     valtype = c_m3Type_i64;
            r = v_اسحب_expect(v, valtype, &a); إذا (r) إرجاع r;
            r = v_اسحب_expect(v, v_memory_addrtype(v, memidx), &a); إذا (r) إرجاع r;
            اكسر;
        }

        // ---- Memory size/grow ----
        حالة 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (not v_has_memory_idx(v, memidx)) إرجاع m3Err_unknownMemory;
            r = v_ادفع(v, v_memory_addrtype(v, memidx)); إذا (r) إرجاع r;
            اكسر;
        }
        حالة 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (not v_has_memory_idx(v, memidx)) إرجاع m3Err_unknownMemory;
            r = v_اسحب_expect(v, v_memory_addrtype(v, memidx), &a); إذا (r) إرجاع r;
            r = v_ادفع(v, v_memory_addrtype(v, memidx)); إذا (r) إرجاع r;
            اكسر;
        }

        // ---- Constants ----
        حالة 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            r = v_ادفع(v, c_m3Type_i32); إذا (r) إرجاع r;
            اكسر;
        }
        حالة 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            r = v_ادفع(v, c_m3Type_i64); إذا (r) إرجاع r;
            اكسر;
        }
        حالة 0x43: { // f32.const
            إذا (v->wasm + 4 > v->wasmEnd) إرجاع m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ادفع(v, c_m3Type_f32); إذا (r) إرجاع r;
            اكسر;
        }
        حالة 0x44: { // f64.const
            إذا (v->wasm + 8 > v->wasmEnd) إرجاع m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ادفع(v, c_m3Type_f64); إذا (r) إرجاع r;
            اكسر;
        }


        // ---- i32 comparison ----
        حالة 0x45: r = v_testop(v, c_m3Type_i32); اكسر; // i32.eqz
        حالة 0x46: حالة 0x47: حالة 0x48: حالة 0x49: حالة 0x4a:
        حالة 0x4b: حالة 0x4c: حالة 0x4d: حالة 0x4e: حالة 0x4f:
            r = v_relop(v, c_m3Type_i32); اكسر;

        // ---- i64 comparison ----
        حالة 0x50: r = v_testop(v, c_m3Type_i64); اكسر; // i64.eqz
        حالة 0x51: حالة 0x52: حالة 0x53: حالة 0x54: حالة 0x55:
        حالة 0x56: حالة 0x57: حالة 0x58: حالة 0x59: حالة 0x5a:
            r = v_relop(v, c_m3Type_i64); اكسر;

        // ---- f32 comparison ----
        حالة 0x5b: حالة 0x5c: حالة 0x5d: حالة 0x5e: حالة 0x5f: حالة 0x60:
            r = v_relop(v, c_m3Type_f32); اكسر;

        // ---- f64 comparison ----
        حالة 0x61: حالة 0x62: حالة 0x63: حالة 0x64: حالة 0x65: حالة 0x66:
            r = v_relop(v, c_m3Type_f64); اكسر;

        // ---- i32 unary ----
        حالة 0x67: حالة 0x68: حالة 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); اكسر;

        // ---- i32 binary ----
        حالة 0x6a: حالة 0x6b: حالة 0x6c: حالة 0x6d: حالة 0x6e: حالة 0x6f:
        حالة 0x70: حالة 0x71: حالة 0x72: حالة 0x73: حالة 0x74: حالة 0x75:
        حالة 0x76: حالة 0x77: حالة 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); اكسر;

        // ---- i64 unary ----
        حالة 0x79: حالة 0x7a: حالة 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); اكسر;

        // ---- i64 binary ----
        حالة 0x7c: حالة 0x7d: حالة 0x7e: حالة 0x7f: حالة 0x80: حالة 0x81:
        حالة 0x82: حالة 0x83: حالة 0x84: حالة 0x85: حالة 0x86: حالة 0x87:
        حالة 0x88: حالة 0x89: حالة 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); اكسر;

        // ---- f32 unary ----
        حالة 0x8b: حالة 0x8c: حالة 0x8d: حالة 0x8e: حالة 0x8f:
        حالة 0x90: حالة 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); اكسر;

        // ---- f32 binary ----
        حالة 0x92: حالة 0x93: حالة 0x94: حالة 0x95: حالة 0x96:
        حالة 0x97: حالة 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); اكسر;

        // ---- f64 unary ----
        حالة 0x99: حالة 0x9a: حالة 0x9b: حالة 0x9c: حالة 0x9d:
        حالة 0x9e: حالة 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); اكسر;

        // ---- f64 binary ----
        حالة 0xa0: حالة 0xa1: حالة 0xa2: حالة 0xa3: حالة 0xa4:
        حالة 0xa5: حالة 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); اكسر;

        // ---- Conversions ----
        حالة 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); اكسر; // i32.wrap/i64
        حالة 0xa8: حالة 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); اكسر;
        حالة 0xaa: حالة 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); اكسر;
        حالة 0xac: حالة 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); اكسر;
        حالة 0xae: حالة 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); اكسر;
        حالة 0xb0: حالة 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); اكسر;
        حالة 0xb2: حالة 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); اكسر;
        حالة 0xb4: حالة 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); اكسر;
        حالة 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); اكسر;
        حالة 0xb7: حالة 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); اكسر;
        حالة 0xb9: حالة 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); اكسر;
        حالة 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); اكسر;
        حالة 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); اكسر;
        حالة 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); اكسر;
        حالة 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); اكسر;
        حالة 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); اكسر;

        // ---- Sign-extension (MVP post) ----
        حالة 0xc0: حالة 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); اكسر;
        حالة 0xc2: حالة 0xc3: حالة 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); اكسر;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        حالة 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;
            بدل (sub) {
            حالة 0x00: حالة 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); اكسر;
            حالة 0x02: حالة 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); اكسر;
            حالة 0x04: حالة 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); اكسر;
            حالة 0x06: حالة 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); اكسر;
            حالة 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                إذا (not v_has_memory_idx(v, memidx)) إرجاع m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                إذا (not v->module->has_Data_تعداد) إرجاع m3_Err_data_تعداد_Required;
                إذا (dataidx >= v->module->رقم_Data_Segments) إرجاع m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_اسحب_expect(v, c_m3Type_i32, &a); إذا (r) إرجاع r; // n
                r = v_اسحب_expect(v, c_m3Type_i32, &a); إذا (r) إرجاع r; // src
                r = v_اسحب_expect(v, v_memory_addrtype(v, memidx), &a); إذا (r) إرجاع r; // dst
                اكسر;
            }
            حالة 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                // needs the segment, but not a memory
                إذا (not v->module->has_Data_تعداد) إرجاع m3_Err_data_تعداد_Required;
                إذا (dataidx >= v->module->رقم_Data_Segments) إرجاع m3Err_unknownDataSegment;
                اكسر;
            }
#إذا d_m3HasRefTypes
            حالة 0x0c: // table.init
            حالة 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;

                إذا (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    إذا (a2 >= v->module->رقم_Tables) إرجاع m3Err_unknownTable;
                    إذا (a1 >= v->module->رقم_Element_Segments) إرجاع m3Err_unknownElemSegment;
                    إذا (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        إرجاع m3Err_typeMismatch;
                } وإلا {                                                    // dst, src
                    إذا (a1 >= v->module->رقم_Tables) إرجاع m3Err_unknownTable;
                    إذا (a2 >= v->module->رقم_Tables) إرجاع m3Err_unknownTable;
                    إذا (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        إرجاع m3Err_typeMismatch;
                }

                إذا (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_اسحب_expect(v, c_m3Type_i32, &a); إذا (r) إرجاع r;             // n
                    r = v_اسحب_expect(v, c_m3Type_i32, &a); إذا (r) إرجاع r;             // s
                    r = v_اسحب_expect(v, v_table_addrtype(v, a2), &a); إذا (r) إرجاع r;  // d
                } وإلا {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_اسحب_expect(v, nType, &a); إذا (r) إرجاع r;  // n
                    r = v_اسحب_expect(v, sType, &a); إذا (r) إرجاع r;  // s
                    r = v_اسحب_expect(v, dType, &a); إذا (r) إرجاع r;  // d
                }
                اكسر;
            }
            حالة 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                إذا (elemIdx >= v->module->رقم_Element_Segments) إرجاع m3Err_unknownElemSegment;
                اكسر;
            }
            حالة 0x0f: // table.grow
            حالة 0x10: // table.size
            حالة 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                إذا (tableIdx >= v->module->رقم_Tables) إرجاع m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                إذا (sub == 0x10) {                                          // table.size
                    r = v_ادفع(v, at); إذا (r) إرجاع r;
                } وإلا إذا (sub == 0x0f) {                                   // table.grow
                    r = v_اسحب_expect(v, at, &a); إذا (r) إرجاع r;  // n
                    r = v_اسحب_expect(v, t, &a);  إذا (r) إرجاع r;  // init
                    r = v_ادفع(v, at);           إذا (r) إرجاع r;
                } وإلا {                                                    // table.fill
                    r = v_اسحب_expect(v, at, &a); إذا (r) إرجاع r;  // n
                    r = v_اسحب_expect(v, t, &a);  إذا (r) إرجاع r;  // val
                    r = v_اسحب_expect(v, at, &a); إذا (r) إرجاع r;  // i
                }
                اكسر;
            }
#endif
            حالة 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                إذا (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) إرجاع m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_اسحب_expect(v, lenType, &a); إذا (r) إرجاع r; // n
                r = v_اسحب_expect(v, srcType, &a); إذا (r) إرجاع r; // src
                r = v_اسحب_expect(v, dstType, &a); إذا (r) إرجاع r; // dst
                اكسر;
            }
            حالة 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                إذا (not v_has_memory_idx(v, memidx)) إرجاع m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_اسحب_expect(v, addrType, &a); إذا (r) إرجاع r;      // n
                r = v_اسحب_expect(v, c_m3Type_i32, &a); إذا (r) إرجاع r;  // val
                r = v_اسحب_expect(v, addrType, &a); إذا (r) إرجاع r;      // dst
                اكسر;
            }
#إذا d_m3HasWideArithmetic
            حالة 0x13: حالة 0x14: // i64.add128, i64.sub128
            حالة 0x15: حالة 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 رقم_Operands = (sub <= 0x14) ? 4 : 2;
                لكل (u32 i = 0; i < رقم_Operands; ++i) {
                    r = v_اسحب_expect(v, c_m3Type_i64, &a); إذا (r) إرجاع r;
                }
                r = v_ادفع(v, c_m3Type_i64); إذا (r) إرجاع r;
                r = v_ادفع(v, c_m3Type_i64); إذا (r) إرجاع r;
                اكسر;
            }
#endif
            افتراضي:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                اكسر;
            }
            اكسر;
        }

#إذا d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        حالة 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            إذا (r) إرجاع r;

            إذا (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                إذا (reserved != 0) إرجاع m3Err_wasmMalformed;
                اكسر;
            }

            u32 width, رقم_Operands; m3type_t قيمة_Type, resultType;
            إذا (not v_atomic_shape(sub, &width, &رقم_Operands, &قيمة_Type, &resultType)) إرجاع m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            إذا ((1u << align) != width) إرجاع m3Err_invalidAtomicAlignment;
            إذا (not v_has_memory_idx(v, memidx)) إرجاع m3Err_unknownMemory;
            إذا (not v_offset_in_range(v, memidx, offset)) إرجاع m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            إذا (sub == 0x01 or sub == 0x02) {
                r = v_اسحب_expect(v, c_m3Type_i64, &a); إذا (r) إرجاع r;
                رقم_Operands--;
            }
            لكل (u32 i = 1; i < رقم_Operands; ++i) {
                r = v_اسحب_expect(v, قيمة_Type, &a); إذا (r) إرجاع r;
            }
            r = v_اسحب_expect(v, v_memory_addrtype(v, memidx), &a); إذا (r) إرجاع r;
            إذا (resultType != c_m3Type_none) {
                r = v_ادفع(v, resultType); إذا (r) إرجاع r;
            }
            اكسر;
        }
#endif

#إذا d_m3_Has_مكدس_Switching
        // ---- Stack Switching ----
        حالة 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            إذا (!ft || !ft->isContinuation) إرجاع m3Err_typeMismatch;
            r = v_اسحب_expect(v, c_m3Type_funcref, &a); إذا (r) إرجاع r;
            r = v_ادفع(v, c_m3Type_contref); إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (typeIdx1 >= v->module->رقم_Func_Types || typeIdx2 >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            إذا (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) إرجاع m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 رقم_Params1 = ft1->رقم_Args;
            u16 رقم_Params2 = ft2->رقم_Args;
            إذا (رقم_Params1 < رقم_Params2) إرجاع m3Err_typeMismatch;
            u32 رقم_Bound = رقم_Params1 - رقم_Params2;

            إذا (ft1->رقم_Rets != ft2->رقم_Rets) إرجاع m3Err_typeMismatch;
            لكل (u16 i = 0; i < ft1->رقم_Rets; ++i) {
                إذا (ft1->types[i] != ft2->types[i]) إرجاع m3Err_typeMismatch;
            }
            لكل (u16 i = 0; i < رقم_Params2; ++i) {
                إذا (ft1->types[ft1->رقم_Rets + رقم_Bound + i] != ft2->types[ft2->رقم_Rets + i]) إرجاع m3Err_typeMismatch;
            }

            r = v_اسحب_expect(v, c_m3Type_contref, &a); إذا (r) إرجاع r;

            لكل (u32 i = رقم_Bound; i > 0; i--) {
                r = v_اسحب_expect(v, BaseTypeOf(ft1->types[ft1->رقم_Rets + i - 1]), &a);
                إذا (r) إرجاع r;
            }

            r = v_ادفع(v, c_m3Type_contref); إذا (r) إرجاع r;
            اكسر;
        }

        حالة 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (!v->module || tagIdx >= v->module->رقم_Tags) إرجاع m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            إذا (tt) {
                لكل (u16 i = tt->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(tt->types[tt->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
                لكل (u16 i = 0; i < tt->رقم_Rets; i++) {
                    r = v_ادفع(v, BaseTypeOf(tt->types[i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) إرجاع m3Err_typeMismatch;

            u32 رقم_Clauses;
            r = ReadLEB_u32(&رقم_Clauses, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;

            لكل (u32 i = 0; i < رقم_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                r = v_resume_clause(v, kind, tagIdx, ct); إذا (r) إرجاع r;
            }

            r = v_اسحب_expect(v, c_m3Type_contref, &a); إذا (r) إرجاع r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            إذا (ft) {
                لكل (u16 i = ft->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
                لكل (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_ادفع(v, BaseTypeOf(ft->types[i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, رقم_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            r = ReadLEB_u32(&رقم_Clauses, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;

            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            إذا (!v->module || tagIdx >= v->module->رقم_Tags) إرجاع m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) إرجاع m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            إذا (tagType && tagType->رقم_Rets != 0) إرجاع m3Err_typeMismatch;

            لكل (u32 i = 0; i < رقم_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                r = v_resume_clause(v, kind, cTagIdx, ct); إذا (r) إرجاع r;
            }

            r = v_اسحب_expect(v, c_m3Type_contref, &a); إذا (r) إرجاع r;

            إذا (tagType) {
                لكل (u16 i = tagType->رقم_Args; i > 0; i--) {
                    r = v_اسحب_expect(v, BaseTypeOf(tagType->types[tagType->رقم_Rets + i - 1]), &a);
                    إذا (r) إرجاع r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            إذا (ft) {
                لكل (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_ادفع(v, BaseTypeOf(ft->types[i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0xe5: // resume_throw_ref
        {
            u32 typeIdx, رقم_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            r = ReadLEB_u32(&رقم_Clauses, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;

            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) إرجاع m3Err_typeMismatch;

            لكل (u32 i = 0; i < رقم_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
                r = v_resume_clause(v, kind, cTagIdx, ct); إذا (r) إرجاع r;
            }

            r = v_اسحب_expect(v, c_m3Type_contref, &a); إذا (r) إرجاع r;
            r = v_اسحب_expect(v, c_m3Type_exnref, &a); إذا (r) إرجاع r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            إذا (ft) {
                لكل (u16 i = 0; i < ft->رقم_Rets; i++) {
                    r = v_ادفع(v, BaseTypeOf(ft->types[i]));
                    إذا (r) إرجاع r;
                }
            }
            اكسر;
        }

        حالة 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); إذا (r) إرجاع r;

            إذا (typeIdx >= v->module->رقم_Func_Types) إرجاع m3Err_unknownType;
            إذا (!v->module || tagIdx >= v->module->رقم_Tags) إرجاع m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) إرجاع m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            إذا (tagType && tagType->رقم_Args != 0) إرجاع m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            إذا (!ft || ft->رقم_Args == 0) إرجاع m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->رقم_Rets + ft->رقم_Args - 1]);
            إذا (!peer || !peer->contFuncType) إرجاع m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_اسحب_expect(v, c_m3Type_contref, &a); إذا (r) إرجاع r;

            لكل (u16 i = (u16)(ft->رقم_Args - 1); i > 0; i--) {
                r = v_اسحب_expect(v, BaseTypeOf(ft->types[ft->رقم_Rets + i - 1]), &a);
                إذا (r) إرجاع r;
            }

            لكل (u16 i = 0; i < self->رقم_Args; i++) {
                r = v_ادفع(v, BaseTypeOf(self->types[self->رقم_Rets + i]));
                إذا (r) إرجاع r;
            }
            اكسر;
        }

#endif // d_m3HasStackSwitching

        افتراضي:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            اكسر;

        } // switch

        إذا (r) إرجاع r;

    } // while

    // If we ran out of bytes without hitting the final end
    إرجاع m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    إذا (!i_function->wasm) إرجاع m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    إذا (!v) {
        v = m3_AllocStruct (ValCtx);
        إذا (!v) إرجاع m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, حجم(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    إذا (r) إرجاع r;

    // Parse locals
    u32 رقم_Local_Blocks;
    r = ReadLEB_u32(&رقم_Local_Blocks, &v->wasm, v->wasmEnd);
    إذا (r) إرجاع r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 رقم_Params = funcType ? funcType->رقم_Args : 0;
    إذا (رقم_Params > d_m3_Val_مكدس) إرجاع m3_Err_function_مكدس_Overflow;
    لكل (u16 i = 0; i < رقم_Params; i++) {
        v->localTypes[v->رقم_Locals++] = BaseTypeOf(funcType->types[funcType->رقم_Rets + i]);
    }

    // Then: declared locals
    لكل (u32 b = 0; b < رقم_Local_Blocks; b++) {
        u32 تعداد_2;
        r = ReadLEB_u32(&تعداد_2, &v->wasm, v->wasmEnd);
        إذا (r) إرجاع r;
        m3type_t localType;
        r = Parse_قيمة_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        إذا (r) إرجاع r;
        u8 normalized = BaseTypeOf(localType);
        إذا (تعداد_2 > (u32) (d_m3_Val_مكدس - v->رقم_Locals)) إرجاع m3_Err_function_مكدس_Overflow;
        لكل (u32 c = 0; c < تعداد_2; c++) {
            v->localTypes[v->رقم_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ادفع_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    إذا (r) إرجاع r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    إذا (r) إرجاع r;

    // After validation, control stack should be empty and all declared bytes consumed
    إذا (v->ctrl_قمة != 0)
        إرجاع m3Err_wasmMalformed;

    إذا (v->wasm != v->wasmEnd)
        إرجاع m3Err_wasmSectionUnderrun;

    إرجاع m3Err_none;
}

#وإلا // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (فراغ)i_function;
    إرجاع m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
