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

#समावेश "m3_env.h"
#समावेश "m3_compile.h"
#समावेश "m3_exception.h"
#समावेश "m3_host.h"

#समावेश <limits.h>

#अगर d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
अटल स्थिर u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
अटल स्थिर u32 c_snapshotVersion  = 1;

क्रमागत {
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
#  define d_m3_Max_Snapshot_Name_लंबाई   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
क्रमागत {
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
अटल
M3_परिणाम ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_परिणाम परिणाम = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasmStart or i_module->wasmEnd - i_module->wasmStart < 8);

    {
        bytes_t  pos = i_module->wasmStart + 8;
        cbytes_t end = i_module->wasmEnd;

        Xxh64_Init(&hash, 0);

        जबतक (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     आकार      = 0;

_           (ReadLEB_u32(&आकार, &pos, end));
            _throwif(m3Err_wasmUnderrun, आकार > (माप_प्रकार)(end - pos));

            pos += आकार;
            अगर (sectionId != 0) {
                Xxh64_Update(&hash, start, (माप_प्रकार)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    वापस परिणाम;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

अटल
स्थिर M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    स्थिर M3SnapshotMap* map = i_function->snapshotMap;

    अगर (map) {
        क्रम (u32 i = 0; i < map->अंक_Safe_Points; ++i) {
            अगर (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                वापस &map->safePoints[i];
            }
        }
    }

    वापस शून्य;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
अटल
स्थिर M3SafePoint* FindSafePointAt (स्थिर M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 निचला  = 0;
    u32 ऊपरी = i_map->अंक_Safe_Points;

    जबतक (निचला < ऊपरी) {
        u32 मध्य = निचला + (ऊपरी - निचला) / 2;

        अगर (i_map->safePoints[मध्य].wasmOffset < i_wasmOffset) {
            निचला = मध्य + 1;
        } अन्यथा {
            ऊपरी = मध्य;
        }
    }

    क्रम (u32 i = निचला; i < i_map->अंक_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        स्थिर M3SafePoint* point = &i_map->safePoints[i];

        अगर (point->kind != i_kind or (point->flags & d_m3_Safe_Point_पुच्छ_Call)) {
            जारी;
        }
        अगर (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            वापस point;
        }
    }

    वापस शून्य;
}

अटल
स्थिर M3BlockStart* FindBlockAt (स्थिर M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    क्रम (u32 i = 0; i < i_map->अंक_Blocks; ++i) {
        अगर (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            वापस &i_map->blocks[i];
        }
    }

    वापस शून्य;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
अटल अंतरभूत
bool EnclosesOffset (स्थिर M3BlockStart* i_block, u32 i_wasmOffset)
{
    वापस i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
अटल अंतरभूत
bool HandlersLiveAt (स्थिर M3SnapshotMap* i_map, स्थिर M3SafePoint* i_point, स्थिर M3BlockStart* i_try)
{
    वापस not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
अटल
M3_परिणाम CallSiteType (IM3Function i_caller, स्थिर M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3_परिणाम   परिणाम = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        आकार   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = शून्य;

_   (ReadLEB_u32(&आकार, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (माप_प्रकार)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    अगर (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->अंक_Functions);
        *o_type = module->functions[index].funcType;
    } अन्यथा {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->अंक_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    वापस परिणाम;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
अटल
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 अंक_Results = Get_Func_Type_अंक_Results(i_expected);

    अगर (not i_type or Get_Func_Type_अंक_Results(i_type) != अंक_Results) {
        वापस false;
    }

    क्रम (u16 i = 0; i < अंक_Results; ++i) {
        अगर (not IsSubTypeOf(Get_Func_Type_परिणाम_Type(i_type, i), Get_Func_Type_परिणाम_Type(i_expected, i))) {
            वापस false;
        }
    }

    वापस true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
प्रकार काष्ठा M3FrameRegisters {
    m3reg_t* r0;
#  अगर d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
अटल
M3_परिणाम Continuation_ढेर (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_अंक_Slots)
{
    अगर (i_cont == i_runtime->मूल_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_ढेर;
        *o_अंक_Slots = i_runtime->अंक_ढेर_Slots;
    } अन्यथा {
        *o_base     = i_cont->val_ढेर;
        *o_अंक_Slots = i_cont->अंक_ढेर_Slots;
    }

    वापस (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
अटल
M3_परिणाम Locate_मूल्य (m3slot_t* i_base, u32 i_अंक_Slots, m3stack_t i_sp, स्थिर M3_Slot_मूल्य* i_मूल्य, u8** o_where)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       type   = BaseTypeOf(i_मूल्य->type);

    *o_where = शून्य;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_मूल्य->slot));

    अगर (not IsRegisterSlotAlias(i_मूल्य->slot)) {
        माप_प्रकार आकार   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? माप(व्योम*)
                                                                   : (Is64BitType(type) ? माप(u64) : माप(u32)));
        माप_प्रकार offset = (माप_प्रकार)((m3slot_t*)i_sp - i_base + i_मूल्य->slot) * माप(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + आकार > (माप_प्रकार)i_अंक_Slots * माप(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    वापस परिणाम;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
अटल
M3_परिणाम Read_मूल्य (u8* i_where, स्थिर M3FrameRegisters* i_registers, स्थिर M3_Slot_मूल्य* i_मूल्य, u64* o_bits,
                    व्योम** o_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       type   = BaseTypeOf(i_मूल्य->type);

    *o_bits      = 0;
    *o_reference = शून्य;

    अगर (i_where) {
        अगर (IsRefType(type)) {
            स_नकल(o_reference, i_where, माप(व्योम*));
        } अन्यथा अगर (Is64BitType(type)) {
            स_नकल(o_bits, i_where, माप(u64));
        } अन्यथा {
            u32 narrow;
            स_नकल(&narrow, i_where, माप(narrow));
            *o_bits = narrow;
        }
    } अन्यथा अगर (IsFpRegisterSlotAlias(i_मूल्य->slot)) {
#  अगर d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        अगर (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            स_नकल(&bits, &narrow, माप(bits));
            *o_bits = bits;
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            स_नकल(o_bits, i_registers->fp0, माप(u64));
        }
#  अन्यथा
        _throw(m3Err_wasmMalformed);
#  endif
    } अन्यथा {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        अगर (IsRefType(type)) {
            *o_reference = (व्योम*)(uintptr_t)r0;
        } अन्यथा अगर (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } अन्यथा {
            *o_bits = (u32)r0;
        }
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम Write_मूल्य (u8* o_where, स्थिर M3FrameRegisters* i_registers, स्थिर M3_Slot_मूल्य* i_मूल्य, u64 i_bits,
                     व्योम* i_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       type   = BaseTypeOf(i_मूल्य->type);

    अगर (o_where) {
        अगर (IsRefType(type)) {
            स_नकल(o_where, &i_reference, माप(व्योम*));
        } अन्यथा अगर (Is64BitType(type)) {
            स_नकल(o_where, &i_bits, माप(u64));
        } अन्यथा {
            u32 narrow = (u32)i_bits;
            स_नकल(o_where, &narrow, माप(narrow));
        }
    } अन्यथा अगर (IsFpRegisterSlotAlias(i_मूल्य->slot)) {
#  अगर d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        अगर (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            स_नकल(&narrow, &bits, माप(narrow));
            *i_registers->fp0 = narrow;
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            स_नकल(i_registers->fp0, &i_bits, माप(u64));
        }
#  अन्यथा
        _throw(m3Err_wasmMalformed);
#  endif
    } अन्यथा {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        अगर (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } अन्यथा {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    वापस परिणाम;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
अटल
स्थिर M3_Slot_मूल्य* Frame_मूल्य (स्थिर M3SnapshotMap* i_map, स्थिर M3SafePoint* i_point, u32 i_सूचकांक)
{
    अगर (i_सूचकांक < i_map->अंक_Locals) {
        वापस i_point->first_Local_मूल्य == UINT32_MAX ? &i_map->locals[i_सूचकांक]
                                                      : &i_map->मूल्यावली[i_point->first_Local_मूल्य + i_सूचकांक];
    }
    वापस &i_map->मूल्यावली[i_point->first_मूल्य + i_सूचकांक - i_map->अंक_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
अटल
व्योम RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    अगर (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->अंक_Ret_And_Arg_Slots) + i_function->अंक_Local_Bytes;

        स_नकल(where, i_function->constants, i_function->अंक_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
प्रकार काष्ठा M3PointerIds {
    व्योम** वस्तुएँ;
    u32    गिनती;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  सूचकांक_Capacity;       // a power of two, kept at least twice count
} M3PointerIds;

अटल
u32 PointerBucket (व्योम* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    वापस (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

अटल
u32 PointerIds_Find (स्थिर M3PointerIds* i_ids, व्योम* i_pointer)
{
    अगर (i_ids->सूचकांक_Capacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->सूचकांक_Capacity);

        जबतक (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            अगर (i_ids->वस्तुएँ[id] == i_pointer) {
                वापस id;
            }

            bucket = (bucket + 1) & (i_ids->सूचकांक_Capacity - 1);
        }
    }

    वापस d_m3SnapshotNone;
}

अटल
M3_परिणाम PointerIds_Add (M3PointerIds* io_ids, व्योम* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    अगर (id != d_m3SnapshotNone) {
        *o_id = id;
        वापस m3Err_none;
    }

    अगर (io_ids->गिनती == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        व्योम** वस्तुएँ    = m3_ReallocArray(व्योम*, io_ids->वस्तुएँ, capacity, io_ids->capacity);

        अगर (not वस्तुएँ) {
            वापस m3Err_mallocFailed;
        }

        io_ids->वस्तुएँ    = वस्तुएँ;
        io_ids->capacity = capacity;
    }

    अगर ((io_ids->गिनती + 1) * 2 > io_ids->सूचकांक_Capacity) {
        u32  capacity = io_ids->सूचकांक_Capacity ? io_ids->सूचकांक_Capacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        अगर (not index) {
            वापस m3Err_mallocFailed;
        }

        क्रम (u32 i = 0; i < io_ids->गिनती; ++i) {
            u32 bucket = PointerBucket(io_ids->वस्तुएँ[i], capacity);

            जबतक (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->सूचकांक_Capacity = capacity;
    }

    id                = io_ids->गिनती++;
    io_ids->वस्तुएँ[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->सूचकांक_Capacity);

    जबतक (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->सूचकांक_Capacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    वापस m3Err_none;
}

अटल
व्योम PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->वस्तुएँ);
    m3_Free(io_ids->index);
    स_रखो(io_ids, 0, माप(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

प्रकार काष्ठा M3_बफर_Writer {
    u8*    बफर;
    माप_प्रकार आकार;
    माप_प्रकार capacity;
} M3_बफर_Writer;

अटल
M3_परिणाम बफर_Writer_Write (स्थिर व्योम* i_तथ्य, माप_प्रकार i_आकार, व्योम* i_userdata)
{
    M3_बफर_Writer* bw = (M3_बफर_Writer*)i_userdata;

    अगर (bw->आकार + i_आकार > bw->capacity) {
        माप_प्रकार newCap = bw->capacity ? bw->capacity * 2 : 4096;
        जबतक (bw->आकार + i_आकार > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->बफर, newCap, bw->capacity);
        अगर (!newBuf) {
            वापस m3Err_mallocFailed;
        }
        bw->बफर   = newBuf;
        bw->capacity = newCap;
    }

    अगर (i_आकार) {
        स_नकल(bw->बफर + bw->आकार, i_तथ्य, i_आकार);
        bw->आकार += i_आकार;
    }

    वापस m3Err_none;
}

प्रकार काष्ठा M3_बफर_Reader {
    स्थिर u8* बफर;
    माप_प्रकार    आकार;
    माप_प्रकार    cursor;
} M3_बफर_Reader;

अटल
M3_परिणाम बफर_Reader_Read (व्योम* o_बफर, माप_प्रकार i_आकार, व्योम* i_userdata)
{
    M3_बफर_Reader* br = (M3_बफर_Reader*)i_userdata;

    अगर (i_आकार > br->आकार - br->cursor) {
        वापस m3Err_wasmMalformed;
    }

    अगर (i_आकार) {
        स_नकल(o_बफर, br->बफर + br->cursor, i_आकार);
        br->cursor += i_आकार;
    }

    वापस m3Err_none;
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
प्रकार काष्ठा M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    व्योम*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

अटल अंतरभूत
u8 M3TypeToValType (m3type_t i_type)
{
    चयन (BaseTypeOf(i_type)) {
    हाल c_m3Type_i32: वापस 0x7F;
    हाल c_m3Type_i64: वापस 0x7E;
    हाल c_m3Type_f32: वापस 0x7D;
    हाल c_m3Type_f64: वापस 0x7C;
    हाल c_m3Type_v128: वापस 0x7B;
    हाल c_m3Type_funcref: वापस 0x70;
    हाल c_m3Type_externref: वापस 0x6F;
    हाल c_m3Type_exnref: वापस 0x69;
    हाल c_m3Type_contref: वापस 0x68;
    शेष: वापस 0x00;
    }
}

अटल अंतरभूत
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    चयन (i_valtype) {
    हाल 0x7F: वापस c_m3Type_i32;
    हाल 0x7E: वापस c_m3Type_i64;
    हाल 0x7D: वापस c_m3Type_f32;
    हाल 0x7C: वापस c_m3Type_f64;
    हाल 0x7B: वापस c_m3Type_v128;
    हाल 0x70: वापस c_m3Type_funcref;
    हाल 0x6F: वापस c_m3Type_externref;
    हाल 0x69: वापस c_m3Type_exnref;
    हाल 0x68: वापस c_m3Type_contref;
    शेष: वापस c_m3Type_none;
    }
}

अटल
M3_परिणाम Put (M3SnapshotSave* s, स्थिर व्योम* i_तथ्य, माप_प्रकार i_आकार)
{
    वापस s->writing ? s->writer(i_तथ्य, i_आकार, s->userdata) : m3Err_none;
}

अटल
M3_परिणाम PutU8 (M3SnapshotSave* s, u8 i_मूल्य)
{
    वापस Put(s, &i_मूल्य, माप(i_मूल्य));
}


अटल
M3_परिणाम PutLEB_u32 (M3SnapshotSave* s, u32 i_मूल्य)
{
    u8  bytes[5];
    u32 len = 0;

    करो {
        u8 byte = i_मूल्य & 0x7F;
        i_मूल्य >>= 7;
        अगर (i_मूल्य != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } जबतक (i_मूल्य != 0);

    वापस Put(s, bytes, len);
}

अटल
M3_परिणाम PutLEB_u64 (M3SnapshotSave* s, u64 i_मूल्य)
{
    u8  bytes[10];
    u32 len = 0;

    करो {
        u8 byte = i_मूल्य & 0x7F;
        i_मूल्य >>= 7;
        अगर (i_मूल्य != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } जबतक (i_मूल्य != 0);

    वापस Put(s, bytes, len);
}

अटल
M3_परिणाम PutLEB_i64 (M3SnapshotSave* s, i64 i_मूल्य)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    जबतक (more) {
        u8 byte = i_मूल्य & 0x7F;
        i_मूल्य >>= 7;
        अगर ((i_मूल्य == 0 && !(byte & 0x40)) || (i_मूल्य == -1 && (byte & 0x40))) {
            more = false;
        } अन्यथा {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    वापस Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
प्रकार M3_परिणाम (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
अटल
M3_परिणाम SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_परिणाम         परिणाम   = m3Err_none;
    M3_बफर_Writer   bw       = { शून्य, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    व्योम*            userdata = s->userdata;

    s->writer   = बफर_Writer_Write;
    s->userdata = &bw;

    परिणाम = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    अगर (परिणाम) {
        जाओ _catch;
    }

    // a section with nothing in it is left out
    अगर (bw.आकार) {
        _throwif("snapshot section is too large", bw.आकार > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.आकार));
_       (Put(s, bw.बफर, bw.आकार));
    }

_catch:
    m3_Free(bw.बफर);

    वापस परिणाम;
}

अटल
u32 Function_सूचकांक (IM3Module i_module, IM3Function i_function)
{
    अगर (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->अंक_Functions) {
        वापस (u32)(i_function - i_module->functions);
    }

    वापस d_m3SnapshotNone;
}

अटल
u32 Func_Type_सूचकांक (IM3Module i_module, IM3FuncType i_type)
{
    क्रम (u32 i = 0; i < i_module->अंक_Func_Types; ++i) {
        अगर (i_module->funcTypes[i] == i_type) {
            वापस i;
        }
    }

    वापस d_m3SnapshotNone;
}

#  अगर d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
अटल
IM3Tag Tag_Of_सूचकांक (IM3Module i_module, u32 i_सूचकांक)
{
    IM3Tag tag = &i_module->tags[i_सूचकांक];

    वापस tag->resolved ? tag->resolved : tag;
}

अटल
u32 Tag_सूचकांक (IM3Module i_module, IM3Tag i_tag)
{
    क्रम (u32 i = 0; i < i_module->अंक_Tags; ++i) {
        अगर (Tag_Of_सूचकांक(i_module, i) == i_tag) {
            वापस i;
        }
    }

    वापस d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
अटल
M3_परिणाम NameReference (M3SnapshotSave* s, u8 i_type, व्योम* i_reference, u64* o_word)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    अगर (not i_reference) {
        वापस m3Err_none;
    }

    अगर (i_type == c_m3Type_funcref) {
        id = Function_सूचकांक(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } अन्यथा अगर (i_type == c_m3Type_externref) {
        स्थिर M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        जाओ _catch;
    }
#  अगर d_m3HasExceptionHandling
    अन्यथा अगर (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 गिनती = s->exceptions.गिनती;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.गिनती != गिनती);
    }
#  endif
    अन्यथा अगर (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 गिनती = s->continuations.गिनती;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.गिनती != गिनती);
    } अन्यथा {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    वापस परिणाम;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
अटल
M3_परिणाम Put_Ref_मूल्य (M3SnapshotSave* s, u8 i_type, व्योम* i_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    अगर (not s->writing) {
        वापस m3Err_none;
    }

    अगर (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } अन्यथा {
        u8 kind = 0;
        अगर (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } अन्यथा अगर (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  अगर d_m3HasExceptionHandling
        अन्यथा अगर (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        अन्यथा अगर (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    वापस परिणाम;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
अटल
M3_परिणाम PutLE32 (M3SnapshotSave* s, u32 i_मूल्य)
{
    u8 bytes[4];

    क्रम (u32 i = 0; i < माप(bytes); ++i) {
        bytes[i] = (u8)(i_मूल्य >> (8 * i));
    }

    वापस Put(s, bytes, माप(bytes));
}

अटल
M3_परिणाम PutLE64 (M3SnapshotSave* s, u64 i_मूल्य)
{
    u8 bytes[8];

    क्रम (u32 i = 0; i < माप(bytes); ++i) {
        bytes[i] = (u8)(i_मूल्य >> (8 * i));
    }

    वापस Put(s, bytes, माप(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
अटल
M3_परिणाम Put_मूल्य_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    अगर (IsRefType(i_type)) {
        वापस Put_Ref_मूल्य(s, BaseTypeOf(i_type), (व्योम*)(uintptr_t)i_bits);
    }

    चयन (BaseTypeOf(i_type)) {
    हाल c_m3Type_i32: वापस PutLEB_i64(s, (i32)(u32)i_bits);
    हाल c_m3Type_i64: वापस PutLEB_i64(s, (i64)i_bits);
    हाल c_m3Type_f32: वापस PutLE32(s, (u32)i_bits);
    हाल c_m3Type_f64: वापस PutLE64(s, i_bits);
    शेष: वापस "a snapshot cannot hold a value of this type as bits";
    }
}

अटल
M3_परिणाम Put_मूल्य_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    अगर (not valtype) {
        वापस "a snapshot has no tag for a value of this type";
    }

    वापस PutU8(s, valtype);
}

अटल
M3_परिणाम Put_मूल्य_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (Put_मूल्य_Tag(s, i_type));
_   (Put_मूल्य_Body(s, i_type, i_bits));

_catch:
    वापस परिणाम;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
अटल
M3_परिणाम Put_मूल्य (M3SnapshotSave* s, m3type_t i_type, स्थिर व्योम* i_where)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (Put_मूल्य_Tag(s, i_type));

    अगर (IsRefType(i_type)) {
        व्योम* reference;
        स_नकल(&reference, i_where, माप(reference));
_       (Put_मूल्य_Body(s, i_type, (u64)(uintptr_t)reference));
    } अन्यथा अगर (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        स_नकल(bytes, i_where, माप(bytes));
_       (Put(s, bytes, माप(bytes)));
    } अन्यथा अगर (Is64BitType(i_type)) {
        u64 bits;
        स_नकल(&bits, i_where, माप(bits));
_       (Put_मूल्य_Body(s, i_type, bits));
    } अन्यथा {
        u32 narrow;
        स_नकल(&narrow, i_where, माप(narrow));
_       (Put_मूल्य_Body(s, i_type, narrow));
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम PutMemoryChunks (M3SnapshotSave* s, स्थिर u8* i_bytes, माप_प्रकार i_आकार)
{
    M3_परिणाम परिणाम = m3Err_none;
    माप_प्रकार   cursor = 0;

    जबतक (cursor < i_आकार) {
        u8     byte = i_bytes[cursor];
        माप_प्रकार run  = 1;

        जबतक (cursor + run < i_आकार and i_bytes[cursor + run] == byte) {
            run++;
        }

        अगर ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            अगर (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            जारी;
        }

        माप_प्रकार rawStart = cursor;

        जबतक (cursor < i_आकार) {
            u8     b      = i_bytes[cursor];
            माप_प्रकार लंबाई = 1;

            जबतक (cursor + लंबाई < i_आकार and i_bytes[cursor + लंबाई] == b) {
                लंबाई++;
            }

            अगर ((b == 0x00 or b == 0xFF) and लंबाई >= d_m3SnapshotRunThreshold) {
                अवरोध;
            }

            cursor += लंबाई;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)rawStart));
_       (PutLEB_u32(s, (u32)(cursor - rawStart)));
_       (Put(s, i_bytes + rawStart, cursor - rawStart));
    }

_   (PutU8(s, d_m3ChunkEnd));

_catch:
    वापस परिणाम;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
अटल
u32 Memory_First_सूचकांक (IM3Module module, u32 index)
{
    u32 j = 0;
    जबतक (module->memories[j] != module->memories[index]) {
        ++j;
    }
    वापस j;
}

अटल
u32 Table_First_सूचकांक (IM3Module module, u32 index)
{
    u32 j = 0;
    जबतक (module->tables[j] != module->tables[index]) {
        ++j;
    }
    वापस j;
}

अटल
M3_परिणाम SaveMemories (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अंक_Memories));

    क्रम (u32 m = 0; m < module->अंक_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_First_सूचकांक(module, m);
        bool      has_तथ्य  = memory and memory->mallocated;
        u32       page_आकार = memory ? Memory_Page_आकार(memory) : d_m3_Default_Mem_Page_आकार;
        u64       bytes    = memory ? memory->अंक_Pages * (u64)page_आकार : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        अगर (first != m) {
            जारी;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", memory and memory->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", bytes > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 memory and memory->mallocated and memory->mallocated->लंबाई < bytes);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        जबतक ((1u << pageBits) < page_आकार) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->अंक_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        अगर (not s->writing) {
            जारी;
        }

        अगर (has_तथ्य) {
_           (PutMemoryChunks(s, m3_Mem_तथ्य(memory->mallocated), (माप_प्रकार)bytes));
        } अन्यथा {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम SaveGlobals (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अंक_Globals));

    क्रम (u32 g = 0; g < module->अंक_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_मूल्य(s, global->type, &cell->i64_मूल्य));
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम SaveTables (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अंक_Tables));

    क्रम (u32 t = 0; t < module->अंक_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_First_सूचकांक(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        अगर (first != t) {
            जारी;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->आकार));

        क्रम (u32 e = 0; e < table->आकार; ++e) {
_           (Put_Ref_मूल्य(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम SaveSegments (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अंक_तथ्य_Segments));

    क्रम (u32 i = 0; i < module->अंक_तथ्य_Segments; ++i) {
_       (PutU8(s, module->तथ्य_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->अंक_Element_Segments));

    क्रम (u32 i = 0; i < module->अंक_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    वापस परिणाम;
}

#  अगर d_m3HasExceptionHandling

अटल
M3_परिणाम SaveExceptions (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.गिनती));

    क्रम (u32 i = 0; i < s->exceptions.गिनती; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.वस्तुएँ[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->अंक_Args != Get_Func_Type_अंक_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->अंक_Args));

        क्रम (u32 a = 0; a < exn->अंक_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_मूल्य_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    वापस परिणाम;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
प्रकार काष्ठा M3Activation {
    IM3Function        function;
    स्थिर M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    स्थिर M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
अटल
स्थिर M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    स्थिर M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : शून्य;

    क्रम (u32 i = 0; not point and i < i_module->अंक_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        अगर (point) {
            *io_function = &i_module->functions[i];
        }
    }

    वापस point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
अटल
M3_परिणाम अग्र_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3_परिणाम परिणाम = m3Err_none;
    i32      i      = i_first;

    स_रखो(o_act, 0, माप(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    क्रम (; i >= 0; --i) {
        स्थिर M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        अगर (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  अगर d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            अवरोध;
        } अन्यथा अगर (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            अवरोध;
        }
    }

    अगर (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  अगर d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  अगर d_m3EntryKeepsFrame
    क्रम (i32 k = i_first; k > o_act->lastFrame; --k) {
        स्थिर M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    वापस परिणाम;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
अटल
M3_परिणाम SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_अंक_Slots, स्थिर M3Activation* i_act)
{
    M3_परिणाम             परिणाम        = m3Err_none;
    IM3Function          function      = i_act->function;
    स्थिर M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_सूचकांक = Function_सूचकांक(s->module, function);
    स्थिर M3SafePoint*   point         = i_act->point;
    u32                  अंक_मूल्यावली;

    _throwif("a suspended frame belongs to another module", function_सूचकांक == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_सूचकांक));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    अगर (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    अंक_मूल्यावली = map->अंक_Locals + point->अंक_मूल्यावली;

_   (PutLEB_u32(s, अंक_मूल्यावली));

    क्रम (u32 v = 0; v < अंक_मूल्यावली; ++v) {
        स्थिर M3_Slot_मूल्य* मूल्य = Frame_मूल्य(map, point, v);
        u8*                where;
        u64                word;
        व्योम*              reference;

_       (Locate_मूल्य(i_base, i_अंक_Slots, i_act->sp, मूल्य, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        अगर (BaseTypeOf(मूल्य->type) == c_m3Type_v128) {
_           (Put_मूल्य_Tag(s, मूल्य->type));
_           (Put(s, where, 16));
            जारी;
        }

_       (Read_मूल्य(where, &i_act->registers, मूल्य, &word, &reference));

_       (Put_मूल्य_Bits(s, मूल्य->type, IsRefType(मूल्य->type) ? (u64)(uintptr_t)reference : word));
    }

    अगर (point->kind == safepoint_resume) {
_       (Put_Ref_मूल्य(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    वापस परिणाम;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
अटल
M3_परिणाम SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_परिणाम      परिणाम   = m3Err_none;
    M3Activation* acts     = शून्य;
    u32           अंक_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->अंक_Frames - 1;
    m3slot_t*     base;
    u32           अंक_Slots;
    m3stack_t     sp;

_   (Continuation_ढेर(s->runtime, i_cont, &base, &अंक_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->अंक_Frames + 1);
    _throwifnull(acts);

    sp = base;

    जबतक (true) {
        M3Activation* act = &acts[अंक_Acts];

_       (अग्र_Activation(s->module, i_cont, function, sp, first, act));

        अगर (act->kind != safepoint_call) {
            अंक_Acts++;
            अवरोध;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        अगर (not (act->point->flags & d_m3_Safe_Point_पुच्छ_Call)) {
            अंक_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, अंक_Acts));

    क्रम (u32 a = 0; a < अंक_Acts; ++a) {
_       (SaveActivation(s, base, अंक_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    वापस परिणाम;
}

अटल
M3_परिणाम SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_परिणाम  परिणाम    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_मूल    = (i_cont == s->runtime->मूल_Continuation);
    u8        state     = snapshot_contFinished;
    u32       type_सूचकांक = is_मूल ? d_m3SnapshotNone : Func_Type_सूचकांक(module, i_cont->type);

    अगर (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } अन्यथा अगर (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } अन्यथा अगर (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", Function_सूचकांक(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_मूल and type_सूचकांक == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_मूल));
_   (PutLEB_u32(s, type_सूचकांक));
_   (PutLEB_u32(s, Function_सूचकांक(module, i_cont->entryFunction)));
#  अगर d_m3HasExceptionHandling
_   (Put_Ref_मूल्य(s, c_m3Type_exnref, i_cont->resumeThrow));
#  अन्यथा
_   (Put_Ref_मूल्य(s, c_m3Type_exnref, शून्य));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    अगर (s->postmortem) {
        जाओ _catch;
    }

    अगर (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : शून्य;
        u16         अंक_Rets = inner ? Get_Func_Type_अंक_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_गिनती > Get_Func_Type_अंक_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_गिनती));

        क्रम (u32 i = 0; i < i_cont->bound_Args_गिनती; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_मूल्य(s, type, i_cont->val_ढेर + (अंक_Rets + i) * c_io_Slot_गिनती));
        }
    } अन्यथा अगर (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_गिनती));
_       (SaveSuspendedBody(s, i_cont));
    } अन्यथा {
_       (PutLEB_u32(s, 0));
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम SaveContinuations (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (PutLEB_u32(s, s->continuations.गिनती));

    क्रम (u32 i = 0; i < s->continuations.गिनती; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.वस्तुएँ[i]));
    }

_catch:
    वापस परिणाम;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
अटल
M3_परिणाम SaveHostState (M3SnapshotSave* s)
{
    M3_परिणाम               परिणाम = m3Err_none;
    स्थिर M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_बफर_Writer         bw     = { शून्य, 0, 0 };

    अगर (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, बफर_Writer_Write, &bw));
    }

    अगर (bw.आकार > 0) {
_       (Put(s, bw.बफर, bw.आकार));
    }

_catch:
    m3_Free(bw.बफर);

    वापस परिणाम;
}

अटल
व्योम SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    क्रम (u32 i = 0; i < module->अंक_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        अगर (Memory_First_सूचकांक(module, i) == i and memory) {
            *memoryBytes += memory->अंक_Pages * (u64)Memory_Page_आकार(memory);
        }
    }
    क्रम (u32 i = 0; i < module->अंक_Tables; ++i) {
        IM3Table table = module->tables[i];
        अगर (Table_First_सूचकांक(module, i) == i and table) {
            *tableElements += table->आकार;
        }
    }
}

अटल
M3_परिणाम SaveMeta (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    क्रम (u32 i = 0; i < s->continuations.गिनती; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.वस्तुएँ[i];
        अगर (cont != s->runtime->मूल_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.गिनती));
_   (PutLEB_u32(s, s->exceptions.गिनती));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  अगर d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    क्रम (u32 i = 0; i < s->exceptions.गिनती; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.वस्तुएँ[i];
        u32          tag_सूचकांक  = Tag_सूचकांक(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tag_सूचकांक == d_m3SnapshotNone);

_       (PutLEB_u32(s, tag_सूचकांक));
    }
#  endif

_catch:
    वापस परिणाम;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
अटल
M3_परिणाम DiscoverReferences (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    जबतक (doneContinuations < s->continuations.गिनती or doneExceptions < s->exceptions.गिनती) {
#  अगर d_m3HasExceptionHandling
        जबतक (doneExceptions < s->exceptions.गिनती) {
            M3Exception* exception = (M3Exception*)s->exceptions.वस्तुएँ[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->अंक_Args != Get_Func_Type_अंक_Params(type));

            क्रम (u32 a = 0; a < exception->अंक_Args; ++a) {
_               (Put_मूल्य_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        जबतक (doneContinuations < s->continuations.गिनती and doneExceptions == s->exceptions.गिनती) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.वस्तुएँ[id]));
        }
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम SaveSections (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (not s->writing) {
        वापस DiscoverReferences(s);
    }

    अगर (not s->embedded) {
_       (Put(s, c_snapshotMagic, माप(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    अगर (s->module->अंक_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    अगर (s->module->अंक_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    अगर (s->module->अंक_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    अगर (s->module->अंक_तथ्य_Segments or s->module->अंक_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  अगर d_m3HasExceptionHandling
    अगर (s->exceptions.गिनती) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    अगर (s->continuations.गिनती) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    वापस परिणाम;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

प्रकार काष्ठा M3SnapshotSuspendSite {
    स्थिर M3SnapshotMap* map;
    स्थिर M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

प्रकार काष्ठा M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    व्योम*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ढेर;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    अंक_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  अगर d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 अंक_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

अटल
M3_परिणाम Get (M3SnapshotLoad* l, व्योम* o_तथ्य, माप_प्रकार i_आकार)
{
    अगर (not l->inSection) {
        वापस l->reader(o_तथ्य, i_आकार, l->userdata);
    }
    अगर (i_आकार > (माप_प्रकार)(l->end - l->pos)) {
        वापस m3Err_wasmUnderrun;
    }
    अगर (i_आकार) {
        स_नकल(o_तथ्य, l->pos, i_आकार);
        l->pos += i_आकार;
    }
    वापस m3Err_none;
}

अटल
M3_परिणाम GetU8 (M3SnapshotLoad* l, u8* o_मूल्य)
{
    वापस Get(l, o_मूल्य, माप(*o_मूल्य));
}

अटल
M3_परिणाम GetBool (M3SnapshotLoad* l, u8* o_मूल्य)
{
    M3_परिणाम परिणाम = GetU8(l, o_मूल्य);
    वापस परिणाम ? परिणाम : (*o_मूल्य <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

अटल
M3_परिणाम GetLEB_u32 (M3SnapshotLoad* l, u32* o_मूल्य)
{
    अगर (l->inSection) {
        वापस ReadLEB_u32(o_मूल्य, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    क्रम (u32 i = 0; i < माप(encoded); ++i) {
        M3_परिणाम परिणाम = GetU8(l, &encoded[i]);
        अगर (परिणाम) {
            वापस परिणाम;
        }
        अगर (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            वापस ReadLEB_u32(o_मूल्य, &pos, encoded + i + 1);
        }
    }
    वापस m3Err_lebOverflow;
}

अटल
M3_परिणाम GetLEB_u64 (M3SnapshotLoad* l, u64* o_मूल्य)
{
    वापस ReadLebUnsigned(o_मूल्य, 64, &l->pos, l->end);
}

अटल
M3_परिणाम GetLEB_i64 (M3SnapshotLoad* l, i64* o_मूल्य)
{
    वापस ReadLEB_i64(o_मूल्य, &l->pos, l->end);
}

अटल
M3_परिणाम SkipBytes (M3SnapshotLoad* l, माप_प्रकार i_आकार)
{
    अगर (i_आकार > (माप_प्रकार)(l->end - l->pos)) {
        वापस m3Err_wasmUnderrun;
    }
    l->pos += i_आकार;
    वापस m3Err_none;
}


अटल
M3_परिणाम Get_Ref_मूल्य (M3SnapshotLoad* l, m3type_t i_type, व्योम** o_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = शून्य;

_   (GetU8(l, &kind));
    अगर (kind == 0x00) {
        वापस (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    अगर (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->अंक_Functions);
#  अगर d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } अन्यथा अगर (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        स्थिर M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        अगर (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  अगर d_m3HasExceptionHandling
    अन्यथा अगर (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->अंक_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    अन्यथा अगर (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->अंक_Continuations);
        *o_reference = l->continuations[id];
#  अगर d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        अगर (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } अन्यथा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम GetLE32 (M3SnapshotLoad* l, u32* o_मूल्य)
{
    u8       bytes[4];
    M3_परिणाम परिणाम = Get(l, bytes, माप(bytes));

    *o_मूल्य = 0;

    क्रम (u32 i = 0; i < माप(bytes); ++i) {
        *o_मूल्य |= (u32)bytes[i] << (8 * i);
    }

    वापस परिणाम;
}

अटल
M3_परिणाम GetLE64 (M3SnapshotLoad* l, u64* o_मूल्य)
{
    u8       bytes[8];
    M3_परिणाम परिणाम = Get(l, bytes, माप(bytes));

    *o_मूल्य = 0;

    क्रम (u32 i = 0; i < माप(bytes); ++i) {
        *o_मूल्य |= (u64)bytes[i] << (8 * i);
    }

    वापस परिणाम;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
अटल
M3_परिणाम Get_मूल्य_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_परिणाम परिणाम  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    अगर (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    वापस परिणाम;
}

// The mirror of PutValueBody: narrow types come back in the low bits
अटल
M3_परिणाम Get_मूल्य_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3_परिणाम परिणाम = m3Err_none;

    *o_bits = 0;

    अगर (IsRefType(i_type)) {
        व्योम* reference = शून्य;
_       (Get_Ref_मूल्य(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } अन्यथा अगर (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 मूल्य = 0;
_       (ReadLEB_i32(&मूल्य, &l->pos, l->end));
        *o_bits = (u32)मूल्य;
    } अन्यथा अगर (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 मूल्य = 0;
_       (GetLEB_i64(l, &मूल्य));
        *o_bits = (u64)मूल्य;
    } अन्यथा अगर (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } अन्यथा अगर (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } अन्यथा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम Get_मूल्य_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3_परिणाम परिणाम = m3Err_none;
    m3type_t type;

_   (Get_मूल्य_Tag(l, i_expectedType, &type));
_   (Get_मूल्य_Body(l, type, o_bits));

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम Get_मूल्य (M3SnapshotLoad* l, m3type_t i_expectedType, व्योम* o_where)
{
    M3_परिणाम परिणाम = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    अगर (l->checking) {
        o_where = scratch;
    }

_   (Get_मूल्य_Tag(l, i_expectedType, &type));

    अगर (IsRefType(type)) {
        व्योम* reference = शून्य;
        u64   bits      = 0;
_       (Get_मूल्य_Body(l, type, &bits));
        reference = (व्योम*)(uintptr_t)bits;
        स_नकल(o_where, &reference, माप(reference));
    } अन्यथा अगर (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } अन्यथा {
        u64 bits = 0;
_       (Get_मूल्य_Body(l, type, &bits));

        अगर (Is64BitType(type)) {
            स_नकल(o_where, &bits, माप(bits));
        } अन्यथा {
            u32 narrow = (u32)bits;
            स_नकल(o_where, &narrow, माप(narrow));
        }
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->अंक_Functions);
    *o_function = &l->module->functions[index];

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, माप_प्रकार i_आकार)
{
    M3_परिणाम परिणाम = m3Err_none;

    क्रम (;;) {
        u8  chunkType = 0;
        u32 offset = 0, लंबाई = 0;

_       (GetU8(l, &chunkType));

        अगर (chunkType == d_m3ChunkEnd) {
            अवरोध;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &लंबाई));

        _throwif(m3Err_wasmMalformed, (माप_प्रकार)offset > i_आकार or लंबाई > i_आकार - (माप_प्रकार)offset);

        अगर (chunkType == d_m3ChunkRaw) {
            अगर (l->checking or लंबाई == 0) {
_               (SkipBytes(l, लंबाई));
            } अन्यथा {
_               (Get(l, o_bytes + offset, लंबाई));
            }
        } अन्यथा अगर (chunkType == d_m3ChunkFillFF) {
            अगर (not l->checking and लंबाई) {
                स_रखो(o_bytes + offset, 0xFF, लंबाई);
            }
        } अन्यथा {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadMemories (M3SnapshotLoad* l)
{
    M3_परिणाम  परिणाम      = m3Err_none;
    IM3Module module      = l->module;
    u32       अंक_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &अंक_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, अंक_Memories == 0 or अंक_Memories != module->अंक_Memories);

    क्रम (u32 m = 0; m < अंक_Memories; ++m) {
        u32       mem_सूचकांक = 0, first_सूचकांक = 0, pageBits = 0, अंक_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &mem_सूचकांक));
_       (GetLEB_u32(l, &first_सूचकांक));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_सूचकांक != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 first_सूचकांक != Memory_First_सूचकांक(module, m));
        अगर (first_सूचकांक != m) {
            जारी;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &अंक_Pages));

        memory = module->memories[mem_सूचकांक];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        माप_प्रकार bytes = 0;
        u8*    तथ्य  = शून्य;

        अगर (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_Page_आकार(memory));
            _throwif(m3Err_wasmMalformed, अंक_Pages < memory->अंक_Pages or अंक_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, अंक_Pages > SIZE_MAX / Memory_Page_आकार(memory));
            bytes = (माप_प्रकार)अंक_Pages * Memory_Page_आकार(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (माप_प्रकार)l->runtime->memoryLimit);

            अगर (not l->checking) {
                अगर (memory->अंक_Pages < अंक_Pages) {
_                   (ResizeMemory(l->runtime, memory, अंक_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->लंबाई < bytes);
            }

            l->loadedMemoryBytes += bytes;

            अगर (bytes and memory->mallocated) {
                तथ्य = m3_Mem_तथ्य(memory->mallocated);
            }
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, अंक_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        अगर (तथ्य and not l->checking) {
            स_रखो(तथ्य, 0, bytes);
        }

_       (LoadMemoryChunks(l, तथ्य, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadGlobals (M3SnapshotLoad* l)
{
    M3_परिणाम  परिणाम     = m3Err_none;
    IM3Module module     = l->module;
    u32       अंक_Globals = 0;

_   (GetLEB_u32(l, &अंक_Globals));
    _throwif(m3Err_wasmMalformed, अंक_Globals == 0 or अंक_Globals != module->अंक_Globals);

    क्रम (u32 g = 0; g < अंक_Globals; ++g) {
        u32       global_सूचकांक = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_सूचकांक));
        _throwif(m3Err_wasmMalformed, global_सूचकांक != g);

        global = &module->globals[global_सूचकांक];
        cell   = global->resolved ? global->resolved : global;

_       (Get_मूल्य(l, global->type, &cell->i64_मूल्य));
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadTables (M3SnapshotLoad* l)
{
    M3_परिणाम  परिणाम       = m3Err_none;
    IM3Module module       = l->module;
    u32       अंक_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &अंक_Tables));
    _throwif(m3Err_wasmMalformed, अंक_Tables == 0 or अंक_Tables != module->अंक_Tables);

    क्रम (u32 t = 0; t < अंक_Tables; ++t) {
        u32      table_सूचकांक = 0, first_सूचकांक = 0, आकार = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &table_सूचकांक));
_       (GetLEB_u32(l, &first_सूचकांक));

        _throwif(m3Err_wasmMalformed, table_सूचकांक != t);
        _throwif("the snapshot shares tables between imports differently",
                 first_सूचकांक != Table_First_सूचकांक(module, t));
        अगर (first_सूचकांक != t) {
            जारी;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &आकार));

        table = module->tables[table_सूचकांक];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, आकार > (table->hasMax ? table->max_आकार : d_m3_Max_Sane_Table_आकार));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, आकार < table->आकार);

        l->loadedTableElements += आकार;

        अगर (not l->checking and आकार != table->आकार) {
            व्योम** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 आकार - table->आकार > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(व्योम*, table->elements, आकार, table->आकार);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += आकार - table->आकार;
            table->elements = elements;
            table->आकार     = आकार;
        }

        क्रम (u32 e = 0; e < आकार; ++e) {
            व्योम* reference = शून्य;
_           (Get_Ref_मूल्य(l, table->type, &reference));
            अगर (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadSegments (M3SnapshotLoad* l)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = l->module;
    u32       गिनती  = 0;

_   (GetLEB_u32(l, &गिनती));
    _throwif(m3Err_wasmMalformed, गिनती != module->अंक_तथ्य_Segments);
    _throwif(m3Err_wasmMalformed, module->अंक_तथ्य_Segments == 0 and module->अंक_Element_Segments == 0);

    क्रम (u32 i = 0; i < गिनती; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        अगर (not l->checking) {
            module->तथ्य_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &गिनती));
    _throwif(m3Err_wasmMalformed, गिनती != module->अंक_Element_Segments);

    क्रम (u32 i = 0; i < गिनती; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        अगर (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    वापस परिणाम;
}

// A frame for the continuation being put together, outermost first
अटल
M3_परिणाम AddFrame (M3SnapshotLoad* l, u32* io_अंक_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    अगर (*io_अंक_Frames >= d_m3ContinuationMaxFrames) {
        वापस "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_अंक_Frames)++];

    स_रखो(frame, 0, माप(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    वापस m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
अटल
M3_परिणाम PrepareFunction (IM3Function i_function)
{
    M3_परिणाम परिणाम = m3Err_none;

    अगर (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    वापस परिणाम;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
अटल
M3_परिणाम AddBlockFrames (M3SnapshotLoad* l, स्थिर M3SnapshotMap* i_map, स्थिर M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_अंक_Frames)
{
    M3_परिणाम परिणाम = m3Err_none;

    क्रम (u32 b = 0; b < i_map->अंक_Blocks; ++b) {
        स्थिर M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        अगर (not EnclosesOffset(block, i_point->wasmOffset)) {
            जारी;
        }

        अगर (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_अंक_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  अगर d_m3HasExceptionHandling
        अन्यथा {
_           (AddFrame(l, io_अंक_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.अंक_Clauses   = block->अंक_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    वापस परिणाम;
}

// Puts a function's values where this build keeps them at the safepoint
अटल
M3_परिणाम Load_मूल्यावली (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_अंक_Slots, m3stack_t i_sp,
                     स्थिर M3SnapshotMap* i_map, स्थिर M3SafePoint* i_point, स्थिर M3FrameRegisters* i_registers)
{
    M3_परिणाम परिणाम    = m3Err_none;
    u32      अंक_मूल्यावली = 0;

_   (GetLEB_u32(l, &अंक_मूल्यावली));
    _throwif("the snapshot's frame does not match what this build compiled", अंक_मूल्यावली != i_map->अंक_Locals + i_point->अंक_मूल्यावली);

    क्रम (u32 v = 0; v < अंक_मूल्यावली; ++v) {
        स्थिर M3_Slot_मूल्य* मूल्य     = Frame_मूल्य(i_map, i_point, v);
        u64                word      = 0;
        व्योम*              reference = शून्य;
        u8*                where;

_       (Locate_मूल्य(i_base, i_अंक_Slots, i_sp, मूल्य, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        अगर (BaseTypeOf(मूल्य->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_मूल्य_Tag(l, मूल्य->type, &type));
            अगर (l->checking) {
_               (SkipBytes(l, 16));
            } अन्यथा {
_               (Get(l, where, 16));
            }
            जारी;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= अंक_मूल्यावली - i_point->अंक_Results;
_       (Get_मूल्य_Bits(l, मूल्य->type, &word));
        l->allowUnfilledRef = false;

        अगर (IsRefType(मूल्य->type)) {
            reference = (व्योम*)(uintptr_t)word;
        }

        अगर (not l->checking) {
_           (Write_मूल्य(where, i_registers, मूल्य, word, reference));
        }
    }

_catch:
    वापस परिणाम;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
अटल
M3_परिणाम LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3_परिणाम    परिणाम    = m3Err_none;
    u32         अंक_Frames = 0;
    M3Frame*    callFrame = शून्य;
    IM3Function function  = शून्य;
    m3slot_t*   base;
    u32         अंक_Slots;
    u32         अंक_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_ढेर(l->runtime, io_cont, &base, &अंक_Slots));

_   (GetLEB_u32(l, &अंक_Activations));
    _throwif(m3Err_wasmMalformed, अंक_Activations == 0 or अंक_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = शून्य;
    io_cont->r0                = 0;
    io_cont->अंक_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  अगर d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    क्रम (u32 a = 0; a < अंक_Activations; ++a) {
        bool                 isInnermost = (a + 1 == अंक_Activations);
        स्थिर M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        स्थिर M3SnapshotMap* map;

        स_रखो(&registers, 0, माप(registers));

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
                 (u32)(sp - base) + function->max_ढेर_Slots > अंक_Slots);

        // the call the frame outside this one is waiting on
        अगर (callFrame) {
            callFrame->call.function = function;
        }

#  अगर d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &अंक_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        अगर (kind == safepoint_op) {
            स्थिर M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &अंक_Frames));

        अगर (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &अंक_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  अगर d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } अन्यथा अगर (kind == safepoint_resume) {
            M3Frame* frame;
            u32      अंक_Results = point->अंक_Results;
            pc_t     resultsPC  = point->pc - 2 * अंक_Results;

_           (AddFrame(l, &अंक_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.अंक_Results  = अंक_Results;
            frame->resume.अंक_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  अगर d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_मूल्यावली(l, base, अंक_Slots, sp, map, point, &registers));

        अगर (not l->checking) {
            RestoreConstants(function, sp);
        }

        अगर (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } अन्यथा अगर (kind == safepoint_resume) {
            व्योम* cont = शून्य;
_           (Get_Ref_मूल्य(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == शून्य);

            callFrame->resume.cont = (IM3Continuation)cont;
            अगर (not l->checking) {
                l->suspendSites[i_id].point = point;
                क्रम (u32 i = 1; i < l->अंक_Continuations; ++i) {
                    अगर (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        अवरोध;
                    }
                }
            }
        } अन्यथा {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            अगर (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            अगर (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->अंक_Results > point->अंक_मूल्यावली or
                                                point->अंक_Results > d_m3MaxContinuationPayload);

                io_cont->अंक_Suspend_Results = point->अंक_Results;
                अगर (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                क्रम (u32 i = 0; i < point->अंक_Results; ++i) {
                    स्थिर M3_Slot_मूल्य* मूल्य = &map->मूल्यावली[point->first_मूल्य + point->अंक_मूल्यावली - point->अंक_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(मूल्य->slot));

                    io_cont->suspend_परिणाम_Offsets[i] = मूल्य->slot;
                    io_cont->suspend_परिणाम_Is64[i]    = Is64BitType(मूल्य->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    अगर (l->checking) {
        वापस m3Err_none;
    }

    // held innermost first
    अगर (अंक_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, अंक_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = अंक_Frames;
    }

    क्रम (u32 k = 0; k < अंक_Frames; ++k) {
        io_cont->frames[अंक_Frames - 1 - k] = l->frames[k];
    }

    io_cont->अंक_Frames = अंक_Frames;

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3_परिणाम        परिणाम = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    अगर (l->checking) {
        scratch          = *cont;
        scratch.val_ढेर = l->check_ढेर;
        अगर (i_id == 0) {
            scratch.val_ढेर = (m3slot_t*)l->runtime->origin_ढेर;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_सूचकांक = 0, entry_सूचकांक = 0, bound_Args_गिनती = 0;
    u8  state = 0, is_मूल = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_मूल));
_   (GetLEB_u32(l, &type_सूचकांक));
_   (GetLEB_u32(l, &entry_सूचकांक));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_मूल != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_मूल and state != snapshot_contSuspended);
    अगर (not is_मूल and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_सूचकांक != d_m3SnapshotNone and type_सूचकांक >= module->अंक_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entry_सूचकांक >= module->अंक_Functions);
    _throwif(m3Err_wasmMalformed, is_मूल ? type_सूचकांक != d_m3SnapshotNone : (type_सूचकांक == d_m3SnapshotNone or not module->funcTypes[type_सूचकांक]->isContinuation));

    अगर (not l->checking and not is_मूल and state != snapshot_contFinished) {
_       (Continuation_Acquire_ढेर(l->runtime, cont));
    }
    cont->type          = (type_सूचकांक != d_m3SnapshotNone) ? module->funcTypes[type_सूचकांक] : शून्य;
    cont->entryFunction = &module->functions[entry_सूचकांक];
    cont->अंक_Handlers   = 0;
    cont->handlersPC    = शून्य;
    cont->parent        = शून्य;
    cont->अंक_Frames     = 0;

#  अगर d_m3HasExceptionHandling
    {
        व्योम* ref = शून्य;
_       (Get_Ref_मूल्य(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_मूल or state == snapshot_contFinished) and cont->resumeThrow != शून्य);
#  अन्यथा
    {
        व्योम* ref = शून्य;
_       (Get_Ref_मूल्य(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != शून्य);
    }
#  endif

    अगर (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         अंक_Rets;

_       (GetLEB_u32(l, &bound_Args_गिनती));
        cont->bound_Args_गिनती = bound_Args_गिनती;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_गिनती > Get_Func_Type_अंक_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_अंक_Params(remaining) != Get_Func_Type_अंक_Params(inner) - bound_Args_गिनती or
                                            Get_Func_Type_अंक_Results(remaining) != Get_Func_Type_अंक_Results(inner));
            क्रम (u16 i = 0; i < Get_Func_Type_अंक_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_परिणाम_Type(inner, i) != Get_Func_Type_परिणाम_Type(remaining, i));
            }
            क्रम (u16 i = 0; i < Get_Func_Type_अंक_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_गिनती + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        अंक_Rets = Get_Func_Type_अंक_Results(inner);

        _throwif(m3Err_wasmMalformed, (अंक_Rets + bound_Args_गिनती) * c_io_Slot_गिनती > cont->अंक_ढेर_Slots);

        क्रम (u32 i = 0; i < bound_Args_गिनती; ++i) {
_           (Get_मूल्य(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ढेर + (अंक_Rets + i) * c_io_Slot_गिनती));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ढेर;
        cont->pc    = cont->entryFunction->compiled;
    } अन्यथा अगर (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_गिनती));
        _throwif(m3Err_wasmMalformed, is_मूल and bound_Args_गिनती != 0);
        cont->bound_Args_गिनती = bound_Args_गिनती;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } अन्यथा अगर (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_गिनती));
        _throwif(m3Err_wasmMalformed, bound_Args_गिनती != 0);
        cont->bound_Args_गिनती = 0;
        cont->state          = cont_consumed;
    } अन्यथा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    अगर (परिणाम) {
        cont->अंक_Frames = 0;
    }

    वापस परिणाम;
}

अटल
M3_परिणाम LoadContinuations (M3SnapshotLoad* l)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      गिनती  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &गिनती));
    _throwif(m3Err_wasmMalformed, गिनती == 0 or गिनती != l->अंक_Continuations);

    क्रम (u32 i = 0; i < गिनती; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    वापस परिणाम;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
अटल
M3_परिणाम CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3_परिणाम परिणाम = m3Err_none;

    क्रम (u32 id = 0; id < l->अंक_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  अगर d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        अगर (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : शून्य;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_अंक_Results(actual) != Get_Func_Type_अंक_Results(expected));
            क्रम (u16 i = 0; i < Get_Func_Type_अंक_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_परिणाम_Type(actual, i) != Get_Func_Type_परिणाम_Type(expected, i));
            }
        }
        जबतक (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->अंक_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        अगर (cont->state != cont_suspended or id == 0) {
            जारी;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        अगर (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_गिनती != 0);
            जारी;
        }

        स्थिर M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_गिनती > inner->अंक_Suspend_Results or
                                                                          Get_Func_Type_अंक_Params(remaining) != inner->अंक_Suspend_Results - cont->bound_Args_गिनती);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_अंक_Results(remaining) != Get_Func_Type_अंक_Results(cont->entryFunction->funcType));
        क्रम (u16 i = 0; i < Get_Func_Type_अंक_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_परिणाम_Type(remaining, i) !=
                                            Get_Func_Type_परिणाम_Type(cont->entryFunction->funcType, i));
        }

        क्रम (u32 i = 0; i < inner->अंक_Suspend_Results; ++i) {
            स्थिर M3_Slot_मूल्य* मूल्य = &site->map->मूल्यावली[site->point->first_मूल्य +
                                                          site->point->अंक_मूल्यावली - site->point->अंक_Results + i];
            अगर (i < cont->bound_Args_गिनती) {
                अगर (IsRefType(मूल्य->type) and not IsNullableRef(मूल्य->type)) {
                    व्योम* ref = शून्य;
                    स_नकल(&ref, inner->sp + मूल्य->slot, माप(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } अन्यथा {
                _throwif("a captured continuation has the wrong parameter type", मूल्य->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_गिनती)));
            }
        }
    }

_catch:
    वापस परिणाम;
}

#  अगर d_m3HasExceptionHandling

अटल
M3_परिणाम LoadExceptions (M3SnapshotLoad* l)
{
    M3_परिणाम परिणाम        = m3Err_none;
    u32      अंक_Exceptions = 0;

_   (GetLEB_u32(l, &अंक_Exceptions));
    _throwif(m3Err_wasmMalformed, अंक_Exceptions == 0 or अंक_Exceptions != l->अंक_Exceptions);

    क्रम (u32 i = 0; i < अंक_Exceptions; ++i) {
        u32          exnId = 0, अंक_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &अंक_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, अंक_Args != exn->अंक_Args);

        क्रम (u32 a = 0; a < अंक_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_मूल्य_Bits(l, argType, &bits));
            अगर (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    वापस परिणाम;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
प्रकार काष्ठा M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3_परिणाम        error;
} M3BoundedReader;

अटल
M3_परिणाम BoundedReader_Read (व्योम* o_बफर, माप_प्रकार i_आकार, व्योम* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    अगर (i_आकार > br->remaining) {
        br->error = "the host read past the end of its state";
        वापस br->error;
    }

    br->remaining -= i_आकार;

    वापस Get(br->load, o_बफर, i_आकार);
}

अटल
M3_परिणाम LoadHostState (M3SnapshotLoad* l)
{
    M3_परिणाम               परिणाम = m3Err_none;
    स्थिर M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    माप_प्रकार                 आकार = (माप_प्रकार)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    अगर (l->checking) {
        वापस SkipBytes(l, आकार);
    }
    reader.load      = l;
    reader.remaining = आकार;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, आकार));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadHeader (M3SnapshotLoad* l)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       magic[माप(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, माप(magic)));
    _throwif(m3Err_wasmMalformed, स_भेद(magic, c_snapshotMagic, माप(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadMeta (M3SnapshotLoad* l, u32* o_flags)
{
    M3_परिणाम परिणाम      = m3Err_none;
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

    (व्योम)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->अंक_Continuations));
_   (GetLEB_u32(l, &l->अंक_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->अंक_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->अंक_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    अगर (*o_flags & d_m3SnapshotFlagPostmortem) {
        क्रम (u32 i = 0; i < l->अंक_Exceptions; ++i) {
            u32 tag_सूचकांक = 0;
_           (GetLEB_u32(l, &tag_सूचकांक));
        }
        वापस m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->अंक_Continuations == 0 or activeStacks >= l->अंक_Continuations);
#  अगर !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->अंक_Exceptions != 0);
#  endif

    अगर (l->checking) {
        क्रम (u32 i = 0; i < l->अंक_Exceptions; ++i) {
            u32 tag_सूचकांक = 0;
_           (GetLEB_u32(l, &tag_सूचकांक));
#  अगर d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tag_सूचकांक >= l->module->अंक_Tags);
#  endif
        }
        वापस m3Err_none;
    }

    l->check_ढेर = m3_AllocArray(m3slot_t, d_m3_Continuation_ढेर_Slots + 4);
    _throwifnull(l->check_ढेर);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->अंक_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->अंक_Continuations);
    _throwifnull(l->suspendSites);

    अगर (not l->runtime->मूल_Continuation) {
        IM3Continuation मूल = Continuation_New(l->runtime, शून्य, शून्य);
        _throwifnull(मूल);

        // the root borrows the runtime's stack, so it holds none of its own
        मूल->अंक_ढेर_Slots          = l->runtime->अंक_ढेर_Slots;
        l->runtime->मूल_Continuation = मूल;
    }

    l->continuations[0] = l->runtime->मूल_Continuation;

    क्रम (u32 i = 1; i < l->अंक_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, शून्य, शून्य);
        _throwifnull(l->continuations[i]);
    }

#  अगर d_m3HasExceptionHandling
    अगर (l->अंक_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->अंक_Exceptions);
        _throwifnull(l->exceptions);
    }

    क्रम (u32 i = 0; i < l->अंक_Exceptions; ++i) {
        u32    tag_सूचकांक = 0;
        IM3Tag tag;
        u32    अंक_Args;

_       (GetLEB_u32(l, &tag_सूचकांक));
        _throwif(m3Err_wasmMalformed, tag_सूचकांक >= l->module->अंक_Tags);

        tag     = Tag_Of_सूचकांक(l->module, tag_सूचकांक);
        अंक_Args = Get_Func_Type_अंक_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, अंक_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  अन्यथा
    क्रम (u32 i = 0; i < l->अंक_Exceptions; ++i) {
        u32 tag_सूचकांक = 0;
_       (GetLEB_u32(l, &tag_सूचकांक));
    }
#  endif

_catch:
    वापस परिणाम;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
अटल
M3_परिणाम LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      flags  = 0;
    चयन (i_sectionId) {
    हाल d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        अवरोध;

    हाल d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        अवरोध;

    हाल d_m3SnapshotSection_Table:
_       (LoadTables(l));
        अवरोध;

    हाल d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        अवरोध;

    हाल d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        अवरोध;

#  अगर d_m3HasExceptionHandling
    हाल d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        अवरोध;
#  endif

    हाल d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        अवरोध;

    हाल d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        अवरोध;

    शेष:
_       (SkipBytes(l, (माप_प्रकार)(l->end - l->pos)));
        अवरोध;
    }

_catch:
    वापस परिणाम;
}

अटल
M3_परिणाम LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3_परिणाम परिणाम  = m3Err_none;
    u8*      payload = शून्य;

    अगर (not i_embedded) {
_       (LoadHeader(l));
    }

    जबतक (true) {
        u8  sectionId   = 0;
        u32 section_आकार = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        अगर (GetU8(l, &sectionId)) {
            अवरोध;
        }

_       (GetLEB_u32(l, &section_आकार));

        _throwif("the snapshot holds a section twice", l->seenSections[sectionId]);
        l->seenSections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_आकार == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_आकार);
        _throwifnull(payload);
_       (Get(l, payload, section_आकार));
        l->pos       = payload;
        l->end       = payload + section_आकार;
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
             l->अंक_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->अंक_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->अंक_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->अंक_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->अंक_तथ्य_Segments or l->module->अंक_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#अगर d_m3HasSnapshots

अटल
M3_परिणाम SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, व्योम* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_परिणाम       परिणाम = m3Err_none;
    M3SnapshotSave save;

    स_रखो(&save, 0, माप(save));

    IM3Continuation मूल = io_runtime->मूल_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not मूल or मूल->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : मूल->entryFunction;

    अगर (o_module) {
        *o_module = शून्य;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    अगर (o_module) {
        *o_module = save.module;
    }

    क्रम (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->अग्र) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  अगर d_m3HasExceptionHandling
    क्रम (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->अग्र) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    अगर (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, मूल, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    वापस परिणाम;
}

#पूर्ण_अगर

M3_परिणाम m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, व्योम* i_userdata)
{
    अगर (not io_runtime or not i_writer) {
        वापस m3Err_mallocFailed;
    }

#अगर d_m3HasSnapshots
    वापस SaveSnapshot(io_runtime, i_writer, i_userdata, शून्य, false);
#अन्यथा
    (व्योम)i_userdata;
    वापस "snapshots are not available in this build of Wasm3";
#पूर्ण_अगर
}


अटल
M3_परिणाम LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, व्योम* i_userdata, bool i_embedded)
{
    M3_परिणाम परिणाम = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#अगर d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        स_रखो(&load, 0, माप(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        परिणाम = LoadSections(&load, i_embedded);

        IM3Continuation मूल = io_runtime->मूल_Continuation;

        अगर (परिणाम) {
            अगर (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                अगर (मूल) {
                    मूल->state     = cont_returned;
                    मूल->अंक_Frames = 0;
                }
            }
        } अन्यथा {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_ढेर(io_runtime);

        m3_Free(load.check_ढेर);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  अगर d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#अन्यथा
    (व्योम)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#पूर्ण_अगर

_catch:
    वापस परिणाम;
}


M3_परिणाम m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, व्योम* i_userdata)
{
    वापस LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


व्योम m3_SetSnapshotHooks (IM3Runtime io_runtime, स्थिर M3SnapshotHooks* i_hooks)
{
#अगर d_m3HasSnapshots
    अगर (io_runtime) {
        अगर (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } अन्यथा {
            स_रखो(&io_runtime->snapshotHooks, 0, माप(io_runtime->snapshotHooks));
        }
    }
#अन्यथा
    (व्योम)io_runtime;
    (व्योम)i_hooks;
#पूर्ण_अगर
}


M3_परिणाम m3_Save_Snapshot_To_बफर (IM3Runtime io_runtime, व्योम** o_bytes, माप_प्रकार* o_आकार)
{
    अगर (!io_runtime || !o_bytes || !o_आकार) {
        वापस m3Err_mallocFailed;
    }
#अगर d_m3HasSnapshots
    M3_बफर_Writer bw     = { शून्य, 0, 0 };
    M3_परिणाम       परिणाम = m3_SaveSnapshot(io_runtime, बफर_Writer_Write, &bw);
    अगर (परिणाम) {
        m3_Free(bw.बफर);
        *o_bytes = शून्य;
        *o_आकार  = 0;
        वापस परिणाम;
    }
    *o_bytes = bw.बफर;
    *o_आकार  = bw.आकार;
    वापस m3Err_none;
#अन्यथा
    *o_bytes = शून्य;
    *o_आकार  = 0;
    वापस "snapshots are not available in this build of Wasm3";
#पूर्ण_अगर
}

#अगर d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
अटल
u8* EmitLEB_u32 (u8* o_where, u32 i_मूल्य)
{
    करो {
        u8 byte = i_मूल्य & 0x7F;
        i_मूल्य >>= 7;
        अगर (i_मूल्य != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } जबतक (i_मूल्य != 0);

    वापस o_where;
}

अटल
u32 आकार_Of_LEB_u32 (u32 i_मूल्य)
{
    u32 आकार = 0;

    करो {
        i_मूल्य >>= 7;
        ++आकार;
    } जबतक (i_मूल्य != 0);

    वापस आकार;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
अटल
M3_परिणाम FindEmbeddedSnapshot (स्थिर u8* i_start, स्थिर u8* i_end, cstr_t i_secName,
                               स्थिर u8** o_secStart, स्थिर u8** o_secEnd,
                               स्थिर u8** o_तथ्य, u32* o_आकार)
{
    M3_परिणाम परिणाम = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = शून्य;

    अगर (o_secStart) {
        *o_secStart = शून्य;
    }
    अगर (o_secEnd) {
        *o_secEnd = शून्य;
    }
    अगर (o_तथ्य) {
        *o_तथ्य = शून्य;
    }
    अगर (o_आकार) {
        *o_आकार = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or स_भेद(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    जबतक (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     आकार      = 0;
_       (ReadLEB_u32(&आकार, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, आकार > (माप_प्रकार)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += आकार;
            अगर (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                अगर (म_भेद(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    अगर (o_secStart) {
                        *o_secStart = secStart;
                    }
                    अगर (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    अगर (o_तथ्य) {
                        *o_तथ्य = payload;
                    }
                    अगर (o_आकार) {
                        *o_आकार = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    वापस परिणाम;
}

// Builds the custom section name a snapshot of i_name lives under
अटल
M3_परिणाम SnapshotSectionName (cstr_t i_name, अक्षर* o_secName, माप_प्रकार i_आकार)
{
    अगर (i_name and *i_name) {
        अगर (म_माप(i_name) > d_m3_Max_Snapshot_Name_लंबाई) {
            वापस "the snapshot name is too long";
        }
        snprintf(o_secName, i_आकार, "snapshot.%s", i_name);
    } अन्यथा {
        snprintf(o_secName, i_आकार, "snapshot");
    }

    वापस m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
अटल
M3_परिणाम ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_परिणाम परिणाम = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = शून्य;

    क्रम (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->अग्र) {
        अगर (म_भेद(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    वापस परिणाम;
}
#पूर्ण_अगर

M3_परिणाम m3_Load_Snapshot_From_बफर (IM3Runtime io_runtime, IM3Module i_module, स्थिर व्योम* i_bytes, माप_प्रकार i_आकार)
{
    अगर (!io_runtime || !i_module || !i_bytes) {
        वापस m3Err_mallocFailed;
    }
#अगर d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    अगर (i_आकार >= 8 && स_भेद(i_bytes, "\0asm", 4) == 0) {
        स्थिर u8* तथ्य   = शून्य;
        u32       आकार   = 0;
        M3_परिणाम  परिणाम = FindEmbeddedSnapshot((स्थिर u8*)i_bytes, (स्थिर u8*)i_bytes + i_आकार,
                                                "snapshot", शून्य, शून्य, &तथ्य, &आकार);
        अगर (परिणाम) {
            वापस परिणाम;
        }
        अगर (not तथ्य) {
            वापस "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_बफर_Reader br = { तथ्य, आकार, 0 };
            वापस LoadSnapshot(io_runtime, i_module, बफर_Reader_Read, &br, true);
        }
    }
    {
        M3_बफर_Reader br = { (स्थिर u8*)i_bytes, i_आकार, 0 };
        वापस LoadSnapshot(io_runtime, i_module, बफर_Reader_Read, &br,
                            i_आकार < 4 or स_भेद(i_bytes, c_snapshotMagic, 4) != 0);
    }
#अन्यथा
    (व्योम)i_आकार;
    वापस "snapshots are not available in this build of Wasm3";
#पूर्ण_अगर
}

bool m3_HasSnapshot (IM3Module i_module, स्थिर अक्षर* i_name)
{
    अगर (not i_module) {
        वापस false;
    }
#अगर d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = शून्य;
    (व्योम)ModuleSnapshot(i_module, i_name, &snap);

    वापस snap != शून्य;
#अन्यथा
    (व्योम)i_name;
    वापस false;
#पूर्ण_अगर
}

M3_परिणाम m3_GetEmbeddedSnapshot (IM3Module i_module, स्थिर अक्षर* i_name,
                                 स्थिर व्योम** o_bytes, माप_प्रकार* o_आकार)
{
    अगर (not i_module or not o_bytes or not o_आकार) {
        वापस m3Err_mallocFailed;
    }

    *o_bytes = शून्य;
    *o_आकार  = 0;

#अगर d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = शून्य;
        M3_परिणाम            परिणाम = ModuleSnapshot(i_module, i_name, &snap);

        अगर (परिणाम) {
            वापस परिणाम;
        }
        अगर (not snap) {
            वापस "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->तथ्य;
        *o_आकार  = snap->आकार;

        वापस m3Err_none;
    }
#अन्यथा
    (व्योम)i_name;
    वापस "snapshots are not available in this build of Wasm3";
#पूर्ण_अगर
}

M3_परिणाम m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, स्थिर अक्षर* i_name)
{
    अगर (not io_runtime or not i_module) {
        वापस m3Err_mallocFailed;
    }
#अगर d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = शून्य;
        M3_परिणाम            परिणाम = ModuleSnapshot(i_module, i_name, &snap);
        M3_बफर_Reader      br;

        अगर (परिणाम) {
            वापस परिणाम;
        }
        अगर (not snap) {
            वापस "the module holds no embedded snapshot of that name";
        }

        br.बफर = snap->तथ्य;
        br.आकार   = snap->आकार;
        br.cursor = 0;

        वापस LoadSnapshot(io_runtime, i_module, बफर_Reader_Read, &br, true);
    }
#अन्यथा
    (व्योम)i_name;
    वापस "snapshots are not available in this build of Wasm3";
#पूर्ण_अगर
}


M3_परिणाम m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, स्थिर अक्षर* i_name,
                                  व्योम** o_bytes, माप_प्रकार* o_आकार)
{
#अगर d_m3HasSnapshots
    M3_परिणाम       परिणाम = m3Err_none;
    M3_बफर_Writer bw     = { शून्य, 0, 0 };
    u8*            out    = शून्य;

    अक्षर      secName[d_m3_Max_Snapshot_Name_लंबाई + माप("snapshot.")];
    u32       nameLen, name_Len_Leb_आकार, payload_आकार, payload_आकार_Leb_आकार;
    माप_प्रकार    orig_आकार, योग_आकार;
    स्थिर u8* dropStart = शून्य;
    स्थिर u8* dropEnd   = शून्य;
    IM3Module saved     = शून्य;
    u8*       p;

    अगर (not io_runtime or not i_module or not o_bytes or not o_आकार) {
        वापस m3Err_mallocFailed;
    }

    *o_bytes = शून्य;
    *o_आकार  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, माप(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->मूल_Continuation or io_runtime->मूल_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, बफर_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.आकार);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.आकार > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, शून्य, शून्य));

    orig_आकार           = (माप_प्रकार)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (माप_प्रकार)(dropEnd - dropStart) : 0);
    nameLen            = (u32)म_माप(secName);
    name_Len_Leb_आकार     = आकार_Of_LEB_u32(nameLen);
    payload_आकार        = name_Len_Leb_आकार + nameLen + (u32)bw.आकार;
    payload_आकार_Leb_आकार = आकार_Of_LEB_u32(payload_आकार);
    योग_आकार          = orig_आकार + 1 + payload_आकार_Leb_आकार + payload_आकार;

    out = (u8*)m3_Malloc("wasm_snapshot_module", योग_आकार);
    _throwifnull(out);

    p = out;

    अगर (dropStart) {
        माप_प्रकार before = (माप_प्रकार)(dropStart - i_module->wasmStart);
        माप_प्रकार after  = (माप_प्रकार)(i_module->wasmEnd - dropEnd);

        स_नकल(p, i_module->wasmStart, before);
        p += before;
        स_नकल(p, dropEnd, after);
        p += after;
    } अन्यथा {
        स_नकल(p, i_module->wasmStart, orig_आकार);
        p += orig_आकार;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_आकार);
    p    = EmitLEB_u32(p, nameLen);
    स_नकल(p, secName, nameLen);
    p += nameLen;
    स_नकल(p, bw.बफर, bw.आकार);

    *o_bytes = out;
    *o_आकार  = योग_आकार;
    out      = शून्य;

_catch:
    m3_Free(bw.बफर);
    m3_Free(out);

    वापस परिणाम;
#अन्यथा
    (व्योम)io_runtime;
    (व्योम)i_module;
    (व्योम)i_name;
    (व्योम)o_bytes;
    (व्योम)o_आकार;
    वापस "snapshots are not available in this build of Wasm3";
#पूर्ण_अगर
}
