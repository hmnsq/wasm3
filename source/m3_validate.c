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
#وصف_ڪريو c_valBottom         0xFF

// ---------- Control frame ----------

تعريفنوع بنية {
    m3opcode_t  opcode;
    u16         اوچائي;         // operand stack height at block entry
    u16         param_ڳڻپ;
    u16         نتيجو_ڳڻپ;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} Val_Ctrl_چوکٺو;

// ---------- Validator context ----------

بنية ValCtx {
    بائيٽون_t     wasm;
    بائيٽون_t     wasm_پڇاڙي;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ڍير];
    u16         opd_چوٽي;

    Val_Ctrl_چوکٺو ctrl [d_m3_Val_Ctrl_اونهائي];
    u16          ctrl_چوٽي;

    m3type_t    localTypes [d_m3_Val_ڍير];
    u16         انگ_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ساكن bool v_has_يادگيري_idx (ValCtx * v, u32 i_يادگيري_Idx)
{
    أرجع v->module and i_يادگيري_Idx < v->module->انگ_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ساكن m3type_t v_يادگيري_addrtype (ValCtx * v, u32 i_يادگيري_Idx)
{
    أرجع يادگيري_Addr_Type (v->module->memories [i_يادگيري_Idx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ساكن bool v_وٿي_اندريون_range (ValCtx * v, u32 i_يادگيري_Idx, u64 i_وٿي)
{
    أرجع v_يادگيري_addrtype (v, i_يادگيري_Idx) == c_m3Type_i64 or i_وٿي <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ساكن m3type_t v_جدول_addrtype (ValCtx * v, u32 i_جدول_Idx)
{
    أرجع جدول_Addr_Type (v->module->tables [i_جدول_Idx]);
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
        أرجع 0;
    حالة 0x2e: حالة 0x2f:   // i32.load16_s, i32.load16_u
    حالة 0x32: حالة 0x33:   // i64.load16_s, i64.load16_u
    حالة 0x3b:              // i32.store16
    حالة 0x3d:              // i64.store16
        أرجع 1;
    حالة 0x29:              // i64.load
    حالة 0x2b:              // f64.load
    حالة 0x37:              // i64.store
    حالة 0x39:              // f64.store
        أرجع 3;
    افتراضي:                // 32-bit accesses, and a safe fallback
        أرجع 2;
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
ساكن bool v_atomic_shape (u32 sub, u32 * o_ويڪر, u32 * o_انگ_Operands, m3type_t * o_قدر_Type, m3type_t * o_نتيجو_Type)
{
    ساكن ثابت u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ساكن ثابت m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    إذا (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_ويڪر       = (sub == 0x02) ? 8 : 4;
        *o_انگ_Operands = (sub == 0x00) ? 2 : 3;
        *o_قدر_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_نتيجو_Type  = c_m3Type_i32;
        أرجع true;
    }

    إذا (sub < 0x10 or sub > c_wa_Op_آخري_Atomic) أرجع false;

    إذا (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_انگ_Operands = 1;
    } وإلا إذا (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_انگ_Operands = 2;
    } وإلا إذا (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_انگ_Operands = 2;
    } وإلا {                        // cmpxchg
        variant = sub - 0x48;       *o_انگ_Operands = 3;
    }

    *o_ويڪر      = c_widths [variant];
    *o_قدر_Type  = c_types [variant];
    *o_نتيجو_Type = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    أرجع true;
}

#جيڪڏهن_پورو

// ---------- Operand stack ----------

ساكن M3_نتيجو v_ڌڪو (ValCtx * v, m3type_t type)
{
    إذا (M3_UNLIKELY(v->opd_چوٽي >= d_m3_Val_ڍير))
        أرجع m3_Err_function_ڍير_حد_کان_مٿي;
    v->opd[v->opd_چوٽي++] = type;
    أرجع m3Err_none;
}

ساكن M3_نتيجو v_ڪڍو (ValCtx * v, m3type_t * o_type)
{
    Val_Ctrl_چوکٺو * f = &v->ctrl[v->ctrl_چوٽي - 1];
    إذا (M3_UNLIKELY(v->opd_چوٽي == f->اوچائي)) {
        إذا (f->is_unreachable) { *o_type = c_valBottom; أرجع m3Err_none; }
        أرجع m3_Err_function_ڍير_Underrun;
    }
    *o_type = v->opd[--v->opd_چوٽي];
    أرجع m3Err_none;
}

ساكن M3_نتيجو v_ڪڍو_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3_نتيجو r = v_ڪڍو(v, &actual);
    إذا (M3_UNLIKELY(r)) أرجع r;
    إذا (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        أرجع m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    أرجع m3Err_none;
}

// ---------- Control stack ----------

ساكن M3_نتيجو v_ڌڪو_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    إذا (M3_UNLIKELY(v->ctrl_چوٽي >= d_m3_Val_Ctrl_اونهائي))
        أرجع m3_Err_function_ڍير_حد_کان_مٿي;
    Val_Ctrl_چوکٺو * f = &v->ctrl[v->ctrl_چوٽي++];
    f->opcode       = op;
    f->type         = type;
    f->param_ڳڻپ  = type ? type->انگ_دليلون : 0;
    f->نتيجو_ڳڻپ = type ? type->انگ_Rets : 0;
    f->اوچائي       = v->opd_چوٽي;
    f->is_unreachable = false;
    أرجع m3Err_none;
}

ساكن M3_نتيجو v_ڪڍو_ctrl (ValCtx * v, Val_Ctrl_چوکٺو * o_چوکٺو)
{
    إذا (M3_UNLIKELY(v->ctrl_چوٽي == 0))
        أرجع m3Err_wasmMalformed;
    Val_Ctrl_چوکٺو * f = &v->ctrl[v->ctrl_چوٽي - 1];
    // pop result types
    إذا (f->type) {
        لكل (u16 i = f->نتيجو_ڳڻپ; i > 0; i--) {
            m3type_t a;
            M3_نتيجو r = v_ڪڍو_expect(v, بنياد_Type_جو(f->type->types[i - 1]), &a);
            إذا (M3_UNLIKELY(r)) أرجع r;
        }
    }
    إذا (M3_UNLIKELY(v->opd_چوٽي != f->اوچائي))
        أرجع m3_Err_type_ڳڻپ_Mismatch;
    إذا (o_چوکٺو) *o_چوکٺو = *f;
    v->ctrl_چوٽي--;
    أرجع m3Err_none;
}

ساكن فراغ v_unreachable (ValCtx * v)
{
    Val_Ctrl_چوکٺو * f = &v->ctrl[v->ctrl_چوٽي - 1];
    v->opd_چوٽي = f->اوچائي;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ساكن u16 v_label_n (Val_Ctrl_چوکٺو * f)
{
    أرجع (f->opcode == 0x03) ? f->param_ڳڻپ : f->نتيجو_ڳڻپ;
}

ساكن m3type_t v_label_t (Val_Ctrl_چوکٺو * f, u16 i)
{
    إذا (!f->type) أرجع c_m3Type_none;
    إذا (f->opcode == 0x03)
        أرجع بنياد_Type_جو(f->type->types[f->type->انگ_Rets + i]); // params
    أرجع بنياد_Type_جو(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ساكن m3type_t v_peek (ValCtx * v, u16 i_اونهائي)
{
    Val_Ctrl_چوکٺو * f = &v->ctrl[v->ctrl_چوٽي - 1];

    إذا (v->opd_چوٽي <= i_اونهائي || (u16)(v->opd_چوٽي - i_اونهائي - 1) < f->اوچائي)
        أرجع c_valBottom;

    أرجع v->opd[v->opd_چوٽي - i_اونهائي - 1];
}

// Pop label types for branch target
ساكن M3_نتيجو v_ڪڍو_labels (ValCtx * v, Val_Ctrl_چوکٺو * tgt)
{
    u16 n = v_label_n(tgt);
    لكل (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3_نتيجو r = v_ڪڍو_expect(v, v_label_t(tgt, i - 1), &a);
        إذا (M3_UNLIKELY(r)) أرجع r;
    }
    أرجع m3Err_none;
}

// Push label types back
ساكن M3_نتيجو v_ڌڪو_labels (ValCtx * v, Val_Ctrl_چوکٺو * tgt)
{
    u16 n = v_label_n(tgt);
    لكل (u16 i = 0; i < n; i++) {
        M3_نتيجو r = v_ڌڪو(v, v_label_t(tgt, i));
        إذا (M3_UNLIKELY(r)) أرجع r;
    }
    أرجع m3Err_none;
}

// ---------- Block type resolution ----------

ساكن M3_نتيجو v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    إذا (v->wasm >= v->wasm_پڇاڙي)
        أرجع m3Err_wasmUnderrun;

#إذا d_m3HasTypedRefs
    إذا (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3_نتيجو rr = Parse_قدر_Type(v->module, &refType, &v->wasm, v->wasm_پڇاڙي);
        إذا (rr) أرجع rr;
        IM3FuncType ftype = عدم;
        rr = AllocFuncType(&ftype, 1);
        إذا (rr) أرجع rr;
        ftype->انگ_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_جوڙيو_Func_Type(v->module->environment, &ftype);
        إذا (rr) أرجع rr;
        *o_type = ftype;
        أرجع m3Err_none;
    }
#جيڪڏهن_پورو

    i64 type;
    M3_نتيجو r = ReadLebSigned(&type, 33, &v->wasm, v->wasm_پڇاڙي);
    إذا (r) أرجع r;

    إذا (type < 0) {
        إذا (type < -64) أرجع m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        إذا (r) أرجع r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } وإلا {
        إذا ((u32)type >= v->module->انگ_Func_Types)
            أرجع m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    أرجع m3Err_none;
}

// ---------- Convenience ----------

ساكن M3_نتيجو v_unop (ValCtx * v, m3type_t اندريون, m3type_t ٻاهريون)
{
    m3type_t a; M3_نتيجو r = v_ڪڍو_expect(v, اندريون, &a);
    إذا (M3_UNLIKELY(r)) أرجع r;
    أرجع v_ڌڪو(v, ٻاهريون);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ساكن M3_نتيجو v_check_پڇ_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : عدم;

    u16 انگ_Callee_Rets = i_calleeType ? i_calleeType->انگ_Rets : 0;
    u16 انگ_Func_Rets   = ft ? ft->انگ_Rets : 0;

    إذا (انگ_Callee_Rets != انگ_Func_Rets)
        أرجع m3Err_typeMismatch;

    لكل (u16 i = 0; i < انگ_Callee_Rets; i++) {
        إذا (i_calleeType->types[i] != ft->types[i])
            أرجع m3Err_typeMismatch;
    }

    أرجع m3Err_none;
}

ساكن M3_نتيجو v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_نتيجو r;
    r = v_ڪڍو_expect(v, t, &a); إذا (M3_UNLIKELY(r)) أرجع r;
    r = v_ڪڍو_expect(v, t, &a); إذا (M3_UNLIKELY(r)) أرجع r;
    أرجع v_ڌڪو(v, t);
}

ساكن M3_نتيجو v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_نتيجو r;
    r = v_ڪڍو_expect(v, t, &a); إذا (M3_UNLIKELY(r)) أرجع r;
    r = v_ڪڍو_expect(v, t, &a); إذا (M3_UNLIKELY(r)) أرجع r;
    أرجع v_ڌڪو(v, c_m3Type_i32);
}

ساكن M3_نتيجو v_testop (ValCtx * v, m3type_t t)
{
    أرجع v_unop(v, t, c_m3Type_i32);
}

ساكن M3_نتيجو v_cvtop (ValCtx * v, m3type_t اندريون, m3type_t ٻاهريون)
{
    أرجع v_unop(v, اندريون, ٻاهريون);
}


// ---------- Main validation loop ----------

#إذا d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ساكن M3_نتيجو v_catch_clause (ValCtx * v)
{
    u8 قسم;
    M3_نتيجو r = Read_u8(&قسم, &v->wasm, v->wasm_پڇاڙي);
    إذا (r) أرجع r;
    إذا (قسم > 0x03) أرجع m3Err_wasmMalformed;

    bool has_ٽيگ = (قسم == 0x00 || قسم == 0x01);
    bool hasRef = (قسم == 0x01 || قسم == 0x03);

    IM3FuncType ٽيگ_Type = عدم;
    إذا (has_ٽيگ) {
        u32 ٽيگ_اشاريو;
        r = ReadLEB_u32(&ٽيگ_اشاريو, &v->wasm, v->wasm_پڇاڙي);
        إذا (r) أرجع r;
        إذا (!v->module || ٽيگ_اشاريو >= v->module->انگ_ٽيگ_فهرست) أرجع m3_Err_unknown_ٽيگ;
        ٽيگ_Type = v->module->ٽيگ_فهرست[ٽيگ_اشاريو].type;
    }

    u32 اونهائي;
    r = ReadLEB_u32(&اونهائي, &v->wasm, v->wasm_پڇاڙي);
    إذا (r) أرجع r;
    إذا (اونهائي >= v->ctrl_چوٽي) أرجع m3Err_unknownLabel;

    Val_Ctrl_چوکٺو * tgt = &v->ctrl[v->ctrl_چوٽي - 1 - اونهائي];

    u16 انگ_Payload = ٽيگ_Type ? ٽيگ_Type->انگ_دليلون : 0;
    u16 انگ_Label   = v_label_n(tgt);

    إذا (انگ_Label != انگ_Payload + (hasRef ? 1u : 0u))
        أرجع m3_Err_type_ڳڻپ_Mismatch;

    لكل (u16 i = 0; i < انگ_Payload; i++) {
        إذا (v_label_t(tgt, i) != بنياد_Type_جو(ٽيگ_Type->types[ٽيگ_Type->انگ_Rets + i]))
            أرجع m3Err_typeMismatch;
    }

    إذا (hasRef and v_label_t(tgt, انگ_Payload) != c_m3Type_exnref)
        أرجع m3Err_typeMismatch;

    أرجع m3Err_none;
}

#جيڪڏهن_پورو // d_m3HasExceptionHandling

#إذا d_m3_Has_ڍير_Switching

ساكن M3_نتيجو v_resume_clause (ValCtx * v, u8 قسم, u32 ٽيگ_Idx, IM3FuncType ct)
{
    إذا (قسم > 0x01) أرجع m3Err_wasmMalformed;
    إذا (!v->module || ٽيگ_Idx >= v->module->انگ_ٽيگ_فهرست) أرجع m3_Err_unknown_ٽيگ;
    IM3FuncType ٽيگ_Type = v->module->ٽيگ_فهرست[ٽيگ_Idx].type;

    إذا (قسم == 0x00) {
        u32 اونهائي;
        M3_نتيجو r = ReadLEB_u32(&اونهائي, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
        إذا (اونهائي >= v->ctrl_چوٽي) أرجع m3Err_unknownLabel;

        Val_Ctrl_چوکٺو * tgt = &v->ctrl[v->ctrl_چوٽي - 1 - اونهائي];
        u16 انگ_Payload = ٽيگ_Type ? ٽيگ_Type->انگ_دليلون : 0;
        u16 انگ_Label   = v_label_n(tgt);

        إذا (انگ_Label != انگ_Payload + 1u) أرجع m3_Err_type_ڳڻپ_Mismatch;

        لكل (u16 j = 0; j < انگ_Payload; j++) {
            إذا (v_label_t(tgt, j) != بنياد_Type_جو(ٽيگ_Type->types[ٽيگ_Type->انگ_Rets + j]))
                أرجع m3Err_typeMismatch;
        }
        إذا (v_label_t(tgt, انگ_Payload) != c_m3Type_contref)
            أرجع m3Err_typeMismatch;

#إذا d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->انگ_Rets + انگ_Payload]
            : tgt->type->types[انگ_Payload];

        إذا (ڍير_Type_جو(labelContType) == d_m3_Type_ڍير_Abstract ||
            ڍير_Type_جو(labelContType) == d_m3_Type_ڍير_None) {
            أرجع m3Err_typeMismatch;
        }

        u32 ڍير_Type_2 = ڍير_Type_جو(labelContType);
        IM3FuncType labelCt = عدم;
        لكل (u32 idx = 0; idx < v->module->انگ_Func_Types; ++idx) {
            إذا (v->module->funcTypes[idx]->canonical_اشاريو == ڍير_Type_2 &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                اكسر;
            }
        }
        إذا (!labelCt) أرجع m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_انگ_Params = ٽيگ_Type ? ٽيگ_Type->انگ_Rets : 0;
        إذا (labelFt->انگ_دليلون != expected_انگ_Params) أرجع m3Err_typeMismatch;
        لكل (u16 p = 0; p < expected_انگ_Params; ++p) {
            إذا (labelFt->types[labelFt->انگ_Rets + p] != ٽيگ_Type->types[p])
                أرجع m3Err_typeMismatch;
        }

        u16 expected_انگ_Rets = ft ? ft->انگ_Rets : 0;
        إذا (labelFt->انگ_Rets != expected_انگ_Rets) أرجع m3Err_typeMismatch;
        لكل (u16 r_idx = 0; r_idx < expected_انگ_Rets; ++r_idx) {
            إذا (labelFt->types[r_idx] != ft->types[r_idx])
                أرجع m3Err_typeMismatch;
        }
#جيڪڏهن_پورو
    } وإلا {
        إذا (ٽيگ_Type && ٽيگ_Type->انگ_دليلون != 0) أرجع m3Err_typeMismatch;
    }
    أرجع m3Err_none;
}

#جيڪڏهن_پورو // d_m3HasStackSwitching

ساكن M3_نتيجو v_validate_body (ValCtx * v)
{
    M3_نتيجو r = m3Err_none;
    m3type_t a = c_valBottom;

    طالما (v->wasm < v->wasm_پڇاڙي)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasm_پڇاڙي);
        إذا (M3_UNLIKELY(r)) أرجع r;

        بدل (opcode)
        {
        // ---- Control ----
        حالة 0x00: // unreachable
            v_unreachable(v);
            اكسر;

        حالة 0x01: // nop
            اكسر;

        حالة 0x02: //بلاڪ
        حالة 0x03: // loop
        حالة 0x04: // if
        {
            IM3FuncType bt = عدم;
            r = v_read_blocktype(v, &bt);
            إذا (r) أرجع r;
            إذا (opcode == 0x04) {
                r = v_ڪڍو_expect(v, c_m3Type_i32, &a);
                إذا (r) أرجع r;
            }
            // Pop block params from caller stack
            إذا (bt) {
                لكل (u16 i = bt->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(bt->types[bt->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
            }
            r = v_ڌڪو_ctrl(v, opcode, bt);
            إذا (r) أرجع r;
            // Push params inside block
            إذا (bt) {
                لكل (u16 i = 0; i < bt->انگ_دليلون; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(bt->types[bt->انگ_Rets + i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

#إذا d_m3HasExceptionHandling
        حالة 0x1f: // try_table
        {
            IM3FuncType bt = عدم;
            r = v_read_blocktype(v, &bt);
            إذا (r) أرجع r;
            إذا (bt) {
                لكل (u16 i = bt->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(bt->types[bt->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 انگ_Catch;
            r = ReadLEB_u32(&انگ_Catch, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            لكل (u32 i = 0; i < انگ_Catch; i++) {
                r = v_catch_clause(v);
                إذا (r) أرجع r;
            }

            r = v_ڌڪو_ctrl(v, opcode, bt);
            إذا (r) أرجع r;

            إذا (bt) {
                لكل (u16 i = 0; i < bt->انگ_دليلون; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(bt->types[bt->انگ_Rets + i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0x08: // throw
        {
            u32 ٽيگ_اشاريو;
            r = ReadLEB_u32(&ٽيگ_اشاريو, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (!v->module || ٽيگ_اشاريو >= v->module->انگ_ٽيگ_فهرست) أرجع m3_Err_unknown_ٽيگ;
            IM3FuncType tt = v->module->ٽيگ_فهرست[ٽيگ_اشاريو].type;
            إذا (tt) {
                لكل (u16 i = tt->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(tt->types[tt->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
            }
            v_unreachable(v);
            اكسر;
        }

        حالة 0x0a: // throw_ref
            r = v_ڪڍو_expect(v, c_m3Type_exnref, &a);
            إذا (r) أرجع r;
            v_unreachable(v);
            اكسر;
#جيڪڏهن_پورو // d_m3HasExceptionHandling

        حالة 0x05: // else
        {
            Val_Ctrl_چوکٺو چوکٺو;
            r = v_ڪڍو_ctrl(v, &چوکٺو);
            إذا (r) أرجع r;
            إذا (چوکٺو.opcode != 0x04)
                أرجع m3Err_wasmMalformed;
            r = v_ڌڪو_ctrl(v, 0x05, چوکٺو.type);
            إذا (r) أرجع r;
            إذا (چوکٺو.type) {
                لكل (u16 i = 0; i < چوکٺو.type->انگ_دليلون; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(چوکٺو.type->types[چوکٺو.type->انگ_Rets + i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0x0b: //پڇاڙي
        {
            Val_Ctrl_چوکٺو چوکٺو;
            r = v_ڪڍو_ctrl(v, &چوکٺو);
            إذا (r) أرجع r;
            // Push results
            إذا (چوکٺو.type) {
                لكل (u16 i = 0; i < چوکٺو.نتيجو_ڳڻپ; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(چوکٺو.type->types[i]));
                    إذا (r) أرجع r;
                }
            }
            // If this was the outermost frame, we're done
            إذا (v->ctrl_چوٽي == 0)
                أرجع m3Err_none;
            اكسر;
        }

        حالة 0x0c: // br
        {
            u32 اونهائي;
            r = ReadLEB_u32(&اونهائي, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (اونهائي >= v->ctrl_چوٽي) أرجع m3Err_unknownLabel;
            Val_Ctrl_چوکٺو * tgt = &v->ctrl[v->ctrl_چوٽي - 1 - اونهائي];
            r = v_ڪڍو_labels(v, tgt);
            إذا (r) أرجع r;
            v_unreachable(v);
            اكسر;
        }

        حالة 0x0d: // br_if
        {
            u32 اونهائي;
            r = ReadLEB_u32(&اونهائي, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (اونهائي >= v->ctrl_چوٽي) أرجع m3Err_unknownLabel;
            r = v_ڪڍو_expect(v, c_m3Type_i32, &a);
            إذا (r) أرجع r;
            Val_Ctrl_چوکٺو * tgt = &v->ctrl[v->ctrl_چوٽي - 1 - اونهائي];
            r = v_ڪڍو_labels(v, tgt);
            إذا (r) أرجع r;
            r = v_ڌڪو_labels(v, tgt);
            إذا (r) أرجع r;
            اكسر;
        }

        حالة 0x0e: // br_table
        {
            u32 ڳڻپ;
            r = ReadLEB_u32(&ڳڻپ, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            u32 def_اونهائي = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            بائيٽون_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            لكل (u32 i = 0; i <= ڳڻپ; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_پڇاڙي);
                إذا (r) أرجع r;
                إذا (d >= v->ctrl_چوٽي) أرجع m3Err_unknownLabel;
                إذا (i == ڳڻپ) def_اونهائي = d;
            }
            // Now validate all labels match the default's types
            Val_Ctrl_چوکٺو * defTgt = &v->ctrl[v->ctrl_چوٽي - 1 - def_اونهائي];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_ڪڍو_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            إذا (r) أرجع r;

            لكل (u32 i = 0; i <= ڳڻپ; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_پڇاڙي);
                إذا (r) أرجع r;
                Val_Ctrl_چوکٺو * t = &v->ctrl[v->ctrl_چوٽي - 1 - d];
                u16 n = v_label_n(t);
                إذا (n != arity) أرجع m3_Err_type_ڳڻپ_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                لكل (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    إذا (want != c_valBottom && have != c_valBottom && want != have)
                        أرجع m3Err_typeMismatch;
                }
            }
            Val_Ctrl_چوکٺو * dt = &v->ctrl[v->ctrl_چوٽي - 1 - def_اونهائي];
            r = v_ڪڍو_labels(v, dt);
            إذا (r) أرجع r;
            v_unreachable(v);
            اكسر;
        }

        حالة 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            إذا (ft) {
                لكل (u16 i = ft->انگ_Rets; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[i - 1]), &a);
                    إذا (r) أرجع r;
                }
            }
            v_unreachable(v);
            اكسر;
        }

        // ---- Call ----
        حالة 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (idx >= v->module->انگ_Functions) أرجع m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            إذا (ft) {
                لكل (u16 i = ft->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[ft->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
                لكل (u16 i = 0; i < ft->انگ_Rets; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(ft->types[i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            u32 جدول_Idx;
            r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            // the table must exist and hold funcrefs
            إذا (جدول_Idx >= v->module->انگ_Tables) أرجع m3_Err_unknown_جدول;
            إذا (v->module->tables[جدول_Idx]->type != c_m3Type_funcref) أرجع m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_ڪڍو_expect(v, v_جدول_addrtype(v, جدول_Idx), &a); // table index operand
            إذا (r) أرجع r;
            إذا (ft) {
                لكل (u16 i = ft->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[ft->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
                لكل (u16 i = 0; i < ft->انگ_Rets; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(ft->types[i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (idx >= v->module->انگ_Functions) أرجع m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_پڇ_results(v, ft);
            إذا (r) أرجع r;
            إذا (ft) {
                لكل (u16 i = ft->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[ft->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
            }
            v_unreachable(v);
            اكسر;
        }

        حالة 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            u32 جدول_Idx;
            r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            // the table must exist and hold funcrefs
            إذا (جدول_Idx >= v->module->انگ_Tables) أرجع m3_Err_unknown_جدول;
            إذا (v->module->tables[جدول_Idx]->type != c_m3Type_funcref) أرجع m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_پڇ_results(v, ft);
            إذا (r) أرجع r;
            r = v_ڪڍو_expect(v, v_جدول_addrtype(v, جدول_Idx), &a); // table index operand
            إذا (r) أرجع r;
            إذا (ft) {
                لكل (u16 i = ft->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[ft->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
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
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool is_پڇ = (opcode == 0x15);
            إذا (is_پڇ) {
                r = v_check_پڇ_results(v, ft);
                إذا (r) أرجع r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_ڪڍو_expect(v, c_m3Type_funcref, &a);
            إذا (r) أرجع r;
            إذا (ft) {
                لكل (u16 i = ft->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[ft->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
                إذا (not is_پڇ) {
                    لكل (u16 i = 0; i < ft->انگ_Rets; i++) {
                        r = v_ڌڪو(v, بنياد_Type_جو(ft->types[i]));
                        إذا (r) أرجع r;
                    }
                }
            }
            إذا (is_پڇ) v_unreachable(v);
            اكسر;
        }

        حالة 0xd4: // ref.as_non_null
        {
            r = v_ڪڍو_expect(v, c_m3Type_funcref, &a);
            إذا (r) أرجع r;
            r = v_ڌڪو(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            إذا (r) أرجع r;
            اكسر;
        }
#جيڪڏهن_پورو // d_m3HasTypedRefs

        // ---- Parametric ----
        حالة 0x1a: // drop
            r = v_ڪڍو(v, &a);
            إذا (r) أرجع r;
            اكسر;

        حالة 0x1b: // select
        {
            r = v_ڪڍو_expect(v, c_m3Type_i32, &a);
            إذا (r) أرجع r;
            m3type_t t2 = c_valBottom;
            r = v_ڪڍو(v, &t2);
            إذا (r) أرجع r;
            m3type_t t1;
            r = v_ڪڍو_expect(v, t2, &t1);
            إذا (r) أرجع r;
            // untyped select is numeric only; references need the 0x1c form
            إذا (t2 != c_valBottom && IsRefType(t2)) أرجع m3Err_typeMismatch;
            r = v_ڌڪو(v, (t2 == c_valBottom) ? t1 : t2);
            إذا (r) أرجع r;
            اكسر;
        }

#إذا d_m3HasRefTypes
        حالة 0x1c: // select with an explicit result type
        {
            u32 انگ_Types;
            r = ReadLEB_u32(&انگ_Types, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (انگ_Types != 1) أرجع m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_قدر_Type(v->module, &selType, &v->wasm, v->wasm_پڇاڙي);  إذا (r) أرجع r;
            u8 t = بنياد_Type_جو(selType);

            r = v_ڪڍو_expect(v, c_m3Type_i32, &a); إذا (r) أرجع r;
            r = v_ڪڍو_expect(v, t, &a);            إذا (r) أرجع r;
            r = v_ڪڍو_expect(v, t, &a);            إذا (r) أرجع r;
            r = v_ڌڪو(v, t);                      إذا (r) أرجع r;
            اكسر;
        }

        حالة 0x25: // table.get
        حالة 0x26: // table.set
        {
            u32 جدول_Idx;
            r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (جدول_Idx >= v->module->انگ_Tables) أرجع m3_Err_unknown_جدول;
            u8 t = بنياد_Type_جو(v->module->tables[جدول_Idx]->type);
            m3type_t at = v_جدول_addrtype(v, جدول_Idx);

            إذا (opcode == 0x26) {
                r = v_ڪڍو_expect(v, t, &a);   إذا (r) أرجع r;
                r = v_ڪڍو_expect(v, at, &a);  إذا (r) أرجع r;
            } وإلا {
                r = v_ڪڍو_expect(v, at, &a);  إذا (r) أرجع r;
                r = v_ڌڪو(v, t);             إذا (r) أرجع r;
            }
            اكسر;
        }

        حالة 0xd0: // ref.null
        {
#إذا d_m3HasTypedRefs
            m3type_t ڍير_بٽ;
            r = Parse_ڍير_Type(v->module, &ڍير_بٽ, &v->wasm, v->wasm_پڇاڙي);  إذا (r) أرجع r;
            u8 t = (ڍير_بٽ & d_m3Type_refExtern) ? c_m3Type_externref :
# إذا d_m3_Has_ڍير_Switching
                   (ڍير_بٽ & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#وإلا
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasm_پڇاڙي);   إذا (r) أرجع r;
            r = NormalizeType(&t, waType);                   إذا (r) أرجع r;
            إذا (!IsRefType(t)) أرجع m3Err_wasmMalformed;
#جيڪڏهن_پورو
            r = v_ڌڪو(v, t);                                إذا (r) أرجع r;
            اكسر;
        }

        حالة 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_ڪڍو(v, &t);
            إذا (r) أرجع r;
            إذا (t != c_valBottom && !IsRefType(t)) أرجع m3Err_typeMismatch;
            r = v_ڌڪو(v, c_m3Type_i32);
            إذا (r) أرجع r;
            اكسر;
        }

        حالة 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (funcIdx >= v->module->انگ_Functions) أرجع m3Err_unknownFunction;
            إذا (!Module_IsFunctionDeclared(v->module, funcIdx)) أرجع m3Err_undeclaredFuncRef;
            r = v_ڌڪو(v, c_m3Type_funcref);
            إذا (r) أرجع r;
            اكسر;
        }
#جيڪڏهن_پورو

        // ---- Variable ----
        حالة 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (idx >= v->انگ_Locals) أرجع m3Err_unknownLocal;
            r = v_ڌڪو(v, v->localTypes[idx]);
            إذا (r) أرجع r;
            اكسر;
        }

        حالة 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (idx >= v->انگ_Locals) أرجع m3Err_unknownLocal;
            r = v_ڪڍو_expect(v, v->localTypes[idx], &a);
            إذا (r) أرجع r;
            اكسر;
        }

        حالة 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (idx >= v->انگ_Locals) أرجع m3Err_unknownLocal;
            r = v_ڪڍو_expect(v, v->localTypes[idx], &a);
            إذا (r) أرجع r;
            r = v_ڌڪو(v, v->localTypes[idx]);
            إذا (r) أرجع r;
            اكسر;
        }

        حالة 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (idx >= v->module->انگ_Globals) أرجع m3Err_unknownGlobal;
            r = v_ڌڪو(v, بنياد_Type_جو(v->module->globals[idx].type));
            إذا (r) أرجع r;
            اكسر;
        }

        حالة 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
            إذا (idx >= v->module->انگ_Globals) أرجع m3Err_unknownGlobal;
            r = v_ڪڍو_expect(v, بنياد_Type_جو(v->module->globals[idx].type), &a);
            إذا (r) أرجع r;
            اكسر;
        }

        // ---- Memory load ----
        حالة 0x28: حالة 0x29: حالة 0x2a: حالة 0x2b: // i32/i64/f32/f64.load
        حالة 0x2c: حالة 0x2d: حالة 0x2e: حالة 0x2f: // i32.load8/16 s/u
        حالة 0x30: حالة 0x31: حالة 0x32: حالة 0x33: // i64.load8/16 s/u
        حالة 0x34: حالة 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = Read_يادگيري_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (align > v_max_align(opcode)) أرجع m3Err_invalidAlignment;
            إذا (not v_has_يادگيري_idx(v, memidx)) أرجع m3_Err_unknown_يادگيري;
            إذا (not v_وٿي_اندريون_range(v, memidx, offset)) أرجع m3Err_wasmMalformed;
            r = v_ڪڍو_expect(v, v_يادگيري_addrtype(v, memidx), &a); إذا (r) أرجع r;
            u8 نتيجو;
            إذا      (opcode == 0x28) نتيجو = c_m3Type_i32;
            وإلا إذا (opcode == 0x29) نتيجو = c_m3Type_i64;
            وإلا إذا (opcode == 0x2a) نتيجو = c_m3Type_f32;
            وإلا إذا (opcode == 0x2b) نتيجو = c_m3Type_f64;
            وإلا إذا (opcode <= 0x2f) نتيجو = c_m3Type_i32;
            وإلا                     نتيجو = c_m3Type_i64;
            r = v_ڌڪو(v, نتيجو);
            إذا (r) أرجع r;
            اكسر;
        }

        // ---- Memory store ----
        حالة 0x36: حالة 0x37: حالة 0x38: حالة 0x39: // i32/i64/f32/f64.store
        حالة 0x3a: حالة 0x3b:                         // i32.store8/16
        حالة 0x3c: حالة 0x3d: حالة 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = Read_يادگيري_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (align > v_max_align(opcode)) أرجع m3Err_invalidAlignment;
            إذا (not v_has_يادگيري_idx(v, memidx)) أرجع m3_Err_unknown_يادگيري;
            إذا (not v_وٿي_اندريون_range(v, memidx, offset)) أرجع m3Err_wasmMalformed;
            u8 valtype;
            إذا      (opcode == 0x36) valtype = c_m3Type_i32;
            وإلا إذا (opcode == 0x37) valtype = c_m3Type_i64;
            وإلا إذا (opcode == 0x38) valtype = c_m3Type_f32;
            وإلا إذا (opcode == 0x39) valtype = c_m3Type_f64;
            وإلا إذا (opcode <= 0x3b) valtype = c_m3Type_i32;
            وإلا                     valtype = c_m3Type_i64;
            r = v_ڪڍو_expect(v, valtype, &a); إذا (r) أرجع r;
            r = v_ڪڍو_expect(v, v_يادگيري_addrtype(v, memidx), &a); إذا (r) أرجع r;
            اكسر;
        }

        // ---- Memory size/grow ----
        حالة 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (not v_has_يادگيري_idx(v, memidx)) أرجع m3_Err_unknown_يادگيري;
            r = v_ڌڪو(v, v_يادگيري_addrtype(v, memidx)); إذا (r) أرجع r;
            اكسر;
        }
        حالة 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (not v_has_يادگيري_idx(v, memidx)) أرجع m3_Err_unknown_يادگيري;
            r = v_ڪڍو_expect(v, v_يادگيري_addrtype(v, memidx), &a); إذا (r) أرجع r;
            r = v_ڌڪو(v, v_يادگيري_addrtype(v, memidx)); إذا (r) أرجع r;
            اكسر;
        }

        // ---- Constants ----
        حالة 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            r = v_ڌڪو(v, c_m3Type_i32); إذا (r) أرجع r;
            اكسر;
        }
        حالة 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            r = v_ڌڪو(v, c_m3Type_i64); إذا (r) أرجع r;
            اكسر;
        }
        حالة 0x43: { // f32.const
            إذا (v->wasm + 4 > v->wasm_پڇاڙي) أرجع m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ڌڪو(v, c_m3Type_f32); إذا (r) أرجع r;
            اكسر;
        }
        حالة 0x44: { // f64.const
            إذا (v->wasm + 8 > v->wasm_پڇاڙي) أرجع m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ڌڪو(v, c_m3Type_f64); إذا (r) أرجع r;
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
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;
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
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                إذا (not v_has_يادگيري_idx(v, memidx)) أرجع m3_Err_unknown_يادگيري;
                // the segments must have been declared up front by a data count section
                إذا (not v->module->has_معلومات_ڳڻپ) أرجع m3_Err_معلومات_ڳڻپ_Required;
                إذا (dataidx >= v->module->انگ_معلومات_Segments) أرجع m3_Err_unknown_معلومات_Segment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_ڪڍو_expect(v, c_m3Type_i32, &a); إذا (r) أرجع r; // n
                r = v_ڪڍو_expect(v, c_m3Type_i32, &a); إذا (r) أرجع r; // src
                r = v_ڪڍو_expect(v, v_يادگيري_addrtype(v, memidx), &a); إذا (r) أرجع r; // dst
                اكسر;
            }
            حالة 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                // needs the segment, but not a memory
                إذا (not v->module->has_معلومات_ڳڻپ) أرجع m3_Err_معلومات_ڳڻپ_Required;
                إذا (dataidx >= v->module->انگ_معلومات_Segments) أرجع m3_Err_unknown_معلومات_Segment;
                اكسر;
            }
#إذا d_m3HasRefTypes
            حالة 0x0c: // table.init
            حالة 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;

                إذا (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    إذا (a2 >= v->module->انگ_Tables) أرجع m3_Err_unknown_جدول;
                    إذا (a1 >= v->module->انگ_Element_Segments) أرجع m3Err_unknownElemSegment;
                    إذا (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        أرجع m3Err_typeMismatch;
                } وإلا {                                                    // dst, src
                    إذا (a1 >= v->module->انگ_Tables) أرجع m3_Err_unknown_جدول;
                    إذا (a2 >= v->module->انگ_Tables) أرجع m3_Err_unknown_جدول;
                    إذا (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        أرجع m3Err_typeMismatch;
                }

                إذا (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_ڪڍو_expect(v, c_m3Type_i32, &a); إذا (r) أرجع r;             // n
                    r = v_ڪڍو_expect(v, c_m3Type_i32, &a); إذا (r) أرجع r;             // s
                    r = v_ڪڍو_expect(v, v_جدول_addrtype(v, a2), &a); إذا (r) أرجع r;  // d
                } وإلا {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_جدول_addrtype(v, a1);
                    m3type_t sType = v_جدول_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_ڪڍو_expect(v, nType, &a); إذا (r) أرجع r;  // n
                    r = v_ڪڍو_expect(v, sType, &a); إذا (r) أرجع r;  // s
                    r = v_ڪڍو_expect(v, dType, &a); إذا (r) أرجع r;  // d
                }
                اكسر;
            }
            حالة 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                إذا (elemIdx >= v->module->انگ_Element_Segments) أرجع m3Err_unknownElemSegment;
                اكسر;
            }
            حالة 0x0f: // table.grow
            حالة 0x10: // table.size
            حالة 0x11: // table.fill
            {
                u32 جدول_Idx;
                r = ReadLEB_u32(&جدول_Idx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                إذا (جدول_Idx >= v->module->انگ_Tables) أرجع m3_Err_unknown_جدول;
                u8 t = بنياد_Type_جو(v->module->tables[جدول_Idx]->type);
                m3type_t at = v_جدول_addrtype(v, جدول_Idx);

                إذا (sub == 0x10) {                                          // table.size
                    r = v_ڌڪو(v, at); إذا (r) أرجع r;
                } وإلا إذا (sub == 0x0f) {                                   // table.grow
                    r = v_ڪڍو_expect(v, at, &a); إذا (r) أرجع r;  // n
                    r = v_ڪڍو_expect(v, t, &a);  إذا (r) أرجع r;  // init
                    r = v_ڌڪو(v, at);           إذا (r) أرجع r;
                } وإلا {                                                    // table.fill
                    r = v_ڪڍو_expect(v, at, &a); إذا (r) أرجع r;  // n
                    r = v_ڪڍو_expect(v, t, &a);  إذا (r) أرجع r;  // val
                    r = v_ڪڍو_expect(v, at, &a); إذا (r) أرجع r;  // i
                }
                اكسر;
            }
#جيڪڏهن_پورو
            حالة 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                إذا (not v_has_يادگيري_idx(v, dst) or not v_has_يادگيري_idx(v, src)) أرجع m3_Err_unknown_يادگيري;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_يادگيري_addrtype(v, dst);
                m3type_t srcType = v_يادگيري_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_ڪڍو_expect(v, lenType, &a); إذا (r) أرجع r; // n
                r = v_ڪڍو_expect(v, srcType, &a); إذا (r) أرجع r; // src
                r = v_ڪڍو_expect(v, dstType, &a); إذا (r) أرجع r; // dst
                اكسر;
            }
            حالة 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                إذا (not v_has_يادگيري_idx(v, memidx)) أرجع m3_Err_unknown_يادگيري;
                m3type_t addrType = v_يادگيري_addrtype(v, memidx);
                r = v_ڪڍو_expect(v, addrType, &a); إذا (r) أرجع r;      // n
                r = v_ڪڍو_expect(v, c_m3Type_i32, &a); إذا (r) أرجع r;  // val
                r = v_ڪڍو_expect(v, addrType, &a); إذا (r) أرجع r;      // dst
                اكسر;
            }
#إذا d_m3HasWideArithmetic
            حالة 0x13: حالة 0x14: // i64.add128, i64.sub128
            حالة 0x15: حالة 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 انگ_Operands = (sub <= 0x14) ? 4 : 2;
                لكل (u32 i = 0; i < انگ_Operands; ++i) {
                    r = v_ڪڍو_expect(v, c_m3Type_i64, &a); إذا (r) أرجع r;
                }
                r = v_ڌڪو(v, c_m3Type_i64); إذا (r) أرجع r;
                r = v_ڌڪو(v, c_m3Type_i64); إذا (r) أرجع r;
                اكسر;
            }
#جيڪڏهن_پورو
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
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_پڇاڙي);
            إذا (r) أرجع r;

            إذا (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                إذا (reserved != 0) أرجع m3Err_wasmMalformed;
                اكسر;
            }

            u32 ويڪر_2, انگ_Operands; m3type_t قدر_Type, نتيجو_Type;
            إذا (not v_atomic_shape(sub, &ويڪر_2, &انگ_Operands, &قدر_Type, &نتيجو_Type)) أرجع m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = Read_يادگيري_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            إذا ((1u << align) != ويڪر_2) أرجع m3Err_invalidAtomicAlignment;
            إذا (not v_has_يادگيري_idx(v, memidx)) أرجع m3_Err_unknown_يادگيري;
            إذا (not v_وٿي_اندريون_range(v, memidx, offset)) أرجع m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            إذا (sub == 0x01 or sub == 0x02) {
                r = v_ڪڍو_expect(v, c_m3Type_i64, &a); إذا (r) أرجع r;
                انگ_Operands--;
            }
            لكل (u32 i = 1; i < انگ_Operands; ++i) {
                r = v_ڪڍو_expect(v, قدر_Type, &a); إذا (r) أرجع r;
            }
            r = v_ڪڍو_expect(v, v_يادگيري_addrtype(v, memidx), &a); إذا (r) أرجع r;
            إذا (نتيجو_Type != c_m3Type_none) {
                r = v_ڌڪو(v, نتيجو_Type); إذا (r) أرجع r;
            }
            اكسر;
        }
#جيڪڏهن_پورو

#إذا d_m3_Has_ڍير_Switching
        // ---- Stack Switching ----
        حالة 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            إذا (!ft || !ft->isContinuation) أرجع m3Err_typeMismatch;
            r = v_ڪڍو_expect(v, c_m3Type_funcref, &a); إذا (r) أرجع r;
            r = v_ڌڪو(v, c_m3Type_contref); إذا (r) أرجع r;
            اكسر;
        }

        حالة 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (typeIdx1 >= v->module->انگ_Func_Types || typeIdx2 >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            إذا (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) أرجع m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 انگ_Params1 = ft1->انگ_دليلون;
            u16 انگ_Params2 = ft2->انگ_دليلون;
            إذا (انگ_Params1 < انگ_Params2) أرجع m3Err_typeMismatch;
            u32 انگ_Bound = انگ_Params1 - انگ_Params2;

            إذا (ft1->انگ_Rets != ft2->انگ_Rets) أرجع m3Err_typeMismatch;
            لكل (u16 i = 0; i < ft1->انگ_Rets; ++i) {
                إذا (ft1->types[i] != ft2->types[i]) أرجع m3Err_typeMismatch;
            }
            لكل (u16 i = 0; i < انگ_Params2; ++i) {
                إذا (ft1->types[ft1->انگ_Rets + انگ_Bound + i] != ft2->types[ft2->انگ_Rets + i]) أرجع m3Err_typeMismatch;
            }

            r = v_ڪڍو_expect(v, c_m3Type_contref, &a); إذا (r) أرجع r;

            لكل (u32 i = انگ_Bound; i > 0; i--) {
                r = v_ڪڍو_expect(v, بنياد_Type_جو(ft1->types[ft1->انگ_Rets + i - 1]), &a);
                إذا (r) أرجع r;
            }

            r = v_ڌڪو(v, c_m3Type_contref); إذا (r) أرجع r;
            اكسر;
        }

        حالة 0xe2: // suspend
        {
            u32 ٽيگ_Idx;
            r = ReadLEB_u32(&ٽيگ_Idx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (!v->module || ٽيگ_Idx >= v->module->انگ_ٽيگ_فهرست) أرجع m3_Err_unknown_ٽيگ;
            IM3FuncType tt = v->module->ٽيگ_فهرست[ٽيگ_Idx].type;
            إذا (tt) {
                لكل (u16 i = tt->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(tt->types[tt->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
                لكل (u16 i = 0; i < tt->انگ_Rets; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(tt->types[i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) أرجع m3Err_typeMismatch;

            u32 انگ_Clauses;
            r = ReadLEB_u32(&انگ_Clauses, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;

            لكل (u32 i = 0; i < انگ_Clauses; i++) {
                u8 قسم;
                r = Read_u8(&قسم, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                u32 ٽيگ_Idx;
                r = ReadLEB_u32(&ٽيگ_Idx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                r = v_resume_clause(v, قسم, ٽيگ_Idx, ct); إذا (r) أرجع r;
            }

            r = v_ڪڍو_expect(v, c_m3Type_contref, &a); إذا (r) أرجع r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            إذا (ft) {
                لكل (u16 i = ft->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[ft->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
                لكل (u16 i = 0; i < ft->انگ_Rets; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(ft->types[i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0xe4: // resume_throw
        {
            u32 typeIdx, ٽيگ_Idx, انگ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            r = ReadLEB_u32(&ٽيگ_Idx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            r = ReadLEB_u32(&انگ_Clauses, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;

            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            إذا (!v->module || ٽيگ_Idx >= v->module->انگ_ٽيگ_فهرست) أرجع m3_Err_unknown_ٽيگ;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) أرجع m3Err_typeMismatch;
            IM3FuncType ٽيگ_Type = v->module->ٽيگ_فهرست[ٽيگ_Idx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            إذا (ٽيگ_Type && ٽيگ_Type->انگ_Rets != 0) أرجع m3Err_typeMismatch;

            لكل (u32 i = 0; i < انگ_Clauses; i++) {
                u8 قسم;
                r = Read_u8(&قسم, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                u32 c_ٽيگ_Idx;
                r = ReadLEB_u32(&c_ٽيگ_Idx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                r = v_resume_clause(v, قسم, c_ٽيگ_Idx, ct); إذا (r) أرجع r;
            }

            r = v_ڪڍو_expect(v, c_m3Type_contref, &a); إذا (r) أرجع r;

            إذا (ٽيگ_Type) {
                لكل (u16 i = ٽيگ_Type->انگ_دليلون; i > 0; i--) {
                    r = v_ڪڍو_expect(v, بنياد_Type_جو(ٽيگ_Type->types[ٽيگ_Type->انگ_Rets + i - 1]), &a);
                    إذا (r) أرجع r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            إذا (ft) {
                لكل (u16 i = 0; i < ft->انگ_Rets; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(ft->types[i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0xe5: // resume_throw_ref
        {
            u32 typeIdx, انگ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            r = ReadLEB_u32(&انگ_Clauses, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;

            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) أرجع m3Err_typeMismatch;

            لكل (u32 i = 0; i < انگ_Clauses; i++) {
                u8 قسم;
                r = Read_u8(&قسم, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                u32 c_ٽيگ_Idx;
                r = ReadLEB_u32(&c_ٽيگ_Idx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
                r = v_resume_clause(v, قسم, c_ٽيگ_Idx, ct); إذا (r) أرجع r;
            }

            r = v_ڪڍو_expect(v, c_m3Type_contref, &a); إذا (r) أرجع r;
            r = v_ڪڍو_expect(v, c_m3Type_exnref, &a); إذا (r) أرجع r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            إذا (ft) {
                لكل (u16 i = 0; i < ft->انگ_Rets; i++) {
                    r = v_ڌڪو(v, بنياد_Type_جو(ft->types[i]));
                    إذا (r) أرجع r;
                }
            }
            اكسر;
        }

        حالة 0xe6: // switch
        {
            u32 typeIdx, ٽيگ_Idx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;
            r = ReadLEB_u32(&ٽيگ_Idx, &v->wasm, v->wasm_پڇاڙي); إذا (r) أرجع r;

            إذا (typeIdx >= v->module->انگ_Func_Types) أرجع m3Err_unknownType;
            إذا (!v->module || ٽيگ_Idx >= v->module->انگ_ٽيگ_فهرست) أرجع m3_Err_unknown_ٽيگ;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            إذا (!ct || !ct->isContinuation) أرجع m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType ٽيگ_Type = v->module->ٽيگ_فهرست[ٽيگ_Idx].type;
            إذا (ٽيگ_Type && ٽيگ_Type->انگ_دليلون != 0) أرجع m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            إذا (!ft || ft->انگ_دليلون == 0) أرجع m3Err_typeMismatch;

            IM3FuncType peer = Module_Cont_Type_جو_Ref(v->module, ft->types[ft->انگ_Rets + ft->انگ_دليلون - 1]);
            إذا (!peer || !peer->contFuncType) أرجع m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_ڪڍو_expect(v, c_m3Type_contref, &a); إذا (r) أرجع r;

            لكل (u16 i = (u16)(ft->انگ_دليلون - 1); i > 0; i--) {
                r = v_ڪڍو_expect(v, بنياد_Type_جو(ft->types[ft->انگ_Rets + i - 1]), &a);
                إذا (r) أرجع r;
            }

            لكل (u16 i = 0; i < self->انگ_دليلون; i++) {
                r = v_ڌڪو(v, بنياد_Type_جو(self->types[self->انگ_Rets + i]));
                إذا (r) أرجع r;
            }
            اكسر;
        }

#جيڪڏهن_پورو // d_m3HasStackSwitching

        افتراضي:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            اكسر;

        } // switch

        إذا (r) أرجع r;

    } // while

    // If we ran out of bytes without hitting the final end
    أرجع m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3_نتيجو  ValidateFunction  (IM3Function i_function)
{
    إذا (!i_function->wasm) أرجع m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    إذا (!v) {
        v = m3_AllocStruct (ValCtx);
        إذا (!v) أرجع m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, حجم(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasm_پڇاڙي  = i_function->wasm_پڇاڙي;

    // Skip code size LEB
    u32 سائيز;
    M3_نتيجو r = ReadLEB_u32(&سائيز, &v->wasm, v->wasm_پڇاڙي);
    إذا (r) أرجع r;

    // Parse locals
    u32 انگ_Local_بلاڪون;
    r = ReadLEB_u32(&انگ_Local_بلاڪون, &v->wasm, v->wasm_پڇاڙي);
    إذا (r) أرجع r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 انگ_Params = funcType ? funcType->انگ_دليلون : 0;
    إذا (انگ_Params > d_m3_Val_ڍير) أرجع m3_Err_function_ڍير_حد_کان_مٿي;
    لكل (u16 i = 0; i < انگ_Params; i++) {
        v->localTypes[v->انگ_Locals++] = بنياد_Type_جو(funcType->types[funcType->انگ_Rets + i]);
    }

    // Then: declared locals
    لكل (u32 b = 0; b < انگ_Local_بلاڪون; b++) {
        u32 ڳڻپ;
        r = ReadLEB_u32(&ڳڻپ, &v->wasm, v->wasm_پڇاڙي);
        إذا (r) أرجع r;
        m3type_t localType;
        r = Parse_قدر_Type(v->module, &localType, &v->wasm, v->wasm_پڇاڙي);
        إذا (r) أرجع r;
        u8 normalized = بنياد_Type_جو(localType);
        إذا (ڳڻپ > (u32) (d_m3_Val_ڍير - v->انگ_Locals)) أرجع m3_Err_function_ڍير_حد_کان_مٿي;
        لكل (u32 c = 0; c < ڳڻپ; c++) {
            v->localTypes[v->انگ_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ڌڪو_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    إذا (r) أرجع r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    إذا (r) أرجع r;

    // After validation, control stack should be empty and all declared bytes consumed
    إذا (v->ctrl_چوٽي != 0)
        أرجع m3Err_wasmMalformed;

    إذا (v->wasm != v->wasm_پڇاڙي)
        أرجع m3Err_wasmSectionUnderrun;

    أرجع m3Err_none;
}

#وإلا // !d_m3EnableValidation

M3_نتيجو  ValidateFunction  (IM3Function i_function)
{
    (فراغ)i_function;
    أرجع m3Err_none;
}

// clang-format on

#جيڪڏهن_پورو // d_m3EnableValidation
