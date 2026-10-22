//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_validate.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_info.h"

#ਜੇ d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ c_valBottom         0xFF

// ---------- Control frame ----------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ {
    m3opcode_t  opcode;
    u16         ਉਚਾਈ;         // operand stack height at block entry
    u16         param_ਗਿਣਤੀ;
    u16         ਨਤੀਜਾ_ਗਿਣਤੀ;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} Val_Ctrl_ਚੌਖਟਾ;

// ---------- Validator context ----------

ਬਣਤਰ ValCtx {
    ਬਾਈਟਾਂ_t     wasm;
    ਬਾਈਟਾਂ_t     wasm_ਸਿਰਾ;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ਢੇਰ];
    u16         opd_ਸਿਖਰ;

    Val_Ctrl_ਚੌਖਟਾ ctrl [d_m3_Val_Ctrl_ਡੂੰਘਾਈ];
    u16          ctrl_ਸਿਖਰ;

    m3type_t    localTypes [d_m3_Val_ਢੇਰ];
    u16         ਅੰਕ_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ਸਥਿਰ bool v_has_ਸਮ੍ਰਿਤੀ_idx (ValCtx * v, u32 i_ਸਮ੍ਰਿਤੀ_Idx)
{
    ਵਾਪਸ v->module and i_ਸਮ੍ਰਿਤੀ_Idx < v->module->ਅੰਕ_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ਸਥਿਰ m3type_t v_ਸਮ੍ਰਿਤੀ_addrtype (ValCtx * v, u32 i_ਸਮ੍ਰਿਤੀ_Idx)
{
    ਵਾਪਸ ਸਮ੍ਰਿਤੀ_Addr_Type (v->module->memories [i_ਸਮ੍ਰਿਤੀ_Idx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ਸਥਿਰ bool v_ਵਿੱਥ_ਵਿੱਚ_range (ValCtx * v, u32 i_ਸਮ੍ਰਿਤੀ_Idx, u64 i_ਵਿੱਥ)
{
    ਵਾਪਸ v_ਸਮ੍ਰਿਤੀ_addrtype (v, i_ਸਮ੍ਰਿਤੀ_Idx) == c_m3Type_i64 or i_ਵਿੱਥ <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ਸਥਿਰ m3type_t v_ਸਾਰਣੀ_addrtype (ValCtx * v, u32 i_ਸਾਰਣੀ_Idx)
{
    ਵਾਪਸ ਸਾਰਣੀ_Addr_Type (v->module->tables [i_ਸਾਰਣੀ_Idx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
ਸਥਿਰ u32 v_max_align (m3opcode_t opcode)
{
    ਚੋਣ (opcode) {
    ਹਾਲਤ 0x2c: ਹਾਲਤ 0x2d:   // i32.load8_s, i32.load8_u
    ਹਾਲਤ 0x30: ਹਾਲਤ 0x31:   // i64.load8_s, i64.load8_u
    ਹਾਲਤ 0x3a:              // i32.store8
    ਹਾਲਤ 0x3c:              // i64.store8
        ਵਾਪਸ 0;
    ਹਾਲਤ 0x2e: ਹਾਲਤ 0x2f:   // i32.load16_s, i32.load16_u
    ਹਾਲਤ 0x32: ਹਾਲਤ 0x33:   // i64.load16_s, i64.load16_u
    ਹਾਲਤ 0x3b:              // i32.store16
    ਹਾਲਤ 0x3d:              // i64.store16
        ਵਾਪਸ 1;
    ਹਾਲਤ 0x29:              // i64.load
    ਹਾਲਤ 0x2b:              // f64.load
    ਹਾਲਤ 0x37:              // i64.store
    ਹਾਲਤ 0x39:              // f64.store
        ਵਾਪਸ 3;
    ਮੂਲ:                // 32-bit accesses, and a safe fallback
        ਵਾਪਸ 2;
    }
}

#ਜੇ d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
ਸਥਿਰ bool v_atomic_shape (u32 sub, u32 * o_ਚੌੜਾਈ, u32 * o_ਅੰਕ_Operands, m3type_t * o_ਮੁੱਲ_Type, m3type_t * o_ਨਤੀਜਾ_Type)
{
    ਸਥਿਰ ਅਚਲ u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ਸਥਿਰ ਅਚਲ m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    ਜੇ (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_ਚੌੜਾਈ       = (sub == 0x02) ? 8 : 4;
        *o_ਅੰਕ_Operands = (sub == 0x00) ? 2 : 3;
        *o_ਮੁੱਲ_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_ਨਤੀਜਾ_Type  = c_m3Type_i32;
        ਵਾਪਸ true;
    }

    ਜੇ (sub < 0x10 or sub > c_wa_Op_ਆਖਰੀ_Atomic) ਵਾਪਸ false;

    ਜੇ (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_ਅੰਕ_Operands = 1;
    } ਨਹੀਂਤਾਂ ਜੇ (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_ਅੰਕ_Operands = 2;
    } ਨਹੀਂਤਾਂ ਜੇ (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_ਅੰਕ_Operands = 2;
    } ਨਹੀਂਤਾਂ {                        // cmpxchg
        variant = sub - 0x48;       *o_ਅੰਕ_Operands = 3;
    }

    *o_ਚੌੜਾਈ      = c_widths [variant];
    *o_ਮੁੱਲ_Type  = c_types [variant];
    *o_ਨਤੀਜਾ_Type = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    ਵਾਪਸ true;
}

#ਜੇ_ਅੰਤ

// ---------- Operand stack ----------

ਸਥਿਰ M3_ਨਤੀਜਾ v_ਧੱਕੋ (ValCtx * v, m3type_t type)
{
    ਜੇ (M3_UNLIKELY(v->opd_ਸਿਖਰ >= d_m3_Val_ਢੇਰ))
        ਵਾਪਸ m3_Err_function_ਢੇਰ_ਉਛਾਲ;
    v->opd[v->opd_ਸਿਖਰ++] = type;
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ M3_ਨਤੀਜਾ v_ਕੱਢੋ (ValCtx * v, m3type_t * o_type)
{
    Val_Ctrl_ਚੌਖਟਾ * f = &v->ctrl[v->ctrl_ਸਿਖਰ - 1];
    ਜੇ (M3_UNLIKELY(v->opd_ਸਿਖਰ == f->ਉਚਾਈ)) {
        ਜੇ (f->is_unreachable) { *o_type = c_valBottom; ਵਾਪਸ m3Err_none; }
        ਵਾਪਸ m3_Err_function_ਢੇਰ_Underrun;
    }
    *o_type = v->opd[--v->opd_ਸਿਖਰ];
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ M3_ਨਤੀਜਾ v_ਕੱਢੋ_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3_ਨਤੀਜਾ r = v_ਕੱਢੋ(v, &actual);
    ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    ਜੇ (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        ਵਾਪਸ m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    ਵਾਪਸ m3Err_none;
}

// ---------- Control stack ----------

ਸਥਿਰ M3_ਨਤੀਜਾ v_ਧੱਕੋ_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    ਜੇ (M3_UNLIKELY(v->ctrl_ਸਿਖਰ >= d_m3_Val_Ctrl_ਡੂੰਘਾਈ))
        ਵਾਪਸ m3_Err_function_ਢੇਰ_ਉਛਾਲ;
    Val_Ctrl_ਚੌਖਟਾ * f = &v->ctrl[v->ctrl_ਸਿਖਰ++];
    f->opcode       = op;
    f->type         = type;
    f->param_ਗਿਣਤੀ  = type ? type->ਅੰਕ_ਦਲੀਲਾਂ : 0;
    f->ਨਤੀਜਾ_ਗਿਣਤੀ = type ? type->ਅੰਕ_Rets : 0;
    f->ਉਚਾਈ       = v->opd_ਸਿਖਰ;
    f->is_unreachable = false;
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ M3_ਨਤੀਜਾ v_ਕੱਢੋ_ctrl (ValCtx * v, Val_Ctrl_ਚੌਖਟਾ * o_ਚੌਖਟਾ)
{
    ਜੇ (M3_UNLIKELY(v->ctrl_ਸਿਖਰ == 0))
        ਵਾਪਸ m3Err_wasmMalformed;
    Val_Ctrl_ਚੌਖਟਾ * f = &v->ctrl[v->ctrl_ਸਿਖਰ - 1];
    // pop result types
    ਜੇ (f->type) {
        ਲਈ (u16 i = f->ਨਤੀਜਾ_ਗਿਣਤੀ; i > 0; i--) {
            m3type_t a;
            M3_ਨਤੀਜਾ r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(f->type->types[i - 1]), &a);
            ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
        }
    }
    ਜੇ (M3_UNLIKELY(v->opd_ਸਿਖਰ != f->ਉਚਾਈ))
        ਵਾਪਸ m3_Err_type_ਗਿਣਤੀ_Mismatch;
    ਜੇ (o_ਚੌਖਟਾ) *o_ਚੌਖਟਾ = *f;
    v->ctrl_ਸਿਖਰ--;
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ ਖਾਲੀ v_unreachable (ValCtx * v)
{
    Val_Ctrl_ਚੌਖਟਾ * f = &v->ctrl[v->ctrl_ਸਿਖਰ - 1];
    v->opd_ਸਿਖਰ = f->ਉਚਾਈ;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ਸਥਿਰ u16 v_label_n (Val_Ctrl_ਚੌਖਟਾ * f)
{
    ਵਾਪਸ (f->opcode == 0x03) ? f->param_ਗਿਣਤੀ : f->ਨਤੀਜਾ_ਗਿਣਤੀ;
}

ਸਥਿਰ m3type_t v_label_t (Val_Ctrl_ਚੌਖਟਾ * f, u16 i)
{
    ਜੇ (!f->type) ਵਾਪਸ c_m3Type_none;
    ਜੇ (f->opcode == 0x03)
        ਵਾਪਸ ਅਧਾਰ_Type_ਦਾ(f->type->types[f->type->ਅੰਕ_Rets + i]); // params
    ਵਾਪਸ ਅਧਾਰ_Type_ਦਾ(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ਸਥਿਰ m3type_t v_peek (ValCtx * v, u16 i_ਡੂੰਘਾਈ)
{
    Val_Ctrl_ਚੌਖਟਾ * f = &v->ctrl[v->ctrl_ਸਿਖਰ - 1];

    ਜੇ (v->opd_ਸਿਖਰ <= i_ਡੂੰਘਾਈ || (u16)(v->opd_ਸਿਖਰ - i_ਡੂੰਘਾਈ - 1) < f->ਉਚਾਈ)
        ਵਾਪਸ c_valBottom;

    ਵਾਪਸ v->opd[v->opd_ਸਿਖਰ - i_ਡੂੰਘਾਈ - 1];
}

// Pop label types for branch target
ਸਥਿਰ M3_ਨਤੀਜਾ v_ਕੱਢੋ_labels (ValCtx * v, Val_Ctrl_ਚੌਖਟਾ * tgt)
{
    u16 n = v_label_n(tgt);
    ਲਈ (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3_ਨਤੀਜਾ r = v_ਕੱਢੋ_expect(v, v_label_t(tgt, i - 1), &a);
        ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    }
    ਵਾਪਸ m3Err_none;
}

// Push label types back
ਸਥਿਰ M3_ਨਤੀਜਾ v_ਧੱਕੋ_labels (ValCtx * v, Val_Ctrl_ਚੌਖਟਾ * tgt)
{
    u16 n = v_label_n(tgt);
    ਲਈ (u16 i = 0; i < n; i++) {
        M3_ਨਤੀਜਾ r = v_ਧੱਕੋ(v, v_label_t(tgt, i));
        ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    }
    ਵਾਪਸ m3Err_none;
}

// ---------- Block type resolution ----------

ਸਥਿਰ M3_ਨਤੀਜਾ v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    ਜੇ (v->wasm >= v->wasm_ਸਿਰਾ)
        ਵਾਪਸ m3Err_wasmUnderrun;

#ਜੇ d_m3HasTypedRefs
    ਜੇ (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3_ਨਤੀਜਾ rr = Parse_ਮੁੱਲ_Type(v->module, &refType, &v->wasm, v->wasm_ਸਿਰਾ);
        ਜੇ (rr) ਵਾਪਸ rr;
        IM3FuncType ftype = ਨਿਰਾ;
        rr = AllocFuncType(&ftype, 1);
        ਜੇ (rr) ਵਾਪਸ rr;
        ftype->ਅੰਕ_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_ਜੋੜੋ_Func_Type(v->module->environment, &ftype);
        ਜੇ (rr) ਵਾਪਸ rr;
        *o_type = ftype;
        ਵਾਪਸ m3Err_none;
    }
#ਜੇ_ਅੰਤ

    i64 type;
    M3_ਨਤੀਜਾ r = ReadLebSigned(&type, 33, &v->wasm, v->wasm_ਸਿਰਾ);
    ਜੇ (r) ਵਾਪਸ r;

    ਜੇ (type < 0) {
        ਜੇ (type < -64) ਵਾਪਸ m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        ਜੇ (r) ਵਾਪਸ r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } ਨਹੀਂਤਾਂ {
        ਜੇ ((u32)type >= v->module->ਅੰਕ_Func_Types)
            ਵਾਪਸ m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    ਵਾਪਸ m3Err_none;
}

// ---------- Convenience ----------

ਸਥਿਰ M3_ਨਤੀਜਾ v_unop (ValCtx * v, m3type_t ਵਿੱਚ, m3type_t ਬਾਹਰਲਾ)
{
    m3type_t a; M3_ਨਤੀਜਾ r = v_ਕੱਢੋ_expect(v, ਵਿੱਚ, &a);
    ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    ਵਾਪਸ v_ਧੱਕੋ(v, ਬਾਹਰਲਾ);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ਸਥਿਰ M3_ਨਤੀਜਾ v_check_ਪੂਛ_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ਨਿਰਾ;

    u16 ਅੰਕ_Callee_Rets = i_calleeType ? i_calleeType->ਅੰਕ_Rets : 0;
    u16 ਅੰਕ_Func_Rets   = ft ? ft->ਅੰਕ_Rets : 0;

    ਜੇ (ਅੰਕ_Callee_Rets != ਅੰਕ_Func_Rets)
        ਵਾਪਸ m3Err_typeMismatch;

    ਲਈ (u16 i = 0; i < ਅੰਕ_Callee_Rets; i++) {
        ਜੇ (i_calleeType->types[i] != ft->types[i])
            ਵਾਪਸ m3Err_typeMismatch;
    }

    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ M3_ਨਤੀਜਾ v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ਨਤੀਜਾ r;
    r = v_ਕੱਢੋ_expect(v, t, &a); ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    r = v_ਕੱਢੋ_expect(v, t, &a); ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    ਵਾਪਸ v_ਧੱਕੋ(v, t);
}

ਸਥਿਰ M3_ਨਤੀਜਾ v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ਨਤੀਜਾ r;
    r = v_ਕੱਢੋ_expect(v, t, &a); ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    r = v_ਕੱਢੋ_expect(v, t, &a); ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;
    ਵਾਪਸ v_ਧੱਕੋ(v, c_m3Type_i32);
}

ਸਥਿਰ M3_ਨਤੀਜਾ v_testop (ValCtx * v, m3type_t t)
{
    ਵਾਪਸ v_unop(v, t, c_m3Type_i32);
}

ਸਥਿਰ M3_ਨਤੀਜਾ v_cvtop (ValCtx * v, m3type_t ਵਿੱਚ, m3type_t ਬਾਹਰਲਾ)
{
    ਵਾਪਸ v_unop(v, ਵਿੱਚ, ਬਾਹਰਲਾ);
}


// ---------- Main validation loop ----------

#ਜੇ d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ਸਥਿਰ M3_ਨਤੀਜਾ v_catch_clause (ValCtx * v)
{
    u8 ਕਿਸਮ;
    M3_ਨਤੀਜਾ r = Read_u8(&ਕਿਸਮ, &v->wasm, v->wasm_ਸਿਰਾ);
    ਜੇ (r) ਵਾਪਸ r;
    ਜੇ (ਕਿਸਮ > 0x03) ਵਾਪਸ m3Err_wasmMalformed;

    bool has_ਟੈਗ = (ਕਿਸਮ == 0x00 || ਕਿਸਮ == 0x01);
    bool hasRef = (ਕਿਸਮ == 0x01 || ਕਿਸਮ == 0x03);

    IM3FuncType ਟੈਗ_Type = ਨਿਰਾ;
    ਜੇ (has_ਟੈਗ) {
        u32 ਟੈਗ_ਸੂਚਕਾਂਕ;
        r = ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &v->wasm, v->wasm_ਸਿਰਾ);
        ਜੇ (r) ਵਾਪਸ r;
        ਜੇ (!v->module || ਟੈਗ_ਸੂਚਕਾਂਕ >= v->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ) ਵਾਪਸ m3_Err_unknown_ਟੈਗ;
        ਟੈਗ_Type = v->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ].type;
    }

    u32 ਡੂੰਘਾਈ;
    r = ReadLEB_u32(&ਡੂੰਘਾਈ, &v->wasm, v->wasm_ਸਿਰਾ);
    ਜੇ (r) ਵਾਪਸ r;
    ਜੇ (ਡੂੰਘਾਈ >= v->ctrl_ਸਿਖਰ) ਵਾਪਸ m3Err_unknownLabel;

    Val_Ctrl_ਚੌਖਟਾ * tgt = &v->ctrl[v->ctrl_ਸਿਖਰ - 1 - ਡੂੰਘਾਈ];

    u16 ਅੰਕ_Payload = ਟੈਗ_Type ? ਟੈਗ_Type->ਅੰਕ_ਦਲੀਲਾਂ : 0;
    u16 ਅੰਕ_Label   = v_label_n(tgt);

    ਜੇ (ਅੰਕ_Label != ਅੰਕ_Payload + (hasRef ? 1u : 0u))
        ਵਾਪਸ m3_Err_type_ਗਿਣਤੀ_Mismatch;

    ਲਈ (u16 i = 0; i < ਅੰਕ_Payload; i++) {
        ਜੇ (v_label_t(tgt, i) != ਅਧਾਰ_Type_ਦਾ(ਟੈਗ_Type->types[ਟੈਗ_Type->ਅੰਕ_Rets + i]))
            ਵਾਪਸ m3Err_typeMismatch;
    }

    ਜੇ (hasRef and v_label_t(tgt, ਅੰਕ_Payload) != c_m3Type_exnref)
        ਵਾਪਸ m3Err_typeMismatch;

    ਵਾਪਸ m3Err_none;
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling

#ਜੇ d_m3_Has_ਢੇਰ_Switching

ਸਥਿਰ M3_ਨਤੀਜਾ v_resume_clause (ValCtx * v, u8 ਕਿਸਮ, u32 ਟੈਗ_Idx, IM3FuncType ct)
{
    ਜੇ (ਕਿਸਮ > 0x01) ਵਾਪਸ m3Err_wasmMalformed;
    ਜੇ (!v->module || ਟੈਗ_Idx >= v->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ) ਵਾਪਸ m3_Err_unknown_ਟੈਗ;
    IM3FuncType ਟੈਗ_Type = v->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_Idx].type;

    ਜੇ (ਕਿਸਮ == 0x00) {
        u32 ਡੂੰਘਾਈ;
        M3_ਨਤੀਜਾ r = ReadLEB_u32(&ਡੂੰਘਾਈ, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
        ਜੇ (ਡੂੰਘਾਈ >= v->ctrl_ਸਿਖਰ) ਵਾਪਸ m3Err_unknownLabel;

        Val_Ctrl_ਚੌਖਟਾ * tgt = &v->ctrl[v->ctrl_ਸਿਖਰ - 1 - ਡੂੰਘਾਈ];
        u16 ਅੰਕ_Payload = ਟੈਗ_Type ? ਟੈਗ_Type->ਅੰਕ_ਦਲੀਲਾਂ : 0;
        u16 ਅੰਕ_Label   = v_label_n(tgt);

        ਜੇ (ਅੰਕ_Label != ਅੰਕ_Payload + 1u) ਵਾਪਸ m3_Err_type_ਗਿਣਤੀ_Mismatch;

        ਲਈ (u16 j = 0; j < ਅੰਕ_Payload; j++) {
            ਜੇ (v_label_t(tgt, j) != ਅਧਾਰ_Type_ਦਾ(ਟੈਗ_Type->types[ਟੈਗ_Type->ਅੰਕ_Rets + j]))
                ਵਾਪਸ m3Err_typeMismatch;
        }
        ਜੇ (v_label_t(tgt, ਅੰਕ_Payload) != c_m3Type_contref)
            ਵਾਪਸ m3Err_typeMismatch;

#ਜੇ d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->ਅੰਕ_Rets + ਅੰਕ_Payload]
            : tgt->type->types[ਅੰਕ_Payload];

        ਜੇ (ਢੇਰ_Type_ਦਾ(labelContType) == d_m3_Type_ਢੇਰ_Abstract ||
            ਢੇਰ_Type_ਦਾ(labelContType) == d_m3_Type_ਢੇਰ_None) {
            ਵਾਪਸ m3Err_typeMismatch;
        }

        u32 ਢੇਰ_Type_2 = ਢੇਰ_Type_ਦਾ(labelContType);
        IM3FuncType labelCt = ਨਿਰਾ;
        ਲਈ (u32 idx = 0; idx < v->module->ਅੰਕ_Func_Types; ++idx) {
            ਜੇ (v->module->funcTypes[idx]->canonical_ਸੂਚਕਾਂਕ == ਢੇਰ_Type_2 &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                ਰੋਕੋ;
            }
        }
        ਜੇ (!labelCt) ਵਾਪਸ m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_ਅੰਕ_Params = ਟੈਗ_Type ? ਟੈਗ_Type->ਅੰਕ_Rets : 0;
        ਜੇ (labelFt->ਅੰਕ_ਦਲੀਲਾਂ != expected_ਅੰਕ_Params) ਵਾਪਸ m3Err_typeMismatch;
        ਲਈ (u16 p = 0; p < expected_ਅੰਕ_Params; ++p) {
            ਜੇ (labelFt->types[labelFt->ਅੰਕ_Rets + p] != ਟੈਗ_Type->types[p])
                ਵਾਪਸ m3Err_typeMismatch;
        }

        u16 expected_ਅੰਕ_Rets = ft ? ft->ਅੰਕ_Rets : 0;
        ਜੇ (labelFt->ਅੰਕ_Rets != expected_ਅੰਕ_Rets) ਵਾਪਸ m3Err_typeMismatch;
        ਲਈ (u16 r_idx = 0; r_idx < expected_ਅੰਕ_Rets; ++r_idx) {
            ਜੇ (labelFt->types[r_idx] != ft->types[r_idx])
                ਵਾਪਸ m3Err_typeMismatch;
        }
#ਜੇ_ਅੰਤ
    } ਨਹੀਂਤਾਂ {
        ਜੇ (ਟੈਗ_Type && ਟੈਗ_Type->ਅੰਕ_ਦਲੀਲਾਂ != 0) ਵਾਪਸ m3Err_typeMismatch;
    }
    ਵਾਪਸ m3Err_none;
}

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching

ਸਥਿਰ M3_ਨਤੀਜਾ v_validate_body (ValCtx * v)
{
    M3_ਨਤੀਜਾ r = m3Err_none;
    m3type_t a = c_valBottom;

    ਜਦੋਂ (v->wasm < v->wasm_ਸਿਰਾ)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasm_ਸਿਰਾ);
        ਜੇ (M3_UNLIKELY(r)) ਵਾਪਸ r;

        ਚੋਣ (opcode)
        {
        // ---- Control ----
        ਹਾਲਤ 0x00: // unreachable
            v_unreachable(v);
            ਰੋਕੋ;

        ਹਾਲਤ 0x01: // nop
            ਰੋਕੋ;

        ਹਾਲਤ 0x02: //ਖੰਡ
        ਹਾਲਤ 0x03: // loop
        ਹਾਲਤ 0x04: // if
        {
            IM3FuncType bt = ਨਿਰਾ;
            r = v_read_blocktype(v, &bt);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (opcode == 0x04) {
                r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a);
                ਜੇ (r) ਵਾਪਸ r;
            }
            // Pop block params from caller stack
            ਜੇ (bt) {
                ਲਈ (u16 i = bt->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(bt->types[bt->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            r = v_ਧੱਕੋ_ctrl(v, opcode, bt);
            ਜੇ (r) ਵਾਪਸ r;
            // Push params inside block
            ਜੇ (bt) {
                ਲਈ (u16 i = 0; i < bt->ਅੰਕ_ਦਲੀਲਾਂ; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(bt->types[bt->ਅੰਕ_Rets + i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

#ਜੇ d_m3HasExceptionHandling
        ਹਾਲਤ 0x1f: // try_table
        {
            IM3FuncType bt = ਨਿਰਾ;
            r = v_read_blocktype(v, &bt);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (bt) {
                ਲਈ (u16 i = bt->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(bt->types[bt->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 ਅੰਕ_Catch;
            r = ReadLEB_u32(&ਅੰਕ_Catch, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਲਈ (u32 i = 0; i < ਅੰਕ_Catch; i++) {
                r = v_catch_clause(v);
                ਜੇ (r) ਵਾਪਸ r;
            }

            r = v_ਧੱਕੋ_ctrl(v, opcode, bt);
            ਜੇ (r) ਵਾਪਸ r;

            ਜੇ (bt) {
                ਲਈ (u16 i = 0; i < bt->ਅੰਕ_ਦਲੀਲਾਂ; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(bt->types[bt->ਅੰਕ_Rets + i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x08: // throw
        {
            u32 ਟੈਗ_ਸੂਚਕਾਂਕ;
            r = ReadLEB_u32(&ਟੈਗ_ਸੂਚਕਾਂਕ, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (!v->module || ਟੈਗ_ਸੂਚਕਾਂਕ >= v->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ) ਵਾਪਸ m3_Err_unknown_ਟੈਗ;
            IM3FuncType tt = v->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_ਸੂਚਕਾਂਕ].type;
            ਜੇ (tt) {
                ਲਈ (u16 i = tt->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(tt->types[tt->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            v_unreachable(v);
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x0a: // throw_ref
            r = v_ਕੱਢੋ_expect(v, c_m3Type_exnref, &a);
            ਜੇ (r) ਵਾਪਸ r;
            v_unreachable(v);
            ਰੋਕੋ;
#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling

        ਹਾਲਤ 0x05: // else
        {
            Val_Ctrl_ਚੌਖਟਾ ਚੌਖਟਾ;
            r = v_ਕੱਢੋ_ctrl(v, &ਚੌਖਟਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ਚੌਖਟਾ.opcode != 0x04)
                ਵਾਪਸ m3Err_wasmMalformed;
            r = v_ਧੱਕੋ_ctrl(v, 0x05, ਚੌਖਟਾ.type);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ਚੌਖਟਾ.type) {
                ਲਈ (u16 i = 0; i < ਚੌਖਟਾ.type->ਅੰਕ_ਦਲੀਲਾਂ; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ਚੌਖਟਾ.type->types[ਚੌਖਟਾ.type->ਅੰਕ_Rets + i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x0b: //ਸਿਰਾ
        {
            Val_Ctrl_ਚੌਖਟਾ ਚੌਖਟਾ;
            r = v_ਕੱਢੋ_ctrl(v, &ਚੌਖਟਾ);
            ਜੇ (r) ਵਾਪਸ r;
            // Push results
            ਜੇ (ਚੌਖਟਾ.type) {
                ਲਈ (u16 i = 0; i < ਚੌਖਟਾ.ਨਤੀਜਾ_ਗਿਣਤੀ; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ਚੌਖਟਾ.type->types[i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            // If this was the outermost frame, we're done
            ਜੇ (v->ctrl_ਸਿਖਰ == 0)
                ਵਾਪਸ m3Err_none;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x0c: // br
        {
            u32 ਡੂੰਘਾਈ;
            r = ReadLEB_u32(&ਡੂੰਘਾਈ, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ਡੂੰਘਾਈ >= v->ctrl_ਸਿਖਰ) ਵਾਪਸ m3Err_unknownLabel;
            Val_Ctrl_ਚੌਖਟਾ * tgt = &v->ctrl[v->ctrl_ਸਿਖਰ - 1 - ਡੂੰਘਾਈ];
            r = v_ਕੱਢੋ_labels(v, tgt);
            ਜੇ (r) ਵਾਪਸ r;
            v_unreachable(v);
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x0d: // br_if
        {
            u32 ਡੂੰਘਾਈ;
            r = ReadLEB_u32(&ਡੂੰਘਾਈ, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ਡੂੰਘਾਈ >= v->ctrl_ਸਿਖਰ) ਵਾਪਸ m3Err_unknownLabel;
            r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a);
            ਜੇ (r) ਵਾਪਸ r;
            Val_Ctrl_ਚੌਖਟਾ * tgt = &v->ctrl[v->ctrl_ਸਿਖਰ - 1 - ਡੂੰਘਾਈ];
            r = v_ਕੱਢੋ_labels(v, tgt);
            ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ_labels(v, tgt);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x0e: // br_table
        {
            u32 ਗਿਣਤੀ_2;
            r = ReadLEB_u32(&ਗਿਣਤੀ_2, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            u32 def_ਡੂੰਘਾਈ = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            ਬਾਈਟਾਂ_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            ਲਈ (u32 i = 0; i <= ਗਿਣਤੀ_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_ਸਿਰਾ);
                ਜੇ (r) ਵਾਪਸ r;
                ਜੇ (d >= v->ctrl_ਸਿਖਰ) ਵਾਪਸ m3Err_unknownLabel;
                ਜੇ (i == ਗਿਣਤੀ_2) def_ਡੂੰਘਾਈ = d;
            }
            // Now validate all labels match the default's types
            Val_Ctrl_ਚੌਖਟਾ * defTgt = &v->ctrl[v->ctrl_ਸਿਖਰ - 1 - def_ਡੂੰਘਾਈ];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            ਜੇ (r) ਵਾਪਸ r;

            ਲਈ (u32 i = 0; i <= ਗਿਣਤੀ_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_ਸਿਰਾ);
                ਜੇ (r) ਵਾਪਸ r;
                Val_Ctrl_ਚੌਖਟਾ * t = &v->ctrl[v->ctrl_ਸਿਖਰ - 1 - d];
                u16 n = v_label_n(t);
                ਜੇ (n != arity) ਵਾਪਸ m3_Err_type_ਗਿਣਤੀ_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                ਲਈ (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    ਜੇ (want != c_valBottom && have != c_valBottom && want != have)
                        ਵਾਪਸ m3Err_typeMismatch;
                }
            }
            Val_Ctrl_ਚੌਖਟਾ * dt = &v->ctrl[v->ctrl_ਸਿਖਰ - 1 - def_ਡੂੰਘਾਈ];
            r = v_ਕੱਢੋ_labels(v, dt);
            ਜੇ (r) ਵਾਪਸ r;
            v_unreachable(v);
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            ਜੇ (ft) {
                ਲਈ (u16 i = ft->ਅੰਕ_Rets; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            v_unreachable(v);
            ਰੋਕੋ;
        }

        // ---- Call ----
        ਹਾਲਤ 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (idx >= v->module->ਅੰਕ_Functions) ਵਾਪਸ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            ਜੇ (ft) {
                ਲਈ (u16 i = ft->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[ft->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
                ਲਈ (u16 i = 0; i < ft->ਅੰਕ_Rets; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ft->types[i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            u32 ਸਾਰਣੀ_Idx;
            r = ReadLEB_u32(&ਸਾਰਣੀ_Idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ਜੇ (ਸਾਰਣੀ_Idx >= v->module->ਅੰਕ_Tables) ਵਾਪਸ m3_Err_unknown_ਸਾਰਣੀ;
            ਜੇ (v->module->tables[ਸਾਰਣੀ_Idx]->type != c_m3Type_funcref) ਵਾਪਸ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_ਕੱਢੋ_expect(v, v_ਸਾਰਣੀ_addrtype(v, ਸਾਰਣੀ_Idx), &a); // table index operand
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ft) {
                ਲਈ (u16 i = ft->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[ft->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
                ਲਈ (u16 i = 0; i < ft->ਅੰਕ_Rets; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ft->types[i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (idx >= v->module->ਅੰਕ_Functions) ਵਾਪਸ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_ਪੂਛ_results(v, ft);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ft) {
                ਲਈ (u16 i = ft->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[ft->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            v_unreachable(v);
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            u32 ਸਾਰਣੀ_Idx;
            r = ReadLEB_u32(&ਸਾਰਣੀ_Idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ਜੇ (ਸਾਰਣੀ_Idx >= v->module->ਅੰਕ_Tables) ਵਾਪਸ m3_Err_unknown_ਸਾਰਣੀ;
            ਜੇ (v->module->tables[ਸਾਰਣੀ_Idx]->type != c_m3Type_funcref) ਵਾਪਸ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_ਪੂਛ_results(v, ft);
            ਜੇ (r) ਵਾਪਸ r;
            r = v_ਕੱਢੋ_expect(v, v_ਸਾਰਣੀ_addrtype(v, ਸਾਰਣੀ_Idx), &a); // table index operand
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ft) {
                ਲਈ (u16 i = ft->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[ft->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            v_unreachable(v);
            ਰੋਕੋ;
        }

#ਜੇ d_m3HasTypedRefs
        ਹਾਲਤ 0x14: // call_ref
        ਹਾਲਤ 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool is_ਪੂਛ = (opcode == 0x15);
            ਜੇ (is_ਪੂਛ) {
                r = v_check_ਪੂਛ_results(v, ft);
                ਜੇ (r) ਵਾਪਸ r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_ਕੱਢੋ_expect(v, c_m3Type_funcref, &a);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ft) {
                ਲਈ (u16 i = ft->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[ft->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
                ਜੇ (not is_ਪੂਛ) {
                    ਲਈ (u16 i = 0; i < ft->ਅੰਕ_Rets; i++) {
                        r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ft->types[i]));
                        ਜੇ (r) ਵਾਪਸ r;
                    }
                }
            }
            ਜੇ (is_ਪੂਛ) v_unreachable(v);
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xd4: // ref.as_non_null
        {
            r = v_ਕੱਢੋ_expect(v, c_m3Type_funcref, &a);
            ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }
#ਜੇ_ਅੰਤ // d_m3HasTypedRefs

        // ---- Parametric ----
        ਹਾਲਤ 0x1a: // drop
            r = v_ਕੱਢੋ(v, &a);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;

        ਹਾਲਤ 0x1b: // select
        {
            r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a);
            ਜੇ (r) ਵਾਪਸ r;
            m3type_t t2 = c_valBottom;
            r = v_ਕੱਢੋ(v, &t2);
            ਜੇ (r) ਵਾਪਸ r;
            m3type_t t1;
            r = v_ਕੱਢੋ_expect(v, t2, &t1);
            ਜੇ (r) ਵਾਪਸ r;
            // untyped select is numeric only; references need the 0x1c form
            ਜੇ (t2 != c_valBottom && IsRefType(t2)) ਵਾਪਸ m3Err_typeMismatch;
            r = v_ਧੱਕੋ(v, (t2 == c_valBottom) ? t1 : t2);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

#ਜੇ d_m3HasRefTypes
        ਹਾਲਤ 0x1c: // select with an explicit result type
        {
            u32 ਅੰਕ_Types;
            r = ReadLEB_u32(&ਅੰਕ_Types, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ਅੰਕ_Types != 1) ਵਾਪਸ m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_ਮੁੱਲ_Type(v->module, &selType, &v->wasm, v->wasm_ਸਿਰਾ);  ਜੇ (r) ਵਾਪਸ r;
            u8 t = ਅਧਾਰ_Type_ਦਾ(selType);

            r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a); ਜੇ (r) ਵਾਪਸ r;
            r = v_ਕੱਢੋ_expect(v, t, &a);            ਜੇ (r) ਵਾਪਸ r;
            r = v_ਕੱਢੋ_expect(v, t, &a);            ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ(v, t);                      ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x25: // table.get
        ਹਾਲਤ 0x26: // table.set
        {
            u32 ਸਾਰਣੀ_Idx;
            r = ReadLEB_u32(&ਸਾਰਣੀ_Idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ਸਾਰਣੀ_Idx >= v->module->ਅੰਕ_Tables) ਵਾਪਸ m3_Err_unknown_ਸਾਰਣੀ;
            u8 t = ਅਧਾਰ_Type_ਦਾ(v->module->tables[ਸਾਰਣੀ_Idx]->type);
            m3type_t at = v_ਸਾਰਣੀ_addrtype(v, ਸਾਰਣੀ_Idx);

            ਜੇ (opcode == 0x26) {
                r = v_ਕੱਢੋ_expect(v, t, &a);   ਜੇ (r) ਵਾਪਸ r;
                r = v_ਕੱਢੋ_expect(v, at, &a);  ਜੇ (r) ਵਾਪਸ r;
            } ਨਹੀਂਤਾਂ {
                r = v_ਕੱਢੋ_expect(v, at, &a);  ਜੇ (r) ਵਾਪਸ r;
                r = v_ਧੱਕੋ(v, t);             ਜੇ (r) ਵਾਪਸ r;
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xd0: // ref.null
        {
#ਜੇ d_m3HasTypedRefs
            m3type_t ਢੇਰ_ਬਿੱਟ;
            r = Parse_ਢੇਰ_Type(v->module, &ਢੇਰ_ਬਿੱਟ, &v->wasm, v->wasm_ਸਿਰਾ);  ਜੇ (r) ਵਾਪਸ r;
            u8 t = (ਢੇਰ_ਬਿੱਟ & d_m3Type_refExtern) ? c_m3Type_externref :
# ਜੇ d_m3_Has_ਢੇਰ_Switching
                   (ਢੇਰ_ਬਿੱਟ & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#ਨਹੀਂਤਾਂ
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasm_ਸਿਰਾ);   ਜੇ (r) ਵਾਪਸ r;
            r = NormalizeType(&t, waType);                   ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (!IsRefType(t)) ਵਾਪਸ m3Err_wasmMalformed;
#ਜੇ_ਅੰਤ
            r = v_ਧੱਕੋ(v, t);                                ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_ਕੱਢੋ(v, &t);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (t != c_valBottom && !IsRefType(t)) ਵਾਪਸ m3Err_typeMismatch;
            r = v_ਧੱਕੋ(v, c_m3Type_i32);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (funcIdx >= v->module->ਅੰਕ_Functions) ਵਾਪਸ m3Err_unknownFunction;
            ਜੇ (!Module_IsFunctionDeclared(v->module, funcIdx)) ਵਾਪਸ m3Err_undeclaredFuncRef;
            r = v_ਧੱਕੋ(v, c_m3Type_funcref);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }
#ਜੇ_ਅੰਤ

        // ---- Variable ----
        ਹਾਲਤ 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (idx >= v->ਅੰਕ_Locals) ਵਾਪਸ m3Err_unknownLocal;
            r = v_ਧੱਕੋ(v, v->localTypes[idx]);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (idx >= v->ਅੰਕ_Locals) ਵਾਪਸ m3Err_unknownLocal;
            r = v_ਕੱਢੋ_expect(v, v->localTypes[idx], &a);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (idx >= v->ਅੰਕ_Locals) ਵਾਪਸ m3Err_unknownLocal;
            r = v_ਕੱਢੋ_expect(v, v->localTypes[idx], &a);
            ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ(v, v->localTypes[idx]);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (idx >= v->module->ਅੰਕ_Globals) ਵਾਪਸ m3Err_unknownGlobal;
            r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(v->module->globals[idx].type));
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (idx >= v->module->ਅੰਕ_Globals) ਵਾਪਸ m3Err_unknownGlobal;
            r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(v->module->globals[idx].type), &a);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        // ---- Memory load ----
        ਹਾਲਤ 0x28: ਹਾਲਤ 0x29: ਹਾਲਤ 0x2a: ਹਾਲਤ 0x2b: // i32/i64/f32/f64.load
        ਹਾਲਤ 0x2c: ਹਾਲਤ 0x2d: ਹਾਲਤ 0x2e: ਹਾਲਤ 0x2f: // i32.load8/16 s/u
        ਹਾਲਤ 0x30: ਹਾਲਤ 0x31: ਹਾਲਤ 0x32: ਹਾਲਤ 0x33: // i64.load8/16 s/u
        ਹਾਲਤ 0x34: ਹਾਲਤ 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = Read_ਸਮ੍ਰਿਤੀ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (align > v_max_align(opcode)) ਵਾਪਸ m3Err_invalidAlignment;
            ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, memidx)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
            ਜੇ (not v_ਵਿੱਥ_ਵਿੱਚ_range(v, memidx, offset)) ਵਾਪਸ m3Err_wasmMalformed;
            r = v_ਕੱਢੋ_expect(v, v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx), &a); ਜੇ (r) ਵਾਪਸ r;
            u8 ਨਤੀਜਾ;
            ਜੇ      (opcode == 0x28) ਨਤੀਜਾ = c_m3Type_i32;
            ਨਹੀਂਤਾਂ ਜੇ (opcode == 0x29) ਨਤੀਜਾ = c_m3Type_i64;
            ਨਹੀਂਤਾਂ ਜੇ (opcode == 0x2a) ਨਤੀਜਾ = c_m3Type_f32;
            ਨਹੀਂਤਾਂ ਜੇ (opcode == 0x2b) ਨਤੀਜਾ = c_m3Type_f64;
            ਨਹੀਂਤਾਂ ਜੇ (opcode <= 0x2f) ਨਤੀਜਾ = c_m3Type_i32;
            ਨਹੀਂਤਾਂ                     ਨਤੀਜਾ = c_m3Type_i64;
            r = v_ਧੱਕੋ(v, ਨਤੀਜਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        // ---- Memory store ----
        ਹਾਲਤ 0x36: ਹਾਲਤ 0x37: ਹਾਲਤ 0x38: ਹਾਲਤ 0x39: // i32/i64/f32/f64.store
        ਹਾਲਤ 0x3a: ਹਾਲਤ 0x3b:                         // i32.store8/16
        ਹਾਲਤ 0x3c: ਹਾਲਤ 0x3d: ਹਾਲਤ 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = Read_ਸਮ੍ਰਿਤੀ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (align > v_max_align(opcode)) ਵਾਪਸ m3Err_invalidAlignment;
            ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, memidx)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
            ਜੇ (not v_ਵਿੱਥ_ਵਿੱਚ_range(v, memidx, offset)) ਵਾਪਸ m3Err_wasmMalformed;
            u8 valtype;
            ਜੇ      (opcode == 0x36) valtype = c_m3Type_i32;
            ਨਹੀਂਤਾਂ ਜੇ (opcode == 0x37) valtype = c_m3Type_i64;
            ਨਹੀਂਤਾਂ ਜੇ (opcode == 0x38) valtype = c_m3Type_f32;
            ਨਹੀਂਤਾਂ ਜੇ (opcode == 0x39) valtype = c_m3Type_f64;
            ਨਹੀਂਤਾਂ ਜੇ (opcode <= 0x3b) valtype = c_m3Type_i32;
            ਨਹੀਂਤਾਂ                     valtype = c_m3Type_i64;
            r = v_ਕੱਢੋ_expect(v, valtype, &a); ਜੇ (r) ਵਾਪਸ r;
            r = v_ਕੱਢੋ_expect(v, v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx), &a); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        // ---- Memory size/grow ----
        ਹਾਲਤ 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, memidx)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
            r = v_ਧੱਕੋ(v, v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx)); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }
        ਹਾਲਤ 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, memidx)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
            r = v_ਕੱਢੋ_expect(v, v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx), &a); ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ(v, v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx)); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        // ---- Constants ----
        ਹਾਲਤ 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ(v, c_m3Type_i32); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }
        ਹਾਲਤ 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ(v, c_m3Type_i64); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }
        ਹਾਲਤ 0x43: { // f32.const
            ਜੇ (v->wasm + 4 > v->wasm_ਸਿਰਾ) ਵਾਪਸ m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ਧੱਕੋ(v, c_m3Type_f32); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }
        ਹਾਲਤ 0x44: { // f64.const
            ਜੇ (v->wasm + 8 > v->wasm_ਸਿਰਾ) ਵਾਪਸ m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ਧੱਕੋ(v, c_m3Type_f64); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }


        // ---- i32 comparison ----
        ਹਾਲਤ 0x45: r = v_testop(v, c_m3Type_i32); ਰੋਕੋ; // i32.eqz
        ਹਾਲਤ 0x46: ਹਾਲਤ 0x47: ਹਾਲਤ 0x48: ਹਾਲਤ 0x49: ਹਾਲਤ 0x4a:
        ਹਾਲਤ 0x4b: ਹਾਲਤ 0x4c: ਹਾਲਤ 0x4d: ਹਾਲਤ 0x4e: ਹਾਲਤ 0x4f:
            r = v_relop(v, c_m3Type_i32); ਰੋਕੋ;

        // ---- i64 comparison ----
        ਹਾਲਤ 0x50: r = v_testop(v, c_m3Type_i64); ਰੋਕੋ; // i64.eqz
        ਹਾਲਤ 0x51: ਹਾਲਤ 0x52: ਹਾਲਤ 0x53: ਹਾਲਤ 0x54: ਹਾਲਤ 0x55:
        ਹਾਲਤ 0x56: ਹਾਲਤ 0x57: ਹਾਲਤ 0x58: ਹਾਲਤ 0x59: ਹਾਲਤ 0x5a:
            r = v_relop(v, c_m3Type_i64); ਰੋਕੋ;

        // ---- f32 comparison ----
        ਹਾਲਤ 0x5b: ਹਾਲਤ 0x5c: ਹਾਲਤ 0x5d: ਹਾਲਤ 0x5e: ਹਾਲਤ 0x5f: ਹਾਲਤ 0x60:
            r = v_relop(v, c_m3Type_f32); ਰੋਕੋ;

        // ---- f64 comparison ----
        ਹਾਲਤ 0x61: ਹਾਲਤ 0x62: ਹਾਲਤ 0x63: ਹਾਲਤ 0x64: ਹਾਲਤ 0x65: ਹਾਲਤ 0x66:
            r = v_relop(v, c_m3Type_f64); ਰੋਕੋ;

        // ---- i32 unary ----
        ਹਾਲਤ 0x67: ਹਾਲਤ 0x68: ਹਾਲਤ 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ਰੋਕੋ;

        // ---- i32 binary ----
        ਹਾਲਤ 0x6a: ਹਾਲਤ 0x6b: ਹਾਲਤ 0x6c: ਹਾਲਤ 0x6d: ਹਾਲਤ 0x6e: ਹਾਲਤ 0x6f:
        ਹਾਲਤ 0x70: ਹਾਲਤ 0x71: ਹਾਲਤ 0x72: ਹਾਲਤ 0x73: ਹਾਲਤ 0x74: ਹਾਲਤ 0x75:
        ਹਾਲਤ 0x76: ਹਾਲਤ 0x77: ਹਾਲਤ 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); ਰੋਕੋ;

        // ---- i64 unary ----
        ਹਾਲਤ 0x79: ਹਾਲਤ 0x7a: ਹਾਲਤ 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ਰੋਕੋ;

        // ---- i64 binary ----
        ਹਾਲਤ 0x7c: ਹਾਲਤ 0x7d: ਹਾਲਤ 0x7e: ਹਾਲਤ 0x7f: ਹਾਲਤ 0x80: ਹਾਲਤ 0x81:
        ਹਾਲਤ 0x82: ਹਾਲਤ 0x83: ਹਾਲਤ 0x84: ਹਾਲਤ 0x85: ਹਾਲਤ 0x86: ਹਾਲਤ 0x87:
        ਹਾਲਤ 0x88: ਹਾਲਤ 0x89: ਹਾਲਤ 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); ਰੋਕੋ;

        // ---- f32 unary ----
        ਹਾਲਤ 0x8b: ਹਾਲਤ 0x8c: ਹਾਲਤ 0x8d: ਹਾਲਤ 0x8e: ਹਾਲਤ 0x8f:
        ਹਾਲਤ 0x90: ਹਾਲਤ 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); ਰੋਕੋ;

        // ---- f32 binary ----
        ਹਾਲਤ 0x92: ਹਾਲਤ 0x93: ਹਾਲਤ 0x94: ਹਾਲਤ 0x95: ਹਾਲਤ 0x96:
        ਹਾਲਤ 0x97: ਹਾਲਤ 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); ਰੋਕੋ;

        // ---- f64 unary ----
        ਹਾਲਤ 0x99: ਹਾਲਤ 0x9a: ਹਾਲਤ 0x9b: ਹਾਲਤ 0x9c: ਹਾਲਤ 0x9d:
        ਹਾਲਤ 0x9e: ਹਾਲਤ 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); ਰੋਕੋ;

        // ---- f64 binary ----
        ਹਾਲਤ 0xa0: ਹਾਲਤ 0xa1: ਹਾਲਤ 0xa2: ਹਾਲਤ 0xa3: ਹਾਲਤ 0xa4:
        ਹਾਲਤ 0xa5: ਹਾਲਤ 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); ਰੋਕੋ;

        // ---- Conversions ----
        ਹਾਲਤ 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); ਰੋਕੋ; // i32.wrap/i64
        ਹਾਲਤ 0xa8: ਹਾਲਤ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ਰੋਕੋ;
        ਹਾਲਤ 0xaa: ਹਾਲਤ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ਰੋਕੋ;
        ਹਾਲਤ 0xac: ਹਾਲਤ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); ਰੋਕੋ;
        ਹਾਲਤ 0xae: ਹਾਲਤ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ਰੋਕੋ;
        ਹਾਲਤ 0xb0: ਹਾਲਤ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ਰੋਕੋ;
        ਹਾਲਤ 0xb2: ਹਾਲਤ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ਰੋਕੋ;
        ਹਾਲਤ 0xb4: ਹਾਲਤ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); ਰੋਕੋ;
        ਹਾਲਤ 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); ਰੋਕੋ;
        ਹਾਲਤ 0xb7: ਹਾਲਤ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); ਰੋਕੋ;
        ਹਾਲਤ 0xb9: ਹਾਲਤ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ਰੋਕੋ;
        ਹਾਲਤ 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); ਰੋਕੋ;
        ਹਾਲਤ 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ਰੋਕੋ;
        ਹਾਲਤ 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ਰੋਕੋ;
        ਹਾਲਤ 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ਰੋਕੋ;
        ਹਾਲਤ 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ਰੋਕੋ;

        // ---- Sign-extension (MVP post) ----
        ਹਾਲਤ 0xc0: ਹਾਲਤ 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ਰੋਕੋ;
        ਹਾਲਤ 0xc2: ਹਾਲਤ 0xc3: ਹਾਲਤ 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ਰੋਕੋ;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        ਹਾਲਤ 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;
            ਚੋਣ (sub) {
            ਹਾਲਤ 0x00: ਹਾਲਤ 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ਰੋਕੋ;
            ਹਾਲਤ 0x02: ਹਾਲਤ 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ਰੋਕੋ;
            ਹਾਲਤ 0x04: ਹਾਲਤ 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ਰੋਕੋ;
            ਹਾਲਤ 0x06: ਹਾਲਤ 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ਰੋਕੋ;
            ਹਾਲਤ 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, memidx)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
                // the segments must have been declared up front by a data count section
                ਜੇ (not v->module->has_ਅੰਕੜੇ_ਗਿਣਤੀ) ਵਾਪਸ m3_Err_ਅੰਕੜੇ_ਗਿਣਤੀ_Required;
                ਜੇ (dataidx >= v->module->ਅੰਕ_ਅੰਕੜੇ_Segments) ਵਾਪਸ m3_Err_unknown_ਅੰਕੜੇ_Segment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a); ਜੇ (r) ਵਾਪਸ r; // n
                r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a); ਜੇ (r) ਵਾਪਸ r; // src
                r = v_ਕੱਢੋ_expect(v, v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx), &a); ਜੇ (r) ਵਾਪਸ r; // dst
                ਰੋਕੋ;
            }
            ਹਾਲਤ 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                // needs the segment, but not a memory
                ਜੇ (not v->module->has_ਅੰਕੜੇ_ਗਿਣਤੀ) ਵਾਪਸ m3_Err_ਅੰਕੜੇ_ਗਿਣਤੀ_Required;
                ਜੇ (dataidx >= v->module->ਅੰਕ_ਅੰਕੜੇ_Segments) ਵਾਪਸ m3_Err_unknown_ਅੰਕੜੇ_Segment;
                ਰੋਕੋ;
            }
#ਜੇ d_m3HasRefTypes
            ਹਾਲਤ 0x0c: // table.init
            ਹਾਲਤ 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;

                ਜੇ (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    ਜੇ (a2 >= v->module->ਅੰਕ_Tables) ਵਾਪਸ m3_Err_unknown_ਸਾਰਣੀ;
                    ਜੇ (a1 >= v->module->ਅੰਕ_Element_Segments) ਵਾਪਸ m3Err_unknownElemSegment;
                    ਜੇ (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        ਵਾਪਸ m3Err_typeMismatch;
                } ਨਹੀਂਤਾਂ {                                                    // dst, src
                    ਜੇ (a1 >= v->module->ਅੰਕ_Tables) ਵਾਪਸ m3_Err_unknown_ਸਾਰਣੀ;
                    ਜੇ (a2 >= v->module->ਅੰਕ_Tables) ਵਾਪਸ m3_Err_unknown_ਸਾਰਣੀ;
                    ਜੇ (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        ਵਾਪਸ m3Err_typeMismatch;
                }

                ਜੇ (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a); ਜੇ (r) ਵਾਪਸ r;             // n
                    r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a); ਜੇ (r) ਵਾਪਸ r;             // s
                    r = v_ਕੱਢੋ_expect(v, v_ਸਾਰਣੀ_addrtype(v, a2), &a); ਜੇ (r) ਵਾਪਸ r;  // d
                } ਨਹੀਂਤਾਂ {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_ਸਾਰਣੀ_addrtype(v, a1);
                    m3type_t sType = v_ਸਾਰਣੀ_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_ਕੱਢੋ_expect(v, nType, &a); ਜੇ (r) ਵਾਪਸ r;  // n
                    r = v_ਕੱਢੋ_expect(v, sType, &a); ਜੇ (r) ਵਾਪਸ r;  // s
                    r = v_ਕੱਢੋ_expect(v, dType, &a); ਜੇ (r) ਵਾਪਸ r;  // d
                }
                ਰੋਕੋ;
            }
            ਹਾਲਤ 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                ਜੇ (elemIdx >= v->module->ਅੰਕ_Element_Segments) ਵਾਪਸ m3Err_unknownElemSegment;
                ਰੋਕੋ;
            }
            ਹਾਲਤ 0x0f: // table.grow
            ਹਾਲਤ 0x10: // table.size
            ਹਾਲਤ 0x11: // table.fill
            {
                u32 ਸਾਰਣੀ_Idx;
                r = ReadLEB_u32(&ਸਾਰਣੀ_Idx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                ਜੇ (ਸਾਰਣੀ_Idx >= v->module->ਅੰਕ_Tables) ਵਾਪਸ m3_Err_unknown_ਸਾਰਣੀ;
                u8 t = ਅਧਾਰ_Type_ਦਾ(v->module->tables[ਸਾਰਣੀ_Idx]->type);
                m3type_t at = v_ਸਾਰਣੀ_addrtype(v, ਸਾਰਣੀ_Idx);

                ਜੇ (sub == 0x10) {                                          // table.size
                    r = v_ਧੱਕੋ(v, at); ਜੇ (r) ਵਾਪਸ r;
                } ਨਹੀਂਤਾਂ ਜੇ (sub == 0x0f) {                                   // table.grow
                    r = v_ਕੱਢੋ_expect(v, at, &a); ਜੇ (r) ਵਾਪਸ r;  // n
                    r = v_ਕੱਢੋ_expect(v, t, &a);  ਜੇ (r) ਵਾਪਸ r;  // init
                    r = v_ਧੱਕੋ(v, at);           ਜੇ (r) ਵਾਪਸ r;
                } ਨਹੀਂਤਾਂ {                                                    // table.fill
                    r = v_ਕੱਢੋ_expect(v, at, &a); ਜੇ (r) ਵਾਪਸ r;  // n
                    r = v_ਕੱਢੋ_expect(v, t, &a);  ਜੇ (r) ਵਾਪਸ r;  // val
                    r = v_ਕੱਢੋ_expect(v, at, &a); ਜੇ (r) ਵਾਪਸ r;  // i
                }
                ਰੋਕੋ;
            }
#ਜੇ_ਅੰਤ
            ਹਾਲਤ 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, dst) or not v_has_ਸਮ੍ਰਿਤੀ_idx(v, src)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_ਸਮ੍ਰਿਤੀ_addrtype(v, dst);
                m3type_t srcType = v_ਸਮ੍ਰਿਤੀ_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_ਕੱਢੋ_expect(v, lenType, &a); ਜੇ (r) ਵਾਪਸ r; // n
                r = v_ਕੱਢੋ_expect(v, srcType, &a); ਜੇ (r) ਵਾਪਸ r; // src
                r = v_ਕੱਢੋ_expect(v, dstType, &a); ਜੇ (r) ਵਾਪਸ r; // dst
                ਰੋਕੋ;
            }
            ਹਾਲਤ 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, memidx)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
                m3type_t addrType = v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx);
                r = v_ਕੱਢੋ_expect(v, addrType, &a); ਜੇ (r) ਵਾਪਸ r;      // n
                r = v_ਕੱਢੋ_expect(v, c_m3Type_i32, &a); ਜੇ (r) ਵਾਪਸ r;  // val
                r = v_ਕੱਢੋ_expect(v, addrType, &a); ਜੇ (r) ਵਾਪਸ r;      // dst
                ਰੋਕੋ;
            }
#ਜੇ d_m3HasWideArithmetic
            ਹਾਲਤ 0x13: ਹਾਲਤ 0x14: // i64.add128, i64.sub128
            ਹਾਲਤ 0x15: ਹਾਲਤ 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 ਅੰਕ_Operands = (sub <= 0x14) ? 4 : 2;
                ਲਈ (u32 i = 0; i < ਅੰਕ_Operands; ++i) {
                    r = v_ਕੱਢੋ_expect(v, c_m3Type_i64, &a); ਜੇ (r) ਵਾਪਸ r;
                }
                r = v_ਧੱਕੋ(v, c_m3Type_i64); ਜੇ (r) ਵਾਪਸ r;
                r = v_ਧੱਕੋ(v, c_m3Type_i64); ਜੇ (r) ਵਾਪਸ r;
                ਰੋਕੋ;
            }
#ਜੇ_ਅੰਤ
            ਮੂਲ:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                ਰੋਕੋ;
            }
            ਰੋਕੋ;
        }

#ਜੇ d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        ਹਾਲਤ 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_ਸਿਰਾ);
            ਜੇ (r) ਵਾਪਸ r;

            ਜੇ (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                ਜੇ (reserved != 0) ਵਾਪਸ m3Err_wasmMalformed;
                ਰੋਕੋ;
            }

            u32 ਚੌੜਾਈ_2, ਅੰਕ_Operands; m3type_t ਮੁੱਲ_Type, ਨਤੀਜਾ_Type;
            ਜੇ (not v_atomic_shape(sub, &ਚੌੜਾਈ_2, &ਅੰਕ_Operands, &ਮੁੱਲ_Type, &ਨਤੀਜਾ_Type)) ਵਾਪਸ m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = Read_ਸਮ੍ਰਿਤੀ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            ਜੇ ((1u << align) != ਚੌੜਾਈ_2) ਵਾਪਸ m3Err_invalidAtomicAlignment;
            ਜੇ (not v_has_ਸਮ੍ਰਿਤੀ_idx(v, memidx)) ਵਾਪਸ m3_Err_unknown_ਸਮ੍ਰਿਤੀ;
            ਜੇ (not v_ਵਿੱਥ_ਵਿੱਚ_range(v, memidx, offset)) ਵਾਪਸ m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            ਜੇ (sub == 0x01 or sub == 0x02) {
                r = v_ਕੱਢੋ_expect(v, c_m3Type_i64, &a); ਜੇ (r) ਵਾਪਸ r;
                ਅੰਕ_Operands--;
            }
            ਲਈ (u32 i = 1; i < ਅੰਕ_Operands; ++i) {
                r = v_ਕੱਢੋ_expect(v, ਮੁੱਲ_Type, &a); ਜੇ (r) ਵਾਪਸ r;
            }
            r = v_ਕੱਢੋ_expect(v, v_ਸਮ੍ਰਿਤੀ_addrtype(v, memidx), &a); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (ਨਤੀਜਾ_Type != c_m3Type_none) {
                r = v_ਧੱਕੋ(v, ਨਤੀਜਾ_Type); ਜੇ (r) ਵਾਪਸ r;
            }
            ਰੋਕੋ;
        }
#ਜੇ_ਅੰਤ

#ਜੇ d_m3_Has_ਢੇਰ_Switching
        // ---- Stack Switching ----
        ਹਾਲਤ 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            ਜੇ (!ft || !ft->isContinuation) ਵਾਪਸ m3Err_typeMismatch;
            r = v_ਕੱਢੋ_expect(v, c_m3Type_funcref, &a); ਜੇ (r) ਵਾਪਸ r;
            r = v_ਧੱਕੋ(v, c_m3Type_contref); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (typeIdx1 >= v->module->ਅੰਕ_Func_Types || typeIdx2 >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            ਜੇ (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) ਵਾਪਸ m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 ਅੰਕ_Params1 = ft1->ਅੰਕ_ਦਲੀਲਾਂ;
            u16 ਅੰਕ_Params2 = ft2->ਅੰਕ_ਦਲੀਲਾਂ;
            ਜੇ (ਅੰਕ_Params1 < ਅੰਕ_Params2) ਵਾਪਸ m3Err_typeMismatch;
            u32 ਅੰਕ_Bound = ਅੰਕ_Params1 - ਅੰਕ_Params2;

            ਜੇ (ft1->ਅੰਕ_Rets != ft2->ਅੰਕ_Rets) ਵਾਪਸ m3Err_typeMismatch;
            ਲਈ (u16 i = 0; i < ft1->ਅੰਕ_Rets; ++i) {
                ਜੇ (ft1->types[i] != ft2->types[i]) ਵਾਪਸ m3Err_typeMismatch;
            }
            ਲਈ (u16 i = 0; i < ਅੰਕ_Params2; ++i) {
                ਜੇ (ft1->types[ft1->ਅੰਕ_Rets + ਅੰਕ_Bound + i] != ft2->types[ft2->ਅੰਕ_Rets + i]) ਵਾਪਸ m3Err_typeMismatch;
            }

            r = v_ਕੱਢੋ_expect(v, c_m3Type_contref, &a); ਜੇ (r) ਵਾਪਸ r;

            ਲਈ (u32 i = ਅੰਕ_Bound; i > 0; i--) {
                r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft1->types[ft1->ਅੰਕ_Rets + i - 1]), &a);
                ਜੇ (r) ਵਾਪਸ r;
            }

            r = v_ਧੱਕੋ(v, c_m3Type_contref); ਜੇ (r) ਵਾਪਸ r;
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xe2: // suspend
        {
            u32 ਟੈਗ_Idx;
            r = ReadLEB_u32(&ਟੈਗ_Idx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (!v->module || ਟੈਗ_Idx >= v->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ) ਵਾਪਸ m3_Err_unknown_ਟੈਗ;
            IM3FuncType tt = v->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_Idx].type;
            ਜੇ (tt) {
                ਲਈ (u16 i = tt->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(tt->types[tt->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
                ਲਈ (u16 i = 0; i < tt->ਅੰਕ_Rets; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(tt->types[i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ਜੇ (!ct || !ct->isContinuation) ਵਾਪਸ m3Err_typeMismatch;

            u32 ਅੰਕ_Clauses;
            r = ReadLEB_u32(&ਅੰਕ_Clauses, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;

            ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; i++) {
                u8 ਕਿਸਮ;
                r = Read_u8(&ਕਿਸਮ, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                u32 ਟੈਗ_Idx;
                r = ReadLEB_u32(&ਟੈਗ_Idx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                r = v_resume_clause(v, ਕਿਸਮ, ਟੈਗ_Idx, ct); ਜੇ (r) ਵਾਪਸ r;
            }

            r = v_ਕੱਢੋ_expect(v, c_m3Type_contref, &a); ਜੇ (r) ਵਾਪਸ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ਜੇ (ft) {
                ਲਈ (u16 i = ft->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[ft->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
                ਲਈ (u16 i = 0; i < ft->ਅੰਕ_Rets; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ft->types[i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xe4: // resume_throw
        {
            u32 typeIdx, ਟੈਗ_Idx, ਅੰਕ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            r = ReadLEB_u32(&ਟੈਗ_Idx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            r = ReadLEB_u32(&ਅੰਕ_Clauses, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;

            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            ਜੇ (!v->module || ਟੈਗ_Idx >= v->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ) ਵਾਪਸ m3_Err_unknown_ਟੈਗ;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ਜੇ (!ct || !ct->isContinuation) ਵਾਪਸ m3Err_typeMismatch;
            IM3FuncType ਟੈਗ_Type = v->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_Idx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            ਜੇ (ਟੈਗ_Type && ਟੈਗ_Type->ਅੰਕ_Rets != 0) ਵਾਪਸ m3Err_typeMismatch;

            ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; i++) {
                u8 ਕਿਸਮ;
                r = Read_u8(&ਕਿਸਮ, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                u32 c_ਟੈਗ_Idx;
                r = ReadLEB_u32(&c_ਟੈਗ_Idx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                r = v_resume_clause(v, ਕਿਸਮ, c_ਟੈਗ_Idx, ct); ਜੇ (r) ਵਾਪਸ r;
            }

            r = v_ਕੱਢੋ_expect(v, c_m3Type_contref, &a); ਜੇ (r) ਵਾਪਸ r;

            ਜੇ (ਟੈਗ_Type) {
                ਲਈ (u16 i = ਟੈਗ_Type->ਅੰਕ_ਦਲੀਲਾਂ; i > 0; i--) {
                    r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ਟੈਗ_Type->types[ਟੈਗ_Type->ਅੰਕ_Rets + i - 1]), &a);
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ਜੇ (ft) {
                ਲਈ (u16 i = 0; i < ft->ਅੰਕ_Rets; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ft->types[i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xe5: // resume_throw_ref
        {
            u32 typeIdx, ਅੰਕ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            r = ReadLEB_u32(&ਅੰਕ_Clauses, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;

            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ਜੇ (!ct || !ct->isContinuation) ਵਾਪਸ m3Err_typeMismatch;

            ਲਈ (u32 i = 0; i < ਅੰਕ_Clauses; i++) {
                u8 ਕਿਸਮ;
                r = Read_u8(&ਕਿਸਮ, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                u32 c_ਟੈਗ_Idx;
                r = ReadLEB_u32(&c_ਟੈਗ_Idx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
                r = v_resume_clause(v, ਕਿਸਮ, c_ਟੈਗ_Idx, ct); ਜੇ (r) ਵਾਪਸ r;
            }

            r = v_ਕੱਢੋ_expect(v, c_m3Type_contref, &a); ਜੇ (r) ਵਾਪਸ r;
            r = v_ਕੱਢੋ_expect(v, c_m3Type_exnref, &a); ਜੇ (r) ਵਾਪਸ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ਜੇ (ft) {
                ਲਈ (u16 i = 0; i < ft->ਅੰਕ_Rets; i++) {
                    r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(ft->types[i]));
                    ਜੇ (r) ਵਾਪਸ r;
                }
            }
            ਰੋਕੋ;
        }

        ਹਾਲਤ 0xe6: // switch
        {
            u32 typeIdx, ਟੈਗ_Idx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;
            r = ReadLEB_u32(&ਟੈਗ_Idx, &v->wasm, v->wasm_ਸਿਰਾ); ਜੇ (r) ਵਾਪਸ r;

            ਜੇ (typeIdx >= v->module->ਅੰਕ_Func_Types) ਵਾਪਸ m3Err_unknownType;
            ਜੇ (!v->module || ਟੈਗ_Idx >= v->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ) ਵਾਪਸ m3_Err_unknown_ਟੈਗ;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ਜੇ (!ct || !ct->isContinuation) ਵਾਪਸ m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType ਟੈਗ_Type = v->module->ਟੈਗ_ਸੂਚੀ[ਟੈਗ_Idx].type;
            ਜੇ (ਟੈਗ_Type && ਟੈਗ_Type->ਅੰਕ_ਦਲੀਲਾਂ != 0) ਵਾਪਸ m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            ਜੇ (!ft || ft->ਅੰਕ_ਦਲੀਲਾਂ == 0) ਵਾਪਸ m3Err_typeMismatch;

            IM3FuncType peer = Module_Cont_Type_ਦਾ_Ref(v->module, ft->types[ft->ਅੰਕ_Rets + ft->ਅੰਕ_ਦਲੀਲਾਂ - 1]);
            ਜੇ (!peer || !peer->contFuncType) ਵਾਪਸ m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_ਕੱਢੋ_expect(v, c_m3Type_contref, &a); ਜੇ (r) ਵਾਪਸ r;

            ਲਈ (u16 i = (u16)(ft->ਅੰਕ_ਦਲੀਲਾਂ - 1); i > 0; i--) {
                r = v_ਕੱਢੋ_expect(v, ਅਧਾਰ_Type_ਦਾ(ft->types[ft->ਅੰਕ_Rets + i - 1]), &a);
                ਜੇ (r) ਵਾਪਸ r;
            }

            ਲਈ (u16 i = 0; i < self->ਅੰਕ_ਦਲੀਲਾਂ; i++) {
                r = v_ਧੱਕੋ(v, ਅਧਾਰ_Type_ਦਾ(self->types[self->ਅੰਕ_Rets + i]));
                ਜੇ (r) ਵਾਪਸ r;
            }
            ਰੋਕੋ;
        }

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching

        ਮੂਲ:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            ਰੋਕੋ;

        } // switch

        ਜੇ (r) ਵਾਪਸ r;

    } // while

    // If we ran out of bytes without hitting the final end
    ਵਾਪਸ m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3_ਨਤੀਜਾ  ValidateFunction  (IM3Function i_function)
{
    ਜੇ (!i_function->wasm) ਵਾਪਸ m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    ਜੇ (!v) {
        v = m3_AllocStruct (ValCtx);
        ਜੇ (!v) ਵਾਪਸ m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, ਆਕਾਰ(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasm_ਸਿਰਾ  = i_function->wasm_ਸਿਰਾ;

    // Skip code size LEB
    u32 ਮਾਪ;
    M3_ਨਤੀਜਾ r = ReadLEB_u32(&ਮਾਪ, &v->wasm, v->wasm_ਸਿਰਾ);
    ਜੇ (r) ਵਾਪਸ r;

    // Parse locals
    u32 ਅੰਕ_Local_ਖੰਡਾਂ;
    r = ReadLEB_u32(&ਅੰਕ_Local_ਖੰਡਾਂ, &v->wasm, v->wasm_ਸਿਰਾ);
    ਜੇ (r) ਵਾਪਸ r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 ਅੰਕ_Params = funcType ? funcType->ਅੰਕ_ਦਲੀਲਾਂ : 0;
    ਜੇ (ਅੰਕ_Params > d_m3_Val_ਢੇਰ) ਵਾਪਸ m3_Err_function_ਢੇਰ_ਉਛਾਲ;
    ਲਈ (u16 i = 0; i < ਅੰਕ_Params; i++) {
        v->localTypes[v->ਅੰਕ_Locals++] = ਅਧਾਰ_Type_ਦਾ(funcType->types[funcType->ਅੰਕ_Rets + i]);
    }

    // Then: declared locals
    ਲਈ (u32 b = 0; b < ਅੰਕ_Local_ਖੰਡਾਂ; b++) {
        u32 ਗਿਣਤੀ_2;
        r = ReadLEB_u32(&ਗਿਣਤੀ_2, &v->wasm, v->wasm_ਸਿਰਾ);
        ਜੇ (r) ਵਾਪਸ r;
        m3type_t localType;
        r = Parse_ਮੁੱਲ_Type(v->module, &localType, &v->wasm, v->wasm_ਸਿਰਾ);
        ਜੇ (r) ਵਾਪਸ r;
        u8 normalized = ਅਧਾਰ_Type_ਦਾ(localType);
        ਜੇ (ਗਿਣਤੀ_2 > (u32) (d_m3_Val_ਢੇਰ - v->ਅੰਕ_Locals)) ਵਾਪਸ m3_Err_function_ਢੇਰ_ਉਛਾਲ;
        ਲਈ (u32 c = 0; c < ਗਿਣਤੀ_2; c++) {
            v->localTypes[v->ਅੰਕ_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ਧੱਕੋ_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    ਜੇ (r) ਵਾਪਸ r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    ਜੇ (r) ਵਾਪਸ r;

    // After validation, control stack should be empty and all declared bytes consumed
    ਜੇ (v->ctrl_ਸਿਖਰ != 0)
        ਵਾਪਸ m3Err_wasmMalformed;

    ਜੇ (v->wasm != v->wasm_ਸਿਰਾ)
        ਵਾਪਸ m3Err_wasmSectionUnderrun;

    ਵਾਪਸ m3Err_none;
}

#ਨਹੀਂਤਾਂ // !d_m3EnableValidation

M3_ਨਤੀਜਾ  ValidateFunction  (IM3Function i_function)
{
    (ਖਾਲੀ)i_function;
    ਵਾਪਸ m3Err_none;
}

// clang-format on

#ਜੇ_ਅੰਤ // d_m3EnableValidation
