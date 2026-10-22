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

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_compile.h"
#അന്തര്ഭാവ "m3_exception.h"
#അന്തര്ഭാവ "m3_host.h"

#അന്തര്ഭാവ <limits.h>

#എങ്കിൽ d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
സ്ഥിതം സ്ഥിരം u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
സ്ഥിതം സ്ഥിരം u32 c_snapshotVersion  = 1;

എണ്ണൽ {
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
എണ്ണൽ {
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
സ്ഥിതം
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

        ആകുമ്പോൾ (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     size      = 0;

_           (ReadLEB_u32(&size, &pos, end));
            _throwif(m3Err_wasmUnderrun, size > (size_t)(end - pos));

            pos += size;
            എങ്കിൽ (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    തിരികെ result;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

സ്ഥിതം
സ്ഥിരം M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    സ്ഥിരം M3SnapshotMap* map = i_function->snapshotMap;

    എങ്കിൽ (map) {
        വേണ്ടി (u32 i = 0; i < map->അക്കം_Safe_Points; ++i) {
            എങ്കിൽ (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                തിരികെ &map->safePoints[i];
            }
        }
    }

    തിരികെ ശൂന്യ;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
സ്ഥിതം
സ്ഥിരം M3SafePoint* FindSafePointAt (സ്ഥിരം M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 താഴെയുള്ള  = 0;
    u32 മുകളിലെ = i_map->അക്കം_Safe_Points;

    ആകുമ്പോൾ (താഴെയുള്ള < മുകളിലെ) {
        u32 നടുവ് = താഴെയുള്ള + (മുകളിലെ - താഴെയുള്ള) / 2;

        എങ്കിൽ (i_map->safePoints[നടുവ്].wasmOffset < i_wasmOffset) {
            താഴെയുള്ള = നടുവ് + 1;
        } അല്ലെങ്കിൽ {
            മുകളിലെ = നടുവ്;
        }
    }

    വേണ്ടി (u32 i = താഴെയുള്ള; i < i_map->അക്കം_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        സ്ഥിരം M3SafePoint* point = &i_map->safePoints[i];

        എങ്കിൽ (point->kind != i_kind or (point->flags & d_m3SafePointTailCall)) {
            തുടരുക;
        }
        എങ്കിൽ (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            തിരികെ point;
        }
    }

    തിരികെ ശൂന്യ;
}

സ്ഥിതം
സ്ഥിരം M3BlockStart* FindBlockAt (സ്ഥിരം M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    വേണ്ടി (u32 i = 0; i < i_map->അക്കം_Blocks; ++i) {
        എങ്കിൽ (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            തിരികെ &i_map->blocks[i];
        }
    }

    തിരികെ ശൂന്യ;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
സ്ഥിതം വരിയിൽ
bool EnclosesOffset (സ്ഥിരം M3BlockStart* i_block, u32 i_wasmOffset)
{
    തിരികെ i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
സ്ഥിതം വരിയിൽ
bool HandlersLiveAt (സ്ഥിരം M3SnapshotMap* i_map, സ്ഥിരം M3SafePoint* i_point, സ്ഥിരം M3BlockStart* i_try)
{
    തിരികെ not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
സ്ഥിതം
M3Result CallSiteType (IM3Function i_caller, സ്ഥിരം M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3Result   result = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        size   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ശൂന്യ;

_   (ReadLEB_u32(&size, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    എങ്കിൽ (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->അക്കം_Functions);
        *o_type = module->functions[index].funcType;
    } അല്ലെങ്കിൽ {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->അക്കം_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    തിരികെ result;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
സ്ഥിതം
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 അക്കം_Results = Get_Func_Type_അക്കം_Results(i_expected);

    എങ്കിൽ (not i_type or Get_Func_Type_അക്കം_Results(i_type) != അക്കം_Results) {
        തിരികെ false;
    }

    വേണ്ടി (u16 i = 0; i < അക്കം_Results; ++i) {
        എങ്കിൽ (not IsSubTypeOf(GetFuncTypeResultType(i_type, i), GetFuncTypeResultType(i_expected, i))) {
            തിരികെ false;
        }
    }

    തിരികെ true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
തരനിർവചനം ഘടന M3FrameRegisters {
    m3reg_t* r0;
#  എങ്കിൽ d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
സ്ഥിതം
M3Result Continuation_അട്ടി (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_അക്കം_Slots)
{
    എങ്കിൽ (i_cont == i_runtime->വേര്_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_അട്ടി;
        *o_അക്കം_Slots = i_runtime->അക്കം_അട്ടി_Slots;
    } അല്ലെങ്കിൽ {
        *o_base     = i_cont->val_അട്ടി;
        *o_അക്കം_Slots = i_cont->അക്കം_അട്ടി_Slots;
    }

    തിരികെ (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
സ്ഥിതം
M3Result Locate_മൂല്യം (m3slot_t* i_base, u32 i_അക്കം_Slots, m3stack_t i_sp, സ്ഥിരം M3_Slot_മൂല്യം* i_മൂല്യം, u8** o_where)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_മൂല്യം->type);

    *o_where = ശൂന്യ;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_മൂല്യം->slot));

    എങ്കിൽ (not IsRegisterSlotAlias(i_മൂല്യം->slot)) {
        size_t size   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? വലുപ്പം(ശൂന്യം*)
                                                                   : (Is64BitType(type) ? വലുപ്പം(u64) : വലുപ്പം(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_മൂല്യം->slot) * വലുപ്പം(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + size > (size_t)i_അക്കം_Slots * വലുപ്പം(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    തിരികെ result;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
സ്ഥിതം
M3Result Read_മൂല്യം (u8* i_where, സ്ഥിരം M3FrameRegisters* i_registers, സ്ഥിരം M3_Slot_മൂല്യം* i_മൂല്യം, u64* o_bits,
                    ശൂന്യം** o_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_മൂല്യം->type);

    *o_bits      = 0;
    *o_reference = ശൂന്യ;

    എങ്കിൽ (i_where) {
        എങ്കിൽ (IsRefType(type)) {
            memcpy(o_reference, i_where, വലുപ്പം(ശൂന്യം*));
        } അല്ലെങ്കിൽ എങ്കിൽ (Is64BitType(type)) {
            memcpy(o_bits, i_where, വലുപ്പം(u64));
        } അല്ലെങ്കിൽ {
            u32 narrow;
            memcpy(&narrow, i_where, വലുപ്പം(narrow));
            *o_bits = narrow;
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (IsFpRegisterSlotAlias(i_മൂല്യം->slot)) {
#  എങ്കിൽ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        എങ്കിൽ (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, വലുപ്പം(bits));
            *o_bits = bits;
        } അല്ലെങ്കിൽ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, വലുപ്പം(u64));
        }
#  അല്ലെങ്കിൽ
        _throw(m3Err_wasmMalformed);
#  endif
    } അല്ലെങ്കിൽ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        എങ്കിൽ (IsRefType(type)) {
            *o_reference = (ശൂന്യം*)(uintptr_t)r0;
        } അല്ലെങ്കിൽ എങ്കിൽ (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } അല്ലെങ്കിൽ {
            *o_bits = (u32)r0;
        }
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result Write_മൂല്യം (u8* o_where, സ്ഥിരം M3FrameRegisters* i_registers, സ്ഥിരം M3_Slot_മൂല്യം* i_മൂല്യം, u64 i_bits,
                     ശൂന്യം* i_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_മൂല്യം->type);

    എങ്കിൽ (o_where) {
        എങ്കിൽ (IsRefType(type)) {
            memcpy(o_where, &i_reference, വലുപ്പം(ശൂന്യം*));
        } അല്ലെങ്കിൽ എങ്കിൽ (Is64BitType(type)) {
            memcpy(o_where, &i_bits, വലുപ്പം(u64));
        } അല്ലെങ്കിൽ {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, വലുപ്പം(narrow));
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (IsFpRegisterSlotAlias(i_മൂല്യം->slot)) {
#  എങ്കിൽ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        എങ്കിൽ (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, വലുപ്പം(narrow));
            *i_registers->fp0 = narrow;
        } അല്ലെങ്കിൽ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, വലുപ്പം(u64));
        }
#  അല്ലെങ്കിൽ
        _throw(m3Err_wasmMalformed);
#  endif
    } അല്ലെങ്കിൽ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        എങ്കിൽ (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } അല്ലെങ്കിൽ {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    തിരികെ result;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
സ്ഥിതം
സ്ഥിരം M3_Slot_മൂല്യം* Frame_മൂല്യം (സ്ഥിരം M3SnapshotMap* i_map, സ്ഥിരം M3SafePoint* i_point, u32 i_index)
{
    എങ്കിൽ (i_index < i_map->അക്കം_Locals) {
        തിരികെ i_point->first_Local_മൂല്യം == UINT32_MAX ? &i_map->locals[i_index]
                                                      : &i_map->മൂല്യങ്ങൾ[i_point->first_Local_മൂല്യം + i_index];
    }
    തിരികെ &i_map->മൂല്യങ്ങൾ[i_point->first_മൂല്യം + i_index - i_map->അക്കം_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
സ്ഥിതം
ശൂന്യം RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    എങ്കിൽ (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->അക്കം_Ret_And_Arg_Slots) + i_function->അക്കം_Local_Bytes;

        memcpy(where, i_function->constants, i_function->അക്കം_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
തരനിർവചനം ഘടന M3PointerIds {
    ശൂന്യം** ഇനങ്ങൾ;
    u32    എണ്ണം;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  indexCapacity;       // a power of two, kept at least twice count
} M3PointerIds;

സ്ഥിതം
u32 PointerBucket (ശൂന്യം* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    തിരികെ (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

സ്ഥിതം
u32 PointerIds_Find (സ്ഥിരം M3PointerIds* i_ids, ശൂന്യം* i_pointer)
{
    എങ്കിൽ (i_ids->indexCapacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->indexCapacity);

        ആകുമ്പോൾ (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            എങ്കിൽ (i_ids->ഇനങ്ങൾ[id] == i_pointer) {
                തിരികെ id;
            }

            bucket = (bucket + 1) & (i_ids->indexCapacity - 1);
        }
    }

    തിരികെ d_m3SnapshotNone;
}

സ്ഥിതം
M3Result PointerIds_Add (M3PointerIds* io_ids, ശൂന്യം* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    എങ്കിൽ (id != d_m3SnapshotNone) {
        *o_id = id;
        തിരികെ m3Err_none;
    }

    എങ്കിൽ (io_ids->എണ്ണം == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        ശൂന്യം** ഇനങ്ങൾ    = m3_ReallocArray(ശൂന്യം*, io_ids->ഇനങ്ങൾ, capacity, io_ids->capacity);

        എങ്കിൽ (not ഇനങ്ങൾ) {
            തിരികെ m3Err_mallocFailed;
        }

        io_ids->ഇനങ്ങൾ    = ഇനങ്ങൾ;
        io_ids->capacity = capacity;
    }

    എങ്കിൽ ((io_ids->എണ്ണം + 1) * 2 > io_ids->indexCapacity) {
        u32  capacity = io_ids->indexCapacity ? io_ids->indexCapacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        എങ്കിൽ (not index) {
            തിരികെ m3Err_mallocFailed;
        }

        വേണ്ടി (u32 i = 0; i < io_ids->എണ്ണം; ++i) {
            u32 bucket = PointerBucket(io_ids->ഇനങ്ങൾ[i], capacity);

            ആകുമ്പോൾ (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->indexCapacity = capacity;
    }

    id                = io_ids->എണ്ണം++;
    io_ids->ഇനങ്ങൾ[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->indexCapacity);

    ആകുമ്പോൾ (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->indexCapacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    തിരികെ m3Err_none;
}

സ്ഥിതം
ശൂന്യം PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->ഇനങ്ങൾ);
    m3_Free(io_ids->index);
    memset(io_ids, 0, വലുപ്പം(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

തരനിർവചനം ഘടന M3BufferWriter {
    u8*    buffer;
    size_t size;
    size_t capacity;
} M3BufferWriter;

സ്ഥിതം
M3Result BufferWriter_Write (സ്ഥിരം ശൂന്യം* i_data, size_t i_size, ശൂന്യം* i_userdata)
{
    M3BufferWriter* bw = (M3BufferWriter*)i_userdata;

    എങ്കിൽ (bw->size + i_size > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        ആകുമ്പോൾ (bw->size + i_size > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->buffer, newCap, bw->capacity);
        എങ്കിൽ (!newBuf) {
            തിരികെ m3Err_mallocFailed;
        }
        bw->buffer   = newBuf;
        bw->capacity = newCap;
    }

    എങ്കിൽ (i_size) {
        memcpy(bw->buffer + bw->size, i_data, i_size);
        bw->size += i_size;
    }

    തിരികെ m3Err_none;
}

തരനിർവചനം ഘടന M3BufferReader {
    സ്ഥിരം u8* buffer;
    size_t    size;
    size_t    cursor;
} M3BufferReader;

സ്ഥിതം
M3Result BufferReader_Read (ശൂന്യം* o_buffer, size_t i_size, ശൂന്യം* i_userdata)
{
    M3BufferReader* br = (M3BufferReader*)i_userdata;

    എങ്കിൽ (i_size > br->size - br->cursor) {
        തിരികെ m3Err_wasmMalformed;
    }

    എങ്കിൽ (i_size) {
        memcpy(o_buffer, br->buffer + br->cursor, i_size);
        br->cursor += i_size;
    }

    തിരികെ m3Err_none;
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
തരനിർവചനം ഘടന M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    ശൂന്യം*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

സ്ഥിതം വരിയിൽ
u8 M3TypeToValType (m3type_t i_type)
{
    തിരഞ്ഞെടുപ്പ് (BaseTypeOf(i_type)) {
    അവസ്ഥ c_m3Type_i32: തിരികെ 0x7F;
    അവസ്ഥ c_m3Type_i64: തിരികെ 0x7E;
    അവസ്ഥ c_m3Type_f32: തിരികെ 0x7D;
    അവസ്ഥ c_m3Type_f64: തിരികെ 0x7C;
    അവസ്ഥ c_m3Type_v128: തിരികെ 0x7B;
    അവസ്ഥ c_m3Type_funcref: തിരികെ 0x70;
    അവസ്ഥ c_m3Type_externref: തിരികെ 0x6F;
    അവസ്ഥ c_m3Type_exnref: തിരികെ 0x69;
    അവസ്ഥ c_m3Type_contref: തിരികെ 0x68;
    സ്ഥിരസ്ഥിതി: തിരികെ 0x00;
    }
}

സ്ഥിതം വരിയിൽ
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    തിരഞ്ഞെടുപ്പ് (i_valtype) {
    അവസ്ഥ 0x7F: തിരികെ c_m3Type_i32;
    അവസ്ഥ 0x7E: തിരികെ c_m3Type_i64;
    അവസ്ഥ 0x7D: തിരികെ c_m3Type_f32;
    അവസ്ഥ 0x7C: തിരികെ c_m3Type_f64;
    അവസ്ഥ 0x7B: തിരികെ c_m3Type_v128;
    അവസ്ഥ 0x70: തിരികെ c_m3Type_funcref;
    അവസ്ഥ 0x6F: തിരികെ c_m3Type_externref;
    അവസ്ഥ 0x69: തിരികെ c_m3Type_exnref;
    അവസ്ഥ 0x68: തിരികെ c_m3Type_contref;
    സ്ഥിരസ്ഥിതി: തിരികെ c_m3Type_none;
    }
}

സ്ഥിതം
M3Result Put (M3SnapshotSave* s, സ്ഥിരം ശൂന്യം* i_data, size_t i_size)
{
    തിരികെ s->writing ? s->writer(i_data, i_size, s->userdata) : m3Err_none;
}

സ്ഥിതം
M3Result PutU8 (M3SnapshotSave* s, u8 i_മൂല്യം)
{
    തിരികെ Put(s, &i_മൂല്യം, വലുപ്പം(i_മൂല്യം));
}


സ്ഥിതം
M3Result PutLEB_u32 (M3SnapshotSave* s, u32 i_മൂല്യം)
{
    u8  bytes[5];
    u32 len = 0;

    ചെയ്യുക {
        u8 byte = i_മൂല്യം & 0x7F;
        i_മൂല്യം >>= 7;
        എങ്കിൽ (i_മൂല്യം != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } ആകുമ്പോൾ (i_മൂല്യം != 0);

    തിരികെ Put(s, bytes, len);
}

സ്ഥിതം
M3Result PutLEB_u64 (M3SnapshotSave* s, u64 i_മൂല്യം)
{
    u8  bytes[10];
    u32 len = 0;

    ചെയ്യുക {
        u8 byte = i_മൂല്യം & 0x7F;
        i_മൂല്യം >>= 7;
        എങ്കിൽ (i_മൂല്യം != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } ആകുമ്പോൾ (i_മൂല്യം != 0);

    തിരികെ Put(s, bytes, len);
}

സ്ഥിതം
M3Result PutLEB_i64 (M3SnapshotSave* s, i64 i_മൂല്യം)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    ആകുമ്പോൾ (more) {
        u8 byte = i_മൂല്യം & 0x7F;
        i_മൂല്യം >>= 7;
        എങ്കിൽ ((i_മൂല്യം == 0 && !(byte & 0x40)) || (i_മൂല്യം == -1 && (byte & 0x40))) {
            more = false;
        } അല്ലെങ്കിൽ {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    തിരികെ Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
തരനിർവചനം M3Result (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
സ്ഥിതം
M3Result SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3Result         result   = m3Err_none;
    M3BufferWriter   bw       = { ശൂന്യ, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    ശൂന്യം*            userdata = s->userdata;

    s->writer   = BufferWriter_Write;
    s->userdata = &bw;

    result = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    എങ്കിൽ (result) {
        പോകുക _catch;
    }

    // a section with nothing in it is left out
    എങ്കിൽ (bw.size) {
        _throwif("snapshot section is too large", bw.size > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.size));
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    തിരികെ result;
}

സ്ഥിതം
u32 FunctionIndex (IM3Module i_module, IM3Function i_function)
{
    എങ്കിൽ (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->അക്കം_Functions) {
        തിരികെ (u32)(i_function - i_module->functions);
    }

    തിരികെ d_m3SnapshotNone;
}

സ്ഥിതം
u32 FuncTypeIndex (IM3Module i_module, IM3FuncType i_type)
{
    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Func_Types; ++i) {
        എങ്കിൽ (i_module->funcTypes[i] == i_type) {
            തിരികെ i;
        }
    }

    തിരികെ d_m3SnapshotNone;
}

#  എങ്കിൽ d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
സ്ഥിതം
IM3Tag TagOfIndex (IM3Module i_module, u32 i_index)
{
    IM3Tag tag = &i_module->tags[i_index];

    തിരികെ tag->resolved ? tag->resolved : tag;
}

സ്ഥിതം
u32 TagIndex (IM3Module i_module, IM3Tag i_tag)
{
    വേണ്ടി (u32 i = 0; i < i_module->അക്കം_Tags; ++i) {
        എങ്കിൽ (TagOfIndex(i_module, i) == i_tag) {
            തിരികെ i;
        }
    }

    തിരികെ d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
സ്ഥിതം
M3Result NameReference (M3SnapshotSave* s, u8 i_type, ശൂന്യം* i_reference, u64* o_word)
{
    M3Result result = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    എങ്കിൽ (not i_reference) {
        തിരികെ m3Err_none;
    }

    എങ്കിൽ (i_type == c_m3Type_funcref) {
        id = FunctionIndex(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_externref) {
        സ്ഥിരം M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        പോകുക _catch;
    }
#  എങ്കിൽ d_m3HasExceptionHandling
    അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 എണ്ണം = s->exceptions.എണ്ണം;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.എണ്ണം != എണ്ണം);
    }
#  endif
    അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 എണ്ണം = s->continuations.എണ്ണം;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.എണ്ണം != എണ്ണം);
    } അല്ലെങ്കിൽ {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    തിരികെ result;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
സ്ഥിതം
M3Result Put_Ref_മൂല്യം (M3SnapshotSave* s, u8 i_type, ശൂന്യം* i_reference)
{
    M3Result result = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    എങ്കിൽ (not s->writing) {
        തിരികെ m3Err_none;
    }

    എങ്കിൽ (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } അല്ലെങ്കിൽ {
        u8 kind = 0;
        എങ്കിൽ (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  എങ്കിൽ d_m3HasExceptionHandling
        അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    തിരികെ result;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
സ്ഥിതം
M3Result PutLE32 (M3SnapshotSave* s, u32 i_മൂല്യം)
{
    u8 bytes[4];

    വേണ്ടി (u32 i = 0; i < വലുപ്പം(bytes); ++i) {
        bytes[i] = (u8)(i_മൂല്യം >> (8 * i));
    }

    തിരികെ Put(s, bytes, വലുപ്പം(bytes));
}

സ്ഥിതം
M3Result PutLE64 (M3SnapshotSave* s, u64 i_മൂല്യം)
{
    u8 bytes[8];

    വേണ്ടി (u32 i = 0; i < വലുപ്പം(bytes); ++i) {
        bytes[i] = (u8)(i_മൂല്യം >> (8 * i));
    }

    തിരികെ Put(s, bytes, വലുപ്പം(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
സ്ഥിതം
M3Result Put_മൂല്യം_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    എങ്കിൽ (IsRefType(i_type)) {
        തിരികെ Put_Ref_മൂല്യം(s, BaseTypeOf(i_type), (ശൂന്യം*)(uintptr_t)i_bits);
    }

    തിരഞ്ഞെടുപ്പ് (BaseTypeOf(i_type)) {
    അവസ്ഥ c_m3Type_i32: തിരികെ PutLEB_i64(s, (i32)(u32)i_bits);
    അവസ്ഥ c_m3Type_i64: തിരികെ PutLEB_i64(s, (i64)i_bits);
    അവസ്ഥ c_m3Type_f32: തിരികെ PutLE32(s, (u32)i_bits);
    അവസ്ഥ c_m3Type_f64: തിരികെ PutLE64(s, i_bits);
    സ്ഥിരസ്ഥിതി: തിരികെ "a snapshot cannot hold a value of this type as bits";
    }
}

സ്ഥിതം
M3Result Put_മൂല്യം_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    എങ്കിൽ (not valtype) {
        തിരികെ "a snapshot has no tag for a value of this type";
    }

    തിരികെ PutU8(s, valtype);
}

സ്ഥിതം
M3Result Put_മൂല്യം_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3Result result = m3Err_none;

_   (Put_മൂല്യം_Tag(s, i_type));
_   (Put_മൂല്യം_Body(s, i_type, i_bits));

_catch:
    തിരികെ result;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
സ്ഥിതം
M3Result Put_മൂല്യം (M3SnapshotSave* s, m3type_t i_type, സ്ഥിരം ശൂന്യം* i_where)
{
    M3Result result = m3Err_none;

_   (Put_മൂല്യം_Tag(s, i_type));

    എങ്കിൽ (IsRefType(i_type)) {
        ശൂന്യം* reference;
        memcpy(&reference, i_where, വലുപ്പം(reference));
_       (Put_മൂല്യം_Body(s, i_type, (u64)(uintptr_t)reference));
    } അല്ലെങ്കിൽ എങ്കിൽ (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, വലുപ്പം(bytes));
_       (Put(s, bytes, വലുപ്പം(bytes)));
    } അല്ലെങ്കിൽ എങ്കിൽ (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, വലുപ്പം(bits));
_       (Put_മൂല്യം_Body(s, i_type, bits));
    } അല്ലെങ്കിൽ {
        u32 narrow;
        memcpy(&narrow, i_where, വലുപ്പം(narrow));
_       (Put_മൂല്യം_Body(s, i_type, narrow));
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result PutMemoryChunks (M3SnapshotSave* s, സ്ഥിരം u8* i_bytes, size_t i_size)
{
    M3Result result = m3Err_none;
    size_t   cursor = 0;

    ആകുമ്പോൾ (cursor < i_size) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        ആകുമ്പോൾ (cursor + run < i_size and i_bytes[cursor + run] == byte) {
            run++;
        }

        എങ്കിൽ ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            എങ്കിൽ (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            തുടരുക;
        }

        size_t rawStart = cursor;

        ആകുമ്പോൾ (cursor < i_size) {
            u8     b      = i_bytes[cursor];
            size_t length = 1;

            ആകുമ്പോൾ (cursor + length < i_size and i_bytes[cursor + length] == b) {
                length++;
            }

            എങ്കിൽ ((b == 0x00 or b == 0xFF) and length >= d_m3SnapshotRunThreshold) {
                നിർത്തുക;
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
    തിരികെ result;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
സ്ഥിതം
u32 Memory_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    ആകുമ്പോൾ (module->memories[j] != module->memories[index]) {
        ++j;
    }
    തിരികെ j;
}

സ്ഥിതം
u32 Table_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    ആകുമ്പോൾ (module->tables[j] != module->tables[index]) {
        ++j;
    }
    തിരികെ j;
}

സ്ഥിതം
M3Result SaveMemories (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->അക്കം_Memories));

    വേണ്ടി (u32 m = 0; m < module->അക്കം_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_FirstIndex(module, m);
        bool      hasData  = memory and memory->mallocated;
        u32       pageSize = memory ? Memory_PageSize(memory) : d_m3DefaultMemPageSize;
        u64       bytes    = memory ? memory->അക്കം_Pages * (u64)pageSize : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        എങ്കിൽ (first != m) {
            തുടരുക;
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
        ആകുമ്പോൾ ((1u << pageBits) < pageSize) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->അക്കം_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        എങ്കിൽ (not s->writing) {
            തുടരുക;
        }

        എങ്കിൽ (hasData) {
_           (PutMemoryChunks(s, m3MemData(memory->mallocated), (size_t)bytes));
        } അല്ലെങ്കിൽ {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result SaveGlobals (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->അക്കം_Globals));

    വേണ്ടി (u32 g = 0; g < module->അക്കം_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_മൂല്യം(s, global->type, &cell->i64_മൂല്യം));
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result SaveTables (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->അക്കം_Tables));

    വേണ്ടി (u32 t = 0; t < module->അക്കം_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_FirstIndex(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        എങ്കിൽ (first != t) {
            തുടരുക;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->size));

        വേണ്ടി (u32 e = 0; e < table->size; ++e) {
_           (Put_Ref_മൂല്യം(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result SaveSegments (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->അക്കം_Data_Segments));

    വേണ്ടി (u32 i = 0; i < module->അക്കം_Data_Segments; ++i) {
_       (PutU8(s, module->dataSegments[i].dropped));
    }

_   (PutLEB_u32(s, module->അക്കം_Element_Segments));

    വേണ്ടി (u32 i = 0; i < module->അക്കം_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    തിരികെ result;
}

#  എങ്കിൽ d_m3HasExceptionHandling

സ്ഥിതം
M3Result SaveExceptions (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.എണ്ണം));

    വേണ്ടി (u32 i = 0; i < s->exceptions.എണ്ണം; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.ഇനങ്ങൾ[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->അക്കം_Args != Get_Func_Type_അക്കം_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->അക്കം_Args));

        വേണ്ടി (u32 a = 0; a < exn->അക്കം_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_മൂല്യം_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    തിരികെ result;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
തരനിർവചനം ഘടന M3Activation {
    IM3Function        function;
    സ്ഥിരം M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    സ്ഥിരം M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
സ്ഥിതം
സ്ഥിരം M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    സ്ഥിരം M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : ശൂന്യ;

    വേണ്ടി (u32 i = 0; not point and i < i_module->അക്കം_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        എങ്കിൽ (point) {
            *io_function = &i_module->functions[i];
        }
    }

    തിരികെ point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
സ്ഥിതം
M3Result NextActivation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3Result result = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, വലുപ്പം(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    വേണ്ടി (; i >= 0; --i) {
        സ്ഥിരം M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        എങ്കിൽ (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  എങ്കിൽ d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            നിർത്തുക;
        } അല്ലെങ്കിൽ എങ്കിൽ (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            നിർത്തുക;
        }
    }

    എങ്കിൽ (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  എങ്കിൽ d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  എങ്കിൽ d_m3EntryKeepsFrame
    വേണ്ടി (i32 k = i_first; k > o_act->lastFrame; --k) {
        സ്ഥിരം M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    തിരികെ result;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
സ്ഥിതം
M3Result SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_അക്കം_Slots, സ്ഥിരം M3Activation* i_act)
{
    M3Result             result        = m3Err_none;
    IM3Function          function      = i_act->function;
    സ്ഥിരം M3SnapshotMap* map           = function->snapshotMap;
    u32                  functionIndex = FunctionIndex(s->module, function);
    സ്ഥിരം M3SafePoint*   point         = i_act->point;
    u32                  അക്കം_മൂല്യങ്ങൾ;

    _throwif("a suspended frame belongs to another module", functionIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, functionIndex));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    എങ്കിൽ (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    അക്കം_മൂല്യങ്ങൾ = map->അക്കം_Locals + point->അക്കം_മൂല്യങ്ങൾ;

_   (PutLEB_u32(s, അക്കം_മൂല്യങ്ങൾ));

    വേണ്ടി (u32 v = 0; v < അക്കം_മൂല്യങ്ങൾ; ++v) {
        സ്ഥിരം M3_Slot_മൂല്യം* മൂല്യം = Frame_മൂല്യം(map, point, v);
        u8*                where;
        u64                word;
        ശൂന്യം*              reference;

_       (Locate_മൂല്യം(i_base, i_അക്കം_Slots, i_act->sp, മൂല്യം, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        എങ്കിൽ (BaseTypeOf(മൂല്യം->type) == c_m3Type_v128) {
_           (Put_മൂല്യം_Tag(s, മൂല്യം->type));
_           (Put(s, where, 16));
            തുടരുക;
        }

_       (Read_മൂല്യം(where, &i_act->registers, മൂല്യം, &word, &reference));

_       (Put_മൂല്യം_Bits(s, മൂല്യം->type, IsRefType(മൂല്യം->type) ? (u64)(uintptr_t)reference : word));
    }

    എങ്കിൽ (point->kind == safepoint_resume) {
_       (Put_Ref_മൂല്യം(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    തിരികെ result;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
സ്ഥിതം
M3Result SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3Result      result   = m3Err_none;
    M3Activation* acts     = ശൂന്യ;
    u32           അക്കം_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->അക്കം_Frames - 1;
    m3slot_t*     base;
    u32           അക്കം_Slots;
    m3stack_t     sp;

_   (Continuation_അട്ടി(s->runtime, i_cont, &base, &അക്കം_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->അക്കം_Frames + 1);
    _throwifnull(acts);

    sp = base;

    ആകുമ്പോൾ (true) {
        M3Activation* act = &acts[അക്കം_Acts];

_       (NextActivation(s->module, i_cont, function, sp, first, act));

        എങ്കിൽ (act->kind != safepoint_call) {
            അക്കം_Acts++;
            നിർത്തുക;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        എങ്കിൽ (not (act->point->flags & d_m3SafePointTailCall)) {
            അക്കം_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, അക്കം_Acts));

    വേണ്ടി (u32 a = 0; a < അക്കം_Acts; ++a) {
_       (SaveActivation(s, base, അക്കം_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    തിരികെ result;
}

സ്ഥിതം
M3Result SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3Result  result    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_വേര്    = (i_cont == s->runtime->വേര്_Continuation);
    u8        state     = snapshot_contFinished;
    u32       typeIndex = is_വേര് ? d_m3SnapshotNone : FuncTypeIndex(module, i_cont->type);

    എങ്കിൽ (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } അല്ലെങ്കിൽ എങ്കിൽ (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", FunctionIndex(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_വേര് and typeIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_വേര്));
_   (PutLEB_u32(s, typeIndex));
_   (PutLEB_u32(s, FunctionIndex(module, i_cont->entryFunction)));
#  എങ്കിൽ d_m3HasExceptionHandling
_   (Put_Ref_മൂല്യം(s, c_m3Type_exnref, i_cont->resumeThrow));
#  അല്ലെങ്കിൽ
_   (Put_Ref_മൂല്യം(s, c_m3Type_exnref, ശൂന്യ));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    എങ്കിൽ (s->postmortem) {
        പോകുക _catch;
    }

    എങ്കിൽ (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : ശൂന്യ;
        u16         അക്കം_Rets = inner ? Get_Func_Type_അക്കം_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_എണ്ണം > Get_Func_Type_അക്കം_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_എണ്ണം));

        വേണ്ടി (u32 i = 0; i < i_cont->bound_Args_എണ്ണം; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_മൂല്യം(s, type, i_cont->val_അട്ടി + (അക്കം_Rets + i) * c_io_Slot_എണ്ണം));
        }
    } അല്ലെങ്കിൽ എങ്കിൽ (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_എണ്ണം));
_       (SaveSuspendedBody(s, i_cont));
    } അല്ലെങ്കിൽ {
_       (PutLEB_u32(s, 0));
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result SaveContinuations (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->continuations.എണ്ണം));

    വേണ്ടി (u32 i = 0; i < s->continuations.എണ്ണം; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.ഇനങ്ങൾ[i]));
    }

_catch:
    തിരികെ result;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
സ്ഥിതം
M3Result SaveHostState (M3SnapshotSave* s)
{
    M3Result               result = m3Err_none;
    സ്ഥിരം M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3BufferWriter         bw     = { ശൂന്യ, 0, 0 };

    എങ്കിൽ (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, BufferWriter_Write, &bw));
    }

    എങ്കിൽ (bw.size > 0) {
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    തിരികെ result;
}

സ്ഥിതം
ശൂന്യം SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    വേണ്ടി (u32 i = 0; i < module->അക്കം_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        എങ്കിൽ (Memory_FirstIndex(module, i) == i and memory) {
            *memoryBytes += memory->അക്കം_Pages * (u64)Memory_PageSize(memory);
        }
    }
    വേണ്ടി (u32 i = 0; i < module->അക്കം_Tables; ++i) {
        IM3Table table = module->tables[i];
        എങ്കിൽ (Table_FirstIndex(module, i) == i and table) {
            *tableElements += table->size;
        }
    }
}

സ്ഥിതം
M3Result SaveMeta (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    വേണ്ടി (u32 i = 0; i < s->continuations.എണ്ണം; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.ഇനങ്ങൾ[i];
        എങ്കിൽ (cont != s->runtime->വേര്_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.എണ്ണം));
_   (PutLEB_u32(s, s->exceptions.എണ്ണം));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  എങ്കിൽ d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    വേണ്ടി (u32 i = 0; i < s->exceptions.എണ്ണം; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.ഇനങ്ങൾ[i];
        u32          tagIndex  = TagIndex(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tagIndex == d_m3SnapshotNone);

_       (PutLEB_u32(s, tagIndex));
    }
#  endif

_catch:
    തിരികെ result;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
സ്ഥിതം
M3Result DiscoverReferences (M3SnapshotSave* s)
{
    M3Result result            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    ആകുമ്പോൾ (doneContinuations < s->continuations.എണ്ണം or doneExceptions < s->exceptions.എണ്ണം) {
#  എങ്കിൽ d_m3HasExceptionHandling
        ആകുമ്പോൾ (doneExceptions < s->exceptions.എണ്ണം) {
            M3Exception* exception = (M3Exception*)s->exceptions.ഇനങ്ങൾ[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->അക്കം_Args != Get_Func_Type_അക്കം_Params(type));

            വേണ്ടി (u32 a = 0; a < exception->അക്കം_Args; ++a) {
_               (Put_മൂല്യം_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        ആകുമ്പോൾ (doneContinuations < s->continuations.എണ്ണം and doneExceptions == s->exceptions.എണ്ണം) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.ഇനങ്ങൾ[id]));
        }
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result SaveSections (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (not s->writing) {
        തിരികെ DiscoverReferences(s);
    }

    എങ്കിൽ (not s->embedded) {
_       (Put(s, c_snapshotMagic, വലുപ്പം(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    എങ്കിൽ (s->module->അക്കം_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    എങ്കിൽ (s->module->അക്കം_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    എങ്കിൽ (s->module->അക്കം_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    എങ്കിൽ (s->module->അക്കം_Data_Segments or s->module->അക്കം_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  എങ്കിൽ d_m3HasExceptionHandling
    എങ്കിൽ (s->exceptions.എണ്ണം) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    എങ്കിൽ (s->continuations.എണ്ണം) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    തിരികെ result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

തരനിർവചനം ഘടന M3SnapshotSuspendSite {
    സ്ഥിരം M3SnapshotMap* map;
    സ്ഥിരം M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

തരനിർവചനം ഘടന M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    ശൂന്യം*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_അട്ടി;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    അക്കം_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  എങ്കിൽ d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 അക്കം_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

സ്ഥിതം
M3Result Get (M3SnapshotLoad* l, ശൂന്യം* o_data, size_t i_size)
{
    എങ്കിൽ (not l->inSection) {
        തിരികെ l->reader(o_data, i_size, l->userdata);
    }
    എങ്കിൽ (i_size > (size_t)(l->end - l->pos)) {
        തിരികെ m3Err_wasmUnderrun;
    }
    എങ്കിൽ (i_size) {
        memcpy(o_data, l->pos, i_size);
        l->pos += i_size;
    }
    തിരികെ m3Err_none;
}

സ്ഥിതം
M3Result GetU8 (M3SnapshotLoad* l, u8* o_മൂല്യം)
{
    തിരികെ Get(l, o_മൂല്യം, വലുപ്പം(*o_മൂല്യം));
}

സ്ഥിതം
M3Result GetBool (M3SnapshotLoad* l, u8* o_മൂല്യം)
{
    M3Result result = GetU8(l, o_മൂല്യം);
    തിരികെ result ? result : (*o_മൂല്യം <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

സ്ഥിതം
M3Result GetLEB_u32 (M3SnapshotLoad* l, u32* o_മൂല്യം)
{
    എങ്കിൽ (l->inSection) {
        തിരികെ ReadLEB_u32(o_മൂല്യം, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    വേണ്ടി (u32 i = 0; i < വലുപ്പം(encoded); ++i) {
        M3Result result = GetU8(l, &encoded[i]);
        എങ്കിൽ (result) {
            തിരികെ result;
        }
        എങ്കിൽ (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            തിരികെ ReadLEB_u32(o_മൂല്യം, &pos, encoded + i + 1);
        }
    }
    തിരികെ m3Err_lebOverflow;
}

സ്ഥിതം
M3Result GetLEB_u64 (M3SnapshotLoad* l, u64* o_മൂല്യം)
{
    തിരികെ ReadLebUnsigned(o_മൂല്യം, 64, &l->pos, l->end);
}

സ്ഥിതം
M3Result GetLEB_i64 (M3SnapshotLoad* l, i64* o_മൂല്യം)
{
    തിരികെ ReadLEB_i64(o_മൂല്യം, &l->pos, l->end);
}

സ്ഥിതം
M3Result SkipBytes (M3SnapshotLoad* l, size_t i_size)
{
    എങ്കിൽ (i_size > (size_t)(l->end - l->pos)) {
        തിരികെ m3Err_wasmUnderrun;
    }
    l->pos += i_size;
    തിരികെ m3Err_none;
}


സ്ഥിതം
M3Result Get_Ref_മൂല്യം (M3SnapshotLoad* l, m3type_t i_type, ശൂന്യം** o_reference)
{
    M3Result result = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = ശൂന്യ;

_   (GetU8(l, &kind));
    എങ്കിൽ (kind == 0x00) {
        തിരികെ (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    എങ്കിൽ (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->അക്കം_Functions);
#  എങ്കിൽ d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } അല്ലെങ്കിൽ എങ്കിൽ (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        സ്ഥിരം M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        എങ്കിൽ (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  എങ്കിൽ d_m3HasExceptionHandling
    അല്ലെങ്കിൽ എങ്കിൽ (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->അക്കം_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    അല്ലെങ്കിൽ എങ്കിൽ (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->അക്കം_Continuations);
        *o_reference = l->continuations[id];
#  എങ്കിൽ d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        എങ്കിൽ (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } അല്ലെങ്കിൽ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result GetLE32 (M3SnapshotLoad* l, u32* o_മൂല്യം)
{
    u8       bytes[4];
    M3Result result = Get(l, bytes, വലുപ്പം(bytes));

    *o_മൂല്യം = 0;

    വേണ്ടി (u32 i = 0; i < വലുപ്പം(bytes); ++i) {
        *o_മൂല്യം |= (u32)bytes[i] << (8 * i);
    }

    തിരികെ result;
}

സ്ഥിതം
M3Result GetLE64 (M3SnapshotLoad* l, u64* o_മൂല്യം)
{
    u8       bytes[8];
    M3Result result = Get(l, bytes, വലുപ്പം(bytes));

    *o_മൂല്യം = 0;

    വേണ്ടി (u32 i = 0; i < വലുപ്പം(bytes); ++i) {
        *o_മൂല്യം |= (u64)bytes[i] << (8 * i);
    }

    തിരികെ result;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
സ്ഥിതം
M3Result Get_മൂല്യം_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3Result result  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    എങ്കിൽ (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    തിരികെ result;
}

// The mirror of PutValueBody: narrow types come back in the low bits
സ്ഥിതം
M3Result Get_മൂല്യം_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3Result result = m3Err_none;

    *o_bits = 0;

    എങ്കിൽ (IsRefType(i_type)) {
        ശൂന്യം* reference = ശൂന്യ;
_       (Get_Ref_മൂല്യം(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } അല്ലെങ്കിൽ എങ്കിൽ (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 മൂല്യം = 0;
_       (ReadLEB_i32(&മൂല്യം, &l->pos, l->end));
        *o_bits = (u32)മൂല്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 മൂല്യം = 0;
_       (GetLEB_i64(l, &മൂല്യം));
        *o_bits = (u64)മൂല്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } അല്ലെങ്കിൽ എങ്കിൽ (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } അല്ലെങ്കിൽ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result Get_മൂല്യം_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3Result result = m3Err_none;
    m3type_t type;

_   (Get_മൂല്യം_Tag(l, i_expectedType, &type));
_   (Get_മൂല്യം_Body(l, type, o_bits));

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result Get_മൂല്യം (M3SnapshotLoad* l, m3type_t i_expectedType, ശൂന്യം* o_where)
{
    M3Result result = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    എങ്കിൽ (l->checking) {
        o_where = scratch;
    }

_   (Get_മൂല്യം_Tag(l, i_expectedType, &type));

    എങ്കിൽ (IsRefType(type)) {
        ശൂന്യം* reference = ശൂന്യ;
        u64   bits      = 0;
_       (Get_മൂല്യം_Body(l, type, &bits));
        reference = (ശൂന്യം*)(uintptr_t)bits;
        memcpy(o_where, &reference, വലുപ്പം(reference));
    } അല്ലെങ്കിൽ എങ്കിൽ (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } അല്ലെങ്കിൽ {
        u64 bits = 0;
_       (Get_മൂല്യം_Body(l, type, &bits));

        എങ്കിൽ (Is64BitType(type)) {
            memcpy(o_where, &bits, വലുപ്പം(bits));
        } അല്ലെങ്കിൽ {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, വലുപ്പം(narrow));
        }
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3Result result = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->അക്കം_Functions);
    *o_function = &l->module->functions[index];

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_size)
{
    M3Result result = m3Err_none;

    വേണ്ടി (;;) {
        u8  chunkType = 0;
        u32 offset = 0, length = 0;

_       (GetU8(l, &chunkType));

        എങ്കിൽ (chunkType == d_m3ChunkEnd) {
            നിർത്തുക;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &length));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_size or length > i_size - (size_t)offset);

        എങ്കിൽ (chunkType == d_m3ChunkRaw) {
            എങ്കിൽ (l->checking or length == 0) {
_               (SkipBytes(l, length));
            } അല്ലെങ്കിൽ {
_               (Get(l, o_bytes + offset, length));
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (chunkType == d_m3ChunkFillFF) {
            എങ്കിൽ (not l->checking and length) {
                memset(o_bytes + offset, 0xFF, length);
            }
        } അല്ലെങ്കിൽ {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadMemories (M3SnapshotLoad* l)
{
    M3Result  result      = m3Err_none;
    IM3Module module      = l->module;
    u32       അക്കം_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &അക്കം_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, അക്കം_Memories == 0 or അക്കം_Memories != module->അക്കം_Memories);

    വേണ്ടി (u32 m = 0; m < അക്കം_Memories; ++m) {
        u32       memIndex = 0, firstIndex = 0, pageBits = 0, അക്കം_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &memIndex));
_       (GetLEB_u32(l, &firstIndex));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, memIndex != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 firstIndex != Memory_FirstIndex(module, m));
        എങ്കിൽ (firstIndex != m) {
            തുടരുക;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &അക്കം_Pages));

        memory = module->memories[memIndex];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    data  = ശൂന്യ;

        എങ്കിൽ (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_PageSize(memory));
            _throwif(m3Err_wasmMalformed, അക്കം_Pages < memory->അക്കം_Pages or അക്കം_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, അക്കം_Pages > SIZE_MAX / Memory_PageSize(memory));
            bytes = (size_t)അക്കം_Pages * Memory_PageSize(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            എങ്കിൽ (not l->checking) {
                എങ്കിൽ (memory->അക്കം_Pages < അക്കം_Pages) {
_                   (ResizeMemory(l->runtime, memory, അക്കം_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->length < bytes);
            }

            l->loadedMemoryBytes += bytes;

            എങ്കിൽ (bytes and memory->mallocated) {
                data = m3MemData(memory->mallocated);
            }
        } അല്ലെങ്കിൽ {
            _throwif(m3Err_wasmMalformed, അക്കം_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        എങ്കിൽ (data and not l->checking) {
            memset(data, 0, bytes);
        }

_       (LoadMemoryChunks(l, data, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadGlobals (M3SnapshotLoad* l)
{
    M3Result  result     = m3Err_none;
    IM3Module module     = l->module;
    u32       അക്കം_Globals = 0;

_   (GetLEB_u32(l, &അക്കം_Globals));
    _throwif(m3Err_wasmMalformed, അക്കം_Globals == 0 or അക്കം_Globals != module->അക്കം_Globals);

    വേണ്ടി (u32 g = 0; g < അക്കം_Globals; ++g) {
        u32       globalIndex = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &globalIndex));
        _throwif(m3Err_wasmMalformed, globalIndex != g);

        global = &module->globals[globalIndex];
        cell   = global->resolved ? global->resolved : global;

_       (Get_മൂല്യം(l, global->type, &cell->i64_മൂല്യം));
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadTables (M3SnapshotLoad* l)
{
    M3Result  result       = m3Err_none;
    IM3Module module       = l->module;
    u32       അക്കം_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &അക്കം_Tables));
    _throwif(m3Err_wasmMalformed, അക്കം_Tables == 0 or അക്കം_Tables != module->അക്കം_Tables);

    വേണ്ടി (u32 t = 0; t < അക്കം_Tables; ++t) {
        u32      tableIndex = 0, firstIndex = 0, size = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &tableIndex));
_       (GetLEB_u32(l, &firstIndex));

        _throwif(m3Err_wasmMalformed, tableIndex != t);
        _throwif("the snapshot shares tables between imports differently",
                 firstIndex != Table_FirstIndex(module, t));
        എങ്കിൽ (firstIndex != t) {
            തുടരുക;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &size));

        table = module->tables[tableIndex];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, size > (table->hasMax ? table->maxSize : d_m3MaxSaneTableSize));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, size < table->size);

        l->loadedTableElements += size;

        എങ്കിൽ (not l->checking and size != table->size) {
            ശൂന്യം** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 size - table->size > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(ശൂന്യം*, table->elements, size, table->size);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += size - table->size;
            table->elements = elements;
            table->size     = size;
        }

        വേണ്ടി (u32 e = 0; e < size; ++e) {
            ശൂന്യം* reference = ശൂന്യ;
_           (Get_Ref_മൂല്യം(l, table->type, &reference));
            എങ്കിൽ (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadSegments (M3SnapshotLoad* l)
{
    M3Result  result = m3Err_none;
    IM3Module module = l->module;
    u32       എണ്ണം  = 0;

_   (GetLEB_u32(l, &എണ്ണം));
    _throwif(m3Err_wasmMalformed, എണ്ണം != module->അക്കം_Data_Segments);
    _throwif(m3Err_wasmMalformed, module->അക്കം_Data_Segments == 0 and module->അക്കം_Element_Segments == 0);

    വേണ്ടി (u32 i = 0; i < എണ്ണം; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        എങ്കിൽ (not l->checking) {
            module->dataSegments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &എണ്ണം));
    _throwif(m3Err_wasmMalformed, എണ്ണം != module->അക്കം_Element_Segments);

    വേണ്ടി (u32 i = 0; i < എണ്ണം; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        എങ്കിൽ (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    തിരികെ result;
}

// A frame for the continuation being put together, outermost first
സ്ഥിതം
M3Result AddFrame (M3SnapshotLoad* l, u32* io_അക്കം_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    എങ്കിൽ (*io_അക്കം_Frames >= d_m3ContinuationMaxFrames) {
        തിരികെ "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_അക്കം_Frames)++];

    memset(frame, 0, വലുപ്പം(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    തിരികെ m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
സ്ഥിതം
M3Result PrepareFunction (IM3Function i_function)
{
    M3Result result = m3Err_none;

    എങ്കിൽ (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    തിരികെ result;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
സ്ഥിതം
M3Result AddBlockFrames (M3SnapshotLoad* l, സ്ഥിരം M3SnapshotMap* i_map, സ്ഥിരം M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_അക്കം_Frames)
{
    M3Result result = m3Err_none;

    വേണ്ടി (u32 b = 0; b < i_map->അക്കം_Blocks; ++b) {
        സ്ഥിരം M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        എങ്കിൽ (not EnclosesOffset(block, i_point->wasmOffset)) {
            തുടരുക;
        }

        എങ്കിൽ (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_അക്കം_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  എങ്കിൽ d_m3HasExceptionHandling
        അല്ലെങ്കിൽ {
_           (AddFrame(l, io_അക്കം_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.അക്കം_Clauses   = block->അക്കം_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    തിരികെ result;
}

// Puts a function's values where this build keeps them at the safepoint
സ്ഥിതം
M3Result Load_മൂല്യങ്ങൾ (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_അക്കം_Slots, m3stack_t i_sp,
                     സ്ഥിരം M3SnapshotMap* i_map, സ്ഥിരം M3SafePoint* i_point, സ്ഥിരം M3FrameRegisters* i_registers)
{
    M3Result result    = m3Err_none;
    u32      അക്കം_മൂല്യങ്ങൾ = 0;

_   (GetLEB_u32(l, &അക്കം_മൂല്യങ്ങൾ));
    _throwif("the snapshot's frame does not match what this build compiled", അക്കം_മൂല്യങ്ങൾ != i_map->അക്കം_Locals + i_point->അക്കം_മൂല്യങ്ങൾ);

    വേണ്ടി (u32 v = 0; v < അക്കം_മൂല്യങ്ങൾ; ++v) {
        സ്ഥിരം M3_Slot_മൂല്യം* മൂല്യം     = Frame_മൂല്യം(i_map, i_point, v);
        u64                word      = 0;
        ശൂന്യം*              reference = ശൂന്യ;
        u8*                where;

_       (Locate_മൂല്യം(i_base, i_അക്കം_Slots, i_sp, മൂല്യം, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        എങ്കിൽ (BaseTypeOf(മൂല്യം->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_മൂല്യം_Tag(l, മൂല്യം->type, &type));
            എങ്കിൽ (l->checking) {
_               (SkipBytes(l, 16));
            } അല്ലെങ്കിൽ {
_               (Get(l, where, 16));
            }
            തുടരുക;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= അക്കം_മൂല്യങ്ങൾ - i_point->അക്കം_Results;
_       (Get_മൂല്യം_Bits(l, മൂല്യം->type, &word));
        l->allowUnfilledRef = false;

        എങ്കിൽ (IsRefType(മൂല്യം->type)) {
            reference = (ശൂന്യം*)(uintptr_t)word;
        }

        എങ്കിൽ (not l->checking) {
_           (Write_മൂല്യം(where, i_registers, മൂല്യം, word, reference));
        }
    }

_catch:
    തിരികെ result;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
സ്ഥിതം
M3Result LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3Result    result    = m3Err_none;
    u32         അക്കം_Frames = 0;
    M3Frame*    callFrame = ശൂന്യ;
    IM3Function function  = ശൂന്യ;
    m3slot_t*   base;
    u32         അക്കം_Slots;
    u32         അക്കം_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_അട്ടി(l->runtime, io_cont, &base, &അക്കം_Slots));

_   (GetLEB_u32(l, &അക്കം_Activations));
    _throwif(m3Err_wasmMalformed, അക്കം_Activations == 0 or അക്കം_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = ശൂന്യ;
    io_cont->r0                = 0;
    io_cont->അക്കം_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  എങ്കിൽ d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    വേണ്ടി (u32 a = 0; a < അക്കം_Activations; ++a) {
        bool                 isInnermost = (a + 1 == അക്കം_Activations);
        സ്ഥിരം M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        സ്ഥിരം M3SnapshotMap* map;

        memset(&registers, 0, വലുപ്പം(registers));

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
                 (u32)(sp - base) + function->max_അട്ടി_Slots > അക്കം_Slots);

        // the call the frame outside this one is waiting on
        എങ്കിൽ (callFrame) {
            callFrame->call.function = function;
        }

#  എങ്കിൽ d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &അക്കം_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        എങ്കിൽ (kind == safepoint_op) {
            സ്ഥിരം M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &അക്കം_Frames));

        എങ്കിൽ (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &അക്കം_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  എങ്കിൽ d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } അല്ലെങ്കിൽ എങ്കിൽ (kind == safepoint_resume) {
            M3Frame* frame;
            u32      അക്കം_Results = point->അക്കം_Results;
            pc_t     resultsPC  = point->pc - 2 * അക്കം_Results;

_           (AddFrame(l, &അക്കം_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.അക്കം_Results  = അക്കം_Results;
            frame->resume.അക്കം_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } അല്ലെങ്കിൽ {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  എങ്കിൽ d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_മൂല്യങ്ങൾ(l, base, അക്കം_Slots, sp, map, point, &registers));

        എങ്കിൽ (not l->checking) {
            RestoreConstants(function, sp);
        }

        എങ്കിൽ (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } അല്ലെങ്കിൽ എങ്കിൽ (kind == safepoint_resume) {
            ശൂന്യം* cont = ശൂന്യ;
_           (Get_Ref_മൂല്യം(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ശൂന്യ);

            callFrame->resume.cont = (IM3Continuation)cont;
            എങ്കിൽ (not l->checking) {
                l->suspendSites[i_id].point = point;
                വേണ്ടി (u32 i = 1; i < l->അക്കം_Continuations; ++i) {
                    എങ്കിൽ (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        നിർത്തുക;
                    }
                }
            }
        } അല്ലെങ്കിൽ {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            എങ്കിൽ (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            എങ്കിൽ (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->അക്കം_Results > point->അക്കം_മൂല്യങ്ങൾ or
                                                point->അക്കം_Results > d_m3MaxContinuationPayload);

                io_cont->അക്കം_Suspend_Results = point->അക്കം_Results;
                എങ്കിൽ (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                വേണ്ടി (u32 i = 0; i < point->അക്കം_Results; ++i) {
                    സ്ഥിരം M3_Slot_മൂല്യം* മൂല്യം = &map->മൂല്യങ്ങൾ[point->first_മൂല്യം + point->അക്കം_മൂല്യങ്ങൾ - point->അക്കം_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(മൂല്യം->slot));

                    io_cont->suspendResultOffsets[i] = മൂല്യം->slot;
                    io_cont->suspendResultIs64[i]    = Is64BitType(മൂല്യം->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    എങ്കിൽ (l->checking) {
        തിരികെ m3Err_none;
    }

    // held innermost first
    എങ്കിൽ (അക്കം_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, അക്കം_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = അക്കം_Frames;
    }

    വേണ്ടി (u32 k = 0; k < അക്കം_Frames; ++k) {
        io_cont->frames[അക്കം_Frames - 1 - k] = l->frames[k];
    }

    io_cont->അക്കം_Frames = അക്കം_Frames;

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3Result        result = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    എങ്കിൽ (l->checking) {
        scratch          = *cont;
        scratch.val_അട്ടി = l->check_അട്ടി;
        എങ്കിൽ (i_id == 0) {
            scratch.val_അട്ടി = (m3slot_t*)l->runtime->origin_അട്ടി;
        }
        cont = &scratch;
    }

    u32 contId = 0, typeIndex = 0, entryIndex = 0, bound_Args_എണ്ണം = 0;
    u8  state = 0, is_വേര് = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_വേര്));
_   (GetLEB_u32(l, &typeIndex));
_   (GetLEB_u32(l, &entryIndex));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_വേര് != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_വേര് and state != snapshot_contSuspended);
    എങ്കിൽ (not is_വേര് and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, typeIndex != d_m3SnapshotNone and typeIndex >= module->അക്കം_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entryIndex >= module->അക്കം_Functions);
    _throwif(m3Err_wasmMalformed, is_വേര് ? typeIndex != d_m3SnapshotNone : (typeIndex == d_m3SnapshotNone or not module->funcTypes[typeIndex]->isContinuation));

    എങ്കിൽ (not l->checking and not is_വേര് and state != snapshot_contFinished) {
_       (Continuation_Acquire_അട്ടി(l->runtime, cont));
    }
    cont->type          = (typeIndex != d_m3SnapshotNone) ? module->funcTypes[typeIndex] : ശൂന്യ;
    cont->entryFunction = &module->functions[entryIndex];
    cont->അക്കം_Handlers   = 0;
    cont->handlersPC    = ശൂന്യ;
    cont->parent        = ശൂന്യ;
    cont->അക്കം_Frames     = 0;

#  എങ്കിൽ d_m3HasExceptionHandling
    {
        ശൂന്യം* ref = ശൂന്യ;
_       (Get_Ref_മൂല്യം(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_വേര് or state == snapshot_contFinished) and cont->resumeThrow != ശൂന്യ);
#  അല്ലെങ്കിൽ
    {
        ശൂന്യം* ref = ശൂന്യ;
_       (Get_Ref_മൂല്യം(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ശൂന്യ);
    }
#  endif

    എങ്കിൽ (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         അക്കം_Rets;

_       (GetLEB_u32(l, &bound_Args_എണ്ണം));
        cont->bound_Args_എണ്ണം = bound_Args_എണ്ണം;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_എണ്ണം > Get_Func_Type_അക്കം_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_അക്കം_Params(remaining) != Get_Func_Type_അക്കം_Params(inner) - bound_Args_എണ്ണം or
                                            Get_Func_Type_അക്കം_Results(remaining) != Get_Func_Type_അക്കം_Results(inner));
            വേണ്ടി (u16 i = 0; i < Get_Func_Type_അക്കം_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(inner, i) != GetFuncTypeResultType(remaining, i));
            }
            വേണ്ടി (u16 i = 0; i < Get_Func_Type_അക്കം_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_എണ്ണം + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        അക്കം_Rets = Get_Func_Type_അക്കം_Results(inner);

        _throwif(m3Err_wasmMalformed, (അക്കം_Rets + bound_Args_എണ്ണം) * c_io_Slot_എണ്ണം > cont->അക്കം_അട്ടി_Slots);

        വേണ്ടി (u32 i = 0; i < bound_Args_എണ്ണം; ++i) {
_           (Get_മൂല്യം(l, GetFuncTypeParamType(inner, (u16)i), cont->val_അട്ടി + (അക്കം_Rets + i) * c_io_Slot_എണ്ണം));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_അട്ടി;
        cont->pc    = cont->entryFunction->compiled;
    } അല്ലെങ്കിൽ എങ്കിൽ (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_എണ്ണം));
        _throwif(m3Err_wasmMalformed, is_വേര് and bound_Args_എണ്ണം != 0);
        cont->bound_Args_എണ്ണം = bound_Args_എണ്ണം;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } അല്ലെങ്കിൽ എങ്കിൽ (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_എണ്ണം));
        _throwif(m3Err_wasmMalformed, bound_Args_എണ്ണം != 0);
        cont->bound_Args_എണ്ണം = 0;
        cont->state          = cont_consumed;
    } അല്ലെങ്കിൽ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    എങ്കിൽ (result) {
        cont->അക്കം_Frames = 0;
    }

    തിരികെ result;
}

സ്ഥിതം
M3Result LoadContinuations (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u32      എണ്ണം  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &എണ്ണം));
    _throwif(m3Err_wasmMalformed, എണ്ണം == 0 or എണ്ണം != l->അക്കം_Continuations);

    വേണ്ടി (u32 i = 0; i < എണ്ണം; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    തിരികെ result;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
സ്ഥിതം
M3Result CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;

    വേണ്ടി (u32 id = 0; id < l->അക്കം_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  എങ്കിൽ d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        എങ്കിൽ (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : ശൂന്യ;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_അക്കം_Results(actual) != Get_Func_Type_അക്കം_Results(expected));
            വേണ്ടി (u16 i = 0; i < Get_Func_Type_അക്കം_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(actual, i) != GetFuncTypeResultType(expected, i));
            }
        }
        ആകുമ്പോൾ (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->അക്കം_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        എങ്കിൽ (cont->state != cont_suspended or id == 0) {
            തുടരുക;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        എങ്കിൽ (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_എണ്ണം != 0);
            തുടരുക;
        }

        സ്ഥിരം M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_എണ്ണം > inner->അക്കം_Suspend_Results or
                                                                          Get_Func_Type_അക്കം_Params(remaining) != inner->അക്കം_Suspend_Results - cont->bound_Args_എണ്ണം);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_അക്കം_Results(remaining) != Get_Func_Type_അക്കം_Results(cont->entryFunction->funcType));
        വേണ്ടി (u16 i = 0; i < Get_Func_Type_അക്കം_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(remaining, i) !=
                                            GetFuncTypeResultType(cont->entryFunction->funcType, i));
        }

        വേണ്ടി (u32 i = 0; i < inner->അക്കം_Suspend_Results; ++i) {
            സ്ഥിരം M3_Slot_മൂല്യം* മൂല്യം = &site->map->മൂല്യങ്ങൾ[site->point->first_മൂല്യം +
                                                          site->point->അക്കം_മൂല്യങ്ങൾ - site->point->അക്കം_Results + i];
            എങ്കിൽ (i < cont->bound_Args_എണ്ണം) {
                എങ്കിൽ (IsRefType(മൂല്യം->type) and not IsNullableRef(മൂല്യം->type)) {
                    ശൂന്യം* ref = ശൂന്യ;
                    memcpy(&ref, inner->sp + മൂല്യം->slot, വലുപ്പം(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } അല്ലെങ്കിൽ {
                _throwif("a captured continuation has the wrong parameter type", മൂല്യം->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_എണ്ണം)));
            }
        }
    }

_catch:
    തിരികെ result;
}

#  എങ്കിൽ d_m3HasExceptionHandling

സ്ഥിതം
M3Result LoadExceptions (M3SnapshotLoad* l)
{
    M3Result result        = m3Err_none;
    u32      അക്കം_Exceptions = 0;

_   (GetLEB_u32(l, &അക്കം_Exceptions));
    _throwif(m3Err_wasmMalformed, അക്കം_Exceptions == 0 or അക്കം_Exceptions != l->അക്കം_Exceptions);

    വേണ്ടി (u32 i = 0; i < അക്കം_Exceptions; ++i) {
        u32          exnId = 0, അക്കം_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &അക്കം_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, അക്കം_Args != exn->അക്കം_Args);

        വേണ്ടി (u32 a = 0; a < അക്കം_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_മൂല്യം_Bits(l, argType, &bits));
            എങ്കിൽ (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    തിരികെ result;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
തരനിർവചനം ഘടന M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3Result        error;
} M3BoundedReader;

സ്ഥിതം
M3Result BoundedReader_Read (ശൂന്യം* o_buffer, size_t i_size, ശൂന്യം* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    എങ്കിൽ (i_size > br->remaining) {
        br->error = "the host read past the end of its state";
        തിരികെ br->error;
    }

    br->remaining -= i_size;

    തിരികെ Get(br->load, o_buffer, i_size);
}

സ്ഥിതം
M3Result LoadHostState (M3SnapshotLoad* l)
{
    M3Result               result = m3Err_none;
    സ്ഥിരം M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 size = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    എങ്കിൽ (l->checking) {
        തിരികെ SkipBytes(l, size);
    }
    reader.load      = l;
    reader.remaining = size;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, size));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadHeader (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u8       magic[വലുപ്പം(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, വലുപ്പം(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, വലുപ്പം(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    തിരികെ result;
}

സ്ഥിതം
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

    (ശൂന്യം)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->അക്കം_Continuations));
_   (GetLEB_u32(l, &l->അക്കം_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->അക്കം_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->അക്കം_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    എങ്കിൽ (*o_flags & d_m3SnapshotFlagPostmortem) {
        വേണ്ടി (u32 i = 0; i < l->അക്കം_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
        }
        തിരികെ m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->അക്കം_Continuations == 0 or activeStacks >= l->അക്കം_Continuations);
#  എങ്കിൽ !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->അക്കം_Exceptions != 0);
#  endif

    എങ്കിൽ (l->checking) {
        വേണ്ടി (u32 i = 0; i < l->അക്കം_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
#  എങ്കിൽ d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tagIndex >= l->module->അക്കം_Tags);
#  endif
        }
        തിരികെ m3Err_none;
    }

    l->check_അട്ടി = m3_AllocArray(m3slot_t, d_m3_Continuation_അട്ടി_Slots + 4);
    _throwifnull(l->check_അട്ടി);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->അക്കം_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->അക്കം_Continuations);
    _throwifnull(l->suspendSites);

    എങ്കിൽ (not l->runtime->വേര്_Continuation) {
        IM3Continuation വേര് = Continuation_New(l->runtime, ശൂന്യ, ശൂന്യ);
        _throwifnull(വേര്);

        // the root borrows the runtime's stack, so it holds none of its own
        വേര്->അക്കം_അട്ടി_Slots          = l->runtime->അക്കം_അട്ടി_Slots;
        l->runtime->വേര്_Continuation = വേര്;
    }

    l->continuations[0] = l->runtime->വേര്_Continuation;

    വേണ്ടി (u32 i = 1; i < l->അക്കം_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ശൂന്യ, ശൂന്യ);
        _throwifnull(l->continuations[i]);
    }

#  എങ്കിൽ d_m3HasExceptionHandling
    എങ്കിൽ (l->അക്കം_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->അക്കം_Exceptions);
        _throwifnull(l->exceptions);
    }

    വേണ്ടി (u32 i = 0; i < l->അക്കം_Exceptions; ++i) {
        u32    tagIndex = 0;
        IM3Tag tag;
        u32    അക്കം_Args;

_       (GetLEB_u32(l, &tagIndex));
        _throwif(m3Err_wasmMalformed, tagIndex >= l->module->അക്കം_Tags);

        tag     = TagOfIndex(l->module, tagIndex);
        അക്കം_Args = Get_Func_Type_അക്കം_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, അക്കം_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  അല്ലെങ്കിൽ
    വേണ്ടി (u32 i = 0; i < l->അക്കം_Exceptions; ++i) {
        u32 tagIndex = 0;
_       (GetLEB_u32(l, &tagIndex));
    }
#  endif

_catch:
    തിരികെ result;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
സ്ഥിതം
M3Result LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3Result result = m3Err_none;
    u32      flags  = 0;
    തിരഞ്ഞെടുപ്പ് (i_sectionId) {
    അവസ്ഥ d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        നിർത്തുക;

    അവസ്ഥ d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        നിർത്തുക;

    അവസ്ഥ d_m3SnapshotSection_Table:
_       (LoadTables(l));
        നിർത്തുക;

    അവസ്ഥ d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        നിർത്തുക;

    അവസ്ഥ d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        നിർത്തുക;

#  എങ്കിൽ d_m3HasExceptionHandling
    അവസ്ഥ d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        നിർത്തുക;
#  endif

    അവസ്ഥ d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        നിർത്തുക;

    അവസ്ഥ d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        നിർത്തുക;

    സ്ഥിരസ്ഥിതി:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        നിർത്തുക;
    }

_catch:
    തിരികെ result;
}

സ്ഥിതം
M3Result LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3Result result  = m3Err_none;
    u8*      payload = ശൂന്യ;

    എങ്കിൽ (not i_embedded) {
_       (LoadHeader(l));
    }

    ആകുമ്പോൾ (true) {
        u8  sectionId   = 0;
        u32 sectionSize = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        എങ്കിൽ (GetU8(l, &sectionId)) {
            നിർത്തുക;
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
             l->അക്കം_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->അക്കം_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->അക്കം_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->അക്കം_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->അക്കം_Data_Segments or l->module->അക്കം_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    തിരികെ result;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#എങ്കിൽ d_m3HasSnapshots

സ്ഥിതം
M3Result SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ശൂന്യം* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3Result       result = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, വലുപ്പം(save));

    IM3Continuation വേര് = io_runtime->വേര്_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not വേര് or വേര്->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : വേര്->entryFunction;

    എങ്കിൽ (o_module) {
        *o_module = ശൂന്യ;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    എങ്കിൽ (o_module) {
        *o_module = save.module;
    }

    വേണ്ടി (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->next) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  എങ്കിൽ d_m3HasExceptionHandling
    വേണ്ടി (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->next) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    എങ്കിൽ (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, വേര്, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    തിരികെ result;
}

#endif

M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ശൂന്യം* i_userdata)
{
    എങ്കിൽ (not io_runtime or not i_writer) {
        തിരികെ m3Err_mallocFailed;
    }

#എങ്കിൽ d_m3HasSnapshots
    തിരികെ SaveSnapshot(io_runtime, i_writer, i_userdata, ശൂന്യ, false);
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_userdata;
    തിരികെ "snapshots are not available in this build of Wasm3";
#endif
}


സ്ഥിതം
M3Result LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ശൂന്യം* i_userdata, bool i_embedded)
{
    M3Result result = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#എങ്കിൽ d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, വലുപ്പം(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        result = LoadSections(&load, i_embedded);

        IM3Continuation വേര് = io_runtime->വേര്_Continuation;

        എങ്കിൽ (result) {
            എങ്കിൽ (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                എങ്കിൽ (വേര്) {
                    വേര്->state     = cont_returned;
                    വേര്->അക്കം_Frames = 0;
                }
            }
        } അല്ലെങ്കിൽ {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_അട്ടി(io_runtime);

        m3_Free(load.check_അട്ടി);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  എങ്കിൽ d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    തിരികെ result;
}


M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ശൂന്യം* i_userdata)
{
    തിരികെ LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


ശൂന്യം m3_SetSnapshotHooks (IM3Runtime io_runtime, സ്ഥിരം M3SnapshotHooks* i_hooks)
{
#എങ്കിൽ d_m3HasSnapshots
    എങ്കിൽ (io_runtime) {
        എങ്കിൽ (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } അല്ലെങ്കിൽ {
            memset(&io_runtime->snapshotHooks, 0, വലുപ്പം(io_runtime->snapshotHooks));
        }
    }
#അല്ലെങ്കിൽ
    (ശൂന്യം)io_runtime;
    (ശൂന്യം)i_hooks;
#endif
}


M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, ശൂന്യം** o_bytes, size_t* o_size)
{
    എങ്കിൽ (!io_runtime || !o_bytes || !o_size) {
        തിരികെ m3Err_mallocFailed;
    }
#എങ്കിൽ d_m3HasSnapshots
    M3BufferWriter bw     = { ശൂന്യ, 0, 0 };
    M3Result       result = m3_SaveSnapshot(io_runtime, BufferWriter_Write, &bw);
    എങ്കിൽ (result) {
        m3_Free(bw.buffer);
        *o_bytes = ശൂന്യ;
        *o_size  = 0;
        തിരികെ result;
    }
    *o_bytes = bw.buffer;
    *o_size  = bw.size;
    തിരികെ m3Err_none;
#അല്ലെങ്കിൽ
    *o_bytes = ശൂന്യ;
    *o_size  = 0;
    തിരികെ "snapshots are not available in this build of Wasm3";
#endif
}

#എങ്കിൽ d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
സ്ഥിതം
u8* EmitLEB_u32 (u8* o_where, u32 i_മൂല്യം)
{
    ചെയ്യുക {
        u8 byte = i_മൂല്യം & 0x7F;
        i_മൂല്യം >>= 7;
        എങ്കിൽ (i_മൂല്യം != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } ആകുമ്പോൾ (i_മൂല്യം != 0);

    തിരികെ o_where;
}

സ്ഥിതം
u32 SizeOfLEB_u32 (u32 i_മൂല്യം)
{
    u32 size = 0;

    ചെയ്യുക {
        i_മൂല്യം >>= 7;
        ++size;
    } ആകുമ്പോൾ (i_മൂല്യം != 0);

    തിരികെ size;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
സ്ഥിതം
M3Result FindEmbeddedSnapshot (സ്ഥിരം u8* i_start, സ്ഥിരം u8* i_end, cstr_t i_secName,
                               സ്ഥിരം u8** o_secStart, സ്ഥിരം u8** o_secEnd,
                               സ്ഥിരം u8** o_data, u32* o_size)
{
    M3Result result = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = ശൂന്യ;

    എങ്കിൽ (o_secStart) {
        *o_secStart = ശൂന്യ;
    }
    എങ്കിൽ (o_secEnd) {
        *o_secEnd = ശൂന്യ;
    }
    എങ്കിൽ (o_data) {
        *o_data = ശൂന്യ;
    }
    എങ്കിൽ (o_size) {
        *o_size = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    ആകുമ്പോൾ (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     size      = 0;
_       (ReadLEB_u32(&size, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, size > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += size;
            എങ്കിൽ (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                എങ്കിൽ (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    എങ്കിൽ (o_secStart) {
                        *o_secStart = secStart;
                    }
                    എങ്കിൽ (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    എങ്കിൽ (o_data) {
                        *o_data = payload;
                    }
                    എങ്കിൽ (o_size) {
                        *o_size = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    തിരികെ result;
}

// Builds the custom section name a snapshot of i_name lives under
സ്ഥിതം
M3Result SnapshotSectionName (cstr_t i_name, അക്ഷരം* o_secName, size_t i_size)
{
    എങ്കിൽ (i_name and *i_name) {
        എങ്കിൽ (strlen(i_name) > d_m3MaxSnapshotNameLength) {
            തിരികെ "the snapshot name is too long";
        }
        snprintf(o_secName, i_size, "snapshot.%s", i_name);
    } അല്ലെങ്കിൽ {
        snprintf(o_secName, i_size, "snapshot");
    }

    തിരികെ m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
സ്ഥിതം
M3Result ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3Result result = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ശൂന്യ;

    വേണ്ടി (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->next) {
        എങ്കിൽ (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    തിരികെ result;
}
#endif

M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, സ്ഥിരം ശൂന്യം* i_bytes, size_t i_size)
{
    എങ്കിൽ (!io_runtime || !i_module || !i_bytes) {
        തിരികെ m3Err_mallocFailed;
    }
#എങ്കിൽ d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    എങ്കിൽ (i_size >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        സ്ഥിരം u8* data   = ശൂന്യ;
        u32       size   = 0;
        M3Result  result = FindEmbeddedSnapshot((സ്ഥിരം u8*)i_bytes, (സ്ഥിരം u8*)i_bytes + i_size,
                                                "snapshot", ശൂന്യ, ശൂന്യ, &data, &size);
        എങ്കിൽ (result) {
            തിരികെ result;
        }
        എങ്കിൽ (not data) {
            തിരികെ "the Wasm binary holds no embedded snapshot";
        }

        {
            M3BufferReader br = { data, size, 0 };
            തിരികെ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
        }
    }
    {
        M3BufferReader br = { (സ്ഥിരം u8*)i_bytes, i_size, 0 };
        തിരികെ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br,
                            i_size < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_size;
    തിരികെ "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name)
{
    എങ്കിൽ (not i_module) {
        തിരികെ false;
    }
#എങ്കിൽ d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ശൂന്യ;
    (ശൂന്യം)ModuleSnapshot(i_module, i_name, &snap);

    തിരികെ snap != ശൂന്യ;
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_name;
    തിരികെ false;
#endif
}

M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name,
                                 സ്ഥിരം ശൂന്യം** o_bytes, size_t* o_size)
{
    എങ്കിൽ (not i_module or not o_bytes or not o_size) {
        തിരികെ m3Err_mallocFailed;
    }

    *o_bytes = ശൂന്യ;
    *o_size  = 0;

#എങ്കിൽ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ശൂന്യ;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);

        എങ്കിൽ (result) {
            തിരികെ result;
        }
        എങ്കിൽ (not snap) {
            തിരികെ "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->data;
        *o_size  = snap->size;

        തിരികെ m3Err_none;
    }
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_name;
    തിരികെ "snapshots are not available in this build of Wasm3";
#endif
}

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name)
{
    എങ്കിൽ (not io_runtime or not i_module) {
        തിരികെ m3Err_mallocFailed;
    }
#എങ്കിൽ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ശൂന്യ;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);
        M3BufferReader      br;

        എങ്കിൽ (result) {
            തിരികെ result;
        }
        എങ്കിൽ (not snap) {
            തിരികെ "the module holds no embedded snapshot of that name";
        }

        br.buffer = snap->data;
        br.size   = snap->size;
        br.cursor = 0;

        തിരികെ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
    }
#അല്ലെങ്കിൽ
    (ശൂന്യം)i_name;
    തിരികെ "snapshots are not available in this build of Wasm3";
#endif
}


M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, സ്ഥിരം അക്ഷരം* i_name,
                                  ശൂന്യം** o_bytes, size_t* o_size)
{
#എങ്കിൽ d_m3HasSnapshots
    M3Result       result = m3Err_none;
    M3BufferWriter bw     = { ശൂന്യ, 0, 0 };
    u8*            out    = ശൂന്യ;

    അക്ഷരം      secName[d_m3MaxSnapshotNameLength + വലുപ്പം("snapshot.")];
    u32       nameLen, nameLenLebSize, payloadSize, payloadSizeLebSize;
    size_t    origSize, totalSize;
    സ്ഥിരം u8* dropStart = ശൂന്യ;
    സ്ഥിരം u8* dropEnd   = ശൂന്യ;
    IM3Module saved     = ശൂന്യ;
    u8*       p;

    എങ്കിൽ (not io_runtime or not i_module or not o_bytes or not o_size) {
        തിരികെ m3Err_mallocFailed;
    }

    *o_bytes = ശൂന്യ;
    *o_size  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, വലുപ്പം(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->വേര്_Continuation or io_runtime->വേര്_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, BufferWriter_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.size);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.size > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, ശൂന്യ, ശൂന്യ));

    origSize           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    nameLenLebSize     = SizeOfLEB_u32(nameLen);
    payloadSize        = nameLenLebSize + nameLen + (u32)bw.size;
    payloadSizeLebSize = SizeOfLEB_u32(payloadSize);
    totalSize          = origSize + 1 + payloadSizeLebSize + payloadSize;

    out = (u8*)m3_Malloc("wasm_snapshot_module", totalSize);
    _throwifnull(out);

    p = out;

    എങ്കിൽ (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } അല്ലെങ്കിൽ {
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
    out      = ശൂന്യ;

_catch:
    m3_Free(bw.buffer);
    m3_Free(out);

    തിരികെ result;
#അല്ലെങ്കിൽ
    (ശൂന്യം)io_runtime;
    (ശൂന്യം)i_module;
    (ശൂന്യം)i_name;
    (ശൂന്യം)o_bytes;
    (ശൂന്യം)o_size;
    തിരികെ "snapshots are not available in this build of Wasm3";
#endif
}
