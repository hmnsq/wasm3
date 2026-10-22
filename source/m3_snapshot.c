//
//  m3_snapshot.c
//
//  Copyright © 2026 Volodymyr Shymanskyy. All rights reserved.
//
//  Saving a runtime's execution state to a stream and bringing it back.
//
//  A snapshot is written in Wasm's terms rather than this build's, so it can be
//  resumed by a build on another architecture, with another slot width, another
//  revision, or with gas metering set differently. Every number goes out little
//  endian. A place in a function is the offset of a Wasm instruction; a frame is
//  the function's locals and live operand stack as typed values; a reference is
//  a function index, or the id of a continuation or exception the snapshot
//  carries whole. Nothing in the file is an address, a slot or a metacode word.
//
//  The compiler's snapshot maps (see M3SnapshotMap) are what make this work.
//  Saving reads each value out of wherever this build kept it; loading compiles
//  the same functions, finds the same safepoints in the result, and puts each
//  value wherever that build keeps it - and then stands up the native frames the
//  interpreter needs from what its own code says they hold.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_host.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits.h>

#ਜੇ d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
ਸਥਿਰ ਅਚਲ u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
ਸਥਿਰ ਅਚਲ u32 c_snapshot_ਸੰਸਕਰਣ  = 1;

ਗਿਣਤੀ {
    d_m3SnapshotSection_Meta         = 0,
    d_m3_Snapshot_Section_ਸਮ੍ਰਿਤੀ       = 1,
    d_m3_Snapshot_Section_ਸਾਰਣੀ        = 2,
    d_m3SnapshotSection_Global       = 3,
    d_m3SnapshotSection_Segment      = 4,
    d_m3SnapshotSection_Exception    = 5,
    d_m3SnapshotSection_Continuation = 6,
    d_m3SnapshotSection_HostState    = 7,
};

#  define d_m3SnapshotFlagPostmortem  0x1

// The longest <name> in a "snapshot.<name>" custom section. A name is a label
// the embedder picks, not something the format needs room to grow.
#  define d_m3_Max_Snapshot_Name_ਲੰਮਾਈ   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
ਗਿਣਤੀ {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_ਮੁਕੰਮਲ  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_ਸਿਰਾ                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//ਪਛਾਣ_ਤੱਤ
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ਸਥਿਰ
M3_ਨਤੀਜਾ ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_ਸ਼ੁਰੂ or i_module->wasm_ਸਿਰਾ - i_module->wasm_ਸ਼ੁਰੂ < 8);

    {
        ਬਾਈਟਾਂ_t  pos = i_module->wasm_ਸ਼ੁਰੂ + 8;
        cbytes_t ਸਿਰਾ = i_module->wasm_ਸਿਰਾ;

        Xxh64_Init(&hash, 0);

        ਜਦੋਂ (pos < ਸਿਰਾ) {
            ਬਾਈਟਾਂ_t ਸ਼ੁਰੂ     = pos;
            u8      sectionId = *pos++;
            u32     ਮਾਪ      = 0;

_           (ReadLEB_u32(&ਮਾਪ, &pos, ਸਿਰਾ));
            _throwif(m3Err_wasmUnderrun, ਮਾਪ > (size_t)(ਸਿਰਾ - pos));

            pos += ਮਾਪ;
            ਜੇ (sectionId != 0) {
                Xxh64_Update(&hash, ਸ਼ੁਰੂ, (size_t)(pos - ਸ਼ੁਰੂ));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ਸਥਿਰ
ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* ਲੱਭੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ (IM3Function i_function, pc_t i_pc, u8 i_ਕਿਸਮ)
{
    ਅਚਲ M3SnapshotMap* map = i_function->snapshotMap;

    ਜੇ (map) {
        ਲਈ (u32 i = 0; i < map->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points; ++i) {
            ਜੇ (map->ਸੁਰੱਖਿਅਤ_Points[i].pc == i_pc and map->ਸੁਰੱਖਿਅਤ_Points[i].ਕਿਸਮ == i_ਕਿਸਮ) {
                ਵਾਪਸ &map->ਸੁਰੱਖਿਅਤ_Points[i];
            }
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
ਸਥਿਰ
ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* ਲੱਭੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_At (ਅਚਲ M3SnapshotMap* i_map, u32 i_wasm_ਵਿੱਥ, u8 i_ਕਿਸਮ, u32 i_ਨਿਸ਼ਾਨਾ_Loop)
{
    u32 ਨੀਵਾਂ  = 0;
    u32 ਉੱਚਾ = i_map->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points;

    ਜਦੋਂ (ਨੀਵਾਂ < ਉੱਚਾ) {
        u32 ਵਿਚਕਾਰ = ਨੀਵਾਂ + (ਉੱਚਾ - ਨੀਵਾਂ) / 2;

        ਜੇ (i_map->ਸੁਰੱਖਿਅਤ_Points[ਵਿਚਕਾਰ].wasm_ਵਿੱਥ < i_wasm_ਵਿੱਥ) {
            ਨੀਵਾਂ = ਵਿਚਕਾਰ + 1;
        } ਨਹੀਂਤਾਂ {
            ਉੱਚਾ = ਵਿਚਕਾਰ;
        }
    }

    ਲਈ (u32 i = ਨੀਵਾਂ; i < i_map->ਅੰਕ_ਸੁਰੱਖਿਅਤ_Points and i_map->ਸੁਰੱਖਿਅਤ_Points[i].wasm_ਵਿੱਥ == i_wasm_ਵਿੱਥ; ++i) {
        ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* ਬਿੰਦੂ = &i_map->ਸੁਰੱਖਿਅਤ_Points[i];

        ਜੇ (ਬਿੰਦੂ->ਕਿਸਮ != i_ਕਿਸਮ or (ਬਿੰਦੂ->flags & d_m3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਪੂਛ_Call)) {
            ਜਾਰੀ;
        }
        ਜੇ (i_ਕਿਸਮ != safepoint_op or i_map->ਖੰਡਾਂ[ਬਿੰਦੂ->aux].wasm_ਵਿੱਥ == i_ਨਿਸ਼ਾਨਾ_Loop) {
            ਵਾਪਸ ਬਿੰਦੂ;
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}

ਸਥਿਰ
ਅਚਲ M3_ਖੰਡ_ਸ਼ੁਰੂ* ਲੱਭੋ_ਖੰਡ_At (ਅਚਲ M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_ਵਿੱਥ)
{
    ਲਈ (u32 i = 0; i < i_map->ਅੰਕ_ਖੰਡਾਂ; ++i) {
        ਜੇ (i_map->ਖੰਡਾਂ[i].opcode == i_opcode and i_map->ਖੰਡਾਂ[i].wasm_ਵਿੱਥ == i_wasm_ਵਿੱਥ) {
            ਵਾਪਸ &i_map->ਖੰਡਾਂ[i];
        }
    }

    ਵਾਪਸ ਨਿਰਾ;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Encloses_ਵਿੱਥ (ਅਚਲ M3_ਖੰਡ_ਸ਼ੁਰੂ* i_ਖੰਡ, u32 i_wasm_ਵਿੱਥ)
{
    ਵਾਪਸ i_ਖੰਡ->wasm_ਵਿੱਥ < i_wasm_ਵਿੱਥ and i_wasm_ਵਿੱਥ < i_ਖੰਡ->wasm_ਸਿਰਾ;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ਸਥਿਰ ਸਤਰਅੰਦਰ
bool Handlers_ਜੀਵਤ_At (ਅਚਲ M3SnapshotMap* i_map, ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* i_ਬਿੰਦੂ, ਅਚਲ M3_ਖੰਡ_ਸ਼ੁਰੂ* i_try)
{
    ਵਾਪਸ not (i_ਬਿੰਦੂ->ਕਿਸਮ == safepoint_op and i_try->wasm_ਵਿੱਥ > i_map->ਖੰਡਾਂ[i_ਬਿੰਦੂ->aux].wasm_ਵਿੱਥ);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ਸਥਿਰ
M3_ਨਤੀਜਾ CallSiteType (IM3Function i_caller, ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* i_ਬਿੰਦੂ, IM3FuncType* o_type)
{
    M3_ਨਤੀਜਾ   ਨਤੀਜਾ = m3Err_none;
    IM3Module  module = i_caller->module;
    ਬਾਈਟਾਂ_t    pos    = i_caller->wasm;
    cbytes_t   ਸਿਰਾ    = i_caller->wasm_ਸਿਰਾ;
    u32        ਮਾਪ   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ਨਿਰਾ;

_   (ReadLEB_u32(&ਮਾਪ, &pos, ਸਿਰਾ));
    _throwif(m3Err_wasmUnderrun, i_ਬਿੰਦੂ->wasm_ਵਿੱਥ >= (size_t)(ਸਿਰਾ - pos));
    pos += i_ਬਿੰਦੂ->wasm_ਵਿੱਥ;

_   (Read_opcode(&opcode, &pos, ਸਿਰਾ));
_   (ReadLEB_u32(&index, &pos, ਸਿਰਾ));

    ਜੇ (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->ਅੰਕ_Functions);
        *o_type = module->functions[index].funcType;
    } ਨਹੀਂਤਾਂ {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_ਅਸਿੱਧਾ and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->ਅੰਕ_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ਸਥਿਰ
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 ਅੰਕ_Results = Get_Func_Type_ਅੰਕ_Results(i_expected);

    ਜੇ (not i_type or Get_Func_Type_ਅੰਕ_Results(i_type) != ਅੰਕ_Results) {
        ਵਾਪਸ false;
    }

    ਲਈ (u16 i = 0; i < ਅੰਕ_Results; ++i) {
        ਜੇ (not Is_Sub_Type_ਦਾ(Get_Func_Type_ਨਤੀਜਾ_Type(i_type, i), Get_Func_Type_ਨਤੀਜਾ_Type(i_expected, i))) {
            ਵਾਪਸ false;
        }
    }

    ਵਾਪਸ true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਚੌਖਟਾ_Registers {
    m3reg_t* r0;
#  ਜੇ d_m3HasFloat
    f64* fp0;
#  endif
} M3_ਚੌਖਟਾ_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
ਸਥਿਰ
M3_ਨਤੀਜਾ Continuation_ਢੇਰ (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_ਅਧਾਰ, u32* o_ਅੰਕ_Slots)
{
    ਜੇ (i_cont == i_runtime->ਜੜ੍ਹ_Continuation) {
        *o_ਅਧਾਰ     = (m3slot_t*)i_runtime->origin_ਢੇਰ;
        *o_ਅੰਕ_Slots = i_runtime->ਅੰਕ_ਢੇਰ_Slots;
    } ਨਹੀਂਤਾਂ {
        *o_ਅਧਾਰ     = i_cont->val_ਢੇਰ;
        *o_ਅੰਕ_Slots = i_cont->ਅੰਕ_ਢੇਰ_Slots;
    }

    ਵਾਪਸ (*o_ਅਧਾਰ) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ਸਥਿਰ
M3_ਨਤੀਜਾ Locate_ਮੁੱਲ (m3slot_t* i_ਅਧਾਰ, u32 i_ਅੰਕ_Slots, m3stack_t i_sp, ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* i_ਮੁੱਲ, u8** o_where)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u8       type   = ਅਧਾਰ_Type_ਦਾ(i_ਮੁੱਲ->type);

    *o_where = ਨਿਰਾ;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias(i_ਮੁੱਲ->ਥਾਂ));

    ਜੇ (not Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias(i_ਮੁੱਲ->ਥਾਂ)) {
        size_t ਮਾਪ   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? ਆਕਾਰ(ਖਾਲੀ*)
                                                                   : (Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type) ? ਆਕਾਰ(u64) : ਆਕਾਰ(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_ਅਧਾਰ + i_ਮੁੱਲ->ਥਾਂ) * ਆਕਾਰ(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + ਮਾਪ > (size_t)i_ਅੰਕ_Slots * ਆਕਾਰ(m3slot_t));

        *o_where = (u8*)i_ਅਧਾਰ + offset;
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ਸਥਿਰ
M3_ਨਤੀਜਾ Read_ਮੁੱਲ (u8* i_where, ਅਚਲ M3_ਚੌਖਟਾ_Registers* i_registers, ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* i_ਮੁੱਲ, u64* o_ਬਿੱਟ,
                    ਖਾਲੀ** o_reference)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u8       type   = ਅਧਾਰ_Type_ਦਾ(i_ਮੁੱਲ->type);

    *o_ਬਿੱਟ      = 0;
    *o_reference = ਨਿਰਾ;

    ਜੇ (i_where) {
        ਜੇ (IsRefType(type)) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_reference, i_where, ਆਕਾਰ(ਖਾਲੀ*));
        } ਨਹੀਂਤਾਂ ਜੇ (Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type)) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਬਿੱਟ, i_where, ਆਕਾਰ(u64));
        } ਨਹੀਂਤਾਂ {
            u32 narrow;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(&narrow, i_where, ਆਕਾਰ(narrow));
            *o_ਬਿੱਟ = narrow;
        }
    } ਨਹੀਂਤਾਂ ਜੇ (Is_Fp_ਰਜਿਸਟਰ_ਥਾਂ_Alias(i_ਮੁੱਲ->ਥਾਂ)) {
#  ਜੇ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ਜੇ (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 ਬਿੱਟ;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਬਿੱਟ, &narrow, ਆਕਾਰ(ਬਿੱਟ));
            *o_ਬਿੱਟ = ਬਿੱਟ;
        } ਨਹੀਂਤਾਂ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਬਿੱਟ, i_registers->fp0, ਆਕਾਰ(u64));
        }
#  ਨਹੀਂਤਾਂ
        _throw(m3Err_wasmMalformed);
#  endif
    } ਨਹੀਂਤਾਂ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        ਜੇ (IsRefType(type)) {
            *o_reference = (ਖਾਲੀ*)(uintptr_t)r0;
        } ਨਹੀਂਤਾਂ ਜੇ (Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type)) {
            *o_ਬਿੱਟ = (u64)r0;
        } ਨਹੀਂਤਾਂ {
            *o_ਬਿੱਟ = (u32)r0;
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Write_ਮੁੱਲ (u8* o_where, ਅਚਲ M3_ਚੌਖਟਾ_Registers* i_registers, ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* i_ਮੁੱਲ, u64 i_ਬਿੱਟ,
                     ਖਾਲੀ* i_reference)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u8       type   = ਅਧਾਰ_Type_ਦਾ(i_ਮੁੱਲ->type);

    ਜੇ (o_where) {
        ਜੇ (IsRefType(type)) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_where, &i_reference, ਆਕਾਰ(ਖਾਲੀ*));
        } ਨਹੀਂਤਾਂ ਜੇ (Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type)) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_where, &i_ਬਿੱਟ, ਆਕਾਰ(u64));
        } ਨਹੀਂਤਾਂ {
            u32 narrow = (u32)i_ਬਿੱਟ;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_where, &narrow, ਆਕਾਰ(narrow));
        }
    } ਨਹੀਂਤਾਂ ਜੇ (Is_Fp_ਰਜਿਸਟਰ_ਥਾਂ_Alias(i_ਮੁੱਲ->ਥਾਂ)) {
#  ਜੇ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ਜੇ (type == c_m3Type_f32) {
            u32 ਬਿੱਟ = (u32)i_ਬਿੱਟ;
            f32 narrow;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(&narrow, &ਬਿੱਟ, ਆਕਾਰ(narrow));
            *i_registers->fp0 = narrow;
        } ਨਹੀਂਤਾਂ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(i_registers->fp0, &i_ਬਿੱਟ, ਆਕਾਰ(u64));
        }
#  ਨਹੀਂਤਾਂ
        _throw(m3Err_wasmMalformed);
#  endif
    } ਨਹੀਂਤਾਂ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        ਜੇ (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } ਨਹੀਂਤਾਂ {
            *i_registers->r0 = (m3reg_t)i_ਬਿੱਟ;
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ਸਥਿਰ
ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* ਚੌਖਟਾ_ਮੁੱਲ (ਅਚਲ M3SnapshotMap* i_map, ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* i_ਬਿੰਦੂ, u32 i_ਸੂਚਕਾਂਕ)
{
    ਜੇ (i_ਸੂਚਕਾਂਕ < i_map->ਅੰਕ_Locals) {
        ਵਾਪਸ i_ਬਿੰਦੂ->ਪਹਿਲਾ_Local_ਮੁੱਲ == UINT32_MAX ? &i_map->locals[i_ਸੂਚਕਾਂਕ]
                                                      : &i_map->ਮੁੱਲਾਂ[i_ਬਿੰਦੂ->ਪਹਿਲਾ_Local_ਮੁੱਲ + i_ਸੂਚਕਾਂਕ];
    }
    ਵਾਪਸ &i_map->ਮੁੱਲਾਂ[i_ਬਿੰਦੂ->ਪਹਿਲਾ_ਮੁੱਲ + i_ਸੂਚਕਾਂਕ - i_map->ਅੰਕ_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ਸਥਿਰ
ਖਾਲੀ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    ਜੇ (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->ਅੰਕ_Ret_And_Arg_Slots) + i_function->ਅੰਕ_Local_ਬਾਈਟਾਂ;

        ਸਮ੍ਰਿਤੀ_ਨਕਲ(where, i_function->constants, i_function->ਅੰਕ_Constant_ਬਾਈਟਾਂ);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਸੂਚਕ_Ids {
    ਖਾਲੀ** ਚੀਜ਼ਾਂ;
    u32    ਗਿਣਤੀ_2;
    u32    ਸਮਰੱਥਾ;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ;       // a power of two, kept at least twice count
} M3_ਸੂਚਕ_Ids;

ਸਥਿਰ
u32 ਸੂਚਕ_ਬਾਲਟੀ (ਖਾਲੀ* i_ਸੂਚਕ, u32 i_ਸਮਰੱਥਾ)
{
    u64 ਬਿੱਟ = (u64)(uintptr_t)i_ਸੂਚਕ;

    ਵਾਪਸ (u32)((ਬਿੱਟ * 0x9E3779B97F4A7C15ULL) >> 32) & (i_ਸਮਰੱਥਾ - 1);
}

ਸਥਿਰ
u32 ਸੂਚਕ_Ids_ਲੱਭੋ (ਅਚਲ M3_ਸੂਚਕ_Ids* i_ids, ਖਾਲੀ* i_ਸੂਚਕ)
{
    ਜੇ (i_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ) {
        u32 ਬਾਲਟੀ = ਸੂਚਕ_ਬਾਲਟੀ(i_ਸੂਚਕ, i_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ);

        ਜਦੋਂ (i_ids->index[ਬਾਲਟੀ]) {
            u32 id = i_ids->index[ਬਾਲਟੀ] - 1;

            ਜੇ (i_ids->ਚੀਜ਼ਾਂ[id] == i_ਸੂਚਕ) {
                ਵਾਪਸ id;
            }

            ਬਾਲਟੀ = (ਬਾਲਟੀ + 1) & (i_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ - 1);
        }
    }

    ਵਾਪਸ d_m3SnapshotNone;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਸੂਚਕ_Ids_ਜੋੜੋ (M3_ਸੂਚਕ_Ids* io_ids, ਖਾਲੀ* i_ਸੂਚਕ, u32* o_id)
{
    u32 id = ਸੂਚਕ_Ids_ਲੱਭੋ(io_ids, i_ਸੂਚਕ);

    ਜੇ (id != d_m3SnapshotNone) {
        *o_id = id;
        ਵਾਪਸ m3Err_none;
    }

    ਜੇ (io_ids->ਗਿਣਤੀ_2 == io_ids->ਸਮਰੱਥਾ) {
        u32    ਸਮਰੱਥਾ = io_ids->ਸਮਰੱਥਾ ? io_ids->ਸਮਰੱਥਾ * 2 : 16;
        ਖਾਲੀ** ਚੀਜ਼ਾਂ    = m3_ReallocArray(ਖਾਲੀ*, io_ids->ਚੀਜ਼ਾਂ, ਸਮਰੱਥਾ, io_ids->ਸਮਰੱਥਾ);

        ਜੇ (not ਚੀਜ਼ਾਂ) {
            ਵਾਪਸ m3Err_mallocFailed;
        }

        io_ids->ਚੀਜ਼ਾਂ    = ਚੀਜ਼ਾਂ;
        io_ids->ਸਮਰੱਥਾ = ਸਮਰੱਥਾ;
    }

    ਜੇ ((io_ids->ਗਿਣਤੀ_2 + 1) * 2 > io_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ) {
        u32  ਸਮਰੱਥਾ = io_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ ? io_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ * 2 : 32;
        u32* index    = m3_AllocArray(u32, ਸਮਰੱਥਾ);

        ਜੇ (not index) {
            ਵਾਪਸ m3Err_mallocFailed;
        }

        ਲਈ (u32 i = 0; i < io_ids->ਗਿਣਤੀ_2; ++i) {
            u32 ਬਾਲਟੀ = ਸੂਚਕ_ਬਾਲਟੀ(io_ids->ਚੀਜ਼ਾਂ[i], ਸਮਰੱਥਾ);

            ਜਦੋਂ (index[ਬਾਲਟੀ]) {
                ਬਾਲਟੀ = (ਬਾਲਟੀ + 1) & (ਸਮਰੱਥਾ - 1);
            }

            index[ਬਾਲਟੀ] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ = ਸਮਰੱਥਾ;
    }

    id                = io_ids->ਗਿਣਤੀ_2++;
    io_ids->ਚੀਜ਼ਾਂ[id] = i_ਸੂਚਕ;

    u32 ਬਾਲਟੀ = ਸੂਚਕ_ਬਾਲਟੀ(i_ਸੂਚਕ, io_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ);

    ਜਦੋਂ (io_ids->index[ਬਾਲਟੀ]) {
        ਬਾਲਟੀ = (ਬਾਲਟੀ + 1) & (io_ids->ਸੂਚਕਾਂਕ_ਸਮਰੱਥਾ - 1);
    }

    io_ids->index[ਬਾਲਟੀ] = id + 1;

    *o_id = id;

    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ
ਖਾਲੀ ਸੂਚਕ_Ids_Free (M3_ਸੂਚਕ_Ids* io_ids)
{
    m3_Free(io_ids->ਚੀਜ਼ਾਂ);
    m3_Free(io_ids->index);
    memset(io_ids, 0, ਆਕਾਰ(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਬਫਰ_Writer {
    u8*    ਬਫਰ;
    size_t ਮਾਪ;
    size_t ਸਮਰੱਥਾ;
} M3_ਬਫਰ_Writer;

ਸਥਿਰ
M3_ਨਤੀਜਾ ਬਫਰ_Writer_Write (ਅਚਲ ਖਾਲੀ* i_ਅੰਕੜੇ, size_t i_ਮਾਪ, ਖਾਲੀ* i_userdata)
{
    M3_ਬਫਰ_Writer* bw = (M3_ਬਫਰ_Writer*)i_userdata;

    ਜੇ (bw->ਮਾਪ + i_ਮਾਪ > bw->ਸਮਰੱਥਾ) {
        size_t newCap = bw->ਸਮਰੱਥਾ ? bw->ਸਮਰੱਥਾ * 2 : 4096;
        ਜਦੋਂ (bw->ਮਾਪ + i_ਮਾਪ > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->ਬਫਰ, newCap, bw->ਸਮਰੱਥਾ);
        ਜੇ (!newBuf) {
            ਵਾਪਸ m3Err_mallocFailed;
        }
        bw->ਬਫਰ   = newBuf;
        bw->ਸਮਰੱਥਾ = newCap;
    }

    ਜੇ (i_ਮਾਪ) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(bw->ਬਫਰ + bw->ਮਾਪ, i_ਅੰਕੜੇ, i_ਮਾਪ);
        bw->ਮਾਪ += i_ਮਾਪ;
    }

    ਵਾਪਸ m3Err_none;
}

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਬਫਰ_Reader {
    ਅਚਲ u8* ਬਫਰ;
    size_t    ਮਾਪ;
    size_t    cursor;
} M3_ਬਫਰ_Reader;

ਸਥਿਰ
M3_ਨਤੀਜਾ ਬਫਰ_Reader_Read (ਖਾਲੀ* o_ਬਫਰ, size_t i_ਮਾਪ, ਖਾਲੀ* i_userdata)
{
    M3_ਬਫਰ_Reader* br = (M3_ਬਫਰ_Reader*)i_userdata;

    ਜੇ (i_ਮਾਪ > br->ਮਾਪ - br->cursor) {
        ਵਾਪਸ m3Err_wasmMalformed;
    }

    ਜੇ (i_ਮਾਪ) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਬਫਰ, br->ਬਫਰ + br->cursor, i_ਮਾਪ);
        br->cursor += i_ਮਾਪ;
    }

    ਵਾਪਸ m3Err_none;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  saving
//---------------------------------------------------------------------------------------------------------------------------------

// The snapshot is written twice. The first pass writes nothing and only
// follows references: every continuation and exception it meets is numbered,
// and the ones it numbers are followed in turn until nothing new turns up. The
// second pass writes, and by then every reference has the id it will be
// written as - which the file needs up front, since what a reference names can
// come later in it than the reference does.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    ਖਾਲੀ*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3_ਸੂਚਕ_Ids ਜੀਵਤ_Continuations;     // everything the runtime still holds, to check
    M3_ਸੂਚਕ_Ids ਜੀਵਤ_Exceptions;        //   a reference against before following it
    M3_ਸੂਚਕ_Ids continuations;         // what the snapshot carries, by id
    M3_ਸੂਚਕ_Ids exceptions;
} M3SnapshotSave;

ਸਥਿਰ ਸਤਰਅੰਦਰ
u8 M3_Type_ਮੰਜ਼ਿਲ_Val_Type (m3type_t i_type)
{
    ਚੋਣ (ਅਧਾਰ_Type_ਦਾ(i_type)) {
    ਹਾਲਤ c_m3Type_i32: ਵਾਪਸ 0x7F;
    ਹਾਲਤ c_m3Type_i64: ਵਾਪਸ 0x7E;
    ਹਾਲਤ c_m3Type_f32: ਵਾਪਸ 0x7D;
    ਹਾਲਤ c_m3Type_f64: ਵਾਪਸ 0x7C;
    ਹਾਲਤ c_m3Type_v128: ਵਾਪਸ 0x7B;
    ਹਾਲਤ c_m3Type_funcref: ਵਾਪਸ 0x70;
    ਹਾਲਤ c_m3Type_externref: ਵਾਪਸ 0x6F;
    ਹਾਲਤ c_m3Type_exnref: ਵਾਪਸ 0x69;
    ਹਾਲਤ c_m3Type_contref: ਵਾਪਸ 0x68;
    ਮੂਲ: ਵਾਪਸ 0x00;
    }
}

ਸਥਿਰ ਸਤਰਅੰਦਰ
m3type_t Val_Type_ਮੰਜ਼ਿਲ_M3_Type (u8 i_valtype)
{
    ਚੋਣ (i_valtype) {
    ਹਾਲਤ 0x7F: ਵਾਪਸ c_m3Type_i32;
    ਹਾਲਤ 0x7E: ਵਾਪਸ c_m3Type_i64;
    ਹਾਲਤ 0x7D: ਵਾਪਸ c_m3Type_f32;
    ਹਾਲਤ 0x7C: ਵਾਪਸ c_m3Type_f64;
    ਹਾਲਤ 0x7B: ਵਾਪਸ c_m3Type_v128;
    ਹਾਲਤ 0x70: ਵਾਪਸ c_m3Type_funcref;
    ਹਾਲਤ 0x6F: ਵਾਪਸ c_m3Type_externref;
    ਹਾਲਤ 0x69: ਵਾਪਸ c_m3Type_exnref;
    ਹਾਲਤ 0x68: ਵਾਪਸ c_m3Type_contref;
    ਮੂਲ: ਵਾਪਸ c_m3Type_none;
    }
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Put (M3SnapshotSave* s, ਅਚਲ ਖਾਲੀ* i_ਅੰਕੜੇ, size_t i_ਮਾਪ)
{
    ਵਾਪਸ s->writing ? s->writer(i_ਅੰਕੜੇ, i_ਮਾਪ, s->userdata) : m3Err_none;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ PutU8 (M3SnapshotSave* s, u8 i_ਮੁੱਲ)
{
    ਵਾਪਸ Put(s, &i_ਮੁੱਲ, ਆਕਾਰ(i_ਮੁੱਲ));
}


ਸਥਿਰ
M3_ਨਤੀਜਾ PutLEB_u32 (M3SnapshotSave* s, u32 i_ਮੁੱਲ)
{
    u8  ਬਾਈਟਾਂ[5];
    u32 len = 0;

    ਕਰੋ {
        u8 byte = i_ਮੁੱਲ & 0x7F;
        i_ਮੁੱਲ >>= 7;
        ਜੇ (i_ਮੁੱਲ != 0) {
            byte |= 0x80;
        }
        ਬਾਈਟਾਂ[len++] = byte;
    } ਜਦੋਂ (i_ਮੁੱਲ != 0);

    ਵਾਪਸ Put(s, ਬਾਈਟਾਂ, len);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ PutLEB_u64 (M3SnapshotSave* s, u64 i_ਮੁੱਲ)
{
    u8  ਬਾਈਟਾਂ[10];
    u32 len = 0;

    ਕਰੋ {
        u8 byte = i_ਮੁੱਲ & 0x7F;
        i_ਮੁੱਲ >>= 7;
        ਜੇ (i_ਮੁੱਲ != 0) {
            byte |= 0x80;
        }
        ਬਾਈਟਾਂ[len++] = byte;
    } ਜਦੋਂ (i_ਮੁੱਲ != 0);

    ਵਾਪਸ Put(s, ਬਾਈਟਾਂ, len);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ PutLEB_i64 (M3SnapshotSave* s, i64 i_ਮੁੱਲ)
{
    u8   ਬਾਈਟਾਂ[10];
    u32  len  = 0;
    bool ਹੋਰ = true;

    ਜਦੋਂ (ਹੋਰ) {
        u8 byte = i_ਮੁੱਲ & 0x7F;
        i_ਮੁੱਲ >>= 7;
        ਜੇ ((i_ਮੁੱਲ == 0 && !(byte & 0x40)) || (i_ਮੁੱਲ == -1 && (byte & 0x40))) {
            ਹੋਰ = false;
        } ਨਹੀਂਤਾਂ {
            byte |= 0x80;
        }
        ਬਾਈਟਾਂ[len++] = byte;
    }

    ਵਾਪਸ Put(s, ਬਾਈਟਾਂ, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਨਤੀਜਾ (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ਸਥਿਰ
M3_ਨਤੀਜਾ SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_ਨਤੀਜਾ         ਨਤੀਜਾ   = m3Err_none;
    M3_ਬਫਰ_Writer   bw       = { ਨਿਰਾ, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    ਖਾਲੀ*            userdata = s->userdata;

    s->writer   = ਬਫਰ_Writer_Write;
    s->userdata = &bw;

    ਨਤੀਜਾ = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    ਜੇ (ਨਤੀਜਾ) {
        ਜਾਓ _catch;
    }

    // a section with nothing in it is left out
    ਜੇ (bw.ਮਾਪ) {
        _throwif("snapshot section is too large", bw.ਮਾਪ > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.ਮਾਪ));
_       (Put(s, bw.ਬਫਰ, bw.ਮਾਪ));
    }

_catch:
    m3_Free(bw.ਬਫਰ);

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
u32 Function_ਸੂਚਕਾਂਕ (IM3Module i_module, IM3Function i_function)
{
    ਜੇ (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->ਅੰਕ_Functions) {
        ਵਾਪਸ (u32)(i_function - i_module->functions);
    }

    ਵਾਪਸ d_m3SnapshotNone;
}

ਸਥਿਰ
u32 Func_Type_ਸੂਚਕਾਂਕ (IM3Module i_module, IM3FuncType i_type)
{
    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_Func_Types; ++i) {
        ਜੇ (i_module->funcTypes[i] == i_type) {
            ਵਾਪਸ i;
        }
    }

    ਵਾਪਸ d_m3SnapshotNone;
}

#  ਜੇ d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
ਸਥਿਰ
IM3_ਟੈਗ ਟੈਗ_ਦਾ_ਸੂਚਕਾਂਕ (IM3Module i_module, u32 i_ਸੂਚਕਾਂਕ)
{
    IM3_ਟੈਗ ਟੈਗ = &i_module->ਟੈਗ_ਸੂਚੀ[i_ਸੂਚਕਾਂਕ];

    ਵਾਪਸ ਟੈਗ->resolved ? ਟੈਗ->resolved : ਟੈਗ;
}

ਸਥਿਰ
u32 ਟੈਗ_ਸੂਚਕਾਂਕ_2 (IM3Module i_module, IM3_ਟੈਗ i_ਟੈਗ)
{
    ਲਈ (u32 i = 0; i < i_module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ; ++i) {
        ਜੇ (ਟੈਗ_ਦਾ_ਸੂਚਕਾਂਕ(i_module, i) == i_ਟੈਗ) {
            ਵਾਪਸ i;
        }
    }

    ਵਾਪਸ d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ਸਥਿਰ
M3_ਨਤੀਜਾ NameReference (M3SnapshotSave* s, u8 i_type, ਖਾਲੀ* i_reference, u64* o_ਸ਼ਬਦ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_ਸ਼ਬਦ = d_m3SnapshotNullRef;

    ਜੇ (not i_reference) {
        ਵਾਪਸ m3Err_none;
    }

    ਜੇ (i_type == c_m3Type_funcref) {
        id = Function_ਸੂਚਕਾਂਕ(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_externref) {
        ਅਚਲ M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_ਸ਼ਬਦ = name;
        ਜਾਓ _catch;
    }
#  ਜੇ d_m3HasExceptionHandling
    ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 ਸੂਚਕ_Ids_ਲੱਭੋ(&s->ਜੀਵਤ_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 ਗਿਣਤੀ_2 = s->exceptions.ਗਿਣਤੀ_2;
_       (ਸੂਚਕ_Ids_ਜੋੜੋ(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.ਗਿਣਤੀ_2 != ਗਿਣਤੀ_2);
    }
#  endif
    ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 ਸੂਚਕ_Ids_ਲੱਭੋ(&s->ਜੀਵਤ_Continuations, i_reference) == d_m3SnapshotNone);

        u32 ਗਿਣਤੀ_2 = s->continuations.ਗਿਣਤੀ_2;
_       (ਸੂਚਕ_Ids_ਜੋੜੋ(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.ਗਿਣਤੀ_2 != ਗਿਣਤੀ_2);
    } ਨਹੀਂਤਾਂ {
        _throw("a reference of an unknown type");
    }

    *o_ਸ਼ਬਦ = id;

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ਸਥਿਰ
M3_ਨਤੀਜਾ Put_Ref_ਮੁੱਲ (M3SnapshotSave* s, u8 i_type, ਖਾਲੀ* i_reference)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u64      ਸ਼ਬਦ   = 0;

_   (NameReference(s, i_type, i_reference, &ਸ਼ਬਦ));

    ਜੇ (not s->writing) {
        ਵਾਪਸ m3Err_none;
    }

    ਜੇ (ਸ਼ਬਦ == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } ਨਹੀਂਤਾਂ {
        u8 ਕਿਸਮ = 0;
        ਜੇ (i_type == c_m3Type_funcref) {
            ਕਿਸਮ = 0x01;
        } ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_externref) {
            ਕਿਸਮ = 0x02;
        }
#  ਜੇ d_m3HasExceptionHandling
        ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_exnref) {
            ਕਿਸਮ = 0x03;
        }
#  endif
        ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_contref) {
            ਕਿਸਮ = 0x04;
        }

_       (PutU8(s, ਕਿਸਮ));
_       (PutLEB_u64(s, ਸ਼ਬਦ));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ਸਥਿਰ
M3_ਨਤੀਜਾ PutLE32 (M3SnapshotSave* s, u32 i_ਮੁੱਲ)
{
    u8 ਬਾਈਟਾਂ[4];

    ਲਈ (u32 i = 0; i < ਆਕਾਰ(ਬਾਈਟਾਂ); ++i) {
        ਬਾਈਟਾਂ[i] = (u8)(i_ਮੁੱਲ >> (8 * i));
    }

    ਵਾਪਸ Put(s, ਬਾਈਟਾਂ, ਆਕਾਰ(ਬਾਈਟਾਂ));
}

ਸਥਿਰ
M3_ਨਤੀਜਾ PutLE64 (M3SnapshotSave* s, u64 i_ਮੁੱਲ)
{
    u8 ਬਾਈਟਾਂ[8];

    ਲਈ (u32 i = 0; i < ਆਕਾਰ(ਬਾਈਟਾਂ); ++i) {
        ਬਾਈਟਾਂ[i] = (u8)(i_ਮੁੱਲ >> (8 * i));
    }

    ਵਾਪਸ Put(s, ਬਾਈਟਾਂ, ਆਕਾਰ(ਬਾਈਟਾਂ));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ਸਥਿਰ
M3_ਨਤੀਜਾ Put_ਮੁੱਲ_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_ਬਿੱਟ)
{
    ਜੇ (IsRefType(i_type)) {
        ਵਾਪਸ Put_Ref_ਮੁੱਲ(s, ਅਧਾਰ_Type_ਦਾ(i_type), (ਖਾਲੀ*)(uintptr_t)i_ਬਿੱਟ);
    }

    ਚੋਣ (ਅਧਾਰ_Type_ਦਾ(i_type)) {
    ਹਾਲਤ c_m3Type_i32: ਵਾਪਸ PutLEB_i64(s, (i32)(u32)i_ਬਿੱਟ);
    ਹਾਲਤ c_m3Type_i64: ਵਾਪਸ PutLEB_i64(s, (i64)i_ਬਿੱਟ);
    ਹਾਲਤ c_m3Type_f32: ਵਾਪਸ PutLE32(s, (u32)i_ਬਿੱਟ);
    ਹਾਲਤ c_m3Type_f64: ਵਾਪਸ PutLE64(s, i_ਬਿੱਟ);
    ਮੂਲ: ਵਾਪਸ "a snapshot cannot hold a value of this type as bits";
    }
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Put_ਮੁੱਲ_ਟੈਗ (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_ਮੰਜ਼ਿਲ_Val_Type(i_type);

    ਜੇ (not valtype) {
        ਵਾਪਸ "a snapshot has no tag for a value of this type";
    }

    ਵਾਪਸ PutU8(s, valtype);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Put_ਮੁੱਲ_ਬਿੱਟ (M3SnapshotSave* s, m3type_t i_type, u64 i_ਬਿੱਟ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (Put_ਮੁੱਲ_ਟੈਗ(s, i_type));
_   (Put_ਮੁੱਲ_Body(s, i_type, i_ਬਿੱਟ));

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ਸਥਿਰ
M3_ਨਤੀਜਾ Put_ਮੁੱਲ (M3SnapshotSave* s, m3type_t i_type, ਅਚਲ ਖਾਲੀ* i_where)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (Put_ਮੁੱਲ_ਟੈਗ(s, i_type));

    ਜੇ (IsRefType(i_type)) {
        ਖਾਲੀ* reference;
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(&reference, i_where, ਆਕਾਰ(reference));
_       (Put_ਮੁੱਲ_Body(s, i_type, (u64)(uintptr_t)reference));
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ_Type_ਦਾ(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 ਬਾਈਟਾਂ[16];
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(ਬਾਈਟਾਂ, i_where, ਆਕਾਰ(ਬਾਈਟਾਂ));
_       (Put(s, ਬਾਈਟਾਂ, ਆਕਾਰ(ਬਾਈਟਾਂ)));
    } ਨਹੀਂਤਾਂ ਜੇ (Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(i_type)) {
        u64 ਬਿੱਟ;
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ਬਿੱਟ, i_where, ਆਕਾਰ(ਬਿੱਟ));
_       (Put_ਮੁੱਲ_Body(s, i_type, ਬਿੱਟ));
    } ਨਹੀਂਤਾਂ {
        u32 narrow;
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(&narrow, i_where, ਆਕਾਰ(narrow));
_       (Put_ਮੁੱਲ_Body(s, i_type, narrow));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Put_ਸਮ੍ਰਿਤੀ_Chunks (M3SnapshotSave* s, ਅਚਲ u8* i_ਬਾਈਟਾਂ, size_t i_ਮਾਪ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    size_t   cursor = 0;

    ਜਦੋਂ (cursor < i_ਮਾਪ) {
        u8     byte = i_ਬਾਈਟਾਂ[cursor];
        size_t ਲੜੀਵਾਰ  = 1;

        ਜਦੋਂ (cursor + ਲੜੀਵਾਰ < i_ਮਾਪ and i_ਬਾਈਟਾਂ[cursor + ਲੜੀਵਾਰ] == byte) {
            ਲੜੀਵਾਰ++;
        }

        ਜੇ ((byte == 0x00 or byte == 0xFF) and ਲੜੀਵਾਰ >= d_m3_Snapshot_ਲੜੀਵਾਰ_ਹੱਦ) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            ਜੇ (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)ਲੜੀਵਾਰ));
            }

            cursor += ਲੜੀਵਾਰ;
            ਜਾਰੀ;
        }

        size_t raw_ਸ਼ੁਰੂ = cursor;

        ਜਦੋਂ (cursor < i_ਮਾਪ) {
            u8     b      = i_ਬਾਈਟਾਂ[cursor];
            size_t ਲੰਮਾਈ = 1;

            ਜਦੋਂ (cursor + ਲੰਮਾਈ < i_ਮਾਪ and i_ਬਾਈਟਾਂ[cursor + ਲੰਮਾਈ] == b) {
                ਲੰਮਾਈ++;
            }

            ਜੇ ((b == 0x00 or b == 0xFF) and ਲੰਮਾਈ >= d_m3_Snapshot_ਲੜੀਵਾਰ_ਹੱਦ) {
                ਰੋਕੋ;
            }

            cursor += ਲੰਮਾਈ;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_ਸ਼ੁਰੂ));
_       (PutLEB_u32(s, (u32)(cursor - raw_ਸ਼ੁਰੂ)));
_       (Put(s, i_ਬਾਈਟਾਂ + raw_ਸ਼ੁਰੂ, cursor - raw_ਸ਼ੁਰੂ));
    }

_   (PutU8(s, d_m3_Chunk_ਸਿਰਾ));

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ਸਥਿਰ
u32 ਸਮ੍ਰਿਤੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ (IM3Module module, u32 index)
{
    u32 j = 0;
    ਜਦੋਂ (module->memories[j] != module->memories[index]) {
        ++j;
    }
    ਵਾਪਸ j;
}

ਸਥਿਰ
u32 ਸਾਰਣੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ (IM3Module module, u32 index)
{
    u32 j = 0;
    ਜਦੋਂ (module->tables[j] != module->tables[index]) {
        ++j;
    }
    ਵਾਪਸ j;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveMemories (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ਅੰਕ_Memories));

    ਲਈ (u32 m = 0; m < module->ਅੰਕ_Memories; ++m) {
        IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ   = module->memories[m];
        u32       ਪਹਿਲਾ    = ਸਮ੍ਰਿਤੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ(module, m);
        bool      has_ਅੰਕੜੇ  = ਸਮ੍ਰਿਤੀ and ਸਮ੍ਰਿਤੀ->mallocated;
        u32       ਪੰਨਾ_ਮਾਪ = ਸਮ੍ਰਿਤੀ ? ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(ਸਮ੍ਰਿਤੀ) : d_m3_Default_Mem_ਪੰਨਾ_ਮਾਪ;
        u64       ਬਾਈਟਾਂ    = ਸਮ੍ਰਿਤੀ ? ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ * (u64)ਪੰਨਾ_ਮਾਪ : 0;
        u32       ਪੰਨਾ_ਬਿੱਟ = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, ਪਹਿਲਾ));

        ਜੇ (ਪਹਿਲਾ != m) {
            ਜਾਰੀ;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", ਸਮ੍ਰਿਤੀ and ਸਮ੍ਰਿਤੀ->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", ਬਾਈਟਾਂ > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 ਸਮ੍ਰਿਤੀ and ਸਮ੍ਰਿਤੀ->mallocated and ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ < ਬਾਈਟਾਂ);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        ਜਦੋਂ ((1u << ਪੰਨਾ_ਬਿੱਟ) < ਪੰਨਾ_ਮਾਪ) {
            ++ਪੰਨਾ_ਬਿੱਟ;
        }

_       (PutLEB_u32(s, ਪੰਨਾ_ਬਿੱਟ));
_       (PutLEB_u32(s, ਸਮ੍ਰਿਤੀ ? (u32)ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        ਜੇ (not s->writing) {
            ਜਾਰੀ;
        }

        ਜੇ (has_ਅੰਕੜੇ) {
_           (Put_ਸਮ੍ਰਿਤੀ_Chunks(s, m3_Mem_ਅੰਕੜੇ(ਸਮ੍ਰਿਤੀ->mallocated), (size_t)ਬਾਈਟਾਂ));
        } ਨਹੀਂਤਾਂ {
_           (PutU8(s, d_m3_Chunk_ਸਿਰਾ));
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveGlobals (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ਅੰਕ_Globals));

    ਲਈ (u32 g = 0; g < module->ਅੰਕ_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_ਮੁੱਲ(s, global->type, &cell->i64_ਮੁੱਲ));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveTables (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ਅੰਕ_Tables));

    ਲਈ (u32 t = 0; t < module->ਅੰਕ_Tables; ++t) {
        IM3_ਸਾਰਣੀ ਸਾਰਣੀ_2 = module->tables[t];
        u32      ਪਹਿਲਾ = ਸਾਰਣੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, ਪਹਿਲਾ));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        ਜੇ (ਪਹਿਲਾ != t) {
            ਜਾਰੀ;
        }

_       (PutU8(s, M3_Type_ਮੰਜ਼ਿਲ_Val_Type(ਸਾਰਣੀ_2->type)));
_       (PutLEB_u32(s, ਸਾਰਣੀ_2->ਮਾਪ));

        ਲਈ (u32 e = 0; e < ਸਾਰਣੀ_2->ਮਾਪ; ++e) {
_           (Put_Ref_ਮੁੱਲ(s, ਅਧਾਰ_Type_ਦਾ(ਸਾਰਣੀ_2->type), ਸਾਰਣੀ_2->elements[e]));
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveSegments (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ਅੰਕ_ਅੰਕੜੇ_Segments));

    ਲਈ (u32 i = 0; i < module->ਅੰਕ_ਅੰਕੜੇ_Segments; ++i) {
_       (PutU8(s, module->ਅੰਕੜੇ_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->ਅੰਕ_Element_Segments));

    ਲਈ (u32 i = 0; i < module->ਅੰਕ_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

#  ਜੇ d_m3HasExceptionHandling

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveExceptions (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.ਗਿਣਤੀ_2));

    ਲਈ (u32 i = 0; i < s->exceptions.ਗਿਣਤੀ_2; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.ਚੀਜ਼ਾਂ[i];
        IM3FuncType  type = exn->ਟੈਗ->type;

        _throwif(m3Err_wasmMalformed, exn->ਅੰਕ_ਦਲੀਲਾਂ != Get_Func_Type_ਅੰਕ_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->ਅੰਕ_ਦਲੀਲਾਂ));

        ਲਈ (u32 a = 0; a < exn->ਅੰਕ_ਦਲੀਲਾਂ; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_ਮੁੱਲ_ਬਿੱਟ(s, argType, exn->ਦਲੀਲਾਂ[a]));
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3Activation {
    IM3Function        function;
    ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* ਬਿੰਦੂ;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 ਕਿਸਮ;            // M3SafePointKind
    M3_ਚੌਖਟਾ_Registers   registers;       // the ones it goes on with
    ਅਚਲ M3_ਚੌਖਟਾ*     resume;          // the resume it waits in, for safepoint_resume
    i32                ਪਹਿਲਾ_ਚੌਖਟਾ;      // its frames, outermost first: firstFrame down to lastFrame
    i32                ਆਖਰੀ_ਚੌਖਟਾ;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ਸਥਿਰ
ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* ਲੱਭੋ_ਚੌਖਟਾ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_ਕਿਸਮ)
{
    ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* ਬਿੰਦੂ = *io_function ? ਲੱਭੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(*io_function, i_pc, i_ਕਿਸਮ) : ਨਿਰਾ;

    ਲਈ (u32 i = 0; not ਬਿੰਦੂ and i < i_module->ਅੰਕ_Functions; ++i) {
        ਬਿੰਦੂ = ਲੱਭੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(&i_module->functions[i], i_pc, i_ਕਿਸਮ);
        ਜੇ (ਬਿੰਦੂ) {
            *io_function = &i_module->functions[i];
        }
    }

    ਵਾਪਸ ਬਿੰਦੂ;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਅਗਲਾ_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_ਪਹਿਲਾ, M3Activation* o_act)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    i32      i      = i_ਪਹਿਲਾ;

    memset(o_act, 0, ਆਕਾਰ(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->ਪਹਿਲਾ_ਚੌਖਟਾ = i_ਪਹਿਲਾ;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    ਲਈ (; i >= 0; --i) {
        ਅਚਲ M3_ਚੌਖਟਾ* ਚੌਖਟਾ = &i_cont->ਚੌਖਟੇ[i];

        _throwif("a suspended frame runs against another module's memory", ਚੌਖਟਾ->ਸਮ੍ਰਿਤੀ != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", ਚੌਖਟਾ->sp != i_sp);

        ਜੇ (ਚੌਖਟਾ->ਕਿਸਮ == ਚੌਖਟਾ_call) {
            o_act->pc           = ਚੌਖਟਾ->pc;
            o_act->ਕਿਸਮ         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&ਚੌਖਟਾ->call.r0;
#  ਜੇ d_m3HasFloat
            o_act->registers.fp0 = (f64*)&ਚੌਖਟਾ->call.fp0;
#  endif
            ਰੋਕੋ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਚੌਖਟਾ->ਕਿਸਮ == ਚੌਖਟਾ_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = ਚੌਖਟਾ->pc;
            o_act->ਕਿਸਮ   = safepoint_resume;
            o_act->resume = ਚੌਖਟਾ;
            ਰੋਕੋ;
        }
    }

    ਜੇ (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->ਕਿਸਮ         = i_cont->suspend_ਬਿੰਦੂ;
        o_act->registers.r0 = &i_cont->r0;
#  ਜੇ d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->ਆਖਰੀ_ਚੌਖਟਾ = i;
    o_act->ਬਿੰਦੂ     = ਲੱਭੋ_ਚੌਖਟਾ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ(i_module, &o_act->function, o_act->pc, o_act->ਕਿਸਮ);

    _throwif("a suspended frame is not at a safepoint", not o_act->ਬਿੰਦੂ);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
    ਲਈ (i32 k = i_ਪਹਿਲਾ; k > o_act->ਆਖਰੀ_ਚੌਖਟਾ; --k) {
        ਅਚਲ M3_ਚੌਖਟਾ* ਚੌਖਟਾ = &i_cont->ਚੌਖਟੇ[k];
        _throwif("a suspended frame is not the function it is entered with",
                 ਚੌਖਟਾ->ਕਿਸਮ == ਚੌਖਟਾ_ਇੰਦਰਾਜ and ਚੌਖਟਾ->ਇੰਦਰਾਜ.function != o_act->function);
    }
#  endif

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ਸਥਿਰ
M3_ਨਤੀਜਾ SaveActivation (M3SnapshotSave* s, m3slot_t* i_ਅਧਾਰ, u32 i_ਅੰਕ_Slots, ਅਚਲ M3Activation* i_act)
{
    M3_ਨਤੀਜਾ             ਨਤੀਜਾ        = m3Err_none;
    IM3Function          function      = i_act->function;
    ਅਚਲ M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_ਸੂਚਕਾਂਕ = Function_ਸੂਚਕਾਂਕ(s->module, function);
    ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ*   ਬਿੰਦੂ         = i_act->ਬਿੰਦੂ;
    u32                  ਅੰਕ_ਮੁੱਲਾਂ;

    _throwif("a suspended frame belongs to another module", function_ਸੂਚਕਾਂਕ == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_ਸੂਚਕਾਂਕ));
_   (PutU8(s, ਬਿੰਦੂ->ਕਿਸਮ));
_   (PutLEB_u32(s, ਬਿੰਦੂ->wasm_ਵਿੱਥ));

    ਜੇ (ਬਿੰਦੂ->ਕਿਸਮ == safepoint_op) {
_       (PutLEB_u32(s, map->ਖੰਡਾਂ[ਬਿੰਦੂ->aux].wasm_ਵਿੱਥ));
    }

    ਅੰਕ_ਮੁੱਲਾਂ = map->ਅੰਕ_Locals + ਬਿੰਦੂ->ਅੰਕ_ਮੁੱਲਾਂ;

_   (PutLEB_u32(s, ਅੰਕ_ਮੁੱਲਾਂ));

    ਲਈ (u32 v = 0; v < ਅੰਕ_ਮੁੱਲਾਂ; ++v) {
        ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* ਮੁੱਲ = ਚੌਖਟਾ_ਮੁੱਲ(map, ਬਿੰਦੂ, v);
        u8*                where;
        u64                ਸ਼ਬਦ;
        ਖਾਲੀ*              reference;

_       (Locate_ਮੁੱਲ(i_ਅਧਾਰ, i_ਅੰਕ_Slots, i_act->sp, ਮੁੱਲ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ਜੇ (ਅਧਾਰ_Type_ਦਾ(ਮੁੱਲ->type) == c_m3Type_v128) {
_           (Put_ਮੁੱਲ_ਟੈਗ(s, ਮੁੱਲ->type));
_           (Put(s, where, 16));
            ਜਾਰੀ;
        }

_       (Read_ਮੁੱਲ(where, &i_act->registers, ਮੁੱਲ, &ਸ਼ਬਦ, &reference));

_       (Put_ਮੁੱਲ_ਬਿੱਟ(s, ਮੁੱਲ->type, IsRefType(ਮੁੱਲ->type) ? (u64)(uintptr_t)reference : ਸ਼ਬਦ));
    }

    ਜੇ (ਬਿੰਦੂ->ਕਿਸਮ == safepoint_resume) {
_       (Put_Ref_ਮੁੱਲ(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
ਸਥਿਰ
M3_ਨਤੀਜਾ SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_ਨਤੀਜਾ      ਨਤੀਜਾ   = m3Err_none;
    M3Activation* acts     = ਨਿਰਾ;
    u32           ਅੰਕ_Acts  = 0;
    IM3Function   function = i_cont->ਇੰਦਰਾਜ_Function;
    i32           ਪਹਿਲਾ    = (i32)i_cont->ਅੰਕ_ਚੌਖਟੇ - 1;
    m3slot_t*     ਅਧਾਰ;
    u32           ਅੰਕ_Slots;
    m3stack_t     sp;

_   (Continuation_ਢੇਰ(s->runtime, i_cont, &ਅਧਾਰ, &ਅੰਕ_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->ਅੰਕ_ਚੌਖਟੇ + 1);
    _throwifnull(acts);

    sp = ਅਧਾਰ;

    ਜਦੋਂ (true) {
        M3Activation* act = &acts[ਅੰਕ_Acts];

_       (ਅਗਲਾ_Activation(s->module, i_cont, function, sp, ਪਹਿਲਾ, act));

        ਜੇ (act->ਕਿਸਮ != safepoint_call) {
            ਅੰਕ_Acts++;
            ਰੋਕੋ;
        }

        _throwif("a suspended continuation stops at a call", act->ਆਖਰੀ_ਚੌਖਟਾ < 0);

        ਜੇ (not (act->ਬਿੰਦੂ->flags & d_m3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਪੂਛ_Call)) {
            ਅੰਕ_Acts++;
        }

        function = i_cont->ਚੌਖਟੇ[act->ਆਖਰੀ_ਚੌਖਟਾ].call.function;
        sp       = act->sp + act->ਬਿੰਦੂ->aux;
        ਪਹਿਲਾ    = act->ਆਖਰੀ_ਚੌਖਟਾ - 1;
    }

_   (PutLEB_u32(s, ਅੰਕ_Acts));

    ਲਈ (u32 a = 0; a < ਅੰਕ_Acts; ++a) {
_       (SaveActivation(s, ਅਧਾਰ, ਅੰਕ_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_ਜੜ੍ਹ    = (i_cont == s->runtime->ਜੜ੍ਹ_Continuation);
    u8        state     = snapshot_cont_ਮੁਕੰਮਲ;
    u32       type_ਸੂਚਕਾਂਕ = is_ਜੜ੍ਹ ? d_m3SnapshotNone : Func_Type_ਸੂਚਕਾਂਕ(module, i_cont->type);

    ਜੇ (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } ਨਹੀਂਤਾਂ ਜੇ (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } ਨਹੀਂਤਾਂ ਜੇ (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->ਇੰਦਰਾਜ_Function);
    _throwif("a continuation belongs to another module", Function_ਸੂਚਕਾਂਕ(module, i_cont->ਇੰਦਰਾਜ_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_ਜੜ੍ਹ and type_ਸੂਚਕਾਂਕ == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_ਜੜ੍ਹ));
_   (PutLEB_u32(s, type_ਸੂਚਕਾਂਕ));
_   (PutLEB_u32(s, Function_ਸੂਚਕਾਂਕ(module, i_cont->ਇੰਦਰਾਜ_Function)));
#  ਜੇ d_m3HasExceptionHandling
_   (Put_Ref_ਮੁੱਲ(s, c_m3Type_exnref, i_cont->resumeThrow));
#  ਨਹੀਂਤਾਂ
_   (Put_Ref_ਮੁੱਲ(s, c_m3Type_exnref, ਨਿਰਾ));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    ਜੇ (s->postmortem) {
        ਜਾਓ _catch;
    }

    ਜੇ (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->ਇੰਦਰਾਜ_Function ? i_cont->ਇੰਦਰਾਜ_Function->funcType : ਨਿਰਾ;
        u16         ਅੰਕ_Rets = inner ? Get_Func_Type_ਅੰਕ_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ > Get_Func_Type_ਅੰਕ_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ));

        ਲਈ (u32 i = 0; i < i_cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_ਮੁੱਲ(s, type, i_cont->val_ਢੇਰ + (ਅੰਕ_Rets + i) * c_io_ਥਾਂ_ਗਿਣਤੀ));
        }
    } ਨਹੀਂਤਾਂ ਜੇ (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ));
_       (SaveSuspendedBody(s, i_cont));
    } ਨਹੀਂਤਾਂ {
_       (PutLEB_u32(s, 0));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveContinuations (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (PutLEB_u32(s, s->continuations.ਗਿਣਤੀ_2));

    ਲਈ (u32 i = 0; i < s->continuations.ਗਿਣਤੀ_2; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.ਚੀਜ਼ਾਂ[i]));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ਸਥਿਰ
M3_ਨਤੀਜਾ SaveHostState (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ               ਨਤੀਜਾ = m3Err_none;
    ਅਚਲ M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_ਬਫਰ_Writer         bw     = { ਨਿਰਾ, 0, 0 };

    ਜੇ (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, ਬਫਰ_Writer_Write, &bw));
    }

    ਜੇ (bw.ਮਾਪ > 0) {
_       (Put(s, bw.ਬਫਰ, bw.ਮਾਪ));
    }

_catch:
    m3_Free(bw.ਬਫਰ);

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
ਖਾਲੀ SnapshotResourceTotals (IM3Module module, u64* ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ, u64* ਸਾਰਣੀ_Elements)
{
    *ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ = *ਸਾਰਣੀ_Elements = 0;
    ਲਈ (u32 i = 0; i < module->ਅੰਕ_Memories; ++i) {
        IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ = module->memories[i];
        ਜੇ (ਸਮ੍ਰਿਤੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ(module, i) == i and ਸਮ੍ਰਿਤੀ) {
            *ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ += ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ * (u64)ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(ਸਮ੍ਰਿਤੀ);
        }
    }
    ਲਈ (u32 i = 0; i < module->ਅੰਕ_Tables; ++i) {
        IM3_ਸਾਰਣੀ ਸਾਰਣੀ_2 = module->tables[i];
        ਜੇ (ਸਾਰਣੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ(module, i) == i and ਸਾਰਣੀ_2) {
            *ਸਾਰਣੀ_Elements += ਸਾਰਣੀ_2->ਮਾਪ;
        }
    }
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveMeta (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u64      hash   = 0;
    u64      ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ, ਸਾਰਣੀ_Elements;
    u32      ਸਰਗਰਮ_Stacks = 0;
    ਲਈ (u32 i = 0; i < s->continuations.ਗਿਣਤੀ_2; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.ਚੀਜ਼ਾਂ[i];
        ਜੇ (cont != s->runtime->ਜੜ੍ਹ_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            ਸਰਗਰਮ_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ, &ਸਾਰਣੀ_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_ਸਮਾਂ_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.ਗਿਣਤੀ_2));
_   (PutLEB_u32(s, s->exceptions.ਗਿਣਤੀ_2));
_   (PutLEB_u64(s, ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ));
_   (PutLEB_u64(s, ਸਾਰਣੀ_Elements));
_   (PutLEB_u32(s, ਸਰਗਰਮ_Stacks));

#  ਜੇ d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    ਲਈ (u32 i = 0; i < s->exceptions.ਗਿਣਤੀ_2; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.ਚੀਜ਼ਾਂ[i];
        u32          ਟੈਗ_ਸੂਚਕਾਂਕ  = ਟੈਗ_ਸੂਚਕਾਂਕ_2(s->module, exception->ਟੈਗ);

        _throwif("an exception was thrown with another module's tag", ਟੈਗ_ਸੂਚਕਾਂਕ == d_m3SnapshotNone);

_       (PutLEB_u32(s, ਟੈਗ_ਸੂਚਕਾਂਕ));
    }
#  endif

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ਸਥਿਰ
M3_ਨਤੀਜਾ DiscoverReferences (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ            = m3Err_none;
    u32      ਪੂਰਾ_Continuations = 0;
    u32      ਪੂਰਾ_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    ਜਦੋਂ (ਪੂਰਾ_Continuations < s->continuations.ਗਿਣਤੀ_2 or ਪੂਰਾ_Exceptions < s->exceptions.ਗਿਣਤੀ_2) {
#  ਜੇ d_m3HasExceptionHandling
        ਜਦੋਂ (ਪੂਰਾ_Exceptions < s->exceptions.ਗਿਣਤੀ_2) {
            M3Exception* exception = (M3Exception*)s->exceptions.ਚੀਜ਼ਾਂ[ਪੂਰਾ_Exceptions++];
            IM3FuncType  type      = exception->ਟੈਗ->type;

            _throwif(m3Err_wasmMalformed, exception->ਅੰਕ_ਦਲੀਲਾਂ != Get_Func_Type_ਅੰਕ_Params(type));

            ਲਈ (u32 a = 0; a < exception->ਅੰਕ_ਦਲੀਲਾਂ; ++a) {
_               (Put_ਮੁੱਲ_ਬਿੱਟ(s, GetFuncTypeParamType(type, (u16)a), exception->ਦਲੀਲਾਂ[a]));
            }
        }
#  endif
        ਜਦੋਂ (ਪੂਰਾ_Continuations < s->continuations.ਗਿਣਤੀ_2 and ਪੂਰਾ_Exceptions == s->exceptions.ਗਿਣਤੀ_2) {
            u32 id = ਪੂਰਾ_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.ਚੀਜ਼ਾਂ[id]));
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveSections (M3SnapshotSave* s)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (not s->writing) {
        ਵਾਪਸ DiscoverReferences(s);
    }

    ਜੇ (not s->embedded) {
_       (Put(s, c_snapshotMagic, ਆਕਾਰ(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_ਸੰਸਕਰਣ));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    ਜੇ (s->module->ਅੰਕ_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_ਸਮ੍ਰਿਤੀ, SaveMemories));
    }
    ਜੇ (s->module->ਅੰਕ_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_ਸਾਰਣੀ, SaveTables));
    }
    ਜੇ (s->module->ਅੰਕ_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    ਜੇ (s->module->ਅੰਕ_ਅੰਕੜੇ_Segments or s->module->ਅੰਕ_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  ਜੇ d_m3HasExceptionHandling
    ਜੇ (s->exceptions.ਗਿਣਤੀ_2) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    ਜੇ (s->continuations.ਗਿਣਤੀ_2) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3SnapshotSuspendSite {
    ਅਚਲ M3SnapshotMap* map;
    ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ*   ਬਿੰਦੂ;
    u32                  ਨਿਸ਼ਾਨਾ;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_Snapshot_ਲੱਦੋ {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    ਖਾਲੀ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ਢੇਰ;
    bool      ਬਦਲਿਆ;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    ਅੰਕ_Continuations;
    u64                    required_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ;
    u64                    required_ਸਾਰਣੀ_Elements;
    u64                    loaded_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ;
    u64                    loaded_ਸਾਰਣੀ_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  ਜੇ d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 ਅੰਕ_Exceptions;

    M3_ਚੌਖਟਾ* ਚੌਖਟੇ;           // a suspended continuation's frames as they are put together, outermost first

    ਬਾਈਟਾਂ_t pos;               // bounded section payload, read using the Wasm parser's helpers
    ਬਾਈਟਾਂ_t ਸਿਰਾ;
    bool    ਵਿੱਚ_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    ਵੇਖਿਆ_Sections[256];
} M3_Snapshot_ਲੱਦੋ;

ਸਥਿਰ
M3_ਨਤੀਜਾ Get (M3_Snapshot_ਲੱਦੋ* l, ਖਾਲੀ* o_ਅੰਕੜੇ, size_t i_ਮਾਪ)
{
    ਜੇ (not l->ਵਿੱਚ_Section) {
        ਵਾਪਸ l->reader(o_ਅੰਕੜੇ, i_ਮਾਪ, l->userdata);
    }
    ਜੇ (i_ਮਾਪ > (size_t)(l->ਸਿਰਾ - l->pos)) {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
    ਜੇ (i_ਮਾਪ) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_ਅੰਕੜੇ, l->pos, i_ਮਾਪ);
        l->pos += i_ਮਾਪ;
    }
    ਵਾਪਸ m3Err_none;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetU8 (M3_Snapshot_ਲੱਦੋ* l, u8* o_ਮੁੱਲ)
{
    ਵਾਪਸ Get(l, o_ਮੁੱਲ, ਆਕਾਰ(*o_ਮੁੱਲ));
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetBool (M3_Snapshot_ਲੱਦੋ* l, u8* o_ਮੁੱਲ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = GetU8(l, o_ਮੁੱਲ);
    ਵਾਪਸ ਨਤੀਜਾ ? ਨਤੀਜਾ : (*o_ਮੁੱਲ <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetLEB_u32 (M3_Snapshot_ਲੱਦੋ* l, u32* o_ਮੁੱਲ)
{
    ਜੇ (l->ਵਿੱਚ_Section) {
        ਵਾਪਸ ReadLEB_u32(o_ਮੁੱਲ, &l->pos, l->ਸਿਰਾ);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    ਲਈ (u32 i = 0; i < ਆਕਾਰ(encoded); ++i) {
        M3_ਨਤੀਜਾ ਨਤੀਜਾ = GetU8(l, &encoded[i]);
        ਜੇ (ਨਤੀਜਾ) {
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        ਜੇ (not (encoded[i] & 0x80)) {
            ਬਾਈਟਾਂ_t pos = encoded;
            ਵਾਪਸ ReadLEB_u32(o_ਮੁੱਲ, &pos, encoded + i + 1);
        }
    }
    ਵਾਪਸ m3_Err_leb_ਉਛਾਲ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetLEB_u64 (M3_Snapshot_ਲੱਦੋ* l, u64* o_ਮੁੱਲ)
{
    ਵਾਪਸ ReadLebUnsigned(o_ਮੁੱਲ, 64, &l->pos, l->ਸਿਰਾ);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetLEB_i64 (M3_Snapshot_ਲੱਦੋ* l, i64* o_ਮੁੱਲ)
{
    ਵਾਪਸ ReadLEB_i64(o_ਮੁੱਲ, &l->pos, l->ਸਿਰਾ);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Skip_ਬਾਈਟਾਂ (M3_Snapshot_ਲੱਦੋ* l, size_t i_ਮਾਪ)
{
    ਜੇ (i_ਮਾਪ > (size_t)(l->ਸਿਰਾ - l->pos)) {
        ਵਾਪਸ m3Err_wasmUnderrun;
    }
    l->pos += i_ਮਾਪ;
    ਵਾਪਸ m3Err_none;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Get_Ref_ਮੁੱਲ (M3_Snapshot_ਲੱਦੋ* l, m3type_t i_type, ਖਾਲੀ** o_reference)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u8       ਕਿਸਮ   = 0;
    u64      id     = 0;

    *o_reference = ਨਿਰਾ;

_   (GetU8(l, &ਕਿਸਮ));
    ਜੇ (ਕਿਸਮ == 0x00) {
        ਵਾਪਸ (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    ਜੇ (ਕਿਸਮ == 0x01) {
        _throwif(m3Err_wasmMalformed, ਅਧਾਰ_Type_ਦਾ(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->ਅੰਕ_Functions);
#  ਜੇ d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_ਦਾ(Ref_Type_ਦਾ_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } ਨਹੀਂਤਾਂ ਜੇ (ਕਿਸਮ == 0x02) {
        _throwif(m3Err_wasmMalformed, ਅਧਾਰ_Type_ਦਾ(i_type) != c_m3Type_externref);
        ਅਚਲ M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        ਜੇ (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  ਜੇ d_m3HasExceptionHandling
    ਨਹੀਂਤਾਂ ਜੇ (ਕਿਸਮ == 0x03) {
        _throwif(m3Err_wasmMalformed, ਅਧਾਰ_Type_ਦਾ(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->ਅੰਕ_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    ਨਹੀਂਤਾਂ ਜੇ (ਕਿਸਮ == 0x04) {
        _throwif(m3Err_wasmMalformed, ਅਧਾਰ_Type_ਦਾ(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->ਅੰਕ_Continuations);
        *o_reference = l->continuations[id];
#  ਜੇ d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        ਜੇ (not l->checking and ਢੇਰ_Type_ਦਾ(i_type) != d_m3_Type_ਢੇਰ_Abstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } ਨਹੀਂਤਾਂ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetLE32 (M3_Snapshot_ਲੱਦੋ* l, u32* o_ਮੁੱਲ)
{
    u8       ਬਾਈਟਾਂ[4];
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = Get(l, ਬਾਈਟਾਂ, ਆਕਾਰ(ਬਾਈਟਾਂ));

    *o_ਮੁੱਲ = 0;

    ਲਈ (u32 i = 0; i < ਆਕਾਰ(ਬਾਈਟਾਂ); ++i) {
        *o_ਮੁੱਲ |= (u32)ਬਾਈਟਾਂ[i] << (8 * i);
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetLE64 (M3_Snapshot_ਲੱਦੋ* l, u64* o_ਮੁੱਲ)
{
    u8       ਬਾਈਟਾਂ[8];
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = Get(l, ਬਾਈਟਾਂ, ਆਕਾਰ(ਬਾਈਟਾਂ));

    *o_ਮੁੱਲ = 0;

    ਲਈ (u32 i = 0; i < ਆਕਾਰ(ਬਾਈਟਾਂ); ++i) {
        *o_ਮੁੱਲ |= (u64)ਬਾਈਟਾਂ[i] << (8 * i);
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ਸਥਿਰ
M3_ਨਤੀਜਾ Get_ਮੁੱਲ_ਟੈਗ (M3_Snapshot_ਲੱਦੋ* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_ਮੰਜ਼ਿਲ_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and ਅਧਾਰ_Type_ਦਾ(*o_type) != ਅਧਾਰ_Type_ਦਾ(i_expectedType));
    ਜੇ (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ਸਥਿਰ
M3_ਨਤੀਜਾ Get_ਮੁੱਲ_Body (M3_Snapshot_ਲੱਦੋ* l, m3type_t i_type, u64* o_ਬਿੱਟ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    *o_ਬਿੱਟ = 0;

    ਜੇ (IsRefType(i_type)) {
        ਖਾਲੀ* reference = ਨਿਰਾ;
_       (Get_Ref_ਮੁੱਲ(l, i_type, &reference));
        *o_ਬਿੱਟ = (u64)(uintptr_t)reference;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ_Type_ਦਾ(i_type) == c_m3Type_i32) {
        i32 ਮੁੱਲ = 0;
_       (ReadLEB_i32(&ਮੁੱਲ, &l->pos, l->ਸਿਰਾ));
        *o_ਬਿੱਟ = (u32)ਮੁੱਲ;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ_Type_ਦਾ(i_type) == c_m3Type_i64) {
        i64 ਮੁੱਲ = 0;
_       (GetLEB_i64(l, &ਮੁੱਲ));
        *o_ਬਿੱਟ = (u64)ਮੁੱਲ;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ_Type_ਦਾ(i_type) == c_m3Type_f32) {
        u32 ਬਿੱਟ = 0;
_       (GetLE32(l, &ਬਿੱਟ));
        *o_ਬਿੱਟ = ਬਿੱਟ;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ_Type_ਦਾ(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_ਬਿੱਟ));
    } ਨਹੀਂਤਾਂ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Get_ਮੁੱਲ_ਬਿੱਟ (M3_Snapshot_ਲੱਦੋ* l, m3type_t i_expectedType, u64* o_ਬਿੱਟ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    m3type_t type;

_   (Get_ਮੁੱਲ_ਟੈਗ(l, i_expectedType, &type));
_   (Get_ਮੁੱਲ_Body(l, type, o_ਬਿੱਟ));

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ Get_ਮੁੱਲ (M3_Snapshot_ਲੱਦੋ* l, m3type_t i_expectedType, ਖਾਲੀ* o_where)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    ਜੇ (l->checking) {
        o_where = scratch;
    }

_   (Get_ਮੁੱਲ_ਟੈਗ(l, i_expectedType, &type));

    ਜੇ (IsRefType(type)) {
        ਖਾਲੀ* reference = ਨਿਰਾ;
        u64   ਬਿੱਟ      = 0;
_       (Get_ਮੁੱਲ_Body(l, type, &ਬਿੱਟ));
        reference = (ਖਾਲੀ*)(uintptr_t)ਬਿੱਟ;
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_where, &reference, ਆਕਾਰ(reference));
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਧਾਰ_Type_ਦਾ(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } ਨਹੀਂਤਾਂ {
        u64 ਬਿੱਟ = 0;
_       (Get_ਮੁੱਲ_Body(l, type, &ਬਿੱਟ));

        ਜੇ (Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(type)) {
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_where, &ਬਿੱਟ, ਆਕਾਰ(ਬਿੱਟ));
        } ਨਹੀਂਤਾਂ {
            u32 narrow = (u32)ਬਿੱਟ;
            ਸਮ੍ਰਿਤੀ_ਨਕਲ(o_where, &narrow, ਆਕਾਰ(narrow));
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ GetFunction (M3_Snapshot_ਲੱਦੋ* l, IM3Function* o_function)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->ਅੰਕ_Functions);
    *o_function = &l->module->functions[index];

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_ਸਮ੍ਰਿਤੀ_Chunks (M3_Snapshot_ਲੱਦੋ* l, u8* o_ਬਾਈਟਾਂ, size_t i_ਮਾਪ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਲਈ (;;) {
        u8  chunkType = 0;
        u32 offset = 0, ਲੰਮਾਈ = 0;

_       (GetU8(l, &chunkType));

        ਜੇ (chunkType == d_m3_Chunk_ਸਿਰਾ) {
            ਰੋਕੋ;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &ਲੰਮਾਈ));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_ਮਾਪ or ਲੰਮਾਈ > i_ਮਾਪ - (size_t)offset);

        ਜੇ (chunkType == d_m3ChunkRaw) {
            ਜੇ (l->checking or ਲੰਮਾਈ == 0) {
_               (Skip_ਬਾਈਟਾਂ(l, ਲੰਮਾਈ));
            } ਨਹੀਂਤਾਂ {
_               (Get(l, o_ਬਾਈਟਾਂ + offset, ਲੰਮਾਈ));
            }
        } ਨਹੀਂਤਾਂ ਜੇ (chunkType == d_m3ChunkFillFF) {
            ਜੇ (not l->checking and ਲੰਮਾਈ) {
                memset(o_ਬਾਈਟਾਂ + offset, 0xFF, ਲੰਮਾਈ);
            }
        } ਨਹੀਂਤਾਂ {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Memories (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ      = m3Err_none;
    IM3Module module      = l->module;
    u32       ਅੰਕ_Memories = 0;
    l->loaded_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ  = 0;

_   (GetLEB_u32(l, &ਅੰਕ_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, ਅੰਕ_Memories == 0 or ਅੰਕ_Memories != module->ਅੰਕ_Memories);

    ਲਈ (u32 m = 0; m < ਅੰਕ_Memories; ++m) {
        u32       mem_ਸੂਚਕਾਂਕ = 0, ਪਹਿਲਾ_ਸੂਚਕਾਂਕ = 0, ਪੰਨਾ_ਬਿੱਟ = 0, ਅੰਕ_ਪੰਨੇ = 0;
        IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ;

_       (GetLEB_u32(l, &mem_ਸੂਚਕਾਂਕ));
_       (GetLEB_u32(l, &ਪਹਿਲਾ_ਸੂਚਕਾਂਕ));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_ਸੂਚਕਾਂਕ != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 ਪਹਿਲਾ_ਸੂਚਕਾਂਕ != ਸਮ੍ਰਿਤੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ(module, m));
        ਜੇ (ਪਹਿਲਾ_ਸੂਚਕਾਂਕ != m) {
            ਜਾਰੀ;
        }

_       (GetLEB_u32(l, &ਪੰਨਾ_ਬਿੱਟ));
_       (GetLEB_u32(l, &ਅੰਕ_ਪੰਨੇ));

        ਸਮ੍ਰਿਤੀ = module->memories[mem_ਸੂਚਕਾਂਕ];
        _throwif("a shared memory cannot be restored from a snapshot", ਸਮ੍ਰਿਤੀ and ਸਮ੍ਰਿਤੀ->isShared);

        size_t ਬਾਈਟਾਂ = 0;
        u8*    ਅੰਕੜੇ  = ਨਿਰਾ;

        ਜੇ (ਸਮ੍ਰਿਤੀ) {
            _throwif(m3Err_wasmMalformed, ਪੰਨਾ_ਬਿੱਟ >= 32 or (1u << ਪੰਨਾ_ਬਿੱਟ) != ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(ਸਮ੍ਰਿਤੀ));
            _throwif(m3Err_wasmMalformed, ਅੰਕ_ਪੰਨੇ < ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ or ਅੰਕ_ਪੰਨੇ > ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ);
            _throwif(m3Err_wasmMalformed, ਅੰਕ_ਪੰਨੇ > SIZE_MAX / ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(ਸਮ੍ਰਿਤੀ));
            ਬਾਈਟਾਂ = (size_t)ਅੰਕ_ਪੰਨੇ * ਸਮ੍ਰਿਤੀ_ਪੰਨਾ_ਮਾਪ(ਸਮ੍ਰਿਤੀ);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ and ਬਾਈਟਾਂ > (size_t)l->runtime->ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ);

            ਜੇ (not l->checking) {
                ਜੇ (ਸਮ੍ਰਿਤੀ->ਅੰਕ_ਪੰਨੇ < ਅੰਕ_ਪੰਨੇ) {
_                   (Resize_ਸਮ੍ਰਿਤੀ(l->runtime, ਸਮ੍ਰਿਤੀ, ਅੰਕ_ਪੰਨੇ));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         ਸਮ੍ਰਿਤੀ->mallocated and ਸਮ੍ਰਿਤੀ->mallocated->ਲੰਮਾਈ < ਬਾਈਟਾਂ);
            }

            l->loaded_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ += ਬਾਈਟਾਂ;

            ਜੇ (ਬਾਈਟਾਂ and ਸਮ੍ਰਿਤੀ->mallocated) {
                ਅੰਕੜੇ = m3_Mem_ਅੰਕੜੇ(ਸਮ੍ਰਿਤੀ->mallocated);
            }
        } ਨਹੀਂਤਾਂ {
            _throwif(m3Err_wasmMalformed, ਅੰਕ_ਪੰਨੇ != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        ਜੇ (ਅੰਕੜੇ and not l->checking) {
            memset(ਅੰਕੜੇ, 0, ਬਾਈਟਾਂ);
        }

_       (ਲੱਦੋ_ਸਮ੍ਰਿਤੀ_Chunks(l, ਅੰਕੜੇ, ਬਾਈਟਾਂ));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ != l->required_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Globals (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ     = m3Err_none;
    IM3Module module     = l->module;
    u32       ਅੰਕ_Globals = 0;

_   (GetLEB_u32(l, &ਅੰਕ_Globals));
    _throwif(m3Err_wasmMalformed, ਅੰਕ_Globals == 0 or ਅੰਕ_Globals != module->ਅੰਕ_Globals);

    ਲਈ (u32 g = 0; g < ਅੰਕ_Globals; ++g) {
        u32       global_ਸੂਚਕਾਂਕ = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_ਸੂਚਕਾਂਕ));
        _throwif(m3Err_wasmMalformed, global_ਸੂਚਕਾਂਕ != g);

        global = &module->globals[global_ਸੂਚਕਾਂਕ];
        cell   = global->resolved ? global->resolved : global;

_       (Get_ਮੁੱਲ(l, global->type, &cell->i64_ਮੁੱਲ));
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Tables (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ       = m3Err_none;
    IM3Module module       = l->module;
    u32       ਅੰਕ_Tables    = 0;
    l->loaded_ਸਾਰਣੀ_Elements = 0;

_   (GetLEB_u32(l, &ਅੰਕ_Tables));
    _throwif(m3Err_wasmMalformed, ਅੰਕ_Tables == 0 or ਅੰਕ_Tables != module->ਅੰਕ_Tables);

    ਲਈ (u32 t = 0; t < ਅੰਕ_Tables; ++t) {
        u32      ਸਾਰਣੀ_ਸੂਚਕਾਂਕ = 0, ਪਹਿਲਾ_ਸੂਚਕਾਂਕ = 0, ਮਾਪ = 0;
        u8       valtype = 0;
        IM3_ਸਾਰਣੀ ਸਾਰਣੀ_2;

_       (GetLEB_u32(l, &ਸਾਰਣੀ_ਸੂਚਕਾਂਕ));
_       (GetLEB_u32(l, &ਪਹਿਲਾ_ਸੂਚਕਾਂਕ));

        _throwif(m3Err_wasmMalformed, ਸਾਰਣੀ_ਸੂਚਕਾਂਕ != t);
        _throwif("the snapshot shares tables between imports differently",
                 ਪਹਿਲਾ_ਸੂਚਕਾਂਕ != ਸਾਰਣੀ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ(module, t));
        ਜੇ (ਪਹਿਲਾ_ਸੂਚਕਾਂਕ != t) {
            ਜਾਰੀ;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &ਮਾਪ));

        ਸਾਰਣੀ_2 = module->tables[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_ਮੰਜ਼ਿਲ_Val_Type(ਸਾਰਣੀ_2->type));
        _throwif(m3Err_wasmMalformed, ਮਾਪ > (ਸਾਰਣੀ_2->hasMax ? ਸਾਰਣੀ_2->max_ਮਾਪ : d_m3_Max_Sane_ਸਾਰਣੀ_ਮਾਪ));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, ਮਾਪ < ਸਾਰਣੀ_2->ਮਾਪ);

        l->loaded_ਸਾਰਣੀ_Elements += ਮਾਪ;

        ਜੇ (not l->checking and ਮਾਪ != ਸਾਰਣੀ_2->ਮਾਪ) {
            ਖਾਲੀ** elements;

            _throwif(m3_Err_ਸਾਰਣੀ_ਹੱਦਬੰਦੀ_Exceeded, l->runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ and
                                                 ਮਾਪ - ਸਾਰਣੀ_2->ਮਾਪ > l->runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ - l->runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ);
            elements = m3_ReallocArray(ਖਾਲੀ*, ਸਾਰਣੀ_2->elements, ਮਾਪ, ਸਾਰਣੀ_2->ਮਾਪ);
            _throwifnull(elements);

            l->runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ += ਮਾਪ - ਸਾਰਣੀ_2->ਮਾਪ;
            ਸਾਰਣੀ_2->elements = elements;
            ਸਾਰਣੀ_2->ਮਾਪ     = ਮਾਪ;
        }

        ਲਈ (u32 e = 0; e < ਮਾਪ; ++e) {
            ਖਾਲੀ* reference = ਨਿਰਾ;
_           (Get_Ref_ਮੁੱਲ(l, ਸਾਰਣੀ_2->type, &reference));
            ਜੇ (not l->checking) {
                ਸਾਰਣੀ_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_ਸਾਰਣੀ_Elements != l->required_ਸਾਰਣੀ_Elements);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Segments (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ  ਨਤੀਜਾ = m3Err_none;
    IM3Module module = l->module;
    u32       ਗਿਣਤੀ_2  = 0;

_   (GetLEB_u32(l, &ਗਿਣਤੀ_2));
    _throwif(m3Err_wasmMalformed, ਗਿਣਤੀ_2 != module->ਅੰਕ_ਅੰਕੜੇ_Segments);
    _throwif(m3Err_wasmMalformed, module->ਅੰਕ_ਅੰਕੜੇ_Segments == 0 and module->ਅੰਕ_Element_Segments == 0);

    ਲਈ (u32 i = 0; i < ਗਿਣਤੀ_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ਜੇ (not l->checking) {
            module->ਅੰਕੜੇ_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &ਗਿਣਤੀ_2));
    _throwif(m3Err_wasmMalformed, ਗਿਣਤੀ_2 != module->ਅੰਕ_Element_Segments);

    ਲਈ (u32 i = 0; i < ਗਿਣਤੀ_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ਜੇ (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// A frame for the continuation being put together, outermost first
ਸਥਿਰ
M3_ਨਤੀਜਾ ਜੋੜੋ_ਚੌਖਟਾ (M3_Snapshot_ਲੱਦੋ* l, u32* io_ਅੰਕ_ਚੌਖਟੇ, M3_ਚੌਖਟਾ_ਕਿਸਮ i_ਕਿਸਮ, m3stack_t i_sp, M3_ਚੌਖਟਾ** o_ਚੌਖਟਾ)
{
    ਜੇ (*io_ਅੰਕ_ਚੌਖਟੇ >= d_m3_Continuation_Max_ਚੌਖਟੇ) {
        ਵਾਪਸ "the snapshot's frames do not fit this build's";
    }

    M3_ਚੌਖਟਾ* ਚੌਖਟਾ = &l->ਚੌਖਟੇ[(*io_ਅੰਕ_ਚੌਖਟੇ)++];

    memset(ਚੌਖਟਾ, 0, ਆਕਾਰ(*ਚੌਖਟਾ));
    ਚੌਖਟਾ->ਕਿਸਮ   = (u8)i_ਕਿਸਮ;
    ਚੌਖਟਾ->sp     = i_sp;
    ਚੌਖਟਾ->ਸਮ੍ਰਿਤੀ = Module_Memory0(l->module);

    *o_ਚੌਖਟਾ = ਚੌਖਟਾ;

    ਵਾਪਸ m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ਸਥਿਰ
M3_ਨਤੀਜਾ PrepareFunction (IM3Function i_function)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਜੇ (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਜੋੜੋ_ਖੰਡ_ਚੌਖਟੇ (M3_Snapshot_ਲੱਦੋ* l, ਅਚਲ M3SnapshotMap* i_map, ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* i_ਬਿੰਦੂ, m3stack_t i_sp,
                         u32* io_ਅੰਕ_ਚੌਖਟੇ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਲਈ (u32 b = 0; b < i_map->ਅੰਕ_ਖੰਡਾਂ; ++b) {
        ਅਚਲ M3_ਖੰਡ_ਸ਼ੁਰੂ* ਖੰਡ = &i_map->ਖੰਡਾਂ[b];
        M3_ਚੌਖਟਾ*            ਚੌਖਟਾ;

        ਜੇ (not Encloses_ਵਿੱਥ(ਖੰਡ, i_ਬਿੰਦੂ->wasm_ਵਿੱਥ)) {
            ਜਾਰੀ;
        }

        ਜੇ (ਖੰਡ->opcode == c_waOp_loop) {
_           (ਜੋੜੋ_ਚੌਖਟਾ(l, io_ਅੰਕ_ਚੌਖਟੇ, ਚੌਖਟਾ_loop, i_sp, &ਚੌਖਟਾ));
            ਚੌਖਟਾ->pc = ਖੰਡ->pc;
        }
#  ਜੇ d_m3HasExceptionHandling
        ਨਹੀਂਤਾਂ {
_           (ਜੋੜੋ_ਚੌਖਟਾ(l, io_ਅੰਕ_ਚੌਖਟੇ, ਚੌਖਟਾ_try, i_sp, &ਚੌਖਟਾ));
            ਚੌਖਟਾ->pc                = ਖੰਡ->pc;
            ਚੌਖਟਾ->try_.ਅੰਕ_Clauses   = ਖੰਡ->ਅੰਕ_Clauses;
            ਚੌਖਟਾ->try_.handlers_ਜੀਵਤ = Handlers_ਜੀਵਤ_At(i_map, i_ਬਿੰਦੂ, ਖੰਡ);
        }
#  endif
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Puts a function's values where this build keeps them at the safepoint
ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_ਮੁੱਲਾਂ (M3_Snapshot_ਲੱਦੋ* l, m3slot_t* i_ਅਧਾਰ, u32 i_ਅੰਕ_Slots, m3stack_t i_sp,
                     ਅਚਲ M3SnapshotMap* i_map, ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ* i_ਬਿੰਦੂ, ਅਚਲ M3_ਚੌਖਟਾ_Registers* i_registers)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ    = m3Err_none;
    u32      ਅੰਕ_ਮੁੱਲਾਂ = 0;

_   (GetLEB_u32(l, &ਅੰਕ_ਮੁੱਲਾਂ));
    _throwif("the snapshot's frame does not match what this build compiled", ਅੰਕ_ਮੁੱਲਾਂ != i_map->ਅੰਕ_Locals + i_ਬਿੰਦੂ->ਅੰਕ_ਮੁੱਲਾਂ);

    ਲਈ (u32 v = 0; v < ਅੰਕ_ਮੁੱਲਾਂ; ++v) {
        ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* ਮੁੱਲ     = ਚੌਖਟਾ_ਮੁੱਲ(i_map, i_ਬਿੰਦੂ, v);
        u64                ਸ਼ਬਦ      = 0;
        ਖਾਲੀ*              reference = ਨਿਰਾ;
        u8*                where;

_       (Locate_ਮੁੱਲ(i_ਅਧਾਰ, i_ਅੰਕ_Slots, i_sp, ਮੁੱਲ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ਜੇ (ਅਧਾਰ_Type_ਦਾ(ਮੁੱਲ->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_ਮੁੱਲ_ਟੈਗ(l, ਮੁੱਲ->type, &type));
            ਜੇ (l->checking) {
_               (Skip_ਬਾਈਟਾਂ(l, 16));
            } ਨਹੀਂਤਾਂ {
_               (Get(l, where, 16));
            }
            ਜਾਰੀ;
        }

        l->allowUnfilledRef = i_ਬਿੰਦੂ->ਕਿਸਮ == safepoint_suspend and
                              v >= ਅੰਕ_ਮੁੱਲਾਂ - i_ਬਿੰਦੂ->ਅੰਕ_Results;
_       (Get_ਮੁੱਲ_ਬਿੱਟ(l, ਮੁੱਲ->type, &ਸ਼ਬਦ));
        l->allowUnfilledRef = false;

        ਜੇ (IsRefType(ਮੁੱਲ->type)) {
            reference = (ਖਾਲੀ*)(uintptr_t)ਸ਼ਬਦ;
        }

        ਜੇ (not l->checking) {
_           (Write_ਮੁੱਲ(where, i_registers, ਮੁੱਲ, ਸ਼ਬਦ, reference));
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Suspended_Body (M3_Snapshot_ਲੱਦੋ* l, IM3Continuation io_cont, u32 i_id)
{
    M3_ਨਤੀਜਾ    ਨਤੀਜਾ    = m3Err_none;
    u32         ਅੰਕ_ਚੌਖਟੇ = 0;
    M3_ਚੌਖਟਾ*    call_ਚੌਖਟਾ = ਨਿਰਾ;
    IM3Function function  = ਨਿਰਾ;
    m3slot_t*   ਅਧਾਰ;
    u32         ਅੰਕ_Slots;
    u32         ਅੰਕ_Activations = 0;
    IM3FuncType expected       = io_cont->ਇੰਦਰਾਜ_Function->funcType;
    m3stack_t   sp;

_   (Continuation_ਢੇਰ(l->runtime, io_cont, &ਅਧਾਰ, &ਅੰਕ_Slots));

_   (GetLEB_u32(l, &ਅੰਕ_Activations));
    _throwif(m3Err_wasmMalformed, ਅੰਕ_Activations == 0 or ਅੰਕ_Activations > d_m3_Continuation_Max_ਚੌਖਟੇ);

    io_cont->pc                = ਨਿਰਾ;
    io_cont->r0                = 0;
    io_cont->ਅੰਕ_Suspend_Results = 0;
    io_cont->suspend_ਬਿੰਦੂ      = safepoint_op;
#  ਜੇ d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = ਅਧਾਰ;

    ਲਈ (u32 a = 0; a < ਅੰਕ_Activations; ++a) {
        bool                 isInnermost = (a + 1 == ਅੰਕ_Activations);
        ਅਚਲ M3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ*   ਬਿੰਦੂ;
        M3_ਚੌਖਟਾ_Registers     registers;
        u8                   ਕਿਸਮ       = 0;
        u32                  wasm_ਵਿੱਥ = 0, ਨਿਸ਼ਾਨਾ_Loop = 0;
        ਅਚਲ M3SnapshotMap* map;

        memset(&registers, 0, ਆਕਾਰ(registers));

_       (GetFunction(l, &function));

        // A return_call hands its frame to the function it calls, so the
        // function standing here need not be the one the continuation was made
        // with, or the one the call outside it named. It does return what
        // they would have.
        _throwif("a frame in the snapshot does not return what its caller expects",
                 not ReturnsWhat(function->funcType, expected));

_       (PrepareFunction(function));

        map = function->snapshotMap;

        _throwif("the snapshot's stack does not fit this runtime's",
                 (u32)(sp - ਅਧਾਰ) + function->max_ਢੇਰ_Slots > ਅੰਕ_Slots);

        // the call the frame outside this one is waiting on
        ਜੇ (call_ਚੌਖਟਾ) {
            call_ਚੌਖਟਾ->call.function = function;
        }

#  ਜੇ d_m3_ਇੰਦਰਾਜ_Keeps_ਚੌਖਟਾ
        {
            M3_ਚੌਖਟਾ* ਇੰਦਰਾਜ;
_           (ਜੋੜੋ_ਚੌਖਟਾ(l, &ਅੰਕ_ਚੌਖਟੇ, ਚੌਖਟਾ_ਇੰਦਰਾਜ, sp, &ਇੰਦਰਾਜ));
            ਇੰਦਰਾਜ->pc             = function->compiled;
            ਇੰਦਰਾਜ->ਇੰਦਰਾਜ.function = function;
        }
#  endif

_       (GetU8(l, &ਕਿਸਮ));
_       (GetLEB_u32(l, &wasm_ਵਿੱਥ));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        ਜੇ (ਕਿਸਮ == safepoint_op) {
            ਅਚਲ M3_ਖੰਡ_ਸ਼ੁਰੂ* loop;

_           (GetLEB_u32(l, &ਨਿਸ਼ਾਨਾ_Loop));

            loop = ਲੱਭੋ_ਖੰਡ_At(map, c_waOp_loop, ਨਿਸ਼ਾਨਾ_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_ਵਿੱਥ(loop, wasm_ਵਿੱਥ));
        }

        ਬਿੰਦੂ = ਲੱਭੋ_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_At(map, wasm_ਵਿੱਥ, ਕਿਸਮ, ਨਿਸ਼ਾਨਾ_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not ਬਿੰਦੂ);

_       (ਜੋੜੋ_ਖੰਡ_ਚੌਖਟੇ(l, map, ਬਿੰਦੂ, sp, &ਅੰਕ_ਚੌਖਟੇ));

        ਜੇ (not isInnermost) {
            _throwif(m3Err_wasmMalformed, ਕਿਸਮ != safepoint_call);

_           (ਜੋੜੋ_ਚੌਖਟਾ(l, &ਅੰਕ_ਚੌਖਟੇ, ਚੌਖਟਾ_call, sp, &call_ਚੌਖਟਾ));
            call_ਚੌਖਟਾ->pc = ਬਿੰਦੂ->pc;
            registers.r0  = &call_ਚੌਖਟਾ->call.r0;
#  ਜੇ d_m3HasFloat
            registers.fp0 = &call_ਚੌਖਟਾ->call.fp0;
#  endif
        } ਨਹੀਂਤਾਂ ਜੇ (ਕਿਸਮ == safepoint_resume) {
            M3_ਚੌਖਟਾ* ਚੌਖਟਾ;
            u32      ਅੰਕ_Results = ਬਿੰਦੂ->ਅੰਕ_Results;
            pc_t     resultsPC  = ਬਿੰਦੂ->pc - 2 * ਅੰਕ_Results;

_           (ਜੋੜੋ_ਚੌਖਟਾ(l, &ਅੰਕ_ਚੌਖਟੇ, ਚੌਖਟਾ_resume, sp, &ਚੌਖਟਾ));
            ਚੌਖਟਾ->pc                 = ਬਿੰਦੂ->pc;
            ਚੌਖਟਾ->resume.resultsPC   = resultsPC;
            ਚੌਖਟਾ->resume.ਅੰਕ_Results  = ਅੰਕ_Results;
            ਚੌਖਟਾ->resume.ਅੰਕ_Handlers = ਬਿੰਦੂ->aux;
            ਚੌਖਟਾ->resume.handlersPC  = resultsPC - 1 - 3 * ਬਿੰਦੂ->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            call_ਚੌਖਟਾ = ਚੌਖਟਾ;
        } ਨਹੀਂਤਾਂ {
            _throwif(m3Err_wasmMalformed, ਕਿਸਮ != safepoint_op and ਕਿਸਮ != safepoint_ਇੰਦਰਾਜ and ਕਿਸਮ != safepoint_suspend);

            io_cont->pc           = ਬਿੰਦੂ->pc;
            io_cont->suspend_ਬਿੰਦੂ = ਕਿਸਮ;
            registers.r0          = &io_cont->r0;
#  ਜੇ d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (ਲੱਦੋ_ਮੁੱਲਾਂ(l, ਅਧਾਰ, ਅੰਕ_Slots, sp, map, ਬਿੰਦੂ, &registers));

        ਜੇ (not l->checking) {
            RestoreConstants(function, sp);
        }

        ਜੇ (not isInnermost) {
            sp = sp + ਬਿੰਦੂ->aux;
_           (CallSiteType(function, ਬਿੰਦੂ, &expected));
        } ਨਹੀਂਤਾਂ ਜੇ (ਕਿਸਮ == safepoint_resume) {
            ਖਾਲੀ* cont = ਨਿਰਾ;
_           (Get_Ref_ਮੁੱਲ(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ਨਿਰਾ);

            call_ਚੌਖਟਾ->resume.cont = (IM3Continuation)cont;
            ਜੇ (not l->checking) {
                l->suspendSites[i_id].ਬਿੰਦੂ = ਬਿੰਦੂ;
                ਲਈ (u32 i = 1; i < l->ਅੰਕ_Continuations; ++i) {
                    ਜੇ (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].ਨਿਸ਼ਾਨਾ = i;
                        l->suspendSites[i].resumed   = true;
                        ਰੋਕੋ;
                    }
                }
            }
        } ਨਹੀਂਤਾਂ {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            ਜੇ (ਬਿੰਦੂ->flags & d_m3_ਸੁਰੱਖਿਅਤ_ਬਿੰਦੂ_ਲਿਆ_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            ਜੇ (ਕਿਸਮ == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, ਬਿੰਦੂ->ਅੰਕ_Results > ਬਿੰਦੂ->ਅੰਕ_ਮੁੱਲਾਂ or
                                                ਬਿੰਦੂ->ਅੰਕ_Results > d_m3MaxContinuationPayload);

                io_cont->ਅੰਕ_Suspend_Results = ਬਿੰਦੂ->ਅੰਕ_Results;
                ਜੇ (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].ਬਿੰਦੂ = ਬਿੰਦੂ;
                }

                ਲਈ (u32 i = 0; i < ਬਿੰਦੂ->ਅੰਕ_Results; ++i) {
                    ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* ਮੁੱਲ = &map->ਮੁੱਲਾਂ[ਬਿੰਦੂ->ਪਹਿਲਾ_ਮੁੱਲ + ਬਿੰਦੂ->ਅੰਕ_ਮੁੱਲਾਂ - ਬਿੰਦੂ->ਅੰਕ_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias(ਮੁੱਲ->ਥਾਂ));

                    io_cont->suspend_ਨਤੀਜਾ_Offsets[i] = ਮੁੱਲ->ਥਾਂ;
                    io_cont->suspend_ਨਤੀਜਾ_Is64[i]    = Is64_ਦੋਆਧਾਰੀ_ਅੰਕ_Type(ਮੁੱਲ->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    ਜੇ (l->checking) {
        ਵਾਪਸ m3Err_none;
    }

    // held innermost first
    ਜੇ (ਅੰਕ_ਚੌਖਟੇ > io_cont->ਚੌਖਟੇ_Cap) {
        M3_ਚੌਖਟਾ* ਚੌਖਟੇ = m3_ReallocArray(M3_ਚੌਖਟਾ, io_cont->ਚੌਖਟੇ, ਅੰਕ_ਚੌਖਟੇ, io_cont->ਚੌਖਟੇ_Cap);
        _throwifnull(ਚੌਖਟੇ);

        io_cont->ਚੌਖਟੇ    = ਚੌਖਟੇ;
        io_cont->ਚੌਖਟੇ_Cap = ਅੰਕ_ਚੌਖਟੇ;
    }

    ਲਈ (u32 k = 0; k < ਅੰਕ_ਚੌਖਟੇ; ++k) {
        io_cont->ਚੌਖਟੇ[ਅੰਕ_ਚੌਖਟੇ - 1 - k] = l->ਚੌਖਟੇ[k];
    }

    io_cont->ਅੰਕ_ਚੌਖਟੇ = ਅੰਕ_ਚੌਖਟੇ;

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Continuation (M3_Snapshot_ਲੱਦੋ* l, u32 i_id)
{
    M3_ਨਤੀਜਾ        ਨਤੀਜਾ = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    ਜੇ (l->checking) {
        scratch          = *cont;
        scratch.val_ਢੇਰ = l->check_ਢੇਰ;
        ਜੇ (i_id == 0) {
            scratch.val_ਢੇਰ = (m3slot_t*)l->runtime->origin_ਢੇਰ;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_ਸੂਚਕਾਂਕ = 0, ਇੰਦਰਾਜ_ਸੂਚਕਾਂਕ = 0, bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = 0;
    u8  state = 0, is_ਜੜ੍ਹ = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_ਜੜ੍ਹ));
_   (GetLEB_u32(l, &type_ਸੂਚਕਾਂਕ));
_   (GetLEB_u32(l, &ਇੰਦਰਾਜ_ਸੂਚਕਾਂਕ));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_ਜੜ੍ਹ != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_ਜੜ੍ਹ and state != snapshot_contSuspended);
    ਜੇ (not is_ਜੜ੍ਹ and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_ਸੂਚਕਾਂਕ != d_m3SnapshotNone and type_ਸੂਚਕਾਂਕ >= module->ਅੰਕ_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, ਇੰਦਰਾਜ_ਸੂਚਕਾਂਕ >= module->ਅੰਕ_Functions);
    _throwif(m3Err_wasmMalformed, is_ਜੜ੍ਹ ? type_ਸੂਚਕਾਂਕ != d_m3SnapshotNone : (type_ਸੂਚਕਾਂਕ == d_m3SnapshotNone or not module->funcTypes[type_ਸੂਚਕਾਂਕ]->isContinuation));

    ਜੇ (not l->checking and not is_ਜੜ੍ਹ and state != snapshot_cont_ਮੁਕੰਮਲ) {
_       (Continuation_Acquire_ਢੇਰ(l->runtime, cont));
    }
    cont->type          = (type_ਸੂਚਕਾਂਕ != d_m3SnapshotNone) ? module->funcTypes[type_ਸੂਚਕਾਂਕ] : ਨਿਰਾ;
    cont->ਇੰਦਰਾਜ_Function = &module->functions[ਇੰਦਰਾਜ_ਸੂਚਕਾਂਕ];
    cont->ਅੰਕ_Handlers   = 0;
    cont->handlersPC    = ਨਿਰਾ;
    cont->ਮਾਪਾ        = ਨਿਰਾ;
    cont->ਅੰਕ_ਚੌਖਟੇ     = 0;

#  ਜੇ d_m3HasExceptionHandling
    {
        ਖਾਲੀ* ref = ਨਿਰਾ;
_       (Get_Ref_ਮੁੱਲ(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_ਜੜ੍ਹ or state == snapshot_cont_ਮੁਕੰਮਲ) and cont->resumeThrow != ਨਿਰਾ);
#  ਨਹੀਂਤਾਂ
    {
        ਖਾਲੀ* ref = ਨਿਰਾ;
_       (Get_Ref_ਮੁੱਲ(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ਨਿਰਾ);
    }
#  endif

    ਜੇ (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         ਅੰਕ_Rets;

_       (GetLEB_u32(l, &bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ));
        cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ;
        inner                = cont->ਇੰਦਰਾਜ_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ > Get_Func_Type_ਅੰਕ_Params(inner));

        {
            IM3FuncType ਬਚਿਆ = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not ਬਚਿਆ or
                                            Get_Func_Type_ਅੰਕ_Params(ਬਚਿਆ) != Get_Func_Type_ਅੰਕ_Params(inner) - bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ or
                                            Get_Func_Type_ਅੰਕ_Results(ਬਚਿਆ) != Get_Func_Type_ਅੰਕ_Results(inner));
            ਲਈ (u16 i = 0; i < Get_Func_Type_ਅੰਕ_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ਨਤੀਜਾ_Type(inner, i) != Get_Func_Type_ਨਤੀਜਾ_Type(ਬਚਿਆ, i));
            }
            ਲਈ (u16 i = 0; i < Get_Func_Type_ਅੰਕ_Params(ਬਚਿਆ); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ + i)) != GetFuncTypeParamType(ਬਚਿਆ, i));
            }
        }

        ਅੰਕ_Rets = Get_Func_Type_ਅੰਕ_Results(inner);

        _throwif(m3Err_wasmMalformed, (ਅੰਕ_Rets + bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ) * c_io_ਥਾਂ_ਗਿਣਤੀ > cont->ਅੰਕ_ਢੇਰ_Slots);

        ਲਈ (u32 i = 0; i < bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ; ++i) {
_           (Get_ਮੁੱਲ(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ਢੇਰ + (ਅੰਕ_Rets + i) * c_io_ਥਾਂ_ਗਿਣਤੀ));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ਢੇਰ;
        cont->pc    = cont->ਇੰਦਰਾਜ_Function->compiled;
    } ਨਹੀਂਤਾਂ ਜੇ (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ));
        _throwif(m3Err_wasmMalformed, is_ਜੜ੍ਹ and bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ != 0);
        cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ;

_       (ਲੱਦੋ_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } ਨਹੀਂਤਾਂ ਜੇ (state == snapshot_cont_ਮੁਕੰਮਲ) {
_       (GetLEB_u32(l, &bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ));
        _throwif(m3Err_wasmMalformed, bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ != 0);
        cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ = 0;
        cont->state          = cont_consumed;
    } ਨਹੀਂਤਾਂ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ਜੇ (ਨਤੀਜਾ) {
        cont->ਅੰਕ_ਚੌਖਟੇ = 0;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Continuations (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u32      ਗਿਣਤੀ_2  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &ਗਿਣਤੀ_2));
    _throwif(m3Err_wasmMalformed, ਗਿਣਤੀ_2 == 0 or ਗਿਣਤੀ_2 != l->ਅੰਕ_Continuations);

    ਲਈ (u32 i = 0; i < ਗਿਣਤੀ_2; ++i) {
_       (ਲੱਦੋ_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ਸਥਿਰ
M3_ਨਤੀਜਾ CheckContinuationReferences (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਲਈ (u32 id = 0; id < l->ਅੰਕ_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             ਪੱਤੀ  = id;
        u32             ਡੂੰਘਾਈ = 0;
#  ਜੇ d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_ਦਾ(Ref_Type_ਦਾ_Func_Type(cont->type, true), expectedType)));
#  endif
        ਜੇ (l->suspendSites[id].ਨਿਸ਼ਾਨਾ) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_ਦਾ_Ref(l->module, l->suspendSites[id].ਬਿੰਦੂ->resumeType);
            IM3Continuation ਬੱਚਾ    = l->continuations[l->suspendSites[id].ਨਿਸ਼ਾਨਾ];
            IM3FuncType     actual   = ਬੱਚਾ->ਇੰਦਰਾਜ_Function ? ਬੱਚਾ->ਇੰਦਰਾਜ_Function->funcType : ਨਿਰਾ;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ਅੰਕ_Results(actual) != Get_Func_Type_ਅੰਕ_Results(expected));
            ਲਈ (u16 i = 0; i < Get_Func_Type_ਅੰਕ_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ਨਤੀਜਾ_Type(actual, i) != Get_Func_Type_ਨਤੀਜਾ_Type(expected, i));
            }
        }
        ਜਦੋਂ (l->suspendSites[ਪੱਤੀ].ਨਿਸ਼ਾਨਾ) {
            ਪੱਤੀ = l->suspendSites[ਪੱਤੀ].ਨਿਸ਼ਾਨਾ;
            _throwif(m3Err_wasmMalformed, ++ਡੂੰਘਾਈ >= l->ਅੰਕ_Continuations or
                                            l->continuations[ਪੱਤੀ]->state != cont_suspended);
        }
        ਜੇ (cont->state != cont_suspended or id == 0) {
            ਜਾਰੀ;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        ਜੇ (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ != 0);
            ਜਾਰੀ;
        }

        ਅਚਲ M3SnapshotSuspendSite* site      = &l->suspendSites[ਪੱਤੀ];
        IM3Continuation              inner     = l->continuations[ਪੱਤੀ];
        IM3FuncType                  ਬਚਿਆ = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->ਬਿੰਦੂ or not ਬਚਿਆ);
        _throwif("a captured continuation has the wrong binding count", cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ > inner->ਅੰਕ_Suspend_Results or
                                                                          Get_Func_Type_ਅੰਕ_Params(ਬਚਿਆ) != inner->ਅੰਕ_Suspend_Results - cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_ਅੰਕ_Results(ਬਚਿਆ) != Get_Func_Type_ਅੰਕ_Results(cont->ਇੰਦਰਾਜ_Function->funcType));
        ਲਈ (u16 i = 0; i < Get_Func_Type_ਅੰਕ_Results(ਬਚਿਆ); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ਨਤੀਜਾ_Type(ਬਚਿਆ, i) !=
                                            Get_Func_Type_ਨਤੀਜਾ_Type(cont->ਇੰਦਰਾਜ_Function->funcType, i));
        }

        ਲਈ (u32 i = 0; i < inner->ਅੰਕ_Suspend_Results; ++i) {
            ਅਚਲ M3_ਥਾਂ_ਮੁੱਲ* ਮੁੱਲ = &site->map->ਮੁੱਲਾਂ[site->ਬਿੰਦੂ->ਪਹਿਲਾ_ਮੁੱਲ +
                                                          site->ਬਿੰਦੂ->ਅੰਕ_ਮੁੱਲਾਂ - site->ਬਿੰਦੂ->ਅੰਕ_Results + i];
            ਜੇ (i < cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ) {
                ਜੇ (IsRefType(ਮੁੱਲ->type) and not IsNullableRef(ਮੁੱਲ->type)) {
                    ਖਾਲੀ* ref = ਨਿਰਾ;
                    ਸਮ੍ਰਿਤੀ_ਨਕਲ(&ref, inner->sp + ਮੁੱਲ->ਥਾਂ, ਆਕਾਰ(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } ਨਹੀਂਤਾਂ {
                _throwif("a captured continuation has the wrong parameter type", ਮੁੱਲ->type !=
                                                                                   GetFuncTypeParamType(ਬਚਿਆ, (u16)(i - cont->bound_ਦਲੀਲਾਂ_ਗਿਣਤੀ)));
            }
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

#  ਜੇ d_m3HasExceptionHandling

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Exceptions (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ        = m3Err_none;
    u32      ਅੰਕ_Exceptions = 0;

_   (GetLEB_u32(l, &ਅੰਕ_Exceptions));
    _throwif(m3Err_wasmMalformed, ਅੰਕ_Exceptions == 0 or ਅੰਕ_Exceptions != l->ਅੰਕ_Exceptions);

    ਲਈ (u32 i = 0; i < ਅੰਕ_Exceptions; ++i) {
        u32          exnId = 0, ਅੰਕ_ਦਲੀਲਾਂ = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &ਅੰਕ_ਦਲੀਲਾਂ));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->ਟੈਗ->type;

        _throwif(m3Err_wasmMalformed, ਅੰਕ_ਦਲੀਲਾਂ != exn->ਅੰਕ_ਦਲੀਲਾਂ);

        ਲਈ (u32 a = 0; a < ਅੰਕ_ਦਲੀਲਾਂ; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      ਬਿੱਟ    = 0;
_           (Get_ਮੁੱਲ_ਬਿੱਟ(l, argType, &ਬਿੱਟ));
            ਜੇ (not l->checking) {
                exn->ਦਲੀਲਾਂ[a] = ਬਿੱਟ;
            }
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3BoundedReader {
    M3_Snapshot_ਲੱਦੋ* ਲੱਦੋ;
    u64             ਬਚਿਆ;
    M3_ਨਤੀਜਾ        error;
} M3BoundedReader;

ਸਥਿਰ
M3_ਨਤੀਜਾ BoundedReader_Read (ਖਾਲੀ* o_ਬਫਰ, size_t i_ਮਾਪ, ਖਾਲੀ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    ਜੇ (i_ਮਾਪ > br->ਬਚਿਆ) {
        br->error = "the host read past the end of its state";
        ਵਾਪਸ br->error;
    }

    br->ਬਚਿਆ -= i_ਮਾਪ;

    ਵਾਪਸ Get(br->ਲੱਦੋ, o_ਬਫਰ, i_ਮਾਪ);
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Host_State (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ               ਨਤੀਜਾ = m3Err_none;
    ਅਚਲ M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 ਮਾਪ = (size_t)(l->ਸਿਰਾ - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->ਲੱਦੋ_Host_State_2);

    ਜੇ (l->checking) {
        ਵਾਪਸ Skip_ਬਾਈਟਾਂ(l, ਮਾਪ);
    }
    reader.ਲੱਦੋ      = l;
    reader.ਬਚਿਆ = ਮਾਪ;
    reader.error     = m3Err_none;

_   (hooks->ਲੱਦੋ_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, ਮਾਪ));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.ਬਚਿਆ != 0);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_ਸਿਰਲੇਖ (M3_Snapshot_ਲੱਦੋ* l)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u8       magic[ਆਕਾਰ(c_snapshotMagic)];
    u32      ਸੰਸਕਰਣ = 0;

_   (Get(l, magic, ਆਕਾਰ(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, ਆਕਾਰ(magic)) != 0);

_   (GetLE32(l, &ਸੰਸਕਰਣ));
    _throwif(m3Err_wasmMalformed, ਸੰਸਕਰਣ != c_snapshot_ਸੰਸਕਰਣ);

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Meta (M3_Snapshot_ਲੱਦੋ* l, u32* o_flags)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ = 0, ਸਾਰਣੀ_Elements = 0;
    u32      ਸਰਗਰਮ_Stacks = 0;
    u64      current_ਸਮ੍ਰਿਤੀ, currentTables;
    SnapshotResourceTotals(l->module, &current_ਸਮ੍ਰਿਤੀ, &currentTables);

_   (GetLEB_u32(l, o_flags));
_   (GetLEB_u64(l, &timestamp));
_   (GetLEB_u64(l, &moduleHash));

    (ਖਾਲੀ)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->ਅੰਕ_Continuations));
_   (GetLEB_u32(l, &l->ਅੰਕ_Exceptions));
_   (GetLEB_u64(l, &ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ));
_   (GetLEB_u64(l, &ਸਾਰਣੀ_Elements));
_   (GetLEB_u32(l, &ਸਰਗਰਮ_Stacks));
    l->requiredStacks        = ਸਰਗਰਮ_Stacks;
    l->required_ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ   = ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ;
    l->required_ਸਾਰਣੀ_Elements = ਸਾਰਣੀ_Elements;
    _throwif(m3Err_wasmMalformed, ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ < current_ਸਮ੍ਰਿਤੀ or ਸਾਰਣੀ_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->ਅੰਕ_Memories and ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ != 0);
    _throwif(m3Err_wasmMalformed, not l->module->ਅੰਕ_Tables and ਸਾਰਣੀ_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    ਜੇ (*o_flags & d_m3SnapshotFlagPostmortem) {
        ਲਈ (u32 i = 0; i < l->ਅੰਕ_Exceptions; ++i) {
            u32 ਟੈਗ_ਸੂਚਕਾਂਕ = 0;
_           (GetLEB_u32(l, &ਟੈਗ_ਸੂਚਕਾਂਕ));
        }
        ਵਾਪਸ m3Err_none;
    }

    _throwif(m3_Err_ਸਮ੍ਰਿਤੀ_ਹੱਦਬੰਦੀ_Exceeded, l->runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ and
                                          ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ - current_ਸਮ੍ਰਿਤੀ > l->runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਹੱਦਬੰਦੀ - l->runtime->ਸਮ੍ਰਿਤੀ_ਬਾਈਟਾਂ_ਵਰਤਿਆ);
    _throwif(m3_Err_ਸਾਰਣੀ_ਹੱਦਬੰਦੀ_Exceeded, l->runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ and
                                         ਸਾਰਣੀ_Elements - currentTables > l->runtime->ਸਾਰਣੀ_Elements_ਹੱਦਬੰਦੀ - l->runtime->ਸਾਰਣੀ_Elements_ਵਰਤਿਆ);
    _throwif(m3_Err_continuation_ਹੱਦਬੰਦੀ_Exceeded, l->runtime->continuations_ਹੱਦਬੰਦੀ and
                                                ਸਰਗਰਮ_Stacks > l->runtime->continuations_ਹੱਦਬੰਦੀ - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->ਅੰਕ_Continuations == 0 or ਸਰਗਰਮ_Stacks >= l->ਅੰਕ_Continuations);
#  ਜੇ !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->ਅੰਕ_Exceptions != 0);
#  endif

    ਜੇ (l->checking) {
        ਲਈ (u32 i = 0; i < l->ਅੰਕ_Exceptions; ++i) {
            u32 ਟੈਗ_ਸੂਚਕਾਂਕ = 0;
_           (GetLEB_u32(l, &ਟੈਗ_ਸੂਚਕਾਂਕ));
#  ਜੇ d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, ਟੈਗ_ਸੂਚਕਾਂਕ >= l->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);
#  endif
        }
        ਵਾਪਸ m3Err_none;
    }

    l->check_ਢੇਰ = m3_AllocArray(m3slot_t, d_m3_Continuation_ਢੇਰ_Slots + 4);
    _throwifnull(l->check_ਢੇਰ);
    l->ਚੌਖਟੇ = m3_AllocArray(M3_ਚੌਖਟਾ, d_m3_Continuation_Max_ਚੌਖਟੇ);
    _throwifnull(l->ਚੌਖਟੇ);

    l->continuations = m3_AllocArray(IM3Continuation, l->ਅੰਕ_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->ਅੰਕ_Continuations);
    _throwifnull(l->suspendSites);

    ਜੇ (not l->runtime->ਜੜ੍ਹ_Continuation) {
        IM3Continuation ਜੜ੍ਹ = Continuation_New(l->runtime, ਨਿਰਾ, ਨਿਰਾ);
        _throwifnull(ਜੜ੍ਹ);

        // the root borrows the runtime's stack, so it holds none of its own
        ਜੜ੍ਹ->ਅੰਕ_ਢੇਰ_Slots          = l->runtime->ਅੰਕ_ਢੇਰ_Slots;
        l->runtime->ਜੜ੍ਹ_Continuation = ਜੜ੍ਹ;
    }

    l->continuations[0] = l->runtime->ਜੜ੍ਹ_Continuation;

    ਲਈ (u32 i = 1; i < l->ਅੰਕ_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ਨਿਰਾ, ਨਿਰਾ);
        _throwifnull(l->continuations[i]);
    }

#  ਜੇ d_m3HasExceptionHandling
    ਜੇ (l->ਅੰਕ_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->ਅੰਕ_Exceptions);
        _throwifnull(l->exceptions);
    }

    ਲਈ (u32 i = 0; i < l->ਅੰਕ_Exceptions; ++i) {
        u32    ਟੈਗ_ਸੂਚਕਾਂਕ = 0;
        IM3_ਟੈਗ ਟੈਗ;
        u32    ਅੰਕ_ਦਲੀਲਾਂ;

_       (GetLEB_u32(l, &ਟੈਗ_ਸੂਚਕਾਂਕ));
        _throwif(m3Err_wasmMalformed, ਟੈਗ_ਸੂਚਕਾਂਕ >= l->module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

        ਟੈਗ     = ਟੈਗ_ਦਾ_ਸੂਚਕਾਂਕ(l->module, ਟੈਗ_ਸੂਚਕਾਂਕ);
        ਅੰਕ_ਦਲੀਲਾਂ = Get_Func_Type_ਅੰਕ_Params(ਟੈਗ->type);

        l->exceptions[i] = NewException(l->runtime, ਟੈਗ, ਅੰਕ_ਦਲੀਲਾਂ);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  ਨਹੀਂਤਾਂ
    ਲਈ (u32 i = 0; i < l->ਅੰਕ_Exceptions; ++i) {
        u32 ਟੈਗ_ਸੂਚਕਾਂਕ = 0;
_       (GetLEB_u32(l, &ਟੈਗ_ਸੂਚਕਾਂਕ));
    }
#  endif

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Section (M3_Snapshot_ਲੱਦੋ* l, u8 i_sectionId)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    u32      flags  = 0;
    ਚੋਣ (i_sectionId) {
    ਹਾਲਤ d_m3SnapshotSection_Meta:
_       (ਲੱਦੋ_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        ਰੋਕੋ;

    ਹਾਲਤ d_m3_Snapshot_Section_ਸਮ੍ਰਿਤੀ:
_       (ਲੱਦੋ_Memories(l));
        ਰੋਕੋ;

    ਹਾਲਤ d_m3_Snapshot_Section_ਸਾਰਣੀ:
_       (ਲੱਦੋ_Tables(l));
        ਰੋਕੋ;

    ਹਾਲਤ d_m3SnapshotSection_Global:
_       (ਲੱਦੋ_Globals(l));
        ਰੋਕੋ;

    ਹਾਲਤ d_m3SnapshotSection_Segment:
_       (ਲੱਦੋ_Segments(l));
        ਰੋਕੋ;

#  ਜੇ d_m3HasExceptionHandling
    ਹਾਲਤ d_m3SnapshotSection_Exception:
_       (ਲੱਦੋ_Exceptions(l));
        ਰੋਕੋ;
#  endif

    ਹਾਲਤ d_m3SnapshotSection_Continuation:
_       (ਲੱਦੋ_Continuations(l));
        ਰੋਕੋ;

    ਹਾਲਤ d_m3SnapshotSection_HostState:
_       (ਲੱਦੋ_Host_State(l));
        ਰੋਕੋ;

    ਮੂਲ:
_       (Skip_ਬਾਈਟਾਂ(l, (size_t)(l->ਸਿਰਾ - l->pos)));
        ਰੋਕੋ;
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Sections (M3_Snapshot_ਲੱਦੋ* l, bool i_embedded)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ  = m3Err_none;
    u8*      payload = ਨਿਰਾ;

    ਜੇ (not i_embedded) {
_       (ਲੱਦੋ_ਸਿਰਲੇਖ(l));
    }

    ਜਦੋਂ (true) {
        u8  sectionId   = 0;
        u32 section_ਮਾਪ = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        ਜੇ (GetU8(l, &sectionId)) {
            ਰੋਕੋ;
        }

_       (GetLEB_u32(l, &section_ਮਾਪ));

        _throwif("the snapshot holds a section twice", l->ਵੇਖਿਆ_Sections[sectionId]);
        l->ਵੇਖਿਆ_Sections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_ਮਾਪ == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_ਮਾਪ);
        _throwifnull(payload);
_       (Get(l, payload, section_ਮਾਪ));
        l->pos       = payload;
        l->ਸਿਰਾ       = payload + section_ਮਾਪ;
        l->ਵਿੱਚ_Section = true;

        {
            M3_Snapshot_ਲੱਦੋ check = *l;
            check.checking       = true;
_           (ਲੱਦੋ_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.ਸਿਰਾ);
        }
        l->ਬਦਲਿਆ = l->ਬਦਲਿਆ or sectionId != d_m3SnapshotSection_Meta;
_       (ਲੱਦੋ_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->ਸਿਰਾ);
        l->ਵਿੱਚ_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->ਵੇਖਿਆ_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->ਅੰਕ_Exceptions and not l->ਵੇਖਿਆ_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->ਅੰਕ_Memories and not l->ਵੇਖਿਆ_Sections[d_m3_Snapshot_Section_ਸਮ੍ਰਿਤੀ]);
    _throwif("the snapshot has no table section",
             l->module->ਅੰਕ_Tables and not l->ਵੇਖਿਆ_Sections[d_m3_Snapshot_Section_ਸਾਰਣੀ]);
    _throwif("the snapshot has no global section",
             l->module->ਅੰਕ_Globals and not l->ਵੇਖਿਆ_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->ਅੰਕ_ਅੰਕੜੇ_Segments or l->module->ਅੰਕ_Element_Segments) and
               not l->ਵੇਖਿਆ_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#ਜੇ d_m3HasSnapshots

ਸਥਿਰ
M3_ਨਤੀਜਾ SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ਖਾਲੀ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_ਨਤੀਜਾ       ਨਤੀਜਾ = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, ਆਕਾਰ(save));

    IM3Continuation ਜੜ੍ਹ = io_runtime->ਜੜ੍ਹ_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not ਜੜ੍ਹ or ਜੜ੍ਹ->state != cont_suspended;

    IM3Function ਇੰਦਰਾਜ = save.postmortem ? io_runtime->entered : ਜੜ੍ਹ->ਇੰਦਰਾਜ_Function;

    ਜੇ (o_module) {
        *o_module = ਨਿਰਾ;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not ਇੰਦਰਾਜ);

    save.module = ਇੰਦਰਾਜ->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    ਜੇ (o_module) {
        *o_module = save.module;
    }

    ਲਈ (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->ਅਗਲਾ) {
        u32 id;
_       (ਸੂਚਕ_Ids_ਜੋੜੋ(&save.ਜੀਵਤ_Continuations, cont, &id));
    }

#  ਜੇ d_m3HasExceptionHandling
    ਲਈ (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->ਅਗਲਾ) {
        u32 id;
_       (ਸੂਚਕ_Ids_ਜੋੜੋ(&save.ਜੀਵਤ_Exceptions, exception, &id));
    }
#  endif

    ਜੇ (not save.postmortem) {
        u32 id;
_       (ਸੂਚਕ_Ids_ਜੋੜੋ(&save.continuations, ਜੜ੍ਹ, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    ਸੂਚਕ_Ids_Free(&save.ਜੀਵਤ_Continuations);
    ਸੂਚਕ_Ids_Free(&save.ਜੀਵਤ_Exceptions);
    ਸੂਚਕ_Ids_Free(&save.continuations);
    ਸੂਚਕ_Ids_Free(&save.exceptions);

    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ

M3_ਨਤੀਜਾ m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ਖਾਲੀ* i_userdata)
{
    ਜੇ (not io_runtime or not i_writer) {
        ਵਾਪਸ m3Err_mallocFailed;
    }

#ਜੇ d_m3HasSnapshots
    ਵਾਪਸ SaveSnapshot(io_runtime, i_writer, i_userdata, ਨਿਰਾ, false);
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_userdata;
    ਵਾਪਸ "snapshots are not available in this build of Wasm3";
#ਜੇ_ਅੰਤ
}


ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਦੋ_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ਖਾਲੀ* i_userdata, bool i_embedded)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#ਜੇ d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_ਲੜੀਵਾਰ);

    {
        M3_Snapshot_ਲੱਦੋ ਲੱਦੋ;
        memset(&ਲੱਦੋ, 0, ਆਕਾਰ(ਲੱਦੋ));

        ਲੱਦੋ.runtime  = io_runtime;
        ਲੱਦੋ.module   = i_module;
        ਲੱਦੋ.reader   = i_reader;
        ਲੱਦੋ.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        ਨਤੀਜਾ = ਲੱਦੋ_Sections(&ਲੱਦੋ, i_embedded);

        IM3Continuation ਜੜ੍ਹ = io_runtime->ਜੜ੍ਹ_Continuation;

        ਜੇ (ਨਤੀਜਾ) {
            ਜੇ (ਲੱਦੋ.ਬਦਲਿਆ) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                ਜੇ (ਜੜ੍ਹ) {
                    ਜੜ੍ਹ->state     = cont_returned;
                    ਜੜ੍ਹ->ਅੰਕ_ਚੌਖਟੇ = 0;
                }
            }
        } ਨਹੀਂਤਾਂ {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->ਸ਼ੁਰੂ_Function = -1;
            i_module->has_ਲੜੀਵਾਰ        = true;
        }

        Runtime_ਰੱਖੋ_Call_ਢੇਰ(io_runtime);

        m3_Free(ਲੱਦੋ.check_ਢੇਰ);
        m3_Free(ਲੱਦੋ.ਚੌਖਟੇ);
        m3_Free(ਲੱਦੋ.suspendSites);
        m3_Free(ਲੱਦੋ.continuations);
#  ਜੇ d_m3HasExceptionHandling
        m3_Free(ਲੱਦੋ.exceptions);
#  endif
    }
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#ਜੇ_ਅੰਤ

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ m3_ਲੱਦੋ_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ਖਾਲੀ* i_userdata)
{
    ਵਾਪਸ ਲੱਦੋ_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


ਖਾਲੀ m3_ਸੈੱਟ_Snapshot_Hooks (IM3Runtime io_runtime, ਅਚਲ M3SnapshotHooks* i_hooks)
{
#ਜੇ d_m3HasSnapshots
    ਜੇ (io_runtime) {
        ਜੇ (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } ਨਹੀਂਤਾਂ {
            memset(&io_runtime->snapshotHooks, 0, ਆਕਾਰ(io_runtime->snapshotHooks));
        }
    }
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)io_runtime;
    (ਖਾਲੀ)i_hooks;
#ਜੇ_ਅੰਤ
}


M3_ਨਤੀਜਾ m3_Save_Snapshot_ਮੰਜ਼ਿਲ_ਬਫਰ (IM3Runtime io_runtime, ਖਾਲੀ** o_ਬਾਈਟਾਂ, size_t* o_ਮਾਪ)
{
    ਜੇ (!io_runtime || !o_ਬਾਈਟਾਂ || !o_ਮਾਪ) {
        ਵਾਪਸ m3Err_mallocFailed;
    }
#ਜੇ d_m3HasSnapshots
    M3_ਬਫਰ_Writer bw     = { ਨਿਰਾ, 0, 0 };
    M3_ਨਤੀਜਾ       ਨਤੀਜਾ = m3_SaveSnapshot(io_runtime, ਬਫਰ_Writer_Write, &bw);
    ਜੇ (ਨਤੀਜਾ) {
        m3_Free(bw.ਬਫਰ);
        *o_ਬਾਈਟਾਂ = ਨਿਰਾ;
        *o_ਮਾਪ  = 0;
        ਵਾਪਸ ਨਤੀਜਾ;
    }
    *o_ਬਾਈਟਾਂ = bw.ਬਫਰ;
    *o_ਮਾਪ  = bw.ਮਾਪ;
    ਵਾਪਸ m3Err_none;
#ਨਹੀਂਤਾਂ
    *o_ਬਾਈਟਾਂ = ਨਿਰਾ;
    *o_ਮਾਪ  = 0;
    ਵਾਪਸ "snapshots are not available in this build of Wasm3";
#ਜੇ_ਅੰਤ
}

#ਜੇ d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ਸਥਿਰ
u8* EmitLEB_u32 (u8* o_where, u32 i_ਮੁੱਲ)
{
    ਕਰੋ {
        u8 byte = i_ਮੁੱਲ & 0x7F;
        i_ਮੁੱਲ >>= 7;
        ਜੇ (i_ਮੁੱਲ != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } ਜਦੋਂ (i_ਮੁੱਲ != 0);

    ਵਾਪਸ o_where;
}

ਸਥਿਰ
u32 ਮਾਪ_ਦਾ_LEB_u32 (u32 i_ਮੁੱਲ)
{
    u32 ਮਾਪ = 0;

    ਕਰੋ {
        i_ਮੁੱਲ >>= 7;
        ++ਮਾਪ;
    } ਜਦੋਂ (i_ਮੁੱਲ != 0);

    ਵਾਪਸ ਮਾਪ;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ਸਥਿਰ
M3_ਨਤੀਜਾ ਲੱਭੋ_Embedded_Snapshot (ਅਚਲ u8* i_ਸ਼ੁਰੂ, ਅਚਲ u8* i_ਸਿਰਾ, cstr_t i_secName,
                               ਅਚਲ u8** o_sec_ਸ਼ੁਰੂ, ਅਚਲ u8** o_sec_ਸਿਰਾ,
                               ਅਚਲ u8** o_ਅੰਕੜੇ, u32* o_ਮਾਪ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    ਬਾਈਟਾਂ_t  pos    = i_ਸ਼ੁਰੂ;
    bool     ਮਿਲਿਆ  = false;
    cstr_t   name   = ਨਿਰਾ;

    ਜੇ (o_sec_ਸ਼ੁਰੂ) {
        *o_sec_ਸ਼ੁਰੂ = ਨਿਰਾ;
    }
    ਜੇ (o_sec_ਸਿਰਾ) {
        *o_sec_ਸਿਰਾ = ਨਿਰਾ;
    }
    ਜੇ (o_ਅੰਕੜੇ) {
        *o_ਅੰਕੜੇ = ਨਿਰਾ;
    }
    ਜੇ (o_ਮਾਪ) {
        *o_ਮਾਪ = 0;
    }

    _throwif(m3Err_wasmMalformed, i_ਸਿਰਾ - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    ਜਦੋਂ (pos < i_ਸਿਰਾ) {
        ਬਾਈਟਾਂ_t sec_ਸ਼ੁਰੂ  = pos;
        u8      sectionId = *pos++;
        u32     ਮਾਪ      = 0;
_       (ReadLEB_u32(&ਮਾਪ, &pos, i_ਸਿਰਾ));
        _throwif(m3Err_wasmUnderrun, ਮਾਪ > (size_t)(i_ਸਿਰਾ - pos));
        {
            ਬਾਈਟਾਂ_t payload = pos;
            pos += ਮਾਪ;
            ਜੇ (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                ਜੇ (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", ਮਿਲਿਆ);
                    ਮਿਲਿਆ = true;
                    ਜੇ (o_sec_ਸ਼ੁਰੂ) {
                        *o_sec_ਸ਼ੁਰੂ = sec_ਸ਼ੁਰੂ;
                    }
                    ਜੇ (o_sec_ਸਿਰਾ) {
                        *o_sec_ਸਿਰਾ = pos;
                    }
                    ਜੇ (o_ਅੰਕੜੇ) {
                        *o_ਅੰਕੜੇ = payload;
                    }
                    ਜੇ (o_ਮਾਪ) {
                        *o_ਮਾਪ = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Builds the custom section name a snapshot of i_name lives under
ਸਥਿਰ
M3_ਨਤੀਜਾ SnapshotSectionName (cstr_t i_name, ਅੱਖਰ* o_secName, size_t i_ਮਾਪ)
{
    ਜੇ (i_name and *i_name) {
        ਜੇ (ਲੜੀ_ਲੰਬਾਈ(i_name) > d_m3_Max_Snapshot_Name_ਲੰਮਾਈ) {
            ਵਾਪਸ "the snapshot name is too long";
        }
        snprintf(o_secName, i_ਮਾਪ, "snapshot.%s", i_name);
    } ਨਹੀਂਤਾਂ {
        snprintf(o_secName, i_ਮਾਪ, "snapshot");
    }

    ਵਾਪਸ m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ਸਥਿਰ
M3_ਨਤੀਜਾ ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ਨਿਰਾ;

    ਲਈ (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->ਅਗਲਾ) {
        ਜੇ (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    ਵਾਪਸ ਨਤੀਜਾ;
}
#ਜੇ_ਅੰਤ

M3_ਨਤੀਜਾ m3_ਲੱਦੋ_Snapshot_From_ਬਫਰ (IM3Runtime io_runtime, IM3Module i_module, ਅਚਲ ਖਾਲੀ* i_ਬਾਈਟਾਂ, size_t i_ਮਾਪ)
{
    ਜੇ (!io_runtime || !i_module || !i_ਬਾਈਟਾਂ) {
        ਵਾਪਸ m3Err_mallocFailed;
    }
#ਜੇ d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    ਜੇ (i_ਮਾਪ >= 8 && memcmp(i_ਬਾਈਟਾਂ, "\0asm", 4) == 0) {
        ਅਚਲ u8* ਅੰਕੜੇ   = ਨਿਰਾ;
        u32       ਮਾਪ   = 0;
        M3_ਨਤੀਜਾ  ਨਤੀਜਾ = ਲੱਭੋ_Embedded_Snapshot((ਅਚਲ u8*)i_ਬਾਈਟਾਂ, (ਅਚਲ u8*)i_ਬਾਈਟਾਂ + i_ਮਾਪ,
                                                "snapshot", ਨਿਰਾ, ਨਿਰਾ, &ਅੰਕੜੇ, &ਮਾਪ);
        ਜੇ (ਨਤੀਜਾ) {
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        ਜੇ (not ਅੰਕੜੇ) {
            ਵਾਪਸ "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_ਬਫਰ_Reader br = { ਅੰਕੜੇ, ਮਾਪ, 0 };
            ਵਾਪਸ ਲੱਦੋ_Snapshot(io_runtime, i_module, ਬਫਰ_Reader_Read, &br, true);
        }
    }
    {
        M3_ਬਫਰ_Reader br = { (ਅਚਲ u8*)i_ਬਾਈਟਾਂ, i_ਮਾਪ, 0 };
        ਵਾਪਸ ਲੱਦੋ_Snapshot(io_runtime, i_module, ਬਫਰ_Reader_Read, &br,
                            i_ਮਾਪ < 4 or memcmp(i_ਬਾਈਟਾਂ, c_snapshotMagic, 4) != 0);
    }
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_ਮਾਪ;
    ਵਾਪਸ "snapshots are not available in this build of Wasm3";
#ਜੇ_ਅੰਤ
}

bool m3_HasSnapshot (IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name)
{
    ਜੇ (not i_module) {
        ਵਾਪਸ false;
    }
#ਜੇ d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ਨਿਰਾ;
    (ਖਾਲੀ)ModuleSnapshot(i_module, i_name, &snap);

    ਵਾਪਸ snap != ਨਿਰਾ;
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_name;
    ਵਾਪਸ false;
#ਜੇ_ਅੰਤ
}

M3_ਨਤੀਜਾ m3_GetEmbeddedSnapshot (IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name,
                                 ਅਚਲ ਖਾਲੀ** o_ਬਾਈਟਾਂ, size_t* o_ਮਾਪ)
{
    ਜੇ (not i_module or not o_ਬਾਈਟਾਂ or not o_ਮਾਪ) {
        ਵਾਪਸ m3Err_mallocFailed;
    }

    *o_ਬਾਈਟਾਂ = ਨਿਰਾ;
    *o_ਮਾਪ  = 0;

#ਜੇ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ਨਿਰਾ;
        M3_ਨਤੀਜਾ            ਨਤੀਜਾ = ModuleSnapshot(i_module, i_name, &snap);

        ਜੇ (ਨਤੀਜਾ) {
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        ਜੇ (not snap) {
            ਵਾਪਸ "the module holds no embedded snapshot of that name";
        }

        *o_ਬਾਈਟਾਂ = snap->ਅੰਕੜੇ;
        *o_ਮਾਪ  = snap->ਮਾਪ;

        ਵਾਪਸ m3Err_none;
    }
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_name;
    ਵਾਪਸ "snapshots are not available in this build of Wasm3";
#ਜੇ_ਅੰਤ
}

M3_ਨਤੀਜਾ m3_ਲੱਦੋ_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name)
{
    ਜੇ (not io_runtime or not i_module) {
        ਵਾਪਸ m3Err_mallocFailed;
    }
#ਜੇ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ਨਿਰਾ;
        M3_ਨਤੀਜਾ            ਨਤੀਜਾ = ModuleSnapshot(i_module, i_name, &snap);
        M3_ਬਫਰ_Reader      br;

        ਜੇ (ਨਤੀਜਾ) {
            ਵਾਪਸ ਨਤੀਜਾ;
        }
        ਜੇ (not snap) {
            ਵਾਪਸ "the module holds no embedded snapshot of that name";
        }

        br.ਬਫਰ = snap->ਅੰਕੜੇ;
        br.ਮਾਪ   = snap->ਮਾਪ;
        br.cursor = 0;

        ਵਾਪਸ ਲੱਦੋ_Snapshot(io_runtime, i_module, ਬਫਰ_Reader_Read, &br, true);
    }
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)i_name;
    ਵਾਪਸ "snapshots are not available in this build of Wasm3";
#ਜੇ_ਅੰਤ
}


M3_ਨਤੀਜਾ m3_Save_Snapshot_ਮੰਜ਼ਿਲ_Module (IM3Runtime io_runtime, IM3Module i_module, ਅਚਲ ਅੱਖਰ* i_name,
                                  ਖਾਲੀ** o_ਬਾਈਟਾਂ, size_t* o_ਮਾਪ)
{
#ਜੇ d_m3HasSnapshots
    M3_ਨਤੀਜਾ       ਨਤੀਜਾ = m3Err_none;
    M3_ਬਫਰ_Writer bw     = { ਨਿਰਾ, 0, 0 };
    u8*            ਬਾਹਰਲਾ    = ਨਿਰਾ;

    ਅੱਖਰ      secName[d_m3_Max_Snapshot_Name_ਲੰਮਾਈ + ਆਕਾਰ("snapshot.")];
    u32       nameLen, name_Len_Leb_ਮਾਪ, payload_ਮਾਪ, payload_ਮਾਪ_Leb_ਮਾਪ;
    size_t    orig_ਮਾਪ, ਕੁੱਲ_ਮਾਪ;
    ਅਚਲ u8* drop_ਸ਼ੁਰੂ = ਨਿਰਾ;
    ਅਚਲ u8* drop_ਸਿਰਾ   = ਨਿਰਾ;
    IM3Module saved     = ਨਿਰਾ;
    u8*       p;

    ਜੇ (not io_runtime or not i_module or not o_ਬਾਈਟਾਂ or not o_ਮਾਪ) {
        ਵਾਪਸ m3Err_mallocFailed;
    }

    *o_ਬਾਈਟਾਂ = ਨਿਰਾ;
    *o_ਮਾਪ  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_ਸ਼ੁਰੂ or i_module->wasm_ਸਿਰਾ <= i_module->wasm_ਸ਼ੁਰੂ);

_   (SnapshotSectionName(i_name, secName, ਆਕਾਰ(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->ਜੜ੍ਹ_Continuation or io_runtime->ਜੜ੍ਹ_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, ਬਫਰ_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.ਮਾਪ);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.ਮਾਪ > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (ਲੱਭੋ_Embedded_Snapshot(i_module->wasm_ਸ਼ੁਰੂ, i_module->wasm_ਸਿਰਾ, secName, &drop_ਸ਼ੁਰੂ, &drop_ਸਿਰਾ, ਨਿਰਾ, ਨਿਰਾ));

    orig_ਮਾਪ           = (size_t)(i_module->wasm_ਸਿਰਾ - i_module->wasm_ਸ਼ੁਰੂ) - (drop_ਸ਼ੁਰੂ ? (size_t)(drop_ਸਿਰਾ - drop_ਸ਼ੁਰੂ) : 0);
    nameLen            = (u32)ਲੜੀ_ਲੰਬਾਈ(secName);
    name_Len_Leb_ਮਾਪ     = ਮਾਪ_ਦਾ_LEB_u32(nameLen);
    payload_ਮਾਪ        = name_Len_Leb_ਮਾਪ + nameLen + (u32)bw.ਮਾਪ;
    payload_ਮਾਪ_Leb_ਮਾਪ = ਮਾਪ_ਦਾ_LEB_u32(payload_ਮਾਪ);
    ਕੁੱਲ_ਮਾਪ          = orig_ਮਾਪ + 1 + payload_ਮਾਪ_Leb_ਮਾਪ + payload_ਮਾਪ;

    ਬਾਹਰਲਾ = (u8*)m3_Malloc("wasm_snapshot_module", ਕੁੱਲ_ਮਾਪ);
    _throwifnull(ਬਾਹਰਲਾ);

    p = ਬਾਹਰਲਾ;

    ਜੇ (drop_ਸ਼ੁਰੂ) {
        size_t before = (size_t)(drop_ਸ਼ੁਰੂ - i_module->wasm_ਸ਼ੁਰੂ);
        size_t after  = (size_t)(i_module->wasm_ਸਿਰਾ - drop_ਸਿਰਾ);

        ਸਮ੍ਰਿਤੀ_ਨਕਲ(p, i_module->wasm_ਸ਼ੁਰੂ, before);
        p += before;
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(p, drop_ਸਿਰਾ, after);
        p += after;
    } ਨਹੀਂਤਾਂ {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(p, i_module->wasm_ਸ਼ੁਰੂ, orig_ਮਾਪ);
        p += orig_ਮਾਪ;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_ਮਾਪ);
    p    = EmitLEB_u32(p, nameLen);
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(p, secName, nameLen);
    p += nameLen;
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(p, bw.ਬਫਰ, bw.ਮਾਪ);

    *o_ਬਾਈਟਾਂ = ਬਾਹਰਲਾ;
    *o_ਮਾਪ  = ਕੁੱਲ_ਮਾਪ;
    ਬਾਹਰਲਾ      = ਨਿਰਾ;

_catch:
    m3_Free(bw.ਬਫਰ);
    m3_Free(ਬਾਹਰਲਾ);

    ਵਾਪਸ ਨਤੀਜਾ;
#ਨਹੀਂਤਾਂ
    (ਖਾਲੀ)io_runtime;
    (ਖਾਲੀ)i_module;
    (ਖਾਲੀ)i_name;
    (ਖਾਲੀ)o_ਬਾਈਟਾਂ;
    (ਖਾਲੀ)o_ਮਾਪ;
    ਵਾਪਸ "snapshots are not available in this build of Wasm3";
#ਜੇ_ਅੰਤ
}
