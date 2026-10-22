//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#അന്തര്ഭാവ "m3_validate.h"
#അന്തര്ഭാവ "m3_exception.h"
#അന്തര്ഭാവ "m3_info.h"

#എങ്കിൽ d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

തരനിർവചനം ഘടന {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_എണ്ണം;
    u16         result_എണ്ണം;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

ഘടന ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_അട്ടി];
    u16         opd_മുകൾ;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_മുകൾ;

    m3type_t    localTypes [d_m3_Val_അട്ടി];
    u16         അക്കം_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
സ്ഥിതം bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    തിരികെ v->module and i_memoryIdx < v->module->അക്കം_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
സ്ഥിതം m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    തിരികെ Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
സ്ഥിതം bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    തിരികെ v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
സ്ഥിതം m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    തിരികെ Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
സ്ഥിതം u32 v_max_align (m3opcode_t opcode)
{
    തിരഞ്ഞെടുപ്പ് (opcode) {
    അവസ്ഥ 0x2c: അവസ്ഥ 0x2d:   // i32.load8_s, i32.load8_u
    അവസ്ഥ 0x30: അവസ്ഥ 0x31:   // i64.load8_s, i64.load8_u
    അവസ്ഥ 0x3a:              // i32.store8
    അവസ്ഥ 0x3c:              // i64.store8
        തിരികെ 0;
    അവസ്ഥ 0x2e: അവസ്ഥ 0x2f:   // i32.load16_s, i32.load16_u
    അവസ്ഥ 0x32: അവസ്ഥ 0x33:   // i64.load16_s, i64.load16_u
    അവസ്ഥ 0x3b:              // i32.store16
    അവസ്ഥ 0x3d:              // i64.store16
        തിരികെ 1;
    അവസ്ഥ 0x29:              // i64.load
    അവസ്ഥ 0x2b:              // f64.load
    അവസ്ഥ 0x37:              // i64.store
    അവസ്ഥ 0x39:              // f64.store
        തിരികെ 3;
    സ്ഥിരസ്ഥിതി:                // 32-bit accesses, and a safe fallback
        തിരികെ 2;
    }
}

#എങ്കിൽ d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
സ്ഥിതം bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_അക്കം_Operands, m3type_t * o_മൂല്യം_Type, m3type_t * o_resultType)
{
    സ്ഥിതം സ്ഥിരം u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    സ്ഥിതം സ്ഥിരം m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    എങ്കിൽ (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_അക്കം_Operands = (sub == 0x00) ? 2 : 3;
        *o_മൂല്യം_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        തിരികെ true;
    }

    എങ്കിൽ (sub < 0x10 or sub > c_waOp_lastAtomic) തിരികെ false;

    എങ്കിൽ (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_അക്കം_Operands = 1;
    } അല്ലെങ്കിൽ എങ്കിൽ (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_അക്കം_Operands = 2;
    } അല്ലെങ്കിൽ എങ്കിൽ (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_അക്കം_Operands = 2;
    } അല്ലെങ്കിൽ {                        // cmpxchg
        variant = sub - 0x48;       *o_അക്കം_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_മൂല്യം_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    തിരികെ true;
}

#endif

// ---------- Operand stack ----------

സ്ഥിതം M3Result v_തള്ളുക (ValCtx * v, m3type_t type)
{
    എങ്കിൽ (M3_UNLIKELY(v->opd_മുകൾ >= d_m3_Val_അട്ടി))
        തിരികെ m3_Err_function_അട്ടി_Overflow;
    v->opd[v->opd_മുകൾ++] = type;
    തിരികെ m3Err_none;
}

സ്ഥിതം M3Result v_എടുക്കുക (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_മുകൾ - 1];
    എങ്കിൽ (M3_UNLIKELY(v->opd_മുകൾ == f->height)) {
        എങ്കിൽ (f->is_unreachable) { *o_type = c_valBottom; തിരികെ m3Err_none; }
        തിരികെ m3_Err_function_അട്ടി_Underrun;
    }
    *o_type = v->opd[--v->opd_മുകൾ];
    തിരികെ m3Err_none;
}

സ്ഥിതം M3Result v_എടുക്കുക_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_എടുക്കുക(v, &actual);
    എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    എങ്കിൽ (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        തിരികെ m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    തിരികെ m3Err_none;
}

// ---------- Control stack ----------

സ്ഥിതം M3Result v_തള്ളുക_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    എങ്കിൽ (M3_UNLIKELY(v->ctrl_മുകൾ >= d_m3ValCtrlDepth))
        തിരികെ m3_Err_function_അട്ടി_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_മുകൾ++];
    f->opcode       = op;
    f->type         = type;
    f->param_എണ്ണം  = type ? type->അക്കം_Args : 0;
    f->result_എണ്ണം = type ? type->അക്കം_Rets : 0;
    f->height       = v->opd_മുകൾ;
    f->is_unreachable = false;
    തിരികെ m3Err_none;
}

സ്ഥിതം M3Result v_എടുക്കുക_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    എങ്കിൽ (M3_UNLIKELY(v->ctrl_മുകൾ == 0))
        തിരികെ m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_മുകൾ - 1];
    // pop result types
    എങ്കിൽ (f->type) {
        വേണ്ടി (u16 i = f->result_എണ്ണം; i > 0; i--) {
            m3type_t a;
            M3Result r = v_എടുക്കുക_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
        }
    }
    എങ്കിൽ (M3_UNLIKELY(v->opd_മുകൾ != f->height))
        തിരികെ m3_Err_type_എണ്ണം_Mismatch;
    എങ്കിൽ (o_frame) *o_frame = *f;
    v->ctrl_മുകൾ--;
    തിരികെ m3Err_none;
}

സ്ഥിതം ശൂന്യം v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_മുകൾ - 1];
    v->opd_മുകൾ = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
സ്ഥിതം u16 v_label_n (ValCtrlFrame * f)
{
    തിരികെ (f->opcode == 0x03) ? f->param_എണ്ണം : f->result_എണ്ണം;
}

സ്ഥിതം m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    എങ്കിൽ (!f->type) തിരികെ c_m3Type_none;
    എങ്കിൽ (f->opcode == 0x03)
        തിരികെ BaseTypeOf(f->type->types[f->type->അക്കം_Rets + i]); // params
    തിരികെ BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
സ്ഥിതം m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_മുകൾ - 1];

    എങ്കിൽ (v->opd_മുകൾ <= i_depth || (u16)(v->opd_മുകൾ - i_depth - 1) < f->height)
        തിരികെ c_valBottom;

    തിരികെ v->opd[v->opd_മുകൾ - i_depth - 1];
}

// Pop label types for branch target
സ്ഥിതം M3Result v_എടുക്കുക_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    വേണ്ടി (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_എടുക്കുക_expect(v, v_label_t(tgt, i - 1), &a);
        എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    }
    തിരികെ m3Err_none;
}

// Push label types back
സ്ഥിതം M3Result v_തള്ളുക_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    വേണ്ടി (u16 i = 0; i < n; i++) {
        M3Result r = v_തള്ളുക(v, v_label_t(tgt, i));
        എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    }
    തിരികെ m3Err_none;
}

// ---------- Block type resolution ----------

സ്ഥിതം M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    എങ്കിൽ (v->wasm >= v->wasmEnd)
        തിരികെ m3Err_wasmUnderrun;

#എങ്കിൽ d_m3HasTypedRefs
    എങ്കിൽ (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_മൂല്യം_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        എങ്കിൽ (rr) തിരികെ rr;
        IM3FuncType ftype = ശൂന്യ;
        rr = AllocFuncType(&ftype, 1);
        എങ്കിൽ (rr) തിരികെ rr;
        ftype->അക്കം_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        എങ്കിൽ (rr) തിരികെ rr;
        *o_type = ftype;
        തിരികെ m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    എങ്കിൽ (r) തിരികെ r;

    എങ്കിൽ (type < 0) {
        എങ്കിൽ (type < -64) തിരികെ m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        എങ്കിൽ (r) തിരികെ r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } അല്ലെങ്കിൽ {
        എങ്കിൽ ((u32)type >= v->module->അക്കം_Func_Types)
            തിരികെ m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    തിരികെ m3Err_none;
}

// ---------- Convenience ----------

സ്ഥിതം M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_എടുക്കുക_expect(v, in, &a);
    എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    തിരികെ v_തള്ളുക(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
സ്ഥിതം M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ശൂന്യ;

    u16 അക്കം_Callee_Rets = i_calleeType ? i_calleeType->അക്കം_Rets : 0;
    u16 അക്കം_Func_Rets   = ft ? ft->അക്കം_Rets : 0;

    എങ്കിൽ (അക്കം_Callee_Rets != അക്കം_Func_Rets)
        തിരികെ m3Err_typeMismatch;

    വേണ്ടി (u16 i = 0; i < അക്കം_Callee_Rets; i++) {
        എങ്കിൽ (i_calleeType->types[i] != ft->types[i])
            തിരികെ m3Err_typeMismatch;
    }

    തിരികെ m3Err_none;
}

സ്ഥിതം M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_എടുക്കുക_expect(v, t, &a); എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    r = v_എടുക്കുക_expect(v, t, &a); എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    തിരികെ v_തള്ളുക(v, t);
}

സ്ഥിതം M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_എടുക്കുക_expect(v, t, &a); എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    r = v_എടുക്കുക_expect(v, t, &a); എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;
    തിരികെ v_തള്ളുക(v, c_m3Type_i32);
}

സ്ഥിതം M3Result v_testop (ValCtx * v, m3type_t t)
{
    തിരികെ v_unop(v, t, c_m3Type_i32);
}

സ്ഥിതം M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    തിരികെ v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#എങ്കിൽ d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
സ്ഥിതം M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    എങ്കിൽ (r) തിരികെ r;
    എങ്കിൽ (kind > 0x03) തിരികെ m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = ശൂന്യ;
    എങ്കിൽ (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        എങ്കിൽ (r) തിരികെ r;
        എങ്കിൽ (!v->module || tagIndex >= v->module->അക്കം_Tags) തിരികെ m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    എങ്കിൽ (r) തിരികെ r;
    എങ്കിൽ (depth >= v->ctrl_മുകൾ) തിരികെ m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_മുകൾ - 1 - depth];

    u16 അക്കം_Payload = tagType ? tagType->അക്കം_Args : 0;
    u16 അക്കം_Label   = v_label_n(tgt);

    എങ്കിൽ (അക്കം_Label != അക്കം_Payload + (hasRef ? 1u : 0u))
        തിരികെ m3_Err_type_എണ്ണം_Mismatch;

    വേണ്ടി (u16 i = 0; i < അക്കം_Payload; i++) {
        എങ്കിൽ (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->അക്കം_Rets + i]))
            തിരികെ m3Err_typeMismatch;
    }

    എങ്കിൽ (hasRef and v_label_t(tgt, അക്കം_Payload) != c_m3Type_exnref)
        തിരികെ m3Err_typeMismatch;

    തിരികെ m3Err_none;
}

#endif // d_m3HasExceptionHandling

#എങ്കിൽ d_m3_Has_അട്ടി_Switching

സ്ഥിതം M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    എങ്കിൽ (kind > 0x01) തിരികെ m3Err_wasmMalformed;
    എങ്കിൽ (!v->module || tagIdx >= v->module->അക്കം_Tags) തിരികെ m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    എങ്കിൽ (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
        എങ്കിൽ (depth >= v->ctrl_മുകൾ) തിരികെ m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_മുകൾ - 1 - depth];
        u16 അക്കം_Payload = tagType ? tagType->അക്കം_Args : 0;
        u16 അക്കം_Label   = v_label_n(tgt);

        എങ്കിൽ (അക്കം_Label != അക്കം_Payload + 1u) തിരികെ m3_Err_type_എണ്ണം_Mismatch;

        വേണ്ടി (u16 j = 0; j < അക്കം_Payload; j++) {
            എങ്കിൽ (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->അക്കം_Rets + j]))
                തിരികെ m3Err_typeMismatch;
        }
        എങ്കിൽ (v_label_t(tgt, അക്കം_Payload) != c_m3Type_contref)
            തിരികെ m3Err_typeMismatch;

#എങ്കിൽ d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->അക്കം_Rets + അക്കം_Payload]
            : tgt->type->types[അക്കം_Payload];

        എങ്കിൽ (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            തിരികെ m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = ശൂന്യ;
        വേണ്ടി (u32 idx = 0; idx < v->module->അക്കം_Func_Types; ++idx) {
            എങ്കിൽ (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                നിർത്തുക;
            }
        }
        എങ്കിൽ (!labelCt) തിരികെ m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_അക്കം_Params = tagType ? tagType->അക്കം_Rets : 0;
        എങ്കിൽ (labelFt->അക്കം_Args != expected_അക്കം_Params) തിരികെ m3Err_typeMismatch;
        വേണ്ടി (u16 p = 0; p < expected_അക്കം_Params; ++p) {
            എങ്കിൽ (labelFt->types[labelFt->അക്കം_Rets + p] != tagType->types[p])
                തിരികെ m3Err_typeMismatch;
        }

        u16 expected_അക്കം_Rets = ft ? ft->അക്കം_Rets : 0;
        എങ്കിൽ (labelFt->അക്കം_Rets != expected_അക്കം_Rets) തിരികെ m3Err_typeMismatch;
        വേണ്ടി (u16 r_idx = 0; r_idx < expected_അക്കം_Rets; ++r_idx) {
            എങ്കിൽ (labelFt->types[r_idx] != ft->types[r_idx])
                തിരികെ m3Err_typeMismatch;
        }
#endif
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (tagType && tagType->അക്കം_Args != 0) തിരികെ m3Err_typeMismatch;
    }
    തിരികെ m3Err_none;
}

#endif // d_m3HasStackSwitching

സ്ഥിതം M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    ആകുമ്പോൾ (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        എങ്കിൽ (M3_UNLIKELY(r)) തിരികെ r;

        തിരഞ്ഞെടുപ്പ് (opcode)
        {
        // ---- Control ----
        അവസ്ഥ 0x00: // unreachable
            v_unreachable(v);
            നിർത്തുക;

        അവസ്ഥ 0x01: // nop
            നിർത്തുക;

        അവസ്ഥ 0x02: // block
        അവസ്ഥ 0x03: // loop
        അവസ്ഥ 0x04: // if
        {
            IM3FuncType bt = ശൂന്യ;
            r = v_read_blocktype(v, &bt);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (opcode == 0x04) {
                r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a);
                എങ്കിൽ (r) തിരികെ r;
            }
            // Pop block params from caller stack
            എങ്കിൽ (bt) {
                വേണ്ടി (u16 i = bt->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(bt->types[bt->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            r = v_തള്ളുക_ctrl(v, opcode, bt);
            എങ്കിൽ (r) തിരികെ r;
            // Push params inside block
            എങ്കിൽ (bt) {
                വേണ്ടി (u16 i = 0; i < bt->അക്കം_Args; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(bt->types[bt->അക്കം_Rets + i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

#എങ്കിൽ d_m3HasExceptionHandling
        അവസ്ഥ 0x1f: // try_table
        {
            IM3FuncType bt = ശൂന്യ;
            r = v_read_blocktype(v, &bt);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (bt) {
                വേണ്ടി (u16 i = bt->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(bt->types[bt->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 അക്കം_Catch;
            r = ReadLEB_u32(&അക്കം_Catch, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            വേണ്ടി (u32 i = 0; i < അക്കം_Catch; i++) {
                r = v_catch_clause(v);
                എങ്കിൽ (r) തിരികെ r;
            }

            r = v_തള്ളുക_ctrl(v, opcode, bt);
            എങ്കിൽ (r) തിരികെ r;

            എങ്കിൽ (bt) {
                വേണ്ടി (u16 i = 0; i < bt->അക്കം_Args; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(bt->types[bt->അക്കം_Rets + i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (!v->module || tagIndex >= v->module->അക്കം_Tags) തിരികെ m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            എങ്കിൽ (tt) {
                വേണ്ടി (u16 i = tt->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(tt->types[tt->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            v_unreachable(v);
            നിർത്തുക;
        }

        അവസ്ഥ 0x0a: // throw_ref
            r = v_എടുക്കുക_expect(v, c_m3Type_exnref, &a);
            എങ്കിൽ (r) തിരികെ r;
            v_unreachable(v);
            നിർത്തുക;
#endif // d_m3HasExceptionHandling

        അവസ്ഥ 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_എടുക്കുക_ctrl(v, &frame);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (frame.opcode != 0x04)
                തിരികെ m3Err_wasmMalformed;
            r = v_തള്ളുക_ctrl(v, 0x05, frame.type);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (frame.type) {
                വേണ്ടി (u16 i = 0; i < frame.type->അക്കം_Args; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(frame.type->types[frame.type->അക്കം_Rets + i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_എടുക്കുക_ctrl(v, &frame);
            എങ്കിൽ (r) തിരികെ r;
            // Push results
            എങ്കിൽ (frame.type) {
                വേണ്ടി (u16 i = 0; i < frame.result_എണ്ണം; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(frame.type->types[i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            // If this was the outermost frame, we're done
            എങ്കിൽ (v->ctrl_മുകൾ == 0)
                തിരികെ m3Err_none;
            നിർത്തുക;
        }

        അവസ്ഥ 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (depth >= v->ctrl_മുകൾ) തിരികെ m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_മുകൾ - 1 - depth];
            r = v_എടുക്കുക_labels(v, tgt);
            എങ്കിൽ (r) തിരികെ r;
            v_unreachable(v);
            നിർത്തുക;
        }

        അവസ്ഥ 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (depth >= v->ctrl_മുകൾ) തിരികെ m3Err_unknownLabel;
            r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a);
            എങ്കിൽ (r) തിരികെ r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_മുകൾ - 1 - depth];
            r = v_എടുക്കുക_labels(v, tgt);
            എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക_labels(v, tgt);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0x0e: // br_table
        {
            u32 എണ്ണം;
            r = ReadLEB_u32(&എണ്ണം, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            വേണ്ടി (u32 i = 0; i <= എണ്ണം; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                എങ്കിൽ (r) തിരികെ r;
                എങ്കിൽ (d >= v->ctrl_മുകൾ) തിരികെ m3Err_unknownLabel;
                എങ്കിൽ (i == എണ്ണം) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_മുകൾ - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            എങ്കിൽ (r) തിരികെ r;

            വേണ്ടി (u32 i = 0; i <= എണ്ണം; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                എങ്കിൽ (r) തിരികെ r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_മുകൾ - 1 - d];
                u16 n = v_label_n(t);
                എങ്കിൽ (n != arity) തിരികെ m3_Err_type_എണ്ണം_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                വേണ്ടി (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    എങ്കിൽ (want != c_valBottom && have != c_valBottom && want != have)
                        തിരികെ m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_മുകൾ - 1 - defDepth];
            r = v_എടുക്കുക_labels(v, dt);
            എങ്കിൽ (r) തിരികെ r;
            v_unreachable(v);
            നിർത്തുക;
        }

        അവസ്ഥ 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = ft->അക്കം_Rets; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            v_unreachable(v);
            നിർത്തുക;
        }

        // ---- Call ----
        അവസ്ഥ 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (idx >= v->module->അക്കം_Functions) തിരികെ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = ft->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[ft->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
                വേണ്ടി (u16 i = 0; i < ft->അക്കം_Rets; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(ft->types[i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            // the table must exist and hold funcrefs
            എങ്കിൽ (tableIdx >= v->module->അക്കം_Tables) തിരികെ m3Err_unknownTable;
            എങ്കിൽ (v->module->tables[tableIdx]->type != c_m3Type_funcref) തിരികെ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_എടുക്കുക_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = ft->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[ft->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
                വേണ്ടി (u16 i = 0; i < ft->അക്കം_Rets; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(ft->types[i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (idx >= v->module->അക്കം_Functions) തിരികെ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = ft->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[ft->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            v_unreachable(v);
            നിർത്തുക;
        }

        അവസ്ഥ 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            // the table must exist and hold funcrefs
            എങ്കിൽ (tableIdx >= v->module->അക്കം_Tables) തിരികെ m3Err_unknownTable;
            എങ്കിൽ (v->module->tables[tableIdx]->type != c_m3Type_funcref) തിരികെ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            എങ്കിൽ (r) തിരികെ r;
            r = v_എടുക്കുക_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = ft->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[ft->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            v_unreachable(v);
            നിർത്തുക;
        }

#എങ്കിൽ d_m3HasTypedRefs
        അവസ്ഥ 0x14: // call_ref
        അവസ്ഥ 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            എങ്കിൽ (isTail) {
                r = v_check_tail_results(v, ft);
                എങ്കിൽ (r) തിരികെ r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_എടുക്കുക_expect(v, c_m3Type_funcref, &a);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = ft->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[ft->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
                എങ്കിൽ (not isTail) {
                    വേണ്ടി (u16 i = 0; i < ft->അക്കം_Rets; i++) {
                        r = v_തള്ളുക(v, BaseTypeOf(ft->types[i]));
                        എങ്കിൽ (r) തിരികെ r;
                    }
                }
            }
            എങ്കിൽ (isTail) v_unreachable(v);
            നിർത്തുക;
        }

        അവസ്ഥ 0xd4: // ref.as_non_null
        {
            r = v_എടുക്കുക_expect(v, c_m3Type_funcref, &a);
            എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        അവസ്ഥ 0x1a: // drop
            r = v_എടുക്കുക(v, &a);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;

        അവസ്ഥ 0x1b: // select
        {
            r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a);
            എങ്കിൽ (r) തിരികെ r;
            m3type_t t2 = c_valBottom;
            r = v_എടുക്കുക(v, &t2);
            എങ്കിൽ (r) തിരികെ r;
            m3type_t t1;
            r = v_എടുക്കുക_expect(v, t2, &t1);
            എങ്കിൽ (r) തിരികെ r;
            // untyped select is numeric only; references need the 0x1c form
            എങ്കിൽ (t2 != c_valBottom && IsRefType(t2)) തിരികെ m3Err_typeMismatch;
            r = v_തള്ളുക(v, (t2 == c_valBottom) ? t1 : t2);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

#എങ്കിൽ d_m3HasRefTypes
        അവസ്ഥ 0x1c: // select with an explicit result type
        {
            u32 അക്കം_Types;
            r = ReadLEB_u32(&അക്കം_Types, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (അക്കം_Types != 1) തിരികെ m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_മൂല്യം_Type(v->module, &selType, &v->wasm, v->wasmEnd);  എങ്കിൽ (r) തിരികെ r;
            u8 t = BaseTypeOf(selType);

            r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a); എങ്കിൽ (r) തിരികെ r;
            r = v_എടുക്കുക_expect(v, t, &a);            എങ്കിൽ (r) തിരികെ r;
            r = v_എടുക്കുക_expect(v, t, &a);            എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക(v, t);                      എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0x25: // table.get
        അവസ്ഥ 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (tableIdx >= v->module->അക്കം_Tables) തിരികെ m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            എങ്കിൽ (opcode == 0x26) {
                r = v_എടുക്കുക_expect(v, t, &a);   എങ്കിൽ (r) തിരികെ r;
                r = v_എടുക്കുക_expect(v, at, &a);  എങ്കിൽ (r) തിരികെ r;
            } അല്ലെങ്കിൽ {
                r = v_എടുക്കുക_expect(v, at, &a);  എങ്കിൽ (r) തിരികെ r;
                r = v_തള്ളുക(v, t);             എങ്കിൽ (r) തിരികെ r;
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0xd0: // ref.null
        {
#എങ്കിൽ d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  എങ്കിൽ (r) തിരികെ r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# എങ്കിൽ d_m3_Has_അട്ടി_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#അല്ലെങ്കിൽ
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   എങ്കിൽ (r) തിരികെ r;
            r = NormalizeType(&t, waType);                   എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (!IsRefType(t)) തിരികെ m3Err_wasmMalformed;
#endif
            r = v_തള്ളുക(v, t);                                എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_എടുക്കുക(v, &t);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (t != c_valBottom && !IsRefType(t)) തിരികെ m3Err_typeMismatch;
            r = v_തള്ളുക(v, c_m3Type_i32);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (funcIdx >= v->module->അക്കം_Functions) തിരികെ m3Err_unknownFunction;
            എങ്കിൽ (!Module_IsFunctionDeclared(v->module, funcIdx)) തിരികെ m3Err_undeclaredFuncRef;
            r = v_തള്ളുക(v, c_m3Type_funcref);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }
#endif

        // ---- Variable ----
        അവസ്ഥ 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (idx >= v->അക്കം_Locals) തിരികെ m3Err_unknownLocal;
            r = v_തള്ളുക(v, v->localTypes[idx]);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (idx >= v->അക്കം_Locals) തിരികെ m3Err_unknownLocal;
            r = v_എടുക്കുക_expect(v, v->localTypes[idx], &a);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (idx >= v->അക്കം_Locals) തിരികെ m3Err_unknownLocal;
            r = v_എടുക്കുക_expect(v, v->localTypes[idx], &a);
            എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക(v, v->localTypes[idx]);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (idx >= v->module->അക്കം_Globals) തിരികെ m3Err_unknownGlobal;
            r = v_തള്ളുക(v, BaseTypeOf(v->module->globals[idx].type));
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (idx >= v->module->അക്കം_Globals) തിരികെ m3Err_unknownGlobal;
            r = v_എടുക്കുക_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        // ---- Memory load ----
        അവസ്ഥ 0x28: അവസ്ഥ 0x29: അവസ്ഥ 0x2a: അവസ്ഥ 0x2b: // i32/i64/f32/f64.load
        അവസ്ഥ 0x2c: അവസ്ഥ 0x2d: അവസ്ഥ 0x2e: അവസ്ഥ 0x2f: // i32.load8/16 s/u
        അവസ്ഥ 0x30: അവസ്ഥ 0x31: അവസ്ഥ 0x32: അവസ്ഥ 0x33: // i64.load8/16 s/u
        അവസ്ഥ 0x34: അവസ്ഥ 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (align > v_max_align(opcode)) തിരികെ m3Err_invalidAlignment;
            എങ്കിൽ (not v_has_memory_idx(v, memidx)) തിരികെ m3Err_unknownMemory;
            എങ്കിൽ (not v_offset_in_range(v, memidx, offset)) തിരികെ m3Err_wasmMalformed;
            r = v_എടുക്കുക_expect(v, v_memory_addrtype(v, memidx), &a); എങ്കിൽ (r) തിരികെ r;
            u8 result;
            എങ്കിൽ      (opcode == 0x28) result = c_m3Type_i32;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode == 0x29) result = c_m3Type_i64;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode == 0x2a) result = c_m3Type_f32;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode == 0x2b) result = c_m3Type_f64;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode <= 0x2f) result = c_m3Type_i32;
            അല്ലെങ്കിൽ                     result = c_m3Type_i64;
            r = v_തള്ളുക(v, result);
            എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        // ---- Memory store ----
        അവസ്ഥ 0x36: അവസ്ഥ 0x37: അവസ്ഥ 0x38: അവസ്ഥ 0x39: // i32/i64/f32/f64.store
        അവസ്ഥ 0x3a: അവസ്ഥ 0x3b:                         // i32.store8/16
        അവസ്ഥ 0x3c: അവസ്ഥ 0x3d: അവസ്ഥ 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (align > v_max_align(opcode)) തിരികെ m3Err_invalidAlignment;
            എങ്കിൽ (not v_has_memory_idx(v, memidx)) തിരികെ m3Err_unknownMemory;
            എങ്കിൽ (not v_offset_in_range(v, memidx, offset)) തിരികെ m3Err_wasmMalformed;
            u8 valtype;
            എങ്കിൽ      (opcode == 0x36) valtype = c_m3Type_i32;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode == 0x37) valtype = c_m3Type_i64;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode == 0x38) valtype = c_m3Type_f32;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode == 0x39) valtype = c_m3Type_f64;
            അല്ലെങ്കിൽ എങ്കിൽ (opcode <= 0x3b) valtype = c_m3Type_i32;
            അല്ലെങ്കിൽ                     valtype = c_m3Type_i64;
            r = v_എടുക്കുക_expect(v, valtype, &a); എങ്കിൽ (r) തിരികെ r;
            r = v_എടുക്കുക_expect(v, v_memory_addrtype(v, memidx), &a); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        // ---- Memory size/grow ----
        അവസ്ഥ 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (not v_has_memory_idx(v, memidx)) തിരികെ m3Err_unknownMemory;
            r = v_തള്ളുക(v, v_memory_addrtype(v, memidx)); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }
        അവസ്ഥ 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (not v_has_memory_idx(v, memidx)) തിരികെ m3Err_unknownMemory;
            r = v_എടുക്കുക_expect(v, v_memory_addrtype(v, memidx), &a); എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക(v, v_memory_addrtype(v, memidx)); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        // ---- Constants ----
        അവസ്ഥ 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക(v, c_m3Type_i32); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }
        അവസ്ഥ 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക(v, c_m3Type_i64); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }
        അവസ്ഥ 0x43: { // f32.const
            എങ്കിൽ (v->wasm + 4 > v->wasmEnd) തിരികെ m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_തള്ളുക(v, c_m3Type_f32); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }
        അവസ്ഥ 0x44: { // f64.const
            എങ്കിൽ (v->wasm + 8 > v->wasmEnd) തിരികെ m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_തള്ളുക(v, c_m3Type_f64); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }


        // ---- i32 comparison ----
        അവസ്ഥ 0x45: r = v_testop(v, c_m3Type_i32); നിർത്തുക; // i32.eqz
        അവസ്ഥ 0x46: അവസ്ഥ 0x47: അവസ്ഥ 0x48: അവസ്ഥ 0x49: അവസ്ഥ 0x4a:
        അവസ്ഥ 0x4b: അവസ്ഥ 0x4c: അവസ്ഥ 0x4d: അവസ്ഥ 0x4e: അവസ്ഥ 0x4f:
            r = v_relop(v, c_m3Type_i32); നിർത്തുക;

        // ---- i64 comparison ----
        അവസ്ഥ 0x50: r = v_testop(v, c_m3Type_i64); നിർത്തുക; // i64.eqz
        അവസ്ഥ 0x51: അവസ്ഥ 0x52: അവസ്ഥ 0x53: അവസ്ഥ 0x54: അവസ്ഥ 0x55:
        അവസ്ഥ 0x56: അവസ്ഥ 0x57: അവസ്ഥ 0x58: അവസ്ഥ 0x59: അവസ്ഥ 0x5a:
            r = v_relop(v, c_m3Type_i64); നിർത്തുക;

        // ---- f32 comparison ----
        അവസ്ഥ 0x5b: അവസ്ഥ 0x5c: അവസ്ഥ 0x5d: അവസ്ഥ 0x5e: അവസ്ഥ 0x5f: അവസ്ഥ 0x60:
            r = v_relop(v, c_m3Type_f32); നിർത്തുക;

        // ---- f64 comparison ----
        അവസ്ഥ 0x61: അവസ്ഥ 0x62: അവസ്ഥ 0x63: അവസ്ഥ 0x64: അവസ്ഥ 0x65: അവസ്ഥ 0x66:
            r = v_relop(v, c_m3Type_f64); നിർത്തുക;

        // ---- i32 unary ----
        അവസ്ഥ 0x67: അവസ്ഥ 0x68: അവസ്ഥ 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); നിർത്തുക;

        // ---- i32 binary ----
        അവസ്ഥ 0x6a: അവസ്ഥ 0x6b: അവസ്ഥ 0x6c: അവസ്ഥ 0x6d: അവസ്ഥ 0x6e: അവസ്ഥ 0x6f:
        അവസ്ഥ 0x70: അവസ്ഥ 0x71: അവസ്ഥ 0x72: അവസ്ഥ 0x73: അവസ്ഥ 0x74: അവസ്ഥ 0x75:
        അവസ്ഥ 0x76: അവസ്ഥ 0x77: അവസ്ഥ 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); നിർത്തുക;

        // ---- i64 unary ----
        അവസ്ഥ 0x79: അവസ്ഥ 0x7a: അവസ്ഥ 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); നിർത്തുക;

        // ---- i64 binary ----
        അവസ്ഥ 0x7c: അവസ്ഥ 0x7d: അവസ്ഥ 0x7e: അവസ്ഥ 0x7f: അവസ്ഥ 0x80: അവസ്ഥ 0x81:
        അവസ്ഥ 0x82: അവസ്ഥ 0x83: അവസ്ഥ 0x84: അവസ്ഥ 0x85: അവസ്ഥ 0x86: അവസ്ഥ 0x87:
        അവസ്ഥ 0x88: അവസ്ഥ 0x89: അവസ്ഥ 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); നിർത്തുക;

        // ---- f32 unary ----
        അവസ്ഥ 0x8b: അവസ്ഥ 0x8c: അവസ്ഥ 0x8d: അവസ്ഥ 0x8e: അവസ്ഥ 0x8f:
        അവസ്ഥ 0x90: അവസ്ഥ 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); നിർത്തുക;

        // ---- f32 binary ----
        അവസ്ഥ 0x92: അവസ്ഥ 0x93: അവസ്ഥ 0x94: അവസ്ഥ 0x95: അവസ്ഥ 0x96:
        അവസ്ഥ 0x97: അവസ്ഥ 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); നിർത്തുക;

        // ---- f64 unary ----
        അവസ്ഥ 0x99: അവസ്ഥ 0x9a: അവസ്ഥ 0x9b: അവസ്ഥ 0x9c: അവസ്ഥ 0x9d:
        അവസ്ഥ 0x9e: അവസ്ഥ 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); നിർത്തുക;

        // ---- f64 binary ----
        അവസ്ഥ 0xa0: അവസ്ഥ 0xa1: അവസ്ഥ 0xa2: അവസ്ഥ 0xa3: അവസ്ഥ 0xa4:
        അവസ്ഥ 0xa5: അവസ്ഥ 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); നിർത്തുക;

        // ---- Conversions ----
        അവസ്ഥ 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); നിർത്തുക; // i32.wrap/i64
        അവസ്ഥ 0xa8: അവസ്ഥ 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); നിർത്തുക;
        അവസ്ഥ 0xaa: അവസ്ഥ 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); നിർത്തുക;
        അവസ്ഥ 0xac: അവസ്ഥ 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); നിർത്തുക;
        അവസ്ഥ 0xae: അവസ്ഥ 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); നിർത്തുക;
        അവസ്ഥ 0xb0: അവസ്ഥ 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); നിർത്തുക;
        അവസ്ഥ 0xb2: അവസ്ഥ 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); നിർത്തുക;
        അവസ്ഥ 0xb4: അവസ്ഥ 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); നിർത്തുക;
        അവസ്ഥ 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); നിർത്തുക;
        അവസ്ഥ 0xb7: അവസ്ഥ 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); നിർത്തുക;
        അവസ്ഥ 0xb9: അവസ്ഥ 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); നിർത്തുക;
        അവസ്ഥ 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); നിർത്തുക;
        അവസ്ഥ 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); നിർത്തുക;
        അവസ്ഥ 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); നിർത്തുക;
        അവസ്ഥ 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); നിർത്തുക;
        അവസ്ഥ 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); നിർത്തുക;

        // ---- Sign-extension (MVP post) ----
        അവസ്ഥ 0xc0: അവസ്ഥ 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); നിർത്തുക;
        അവസ്ഥ 0xc2: അവസ്ഥ 0xc3: അവസ്ഥ 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); നിർത്തുക;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        അവസ്ഥ 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;
            തിരഞ്ഞെടുപ്പ് (sub) {
            അവസ്ഥ 0x00: അവസ്ഥ 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); നിർത്തുക;
            അവസ്ഥ 0x02: അവസ്ഥ 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); നിർത്തുക;
            അവസ്ഥ 0x04: അവസ്ഥ 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); നിർത്തുക;
            അവസ്ഥ 0x06: അവസ്ഥ 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); നിർത്തുക;
            അവസ്ഥ 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                എങ്കിൽ (not v_has_memory_idx(v, memidx)) തിരികെ m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                എങ്കിൽ (not v->module->has_Data_എണ്ണം) തിരികെ m3_Err_data_എണ്ണം_Required;
                എങ്കിൽ (dataidx >= v->module->അക്കം_Data_Segments) തിരികെ m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a); എങ്കിൽ (r) തിരികെ r; // n
                r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a); എങ്കിൽ (r) തിരികെ r; // src
                r = v_എടുക്കുക_expect(v, v_memory_addrtype(v, memidx), &a); എങ്കിൽ (r) തിരികെ r; // dst
                നിർത്തുക;
            }
            അവസ്ഥ 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                // needs the segment, but not a memory
                എങ്കിൽ (not v->module->has_Data_എണ്ണം) തിരികെ m3_Err_data_എണ്ണം_Required;
                എങ്കിൽ (dataidx >= v->module->അക്കം_Data_Segments) തിരികെ m3Err_unknownDataSegment;
                നിർത്തുക;
            }
#എങ്കിൽ d_m3HasRefTypes
            അവസ്ഥ 0x0c: // table.init
            അവസ്ഥ 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;

                എങ്കിൽ (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    എങ്കിൽ (a2 >= v->module->അക്കം_Tables) തിരികെ m3Err_unknownTable;
                    എങ്കിൽ (a1 >= v->module->അക്കം_Element_Segments) തിരികെ m3Err_unknownElemSegment;
                    എങ്കിൽ (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        തിരികെ m3Err_typeMismatch;
                } അല്ലെങ്കിൽ {                                                    // dst, src
                    എങ്കിൽ (a1 >= v->module->അക്കം_Tables) തിരികെ m3Err_unknownTable;
                    എങ്കിൽ (a2 >= v->module->അക്കം_Tables) തിരികെ m3Err_unknownTable;
                    എങ്കിൽ (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        തിരികെ m3Err_typeMismatch;
                }

                എങ്കിൽ (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a); എങ്കിൽ (r) തിരികെ r;             // n
                    r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a); എങ്കിൽ (r) തിരികെ r;             // s
                    r = v_എടുക്കുക_expect(v, v_table_addrtype(v, a2), &a); എങ്കിൽ (r) തിരികെ r;  // d
                } അല്ലെങ്കിൽ {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_എടുക്കുക_expect(v, nType, &a); എങ്കിൽ (r) തിരികെ r;  // n
                    r = v_എടുക്കുക_expect(v, sType, &a); എങ്കിൽ (r) തിരികെ r;  // s
                    r = v_എടുക്കുക_expect(v, dType, &a); എങ്കിൽ (r) തിരികെ r;  // d
                }
                നിർത്തുക;
            }
            അവസ്ഥ 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                എങ്കിൽ (elemIdx >= v->module->അക്കം_Element_Segments) തിരികെ m3Err_unknownElemSegment;
                നിർത്തുക;
            }
            അവസ്ഥ 0x0f: // table.grow
            അവസ്ഥ 0x10: // table.size
            അവസ്ഥ 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                എങ്കിൽ (tableIdx >= v->module->അക്കം_Tables) തിരികെ m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                എങ്കിൽ (sub == 0x10) {                                          // table.size
                    r = v_തള്ളുക(v, at); എങ്കിൽ (r) തിരികെ r;
                } അല്ലെങ്കിൽ എങ്കിൽ (sub == 0x0f) {                                   // table.grow
                    r = v_എടുക്കുക_expect(v, at, &a); എങ്കിൽ (r) തിരികെ r;  // n
                    r = v_എടുക്കുക_expect(v, t, &a);  എങ്കിൽ (r) തിരികെ r;  // init
                    r = v_തള്ളുക(v, at);           എങ്കിൽ (r) തിരികെ r;
                } അല്ലെങ്കിൽ {                                                    // table.fill
                    r = v_എടുക്കുക_expect(v, at, &a); എങ്കിൽ (r) തിരികെ r;  // n
                    r = v_എടുക്കുക_expect(v, t, &a);  എങ്കിൽ (r) തിരികെ r;  // val
                    r = v_എടുക്കുക_expect(v, at, &a); എങ്കിൽ (r) തിരികെ r;  // i
                }
                നിർത്തുക;
            }
#endif
            അവസ്ഥ 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                എങ്കിൽ (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) തിരികെ m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_എടുക്കുക_expect(v, lenType, &a); എങ്കിൽ (r) തിരികെ r; // n
                r = v_എടുക്കുക_expect(v, srcType, &a); എങ്കിൽ (r) തിരികെ r; // src
                r = v_എടുക്കുക_expect(v, dstType, &a); എങ്കിൽ (r) തിരികെ r; // dst
                നിർത്തുക;
            }
            അവസ്ഥ 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                എങ്കിൽ (not v_has_memory_idx(v, memidx)) തിരികെ m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_എടുക്കുക_expect(v, addrType, &a); എങ്കിൽ (r) തിരികെ r;      // n
                r = v_എടുക്കുക_expect(v, c_m3Type_i32, &a); എങ്കിൽ (r) തിരികെ r;  // val
                r = v_എടുക്കുക_expect(v, addrType, &a); എങ്കിൽ (r) തിരികെ r;      // dst
                നിർത്തുക;
            }
#എങ്കിൽ d_m3HasWideArithmetic
            അവസ്ഥ 0x13: അവസ്ഥ 0x14: // i64.add128, i64.sub128
            അവസ്ഥ 0x15: അവസ്ഥ 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 അക്കം_Operands = (sub <= 0x14) ? 4 : 2;
                വേണ്ടി (u32 i = 0; i < അക്കം_Operands; ++i) {
                    r = v_എടുക്കുക_expect(v, c_m3Type_i64, &a); എങ്കിൽ (r) തിരികെ r;
                }
                r = v_തള്ളുക(v, c_m3Type_i64); എങ്കിൽ (r) തിരികെ r;
                r = v_തള്ളുക(v, c_m3Type_i64); എങ്കിൽ (r) തിരികെ r;
                നിർത്തുക;
            }
#endif
            സ്ഥിരസ്ഥിതി:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                നിർത്തുക;
            }
            നിർത്തുക;
        }

#എങ്കിൽ d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        അവസ്ഥ 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            എങ്കിൽ (r) തിരികെ r;

            എങ്കിൽ (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                എങ്കിൽ (reserved != 0) തിരികെ m3Err_wasmMalformed;
                നിർത്തുക;
            }

            u32 width, അക്കം_Operands; m3type_t മൂല്യം_Type, resultType;
            എങ്കിൽ (not v_atomic_shape(sub, &width, &അക്കം_Operands, &മൂല്യം_Type, &resultType)) തിരികെ m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            എങ്കിൽ ((1u << align) != width) തിരികെ m3Err_invalidAtomicAlignment;
            എങ്കിൽ (not v_has_memory_idx(v, memidx)) തിരികെ m3Err_unknownMemory;
            എങ്കിൽ (not v_offset_in_range(v, memidx, offset)) തിരികെ m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            എങ്കിൽ (sub == 0x01 or sub == 0x02) {
                r = v_എടുക്കുക_expect(v, c_m3Type_i64, &a); എങ്കിൽ (r) തിരികെ r;
                അക്കം_Operands--;
            }
            വേണ്ടി (u32 i = 1; i < അക്കം_Operands; ++i) {
                r = v_എടുക്കുക_expect(v, മൂല്യം_Type, &a); എങ്കിൽ (r) തിരികെ r;
            }
            r = v_എടുക്കുക_expect(v, v_memory_addrtype(v, memidx), &a); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (resultType != c_m3Type_none) {
                r = v_തള്ളുക(v, resultType); എങ്കിൽ (r) തിരികെ r;
            }
            നിർത്തുക;
        }
#endif

#എങ്കിൽ d_m3_Has_അട്ടി_Switching
        // ---- Stack Switching ----
        അവസ്ഥ 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            എങ്കിൽ (!ft || !ft->isContinuation) തിരികെ m3Err_typeMismatch;
            r = v_എടുക്കുക_expect(v, c_m3Type_funcref, &a); എങ്കിൽ (r) തിരികെ r;
            r = v_തള്ളുക(v, c_m3Type_contref); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (typeIdx1 >= v->module->അക്കം_Func_Types || typeIdx2 >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            എങ്കിൽ (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) തിരികെ m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 അക്കം_Params1 = ft1->അക്കം_Args;
            u16 അക്കം_Params2 = ft2->അക്കം_Args;
            എങ്കിൽ (അക്കം_Params1 < അക്കം_Params2) തിരികെ m3Err_typeMismatch;
            u32 അക്കം_Bound = അക്കം_Params1 - അക്കം_Params2;

            എങ്കിൽ (ft1->അക്കം_Rets != ft2->അക്കം_Rets) തിരികെ m3Err_typeMismatch;
            വേണ്ടി (u16 i = 0; i < ft1->അക്കം_Rets; ++i) {
                എങ്കിൽ (ft1->types[i] != ft2->types[i]) തിരികെ m3Err_typeMismatch;
            }
            വേണ്ടി (u16 i = 0; i < അക്കം_Params2; ++i) {
                എങ്കിൽ (ft1->types[ft1->അക്കം_Rets + അക്കം_Bound + i] != ft2->types[ft2->അക്കം_Rets + i]) തിരികെ m3Err_typeMismatch;
            }

            r = v_എടുക്കുക_expect(v, c_m3Type_contref, &a); എങ്കിൽ (r) തിരികെ r;

            വേണ്ടി (u32 i = അക്കം_Bound; i > 0; i--) {
                r = v_എടുക്കുക_expect(v, BaseTypeOf(ft1->types[ft1->അക്കം_Rets + i - 1]), &a);
                എങ്കിൽ (r) തിരികെ r;
            }

            r = v_തള്ളുക(v, c_m3Type_contref); എങ്കിൽ (r) തിരികെ r;
            നിർത്തുക;
        }

        അവസ്ഥ 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (!v->module || tagIdx >= v->module->അക്കം_Tags) തിരികെ m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            എങ്കിൽ (tt) {
                വേണ്ടി (u16 i = tt->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(tt->types[tt->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
                വേണ്ടി (u16 i = 0; i < tt->അക്കം_Rets; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(tt->types[i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            എങ്കിൽ (!ct || !ct->isContinuation) തിരികെ m3Err_typeMismatch;

            u32 അക്കം_Clauses;
            r = ReadLEB_u32(&അക്കം_Clauses, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;

            വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                r = v_resume_clause(v, kind, tagIdx, ct); എങ്കിൽ (r) തിരികെ r;
            }

            r = v_എടുക്കുക_expect(v, c_m3Type_contref, &a); എങ്കിൽ (r) തിരികെ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = ft->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[ft->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
                വേണ്ടി (u16 i = 0; i < ft->അക്കം_Rets; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(ft->types[i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, അക്കം_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            r = ReadLEB_u32(&അക്കം_Clauses, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;

            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            എങ്കിൽ (!v->module || tagIdx >= v->module->അക്കം_Tags) തിരികെ m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            എങ്കിൽ (!ct || !ct->isContinuation) തിരികെ m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            എങ്കിൽ (tagType && tagType->അക്കം_Rets != 0) തിരികെ m3Err_typeMismatch;

            വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                r = v_resume_clause(v, kind, cTagIdx, ct); എങ്കിൽ (r) തിരികെ r;
            }

            r = v_എടുക്കുക_expect(v, c_m3Type_contref, &a); എങ്കിൽ (r) തിരികെ r;

            എങ്കിൽ (tagType) {
                വേണ്ടി (u16 i = tagType->അക്കം_Args; i > 0; i--) {
                    r = v_എടുക്കുക_expect(v, BaseTypeOf(tagType->types[tagType->അക്കം_Rets + i - 1]), &a);
                    എങ്കിൽ (r) തിരികെ r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = 0; i < ft->അക്കം_Rets; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(ft->types[i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0xe5: // resume_throw_ref
        {
            u32 typeIdx, അക്കം_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            r = ReadLEB_u32(&അക്കം_Clauses, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;

            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            എങ്കിൽ (!ct || !ct->isContinuation) തിരികെ m3Err_typeMismatch;

            വേണ്ടി (u32 i = 0; i < അക്കം_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
                r = v_resume_clause(v, kind, cTagIdx, ct); എങ്കിൽ (r) തിരികെ r;
            }

            r = v_എടുക്കുക_expect(v, c_m3Type_contref, &a); എങ്കിൽ (r) തിരികെ r;
            r = v_എടുക്കുക_expect(v, c_m3Type_exnref, &a); എങ്കിൽ (r) തിരികെ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            എങ്കിൽ (ft) {
                വേണ്ടി (u16 i = 0; i < ft->അക്കം_Rets; i++) {
                    r = v_തള്ളുക(v, BaseTypeOf(ft->types[i]));
                    എങ്കിൽ (r) തിരികെ r;
                }
            }
            നിർത്തുക;
        }

        അവസ്ഥ 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); എങ്കിൽ (r) തിരികെ r;

            എങ്കിൽ (typeIdx >= v->module->അക്കം_Func_Types) തിരികെ m3Err_unknownType;
            എങ്കിൽ (!v->module || tagIdx >= v->module->അക്കം_Tags) തിരികെ m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            എങ്കിൽ (!ct || !ct->isContinuation) തിരികെ m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            എങ്കിൽ (tagType && tagType->അക്കം_Args != 0) തിരികെ m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            എങ്കിൽ (!ft || ft->അക്കം_Args == 0) തിരികെ m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->അക്കം_Rets + ft->അക്കം_Args - 1]);
            എങ്കിൽ (!peer || !peer->contFuncType) തിരികെ m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_എടുക്കുക_expect(v, c_m3Type_contref, &a); എങ്കിൽ (r) തിരികെ r;

            വേണ്ടി (u16 i = (u16)(ft->അക്കം_Args - 1); i > 0; i--) {
                r = v_എടുക്കുക_expect(v, BaseTypeOf(ft->types[ft->അക്കം_Rets + i - 1]), &a);
                എങ്കിൽ (r) തിരികെ r;
            }

            വേണ്ടി (u16 i = 0; i < self->അക്കം_Args; i++) {
                r = v_തള്ളുക(v, BaseTypeOf(self->types[self->അക്കം_Rets + i]));
                എങ്കിൽ (r) തിരികെ r;
            }
            നിർത്തുക;
        }

#endif // d_m3HasStackSwitching

        സ്ഥിരസ്ഥിതി:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            നിർത്തുക;

        } // switch

        എങ്കിൽ (r) തിരികെ r;

    } // while

    // If we ran out of bytes without hitting the final end
    തിരികെ m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    എങ്കിൽ (!i_function->wasm) തിരികെ m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    എങ്കിൽ (!v) {
        v = m3_AllocStruct (ValCtx);
        എങ്കിൽ (!v) തിരികെ m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, വലുപ്പം(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    എങ്കിൽ (r) തിരികെ r;

    // Parse locals
    u32 അക്കം_Local_Blocks;
    r = ReadLEB_u32(&അക്കം_Local_Blocks, &v->wasm, v->wasmEnd);
    എങ്കിൽ (r) തിരികെ r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 അക്കം_Params = funcType ? funcType->അക്കം_Args : 0;
    എങ്കിൽ (അക്കം_Params > d_m3_Val_അട്ടി) തിരികെ m3_Err_function_അട്ടി_Overflow;
    വേണ്ടി (u16 i = 0; i < അക്കം_Params; i++) {
        v->localTypes[v->അക്കം_Locals++] = BaseTypeOf(funcType->types[funcType->അക്കം_Rets + i]);
    }

    // Then: declared locals
    വേണ്ടി (u32 b = 0; b < അക്കം_Local_Blocks; b++) {
        u32 എണ്ണം;
        r = ReadLEB_u32(&എണ്ണം, &v->wasm, v->wasmEnd);
        എങ്കിൽ (r) തിരികെ r;
        m3type_t localType;
        r = Parse_മൂല്യം_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        എങ്കിൽ (r) തിരികെ r;
        u8 normalized = BaseTypeOf(localType);
        എങ്കിൽ (എണ്ണം > (u32) (d_m3_Val_അട്ടി - v->അക്കം_Locals)) തിരികെ m3_Err_function_അട്ടി_Overflow;
        വേണ്ടി (u32 c = 0; c < എണ്ണം; c++) {
            v->localTypes[v->അക്കം_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_തള്ളുക_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    എങ്കിൽ (r) തിരികെ r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    എങ്കിൽ (r) തിരികെ r;

    // After validation, control stack should be empty and all declared bytes consumed
    എങ്കിൽ (v->ctrl_മുകൾ != 0)
        തിരികെ m3Err_wasmMalformed;

    എങ്കിൽ (v->wasm != v->wasmEnd)
        തിരികെ m3Err_wasmSectionUnderrun;

    തിരികെ m3Err_none;
}

#അല്ലെങ്കിൽ // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (ശൂന്യം)i_function;
    തിരികെ m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
