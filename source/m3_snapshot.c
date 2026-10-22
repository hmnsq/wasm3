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

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_host.h"

#ಅನ್ತರ್ಭಾವ <limits.h>

#ಆದರೆ d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u32 c_snapshot_ಆವೃತ್ತಿ  = 1;

ಎಣಿಕೆ {
    d_m3SnapshotSection_Meta         = 0,
    d_m3_Snapshot_Section_ಸ್ಮರಣೆ       = 1,
    d_m3_Snapshot_Section_ಕೋಷ್ಟಕ        = 2,
    d_m3SnapshotSection_Global       = 3,
    d_m3SnapshotSection_Segment      = 4,
    d_m3SnapshotSection_Exception    = 5,
    d_m3SnapshotSection_Continuation = 6,
    d_m3SnapshotSection_HostState    = 7,
};

#  define d_m3SnapshotFlagPostmortem  0x1

// The longest <name> in a "snapshot.<name>" custom section. A name is a label
// the embedder picks, not something the format needs room to grow.
#  define d_m3_Max_Snapshot_Name_ಉದ್ದಳತೆ   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
ಎಣಿಕೆ {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_ಪೂರ್ಣಗೊಂಡ  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_ಕೊನೆ                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//ತಟಸ್ಥ
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_ಆರಂಭ or i_module->wasm_ಕೊನೆ - i_module->wasm_ಆರಂಭ < 8);

    {
        ಬೈಟುಗಳು_t  pos = i_module->wasm_ಆರಂಭ + 8;
        cbytes_t ಕೊನೆ = i_module->wasm_ಕೊನೆ;

        Xxh64_Init(&hash, 0);

        ಆಗ (pos < ಕೊನೆ) {
            ಬೈಟುಗಳು_t ಆರಂಭ     = pos;
            u8      sectionId = *pos++;
            u32     ಗಾತ್ರ_2      = 0;

_           (ReadLEB_u32(&ಗಾತ್ರ_2, &pos, ಕೊನೆ));
            _throwif(m3Err_wasmUnderrun, ಗಾತ್ರ_2 > (size_t)(ಕೊನೆ - pos));

            pos += ಗಾತ್ರ_2;
            ಆದರೆ (sectionId != 0) {
                Xxh64_Update(&hash, ಆರಂಭ, (size_t)(pos - ಆರಂಭ));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* ಕಂಡುಹಿಡಿ_ಸುರಕ್ಷಿತ_ಬಿಂದು (IM3Function i_function, pc_t i_pc, u8 i_ಬಗೆ)
{
    ಸ್ಥಿರಾಂಕ M3SnapshotMap* map = i_function->snapshotMap;

    ಆದರೆ (map) {
        ಗಾಗಿ (u32 i = 0; i < map->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points; ++i) {
            ಆದರೆ (map->ಸುರಕ್ಷಿತ_Points[i].pc == i_pc and map->ಸುರಕ್ಷಿತ_Points[i].ಬಗೆ == i_ಬಗೆ) {
                ಹಿಂತಿರುಗಿ &map->ಸುರಕ್ಷಿತ_Points[i];
            }
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* ಕಂಡುಹಿಡಿ_ಸುರಕ್ಷಿತ_ಬಿಂದು_At (ಸ್ಥಿರಾಂಕ M3SnapshotMap* i_map, u32 i_wasm_ಸ್ಥಾನಾಂತರ, u8 i_ಬಗೆ, u32 i_ಗುರಿ_Loop)
{
    u32 ಕೆಳಗಿನ  = 0;
    u32 ಮೇಲಿನ = i_map->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points;

    ಆಗ (ಕೆಳಗಿನ < ಮೇಲಿನ) {
        u32 ಮಧ್ಯ = ಕೆಳಗಿನ + (ಮೇಲಿನ - ಕೆಳಗಿನ) / 2;

        ಆದರೆ (i_map->ಸುರಕ್ಷಿತ_Points[ಮಧ್ಯ].wasm_ಸ್ಥಾನಾಂತರ < i_wasm_ಸ್ಥಾನಾಂತರ) {
            ಕೆಳಗಿನ = ಮಧ್ಯ + 1;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಮೇಲಿನ = ಮಧ್ಯ;
        }
    }

    ಗಾಗಿ (u32 i = ಕೆಳಗಿನ; i < i_map->ಅಂಕೆ_ಸುರಕ್ಷಿತ_Points and i_map->ಸುರಕ್ಷಿತ_Points[i].wasm_ಸ್ಥಾನಾಂತರ == i_wasm_ಸ್ಥಾನಾಂತರ; ++i) {
        ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* ಬಿಂದು = &i_map->ಸುರಕ್ಷಿತ_Points[i];

        ಆದರೆ (ಬಿಂದು->ಬಗೆ != i_ಬಗೆ or (ಬಿಂದು->flags & d_m3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಬಾಲ_Call)) {
            ಮುಂದುವರಿ;
        }
        ಆದರೆ (i_ಬಗೆ != safepoint_op or i_map->ಖಂಡಗಳು[ಬಿಂದು->aux].wasm_ಸ್ಥಾನಾಂತರ == i_ಗುರಿ_Loop) {
            ಹಿಂತಿರುಗಿ ಬಿಂದು;
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ M3_ಖಂಡ_ಆರಂಭ* ಕಂಡುಹಿಡಿ_ಖಂಡ_At (ಸ್ಥಿರಾಂಕ M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_ಸ್ಥಾನಾಂತರ)
{
    ಗಾಗಿ (u32 i = 0; i < i_map->ಅಂಕೆ_ಖಂಡಗಳು; ++i) {
        ಆದರೆ (i_map->ಖಂಡಗಳು[i].opcode == i_opcode and i_map->ಖಂಡಗಳು[i].wasm_ಸ್ಥಾನಾಂತರ == i_wasm_ಸ್ಥಾನಾಂತರ) {
            ಹಿಂತಿರುಗಿ &i_map->ಖಂಡಗಳು[i];
        }
    }

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Encloses_ಸ್ಥಾನಾಂತರ (ಸ್ಥಿರಾಂಕ M3_ಖಂಡ_ಆರಂಭ* i_ಖಂಡ, u32 i_wasm_ಸ್ಥಾನಾಂತರ)
{
    ಹಿಂತಿರುಗಿ i_ಖಂಡ->wasm_ಸ್ಥಾನಾಂತರ < i_wasm_ಸ್ಥಾನಾಂತರ and i_wasm_ಸ್ಥಾನಾಂತರ < i_ಖಂಡ->wasm_ಕೊನೆ;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ಸ್ಥಿರ ಸಾಲೊಳಗೆ
bool Handlers_ಜೀವಂತ_At (ಸ್ಥಿರಾಂಕ M3SnapshotMap* i_map, ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* i_ಬಿಂದು, ಸ್ಥಿರಾಂಕ M3_ಖಂಡ_ಆರಂಭ* i_try)
{
    ಹಿಂತಿರುಗಿ not (i_ಬಿಂದು->ಬಗೆ == safepoint_op and i_try->wasm_ಸ್ಥಾನಾಂತರ > i_map->ಖಂಡಗಳು[i_ಬಿಂದು->aux].wasm_ಸ್ಥಾನಾಂತರ);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ CallSiteType (IM3Function i_caller, ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* i_ಬಿಂದು, IM3FuncType* o_type)
{
    M3_ಫಲಿತಾಂಶ   ಫಲಿತಾಂಶ = m3Err_none;
    IM3Module  module = i_caller->module;
    ಬೈಟುಗಳು_t    pos    = i_caller->wasm;
    cbytes_t   ಕೊನೆ    = i_caller->wasm_ಕೊನೆ;
    u32        ಗಾತ್ರ_2   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ಖಾಲಿ;

_   (ReadLEB_u32(&ಗಾತ್ರ_2, &pos, ಕೊನೆ));
    _throwif(m3Err_wasmUnderrun, i_ಬಿಂದು->wasm_ಸ್ಥಾನಾಂತರ >= (size_t)(ಕೊನೆ - pos));
    pos += i_ಬಿಂದು->wasm_ಸ್ಥಾನಾಂತರ;

_   (Read_opcode(&opcode, &pos, ಕೊನೆ));
_   (ReadLEB_u32(&index, &pos, ಕೊನೆ));

    ಆದರೆ (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->ಅಂಕೆ_Functions);
        *o_type = module->functions[index].funcType;
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_ಪರೋಕ್ಷ and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->ಅಂಕೆ_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ಸ್ಥಿರ
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 ಅಂಕೆ_Results = Get_Func_Type_ಅಂಕೆ_Results(i_expected);

    ಆದರೆ (not i_type or Get_Func_Type_ಅಂಕೆ_Results(i_type) != ಅಂಕೆ_Results) {
        ಹಿಂತಿರುಗಿ false;
    }

    ಗಾಗಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
        ಆದರೆ (not Is_Sub_Type_ದ(Get_Func_Type_ಫಲಿತಾಂಶ_Type(i_type, i), Get_Func_Type_ಫಲಿತಾಂಶ_Type(i_expected, i))) {
            ಹಿಂತಿರುಗಿ false;
        }
    }

    ಹಿಂತಿರುಗಿ true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಚೌಕಟ್ಟು_Registers {
    m3reg_t* r0;
#  ಆದರೆ d_m3HasFloat
    f64* fp0;
#  endif
} M3_ಚೌಕಟ್ಟು_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Continuation_ರಾಶಿ (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_ತಳ, u32* o_ಅಂಕೆ_Slots)
{
    ಆದರೆ (i_cont == i_runtime->ಬೇರು_Continuation) {
        *o_ತಳ     = (m3slot_t*)i_runtime->origin_ರಾಶಿ;
        *o_ಅಂಕೆ_Slots = i_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
    } ಇಲ್ಲದಿದ್ದರೆ {
        *o_ತಳ     = i_cont->val_ರಾಶಿ;
        *o_ಅಂಕೆ_Slots = i_cont->ಅಂಕೆ_ರಾಶಿ_Slots;
    }

    ಹಿಂತಿರುಗಿ (*o_ತಳ) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Locate_ಮೌಲ್ಯ (m3slot_t* i_ತಳ, u32 i_ಅಂಕೆ_Slots, m3stack_t i_sp, ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* i_ಮೌಲ್ಯ, u8** o_where)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u8       type   = ತಳ_Type_ದ(i_ಮೌಲ್ಯ->type);

    *o_where = ಖಾಲಿ;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(i_ಮೌಲ್ಯ->ಸ್ಥಾನ));

    ಆದರೆ (not Is_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(i_ಮೌಲ್ಯ->ಸ್ಥಾನ)) {
        size_t ಗಾತ್ರ_2   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? ಗಾತ್ರ(ಶೂನ್ಯ*)
                                                                   : (Is64_ಬಿಟ್_Type(type) ? ಗಾತ್ರ(u64) : ಗಾತ್ರ(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_ತಳ + i_ಮೌಲ್ಯ->ಸ್ಥಾನ) * ಗಾತ್ರ(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + ಗಾತ್ರ_2 > (size_t)i_ಅಂಕೆ_Slots * ಗಾತ್ರ(m3slot_t));

        *o_where = (u8*)i_ತಳ + offset;
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Read_ಮೌಲ್ಯ (u8* i_where, ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು_Registers* i_registers, ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* i_ಮೌಲ್ಯ, u64* o_ಬಿಟ್ಟುಗಳು,
                    ಶೂನ್ಯ** o_reference)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u8       type   = ತಳ_Type_ದ(i_ಮೌಲ್ಯ->type);

    *o_ಬಿಟ್ಟುಗಳು      = 0;
    *o_reference = ಖಾಲಿ;

    ಆದರೆ (i_where) {
        ಆದರೆ (IsRefType(type)) {
            ಸ್ಮರಣೆ_ನಕಲು(o_reference, i_where, ಗಾತ್ರ(ಶೂನ್ಯ*));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Is64_ಬಿಟ್_Type(type)) {
            ಸ್ಮರಣೆ_ನಕಲು(o_ಬಿಟ್ಟುಗಳು, i_where, ಗಾತ್ರ(u64));
        } ಇಲ್ಲದಿದ್ದರೆ {
            u32 narrow;
            ಸ್ಮರಣೆ_ನಕಲು(&narrow, i_where, ಗಾತ್ರ(narrow));
            *o_ಬಿಟ್ಟುಗಳು = narrow;
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Is_Fp_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(i_ಮೌಲ್ಯ->ಸ್ಥಾನ)) {
#  ಆದರೆ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ಆದರೆ (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 ಬಿಟ್ಟುಗಳು;
            ಸ್ಮರಣೆ_ನಕಲು(&ಬಿಟ್ಟುಗಳು, &narrow, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
            *o_ಬಿಟ್ಟುಗಳು = ಬಿಟ್ಟುಗಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            ಸ್ಮರಣೆ_ನಕಲು(o_ಬಿಟ್ಟುಗಳು, i_registers->fp0, ಗಾತ್ರ(u64));
        }
#  ಇಲ್ಲದಿದ್ದರೆ
        _throw(m3Err_wasmMalformed);
#  endif
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        ಆದರೆ (IsRefType(type)) {
            *o_reference = (ಶೂನ್ಯ*)(uintptr_t)r0;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Is64_ಬಿಟ್_Type(type)) {
            *o_ಬಿಟ್ಟುಗಳು = (u64)r0;
        } ಇಲ್ಲದಿದ್ದರೆ {
            *o_ಬಿಟ್ಟುಗಳು = (u32)r0;
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Write_ಮೌಲ್ಯ (u8* o_where, ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು_Registers* i_registers, ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* i_ಮೌಲ್ಯ, u64 i_ಬಿಟ್ಟುಗಳು,
                     ಶೂನ್ಯ* i_reference)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u8       type   = ತಳ_Type_ದ(i_ಮೌಲ್ಯ->type);

    ಆದರೆ (o_where) {
        ಆದರೆ (IsRefType(type)) {
            ಸ್ಮರಣೆ_ನಕಲು(o_where, &i_reference, ಗಾತ್ರ(ಶೂನ್ಯ*));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Is64_ಬಿಟ್_Type(type)) {
            ಸ್ಮರಣೆ_ನಕಲು(o_where, &i_ಬಿಟ್ಟುಗಳು, ಗಾತ್ರ(u64));
        } ಇಲ್ಲದಿದ್ದರೆ {
            u32 narrow = (u32)i_ಬಿಟ್ಟುಗಳು;
            ಸ್ಮರಣೆ_ನಕಲು(o_where, &narrow, ಗಾತ್ರ(narrow));
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Is_Fp_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(i_ಮೌಲ್ಯ->ಸ್ಥಾನ)) {
#  ಆದರೆ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ಆದರೆ (type == c_m3Type_f32) {
            u32 ಬಿಟ್ಟುಗಳು = (u32)i_ಬಿಟ್ಟುಗಳು;
            f32 narrow;
            ಸ್ಮರಣೆ_ನಕಲು(&narrow, &ಬಿಟ್ಟುಗಳು, ಗಾತ್ರ(narrow));
            *i_registers->fp0 = narrow;
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            ಸ್ಮರಣೆ_ನಕಲು(i_registers->fp0, &i_ಬಿಟ್ಟುಗಳು, ಗಾತ್ರ(u64));
        }
#  ಇಲ್ಲದಿದ್ದರೆ
        _throw(m3Err_wasmMalformed);
#  endif
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        ಆದರೆ (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } ಇಲ್ಲದಿದ್ದರೆ {
            *i_registers->r0 = (m3reg_t)i_ಬಿಟ್ಟುಗಳು;
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* ಚೌಕಟ್ಟು_ಮೌಲ್ಯ (ಸ್ಥಿರಾಂಕ M3SnapshotMap* i_map, ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* i_ಬಿಂದು, u32 i_ಸೂಚ್ಯಂಕ)
{
    ಆದರೆ (i_ಸೂಚ್ಯಂಕ < i_map->ಅಂಕೆ_Locals) {
        ಹಿಂತಿರುಗಿ i_ಬಿಂದು->ಮೊದಲ_Local_ಮೌಲ್ಯ == UINT32_MAX ? &i_map->locals[i_ಸೂಚ್ಯಂಕ]
                                                      : &i_map->ಮೌಲ್ಯಗಳು[i_ಬಿಂದು->ಮೊದಲ_Local_ಮೌಲ್ಯ + i_ಸೂಚ್ಯಂಕ];
    }
    ಹಿಂತಿರುಗಿ &i_map->ಮೌಲ್ಯಗಳು[i_ಬಿಂದು->ಮೊದಲ_ಮೌಲ್ಯ + i_ಸೂಚ್ಯಂಕ - i_map->ಅಂಕೆ_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ಸ್ಥಿರ
ಶೂನ್ಯ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    ಆದರೆ (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->ಅಂಕೆ_Ret_And_Arg_Slots) + i_function->ಅಂಕೆ_Local_ಬೈಟುಗಳು;

        ಸ್ಮರಣೆ_ನಕಲು(where, i_function->constants, i_function->ಅಂಕೆ_Constant_ಬೈಟುಗಳು);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಸೂಚಕ_Ids {
    ಶೂನ್ಯ** ವಸ್ತುಗಳು;
    u32    ಎಣಿಕೆ_2;
    u32    ಸಾಮರ್ಥ್ಯ;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ;       // a power of two, kept at least twice count
} M3_ಸೂಚಕ_Ids;

ಸ್ಥಿರ
u32 ಸೂಚಕ_ಪಾತ್ರೆ (ಶೂನ್ಯ* i_ಸೂಚಕ, u32 i_ಸಾಮರ್ಥ್ಯ)
{
    u64 ಬಿಟ್ಟುಗಳು = (u64)(uintptr_t)i_ಸೂಚಕ;

    ಹಿಂತಿರುಗಿ (u32)((ಬಿಟ್ಟುಗಳು * 0x9E3779B97F4A7C15ULL) >> 32) & (i_ಸಾಮರ್ಥ್ಯ - 1);
}

ಸ್ಥಿರ
u32 ಸೂಚಕ_Ids_ಕಂಡುಹಿಡಿ (ಸ್ಥಿರಾಂಕ M3_ಸೂಚಕ_Ids* i_ids, ಶೂನ್ಯ* i_ಸೂಚಕ)
{
    ಆದರೆ (i_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ) {
        u32 ಪಾತ್ರೆ = ಸೂಚಕ_ಪಾತ್ರೆ(i_ಸೂಚಕ, i_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ);

        ಆಗ (i_ids->index[ಪಾತ್ರೆ]) {
            u32 id = i_ids->index[ಪಾತ್ರೆ] - 1;

            ಆದರೆ (i_ids->ವಸ್ತುಗಳು[id] == i_ಸೂಚಕ) {
                ಹಿಂತಿರುಗಿ id;
            }

            ಪಾತ್ರೆ = (ಪಾತ್ರೆ + 1) & (i_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ - 1);
        }
    }

    ಹಿಂತಿರುಗಿ d_m3SnapshotNone;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಸೂಚಕ_Ids_ಕೂಡಿಸು (M3_ಸೂಚಕ_Ids* io_ids, ಶೂನ್ಯ* i_ಸೂಚಕ, u32* o_id)
{
    u32 id = ಸೂಚಕ_Ids_ಕಂಡುಹಿಡಿ(io_ids, i_ಸೂಚಕ);

    ಆದರೆ (id != d_m3SnapshotNone) {
        *o_id = id;
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    ಆದರೆ (io_ids->ಎಣಿಕೆ_2 == io_ids->ಸಾಮರ್ಥ್ಯ) {
        u32    ಸಾಮರ್ಥ್ಯ = io_ids->ಸಾಮರ್ಥ್ಯ ? io_ids->ಸಾಮರ್ಥ್ಯ * 2 : 16;
        ಶೂನ್ಯ** ವಸ್ತುಗಳು    = m3_ReallocArray(ಶೂನ್ಯ*, io_ids->ವಸ್ತುಗಳು, ಸಾಮರ್ಥ್ಯ, io_ids->ಸಾಮರ್ಥ್ಯ);

        ಆದರೆ (not ವಸ್ತುಗಳು) {
            ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
        }

        io_ids->ವಸ್ತುಗಳು    = ವಸ್ತುಗಳು;
        io_ids->ಸಾಮರ್ಥ್ಯ = ಸಾಮರ್ಥ್ಯ;
    }

    ಆದರೆ ((io_ids->ಎಣಿಕೆ_2 + 1) * 2 > io_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ) {
        u32  ಸಾಮರ್ಥ್ಯ = io_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ ? io_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ * 2 : 32;
        u32* index    = m3_AllocArray(u32, ಸಾಮರ್ಥ್ಯ);

        ಆದರೆ (not index) {
            ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
        }

        ಗಾಗಿ (u32 i = 0; i < io_ids->ಎಣಿಕೆ_2; ++i) {
            u32 ಪಾತ್ರೆ = ಸೂಚಕ_ಪಾತ್ರೆ(io_ids->ವಸ್ತುಗಳು[i], ಸಾಮರ್ಥ್ಯ);

            ಆಗ (index[ಪಾತ್ರೆ]) {
                ಪಾತ್ರೆ = (ಪಾತ್ರೆ + 1) & (ಸಾಮರ್ಥ್ಯ - 1);
            }

            index[ಪಾತ್ರೆ] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ = ಸಾಮರ್ಥ್ಯ;
    }

    id                = io_ids->ಎಣಿಕೆ_2++;
    io_ids->ವಸ್ತುಗಳು[id] = i_ಸೂಚಕ;

    u32 ಪಾತ್ರೆ = ಸೂಚಕ_ಪಾತ್ರೆ(i_ಸೂಚಕ, io_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ);

    ಆಗ (io_ids->index[ಪಾತ್ರೆ]) {
        ಪಾತ್ರೆ = (ಪಾತ್ರೆ + 1) & (io_ids->ಸೂಚ್ಯಂಕ_ಸಾಮರ್ಥ್ಯ - 1);
    }

    io_ids->index[ಪಾತ್ರೆ] = id + 1;

    *o_id = id;

    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ
ಶೂನ್ಯ ಸೂಚಕ_Ids_Free (M3_ಸೂಚಕ_Ids* io_ids)
{
    m3_Free(io_ids->ವಸ್ತುಗಳು);
    m3_Free(io_ids->index);
    memset(io_ids, 0, ಗಾತ್ರ(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಬಫರ್_Writer {
    u8*    ಬಫರ್;
    size_t ಗಾತ್ರ_2;
    size_t ಸಾಮರ್ಥ್ಯ;
} M3_ಬಫರ್_Writer;

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಬಫರ್_Writer_Write (ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ದತ್ತಾಂಶ, size_t i_ಗಾತ್ರ, ಶೂನ್ಯ* i_userdata)
{
    M3_ಬಫರ್_Writer* bw = (M3_ಬಫರ್_Writer*)i_userdata;

    ಆದರೆ (bw->ಗಾತ್ರ_2 + i_ಗಾತ್ರ > bw->ಸಾಮರ್ಥ್ಯ) {
        size_t newCap = bw->ಸಾಮರ್ಥ್ಯ ? bw->ಸಾಮರ್ಥ್ಯ * 2 : 4096;
        ಆಗ (bw->ಗಾತ್ರ_2 + i_ಗಾತ್ರ > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->ಬಫರ್, newCap, bw->ಸಾಮರ್ಥ್ಯ);
        ಆದರೆ (!newBuf) {
            ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
        }
        bw->ಬಫರ್   = newBuf;
        bw->ಸಾಮರ್ಥ್ಯ = newCap;
    }

    ಆದರೆ (i_ಗಾತ್ರ) {
        ಸ್ಮರಣೆ_ನಕಲು(bw->ಬಫರ್ + bw->ಗಾತ್ರ_2, i_ದತ್ತಾಂಶ, i_ಗಾತ್ರ);
        bw->ಗಾತ್ರ_2 += i_ಗಾತ್ರ;
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_ಬಫರ್_Reader {
    ಸ್ಥಿರಾಂಕ u8* ಬಫರ್;
    size_t    ಗಾತ್ರ_2;
    size_t    cursor;
} M3_ಬಫರ್_Reader;

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಬಫರ್_Reader_Read (ಶೂನ್ಯ* o_ಬಫರ್, size_t i_ಗಾತ್ರ, ಶೂನ್ಯ* i_userdata)
{
    M3_ಬಫರ್_Reader* br = (M3_ಬಫರ್_Reader*)i_userdata;

    ಆದರೆ (i_ಗಾತ್ರ > br->ಗಾತ್ರ_2 - br->cursor) {
        ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
    }

    ಆದರೆ (i_ಗಾತ್ರ) {
        ಸ್ಮರಣೆ_ನಕಲು(o_ಬಫರ್, br->ಬಫರ್ + br->cursor, i_ಗಾತ್ರ);
        br->cursor += i_ಗಾತ್ರ;
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
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
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    ಶೂನ್ಯ*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3_ಸೂಚಕ_Ids ಜೀವಂತ_Continuations;     // everything the runtime still holds, to check
    M3_ಸೂಚಕ_Ids ಜೀವಂತ_Exceptions;        //   a reference against before following it
    M3_ಸೂಚಕ_Ids continuations;         // what the snapshot carries, by id
    M3_ಸೂಚಕ_Ids exceptions;
} M3SnapshotSave;

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
u8 M3_Type_ಗಮ್ಯ_Val_Type (m3type_t i_type)
{
    ಆಯ್ಕೆ (ತಳ_Type_ದ(i_type)) {
    ಸ್ಥಿತಿ c_m3Type_i32: ಹಿಂತಿರುಗಿ 0x7F;
    ಸ್ಥಿತಿ c_m3Type_i64: ಹಿಂತಿರುಗಿ 0x7E;
    ಸ್ಥಿತಿ c_m3Type_f32: ಹಿಂತಿರುಗಿ 0x7D;
    ಸ್ಥಿತಿ c_m3Type_f64: ಹಿಂತಿರುಗಿ 0x7C;
    ಸ್ಥಿತಿ c_m3Type_v128: ಹಿಂತಿರುಗಿ 0x7B;
    ಸ್ಥಿತಿ c_m3Type_funcref: ಹಿಂತಿರುಗಿ 0x70;
    ಸ್ಥಿತಿ c_m3Type_externref: ಹಿಂತಿರುಗಿ 0x6F;
    ಸ್ಥಿತಿ c_m3Type_exnref: ಹಿಂತಿರುಗಿ 0x69;
    ಸ್ಥಿತಿ c_m3Type_contref: ಹಿಂತಿರುಗಿ 0x68;
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ 0x00;
    }
}

ಸ್ಥಿರ ಸಾಲೊಳಗೆ
m3type_t Val_Type_ಗಮ್ಯ_M3_Type (u8 i_valtype)
{
    ಆಯ್ಕೆ (i_valtype) {
    ಸ್ಥಿತಿ 0x7F: ಹಿಂತಿರುಗಿ c_m3Type_i32;
    ಸ್ಥಿತಿ 0x7E: ಹಿಂತಿರುಗಿ c_m3Type_i64;
    ಸ್ಥಿತಿ 0x7D: ಹಿಂತಿರುಗಿ c_m3Type_f32;
    ಸ್ಥಿತಿ 0x7C: ಹಿಂತಿರುಗಿ c_m3Type_f64;
    ಸ್ಥಿತಿ 0x7B: ಹಿಂತಿರುಗಿ c_m3Type_v128;
    ಸ್ಥಿತಿ 0x70: ಹಿಂತಿರುಗಿ c_m3Type_funcref;
    ಸ್ಥಿತಿ 0x6F: ಹಿಂತಿರುಗಿ c_m3Type_externref;
    ಸ್ಥಿತಿ 0x69: ಹಿಂತಿರುಗಿ c_m3Type_exnref;
    ಸ್ಥಿತಿ 0x68: ಹಿಂತಿರುಗಿ c_m3Type_contref;
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ c_m3Type_none;
    }
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Put (M3SnapshotSave* s, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ದತ್ತಾಂಶ, size_t i_ಗಾತ್ರ)
{
    ಹಿಂತಿರುಗಿ s->writing ? s->writer(i_ದತ್ತಾಂಶ, i_ಗಾತ್ರ, s->userdata) : m3Err_none;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ PutU8 (M3SnapshotSave* s, u8 i_ಮೌಲ್ಯ)
{
    ಹಿಂತಿರುಗಿ Put(s, &i_ಮೌಲ್ಯ, ಗಾತ್ರ(i_ಮೌಲ್ಯ));
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ PutLEB_u32 (M3SnapshotSave* s, u32 i_ಮೌಲ್ಯ)
{
    u8  ಬೈಟುಗಳು[5];
    u32 len = 0;

    ಮಾಡು {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಆದರೆ (i_ಮೌಲ್ಯ != 0) {
            byte |= 0x80;
        }
        ಬೈಟುಗಳು[len++] = byte;
    } ಆಗ (i_ಮೌಲ್ಯ != 0);

    ಹಿಂತಿರುಗಿ Put(s, ಬೈಟುಗಳು, len);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ PutLEB_u64 (M3SnapshotSave* s, u64 i_ಮೌಲ್ಯ)
{
    u8  ಬೈಟುಗಳು[10];
    u32 len = 0;

    ಮಾಡು {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಆದರೆ (i_ಮೌಲ್ಯ != 0) {
            byte |= 0x80;
        }
        ಬೈಟುಗಳು[len++] = byte;
    } ಆಗ (i_ಮೌಲ್ಯ != 0);

    ಹಿಂತಿರುಗಿ Put(s, ಬೈಟುಗಳು, len);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ PutLEB_i64 (M3SnapshotSave* s, i64 i_ಮೌಲ್ಯ)
{
    u8   ಬೈಟುಗಳು[10];
    u32  len  = 0;
    bool ಇನ್ನಷ್ಟು = true;

    ಆಗ (ಇನ್ನಷ್ಟು) {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಆದರೆ ((i_ಮೌಲ್ಯ == 0 && !(byte & 0x40)) || (i_ಮೌಲ್ಯ == -1 && (byte & 0x40))) {
            ಇನ್ನಷ್ಟು = false;
        } ಇಲ್ಲದಿದ್ದರೆ {
            byte |= 0x80;
        }
        ಬೈಟುಗಳು[len++] = byte;
    }

    ಹಿಂತಿರುಗಿ Put(s, ಬೈಟುಗಳು, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ M3_ಫಲಿತಾಂಶ (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_ಫಲಿತಾಂಶ         ಫಲಿತಾಂಶ   = m3Err_none;
    M3_ಬಫರ್_Writer   bw       = { ಖಾಲಿ, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    ಶೂನ್ಯ*            userdata = s->userdata;

    s->writer   = ಬಫರ್_Writer_Write;
    s->userdata = &bw;

    ಫಲಿತಾಂಶ = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    ಆದರೆ (ಫಲಿತಾಂಶ) {
        ಹೋಗು _catch;
    }

    // a section with nothing in it is left out
    ಆದರೆ (bw.ಗಾತ್ರ_2) {
        _throwif("snapshot section is too large", bw.ಗಾತ್ರ_2 > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.ಗಾತ್ರ_2));
_       (Put(s, bw.ಬಫರ್, bw.ಗಾತ್ರ_2));
    }

_catch:
    m3_Free(bw.ಬಫರ್);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
u32 Function_ಸೂಚ್ಯಂಕ (IM3Module i_module, IM3Function i_function)
{
    ಆದರೆ (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->ಅಂಕೆ_Functions) {
        ಹಿಂತಿರುಗಿ (u32)(i_function - i_module->functions);
    }

    ಹಿಂತಿರುಗಿ d_m3SnapshotNone;
}

ಸ್ಥಿರ
u32 Func_Type_ಸೂಚ್ಯಂಕ (IM3Module i_module, IM3FuncType i_type)
{
    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Func_Types; ++i) {
        ಆದರೆ (i_module->funcTypes[i] == i_type) {
            ಹಿಂತಿರುಗಿ i;
        }
    }

    ಹಿಂತಿರುಗಿ d_m3SnapshotNone;
}

#  ಆದರೆ d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
ಸ್ಥಿರ
IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್_ದ_ಸೂಚ್ಯಂಕ (IM3Module i_module, u32 i_ಸೂಚ್ಯಂಕ)
{
    IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್ = &i_module->ಟ್ಯಾಗುಗಳು[i_ಸೂಚ್ಯಂಕ];

    ಹಿಂತಿರುಗಿ ಟ್ಯಾಗ್->resolved ? ಟ್ಯಾಗ್->resolved : ಟ್ಯಾಗ್;
}

ಸ್ಥಿರ
u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ_2 (IM3Module i_module, IM3_ಟ್ಯಾಗ್ i_ಟ್ಯಾಗ್)
{
    ಗಾಗಿ (u32 i = 0; i < i_module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು; ++i) {
        ಆದರೆ (ಟ್ಯಾಗ್_ದ_ಸೂಚ್ಯಂಕ(i_module, i) == i_ಟ್ಯಾಗ್) {
            ಹಿಂತಿರುಗಿ i;
        }
    }

    ಹಿಂತಿರುಗಿ d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ NameReference (M3SnapshotSave* s, u8 i_type, ಶೂನ್ಯ* i_reference, u64* o_ಶಬ್ದ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_ಶಬ್ದ = d_m3SnapshotNullRef;

    ಆದರೆ (not i_reference) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    ಆದರೆ (i_type == c_m3Type_funcref) {
        id = Function_ಸೂಚ್ಯಂಕ(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_externref) {
        ಸ್ಥಿರಾಂಕ M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_ಶಬ್ದ = name;
        ಹೋಗು _catch;
    }
#  ಆದರೆ d_m3HasExceptionHandling
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 ಸೂಚಕ_Ids_ಕಂಡುಹಿಡಿ(&s->ಜೀವಂತ_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 ಎಣಿಕೆ_2 = s->exceptions.ಎಣಿಕೆ_2;
_       (ಸೂಚಕ_Ids_ಕೂಡಿಸು(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.ಎಣಿಕೆ_2 != ಎಣಿಕೆ_2);
    }
#  endif
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 ಸೂಚಕ_Ids_ಕಂಡುಹಿಡಿ(&s->ಜೀವಂತ_Continuations, i_reference) == d_m3SnapshotNone);

        u32 ಎಣಿಕೆ_2 = s->continuations.ಎಣಿಕೆ_2;
_       (ಸೂಚಕ_Ids_ಕೂಡಿಸು(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.ಎಣಿಕೆ_2 != ಎಣಿಕೆ_2);
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw("a reference of an unknown type");
    }

    *o_ಶಬ್ದ = id;

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Put_Ref_ಮೌಲ್ಯ (M3SnapshotSave* s, u8 i_type, ಶೂನ್ಯ* i_reference)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u64      ಶಬ್ದ   = 0;

_   (NameReference(s, i_type, i_reference, &ಶಬ್ದ));

    ಆದರೆ (not s->writing) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    ಆದರೆ (ಶಬ್ದ == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } ಇಲ್ಲದಿದ್ದರೆ {
        u8 ಬಗೆ = 0;
        ಆದರೆ (i_type == c_m3Type_funcref) {
            ಬಗೆ = 0x01;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_externref) {
            ಬಗೆ = 0x02;
        }
#  ಆದರೆ d_m3HasExceptionHandling
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_exnref) {
            ಬಗೆ = 0x03;
        }
#  endif
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_contref) {
            ಬಗೆ = 0x04;
        }

_       (PutU8(s, ಬಗೆ));
_       (PutLEB_u64(s, ಶಬ್ದ));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ PutLE32 (M3SnapshotSave* s, u32 i_ಮೌಲ್ಯ)
{
    u8 ಬೈಟುಗಳು[4];

    ಗಾಗಿ (u32 i = 0; i < ಗಾತ್ರ(ಬೈಟುಗಳು); ++i) {
        ಬೈಟುಗಳು[i] = (u8)(i_ಮೌಲ್ಯ >> (8 * i));
    }

    ಹಿಂತಿರುಗಿ Put(s, ಬೈಟುಗಳು, ಗಾತ್ರ(ಬೈಟುಗಳು));
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ PutLE64 (M3SnapshotSave* s, u64 i_ಮೌಲ್ಯ)
{
    u8 ಬೈಟುಗಳು[8];

    ಗಾಗಿ (u32 i = 0; i < ಗಾತ್ರ(ಬೈಟುಗಳು); ++i) {
        ಬೈಟುಗಳು[i] = (u8)(i_ಮೌಲ್ಯ >> (8 * i));
    }

    ಹಿಂತಿರುಗಿ Put(s, ಬೈಟುಗಳು, ಗಾತ್ರ(ಬೈಟುಗಳು));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Put_ಮೌಲ್ಯ_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_ಬಿಟ್ಟುಗಳು)
{
    ಆದರೆ (IsRefType(i_type)) {
        ಹಿಂತಿರುಗಿ Put_Ref_ಮೌಲ್ಯ(s, ತಳ_Type_ದ(i_type), (ಶೂನ್ಯ*)(uintptr_t)i_ಬಿಟ್ಟುಗಳು);
    }

    ಆಯ್ಕೆ (ತಳ_Type_ದ(i_type)) {
    ಸ್ಥಿತಿ c_m3Type_i32: ಹಿಂತಿರುಗಿ PutLEB_i64(s, (i32)(u32)i_ಬಿಟ್ಟುಗಳು);
    ಸ್ಥಿತಿ c_m3Type_i64: ಹಿಂತಿರುಗಿ PutLEB_i64(s, (i64)i_ಬಿಟ್ಟುಗಳು);
    ಸ್ಥಿತಿ c_m3Type_f32: ಹಿಂತಿರುಗಿ PutLE32(s, (u32)i_ಬಿಟ್ಟುಗಳು);
    ಸ್ಥಿತಿ c_m3Type_f64: ಹಿಂತಿರುಗಿ PutLE64(s, i_ಬಿಟ್ಟುಗಳು);
    ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "a snapshot cannot hold a value of this type as bits";
    }
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Put_ಮೌಲ್ಯ_ಟ್ಯಾಗ್ (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_ಗಮ್ಯ_Val_Type(i_type);

    ಆದರೆ (not valtype) {
        ಹಿಂತಿರುಗಿ "a snapshot has no tag for a value of this type";
    }

    ಹಿಂತಿರುಗಿ PutU8(s, valtype);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Put_ಮೌಲ್ಯ_ಬಿಟ್ಟುಗಳು (M3SnapshotSave* s, m3type_t i_type, u64 i_ಬಿಟ್ಟುಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (Put_ಮೌಲ್ಯ_ಟ್ಯಾಗ್(s, i_type));
_   (Put_ಮೌಲ್ಯ_Body(s, i_type, i_ಬಿಟ್ಟುಗಳು));

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Put_ಮೌಲ್ಯ (M3SnapshotSave* s, m3type_t i_type, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_where)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (Put_ಮೌಲ್ಯ_ಟ್ಯಾಗ್(s, i_type));

    ಆದರೆ (IsRefType(i_type)) {
        ಶೂನ್ಯ* reference;
        ಸ್ಮರಣೆ_ನಕಲು(&reference, i_where, ಗಾತ್ರ(reference));
_       (Put_ಮೌಲ್ಯ_Body(s, i_type, (u64)(uintptr_t)reference));
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ_Type_ದ(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 ಬೈಟುಗಳು[16];
        ಸ್ಮರಣೆ_ನಕಲು(ಬೈಟುಗಳು, i_where, ಗಾತ್ರ(ಬೈಟುಗಳು));
_       (Put(s, ಬೈಟುಗಳು, ಗಾತ್ರ(ಬೈಟುಗಳು)));
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (Is64_ಬಿಟ್_Type(i_type)) {
        u64 ಬಿಟ್ಟುಗಳು;
        ಸ್ಮರಣೆ_ನಕಲು(&ಬಿಟ್ಟುಗಳು, i_where, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
_       (Put_ಮೌಲ್ಯ_Body(s, i_type, ಬಿಟ್ಟುಗಳು));
    } ಇಲ್ಲದಿದ್ದರೆ {
        u32 narrow;
        ಸ್ಮರಣೆ_ನಕಲು(&narrow, i_where, ಗಾತ್ರ(narrow));
_       (Put_ಮೌಲ್ಯ_Body(s, i_type, narrow));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Put_ಸ್ಮರಣೆ_Chunks (M3SnapshotSave* s, ಸ್ಥಿರಾಂಕ u8* i_ಬೈಟುಗಳು, size_t i_ಗಾತ್ರ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    size_t   cursor = 0;

    ಆಗ (cursor < i_ಗಾತ್ರ) {
        u8     byte = i_ಬೈಟುಗಳು[cursor];
        size_t ಸತತ  = 1;

        ಆಗ (cursor + ಸತತ < i_ಗಾತ್ರ and i_ಬೈಟುಗಳು[cursor + ಸತತ] == byte) {
            ಸತತ++;
        }

        ಆದರೆ ((byte == 0x00 or byte == 0xFF) and ಸತತ >= d_m3_Snapshot_ಸತತ_ಮಿತಿ) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            ಆದರೆ (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)ಸತತ));
            }

            cursor += ಸತತ;
            ಮುಂದುವರಿ;
        }

        size_t raw_ಆರಂಭ = cursor;

        ಆಗ (cursor < i_ಗಾತ್ರ) {
            u8     b      = i_ಬೈಟುಗಳು[cursor];
            size_t ಉದ್ದಳತೆ = 1;

            ಆಗ (cursor + ಉದ್ದಳತೆ < i_ಗಾತ್ರ and i_ಬೈಟುಗಳು[cursor + ಉದ್ದಳತೆ] == b) {
                ಉದ್ದಳತೆ++;
            }

            ಆದರೆ ((b == 0x00 or b == 0xFF) and ಉದ್ದಳತೆ >= d_m3_Snapshot_ಸತತ_ಮಿತಿ) {
                ನಿಲ್ಲಿಸು;
            }

            cursor += ಉದ್ದಳತೆ;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_ಆರಂಭ));
_       (PutLEB_u32(s, (u32)(cursor - raw_ಆರಂಭ)));
_       (Put(s, i_ಬೈಟುಗಳು + raw_ಆರಂಭ, cursor - raw_ಆರಂಭ));
    }

_   (PutU8(s, d_m3_Chunk_ಕೊನೆ));

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ಸ್ಥಿರ
u32 ಸ್ಮರಣೆ_ಮೊದಲ_ಸೂಚ್ಯಂಕ (IM3Module module, u32 index)
{
    u32 j = 0;
    ಆಗ (module->memories[j] != module->memories[index]) {
        ++j;
    }
    ಹಿಂತಿರುಗಿ j;
}

ಸ್ಥಿರ
u32 ಕೋಷ್ಟಕ_ಮೊದಲ_ಸೂಚ್ಯಂಕ (IM3Module module, u32 index)
{
    u32 j = 0;
    ಆಗ (module->tables[j] != module->tables[index]) {
        ++j;
    }
    ಹಿಂತಿರುಗಿ j;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveMemories (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_Memories));

    ಗಾಗಿ (u32 m = 0; m < module->ಅಂಕೆ_Memories; ++m) {
        IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ   = module->memories[m];
        u32       ಮೊದಲ    = ಸ್ಮರಣೆ_ಮೊದಲ_ಸೂಚ್ಯಂಕ(module, m);
        bool      has_ದತ್ತಾಂಶ  = ಸ್ಮರಣೆ and ಸ್ಮರಣೆ->mallocated;
        u32       ಪುಟ_ಗಾತ್ರ = ಸ್ಮರಣೆ ? ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(ಸ್ಮರಣೆ) : d_m3_Default_Mem_ಪುಟ_ಗಾತ್ರ;
        u64       ಬೈಟುಗಳು    = ಸ್ಮರಣೆ ? ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು * (u64)ಪುಟ_ಗಾತ್ರ : 0;
        u32       ಪುಟ_ಬಿಟ್ಟುಗಳು = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, ಮೊದಲ));

        ಆದರೆ (ಮೊದಲ != m) {
            ಮುಂದುವರಿ;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", ಸ್ಮರಣೆ and ಸ್ಮರಣೆ->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", ಬೈಟುಗಳು > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 ಸ್ಮರಣೆ and ಸ್ಮರಣೆ->mallocated and ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ < ಬೈಟುಗಳು);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        ಆಗ ((1u << ಪುಟ_ಬಿಟ್ಟುಗಳು) < ಪುಟ_ಗಾತ್ರ) {
            ++ಪುಟ_ಬಿಟ್ಟುಗಳು;
        }

_       (PutLEB_u32(s, ಪುಟ_ಬಿಟ್ಟುಗಳು));
_       (PutLEB_u32(s, ಸ್ಮರಣೆ ? (u32)ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        ಆದರೆ (not s->writing) {
            ಮುಂದುವರಿ;
        }

        ಆದರೆ (has_ದತ್ತಾಂಶ) {
_           (Put_ಸ್ಮರಣೆ_Chunks(s, m3_Mem_ದತ್ತಾಂಶ(ಸ್ಮರಣೆ->mallocated), (size_t)ಬೈಟುಗಳು));
        } ಇಲ್ಲದಿದ್ದರೆ {
_           (PutU8(s, d_m3_Chunk_ಕೊನೆ));
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveGlobals (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_Globals));

    ಗಾಗಿ (u32 g = 0; g < module->ಅಂಕೆ_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_ಮೌಲ್ಯ(s, global->type, &cell->i64_ಮೌಲ್ಯ));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveTables (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_Tables));

    ಗಾಗಿ (u32 t = 0; t < module->ಅಂಕೆ_Tables; ++t) {
        IM3_ಕೋಷ್ಟಕ ಕೋಷ್ಟಕ_2 = module->tables[t];
        u32      ಮೊದಲ = ಕೋಷ್ಟಕ_ಮೊದಲ_ಸೂಚ್ಯಂಕ(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, ಮೊದಲ));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        ಆದರೆ (ಮೊದಲ != t) {
            ಮುಂದುವರಿ;
        }

_       (PutU8(s, M3_Type_ಗಮ್ಯ_Val_Type(ಕೋಷ್ಟಕ_2->type)));
_       (PutLEB_u32(s, ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2));

        ಗಾಗಿ (u32 e = 0; e < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2; ++e) {
_           (Put_Ref_ಮೌಲ್ಯ(s, ತಳ_Type_ದ(ಕೋಷ್ಟಕ_2->type), ಕೋಷ್ಟಕ_2->elements[e]));
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveSegments (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments));

    ಗಾಗಿ (u32 i = 0; i < module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments; ++i) {
_       (PutU8(s, module->ದತ್ತಾಂಶ_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->ಅಂಕೆ_Element_Segments));

    ಗಾಗಿ (u32 i = 0; i < module->ಅಂಕೆ_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#  ಆದರೆ d_m3HasExceptionHandling

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveExceptions (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.ಎಣಿಕೆ_2));

    ಗಾಗಿ (u32 i = 0; i < s->exceptions.ಎಣಿಕೆ_2; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.ವಸ್ತುಗಳು[i];
        IM3FuncType  type = exn->ಟ್ಯಾಗ್->type;

        _throwif(m3Err_wasmMalformed, exn->ಅಂಕೆ_ವಾದಗಳು != Get_Func_Type_ಅಂಕೆ_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->ಅಂಕೆ_ವಾದಗಳು));

        ಗಾಗಿ (u32 a = 0; a < exn->ಅಂಕೆ_ವಾದಗಳು; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_ಮೌಲ್ಯ_ಬಿಟ್ಟುಗಳು(s, argType, exn->ವಾದಗಳು[a]));
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3Activation {
    IM3Function        function;
    ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* ಬಿಂದು;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 ಬಗೆ;            // M3SafePointKind
    M3_ಚೌಕಟ್ಟು_Registers   registers;       // the ones it goes on with
    ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು*     resume;          // the resume it waits in, for safepoint_resume
    i32                ಮೊದಲ_ಚೌಕಟ್ಟು;      // its frames, outermost first: firstFrame down to lastFrame
    i32                ಕೊನೆಯ_ಚೌಕಟ್ಟು;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* ಕಂಡುಹಿಡಿ_ಚೌಕಟ್ಟು_ಸುರಕ್ಷಿತ_ಬಿಂದು (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_ಬಗೆ)
{
    ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* ಬಿಂದು = *io_function ? ಕಂಡುಹಿಡಿ_ಸುರಕ್ಷಿತ_ಬಿಂದು(*io_function, i_pc, i_ಬಗೆ) : ಖಾಲಿ;

    ಗಾಗಿ (u32 i = 0; not ಬಿಂದು and i < i_module->ಅಂಕೆ_Functions; ++i) {
        ಬಿಂದು = ಕಂಡುಹಿಡಿ_ಸುರಕ್ಷಿತ_ಬಿಂದು(&i_module->functions[i], i_pc, i_ಬಗೆ);
        ಆದರೆ (ಬಿಂದು) {
            *io_function = &i_module->functions[i];
        }
    }

    ಹಿಂತಿರುಗಿ ಬಿಂದು;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಮುಂದಿನ_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_ಮೊದಲ, M3Activation* o_act)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    i32      i      = i_ಮೊದಲ;

    memset(o_act, 0, ಗಾತ್ರ(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->ಮೊದಲ_ಚೌಕಟ್ಟು = i_ಮೊದಲ;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    ಗಾಗಿ (; i >= 0; --i) {
        ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟು = &i_cont->ಚೌಕಟ್ಟುಗಳು[i];

        _throwif("a suspended frame runs against another module's memory", ಚೌಕಟ್ಟು->ಸ್ಮರಣೆ != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", ಚೌಕಟ್ಟು->sp != i_sp);

        ಆದರೆ (ಚೌಕಟ್ಟು->ಬಗೆ == ಚೌಕಟ್ಟು_call) {
            o_act->pc           = ಚೌಕಟ್ಟು->pc;
            o_act->ಬಗೆ         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&ಚೌಕಟ್ಟು->call.r0;
#  ಆದರೆ d_m3HasFloat
            o_act->registers.fp0 = (f64*)&ಚೌಕಟ್ಟು->call.fp0;
#  endif
            ನಿಲ್ಲಿಸು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಚೌಕಟ್ಟು->ಬಗೆ == ಚೌಕಟ್ಟು_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = ಚೌಕಟ್ಟು->pc;
            o_act->ಬಗೆ   = safepoint_resume;
            o_act->resume = ಚೌಕಟ್ಟು;
            ನಿಲ್ಲಿಸು;
        }
    }

    ಆದರೆ (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->ಬಗೆ         = i_cont->suspend_ಬಿಂದು;
        o_act->registers.r0 = &i_cont->r0;
#  ಆದರೆ d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->ಕೊನೆಯ_ಚೌಕಟ್ಟು = i;
    o_act->ಬಿಂದು     = ಕಂಡುಹಿಡಿ_ಚೌಕಟ್ಟು_ಸುರಕ್ಷಿತ_ಬಿಂದು(i_module, &o_act->function, o_act->pc, o_act->ಬಗೆ);

    _throwif("a suspended frame is not at a safepoint", not o_act->ಬಿಂದು);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
    ಗಾಗಿ (i32 k = i_ಮೊದಲ; k > o_act->ಕೊನೆಯ_ಚೌಕಟ್ಟು; --k) {
        ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟು = &i_cont->ಚೌಕಟ್ಟುಗಳು[k];
        _throwif("a suspended frame is not the function it is entered with",
                 ಚೌಕಟ್ಟು->ಬಗೆ == ಚೌಕಟ್ಟು_ನಮೂದು and ಚೌಕಟ್ಟು->ನಮೂದು.function != o_act->function);
    }
#  endif

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveActivation (M3SnapshotSave* s, m3slot_t* i_ತಳ, u32 i_ಅಂಕೆ_Slots, ಸ್ಥಿರಾಂಕ M3Activation* i_act)
{
    M3_ಫಲಿತಾಂಶ             ಫಲಿತಾಂಶ        = m3Err_none;
    IM3Function          function      = i_act->function;
    ಸ್ಥಿರಾಂಕ M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_ಸೂಚ್ಯಂಕ = Function_ಸೂಚ್ಯಂಕ(s->module, function);
    ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು*   ಬಿಂದು         = i_act->ಬಿಂದು;
    u32                  ಅಂಕೆ_ಮೌಲ್ಯಗಳು;

    _throwif("a suspended frame belongs to another module", function_ಸೂಚ್ಯಂಕ == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_ಸೂಚ್ಯಂಕ));
_   (PutU8(s, ಬಿಂದು->ಬಗೆ));
_   (PutLEB_u32(s, ಬಿಂದು->wasm_ಸ್ಥಾನಾಂತರ));

    ಆದರೆ (ಬಿಂದು->ಬಗೆ == safepoint_op) {
_       (PutLEB_u32(s, map->ಖಂಡಗಳು[ಬಿಂದು->aux].wasm_ಸ್ಥಾನಾಂತರ));
    }

    ಅಂಕೆ_ಮೌಲ್ಯಗಳು = map->ಅಂಕೆ_Locals + ಬಿಂದು->ಅಂಕೆ_ಮೌಲ್ಯಗಳು;

_   (PutLEB_u32(s, ಅಂಕೆ_ಮೌಲ್ಯಗಳು));

    ಗಾಗಿ (u32 v = 0; v < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++v) {
        ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* ಮೌಲ್ಯ = ಚೌಕಟ್ಟು_ಮೌಲ್ಯ(map, ಬಿಂದು, v);
        u8*                where;
        u64                ಶಬ್ದ;
        ಶೂನ್ಯ*              reference;

_       (Locate_ಮೌಲ್ಯ(i_ತಳ, i_ಅಂಕೆ_Slots, i_act->sp, ಮೌಲ್ಯ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ಆದರೆ (ತಳ_Type_ದ(ಮೌಲ್ಯ->type) == c_m3Type_v128) {
_           (Put_ಮೌಲ್ಯ_ಟ್ಯಾಗ್(s, ಮೌಲ್ಯ->type));
_           (Put(s, where, 16));
            ಮುಂದುವರಿ;
        }

_       (Read_ಮೌಲ್ಯ(where, &i_act->registers, ಮೌಲ್ಯ, &ಶಬ್ದ, &reference));

_       (Put_ಮೌಲ್ಯ_ಬಿಟ್ಟುಗಳು(s, ಮೌಲ್ಯ->type, IsRefType(ಮೌಲ್ಯ->type) ? (u64)(uintptr_t)reference : ಶಬ್ದ));
    }

    ಆದರೆ (ಬಿಂದು->ಬಗೆ == safepoint_resume) {
_       (Put_Ref_ಮೌಲ್ಯ(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_ಫಲಿತಾಂಶ      ಫಲಿತಾಂಶ   = m3Err_none;
    M3Activation* acts     = ಖಾಲಿ;
    u32           ಅಂಕೆ_Acts  = 0;
    IM3Function   function = i_cont->ನಮೂದು_Function;
    i32           ಮೊದಲ    = (i32)i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು - 1;
    m3slot_t*     ತಳ;
    u32           ಅಂಕೆ_Slots;
    m3stack_t     sp;

_   (Continuation_ರಾಶಿ(s->runtime, i_cont, &ತಳ, &ಅಂಕೆ_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು + 1);
    _throwifnull(acts);

    sp = ತಳ;

    ಆಗ (true) {
        M3Activation* act = &acts[ಅಂಕೆ_Acts];

_       (ಮುಂದಿನ_Activation(s->module, i_cont, function, sp, ಮೊದಲ, act));

        ಆದರೆ (act->ಬಗೆ != safepoint_call) {
            ಅಂಕೆ_Acts++;
            ನಿಲ್ಲಿಸು;
        }

        _throwif("a suspended continuation stops at a call", act->ಕೊನೆಯ_ಚೌಕಟ್ಟು < 0);

        ಆದರೆ (not (act->ಬಿಂದು->flags & d_m3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ಬಾಲ_Call)) {
            ಅಂಕೆ_Acts++;
        }

        function = i_cont->ಚೌಕಟ್ಟುಗಳು[act->ಕೊನೆಯ_ಚೌಕಟ್ಟು].call.function;
        sp       = act->sp + act->ಬಿಂದು->aux;
        ಮೊದಲ    = act->ಕೊನೆಯ_ಚೌಕಟ್ಟು - 1;
    }

_   (PutLEB_u32(s, ಅಂಕೆ_Acts));

    ಗಾಗಿ (u32 a = 0; a < ಅಂಕೆ_Acts; ++a) {
_       (SaveActivation(s, ತಳ, ಅಂಕೆ_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_ಬೇರು    = (i_cont == s->runtime->ಬೇರು_Continuation);
    u8        state     = snapshot_cont_ಪೂರ್ಣಗೊಂಡ;
    u32       type_ಸೂಚ್ಯಂಕ = is_ಬೇರು ? d_m3SnapshotNone : Func_Type_ಸೂಚ್ಯಂಕ(module, i_cont->type);

    ಆದರೆ (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->ನಮೂದು_Function);
    _throwif("a continuation belongs to another module", Function_ಸೂಚ್ಯಂಕ(module, i_cont->ನಮೂದು_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_ಬೇರು and type_ಸೂಚ್ಯಂಕ == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_ಬೇರು));
_   (PutLEB_u32(s, type_ಸೂಚ್ಯಂಕ));
_   (PutLEB_u32(s, Function_ಸೂಚ್ಯಂಕ(module, i_cont->ನಮೂದು_Function)));
#  ಆದರೆ d_m3HasExceptionHandling
_   (Put_Ref_ಮೌಲ್ಯ(s, c_m3Type_exnref, i_cont->resumeThrow));
#  ಇಲ್ಲದಿದ್ದರೆ
_   (Put_Ref_ಮೌಲ್ಯ(s, c_m3Type_exnref, ಖಾಲಿ));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    ಆದರೆ (s->postmortem) {
        ಹೋಗು _catch;
    }

    ಆದರೆ (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->ನಮೂದು_Function ? i_cont->ನಮೂದು_Function->funcType : ಖಾಲಿ;
        u16         ಅಂಕೆ_Rets = inner ? Get_Func_Type_ಅಂಕೆ_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_ವಾದಗಳು_ಎಣಿಕೆ > Get_Func_Type_ಅಂಕೆ_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_ವಾದಗಳು_ಎಣಿಕೆ));

        ಗಾಗಿ (u32 i = 0; i < i_cont->bound_ವಾದಗಳು_ಎಣಿಕೆ; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_ಮೌಲ್ಯ(s, type, i_cont->val_ರಾಶಿ + (ಅಂಕೆ_Rets + i) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ));
        }
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_ವಾದಗಳು_ಎಣಿಕೆ));
_       (SaveSuspendedBody(s, i_cont));
    } ಇಲ್ಲದಿದ್ದರೆ {
_       (PutLEB_u32(s, 0));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveContinuations (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (PutLEB_u32(s, s->continuations.ಎಣಿಕೆ_2));

    ಗಾಗಿ (u32 i = 0; i < s->continuations.ಎಣಿಕೆ_2; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.ವಸ್ತುಗಳು[i]));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveHostState (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ               ಫಲಿತಾಂಶ = m3Err_none;
    ಸ್ಥಿರಾಂಕ M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_ಬಫರ್_Writer         bw     = { ಖಾಲಿ, 0, 0 };

    ಆದರೆ (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, ಬಫರ್_Writer_Write, &bw));
    }

    ಆದರೆ (bw.ಗಾತ್ರ_2 > 0) {
_       (Put(s, bw.ಬಫರ್, bw.ಗಾತ್ರ_2));
    }

_catch:
    m3_Free(bw.ಬಫರ್);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
ಶೂನ್ಯ SnapshotResourceTotals (IM3Module module, u64* ಸ್ಮರಣೆ_ಬೈಟುಗಳು, u64* ಕೋಷ್ಟಕ_Elements)
{
    *ಸ್ಮರಣೆ_ಬೈಟುಗಳು = *ಕೋಷ್ಟಕ_Elements = 0;
    ಗಾಗಿ (u32 i = 0; i < module->ಅಂಕೆ_Memories; ++i) {
        IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ = module->memories[i];
        ಆದರೆ (ಸ್ಮರಣೆ_ಮೊದಲ_ಸೂಚ್ಯಂಕ(module, i) == i and ಸ್ಮರಣೆ) {
            *ಸ್ಮರಣೆ_ಬೈಟುಗಳು += ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು * (u64)ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(ಸ್ಮರಣೆ);
        }
    }
    ಗಾಗಿ (u32 i = 0; i < module->ಅಂಕೆ_Tables; ++i) {
        IM3_ಕೋಷ್ಟಕ ಕೋಷ್ಟಕ_2 = module->tables[i];
        ಆದರೆ (ಕೋಷ್ಟಕ_ಮೊದಲ_ಸೂಚ್ಯಂಕ(module, i) == i and ಕೋಷ್ಟಕ_2) {
            *ಕೋಷ್ಟಕ_Elements += ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2;
        }
    }
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveMeta (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u64      hash   = 0;
    u64      ಸ್ಮರಣೆ_ಬೈಟುಗಳು, ಕೋಷ್ಟಕ_Elements;
    u32      ಸಕ್ರಿಯ_Stacks = 0;
    ಗಾಗಿ (u32 i = 0; i < s->continuations.ಎಣಿಕೆ_2; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.ವಸ್ತುಗಳು[i];
        ಆದರೆ (cont != s->runtime->ಬೇರು_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            ಸಕ್ರಿಯ_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &ಸ್ಮರಣೆ_ಬೈಟುಗಳು, &ಕೋಷ್ಟಕ_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_ಸಮಯ_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.ಎಣಿಕೆ_2));
_   (PutLEB_u32(s, s->exceptions.ಎಣಿಕೆ_2));
_   (PutLEB_u64(s, ಸ್ಮರಣೆ_ಬೈಟುಗಳು));
_   (PutLEB_u64(s, ಕೋಷ್ಟಕ_Elements));
_   (PutLEB_u32(s, ಸಕ್ರಿಯ_Stacks));

#  ಆದರೆ d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    ಗಾಗಿ (u32 i = 0; i < s->exceptions.ಎಣಿಕೆ_2; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.ವಸ್ತುಗಳು[i];
        u32          ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ  = ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ_2(s->module, exception->ಟ್ಯಾಗ್);

        _throwif("an exception was thrown with another module's tag", ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ == d_m3SnapshotNone);

_       (PutLEB_u32(s, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ));
    }
#  endif

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ DiscoverReferences (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ            = m3Err_none;
    u32      ಮುಗಿದ_Continuations = 0;
    u32      ಮುಗಿದ_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    ಆಗ (ಮುಗಿದ_Continuations < s->continuations.ಎಣಿಕೆ_2 or ಮುಗಿದ_Exceptions < s->exceptions.ಎಣಿಕೆ_2) {
#  ಆದರೆ d_m3HasExceptionHandling
        ಆಗ (ಮುಗಿದ_Exceptions < s->exceptions.ಎಣಿಕೆ_2) {
            M3Exception* exception = (M3Exception*)s->exceptions.ವಸ್ತುಗಳು[ಮುಗಿದ_Exceptions++];
            IM3FuncType  type      = exception->ಟ್ಯಾಗ್->type;

            _throwif(m3Err_wasmMalformed, exception->ಅಂಕೆ_ವಾದಗಳು != Get_Func_Type_ಅಂಕೆ_Params(type));

            ಗಾಗಿ (u32 a = 0; a < exception->ಅಂಕೆ_ವಾದಗಳು; ++a) {
_               (Put_ಮೌಲ್ಯ_ಬಿಟ್ಟುಗಳು(s, GetFuncTypeParamType(type, (u16)a), exception->ವಾದಗಳು[a]));
            }
        }
#  endif
        ಆಗ (ಮುಗಿದ_Continuations < s->continuations.ಎಣಿಕೆ_2 and ಮುಗಿದ_Exceptions == s->exceptions.ಎಣಿಕೆ_2) {
            u32 id = ಮುಗಿದ_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.ವಸ್ತುಗಳು[id]));
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveSections (M3SnapshotSave* s)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (not s->writing) {
        ಹಿಂತಿರುಗಿ DiscoverReferences(s);
    }

    ಆದರೆ (not s->embedded) {
_       (Put(s, c_snapshotMagic, ಗಾತ್ರ(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_ಆವೃತ್ತಿ));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    ಆದರೆ (s->module->ಅಂಕೆ_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_ಸ್ಮರಣೆ, SaveMemories));
    }
    ಆದರೆ (s->module->ಅಂಕೆ_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_ಕೋಷ್ಟಕ, SaveTables));
    }
    ಆದರೆ (s->module->ಅಂಕೆ_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    ಆದರೆ (s->module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments or s->module->ಅಂಕೆ_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  ಆದರೆ d_m3HasExceptionHandling
    ಆದರೆ (s->exceptions.ಎಣಿಕೆ_2) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    ಆದರೆ (s->continuations.ಎಣಿಕೆ_2) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3SnapshotSuspendSite {
    ಸ್ಥಿರಾಂಕ M3SnapshotMap* map;
    ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು*   ಬಿಂದು;
    u32                  ಗುರಿ;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_Snapshot_ತುಂಬು {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    ಶೂನ್ಯ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ರಾಶಿ;
    bool      ಬದಲಾಯಿತು;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    ಅಂಕೆ_Continuations;
    u64                    required_ಸ್ಮರಣೆ_ಬೈಟುಗಳು;
    u64                    required_ಕೋಷ್ಟಕ_Elements;
    u64                    loaded_ಸ್ಮರಣೆ_ಬೈಟುಗಳು;
    u64                    loaded_ಕೋಷ್ಟಕ_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  ಆದರೆ d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 ಅಂಕೆ_Exceptions;

    M3_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟುಗಳು;           // a suspended continuation's frames as they are put together, outermost first

    ಬೈಟುಗಳು_t pos;               // bounded section payload, read using the Wasm parser's helpers
    ಬೈಟುಗಳು_t ಕೊನೆ;
    bool    ಒಳ_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    ನೋಡಿದ_Sections[256];
} M3_Snapshot_ತುಂಬು;

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get (M3_Snapshot_ತುಂಬು* l, ಶೂನ್ಯ* o_ದತ್ತಾಂಶ, size_t i_ಗಾತ್ರ)
{
    ಆದರೆ (not l->ಒಳ_Section) {
        ಹಿಂತಿರುಗಿ l->reader(o_ದತ್ತಾಂಶ, i_ಗಾತ್ರ, l->userdata);
    }
    ಆದರೆ (i_ಗಾತ್ರ > (size_t)(l->ಕೊನೆ - l->pos)) {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
    ಆದರೆ (i_ಗಾತ್ರ) {
        ಸ್ಮರಣೆ_ನಕಲು(o_ದತ್ತಾಂಶ, l->pos, i_ಗಾತ್ರ);
        l->pos += i_ಗಾತ್ರ;
    }
    ಹಿಂತಿರುಗಿ m3Err_none;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetU8 (M3_Snapshot_ತುಂಬು* l, u8* o_ಮೌಲ್ಯ)
{
    ಹಿಂತಿರುಗಿ Get(l, o_ಮೌಲ್ಯ, ಗಾತ್ರ(*o_ಮೌಲ್ಯ));
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetBool (M3_Snapshot_ತುಂಬು* l, u8* o_ಮೌಲ್ಯ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = GetU8(l, o_ಮೌಲ್ಯ);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ ? ಫಲಿತಾಂಶ : (*o_ಮೌಲ್ಯ <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetLEB_u32 (M3_Snapshot_ತುಂಬು* l, u32* o_ಮೌಲ್ಯ)
{
    ಆದರೆ (l->ಒಳ_Section) {
        ಹಿಂತಿರುಗಿ ReadLEB_u32(o_ಮೌಲ್ಯ, &l->pos, l->ಕೊನೆ);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    ಗಾಗಿ (u32 i = 0; i < ಗಾತ್ರ(encoded); ++i) {
        M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = GetU8(l, &encoded[i]);
        ಆದರೆ (ಫಲಿತಾಂಶ) {
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        ಆದರೆ (not (encoded[i] & 0x80)) {
            ಬೈಟುಗಳು_t pos = encoded;
            ಹಿಂತಿರುಗಿ ReadLEB_u32(o_ಮೌಲ್ಯ, &pos, encoded + i + 1);
        }
    }
    ಹಿಂತಿರುಗಿ m3_Err_leb_ಅತಿಪ್ರವಾಹ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetLEB_u64 (M3_Snapshot_ತುಂಬು* l, u64* o_ಮೌಲ್ಯ)
{
    ಹಿಂತಿರುಗಿ ReadLebUnsigned(o_ಮೌಲ್ಯ, 64, &l->pos, l->ಕೊನೆ);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetLEB_i64 (M3_Snapshot_ತುಂಬು* l, i64* o_ಮೌಲ್ಯ)
{
    ಹಿಂತಿರುಗಿ ReadLEB_i64(o_ಮೌಲ್ಯ, &l->pos, l->ಕೊನೆ);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Skip_ಬೈಟುಗಳು (M3_Snapshot_ತುಂಬು* l, size_t i_ಗಾತ್ರ)
{
    ಆದರೆ (i_ಗಾತ್ರ > (size_t)(l->ಕೊನೆ - l->pos)) {
        ಹಿಂತಿರುಗಿ m3Err_wasmUnderrun;
    }
    l->pos += i_ಗಾತ್ರ;
    ಹಿಂತಿರುಗಿ m3Err_none;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get_Ref_ಮೌಲ್ಯ (M3_Snapshot_ತುಂಬು* l, m3type_t i_type, ಶೂನ್ಯ** o_reference)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u8       ಬಗೆ   = 0;
    u64      id     = 0;

    *o_reference = ಖಾಲಿ;

_   (GetU8(l, &ಬಗೆ));
    ಆದರೆ (ಬಗೆ == 0x00) {
        ಹಿಂತಿರುಗಿ (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    ಆದರೆ (ಬಗೆ == 0x01) {
        _throwif(m3Err_wasmMalformed, ತಳ_Type_ದ(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->ಅಂಕೆ_Functions);
#  ಆದರೆ d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_ದ(Ref_Type_ದ_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಗೆ == 0x02) {
        _throwif(m3Err_wasmMalformed, ತಳ_Type_ದ(i_type) != c_m3Type_externref);
        ಸ್ಥಿರಾಂಕ M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        ಆದರೆ (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  ಆದರೆ d_m3HasExceptionHandling
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಗೆ == 0x03) {
        _throwif(m3Err_wasmMalformed, ತಳ_Type_ದ(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->ಅಂಕೆ_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಗೆ == 0x04) {
        _throwif(m3Err_wasmMalformed, ತಳ_Type_ದ(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->ಅಂಕೆ_Continuations);
        *o_reference = l->continuations[id];
#  ಆದರೆ d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        ಆದರೆ (not l->checking and ರಾಶಿ_Type_ದ(i_type) != d_m3_Type_ರಾಶಿ_Abstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetLE32 (M3_Snapshot_ತುಂಬು* l, u32* o_ಮೌಲ್ಯ)
{
    u8       ಬೈಟುಗಳು[4];
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = Get(l, ಬೈಟುಗಳು, ಗಾತ್ರ(ಬೈಟುಗಳು));

    *o_ಮೌಲ್ಯ = 0;

    ಗಾಗಿ (u32 i = 0; i < ಗಾತ್ರ(ಬೈಟುಗಳು); ++i) {
        *o_ಮೌಲ್ಯ |= (u32)ಬೈಟುಗಳು[i] << (8 * i);
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetLE64 (M3_Snapshot_ತುಂಬು* l, u64* o_ಮೌಲ್ಯ)
{
    u8       ಬೈಟುಗಳು[8];
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = Get(l, ಬೈಟುಗಳು, ಗಾತ್ರ(ಬೈಟುಗಳು));

    *o_ಮೌಲ್ಯ = 0;

    ಗಾಗಿ (u32 i = 0; i < ಗಾತ್ರ(ಬೈಟುಗಳು); ++i) {
        *o_ಮೌಲ್ಯ |= (u64)ಬೈಟುಗಳು[i] << (8 * i);
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get_ಮೌಲ್ಯ_ಟ್ಯಾಗ್ (M3_Snapshot_ತುಂಬು* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_ಗಮ್ಯ_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and ತಳ_Type_ದ(*o_type) != ತಳ_Type_ದ(i_expectedType));
    ಆದರೆ (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get_ಮೌಲ್ಯ_Body (M3_Snapshot_ತುಂಬು* l, m3type_t i_type, u64* o_ಬಿಟ್ಟುಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    *o_ಬಿಟ್ಟುಗಳು = 0;

    ಆದರೆ (IsRefType(i_type)) {
        ಶೂನ್ಯ* reference = ಖಾಲಿ;
_       (Get_Ref_ಮೌಲ್ಯ(l, i_type, &reference));
        *o_ಬಿಟ್ಟುಗಳು = (u64)(uintptr_t)reference;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ_Type_ದ(i_type) == c_m3Type_i32) {
        i32 ಮೌಲ್ಯ = 0;
_       (ReadLEB_i32(&ಮೌಲ್ಯ, &l->pos, l->ಕೊನೆ));
        *o_ಬಿಟ್ಟುಗಳು = (u32)ಮೌಲ್ಯ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ_Type_ದ(i_type) == c_m3Type_i64) {
        i64 ಮೌಲ್ಯ = 0;
_       (GetLEB_i64(l, &ಮೌಲ್ಯ));
        *o_ಬಿಟ್ಟುಗಳು = (u64)ಮೌಲ್ಯ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ_Type_ದ(i_type) == c_m3Type_f32) {
        u32 ಬಿಟ್ಟುಗಳು = 0;
_       (GetLE32(l, &ಬಿಟ್ಟುಗಳು));
        *o_ಬಿಟ್ಟುಗಳು = ಬಿಟ್ಟುಗಳು;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ_Type_ದ(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_ಬಿಟ್ಟುಗಳು));
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get_ಮೌಲ್ಯ_ಬಿಟ್ಟುಗಳು (M3_Snapshot_ತುಂಬು* l, m3type_t i_expectedType, u64* o_ಬಿಟ್ಟುಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    m3type_t type;

_   (Get_ಮೌಲ್ಯ_ಟ್ಯಾಗ್(l, i_expectedType, &type));
_   (Get_ಮೌಲ್ಯ_Body(l, type, o_ಬಿಟ್ಟುಗಳು));

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Get_ಮೌಲ್ಯ (M3_Snapshot_ತುಂಬು* l, m3type_t i_expectedType, ಶೂನ್ಯ* o_where)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    ಆದರೆ (l->checking) {
        o_where = scratch;
    }

_   (Get_ಮೌಲ್ಯ_ಟ್ಯಾಗ್(l, i_expectedType, &type));

    ಆದರೆ (IsRefType(type)) {
        ಶೂನ್ಯ* reference = ಖಾಲಿ;
        u64   ಬಿಟ್ಟುಗಳು      = 0;
_       (Get_ಮೌಲ್ಯ_Body(l, type, &ಬಿಟ್ಟುಗಳು));
        reference = (ಶೂನ್ಯ*)(uintptr_t)ಬಿಟ್ಟುಗಳು;
        ಸ್ಮರಣೆ_ನಕಲು(o_where, &reference, ಗಾತ್ರ(reference));
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ತಳ_Type_ದ(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } ಇಲ್ಲದಿದ್ದರೆ {
        u64 ಬಿಟ್ಟುಗಳು = 0;
_       (Get_ಮೌಲ್ಯ_Body(l, type, &ಬಿಟ್ಟುಗಳು));

        ಆದರೆ (Is64_ಬಿಟ್_Type(type)) {
            ಸ್ಮರಣೆ_ನಕಲು(o_where, &ಬಿಟ್ಟುಗಳು, ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
        } ಇಲ್ಲದಿದ್ದರೆ {
            u32 narrow = (u32)ಬಿಟ್ಟುಗಳು;
            ಸ್ಮರಣೆ_ನಕಲು(o_where, &narrow, ಗಾತ್ರ(narrow));
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ GetFunction (M3_Snapshot_ತುಂಬು* l, IM3Function* o_function)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->ಅಂಕೆ_Functions);
    *o_function = &l->module->functions[index];

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_ಸ್ಮರಣೆ_Chunks (M3_Snapshot_ತುಂಬು* l, u8* o_ಬೈಟುಗಳು, size_t i_ಗಾತ್ರ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಗಾಗಿ (;;) {
        u8  chunkType = 0;
        u32 offset = 0, ಉದ್ದಳತೆ = 0;

_       (GetU8(l, &chunkType));

        ಆದರೆ (chunkType == d_m3_Chunk_ಕೊನೆ) {
            ನಿಲ್ಲಿಸು;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &ಉದ್ದಳತೆ));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_ಗಾತ್ರ or ಉದ್ದಳತೆ > i_ಗಾತ್ರ - (size_t)offset);

        ಆದರೆ (chunkType == d_m3ChunkRaw) {
            ಆದರೆ (l->checking or ಉದ್ದಳತೆ == 0) {
_               (Skip_ಬೈಟುಗಳು(l, ಉದ್ದಳತೆ));
            } ಇಲ್ಲದಿದ್ದರೆ {
_               (Get(l, o_ಬೈಟುಗಳು + offset, ಉದ್ದಳತೆ));
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (chunkType == d_m3ChunkFillFF) {
            ಆದರೆ (not l->checking and ಉದ್ದಳತೆ) {
                memset(o_ಬೈಟುಗಳು + offset, 0xFF, ಉದ್ದಳತೆ);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Memories (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ      = m3Err_none;
    IM3Module module      = l->module;
    u32       ಅಂಕೆ_Memories = 0;
    l->loaded_ಸ್ಮರಣೆ_ಬೈಟುಗಳು  = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Memories == 0 or ಅಂಕೆ_Memories != module->ಅಂಕೆ_Memories);

    ಗಾಗಿ (u32 m = 0; m < ಅಂಕೆ_Memories; ++m) {
        u32       mem_ಸೂಚ್ಯಂಕ = 0, ಮೊದಲ_ಸೂಚ್ಯಂಕ = 0, ಪುಟ_ಬಿಟ್ಟುಗಳು = 0, ಅಂಕೆ_ಪುಟಗಳು = 0;
        IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ;

_       (GetLEB_u32(l, &mem_ಸೂಚ್ಯಂಕ));
_       (GetLEB_u32(l, &ಮೊದಲ_ಸೂಚ್ಯಂಕ));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_ಸೂಚ್ಯಂಕ != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 ಮೊದಲ_ಸೂಚ್ಯಂಕ != ಸ್ಮರಣೆ_ಮೊದಲ_ಸೂಚ್ಯಂಕ(module, m));
        ಆದರೆ (ಮೊದಲ_ಸೂಚ್ಯಂಕ != m) {
            ಮುಂದುವರಿ;
        }

_       (GetLEB_u32(l, &ಪುಟ_ಬಿಟ್ಟುಗಳು));
_       (GetLEB_u32(l, &ಅಂಕೆ_ಪುಟಗಳು));

        ಸ್ಮರಣೆ = module->memories[mem_ಸೂಚ್ಯಂಕ];
        _throwif("a shared memory cannot be restored from a snapshot", ಸ್ಮರಣೆ and ಸ್ಮರಣೆ->isShared);

        size_t ಬೈಟುಗಳು = 0;
        u8*    ದತ್ತಾಂಶ  = ಖಾಲಿ;

        ಆದರೆ (ಸ್ಮರಣೆ) {
            _throwif(m3Err_wasmMalformed, ಪುಟ_ಬಿಟ್ಟುಗಳು >= 32 or (1u << ಪುಟ_ಬಿಟ್ಟುಗಳು) != ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(ಸ್ಮರಣೆ));
            _throwif(m3Err_wasmMalformed, ಅಂಕೆ_ಪುಟಗಳು < ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು or ಅಂಕೆ_ಪುಟಗಳು > ಸ್ಮರಣೆ->max_ಪುಟಗಳು);
            _throwif(m3Err_wasmMalformed, ಅಂಕೆ_ಪುಟಗಳು > SIZE_MAX / ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(ಸ್ಮರಣೆ));
            ಬೈಟುಗಳು = (size_t)ಅಂಕೆ_ಪುಟಗಳು * ಸ್ಮರಣೆ_ಪುಟ_ಗಾತ್ರ(ಸ್ಮರಣೆ);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->ಸ್ಮರಣೆ_ಎಲ್ಲೆ and ಬೈಟುಗಳು > (size_t)l->runtime->ಸ್ಮರಣೆ_ಎಲ್ಲೆ);

            ಆದರೆ (not l->checking) {
                ಆದರೆ (ಸ್ಮರಣೆ->ಅಂಕೆ_ಪುಟಗಳು < ಅಂಕೆ_ಪುಟಗಳು) {
_                   (Resize_ಸ್ಮರಣೆ(l->runtime, ಸ್ಮರಣೆ, ಅಂಕೆ_ಪುಟಗಳು));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         ಸ್ಮರಣೆ->mallocated and ಸ್ಮರಣೆ->mallocated->ಉದ್ದಳತೆ < ಬೈಟುಗಳು);
            }

            l->loaded_ಸ್ಮರಣೆ_ಬೈಟುಗಳು += ಬೈಟುಗಳು;

            ಆದರೆ (ಬೈಟುಗಳು and ಸ್ಮರಣೆ->mallocated) {
                ದತ್ತಾಂಶ = m3_Mem_ದತ್ತಾಂಶ(ಸ್ಮರಣೆ->mallocated);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throwif(m3Err_wasmMalformed, ಅಂಕೆ_ಪುಟಗಳು != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        ಆದರೆ (ದತ್ತಾಂಶ and not l->checking) {
            memset(ದತ್ತಾಂಶ, 0, ಬೈಟುಗಳು);
        }

_       (ತುಂಬು_ಸ್ಮರಣೆ_Chunks(l, ದತ್ತಾಂಶ, ಬೈಟುಗಳು));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_ಸ್ಮರಣೆ_ಬೈಟುಗಳು != l->required_ಸ್ಮರಣೆ_ಬೈಟುಗಳು);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Globals (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ     = m3Err_none;
    IM3Module module     = l->module;
    u32       ಅಂಕೆ_Globals = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Globals));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Globals == 0 or ಅಂಕೆ_Globals != module->ಅಂಕೆ_Globals);

    ಗಾಗಿ (u32 g = 0; g < ಅಂಕೆ_Globals; ++g) {
        u32       global_ಸೂಚ್ಯಂಕ = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_ಸೂಚ್ಯಂಕ));
        _throwif(m3Err_wasmMalformed, global_ಸೂಚ್ಯಂಕ != g);

        global = &module->globals[global_ಸೂಚ್ಯಂಕ];
        cell   = global->resolved ? global->resolved : global;

_       (Get_ಮೌಲ್ಯ(l, global->type, &cell->i64_ಮೌಲ್ಯ));
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Tables (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ       = m3Err_none;
    IM3Module module       = l->module;
    u32       ಅಂಕೆ_Tables    = 0;
    l->loaded_ಕೋಷ್ಟಕ_Elements = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Tables));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Tables == 0 or ಅಂಕೆ_Tables != module->ಅಂಕೆ_Tables);

    ಗಾಗಿ (u32 t = 0; t < ಅಂಕೆ_Tables; ++t) {
        u32      ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ = 0, ಮೊದಲ_ಸೂಚ್ಯಂಕ = 0, ಗಾತ್ರ_2 = 0;
        u8       valtype = 0;
        IM3_ಕೋಷ್ಟಕ ಕೋಷ್ಟಕ_2;

_       (GetLEB_u32(l, &ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ));
_       (GetLEB_u32(l, &ಮೊದಲ_ಸೂಚ್ಯಂಕ));

        _throwif(m3Err_wasmMalformed, ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ != t);
        _throwif("the snapshot shares tables between imports differently",
                 ಮೊದಲ_ಸೂಚ್ಯಂಕ != ಕೋಷ್ಟಕ_ಮೊದಲ_ಸೂಚ್ಯಂಕ(module, t));
        ಆದರೆ (ಮೊದಲ_ಸೂಚ್ಯಂಕ != t) {
            ಮುಂದುವರಿ;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &ಗಾತ್ರ_2));

        ಕೋಷ್ಟಕ_2 = module->tables[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_ಗಮ್ಯ_Val_Type(ಕೋಷ್ಟಕ_2->type));
        _throwif(m3Err_wasmMalformed, ಗಾತ್ರ_2 > (ಕೋಷ್ಟಕ_2->hasMax ? ಕೋಷ್ಟಕ_2->max_ಗಾತ್ರ : d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಗಾತ್ರ));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, ಗಾತ್ರ_2 < ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2);

        l->loaded_ಕೋಷ್ಟಕ_Elements += ಗಾತ್ರ_2;

        ಆದರೆ (not l->checking and ಗಾತ್ರ_2 != ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2) {
            ಶೂನ್ಯ** elements;

            _throwif(m3_Err_ಕೋಷ್ಟಕ_ಎಲ್ಲೆ_Exceeded, l->runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ and
                                                 ಗಾತ್ರ_2 - ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2 > l->runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ - l->runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ);
            elements = m3_ReallocArray(ಶೂನ್ಯ*, ಕೋಷ್ಟಕ_2->elements, ಗಾತ್ರ_2, ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2);
            _throwifnull(elements);

            l->runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ += ಗಾತ್ರ_2 - ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2;
            ಕೋಷ್ಟಕ_2->elements = elements;
            ಕೋಷ್ಟಕ_2->ಗಾತ್ರ_2     = ಗಾತ್ರ_2;
        }

        ಗಾಗಿ (u32 e = 0; e < ಗಾತ್ರ_2; ++e) {
            ಶೂನ್ಯ* reference = ಖಾಲಿ;
_           (Get_Ref_ಮೌಲ್ಯ(l, ಕೋಷ್ಟಕ_2->type, &reference));
            ಆದರೆ (not l->checking) {
                ಕೋಷ್ಟಕ_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_ಕೋಷ್ಟಕ_Elements != l->required_ಕೋಷ್ಟಕ_Elements);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Segments (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ = m3Err_none;
    IM3Module module = l->module;
    u32       ಎಣಿಕೆ_2  = 0;

_   (GetLEB_u32(l, &ಎಣಿಕೆ_2));
    _throwif(m3Err_wasmMalformed, ಎಣಿಕೆ_2 != module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments);
    _throwif(m3Err_wasmMalformed, module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments == 0 and module->ಅಂಕೆ_Element_Segments == 0);

    ಗಾಗಿ (u32 i = 0; i < ಎಣಿಕೆ_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ಆದರೆ (not l->checking) {
            module->ದತ್ತಾಂಶ_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &ಎಣಿಕೆ_2));
    _throwif(m3Err_wasmMalformed, ಎಣಿಕೆ_2 != module->ಅಂಕೆ_Element_Segments);

    ಗಾಗಿ (u32 i = 0; i < ಎಣಿಕೆ_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ಆದರೆ (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// A frame for the continuation being put together, outermost first
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಕೂಡಿಸು_ಚೌಕಟ್ಟು (M3_Snapshot_ತುಂಬು* l, u32* io_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು, M3_ಚೌಕಟ್ಟು_ಬಗೆ i_ಬಗೆ, m3stack_t i_sp, M3_ಚೌಕಟ್ಟು** o_ಚೌಕಟ್ಟು)
{
    ಆದರೆ (*io_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು >= d_m3_Continuation_Max_ಚೌಕಟ್ಟುಗಳು) {
        ಹಿಂತಿರುಗಿ "the snapshot's frames do not fit this build's";
    }

    M3_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟು = &l->ಚೌಕಟ್ಟುಗಳು[(*io_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು)++];

    memset(ಚೌಕಟ್ಟು, 0, ಗಾತ್ರ(*ಚೌಕಟ್ಟು));
    ಚೌಕಟ್ಟು->ಬಗೆ   = (u8)i_ಬಗೆ;
    ಚೌಕಟ್ಟು->sp     = i_sp;
    ಚೌಕಟ್ಟು->ಸ್ಮರಣೆ = Module_Memory0(l->module);

    *o_ಚೌಕಟ್ಟು = ಚೌಕಟ್ಟು;

    ಹಿಂತಿರುಗಿ m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ PrepareFunction (IM3Function i_function)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಆದರೆ (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಕೂಡಿಸು_ಖಂಡ_ಚೌಕಟ್ಟುಗಳು (M3_Snapshot_ತುಂಬು* l, ಸ್ಥಿರಾಂಕ M3SnapshotMap* i_map, ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* i_ಬಿಂದು, m3stack_t i_sp,
                         u32* io_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಗಾಗಿ (u32 b = 0; b < i_map->ಅಂಕೆ_ಖಂಡಗಳು; ++b) {
        ಸ್ಥಿರಾಂಕ M3_ಖಂಡ_ಆರಂಭ* ಖಂಡ = &i_map->ಖಂಡಗಳು[b];
        M3_ಚೌಕಟ್ಟು*            ಚೌಕಟ್ಟು;

        ಆದರೆ (not Encloses_ಸ್ಥಾನಾಂತರ(ಖಂಡ, i_ಬಿಂದು->wasm_ಸ್ಥಾನಾಂತರ)) {
            ಮುಂದುವರಿ;
        }

        ಆದರೆ (ಖಂಡ->opcode == c_waOp_loop) {
_           (ಕೂಡಿಸು_ಚೌಕಟ್ಟು(l, io_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು, ಚೌಕಟ್ಟು_loop, i_sp, &ಚೌಕಟ್ಟು));
            ಚೌಕಟ್ಟು->pc = ಖಂಡ->pc;
        }
#  ಆದರೆ d_m3HasExceptionHandling
        ಇಲ್ಲದಿದ್ದರೆ {
_           (ಕೂಡಿಸು_ಚೌಕಟ್ಟು(l, io_ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು, ಚೌಕಟ್ಟು_try, i_sp, &ಚೌಕಟ್ಟು));
            ಚೌಕಟ್ಟು->pc                = ಖಂಡ->pc;
            ಚೌಕಟ್ಟು->try_.ಅಂಕೆ_Clauses   = ಖಂಡ->ಅಂಕೆ_Clauses;
            ಚೌಕಟ್ಟು->try_.handlers_ಜೀವಂತ = Handlers_ಜೀವಂತ_At(i_map, i_ಬಿಂದು, ಖಂಡ);
        }
#  endif
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Puts a function's values where this build keeps them at the safepoint
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_ಮೌಲ್ಯಗಳು (M3_Snapshot_ತುಂಬು* l, m3slot_t* i_ತಳ, u32 i_ಅಂಕೆ_Slots, m3stack_t i_sp,
                     ಸ್ಥಿರಾಂಕ M3SnapshotMap* i_map, ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು* i_ಬಿಂದು, ಸ್ಥಿರಾಂಕ M3_ಚೌಕಟ್ಟು_Registers* i_registers)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ    = m3Err_none;
    u32      ಅಂಕೆ_ಮೌಲ್ಯಗಳು = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_ಮೌಲ್ಯಗಳು));
    _throwif("the snapshot's frame does not match what this build compiled", ಅಂಕೆ_ಮೌಲ್ಯಗಳು != i_map->ಅಂಕೆ_Locals + i_ಬಿಂದು->ಅಂಕೆ_ಮೌಲ್ಯಗಳು);

    ಗಾಗಿ (u32 v = 0; v < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++v) {
        ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* ಮೌಲ್ಯ     = ಚೌಕಟ್ಟು_ಮೌಲ್ಯ(i_map, i_ಬಿಂದು, v);
        u64                ಶಬ್ದ      = 0;
        ಶೂನ್ಯ*              reference = ಖಾಲಿ;
        u8*                where;

_       (Locate_ಮೌಲ್ಯ(i_ತಳ, i_ಅಂಕೆ_Slots, i_sp, ಮೌಲ್ಯ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ಆದರೆ (ತಳ_Type_ದ(ಮೌಲ್ಯ->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_ಮೌಲ್ಯ_ಟ್ಯಾಗ್(l, ಮೌಲ್ಯ->type, &type));
            ಆದರೆ (l->checking) {
_               (Skip_ಬೈಟುಗಳು(l, 16));
            } ಇಲ್ಲದಿದ್ದರೆ {
_               (Get(l, where, 16));
            }
            ಮುಂದುವರಿ;
        }

        l->allowUnfilledRef = i_ಬಿಂದು->ಬಗೆ == safepoint_suspend and
                              v >= ಅಂಕೆ_ಮೌಲ್ಯಗಳು - i_ಬಿಂದು->ಅಂಕೆ_Results;
_       (Get_ಮೌಲ್ಯ_ಬಿಟ್ಟುಗಳು(l, ಮೌಲ್ಯ->type, &ಶಬ್ದ));
        l->allowUnfilledRef = false;

        ಆದರೆ (IsRefType(ಮೌಲ್ಯ->type)) {
            reference = (ಶೂನ್ಯ*)(uintptr_t)ಶಬ್ದ;
        }

        ಆದರೆ (not l->checking) {
_           (Write_ಮೌಲ್ಯ(where, i_registers, ಮೌಲ್ಯ, ಶಬ್ದ, reference));
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Suspended_Body (M3_Snapshot_ತುಂಬು* l, IM3Continuation io_cont, u32 i_id)
{
    M3_ಫಲಿತಾಂಶ    ಫಲಿತಾಂಶ    = m3Err_none;
    u32         ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;
    M3_ಚೌಕಟ್ಟು*    call_ಚೌಕಟ್ಟು = ಖಾಲಿ;
    IM3Function function  = ಖಾಲಿ;
    m3slot_t*   ತಳ;
    u32         ಅಂಕೆ_Slots;
    u32         ಅಂಕೆ_Activations = 0;
    IM3FuncType expected       = io_cont->ನಮೂದು_Function->funcType;
    m3stack_t   sp;

_   (Continuation_ರಾಶಿ(l->runtime, io_cont, &ತಳ, &ಅಂಕೆ_Slots));

_   (GetLEB_u32(l, &ಅಂಕೆ_Activations));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Activations == 0 or ಅಂಕೆ_Activations > d_m3_Continuation_Max_ಚೌಕಟ್ಟುಗಳು);

    io_cont->pc                = ಖಾಲಿ;
    io_cont->r0                = 0;
    io_cont->ಅಂಕೆ_Suspend_Results = 0;
    io_cont->suspend_ಬಿಂದು      = safepoint_op;
#  ಆದರೆ d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = ತಳ;

    ಗಾಗಿ (u32 a = 0; a < ಅಂಕೆ_Activations; ++a) {
        bool                 isInnermost = (a + 1 == ಅಂಕೆ_Activations);
        ಸ್ಥಿರಾಂಕ M3_ಸುರಕ್ಷಿತ_ಬಿಂದು*   ಬಿಂದು;
        M3_ಚೌಕಟ್ಟು_Registers     registers;
        u8                   ಬಗೆ       = 0;
        u32                  wasm_ಸ್ಥಾನಾಂತರ = 0, ಗುರಿ_Loop = 0;
        ಸ್ಥಿರಾಂಕ M3SnapshotMap* map;

        memset(&registers, 0, ಗಾತ್ರ(registers));

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
                 (u32)(sp - ತಳ) + function->max_ರಾಶಿ_Slots > ಅಂಕೆ_Slots);

        // the call the frame outside this one is waiting on
        ಆದರೆ (call_ಚೌಕಟ್ಟು) {
            call_ಚೌಕಟ್ಟು->call.function = function;
        }

#  ಆದರೆ d_m3_ನಮೂದು_Keeps_ಚೌಕಟ್ಟು
        {
            M3_ಚೌಕಟ್ಟು* ನಮೂದು;
_           (ಕೂಡಿಸು_ಚೌಕಟ್ಟು(l, &ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು, ಚೌಕಟ್ಟು_ನಮೂದು, sp, &ನಮೂದು));
            ನಮೂದು->pc             = function->compiled;
            ನಮೂದು->ನಮೂದು.function = function;
        }
#  endif

_       (GetU8(l, &ಬಗೆ));
_       (GetLEB_u32(l, &wasm_ಸ್ಥಾನಾಂತರ));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        ಆದರೆ (ಬಗೆ == safepoint_op) {
            ಸ್ಥಿರಾಂಕ M3_ಖಂಡ_ಆರಂಭ* loop;

_           (GetLEB_u32(l, &ಗುರಿ_Loop));

            loop = ಕಂಡುಹಿಡಿ_ಖಂಡ_At(map, c_waOp_loop, ಗುರಿ_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_ಸ್ಥಾನಾಂತರ(loop, wasm_ಸ್ಥಾನಾಂತರ));
        }

        ಬಿಂದು = ಕಂಡುಹಿಡಿ_ಸುರಕ್ಷಿತ_ಬಿಂದು_At(map, wasm_ಸ್ಥಾನಾಂತರ, ಬಗೆ, ಗುರಿ_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not ಬಿಂದು);

_       (ಕೂಡಿಸು_ಖಂಡ_ಚೌಕಟ್ಟುಗಳು(l, map, ಬಿಂದು, sp, &ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು));

        ಆದರೆ (not isInnermost) {
            _throwif(m3Err_wasmMalformed, ಬಗೆ != safepoint_call);

_           (ಕೂಡಿಸು_ಚೌಕಟ್ಟು(l, &ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು, ಚೌಕಟ್ಟು_call, sp, &call_ಚೌಕಟ್ಟು));
            call_ಚೌಕಟ್ಟು->pc = ಬಿಂದು->pc;
            registers.r0  = &call_ಚೌಕಟ್ಟು->call.r0;
#  ಆದರೆ d_m3HasFloat
            registers.fp0 = &call_ಚೌಕಟ್ಟು->call.fp0;
#  endif
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಗೆ == safepoint_resume) {
            M3_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟು;
            u32      ಅಂಕೆ_Results = ಬಿಂದು->ಅಂಕೆ_Results;
            pc_t     resultsPC  = ಬಿಂದು->pc - 2 * ಅಂಕೆ_Results;

_           (ಕೂಡಿಸು_ಚೌಕಟ್ಟು(l, &ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು, ಚೌಕಟ್ಟು_resume, sp, &ಚೌಕಟ್ಟು));
            ಚೌಕಟ್ಟು->pc                 = ಬಿಂದು->pc;
            ಚೌಕಟ್ಟು->resume.resultsPC   = resultsPC;
            ಚೌಕಟ್ಟು->resume.ಅಂಕೆ_Results  = ಅಂಕೆ_Results;
            ಚೌಕಟ್ಟು->resume.ಅಂಕೆ_Handlers = ಬಿಂದು->aux;
            ಚೌಕಟ್ಟು->resume.handlersPC  = resultsPC - 1 - 3 * ಬಿಂದು->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            call_ಚೌಕಟ್ಟು = ಚೌಕಟ್ಟು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            _throwif(m3Err_wasmMalformed, ಬಗೆ != safepoint_op and ಬಗೆ != safepoint_ನಮೂದು and ಬಗೆ != safepoint_suspend);

            io_cont->pc           = ಬಿಂದು->pc;
            io_cont->suspend_ಬಿಂದು = ಬಗೆ;
            registers.r0          = &io_cont->r0;
#  ಆದರೆ d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (ತುಂಬು_ಮೌಲ್ಯಗಳು(l, ತಳ, ಅಂಕೆ_Slots, sp, map, ಬಿಂದು, &registers));

        ಆದರೆ (not l->checking) {
            RestoreConstants(function, sp);
        }

        ಆದರೆ (not isInnermost) {
            sp = sp + ಬಿಂದು->aux;
_           (CallSiteType(function, ಬಿಂದು, &expected));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಗೆ == safepoint_resume) {
            ಶೂನ್ಯ* cont = ಖಾಲಿ;
_           (Get_Ref_ಮೌಲ್ಯ(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ಖಾಲಿ);

            call_ಚೌಕಟ್ಟು->resume.cont = (IM3Continuation)cont;
            ಆದರೆ (not l->checking) {
                l->suspendSites[i_id].ಬಿಂದು = ಬಿಂದು;
                ಗಾಗಿ (u32 i = 1; i < l->ಅಂಕೆ_Continuations; ++i) {
                    ಆದರೆ (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].ಗುರಿ = i;
                        l->suspendSites[i].resumed   = true;
                        ನಿಲ್ಲಿಸು;
                    }
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            ಆದರೆ (ಬಿಂದು->flags & d_m3_ಸುರಕ್ಷಿತ_ಬಿಂದು_ತೆಗೆದುಕೊಂಡ_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            ಆದರೆ (ಬಗೆ == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, ಬಿಂದು->ಅಂಕೆ_Results > ಬಿಂದು->ಅಂಕೆ_ಮೌಲ್ಯಗಳು or
                                                ಬಿಂದು->ಅಂಕೆ_Results > d_m3MaxContinuationPayload);

                io_cont->ಅಂಕೆ_Suspend_Results = ಬಿಂದು->ಅಂಕೆ_Results;
                ಆದರೆ (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].ಬಿಂದು = ಬಿಂದು;
                }

                ಗಾಗಿ (u32 i = 0; i < ಬಿಂದು->ಅಂಕೆ_Results; ++i) {
                    ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* ಮೌಲ್ಯ = &map->ಮೌಲ್ಯಗಳು[ಬಿಂದು->ಮೊದಲ_ಮೌಲ್ಯ + ಬಿಂದು->ಅಂಕೆ_ಮೌಲ್ಯಗಳು - ಬಿಂದು->ಅಂಕೆ_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(ಮೌಲ್ಯ->ಸ್ಥಾನ));

                    io_cont->suspend_ಫಲಿತಾಂಶ_Offsets[i] = ಮೌಲ್ಯ->ಸ್ಥಾನ;
                    io_cont->suspend_ಫಲಿತಾಂಶ_Is64[i]    = Is64_ಬಿಟ್_Type(ಮೌಲ್ಯ->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    ಆದರೆ (l->checking) {
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    // held innermost first
    ಆದರೆ (ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು > io_cont->ಚೌಕಟ್ಟುಗಳು_Cap) {
        M3_ಚೌಕಟ್ಟು* ಚೌಕಟ್ಟುಗಳು = m3_ReallocArray(M3_ಚೌಕಟ್ಟು, io_cont->ಚೌಕಟ್ಟುಗಳು, ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು, io_cont->ಚೌಕಟ್ಟುಗಳು_Cap);
        _throwifnull(ಚೌಕಟ್ಟುಗಳು);

        io_cont->ಚೌಕಟ್ಟುಗಳು    = ಚೌಕಟ್ಟುಗಳು;
        io_cont->ಚೌಕಟ್ಟುಗಳು_Cap = ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು;
    }

    ಗಾಗಿ (u32 k = 0; k < ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು; ++k) {
        io_cont->ಚೌಕಟ್ಟುಗಳು[ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು - 1 - k] = l->ಚೌಕಟ್ಟುಗಳು[k];
    }

    io_cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು;

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Continuation (M3_Snapshot_ತುಂಬು* l, u32 i_id)
{
    M3_ಫಲಿತಾಂಶ        ಫಲಿತಾಂಶ = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    ಆದರೆ (l->checking) {
        scratch          = *cont;
        scratch.val_ರಾಶಿ = l->check_ರಾಶಿ;
        ಆದರೆ (i_id == 0) {
            scratch.val_ರಾಶಿ = (m3slot_t*)l->runtime->origin_ರಾಶಿ;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_ಸೂಚ್ಯಂಕ = 0, ನಮೂದು_ಸೂಚ್ಯಂಕ = 0, bound_ವಾದಗಳು_ಎಣಿಕೆ = 0;
    u8  state = 0, is_ಬೇರು = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_ಬೇರು));
_   (GetLEB_u32(l, &type_ಸೂಚ್ಯಂಕ));
_   (GetLEB_u32(l, &ನಮೂದು_ಸೂಚ್ಯಂಕ));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_ಬೇರು != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_ಬೇರು and state != snapshot_contSuspended);
    ಆದರೆ (not is_ಬೇರು and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_ಸೂಚ್ಯಂಕ != d_m3SnapshotNone and type_ಸೂಚ್ಯಂಕ >= module->ಅಂಕೆ_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, ನಮೂದು_ಸೂಚ್ಯಂಕ >= module->ಅಂಕೆ_Functions);
    _throwif(m3Err_wasmMalformed, is_ಬೇರು ? type_ಸೂಚ್ಯಂಕ != d_m3SnapshotNone : (type_ಸೂಚ್ಯಂಕ == d_m3SnapshotNone or not module->funcTypes[type_ಸೂಚ್ಯಂಕ]->isContinuation));

    ಆದರೆ (not l->checking and not is_ಬೇರು and state != snapshot_cont_ಪೂರ್ಣಗೊಂಡ) {
_       (Continuation_Acquire_ರಾಶಿ(l->runtime, cont));
    }
    cont->type          = (type_ಸೂಚ್ಯಂಕ != d_m3SnapshotNone) ? module->funcTypes[type_ಸೂಚ್ಯಂಕ] : ಖಾಲಿ;
    cont->ನಮೂದು_Function = &module->functions[ನಮೂದು_ಸೂಚ್ಯಂಕ];
    cont->ಅಂಕೆ_Handlers   = 0;
    cont->handlersPC    = ಖಾಲಿ;
    cont->ಪೋಷಕ        = ಖಾಲಿ;
    cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು     = 0;

#  ಆದರೆ d_m3HasExceptionHandling
    {
        ಶೂನ್ಯ* ref = ಖಾಲಿ;
_       (Get_Ref_ಮೌಲ್ಯ(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_ಬೇರು or state == snapshot_cont_ಪೂರ್ಣಗೊಂಡ) and cont->resumeThrow != ಖಾಲಿ);
#  ಇಲ್ಲದಿದ್ದರೆ
    {
        ಶೂನ್ಯ* ref = ಖಾಲಿ;
_       (Get_Ref_ಮೌಲ್ಯ(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ಖಾಲಿ);
    }
#  endif

    ಆದರೆ (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         ಅಂಕೆ_Rets;

_       (GetLEB_u32(l, &bound_ವಾದಗಳು_ಎಣಿಕೆ));
        cont->bound_ವಾದಗಳು_ಎಣಿಕೆ = bound_ವಾದಗಳು_ಎಣಿಕೆ;
        inner                = cont->ನಮೂದು_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_ವಾದಗಳು_ಎಣಿಕೆ > Get_Func_Type_ಅಂಕೆ_Params(inner));

        {
            IM3FuncType ಉಳಿದ = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not ಉಳಿದ or
                                            Get_Func_Type_ಅಂಕೆ_Params(ಉಳಿದ) != Get_Func_Type_ಅಂಕೆ_Params(inner) - bound_ವಾದಗಳು_ಎಣಿಕೆ or
                                            Get_Func_Type_ಅಂಕೆ_Results(ಉಳಿದ) != Get_Func_Type_ಅಂಕೆ_Results(inner));
            ಗಾಗಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ಫಲಿತಾಂಶ_Type(inner, i) != Get_Func_Type_ಫಲಿತಾಂಶ_Type(ಉಳಿದ, i));
            }
            ಗಾಗಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Params(ಉಳಿದ); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_ವಾದಗಳು_ಎಣಿಕೆ + i)) != GetFuncTypeParamType(ಉಳಿದ, i));
            }
        }

        ಅಂಕೆ_Rets = Get_Func_Type_ಅಂಕೆ_Results(inner);

        _throwif(m3Err_wasmMalformed, (ಅಂಕೆ_Rets + bound_ವಾದಗಳು_ಎಣಿಕೆ) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ > cont->ಅಂಕೆ_ರಾಶಿ_Slots);

        ಗಾಗಿ (u32 i = 0; i < bound_ವಾದಗಳು_ಎಣಿಕೆ; ++i) {
_           (Get_ಮೌಲ್ಯ(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ರಾಶಿ + (ಅಂಕೆ_Rets + i) * c_io_ಸ್ಥಾನ_ಎಣಿಕೆ));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ರಾಶಿ;
        cont->pc    = cont->ನಮೂದು_Function->compiled;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_ವಾದಗಳು_ಎಣಿಕೆ));
        _throwif(m3Err_wasmMalformed, is_ಬೇರು and bound_ವಾದಗಳು_ಎಣಿಕೆ != 0);
        cont->bound_ವಾದಗಳು_ಎಣಿಕೆ = bound_ವಾದಗಳು_ಎಣಿಕೆ;

_       (ತುಂಬು_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (state == snapshot_cont_ಪೂರ್ಣಗೊಂಡ) {
_       (GetLEB_u32(l, &bound_ವಾದಗಳು_ಎಣಿಕೆ));
        _throwif(m3Err_wasmMalformed, bound_ವಾದಗಳು_ಎಣಿಕೆ != 0);
        cont->bound_ವಾದಗಳು_ಎಣಿಕೆ = 0;
        cont->state          = cont_consumed;
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ಆದರೆ (ಫಲಿತಾಂಶ) {
        cont->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Continuations (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u32      ಎಣಿಕೆ_2  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &ಎಣಿಕೆ_2));
    _throwif(m3Err_wasmMalformed, ಎಣಿಕೆ_2 == 0 or ಎಣಿಕೆ_2 != l->ಅಂಕೆ_Continuations);

    ಗಾಗಿ (u32 i = 0; i < ಎಣಿಕೆ_2; ++i) {
_       (ತುಂಬು_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ CheckContinuationReferences (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಗಾಗಿ (u32 id = 0; id < l->ಅಂಕೆ_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             ಎಲೆ  = id;
        u32             ಆಳ = 0;
#  ಆದರೆ d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_ದ(Ref_Type_ದ_Func_Type(cont->type, true), expectedType)));
#  endif
        ಆದರೆ (l->suspendSites[id].ಗುರಿ) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_ದ_Ref(l->module, l->suspendSites[id].ಬಿಂದು->resumeType);
            IM3Continuation ಮಗು    = l->continuations[l->suspendSites[id].ಗುರಿ];
            IM3FuncType     actual   = ಮಗು->ನಮೂದು_Function ? ಮಗು->ನಮೂದು_Function->funcType : ಖಾಲಿ;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ಅಂಕೆ_Results(actual) != Get_Func_Type_ಅಂಕೆ_Results(expected));
            ಗಾಗಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ಫಲಿತಾಂಶ_Type(actual, i) != Get_Func_Type_ಫಲಿತಾಂಶ_Type(expected, i));
            }
        }
        ಆಗ (l->suspendSites[ಎಲೆ].ಗುರಿ) {
            ಎಲೆ = l->suspendSites[ಎಲೆ].ಗುರಿ;
            _throwif(m3Err_wasmMalformed, ++ಆಳ >= l->ಅಂಕೆ_Continuations or
                                            l->continuations[ಎಲೆ]->state != cont_suspended);
        }
        ಆದರೆ (cont->state != cont_suspended or id == 0) {
            ಮುಂದುವರಿ;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        ಆದರೆ (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_ವಾದಗಳು_ಎಣಿಕೆ != 0);
            ಮುಂದುವರಿ;
        }

        ಸ್ಥಿರಾಂಕ M3SnapshotSuspendSite* site      = &l->suspendSites[ಎಲೆ];
        IM3Continuation              inner     = l->continuations[ಎಲೆ];
        IM3FuncType                  ಉಳಿದ = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->ಬಿಂದು or not ಉಳಿದ);
        _throwif("a captured continuation has the wrong binding count", cont->bound_ವಾದಗಳು_ಎಣಿಕೆ > inner->ಅಂಕೆ_Suspend_Results or
                                                                          Get_Func_Type_ಅಂಕೆ_Params(ಉಳಿದ) != inner->ಅಂಕೆ_Suspend_Results - cont->bound_ವಾದಗಳು_ಎಣಿಕೆ);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_ಅಂಕೆ_Results(ಉಳಿದ) != Get_Func_Type_ಅಂಕೆ_Results(cont->ನಮೂದು_Function->funcType));
        ಗಾಗಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Results(ಉಳಿದ); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ಫಲಿತಾಂಶ_Type(ಉಳಿದ, i) !=
                                            Get_Func_Type_ಫಲಿತಾಂಶ_Type(cont->ನಮೂದು_Function->funcType, i));
        }

        ಗಾಗಿ (u32 i = 0; i < inner->ಅಂಕೆ_Suspend_Results; ++i) {
            ಸ್ಥಿರಾಂಕ M3_ಸ್ಥಾನ_ಮೌಲ್ಯ* ಮೌಲ್ಯ = &site->map->ಮೌಲ್ಯಗಳು[site->ಬಿಂದು->ಮೊದಲ_ಮೌಲ್ಯ +
                                                          site->ಬಿಂದು->ಅಂಕೆ_ಮೌಲ್ಯಗಳು - site->ಬಿಂದು->ಅಂಕೆ_Results + i];
            ಆದರೆ (i < cont->bound_ವಾದಗಳು_ಎಣಿಕೆ) {
                ಆದರೆ (IsRefType(ಮೌಲ್ಯ->type) and not IsNullableRef(ಮೌಲ್ಯ->type)) {
                    ಶೂನ್ಯ* ref = ಖಾಲಿ;
                    ಸ್ಮರಣೆ_ನಕಲು(&ref, inner->sp + ಮೌಲ್ಯ->ಸ್ಥಾನ, ಗಾತ್ರ(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                _throwif("a captured continuation has the wrong parameter type", ಮೌಲ್ಯ->type !=
                                                                                   GetFuncTypeParamType(ಉಳಿದ, (u16)(i - cont->bound_ವಾದಗಳು_ಎಣಿಕೆ)));
            }
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#  ಆದರೆ d_m3HasExceptionHandling

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Exceptions (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ        = m3Err_none;
    u32      ಅಂಕೆ_Exceptions = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Exceptions));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Exceptions == 0 or ಅಂಕೆ_Exceptions != l->ಅಂಕೆ_Exceptions);

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Exceptions; ++i) {
        u32          exnId = 0, ಅಂಕೆ_ವಾದಗಳು = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &ಅಂಕೆ_ವಾದಗಳು));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->ಟ್ಯಾಗ್->type;

        _throwif(m3Err_wasmMalformed, ಅಂಕೆ_ವಾದಗಳು != exn->ಅಂಕೆ_ವಾದಗಳು);

        ಗಾಗಿ (u32 a = 0; a < ಅಂಕೆ_ವಾದಗಳು; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      ಬಿಟ್ಟುಗಳು    = 0;
_           (Get_ಮೌಲ್ಯ_ಬಿಟ್ಟುಗಳು(l, argType, &ಬಿಟ್ಟುಗಳು));
            ಆದರೆ (not l->checking) {
                exn->ವಾದಗಳು[a] = ಬಿಟ್ಟುಗಳು;
            }
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3BoundedReader {
    M3_Snapshot_ತುಂಬು* ತುಂಬು;
    u64             ಉಳಿದ;
    M3_ಫಲಿತಾಂಶ        error;
} M3BoundedReader;

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ BoundedReader_Read (ಶೂನ್ಯ* o_ಬಫರ್, size_t i_ಗಾತ್ರ, ಶೂನ್ಯ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    ಆದರೆ (i_ಗಾತ್ರ > br->ಉಳಿದ) {
        br->error = "the host read past the end of its state";
        ಹಿಂತಿರುಗಿ br->error;
    }

    br->ಉಳಿದ -= i_ಗಾತ್ರ;

    ಹಿಂತಿರುಗಿ Get(br->ತುಂಬು, o_ಬಫರ್, i_ಗಾತ್ರ);
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Host_State (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ               ಫಲಿತಾಂಶ = m3Err_none;
    ಸ್ಥಿರಾಂಕ M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 ಗಾತ್ರ_2 = (size_t)(l->ಕೊನೆ - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->ತುಂಬು_Host_State_2);

    ಆದರೆ (l->checking) {
        ಹಿಂತಿರುಗಿ Skip_ಬೈಟುಗಳು(l, ಗಾತ್ರ_2);
    }
    reader.ತುಂಬು      = l;
    reader.ಉಳಿದ = ಗಾತ್ರ_2;
    reader.error     = m3Err_none;

_   (hooks->ತುಂಬು_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, ಗಾತ್ರ_2));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.ಉಳಿದ != 0);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_ಶೀರ್ಷಿಕೆ (M3_Snapshot_ತುಂಬು* l)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u8       magic[ಗಾತ್ರ(c_snapshotMagic)];
    u32      ಆವೃತ್ತಿ = 0;

_   (Get(l, magic, ಗಾತ್ರ(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, ಗಾತ್ರ(magic)) != 0);

_   (GetLE32(l, &ಆವೃತ್ತಿ));
    _throwif(m3Err_wasmMalformed, ಆವೃತ್ತಿ != c_snapshot_ಆವೃತ್ತಿ);

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Meta (M3_Snapshot_ತುಂಬು* l, u32* o_flags)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      ಸ್ಮರಣೆ_ಬೈಟುಗಳು = 0, ಕೋಷ್ಟಕ_Elements = 0;
    u32      ಸಕ್ರಿಯ_Stacks = 0;
    u64      current_ಸ್ಮರಣೆ, currentTables;
    SnapshotResourceTotals(l->module, &current_ಸ್ಮರಣೆ, &currentTables);

_   (GetLEB_u32(l, o_flags));
_   (GetLEB_u64(l, &timestamp));
_   (GetLEB_u64(l, &moduleHash));

    (ಶೂನ್ಯ)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->ಅಂಕೆ_Continuations));
_   (GetLEB_u32(l, &l->ಅಂಕೆ_Exceptions));
_   (GetLEB_u64(l, &ಸ್ಮರಣೆ_ಬೈಟುಗಳು));
_   (GetLEB_u64(l, &ಕೋಷ್ಟಕ_Elements));
_   (GetLEB_u32(l, &ಸಕ್ರಿಯ_Stacks));
    l->requiredStacks        = ಸಕ್ರಿಯ_Stacks;
    l->required_ಸ್ಮರಣೆ_ಬೈಟುಗಳು   = ಸ್ಮರಣೆ_ಬೈಟುಗಳು;
    l->required_ಕೋಷ್ಟಕ_Elements = ಕೋಷ್ಟಕ_Elements;
    _throwif(m3Err_wasmMalformed, ಸ್ಮರಣೆ_ಬೈಟುಗಳು < current_ಸ್ಮರಣೆ or ಕೋಷ್ಟಕ_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->ಅಂಕೆ_Memories and ಸ್ಮರಣೆ_ಬೈಟುಗಳು != 0);
    _throwif(m3Err_wasmMalformed, not l->module->ಅಂಕೆ_Tables and ಕೋಷ್ಟಕ_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    ಆದರೆ (*o_flags & d_m3SnapshotFlagPostmortem) {
        ಗಾಗಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
            u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ = 0;
_           (GetLEB_u32(l, &ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ));
        }
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    _throwif(m3_Err_ಸ್ಮರಣೆ_ಎಲ್ಲೆ_Exceeded, l->runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ and
                                          ಸ್ಮರಣೆ_ಬೈಟುಗಳು - current_ಸ್ಮರಣೆ > l->runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಎಲ್ಲೆ - l->runtime->ಸ್ಮರಣೆ_ಬೈಟುಗಳು_ಬಳಸಿದ);
    _throwif(m3_Err_ಕೋಷ್ಟಕ_ಎಲ್ಲೆ_Exceeded, l->runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ and
                                         ಕೋಷ್ಟಕ_Elements - currentTables > l->runtime->ಕೋಷ್ಟಕ_Elements_ಎಲ್ಲೆ - l->runtime->ಕೋಷ್ಟಕ_Elements_ಬಳಸಿದ);
    _throwif(m3_Err_continuation_ಎಲ್ಲೆ_Exceeded, l->runtime->continuations_ಎಲ್ಲೆ and
                                                ಸಕ್ರಿಯ_Stacks > l->runtime->continuations_ಎಲ್ಲೆ - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->ಅಂಕೆ_Continuations == 0 or ಸಕ್ರಿಯ_Stacks >= l->ಅಂಕೆ_Continuations);
#  ಆದರೆ !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->ಅಂಕೆ_Exceptions != 0);
#  endif

    ಆದರೆ (l->checking) {
        ಗಾಗಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
            u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ = 0;
_           (GetLEB_u32(l, &ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ));
#  ಆದರೆ d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= l->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);
#  endif
        }
        ಹಿಂತಿರುಗಿ m3Err_none;
    }

    l->check_ರಾಶಿ = m3_AllocArray(m3slot_t, d_m3_Continuation_ರಾಶಿ_Slots + 4);
    _throwifnull(l->check_ರಾಶಿ);
    l->ಚೌಕಟ್ಟುಗಳು = m3_AllocArray(M3_ಚೌಕಟ್ಟು, d_m3_Continuation_Max_ಚೌಕಟ್ಟುಗಳು);
    _throwifnull(l->ಚೌಕಟ್ಟುಗಳು);

    l->continuations = m3_AllocArray(IM3Continuation, l->ಅಂಕೆ_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->ಅಂಕೆ_Continuations);
    _throwifnull(l->suspendSites);

    ಆದರೆ (not l->runtime->ಬೇರು_Continuation) {
        IM3Continuation ಬೇರು = Continuation_New(l->runtime, ಖಾಲಿ, ಖಾಲಿ);
        _throwifnull(ಬೇರು);

        // the root borrows the runtime's stack, so it holds none of its own
        ಬೇರು->ಅಂಕೆ_ರಾಶಿ_Slots          = l->runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
        l->runtime->ಬೇರು_Continuation = ಬೇರು;
    }

    l->continuations[0] = l->runtime->ಬೇರು_Continuation;

    ಗಾಗಿ (u32 i = 1; i < l->ಅಂಕೆ_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ಖಾಲಿ, ಖಾಲಿ);
        _throwifnull(l->continuations[i]);
    }

#  ಆದರೆ d_m3HasExceptionHandling
    ಆದರೆ (l->ಅಂಕೆ_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->ಅಂಕೆ_Exceptions);
        _throwifnull(l->exceptions);
    }

    ಗಾಗಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
        u32    ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ = 0;
        IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್;
        u32    ಅಂಕೆ_ವಾದಗಳು;

_       (GetLEB_u32(l, &ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ));
        _throwif(m3Err_wasmMalformed, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ >= l->module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

        ಟ್ಯಾಗ್     = ಟ್ಯಾಗ್_ದ_ಸೂಚ್ಯಂಕ(l->module, ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ);
        ಅಂಕೆ_ವಾದಗಳು = Get_Func_Type_ಅಂಕೆ_Params(ಟ್ಯಾಗ್->type);

        l->exceptions[i] = NewException(l->runtime, ಟ್ಯಾಗ್, ಅಂಕೆ_ವಾದಗಳು);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  ಇಲ್ಲದಿದ್ದರೆ
    ಗಾಗಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
        u32 ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ = 0;
_       (GetLEB_u32(l, &ಟ್ಯಾಗ್_ಸೂಚ್ಯಂಕ));
    }
#  endif

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Section (M3_Snapshot_ತುಂಬು* l, u8 i_sectionId)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    u32      flags  = 0;
    ಆಯ್ಕೆ (i_sectionId) {
    ಸ್ಥಿತಿ d_m3SnapshotSection_Meta:
_       (ತುಂಬು_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_m3_Snapshot_Section_ಸ್ಮರಣೆ:
_       (ತುಂಬು_Memories(l));
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_m3_Snapshot_Section_ಕೋಷ್ಟಕ:
_       (ತುಂಬು_Tables(l));
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_m3SnapshotSection_Global:
_       (ತುಂಬು_Globals(l));
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_m3SnapshotSection_Segment:
_       (ತುಂಬು_Segments(l));
        ನಿಲ್ಲಿಸು;

#  ಆದರೆ d_m3HasExceptionHandling
    ಸ್ಥಿತಿ d_m3SnapshotSection_Exception:
_       (ತುಂಬು_Exceptions(l));
        ನಿಲ್ಲಿಸು;
#  endif

    ಸ್ಥಿತಿ d_m3SnapshotSection_Continuation:
_       (ತುಂಬು_Continuations(l));
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_m3SnapshotSection_HostState:
_       (ತುಂಬು_Host_State(l));
        ನಿಲ್ಲಿಸು;

    ಪೂರ್ವನಿಯೋಜಿತ:
_       (Skip_ಬೈಟುಗಳು(l, (size_t)(l->ಕೊನೆ - l->pos)));
        ನಿಲ್ಲಿಸು;
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Sections (M3_Snapshot_ತುಂಬು* l, bool i_embedded)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ  = m3Err_none;
    u8*      payload = ಖಾಲಿ;

    ಆದರೆ (not i_embedded) {
_       (ತುಂಬು_ಶೀರ್ಷಿಕೆ(l));
    }

    ಆಗ (true) {
        u8  sectionId   = 0;
        u32 section_ಗಾತ್ರ = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        ಆದರೆ (GetU8(l, &sectionId)) {
            ನಿಲ್ಲಿಸು;
        }

_       (GetLEB_u32(l, &section_ಗಾತ್ರ));

        _throwif("the snapshot holds a section twice", l->ನೋಡಿದ_Sections[sectionId]);
        l->ನೋಡಿದ_Sections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_ಗಾತ್ರ == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_ಗಾತ್ರ);
        _throwifnull(payload);
_       (Get(l, payload, section_ಗಾತ್ರ));
        l->pos       = payload;
        l->ಕೊನೆ       = payload + section_ಗಾತ್ರ;
        l->ಒಳ_Section = true;

        {
            M3_Snapshot_ತುಂಬು check = *l;
            check.checking       = true;
_           (ತುಂಬು_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.ಕೊನೆ);
        }
        l->ಬದಲಾಯಿತು = l->ಬದಲಾಯಿತು or sectionId != d_m3SnapshotSection_Meta;
_       (ತುಂಬು_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->ಕೊನೆ);
        l->ಒಳ_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->ನೋಡಿದ_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->ಅಂಕೆ_Exceptions and not l->ನೋಡಿದ_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->ಅಂಕೆ_Memories and not l->ನೋಡಿದ_Sections[d_m3_Snapshot_Section_ಸ್ಮರಣೆ]);
    _throwif("the snapshot has no table section",
             l->module->ಅಂಕೆ_Tables and not l->ನೋಡಿದ_Sections[d_m3_Snapshot_Section_ಕೋಷ್ಟಕ]);
    _throwif("the snapshot has no global section",
             l->module->ಅಂಕೆ_Globals and not l->ನೋಡಿದ_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments or l->module->ಅಂಕೆ_Element_Segments) and
               not l->ನೋಡಿದ_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#ಆದರೆ d_m3HasSnapshots

ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ಶೂನ್ಯ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_ಫಲಿತಾಂಶ       ಫಲಿತಾಂಶ = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, ಗಾತ್ರ(save));

    IM3Continuation ಬೇರು = io_runtime->ಬೇರು_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not ಬೇರು or ಬೇರು->state != cont_suspended;

    IM3Function ನಮೂದು = save.postmortem ? io_runtime->entered : ಬೇರು->ನಮೂದು_Function;

    ಆದರೆ (o_module) {
        *o_module = ಖಾಲಿ;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not ನಮೂದು);

    save.module = ನಮೂದು->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    ಆದರೆ (o_module) {
        *o_module = save.module;
    }

    ಗಾಗಿ (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->ಮುಂದಿನ) {
        u32 id;
_       (ಸೂಚಕ_Ids_ಕೂಡಿಸು(&save.ಜೀವಂತ_Continuations, cont, &id));
    }

#  ಆದರೆ d_m3HasExceptionHandling
    ಗಾಗಿ (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->ಮುಂದಿನ) {
        u32 id;
_       (ಸೂಚಕ_Ids_ಕೂಡಿಸು(&save.ಜೀವಂತ_Exceptions, exception, &id));
    }
#  endif

    ಆದರೆ (not save.postmortem) {
        u32 id;
_       (ಸೂಚಕ_Ids_ಕೂಡಿಸು(&save.continuations, ಬೇರು, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    ಸೂಚಕ_Ids_Free(&save.ಜೀವಂತ_Continuations);
    ಸೂಚಕ_Ids_Free(&save.ಜೀವಂತ_Exceptions);
    ಸೂಚಕ_Ids_Free(&save.continuations);
    ಸೂಚಕ_Ids_Free(&save.exceptions);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ

M3_ಫಲಿತಾಂಶ m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ಶೂನ್ಯ* i_userdata)
{
    ಆದರೆ (not io_runtime or not i_writer) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }

#ಆದರೆ d_m3HasSnapshots
    ಹಿಂತಿರುಗಿ SaveSnapshot(io_runtime, i_writer, i_userdata, ಖಾಲಿ, false);
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_userdata;
    ಹಿಂತಿರುಗಿ "snapshots are not available in this build of Wasm3";
#ಆದರೆ_ಅಂತ್ಯ
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ತುಂಬು_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ಶೂನ್ಯ* i_userdata, bool i_embedded)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#ಆದರೆ d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_ಸತತ);

    {
        M3_Snapshot_ತುಂಬು ತುಂಬು;
        memset(&ತುಂಬು, 0, ಗಾತ್ರ(ತುಂಬು));

        ತುಂಬು.runtime  = io_runtime;
        ತುಂಬು.module   = i_module;
        ತುಂಬು.reader   = i_reader;
        ತುಂಬು.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        ಫಲಿತಾಂಶ = ತುಂಬು_Sections(&ತುಂಬು, i_embedded);

        IM3Continuation ಬೇರು = io_runtime->ಬೇರು_Continuation;

        ಆದರೆ (ಫಲಿತಾಂಶ) {
            ಆದರೆ (ತುಂಬು.ಬದಲಾಯಿತು) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                ಆದರೆ (ಬೇರು) {
                    ಬೇರು->state     = cont_returned;
                    ಬೇರು->ಅಂಕೆ_ಚೌಕಟ್ಟುಗಳು = 0;
                }
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->ಆರಂಭ_Function = -1;
            i_module->has_ಸತತ        = true;
        }

        Runtime_ಇರಿಸು_Call_ರಾಶಿ(io_runtime);

        m3_Free(ತುಂಬು.check_ರಾಶಿ);
        m3_Free(ತುಂಬು.ಚೌಕಟ್ಟುಗಳು);
        m3_Free(ತುಂಬು.suspendSites);
        m3_Free(ತುಂಬು.continuations);
#  ಆದರೆ d_m3HasExceptionHandling
        m3_Free(ತುಂಬು.exceptions);
#  endif
    }
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#ಆದರೆ_ಅಂತ್ಯ

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ m3_ತುಂಬು_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ಶೂನ್ಯ* i_userdata)
{
    ಹಿಂತಿರುಗಿ ತುಂಬು_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


ಶೂನ್ಯ m3_ಗಣ_Snapshot_Hooks (IM3Runtime io_runtime, ಸ್ಥಿರಾಂಕ M3SnapshotHooks* i_hooks)
{
#ಆದರೆ d_m3HasSnapshots
    ಆದರೆ (io_runtime) {
        ಆದರೆ (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } ಇಲ್ಲದಿದ್ದರೆ {
            memset(&io_runtime->snapshotHooks, 0, ಗಾತ್ರ(io_runtime->snapshotHooks));
        }
    }
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)io_runtime;
    (ಶೂನ್ಯ)i_hooks;
#ಆದರೆ_ಅಂತ್ಯ
}


M3_ಫಲಿತಾಂಶ m3_Save_Snapshot_ಗಮ್ಯ_ಬಫರ್ (IM3Runtime io_runtime, ಶೂನ್ಯ** o_ಬೈಟುಗಳು, size_t* o_ಗಾತ್ರ)
{
    ಆದರೆ (!io_runtime || !o_ಬೈಟುಗಳು || !o_ಗಾತ್ರ) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }
#ಆದರೆ d_m3HasSnapshots
    M3_ಬಫರ್_Writer bw     = { ಖಾಲಿ, 0, 0 };
    M3_ಫಲಿತಾಂಶ       ಫಲಿತಾಂಶ = m3_SaveSnapshot(io_runtime, ಬಫರ್_Writer_Write, &bw);
    ಆದರೆ (ಫಲಿತಾಂಶ) {
        m3_Free(bw.ಬಫರ್);
        *o_ಬೈಟುಗಳು = ಖಾಲಿ;
        *o_ಗಾತ್ರ  = 0;
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
    *o_ಬೈಟುಗಳು = bw.ಬಫರ್;
    *o_ಗಾತ್ರ  = bw.ಗಾತ್ರ_2;
    ಹಿಂತಿರುಗಿ m3Err_none;
#ಇಲ್ಲದಿದ್ದರೆ
    *o_ಬೈಟುಗಳು = ಖಾಲಿ;
    *o_ಗಾತ್ರ  = 0;
    ಹಿಂತಿರುಗಿ "snapshots are not available in this build of Wasm3";
#ಆದರೆ_ಅಂತ್ಯ
}

#ಆದರೆ d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ಸ್ಥಿರ
u8* EmitLEB_u32 (u8* o_where, u32 i_ಮೌಲ್ಯ)
{
    ಮಾಡು {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಆದರೆ (i_ಮೌಲ್ಯ != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } ಆಗ (i_ಮೌಲ್ಯ != 0);

    ಹಿಂತಿರುಗಿ o_where;
}

ಸ್ಥಿರ
u32 ಗಾತ್ರ_ದ_LEB_u32 (u32 i_ಮೌಲ್ಯ)
{
    u32 ಗಾತ್ರ_2 = 0;

    ಮಾಡು {
        i_ಮೌಲ್ಯ >>= 7;
        ++ಗಾತ್ರ_2;
    } ಆಗ (i_ಮೌಲ್ಯ != 0);

    ಹಿಂತಿರುಗಿ ಗಾತ್ರ_2;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ಕಂಡುಹಿಡಿ_Embedded_Snapshot (ಸ್ಥಿರಾಂಕ u8* i_ಆರಂಭ, ಸ್ಥಿರಾಂಕ u8* i_ಕೊನೆ, cstr_t i_secName,
                               ಸ್ಥಿರಾಂಕ u8** o_sec_ಆರಂಭ, ಸ್ಥಿರಾಂಕ u8** o_sec_ಕೊನೆ,
                               ಸ್ಥಿರಾಂಕ u8** o_ದತ್ತಾಂಶ, u32* o_ಗಾತ್ರ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    ಬೈಟುಗಳು_t  pos    = i_ಆರಂಭ;
    bool     ಸಿಕ್ಕಿತು  = false;
    cstr_t   name   = ಖಾಲಿ;

    ಆದರೆ (o_sec_ಆರಂಭ) {
        *o_sec_ಆರಂಭ = ಖಾಲಿ;
    }
    ಆದರೆ (o_sec_ಕೊನೆ) {
        *o_sec_ಕೊನೆ = ಖಾಲಿ;
    }
    ಆದರೆ (o_ದತ್ತಾಂಶ) {
        *o_ದತ್ತಾಂಶ = ಖಾಲಿ;
    }
    ಆದರೆ (o_ಗಾತ್ರ) {
        *o_ಗಾತ್ರ = 0;
    }

    _throwif(m3Err_wasmMalformed, i_ಕೊನೆ - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    ಆಗ (pos < i_ಕೊನೆ) {
        ಬೈಟುಗಳು_t sec_ಆರಂಭ  = pos;
        u8      sectionId = *pos++;
        u32     ಗಾತ್ರ_2      = 0;
_       (ReadLEB_u32(&ಗಾತ್ರ_2, &pos, i_ಕೊನೆ));
        _throwif(m3Err_wasmUnderrun, ಗಾತ್ರ_2 > (size_t)(i_ಕೊನೆ - pos));
        {
            ಬೈಟುಗಳು_t payload = pos;
            pos += ಗಾತ್ರ_2;
            ಆದರೆ (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                ಆದರೆ (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", ಸಿಕ್ಕಿತು);
                    ಸಿಕ್ಕಿತು = true;
                    ಆದರೆ (o_sec_ಆರಂಭ) {
                        *o_sec_ಆರಂಭ = sec_ಆರಂಭ;
                    }
                    ಆದರೆ (o_sec_ಕೊನೆ) {
                        *o_sec_ಕೊನೆ = pos;
                    }
                    ಆದರೆ (o_ದತ್ತಾಂಶ) {
                        *o_ದತ್ತಾಂಶ = payload;
                    }
                    ಆದರೆ (o_ಗಾತ್ರ) {
                        *o_ಗಾತ್ರ = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Builds the custom section name a snapshot of i_name lives under
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SnapshotSectionName (cstr_t i_name, ಅಕ್ಷರ* o_secName, size_t i_ಗಾತ್ರ)
{
    ಆದರೆ (i_name and *i_name) {
        ಆದರೆ (ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(i_name) > d_m3_Max_Snapshot_Name_ಉದ್ದಳತೆ) {
            ಹಿಂತಿರುಗಿ "the snapshot name is too long";
        }
        snprintf(o_secName, i_ಗಾತ್ರ, "snapshot.%s", i_name);
    } ಇಲ್ಲದಿದ್ದರೆ {
        snprintf(o_secName, i_ಗಾತ್ರ, "snapshot");
    }

    ಹಿಂತಿರುಗಿ m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ಖಾಲಿ;

    ಗಾಗಿ (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->ಮುಂದಿನ) {
        ಆದರೆ (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
#ಆದರೆ_ಅಂತ್ಯ

M3_ಫಲಿತಾಂಶ m3_ತುಂಬು_Snapshot_From_ಬಫರ್ (IM3Runtime io_runtime, IM3Module i_module, ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* i_ಬೈಟುಗಳು, size_t i_ಗಾತ್ರ)
{
    ಆದರೆ (!io_runtime || !i_module || !i_ಬೈಟುಗಳು) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }
#ಆದರೆ d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    ಆದರೆ (i_ಗಾತ್ರ >= 8 && memcmp(i_ಬೈಟುಗಳು, "\0asm", 4) == 0) {
        ಸ್ಥಿರಾಂಕ u8* ದತ್ತಾಂಶ   = ಖಾಲಿ;
        u32       ಗಾತ್ರ_2   = 0;
        M3_ಫಲಿತಾಂಶ  ಫಲಿತಾಂಶ = ಕಂಡುಹಿಡಿ_Embedded_Snapshot((ಸ್ಥಿರಾಂಕ u8*)i_ಬೈಟುಗಳು, (ಸ್ಥಿರಾಂಕ u8*)i_ಬೈಟುಗಳು + i_ಗಾತ್ರ,
                                                "snapshot", ಖಾಲಿ, ಖಾಲಿ, &ದತ್ತಾಂಶ, &ಗಾತ್ರ_2);
        ಆದರೆ (ಫಲಿತಾಂಶ) {
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        ಆದರೆ (not ದತ್ತಾಂಶ) {
            ಹಿಂತಿರುಗಿ "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_ಬಫರ್_Reader br = { ದತ್ತಾಂಶ, ಗಾತ್ರ_2, 0 };
            ಹಿಂತಿರುಗಿ ತುಂಬು_Snapshot(io_runtime, i_module, ಬಫರ್_Reader_Read, &br, true);
        }
    }
    {
        M3_ಬಫರ್_Reader br = { (ಸ್ಥಿರಾಂಕ u8*)i_ಬೈಟುಗಳು, i_ಗಾತ್ರ, 0 };
        ಹಿಂತಿರುಗಿ ತುಂಬು_Snapshot(io_runtime, i_module, ಬಫರ್_Reader_Read, &br,
                            i_ಗಾತ್ರ < 4 or memcmp(i_ಬೈಟುಗಳು, c_snapshotMagic, 4) != 0);
    }
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_ಗಾತ್ರ;
    ಹಿಂತಿರುಗಿ "snapshots are not available in this build of Wasm3";
#ಆದರೆ_ಅಂತ್ಯ
}

bool m3_HasSnapshot (IM3Module i_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_name)
{
    ಆದರೆ (not i_module) {
        ಹಿಂತಿರುಗಿ false;
    }
#ಆದರೆ d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ಖಾಲಿ;
    (ಶೂನ್ಯ)ModuleSnapshot(i_module, i_name, &snap);

    ಹಿಂತಿರುಗಿ snap != ಖಾಲಿ;
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_name;
    ಹಿಂತಿರುಗಿ false;
#ಆದರೆ_ಅಂತ್ಯ
}

M3_ಫಲಿತಾಂಶ m3_GetEmbeddedSnapshot (IM3Module i_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_name,
                                 ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ** o_ಬೈಟುಗಳು, size_t* o_ಗಾತ್ರ)
{
    ಆದರೆ (not i_module or not o_ಬೈಟುಗಳು or not o_ಗಾತ್ರ) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }

    *o_ಬೈಟುಗಳು = ಖಾಲಿ;
    *o_ಗಾತ್ರ  = 0;

#ಆದರೆ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ಖಾಲಿ;
        M3_ಫಲಿತಾಂಶ            ಫಲಿತಾಂಶ = ModuleSnapshot(i_module, i_name, &snap);

        ಆದರೆ (ಫಲಿತಾಂಶ) {
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        ಆದರೆ (not snap) {
            ಹಿಂತಿರುಗಿ "the module holds no embedded snapshot of that name";
        }

        *o_ಬೈಟುಗಳು = snap->ದತ್ತಾಂಶ;
        *o_ಗಾತ್ರ  = snap->ಗಾತ್ರ_2;

        ಹಿಂತಿರುಗಿ m3Err_none;
    }
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_name;
    ಹಿಂತಿರುಗಿ "snapshots are not available in this build of Wasm3";
#ಆದರೆ_ಅಂತ್ಯ
}

M3_ಫಲಿತಾಂಶ m3_ತುಂಬು_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_name)
{
    ಆದರೆ (not io_runtime or not i_module) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }
#ಆದರೆ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ಖಾಲಿ;
        M3_ಫಲಿತಾಂಶ            ಫಲಿತಾಂಶ = ModuleSnapshot(i_module, i_name, &snap);
        M3_ಬಫರ್_Reader      br;

        ಆದರೆ (ಫಲಿತಾಂಶ) {
            ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
        }
        ಆದರೆ (not snap) {
            ಹಿಂತಿರುಗಿ "the module holds no embedded snapshot of that name";
        }

        br.ಬಫರ್ = snap->ದತ್ತಾಂಶ;
        br.ಗಾತ್ರ_2   = snap->ಗಾತ್ರ_2;
        br.cursor = 0;

        ಹಿಂತಿರುಗಿ ತುಂಬು_Snapshot(io_runtime, i_module, ಬಫರ್_Reader_Read, &br, true);
    }
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)i_name;
    ಹಿಂತಿರುಗಿ "snapshots are not available in this build of Wasm3";
#ಆದರೆ_ಅಂತ್ಯ
}


M3_ಫಲಿತಾಂಶ m3_Save_Snapshot_ಗಮ್ಯ_Module (IM3Runtime io_runtime, IM3Module i_module, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* i_name,
                                  ಶೂನ್ಯ** o_ಬೈಟುಗಳು, size_t* o_ಗಾತ್ರ)
{
#ಆದರೆ d_m3HasSnapshots
    M3_ಫಲಿತಾಂಶ       ಫಲಿತಾಂಶ = m3Err_none;
    M3_ಬಫರ್_Writer bw     = { ಖಾಲಿ, 0, 0 };
    u8*            ಹೊರಹೋಗು    = ಖಾಲಿ;

    ಅಕ್ಷರ      secName[d_m3_Max_Snapshot_Name_ಉದ್ದಳತೆ + ಗಾತ್ರ("snapshot.")];
    u32       nameLen, name_Len_Leb_ಗಾತ್ರ, payload_ಗಾತ್ರ, payload_ಗಾತ್ರ_Leb_ಗಾತ್ರ;
    size_t    orig_ಗಾತ್ರ, ಒಟ್ಟು_ಗಾತ್ರ;
    ಸ್ಥಿರಾಂಕ u8* drop_ಆರಂಭ = ಖಾಲಿ;
    ಸ್ಥಿರಾಂಕ u8* drop_ಕೊನೆ   = ಖಾಲಿ;
    IM3Module saved     = ಖಾಲಿ;
    u8*       p;

    ಆದರೆ (not io_runtime or not i_module or not o_ಬೈಟುಗಳು or not o_ಗಾತ್ರ) {
        ಹಿಂತಿರುಗಿ m3Err_mallocFailed;
    }

    *o_ಬೈಟುಗಳು = ಖಾಲಿ;
    *o_ಗಾತ್ರ  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_ಆರಂಭ or i_module->wasm_ಕೊನೆ <= i_module->wasm_ಆರಂಭ);

_   (SnapshotSectionName(i_name, secName, ಗಾತ್ರ(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->ಬೇರು_Continuation or io_runtime->ಬೇರು_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, ಬಫರ್_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.ಗಾತ್ರ_2);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.ಗಾತ್ರ_2 > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (ಕಂಡುಹಿಡಿ_Embedded_Snapshot(i_module->wasm_ಆರಂಭ, i_module->wasm_ಕೊನೆ, secName, &drop_ಆರಂಭ, &drop_ಕೊನೆ, ಖಾಲಿ, ಖಾಲಿ));

    orig_ಗಾತ್ರ           = (size_t)(i_module->wasm_ಕೊನೆ - i_module->wasm_ಆರಂಭ) - (drop_ಆರಂಭ ? (size_t)(drop_ಕೊನೆ - drop_ಆರಂಭ) : 0);
    nameLen            = (u32)ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(secName);
    name_Len_Leb_ಗಾತ್ರ     = ಗಾತ್ರ_ದ_LEB_u32(nameLen);
    payload_ಗಾತ್ರ        = name_Len_Leb_ಗಾತ್ರ + nameLen + (u32)bw.ಗಾತ್ರ_2;
    payload_ಗಾತ್ರ_Leb_ಗಾತ್ರ = ಗಾತ್ರ_ದ_LEB_u32(payload_ಗಾತ್ರ);
    ಒಟ್ಟು_ಗಾತ್ರ          = orig_ಗಾತ್ರ + 1 + payload_ಗಾತ್ರ_Leb_ಗಾತ್ರ + payload_ಗಾತ್ರ;

    ಹೊರಹೋಗು = (u8*)m3_Malloc("wasm_snapshot_module", ಒಟ್ಟು_ಗಾತ್ರ);
    _throwifnull(ಹೊರಹೋಗು);

    p = ಹೊರಹೋಗು;

    ಆದರೆ (drop_ಆರಂಭ) {
        size_t before = (size_t)(drop_ಆರಂಭ - i_module->wasm_ಆರಂಭ);
        size_t after  = (size_t)(i_module->wasm_ಕೊನೆ - drop_ಕೊನೆ);

        ಸ್ಮರಣೆ_ನಕಲು(p, i_module->wasm_ಆರಂಭ, before);
        p += before;
        ಸ್ಮರಣೆ_ನಕಲು(p, drop_ಕೊನೆ, after);
        p += after;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಸ್ಮರಣೆ_ನಕಲು(p, i_module->wasm_ಆರಂಭ, orig_ಗಾತ್ರ);
        p += orig_ಗಾತ್ರ;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_ಗಾತ್ರ);
    p    = EmitLEB_u32(p, nameLen);
    ಸ್ಮರಣೆ_ನಕಲು(p, secName, nameLen);
    p += nameLen;
    ಸ್ಮರಣೆ_ನಕಲು(p, bw.ಬಫರ್, bw.ಗಾತ್ರ_2);

    *o_ಬೈಟುಗಳು = ಹೊರಹೋಗು;
    *o_ಗಾತ್ರ  = ಒಟ್ಟು_ಗಾತ್ರ;
    ಹೊರಹೋಗು      = ಖಾಲಿ;

_catch:
    m3_Free(bw.ಬಫರ್);
    m3_Free(ಹೊರಹೋಗು);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
#ಇಲ್ಲದಿದ್ದರೆ
    (ಶೂನ್ಯ)io_runtime;
    (ಶೂನ್ಯ)i_module;
    (ಶೂನ್ಯ)i_name;
    (ಶೂನ್ಯ)o_ಬೈಟುಗಳು;
    (ಶೂನ್ಯ)o_ಗಾತ್ರ;
    ಹಿಂತಿರುಗಿ "snapshots are not available in this build of Wasm3";
#ಆದರೆ_ಅಂತ್ಯ
}
