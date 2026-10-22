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

#ضمن "m3_env.h"
#ضمن "m3_compile.h"
#ضمن "m3_exception.h"
#ضمن "m3_host.h"

#ضمن <limits.h>

#إذا d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
ساكن ثابت u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
ساكن ثابت u32 c_snapshotVersion  = 1;

تعداد {
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
تعداد {
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
ساكن
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

        بينما (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     size      = 0;

_           (ReadLEB_u32(&size, &pos, end));
            _throwif(m3Err_wasmUnderrun, size > (size_t)(end - pos));

            pos += size;
            إذا (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    إرجاع result;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ساكن
ثابت M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    ثابت M3SnapshotMap* map = i_function->snapshotMap;

    إذا (map) {
        لكل (u32 i = 0; i < map->رقم_Safe_Points; ++i) {
            إذا (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                إرجاع &map->safePoints[i];
            }
        }
    }

    إرجاع عدم;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
ساكن
ثابت M3SafePoint* FindSafePointAt (ثابت M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 أدنى  = 0;
    u32 أعلى = i_map->رقم_Safe_Points;

    بينما (أدنى < أعلى) {
        u32 وسط = أدنى + (أعلى - أدنى) / 2;

        إذا (i_map->safePoints[وسط].wasmOffset < i_wasmOffset) {
            أدنى = وسط + 1;
        } وإلا {
            أعلى = وسط;
        }
    }

    لكل (u32 i = أدنى; i < i_map->رقم_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        ثابت M3SafePoint* point = &i_map->safePoints[i];

        إذا (point->kind != i_kind or (point->flags & d_m3SafePointTailCall)) {
            استمر;
        }
        إذا (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            إرجاع point;
        }
    }

    إرجاع عدم;
}

ساكن
ثابت M3BlockStart* FindBlockAt (ثابت M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    لكل (u32 i = 0; i < i_map->رقم_Blocks; ++i) {
        إذا (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            إرجاع &i_map->blocks[i];
        }
    }

    إرجاع عدم;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ساكن مضمن
bool EnclosesOffset (ثابت M3BlockStart* i_block, u32 i_wasmOffset)
{
    إرجاع i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ساكن مضمن
bool HandlersLiveAt (ثابت M3SnapshotMap* i_map, ثابت M3SafePoint* i_point, ثابت M3BlockStart* i_try)
{
    إرجاع not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ساكن
M3Result CallSiteType (IM3Function i_caller, ثابت M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3Result   result = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        size   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = عدم;

_   (ReadLEB_u32(&size, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    إذا (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->رقم_Functions);
        *o_type = module->functions[index].funcType;
    } وإلا {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->رقم_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    إرجاع result;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ساكن
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 رقم_Results = Get_Func_Type_رقم_Results(i_expected);

    إذا (not i_type or Get_Func_Type_رقم_Results(i_type) != رقم_Results) {
        إرجاع false;
    }

    لكل (u16 i = 0; i < رقم_Results; ++i) {
        إذا (not IsSubTypeOf(GetFuncTypeResultType(i_type, i), GetFuncTypeResultType(i_expected, i))) {
            إرجاع false;
        }
    }

    إرجاع true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
تعريفنوع بنية M3FrameRegisters {
    m3reg_t* r0;
#  إذا d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
ساكن
M3Result Continuation_مكدس (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_رقم_Slots)
{
    إذا (i_cont == i_runtime->جذر_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_مكدس;
        *o_رقم_Slots = i_runtime->رقم_مكدس_Slots;
    } وإلا {
        *o_base     = i_cont->val_مكدس;
        *o_رقم_Slots = i_cont->رقم_مكدس_Slots;
    }

    إرجاع (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ساكن
M3Result Locate_قيمة (m3slot_t* i_base, u32 i_رقم_Slots, m3stack_t i_sp, ثابت M3_Slot_قيمة* i_قيمة, u8** o_where)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_قيمة->type);

    *o_where = عدم;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_قيمة->slot));

    إذا (not IsRegisterSlotAlias(i_قيمة->slot)) {
        size_t size   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? حجم(فراغ*)
                                                                   : (Is64BitType(type) ? حجم(u64) : حجم(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_قيمة->slot) * حجم(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + size > (size_t)i_رقم_Slots * حجم(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    إرجاع result;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ساكن
M3Result Read_قيمة (u8* i_where, ثابت M3FrameRegisters* i_registers, ثابت M3_Slot_قيمة* i_قيمة, u64* o_bits,
                    فراغ** o_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_قيمة->type);

    *o_bits      = 0;
    *o_reference = عدم;

    إذا (i_where) {
        إذا (IsRefType(type)) {
            memcpy(o_reference, i_where, حجم(فراغ*));
        } وإلا إذا (Is64BitType(type)) {
            memcpy(o_bits, i_where, حجم(u64));
        } وإلا {
            u32 narrow;
            memcpy(&narrow, i_where, حجم(narrow));
            *o_bits = narrow;
        }
    } وإلا إذا (IsFpRegisterSlotAlias(i_قيمة->slot)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, حجم(bits));
            *o_bits = bits;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        إذا (IsRefType(type)) {
            *o_reference = (فراغ*)(uintptr_t)r0;
        } وإلا إذا (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } وإلا {
            *o_bits = (u32)r0;
        }
    }

_catch:
    إرجاع result;
}

ساكن
M3Result Write_قيمة (u8* o_where, ثابت M3FrameRegisters* i_registers, ثابت M3_Slot_قيمة* i_قيمة, u64 i_bits,
                     فراغ* i_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_قيمة->type);

    إذا (o_where) {
        إذا (IsRefType(type)) {
            memcpy(o_where, &i_reference, حجم(فراغ*));
        } وإلا إذا (Is64BitType(type)) {
            memcpy(o_where, &i_bits, حجم(u64));
        } وإلا {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, حجم(narrow));
        }
    } وإلا إذا (IsFpRegisterSlotAlias(i_قيمة->slot)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, حجم(narrow));
            *i_registers->fp0 = narrow;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        إذا (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } وإلا {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    إرجاع result;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ساكن
ثابت M3_Slot_قيمة* Frame_قيمة (ثابت M3SnapshotMap* i_map, ثابت M3SafePoint* i_point, u32 i_index)
{
    إذا (i_index < i_map->رقم_Locals) {
        إرجاع i_point->first_Local_قيمة == UINT32_MAX ? &i_map->locals[i_index]
                                                      : &i_map->قيم[i_point->first_Local_قيمة + i_index];
    }
    إرجاع &i_map->قيم[i_point->first_قيمة + i_index - i_map->رقم_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ساكن
فراغ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    إذا (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->رقم_Ret_And_Arg_Slots) + i_function->رقم_Local_Bytes;

        memcpy(where, i_function->constants, i_function->رقم_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
تعريفنوع بنية M3PointerIds {
    فراغ** عناصر;
    u32    تعداد_2;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  indexCapacity;       // a power of two, kept at least twice count
} M3PointerIds;

ساكن
u32 PointerBucket (فراغ* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    إرجاع (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

ساكن
u32 PointerIds_Find (ثابت M3PointerIds* i_ids, فراغ* i_pointer)
{
    إذا (i_ids->indexCapacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->indexCapacity);

        بينما (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            إذا (i_ids->عناصر[id] == i_pointer) {
                إرجاع id;
            }

            bucket = (bucket + 1) & (i_ids->indexCapacity - 1);
        }
    }

    إرجاع d_m3SnapshotNone;
}

ساكن
M3Result PointerIds_Add (M3PointerIds* io_ids, فراغ* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    إذا (id != d_m3SnapshotNone) {
        *o_id = id;
        إرجاع m3Err_none;
    }

    إذا (io_ids->تعداد_2 == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        فراغ** عناصر    = m3_ReallocArray(فراغ*, io_ids->عناصر, capacity, io_ids->capacity);

        إذا (not عناصر) {
            إرجاع m3Err_mallocFailed;
        }

        io_ids->عناصر    = عناصر;
        io_ids->capacity = capacity;
    }

    إذا ((io_ids->تعداد_2 + 1) * 2 > io_ids->indexCapacity) {
        u32  capacity = io_ids->indexCapacity ? io_ids->indexCapacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        إذا (not index) {
            إرجاع m3Err_mallocFailed;
        }

        لكل (u32 i = 0; i < io_ids->تعداد_2; ++i) {
            u32 bucket = PointerBucket(io_ids->عناصر[i], capacity);

            بينما (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->indexCapacity = capacity;
    }

    id                = io_ids->تعداد_2++;
    io_ids->عناصر[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->indexCapacity);

    بينما (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->indexCapacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    إرجاع m3Err_none;
}

ساكن
فراغ PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->عناصر);
    m3_Free(io_ids->index);
    memset(io_ids, 0, حجم(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3BufferWriter {
    u8*    buffer;
    size_t size;
    size_t capacity;
} M3BufferWriter;

ساكن
M3Result BufferWriter_Write (ثابت فراغ* i_data, size_t i_size, فراغ* i_userdata)
{
    M3BufferWriter* bw = (M3BufferWriter*)i_userdata;

    إذا (bw->size + i_size > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        بينما (bw->size + i_size > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->buffer, newCap, bw->capacity);
        إذا (!newBuf) {
            إرجاع m3Err_mallocFailed;
        }
        bw->buffer   = newBuf;
        bw->capacity = newCap;
    }

    إذا (i_size) {
        memcpy(bw->buffer + bw->size, i_data, i_size);
        bw->size += i_size;
    }

    إرجاع m3Err_none;
}

تعريفنوع بنية M3BufferReader {
    ثابت u8* buffer;
    size_t    size;
    size_t    cursor;
} M3BufferReader;

ساكن
M3Result BufferReader_Read (فراغ* o_buffer, size_t i_size, فراغ* i_userdata)
{
    M3BufferReader* br = (M3BufferReader*)i_userdata;

    إذا (i_size > br->size - br->cursor) {
        إرجاع m3Err_wasmMalformed;
    }

    إذا (i_size) {
        memcpy(o_buffer, br->buffer + br->cursor, i_size);
        br->cursor += i_size;
    }

    إرجاع m3Err_none;
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
تعريفنوع بنية M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    فراغ*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

ساكن مضمن
u8 M3TypeToValType (m3type_t i_type)
{
    بدل (BaseTypeOf(i_type)) {
    حالة c_m3Type_i32: إرجاع 0x7F;
    حالة c_m3Type_i64: إرجاع 0x7E;
    حالة c_m3Type_f32: إرجاع 0x7D;
    حالة c_m3Type_f64: إرجاع 0x7C;
    حالة c_m3Type_v128: إرجاع 0x7B;
    حالة c_m3Type_funcref: إرجاع 0x70;
    حالة c_m3Type_externref: إرجاع 0x6F;
    حالة c_m3Type_exnref: إرجاع 0x69;
    حالة c_m3Type_contref: إرجاع 0x68;
    افتراضي: إرجاع 0x00;
    }
}

ساكن مضمن
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    بدل (i_valtype) {
    حالة 0x7F: إرجاع c_m3Type_i32;
    حالة 0x7E: إرجاع c_m3Type_i64;
    حالة 0x7D: إرجاع c_m3Type_f32;
    حالة 0x7C: إرجاع c_m3Type_f64;
    حالة 0x7B: إرجاع c_m3Type_v128;
    حالة 0x70: إرجاع c_m3Type_funcref;
    حالة 0x6F: إرجاع c_m3Type_externref;
    حالة 0x69: إرجاع c_m3Type_exnref;
    حالة 0x68: إرجاع c_m3Type_contref;
    افتراضي: إرجاع c_m3Type_none;
    }
}

ساكن
M3Result Put (M3SnapshotSave* s, ثابت فراغ* i_data, size_t i_size)
{
    إرجاع s->writing ? s->writer(i_data, i_size, s->userdata) : m3Err_none;
}

ساكن
M3Result PutU8 (M3SnapshotSave* s, u8 i_قيمة)
{
    إرجاع Put(s, &i_قيمة, حجم(i_قيمة));
}


ساكن
M3Result PutLEB_u32 (M3SnapshotSave* s, u32 i_قيمة)
{
    u8  bytes[5];
    u32 len = 0;

    افعل {
        u8 byte = i_قيمة & 0x7F;
        i_قيمة >>= 7;
        إذا (i_قيمة != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } بينما (i_قيمة != 0);

    إرجاع Put(s, bytes, len);
}

ساكن
M3Result PutLEB_u64 (M3SnapshotSave* s, u64 i_قيمة)
{
    u8  bytes[10];
    u32 len = 0;

    افعل {
        u8 byte = i_قيمة & 0x7F;
        i_قيمة >>= 7;
        إذا (i_قيمة != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } بينما (i_قيمة != 0);

    إرجاع Put(s, bytes, len);
}

ساكن
M3Result PutLEB_i64 (M3SnapshotSave* s, i64 i_قيمة)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    بينما (more) {
        u8 byte = i_قيمة & 0x7F;
        i_قيمة >>= 7;
        إذا ((i_قيمة == 0 && !(byte & 0x40)) || (i_قيمة == -1 && (byte & 0x40))) {
            more = false;
        } وإلا {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    إرجاع Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
تعريفنوع M3Result (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ساكن
M3Result SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3Result         result   = m3Err_none;
    M3BufferWriter   bw       = { عدم, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    فراغ*            userdata = s->userdata;

    s->writer   = BufferWriter_Write;
    s->userdata = &bw;

    result = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    إذا (result) {
        اذهب _catch;
    }

    // a section with nothing in it is left out
    إذا (bw.size) {
        _throwif("snapshot section is too large", bw.size > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.size));
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    إرجاع result;
}

ساكن
u32 FunctionIndex (IM3Module i_module, IM3Function i_function)
{
    إذا (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->رقم_Functions) {
        إرجاع (u32)(i_function - i_module->functions);
    }

    إرجاع d_m3SnapshotNone;
}

ساكن
u32 FuncTypeIndex (IM3Module i_module, IM3FuncType i_type)
{
    لكل (u32 i = 0; i < i_module->رقم_Func_Types; ++i) {
        إذا (i_module->funcTypes[i] == i_type) {
            إرجاع i;
        }
    }

    إرجاع d_m3SnapshotNone;
}

#  إذا d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
ساكن
IM3Tag TagOfIndex (IM3Module i_module, u32 i_index)
{
    IM3Tag tag = &i_module->tags[i_index];

    إرجاع tag->resolved ? tag->resolved : tag;
}

ساكن
u32 TagIndex (IM3Module i_module, IM3Tag i_tag)
{
    لكل (u32 i = 0; i < i_module->رقم_Tags; ++i) {
        إذا (TagOfIndex(i_module, i) == i_tag) {
            إرجاع i;
        }
    }

    إرجاع d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ساكن
M3Result NameReference (M3SnapshotSave* s, u8 i_type, فراغ* i_reference, u64* o_word)
{
    M3Result result = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    إذا (not i_reference) {
        إرجاع m3Err_none;
    }

    إذا (i_type == c_m3Type_funcref) {
        id = FunctionIndex(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } وإلا إذا (i_type == c_m3Type_externref) {
        ثابت M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        اذهب _catch;
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 تعداد_2 = s->exceptions.تعداد_2;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.تعداد_2 != تعداد_2);
    }
#  endif
    وإلا إذا (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 تعداد_2 = s->continuations.تعداد_2;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.تعداد_2 != تعداد_2);
    } وإلا {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    إرجاع result;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ساكن
M3Result Put_Ref_قيمة (M3SnapshotSave* s, u8 i_type, فراغ* i_reference)
{
    M3Result result = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    إذا (not s->writing) {
        إرجاع m3Err_none;
    }

    إذا (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } وإلا {
        u8 kind = 0;
        إذا (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } وإلا إذا (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  إذا d_m3HasExceptionHandling
        وإلا إذا (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        وإلا إذا (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    إرجاع result;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ساكن
M3Result PutLE32 (M3SnapshotSave* s, u32 i_قيمة)
{
    u8 bytes[4];

    لكل (u32 i = 0; i < حجم(bytes); ++i) {
        bytes[i] = (u8)(i_قيمة >> (8 * i));
    }

    إرجاع Put(s, bytes, حجم(bytes));
}

ساكن
M3Result PutLE64 (M3SnapshotSave* s, u64 i_قيمة)
{
    u8 bytes[8];

    لكل (u32 i = 0; i < حجم(bytes); ++i) {
        bytes[i] = (u8)(i_قيمة >> (8 * i));
    }

    إرجاع Put(s, bytes, حجم(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ساكن
M3Result Put_قيمة_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    إذا (IsRefType(i_type)) {
        إرجاع Put_Ref_قيمة(s, BaseTypeOf(i_type), (فراغ*)(uintptr_t)i_bits);
    }

    بدل (BaseTypeOf(i_type)) {
    حالة c_m3Type_i32: إرجاع PutLEB_i64(s, (i32)(u32)i_bits);
    حالة c_m3Type_i64: إرجاع PutLEB_i64(s, (i64)i_bits);
    حالة c_m3Type_f32: إرجاع PutLE32(s, (u32)i_bits);
    حالة c_m3Type_f64: إرجاع PutLE64(s, i_bits);
    افتراضي: إرجاع "a snapshot cannot hold a value of this type as bits";
    }
}

ساكن
M3Result Put_قيمة_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    إذا (not valtype) {
        إرجاع "a snapshot has no tag for a value of this type";
    }

    إرجاع PutU8(s, valtype);
}

ساكن
M3Result Put_قيمة_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3Result result = m3Err_none;

_   (Put_قيمة_Tag(s, i_type));
_   (Put_قيمة_Body(s, i_type, i_bits));

_catch:
    إرجاع result;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ساكن
M3Result Put_قيمة (M3SnapshotSave* s, m3type_t i_type, ثابت فراغ* i_where)
{
    M3Result result = m3Err_none;

_   (Put_قيمة_Tag(s, i_type));

    إذا (IsRefType(i_type)) {
        فراغ* reference;
        memcpy(&reference, i_where, حجم(reference));
_       (Put_قيمة_Body(s, i_type, (u64)(uintptr_t)reference));
    } وإلا إذا (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, حجم(bytes));
_       (Put(s, bytes, حجم(bytes)));
    } وإلا إذا (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, حجم(bits));
_       (Put_قيمة_Body(s, i_type, bits));
    } وإلا {
        u32 narrow;
        memcpy(&narrow, i_where, حجم(narrow));
_       (Put_قيمة_Body(s, i_type, narrow));
    }

_catch:
    إرجاع result;
}

ساكن
M3Result PutMemoryChunks (M3SnapshotSave* s, ثابت u8* i_bytes, size_t i_size)
{
    M3Result result = m3Err_none;
    size_t   cursor = 0;

    بينما (cursor < i_size) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        بينما (cursor + run < i_size and i_bytes[cursor + run] == byte) {
            run++;
        }

        إذا ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            إذا (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            استمر;
        }

        size_t rawStart = cursor;

        بينما (cursor < i_size) {
            u8     b      = i_bytes[cursor];
            size_t length = 1;

            بينما (cursor + length < i_size and i_bytes[cursor + length] == b) {
                length++;
            }

            إذا ((b == 0x00 or b == 0xFF) and length >= d_m3SnapshotRunThreshold) {
                اكسر;
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
    إرجاع result;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ساكن
u32 Memory_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    بينما (module->memories[j] != module->memories[index]) {
        ++j;
    }
    إرجاع j;
}

ساكن
u32 Table_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    بينما (module->tables[j] != module->tables[index]) {
        ++j;
    }
    إرجاع j;
}

ساكن
M3Result SaveMemories (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_Memories));

    لكل (u32 m = 0; m < module->رقم_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_FirstIndex(module, m);
        bool      hasData  = memory and memory->mallocated;
        u32       pageSize = memory ? Memory_PageSize(memory) : d_m3DefaultMemPageSize;
        u64       bytes    = memory ? memory->رقم_Pages * (u64)pageSize : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        إذا (first != m) {
            استمر;
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
        بينما ((1u << pageBits) < pageSize) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->رقم_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        إذا (not s->writing) {
            استمر;
        }

        إذا (hasData) {
_           (PutMemoryChunks(s, m3MemData(memory->mallocated), (size_t)bytes));
        } وإلا {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    إرجاع result;
}

ساكن
M3Result SaveGlobals (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_Globals));

    لكل (u32 g = 0; g < module->رقم_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_قيمة(s, global->type, &cell->i64_قيمة));
    }

_catch:
    إرجاع result;
}

ساكن
M3Result SaveTables (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_Tables));

    لكل (u32 t = 0; t < module->رقم_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_FirstIndex(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        إذا (first != t) {
            استمر;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->size));

        لكل (u32 e = 0; e < table->size; ++e) {
_           (Put_Ref_قيمة(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    إرجاع result;
}

ساكن
M3Result SaveSegments (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_Data_Segments));

    لكل (u32 i = 0; i < module->رقم_Data_Segments; ++i) {
_       (PutU8(s, module->dataSegments[i].dropped));
    }

_   (PutLEB_u32(s, module->رقم_Element_Segments));

    لكل (u32 i = 0; i < module->رقم_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    إرجاع result;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3Result SaveExceptions (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.تعداد_2));

    لكل (u32 i = 0; i < s->exceptions.تعداد_2; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.عناصر[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->رقم_Args != Get_Func_Type_رقم_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->رقم_Args));

        لكل (u32 a = 0; a < exn->رقم_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_قيمة_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    إرجاع result;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
تعريفنوع بنية M3Activation {
    IM3Function        function;
    ثابت M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    ثابت M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ساكن
ثابت M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    ثابت M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : عدم;

    لكل (u32 i = 0; not point and i < i_module->رقم_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        إذا (point) {
            *io_function = &i_module->functions[i];
        }
    }

    إرجاع point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ساكن
M3Result NextActivation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3Result result = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, حجم(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    لكل (; i >= 0; --i) {
        ثابت M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        إذا (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  إذا d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            اكسر;
        } وإلا إذا (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            اكسر;
        }
    }

    إذا (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  إذا d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  إذا d_m3EntryKeepsFrame
    لكل (i32 k = i_first; k > o_act->lastFrame; --k) {
        ثابت M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    إرجاع result;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ساكن
M3Result SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_رقم_Slots, ثابت M3Activation* i_act)
{
    M3Result             result        = m3Err_none;
    IM3Function          function      = i_act->function;
    ثابت M3SnapshotMap* map           = function->snapshotMap;
    u32                  functionIndex = FunctionIndex(s->module, function);
    ثابت M3SafePoint*   point         = i_act->point;
    u32                  رقم_قيم;

    _throwif("a suspended frame belongs to another module", functionIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, functionIndex));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    إذا (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    رقم_قيم = map->رقم_Locals + point->رقم_قيم;

_   (PutLEB_u32(s, رقم_قيم));

    لكل (u32 v = 0; v < رقم_قيم; ++v) {
        ثابت M3_Slot_قيمة* قيمة = Frame_قيمة(map, point, v);
        u8*                where;
        u64                word;
        فراغ*              reference;

_       (Locate_قيمة(i_base, i_رقم_Slots, i_act->sp, قيمة, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (BaseTypeOf(قيمة->type) == c_m3Type_v128) {
_           (Put_قيمة_Tag(s, قيمة->type));
_           (Put(s, where, 16));
            استمر;
        }

_       (Read_قيمة(where, &i_act->registers, قيمة, &word, &reference));

_       (Put_قيمة_Bits(s, قيمة->type, IsRefType(قيمة->type) ? (u64)(uintptr_t)reference : word));
    }

    إذا (point->kind == safepoint_resume) {
_       (Put_Ref_قيمة(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    إرجاع result;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
ساكن
M3Result SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3Result      result   = m3Err_none;
    M3Activation* acts     = عدم;
    u32           رقم_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->رقم_Frames - 1;
    m3slot_t*     base;
    u32           رقم_Slots;
    m3stack_t     sp;

_   (Continuation_مكدس(s->runtime, i_cont, &base, &رقم_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->رقم_Frames + 1);
    _throwifnull(acts);

    sp = base;

    بينما (true) {
        M3Activation* act = &acts[رقم_Acts];

_       (NextActivation(s->module, i_cont, function, sp, first, act));

        إذا (act->kind != safepoint_call) {
            رقم_Acts++;
            اكسر;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        إذا (not (act->point->flags & d_m3SafePointTailCall)) {
            رقم_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, رقم_Acts));

    لكل (u32 a = 0; a < رقم_Acts; ++a) {
_       (SaveActivation(s, base, رقم_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    إرجاع result;
}

ساكن
M3Result SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3Result  result    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_جذر    = (i_cont == s->runtime->جذر_Continuation);
    u8        state     = snapshot_contFinished;
    u32       typeIndex = is_جذر ? d_m3SnapshotNone : FuncTypeIndex(module, i_cont->type);

    إذا (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } وإلا إذا (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } وإلا إذا (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", FunctionIndex(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_جذر and typeIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_جذر));
_   (PutLEB_u32(s, typeIndex));
_   (PutLEB_u32(s, FunctionIndex(module, i_cont->entryFunction)));
#  إذا d_m3HasExceptionHandling
_   (Put_Ref_قيمة(s, c_m3Type_exnref, i_cont->resumeThrow));
#  وإلا
_   (Put_Ref_قيمة(s, c_m3Type_exnref, عدم));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    إذا (s->postmortem) {
        اذهب _catch;
    }

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : عدم;
        u16         رقم_Rets = inner ? Get_Func_Type_رقم_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_تعداد > Get_Func_Type_رقم_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_تعداد));

        لكل (u32 i = 0; i < i_cont->bound_Args_تعداد; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_قيمة(s, type, i_cont->val_مكدس + (رقم_Rets + i) * c_io_Slot_تعداد));
        }
    } وإلا إذا (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_تعداد));
_       (SaveSuspendedBody(s, i_cont));
    } وإلا {
_       (PutLEB_u32(s, 0));
    }

_catch:
    إرجاع result;
}

ساكن
M3Result SaveContinuations (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->continuations.تعداد_2));

    لكل (u32 i = 0; i < s->continuations.تعداد_2; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.عناصر[i]));
    }

_catch:
    إرجاع result;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ساكن
M3Result SaveHostState (M3SnapshotSave* s)
{
    M3Result               result = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3BufferWriter         bw     = { عدم, 0, 0 };

    إذا (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, BufferWriter_Write, &bw));
    }

    إذا (bw.size > 0) {
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    إرجاع result;
}

ساكن
فراغ SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    لكل (u32 i = 0; i < module->رقم_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        إذا (Memory_FirstIndex(module, i) == i and memory) {
            *memoryBytes += memory->رقم_Pages * (u64)Memory_PageSize(memory);
        }
    }
    لكل (u32 i = 0; i < module->رقم_Tables; ++i) {
        IM3Table table = module->tables[i];
        إذا (Table_FirstIndex(module, i) == i and table) {
            *tableElements += table->size;
        }
    }
}

ساكن
M3Result SaveMeta (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    لكل (u32 i = 0; i < s->continuations.تعداد_2; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.عناصر[i];
        إذا (cont != s->runtime->جذر_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.تعداد_2));
_   (PutLEB_u32(s, s->exceptions.تعداد_2));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  إذا d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    لكل (u32 i = 0; i < s->exceptions.تعداد_2; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.عناصر[i];
        u32          tagIndex  = TagIndex(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tagIndex == d_m3SnapshotNone);

_       (PutLEB_u32(s, tagIndex));
    }
#  endif

_catch:
    إرجاع result;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ساكن
M3Result DiscoverReferences (M3SnapshotSave* s)
{
    M3Result result            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    بينما (doneContinuations < s->continuations.تعداد_2 or doneExceptions < s->exceptions.تعداد_2) {
#  إذا d_m3HasExceptionHandling
        بينما (doneExceptions < s->exceptions.تعداد_2) {
            M3Exception* exception = (M3Exception*)s->exceptions.عناصر[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->رقم_Args != Get_Func_Type_رقم_Params(type));

            لكل (u32 a = 0; a < exception->رقم_Args; ++a) {
_               (Put_قيمة_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        بينما (doneContinuations < s->continuations.تعداد_2 and doneExceptions == s->exceptions.تعداد_2) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.عناصر[id]));
        }
    }

_catch:
    إرجاع result;
}

ساكن
M3Result SaveSections (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

    إذا (not s->writing) {
        إرجاع DiscoverReferences(s);
    }

    إذا (not s->embedded) {
_       (Put(s, c_snapshotMagic, حجم(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    إذا (s->module->رقم_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    إذا (s->module->رقم_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    إذا (s->module->رقم_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    إذا (s->module->رقم_Data_Segments or s->module->رقم_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  إذا d_m3HasExceptionHandling
    إذا (s->exceptions.تعداد_2) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    إذا (s->continuations.تعداد_2) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    إرجاع result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3SnapshotSuspendSite {
    ثابت M3SnapshotMap* map;
    ثابت M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

تعريفنوع بنية M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    فراغ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_مكدس;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    رقم_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  إذا d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 رقم_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

ساكن
M3Result Get (M3SnapshotLoad* l, فراغ* o_data, size_t i_size)
{
    إذا (not l->inSection) {
        إرجاع l->reader(o_data, i_size, l->userdata);
    }
    إذا (i_size > (size_t)(l->end - l->pos)) {
        إرجاع m3Err_wasmUnderrun;
    }
    إذا (i_size) {
        memcpy(o_data, l->pos, i_size);
        l->pos += i_size;
    }
    إرجاع m3Err_none;
}

ساكن
M3Result GetU8 (M3SnapshotLoad* l, u8* o_قيمة)
{
    إرجاع Get(l, o_قيمة, حجم(*o_قيمة));
}

ساكن
M3Result GetBool (M3SnapshotLoad* l, u8* o_قيمة)
{
    M3Result result = GetU8(l, o_قيمة);
    إرجاع result ? result : (*o_قيمة <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ساكن
M3Result GetLEB_u32 (M3SnapshotLoad* l, u32* o_قيمة)
{
    إذا (l->inSection) {
        إرجاع ReadLEB_u32(o_قيمة, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    لكل (u32 i = 0; i < حجم(encoded); ++i) {
        M3Result result = GetU8(l, &encoded[i]);
        إذا (result) {
            إرجاع result;
        }
        إذا (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            إرجاع ReadLEB_u32(o_قيمة, &pos, encoded + i + 1);
        }
    }
    إرجاع m3Err_lebOverflow;
}

ساكن
M3Result GetLEB_u64 (M3SnapshotLoad* l, u64* o_قيمة)
{
    إرجاع ReadLebUnsigned(o_قيمة, 64, &l->pos, l->end);
}

ساكن
M3Result GetLEB_i64 (M3SnapshotLoad* l, i64* o_قيمة)
{
    إرجاع ReadLEB_i64(o_قيمة, &l->pos, l->end);
}

ساكن
M3Result SkipBytes (M3SnapshotLoad* l, size_t i_size)
{
    إذا (i_size > (size_t)(l->end - l->pos)) {
        إرجاع m3Err_wasmUnderrun;
    }
    l->pos += i_size;
    إرجاع m3Err_none;
}


ساكن
M3Result Get_Ref_قيمة (M3SnapshotLoad* l, m3type_t i_type, فراغ** o_reference)
{
    M3Result result = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = عدم;

_   (GetU8(l, &kind));
    إذا (kind == 0x00) {
        إرجاع (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    إذا (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->رقم_Functions);
#  إذا d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } وإلا إذا (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        ثابت M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        إذا (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->رقم_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    وإلا إذا (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->رقم_Continuations);
        *o_reference = l->continuations[id];
#  إذا d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        إذا (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    إرجاع result;
}

ساكن
M3Result GetLE32 (M3SnapshotLoad* l, u32* o_قيمة)
{
    u8       bytes[4];
    M3Result result = Get(l, bytes, حجم(bytes));

    *o_قيمة = 0;

    لكل (u32 i = 0; i < حجم(bytes); ++i) {
        *o_قيمة |= (u32)bytes[i] << (8 * i);
    }

    إرجاع result;
}

ساكن
M3Result GetLE64 (M3SnapshotLoad* l, u64* o_قيمة)
{
    u8       bytes[8];
    M3Result result = Get(l, bytes, حجم(bytes));

    *o_قيمة = 0;

    لكل (u32 i = 0; i < حجم(bytes); ++i) {
        *o_قيمة |= (u64)bytes[i] << (8 * i);
    }

    إرجاع result;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ساكن
M3Result Get_قيمة_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3Result result  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    إذا (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    إرجاع result;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ساكن
M3Result Get_قيمة_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3Result result = m3Err_none;

    *o_bits = 0;

    إذا (IsRefType(i_type)) {
        فراغ* reference = عدم;
_       (Get_Ref_قيمة(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } وإلا إذا (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 قيمة = 0;
_       (ReadLEB_i32(&قيمة, &l->pos, l->end));
        *o_bits = (u32)قيمة;
    } وإلا إذا (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 قيمة = 0;
_       (GetLEB_i64(l, &قيمة));
        *o_bits = (u64)قيمة;
    } وإلا إذا (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } وإلا إذا (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    إرجاع result;
}

ساكن
M3Result Get_قيمة_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3Result result = m3Err_none;
    m3type_t type;

_   (Get_قيمة_Tag(l, i_expectedType, &type));
_   (Get_قيمة_Body(l, type, o_bits));

_catch:
    إرجاع result;
}

ساكن
M3Result Get_قيمة (M3SnapshotLoad* l, m3type_t i_expectedType, فراغ* o_where)
{
    M3Result result = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    إذا (l->checking) {
        o_where = scratch;
    }

_   (Get_قيمة_Tag(l, i_expectedType, &type));

    إذا (IsRefType(type)) {
        فراغ* reference = عدم;
        u64   bits      = 0;
_       (Get_قيمة_Body(l, type, &bits));
        reference = (فراغ*)(uintptr_t)bits;
        memcpy(o_where, &reference, حجم(reference));
    } وإلا إذا (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } وإلا {
        u64 bits = 0;
_       (Get_قيمة_Body(l, type, &bits));

        إذا (Is64BitType(type)) {
            memcpy(o_where, &bits, حجم(bits));
        } وإلا {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, حجم(narrow));
        }
    }

_catch:
    إرجاع result;
}

ساكن
M3Result GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3Result result = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->رقم_Functions);
    *o_function = &l->module->functions[index];

_catch:
    إرجاع result;
}

ساكن
M3Result LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_size)
{
    M3Result result = m3Err_none;

    لكل (;;) {
        u8  chunkType = 0;
        u32 offset = 0, length = 0;

_       (GetU8(l, &chunkType));

        إذا (chunkType == d_m3ChunkEnd) {
            اكسر;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &length));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_size or length > i_size - (size_t)offset);

        إذا (chunkType == d_m3ChunkRaw) {
            إذا (l->checking or length == 0) {
_               (SkipBytes(l, length));
            } وإلا {
_               (Get(l, o_bytes + offset, length));
            }
        } وإلا إذا (chunkType == d_m3ChunkFillFF) {
            إذا (not l->checking and length) {
                memset(o_bytes + offset, 0xFF, length);
            }
        } وإلا {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    إرجاع result;
}

ساكن
M3Result LoadMemories (M3SnapshotLoad* l)
{
    M3Result  result      = m3Err_none;
    IM3Module module      = l->module;
    u32       رقم_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &رقم_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, رقم_Memories == 0 or رقم_Memories != module->رقم_Memories);

    لكل (u32 m = 0; m < رقم_Memories; ++m) {
        u32       memIndex = 0, firstIndex = 0, pageBits = 0, رقم_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &memIndex));
_       (GetLEB_u32(l, &firstIndex));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, memIndex != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 firstIndex != Memory_FirstIndex(module, m));
        إذا (firstIndex != m) {
            استمر;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &رقم_Pages));

        memory = module->memories[memIndex];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    data  = عدم;

        إذا (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_PageSize(memory));
            _throwif(m3Err_wasmMalformed, رقم_Pages < memory->رقم_Pages or رقم_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, رقم_Pages > SIZE_MAX / Memory_PageSize(memory));
            bytes = (size_t)رقم_Pages * Memory_PageSize(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            إذا (not l->checking) {
                إذا (memory->رقم_Pages < رقم_Pages) {
_                   (ResizeMemory(l->runtime, memory, رقم_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->length < bytes);
            }

            l->loadedMemoryBytes += bytes;

            إذا (bytes and memory->mallocated) {
                data = m3MemData(memory->mallocated);
            }
        } وإلا {
            _throwif(m3Err_wasmMalformed, رقم_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        إذا (data and not l->checking) {
            memset(data, 0, bytes);
        }

_       (LoadMemoryChunks(l, data, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    إرجاع result;
}

ساكن
M3Result LoadGlobals (M3SnapshotLoad* l)
{
    M3Result  result     = m3Err_none;
    IM3Module module     = l->module;
    u32       رقم_Globals = 0;

_   (GetLEB_u32(l, &رقم_Globals));
    _throwif(m3Err_wasmMalformed, رقم_Globals == 0 or رقم_Globals != module->رقم_Globals);

    لكل (u32 g = 0; g < رقم_Globals; ++g) {
        u32       globalIndex = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &globalIndex));
        _throwif(m3Err_wasmMalformed, globalIndex != g);

        global = &module->globals[globalIndex];
        cell   = global->resolved ? global->resolved : global;

_       (Get_قيمة(l, global->type, &cell->i64_قيمة));
    }

_catch:
    إرجاع result;
}

ساكن
M3Result LoadTables (M3SnapshotLoad* l)
{
    M3Result  result       = m3Err_none;
    IM3Module module       = l->module;
    u32       رقم_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &رقم_Tables));
    _throwif(m3Err_wasmMalformed, رقم_Tables == 0 or رقم_Tables != module->رقم_Tables);

    لكل (u32 t = 0; t < رقم_Tables; ++t) {
        u32      tableIndex = 0, firstIndex = 0, size = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &tableIndex));
_       (GetLEB_u32(l, &firstIndex));

        _throwif(m3Err_wasmMalformed, tableIndex != t);
        _throwif("the snapshot shares tables between imports differently",
                 firstIndex != Table_FirstIndex(module, t));
        إذا (firstIndex != t) {
            استمر;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &size));

        table = module->tables[tableIndex];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, size > (table->hasMax ? table->maxSize : d_m3MaxSaneTableSize));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, size < table->size);

        l->loadedTableElements += size;

        إذا (not l->checking and size != table->size) {
            فراغ** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 size - table->size > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(فراغ*, table->elements, size, table->size);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += size - table->size;
            table->elements = elements;
            table->size     = size;
        }

        لكل (u32 e = 0; e < size; ++e) {
            فراغ* reference = عدم;
_           (Get_Ref_قيمة(l, table->type, &reference));
            إذا (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    إرجاع result;
}

ساكن
M3Result LoadSegments (M3SnapshotLoad* l)
{
    M3Result  result = m3Err_none;
    IM3Module module = l->module;
    u32       تعداد_2  = 0;

_   (GetLEB_u32(l, &تعداد_2));
    _throwif(m3Err_wasmMalformed, تعداد_2 != module->رقم_Data_Segments);
    _throwif(m3Err_wasmMalformed, module->رقم_Data_Segments == 0 and module->رقم_Element_Segments == 0);

    لكل (u32 i = 0; i < تعداد_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->dataSegments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &تعداد_2));
    _throwif(m3Err_wasmMalformed, تعداد_2 != module->رقم_Element_Segments);

    لكل (u32 i = 0; i < تعداد_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    إرجاع result;
}

// A frame for the continuation being put together, outermost first
ساكن
M3Result AddFrame (M3SnapshotLoad* l, u32* io_رقم_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    إذا (*io_رقم_Frames >= d_m3ContinuationMaxFrames) {
        إرجاع "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_رقم_Frames)++];

    memset(frame, 0, حجم(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    إرجاع m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ساكن
M3Result PrepareFunction (IM3Function i_function)
{
    M3Result result = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    إرجاع result;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ساكن
M3Result AddBlockFrames (M3SnapshotLoad* l, ثابت M3SnapshotMap* i_map, ثابت M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_رقم_Frames)
{
    M3Result result = m3Err_none;

    لكل (u32 b = 0; b < i_map->رقم_Blocks; ++b) {
        ثابت M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        إذا (not EnclosesOffset(block, i_point->wasmOffset)) {
            استمر;
        }

        إذا (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_رقم_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  إذا d_m3HasExceptionHandling
        وإلا {
_           (AddFrame(l, io_رقم_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.رقم_Clauses   = block->رقم_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    إرجاع result;
}

// Puts a function's values where this build keeps them at the safepoint
ساكن
M3Result Load_قيم (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_رقم_Slots, m3stack_t i_sp,
                     ثابت M3SnapshotMap* i_map, ثابت M3SafePoint* i_point, ثابت M3FrameRegisters* i_registers)
{
    M3Result result    = m3Err_none;
    u32      رقم_قيم = 0;

_   (GetLEB_u32(l, &رقم_قيم));
    _throwif("the snapshot's frame does not match what this build compiled", رقم_قيم != i_map->رقم_Locals + i_point->رقم_قيم);

    لكل (u32 v = 0; v < رقم_قيم; ++v) {
        ثابت M3_Slot_قيمة* قيمة     = Frame_قيمة(i_map, i_point, v);
        u64                word      = 0;
        فراغ*              reference = عدم;
        u8*                where;

_       (Locate_قيمة(i_base, i_رقم_Slots, i_sp, قيمة, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (BaseTypeOf(قيمة->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_قيمة_Tag(l, قيمة->type, &type));
            إذا (l->checking) {
_               (SkipBytes(l, 16));
            } وإلا {
_               (Get(l, where, 16));
            }
            استمر;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= رقم_قيم - i_point->رقم_Results;
_       (Get_قيمة_Bits(l, قيمة->type, &word));
        l->allowUnfilledRef = false;

        إذا (IsRefType(قيمة->type)) {
            reference = (فراغ*)(uintptr_t)word;
        }

        إذا (not l->checking) {
_           (Write_قيمة(where, i_registers, قيمة, word, reference));
        }
    }

_catch:
    إرجاع result;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ساكن
M3Result LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3Result    result    = m3Err_none;
    u32         رقم_Frames = 0;
    M3Frame*    callFrame = عدم;
    IM3Function function  = عدم;
    m3slot_t*   base;
    u32         رقم_Slots;
    u32         رقم_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_مكدس(l->runtime, io_cont, &base, &رقم_Slots));

_   (GetLEB_u32(l, &رقم_Activations));
    _throwif(m3Err_wasmMalformed, رقم_Activations == 0 or رقم_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = عدم;
    io_cont->r0                = 0;
    io_cont->رقم_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  إذا d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    لكل (u32 a = 0; a < رقم_Activations; ++a) {
        bool                 isInnermost = (a + 1 == رقم_Activations);
        ثابت M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        ثابت M3SnapshotMap* map;

        memset(&registers, 0, حجم(registers));

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
                 (u32)(sp - base) + function->max_مكدس_Slots > رقم_Slots);

        // the call the frame outside this one is waiting on
        إذا (callFrame) {
            callFrame->call.function = function;
        }

#  إذا d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &رقم_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        إذا (kind == safepoint_op) {
            ثابت M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &رقم_Frames));

        إذا (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &رقم_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  إذا d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } وإلا إذا (kind == safepoint_resume) {
            M3Frame* frame;
            u32      رقم_Results = point->رقم_Results;
            pc_t     resultsPC  = point->pc - 2 * رقم_Results;

_           (AddFrame(l, &رقم_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.رقم_Results  = رقم_Results;
            frame->resume.رقم_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } وإلا {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  إذا d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_قيم(l, base, رقم_Slots, sp, map, point, &registers));

        إذا (not l->checking) {
            RestoreConstants(function, sp);
        }

        إذا (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } وإلا إذا (kind == safepoint_resume) {
            فراغ* cont = عدم;
_           (Get_Ref_قيمة(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == عدم);

            callFrame->resume.cont = (IM3Continuation)cont;
            إذا (not l->checking) {
                l->suspendSites[i_id].point = point;
                لكل (u32 i = 1; i < l->رقم_Continuations; ++i) {
                    إذا (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        اكسر;
                    }
                }
            }
        } وإلا {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            إذا (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            إذا (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->رقم_Results > point->رقم_قيم or
                                                point->رقم_Results > d_m3MaxContinuationPayload);

                io_cont->رقم_Suspend_Results = point->رقم_Results;
                إذا (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                لكل (u32 i = 0; i < point->رقم_Results; ++i) {
                    ثابت M3_Slot_قيمة* قيمة = &map->قيم[point->first_قيمة + point->رقم_قيم - point->رقم_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(قيمة->slot));

                    io_cont->suspendResultOffsets[i] = قيمة->slot;
                    io_cont->suspendResultIs64[i]    = Is64BitType(قيمة->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    إذا (l->checking) {
        إرجاع m3Err_none;
    }

    // held innermost first
    إذا (رقم_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, رقم_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = رقم_Frames;
    }

    لكل (u32 k = 0; k < رقم_Frames; ++k) {
        io_cont->frames[رقم_Frames - 1 - k] = l->frames[k];
    }

    io_cont->رقم_Frames = رقم_Frames;

_catch:
    إرجاع result;
}

ساكن
M3Result LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3Result        result = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    إذا (l->checking) {
        scratch          = *cont;
        scratch.val_مكدس = l->check_مكدس;
        إذا (i_id == 0) {
            scratch.val_مكدس = (m3slot_t*)l->runtime->origin_مكدس;
        }
        cont = &scratch;
    }

    u32 contId = 0, typeIndex = 0, entryIndex = 0, bound_Args_تعداد = 0;
    u8  state = 0, is_جذر = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_جذر));
_   (GetLEB_u32(l, &typeIndex));
_   (GetLEB_u32(l, &entryIndex));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_جذر != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_جذر and state != snapshot_contSuspended);
    إذا (not is_جذر and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, typeIndex != d_m3SnapshotNone and typeIndex >= module->رقم_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entryIndex >= module->رقم_Functions);
    _throwif(m3Err_wasmMalformed, is_جذر ? typeIndex != d_m3SnapshotNone : (typeIndex == d_m3SnapshotNone or not module->funcTypes[typeIndex]->isContinuation));

    إذا (not l->checking and not is_جذر and state != snapshot_contFinished) {
_       (Continuation_Acquire_مكدس(l->runtime, cont));
    }
    cont->type          = (typeIndex != d_m3SnapshotNone) ? module->funcTypes[typeIndex] : عدم;
    cont->entryFunction = &module->functions[entryIndex];
    cont->رقم_Handlers   = 0;
    cont->handlersPC    = عدم;
    cont->parent        = عدم;
    cont->رقم_Frames     = 0;

#  إذا d_m3HasExceptionHandling
    {
        فراغ* ref = عدم;
_       (Get_Ref_قيمة(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_جذر or state == snapshot_contFinished) and cont->resumeThrow != عدم);
#  وإلا
    {
        فراغ* ref = عدم;
_       (Get_Ref_قيمة(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != عدم);
    }
#  endif

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         رقم_Rets;

_       (GetLEB_u32(l, &bound_Args_تعداد));
        cont->bound_Args_تعداد = bound_Args_تعداد;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_تعداد > Get_Func_Type_رقم_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_رقم_Params(remaining) != Get_Func_Type_رقم_Params(inner) - bound_Args_تعداد or
                                            Get_Func_Type_رقم_Results(remaining) != Get_Func_Type_رقم_Results(inner));
            لكل (u16 i = 0; i < Get_Func_Type_رقم_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(inner, i) != GetFuncTypeResultType(remaining, i));
            }
            لكل (u16 i = 0; i < Get_Func_Type_رقم_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_تعداد + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        رقم_Rets = Get_Func_Type_رقم_Results(inner);

        _throwif(m3Err_wasmMalformed, (رقم_Rets + bound_Args_تعداد) * c_io_Slot_تعداد > cont->رقم_مكدس_Slots);

        لكل (u32 i = 0; i < bound_Args_تعداد; ++i) {
_           (Get_قيمة(l, GetFuncTypeParamType(inner, (u16)i), cont->val_مكدس + (رقم_Rets + i) * c_io_Slot_تعداد));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_مكدس;
        cont->pc    = cont->entryFunction->compiled;
    } وإلا إذا (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_تعداد));
        _throwif(m3Err_wasmMalformed, is_جذر and bound_Args_تعداد != 0);
        cont->bound_Args_تعداد = bound_Args_تعداد;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } وإلا إذا (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_تعداد));
        _throwif(m3Err_wasmMalformed, bound_Args_تعداد != 0);
        cont->bound_Args_تعداد = 0;
        cont->state          = cont_consumed;
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    إذا (result) {
        cont->رقم_Frames = 0;
    }

    إرجاع result;
}

ساكن
M3Result LoadContinuations (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u32      تعداد_2  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &تعداد_2));
    _throwif(m3Err_wasmMalformed, تعداد_2 == 0 or تعداد_2 != l->رقم_Continuations);

    لكل (u32 i = 0; i < تعداد_2; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    إرجاع result;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ساكن
M3Result CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;

    لكل (u32 id = 0; id < l->رقم_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  إذا d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        إذا (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : عدم;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_رقم_Results(actual) != Get_Func_Type_رقم_Results(expected));
            لكل (u16 i = 0; i < Get_Func_Type_رقم_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(actual, i) != GetFuncTypeResultType(expected, i));
            }
        }
        بينما (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->رقم_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        إذا (cont->state != cont_suspended or id == 0) {
            استمر;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        إذا (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_تعداد != 0);
            استمر;
        }

        ثابت M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_تعداد > inner->رقم_Suspend_Results or
                                                                          Get_Func_Type_رقم_Params(remaining) != inner->رقم_Suspend_Results - cont->bound_Args_تعداد);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_رقم_Results(remaining) != Get_Func_Type_رقم_Results(cont->entryFunction->funcType));
        لكل (u16 i = 0; i < Get_Func_Type_رقم_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(remaining, i) !=
                                            GetFuncTypeResultType(cont->entryFunction->funcType, i));
        }

        لكل (u32 i = 0; i < inner->رقم_Suspend_Results; ++i) {
            ثابت M3_Slot_قيمة* قيمة = &site->map->قيم[site->point->first_قيمة +
                                                          site->point->رقم_قيم - site->point->رقم_Results + i];
            إذا (i < cont->bound_Args_تعداد) {
                إذا (IsRefType(قيمة->type) and not IsNullableRef(قيمة->type)) {
                    فراغ* ref = عدم;
                    memcpy(&ref, inner->sp + قيمة->slot, حجم(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } وإلا {
                _throwif("a captured continuation has the wrong parameter type", قيمة->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_تعداد)));
            }
        }
    }

_catch:
    إرجاع result;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3Result LoadExceptions (M3SnapshotLoad* l)
{
    M3Result result        = m3Err_none;
    u32      رقم_Exceptions = 0;

_   (GetLEB_u32(l, &رقم_Exceptions));
    _throwif(m3Err_wasmMalformed, رقم_Exceptions == 0 or رقم_Exceptions != l->رقم_Exceptions);

    لكل (u32 i = 0; i < رقم_Exceptions; ++i) {
        u32          exnId = 0, رقم_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &رقم_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, رقم_Args != exn->رقم_Args);

        لكل (u32 a = 0; a < رقم_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_قيمة_Bits(l, argType, &bits));
            إذا (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    إرجاع result;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
تعريفنوع بنية M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3Result        error;
} M3BoundedReader;

ساكن
M3Result BoundedReader_Read (فراغ* o_buffer, size_t i_size, فراغ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    إذا (i_size > br->remaining) {
        br->error = "the host read past the end of its state";
        إرجاع br->error;
    }

    br->remaining -= i_size;

    إرجاع Get(br->load, o_buffer, i_size);
}

ساكن
M3Result LoadHostState (M3SnapshotLoad* l)
{
    M3Result               result = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 size = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    إذا (l->checking) {
        إرجاع SkipBytes(l, size);
    }
    reader.load      = l;
    reader.remaining = size;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, size));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    إرجاع result;
}

ساكن
M3Result LoadHeader (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u8       magic[حجم(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, حجم(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, حجم(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    إرجاع result;
}

ساكن
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

    (فراغ)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->رقم_Continuations));
_   (GetLEB_u32(l, &l->رقم_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->رقم_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->رقم_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    إذا (*o_flags & d_m3SnapshotFlagPostmortem) {
        لكل (u32 i = 0; i < l->رقم_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
        }
        إرجاع m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->رقم_Continuations == 0 or activeStacks >= l->رقم_Continuations);
#  إذا !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->رقم_Exceptions != 0);
#  endif

    إذا (l->checking) {
        لكل (u32 i = 0; i < l->رقم_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
#  إذا d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tagIndex >= l->module->رقم_Tags);
#  endif
        }
        إرجاع m3Err_none;
    }

    l->check_مكدس = m3_AllocArray(m3slot_t, d_m3_Continuation_مكدس_Slots + 4);
    _throwifnull(l->check_مكدس);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->رقم_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->رقم_Continuations);
    _throwifnull(l->suspendSites);

    إذا (not l->runtime->جذر_Continuation) {
        IM3Continuation جذر = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(جذر);

        // the root borrows the runtime's stack, so it holds none of its own
        جذر->رقم_مكدس_Slots          = l->runtime->رقم_مكدس_Slots;
        l->runtime->جذر_Continuation = جذر;
    }

    l->continuations[0] = l->runtime->جذر_Continuation;

    لكل (u32 i = 1; i < l->رقم_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(l->continuations[i]);
    }

#  إذا d_m3HasExceptionHandling
    إذا (l->رقم_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->رقم_Exceptions);
        _throwifnull(l->exceptions);
    }

    لكل (u32 i = 0; i < l->رقم_Exceptions; ++i) {
        u32    tagIndex = 0;
        IM3Tag tag;
        u32    رقم_Args;

_       (GetLEB_u32(l, &tagIndex));
        _throwif(m3Err_wasmMalformed, tagIndex >= l->module->رقم_Tags);

        tag     = TagOfIndex(l->module, tagIndex);
        رقم_Args = Get_Func_Type_رقم_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, رقم_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  وإلا
    لكل (u32 i = 0; i < l->رقم_Exceptions; ++i) {
        u32 tagIndex = 0;
_       (GetLEB_u32(l, &tagIndex));
    }
#  endif

_catch:
    إرجاع result;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ساكن
M3Result LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3Result result = m3Err_none;
    u32      flags  = 0;
    بدل (i_sectionId) {
    حالة d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        اكسر;

    حالة d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        اكسر;

    حالة d_m3SnapshotSection_Table:
_       (LoadTables(l));
        اكسر;

    حالة d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        اكسر;

    حالة d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        اكسر;

#  إذا d_m3HasExceptionHandling
    حالة d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        اكسر;
#  endif

    حالة d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        اكسر;

    حالة d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        اكسر;

    افتراضي:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        اكسر;
    }

_catch:
    إرجاع result;
}

ساكن
M3Result LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3Result result  = m3Err_none;
    u8*      payload = عدم;

    إذا (not i_embedded) {
_       (LoadHeader(l));
    }

    بينما (true) {
        u8  sectionId   = 0;
        u32 sectionSize = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        إذا (GetU8(l, &sectionId)) {
            اكسر;
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
             l->رقم_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->رقم_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->رقم_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->رقم_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->رقم_Data_Segments or l->module->رقم_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    إرجاع result;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3HasSnapshots

ساكن
M3Result SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3Result       result = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, حجم(save));

    IM3Continuation جذر = io_runtime->جذر_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not جذر or جذر->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : جذر->entryFunction;

    إذا (o_module) {
        *o_module = عدم;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    إذا (o_module) {
        *o_module = save.module;
    }

    لكل (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->next) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  إذا d_m3HasExceptionHandling
    لكل (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->next) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    إذا (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, جذر, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    إرجاع result;
}

#endif

M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata)
{
    إذا (not io_runtime or not i_writer) {
        إرجاع m3Err_mallocFailed;
    }

#إذا d_m3HasSnapshots
    إرجاع SaveSnapshot(io_runtime, i_writer, i_userdata, عدم, false);
#وإلا
    (فراغ)i_userdata;
    إرجاع "snapshots are not available in this build of Wasm3";
#endif
}


ساكن
M3Result LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata, bool i_embedded)
{
    M3Result result = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#إذا d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, حجم(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        result = LoadSections(&load, i_embedded);

        IM3Continuation جذر = io_runtime->جذر_Continuation;

        إذا (result) {
            إذا (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                إذا (جذر) {
                    جذر->state     = cont_returned;
                    جذر->رقم_Frames = 0;
                }
            }
        } وإلا {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_مكدس(io_runtime);

        m3_Free(load.check_مكدس);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  إذا d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#وإلا
    (فراغ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    إرجاع result;
}


M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata)
{
    إرجاع LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


فراغ m3_SetSnapshotHooks (IM3Runtime io_runtime, ثابت M3SnapshotHooks* i_hooks)
{
#إذا d_m3HasSnapshots
    إذا (io_runtime) {
        إذا (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } وإلا {
            memset(&io_runtime->snapshotHooks, 0, حجم(io_runtime->snapshotHooks));
        }
    }
#وإلا
    (فراغ)io_runtime;
    (فراغ)i_hooks;
#endif
}


M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, فراغ** o_bytes, size_t* o_size)
{
    إذا (!io_runtime || !o_bytes || !o_size) {
        إرجاع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    M3BufferWriter bw     = { عدم, 0, 0 };
    M3Result       result = m3_SaveSnapshot(io_runtime, BufferWriter_Write, &bw);
    إذا (result) {
        m3_Free(bw.buffer);
        *o_bytes = عدم;
        *o_size  = 0;
        إرجاع result;
    }
    *o_bytes = bw.buffer;
    *o_size  = bw.size;
    إرجاع m3Err_none;
#وإلا
    *o_bytes = عدم;
    *o_size  = 0;
    إرجاع "snapshots are not available in this build of Wasm3";
#endif
}

#إذا d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ساكن
u8* EmitLEB_u32 (u8* o_where, u32 i_قيمة)
{
    افعل {
        u8 byte = i_قيمة & 0x7F;
        i_قيمة >>= 7;
        إذا (i_قيمة != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } بينما (i_قيمة != 0);

    إرجاع o_where;
}

ساكن
u32 SizeOfLEB_u32 (u32 i_قيمة)
{
    u32 size = 0;

    افعل {
        i_قيمة >>= 7;
        ++size;
    } بينما (i_قيمة != 0);

    إرجاع size;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ساكن
M3Result FindEmbeddedSnapshot (ثابت u8* i_start, ثابت u8* i_end, cstr_t i_secName,
                               ثابت u8** o_secStart, ثابت u8** o_secEnd,
                               ثابت u8** o_data, u32* o_size)
{
    M3Result result = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = عدم;

    إذا (o_secStart) {
        *o_secStart = عدم;
    }
    إذا (o_secEnd) {
        *o_secEnd = عدم;
    }
    إذا (o_data) {
        *o_data = عدم;
    }
    إذا (o_size) {
        *o_size = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    بينما (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     size      = 0;
_       (ReadLEB_u32(&size, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, size > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += size;
            إذا (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                إذا (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    إذا (o_secStart) {
                        *o_secStart = secStart;
                    }
                    إذا (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    إذا (o_data) {
                        *o_data = payload;
                    }
                    إذا (o_size) {
                        *o_size = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    إرجاع result;
}

// Builds the custom section name a snapshot of i_name lives under
ساكن
M3Result SnapshotSectionName (cstr_t i_name, حرف* o_secName, size_t i_size)
{
    إذا (i_name and *i_name) {
        إذا (strlen(i_name) > d_m3MaxSnapshotNameLength) {
            إرجاع "the snapshot name is too long";
        }
        snprintf(o_secName, i_size, "snapshot.%s", i_name);
    } وإلا {
        snprintf(o_secName, i_size, "snapshot");
    }

    إرجاع m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ساكن
M3Result ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3Result result = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = عدم;

    لكل (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->next) {
        إذا (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    إرجاع result;
}
#endif

M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, ثابت فراغ* i_bytes, size_t i_size)
{
    إذا (!io_runtime || !i_module || !i_bytes) {
        إرجاع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    إذا (i_size >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        ثابت u8* data   = عدم;
        u32       size   = 0;
        M3Result  result = FindEmbeddedSnapshot((ثابت u8*)i_bytes, (ثابت u8*)i_bytes + i_size,
                                                "snapshot", عدم, عدم, &data, &size);
        إذا (result) {
            إرجاع result;
        }
        إذا (not data) {
            إرجاع "the Wasm binary holds no embedded snapshot";
        }

        {
            M3BufferReader br = { data, size, 0 };
            إرجاع LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
        }
    }
    {
        M3BufferReader br = { (ثابت u8*)i_bytes, i_size, 0 };
        إرجاع LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br,
                            i_size < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#وإلا
    (فراغ)i_size;
    إرجاع "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, ثابت حرف* i_name)
{
    إذا (not i_module) {
        إرجاع false;
    }
#إذا d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = عدم;
    (فراغ)ModuleSnapshot(i_module, i_name, &snap);

    إرجاع snap != عدم;
#وإلا
    (فراغ)i_name;
    إرجاع false;
#endif
}

M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, ثابت حرف* i_name,
                                 ثابت فراغ** o_bytes, size_t* o_size)
{
    إذا (not i_module or not o_bytes or not o_size) {
        إرجاع m3Err_mallocFailed;
    }

    *o_bytes = عدم;
    *o_size  = 0;

#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);

        إذا (result) {
            إرجاع result;
        }
        إذا (not snap) {
            إرجاع "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->data;
        *o_size  = snap->size;

        إرجاع m3Err_none;
    }
#وإلا
    (فراغ)i_name;
    إرجاع "snapshots are not available in this build of Wasm3";
#endif
}

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name)
{
    إذا (not io_runtime or not i_module) {
        إرجاع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);
        M3BufferReader      br;

        إذا (result) {
            إرجاع result;
        }
        إذا (not snap) {
            إرجاع "the module holds no embedded snapshot of that name";
        }

        br.buffer = snap->data;
        br.size   = snap->size;
        br.cursor = 0;

        إرجاع LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
    }
#وإلا
    (فراغ)i_name;
    إرجاع "snapshots are not available in this build of Wasm3";
#endif
}


M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name,
                                  فراغ** o_bytes, size_t* o_size)
{
#إذا d_m3HasSnapshots
    M3Result       result = m3Err_none;
    M3BufferWriter bw     = { عدم, 0, 0 };
    u8*            out    = عدم;

    حرف      secName[d_m3MaxSnapshotNameLength + حجم("snapshot.")];
    u32       nameLen, nameLenLebSize, payloadSize, payloadSizeLebSize;
    size_t    origSize, totalSize;
    ثابت u8* dropStart = عدم;
    ثابت u8* dropEnd   = عدم;
    IM3Module saved     = عدم;
    u8*       p;

    إذا (not io_runtime or not i_module or not o_bytes or not o_size) {
        إرجاع m3Err_mallocFailed;
    }

    *o_bytes = عدم;
    *o_size  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, حجم(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->جذر_Continuation or io_runtime->جذر_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, BufferWriter_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.size);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.size > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, عدم, عدم));

    origSize           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    nameLenLebSize     = SizeOfLEB_u32(nameLen);
    payloadSize        = nameLenLebSize + nameLen + (u32)bw.size;
    payloadSizeLebSize = SizeOfLEB_u32(payloadSize);
    totalSize          = origSize + 1 + payloadSizeLebSize + payloadSize;

    out = (u8*)m3_Malloc("wasm_snapshot_module", totalSize);
    _throwifnull(out);

    p = out;

    إذا (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } وإلا {
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
    out      = عدم;

_catch:
    m3_Free(bw.buffer);
    m3_Free(out);

    إرجاع result;
#وإلا
    (فراغ)io_runtime;
    (فراغ)i_module;
    (فراغ)i_name;
    (فراغ)o_bytes;
    (فراغ)o_size;
    إرجاع "snapshots are not available in this build of Wasm3";
#endif
}
