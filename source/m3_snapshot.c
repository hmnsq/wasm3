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

#અન્તર્ભાવ "m3_env.h"
#અન્તર્ભાવ "m3_compile.h"
#અન્તર્ભાવ "m3_exception.h"
#અન્તર્ભાવ "m3_host.h"

#અન્તર્ભાવ <limits.h>

#જો d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
સ્થિર અચલ u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
સ્થિર અચલ u32 c_snapshotVersion  = 1;

ગણના {
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
ગણના {
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
સ્થિર
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

        જ્યારે (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     size      = 0;

_           (ReadLEB_u32(&size, &pos, end));
            _throwif(m3Err_wasmUnderrun, size > (size_t)(end - pos));

            pos += size;
            જો (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    પાછા result;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

સ્થિર
અચલ M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    અચલ M3SnapshotMap* map = i_function->snapshotMap;

    જો (map) {
        માટે (u32 i = 0; i < map->અંક_Safe_Points; ++i) {
            જો (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                પાછા &map->safePoints[i];
            }
        }
    }

    પાછા ખાલી;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
સ્થિર
અચલ M3SafePoint* FindSafePointAt (અચલ M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 નીચલું  = 0;
    u32 ઉપલું = i_map->અંક_Safe_Points;

    જ્યારે (નીચલું < ઉપલું) {
        u32 મધ્ય = નીચલું + (ઉપલું - નીચલું) / 2;

        જો (i_map->safePoints[મધ્ય].wasmOffset < i_wasmOffset) {
            નીચલું = મધ્ય + 1;
        } નહીંતર {
            ઉપલું = મધ્ય;
        }
    }

    માટે (u32 i = નીચલું; i < i_map->અંક_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        અચલ M3SafePoint* point = &i_map->safePoints[i];

        જો (point->kind != i_kind or (point->flags & d_m3SafePointTailCall)) {
            ચાલુ;
        }
        જો (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            પાછા point;
        }
    }

    પાછા ખાલી;
}

સ્થિર
અચલ M3BlockStart* FindBlockAt (અચલ M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    માટે (u32 i = 0; i < i_map->અંક_Blocks; ++i) {
        જો (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            પાછા &i_map->blocks[i];
        }
    }

    પાછા ખાલી;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
સ્થિર અંતઃરેખ
bool EnclosesOffset (અચલ M3BlockStart* i_block, u32 i_wasmOffset)
{
    પાછા i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
સ્થિર અંતઃરેખ
bool HandlersLiveAt (અચલ M3SnapshotMap* i_map, અચલ M3SafePoint* i_point, અચલ M3BlockStart* i_try)
{
    પાછા not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
સ્થિર
M3Result CallSiteType (IM3Function i_caller, અચલ M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3Result   result = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        size   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ખાલી;

_   (ReadLEB_u32(&size, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    જો (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->અંક_Functions);
        *o_type = module->functions[index].funcType;
    } નહીંતર {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->અંક_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    પાછા result;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
સ્થિર
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 અંક_Results = Get_Func_Type_અંક_Results(i_expected);

    જો (not i_type or Get_Func_Type_અંક_Results(i_type) != અંક_Results) {
        પાછા false;
    }

    માટે (u16 i = 0; i < અંક_Results; ++i) {
        જો (not IsSubTypeOf(GetFuncTypeResultType(i_type, i), GetFuncTypeResultType(i_expected, i))) {
            પાછા false;
        }
    }

    પાછા true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
પ્રકારવ્યાખ્યા સંરચના M3FrameRegisters {
    m3reg_t* r0;
#  જો d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
સ્થિર
M3Result Continuation_ઢગલો (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_અંક_Slots)
{
    જો (i_cont == i_runtime->મૂળ_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_ઢગલો;
        *o_અંક_Slots = i_runtime->અંક_ઢગલો_Slots;
    } નહીંતર {
        *o_base     = i_cont->val_ઢગલો;
        *o_અંક_Slots = i_cont->અંક_ઢગલો_Slots;
    }

    પાછા (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
સ્થિર
M3Result Locate_કિંમત (m3slot_t* i_base, u32 i_અંક_Slots, m3stack_t i_sp, અચલ M3_Slot_કિંમત* i_કિંમત, u8** o_where)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_કિંમત->type);

    *o_where = ખાલી;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_કિંમત->slot));

    જો (not IsRegisterSlotAlias(i_કિંમત->slot)) {
        size_t size   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? કદ(શૂન્ય*)
                                                                   : (Is64BitType(type) ? કદ(u64) : કદ(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_કિંમત->slot) * કદ(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + size > (size_t)i_અંક_Slots * કદ(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    પાછા result;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
સ્થિર
M3Result Read_કિંમત (u8* i_where, અચલ M3FrameRegisters* i_registers, અચલ M3_Slot_કિંમત* i_કિંમત, u64* o_bits,
                    શૂન્ય** o_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_કિંમત->type);

    *o_bits      = 0;
    *o_reference = ખાલી;

    જો (i_where) {
        જો (IsRefType(type)) {
            memcpy(o_reference, i_where, કદ(શૂન્ય*));
        } નહીંતર જો (Is64BitType(type)) {
            memcpy(o_bits, i_where, કદ(u64));
        } નહીંતર {
            u32 narrow;
            memcpy(&narrow, i_where, કદ(narrow));
            *o_bits = narrow;
        }
    } નહીંતર જો (IsFpRegisterSlotAlias(i_કિંમત->slot)) {
#  જો d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        જો (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, કદ(bits));
            *o_bits = bits;
        } નહીંતર {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, કદ(u64));
        }
#  નહીંતર
        _throw(m3Err_wasmMalformed);
#  endif
    } નહીંતર {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        જો (IsRefType(type)) {
            *o_reference = (શૂન્ય*)(uintptr_t)r0;
        } નહીંતર જો (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } નહીંતર {
            *o_bits = (u32)r0;
        }
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result Write_કિંમત (u8* o_where, અચલ M3FrameRegisters* i_registers, અચલ M3_Slot_કિંમત* i_કિંમત, u64 i_bits,
                     શૂન્ય* i_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_કિંમત->type);

    જો (o_where) {
        જો (IsRefType(type)) {
            memcpy(o_where, &i_reference, કદ(શૂન્ય*));
        } નહીંતર જો (Is64BitType(type)) {
            memcpy(o_where, &i_bits, કદ(u64));
        } નહીંતર {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, કદ(narrow));
        }
    } નહીંતર જો (IsFpRegisterSlotAlias(i_કિંમત->slot)) {
#  જો d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        જો (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, કદ(narrow));
            *i_registers->fp0 = narrow;
        } નહીંતર {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, કદ(u64));
        }
#  નહીંતર
        _throw(m3Err_wasmMalformed);
#  endif
    } નહીંતર {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        જો (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } નહીંતર {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    પાછા result;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
સ્થિર
અચલ M3_Slot_કિંમત* Frame_કિંમત (અચલ M3SnapshotMap* i_map, અચલ M3SafePoint* i_point, u32 i_index)
{
    જો (i_index < i_map->અંક_Locals) {
        પાછા i_point->first_Local_કિંમત == UINT32_MAX ? &i_map->locals[i_index]
                                                      : &i_map->કિંમતો[i_point->first_Local_કિંમત + i_index];
    }
    પાછા &i_map->કિંમતો[i_point->first_કિંમત + i_index - i_map->અંક_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
સ્થિર
શૂન્ય RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    જો (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->અંક_Ret_And_Arg_Slots) + i_function->અંક_Local_Bytes;

        memcpy(where, i_function->constants, i_function->અંક_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
પ્રકારવ્યાખ્યા સંરચના M3PointerIds {
    શૂન્ય** વસ્તુઓ;
    u32    ગણતરી;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  indexCapacity;       // a power of two, kept at least twice count
} M3PointerIds;

સ્થિર
u32 PointerBucket (શૂન્ય* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    પાછા (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

સ્થિર
u32 PointerIds_Find (અચલ M3PointerIds* i_ids, શૂન્ય* i_pointer)
{
    જો (i_ids->indexCapacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->indexCapacity);

        જ્યારે (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            જો (i_ids->વસ્તુઓ[id] == i_pointer) {
                પાછા id;
            }

            bucket = (bucket + 1) & (i_ids->indexCapacity - 1);
        }
    }

    પાછા d_m3SnapshotNone;
}

સ્થિર
M3Result PointerIds_Add (M3PointerIds* io_ids, શૂન્ય* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    જો (id != d_m3SnapshotNone) {
        *o_id = id;
        પાછા m3Err_none;
    }

    જો (io_ids->ગણતરી == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        શૂન્ય** વસ્તુઓ    = m3_ReallocArray(શૂન્ય*, io_ids->વસ્તુઓ, capacity, io_ids->capacity);

        જો (not વસ્તુઓ) {
            પાછા m3Err_mallocFailed;
        }

        io_ids->વસ્તુઓ    = વસ્તુઓ;
        io_ids->capacity = capacity;
    }

    જો ((io_ids->ગણતરી + 1) * 2 > io_ids->indexCapacity) {
        u32  capacity = io_ids->indexCapacity ? io_ids->indexCapacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        જો (not index) {
            પાછા m3Err_mallocFailed;
        }

        માટે (u32 i = 0; i < io_ids->ગણતરી; ++i) {
            u32 bucket = PointerBucket(io_ids->વસ્તુઓ[i], capacity);

            જ્યારે (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->indexCapacity = capacity;
    }

    id                = io_ids->ગણતરી++;
    io_ids->વસ્તુઓ[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->indexCapacity);

    જ્યારે (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->indexCapacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    પાછા m3Err_none;
}

સ્થિર
શૂન્ય PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->વસ્તુઓ);
    m3_Free(io_ids->index);
    memset(io_ids, 0, કદ(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

પ્રકારવ્યાખ્યા સંરચના M3BufferWriter {
    u8*    buffer;
    size_t size;
    size_t capacity;
} M3BufferWriter;

સ્થિર
M3Result BufferWriter_Write (અચલ શૂન્ય* i_data, size_t i_size, શૂન્ય* i_userdata)
{
    M3BufferWriter* bw = (M3BufferWriter*)i_userdata;

    જો (bw->size + i_size > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        જ્યારે (bw->size + i_size > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->buffer, newCap, bw->capacity);
        જો (!newBuf) {
            પાછા m3Err_mallocFailed;
        }
        bw->buffer   = newBuf;
        bw->capacity = newCap;
    }

    જો (i_size) {
        memcpy(bw->buffer + bw->size, i_data, i_size);
        bw->size += i_size;
    }

    પાછા m3Err_none;
}

પ્રકારવ્યાખ્યા સંરચના M3BufferReader {
    અચલ u8* buffer;
    size_t    size;
    size_t    cursor;
} M3BufferReader;

સ્થિર
M3Result BufferReader_Read (શૂન્ય* o_buffer, size_t i_size, શૂન્ય* i_userdata)
{
    M3BufferReader* br = (M3BufferReader*)i_userdata;

    જો (i_size > br->size - br->cursor) {
        પાછા m3Err_wasmMalformed;
    }

    જો (i_size) {
        memcpy(o_buffer, br->buffer + br->cursor, i_size);
        br->cursor += i_size;
    }

    પાછા m3Err_none;
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
પ્રકારવ્યાખ્યા સંરચના M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    શૂન્ય*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

સ્થિર અંતઃરેખ
u8 M3TypeToValType (m3type_t i_type)
{
    પસંદગી (BaseTypeOf(i_type)) {
    સ્થિતિ c_m3Type_i32: પાછા 0x7F;
    સ્થિતિ c_m3Type_i64: પાછા 0x7E;
    સ્થિતિ c_m3Type_f32: પાછા 0x7D;
    સ્થિતિ c_m3Type_f64: પાછા 0x7C;
    સ્થિતિ c_m3Type_v128: પાછા 0x7B;
    સ્થિતિ c_m3Type_funcref: પાછા 0x70;
    સ્થિતિ c_m3Type_externref: પાછા 0x6F;
    સ્થિતિ c_m3Type_exnref: પાછા 0x69;
    સ્થિતિ c_m3Type_contref: પાછા 0x68;
    મૂળભૂત: પાછા 0x00;
    }
}

સ્થિર અંતઃરેખ
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    પસંદગી (i_valtype) {
    સ્થિતિ 0x7F: પાછા c_m3Type_i32;
    સ્થિતિ 0x7E: પાછા c_m3Type_i64;
    સ્થિતિ 0x7D: પાછા c_m3Type_f32;
    સ્થિતિ 0x7C: પાછા c_m3Type_f64;
    સ્થિતિ 0x7B: પાછા c_m3Type_v128;
    સ્થિતિ 0x70: પાછા c_m3Type_funcref;
    સ્થિતિ 0x6F: પાછા c_m3Type_externref;
    સ્થિતિ 0x69: પાછા c_m3Type_exnref;
    સ્થિતિ 0x68: પાછા c_m3Type_contref;
    મૂળભૂત: પાછા c_m3Type_none;
    }
}

સ્થિર
M3Result Put (M3SnapshotSave* s, અચલ શૂન્ય* i_data, size_t i_size)
{
    પાછા s->writing ? s->writer(i_data, i_size, s->userdata) : m3Err_none;
}

સ્થિર
M3Result PutU8 (M3SnapshotSave* s, u8 i_કિંમત)
{
    પાછા Put(s, &i_કિંમત, કદ(i_કિંમત));
}


સ્થિર
M3Result PutLEB_u32 (M3SnapshotSave* s, u32 i_કિંમત)
{
    u8  bytes[5];
    u32 len = 0;

    કરો {
        u8 byte = i_કિંમત & 0x7F;
        i_કિંમત >>= 7;
        જો (i_કિંમત != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } જ્યારે (i_કિંમત != 0);

    પાછા Put(s, bytes, len);
}

સ્થિર
M3Result PutLEB_u64 (M3SnapshotSave* s, u64 i_કિંમત)
{
    u8  bytes[10];
    u32 len = 0;

    કરો {
        u8 byte = i_કિંમત & 0x7F;
        i_કિંમત >>= 7;
        જો (i_કિંમત != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } જ્યારે (i_કિંમત != 0);

    પાછા Put(s, bytes, len);
}

સ્થિર
M3Result PutLEB_i64 (M3SnapshotSave* s, i64 i_કિંમત)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    જ્યારે (more) {
        u8 byte = i_કિંમત & 0x7F;
        i_કિંમત >>= 7;
        જો ((i_કિંમત == 0 && !(byte & 0x40)) || (i_કિંમત == -1 && (byte & 0x40))) {
            more = false;
        } નહીંતર {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    પાછા Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
પ્રકારવ્યાખ્યા M3Result (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
સ્થિર
M3Result SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3Result         result   = m3Err_none;
    M3BufferWriter   bw       = { ખાલી, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    શૂન્ય*            userdata = s->userdata;

    s->writer   = BufferWriter_Write;
    s->userdata = &bw;

    result = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    જો (result) {
        જાઓ _catch;
    }

    // a section with nothing in it is left out
    જો (bw.size) {
        _throwif("snapshot section is too large", bw.size > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.size));
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    પાછા result;
}

સ્થિર
u32 FunctionIndex (IM3Module i_module, IM3Function i_function)
{
    જો (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->અંક_Functions) {
        પાછા (u32)(i_function - i_module->functions);
    }

    પાછા d_m3SnapshotNone;
}

સ્થિર
u32 FuncTypeIndex (IM3Module i_module, IM3FuncType i_type)
{
    માટે (u32 i = 0; i < i_module->અંક_Func_Types; ++i) {
        જો (i_module->funcTypes[i] == i_type) {
            પાછા i;
        }
    }

    પાછા d_m3SnapshotNone;
}

#  જો d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
સ્થિર
IM3Tag TagOfIndex (IM3Module i_module, u32 i_index)
{
    IM3Tag tag = &i_module->tags[i_index];

    પાછા tag->resolved ? tag->resolved : tag;
}

સ્થિર
u32 TagIndex (IM3Module i_module, IM3Tag i_tag)
{
    માટે (u32 i = 0; i < i_module->અંક_Tags; ++i) {
        જો (TagOfIndex(i_module, i) == i_tag) {
            પાછા i;
        }
    }

    પાછા d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
સ્થિર
M3Result NameReference (M3SnapshotSave* s, u8 i_type, શૂન્ય* i_reference, u64* o_word)
{
    M3Result result = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    જો (not i_reference) {
        પાછા m3Err_none;
    }

    જો (i_type == c_m3Type_funcref) {
        id = FunctionIndex(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } નહીંતર જો (i_type == c_m3Type_externref) {
        અચલ M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        જાઓ _catch;
    }
#  જો d_m3HasExceptionHandling
    નહીંતર જો (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 ગણતરી = s->exceptions.ગણતરી;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.ગણતરી != ગણતરી);
    }
#  endif
    નહીંતર જો (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 ગણતરી = s->continuations.ગણતરી;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.ગણતરી != ગણતરી);
    } નહીંતર {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    પાછા result;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
સ્થિર
M3Result Put_Ref_કિંમત (M3SnapshotSave* s, u8 i_type, શૂન્ય* i_reference)
{
    M3Result result = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    જો (not s->writing) {
        પાછા m3Err_none;
    }

    જો (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } નહીંતર {
        u8 kind = 0;
        જો (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } નહીંતર જો (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  જો d_m3HasExceptionHandling
        નહીંતર જો (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        નહીંતર જો (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    પાછા result;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
સ્થિર
M3Result PutLE32 (M3SnapshotSave* s, u32 i_કિંમત)
{
    u8 bytes[4];

    માટે (u32 i = 0; i < કદ(bytes); ++i) {
        bytes[i] = (u8)(i_કિંમત >> (8 * i));
    }

    પાછા Put(s, bytes, કદ(bytes));
}

સ્થિર
M3Result PutLE64 (M3SnapshotSave* s, u64 i_કિંમત)
{
    u8 bytes[8];

    માટે (u32 i = 0; i < કદ(bytes); ++i) {
        bytes[i] = (u8)(i_કિંમત >> (8 * i));
    }

    પાછા Put(s, bytes, કદ(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
સ્થિર
M3Result Put_કિંમત_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    જો (IsRefType(i_type)) {
        પાછા Put_Ref_કિંમત(s, BaseTypeOf(i_type), (શૂન્ય*)(uintptr_t)i_bits);
    }

    પસંદગી (BaseTypeOf(i_type)) {
    સ્થિતિ c_m3Type_i32: પાછા PutLEB_i64(s, (i32)(u32)i_bits);
    સ્થિતિ c_m3Type_i64: પાછા PutLEB_i64(s, (i64)i_bits);
    સ્થિતિ c_m3Type_f32: પાછા PutLE32(s, (u32)i_bits);
    સ્થિતિ c_m3Type_f64: પાછા PutLE64(s, i_bits);
    મૂળભૂત: પાછા "a snapshot cannot hold a value of this type as bits";
    }
}

સ્થિર
M3Result Put_કિંમત_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    જો (not valtype) {
        પાછા "a snapshot has no tag for a value of this type";
    }

    પાછા PutU8(s, valtype);
}

સ્થિર
M3Result Put_કિંમત_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3Result result = m3Err_none;

_   (Put_કિંમત_Tag(s, i_type));
_   (Put_કિંમત_Body(s, i_type, i_bits));

_catch:
    પાછા result;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
સ્થિર
M3Result Put_કિંમત (M3SnapshotSave* s, m3type_t i_type, અચલ શૂન્ય* i_where)
{
    M3Result result = m3Err_none;

_   (Put_કિંમત_Tag(s, i_type));

    જો (IsRefType(i_type)) {
        શૂન્ય* reference;
        memcpy(&reference, i_where, કદ(reference));
_       (Put_કિંમત_Body(s, i_type, (u64)(uintptr_t)reference));
    } નહીંતર જો (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, કદ(bytes));
_       (Put(s, bytes, કદ(bytes)));
    } નહીંતર જો (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, કદ(bits));
_       (Put_કિંમત_Body(s, i_type, bits));
    } નહીંતર {
        u32 narrow;
        memcpy(&narrow, i_where, કદ(narrow));
_       (Put_કિંમત_Body(s, i_type, narrow));
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result PutMemoryChunks (M3SnapshotSave* s, અચલ u8* i_bytes, size_t i_size)
{
    M3Result result = m3Err_none;
    size_t   cursor = 0;

    જ્યારે (cursor < i_size) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        જ્યારે (cursor + run < i_size and i_bytes[cursor + run] == byte) {
            run++;
        }

        જો ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            જો (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            ચાલુ;
        }

        size_t rawStart = cursor;

        જ્યારે (cursor < i_size) {
            u8     b      = i_bytes[cursor];
            size_t length = 1;

            જ્યારે (cursor + length < i_size and i_bytes[cursor + length] == b) {
                length++;
            }

            જો ((b == 0x00 or b == 0xFF) and length >= d_m3SnapshotRunThreshold) {
                તોડો;
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
    પાછા result;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
સ્થિર
u32 Memory_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    જ્યારે (module->memories[j] != module->memories[index]) {
        ++j;
    }
    પાછા j;
}

સ્થિર
u32 Table_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    જ્યારે (module->tables[j] != module->tables[index]) {
        ++j;
    }
    પાછા j;
}

સ્થિર
M3Result SaveMemories (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->અંક_Memories));

    માટે (u32 m = 0; m < module->અંક_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_FirstIndex(module, m);
        bool      hasData  = memory and memory->mallocated;
        u32       pageSize = memory ? Memory_PageSize(memory) : d_m3DefaultMemPageSize;
        u64       bytes    = memory ? memory->અંક_Pages * (u64)pageSize : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        જો (first != m) {
            ચાલુ;
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
        જ્યારે ((1u << pageBits) < pageSize) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->અંક_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        જો (not s->writing) {
            ચાલુ;
        }

        જો (hasData) {
_           (PutMemoryChunks(s, m3MemData(memory->mallocated), (size_t)bytes));
        } નહીંતર {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result SaveGlobals (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->અંક_Globals));

    માટે (u32 g = 0; g < module->અંક_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_કિંમત(s, global->type, &cell->i64_કિંમત));
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result SaveTables (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->અંક_Tables));

    માટે (u32 t = 0; t < module->અંક_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_FirstIndex(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        જો (first != t) {
            ચાલુ;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->size));

        માટે (u32 e = 0; e < table->size; ++e) {
_           (Put_Ref_કિંમત(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result SaveSegments (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->અંક_Data_Segments));

    માટે (u32 i = 0; i < module->અંક_Data_Segments; ++i) {
_       (PutU8(s, module->dataSegments[i].dropped));
    }

_   (PutLEB_u32(s, module->અંક_Element_Segments));

    માટે (u32 i = 0; i < module->અંક_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    પાછા result;
}

#  જો d_m3HasExceptionHandling

સ્થિર
M3Result SaveExceptions (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.ગણતરી));

    માટે (u32 i = 0; i < s->exceptions.ગણતરી; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.વસ્તુઓ[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->અંક_Args != Get_Func_Type_અંક_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->અંક_Args));

        માટે (u32 a = 0; a < exn->અંક_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_કિંમત_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    પાછા result;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
પ્રકારવ્યાખ્યા સંરચના M3Activation {
    IM3Function        function;
    અચલ M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    અચલ M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
સ્થિર
અચલ M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    અચલ M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : ખાલી;

    માટે (u32 i = 0; not point and i < i_module->અંક_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        જો (point) {
            *io_function = &i_module->functions[i];
        }
    }

    પાછા point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
સ્થિર
M3Result NextActivation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3Result result = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, કદ(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    માટે (; i >= 0; --i) {
        અચલ M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        જો (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  જો d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            તોડો;
        } નહીંતર જો (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            તોડો;
        }
    }

    જો (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  જો d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  જો d_m3EntryKeepsFrame
    માટે (i32 k = i_first; k > o_act->lastFrame; --k) {
        અચલ M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    પાછા result;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
સ્થિર
M3Result SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_અંક_Slots, અચલ M3Activation* i_act)
{
    M3Result             result        = m3Err_none;
    IM3Function          function      = i_act->function;
    અચલ M3SnapshotMap* map           = function->snapshotMap;
    u32                  functionIndex = FunctionIndex(s->module, function);
    અચલ M3SafePoint*   point         = i_act->point;
    u32                  અંક_કિંમતો;

    _throwif("a suspended frame belongs to another module", functionIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, functionIndex));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    જો (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    અંક_કિંમતો = map->અંક_Locals + point->અંક_કિંમતો;

_   (PutLEB_u32(s, અંક_કિંમતો));

    માટે (u32 v = 0; v < અંક_કિંમતો; ++v) {
        અચલ M3_Slot_કિંમત* કિંમત = Frame_કિંમત(map, point, v);
        u8*                where;
        u64                word;
        શૂન્ય*              reference;

_       (Locate_કિંમત(i_base, i_અંક_Slots, i_act->sp, કિંમત, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        જો (BaseTypeOf(કિંમત->type) == c_m3Type_v128) {
_           (Put_કિંમત_Tag(s, કિંમત->type));
_           (Put(s, where, 16));
            ચાલુ;
        }

_       (Read_કિંમત(where, &i_act->registers, કિંમત, &word, &reference));

_       (Put_કિંમત_Bits(s, કિંમત->type, IsRefType(કિંમત->type) ? (u64)(uintptr_t)reference : word));
    }

    જો (point->kind == safepoint_resume) {
_       (Put_Ref_કિંમત(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    પાછા result;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
સ્થિર
M3Result SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3Result      result   = m3Err_none;
    M3Activation* acts     = ખાલી;
    u32           અંક_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->અંક_Frames - 1;
    m3slot_t*     base;
    u32           અંક_Slots;
    m3stack_t     sp;

_   (Continuation_ઢગલો(s->runtime, i_cont, &base, &અંક_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->અંક_Frames + 1);
    _throwifnull(acts);

    sp = base;

    જ્યારે (true) {
        M3Activation* act = &acts[અંક_Acts];

_       (NextActivation(s->module, i_cont, function, sp, first, act));

        જો (act->kind != safepoint_call) {
            અંક_Acts++;
            તોડો;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        જો (not (act->point->flags & d_m3SafePointTailCall)) {
            અંક_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, અંક_Acts));

    માટે (u32 a = 0; a < અંક_Acts; ++a) {
_       (SaveActivation(s, base, અંક_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    પાછા result;
}

સ્થિર
M3Result SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3Result  result    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_મૂળ    = (i_cont == s->runtime->મૂળ_Continuation);
    u8        state     = snapshot_contFinished;
    u32       typeIndex = is_મૂળ ? d_m3SnapshotNone : FuncTypeIndex(module, i_cont->type);

    જો (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } નહીંતર જો (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } નહીંતર જો (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", FunctionIndex(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_મૂળ and typeIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_મૂળ));
_   (PutLEB_u32(s, typeIndex));
_   (PutLEB_u32(s, FunctionIndex(module, i_cont->entryFunction)));
#  જો d_m3HasExceptionHandling
_   (Put_Ref_કિંમત(s, c_m3Type_exnref, i_cont->resumeThrow));
#  નહીંતર
_   (Put_Ref_કિંમત(s, c_m3Type_exnref, ખાલી));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    જો (s->postmortem) {
        જાઓ _catch;
    }

    જો (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : ખાલી;
        u16         અંક_Rets = inner ? Get_Func_Type_અંક_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_ગણતરી > Get_Func_Type_અંક_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_ગણતરી));

        માટે (u32 i = 0; i < i_cont->bound_Args_ગણતરી; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_કિંમત(s, type, i_cont->val_ઢગલો + (અંક_Rets + i) * c_io_Slot_ગણતરી));
        }
    } નહીંતર જો (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_ગણતરી));
_       (SaveSuspendedBody(s, i_cont));
    } નહીંતર {
_       (PutLEB_u32(s, 0));
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result SaveContinuations (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->continuations.ગણતરી));

    માટે (u32 i = 0; i < s->continuations.ગણતરી; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.વસ્તુઓ[i]));
    }

_catch:
    પાછા result;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
સ્થિર
M3Result SaveHostState (M3SnapshotSave* s)
{
    M3Result               result = m3Err_none;
    અચલ M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3BufferWriter         bw     = { ખાલી, 0, 0 };

    જો (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, BufferWriter_Write, &bw));
    }

    જો (bw.size > 0) {
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    પાછા result;
}

સ્થિર
શૂન્ય SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    માટે (u32 i = 0; i < module->અંક_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        જો (Memory_FirstIndex(module, i) == i and memory) {
            *memoryBytes += memory->અંક_Pages * (u64)Memory_PageSize(memory);
        }
    }
    માટે (u32 i = 0; i < module->અંક_Tables; ++i) {
        IM3Table table = module->tables[i];
        જો (Table_FirstIndex(module, i) == i and table) {
            *tableElements += table->size;
        }
    }
}

સ્થિર
M3Result SaveMeta (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    માટે (u32 i = 0; i < s->continuations.ગણતરી; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.વસ્તુઓ[i];
        જો (cont != s->runtime->મૂળ_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.ગણતરી));
_   (PutLEB_u32(s, s->exceptions.ગણતરી));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  જો d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    માટે (u32 i = 0; i < s->exceptions.ગણતરી; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.વસ્તુઓ[i];
        u32          tagIndex  = TagIndex(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tagIndex == d_m3SnapshotNone);

_       (PutLEB_u32(s, tagIndex));
    }
#  endif

_catch:
    પાછા result;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
સ્થિર
M3Result DiscoverReferences (M3SnapshotSave* s)
{
    M3Result result            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    જ્યારે (doneContinuations < s->continuations.ગણતરી or doneExceptions < s->exceptions.ગણતરી) {
#  જો d_m3HasExceptionHandling
        જ્યારે (doneExceptions < s->exceptions.ગણતરી) {
            M3Exception* exception = (M3Exception*)s->exceptions.વસ્તુઓ[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->અંક_Args != Get_Func_Type_અંક_Params(type));

            માટે (u32 a = 0; a < exception->અંક_Args; ++a) {
_               (Put_કિંમત_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        જ્યારે (doneContinuations < s->continuations.ગણતરી and doneExceptions == s->exceptions.ગણતરી) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.વસ્તુઓ[id]));
        }
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result SaveSections (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

    જો (not s->writing) {
        પાછા DiscoverReferences(s);
    }

    જો (not s->embedded) {
_       (Put(s, c_snapshotMagic, કદ(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    જો (s->module->અંક_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    જો (s->module->અંક_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    જો (s->module->અંક_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    જો (s->module->અંક_Data_Segments or s->module->અંક_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  જો d_m3HasExceptionHandling
    જો (s->exceptions.ગણતરી) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    જો (s->continuations.ગણતરી) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    પાછા result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

પ્રકારવ્યાખ્યા સંરચના M3SnapshotSuspendSite {
    અચલ M3SnapshotMap* map;
    અચલ M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

પ્રકારવ્યાખ્યા સંરચના M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    શૂન્ય*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ઢગલો;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    અંક_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  જો d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 અંક_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

સ્થિર
M3Result Get (M3SnapshotLoad* l, શૂન્ય* o_data, size_t i_size)
{
    જો (not l->inSection) {
        પાછા l->reader(o_data, i_size, l->userdata);
    }
    જો (i_size > (size_t)(l->end - l->pos)) {
        પાછા m3Err_wasmUnderrun;
    }
    જો (i_size) {
        memcpy(o_data, l->pos, i_size);
        l->pos += i_size;
    }
    પાછા m3Err_none;
}

સ્થિર
M3Result GetU8 (M3SnapshotLoad* l, u8* o_કિંમત)
{
    પાછા Get(l, o_કિંમત, કદ(*o_કિંમત));
}

સ્થિર
M3Result GetBool (M3SnapshotLoad* l, u8* o_કિંમત)
{
    M3Result result = GetU8(l, o_કિંમત);
    પાછા result ? result : (*o_કિંમત <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

સ્થિર
M3Result GetLEB_u32 (M3SnapshotLoad* l, u32* o_કિંમત)
{
    જો (l->inSection) {
        પાછા ReadLEB_u32(o_કિંમત, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    માટે (u32 i = 0; i < કદ(encoded); ++i) {
        M3Result result = GetU8(l, &encoded[i]);
        જો (result) {
            પાછા result;
        }
        જો (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            પાછા ReadLEB_u32(o_કિંમત, &pos, encoded + i + 1);
        }
    }
    પાછા m3Err_lebOverflow;
}

સ્થિર
M3Result GetLEB_u64 (M3SnapshotLoad* l, u64* o_કિંમત)
{
    પાછા ReadLebUnsigned(o_કિંમત, 64, &l->pos, l->end);
}

સ્થિર
M3Result GetLEB_i64 (M3SnapshotLoad* l, i64* o_કિંમત)
{
    પાછા ReadLEB_i64(o_કિંમત, &l->pos, l->end);
}

સ્થિર
M3Result SkipBytes (M3SnapshotLoad* l, size_t i_size)
{
    જો (i_size > (size_t)(l->end - l->pos)) {
        પાછા m3Err_wasmUnderrun;
    }
    l->pos += i_size;
    પાછા m3Err_none;
}


સ્થિર
M3Result Get_Ref_કિંમત (M3SnapshotLoad* l, m3type_t i_type, શૂન્ય** o_reference)
{
    M3Result result = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = ખાલી;

_   (GetU8(l, &kind));
    જો (kind == 0x00) {
        પાછા (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    જો (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->અંક_Functions);
#  જો d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } નહીંતર જો (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        અચલ M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        જો (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  જો d_m3HasExceptionHandling
    નહીંતર જો (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->અંક_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    નહીંતર જો (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->અંક_Continuations);
        *o_reference = l->continuations[id];
#  જો d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        જો (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } નહીંતર {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result GetLE32 (M3SnapshotLoad* l, u32* o_કિંમત)
{
    u8       bytes[4];
    M3Result result = Get(l, bytes, કદ(bytes));

    *o_કિંમત = 0;

    માટે (u32 i = 0; i < કદ(bytes); ++i) {
        *o_કિંમત |= (u32)bytes[i] << (8 * i);
    }

    પાછા result;
}

સ્થિર
M3Result GetLE64 (M3SnapshotLoad* l, u64* o_કિંમત)
{
    u8       bytes[8];
    M3Result result = Get(l, bytes, કદ(bytes));

    *o_કિંમત = 0;

    માટે (u32 i = 0; i < કદ(bytes); ++i) {
        *o_કિંમત |= (u64)bytes[i] << (8 * i);
    }

    પાછા result;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
સ્થિર
M3Result Get_કિંમત_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3Result result  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    જો (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    પાછા result;
}

// The mirror of PutValueBody: narrow types come back in the low bits
સ્થિર
M3Result Get_કિંમત_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3Result result = m3Err_none;

    *o_bits = 0;

    જો (IsRefType(i_type)) {
        શૂન્ય* reference = ખાલી;
_       (Get_Ref_કિંમત(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } નહીંતર જો (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 કિંમત = 0;
_       (ReadLEB_i32(&કિંમત, &l->pos, l->end));
        *o_bits = (u32)કિંમત;
    } નહીંતર જો (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 કિંમત = 0;
_       (GetLEB_i64(l, &કિંમત));
        *o_bits = (u64)કિંમત;
    } નહીંતર જો (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } નહીંતર જો (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } નહીંતર {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result Get_કિંમત_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3Result result = m3Err_none;
    m3type_t type;

_   (Get_કિંમત_Tag(l, i_expectedType, &type));
_   (Get_કિંમત_Body(l, type, o_bits));

_catch:
    પાછા result;
}

સ્થિર
M3Result Get_કિંમત (M3SnapshotLoad* l, m3type_t i_expectedType, શૂન્ય* o_where)
{
    M3Result result = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    જો (l->checking) {
        o_where = scratch;
    }

_   (Get_કિંમત_Tag(l, i_expectedType, &type));

    જો (IsRefType(type)) {
        શૂન્ય* reference = ખાલી;
        u64   bits      = 0;
_       (Get_કિંમત_Body(l, type, &bits));
        reference = (શૂન્ય*)(uintptr_t)bits;
        memcpy(o_where, &reference, કદ(reference));
    } નહીંતર જો (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } નહીંતર {
        u64 bits = 0;
_       (Get_કિંમત_Body(l, type, &bits));

        જો (Is64BitType(type)) {
            memcpy(o_where, &bits, કદ(bits));
        } નહીંતર {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, કદ(narrow));
        }
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3Result result = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->અંક_Functions);
    *o_function = &l->module->functions[index];

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_size)
{
    M3Result result = m3Err_none;

    માટે (;;) {
        u8  chunkType = 0;
        u32 offset = 0, length = 0;

_       (GetU8(l, &chunkType));

        જો (chunkType == d_m3ChunkEnd) {
            તોડો;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &length));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_size or length > i_size - (size_t)offset);

        જો (chunkType == d_m3ChunkRaw) {
            જો (l->checking or length == 0) {
_               (SkipBytes(l, length));
            } નહીંતર {
_               (Get(l, o_bytes + offset, length));
            }
        } નહીંતર જો (chunkType == d_m3ChunkFillFF) {
            જો (not l->checking and length) {
                memset(o_bytes + offset, 0xFF, length);
            }
        } નહીંતર {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadMemories (M3SnapshotLoad* l)
{
    M3Result  result      = m3Err_none;
    IM3Module module      = l->module;
    u32       અંક_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &અંક_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, અંક_Memories == 0 or અંક_Memories != module->અંક_Memories);

    માટે (u32 m = 0; m < અંક_Memories; ++m) {
        u32       memIndex = 0, firstIndex = 0, pageBits = 0, અંક_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &memIndex));
_       (GetLEB_u32(l, &firstIndex));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, memIndex != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 firstIndex != Memory_FirstIndex(module, m));
        જો (firstIndex != m) {
            ચાલુ;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &અંક_Pages));

        memory = module->memories[memIndex];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    data  = ખાલી;

        જો (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_PageSize(memory));
            _throwif(m3Err_wasmMalformed, અંક_Pages < memory->અંક_Pages or અંક_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, અંક_Pages > SIZE_MAX / Memory_PageSize(memory));
            bytes = (size_t)અંક_Pages * Memory_PageSize(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            જો (not l->checking) {
                જો (memory->અંક_Pages < અંક_Pages) {
_                   (ResizeMemory(l->runtime, memory, અંક_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->length < bytes);
            }

            l->loadedMemoryBytes += bytes;

            જો (bytes and memory->mallocated) {
                data = m3MemData(memory->mallocated);
            }
        } નહીંતર {
            _throwif(m3Err_wasmMalformed, અંક_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        જો (data and not l->checking) {
            memset(data, 0, bytes);
        }

_       (LoadMemoryChunks(l, data, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadGlobals (M3SnapshotLoad* l)
{
    M3Result  result     = m3Err_none;
    IM3Module module     = l->module;
    u32       અંક_Globals = 0;

_   (GetLEB_u32(l, &અંક_Globals));
    _throwif(m3Err_wasmMalformed, અંક_Globals == 0 or અંક_Globals != module->અંક_Globals);

    માટે (u32 g = 0; g < અંક_Globals; ++g) {
        u32       globalIndex = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &globalIndex));
        _throwif(m3Err_wasmMalformed, globalIndex != g);

        global = &module->globals[globalIndex];
        cell   = global->resolved ? global->resolved : global;

_       (Get_કિંમત(l, global->type, &cell->i64_કિંમત));
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadTables (M3SnapshotLoad* l)
{
    M3Result  result       = m3Err_none;
    IM3Module module       = l->module;
    u32       અંક_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &અંક_Tables));
    _throwif(m3Err_wasmMalformed, અંક_Tables == 0 or અંક_Tables != module->અંક_Tables);

    માટે (u32 t = 0; t < અંક_Tables; ++t) {
        u32      tableIndex = 0, firstIndex = 0, size = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &tableIndex));
_       (GetLEB_u32(l, &firstIndex));

        _throwif(m3Err_wasmMalformed, tableIndex != t);
        _throwif("the snapshot shares tables between imports differently",
                 firstIndex != Table_FirstIndex(module, t));
        જો (firstIndex != t) {
            ચાલુ;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &size));

        table = module->tables[tableIndex];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, size > (table->hasMax ? table->maxSize : d_m3MaxSaneTableSize));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, size < table->size);

        l->loadedTableElements += size;

        જો (not l->checking and size != table->size) {
            શૂન્ય** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 size - table->size > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(શૂન્ય*, table->elements, size, table->size);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += size - table->size;
            table->elements = elements;
            table->size     = size;
        }

        માટે (u32 e = 0; e < size; ++e) {
            શૂન્ય* reference = ખાલી;
_           (Get_Ref_કિંમત(l, table->type, &reference));
            જો (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadSegments (M3SnapshotLoad* l)
{
    M3Result  result = m3Err_none;
    IM3Module module = l->module;
    u32       ગણતરી  = 0;

_   (GetLEB_u32(l, &ગણતરી));
    _throwif(m3Err_wasmMalformed, ગણતરી != module->અંક_Data_Segments);
    _throwif(m3Err_wasmMalformed, module->અંક_Data_Segments == 0 and module->અંક_Element_Segments == 0);

    માટે (u32 i = 0; i < ગણતરી; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        જો (not l->checking) {
            module->dataSegments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &ગણતરી));
    _throwif(m3Err_wasmMalformed, ગણતરી != module->અંક_Element_Segments);

    માટે (u32 i = 0; i < ગણતરી; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        જો (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    પાછા result;
}

// A frame for the continuation being put together, outermost first
સ્થિર
M3Result AddFrame (M3SnapshotLoad* l, u32* io_અંક_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    જો (*io_અંક_Frames >= d_m3ContinuationMaxFrames) {
        પાછા "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_અંક_Frames)++];

    memset(frame, 0, કદ(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    પાછા m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
સ્થિર
M3Result PrepareFunction (IM3Function i_function)
{
    M3Result result = m3Err_none;

    જો (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    પાછા result;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
સ્થિર
M3Result AddBlockFrames (M3SnapshotLoad* l, અચલ M3SnapshotMap* i_map, અચલ M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_અંક_Frames)
{
    M3Result result = m3Err_none;

    માટે (u32 b = 0; b < i_map->અંક_Blocks; ++b) {
        અચલ M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        જો (not EnclosesOffset(block, i_point->wasmOffset)) {
            ચાલુ;
        }

        જો (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_અંક_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  જો d_m3HasExceptionHandling
        નહીંતર {
_           (AddFrame(l, io_અંક_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.અંક_Clauses   = block->અંક_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    પાછા result;
}

// Puts a function's values where this build keeps them at the safepoint
સ્થિર
M3Result Load_કિંમતો (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_અંક_Slots, m3stack_t i_sp,
                     અચલ M3SnapshotMap* i_map, અચલ M3SafePoint* i_point, અચલ M3FrameRegisters* i_registers)
{
    M3Result result    = m3Err_none;
    u32      અંક_કિંમતો = 0;

_   (GetLEB_u32(l, &અંક_કિંમતો));
    _throwif("the snapshot's frame does not match what this build compiled", અંક_કિંમતો != i_map->અંક_Locals + i_point->અંક_કિંમતો);

    માટે (u32 v = 0; v < અંક_કિંમતો; ++v) {
        અચલ M3_Slot_કિંમત* કિંમત     = Frame_કિંમત(i_map, i_point, v);
        u64                word      = 0;
        શૂન્ય*              reference = ખાલી;
        u8*                where;

_       (Locate_કિંમત(i_base, i_અંક_Slots, i_sp, કિંમત, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        જો (BaseTypeOf(કિંમત->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_કિંમત_Tag(l, કિંમત->type, &type));
            જો (l->checking) {
_               (SkipBytes(l, 16));
            } નહીંતર {
_               (Get(l, where, 16));
            }
            ચાલુ;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= અંક_કિંમતો - i_point->અંક_Results;
_       (Get_કિંમત_Bits(l, કિંમત->type, &word));
        l->allowUnfilledRef = false;

        જો (IsRefType(કિંમત->type)) {
            reference = (શૂન્ય*)(uintptr_t)word;
        }

        જો (not l->checking) {
_           (Write_કિંમત(where, i_registers, કિંમત, word, reference));
        }
    }

_catch:
    પાછા result;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
સ્થિર
M3Result LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3Result    result    = m3Err_none;
    u32         અંક_Frames = 0;
    M3Frame*    callFrame = ખાલી;
    IM3Function function  = ખાલી;
    m3slot_t*   base;
    u32         અંક_Slots;
    u32         અંક_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_ઢગલો(l->runtime, io_cont, &base, &અંક_Slots));

_   (GetLEB_u32(l, &અંક_Activations));
    _throwif(m3Err_wasmMalformed, અંક_Activations == 0 or અંક_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = ખાલી;
    io_cont->r0                = 0;
    io_cont->અંક_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  જો d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    માટે (u32 a = 0; a < અંક_Activations; ++a) {
        bool                 isInnermost = (a + 1 == અંક_Activations);
        અચલ M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        અચલ M3SnapshotMap* map;

        memset(&registers, 0, કદ(registers));

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
                 (u32)(sp - base) + function->max_ઢગલો_Slots > અંક_Slots);

        // the call the frame outside this one is waiting on
        જો (callFrame) {
            callFrame->call.function = function;
        }

#  જો d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &અંક_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        જો (kind == safepoint_op) {
            અચલ M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &અંક_Frames));

        જો (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &અંક_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  જો d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } નહીંતર જો (kind == safepoint_resume) {
            M3Frame* frame;
            u32      અંક_Results = point->અંક_Results;
            pc_t     resultsPC  = point->pc - 2 * અંક_Results;

_           (AddFrame(l, &અંક_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.અંક_Results  = અંક_Results;
            frame->resume.અંક_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } નહીંતર {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  જો d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_કિંમતો(l, base, અંક_Slots, sp, map, point, &registers));

        જો (not l->checking) {
            RestoreConstants(function, sp);
        }

        જો (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } નહીંતર જો (kind == safepoint_resume) {
            શૂન્ય* cont = ખાલી;
_           (Get_Ref_કિંમત(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ખાલી);

            callFrame->resume.cont = (IM3Continuation)cont;
            જો (not l->checking) {
                l->suspendSites[i_id].point = point;
                માટે (u32 i = 1; i < l->અંક_Continuations; ++i) {
                    જો (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        તોડો;
                    }
                }
            }
        } નહીંતર {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            જો (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            જો (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->અંક_Results > point->અંક_કિંમતો or
                                                point->અંક_Results > d_m3MaxContinuationPayload);

                io_cont->અંક_Suspend_Results = point->અંક_Results;
                જો (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                માટે (u32 i = 0; i < point->અંક_Results; ++i) {
                    અચલ M3_Slot_કિંમત* કિંમત = &map->કિંમતો[point->first_કિંમત + point->અંક_કિંમતો - point->અંક_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(કિંમત->slot));

                    io_cont->suspendResultOffsets[i] = કિંમત->slot;
                    io_cont->suspendResultIs64[i]    = Is64BitType(કિંમત->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    જો (l->checking) {
        પાછા m3Err_none;
    }

    // held innermost first
    જો (અંક_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, અંક_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = અંક_Frames;
    }

    માટે (u32 k = 0; k < અંક_Frames; ++k) {
        io_cont->frames[અંક_Frames - 1 - k] = l->frames[k];
    }

    io_cont->અંક_Frames = અંક_Frames;

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3Result        result = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    જો (l->checking) {
        scratch          = *cont;
        scratch.val_ઢગલો = l->check_ઢગલો;
        જો (i_id == 0) {
            scratch.val_ઢગલો = (m3slot_t*)l->runtime->origin_ઢગલો;
        }
        cont = &scratch;
    }

    u32 contId = 0, typeIndex = 0, entryIndex = 0, bound_Args_ગણતરી = 0;
    u8  state = 0, is_મૂળ = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_મૂળ));
_   (GetLEB_u32(l, &typeIndex));
_   (GetLEB_u32(l, &entryIndex));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_મૂળ != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_મૂળ and state != snapshot_contSuspended);
    જો (not is_મૂળ and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, typeIndex != d_m3SnapshotNone and typeIndex >= module->અંક_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entryIndex >= module->અંક_Functions);
    _throwif(m3Err_wasmMalformed, is_મૂળ ? typeIndex != d_m3SnapshotNone : (typeIndex == d_m3SnapshotNone or not module->funcTypes[typeIndex]->isContinuation));

    જો (not l->checking and not is_મૂળ and state != snapshot_contFinished) {
_       (Continuation_Acquire_ઢગલો(l->runtime, cont));
    }
    cont->type          = (typeIndex != d_m3SnapshotNone) ? module->funcTypes[typeIndex] : ખાલી;
    cont->entryFunction = &module->functions[entryIndex];
    cont->અંક_Handlers   = 0;
    cont->handlersPC    = ખાલી;
    cont->parent        = ખાલી;
    cont->અંક_Frames     = 0;

#  જો d_m3HasExceptionHandling
    {
        શૂન્ય* ref = ખાલી;
_       (Get_Ref_કિંમત(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_મૂળ or state == snapshot_contFinished) and cont->resumeThrow != ખાલી);
#  નહીંતર
    {
        શૂન્ય* ref = ખાલી;
_       (Get_Ref_કિંમત(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ખાલી);
    }
#  endif

    જો (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         અંક_Rets;

_       (GetLEB_u32(l, &bound_Args_ગણતરી));
        cont->bound_Args_ગણતરી = bound_Args_ગણતરી;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_ગણતરી > Get_Func_Type_અંક_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_અંક_Params(remaining) != Get_Func_Type_અંક_Params(inner) - bound_Args_ગણતરી or
                                            Get_Func_Type_અંક_Results(remaining) != Get_Func_Type_અંક_Results(inner));
            માટે (u16 i = 0; i < Get_Func_Type_અંક_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(inner, i) != GetFuncTypeResultType(remaining, i));
            }
            માટે (u16 i = 0; i < Get_Func_Type_અંક_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_ગણતરી + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        અંક_Rets = Get_Func_Type_અંક_Results(inner);

        _throwif(m3Err_wasmMalformed, (અંક_Rets + bound_Args_ગણતરી) * c_io_Slot_ગણતરી > cont->અંક_ઢગલો_Slots);

        માટે (u32 i = 0; i < bound_Args_ગણતરી; ++i) {
_           (Get_કિંમત(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ઢગલો + (અંક_Rets + i) * c_io_Slot_ગણતરી));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ઢગલો;
        cont->pc    = cont->entryFunction->compiled;
    } નહીંતર જો (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_ગણતરી));
        _throwif(m3Err_wasmMalformed, is_મૂળ and bound_Args_ગણતરી != 0);
        cont->bound_Args_ગણતરી = bound_Args_ગણતરી;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } નહીંતર જો (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_ગણતરી));
        _throwif(m3Err_wasmMalformed, bound_Args_ગણતરી != 0);
        cont->bound_Args_ગણતરી = 0;
        cont->state          = cont_consumed;
    } નહીંતર {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    જો (result) {
        cont->અંક_Frames = 0;
    }

    પાછા result;
}

સ્થિર
M3Result LoadContinuations (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u32      ગણતરી  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &ગણતરી));
    _throwif(m3Err_wasmMalformed, ગણતરી == 0 or ગણતરી != l->અંક_Continuations);

    માટે (u32 i = 0; i < ગણતરી; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    પાછા result;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
સ્થિર
M3Result CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;

    માટે (u32 id = 0; id < l->અંક_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  જો d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        જો (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : ખાલી;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_અંક_Results(actual) != Get_Func_Type_અંક_Results(expected));
            માટે (u16 i = 0; i < Get_Func_Type_અંક_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(actual, i) != GetFuncTypeResultType(expected, i));
            }
        }
        જ્યારે (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->અંક_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        જો (cont->state != cont_suspended or id == 0) {
            ચાલુ;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        જો (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_ગણતરી != 0);
            ચાલુ;
        }

        અચલ M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_ગણતરી > inner->અંક_Suspend_Results or
                                                                          Get_Func_Type_અંક_Params(remaining) != inner->અંક_Suspend_Results - cont->bound_Args_ગણતરી);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_અંક_Results(remaining) != Get_Func_Type_અંક_Results(cont->entryFunction->funcType));
        માટે (u16 i = 0; i < Get_Func_Type_અંક_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(remaining, i) !=
                                            GetFuncTypeResultType(cont->entryFunction->funcType, i));
        }

        માટે (u32 i = 0; i < inner->અંક_Suspend_Results; ++i) {
            અચલ M3_Slot_કિંમત* કિંમત = &site->map->કિંમતો[site->point->first_કિંમત +
                                                          site->point->અંક_કિંમતો - site->point->અંક_Results + i];
            જો (i < cont->bound_Args_ગણતરી) {
                જો (IsRefType(કિંમત->type) and not IsNullableRef(કિંમત->type)) {
                    શૂન્ય* ref = ખાલી;
                    memcpy(&ref, inner->sp + કિંમત->slot, કદ(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } નહીંતર {
                _throwif("a captured continuation has the wrong parameter type", કિંમત->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_ગણતરી)));
            }
        }
    }

_catch:
    પાછા result;
}

#  જો d_m3HasExceptionHandling

સ્થિર
M3Result LoadExceptions (M3SnapshotLoad* l)
{
    M3Result result        = m3Err_none;
    u32      અંક_Exceptions = 0;

_   (GetLEB_u32(l, &અંક_Exceptions));
    _throwif(m3Err_wasmMalformed, અંક_Exceptions == 0 or અંક_Exceptions != l->અંક_Exceptions);

    માટે (u32 i = 0; i < અંક_Exceptions; ++i) {
        u32          exnId = 0, અંક_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &અંક_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, અંક_Args != exn->અંક_Args);

        માટે (u32 a = 0; a < અંક_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_કિંમત_Bits(l, argType, &bits));
            જો (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    પાછા result;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
પ્રકારવ્યાખ્યા સંરચના M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3Result        error;
} M3BoundedReader;

સ્થિર
M3Result BoundedReader_Read (શૂન્ય* o_buffer, size_t i_size, શૂન્ય* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    જો (i_size > br->remaining) {
        br->error = "the host read past the end of its state";
        પાછા br->error;
    }

    br->remaining -= i_size;

    પાછા Get(br->load, o_buffer, i_size);
}

સ્થિર
M3Result LoadHostState (M3SnapshotLoad* l)
{
    M3Result               result = m3Err_none;
    અચલ M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 size = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    જો (l->checking) {
        પાછા SkipBytes(l, size);
    }
    reader.load      = l;
    reader.remaining = size;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, size));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadHeader (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u8       magic[કદ(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, કદ(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, કદ(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    પાછા result;
}

સ્થિર
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

    (શૂન્ય)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->અંક_Continuations));
_   (GetLEB_u32(l, &l->અંક_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->અંક_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->અંક_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    જો (*o_flags & d_m3SnapshotFlagPostmortem) {
        માટે (u32 i = 0; i < l->અંક_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
        }
        પાછા m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->અંક_Continuations == 0 or activeStacks >= l->અંક_Continuations);
#  જો !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->અંક_Exceptions != 0);
#  endif

    જો (l->checking) {
        માટે (u32 i = 0; i < l->અંક_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
#  જો d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tagIndex >= l->module->અંક_Tags);
#  endif
        }
        પાછા m3Err_none;
    }

    l->check_ઢગલો = m3_AllocArray(m3slot_t, d_m3_Continuation_ઢગલો_Slots + 4);
    _throwifnull(l->check_ઢગલો);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->અંક_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->અંક_Continuations);
    _throwifnull(l->suspendSites);

    જો (not l->runtime->મૂળ_Continuation) {
        IM3Continuation મૂળ = Continuation_New(l->runtime, ખાલી, ખાલી);
        _throwifnull(મૂળ);

        // the root borrows the runtime's stack, so it holds none of its own
        મૂળ->અંક_ઢગલો_Slots          = l->runtime->અંક_ઢગલો_Slots;
        l->runtime->મૂળ_Continuation = મૂળ;
    }

    l->continuations[0] = l->runtime->મૂળ_Continuation;

    માટે (u32 i = 1; i < l->અંક_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ખાલી, ખાલી);
        _throwifnull(l->continuations[i]);
    }

#  જો d_m3HasExceptionHandling
    જો (l->અંક_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->અંક_Exceptions);
        _throwifnull(l->exceptions);
    }

    માટે (u32 i = 0; i < l->અંક_Exceptions; ++i) {
        u32    tagIndex = 0;
        IM3Tag tag;
        u32    અંક_Args;

_       (GetLEB_u32(l, &tagIndex));
        _throwif(m3Err_wasmMalformed, tagIndex >= l->module->અંક_Tags);

        tag     = TagOfIndex(l->module, tagIndex);
        અંક_Args = Get_Func_Type_અંક_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, અંક_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  નહીંતર
    માટે (u32 i = 0; i < l->અંક_Exceptions; ++i) {
        u32 tagIndex = 0;
_       (GetLEB_u32(l, &tagIndex));
    }
#  endif

_catch:
    પાછા result;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
સ્થિર
M3Result LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3Result result = m3Err_none;
    u32      flags  = 0;
    પસંદગી (i_sectionId) {
    સ્થિતિ d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        તોડો;

    સ્થિતિ d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        તોડો;

    સ્થિતિ d_m3SnapshotSection_Table:
_       (LoadTables(l));
        તોડો;

    સ્થિતિ d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        તોડો;

    સ્થિતિ d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        તોડો;

#  જો d_m3HasExceptionHandling
    સ્થિતિ d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        તોડો;
#  endif

    સ્થિતિ d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        તોડો;

    સ્થિતિ d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        તોડો;

    મૂળભૂત:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        તોડો;
    }

_catch:
    પાછા result;
}

સ્થિર
M3Result LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3Result result  = m3Err_none;
    u8*      payload = ખાલી;

    જો (not i_embedded) {
_       (LoadHeader(l));
    }

    જ્યારે (true) {
        u8  sectionId   = 0;
        u32 sectionSize = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        જો (GetU8(l, &sectionId)) {
            તોડો;
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
             l->અંક_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->અંક_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->અંક_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->અંક_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->અંક_Data_Segments or l->module->અંક_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    પાછા result;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#જો d_m3HasSnapshots

સ્થિર
M3Result SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, શૂન્ય* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3Result       result = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, કદ(save));

    IM3Continuation મૂળ = io_runtime->મૂળ_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not મૂળ or મૂળ->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : મૂળ->entryFunction;

    જો (o_module) {
        *o_module = ખાલી;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    જો (o_module) {
        *o_module = save.module;
    }

    માટે (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->next) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  જો d_m3HasExceptionHandling
    માટે (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->next) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    જો (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, મૂળ, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    પાછા result;
}

#endif

M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, શૂન્ય* i_userdata)
{
    જો (not io_runtime or not i_writer) {
        પાછા m3Err_mallocFailed;
    }

#જો d_m3HasSnapshots
    પાછા SaveSnapshot(io_runtime, i_writer, i_userdata, ખાલી, false);
#નહીંતર
    (શૂન્ય)i_userdata;
    પાછા "snapshots are not available in this build of Wasm3";
#endif
}


સ્થિર
M3Result LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, શૂન્ય* i_userdata, bool i_embedded)
{
    M3Result result = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#જો d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, કદ(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        result = LoadSections(&load, i_embedded);

        IM3Continuation મૂળ = io_runtime->મૂળ_Continuation;

        જો (result) {
            જો (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                જો (મૂળ) {
                    મૂળ->state     = cont_returned;
                    મૂળ->અંક_Frames = 0;
                }
            }
        } નહીંતર {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_ઢગલો(io_runtime);

        m3_Free(load.check_ઢગલો);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  જો d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#નહીંતર
    (શૂન્ય)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    પાછા result;
}


M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, શૂન્ય* i_userdata)
{
    પાછા LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


શૂન્ય m3_SetSnapshotHooks (IM3Runtime io_runtime, અચલ M3SnapshotHooks* i_hooks)
{
#જો d_m3HasSnapshots
    જો (io_runtime) {
        જો (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } નહીંતર {
            memset(&io_runtime->snapshotHooks, 0, કદ(io_runtime->snapshotHooks));
        }
    }
#નહીંતર
    (શૂન્ય)io_runtime;
    (શૂન્ય)i_hooks;
#endif
}


M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, શૂન્ય** o_bytes, size_t* o_size)
{
    જો (!io_runtime || !o_bytes || !o_size) {
        પાછા m3Err_mallocFailed;
    }
#જો d_m3HasSnapshots
    M3BufferWriter bw     = { ખાલી, 0, 0 };
    M3Result       result = m3_SaveSnapshot(io_runtime, BufferWriter_Write, &bw);
    જો (result) {
        m3_Free(bw.buffer);
        *o_bytes = ખાલી;
        *o_size  = 0;
        પાછા result;
    }
    *o_bytes = bw.buffer;
    *o_size  = bw.size;
    પાછા m3Err_none;
#નહીંતર
    *o_bytes = ખાલી;
    *o_size  = 0;
    પાછા "snapshots are not available in this build of Wasm3";
#endif
}

#જો d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
સ્થિર
u8* EmitLEB_u32 (u8* o_where, u32 i_કિંમત)
{
    કરો {
        u8 byte = i_કિંમત & 0x7F;
        i_કિંમત >>= 7;
        જો (i_કિંમત != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } જ્યારે (i_કિંમત != 0);

    પાછા o_where;
}

સ્થિર
u32 SizeOfLEB_u32 (u32 i_કિંમત)
{
    u32 size = 0;

    કરો {
        i_કિંમત >>= 7;
        ++size;
    } જ્યારે (i_કિંમત != 0);

    પાછા size;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
સ્થિર
M3Result FindEmbeddedSnapshot (અચલ u8* i_start, અચલ u8* i_end, cstr_t i_secName,
                               અચલ u8** o_secStart, અચલ u8** o_secEnd,
                               અચલ u8** o_data, u32* o_size)
{
    M3Result result = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = ખાલી;

    જો (o_secStart) {
        *o_secStart = ખાલી;
    }
    જો (o_secEnd) {
        *o_secEnd = ખાલી;
    }
    જો (o_data) {
        *o_data = ખાલી;
    }
    જો (o_size) {
        *o_size = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    જ્યારે (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     size      = 0;
_       (ReadLEB_u32(&size, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, size > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += size;
            જો (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                જો (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    જો (o_secStart) {
                        *o_secStart = secStart;
                    }
                    જો (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    જો (o_data) {
                        *o_data = payload;
                    }
                    જો (o_size) {
                        *o_size = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    પાછા result;
}

// Builds the custom section name a snapshot of i_name lives under
સ્થિર
M3Result SnapshotSectionName (cstr_t i_name, અક્ષર* o_secName, size_t i_size)
{
    જો (i_name and *i_name) {
        જો (strlen(i_name) > d_m3MaxSnapshotNameLength) {
            પાછા "the snapshot name is too long";
        }
        snprintf(o_secName, i_size, "snapshot.%s", i_name);
    } નહીંતર {
        snprintf(o_secName, i_size, "snapshot");
    }

    પાછા m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
સ્થિર
M3Result ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3Result result = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ખાલી;

    માટે (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->next) {
        જો (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    પાછા result;
}
#endif

M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, અચલ શૂન્ય* i_bytes, size_t i_size)
{
    જો (!io_runtime || !i_module || !i_bytes) {
        પાછા m3Err_mallocFailed;
    }
#જો d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    જો (i_size >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        અચલ u8* data   = ખાલી;
        u32       size   = 0;
        M3Result  result = FindEmbeddedSnapshot((અચલ u8*)i_bytes, (અચલ u8*)i_bytes + i_size,
                                                "snapshot", ખાલી, ખાલી, &data, &size);
        જો (result) {
            પાછા result;
        }
        જો (not data) {
            પાછા "the Wasm binary holds no embedded snapshot";
        }

        {
            M3BufferReader br = { data, size, 0 };
            પાછા LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
        }
    }
    {
        M3BufferReader br = { (અચલ u8*)i_bytes, i_size, 0 };
        પાછા LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br,
                            i_size < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#નહીંતર
    (શૂન્ય)i_size;
    પાછા "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, અચલ અક્ષર* i_name)
{
    જો (not i_module) {
        પાછા false;
    }
#જો d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ખાલી;
    (શૂન્ય)ModuleSnapshot(i_module, i_name, &snap);

    પાછા snap != ખાલી;
#નહીંતર
    (શૂન્ય)i_name;
    પાછા false;
#endif
}

M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, અચલ અક્ષર* i_name,
                                 અચલ શૂન્ય** o_bytes, size_t* o_size)
{
    જો (not i_module or not o_bytes or not o_size) {
        પાછા m3Err_mallocFailed;
    }

    *o_bytes = ખાલી;
    *o_size  = 0;

#જો d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ખાલી;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);

        જો (result) {
            પાછા result;
        }
        જો (not snap) {
            પાછા "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->data;
        *o_size  = snap->size;

        પાછા m3Err_none;
    }
#નહીંતર
    (શૂન્ય)i_name;
    પાછા "snapshots are not available in this build of Wasm3";
#endif
}

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, અચલ અક્ષર* i_name)
{
    જો (not io_runtime or not i_module) {
        પાછા m3Err_mallocFailed;
    }
#જો d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ખાલી;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);
        M3BufferReader      br;

        જો (result) {
            પાછા result;
        }
        જો (not snap) {
            પાછા "the module holds no embedded snapshot of that name";
        }

        br.buffer = snap->data;
        br.size   = snap->size;
        br.cursor = 0;

        પાછા LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
    }
#નહીંતર
    (શૂન્ય)i_name;
    પાછા "snapshots are not available in this build of Wasm3";
#endif
}


M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, અચલ અક્ષર* i_name,
                                  શૂન્ય** o_bytes, size_t* o_size)
{
#જો d_m3HasSnapshots
    M3Result       result = m3Err_none;
    M3BufferWriter bw     = { ખાલી, 0, 0 };
    u8*            out    = ખાલી;

    અક્ષર      secName[d_m3MaxSnapshotNameLength + કદ("snapshot.")];
    u32       nameLen, nameLenLebSize, payloadSize, payloadSizeLebSize;
    size_t    origSize, totalSize;
    અચલ u8* dropStart = ખાલી;
    અચલ u8* dropEnd   = ખાલી;
    IM3Module saved     = ખાલી;
    u8*       p;

    જો (not io_runtime or not i_module or not o_bytes or not o_size) {
        પાછા m3Err_mallocFailed;
    }

    *o_bytes = ખાલી;
    *o_size  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, કદ(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->મૂળ_Continuation or io_runtime->મૂળ_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, BufferWriter_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.size);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.size > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, ખાલી, ખાલી));

    origSize           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    nameLenLebSize     = SizeOfLEB_u32(nameLen);
    payloadSize        = nameLenLebSize + nameLen + (u32)bw.size;
    payloadSizeLebSize = SizeOfLEB_u32(payloadSize);
    totalSize          = origSize + 1 + payloadSizeLebSize + payloadSize;

    out = (u8*)m3_Malloc("wasm_snapshot_module", totalSize);
    _throwifnull(out);

    p = out;

    જો (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } નહીંતર {
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
    out      = ખાલી;

_catch:
    m3_Free(bw.buffer);
    m3_Free(out);

    પાછા result;
#નહીંતર
    (શૂન્ય)io_runtime;
    (શૂન્ય)i_module;
    (શૂન્ય)i_name;
    (શૂન્ય)o_bytes;
    (શૂન્ય)o_size;
    પાછા "snapshots are not available in this build of Wasm3";
#endif
}
