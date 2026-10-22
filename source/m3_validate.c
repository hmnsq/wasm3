//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#ଅନ୍ତର୍ଭାଵ "m3_validate.h"
#ଅନ୍ତର୍ଭାଵ "m3_exception.h"
#ଅନ୍ତର୍ଭାଵ "m3_info.h"

#ଯଦି d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_ଗଣନା;
    u16         result_ଗଣନା;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

ସଂରଚନା ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_ଗଦା];
    u16         opd_ଶୀର୍ଷ;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrl_ଶୀର୍ଷ;

    m3type_t    localTypes [d_m3_Val_ଗଦା];
    u16         ଅଙ୍କ_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
ସ୍ଥିର bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    ଫେରନ୍ତୁ v->module and i_memoryIdx < v->module->ଅଙ୍କ_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
ସ୍ଥିର m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    ଫେରନ୍ତୁ Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
ସ୍ଥିର bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    ଫେରନ୍ତୁ v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
ସ୍ଥିର m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    ଫେରନ୍ତୁ Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
ସ୍ଥିର u32 v_max_align (m3opcode_t opcode)
{
    ବାଛ (opcode) {
    ସ୍ଥିତି 0x2c: ସ୍ଥିତି 0x2d:   // i32.load8_s, i32.load8_u
    ସ୍ଥିତି 0x30: ସ୍ଥିତି 0x31:   // i64.load8_s, i64.load8_u
    ସ୍ଥିତି 0x3a:              // i32.store8
    ସ୍ଥିତି 0x3c:              // i64.store8
        ଫେରନ୍ତୁ 0;
    ସ୍ଥିତି 0x2e: ସ୍ଥିତି 0x2f:   // i32.load16_s, i32.load16_u
    ସ୍ଥିତି 0x32: ସ୍ଥିତି 0x33:   // i64.load16_s, i64.load16_u
    ସ୍ଥିତି 0x3b:              // i32.store16
    ସ୍ଥିତି 0x3d:              // i64.store16
        ଫେରନ୍ତୁ 1;
    ସ୍ଥିତି 0x29:              // i64.load
    ସ୍ଥିତି 0x2b:              // f64.load
    ସ୍ଥିତି 0x37:              // i64.store
    ସ୍ଥିତି 0x39:              // f64.store
        ଫେରନ୍ତୁ 3;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ:                // 32-bit accesses, and a safe fallback
        ଫେରନ୍ତୁ 2;
    }
}

#ଯଦି d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
ସ୍ଥିର bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_ଅଙ୍କ_Operands, m3type_t * o_ମୂଲ୍ୟ_Type, m3type_t * o_resultType)
{
    ସ୍ଥିର ଅଚଳ u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    ସ୍ଥିର ଅଚଳ m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    ଯଦି (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_ଅଙ୍କ_Operands = (sub == 0x00) ? 2 : 3;
        *o_ମୂଲ୍ୟ_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        ଫେରନ୍ତୁ true;
    }

    ଯଦି (sub < 0x10 or sub > c_waOp_lastAtomic) ଫେରନ୍ତୁ false;

    ଯଦି (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_ଅଙ୍କ_Operands = 1;
    } ଅନ୍ୟଥା ଯଦି (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_ଅଙ୍କ_Operands = 2;
    } ଅନ୍ୟଥା ଯଦି (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_ଅଙ୍କ_Operands = 2;
    } ଅନ୍ୟଥା {                        // cmpxchg
        variant = sub - 0x48;       *o_ଅଙ୍କ_Operands = 3;
    }

    *o_width      = c_widths [variant];
    *o_ମୂଲ୍ୟ_Type  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    ଫେରନ୍ତୁ true;
}

#endif

// ---------- Operand stack ----------

ସ୍ଥିର M3Result v_ଠେଲ (ValCtx * v, m3type_t type)
{
    ଯଦି (M3_UNLIKELY(v->opd_ଶୀର୍ଷ >= d_m3_Val_ଗଦା))
        ଫେରନ୍ତୁ m3_Err_function_ଗଦା_Overflow;
    v->opd[v->opd_ଶୀର୍ଷ++] = type;
    ଫେରନ୍ତୁ m3Err_none;
}

ସ୍ଥିର M3Result v_କାଢ଼ (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1];
    ଯଦି (M3_UNLIKELY(v->opd_ଶୀର୍ଷ == f->height)) {
        ଯଦି (f->is_unreachable) { *o_type = c_valBottom; ଫେରନ୍ତୁ m3Err_none; }
        ଫେରନ୍ତୁ m3_Err_function_ଗଦା_Underrun;
    }
    *o_type = v->opd[--v->opd_ଶୀର୍ଷ];
    ଫେରନ୍ତୁ m3Err_none;
}

ସ୍ଥିର M3Result v_କାଢ଼_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_କାଢ଼(v, &actual);
    ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    ଯଦି (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        ଫେରନ୍ତୁ m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    ଫେରନ୍ତୁ m3Err_none;
}

// ---------- Control stack ----------

ସ୍ଥିର M3Result v_ଠେଲ_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    ଯଦି (M3_UNLIKELY(v->ctrl_ଶୀର୍ଷ >= d_m3ValCtrlDepth))
        ଫେରନ୍ତୁ m3_Err_function_ଗଦା_Overflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ଶୀର୍ଷ++];
    f->opcode       = op;
    f->type         = type;
    f->param_ଗଣନା  = type ? type->ଅଙ୍କ_Args : 0;
    f->result_ଗଣନା = type ? type->ଅଙ୍କ_Rets : 0;
    f->height       = v->opd_ଶୀର୍ଷ;
    f->is_unreachable = false;
    ଫେରନ୍ତୁ m3Err_none;
}

ସ୍ଥିର M3Result v_କାଢ଼_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    ଯଦି (M3_UNLIKELY(v->ctrl_ଶୀର୍ଷ == 0))
        ଫେରନ୍ତୁ m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1];
    // pop result types
    ଯଦି (f->type) {
        ପାଇଁ (u16 i = f->result_ଗଣନା; i > 0; i--) {
            m3type_t a;
            M3Result r = v_କାଢ଼_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
        }
    }
    ଯଦି (M3_UNLIKELY(v->opd_ଶୀର୍ଷ != f->height))
        ଫେରନ୍ତୁ m3_Err_type_ଗଣନା_Mismatch;
    ଯଦି (o_frame) *o_frame = *f;
    v->ctrl_ଶୀର୍ଷ--;
    ଫେରନ୍ତୁ m3Err_none;
}

ସ୍ଥିର ଶୂନ୍ୟ v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1];
    v->opd_ଶୀର୍ଷ = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
ସ୍ଥିର u16 v_label_n (ValCtrlFrame * f)
{
    ଫେରନ୍ତୁ (f->opcode == 0x03) ? f->param_ଗଣନା : f->result_ଗଣନା;
}

ସ୍ଥିର m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    ଯଦି (!f->type) ଫେରନ୍ତୁ c_m3Type_none;
    ଯଦି (f->opcode == 0x03)
        ଫେରନ୍ତୁ BaseTypeOf(f->type->types[f->type->ଅଙ୍କ_Rets + i]); // params
    ଫେରନ୍ତୁ BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
ସ୍ଥିର m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1];

    ଯଦି (v->opd_ଶୀର୍ଷ <= i_depth || (u16)(v->opd_ଶୀର୍ଷ - i_depth - 1) < f->height)
        ଫେରନ୍ତୁ c_valBottom;

    ଫେରନ୍ତୁ v->opd[v->opd_ଶୀର୍ଷ - i_depth - 1];
}

// Pop label types for branch target
ସ୍ଥିର M3Result v_କାଢ଼_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    ପାଇଁ (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_କାଢ଼_expect(v, v_label_t(tgt, i - 1), &a);
        ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    }
    ଫେରନ୍ତୁ m3Err_none;
}

// Push label types back
ସ୍ଥିର M3Result v_ଠେଲ_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    ପାଇଁ (u16 i = 0; i < n; i++) {
        M3Result r = v_ଠେଲ(v, v_label_t(tgt, i));
        ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    }
    ଫେରନ୍ତୁ m3Err_none;
}

// ---------- Block type resolution ----------

ସ୍ଥିର M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    ଯଦି (v->wasm >= v->wasmEnd)
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;

#ଯଦି d_m3HasTypedRefs
    ଯଦି (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = Parse_ମୂଲ୍ୟ_Type(v->module, &refType, &v->wasm, v->wasmEnd);
        ଯଦି (rr) ଫେରନ୍ତୁ rr;
        IM3FuncType ftype = ଖାଲି;
        rr = AllocFuncType(&ftype, 1);
        ଯଦି (rr) ଫେରନ୍ତୁ rr;
        ftype->ଅଙ୍କ_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        ଯଦି (rr) ଫେରନ୍ତୁ rr;
        *o_type = ftype;
        ଫେରନ୍ତୁ m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    ଯଦି (r) ଫେରନ୍ତୁ r;

    ଯଦି (type < 0) {
        ଯଦି (type < -64) ଫେରନ୍ତୁ m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        ଯଦି (r) ଫେରନ୍ତୁ r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } ଅନ୍ୟଥା {
        ଯଦି ((u32)type >= v->module->ଅଙ୍କ_Func_Types)
            ଫେରନ୍ତୁ m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    ଫେରନ୍ତୁ m3Err_none;
}

// ---------- Convenience ----------

ସ୍ଥିର M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_କାଢ଼_expect(v, in, &a);
    ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    ଫେରନ୍ତୁ v_ଠେଲ(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
ସ୍ଥିର M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ଖାଲି;

    u16 ଅଙ୍କ_Callee_Rets = i_calleeType ? i_calleeType->ଅଙ୍କ_Rets : 0;
    u16 ଅଙ୍କ_Func_Rets   = ft ? ft->ଅଙ୍କ_Rets : 0;

    ଯଦି (ଅଙ୍କ_Callee_Rets != ଅଙ୍କ_Func_Rets)
        ଫେରନ୍ତୁ m3Err_typeMismatch;

    ପାଇଁ (u16 i = 0; i < ଅଙ୍କ_Callee_Rets; i++) {
        ଯଦି (i_calleeType->types[i] != ft->types[i])
            ଫେରନ୍ତୁ m3Err_typeMismatch;
    }

    ଫେରନ୍ତୁ m3Err_none;
}

ସ୍ଥିର M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_କାଢ଼_expect(v, t, &a); ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    r = v_କାଢ଼_expect(v, t, &a); ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    ଫେରନ୍ତୁ v_ଠେଲ(v, t);
}

ସ୍ଥିର M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_କାଢ଼_expect(v, t, &a); ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    r = v_କାଢ଼_expect(v, t, &a); ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;
    ଫେରନ୍ତୁ v_ଠେଲ(v, c_m3Type_i32);
}

ସ୍ଥିର M3Result v_testop (ValCtx * v, m3type_t t)
{
    ଫେରନ୍ତୁ v_unop(v, t, c_m3Type_i32);
}

ସ୍ଥିର M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    ଫେରନ୍ତୁ v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#ଯଦି d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
ସ୍ଥିର M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    ଯଦି (r) ଫେରନ୍ତୁ r;
    ଯଦି (kind > 0x03) ଫେରନ୍ତୁ m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = ଖାଲି;
    ଯଦି (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        ଯଦି (r) ଫେରନ୍ତୁ r;
        ଯଦି (!v->module || tagIndex >= v->module->ଅଙ୍କ_Tags) ଫେରନ୍ତୁ m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    ଯଦି (r) ଫେରନ୍ତୁ r;
    ଯଦି (depth >= v->ctrl_ଶୀର୍ଷ) ଫେରନ୍ତୁ m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1 - depth];

    u16 ଅଙ୍କ_Payload = tagType ? tagType->ଅଙ୍କ_Args : 0;
    u16 ଅଙ୍କ_Label   = v_label_n(tgt);

    ଯଦି (ଅଙ୍କ_Label != ଅଙ୍କ_Payload + (hasRef ? 1u : 0u))
        ଫେରନ୍ତୁ m3_Err_type_ଗଣନା_Mismatch;

    ପାଇଁ (u16 i = 0; i < ଅଙ୍କ_Payload; i++) {
        ଯଦି (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->ଅଙ୍କ_Rets + i]))
            ଫେରନ୍ତୁ m3Err_typeMismatch;
    }

    ଯଦି (hasRef and v_label_t(tgt, ଅଙ୍କ_Payload) != c_m3Type_exnref)
        ଫେରନ୍ତୁ m3Err_typeMismatch;

    ଫେରନ୍ତୁ m3Err_none;
}

#endif // d_m3HasExceptionHandling

#ଯଦି d_m3_Has_ଗଦା_Switching

ସ୍ଥିର M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    ଯଦି (kind > 0x01) ଫେରନ୍ତୁ m3Err_wasmMalformed;
    ଯଦି (!v->module || tagIdx >= v->module->ଅଙ୍କ_Tags) ଫେରନ୍ତୁ m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    ଯଦି (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
        ଯଦି (depth >= v->ctrl_ଶୀର୍ଷ) ଫେରନ୍ତୁ m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1 - depth];
        u16 ଅଙ୍କ_Payload = tagType ? tagType->ଅଙ୍କ_Args : 0;
        u16 ଅଙ୍କ_Label   = v_label_n(tgt);

        ଯଦି (ଅଙ୍କ_Label != ଅଙ୍କ_Payload + 1u) ଫେରନ୍ତୁ m3_Err_type_ଗଣନା_Mismatch;

        ପାଇଁ (u16 j = 0; j < ଅଙ୍କ_Payload; j++) {
            ଯଦି (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->ଅଙ୍କ_Rets + j]))
                ଫେରନ୍ତୁ m3Err_typeMismatch;
        }
        ଯଦି (v_label_t(tgt, ଅଙ୍କ_Payload) != c_m3Type_contref)
            ଫେରନ୍ତୁ m3Err_typeMismatch;

#ଯଦି d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->ଅଙ୍କ_Rets + ଅଙ୍କ_Payload]
            : tgt->type->types[ଅଙ୍କ_Payload];

        ଯଦି (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            ଫେରନ୍ତୁ m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = ଖାଲି;
        ପାଇଁ (u32 idx = 0; idx < v->module->ଅଙ୍କ_Func_Types; ++idx) {
            ଯଦି (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (!labelCt) ଫେରନ୍ତୁ m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_ଅଙ୍କ_Params = tagType ? tagType->ଅଙ୍କ_Rets : 0;
        ଯଦି (labelFt->ଅଙ୍କ_Args != expected_ଅଙ୍କ_Params) ଫେରନ୍ତୁ m3Err_typeMismatch;
        ପାଇଁ (u16 p = 0; p < expected_ଅଙ୍କ_Params; ++p) {
            ଯଦି (labelFt->types[labelFt->ଅଙ୍କ_Rets + p] != tagType->types[p])
                ଫେରନ୍ତୁ m3Err_typeMismatch;
        }

        u16 expected_ଅଙ୍କ_Rets = ft ? ft->ଅଙ୍କ_Rets : 0;
        ଯଦି (labelFt->ଅଙ୍କ_Rets != expected_ଅଙ୍କ_Rets) ଫେରନ୍ତୁ m3Err_typeMismatch;
        ପାଇଁ (u16 r_idx = 0; r_idx < expected_ଅଙ୍କ_Rets; ++r_idx) {
            ଯଦି (labelFt->types[r_idx] != ft->types[r_idx])
                ଫେରନ୍ତୁ m3Err_typeMismatch;
        }
#endif
    } ଅନ୍ୟଥା {
        ଯଦି (tagType && tagType->ଅଙ୍କ_Args != 0) ଫେରନ୍ତୁ m3Err_typeMismatch;
    }
    ଫେରନ୍ତୁ m3Err_none;
}

#endif // d_m3HasStackSwitching

ସ୍ଥିର M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    ଯେତେବେଳେ (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        ଯଦି (M3_UNLIKELY(r)) ଫେରନ୍ତୁ r;

        ବାଛ (opcode)
        {
        // ---- Control ----
        ସ୍ଥିତି 0x00: // unreachable
            v_unreachable(v);
            ଭାଙ୍ଗ;

        ସ୍ଥିତି 0x01: // nop
            ଭାଙ୍ଗ;

        ସ୍ଥିତି 0x02: // block
        ସ୍ଥିତି 0x03: // loop
        ସ୍ଥିତି 0x04: // if
        {
            IM3FuncType bt = ଖାଲି;
            r = v_read_blocktype(v, &bt);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (opcode == 0x04) {
                r = v_କାଢ଼_expect(v, c_m3Type_i32, &a);
                ଯଦି (r) ଫେରନ୍ତୁ r;
            }
            // Pop block params from caller stack
            ଯଦି (bt) {
                ପାଇଁ (u16 i = bt->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(bt->types[bt->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            r = v_ଠେଲ_ctrl(v, opcode, bt);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            // Push params inside block
            ଯଦି (bt) {
                ପାଇଁ (u16 i = 0; i < bt->ଅଙ୍କ_Args; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(bt->types[bt->ଅଙ୍କ_Rets + i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

#ଯଦି d_m3HasExceptionHandling
        ସ୍ଥିତି 0x1f: // try_table
        {
            IM3FuncType bt = ଖାଲି;
            r = v_read_blocktype(v, &bt);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (bt) {
                ପାଇଁ (u16 i = bt->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(bt->types[bt->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 ଅଙ୍କ_Catch;
            r = ReadLEB_u32(&ଅଙ୍କ_Catch, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ପାଇଁ (u32 i = 0; i < ଅଙ୍କ_Catch; i++) {
                r = v_catch_clause(v);
                ଯଦି (r) ଫେରନ୍ତୁ r;
            }

            r = v_ଠେଲ_ctrl(v, opcode, bt);
            ଯଦି (r) ଫେରନ୍ତୁ r;

            ଯଦି (bt) {
                ପାଇଁ (u16 i = 0; i < bt->ଅଙ୍କ_Args; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(bt->types[bt->ଅଙ୍କ_Rets + i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (!v->module || tagIndex >= v->module->ଅଙ୍କ_Tags) ଫେରନ୍ତୁ m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            ଯଦି (tt) {
                ପାଇଁ (u16 i = tt->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(tt->types[tt->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            v_unreachable(v);
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x0a: // throw_ref
            r = v_କାଢ଼_expect(v, c_m3Type_exnref, &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            v_unreachable(v);
            ଭାଙ୍ଗ;
#endif // d_m3HasExceptionHandling

        ସ୍ଥିତି 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_କାଢ଼_ctrl(v, &frame);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (frame.opcode != 0x04)
                ଫେରନ୍ତୁ m3Err_wasmMalformed;
            r = v_ଠେଲ_ctrl(v, 0x05, frame.type);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (frame.type) {
                ପାଇଁ (u16 i = 0; i < frame.type->ଅଙ୍କ_Args; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(frame.type->types[frame.type->ଅଙ୍କ_Rets + i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_କାଢ଼_ctrl(v, &frame);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            // Push results
            ଯଦି (frame.type) {
                ପାଇଁ (u16 i = 0; i < frame.result_ଗଣନା; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(frame.type->types[i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            // If this was the outermost frame, we're done
            ଯଦି (v->ctrl_ଶୀର୍ଷ == 0)
                ଫେରନ୍ତୁ m3Err_none;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (depth >= v->ctrl_ଶୀର୍ଷ) ଫେରନ୍ତୁ m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1 - depth];
            r = v_କାଢ଼_labels(v, tgt);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            v_unreachable(v);
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (depth >= v->ctrl_ଶୀର୍ଷ) ଫେରନ୍ତୁ m3Err_unknownLabel;
            r = v_କାଢ଼_expect(v, c_m3Type_i32, &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1 - depth];
            r = v_କାଢ଼_labels(v, tgt);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ_labels(v, tgt);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x0e: // br_table
        {
            u32 ଗଣନା_2;
            r = ReadLEB_u32(&ଗଣନା_2, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            ପାଇଁ (u32 i = 0; i <= ଗଣନା_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                ଯଦି (r) ଫେରନ୍ତୁ r;
                ଯଦି (d >= v->ctrl_ଶୀର୍ଷ) ଫେରନ୍ତୁ m3Err_unknownLabel;
                ଯଦି (i == ଗଣନା_2) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_କାଢ଼_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            ଯଦି (r) ଫେରନ୍ତୁ r;

            ପାଇଁ (u32 i = 0; i <= ଗଣନା_2; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                ଯଦି (r) ଫେରନ୍ତୁ r;
                ValCtrlFrame * t = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1 - d];
                u16 n = v_label_n(t);
                ଯଦି (n != arity) ଫେରନ୍ତୁ m3_Err_type_ଗଣନା_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                ପାଇଁ (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    ଯଦି (want != c_valBottom && have != c_valBottom && want != have)
                        ଫେରନ୍ତୁ m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrl_ଶୀର୍ଷ - 1 - defDepth];
            r = v_କାଢ଼_labels(v, dt);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            v_unreachable(v);
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = ft->ଅଙ୍କ_Rets; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            v_unreachable(v);
            ଭାଙ୍ଗ;
        }

        // ---- Call ----
        ସ୍ଥିତି 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (idx >= v->module->ଅଙ୍କ_Functions) ଫେରନ୍ତୁ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = ft->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[ft->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
                ପାଇଁ (u16 i = 0; i < ft->ଅଙ୍କ_Rets; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(ft->types[i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ଯଦି (tableIdx >= v->module->ଅଙ୍କ_Tables) ଫେରନ୍ତୁ m3Err_unknownTable;
            ଯଦି (v->module->tables[tableIdx]->type != c_m3Type_funcref) ଫେରନ୍ତୁ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_କାଢ଼_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = ft->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[ft->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
                ପାଇଁ (u16 i = 0; i < ft->ଅଙ୍କ_Rets; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(ft->types[i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (idx >= v->module->ଅଙ୍କ_Functions) ଫେରନ୍ତୁ m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = ft->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[ft->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            v_unreachable(v);
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            // the table must exist and hold funcrefs
            ଯଦି (tableIdx >= v->module->ଅଙ୍କ_Tables) ଫେରନ୍ତୁ m3Err_unknownTable;
            ଯଦି (v->module->tables[tableIdx]->type != c_m3Type_funcref) ଫେରନ୍ତୁ m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_କାଢ଼_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = ft->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[ft->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            v_unreachable(v);
            ଭାଙ୍ଗ;
        }

#ଯଦି d_m3HasTypedRefs
        ସ୍ଥିତି 0x14: // call_ref
        ସ୍ଥିତି 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            ଯଦି (isTail) {
                r = v_check_tail_results(v, ft);
                ଯଦି (r) ଫେରନ୍ତୁ r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_କାଢ଼_expect(v, c_m3Type_funcref, &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = ft->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[ft->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
                ଯଦି (not isTail) {
                    ପାଇଁ (u16 i = 0; i < ft->ଅଙ୍କ_Rets; i++) {
                        r = v_ଠେଲ(v, BaseTypeOf(ft->types[i]));
                        ଯଦି (r) ଫେରନ୍ତୁ r;
                    }
                }
            }
            ଯଦି (isTail) v_unreachable(v);
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xd4: // ref.as_non_null
        {
            r = v_କାଢ଼_expect(v, c_m3Type_funcref, &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        ସ୍ଥିତି 0x1a: // drop
            r = v_କାଢ଼(v, &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;

        ସ୍ଥିତି 0x1b: // select
        {
            r = v_କାଢ଼_expect(v, c_m3Type_i32, &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            m3type_t t2 = c_valBottom;
            r = v_କାଢ଼(v, &t2);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            m3type_t t1;
            r = v_କାଢ଼_expect(v, t2, &t1);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            // untyped select is numeric only; references need the 0x1c form
            ଯଦି (t2 != c_valBottom && IsRefType(t2)) ଫେରନ୍ତୁ m3Err_typeMismatch;
            r = v_ଠେଲ(v, (t2 == c_valBottom) ? t1 : t2);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

#ଯଦି d_m3HasRefTypes
        ସ୍ଥିତି 0x1c: // select with an explicit result type
        {
            u32 ଅଙ୍କ_Types;
            r = ReadLEB_u32(&ଅଙ୍କ_Types, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (ଅଙ୍କ_Types != 1) ଫେରନ୍ତୁ m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_ମୂଲ୍ୟ_Type(v->module, &selType, &v->wasm, v->wasmEnd);  ଯଦି (r) ଫେରନ୍ତୁ r;
            u8 t = BaseTypeOf(selType);

            r = v_କାଢ଼_expect(v, c_m3Type_i32, &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_କାଢ଼_expect(v, t, &a);            ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_କାଢ଼_expect(v, t, &a);            ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ(v, t);                      ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x25: // table.get
        ସ୍ଥିତି 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (tableIdx >= v->module->ଅଙ୍କ_Tables) ଫେରନ୍ତୁ m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            ଯଦି (opcode == 0x26) {
                r = v_କାଢ଼_expect(v, t, &a);   ଯଦି (r) ଫେରନ୍ତୁ r;
                r = v_କାଢ଼_expect(v, at, &a);  ଯଦି (r) ଫେରନ୍ତୁ r;
            } ଅନ୍ୟଥା {
                r = v_କାଢ଼_expect(v, at, &a);  ଯଦି (r) ଫେରନ୍ତୁ r;
                r = v_ଠେଲ(v, t);             ଯଦି (r) ଫେରନ୍ତୁ r;
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xd0: // ref.null
        {
#ଯଦି d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  ଯଦି (r) ଫେରନ୍ତୁ r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# ଯଦି d_m3_Has_ଗଦା_Switching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#ଅନ୍ୟଥା
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   ଯଦି (r) ଫେରନ୍ତୁ r;
            r = NormalizeType(&t, waType);                   ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (!IsRefType(t)) ଫେରନ୍ତୁ m3Err_wasmMalformed;
#endif
            r = v_ଠେଲ(v, t);                                ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_କାଢ଼(v, &t);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (t != c_valBottom && !IsRefType(t)) ଫେରନ୍ତୁ m3Err_typeMismatch;
            r = v_ଠେଲ(v, c_m3Type_i32);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (funcIdx >= v->module->ଅଙ୍କ_Functions) ଫେରନ୍ତୁ m3Err_unknownFunction;
            ଯଦି (!Module_IsFunctionDeclared(v->module, funcIdx)) ଫେରନ୍ତୁ m3Err_undeclaredFuncRef;
            r = v_ଠେଲ(v, c_m3Type_funcref);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }
#endif

        // ---- Variable ----
        ସ୍ଥିତି 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (idx >= v->ଅଙ୍କ_Locals) ଫେରନ୍ତୁ m3Err_unknownLocal;
            r = v_ଠେଲ(v, v->localTypes[idx]);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (idx >= v->ଅଙ୍କ_Locals) ଫେରନ୍ତୁ m3Err_unknownLocal;
            r = v_କାଢ଼_expect(v, v->localTypes[idx], &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (idx >= v->ଅଙ୍କ_Locals) ଫେରନ୍ତୁ m3Err_unknownLocal;
            r = v_କାଢ଼_expect(v, v->localTypes[idx], &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ(v, v->localTypes[idx]);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (idx >= v->module->ଅଙ୍କ_Globals) ଫେରନ୍ତୁ m3Err_unknownGlobal;
            r = v_ଠେଲ(v, BaseTypeOf(v->module->globals[idx].type));
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (idx >= v->module->ଅଙ୍କ_Globals) ଫେରନ୍ତୁ m3Err_unknownGlobal;
            r = v_କାଢ଼_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        // ---- Memory load ----
        ସ୍ଥିତି 0x28: ସ୍ଥିତି 0x29: ସ୍ଥିତି 0x2a: ସ୍ଥିତି 0x2b: // i32/i64/f32/f64.load
        ସ୍ଥିତି 0x2c: ସ୍ଥିତି 0x2d: ସ୍ଥିତି 0x2e: ସ୍ଥିତି 0x2f: // i32.load8/16 s/u
        ସ୍ଥିତି 0x30: ସ୍ଥିତି 0x31: ସ୍ଥିତି 0x32: ସ୍ଥିତି 0x33: // i64.load8/16 s/u
        ସ୍ଥିତି 0x34: ସ୍ଥିତି 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (align > v_max_align(opcode)) ଫେରନ୍ତୁ m3Err_invalidAlignment;
            ଯଦି (not v_has_memory_idx(v, memidx)) ଫେରନ୍ତୁ m3Err_unknownMemory;
            ଯଦି (not v_offset_in_range(v, memidx, offset)) ଫେରନ୍ତୁ m3Err_wasmMalformed;
            r = v_କାଢ଼_expect(v, v_memory_addrtype(v, memidx), &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            u8 result;
            ଯଦି      (opcode == 0x28) result = c_m3Type_i32;
            ଅନ୍ୟଥା ଯଦି (opcode == 0x29) result = c_m3Type_i64;
            ଅନ୍ୟଥା ଯଦି (opcode == 0x2a) result = c_m3Type_f32;
            ଅନ୍ୟଥା ଯଦି (opcode == 0x2b) result = c_m3Type_f64;
            ଅନ୍ୟଥା ଯଦି (opcode <= 0x2f) result = c_m3Type_i32;
            ଅନ୍ୟଥା                     result = c_m3Type_i64;
            r = v_ଠେଲ(v, result);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        // ---- Memory store ----
        ସ୍ଥିତି 0x36: ସ୍ଥିତି 0x37: ସ୍ଥିତି 0x38: ସ୍ଥିତି 0x39: // i32/i64/f32/f64.store
        ସ୍ଥିତି 0x3a: ସ୍ଥିତି 0x3b:                         // i32.store8/16
        ସ୍ଥିତି 0x3c: ସ୍ଥିତି 0x3d: ସ୍ଥିତି 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (align > v_max_align(opcode)) ଫେରନ୍ତୁ m3Err_invalidAlignment;
            ଯଦି (not v_has_memory_idx(v, memidx)) ଫେରନ୍ତୁ m3Err_unknownMemory;
            ଯଦି (not v_offset_in_range(v, memidx, offset)) ଫେରନ୍ତୁ m3Err_wasmMalformed;
            u8 valtype;
            ଯଦି      (opcode == 0x36) valtype = c_m3Type_i32;
            ଅନ୍ୟଥା ଯଦି (opcode == 0x37) valtype = c_m3Type_i64;
            ଅନ୍ୟଥା ଯଦି (opcode == 0x38) valtype = c_m3Type_f32;
            ଅନ୍ୟଥା ଯଦି (opcode == 0x39) valtype = c_m3Type_f64;
            ଅନ୍ୟଥା ଯଦି (opcode <= 0x3b) valtype = c_m3Type_i32;
            ଅନ୍ୟଥା                     valtype = c_m3Type_i64;
            r = v_କାଢ଼_expect(v, valtype, &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_କାଢ଼_expect(v, v_memory_addrtype(v, memidx), &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        // ---- Memory size/grow ----
        ସ୍ଥିତି 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (not v_has_memory_idx(v, memidx)) ଫେରନ୍ତୁ m3Err_unknownMemory;
            r = v_ଠେଲ(v, v_memory_addrtype(v, memidx)); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (not v_has_memory_idx(v, memidx)) ଫେରନ୍ତୁ m3Err_unknownMemory;
            r = v_କାଢ଼_expect(v, v_memory_addrtype(v, memidx), &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ(v, v_memory_addrtype(v, memidx)); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        // ---- Constants ----
        ସ୍ଥିତି 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ(v, c_m3Type_i32); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ(v, c_m3Type_i64); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି 0x43: { // f32.const
            ଯଦି (v->wasm + 4 > v->wasmEnd) ଫେରନ୍ତୁ m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_ଠେଲ(v, c_m3Type_f32); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }
        ସ୍ଥିତି 0x44: { // f64.const
            ଯଦି (v->wasm + 8 > v->wasmEnd) ଫେରନ୍ତୁ m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_ଠେଲ(v, c_m3Type_f64); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }


        // ---- i32 comparison ----
        ସ୍ଥିତି 0x45: r = v_testop(v, c_m3Type_i32); ଭାଙ୍ଗ; // i32.eqz
        ସ୍ଥିତି 0x46: ସ୍ଥିତି 0x47: ସ୍ଥିତି 0x48: ସ୍ଥିତି 0x49: ସ୍ଥିତି 0x4a:
        ସ୍ଥିତି 0x4b: ସ୍ଥିତି 0x4c: ସ୍ଥିତି 0x4d: ସ୍ଥିତି 0x4e: ସ୍ଥିତି 0x4f:
            r = v_relop(v, c_m3Type_i32); ଭାଙ୍ଗ;

        // ---- i64 comparison ----
        ସ୍ଥିତି 0x50: r = v_testop(v, c_m3Type_i64); ଭାଙ୍ଗ; // i64.eqz
        ସ୍ଥିତି 0x51: ସ୍ଥିତି 0x52: ସ୍ଥିତି 0x53: ସ୍ଥିତି 0x54: ସ୍ଥିତି 0x55:
        ସ୍ଥିତି 0x56: ସ୍ଥିତି 0x57: ସ୍ଥିତି 0x58: ସ୍ଥିତି 0x59: ସ୍ଥିତି 0x5a:
            r = v_relop(v, c_m3Type_i64); ଭାଙ୍ଗ;

        // ---- f32 comparison ----
        ସ୍ଥିତି 0x5b: ସ୍ଥିତି 0x5c: ସ୍ଥିତି 0x5d: ସ୍ଥିତି 0x5e: ସ୍ଥିତି 0x5f: ସ୍ଥିତି 0x60:
            r = v_relop(v, c_m3Type_f32); ଭାଙ୍ଗ;

        // ---- f64 comparison ----
        ସ୍ଥିତି 0x61: ସ୍ଥିତି 0x62: ସ୍ଥିତି 0x63: ସ୍ଥିତି 0x64: ସ୍ଥିତି 0x65: ସ୍ଥିତି 0x66:
            r = v_relop(v, c_m3Type_f64); ଭାଙ୍ଗ;

        // ---- i32 unary ----
        ସ୍ଥିତି 0x67: ସ୍ଥିତି 0x68: ସ୍ଥିତି 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ଭାଙ୍ଗ;

        // ---- i32 binary ----
        ସ୍ଥିତି 0x6a: ସ୍ଥିତି 0x6b: ସ୍ଥିତି 0x6c: ସ୍ଥିତି 0x6d: ସ୍ଥିତି 0x6e: ସ୍ଥିତି 0x6f:
        ସ୍ଥିତି 0x70: ସ୍ଥିତି 0x71: ସ୍ଥିତି 0x72: ସ୍ଥିତି 0x73: ସ୍ଥିତି 0x74: ସ୍ଥିତି 0x75:
        ସ୍ଥିତି 0x76: ସ୍ଥିତି 0x77: ସ୍ଥିତି 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); ଭାଙ୍ଗ;

        // ---- i64 unary ----
        ସ୍ଥିତି 0x79: ସ୍ଥିତି 0x7a: ସ୍ଥିତି 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ଭାଙ୍ଗ;

        // ---- i64 binary ----
        ସ୍ଥିତି 0x7c: ସ୍ଥିତି 0x7d: ସ୍ଥିତି 0x7e: ସ୍ଥିତି 0x7f: ସ୍ଥିତି 0x80: ସ୍ଥିତି 0x81:
        ସ୍ଥିତି 0x82: ସ୍ଥିତି 0x83: ସ୍ଥିତି 0x84: ସ୍ଥିତି 0x85: ସ୍ଥିତି 0x86: ସ୍ଥିତି 0x87:
        ସ୍ଥିତି 0x88: ସ୍ଥିତି 0x89: ସ୍ଥିତି 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); ଭାଙ୍ଗ;

        // ---- f32 unary ----
        ସ୍ଥିତି 0x8b: ସ୍ଥିତି 0x8c: ସ୍ଥିତି 0x8d: ସ୍ଥିତି 0x8e: ସ୍ଥିତି 0x8f:
        ସ୍ଥିତି 0x90: ସ୍ଥିତି 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); ଭାଙ୍ଗ;

        // ---- f32 binary ----
        ସ୍ଥିତି 0x92: ସ୍ଥିତି 0x93: ସ୍ଥିତି 0x94: ସ୍ଥିତି 0x95: ସ୍ଥିତି 0x96:
        ସ୍ଥିତି 0x97: ସ୍ଥିତି 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); ଭାଙ୍ଗ;

        // ---- f64 unary ----
        ସ୍ଥିତି 0x99: ସ୍ଥିତି 0x9a: ସ୍ଥିତି 0x9b: ସ୍ଥିତି 0x9c: ସ୍ଥିତି 0x9d:
        ସ୍ଥିତି 0x9e: ସ୍ଥିତି 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); ଭାଙ୍ଗ;

        // ---- f64 binary ----
        ସ୍ଥିତି 0xa0: ସ୍ଥିତି 0xa1: ସ୍ଥିତି 0xa2: ସ୍ଥିତି 0xa3: ସ୍ଥିତି 0xa4:
        ସ୍ଥିତି 0xa5: ସ୍ଥିତି 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); ଭାଙ୍ଗ;

        // ---- Conversions ----
        ସ୍ଥିତି 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); ଭାଙ୍ଗ; // i32.wrap/i64
        ସ୍ଥିତି 0xa8: ସ୍ଥିତି 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xaa: ସ୍ଥିତି 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xac: ସ୍ଥିତି 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xae: ସ୍ଥିତି 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xb0: ସ୍ଥିତି 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xb2: ସ୍ଥିତି 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xb4: ସ୍ଥିତି 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xb7: ସ୍ଥିତି 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xb9: ସ୍ଥିତି 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ଭାଙ୍ଗ;

        // ---- Sign-extension (MVP post) ----
        ସ୍ଥିତି 0xc0: ସ୍ଥିତି 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ଭାଙ୍ଗ;
        ସ୍ଥିତି 0xc2: ସ୍ଥିତି 0xc3: ସ୍ଥିତି 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ଭାଙ୍ଗ;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        ସ୍ଥିତି 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;
            ବାଛ (sub) {
            ସ୍ଥିତି 0x00: ସ୍ଥିତି 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ଭାଙ୍ଗ;
            ସ୍ଥିତି 0x02: ସ୍ଥିତି 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ଭାଙ୍ଗ;
            ସ୍ଥିତି 0x04: ସ୍ଥିତି 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ଭାଙ୍ଗ;
            ସ୍ଥିତି 0x06: ସ୍ଥିତି 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ଭାଙ୍ଗ;
            ସ୍ଥିତି 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଯଦି (not v_has_memory_idx(v, memidx)) ଫେରନ୍ତୁ m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                ଯଦି (not v->module->has_Data_ଗଣନା) ଫେରନ୍ତୁ m3_Err_data_ଗଣନା_Required;
                ଯଦି (dataidx >= v->module->ଅଙ୍କ_Data_Segments) ଫେରନ୍ତୁ m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_କାଢ଼_expect(v, c_m3Type_i32, &a); ଯଦି (r) ଫେରନ୍ତୁ r; // n
                r = v_କାଢ଼_expect(v, c_m3Type_i32, &a); ଯଦି (r) ଫେରନ୍ତୁ r; // src
                r = v_କାଢ଼_expect(v, v_memory_addrtype(v, memidx), &a); ଯଦି (r) ଫେରନ୍ତୁ r; // dst
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                // needs the segment, but not a memory
                ଯଦି (not v->module->has_Data_ଗଣନା) ଫେରନ୍ତୁ m3_Err_data_ଗଣନା_Required;
                ଯଦି (dataidx >= v->module->ଅଙ୍କ_Data_Segments) ଫେରନ୍ତୁ m3Err_unknownDataSegment;
                ଭାଙ୍ଗ;
            }
#ଯଦି d_m3HasRefTypes
            ସ୍ଥିତି 0x0c: // table.init
            ସ୍ଥିତି 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;

                ଯଦି (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    ଯଦି (a2 >= v->module->ଅଙ୍କ_Tables) ଫେରନ୍ତୁ m3Err_unknownTable;
                    ଯଦି (a1 >= v->module->ଅଙ୍କ_Element_Segments) ଫେରନ୍ତୁ m3Err_unknownElemSegment;
                    ଯଦି (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        ଫେରନ୍ତୁ m3Err_typeMismatch;
                } ଅନ୍ୟଥା {                                                    // dst, src
                    ଯଦି (a1 >= v->module->ଅଙ୍କ_Tables) ଫେରନ୍ତୁ m3Err_unknownTable;
                    ଯଦି (a2 >= v->module->ଅଙ୍କ_Tables) ଫେରନ୍ତୁ m3Err_unknownTable;
                    ଯଦି (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        ଫେରନ୍ତୁ m3Err_typeMismatch;
                }

                ଯଦି (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_କାଢ଼_expect(v, c_m3Type_i32, &a); ଯଦି (r) ଫେରନ୍ତୁ r;             // n
                    r = v_କାଢ଼_expect(v, c_m3Type_i32, &a); ଯଦି (r) ଫେରନ୍ତୁ r;             // s
                    r = v_କାଢ଼_expect(v, v_table_addrtype(v, a2), &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // d
                } ଅନ୍ୟଥା {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_କାଢ଼_expect(v, nType, &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // n
                    r = v_କାଢ଼_expect(v, sType, &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // s
                    r = v_କାଢ଼_expect(v, dType, &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // d
                }
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଯଦି (elemIdx >= v->module->ଅଙ୍କ_Element_Segments) ଫେରନ୍ତୁ m3Err_unknownElemSegment;
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି 0x0f: // table.grow
            ସ୍ଥିତି 0x10: // table.size
            ସ୍ଥିତି 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଯଦି (tableIdx >= v->module->ଅଙ୍କ_Tables) ଫେରନ୍ତୁ m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                ଯଦି (sub == 0x10) {                                          // table.size
                    r = v_ଠେଲ(v, at); ଯଦି (r) ଫେରନ୍ତୁ r;
                } ଅନ୍ୟଥା ଯଦି (sub == 0x0f) {                                   // table.grow
                    r = v_କାଢ଼_expect(v, at, &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // n
                    r = v_କାଢ଼_expect(v, t, &a);  ଯଦି (r) ଫେରନ୍ତୁ r;  // init
                    r = v_ଠେଲ(v, at);           ଯଦି (r) ଫେରନ୍ତୁ r;
                } ଅନ୍ୟଥା {                                                    // table.fill
                    r = v_କାଢ଼_expect(v, at, &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // n
                    r = v_କାଢ଼_expect(v, t, &a);  ଯଦି (r) ଫେରନ୍ତୁ r;  // val
                    r = v_କାଢ଼_expect(v, at, &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // i
                }
                ଭାଙ୍ଗ;
            }
#endif
            ସ୍ଥିତି 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଯଦି (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) ଫେରନ୍ତୁ m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_କାଢ଼_expect(v, lenType, &a); ଯଦି (r) ଫେରନ୍ତୁ r; // n
                r = v_କାଢ଼_expect(v, srcType, &a); ଯଦି (r) ଫେରନ୍ତୁ r; // src
                r = v_କାଢ଼_expect(v, dstType, &a); ଯଦି (r) ଫେରନ୍ତୁ r; // dst
                ଭାଙ୍ଗ;
            }
            ସ୍ଥିତି 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଯଦି (not v_has_memory_idx(v, memidx)) ଫେରନ୍ତୁ m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_କାଢ଼_expect(v, addrType, &a); ଯଦି (r) ଫେରନ୍ତୁ r;      // n
                r = v_କାଢ଼_expect(v, c_m3Type_i32, &a); ଯଦି (r) ଫେରନ୍ତୁ r;  // val
                r = v_କାଢ଼_expect(v, addrType, &a); ଯଦି (r) ଫେରନ୍ତୁ r;      // dst
                ଭାଙ୍ଗ;
            }
#ଯଦି d_m3HasWideArithmetic
            ସ୍ଥିତି 0x13: ସ୍ଥିତି 0x14: // i64.add128, i64.sub128
            ସ୍ଥିତି 0x15: ସ୍ଥିତି 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 ଅଙ୍କ_Operands = (sub <= 0x14) ? 4 : 2;
                ପାଇଁ (u32 i = 0; i < ଅଙ୍କ_Operands; ++i) {
                    r = v_କାଢ଼_expect(v, c_m3Type_i64, &a); ଯଦି (r) ଫେରନ୍ତୁ r;
                }
                r = v_ଠେଲ(v, c_m3Type_i64); ଯଦି (r) ଫେରନ୍ତୁ r;
                r = v_ଠେଲ(v, c_m3Type_i64); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଭାଙ୍ଗ;
            }
#endif
            ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                ଭାଙ୍ଗ;
            }
            ଭାଙ୍ଗ;
        }

#ଯଦି d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        ସ୍ଥିତି 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            ଯଦି (r) ଫେରନ୍ତୁ r;

            ଯଦି (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଯଦି (reserved != 0) ଫେରନ୍ତୁ m3Err_wasmMalformed;
                ଭାଙ୍ଗ;
            }

            u32 width, ଅଙ୍କ_Operands; m3type_t ମୂଲ୍ୟ_Type, resultType;
            ଯଦି (not v_atomic_shape(sub, &width, &ଅଙ୍କ_Operands, &ମୂଲ୍ୟ_Type, &resultType)) ଫେରନ୍ତୁ m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            ଯଦି ((1u << align) != width) ଫେରନ୍ତୁ m3Err_invalidAtomicAlignment;
            ଯଦି (not v_has_memory_idx(v, memidx)) ଫେରନ୍ତୁ m3Err_unknownMemory;
            ଯଦି (not v_offset_in_range(v, memidx, offset)) ଫେରନ୍ତୁ m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            ଯଦି (sub == 0x01 or sub == 0x02) {
                r = v_କାଢ଼_expect(v, c_m3Type_i64, &a); ଯଦି (r) ଫେରନ୍ତୁ r;
                ଅଙ୍କ_Operands--;
            }
            ପାଇଁ (u32 i = 1; i < ଅଙ୍କ_Operands; ++i) {
                r = v_କାଢ଼_expect(v, ମୂଲ୍ୟ_Type, &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            }
            r = v_କାଢ଼_expect(v, v_memory_addrtype(v, memidx), &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (resultType != c_m3Type_none) {
                r = v_ଠେଲ(v, resultType); ଯଦି (r) ଫେରନ୍ତୁ r;
            }
            ଭାଙ୍ଗ;
        }
#endif

#ଯଦି d_m3_Has_ଗଦା_Switching
        // ---- Stack Switching ----
        ସ୍ଥିତି 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            ଯଦି (!ft || !ft->isContinuation) ଫେରନ୍ତୁ m3Err_typeMismatch;
            r = v_କାଢ଼_expect(v, c_m3Type_funcref, &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_ଠେଲ(v, c_m3Type_contref); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (typeIdx1 >= v->module->ଅଙ୍କ_Func_Types || typeIdx2 >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            ଯଦି (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) ଫେରନ୍ତୁ m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 ଅଙ୍କ_Params1 = ft1->ଅଙ୍କ_Args;
            u16 ଅଙ୍କ_Params2 = ft2->ଅଙ୍କ_Args;
            ଯଦି (ଅଙ୍କ_Params1 < ଅଙ୍କ_Params2) ଫେରନ୍ତୁ m3Err_typeMismatch;
            u32 ଅଙ୍କ_Bound = ଅଙ୍କ_Params1 - ଅଙ୍କ_Params2;

            ଯଦି (ft1->ଅଙ୍କ_Rets != ft2->ଅଙ୍କ_Rets) ଫେରନ୍ତୁ m3Err_typeMismatch;
            ପାଇଁ (u16 i = 0; i < ft1->ଅଙ୍କ_Rets; ++i) {
                ଯଦି (ft1->types[i] != ft2->types[i]) ଫେରନ୍ତୁ m3Err_typeMismatch;
            }
            ପାଇଁ (u16 i = 0; i < ଅଙ୍କ_Params2; ++i) {
                ଯଦି (ft1->types[ft1->ଅଙ୍କ_Rets + ଅଙ୍କ_Bound + i] != ft2->types[ft2->ଅଙ୍କ_Rets + i]) ଫେରନ୍ତୁ m3Err_typeMismatch;
            }

            r = v_କାଢ଼_expect(v, c_m3Type_contref, &a); ଯଦି (r) ଫେରନ୍ତୁ r;

            ପାଇଁ (u32 i = ଅଙ୍କ_Bound; i > 0; i--) {
                r = v_କାଢ଼_expect(v, BaseTypeOf(ft1->types[ft1->ଅଙ୍କ_Rets + i - 1]), &a);
                ଯଦି (r) ଫେରନ୍ତୁ r;
            }

            r = v_ଠେଲ(v, c_m3Type_contref); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (!v->module || tagIdx >= v->module->ଅଙ୍କ_Tags) ଫେରନ୍ତୁ m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            ଯଦି (tt) {
                ପାଇଁ (u16 i = tt->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(tt->types[tt->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
                ପାଇଁ (u16 i = 0; i < tt->ଅଙ୍କ_Rets; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(tt->types[i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ଯଦି (!ct || !ct->isContinuation) ଫେରନ୍ତୁ m3Err_typeMismatch;

            u32 ଅଙ୍କ_Clauses;
            r = ReadLEB_u32(&ଅଙ୍କ_Clauses, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;

            ପାଇଁ (u32 i = 0; i < ଅଙ୍କ_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                r = v_resume_clause(v, kind, tagIdx, ct); ଯଦି (r) ଫେରନ୍ତୁ r;
            }

            r = v_କାଢ଼_expect(v, c_m3Type_contref, &a); ଯଦି (r) ଫେରନ୍ତୁ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = ft->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[ft->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
                ପାଇଁ (u16 i = 0; i < ft->ଅଙ୍କ_Rets; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(ft->types[i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, ଅଙ୍କ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = ReadLEB_u32(&ଅଙ୍କ_Clauses, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;

            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            ଯଦି (!v->module || tagIdx >= v->module->ଅଙ୍କ_Tags) ଫେରନ୍ତୁ m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ଯଦି (!ct || !ct->isContinuation) ଫେରନ୍ତୁ m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            ଯଦି (tagType && tagType->ଅଙ୍କ_Rets != 0) ଫେରନ୍ତୁ m3Err_typeMismatch;

            ପାଇଁ (u32 i = 0; i < ଅଙ୍କ_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                r = v_resume_clause(v, kind, cTagIdx, ct); ଯଦି (r) ଫେରନ୍ତୁ r;
            }

            r = v_କାଢ଼_expect(v, c_m3Type_contref, &a); ଯଦି (r) ଫେରନ୍ତୁ r;

            ଯଦି (tagType) {
                ପାଇଁ (u16 i = tagType->ଅଙ୍କ_Args; i > 0; i--) {
                    r = v_କାଢ଼_expect(v, BaseTypeOf(tagType->types[tagType->ଅଙ୍କ_Rets + i - 1]), &a);
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = 0; i < ft->ଅଙ୍କ_Rets; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(ft->types[i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xe5: // resume_throw_ref
        {
            u32 typeIdx, ଅଙ୍କ_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = ReadLEB_u32(&ଅଙ୍କ_Clauses, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;

            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ଯଦି (!ct || !ct->isContinuation) ଫେରନ୍ତୁ m3Err_typeMismatch;

            ପାଇଁ (u32 i = 0; i < ଅଙ୍କ_Clauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
                r = v_resume_clause(v, kind, cTagIdx, ct); ଯଦି (r) ଫେରନ୍ତୁ r;
            }

            r = v_କାଢ଼_expect(v, c_m3Type_contref, &a); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = v_କାଢ଼_expect(v, c_m3Type_exnref, &a); ଯଦି (r) ଫେରନ୍ତୁ r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ଯଦି (ft) {
                ପାଇଁ (u16 i = 0; i < ft->ଅଙ୍କ_Rets; i++) {
                    r = v_ଠେଲ(v, BaseTypeOf(ft->types[i]));
                    ଯଦି (r) ଫେରନ୍ତୁ r;
                }
            }
            ଭାଙ୍ଗ;
        }

        ସ୍ଥିତି 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); ଯଦି (r) ଫେରନ୍ତୁ r;

            ଯଦି (typeIdx >= v->module->ଅଙ୍କ_Func_Types) ଫେରନ୍ତୁ m3Err_unknownType;
            ଯଦି (!v->module || tagIdx >= v->module->ଅଙ୍କ_Tags) ଫେରନ୍ତୁ m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ଯଦି (!ct || !ct->isContinuation) ଫେରନ୍ତୁ m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            ଯଦି (tagType && tagType->ଅଙ୍କ_Args != 0) ଫେରନ୍ତୁ m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            ଯଦି (!ft || ft->ଅଙ୍କ_Args == 0) ଫେରନ୍ତୁ m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->ଅଙ୍କ_Rets + ft->ଅଙ୍କ_Args - 1]);
            ଯଦି (!peer || !peer->contFuncType) ଫେରନ୍ତୁ m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_କାଢ଼_expect(v, c_m3Type_contref, &a); ଯଦି (r) ଫେରନ୍ତୁ r;

            ପାଇଁ (u16 i = (u16)(ft->ଅଙ୍କ_Args - 1); i > 0; i--) {
                r = v_କାଢ଼_expect(v, BaseTypeOf(ft->types[ft->ଅଙ୍କ_Rets + i - 1]), &a);
                ଯଦି (r) ଫେରନ୍ତୁ r;
            }

            ପାଇଁ (u16 i = 0; i < self->ଅଙ୍କ_Args; i++) {
                r = v_ଠେଲ(v, BaseTypeOf(self->types[self->ଅଙ୍କ_Rets + i]));
                ଯଦି (r) ଫେରନ୍ତୁ r;
            }
            ଭାଙ୍ଗ;
        }

#endif // d_m3HasStackSwitching

        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            ଭାଙ୍ଗ;

        } // switch

        ଯଦି (r) ଫେରନ୍ତୁ r;

    } // while

    // If we ran out of bytes without hitting the final end
    ଫେରନ୍ତୁ m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    ଯଦି (!i_function->wasm) ଫେରନ୍ତୁ m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    ଯଦି (!v) {
        v = m3_AllocStruct (ValCtx);
        ଯଦି (!v) ଫେରନ୍ତୁ m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, ଆକାର(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    ଯଦି (r) ଫେରନ୍ତୁ r;

    // Parse locals
    u32 ଅଙ୍କ_Local_Blocks;
    r = ReadLEB_u32(&ଅଙ୍କ_Local_Blocks, &v->wasm, v->wasmEnd);
    ଯଦି (r) ଫେରନ୍ତୁ r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 ଅଙ୍କ_Params = funcType ? funcType->ଅଙ୍କ_Args : 0;
    ଯଦି (ଅଙ୍କ_Params > d_m3_Val_ଗଦା) ଫେରନ୍ତୁ m3_Err_function_ଗଦା_Overflow;
    ପାଇଁ (u16 i = 0; i < ଅଙ୍କ_Params; i++) {
        v->localTypes[v->ଅଙ୍କ_Locals++] = BaseTypeOf(funcType->types[funcType->ଅଙ୍କ_Rets + i]);
    }

    // Then: declared locals
    ପାଇଁ (u32 b = 0; b < ଅଙ୍କ_Local_Blocks; b++) {
        u32 ଗଣନା_2;
        r = ReadLEB_u32(&ଗଣନା_2, &v->wasm, v->wasmEnd);
        ଯଦି (r) ଫେରନ୍ତୁ r;
        m3type_t localType;
        r = Parse_ମୂଲ୍ୟ_Type(v->module, &localType, &v->wasm, v->wasmEnd);
        ଯଦି (r) ଫେରନ୍ତୁ r;
        u8 normalized = BaseTypeOf(localType);
        ଯଦି (ଗଣନା_2 > (u32) (d_m3_Val_ଗଦା - v->ଅଙ୍କ_Locals)) ଫେରନ୍ତୁ m3_Err_function_ଗଦା_Overflow;
        ପାଇଁ (u32 c = 0; c < ଗଣନା_2; c++) {
            v->localTypes[v->ଅଙ୍କ_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_ଠେଲ_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    ଯଦି (r) ଫେରନ୍ତୁ r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    ଯଦି (r) ଫେରନ୍ତୁ r;

    // After validation, control stack should be empty and all declared bytes consumed
    ଯଦି (v->ctrl_ଶୀର୍ଷ != 0)
        ଫେରନ୍ତୁ m3Err_wasmMalformed;

    ଯଦି (v->wasm != v->wasmEnd)
        ଫେରନ୍ତୁ m3Err_wasmSectionUnderrun;

    ଫେରନ୍ତୁ m3Err_none;
}

#ଅନ୍ୟଥା // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (ଶୂନ୍ୟ)i_function;
    ଫେରନ୍ତୁ m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
