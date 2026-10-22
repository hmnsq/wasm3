//
//  m3_validate.c
//
//  Pre-pass WebAssembly bytecode validator.
//  Implements the spec's type-checking algorithm with operand/control stacks.
//

#include "m3_validate.h"
#include "m3_exception.h"
#include "m3_info.h"

#もし d_m3EnableValidation

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
    戻る v->module and i_memoryIdx < v->module->numMemories;
}

// The type of the addresses, sizes and page counts a memory instruction takes:
// whichever the memory it names was declared with. Only ever called after
// v_has_memory_idx has said the index is in range.
static m3type_t v_memory_addrtype (ValCtx * v, u32 i_memoryIdx)
{
    戻る Memory_AddrType (v->module->memories [i_memoryIdx]);
}

// Spec: the static offset of a memory access must be in range of the address
// type, so anything at all for a 64-bit memory and below 2^32 for a 32-bit one.
static bool v_offset_in_range (ValCtx * v, u32 i_memoryIdx, u64 i_offset)
{
    戻る v_memory_addrtype (v, i_memoryIdx) == c_m3Type_i64 or i_offset <= 0xFFFFFFFFull;
}

// The type a table's indexes and sizes are given in - table64 is the same
// choice as memory64, made per table. Only ever called once the table index has
// been checked against the module's index space.
static m3type_t v_table_addrtype (ValCtx * v, u32 i_tableIdx)
{
    戻る Table_AddrType (v->module->tables [i_tableIdx]);
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
        戻る 0;
    case 0x2e: case 0x2f:   // i32.load16_s, i32.load16_u
    case 0x32: case 0x33:   // i64.load16_s, i64.load16_u
    case 0x3b:              // i32.store16
    case 0x3d:              // i64.store16
        戻る 1;
    case 0x29:              // i64.load
    case 0x2b:              // f64.load
    case 0x37:              // i64.store
    case 0x39:              // f64.store
        戻る 3;
    default:                // 32-bit accesses, and a safe fallback
        戻る 2;
    }
}

#もし d_m3HasAtomics

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

    もし (sub <= 0x02) {              // memory.atomic.notify, wait32, wait64
        *o_width       = (sub == 0x02) ? 8 : 4;
        *o_numOperands = (sub == 0x00) ? 2 : 3;
        *o_valueType   = (sub == 0x02) ? c_m3Type_i64 : c_m3Type_i32;
        *o_resultType  = c_m3Type_i32;
        戻る true;
    }

    もし (sub < 0x10 or sub > c_waOp_lastAtomic) 戻る false;

    もし (sub <= 0x16) {              // loads
        variant = sub - 0x10;       *o_numOperands = 1;
    } 否则 もし (sub <= 0x1d) {       // stores
        variant = sub - 0x17;       *o_numOperands = 2;
    } 否则 もし (sub <= 0x47) {       // add, sub, and, or, xor, xchg
        variant = (sub - 0x1e) % 7; *o_numOperands = 2;
    } 否则 {                        // cmpxchg
        variant = sub - 0x48;       *o_numOperands = 3;
    }

    *o_width      = c_widths [variant];
    *o_valueType  = c_types [variant];
    *o_resultType = (sub >= 0x17 and sub <= 0x1d) ? c_m3Type_none : c_types [variant];
    戻る true;
}

#endif

// ---------- Operand stack ----------

static M3Result v_push (ValCtx * v, m3type_t type)
{
    もし (M3_UNLIKELY(v->opdTop >= d_m3ValStack))
        戻る m3Err_functionStackOverflow;
    v->opd[v->opdTop++] = type;
    戻る m3Err_none;
}

static M3Result v_pop (ValCtx * v, m3type_t * o_type)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop - 1];
    もし (M3_UNLIKELY(v->opdTop == f->height)) {
        もし (f->is_unreachable) { *o_type = c_valBottom; 戻る m3Err_none; }
        戻る m3Err_functionStackUnderrun;
    }
    *o_type = v->opd[--v->opdTop];
    戻る m3Err_none;
}

static M3Result v_pop_expect (ValCtx * v, m3type_t expect, m3type_t * o_actual)
{
    m3type_t actual = c_valBottom;
    M3Result r = v_pop(v, &actual);
    もし (M3_UNLIKELY(r)) 戻る r;
    もし (M3_UNLIKELY(expect != c_valBottom && actual != c_valBottom && actual != expect))
        戻る m3Err_typeMismatch;
    *o_actual = (actual == c_valBottom) ? expect : actual;
    戻る m3Err_none;
}

// ---------- Control stack ----------

static M3Result v_push_ctrl (ValCtx * v, m3opcode_t op, IM3FuncType type)
{
    もし (M3_UNLIKELY(v->ctrlTop >= d_m3ValCtrlDepth))
        戻る m3Err_functionStackOverflow;
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop++];
    f->opcode       = op;
    f->type         = type;
    f->param_count  = type ? type->numArgs : 0;
    f->result_count = type ? type->numRets : 0;
    f->height       = v->opdTop;
    f->is_unreachable = false;
    戻る m3Err_none;
}

static M3Result v_pop_ctrl (ValCtx * v, ValCtrlFrame * o_frame)
{
    もし (M3_UNLIKELY(v->ctrlTop == 0))
        戻る m3Err_wasmMalformed;
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop - 1];
    // pop result types
    もし (f->type) {
        ために (u16 i = f->result_count; i > 0; i--) {
            m3type_t a;
            M3Result r = v_pop_expect(v, BaseTypeOf(f->type->types[i - 1]), &a);
            もし (M3_UNLIKELY(r)) 戻る r;
        }
    }
    もし (M3_UNLIKELY(v->opdTop != f->height))
        戻る m3Err_typeCountMismatch;
    もし (o_frame) *o_frame = *f;
    v->ctrlTop--;
    戻る m3Err_none;
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
    戻る (f->opcode == 0x03) ? f->param_count : f->result_count;
}

static m3type_t v_label_t (ValCtrlFrame * f, u16 i)
{
    もし (!f->type) 戻る c_m3Type_none;
    もし (f->opcode == 0x03)
        戻る BaseTypeOf(f->type->types[f->type->numRets + i]); // params
    戻る BaseTypeOf(f->type->types[i]); // results
}

// The operand i_depth below the top, without popping. Anything at or below the
// current frame's height is implicitly bottom in an unreachable frame; reporting
// unknown there defers a genuine underrun to the pop that follows.
static m3type_t v_peek (ValCtx * v, u16 i_depth)
{
    ValCtrlFrame * f = &v->ctrl[v->ctrlTop - 1];

    もし (v->opdTop <= i_depth || (u16)(v->opdTop - i_depth - 1) < f->height)
        戻る c_valBottom;

    戻る v->opd[v->opdTop - i_depth - 1];
}

// Pop label types for branch target
static M3Result v_pop_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    ために (u16 i = n; i > 0; i--) {
        m3type_t a;
        M3Result r = v_pop_expect(v, v_label_t(tgt, i - 1), &a);
        もし (M3_UNLIKELY(r)) 戻る r;
    }
    戻る m3Err_none;
}

// Push label types back
static M3Result v_push_labels (ValCtx * v, ValCtrlFrame * tgt)
{
    u16 n = v_label_n(tgt);
    ために (u16 i = 0; i < n; i++) {
        M3Result r = v_push(v, v_label_t(tgt, i));
        もし (M3_UNLIKELY(r)) 戻る r;
    }
    戻る m3Err_none;
}

// ---------- Block type resolution ----------

static M3Result v_read_blocktype (ValCtx * v, IM3FuncType * o_type)
{
    もし (v->wasm >= v->wasmEnd)
        戻る m3Err_wasmUnderrun;

#もし d_m3HasTypedRefs
    もし (*v->wasm == d_waEncode_ref or *v->wasm == d_waEncode_refNull) {
        m3type_t refType;
        M3Result rr = ParseValueType(v->module, &refType, &v->wasm, v->wasmEnd);
        もし (rr) 戻る rr;
        IM3FuncType ftype = NULL;
        rr = AllocFuncType(&ftype, 1);
        もし (rr) 戻る rr;
        ftype->numRets = 1;
        ftype->types[0] = refType;
        // on failure this leaves ftype NULL, which would read downstream as a
        // block that returns nothing
        rr = Environment_AddFuncType(v->module->environment, &ftype);
        もし (rr) 戻る rr;
        *o_type = ftype;
        戻る m3Err_none;
    }
#endif

    i64 type;
    M3Result r = ReadLebSigned(&type, 33, &v->wasm, v->wasmEnd);
    もし (r) 戻る r;

    もし (type < 0) {
        もし (type < -64) 戻る m3Err_invalidTypeId;
        u8 valtype;
        r = NormalizeType(&valtype, (i8)type);
        もし (r) 戻る r;
        IM3Environment env = v->module->environment;
        *o_type = env->retFuncTypes[valtype];
    } 否则 {
        もし ((u32)type >= v->module->numFuncTypes)
            戻る m3Err_unknownType;
        *o_type = v->module->funcTypes[(u32)type];
    }
    戻る m3Err_none;
}

// ---------- Convenience ----------

static M3Result v_unop (ValCtx * v, m3type_t in, m3type_t out)
{
    m3type_t a; M3Result r = v_pop_expect(v, in, &a);
    もし (M3_UNLIKELY(r)) 戻る r;
    戻る v_push(v, out);
}

// return_call/return_call_indirect return the callee's results straight to the enclosing
// function's caller, so the callee's result types must be the enclosing function's
static M3Result v_check_tail_results (ValCtx * v, IM3FuncType i_calleeType)
{
    IM3FuncType ft = v->function ? v->function->funcType : NULL;

    u16 numCalleeRets = i_calleeType ? i_calleeType->numRets : 0;
    u16 numFuncRets   = ft ? ft->numRets : 0;

    もし (numCalleeRets != numFuncRets)
        戻る m3Err_typeMismatch;

    ために (u16 i = 0; i < numCalleeRets; i++) {
        もし (i_calleeType->types[i] != ft->types[i])
            戻る m3Err_typeMismatch;
    }

    戻る m3Err_none;
}

static M3Result v_binop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_pop_expect(v, t, &a); もし (M3_UNLIKELY(r)) 戻る r;
    r = v_pop_expect(v, t, &a); もし (M3_UNLIKELY(r)) 戻る r;
    戻る v_push(v, t);
}

static M3Result v_relop (ValCtx * v, m3type_t t)
{
    m3type_t a; M3Result r;
    r = v_pop_expect(v, t, &a); もし (M3_UNLIKELY(r)) 戻る r;
    r = v_pop_expect(v, t, &a); もし (M3_UNLIKELY(r)) 戻る r;
    戻る v_push(v, c_m3Type_i32);
}

static M3Result v_testop (ValCtx * v, m3type_t t)
{
    戻る v_unop(v, t, c_m3Type_i32);
}

static M3Result v_cvtop (ValCtx * v, m3type_t in, m3type_t out)
{
    戻る v_unop(v, in, out);
}


// ---------- Main validation loop ----------

#もし d_m3HasExceptionHandling

// One catch clause of a try_table. The label is resolved in the context the
// try_table itself appears in - the spec checks the clauses against C, not
// against C extended with the block's own label - so depth 0 names the block
// enclosing the try, not the try. What the clause hands the label is the tag's
// payload, optionally followed by the exnref that reifies the caught exception.
static M3Result v_catch_clause (ValCtx * v)
{
    u8 kind;
    M3Result r = Read_u8(&kind, &v->wasm, v->wasmEnd);
    もし (r) 戻る r;
    もし (kind > 0x03) 戻る m3Err_wasmMalformed;

    bool hasTag = (kind == 0x00 || kind == 0x01);
    bool hasRef = (kind == 0x01 || kind == 0x03);

    IM3FuncType tagType = NULL;
    もし (hasTag) {
        u32 tagIndex;
        r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
        もし (r) 戻る r;
        もし (!v->module || tagIndex >= v->module->numTags) 戻る m3Err_unknownTag;
        tagType = v->module->tags[tagIndex].type;
    }

    u32 depth;
    r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
    もし (r) 戻る r;
    もし (depth >= v->ctrlTop) 戻る m3Err_unknownLabel;

    ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];

    u16 numPayload = tagType ? tagType->numArgs : 0;
    u16 numLabel   = v_label_n(tgt);

    もし (numLabel != numPayload + (hasRef ? 1u : 0u))
        戻る m3Err_typeCountMismatch;

    ために (u16 i = 0; i < numPayload; i++) {
        もし (v_label_t(tgt, i) != BaseTypeOf(tagType->types[tagType->numRets + i]))
            戻る m3Err_typeMismatch;
    }

    もし (hasRef and v_label_t(tgt, numPayload) != c_m3Type_exnref)
        戻る m3Err_typeMismatch;

    戻る m3Err_none;
}

#endif // d_m3HasExceptionHandling

#もし d_m3HasStackSwitching

static M3Result v_resume_clause (ValCtx * v, u8 kind, u32 tagIdx, IM3FuncType ct)
{
    もし (kind > 0x01) 戻る m3Err_wasmMalformed;
    もし (!v->module || tagIdx >= v->module->numTags) 戻る m3Err_unknownTag;
    IM3FuncType tagType = v->module->tags[tagIdx].type;

    もし (kind == 0x00) {
        u32 depth;
        M3Result r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd); もし (r) 戻る r;
        もし (depth >= v->ctrlTop) 戻る m3Err_unknownLabel;

        ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];
        u16 numPayload = tagType ? tagType->numArgs : 0;
        u16 numLabel   = v_label_n(tgt);

        もし (numLabel != numPayload + 1u) 戻る m3Err_typeCountMismatch;

        ために (u16 j = 0; j < numPayload; j++) {
            もし (v_label_t(tgt, j) != BaseTypeOf(tagType->types[tagType->numRets + j]))
                戻る m3Err_typeMismatch;
        }
        もし (v_label_t(tgt, numPayload) != c_m3Type_contref)
            戻る m3Err_typeMismatch;

#もし d_m3HasTypedRefs
        m3type_t labelContType = (tgt->opcode == 0x03)
            ? tgt->type->types[tgt->type->numRets + numPayload]
            : tgt->type->types[numPayload];

        もし (HeapTypeOf(labelContType) == d_m3Type_heapAbstract ||
            HeapTypeOf(labelContType) == d_m3Type_heapNone) {
            戻る m3Err_typeMismatch;
        }

        u32 heapType = HeapTypeOf(labelContType);
        IM3FuncType labelCt = NULL;
        ために (u32 idx = 0; idx < v->module->numFuncTypes; ++idx) {
            もし (v->module->funcTypes[idx]->canonicalIndex == heapType &&
                v->module->funcTypes[idx]->isContinuation) {
                labelCt = v->module->funcTypes[idx];
                break;
            }
        }
        もし (!labelCt) 戻る m3Err_typeMismatch;

        IM3FuncType labelFt = labelCt->contFuncType ? labelCt->contFuncType : labelCt;
        IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;

        u16 expectedNumParams = tagType ? tagType->numRets : 0;
        もし (labelFt->numArgs != expectedNumParams) 戻る m3Err_typeMismatch;
        ために (u16 p = 0; p < expectedNumParams; ++p) {
            もし (labelFt->types[labelFt->numRets + p] != tagType->types[p])
                戻る m3Err_typeMismatch;
        }

        u16 expectedNumRets = ft ? ft->numRets : 0;
        もし (labelFt->numRets != expectedNumRets) 戻る m3Err_typeMismatch;
        ために (u16 r_idx = 0; r_idx < expectedNumRets; ++r_idx) {
            もし (labelFt->types[r_idx] != ft->types[r_idx])
                戻る m3Err_typeMismatch;
        }
#endif
    } 否则 {
        もし (tagType && tagType->numArgs != 0) 戻る m3Err_typeMismatch;
    }
    戻る m3Err_none;
}

#endif // d_m3HasStackSwitching

static M3Result v_validate_body (ValCtx * v)
{
    M3Result r = m3Err_none;
    m3type_t a = c_valBottom;

    ながら (v->wasm < v->wasmEnd)
    {
        m3opcode_t opcode;
        r = Read_opcode(&opcode, &v->wasm, v->wasmEnd);
        もし (M3_UNLIKELY(r)) 戻る r;

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
            もし (r) 戻る r;
            もし (opcode == 0x04) {
                r = v_pop_expect(v, c_m3Type_i32, &a);
                もし (r) 戻る r;
            }
            // Pop block params from caller stack
            もし (bt) {
                ために (u16 i = bt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(bt->types[bt->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
            }
            r = v_push_ctrl(v, opcode, bt);
            もし (r) 戻る r;
            // Push params inside block
            もし (bt) {
                ために (u16 i = 0; i < bt->numArgs; i++) {
                    r = v_push(v, BaseTypeOf(bt->types[bt->numRets + i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

#もし d_m3HasExceptionHandling
        case 0x1f: // try_table
        {
            IM3FuncType bt = NULL;
            r = v_read_blocktype(v, &bt);
            もし (r) 戻る r;
            もし (bt) {
                ために (u16 i = bt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(bt->types[bt->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
            }
            // the clauses are checked before the frame goes on, so their
            // labels count from outside the try block
            u32 numCatch;
            r = ReadLEB_u32(&numCatch, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            ために (u32 i = 0; i < numCatch; i++) {
                r = v_catch_clause(v);
                もし (r) 戻る r;
            }

            r = v_push_ctrl(v, opcode, bt);
            もし (r) 戻る r;

            もし (bt) {
                ために (u16 i = 0; i < bt->numArgs; i++) {
                    r = v_push(v, BaseTypeOf(bt->types[bt->numRets + i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0x08: // throw
        {
            u32 tagIndex;
            r = ReadLEB_u32(&tagIndex, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (!v->module || tagIndex >= v->module->numTags) 戻る m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIndex].type;
            もし (tt) {
                ために (u16 i = tt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(tt->types[tt->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
            }
            v_unreachable(v);
            break;
        }

        case 0x0a: // throw_ref
            r = v_pop_expect(v, c_m3Type_exnref, &a);
            もし (r) 戻る r;
            v_unreachable(v);
            break;
#endif // d_m3HasExceptionHandling

        case 0x05: // else
        {
            ValCtrlFrame frame;
            r = v_pop_ctrl(v, &frame);
            もし (r) 戻る r;
            もし (frame.opcode != 0x04)
                戻る m3Err_wasmMalformed;
            r = v_push_ctrl(v, 0x05, frame.type);
            もし (r) 戻る r;
            もし (frame.type) {
                ために (u16 i = 0; i < frame.type->numArgs; i++) {
                    r = v_push(v, BaseTypeOf(frame.type->types[frame.type->numRets + i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0x0b: // end
        {
            ValCtrlFrame frame;
            r = v_pop_ctrl(v, &frame);
            もし (r) 戻る r;
            // Push results
            もし (frame.type) {
                ために (u16 i = 0; i < frame.result_count; i++) {
                    r = v_push(v, BaseTypeOf(frame.type->types[i]));
                    もし (r) 戻る r;
                }
            }
            // If this was the outermost frame, we're done
            もし (v->ctrlTop == 0)
                戻る m3Err_none;
            break;
        }

        case 0x0c: // br
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (depth >= v->ctrlTop) 戻る m3Err_unknownLabel;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];
            r = v_pop_labels(v, tgt);
            もし (r) 戻る r;
            v_unreachable(v);
            break;
        }

        case 0x0d: // br_if
        {
            u32 depth;
            r = ReadLEB_u32(&depth, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (depth >= v->ctrlTop) 戻る m3Err_unknownLabel;
            r = v_pop_expect(v, c_m3Type_i32, &a);
            もし (r) 戻る r;
            ValCtrlFrame * tgt = &v->ctrl[v->ctrlTop - 1 - depth];
            r = v_pop_labels(v, tgt);
            もし (r) 戻る r;
            r = v_push_labels(v, tgt);
            もし (r) 戻る r;
            break;
        }

        case 0x0e: // br_table
        {
            u32 count;
            r = ReadLEB_u32(&count, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            u32 defDepth = 0;
            u16 arity = 0;
            // First pass: read all depths and validate arity + types match default
            bytes_t savedPos = v->wasm;
            // Read all targets to find the default (last one)
            ために (u32 i = 0; i <= count; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                もし (r) 戻る r;
                もし (d >= v->ctrlTop) 戻る m3Err_unknownLabel;
                もし (i == count) defDepth = d;
            }
            // Now validate all labels match the default's types
            ValCtrlFrame * defTgt = &v->ctrl[v->ctrlTop - 1 - defDepth];
            arity = v_label_n(defTgt);
            v->wasm = savedPos;

            r = v_pop_expect(v, c_m3Type_i32, &a);      // the index, before the labels are inspected
            もし (r) 戻る r;

            ために (u32 i = 0; i <= count; i++) {
                u32 d;
                r = ReadLEB_u32(&d, &v->wasm, v->wasmEnd);
                もし (r) 戻る r;
                ValCtrlFrame * t = &v->ctrl[v->ctrlTop - 1 - d];
                u16 n = v_label_n(t);
                もし (n != arity) 戻る m3Err_typeCountMismatch;
                // Spec: each target's types must be compatible with the operand
                // stack, not identical to the default's. In unreachable code the
                // operands are bottom, so the targets may legitimately differ.
                ために (u16 j = 0; j < n; j++) {
                    m3type_t want = v_label_t(t, n - 1 - j);
                    m3type_t have = v_peek(v, j);
                    もし (want != c_valBottom && have != c_valBottom && want != have)
                        戻る m3Err_typeMismatch;
                }
            }
            ValCtrlFrame * dt = &v->ctrl[v->ctrlTop - 1 - defDepth];
            r = v_pop_labels(v, dt);
            もし (r) 戻る r;
            v_unreachable(v);
            break;
        }

        case 0x0f: // return
        {
            IM3FuncType ft = v->function->funcType;
            もし (ft) {
                ために (u16 i = ft->numRets; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[i - 1]), &a);
                    もし (r) 戻る r;
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
            もし (r) 戻る r;
            もし (idx >= v->module->numFunctions) 戻る m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            もし (ft) {
                ために (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
                ために (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0x11: // call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            // the table must exist and hold funcrefs
            もし (tableIdx >= v->module->numTables) 戻る m3Err_unknownTable;
            もし (v->module->tables[tableIdx]->type != c_m3Type_funcref) 戻る m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_pop_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            もし (r) 戻る r;
            もし (ft) {
                ために (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
                ために (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0x12: // return_call
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (idx >= v->module->numFunctions) 戻る m3Err_unknownFunction;
            IM3FuncType ft = v->module->functions[idx].funcType;
            r = v_check_tail_results(v, ft);
            もし (r) 戻る r;
            もし (ft) {
                ために (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
            }
            v_unreachable(v);
            break;
        }

        case 0x13: // return_call_indirect
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            // the table must exist and hold funcrefs
            もし (tableIdx >= v->module->numTables) 戻る m3Err_unknownTable;
            もし (v->module->tables[tableIdx]->type != c_m3Type_funcref) 戻る m3Err_typeMismatch;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            r = v_check_tail_results(v, ft);
            もし (r) 戻る r;
            r = v_pop_expect(v, v_table_addrtype(v, tableIdx), &a); // table index operand
            もし (r) 戻る r;
            もし (ft) {
                ために (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
            }
            v_unreachable(v);
            break;
        }

#もし d_m3HasTypedRefs
        case 0x14: // call_ref
        case 0x15: // return_call_ref
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            bool isTail = (opcode == 0x15);
            もし (isTail) {
                r = v_check_tail_results(v, ft);
                もし (r) 戻る r;
            }
            // the callee itself, as a reference to a function of this type
            r = v_pop_expect(v, c_m3Type_funcref, &a);
            もし (r) 戻る r;
            もし (ft) {
                ために (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
                もし (not isTail) {
                    ために (u16 i = 0; i < ft->numRets; i++) {
                        r = v_push(v, BaseTypeOf(ft->types[i]));
                        もし (r) 戻る r;
                    }
                }
            }
            もし (isTail) v_unreachable(v);
            break;
        }

        case 0xd4: // ref.as_non_null
        {
            r = v_pop_expect(v, c_m3Type_funcref, &a);
            もし (r) 戻る r;
            r = v_push(v, (a == c_valBottom) ? c_m3Type_funcref : a);
            もし (r) 戻る r;
            break;
        }
#endif // d_m3HasTypedRefs

        // ---- Parametric ----
        case 0x1a: // drop
            r = v_pop(v, &a);
            もし (r) 戻る r;
            break;

        case 0x1b: // select
        {
            r = v_pop_expect(v, c_m3Type_i32, &a);
            もし (r) 戻る r;
            m3type_t t2 = c_valBottom;
            r = v_pop(v, &t2);
            もし (r) 戻る r;
            m3type_t t1;
            r = v_pop_expect(v, t2, &t1);
            もし (r) 戻る r;
            // untyped select is numeric only; references need the 0x1c form
            もし (t2 != c_valBottom && IsRefType(t2)) 戻る m3Err_typeMismatch;
            r = v_push(v, (t2 == c_valBottom) ? t1 : t2);
            もし (r) 戻る r;
            break;
        }

#もし d_m3HasRefTypes
        case 0x1c: // select with an explicit result type
        {
            u32 numTypes;
            r = ReadLEB_u32(&numTypes, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (numTypes != 1) 戻る m3Err_wasmMalformed;

            m3type_t selType;
            r = ParseValueType(v->module, &selType, &v->wasm, v->wasmEnd);  もし (r) 戻る r;
            u8 t = BaseTypeOf(selType);

            r = v_pop_expect(v, c_m3Type_i32, &a); もし (r) 戻る r;
            r = v_pop_expect(v, t, &a);            もし (r) 戻る r;
            r = v_pop_expect(v, t, &a);            もし (r) 戻る r;
            r = v_push(v, t);                      もし (r) 戻る r;
            break;
        }

        case 0x25: // table.get
        case 0x26: // table.set
        {
            u32 tableIdx;
            r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (tableIdx >= v->module->numTables) 戻る m3Err_unknownTable;
            u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
            m3type_t at = v_table_addrtype(v, tableIdx);

            もし (opcode == 0x26) {
                r = v_pop_expect(v, t, &a);   もし (r) 戻る r;
                r = v_pop_expect(v, at, &a);  もし (r) 戻る r;
            } 否则 {
                r = v_pop_expect(v, at, &a);  もし (r) 戻る r;
                r = v_push(v, t);             もし (r) 戻る r;
            }
            break;
        }

        case 0xd0: // ref.null
        {
#もし d_m3HasTypedRefs
            m3type_t heapBits;
            r = ParseHeapType(v->module, &heapBits, &v->wasm, v->wasmEnd);  もし (r) 戻る r;
            u8 t = (heapBits & d_m3Type_refExtern) ? c_m3Type_externref :
# もし d_m3HasStackSwitching
                   (heapBits & d_m3Type_refCont)   ? c_m3Type_contref :
# endif
                   c_m3Type_funcref;
#否则
            i8 waType;
            u8 t;
            r = ReadLEB_i7(&waType, &v->wasm, v->wasmEnd);   もし (r) 戻る r;
            r = NormalizeType(&t, waType);                   もし (r) 戻る r;
            もし (!IsRefType(t)) 戻る m3Err_wasmMalformed;
#endif
            r = v_push(v, t);                                もし (r) 戻る r;
            break;
        }

        case 0xd1: // ref.is_null
        {
            m3type_t t = c_valBottom;
            r = v_pop(v, &t);
            もし (r) 戻る r;
            もし (t != c_valBottom && !IsRefType(t)) 戻る m3Err_typeMismatch;
            r = v_push(v, c_m3Type_i32);
            もし (r) 戻る r;
            break;
        }

        case 0xd2: // ref.func
        {
            u32 funcIdx;
            r = ReadLEB_u32(&funcIdx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (funcIdx >= v->module->numFunctions) 戻る m3Err_unknownFunction;
            もし (!Module_IsFunctionDeclared(v->module, funcIdx)) 戻る m3Err_undeclaredFuncRef;
            r = v_push(v, c_m3Type_funcref);
            もし (r) 戻る r;
            break;
        }
#endif

        // ---- Variable ----
        case 0x20: // local.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (idx >= v->numLocals) 戻る m3Err_unknownLocal;
            r = v_push(v, v->localTypes[idx]);
            もし (r) 戻る r;
            break;
        }

        case 0x21: // local.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (idx >= v->numLocals) 戻る m3Err_unknownLocal;
            r = v_pop_expect(v, v->localTypes[idx], &a);
            もし (r) 戻る r;
            break;
        }

        case 0x22: // local.tee
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (idx >= v->numLocals) 戻る m3Err_unknownLocal;
            r = v_pop_expect(v, v->localTypes[idx], &a);
            もし (r) 戻る r;
            r = v_push(v, v->localTypes[idx]);
            もし (r) 戻る r;
            break;
        }

        case 0x23: // global.get
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (idx >= v->module->numGlobals) 戻る m3Err_unknownGlobal;
            r = v_push(v, BaseTypeOf(v->module->globals[idx].type));
            もし (r) 戻る r;
            break;
        }

        case 0x24: // global.set
        {
            u32 idx;
            r = ReadLEB_u32(&idx, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;
            もし (idx >= v->module->numGlobals) 戻る m3Err_unknownGlobal;
            r = v_pop_expect(v, BaseTypeOf(v->module->globals[idx].type), &a);
            もし (r) 戻る r;
            break;
        }

        // ---- Memory load ----
        case 0x28: case 0x29: case 0x2a: case 0x2b: // i32/i64/f32/f64.load
        case 0x2c: case 0x2d: case 0x2e: case 0x2f: // i32.load8/16 s/u
        case 0x30: case 0x31: case 0x32: case 0x33: // i64.load8/16 s/u
        case 0x34: case 0x35:                         // i64.load32 s/u
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (align > v_max_align(opcode)) 戻る m3Err_invalidAlignment;
            もし (not v_has_memory_idx(v, memidx)) 戻る m3Err_unknownMemory;
            もし (not v_offset_in_range(v, memidx, offset)) 戻る m3Err_wasmMalformed;
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); もし (r) 戻る r;
            u8 result;
            もし      (opcode == 0x28) result = c_m3Type_i32;
            否则 もし (opcode == 0x29) result = c_m3Type_i64;
            否则 もし (opcode == 0x2a) result = c_m3Type_f32;
            否则 もし (opcode == 0x2b) result = c_m3Type_f64;
            否则 もし (opcode <= 0x2f) result = c_m3Type_i32;
            否则                     result = c_m3Type_i64;
            r = v_push(v, result);
            もし (r) 戻る r;
            break;
        }

        // ---- Memory store ----
        case 0x36: case 0x37: case 0x38: case 0x39: // i32/i64/f32/f64.store
        case 0x3a: case 0x3b:                         // i32.store8/16
        case 0x3c: case 0x3d: case 0x3e:             // i64.store8/16/32
        {
            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (align > v_max_align(opcode)) 戻る m3Err_invalidAlignment;
            もし (not v_has_memory_idx(v, memidx)) 戻る m3Err_unknownMemory;
            もし (not v_offset_in_range(v, memidx, offset)) 戻る m3Err_wasmMalformed;
            u8 valtype;
            もし      (opcode == 0x36) valtype = c_m3Type_i32;
            否则 もし (opcode == 0x37) valtype = c_m3Type_i64;
            否则 もし (opcode == 0x38) valtype = c_m3Type_f32;
            否则 もし (opcode == 0x39) valtype = c_m3Type_f64;
            否则 もし (opcode <= 0x3b) valtype = c_m3Type_i32;
            否则                     valtype = c_m3Type_i64;
            r = v_pop_expect(v, valtype, &a); もし (r) 戻る r;
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); もし (r) 戻る r;
            break;
        }

        // ---- Memory size/grow ----
        case 0x3f: // memory.size
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (not v_has_memory_idx(v, memidx)) 戻る m3Err_unknownMemory;
            r = v_push(v, v_memory_addrtype(v, memidx)); もし (r) 戻る r;
            break;
        }
        case 0x40: // memory.grow
        {
            u32 memidx;
            r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (not v_has_memory_idx(v, memidx)) 戻る m3Err_unknownMemory;
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); もし (r) 戻る r;
            r = v_push(v, v_memory_addrtype(v, memidx)); もし (r) 戻る r;
            break;
        }

        // ---- Constants ----
        case 0x41: { // i32.const
            i32 val;
            r = ReadLEB_i32(&val, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            r = v_push(v, c_m3Type_i32); もし (r) 戻る r;
            break;
        }
        case 0x42: { // i64.const
            i64 val;
            r = ReadLEB_i64(&val, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            r = v_push(v, c_m3Type_i64); もし (r) 戻る r;
            break;
        }
        case 0x43: { // f32.const
            もし (v->wasm + 4 > v->wasmEnd) 戻る m3Err_wasmUnderrun;
            v->wasm += 4;
            r = v_push(v, c_m3Type_f32); もし (r) 戻る r;
            break;
        }
        case 0x44: { // f64.const
            もし (v->wasm + 8 > v->wasmEnd) 戻る m3Err_wasmUnderrun;
            v->wasm += 8;
            r = v_push(v, c_m3Type_f64); もし (r) 戻る r;
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
            もし (r) 戻る r;
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
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                もし (not v_has_memory_idx(v, memidx)) 戻る m3Err_unknownMemory;
                // the segments must have been declared up front by a data count section
                もし (not v->module->hasDataCount) 戻る m3Err_dataCountRequired;
                もし (dataidx >= v->module->numDataSegments) 戻る m3Err_unknownDataSegment;
                // the segment is indexed as an i32 no matter how the memory is
                // addressed; only the destination follows the address type
                r = v_pop_expect(v, c_m3Type_i32, &a); もし (r) 戻る r; // n
                r = v_pop_expect(v, c_m3Type_i32, &a); もし (r) 戻る r; // src
                r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); もし (r) 戻る r; // dst
                break;
            }
            case 0x09: // data.drop
            {
                u32 dataidx;
                r = ReadLEB_u32(&dataidx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                // needs the segment, but not a memory
                もし (not v->module->hasDataCount) 戻る m3Err_dataCountRequired;
                もし (dataidx >= v->module->numDataSegments) 戻る m3Err_unknownDataSegment;
                break;
            }
#もし d_m3HasRefTypes
            case 0x0c: // table.init
            case 0x0e: // table.copy
            {
                u32 a1, a2;
                r = ReadLEB_u32(&a1, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                r = ReadLEB_u32(&a2, &v->wasm, v->wasmEnd); もし (r) 戻る r;

                もし (sub == 0x0c) {                                          // elemidx, tableidx
                    // the immediates are encoded elem-then-table, but the spec's
                    // rule requires the table to be defined before the segment
                    もし (a2 >= v->module->numTables) 戻る m3Err_unknownTable;
                    もし (a1 >= v->module->numElementSegments) 戻る m3Err_unknownElemSegment;
                    もし (v->module->elementSegments[a1].type != v->module->tables[a2]->type)
                        戻る m3Err_typeMismatch;
                } 否则 {                                                    // dst, src
                    もし (a1 >= v->module->numTables) 戻る m3Err_unknownTable;
                    もし (a2 >= v->module->numTables) 戻る m3Err_unknownTable;
                    もし (v->module->tables[a1]->type != v->module->tables[a2]->type)
                        戻る m3Err_typeMismatch;
                }

                もし (sub == 0x0c) {
                    // table.init indexes the segment as an i32 whatever the
                    // table is; only the destination follows the table
                    r = v_pop_expect(v, c_m3Type_i32, &a); もし (r) 戻る r;             // n
                    r = v_pop_expect(v, c_m3Type_i32, &a); もし (r) 戻る r;             // s
                    r = v_pop_expect(v, v_table_addrtype(v, a2), &a); もし (r) 戻る r;  // d
                } 否则 {
                    // table.copy: each index follows its own table, and the
                    // length the narrower of the two
                    m3type_t dType = v_table_addrtype(v, a1);
                    m3type_t sType = v_table_addrtype(v, a2);
                    m3type_t nType = (dType == c_m3Type_i64 and sType == c_m3Type_i64)
                                       ? c_m3Type_i64 : c_m3Type_i32;
                    r = v_pop_expect(v, nType, &a); もし (r) 戻る r;  // n
                    r = v_pop_expect(v, sType, &a); もし (r) 戻る r;  // s
                    r = v_pop_expect(v, dType, &a); もし (r) 戻る r;  // d
                }
                break;
            }
            case 0x0d: // elem.drop
            {
                u32 elemIdx;
                r = ReadLEB_u32(&elemIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                もし (elemIdx >= v->module->numElementSegments) 戻る m3Err_unknownElemSegment;
                break;
            }
            case 0x0f: // table.grow
            case 0x10: // table.size
            case 0x11: // table.fill
            {
                u32 tableIdx;
                r = ReadLEB_u32(&tableIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                もし (tableIdx >= v->module->numTables) 戻る m3Err_unknownTable;
                u8 t = BaseTypeOf(v->module->tables[tableIdx]->type);
                m3type_t at = v_table_addrtype(v, tableIdx);

                もし (sub == 0x10) {                                          // table.size
                    r = v_push(v, at); もし (r) 戻る r;
                } 否则 もし (sub == 0x0f) {                                   // table.grow
                    r = v_pop_expect(v, at, &a); もし (r) 戻る r;  // n
                    r = v_pop_expect(v, t, &a);  もし (r) 戻る r;  // init
                    r = v_push(v, at);           もし (r) 戻る r;
                } 否则 {                                                    // table.fill
                    r = v_pop_expect(v, at, &a); もし (r) 戻る r;  // n
                    r = v_pop_expect(v, t, &a);  もし (r) 戻る r;  // val
                    r = v_pop_expect(v, at, &a); もし (r) 戻る r;  // i
                }
                break;
            }
#endif
            case 0x0a: // memory.copy
            {
                u32 dst, src;
                r = ReadLEB_u32(&dst, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                r = ReadLEB_u32(&src, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                もし (not v_has_memory_idx(v, dst) or not v_has_memory_idx(v, src)) 戻る m3Err_unknownMemory;
                // Spec: each address follows its own memory, and the length is
                // typed by the narrower of the two - so copying between an i32
                // and an i64 memory takes an i32 length.
                m3type_t dstType = v_memory_addrtype(v, dst);
                m3type_t srcType = v_memory_addrtype(v, src);
                m3type_t lenType = (dstType == c_m3Type_i64 and srcType == c_m3Type_i64)
                                     ? c_m3Type_i64 : c_m3Type_i32;
                r = v_pop_expect(v, lenType, &a); もし (r) 戻る r; // n
                r = v_pop_expect(v, srcType, &a); もし (r) 戻る r; // src
                r = v_pop_expect(v, dstType, &a); もし (r) 戻る r; // dst
                break;
            }
            case 0x0b: // memory.fill
            {
                u32 memidx;
                r = ReadLEB_u32(&memidx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                もし (not v_has_memory_idx(v, memidx)) 戻る m3Err_unknownMemory;
                m3type_t addrType = v_memory_addrtype(v, memidx);
                r = v_pop_expect(v, addrType, &a); もし (r) 戻る r;      // n
                r = v_pop_expect(v, c_m3Type_i32, &a); もし (r) 戻る r;  // val
                r = v_pop_expect(v, addrType, &a); もし (r) 戻る r;      // dst
                break;
            }
#もし d_m3HasWideArithmetic
            case 0x13: case 0x14: // i64.add128, i64.sub128
            case 0x15: case 0x16: // i64.mul_wide_s, i64.mul_wide_u
            {
                // two 128-bit operands of two i64 halves each, or two i64 factors;
                // either way a 128-bit result, the low half pushed first
                u32 numOperands = (sub <= 0x14) ? 4 : 2;
                ために (u32 i = 0; i < numOperands; ++i) {
                    r = v_pop_expect(v, c_m3Type_i64, &a); もし (r) 戻る r;
                }
                r = v_push(v, c_m3Type_i64); もし (r) 戻る r;
                r = v_push(v, c_m3Type_i64); もし (r) 戻る r;
                break;
            }
#endif
            default:
                // Unknown FC sub-opcode: skip validation (allow forward compat)
                break;
            }
            break;
        }

#もし d_m3HasAtomics
        // ---- 0xFE prefix (atomics) ----
        case 0xfe:
        {
            u32 sub;
            r = ReadLEB_u32(&sub, &v->wasm, v->wasmEnd);
            もし (r) 戻る r;

            もし (sub == 0x03) { // atomic.fence
                u8 reserved;
                r = Read_u8(&reserved, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                もし (reserved != 0) 戻る m3Err_wasmMalformed;
                break;
            }

            u32 width, numOperands; m3type_t valueType, resultType;
            もし (not v_atomic_shape(sub, &width, &numOperands, &valueType, &resultType)) 戻る m3Err_unknownOpcode;

            u32 align, memidx; u64 offset;
            r = ReadMemoryArg(&align, &memidx, &offset, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            // Spec: unlike an ordinary access, an atomic one must be aligned exactly
            もし ((1u << align) != width) 戻る m3Err_invalidAtomicAlignment;
            もし (not v_has_memory_idx(v, memidx)) 戻る m3Err_unknownMemory;
            もし (not v_offset_in_range(v, memidx, offset)) 戻る m3Err_wasmMalformed;

            // the operands above the address are all of the one type, except a
            // wait, whose timeout is an i64 whatever it is waiting on
            もし (sub == 0x01 or sub == 0x02) {
                r = v_pop_expect(v, c_m3Type_i64, &a); もし (r) 戻る r;
                numOperands--;
            }
            ために (u32 i = 1; i < numOperands; ++i) {
                r = v_pop_expect(v, valueType, &a); もし (r) 戻る r;
            }
            r = v_pop_expect(v, v_memory_addrtype(v, memidx), &a); もし (r) 戻る r;
            もし (resultType != c_m3Type_none) {
                r = v_push(v, resultType); もし (r) 戻る r;
            }
            break;
        }
#endif

#もし d_m3HasStackSwitching
        // ---- Stack Switching ----
        case 0xe0: // cont.new
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            IM3FuncType ft = v->module->funcTypes[typeIdx];
            もし (!ft || !ft->isContinuation) 戻る m3Err_typeMismatch;
            r = v_pop_expect(v, c_m3Type_funcref, &a); もし (r) 戻る r;
            r = v_push(v, c_m3Type_contref); もし (r) 戻る r;
            break;
        }

        case 0xe1: // cont.bind
        {
            u32 typeIdx1, typeIdx2;
            r = ReadLEB_u32(&typeIdx1, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            r = ReadLEB_u32(&typeIdx2, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (typeIdx1 >= v->module->numFuncTypes || typeIdx2 >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            IM3FuncType ct1 = v->module->funcTypes[typeIdx1];
            IM3FuncType ct2 = v->module->funcTypes[typeIdx2];
            もし (!ct1 || !ct2 || !ct1->isContinuation || !ct2->isContinuation) 戻る m3Err_typeMismatch;

            IM3FuncType ft1 = ct1->contFuncType ? ct1->contFuncType : ct1;
            IM3FuncType ft2 = ct2->contFuncType ? ct2->contFuncType : ct2;

            u16 numParams1 = ft1->numArgs;
            u16 numParams2 = ft2->numArgs;
            もし (numParams1 < numParams2) 戻る m3Err_typeMismatch;
            u32 numBound = numParams1 - numParams2;

            もし (ft1->numRets != ft2->numRets) 戻る m3Err_typeMismatch;
            ために (u16 i = 0; i < ft1->numRets; ++i) {
                もし (ft1->types[i] != ft2->types[i]) 戻る m3Err_typeMismatch;
            }
            ために (u16 i = 0; i < numParams2; ++i) {
                もし (ft1->types[ft1->numRets + numBound + i] != ft2->types[ft2->numRets + i]) 戻る m3Err_typeMismatch;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); もし (r) 戻る r;

            ために (u32 i = numBound; i > 0; i--) {
                r = v_pop_expect(v, BaseTypeOf(ft1->types[ft1->numRets + i - 1]), &a);
                もし (r) 戻る r;
            }

            r = v_push(v, c_m3Type_contref); もし (r) 戻る r;
            break;
        }

        case 0xe2: // suspend
        {
            u32 tagIdx;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (!v->module || tagIdx >= v->module->numTags) 戻る m3Err_unknownTag;
            IM3FuncType tt = v->module->tags[tagIdx].type;
            もし (tt) {
                ために (u16 i = tt->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(tt->types[tt->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
                ために (u16 i = 0; i < tt->numRets; i++) {
                    r = v_push(v, BaseTypeOf(tt->types[i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0xe3: // resume
        {
            u32 typeIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            もし (!ct || !ct->isContinuation) 戻る m3Err_typeMismatch;

            u32 numClauses;
            r = ReadLEB_u32(&numClauses, &v->wasm, v->wasmEnd); もし (r) 戻る r;

            ために (u32 i = 0; i < numClauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                u32 tagIdx;
                r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                r = v_resume_clause(v, kind, tagIdx, ct); もし (r) 戻る r;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); もし (r) 戻る r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            もし (ft) {
                ために (u16 i = ft->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
                ために (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0xe4: // resume_throw
        {
            u32 typeIdx, tagIdx, numClauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            r = ReadLEB_u32(&numClauses, &v->wasm, v->wasmEnd); もし (r) 戻る r;

            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            もし (!v->module || tagIdx >= v->module->numTags) 戻る m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            もし (!ct || !ct->isContinuation) 戻る m3Err_typeMismatch;
            IM3FuncType tagType = v->module->tags[tagIdx].type;

            // it is an exception that gets raised, so the tag has to be one
            // throw could have raised: no results
            もし (tagType && tagType->numRets != 0) 戻る m3Err_typeMismatch;

            ために (u32 i = 0; i < numClauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                r = v_resume_clause(v, kind, cTagIdx, ct); もし (r) 戻る r;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); もし (r) 戻る r;

            もし (tagType) {
                ために (u16 i = tagType->numArgs; i > 0; i--) {
                    r = v_pop_expect(v, BaseTypeOf(tagType->types[tagType->numRets + i - 1]), &a);
                    もし (r) 戻る r;
                }
            }

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            もし (ft) {
                ために (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0xe5: // resume_throw_ref
        {
            u32 typeIdx, numClauses;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            r = ReadLEB_u32(&numClauses, &v->wasm, v->wasmEnd); もし (r) 戻る r;

            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            もし (!ct || !ct->isContinuation) 戻る m3Err_typeMismatch;

            ために (u32 i = 0; i < numClauses; i++) {
                u8 kind;
                r = Read_u8(&kind, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                u32 cTagIdx;
                r = ReadLEB_u32(&cTagIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
                r = v_resume_clause(v, kind, cTagIdx, ct); もし (r) 戻る r;
            }

            r = v_pop_expect(v, c_m3Type_contref, &a); もし (r) 戻る r;
            r = v_pop_expect(v, c_m3Type_exnref, &a); もし (r) 戻る r;

            IM3FuncType ft = ct->contFuncType ? ct->contFuncType : ct;
            もし (ft) {
                ために (u16 i = 0; i < ft->numRets; i++) {
                    r = v_push(v, BaseTypeOf(ft->types[i]));
                    もし (r) 戻る r;
                }
            }
            break;
        }

        case 0xe6: // switch
        {
            u32 typeIdx, tagIdx;
            r = ReadLEB_u32(&typeIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;
            r = ReadLEB_u32(&tagIdx, &v->wasm, v->wasmEnd); もし (r) 戻る r;

            もし (typeIdx >= v->module->numFuncTypes) 戻る m3Err_unknownType;
            もし (!v->module || tagIdx >= v->module->numTags) 戻る m3Err_unknownTag;
            IM3FuncType ct = v->module->funcTypes[typeIdx];
            もし (!ct || !ct->isContinuation) 戻る m3Err_typeMismatch;

            // a switch tag carries no payload of its own
            IM3FuncType tagType = v->module->tags[tagIdx].type;
            もし (tagType && tagType->numArgs != 0) 戻る m3Err_typeMismatch;

            // switch $ct1 : [t1* (ref null $ct1)] -> [t2*], where $ct1's last
            // parameter names $ct2 - the continuation this instruction
            // suspends - and t2* is $ct2's parameters, not $ct1's results
            IM3FuncType ft = ct->contFuncType;
            もし (!ft || ft->numArgs == 0) 戻る m3Err_typeMismatch;

            IM3FuncType peer = Module_ContTypeOfRef(v->module, ft->types[ft->numRets + ft->numArgs - 1]);
            もし (!peer || !peer->contFuncType) 戻る m3Err_typeMismatch;

            IM3FuncType self = peer->contFuncType;

            r = v_pop_expect(v, c_m3Type_contref, &a); もし (r) 戻る r;

            ために (u16 i = (u16)(ft->numArgs - 1); i > 0; i--) {
                r = v_pop_expect(v, BaseTypeOf(ft->types[ft->numRets + i - 1]), &a);
                もし (r) 戻る r;
            }

            ために (u16 i = 0; i < self->numArgs; i++) {
                r = v_push(v, BaseTypeOf(self->types[self->numRets + i]));
                もし (r) 戻る r;
            }
            break;
        }

#endif // d_m3HasStackSwitching

        default:
            // Unknown opcode - skip rather than fail for forward compat
            // (the compiler will reject truly unsupported ops later)
            break;

        } // switch

        もし (r) 戻る r;

    } // while

    // If we ran out of bytes without hitting the final end
    戻る m3Err_wasmMalformed;
}

// ---------- Public entry point ----------

M3Result  ValidateFunction  (IM3Function i_function)
{
    もし (!i_function->wasm) 戻る m3Err_none;

    IM3FuncType funcType = i_function->funcType;
    IM3Module   module   = i_function->module;

    IM3Runtime runtime = module->runtime;

    ValCtx* v = runtime->validator;
    もし (!v) {
        v = m3_AllocStruct (ValCtx);
        もし (!v) 戻る m3Err_mallocFailed;
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
    もし (r) 戻る r;

    // Parse locals
    u32 numLocalBlocks;
    r = ReadLEB_u32(&numLocalBlocks, &v->wasm, v->wasmEnd);
    もし (r) 戻る r;

    // First: params. Running out of room has to be an error, not a truncation:
    // a short localTypes would make later local.get indices read as unknown
    u16 numParams = funcType ? funcType->numArgs : 0;
    もし (numParams > d_m3ValStack) 戻る m3Err_functionStackOverflow;
    ために (u16 i = 0; i < numParams; i++) {
        v->localTypes[v->numLocals++] = BaseTypeOf(funcType->types[funcType->numRets + i]);
    }

    // Then: declared locals
    ために (u32 b = 0; b < numLocalBlocks; b++) {
        u32 count;
        r = ReadLEB_u32(&count, &v->wasm, v->wasmEnd);
        もし (r) 戻る r;
        m3type_t localType;
        r = ParseValueType(v->module, &localType, &v->wasm, v->wasmEnd);
        もし (r) 戻る r;
        u8 normalized = BaseTypeOf(localType);
        もし (count > (u32) (d_m3ValStack - v->numLocals)) 戻る m3Err_functionStackOverflow;
        ために (u32 c = 0; c < count; c++) {
            v->localTypes[v->numLocals++] = normalized;
        }
    }

    // Push the function-level control frame
    r = v_push_ctrl(v, 0x00, funcType); // opcode 0x00 marks function frame
    もし (r) 戻る r;

    // Push params onto operand stack (they're part of the function body's initial stack)
    // Actually per the spec, locals are indexed but not on the operand stack.
    // The function frame's params are NOT pushed to the operand stack.
    // Only block params would be pushed (and for the function frame there are no block params
    // since the function body's "block type" has results = function returns, params = 0).
    // The function frame's label_types = results (since it's not a loop).

    // Validate the body
    r = v_validate_body(v);
    もし (r) 戻る r;

    // After validation, control stack should be empty and all declared bytes consumed
    もし (v->ctrlTop != 0)
        戻る m3Err_wasmMalformed;

    もし (v->wasm != v->wasmEnd)
        戻る m3Err_wasmSectionUnderrun;

    戻る m3Err_none;
}

#否则 // !d_m3EnableValidation

M3Result  ValidateFunction  (IM3Function i_function)
{
    (void)i_function;
    戻る m3Err_none;
}

// clang-format on

#endif // d_m3EnableValidation
