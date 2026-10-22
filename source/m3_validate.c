//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#అన్తర్భావ "m3_validate.h"
#అన్తర్భావ "m3_exception.h"
#అన్తర్భావ "m3_info.h"

#ఒకవేళ d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#నిర్వచించు c_valBottom         0xFF

// ---------- Control frame ----------

రకనిర్వచనం నిర్మాణం {
    m3opcode_t  opcode;
    u16         ఎత్తు;         // operand stack height at block entry
    u16         param_లెక్క;
    u16         ఫలితం_లెక్క;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} Val_Ctrl_చట్రం;

// ---------- Validator context ----------

నిర్మాణం ValCtx {
    బైట్లు_t     wasm;
    బైట్లు_t     wasm_చివర;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3_Val_దొంతర];
    u16         opd_శిఖరం;

    Val_Ctrl_చట్రం ctrl [d_m3_Val_Ctrl_లోతు];
    u16          ctrl_శిఖరం;

    m3type_t    localTypes [d_m3_Val_దొంతర];
    u16         అంకె_Locals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
స్థిర bool v_has_జ్ఞాపకం_idx (ValCtx * v, u32 i_జ్ఞాపకం_Idx)
{
    తిరిగి v->module and i_జ్ఞాపకం_Idx < v->module->అంకె_Memories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
స్థిర m3type_t v_జ్ఞాపకం_addrtype (ValCtx * v, u32 i_జ్ఞాపకం_Idx)
{
    తిరిగి జ్ఞాపకం_Addr_Type (v->module->memories [i_జ్ఞాపకం_Idx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
స్థిర bool v_స్థానభ్రంశం_లోపల_range (ValCtx * v, u32 i_జ్ఞాపకం_Idx, u64 i_స్థానభ్రంశం)
{
    తిరిగి v_జ్ఞాపకం_addrtype (v, i_జ్ఞాపకం_Idx) == c_m3Type_i64 or i_స్థానభ్రంశం <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
స్థిర m3type_t v_పట్టిక_addrtype (ValCtx * v, u32 i_పట్టిక_Idx)
{
    తిరిగి పట్టిక_Addr_Type (v->module->tables [i_పట్టిక_Idx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
స్థిర u32 v_max_align (m3opcode_t opcode)
{
    ఎంపిక (opcode) {
    స్థితి 0x2c: స్థితి 0x2d:   // i32.load8_s, i32.load8_u
    స్థితి 0x30: స్థితి 0x31:   // i64.load8_s, i64.load8_u
    స్థితి 0x3a:              // i32.store8
    స్థితి 0x3c:              // i64.store8
        తిరిగి 0;
    స్థితి 0x2e: స్థితి 0x2f:   // i32.load16_s, i32.load16_u
    స్థితి 0x32: స్థితి 0x33:   // i64.load16_s, i64.load16_u
    స్థితి 0x3b:              // i32.store16
    స్థితి 0x3d:              // i64.store16
        తిరిగి 1;
    స్థితి 0x29:              // i64.load
    స్థితి 0x2b:              // f64.load
    స్థితి 0x37:              // i64.store
    స్థితి 0x39:              // f64.store
        తిరిగి 3;
    అప్రమేయ:                // 32-bit accesses, and a safe fallback
        తిరిగి 2;
    }
}

#ఒకవేళ d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
స్థిర bool v_atomic_shape (u32 sub, u32 * o_వెడల్పు, u32 * o_అంకె_Operands, m3type_t * o_విలువ_Type, m3type_t * o_ఫలితం_Type)
{
    స్థిర స్థిరరాశి u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    స్థిర స్థిరరాశి m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    ఒకవేళ (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_వెడల్పు       = (sub == 0x02) ? 8 : 4;
        *o_అంకె_Operands = (sub == 0x00) ? 2 : 3;
        *o_విలువ_Type   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_ఫలితం_Type  = c_m3Type_i32;
        తిరిగి true;
    }

    ఒకవేళ (sub < 0x10 or sub > c_wa_Op_చివరి_Atomic) తిరిగి false;

    ఒకవేళ (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_అంకె_Operands = 1;
    } లేకపోతే ఒకవేళ (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_అంకె_Operands = 2;
    } లేకపోతే ఒకవేళ (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_అంకె_Operands = 2;
    } లేకపోతే {                        // cmpxchg
        variant = sub - 0x48;       *o_అంకె_Operands = 3;
    }

    *o_వెడల్పు      = c_widths [variant];
    *o_విలువ_Type  = c_types [variant];
    *o_ఫలితం_Type = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    తిరిగి true;
}

#అయితే_ముగింపు

// ---------- Operand stack ----------

స్థిర M3_ఫలితం v_నెట్టు (ValCtx * v, m3type_t type)
{
    ఒకవేళ (M3_UNLIKELY(v->opd_శిఖరం >= d_m3_Val_దొంతర))
        తిరిగి m3_Err_function_దొంతర_అధికప్రవాహం;
    v->opd[v->opd_శిఖరం++] = type;
    తిరిగి m3Err_none;
}

స్థిర M3_ఫలితం v_తీయి (ValCtx * v, m3type_t * o_type)
{
    Val_Ctrl_చట్రం * f = &v->ctrl[v->ctrl_శిఖరం - 1];
    ఒకవేళ (M3_UNLIKELY(v->opd_శిఖరం == f->ఎత్తు)) {
        ఒకవేళ (f->is_unreachable) { *o_type = c_valBottom; తిరిగి m3Err_none; }
        తిరిగి m3_Err_function_దొంతర_Underrun;
    }
    *o_type = v->opd[--v->opd_శిఖరం];
    తిరిగి m3Err_none;
}

స్థిర M3_ఫలితం v_తీయి_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3_ఫలితం r = v_తీయి(v, &actual);
    ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    ఒకవేళ (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        తిరిగి m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    తిరిగి m3Err_none;
}

// ---------- Control stack ----------

స్థిర M3_ఫలితం v_నెట్టు_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    ఒకవేళ (M3_UNLIKELY(v->ctrl_శిఖరం >= d_m3_Val_Ctrl_లోతు))
        తిరిగి m3_Err_function_దొంతర_అధికప్రవాహం;
    Val_Ctrl_చట్రం * f = &v->ctrl[v->ctrl_శిఖరం++];
    f->opcode       = op;
    f->type         = type;
    f->param_లెక్క  = type ? type->అంకె_వాదనలు : 0;
    f->ఫలితం_లెక్క = type ? type->అంకె_Rets : 0;
    f->ఎత్తు       = v->opd_శిఖరం;
    f->is_unreachable = false;
    తిరిగి m3Err_none;
}

స్థిర M3_ఫలితం v_తీయి_ctrl (ValCtx * v, Val_Ctrl_చట్రం * o_చట్రం)
{
    ఒకవేళ (M3_UNLIKELY(v->ctrl_శిఖరం == 0))
        తిరిగి m3Err_wasmMalformed;
    Val_Ctrl_చట్రం * f = &v->ctrl[v->ctrl_శిఖరం - 1];
    // pop result types
    ఒకవేళ (f->type) {
        కొరకు (u16 i = f->ఫలితం_లెక్క; i > 0; i--) {
            m3type_t a;
            M3_ఫలితం r = v_తీయి_expect(v, పునాది_Type_యొక్క(f->type->types[i - 1]), &a);
            ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
        }
    }
    ఒకవేళ (M3_UNLIKELY(v->opd_శిఖరం != f->ఎత్తు))
        తిరిగి m3_Err_type_లెక్క_Mismatch;
    ఒకవేళ (o_చట్రం) *o_చట్రం = *f;
    v->ctrl_శిఖరం--;
    తిరిగి m3Err_none;
}

స్థిర శూన్య v_unreachable (ValCtx * v)
{
    Val_Ctrl_చట్రం * f = &v->ctrl[v->ctrl_శిఖరం - 1];
    v->opd_శిఖరం = f->ఎత్తు;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
స్థిర u16 v_label_n (Val_Ctrl_చట్రం * f)
{
    తిరిగి (f->opcode == 0x03) ? f->param_లెక్క : f->ఫలితం_లెక్క;
}

స్థిర m3type_t v_label_t (Val_Ctrl_చట్రం * f, u16 i)
{
    ఒకవేళ (!f->type) తిరిగి c_m3Type_none;
    ఒకవేళ (f->opcode == 0x03)
        తిరిగి పునాది_Type_యొక్క(f->type->types[f->type->అంకె_Rets + i]); // params
    తిరిగి పునాది_Type_యొక్క(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
స్థిర m3type_t v_peek (ValCtx * v, u16 i_లోతు)
{
    Val_Ctrl_చట్రం * f = &v->ctrl[v->ctrl_శిఖరం - 1];

    ఒకవేళ (v->opd_శిఖరం <= i_లోతు || (u16)(v->opd_శిఖరం - i_లోతు - 1) < f->ఎత్తు)
        తిరిగి c_valBottom;

    తిరిగి v->opd[v->opd_శిఖరం - i_లోతు - 1];
}

// Pop label types for branch target
స్థిర M3_ఫలితం v_తీయి_labels (ValCtx * v, Val_Ctrl_చట్రం * tgt)
{
    u16 n = v_label_n(tgt);
    కొరకు (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3_ఫలితం r = v_తీయి_expect(v, v_label_t(tgt, i - 1), &a);
        ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    }
    తిరిగి m3Err_none;
}

// Push label types back
స్థిర M3_ఫలితం v_నెట్టు_labels (ValCtx * v, Val_Ctrl_చట్రం * tgt)
{
    u16 n = v_label_n(tgt);
    కొరకు (u16 i = 0; i < n; i++) {
        M3_ఫలితం r = v_నెట్టు(v, v_label_t(tgt, i));
        ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    }
    తిరిగి m3Err_none;
}

// ---------- Block type resolution ----------

స్థిర M3_ఫలితం v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    ఒకవేళ (v->wasm >= v->wasm_చివర)
        తిరిగి m3Err_wasmUnderrun;

#ఒకవేళ d_m3HasTypedRefs
    ఒకవేళ (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3_ఫలితం rr = Parse_విలువ_Type(v->module, &refType, &v->wasm, v->wasm_చివర);
        ఒకవేళ (rr) తిరిగి rr;
        IM3FuncType ftype = ఖాళీ;
        rr = AllocFuncType(&ftype, 1);
        ఒకవేళ (rr) తిరిగి rr;
        ftype->అంకె_Rets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_కూడు_Func_Type(v->module->environment, &ftype);
        ఒకవేళ (rr) తిరిగి rr;
        *o_type = ftype;
        తిరిగి m3Err_none;
    }
#అయితే_ముగింపు

    i64 type;
    M3_ఫలితం r = ReadLebSigned(&type, 33, &v->wasm, v->wasm_చివర);
    ఒకవేళ (r) తిరిగి r;

    ఒకవేళ (type < 0) {
        ఒకవేళ (type < -64) తిరిగి m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        ఒకవేళ (r) తిరిగి r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } లేకపోతే {
        ఒకవేళ ((u32)type >= v->module->అంకె_Func_Types)
            తిరిగి m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    తిరిగి m3Err_none;
}

// ---------- Convenience ----------

స్థిర M3_ఫలితం v_unop (ValCtx * v, m3type_t లోపల, m3type_t బహిర్గతం)
{
    m3type_t a; M3_ఫలితం r = v_తీయి_expect(v, లోపల, &a);
    ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    తిరిగి v_నెట్టు(v, బహిర్గతం);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
స్థిర M3_ఫలితం v_check_తోక_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : ఖాళీ;

    u16 అంకె_Callee_Rets = i_calleeType ? i_calleeType->అంకె_Rets : 0;
    u16 అంకె_Func_Rets   = ft ? ft->అంకె_Rets : 0;

    ఒకవేళ (అంకె_Callee_Rets != అంకె_Func_Rets)
        తిరిగి m3Err_typeMismatch;

    కొరకు (u16 i = 0; i < అంకె_Callee_Rets; i++) {
        ఒకవేళ (i_calleeType->types[i] != ft->types[i])
            తిరిగి m3Err_typeMismatch;
    }

    తిరిగి m3Err_none;
}

స్థిర M3_ఫలితం v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ఫలితం r;
    r = v_తీయి_expect(v, t, &a); ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    r = v_తీయి_expect(v, t, &a); ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    తిరిగి v_నెట్టు(v, t);
}

స్థిర M3_ఫలితం v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3_ఫలితం r;
    r = v_తీయి_expect(v, t, &a); ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    r = v_తీయి_expect(v, t, &a); ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;
    తిరిగి v_నెట్టు(v, c_m3Type_i32);
}

స్థిర M3_ఫలితం v_testop (ValCtx * v, m3type_t t)
{
    తిరిగి v_unop(v, t, c_m3Type_i32);
}

స్థిర M3_ఫలితం v_cvtop (ValCtx * v, m3type_t లోపల, m3type_t బహిర్గతం)
{
    తిరిగి v_unop(v, లోపల, బహిర్గతం);
}


// ---------- Main validation loop ----------

#ఒకవేళ d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
స్థిర M3_ఫలితం v_catch_clause (ValCtx * v)
{
    u8 రకం;
    M3_ఫలితం r = Read_u8(&రకం, &v->wasm, v->wasm_చివర);
    ఒకవేళ (r) తిరిగి r;
    ఒకవేళ (రకం > 0x03) తిరిగి m3Err_wasmMalformed;

    bool has_ట్యాగ్ = (రకం == 0x00 || రకం == 0x01);
    bool hasRef = (రకం == 0x01 || రకం == 0x03);

    IM3FuncType ట్యాగ్_Type = ఖాళీ;
    ఒకవేళ (has_ట్యాగ్) {
        u32 ట్యాగ్_సూచీ;
        r = ReadLEB_u32(&ట్యాగ్_సూచీ, &v->wasm, v->wasm_చివర);
        ఒకవేళ (r) తిరిగి r;
        ఒకవేళ (!v->module || ట్యాగ్_సూచీ >= v->module->అంకె_ట్యాగులు) తిరిగి m3_Err_unknown_ట్యాగ్;
        ట్యాగ్_Type = v->module->ట్యాగులు[ట్యాగ్_సూచీ].type;
    }

    u32 లోతు;
    r = ReadLEB_u32(&లోతు, &v->wasm, v->wasm_చివర);
    ఒకవేళ (r) తిరిగి r;
    ఒకవేళ (లోతు >= v->ctrl_శిఖరం) తిరిగి m3Err_unknownLabel;

    Val_Ctrl_చట్రం * tgt = &v->ctrl[v->ctrl_శిఖరం - 1 - లోతు];

    u16 అంకె_Payload = ట్యాగ్_Type ? ట్యాగ్_Type->అంకె_వాదనలు : 0;
    u16 అంకె_Label   = v_label_n(tgt);

    ఒకవేళ (అంకె_Label != అంకె_Payload + (hasRef ? 1u : 0u))
        తిరిగి m3_Err_type_లెక్క_Mismatch;

    కొరకు (u16 i = 0; i < అంకె_Payload; i++) {
        ఒకవేళ (v_label_t(tgt, i) != పునాది_Type_యొక్క(ట్యాగ్_Type->types[ట్యాగ్_Type->అంకె_Rets + i]))
            తిరిగి m3Err_typeMismatch;
    }

    ఒకవేళ (hasRef and v_label_t(tgt, అంకె_Payload) != c_m3Type_exnref)
        తిరిగి m3Err_typeMismatch;

    తిరిగి m3Err_none;
}

#అయితే_ముగింపు // d_m3HasExceptionHandling

#ఒకవేళ d_m3_Has_దొంతర_Switching

స్థిర M3_ఫలితం v_resume_clause (ValCtx * v, u8 రకం, u32 ట్యాగ్_Idx, IM3FuncType ct)
{
    ఒకవేళ (రకం > 0x01) తిరిగి m3Err_wasmMalformed;
    ఒకవేళ (!v->module || ట్యాగ్_Idx >= v->module->అంకె_ట్యాగులు) తిరిగి m3_Err_unknown_ట్యాగ్;
    IM3FuncType ట్యాగ్_Type = v->module->ట్యాగులు[ట్యాగ్_Idx].type;

    ఒకవేళ (రకం == 0x00) {
        u32 లోతు;
        M3_ఫలితం r = ReadLEB_u32(&లోతు, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
        ఒకవేళ (లోతు >= v->ctrl_శిఖరం) తిరిగి m3Err_unknownLabel;

        Val_Ctrl_చట్రం * tgt = &v->ctrl[v->ctrl_శిఖరం - 1 - లోతు];
        u16 అంకె_Payload = ట్యాగ్_Type ? ట్యాగ్_Type->అంకె_వాదనలు : 0;
        u16 అంకె_Label   = v_label_n(tgt);

        ఒకవేళ (అంకె_Label != అంకె_Payload + 1u) తిరిగి m3_Err_type_లెక్క_Mismatch;

        కొరకు (u16 j = 0; j < అంకె_Payload; j++) {
            ఒకవేళ (v_label_t(tgt, j) != పునాది_Type_యొక్క(ట్యాగ్_Type->types[ట్యాగ్_Type->అంకె_Rets + j]))
                తిరిగి m3Err_typeMismatch;
        }
        ఒకవేళ (v_label_t(tgt, అంకె_Payload) != c_m3Type_contref)
            తిరిగి m3Err_typeMismatch;

#ఒకవేళ d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->అంకె_Rets + అంకె_Payload]
            : tgt->type->types[అంకె_Payload];

        ఒకవేళ (కుప్ప_Type_యొక్క(labelContType) == d_m3_Type_కుప్ప_Abstract ||
            కుప్ప_Type_యొక్క(labelContType) == d_m3_Type_కుప్ప_None) {
            తిరిగి m3Err_typeMismatch;
        }

        u32 కుప్ప_Type = కుప్ప_Type_యొక్క(labelContType);
        IM3FuncType labelCt = ఖాళీ;
        కొరకు (u32 idx = 0; idx < v->module->అంకె_Func_Types; ++idx) {
            ఒకవేళ (v->module->funcTypes[idx]->canonical_సూచీ == కుప్ప_Type &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                ఆపు;
            }
        }
        ఒకవేళ (!labelCt) తిరిగి m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expected_అంకె_Params = ట్యాగ్_Type ? ట్యాగ్_Type->అంకె_Rets : 0;
        ఒకవేళ (labelFt->అంకె_వాదనలు != expected_అంకె_Params) తిరిగి m3Err_typeMismatch;
        కొరకు (u16 p = 0; p < expected_అంకె_Params; ++p) {
            ఒకవేళ (labelFt->types[labelFt->అంకె_Rets + p] != ట్యాగ్_Type->types[p])
                తిరిగి m3Err_typeMismatch;
        }

        u16 expected_అంకె_Rets = ft ? ft->అంకె_Rets : 0;
        ఒకవేళ (labelFt->అంకె_Rets != expected_అంకె_Rets) తిరిగి m3Err_typeMismatch;
        కొరకు (u16 r_idx = 0; r_idx < expected_అంకె_Rets; ++r_idx) {
            ఒకవేళ (labelFt->types[r_idx] != ft->types[r_idx])
                తిరిగి m3Err_typeMismatch;
        }
#అయితే_ముగింపు
    } లేకపోతే {
        ఒకవేళ (ట్యాగ్_Type && ట్యాగ్_Type->అంకె_వాదనలు != 0) తిరిగి m3Err_typeMismatch;
    }
    తిరిగి m3Err_none;
}

#అయితే_ముగింపు // d_m3HasStackSwitching

స్థిర M3_ఫలితం v_validate_body (ValCtx * v)
{
    M3_ఫలితం r = m3Err_none;
    m3type_t a = c_valBottom;

    అయితే (v->wasm < v->wasm_చివర)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasm_చివర);
        ఒకవేళ (M3_UNLIKELY(r)) తిరిగి r;

        ఎంపిక (opcode)
        {
        // ---- Control ----
        స్థితి 0x00: // unreachable
            v_unreachable(v);
            ఆపు;

        స్థితి 0x01: // nop
            ఆపు;

        స్థితి 0x02: //ఖండం
        స్థితి 0x03: // loop
        స్థితి 0x04: // if
        {
            IM3FuncType bt = ఖాళీ;
            r = v_read_blocktype(v, &bt);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (opcode == 0x04) {
                r = v_తీయి_expect(v, c_m3Type_i32, &a);
                ఒకవేళ (r) తిరిగి r;
            }
            // Pop block params from caller stack
            ఒకవేళ (bt) {
                కొరకు (u16 i = bt->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(bt->types[bt->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            r = v_నెట్టు_ctrl(v, opcode, bt);
            ఒకవేళ (r) తిరిగి r;
            // Push params inside block
            ఒకవేళ (bt) {
                కొరకు (u16 i = 0; i < bt->అంకె_వాదనలు; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(bt->types[bt->అంకె_Rets + i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

#ఒకవేళ d_m3HasExceptionHandling
        స్థితి 0x1f: // try_table
        {
            IM3FuncType bt = ఖాళీ;
            r = v_read_blocktype(v, &bt);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (bt) {
                కొరకు (u16 i = bt->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(bt->types[bt->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 అంకె_Catch;
            r = ReadLEB_u32(&అంకె_Catch, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            కొరకు (u32 i = 0; i < అంకె_Catch; i++) {
                r = v_catch_clause(v);
                ఒకవేళ (r) తిరిగి r;
            }

            r = v_నెట్టు_ctrl(v, opcode, bt);
            ఒకవేళ (r) తిరిగి r;

            ఒకవేళ (bt) {
                కొరకు (u16 i = 0; i < bt->అంకె_వాదనలు; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(bt->types[bt->అంకె_Rets + i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0x08: // throw
        {
            u32 ట్యాగ్_సూచీ;
            r = ReadLEB_u32(&ట్యాగ్_సూచీ, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (!v->module || ట్యాగ్_సూచీ >= v->module->అంకె_ట్యాగులు) తిరిగి m3_Err_unknown_ట్యాగ్;
            IM3FuncType tt = v->module->ట్యాగులు[ట్యాగ్_సూచీ].type;
            ఒకవేళ (tt) {
                కొరకు (u16 i = tt->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(tt->types[tt->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            v_unreachable(v);
            ఆపు;
        }

        స్థితి 0x0a: // throw_ref
            r = v_తీయి_expect(v, c_m3Type_exnref, &a);
            ఒకవేళ (r) తిరిగి r;
            v_unreachable(v);
            ఆపు;
#అయితే_ముగింపు // d_m3HasExceptionHandling

        స్థితి 0x05: // else
        {
            Val_Ctrl_చట్రం చట్రం;
            r = v_తీయి_ctrl(v, &చట్రం);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (చట్రం.opcode != 0x04)
                తిరిగి m3Err_wasmMalformed;
            r = v_నెట్టు_ctrl(v, 0x05, చట్రం.type);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (చట్రం.type) {
                కొరకు (u16 i = 0; i < చట్రం.type->అంకె_వాదనలు; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(చట్రం.type->types[చట్రం.type->అంకె_Rets + i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0x0b: //చివర
        {
            Val_Ctrl_చట్రం చట్రం;
            r = v_తీయి_ctrl(v, &చట్రం);
            ఒకవేళ (r) తిరిగి r;
            // Push results
            ఒకవేళ (చట్రం.type) {
                కొరకు (u16 i = 0; i < చట్రం.ఫలితం_లెక్క; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(చట్రం.type->types[i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            // If this was the outermost frame, we're done
            ఒకవేళ (v->ctrl_శిఖరం == 0)
                తిరిగి m3Err_none;
            ఆపు;
        }

        స్థితి 0x0c: // br
        {
            u32 లోతు;
            r = ReadLEB_u32(&లోతు, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (లోతు >= v->ctrl_శిఖరం) తిరిగి m3Err_unknownLabel;
            Val_Ctrl_చట్రం * tgt = &v->ctrl[v->ctrl_శిఖరం - 1 - లోతు];
            r = v_తీయి_labels(v, tgt);
            ఒకవేళ (r) తిరిగి r;
            v_unreachable(v);
            ఆపు;
        }

        స్థితి 0x0d: // br_if
        {
            u32 లోతు;
            r = ReadLEB_u32(&లోతు, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (లోతు >= v->ctrl_శిఖరం) తిరిగి m3Err_unknownLabel;
            r = v_తీయి_expect(v, c_m3Type_i32, &a);
            ఒకవేళ (r) తిరిగి r;
            Val_Ctrl_చట్రం * tgt = &v->ctrl[v->ctrl_శిఖరం - 1 - లోతు];
            r = v_తీయి_labels(v, tgt);
            ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు_labels(v, tgt);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0x0e: // br_table
        {
            u32 లెక్క;
            r = ReadLEB_u32(&లెక్క, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            u32 def_లోతు = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            బైట్లు_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            కొరకు (u32 i = 0; i <= లెక్క; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_చివర);
                ఒకవేళ (r) తిరిగి r;
                ఒకవేళ (d >= v->ctrl_శిఖరం) తిరిగి m3Err_unknownLabel;
                ఒకవేళ (i == లెక్క) def_లోతు = d;
            }
            // Now validate all labels match the default's types
            Val_Ctrl_చట్రం * defTgt = &v->ctrl[v->ctrl_శిఖరం - 1 - def_లోతు];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_తీయి_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            ఒకవేళ (r) తిరిగి r;

            కొరకు (u32 i = 0; i <= లెక్క; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasm_చివర);
                ఒకవేళ (r) తిరిగి r;
                Val_Ctrl_చట్రం * t = &v->ctrl[v->ctrl_శిఖరం - 1 - d];
                u16 n = v_label_n(t);
                ఒకవేళ (n != arity) తిరిగి m3_Err_type_లెక్క_Mismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                కొరకు (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    ఒకవేళ (want != c_valBottom && have != c_valBottom && want != have)
                        తిరిగి m3Err_typeMismatch;
                }
            }
            Val_Ctrl_చట్రం * dt = &v->ctrl[v->ctrl_శిఖరం - 1 - def_లోతు];
            r = v_తీయి_labels(v, dt);
            ఒకవేళ (r) తిరిగి r;
            v_unreachable(v);
            ఆపు;
        }

        స్థితి 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            ఒకవేళ (ft) {
                కొరకు (u16 i = ft->అంకె_Rets; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            v_unreachable(v);
            ఆపు;
        }

        // ---- Call ----
        స్థితి 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (idx >= v->module->అంకె_Functions) తిరిగి m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            ఒకవేళ (ft) {
                కొరకు (u16 i = ft->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[ft->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
                కొరకు (u16 i = 0; i < ft->అంకె_Rets; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(ft->types[i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            u32 పట్టిక_Idx;
            r = ReadLEB_u32(&పట్టిక_Idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            // the table must exist and hold funcrefs
            ఒకవేళ (పట్టిక_Idx >= v->module->అంకె_Tables) తిరిగి m3_Err_unknown_పట్టిక;
            ఒకవేళ (v->module->tables[పట్టిక_Idx]->type != c_m3Type_funcref) తిరిగి m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_తీయి_expect(v, v_పట్టిక_addrtype(v, పట్టిక_Idx), &a); // table index operand
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (ft) {
                కొరకు (u16 i = ft->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[ft->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
                కొరకు (u16 i = 0; i < ft->అంకె_Rets; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(ft->types[i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (idx >= v->module->అంకె_Functions) తిరిగి m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_తోక_results(v, ft);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (ft) {
                కొరకు (u16 i = ft->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[ft->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            v_unreachable(v);
            ఆపు;
        }

        స్థితి 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            u32 పట్టిక_Idx;
            r = ReadLEB_u32(&పట్టిక_Idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            // the table must exist and hold funcrefs
            ఒకవేళ (పట్టిక_Idx >= v->module->అంకె_Tables) తిరిగి m3_Err_unknown_పట్టిక;
            ఒకవేళ (v->module->tables[పట్టిక_Idx]->type != c_m3Type_funcref) తిరిగి m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_తోక_results(v, ft);
            ఒకవేళ (r) తిరిగి r;
            r = v_తీయి_expect(v, v_పట్టిక_addrtype(v, పట్టిక_Idx), &a); // table index operand
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (ft) {
                కొరకు (u16 i = ft->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[ft->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            v_unreachable(v);
            ఆపు;
        }

#ఒకవేళ d_m3HasTypedRefs
        స్థితి 0x14: // call_ref
        స్థితి 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool is_తోక = (opcode == 0x15);
            ఒకవేళ (is_తోక) {
                r = v_check_తోక_results(v, ft);
                ఒకవేళ (r) తిరిగి r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_తీయి_expect(v, c_m3Type_funcref, &a);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (ft) {
                కొరకు (u16 i = ft->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[ft->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
                ఒకవేళ (not is_తోక) {
                    కొరకు (u16 i = 0; i < ft->అంకె_Rets; i++) {
                        r = v_నెట్టు(v, పునాది_Type_యొక్క(ft->types[i]));
                        ఒకవేళ (r) తిరిగి r;
                    }
                }
            }
            ఒకవేళ (is_తోక) v_unreachable(v);
            ఆపు;
        }

        స్థితి 0xd4: // ref.as_non_null
        {
            r = v_తీయి_expect(v, c_m3Type_funcref, &a);
            ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }
#అయితే_ముగింపు // d_m3HasTypedRefs

        // ---- Parametric ----
        స్థితి 0x1a: // drop
            r = v_తీయి(v, &a);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;

        స్థితి 0x1b: // select
        {
            r = v_తీయి_expect(v, c_m3Type_i32, &a);
            ఒకవేళ (r) తిరిగి r;
            m3type_t t2 = c_valBottom;
            r = v_తీయి(v, &t2);
            ఒకవేళ (r) తిరిగి r;
            m3type_t t1;
            r = v_తీయి_expect(v, t2, &t1);
            ఒకవేళ (r) తిరిగి r;
            // untyped select is numeric only; references need the 0x1c form
            ఒకవేళ (t2 != c_valBottom && IsRefType(t2)) తిరిగి m3Err_typeMismatch;
            r = v_నెట్టు(v, (t2 == c_valBottom) ? t1 : t2);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

#ఒకవేళ d_m3HasRefTypes
        స్థితి 0x1c: // select with an explicit result type
        {
            u32 అంకె_Types;
            r = ReadLEB_u32(&అంకె_Types, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (అంకె_Types != 1) తిరిగి m3Err_wasmMalformed;

            m3type_t selType;
            r = Parse_విలువ_Type(v->module, &selType, &v->wasm, v->wasm_చివర);  ఒకవేళ (r) తిరిగి r;
            u8 t = పునాది_Type_యొక్క(selType);

            r = v_తీయి_expect(v, c_m3Type_i32, &a); ఒకవేళ (r) తిరిగి r;
            r = v_తీయి_expect(v, t, &a);            ఒకవేళ (r) తిరిగి r;
            r = v_తీయి_expect(v, t, &a);            ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు(v, t);                      ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0x25: // table.get
        స్థితి 0x26: // table.set
        {
            u32 పట్టిక_Idx;
            r = ReadLEB_u32(&పట్టిక_Idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (పట్టిక_Idx >= v->module->అంకె_Tables) తిరిగి m3_Err_unknown_పట్టిక;
            u8 t = పునాది_Type_యొక్క(v->module->tables[పట్టిక_Idx]->type);
            m3type_t at = v_పట్టిక_addrtype(v, పట్టిక_Idx);

            ఒకవేళ (opcode == 0x26) {
                r = v_తీయి_expect(v, t, &a);   ఒకవేళ (r) తిరిగి r;
                r = v_తీయి_expect(v, at, &a);  ఒకవేళ (r) తిరిగి r;
            } లేకపోతే {
                r = v_తీయి_expect(v, at, &a);  ఒకవేళ (r) తిరిగి r;
                r = v_నెట్టు(v, t);             ఒకవేళ (r) తిరిగి r;
            }
            ఆపు;
        }

        స్థితి 0xd0: // ref.null
        {
#ఒకవేళ d_m3HasTypedRefs
            m3type_t కుప్ప_బిట్లు;
            r = Parse_కుప్ప_Type(v->module, &కుప్ప_బిట్లు, &v->wasm, v->wasm_చివర);  ఒకవేళ (r) తిరిగి r;
            u8 t = (కుప్ప_బిట్లు & d_m3Type_refExtern) ? c_m3Type_externref :
# ఒకవేళ d_m3_Has_దొంతర_Switching
                   (కుప్ప_బిట్లు & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#లేకపోతే
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasm_చివర);   ఒకవేళ (r) తిరిగి r;
            r = NormalizeType(&t, waType);                   ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (!IsRefType(t)) తిరిగి m3Err_wasmMalformed;
#అయితే_ముగింపు
            r = v_నెట్టు(v, t);                                ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_తీయి(v, &t);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (t != c_valBottom && !IsRefType(t)) తిరిగి m3Err_typeMismatch;
            r = v_నెట్టు(v, c_m3Type_i32);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (funcIdx >= v->module->అంకె_Functions) తిరిగి m3Err_unknownFunction;
            ఒకవేళ (!Module_IsFunctionDeclared(v->module, funcIdx)) తిరిగి m3Err_undeclaredFuncRef;
            r = v_నెట్టు(v, c_m3Type_funcref);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }
#అయితే_ముగింపు

        // ---- Variable ----
        స్థితి 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (idx >= v->అంకె_Locals) తిరిగి m3Err_unknownLocal;
            r = v_నెట్టు(v, v->localTypes[idx]);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (idx >= v->అంకె_Locals) తిరిగి m3Err_unknownLocal;
            r = v_తీయి_expect(v, v->localTypes[idx], &a);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (idx >= v->అంకె_Locals) తిరిగి m3Err_unknownLocal;
            r = v_తీయి_expect(v, v->localTypes[idx], &a);
            ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు(v, v->localTypes[idx]);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (idx >= v->module->అంకె_Globals) తిరిగి m3Err_unknownGlobal;
            r = v_నెట్టు(v, పునాది_Type_యొక్క(v->module->globals[idx].type));
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (idx >= v->module->అంకె_Globals) తిరిగి m3Err_unknownGlobal;
            r = v_తీయి_expect(v, పునాది_Type_యొక్క(v->module->globals[idx].type), &a);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        // ---- Memory load ----
        స్థితి 0x28: స్థితి 0x29: స్థితి 0x2a: స్థితి 0x2b: // i32/i64/f32/f64.load
        స్థితి 0x2c: స్థితి 0x2d: స్థితి 0x2e: స్థితి 0x2f: // i32.load8/16 s/u
        స్థితి 0x30: స్థితి 0x31: స్థితి 0x32: స్థితి 0x33: // i64.load8/16 s/u
        స్థితి 0x34: స్థితి 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = Read_జ్ఞాపకం_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (align > v_max_align(opcode)) తిరిగి m3Err_invalidAlignment;
            ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, memidx)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
            ఒకవేళ (not v_స్థానభ్రంశం_లోపల_range(v, memidx, offset)) తిరిగి m3Err_wasmMalformed;
            r = v_తీయి_expect(v, v_జ్ఞాపకం_addrtype(v, memidx), &a); ఒకవేళ (r) తిరిగి r;
            u8 ఫలితం;
            ఒకవేళ      (opcode == 0x28) ఫలితం = c_m3Type_i32;
            లేకపోతే ఒకవేళ (opcode == 0x29) ఫలితం = c_m3Type_i64;
            లేకపోతే ఒకవేళ (opcode == 0x2a) ఫలితం = c_m3Type_f32;
            లేకపోతే ఒకవేళ (opcode == 0x2b) ఫలితం = c_m3Type_f64;
            లేకపోతే ఒకవేళ (opcode <= 0x2f) ఫలితం = c_m3Type_i32;
            లేకపోతే                     ఫలితం = c_m3Type_i64;
            r = v_నెట్టు(v, ఫలితం);
            ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        // ---- Memory store ----
        స్థితి 0x36: స్థితి 0x37: స్థితి 0x38: స్థితి 0x39: // i32/i64/f32/f64.store
        స్థితి 0x3a: స్థితి 0x3b:                         // i32.store8/16
        స్థితి 0x3c: స్థితి 0x3d: స్థితి 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = Read_జ్ఞాపకం_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (align > v_max_align(opcode)) తిరిగి m3Err_invalidAlignment;
            ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, memidx)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
            ఒకవేళ (not v_స్థానభ్రంశం_లోపల_range(v, memidx, offset)) తిరిగి m3Err_wasmMalformed;
            u8 valtype;
            ఒకవేళ      (opcode == 0x36) valtype = c_m3Type_i32;
            లేకపోతే ఒకవేళ (opcode == 0x37) valtype = c_m3Type_i64;
            లేకపోతే ఒకవేళ (opcode == 0x38) valtype = c_m3Type_f32;
            లేకపోతే ఒకవేళ (opcode == 0x39) valtype = c_m3Type_f64;
            లేకపోతే ఒకవేళ (opcode <= 0x3b) valtype = c_m3Type_i32;
            లేకపోతే                     valtype = c_m3Type_i64;
            r = v_తీయి_expect(v, valtype, &a); ఒకవేళ (r) తిరిగి r;
            r = v_తీయి_expect(v, v_జ్ఞాపకం_addrtype(v, memidx), &a); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        // ---- Memory size/grow ----
        స్థితి 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, memidx)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
            r = v_నెట్టు(v, v_జ్ఞాపకం_addrtype(v, memidx)); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }
        స్థితి 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, memidx)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
            r = v_తీయి_expect(v, v_జ్ఞాపకం_addrtype(v, memidx), &a); ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు(v, v_జ్ఞాపకం_addrtype(v, memidx)); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        // ---- Constants ----
        స్థితి 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు(v, c_m3Type_i32); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }
        స్థితి 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు(v, c_m3Type_i64); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }
        స్థితి 0x43: { // f32.const
            ఒకవేళ (v->wasm + 4 > v->wasm_చివర) తిరిగి m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_నెట్టు(v, c_m3Type_f32); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }
        స్థితి 0x44: { // f64.const
            ఒకవేళ (v->wasm + 8 > v->wasm_చివర) తిరిగి m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_నెట్టు(v, c_m3Type_f64); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }


        // ---- i32 comparison ----
        స్థితి 0x45: r = v_testop(v, c_m3Type_i32); ఆపు; // i32.eqz
        స్థితి 0x46: స్థితి 0x47: స్థితి 0x48: స్థితి 0x49: స్థితి 0x4a:
        స్థితి 0x4b: స్థితి 0x4c: స్థితి 0x4d: స్థితి 0x4e: స్థితి 0x4f:
            r = v_relop(v, c_m3Type_i32); ఆపు;

        // ---- i64 comparison ----
        స్థితి 0x50: r = v_testop(v, c_m3Type_i64); ఆపు; // i64.eqz
        స్థితి 0x51: స్థితి 0x52: స్థితి 0x53: స్థితి 0x54: స్థితి 0x55:
        స్థితి 0x56: స్థితి 0x57: స్థితి 0x58: స్థితి 0x59: స్థితి 0x5a:
            r = v_relop(v, c_m3Type_i64); ఆపు;

        // ---- f32 comparison ----
        స్థితి 0x5b: స్థితి 0x5c: స్థితి 0x5d: స్థితి 0x5e: స్థితి 0x5f: స్థితి 0x60:
            r = v_relop(v, c_m3Type_f32); ఆపు;

        // ---- f64 comparison ----
        స్థితి 0x61: స్థితి 0x62: స్థితి 0x63: స్థితి 0x64: స్థితి 0x65: స్థితి 0x66:
            r = v_relop(v, c_m3Type_f64); ఆపు;

        // ---- i32 unary ----
        స్థితి 0x67: స్థితి 0x68: స్థితి 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ఆపు;

        // ---- i32 binary ----
        స్థితి 0x6a: స్థితి 0x6b: స్థితి 0x6c: స్థితి 0x6d: స్థితి 0x6e: స్థితి 0x6f:
        స్థితి 0x70: స్థితి 0x71: స్థితి 0x72: స్థితి 0x73: స్థితి 0x74: స్థితి 0x75:
        స్థితి 0x76: స్థితి 0x77: స్థితి 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); ఆపు;

        // ---- i64 unary ----
        స్థితి 0x79: స్థితి 0x7a: స్థితి 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ఆపు;

        // ---- i64 binary ----
        స్థితి 0x7c: స్థితి 0x7d: స్థితి 0x7e: స్థితి 0x7f: స్థితి 0x80: స్థితి 0x81:
        స్థితి 0x82: స్థితి 0x83: స్థితి 0x84: స్థితి 0x85: స్థితి 0x86: స్థితి 0x87:
        స్థితి 0x88: స్థితి 0x89: స్థితి 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); ఆపు;

        // ---- f32 unary ----
        స్థితి 0x8b: స్థితి 0x8c: స్థితి 0x8d: స్థితి 0x8e: స్థితి 0x8f:
        స్థితి 0x90: స్థితి 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); ఆపు;

        // ---- f32 binary ----
        స్థితి 0x92: స్థితి 0x93: స్థితి 0x94: స్థితి 0x95: స్థితి 0x96:
        స్థితి 0x97: స్థితి 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); ఆపు;

        // ---- f64 unary ----
        స్థితి 0x99: స్థితి 0x9a: స్థితి 0x9b: స్థితి 0x9c: స్థితి 0x9d:
        స్థితి 0x9e: స్థితి 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); ఆపు;

        // ---- f64 binary ----
        స్థితి 0xa0: స్థితి 0xa1: స్థితి 0xa2: స్థితి 0xa3: స్థితి 0xa4:
        స్థితి 0xa5: స్థితి 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); ఆపు;

        // ---- Conversions ----
        స్థితి 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); ఆపు; // i32.wrap/i64
        స్థితి 0xa8: స్థితి 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ఆపు;
        స్థితి 0xaa: స్థితి 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ఆపు;
        స్థితి 0xac: స్థితి 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); ఆపు;
        స్థితి 0xae: స్థితి 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ఆపు;
        స్థితి 0xb0: స్థితి 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ఆపు;
        స్థితి 0xb2: స్థితి 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ఆపు;
        స్థితి 0xb4: స్థితి 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); ఆపు;
        స్థితి 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); ఆపు;
        స్థితి 0xb7: స్థితి 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); ఆపు;
        స్థితి 0xb9: స్థితి 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ఆపు;
        స్థితి 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); ఆపు;
        స్థితి 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ఆపు;
        స్థితి 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ఆపు;
        స్థితి 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); ఆపు;
        స్థితి 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); ఆపు;

        // ---- Sign-extension (MVP post) ----
        స్థితి 0xc0: స్థితి 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); ఆపు;
        స్థితి 0xc2: స్థితి 0xc3: స్థితి 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); ఆపు;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        స్థితి 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;
            ఎంపిక (sub) {
            స్థితి 0x00: స్థితి 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); ఆపు;
            స్థితి 0x02: స్థితి 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); ఆపు;
            స్థితి 0x04: స్థితి 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); ఆపు;
            స్థితి 0x06: స్థితి 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); ఆపు;
            స్థితి 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, memidx)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
                // the segments must have been declared up front by a data count section
                ఒకవేళ (not v->module->has_దత్తాంశం_లెక్క) తిరిగి m3_Err_దత్తాంశం_లెక్క_Required;
                ఒకవేళ (dataidx >= v->module->అంకె_దత్తాంశం_Segments) తిరిగి m3_Err_unknown_దత్తాంశం_Segment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_తీయి_expect(v, c_m3Type_i32, &a); ఒకవేళ (r) తిరిగి r; // n
                r = v_తీయి_expect(v, c_m3Type_i32, &a); ఒకవేళ (r) తిరిగి r; // src
                r = v_తీయి_expect(v, v_జ్ఞాపకం_addrtype(v, memidx), &a); ఒకవేళ (r) తిరిగి r; // dst
                ఆపు;
            }
            స్థితి 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                // needs the segment, but not a memory
                ఒకవేళ (not v->module->has_దత్తాంశం_లెక్క) తిరిగి m3_Err_దత్తాంశం_లెక్క_Required;
                ఒకవేళ (dataidx >= v->module->అంకె_దత్తాంశం_Segments) తిరిగి m3_Err_unknown_దత్తాంశం_Segment;
                ఆపు;
            }
#ఒకవేళ d_m3HasRefTypes
            స్థితి 0x0c: // table.init
            స్థితి 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;

                ఒకవేళ (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    ఒకవేళ (a2 >= v->module->అంకె_Tables) తిరిగి m3_Err_unknown_పట్టిక;
                    ఒకవేళ (a1 >= v->module->అంకె_Element_Segments) తిరిగి m3Err_unknownElemSegment;
                    ఒకవేళ (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        తిరిగి m3Err_typeMismatch;
                } లేకపోతే {                                                    // dst, src
                    ఒకవేళ (a1 >= v->module->అంకె_Tables) తిరిగి m3_Err_unknown_పట్టిక;
                    ఒకవేళ (a2 >= v->module->అంకె_Tables) తిరిగి m3_Err_unknown_పట్టిక;
                    ఒకవేళ (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        తిరిగి m3Err_typeMismatch;
                }

                ఒకవేళ (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_తీయి_expect(v, c_m3Type_i32, &a); ఒకవేళ (r) తిరిగి r;             // n
                    r = v_తీయి_expect(v, c_m3Type_i32, &a); ఒకవేళ (r) తిరిగి r;             // s
                    r = v_తీయి_expect(v, v_పట్టిక_addrtype(v, a2), &a); ఒకవేళ (r) తిరిగి r;  // d
                } లేకపోతే {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_పట్టిక_addrtype(v, a1);
                    m3type_t sType = v_పట్టిక_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_తీయి_expect(v, nType, &a); ఒకవేళ (r) తిరిగి r;  // n
                    r = v_తీయి_expect(v, sType, &a); ఒకవేళ (r) తిరిగి r;  // s
                    r = v_తీయి_expect(v, dType, &a); ఒకవేళ (r) తిరిగి r;  // d
                }
                ఆపు;
            }
            స్థితి 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                ఒకవేళ (elemIdx >= v->module->అంకె_Element_Segments) తిరిగి m3Err_unknownElemSegment;
                ఆపు;
            }
            స్థితి 0x0f: // table.grow
            స్థితి 0x10: // table.size
            స్థితి 0x11: // table.fill
            {
                u32 పట్టిక_Idx;
                r = ReadLEB_u32(&పట్టిక_Idx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                ఒకవేళ (పట్టిక_Idx >= v->module->అంకె_Tables) తిరిగి m3_Err_unknown_పట్టిక;
                u8 t = పునాది_Type_యొక్క(v->module->tables[పట్టిక_Idx]->type);
                m3type_t at = v_పట్టిక_addrtype(v, పట్టిక_Idx);

                ఒకవేళ (sub == 0x10) {                                          // table.size
                    r = v_నెట్టు(v, at); ఒకవేళ (r) తిరిగి r;
                } లేకపోతే ఒకవేళ (sub == 0x0f) {                                   // table.grow
                    r = v_తీయి_expect(v, at, &a); ఒకవేళ (r) తిరిగి r;  // n
                    r = v_తీయి_expect(v, t, &a);  ఒకవేళ (r) తిరిగి r;  // init
                    r = v_నెట్టు(v, at);           ఒకవేళ (r) తిరిగి r;
                } లేకపోతే {                                                    // table.fill
                    r = v_తీయి_expect(v, at, &a); ఒకవేళ (r) తిరిగి r;  // n
                    r = v_తీయి_expect(v, t, &a);  ఒకవేళ (r) తిరిగి r;  // val
                    r = v_తీయి_expect(v, at, &a); ఒకవేళ (r) తిరిగి r;  // i
                }
                ఆపు;
            }
#అయితే_ముగింపు
            స్థితి 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, dst) or not v_has_జ్ఞాపకం_idx(v, src)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_జ్ఞాపకం_addrtype(v, dst);
                m3type_t srcType = v_జ్ఞాపకం_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_తీయి_expect(v, lenType, &a); ఒకవేళ (r) తిరిగి r; // n
                r = v_తీయి_expect(v, srcType, &a); ఒకవేళ (r) తిరిగి r; // src
                r = v_తీయి_expect(v, dstType, &a); ఒకవేళ (r) తిరిగి r; // dst
                ఆపు;
            }
            స్థితి 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, memidx)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
                m3type_t addrType = v_జ్ఞాపకం_addrtype(v, memidx);
                r = v_తీయి_expect(v, addrType, &a); ఒకవేళ (r) తిరిగి r;      // n
                r = v_తీయి_expect(v, c_m3Type_i32, &a); ఒకవేళ (r) తిరిగి r;  // val
                r = v_తీయి_expect(v, addrType, &a); ఒకవేళ (r) తిరిగి r;      // dst
                ఆపు;
            }
#ఒకవేళ d_m3HasWideArithmetic
            స్థితి 0x13: స్థితి 0x14: // i64.add128, i64.sub128
            స్థితి 0x15: స్థితి 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 అంకె_Operands = (sub <= 0x14) ? 4 : 2;
                కొరకు (u32 i = 0; i < అంకె_Operands; ++i) {
                    r = v_తీయి_expect(v, c_m3Type_i64, &a); ఒకవేళ (r) తిరిగి r;
                }
                r = v_నెట్టు(v, c_m3Type_i64); ఒకవేళ (r) తిరిగి r;
                r = v_నెట్టు(v, c_m3Type_i64); ఒకవేళ (r) తిరిగి r;
                ఆపు;
            }
#అయితే_ముగింపు
            అప్రమేయ:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                ఆపు;
            }
            ఆపు;
        }

#ఒకవేళ d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        స్థితి 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasm_చివర);
            ఒకవేళ (r) తిరిగి r;

            ఒకవేళ (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                ఒకవేళ (reserved != 0) తిరిగి m3Err_wasmMalformed;
                ఆపు;
            }

            u32 వెడల్పు_2, అంకె_Operands; m3type_t విలువ_Type, ఫలితం_Type;
            ఒకవేళ (not v_atomic_shape(sub, &వెడల్పు_2, &అంకె_Operands, &విలువ_Type, &ఫలితం_Type)) తిరిగి m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = Read_జ్ఞాపకం_Arg(&align, &memidx, &offset, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            ఒకవేళ ((1u << align) != వెడల్పు_2) తిరిగి m3Err_invalidAtomicAlignment;
            ఒకవేళ (not v_has_జ్ఞాపకం_idx(v, memidx)) తిరిగి m3_Err_unknown_జ్ఞాపకం;
            ఒకవేళ (not v_స్థానభ్రంశం_లోపల_range(v, memidx, offset)) తిరిగి m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            ఒకవేళ (sub == 0x01 or sub == 0x02) {
                r = v_తీయి_expect(v, c_m3Type_i64, &a); ఒకవేళ (r) తిరిగి r;
                అంకె_Operands--;
            }
            కొరకు (u32 i = 1; i < అంకె_Operands; ++i) {
                r = v_తీయి_expect(v, విలువ_Type, &a); ఒకవేళ (r) తిరిగి r;
            }
            r = v_తీయి_expect(v, v_జ్ఞాపకం_addrtype(v, memidx), &a); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (ఫలితం_Type != c_m3Type_none) {
                r = v_నెట్టు(v, ఫలితం_Type); ఒకవేళ (r) తిరిగి r;
            }
            ఆపు;
        }
#అయితే_ముగింపు

#ఒకవేళ d_m3_Has_దొంతర_Switching
        // ---- Stack Switching ----
        స్థితి 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            ఒకవేళ (!ft || !ft->isContinuation) తిరిగి m3Err_typeMismatch;
            r = v_తీయి_expect(v, c_m3Type_funcref, &a); ఒకవేళ (r) తిరిగి r;
            r = v_నెట్టు(v, c_m3Type_contref); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (typeIdx1 >= v->module->అంకె_Func_Types || typeIdx2 >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            ఒకవేళ (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) తిరిగి m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 అంకె_Params1 = ft1->అంకె_వాదనలు;
            u16 అంకె_Params2 = ft2->అంకె_వాదనలు;
            ఒకవేళ (అంకె_Params1 < అంకె_Params2) తిరిగి m3Err_typeMismatch;
            u32 అంకె_Bound = అంకె_Params1 - అంకె_Params2;

            ఒకవేళ (ft1->అంకె_Rets != ft2->అంకె_Rets) తిరిగి m3Err_typeMismatch;
            కొరకు (u16 i = 0; i < ft1->అంకె_Rets; ++i) {
                ఒకవేళ (ft1->types[i] != ft2->types[i]) తిరిగి m3Err_typeMismatch;
            }
            కొరకు (u16 i = 0; i < అంకె_Params2; ++i) {
                ఒకవేళ (ft1->types[ft1->అంకె_Rets + అంకె_Bound + i] != ft2->types[ft2->అంకె_Rets + i]) తిరిగి m3Err_typeMismatch;
            }

            r = v_తీయి_expect(v, c_m3Type_contref, &a); ఒకవేళ (r) తిరిగి r;

            కొరకు (u32 i = అంకె_Bound; i > 0; i--) {
                r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft1->types[ft1->అంకె_Rets + i - 1]), &a);
                ఒకవేళ (r) తిరిగి r;
            }

            r = v_నెట్టు(v, c_m3Type_contref); ఒకవేళ (r) తిరిగి r;
            ఆపు;
        }

        స్థితి 0xe2: // suspend
        {
            u32 ట్యాగ్_Idx;
            r = ReadLEB_u32(&ట్యాగ్_Idx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (!v->module || ట్యాగ్_Idx >= v->module->అంకె_ట్యాగులు) తిరిగి m3_Err_unknown_ట్యాగ్;
            IM3FuncType tt = v->module->ట్యాగులు[ట్యాగ్_Idx].type;
            ఒకవేళ (tt) {
                కొరకు (u16 i = tt->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(tt->types[tt->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
                కొరకు (u16 i = 0; i < tt->అంకె_Rets; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(tt->types[i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ఒకవేళ (!ct || !ct->isContinuation) తిరిగి m3Err_typeMismatch;

            u32 అంకె_Clauses;
            r = ReadLEB_u32(&అంకె_Clauses, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;

            కొరకు (u32 i = 0; i < అంకె_Clauses; i++) {
                u8 రకం;
                r = Read_u8(&రకం, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                u32 ట్యాగ్_Idx;
                r = ReadLEB_u32(&ట్యాగ్_Idx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                r = v_resume_clause(v, రకం, ట్యాగ్_Idx, ct); ఒకవేళ (r) తిరిగి r;
            }

            r = v_తీయి_expect(v, c_m3Type_contref, &a); ఒకవేళ (r) తిరిగి r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ఒకవేళ (ft) {
                కొరకు (u16 i = ft->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[ft->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
                కొరకు (u16 i = 0; i < ft->అంకె_Rets; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(ft->types[i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0xe4: // resume_throw
        {
            u32 typeIdx, ట్యాగ్_Idx, అంకె_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            r = ReadLEB_u32(&ట్యాగ్_Idx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            r = ReadLEB_u32(&అంకె_Clauses, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;

            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            ఒకవేళ (!v->module || ట్యాగ్_Idx >= v->module->అంకె_ట్యాగులు) తిరిగి m3_Err_unknown_ట్యాగ్;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ఒకవేళ (!ct || !ct->isContinuation) తిరిగి m3Err_typeMismatch;
            IM3FuncType ట్యాగ్_Type = v->module->ట్యాగులు[ట్యాగ్_Idx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            ఒకవేళ (ట్యాగ్_Type && ట్యాగ్_Type->అంకె_Rets != 0) తిరిగి m3Err_typeMismatch;

            కొరకు (u32 i = 0; i < అంకె_Clauses; i++) {
                u8 రకం;
                r = Read_u8(&రకం, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                u32 c_ట్యాగ్_Idx;
                r = ReadLEB_u32(&c_ట్యాగ్_Idx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                r = v_resume_clause(v, రకం, c_ట్యాగ్_Idx, ct); ఒకవేళ (r) తిరిగి r;
            }

            r = v_తీయి_expect(v, c_m3Type_contref, &a); ఒకవేళ (r) తిరిగి r;

            ఒకవేళ (ట్యాగ్_Type) {
                కొరకు (u16 i = ట్యాగ్_Type->అంకె_వాదనలు; i > 0; i--) {
                    r = v_తీయి_expect(v, పునాది_Type_యొక్క(ట్యాగ్_Type->types[ట్యాగ్_Type->అంకె_Rets + i - 1]), &a);
                    ఒకవేళ (r) తిరిగి r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ఒకవేళ (ft) {
                కొరకు (u16 i = 0; i < ft->అంకె_Rets; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(ft->types[i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0xe5: // resume_throw_ref
        {
            u32 typeIdx, అంకె_Clauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            r = ReadLEB_u32(&అంకె_Clauses, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;

            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ఒకవేళ (!ct || !ct->isContinuation) తిరిగి m3Err_typeMismatch;

            కొరకు (u32 i = 0; i < అంకె_Clauses; i++) {
                u8 రకం;
                r = Read_u8(&రకం, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                u32 c_ట్యాగ్_Idx;
                r = ReadLEB_u32(&c_ట్యాగ్_Idx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
                r = v_resume_clause(v, రకం, c_ట్యాగ్_Idx, ct); ఒకవేళ (r) తిరిగి r;
            }

            r = v_తీయి_expect(v, c_m3Type_contref, &a); ఒకవేళ (r) తిరిగి r;
            r = v_తీయి_expect(v, c_m3Type_exnref, &a); ఒకవేళ (r) తిరిగి r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            ఒకవేళ (ft) {
                కొరకు (u16 i = 0; i < ft->అంకె_Rets; i++) {
                    r = v_నెట్టు(v, పునాది_Type_యొక్క(ft->types[i]));
                    ఒకవేళ (r) తిరిగి r;
                }
            }
            ఆపు;
        }

        స్థితి 0xe6: // switch
        {
            u32 typeIdx, ట్యాగ్_Idx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;
            r = ReadLEB_u32(&ట్యాగ్_Idx, &v->wasm, v->wasm_చివర); ఒకవేళ (r) తిరిగి r;

            ఒకవేళ (typeIdx >= v->module->అంకె_Func_Types) తిరిగి m3Err_unknownType;
            ఒకవేళ (!v->module || ట్యాగ్_Idx >= v->module->అంకె_ట్యాగులు) తిరిగి m3_Err_unknown_ట్యాగ్;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            ఒకవేళ (!ct || !ct->isContinuation) తిరిగి m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType ట్యాగ్_Type = v->module->ట్యాగులు[ట్యాగ్_Idx].type;
            ఒకవేళ (ట్యాగ్_Type && ట్యాగ్_Type->అంకె_వాదనలు != 0) తిరిగి m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            ఒకవేళ (!ft || ft->అంకె_వాదనలు == 0) తిరిగి m3Err_typeMismatch;

            IM3FuncType peer = Module_Cont_Type_యొక్క_Ref(v->module, ft->types[ft->అంకె_Rets + ft->అంకె_వాదనలు - 1]);
            ఒకవేళ (!peer || !peer->contFuncType) తిరిగి m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_తీయి_expect(v, c_m3Type_contref, &a); ఒకవేళ (r) తిరిగి r;

            కొరకు (u16 i = (u16)(ft->అంకె_వాదనలు - 1); i > 0; i--) {
                r = v_తీయి_expect(v, పునాది_Type_యొక్క(ft->types[ft->అంకె_Rets + i - 1]), &a);
                ఒకవేళ (r) తిరిగి r;
            }

            కొరకు (u16 i = 0; i < self->అంకె_వాదనలు; i++) {
                r = v_నెట్టు(v, పునాది_Type_యొక్క(self->types[self->అంకె_Rets + i]));
                ఒకవేళ (r) తిరిగి r;
            }
            ఆపు;
        }

#అయితే_ముగింపు // d_m3HasStackSwitching

        అప్రమేయ:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            ఆపు;

        } // switch

        ఒకవేళ (r) తిరిగి r;

    } // while

    // If we ran out of bytes without hitting the final end
    తిరిగి m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3_ఫలితం  ValidateFunction  (IM3Function i_function)
{
    ఒకవేళ (!i_function->wasm) తిరిగి m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    ఒకవేళ (!v) {
        v = m3_AllocStruct (ValCtx);
        ఒకవేళ (!v) తిరిగి m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, పరిమాణం(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasm_చివర  = i_function->wasm_చివర;

    // Skip code size LEB
    u32 కొలత;
    M3_ఫలితం r = ReadLEB_u32(&కొలత, &v->wasm, v->wasm_చివర);
    ఒకవేళ (r) తిరిగి r;

    // Parse locals
    u32 అంకె_Local_ఖండాలు;
    r = ReadLEB_u32(&అంకె_Local_ఖండాలు, &v->wasm, v->wasm_చివర);
    ఒకవేళ (r) తిరిగి r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 అంకె_Params = funcType ? funcType->అంకె_వాదనలు : 0;
    ఒకవేళ (అంకె_Params > d_m3_Val_దొంతర) తిరిగి m3_Err_function_దొంతర_అధికప్రవాహం;
    కొరకు (u16 i = 0; i < అంకె_Params; i++) {
        v->localTypes[v->అంకె_Locals++] = పునాది_Type_యొక్క(funcType->types[funcType->అంకె_Rets + i]);
    }

    // Then: declared locals
    కొరకు (u32 b = 0; b < అంకె_Local_ఖండాలు; b++) {
        u32 లెక్క;
        r = ReadLEB_u32(&లెక్క, &v->wasm, v->wasm_చివర);
        ఒకవేళ (r) తిరిగి r;
        m3type_t localType;
        r = Parse_విలువ_Type(v->module, &localType, &v->wasm, v->wasm_చివర);
        ఒకవేళ (r) తిరిగి r;
        u8 normalized = పునాది_Type_యొక్క(localType);
        ఒకవేళ (లెక్క > (u32) (d_m3_Val_దొంతర - v->అంకె_Locals)) తిరిగి m3_Err_function_దొంతర_అధికప్రవాహం;
        కొరకు (u32 c = 0; c < లెక్క; c++) {
            v->localTypes[v->అంకె_Locals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_నెట్టు_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    ఒకవేళ (r) తిరిగి r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    ఒకవేళ (r) తిరిగి r;

    // After validation, control stack should be empty and all declared bytes consumed
    ఒకవేళ (v->ctrl_శిఖరం != 0)
        తిరిగి m3Err_wasmMalformed;

    ఒకవేళ (v->wasm != v->wasm_చివర)
        తిరిగి m3Err_wasmSectionUnderrun;

    తిరిగి m3Err_none;
}

#లేకపోతే // !d_m3EnableValidation

M3_ఫలితం  ValidateFunction  (IM3Function i_function)
{
    (శూన్య)i_function;
    తిరిగి m3Err_none;
}

// clang-format on

#అయితే_ముగింపు // d_m3EnableValidation
