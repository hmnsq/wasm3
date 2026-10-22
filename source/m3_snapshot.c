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

#ಯದಿ d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
ಸ್ಥಿರ ಧ್ರುವ u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
ಸ್ಥಿರ ಧ್ರುವ u32 c_snapshotVersion  = 1;

ಗಣನಾ {
    d_m3SnapshotSection_Meta         = 0,
    d_m3SnapshotSection_Memory       = 1,
    d_m3SnapshotSection_Table        = 2,
    d_m3SnapshotSection_Global       = 3,
    d_m3SnapshotSection_Segment      = 4,
    d_m3SnapshotSection_Exception    = 5,
    d_m3SnapshotSection_Continuation = 6,
    d_m3SnapshotSection_HostState    = 7,
};

#  define d_m3SnapshotFlagPostmortem  0x1

// The longest <name> in a "snapshot.<name>" custom section. A name is a label
// the embedder picks, not something the format needs room to grow.
#  define d_m3MaxSnapshotNameLength   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
ಗಣನಾ {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_contFinished  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3ChunkEnd                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//  identity
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ಸ್ಥಿರ
M3Result ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3Result result = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasmStart or i_module->wasmEnd - i_module->wasmStart < 8);

    {
        bytes_t  pos = i_module->wasmStart + 8;
        cbytes_t end = i_module->wasmEnd;

        Xxh64_Init(&hash, 0);

        ಯಾವತ್ (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     size      = 0;

_           (ReadLEB_u32(&size, &pos, end));
            _throwif(m3Err_wasmUnderrun, size > (size_t)(end - pos));

            pos += size;
            ಯದಿ (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ಸ್ಥಿರ
ಧ್ರುವ M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    ಧ್ರುವ M3SnapshotMap* map = i_function->snapshotMap;

    ಯದಿ (map) {
        ಪ್ರತಿ (u32 i = 0; i < map->ಅಂಕೆ_Safe_Points; ++i) {
            ಯದಿ (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                ಪ್ರತ್ಯಾಗಮ &map->safePoints[i];
            }
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
ಸ್ಥಿರ
ಧ್ರುವ M3SafePoint* FindSafePointAt (ಧ್ರುವ M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 ಕೆಳಗಿನ  = 0;
    u32 ಮೇಲಿನ = i_map->ಅಂಕೆ_Safe_Points;

    ಯಾವತ್ (ಕೆಳಗಿನ < ಮೇಲಿನ) {
        u32 ಮಧ್ಯ = ಕೆಳಗಿನ + (ಮೇಲಿನ - ಕೆಳಗಿನ) / 2;

        ಯದಿ (i_map->safePoints[ಮಧ್ಯ].wasmOffset < i_wasmOffset) {
            ಕೆಳಗಿನ = ಮಧ್ಯ + 1;
        } ಅನ್ಯಥಾ {
            ಮೇಲಿನ = ಮಧ್ಯ;
        }
    }

    ಪ್ರತಿ (u32 i = ಕೆಳಗಿನ; i < i_map->ಅಂಕೆ_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        ಧ್ರುವ M3SafePoint* point = &i_map->safePoints[i];

        ಯದಿ (point->kind != i_kind or (point->flags & d_m3SafePointTailCall)) {
            ಅನುವರ್ತ;
        }
        ಯದಿ (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            ಪ್ರತ್ಯಾಗಮ point;
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

ಸ್ಥಿರ
ಧ್ರುವ M3BlockStart* FindBlockAt (ಧ್ರುವ M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    ಪ್ರತಿ (u32 i = 0; i < i_map->ಅಂಕೆ_Blocks; ++i) {
        ಯದಿ (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            ಪ್ರತ್ಯಾಗಮ &i_map->blocks[i];
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
bool EnclosesOffset (ಧ್ರುವ M3BlockStart* i_block, u32 i_wasmOffset)
{
    ಪ್ರತ್ಯಾಗಮ i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
bool HandlersLiveAt (ಧ್ರುವ M3SnapshotMap* i_map, ಧ್ರುವ M3SafePoint* i_point, ಧ್ರುವ M3BlockStart* i_try)
{
    ಪ್ರತ್ಯಾಗಮ not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ಸ್ಥಿರ
M3Result CallSiteType (IM3Function i_caller, ಧ್ರುವ M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3Result   result = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        size   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ರಿಕ್ತ;

_   (ReadLEB_u32(&size, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    ಯದಿ (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->ಅಂಕೆ_Functions);
        *o_type = module->functions[index].funcType;
    } ಅನ್ಯಥಾ {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->ಅಂಕೆ_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ಸ್ಥಿರ
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 ಅಂಕೆ_Results = Get_Func_Type_ಅಂಕೆ_Results(i_expected);

    ಯದಿ (not i_type or Get_Func_Type_ಅಂಕೆ_Results(i_type) != ಅಂಕೆ_Results) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಪ್ರತಿ (u16 i = 0; i < ಅಂಕೆ_Results; ++i) {
        ಯದಿ (not IsSubTypeOf(GetFuncTypeResultType(i_type, i), GetFuncTypeResultType(i_expected, i))) {
            ಪ್ರತ್ಯಾಗಮ false;
        }
    }

    ಪ್ರತ್ಯಾಗಮ true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3FrameRegisters {
    m3reg_t* r0;
#  ಯದಿ d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
ಸ್ಥಿರ
M3Result Continuation_ರಾಶಿ (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_ಅಂಕೆ_Slots)
{
    ಯದಿ (i_cont == i_runtime->ಬೇರು_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_ರಾಶಿ;
        *o_ಅಂಕೆ_Slots = i_runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
    } ಅನ್ಯಥಾ {
        *o_base     = i_cont->val_ರಾಶಿ;
        *o_ಅಂಕೆ_Slots = i_cont->ಅಂಕೆ_ರಾಶಿ_Slots;
    }

    ಪ್ರತ್ಯಾಗಮ (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ಸ್ಥಿರ
M3Result Locate_ಮೌಲ್ಯ (m3slot_t* i_base, u32 i_ಅಂಕೆ_Slots, m3stack_t i_sp, ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* i_ಮೌಲ್ಯ, u8** o_where)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_ಮೌಲ್ಯ->type);

    *o_where = ರಿಕ್ತ;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_ಮೌಲ್ಯ->slot));

    ಯದಿ (not IsRegisterSlotAlias(i_ಮೌಲ್ಯ->slot)) {
        size_t size   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? ಪರಿಮಾಣ(ಶೂನ್ಯ*)
                                                                   : (Is64BitType(type) ? ಪರಿಮಾಣ(u64) : ಪರಿಮಾಣ(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_ಮೌಲ್ಯ->slot) * ಪರಿಮಾಣ(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + size > (size_t)i_ಅಂಕೆ_Slots * ಪರಿಮಾಣ(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ಸ್ಥಿರ
M3Result Read_ಮೌಲ್ಯ (u8* i_where, ಧ್ರುವ M3FrameRegisters* i_registers, ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* i_ಮೌಲ್ಯ, u64* o_bits,
                    ಶೂನ್ಯ** o_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_ಮೌಲ್ಯ->type);

    *o_bits      = 0;
    *o_reference = ರಿಕ್ತ;

    ಯದಿ (i_where) {
        ಯದಿ (IsRefType(type)) {
            memcpy(o_reference, i_where, ಪರಿಮಾಣ(ಶೂನ್ಯ*));
        } ಅನ್ಯಥಾ ಯದಿ (Is64BitType(type)) {
            memcpy(o_bits, i_where, ಪರಿಮಾಣ(u64));
        } ಅನ್ಯಥಾ {
            u32 narrow;
            memcpy(&narrow, i_where, ಪರಿಮಾಣ(narrow));
            *o_bits = narrow;
        }
    } ಅನ್ಯಥಾ ಯದಿ (IsFpRegisterSlotAlias(i_ಮೌಲ್ಯ->slot)) {
#  ಯದಿ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ಯದಿ (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, ಪರಿಮಾಣ(bits));
            *o_bits = bits;
        } ಅನ್ಯಥಾ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, ಪರಿಮಾಣ(u64));
        }
#  ಅನ್ಯಥಾ
        _throw(m3Err_wasmMalformed);
#  endif
    } ಅನ್ಯಥಾ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        ಯದಿ (IsRefType(type)) {
            *o_reference = (ಶೂನ್ಯ*)(uintptr_t)r0;
        } ಅನ್ಯಥಾ ಯದಿ (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } ಅನ್ಯಥಾ {
            *o_bits = (u32)r0;
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result Write_ಮೌಲ್ಯ (u8* o_where, ಧ್ರುವ M3FrameRegisters* i_registers, ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* i_ಮೌಲ್ಯ, u64 i_bits,
                     ಶೂನ್ಯ* i_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_ಮೌಲ್ಯ->type);

    ಯದಿ (o_where) {
        ಯದಿ (IsRefType(type)) {
            memcpy(o_where, &i_reference, ಪರಿಮಾಣ(ಶೂನ್ಯ*));
        } ಅನ್ಯಥಾ ಯದಿ (Is64BitType(type)) {
            memcpy(o_where, &i_bits, ಪರಿಮಾಣ(u64));
        } ಅನ್ಯಥಾ {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, ಪರಿಮಾಣ(narrow));
        }
    } ಅನ್ಯಥಾ ಯದಿ (IsFpRegisterSlotAlias(i_ಮೌಲ್ಯ->slot)) {
#  ಯದಿ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ಯದಿ (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, ಪರಿಮಾಣ(narrow));
            *i_registers->fp0 = narrow;
        } ಅನ್ಯಥಾ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, ಪರಿಮಾಣ(u64));
        }
#  ಅನ್ಯಥಾ
        _throw(m3Err_wasmMalformed);
#  endif
    } ಅನ್ಯಥಾ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        ಯದಿ (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } ಅನ್ಯಥಾ {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ಸ್ಥಿರ
ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* Frame_ಮೌಲ್ಯ (ಧ್ರುವ M3SnapshotMap* i_map, ಧ್ರುವ M3SafePoint* i_point, u32 i_index)
{
    ಯದಿ (i_index < i_map->ಅಂಕೆ_Locals) {
        ಪ್ರತ್ಯಾಗಮ i_point->first_Local_ಮೌಲ್ಯ == UINT32_MAX ? &i_map->locals[i_index]
                                                      : &i_map->ಮೌಲ್ಯಗಳು[i_point->first_Local_ಮೌಲ್ಯ + i_index];
    }
    ಪ್ರತ್ಯಾಗಮ &i_map->ಮೌಲ್ಯಗಳು[i_point->first_ಮೌಲ್ಯ + i_index - i_map->ಅಂಕೆ_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ಸ್ಥಿರ
ಶೂನ್ಯ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    ಯದಿ (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->ಅಂಕೆ_Ret_And_Arg_Slots) + i_function->ಅಂಕೆ_Local_Bytes;

        memcpy(where, i_function->constants, i_function->ಅಂಕೆ_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3PointerIds {
    ಶೂನ್ಯ** ವಸ್ತುಗಳು;
    u32    ಎಣಿಕೆ;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  indexCapacity;       // a power of two, kept at least twice count
} M3PointerIds;

ಸ್ಥಿರ
u32 PointerBucket (ಶೂನ್ಯ* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    ಪ್ರತ್ಯಾಗಮ (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

ಸ್ಥಿರ
u32 PointerIds_Find (ಧ್ರುವ M3PointerIds* i_ids, ಶೂನ್ಯ* i_pointer)
{
    ಯದಿ (i_ids->indexCapacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->indexCapacity);

        ಯಾವತ್ (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            ಯದಿ (i_ids->ವಸ್ತುಗಳು[id] == i_pointer) {
                ಪ್ರತ್ಯಾಗಮ id;
            }

            bucket = (bucket + 1) & (i_ids->indexCapacity - 1);
        }
    }

    ಪ್ರತ್ಯಾಗಮ d_m3SnapshotNone;
}

ಸ್ಥಿರ
M3Result PointerIds_Add (M3PointerIds* io_ids, ಶೂನ್ಯ* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    ಯದಿ (id != d_m3SnapshotNone) {
        *o_id = id;
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }

    ಯದಿ (io_ids->ಎಣಿಕೆ == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        ಶೂನ್ಯ** ವಸ್ತುಗಳು    = m3_ReallocArray(ಶೂನ್ಯ*, io_ids->ವಸ್ತುಗಳು, capacity, io_ids->capacity);

        ಯದಿ (not ವಸ್ತುಗಳು) {
            ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
        }

        io_ids->ವಸ್ತುಗಳು    = ವಸ್ತುಗಳು;
        io_ids->capacity = capacity;
    }

    ಯದಿ ((io_ids->ಎಣಿಕೆ + 1) * 2 > io_ids->indexCapacity) {
        u32  capacity = io_ids->indexCapacity ? io_ids->indexCapacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        ಯದಿ (not index) {
            ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
        }

        ಪ್ರತಿ (u32 i = 0; i < io_ids->ಎಣಿಕೆ; ++i) {
            u32 bucket = PointerBucket(io_ids->ವಸ್ತುಗಳು[i], capacity);

            ಯಾವತ್ (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->indexCapacity = capacity;
    }

    id                = io_ids->ಎಣಿಕೆ++;
    io_ids->ವಸ್ತುಗಳು[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->indexCapacity);

    ಯಾವತ್ (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->indexCapacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಸ್ಥಿರ
ಶೂನ್ಯ PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->ವಸ್ತುಗಳು);
    m3_Free(io_ids->index);
    memset(io_ids, 0, ಪರಿಮಾಣ(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3BufferWriter {
    u8*    buffer;
    size_t size;
    size_t capacity;
} M3BufferWriter;

ಸ್ಥಿರ
M3Result BufferWriter_Write (ಧ್ರುವ ಶೂನ್ಯ* i_data, size_t i_size, ಶೂನ್ಯ* i_userdata)
{
    M3BufferWriter* bw = (M3BufferWriter*)i_userdata;

    ಯದಿ (bw->size + i_size > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        ಯಾವತ್ (bw->size + i_size > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->buffer, newCap, bw->capacity);
        ಯದಿ (!newBuf) {
            ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
        }
        bw->buffer   = newBuf;
        bw->capacity = newCap;
    }

    ಯದಿ (i_size) {
        memcpy(bw->buffer + bw->size, i_data, i_size);
        bw->size += i_size;
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3BufferReader {
    ಧ್ರುವ u8* buffer;
    size_t    size;
    size_t    cursor;
} M3BufferReader;

ಸ್ಥಿರ
M3Result BufferReader_Read (ಶೂನ್ಯ* o_buffer, size_t i_size, ಶೂನ್ಯ* i_userdata)
{
    M3BufferReader* br = (M3BufferReader*)i_userdata;

    ಯದಿ (i_size > br->size - br->cursor) {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmMalformed;
    }

    ಯದಿ (i_size) {
        memcpy(o_buffer, br->buffer + br->cursor, i_size);
        br->cursor += i_size;
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
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
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    ಶೂನ್ಯ*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
u8 M3TypeToValType (m3type_t i_type)
{
    ವಿಕಲ್ಪ (BaseTypeOf(i_type)) {
    ಪಕ್ಷ c_m3Type_i32: ಪ್ರತ್ಯಾಗಮ 0x7F;
    ಪಕ್ಷ c_m3Type_i64: ಪ್ರತ್ಯಾಗಮ 0x7E;
    ಪಕ್ಷ c_m3Type_f32: ಪ್ರತ್ಯಾಗಮ 0x7D;
    ಪಕ್ಷ c_m3Type_f64: ಪ್ರತ್ಯಾಗಮ 0x7C;
    ಪಕ್ಷ c_m3Type_v128: ಪ್ರತ್ಯಾಗಮ 0x7B;
    ಪಕ್ಷ c_m3Type_funcref: ಪ್ರತ್ಯಾಗಮ 0x70;
    ಪಕ್ಷ c_m3Type_externref: ಪ್ರತ್ಯಾಗಮ 0x6F;
    ಪಕ್ಷ c_m3Type_exnref: ಪ್ರತ್ಯಾಗಮ 0x69;
    ಪಕ್ಷ c_m3Type_contref: ಪ್ರತ್ಯಾಗಮ 0x68;
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ 0x00;
    }
}

ಸ್ಥಿರ ಅನ್ತಃಸ್ಥ
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    ವಿಕಲ್ಪ (i_valtype) {
    ಪಕ್ಷ 0x7F: ಪ್ರತ್ಯಾಗಮ c_m3Type_i32;
    ಪಕ್ಷ 0x7E: ಪ್ರತ್ಯಾಗಮ c_m3Type_i64;
    ಪಕ್ಷ 0x7D: ಪ್ರತ್ಯಾಗಮ c_m3Type_f32;
    ಪಕ್ಷ 0x7C: ಪ್ರತ್ಯಾಗಮ c_m3Type_f64;
    ಪಕ್ಷ 0x7B: ಪ್ರತ್ಯಾಗಮ c_m3Type_v128;
    ಪಕ್ಷ 0x70: ಪ್ರತ್ಯಾಗಮ c_m3Type_funcref;
    ಪಕ್ಷ 0x6F: ಪ್ರತ್ಯಾಗಮ c_m3Type_externref;
    ಪಕ್ಷ 0x69: ಪ್ರತ್ಯಾಗಮ c_m3Type_exnref;
    ಪಕ್ಷ 0x68: ಪ್ರತ್ಯಾಗಮ c_m3Type_contref;
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ c_m3Type_none;
    }
}

ಸ್ಥಿರ
M3Result Put (M3SnapshotSave* s, ಧ್ರುವ ಶೂನ್ಯ* i_data, size_t i_size)
{
    ಪ್ರತ್ಯಾಗಮ s->writing ? s->writer(i_data, i_size, s->userdata) : m3Err_none;
}

ಸ್ಥಿರ
M3Result PutU8 (M3SnapshotSave* s, u8 i_ಮೌಲ್ಯ)
{
    ಪ್ರತ್ಯಾಗಮ Put(s, &i_ಮೌಲ್ಯ, ಪರಿಮಾಣ(i_ಮೌಲ್ಯ));
}


ಸ್ಥಿರ
M3Result PutLEB_u32 (M3SnapshotSave* s, u32 i_ಮೌಲ್ಯ)
{
    u8  bytes[5];
    u32 len = 0;

    ಕುರು {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಯದಿ (i_ಮೌಲ್ಯ != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } ಯಾವತ್ (i_ಮೌಲ್ಯ != 0);

    ಪ್ರತ್ಯಾಗಮ Put(s, bytes, len);
}

ಸ್ಥಿರ
M3Result PutLEB_u64 (M3SnapshotSave* s, u64 i_ಮೌಲ್ಯ)
{
    u8  bytes[10];
    u32 len = 0;

    ಕುರು {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಯದಿ (i_ಮೌಲ್ಯ != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } ಯಾವತ್ (i_ಮೌಲ್ಯ != 0);

    ಪ್ರತ್ಯಾಗಮ Put(s, bytes, len);
}

ಸ್ಥಿರ
M3Result PutLEB_i64 (M3SnapshotSave* s, i64 i_ಮೌಲ್ಯ)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    ಯಾವತ್ (more) {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಯದಿ ((i_ಮೌಲ್ಯ == 0 && !(byte & 0x40)) || (i_ಮೌಲ್ಯ == -1 && (byte & 0x40))) {
            more = false;
        } ಅನ್ಯಥಾ {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    ಪ್ರತ್ಯಾಗಮ Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
ಪ್ರಕಾರನಿರ್ದೇಶ M3Result (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ಸ್ಥಿರ
M3Result SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3Result         result   = m3Err_none;
    M3BufferWriter   bw       = { ರಿಕ್ತ, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    ಶೂನ್ಯ*            userdata = s->userdata;

    s->writer   = BufferWriter_Write;
    s->userdata = &bw;

    result = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    ಯದಿ (result) {
        ಗಚ್ಛ _catch;
    }

    // a section with nothing in it is left out
    ಯದಿ (bw.size) {
        _throwif("snapshot section is too large", bw.size > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.size));
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
u32 FunctionIndex (IM3Module i_module, IM3Function i_function)
{
    ಯದಿ (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->ಅಂಕೆ_Functions) {
        ಪ್ರತ್ಯಾಗಮ (u32)(i_function - i_module->functions);
    }

    ಪ್ರತ್ಯಾಗಮ d_m3SnapshotNone;
}

ಸ್ಥಿರ
u32 FuncTypeIndex (IM3Module i_module, IM3FuncType i_type)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Func_Types; ++i) {
        ಯದಿ (i_module->funcTypes[i] == i_type) {
            ಪ್ರತ್ಯಾಗಮ i;
        }
    }

    ಪ್ರತ್ಯಾಗಮ d_m3SnapshotNone;
}

#  ಯದಿ d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
ಸ್ಥಿರ
IM3Tag TagOfIndex (IM3Module i_module, u32 i_index)
{
    IM3Tag tag = &i_module->tags[i_index];

    ಪ್ರತ್ಯಾಗಮ tag->resolved ? tag->resolved : tag;
}

ಸ್ಥಿರ
u32 TagIndex (IM3Module i_module, IM3Tag i_tag)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Tags; ++i) {
        ಯದಿ (TagOfIndex(i_module, i) == i_tag) {
            ಪ್ರತ್ಯಾಗಮ i;
        }
    }

    ಪ್ರತ್ಯಾಗಮ d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ಸ್ಥಿರ
M3Result NameReference (M3SnapshotSave* s, u8 i_type, ಶೂನ್ಯ* i_reference, u64* o_word)
{
    M3Result result = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    ಯದಿ (not i_reference) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }

    ಯದಿ (i_type == c_m3Type_funcref) {
        id = FunctionIndex(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_externref) {
        ಧ್ರುವ M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        ಗಚ್ಛ _catch;
    }
#  ಯದಿ d_m3HasExceptionHandling
    ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 ಎಣಿಕೆ = s->exceptions.ಎಣಿಕೆ;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.ಎಣಿಕೆ != ಎಣಿಕೆ);
    }
#  endif
    ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 ಎಣಿಕೆ = s->continuations.ಎಣಿಕೆ;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.ಎಣಿಕೆ != ಎಣಿಕೆ);
    } ಅನ್ಯಥಾ {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ಸ್ಥಿರ
M3Result Put_Ref_ಮೌಲ್ಯ (M3SnapshotSave* s, u8 i_type, ಶೂನ್ಯ* i_reference)
{
    M3Result result = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    ಯದಿ (not s->writing) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }

    ಯದಿ (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } ಅನ್ಯಥಾ {
        u8 kind = 0;
        ಯದಿ (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  ಯದಿ d_m3HasExceptionHandling
        ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ಸ್ಥಿರ
M3Result PutLE32 (M3SnapshotSave* s, u32 i_ಮೌಲ್ಯ)
{
    u8 bytes[4];

    ಪ್ರತಿ (u32 i = 0; i < ಪರಿಮಾಣ(bytes); ++i) {
        bytes[i] = (u8)(i_ಮೌಲ್ಯ >> (8 * i));
    }

    ಪ್ರತ್ಯಾಗಮ Put(s, bytes, ಪರಿಮಾಣ(bytes));
}

ಸ್ಥಿರ
M3Result PutLE64 (M3SnapshotSave* s, u64 i_ಮೌಲ್ಯ)
{
    u8 bytes[8];

    ಪ್ರತಿ (u32 i = 0; i < ಪರಿಮಾಣ(bytes); ++i) {
        bytes[i] = (u8)(i_ಮೌಲ್ಯ >> (8 * i));
    }

    ಪ್ರತ್ಯಾಗಮ Put(s, bytes, ಪರಿಮಾಣ(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ಸ್ಥಿರ
M3Result Put_ಮೌಲ್ಯ_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    ಯದಿ (IsRefType(i_type)) {
        ಪ್ರತ್ಯಾಗಮ Put_Ref_ಮೌಲ್ಯ(s, BaseTypeOf(i_type), (ಶೂನ್ಯ*)(uintptr_t)i_bits);
    }

    ವಿಕಲ್ಪ (BaseTypeOf(i_type)) {
    ಪಕ್ಷ c_m3Type_i32: ಪ್ರತ್ಯಾಗಮ PutLEB_i64(s, (i32)(u32)i_bits);
    ಪಕ್ಷ c_m3Type_i64: ಪ್ರತ್ಯಾಗಮ PutLEB_i64(s, (i64)i_bits);
    ಪಕ್ಷ c_m3Type_f32: ಪ್ರತ್ಯಾಗಮ PutLE32(s, (u32)i_bits);
    ಪಕ್ಷ c_m3Type_f64: ಪ್ರತ್ಯಾಗಮ PutLE64(s, i_bits);
    ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "a snapshot cannot hold a value of this type as bits";
    }
}

ಸ್ಥಿರ
M3Result Put_ಮೌಲ್ಯ_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    ಯದಿ (not valtype) {
        ಪ್ರತ್ಯಾಗಮ "a snapshot has no tag for a value of this type";
    }

    ಪ್ರತ್ಯಾಗಮ PutU8(s, valtype);
}

ಸ್ಥಿರ
M3Result Put_ಮೌಲ್ಯ_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3Result result = m3Err_none;

_   (Put_ಮೌಲ್ಯ_Tag(s, i_type));
_   (Put_ಮೌಲ್ಯ_Body(s, i_type, i_bits));

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ಸ್ಥಿರ
M3Result Put_ಮೌಲ್ಯ (M3SnapshotSave* s, m3type_t i_type, ಧ್ರುವ ಶೂನ್ಯ* i_where)
{
    M3Result result = m3Err_none;

_   (Put_ಮೌಲ್ಯ_Tag(s, i_type));

    ಯದಿ (IsRefType(i_type)) {
        ಶೂನ್ಯ* reference;
        memcpy(&reference, i_where, ಪರಿಮಾಣ(reference));
_       (Put_ಮೌಲ್ಯ_Body(s, i_type, (u64)(uintptr_t)reference));
    } ಅನ್ಯಥಾ ಯದಿ (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, ಪರಿಮಾಣ(bytes));
_       (Put(s, bytes, ಪರಿಮಾಣ(bytes)));
    } ಅನ್ಯಥಾ ಯದಿ (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, ಪರಿಮಾಣ(bits));
_       (Put_ಮೌಲ್ಯ_Body(s, i_type, bits));
    } ಅನ್ಯಥಾ {
        u32 narrow;
        memcpy(&narrow, i_where, ಪರಿಮಾಣ(narrow));
_       (Put_ಮೌಲ್ಯ_Body(s, i_type, narrow));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result PutMemoryChunks (M3SnapshotSave* s, ಧ್ರುವ u8* i_bytes, size_t i_size)
{
    M3Result result = m3Err_none;
    size_t   cursor = 0;

    ಯಾವತ್ (cursor < i_size) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        ಯಾವತ್ (cursor + run < i_size and i_bytes[cursor + run] == byte) {
            run++;
        }

        ಯದಿ ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            ಯದಿ (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            ಅನುವರ್ತ;
        }

        size_t rawStart = cursor;

        ಯಾವತ್ (cursor < i_size) {
            u8     b      = i_bytes[cursor];
            size_t length = 1;

            ಯಾವತ್ (cursor + length < i_size and i_bytes[cursor + length] == b) {
                length++;
            }

            ಯದಿ ((b == 0x00 or b == 0xFF) and length >= d_m3SnapshotRunThreshold) {
                ಭಙ್ಗ;
            }

            cursor += length;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)rawStart));
_       (PutLEB_u32(s, (u32)(cursor - rawStart)));
_       (Put(s, i_bytes + rawStart, cursor - rawStart));
    }

_   (PutU8(s, d_m3ChunkEnd));

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ಸ್ಥಿರ
u32 Memory_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    ಯಾವತ್ (module->memories[j] != module->memories[index]) {
        ++j;
    }
    ಪ್ರತ್ಯಾಗಮ j;
}

ಸ್ಥಿರ
u32 Table_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    ಯಾವತ್ (module->tables[j] != module->tables[index]) {
        ++j;
    }
    ಪ್ರತ್ಯಾಗಮ j;
}

ಸ್ಥಿರ
M3Result SaveMemories (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_Memories));

    ಪ್ರತಿ (u32 m = 0; m < module->ಅಂಕೆ_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_FirstIndex(module, m);
        bool      hasData  = memory and memory->mallocated;
        u32       pageSize = memory ? Memory_PageSize(memory) : d_m3DefaultMemPageSize;
        u64       bytes    = memory ? memory->ಅಂಕೆ_Pages * (u64)pageSize : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        ಯದಿ (first != m) {
            ಅನುವರ್ತ;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", memory and memory->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", bytes > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 memory and memory->mallocated and memory->mallocated->length < bytes);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        ಯಾವತ್ ((1u << pageBits) < pageSize) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->ಅಂಕೆ_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        ಯದಿ (not s->writing) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (hasData) {
_           (PutMemoryChunks(s, m3MemData(memory->mallocated), (size_t)bytes));
        } ಅನ್ಯಥಾ {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result SaveGlobals (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_Globals));

    ಪ್ರತಿ (u32 g = 0; g < module->ಅಂಕೆ_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_ಮೌಲ್ಯ(s, global->type, &cell->i64_ಮೌಲ್ಯ));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result SaveTables (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_Tables));

    ಪ್ರತಿ (u32 t = 0; t < module->ಅಂಕೆ_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_FirstIndex(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        ಯದಿ (first != t) {
            ಅನುವರ್ತ;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->size));

        ಪ್ರತಿ (u32 e = 0; e < table->size; ++e) {
_           (Put_Ref_ಮೌಲ್ಯ(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result SaveSegments (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ಅಂಕೆ_Data_Segments));

    ಪ್ರತಿ (u32 i = 0; i < module->ಅಂಕೆ_Data_Segments; ++i) {
_       (PutU8(s, module->dataSegments[i].dropped));
    }

_   (PutLEB_u32(s, module->ಅಂಕೆ_Element_Segments));

    ಪ್ರತಿ (u32 i = 0; i < module->ಅಂಕೆ_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

#  ಯದಿ d_m3HasExceptionHandling

ಸ್ಥಿರ
M3Result SaveExceptions (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.ಎಣಿಕೆ));

    ಪ್ರತಿ (u32 i = 0; i < s->exceptions.ಎಣಿಕೆ; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.ವಸ್ತುಗಳು[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->ಅಂಕೆ_Args != Get_Func_Type_ಅಂಕೆ_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->ಅಂಕೆ_Args));

        ಪ್ರತಿ (u32 a = 0; a < exn->ಅಂಕೆ_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_ಮೌಲ್ಯ_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3Activation {
    IM3Function        function;
    ಧ್ರುವ M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    ಧ್ರುವ M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ಸ್ಥಿರ
ಧ್ರುವ M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    ಧ್ರುವ M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : ರಿಕ್ತ;

    ಪ್ರತಿ (u32 i = 0; not point and i < i_module->ಅಂಕೆ_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        ಯದಿ (point) {
            *io_function = &i_module->functions[i];
        }
    }

    ಪ್ರತ್ಯಾಗಮ point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ಸ್ಥಿರ
M3Result NextActivation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3Result result = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, ಪರಿಮಾಣ(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    ಪ್ರತಿ (; i >= 0; --i) {
        ಧ್ರುವ M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        ಯದಿ (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  ಯದಿ d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            ಭಙ್ಗ;
        } ಅನ್ಯಥಾ ಯದಿ (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            ಭಙ್ಗ;
        }
    }

    ಯದಿ (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  ಯದಿ d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  ಯದಿ d_m3EntryKeepsFrame
    ಪ್ರತಿ (i32 k = i_first; k > o_act->lastFrame; --k) {
        ಧ್ರುವ M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ಸ್ಥಿರ
M3Result SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_ಅಂಕೆ_Slots, ಧ್ರುವ M3Activation* i_act)
{
    M3Result             result        = m3Err_none;
    IM3Function          function      = i_act->function;
    ಧ್ರುವ M3SnapshotMap* map           = function->snapshotMap;
    u32                  functionIndex = FunctionIndex(s->module, function);
    ಧ್ರುವ M3SafePoint*   point         = i_act->point;
    u32                  ಅಂಕೆ_ಮೌಲ್ಯಗಳು;

    _throwif("a suspended frame belongs to another module", functionIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, functionIndex));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    ಯದಿ (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    ಅಂಕೆ_ಮೌಲ್ಯಗಳು = map->ಅಂಕೆ_Locals + point->ಅಂಕೆ_ಮೌಲ್ಯಗಳು;

_   (PutLEB_u32(s, ಅಂಕೆ_ಮೌಲ್ಯಗಳು));

    ಪ್ರತಿ (u32 v = 0; v < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++v) {
        ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* ಮೌಲ್ಯ = Frame_ಮೌಲ್ಯ(map, point, v);
        u8*                where;
        u64                word;
        ಶೂನ್ಯ*              reference;

_       (Locate_ಮೌಲ್ಯ(i_base, i_ಅಂಕೆ_Slots, i_act->sp, ಮೌಲ್ಯ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ಯದಿ (BaseTypeOf(ಮೌಲ್ಯ->type) == c_m3Type_v128) {
_           (Put_ಮೌಲ್ಯ_Tag(s, ಮೌಲ್ಯ->type));
_           (Put(s, where, 16));
            ಅನುವರ್ತ;
        }

_       (Read_ಮೌಲ್ಯ(where, &i_act->registers, ಮೌಲ್ಯ, &word, &reference));

_       (Put_ಮೌಲ್ಯ_Bits(s, ಮೌಲ್ಯ->type, IsRefType(ಮೌಲ್ಯ->type) ? (u64)(uintptr_t)reference : word));
    }

    ಯದಿ (point->kind == safepoint_resume) {
_       (Put_Ref_ಮೌಲ್ಯ(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
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
M3Result SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3Result      result   = m3Err_none;
    M3Activation* acts     = ರಿಕ್ತ;
    u32           ಅಂಕೆ_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->ಅಂಕೆ_Frames - 1;
    m3slot_t*     base;
    u32           ಅಂಕೆ_Slots;
    m3stack_t     sp;

_   (Continuation_ರಾಶಿ(s->runtime, i_cont, &base, &ಅಂಕೆ_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->ಅಂಕೆ_Frames + 1);
    _throwifnull(acts);

    sp = base;

    ಯಾವತ್ (true) {
        M3Activation* act = &acts[ಅಂಕೆ_Acts];

_       (NextActivation(s->module, i_cont, function, sp, first, act));

        ಯದಿ (act->kind != safepoint_call) {
            ಅಂಕೆ_Acts++;
            ಭಙ್ಗ;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        ಯದಿ (not (act->point->flags & d_m3SafePointTailCall)) {
            ಅಂಕೆ_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, ಅಂಕೆ_Acts));

    ಪ್ರತಿ (u32 a = 0; a < ಅಂಕೆ_Acts; ++a) {
_       (SaveActivation(s, base, ಅಂಕೆ_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3Result  result    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_ಬೇರು    = (i_cont == s->runtime->ಬೇರು_Continuation);
    u8        state     = snapshot_contFinished;
    u32       typeIndex = is_ಬೇರು ? d_m3SnapshotNone : FuncTypeIndex(module, i_cont->type);

    ಯದಿ (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } ಅನ್ಯಥಾ ಯದಿ (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } ಅನ್ಯಥಾ ಯದಿ (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", FunctionIndex(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_ಬೇರು and typeIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_ಬೇರು));
_   (PutLEB_u32(s, typeIndex));
_   (PutLEB_u32(s, FunctionIndex(module, i_cont->entryFunction)));
#  ಯದಿ d_m3HasExceptionHandling
_   (Put_Ref_ಮೌಲ್ಯ(s, c_m3Type_exnref, i_cont->resumeThrow));
#  ಅನ್ಯಥಾ
_   (Put_Ref_ಮೌಲ್ಯ(s, c_m3Type_exnref, ರಿಕ್ತ));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    ಯದಿ (s->postmortem) {
        ಗಚ್ಛ _catch;
    }

    ಯದಿ (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : ರಿಕ್ತ;
        u16         ಅಂಕೆ_Rets = inner ? Get_Func_Type_ಅಂಕೆ_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_ಎಣಿಕೆ > Get_Func_Type_ಅಂಕೆ_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_ಎಣಿಕೆ));

        ಪ್ರತಿ (u32 i = 0; i < i_cont->bound_Args_ಎಣಿಕೆ; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_ಮೌಲ್ಯ(s, type, i_cont->val_ರಾಶಿ + (ಅಂಕೆ_Rets + i) * c_io_Slot_ಎಣಿಕೆ));
        }
    } ಅನ್ಯಥಾ ಯದಿ (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_ಎಣಿಕೆ));
_       (SaveSuspendedBody(s, i_cont));
    } ಅನ್ಯಥಾ {
_       (PutLEB_u32(s, 0));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result SaveContinuations (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->continuations.ಎಣಿಕೆ));

    ಪ್ರತಿ (u32 i = 0; i < s->continuations.ಎಣಿಕೆ; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.ವಸ್ತುಗಳು[i]));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ಸ್ಥಿರ
M3Result SaveHostState (M3SnapshotSave* s)
{
    M3Result               result = m3Err_none;
    ಧ್ರುವ M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3BufferWriter         bw     = { ರಿಕ್ತ, 0, 0 };

    ಯದಿ (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, BufferWriter_Write, &bw));
    }

    ಯದಿ (bw.size > 0) {
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
ಶೂನ್ಯ SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    ಪ್ರತಿ (u32 i = 0; i < module->ಅಂಕೆ_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        ಯದಿ (Memory_FirstIndex(module, i) == i and memory) {
            *memoryBytes += memory->ಅಂಕೆ_Pages * (u64)Memory_PageSize(memory);
        }
    }
    ಪ್ರತಿ (u32 i = 0; i < module->ಅಂಕೆ_Tables; ++i) {
        IM3Table table = module->tables[i];
        ಯದಿ (Table_FirstIndex(module, i) == i and table) {
            *tableElements += table->size;
        }
    }
}

ಸ್ಥಿರ
M3Result SaveMeta (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    ಪ್ರತಿ (u32 i = 0; i < s->continuations.ಎಣಿಕೆ; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.ವಸ್ತುಗಳು[i];
        ಯದಿ (cont != s->runtime->ಬೇರು_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.ಎಣಿಕೆ));
_   (PutLEB_u32(s, s->exceptions.ಎಣಿಕೆ));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  ಯದಿ d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    ಪ್ರತಿ (u32 i = 0; i < s->exceptions.ಎಣಿಕೆ; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.ವಸ್ತುಗಳು[i];
        u32          tagIndex  = TagIndex(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tagIndex == d_m3SnapshotNone);

_       (PutLEB_u32(s, tagIndex));
    }
#  endif

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ಸ್ಥಿರ
M3Result DiscoverReferences (M3SnapshotSave* s)
{
    M3Result result            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    ಯಾವತ್ (doneContinuations < s->continuations.ಎಣಿಕೆ or doneExceptions < s->exceptions.ಎಣಿಕೆ) {
#  ಯದಿ d_m3HasExceptionHandling
        ಯಾವತ್ (doneExceptions < s->exceptions.ಎಣಿಕೆ) {
            M3Exception* exception = (M3Exception*)s->exceptions.ವಸ್ತುಗಳು[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->ಅಂಕೆ_Args != Get_Func_Type_ಅಂಕೆ_Params(type));

            ಪ್ರತಿ (u32 a = 0; a < exception->ಅಂಕೆ_Args; ++a) {
_               (Put_ಮೌಲ್ಯ_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        ಯಾವತ್ (doneContinuations < s->continuations.ಎಣಿಕೆ and doneExceptions == s->exceptions.ಎಣಿಕೆ) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.ವಸ್ತುಗಳು[id]));
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result SaveSections (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

    ಯದಿ (not s->writing) {
        ಪ್ರತ್ಯಾಗಮ DiscoverReferences(s);
    }

    ಯದಿ (not s->embedded) {
_       (Put(s, c_snapshotMagic, ಪರಿಮಾಣ(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    ಯದಿ (s->module->ಅಂಕೆ_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    ಯದಿ (s->module->ಅಂಕೆ_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    ಯದಿ (s->module->ಅಂಕೆ_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    ಯದಿ (s->module->ಅಂಕೆ_Data_Segments or s->module->ಅಂಕೆ_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  ಯದಿ d_m3HasExceptionHandling
    ಯದಿ (s->exceptions.ಎಣಿಕೆ) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    ಯದಿ (s->continuations.ಎಣಿಕೆ) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3SnapshotSuspendSite {
    ಧ್ರುವ M3SnapshotMap* map;
    ಧ್ರುವ M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    ಶೂನ್ಯ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ರಾಶಿ;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    ಅಂಕೆ_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  ಯದಿ d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 ಅಂಕೆ_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

ಸ್ಥಿರ
M3Result Get (M3SnapshotLoad* l, ಶೂನ್ಯ* o_data, size_t i_size)
{
    ಯದಿ (not l->inSection) {
        ಪ್ರತ್ಯಾಗಮ l->reader(o_data, i_size, l->userdata);
    }
    ಯದಿ (i_size > (size_t)(l->end - l->pos)) {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
    ಯದಿ (i_size) {
        memcpy(o_data, l->pos, i_size);
        l->pos += i_size;
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

ಸ್ಥಿರ
M3Result GetU8 (M3SnapshotLoad* l, u8* o_ಮೌಲ್ಯ)
{
    ಪ್ರತ್ಯಾಗಮ Get(l, o_ಮೌಲ್ಯ, ಪರಿಮಾಣ(*o_ಮೌಲ್ಯ));
}

ಸ್ಥಿರ
M3Result GetBool (M3SnapshotLoad* l, u8* o_ಮೌಲ್ಯ)
{
    M3Result result = GetU8(l, o_ಮೌಲ್ಯ);
    ಪ್ರತ್ಯಾಗಮ result ? result : (*o_ಮೌಲ್ಯ <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ಸ್ಥಿರ
M3Result GetLEB_u32 (M3SnapshotLoad* l, u32* o_ಮೌಲ್ಯ)
{
    ಯದಿ (l->inSection) {
        ಪ್ರತ್ಯಾಗಮ ReadLEB_u32(o_ಮೌಲ್ಯ, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    ಪ್ರತಿ (u32 i = 0; i < ಪರಿಮಾಣ(encoded); ++i) {
        M3Result result = GetU8(l, &encoded[i]);
        ಯದಿ (result) {
            ಪ್ರತ್ಯಾಗಮ result;
        }
        ಯದಿ (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            ಪ್ರತ್ಯಾಗಮ ReadLEB_u32(o_ಮೌಲ್ಯ, &pos, encoded + i + 1);
        }
    }
    ಪ್ರತ್ಯಾಗಮ m3Err_lebOverflow;
}

ಸ್ಥಿರ
M3Result GetLEB_u64 (M3SnapshotLoad* l, u64* o_ಮೌಲ್ಯ)
{
    ಪ್ರತ್ಯಾಗಮ ReadLebUnsigned(o_ಮೌಲ್ಯ, 64, &l->pos, l->end);
}

ಸ್ಥಿರ
M3Result GetLEB_i64 (M3SnapshotLoad* l, i64* o_ಮೌಲ್ಯ)
{
    ಪ್ರತ್ಯಾಗಮ ReadLEB_i64(o_ಮೌಲ್ಯ, &l->pos, l->end);
}

ಸ್ಥಿರ
M3Result SkipBytes (M3SnapshotLoad* l, size_t i_size)
{
    ಯದಿ (i_size > (size_t)(l->end - l->pos)) {
        ಪ್ರತ್ಯಾಗಮ m3Err_wasmUnderrun;
    }
    l->pos += i_size;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}


ಸ್ಥಿರ
M3Result Get_Ref_ಮೌಲ್ಯ (M3SnapshotLoad* l, m3type_t i_type, ಶೂನ್ಯ** o_reference)
{
    M3Result result = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = ರಿಕ್ತ;

_   (GetU8(l, &kind));
    ಯದಿ (kind == 0x00) {
        ಪ್ರತ್ಯಾಗಮ (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    ಯದಿ (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->ಅಂಕೆ_Functions);
#  ಯದಿ d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } ಅನ್ಯಥಾ ಯದಿ (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        ಧ್ರುವ M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        ಯದಿ (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  ಯದಿ d_m3HasExceptionHandling
    ಅನ್ಯಥಾ ಯದಿ (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->ಅಂಕೆ_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    ಅನ್ಯಥಾ ಯದಿ (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->ಅಂಕೆ_Continuations);
        *o_reference = l->continuations[id];
#  ಯದಿ d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        ಯದಿ (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } ಅನ್ಯಥಾ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result GetLE32 (M3SnapshotLoad* l, u32* o_ಮೌಲ್ಯ)
{
    u8       bytes[4];
    M3Result result = Get(l, bytes, ಪರಿಮಾಣ(bytes));

    *o_ಮೌಲ್ಯ = 0;

    ಪ್ರತಿ (u32 i = 0; i < ಪರಿಮಾಣ(bytes); ++i) {
        *o_ಮೌಲ್ಯ |= (u32)bytes[i] << (8 * i);
    }

    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result GetLE64 (M3SnapshotLoad* l, u64* o_ಮೌಲ್ಯ)
{
    u8       bytes[8];
    M3Result result = Get(l, bytes, ಪರಿಮಾಣ(bytes));

    *o_ಮೌಲ್ಯ = 0;

    ಪ್ರತಿ (u32 i = 0; i < ಪರಿಮಾಣ(bytes); ++i) {
        *o_ಮೌಲ್ಯ |= (u64)bytes[i] << (8 * i);
    }

    ಪ್ರತ್ಯಾಗಮ result;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ಸ್ಥಿರ
M3Result Get_ಮೌಲ್ಯ_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3Result result  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    ಯದಿ (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ಸ್ಥಿರ
M3Result Get_ಮೌಲ್ಯ_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3Result result = m3Err_none;

    *o_bits = 0;

    ಯದಿ (IsRefType(i_type)) {
        ಶೂನ್ಯ* reference = ರಿಕ್ತ;
_       (Get_Ref_ಮೌಲ್ಯ(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } ಅನ್ಯಥಾ ಯದಿ (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 ಮೌಲ್ಯ = 0;
_       (ReadLEB_i32(&ಮೌಲ್ಯ, &l->pos, l->end));
        *o_bits = (u32)ಮೌಲ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 ಮೌಲ್ಯ = 0;
_       (GetLEB_i64(l, &ಮೌಲ್ಯ));
        *o_bits = (u64)ಮೌಲ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } ಅನ್ಯಥಾ ಯದಿ (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } ಅನ್ಯಥಾ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result Get_ಮೌಲ್ಯ_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3Result result = m3Err_none;
    m3type_t type;

_   (Get_ಮೌಲ್ಯ_Tag(l, i_expectedType, &type));
_   (Get_ಮೌಲ್ಯ_Body(l, type, o_bits));

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result Get_ಮೌಲ್ಯ (M3SnapshotLoad* l, m3type_t i_expectedType, ಶೂನ್ಯ* o_where)
{
    M3Result result = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    ಯದಿ (l->checking) {
        o_where = scratch;
    }

_   (Get_ಮೌಲ್ಯ_Tag(l, i_expectedType, &type));

    ಯದಿ (IsRefType(type)) {
        ಶೂನ್ಯ* reference = ರಿಕ್ತ;
        u64   bits      = 0;
_       (Get_ಮೌಲ್ಯ_Body(l, type, &bits));
        reference = (ಶೂನ್ಯ*)(uintptr_t)bits;
        memcpy(o_where, &reference, ಪರಿಮಾಣ(reference));
    } ಅನ್ಯಥಾ ಯದಿ (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } ಅನ್ಯಥಾ {
        u64 bits = 0;
_       (Get_ಮೌಲ್ಯ_Body(l, type, &bits));

        ಯದಿ (Is64BitType(type)) {
            memcpy(o_where, &bits, ಪರಿಮಾಣ(bits));
        } ಅನ್ಯಥಾ {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, ಪರಿಮಾಣ(narrow));
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3Result result = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->ಅಂಕೆ_Functions);
    *o_function = &l->module->functions[index];

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_size)
{
    M3Result result = m3Err_none;

    ಪ್ರತಿ (;;) {
        u8  chunkType = 0;
        u32 offset = 0, length = 0;

_       (GetU8(l, &chunkType));

        ಯದಿ (chunkType == d_m3ChunkEnd) {
            ಭಙ್ಗ;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &length));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_size or length > i_size - (size_t)offset);

        ಯದಿ (chunkType == d_m3ChunkRaw) {
            ಯದಿ (l->checking or length == 0) {
_               (SkipBytes(l, length));
            } ಅನ್ಯಥಾ {
_               (Get(l, o_bytes + offset, length));
            }
        } ಅನ್ಯಥಾ ಯದಿ (chunkType == d_m3ChunkFillFF) {
            ಯದಿ (not l->checking and length) {
                memset(o_bytes + offset, 0xFF, length);
            }
        } ಅನ್ಯಥಾ {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadMemories (M3SnapshotLoad* l)
{
    M3Result  result      = m3Err_none;
    IM3Module module      = l->module;
    u32       ಅಂಕೆ_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Memories == 0 or ಅಂಕೆ_Memories != module->ಅಂಕೆ_Memories);

    ಪ್ರತಿ (u32 m = 0; m < ಅಂಕೆ_Memories; ++m) {
        u32       memIndex = 0, firstIndex = 0, pageBits = 0, ಅಂಕೆ_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &memIndex));
_       (GetLEB_u32(l, &firstIndex));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, memIndex != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 firstIndex != Memory_FirstIndex(module, m));
        ಯದಿ (firstIndex != m) {
            ಅನುವರ್ತ;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &ಅಂಕೆ_Pages));

        memory = module->memories[memIndex];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    data  = ರಿಕ್ತ;

        ಯದಿ (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_PageSize(memory));
            _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Pages < memory->ಅಂಕೆ_Pages or ಅಂಕೆ_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Pages > SIZE_MAX / Memory_PageSize(memory));
            bytes = (size_t)ಅಂಕೆ_Pages * Memory_PageSize(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            ಯದಿ (not l->checking) {
                ಯದಿ (memory->ಅಂಕೆ_Pages < ಅಂಕೆ_Pages) {
_                   (ResizeMemory(l->runtime, memory, ಅಂಕೆ_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->length < bytes);
            }

            l->loadedMemoryBytes += bytes;

            ಯದಿ (bytes and memory->mallocated) {
                data = m3MemData(memory->mallocated);
            }
        } ಅನ್ಯಥಾ {
            _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        ಯದಿ (data and not l->checking) {
            memset(data, 0, bytes);
        }

_       (LoadMemoryChunks(l, data, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadGlobals (M3SnapshotLoad* l)
{
    M3Result  result     = m3Err_none;
    IM3Module module     = l->module;
    u32       ಅಂಕೆ_Globals = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Globals));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Globals == 0 or ಅಂಕೆ_Globals != module->ಅಂಕೆ_Globals);

    ಪ್ರತಿ (u32 g = 0; g < ಅಂಕೆ_Globals; ++g) {
        u32       globalIndex = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &globalIndex));
        _throwif(m3Err_wasmMalformed, globalIndex != g);

        global = &module->globals[globalIndex];
        cell   = global->resolved ? global->resolved : global;

_       (Get_ಮೌಲ್ಯ(l, global->type, &cell->i64_ಮೌಲ್ಯ));
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadTables (M3SnapshotLoad* l)
{
    M3Result  result       = m3Err_none;
    IM3Module module       = l->module;
    u32       ಅಂಕೆ_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Tables));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Tables == 0 or ಅಂಕೆ_Tables != module->ಅಂಕೆ_Tables);

    ಪ್ರತಿ (u32 t = 0; t < ಅಂಕೆ_Tables; ++t) {
        u32      tableIndex = 0, firstIndex = 0, size = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &tableIndex));
_       (GetLEB_u32(l, &firstIndex));

        _throwif(m3Err_wasmMalformed, tableIndex != t);
        _throwif("the snapshot shares tables between imports differently",
                 firstIndex != Table_FirstIndex(module, t));
        ಯದಿ (firstIndex != t) {
            ಅನುವರ್ತ;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &size));

        table = module->tables[tableIndex];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, size > (table->hasMax ? table->maxSize : d_m3MaxSaneTableSize));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, size < table->size);

        l->loadedTableElements += size;

        ಯದಿ (not l->checking and size != table->size) {
            ಶೂನ್ಯ** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 size - table->size > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(ಶೂನ್ಯ*, table->elements, size, table->size);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += size - table->size;
            table->elements = elements;
            table->size     = size;
        }

        ಪ್ರತಿ (u32 e = 0; e < size; ++e) {
            ಶೂನ್ಯ* reference = ರಿಕ್ತ;
_           (Get_Ref_ಮೌಲ್ಯ(l, table->type, &reference));
            ಯದಿ (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadSegments (M3SnapshotLoad* l)
{
    M3Result  result = m3Err_none;
    IM3Module module = l->module;
    u32       ಎಣಿಕೆ  = 0;

_   (GetLEB_u32(l, &ಎಣಿಕೆ));
    _throwif(m3Err_wasmMalformed, ಎಣಿಕೆ != module->ಅಂಕೆ_Data_Segments);
    _throwif(m3Err_wasmMalformed, module->ಅಂಕೆ_Data_Segments == 0 and module->ಅಂಕೆ_Element_Segments == 0);

    ಪ್ರತಿ (u32 i = 0; i < ಎಣಿಕೆ; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ಯದಿ (not l->checking) {
            module->dataSegments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &ಎಣಿಕೆ));
    _throwif(m3Err_wasmMalformed, ಎಣಿಕೆ != module->ಅಂಕೆ_Element_Segments);

    ಪ್ರತಿ (u32 i = 0; i < ಎಣಿಕೆ; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ಯದಿ (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// A frame for the continuation being put together, outermost first
ಸ್ಥಿರ
M3Result AddFrame (M3SnapshotLoad* l, u32* io_ಅಂಕೆ_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    ಯದಿ (*io_ಅಂಕೆ_Frames >= d_m3ContinuationMaxFrames) {
        ಪ್ರತ್ಯಾಗಮ "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_ಅಂಕೆ_Frames)++];

    memset(frame, 0, ಪರಿಮಾಣ(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ಸ್ಥಿರ
M3Result PrepareFunction (IM3Function i_function)
{
    M3Result result = m3Err_none;

    ಯದಿ (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ಸ್ಥಿರ
M3Result AddBlockFrames (M3SnapshotLoad* l, ಧ್ರುವ M3SnapshotMap* i_map, ಧ್ರುವ M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_ಅಂಕೆ_Frames)
{
    M3Result result = m3Err_none;

    ಪ್ರತಿ (u32 b = 0; b < i_map->ಅಂಕೆ_Blocks; ++b) {
        ಧ್ರುವ M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        ಯದಿ (not EnclosesOffset(block, i_point->wasmOffset)) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_ಅಂಕೆ_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  ಯದಿ d_m3HasExceptionHandling
        ಅನ್ಯಥಾ {
_           (AddFrame(l, io_ಅಂಕೆ_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.ಅಂಕೆ_Clauses   = block->ಅಂಕೆ_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// Puts a function's values where this build keeps them at the safepoint
ಸ್ಥಿರ
M3Result Load_ಮೌಲ್ಯಗಳು (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_ಅಂಕೆ_Slots, m3stack_t i_sp,
                     ಧ್ರುವ M3SnapshotMap* i_map, ಧ್ರುವ M3SafePoint* i_point, ಧ್ರುವ M3FrameRegisters* i_registers)
{
    M3Result result    = m3Err_none;
    u32      ಅಂಕೆ_ಮೌಲ್ಯಗಳು = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_ಮೌಲ್ಯಗಳು));
    _throwif("the snapshot's frame does not match what this build compiled", ಅಂಕೆ_ಮೌಲ್ಯಗಳು != i_map->ಅಂಕೆ_Locals + i_point->ಅಂಕೆ_ಮೌಲ್ಯಗಳು);

    ಪ್ರತಿ (u32 v = 0; v < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++v) {
        ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* ಮೌಲ್ಯ     = Frame_ಮೌಲ್ಯ(i_map, i_point, v);
        u64                word      = 0;
        ಶೂನ್ಯ*              reference = ರಿಕ್ತ;
        u8*                where;

_       (Locate_ಮೌಲ್ಯ(i_base, i_ಅಂಕೆ_Slots, i_sp, ಮೌಲ್ಯ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ಯದಿ (BaseTypeOf(ಮೌಲ್ಯ->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_ಮೌಲ್ಯ_Tag(l, ಮೌಲ್ಯ->type, &type));
            ಯದಿ (l->checking) {
_               (SkipBytes(l, 16));
            } ಅನ್ಯಥಾ {
_               (Get(l, where, 16));
            }
            ಅನುವರ್ತ;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= ಅಂಕೆ_ಮೌಲ್ಯಗಳು - i_point->ಅಂಕೆ_Results;
_       (Get_ಮೌಲ್ಯ_Bits(l, ಮೌಲ್ಯ->type, &word));
        l->allowUnfilledRef = false;

        ಯದಿ (IsRefType(ಮೌಲ್ಯ->type)) {
            reference = (ಶೂನ್ಯ*)(uintptr_t)word;
        }

        ಯದಿ (not l->checking) {
_           (Write_ಮೌಲ್ಯ(where, i_registers, ಮೌಲ್ಯ, word, reference));
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ಸ್ಥಿರ
M3Result LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3Result    result    = m3Err_none;
    u32         ಅಂಕೆ_Frames = 0;
    M3Frame*    callFrame = ರಿಕ್ತ;
    IM3Function function  = ರಿಕ್ತ;
    m3slot_t*   base;
    u32         ಅಂಕೆ_Slots;
    u32         ಅಂಕೆ_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_ರಾಶಿ(l->runtime, io_cont, &base, &ಅಂಕೆ_Slots));

_   (GetLEB_u32(l, &ಅಂಕೆ_Activations));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Activations == 0 or ಅಂಕೆ_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = ರಿಕ್ತ;
    io_cont->r0                = 0;
    io_cont->ಅಂಕೆ_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  ಯದಿ d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    ಪ್ರತಿ (u32 a = 0; a < ಅಂಕೆ_Activations; ++a) {
        bool                 isInnermost = (a + 1 == ಅಂಕೆ_Activations);
        ಧ್ರುವ M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        ಧ್ರುವ M3SnapshotMap* map;

        memset(&registers, 0, ಪರಿಮಾಣ(registers));

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
                 (u32)(sp - base) + function->max_ರಾಶಿ_Slots > ಅಂಕೆ_Slots);

        // the call the frame outside this one is waiting on
        ಯದಿ (callFrame) {
            callFrame->call.function = function;
        }

#  ಯದಿ d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &ಅಂಕೆ_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        ಯದಿ (kind == safepoint_op) {
            ಧ್ರುವ M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &ಅಂಕೆ_Frames));

        ಯದಿ (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &ಅಂಕೆ_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  ಯದಿ d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } ಅನ್ಯಥಾ ಯದಿ (kind == safepoint_resume) {
            M3Frame* frame;
            u32      ಅಂಕೆ_Results = point->ಅಂಕೆ_Results;
            pc_t     resultsPC  = point->pc - 2 * ಅಂಕೆ_Results;

_           (AddFrame(l, &ಅಂಕೆ_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.ಅಂಕೆ_Results  = ಅಂಕೆ_Results;
            frame->resume.ಅಂಕೆ_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } ಅನ್ಯಥಾ {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  ಯದಿ d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_ಮೌಲ್ಯಗಳು(l, base, ಅಂಕೆ_Slots, sp, map, point, &registers));

        ಯದಿ (not l->checking) {
            RestoreConstants(function, sp);
        }

        ಯದಿ (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } ಅನ್ಯಥಾ ಯದಿ (kind == safepoint_resume) {
            ಶೂನ್ಯ* cont = ರಿಕ್ತ;
_           (Get_Ref_ಮೌಲ್ಯ(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ರಿಕ್ತ);

            callFrame->resume.cont = (IM3Continuation)cont;
            ಯದಿ (not l->checking) {
                l->suspendSites[i_id].point = point;
                ಪ್ರತಿ (u32 i = 1; i < l->ಅಂಕೆ_Continuations; ++i) {
                    ಯದಿ (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        ಭಙ್ಗ;
                    }
                }
            }
        } ಅನ್ಯಥಾ {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            ಯದಿ (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            ಯದಿ (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->ಅಂಕೆ_Results > point->ಅಂಕೆ_ಮೌಲ್ಯಗಳು or
                                                point->ಅಂಕೆ_Results > d_m3MaxContinuationPayload);

                io_cont->ಅಂಕೆ_Suspend_Results = point->ಅಂಕೆ_Results;
                ಯದಿ (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                ಪ್ರತಿ (u32 i = 0; i < point->ಅಂಕೆ_Results; ++i) {
                    ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* ಮೌಲ್ಯ = &map->ಮೌಲ್ಯಗಳು[point->first_ಮೌಲ್ಯ + point->ಅಂಕೆ_ಮೌಲ್ಯಗಳು - point->ಅಂಕೆ_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(ಮೌಲ್ಯ->slot));

                    io_cont->suspendResultOffsets[i] = ಮೌಲ್ಯ->slot;
                    io_cont->suspendResultIs64[i]    = Is64BitType(ಮೌಲ್ಯ->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    ಯದಿ (l->checking) {
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }

    // held innermost first
    ಯದಿ (ಅಂಕೆ_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, ಅಂಕೆ_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = ಅಂಕೆ_Frames;
    }

    ಪ್ರತಿ (u32 k = 0; k < ಅಂಕೆ_Frames; ++k) {
        io_cont->frames[ಅಂಕೆ_Frames - 1 - k] = l->frames[k];
    }

    io_cont->ಅಂಕೆ_Frames = ಅಂಕೆ_Frames;

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3Result        result = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    ಯದಿ (l->checking) {
        scratch          = *cont;
        scratch.val_ರಾಶಿ = l->check_ರಾಶಿ;
        ಯದಿ (i_id == 0) {
            scratch.val_ರಾಶಿ = (m3slot_t*)l->runtime->origin_ರಾಶಿ;
        }
        cont = &scratch;
    }

    u32 contId = 0, typeIndex = 0, entryIndex = 0, bound_Args_ಎಣಿಕೆ = 0;
    u8  state = 0, is_ಬೇರು = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_ಬೇರು));
_   (GetLEB_u32(l, &typeIndex));
_   (GetLEB_u32(l, &entryIndex));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_ಬೇರು != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_ಬೇರು and state != snapshot_contSuspended);
    ಯದಿ (not is_ಬೇರು and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, typeIndex != d_m3SnapshotNone and typeIndex >= module->ಅಂಕೆ_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entryIndex >= module->ಅಂಕೆ_Functions);
    _throwif(m3Err_wasmMalformed, is_ಬೇರು ? typeIndex != d_m3SnapshotNone : (typeIndex == d_m3SnapshotNone or not module->funcTypes[typeIndex]->isContinuation));

    ಯದಿ (not l->checking and not is_ಬೇರು and state != snapshot_contFinished) {
_       (Continuation_Acquire_ರಾಶಿ(l->runtime, cont));
    }
    cont->type          = (typeIndex != d_m3SnapshotNone) ? module->funcTypes[typeIndex] : ರಿಕ್ತ;
    cont->entryFunction = &module->functions[entryIndex];
    cont->ಅಂಕೆ_Handlers   = 0;
    cont->handlersPC    = ರಿಕ್ತ;
    cont->parent        = ರಿಕ್ತ;
    cont->ಅಂಕೆ_Frames     = 0;

#  ಯದಿ d_m3HasExceptionHandling
    {
        ಶೂನ್ಯ* ref = ರಿಕ್ತ;
_       (Get_Ref_ಮೌಲ್ಯ(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_ಬೇರು or state == snapshot_contFinished) and cont->resumeThrow != ರಿಕ್ತ);
#  ಅನ್ಯಥಾ
    {
        ಶೂನ್ಯ* ref = ರಿಕ್ತ;
_       (Get_Ref_ಮೌಲ್ಯ(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ರಿಕ್ತ);
    }
#  endif

    ಯದಿ (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         ಅಂಕೆ_Rets;

_       (GetLEB_u32(l, &bound_Args_ಎಣಿಕೆ));
        cont->bound_Args_ಎಣಿಕೆ = bound_Args_ಎಣಿಕೆ;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_ಎಣಿಕೆ > Get_Func_Type_ಅಂಕೆ_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_ಅಂಕೆ_Params(remaining) != Get_Func_Type_ಅಂಕೆ_Params(inner) - bound_Args_ಎಣಿಕೆ or
                                            Get_Func_Type_ಅಂಕೆ_Results(remaining) != Get_Func_Type_ಅಂಕೆ_Results(inner));
            ಪ್ರತಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(inner, i) != GetFuncTypeResultType(remaining, i));
            }
            ಪ್ರತಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_ಎಣಿಕೆ + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        ಅಂಕೆ_Rets = Get_Func_Type_ಅಂಕೆ_Results(inner);

        _throwif(m3Err_wasmMalformed, (ಅಂಕೆ_Rets + bound_Args_ಎಣಿಕೆ) * c_io_Slot_ಎಣಿಕೆ > cont->ಅಂಕೆ_ರಾಶಿ_Slots);

        ಪ್ರತಿ (u32 i = 0; i < bound_Args_ಎಣಿಕೆ; ++i) {
_           (Get_ಮೌಲ್ಯ(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ರಾಶಿ + (ಅಂಕೆ_Rets + i) * c_io_Slot_ಎಣಿಕೆ));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ರಾಶಿ;
        cont->pc    = cont->entryFunction->compiled;
    } ಅನ್ಯಥಾ ಯದಿ (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_ಎಣಿಕೆ));
        _throwif(m3Err_wasmMalformed, is_ಬೇರು and bound_Args_ಎಣಿಕೆ != 0);
        cont->bound_Args_ಎಣಿಕೆ = bound_Args_ಎಣಿಕೆ;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } ಅನ್ಯಥಾ ಯದಿ (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_ಎಣಿಕೆ));
        _throwif(m3Err_wasmMalformed, bound_Args_ಎಣಿಕೆ != 0);
        cont->bound_Args_ಎಣಿಕೆ = 0;
        cont->state          = cont_consumed;
    } ಅನ್ಯಥಾ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ಯದಿ (result) {
        cont->ಅಂಕೆ_Frames = 0;
    }

    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadContinuations (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u32      ಎಣಿಕೆ  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &ಎಣಿಕೆ));
    _throwif(m3Err_wasmMalformed, ಎಣಿಕೆ == 0 or ಎಣಿಕೆ != l->ಅಂಕೆ_Continuations);

    ಪ್ರತಿ (u32 i = 0; i < ಎಣಿಕೆ; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ಸ್ಥಿರ
M3Result CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;

    ಪ್ರತಿ (u32 id = 0; id < l->ಅಂಕೆ_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  ಯದಿ d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        ಯದಿ (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : ರಿಕ್ತ;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ಅಂಕೆ_Results(actual) != Get_Func_Type_ಅಂಕೆ_Results(expected));
            ಪ್ರತಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(actual, i) != GetFuncTypeResultType(expected, i));
            }
        }
        ಯಾವತ್ (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->ಅಂಕೆ_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        ಯದಿ (cont->state != cont_suspended or id == 0) {
            ಅನುವರ್ತ;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        ಯದಿ (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_ಎಣಿಕೆ != 0);
            ಅನುವರ್ತ;
        }

        ಧ್ರುವ M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_ಎಣಿಕೆ > inner->ಅಂಕೆ_Suspend_Results or
                                                                          Get_Func_Type_ಅಂಕೆ_Params(remaining) != inner->ಅಂಕೆ_Suspend_Results - cont->bound_Args_ಎಣಿಕೆ);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_ಅಂಕೆ_Results(remaining) != Get_Func_Type_ಅಂಕೆ_Results(cont->entryFunction->funcType));
        ಪ್ರತಿ (u16 i = 0; i < Get_Func_Type_ಅಂಕೆ_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(remaining, i) !=
                                            GetFuncTypeResultType(cont->entryFunction->funcType, i));
        }

        ಪ್ರತಿ (u32 i = 0; i < inner->ಅಂಕೆ_Suspend_Results; ++i) {
            ಧ್ರುವ M3_Slot_ಮೌಲ್ಯ* ಮೌಲ್ಯ = &site->map->ಮೌಲ್ಯಗಳು[site->point->first_ಮೌಲ್ಯ +
                                                          site->point->ಅಂಕೆ_ಮೌಲ್ಯಗಳು - site->point->ಅಂಕೆ_Results + i];
            ಯದಿ (i < cont->bound_Args_ಎಣಿಕೆ) {
                ಯದಿ (IsRefType(ಮೌಲ್ಯ->type) and not IsNullableRef(ಮೌಲ್ಯ->type)) {
                    ಶೂನ್ಯ* ref = ರಿಕ್ತ;
                    memcpy(&ref, inner->sp + ಮೌಲ್ಯ->slot, ಪರಿಮಾಣ(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } ಅನ್ಯಥಾ {
                _throwif("a captured continuation has the wrong parameter type", ಮೌಲ್ಯ->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_ಎಣಿಕೆ)));
            }
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

#  ಯದಿ d_m3HasExceptionHandling

ಸ್ಥಿರ
M3Result LoadExceptions (M3SnapshotLoad* l)
{
    M3Result result        = m3Err_none;
    u32      ಅಂಕೆ_Exceptions = 0;

_   (GetLEB_u32(l, &ಅಂಕೆ_Exceptions));
    _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Exceptions == 0 or ಅಂಕೆ_Exceptions != l->ಅಂಕೆ_Exceptions);

    ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Exceptions; ++i) {
        u32          exnId = 0, ಅಂಕೆ_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &ಅಂಕೆ_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, ಅಂಕೆ_Args != exn->ಅಂಕೆ_Args);

        ಪ್ರತಿ (u32 a = 0; a < ಅಂಕೆ_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_ಮೌಲ್ಯ_Bits(l, argType, &bits));
            ಯದಿ (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3Result        error;
} M3BoundedReader;

ಸ್ಥಿರ
M3Result BoundedReader_Read (ಶೂನ್ಯ* o_buffer, size_t i_size, ಶೂನ್ಯ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    ಯದಿ (i_size > br->remaining) {
        br->error = "the host read past the end of its state";
        ಪ್ರತ್ಯಾಗಮ br->error;
    }

    br->remaining -= i_size;

    ಪ್ರತ್ಯಾಗಮ Get(br->load, o_buffer, i_size);
}

ಸ್ಥಿರ
M3Result LoadHostState (M3SnapshotLoad* l)
{
    M3Result               result = m3Err_none;
    ಧ್ರುವ M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 size = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    ಯದಿ (l->checking) {
        ಪ್ರತ್ಯಾಗಮ SkipBytes(l, size);
    }
    reader.load      = l;
    reader.remaining = size;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, size));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadHeader (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u8       magic[ಪರಿಮಾಣ(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, ಪರಿಮಾಣ(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, ಪರಿಮಾಣ(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadMeta (M3SnapshotLoad* l, u32* o_flags)
{
    M3Result result      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      memoryBytes = 0, tableElements = 0;
    u32      activeStacks = 0;
    u64      currentMemory, currentTables;
    SnapshotResourceTotals(l->module, &currentMemory, &currentTables);

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
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->ಅಂಕೆ_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->ಅಂಕೆ_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    ಯದಿ (*o_flags & d_m3SnapshotFlagPostmortem) {
        ಪ್ರತಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
        }
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->ಅಂಕೆ_Continuations == 0 or activeStacks >= l->ಅಂಕೆ_Continuations);
#  ಯದಿ !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->ಅಂಕೆ_Exceptions != 0);
#  endif

    ಯದಿ (l->checking) {
        ಪ್ರತಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
#  ಯದಿ d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tagIndex >= l->module->ಅಂಕೆ_Tags);
#  endif
        }
        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }

    l->check_ರಾಶಿ = m3_AllocArray(m3slot_t, d_m3_Continuation_ರಾಶಿ_Slots + 4);
    _throwifnull(l->check_ರಾಶಿ);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->ಅಂಕೆ_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->ಅಂಕೆ_Continuations);
    _throwifnull(l->suspendSites);

    ಯದಿ (not l->runtime->ಬೇರು_Continuation) {
        IM3Continuation ಬೇರು = Continuation_New(l->runtime, ರಿಕ್ತ, ರಿಕ್ತ);
        _throwifnull(ಬೇರು);

        // the root borrows the runtime's stack, so it holds none of its own
        ಬೇರು->ಅಂಕೆ_ರಾಶಿ_Slots          = l->runtime->ಅಂಕೆ_ರಾಶಿ_Slots;
        l->runtime->ಬೇರು_Continuation = ಬೇರು;
    }

    l->continuations[0] = l->runtime->ಬೇರು_Continuation;

    ಪ್ರತಿ (u32 i = 1; i < l->ಅಂಕೆ_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ರಿಕ್ತ, ರಿಕ್ತ);
        _throwifnull(l->continuations[i]);
    }

#  ಯದಿ d_m3HasExceptionHandling
    ಯದಿ (l->ಅಂಕೆ_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->ಅಂಕೆ_Exceptions);
        _throwifnull(l->exceptions);
    }

    ಪ್ರತಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
        u32    tagIndex = 0;
        IM3Tag tag;
        u32    ಅಂಕೆ_Args;

_       (GetLEB_u32(l, &tagIndex));
        _throwif(m3Err_wasmMalformed, tagIndex >= l->module->ಅಂಕೆ_Tags);

        tag     = TagOfIndex(l->module, tagIndex);
        ಅಂಕೆ_Args = Get_Func_Type_ಅಂಕೆ_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, ಅಂಕೆ_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  ಅನ್ಯಥಾ
    ಪ್ರತಿ (u32 i = 0; i < l->ಅಂಕೆ_Exceptions; ++i) {
        u32 tagIndex = 0;
_       (GetLEB_u32(l, &tagIndex));
    }
#  endif

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ಸ್ಥಿರ
M3Result LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3Result result = m3Err_none;
    u32      flags  = 0;
    ವಿಕಲ್ಪ (i_sectionId) {
    ಪಕ್ಷ d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        ಭಙ್ಗ;

    ಪಕ್ಷ d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        ಭಙ್ಗ;

    ಪಕ್ಷ d_m3SnapshotSection_Table:
_       (LoadTables(l));
        ಭಙ್ಗ;

    ಪಕ್ಷ d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        ಭಙ್ಗ;

    ಪಕ್ಷ d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        ಭಙ್ಗ;

#  ಯದಿ d_m3HasExceptionHandling
    ಪಕ್ಷ d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        ಭಙ್ಗ;
#  endif

    ಪಕ್ಷ d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        ಭಙ್ಗ;

    ಪಕ್ಷ d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        ಭಙ್ಗ;

    ಸ್ವಭಾವ:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        ಭಙ್ಗ;
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

ಸ್ಥಿರ
M3Result LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3Result result  = m3Err_none;
    u8*      payload = ರಿಕ್ತ;

    ಯದಿ (not i_embedded) {
_       (LoadHeader(l));
    }

    ಯಾವತ್ (true) {
        u8  sectionId   = 0;
        u32 sectionSize = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        ಯದಿ (GetU8(l, &sectionId)) {
            ಭಙ್ಗ;
        }

_       (GetLEB_u32(l, &sectionSize));

        _throwif("the snapshot holds a section twice", l->seenSections[sectionId]);
        l->seenSections[sectionId] = true;
        _throwif("the snapshot holds an empty section", sectionSize == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", sectionSize);
        _throwifnull(payload);
_       (Get(l, payload, sectionSize));
        l->pos       = payload;
        l->end       = payload + sectionSize;
        l->inSection = true;

        {
            M3SnapshotLoad check = *l;
            check.checking       = true;
_           (LoadSection(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.end);
        }
        l->changed = l->changed or sectionId != d_m3SnapshotSection_Meta;
_       (LoadSection(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->end);
        l->inSection = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->seenSections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->ಅಂಕೆ_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->ಅಂಕೆ_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->ಅಂಕೆ_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->ಅಂಕೆ_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->ಅಂಕೆ_Data_Segments or l->module->ಅಂಕೆ_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    ಪ್ರತ್ಯಾಗಮ result;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#ಯದಿ d_m3HasSnapshots

ಸ್ಥಿರ
M3Result SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ಶೂನ್ಯ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3Result       result = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, ಪರಿಮಾಣ(save));

    IM3Continuation ಬೇರು = io_runtime->ಬೇರು_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not ಬೇರು or ಬೇರು->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : ಬೇರು->entryFunction;

    ಯದಿ (o_module) {
        *o_module = ರಿಕ್ತ;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    ಯದಿ (o_module) {
        *o_module = save.module;
    }

    ಪ್ರತಿ (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->next) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  ಯದಿ d_m3HasExceptionHandling
    ಪ್ರತಿ (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->next) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    ಯದಿ (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, ಬೇರು, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    ಪ್ರತ್ಯಾಗಮ result;
}

#endif

M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ಶೂನ್ಯ* i_userdata)
{
    ಯದಿ (not io_runtime or not i_writer) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }

#ಯದಿ d_m3HasSnapshots
    ಪ್ರತ್ಯಾಗಮ SaveSnapshot(io_runtime, i_writer, i_userdata, ರಿಕ್ತ, false);
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_userdata;
    ಪ್ರತ್ಯಾಗಮ "snapshots are not available in this build of Wasm3";
#endif
}


ಸ್ಥಿರ
M3Result LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ಶೂನ್ಯ* i_userdata, bool i_embedded)
{
    M3Result result = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#ಯದಿ d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, ಪರಿಮಾಣ(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        result = LoadSections(&load, i_embedded);

        IM3Continuation ಬೇರು = io_runtime->ಬೇರು_Continuation;

        ಯದಿ (result) {
            ಯದಿ (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                ಯದಿ (ಬೇರು) {
                    ಬೇರು->state     = cont_returned;
                    ಬೇರು->ಅಂಕೆ_Frames = 0;
                }
            }
        } ಅನ್ಯಥಾ {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_ರಾಶಿ(io_runtime);

        m3_Free(load.check_ರಾಶಿ);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  ಯದಿ d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}


M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ಶೂನ್ಯ* i_userdata)
{
    ಪ್ರತ್ಯಾಗಮ LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


ಶೂನ್ಯ m3_SetSnapshotHooks (IM3Runtime io_runtime, ಧ್ರುವ M3SnapshotHooks* i_hooks)
{
#ಯದಿ d_m3HasSnapshots
    ಯದಿ (io_runtime) {
        ಯದಿ (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } ಅನ್ಯಥಾ {
            memset(&io_runtime->snapshotHooks, 0, ಪರಿಮಾಣ(io_runtime->snapshotHooks));
        }
    }
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)io_runtime;
    (ಶೂನ್ಯ)i_hooks;
#endif
}


M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, ಶೂನ್ಯ** o_bytes, size_t* o_size)
{
    ಯದಿ (!io_runtime || !o_bytes || !o_size) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }
#ಯದಿ d_m3HasSnapshots
    M3BufferWriter bw     = { ರಿಕ್ತ, 0, 0 };
    M3Result       result = m3_SaveSnapshot(io_runtime, BufferWriter_Write, &bw);
    ಯದಿ (result) {
        m3_Free(bw.buffer);
        *o_bytes = ರಿಕ್ತ;
        *o_size  = 0;
        ಪ್ರತ್ಯಾಗಮ result;
    }
    *o_bytes = bw.buffer;
    *o_size  = bw.size;
    ಪ್ರತ್ಯಾಗಮ m3Err_none;
#ಅನ್ಯಥಾ
    *o_bytes = ರಿಕ್ತ;
    *o_size  = 0;
    ಪ್ರತ್ಯಾಗಮ "snapshots are not available in this build of Wasm3";
#endif
}

#ಯದಿ d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ಸ್ಥಿರ
u8* EmitLEB_u32 (u8* o_where, u32 i_ಮೌಲ್ಯ)
{
    ಕುರು {
        u8 byte = i_ಮೌಲ್ಯ & 0x7F;
        i_ಮೌಲ್ಯ >>= 7;
        ಯದಿ (i_ಮೌಲ್ಯ != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } ಯಾವತ್ (i_ಮೌಲ್ಯ != 0);

    ಪ್ರತ್ಯಾಗಮ o_where;
}

ಸ್ಥಿರ
u32 SizeOfLEB_u32 (u32 i_ಮೌಲ್ಯ)
{
    u32 size = 0;

    ಕುರು {
        i_ಮೌಲ್ಯ >>= 7;
        ++size;
    } ಯಾವತ್ (i_ಮೌಲ್ಯ != 0);

    ಪ್ರತ್ಯಾಗಮ size;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ಸ್ಥಿರ
M3Result FindEmbeddedSnapshot (ಧ್ರುವ u8* i_start, ಧ್ರುವ u8* i_end, cstr_t i_secName,
                               ಧ್ರುವ u8** o_secStart, ಧ್ರುವ u8** o_secEnd,
                               ಧ್ರುವ u8** o_data, u32* o_size)
{
    M3Result result = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = ರಿಕ್ತ;

    ಯದಿ (o_secStart) {
        *o_secStart = ರಿಕ್ತ;
    }
    ಯದಿ (o_secEnd) {
        *o_secEnd = ರಿಕ್ತ;
    }
    ಯದಿ (o_data) {
        *o_data = ರಿಕ್ತ;
    }
    ಯದಿ (o_size) {
        *o_size = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    ಯಾವತ್ (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     size      = 0;
_       (ReadLEB_u32(&size, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, size > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += size;
            ಯದಿ (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                ಯದಿ (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    ಯದಿ (o_secStart) {
                        *o_secStart = secStart;
                    }
                    ಯದಿ (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    ಯದಿ (o_data) {
                        *o_data = payload;
                    }
                    ಯದಿ (o_size) {
                        *o_size = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    ಪ್ರತ್ಯಾಗಮ result;
}

// Builds the custom section name a snapshot of i_name lives under
ಸ್ಥಿರ
M3Result SnapshotSectionName (cstr_t i_name, ವರ್ಣ* o_secName, size_t i_size)
{
    ಯದಿ (i_name and *i_name) {
        ಯದಿ (strlen(i_name) > d_m3MaxSnapshotNameLength) {
            ಪ್ರತ್ಯಾಗಮ "the snapshot name is too long";
        }
        snprintf(o_secName, i_size, "snapshot.%s", i_name);
    } ಅನ್ಯಥಾ {
        snprintf(o_secName, i_size, "snapshot");
    }

    ಪ್ರತ್ಯಾಗಮ m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ಸ್ಥಿರ
M3Result ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3Result result = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ರಿಕ್ತ;

    ಪ್ರತಿ (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->next) {
        ಯದಿ (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    ಪ್ರತ್ಯಾಗಮ result;
}
#endif

M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, ಧ್ರುವ ಶೂನ್ಯ* i_bytes, size_t i_size)
{
    ಯದಿ (!io_runtime || !i_module || !i_bytes) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }
#ಯದಿ d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    ಯದಿ (i_size >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        ಧ್ರುವ u8* data   = ರಿಕ್ತ;
        u32       size   = 0;
        M3Result  result = FindEmbeddedSnapshot((ಧ್ರುವ u8*)i_bytes, (ಧ್ರುವ u8*)i_bytes + i_size,
                                                "snapshot", ರಿಕ್ತ, ರಿಕ್ತ, &data, &size);
        ಯದಿ (result) {
            ಪ್ರತ್ಯಾಗಮ result;
        }
        ಯದಿ (not data) {
            ಪ್ರತ್ಯಾಗಮ "the Wasm binary holds no embedded snapshot";
        }

        {
            M3BufferReader br = { data, size, 0 };
            ಪ್ರತ್ಯಾಗಮ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
        }
    }
    {
        M3BufferReader br = { (ಧ್ರುವ u8*)i_bytes, i_size, 0 };
        ಪ್ರತ್ಯಾಗಮ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br,
                            i_size < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_size;
    ಪ್ರತ್ಯಾಗಮ "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, ಧ್ರುವ ವರ್ಣ* i_name)
{
    ಯದಿ (not i_module) {
        ಪ್ರತ್ಯಾಗಮ false;
    }
#ಯದಿ d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ರಿಕ್ತ;
    (ಶೂನ್ಯ)ModuleSnapshot(i_module, i_name, &snap);

    ಪ್ರತ್ಯಾಗಮ snap != ರಿಕ್ತ;
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_name;
    ಪ್ರತ್ಯಾಗಮ false;
#endif
}

M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, ಧ್ರುವ ವರ್ಣ* i_name,
                                 ಧ್ರುವ ಶೂನ್ಯ** o_bytes, size_t* o_size)
{
    ಯದಿ (not i_module or not o_bytes or not o_size) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }

    *o_bytes = ರಿಕ್ತ;
    *o_size  = 0;

#ಯದಿ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ರಿಕ್ತ;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);

        ಯದಿ (result) {
            ಪ್ರತ್ಯಾಗಮ result;
        }
        ಯದಿ (not snap) {
            ಪ್ರತ್ಯಾಗಮ "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->data;
        *o_size  = snap->size;

        ಪ್ರತ್ಯಾಗಮ m3Err_none;
    }
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_name;
    ಪ್ರತ್ಯಾಗಮ "snapshots are not available in this build of Wasm3";
#endif
}

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, ಧ್ರುವ ವರ್ಣ* i_name)
{
    ಯದಿ (not io_runtime or not i_module) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }
#ಯದಿ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ರಿಕ್ತ;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);
        M3BufferReader      br;

        ಯದಿ (result) {
            ಪ್ರತ್ಯಾಗಮ result;
        }
        ಯದಿ (not snap) {
            ಪ್ರತ್ಯಾಗಮ "the module holds no embedded snapshot of that name";
        }

        br.buffer = snap->data;
        br.size   = snap->size;
        br.cursor = 0;

        ಪ್ರತ್ಯಾಗಮ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
    }
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)i_name;
    ಪ್ರತ್ಯಾಗಮ "snapshots are not available in this build of Wasm3";
#endif
}


M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, ಧ್ರುವ ವರ್ಣ* i_name,
                                  ಶೂನ್ಯ** o_bytes, size_t* o_size)
{
#ಯದಿ d_m3HasSnapshots
    M3Result       result = m3Err_none;
    M3BufferWriter bw     = { ರಿಕ್ತ, 0, 0 };
    u8*            out    = ರಿಕ್ತ;

    ವರ್ಣ      secName[d_m3MaxSnapshotNameLength + ಪರಿಮಾಣ("snapshot.")];
    u32       nameLen, nameLenLebSize, payloadSize, payloadSizeLebSize;
    size_t    origSize, totalSize;
    ಧ್ರುವ u8* dropStart = ರಿಕ್ತ;
    ಧ್ರುವ u8* dropEnd   = ರಿಕ್ತ;
    IM3Module saved     = ರಿಕ್ತ;
    u8*       p;

    ಯದಿ (not io_runtime or not i_module or not o_bytes or not o_size) {
        ಪ್ರತ್ಯಾಗಮ m3Err_mallocFailed;
    }

    *o_bytes = ರಿಕ್ತ;
    *o_size  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, ಪರಿಮಾಣ(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->ಬೇರು_Continuation or io_runtime->ಬೇರು_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, BufferWriter_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.size);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.size > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, ರಿಕ್ತ, ರಿಕ್ತ));

    origSize           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    nameLenLebSize     = SizeOfLEB_u32(nameLen);
    payloadSize        = nameLenLebSize + nameLen + (u32)bw.size;
    payloadSizeLebSize = SizeOfLEB_u32(payloadSize);
    totalSize          = origSize + 1 + payloadSizeLebSize + payloadSize;

    out = (u8*)m3_Malloc("wasm_snapshot_module", totalSize);
    _throwifnull(out);

    p = out;

    ಯದಿ (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } ಅನ್ಯಥಾ {
        memcpy(p, i_module->wasmStart, origSize);
        p += origSize;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payloadSize);
    p    = EmitLEB_u32(p, nameLen);
    memcpy(p, secName, nameLen);
    p += nameLen;
    memcpy(p, bw.buffer, bw.size);

    *o_bytes = out;
    *o_size  = totalSize;
    out      = ರಿಕ್ತ;

_catch:
    m3_Free(bw.buffer);
    m3_Free(out);

    ಪ್ರತ್ಯಾಗಮ result;
#ಅನ್ಯಥಾ
    (ಶೂನ್ಯ)io_runtime;
    (ಶೂನ್ಯ)i_module;
    (ಶೂನ್ಯ)i_name;
    (ಶೂನ್ಯ)o_bytes;
    (ಶೂನ್ಯ)o_size;
    ಪ್ರತ್ಯಾಗಮ "snapshots are not available in this build of Wasm3";
#endif
}
