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

#அந்தர்பாவ "m3_env.h"
#அந்தர்பாவ "m3_compile.h"
#அந்தர்பாவ "m3_exception.h"
#அந்தர்பாவ "m3_host.h"

#அந்தர்பாவ <limits.h>

#எனில் d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
நிலையான மாறிலி u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
நிலையான மாறிலி u32 c_snapshotVersion  = 1;

எண்ணிக்கை {
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
எண்ணிக்கை {
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
நிலையான
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

        போது (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     size      = 0;

_           (ReadLEB_u32(&size, &pos, end));
            _throwif(m3Err_wasmUnderrun, size > (size_t)(end - pos));

            pos += size;
            எனில் (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    திரும்பு result;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

நிலையான
மாறிலி M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    மாறிலி M3SnapshotMap* map = i_function->snapshotMap;

    எனில் (map) {
        க்கு (u32 i = 0; i < map->இலக்கம்_Safe_Points; ++i) {
            எனில் (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                திரும்பு &map->safePoints[i];
            }
        }
    }

    திரும்பு வெற்று;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
நிலையான
மாறிலி M3SafePoint* FindSafePointAt (மாறிலி M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 கீழ்  = 0;
    u32 உயர்வு = i_map->இலக்கம்_Safe_Points;

    போது (கீழ் < உயர்வு) {
        u32 நடு = கீழ் + (உயர்வு - கீழ்) / 2;

        எனில் (i_map->safePoints[நடு].wasmOffset < i_wasmOffset) {
            கீழ் = நடு + 1;
        } இல்லையெனில் {
            உயர்வு = நடு;
        }
    }

    க்கு (u32 i = கீழ்; i < i_map->இலக்கம்_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        மாறிலி M3SafePoint* point = &i_map->safePoints[i];

        எனில் (point->kind != i_kind or (point->flags & d_m3SafePointTailCall)) {
            தொடர்;
        }
        எனில் (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            திரும்பு point;
        }
    }

    திரும்பு வெற்று;
}

நிலையான
மாறிலி M3BlockStart* FindBlockAt (மாறிலி M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    க்கு (u32 i = 0; i < i_map->இலக்கம்_Blocks; ++i) {
        எனில் (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            திரும்பு &i_map->blocks[i];
        }
    }

    திரும்பு வெற்று;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
நிலையான உள்வரி
bool EnclosesOffset (மாறிலி M3BlockStart* i_block, u32 i_wasmOffset)
{
    திரும்பு i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
நிலையான உள்வரி
bool HandlersLiveAt (மாறிலி M3SnapshotMap* i_map, மாறிலி M3SafePoint* i_point, மாறிலி M3BlockStart* i_try)
{
    திரும்பு not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
நிலையான
M3Result CallSiteType (IM3Function i_caller, மாறிலி M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3Result   result = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        size   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = வெற்று;

_   (ReadLEB_u32(&size, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    எனில் (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->இலக்கம்_Functions);
        *o_type = module->functions[index].funcType;
    } இல்லையெனில் {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->இலக்கம்_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    திரும்பு result;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
நிலையான
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 இலக்கம்_Results = Get_Func_Type_இலக்கம்_Results(i_expected);

    எனில் (not i_type or Get_Func_Type_இலக்கம்_Results(i_type) != இலக்கம்_Results) {
        திரும்பு false;
    }

    க்கு (u16 i = 0; i < இலக்கம்_Results; ++i) {
        எனில் (not IsSubTypeOf(GetFuncTypeResultType(i_type, i), GetFuncTypeResultType(i_expected, i))) {
            திரும்பு false;
        }
    }

    திரும்பு true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
வகைவரை கட்டமைப்பு M3FrameRegisters {
    m3reg_t* r0;
#  எனில் d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
நிலையான
M3Result Continuation_அடுக்கு (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_இலக்கம்_Slots)
{
    எனில் (i_cont == i_runtime->வேர்_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_அடுக்கு;
        *o_இலக்கம்_Slots = i_runtime->இலக்கம்_அடுக்கு_Slots;
    } இல்லையெனில் {
        *o_base     = i_cont->val_அடுக்கு;
        *o_இலக்கம்_Slots = i_cont->இலக்கம்_அடுக்கு_Slots;
    }

    திரும்பு (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
நிலையான
M3Result Locate_மதிப்பு (m3slot_t* i_base, u32 i_இலக்கம்_Slots, m3stack_t i_sp, மாறிலி M3_Slot_மதிப்பு* i_மதிப்பு, u8** o_where)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_மதிப்பு->type);

    *o_where = வெற்று;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_மதிப்பு->slot));

    எனில் (not IsRegisterSlotAlias(i_மதிப்பு->slot)) {
        size_t size   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? அளவு(வெறுமை*)
                                                                   : (Is64BitType(type) ? அளவு(u64) : அளவு(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_மதிப்பு->slot) * அளவு(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + size > (size_t)i_இலக்கம்_Slots * அளவு(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    திரும்பு result;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
நிலையான
M3Result Read_மதிப்பு (u8* i_where, மாறிலி M3FrameRegisters* i_registers, மாறிலி M3_Slot_மதிப்பு* i_மதிப்பு, u64* o_bits,
                    வெறுமை** o_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_மதிப்பு->type);

    *o_bits      = 0;
    *o_reference = வெற்று;

    எனில் (i_where) {
        எனில் (IsRefType(type)) {
            memcpy(o_reference, i_where, அளவு(வெறுமை*));
        } இல்லையெனில் எனில் (Is64BitType(type)) {
            memcpy(o_bits, i_where, அளவு(u64));
        } இல்லையெனில் {
            u32 narrow;
            memcpy(&narrow, i_where, அளவு(narrow));
            *o_bits = narrow;
        }
    } இல்லையெனில் எனில் (IsFpRegisterSlotAlias(i_மதிப்பு->slot)) {
#  எனில் d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        எனில் (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, அளவு(bits));
            *o_bits = bits;
        } இல்லையெனில் {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, அளவு(u64));
        }
#  இல்லையெனில்
        _throw(m3Err_wasmMalformed);
#  endif
    } இல்லையெனில் {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        எனில் (IsRefType(type)) {
            *o_reference = (வெறுமை*)(uintptr_t)r0;
        } இல்லையெனில் எனில் (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } இல்லையெனில் {
            *o_bits = (u32)r0;
        }
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result Write_மதிப்பு (u8* o_where, மாறிலி M3FrameRegisters* i_registers, மாறிலி M3_Slot_மதிப்பு* i_மதிப்பு, u64 i_bits,
                     வெறுமை* i_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_மதிப்பு->type);

    எனில் (o_where) {
        எனில் (IsRefType(type)) {
            memcpy(o_where, &i_reference, அளவு(வெறுமை*));
        } இல்லையெனில் எனில் (Is64BitType(type)) {
            memcpy(o_where, &i_bits, அளவு(u64));
        } இல்லையெனில் {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, அளவு(narrow));
        }
    } இல்லையெனில் எனில் (IsFpRegisterSlotAlias(i_மதிப்பு->slot)) {
#  எனில் d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        எனில் (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, அளவு(narrow));
            *i_registers->fp0 = narrow;
        } இல்லையெனில் {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, அளவு(u64));
        }
#  இல்லையெனில்
        _throw(m3Err_wasmMalformed);
#  endif
    } இல்லையெனில் {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        எனில் (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } இல்லையெனில் {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    திரும்பு result;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
நிலையான
மாறிலி M3_Slot_மதிப்பு* Frame_மதிப்பு (மாறிலி M3SnapshotMap* i_map, மாறிலி M3SafePoint* i_point, u32 i_index)
{
    எனில் (i_index < i_map->இலக்கம்_Locals) {
        திரும்பு i_point->first_Local_மதிப்பு == UINT32_MAX ? &i_map->locals[i_index]
                                                      : &i_map->மதிப்புகள்[i_point->first_Local_மதிப்பு + i_index];
    }
    திரும்பு &i_map->மதிப்புகள்[i_point->first_மதிப்பு + i_index - i_map->இலக்கம்_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
நிலையான
வெறுமை RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    எனில் (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->இலக்கம்_Ret_And_Arg_Slots) + i_function->இலக்கம்_Local_Bytes;

        memcpy(where, i_function->constants, i_function->இலக்கம்_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
வகைவரை கட்டமைப்பு M3PointerIds {
    வெறுமை** பொருட்கள்;
    u32    மொத்தம்;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  indexCapacity;       // a power of two, kept at least twice count
} M3PointerIds;

நிலையான
u32 PointerBucket (வெறுமை* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    திரும்பு (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

நிலையான
u32 PointerIds_Find (மாறிலி M3PointerIds* i_ids, வெறுமை* i_pointer)
{
    எனில் (i_ids->indexCapacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->indexCapacity);

        போது (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            எனில் (i_ids->பொருட்கள்[id] == i_pointer) {
                திரும்பு id;
            }

            bucket = (bucket + 1) & (i_ids->indexCapacity - 1);
        }
    }

    திரும்பு d_m3SnapshotNone;
}

நிலையான
M3Result PointerIds_Add (M3PointerIds* io_ids, வெறுமை* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    எனில் (id != d_m3SnapshotNone) {
        *o_id = id;
        திரும்பு m3Err_none;
    }

    எனில் (io_ids->மொத்தம் == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        வெறுமை** பொருட்கள்    = m3_ReallocArray(வெறுமை*, io_ids->பொருட்கள், capacity, io_ids->capacity);

        எனில் (not பொருட்கள்) {
            திரும்பு m3Err_mallocFailed;
        }

        io_ids->பொருட்கள்    = பொருட்கள்;
        io_ids->capacity = capacity;
    }

    எனில் ((io_ids->மொத்தம் + 1) * 2 > io_ids->indexCapacity) {
        u32  capacity = io_ids->indexCapacity ? io_ids->indexCapacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        எனில் (not index) {
            திரும்பு m3Err_mallocFailed;
        }

        க்கு (u32 i = 0; i < io_ids->மொத்தம்; ++i) {
            u32 bucket = PointerBucket(io_ids->பொருட்கள்[i], capacity);

            போது (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->indexCapacity = capacity;
    }

    id                = io_ids->மொத்தம்++;
    io_ids->பொருட்கள்[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->indexCapacity);

    போது (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->indexCapacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    திரும்பு m3Err_none;
}

நிலையான
வெறுமை PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->பொருட்கள்);
    m3_Free(io_ids->index);
    memset(io_ids, 0, அளவு(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

வகைவரை கட்டமைப்பு M3BufferWriter {
    u8*    buffer;
    size_t size;
    size_t capacity;
} M3BufferWriter;

நிலையான
M3Result BufferWriter_Write (மாறிலி வெறுமை* i_data, size_t i_size, வெறுமை* i_userdata)
{
    M3BufferWriter* bw = (M3BufferWriter*)i_userdata;

    எனில் (bw->size + i_size > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        போது (bw->size + i_size > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->buffer, newCap, bw->capacity);
        எனில் (!newBuf) {
            திரும்பு m3Err_mallocFailed;
        }
        bw->buffer   = newBuf;
        bw->capacity = newCap;
    }

    எனில் (i_size) {
        memcpy(bw->buffer + bw->size, i_data, i_size);
        bw->size += i_size;
    }

    திரும்பு m3Err_none;
}

வகைவரை கட்டமைப்பு M3BufferReader {
    மாறிலி u8* buffer;
    size_t    size;
    size_t    cursor;
} M3BufferReader;

நிலையான
M3Result BufferReader_Read (வெறுமை* o_buffer, size_t i_size, வெறுமை* i_userdata)
{
    M3BufferReader* br = (M3BufferReader*)i_userdata;

    எனில் (i_size > br->size - br->cursor) {
        திரும்பு m3Err_wasmMalformed;
    }

    எனில் (i_size) {
        memcpy(o_buffer, br->buffer + br->cursor, i_size);
        br->cursor += i_size;
    }

    திரும்பு m3Err_none;
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
வகைவரை கட்டமைப்பு M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    வெறுமை*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

நிலையான உள்வரி
u8 M3TypeToValType (m3type_t i_type)
{
    தேர்வு (BaseTypeOf(i_type)) {
    நிலை c_m3Type_i32: திரும்பு 0x7F;
    நிலை c_m3Type_i64: திரும்பு 0x7E;
    நிலை c_m3Type_f32: திரும்பு 0x7D;
    நிலை c_m3Type_f64: திரும்பு 0x7C;
    நிலை c_m3Type_v128: திரும்பு 0x7B;
    நிலை c_m3Type_funcref: திரும்பு 0x70;
    நிலை c_m3Type_externref: திரும்பு 0x6F;
    நிலை c_m3Type_exnref: திரும்பு 0x69;
    நிலை c_m3Type_contref: திரும்பு 0x68;
    இயல்பு: திரும்பு 0x00;
    }
}

நிலையான உள்வரி
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    தேர்வு (i_valtype) {
    நிலை 0x7F: திரும்பு c_m3Type_i32;
    நிலை 0x7E: திரும்பு c_m3Type_i64;
    நிலை 0x7D: திரும்பு c_m3Type_f32;
    நிலை 0x7C: திரும்பு c_m3Type_f64;
    நிலை 0x7B: திரும்பு c_m3Type_v128;
    நிலை 0x70: திரும்பு c_m3Type_funcref;
    நிலை 0x6F: திரும்பு c_m3Type_externref;
    நிலை 0x69: திரும்பு c_m3Type_exnref;
    நிலை 0x68: திரும்பு c_m3Type_contref;
    இயல்பு: திரும்பு c_m3Type_none;
    }
}

நிலையான
M3Result Put (M3SnapshotSave* s, மாறிலி வெறுமை* i_data, size_t i_size)
{
    திரும்பு s->writing ? s->writer(i_data, i_size, s->userdata) : m3Err_none;
}

நிலையான
M3Result PutU8 (M3SnapshotSave* s, u8 i_மதிப்பு)
{
    திரும்பு Put(s, &i_மதிப்பு, அளவு(i_மதிப்பு));
}


நிலையான
M3Result PutLEB_u32 (M3SnapshotSave* s, u32 i_மதிப்பு)
{
    u8  bytes[5];
    u32 len = 0;

    செய் {
        u8 byte = i_மதிப்பு & 0x7F;
        i_மதிப்பு >>= 7;
        எனில் (i_மதிப்பு != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } போது (i_மதிப்பு != 0);

    திரும்பு Put(s, bytes, len);
}

நிலையான
M3Result PutLEB_u64 (M3SnapshotSave* s, u64 i_மதிப்பு)
{
    u8  bytes[10];
    u32 len = 0;

    செய் {
        u8 byte = i_மதிப்பு & 0x7F;
        i_மதிப்பு >>= 7;
        எனில் (i_மதிப்பு != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } போது (i_மதிப்பு != 0);

    திரும்பு Put(s, bytes, len);
}

நிலையான
M3Result PutLEB_i64 (M3SnapshotSave* s, i64 i_மதிப்பு)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    போது (more) {
        u8 byte = i_மதிப்பு & 0x7F;
        i_மதிப்பு >>= 7;
        எனில் ((i_மதிப்பு == 0 && !(byte & 0x40)) || (i_மதிப்பு == -1 && (byte & 0x40))) {
            more = false;
        } இல்லையெனில் {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    திரும்பு Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
வகைவரை M3Result (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
நிலையான
M3Result SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3Result         result   = m3Err_none;
    M3BufferWriter   bw       = { வெற்று, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    வெறுமை*            userdata = s->userdata;

    s->writer   = BufferWriter_Write;
    s->userdata = &bw;

    result = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    எனில் (result) {
        செல் _catch;
    }

    // a section with nothing in it is left out
    எனில் (bw.size) {
        _throwif("snapshot section is too large", bw.size > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.size));
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    திரும்பு result;
}

நிலையான
u32 FunctionIndex (IM3Module i_module, IM3Function i_function)
{
    எனில் (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->இலக்கம்_Functions) {
        திரும்பு (u32)(i_function - i_module->functions);
    }

    திரும்பு d_m3SnapshotNone;
}

நிலையான
u32 FuncTypeIndex (IM3Module i_module, IM3FuncType i_type)
{
    க்கு (u32 i = 0; i < i_module->இலக்கம்_Func_Types; ++i) {
        எனில் (i_module->funcTypes[i] == i_type) {
            திரும்பு i;
        }
    }

    திரும்பு d_m3SnapshotNone;
}

#  எனில் d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
நிலையான
IM3Tag TagOfIndex (IM3Module i_module, u32 i_index)
{
    IM3Tag tag = &i_module->tags[i_index];

    திரும்பு tag->resolved ? tag->resolved : tag;
}

நிலையான
u32 TagIndex (IM3Module i_module, IM3Tag i_tag)
{
    க்கு (u32 i = 0; i < i_module->இலக்கம்_Tags; ++i) {
        எனில் (TagOfIndex(i_module, i) == i_tag) {
            திரும்பு i;
        }
    }

    திரும்பு d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
நிலையான
M3Result NameReference (M3SnapshotSave* s, u8 i_type, வெறுமை* i_reference, u64* o_word)
{
    M3Result result = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    எனில் (not i_reference) {
        திரும்பு m3Err_none;
    }

    எனில் (i_type == c_m3Type_funcref) {
        id = FunctionIndex(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } இல்லையெனில் எனில் (i_type == c_m3Type_externref) {
        மாறிலி M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        செல் _catch;
    }
#  எனில் d_m3HasExceptionHandling
    இல்லையெனில் எனில் (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 மொத்தம் = s->exceptions.மொத்தம்;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.மொத்தம் != மொத்தம்);
    }
#  endif
    இல்லையெனில் எனில் (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 மொத்தம் = s->continuations.மொத்தம்;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.மொத்தம் != மொத்தம்);
    } இல்லையெனில் {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    திரும்பு result;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
நிலையான
M3Result Put_Ref_மதிப்பு (M3SnapshotSave* s, u8 i_type, வெறுமை* i_reference)
{
    M3Result result = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    எனில் (not s->writing) {
        திரும்பு m3Err_none;
    }

    எனில் (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } இல்லையெனில் {
        u8 kind = 0;
        எனில் (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } இல்லையெனில் எனில் (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  எனில் d_m3HasExceptionHandling
        இல்லையெனில் எனில் (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        இல்லையெனில் எனில் (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    திரும்பு result;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
நிலையான
M3Result PutLE32 (M3SnapshotSave* s, u32 i_மதிப்பு)
{
    u8 bytes[4];

    க்கு (u32 i = 0; i < அளவு(bytes); ++i) {
        bytes[i] = (u8)(i_மதிப்பு >> (8 * i));
    }

    திரும்பு Put(s, bytes, அளவு(bytes));
}

நிலையான
M3Result PutLE64 (M3SnapshotSave* s, u64 i_மதிப்பு)
{
    u8 bytes[8];

    க்கு (u32 i = 0; i < அளவு(bytes); ++i) {
        bytes[i] = (u8)(i_மதிப்பு >> (8 * i));
    }

    திரும்பு Put(s, bytes, அளவு(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
நிலையான
M3Result Put_மதிப்பு_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    எனில் (IsRefType(i_type)) {
        திரும்பு Put_Ref_மதிப்பு(s, BaseTypeOf(i_type), (வெறுமை*)(uintptr_t)i_bits);
    }

    தேர்வு (BaseTypeOf(i_type)) {
    நிலை c_m3Type_i32: திரும்பு PutLEB_i64(s, (i32)(u32)i_bits);
    நிலை c_m3Type_i64: திரும்பு PutLEB_i64(s, (i64)i_bits);
    நிலை c_m3Type_f32: திரும்பு PutLE32(s, (u32)i_bits);
    நிலை c_m3Type_f64: திரும்பு PutLE64(s, i_bits);
    இயல்பு: திரும்பு "a snapshot cannot hold a value of this type as bits";
    }
}

நிலையான
M3Result Put_மதிப்பு_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    எனில் (not valtype) {
        திரும்பு "a snapshot has no tag for a value of this type";
    }

    திரும்பு PutU8(s, valtype);
}

நிலையான
M3Result Put_மதிப்பு_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3Result result = m3Err_none;

_   (Put_மதிப்பு_Tag(s, i_type));
_   (Put_மதிப்பு_Body(s, i_type, i_bits));

_catch:
    திரும்பு result;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
நிலையான
M3Result Put_மதிப்பு (M3SnapshotSave* s, m3type_t i_type, மாறிலி வெறுமை* i_where)
{
    M3Result result = m3Err_none;

_   (Put_மதிப்பு_Tag(s, i_type));

    எனில் (IsRefType(i_type)) {
        வெறுமை* reference;
        memcpy(&reference, i_where, அளவு(reference));
_       (Put_மதிப்பு_Body(s, i_type, (u64)(uintptr_t)reference));
    } இல்லையெனில் எனில் (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, அளவு(bytes));
_       (Put(s, bytes, அளவு(bytes)));
    } இல்லையெனில் எனில் (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, அளவு(bits));
_       (Put_மதிப்பு_Body(s, i_type, bits));
    } இல்லையெனில் {
        u32 narrow;
        memcpy(&narrow, i_where, அளவு(narrow));
_       (Put_மதிப்பு_Body(s, i_type, narrow));
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result PutMemoryChunks (M3SnapshotSave* s, மாறிலி u8* i_bytes, size_t i_size)
{
    M3Result result = m3Err_none;
    size_t   cursor = 0;

    போது (cursor < i_size) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        போது (cursor + run < i_size and i_bytes[cursor + run] == byte) {
            run++;
        }

        எனில் ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            எனில் (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            தொடர்;
        }

        size_t rawStart = cursor;

        போது (cursor < i_size) {
            u8     b      = i_bytes[cursor];
            size_t length = 1;

            போது (cursor + length < i_size and i_bytes[cursor + length] == b) {
                length++;
            }

            எனில் ((b == 0x00 or b == 0xFF) and length >= d_m3SnapshotRunThreshold) {
                நிறுத்து;
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
    திரும்பு result;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
நிலையான
u32 Memory_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    போது (module->memories[j] != module->memories[index]) {
        ++j;
    }
    திரும்பு j;
}

நிலையான
u32 Table_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    போது (module->tables[j] != module->tables[index]) {
        ++j;
    }
    திரும்பு j;
}

நிலையான
M3Result SaveMemories (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->இலக்கம்_Memories));

    க்கு (u32 m = 0; m < module->இலக்கம்_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_FirstIndex(module, m);
        bool      hasData  = memory and memory->mallocated;
        u32       pageSize = memory ? Memory_PageSize(memory) : d_m3DefaultMemPageSize;
        u64       bytes    = memory ? memory->இலக்கம்_Pages * (u64)pageSize : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        எனில் (first != m) {
            தொடர்;
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
        போது ((1u << pageBits) < pageSize) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->இலக்கம்_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        எனில் (not s->writing) {
            தொடர்;
        }

        எனில் (hasData) {
_           (PutMemoryChunks(s, m3MemData(memory->mallocated), (size_t)bytes));
        } இல்லையெனில் {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result SaveGlobals (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->இலக்கம்_Globals));

    க்கு (u32 g = 0; g < module->இலக்கம்_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_மதிப்பு(s, global->type, &cell->i64_மதிப்பு));
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result SaveTables (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->இலக்கம்_Tables));

    க்கு (u32 t = 0; t < module->இலக்கம்_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_FirstIndex(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        எனில் (first != t) {
            தொடர்;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->size));

        க்கு (u32 e = 0; e < table->size; ++e) {
_           (Put_Ref_மதிப்பு(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result SaveSegments (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->இலக்கம்_Data_Segments));

    க்கு (u32 i = 0; i < module->இலக்கம்_Data_Segments; ++i) {
_       (PutU8(s, module->dataSegments[i].dropped));
    }

_   (PutLEB_u32(s, module->இலக்கம்_Element_Segments));

    க்கு (u32 i = 0; i < module->இலக்கம்_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    திரும்பு result;
}

#  எனில் d_m3HasExceptionHandling

நிலையான
M3Result SaveExceptions (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.மொத்தம்));

    க்கு (u32 i = 0; i < s->exceptions.மொத்தம்; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.பொருட்கள்[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->இலக்கம்_Args != Get_Func_Type_இலக்கம்_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->இலக்கம்_Args));

        க்கு (u32 a = 0; a < exn->இலக்கம்_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_மதிப்பு_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    திரும்பு result;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
வகைவரை கட்டமைப்பு M3Activation {
    IM3Function        function;
    மாறிலி M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    மாறிலி M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
நிலையான
மாறிலி M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    மாறிலி M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : வெற்று;

    க்கு (u32 i = 0; not point and i < i_module->இலக்கம்_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        எனில் (point) {
            *io_function = &i_module->functions[i];
        }
    }

    திரும்பு point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
நிலையான
M3Result NextActivation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3Result result = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, அளவு(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    க்கு (; i >= 0; --i) {
        மாறிலி M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        எனில் (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  எனில் d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            நிறுத்து;
        } இல்லையெனில் எனில் (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            நிறுத்து;
        }
    }

    எனில் (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  எனில் d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  எனில் d_m3EntryKeepsFrame
    க்கு (i32 k = i_first; k > o_act->lastFrame; --k) {
        மாறிலி M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    திரும்பு result;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
நிலையான
M3Result SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_இலக்கம்_Slots, மாறிலி M3Activation* i_act)
{
    M3Result             result        = m3Err_none;
    IM3Function          function      = i_act->function;
    மாறிலி M3SnapshotMap* map           = function->snapshotMap;
    u32                  functionIndex = FunctionIndex(s->module, function);
    மாறிலி M3SafePoint*   point         = i_act->point;
    u32                  இலக்கம்_மதிப்புகள்;

    _throwif("a suspended frame belongs to another module", functionIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, functionIndex));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    எனில் (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    இலக்கம்_மதிப்புகள் = map->இலக்கம்_Locals + point->இலக்கம்_மதிப்புகள்;

_   (PutLEB_u32(s, இலக்கம்_மதிப்புகள்));

    க்கு (u32 v = 0; v < இலக்கம்_மதிப்புகள்; ++v) {
        மாறிலி M3_Slot_மதிப்பு* மதிப்பு = Frame_மதிப்பு(map, point, v);
        u8*                where;
        u64                word;
        வெறுமை*              reference;

_       (Locate_மதிப்பு(i_base, i_இலக்கம்_Slots, i_act->sp, மதிப்பு, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        எனில் (BaseTypeOf(மதிப்பு->type) == c_m3Type_v128) {
_           (Put_மதிப்பு_Tag(s, மதிப்பு->type));
_           (Put(s, where, 16));
            தொடர்;
        }

_       (Read_மதிப்பு(where, &i_act->registers, மதிப்பு, &word, &reference));

_       (Put_மதிப்பு_Bits(s, மதிப்பு->type, IsRefType(மதிப்பு->type) ? (u64)(uintptr_t)reference : word));
    }

    எனில் (point->kind == safepoint_resume) {
_       (Put_Ref_மதிப்பு(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    திரும்பு result;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
நிலையான
M3Result SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3Result      result   = m3Err_none;
    M3Activation* acts     = வெற்று;
    u32           இலக்கம்_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->இலக்கம்_Frames - 1;
    m3slot_t*     base;
    u32           இலக்கம்_Slots;
    m3stack_t     sp;

_   (Continuation_அடுக்கு(s->runtime, i_cont, &base, &இலக்கம்_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->இலக்கம்_Frames + 1);
    _throwifnull(acts);

    sp = base;

    போது (true) {
        M3Activation* act = &acts[இலக்கம்_Acts];

_       (NextActivation(s->module, i_cont, function, sp, first, act));

        எனில் (act->kind != safepoint_call) {
            இலக்கம்_Acts++;
            நிறுத்து;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        எனில் (not (act->point->flags & d_m3SafePointTailCall)) {
            இலக்கம்_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, இலக்கம்_Acts));

    க்கு (u32 a = 0; a < இலக்கம்_Acts; ++a) {
_       (SaveActivation(s, base, இலக்கம்_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    திரும்பு result;
}

நிலையான
M3Result SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3Result  result    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_வேர்    = (i_cont == s->runtime->வேர்_Continuation);
    u8        state     = snapshot_contFinished;
    u32       typeIndex = is_வேர் ? d_m3SnapshotNone : FuncTypeIndex(module, i_cont->type);

    எனில் (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } இல்லையெனில் எனில் (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } இல்லையெனில் எனில் (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", FunctionIndex(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_வேர் and typeIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_வேர்));
_   (PutLEB_u32(s, typeIndex));
_   (PutLEB_u32(s, FunctionIndex(module, i_cont->entryFunction)));
#  எனில் d_m3HasExceptionHandling
_   (Put_Ref_மதிப்பு(s, c_m3Type_exnref, i_cont->resumeThrow));
#  இல்லையெனில்
_   (Put_Ref_மதிப்பு(s, c_m3Type_exnref, வெற்று));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    எனில் (s->postmortem) {
        செல் _catch;
    }

    எனில் (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : வெற்று;
        u16         இலக்கம்_Rets = inner ? Get_Func_Type_இலக்கம்_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_மொத்தம் > Get_Func_Type_இலக்கம்_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_மொத்தம்));

        க்கு (u32 i = 0; i < i_cont->bound_Args_மொத்தம்; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_மதிப்பு(s, type, i_cont->val_அடுக்கு + (இலக்கம்_Rets + i) * c_io_Slot_மொத்தம்));
        }
    } இல்லையெனில் எனில் (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_மொத்தம்));
_       (SaveSuspendedBody(s, i_cont));
    } இல்லையெனில் {
_       (PutLEB_u32(s, 0));
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result SaveContinuations (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->continuations.மொத்தம்));

    க்கு (u32 i = 0; i < s->continuations.மொத்தம்; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.பொருட்கள்[i]));
    }

_catch:
    திரும்பு result;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
நிலையான
M3Result SaveHostState (M3SnapshotSave* s)
{
    M3Result               result = m3Err_none;
    மாறிலி M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3BufferWriter         bw     = { வெற்று, 0, 0 };

    எனில் (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, BufferWriter_Write, &bw));
    }

    எனில் (bw.size > 0) {
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    திரும்பு result;
}

நிலையான
வெறுமை SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    க்கு (u32 i = 0; i < module->இலக்கம்_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        எனில் (Memory_FirstIndex(module, i) == i and memory) {
            *memoryBytes += memory->இலக்கம்_Pages * (u64)Memory_PageSize(memory);
        }
    }
    க்கு (u32 i = 0; i < module->இலக்கம்_Tables; ++i) {
        IM3Table table = module->tables[i];
        எனில் (Table_FirstIndex(module, i) == i and table) {
            *tableElements += table->size;
        }
    }
}

நிலையான
M3Result SaveMeta (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    க்கு (u32 i = 0; i < s->continuations.மொத்தம்; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.பொருட்கள்[i];
        எனில் (cont != s->runtime->வேர்_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.மொத்தம்));
_   (PutLEB_u32(s, s->exceptions.மொத்தம்));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  எனில் d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    க்கு (u32 i = 0; i < s->exceptions.மொத்தம்; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.பொருட்கள்[i];
        u32          tagIndex  = TagIndex(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tagIndex == d_m3SnapshotNone);

_       (PutLEB_u32(s, tagIndex));
    }
#  endif

_catch:
    திரும்பு result;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
நிலையான
M3Result DiscoverReferences (M3SnapshotSave* s)
{
    M3Result result            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    போது (doneContinuations < s->continuations.மொத்தம் or doneExceptions < s->exceptions.மொத்தம்) {
#  எனில் d_m3HasExceptionHandling
        போது (doneExceptions < s->exceptions.மொத்தம்) {
            M3Exception* exception = (M3Exception*)s->exceptions.பொருட்கள்[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->இலக்கம்_Args != Get_Func_Type_இலக்கம்_Params(type));

            க்கு (u32 a = 0; a < exception->இலக்கம்_Args; ++a) {
_               (Put_மதிப்பு_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        போது (doneContinuations < s->continuations.மொத்தம் and doneExceptions == s->exceptions.மொத்தம்) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.பொருட்கள்[id]));
        }
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result SaveSections (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

    எனில் (not s->writing) {
        திரும்பு DiscoverReferences(s);
    }

    எனில் (not s->embedded) {
_       (Put(s, c_snapshotMagic, அளவு(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    எனில் (s->module->இலக்கம்_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    எனில் (s->module->இலக்கம்_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    எனில் (s->module->இலக்கம்_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    எனில் (s->module->இலக்கம்_Data_Segments or s->module->இலக்கம்_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  எனில் d_m3HasExceptionHandling
    எனில் (s->exceptions.மொத்தம்) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    எனில் (s->continuations.மொத்தம்) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    திரும்பு result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

வகைவரை கட்டமைப்பு M3SnapshotSuspendSite {
    மாறிலி M3SnapshotMap* map;
    மாறிலி M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

வகைவரை கட்டமைப்பு M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    வெறுமை*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_அடுக்கு;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    இலக்கம்_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  எனில் d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 இலக்கம்_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

நிலையான
M3Result Get (M3SnapshotLoad* l, வெறுமை* o_data, size_t i_size)
{
    எனில் (not l->inSection) {
        திரும்பு l->reader(o_data, i_size, l->userdata);
    }
    எனில் (i_size > (size_t)(l->end - l->pos)) {
        திரும்பு m3Err_wasmUnderrun;
    }
    எனில் (i_size) {
        memcpy(o_data, l->pos, i_size);
        l->pos += i_size;
    }
    திரும்பு m3Err_none;
}

நிலையான
M3Result GetU8 (M3SnapshotLoad* l, u8* o_மதிப்பு)
{
    திரும்பு Get(l, o_மதிப்பு, அளவு(*o_மதிப்பு));
}

நிலையான
M3Result GetBool (M3SnapshotLoad* l, u8* o_மதிப்பு)
{
    M3Result result = GetU8(l, o_மதிப்பு);
    திரும்பு result ? result : (*o_மதிப்பு <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

நிலையான
M3Result GetLEB_u32 (M3SnapshotLoad* l, u32* o_மதிப்பு)
{
    எனில் (l->inSection) {
        திரும்பு ReadLEB_u32(o_மதிப்பு, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    க்கு (u32 i = 0; i < அளவு(encoded); ++i) {
        M3Result result = GetU8(l, &encoded[i]);
        எனில் (result) {
            திரும்பு result;
        }
        எனில் (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            திரும்பு ReadLEB_u32(o_மதிப்பு, &pos, encoded + i + 1);
        }
    }
    திரும்பு m3Err_lebOverflow;
}

நிலையான
M3Result GetLEB_u64 (M3SnapshotLoad* l, u64* o_மதிப்பு)
{
    திரும்பு ReadLebUnsigned(o_மதிப்பு, 64, &l->pos, l->end);
}

நிலையான
M3Result GetLEB_i64 (M3SnapshotLoad* l, i64* o_மதிப்பு)
{
    திரும்பு ReadLEB_i64(o_மதிப்பு, &l->pos, l->end);
}

நிலையான
M3Result SkipBytes (M3SnapshotLoad* l, size_t i_size)
{
    எனில் (i_size > (size_t)(l->end - l->pos)) {
        திரும்பு m3Err_wasmUnderrun;
    }
    l->pos += i_size;
    திரும்பு m3Err_none;
}


நிலையான
M3Result Get_Ref_மதிப்பு (M3SnapshotLoad* l, m3type_t i_type, வெறுமை** o_reference)
{
    M3Result result = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = வெற்று;

_   (GetU8(l, &kind));
    எனில் (kind == 0x00) {
        திரும்பு (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    எனில் (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->இலக்கம்_Functions);
#  எனில் d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } இல்லையெனில் எனில் (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        மாறிலி M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        எனில் (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  எனில் d_m3HasExceptionHandling
    இல்லையெனில் எனில் (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->இலக்கம்_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    இல்லையெனில் எனில் (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->இலக்கம்_Continuations);
        *o_reference = l->continuations[id];
#  எனில் d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        எனில் (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } இல்லையெனில் {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result GetLE32 (M3SnapshotLoad* l, u32* o_மதிப்பு)
{
    u8       bytes[4];
    M3Result result = Get(l, bytes, அளவு(bytes));

    *o_மதிப்பு = 0;

    க்கு (u32 i = 0; i < அளவு(bytes); ++i) {
        *o_மதிப்பு |= (u32)bytes[i] << (8 * i);
    }

    திரும்பு result;
}

நிலையான
M3Result GetLE64 (M3SnapshotLoad* l, u64* o_மதிப்பு)
{
    u8       bytes[8];
    M3Result result = Get(l, bytes, அளவு(bytes));

    *o_மதிப்பு = 0;

    க்கு (u32 i = 0; i < அளவு(bytes); ++i) {
        *o_மதிப்பு |= (u64)bytes[i] << (8 * i);
    }

    திரும்பு result;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
நிலையான
M3Result Get_மதிப்பு_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3Result result  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    எனில் (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    திரும்பு result;
}

// The mirror of PutValueBody: narrow types come back in the low bits
நிலையான
M3Result Get_மதிப்பு_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3Result result = m3Err_none;

    *o_bits = 0;

    எனில் (IsRefType(i_type)) {
        வெறுமை* reference = வெற்று;
_       (Get_Ref_மதிப்பு(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } இல்லையெனில் எனில் (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 மதிப்பு = 0;
_       (ReadLEB_i32(&மதிப்பு, &l->pos, l->end));
        *o_bits = (u32)மதிப்பு;
    } இல்லையெனில் எனில் (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 மதிப்பு = 0;
_       (GetLEB_i64(l, &மதிப்பு));
        *o_bits = (u64)மதிப்பு;
    } இல்லையெனில் எனில் (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } இல்லையெனில் எனில் (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } இல்லையெனில் {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result Get_மதிப்பு_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3Result result = m3Err_none;
    m3type_t type;

_   (Get_மதிப்பு_Tag(l, i_expectedType, &type));
_   (Get_மதிப்பு_Body(l, type, o_bits));

_catch:
    திரும்பு result;
}

நிலையான
M3Result Get_மதிப்பு (M3SnapshotLoad* l, m3type_t i_expectedType, வெறுமை* o_where)
{
    M3Result result = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    எனில் (l->checking) {
        o_where = scratch;
    }

_   (Get_மதிப்பு_Tag(l, i_expectedType, &type));

    எனில் (IsRefType(type)) {
        வெறுமை* reference = வெற்று;
        u64   bits      = 0;
_       (Get_மதிப்பு_Body(l, type, &bits));
        reference = (வெறுமை*)(uintptr_t)bits;
        memcpy(o_where, &reference, அளவு(reference));
    } இல்லையெனில் எனில் (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } இல்லையெனில் {
        u64 bits = 0;
_       (Get_மதிப்பு_Body(l, type, &bits));

        எனில் (Is64BitType(type)) {
            memcpy(o_where, &bits, அளவு(bits));
        } இல்லையெனில் {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, அளவு(narrow));
        }
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3Result result = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->இலக்கம்_Functions);
    *o_function = &l->module->functions[index];

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_size)
{
    M3Result result = m3Err_none;

    க்கு (;;) {
        u8  chunkType = 0;
        u32 offset = 0, length = 0;

_       (GetU8(l, &chunkType));

        எனில் (chunkType == d_m3ChunkEnd) {
            நிறுத்து;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &length));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_size or length > i_size - (size_t)offset);

        எனில் (chunkType == d_m3ChunkRaw) {
            எனில் (l->checking or length == 0) {
_               (SkipBytes(l, length));
            } இல்லையெனில் {
_               (Get(l, o_bytes + offset, length));
            }
        } இல்லையெனில் எனில் (chunkType == d_m3ChunkFillFF) {
            எனில் (not l->checking and length) {
                memset(o_bytes + offset, 0xFF, length);
            }
        } இல்லையெனில் {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadMemories (M3SnapshotLoad* l)
{
    M3Result  result      = m3Err_none;
    IM3Module module      = l->module;
    u32       இலக்கம்_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &இலக்கம்_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, இலக்கம்_Memories == 0 or இலக்கம்_Memories != module->இலக்கம்_Memories);

    க்கு (u32 m = 0; m < இலக்கம்_Memories; ++m) {
        u32       memIndex = 0, firstIndex = 0, pageBits = 0, இலக்கம்_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &memIndex));
_       (GetLEB_u32(l, &firstIndex));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, memIndex != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 firstIndex != Memory_FirstIndex(module, m));
        எனில் (firstIndex != m) {
            தொடர்;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &இலக்கம்_Pages));

        memory = module->memories[memIndex];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    data  = வெற்று;

        எனில் (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_PageSize(memory));
            _throwif(m3Err_wasmMalformed, இலக்கம்_Pages < memory->இலக்கம்_Pages or இலக்கம்_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, இலக்கம்_Pages > SIZE_MAX / Memory_PageSize(memory));
            bytes = (size_t)இலக்கம்_Pages * Memory_PageSize(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            எனில் (not l->checking) {
                எனில் (memory->இலக்கம்_Pages < இலக்கம்_Pages) {
_                   (ResizeMemory(l->runtime, memory, இலக்கம்_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->length < bytes);
            }

            l->loadedMemoryBytes += bytes;

            எனில் (bytes and memory->mallocated) {
                data = m3MemData(memory->mallocated);
            }
        } இல்லையெனில் {
            _throwif(m3Err_wasmMalformed, இலக்கம்_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        எனில் (data and not l->checking) {
            memset(data, 0, bytes);
        }

_       (LoadMemoryChunks(l, data, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadGlobals (M3SnapshotLoad* l)
{
    M3Result  result     = m3Err_none;
    IM3Module module     = l->module;
    u32       இலக்கம்_Globals = 0;

_   (GetLEB_u32(l, &இலக்கம்_Globals));
    _throwif(m3Err_wasmMalformed, இலக்கம்_Globals == 0 or இலக்கம்_Globals != module->இலக்கம்_Globals);

    க்கு (u32 g = 0; g < இலக்கம்_Globals; ++g) {
        u32       globalIndex = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &globalIndex));
        _throwif(m3Err_wasmMalformed, globalIndex != g);

        global = &module->globals[globalIndex];
        cell   = global->resolved ? global->resolved : global;

_       (Get_மதிப்பு(l, global->type, &cell->i64_மதிப்பு));
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadTables (M3SnapshotLoad* l)
{
    M3Result  result       = m3Err_none;
    IM3Module module       = l->module;
    u32       இலக்கம்_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &இலக்கம்_Tables));
    _throwif(m3Err_wasmMalformed, இலக்கம்_Tables == 0 or இலக்கம்_Tables != module->இலக்கம்_Tables);

    க்கு (u32 t = 0; t < இலக்கம்_Tables; ++t) {
        u32      tableIndex = 0, firstIndex = 0, size = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &tableIndex));
_       (GetLEB_u32(l, &firstIndex));

        _throwif(m3Err_wasmMalformed, tableIndex != t);
        _throwif("the snapshot shares tables between imports differently",
                 firstIndex != Table_FirstIndex(module, t));
        எனில் (firstIndex != t) {
            தொடர்;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &size));

        table = module->tables[tableIndex];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, size > (table->hasMax ? table->maxSize : d_m3MaxSaneTableSize));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, size < table->size);

        l->loadedTableElements += size;

        எனில் (not l->checking and size != table->size) {
            வெறுமை** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 size - table->size > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(வெறுமை*, table->elements, size, table->size);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += size - table->size;
            table->elements = elements;
            table->size     = size;
        }

        க்கு (u32 e = 0; e < size; ++e) {
            வெறுமை* reference = வெற்று;
_           (Get_Ref_மதிப்பு(l, table->type, &reference));
            எனில் (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadSegments (M3SnapshotLoad* l)
{
    M3Result  result = m3Err_none;
    IM3Module module = l->module;
    u32       மொத்தம்  = 0;

_   (GetLEB_u32(l, &மொத்தம்));
    _throwif(m3Err_wasmMalformed, மொத்தம் != module->இலக்கம்_Data_Segments);
    _throwif(m3Err_wasmMalformed, module->இலக்கம்_Data_Segments == 0 and module->இலக்கம்_Element_Segments == 0);

    க்கு (u32 i = 0; i < மொத்தம்; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        எனில் (not l->checking) {
            module->dataSegments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &மொத்தம்));
    _throwif(m3Err_wasmMalformed, மொத்தம் != module->இலக்கம்_Element_Segments);

    க்கு (u32 i = 0; i < மொத்தம்; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        எனில் (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    திரும்பு result;
}

// A frame for the continuation being put together, outermost first
நிலையான
M3Result AddFrame (M3SnapshotLoad* l, u32* io_இலக்கம்_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    எனில் (*io_இலக்கம்_Frames >= d_m3ContinuationMaxFrames) {
        திரும்பு "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_இலக்கம்_Frames)++];

    memset(frame, 0, அளவு(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    திரும்பு m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
நிலையான
M3Result PrepareFunction (IM3Function i_function)
{
    M3Result result = m3Err_none;

    எனில் (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    திரும்பு result;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
நிலையான
M3Result AddBlockFrames (M3SnapshotLoad* l, மாறிலி M3SnapshotMap* i_map, மாறிலி M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_இலக்கம்_Frames)
{
    M3Result result = m3Err_none;

    க்கு (u32 b = 0; b < i_map->இலக்கம்_Blocks; ++b) {
        மாறிலி M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        எனில் (not EnclosesOffset(block, i_point->wasmOffset)) {
            தொடர்;
        }

        எனில் (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_இலக்கம்_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  எனில் d_m3HasExceptionHandling
        இல்லையெனில் {
_           (AddFrame(l, io_இலக்கம்_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.இலக்கம்_Clauses   = block->இலக்கம்_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    திரும்பு result;
}

// Puts a function's values where this build keeps them at the safepoint
நிலையான
M3Result Load_மதிப்புகள் (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_இலக்கம்_Slots, m3stack_t i_sp,
                     மாறிலி M3SnapshotMap* i_map, மாறிலி M3SafePoint* i_point, மாறிலி M3FrameRegisters* i_registers)
{
    M3Result result    = m3Err_none;
    u32      இலக்கம்_மதிப்புகள் = 0;

_   (GetLEB_u32(l, &இலக்கம்_மதிப்புகள்));
    _throwif("the snapshot's frame does not match what this build compiled", இலக்கம்_மதிப்புகள் != i_map->இலக்கம்_Locals + i_point->இலக்கம்_மதிப்புகள்);

    க்கு (u32 v = 0; v < இலக்கம்_மதிப்புகள்; ++v) {
        மாறிலி M3_Slot_மதிப்பு* மதிப்பு     = Frame_மதிப்பு(i_map, i_point, v);
        u64                word      = 0;
        வெறுமை*              reference = வெற்று;
        u8*                where;

_       (Locate_மதிப்பு(i_base, i_இலக்கம்_Slots, i_sp, மதிப்பு, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        எனில் (BaseTypeOf(மதிப்பு->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_மதிப்பு_Tag(l, மதிப்பு->type, &type));
            எனில் (l->checking) {
_               (SkipBytes(l, 16));
            } இல்லையெனில் {
_               (Get(l, where, 16));
            }
            தொடர்;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= இலக்கம்_மதிப்புகள் - i_point->இலக்கம்_Results;
_       (Get_மதிப்பு_Bits(l, மதிப்பு->type, &word));
        l->allowUnfilledRef = false;

        எனில் (IsRefType(மதிப்பு->type)) {
            reference = (வெறுமை*)(uintptr_t)word;
        }

        எனில் (not l->checking) {
_           (Write_மதிப்பு(where, i_registers, மதிப்பு, word, reference));
        }
    }

_catch:
    திரும்பு result;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
நிலையான
M3Result LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3Result    result    = m3Err_none;
    u32         இலக்கம்_Frames = 0;
    M3Frame*    callFrame = வெற்று;
    IM3Function function  = வெற்று;
    m3slot_t*   base;
    u32         இலக்கம்_Slots;
    u32         இலக்கம்_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_அடுக்கு(l->runtime, io_cont, &base, &இலக்கம்_Slots));

_   (GetLEB_u32(l, &இலக்கம்_Activations));
    _throwif(m3Err_wasmMalformed, இலக்கம்_Activations == 0 or இலக்கம்_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = வெற்று;
    io_cont->r0                = 0;
    io_cont->இலக்கம்_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  எனில் d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    க்கு (u32 a = 0; a < இலக்கம்_Activations; ++a) {
        bool                 isInnermost = (a + 1 == இலக்கம்_Activations);
        மாறிலி M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        மாறிலி M3SnapshotMap* map;

        memset(&registers, 0, அளவு(registers));

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
                 (u32)(sp - base) + function->max_அடுக்கு_Slots > இலக்கம்_Slots);

        // the call the frame outside this one is waiting on
        எனில் (callFrame) {
            callFrame->call.function = function;
        }

#  எனில் d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &இலக்கம்_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        எனில் (kind == safepoint_op) {
            மாறிலி M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &இலக்கம்_Frames));

        எனில் (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &இலக்கம்_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  எனில் d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } இல்லையெனில் எனில் (kind == safepoint_resume) {
            M3Frame* frame;
            u32      இலக்கம்_Results = point->இலக்கம்_Results;
            pc_t     resultsPC  = point->pc - 2 * இலக்கம்_Results;

_           (AddFrame(l, &இலக்கம்_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.இலக்கம்_Results  = இலக்கம்_Results;
            frame->resume.இலக்கம்_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } இல்லையெனில் {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  எனில் d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_மதிப்புகள்(l, base, இலக்கம்_Slots, sp, map, point, &registers));

        எனில் (not l->checking) {
            RestoreConstants(function, sp);
        }

        எனில் (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } இல்லையெனில் எனில் (kind == safepoint_resume) {
            வெறுமை* cont = வெற்று;
_           (Get_Ref_மதிப்பு(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == வெற்று);

            callFrame->resume.cont = (IM3Continuation)cont;
            எனில் (not l->checking) {
                l->suspendSites[i_id].point = point;
                க்கு (u32 i = 1; i < l->இலக்கம்_Continuations; ++i) {
                    எனில் (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        நிறுத்து;
                    }
                }
            }
        } இல்லையெனில் {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            எனில் (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            எனில் (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->இலக்கம்_Results > point->இலக்கம்_மதிப்புகள் or
                                                point->இலக்கம்_Results > d_m3MaxContinuationPayload);

                io_cont->இலக்கம்_Suspend_Results = point->இலக்கம்_Results;
                எனில் (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                க்கு (u32 i = 0; i < point->இலக்கம்_Results; ++i) {
                    மாறிலி M3_Slot_மதிப்பு* மதிப்பு = &map->மதிப்புகள்[point->first_மதிப்பு + point->இலக்கம்_மதிப்புகள் - point->இலக்கம்_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(மதிப்பு->slot));

                    io_cont->suspendResultOffsets[i] = மதிப்பு->slot;
                    io_cont->suspendResultIs64[i]    = Is64BitType(மதிப்பு->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    எனில் (l->checking) {
        திரும்பு m3Err_none;
    }

    // held innermost first
    எனில் (இலக்கம்_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, இலக்கம்_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = இலக்கம்_Frames;
    }

    க்கு (u32 k = 0; k < இலக்கம்_Frames; ++k) {
        io_cont->frames[இலக்கம்_Frames - 1 - k] = l->frames[k];
    }

    io_cont->இலக்கம்_Frames = இலக்கம்_Frames;

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3Result        result = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    எனில் (l->checking) {
        scratch          = *cont;
        scratch.val_அடுக்கு = l->check_அடுக்கு;
        எனில் (i_id == 0) {
            scratch.val_அடுக்கு = (m3slot_t*)l->runtime->origin_அடுக்கு;
        }
        cont = &scratch;
    }

    u32 contId = 0, typeIndex = 0, entryIndex = 0, bound_Args_மொத்தம் = 0;
    u8  state = 0, is_வேர் = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_வேர்));
_   (GetLEB_u32(l, &typeIndex));
_   (GetLEB_u32(l, &entryIndex));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_வேர் != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_வேர் and state != snapshot_contSuspended);
    எனில் (not is_வேர் and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, typeIndex != d_m3SnapshotNone and typeIndex >= module->இலக்கம்_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entryIndex >= module->இலக்கம்_Functions);
    _throwif(m3Err_wasmMalformed, is_வேர் ? typeIndex != d_m3SnapshotNone : (typeIndex == d_m3SnapshotNone or not module->funcTypes[typeIndex]->isContinuation));

    எனில் (not l->checking and not is_வேர் and state != snapshot_contFinished) {
_       (Continuation_Acquire_அடுக்கு(l->runtime, cont));
    }
    cont->type          = (typeIndex != d_m3SnapshotNone) ? module->funcTypes[typeIndex] : வெற்று;
    cont->entryFunction = &module->functions[entryIndex];
    cont->இலக்கம்_Handlers   = 0;
    cont->handlersPC    = வெற்று;
    cont->parent        = வெற்று;
    cont->இலக்கம்_Frames     = 0;

#  எனில் d_m3HasExceptionHandling
    {
        வெறுமை* ref = வெற்று;
_       (Get_Ref_மதிப்பு(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_வேர் or state == snapshot_contFinished) and cont->resumeThrow != வெற்று);
#  இல்லையெனில்
    {
        வெறுமை* ref = வெற்று;
_       (Get_Ref_மதிப்பு(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != வெற்று);
    }
#  endif

    எனில் (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         இலக்கம்_Rets;

_       (GetLEB_u32(l, &bound_Args_மொத்தம்));
        cont->bound_Args_மொத்தம் = bound_Args_மொத்தம்;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_மொத்தம் > Get_Func_Type_இலக்கம்_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_இலக்கம்_Params(remaining) != Get_Func_Type_இலக்கம்_Params(inner) - bound_Args_மொத்தம் or
                                            Get_Func_Type_இலக்கம்_Results(remaining) != Get_Func_Type_இலக்கம்_Results(inner));
            க்கு (u16 i = 0; i < Get_Func_Type_இலக்கம்_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(inner, i) != GetFuncTypeResultType(remaining, i));
            }
            க்கு (u16 i = 0; i < Get_Func_Type_இலக்கம்_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_மொத்தம் + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        இலக்கம்_Rets = Get_Func_Type_இலக்கம்_Results(inner);

        _throwif(m3Err_wasmMalformed, (இலக்கம்_Rets + bound_Args_மொத்தம்) * c_io_Slot_மொத்தம் > cont->இலக்கம்_அடுக்கு_Slots);

        க்கு (u32 i = 0; i < bound_Args_மொத்தம்; ++i) {
_           (Get_மதிப்பு(l, GetFuncTypeParamType(inner, (u16)i), cont->val_அடுக்கு + (இலக்கம்_Rets + i) * c_io_Slot_மொத்தம்));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_அடுக்கு;
        cont->pc    = cont->entryFunction->compiled;
    } இல்லையெனில் எனில் (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_மொத்தம்));
        _throwif(m3Err_wasmMalformed, is_வேர் and bound_Args_மொத்தம் != 0);
        cont->bound_Args_மொத்தம் = bound_Args_மொத்தம்;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } இல்லையெனில் எனில் (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_மொத்தம்));
        _throwif(m3Err_wasmMalformed, bound_Args_மொத்தம் != 0);
        cont->bound_Args_மொத்தம் = 0;
        cont->state          = cont_consumed;
    } இல்லையெனில் {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    எனில் (result) {
        cont->இலக்கம்_Frames = 0;
    }

    திரும்பு result;
}

நிலையான
M3Result LoadContinuations (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u32      மொத்தம்  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &மொத்தம்));
    _throwif(m3Err_wasmMalformed, மொத்தம் == 0 or மொத்தம் != l->இலக்கம்_Continuations);

    க்கு (u32 i = 0; i < மொத்தம்; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    திரும்பு result;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
நிலையான
M3Result CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;

    க்கு (u32 id = 0; id < l->இலக்கம்_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  எனில் d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        எனில் (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : வெற்று;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_இலக்கம்_Results(actual) != Get_Func_Type_இலக்கம்_Results(expected));
            க்கு (u16 i = 0; i < Get_Func_Type_இலக்கம்_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(actual, i) != GetFuncTypeResultType(expected, i));
            }
        }
        போது (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->இலக்கம்_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        எனில் (cont->state != cont_suspended or id == 0) {
            தொடர்;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        எனில் (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_மொத்தம் != 0);
            தொடர்;
        }

        மாறிலி M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_மொத்தம் > inner->இலக்கம்_Suspend_Results or
                                                                          Get_Func_Type_இலக்கம்_Params(remaining) != inner->இலக்கம்_Suspend_Results - cont->bound_Args_மொத்தம்);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_இலக்கம்_Results(remaining) != Get_Func_Type_இலக்கம்_Results(cont->entryFunction->funcType));
        க்கு (u16 i = 0; i < Get_Func_Type_இலக்கம்_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(remaining, i) !=
                                            GetFuncTypeResultType(cont->entryFunction->funcType, i));
        }

        க்கு (u32 i = 0; i < inner->இலக்கம்_Suspend_Results; ++i) {
            மாறிலி M3_Slot_மதிப்பு* மதிப்பு = &site->map->மதிப்புகள்[site->point->first_மதிப்பு +
                                                          site->point->இலக்கம்_மதிப்புகள் - site->point->இலக்கம்_Results + i];
            எனில் (i < cont->bound_Args_மொத்தம்) {
                எனில் (IsRefType(மதிப்பு->type) and not IsNullableRef(மதிப்பு->type)) {
                    வெறுமை* ref = வெற்று;
                    memcpy(&ref, inner->sp + மதிப்பு->slot, அளவு(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } இல்லையெனில் {
                _throwif("a captured continuation has the wrong parameter type", மதிப்பு->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_மொத்தம்)));
            }
        }
    }

_catch:
    திரும்பு result;
}

#  எனில் d_m3HasExceptionHandling

நிலையான
M3Result LoadExceptions (M3SnapshotLoad* l)
{
    M3Result result        = m3Err_none;
    u32      இலக்கம்_Exceptions = 0;

_   (GetLEB_u32(l, &இலக்கம்_Exceptions));
    _throwif(m3Err_wasmMalformed, இலக்கம்_Exceptions == 0 or இலக்கம்_Exceptions != l->இலக்கம்_Exceptions);

    க்கு (u32 i = 0; i < இலக்கம்_Exceptions; ++i) {
        u32          exnId = 0, இலக்கம்_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &இலக்கம்_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, இலக்கம்_Args != exn->இலக்கம்_Args);

        க்கு (u32 a = 0; a < இலக்கம்_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_மதிப்பு_Bits(l, argType, &bits));
            எனில் (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    திரும்பு result;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
வகைவரை கட்டமைப்பு M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3Result        error;
} M3BoundedReader;

நிலையான
M3Result BoundedReader_Read (வெறுமை* o_buffer, size_t i_size, வெறுமை* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    எனில் (i_size > br->remaining) {
        br->error = "the host read past the end of its state";
        திரும்பு br->error;
    }

    br->remaining -= i_size;

    திரும்பு Get(br->load, o_buffer, i_size);
}

நிலையான
M3Result LoadHostState (M3SnapshotLoad* l)
{
    M3Result               result = m3Err_none;
    மாறிலி M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 size = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    எனில் (l->checking) {
        திரும்பு SkipBytes(l, size);
    }
    reader.load      = l;
    reader.remaining = size;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, size));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadHeader (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u8       magic[அளவு(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, அளவு(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, அளவு(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    திரும்பு result;
}

நிலையான
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

    (வெறுமை)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->இலக்கம்_Continuations));
_   (GetLEB_u32(l, &l->இலக்கம்_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->இலக்கம்_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->இலக்கம்_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    எனில் (*o_flags & d_m3SnapshotFlagPostmortem) {
        க்கு (u32 i = 0; i < l->இலக்கம்_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
        }
        திரும்பு m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->இலக்கம்_Continuations == 0 or activeStacks >= l->இலக்கம்_Continuations);
#  எனில் !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->இலக்கம்_Exceptions != 0);
#  endif

    எனில் (l->checking) {
        க்கு (u32 i = 0; i < l->இலக்கம்_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
#  எனில் d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tagIndex >= l->module->இலக்கம்_Tags);
#  endif
        }
        திரும்பு m3Err_none;
    }

    l->check_அடுக்கு = m3_AllocArray(m3slot_t, d_m3_Continuation_அடுக்கு_Slots + 4);
    _throwifnull(l->check_அடுக்கு);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->இலக்கம்_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->இலக்கம்_Continuations);
    _throwifnull(l->suspendSites);

    எனில் (not l->runtime->வேர்_Continuation) {
        IM3Continuation வேர் = Continuation_New(l->runtime, வெற்று, வெற்று);
        _throwifnull(வேர்);

        // the root borrows the runtime's stack, so it holds none of its own
        வேர்->இலக்கம்_அடுக்கு_Slots          = l->runtime->இலக்கம்_அடுக்கு_Slots;
        l->runtime->வேர்_Continuation = வேர்;
    }

    l->continuations[0] = l->runtime->வேர்_Continuation;

    க்கு (u32 i = 1; i < l->இலக்கம்_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, வெற்று, வெற்று);
        _throwifnull(l->continuations[i]);
    }

#  எனில் d_m3HasExceptionHandling
    எனில் (l->இலக்கம்_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->இலக்கம்_Exceptions);
        _throwifnull(l->exceptions);
    }

    க்கு (u32 i = 0; i < l->இலக்கம்_Exceptions; ++i) {
        u32    tagIndex = 0;
        IM3Tag tag;
        u32    இலக்கம்_Args;

_       (GetLEB_u32(l, &tagIndex));
        _throwif(m3Err_wasmMalformed, tagIndex >= l->module->இலக்கம்_Tags);

        tag     = TagOfIndex(l->module, tagIndex);
        இலக்கம்_Args = Get_Func_Type_இலக்கம்_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, இலக்கம்_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  இல்லையெனில்
    க்கு (u32 i = 0; i < l->இலக்கம்_Exceptions; ++i) {
        u32 tagIndex = 0;
_       (GetLEB_u32(l, &tagIndex));
    }
#  endif

_catch:
    திரும்பு result;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
நிலையான
M3Result LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3Result result = m3Err_none;
    u32      flags  = 0;
    தேர்வு (i_sectionId) {
    நிலை d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        நிறுத்து;

    நிலை d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        நிறுத்து;

    நிலை d_m3SnapshotSection_Table:
_       (LoadTables(l));
        நிறுத்து;

    நிலை d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        நிறுத்து;

    நிலை d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        நிறுத்து;

#  எனில் d_m3HasExceptionHandling
    நிலை d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        நிறுத்து;
#  endif

    நிலை d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        நிறுத்து;

    நிலை d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        நிறுத்து;

    இயல்பு:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        நிறுத்து;
    }

_catch:
    திரும்பு result;
}

நிலையான
M3Result LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3Result result  = m3Err_none;
    u8*      payload = வெற்று;

    எனில் (not i_embedded) {
_       (LoadHeader(l));
    }

    போது (true) {
        u8  sectionId   = 0;
        u32 sectionSize = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        எனில் (GetU8(l, &sectionId)) {
            நிறுத்து;
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
             l->இலக்கம்_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->இலக்கம்_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->இலக்கம்_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->இலக்கம்_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->இலக்கம்_Data_Segments or l->module->இலக்கம்_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    திரும்பு result;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#எனில் d_m3HasSnapshots

நிலையான
M3Result SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, வெறுமை* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3Result       result = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, அளவு(save));

    IM3Continuation வேர் = io_runtime->வேர்_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not வேர் or வேர்->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : வேர்->entryFunction;

    எனில் (o_module) {
        *o_module = வெற்று;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    எனில் (o_module) {
        *o_module = save.module;
    }

    க்கு (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->next) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  எனில் d_m3HasExceptionHandling
    க்கு (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->next) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    எனில் (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, வேர், &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    திரும்பு result;
}

#endif

M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, வெறுமை* i_userdata)
{
    எனில் (not io_runtime or not i_writer) {
        திரும்பு m3Err_mallocFailed;
    }

#எனில் d_m3HasSnapshots
    திரும்பு SaveSnapshot(io_runtime, i_writer, i_userdata, வெற்று, false);
#இல்லையெனில்
    (வெறுமை)i_userdata;
    திரும்பு "snapshots are not available in this build of Wasm3";
#endif
}


நிலையான
M3Result LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, வெறுமை* i_userdata, bool i_embedded)
{
    M3Result result = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#எனில் d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, அளவு(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        result = LoadSections(&load, i_embedded);

        IM3Continuation வேர் = io_runtime->வேர்_Continuation;

        எனில் (result) {
            எனில் (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                எனில் (வேர்) {
                    வேர்->state     = cont_returned;
                    வேர்->இலக்கம்_Frames = 0;
                }
            }
        } இல்லையெனில் {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_அடுக்கு(io_runtime);

        m3_Free(load.check_அடுக்கு);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  எனில் d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#இல்லையெனில்
    (வெறுமை)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    திரும்பு result;
}


M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, வெறுமை* i_userdata)
{
    திரும்பு LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


வெறுமை m3_SetSnapshotHooks (IM3Runtime io_runtime, மாறிலி M3SnapshotHooks* i_hooks)
{
#எனில் d_m3HasSnapshots
    எனில் (io_runtime) {
        எனில் (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } இல்லையெனில் {
            memset(&io_runtime->snapshotHooks, 0, அளவு(io_runtime->snapshotHooks));
        }
    }
#இல்லையெனில்
    (வெறுமை)io_runtime;
    (வெறுமை)i_hooks;
#endif
}


M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, வெறுமை** o_bytes, size_t* o_size)
{
    எனில் (!io_runtime || !o_bytes || !o_size) {
        திரும்பு m3Err_mallocFailed;
    }
#எனில் d_m3HasSnapshots
    M3BufferWriter bw     = { வெற்று, 0, 0 };
    M3Result       result = m3_SaveSnapshot(io_runtime, BufferWriter_Write, &bw);
    எனில் (result) {
        m3_Free(bw.buffer);
        *o_bytes = வெற்று;
        *o_size  = 0;
        திரும்பு result;
    }
    *o_bytes = bw.buffer;
    *o_size  = bw.size;
    திரும்பு m3Err_none;
#இல்லையெனில்
    *o_bytes = வெற்று;
    *o_size  = 0;
    திரும்பு "snapshots are not available in this build of Wasm3";
#endif
}

#எனில் d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
நிலையான
u8* EmitLEB_u32 (u8* o_where, u32 i_மதிப்பு)
{
    செய் {
        u8 byte = i_மதிப்பு & 0x7F;
        i_மதிப்பு >>= 7;
        எனில் (i_மதிப்பு != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } போது (i_மதிப்பு != 0);

    திரும்பு o_where;
}

நிலையான
u32 SizeOfLEB_u32 (u32 i_மதிப்பு)
{
    u32 size = 0;

    செய் {
        i_மதிப்பு >>= 7;
        ++size;
    } போது (i_மதிப்பு != 0);

    திரும்பு size;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
நிலையான
M3Result FindEmbeddedSnapshot (மாறிலி u8* i_start, மாறிலி u8* i_end, cstr_t i_secName,
                               மாறிலி u8** o_secStart, மாறிலி u8** o_secEnd,
                               மாறிலி u8** o_data, u32* o_size)
{
    M3Result result = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = வெற்று;

    எனில் (o_secStart) {
        *o_secStart = வெற்று;
    }
    எனில் (o_secEnd) {
        *o_secEnd = வெற்று;
    }
    எனில் (o_data) {
        *o_data = வெற்று;
    }
    எனில் (o_size) {
        *o_size = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    போது (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     size      = 0;
_       (ReadLEB_u32(&size, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, size > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += size;
            எனில் (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                எனில் (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    எனில் (o_secStart) {
                        *o_secStart = secStart;
                    }
                    எனில் (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    எனில் (o_data) {
                        *o_data = payload;
                    }
                    எனில் (o_size) {
                        *o_size = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    திரும்பு result;
}

// Builds the custom section name a snapshot of i_name lives under
நிலையான
M3Result SnapshotSectionName (cstr_t i_name, எழுத்து* o_secName, size_t i_size)
{
    எனில் (i_name and *i_name) {
        எனில் (strlen(i_name) > d_m3MaxSnapshotNameLength) {
            திரும்பு "the snapshot name is too long";
        }
        snprintf(o_secName, i_size, "snapshot.%s", i_name);
    } இல்லையெனில் {
        snprintf(o_secName, i_size, "snapshot");
    }

    திரும்பு m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
நிலையான
M3Result ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3Result result = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = வெற்று;

    க்கு (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->next) {
        எனில் (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    திரும்பு result;
}
#endif

M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, மாறிலி வெறுமை* i_bytes, size_t i_size)
{
    எனில் (!io_runtime || !i_module || !i_bytes) {
        திரும்பு m3Err_mallocFailed;
    }
#எனில் d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    எனில் (i_size >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        மாறிலி u8* data   = வெற்று;
        u32       size   = 0;
        M3Result  result = FindEmbeddedSnapshot((மாறிலி u8*)i_bytes, (மாறிலி u8*)i_bytes + i_size,
                                                "snapshot", வெற்று, வெற்று, &data, &size);
        எனில் (result) {
            திரும்பு result;
        }
        எனில் (not data) {
            திரும்பு "the Wasm binary holds no embedded snapshot";
        }

        {
            M3BufferReader br = { data, size, 0 };
            திரும்பு LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
        }
    }
    {
        M3BufferReader br = { (மாறிலி u8*)i_bytes, i_size, 0 };
        திரும்பு LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br,
                            i_size < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#இல்லையெனில்
    (வெறுமை)i_size;
    திரும்பு "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, மாறிலி எழுத்து* i_name)
{
    எனில் (not i_module) {
        திரும்பு false;
    }
#எனில் d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = வெற்று;
    (வெறுமை)ModuleSnapshot(i_module, i_name, &snap);

    திரும்பு snap != வெற்று;
#இல்லையெனில்
    (வெறுமை)i_name;
    திரும்பு false;
#endif
}

M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, மாறிலி எழுத்து* i_name,
                                 மாறிலி வெறுமை** o_bytes, size_t* o_size)
{
    எனில் (not i_module or not o_bytes or not o_size) {
        திரும்பு m3Err_mallocFailed;
    }

    *o_bytes = வெற்று;
    *o_size  = 0;

#எனில் d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = வெற்று;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);

        எனில் (result) {
            திரும்பு result;
        }
        எனில் (not snap) {
            திரும்பு "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->data;
        *o_size  = snap->size;

        திரும்பு m3Err_none;
    }
#இல்லையெனில்
    (வெறுமை)i_name;
    திரும்பு "snapshots are not available in this build of Wasm3";
#endif
}

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, மாறிலி எழுத்து* i_name)
{
    எனில் (not io_runtime or not i_module) {
        திரும்பு m3Err_mallocFailed;
    }
#எனில் d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = வெற்று;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);
        M3BufferReader      br;

        எனில் (result) {
            திரும்பு result;
        }
        எனில் (not snap) {
            திரும்பு "the module holds no embedded snapshot of that name";
        }

        br.buffer = snap->data;
        br.size   = snap->size;
        br.cursor = 0;

        திரும்பு LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
    }
#இல்லையெனில்
    (வெறுமை)i_name;
    திரும்பு "snapshots are not available in this build of Wasm3";
#endif
}


M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, மாறிலி எழுத்து* i_name,
                                  வெறுமை** o_bytes, size_t* o_size)
{
#எனில் d_m3HasSnapshots
    M3Result       result = m3Err_none;
    M3BufferWriter bw     = { வெற்று, 0, 0 };
    u8*            out    = வெற்று;

    எழுத்து      secName[d_m3MaxSnapshotNameLength + அளவு("snapshot.")];
    u32       nameLen, nameLenLebSize, payloadSize, payloadSizeLebSize;
    size_t    origSize, totalSize;
    மாறிலி u8* dropStart = வெற்று;
    மாறிலி u8* dropEnd   = வெற்று;
    IM3Module saved     = வெற்று;
    u8*       p;

    எனில் (not io_runtime or not i_module or not o_bytes or not o_size) {
        திரும்பு m3Err_mallocFailed;
    }

    *o_bytes = வெற்று;
    *o_size  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, அளவு(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->வேர்_Continuation or io_runtime->வேர்_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, BufferWriter_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.size);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.size > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, வெற்று, வெற்று));

    origSize           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    nameLenLebSize     = SizeOfLEB_u32(nameLen);
    payloadSize        = nameLenLebSize + nameLen + (u32)bw.size;
    payloadSizeLebSize = SizeOfLEB_u32(payloadSize);
    totalSize          = origSize + 1 + payloadSizeLebSize + payloadSize;

    out = (u8*)m3_Malloc("wasm_snapshot_module", totalSize);
    _throwifnull(out);

    p = out;

    எனில் (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } இல்லையெனில் {
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
    out      = வெற்று;

_catch:
    m3_Free(bw.buffer);
    m3_Free(out);

    திரும்பு result;
#இல்லையெனில்
    (வெறுமை)io_runtime;
    (வெறுமை)i_module;
    (வெறுமை)i_name;
    (வெறுமை)o_bytes;
    (வெறுமை)o_size;
    திரும்பு "snapshots are not available in this build of Wasm3";
#endif
}
