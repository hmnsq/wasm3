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

#অন্তর্ভাব "m3_env.h"
#অন্তর্ভাব "m3_compile.h"
#অন্তর্ভাব "m3_exception.h"
#অন্তর্ভাব "m3_host.h"

#অন্তর্ভাব <limits.h>

#যদি d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
স্থির ধ্রুবক u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
স্থির ধ্রুবক u32 c_snapshotVersion  = 1;

গণনা {
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
#  define d_m3_Max_Snapshot_Name_দৈর্ঘ্য   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
গণনা {
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
স্থির
M3_ফলাফল ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasmStart or i_module->wasmEnd - i_module->wasmStart < 8);

    {
        bytes_t  pos = i_module->wasmStart + 8;
        cbytes_t end = i_module->wasmEnd;

        Xxh64_Init(&hash, 0);

        যখন (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     আকার_2      = 0;

_           (ReadLEB_u32(&আকার_2, &pos, end));
            _throwif(m3Err_wasmUnderrun, আকার_2 > (size_t)(end - pos));

            pos += আকার_2;
            যদি (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    ফেরত ফলাফল;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

স্থির
ধ্রুবক M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    ধ্রুবক M3SnapshotMap* map = i_function->snapshotMap;

    যদি (map) {
        জন্য (u32 i = 0; i < map->অঙ্ক_Safe_Points; ++i) {
            যদি (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                ফেরত &map->safePoints[i];
            }
        }
    }

    ফেরত ফাঁকা;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
স্থির
ধ্রুবক M3SafePoint* FindSafePointAt (ধ্রুবক M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 নিচের  = 0;
    u32 উপরের = i_map->অঙ্ক_Safe_Points;

    যখন (নিচের < উপরের) {
        u32 মধ্য = নিচের + (উপরের - নিচের) / 2;

        যদি (i_map->safePoints[মধ্য].wasmOffset < i_wasmOffset) {
            নিচের = মধ্য + 1;
        } অন্যথা {
            উপরের = মধ্য;
        }
    }

    জন্য (u32 i = নিচের; i < i_map->অঙ্ক_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        ধ্রুবক M3SafePoint* point = &i_map->safePoints[i];

        যদি (point->kind != i_kind or (point->flags & d_m3_Safe_Point_লেজ_Call)) {
            চালু;
        }
        যদি (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            ফেরত point;
        }
    }

    ফেরত ফাঁকা;
}

স্থির
ধ্রুবক M3BlockStart* FindBlockAt (ধ্রুবক M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    জন্য (u32 i = 0; i < i_map->অঙ্ক_Blocks; ++i) {
        যদি (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            ফেরত &i_map->blocks[i];
        }
    }

    ফেরত ফাঁকা;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
স্থির অন্তঃরেখ
bool EnclosesOffset (ধ্রুবক M3BlockStart* i_block, u32 i_wasmOffset)
{
    ফেরত i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
স্থির অন্তঃরেখ
bool HandlersLiveAt (ধ্রুবক M3SnapshotMap* i_map, ধ্রুবক M3SafePoint* i_point, ধ্রুবক M3BlockStart* i_try)
{
    ফেরত not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
স্থির
M3_ফলাফল CallSiteType (IM3Function i_caller, ধ্রুবক M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3_ফলাফল   ফলাফল = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        আকার_2   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ফাঁকা;

_   (ReadLEB_u32(&আকার_2, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    যদি (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->অঙ্ক_Functions);
        *o_type = module->functions[index].funcType;
    } অন্যথা {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->অঙ্ক_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    ফেরত ফলাফল;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
স্থির
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 অঙ্ক_Results = Get_Func_Type_অঙ্ক_Results(i_expected);

    যদি (not i_type or Get_Func_Type_অঙ্ক_Results(i_type) != অঙ্ক_Results) {
        ফেরত false;
    }

    জন্য (u16 i = 0; i < অঙ্ক_Results; ++i) {
        যদি (not IsSubTypeOf(Get_Func_Type_ফলাফল_Type(i_type, i), Get_Func_Type_ফলাফল_Type(i_expected, i))) {
            ফেরত false;
        }
    }

    ফেরত true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
প্রকারলেখ কাঠামো M3FrameRegisters {
    m3reg_t* r0;
#  যদি d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
স্থির
M3_ফলাফল Continuation_স্তূপ (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_অঙ্ক_Slots)
{
    যদি (i_cont == i_runtime->মূল_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_স্তূপ;
        *o_অঙ্ক_Slots = i_runtime->অঙ্ক_স্তূপ_Slots;
    } অন্যথা {
        *o_base     = i_cont->val_স্তূপ;
        *o_অঙ্ক_Slots = i_cont->অঙ্ক_স্তূপ_Slots;
    }

    ফেরত (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
স্থির
M3_ফলাফল Locate_মান (m3slot_t* i_base, u32 i_অঙ্ক_Slots, m3stack_t i_sp, ধ্রুবক M3_Slot_মান* i_মান, u8** o_where)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u8       type   = BaseTypeOf(i_মান->type);

    *o_where = ফাঁকা;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_মান->slot));

    যদি (not IsRegisterSlotAlias(i_মান->slot)) {
        size_t আকার_2   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? আকার(শূন্য*)
                                                                   : (Is64BitType(type) ? আকার(u64) : আকার(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_মান->slot) * আকার(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + আকার_2 > (size_t)i_অঙ্ক_Slots * আকার(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    ফেরত ফলাফল;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
স্থির
M3_ফলাফল Read_মান (u8* i_where, ধ্রুবক M3FrameRegisters* i_registers, ধ্রুবক M3_Slot_মান* i_মান, u64* o_bits,
                    শূন্য** o_reference)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u8       type   = BaseTypeOf(i_মান->type);

    *o_bits      = 0;
    *o_reference = ফাঁকা;

    যদি (i_where) {
        যদি (IsRefType(type)) {
            memcpy(o_reference, i_where, আকার(শূন্য*));
        } অন্যথা যদি (Is64BitType(type)) {
            memcpy(o_bits, i_where, আকার(u64));
        } অন্যথা {
            u32 narrow;
            memcpy(&narrow, i_where, আকার(narrow));
            *o_bits = narrow;
        }
    } অন্যথা যদি (IsFpRegisterSlotAlias(i_মান->slot)) {
#  যদি d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        যদি (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, আকার(bits));
            *o_bits = bits;
        } অন্যথা {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, আকার(u64));
        }
#  অন্যথা
        _throw(m3Err_wasmMalformed);
#  endif
    } অন্যথা {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        যদি (IsRefType(type)) {
            *o_reference = (শূন্য*)(uintptr_t)r0;
        } অন্যথা যদি (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } অন্যথা {
            *o_bits = (u32)r0;
        }
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Write_মান (u8* o_where, ধ্রুবক M3FrameRegisters* i_registers, ধ্রুবক M3_Slot_মান* i_মান, u64 i_bits,
                     শূন্য* i_reference)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u8       type   = BaseTypeOf(i_মান->type);

    যদি (o_where) {
        যদি (IsRefType(type)) {
            memcpy(o_where, &i_reference, আকার(শূন্য*));
        } অন্যথা যদি (Is64BitType(type)) {
            memcpy(o_where, &i_bits, আকার(u64));
        } অন্যথা {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, আকার(narrow));
        }
    } অন্যথা যদি (IsFpRegisterSlotAlias(i_মান->slot)) {
#  যদি d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        যদি (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, আকার(narrow));
            *i_registers->fp0 = narrow;
        } অন্যথা {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, আকার(u64));
        }
#  অন্যথা
        _throw(m3Err_wasmMalformed);
#  endif
    } অন্যথা {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        যদি (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } অন্যথা {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    ফেরত ফলাফল;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
স্থির
ধ্রুবক M3_Slot_মান* Frame_মান (ধ্রুবক M3SnapshotMap* i_map, ধ্রুবক M3SafePoint* i_point, u32 i_সূচক)
{
    যদি (i_সূচক < i_map->অঙ্ক_Locals) {
        ফেরত i_point->first_Local_মান == UINT32_MAX ? &i_map->locals[i_সূচক]
                                                      : &i_map->মানসমূহ[i_point->first_Local_মান + i_সূচক];
    }
    ফেরত &i_map->মানসমূহ[i_point->first_মান + i_সূচক - i_map->অঙ্ক_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
স্থির
শূন্য RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    যদি (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->অঙ্ক_Ret_And_Arg_Slots) + i_function->অঙ্ক_Local_Bytes;

        memcpy(where, i_function->constants, i_function->অঙ্ক_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
প্রকারলেখ কাঠামো M3PointerIds {
    শূন্য** বস্তুসমূহ;
    u32    গুনতি;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  সূচক_Capacity;       // a power of two, kept at least twice count
} M3PointerIds;

স্থির
u32 PointerBucket (শূন্য* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    ফেরত (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

স্থির
u32 PointerIds_Find (ধ্রুবক M3PointerIds* i_ids, শূন্য* i_pointer)
{
    যদি (i_ids->সূচক_Capacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->সূচক_Capacity);

        যখন (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            যদি (i_ids->বস্তুসমূহ[id] == i_pointer) {
                ফেরত id;
            }

            bucket = (bucket + 1) & (i_ids->সূচক_Capacity - 1);
        }
    }

    ফেরত d_m3SnapshotNone;
}

স্থির
M3_ফলাফল PointerIds_Add (M3PointerIds* io_ids, শূন্য* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    যদি (id != d_m3SnapshotNone) {
        *o_id = id;
        ফেরত m3Err_none;
    }

    যদি (io_ids->গুনতি == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        শূন্য** বস্তুসমূহ    = m3_ReallocArray(শূন্য*, io_ids->বস্তুসমূহ, capacity, io_ids->capacity);

        যদি (not বস্তুসমূহ) {
            ফেরত m3Err_mallocFailed;
        }

        io_ids->বস্তুসমূহ    = বস্তুসমূহ;
        io_ids->capacity = capacity;
    }

    যদি ((io_ids->গুনতি + 1) * 2 > io_ids->সূচক_Capacity) {
        u32  capacity = io_ids->সূচক_Capacity ? io_ids->সূচক_Capacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        যদি (not index) {
            ফেরত m3Err_mallocFailed;
        }

        জন্য (u32 i = 0; i < io_ids->গুনতি; ++i) {
            u32 bucket = PointerBucket(io_ids->বস্তুসমূহ[i], capacity);

            যখন (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->সূচক_Capacity = capacity;
    }

    id                = io_ids->গুনতি++;
    io_ids->বস্তুসমূহ[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->সূচক_Capacity);

    যখন (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->সূচক_Capacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    ফেরত m3Err_none;
}

স্থির
শূন্য PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->বস্তুসমূহ);
    m3_Free(io_ids->index);
    memset(io_ids, 0, আকার(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

প্রকারলেখ কাঠামো M3_বাফার_Writer {
    u8*    বাফার;
    size_t আকার_2;
    size_t capacity;
} M3_বাফার_Writer;

স্থির
M3_ফলাফল বাফার_Writer_Write (ধ্রুবক শূন্য* i_তথ্য, size_t i_আকার, শূন্য* i_userdata)
{
    M3_বাফার_Writer* bw = (M3_বাফার_Writer*)i_userdata;

    যদি (bw->আকার_2 + i_আকার > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        যখন (bw->আকার_2 + i_আকার > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->বাফার, newCap, bw->capacity);
        যদি (!newBuf) {
            ফেরত m3Err_mallocFailed;
        }
        bw->বাফার   = newBuf;
        bw->capacity = newCap;
    }

    যদি (i_আকার) {
        memcpy(bw->বাফার + bw->আকার_2, i_তথ্য, i_আকার);
        bw->আকার_2 += i_আকার;
    }

    ফেরত m3Err_none;
}

প্রকারলেখ কাঠামো M3_বাফার_Reader {
    ধ্রুবক u8* বাফার;
    size_t    আকার_2;
    size_t    cursor;
} M3_বাফার_Reader;

স্থির
M3_ফলাফল বাফার_Reader_Read (শূন্য* o_বাফার, size_t i_আকার, শূন্য* i_userdata)
{
    M3_বাফার_Reader* br = (M3_বাফার_Reader*)i_userdata;

    যদি (i_আকার > br->আকার_2 - br->cursor) {
        ফেরত m3Err_wasmMalformed;
    }

    যদি (i_আকার) {
        memcpy(o_বাফার, br->বাফার + br->cursor, i_আকার);
        br->cursor += i_আকার;
    }

    ফেরত m3Err_none;
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
প্রকারলেখ কাঠামো M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    শূন্য*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

স্থির অন্তঃরেখ
u8 M3TypeToValType (m3type_t i_type)
{
    নির্বাচন (BaseTypeOf(i_type)) {
    ক্ষেত্র c_m3Type_i32: ফেরত 0x7F;
    ক্ষেত্র c_m3Type_i64: ফেরত 0x7E;
    ক্ষেত্র c_m3Type_f32: ফেরত 0x7D;
    ক্ষেত্র c_m3Type_f64: ফেরত 0x7C;
    ক্ষেত্র c_m3Type_v128: ফেরত 0x7B;
    ক্ষেত্র c_m3Type_funcref: ফেরত 0x70;
    ক্ষেত্র c_m3Type_externref: ফেরত 0x6F;
    ক্ষেত্র c_m3Type_exnref: ফেরত 0x69;
    ক্ষেত্র c_m3Type_contref: ফেরত 0x68;
    পূর্বনির্ধারিত: ফেরত 0x00;
    }
}

স্থির অন্তঃরেখ
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    নির্বাচন (i_valtype) {
    ক্ষেত্র 0x7F: ফেরত c_m3Type_i32;
    ক্ষেত্র 0x7E: ফেরত c_m3Type_i64;
    ক্ষেত্র 0x7D: ফেরত c_m3Type_f32;
    ক্ষেত্র 0x7C: ফেরত c_m3Type_f64;
    ক্ষেত্র 0x7B: ফেরত c_m3Type_v128;
    ক্ষেত্র 0x70: ফেরত c_m3Type_funcref;
    ক্ষেত্র 0x6F: ফেরত c_m3Type_externref;
    ক্ষেত্র 0x69: ফেরত c_m3Type_exnref;
    ক্ষেত্র 0x68: ফেরত c_m3Type_contref;
    পূর্বনির্ধারিত: ফেরত c_m3Type_none;
    }
}

স্থির
M3_ফলাফল Put (M3SnapshotSave* s, ধ্রুবক শূন্য* i_তথ্য, size_t i_আকার)
{
    ফেরত s->writing ? s->writer(i_তথ্য, i_আকার, s->userdata) : m3Err_none;
}

স্থির
M3_ফলাফল PutU8 (M3SnapshotSave* s, u8 i_মান)
{
    ফেরত Put(s, &i_মান, আকার(i_মান));
}


স্থির
M3_ফলাফল PutLEB_u32 (M3SnapshotSave* s, u32 i_মান)
{
    u8  bytes[5];
    u32 len = 0;

    করো {
        u8 byte = i_মান & 0x7F;
        i_মান >>= 7;
        যদি (i_মান != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } যখন (i_মান != 0);

    ফেরত Put(s, bytes, len);
}

স্থির
M3_ফলাফল PutLEB_u64 (M3SnapshotSave* s, u64 i_মান)
{
    u8  bytes[10];
    u32 len = 0;

    করো {
        u8 byte = i_মান & 0x7F;
        i_মান >>= 7;
        যদি (i_মান != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } যখন (i_মান != 0);

    ফেরত Put(s, bytes, len);
}

স্থির
M3_ফলাফল PutLEB_i64 (M3SnapshotSave* s, i64 i_মান)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    যখন (more) {
        u8 byte = i_মান & 0x7F;
        i_মান >>= 7;
        যদি ((i_মান == 0 && !(byte & 0x40)) || (i_মান == -1 && (byte & 0x40))) {
            more = false;
        } অন্যথা {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    ফেরত Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
প্রকারলেখ M3_ফলাফল (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
স্থির
M3_ফলাফল SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_ফলাফল         ফলাফল   = m3Err_none;
    M3_বাফার_Writer   bw       = { ফাঁকা, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    শূন্য*            userdata = s->userdata;

    s->writer   = বাফার_Writer_Write;
    s->userdata = &bw;

    ফলাফল = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    যদি (ফলাফল) {
        যাও _catch;
    }

    // a section with nothing in it is left out
    যদি (bw.আকার_2) {
        _throwif("snapshot section is too large", bw.আকার_2 > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.আকার_2));
_       (Put(s, bw.বাফার, bw.আকার_2));
    }

_catch:
    m3_Free(bw.বাফার);

    ফেরত ফলাফল;
}

স্থির
u32 Function_সূচক (IM3Module i_module, IM3Function i_function)
{
    যদি (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->অঙ্ক_Functions) {
        ফেরত (u32)(i_function - i_module->functions);
    }

    ফেরত d_m3SnapshotNone;
}

স্থির
u32 Func_Type_সূচক (IM3Module i_module, IM3FuncType i_type)
{
    জন্য (u32 i = 0; i < i_module->অঙ্ক_Func_Types; ++i) {
        যদি (i_module->funcTypes[i] == i_type) {
            ফেরত i;
        }
    }

    ফেরত d_m3SnapshotNone;
}

#  যদি d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
স্থির
IM3Tag Tag_Of_সূচক (IM3Module i_module, u32 i_সূচক)
{
    IM3Tag tag = &i_module->tags[i_সূচক];

    ফেরত tag->resolved ? tag->resolved : tag;
}

স্থির
u32 Tag_সূচক (IM3Module i_module, IM3Tag i_tag)
{
    জন্য (u32 i = 0; i < i_module->অঙ্ক_Tags; ++i) {
        যদি (Tag_Of_সূচক(i_module, i) == i_tag) {
            ফেরত i;
        }
    }

    ফেরত d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
স্থির
M3_ফলাফল NameReference (M3SnapshotSave* s, u8 i_type, শূন্য* i_reference, u64* o_word)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    যদি (not i_reference) {
        ফেরত m3Err_none;
    }

    যদি (i_type == c_m3Type_funcref) {
        id = Function_সূচক(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } অন্যথা যদি (i_type == c_m3Type_externref) {
        ধ্রুবক M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        যাও _catch;
    }
#  যদি d_m3HasExceptionHandling
    অন্যথা যদি (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 গুনতি = s->exceptions.গুনতি;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.গুনতি != গুনতি);
    }
#  endif
    অন্যথা যদি (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 গুনতি = s->continuations.গুনতি;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.গুনতি != গুনতি);
    } অন্যথা {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    ফেরত ফলাফল;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
স্থির
M3_ফলাফল Put_Ref_মান (M3SnapshotSave* s, u8 i_type, শূন্য* i_reference)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    যদি (not s->writing) {
        ফেরত m3Err_none;
    }

    যদি (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } অন্যথা {
        u8 kind = 0;
        যদি (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } অন্যথা যদি (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  যদি d_m3HasExceptionHandling
        অন্যথা যদি (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        অন্যথা যদি (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    ফেরত ফলাফল;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
স্থির
M3_ফলাফল PutLE32 (M3SnapshotSave* s, u32 i_মান)
{
    u8 bytes[4];

    জন্য (u32 i = 0; i < আকার(bytes); ++i) {
        bytes[i] = (u8)(i_মান >> (8 * i));
    }

    ফেরত Put(s, bytes, আকার(bytes));
}

স্থির
M3_ফলাফল PutLE64 (M3SnapshotSave* s, u64 i_মান)
{
    u8 bytes[8];

    জন্য (u32 i = 0; i < আকার(bytes); ++i) {
        bytes[i] = (u8)(i_মান >> (8 * i));
    }

    ফেরত Put(s, bytes, আকার(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
স্থির
M3_ফলাফল Put_মান_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    যদি (IsRefType(i_type)) {
        ফেরত Put_Ref_মান(s, BaseTypeOf(i_type), (শূন্য*)(uintptr_t)i_bits);
    }

    নির্বাচন (BaseTypeOf(i_type)) {
    ক্ষেত্র c_m3Type_i32: ফেরত PutLEB_i64(s, (i32)(u32)i_bits);
    ক্ষেত্র c_m3Type_i64: ফেরত PutLEB_i64(s, (i64)i_bits);
    ক্ষেত্র c_m3Type_f32: ফেরত PutLE32(s, (u32)i_bits);
    ক্ষেত্র c_m3Type_f64: ফেরত PutLE64(s, i_bits);
    পূর্বনির্ধারিত: ফেরত "a snapshot cannot hold a value of this type as bits";
    }
}

স্থির
M3_ফলাফল Put_মান_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    যদি (not valtype) {
        ফেরত "a snapshot has no tag for a value of this type";
    }

    ফেরত PutU8(s, valtype);
}

স্থির
M3_ফলাফল Put_মান_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (Put_মান_Tag(s, i_type));
_   (Put_মান_Body(s, i_type, i_bits));

_catch:
    ফেরত ফলাফল;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
স্থির
M3_ফলাফল Put_মান (M3SnapshotSave* s, m3type_t i_type, ধ্রুবক শূন্য* i_where)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (Put_মান_Tag(s, i_type));

    যদি (IsRefType(i_type)) {
        শূন্য* reference;
        memcpy(&reference, i_where, আকার(reference));
_       (Put_মান_Body(s, i_type, (u64)(uintptr_t)reference));
    } অন্যথা যদি (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, আকার(bytes));
_       (Put(s, bytes, আকার(bytes)));
    } অন্যথা যদি (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, আকার(bits));
_       (Put_মান_Body(s, i_type, bits));
    } অন্যথা {
        u32 narrow;
        memcpy(&narrow, i_where, আকার(narrow));
_       (Put_মান_Body(s, i_type, narrow));
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল PutMemoryChunks (M3SnapshotSave* s, ধ্রুবক u8* i_bytes, size_t i_আকার)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    size_t   cursor = 0;

    যখন (cursor < i_আকার) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        যখন (cursor + run < i_আকার and i_bytes[cursor + run] == byte) {
            run++;
        }

        যদি ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            যদি (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            চালু;
        }

        size_t rawStart = cursor;

        যখন (cursor < i_আকার) {
            u8     b      = i_bytes[cursor];
            size_t দৈর্ঘ্য = 1;

            যখন (cursor + দৈর্ঘ্য < i_আকার and i_bytes[cursor + দৈর্ঘ্য] == b) {
                দৈর্ঘ্য++;
            }

            যদি ((b == 0x00 or b == 0xFF) and দৈর্ঘ্য >= d_m3SnapshotRunThreshold) {
                বিরতি;
            }

            cursor += দৈর্ঘ্য;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)rawStart));
_       (PutLEB_u32(s, (u32)(cursor - rawStart)));
_       (Put(s, i_bytes + rawStart, cursor - rawStart));
    }

_   (PutU8(s, d_m3ChunkEnd));

_catch:
    ফেরত ফলাফল;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
স্থির
u32 Memory_First_সূচক (IM3Module module, u32 index)
{
    u32 j = 0;
    যখন (module->memories[j] != module->memories[index]) {
        ++j;
    }
    ফেরত j;
}

স্থির
u32 Table_First_সূচক (IM3Module module, u32 index)
{
    u32 j = 0;
    যখন (module->tables[j] != module->tables[index]) {
        ++j;
    }
    ফেরত j;
}

স্থির
M3_ফলাফল SaveMemories (M3SnapshotSave* s)
{
    M3_ফলাফল  ফলাফল = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->অঙ্ক_Memories));

    জন্য (u32 m = 0; m < module->অঙ্ক_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_First_সূচক(module, m);
        bool      has_তথ্য  = memory and memory->mallocated;
        u32       page_আকার = memory ? Memory_Page_আকার(memory) : d_m3_Default_Mem_Page_আকার;
        u64       bytes    = memory ? memory->অঙ্ক_Pages * (u64)page_আকার : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        যদি (first != m) {
            চালু;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", memory and memory->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", bytes > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 memory and memory->mallocated and memory->mallocated->দৈর্ঘ্য < bytes);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        যখন ((1u << pageBits) < page_আকার) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->অঙ্ক_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        যদি (not s->writing) {
            চালু;
        }

        যদি (has_তথ্য) {
_           (PutMemoryChunks(s, m3_Mem_তথ্য(memory->mallocated), (size_t)bytes));
        } অন্যথা {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল SaveGlobals (M3SnapshotSave* s)
{
    M3_ফলাফল  ফলাফল = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->অঙ্ক_Globals));

    জন্য (u32 g = 0; g < module->অঙ্ক_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_মান(s, global->type, &cell->i64_মান));
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল SaveTables (M3SnapshotSave* s)
{
    M3_ফলাফল  ফলাফল = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->অঙ্ক_Tables));

    জন্য (u32 t = 0; t < module->অঙ্ক_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_First_সূচক(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        যদি (first != t) {
            চালু;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->আকার_2));

        জন্য (u32 e = 0; e < table->আকার_2; ++e) {
_           (Put_Ref_মান(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল SaveSegments (M3SnapshotSave* s)
{
    M3_ফলাফল  ফলাফল = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->অঙ্ক_তথ্য_Segments));

    জন্য (u32 i = 0; i < module->অঙ্ক_তথ্য_Segments; ++i) {
_       (PutU8(s, module->তথ্য_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->অঙ্ক_Element_Segments));

    জন্য (u32 i = 0; i < module->অঙ্ক_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    ফেরত ফলাফল;
}

#  যদি d_m3HasExceptionHandling

স্থির
M3_ফলাফল SaveExceptions (M3SnapshotSave* s)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.গুনতি));

    জন্য (u32 i = 0; i < s->exceptions.গুনতি; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.বস্তুসমূহ[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->অঙ্ক_Args != Get_Func_Type_অঙ্ক_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->অঙ্ক_Args));

        জন্য (u32 a = 0; a < exn->অঙ্ক_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_মান_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    ফেরত ফলাফল;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
প্রকারলেখ কাঠামো M3Activation {
    IM3Function        function;
    ধ্রুবক M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    ধ্রুবক M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
স্থির
ধ্রুবক M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    ধ্রুবক M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : ফাঁকা;

    জন্য (u32 i = 0; not point and i < i_module->অঙ্ক_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        যদি (point) {
            *io_function = &i_module->functions[i];
        }
    }

    ফেরত point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
স্থির
M3_ফলাফল পরবর্তী_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, আকার(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    জন্য (; i >= 0; --i) {
        ধ্রুবক M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        যদি (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  যদি d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            বিরতি;
        } অন্যথা যদি (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            বিরতি;
        }
    }

    যদি (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  যদি d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  যদি d_m3EntryKeepsFrame
    জন্য (i32 k = i_first; k > o_act->lastFrame; --k) {
        ধ্রুবক M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    ফেরত ফলাফল;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
স্থির
M3_ফলাফল SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_অঙ্ক_Slots, ধ্রুবক M3Activation* i_act)
{
    M3_ফলাফল             ফলাফল        = m3Err_none;
    IM3Function          function      = i_act->function;
    ধ্রুবক M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_সূচক = Function_সূচক(s->module, function);
    ধ্রুবক M3SafePoint*   point         = i_act->point;
    u32                  অঙ্ক_মানসমূহ;

    _throwif("a suspended frame belongs to another module", function_সূচক == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_সূচক));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    যদি (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    অঙ্ক_মানসমূহ = map->অঙ্ক_Locals + point->অঙ্ক_মানসমূহ;

_   (PutLEB_u32(s, অঙ্ক_মানসমূহ));

    জন্য (u32 v = 0; v < অঙ্ক_মানসমূহ; ++v) {
        ধ্রুবক M3_Slot_মান* মান = Frame_মান(map, point, v);
        u8*                where;
        u64                word;
        শূন্য*              reference;

_       (Locate_মান(i_base, i_অঙ্ক_Slots, i_act->sp, মান, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        যদি (BaseTypeOf(মান->type) == c_m3Type_v128) {
_           (Put_মান_Tag(s, মান->type));
_           (Put(s, where, 16));
            চালু;
        }

_       (Read_মান(where, &i_act->registers, মান, &word, &reference));

_       (Put_মান_Bits(s, মান->type, IsRefType(মান->type) ? (u64)(uintptr_t)reference : word));
    }

    যদি (point->kind == safepoint_resume) {
_       (Put_Ref_মান(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    ফেরত ফলাফল;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
স্থির
M3_ফলাফল SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_ফলাফল      ফলাফল   = m3Err_none;
    M3Activation* acts     = ফাঁকা;
    u32           অঙ্ক_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->অঙ্ক_Frames - 1;
    m3slot_t*     base;
    u32           অঙ্ক_Slots;
    m3stack_t     sp;

_   (Continuation_স্তূপ(s->runtime, i_cont, &base, &অঙ্ক_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->অঙ্ক_Frames + 1);
    _throwifnull(acts);

    sp = base;

    যখন (true) {
        M3Activation* act = &acts[অঙ্ক_Acts];

_       (পরবর্তী_Activation(s->module, i_cont, function, sp, first, act));

        যদি (act->kind != safepoint_call) {
            অঙ্ক_Acts++;
            বিরতি;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        যদি (not (act->point->flags & d_m3_Safe_Point_লেজ_Call)) {
            অঙ্ক_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, অঙ্ক_Acts));

    জন্য (u32 a = 0; a < অঙ্ক_Acts; ++a) {
_       (SaveActivation(s, base, অঙ্ক_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_ফলাফল  ফলাফল    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_মূল    = (i_cont == s->runtime->মূল_Continuation);
    u8        state     = snapshot_contFinished;
    u32       type_সূচক = is_মূল ? d_m3SnapshotNone : Func_Type_সূচক(module, i_cont->type);

    যদি (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } অন্যথা যদি (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } অন্যথা যদি (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", Function_সূচক(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_মূল and type_সূচক == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_মূল));
_   (PutLEB_u32(s, type_সূচক));
_   (PutLEB_u32(s, Function_সূচক(module, i_cont->entryFunction)));
#  যদি d_m3HasExceptionHandling
_   (Put_Ref_মান(s, c_m3Type_exnref, i_cont->resumeThrow));
#  অন্যথা
_   (Put_Ref_মান(s, c_m3Type_exnref, ফাঁকা));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    যদি (s->postmortem) {
        যাও _catch;
    }

    যদি (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : ফাঁকা;
        u16         অঙ্ক_Rets = inner ? Get_Func_Type_অঙ্ক_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_গুনতি > Get_Func_Type_অঙ্ক_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_গুনতি));

        জন্য (u32 i = 0; i < i_cont->bound_Args_গুনতি; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_মান(s, type, i_cont->val_স্তূপ + (অঙ্ক_Rets + i) * c_io_Slot_গুনতি));
        }
    } অন্যথা যদি (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_গুনতি));
_       (SaveSuspendedBody(s, i_cont));
    } অন্যথা {
_       (PutLEB_u32(s, 0));
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল SaveContinuations (M3SnapshotSave* s)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (PutLEB_u32(s, s->continuations.গুনতি));

    জন্য (u32 i = 0; i < s->continuations.গুনতি; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.বস্তুসমূহ[i]));
    }

_catch:
    ফেরত ফলাফল;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
স্থির
M3_ফলাফল SaveHostState (M3SnapshotSave* s)
{
    M3_ফলাফল               ফলাফল = m3Err_none;
    ধ্রুবক M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_বাফার_Writer         bw     = { ফাঁকা, 0, 0 };

    যদি (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, বাফার_Writer_Write, &bw));
    }

    যদি (bw.আকার_2 > 0) {
_       (Put(s, bw.বাফার, bw.আকার_2));
    }

_catch:
    m3_Free(bw.বাফার);

    ফেরত ফলাফল;
}

স্থির
শূন্য SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    জন্য (u32 i = 0; i < module->অঙ্ক_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        যদি (Memory_First_সূচক(module, i) == i and memory) {
            *memoryBytes += memory->অঙ্ক_Pages * (u64)Memory_Page_আকার(memory);
        }
    }
    জন্য (u32 i = 0; i < module->অঙ্ক_Tables; ++i) {
        IM3Table table = module->tables[i];
        যদি (Table_First_সূচক(module, i) == i and table) {
            *tableElements += table->আকার_2;
        }
    }
}

স্থির
M3_ফলাফল SaveMeta (M3SnapshotSave* s)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    জন্য (u32 i = 0; i < s->continuations.গুনতি; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.বস্তুসমূহ[i];
        যদি (cont != s->runtime->মূল_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.গুনতি));
_   (PutLEB_u32(s, s->exceptions.গুনতি));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  যদি d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    জন্য (u32 i = 0; i < s->exceptions.গুনতি; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.বস্তুসমূহ[i];
        u32          tag_সূচক  = Tag_সূচক(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tag_সূচক == d_m3SnapshotNone);

_       (PutLEB_u32(s, tag_সূচক));
    }
#  endif

_catch:
    ফেরত ফলাফল;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
স্থির
M3_ফলাফল DiscoverReferences (M3SnapshotSave* s)
{
    M3_ফলাফল ফলাফল            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    যখন (doneContinuations < s->continuations.গুনতি or doneExceptions < s->exceptions.গুনতি) {
#  যদি d_m3HasExceptionHandling
        যখন (doneExceptions < s->exceptions.গুনতি) {
            M3Exception* exception = (M3Exception*)s->exceptions.বস্তুসমূহ[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->অঙ্ক_Args != Get_Func_Type_অঙ্ক_Params(type));

            জন্য (u32 a = 0; a < exception->অঙ্ক_Args; ++a) {
_               (Put_মান_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        যখন (doneContinuations < s->continuations.গুনতি and doneExceptions == s->exceptions.গুনতি) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.বস্তুসমূহ[id]));
        }
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল SaveSections (M3SnapshotSave* s)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (not s->writing) {
        ফেরত DiscoverReferences(s);
    }

    যদি (not s->embedded) {
_       (Put(s, c_snapshotMagic, আকার(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    যদি (s->module->অঙ্ক_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    যদি (s->module->অঙ্ক_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    যদি (s->module->অঙ্ক_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    যদি (s->module->অঙ্ক_তথ্য_Segments or s->module->অঙ্ক_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  যদি d_m3HasExceptionHandling
    যদি (s->exceptions.গুনতি) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    যদি (s->continuations.গুনতি) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    ফেরত ফলাফল;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

প্রকারলেখ কাঠামো M3SnapshotSuspendSite {
    ধ্রুবক M3SnapshotMap* map;
    ধ্রুবক M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

প্রকারলেখ কাঠামো M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    শূন্য*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_স্তূপ;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    অঙ্ক_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  যদি d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 অঙ্ক_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

স্থির
M3_ফলাফল Get (M3SnapshotLoad* l, শূন্য* o_তথ্য, size_t i_আকার)
{
    যদি (not l->inSection) {
        ফেরত l->reader(o_তথ্য, i_আকার, l->userdata);
    }
    যদি (i_আকার > (size_t)(l->end - l->pos)) {
        ফেরত m3Err_wasmUnderrun;
    }
    যদি (i_আকার) {
        memcpy(o_তথ্য, l->pos, i_আকার);
        l->pos += i_আকার;
    }
    ফেরত m3Err_none;
}

স্থির
M3_ফলাফল GetU8 (M3SnapshotLoad* l, u8* o_মান)
{
    ফেরত Get(l, o_মান, আকার(*o_মান));
}

স্থির
M3_ফলাফল GetBool (M3SnapshotLoad* l, u8* o_মান)
{
    M3_ফলাফল ফলাফল = GetU8(l, o_মান);
    ফেরত ফলাফল ? ফলাফল : (*o_মান <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

স্থির
M3_ফলাফল GetLEB_u32 (M3SnapshotLoad* l, u32* o_মান)
{
    যদি (l->inSection) {
        ফেরত ReadLEB_u32(o_মান, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    জন্য (u32 i = 0; i < আকার(encoded); ++i) {
        M3_ফলাফল ফলাফল = GetU8(l, &encoded[i]);
        যদি (ফলাফল) {
            ফেরত ফলাফল;
        }
        যদি (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            ফেরত ReadLEB_u32(o_মান, &pos, encoded + i + 1);
        }
    }
    ফেরত m3Err_lebOverflow;
}

স্থির
M3_ফলাফল GetLEB_u64 (M3SnapshotLoad* l, u64* o_মান)
{
    ফেরত ReadLebUnsigned(o_মান, 64, &l->pos, l->end);
}

স্থির
M3_ফলাফল GetLEB_i64 (M3SnapshotLoad* l, i64* o_মান)
{
    ফেরত ReadLEB_i64(o_মান, &l->pos, l->end);
}

স্থির
M3_ফলাফল SkipBytes (M3SnapshotLoad* l, size_t i_আকার)
{
    যদি (i_আকার > (size_t)(l->end - l->pos)) {
        ফেরত m3Err_wasmUnderrun;
    }
    l->pos += i_আকার;
    ফেরত m3Err_none;
}


স্থির
M3_ফলাফল Get_Ref_মান (M3SnapshotLoad* l, m3type_t i_type, শূন্য** o_reference)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = ফাঁকা;

_   (GetU8(l, &kind));
    যদি (kind == 0x00) {
        ফেরত (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    যদি (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->অঙ্ক_Functions);
#  যদি d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } অন্যথা যদি (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        ধ্রুবক M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        যদি (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  যদি d_m3HasExceptionHandling
    অন্যথা যদি (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->অঙ্ক_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    অন্যথা যদি (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->অঙ্ক_Continuations);
        *o_reference = l->continuations[id];
#  যদি d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        যদি (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } অন্যথা {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল GetLE32 (M3SnapshotLoad* l, u32* o_মান)
{
    u8       bytes[4];
    M3_ফলাফল ফলাফল = Get(l, bytes, আকার(bytes));

    *o_মান = 0;

    জন্য (u32 i = 0; i < আকার(bytes); ++i) {
        *o_মান |= (u32)bytes[i] << (8 * i);
    }

    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল GetLE64 (M3SnapshotLoad* l, u64* o_মান)
{
    u8       bytes[8];
    M3_ফলাফল ফলাফল = Get(l, bytes, আকার(bytes));

    *o_মান = 0;

    জন্য (u32 i = 0; i < আকার(bytes); ++i) {
        *o_মান |= (u64)bytes[i] << (8 * i);
    }

    ফেরত ফলাফল;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
স্থির
M3_ফলাফল Get_মান_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_ফলাফল ফলাফল  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    যদি (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    ফেরত ফলাফল;
}

// The mirror of PutValueBody: narrow types come back in the low bits
স্থির
M3_ফলাফল Get_মান_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    *o_bits = 0;

    যদি (IsRefType(i_type)) {
        শূন্য* reference = ফাঁকা;
_       (Get_Ref_মান(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } অন্যথা যদি (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 মান = 0;
_       (ReadLEB_i32(&মান, &l->pos, l->end));
        *o_bits = (u32)মান;
    } অন্যথা যদি (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 মান = 0;
_       (GetLEB_i64(l, &মান));
        *o_bits = (u64)মান;
    } অন্যথা যদি (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } অন্যথা যদি (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } অন্যথা {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Get_মান_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    m3type_t type;

_   (Get_মান_Tag(l, i_expectedType, &type));
_   (Get_মান_Body(l, type, o_bits));

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল Get_মান (M3SnapshotLoad* l, m3type_t i_expectedType, শূন্য* o_where)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    যদি (l->checking) {
        o_where = scratch;
    }

_   (Get_মান_Tag(l, i_expectedType, &type));

    যদি (IsRefType(type)) {
        শূন্য* reference = ফাঁকা;
        u64   bits      = 0;
_       (Get_মান_Body(l, type, &bits));
        reference = (শূন্য*)(uintptr_t)bits;
        memcpy(o_where, &reference, আকার(reference));
    } অন্যথা যদি (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } অন্যথা {
        u64 bits = 0;
_       (Get_মান_Body(l, type, &bits));

        যদি (Is64BitType(type)) {
            memcpy(o_where, &bits, আকার(bits));
        } অন্যথা {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, আকার(narrow));
        }
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->অঙ্ক_Functions);
    *o_function = &l->module->functions[index];

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_আকার)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    জন্য (;;) {
        u8  chunkType = 0;
        u32 offset = 0, দৈর্ঘ্য = 0;

_       (GetU8(l, &chunkType));

        যদি (chunkType == d_m3ChunkEnd) {
            বিরতি;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &দৈর্ঘ্য));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_আকার or দৈর্ঘ্য > i_আকার - (size_t)offset);

        যদি (chunkType == d_m3ChunkRaw) {
            যদি (l->checking or দৈর্ঘ্য == 0) {
_               (SkipBytes(l, দৈর্ঘ্য));
            } অন্যথা {
_               (Get(l, o_bytes + offset, দৈর্ঘ্য));
            }
        } অন্যথা যদি (chunkType == d_m3ChunkFillFF) {
            যদি (not l->checking and দৈর্ঘ্য) {
                memset(o_bytes + offset, 0xFF, দৈর্ঘ্য);
            }
        } অন্যথা {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadMemories (M3SnapshotLoad* l)
{
    M3_ফলাফল  ফলাফল      = m3Err_none;
    IM3Module module      = l->module;
    u32       অঙ্ক_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &অঙ্ক_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, অঙ্ক_Memories == 0 or অঙ্ক_Memories != module->অঙ্ক_Memories);

    জন্য (u32 m = 0; m < অঙ্ক_Memories; ++m) {
        u32       mem_সূচক = 0, first_সূচক = 0, pageBits = 0, অঙ্ক_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &mem_সূচক));
_       (GetLEB_u32(l, &first_সূচক));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_সূচক != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 first_সূচক != Memory_First_সূচক(module, m));
        যদি (first_সূচক != m) {
            চালু;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &অঙ্ক_Pages));

        memory = module->memories[mem_সূচক];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    তথ্য  = ফাঁকা;

        যদি (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_Page_আকার(memory));
            _throwif(m3Err_wasmMalformed, অঙ্ক_Pages < memory->অঙ্ক_Pages or অঙ্ক_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, অঙ্ক_Pages > SIZE_MAX / Memory_Page_আকার(memory));
            bytes = (size_t)অঙ্ক_Pages * Memory_Page_আকার(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            যদি (not l->checking) {
                যদি (memory->অঙ্ক_Pages < অঙ্ক_Pages) {
_                   (ResizeMemory(l->runtime, memory, অঙ্ক_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->দৈর্ঘ্য < bytes);
            }

            l->loadedMemoryBytes += bytes;

            যদি (bytes and memory->mallocated) {
                তথ্য = m3_Mem_তথ্য(memory->mallocated);
            }
        } অন্যথা {
            _throwif(m3Err_wasmMalformed, অঙ্ক_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        যদি (তথ্য and not l->checking) {
            memset(তথ্য, 0, bytes);
        }

_       (LoadMemoryChunks(l, তথ্য, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadGlobals (M3SnapshotLoad* l)
{
    M3_ফলাফল  ফলাফল     = m3Err_none;
    IM3Module module     = l->module;
    u32       অঙ্ক_Globals = 0;

_   (GetLEB_u32(l, &অঙ্ক_Globals));
    _throwif(m3Err_wasmMalformed, অঙ্ক_Globals == 0 or অঙ্ক_Globals != module->অঙ্ক_Globals);

    জন্য (u32 g = 0; g < অঙ্ক_Globals; ++g) {
        u32       global_সূচক = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_সূচক));
        _throwif(m3Err_wasmMalformed, global_সূচক != g);

        global = &module->globals[global_সূচক];
        cell   = global->resolved ? global->resolved : global;

_       (Get_মান(l, global->type, &cell->i64_মান));
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadTables (M3SnapshotLoad* l)
{
    M3_ফলাফল  ফলাফল       = m3Err_none;
    IM3Module module       = l->module;
    u32       অঙ্ক_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &অঙ্ক_Tables));
    _throwif(m3Err_wasmMalformed, অঙ্ক_Tables == 0 or অঙ্ক_Tables != module->অঙ্ক_Tables);

    জন্য (u32 t = 0; t < অঙ্ক_Tables; ++t) {
        u32      table_সূচক = 0, first_সূচক = 0, আকার_2 = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &table_সূচক));
_       (GetLEB_u32(l, &first_সূচক));

        _throwif(m3Err_wasmMalformed, table_সূচক != t);
        _throwif("the snapshot shares tables between imports differently",
                 first_সূচক != Table_First_সূচক(module, t));
        যদি (first_সূচক != t) {
            চালু;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &আকার_2));

        table = module->tables[table_সূচক];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, আকার_2 > (table->hasMax ? table->max_আকার : d_m3_Max_Sane_Table_আকার));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, আকার_2 < table->আকার_2);

        l->loadedTableElements += আকার_2;

        যদি (not l->checking and আকার_2 != table->আকার_2) {
            শূন্য** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 আকার_2 - table->আকার_2 > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(শূন্য*, table->elements, আকার_2, table->আকার_2);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += আকার_2 - table->আকার_2;
            table->elements = elements;
            table->আকার_2     = আকার_2;
        }

        জন্য (u32 e = 0; e < আকার_2; ++e) {
            শূন্য* reference = ফাঁকা;
_           (Get_Ref_মান(l, table->type, &reference));
            যদি (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadSegments (M3SnapshotLoad* l)
{
    M3_ফলাফল  ফলাফল = m3Err_none;
    IM3Module module = l->module;
    u32       গুনতি  = 0;

_   (GetLEB_u32(l, &গুনতি));
    _throwif(m3Err_wasmMalformed, গুনতি != module->অঙ্ক_তথ্য_Segments);
    _throwif(m3Err_wasmMalformed, module->অঙ্ক_তথ্য_Segments == 0 and module->অঙ্ক_Element_Segments == 0);

    জন্য (u32 i = 0; i < গুনতি; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        যদি (not l->checking) {
            module->তথ্য_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &গুনতি));
    _throwif(m3Err_wasmMalformed, গুনতি != module->অঙ্ক_Element_Segments);

    জন্য (u32 i = 0; i < গুনতি; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        যদি (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    ফেরত ফলাফল;
}

// A frame for the continuation being put together, outermost first
স্থির
M3_ফলাফল AddFrame (M3SnapshotLoad* l, u32* io_অঙ্ক_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    যদি (*io_অঙ্ক_Frames >= d_m3ContinuationMaxFrames) {
        ফেরত "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_অঙ্ক_Frames)++];

    memset(frame, 0, আকার(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    ফেরত m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
স্থির
M3_ফলাফল PrepareFunction (IM3Function i_function)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    যদি (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    ফেরত ফলাফল;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
স্থির
M3_ফলাফল AddBlockFrames (M3SnapshotLoad* l, ধ্রুবক M3SnapshotMap* i_map, ধ্রুবক M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_অঙ্ক_Frames)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    জন্য (u32 b = 0; b < i_map->অঙ্ক_Blocks; ++b) {
        ধ্রুবক M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        যদি (not EnclosesOffset(block, i_point->wasmOffset)) {
            চালু;
        }

        যদি (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_অঙ্ক_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  যদি d_m3HasExceptionHandling
        অন্যথা {
_           (AddFrame(l, io_অঙ্ক_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.অঙ্ক_Clauses   = block->অঙ্ক_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    ফেরত ফলাফল;
}

// Puts a function's values where this build keeps them at the safepoint
স্থির
M3_ফলাফল Load_মানসমূহ (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_অঙ্ক_Slots, m3stack_t i_sp,
                     ধ্রুবক M3SnapshotMap* i_map, ধ্রুবক M3SafePoint* i_point, ধ্রুবক M3FrameRegisters* i_registers)
{
    M3_ফলাফল ফলাফল    = m3Err_none;
    u32      অঙ্ক_মানসমূহ = 0;

_   (GetLEB_u32(l, &অঙ্ক_মানসমূহ));
    _throwif("the snapshot's frame does not match what this build compiled", অঙ্ক_মানসমূহ != i_map->অঙ্ক_Locals + i_point->অঙ্ক_মানসমূহ);

    জন্য (u32 v = 0; v < অঙ্ক_মানসমূহ; ++v) {
        ধ্রুবক M3_Slot_মান* মান     = Frame_মান(i_map, i_point, v);
        u64                word      = 0;
        শূন্য*              reference = ফাঁকা;
        u8*                where;

_       (Locate_মান(i_base, i_অঙ্ক_Slots, i_sp, মান, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        যদি (BaseTypeOf(মান->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_মান_Tag(l, মান->type, &type));
            যদি (l->checking) {
_               (SkipBytes(l, 16));
            } অন্যথা {
_               (Get(l, where, 16));
            }
            চালু;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= অঙ্ক_মানসমূহ - i_point->অঙ্ক_Results;
_       (Get_মান_Bits(l, মান->type, &word));
        l->allowUnfilledRef = false;

        যদি (IsRefType(মান->type)) {
            reference = (শূন্য*)(uintptr_t)word;
        }

        যদি (not l->checking) {
_           (Write_মান(where, i_registers, মান, word, reference));
        }
    }

_catch:
    ফেরত ফলাফল;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
স্থির
M3_ফলাফল LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3_ফলাফল    ফলাফল    = m3Err_none;
    u32         অঙ্ক_Frames = 0;
    M3Frame*    callFrame = ফাঁকা;
    IM3Function function  = ফাঁকা;
    m3slot_t*   base;
    u32         অঙ্ক_Slots;
    u32         অঙ্ক_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_স্তূপ(l->runtime, io_cont, &base, &অঙ্ক_Slots));

_   (GetLEB_u32(l, &অঙ্ক_Activations));
    _throwif(m3Err_wasmMalformed, অঙ্ক_Activations == 0 or অঙ্ক_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = ফাঁকা;
    io_cont->r0                = 0;
    io_cont->অঙ্ক_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  যদি d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    জন্য (u32 a = 0; a < অঙ্ক_Activations; ++a) {
        bool                 isInnermost = (a + 1 == অঙ্ক_Activations);
        ধ্রুবক M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        ধ্রুবক M3SnapshotMap* map;

        memset(&registers, 0, আকার(registers));

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
                 (u32)(sp - base) + function->max_স্তূপ_Slots > অঙ্ক_Slots);

        // the call the frame outside this one is waiting on
        যদি (callFrame) {
            callFrame->call.function = function;
        }

#  যদি d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &অঙ্ক_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        যদি (kind == safepoint_op) {
            ধ্রুবক M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &অঙ্ক_Frames));

        যদি (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &অঙ্ক_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  যদি d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } অন্যথা যদি (kind == safepoint_resume) {
            M3Frame* frame;
            u32      অঙ্ক_Results = point->অঙ্ক_Results;
            pc_t     resultsPC  = point->pc - 2 * অঙ্ক_Results;

_           (AddFrame(l, &অঙ্ক_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.অঙ্ক_Results  = অঙ্ক_Results;
            frame->resume.অঙ্ক_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } অন্যথা {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  যদি d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_মানসমূহ(l, base, অঙ্ক_Slots, sp, map, point, &registers));

        যদি (not l->checking) {
            RestoreConstants(function, sp);
        }

        যদি (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } অন্যথা যদি (kind == safepoint_resume) {
            শূন্য* cont = ফাঁকা;
_           (Get_Ref_মান(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ফাঁকা);

            callFrame->resume.cont = (IM3Continuation)cont;
            যদি (not l->checking) {
                l->suspendSites[i_id].point = point;
                জন্য (u32 i = 1; i < l->অঙ্ক_Continuations; ++i) {
                    যদি (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        বিরতি;
                    }
                }
            }
        } অন্যথা {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            যদি (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            যদি (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->অঙ্ক_Results > point->অঙ্ক_মানসমূহ or
                                                point->অঙ্ক_Results > d_m3MaxContinuationPayload);

                io_cont->অঙ্ক_Suspend_Results = point->অঙ্ক_Results;
                যদি (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                জন্য (u32 i = 0; i < point->অঙ্ক_Results; ++i) {
                    ধ্রুবক M3_Slot_মান* মান = &map->মানসমূহ[point->first_মান + point->অঙ্ক_মানসমূহ - point->অঙ্ক_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(মান->slot));

                    io_cont->suspend_ফলাফল_Offsets[i] = মান->slot;
                    io_cont->suspend_ফলাফল_Is64[i]    = Is64BitType(মান->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    যদি (l->checking) {
        ফেরত m3Err_none;
    }

    // held innermost first
    যদি (অঙ্ক_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, অঙ্ক_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = অঙ্ক_Frames;
    }

    জন্য (u32 k = 0; k < অঙ্ক_Frames; ++k) {
        io_cont->frames[অঙ্ক_Frames - 1 - k] = l->frames[k];
    }

    io_cont->অঙ্ক_Frames = অঙ্ক_Frames;

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3_ফলাফল        ফলাফল = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    যদি (l->checking) {
        scratch          = *cont;
        scratch.val_স্তূপ = l->check_স্তূপ;
        যদি (i_id == 0) {
            scratch.val_স্তূপ = (m3slot_t*)l->runtime->origin_স্তূপ;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_সূচক = 0, entry_সূচক = 0, bound_Args_গুনতি = 0;
    u8  state = 0, is_মূল = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_মূল));
_   (GetLEB_u32(l, &type_সূচক));
_   (GetLEB_u32(l, &entry_সূচক));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_মূল != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_মূল and state != snapshot_contSuspended);
    যদি (not is_মূল and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_সূচক != d_m3SnapshotNone and type_সূচক >= module->অঙ্ক_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entry_সূচক >= module->অঙ্ক_Functions);
    _throwif(m3Err_wasmMalformed, is_মূল ? type_সূচক != d_m3SnapshotNone : (type_সূচক == d_m3SnapshotNone or not module->funcTypes[type_সূচক]->isContinuation));

    যদি (not l->checking and not is_মূল and state != snapshot_contFinished) {
_       (Continuation_Acquire_স্তূপ(l->runtime, cont));
    }
    cont->type          = (type_সূচক != d_m3SnapshotNone) ? module->funcTypes[type_সূচক] : ফাঁকা;
    cont->entryFunction = &module->functions[entry_সূচক];
    cont->অঙ্ক_Handlers   = 0;
    cont->handlersPC    = ফাঁকা;
    cont->parent        = ফাঁকা;
    cont->অঙ্ক_Frames     = 0;

#  যদি d_m3HasExceptionHandling
    {
        শূন্য* ref = ফাঁকা;
_       (Get_Ref_মান(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_মূল or state == snapshot_contFinished) and cont->resumeThrow != ফাঁকা);
#  অন্যথা
    {
        শূন্য* ref = ফাঁকা;
_       (Get_Ref_মান(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ফাঁকা);
    }
#  endif

    যদি (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         অঙ্ক_Rets;

_       (GetLEB_u32(l, &bound_Args_গুনতি));
        cont->bound_Args_গুনতি = bound_Args_গুনতি;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_গুনতি > Get_Func_Type_অঙ্ক_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_অঙ্ক_Params(remaining) != Get_Func_Type_অঙ্ক_Params(inner) - bound_Args_গুনতি or
                                            Get_Func_Type_অঙ্ক_Results(remaining) != Get_Func_Type_অঙ্ক_Results(inner));
            জন্য (u16 i = 0; i < Get_Func_Type_অঙ্ক_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ফলাফল_Type(inner, i) != Get_Func_Type_ফলাফল_Type(remaining, i));
            }
            জন্য (u16 i = 0; i < Get_Func_Type_অঙ্ক_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_গুনতি + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        অঙ্ক_Rets = Get_Func_Type_অঙ্ক_Results(inner);

        _throwif(m3Err_wasmMalformed, (অঙ্ক_Rets + bound_Args_গুনতি) * c_io_Slot_গুনতি > cont->অঙ্ক_স্তূপ_Slots);

        জন্য (u32 i = 0; i < bound_Args_গুনতি; ++i) {
_           (Get_মান(l, GetFuncTypeParamType(inner, (u16)i), cont->val_স্তূপ + (অঙ্ক_Rets + i) * c_io_Slot_গুনতি));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_স্তূপ;
        cont->pc    = cont->entryFunction->compiled;
    } অন্যথা যদি (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_গুনতি));
        _throwif(m3Err_wasmMalformed, is_মূল and bound_Args_গুনতি != 0);
        cont->bound_Args_গুনতি = bound_Args_গুনতি;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } অন্যথা যদি (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_গুনতি));
        _throwif(m3Err_wasmMalformed, bound_Args_গুনতি != 0);
        cont->bound_Args_গুনতি = 0;
        cont->state          = cont_consumed;
    } অন্যথা {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    যদি (ফলাফল) {
        cont->অঙ্ক_Frames = 0;
    }

    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadContinuations (M3SnapshotLoad* l)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u32      গুনতি  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &গুনতি));
    _throwif(m3Err_wasmMalformed, গুনতি == 0 or গুনতি != l->অঙ্ক_Continuations);

    জন্য (u32 i = 0; i < গুনতি; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    ফেরত ফলাফল;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
স্থির
M3_ফলাফল CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    জন্য (u32 id = 0; id < l->অঙ্ক_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  যদি d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        যদি (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : ফাঁকা;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_অঙ্ক_Results(actual) != Get_Func_Type_অঙ্ক_Results(expected));
            জন্য (u16 i = 0; i < Get_Func_Type_অঙ্ক_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_ফলাফল_Type(actual, i) != Get_Func_Type_ফলাফল_Type(expected, i));
            }
        }
        যখন (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->অঙ্ক_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        যদি (cont->state != cont_suspended or id == 0) {
            চালু;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        যদি (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_গুনতি != 0);
            চালু;
        }

        ধ্রুবক M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_গুনতি > inner->অঙ্ক_Suspend_Results or
                                                                          Get_Func_Type_অঙ্ক_Params(remaining) != inner->অঙ্ক_Suspend_Results - cont->bound_Args_গুনতি);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_অঙ্ক_Results(remaining) != Get_Func_Type_অঙ্ক_Results(cont->entryFunction->funcType));
        জন্য (u16 i = 0; i < Get_Func_Type_অঙ্ক_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ফলাফল_Type(remaining, i) !=
                                            Get_Func_Type_ফলাফল_Type(cont->entryFunction->funcType, i));
        }

        জন্য (u32 i = 0; i < inner->অঙ্ক_Suspend_Results; ++i) {
            ধ্রুবক M3_Slot_মান* মান = &site->map->মানসমূহ[site->point->first_মান +
                                                          site->point->অঙ্ক_মানসমূহ - site->point->অঙ্ক_Results + i];
            যদি (i < cont->bound_Args_গুনতি) {
                যদি (IsRefType(মান->type) and not IsNullableRef(মান->type)) {
                    শূন্য* ref = ফাঁকা;
                    memcpy(&ref, inner->sp + মান->slot, আকার(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } অন্যথা {
                _throwif("a captured continuation has the wrong parameter type", মান->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_গুনতি)));
            }
        }
    }

_catch:
    ফেরত ফলাফল;
}

#  যদি d_m3HasExceptionHandling

স্থির
M3_ফলাফল LoadExceptions (M3SnapshotLoad* l)
{
    M3_ফলাফল ফলাফল        = m3Err_none;
    u32      অঙ্ক_Exceptions = 0;

_   (GetLEB_u32(l, &অঙ্ক_Exceptions));
    _throwif(m3Err_wasmMalformed, অঙ্ক_Exceptions == 0 or অঙ্ক_Exceptions != l->অঙ্ক_Exceptions);

    জন্য (u32 i = 0; i < অঙ্ক_Exceptions; ++i) {
        u32          exnId = 0, অঙ্ক_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &অঙ্ক_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, অঙ্ক_Args != exn->অঙ্ক_Args);

        জন্য (u32 a = 0; a < অঙ্ক_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_মান_Bits(l, argType, &bits));
            যদি (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    ফেরত ফলাফল;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
প্রকারলেখ কাঠামো M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3_ফলাফল        error;
} M3BoundedReader;

স্থির
M3_ফলাফল BoundedReader_Read (শূন্য* o_বাফার, size_t i_আকার, শূন্য* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    যদি (i_আকার > br->remaining) {
        br->error = "the host read past the end of its state";
        ফেরত br->error;
    }

    br->remaining -= i_আকার;

    ফেরত Get(br->load, o_বাফার, i_আকার);
}

স্থির
M3_ফলাফল LoadHostState (M3SnapshotLoad* l)
{
    M3_ফলাফল               ফলাফল = m3Err_none;
    ধ্রুবক M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 আকার_2 = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    যদি (l->checking) {
        ফেরত SkipBytes(l, আকার_2);
    }
    reader.load      = l;
    reader.remaining = আকার_2;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, আকার_2));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadHeader (M3SnapshotLoad* l)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u8       magic[আকার(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, আকার(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, আকার(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadMeta (M3SnapshotLoad* l, u32* o_flags)
{
    M3_ফলাফল ফলাফল      = m3Err_none;
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

    (শূন্য)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->অঙ্ক_Continuations));
_   (GetLEB_u32(l, &l->অঙ্ক_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->অঙ্ক_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->অঙ্ক_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    যদি (*o_flags & d_m3SnapshotFlagPostmortem) {
        জন্য (u32 i = 0; i < l->অঙ্ক_Exceptions; ++i) {
            u32 tag_সূচক = 0;
_           (GetLEB_u32(l, &tag_সূচক));
        }
        ফেরত m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->অঙ্ক_Continuations == 0 or activeStacks >= l->অঙ্ক_Continuations);
#  যদি !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->অঙ্ক_Exceptions != 0);
#  endif

    যদি (l->checking) {
        জন্য (u32 i = 0; i < l->অঙ্ক_Exceptions; ++i) {
            u32 tag_সূচক = 0;
_           (GetLEB_u32(l, &tag_সূচক));
#  যদি d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tag_সূচক >= l->module->অঙ্ক_Tags);
#  endif
        }
        ফেরত m3Err_none;
    }

    l->check_স্তূপ = m3_AllocArray(m3slot_t, d_m3_Continuation_স্তূপ_Slots + 4);
    _throwifnull(l->check_স্তূপ);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->অঙ্ক_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->অঙ্ক_Continuations);
    _throwifnull(l->suspendSites);

    যদি (not l->runtime->মূল_Continuation) {
        IM3Continuation মূল = Continuation_New(l->runtime, ফাঁকা, ফাঁকা);
        _throwifnull(মূল);

        // the root borrows the runtime's stack, so it holds none of its own
        মূল->অঙ্ক_স্তূপ_Slots          = l->runtime->অঙ্ক_স্তূপ_Slots;
        l->runtime->মূল_Continuation = মূল;
    }

    l->continuations[0] = l->runtime->মূল_Continuation;

    জন্য (u32 i = 1; i < l->অঙ্ক_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ফাঁকা, ফাঁকা);
        _throwifnull(l->continuations[i]);
    }

#  যদি d_m3HasExceptionHandling
    যদি (l->অঙ্ক_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->অঙ্ক_Exceptions);
        _throwifnull(l->exceptions);
    }

    জন্য (u32 i = 0; i < l->অঙ্ক_Exceptions; ++i) {
        u32    tag_সূচক = 0;
        IM3Tag tag;
        u32    অঙ্ক_Args;

_       (GetLEB_u32(l, &tag_সূচক));
        _throwif(m3Err_wasmMalformed, tag_সূচক >= l->module->অঙ্ক_Tags);

        tag     = Tag_Of_সূচক(l->module, tag_সূচক);
        অঙ্ক_Args = Get_Func_Type_অঙ্ক_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, অঙ্ক_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  অন্যথা
    জন্য (u32 i = 0; i < l->অঙ্ক_Exceptions; ++i) {
        u32 tag_সূচক = 0;
_       (GetLEB_u32(l, &tag_সূচক));
    }
#  endif

_catch:
    ফেরত ফলাফল;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
স্থির
M3_ফলাফল LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    u32      flags  = 0;
    নির্বাচন (i_sectionId) {
    ক্ষেত্র d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        বিরতি;

    ক্ষেত্র d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        বিরতি;

    ক্ষেত্র d_m3SnapshotSection_Table:
_       (LoadTables(l));
        বিরতি;

    ক্ষেত্র d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        বিরতি;

    ক্ষেত্র d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        বিরতি;

#  যদি d_m3HasExceptionHandling
    ক্ষেত্র d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        বিরতি;
#  endif

    ক্ষেত্র d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        বিরতি;

    ক্ষেত্র d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        বিরতি;

    পূর্বনির্ধারিত:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        বিরতি;
    }

_catch:
    ফেরত ফলাফল;
}

স্থির
M3_ফলাফল LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3_ফলাফল ফলাফল  = m3Err_none;
    u8*      payload = ফাঁকা;

    যদি (not i_embedded) {
_       (LoadHeader(l));
    }

    যখন (true) {
        u8  sectionId   = 0;
        u32 section_আকার = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        যদি (GetU8(l, &sectionId)) {
            বিরতি;
        }

_       (GetLEB_u32(l, &section_আকার));

        _throwif("the snapshot holds a section twice", l->seenSections[sectionId]);
        l->seenSections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_আকার == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_আকার);
        _throwifnull(payload);
_       (Get(l, payload, section_আকার));
        l->pos       = payload;
        l->end       = payload + section_আকার;
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
             l->অঙ্ক_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->অঙ্ক_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->অঙ্ক_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->অঙ্ক_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->অঙ্ক_তথ্য_Segments or l->module->অঙ্ক_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    ফেরত ফলাফল;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#যদি d_m3HasSnapshots

স্থির
M3_ফলাফল SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, শূন্য* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_ফলাফল       ফলাফল = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, আকার(save));

    IM3Continuation মূল = io_runtime->মূল_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not মূল or মূল->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : মূল->entryFunction;

    যদি (o_module) {
        *o_module = ফাঁকা;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    যদি (o_module) {
        *o_module = save.module;
    }

    জন্য (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->পরবর্তী) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  যদি d_m3HasExceptionHandling
    জন্য (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->পরবর্তী) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    যদি (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, মূল, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    ফেরত ফলাফল;
}

#endif

M3_ফলাফল m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, শূন্য* i_userdata)
{
    যদি (not io_runtime or not i_writer) {
        ফেরত m3Err_mallocFailed;
    }

#যদি d_m3HasSnapshots
    ফেরত SaveSnapshot(io_runtime, i_writer, i_userdata, ফাঁকা, false);
#অন্যথা
    (শূন্য)i_userdata;
    ফেরত "snapshots are not available in this build of Wasm3";
#endif
}


স্থির
M3_ফলাফল LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, শূন্য* i_userdata, bool i_embedded)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#যদি d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, আকার(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        ফলাফল = LoadSections(&load, i_embedded);

        IM3Continuation মূল = io_runtime->মূল_Continuation;

        যদি (ফলাফল) {
            যদি (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                যদি (মূল) {
                    মূল->state     = cont_returned;
                    মূল->অঙ্ক_Frames = 0;
                }
            }
        } অন্যথা {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_স্তূপ(io_runtime);

        m3_Free(load.check_স্তূপ);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  যদি d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#অন্যথা
    (শূন্য)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    ফেরত ফলাফল;
}


M3_ফলাফল m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, শূন্য* i_userdata)
{
    ফেরত LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


শূন্য m3_SetSnapshotHooks (IM3Runtime io_runtime, ধ্রুবক M3SnapshotHooks* i_hooks)
{
#যদি d_m3HasSnapshots
    যদি (io_runtime) {
        যদি (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } অন্যথা {
            memset(&io_runtime->snapshotHooks, 0, আকার(io_runtime->snapshotHooks));
        }
    }
#অন্যথা
    (শূন্য)io_runtime;
    (শূন্য)i_hooks;
#endif
}


M3_ফলাফল m3_Save_Snapshot_To_বাফার (IM3Runtime io_runtime, শূন্য** o_bytes, size_t* o_আকার)
{
    যদি (!io_runtime || !o_bytes || !o_আকার) {
        ফেরত m3Err_mallocFailed;
    }
#যদি d_m3HasSnapshots
    M3_বাফার_Writer bw     = { ফাঁকা, 0, 0 };
    M3_ফলাফল       ফলাফল = m3_SaveSnapshot(io_runtime, বাফার_Writer_Write, &bw);
    যদি (ফলাফল) {
        m3_Free(bw.বাফার);
        *o_bytes = ফাঁকা;
        *o_আকার  = 0;
        ফেরত ফলাফল;
    }
    *o_bytes = bw.বাফার;
    *o_আকার  = bw.আকার_2;
    ফেরত m3Err_none;
#অন্যথা
    *o_bytes = ফাঁকা;
    *o_আকার  = 0;
    ফেরত "snapshots are not available in this build of Wasm3";
#endif
}

#যদি d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
স্থির
u8* EmitLEB_u32 (u8* o_where, u32 i_মান)
{
    করো {
        u8 byte = i_মান & 0x7F;
        i_মান >>= 7;
        যদি (i_মান != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } যখন (i_মান != 0);

    ফেরত o_where;
}

স্থির
u32 আকার_Of_LEB_u32 (u32 i_মান)
{
    u32 আকার_2 = 0;

    করো {
        i_মান >>= 7;
        ++আকার_2;
    } যখন (i_মান != 0);

    ফেরত আকার_2;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
স্থির
M3_ফলাফল FindEmbeddedSnapshot (ধ্রুবক u8* i_start, ধ্রুবক u8* i_end, cstr_t i_secName,
                               ধ্রুবক u8** o_secStart, ধ্রুবক u8** o_secEnd,
                               ধ্রুবক u8** o_তথ্য, u32* o_আকার)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = ফাঁকা;

    যদি (o_secStart) {
        *o_secStart = ফাঁকা;
    }
    যদি (o_secEnd) {
        *o_secEnd = ফাঁকা;
    }
    যদি (o_তথ্য) {
        *o_তথ্য = ফাঁকা;
    }
    যদি (o_আকার) {
        *o_আকার = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    যখন (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     আকার_2      = 0;
_       (ReadLEB_u32(&আকার_2, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, আকার_2 > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += আকার_2;
            যদি (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                যদি (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    যদি (o_secStart) {
                        *o_secStart = secStart;
                    }
                    যদি (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    যদি (o_তথ্য) {
                        *o_তথ্য = payload;
                    }
                    যদি (o_আকার) {
                        *o_আকার = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    ফেরত ফলাফল;
}

// Builds the custom section name a snapshot of i_name lives under
স্থির
M3_ফলাফল SnapshotSectionName (cstr_t i_name, অক্ষর* o_secName, size_t i_আকার)
{
    যদি (i_name and *i_name) {
        যদি (strlen(i_name) > d_m3_Max_Snapshot_Name_দৈর্ঘ্য) {
            ফেরত "the snapshot name is too long";
        }
        snprintf(o_secName, i_আকার, "snapshot.%s", i_name);
    } অন্যথা {
        snprintf(o_secName, i_আকার, "snapshot");
    }

    ফেরত m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
স্থির
M3_ফলাফল ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_ফলাফল ফলাফল = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ফাঁকা;

    জন্য (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->পরবর্তী) {
        যদি (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    ফেরত ফলাফল;
}
#endif

M3_ফলাফল m3_Load_Snapshot_From_বাফার (IM3Runtime io_runtime, IM3Module i_module, ধ্রুবক শূন্য* i_bytes, size_t i_আকার)
{
    যদি (!io_runtime || !i_module || !i_bytes) {
        ফেরত m3Err_mallocFailed;
    }
#যদি d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    যদি (i_আকার >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        ধ্রুবক u8* তথ্য   = ফাঁকা;
        u32       আকার_2   = 0;
        M3_ফলাফল  ফলাফল = FindEmbeddedSnapshot((ধ্রুবক u8*)i_bytes, (ধ্রুবক u8*)i_bytes + i_আকার,
                                                "snapshot", ফাঁকা, ফাঁকা, &তথ্য, &আকার_2);
        যদি (ফলাফল) {
            ফেরত ফলাফল;
        }
        যদি (not তথ্য) {
            ফেরত "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_বাফার_Reader br = { তথ্য, আকার_2, 0 };
            ফেরত LoadSnapshot(io_runtime, i_module, বাফার_Reader_Read, &br, true);
        }
    }
    {
        M3_বাফার_Reader br = { (ধ্রুবক u8*)i_bytes, i_আকার, 0 };
        ফেরত LoadSnapshot(io_runtime, i_module, বাফার_Reader_Read, &br,
                            i_আকার < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#অন্যথা
    (শূন্য)i_আকার;
    ফেরত "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, ধ্রুবক অক্ষর* i_name)
{
    যদি (not i_module) {
        ফেরত false;
    }
#যদি d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ফাঁকা;
    (শূন্য)ModuleSnapshot(i_module, i_name, &snap);

    ফেরত snap != ফাঁকা;
#অন্যথা
    (শূন্য)i_name;
    ফেরত false;
#endif
}

M3_ফলাফল m3_GetEmbeddedSnapshot (IM3Module i_module, ধ্রুবক অক্ষর* i_name,
                                 ধ্রুবক শূন্য** o_bytes, size_t* o_আকার)
{
    যদি (not i_module or not o_bytes or not o_আকার) {
        ফেরত m3Err_mallocFailed;
    }

    *o_bytes = ফাঁকা;
    *o_আকার  = 0;

#যদি d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ফাঁকা;
        M3_ফলাফল            ফলাফল = ModuleSnapshot(i_module, i_name, &snap);

        যদি (ফলাফল) {
            ফেরত ফলাফল;
        }
        যদি (not snap) {
            ফেরত "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->তথ্য;
        *o_আকার  = snap->আকার_2;

        ফেরত m3Err_none;
    }
#অন্যথা
    (শূন্য)i_name;
    ফেরত "snapshots are not available in this build of Wasm3";
#endif
}

M3_ফলাফল m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, ধ্রুবক অক্ষর* i_name)
{
    যদি (not io_runtime or not i_module) {
        ফেরত m3Err_mallocFailed;
    }
#যদি d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ফাঁকা;
        M3_ফলাফল            ফলাফল = ModuleSnapshot(i_module, i_name, &snap);
        M3_বাফার_Reader      br;

        যদি (ফলাফল) {
            ফেরত ফলাফল;
        }
        যদি (not snap) {
            ফেরত "the module holds no embedded snapshot of that name";
        }

        br.বাফার = snap->তথ্য;
        br.আকার_2   = snap->আকার_2;
        br.cursor = 0;

        ফেরত LoadSnapshot(io_runtime, i_module, বাফার_Reader_Read, &br, true);
    }
#অন্যথা
    (শূন্য)i_name;
    ফেরত "snapshots are not available in this build of Wasm3";
#endif
}


M3_ফলাফল m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, ধ্রুবক অক্ষর* i_name,
                                  শূন্য** o_bytes, size_t* o_আকার)
{
#যদি d_m3HasSnapshots
    M3_ফলাফল       ফলাফল = m3Err_none;
    M3_বাফার_Writer bw     = { ফাঁকা, 0, 0 };
    u8*            out    = ফাঁকা;

    অক্ষর      secName[d_m3_Max_Snapshot_Name_দৈর্ঘ্য + আকার("snapshot.")];
    u32       nameLen, name_Len_Leb_আকার, payload_আকার, payload_আকার_Leb_আকার;
    size_t    orig_আকার, যোগ_আকার;
    ধ্রুবক u8* dropStart = ফাঁকা;
    ধ্রুবক u8* dropEnd   = ফাঁকা;
    IM3Module saved     = ফাঁকা;
    u8*       p;

    যদি (not io_runtime or not i_module or not o_bytes or not o_আকার) {
        ফেরত m3Err_mallocFailed;
    }

    *o_bytes = ফাঁকা;
    *o_আকার  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, আকার(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->মূল_Continuation or io_runtime->মূল_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, বাফার_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.আকার_2);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.আকার_2 > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, ফাঁকা, ফাঁকা));

    orig_আকার           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    name_Len_Leb_আকার     = আকার_Of_LEB_u32(nameLen);
    payload_আকার        = name_Len_Leb_আকার + nameLen + (u32)bw.আকার_2;
    payload_আকার_Leb_আকার = আকার_Of_LEB_u32(payload_আকার);
    যোগ_আকার          = orig_আকার + 1 + payload_আকার_Leb_আকার + payload_আকার;

    out = (u8*)m3_Malloc("wasm_snapshot_module", যোগ_আকার);
    _throwifnull(out);

    p = out;

    যদি (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } অন্যথা {
        memcpy(p, i_module->wasmStart, orig_আকার);
        p += orig_আকার;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_আকার);
    p    = EmitLEB_u32(p, nameLen);
    memcpy(p, secName, nameLen);
    p += nameLen;
    memcpy(p, bw.বাফার, bw.আকার_2);

    *o_bytes = out;
    *o_আকার  = যোগ_আকার;
    out      = ফাঁকা;

_catch:
    m3_Free(bw.বাফার);
    m3_Free(out);

    ফেরত ফলাফল;
#অন্যথা
    (শূন্য)io_runtime;
    (শূন্য)i_module;
    (শূন্য)i_name;
    (শূন্য)o_bytes;
    (শূন্য)o_আকার;
    ফেরত "snapshots are not available in this build of Wasm3";
#endif
}
