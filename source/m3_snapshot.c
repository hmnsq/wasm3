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

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_compile.h"
#అన్తర్భావ "m3_exception.h"
#అన్తర్భావ "m3_host.h"

#అన్తర్భావ <limits.h>

#ఒకవేళ d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
స్థిర స్థిరరాశి u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
స్థిర స్థిరరాశి u32 c_snapshot_సంస్కరణ  = 1;

గణన {
    d_m3SnapshotSection_Meta         = 0,
    d_m3_Snapshot_Section_జ్ఞాపకం       = 1,
    d_m3_Snapshot_Section_పట్టిక        = 2,
    d_m3SnapshotSection_Global       = 3,
    d_m3SnapshotSection_Segment      = 4,
    d_m3SnapshotSection_Exception    = 5,
    d_m3SnapshotSection_Continuation = 6,
    d_m3SnapshotSection_HostState    = 7,
};

#  define d_m3SnapshotFlagPostmortem  0x1

// The longest <name> in a "snapshot.<name>" custom section. A name is a label
// the embedder picks, not something the format needs room to grow.
#  define d_m3_Max_Snapshot_Name_నిడివి   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
గణన {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_ముగిసిన  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_చివర                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//తటస్థం
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
స్థిర
M3_ఫలితం ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_ప్రారంభం or i_module->wasm_చివర - i_module->wasm_ప్రారంభం < 8);

    {
        బైట్లు_t  pos = i_module->wasm_ప్రారంభం + 8;
        cbytes_t చివర = i_module->wasm_చివర;

        Xxh64_Init(&hash, 0);

        అయితే (pos < చివర) {
            బైట్లు_t ప్రారంభం     = pos;
            u8      sectionId = *pos++;
            u32     కొలత      = 0;

_           (ReadLEB_u32(&కొలత, &pos, చివర));
            _throwif(m3Err_wasmUnderrun, కొలత > (size_t)(చివర - pos));

            pos += కొలత;
            ఒకవేళ (sectionId != 0) {
                Xxh64_Update(&hash, ప్రారంభం, (size_t)(pos - ప్రారంభం));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    తిరిగి ఫలితం;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

స్థిర
స్థిరరాశి M3_సురక్షిత_బిందువు* కనుగొను_సురక్షిత_బిందువు (IM3Function i_function, pc_t i_pc, u8 i_రకం)
{
    స్థిరరాశి M3SnapshotMap* map = i_function->snapshotMap;

    ఒకవేళ (map) {
        కొరకు (u32 i = 0; i < map->అంకె_సురక్షిత_Points; ++i) {
            ఒకవేళ (map->సురక్షిత_Points[i].pc == i_pc and map->సురక్షిత_Points[i].రకం == i_రకం) {
                తిరిగి &map->సురక్షిత_Points[i];
            }
        }
    }

    తిరిగి ఖాళీ;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
స్థిర
స్థిరరాశి M3_సురక్షిత_బిందువు* కనుగొను_సురక్షిత_బిందువు_At (స్థిరరాశి M3SnapshotMap* i_map, u32 i_wasm_స్థానభ్రంశం, u8 i_రకం, u32 i_లక్ష్యం_Loop)
{
    u32 దిగువ  = 0;
    u32 ఎగువ = i_map->అంకె_సురక్షిత_Points;

    అయితే (దిగువ < ఎగువ) {
        u32 మధ్య = దిగువ + (ఎగువ - దిగువ) / 2;

        ఒకవేళ (i_map->సురక్షిత_Points[మధ్య].wasm_స్థానభ్రంశం < i_wasm_స్థానభ్రంశం) {
            దిగువ = మధ్య + 1;
        } లేకపోతే {
            ఎగువ = మధ్య;
        }
    }

    కొరకు (u32 i = దిగువ; i < i_map->అంకె_సురక్షిత_Points and i_map->సురక్షిత_Points[i].wasm_స్థానభ్రంశం == i_wasm_స్థానభ్రంశం; ++i) {
        స్థిరరాశి M3_సురక్షిత_బిందువు* బిందువు = &i_map->సురక్షిత_Points[i];

        ఒకవేళ (బిందువు->రకం != i_రకం or (బిందువు->flags & d_m3_సురక్షిత_బిందువు_తోక_Call)) {
            కొనసాగు;
        }
        ఒకవేళ (i_రకం != safepoint_op or i_map->ఖండాలు[బిందువు->aux].wasm_స్థానభ్రంశం == i_లక్ష్యం_Loop) {
            తిరిగి బిందువు;
        }
    }

    తిరిగి ఖాళీ;
}

స్థిర
స్థిరరాశి M3_ఖండం_ప్రారంభం* కనుగొను_ఖండం_At (స్థిరరాశి M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_స్థానభ్రంశం)
{
    కొరకు (u32 i = 0; i < i_map->అంకె_ఖండాలు; ++i) {
        ఒకవేళ (i_map->ఖండాలు[i].opcode == i_opcode and i_map->ఖండాలు[i].wasm_స్థానభ్రంశం == i_wasm_స్థానభ్రంశం) {
            తిరిగి &i_map->ఖండాలు[i];
        }
    }

    తిరిగి ఖాళీ;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
స్థిర అంతఃవరుస
bool Encloses_స్థానభ్రంశం (స్థిరరాశి M3_ఖండం_ప్రారంభం* i_ఖండం, u32 i_wasm_స్థానభ్రంశం)
{
    తిరిగి i_ఖండం->wasm_స్థానభ్రంశం < i_wasm_స్థానభ్రంశం and i_wasm_స్థానభ్రంశం < i_ఖండం->wasm_చివర;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
స్థిర అంతఃవరుస
bool Handlers_సజీవ_At (స్థిరరాశి M3SnapshotMap* i_map, స్థిరరాశి M3_సురక్షిత_బిందువు* i_బిందువు, స్థిరరాశి M3_ఖండం_ప్రారంభం* i_try)
{
    తిరిగి not (i_బిందువు->రకం == safepoint_op and i_try->wasm_స్థానభ్రంశం > i_map->ఖండాలు[i_బిందువు->aux].wasm_స్థానభ్రంశం);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
స్థిర
M3_ఫలితం CallSiteType (IM3Function i_caller, స్థిరరాశి M3_సురక్షిత_బిందువు* i_బిందువు, IM3FuncType* o_type)
{
    M3_ఫలితం   ఫలితం = m3Err_none;
    IM3Module  module = i_caller->module;
    బైట్లు_t    pos    = i_caller->wasm;
    cbytes_t   చివర    = i_caller->wasm_చివర;
    u32        కొలత   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ఖాళీ;

_   (ReadLEB_u32(&కొలత, &pos, చివర));
    _throwif(m3Err_wasmUnderrun, i_బిందువు->wasm_స్థానభ్రంశం >= (size_t)(చివర - pos));
    pos += i_బిందువు->wasm_స్థానభ్రంశం;

_   (Read_opcode(&opcode, &pos, చివర));
_   (ReadLEB_u32(&index, &pos, చివర));

    ఒకవేళ (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->అంకె_Functions);
        *o_type = module->functions[index].funcType;
    } లేకపోతే {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_పరోక్ష and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->అంకె_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    తిరిగి ఫలితం;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
స్థిర
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 అంకె_Results = Get_Func_Type_అంకె_Results(i_expected);

    ఒకవేళ (not i_type or Get_Func_Type_అంకె_Results(i_type) != అంకె_Results) {
        తిరిగి false;
    }

    కొరకు (u16 i = 0; i < అంకె_Results; ++i) {
        ఒకవేళ (not Is_Sub_Type_యొక్క(Get_Func_Type_ఫలితం_Type(i_type, i), Get_Func_Type_ఫలితం_Type(i_expected, i))) {
            తిరిగి false;
        }
    }

    తిరిగి true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
రకనిర్వచనం నిర్మాణం M3_చట్రం_Registers {
    m3reg_t* r0;
#  ఒకవేళ d_m3HasFloat
    f64* fp0;
#  endif
} M3_చట్రం_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
స్థిర
M3_ఫలితం Continuation_దొంతర (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_పునాది, u32* o_అంకె_Slots)
{
    ఒకవేళ (i_cont == i_runtime->మూలం_Continuation) {
        *o_పునాది     = (m3slot_t*)i_runtime->origin_దొంతర;
        *o_అంకె_Slots = i_runtime->అంకె_దొంతర_Slots;
    } లేకపోతే {
        *o_పునాది     = i_cont->val_దొంతర;
        *o_అంకె_Slots = i_cont->అంకె_దొంతర_Slots;
    }

    తిరిగి (*o_పునాది) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
స్థిర
M3_ఫలితం Locate_విలువ (m3slot_t* i_పునాది, u32 i_అంకె_Slots, m3stack_t i_sp, స్థిరరాశి M3_స్థానం_విలువ* i_విలువ, u8** o_where)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u8       type   = పునాది_Type_యొక్క(i_విలువ->type);

    *o_where = ఖాళీ;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_రిజిస్టరు_స్థానం_Alias(i_విలువ->స్థానం));

    ఒకవేళ (not Is_రిజిస్టరు_స్థానం_Alias(i_విలువ->స్థానం)) {
        size_t కొలత   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? పరిమాణం(శూన్య*)
                                                                   : (Is64_బిట్టు_Type(type) ? పరిమాణం(u64) : పరిమాణం(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_పునాది + i_విలువ->స్థానం) * పరిమాణం(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + కొలత > (size_t)i_అంకె_Slots * పరిమాణం(m3slot_t));

        *o_where = (u8*)i_పునాది + offset;
    }

_catch:
    తిరిగి ఫలితం;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
స్థిర
M3_ఫలితం Read_విలువ (u8* i_where, స్థిరరాశి M3_చట్రం_Registers* i_registers, స్థిరరాశి M3_స్థానం_విలువ* i_విలువ, u64* o_బిట్లు,
                    శూన్య** o_reference)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u8       type   = పునాది_Type_యొక్క(i_విలువ->type);

    *o_బిట్లు      = 0;
    *o_reference = ఖాళీ;

    ఒకవేళ (i_where) {
        ఒకవేళ (IsRefType(type)) {
            జ్ఞాపకం_నకలు(o_reference, i_where, పరిమాణం(శూన్య*));
        } లేకపోతే ఒకవేళ (Is64_బిట్టు_Type(type)) {
            జ్ఞాపకం_నకలు(o_బిట్లు, i_where, పరిమాణం(u64));
        } లేకపోతే {
            u32 narrow;
            జ్ఞాపకం_నకలు(&narrow, i_where, పరిమాణం(narrow));
            *o_బిట్లు = narrow;
        }
    } లేకపోతే ఒకవేళ (Is_Fp_రిజిస్టరు_స్థానం_Alias(i_విలువ->స్థానం)) {
#  ఒకవేళ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ఒకవేళ (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 బిట్లు;
            జ్ఞాపకం_నకలు(&బిట్లు, &narrow, పరిమాణం(బిట్లు));
            *o_బిట్లు = బిట్లు;
        } లేకపోతే {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            జ్ఞాపకం_నకలు(o_బిట్లు, i_registers->fp0, పరిమాణం(u64));
        }
#  లేకపోతే
        _throw(m3Err_wasmMalformed);
#  endif
    } లేకపోతే {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        ఒకవేళ (IsRefType(type)) {
            *o_reference = (శూన్య*)(uintptr_t)r0;
        } లేకపోతే ఒకవేళ (Is64_బిట్టు_Type(type)) {
            *o_బిట్లు = (u64)r0;
        } లేకపోతే {
            *o_బిట్లు = (u32)r0;
        }
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Write_విలువ (u8* o_where, స్థిరరాశి M3_చట్రం_Registers* i_registers, స్థిరరాశి M3_స్థానం_విలువ* i_విలువ, u64 i_బిట్లు,
                     శూన్య* i_reference)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u8       type   = పునాది_Type_యొక్క(i_విలువ->type);

    ఒకవేళ (o_where) {
        ఒకవేళ (IsRefType(type)) {
            జ్ఞాపకం_నకలు(o_where, &i_reference, పరిమాణం(శూన్య*));
        } లేకపోతే ఒకవేళ (Is64_బిట్టు_Type(type)) {
            జ్ఞాపకం_నకలు(o_where, &i_బిట్లు, పరిమాణం(u64));
        } లేకపోతే {
            u32 narrow = (u32)i_బిట్లు;
            జ్ఞాపకం_నకలు(o_where, &narrow, పరిమాణం(narrow));
        }
    } లేకపోతే ఒకవేళ (Is_Fp_రిజిస్టరు_స్థానం_Alias(i_విలువ->స్థానం)) {
#  ఒకవేళ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ఒకవేళ (type == c_m3Type_f32) {
            u32 బిట్లు = (u32)i_బిట్లు;
            f32 narrow;
            జ్ఞాపకం_నకలు(&narrow, &బిట్లు, పరిమాణం(narrow));
            *i_registers->fp0 = narrow;
        } లేకపోతే {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            జ్ఞాపకం_నకలు(i_registers->fp0, &i_బిట్లు, పరిమాణం(u64));
        }
#  లేకపోతే
        _throw(m3Err_wasmMalformed);
#  endif
    } లేకపోతే {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        ఒకవేళ (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } లేకపోతే {
            *i_registers->r0 = (m3reg_t)i_బిట్లు;
        }
    }

_catch:
    తిరిగి ఫలితం;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
స్థిర
స్థిరరాశి M3_స్థానం_విలువ* చట్రం_విలువ (స్థిరరాశి M3SnapshotMap* i_map, స్థిరరాశి M3_సురక్షిత_బిందువు* i_బిందువు, u32 i_సూచీ)
{
    ఒకవేళ (i_సూచీ < i_map->అంకె_Locals) {
        తిరిగి i_బిందువు->మొదటి_Local_విలువ == UINT32_MAX ? &i_map->locals[i_సూచీ]
                                                      : &i_map->విలువలు[i_బిందువు->మొదటి_Local_విలువ + i_సూచీ];
    }
    తిరిగి &i_map->విలువలు[i_బిందువు->మొదటి_విలువ + i_సూచీ - i_map->అంకె_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
స్థిర
శూన్య RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    ఒకవేళ (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->అంకె_Ret_And_Arg_Slots) + i_function->అంకె_Local_బైట్లు;

        జ్ఞాపకం_నకలు(where, i_function->constants, i_function->అంకె_Constant_బైట్లు);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
రకనిర్వచనం నిర్మాణం M3_సూచిక_Ids {
    శూన్య** వస్తువులు;
    u32    లెక్క;
    u32    సామర్థ్యం;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  సూచీ_సామర్థ్యం;       // a power of two, kept at least twice count
} M3_సూచిక_Ids;

స్థిర
u32 సూచిక_పాత్ర (శూన్య* i_సూచిక, u32 i_సామర్థ్యం)
{
    u64 బిట్లు = (u64)(uintptr_t)i_సూచిక;

    తిరిగి (u32)((బిట్లు * 0x9E3779B97F4A7C15ULL) >> 32) & (i_సామర్థ్యం - 1);
}

స్థిర
u32 సూచిక_Ids_కనుగొను (స్థిరరాశి M3_సూచిక_Ids* i_ids, శూన్య* i_సూచిక)
{
    ఒకవేళ (i_ids->సూచీ_సామర్థ్యం) {
        u32 పాత్ర = సూచిక_పాత్ర(i_సూచిక, i_ids->సూచీ_సామర్థ్యం);

        అయితే (i_ids->index[పాత్ర]) {
            u32 id = i_ids->index[పాత్ర] - 1;

            ఒకవేళ (i_ids->వస్తువులు[id] == i_సూచిక) {
                తిరిగి id;
            }

            పాత్ర = (పాత్ర + 1) & (i_ids->సూచీ_సామర్థ్యం - 1);
        }
    }

    తిరిగి d_m3SnapshotNone;
}

స్థిర
M3_ఫలితం సూచిక_Ids_కూడు (M3_సూచిక_Ids* io_ids, శూన్య* i_సూచిక, u32* o_id)
{
    u32 id = సూచిక_Ids_కనుగొను(io_ids, i_సూచిక);

    ఒకవేళ (id != d_m3SnapshotNone) {
        *o_id = id;
        తిరిగి m3Err_none;
    }

    ఒకవేళ (io_ids->లెక్క == io_ids->సామర్థ్యం) {
        u32    సామర్థ్యం = io_ids->సామర్థ్యం ? io_ids->సామర్థ్యం * 2 : 16;
        శూన్య** వస్తువులు    = m3_ReallocArray(శూన్య*, io_ids->వస్తువులు, సామర్థ్యం, io_ids->సామర్థ్యం);

        ఒకవేళ (not వస్తువులు) {
            తిరిగి m3Err_mallocFailed;
        }

        io_ids->వస్తువులు    = వస్తువులు;
        io_ids->సామర్థ్యం = సామర్థ్యం;
    }

    ఒకవేళ ((io_ids->లెక్క + 1) * 2 > io_ids->సూచీ_సామర్థ్యం) {
        u32  సామర్థ్యం = io_ids->సూచీ_సామర్థ్యం ? io_ids->సూచీ_సామర్థ్యం * 2 : 32;
        u32* index    = m3_AllocArray(u32, సామర్థ్యం);

        ఒకవేళ (not index) {
            తిరిగి m3Err_mallocFailed;
        }

        కొరకు (u32 i = 0; i < io_ids->లెక్క; ++i) {
            u32 పాత్ర = సూచిక_పాత్ర(io_ids->వస్తువులు[i], సామర్థ్యం);

            అయితే (index[పాత్ర]) {
                పాత్ర = (పాత్ర + 1) & (సామర్థ్యం - 1);
            }

            index[పాత్ర] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->సూచీ_సామర్థ్యం = సామర్థ్యం;
    }

    id                = io_ids->లెక్క++;
    io_ids->వస్తువులు[id] = i_సూచిక;

    u32 పాత్ర = సూచిక_పాత్ర(i_సూచిక, io_ids->సూచీ_సామర్థ్యం);

    అయితే (io_ids->index[పాత్ర]) {
        పాత్ర = (పాత్ర + 1) & (io_ids->సూచీ_సామర్థ్యం - 1);
    }

    io_ids->index[పాత్ర] = id + 1;

    *o_id = id;

    తిరిగి m3Err_none;
}

స్థిర
శూన్య సూచిక_Ids_Free (M3_సూచిక_Ids* io_ids)
{
    m3_Free(io_ids->వస్తువులు);
    m3_Free(io_ids->index);
    memset(io_ids, 0, పరిమాణం(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

రకనిర్వచనం నిర్మాణం M3_బఫరు_Writer {
    u8*    బఫరు;
    size_t కొలత;
    size_t సామర్థ్యం;
} M3_బఫరు_Writer;

స్థిర
M3_ఫలితం బఫరు_Writer_Write (స్థిరరాశి శూన్య* i_దత్తాంశం, size_t i_కొలత, శూన్య* i_userdata)
{
    M3_బఫరు_Writer* bw = (M3_బఫరు_Writer*)i_userdata;

    ఒకవేళ (bw->కొలత + i_కొలత > bw->సామర్థ్యం) {
        size_t newCap = bw->సామర్థ్యం ? bw->సామర్థ్యం * 2 : 4096;
        అయితే (bw->కొలత + i_కొలత > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->బఫరు, newCap, bw->సామర్థ్యం);
        ఒకవేళ (!newBuf) {
            తిరిగి m3Err_mallocFailed;
        }
        bw->బఫరు   = newBuf;
        bw->సామర్థ్యం = newCap;
    }

    ఒకవేళ (i_కొలత) {
        జ్ఞాపకం_నకలు(bw->బఫరు + bw->కొలత, i_దత్తాంశం, i_కొలత);
        bw->కొలత += i_కొలత;
    }

    తిరిగి m3Err_none;
}

రకనిర్వచనం నిర్మాణం M3_బఫరు_Reader {
    స్థిరరాశి u8* బఫరు;
    size_t    కొలత;
    size_t    cursor;
} M3_బఫరు_Reader;

స్థిర
M3_ఫలితం బఫరు_Reader_Read (శూన్య* o_బఫరు, size_t i_కొలత, శూన్య* i_userdata)
{
    M3_బఫరు_Reader* br = (M3_బఫరు_Reader*)i_userdata;

    ఒకవేళ (i_కొలత > br->కొలత - br->cursor) {
        తిరిగి m3Err_wasmMalformed;
    }

    ఒకవేళ (i_కొలత) {
        జ్ఞాపకం_నకలు(o_బఫరు, br->బఫరు + br->cursor, i_కొలత);
        br->cursor += i_కొలత;
    }

    తిరిగి m3Err_none;
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
రకనిర్వచనం నిర్మాణం M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    శూన్య*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3_సూచిక_Ids సజీవ_Continuations;     // everything the runtime still holds, to check
    M3_సూచిక_Ids సజీవ_Exceptions;        //   a reference against before following it
    M3_సూచిక_Ids continuations;         // what the snapshot carries, by id
    M3_సూచిక_Ids exceptions;
} M3SnapshotSave;

స్థిర అంతఃవరుస
u8 M3_Type_గమ్యం_Val_Type (m3type_t i_type)
{
    ఎంపిక (పునాది_Type_యొక్క(i_type)) {
    స్థితి c_m3Type_i32: తిరిగి 0x7F;
    స్థితి c_m3Type_i64: తిరిగి 0x7E;
    స్థితి c_m3Type_f32: తిరిగి 0x7D;
    స్థితి c_m3Type_f64: తిరిగి 0x7C;
    స్థితి c_m3Type_v128: తిరిగి 0x7B;
    స్థితి c_m3Type_funcref: తిరిగి 0x70;
    స్థితి c_m3Type_externref: తిరిగి 0x6F;
    స్థితి c_m3Type_exnref: తిరిగి 0x69;
    స్థితి c_m3Type_contref: తిరిగి 0x68;
    అప్రమేయ: తిరిగి 0x00;
    }
}

స్థిర అంతఃవరుస
m3type_t Val_Type_గమ్యం_M3_Type (u8 i_valtype)
{
    ఎంపిక (i_valtype) {
    స్థితి 0x7F: తిరిగి c_m3Type_i32;
    స్థితి 0x7E: తిరిగి c_m3Type_i64;
    స్థితి 0x7D: తిరిగి c_m3Type_f32;
    స్థితి 0x7C: తిరిగి c_m3Type_f64;
    స్థితి 0x7B: తిరిగి c_m3Type_v128;
    స్థితి 0x70: తిరిగి c_m3Type_funcref;
    స్థితి 0x6F: తిరిగి c_m3Type_externref;
    స్థితి 0x69: తిరిగి c_m3Type_exnref;
    స్థితి 0x68: తిరిగి c_m3Type_contref;
    అప్రమేయ: తిరిగి c_m3Type_none;
    }
}

స్థిర
M3_ఫలితం Put (M3SnapshotSave* s, స్థిరరాశి శూన్య* i_దత్తాంశం, size_t i_కొలత)
{
    తిరిగి s->writing ? s->writer(i_దత్తాంశం, i_కొలత, s->userdata) : m3Err_none;
}

స్థిర
M3_ఫలితం PutU8 (M3SnapshotSave* s, u8 i_విలువ)
{
    తిరిగి Put(s, &i_విలువ, పరిమాణం(i_విలువ));
}


స్థిర
M3_ఫలితం PutLEB_u32 (M3SnapshotSave* s, u32 i_విలువ)
{
    u8  బైట్లు[5];
    u32 len = 0;

    చేయి {
        u8 byte = i_విలువ & 0x7F;
        i_విలువ >>= 7;
        ఒకవేళ (i_విలువ != 0) {
            byte |= 0x80;
        }
        బైట్లు[len++] = byte;
    } అయితే (i_విలువ != 0);

    తిరిగి Put(s, బైట్లు, len);
}

స్థిర
M3_ఫలితం PutLEB_u64 (M3SnapshotSave* s, u64 i_విలువ)
{
    u8  బైట్లు[10];
    u32 len = 0;

    చేయి {
        u8 byte = i_విలువ & 0x7F;
        i_విలువ >>= 7;
        ఒకవేళ (i_విలువ != 0) {
            byte |= 0x80;
        }
        బైట్లు[len++] = byte;
    } అయితే (i_విలువ != 0);

    తిరిగి Put(s, బైట్లు, len);
}

స్థిర
M3_ఫలితం PutLEB_i64 (M3SnapshotSave* s, i64 i_విలువ)
{
    u8   బైట్లు[10];
    u32  len  = 0;
    bool మరిన్ని = true;

    అయితే (మరిన్ని) {
        u8 byte = i_విలువ & 0x7F;
        i_విలువ >>= 7;
        ఒకవేళ ((i_విలువ == 0 && !(byte & 0x40)) || (i_విలువ == -1 && (byte & 0x40))) {
            మరిన్ని = false;
        } లేకపోతే {
            byte |= 0x80;
        }
        బైట్లు[len++] = byte;
    }

    తిరిగి Put(s, బైట్లు, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
రకనిర్వచనం M3_ఫలితం (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
స్థిర
M3_ఫలితం SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_ఫలితం         ఫలితం   = m3Err_none;
    M3_బఫరు_Writer   bw       = { ఖాళీ, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    శూన్య*            userdata = s->userdata;

    s->writer   = బఫరు_Writer_Write;
    s->userdata = &bw;

    ఫలితం = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    ఒకవేళ (ఫలితం) {
        వెళ్ళు _catch;
    }

    // a section with nothing in it is left out
    ఒకవేళ (bw.కొలత) {
        _throwif("snapshot section is too large", bw.కొలత > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.కొలత));
_       (Put(s, bw.బఫరు, bw.కొలత));
    }

_catch:
    m3_Free(bw.బఫరు);

    తిరిగి ఫలితం;
}

స్థిర
u32 Function_సూచీ (IM3Module i_module, IM3Function i_function)
{
    ఒకవేళ (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->అంకె_Functions) {
        తిరిగి (u32)(i_function - i_module->functions);
    }

    తిరిగి d_m3SnapshotNone;
}

స్థిర
u32 Func_Type_సూచీ (IM3Module i_module, IM3FuncType i_type)
{
    కొరకు (u32 i = 0; i < i_module->అంకె_Func_Types; ++i) {
        ఒకవేళ (i_module->funcTypes[i] == i_type) {
            తిరిగి i;
        }
    }

    తిరిగి d_m3SnapshotNone;
}

#  ఒకవేళ d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
స్థిర
IM3_ట్యాగ్ ట్యాగ్_యొక్క_సూచీ (IM3Module i_module, u32 i_సూచీ)
{
    IM3_ట్యాగ్ ట్యాగ్ = &i_module->ట్యాగులు[i_సూచీ];

    తిరిగి ట్యాగ్->resolved ? ట్యాగ్->resolved : ట్యాగ్;
}

స్థిర
u32 ట్యాగ్_సూచీ_2 (IM3Module i_module, IM3_ట్యాగ్ i_ట్యాగ్)
{
    కొరకు (u32 i = 0; i < i_module->అంకె_ట్యాగులు; ++i) {
        ఒకవేళ (ట్యాగ్_యొక్క_సూచీ(i_module, i) == i_ట్యాగ్) {
            తిరిగి i;
        }
    }

    తిరిగి d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
స్థిర
M3_ఫలితం NameReference (M3SnapshotSave* s, u8 i_type, శూన్య* i_reference, u64* o_మాట)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_మాట = d_m3SnapshotNullRef;

    ఒకవేళ (not i_reference) {
        తిరిగి m3Err_none;
    }

    ఒకవేళ (i_type == c_m3Type_funcref) {
        id = Function_సూచీ(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } లేకపోతే ఒకవేళ (i_type == c_m3Type_externref) {
        స్థిరరాశి M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_మాట = name;
        వెళ్ళు _catch;
    }
#  ఒకవేళ d_m3HasExceptionHandling
    లేకపోతే ఒకవేళ (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 సూచిక_Ids_కనుగొను(&s->సజీవ_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 లెక్క = s->exceptions.లెక్క;
_       (సూచిక_Ids_కూడు(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.లెక్క != లెక్క);
    }
#  endif
    లేకపోతే ఒకవేళ (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 సూచిక_Ids_కనుగొను(&s->సజీవ_Continuations, i_reference) == d_m3SnapshotNone);

        u32 లెక్క = s->continuations.లెక్క;
_       (సూచిక_Ids_కూడు(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.లెక్క != లెక్క);
    } లేకపోతే {
        _throw("a reference of an unknown type");
    }

    *o_మాట = id;

_catch:
    తిరిగి ఫలితం;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
స్థిర
M3_ఫలితం Put_Ref_విలువ (M3SnapshotSave* s, u8 i_type, శూన్య* i_reference)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u64      మాట   = 0;

_   (NameReference(s, i_type, i_reference, &మాట));

    ఒకవేళ (not s->writing) {
        తిరిగి m3Err_none;
    }

    ఒకవేళ (మాట == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } లేకపోతే {
        u8 రకం = 0;
        ఒకవేళ (i_type == c_m3Type_funcref) {
            రకం = 0x01;
        } లేకపోతే ఒకవేళ (i_type == c_m3Type_externref) {
            రకం = 0x02;
        }
#  ఒకవేళ d_m3HasExceptionHandling
        లేకపోతే ఒకవేళ (i_type == c_m3Type_exnref) {
            రకం = 0x03;
        }
#  endif
        లేకపోతే ఒకవేళ (i_type == c_m3Type_contref) {
            రకం = 0x04;
        }

_       (PutU8(s, రకం));
_       (PutLEB_u64(s, మాట));
    }

_catch:
    తిరిగి ఫలితం;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
స్థిర
M3_ఫలితం PutLE32 (M3SnapshotSave* s, u32 i_విలువ)
{
    u8 బైట్లు[4];

    కొరకు (u32 i = 0; i < పరిమాణం(బైట్లు); ++i) {
        బైట్లు[i] = (u8)(i_విలువ >> (8 * i));
    }

    తిరిగి Put(s, బైట్లు, పరిమాణం(బైట్లు));
}

స్థిర
M3_ఫలితం PutLE64 (M3SnapshotSave* s, u64 i_విలువ)
{
    u8 బైట్లు[8];

    కొరకు (u32 i = 0; i < పరిమాణం(బైట్లు); ++i) {
        బైట్లు[i] = (u8)(i_విలువ >> (8 * i));
    }

    తిరిగి Put(s, బైట్లు, పరిమాణం(బైట్లు));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
స్థిర
M3_ఫలితం Put_విలువ_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_బిట్లు)
{
    ఒకవేళ (IsRefType(i_type)) {
        తిరిగి Put_Ref_విలువ(s, పునాది_Type_యొక్క(i_type), (శూన్య*)(uintptr_t)i_బిట్లు);
    }

    ఎంపిక (పునాది_Type_యొక్క(i_type)) {
    స్థితి c_m3Type_i32: తిరిగి PutLEB_i64(s, (i32)(u32)i_బిట్లు);
    స్థితి c_m3Type_i64: తిరిగి PutLEB_i64(s, (i64)i_బిట్లు);
    స్థితి c_m3Type_f32: తిరిగి PutLE32(s, (u32)i_బిట్లు);
    స్థితి c_m3Type_f64: తిరిగి PutLE64(s, i_బిట్లు);
    అప్రమేయ: తిరిగి "a snapshot cannot hold a value of this type as bits";
    }
}

స్థిర
M3_ఫలితం Put_విలువ_ట్యాగ్ (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_గమ్యం_Val_Type(i_type);

    ఒకవేళ (not valtype) {
        తిరిగి "a snapshot has no tag for a value of this type";
    }

    తిరిగి PutU8(s, valtype);
}

స్థిర
M3_ఫలితం Put_విలువ_బిట్లు (M3SnapshotSave* s, m3type_t i_type, u64 i_బిట్లు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (Put_విలువ_ట్యాగ్(s, i_type));
_   (Put_విలువ_Body(s, i_type, i_బిట్లు));

_catch:
    తిరిగి ఫలితం;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
స్థిర
M3_ఫలితం Put_విలువ (M3SnapshotSave* s, m3type_t i_type, స్థిరరాశి శూన్య* i_where)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (Put_విలువ_ట్యాగ్(s, i_type));

    ఒకవేళ (IsRefType(i_type)) {
        శూన్య* reference;
        జ్ఞాపకం_నకలు(&reference, i_where, పరిమాణం(reference));
_       (Put_విలువ_Body(s, i_type, (u64)(uintptr_t)reference));
    } లేకపోతే ఒకవేళ (పునాది_Type_యొక్క(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 బైట్లు[16];
        జ్ఞాపకం_నకలు(బైట్లు, i_where, పరిమాణం(బైట్లు));
_       (Put(s, బైట్లు, పరిమాణం(బైట్లు)));
    } లేకపోతే ఒకవేళ (Is64_బిట్టు_Type(i_type)) {
        u64 బిట్లు;
        జ్ఞాపకం_నకలు(&బిట్లు, i_where, పరిమాణం(బిట్లు));
_       (Put_విలువ_Body(s, i_type, బిట్లు));
    } లేకపోతే {
        u32 narrow;
        జ్ఞాపకం_నకలు(&narrow, i_where, పరిమాణం(narrow));
_       (Put_విలువ_Body(s, i_type, narrow));
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Put_జ్ఞాపకం_Chunks (M3SnapshotSave* s, స్థిరరాశి u8* i_బైట్లు, size_t i_కొలత)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    size_t   cursor = 0;

    అయితే (cursor < i_కొలత) {
        u8     byte = i_బైట్లు[cursor];
        size_t నిరంతరం  = 1;

        అయితే (cursor + నిరంతరం < i_కొలత and i_బైట్లు[cursor + నిరంతరం] == byte) {
            నిరంతరం++;
        }

        ఒకవేళ ((byte == 0x00 or byte == 0xFF) and నిరంతరం >= d_m3_Snapshot_నిరంతరం_పరిమితి) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            ఒకవేళ (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)నిరంతరం));
            }

            cursor += నిరంతరం;
            కొనసాగు;
        }

        size_t raw_ప్రారంభం = cursor;

        అయితే (cursor < i_కొలత) {
            u8     b      = i_బైట్లు[cursor];
            size_t నిడివి = 1;

            అయితే (cursor + నిడివి < i_కొలత and i_బైట్లు[cursor + నిడివి] == b) {
                నిడివి++;
            }

            ఒకవేళ ((b == 0x00 or b == 0xFF) and నిడివి >= d_m3_Snapshot_నిరంతరం_పరిమితి) {
                ఆపు;
            }

            cursor += నిడివి;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_ప్రారంభం));
_       (PutLEB_u32(s, (u32)(cursor - raw_ప్రారంభం)));
_       (Put(s, i_బైట్లు + raw_ప్రారంభం, cursor - raw_ప్రారంభం));
    }

_   (PutU8(s, d_m3_Chunk_చివర));

_catch:
    తిరిగి ఫలితం;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
స్థిర
u32 జ్ఞాపకం_మొదటి_సూచీ (IM3Module module, u32 index)
{
    u32 j = 0;
    అయితే (module->memories[j] != module->memories[index]) {
        ++j;
    }
    తిరిగి j;
}

స్థిర
u32 పట్టిక_మొదటి_సూచీ (IM3Module module, u32 index)
{
    u32 j = 0;
    అయితే (module->tables[j] != module->tables[index]) {
        ++j;
    }
    తిరిగి j;
}

స్థిర
M3_ఫలితం SaveMemories (M3SnapshotSave* s)
{
    M3_ఫలితం  ఫలితం = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->అంకె_Memories));

    కొరకు (u32 m = 0; m < module->అంకె_Memories; ++m) {
        IM3_జ్ఞాపకం జ్ఞాపకం   = module->memories[m];
        u32       మొదటి    = జ్ఞాపకం_మొదటి_సూచీ(module, m);
        bool      has_దత్తాంశం  = జ్ఞాపకం and జ్ఞాపకం->mallocated;
        u32       పుట_కొలత = జ్ఞాపకం ? జ్ఞాపకం_పుట_కొలత(జ్ఞాపకం) : d_m3_Default_Mem_పుట_కొలత;
        u64       బైట్లు    = జ్ఞాపకం ? జ్ఞాపకం->అంకె_పుటలు * (u64)పుట_కొలత : 0;
        u32       పుట_బిట్లు = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, మొదటి));

        ఒకవేళ (మొదటి != m) {
            కొనసాగు;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", జ్ఞాపకం and జ్ఞాపకం->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", బైట్లు > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 జ్ఞాపకం and జ్ఞాపకం->mallocated and జ్ఞాపకం->mallocated->నిడివి < బైట్లు);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        అయితే ((1u << పుట_బిట్లు) < పుట_కొలత) {
            ++పుట_బిట్లు;
        }

_       (PutLEB_u32(s, పుట_బిట్లు));
_       (PutLEB_u32(s, జ్ఞాపకం ? (u32)జ్ఞాపకం->అంకె_పుటలు : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        ఒకవేళ (not s->writing) {
            కొనసాగు;
        }

        ఒకవేళ (has_దత్తాంశం) {
_           (Put_జ్ఞాపకం_Chunks(s, m3_Mem_దత్తాంశం(జ్ఞాపకం->mallocated), (size_t)బైట్లు));
        } లేకపోతే {
_           (PutU8(s, d_m3_Chunk_చివర));
        }
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం SaveGlobals (M3SnapshotSave* s)
{
    M3_ఫలితం  ఫలితం = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->అంకె_Globals));

    కొరకు (u32 g = 0; g < module->అంకె_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_విలువ(s, global->type, &cell->i64_విలువ));
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం SaveTables (M3SnapshotSave* s)
{
    M3_ఫలితం  ఫలితం = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->అంకె_Tables));

    కొరకు (u32 t = 0; t < module->అంకె_Tables; ++t) {
        IM3_పట్టిక పట్టిక_2 = module->tables[t];
        u32      మొదటి = పట్టిక_మొదటి_సూచీ(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, మొదటి));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        ఒకవేళ (మొదటి != t) {
            కొనసాగు;
        }

_       (PutU8(s, M3_Type_గమ్యం_Val_Type(పట్టిక_2->type)));
_       (PutLEB_u32(s, పట్టిక_2->కొలత));

        కొరకు (u32 e = 0; e < పట్టిక_2->కొలత; ++e) {
_           (Put_Ref_విలువ(s, పునాది_Type_యొక్క(పట్టిక_2->type), పట్టిక_2->elements[e]));
        }
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం SaveSegments (M3SnapshotSave* s)
{
    M3_ఫలితం  ఫలితం = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->అంకె_దత్తాంశం_Segments));

    కొరకు (u32 i = 0; i < module->అంకె_దత్తాంశం_Segments; ++i) {
_       (PutU8(s, module->దత్తాంశం_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->అంకె_Element_Segments));

    కొరకు (u32 i = 0; i < module->అంకె_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    తిరిగి ఫలితం;
}

#  ఒకవేళ d_m3HasExceptionHandling

స్థిర
M3_ఫలితం SaveExceptions (M3SnapshotSave* s)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.లెక్క));

    కొరకు (u32 i = 0; i < s->exceptions.లెక్క; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.వస్తువులు[i];
        IM3FuncType  type = exn->ట్యాగ్->type;

        _throwif(m3Err_wasmMalformed, exn->అంకె_వాదనలు != Get_Func_Type_అంకె_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->అంకె_వాదనలు));

        కొరకు (u32 a = 0; a < exn->అంకె_వాదనలు; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_విలువ_బిట్లు(s, argType, exn->వాదనలు[a]));
        }
    }

_catch:
    తిరిగి ఫలితం;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
రకనిర్వచనం నిర్మాణం M3Activation {
    IM3Function        function;
    స్థిరరాశి M3_సురక్షిత_బిందువు* బిందువు;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 రకం;            // M3SafePointKind
    M3_చట్రం_Registers   registers;       // the ones it goes on with
    స్థిరరాశి M3_చట్రం*     resume;          // the resume it waits in, for safepoint_resume
    i32                మొదటి_చట్రం;      // its frames, outermost first: firstFrame down to lastFrame
    i32                చివరి_చట్రం;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
స్థిర
స్థిరరాశి M3_సురక్షిత_బిందువు* కనుగొను_చట్రం_సురక్షిత_బిందువు (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_రకం)
{
    స్థిరరాశి M3_సురక్షిత_బిందువు* బిందువు = *io_function ? కనుగొను_సురక్షిత_బిందువు(*io_function, i_pc, i_రకం) : ఖాళీ;

    కొరకు (u32 i = 0; not బిందువు and i < i_module->అంకె_Functions; ++i) {
        బిందువు = కనుగొను_సురక్షిత_బిందువు(&i_module->functions[i], i_pc, i_రకం);
        ఒకవేళ (బిందువు) {
            *io_function = &i_module->functions[i];
        }
    }

    తిరిగి బిందువు;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
స్థిర
M3_ఫలితం తదుపరి_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_మొదటి, M3Activation* o_act)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    i32      i      = i_మొదటి;

    memset(o_act, 0, పరిమాణం(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->మొదటి_చట్రం = i_మొదటి;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    కొరకు (; i >= 0; --i) {
        స్థిరరాశి M3_చట్రం* చట్రం = &i_cont->చట్రాలు[i];

        _throwif("a suspended frame runs against another module's memory", చట్రం->జ్ఞాపకం != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", చట్రం->sp != i_sp);

        ఒకవేళ (చట్రం->రకం == చట్రం_call) {
            o_act->pc           = చట్రం->pc;
            o_act->రకం         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&చట్రం->call.r0;
#  ఒకవేళ d_m3HasFloat
            o_act->registers.fp0 = (f64*)&చట్రం->call.fp0;
#  endif
            ఆపు;
        } లేకపోతే ఒకవేళ (చట్రం->రకం == చట్రం_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = చట్రం->pc;
            o_act->రకం   = safepoint_resume;
            o_act->resume = చట్రం;
            ఆపు;
        }
    }

    ఒకవేళ (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->రకం         = i_cont->suspend_బిందువు;
        o_act->registers.r0 = &i_cont->r0;
#  ఒకవేళ d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->చివరి_చట్రం = i;
    o_act->బిందువు     = కనుగొను_చట్రం_సురక్షిత_బిందువు(i_module, &o_act->function, o_act->pc, o_act->రకం);

    _throwif("a suspended frame is not at a safepoint", not o_act->బిందువు);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
    కొరకు (i32 k = i_మొదటి; k > o_act->చివరి_చట్రం; --k) {
        స్థిరరాశి M3_చట్రం* చట్రం = &i_cont->చట్రాలు[k];
        _throwif("a suspended frame is not the function it is entered with",
                 చట్రం->రకం == చట్రం_నమోదు and చట్రం->నమోదు_2.function != o_act->function);
    }
#  endif

_catch:
    తిరిగి ఫలితం;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
స్థిర
M3_ఫలితం SaveActivation (M3SnapshotSave* s, m3slot_t* i_పునాది, u32 i_అంకె_Slots, స్థిరరాశి M3Activation* i_act)
{
    M3_ఫలితం             ఫలితం        = m3Err_none;
    IM3Function          function      = i_act->function;
    స్థిరరాశి M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_సూచీ = Function_సూచీ(s->module, function);
    స్థిరరాశి M3_సురక్షిత_బిందువు*   బిందువు         = i_act->బిందువు;
    u32                  అంకె_విలువలు;

    _throwif("a suspended frame belongs to another module", function_సూచీ == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_సూచీ));
_   (PutU8(s, బిందువు->రకం));
_   (PutLEB_u32(s, బిందువు->wasm_స్థానభ్రంశం));

    ఒకవేళ (బిందువు->రకం == safepoint_op) {
_       (PutLEB_u32(s, map->ఖండాలు[బిందువు->aux].wasm_స్థానభ్రంశం));
    }

    అంకె_విలువలు = map->అంకె_Locals + బిందువు->అంకె_విలువలు;

_   (PutLEB_u32(s, అంకె_విలువలు));

    కొరకు (u32 v = 0; v < అంకె_విలువలు; ++v) {
        స్థిరరాశి M3_స్థానం_విలువ* విలువ = చట్రం_విలువ(map, బిందువు, v);
        u8*                where;
        u64                మాట;
        శూన్య*              reference;

_       (Locate_విలువ(i_పునాది, i_అంకె_Slots, i_act->sp, విలువ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ఒకవేళ (పునాది_Type_యొక్క(విలువ->type) == c_m3Type_v128) {
_           (Put_విలువ_ట్యాగ్(s, విలువ->type));
_           (Put(s, where, 16));
            కొనసాగు;
        }

_       (Read_విలువ(where, &i_act->registers, విలువ, &మాట, &reference));

_       (Put_విలువ_బిట్లు(s, విలువ->type, IsRefType(విలువ->type) ? (u64)(uintptr_t)reference : మాట));
    }

    ఒకవేళ (బిందువు->రకం == safepoint_resume) {
_       (Put_Ref_విలువ(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    తిరిగి ఫలితం;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
స్థిర
M3_ఫలితం SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_ఫలితం      ఫలితం   = m3Err_none;
    M3Activation* acts     = ఖాళీ;
    u32           అంకె_Acts  = 0;
    IM3Function   function = i_cont->నమోదు_Function;
    i32           మొదటి    = (i32)i_cont->అంకె_చట్రాలు - 1;
    m3slot_t*     పునాది;
    u32           అంకె_Slots;
    m3stack_t     sp;

_   (Continuation_దొంతర(s->runtime, i_cont, &పునాది, &అంకె_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->అంకె_చట్రాలు + 1);
    _throwifnull(acts);

    sp = పునాది;

    అయితే (true) {
        M3Activation* act = &acts[అంకె_Acts];

_       (తదుపరి_Activation(s->module, i_cont, function, sp, మొదటి, act));

        ఒకవేళ (act->రకం != safepoint_call) {
            అంకె_Acts++;
            ఆపు;
        }

        _throwif("a suspended continuation stops at a call", act->చివరి_చట్రం < 0);

        ఒకవేళ (not (act->బిందువు->flags & d_m3_సురక్షిత_బిందువు_తోక_Call)) {
            అంకె_Acts++;
        }

        function = i_cont->చట్రాలు[act->చివరి_చట్రం].call.function;
        sp       = act->sp + act->బిందువు->aux;
        మొదటి    = act->చివరి_చట్రం - 1;
    }

_   (PutLEB_u32(s, అంకె_Acts));

    కొరకు (u32 a = 0; a < అంకె_Acts; ++a) {
_       (SaveActivation(s, పునాది, అంకె_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_ఫలితం  ఫలితం    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_మూలం    = (i_cont == s->runtime->మూలం_Continuation);
    u8        state     = snapshot_cont_ముగిసిన;
    u32       type_సూచీ = is_మూలం ? d_m3SnapshotNone : Func_Type_సూచీ(module, i_cont->type);

    ఒకవేళ (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } లేకపోతే ఒకవేళ (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } లేకపోతే ఒకవేళ (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->నమోదు_Function);
    _throwif("a continuation belongs to another module", Function_సూచీ(module, i_cont->నమోదు_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_మూలం and type_సూచీ == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_మూలం));
_   (PutLEB_u32(s, type_సూచీ));
_   (PutLEB_u32(s, Function_సూచీ(module, i_cont->నమోదు_Function)));
#  ఒకవేళ d_m3HasExceptionHandling
_   (Put_Ref_విలువ(s, c_m3Type_exnref, i_cont->resumeThrow));
#  లేకపోతే
_   (Put_Ref_విలువ(s, c_m3Type_exnref, ఖాళీ));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    ఒకవేళ (s->postmortem) {
        వెళ్ళు _catch;
    }

    ఒకవేళ (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->నమోదు_Function ? i_cont->నమోదు_Function->funcType : ఖాళీ;
        u16         అంకె_Rets = inner ? Get_Func_Type_అంకె_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_వాదనలు_లెక్క > Get_Func_Type_అంకె_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_వాదనలు_లెక్క));

        కొరకు (u32 i = 0; i < i_cont->bound_వాదనలు_లెక్క; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_విలువ(s, type, i_cont->val_దొంతర + (అంకె_Rets + i) * c_io_స్థానం_లెక్క));
        }
    } లేకపోతే ఒకవేళ (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_వాదనలు_లెక్క));
_       (SaveSuspendedBody(s, i_cont));
    } లేకపోతే {
_       (PutLEB_u32(s, 0));
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం SaveContinuations (M3SnapshotSave* s)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (PutLEB_u32(s, s->continuations.లెక్క));

    కొరకు (u32 i = 0; i < s->continuations.లెక్క; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.వస్తువులు[i]));
    }

_catch:
    తిరిగి ఫలితం;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
స్థిర
M3_ఫలితం SaveHostState (M3SnapshotSave* s)
{
    M3_ఫలితం               ఫలితం = m3Err_none;
    స్థిరరాశి M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_బఫరు_Writer         bw     = { ఖాళీ, 0, 0 };

    ఒకవేళ (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, బఫరు_Writer_Write, &bw));
    }

    ఒకవేళ (bw.కొలత > 0) {
_       (Put(s, bw.బఫరు, bw.కొలత));
    }

_catch:
    m3_Free(bw.బఫరు);

    తిరిగి ఫలితం;
}

స్థిర
శూన్య SnapshotResourceTotals (IM3Module module, u64* జ్ఞాపకం_బైట్లు, u64* పట్టిక_Elements)
{
    *జ్ఞాపకం_బైట్లు = *పట్టిక_Elements = 0;
    కొరకు (u32 i = 0; i < module->అంకె_Memories; ++i) {
        IM3_జ్ఞాపకం జ్ఞాపకం = module->memories[i];
        ఒకవేళ (జ్ఞాపకం_మొదటి_సూచీ(module, i) == i and జ్ఞాపకం) {
            *జ్ఞాపకం_బైట్లు += జ్ఞాపకం->అంకె_పుటలు * (u64)జ్ఞాపకం_పుట_కొలత(జ్ఞాపకం);
        }
    }
    కొరకు (u32 i = 0; i < module->అంకె_Tables; ++i) {
        IM3_పట్టిక పట్టిక_2 = module->tables[i];
        ఒకవేళ (పట్టిక_మొదటి_సూచీ(module, i) == i and పట్టిక_2) {
            *పట్టిక_Elements += పట్టిక_2->కొలత;
        }
    }
}

స్థిర
M3_ఫలితం SaveMeta (M3SnapshotSave* s)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u64      hash   = 0;
    u64      జ్ఞాపకం_బైట్లు, పట్టిక_Elements;
    u32      చురుకైన_Stacks = 0;
    కొరకు (u32 i = 0; i < s->continuations.లెక్క; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.వస్తువులు[i];
        ఒకవేళ (cont != s->runtime->మూలం_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            చురుకైన_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &జ్ఞాపకం_బైట్లు, &పట్టిక_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_సమయం_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.లెక్క));
_   (PutLEB_u32(s, s->exceptions.లెక్క));
_   (PutLEB_u64(s, జ్ఞాపకం_బైట్లు));
_   (PutLEB_u64(s, పట్టిక_Elements));
_   (PutLEB_u32(s, చురుకైన_Stacks));

#  ఒకవేళ d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    కొరకు (u32 i = 0; i < s->exceptions.లెక్క; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.వస్తువులు[i];
        u32          ట్యాగ్_సూచీ  = ట్యాగ్_సూచీ_2(s->module, exception->ట్యాగ్);

        _throwif("an exception was thrown with another module's tag", ట్యాగ్_సూచీ == d_m3SnapshotNone);

_       (PutLEB_u32(s, ట్యాగ్_సూచీ));
    }
#  endif

_catch:
    తిరిగి ఫలితం;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
స్థిర
M3_ఫలితం DiscoverReferences (M3SnapshotSave* s)
{
    M3_ఫలితం ఫలితం            = m3Err_none;
    u32      పూర్తి_Continuations = 0;
    u32      పూర్తి_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    అయితే (పూర్తి_Continuations < s->continuations.లెక్క or పూర్తి_Exceptions < s->exceptions.లెక్క) {
#  ఒకవేళ d_m3HasExceptionHandling
        అయితే (పూర్తి_Exceptions < s->exceptions.లెక్క) {
            M3Exception* exception = (M3Exception*)s->exceptions.వస్తువులు[పూర్తి_Exceptions++];
            IM3FuncType  type      = exception->ట్యాగ్->type;

            _throwif(m3Err_wasmMalformed, exception->అంకె_వాదనలు != Get_Func_Type_అంకె_Params(type));

            కొరకు (u32 a = 0; a < exception->అంకె_వాదనలు; ++a) {
_               (Put_విలువ_బిట్లు(s, GetFuncTypeParamType(type, (u16)a), exception->వాదనలు[a]));
            }
        }
#  endif
        అయితే (పూర్తి_Continuations < s->continuations.లెక్క and పూర్తి_Exceptions == s->exceptions.లెక్క) {
            u32 id = పూర్తి_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.వస్తువులు[id]));
        }
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం SaveSections (M3SnapshotSave* s)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (not s->writing) {
        తిరిగి DiscoverReferences(s);
    }

    ఒకవేళ (not s->embedded) {
_       (Put(s, c_snapshotMagic, పరిమాణం(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_సంస్కరణ));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    ఒకవేళ (s->module->అంకె_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_జ్ఞాపకం, SaveMemories));
    }
    ఒకవేళ (s->module->అంకె_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_పట్టిక, SaveTables));
    }
    ఒకవేళ (s->module->అంకె_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    ఒకవేళ (s->module->అంకె_దత్తాంశం_Segments or s->module->అంకె_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  ఒకవేళ d_m3HasExceptionHandling
    ఒకవేళ (s->exceptions.లెక్క) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    ఒకవేళ (s->continuations.లెక్క) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    తిరిగి ఫలితం;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

రకనిర్వచనం నిర్మాణం M3SnapshotSuspendSite {
    స్థిరరాశి M3SnapshotMap* map;
    స్థిరరాశి M3_సురక్షిత_బిందువు*   బిందువు;
    u32                  లక్ష్యం;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

రకనిర్వచనం నిర్మాణం M3_Snapshot_ఎక్కించు {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    శూన్య*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_దొంతర;
    bool      మారింది;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    అంకె_Continuations;
    u64                    required_జ్ఞాపకం_బైట్లు;
    u64                    required_పట్టిక_Elements;
    u64                    loaded_జ్ఞాపకం_బైట్లు;
    u64                    loaded_పట్టిక_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  ఒకవేళ d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 అంకె_Exceptions;

    M3_చట్రం* చట్రాలు;           // a suspended continuation's frames as they are put together, outermost first

    బైట్లు_t pos;               // bounded section payload, read using the Wasm parser's helpers
    బైట్లు_t చివర;
    bool    లోపల_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    చూసిన_Sections[256];
} M3_Snapshot_ఎక్కించు;

స్థిర
M3_ఫలితం Get (M3_Snapshot_ఎక్కించు* l, శూన్య* o_దత్తాంశం, size_t i_కొలత)
{
    ఒకవేళ (not l->లోపల_Section) {
        తిరిగి l->reader(o_దత్తాంశం, i_కొలత, l->userdata);
    }
    ఒకవేళ (i_కొలత > (size_t)(l->చివర - l->pos)) {
        తిరిగి m3Err_wasmUnderrun;
    }
    ఒకవేళ (i_కొలత) {
        జ్ఞాపకం_నకలు(o_దత్తాంశం, l->pos, i_కొలత);
        l->pos += i_కొలత;
    }
    తిరిగి m3Err_none;
}

స్థిర
M3_ఫలితం GetU8 (M3_Snapshot_ఎక్కించు* l, u8* o_విలువ)
{
    తిరిగి Get(l, o_విలువ, పరిమాణం(*o_విలువ));
}

స్థిర
M3_ఫలితం GetBool (M3_Snapshot_ఎక్కించు* l, u8* o_విలువ)
{
    M3_ఫలితం ఫలితం = GetU8(l, o_విలువ);
    తిరిగి ఫలితం ? ఫలితం : (*o_విలువ <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

స్థిర
M3_ఫలితం GetLEB_u32 (M3_Snapshot_ఎక్కించు* l, u32* o_విలువ)
{
    ఒకవేళ (l->లోపల_Section) {
        తిరిగి ReadLEB_u32(o_విలువ, &l->pos, l->చివర);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    కొరకు (u32 i = 0; i < పరిమాణం(encoded); ++i) {
        M3_ఫలితం ఫలితం = GetU8(l, &encoded[i]);
        ఒకవేళ (ఫలితం) {
            తిరిగి ఫలితం;
        }
        ఒకవేళ (not (encoded[i] & 0x80)) {
            బైట్లు_t pos = encoded;
            తిరిగి ReadLEB_u32(o_విలువ, &pos, encoded + i + 1);
        }
    }
    తిరిగి m3_Err_leb_అధికప్రవాహం;
}

స్థిర
M3_ఫలితం GetLEB_u64 (M3_Snapshot_ఎక్కించు* l, u64* o_విలువ)
{
    తిరిగి ReadLebUnsigned(o_విలువ, 64, &l->pos, l->చివర);
}

స్థిర
M3_ఫలితం GetLEB_i64 (M3_Snapshot_ఎక్కించు* l, i64* o_విలువ)
{
    తిరిగి ReadLEB_i64(o_విలువ, &l->pos, l->చివర);
}

స్థిర
M3_ఫలితం Skip_బైట్లు (M3_Snapshot_ఎక్కించు* l, size_t i_కొలత)
{
    ఒకవేళ (i_కొలత > (size_t)(l->చివర - l->pos)) {
        తిరిగి m3Err_wasmUnderrun;
    }
    l->pos += i_కొలత;
    తిరిగి m3Err_none;
}


స్థిర
M3_ఫలితం Get_Ref_విలువ (M3_Snapshot_ఎక్కించు* l, m3type_t i_type, శూన్య** o_reference)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u8       రకం   = 0;
    u64      id     = 0;

    *o_reference = ఖాళీ;

_   (GetU8(l, &రకం));
    ఒకవేళ (రకం == 0x00) {
        తిరిగి (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    ఒకవేళ (రకం == 0x01) {
        _throwif(m3Err_wasmMalformed, పునాది_Type_యొక్క(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->అంకె_Functions);
#  ఒకవేళ d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_యొక్క(Ref_Type_యొక్క_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } లేకపోతే ఒకవేళ (రకం == 0x02) {
        _throwif(m3Err_wasmMalformed, పునాది_Type_యొక్క(i_type) != c_m3Type_externref);
        స్థిరరాశి M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        ఒకవేళ (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  ఒకవేళ d_m3HasExceptionHandling
    లేకపోతే ఒకవేళ (రకం == 0x03) {
        _throwif(m3Err_wasmMalformed, పునాది_Type_యొక్క(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->అంకె_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    లేకపోతే ఒకవేళ (రకం == 0x04) {
        _throwif(m3Err_wasmMalformed, పునాది_Type_యొక్క(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->అంకె_Continuations);
        *o_reference = l->continuations[id];
#  ఒకవేళ d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        ఒకవేళ (not l->checking and కుప్ప_Type_యొక్క(i_type) != d_m3_Type_కుప్ప_Abstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } లేకపోతే {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం GetLE32 (M3_Snapshot_ఎక్కించు* l, u32* o_విలువ)
{
    u8       బైట్లు[4];
    M3_ఫలితం ఫలితం = Get(l, బైట్లు, పరిమాణం(బైట్లు));

    *o_విలువ = 0;

    కొరకు (u32 i = 0; i < పరిమాణం(బైట్లు); ++i) {
        *o_విలువ |= (u32)బైట్లు[i] << (8 * i);
    }

    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం GetLE64 (M3_Snapshot_ఎక్కించు* l, u64* o_విలువ)
{
    u8       బైట్లు[8];
    M3_ఫలితం ఫలితం = Get(l, బైట్లు, పరిమాణం(బైట్లు));

    *o_విలువ = 0;

    కొరకు (u32 i = 0; i < పరిమాణం(బైట్లు); ++i) {
        *o_విలువ |= (u64)బైట్లు[i] << (8 * i);
    }

    తిరిగి ఫలితం;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
స్థిర
M3_ఫలితం Get_విలువ_ట్యాగ్ (M3_Snapshot_ఎక్కించు* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_ఫలితం ఫలితం  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_గమ్యం_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and పునాది_Type_యొక్క(*o_type) != పునాది_Type_యొక్క(i_expectedType));
    ఒకవేళ (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    తిరిగి ఫలితం;
}

// The mirror of PutValueBody: narrow types come back in the low bits
స్థిర
M3_ఫలితం Get_విలువ_Body (M3_Snapshot_ఎక్కించు* l, m3type_t i_type, u64* o_బిట్లు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    *o_బిట్లు = 0;

    ఒకవేళ (IsRefType(i_type)) {
        శూన్య* reference = ఖాళీ;
_       (Get_Ref_విలువ(l, i_type, &reference));
        *o_బిట్లు = (u64)(uintptr_t)reference;
    } లేకపోతే ఒకవేళ (పునాది_Type_యొక్క(i_type) == c_m3Type_i32) {
        i32 విలువ = 0;
_       (ReadLEB_i32(&విలువ, &l->pos, l->చివర));
        *o_బిట్లు = (u32)విలువ;
    } లేకపోతే ఒకవేళ (పునాది_Type_యొక్క(i_type) == c_m3Type_i64) {
        i64 విలువ = 0;
_       (GetLEB_i64(l, &విలువ));
        *o_బిట్లు = (u64)విలువ;
    } లేకపోతే ఒకవేళ (పునాది_Type_యొక్క(i_type) == c_m3Type_f32) {
        u32 బిట్లు = 0;
_       (GetLE32(l, &బిట్లు));
        *o_బిట్లు = బిట్లు;
    } లేకపోతే ఒకవేళ (పునాది_Type_యొక్క(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_బిట్లు));
    } లేకపోతే {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Get_విలువ_బిట్లు (M3_Snapshot_ఎక్కించు* l, m3type_t i_expectedType, u64* o_బిట్లు)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    m3type_t type;

_   (Get_విలువ_ట్యాగ్(l, i_expectedType, &type));
_   (Get_విలువ_Body(l, type, o_బిట్లు));

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం Get_విలువ (M3_Snapshot_ఎక్కించు* l, m3type_t i_expectedType, శూన్య* o_where)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    ఒకవేళ (l->checking) {
        o_where = scratch;
    }

_   (Get_విలువ_ట్యాగ్(l, i_expectedType, &type));

    ఒకవేళ (IsRefType(type)) {
        శూన్య* reference = ఖాళీ;
        u64   బిట్లు      = 0;
_       (Get_విలువ_Body(l, type, &బిట్లు));
        reference = (శూన్య*)(uintptr_t)బిట్లు;
        జ్ఞాపకం_నకలు(o_where, &reference, పరిమాణం(reference));
    } లేకపోతే ఒకవేళ (పునాది_Type_యొక్క(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } లేకపోతే {
        u64 బిట్లు = 0;
_       (Get_విలువ_Body(l, type, &బిట్లు));

        ఒకవేళ (Is64_బిట్టు_Type(type)) {
            జ్ఞాపకం_నకలు(o_where, &బిట్లు, పరిమాణం(బిట్లు));
        } లేకపోతే {
            u32 narrow = (u32)బిట్లు;
            జ్ఞాపకం_నకలు(o_where, &narrow, పరిమాణం(narrow));
        }
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం GetFunction (M3_Snapshot_ఎక్కించు* l, IM3Function* o_function)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->అంకె_Functions);
    *o_function = &l->module->functions[index];

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_జ్ఞాపకం_Chunks (M3_Snapshot_ఎక్కించు* l, u8* o_బైట్లు, size_t i_కొలత)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    కొరకు (;;) {
        u8  chunkType = 0;
        u32 offset = 0, నిడివి = 0;

_       (GetU8(l, &chunkType));

        ఒకవేళ (chunkType == d_m3_Chunk_చివర) {
            ఆపు;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &నిడివి));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_కొలత or నిడివి > i_కొలత - (size_t)offset);

        ఒకవేళ (chunkType == d_m3ChunkRaw) {
            ఒకవేళ (l->checking or నిడివి == 0) {
_               (Skip_బైట్లు(l, నిడివి));
            } లేకపోతే {
_               (Get(l, o_బైట్లు + offset, నిడివి));
            }
        } లేకపోతే ఒకవేళ (chunkType == d_m3ChunkFillFF) {
            ఒకవేళ (not l->checking and నిడివి) {
                memset(o_బైట్లు + offset, 0xFF, నిడివి);
            }
        } లేకపోతే {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Memories (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం  ఫలితం      = m3Err_none;
    IM3Module module      = l->module;
    u32       అంకె_Memories = 0;
    l->loaded_జ్ఞాపకం_బైట్లు  = 0;

_   (GetLEB_u32(l, &అంకె_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, అంకె_Memories == 0 or అంకె_Memories != module->అంకె_Memories);

    కొరకు (u32 m = 0; m < అంకె_Memories; ++m) {
        u32       mem_సూచీ = 0, మొదటి_సూచీ = 0, పుట_బిట్లు = 0, అంకె_పుటలు = 0;
        IM3_జ్ఞాపకం జ్ఞాపకం;

_       (GetLEB_u32(l, &mem_సూచీ));
_       (GetLEB_u32(l, &మొదటి_సూచీ));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_సూచీ != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 మొదటి_సూచీ != జ్ఞాపకం_మొదటి_సూచీ(module, m));
        ఒకవేళ (మొదటి_సూచీ != m) {
            కొనసాగు;
        }

_       (GetLEB_u32(l, &పుట_బిట్లు));
_       (GetLEB_u32(l, &అంకె_పుటలు));

        జ్ఞాపకం = module->memories[mem_సూచీ];
        _throwif("a shared memory cannot be restored from a snapshot", జ్ఞాపకం and జ్ఞాపకం->isShared);

        size_t బైట్లు = 0;
        u8*    దత్తాంశం  = ఖాళీ;

        ఒకవేళ (జ్ఞాపకం) {
            _throwif(m3Err_wasmMalformed, పుట_బిట్లు >= 32 or (1u << పుట_బిట్లు) != జ్ఞాపకం_పుట_కొలత(జ్ఞాపకం));
            _throwif(m3Err_wasmMalformed, అంకె_పుటలు < జ్ఞాపకం->అంకె_పుటలు or అంకె_పుటలు > జ్ఞాపకం->max_పుటలు);
            _throwif(m3Err_wasmMalformed, అంకె_పుటలు > SIZE_MAX / జ్ఞాపకం_పుట_కొలత(జ్ఞాపకం));
            బైట్లు = (size_t)అంకె_పుటలు * జ్ఞాపకం_పుట_కొలత(జ్ఞాపకం);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->జ్ఞాపకం_హద్దు and బైట్లు > (size_t)l->runtime->జ్ఞాపకం_హద్దు);

            ఒకవేళ (not l->checking) {
                ఒకవేళ (జ్ఞాపకం->అంకె_పుటలు < అంకె_పుటలు) {
_                   (Resize_జ్ఞాపకం(l->runtime, జ్ఞాపకం, అంకె_పుటలు));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         జ్ఞాపకం->mallocated and జ్ఞాపకం->mallocated->నిడివి < బైట్లు);
            }

            l->loaded_జ్ఞాపకం_బైట్లు += బైట్లు;

            ఒకవేళ (బైట్లు and జ్ఞాపకం->mallocated) {
                దత్తాంశం = m3_Mem_దత్తాంశం(జ్ఞాపకం->mallocated);
            }
        } లేకపోతే {
            _throwif(m3Err_wasmMalformed, అంకె_పుటలు != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        ఒకవేళ (దత్తాంశం and not l->checking) {
            memset(దత్తాంశం, 0, బైట్లు);
        }

_       (ఎక్కించు_జ్ఞాపకం_Chunks(l, దత్తాంశం, బైట్లు));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_జ్ఞాపకం_బైట్లు != l->required_జ్ఞాపకం_బైట్లు);

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Globals (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం  ఫలితం     = m3Err_none;
    IM3Module module     = l->module;
    u32       అంకె_Globals = 0;

_   (GetLEB_u32(l, &అంకె_Globals));
    _throwif(m3Err_wasmMalformed, అంకె_Globals == 0 or అంకె_Globals != module->అంకె_Globals);

    కొరకు (u32 g = 0; g < అంకె_Globals; ++g) {
        u32       global_సూచీ = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_సూచీ));
        _throwif(m3Err_wasmMalformed, global_సూచీ != g);

        global = &module->globals[global_సూచీ];
        cell   = global->resolved ? global->resolved : global;

_       (Get_విలువ(l, global->type, &cell->i64_విలువ));
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Tables (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం  ఫలితం       = m3Err_none;
    IM3Module module       = l->module;
    u32       అంకె_Tables    = 0;
    l->loaded_పట్టిక_Elements = 0;

_   (GetLEB_u32(l, &అంకె_Tables));
    _throwif(m3Err_wasmMalformed, అంకె_Tables == 0 or అంకె_Tables != module->అంకె_Tables);

    కొరకు (u32 t = 0; t < అంకె_Tables; ++t) {
        u32      పట్టిక_సూచీ = 0, మొదటి_సూచీ = 0, కొలత = 0;
        u8       valtype = 0;
        IM3_పట్టిక పట్టిక_2;

_       (GetLEB_u32(l, &పట్టిక_సూచీ));
_       (GetLEB_u32(l, &మొదటి_సూచీ));

        _throwif(m3Err_wasmMalformed, పట్టిక_సూచీ != t);
        _throwif("the snapshot shares tables between imports differently",
                 మొదటి_సూచీ != పట్టిక_మొదటి_సూచీ(module, t));
        ఒకవేళ (మొదటి_సూచీ != t) {
            కొనసాగు;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &కొలత));

        పట్టిక_2 = module->tables[పట్టిక_సూచీ];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_గమ్యం_Val_Type(పట్టిక_2->type));
        _throwif(m3Err_wasmMalformed, కొలత > (పట్టిక_2->hasMax ? పట్టిక_2->max_కొలత : d_m3_Max_Sane_పట్టిక_కొలత));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, కొలత < పట్టిక_2->కొలత);

        l->loaded_పట్టిక_Elements += కొలత;

        ఒకవేళ (not l->checking and కొలత != పట్టిక_2->కొలత) {
            శూన్య** elements;

            _throwif(m3_Err_పట్టిక_హద్దు_Exceeded, l->runtime->పట్టిక_Elements_హద్దు and
                                                 కొలత - పట్టిక_2->కొలత > l->runtime->పట్టిక_Elements_హద్దు - l->runtime->పట్టిక_Elements_వాడిన);
            elements = m3_ReallocArray(శూన్య*, పట్టిక_2->elements, కొలత, పట్టిక_2->కొలత);
            _throwifnull(elements);

            l->runtime->పట్టిక_Elements_వాడిన += కొలత - పట్టిక_2->కొలత;
            పట్టిక_2->elements = elements;
            పట్టిక_2->కొలత     = కొలత;
        }

        కొరకు (u32 e = 0; e < కొలత; ++e) {
            శూన్య* reference = ఖాళీ;
_           (Get_Ref_విలువ(l, పట్టిక_2->type, &reference));
            ఒకవేళ (not l->checking) {
                పట్టిక_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_పట్టిక_Elements != l->required_పట్టిక_Elements);

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Segments (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం  ఫలితం = m3Err_none;
    IM3Module module = l->module;
    u32       లెక్క  = 0;

_   (GetLEB_u32(l, &లెక్క));
    _throwif(m3Err_wasmMalformed, లెక్క != module->అంకె_దత్తాంశం_Segments);
    _throwif(m3Err_wasmMalformed, module->అంకె_దత్తాంశం_Segments == 0 and module->అంకె_Element_Segments == 0);

    కొరకు (u32 i = 0; i < లెక్క; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ఒకవేళ (not l->checking) {
            module->దత్తాంశం_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &లెక్క));
    _throwif(m3Err_wasmMalformed, లెక్క != module->అంకె_Element_Segments);

    కొరకు (u32 i = 0; i < లెక్క; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ఒకవేళ (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    తిరిగి ఫలితం;
}

// A frame for the continuation being put together, outermost first
స్థిర
M3_ఫలితం కూడు_చట్రం (M3_Snapshot_ఎక్కించు* l, u32* io_అంకె_చట్రాలు, M3_చట్రం_రకం i_రకం, m3stack_t i_sp, M3_చట్రం** o_చట్రం)
{
    ఒకవేళ (*io_అంకె_చట్రాలు >= d_m3_Continuation_Max_చట్రాలు) {
        తిరిగి "the snapshot's frames do not fit this build's";
    }

    M3_చట్రం* చట్రం = &l->చట్రాలు[(*io_అంకె_చట్రాలు)++];

    memset(చట్రం, 0, పరిమాణం(*చట్రం));
    చట్రం->రకం   = (u8)i_రకం;
    చట్రం->sp     = i_sp;
    చట్రం->జ్ఞాపకం = Module_Memory0(l->module);

    *o_చట్రం = చట్రం;

    తిరిగి m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
స్థిర
M3_ఫలితం PrepareFunction (IM3Function i_function)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    ఒకవేళ (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    తిరిగి ఫలితం;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
స్థిర
M3_ఫలితం కూడు_ఖండం_చట్రాలు (M3_Snapshot_ఎక్కించు* l, స్థిరరాశి M3SnapshotMap* i_map, స్థిరరాశి M3_సురక్షిత_బిందువు* i_బిందువు, m3stack_t i_sp,
                         u32* io_అంకె_చట్రాలు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    కొరకు (u32 b = 0; b < i_map->అంకె_ఖండాలు; ++b) {
        స్థిరరాశి M3_ఖండం_ప్రారంభం* ఖండం = &i_map->ఖండాలు[b];
        M3_చట్రం*            చట్రం;

        ఒకవేళ (not Encloses_స్థానభ్రంశం(ఖండం, i_బిందువు->wasm_స్థానభ్రంశం)) {
            కొనసాగు;
        }

        ఒకవేళ (ఖండం->opcode == c_waOp_loop) {
_           (కూడు_చట్రం(l, io_అంకె_చట్రాలు, చట్రం_loop, i_sp, &చట్రం));
            చట్రం->pc = ఖండం->pc;
        }
#  ఒకవేళ d_m3HasExceptionHandling
        లేకపోతే {
_           (కూడు_చట్రం(l, io_అంకె_చట్రాలు, చట్రం_try, i_sp, &చట్రం));
            చట్రం->pc                = ఖండం->pc;
            చట్రం->try_.అంకె_Clauses   = ఖండం->అంకె_Clauses;
            చట్రం->try_.handlers_సజీవ = Handlers_సజీవ_At(i_map, i_బిందువు, ఖండం);
        }
#  endif
    }

_catch:
    తిరిగి ఫలితం;
}

// Puts a function's values where this build keeps them at the safepoint
స్థిర
M3_ఫలితం ఎక్కించు_విలువలు (M3_Snapshot_ఎక్కించు* l, m3slot_t* i_పునాది, u32 i_అంకె_Slots, m3stack_t i_sp,
                     స్థిరరాశి M3SnapshotMap* i_map, స్థిరరాశి M3_సురక్షిత_బిందువు* i_బిందువు, స్థిరరాశి M3_చట్రం_Registers* i_registers)
{
    M3_ఫలితం ఫలితం    = m3Err_none;
    u32      అంకె_విలువలు = 0;

_   (GetLEB_u32(l, &అంకె_విలువలు));
    _throwif("the snapshot's frame does not match what this build compiled", అంకె_విలువలు != i_map->అంకె_Locals + i_బిందువు->అంకె_విలువలు);

    కొరకు (u32 v = 0; v < అంకె_విలువలు; ++v) {
        స్థిరరాశి M3_స్థానం_విలువ* విలువ     = చట్రం_విలువ(i_map, i_బిందువు, v);
        u64                మాట      = 0;
        శూన్య*              reference = ఖాళీ;
        u8*                where;

_       (Locate_విలువ(i_పునాది, i_అంకె_Slots, i_sp, విలువ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ఒకవేళ (పునాది_Type_యొక్క(విలువ->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_విలువ_ట్యాగ్(l, విలువ->type, &type));
            ఒకవేళ (l->checking) {
_               (Skip_బైట్లు(l, 16));
            } లేకపోతే {
_               (Get(l, where, 16));
            }
            కొనసాగు;
        }

        l->allowUnfilledRef = i_బిందువు->రకం == safepoint_suspend and
                              v >= అంకె_విలువలు - i_బిందువు->అంకె_Results;
_       (Get_విలువ_బిట్లు(l, విలువ->type, &మాట));
        l->allowUnfilledRef = false;

        ఒకవేళ (IsRefType(విలువ->type)) {
            reference = (శూన్య*)(uintptr_t)మాట;
        }

        ఒకవేళ (not l->checking) {
_           (Write_విలువ(where, i_registers, విలువ, మాట, reference));
        }
    }

_catch:
    తిరిగి ఫలితం;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
స్థిర
M3_ఫలితం ఎక్కించు_Suspended_Body (M3_Snapshot_ఎక్కించు* l, IM3Continuation io_cont, u32 i_id)
{
    M3_ఫలితం    ఫలితం    = m3Err_none;
    u32         అంకె_చట్రాలు = 0;
    M3_చట్రం*    call_చట్రం = ఖాళీ;
    IM3Function function  = ఖాళీ;
    m3slot_t*   పునాది;
    u32         అంకె_Slots;
    u32         అంకె_Activations = 0;
    IM3FuncType expected       = io_cont->నమోదు_Function->funcType;
    m3stack_t   sp;

_   (Continuation_దొంతర(l->runtime, io_cont, &పునాది, &అంకె_Slots));

_   (GetLEB_u32(l, &అంకె_Activations));
    _throwif(m3Err_wasmMalformed, అంకె_Activations == 0 or అంకె_Activations > d_m3_Continuation_Max_చట్రాలు);

    io_cont->pc                = ఖాళీ;
    io_cont->r0                = 0;
    io_cont->అంకె_Suspend_Results = 0;
    io_cont->suspend_బిందువు      = safepoint_op;
#  ఒకవేళ d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = పునాది;

    కొరకు (u32 a = 0; a < అంకె_Activations; ++a) {
        bool                 isInnermost = (a + 1 == అంకె_Activations);
        స్థిరరాశి M3_సురక్షిత_బిందువు*   బిందువు;
        M3_చట్రం_Registers     registers;
        u8                   రకం       = 0;
        u32                  wasm_స్థానభ్రంశం = 0, లక్ష్యం_Loop = 0;
        స్థిరరాశి M3SnapshotMap* map;

        memset(&registers, 0, పరిమాణం(registers));

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
                 (u32)(sp - పునాది) + function->max_దొంతర_Slots > అంకె_Slots);

        // the call the frame outside this one is waiting on
        ఒకవేళ (call_చట్రం) {
            call_చట్రం->call.function = function;
        }

#  ఒకవేళ d_m3_నమోదు_Keeps_చట్రం
        {
            M3_చట్రం* నమోదు_2;
_           (కూడు_చట్రం(l, &అంకె_చట్రాలు, చట్రం_నమోదు, sp, &నమోదు_2));
            నమోదు_2->pc             = function->compiled;
            నమోదు_2->నమోదు_2.function = function;
        }
#  endif

_       (GetU8(l, &రకం));
_       (GetLEB_u32(l, &wasm_స్థానభ్రంశం));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        ఒకవేళ (రకం == safepoint_op) {
            స్థిరరాశి M3_ఖండం_ప్రారంభం* loop;

_           (GetLEB_u32(l, &లక్ష్యం_Loop));

            loop = కనుగొను_ఖండం_At(map, c_waOp_loop, లక్ష్యం_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_స్థానభ్రంశం(loop, wasm_స్థానభ్రంశం));
        }

        బిందువు = కనుగొను_సురక్షిత_బిందువు_At(map, wasm_స్థానభ్రంశం, రకం, లక్ష్యం_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not బిందువు);

_       (కూడు_ఖండం_చట్రాలు(l, map, బిందువు, sp, &అంకె_చట్రాలు));

        ఒకవేళ (not isInnermost) {
            _throwif(m3Err_wasmMalformed, రకం != safepoint_call);

_           (కూడు_చట్రం(l, &అంకె_చట్రాలు, చట్రం_call, sp, &call_చట్రం));
            call_చట్రం->pc = బిందువు->pc;
            registers.r0  = &call_చట్రం->call.r0;
#  ఒకవేళ d_m3HasFloat
            registers.fp0 = &call_చట్రం->call.fp0;
#  endif
        } లేకపోతే ఒకవేళ (రకం == safepoint_resume) {
            M3_చట్రం* చట్రం;
            u32      అంకె_Results = బిందువు->అంకె_Results;
            pc_t     resultsPC  = బిందువు->pc - 2 * అంకె_Results;

_           (కూడు_చట్రం(l, &అంకె_చట్రాలు, చట్రం_resume, sp, &చట్రం));
            చట్రం->pc                 = బిందువు->pc;
            చట్రం->resume.resultsPC   = resultsPC;
            చట్రం->resume.అంకె_Results  = అంకె_Results;
            చట్రం->resume.అంకె_Handlers = బిందువు->aux;
            చట్రం->resume.handlersPC  = resultsPC - 1 - 3 * బిందువు->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            call_చట్రం = చట్రం;
        } లేకపోతే {
            _throwif(m3Err_wasmMalformed, రకం != safepoint_op and రకం != safepoint_నమోదు and రకం != safepoint_suspend);

            io_cont->pc           = బిందువు->pc;
            io_cont->suspend_బిందువు = రకం;
            registers.r0          = &io_cont->r0;
#  ఒకవేళ d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (ఎక్కించు_విలువలు(l, పునాది, అంకె_Slots, sp, map, బిందువు, &registers));

        ఒకవేళ (not l->checking) {
            RestoreConstants(function, sp);
        }

        ఒకవేళ (not isInnermost) {
            sp = sp + బిందువు->aux;
_           (CallSiteType(function, బిందువు, &expected));
        } లేకపోతే ఒకవేళ (రకం == safepoint_resume) {
            శూన్య* cont = ఖాళీ;
_           (Get_Ref_విలువ(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ఖాళీ);

            call_చట్రం->resume.cont = (IM3Continuation)cont;
            ఒకవేళ (not l->checking) {
                l->suspendSites[i_id].బిందువు = బిందువు;
                కొరకు (u32 i = 1; i < l->అంకె_Continuations; ++i) {
                    ఒకవేళ (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].లక్ష్యం = i;
                        l->suspendSites[i].resumed   = true;
                        ఆపు;
                    }
                }
            }
        } లేకపోతే {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            ఒకవేళ (బిందువు->flags & d_m3_సురక్షిత_బిందువు_తీసుకున్న_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            ఒకవేళ (రకం == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, బిందువు->అంకె_Results > బిందువు->అంకె_విలువలు or
                                                బిందువు->అంకె_Results > d_m3MaxContinuationPayload);

                io_cont->అంకె_Suspend_Results = బిందువు->అంకె_Results;
                ఒకవేళ (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].బిందువు = బిందువు;
                }

                కొరకు (u32 i = 0; i < బిందువు->అంకె_Results; ++i) {
                    స్థిరరాశి M3_స్థానం_విలువ* విలువ = &map->విలువలు[బిందువు->మొదటి_విలువ + బిందువు->అంకె_విలువలు - బిందువు->అంకె_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_రిజిస్టరు_స్థానం_Alias(విలువ->స్థానం));

                    io_cont->suspend_ఫలితం_Offsets[i] = విలువ->స్థానం;
                    io_cont->suspend_ఫలితం_Is64[i]    = Is64_బిట్టు_Type(విలువ->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    ఒకవేళ (l->checking) {
        తిరిగి m3Err_none;
    }

    // held innermost first
    ఒకవేళ (అంకె_చట్రాలు > io_cont->చట్రాలు_Cap) {
        M3_చట్రం* చట్రాలు = m3_ReallocArray(M3_చట్రం, io_cont->చట్రాలు, అంకె_చట్రాలు, io_cont->చట్రాలు_Cap);
        _throwifnull(చట్రాలు);

        io_cont->చట్రాలు    = చట్రాలు;
        io_cont->చట్రాలు_Cap = అంకె_చట్రాలు;
    }

    కొరకు (u32 k = 0; k < అంకె_చట్రాలు; ++k) {
        io_cont->చట్రాలు[అంకె_చట్రాలు - 1 - k] = l->చట్రాలు[k];
    }

    io_cont->అంకె_చట్రాలు = అంకె_చట్రాలు;

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Continuation (M3_Snapshot_ఎక్కించు* l, u32 i_id)
{
    M3_ఫలితం        ఫలితం = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    ఒకవేళ (l->checking) {
        scratch          = *cont;
        scratch.val_దొంతర = l->check_దొంతర;
        ఒకవేళ (i_id == 0) {
            scratch.val_దొంతర = (m3slot_t*)l->runtime->origin_దొంతర;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_సూచీ = 0, నమోదు_సూచీ = 0, bound_వాదనలు_లెక్క = 0;
    u8  state = 0, is_మూలం = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_మూలం));
_   (GetLEB_u32(l, &type_సూచీ));
_   (GetLEB_u32(l, &నమోదు_సూచీ));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_మూలం != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_మూలం and state != snapshot_contSuspended);
    ఒకవేళ (not is_మూలం and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_సూచీ != d_m3SnapshotNone and type_సూచీ >= module->అంకె_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, నమోదు_సూచీ >= module->అంకె_Functions);
    _throwif(m3Err_wasmMalformed, is_మూలం ? type_సూచీ != d_m3SnapshotNone : (type_సూచీ == d_m3SnapshotNone or not module->funcTypes[type_సూచీ]->isContinuation));

    ఒకవేళ (not l->checking and not is_మూలం and state != snapshot_cont_ముగిసిన) {
_       (Continuation_Acquire_దొంతర(l->runtime, cont));
    }
    cont->type          = (type_సూచీ != d_m3SnapshotNone) ? module->funcTypes[type_సూచీ] : ఖాళీ;
    cont->నమోదు_Function = &module->functions[నమోదు_సూచీ];
    cont->అంకె_Handlers   = 0;
    cont->handlersPC    = ఖాళీ;
    cont->మాతృ        = ఖాళీ;
    cont->అంకె_చట్రాలు     = 0;

#  ఒకవేళ d_m3HasExceptionHandling
    {
        శూన్య* ref = ఖాళీ;
_       (Get_Ref_విలువ(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_మూలం or state == snapshot_cont_ముగిసిన) and cont->resumeThrow != ఖాళీ);
#  లేకపోతే
    {
        శూన్య* ref = ఖాళీ;
_       (Get_Ref_విలువ(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ఖాళీ);
    }
#  endif

    ఒకవేళ (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         అంకె_Rets;

_       (GetLEB_u32(l, &bound_వాదనలు_లెక్క));
        cont->bound_వాదనలు_లెక్క = bound_వాదనలు_లెక్క;
        inner                = cont->నమోదు_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_వాదనలు_లెక్క > Get_Func_Type_అంకె_Params(inner));

        {
            IM3FuncType మిగిలిన = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not మిగిలిన or
                                            Get_Func_Type_అంకె_Params(మిగిలిన) != Get_Func_Type_అంకె_Params(inner) - bound_వాదనలు_లెక్క or
                                            Get_Func_Type_అంకె_Results(మిగిలిన) != Get_Func_Type_అంకె_Results(inner));
            కొరకు (u16 i = 0; i < Get_Func_Type_అంకె_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ఫలితం_Type(inner, i) != Get_Func_Type_ఫలితం_Type(మిగిలిన, i));
            }
            కొరకు (u16 i = 0; i < Get_Func_Type_అంకె_Params(మిగిలిన); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_వాదనలు_లెక్క + i)) != GetFuncTypeParamType(మిగిలిన, i));
            }
        }

        అంకె_Rets = Get_Func_Type_అంకె_Results(inner);

        _throwif(m3Err_wasmMalformed, (అంకె_Rets + bound_వాదనలు_లెక్క) * c_io_స్థానం_లెక్క > cont->అంకె_దొంతర_Slots);

        కొరకు (u32 i = 0; i < bound_వాదనలు_లెక్క; ++i) {
_           (Get_విలువ(l, GetFuncTypeParamType(inner, (u16)i), cont->val_దొంతర + (అంకె_Rets + i) * c_io_స్థానం_లెక్క));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_దొంతర;
        cont->pc    = cont->నమోదు_Function->compiled;
    } లేకపోతే ఒకవేళ (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_వాదనలు_లెక్క));
        _throwif(m3Err_wasmMalformed, is_మూలం and bound_వాదనలు_లెక్క != 0);
        cont->bound_వాదనలు_లెక్క = bound_వాదనలు_లెక్క;

_       (ఎక్కించు_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } లేకపోతే ఒకవేళ (state == snapshot_cont_ముగిసిన) {
_       (GetLEB_u32(l, &bound_వాదనలు_లెక్క));
        _throwif(m3Err_wasmMalformed, bound_వాదనలు_లెక్క != 0);
        cont->bound_వాదనలు_లెక్క = 0;
        cont->state          = cont_consumed;
    } లేకపోతే {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ఒకవేళ (ఫలితం) {
        cont->అంకె_చట్రాలు = 0;
    }

    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Continuations (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u32      లెక్క  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &లెక్క));
    _throwif(m3Err_wasmMalformed, లెక్క == 0 or లెక్క != l->అంకె_Continuations);

    కొరకు (u32 i = 0; i < లెక్క; ++i) {
_       (ఎక్కించు_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    తిరిగి ఫలితం;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
స్థిర
M3_ఫలితం CheckContinuationReferences (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    కొరకు (u32 id = 0; id < l->అంకె_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             ఆకు  = id;
        u32             లోతు = 0;
#  ఒకవేళ d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_యొక్క(Ref_Type_యొక్క_Func_Type(cont->type, true), expectedType)));
#  endif
        ఒకవేళ (l->suspendSites[id].లక్ష్యం) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_యొక్క_Ref(l->module, l->suspendSites[id].బిందువు->resumeType);
            IM3Continuation బిడ్డ    = l->continuations[l->suspendSites[id].లక్ష్యం];
            IM3FuncType     actual   = బిడ్డ->నమోదు_Function ? బిడ్డ->నమోదు_Function->funcType : ఖాళీ;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_అంకె_Results(actual) != Get_Func_Type_అంకె_Results(expected));
            కొరకు (u16 i = 0; i < Get_Func_Type_అంకె_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ఫలితం_Type(actual, i) != Get_Func_Type_ఫలితం_Type(expected, i));
            }
        }
        అయితే (l->suspendSites[ఆకు].లక్ష్యం) {
            ఆకు = l->suspendSites[ఆకు].లక్ష్యం;
            _throwif(m3Err_wasmMalformed, ++లోతు >= l->అంకె_Continuations or
                                            l->continuations[ఆకు]->state != cont_suspended);
        }
        ఒకవేళ (cont->state != cont_suspended or id == 0) {
            కొనసాగు;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        ఒకవేళ (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_వాదనలు_లెక్క != 0);
            కొనసాగు;
        }

        స్థిరరాశి M3SnapshotSuspendSite* site      = &l->suspendSites[ఆకు];
        IM3Continuation              inner     = l->continuations[ఆకు];
        IM3FuncType                  మిగిలిన = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->బిందువు or not మిగిలిన);
        _throwif("a captured continuation has the wrong binding count", cont->bound_వాదనలు_లెక్క > inner->అంకె_Suspend_Results or
                                                                          Get_Func_Type_అంకె_Params(మిగిలిన) != inner->అంకె_Suspend_Results - cont->bound_వాదనలు_లెక్క);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_అంకె_Results(మిగిలిన) != Get_Func_Type_అంకె_Results(cont->నమోదు_Function->funcType));
        కొరకు (u16 i = 0; i < Get_Func_Type_అంకె_Results(మిగిలిన); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ఫలితం_Type(మిగిలిన, i) !=
                                            Get_Func_Type_ఫలితం_Type(cont->నమోదు_Function->funcType, i));
        }

        కొరకు (u32 i = 0; i < inner->అంకె_Suspend_Results; ++i) {
            స్థిరరాశి M3_స్థానం_విలువ* విలువ = &site->map->విలువలు[site->బిందువు->మొదటి_విలువ +
                                                          site->బిందువు->అంకె_విలువలు - site->బిందువు->అంకె_Results + i];
            ఒకవేళ (i < cont->bound_వాదనలు_లెక్క) {
                ఒకవేళ (IsRefType(విలువ->type) and not IsNullableRef(విలువ->type)) {
                    శూన్య* ref = ఖాళీ;
                    జ్ఞాపకం_నకలు(&ref, inner->sp + విలువ->స్థానం, పరిమాణం(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } లేకపోతే {
                _throwif("a captured continuation has the wrong parameter type", విలువ->type !=
                                                                                   GetFuncTypeParamType(మిగిలిన, (u16)(i - cont->bound_వాదనలు_లెక్క)));
            }
        }
    }

_catch:
    తిరిగి ఫలితం;
}

#  ఒకవేళ d_m3HasExceptionHandling

స్థిర
M3_ఫలితం ఎక్కించు_Exceptions (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం ఫలితం        = m3Err_none;
    u32      అంకె_Exceptions = 0;

_   (GetLEB_u32(l, &అంకె_Exceptions));
    _throwif(m3Err_wasmMalformed, అంకె_Exceptions == 0 or అంకె_Exceptions != l->అంకె_Exceptions);

    కొరకు (u32 i = 0; i < అంకె_Exceptions; ++i) {
        u32          exnId = 0, అంకె_వాదనలు = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &అంకె_వాదనలు));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->ట్యాగ్->type;

        _throwif(m3Err_wasmMalformed, అంకె_వాదనలు != exn->అంకె_వాదనలు);

        కొరకు (u32 a = 0; a < అంకె_వాదనలు; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      బిట్లు    = 0;
_           (Get_విలువ_బిట్లు(l, argType, &బిట్లు));
            ఒకవేళ (not l->checking) {
                exn->వాదనలు[a] = బిట్లు;
            }
        }
    }

_catch:
    తిరిగి ఫలితం;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
రకనిర్వచనం నిర్మాణం M3BoundedReader {
    M3_Snapshot_ఎక్కించు* ఎక్కించు;
    u64             మిగిలిన;
    M3_ఫలితం        error;
} M3BoundedReader;

స్థిర
M3_ఫలితం BoundedReader_Read (శూన్య* o_బఫరు, size_t i_కొలత, శూన్య* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    ఒకవేళ (i_కొలత > br->మిగిలిన) {
        br->error = "the host read past the end of its state";
        తిరిగి br->error;
    }

    br->మిగిలిన -= i_కొలత;

    తిరిగి Get(br->ఎక్కించు, o_బఫరు, i_కొలత);
}

స్థిర
M3_ఫలితం ఎక్కించు_Host_State (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం               ఫలితం = m3Err_none;
    స్థిరరాశి M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 కొలత = (size_t)(l->చివర - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->ఎక్కించు_Host_State_2);

    ఒకవేళ (l->checking) {
        తిరిగి Skip_బైట్లు(l, కొలత);
    }
    reader.ఎక్కించు      = l;
    reader.మిగిలిన = కొలత;
    reader.error     = m3Err_none;

_   (hooks->ఎక్కించు_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, కొలత));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.మిగిలిన != 0);

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_శీర్షిక (M3_Snapshot_ఎక్కించు* l)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u8       magic[పరిమాణం(c_snapshotMagic)];
    u32      సంస్కరణ = 0;

_   (Get(l, magic, పరిమాణం(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, పరిమాణం(magic)) != 0);

_   (GetLE32(l, &సంస్కరణ));
    _throwif(m3Err_wasmMalformed, సంస్కరణ != c_snapshot_సంస్కరణ);

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Meta (M3_Snapshot_ఎక్కించు* l, u32* o_flags)
{
    M3_ఫలితం ఫలితం      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      జ్ఞాపకం_బైట్లు = 0, పట్టిక_Elements = 0;
    u32      చురుకైన_Stacks = 0;
    u64      current_జ్ఞాపకం, currentTables;
    SnapshotResourceTotals(l->module, &current_జ్ఞాపకం, &currentTables);

_   (GetLEB_u32(l, o_flags));
_   (GetLEB_u64(l, &timestamp));
_   (GetLEB_u64(l, &moduleHash));

    (శూన్య)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->అంకె_Continuations));
_   (GetLEB_u32(l, &l->అంకె_Exceptions));
_   (GetLEB_u64(l, &జ్ఞాపకం_బైట్లు));
_   (GetLEB_u64(l, &పట్టిక_Elements));
_   (GetLEB_u32(l, &చురుకైన_Stacks));
    l->requiredStacks        = చురుకైన_Stacks;
    l->required_జ్ఞాపకం_బైట్లు   = జ్ఞాపకం_బైట్లు;
    l->required_పట్టిక_Elements = పట్టిక_Elements;
    _throwif(m3Err_wasmMalformed, జ్ఞాపకం_బైట్లు < current_జ్ఞాపకం or పట్టిక_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->అంకె_Memories and జ్ఞాపకం_బైట్లు != 0);
    _throwif(m3Err_wasmMalformed, not l->module->అంకె_Tables and పట్టిక_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    ఒకవేళ (*o_flags & d_m3SnapshotFlagPostmortem) {
        కొరకు (u32 i = 0; i < l->అంకె_Exceptions; ++i) {
            u32 ట్యాగ్_సూచీ = 0;
_           (GetLEB_u32(l, &ట్యాగ్_సూచీ));
        }
        తిరిగి m3Err_none;
    }

    _throwif(m3_Err_జ్ఞాపకం_హద్దు_Exceeded, l->runtime->జ్ఞాపకం_బైట్లు_హద్దు and
                                          జ్ఞాపకం_బైట్లు - current_జ్ఞాపకం > l->runtime->జ్ఞాపకం_బైట్లు_హద్దు - l->runtime->జ్ఞాపకం_బైట్లు_వాడిన);
    _throwif(m3_Err_పట్టిక_హద్దు_Exceeded, l->runtime->పట్టిక_Elements_హద్దు and
                                         పట్టిక_Elements - currentTables > l->runtime->పట్టిక_Elements_హద్దు - l->runtime->పట్టిక_Elements_వాడిన);
    _throwif(m3_Err_continuation_హద్దు_Exceeded, l->runtime->continuations_హద్దు and
                                                చురుకైన_Stacks > l->runtime->continuations_హద్దు - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->అంకె_Continuations == 0 or చురుకైన_Stacks >= l->అంకె_Continuations);
#  ఒకవేళ !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->అంకె_Exceptions != 0);
#  endif

    ఒకవేళ (l->checking) {
        కొరకు (u32 i = 0; i < l->అంకె_Exceptions; ++i) {
            u32 ట్యాగ్_సూచీ = 0;
_           (GetLEB_u32(l, &ట్యాగ్_సూచీ));
#  ఒకవేళ d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, ట్యాగ్_సూచీ >= l->module->అంకె_ట్యాగులు);
#  endif
        }
        తిరిగి m3Err_none;
    }

    l->check_దొంతర = m3_AllocArray(m3slot_t, d_m3_Continuation_దొంతర_Slots + 4);
    _throwifnull(l->check_దొంతర);
    l->చట్రాలు = m3_AllocArray(M3_చట్రం, d_m3_Continuation_Max_చట్రాలు);
    _throwifnull(l->చట్రాలు);

    l->continuations = m3_AllocArray(IM3Continuation, l->అంకె_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->అంకె_Continuations);
    _throwifnull(l->suspendSites);

    ఒకవేళ (not l->runtime->మూలం_Continuation) {
        IM3Continuation మూలం = Continuation_New(l->runtime, ఖాళీ, ఖాళీ);
        _throwifnull(మూలం);

        // the root borrows the runtime's stack, so it holds none of its own
        మూలం->అంకె_దొంతర_Slots          = l->runtime->అంకె_దొంతర_Slots;
        l->runtime->మూలం_Continuation = మూలం;
    }

    l->continuations[0] = l->runtime->మూలం_Continuation;

    కొరకు (u32 i = 1; i < l->అంకె_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ఖాళీ, ఖాళీ);
        _throwifnull(l->continuations[i]);
    }

#  ఒకవేళ d_m3HasExceptionHandling
    ఒకవేళ (l->అంకె_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->అంకె_Exceptions);
        _throwifnull(l->exceptions);
    }

    కొరకు (u32 i = 0; i < l->అంకె_Exceptions; ++i) {
        u32    ట్యాగ్_సూచీ = 0;
        IM3_ట్యాగ్ ట్యాగ్;
        u32    అంకె_వాదనలు;

_       (GetLEB_u32(l, &ట్యాగ్_సూచీ));
        _throwif(m3Err_wasmMalformed, ట్యాగ్_సూచీ >= l->module->అంకె_ట్యాగులు);

        ట్యాగ్     = ట్యాగ్_యొక్క_సూచీ(l->module, ట్యాగ్_సూచీ);
        అంకె_వాదనలు = Get_Func_Type_అంకె_Params(ట్యాగ్->type);

        l->exceptions[i] = NewException(l->runtime, ట్యాగ్, అంకె_వాదనలు);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  లేకపోతే
    కొరకు (u32 i = 0; i < l->అంకె_Exceptions; ++i) {
        u32 ట్యాగ్_సూచీ = 0;
_       (GetLEB_u32(l, &ట్యాగ్_సూచీ));
    }
#  endif

_catch:
    తిరిగి ఫలితం;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
స్థిర
M3_ఫలితం ఎక్కించు_Section (M3_Snapshot_ఎక్కించు* l, u8 i_sectionId)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    u32      flags  = 0;
    ఎంపిక (i_sectionId) {
    స్థితి d_m3SnapshotSection_Meta:
_       (ఎక్కించు_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        ఆపు;

    స్థితి d_m3_Snapshot_Section_జ్ఞాపకం:
_       (ఎక్కించు_Memories(l));
        ఆపు;

    స్థితి d_m3_Snapshot_Section_పట్టిక:
_       (ఎక్కించు_Tables(l));
        ఆపు;

    స్థితి d_m3SnapshotSection_Global:
_       (ఎక్కించు_Globals(l));
        ఆపు;

    స్థితి d_m3SnapshotSection_Segment:
_       (ఎక్కించు_Segments(l));
        ఆపు;

#  ఒకవేళ d_m3HasExceptionHandling
    స్థితి d_m3SnapshotSection_Exception:
_       (ఎక్కించు_Exceptions(l));
        ఆపు;
#  endif

    స్థితి d_m3SnapshotSection_Continuation:
_       (ఎక్కించు_Continuations(l));
        ఆపు;

    స్థితి d_m3SnapshotSection_HostState:
_       (ఎక్కించు_Host_State(l));
        ఆపు;

    అప్రమేయ:
_       (Skip_బైట్లు(l, (size_t)(l->చివర - l->pos)));
        ఆపు;
    }

_catch:
    తిరిగి ఫలితం;
}

స్థిర
M3_ఫలితం ఎక్కించు_Sections (M3_Snapshot_ఎక్కించు* l, bool i_embedded)
{
    M3_ఫలితం ఫలితం  = m3Err_none;
    u8*      payload = ఖాళీ;

    ఒకవేళ (not i_embedded) {
_       (ఎక్కించు_శీర్షిక(l));
    }

    అయితే (true) {
        u8  sectionId   = 0;
        u32 section_కొలత = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        ఒకవేళ (GetU8(l, &sectionId)) {
            ఆపు;
        }

_       (GetLEB_u32(l, &section_కొలత));

        _throwif("the snapshot holds a section twice", l->చూసిన_Sections[sectionId]);
        l->చూసిన_Sections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_కొలత == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_కొలత);
        _throwifnull(payload);
_       (Get(l, payload, section_కొలత));
        l->pos       = payload;
        l->చివర       = payload + section_కొలత;
        l->లోపల_Section = true;

        {
            M3_Snapshot_ఎక్కించు check = *l;
            check.checking       = true;
_           (ఎక్కించు_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.చివర);
        }
        l->మారింది = l->మారింది or sectionId != d_m3SnapshotSection_Meta;
_       (ఎక్కించు_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->చివర);
        l->లోపల_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->చూసిన_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->అంకె_Exceptions and not l->చూసిన_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->అంకె_Memories and not l->చూసిన_Sections[d_m3_Snapshot_Section_జ్ఞాపకం]);
    _throwif("the snapshot has no table section",
             l->module->అంకె_Tables and not l->చూసిన_Sections[d_m3_Snapshot_Section_పట్టిక]);
    _throwif("the snapshot has no global section",
             l->module->అంకె_Globals and not l->చూసిన_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->అంకె_దత్తాంశం_Segments or l->module->అంకె_Element_Segments) and
               not l->చూసిన_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#ఒకవేళ d_m3HasSnapshots

స్థిర
M3_ఫలితం SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, శూన్య* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_ఫలితం       ఫలితం = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, పరిమాణం(save));

    IM3Continuation మూలం = io_runtime->మూలం_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not మూలం or మూలం->state != cont_suspended;

    IM3Function నమోదు_2 = save.postmortem ? io_runtime->entered : మూలం->నమోదు_Function;

    ఒకవేళ (o_module) {
        *o_module = ఖాళీ;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not నమోదు_2);

    save.module = నమోదు_2->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    ఒకవేళ (o_module) {
        *o_module = save.module;
    }

    కొరకు (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->తదుపరి) {
        u32 id;
_       (సూచిక_Ids_కూడు(&save.సజీవ_Continuations, cont, &id));
    }

#  ఒకవేళ d_m3HasExceptionHandling
    కొరకు (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->తదుపరి) {
        u32 id;
_       (సూచిక_Ids_కూడు(&save.సజీవ_Exceptions, exception, &id));
    }
#  endif

    ఒకవేళ (not save.postmortem) {
        u32 id;
_       (సూచిక_Ids_కూడు(&save.continuations, మూలం, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    సూచిక_Ids_Free(&save.సజీవ_Continuations);
    సూచిక_Ids_Free(&save.సజీవ_Exceptions);
    సూచిక_Ids_Free(&save.continuations);
    సూచిక_Ids_Free(&save.exceptions);

    తిరిగి ఫలితం;
}

#అయితే_ముగింపు

M3_ఫలితం m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, శూన్య* i_userdata)
{
    ఒకవేళ (not io_runtime or not i_writer) {
        తిరిగి m3Err_mallocFailed;
    }

#ఒకవేళ d_m3HasSnapshots
    తిరిగి SaveSnapshot(io_runtime, i_writer, i_userdata, ఖాళీ, false);
#లేకపోతే
    (శూన్య)i_userdata;
    తిరిగి "snapshots are not available in this build of Wasm3";
#అయితే_ముగింపు
}


స్థిర
M3_ఫలితం ఎక్కించు_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, శూన్య* i_userdata, bool i_embedded)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#ఒకవేళ d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_నిరంతరం);

    {
        M3_Snapshot_ఎక్కించు ఎక్కించు;
        memset(&ఎక్కించు, 0, పరిమాణం(ఎక్కించు));

        ఎక్కించు.runtime  = io_runtime;
        ఎక్కించు.module   = i_module;
        ఎక్కించు.reader   = i_reader;
        ఎక్కించు.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        ఫలితం = ఎక్కించు_Sections(&ఎక్కించు, i_embedded);

        IM3Continuation మూలం = io_runtime->మూలం_Continuation;

        ఒకవేళ (ఫలితం) {
            ఒకవేళ (ఎక్కించు.మారింది) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                ఒకవేళ (మూలం) {
                    మూలం->state     = cont_returned;
                    మూలం->అంకె_చట్రాలు = 0;
                }
            }
        } లేకపోతే {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->ప్రారంభం_Function = -1;
            i_module->has_నిరంతరం        = true;
        }

        Runtime_ఉంచు_Call_దొంతర(io_runtime);

        m3_Free(ఎక్కించు.check_దొంతర);
        m3_Free(ఎక్కించు.చట్రాలు);
        m3_Free(ఎక్కించు.suspendSites);
        m3_Free(ఎక్కించు.continuations);
#  ఒకవేళ d_m3HasExceptionHandling
        m3_Free(ఎక్కించు.exceptions);
#  endif
    }
#లేకపోతే
    (శూన్య)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#అయితే_ముగింపు

_catch:
    తిరిగి ఫలితం;
}


M3_ఫలితం m3_ఎక్కించు_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, శూన్య* i_userdata)
{
    తిరిగి ఎక్కించు_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


శూన్య m3_సమితి_Snapshot_Hooks (IM3Runtime io_runtime, స్థిరరాశి M3SnapshotHooks* i_hooks)
{
#ఒకవేళ d_m3HasSnapshots
    ఒకవేళ (io_runtime) {
        ఒకవేళ (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } లేకపోతే {
            memset(&io_runtime->snapshotHooks, 0, పరిమాణం(io_runtime->snapshotHooks));
        }
    }
#లేకపోతే
    (శూన్య)io_runtime;
    (శూన్య)i_hooks;
#అయితే_ముగింపు
}


M3_ఫలితం m3_Save_Snapshot_గమ్యం_బఫరు (IM3Runtime io_runtime, శూన్య** o_బైట్లు, size_t* o_కొలత)
{
    ఒకవేళ (!io_runtime || !o_బైట్లు || !o_కొలత) {
        తిరిగి m3Err_mallocFailed;
    }
#ఒకవేళ d_m3HasSnapshots
    M3_బఫరు_Writer bw     = { ఖాళీ, 0, 0 };
    M3_ఫలితం       ఫలితం = m3_SaveSnapshot(io_runtime, బఫరు_Writer_Write, &bw);
    ఒకవేళ (ఫలితం) {
        m3_Free(bw.బఫరు);
        *o_బైట్లు = ఖాళీ;
        *o_కొలత  = 0;
        తిరిగి ఫలితం;
    }
    *o_బైట్లు = bw.బఫరు;
    *o_కొలత  = bw.కొలత;
    తిరిగి m3Err_none;
#లేకపోతే
    *o_బైట్లు = ఖాళీ;
    *o_కొలత  = 0;
    తిరిగి "snapshots are not available in this build of Wasm3";
#అయితే_ముగింపు
}

#ఒకవేళ d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
స్థిర
u8* EmitLEB_u32 (u8* o_where, u32 i_విలువ)
{
    చేయి {
        u8 byte = i_విలువ & 0x7F;
        i_విలువ >>= 7;
        ఒకవేళ (i_విలువ != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } అయితే (i_విలువ != 0);

    తిరిగి o_where;
}

స్థిర
u32 కొలత_యొక్క_LEB_u32 (u32 i_విలువ)
{
    u32 కొలత = 0;

    చేయి {
        i_విలువ >>= 7;
        ++కొలత;
    } అయితే (i_విలువ != 0);

    తిరిగి కొలత;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
స్థిర
M3_ఫలితం కనుగొను_Embedded_Snapshot (స్థిరరాశి u8* i_ప్రారంభం, స్థిరరాశి u8* i_చివర, cstr_t i_secName,
                               స్థిరరాశి u8** o_sec_ప్రారంభం, స్థిరరాశి u8** o_sec_చివర,
                               స్థిరరాశి u8** o_దత్తాంశం, u32* o_కొలత)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    బైట్లు_t  pos    = i_ప్రారంభం;
    bool     దొరికింది  = false;
    cstr_t   name   = ఖాళీ;

    ఒకవేళ (o_sec_ప్రారంభం) {
        *o_sec_ప్రారంభం = ఖాళీ;
    }
    ఒకవేళ (o_sec_చివర) {
        *o_sec_చివర = ఖాళీ;
    }
    ఒకవేళ (o_దత్తాంశం) {
        *o_దత్తాంశం = ఖాళీ;
    }
    ఒకవేళ (o_కొలత) {
        *o_కొలత = 0;
    }

    _throwif(m3Err_wasmMalformed, i_చివర - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    అయితే (pos < i_చివర) {
        బైట్లు_t sec_ప్రారంభం  = pos;
        u8      sectionId = *pos++;
        u32     కొలత      = 0;
_       (ReadLEB_u32(&కొలత, &pos, i_చివర));
        _throwif(m3Err_wasmUnderrun, కొలత > (size_t)(i_చివర - pos));
        {
            బైట్లు_t payload = pos;
            pos += కొలత;
            ఒకవేళ (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                ఒకవేళ (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", దొరికింది);
                    దొరికింది = true;
                    ఒకవేళ (o_sec_ప్రారంభం) {
                        *o_sec_ప్రారంభం = sec_ప్రారంభం;
                    }
                    ఒకవేళ (o_sec_చివర) {
                        *o_sec_చివర = pos;
                    }
                    ఒకవేళ (o_దత్తాంశం) {
                        *o_దత్తాంశం = payload;
                    }
                    ఒకవేళ (o_కొలత) {
                        *o_కొలత = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    తిరిగి ఫలితం;
}

// Builds the custom section name a snapshot of i_name lives under
స్థిర
M3_ఫలితం SnapshotSectionName (cstr_t i_name, అక్షరం* o_secName, size_t i_కొలత)
{
    ఒకవేళ (i_name and *i_name) {
        ఒకవేళ (అక్షరశ్రేణి_పొడవు(i_name) > d_m3_Max_Snapshot_Name_నిడివి) {
            తిరిగి "the snapshot name is too long";
        }
        snprintf(o_secName, i_కొలత, "snapshot.%s", i_name);
    } లేకపోతే {
        snprintf(o_secName, i_కొలత, "snapshot");
    }

    తిరిగి m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
స్థిర
M3_ఫలితం ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_ఫలితం ఫలితం = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ఖాళీ;

    కొరకు (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->తదుపరి) {
        ఒకవేళ (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    తిరిగి ఫలితం;
}
#అయితే_ముగింపు

M3_ఫలితం m3_ఎక్కించు_Snapshot_From_బఫరు (IM3Runtime io_runtime, IM3Module i_module, స్థిరరాశి శూన్య* i_బైట్లు, size_t i_కొలత)
{
    ఒకవేళ (!io_runtime || !i_module || !i_బైట్లు) {
        తిరిగి m3Err_mallocFailed;
    }
#ఒకవేళ d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    ఒకవేళ (i_కొలత >= 8 && memcmp(i_బైట్లు, "\0asm", 4) == 0) {
        స్థిరరాశి u8* దత్తాంశం   = ఖాళీ;
        u32       కొలత   = 0;
        M3_ఫలితం  ఫలితం = కనుగొను_Embedded_Snapshot((స్థిరరాశి u8*)i_బైట్లు, (స్థిరరాశి u8*)i_బైట్లు + i_కొలత,
                                                "snapshot", ఖాళీ, ఖాళీ, &దత్తాంశం, &కొలత);
        ఒకవేళ (ఫలితం) {
            తిరిగి ఫలితం;
        }
        ఒకవేళ (not దత్తాంశం) {
            తిరిగి "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_బఫరు_Reader br = { దత్తాంశం, కొలత, 0 };
            తిరిగి ఎక్కించు_Snapshot(io_runtime, i_module, బఫరు_Reader_Read, &br, true);
        }
    }
    {
        M3_బఫరు_Reader br = { (స్థిరరాశి u8*)i_బైట్లు, i_కొలత, 0 };
        తిరిగి ఎక్కించు_Snapshot(io_runtime, i_module, బఫరు_Reader_Read, &br,
                            i_కొలత < 4 or memcmp(i_బైట్లు, c_snapshotMagic, 4) != 0);
    }
#లేకపోతే
    (శూన్య)i_కొలత;
    తిరిగి "snapshots are not available in this build of Wasm3";
#అయితే_ముగింపు
}

bool m3_HasSnapshot (IM3Module i_module, స్థిరరాశి అక్షరం* i_name)
{
    ఒకవేళ (not i_module) {
        తిరిగి false;
    }
#ఒకవేళ d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ఖాళీ;
    (శూన్య)ModuleSnapshot(i_module, i_name, &snap);

    తిరిగి snap != ఖాళీ;
#లేకపోతే
    (శూన్య)i_name;
    తిరిగి false;
#అయితే_ముగింపు
}

M3_ఫలితం m3_GetEmbeddedSnapshot (IM3Module i_module, స్థిరరాశి అక్షరం* i_name,
                                 స్థిరరాశి శూన్య** o_బైట్లు, size_t* o_కొలత)
{
    ఒకవేళ (not i_module or not o_బైట్లు or not o_కొలత) {
        తిరిగి m3Err_mallocFailed;
    }

    *o_బైట్లు = ఖాళీ;
    *o_కొలత  = 0;

#ఒకవేళ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ఖాళీ;
        M3_ఫలితం            ఫలితం = ModuleSnapshot(i_module, i_name, &snap);

        ఒకవేళ (ఫలితం) {
            తిరిగి ఫలితం;
        }
        ఒకవేళ (not snap) {
            తిరిగి "the module holds no embedded snapshot of that name";
        }

        *o_బైట్లు = snap->దత్తాంశం;
        *o_కొలత  = snap->కొలత;

        తిరిగి m3Err_none;
    }
#లేకపోతే
    (శూన్య)i_name;
    తిరిగి "snapshots are not available in this build of Wasm3";
#అయితే_ముగింపు
}

M3_ఫలితం m3_ఎక్కించు_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, స్థిరరాశి అక్షరం* i_name)
{
    ఒకవేళ (not io_runtime or not i_module) {
        తిరిగి m3Err_mallocFailed;
    }
#ఒకవేళ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ఖాళీ;
        M3_ఫలితం            ఫలితం = ModuleSnapshot(i_module, i_name, &snap);
        M3_బఫరు_Reader      br;

        ఒకవేళ (ఫలితం) {
            తిరిగి ఫలితం;
        }
        ఒకవేళ (not snap) {
            తిరిగి "the module holds no embedded snapshot of that name";
        }

        br.బఫరు = snap->దత్తాంశం;
        br.కొలత   = snap->కొలత;
        br.cursor = 0;

        తిరిగి ఎక్కించు_Snapshot(io_runtime, i_module, బఫరు_Reader_Read, &br, true);
    }
#లేకపోతే
    (శూన్య)i_name;
    తిరిగి "snapshots are not available in this build of Wasm3";
#అయితే_ముగింపు
}


M3_ఫలితం m3_Save_Snapshot_గమ్యం_Module (IM3Runtime io_runtime, IM3Module i_module, స్థిరరాశి అక్షరం* i_name,
                                  శూన్య** o_బైట్లు, size_t* o_కొలత)
{
#ఒకవేళ d_m3HasSnapshots
    M3_ఫలితం       ఫలితం = m3Err_none;
    M3_బఫరు_Writer bw     = { ఖాళీ, 0, 0 };
    u8*            బహిర్గతం    = ఖాళీ;

    అక్షరం      secName[d_m3_Max_Snapshot_Name_నిడివి + పరిమాణం("snapshot.")];
    u32       nameLen, name_Len_Leb_కొలత, payload_కొలత, payload_కొలత_Leb_కొలత;
    size_t    orig_కొలత, మొత్తం_కొలత;
    స్థిరరాశి u8* drop_ప్రారంభం = ఖాళీ;
    స్థిరరాశి u8* drop_చివర   = ఖాళీ;
    IM3Module saved     = ఖాళీ;
    u8*       p;

    ఒకవేళ (not io_runtime or not i_module or not o_బైట్లు or not o_కొలత) {
        తిరిగి m3Err_mallocFailed;
    }

    *o_బైట్లు = ఖాళీ;
    *o_కొలత  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_ప్రారంభం or i_module->wasm_చివర <= i_module->wasm_ప్రారంభం);

_   (SnapshotSectionName(i_name, secName, పరిమాణం(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->మూలం_Continuation or io_runtime->మూలం_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, బఫరు_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.కొలత);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.కొలత > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (కనుగొను_Embedded_Snapshot(i_module->wasm_ప్రారంభం, i_module->wasm_చివర, secName, &drop_ప్రారంభం, &drop_చివర, ఖాళీ, ఖాళీ));

    orig_కొలత           = (size_t)(i_module->wasm_చివర - i_module->wasm_ప్రారంభం) - (drop_ప్రారంభం ? (size_t)(drop_చివర - drop_ప్రారంభం) : 0);
    nameLen            = (u32)అక్షరశ్రేణి_పొడవు(secName);
    name_Len_Leb_కొలత     = కొలత_యొక్క_LEB_u32(nameLen);
    payload_కొలత        = name_Len_Leb_కొలత + nameLen + (u32)bw.కొలత;
    payload_కొలత_Leb_కొలత = కొలత_యొక్క_LEB_u32(payload_కొలత);
    మొత్తం_కొలత          = orig_కొలత + 1 + payload_కొలత_Leb_కొలత + payload_కొలత;

    బహిర్గతం = (u8*)m3_Malloc("wasm_snapshot_module", మొత్తం_కొలత);
    _throwifnull(బహిర్గతం);

    p = బహిర్గతం;

    ఒకవేళ (drop_ప్రారంభం) {
        size_t before = (size_t)(drop_ప్రారంభం - i_module->wasm_ప్రారంభం);
        size_t after  = (size_t)(i_module->wasm_చివర - drop_చివర);

        జ్ఞాపకం_నకలు(p, i_module->wasm_ప్రారంభం, before);
        p += before;
        జ్ఞాపకం_నకలు(p, drop_చివర, after);
        p += after;
    } లేకపోతే {
        జ్ఞాపకం_నకలు(p, i_module->wasm_ప్రారంభం, orig_కొలత);
        p += orig_కొలత;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_కొలత);
    p    = EmitLEB_u32(p, nameLen);
    జ్ఞాపకం_నకలు(p, secName, nameLen);
    p += nameLen;
    జ్ఞాపకం_నకలు(p, bw.బఫరు, bw.కొలత);

    *o_బైట్లు = బహిర్గతం;
    *o_కొలత  = మొత్తం_కొలత;
    బహిర్గతం      = ఖాళీ;

_catch:
    m3_Free(bw.బఫరు);
    m3_Free(బహిర్గతం);

    తిరిగి ఫలితం;
#లేకపోతే
    (శూన్య)io_runtime;
    (శూన్య)i_module;
    (శూన్య)i_name;
    (శూన్య)o_బైట్లు;
    (శూన్య)o_కొలత;
    తిరిగి "snapshots are not available in this build of Wasm3";
#అయితే_ముగింపు
}
