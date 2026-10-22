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
#تعریف_کرو c_valBottom         0xFF

// ---------- Control frame ----------

قسمتعریف ساخت {
    m3opcode_t  opcode;
    u16         اونچائی;         // operand stack height at block entry
    u16         param_گنتی;
    u16         نتیجہ_گنتی;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} Val_Ctrl_چوکھٹا;

// ---------- Validator context ----------

ساخت ValCtx {
    بائٹیں_t     wasm;
    بائٹیں_t     wasm_انتہا;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ڈھیر];
    u16         opd_چوٹی;

    Val_Ctrl_چوکھٹا ctrl [d_m3_Val_Ctrl_گہرائی];
    u16          ctrl_چوٹی;

    m3type_t    localTypes [d_m3_Val_ڈھیر];
    u16         عدد_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ساکن bool v_has_حافظہ_idx (ValCtx * v, u32 i_حافظہ_Idx)
{
    واپس v->module and i_حافظہ_Idx < v->module->عدد_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ساکن m3type_t v_حافظہ_addrtype (ValCtx * v, u32 i_حافظہ_Idx)
{
    واپس حافظہ_Addr_Type (v->module->memories [i_حافظہ_Idx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ساکن bool v_انحراف_اندر_range (ValCtx * v, u32 i_حافظہ_Idx, u64 i_انحراف)
{
    واپس v_حافظہ_addrtype (v, i_حافظہ_Idx) == c_m3Type_i64 or i_انحراف <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ساکن m3type_t v_جدول_addrtype (ValCtx * v, u32 i_جدول_Idx)
{
    واپس جدول_Addr_Type (v->module->tables [i_جدول_Idx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
ساکن u32 v_max_align (m3opcode_t opcode)
{
    انتخاب (opcode) {
    حالت 0x2c: حالت 0x2d:   // i32.load8_s, i32.load8_u
    حالت 0x30: حالت 0x31:   // i64.load8_s, i64.load8_u
    حالت 0x3a:              // i32.store8
    حالت 0x3c:              // i64.store8
        واپس 0;
    حالت 0x2e: حالت 0x2f:   // i32.load16_s, i32.load16_u
    حالت 0x32: حالت 0x33:   // i64.load16_s, i64.load16_u
    حالت 0x3b:              // i32.store16
    حالت 0x3d:              // i64.store16
        واپس 1;
    حالت 0x29:              // i64.load
    حالت 0x2b:              // f64.load
    حالت 0x37:              // i64.store
    حالت 0x39:              // f64.store
        واپس 3;
    طےشدہ:                // 32-bit accesses, and a safe fallback
        واپس 2;
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
ساکن bool v_atomic_shape (u32 sub, u32 * o_چوڑائی, u32 * o_عدد_Operands, m3type_t * o_قدر_Type, m3type_t * o_نتیجہ_Type)
{
    ساکن ثابت u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ساکن ثابت m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    اگر (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_چوڑائی       = (sub == 0x02) ? 8 : 4;
        *o_عدد_Operands = (sub == 0x00) ? 2 : 3;
        *o_قدر_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_نتیجہ_Type  = c_m3Type_i32;
        واپس true;
    }

    اگر (sub < 0x10 or sub > c_wa_Op_آخری_Atomic) واپس false;

    اگر (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_عدد_Operands = 1;
    } ورنہ اگر (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_عدد_Operands = 2;
    } ورنہ اگر (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_عدد_Operands = 2;
    } ورنہ {                        // cmpxchg
        variant = sub - 0x48;       *o_عدد_Operands = 3;
    }

    *o_چوڑائی      = c_widths [variant];
    *o_قدر_Type  = c_types [variant];
    *o_نتیجہ_Type = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    واپس true;
}

#اگر_ختم

// ---------- Operand stack ----------

ساکن M3_نتیجہ v_ڈالو (ValCtx * v, m3type_t type)
{
    اگر (M3_UNLIKELY(v->opd_چوٹی >= d_m3_Val_ڈھیر))
        واپس m3_Err_function_ڈھیر_حد_سے_تجاوز;
    v->opd[v->opd_چوٹی++] = type;
    واپس m3Err_none;
}

ساکن M3_نتیجہ v_نکالو (ValCtx * v, m3type_t * o_type)
{
    Val_Ctrl_چوکھٹا * f = &v->ctrl[v->ctrl_چوٹی - 1];
    اگر (M3_UNLIKELY(v->opd_چوٹی == f->اونچائی)) {
        اگر (f->is_unreachable) { *o_type = c_valBottom; واپس m3Err_none; }
        واپس m3_Err_function_ڈھیر_Underrun;
    }
    *o_type = v->opd[--v->opd_چوٹی];
    واپس m3Err_none;
}

ساکن M3_نتیجہ v_نکالو_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3_نتیجہ r = v_نکالو(v, &actual);
    اگر (M3_UNLIKELY(r)) واپس r;
    اگر (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        واپس m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    واپس m3Err_none;
}

// ---------- Control stack ----------

ساکن M3_نتیجہ v_ڈالو_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    اگر (M3_UNLIKELY(v->ctrl_چوٹی >= d_m3_Val_Ctrl_گہرائی))
        واپس m3_Err_function_ڈھیر_حد_سے_تجاوز;
    Val_Ctrl_چوکھٹا * f = &v->ctrl[v->ctrl_چوٹی++];
    f->opcode       = op;
    f->type         = type;
    f->param_گنتی  = type ? type->عدد_دلائل : 0;
    f->نتیجہ_گنتی = type ? type->عدد_Rets : 0;
    f->اونچائی       = v->opd_چوٹی;
    f->is_unreachable = false;
    واپس m3Err_none;
}

ساکن M3_نتیجہ v_نکالو_ctrl (ValCtx * v, Val_Ctrl_چوکھٹا * o_چوکھٹا)
{
    اگر (M3_UNLIKELY(v->ctrl_چوٹی == 0))
        واپس m3Err_wasmMalformed;
    Val_Ctrl_چوکھٹا * f = &v->ctrl[v->ctrl_چوٹی - 1];
    // pop result types
    اگر (f->type) {
        برائے (u16 i = f->نتیجہ_گنتی; i > 0; i--) {
            m3type_t a;
            M3_نتیجہ r = v_نکالو_expect(v, بنیاد_Type_کا(f->type->types[i - 1]), &a);
            اگر (M3_UNLIKELY(r)) واپس r;
        }
    }
    اگر (M3_UNLIKELY(v->opd_چوٹی != f->اونچائی))
        واپس m3_Err_type_گنتی_Mismatch;
    اگر (o_چوکھٹا) *o_چوکھٹا = *f;
    v->ctrl_چوٹی--;
    واپس m3Err_none;
}

ساکن خالی v_unreachable (ValCtx * v)
{
    Val_Ctrl_چوکھٹا * f = &v->ctrl[v->ctrl_چوٹی - 1];
    v->opd_چوٹی = f->اونچائی;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ساکن u16 v_label_n (Val_Ctrl_چوکھٹا * f)
{
    واپس (f->opcode == 0x03) ? f->param_گنتی : f->نتیجہ_گنتی;
}

ساکن m3type_t v_label_t (Val_Ctrl_چوکھٹا * f, u16 i)
{
    اگر (!f->type) واپس c_m3Type_none;
    اگر (f->opcode == 0x03)
        واپس بنیاد_Type_کا(f->type->types[f->type->عدد_Rets + i]); // params
    واپس بنیاد_Type_کا(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ساکن m3type_t v_peek (ValCtx * v, u16 i_گہرائی)
{
    Val_Ctrl_چوکھٹا * f = &v->ctrl[v->ctrl_چوٹی - 1];

    اگر (v->opd_چوٹی <= i_گہرائی || (u16)(v->opd_چوٹی - i_گہرائی - 1) < f->اونچائی)
        واپس c_valBottom;

    واپس v->opd[v->opd_چوٹی - i_گہرائی - 1];
}

// Pop label types for branch target
ساکن M3_نتیجہ v_نکالو_labels (ValCtx * v, Val_Ctrl_چوکھٹا * tgt)
{
    u16 n = v_label_n(tgt);
    برائے (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3_نتیجہ r = v_نکالو_expect(v, v_label_t(tgt, i - 1), &a);
        اگر (M3_UNLIKELY(r)) واپس r;
    }
    واپس m3Err_none;
}

// Push label types back
ساکن M3_نتیجہ v_ڈالو_labels (ValCtx * v, Val_Ctrl_چوکھٹا * tgt)
{
    u16 n = v_label_n(tgt);
    برائے (u16 i = 0; i < n; i++) {
        M3_نتیجہ r = v_ڈالو(v, v_label_t(tgt, i));
        اگر (M3_UNLIKELY(r)) واپس r;
    }
    واپس m3Err_none;
}

// ---------- Block type resolution ----------

ساکن M3_نتیجہ v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    اگر (v->wasm >= v->wasm_انتہا)
        واپس m3Err_wasmUnderrun;

#اگر d_m3HasTypedRefs
    اگر (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3_نتیجہ rr = Parse_قدر_Type(v->module, &refType, &v->wasm, v->wasm_انتہا);
        اگر (rr) واپس rr;
        IM3FuncType ftype = تہی;
        rr = AllocFuncType(&ftype, 1);
        اگر (rr) واپس rr;
        ftype->عدد_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_جوڑو_Func_Type(v->module->environment, &ftype);
        اگر (rr) واپس rr;
        *o_type = ftype;
        واپس m3Err_none;
    }
#اگر_ختم

    i64 type;
    M3_نتیجہ r = ReadLebSigned(&type, 33, &v->wasm, v->wasm_انتہا);
    اگر (r) واپس r;

    اگر (type < 0) {
        اگر (type < -64) واپس m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        اگر (r) واپس r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } ورنہ {
        اگر ((u32)type >= v->module->عدد_Func_Types)
            واپس m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    واپس m3Err_none;
}

// ---------- Convenience ----------

ساکن M3_نتیجہ v_unop (ValCtx * v, m3type_t اندر, m3type_t برآمد)
{
    m3type_t a; M3_نتیجہ r = v_نکالو_expect(v, اندر, &a);
    اگر (M3_UNLIKELY(r)) واپس r;
    واپس v_ڈالو(v, برآمد);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ساکن M3_نتیجہ v_check_دم_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : تہی;

    u16 عدد_Callee_Rets = i_calleeType ? i_calleeType->عدد_Rets : 0;
    u16 عدد_Func_Rets   = ft ? ft->عدد_Rets : 0;

    اگر (عدد_Callee_Rets != عدد_Func_Rets)
        واپس m3Err_typeMismatch;

    برائے (u16 i = 0; i < عدد_Callee_Rets; i++) {
        اگر (i_calleeType->types[i] != ft->types[i])
            واپس m3Err_typeMismatch;
    }

    واپس m3Err_none;
}

ساکن M3_نتیجہ v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_نتیجہ r;
    r = v_نکالو_expect(v, t, &a); اگر (M3_UNLIKELY(r)) واپس r;
    r = v_نکالو_expect(v, t, &a); اگر (M3_UNLIKELY(r)) واپس r;
    واپس v_ڈالو(v, t);
}

ساکن M3_نتیجہ v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_نتیجہ r;
    r = v_نکالو_expect(v, t, &a); اگر (M3_UNLIKELY(r)) واپس r;
    r = v_نکالو_expect(v, t, &a); اگر (M3_UNLIKELY(r)) واپس r;
    واپس v_ڈالو(v, c_m3Type_i32);
}

ساکن M3_نتیجہ v_testop (ValCtx * v, m3type_t t)
{
    واپس v_unop(v, t, c_m3Type_i32);
}

ساکن M3_نتیجہ v_cvtop (ValCtx * v, m3type_t اندر, m3type_t برآمد)
{
    واپس v_unop(v, اندر, برآمد);
}


// ---------- Main validation loop ----------

#اگر d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ساکن M3_نتیجہ v_catch_clause (ValCtx * v)
{
    u8 قسم;
    M3_نتیجہ r = Read_u8(&قسم, &v->wasm, v->wasm_انتہا);
    اگر (r) واپس r;
    اگر (قسم > 0x03) واپس m3Err_wasmMalformed;

    bool has_لیبل = (قسم == 0x00 || قسم == 0x01);
    bool hasRef = (قسم == 0x01 || قسم == 0x03);

    IM3FuncType لیبل_Type = تہی;
    اگر (has_لیبل) {
        u32 لیبل_اشاریہ;
        r = ReadLEB_u32(&لیبل_اشاریہ, &v->wasm, v->wasm_انتہا);
        اگر (r) واپس r;
        اگر (!v->module || لیبل_اشاریہ >= v->module->عدد_لیبل_فہرست) واپس m3_Err_unknown_لیبل;
        لیبل_Type = v->module->لیبل_فہرست[لیبل_اشاریہ].type;
    }

    u32 گہرائی;
    r = ReadLEB_u32(&گہرائی, &v->wasm, v->wasm_انتہا);
    اگر (r) واپس r;
    اگر (گہرائی >= v->ctrl_چوٹی) واپس m3Err_unknownLabel;

    Val_Ctrl_چوکھٹا * tgt = &v->ctrl[v->ctrl_چوٹی - 1 - گہرائی];

    u16 عدد_Payload = لیبل_Type ? لیبل_Type->عدد_دلائل : 0;
    u16 عدد_Label   = v_label_n(tgt);

    اگر (عدد_Label != عدد_Payload + (hasRef ? 1u : 0u))
        واپس m3_Err_type_گنتی_Mismatch;

    برائے (u16 i = 0; i < عدد_Payload; i++) {
        اگر (v_label_t(tgt, i) != بنیاد_Type_کا(لیبل_Type->types[لیبل_Type->عدد_Rets + i]))
            واپس m3Err_typeMismatch;
    }

    اگر (hasRef and v_label_t(tgt, عدد_Payload) != c_m3Type_exnref)
        واپس m3Err_typeMismatch;

    واپس m3Err_none;
}

#اگر_ختم // d_m3HasExceptionHandling

#اگر d_m3_Has_ڈھیر_Switching

ساکن M3_نتیجہ v_resume_clause (ValCtx * v, u8 قسم, u32 لیبل_Idx, IM3FuncType ct)
{
    اگر (قسم > 0x01) واپس m3Err_wasmMalformed;
    اگر (!v->module || لیبل_Idx >= v->module->عدد_لیبل_فہرست) واپس m3_Err_unknown_لیبل;
    IM3FuncType لیبل_Type = v->module->لیبل_فہرست[لیبل_Idx].type;

    اگر (قسم == 0x00) {
        u32 گہرائی;
        M3_نتیجہ r = ReadLEB_u32(&گہرائی, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
        اگر (گہرائی >= v->ctrl_چوٹی) واپس m3Err_unknownLabel;

        Val_Ctrl_چوکھٹا * tgt = &v->ctrl[v->ctrl_چوٹی - 1 - گہرائی];
        u16 عدد_Payload = لیبل_Type ? لیبل_Type->عدد_دلائل : 0;
        u16 عدد_Label   = v_label_n(tgt);

        اگر (عدد_Label != عدد_Payload + 1u) واپس m3_Err_type_گنتی_Mismatch;

        برائے (u16 j = 0; j < عدد_Payload; j++) {
            اگر (v_label_t(tgt, j) != بنیاد_Type_کا(لیبل_Type->types[لیبل_Type->عدد_Rets + j]))
                واپس m3Err_typeMismatch;
        }
        اگر (v_label_t(tgt, عدد_Payload) != c_m3Type_contref)
            واپس m3Err_typeMismatch;

#اگر d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->عدد_Rets + عدد_Payload]
            : tgt->type->types[عدد_Payload];

        اگر (ڈھیر_Type_کا(labelContType) == d_m3_Type_ڈھیر_Abstract ||
            ڈھیر_Type_کا(labelContType) == d_m3_Type_ڈھیر_None) {
            واپس m3Err_typeMismatch;
        }

        u32 ڈھیر_Type_2 = ڈھیر_Type_کا(labelContType);
        IM3FuncType labelCt = تہی;
        برائے (u32 idx = 0; idx < v->module->عدد_Func_Types; ++idx) {
            اگر (v->module->funcTypes[idx]->canonical_اشاریہ == ڈھیر_Type_2 &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                توڑو;
            }
        }
        اگر (!labelCt) واپس m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_عدد_Params = لیبل_Type ? لیبل_Type->عدد_Rets : 0;
        اگر (labelFt->عدد_دلائل != expected_عدد_Params) واپس m3Err_typeMismatch;
        برائے (u16 p = 0; p < expected_عدد_Params; ++p) {
            اگر (labelFt->types[labelFt->عدد_Rets + p] != لیبل_Type->types[p])
                واپس m3Err_typeMismatch;
        }

        u16 expected_عدد_Rets = ft ? ft->عدد_Rets : 0;
        اگر (labelFt->عدد_Rets != expected_عدد_Rets) واپس m3Err_typeMismatch;
        برائے (u16 r_idx = 0; r_idx < expected_عدد_Rets; ++r_idx) {
            اگر (labelFt->types[r_idx] != ft->types[r_idx])
                واپس m3Err_typeMismatch;
        }
#اگر_ختم
    } ورنہ {
        اگر (لیبل_Type && لیبل_Type->عدد_دلائل != 0) واپس m3Err_typeMismatch;
    }
    واپس m3Err_none;
}

#اگر_ختم // d_m3HasStackSwitching

ساکن M3_نتیجہ v_validate_body (ValCtx * v)
{
    M3_نتیجہ r = m3Err_none;
    m3type_t a = c_valBottom;

    جب (v->wasm < v->wasm_انتہا)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasm_انتہا);
        اگر (M3_UNLIKELY(r)) واپس r;

        انتخاب (opcode)
        {
        // ---- Control ----
        حالت 0x00: // unreachable
            v_unreachable(v);
            توڑو;

        حالت 0x01: // nop
            توڑو;

        حالت 0x02: //بلاک
        حالت 0x03: // loop
        حالت 0x04: // if
        {
            IM3FuncType bt = تہی;
            r = v_read_blocktype(v, &bt);
            اگر (r) واپس r;
            اگر (opcode == 0x04) {
                r = v_نکالو_expect(v, c_m3Type_i32, &a);
                اگر (r) واپس r;
            }
            // Pop block params from caller stack
            اگر (bt) {
                برائے (u16 i = bt->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(bt->types[bt->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
            }
            r = v_ڈالو_ctrl(v, opcode, bt);
            اگر (r) واپس r;
            // Push params inside block
            اگر (bt) {
                برائے (u16 i = 0; i < bt->عدد_دلائل; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(bt->types[bt->عدد_Rets + i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

#اگر d_m3HasExceptionHandling
        حالت 0x1f: // try_table
        {
            IM3FuncType bt = تہی;
            r = v_read_blocktype(v, &bt);
            اگر (r) واپس r;
            اگر (bt) {
                برائے (u16 i = bt->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(bt->types[bt->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 عدد_Catch;
            r = ReadLEB_u32(&عدد_Catch, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            برائے (u32 i = 0; i < عدد_Catch; i++) {
                r = v_catch_clause(v);
                اگر (r) واپس r;
            }

            r = v_ڈالو_ctrl(v, opcode, bt);
            اگر (r) واپس r;

            اگر (bt) {
                برائے (u16 i = 0; i < bt->عدد_دلائل; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(bt->types[bt->عدد_Rets + i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0x08: // throw
        {
            u32 لیبل_اشاریہ;
            r = ReadLEB_u32(&لیبل_اشاریہ, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (!v->module || لیبل_اشاریہ >= v->module->عدد_لیبل_فہرست) واپس m3_Err_unknown_لیبل;
            IM3FuncType tt = v->module->لیبل_فہرست[لیبل_اشاریہ].type;
            اگر (tt) {
                برائے (u16 i = tt->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(tt->types[tt->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
            }
            v_unreachable(v);
            توڑو;
        }

        حالت 0x0a: // throw_ref
            r = v_نکالو_expect(v, c_m3Type_exnref, &a);
            اگر (r) واپس r;
            v_unreachable(v);
            توڑو;
#اگر_ختم // d_m3HasExceptionHandling

        حالت 0x05: // else
        {
            Val_Ctrl_چوکھٹا چوکھٹا;
            r = v_نکالو_ctrl(v, &چوکھٹا);
            اگر (r) واپس r;
            اگر (چوکھٹا.opcode != 0x04)
                واپس m3Err_wasmMalformed;
            r = v_ڈالو_ctrl(v, 0x05, چوکھٹا.type);
            اگر (r) واپس r;
            اگر (چوکھٹا.type) {
                برائے (u16 i = 0; i < چوکھٹا.type->عدد_دلائل; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(چوکھٹا.type->types[چوکھٹا.type->عدد_Rets + i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0x0b: //انتہا
        {
            Val_Ctrl_چوکھٹا چوکھٹا;
            r = v_نکالو_ctrl(v, &چوکھٹا);
            اگر (r) واپس r;
            // Push results
            اگر (چوکھٹا.type) {
                برائے (u16 i = 0; i < چوکھٹا.نتیجہ_گنتی; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(چوکھٹا.type->types[i]));
                    اگر (r) واپس r;
                }
            }
            // If this was the outermost frame, we're done
            اگر (v->ctrl_چوٹی == 0)
                واپس m3Err_none;
            توڑو;
        }

        حالت 0x0c: // br
        {
            u32 گہرائی;
            r = ReadLEB_u32(&گہرائی, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (گہرائی >= v->ctrl_چوٹی) واپس m3Err_unknownLabel;
            Val_Ctrl_چوکھٹا * tgt = &v->ctrl[v->ctrl_چوٹی - 1 - گہرائی];
            r = v_نکالو_labels(v, tgt);
            اگر (r) واپس r;
            v_unreachable(v);
            توڑو;
        }

        حالت 0x0d: // br_if
        {
            u32 گہرائی;
            r = ReadLEB_u32(&گہرائی, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (گہرائی >= v->ctrl_چوٹی) واپس m3Err_unknownLabel;
            r = v_نکالو_expect(v, c_m3Type_i32, &a);
            اگر (r) واپس r;
            Val_Ctrl_چوکھٹا * tgt = &v->ctrl[v->ctrl_چوٹی - 1 - گہرائی];
            r = v_نکالو_labels(v, tgt);
            اگر (r) واپس r;
            r = v_ڈالو_labels(v, tgt);
            اگر (r) واپس r;
            توڑو;
        }

        حالت 0x0e: // br_table
        {
            u32 گنتی;
            r = ReadLEB_u32(&گنتی, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            u32 def_گہرائی = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            بائٹیں_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            برائے (u32 i = 0; i <= گنتی; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_انتہا);
                اگر (r) واپس r;
                اگر (d >= v->ctrl_چوٹی) واپس m3Err_unknownLabel;
                اگر (i == گنتی) def_گہرائی = d;
            }
            // Now validate all labels match the default's types
            Val_Ctrl_چوکھٹا * defTgt = &v->ctrl[v->ctrl_چوٹی - 1 - def_گہرائی];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_نکالو_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            اگر (r) واپس r;

            برائے (u32 i = 0; i <= گنتی; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_انتہا);
                اگر (r) واپس r;
                Val_Ctrl_چوکھٹا * t = &v->ctrl[v->ctrl_چوٹی - 1 - d];
                u16 n = v_label_n(t);
                اگر (n != arity) واپس m3_Err_type_گنتی_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                برائے (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    اگر (want != c_valBottom && have != c_valBottom && want != have)
                        واپس m3Err_typeMismatch;
                }
            }
            Val_Ctrl_چوکھٹا * dt = &v->ctrl[v->ctrl_چوٹی - 1 - def_گہرائی];
            r = v_نکالو_labels(v, dt);
            اگر (r) واپس r;
            v_unreachable(v);
            توڑو;
        }

        حالت 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            اگر (ft) {
                برائے (u16 i = ft->عدد_Rets; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[i - 1]), &a);
                    اگر (r) واپس r;
                }
            }
            v_unreachable(v);
            توڑو;
        }

        // ---- Call ----
        حالت 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (idx >= v->module->عدد_Functions) واپس m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            اگر (ft) {
                برائے (u16 i = ft->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[ft->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
                برائے (u16 i = 0; i < ft->عدد_Rets; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(ft->types[i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            u32 جدول_Idx;
            r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            // the table must exist and hold funcrefs
            اگر (جدول_Idx >= v->module->عدد_Tables) واپس m3_Err_unknown_جدول;
            اگر (v->module->tables[جدول_Idx]->type != c_m3Type_funcref) واپس m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_نکالو_expect(v, v_جدول_addrtype(v, جدول_Idx), &a); // table index operand
            اگر (r) واپس r;
            اگر (ft) {
                برائے (u16 i = ft->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[ft->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
                برائے (u16 i = 0; i < ft->عدد_Rets; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(ft->types[i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (idx >= v->module->عدد_Functions) واپس m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_دم_results(v, ft);
            اگر (r) واپس r;
            اگر (ft) {
                برائے (u16 i = ft->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[ft->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
            }
            v_unreachable(v);
            توڑو;
        }

        حالت 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            u32 جدول_Idx;
            r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            // the table must exist and hold funcrefs
            اگر (جدول_Idx >= v->module->عدد_Tables) واپس m3_Err_unknown_جدول;
            اگر (v->module->tables[جدول_Idx]->type != c_m3Type_funcref) واپس m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_دم_results(v, ft);
            اگر (r) واپس r;
            r = v_نکالو_expect(v, v_جدول_addrtype(v, جدول_Idx), &a); // table index operand
            اگر (r) واپس r;
            اگر (ft) {
                برائے (u16 i = ft->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[ft->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
            }
            v_unreachable(v);
            توڑو;
        }

#اگر d_m3HasTypedRefs
        حالت 0x14: // call_ref
        حالت 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool is_دم = (opcode == 0x15);
            اگر (is_دم) {
                r = v_check_دم_results(v, ft);
                اگر (r) واپس r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_نکالو_expect(v, c_m3Type_funcref, &a);
            اگر (r) واپس r;
            اگر (ft) {
                برائے (u16 i = ft->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[ft->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
                اگر (not is_دم) {
                    برائے (u16 i = 0; i < ft->عدد_Rets; i++) {
                        r = v_ڈالو(v, بنیاد_Type_کا(ft->types[i]));
                        اگر (r) واپس r;
                    }
                }
            }
            اگر (is_دم) v_unreachable(v);
            توڑو;
        }

        حالت 0xd4: // ref.as_non_null
        {
            r = v_نکالو_expect(v, c_m3Type_funcref, &a);
            اگر (r) واپس r;
            r = v_ڈالو(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            اگر (r) واپس r;
            توڑو;
        }
#اگر_ختم // d_m3HasTypedRefs

        // ---- Parametric ----
        حالت 0x1a: // drop
            r = v_نکالو(v, &a);
            اگر (r) واپس r;
            توڑو;

        حالت 0x1b: // select
        {
            r = v_نکالو_expect(v, c_m3Type_i32, &a);
            اگر (r) واپس r;
            m3type_t t2 = c_valBottom;
            r = v_نکالو(v, &t2);
            اگر (r) واپس r;
            m3type_t t1;
            r = v_نکالو_expect(v, t2, &t1);
            اگر (r) واپس r;
            // untyped select is numeric only; references need the 0x1c form
            اگر (t2 != c_valBottom && IsRefType(t2)) واپس m3Err_typeMismatch;
            r = v_ڈالو(v, (t2 == c_valBottom) ? t1 : t2);
            اگر (r) واپس r;
            توڑو;
        }

#اگر d_m3HasRefTypes
        حالت 0x1c: // select with an explicit result type
        {
            u32 عدد_Types;
            r = ReadLEB_u32(&عدد_Types, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (عدد_Types != 1) واپس m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_قدر_Type(v->module, &selType, &v->wasm, v->wasm_انتہا);  اگر (r) واپس r;
            u8 t = بنیاد_Type_کا(selType);

            r = v_نکالو_expect(v, c_m3Type_i32, &a); اگر (r) واپس r;
            r = v_نکالو_expect(v, t, &a);            اگر (r) واپس r;
            r = v_نکالو_expect(v, t, &a);            اگر (r) واپس r;
            r = v_ڈالو(v, t);                      اگر (r) واپس r;
            توڑو;
        }

        حالت 0x25: // table.get
        حالت 0x26: // table.set
        {
            u32 جدول_Idx;
            r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (جدول_Idx >= v->module->عدد_Tables) واپس m3_Err_unknown_جدول;
            u8 t = بنیاد_Type_کا(v->module->tables[جدول_Idx]->type);
            m3type_t at = v_جدول_addrtype(v, جدول_Idx);

            اگر (opcode == 0x26) {
                r = v_نکالو_expect(v, t, &a);   اگر (r) واپس r;
                r = v_نکالو_expect(v, at, &a);  اگر (r) واپس r;
            } ورنہ {
                r = v_نکالو_expect(v, at, &a);  اگر (r) واپس r;
                r = v_ڈالو(v, t);             اگر (r) واپس r;
            }
            توڑو;
        }

        حالت 0xd0: // ref.null
        {
#اگر d_m3HasTypedRefs
            m3type_t ڈھیر_بٹ;
            r = Parse_ڈھیر_Type(v->module, &ڈھیر_بٹ, &v->wasm, v->wasm_انتہا);  اگر (r) واپس r;
            u8 t = (ڈھیر_بٹ & d_m3Type_refExtern) ? c_m3Type_externref :
# اگر d_m3_Has_ڈھیر_Switching
                   (ڈھیر_بٹ & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#ورنہ
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasm_انتہا);   اگر (r) واپس r;
            r = NormalizeType(&t, waType);                   اگر (r) واپس r;
            اگر (!IsRefType(t)) واپس m3Err_wasmMalformed;
#اگر_ختم
            r = v_ڈالو(v, t);                                اگر (r) واپس r;
            توڑو;
        }

        حالت 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_نکالو(v, &t);
            اگر (r) واپس r;
            اگر (t != c_valBottom && !IsRefType(t)) واپس m3Err_typeMismatch;
            r = v_ڈالو(v, c_m3Type_i32);
            اگر (r) واپس r;
            توڑو;
        }

        حالت 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (funcIdx >= v->module->عدد_Functions) واپس m3Err_unknownFunction;
            اگر (!Module_IsFunctionDeclared(v->module, funcIdx)) واپس m3Err_undeclaredFuncRef;
            r = v_ڈالو(v, c_m3Type_funcref);
            اگر (r) واپس r;
            توڑو;
        }
#اگر_ختم

        // ---- Variable ----
        حالت 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (idx >= v->عدد_Locals) واپس m3Err_unknownLocal;
            r = v_ڈالو(v, v->localTypes[idx]);
            اگر (r) واپس r;
            توڑو;
        }

        حالت 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (idx >= v->عدد_Locals) واپس m3Err_unknownLocal;
            r = v_نکالو_expect(v, v->localTypes[idx], &a);
            اگر (r) واپس r;
            توڑو;
        }

        حالت 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (idx >= v->عدد_Locals) واپس m3Err_unknownLocal;
            r = v_نکالو_expect(v, v->localTypes[idx], &a);
            اگر (r) واپس r;
            r = v_ڈالو(v, v->localTypes[idx]);
            اگر (r) واپس r;
            توڑو;
        }

        حالت 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (idx >= v->module->عدد_Globals) واپس m3Err_unknownGlobal;
            r = v_ڈالو(v, بنیاد_Type_کا(v->module->globals[idx].type));
            اگر (r) واپس r;
            توڑو;
        }

        حالت 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            اگر (idx >= v->module->عدد_Globals) واپس m3Err_unknownGlobal;
            r = v_نکالو_expect(v, بنیاد_Type_کا(v->module->globals[idx].type), &a);
            اگر (r) واپس r;
            توڑو;
        }

        // ---- Memory load ----
        حالت 0x28: حالت 0x29: حالت 0x2a: حالت 0x2b: // i32/i64/f32/f64.load
        حالت 0x2c: حالت 0x2d: حالت 0x2e: حالت 0x2f: // i32.load8/16 s/u
        حالت 0x30: حالت 0x31: حالت 0x32: حالت 0x33: // i64.load8/16 s/u
        حالت 0x34: حالت 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = Read_حافظہ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (align > v_max_align(opcode)) واپس m3Err_invalidAlignment;
            اگر (not v_has_حافظہ_idx(v, memidx)) واپس m3_Err_unknown_حافظہ;
            اگر (not v_انحراف_اندر_range(v, memidx, offset)) واپس m3Err_wasmMalformed;
            r = v_نکالو_expect(v, v_حافظہ_addrtype(v, memidx), &a); اگر (r) واپس r;
            u8 نتیجہ;
            اگر      (opcode == 0x28) نتیجہ = c_m3Type_i32;
            ورنہ اگر (opcode == 0x29) نتیجہ = c_m3Type_i64;
            ورنہ اگر (opcode == 0x2a) نتیجہ = c_m3Type_f32;
            ورنہ اگر (opcode == 0x2b) نتیجہ = c_m3Type_f64;
            ورنہ اگر (opcode <= 0x2f) نتیجہ = c_m3Type_i32;
            ورنہ                     نتیجہ = c_m3Type_i64;
            r = v_ڈالو(v, نتیجہ);
            اگر (r) واپس r;
            توڑو;
        }

        // ---- Memory store ----
        حالت 0x36: حالت 0x37: حالت 0x38: حالت 0x39: // i32/i64/f32/f64.store
        حالت 0x3a: حالت 0x3b:                         // i32.store8/16
        حالت 0x3c: حالت 0x3d: حالت 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = Read_حافظہ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (align > v_max_align(opcode)) واپس m3Err_invalidAlignment;
            اگر (not v_has_حافظہ_idx(v, memidx)) واپس m3_Err_unknown_حافظہ;
            اگر (not v_انحراف_اندر_range(v, memidx, offset)) واپس m3Err_wasmMalformed;
            u8 valtype;
            اگر      (opcode == 0x36) valtype = c_m3Type_i32;
            ورنہ اگر (opcode == 0x37) valtype = c_m3Type_i64;
            ورنہ اگر (opcode == 0x38) valtype = c_m3Type_f32;
            ورنہ اگر (opcode == 0x39) valtype = c_m3Type_f64;
            ورنہ اگر (opcode <= 0x3b) valtype = c_m3Type_i32;
            ورنہ                     valtype = c_m3Type_i64;
            r = v_نکالو_expect(v, valtype, &a); اگر (r) واپس r;
            r = v_نکالو_expect(v, v_حافظہ_addrtype(v, memidx), &a); اگر (r) واپس r;
            توڑو;
        }

        // ---- Memory size/grow ----
        حالت 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (not v_has_حافظہ_idx(v, memidx)) واپس m3_Err_unknown_حافظہ;
            r = v_ڈالو(v, v_حافظہ_addrtype(v, memidx)); اگر (r) واپس r;
            توڑو;
        }
        حالت 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (not v_has_حافظہ_idx(v, memidx)) واپس m3_Err_unknown_حافظہ;
            r = v_نکالو_expect(v, v_حافظہ_addrtype(v, memidx), &a); اگر (r) واپس r;
            r = v_ڈالو(v, v_حافظہ_addrtype(v, memidx)); اگر (r) واپس r;
            توڑو;
        }

        // ---- Constants ----
        حالت 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            r = v_ڈالو(v, c_m3Type_i32); اگر (r) واپس r;
            توڑو;
        }
        حالت 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            r = v_ڈالو(v, c_m3Type_i64); اگر (r) واپس r;
            توڑو;
        }
        حالت 0x43: { // f32.const
            اگر (v->wasm + 4 > v->wasm_انتہا) واپس m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ڈالو(v, c_m3Type_f32); اگر (r) واپس r;
            توڑو;
        }
        حالت 0x44: { // f64.const
            اگر (v->wasm + 8 > v->wasm_انتہا) واپس m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ڈالو(v, c_m3Type_f64); اگر (r) واپس r;
            توڑو;
        }


        // ---- i32 comparison ----
        حالت 0x45: r = v_testop(v, c_m3Type_i32); توڑو; // i32.eqz
        حالت 0x46: حالت 0x47: حالت 0x48: حالت 0x49: حالت 0x4a:
        حالت 0x4b: حالت 0x4c: حالت 0x4d: حالت 0x4e: حالت 0x4f:
            r = v_relop(v, c_m3Type_i32); توڑو;

        // ---- i64 comparison ----
        حالت 0x50: r = v_testop(v, c_m3Type_i64); توڑو; // i64.eqz
        حالت 0x51: حالت 0x52: حالت 0x53: حالت 0x54: حالت 0x55:
        حالت 0x56: حالت 0x57: حالت 0x58: حالت 0x59: حالت 0x5a:
            r = v_relop(v, c_m3Type_i64); توڑو;

        // ---- f32 comparison ----
        حالت 0x5b: حالت 0x5c: حالت 0x5d: حالت 0x5e: حالت 0x5f: حالت 0x60:
            r = v_relop(v, c_m3Type_f32); توڑو;

        // ---- f64 comparison ----
        حالت 0x61: حالت 0x62: حالت 0x63: حالت 0x64: حالت 0x65: حالت 0x66:
            r = v_relop(v, c_m3Type_f64); توڑو;

        // ---- i32 unary ----
        حالت 0x67: حالت 0x68: حالت 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); توڑو;

        // ---- i32 binary ----
        حالت 0x6a: حالت 0x6b: حالت 0x6c: حالت 0x6d: حالت 0x6e: حالت 0x6f:
        حالت 0x70: حالت 0x71: حالت 0x72: حالت 0x73: حالت 0x74: حالت 0x75:
        حالت 0x76: حالت 0x77: حالت 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); توڑو;

        // ---- i64 unary ----
        حالت 0x79: حالت 0x7a: حالت 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); توڑو;

        // ---- i64 binary ----
        حالت 0x7c: حالت 0x7d: حالت 0x7e: حالت 0x7f: حالت 0x80: حالت 0x81:
        حالت 0x82: حالت 0x83: حالت 0x84: حالت 0x85: حالت 0x86: حالت 0x87:
        حالت 0x88: حالت 0x89: حالت 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); توڑو;

        // ---- f32 unary ----
        حالت 0x8b: حالت 0x8c: حالت 0x8d: حالت 0x8e: حالت 0x8f:
        حالت 0x90: حالت 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); توڑو;

        // ---- f32 binary ----
        حالت 0x92: حالت 0x93: حالت 0x94: حالت 0x95: حالت 0x96:
        حالت 0x97: حالت 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); توڑو;

        // ---- f64 unary ----
        حالت 0x99: حالت 0x9a: حالت 0x9b: حالت 0x9c: حالت 0x9d:
        حالت 0x9e: حالت 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); توڑو;

        // ---- f64 binary ----
        حالت 0xa0: حالت 0xa1: حالت 0xa2: حالت 0xa3: حالت 0xa4:
        حالت 0xa5: حالت 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); توڑو;

        // ---- Conversions ----
        حالت 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); توڑو; // i32.wrap/i64
        حالت 0xa8: حالت 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); توڑو;
        حالت 0xaa: حالت 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); توڑو;
        حالت 0xac: حالت 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); توڑو;
        حالت 0xae: حالت 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); توڑو;
        حالت 0xb0: حالت 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); توڑو;
        حالت 0xb2: حالت 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); توڑو;
        حالت 0xb4: حالت 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); توڑو;
        حالت 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); توڑو;
        حالت 0xb7: حالت 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); توڑو;
        حالت 0xb9: حالت 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); توڑو;
        حالت 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); توڑو;
        حالت 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); توڑو;
        حالت 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); توڑو;
        حالت 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); توڑو;
        حالت 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); توڑو;

        // ---- Sign-extension (MVP post) ----
        حالت 0xc0: حالت 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); توڑو;
        حالت 0xc2: حالت 0xc3: حالت 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); توڑو;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        حالت 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;
            انتخاب (sub) {
            حالت 0x00: حالت 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); توڑو;
            حالت 0x02: حالت 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); توڑو;
            حالت 0x04: حالت 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); توڑو;
            حالت 0x06: حالت 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); توڑو;
            حالت 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                اگر (not v_has_حافظہ_idx(v, memidx)) واپس m3_Err_unknown_حافظہ;
                // the segments must have been declared up front by a data count section
                اگر (not v->module->has_ڈیٹا_گنتی) واپس m3_Err_ڈیٹا_گنتی_Required;
                اگر (dataidx >= v->module->عدد_ڈیٹا_Segments) واپس m3_Err_unknown_ڈیٹا_Segment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_نکالو_expect(v, c_m3Type_i32, &a); اگر (r) واپس r; // n
                r = v_نکالو_expect(v, c_m3Type_i32, &a); اگر (r) واپس r; // src
                r = v_نکالو_expect(v, v_حافظہ_addrtype(v, memidx), &a); اگر (r) واپس r; // dst
                توڑو;
            }
            حالت 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                // needs the segment, but not a memory
                اگر (not v->module->has_ڈیٹا_گنتی) واپس m3_Err_ڈیٹا_گنتی_Required;
                اگر (dataidx >= v->module->عدد_ڈیٹا_Segments) واپس m3_Err_unknown_ڈیٹا_Segment;
                توڑو;
            }
#اگر d_m3HasRefTypes
            حالت 0x0c: // table.init
            حالت 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;

                اگر (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    اگر (a2 >= v->module->عدد_Tables) واپس m3_Err_unknown_جدول;
                    اگر (a1 >= v->module->عدد_Element_Segments) واپس m3Err_unknownElemSegment;
                    اگر (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        واپس m3Err_typeMismatch;
                } ورنہ {                                                    // dst, src
                    اگر (a1 >= v->module->عدد_Tables) واپس m3_Err_unknown_جدول;
                    اگر (a2 >= v->module->عدد_Tables) واپس m3_Err_unknown_جدول;
                    اگر (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        واپس m3Err_typeMismatch;
                }

                اگر (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_نکالو_expect(v, c_m3Type_i32, &a); اگر (r) واپس r;             // n
                    r = v_نکالو_expect(v, c_m3Type_i32, &a); اگر (r) واپس r;             // s
                    r = v_نکالو_expect(v, v_جدول_addrtype(v, a2), &a); اگر (r) واپس r;  // d
                } ورنہ {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_جدول_addrtype(v, a1);
                    m3type_t sType = v_جدول_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_نکالو_expect(v, nType, &a); اگر (r) واپس r;  // n
                    r = v_نکالو_expect(v, sType, &a); اگر (r) واپس r;  // s
                    r = v_نکالو_expect(v, dType, &a); اگر (r) واپس r;  // d
                }
                توڑو;
            }
            حالت 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                اگر (elemIdx >= v->module->عدد_Element_Segments) واپس m3Err_unknownElemSegment;
                توڑو;
            }
            حالت 0x0f: // table.grow
            حالت 0x10: // table.size
            حالت 0x11: // table.fill
            {
                u32 جدول_Idx;
                r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                اگر (جدول_Idx >= v->module->عدد_Tables) واپس m3_Err_unknown_جدول;
                u8 t = بنیاد_Type_کا(v->module->tables[جدول_Idx]->type);
                m3type_t at = v_جدول_addrtype(v, جدول_Idx);

                اگر (sub == 0x10) {                                          // table.size
                    r = v_ڈالو(v, at); اگر (r) واپس r;
                } ورنہ اگر (sub == 0x0f) {                                   // table.grow
                    r = v_نکالو_expect(v, at, &a); اگر (r) واپس r;  // n
                    r = v_نکالو_expect(v, t, &a);  اگر (r) واپس r;  // init
                    r = v_ڈالو(v, at);           اگر (r) واپس r;
                } ورنہ {                                                    // table.fill
                    r = v_نکالو_expect(v, at, &a); اگر (r) واپس r;  // n
                    r = v_نکالو_expect(v, t, &a);  اگر (r) واپس r;  // val
                    r = v_نکالو_expect(v, at, &a); اگر (r) واپس r;  // i
                }
                توڑو;
            }
#اگر_ختم
            حالت 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                اگر (not v_has_حافظہ_idx(v, dst) or not v_has_حافظہ_idx(v, src)) واپس m3_Err_unknown_حافظہ;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_حافظہ_addrtype(v, dst);
                m3type_t srcType = v_حافظہ_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_نکالو_expect(v, lenType, &a); اگر (r) واپس r; // n
                r = v_نکالو_expect(v, srcType, &a); اگر (r) واپس r; // src
                r = v_نکالو_expect(v, dstType, &a); اگر (r) واپس r; // dst
                توڑو;
            }
            حالت 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                اگر (not v_has_حافظہ_idx(v, memidx)) واپس m3_Err_unknown_حافظہ;
                m3type_t addrType = v_حافظہ_addrtype(v, memidx);
                r = v_نکالو_expect(v, addrType, &a); اگر (r) واپس r;      // n
                r = v_نکالو_expect(v, c_m3Type_i32, &a); اگر (r) واپس r;  // val
                r = v_نکالو_expect(v, addrType, &a); اگر (r) واپس r;      // dst
                توڑو;
            }
#اگر d_m3HasWideArithmetic
            حالت 0x13: حالت 0x14: // i64.add128, i64.sub128
            حالت 0x15: حالت 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 عدد_Operands = (sub <= 0x14) ? 4 : 2;
                برائے (u32 i = 0; i < عدد_Operands; ++i) {
                    r = v_نکالو_expect(v, c_m3Type_i64, &a); اگر (r) واپس r;
                }
                r = v_ڈالو(v, c_m3Type_i64); اگر (r) واپس r;
                r = v_ڈالو(v, c_m3Type_i64); اگر (r) واپس r;
                توڑو;
            }
#اگر_ختم
            طےشدہ:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                توڑو;
            }
            توڑو;
        }

#اگر d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        حالت 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_انتہا);
            اگر (r) واپس r;

            اگر (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                اگر (reserved != 0) واپس m3Err_wasmMalformed;
                توڑو;
            }

            u32 چوڑائی_2, عدد_Operands; m3type_t قدر_Type, نتیجہ_Type;
            اگر (not v_atomic_shape(sub, &چوڑائی_2, &عدد_Operands, &قدر_Type, &نتیجہ_Type)) واپس m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = Read_حافظہ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            اگر ((1u << align) != چوڑائی_2) واپس m3Err_invalidAtomicAlignment;
            اگر (not v_has_حافظہ_idx(v, memidx)) واپس m3_Err_unknown_حافظہ;
            اگر (not v_انحراف_اندر_range(v, memidx, offset)) واپس m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            اگر (sub == 0x01 or sub == 0x02) {
                r = v_نکالو_expect(v, c_m3Type_i64, &a); اگر (r) واپس r;
                عدد_Operands--;
            }
            برائے (u32 i = 1; i < عدد_Operands; ++i) {
                r = v_نکالو_expect(v, قدر_Type, &a); اگر (r) واپس r;
            }
            r = v_نکالو_expect(v, v_حافظہ_addrtype(v, memidx), &a); اگر (r) واپس r;
            اگر (نتیجہ_Type != c_m3Type_none) {
                r = v_ڈالو(v, نتیجہ_Type); اگر (r) واپس r;
            }
            توڑو;
        }
#اگر_ختم

#اگر d_m3_Has_ڈھیر_Switching
        // ---- Stack Switching ----
        حالت 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            اگر (!ft || !ft->isContinuation) واپس m3Err_typeMismatch;
            r = v_نکالو_expect(v, c_m3Type_funcref, &a); اگر (r) واپس r;
            r = v_ڈالو(v, c_m3Type_contref); اگر (r) واپس r;
            توڑو;
        }

        حالت 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (typeIdx1 >= v->module->عدد_Func_Types || typeIdx2 >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            اگر (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) واپس m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 عدد_Params1 = ft1->عدد_دلائل;
            u16 عدد_Params2 = ft2->عدد_دلائل;
            اگر (عدد_Params1 < عدد_Params2) واپس m3Err_typeMismatch;
            u32 عدد_Bound = عدد_Params1 - عدد_Params2;

            اگر (ft1->عدد_Rets != ft2->عدد_Rets) واپس m3Err_typeMismatch;
            برائے (u16 i = 0; i < ft1->عدد_Rets; ++i) {
                اگر (ft1->types[i] != ft2->types[i]) واپس m3Err_typeMismatch;
            }
            برائے (u16 i = 0; i < عدد_Params2; ++i) {
                اگر (ft1->types[ft1->عدد_Rets + عدد_Bound + i] != ft2->types[ft2->عدد_Rets + i]) واپس m3Err_typeMismatch;
            }

            r = v_نکالو_expect(v, c_m3Type_contref, &a); اگر (r) واپس r;

            برائے (u32 i = عدد_Bound; i > 0; i--) {
                r = v_نکالو_expect(v, بنیاد_Type_کا(ft1->types[ft1->عدد_Rets + i - 1]), &a);
                اگر (r) واپس r;
            }

            r = v_ڈالو(v, c_m3Type_contref); اگر (r) واپس r;
            توڑو;
        }

        حالت 0xe2: // suspend
        {
            u32 لیبل_Idx;
            r = ReadLEB_u32(&لیبل_Idx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (!v->module || لیبل_Idx >= v->module->عدد_لیبل_فہرست) واپس m3_Err_unknown_لیبل;
            IM3FuncType tt = v->module->لیبل_فہرست[لیبل_Idx].type;
            اگر (tt) {
                برائے (u16 i = tt->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(tt->types[tt->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
                برائے (u16 i = 0; i < tt->عدد_Rets; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(tt->types[i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) واپس m3Err_typeMismatch;

            u32 عدد_Clauses;
            r = ReadLEB_u32(&عدد_Clauses, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;

            برائے (u32 i = 0; i < عدد_Clauses; i++) {
                u8 قسم;
                r = Read_u8(&قسم, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                u32 لیبل_Idx;
                r = ReadLEB_u32(&لیبل_Idx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                r = v_resume_clause(v, قسم, لیبل_Idx, ct); اگر (r) واپس r;
            }

            r = v_نکالو_expect(v, c_m3Type_contref, &a); اگر (r) واپس r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            اگر (ft) {
                برائے (u16 i = ft->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[ft->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
                برائے (u16 i = 0; i < ft->عدد_Rets; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(ft->types[i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0xe4: // resume_throw
        {
            u32 typeIdx, لیبل_Idx, عدد_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            r = ReadLEB_u32(&لیبل_Idx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            r = ReadLEB_u32(&عدد_Clauses, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;

            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            اگر (!v->module || لیبل_Idx >= v->module->عدد_لیبل_فہرست) واپس m3_Err_unknown_لیبل;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) واپس m3Err_typeMismatch;
            IM3FuncType لیبل_Type = v->module->لیبل_فہرست[لیبل_Idx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            اگر (لیبل_Type && لیبل_Type->عدد_Rets != 0) واپس m3Err_typeMismatch;

            برائے (u32 i = 0; i < عدد_Clauses; i++) {
                u8 قسم;
                r = Read_u8(&قسم, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                u32 c_لیبل_Idx;
                r = ReadLEB_u32(&c_لیبل_Idx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                r = v_resume_clause(v, قسم, c_لیبل_Idx, ct); اگر (r) واپس r;
            }

            r = v_نکالو_expect(v, c_m3Type_contref, &a); اگر (r) واپس r;

            اگر (لیبل_Type) {
                برائے (u16 i = لیبل_Type->عدد_دلائل; i > 0; i--) {
                    r = v_نکالو_expect(v, بنیاد_Type_کا(لیبل_Type->types[لیبل_Type->عدد_Rets + i - 1]), &a);
                    اگر (r) واپس r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            اگر (ft) {
                برائے (u16 i = 0; i < ft->عدد_Rets; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(ft->types[i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0xe5: // resume_throw_ref
        {
            u32 typeIdx, عدد_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            r = ReadLEB_u32(&عدد_Clauses, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;

            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) واپس m3Err_typeMismatch;

            برائے (u32 i = 0; i < عدد_Clauses; i++) {
                u8 قسم;
                r = Read_u8(&قسم, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                u32 c_لیبل_Idx;
                r = ReadLEB_u32(&c_لیبل_Idx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
                r = v_resume_clause(v, قسم, c_لیبل_Idx, ct); اگر (r) واپس r;
            }

            r = v_نکالو_expect(v, c_m3Type_contref, &a); اگر (r) واپس r;
            r = v_نکالو_expect(v, c_m3Type_exnref, &a); اگر (r) واپس r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            اگر (ft) {
                برائے (u16 i = 0; i < ft->عدد_Rets; i++) {
                    r = v_ڈالو(v, بنیاد_Type_کا(ft->types[i]));
                    اگر (r) واپس r;
                }
            }
            توڑو;
        }

        حالت 0xe6: // switch
        {
            u32 typeIdx, لیبل_Idx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;
            r = ReadLEB_u32(&لیبل_Idx, &v->wasm, v->wasm_انتہا); اگر (r) واپس r;

            اگر (typeIdx >= v->module->عدد_Func_Types) واپس m3Err_unknownType;
            اگر (!v->module || لیبل_Idx >= v->module->عدد_لیبل_فہرست) واپس m3_Err_unknown_لیبل;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            اگر (!ct || !ct->isContinuation) واپس m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType لیبل_Type = v->module->لیبل_فہرست[لیبل_Idx].type;
            اگر (لیبل_Type && لیبل_Type->عدد_دلائل != 0) واپس m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            اگر (!ft || ft->عدد_دلائل == 0) واپس m3Err_typeMismatch;

            IM3FuncType peer = Module_Cont_Type_کا_Ref(v->module, ft->types[ft->عدد_Rets + ft->عدد_دلائل - 1]);
            اگر (!peer || !peer->contFuncType) واپس m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_نکالو_expect(v, c_m3Type_contref, &a); اگر (r) واپس r;

            برائے (u16 i = (u16)(ft->عدد_دلائل - 1); i > 0; i--) {
                r = v_نکالو_expect(v, بنیاد_Type_کا(ft->types[ft->عدد_Rets + i - 1]), &a);
                اگر (r) واپس r;
            }

            برائے (u16 i = 0; i < self->عدد_دلائل; i++) {
                r = v_ڈالو(v, بنیاد_Type_کا(self->types[self->عدد_Rets + i]));
                اگر (r) واپس r;
            }
            توڑو;
        }

#اگر_ختم // d_m3HasStackSwitching

        طےشدہ:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            توڑو;

        } // switch

        اگر (r) واپس r;

    } // while

    // If we ran out of bytes without hitting the final end
    واپس m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3_نتیجہ  ValidateFunction  (IM3Function i_function)
{
    اگر (!i_function->wasm) واپس m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    اگر (!v) {
        v = m3_AllocStruct (ValCtx);
        اگر (!v) واپس m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, حجم(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasm_انتہا  = i_function->wasm_انتہا;

    // Skip code size LEB
    u32 سائز;
    M3_نتیجہ r = ReadLEB_u32(&سائز, &v->wasm, v->wasm_انتہا);
    اگر (r) واپس r;

    // Parse locals
    u32 عدد_Local_بلاکیں;
    r = ReadLEB_u32(&عدد_Local_بلاکیں, &v->wasm, v->wasm_انتہا);
    اگر (r) واپس r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 عدد_Params = funcType ? funcType->عدد_دلائل : 0;
    اگر (عدد_Params > d_m3_Val_ڈھیر) واپس m3_Err_function_ڈھیر_حد_سے_تجاوز;
    برائے (u16 i = 0; i < عدد_Params; i++) {
        v->localTypes[v->عدد_Locals++] = بنیاد_Type_کا(funcType->types[funcType->عدد_Rets + i]);
    }

    // Then: declared locals
    برائے (u32 b = 0; b < عدد_Local_بلاکیں; b++) {
        u32 گنتی;
        r = ReadLEB_u32(&گنتی, &v->wasm, v->wasm_انتہا);
        اگر (r) واپس r;
        m3type_t localType;
        r = Parse_قدر_Type(v->module, &localType, &v->wasm, v->wasm_انتہا);
        اگر (r) واپس r;
        u8 normalized = بنیاد_Type_کا(localType);
        اگر (گنتی > (u32) (d_m3_Val_ڈھیر - v->عدد_Locals)) واپس m3_Err_function_ڈھیر_حد_سے_تجاوز;
        برائے (u32 c = 0; c < گنتی; c++) {
            v->localTypes[v->عدد_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ڈالو_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    اگر (r) واپس r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    اگر (r) واپس r;

    // After validation, control stack should be empty and all declared bytes consumed
    اگر (v->ctrl_چوٹی != 0)
        واپس m3Err_wasmMalformed;

    اگر (v->wasm != v->wasm_انتہا)
        واپس m3Err_wasmSectionUnderrun;

    واپس m3Err_none;
}

#ورنہ // !d_m3EnableValidation

M3_نتیجہ  ValidateFunction  (IM3Function i_function)
{
    (خالی)i_function;
    واپس m3Err_none;
}

// clang-format on

#اگر_ختم // d_m3EnableValidation
