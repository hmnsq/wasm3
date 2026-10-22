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

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_compile.h"
#अन्तर्भाव "m3_exception.h"
#अन्तर्भाव "m3_host.h"

#अन्तर्भाव <limits.h>

#जइ d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
स्थिर ध्रुव u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
स्थिर ध्रुव u32 c_snapshotVersion  = 1;

गणना {
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
गणना {
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
स्थिर
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

        जाव (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     size      = 0;

_           (ReadLEB_u32(&size, &pos, end));
            _throwif(m3Err_wasmUnderrun, size > (size_t)(end - pos));

            pos += size;
            जइ (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    प्रत्यागम result;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

स्थिर
ध्रुव M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    ध्रुव M3SnapshotMap* map = i_function->snapshotMap;

    जइ (map) {
        प्रति (u32 i = 0; i < map->अङ्कः_Safe_Points; ++i) {
            जइ (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                प्रत्यागम &map->safePoints[i];
            }
        }
    }

    प्रत्यागम रिक्त;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
स्थिर
ध्रुव M3SafePoint* FindSafePointAt (ध्रुव M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 नीचः  = 0;
    u32 उच्चः = i_map->अङ्कः_Safe_Points;

    जाव (नीचः < उच्चः) {
        u32 मध्यः = नीचः + (उच्चः - नीचः) / 2;

        जइ (i_map->safePoints[मध्यः].wasmOffset < i_wasmOffset) {
            नीचः = मध्यः + 1;
        } अण्णहा {
            उच्चः = मध्यः;
        }
    }

    प्रति (u32 i = नीचः; i < i_map->अङ्कः_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        ध्रुव M3SafePoint* point = &i_map->safePoints[i];

        जइ (point->kind != i_kind or (point->flags & d_m3SafePointTailCall)) {
            अनुवर्त;
        }
        जइ (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            प्रत्यागम point;
        }
    }

    प्रत्यागम रिक्त;
}

स्थिर
ध्रुव M3BlockStart* FindBlockAt (ध्रुव M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    प्रति (u32 i = 0; i < i_map->अङ्कः_Blocks; ++i) {
        जइ (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            प्रत्यागम &i_map->blocks[i];
        }
    }

    प्रत्यागम रिक्त;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
स्थिर अन्तःस्थ
bool EnclosesOffset (ध्रुव M3BlockStart* i_block, u32 i_wasmOffset)
{
    प्रत्यागम i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
स्थिर अन्तःस्थ
bool HandlersLiveAt (ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, ध्रुव M3BlockStart* i_try)
{
    प्रत्यागम not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
स्थिर
M3Result CallSiteType (IM3Function i_caller, ध्रुव M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3Result   result = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        size   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = रिक्त;

_   (ReadLEB_u32(&size, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    जइ (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->अङ्कः_Functions);
        *o_type = module->functions[index].funcType;
    } अण्णहा {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->अङ्कः_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    प्रत्यागम result;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
स्थिर
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 अङ्कः_Results = Get_Func_Type_अङ्कः_Results(i_expected);

    जइ (not i_type or Get_Func_Type_अङ्कः_Results(i_type) != अङ्कः_Results) {
        प्रत्यागम false;
    }

    प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
        जइ (not IsSubTypeOf(GetFuncTypeResultType(i_type, i), GetFuncTypeResultType(i_expected, i))) {
            प्रत्यागम false;
        }
    }

    प्रत्यागम true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
प्रकारनिर्देश संरचना M3FrameRegisters {
    m3reg_t* r0;
#  जइ d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
स्थिर
M3Result Continuation_राशिः (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_अङ्कः_Slots)
{
    जइ (i_cont == i_runtime->मूलम्_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_राशिः;
        *o_अङ्कः_Slots = i_runtime->अङ्कः_राशिः_Slots;
    } अण्णहा {
        *o_base     = i_cont->val_राशिः;
        *o_अङ्कः_Slots = i_cont->अङ्कः_राशिः_Slots;
    }

    प्रत्यागम (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
स्थिर
M3Result Locate_मानम् (m3slot_t* i_base, u32 i_अङ्कः_Slots, m3stack_t i_sp, ध्रुव M3_Slot_मानम्* i_मानम्, u8** o_where)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_मानम्->type);

    *o_where = रिक्त;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_मानम्->slot));

    जइ (not IsRegisterSlotAlias(i_मानम्->slot)) {
        size_t size   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? परिमाण(शून्य*)
                                                                   : (Is64BitType(type) ? परिमाण(u64) : परिमाण(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_मानम्->slot) * परिमाण(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + size > (size_t)i_अङ्कः_Slots * परिमाण(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    प्रत्यागम result;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
स्थिर
M3Result Read_मानम् (u8* i_where, ध्रुव M3FrameRegisters* i_registers, ध्रुव M3_Slot_मानम्* i_मानम्, u64* o_bits,
                    शून्य** o_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_मानम्->type);

    *o_bits      = 0;
    *o_reference = रिक्त;

    जइ (i_where) {
        जइ (IsRefType(type)) {
            memcpy(o_reference, i_where, परिमाण(शून्य*));
        } अण्णहा जइ (Is64BitType(type)) {
            memcpy(o_bits, i_where, परिमाण(u64));
        } अण्णहा {
            u32 narrow;
            memcpy(&narrow, i_where, परिमाण(narrow));
            *o_bits = narrow;
        }
    } अण्णहा जइ (IsFpRegisterSlotAlias(i_मानम्->slot)) {
#  जइ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        जइ (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, परिमाण(bits));
            *o_bits = bits;
        } अण्णहा {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, परिमाण(u64));
        }
#  अण्णहा
        _throw(m3Err_wasmMalformed);
#  endif
    } अण्णहा {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        जइ (IsRefType(type)) {
            *o_reference = (शून्य*)(uintptr_t)r0;
        } अण्णहा जइ (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } अण्णहा {
            *o_bits = (u32)r0;
        }
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result Write_मानम् (u8* o_where, ध्रुव M3FrameRegisters* i_registers, ध्रुव M3_Slot_मानम्* i_मानम्, u64 i_bits,
                     शून्य* i_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_मानम्->type);

    जइ (o_where) {
        जइ (IsRefType(type)) {
            memcpy(o_where, &i_reference, परिमाण(शून्य*));
        } अण्णहा जइ (Is64BitType(type)) {
            memcpy(o_where, &i_bits, परिमाण(u64));
        } अण्णहा {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, परिमाण(narrow));
        }
    } अण्णहा जइ (IsFpRegisterSlotAlias(i_मानम्->slot)) {
#  जइ d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        जइ (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, परिमाण(narrow));
            *i_registers->fp0 = narrow;
        } अण्णहा {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, परिमाण(u64));
        }
#  अण्णहा
        _throw(m3Err_wasmMalformed);
#  endif
    } अण्णहा {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        जइ (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } अण्णहा {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    प्रत्यागम result;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
स्थिर
ध्रुव M3_Slot_मानम्* Frame_मानम् (ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, u32 i_index)
{
    जइ (i_index < i_map->अङ्कः_Locals) {
        प्रत्यागम i_point->first_Local_मानम् == UINT32_MAX ? &i_map->locals[i_index]
                                                      : &i_map->मानानि[i_point->first_Local_मानम् + i_index];
    }
    प्रत्यागम &i_map->मानानि[i_point->first_मानम् + i_index - i_map->अङ्कः_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
स्थिर
शून्य RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    जइ (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->अङ्कः_Ret_And_Arg_Slots) + i_function->अङ्कः_Local_Bytes;

        memcpy(where, i_function->constants, i_function->अङ्कः_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
प्रकारनिर्देश संरचना M3PointerIds {
    शून्य** वस्तूनि;
    u32    गणनम्;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  indexCapacity;       // a power of two, kept at least twice count
} M3PointerIds;

स्थिर
u32 PointerBucket (शून्य* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    प्रत्यागम (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

स्थिर
u32 PointerIds_Find (ध्रुव M3PointerIds* i_ids, शून्य* i_pointer)
{
    जइ (i_ids->indexCapacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->indexCapacity);

        जाव (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            जइ (i_ids->वस्तूनि[id] == i_pointer) {
                प्रत्यागम id;
            }

            bucket = (bucket + 1) & (i_ids->indexCapacity - 1);
        }
    }

    प्रत्यागम d_m3SnapshotNone;
}

स्थिर
M3Result PointerIds_Add (M3PointerIds* io_ids, शून्य* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    जइ (id != d_m3SnapshotNone) {
        *o_id = id;
        प्रत्यागम m3Err_none;
    }

    जइ (io_ids->गणनम् == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        शून्य** वस्तूनि    = m3_ReallocArray(शून्य*, io_ids->वस्तूनि, capacity, io_ids->capacity);

        जइ (not वस्तूनि) {
            प्रत्यागम m3Err_mallocFailed;
        }

        io_ids->वस्तूनि    = वस्तूनि;
        io_ids->capacity = capacity;
    }

    जइ ((io_ids->गणनम् + 1) * 2 > io_ids->indexCapacity) {
        u32  capacity = io_ids->indexCapacity ? io_ids->indexCapacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        जइ (not index) {
            प्रत्यागम m3Err_mallocFailed;
        }

        प्रति (u32 i = 0; i < io_ids->गणनम्; ++i) {
            u32 bucket = PointerBucket(io_ids->वस्तूनि[i], capacity);

            जाव (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->indexCapacity = capacity;
    }

    id                = io_ids->गणनम्++;
    io_ids->वस्तूनि[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->indexCapacity);

    जाव (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->indexCapacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    प्रत्यागम m3Err_none;
}

स्थिर
शून्य PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->वस्तूनि);
    m3_Free(io_ids->index);
    memset(io_ids, 0, परिमाण(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

प्रकारनिर्देश संरचना M3BufferWriter {
    u8*    buffer;
    size_t size;
    size_t capacity;
} M3BufferWriter;

स्थिर
M3Result BufferWriter_Write (ध्रुव शून्य* i_data, size_t i_size, शून्य* i_userdata)
{
    M3BufferWriter* bw = (M3BufferWriter*)i_userdata;

    जइ (bw->size + i_size > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        जाव (bw->size + i_size > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->buffer, newCap, bw->capacity);
        जइ (!newBuf) {
            प्रत्यागम m3Err_mallocFailed;
        }
        bw->buffer   = newBuf;
        bw->capacity = newCap;
    }

    जइ (i_size) {
        memcpy(bw->buffer + bw->size, i_data, i_size);
        bw->size += i_size;
    }

    प्रत्यागम m3Err_none;
}

प्रकारनिर्देश संरचना M3BufferReader {
    ध्रुव u8* buffer;
    size_t    size;
    size_t    cursor;
} M3BufferReader;

स्थिर
M3Result BufferReader_Read (शून्य* o_buffer, size_t i_size, शून्य* i_userdata)
{
    M3BufferReader* br = (M3BufferReader*)i_userdata;

    जइ (i_size > br->size - br->cursor) {
        प्रत्यागम m3Err_wasmMalformed;
    }

    जइ (i_size) {
        memcpy(o_buffer, br->buffer + br->cursor, i_size);
        br->cursor += i_size;
    }

    प्रत्यागम m3Err_none;
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
प्रकारनिर्देश संरचना M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    शून्य*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

स्थिर अन्तःस्थ
u8 M3TypeToValType (m3type_t i_type)
{
    विकल्प (BaseTypeOf(i_type)) {
    पक्ष c_m3Type_i32: प्रत्यागम 0x7F;
    पक्ष c_m3Type_i64: प्रत्यागम 0x7E;
    पक्ष c_m3Type_f32: प्रत्यागम 0x7D;
    पक्ष c_m3Type_f64: प्रत्यागम 0x7C;
    पक्ष c_m3Type_v128: प्रत्यागम 0x7B;
    पक्ष c_m3Type_funcref: प्रत्यागम 0x70;
    पक्ष c_m3Type_externref: प्रत्यागम 0x6F;
    पक्ष c_m3Type_exnref: प्रत्यागम 0x69;
    पक्ष c_m3Type_contref: प्रत्यागम 0x68;
    स्वभाव: प्रत्यागम 0x00;
    }
}

स्थिर अन्तःस्थ
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    विकल्प (i_valtype) {
    पक्ष 0x7F: प्रत्यागम c_m3Type_i32;
    पक्ष 0x7E: प्रत्यागम c_m3Type_i64;
    पक्ष 0x7D: प्रत्यागम c_m3Type_f32;
    पक्ष 0x7C: प्रत्यागम c_m3Type_f64;
    पक्ष 0x7B: प्रत्यागम c_m3Type_v128;
    पक्ष 0x70: प्रत्यागम c_m3Type_funcref;
    पक्ष 0x6F: प्रत्यागम c_m3Type_externref;
    पक्ष 0x69: प्रत्यागम c_m3Type_exnref;
    पक्ष 0x68: प्रत्यागम c_m3Type_contref;
    स्वभाव: प्रत्यागम c_m3Type_none;
    }
}

स्थिर
M3Result Put (M3SnapshotSave* s, ध्रुव शून्य* i_data, size_t i_size)
{
    प्रत्यागम s->writing ? s->writer(i_data, i_size, s->userdata) : m3Err_none;
}

स्थिर
M3Result PutU8 (M3SnapshotSave* s, u8 i_मानम्)
{
    प्रत्यागम Put(s, &i_मानम्, परिमाण(i_मानम्));
}


स्थिर
M3Result PutLEB_u32 (M3SnapshotSave* s, u32 i_मानम्)
{
    u8  bytes[5];
    u32 len = 0;

    करेहि {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        जइ (i_मानम् != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } जाव (i_मानम् != 0);

    प्रत्यागम Put(s, bytes, len);
}

स्थिर
M3Result PutLEB_u64 (M3SnapshotSave* s, u64 i_मानम्)
{
    u8  bytes[10];
    u32 len = 0;

    करेहि {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        जइ (i_मानम् != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } जाव (i_मानम् != 0);

    प्रत्यागम Put(s, bytes, len);
}

स्थिर
M3Result PutLEB_i64 (M3SnapshotSave* s, i64 i_मानम्)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    जाव (more) {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        जइ ((i_मानम् == 0 && !(byte & 0x40)) || (i_मानम् == -1 && (byte & 0x40))) {
            more = false;
        } अण्णहा {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    प्रत्यागम Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
प्रकारनिर्देश M3Result (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
स्थिर
M3Result SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3Result         result   = m3Err_none;
    M3BufferWriter   bw       = { रिक्त, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    शून्य*            userdata = s->userdata;

    s->writer   = BufferWriter_Write;
    s->userdata = &bw;

    result = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    जइ (result) {
        गच्छ _catch;
    }

    // a section with nothing in it is left out
    जइ (bw.size) {
        _throwif("snapshot section is too large", bw.size > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.size));
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    प्रत्यागम result;
}

स्थिर
u32 FunctionIndex (IM3Module i_module, IM3Function i_function)
{
    जइ (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->अङ्कः_Functions) {
        प्रत्यागम (u32)(i_function - i_module->functions);
    }

    प्रत्यागम d_m3SnapshotNone;
}

स्थिर
u32 FuncTypeIndex (IM3Module i_module, IM3FuncType i_type)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Func_Types; ++i) {
        जइ (i_module->funcTypes[i] == i_type) {
            प्रत्यागम i;
        }
    }

    प्रत्यागम d_m3SnapshotNone;
}

#  जइ d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
स्थिर
IM3Tag TagOfIndex (IM3Module i_module, u32 i_index)
{
    IM3Tag tag = &i_module->tags[i_index];

    प्रत्यागम tag->resolved ? tag->resolved : tag;
}

स्थिर
u32 TagIndex (IM3Module i_module, IM3Tag i_tag)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Tags; ++i) {
        जइ (TagOfIndex(i_module, i) == i_tag) {
            प्रत्यागम i;
        }
    }

    प्रत्यागम d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
स्थिर
M3Result NameReference (M3SnapshotSave* s, u8 i_type, शून्य* i_reference, u64* o_word)
{
    M3Result result = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    जइ (not i_reference) {
        प्रत्यागम m3Err_none;
    }

    जइ (i_type == c_m3Type_funcref) {
        id = FunctionIndex(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } अण्णहा जइ (i_type == c_m3Type_externref) {
        ध्रुव M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        गच्छ _catch;
    }
#  जइ d_m3HasExceptionHandling
    अण्णहा जइ (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 गणनम् = s->exceptions.गणनम्;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.गणनम् != गणनम्);
    }
#  endif
    अण्णहा जइ (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 गणनम् = s->continuations.गणनम्;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.गणनम् != गणनम्);
    } अण्णहा {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    प्रत्यागम result;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
स्थिर
M3Result Put_Ref_मानम् (M3SnapshotSave* s, u8 i_type, शून्य* i_reference)
{
    M3Result result = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    जइ (not s->writing) {
        प्रत्यागम m3Err_none;
    }

    जइ (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } अण्णहा {
        u8 kind = 0;
        जइ (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } अण्णहा जइ (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  जइ d_m3HasExceptionHandling
        अण्णहा जइ (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        अण्णहा जइ (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    प्रत्यागम result;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
स्थिर
M3Result PutLE32 (M3SnapshotSave* s, u32 i_मानम्)
{
    u8 bytes[4];

    प्रति (u32 i = 0; i < परिमाण(bytes); ++i) {
        bytes[i] = (u8)(i_मानम् >> (8 * i));
    }

    प्रत्यागम Put(s, bytes, परिमाण(bytes));
}

स्थिर
M3Result PutLE64 (M3SnapshotSave* s, u64 i_मानम्)
{
    u8 bytes[8];

    प्रति (u32 i = 0; i < परिमाण(bytes); ++i) {
        bytes[i] = (u8)(i_मानम् >> (8 * i));
    }

    प्रत्यागम Put(s, bytes, परिमाण(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
स्थिर
M3Result Put_मानम्_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    जइ (IsRefType(i_type)) {
        प्रत्यागम Put_Ref_मानम्(s, BaseTypeOf(i_type), (शून्य*)(uintptr_t)i_bits);
    }

    विकल्प (BaseTypeOf(i_type)) {
    पक्ष c_m3Type_i32: प्रत्यागम PutLEB_i64(s, (i32)(u32)i_bits);
    पक्ष c_m3Type_i64: प्रत्यागम PutLEB_i64(s, (i64)i_bits);
    पक्ष c_m3Type_f32: प्रत्यागम PutLE32(s, (u32)i_bits);
    पक्ष c_m3Type_f64: प्रत्यागम PutLE64(s, i_bits);
    स्वभाव: प्रत्यागम "a snapshot cannot hold a value of this type as bits";
    }
}

स्थिर
M3Result Put_मानम्_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    जइ (not valtype) {
        प्रत्यागम "a snapshot has no tag for a value of this type";
    }

    प्रत्यागम PutU8(s, valtype);
}

स्थिर
M3Result Put_मानम्_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3Result result = m3Err_none;

_   (Put_मानम्_Tag(s, i_type));
_   (Put_मानम्_Body(s, i_type, i_bits));

_catch:
    प्रत्यागम result;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
स्थिर
M3Result Put_मानम् (M3SnapshotSave* s, m3type_t i_type, ध्रुव शून्य* i_where)
{
    M3Result result = m3Err_none;

_   (Put_मानम्_Tag(s, i_type));

    जइ (IsRefType(i_type)) {
        शून्य* reference;
        memcpy(&reference, i_where, परिमाण(reference));
_       (Put_मानम्_Body(s, i_type, (u64)(uintptr_t)reference));
    } अण्णहा जइ (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, परिमाण(bytes));
_       (Put(s, bytes, परिमाण(bytes)));
    } अण्णहा जइ (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, परिमाण(bits));
_       (Put_मानम्_Body(s, i_type, bits));
    } अण्णहा {
        u32 narrow;
        memcpy(&narrow, i_where, परिमाण(narrow));
_       (Put_मानम्_Body(s, i_type, narrow));
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result PutMemoryChunks (M3SnapshotSave* s, ध्रुव u8* i_bytes, size_t i_size)
{
    M3Result result = m3Err_none;
    size_t   cursor = 0;

    जाव (cursor < i_size) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        जाव (cursor + run < i_size and i_bytes[cursor + run] == byte) {
            run++;
        }

        जइ ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            जइ (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            अनुवर्त;
        }

        size_t rawStart = cursor;

        जाव (cursor < i_size) {
            u8     b      = i_bytes[cursor];
            size_t length = 1;

            जाव (cursor + length < i_size and i_bytes[cursor + length] == b) {
                length++;
            }

            जइ ((b == 0x00 or b == 0xFF) and length >= d_m3SnapshotRunThreshold) {
                भङ्ग;
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
    प्रत्यागम result;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
स्थिर
u32 Memory_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    जाव (module->memories[j] != module->memories[index]) {
        ++j;
    }
    प्रत्यागम j;
}

स्थिर
u32 Table_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    जाव (module->tables[j] != module->tables[index]) {
        ++j;
    }
    प्रत्यागम j;
}

स्थिर
M3Result SaveMemories (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_Memories));

    प्रति (u32 m = 0; m < module->अङ्कः_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_FirstIndex(module, m);
        bool      hasData  = memory and memory->mallocated;
        u32       pageSize = memory ? Memory_PageSize(memory) : d_m3DefaultMemPageSize;
        u64       bytes    = memory ? memory->अङ्कः_Pages * (u64)pageSize : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        जइ (first != m) {
            अनुवर्त;
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
        जाव ((1u << pageBits) < pageSize) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->अङ्कः_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        जइ (not s->writing) {
            अनुवर्त;
        }

        जइ (hasData) {
_           (PutMemoryChunks(s, m3MemData(memory->mallocated), (size_t)bytes));
        } अण्णहा {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result SaveGlobals (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_Globals));

    प्रति (u32 g = 0; g < module->अङ्कः_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_मानम्(s, global->type, &cell->i64_मानम्));
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result SaveTables (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_Tables));

    प्रति (u32 t = 0; t < module->अङ्कः_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_FirstIndex(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        जइ (first != t) {
            अनुवर्त;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->size));

        प्रति (u32 e = 0; e < table->size; ++e) {
_           (Put_Ref_मानम्(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result SaveSegments (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_Data_Segments));

    प्रति (u32 i = 0; i < module->अङ्कः_Data_Segments; ++i) {
_       (PutU8(s, module->dataSegments[i].dropped));
    }

_   (PutLEB_u32(s, module->अङ्कः_Element_Segments));

    प्रति (u32 i = 0; i < module->अङ्कः_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    प्रत्यागम result;
}

#  जइ d_m3HasExceptionHandling

स्थिर
M3Result SaveExceptions (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.गणनम्));

    प्रति (u32 i = 0; i < s->exceptions.गणनम्; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.वस्तूनि[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->अङ्कः_Args != Get_Func_Type_अङ्कः_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->अङ्कः_Args));

        प्रति (u32 a = 0; a < exn->अङ्कः_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_मानम्_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    प्रत्यागम result;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
प्रकारनिर्देश संरचना M3Activation {
    IM3Function        function;
    ध्रुव M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    ध्रुव M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
स्थिर
ध्रुव M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    ध्रुव M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : रिक्त;

    प्रति (u32 i = 0; not point and i < i_module->अङ्कः_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        जइ (point) {
            *io_function = &i_module->functions[i];
        }
    }

    प्रत्यागम point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
स्थिर
M3Result NextActivation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3Result result = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, परिमाण(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    प्रति (; i >= 0; --i) {
        ध्रुव M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        जइ (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  जइ d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            भङ्ग;
        } अण्णहा जइ (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            भङ्ग;
        }
    }

    जइ (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  जइ d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  जइ d_m3EntryKeepsFrame
    प्रति (i32 k = i_first; k > o_act->lastFrame; --k) {
        ध्रुव M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    प्रत्यागम result;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
स्थिर
M3Result SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_अङ्कः_Slots, ध्रुव M3Activation* i_act)
{
    M3Result             result        = m3Err_none;
    IM3Function          function      = i_act->function;
    ध्रुव M3SnapshotMap* map           = function->snapshotMap;
    u32                  functionIndex = FunctionIndex(s->module, function);
    ध्रुव M3SafePoint*   point         = i_act->point;
    u32                  अङ्कः_मानानि;

    _throwif("a suspended frame belongs to another module", functionIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, functionIndex));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    जइ (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    अङ्कः_मानानि = map->अङ्कः_Locals + point->अङ्कः_मानानि;

_   (PutLEB_u32(s, अङ्कः_मानानि));

    प्रति (u32 v = 0; v < अङ्कः_मानानि; ++v) {
        ध्रुव M3_Slot_मानम्* मानम् = Frame_मानम्(map, point, v);
        u8*                where;
        u64                word;
        शून्य*              reference;

_       (Locate_मानम्(i_base, i_अङ्कः_Slots, i_act->sp, मानम्, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        जइ (BaseTypeOf(मानम्->type) == c_m3Type_v128) {
_           (Put_मानम्_Tag(s, मानम्->type));
_           (Put(s, where, 16));
            अनुवर्त;
        }

_       (Read_मानम्(where, &i_act->registers, मानम्, &word, &reference));

_       (Put_मानम्_Bits(s, मानम्->type, IsRefType(मानम्->type) ? (u64)(uintptr_t)reference : word));
    }

    जइ (point->kind == safepoint_resume) {
_       (Put_Ref_मानम्(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    प्रत्यागम result;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
स्थिर
M3Result SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3Result      result   = m3Err_none;
    M3Activation* acts     = रिक्त;
    u32           अङ्कः_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->अङ्कः_Frames - 1;
    m3slot_t*     base;
    u32           अङ्कः_Slots;
    m3stack_t     sp;

_   (Continuation_राशिः(s->runtime, i_cont, &base, &अङ्कः_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->अङ्कः_Frames + 1);
    _throwifnull(acts);

    sp = base;

    जाव (true) {
        M3Activation* act = &acts[अङ्कः_Acts];

_       (NextActivation(s->module, i_cont, function, sp, first, act));

        जइ (act->kind != safepoint_call) {
            अङ्कः_Acts++;
            भङ्ग;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        जइ (not (act->point->flags & d_m3SafePointTailCall)) {
            अङ्कः_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, अङ्कः_Acts));

    प्रति (u32 a = 0; a < अङ्कः_Acts; ++a) {
_       (SaveActivation(s, base, अङ्कः_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    प्रत्यागम result;
}

स्थिर
M3Result SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3Result  result    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_मूलम्    = (i_cont == s->runtime->मूलम्_Continuation);
    u8        state     = snapshot_contFinished;
    u32       typeIndex = is_मूलम् ? d_m3SnapshotNone : FuncTypeIndex(module, i_cont->type);

    जइ (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } अण्णहा जइ (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } अण्णहा जइ (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", FunctionIndex(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_मूलम् and typeIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_मूलम्));
_   (PutLEB_u32(s, typeIndex));
_   (PutLEB_u32(s, FunctionIndex(module, i_cont->entryFunction)));
#  जइ d_m3HasExceptionHandling
_   (Put_Ref_मानम्(s, c_m3Type_exnref, i_cont->resumeThrow));
#  अण्णहा
_   (Put_Ref_मानम्(s, c_m3Type_exnref, रिक्त));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    जइ (s->postmortem) {
        गच्छ _catch;
    }

    जइ (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : रिक्त;
        u16         अङ्कः_Rets = inner ? Get_Func_Type_अङ्कः_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_गणनम् > Get_Func_Type_अङ्कः_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_गणनम्));

        प्रति (u32 i = 0; i < i_cont->bound_Args_गणनम्; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_मानम्(s, type, i_cont->val_राशिः + (अङ्कः_Rets + i) * c_io_Slot_गणनम्));
        }
    } अण्णहा जइ (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_गणनम्));
_       (SaveSuspendedBody(s, i_cont));
    } अण्णहा {
_       (PutLEB_u32(s, 0));
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result SaveContinuations (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->continuations.गणनम्));

    प्रति (u32 i = 0; i < s->continuations.गणनम्; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.वस्तूनि[i]));
    }

_catch:
    प्रत्यागम result;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
स्थिर
M3Result SaveHostState (M3SnapshotSave* s)
{
    M3Result               result = m3Err_none;
    ध्रुव M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3BufferWriter         bw     = { रिक्त, 0, 0 };

    जइ (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, BufferWriter_Write, &bw));
    }

    जइ (bw.size > 0) {
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    प्रत्यागम result;
}

स्थिर
शून्य SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    प्रति (u32 i = 0; i < module->अङ्कः_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        जइ (Memory_FirstIndex(module, i) == i and memory) {
            *memoryBytes += memory->अङ्कः_Pages * (u64)Memory_PageSize(memory);
        }
    }
    प्रति (u32 i = 0; i < module->अङ्कः_Tables; ++i) {
        IM3Table table = module->tables[i];
        जइ (Table_FirstIndex(module, i) == i and table) {
            *tableElements += table->size;
        }
    }
}

स्थिर
M3Result SaveMeta (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    प्रति (u32 i = 0; i < s->continuations.गणनम्; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.वस्तूनि[i];
        जइ (cont != s->runtime->मूलम्_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.गणनम्));
_   (PutLEB_u32(s, s->exceptions.गणनम्));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  जइ d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    प्रति (u32 i = 0; i < s->exceptions.गणनम्; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.वस्तूनि[i];
        u32          tagIndex  = TagIndex(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tagIndex == d_m3SnapshotNone);

_       (PutLEB_u32(s, tagIndex));
    }
#  endif

_catch:
    प्रत्यागम result;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
स्थिर
M3Result DiscoverReferences (M3SnapshotSave* s)
{
    M3Result result            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    जाव (doneContinuations < s->continuations.गणनम् or doneExceptions < s->exceptions.गणनम्) {
#  जइ d_m3HasExceptionHandling
        जाव (doneExceptions < s->exceptions.गणनम्) {
            M3Exception* exception = (M3Exception*)s->exceptions.वस्तूनि[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->अङ्कः_Args != Get_Func_Type_अङ्कः_Params(type));

            प्रति (u32 a = 0; a < exception->अङ्कः_Args; ++a) {
_               (Put_मानम्_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        जाव (doneContinuations < s->continuations.गणनम् and doneExceptions == s->exceptions.गणनम्) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.वस्तूनि[id]));
        }
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result SaveSections (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

    जइ (not s->writing) {
        प्रत्यागम DiscoverReferences(s);
    }

    जइ (not s->embedded) {
_       (Put(s, c_snapshotMagic, परिमाण(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    जइ (s->module->अङ्कः_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    जइ (s->module->अङ्कः_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    जइ (s->module->अङ्कः_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    जइ (s->module->अङ्कः_Data_Segments or s->module->अङ्कः_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  जइ d_m3HasExceptionHandling
    जइ (s->exceptions.गणनम्) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    जइ (s->continuations.गणनम्) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    प्रत्यागम result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

प्रकारनिर्देश संरचना M3SnapshotSuspendSite {
    ध्रुव M3SnapshotMap* map;
    ध्रुव M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

प्रकारनिर्देश संरचना M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    शून्य*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_राशिः;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    अङ्कः_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  जइ d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 अङ्कः_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

स्थिर
M3Result Get (M3SnapshotLoad* l, शून्य* o_data, size_t i_size)
{
    जइ (not l->inSection) {
        प्रत्यागम l->reader(o_data, i_size, l->userdata);
    }
    जइ (i_size > (size_t)(l->end - l->pos)) {
        प्रत्यागम m3Err_wasmUnderrun;
    }
    जइ (i_size) {
        memcpy(o_data, l->pos, i_size);
        l->pos += i_size;
    }
    प्रत्यागम m3Err_none;
}

स्थिर
M3Result GetU8 (M3SnapshotLoad* l, u8* o_मानम्)
{
    प्रत्यागम Get(l, o_मानम्, परिमाण(*o_मानम्));
}

स्थिर
M3Result GetBool (M3SnapshotLoad* l, u8* o_मानम्)
{
    M3Result result = GetU8(l, o_मानम्);
    प्रत्यागम result ? result : (*o_मानम् <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

स्थिर
M3Result GetLEB_u32 (M3SnapshotLoad* l, u32* o_मानम्)
{
    जइ (l->inSection) {
        प्रत्यागम ReadLEB_u32(o_मानम्, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    प्रति (u32 i = 0; i < परिमाण(encoded); ++i) {
        M3Result result = GetU8(l, &encoded[i]);
        जइ (result) {
            प्रत्यागम result;
        }
        जइ (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            प्रत्यागम ReadLEB_u32(o_मानम्, &pos, encoded + i + 1);
        }
    }
    प्रत्यागम m3Err_lebOverflow;
}

स्थिर
M3Result GetLEB_u64 (M3SnapshotLoad* l, u64* o_मानम्)
{
    प्रत्यागम ReadLebUnsigned(o_मानम्, 64, &l->pos, l->end);
}

स्थिर
M3Result GetLEB_i64 (M3SnapshotLoad* l, i64* o_मानम्)
{
    प्रत्यागम ReadLEB_i64(o_मानम्, &l->pos, l->end);
}

स्थिर
M3Result SkipBytes (M3SnapshotLoad* l, size_t i_size)
{
    जइ (i_size > (size_t)(l->end - l->pos)) {
        प्रत्यागम m3Err_wasmUnderrun;
    }
    l->pos += i_size;
    प्रत्यागम m3Err_none;
}


स्थिर
M3Result Get_Ref_मानम् (M3SnapshotLoad* l, m3type_t i_type, शून्य** o_reference)
{
    M3Result result = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = रिक्त;

_   (GetU8(l, &kind));
    जइ (kind == 0x00) {
        प्रत्यागम (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    जइ (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->अङ्कः_Functions);
#  जइ d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } अण्णहा जइ (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        ध्रुव M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        जइ (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  जइ d_m3HasExceptionHandling
    अण्णहा जइ (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->अङ्कः_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    अण्णहा जइ (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->अङ्कः_Continuations);
        *o_reference = l->continuations[id];
#  जइ d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        जइ (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } अण्णहा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result GetLE32 (M3SnapshotLoad* l, u32* o_मानम्)
{
    u8       bytes[4];
    M3Result result = Get(l, bytes, परिमाण(bytes));

    *o_मानम् = 0;

    प्रति (u32 i = 0; i < परिमाण(bytes); ++i) {
        *o_मानम् |= (u32)bytes[i] << (8 * i);
    }

    प्रत्यागम result;
}

स्थिर
M3Result GetLE64 (M3SnapshotLoad* l, u64* o_मानम्)
{
    u8       bytes[8];
    M3Result result = Get(l, bytes, परिमाण(bytes));

    *o_मानम् = 0;

    प्रति (u32 i = 0; i < परिमाण(bytes); ++i) {
        *o_मानम् |= (u64)bytes[i] << (8 * i);
    }

    प्रत्यागम result;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
स्थिर
M3Result Get_मानम्_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3Result result  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    जइ (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    प्रत्यागम result;
}

// The mirror of PutValueBody: narrow types come back in the low bits
स्थिर
M3Result Get_मानम्_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3Result result = m3Err_none;

    *o_bits = 0;

    जइ (IsRefType(i_type)) {
        शून्य* reference = रिक्त;
_       (Get_Ref_मानम्(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } अण्णहा जइ (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 मानम् = 0;
_       (ReadLEB_i32(&मानम्, &l->pos, l->end));
        *o_bits = (u32)मानम्;
    } अण्णहा जइ (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 मानम् = 0;
_       (GetLEB_i64(l, &मानम्));
        *o_bits = (u64)मानम्;
    } अण्णहा जइ (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } अण्णहा जइ (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } अण्णहा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result Get_मानम्_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3Result result = m3Err_none;
    m3type_t type;

_   (Get_मानम्_Tag(l, i_expectedType, &type));
_   (Get_मानम्_Body(l, type, o_bits));

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result Get_मानम् (M3SnapshotLoad* l, m3type_t i_expectedType, शून्य* o_where)
{
    M3Result result = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    जइ (l->checking) {
        o_where = scratch;
    }

_   (Get_मानम्_Tag(l, i_expectedType, &type));

    जइ (IsRefType(type)) {
        शून्य* reference = रिक्त;
        u64   bits      = 0;
_       (Get_मानम्_Body(l, type, &bits));
        reference = (शून्य*)(uintptr_t)bits;
        memcpy(o_where, &reference, परिमाण(reference));
    } अण्णहा जइ (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } अण्णहा {
        u64 bits = 0;
_       (Get_मानम्_Body(l, type, &bits));

        जइ (Is64BitType(type)) {
            memcpy(o_where, &bits, परिमाण(bits));
        } अण्णहा {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, परिमाण(narrow));
        }
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3Result result = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->अङ्कः_Functions);
    *o_function = &l->module->functions[index];

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_size)
{
    M3Result result = m3Err_none;

    प्रति (;;) {
        u8  chunkType = 0;
        u32 offset = 0, length = 0;

_       (GetU8(l, &chunkType));

        जइ (chunkType == d_m3ChunkEnd) {
            भङ्ग;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &length));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_size or length > i_size - (size_t)offset);

        जइ (chunkType == d_m3ChunkRaw) {
            जइ (l->checking or length == 0) {
_               (SkipBytes(l, length));
            } अण्णहा {
_               (Get(l, o_bytes + offset, length));
            }
        } अण्णहा जइ (chunkType == d_m3ChunkFillFF) {
            जइ (not l->checking and length) {
                memset(o_bytes + offset, 0xFF, length);
            }
        } अण्णहा {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadMemories (M3SnapshotLoad* l)
{
    M3Result  result      = m3Err_none;
    IM3Module module      = l->module;
    u32       अङ्कः_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &अङ्कः_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, अङ्कः_Memories == 0 or अङ्कः_Memories != module->अङ्कः_Memories);

    प्रति (u32 m = 0; m < अङ्कः_Memories; ++m) {
        u32       memIndex = 0, firstIndex = 0, pageBits = 0, अङ्कः_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &memIndex));
_       (GetLEB_u32(l, &firstIndex));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, memIndex != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 firstIndex != Memory_FirstIndex(module, m));
        जइ (firstIndex != m) {
            अनुवर्त;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &अङ्कः_Pages));

        memory = module->memories[memIndex];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    data  = रिक्त;

        जइ (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_PageSize(memory));
            _throwif(m3Err_wasmMalformed, अङ्कः_Pages < memory->अङ्कः_Pages or अङ्कः_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, अङ्कः_Pages > SIZE_MAX / Memory_PageSize(memory));
            bytes = (size_t)अङ्कः_Pages * Memory_PageSize(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            जइ (not l->checking) {
                जइ (memory->अङ्कः_Pages < अङ्कः_Pages) {
_                   (ResizeMemory(l->runtime, memory, अङ्कः_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->length < bytes);
            }

            l->loadedMemoryBytes += bytes;

            जइ (bytes and memory->mallocated) {
                data = m3MemData(memory->mallocated);
            }
        } अण्णहा {
            _throwif(m3Err_wasmMalformed, अङ्कः_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        जइ (data and not l->checking) {
            memset(data, 0, bytes);
        }

_       (LoadMemoryChunks(l, data, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadGlobals (M3SnapshotLoad* l)
{
    M3Result  result     = m3Err_none;
    IM3Module module     = l->module;
    u32       अङ्कः_Globals = 0;

_   (GetLEB_u32(l, &अङ्कः_Globals));
    _throwif(m3Err_wasmMalformed, अङ्कः_Globals == 0 or अङ्कः_Globals != module->अङ्कः_Globals);

    प्रति (u32 g = 0; g < अङ्कः_Globals; ++g) {
        u32       globalIndex = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &globalIndex));
        _throwif(m3Err_wasmMalformed, globalIndex != g);

        global = &module->globals[globalIndex];
        cell   = global->resolved ? global->resolved : global;

_       (Get_मानम्(l, global->type, &cell->i64_मानम्));
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadTables (M3SnapshotLoad* l)
{
    M3Result  result       = m3Err_none;
    IM3Module module       = l->module;
    u32       अङ्कः_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &अङ्कः_Tables));
    _throwif(m3Err_wasmMalformed, अङ्कः_Tables == 0 or अङ्कः_Tables != module->अङ्कः_Tables);

    प्रति (u32 t = 0; t < अङ्कः_Tables; ++t) {
        u32      tableIndex = 0, firstIndex = 0, size = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &tableIndex));
_       (GetLEB_u32(l, &firstIndex));

        _throwif(m3Err_wasmMalformed, tableIndex != t);
        _throwif("the snapshot shares tables between imports differently",
                 firstIndex != Table_FirstIndex(module, t));
        जइ (firstIndex != t) {
            अनुवर्त;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &size));

        table = module->tables[tableIndex];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, size > (table->hasMax ? table->maxSize : d_m3MaxSaneTableSize));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, size < table->size);

        l->loadedTableElements += size;

        जइ (not l->checking and size != table->size) {
            शून्य** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 size - table->size > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(शून्य*, table->elements, size, table->size);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += size - table->size;
            table->elements = elements;
            table->size     = size;
        }

        प्रति (u32 e = 0; e < size; ++e) {
            शून्य* reference = रिक्त;
_           (Get_Ref_मानम्(l, table->type, &reference));
            जइ (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadSegments (M3SnapshotLoad* l)
{
    M3Result  result = m3Err_none;
    IM3Module module = l->module;
    u32       गणनम्  = 0;

_   (GetLEB_u32(l, &गणनम्));
    _throwif(m3Err_wasmMalformed, गणनम् != module->अङ्कः_Data_Segments);
    _throwif(m3Err_wasmMalformed, module->अङ्कः_Data_Segments == 0 and module->अङ्कः_Element_Segments == 0);

    प्रति (u32 i = 0; i < गणनम्; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        जइ (not l->checking) {
            module->dataSegments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &गणनम्));
    _throwif(m3Err_wasmMalformed, गणनम् != module->अङ्कः_Element_Segments);

    प्रति (u32 i = 0; i < गणनम्; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        जइ (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    प्रत्यागम result;
}

// A frame for the continuation being put together, outermost first
स्थिर
M3Result AddFrame (M3SnapshotLoad* l, u32* io_अङ्कः_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    जइ (*io_अङ्कः_Frames >= d_m3ContinuationMaxFrames) {
        प्रत्यागम "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_अङ्कः_Frames)++];

    memset(frame, 0, परिमाण(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    प्रत्यागम m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
स्थिर
M3Result PrepareFunction (IM3Function i_function)
{
    M3Result result = m3Err_none;

    जइ (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    प्रत्यागम result;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
स्थिर
M3Result AddBlockFrames (M3SnapshotLoad* l, ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_अङ्कः_Frames)
{
    M3Result result = m3Err_none;

    प्रति (u32 b = 0; b < i_map->अङ्कः_Blocks; ++b) {
        ध्रुव M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        जइ (not EnclosesOffset(block, i_point->wasmOffset)) {
            अनुवर्त;
        }

        जइ (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_अङ्कः_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  जइ d_m3HasExceptionHandling
        अण्णहा {
_           (AddFrame(l, io_अङ्कः_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.अङ्कः_Clauses   = block->अङ्कः_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    प्रत्यागम result;
}

// Puts a function's values where this build keeps them at the safepoint
स्थिर
M3Result Load_मानानि (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_अङ्कः_Slots, m3stack_t i_sp,
                     ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, ध्रुव M3FrameRegisters* i_registers)
{
    M3Result result    = m3Err_none;
    u32      अङ्कः_मानानि = 0;

_   (GetLEB_u32(l, &अङ्कः_मानानि));
    _throwif("the snapshot's frame does not match what this build compiled", अङ्कः_मानानि != i_map->अङ्कः_Locals + i_point->अङ्कः_मानानि);

    प्रति (u32 v = 0; v < अङ्कः_मानानि; ++v) {
        ध्रुव M3_Slot_मानम्* मानम्     = Frame_मानम्(i_map, i_point, v);
        u64                word      = 0;
        शून्य*              reference = रिक्त;
        u8*                where;

_       (Locate_मानम्(i_base, i_अङ्कः_Slots, i_sp, मानम्, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        जइ (BaseTypeOf(मानम्->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_मानम्_Tag(l, मानम्->type, &type));
            जइ (l->checking) {
_               (SkipBytes(l, 16));
            } अण्णहा {
_               (Get(l, where, 16));
            }
            अनुवर्त;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= अङ्कः_मानानि - i_point->अङ्कः_Results;
_       (Get_मानम्_Bits(l, मानम्->type, &word));
        l->allowUnfilledRef = false;

        जइ (IsRefType(मानम्->type)) {
            reference = (शून्य*)(uintptr_t)word;
        }

        जइ (not l->checking) {
_           (Write_मानम्(where, i_registers, मानम्, word, reference));
        }
    }

_catch:
    प्रत्यागम result;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
स्थिर
M3Result LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3Result    result    = m3Err_none;
    u32         अङ्कः_Frames = 0;
    M3Frame*    callFrame = रिक्त;
    IM3Function function  = रिक्त;
    m3slot_t*   base;
    u32         अङ्कः_Slots;
    u32         अङ्कः_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_राशिः(l->runtime, io_cont, &base, &अङ्कः_Slots));

_   (GetLEB_u32(l, &अङ्कः_Activations));
    _throwif(m3Err_wasmMalformed, अङ्कः_Activations == 0 or अङ्कः_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = रिक्त;
    io_cont->r0                = 0;
    io_cont->अङ्कः_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  जइ d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    प्रति (u32 a = 0; a < अङ्कः_Activations; ++a) {
        bool                 isInnermost = (a + 1 == अङ्कः_Activations);
        ध्रुव M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        ध्रुव M3SnapshotMap* map;

        memset(&registers, 0, परिमाण(registers));

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
                 (u32)(sp - base) + function->max_राशिः_Slots > अङ्कः_Slots);

        // the call the frame outside this one is waiting on
        जइ (callFrame) {
            callFrame->call.function = function;
        }

#  जइ d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &अङ्कः_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        जइ (kind == safepoint_op) {
            ध्रुव M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &अङ्कः_Frames));

        जइ (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &अङ्कः_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  जइ d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } अण्णहा जइ (kind == safepoint_resume) {
            M3Frame* frame;
            u32      अङ्कः_Results = point->अङ्कः_Results;
            pc_t     resultsPC  = point->pc - 2 * अङ्कः_Results;

_           (AddFrame(l, &अङ्कः_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.अङ्कः_Results  = अङ्कः_Results;
            frame->resume.अङ्कः_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } अण्णहा {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  जइ d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_मानानि(l, base, अङ्कः_Slots, sp, map, point, &registers));

        जइ (not l->checking) {
            RestoreConstants(function, sp);
        }

        जइ (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } अण्णहा जइ (kind == safepoint_resume) {
            शून्य* cont = रिक्त;
_           (Get_Ref_मानम्(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == रिक्त);

            callFrame->resume.cont = (IM3Continuation)cont;
            जइ (not l->checking) {
                l->suspendSites[i_id].point = point;
                प्रति (u32 i = 1; i < l->अङ्कः_Continuations; ++i) {
                    जइ (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        भङ्ग;
                    }
                }
            }
        } अण्णहा {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            जइ (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            जइ (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->अङ्कः_Results > point->अङ्कः_मानानि or
                                                point->अङ्कः_Results > d_m3MaxContinuationPayload);

                io_cont->अङ्कः_Suspend_Results = point->अङ्कः_Results;
                जइ (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                प्रति (u32 i = 0; i < point->अङ्कः_Results; ++i) {
                    ध्रुव M3_Slot_मानम्* मानम् = &map->मानानि[point->first_मानम् + point->अङ्कः_मानानि - point->अङ्कः_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(मानम्->slot));

                    io_cont->suspendResultOffsets[i] = मानम्->slot;
                    io_cont->suspendResultIs64[i]    = Is64BitType(मानम्->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    जइ (l->checking) {
        प्रत्यागम m3Err_none;
    }

    // held innermost first
    जइ (अङ्कः_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, अङ्कः_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = अङ्कः_Frames;
    }

    प्रति (u32 k = 0; k < अङ्कः_Frames; ++k) {
        io_cont->frames[अङ्कः_Frames - 1 - k] = l->frames[k];
    }

    io_cont->अङ्कः_Frames = अङ्कः_Frames;

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3Result        result = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    जइ (l->checking) {
        scratch          = *cont;
        scratch.val_राशिः = l->check_राशिः;
        जइ (i_id == 0) {
            scratch.val_राशिः = (m3slot_t*)l->runtime->origin_राशिः;
        }
        cont = &scratch;
    }

    u32 contId = 0, typeIndex = 0, entryIndex = 0, bound_Args_गणनम् = 0;
    u8  state = 0, is_मूलम् = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_मूलम्));
_   (GetLEB_u32(l, &typeIndex));
_   (GetLEB_u32(l, &entryIndex));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_मूलम् != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_मूलम् and state != snapshot_contSuspended);
    जइ (not is_मूलम् and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, typeIndex != d_m3SnapshotNone and typeIndex >= module->अङ्कः_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entryIndex >= module->अङ्कः_Functions);
    _throwif(m3Err_wasmMalformed, is_मूलम् ? typeIndex != d_m3SnapshotNone : (typeIndex == d_m3SnapshotNone or not module->funcTypes[typeIndex]->isContinuation));

    जइ (not l->checking and not is_मूलम् and state != snapshot_contFinished) {
_       (Continuation_Acquire_राशिः(l->runtime, cont));
    }
    cont->type          = (typeIndex != d_m3SnapshotNone) ? module->funcTypes[typeIndex] : रिक्त;
    cont->entryFunction = &module->functions[entryIndex];
    cont->अङ्कः_Handlers   = 0;
    cont->handlersPC    = रिक्त;
    cont->parent        = रिक्त;
    cont->अङ्कः_Frames     = 0;

#  जइ d_m3HasExceptionHandling
    {
        शून्य* ref = रिक्त;
_       (Get_Ref_मानम्(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_मूलम् or state == snapshot_contFinished) and cont->resumeThrow != रिक्त);
#  अण्णहा
    {
        शून्य* ref = रिक्त;
_       (Get_Ref_मानम्(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != रिक्त);
    }
#  endif

    जइ (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         अङ्कः_Rets;

_       (GetLEB_u32(l, &bound_Args_गणनम्));
        cont->bound_Args_गणनम् = bound_Args_गणनम्;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_गणनम् > Get_Func_Type_अङ्कः_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_अङ्कः_Params(remaining) != Get_Func_Type_अङ्कः_Params(inner) - bound_Args_गणनम् or
                                            Get_Func_Type_अङ्कः_Results(remaining) != Get_Func_Type_अङ्कः_Results(inner));
            प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(inner, i) != GetFuncTypeResultType(remaining, i));
            }
            प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_गणनम् + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        अङ्कः_Rets = Get_Func_Type_अङ्कः_Results(inner);

        _throwif(m3Err_wasmMalformed, (अङ्कः_Rets + bound_Args_गणनम्) * c_io_Slot_गणनम् > cont->अङ्कः_राशिः_Slots);

        प्रति (u32 i = 0; i < bound_Args_गणनम्; ++i) {
_           (Get_मानम्(l, GetFuncTypeParamType(inner, (u16)i), cont->val_राशिः + (अङ्कः_Rets + i) * c_io_Slot_गणनम्));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_राशिः;
        cont->pc    = cont->entryFunction->compiled;
    } अण्णहा जइ (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_गणनम्));
        _throwif(m3Err_wasmMalformed, is_मूलम् and bound_Args_गणनम् != 0);
        cont->bound_Args_गणनम् = bound_Args_गणनम्;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } अण्णहा जइ (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_गणनम्));
        _throwif(m3Err_wasmMalformed, bound_Args_गणनम् != 0);
        cont->bound_Args_गणनम् = 0;
        cont->state          = cont_consumed;
    } अण्णहा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    जइ (result) {
        cont->अङ्कः_Frames = 0;
    }

    प्रत्यागम result;
}

स्थिर
M3Result LoadContinuations (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u32      गणनम्  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &गणनम्));
    _throwif(m3Err_wasmMalformed, गणनम् == 0 or गणनम् != l->अङ्कः_Continuations);

    प्रति (u32 i = 0; i < गणनम्; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    प्रत्यागम result;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
स्थिर
M3Result CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;

    प्रति (u32 id = 0; id < l->अङ्कः_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  जइ d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        जइ (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : रिक्त;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_अङ्कः_Results(actual) != Get_Func_Type_अङ्कः_Results(expected));
            प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(actual, i) != GetFuncTypeResultType(expected, i));
            }
        }
        जाव (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->अङ्कः_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        जइ (cont->state != cont_suspended or id == 0) {
            अनुवर्त;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        जइ (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_गणनम् != 0);
            अनुवर्त;
        }

        ध्रुव M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_गणनम् > inner->अङ्कः_Suspend_Results or
                                                                          Get_Func_Type_अङ्कः_Params(remaining) != inner->अङ्कः_Suspend_Results - cont->bound_Args_गणनम्);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_अङ्कः_Results(remaining) != Get_Func_Type_अङ्कः_Results(cont->entryFunction->funcType));
        प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(remaining, i) !=
                                            GetFuncTypeResultType(cont->entryFunction->funcType, i));
        }

        प्रति (u32 i = 0; i < inner->अङ्कः_Suspend_Results; ++i) {
            ध्रुव M3_Slot_मानम्* मानम् = &site->map->मानानि[site->point->first_मानम् +
                                                          site->point->अङ्कः_मानानि - site->point->अङ्कः_Results + i];
            जइ (i < cont->bound_Args_गणनम्) {
                जइ (IsRefType(मानम्->type) and not IsNullableRef(मानम्->type)) {
                    शून्य* ref = रिक्त;
                    memcpy(&ref, inner->sp + मानम्->slot, परिमाण(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } अण्णहा {
                _throwif("a captured continuation has the wrong parameter type", मानम्->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_गणनम्)));
            }
        }
    }

_catch:
    प्रत्यागम result;
}

#  जइ d_m3HasExceptionHandling

स्थिर
M3Result LoadExceptions (M3SnapshotLoad* l)
{
    M3Result result        = m3Err_none;
    u32      अङ्कः_Exceptions = 0;

_   (GetLEB_u32(l, &अङ्कः_Exceptions));
    _throwif(m3Err_wasmMalformed, अङ्कः_Exceptions == 0 or अङ्कः_Exceptions != l->अङ्कः_Exceptions);

    प्रति (u32 i = 0; i < अङ्कः_Exceptions; ++i) {
        u32          exnId = 0, अङ्कः_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &अङ्कः_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, अङ्कः_Args != exn->अङ्कः_Args);

        प्रति (u32 a = 0; a < अङ्कः_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_मानम्_Bits(l, argType, &bits));
            जइ (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    प्रत्यागम result;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
प्रकारनिर्देश संरचना M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3Result        error;
} M3BoundedReader;

स्थिर
M3Result BoundedReader_Read (शून्य* o_buffer, size_t i_size, शून्य* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    जइ (i_size > br->remaining) {
        br->error = "the host read past the end of its state";
        प्रत्यागम br->error;
    }

    br->remaining -= i_size;

    प्रत्यागम Get(br->load, o_buffer, i_size);
}

स्थिर
M3Result LoadHostState (M3SnapshotLoad* l)
{
    M3Result               result = m3Err_none;
    ध्रुव M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 size = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    जइ (l->checking) {
        प्रत्यागम SkipBytes(l, size);
    }
    reader.load      = l;
    reader.remaining = size;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, size));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadHeader (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u8       magic[परिमाण(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, परिमाण(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, परिमाण(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    प्रत्यागम result;
}

स्थिर
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

    (शून्य)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->अङ्कः_Continuations));
_   (GetLEB_u32(l, &l->अङ्कः_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->अङ्कः_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->अङ्कः_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    जइ (*o_flags & d_m3SnapshotFlagPostmortem) {
        प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
        }
        प्रत्यागम m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->अङ्कः_Continuations == 0 or activeStacks >= l->अङ्कः_Continuations);
#  जइ !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->अङ्कः_Exceptions != 0);
#  endif

    जइ (l->checking) {
        प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
#  जइ d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tagIndex >= l->module->अङ्कः_Tags);
#  endif
        }
        प्रत्यागम m3Err_none;
    }

    l->check_राशिः = m3_AllocArray(m3slot_t, d_m3_Continuation_राशिः_Slots + 4);
    _throwifnull(l->check_राशिः);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->अङ्कः_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->अङ्कः_Continuations);
    _throwifnull(l->suspendSites);

    जइ (not l->runtime->मूलम्_Continuation) {
        IM3Continuation मूलम् = Continuation_New(l->runtime, रिक्त, रिक्त);
        _throwifnull(मूलम्);

        // the root borrows the runtime's stack, so it holds none of its own
        मूलम्->अङ्कः_राशिः_Slots          = l->runtime->अङ्कः_राशिः_Slots;
        l->runtime->मूलम्_Continuation = मूलम्;
    }

    l->continuations[0] = l->runtime->मूलम्_Continuation;

    प्रति (u32 i = 1; i < l->अङ्कः_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, रिक्त, रिक्त);
        _throwifnull(l->continuations[i]);
    }

#  जइ d_m3HasExceptionHandling
    जइ (l->अङ्कः_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->अङ्कः_Exceptions);
        _throwifnull(l->exceptions);
    }

    प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
        u32    tagIndex = 0;
        IM3Tag tag;
        u32    अङ्कः_Args;

_       (GetLEB_u32(l, &tagIndex));
        _throwif(m3Err_wasmMalformed, tagIndex >= l->module->अङ्कः_Tags);

        tag     = TagOfIndex(l->module, tagIndex);
        अङ्कः_Args = Get_Func_Type_अङ्कः_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, अङ्कः_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  अण्णहा
    प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
        u32 tagIndex = 0;
_       (GetLEB_u32(l, &tagIndex));
    }
#  endif

_catch:
    प्रत्यागम result;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
स्थिर
M3Result LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3Result result = m3Err_none;
    u32      flags  = 0;
    विकल्प (i_sectionId) {
    पक्ष d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        भङ्ग;

    पक्ष d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        भङ्ग;

    पक्ष d_m3SnapshotSection_Table:
_       (LoadTables(l));
        भङ्ग;

    पक्ष d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        भङ्ग;

    पक्ष d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        भङ्ग;

#  जइ d_m3HasExceptionHandling
    पक्ष d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        भङ्ग;
#  endif

    पक्ष d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        भङ्ग;

    पक्ष d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        भङ्ग;

    स्वभाव:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        भङ्ग;
    }

_catch:
    प्रत्यागम result;
}

स्थिर
M3Result LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3Result result  = m3Err_none;
    u8*      payload = रिक्त;

    जइ (not i_embedded) {
_       (LoadHeader(l));
    }

    जाव (true) {
        u8  sectionId   = 0;
        u32 sectionSize = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        जइ (GetU8(l, &sectionId)) {
            भङ्ग;
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
             l->अङ्कः_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->अङ्कः_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->अङ्कः_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->अङ्कः_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->अङ्कः_Data_Segments or l->module->अङ्कः_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    प्रत्यागम result;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#जइ d_m3HasSnapshots

स्थिर
M3Result SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, शून्य* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3Result       result = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, परिमाण(save));

    IM3Continuation मूलम् = io_runtime->मूलम्_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not मूलम् or मूलम्->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : मूलम्->entryFunction;

    जइ (o_module) {
        *o_module = रिक्त;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    जइ (o_module) {
        *o_module = save.module;
    }

    प्रति (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->next) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  जइ d_m3HasExceptionHandling
    प्रति (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->next) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    जइ (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, मूलम्, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    प्रत्यागम result;
}

#endif

M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, शून्य* i_userdata)
{
    जइ (not io_runtime or not i_writer) {
        प्रत्यागम m3Err_mallocFailed;
    }

#जइ d_m3HasSnapshots
    प्रत्यागम SaveSnapshot(io_runtime, i_writer, i_userdata, रिक्त, false);
#अण्णहा
    (शून्य)i_userdata;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#endif
}


स्थिर
M3Result LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, शून्य* i_userdata, bool i_embedded)
{
    M3Result result = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#जइ d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, परिमाण(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        result = LoadSections(&load, i_embedded);

        IM3Continuation मूलम् = io_runtime->मूलम्_Continuation;

        जइ (result) {
            जइ (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                जइ (मूलम्) {
                    मूलम्->state     = cont_returned;
                    मूलम्->अङ्कः_Frames = 0;
                }
            }
        } अण्णहा {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_राशिः(io_runtime);

        m3_Free(load.check_राशिः);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  जइ d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#अण्णहा
    (शून्य)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    प्रत्यागम result;
}


M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, शून्य* i_userdata)
{
    प्रत्यागम LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


शून्य m3_SetSnapshotHooks (IM3Runtime io_runtime, ध्रुव M3SnapshotHooks* i_hooks)
{
#जइ d_m3HasSnapshots
    जइ (io_runtime) {
        जइ (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } अण्णहा {
            memset(&io_runtime->snapshotHooks, 0, परिमाण(io_runtime->snapshotHooks));
        }
    }
#अण्णहा
    (शून्य)io_runtime;
    (शून्य)i_hooks;
#endif
}


M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, शून्य** o_bytes, size_t* o_size)
{
    जइ (!io_runtime || !o_bytes || !o_size) {
        प्रत्यागम m3Err_mallocFailed;
    }
#जइ d_m3HasSnapshots
    M3BufferWriter bw     = { रिक्त, 0, 0 };
    M3Result       result = m3_SaveSnapshot(io_runtime, BufferWriter_Write, &bw);
    जइ (result) {
        m3_Free(bw.buffer);
        *o_bytes = रिक्त;
        *o_size  = 0;
        प्रत्यागम result;
    }
    *o_bytes = bw.buffer;
    *o_size  = bw.size;
    प्रत्यागम m3Err_none;
#अण्णहा
    *o_bytes = रिक्त;
    *o_size  = 0;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#endif
}

#जइ d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
स्थिर
u8* EmitLEB_u32 (u8* o_where, u32 i_मानम्)
{
    करेहि {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        जइ (i_मानम् != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } जाव (i_मानम् != 0);

    प्रत्यागम o_where;
}

स्थिर
u32 SizeOfLEB_u32 (u32 i_मानम्)
{
    u32 size = 0;

    करेहि {
        i_मानम् >>= 7;
        ++size;
    } जाव (i_मानम् != 0);

    प्रत्यागम size;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
स्थिर
M3Result FindEmbeddedSnapshot (ध्रुव u8* i_start, ध्रुव u8* i_end, cstr_t i_secName,
                               ध्रुव u8** o_secStart, ध्रुव u8** o_secEnd,
                               ध्रुव u8** o_data, u32* o_size)
{
    M3Result result = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = रिक्त;

    जइ (o_secStart) {
        *o_secStart = रिक्त;
    }
    जइ (o_secEnd) {
        *o_secEnd = रिक्त;
    }
    जइ (o_data) {
        *o_data = रिक्त;
    }
    जइ (o_size) {
        *o_size = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    जाव (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     size      = 0;
_       (ReadLEB_u32(&size, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, size > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += size;
            जइ (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                जइ (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    जइ (o_secStart) {
                        *o_secStart = secStart;
                    }
                    जइ (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    जइ (o_data) {
                        *o_data = payload;
                    }
                    जइ (o_size) {
                        *o_size = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    प्रत्यागम result;
}

// Builds the custom section name a snapshot of i_name lives under
स्थिर
M3Result SnapshotSectionName (cstr_t i_name, वर्ण* o_secName, size_t i_size)
{
    जइ (i_name and *i_name) {
        जइ (strlen(i_name) > d_m3MaxSnapshotNameLength) {
            प्रत्यागम "the snapshot name is too long";
        }
        snprintf(o_secName, i_size, "snapshot.%s", i_name);
    } अण्णहा {
        snprintf(o_secName, i_size, "snapshot");
    }

    प्रत्यागम m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
स्थिर
M3Result ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3Result result = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = रिक्त;

    प्रति (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->next) {
        जइ (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    प्रत्यागम result;
}
#endif

M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, ध्रुव शून्य* i_bytes, size_t i_size)
{
    जइ (!io_runtime || !i_module || !i_bytes) {
        प्रत्यागम m3Err_mallocFailed;
    }
#जइ d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    जइ (i_size >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        ध्रुव u8* data   = रिक्त;
        u32       size   = 0;
        M3Result  result = FindEmbeddedSnapshot((ध्रुव u8*)i_bytes, (ध्रुव u8*)i_bytes + i_size,
                                                "snapshot", रिक्त, रिक्त, &data, &size);
        जइ (result) {
            प्रत्यागम result;
        }
        जइ (not data) {
            प्रत्यागम "the Wasm binary holds no embedded snapshot";
        }

        {
            M3BufferReader br = { data, size, 0 };
            प्रत्यागम LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
        }
    }
    {
        M3BufferReader br = { (ध्रुव u8*)i_bytes, i_size, 0 };
        प्रत्यागम LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br,
                            i_size < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#अण्णहा
    (शून्य)i_size;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, ध्रुव वर्ण* i_name)
{
    जइ (not i_module) {
        प्रत्यागम false;
    }
#जइ d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = रिक्त;
    (शून्य)ModuleSnapshot(i_module, i_name, &snap);

    प्रत्यागम snap != रिक्त;
#अण्णहा
    (शून्य)i_name;
    प्रत्यागम false;
#endif
}

M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, ध्रुव वर्ण* i_name,
                                 ध्रुव शून्य** o_bytes, size_t* o_size)
{
    जइ (not i_module or not o_bytes or not o_size) {
        प्रत्यागम m3Err_mallocFailed;
    }

    *o_bytes = रिक्त;
    *o_size  = 0;

#जइ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = रिक्त;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);

        जइ (result) {
            प्रत्यागम result;
        }
        जइ (not snap) {
            प्रत्यागम "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->data;
        *o_size  = snap->size;

        प्रत्यागम m3Err_none;
    }
#अण्णहा
    (शून्य)i_name;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#endif
}

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, ध्रुव वर्ण* i_name)
{
    जइ (not io_runtime or not i_module) {
        प्रत्यागम m3Err_mallocFailed;
    }
#जइ d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = रिक्त;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);
        M3BufferReader      br;

        जइ (result) {
            प्रत्यागम result;
        }
        जइ (not snap) {
            प्रत्यागम "the module holds no embedded snapshot of that name";
        }

        br.buffer = snap->data;
        br.size   = snap->size;
        br.cursor = 0;

        प्रत्यागम LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
    }
#अण्णहा
    (शून्य)i_name;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#endif
}


M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, ध्रुव वर्ण* i_name,
                                  शून्य** o_bytes, size_t* o_size)
{
#जइ d_m3HasSnapshots
    M3Result       result = m3Err_none;
    M3BufferWriter bw     = { रिक्त, 0, 0 };
    u8*            out    = रिक्त;

    वर्ण      secName[d_m3MaxSnapshotNameLength + परिमाण("snapshot.")];
    u32       nameLen, nameLenLebSize, payloadSize, payloadSizeLebSize;
    size_t    origSize, totalSize;
    ध्रुव u8* dropStart = रिक्त;
    ध्रुव u8* dropEnd   = रिक्त;
    IM3Module saved     = रिक्त;
    u8*       p;

    जइ (not io_runtime or not i_module or not o_bytes or not o_size) {
        प्रत्यागम m3Err_mallocFailed;
    }

    *o_bytes = रिक्त;
    *o_size  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, परिमाण(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->मूलम्_Continuation or io_runtime->मूलम्_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, BufferWriter_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.size);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.size > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, रिक्त, रिक्त));

    origSize           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    nameLenLebSize     = SizeOfLEB_u32(nameLen);
    payloadSize        = nameLenLebSize + nameLen + (u32)bw.size;
    payloadSizeLebSize = SizeOfLEB_u32(payloadSize);
    totalSize          = origSize + 1 + payloadSizeLebSize + payloadSize;

    out = (u8*)m3_Malloc("wasm_snapshot_module", totalSize);
    _throwifnull(out);

    p = out;

    जइ (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } अण्णहा {
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
    out      = रिक्त;

_catch:
    m3_Free(bw.buffer);
    m3_Free(out);

    प्रत्यागम result;
#अण्णहा
    (शून्य)io_runtime;
    (शून्य)i_module;
    (शून्य)i_name;
    (शून्य)o_bytes;
    (शून्य)o_size;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#endif
}
