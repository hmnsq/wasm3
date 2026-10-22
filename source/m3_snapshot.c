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

#ଅନ୍ତର୍ଭାଵ "m3_env.h"
#ଅନ୍ତର୍ଭାଵ "m3_compile.h"
#ଅନ୍ତର୍ଭାଵ "m3_exception.h"
#ଅନ୍ତର୍ଭାଵ "m3_host.h"

#ଅନ୍ତର୍ଭାଵ <limits.h>

#ଯଦି d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
ସ୍ଥିର ଅଚଳ u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
ସ୍ଥିର ଅଚଳ u32 c_snapshotVersion  = 1;

ଗଣନା {
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
ଗଣନା {
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
ସ୍ଥିର
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

        ଯେତେବେଳେ (pos < end) {
            bytes_t start     = pos;
            u8      sectionId = *pos++;
            u32     size      = 0;

_           (ReadLEB_u32(&size, &pos, end));
            _throwif(m3Err_wasmUnderrun, size > (size_t)(end - pos));

            pos += size;
            ଯଦି (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    ଫେରନ୍ତୁ result;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ସ୍ଥିର
ଅଚଳ M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_kind)
{
    ଅଚଳ M3SnapshotMap* map = i_function->snapshotMap;

    ଯଦି (map) {
        ପାଇଁ (u32 i = 0; i < map->ଅଙ୍କ_Safe_Points; ++i) {
            ଯଦି (map->safePoints[i].pc == i_pc and map->safePoints[i].kind == i_kind) {
                ଫେରନ୍ତୁ &map->safePoints[i];
            }
        }
    }

    ଫେରନ୍ତୁ ଖାଲି;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
ସ୍ଥିର
ଅଚଳ M3SafePoint* FindSafePointAt (ଅଚଳ M3SnapshotMap* i_map, u32 i_wasmOffset, u8 i_kind, u32 i_targetLoop)
{
    u32 ତଳ  = 0;
    u32 ଉପର = i_map->ଅଙ୍କ_Safe_Points;

    ଯେତେବେଳେ (ତଳ < ଉପର) {
        u32 ମଧ୍ୟ = ତଳ + (ଉପର - ତଳ) / 2;

        ଯଦି (i_map->safePoints[ମଧ୍ୟ].wasmOffset < i_wasmOffset) {
            ତଳ = ମଧ୍ୟ + 1;
        } ଅନ୍ୟଥା {
            ଉପର = ମଧ୍ୟ;
        }
    }

    ପାଇଁ (u32 i = ତଳ; i < i_map->ଅଙ୍କ_Safe_Points and i_map->safePoints[i].wasmOffset == i_wasmOffset; ++i) {
        ଅଚଳ M3SafePoint* point = &i_map->safePoints[i];

        ଯଦି (point->kind != i_kind or (point->flags & d_m3SafePointTailCall)) {
            ଜାରି;
        }
        ଯଦି (i_kind != safepoint_op or i_map->blocks[point->aux].wasmOffset == i_targetLoop) {
            ଫେରନ୍ତୁ point;
        }
    }

    ଫେରନ୍ତୁ ଖାଲି;
}

ସ୍ଥିର
ଅଚଳ M3BlockStart* FindBlockAt (ଅଚଳ M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasmOffset)
{
    ପାଇଁ (u32 i = 0; i < i_map->ଅଙ୍କ_Blocks; ++i) {
        ଯଦି (i_map->blocks[i].opcode == i_opcode and i_map->blocks[i].wasmOffset == i_wasmOffset) {
            ଫେରନ୍ତୁ &i_map->blocks[i];
        }
    }

    ଫେରନ୍ତୁ ଖାଲି;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ସ୍ଥିର ଧାଡ଼ିଭିତରେ
bool EnclosesOffset (ଅଚଳ M3BlockStart* i_block, u32 i_wasmOffset)
{
    ଫେରନ୍ତୁ i_block->wasmOffset < i_wasmOffset and i_wasmOffset < i_block->wasmEnd;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ସ୍ଥିର ଧାଡ଼ିଭିତରେ
bool HandlersLiveAt (ଅଚଳ M3SnapshotMap* i_map, ଅଚଳ M3SafePoint* i_point, ଅଚଳ M3BlockStart* i_try)
{
    ଫେରନ୍ତୁ not (i_point->kind == safepoint_op and i_try->wasmOffset > i_map->blocks[i_point->aux].wasmOffset);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ସ୍ଥିର
M3Result CallSiteType (IM3Function i_caller, ଅଚଳ M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3Result   result = m3Err_none;
    IM3Module  module = i_caller->module;
    bytes_t    pos    = i_caller->wasm;
    cbytes_t   end    = i_caller->wasmEnd;
    u32        size   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = ଖାଲି;

_   (ReadLEB_u32(&size, &pos, end));
    _throwif(m3Err_wasmUnderrun, i_point->wasmOffset >= (size_t)(end - pos));
    pos += i_point->wasmOffset;

_   (Read_opcode(&opcode, &pos, end));
_   (ReadLEB_u32(&index, &pos, end));

    ଯଦି (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->ଅଙ୍କ_Functions);
        *o_type = module->functions[index].funcType;
    } ଅନ୍ୟଥା {
        _throwif(m3Err_wasmMalformed, opcode != c_waOp_callIndirect and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->ଅଙ୍କ_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ସ୍ଥିର
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 ଅଙ୍କ_Results = Get_Func_Type_ଅଙ୍କ_Results(i_expected);

    ଯଦି (not i_type or Get_Func_Type_ଅଙ୍କ_Results(i_type) != ଅଙ୍କ_Results) {
        ଫେରନ୍ତୁ false;
    }

    ପାଇଁ (u16 i = 0; i < ଅଙ୍କ_Results; ++i) {
        ଯଦି (not IsSubTypeOf(GetFuncTypeResultType(i_type, i), GetFuncTypeResultType(i_expected, i))) {
            ଫେରନ୍ତୁ false;
        }
    }

    ଫେରନ୍ତୁ true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3FrameRegisters {
    m3reg_t* r0;
#  ଯଦି d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
ସ୍ଥିର
M3Result Continuation_ଗଦା (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_base, u32* o_ଅଙ୍କ_Slots)
{
    ଯଦି (i_cont == i_runtime->ମୂଳ_Continuation) {
        *o_base     = (m3slot_t*)i_runtime->origin_ଗଦା;
        *o_ଅଙ୍କ_Slots = i_runtime->ଅଙ୍କ_ଗଦା_Slots;
    } ଅନ୍ୟଥା {
        *o_base     = i_cont->val_ଗଦା;
        *o_ଅଙ୍କ_Slots = i_cont->ଅଙ୍କ_ଗଦା_Slots;
    }

    ଫେରନ୍ତୁ (*o_base) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ସ୍ଥିର
M3Result Locate_ମୂଲ୍ୟ (m3slot_t* i_base, u32 i_ଅଙ୍କ_Slots, m3stack_t i_sp, ଅଚଳ M3_Slot_ମୂଲ୍ୟ* i_ମୂଲ୍ୟ, u8** o_where)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_ମୂଲ୍ୟ->type);

    *o_where = ଖାଲି;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and IsRegisterSlotAlias(i_ମୂଲ୍ୟ->slot));

    ଯଦି (not IsRegisterSlotAlias(i_ମୂଲ୍ୟ->slot)) {
        size_t size   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? ଆକାର(ଶୂନ୍ୟ*)
                                                                   : (Is64BitType(type) ? ଆକାର(u64) : ଆକାର(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_base + i_ମୂଲ୍ୟ->slot) * ଆକାର(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + size > (size_t)i_ଅଙ୍କ_Slots * ଆକାର(m3slot_t));

        *o_where = (u8*)i_base + offset;
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ସ୍ଥିର
M3Result Read_ମୂଲ୍ୟ (u8* i_where, ଅଚଳ M3FrameRegisters* i_registers, ଅଚଳ M3_Slot_ମୂଲ୍ୟ* i_ମୂଲ୍ୟ, u64* o_bits,
                    ଶୂନ୍ୟ** o_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_ମୂଲ୍ୟ->type);

    *o_bits      = 0;
    *o_reference = ଖାଲି;

    ଯଦି (i_where) {
        ଯଦି (IsRefType(type)) {
            memcpy(o_reference, i_where, ଆକାର(ଶୂନ୍ୟ*));
        } ଅନ୍ୟଥା ଯଦି (Is64BitType(type)) {
            memcpy(o_bits, i_where, ଆକାର(u64));
        } ଅନ୍ୟଥା {
            u32 narrow;
            memcpy(&narrow, i_where, ଆକାର(narrow));
            *o_bits = narrow;
        }
    } ଅନ୍ୟଥା ଯଦି (IsFpRegisterSlotAlias(i_ମୂଲ୍ୟ->slot)) {
#  ଯଦି d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ଯଦି (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 bits;
            memcpy(&bits, &narrow, ଆକାର(bits));
            *o_bits = bits;
        } ଅନ୍ୟଥା {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(o_bits, i_registers->fp0, ଆକାର(u64));
        }
#  ଅନ୍ୟଥା
        _throw(m3Err_wasmMalformed);
#  endif
    } ଅନ୍ୟଥା {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        ଯଦି (IsRefType(type)) {
            *o_reference = (ଶୂନ୍ୟ*)(uintptr_t)r0;
        } ଅନ୍ୟଥା ଯଦି (Is64BitType(type)) {
            *o_bits = (u64)r0;
        } ଅନ୍ୟଥା {
            *o_bits = (u32)r0;
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result Write_ମୂଲ୍ୟ (u8* o_where, ଅଚଳ M3FrameRegisters* i_registers, ଅଚଳ M3_Slot_ମୂଲ୍ୟ* i_ମୂଲ୍ୟ, u64 i_bits,
                     ଶୂନ୍ୟ* i_reference)
{
    M3Result result = m3Err_none;
    u8       type   = BaseTypeOf(i_ମୂଲ୍ୟ->type);

    ଯଦି (o_where) {
        ଯଦି (IsRefType(type)) {
            memcpy(o_where, &i_reference, ଆକାର(ଶୂନ୍ୟ*));
        } ଅନ୍ୟଥା ଯଦି (Is64BitType(type)) {
            memcpy(o_where, &i_bits, ଆକାର(u64));
        } ଅନ୍ୟଥା {
            u32 narrow = (u32)i_bits;
            memcpy(o_where, &narrow, ଆକାର(narrow));
        }
    } ଅନ୍ୟଥା ଯଦି (IsFpRegisterSlotAlias(i_ମୂଲ୍ୟ->slot)) {
#  ଯଦି d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        ଯଦି (type == c_m3Type_f32) {
            u32 bits = (u32)i_bits;
            f32 narrow;
            memcpy(&narrow, &bits, ଆକାର(narrow));
            *i_registers->fp0 = narrow;
        } ଅନ୍ୟଥା {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            memcpy(i_registers->fp0, &i_bits, ଆକାର(u64));
        }
#  ଅନ୍ୟଥା
        _throw(m3Err_wasmMalformed);
#  endif
    } ଅନ୍ୟଥା {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        ଯଦି (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } ଅନ୍ୟଥା {
            *i_registers->r0 = (m3reg_t)i_bits;
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ସ୍ଥିର
ଅଚଳ M3_Slot_ମୂଲ୍ୟ* Frame_ମୂଲ୍ୟ (ଅଚଳ M3SnapshotMap* i_map, ଅଚଳ M3SafePoint* i_point, u32 i_index)
{
    ଯଦି (i_index < i_map->ଅଙ୍କ_Locals) {
        ଫେରନ୍ତୁ i_point->first_Local_ମୂଲ୍ୟ == UINT32_MAX ? &i_map->locals[i_index]
                                                      : &i_map->ମୂଲ୍ୟଗୁଡ଼ିକ[i_point->first_Local_ମୂଲ୍ୟ + i_index];
    }
    ଫେରନ୍ତୁ &i_map->ମୂଲ୍ୟଗୁଡ଼ିକ[i_point->first_ମୂଲ୍ୟ + i_index - i_map->ଅଙ୍କ_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ସ୍ଥିର
ଶୂନ୍ୟ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    ଯଦି (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->ଅଙ୍କ_Ret_And_Arg_Slots) + i_function->ଅଙ୍କ_Local_Bytes;

        memcpy(where, i_function->constants, i_function->ଅଙ୍କ_Constant_Bytes);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3PointerIds {
    ଶୂନ୍ୟ** ବସ୍ତୁଗୁଡ଼ିକ;
    u32    ଗଣନା_2;
    u32    capacity;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  indexCapacity;       // a power of two, kept at least twice count
} M3PointerIds;

ସ୍ଥିର
u32 PointerBucket (ଶୂନ୍ୟ* i_pointer, u32 i_capacity)
{
    u64 bits = (u64)(uintptr_t)i_pointer;

    ଫେରନ୍ତୁ (u32)((bits * 0x9E3779B97F4A7C15ULL) >> 32) & (i_capacity - 1);
}

ସ୍ଥିର
u32 PointerIds_Find (ଅଚଳ M3PointerIds* i_ids, ଶୂନ୍ୟ* i_pointer)
{
    ଯଦି (i_ids->indexCapacity) {
        u32 bucket = PointerBucket(i_pointer, i_ids->indexCapacity);

        ଯେତେବେଳେ (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            ଯଦି (i_ids->ବସ୍ତୁଗୁଡ଼ିକ[id] == i_pointer) {
                ଫେରନ୍ତୁ id;
            }

            bucket = (bucket + 1) & (i_ids->indexCapacity - 1);
        }
    }

    ଫେରନ୍ତୁ d_m3SnapshotNone;
}

ସ୍ଥିର
M3Result PointerIds_Add (M3PointerIds* io_ids, ଶୂନ୍ୟ* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    ଯଦି (id != d_m3SnapshotNone) {
        *o_id = id;
        ଫେରନ୍ତୁ m3Err_none;
    }

    ଯଦି (io_ids->ଗଣନା_2 == io_ids->capacity) {
        u32    capacity = io_ids->capacity ? io_ids->capacity * 2 : 16;
        ଶୂନ୍ୟ** ବସ୍ତୁଗୁଡ଼ିକ    = m3_ReallocArray(ଶୂନ୍ୟ*, io_ids->ବସ୍ତୁଗୁଡ଼ିକ, capacity, io_ids->capacity);

        ଯଦି (not ବସ୍ତୁଗୁଡ଼ିକ) {
            ଫେରନ୍ତୁ m3Err_mallocFailed;
        }

        io_ids->ବସ୍ତୁଗୁଡ଼ିକ    = ବସ୍ତୁଗୁଡ଼ିକ;
        io_ids->capacity = capacity;
    }

    ଯଦି ((io_ids->ଗଣନା_2 + 1) * 2 > io_ids->indexCapacity) {
        u32  capacity = io_ids->indexCapacity ? io_ids->indexCapacity * 2 : 32;
        u32* index    = m3_AllocArray(u32, capacity);

        ଯଦି (not index) {
            ଫେରନ୍ତୁ m3Err_mallocFailed;
        }

        ପାଇଁ (u32 i = 0; i < io_ids->ଗଣନା_2; ++i) {
            u32 bucket = PointerBucket(io_ids->ବସ୍ତୁଗୁଡ଼ିକ[i], capacity);

            ଯେତେବେଳେ (index[bucket]) {
                bucket = (bucket + 1) & (capacity - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->indexCapacity = capacity;
    }

    id                = io_ids->ଗଣନା_2++;
    io_ids->ବସ୍ତୁଗୁଡ଼ିକ[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->indexCapacity);

    ଯେତେବେଳେ (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->indexCapacity - 1);
    }

    io_ids->index[bucket] = id + 1;

    *o_id = id;

    ଫେରନ୍ତୁ m3Err_none;
}

ସ୍ଥିର
ଶୂନ୍ୟ PointerIds_Free (M3PointerIds* io_ids)
{
    m3_Free(io_ids->ବସ୍ତୁଗୁଡ଼ିକ);
    m3_Free(io_ids->index);
    memset(io_ids, 0, ଆକାର(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3BufferWriter {
    u8*    buffer;
    size_t size;
    size_t capacity;
} M3BufferWriter;

ସ୍ଥିର
M3Result BufferWriter_Write (ଅଚଳ ଶୂନ୍ୟ* i_data, size_t i_size, ଶୂନ୍ୟ* i_userdata)
{
    M3BufferWriter* bw = (M3BufferWriter*)i_userdata;

    ଯଦି (bw->size + i_size > bw->capacity) {
        size_t newCap = bw->capacity ? bw->capacity * 2 : 4096;
        ଯେତେବେଳେ (bw->size + i_size > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->buffer, newCap, bw->capacity);
        ଯଦି (!newBuf) {
            ଫେରନ୍ତୁ m3Err_mallocFailed;
        }
        bw->buffer   = newBuf;
        bw->capacity = newCap;
    }

    ଯଦି (i_size) {
        memcpy(bw->buffer + bw->size, i_data, i_size);
        bw->size += i_size;
    }

    ଫେରନ୍ତୁ m3Err_none;
}

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3BufferReader {
    ଅଚଳ u8* buffer;
    size_t    size;
    size_t    cursor;
} M3BufferReader;

ସ୍ଥିର
M3Result BufferReader_Read (ଶୂନ୍ୟ* o_buffer, size_t i_size, ଶୂନ୍ୟ* i_userdata)
{
    M3BufferReader* br = (M3BufferReader*)i_userdata;

    ଯଦି (i_size > br->size - br->cursor) {
        ଫେରନ୍ତୁ m3Err_wasmMalformed;
    }

    ଯଦି (i_size) {
        memcpy(o_buffer, br->buffer + br->cursor, i_size);
        br->cursor += i_size;
    }

    ଫେରନ୍ତୁ m3Err_none;
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
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    ଶୂନ୍ୟ*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3PointerIds liveContinuations;     // everything the runtime still holds, to check
    M3PointerIds liveExceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
u8 M3TypeToValType (m3type_t i_type)
{
    ବାଛ (BaseTypeOf(i_type)) {
    ସ୍ଥିତି c_m3Type_i32: ଫେରନ୍ତୁ 0x7F;
    ସ୍ଥିତି c_m3Type_i64: ଫେରନ୍ତୁ 0x7E;
    ସ୍ଥିତି c_m3Type_f32: ଫେରନ୍ତୁ 0x7D;
    ସ୍ଥିତି c_m3Type_f64: ଫେରନ୍ତୁ 0x7C;
    ସ୍ଥିତି c_m3Type_v128: ଫେରନ୍ତୁ 0x7B;
    ସ୍ଥିତି c_m3Type_funcref: ଫେରନ୍ତୁ 0x70;
    ସ୍ଥିତି c_m3Type_externref: ଫେରନ୍ତୁ 0x6F;
    ସ୍ଥିତି c_m3Type_exnref: ଫେରନ୍ତୁ 0x69;
    ସ୍ଥିତି c_m3Type_contref: ଫେରନ୍ତୁ 0x68;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ 0x00;
    }
}

ସ୍ଥିର ଧାଡ଼ିଭିତରେ
m3type_t ValTypeToM3Type (u8 i_valtype)
{
    ବାଛ (i_valtype) {
    ସ୍ଥିତି 0x7F: ଫେରନ୍ତୁ c_m3Type_i32;
    ସ୍ଥିତି 0x7E: ଫେରନ୍ତୁ c_m3Type_i64;
    ସ୍ଥିତି 0x7D: ଫେରନ୍ତୁ c_m3Type_f32;
    ସ୍ଥିତି 0x7C: ଫେରନ୍ତୁ c_m3Type_f64;
    ସ୍ଥିତି 0x7B: ଫେରନ୍ତୁ c_m3Type_v128;
    ସ୍ଥିତି 0x70: ଫେରନ୍ତୁ c_m3Type_funcref;
    ସ୍ଥିତି 0x6F: ଫେରନ୍ତୁ c_m3Type_externref;
    ସ୍ଥିତି 0x69: ଫେରନ୍ତୁ c_m3Type_exnref;
    ସ୍ଥିତି 0x68: ଫେରନ୍ତୁ c_m3Type_contref;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ c_m3Type_none;
    }
}

ସ୍ଥିର
M3Result Put (M3SnapshotSave* s, ଅଚଳ ଶୂନ୍ୟ* i_data, size_t i_size)
{
    ଫେରନ୍ତୁ s->writing ? s->writer(i_data, i_size, s->userdata) : m3Err_none;
}

ସ୍ଥିର
M3Result PutU8 (M3SnapshotSave* s, u8 i_ମୂଲ୍ୟ)
{
    ଫେରନ୍ତୁ Put(s, &i_ମୂଲ୍ୟ, ଆକାର(i_ମୂଲ୍ୟ));
}


ସ୍ଥିର
M3Result PutLEB_u32 (M3SnapshotSave* s, u32 i_ମୂଲ୍ୟ)
{
    u8  bytes[5];
    u32 len = 0;

    କର {
        u8 byte = i_ମୂଲ୍ୟ & 0x7F;
        i_ମୂଲ୍ୟ >>= 7;
        ଯଦି (i_ମୂଲ୍ୟ != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } ଯେତେବେଳେ (i_ମୂଲ୍ୟ != 0);

    ଫେରନ୍ତୁ Put(s, bytes, len);
}

ସ୍ଥିର
M3Result PutLEB_u64 (M3SnapshotSave* s, u64 i_ମୂଲ୍ୟ)
{
    u8  bytes[10];
    u32 len = 0;

    କର {
        u8 byte = i_ମୂଲ୍ୟ & 0x7F;
        i_ମୂଲ୍ୟ >>= 7;
        ଯଦି (i_ମୂଲ୍ୟ != 0) {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    } ଯେତେବେଳେ (i_ମୂଲ୍ୟ != 0);

    ଫେରନ୍ତୁ Put(s, bytes, len);
}

ସ୍ଥିର
M3Result PutLEB_i64 (M3SnapshotSave* s, i64 i_ମୂଲ୍ୟ)
{
    u8   bytes[10];
    u32  len  = 0;
    bool more = true;

    ଯେତେବେଳେ (more) {
        u8 byte = i_ମୂଲ୍ୟ & 0x7F;
        i_ମୂଲ୍ୟ >>= 7;
        ଯଦି ((i_ମୂଲ୍ୟ == 0 && !(byte & 0x40)) || (i_ମୂଲ୍ୟ == -1 && (byte & 0x40))) {
            more = false;
        } ଅନ୍ୟଥା {
            byte |= 0x80;
        }
        bytes[len++] = byte;
    }

    ଫେରନ୍ତୁ Put(s, bytes, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
ପ୍ରକାରବ୍ୟାଖ୍ୟା M3Result (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ସ୍ଥିର
M3Result SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3Result         result   = m3Err_none;
    M3BufferWriter   bw       = { ଖାଲି, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    ଶୂନ୍ୟ*            userdata = s->userdata;

    s->writer   = BufferWriter_Write;
    s->userdata = &bw;

    result = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    ଯଦି (result) {
        ଯାଅ _catch;
    }

    // a section with nothing in it is left out
    ଯଦି (bw.size) {
        _throwif("snapshot section is too large", bw.size > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.size));
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
u32 FunctionIndex (IM3Module i_module, IM3Function i_function)
{
    ଯଦି (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->ଅଙ୍କ_Functions) {
        ଫେରନ୍ତୁ (u32)(i_function - i_module->functions);
    }

    ଫେରନ୍ତୁ d_m3SnapshotNone;
}

ସ୍ଥିର
u32 FuncTypeIndex (IM3Module i_module, IM3FuncType i_type)
{
    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Func_Types; ++i) {
        ଯଦି (i_module->funcTypes[i] == i_type) {
            ଫେରନ୍ତୁ i;
        }
    }

    ଫେରନ୍ତୁ d_m3SnapshotNone;
}

#  ଯଦି d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
ସ୍ଥିର
IM3Tag TagOfIndex (IM3Module i_module, u32 i_index)
{
    IM3Tag tag = &i_module->tags[i_index];

    ଫେରନ୍ତୁ tag->resolved ? tag->resolved : tag;
}

ସ୍ଥିର
u32 TagIndex (IM3Module i_module, IM3Tag i_tag)
{
    ପାଇଁ (u32 i = 0; i < i_module->ଅଙ୍କ_Tags; ++i) {
        ଯଦି (TagOfIndex(i_module, i) == i_tag) {
            ଫେରନ୍ତୁ i;
        }
    }

    ଫେରନ୍ତୁ d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ସ୍ଥିର
M3Result NameReference (M3SnapshotSave* s, u8 i_type, ଶୂନ୍ୟ* i_reference, u64* o_word)
{
    M3Result result = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    ଯଦି (not i_reference) {
        ଫେରନ୍ତୁ m3Err_none;
    }

    ଯଦି (i_type == c_m3Type_funcref) {
        id = FunctionIndex(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } ଅନ୍ୟଥା ଯଦି (i_type == c_m3Type_externref) {
        ଅଚଳ M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        ଯାଅ _catch;
    }
#  ଯଦି d_m3HasExceptionHandling
    ଅନ୍ୟଥା ଯଦି (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->liveExceptions, i_reference) == d_m3SnapshotNone);

        u32 ଗଣନା_2 = s->exceptions.ଗଣନା_2;
_       (PointerIds_Add(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.ଗଣନା_2 != ଗଣନା_2);
    }
#  endif
    ଅନ୍ୟଥା ଯଦି (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->liveContinuations, i_reference) == d_m3SnapshotNone);

        u32 ଗଣନା_2 = s->continuations.ଗଣନା_2;
_       (PointerIds_Add(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.ଗଣନା_2 != ଗଣନା_2);
    } ଅନ୍ୟଥା {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    ଫେରନ୍ତୁ result;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ସ୍ଥିର
M3Result Put_Ref_ମୂଲ୍ୟ (M3SnapshotSave* s, u8 i_type, ଶୂନ୍ୟ* i_reference)
{
    M3Result result = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    ଯଦି (not s->writing) {
        ଫେରନ୍ତୁ m3Err_none;
    }

    ଯଦି (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } ଅନ୍ୟଥା {
        u8 kind = 0;
        ଯଦି (i_type == c_m3Type_funcref) {
            kind = 0x01;
        } ଅନ୍ୟଥା ଯଦି (i_type == c_m3Type_externref) {
            kind = 0x02;
        }
#  ଯଦି d_m3HasExceptionHandling
        ଅନ୍ୟଥା ଯଦି (i_type == c_m3Type_exnref) {
            kind = 0x03;
        }
#  endif
        ଅନ୍ୟଥା ଯଦି (i_type == c_m3Type_contref) {
            kind = 0x04;
        }

_       (PutU8(s, kind));
_       (PutLEB_u64(s, word));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ସ୍ଥିର
M3Result PutLE32 (M3SnapshotSave* s, u32 i_ମୂଲ୍ୟ)
{
    u8 bytes[4];

    ପାଇଁ (u32 i = 0; i < ଆକାର(bytes); ++i) {
        bytes[i] = (u8)(i_ମୂଲ୍ୟ >> (8 * i));
    }

    ଫେରନ୍ତୁ Put(s, bytes, ଆକାର(bytes));
}

ସ୍ଥିର
M3Result PutLE64 (M3SnapshotSave* s, u64 i_ମୂଲ୍ୟ)
{
    u8 bytes[8];

    ପାଇଁ (u32 i = 0; i < ଆକାର(bytes); ++i) {
        bytes[i] = (u8)(i_ମୂଲ୍ୟ >> (8 * i));
    }

    ଫେରନ୍ତୁ Put(s, bytes, ଆକାର(bytes));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ସ୍ଥିର
M3Result Put_ମୂଲ୍ୟ_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    ଯଦି (IsRefType(i_type)) {
        ଫେରନ୍ତୁ Put_Ref_ମୂଲ୍ୟ(s, BaseTypeOf(i_type), (ଶୂନ୍ୟ*)(uintptr_t)i_bits);
    }

    ବାଛ (BaseTypeOf(i_type)) {
    ସ୍ଥିତି c_m3Type_i32: ଫେରନ୍ତୁ PutLEB_i64(s, (i32)(u32)i_bits);
    ସ୍ଥିତି c_m3Type_i64: ଫେରନ୍ତୁ PutLEB_i64(s, (i64)i_bits);
    ସ୍ଥିତି c_m3Type_f32: ଫେରନ୍ତୁ PutLE32(s, (u32)i_bits);
    ସ୍ଥିତି c_m3Type_f64: ଫେରନ୍ତୁ PutLE64(s, i_bits);
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "a snapshot cannot hold a value of this type as bits";
    }
}

ସ୍ଥିର
M3Result Put_ମୂଲ୍ୟ_Tag (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3TypeToValType(i_type);

    ଯଦି (not valtype) {
        ଫେରନ୍ତୁ "a snapshot has no tag for a value of this type";
    }

    ଫେରନ୍ତୁ PutU8(s, valtype);
}

ସ୍ଥିର
M3Result Put_ମୂଲ୍ୟ_Bits (M3SnapshotSave* s, m3type_t i_type, u64 i_bits)
{
    M3Result result = m3Err_none;

_   (Put_ମୂଲ୍ୟ_Tag(s, i_type));
_   (Put_ମୂଲ୍ୟ_Body(s, i_type, i_bits));

_catch:
    ଫେରନ୍ତୁ result;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ସ୍ଥିର
M3Result Put_ମୂଲ୍ୟ (M3SnapshotSave* s, m3type_t i_type, ଅଚଳ ଶୂନ୍ୟ* i_where)
{
    M3Result result = m3Err_none;

_   (Put_ମୂଲ୍ୟ_Tag(s, i_type));

    ଯଦି (IsRefType(i_type)) {
        ଶୂନ୍ୟ* reference;
        memcpy(&reference, i_where, ଆକାର(reference));
_       (Put_ମୂଲ୍ୟ_Body(s, i_type, (u64)(uintptr_t)reference));
    } ଅନ୍ୟଥା ଯଦି (BaseTypeOf(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 bytes[16];
        memcpy(bytes, i_where, ଆକାର(bytes));
_       (Put(s, bytes, ଆକାର(bytes)));
    } ଅନ୍ୟଥା ଯଦି (Is64BitType(i_type)) {
        u64 bits;
        memcpy(&bits, i_where, ଆକାର(bits));
_       (Put_ମୂଲ୍ୟ_Body(s, i_type, bits));
    } ଅନ୍ୟଥା {
        u32 narrow;
        memcpy(&narrow, i_where, ଆକାର(narrow));
_       (Put_ମୂଲ୍ୟ_Body(s, i_type, narrow));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result PutMemoryChunks (M3SnapshotSave* s, ଅଚଳ u8* i_bytes, size_t i_size)
{
    M3Result result = m3Err_none;
    size_t   cursor = 0;

    ଯେତେବେଳେ (cursor < i_size) {
        u8     byte = i_bytes[cursor];
        size_t run  = 1;

        ଯେତେବେଳେ (cursor + run < i_size and i_bytes[cursor + run] == byte) {
            run++;
        }

        ଯଦି ((byte == 0x00 or byte == 0xFF) and run >= d_m3SnapshotRunThreshold) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            ଯଦି (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)run));
            }

            cursor += run;
            ଜାରି;
        }

        size_t rawStart = cursor;

        ଯେତେବେଳେ (cursor < i_size) {
            u8     b      = i_bytes[cursor];
            size_t length = 1;

            ଯେତେବେଳେ (cursor + length < i_size and i_bytes[cursor + length] == b) {
                length++;
            }

            ଯଦି ((b == 0x00 or b == 0xFF) and length >= d_m3SnapshotRunThreshold) {
                ଭାଙ୍ଗ;
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
    ଫେରନ୍ତୁ result;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ସ୍ଥିର
u32 Memory_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    ଯେତେବେଳେ (module->memories[j] != module->memories[index]) {
        ++j;
    }
    ଫେରନ୍ତୁ j;
}

ସ୍ଥିର
u32 Table_FirstIndex (IM3Module module, u32 index)
{
    u32 j = 0;
    ଯେତେବେଳେ (module->tables[j] != module->tables[index]) {
        ++j;
    }
    ଫେରନ୍ତୁ j;
}

ସ୍ଥିର
M3Result SaveMemories (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ଅଙ୍କ_Memories));

    ପାଇଁ (u32 m = 0; m < module->ଅଙ୍କ_Memories; ++m) {
        IM3Memory memory   = module->memories[m];
        u32       first    = Memory_FirstIndex(module, m);
        bool      hasData  = memory and memory->mallocated;
        u32       pageSize = memory ? Memory_PageSize(memory) : d_m3DefaultMemPageSize;
        u64       bytes    = memory ? memory->ଅଙ୍କ_Pages * (u64)pageSize : 0;
        u32       pageBits = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        ଯଦି (first != m) {
            ଜାରି;
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
        ଯେତେବେଳେ ((1u << pageBits) < pageSize) {
            ++pageBits;
        }

_       (PutLEB_u32(s, pageBits));
_       (PutLEB_u32(s, memory ? (u32)memory->ଅଙ୍କ_Pages : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        ଯଦି (not s->writing) {
            ଜାରି;
        }

        ଯଦି (hasData) {
_           (PutMemoryChunks(s, m3MemData(memory->mallocated), (size_t)bytes));
        } ଅନ୍ୟଥା {
_           (PutU8(s, d_m3ChunkEnd));
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result SaveGlobals (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ଅଙ୍କ_Globals));

    ପାଇଁ (u32 g = 0; g < module->ଅଙ୍କ_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_ମୂଲ୍ୟ(s, global->type, &cell->i64_ମୂଲ୍ୟ));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result SaveTables (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ଅଙ୍କ_Tables));

    ପାଇଁ (u32 t = 0; t < module->ଅଙ୍କ_Tables; ++t) {
        IM3Table table = module->tables[t];
        u32      first = Table_FirstIndex(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        ଯଦି (first != t) {
            ଜାରି;
        }

_       (PutU8(s, M3TypeToValType(table->type)));
_       (PutLEB_u32(s, table->size));

        ପାଇଁ (u32 e = 0; e < table->size; ++e) {
_           (Put_Ref_ମୂଲ୍ୟ(s, BaseTypeOf(table->type), table->elements[e]));
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result SaveSegments (M3SnapshotSave* s)
{
    M3Result  result = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->ଅଙ୍କ_Data_Segments));

    ପାଇଁ (u32 i = 0; i < module->ଅଙ୍କ_Data_Segments; ++i) {
_       (PutU8(s, module->dataSegments[i].dropped));
    }

_   (PutLEB_u32(s, module->ଅଙ୍କ_Element_Segments));

    ପାଇଁ (u32 i = 0; i < module->ଅଙ୍କ_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

#  ଯଦି d_m3HasExceptionHandling

ସ୍ଥିର
M3Result SaveExceptions (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.ଗଣନା_2));

    ପାଇଁ (u32 i = 0; i < s->exceptions.ଗଣନା_2; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.ବସ୍ତୁଗୁଡ଼ିକ[i];
        IM3FuncType  type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, exn->ଅଙ୍କ_Args != Get_Func_Type_ଅଙ୍କ_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->ଅଙ୍କ_Args));

        ପାଇଁ (u32 a = 0; a < exn->ଅଙ୍କ_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_ମୂଲ୍ୟ_Bits(s, argType, exn->args[a]));
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3Activation {
    IM3Function        function;
    ଅଚଳ M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 kind;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    ଅଚଳ M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                lastFrame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ସ୍ଥିର
ଅଚଳ M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_kind)
{
    ଅଚଳ M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_kind) : ଖାଲି;

    ପାଇଁ (u32 i = 0; not point and i < i_module->ଅଙ୍କ_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_kind);
        ଯଦି (point) {
            *io_function = &i_module->functions[i];
        }
    }

    ଫେରନ୍ତୁ point;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ସ୍ଥିର
M3Result NextActivation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3Result result = m3Err_none;
    i32      i      = i_first;

    memset(o_act, 0, ଆକାର(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    ପାଇଁ (; i >= 0; --i) {
        ଅଚଳ M3Frame* frame = &i_cont->frames[i];

        _throwif("a suspended frame runs against another module's memory", frame->memory != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        ଯଦି (frame->kind == frame_call) {
            o_act->pc           = frame->pc;
            o_act->kind         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  ଯଦି d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            ଭାଙ୍ଗ;
        } ଅନ୍ୟଥା ଯଦି (frame->kind == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->kind   = safepoint_resume;
            o_act->resume = frame;
            ଭାଙ୍ଗ;
        }
    }

    ଯଦି (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->kind         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  ଯଦି d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->lastFrame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->kind);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  ଯଦି d_m3EntryKeepsFrame
    ପାଇଁ (i32 k = i_first; k > o_act->lastFrame; --k) {
        ଅଚଳ M3Frame* frame = &i_cont->frames[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->kind == frame_entry and frame->entry.function != o_act->function);
    }
#  endif

_catch:
    ଫେରନ୍ତୁ result;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ସ୍ଥିର
M3Result SaveActivation (M3SnapshotSave* s, m3slot_t* i_base, u32 i_ଅଙ୍କ_Slots, ଅଚଳ M3Activation* i_act)
{
    M3Result             result        = m3Err_none;
    IM3Function          function      = i_act->function;
    ଅଚଳ M3SnapshotMap* map           = function->snapshotMap;
    u32                  functionIndex = FunctionIndex(s->module, function);
    ଅଚଳ M3SafePoint*   point         = i_act->point;
    u32                  ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ;

    _throwif("a suspended frame belongs to another module", functionIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, functionIndex));
_   (PutU8(s, point->kind));
_   (PutLEB_u32(s, point->wasmOffset));

    ଯଦି (point->kind == safepoint_op) {
_       (PutLEB_u32(s, map->blocks[point->aux].wasmOffset));
    }

    ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ = map->ଅଙ୍କ_Locals + point->ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ;

_   (PutLEB_u32(s, ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ));

    ପାଇଁ (u32 v = 0; v < ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ; ++v) {
        ଅଚଳ M3_Slot_ମୂଲ୍ୟ* ମୂଲ୍ୟ = Frame_ମୂଲ୍ୟ(map, point, v);
        u8*                where;
        u64                word;
        ଶୂନ୍ୟ*              reference;

_       (Locate_ମୂଲ୍ୟ(i_base, i_ଅଙ୍କ_Slots, i_act->sp, ମୂଲ୍ୟ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ଯଦି (BaseTypeOf(ମୂଲ୍ୟ->type) == c_m3Type_v128) {
_           (Put_ମୂଲ୍ୟ_Tag(s, ମୂଲ୍ୟ->type));
_           (Put(s, where, 16));
            ଜାରି;
        }

_       (Read_ମୂଲ୍ୟ(where, &i_act->registers, ମୂଲ୍ୟ, &word, &reference));

_       (Put_ମୂଲ୍ୟ_Bits(s, ମୂଲ୍ୟ->type, IsRefType(ମୂଲ୍ୟ->type) ? (u64)(uintptr_t)reference : word));
    }

    ଯଦି (point->kind == safepoint_resume) {
_       (Put_Ref_ମୂଲ୍ୟ(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
ସ୍ଥିର
M3Result SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3Result      result   = m3Err_none;
    M3Activation* acts     = ଖାଲି;
    u32           ଅଙ୍କ_Acts  = 0;
    IM3Function   function = i_cont->entryFunction;
    i32           first    = (i32)i_cont->ଅଙ୍କ_Frames - 1;
    m3slot_t*     base;
    u32           ଅଙ୍କ_Slots;
    m3stack_t     sp;

_   (Continuation_ଗଦା(s->runtime, i_cont, &base, &ଅଙ୍କ_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->ଅଙ୍କ_Frames + 1);
    _throwifnull(acts);

    sp = base;

    ଯେତେବେଳେ (true) {
        M3Activation* act = &acts[ଅଙ୍କ_Acts];

_       (NextActivation(s->module, i_cont, function, sp, first, act));

        ଯଦି (act->kind != safepoint_call) {
            ଅଙ୍କ_Acts++;
            ଭାଙ୍ଗ;
        }

        _throwif("a suspended continuation stops at a call", act->lastFrame < 0);

        ଯଦି (not (act->point->flags & d_m3SafePointTailCall)) {
            ଅଙ୍କ_Acts++;
        }

        function = i_cont->frames[act->lastFrame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->lastFrame - 1;
    }

_   (PutLEB_u32(s, ଅଙ୍କ_Acts));

    ପାଇଁ (u32 a = 0; a < ଅଙ୍କ_Acts; ++a) {
_       (SaveActivation(s, base, ଅଙ୍କ_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3Result  result    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_ମୂଳ    = (i_cont == s->runtime->ମୂଳ_Continuation);
    u8        state     = snapshot_contFinished;
    u32       typeIndex = is_ମୂଳ ? d_m3SnapshotNone : FuncTypeIndex(module, i_cont->type);

    ଯଦି (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } ଅନ୍ୟଥା ଯଦି (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } ଅନ୍ୟଥା ଯଦି (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->entryFunction);
    _throwif("a continuation belongs to another module", FunctionIndex(module, i_cont->entryFunction) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_ମୂଳ and typeIndex == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_ମୂଳ));
_   (PutLEB_u32(s, typeIndex));
_   (PutLEB_u32(s, FunctionIndex(module, i_cont->entryFunction)));
#  ଯଦି d_m3HasExceptionHandling
_   (Put_Ref_ମୂଲ୍ୟ(s, c_m3Type_exnref, i_cont->resumeThrow));
#  ଅନ୍ୟଥା
_   (Put_Ref_ମୂଲ୍ୟ(s, c_m3Type_exnref, ଖାଲି));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    ଯଦି (s->postmortem) {
        ଯାଅ _catch;
    }

    ଯଦି (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->entryFunction ? i_cont->entryFunction->funcType : ଖାଲି;
        u16         ଅଙ୍କ_Rets = inner ? Get_Func_Type_ଅଙ୍କ_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_Args_ଗଣନା > Get_Func_Type_ଅଙ୍କ_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_Args_ଗଣନା));

        ପାଇଁ (u32 i = 0; i < i_cont->bound_Args_ଗଣନା; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_ମୂଲ୍ୟ(s, type, i_cont->val_ଗଦା + (ଅଙ୍କ_Rets + i) * c_io_Slot_ଗଣନା));
        }
    } ଅନ୍ୟଥା ଯଦି (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_Args_ଗଣନା));
_       (SaveSuspendedBody(s, i_cont));
    } ଅନ୍ୟଥା {
_       (PutLEB_u32(s, 0));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result SaveContinuations (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

_   (PutLEB_u32(s, s->continuations.ଗଣନା_2));

    ପାଇଁ (u32 i = 0; i < s->continuations.ଗଣନା_2; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.ବସ୍ତୁଗୁଡ଼ିକ[i]));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ସ୍ଥିର
M3Result SaveHostState (M3SnapshotSave* s)
{
    M3Result               result = m3Err_none;
    ଅଚଳ M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3BufferWriter         bw     = { ଖାଲି, 0, 0 };

    ଯଦି (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, BufferWriter_Write, &bw));
    }

    ଯଦି (bw.size > 0) {
_       (Put(s, bw.buffer, bw.size));
    }

_catch:
    m3_Free(bw.buffer);

    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
ଶୂନ୍ୟ SnapshotResourceTotals (IM3Module module, u64* memoryBytes, u64* tableElements)
{
    *memoryBytes = *tableElements = 0;
    ପାଇଁ (u32 i = 0; i < module->ଅଙ୍କ_Memories; ++i) {
        IM3Memory memory = module->memories[i];
        ଯଦି (Memory_FirstIndex(module, i) == i and memory) {
            *memoryBytes += memory->ଅଙ୍କ_Pages * (u64)Memory_PageSize(memory);
        }
    }
    ପାଇଁ (u32 i = 0; i < module->ଅଙ୍କ_Tables; ++i) {
        IM3Table table = module->tables[i];
        ଯଦି (Table_FirstIndex(module, i) == i and table) {
            *tableElements += table->size;
        }
    }
}

ସ୍ଥିର
M3Result SaveMeta (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;
    u64      hash   = 0;
    u64      memoryBytes, tableElements;
    u32      activeStacks = 0;
    ପାଇଁ (u32 i = 0; i < s->continuations.ଗଣନା_2; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.ବସ୍ତୁଗୁଡ଼ିକ[i];
        ଯଦି (cont != s->runtime->ମୂଳ_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            activeStacks++;
        }
    }
    SnapshotResourceTotals(s->module, &memoryBytes, &tableElements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.ଗଣନା_2));
_   (PutLEB_u32(s, s->exceptions.ଗଣନା_2));
_   (PutLEB_u64(s, memoryBytes));
_   (PutLEB_u64(s, tableElements));
_   (PutLEB_u32(s, activeStacks));

#  ଯଦି d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    ପାଇଁ (u32 i = 0; i < s->exceptions.ଗଣନା_2; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.ବସ୍ତୁଗୁଡ଼ିକ[i];
        u32          tagIndex  = TagIndex(s->module, exception->tag);

        _throwif("an exception was thrown with another module's tag", tagIndex == d_m3SnapshotNone);

_       (PutLEB_u32(s, tagIndex));
    }
#  endif

_catch:
    ଫେରନ୍ତୁ result;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ସ୍ଥିର
M3Result DiscoverReferences (M3SnapshotSave* s)
{
    M3Result result            = m3Err_none;
    u32      doneContinuations = 0;
    u32      doneExceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    ଯେତେବେଳେ (doneContinuations < s->continuations.ଗଣନା_2 or doneExceptions < s->exceptions.ଗଣନା_2) {
#  ଯଦି d_m3HasExceptionHandling
        ଯେତେବେଳେ (doneExceptions < s->exceptions.ଗଣନା_2) {
            M3Exception* exception = (M3Exception*)s->exceptions.ବସ୍ତୁଗୁଡ଼ିକ[doneExceptions++];
            IM3FuncType  type      = exception->tag->type;

            _throwif(m3Err_wasmMalformed, exception->ଅଙ୍କ_Args != Get_Func_Type_ଅଙ୍କ_Params(type));

            ପାଇଁ (u32 a = 0; a < exception->ଅଙ୍କ_Args; ++a) {
_               (Put_ମୂଲ୍ୟ_Bits(s, GetFuncTypeParamType(type, (u16)a), exception->args[a]));
            }
        }
#  endif
        ଯେତେବେଳେ (doneContinuations < s->continuations.ଗଣନା_2 and doneExceptions == s->exceptions.ଗଣନା_2) {
            u32 id = doneContinuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.ବସ୍ତୁଗୁଡ଼ିକ[id]));
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result SaveSections (M3SnapshotSave* s)
{
    M3Result result = m3Err_none;

    ଯଦି (not s->writing) {
        ଫେରନ୍ତୁ DiscoverReferences(s);
    }

    ଯଦି (not s->embedded) {
_       (Put(s, c_snapshotMagic, ଆକାର(c_snapshotMagic)));
_       (PutLE32(s, c_snapshotVersion));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    ଯଦି (s->module->ଅଙ୍କ_Memories) {
_       (SaveSection(s, d_m3SnapshotSection_Memory, SaveMemories));
    }
    ଯଦି (s->module->ଅଙ୍କ_Tables) {
_       (SaveSection(s, d_m3SnapshotSection_Table, SaveTables));
    }
    ଯଦି (s->module->ଅଙ୍କ_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    ଯଦି (s->module->ଅଙ୍କ_Data_Segments or s->module->ଅଙ୍କ_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  ଯଦି d_m3HasExceptionHandling
    ଯଦି (s->exceptions.ଗଣନା_2) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    ଯଦି (s->continuations.ଗଣନା_2) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    ଫେରନ୍ତୁ result;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3SnapshotSuspendSite {
    ଅଚଳ M3SnapshotMap* map;
    ଅଚଳ M3SafePoint*   point;
    u32                  target;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3SnapshotLoad {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    ଶୂନ୍ୟ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ଗଦା;
    bool      changed;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    ଅଙ୍କ_Continuations;
    u64                    requiredMemoryBytes;
    u64                    requiredTableElements;
    u64                    loadedMemoryBytes;
    u64                    loadedTableElements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  ଯଦି d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 ଅଙ୍କ_Exceptions;

    M3Frame* frames;           // a suspended continuation's frames as they are put together, outermost first

    bytes_t pos;               // bounded section payload, read using the Wasm parser's helpers
    bytes_t end;
    bool    inSection;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    seenSections[256];
} M3SnapshotLoad;

ସ୍ଥିର
M3Result Get (M3SnapshotLoad* l, ଶୂନ୍ୟ* o_data, size_t i_size)
{
    ଯଦି (not l->inSection) {
        ଫେରନ୍ତୁ l->reader(o_data, i_size, l->userdata);
    }
    ଯଦି (i_size > (size_t)(l->end - l->pos)) {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
    ଯଦି (i_size) {
        memcpy(o_data, l->pos, i_size);
        l->pos += i_size;
    }
    ଫେରନ୍ତୁ m3Err_none;
}

ସ୍ଥିର
M3Result GetU8 (M3SnapshotLoad* l, u8* o_ମୂଲ୍ୟ)
{
    ଫେରନ୍ତୁ Get(l, o_ମୂଲ୍ୟ, ଆକାର(*o_ମୂଲ୍ୟ));
}

ସ୍ଥିର
M3Result GetBool (M3SnapshotLoad* l, u8* o_ମୂଲ୍ୟ)
{
    M3Result result = GetU8(l, o_ମୂଲ୍ୟ);
    ଫେରନ୍ତୁ result ? result : (*o_ମୂଲ୍ୟ <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ସ୍ଥିର
M3Result GetLEB_u32 (M3SnapshotLoad* l, u32* o_ମୂଲ୍ୟ)
{
    ଯଦି (l->inSection) {
        ଫେରନ୍ତୁ ReadLEB_u32(o_ମୂଲ୍ୟ, &l->pos, l->end);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    ପାଇଁ (u32 i = 0; i < ଆକାର(encoded); ++i) {
        M3Result result = GetU8(l, &encoded[i]);
        ଯଦି (result) {
            ଫେରନ୍ତୁ result;
        }
        ଯଦି (not (encoded[i] & 0x80)) {
            bytes_t pos = encoded;
            ଫେରନ୍ତୁ ReadLEB_u32(o_ମୂଲ୍ୟ, &pos, encoded + i + 1);
        }
    }
    ଫେରନ୍ତୁ m3Err_lebOverflow;
}

ସ୍ଥିର
M3Result GetLEB_u64 (M3SnapshotLoad* l, u64* o_ମୂଲ୍ୟ)
{
    ଫେରନ୍ତୁ ReadLebUnsigned(o_ମୂଲ୍ୟ, 64, &l->pos, l->end);
}

ସ୍ଥିର
M3Result GetLEB_i64 (M3SnapshotLoad* l, i64* o_ମୂଲ୍ୟ)
{
    ଫେରନ୍ତୁ ReadLEB_i64(o_ମୂଲ୍ୟ, &l->pos, l->end);
}

ସ୍ଥିର
M3Result SkipBytes (M3SnapshotLoad* l, size_t i_size)
{
    ଯଦି (i_size > (size_t)(l->end - l->pos)) {
        ଫେରନ୍ତୁ m3Err_wasmUnderrun;
    }
    l->pos += i_size;
    ଫେରନ୍ତୁ m3Err_none;
}


ସ୍ଥିର
M3Result Get_Ref_ମୂଲ୍ୟ (M3SnapshotLoad* l, m3type_t i_type, ଶୂନ୍ୟ** o_reference)
{
    M3Result result = m3Err_none;
    u8       kind   = 0;
    u64      id     = 0;

    *o_reference = ଖାଲି;

_   (GetU8(l, &kind));
    ଯଦି (kind == 0x00) {
        ଫେରନ୍ତୁ (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    ଯଦି (kind == 0x01) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->ଅଙ୍କ_Functions);
#  ଯଦି d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not IsSubTypeOf(RefTypeOfFuncType(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } ଅନ୍ୟଥା ଯଦି (kind == 0x02) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_externref);
        ଅଚଳ M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        ଯଦି (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  ଯଦି d_m3HasExceptionHandling
    ଅନ୍ୟଥା ଯଦି (kind == 0x03) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->ଅଙ୍କ_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    ଅନ୍ୟଥା ଯଦି (kind == 0x04) {
        _throwif(m3Err_wasmMalformed, BaseTypeOf(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->ଅଙ୍କ_Continuations);
        *o_reference = l->continuations[id];
#  ଯଦି d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        ଯଦି (not l->checking and HeapTypeOf(i_type) != d_m3Type_heapAbstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } ଅନ୍ୟଥା {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result GetLE32 (M3SnapshotLoad* l, u32* o_ମୂଲ୍ୟ)
{
    u8       bytes[4];
    M3Result result = Get(l, bytes, ଆକାର(bytes));

    *o_ମୂଲ୍ୟ = 0;

    ପାଇଁ (u32 i = 0; i < ଆକାର(bytes); ++i) {
        *o_ମୂଲ୍ୟ |= (u32)bytes[i] << (8 * i);
    }

    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result GetLE64 (M3SnapshotLoad* l, u64* o_ମୂଲ୍ୟ)
{
    u8       bytes[8];
    M3Result result = Get(l, bytes, ଆକାର(bytes));

    *o_ମୂଲ୍ୟ = 0;

    ପାଇଁ (u32 i = 0; i < ଆକାର(bytes); ++i) {
        *o_ମୂଲ୍ୟ |= (u64)bytes[i] << (8 * i);
    }

    ଫେରନ୍ତୁ result;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ସ୍ଥିର
M3Result Get_ମୂଲ୍ୟ_Tag (M3SnapshotLoad* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3Result result  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = ValTypeToM3Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and BaseTypeOf(*o_type) != BaseTypeOf(i_expectedType));
    ଯଦି (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ସ୍ଥିର
M3Result Get_ମୂଲ୍ୟ_Body (M3SnapshotLoad* l, m3type_t i_type, u64* o_bits)
{
    M3Result result = m3Err_none;

    *o_bits = 0;

    ଯଦି (IsRefType(i_type)) {
        ଶୂନ୍ୟ* reference = ଖାଲି;
_       (Get_Ref_ମୂଲ୍ୟ(l, i_type, &reference));
        *o_bits = (u64)(uintptr_t)reference;
    } ଅନ୍ୟଥା ଯଦି (BaseTypeOf(i_type) == c_m3Type_i32) {
        i32 ମୂଲ୍ୟ = 0;
_       (ReadLEB_i32(&ମୂଲ୍ୟ, &l->pos, l->end));
        *o_bits = (u32)ମୂଲ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (BaseTypeOf(i_type) == c_m3Type_i64) {
        i64 ମୂଲ୍ୟ = 0;
_       (GetLEB_i64(l, &ମୂଲ୍ୟ));
        *o_bits = (u64)ମୂଲ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (BaseTypeOf(i_type) == c_m3Type_f32) {
        u32 bits = 0;
_       (GetLE32(l, &bits));
        *o_bits = bits;
    } ଅନ୍ୟଥା ଯଦି (BaseTypeOf(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_bits));
    } ଅନ୍ୟଥା {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result Get_ମୂଲ୍ୟ_Bits (M3SnapshotLoad* l, m3type_t i_expectedType, u64* o_bits)
{
    M3Result result = m3Err_none;
    m3type_t type;

_   (Get_ମୂଲ୍ୟ_Tag(l, i_expectedType, &type));
_   (Get_ମୂଲ୍ୟ_Body(l, type, o_bits));

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result Get_ମୂଲ୍ୟ (M3SnapshotLoad* l, m3type_t i_expectedType, ଶୂନ୍ୟ* o_where)
{
    M3Result result = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    ଯଦି (l->checking) {
        o_where = scratch;
    }

_   (Get_ମୂଲ୍ୟ_Tag(l, i_expectedType, &type));

    ଯଦି (IsRefType(type)) {
        ଶୂନ୍ୟ* reference = ଖାଲି;
        u64   bits      = 0;
_       (Get_ମୂଲ୍ୟ_Body(l, type, &bits));
        reference = (ଶୂନ୍ୟ*)(uintptr_t)bits;
        memcpy(o_where, &reference, ଆକାର(reference));
    } ଅନ୍ୟଥା ଯଦି (BaseTypeOf(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } ଅନ୍ୟଥା {
        u64 bits = 0;
_       (Get_ମୂଲ୍ୟ_Body(l, type, &bits));

        ଯଦି (Is64BitType(type)) {
            memcpy(o_where, &bits, ଆକାର(bits));
        } ଅନ୍ୟଥା {
            u32 narrow = (u32)bits;
            memcpy(o_where, &narrow, ଆକାର(narrow));
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result GetFunction (M3SnapshotLoad* l, IM3Function* o_function)
{
    M3Result result = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->ଅଙ୍କ_Functions);
    *o_function = &l->module->functions[index];

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadMemoryChunks (M3SnapshotLoad* l, u8* o_bytes, size_t i_size)
{
    M3Result result = m3Err_none;

    ପାଇଁ (;;) {
        u8  chunkType = 0;
        u32 offset = 0, length = 0;

_       (GetU8(l, &chunkType));

        ଯଦି (chunkType == d_m3ChunkEnd) {
            ଭାଙ୍ଗ;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &length));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_size or length > i_size - (size_t)offset);

        ଯଦି (chunkType == d_m3ChunkRaw) {
            ଯଦି (l->checking or length == 0) {
_               (SkipBytes(l, length));
            } ଅନ୍ୟଥା {
_               (Get(l, o_bytes + offset, length));
            }
        } ଅନ୍ୟଥା ଯଦି (chunkType == d_m3ChunkFillFF) {
            ଯଦି (not l->checking and length) {
                memset(o_bytes + offset, 0xFF, length);
            }
        } ଅନ୍ୟଥା {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadMemories (M3SnapshotLoad* l)
{
    M3Result  result      = m3Err_none;
    IM3Module module      = l->module;
    u32       ଅଙ୍କ_Memories = 0;
    l->loadedMemoryBytes  = 0;

_   (GetLEB_u32(l, &ଅଙ୍କ_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Memories == 0 or ଅଙ୍କ_Memories != module->ଅଙ୍କ_Memories);

    ପାଇଁ (u32 m = 0; m < ଅଙ୍କ_Memories; ++m) {
        u32       memIndex = 0, firstIndex = 0, pageBits = 0, ଅଙ୍କ_Pages = 0;
        IM3Memory memory;

_       (GetLEB_u32(l, &memIndex));
_       (GetLEB_u32(l, &firstIndex));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, memIndex != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 firstIndex != Memory_FirstIndex(module, m));
        ଯଦି (firstIndex != m) {
            ଜାରି;
        }

_       (GetLEB_u32(l, &pageBits));
_       (GetLEB_u32(l, &ଅଙ୍କ_Pages));

        memory = module->memories[memIndex];
        _throwif("a shared memory cannot be restored from a snapshot", memory and memory->isShared);

        size_t bytes = 0;
        u8*    data  = ଖାଲି;

        ଯଦି (memory) {
            _throwif(m3Err_wasmMalformed, pageBits >= 32 or (1u << pageBits) != Memory_PageSize(memory));
            _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Pages < memory->ଅଙ୍କ_Pages or ଅଙ୍କ_Pages > memory->maxPages);
            _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Pages > SIZE_MAX / Memory_PageSize(memory));
            bytes = (size_t)ଅଙ୍କ_Pages * Memory_PageSize(memory);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->memoryLimit and bytes > (size_t)l->runtime->memoryLimit);

            ଯଦି (not l->checking) {
                ଯଦି (memory->ଅଙ୍କ_Pages < ଅଙ୍କ_Pages) {
_                   (ResizeMemory(l->runtime, memory, ଅଙ୍କ_Pages));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         memory->mallocated and memory->mallocated->length < bytes);
            }

            l->loadedMemoryBytes += bytes;

            ଯଦି (bytes and memory->mallocated) {
                data = m3MemData(memory->mallocated);
            }
        } ଅନ୍ୟଥା {
            _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Pages != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        ଯଦି (data and not l->checking) {
            memset(data, 0, bytes);
        }

_       (LoadMemoryChunks(l, data, bytes));
    }
    _throwif(m3Err_wasmMalformed, l->loadedMemoryBytes != l->requiredMemoryBytes);

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadGlobals (M3SnapshotLoad* l)
{
    M3Result  result     = m3Err_none;
    IM3Module module     = l->module;
    u32       ଅଙ୍କ_Globals = 0;

_   (GetLEB_u32(l, &ଅଙ୍କ_Globals));
    _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Globals == 0 or ଅଙ୍କ_Globals != module->ଅଙ୍କ_Globals);

    ପାଇଁ (u32 g = 0; g < ଅଙ୍କ_Globals; ++g) {
        u32       globalIndex = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &globalIndex));
        _throwif(m3Err_wasmMalformed, globalIndex != g);

        global = &module->globals[globalIndex];
        cell   = global->resolved ? global->resolved : global;

_       (Get_ମୂଲ୍ୟ(l, global->type, &cell->i64_ମୂଲ୍ୟ));
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadTables (M3SnapshotLoad* l)
{
    M3Result  result       = m3Err_none;
    IM3Module module       = l->module;
    u32       ଅଙ୍କ_Tables    = 0;
    l->loadedTableElements = 0;

_   (GetLEB_u32(l, &ଅଙ୍କ_Tables));
    _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Tables == 0 or ଅଙ୍କ_Tables != module->ଅଙ୍କ_Tables);

    ପାଇଁ (u32 t = 0; t < ଅଙ୍କ_Tables; ++t) {
        u32      tableIndex = 0, firstIndex = 0, size = 0;
        u8       valtype = 0;
        IM3Table table;

_       (GetLEB_u32(l, &tableIndex));
_       (GetLEB_u32(l, &firstIndex));

        _throwif(m3Err_wasmMalformed, tableIndex != t);
        _throwif("the snapshot shares tables between imports differently",
                 firstIndex != Table_FirstIndex(module, t));
        ଯଦି (firstIndex != t) {
            ଜାରି;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &size));

        table = module->tables[tableIndex];

        _throwif(m3Err_wasmMalformed, valtype != M3TypeToValType(table->type));
        _throwif(m3Err_wasmMalformed, size > (table->hasMax ? table->maxSize : d_m3MaxSaneTableSize));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, size < table->size);

        l->loadedTableElements += size;

        ଯଦି (not l->checking and size != table->size) {
            ଶୂନ୍ୟ** elements;

            _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                                 size - table->size > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
            elements = m3_ReallocArray(ଶୂନ୍ୟ*, table->elements, size, table->size);
            _throwifnull(elements);

            l->runtime->tableElementsUsed += size - table->size;
            table->elements = elements;
            table->size     = size;
        }

        ପାଇଁ (u32 e = 0; e < size; ++e) {
            ଶୂନ୍ୟ* reference = ଖାଲି;
_           (Get_Ref_ମୂଲ୍ୟ(l, table->type, &reference));
            ଯଦି (not l->checking) {
                table->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loadedTableElements != l->requiredTableElements);

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadSegments (M3SnapshotLoad* l)
{
    M3Result  result = m3Err_none;
    IM3Module module = l->module;
    u32       ଗଣନା_2  = 0;

_   (GetLEB_u32(l, &ଗଣନା_2));
    _throwif(m3Err_wasmMalformed, ଗଣନା_2 != module->ଅଙ୍କ_Data_Segments);
    _throwif(m3Err_wasmMalformed, module->ଅଙ୍କ_Data_Segments == 0 and module->ଅଙ୍କ_Element_Segments == 0);

    ପାଇଁ (u32 i = 0; i < ଗଣନା_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ଯଦି (not l->checking) {
            module->dataSegments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &ଗଣନା_2));
    _throwif(m3Err_wasmMalformed, ଗଣନା_2 != module->ଅଙ୍କ_Element_Segments);

    ପାଇଁ (u32 i = 0; i < ଗଣନା_2; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        ଯଦି (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// A frame for the continuation being put together, outermost first
ସ୍ଥିର
M3Result AddFrame (M3SnapshotLoad* l, u32* io_ଅଙ୍କ_Frames, M3FrameKind i_kind, m3stack_t i_sp, M3Frame** o_frame)
{
    ଯଦି (*io_ଅଙ୍କ_Frames >= d_m3ContinuationMaxFrames) {
        ଫେରନ୍ତୁ "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->frames[(*io_ଅଙ୍କ_Frames)++];

    memset(frame, 0, ଆକାର(*frame));
    frame->kind   = (u8)i_kind;
    frame->sp     = i_sp;
    frame->memory = Module_Memory0(l->module);

    *o_frame = frame;

    ଫେରନ୍ତୁ m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ସ୍ଥିର
M3Result PrepareFunction (IM3Function i_function)
{
    M3Result result = m3Err_none;

    ଯଦି (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    ଫେରନ୍ତୁ result;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ସ୍ଥିର
M3Result AddBlockFrames (M3SnapshotLoad* l, ଅଚଳ M3SnapshotMap* i_map, ଅଚଳ M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_ଅଙ୍କ_Frames)
{
    M3Result result = m3Err_none;

    ପାଇଁ (u32 b = 0; b < i_map->ଅଙ୍କ_Blocks; ++b) {
        ଅଚଳ M3BlockStart* block = &i_map->blocks[b];
        M3Frame*            frame;

        ଯଦି (not EnclosesOffset(block, i_point->wasmOffset)) {
            ଜାରି;
        }

        ଯଦି (block->opcode == c_waOp_loop) {
_           (AddFrame(l, io_ଅଙ୍କ_Frames, frame_loop, i_sp, &frame));
            frame->pc = block->pc;
        }
#  ଯଦି d_m3HasExceptionHandling
        ଅନ୍ୟଥା {
_           (AddFrame(l, io_ଅଙ୍କ_Frames, frame_try, i_sp, &frame));
            frame->pc                = block->pc;
            frame->try_.ଅଙ୍କ_Clauses   = block->ଅଙ୍କ_Clauses;
            frame->try_.handlersLive = HandlersLiveAt(i_map, i_point, block);
        }
#  endif
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// Puts a function's values where this build keeps them at the safepoint
ସ୍ଥିର
M3Result Load_ମୂଲ୍ୟଗୁଡ଼ିକ (M3SnapshotLoad* l, m3slot_t* i_base, u32 i_ଅଙ୍କ_Slots, m3stack_t i_sp,
                     ଅଚଳ M3SnapshotMap* i_map, ଅଚଳ M3SafePoint* i_point, ଅଚଳ M3FrameRegisters* i_registers)
{
    M3Result result    = m3Err_none;
    u32      ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ = 0;

_   (GetLEB_u32(l, &ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ));
    _throwif("the snapshot's frame does not match what this build compiled", ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ != i_map->ଅଙ୍କ_Locals + i_point->ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ);

    ପାଇଁ (u32 v = 0; v < ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ; ++v) {
        ଅଚଳ M3_Slot_ମୂଲ୍ୟ* ମୂଲ୍ୟ     = Frame_ମୂଲ୍ୟ(i_map, i_point, v);
        u64                word      = 0;
        ଶୂନ୍ୟ*              reference = ଖାଲି;
        u8*                where;

_       (Locate_ମୂଲ୍ୟ(i_base, i_ଅଙ୍କ_Slots, i_sp, ମୂଲ୍ୟ, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        ଯଦି (BaseTypeOf(ମୂଲ୍ୟ->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_ମୂଲ୍ୟ_Tag(l, ମୂଲ୍ୟ->type, &type));
            ଯଦି (l->checking) {
_               (SkipBytes(l, 16));
            } ଅନ୍ୟଥା {
_               (Get(l, where, 16));
            }
            ଜାରି;
        }

        l->allowUnfilledRef = i_point->kind == safepoint_suspend and
                              v >= ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ - i_point->ଅଙ୍କ_Results;
_       (Get_ମୂଲ୍ୟ_Bits(l, ମୂଲ୍ୟ->type, &word));
        l->allowUnfilledRef = false;

        ଯଦି (IsRefType(ମୂଲ୍ୟ->type)) {
            reference = (ଶୂନ୍ୟ*)(uintptr_t)word;
        }

        ଯଦି (not l->checking) {
_           (Write_ମୂଲ୍ୟ(where, i_registers, ମୂଲ୍ୟ, word, reference));
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ସ୍ଥିର
M3Result LoadSuspendedBody (M3SnapshotLoad* l, IM3Continuation io_cont, u32 i_id)
{
    M3Result    result    = m3Err_none;
    u32         ଅଙ୍କ_Frames = 0;
    M3Frame*    callFrame = ଖାଲି;
    IM3Function function  = ଖାଲି;
    m3slot_t*   base;
    u32         ଅଙ୍କ_Slots;
    u32         ଅଙ୍କ_Activations = 0;
    IM3FuncType expected       = io_cont->entryFunction->funcType;
    m3stack_t   sp;

_   (Continuation_ଗଦା(l->runtime, io_cont, &base, &ଅଙ୍କ_Slots));

_   (GetLEB_u32(l, &ଅଙ୍କ_Activations));
    _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Activations == 0 or ଅଙ୍କ_Activations > d_m3ContinuationMaxFrames);

    io_cont->pc                = ଖାଲି;
    io_cont->r0                = 0;
    io_cont->ଅଙ୍କ_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  ଯଦି d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = base;

    ପାଇଁ (u32 a = 0; a < ଅଙ୍କ_Activations; ++a) {
        bool                 isInnermost = (a + 1 == ଅଙ୍କ_Activations);
        ଅଚଳ M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   kind       = 0;
        u32                  wasmOffset = 0, targetLoop = 0;
        ଅଚଳ M3SnapshotMap* map;

        memset(&registers, 0, ଆକାର(registers));

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
                 (u32)(sp - base) + function->max_ଗଦା_Slots > ଅଙ୍କ_Slots);

        // the call the frame outside this one is waiting on
        ଯଦି (callFrame) {
            callFrame->call.function = function;
        }

#  ଯଦି d_m3EntryKeepsFrame
        {
            M3Frame* entry;
_           (AddFrame(l, &ଅଙ୍କ_Frames, frame_entry, sp, &entry));
            entry->pc             = function->compiled;
            entry->entry.function = function;
        }
#  endif

_       (GetU8(l, &kind));
_       (GetLEB_u32(l, &wasmOffset));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        ଯଦି (kind == safepoint_op) {
            ଅଚଳ M3BlockStart* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = FindBlockAt(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not EnclosesOffset(loop, wasmOffset));
        }

        point = FindSafePointAt(map, wasmOffset, kind, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (AddBlockFrames(l, map, point, sp, &ଅଙ୍କ_Frames));

        ଯଦି (not isInnermost) {
            _throwif(m3Err_wasmMalformed, kind != safepoint_call);

_           (AddFrame(l, &ଅଙ୍କ_Frames, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  ଯଦି d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } ଅନ୍ୟଥା ଯଦି (kind == safepoint_resume) {
            M3Frame* frame;
            u32      ଅଙ୍କ_Results = point->ଅଙ୍କ_Results;
            pc_t     resultsPC  = point->pc - 2 * ଅଙ୍କ_Results;

_           (AddFrame(l, &ଅଙ୍କ_Frames, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.ଅଙ୍କ_Results  = ଅଙ୍କ_Results;
            frame->resume.ଅଙ୍କ_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } ଅନ୍ୟଥା {
            _throwif(m3Err_wasmMalformed, kind != safepoint_op and kind != safepoint_entry and kind != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = kind;
            registers.r0          = &io_cont->r0;
#  ଯଦି d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (Load_ମୂଲ୍ୟଗୁଡ଼ିକ(l, base, ଅଙ୍କ_Slots, sp, map, point, &registers));

        ଯଦି (not l->checking) {
            RestoreConstants(function, sp);
        }

        ଯଦି (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } ଅନ୍ୟଥା ଯଦି (kind == safepoint_resume) {
            ଶୂନ୍ୟ* cont = ଖାଲି;
_           (Get_Ref_ମୂଲ୍ୟ(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == ଖାଲି);

            callFrame->resume.cont = (IM3Continuation)cont;
            ଯଦି (not l->checking) {
                l->suspendSites[i_id].point = point;
                ପାଇଁ (u32 i = 1; i < l->ଅଙ୍କ_Continuations; ++i) {
                    ଯଦି (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        ଭାଙ୍ଗ;
                    }
                }
            }
        } ଅନ୍ୟଥା {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            ଯଦି (point->flags & d_m3SafePointTakenBranch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            ଯଦି (kind == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->ଅଙ୍କ_Results > point->ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ or
                                                point->ଅଙ୍କ_Results > d_m3MaxContinuationPayload);

                io_cont->ଅଙ୍କ_Suspend_Results = point->ଅଙ୍କ_Results;
                ଯଦି (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                ପାଇଁ (u32 i = 0; i < point->ଅଙ୍କ_Results; ++i) {
                    ଅଚଳ M3_Slot_ମୂଲ୍ୟ* ମୂଲ୍ୟ = &map->ମୂଲ୍ୟଗୁଡ଼ିକ[point->first_ମୂଲ୍ୟ + point->ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ - point->ଅଙ୍କ_Results + i];

                    _throwif(m3Err_wasmMalformed, IsRegisterSlotAlias(ମୂଲ୍ୟ->slot));

                    io_cont->suspendResultOffsets[i] = ମୂଲ୍ୟ->slot;
                    io_cont->suspendResultIs64[i]    = Is64BitType(ମୂଲ୍ୟ->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    ଯଦି (l->checking) {
        ଫେରନ୍ତୁ m3Err_none;
    }

    // held innermost first
    ଯଦି (ଅଙ୍କ_Frames > io_cont->framesCap) {
        M3Frame* frames = m3_ReallocArray(M3Frame, io_cont->frames, ଅଙ୍କ_Frames, io_cont->framesCap);
        _throwifnull(frames);

        io_cont->frames    = frames;
        io_cont->framesCap = ଅଙ୍କ_Frames;
    }

    ପାଇଁ (u32 k = 0; k < ଅଙ୍କ_Frames; ++k) {
        io_cont->frames[ଅଙ୍କ_Frames - 1 - k] = l->frames[k];
    }

    io_cont->ଅଙ୍କ_Frames = ଅଙ୍କ_Frames;

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadContinuation (M3SnapshotLoad* l, u32 i_id)
{
    M3Result        result = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    ଯଦି (l->checking) {
        scratch          = *cont;
        scratch.val_ଗଦା = l->check_ଗଦା;
        ଯଦି (i_id == 0) {
            scratch.val_ଗଦା = (m3slot_t*)l->runtime->origin_ଗଦା;
        }
        cont = &scratch;
    }

    u32 contId = 0, typeIndex = 0, entryIndex = 0, bound_Args_ଗଣନା = 0;
    u8  state = 0, is_ମୂଳ = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_ମୂଳ));
_   (GetLEB_u32(l, &typeIndex));
_   (GetLEB_u32(l, &entryIndex));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_ମୂଳ != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_ମୂଳ and state != snapshot_contSuspended);
    ଯଦି (not is_ମୂଳ and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, typeIndex != d_m3SnapshotNone and typeIndex >= module->ଅଙ୍କ_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, entryIndex >= module->ଅଙ୍କ_Functions);
    _throwif(m3Err_wasmMalformed, is_ମୂଳ ? typeIndex != d_m3SnapshotNone : (typeIndex == d_m3SnapshotNone or not module->funcTypes[typeIndex]->isContinuation));

    ଯଦି (not l->checking and not is_ମୂଳ and state != snapshot_contFinished) {
_       (Continuation_Acquire_ଗଦା(l->runtime, cont));
    }
    cont->type          = (typeIndex != d_m3SnapshotNone) ? module->funcTypes[typeIndex] : ଖାଲି;
    cont->entryFunction = &module->functions[entryIndex];
    cont->ଅଙ୍କ_Handlers   = 0;
    cont->handlersPC    = ଖାଲି;
    cont->parent        = ଖାଲି;
    cont->ଅଙ୍କ_Frames     = 0;

#  ଯଦି d_m3HasExceptionHandling
    {
        ଶୂନ୍ୟ* ref = ଖାଲି;
_       (Get_Ref_ମୂଲ୍ୟ(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_ମୂଳ or state == snapshot_contFinished) and cont->resumeThrow != ଖାଲି);
#  ଅନ୍ୟଥା
    {
        ଶୂନ୍ୟ* ref = ଖାଲି;
_       (Get_Ref_ମୂଲ୍ୟ(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != ଖାଲି);
    }
#  endif

    ଯଦି (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         ଅଙ୍କ_Rets;

_       (GetLEB_u32(l, &bound_Args_ଗଣନା));
        cont->bound_Args_ଗଣନା = bound_Args_ଗଣନା;
        inner                = cont->entryFunction->funcType;

        _throwif(m3Err_wasmMalformed, bound_Args_ଗଣନା > Get_Func_Type_ଅଙ୍କ_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_ଅଙ୍କ_Params(remaining) != Get_Func_Type_ଅଙ୍କ_Params(inner) - bound_Args_ଗଣନା or
                                            Get_Func_Type_ଅଙ୍କ_Results(remaining) != Get_Func_Type_ଅଙ୍କ_Results(inner));
            ପାଇଁ (u16 i = 0; i < Get_Func_Type_ଅଙ୍କ_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(inner, i) != GetFuncTypeResultType(remaining, i));
            }
            ପାଇଁ (u16 i = 0; i < Get_Func_Type_ଅଙ୍କ_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_Args_ଗଣନା + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        ଅଙ୍କ_Rets = Get_Func_Type_ଅଙ୍କ_Results(inner);

        _throwif(m3Err_wasmMalformed, (ଅଙ୍କ_Rets + bound_Args_ଗଣନା) * c_io_Slot_ଗଣନା > cont->ଅଙ୍କ_ଗଦା_Slots);

        ପାଇଁ (u32 i = 0; i < bound_Args_ଗଣନା; ++i) {
_           (Get_ମୂଲ୍ୟ(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ଗଦା + (ଅଙ୍କ_Rets + i) * c_io_Slot_ଗଣନା));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ଗଦା;
        cont->pc    = cont->entryFunction->compiled;
    } ଅନ୍ୟଥା ଯଦି (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_Args_ଗଣନା));
        _throwif(m3Err_wasmMalformed, is_ମୂଳ and bound_Args_ଗଣନା != 0);
        cont->bound_Args_ଗଣନା = bound_Args_ଗଣନା;

_       (LoadSuspendedBody(l, cont, i_id));

        cont->state = cont_suspended;
    } ଅନ୍ୟଥା ଯଦି (state == snapshot_contFinished) {
_       (GetLEB_u32(l, &bound_Args_ଗଣନା));
        _throwif(m3Err_wasmMalformed, bound_Args_ଗଣନା != 0);
        cont->bound_Args_ଗଣନା = 0;
        cont->state          = cont_consumed;
    } ଅନ୍ୟଥା {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    ଯଦି (result) {
        cont->ଅଙ୍କ_Frames = 0;
    }

    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadContinuations (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u32      ଗଣନା_2  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &ଗଣନା_2));
    _throwif(m3Err_wasmMalformed, ଗଣନା_2 == 0 or ଗଣନା_2 != l->ଅଙ୍କ_Continuations);

    ପାଇଁ (u32 i = 0; i < ଗଣନା_2; ++i) {
_       (LoadContinuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    ଫେରନ୍ତୁ result;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ସ୍ଥିର
M3Result CheckContinuationReferences (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;

    ପାଇଁ (u32 id = 0; id < l->ଅଙ୍କ_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             leaf  = id;
        u32             depth = 0;
#  ଯଦି d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not IsSubTypeOf(RefTypeOfFuncType(cont->type, true), expectedType)));
#  endif
        ଯଦି (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_ContTypeOfRef(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->entryFunction ? child->entryFunction->funcType : ଖାଲି;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_ଅଙ୍କ_Results(actual) != Get_Func_Type_ଅଙ୍କ_Results(expected));
            ପାଇଁ (u16 i = 0; i < Get_Func_Type_ଅଙ୍କ_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(actual, i) != GetFuncTypeResultType(expected, i));
            }
        }
        ଯେତେବେଳେ (l->suspendSites[leaf].target) {
            leaf = l->suspendSites[leaf].target;
            _throwif(m3Err_wasmMalformed, ++depth >= l->ଅଙ୍କ_Continuations or
                                            l->continuations[leaf]->state != cont_suspended);
        }
        ଯଦି (cont->state != cont_suspended or id == 0) {
            ଜାରି;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        ଯଦି (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_Args_ଗଣନା != 0);
            ଜାରି;
        }

        ଅଚଳ M3SnapshotSuspendSite* site      = &l->suspendSites[leaf];
        IM3Continuation              inner     = l->continuations[leaf];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_Args_ଗଣନା > inner->ଅଙ୍କ_Suspend_Results or
                                                                          Get_Func_Type_ଅଙ୍କ_Params(remaining) != inner->ଅଙ୍କ_Suspend_Results - cont->bound_Args_ଗଣନା);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_ଅଙ୍କ_Results(remaining) != Get_Func_Type_ଅଙ୍କ_Results(cont->entryFunction->funcType));
        ପାଇଁ (u16 i = 0; i < Get_Func_Type_ଅଙ୍କ_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, GetFuncTypeResultType(remaining, i) !=
                                            GetFuncTypeResultType(cont->entryFunction->funcType, i));
        }

        ପାଇଁ (u32 i = 0; i < inner->ଅଙ୍କ_Suspend_Results; ++i) {
            ଅଚଳ M3_Slot_ମୂଲ୍ୟ* ମୂଲ୍ୟ = &site->map->ମୂଲ୍ୟଗୁଡ଼ିକ[site->point->first_ମୂଲ୍ୟ +
                                                          site->point->ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ - site->point->ଅଙ୍କ_Results + i];
            ଯଦି (i < cont->bound_Args_ଗଣନା) {
                ଯଦି (IsRefType(ମୂଲ୍ୟ->type) and not IsNullableRef(ମୂଲ୍ୟ->type)) {
                    ଶୂନ୍ୟ* ref = ଖାଲି;
                    memcpy(&ref, inner->sp + ମୂଲ୍ୟ->slot, ଆକାର(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } ଅନ୍ୟଥା {
                _throwif("a captured continuation has the wrong parameter type", ମୂଲ୍ୟ->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_Args_ଗଣନା)));
            }
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

#  ଯଦି d_m3HasExceptionHandling

ସ୍ଥିର
M3Result LoadExceptions (M3SnapshotLoad* l)
{
    M3Result result        = m3Err_none;
    u32      ଅଙ୍କ_Exceptions = 0;

_   (GetLEB_u32(l, &ଅଙ୍କ_Exceptions));
    _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Exceptions == 0 or ଅଙ୍କ_Exceptions != l->ଅଙ୍କ_Exceptions);

    ପାଇଁ (u32 i = 0; i < ଅଙ୍କ_Exceptions; ++i) {
        u32          exnId = 0, ଅଙ୍କ_Args = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &ଅଙ୍କ_Args));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->tag->type;

        _throwif(m3Err_wasmMalformed, ଅଙ୍କ_Args != exn->ଅଙ୍କ_Args);

        ପାଇଁ (u32 a = 0; a < ଅଙ୍କ_Args; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      bits    = 0;
_           (Get_ମୂଲ୍ୟ_Bits(l, argType, &bits));
            ଯଦି (not l->checking) {
                exn->args[a] = bits;
            }
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
ପ୍ରକାରବ୍ୟାଖ୍ୟା ସଂରଚନା M3BoundedReader {
    M3SnapshotLoad* load;
    u64             remaining;
    M3Result        error;
} M3BoundedReader;

ସ୍ଥିର
M3Result BoundedReader_Read (ଶୂନ୍ୟ* o_buffer, size_t i_size, ଶୂନ୍ୟ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    ଯଦି (i_size > br->remaining) {
        br->error = "the host read past the end of its state";
        ଫେରନ୍ତୁ br->error;
    }

    br->remaining -= i_size;

    ଫେରନ୍ତୁ Get(br->load, o_buffer, i_size);
}

ସ୍ଥିର
M3Result LoadHostState (M3SnapshotLoad* l)
{
    M3Result               result = m3Err_none;
    ଅଚଳ M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 size = (size_t)(l->end - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->loadHostState);

    ଯଦି (l->checking) {
        ଫେରନ୍ତୁ SkipBytes(l, size);
    }
    reader.load      = l;
    reader.remaining = size;
    reader.error     = m3Err_none;

_   (hooks->loadHostState(hooks->userdata, BoundedReader_Read, &reader, size));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadHeader (M3SnapshotLoad* l)
{
    M3Result result = m3Err_none;
    u8       magic[ଆକାର(c_snapshotMagic)];
    u32      version = 0;

_   (Get(l, magic, ଆକାର(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, ଆକାର(magic)) != 0);

_   (GetLE32(l, &version));
    _throwif(m3Err_wasmMalformed, version != c_snapshotVersion);

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
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

    (ଶୂନ୍ୟ)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->ଅଙ୍କ_Continuations));
_   (GetLEB_u32(l, &l->ଅଙ୍କ_Exceptions));
_   (GetLEB_u64(l, &memoryBytes));
_   (GetLEB_u64(l, &tableElements));
_   (GetLEB_u32(l, &activeStacks));
    l->requiredStacks        = activeStacks;
    l->requiredMemoryBytes   = memoryBytes;
    l->requiredTableElements = tableElements;
    _throwif(m3Err_wasmMalformed, memoryBytes < currentMemory or tableElements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->ଅଙ୍କ_Memories and memoryBytes != 0);
    _throwif(m3Err_wasmMalformed, not l->module->ଅଙ୍କ_Tables and tableElements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    ଯଦି (*o_flags & d_m3SnapshotFlagPostmortem) {
        ପାଇଁ (u32 i = 0; i < l->ଅଙ୍କ_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
        }
        ଫେରନ୍ତୁ m3Err_none;
    }

    _throwif(m3Err_memoryLimitExceeded, l->runtime->memoryBytesLimit and
                                          memoryBytes - currentMemory > l->runtime->memoryBytesLimit - l->runtime->memoryBytesUsed);
    _throwif(m3Err_tableLimitExceeded, l->runtime->tableElementsLimit and
                                         tableElements - currentTables > l->runtime->tableElementsLimit - l->runtime->tableElementsUsed);
    _throwif(m3Err_continuationLimitExceeded, l->runtime->continuationsLimit and
                                                activeStacks > l->runtime->continuationsLimit - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->ଅଙ୍କ_Continuations == 0 or activeStacks >= l->ଅଙ୍କ_Continuations);
#  ଯଦି !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->ଅଙ୍କ_Exceptions != 0);
#  endif

    ଯଦି (l->checking) {
        ପାଇଁ (u32 i = 0; i < l->ଅଙ୍କ_Exceptions; ++i) {
            u32 tagIndex = 0;
_           (GetLEB_u32(l, &tagIndex));
#  ଯଦି d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, tagIndex >= l->module->ଅଙ୍କ_Tags);
#  endif
        }
        ଫେରନ୍ତୁ m3Err_none;
    }

    l->check_ଗଦା = m3_AllocArray(m3slot_t, d_m3_Continuation_ଗଦା_Slots + 4);
    _throwifnull(l->check_ଗଦା);
    l->frames = m3_AllocArray(M3Frame, d_m3ContinuationMaxFrames);
    _throwifnull(l->frames);

    l->continuations = m3_AllocArray(IM3Continuation, l->ଅଙ୍କ_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->ଅଙ୍କ_Continuations);
    _throwifnull(l->suspendSites);

    ଯଦି (not l->runtime->ମୂଳ_Continuation) {
        IM3Continuation ମୂଳ = Continuation_New(l->runtime, ଖାଲି, ଖାଲି);
        _throwifnull(ମୂଳ);

        // the root borrows the runtime's stack, so it holds none of its own
        ମୂଳ->ଅଙ୍କ_ଗଦା_Slots          = l->runtime->ଅଙ୍କ_ଗଦା_Slots;
        l->runtime->ମୂଳ_Continuation = ମୂଳ;
    }

    l->continuations[0] = l->runtime->ମୂଳ_Continuation;

    ପାଇଁ (u32 i = 1; i < l->ଅଙ୍କ_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, ଖାଲି, ଖାଲି);
        _throwifnull(l->continuations[i]);
    }

#  ଯଦି d_m3HasExceptionHandling
    ଯଦି (l->ଅଙ୍କ_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->ଅଙ୍କ_Exceptions);
        _throwifnull(l->exceptions);
    }

    ପାଇଁ (u32 i = 0; i < l->ଅଙ୍କ_Exceptions; ++i) {
        u32    tagIndex = 0;
        IM3Tag tag;
        u32    ଅଙ୍କ_Args;

_       (GetLEB_u32(l, &tagIndex));
        _throwif(m3Err_wasmMalformed, tagIndex >= l->module->ଅଙ୍କ_Tags);

        tag     = TagOfIndex(l->module, tagIndex);
        ଅଙ୍କ_Args = Get_Func_Type_ଅଙ୍କ_Params(tag->type);

        l->exceptions[i] = NewException(l->runtime, tag, ଅଙ୍କ_Args);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  ଅନ୍ୟଥା
    ପାଇଁ (u32 i = 0; i < l->ଅଙ୍କ_Exceptions; ++i) {
        u32 tagIndex = 0;
_       (GetLEB_u32(l, &tagIndex));
    }
#  endif

_catch:
    ଫେରନ୍ତୁ result;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ସ୍ଥିର
M3Result LoadSection (M3SnapshotLoad* l, u8 i_sectionId)
{
    M3Result result = m3Err_none;
    u32      flags  = 0;
    ବାଛ (i_sectionId) {
    ସ୍ଥିତି d_m3SnapshotSection_Meta:
_       (LoadMeta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        ଭାଙ୍ଗ;

    ସ୍ଥିତି d_m3SnapshotSection_Memory:
_       (LoadMemories(l));
        ଭାଙ୍ଗ;

    ସ୍ଥିତି d_m3SnapshotSection_Table:
_       (LoadTables(l));
        ଭାଙ୍ଗ;

    ସ୍ଥିତି d_m3SnapshotSection_Global:
_       (LoadGlobals(l));
        ଭାଙ୍ଗ;

    ସ୍ଥିତି d_m3SnapshotSection_Segment:
_       (LoadSegments(l));
        ଭାଙ୍ଗ;

#  ଯଦି d_m3HasExceptionHandling
    ସ୍ଥିତି d_m3SnapshotSection_Exception:
_       (LoadExceptions(l));
        ଭାଙ୍ଗ;
#  endif

    ସ୍ଥିତି d_m3SnapshotSection_Continuation:
_       (LoadContinuations(l));
        ଭାଙ୍ଗ;

    ସ୍ଥିତି d_m3SnapshotSection_HostState:
_       (LoadHostState(l));
        ଭାଙ୍ଗ;

    ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
_       (SkipBytes(l, (size_t)(l->end - l->pos)));
        ଭାଙ୍ଗ;
    }

_catch:
    ଫେରନ୍ତୁ result;
}

ସ୍ଥିର
M3Result LoadSections (M3SnapshotLoad* l, bool i_embedded)
{
    M3Result result  = m3Err_none;
    u8*      payload = ଖାଲି;

    ଯଦି (not i_embedded) {
_       (LoadHeader(l));
    }

    ଯେତେବେଳେ (true) {
        u8  sectionId   = 0;
        u32 sectionSize = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        ଯଦି (GetU8(l, &sectionId)) {
            ଭାଙ୍ଗ;
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
             l->ଅଙ୍କ_Exceptions and not l->seenSections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->ଅଙ୍କ_Memories and not l->seenSections[d_m3SnapshotSection_Memory]);
    _throwif("the snapshot has no table section",
             l->module->ଅଙ୍କ_Tables and not l->seenSections[d_m3SnapshotSection_Table]);
    _throwif("the snapshot has no global section",
             l->module->ଅଙ୍କ_Globals and not l->seenSections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->ଅଙ୍କ_Data_Segments or l->module->ଅଙ୍କ_Element_Segments) and
               not l->seenSections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    ଫେରନ୍ତୁ result;
}

#endif // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#ଯଦି d_m3HasSnapshots

ସ୍ଥିର
M3Result SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ଶୂନ୍ୟ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3Result       result = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, ଆକାର(save));

    IM3Continuation ମୂଳ = io_runtime->ମୂଳ_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not ମୂଳ or ମୂଳ->state != cont_suspended;

    IM3Function entry = save.postmortem ? io_runtime->entered : ମୂଳ->entryFunction;

    ଯଦି (o_module) {
        *o_module = ଖାଲି;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not entry);

    save.module = entry->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    ଯଦି (o_module) {
        *o_module = save.module;
    }

    ପାଇଁ (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->next) {
        u32 id;
_       (PointerIds_Add(&save.liveContinuations, cont, &id));
    }

#  ଯଦି d_m3HasExceptionHandling
    ପାଇଁ (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->next) {
        u32 id;
_       (PointerIds_Add(&save.liveExceptions, exception, &id));
    }
#  endif

    ଯଦି (not save.postmortem) {
        u32 id;
_       (PointerIds_Add(&save.continuations, ମୂଳ, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.liveContinuations);
    PointerIds_Free(&save.liveExceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    ଫେରନ୍ତୁ result;
}

#endif

M3Result m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, ଶୂନ୍ୟ* i_userdata)
{
    ଯଦି (not io_runtime or not i_writer) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }

#ଯଦି d_m3HasSnapshots
    ଫେରନ୍ତୁ SaveSnapshot(io_runtime, i_writer, i_userdata, ଖାଲି, false);
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_userdata;
    ଫେରନ୍ତୁ "snapshots are not available in this build of Wasm3";
#endif
}


ସ୍ଥିର
M3Result LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ଶୂନ୍ୟ* i_userdata, bool i_embedded)
{
    M3Result result = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#ଯଦି d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->hasRun);

    {
        M3SnapshotLoad load;
        memset(&load, 0, ଆକାର(load));

        load.runtime  = io_runtime;
        load.module   = i_module;
        load.reader   = i_reader;
        load.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        result = LoadSections(&load, i_embedded);

        IM3Continuation ମୂଳ = io_runtime->ମୂଳ_Continuation;

        ଯଦି (result) {
            ଯଦି (load.changed) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                ଯଦି (ମୂଳ) {
                    ମୂଳ->state     = cont_returned;
                    ମୂଳ->ଅଙ୍କ_Frames = 0;
                }
            }
        } ଅନ୍ୟଥା {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->hasRun        = true;
        }

        Runtime_Place_Call_ଗଦା(io_runtime);

        m3_Free(load.check_ଗଦା);
        m3_Free(load.frames);
        m3_Free(load.suspendSites);
        m3_Free(load.continuations);
#  ଯଦି d_m3HasExceptionHandling
        m3_Free(load.exceptions);
#  endif
    }
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#endif

_catch:
    ଫେରନ୍ତୁ result;
}


M3Result m3_LoadSnapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, ଶୂନ୍ୟ* i_userdata)
{
    ଫେରନ୍ତୁ LoadSnapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


ଶୂନ୍ୟ m3_SetSnapshotHooks (IM3Runtime io_runtime, ଅଚଳ M3SnapshotHooks* i_hooks)
{
#ଯଦି d_m3HasSnapshots
    ଯଦି (io_runtime) {
        ଯଦି (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } ଅନ୍ୟଥା {
            memset(&io_runtime->snapshotHooks, 0, ଆକାର(io_runtime->snapshotHooks));
        }
    }
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)io_runtime;
    (ଶୂନ୍ୟ)i_hooks;
#endif
}


M3Result m3_SaveSnapshotToBuffer (IM3Runtime io_runtime, ଶୂନ୍ୟ** o_bytes, size_t* o_size)
{
    ଯଦି (!io_runtime || !o_bytes || !o_size) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }
#ଯଦି d_m3HasSnapshots
    M3BufferWriter bw     = { ଖାଲି, 0, 0 };
    M3Result       result = m3_SaveSnapshot(io_runtime, BufferWriter_Write, &bw);
    ଯଦି (result) {
        m3_Free(bw.buffer);
        *o_bytes = ଖାଲି;
        *o_size  = 0;
        ଫେରନ୍ତୁ result;
    }
    *o_bytes = bw.buffer;
    *o_size  = bw.size;
    ଫେରନ୍ତୁ m3Err_none;
#ଅନ୍ୟଥା
    *o_bytes = ଖାଲି;
    *o_size  = 0;
    ଫେରନ୍ତୁ "snapshots are not available in this build of Wasm3";
#endif
}

#ଯଦି d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ସ୍ଥିର
u8* EmitLEB_u32 (u8* o_where, u32 i_ମୂଲ୍ୟ)
{
    କର {
        u8 byte = i_ମୂଲ୍ୟ & 0x7F;
        i_ମୂଲ୍ୟ >>= 7;
        ଯଦି (i_ମୂଲ୍ୟ != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } ଯେତେବେଳେ (i_ମୂଲ୍ୟ != 0);

    ଫେରନ୍ତୁ o_where;
}

ସ୍ଥିର
u32 SizeOfLEB_u32 (u32 i_ମୂଲ୍ୟ)
{
    u32 size = 0;

    କର {
        i_ମୂଲ୍ୟ >>= 7;
        ++size;
    } ଯେତେବେଳେ (i_ମୂଲ୍ୟ != 0);

    ଫେରନ୍ତୁ size;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ସ୍ଥିର
M3Result FindEmbeddedSnapshot (ଅଚଳ u8* i_start, ଅଚଳ u8* i_end, cstr_t i_secName,
                               ଅଚଳ u8** o_secStart, ଅଚଳ u8** o_secEnd,
                               ଅଚଳ u8** o_data, u32* o_size)
{
    M3Result result = m3Err_none;
    bytes_t  pos    = i_start;
    bool     found  = false;
    cstr_t   name   = ଖାଲି;

    ଯଦି (o_secStart) {
        *o_secStart = ଖାଲି;
    }
    ଯଦି (o_secEnd) {
        *o_secEnd = ଖାଲି;
    }
    ଯଦି (o_data) {
        *o_data = ଖାଲି;
    }
    ଯଦି (o_size) {
        *o_size = 0;
    }

    _throwif(m3Err_wasmMalformed, i_end - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    ଯେତେବେଳେ (pos < i_end) {
        bytes_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     size      = 0;
_       (ReadLEB_u32(&size, &pos, i_end));
        _throwif(m3Err_wasmUnderrun, size > (size_t)(i_end - pos));
        {
            bytes_t payload = pos;
            pos += size;
            ଯଦି (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                ଯଦି (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", found);
                    found = true;
                    ଯଦି (o_secStart) {
                        *o_secStart = secStart;
                    }
                    ଯଦି (o_secEnd) {
                        *o_secEnd = pos;
                    }
                    ଯଦି (o_data) {
                        *o_data = payload;
                    }
                    ଯଦି (o_size) {
                        *o_size = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    ଫେରନ୍ତୁ result;
}

// Builds the custom section name a snapshot of i_name lives under
ସ୍ଥିର
M3Result SnapshotSectionName (cstr_t i_name, ଅକ୍ଷର* o_secName, size_t i_size)
{
    ଯଦି (i_name and *i_name) {
        ଯଦି (strlen(i_name) > d_m3MaxSnapshotNameLength) {
            ଫେରନ୍ତୁ "the snapshot name is too long";
        }
        snprintf(o_secName, i_size, "snapshot.%s", i_name);
    } ଅନ୍ୟଥା {
        snprintf(o_secName, i_size, "snapshot");
    }

    ଫେରନ୍ତୁ m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ସ୍ଥିର
M3Result ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3Result result = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = ଖାଲି;

    ପାଇଁ (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->next) {
        ଯଦି (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    ଫେରନ୍ତୁ result;
}
#endif

M3Result m3_LoadSnapshotFromBuffer (IM3Runtime io_runtime, IM3Module i_module, ଅଚଳ ଶୂନ୍ୟ* i_bytes, size_t i_size)
{
    ଯଦି (!io_runtime || !i_module || !i_bytes) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }
#ଯଦି d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    ଯଦି (i_size >= 8 && memcmp(i_bytes, "\0asm", 4) == 0) {
        ଅଚଳ u8* data   = ଖାଲି;
        u32       size   = 0;
        M3Result  result = FindEmbeddedSnapshot((ଅଚଳ u8*)i_bytes, (ଅଚଳ u8*)i_bytes + i_size,
                                                "snapshot", ଖାଲି, ଖାଲି, &data, &size);
        ଯଦି (result) {
            ଫେରନ୍ତୁ result;
        }
        ଯଦି (not data) {
            ଫେରନ୍ତୁ "the Wasm binary holds no embedded snapshot";
        }

        {
            M3BufferReader br = { data, size, 0 };
            ଫେରନ୍ତୁ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
        }
    }
    {
        M3BufferReader br = { (ଅଚଳ u8*)i_bytes, i_size, 0 };
        ଫେରନ୍ତୁ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br,
                            i_size < 4 or memcmp(i_bytes, c_snapshotMagic, 4) != 0);
    }
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_size;
    ଫେରନ୍ତୁ "snapshots are not available in this build of Wasm3";
#endif
}

bool m3_HasSnapshot (IM3Module i_module, ଅଚଳ ଅକ୍ଷର* i_name)
{
    ଯଦି (not i_module) {
        ଫେରନ୍ତୁ false;
    }
#ଯଦି d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = ଖାଲି;
    (ଶୂନ୍ୟ)ModuleSnapshot(i_module, i_name, &snap);

    ଫେରନ୍ତୁ snap != ଖାଲି;
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_name;
    ଫେରନ୍ତୁ false;
#endif
}

M3Result m3_GetEmbeddedSnapshot (IM3Module i_module, ଅଚଳ ଅକ୍ଷର* i_name,
                                 ଅଚଳ ଶୂନ୍ୟ** o_bytes, size_t* o_size)
{
    ଯଦି (not i_module or not o_bytes or not o_size) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }

    *o_bytes = ଖାଲି;
    *o_size  = 0;

#ଯଦି d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ଖାଲି;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);

        ଯଦି (result) {
            ଫେରନ୍ତୁ result;
        }
        ଯଦି (not snap) {
            ଫେରନ୍ତୁ "the module holds no embedded snapshot of that name";
        }

        *o_bytes = snap->data;
        *o_size  = snap->size;

        ଫେରନ୍ତୁ m3Err_none;
    }
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_name;
    ଫେରନ୍ତୁ "snapshots are not available in this build of Wasm3";
#endif
}

M3Result m3_LoadEmbeddedSnapshot (IM3Runtime io_runtime, IM3Module i_module, ଅଚଳ ଅକ୍ଷର* i_name)
{
    ଯଦି (not io_runtime or not i_module) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }
#ଯଦି d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = ଖାଲି;
        M3Result            result = ModuleSnapshot(i_module, i_name, &snap);
        M3BufferReader      br;

        ଯଦି (result) {
            ଫେରନ୍ତୁ result;
        }
        ଯଦି (not snap) {
            ଫେରନ୍ତୁ "the module holds no embedded snapshot of that name";
        }

        br.buffer = snap->data;
        br.size   = snap->size;
        br.cursor = 0;

        ଫେରନ୍ତୁ LoadSnapshot(io_runtime, i_module, BufferReader_Read, &br, true);
    }
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)i_name;
    ଫେରନ୍ତୁ "snapshots are not available in this build of Wasm3";
#endif
}


M3Result m3_SaveSnapshotToModule (IM3Runtime io_runtime, IM3Module i_module, ଅଚଳ ଅକ୍ଷର* i_name,
                                  ଶୂନ୍ୟ** o_bytes, size_t* o_size)
{
#ଯଦି d_m3HasSnapshots
    M3Result       result = m3Err_none;
    M3BufferWriter bw     = { ଖାଲି, 0, 0 };
    u8*            out    = ଖାଲି;

    ଅକ୍ଷର      secName[d_m3MaxSnapshotNameLength + ଆକାର("snapshot.")];
    u32       nameLen, nameLenLebSize, payloadSize, payloadSizeLebSize;
    size_t    origSize, totalSize;
    ଅଚଳ u8* dropStart = ଖାଲି;
    ଅଚଳ u8* dropEnd   = ଖାଲି;
    IM3Module saved     = ଖାଲି;
    u8*       p;

    ଯଦି (not io_runtime or not i_module or not o_bytes or not o_size) {
        ଫେରନ୍ତୁ m3Err_mallocFailed;
    }

    *o_bytes = ଖାଲି;
    *o_size  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasmEnd <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, ଆକାର(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->ମୂଳ_Continuation or io_runtime->ମୂଳ_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, BufferWriter_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.size);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.size > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasmEnd, secName, &dropStart, &dropEnd, ଖାଲି, ଖାଲି));

    origSize           = (size_t)(i_module->wasmEnd - i_module->wasmStart) - (dropStart ? (size_t)(dropEnd - dropStart) : 0);
    nameLen            = (u32)strlen(secName);
    nameLenLebSize     = SizeOfLEB_u32(nameLen);
    payloadSize        = nameLenLebSize + nameLen + (u32)bw.size;
    payloadSizeLebSize = SizeOfLEB_u32(payloadSize);
    totalSize          = origSize + 1 + payloadSizeLebSize + payloadSize;

    out = (u8*)m3_Malloc("wasm_snapshot_module", totalSize);
    _throwifnull(out);

    p = out;

    ଯଦି (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasmEnd - dropEnd);

        memcpy(p, i_module->wasmStart, before);
        p += before;
        memcpy(p, dropEnd, after);
        p += after;
    } ଅନ୍ୟଥା {
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
    out      = ଖାଲି;

_catch:
    m3_Free(bw.buffer);
    m3_Free(out);

    ଫେରନ୍ତୁ result;
#ଅନ୍ୟଥା
    (ଶୂନ୍ୟ)io_runtime;
    (ଶୂନ୍ୟ)i_module;
    (ଶୂନ୍ୟ)i_name;
    (ଶୂନ୍ୟ)o_bytes;
    (ଶୂନ୍ୟ)o_size;
    ଫେରନ୍ତୁ "snapshots are not available in this build of Wasm3";
#endif
}
