//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#ಅನ್ತರ್ಭಾವ "m3_validate.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"

#ಆದರೆ d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#ವ್ಯಾಖ್ಯಾನಿಸು c_valBottom         0xFF

// ---------- Control frame ----------

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ {
    m3opcode_t  opcode;
    u16         ಎತ್ತರ;         // operand stack height at block entry
    u16         param_ಎಣಿಕೆ;
    u16         ಫಲಿತಾಂಶ_ಎಣಿಕೆ;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} Val_Ctrl_ಚೌಕಟ್ಟು;

// ---------- Validator context ----------

ರಚನೆ ValCtx {
    ಬೈಟುಗಳು_t     wasm;
    ಬೈಟುಗಳು_t     wasm_ಕೊನೆ;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ರಾಶಿ];
    u16         opd_ತುದಿ;

    Val_Ctrl_ಚೌಕಟ್ಟು ctrl [d_m3_Val_Ctrl_ಆಳ];
    u16          ctrl_ತುದಿ;

    m3type_t    localTypes [d_m3_Val_ರಾಶಿ];
    u16         ಅಂಕೆ_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ಸ್ಥಿರ bool v_has_ಸ್ಮರಣೆ_idx (ValCtx * v, u32 i_ಸ್ಮರಣೆ_Idx)
{
    ಹಿಂತಿರುಗಿ v->module and i_ಸ್ಮರಣೆ_Idx < v->module->ಅಂಕೆ_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ಸ್ಥಿರ m3type_t v_ಸ್ಮರಣೆ_addrtype (ValCtx * v, u32 i_ಸ್ಮರಣೆ_Idx)
{
    ಹಿಂತಿರುಗಿ ಸ್ಮರಣೆ_Addr_Type (v->module->memories [i_ಸ್ಮರಣೆ_Idx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ಸ್ಥಿರ bool v_ಸ್ಥಾನಾಂತರ_ಒಳ_range (ValCtx * v, u32 i_ಸ್ಮರಣೆ_Idx, u64 i_ಸ್ಥಾನಾಂತರ)
{
    ಹಿಂತಿರುಗಿ v_ಸ್ಮರಣೆ_addrtype (v, i_ಸ್ಮರಣೆ_Idx) == c_m3Type_i64 or i_ಸ್ಥಾನಾಂತರ <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ಸ್ಥಿರ m3type_t v_ಕೋಷ್ಟಕ_addrtype (ValCtx * v, u32 i_ಕೋಷ್ಟಕ_Idx)
{
    ಹಿಂತಿರುಗಿ ಕೋಷ್ಟಕ_Addr_Type (v->module->tables [i_ಕೋಷ್ಟಕ_Idx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
ಸ್ಥಿರ u32 v_max_align (m3opcode_t opcode)
{
    ಆಯ್ಕೆ (opcode) {
    ಸ್ಥಿತಿ 0x2c: ಸ್ಥಿತಿ 0x2d:   // i32.load8_s, i32.load8_u
    ಸ್ಥಿತಿ 0x30: ಸ್ಥಿತಿ 0x31:   // i64.load8_s, i64.load8_u
    ಸ್ಥಿತಿ 0x3a:              // i32.store8
    ಸ್ಥಿತಿ 0x3c:              // i64.store8
        ಹಿಂತಿರುಗಿ 0;
    ಸ್ಥಿತಿ 0x2e: ಸ್ಥಿತಿ 0x2f:   // i32.load16_s, i32.load16_u
    ಸ್ಥಿತಿ 0x32: ಸ್ಥಿತಿ 0x33:   // i64.load16_s, i64.load16_u
    ಸ್ಥಿತಿ 0x3b:              // i32.store16
    ಸ್ಥಿತಿ 0x3d:              // i64.store16
        ಹಿಂತಿರುಗಿ 1;
    ಸ್ಥಿತಿ 0x29:              // i64.load
    ಸ್ಥಿತಿ 0x2b:              // f64.load
    ಸ್ಥಿತಿ 0x37:              // i64.store
    ಸ್ಥಿತಿ 0x39:              // f64.store
        ಹಿಂತಿರುಗಿ 3;
    ಪೂರ್ವನಿಯೋಜಿತ:                // 32-bit accesses, and a safe fallback
        ಹಿಂತಿರುಗಿ 2;
    }
}

#ಆದರೆ d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
ಸ್ಥಿರ bool v_atomic_shape (u32 sub, u32 * o_ಅಗಲ, u32 * o_ಅಂಕೆ_Operands, m3type_t * o_ಮೌಲ್ಯ_Type, m3type_t * o_ಫಲಿತಾಂಶ_Type)
{
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    ಆದರೆ (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_ಅಗಲ       = (sub == 0x02) ? 8 : 4;
        *o_ಅಂಕೆ_Operands = (sub == 0x00) ? 2 : 3;
        *o_ಮೌಲ್ಯ_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_ಫಲಿತಾಂಶ_Type  = c_m3Type_i32;
        ಹಿಂತಿರುಗಿ true;
    }

    ಆದರೆ (sub < 0x10 or sub > c_wa_Op_ಕೊನೆಯ_Atomic) ಹಿಂತಿರುಗಿ false;

    ಆದರೆ (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_ಅಂಕೆ_Operands = 1;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_ಅಂಕೆ_Operands = 2;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_ಅಂಕೆ_Operands = 2;
    } ಇಲ್ಲದಿದ್ದರೆ {                        // cmpxchg
        variant = sub - 0x48;       *o_ಅಂಕೆ_Operands = 3;
    }

    *o_ಅಗಲ      = c_widths [variant];
    *o_ಮೌಲ್ಯ_Type  = c_types [variant];
    *o_ಫಲಿತಾಂಶ_Type = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    ಹಿಂತಿರುಗಿ true;
}

#ಆದರೆ_ಅಂತ್ಯ

// ---------- Operand stack ----------

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_ತಳ್ಳು (ValCtx * v, m3type_t type)
{
    ಆದರೆ (M3_UNLIKELY(v->opd_ತುದಿ >= d_m3_Val_ರಾಶಿ))
        ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
    v->opd[v->opd_ತುದಿ++] = type;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_ತೆಗೆ (ValCtx * v, m3type_t * o_type)
{
    Val_Ctrl_ಚೌಕಟ್ಟು * f = &v->ctrl[v->ctrl_ತುದಿ - 1];
    ಆದರೆ (M3_UNLIKELY(v->opd_ತುದಿ == f->ಎತ್ತರ)) {
        ಆದರೆ (f->is_unreachable) { *o_type = c_valBottom; ಹಿಂತಿರುಗಿ m3Err_none; }
        ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_Underrun;
    }
    *o_type = v->opd[--v->opd_ತುದಿ];
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_ತೆಗೆ_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3_ಫಲಿತಾಂಶ r = v_ತೆಗೆ(v, &actual);
    ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    ಆದರೆ (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

// ---------- Control stack ----------

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_ತಳ್ಳು_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    ಆದರೆ (M3_UNLIKELY(v->ctrl_ತುದಿ >= d_m3_Val_Ctrl_ಆಳ))
        ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
    Val_Ctrl_ಚೌಕಟ್ಟು * f = &v->ctrl[v->ctrl_ತುದಿ++];
    f->opcode       = op;
    f->type         = type;
    f->param_ಎಣಿಕೆ  = type ? type->ಅಂಕೆ_ವಾದಗಳು : 0;
    f->ಫಲಿತಾಂಶ_ಎಣಿಕೆ = type ? type->ಅಂಕೆ_Rets : 0;
    f->ಎತ್ತರ       = v->opd_ತುದಿ;
    f->is_unreachable = false;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_ತೆಗೆ_ctrl (ValCtx * v, Val_Ctrl_ಚೌಕಟ್ಟು * o_ಚೌಕಟ್ಟು)
{
    ಆದರೆ (M3_UNLIKELY(v->ctrl_ತುದಿ == 0))
        ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
    Val_Ctrl_ಚೌಕಟ್ಟು * f = &v->ctrl[v->ctrl_ತುದಿ - 1];
    // pop result types
    ಆದರೆ (f->type) {
        ಗಾಗಿ (u16 i = f->ಫಲಿತಾಂಶ_ಎಣಿಕೆ; i > 0; i--) {
            m3type_t a;
            M3_ಫಲಿತಾಂಶ r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(f->type->types[i - 1]), &a);
            ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
        }
    }
    ಆದರೆ (M3_UNLIKELY(v->opd_ತುದಿ != f->ಎತ್ತರ))
        ಹಿಂತಿರುಗಿ m3_Err_type_ಎಣಿಕೆ_Mismatch;
    ಆದರೆ (o_ಚೌಕಟ್ಟು) *o_ಚೌಕಟ್ಟು = *f;
    v->ctrl_ತುದಿ--;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ ಶೂನ್ಯ v_unreachable (ValCtx * v)
{
    Val_Ctrl_ಚೌಕಟ್ಟು * f = &v->ctrl[v->ctrl_ತುದಿ - 1];
    v->opd_ತುದಿ = f->ಎತ್ತರ;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ಸ್ಥಿರ u16 v_label_n (Val_Ctrl_ಚೌಕಟ್ಟು * f)
{
    ಹಿಂತಿರುಗಿ (f->opcode == 0x03) ? f->param_ಎಣಿಕೆ : f->ಫಲಿತಾಂಶ_ಎಣಿಕೆ;
}

ಸ್ಥಿರ m3type_t v_label_t (Val_Ctrl_ಚೌಕಟ್ಟು * f, u16 i)
{
    ಆದರೆ (!f->type) ಹಿಂತಿರುಗಿ c_m3Type_none;
    ಆದರೆ (f->opcode == 0x03)
        ಹಿಂತಿರುಗಿ ತಳ_Type_ದ(f->type->types[f->type->ಅಂಕೆ_Rets + i]); // params
    ಹಿಂತಿರುಗಿ ತಳ_Type_ದ(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ಸ್ಥಿರ m3type_t v_peek (ValCtx * v, u16 i_ಆಳ)
{
    Val_Ctrl_ಚೌಕಟ್ಟು * f = &v->ctrl[v->ctrl_ತುದಿ - 1];

    ಆದರೆ (v->opd_ತುದಿ <= i_ಆಳ || (u16)(v->opd_ತುದಿ - i_ಆಳ - 1) < f->ಎತ್ತರ)
        ಹಿಂತಿರುಗಿ c_valBottom;

    ಹಿಂತಿರುಗಿ v->opd[v->opd_ತುದಿ - i_ಆಳ - 1];
}

// Pop label types for branch target
ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_ತೆಗೆ_labels (ValCtx * v, Val_Ctrl_ಚೌಕಟ್ಟು * tgt)
{
    u16 n = v_label_n(tgt);
    ಗಾಗಿ (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3_ಫಲಿತಾಂಶ r = v_ತೆಗೆ_expect(v, v_label_t(tgt, i - 1), &a);
        ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    }
    ಹಿಂತಿರುಗಿ m3Err_none;
}

// Push label types back
ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_ತಳ್ಳು_labels (ValCtx * v, Val_Ctrl_ಚೌಕಟ್ಟು * tgt)
{
    u16 n = v_label_n(tgt);
    ಗಾಗಿ (u16 i = 0; i < n; i++) {
        M3_ಫಲಿತಾಂಶ r = v_ತಳ್ಳು(v, v_label_t(tgt, i));
        ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    }
    ಹಿಂತಿರುಗಿ m3Err_none;
}

// ---------- Block type resolution ----------

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    ಆದರೆ (v->wasm >= v->wasm_ಕೊನೆ)
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;

#ಆದರೆ d_m3HasTypedRefs
    ಆದರೆ (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3_ಫಲಿತಾಂಶ rr = Parse_ಮೌಲ್ಯ_Type(v->module, &refType, &v->wasm, v->wasm_ಕೊನೆ);
        ಆದರೆ (rr) ಹಿಂತಿರುಗಿ rr;
        IM3FuncType ftype = ಖಾಲಿ;
        rr = AllocFuncType(&ftype, 1);
        ಆದರೆ (rr) ಹಿಂತಿರುಗಿ rr;
        ftype->ಅಂಕೆ_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_ಕೂಡಿಸು_Func_Type(v->module->environment, &ftype);
        ಆದರೆ (rr) ಹಿಂತಿರುಗಿ rr;
        *o_type = ftype;
        ಹಿಂತಿರುಗಿ m3Err_none;
    }
#ಆದರೆ_ಅಂತ್ಯ

    i64 type;
    M3_ಫಲಿತಾಂಶ r = ReadLebSigned(&type, 33, &v->wasm, v->wasm_ಕೊನೆ);
    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

    ಆದರೆ (type < 0) {
        ಆದರೆ (type < -64) ಹಿಂತಿರುಗಿ m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ ((u32)type >= v->module->ಅಂಕೆ_Func_Types)
            ಹಿಂತಿರುಗಿ m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    ಹಿಂತಿರುಗಿ m3Err_none;
}

// ---------- Convenience ----------

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_unop (ValCtx * v, m3type_t ಒಳ, m3type_t ಹೊರಹೋಗು)
{
    m3type_t a; M3_ಫಲಿತಾಂಶ r = v_ತೆಗೆ_expect(v, ಒಳ, &a);
    ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    ಹಿಂತಿರುಗಿ v_ತಳ್ಳು(v, ಹೊರಹೋಗು);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_check_ಬಾಲ_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ಖಾಲಿ;

    u16 ಅಂಕೆ_Callee_Rets = i_calleeType ? i_calleeType->ಅಂಕೆ_Rets : 0;
    u16 ಅಂಕೆ_Func_Rets   = ft ? ft->ಅಂಕೆ_Rets : 0;

    ಆದರೆ (ಅಂಕೆ_Callee_Rets != ಅಂಕೆ_Func_Rets)
        ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Callee_Rets; i++) {
        ಆದರೆ (i_calleeType->types[i] != ft->types[i])
            ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ಫಲಿತಾಂಶ r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    ಹಿಂತಿರುಗಿ v_ತಳ್ಳು(v, t);
}

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ಫಲಿತಾಂಶ r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    r = v_ತೆಗೆ_expect(v, t, &a); ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;
    ಹಿಂತಿರುಗಿ v_ತಳ್ಳು(v, c_m3Type_i32);
}

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_testop (ValCtx * v, m3type_t t)
{
    ಹಿಂತಿರುಗಿ v_unop(v, t, c_m3Type_i32);
}

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_cvtop (ValCtx * v, m3type_t ಒಳ, m3type_t ಹೊರಹೋಗು)
{
    ಹಿಂತಿರುಗಿ v_unop(v, ಒಳ, ಹೊರಹೋಗು);
}


// ---------- Main validation loop ----------

#ಆದರೆ d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_catch_clause (ValCtx * v)
{
    u8 ಬಗೆ;
    M3_ಫಲಿತಾಂಶ r = Read_u8(&ಬಗೆ, &v->wasm, v->wasm_ಕೊನೆ);
    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
    ಆದರೆ (ಬಗೆ > 0x03) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;

    bool has_ಟ್ಯಾಗ್ = (ಬಗೆ == 0x00 || ಬಗೆ == 0x01);
    bool hasRef = (ಬಗೆ == 0x01 || ಬಗೆ == 0x03);

    IM3FuncType ಟ್ಯಾಗ್_Type = ಖಾಲಿ;
    ಆದರೆ (has_ಟ್ಯಾಗ್) {
        u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
        r = ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &v->wasm, v->wasm_ಕೊನೆ);
        ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
        ಆದರೆ (!v->module || ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= v->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಟ್ಯಾಗ್;
        ಟ್ಯಾಗ್_Type = v->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ].type;
    }

    u32 ಆಳ;
    r = ReadLEB_u32(&ಆಳ, &v->wasm, v->wasm_ಕೊನೆ);
    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
    ಆದರೆ (ಆಳ >= v->ctrl_ತುದಿ) ಹಿಂತಿರುಗಿ m3Err_unknownLabel;

    Val_Ctrl_ಚೌಕಟ್ಟು * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - ಆಳ];

    u16 ಅಂಕೆ_Payload = ಟ್ಯಾಗ್_Type ? ಟ್ಯಾಗ್_Type->ಅಂಕೆ_ವಾದಗಳು : 0;
    u16 ಅಂಕೆ_Label   = v_label_n(tgt);

    ಆದರೆ (ಅಂಕೆ_Label != ಅಂಕೆ_Payload + (hasRef ? 1u : 0u))
        ಹಿಂತಿರುಗಿ m3_Err_type_ಎಣಿಕೆ_Mismatch;

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Payload; i++) {
        ಆದರೆ (v_label_t(tgt, i) != ತಳ_Type_ದ(ಟ್ಯಾಗ್_Type->types[ಟ್ಯಾಗ್_Type->ಅಂಕೆ_Rets + i]))
            ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
    }

    ಆದರೆ (hasRef and v_label_t(tgt, ಅಂಕೆ_Payload) != c_m3Type_exnref)
        ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

    ಹಿಂತಿರುಗಿ m3Err_none;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_resume_clause (ValCtx * v, u8 ಬಗೆ, u32 ಟ್ಯಾಗ್_Idx, IM3FuncType ct)
{
    ಆದರೆ (ಬಗೆ > 0x01) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
    ಆದರೆ (!v->module || ಟ್ಯಾಗ್_Idx >= v->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಟ್ಯಾಗ್;
    IM3FuncType ಟ್ಯಾಗ್_Type = v->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_Idx].type;

    ಆದರೆ (ಬಗೆ == 0x00) {
        u32 ಆಳ;
        M3_ಫಲಿತಾಂಶ r = ReadLEB_u32(&ಆಳ, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
        ಆದರೆ (ಆಳ >= v->ctrl_ತುದಿ) ಹಿಂತಿರುಗಿ m3Err_unknownLabel;

        Val_Ctrl_ಚೌಕಟ್ಟು * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - ಆಳ];
        u16 ಅಂಕೆ_Payload = ಟ್ಯಾಗ್_Type ? ಟ್ಯಾಗ್_Type->ಅಂಕೆ_ವಾದಗಳು : 0;
        u16 ಅಂಕೆ_Label   = v_label_n(tgt);

        ಆದರೆ (ಅಂಕೆ_Label != ಅಂಕೆ_Payload + 1u) ಹಿಂತಿರುಗಿ m3_Err_type_ಎಣಿಕೆ_Mismatch;

        ಗಾಗಿ (u16 j = 0; j < ಅಂಕೆ_Payload; j++) {
            ಆದರೆ (v_label_t(tgt, j) != ತಳ_Type_ದ(ಟ್ಯಾಗ್_Type->types[ಟ್ಯಾಗ್_Type->ಅಂಕೆ_Rets + j]))
                ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
        }
        ಆದರೆ (v_label_t(tgt, ಅಂಕೆ_Payload) != c_m3Type_contref)
            ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

#ಆದರೆ d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->ಅಂಕೆ_Rets + ಅಂಕೆ_Payload]
            : tgt->type->types[ಅಂಕೆ_Payload];

        ಆದರೆ (ರಾಶಿ_Type_ದ(labelContType) == d_m3_Type_ರಾಶಿ_Abstract ||
            ರಾಶಿ_Type_ದ(labelContType) == d_m3_Type_ರಾಶಿ_None) {
            ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
        }

        u32 ರಾಶಿ_Type_2 = ರಾಶಿ_Type_ದ(labelContType);
        IM3FuncType labelCt = ಖಾಲಿ;
        ಗಾಗಿ (u32 idx = 0; idx < v->module->ಅಂಕೆ_Func_Types; ++idx) {
            ಆದರೆ (v->module->funcTypes[idx]->canonical_ಸೂಚ್ಯಂಕ == ರಾಶಿ_Type_2 &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (!labelCt) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_ಅಂಕೆ_Params = ಟ್ಯಾಗ್_Type ? ಟ್ಯಾಗ್_Type->ಅಂಕೆ_Rets : 0;
        ಆದರೆ (labelFt->ಅಂಕೆ_ವಾದಗಳು != expected_ಅಂಕೆ_Params) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
        ಗಾಗಿ (u16 p = 0; p < expected_ಅಂಕೆ_Params; ++p) {
            ಆದರೆ (labelFt->types[labelFt->ಅಂಕೆ_Rets + p] != ಟ್ಯಾಗ್_Type->types[p])
                ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
        }

        u16 expected_ಅಂಕೆ_Rets = ft ? ft->ಅಂಕೆ_Rets : 0;
        ಆದರೆ (labelFt->ಅಂಕೆ_Rets != expected_ಅಂಕೆ_Rets) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
        ಗಾಗಿ (u16 r_idx = 0; r_idx < expected_ಅಂಕೆ_Rets; ++r_idx) {
            ಆದರೆ (labelFt->types[r_idx] != ft->types[r_idx])
                ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
        }
#ಆದರೆ_ಅಂತ್ಯ
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (ಟ್ಯಾಗ್_Type && ಟ್ಯಾಗ್_Type->ಅಂಕೆ_ವಾದಗಳು != 0) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
    }
    ಹಿಂತಿರುಗಿ m3Err_none;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching

ಸ್ಥಿರ M3_ಫಲಿತಾಂಶ v_validate_body (ValCtx * v)
{
    M3_ಫಲಿತಾಂಶ r = m3Err_none;
    m3type_t a = c_valBottom;

    ಆಗ (v->wasm < v->wasm_ಕೊನೆ)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasm_ಕೊನೆ);
        ಆದರೆ (M3_UNLIKELY(r)) ಹಿಂತಿರುಗಿ r;

        ಆಯ್ಕೆ (opcode)
        {
        // ---- Control ----
        ಸ್ಥಿತಿ 0x00: // unreachable
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;

        ಸ್ಥಿತಿ 0x01: // nop
            ನಿಲ್ಲಿಸು;

        ಸ್ಥಿತಿ 0x02: //ಖಂಡ
        ಸ್ಥಿತಿ 0x03: // loop
        ಸ್ಥಿತಿ 0x04: // if
        {
            IM3FuncType bt = ಖಾಲಿ;
            r = v_read_blocktype(v, &bt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (opcode == 0x04) {
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }
            // Pop block params from caller stack
            ಆದರೆ (bt) {
                ಗಾಗಿ (u16 i = bt->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(bt->types[bt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            r = v_ತಳ್ಳು_ctrl(v, opcode, bt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            // Push params inside block
            ಆದರೆ (bt) {
                ಗಾಗಿ (u16 i = 0; i < bt->ಅಂಕೆ_ವಾದಗಳು; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(bt->types[bt->ಅಂಕೆ_Rets + i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

#ಆದರೆ d_m3HasExceptionHandling
        ಸ್ಥಿತಿ 0x1f: // try_table
        {
            IM3FuncType bt = ಖಾಲಿ;
            r = v_read_blocktype(v, &bt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (bt) {
                ಗಾಗಿ (u16 i = bt->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(bt->types[bt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 ಅಂಕೆ_Catch;
            r = ReadLEB_u32(&ಅಂಕೆ_Catch, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Catch; i++) {
                r = v_catch_clause(v);
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }

            r = v_ತಳ್ಳು_ctrl(v, opcode, bt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಆದರೆ (bt) {
                ಗಾಗಿ (u16 i = 0; i < bt->ಅಂಕೆ_ವಾದಗಳು; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(bt->types[bt->ಅಂಕೆ_Rets + i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x08: // throw
        {
            u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ;
            r = ReadLEB_u32(&ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (!v->module || ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= v->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಟ್ಯಾಗ್;
            IM3FuncType tt = v->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ].type;
            ಆದರೆ (tt) {
                ಗಾಗಿ (u16 i = tt->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(tt->types[tt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x0a: // throw_ref
            r = v_ತೆಗೆ_expect(v, c_m3Type_exnref, &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling

        ಸ್ಥಿತಿ 0x05: // else
        {
            Val_Ctrl_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;
            r = v_ತೆಗೆ_ctrl(v, &ಚೌಕಟ್ಟು);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ಚೌಕಟ್ಟು.opcode != 0x04)
                ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
            r = v_ತಳ್ಳು_ctrl(v, 0x05, ಚೌಕಟ್ಟು.type);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ಚೌಕಟ್ಟು.type) {
                ಗಾಗಿ (u16 i = 0; i < ಚೌಕಟ್ಟು.type->ಅಂಕೆ_ವಾದಗಳು; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ಚೌಕಟ್ಟು.type->types[ಚೌಕಟ್ಟು.type->ಅಂಕೆ_Rets + i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x0b: //ಕೊನೆ
        {
            Val_Ctrl_ಚೌಕಟ್ಟು ಚೌಕಟ್ಟು;
            r = v_ತೆಗೆ_ctrl(v, &ಚೌಕಟ್ಟು);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            // Push results
            ಆದರೆ (ಚೌಕಟ್ಟು.type) {
                ಗಾಗಿ (u16 i = 0; i < ಚೌಕಟ್ಟು.ಫಲಿತಾಂಶ_ಎಣಿಕೆ; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ಚೌಕಟ್ಟು.type->types[i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            // If this was the outermost frame, we're done
            ಆದರೆ (v->ctrl_ತುದಿ == 0)
                ಹಿಂತಿರುಗಿ m3Err_none;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x0c: // br
        {
            u32 ಆಳ;
            r = ReadLEB_u32(&ಆಳ, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ಆಳ >= v->ctrl_ತುದಿ) ಹಿಂತಿರುಗಿ m3Err_unknownLabel;
            Val_Ctrl_ಚೌಕಟ್ಟು * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - ಆಳ];
            r = v_ತೆಗೆ_labels(v, tgt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x0d: // br_if
        {
            u32 ಆಳ;
            r = ReadLEB_u32(&ಆಳ, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ಆಳ >= v->ctrl_ತುದಿ) ಹಿಂತಿರುಗಿ m3Err_unknownLabel;
            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            Val_Ctrl_ಚೌಕಟ್ಟು * tgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - ಆಳ];
            r = v_ತೆಗೆ_labels(v, tgt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು_labels(v, tgt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x0e: // br_table
        {
            u32 ಎಣಿಕೆ_2;
            r = ReadLEB_u32(&ಎಣಿಕೆ_2, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            u32 def_ಆಳ = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            ಬೈಟುಗಳು_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            ಗಾಗಿ (u32 i = 0; i <= ಎಣಿಕೆ_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_ಕೊನೆ);
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಆದರೆ (d >= v->ctrl_ತುದಿ) ಹಿಂತಿರುಗಿ m3Err_unknownLabel;
                ಆದರೆ (i == ಎಣಿಕೆ_2) def_ಆಳ = d;
            }
            // Now validate all labels match the default's types
            Val_Ctrl_ಚೌಕಟ್ಟು * defTgt = &v->ctrl[v->ctrl_ತುದಿ - 1 - def_ಆಳ];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಗಾಗಿ (u32 i = 0; i <= ಎಣಿಕೆ_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_ಕೊನೆ);
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                Val_Ctrl_ಚೌಕಟ್ಟು * t = &v->ctrl[v->ctrl_ತುದಿ - 1 - d];
                u16 n = v_label_n(t);
                ಆದರೆ (n != arity) ಹಿಂತಿರುಗಿ m3_Err_type_ಎಣಿಕೆ_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                ಗಾಗಿ (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    ಆದರೆ (want != c_valBottom && have != c_valBottom && want != have)
                        ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
                }
            }
            Val_Ctrl_ಚೌಕಟ್ಟು * dt = &v->ctrl[v->ctrl_ತುದಿ - 1 - def_ಆಳ];
            r = v_ತೆಗೆ_labels(v, dt);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = ft->ಅಂಕೆ_Rets; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;
        }

        // ---- Call ----
        ಸ್ಥಿತಿ 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (idx >= v->module->ಅಂಕೆ_Functions) ಹಿಂತಿರುಗಿ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = ft->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
                ಗಾಗಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ft->types[i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            u32 ಕೋಷ್ಟಕ_Idx;
            r = ReadLEB_u32(&ಕೋಷ್ಟಕ_Idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ಆದರೆ (ಕೋಷ್ಟಕ_Idx >= v->module->ಅಂಕೆ_Tables) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಕೋಷ್ಟಕ;
            ಆದರೆ (v->module->tables[ಕೋಷ್ಟಕ_Idx]->type != c_m3Type_funcref) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_ತೆಗೆ_expect(v, v_ಕೋಷ್ಟಕ_addrtype(v, ಕೋಷ್ಟಕ_Idx), &a); // table index operand
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = ft->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
                ಗಾಗಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ft->types[i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (idx >= v->module->ಅಂಕೆ_Functions) ಹಿಂತಿರುಗಿ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_ಬಾಲ_results(v, ft);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = ft->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            u32 ಕೋಷ್ಟಕ_Idx;
            r = ReadLEB_u32(&ಕೋಷ್ಟಕ_Idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ಆದರೆ (ಕೋಷ್ಟಕ_Idx >= v->module->ಅಂಕೆ_Tables) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಕೋಷ್ಟಕ;
            ಆದರೆ (v->module->tables[ಕೋಷ್ಟಕ_Idx]->type != c_m3Type_funcref) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_ಬಾಲ_results(v, ft);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತೆಗೆ_expect(v, v_ಕೋಷ್ಟಕ_addrtype(v, ಕೋಷ್ಟಕ_Idx), &a); // table index operand
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = ft->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            v_unreachable(v);
            ನಿಲ್ಲಿಸು;
        }

#ಆದರೆ d_m3HasTypedRefs
        ಸ್ಥಿತಿ 0x14: // call_ref
        ಸ್ಥಿತಿ 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool is_ಬಾಲ = (opcode == 0x15);
            ಆದರೆ (is_ಬಾಲ) {
                r = v_check_ಬಾಲ_results(v, ft);
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_ತೆಗೆ_expect(v, c_m3Type_funcref, &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = ft->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
                ಆದರೆ (not is_ಬಾಲ) {
                    ಗಾಗಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                        r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ft->types[i]));
                        ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                    }
                }
            }
            ಆದರೆ (is_ಬಾಲ) v_unreachable(v);
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xd4: // ref.as_non_null
        {
            r = v_ತೆಗೆ_expect(v, c_m3Type_funcref, &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }
#ಆದರೆ_ಅಂತ್ಯ // d_m3HasTypedRefs

        // ---- Parametric ----
        ಸ್ಥಿತಿ 0x1a: // drop
            r = v_ತೆಗೆ(v, &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;

        ಸ್ಥಿತಿ 0x1b: // select
        {
            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            m3type_t t2 = c_valBottom;
            r = v_ತೆಗೆ(v, &t2);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            m3type_t t1;
            r = v_ತೆಗೆ_expect(v, t2, &t1);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            // untyped select is numeric only; references need the 0x1c form
            ಆದರೆ (t2 != c_valBottom && IsRefType(t2)) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            r = v_ತಳ್ಳು(v, (t2 == c_valBottom) ? t1 : t2);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

#ಆದರೆ d_m3HasRefTypes
        ಸ್ಥಿತಿ 0x1c: // select with an explicit result type
        {
            u32 ಅಂಕೆ_Types;
            r = ReadLEB_u32(&ಅಂಕೆ_Types, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ಅಂಕೆ_Types != 1) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_ಮೌಲ್ಯ_Type(v->module, &selType, &v->wasm, v->wasm_ಕೊನೆ);  ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            u8 t = ತಳ_Type_ದ(selType);

            r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತೆಗೆ_expect(v, t, &a);            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತೆಗೆ_expect(v, t, &a);            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು(v, t);                      ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x25: // table.get
        ಸ್ಥಿತಿ 0x26: // table.set
        {
            u32 ಕೋಷ್ಟಕ_Idx;
            r = ReadLEB_u32(&ಕೋಷ್ಟಕ_Idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ಕೋಷ್ಟಕ_Idx >= v->module->ಅಂಕೆ_Tables) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಕೋಷ್ಟಕ;
            u8 t = ತಳ_Type_ದ(v->module->tables[ಕೋಷ್ಟಕ_Idx]->type);
            m3type_t at = v_ಕೋಷ್ಟಕ_addrtype(v, ಕೋಷ್ಟಕ_Idx);

            ಆದರೆ (opcode == 0x26) {
                r = v_ತೆಗೆ_expect(v, t, &a);   ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = v_ತೆಗೆ_expect(v, at, &a);  ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            } ಇಲ್ಲದಿದ್ದರೆ {
                r = v_ತೆಗೆ_expect(v, at, &a);  ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = v_ತಳ್ಳು(v, t);             ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xd0: // ref.null
        {
#ಆದರೆ d_m3HasTypedRefs
            m3type_t ರಾಶಿ_ಬಿಟ್ಟುಗಳು;
            r = Parse_ರಾಶಿ_Type(v->module, &ರಾಶಿ_ಬಿಟ್ಟುಗಳು, &v->wasm, v->wasm_ಕೊನೆ);  ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            u8 t = (ರಾಶಿ_ಬಿಟ್ಟುಗಳು & d_m3Type_refExtern) ? c_m3Type_externref :
# ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
                   (ರಾಶಿ_ಬಿಟ್ಟುಗಳು & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#ಇಲ್ಲದಿದ್ದರೆ
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasm_ಕೊನೆ);   ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = NormalizeType(&t, waType);                   ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (!IsRefType(t)) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
#ಆದರೆ_ಅಂತ್ಯ
            r = v_ತಳ್ಳು(v, t);                                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_ತೆಗೆ(v, &t);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (t != c_valBottom && !IsRefType(t)) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            r = v_ತಳ್ಳು(v, c_m3Type_i32);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (funcIdx >= v->module->ಅಂಕೆ_Functions) ಹಿಂತಿರುಗಿ m3Err_unknownFunction;
            ಆದರೆ (!Module_IsFunctionDeclared(v->module, funcIdx)) ಹಿಂತಿರುಗಿ m3Err_undeclaredFuncRef;
            r = v_ತಳ್ಳು(v, c_m3Type_funcref);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }
#ಆದರೆ_ಅಂತ್ಯ

        // ---- Variable ----
        ಸ್ಥಿತಿ 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (idx >= v->ಅಂಕೆ_Locals) ಹಿಂತಿರುಗಿ m3Err_unknownLocal;
            r = v_ತಳ್ಳು(v, v->localTypes[idx]);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (idx >= v->ಅಂಕೆ_Locals) ಹಿಂತಿರುಗಿ m3Err_unknownLocal;
            r = v_ತೆಗೆ_expect(v, v->localTypes[idx], &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (idx >= v->ಅಂಕೆ_Locals) ಹಿಂತಿರುಗಿ m3Err_unknownLocal;
            r = v_ತೆಗೆ_expect(v, v->localTypes[idx], &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು(v, v->localTypes[idx]);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (idx >= v->module->ಅಂಕೆ_Globals) ಹಿಂತಿರುಗಿ m3Err_unknownGlobal;
            r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(v->module->globals[idx].type));
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (idx >= v->module->ಅಂಕೆ_Globals) ಹಿಂತಿರುಗಿ m3Err_unknownGlobal;
            r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(v->module->globals[idx].type), &a);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        // ---- Memory load ----
        ಸ್ಥಿತಿ 0x28: ಸ್ಥಿತಿ 0x29: ಸ್ಥಿತಿ 0x2a: ಸ್ಥಿತಿ 0x2b: // i32/i64/f32/f64.load
        ಸ್ಥಿತಿ 0x2c: ಸ್ಥಿತಿ 0x2d: ಸ್ಥಿತಿ 0x2e: ಸ್ಥಿತಿ 0x2f: // i32.load8/16 s/u
        ಸ್ಥಿತಿ 0x30: ಸ್ಥಿತಿ 0x31: ಸ್ಥಿತಿ 0x32: ಸ್ಥಿತಿ 0x33: // i64.load8/16 s/u
        ಸ್ಥಿತಿ 0x34: ಸ್ಥಿತಿ 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = Read_ಸ್ಮರಣೆ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (align > v_max_align(opcode)) ಹಿಂತಿರುಗಿ m3Err_invalidAlignment;
            ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, memidx)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
            ಆದರೆ (not v_ಸ್ಥಾನಾಂತರ_ಒಳ_range(v, memidx, offset)) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
            r = v_ತೆಗೆ_expect(v, v_ಸ್ಮರಣೆ_addrtype(v, memidx), &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            u8 ಫಲಿತಾಂಶ;
            ಆದರೆ      (opcode == 0x28) ಫಲಿತಾಂಶ = c_m3Type_i32;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == 0x29) ಫಲಿತಾಂಶ = c_m3Type_i64;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == 0x2a) ಫಲಿತಾಂಶ = c_m3Type_f32;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == 0x2b) ಫಲಿತಾಂಶ = c_m3Type_f64;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode <= 0x2f) ಫಲಿತಾಂಶ = c_m3Type_i32;
            ಇಲ್ಲದಿದ್ದರೆ                     ಫಲಿತಾಂಶ = c_m3Type_i64;
            r = v_ತಳ್ಳು(v, ಫಲಿತಾಂಶ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        // ---- Memory store ----
        ಸ್ಥಿತಿ 0x36: ಸ್ಥಿತಿ 0x37: ಸ್ಥಿತಿ 0x38: ಸ್ಥಿತಿ 0x39: // i32/i64/f32/f64.store
        ಸ್ಥಿತಿ 0x3a: ಸ್ಥಿತಿ 0x3b:                         // i32.store8/16
        ಸ್ಥಿತಿ 0x3c: ಸ್ಥಿತಿ 0x3d: ಸ್ಥಿತಿ 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = Read_ಸ್ಮರಣೆ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (align > v_max_align(opcode)) ಹಿಂತಿರುಗಿ m3Err_invalidAlignment;
            ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, memidx)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
            ಆದರೆ (not v_ಸ್ಥಾನಾಂತರ_ಒಳ_range(v, memidx, offset)) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
            u8 valtype;
            ಆದರೆ      (opcode == 0x36) valtype = c_m3Type_i32;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == 0x37) valtype = c_m3Type_i64;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == 0x38) valtype = c_m3Type_f32;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode == 0x39) valtype = c_m3Type_f64;
            ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (opcode <= 0x3b) valtype = c_m3Type_i32;
            ಇಲ್ಲದಿದ್ದರೆ                     valtype = c_m3Type_i64;
            r = v_ತೆಗೆ_expect(v, valtype, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತೆಗೆ_expect(v, v_ಸ್ಮರಣೆ_addrtype(v, memidx), &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        // ---- Memory size/grow ----
        ಸ್ಥಿತಿ 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, memidx)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
            r = v_ತಳ್ಳು(v, v_ಸ್ಮರಣೆ_addrtype(v, memidx)); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, memidx)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
            r = v_ತೆಗೆ_expect(v, v_ಸ್ಮರಣೆ_addrtype(v, memidx), &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು(v, v_ಸ್ಮರಣೆ_addrtype(v, memidx)); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        // ---- Constants ----
        ಸ್ಥಿತಿ 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು(v, c_m3Type_i32); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು(v, c_m3Type_i64); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 0x43: { // f32.const
            ಆದರೆ (v->wasm + 4 > v->wasm_ಕೊನೆ) ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ತಳ್ಳು(v, c_m3Type_f32); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }
        ಸ್ಥಿತಿ 0x44: { // f64.const
            ಆದರೆ (v->wasm + 8 > v->wasm_ಕೊನೆ) ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ತಳ್ಳು(v, c_m3Type_f64); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }


        // ---- i32 comparison ----
        ಸ್ಥಿತಿ 0x45: r = v_testop(v, c_m3Type_i32); ನಿಲ್ಲಿಸು; // i32.eqz
        ಸ್ಥಿತಿ 0x46: ಸ್ಥಿತಿ 0x47: ಸ್ಥಿತಿ 0x48: ಸ್ಥಿತಿ 0x49: ಸ್ಥಿತಿ 0x4a:
        ಸ್ಥಿತಿ 0x4b: ಸ್ಥಿತಿ 0x4c: ಸ್ಥಿತಿ 0x4d: ಸ್ಥಿತಿ 0x4e: ಸ್ಥಿತಿ 0x4f:
            r = v_relop(v, c_m3Type_i32); ನಿಲ್ಲಿಸು;

        // ---- i64 comparison ----
        ಸ್ಥಿತಿ 0x50: r = v_testop(v, c_m3Type_i64); ನಿಲ್ಲಿಸು; // i64.eqz
        ಸ್ಥಿತಿ 0x51: ಸ್ಥಿತಿ 0x52: ಸ್ಥಿತಿ 0x53: ಸ್ಥಿತಿ 0x54: ಸ್ಥಿತಿ 0x55:
        ಸ್ಥಿತಿ 0x56: ಸ್ಥಿತಿ 0x57: ಸ್ಥಿತಿ 0x58: ಸ್ಥಿತಿ 0x59: ಸ್ಥಿತಿ 0x5a:
            r = v_relop(v, c_m3Type_i64); ನಿಲ್ಲಿಸು;

        // ---- f32 comparison ----
        ಸ್ಥಿತಿ 0x5b: ಸ್ಥಿತಿ 0x5c: ಸ್ಥಿತಿ 0x5d: ಸ್ಥಿತಿ 0x5e: ಸ್ಥಿತಿ 0x5f: ಸ್ಥಿತಿ 0x60:
            r = v_relop(v, c_m3Type_f32); ನಿಲ್ಲಿಸು;

        // ---- f64 comparison ----
        ಸ್ಥಿತಿ 0x61: ಸ್ಥಿತಿ 0x62: ಸ್ಥಿತಿ 0x63: ಸ್ಥಿತಿ 0x64: ಸ್ಥಿತಿ 0x65: ಸ್ಥಿತಿ 0x66:
            r = v_relop(v, c_m3Type_f64); ನಿಲ್ಲಿಸು;

        // ---- i32 unary ----
        ಸ್ಥಿತಿ 0x67: ಸ್ಥಿತಿ 0x68: ಸ್ಥಿತಿ 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ನಿಲ್ಲಿಸು;

        // ---- i32 binary ----
        ಸ್ಥಿತಿ 0x6a: ಸ್ಥಿತಿ 0x6b: ಸ್ಥಿತಿ 0x6c: ಸ್ಥಿತಿ 0x6d: ಸ್ಥಿತಿ 0x6e: ಸ್ಥಿತಿ 0x6f:
        ಸ್ಥಿತಿ 0x70: ಸ್ಥಿತಿ 0x71: ಸ್ಥಿತಿ 0x72: ಸ್ಥಿತಿ 0x73: ಸ್ಥಿತಿ 0x74: ಸ್ಥಿತಿ 0x75:
        ಸ್ಥಿತಿ 0x76: ಸ್ಥಿತಿ 0x77: ಸ್ಥಿತಿ 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); ನಿಲ್ಲಿಸು;

        // ---- i64 unary ----
        ಸ್ಥಿತಿ 0x79: ಸ್ಥಿತಿ 0x7a: ಸ್ಥಿತಿ 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ನಿಲ್ಲಿಸು;

        // ---- i64 binary ----
        ಸ್ಥಿತಿ 0x7c: ಸ್ಥಿತಿ 0x7d: ಸ್ಥಿತಿ 0x7e: ಸ್ಥಿತಿ 0x7f: ಸ್ಥಿತಿ 0x80: ಸ್ಥಿತಿ 0x81:
        ಸ್ಥಿತಿ 0x82: ಸ್ಥಿತಿ 0x83: ಸ್ಥಿತಿ 0x84: ಸ್ಥಿತಿ 0x85: ಸ್ಥಿತಿ 0x86: ಸ್ಥಿತಿ 0x87:
        ಸ್ಥಿತಿ 0x88: ಸ್ಥಿತಿ 0x89: ಸ್ಥಿತಿ 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); ನಿಲ್ಲಿಸು;

        // ---- f32 unary ----
        ಸ್ಥಿತಿ 0x8b: ಸ್ಥಿತಿ 0x8c: ಸ್ಥಿತಿ 0x8d: ಸ್ಥಿತಿ 0x8e: ಸ್ಥಿತಿ 0x8f:
        ಸ್ಥಿತಿ 0x90: ಸ್ಥಿತಿ 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); ನಿಲ್ಲಿಸು;

        // ---- f32 binary ----
        ಸ್ಥಿತಿ 0x92: ಸ್ಥಿತಿ 0x93: ಸ್ಥಿತಿ 0x94: ಸ್ಥಿತಿ 0x95: ಸ್ಥಿತಿ 0x96:
        ಸ್ಥಿತಿ 0x97: ಸ್ಥಿತಿ 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); ನಿಲ್ಲಿಸು;

        // ---- f64 unary ----
        ಸ್ಥಿತಿ 0x99: ಸ್ಥಿತಿ 0x9a: ಸ್ಥಿತಿ 0x9b: ಸ್ಥಿತಿ 0x9c: ಸ್ಥಿತಿ 0x9d:
        ಸ್ಥಿತಿ 0x9e: ಸ್ಥಿತಿ 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); ನಿಲ್ಲಿಸು;

        // ---- f64 binary ----
        ಸ್ಥಿತಿ 0xa0: ಸ್ಥಿತಿ 0xa1: ಸ್ಥಿತಿ 0xa2: ಸ್ಥಿತಿ 0xa3: ಸ್ಥಿತಿ 0xa4:
        ಸ್ಥಿತಿ 0xa5: ಸ್ಥಿತಿ 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); ನಿಲ್ಲಿಸು;

        // ---- Conversions ----
        ಸ್ಥಿತಿ 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); ನಿಲ್ಲಿಸು; // i32.wrap/i64
        ಸ್ಥಿತಿ 0xa8: ಸ್ಥಿತಿ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xaa: ಸ್ಥಿತಿ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xac: ಸ್ಥಿತಿ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xae: ಸ್ಥಿತಿ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xb0: ಸ್ಥಿತಿ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xb2: ಸ್ಥಿತಿ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xb4: ಸ್ಥಿತಿ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xb7: ಸ್ಥಿತಿ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xb9: ಸ್ಥಿತಿ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ನಿಲ್ಲಿಸು;

        // ---- Sign-extension (MVP post) ----
        ಸ್ಥಿತಿ 0xc0: ಸ್ಥಿತಿ 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 0xc2: ಸ್ಥಿತಿ 0xc3: ಸ್ಥಿತಿ 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ನಿಲ್ಲಿಸು;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        ಸ್ಥಿತಿ 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆಯ್ಕೆ (sub) {
            ಸ್ಥಿತಿ 0x00: ಸ್ಥಿತಿ 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 0x02: ಸ್ಥಿತಿ 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 0x04: ಸ್ಥಿತಿ 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 0x06: ಸ್ಥಿತಿ 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ನಿಲ್ಲಿಸು;
            ಸ್ಥಿತಿ 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, memidx)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
                // the segments must have been declared up front by a data count section
                ಆದರೆ (not v->module->has_ದತ್ತಾಂಶ_ಎಣಿಕೆ) ಹಿಂತಿರುಗಿ m3_Err_ದತ್ತಾಂಶ_ಎಣಿಕೆ_Required;
                ಆದರೆ (dataidx >= v->module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments) ಹಿಂತಿರುಗಿ m3_Err_unknown_ದತ್ತಾಂಶ_Segment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r; // n
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r; // src
                r = v_ತೆಗೆ_expect(v, v_ಸ್ಮರಣೆ_addrtype(v, memidx), &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r; // dst
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                // needs the segment, but not a memory
                ಆದರೆ (not v->module->has_ದತ್ತಾಂಶ_ಎಣಿಕೆ) ಹಿಂತಿರುಗಿ m3_Err_ದತ್ತಾಂಶ_ಎಣಿಕೆ_Required;
                ಆದರೆ (dataidx >= v->module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments) ಹಿಂತಿರುಗಿ m3_Err_unknown_ದತ್ತಾಂಶ_Segment;
                ನಿಲ್ಲಿಸು;
            }
#ಆದರೆ d_m3HasRefTypes
            ಸ್ಥಿತಿ 0x0c: // table.init
            ಸ್ಥಿತಿ 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

                ಆದರೆ (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    ಆದರೆ (a2 >= v->module->ಅಂಕೆ_Tables) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಕೋಷ್ಟಕ;
                    ಆದರೆ (a1 >= v->module->ಅಂಕೆ_Element_Segments) ಹಿಂತಿರುಗಿ m3Err_unknownElemSegment;
                    ಆದರೆ (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
                } ಇಲ್ಲದಿದ್ದರೆ {                                                    // dst, src
                    ಆದರೆ (a1 >= v->module->ಅಂಕೆ_Tables) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಕೋಷ್ಟಕ;
                    ಆದರೆ (a2 >= v->module->ಅಂಕೆ_Tables) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಕೋಷ್ಟಕ;
                    ಆದರೆ (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
                }

                ಆದರೆ (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;             // n
                    r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;             // s
                    r = v_ತೆಗೆ_expect(v, v_ಕೋಷ್ಟಕ_addrtype(v, a2), &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // d
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_ಕೋಷ್ಟಕ_addrtype(v, a1);
                    m3type_t sType = v_ಕೋಷ್ಟಕ_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_ತೆಗೆ_expect(v, nType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // n
                    r = v_ತೆಗೆ_expect(v, sType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // s
                    r = v_ತೆಗೆ_expect(v, dType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // d
                }
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಆದರೆ (elemIdx >= v->module->ಅಂಕೆ_Element_Segments) ಹಿಂತಿರುಗಿ m3Err_unknownElemSegment;
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ 0x0f: // table.grow
            ಸ್ಥಿತಿ 0x10: // table.size
            ಸ್ಥಿತಿ 0x11: // table.fill
            {
                u32 ಕೋಷ್ಟಕ_Idx;
                r = ReadLEB_u32(&ಕೋಷ್ಟಕ_Idx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಆದರೆ (ಕೋಷ್ಟಕ_Idx >= v->module->ಅಂಕೆ_Tables) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಕೋಷ್ಟಕ;
                u8 t = ತಳ_Type_ದ(v->module->tables[ಕೋಷ್ಟಕ_Idx]->type);
                m3type_t at = v_ಕೋಷ್ಟಕ_addrtype(v, ಕೋಷ್ಟಕ_Idx);

                ಆದರೆ (sub == 0x10) {                                          // table.size
                    r = v_ತಳ್ಳು(v, at); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (sub == 0x0f) {                                   // table.grow
                    r = v_ತೆಗೆ_expect(v, at, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // n
                    r = v_ತೆಗೆ_expect(v, t, &a);  ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // init
                    r = v_ತಳ್ಳು(v, at);           ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                } ಇಲ್ಲದಿದ್ದರೆ {                                                    // table.fill
                    r = v_ತೆಗೆ_expect(v, at, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // n
                    r = v_ತೆಗೆ_expect(v, t, &a);  ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // val
                    r = v_ತೆಗೆ_expect(v, at, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // i
                }
                ನಿಲ್ಲಿಸು;
            }
#ಆದರೆ_ಅಂತ್ಯ
            ಸ್ಥಿತಿ 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, dst) or not v_has_ಸ್ಮರಣೆ_idx(v, src)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_ಸ್ಮರಣೆ_addrtype(v, dst);
                m3type_t srcType = v_ಸ್ಮರಣೆ_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_ತೆಗೆ_expect(v, lenType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r; // n
                r = v_ತೆಗೆ_expect(v, srcType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r; // src
                r = v_ತೆಗೆ_expect(v, dstType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r; // dst
                ನಿಲ್ಲಿಸು;
            }
            ಸ್ಥಿತಿ 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, memidx)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
                m3type_t addrType = v_ಸ್ಮರಣೆ_addrtype(v, memidx);
                r = v_ತೆಗೆ_expect(v, addrType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;      // n
                r = v_ತೆಗೆ_expect(v, c_m3Type_i32, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;  // val
                r = v_ತೆಗೆ_expect(v, addrType, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;      // dst
                ನಿಲ್ಲಿಸು;
            }
#ಆದರೆ d_m3HasWideArithmetic
            ಸ್ಥಿತಿ 0x13: ಸ್ಥಿತಿ 0x14: // i64.add128, i64.sub128
            ಸ್ಥಿತಿ 0x15: ಸ್ಥಿತಿ 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 ಅಂಕೆ_Operands = (sub <= 0x14) ? 4 : 2;
                ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Operands; ++i) {
                    r = v_ತೆಗೆ_expect(v, c_m3Type_i64, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
                r = v_ತಳ್ಳು(v, c_m3Type_i64); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = v_ತಳ್ಳು(v, c_m3Type_i64); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ನಿಲ್ಲಿಸು;
            }
#ಆದರೆ_ಅಂತ್ಯ
            ಪೂರ್ವನಿಯೋಜಿತ:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                ನಿಲ್ಲಿಸು;
            }
            ನಿಲ್ಲಿಸು;
        }

#ಆದರೆ d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        ಸ್ಥಿತಿ 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_ಕೊನೆ);
            ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಆದರೆ (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಆದರೆ (reserved != 0) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
                ನಿಲ್ಲಿಸು;
            }

            u32 ಅಗಲ_2, ಅಂಕೆ_Operands; m3type_t ಮೌಲ್ಯ_Type, ಫಲಿತಾಂಶ_Type;
            ಆದರೆ (not v_atomic_shape(sub, &ಅಗಲ_2, &ಅಂಕೆ_Operands, &ಮೌಲ್ಯ_Type, &ಫಲಿತಾಂಶ_Type)) ಹಿಂತಿರುಗಿ m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = Read_ಸ್ಮರಣೆ_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            ಆದರೆ ((1u << align) != ಅಗಲ_2) ಹಿಂತಿರುಗಿ m3Err_invalidAtomicAlignment;
            ಆದರೆ (not v_has_ಸ್ಮರಣೆ_idx(v, memidx)) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಸ್ಮರಣೆ;
            ಆದರೆ (not v_ಸ್ಥಾನಾಂತರ_ಒಳ_range(v, memidx, offset)) ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            ಆದರೆ (sub == 0x01 or sub == 0x02) {
                r = v_ತೆಗೆ_expect(v, c_m3Type_i64, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                ಅಂಕೆ_Operands--;
            }
            ಗಾಗಿ (u32 i = 1; i < ಅಂಕೆ_Operands; ++i) {
                r = v_ತೆಗೆ_expect(v, ಮೌಲ್ಯ_Type, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }
            r = v_ತೆಗೆ_expect(v, v_ಸ್ಮರಣೆ_addrtype(v, memidx), &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (ಫಲಿತಾಂಶ_Type != c_m3Type_none) {
                r = v_ತಳ್ಳು(v, ಫಲಿತಾಂಶ_Type); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }
            ನಿಲ್ಲಿಸು;
        }
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
        // ---- Stack Switching ----
        ಸ್ಥಿತಿ 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            ಆದರೆ (!ft || !ft->isContinuation) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            r = v_ತೆಗೆ_expect(v, c_m3Type_funcref, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತಳ್ಳು(v, c_m3Type_contref); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (typeIdx1 >= v->module->ಅಂಕೆ_Func_Types || typeIdx2 >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            ಆದರೆ (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 ಅಂಕೆ_Params1 = ft1->ಅಂಕೆ_ವಾದಗಳು;
            u16 ಅಂಕೆ_Params2 = ft2->ಅಂಕೆ_ವಾದಗಳು;
            ಆದರೆ (ಅಂಕೆ_Params1 < ಅಂಕೆ_Params2) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            u32 ಅಂಕೆ_Bound = ಅಂಕೆ_Params1 - ಅಂಕೆ_Params2;

            ಆದರೆ (ft1->ಅಂಕೆ_Rets != ft2->ಅಂಕೆ_Rets) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            ಗಾಗಿ (u16 i = 0; i < ft1->ಅಂಕೆ_Rets; ++i) {
                ಆದರೆ (ft1->types[i] != ft2->types[i]) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            }
            ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params2; ++i) {
                ಆದರೆ (ft1->types[ft1->ಅಂಕೆ_Rets + ಅಂಕೆ_Bound + i] != ft2->types[ft2->ಅಂಕೆ_Rets + i]) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಗಾಗಿ (u32 i = ಅಂಕೆ_Bound; i > 0; i--) {
                r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft1->types[ft1->ಅಂಕೆ_Rets + i - 1]), &a);
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }

            r = v_ತಳ್ಳು(v, c_m3Type_contref); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xe2: // suspend
        {
            u32 ಟ್ಯಾಗ್_Idx;
            r = ReadLEB_u32(&ಟ್ಯಾಗ್_Idx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (!v->module || ಟ್ಯಾಗ್_Idx >= v->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಟ್ಯಾಗ್;
            IM3FuncType tt = v->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_Idx].type;
            ಆದರೆ (tt) {
                ಗಾಗಿ (u16 i = tt->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(tt->types[tt->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
                ಗಾಗಿ (u16 i = 0; i < tt->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(tt->types[i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಆದರೆ (!ct || !ct->isContinuation) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            u32 ಅಂಕೆ_Clauses;
            r = ReadLEB_u32(&ಅಂಕೆ_Clauses, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; i++) {
                u8 ಬಗೆ;
                r = Read_u8(&ಬಗೆ, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                u32 ಟ್ಯಾಗ್_Idx;
                r = ReadLEB_u32(&ಟ್ಯಾಗ್_Idx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = v_resume_clause(v, ಬಗೆ, ಟ್ಯಾಗ್_Idx, ct); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = ft->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
                ಗಾಗಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ft->types[i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xe4: // resume_throw
        {
            u32 typeIdx, ಟ್ಯಾಗ್_Idx, ಅಂಕೆ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = ReadLEB_u32(&ಟ್ಯಾಗ್_Idx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = ReadLEB_u32(&ಅಂಕೆ_Clauses, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            ಆದರೆ (!v->module || ಟ್ಯಾಗ್_Idx >= v->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಟ್ಯಾಗ್;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಆದರೆ (!ct || !ct->isContinuation) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;
            IM3FuncType ಟ್ಯಾಗ್_Type = v->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_Idx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            ಆದರೆ (ಟ್ಯಾಗ್_Type && ಟ್ಯಾಗ್_Type->ಅಂಕೆ_Rets != 0) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; i++) {
                u8 ಬಗೆ;
                r = Read_u8(&ಬಗೆ, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                u32 c_ಟ್ಯಾಗ್_Idx;
                r = ReadLEB_u32(&c_ಟ್ಯಾಗ್_Idx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = v_resume_clause(v, ಬಗೆ, c_ಟ್ಯಾಗ್_Idx, ct); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಆದರೆ (ಟ್ಯಾಗ್_Type) {
                ಗಾಗಿ (u16 i = ಟ್ಯಾಗ್_Type->ಅಂಕೆ_ವಾದಗಳು; i > 0; i--) {
                    r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ಟ್ಯಾಗ್_Type->types[ಟ್ಯಾಗ್_Type->ಅಂಕೆ_Rets + i - 1]), &a);
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ft->types[i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xe5: // resume_throw_ref
        {
            u32 typeIdx, ಅಂಕೆ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = ReadLEB_u32(&ಅಂಕೆ_Clauses, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಆದರೆ (!ct || !ct->isContinuation) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Clauses; i++) {
                u8 ಬಗೆ;
                r = Read_u8(&ಬಗೆ, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                u32 c_ಟ್ಯಾಗ್_Idx;
                r = ReadLEB_u32(&c_ಟ್ಯಾಗ್_Idx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                r = v_resume_clause(v, ಬಗೆ, c_ಟ್ಯಾಗ್_Idx, ct); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = v_ತೆಗೆ_expect(v, c_m3Type_exnref, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ಆದರೆ (ft) {
                ಗಾಗಿ (u16 i = 0; i < ft->ಅಂಕೆ_Rets; i++) {
                    r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(ft->types[i]));
                    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
                }
            }
            ನಿಲ್ಲಿಸು;
        }

        ಸ್ಥಿತಿ 0xe6: // switch
        {
            u32 typeIdx, ಟ್ಯಾಗ್_Idx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            r = ReadLEB_u32(&ಟ್ಯಾಗ್_Idx, &v->wasm, v->wasm_ಕೊನೆ); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಆದರೆ (typeIdx >= v->module->ಅಂಕೆ_Func_Types) ಹಿಂತಿರುಗಿ m3Err_unknownType;
            ಆದರೆ (!v->module || ಟ್ಯಾಗ್_Idx >= v->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು) ಹಿಂತಿರುಗಿ m3_Err_unknown_ಟ್ಯಾಗ್;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ಆದರೆ (!ct || !ct->isContinuation) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType ಟ್ಯಾಗ್_Type = v->module->ಟ್ಯಾಗುಗಳು[ಟ್ಯಾಗ್_Idx].type;
            ಆದರೆ (ಟ್ಯಾಗ್_Type && ಟ್ಯಾಗ್_Type->ಅಂಕೆ_ವಾದಗಳು != 0) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            ಆದರೆ (!ft || ft->ಅಂಕೆ_ವಾದಗಳು == 0) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            IM3FuncType peer = Module_Cont_Type_ದ_Ref(v->module, ft->types[ft->ಅಂಕೆ_Rets + ft->ಅಂಕೆ_ವಾದಗಳು - 1]);
            ಆದರೆ (!peer || !peer->contFuncType) ಹಿಂತಿರುಗಿ m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_ತೆಗೆ_expect(v, c_m3Type_contref, &a); ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

            ಗಾಗಿ (u16 i = (u16)(ft->ಅಂಕೆ_ವಾದಗಳು - 1); i > 0; i--) {
                r = v_ತೆಗೆ_expect(v, ತಳ_Type_ದ(ft->types[ft->ಅಂಕೆ_Rets + i - 1]), &a);
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }

            ಗಾಗಿ (u16 i = 0; i < self->ಅಂಕೆ_ವಾದಗಳು; i++) {
                r = v_ತಳ್ಳು(v, ತಳ_Type_ದ(self->types[self->ಅಂಕೆ_Rets + i]));
                ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
            }
            ನಿಲ್ಲಿಸು;
        }

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching

        ಪೂರ್ವನಿಯೋಜಿತ:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            ನಿಲ್ಲಿಸು;

        } // switch

        ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

    } // while

    // If we ran out of bytes without hitting the final end
    ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3_ಫಲಿತಾಂಶ  ValidateFunction  (IM3Function i_function)
{
    ಆದರೆ (!i_function->wasm) ಹಿಂತಿರುಗಿ m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    ಆದರೆ (!v) {
        v = m3_AllocStruct (ValCtx);
        ಆದರೆ (!v) ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, ಗಾತ್ರ(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasm_ಕೊನೆ  = i_function->wasm_ಕೊನೆ;

    // Skip code size LEB
    u32 ಗಾತ್ರ_2;
    M3_ಫಲಿತಾಂಶ r = ReadLEB_u32(&ಗಾತ್ರ_2, &v->wasm, v->wasm_ಕೊನೆ);
    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

    // Parse locals
    u32 ಅಂಕೆ_Local_ಖಂಡಗಳು;
    r = ReadLEB_u32(&ಅಂಕೆ_Local_ಖಂಡಗಳು, &v->wasm, v->wasm_ಕೊನೆ);
    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 ಅಂಕೆ_Params = funcType ? funcType->ಅಂಕೆ_ವಾದಗಳು : 0;
    ಆದರೆ (ಅಂಕೆ_Params > d_m3_Val_ರಾಶಿ) ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Params; i++) {
        v->localTypes[v->ಅಂಕೆ_Locals++] = ತಳ_Type_ದ(funcType->types[funcType->ಅಂಕೆ_Rets + i]);
    }

    // Then: declared locals
    ಗಾಗಿ (u32 b = 0; b < ಅಂಕೆ_Local_ಖಂಡಗಳು; b++) {
        u32 ಎಣಿಕೆ_2;
        r = ReadLEB_u32(&ಎಣಿಕೆ_2, &v->wasm, v->wasm_ಕೊನೆ);
        ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
        m3type_t localType;
        r = Parse_ಮೌಲ್ಯ_Type(v->module, &localType, &v->wasm, v->wasm_ಕೊನೆ);
        ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;
        u8 normalized = ತಳ_Type_ದ(localType);
        ಆದರೆ (ಎಣಿಕೆ_2 > (u32) (d_m3_Val_ರಾಶಿ - v->ಅಂಕೆ_Locals)) ಹಿಂತಿರುಗಿ m3_Err_function_ರಾಶಿ_ಅತಿಪ್ರವಾಹ;
        ಗಾಗಿ (u32 c = 0; c < ಎಣಿಕೆ_2; c++) {
            v->localTypes[v->ಅಂಕೆ_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ತಳ್ಳು_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    ಆದರೆ (r) ಹಿಂತಿರುಗಿ r;

    // After validation, control stack should be empty and all declared bytes consumed
    ಆದರೆ (v->ctrl_ತುದಿ != 0)
        ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;

    ಆದರೆ (v->wasm != v->wasm_ಕೊನೆ)
        ಹಿಂತಿರುಗಿ m3Err_wasmSectionUnderrun;

    ಹಿಂತಿರುಗಿ m3Err_none;
}

#ಇಲ್ಲದಿದ್ದರೆ // !d_m3EnableValidation

M3_ಫಲಿತಾಂಶ  ValidateFunction  (IM3Function i_function)
{
    (ಶೂನ್ಯ)i_function;
    ಹಿಂತಿರುಗಿ m3Err_none;
}

// clang-format on

#ಆದರೆ_ಅಂತ್ಯ // d_m3EnableValidation
