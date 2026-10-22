//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#include "m3_validate.h"
#include "m3_exception.h"
#include "m3_info.h"

#nếu d_m3EnableValidation

// clang-format off

// The spec's bottom type: an operand of unknown type on an unreachable stack,
// which unifies with every concrete type. Deliberately not c_m3Type_unknown -
// that one means "invalid type" and must never be accepted by a type check.
#define c_valBottom         0xFF

// ---------- Control frame ----------

typedef struct {
    m3opcode_t  opcode;
    u16         height;         // operand stack height at block entry
    u16         param_count;
    u16         result_count;
    IM3FuncType type;           // block type (for params/results)
    bool        is_unreachable;
} ValCtrlFrame;

// ---------- Validator context ----------

struct ValCtx {
    bytes_t     wasm;
    bytes_t     wasmEnd;
    IM3Module   module;
    IM3Function function;

    m3type_t    opd [d_m3ValStack];
    u16         opdTop;

    ValCtrlFrame ctrl [d_m3ValCtrlDepth];
    u16          ctrlTop;

    m3type_t    localTypes [d_m3ValStack];
    u16         numLocals;
};

// A memory op is only valid if the memory it names is in the module's index
// space; for a module that declares none, every index is out of range.
static bool v_has_memory_idx (ValCtx * v, u32 i_memoryIdx)
{
    trả_về v->module and i_memoryIdx < v->module->numMemories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
static m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    trả_về Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
static bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    trả_về v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
static m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    trả_về Table_AddrType (v->module->tables [i_tableIdx]);
}

// Spec: the alignment immediate of a memory access must not be larger than the
// natural alignment of the operation. Natural alignment: 8-bit=0, 16-bit=1,
// 32-bit=2, 64-bit=3.
static u32 v_max_align (m3opcode_t opcode)
{
    switch (opcode) {
    case 0x2c: case 0x2d:   // i32.load8_s, i32.load8_u
    case 0x30: case 0x31:   // i64.load8_s, i64.load8_u
    case 0x3a:              // i32.store8
    case 0x3c:              // i64.store8
        trả_về 0;
    case 0x2e: case 0x2f:   // i32.load16_s, i32.load16_u
    case 0x32: case 0x33:   // i64.load16_s, i64.load16_u
    case 0x3b:              // i32.store16
    case 0x3d:              // i64.store16
        trả_về 1;
    case 0x29:              // i64.load
    case 0x2b:              // f64.load
    case 0x37:              // i64.store
    case 0x39:              // f64.store
        trả_về 3;
    default:                // 32-bit accesses, and a safe fallback
        trả_về 2;
    }
}

#nếu d_m3HasAtomics

// What an instruction of the 0xFE prefix (other than atomic.fence) touches and
// takes: the width in bytes of the cell it addresses, how many operands it pops
// counting the address, the type of every operand but the address - a wait's
// timeout aside - and what it pushes. False for a sub-opcode that is not one.
//
// Outside notify and wait, the accesses come in runs of seven, one per pairing of
// value type and cell width: i32/32, i64/64, i32/8, i32/16, i64/8, i64/16, i64/32.
static bool v_atomic_shape (u32 sub, u32 * o_width, u32 * o_numOperands, m3type_t * o_valueType, m3type_t * o_resultType)
{
    static const u8 c_widths [7] = { 4, 8, 1, 2, 1, 2, 4 };
    static const m3type_t c_types [7] = {
        c_m3Type_i32, c_m3Type_i64,
        c_m3Type_i32, c_m3Type_i32,
        c_m3Type_i64, c_m3Type_i64, c_m3Type_i64
    };

    u32 variant;

    nếu (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_numOperands = (sub == 0x00) ? 2 : 3;
        *o_valueType   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        trả_về true;
    }

    nếu (sub < 0x10 or sub > c_waOp_lastAtomic) trả_về false;

    nếu (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_numOperands = 1;
    } không_thì nếu (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_numOperands = 2;
    } không_thì nếu (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_numOperands = 2;
    } không_thì {                        // cmpxchg
        variant = sub - 0x48;       *o_numOperands = 3;
    }

    *o_width      = c_widths [variant];
    *o_valueType  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    trả_về true;
}

#endif

// ---------- Operand stack ----------

static M3Result v_push (ValCtx * v, m3type_t type)
{
    nếu (M3_UNLIKELY(v->opdTop >= d_m3ValStack))
        trả_về m3Err_functionStackOverflow;
    v->opd[v->opdTop++] = type;
    trả_về m3Err_none;
}

static M3Result v_pop (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop - 1];
    nếu (M3_UNLIKELY(v->opdTop == f->height)) {
        nếu (f->is_unreachable) { *o_type = c_valBottom; trả_về m3Err_none; }
        trả_về m3Err_functionStackUnderrun;
    }
    *o_type = v->opd[--v->opdTop];
    trả_về m3Err_none;
}

static M3Result v_pop_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_pop(v, &actual);
    nếu (M3_UNLIKELY(r)) trả_về r;
    nếu (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        trả_về m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    trả_về m3Err_none;
}

// ---------- Control stack ----------

static M3Result v_push_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    nếu (M3_UNLIKELY(v->ctrlTop >= d_m3ValCtrlDepth))
        trả_về m3Err_functionStackOverflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop++];
    f->opcode       = op;
    f->type         = type;
    f->param_count  = type ? type->numArgs : 0;
    f->result_count = type ? type->numRets : 0;
    f->height       = v->opdTop;
    f->is_unreachable = false;
    trả_về m3Err_none;
}

static M3Result v_pop_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    nếu (M3_UNLIKELY(v->ctrlTop == 0))
        trả_về m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop - 1];
    // pop result types
    nếu (f->type) {
        cho (u16 i = f->result_count; i > 0; i--) {
            m3type_t a;
            M3Result r = v_pop_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            nếu (M3_UNLIKELY(r)) trả_về r;
        }
    }
    nếu (M3_UNLIKELY(v->opdTop != f->height))
        trả_về m3Err_typeCountMismatch;
    nếu (o_frame) *o_frame = *f;
    v->ctrlTop--;
    trả_về m3Err_none;
}

static void v_unreachable (ValCtx * v)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop - 1];
    v->opdTop = f->height;
    f->is_unreachable = true;
}

// Label types: loop -> params, block/if/else/func -> results
static u16 v_label_n (ValCtrlFrame * f)
{
    trả_về (f->opcode == 0x03) ? f->param_count : f->result_count;
}

static m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    nếu (!f->type) trả_về c_m3Type_none;
    nếu (f->opcode == 0x03)
        trả_về BaseTypeOf(f->type->types[f->type->numRets + i]); // params
    trả_về BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
static m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop - 1];

    nếu (v->opdTop <= i_depth || (u16)(v->opdTop - i_depth - 1) < f->height)
        trả_về c_valBottom;

    trả_về v->opd[v->opdTop - i_depth - 1];
}

// Pop label types for branch target
static M3Result v_pop_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    cho (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_pop_expect(v, v_label_t(tgt, i - 1), &a);
        nếu (M3_UNLIKELY(r)) trả_về r;
    }
    trả_về m3Err_none;
}

// Push label types back
static M3Result v_push_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    cho (u16 i = 0; i < n; i++) {
        M3Result r = v_push(v, v_label_t(tgt, i));
        nếu (M3_UNLIKELY(r)) trả_về r;
    }
    trả_về m3Err_none;
}

// ---------- Block type resolution ----------

static M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    nếu (v->wasm >= v->wasmEnd)
        trả_về m3Err_wasmUnderrun;

#nếu d_m3HasTypedRefs
    nếu (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = ParseValueType(v->module, &refType, &v->wasm, v->wasmEnd);
        nếu (rr) trả_về rr;
        IM3FuncType ftype = NULL;
        rr = AllocFuncType(&ftype, 1);
        nếu (rr) trả_về rr;
        ftype->numRets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        nếu (rr) trả_về rr;
        *o_type = ftype;
        trả_về m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    nếu (r) trả_về r;

    nếu (type < 0) {
        nếu (type < -64) trả_về m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        nếu (r) trả_về r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } không_thì {
        nếu ((u32)type >= v->module->numFuncTypes)
            trả_về m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    trả_về m3Err_none;
}

// ---------- Convenience ----------

static M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_pop_expect(v, in, &a);
    nếu (M3_UNLIKELY(r)) trả_về r;
    trả_về v_push(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
static M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : NULL;

    u16 numCalleeRets = i_calleeType ? i_calleeType->numRets : 0;
    u16 numFuncRets   = ft ? ft->numRets : 0;

    nếu (numCalleeRets != numFuncRets)
        trả_về m3Err_typeMismatch;

    cho (u16 i = 0; i < numCalleeRets; i++) {
        nếu (i_calleeType->types[i] != ft->types[i])
            trả_về m3Err_typeMismatch;
    }

    trả_về m3Err_none;
}

static M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_pop_expect(v, t, &a); nếu (M3_UNLIKELY(r)) trả_về r;
    r = v_pop_expect(v, t, &a); nếu (M3_UNLIKELY(r)) trả_về r;
    trả_về v_push(v, t);
}

static M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_pop_expect(v, t, &a); nếu (M3_UNLIKELY(r)) trả_về r;
    r = v_pop_expect(v, t, &a); nếu (M3_UNLIKELY(r)) trả_về r;
    trả_về v_push(v, c_m3Type_i32);
}

static M3Result v_testop (ValCtx * v, m3type_t t)
{
    trả_về v_unop(v, t, c_m3Type_i32);
}

static M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    trả_về v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#nếu d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
static M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    nếu (r) trả_về r;
    nếu (kind > 0x03) trả_về m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = NULL;
    nếu (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        nếu (r) trả_về r;
        nếu (!v->module || tagIndex >= v->module->numTags) trả_về m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    nếu (r) trả_về r;
    nếu (depth >= v->ctrlTop) trả_về m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];

    u16 numPayload = tagType ? tagType->numArgs : 0;
    u16 numLabel   = v_label_n(tgt);

    nếu (numLabel != numPayload + (hasRef ? 1u : 0u))
        trả_về m3Err_typeCountMismatch;

    cho (u16 i = 0; i < numPayload; i++) {
        nếu (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->numRets + i]))
            trả_về m3Err_typeMismatch;
    }

    nếu (hasRef and v_label_t(tgt, numPayload) != c_m3Type_exnref)
        trả_về m3Err_typeMismatch;

    trả_về m3Err_none;
}

#endif // d_m3HasExceptionHandling

#nếu d_m3HasStackSwitching

static M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    nếu (kind > 0x01) trả_về m3Err_wasmMalformed;
    nếu (!v->module || tagIdx >= v->module->numTags) trả_về m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    nếu (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
        nếu (depth >= v->ctrlTop) trả_về m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];
        u16 numPayload = tagType ? tagType->numArgs : 0;
        u16 numLabel   = v_label_n(tgt);

        nếu (numLabel != numPayload + 1u) trả_về m3Err_typeCountMismatch;

        cho (u16 j = 0; j < numPayload; j++) {
            nếu (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->numRets + j]))
                trả_về m3Err_typeMismatch;
        }
        nếu (v_label_t(tgt, numPayload) != c_m3Type_contref)
            trả_về m3Err_typeMismatch;

#nếu d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->numRets + numPayload]
            : tgt->type->types[numPayload];

        nếu (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            trả_về m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = NULL;
        cho (u32 idx = 0; idx < v->module->numFuncTypes; ++idx) {
            nếu (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                break;
            }
        }
        nếu (!labelCt) trả_về m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expectedNumParams = tagType ? tagType->numRets : 0;
        nếu (labelFt->numArgs != expectedNumParams) trả_về m3Err_typeMismatch;
        cho (u16 p = 0; p < expectedNumParams; ++p) {
            nếu (labelFt->types[labelFt->numRets + p] != tagType->types[p])
                trả_về m3Err_typeMismatch;
        }

        u16 expectedNumRets = ft ? ft->numRets : 0;
        nếu (labelFt->numRets != expectedNumRets) trả_về m3Err_typeMismatch;
        cho (u16 r_idx = 0; r_idx < expectedNumRets; ++r_idx) {
            nếu (labelFt->types[r_idx] != ft->types[r_idx])
                trả_về m3Err_typeMismatch;
        }
#endif
    } không_thì {
        nếu (tagType && tagType->numArgs != 0) trả_về m3Err_typeMismatch;
    }
    trả_về m3Err_none;
}

#endif // d_m3HasStackSwitching

static M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    trong_khi (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        nếu (M3_UNLIKELY(r)) trả_về r;

        switch (opcode)
        {
        // ---- Control ----
        case 0x00: // unreachable
            v_unreachable(v);
            break;

        case 0x01: // nop
            break;

        case 0x02: // block
        case 0x03: // loop
        case 0x04: // if
        {
            IM3FuncType bt = NULL;
            r = v_read_blocktype(v, &bt);
            nếu (r) trả_về r;
            nếu (opcode == 0x04) {
                r = v_pop_expect(v, c_m3Type_i32, &a);
                nếu (r) trả_về r;
            }
            // Pop block params from caller stack
            nếu (bt) {
                cho (u16 i = bt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(bt->types[bt->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
            }
            r = v_push_ctrl(v, opcode, bt);
            nếu (r) trả_về r;
            // Push params inside block
            nếu (bt) {
                cho (u16 i = 0; i < bt->numArgs; i++) {
                    r = v_push(v, BaseTypeOf(bt->types[bt->numRets + i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

#nếu d_m3HasExceptionHandling
        case 0x1f: // try_table
        {
            IM3FuncType bt = NULL;
            r = v_read_blocktype(v, &bt);
            nếu (r) trả_về r;
            nếu (bt) {
                cho (u16 i = bt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(bt->types[bt->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 numCatch;
            r = ReadLEB_u32(&numCatch, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            cho (u32 i = 0; i < numCatch; i++) {
                r = v_catch_clause(v);
                nếu (r) trả_về r;
            }

            r = v_push_ctrl(v, opcode, bt);
            nếu (r) trả_về r;

            nếu (bt) {
                cho (u16 i = 0; i < bt->numArgs; i++) {
                    r = v_push(v, BaseTypeOf(bt->types[bt->numRets + i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (!v->module || tagIndex >= v->module->numTags) trả_về m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            nếu (tt) {
                cho (u16 i = tt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(tt->types[tt->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
            }
            v_unreachable(v);
            break;
        }

        case 0x0a: // throw_ref
            r = v_pop_expect(v, c_m3Type_exnref, &a);
            nếu (r) trả_về r;
            v_unreachable(v);
            break;
#endif // d_m3HasExceptionHandling

        case 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_pop_ctrl(v, &frame);
            nếu (r) trả_về r;
            nếu (frame.opcode != 0x04)
                trả_về m3Err_wasmMalformed;
            r = v_push_ctrl(v, 0x05, frame.type);
            nếu (r) trả_về r;
            nếu (frame.type) {
                cho (u16 i = 0; i < frame.type->numArgs; i++) {
                    r = v_push(v, BaseTypeOf(frame.type->types[frame.type->numRets + i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_pop_ctrl(v, &frame);
            nếu (r) trả_về r;
            // Push results
            nếu (frame.type) {
                cho (u16 i = 0; i < frame.result_count; i++) {
                    r = v_push(v, BaseTypeOf(frame.type->types[i]));
                    nếu (r) trả_về r;
                }
            }
            // If this was the outermost frame, we're done
            nếu (v->ctrlTop == 0)
                trả_về m3Err_none;
            break;
        }

        case 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (depth >= v->ctrlTop) trả_về m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];
            r = v_pop_labels(v, tgt);
            nếu (r) trả_về r;
            v_unreachable(v);
            break;
        }

        case 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (depth >= v->ctrlTop) trả_về m3Err_unknownLabel;
            r = v_pop_expect(v, c_m3Type_i32, &a);
            nếu (r) trả_về r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];
            r = v_pop_labels(v, tgt);
            nếu (r) trả_về r;
            r = v_push_labels(v, tgt);
            nếu (r) trả_về r;
            break;
        }

        case 0x0e: // br_table
        {
            u32 count;
            r = ReadLEB_u32(&count, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            cho (u32 i = 0; i <= count; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                nếu (r) trả_về r;
                nếu (d >= v->ctrlTop) trả_về m3Err_unknownLabel;
                nếu (i == count) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrlTop - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_pop_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            nếu (r) trả_về r;

            cho (u32 i = 0; i <= count; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                nếu (r) trả_về r;
                ValCtrlFrame * t = &v->ctrl[v->ctrlTop - 1 - d];
                u16 n = v_label_n(t);
                nếu (n != arity) trả_về m3Err_typeCountMismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                cho (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    nếu (want != c_valBottom && have != c_valBottom && want != have)
                        trả_về m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrlTop - 1 - defDepth];
            r = v_pop_labels(v, dt);
            nếu (r) trả_về r;
            v_unreachable(v);
            break;
        }

        case 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            nếu (ft) {
                cho (u16 i = ft->numRets; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    nếu (r) trả_về r;
                }
            }
            v_unreachable(v);
            break;
        }

        // ---- Call ----
        case 0x10: // call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (idx >= v->module->numFunctions) trả_về m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            nếu (ft) {
                cho (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
                cho (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            // the table must exist and hold funcrefs
            nếu (tableIdx >= v->module->numTables) trả_về m3Err_unknownTable;
            nếu (v->module->tables[tableIdx]->type != c_m3Type_funcref) trả_về m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_pop_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            nếu (r) trả_về r;
            nếu (ft) {
                cho (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
                cho (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (idx >= v->module->numFunctions) trả_về m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            nếu (r) trả_về r;
            nếu (ft) {
                cho (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
            }
            v_unreachable(v);
            break;
        }

        case 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            // the table must exist and hold funcrefs
            nếu (tableIdx >= v->module->numTables) trả_về m3Err_unknownTable;
            nếu (v->module->tables[tableIdx]->type != c_m3Type_funcref) trả_về m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            nếu (r) trả_về r;
            r = v_pop_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            nếu (r) trả_về r;
            nếu (ft) {
                cho (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
            }
            v_unreachable(v);
            break;
        }

#nếu d_m3HasTypedRefs
        case 0x14: // call_ref
        case 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            nếu (isTail) {
                r = v_check_tail_results(v, ft);
                nếu (r) trả_về r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_pop_expect(v, c_m3Type_funcref, &a);
            nếu (r) trả_về r;
            nếu (ft) {
                cho (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
                nếu (not isTail) {
                    cho (u16 i = 0; i < ft->numRets; i++) {
                        r = v_push(v, BaseTypeOf(ft->types[i]));
                        nếu (r) trả_về r;
                    }
                }
            }
            nếu (isTail) v_unreachable(v);
            break;
        }

        case 0xd4: // ref.as_non_null
        {
            r = v_pop_expect(v, c_m3Type_funcref, &a);
            nếu (r) trả_về r;
            r = v_push(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            nếu (r) trả_về r;
            break;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        case 0x1a: // drop
            r = v_pop(v, &a);
            nếu (r) trả_về r;
            break;

        case 0x1b: // select
        {
            r = v_pop_expect(v, c_m3Type_i32, &a);
            nếu (r) trả_về r;
            m3type_t t2 = c_valBottom;
            r = v_pop(v, &t2);
            nếu (r) trả_về r;
            m3type_t t1;
            r = v_pop_expect(v, t2, &t1);
            nếu (r) trả_về r;
            // untyped select is numeric only; references need the 0x1c form
            nếu (t2 != c_valBottom && IsRefType(t2)) trả_về m3Err_typeMismatch;
            r = v_push(v, (t2 == c_valBottom) ? t1 : t2);
            nếu (r) trả_về r;
            break;
        }

#nếu d_m3HasRefTypes
        case 0x1c: // select with an explicit result type
        {
            u32 numTypes;
            r = ReadLEB_u32(&numTypes, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (numTypes != 1) trả_về m3Err_wasmMalformed;

            m3type_t selType;
            r = ParseValueType(v->module, &selType, &v->wasm, v->wasmEnd);  nếu (r) trả_về r;
            u8 t = BaseTypeOf(selType);

            r = v_pop_expect(v, c_m3Type_i32, &a); nếu (r) trả_về r;
            r = v_pop_expect(v, t, &a);            nếu (r) trả_về r;
            r = v_pop_expect(v, t, &a);            nếu (r) trả_về r;
            r = v_push(v, t);                      nếu (r) trả_về r;
            break;
        }

        case 0x25: // table.get
        case 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (tableIdx >= v->module->numTables) trả_về m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            nếu (opcode == 0x26) {
                r = v_pop_expect(v, t, &a);   nếu (r) trả_về r;
                r = v_pop_expect(v, at, &a);  nếu (r) trả_về r;
            } không_thì {
                r = v_pop_expect(v, at, &a);  nếu (r) trả_về r;
                r = v_push(v, t);             nếu (r) trả_về r;
            }
            break;
        }

        case 0xd0: // ref.null
        {
#nếu d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  nếu (r) trả_về r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# nếu d_m3HasStackSwitching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#không_thì
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   nếu (r) trả_về r;
            r = NormalizeType(&t, waType);                   nếu (r) trả_về r;
            nếu (!IsRefType(t)) trả_về m3Err_wasmMalformed;
#endif
            r = v_push(v, t);                                nếu (r) trả_về r;
            break;
        }

        case 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_pop(v, &t);
            nếu (r) trả_về r;
            nếu (t != c_valBottom && !IsRefType(t)) trả_về m3Err_typeMismatch;
            r = v_push(v, c_m3Type_i32);
            nếu (r) trả_về r;
            break;
        }

        case 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (funcIdx >= v->module->numFunctions) trả_về m3Err_unknownFunction;
            nếu (!Module_IsFunctionDeclared(v->module, funcIdx)) trả_về m3Err_undeclaredFuncRef;
            r = v_push(v, c_m3Type_funcref);
            nếu (r) trả_về r;
            break;
        }
#endif

        // ---- Variable ----
        case 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (idx >= v->numLocals) trả_về m3Err_unknownLocal;
            r = v_push(v, v->localTypes[idx]);
            nếu (r) trả_về r;
            break;
        }

        case 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (idx >= v->numLocals) trả_về m3Err_unknownLocal;
            r = v_pop_expect(v, v->localTypes[idx], &a);
            nếu (r) trả_về r;
            break;
        }

        case 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (idx >= v->numLocals) trả_về m3Err_unknownLocal;
            r = v_pop_expect(v, v->localTypes[idx], &a);
            nếu (r) trả_về r;
            r = v_push(v, v->localTypes[idx]);
            nếu (r) trả_về r;
            break;
        }

        case 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (idx >= v->module->numGlobals) trả_về m3Err_unknownGlobal;
            r = v_push(v, BaseTypeOf(v->module->globals[idx].type));
            nếu (r) trả_về r;
            break;
        }

        case 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            nếu (idx >= v->module->numGlobals) trả_về m3Err_unknownGlobal;
            r = v_pop_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            nếu (r) trả_về r;
            break;
        }

        // ---- Memory load ----
        case 0x28: case 0x29: case 0x2a: case 0x2b: // i32/i64/f32/f64.load
        case 0x2c: case 0x2d: case 0x2e: case 0x2f: // i32.load8/16 s/u
        case 0x30: case 0x31: case 0x32: case 0x33: // i64.load8/16 s/u
        case 0x34: case 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (align > v_max_align(opcode)) trả_về m3Err_invalidAlignment;
            nếu (not v_has_memory_idx(v, memidx)) trả_về m3Err_unknownMemory;
            nếu (not v_offset_in_range(v, memidx, offset)) trả_về m3Err_wasmMalformed;
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); nếu (r) trả_về r;
            u8 result;
            nếu      (opcode == 0x28) result = c_m3Type_i32;
            không_thì nếu (opcode == 0x29) result = c_m3Type_i64;
            không_thì nếu (opcode == 0x2a) result = c_m3Type_f32;
            không_thì nếu (opcode == 0x2b) result = c_m3Type_f64;
            không_thì nếu (opcode <= 0x2f) result = c_m3Type_i32;
            không_thì                     result = c_m3Type_i64;
            r = v_push(v, result);
            nếu (r) trả_về r;
            break;
        }

        // ---- Memory store ----
        case 0x36: case 0x37: case 0x38: case 0x39: // i32/i64/f32/f64.store
        case 0x3a: case 0x3b:                         // i32.store8/16
        case 0x3c: case 0x3d: case 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (align > v_max_align(opcode)) trả_về m3Err_invalidAlignment;
            nếu (not v_has_memory_idx(v, memidx)) trả_về m3Err_unknownMemory;
            nếu (not v_offset_in_range(v, memidx, offset)) trả_về m3Err_wasmMalformed;
            u8 valtype;
            nếu      (opcode == 0x36) valtype = c_m3Type_i32;
            không_thì nếu (opcode == 0x37) valtype = c_m3Type_i64;
            không_thì nếu (opcode == 0x38) valtype = c_m3Type_f32;
            không_thì nếu (opcode == 0x39) valtype = c_m3Type_f64;
            không_thì nếu (opcode <= 0x3b) valtype = c_m3Type_i32;
            không_thì                     valtype = c_m3Type_i64;
            r = v_pop_expect(v, valtype, &a); nếu (r) trả_về r;
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); nếu (r) trả_về r;
            break;
        }

        // ---- Memory size/grow ----
        case 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (not v_has_memory_idx(v, memidx)) trả_về m3Err_unknownMemory;
            r = v_push(v, v_memory_addrtype(v, memidx)); nếu (r) trả_về r;
            break;
        }
        case 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (not v_has_memory_idx(v, memidx)) trả_về m3Err_unknownMemory;
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); nếu (r) trả_về r;
            r = v_push(v, v_memory_addrtype(v, memidx)); nếu (r) trả_về r;
            break;
        }

        // ---- Constants ----
        case 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            r = v_push(v, c_m3Type_i32); nếu (r) trả_về r;
            break;
        }
        case 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            r = v_push(v, c_m3Type_i64); nếu (r) trả_về r;
            break;
        }
        case 0x43: { // f32.const
            nếu (v->wasm + 4 > v->wasmEnd) trả_về m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_push(v, c_m3Type_f32); nếu (r) trả_về r;
            break;
        }
        case 0x44: { // f64.const
            nếu (v->wasm + 8 > v->wasmEnd) trả_về m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_push(v, c_m3Type_f64); nếu (r) trả_về r;
            break;
        }


        // ---- i32 comparison ----
        case 0x45: r = v_testop(v, c_m3Type_i32); break; // i32.eqz
        case 0x46: case 0x47: case 0x48: case 0x49: case 0x4a:
        case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
            r = v_relop(v, c_m3Type_i32); break;

        // ---- i64 comparison ----
        case 0x50: r = v_testop(v, c_m3Type_i64); break; // i64.eqz
        case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
        case 0x56: case 0x57: case 0x58: case 0x59: case 0x5a:
            r = v_relop(v, c_m3Type_i64); break;

        // ---- f32 comparison ----
        case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: case 0x60:
            r = v_relop(v, c_m3Type_f32); break;

        // ---- f64 comparison ----
        case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66:
            r = v_relop(v, c_m3Type_f64); break;

        // ---- i32 unary ----
        case 0x67: case 0x68: case 0x69: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); break;

        // ---- i32 binary ----
        case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:
        case 0x76: case 0x77: case 0x78: // add..rotr
            r = v_binop(v, c_m3Type_i32); break;

        // ---- i64 unary ----
        case 0x79: case 0x7a: case 0x7b: // clz, ctz, popcnt
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); break;

        // ---- i64 binary ----
        case 0x7c: case 0x7d: case 0x7e: case 0x7f: case 0x80: case 0x81:
        case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8a: // add..rotr
            r = v_binop(v, c_m3Type_i64); break;

        // ---- f32 unary ----
        case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
        case 0x90: case 0x91: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f32, c_m3Type_f32); break;

        // ---- f32 binary ----
        case 0x92: case 0x93: case 0x94: case 0x95: case 0x96:
        case 0x97: case 0x98: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f32); break;

        // ---- f64 unary ----
        case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d:
        case 0x9e: case 0x9f: // abs, neg, ceil, floor, trunc, nearest, sqrt
            r = v_unop(v, c_m3Type_f64, c_m3Type_f64); break;

        // ---- f64 binary ----
        case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4:
        case 0xa5: case 0xa6: // add, sub, mul, div, min, max, copysign
            r = v_binop(v, c_m3Type_f64); break;

        // ---- Conversions ----
        case 0xa7: r = v_cvtop(v, c_m3Type_i64, c_m3Type_i32); break; // i32.wrap/i64
        case 0xa8: case 0xa9:   // i32.trunc_s/f32, i32.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); break;
        case 0xaa: case 0xab:   // i32.trunc_s/f64, i32.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); break;
        case 0xac: case 0xad:   // i64.extend_s/i32, i64.extend_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_i64); break;
        case 0xae: case 0xaf:   // i64.trunc_s/f32, i64.trunc_u/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); break;
        case 0xb0: case 0xb1:   // i64.trunc_s/f64, i64.trunc_u/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); break;
        case 0xb2: case 0xb3:   // f32.convert_s/i32, f32.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); break;
        case 0xb4: case 0xb5:   // f32.convert_s/i64, f32.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f32); break;
        case 0xb6:              // f32.demote/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_f32); break;
        case 0xb7: case 0xb8:   // f64.convert_s/i32, f64.convert_u/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f64); break;
        case 0xb9: case 0xba:   // f64.convert_s/i64, f64.convert_u/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); break;
        case 0xbb:              // f64.promote/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_f64); break;
        case 0xbc:              // i32.reinterpret/f32
            r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); break;
        case 0xbd:              // i64.reinterpret/f64
            r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); break;
        case 0xbe:              // f32.reinterpret/i32
            r = v_cvtop(v, c_m3Type_i32, c_m3Type_f32); break;
        case 0xbf:              // f64.reinterpret/i64
            r = v_cvtop(v, c_m3Type_i64, c_m3Type_f64); break;

        // ---- Sign-extension (MVP post) ----
        case 0xc0: case 0xc1:   // i32.extend8_s, i32.extend16_s
            r = v_unop(v, c_m3Type_i32, c_m3Type_i32); break;
        case 0xc2: case 0xc3: case 0xc4: // i64.extend8/16/32_s
            r = v_unop(v, c_m3Type_i64, c_m3Type_i64); break;

        // ---- 0xFC prefix (saturating truncations, bulk memory, wide arithmetic) ----
        case 0xfc:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;
            switch (sub) {
            case 0x00: case 0x01: // i32.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i32); break;
            case 0x02: case 0x03: // i32.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i32); break;
            case 0x04: case 0x05: // i64.trunc_sat_f32_s/u
                r = v_cvtop(v, c_m3Type_f32, c_m3Type_i64); break;
            case 0x06: case 0x07: // i64.trunc_sat_f64_s/u
                r = v_cvtop(v, c_m3Type_f64, c_m3Type_i64); break;
            case 0x08: // memory.init
            {
                u32 dataidx, memidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                nếu (not v_has_memory_idx(v, memidx)) trả_về m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                nếu (not v->module->hasDataCount) trả_về m3Err_dataCountRequired;
                nếu (dataidx >= v->module->numDataSegments) trả_về m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_pop_expect(v, c_m3Type_i32, &a); nếu (r) trả_về r; // n
                r = v_pop_expect(v, c_m3Type_i32, &a); nếu (r) trả_về r; // src
                r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); nếu (r) trả_về r; // dst
                break;
            }
            case 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                // needs the segment, but not a memory
                nếu (not v->module->hasDataCount) trả_về m3Err_dataCountRequired;
                nếu (dataidx >= v->module->numDataSegments) trả_về m3Err_unknownDataSegment;
                break;
            }
#nếu d_m3HasRefTypes
            case 0x0c: // table.init
            case 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); nếu (r) trả_về r;

                nếu (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    nếu (a2 >= v->module->numTables) trả_về m3Err_unknownTable;
                    nếu (a1 >= v->module->numElementSegments) trả_về m3Err_unknownElemSegment;
                    nếu (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        trả_về m3Err_typeMismatch;
                } không_thì {                                                    // dst, src
                    nếu (a1 >= v->module->numTables) trả_về m3Err_unknownTable;
                    nếu (a2 >= v->module->numTables) trả_về m3Err_unknownTable;
                    nếu (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        trả_về m3Err_typeMismatch;
                }

                nếu (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_pop_expect(v, c_m3Type_i32, &a); nếu (r) trả_về r;             // n
                    r = v_pop_expect(v, c_m3Type_i32, &a); nếu (r) trả_về r;             // s
                    r = v_pop_expect(v, v_table_addrtype(v, a2), &a); nếu (r) trả_về r;  // d
                } không_thì {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_pop_expect(v, nType, &a); nếu (r) trả_về r;  // n
                    r = v_pop_expect(v, sType, &a); nếu (r) trả_về r;  // s
                    r = v_pop_expect(v, dType, &a); nếu (r) trả_về r;  // d
                }
                break;
            }
            case 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                nếu (elemIdx >= v->module->numElementSegments) trả_về m3Err_unknownElemSegment;
                break;
            }
            case 0x0f: // table.grow
            case 0x10: // table.size
            case 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                nếu (tableIdx >= v->module->numTables) trả_về m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                nếu (sub == 0x10) {                                          // table.size
                    r = v_push(v, at); nếu (r) trả_về r;
                } không_thì nếu (sub == 0x0f) {                                   // table.grow
                    r = v_pop_expect(v, at, &a); nếu (r) trả_về r;  // n
                    r = v_pop_expect(v, t, &a);  nếu (r) trả_về r;  // init
                    r = v_push(v, at);           nếu (r) trả_về r;
                } không_thì {                                                    // table.fill
                    r = v_pop_expect(v, at, &a); nếu (r) trả_về r;  // n
                    r = v_pop_expect(v, t, &a);  nếu (r) trả_về r;  // val
                    r = v_pop_expect(v, at, &a); nếu (r) trả_về r;  // i
                }
                break;
            }
#endif
            case 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                nếu (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) trả_về m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_pop_expect(v, lenType, &a); nếu (r) trả_về r; // n
                r = v_pop_expect(v, srcType, &a); nếu (r) trả_về r; // src
                r = v_pop_expect(v, dstType, &a); nếu (r) trả_về r; // dst
                break;
            }
            case 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                nếu (not v_has_memory_idx(v, memidx)) trả_về m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_pop_expect(v, addrType, &a); nếu (r) trả_về r;      // n
                r = v_pop_expect(v, c_m3Type_i32, &a); nếu (r) trả_về r;  // val
                r = v_pop_expect(v, addrType, &a); nếu (r) trả_về r;      // dst
                break;
            }
#nếu d_m3HasWideArithmetic
            case 0x13: case 0x14: // i64.add128, i64.sub128
            case 0x15: case 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 numOperands = (sub <= 0x14) ? 4 : 2;
                cho (u32 i = 0; i < numOperands; ++i) {
                    r = v_pop_expect(v, c_m3Type_i64, &a); nếu (r) trả_về r;
                }
                r = v_push(v, c_m3Type_i64); nếu (r) trả_về r;
                r = v_push(v, c_m3Type_i64); nếu (r) trả_về r;
                break;
            }
#endif
            default:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                break;
            }
            break;
        }

#nếu d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        case 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            nếu (r) trả_về r;

            nếu (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                nếu (reserved != 0) trả_về m3Err_wasmMalformed;
                break;
            }

            u32 width, numOperands; m3type_t valueType, resultType;
            nếu (not v_atomic_shape(sub, &width, &numOperands, &valueType, &resultType)) trả_về m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            nếu ((1u << align) != width) trả_về m3Err_invalidAtomicAlignment;
            nếu (not v_has_memory_idx(v, memidx)) trả_về m3Err_unknownMemory;
            nếu (not v_offset_in_range(v, memidx, offset)) trả_về m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            nếu (sub == 0x01 or sub == 0x02) {
                r = v_pop_expect(v, c_m3Type_i64, &a); nếu (r) trả_về r;
                numOperands--;
            }
            cho (u32 i = 1; i < numOperands; ++i) {
                r = v_pop_expect(v, valueType, &a); nếu (r) trả_về r;
            }
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); nếu (r) trả_về r;
            nếu (resultType != c_m3Type_none) {
                r = v_push(v, resultType); nếu (r) trả_về r;
            }
            break;
        }
#endif

#nếu d_m3HasStackSwitching
        // ---- Stack Switching ----
        case 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            nếu (!ft || !ft->isContinuation) trả_về m3Err_typeMismatch;
            r = v_pop_expect(v, c_m3Type_funcref, &a); nếu (r) trả_về r;
            r = v_push(v, c_m3Type_contref); nếu (r) trả_về r;
            break;
        }

        case 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (typeIdx1 >= v->module->numFuncTypes || typeIdx2 >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            nếu (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) trả_về m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 numParams1 = ft1->numArgs;
            u16 numParams2 = ft2->numArgs;
            nếu (numParams1 < numParams2) trả_về m3Err_typeMismatch;
            u32 numBound = numParams1 - numParams2;

            nếu (ft1->numRets != ft2->numRets) trả_về m3Err_typeMismatch;
            cho (u16 i = 0; i < ft1->numRets; ++i) {
                nếu (ft1->types[i] != ft2->types[i]) trả_về m3Err_typeMismatch;
            }
            cho (u16 i = 0; i < numParams2; ++i) {
                nếu (ft1->types[ft1->numRets + numBound + i] != ft2->types[ft2->numRets + i]) trả_về m3Err_typeMismatch;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); nếu (r) trả_về r;

            cho (u32 i = numBound; i > 0; i--) {
                r = v_pop_expect(v, BaseTypeOf(ft1->types[ft1->numRets + i - 1]), &a);
                nếu (r) trả_về r;
            }

            r = v_push(v, c_m3Type_contref); nếu (r) trả_về r;
            break;
        }

        case 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (!v->module || tagIdx >= v->module->numTags) trả_về m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            nếu (tt) {
                cho (u16 i = tt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(tt->types[tt->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
                cho (u16 i = 0; i < tt->numRets; i++) {
                    r = v_push(v, BaseTypeOf(tt->types[i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            nếu (!ct || !ct->isContinuation) trả_về m3Err_typeMismatch;

            u32 numClauses;
            r = ReadLEB_u32(&numClauses, &v->wasm, v->wasmEnd); nếu (r) trả_về r;

            cho (u32 i = 0; i < numClauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                r = v_resume_clause(v, kind, tagIdx, ct); nếu (r) trả_về r;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); nếu (r) trả_về r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            nếu (ft) {
                cho (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
                cho (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, numClauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            r = ReadLEB_u32(&numClauses, &v->wasm, v->wasmEnd); nếu (r) trả_về r;

            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            nếu (!v->module || tagIdx >= v->module->numTags) trả_về m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            nếu (!ct || !ct->isContinuation) trả_về m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            nếu (tagType && tagType->numRets != 0) trả_về m3Err_typeMismatch;

            cho (u32 i = 0; i < numClauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                r = v_resume_clause(v, kind, cTagIdx, ct); nếu (r) trả_về r;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); nếu (r) trả_về r;

            nếu (tagType) {
                cho (u16 i = tagType->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(tagType->types[tagType->numRets + i - 1]), &a);
                    nếu (r) trả_về r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            nếu (ft) {
                cho (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0xe5: // resume_throw_ref
        {
            u32 typeIdx, numClauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            r = ReadLEB_u32(&numClauses, &v->wasm, v->wasmEnd); nếu (r) trả_về r;

            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            nếu (!ct || !ct->isContinuation) trả_về m3Err_typeMismatch;

            cho (u32 i = 0; i < numClauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
                r = v_resume_clause(v, kind, cTagIdx, ct); nếu (r) trả_về r;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); nếu (r) trả_về r;
            r = v_pop_expect(v, c_m3Type_exnref, &a); nếu (r) trả_về r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            nếu (ft) {
                cho (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    nếu (r) trả_về r;
                }
            }
            break;
        }

        case 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); nếu (r) trả_về r;

            nếu (typeIdx >= v->module->numFuncTypes) trả_về m3Err_unknownType;
            nếu (!v->module || tagIdx >= v->module->numTags) trả_về m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            nếu (!ct || !ct->isContinuation) trả_về m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            nếu (tagType && tagType->numArgs != 0) trả_về m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            nếu (!ft || ft->numArgs == 0) trả_về m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->numRets + ft->numArgs - 1]);
            nếu (!peer || !peer->contFuncType) trả_về m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_pop_expect(v, c_m3Type_contref, &a); nếu (r) trả_về r;

            cho (u16 i = (u16)(ft->numArgs - 1); i > 0; i--) {
                r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                nếu (r) trả_về r;
            }

            cho (u16 i = 0; i < self->numArgs; i++) {
                r = v_push(v, BaseTypeOf(self->types[self->numRets + i]));
                nếu (r) trả_về r;
            }
            break;
        }

#endif // d_m3HasStackSwitching

        default:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            break;

        } // switch

        nếu (r) trả_về r;

    } // while

    // If we ran out of bytes without hitting the final end
    trả_về m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    nếu (!i_function->wasm) trả_về m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    nếu (!v) {
        v = m3_AllocStruct (ValCtx);
        nếu (!v) trả_về m3Err_mallocFailed;
        runtime->validator = v;
    }

    memset(v, 0, sizeof(*v));
    v->module   = module;
    v->function = i_function;
    v->wasm     = i_function->wasm;
    v->wasmEnd  = i_function->wasmEnd;

    // Skip code size LEB
    u32 size;
    M3Result r = ReadLEB_u32(&size, &v->wasm, v->wasmEnd);
    nếu (r) trả_về r;

    // Parse locals
    u32 numLocalBlocks;
    r = ReadLEB_u32(&numLocalBlocks, &v->wasm, v->wasmEnd);
    nếu (r) trả_về r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 numParams = funcType ? funcType->numArgs : 0;
    nếu (numParams > d_m3ValStack) trả_về m3Err_functionStackOverflow;
    cho (u16 i = 0; i < numParams; i++) {
        v->localTypes[v->numLocals++] = BaseTypeOf(funcType->types[funcType->numRets + i]);
    }

    // Then: declared locals
    cho (u32 b = 0; b < numLocalBlocks; b++) {
        u32 count;
        r = ReadLEB_u32(&count, &v->wasm, v->wasmEnd);
        nếu (r) trả_về r;
        m3type_t localType;
        r = ParseValueType(v->module, &localType, &v->wasm, v->wasmEnd);
        nếu (r) trả_về r;
        u8 normalized = BaseTypeOf(localType);
        nếu (count > (u32) (d_m3ValStack - v->numLocals)) trả_về m3Err_functionStackOverflow;
        cho (u32 c = 0; c < count; c++) {
            v->localTypes[v->numLocals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_push_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    nếu (r) trả_về r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    nếu (r) trả_về r;

    // After validation, control stack should be empty and all declared bytes consumed
    nếu (v->ctrlTop != 0)
        trả_về m3Err_wasmMalformed;

    nếu (v->wasm != v->wasmEnd)
        trả_về m3Err_wasmSectionUnderrun;

    trả_về m3Err_none;
}

#không_thì // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (void)i_function;
    trả_về m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
