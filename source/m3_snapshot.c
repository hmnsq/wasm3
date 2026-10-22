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

#यदि d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
स्थिर ध्रुव u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
स्थिर ध्रुव u32 c_snapshot_संस्करणम्  = 1;

गणना {
    d_m3SnapshotSection_Meta         = 0,
    d_m3_Snapshot_Section_स्मृतिः       = 1,
    d_m3_Snapshot_Section_सारणी        = 2,
    d_m3SnapshotSection_Global       = 3,
    d_m3SnapshotSection_Segment      = 4,
    d_m3SnapshotSection_Exception    = 5,
    d_m3SnapshotSection_Continuation = 6,
    d_m3SnapshotSection_HostState    = 7,
};

#  define d_m3SnapshotFlagPostmortem  0x1

// The longest <name> in a "snapshot.<name>" custom section. A name is a label
// the embedder picks, not something the format needs room to grow.
#  define d_m3_Max_Snapshot_Name_दीर्घता   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
गणना {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_परिसमाप्तम्  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_अन्तः                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//तटस्थः
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
स्थिर
M3_परिणाम ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_परिणाम परिणाम = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasmStart or i_module->wasm_अन्तः - i_module->wasmStart < 8);

    {
        अष्टकानि_t  pos = i_module->wasmStart + 8;
        cbytes_t अन्तः = i_module->wasm_अन्तः;

        Xxh64_Init(&hash, 0);

        यावत् (pos < अन्तः) {
            अष्टकानि_t start     = pos;
            u8      sectionId = *pos++;
            u32     आकार      = 0;

_           (ReadLEB_u32(&आकार, &pos, अन्तः));
            _throwif(m3Err_wasmUnderrun, आकार > (size_t)(अन्तः - pos));

            pos += आकार;
            यदि (sectionId != 0) {
                Xxh64_Update(&hash, start, (size_t)(pos - start));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    प्रत्यागम परिणाम;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

स्थिर
ध्रुव M3SafePoint* FindSafePoint (IM3Function i_function, pc_t i_pc, u8 i_विधा)
{
    ध्रुव M3SnapshotMap* map = i_function->snapshotMap;

    यदि (map) {
        प्रति (u32 i = 0; i < map->अङ्कः_Safe_Points; ++i) {
            यदि (map->safePoints[i].pc == i_pc and map->safePoints[i].विधा == i_विधा) {
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
ध्रुव M3SafePoint* FindSafePointAt (ध्रुव M3SnapshotMap* i_map, u32 i_wasm_विस्थापनम्, u8 i_विधा, u32 i_targetLoop)
{
    u32 नीचः  = 0;
    u32 उच्चः = i_map->अङ्कः_Safe_Points;

    यावत् (नीचः < उच्चः) {
        u32 मध्यः = नीचः + (उच्चः - नीचः) / 2;

        यदि (i_map->safePoints[मध्यः].wasm_विस्थापनम् < i_wasm_विस्थापनम्) {
            नीचः = मध्यः + 1;
        } अन्यथा {
            उच्चः = मध्यः;
        }
    }

    प्रति (u32 i = नीचः; i < i_map->अङ्कः_Safe_Points and i_map->safePoints[i].wasm_विस्थापनम् == i_wasm_विस्थापनम्; ++i) {
        ध्रुव M3SafePoint* point = &i_map->safePoints[i];

        यदि (point->विधा != i_विधा or (point->flags & d_m3_Safe_Point_पुच्छ_Call)) {
            अनुवर्त;
        }
        यदि (i_विधा != safepoint_op or i_map->खण्डानि[point->aux].wasm_विस्थापनम् == i_targetLoop) {
            प्रत्यागम point;
        }
    }

    प्रत्यागम रिक्त;
}

स्थिर
ध्रुव M3_खण्डम्_Start* Find_खण्डम्_At (ध्रुव M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_विस्थापनम्)
{
    प्रति (u32 i = 0; i < i_map->अङ्कः_खण्डानि; ++i) {
        यदि (i_map->खण्डानि[i].opcode == i_opcode and i_map->खण्डानि[i].wasm_विस्थापनम् == i_wasm_विस्थापनम्) {
            प्रत्यागम &i_map->खण्डानि[i];
        }
    }

    प्रत्यागम रिक्त;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
स्थिर अन्तःस्थ
bool Encloses_विस्थापनम् (ध्रुव M3_खण्डम्_Start* i_खण्डम्, u32 i_wasm_विस्थापनम्)
{
    प्रत्यागम i_खण्डम्->wasm_विस्थापनम् < i_wasm_विस्थापनम् and i_wasm_विस्थापनम् < i_खण्डम्->wasm_अन्तः;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
स्थिर अन्तःस्थ
bool Handlers_जीवितम्_At (ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, ध्रुव M3_खण्डम्_Start* i_try)
{
    प्रत्यागम not (i_point->विधा == safepoint_op and i_try->wasm_विस्थापनम् > i_map->खण्डानि[i_point->aux].wasm_विस्थापनम्);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
स्थिर
M3_परिणाम CallSiteType (IM3Function i_caller, ध्रुव M3SafePoint* i_point, IM3FuncType* o_type)
{
    M3_परिणाम   परिणाम = m3Err_none;
    IM3Module  module = i_caller->module;
    अष्टकानि_t    pos    = i_caller->wasm;
    cbytes_t   अन्तः    = i_caller->wasm_अन्तः;
    u32        आकार   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = रिक्त;

_   (ReadLEB_u32(&आकार, &pos, अन्तः));
    _throwif(m3Err_wasmUnderrun, i_point->wasm_विस्थापनम् >= (size_t)(अन्तः - pos));
    pos += i_point->wasm_विस्थापनम्;

_   (Read_opcode(&opcode, &pos, अन्तः));
_   (ReadLEB_u32(&index, &pos, अन्तः));

    यदि (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->अङ्कः_Functions);
        *o_type = module->functions[index].funcType;
    } अन्यथा {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_परोक्षम् and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->अङ्कः_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    प्रत्यागम परिणाम;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
स्थिर
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 अङ्कः_Results = Get_Func_Type_अङ्कः_Results(i_expected);

    यदि (not i_type or Get_Func_Type_अङ्कः_Results(i_type) != अङ्कः_Results) {
        प्रत्यागम false;
    }

    प्रति (u16 i = 0; i < अङ्कः_Results; ++i) {
        यदि (not Is_Sub_Type_सम्बन्धि(Get_Func_Type_परिणाम_Type(i_type, i), Get_Func_Type_परिणाम_Type(i_expected, i))) {
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
#  यदि d_m3HasFloat
    f64* fp0;
#  endif
} M3FrameRegisters;

// The stack a continuation's frames are on: the runtime's own for the root
स्थिर
M3_परिणाम Continuation_राशिः (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_आधारः, u32* o_अङ्कः_Slots)
{
    यदि (i_cont == i_runtime->मूलम्_Continuation) {
        *o_आधारः     = (m3slot_t*)i_runtime->origin_राशिः;
        *o_अङ्कः_Slots = i_runtime->अङ्कः_राशिः_Slots;
    } अन्यथा {
        *o_आधारः     = i_cont->val_राशिः;
        *o_अङ्कः_Slots = i_cont->अङ्कः_राशिः_Slots;
    }

    प्रत्यागम (*o_आधारः) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
स्थिर
M3_परिणाम Locate_मानम् (m3slot_t* i_आधारः, u32 i_अङ्कः_Slots, m3stack_t i_sp, ध्रुव M3_Slot_मानम्* i_मानम्, u8** o_where)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       type   = आधारः_Type_सम्बन्धि(i_मानम्->type);

    *o_where = रिक्त;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_पञ्जिकाकोशः_Slot_Alias(i_मानम्->slot));

    यदि (not Is_पञ्जिकाकोशः_Slot_Alias(i_मानम्->slot)) {
        size_t आकार   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? परिमाण(शून्य*)
                                                                   : (Is64BitType(type) ? परिमाण(u64) : परिमाण(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_आधारः + i_मानम्->slot) * परिमाण(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + आकार > (size_t)i_अङ्कः_Slots * परिमाण(m3slot_t));

        *o_where = (u8*)i_आधारः + offset;
    }

_catch:
    प्रत्यागम परिणाम;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
स्थिर
M3_परिणाम Read_मानम् (u8* i_where, ध्रुव M3FrameRegisters* i_registers, ध्रुव M3_Slot_मानम्* i_मानम्, u64* o_द्वयङ्काः,
                    शून्य** o_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       type   = आधारः_Type_सम्बन्धि(i_मानम्->type);

    *o_द्वयङ्काः      = 0;
    *o_reference = रिक्त;

    यदि (i_where) {
        यदि (IsRefType(type)) {
            स्मृतिं_प्रतिलिख(o_reference, i_where, परिमाण(शून्य*));
        } अन्यथा यदि (Is64BitType(type)) {
            स्मृतिं_प्रतिलिख(o_द्वयङ्काः, i_where, परिमाण(u64));
        } अन्यथा {
            u32 narrow;
            स्मृतिं_प्रतिलिख(&narrow, i_where, परिमाण(narrow));
            *o_द्वयङ्काः = narrow;
        }
    } अन्यथा यदि (Is_Fp_पञ्जिकाकोशः_Slot_Alias(i_मानम्->slot)) {
#  यदि d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        यदि (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 द्वयङ्काः;
            स्मृतिं_प्रतिलिख(&द्वयङ्काः, &narrow, परिमाण(द्वयङ्काः));
            *o_द्वयङ्काः = द्वयङ्काः;
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            स्मृतिं_प्रतिलिख(o_द्वयङ्काः, i_registers->fp0, परिमाण(u64));
        }
#  अन्यथा
        _throw(m3Err_wasmMalformed);
#  endif
    } अन्यथा {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        यदि (IsRefType(type)) {
            *o_reference = (शून्य*)(uintptr_t)r0;
        } अन्यथा यदि (Is64BitType(type)) {
            *o_द्वयङ्काः = (u64)r0;
        } अन्यथा {
            *o_द्वयङ्काः = (u32)r0;
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Write_मानम् (u8* o_where, ध्रुव M3FrameRegisters* i_registers, ध्रुव M3_Slot_मानम्* i_मानम्, u64 i_द्वयङ्काः,
                     शून्य* i_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       type   = आधारः_Type_सम्बन्धि(i_मानम्->type);

    यदि (o_where) {
        यदि (IsRefType(type)) {
            स्मृतिं_प्रतिलिख(o_where, &i_reference, परिमाण(शून्य*));
        } अन्यथा यदि (Is64BitType(type)) {
            स्मृतिं_प्रतिलिख(o_where, &i_द्वयङ्काः, परिमाण(u64));
        } अन्यथा {
            u32 narrow = (u32)i_द्वयङ्काः;
            स्मृतिं_प्रतिलिख(o_where, &narrow, परिमाण(narrow));
        }
    } अन्यथा यदि (Is_Fp_पञ्जिकाकोशः_Slot_Alias(i_मानम्->slot)) {
#  यदि d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        यदि (type == c_m3Type_f32) {
            u32 द्वयङ्काः = (u32)i_द्वयङ्काः;
            f32 narrow;
            स्मृतिं_प्रतिलिख(&narrow, &द्वयङ्काः, परिमाण(narrow));
            *i_registers->fp0 = narrow;
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            स्मृतिं_प्रतिलिख(i_registers->fp0, &i_द्वयङ्काः, परिमाण(u64));
        }
#  अन्यथा
        _throw(m3Err_wasmMalformed);
#  endif
    } अन्यथा {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        यदि (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } अन्यथा {
            *i_registers->r0 = (m3reg_t)i_द्वयङ्काः;
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
स्थिर
ध्रुव M3_Slot_मानम्* Frame_मानम् (ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, u32 i_सूचकांक)
{
    यदि (i_सूचकांक < i_map->अङ्कः_Locals) {
        प्रत्यागम i_point->first_Local_मानम् == UINT32_MAX ? &i_map->locals[i_सूचकांक]
                                                      : &i_map->मानानि[i_point->first_Local_मानम् + i_सूचकांक];
    }
    प्रत्यागम &i_map->मानानि[i_point->first_मानम् + i_सूचकांक - i_map->अङ्कः_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
स्थिर
शून्य RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    यदि (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->अङ्कः_Ret_And_Arg_Slots) + i_function->अङ्कः_Local_अष्टकानि;

        स्मृतिं_प्रतिलिख(where, i_function->constants, i_function->अङ्कः_Constant_अष्टकानि);
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
    u32    क्षमता;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  सूचकांक_क्षमता;       // a power of two, kept at least twice count
} M3PointerIds;

स्थिर
u32 PointerBucket (शून्य* i_pointer, u32 i_क्षमता)
{
    u64 द्वयङ्काः = (u64)(uintptr_t)i_pointer;

    प्रत्यागम (u32)((द्वयङ्काः * 0x9E3779B97F4A7C15ULL) >> 32) & (i_क्षमता - 1);
}

स्थिर
u32 PointerIds_Find (ध्रुव M3PointerIds* i_ids, शून्य* i_pointer)
{
    यदि (i_ids->सूचकांक_क्षमता) {
        u32 bucket = PointerBucket(i_pointer, i_ids->सूचकांक_क्षमता);

        यावत् (i_ids->index[bucket]) {
            u32 id = i_ids->index[bucket] - 1;

            यदि (i_ids->वस्तूनि[id] == i_pointer) {
                प्रत्यागम id;
            }

            bucket = (bucket + 1) & (i_ids->सूचकांक_क्षमता - 1);
        }
    }

    प्रत्यागम d_m3SnapshotNone;
}

स्थिर
M3_परिणाम Pointer_Ids_सङ्कलय (M3PointerIds* io_ids, शून्य* i_pointer, u32* o_id)
{
    u32 id = PointerIds_Find(io_ids, i_pointer);

    यदि (id != d_m3SnapshotNone) {
        *o_id = id;
        प्रत्यागम m3Err_none;
    }

    यदि (io_ids->गणनम् == io_ids->क्षमता) {
        u32    क्षमता = io_ids->क्षमता ? io_ids->क्षमता * 2 : 16;
        शून्य** वस्तूनि    = m3_ReallocArray(शून्य*, io_ids->वस्तूनि, क्षमता, io_ids->क्षमता);

        यदि (not वस्तूनि) {
            प्रत्यागम m3Err_mallocFailed;
        }

        io_ids->वस्तूनि    = वस्तूनि;
        io_ids->क्षमता = क्षमता;
    }

    यदि ((io_ids->गणनम् + 1) * 2 > io_ids->सूचकांक_क्षमता) {
        u32  क्षमता = io_ids->सूचकांक_क्षमता ? io_ids->सूचकांक_क्षमता * 2 : 32;
        u32* index    = m3_AllocArray(u32, क्षमता);

        यदि (not index) {
            प्रत्यागम m3Err_mallocFailed;
        }

        प्रति (u32 i = 0; i < io_ids->गणनम्; ++i) {
            u32 bucket = PointerBucket(io_ids->वस्तूनि[i], क्षमता);

            यावत् (index[bucket]) {
                bucket = (bucket + 1) & (क्षमता - 1);
            }

            index[bucket] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->सूचकांक_क्षमता = क्षमता;
    }

    id                = io_ids->गणनम्++;
    io_ids->वस्तूनि[id] = i_pointer;

    u32 bucket = PointerBucket(i_pointer, io_ids->सूचकांक_क्षमता);

    यावत् (io_ids->index[bucket]) {
        bucket = (bucket + 1) & (io_ids->सूचकांक_क्षमता - 1);
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
    स्मृतिं_पूरय(io_ids, 0, परिमाण(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

प्रकारनिर्देश संरचना M3_बफर_Writer {
    u8*    बफर;
    size_t आकार;
    size_t क्षमता;
} M3_बफर_Writer;

स्थिर
M3_परिणाम बफर_Writer_Write (ध्रुव शून्य* i_दत्त, size_t i_आकार, शून्य* i_userdata)
{
    M3_बफर_Writer* bw = (M3_बफर_Writer*)i_userdata;

    यदि (bw->आकार + i_आकार > bw->क्षमता) {
        size_t newCap = bw->क्षमता ? bw->क्षमता * 2 : 4096;
        यावत् (bw->आकार + i_आकार > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->बफर, newCap, bw->क्षमता);
        यदि (!newBuf) {
            प्रत्यागम m3Err_mallocFailed;
        }
        bw->बफर   = newBuf;
        bw->क्षमता = newCap;
    }

    यदि (i_आकार) {
        स्मृतिं_प्रतिलिख(bw->बफर + bw->आकार, i_दत्त, i_आकार);
        bw->आकार += i_आकार;
    }

    प्रत्यागम m3Err_none;
}

प्रकारनिर्देश संरचना M3_बफर_Reader {
    ध्रुव u8* बफर;
    size_t    आकार;
    size_t    cursor;
} M3_बफर_Reader;

स्थिर
M3_परिणाम बफर_Reader_Read (शून्य* o_बफर, size_t i_आकार, शून्य* i_userdata)
{
    M3_बफर_Reader* br = (M3_बफर_Reader*)i_userdata;

    यदि (i_आकार > br->आकार - br->cursor) {
        प्रत्यागम m3Err_wasmMalformed;
    }

    यदि (i_आकार) {
        स्मृतिं_प्रतिलिख(o_बफर, br->बफर + br->cursor, i_आकार);
        br->cursor += i_आकार;
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

    M3PointerIds जीवितम्_Continuations;     // everything the runtime still holds, to check
    M3PointerIds जीवितम्_Exceptions;        //   a reference against before following it
    M3PointerIds continuations;         // what the snapshot carries, by id
    M3PointerIds exceptions;
} M3SnapshotSave;

स्थिर अन्तःस्थ
u8 M3_Type_गन्तव्यम्_Val_Type (m3type_t i_type)
{
    विकल्प (आधारः_Type_सम्बन्धि(i_type)) {
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
m3type_t Val_Type_गन्तव्यम्_M3_Type (u8 i_valtype)
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
M3_परिणाम Put (M3SnapshotSave* s, ध्रुव शून्य* i_दत्त, size_t i_आकार)
{
    प्रत्यागम s->writing ? s->writer(i_दत्त, i_आकार, s->userdata) : m3Err_none;
}

स्थिर
M3_परिणाम PutU8 (M3SnapshotSave* s, u8 i_मानम्)
{
    प्रत्यागम Put(s, &i_मानम्, परिमाण(i_मानम्));
}


स्थिर
M3_परिणाम PutLEB_u32 (M3SnapshotSave* s, u32 i_मानम्)
{
    u8  अष्टकानि[5];
    u32 len = 0;

    कुरु {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        यदि (i_मानम् != 0) {
            byte |= 0x80;
        }
        अष्टकानि[len++] = byte;
    } यावत् (i_मानम् != 0);

    प्रत्यागम Put(s, अष्टकानि, len);
}

स्थिर
M3_परिणाम PutLEB_u64 (M3SnapshotSave* s, u64 i_मानम्)
{
    u8  अष्टकानि[10];
    u32 len = 0;

    कुरु {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        यदि (i_मानम् != 0) {
            byte |= 0x80;
        }
        अष्टकानि[len++] = byte;
    } यावत् (i_मानम् != 0);

    प्रत्यागम Put(s, अष्टकानि, len);
}

स्थिर
M3_परिणाम PutLEB_i64 (M3SnapshotSave* s, i64 i_मानम्)
{
    u8   अष्टकानि[10];
    u32  len  = 0;
    bool अधिकम् = true;

    यावत् (अधिकम्) {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        यदि ((i_मानम् == 0 && !(byte & 0x40)) || (i_मानम् == -1 && (byte & 0x40))) {
            अधिकम् = false;
        } अन्यथा {
            byte |= 0x80;
        }
        अष्टकानि[len++] = byte;
    }

    प्रत्यागम Put(s, अष्टकानि, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
प्रकारनिर्देश M3_परिणाम (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
स्थिर
M3_परिणाम SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_परिणाम         परिणाम   = m3Err_none;
    M3_बफर_Writer   bw       = { रिक्त, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    शून्य*            userdata = s->userdata;

    s->writer   = बफर_Writer_Write;
    s->userdata = &bw;

    परिणाम = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    यदि (परिणाम) {
        गच्छ _catch;
    }

    // a section with nothing in it is left out
    यदि (bw.आकार) {
        _throwif("snapshot section is too large", bw.आकार > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.आकार));
_       (Put(s, bw.बफर, bw.आकार));
    }

_catch:
    m3_Free(bw.बफर);

    प्रत्यागम परिणाम;
}

स्थिर
u32 Function_सूचकांक (IM3Module i_module, IM3Function i_function)
{
    यदि (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->अङ्कः_Functions) {
        प्रत्यागम (u32)(i_function - i_module->functions);
    }

    प्रत्यागम d_m3SnapshotNone;
}

स्थिर
u32 Func_Type_सूचकांक (IM3Module i_module, IM3FuncType i_type)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Func_Types; ++i) {
        यदि (i_module->funcTypes[i] == i_type) {
            प्रत्यागम i;
        }
    }

    प्रत्यागम d_m3SnapshotNone;
}

#  यदि d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
स्थिर
IM3_चिह्नकम् चिह्नकम्_सम्बन्धि_सूचकांक (IM3Module i_module, u32 i_सूचकांक)
{
    IM3_चिह्नकम् चिह्नकम् = &i_module->चिह्नकानि[i_सूचकांक];

    प्रत्यागम चिह्नकम्->resolved ? चिह्नकम्->resolved : चिह्नकम्;
}

स्थिर
u32 चिह्नकम्_सूचकांक_2 (IM3Module i_module, IM3_चिह्नकम् i_चिह्नकम्)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_चिह्नकानि; ++i) {
        यदि (चिह्नकम्_सम्बन्धि_सूचकांक(i_module, i) == i_चिह्नकम्) {
            प्रत्यागम i;
        }
    }

    प्रत्यागम d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
स्थिर
M3_परिणाम NameReference (M3SnapshotSave* s, u8 i_type, शून्य* i_reference, u64* o_word)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_word = d_m3SnapshotNullRef;

    यदि (not i_reference) {
        प्रत्यागम m3Err_none;
    }

    यदि (i_type == c_m3Type_funcref) {
        id = Function_सूचकांक(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } अन्यथा यदि (i_type == c_m3Type_externref) {
        ध्रुव M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_word = name;
        गच्छ _catch;
    }
#  यदि d_m3HasExceptionHandling
    अन्यथा यदि (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 PointerIds_Find(&s->जीवितम्_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 गणनम् = s->exceptions.गणनम्;
_       (Pointer_Ids_सङ्कलय(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.गणनम् != गणनम्);
    }
#  endif
    अन्यथा यदि (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 PointerIds_Find(&s->जीवितम्_Continuations, i_reference) == d_m3SnapshotNone);

        u32 गणनम् = s->continuations.गणनम्;
_       (Pointer_Ids_सङ्कलय(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.गणनम् != गणनम्);
    } अन्यथा {
        _throw("a reference of an unknown type");
    }

    *o_word = id;

_catch:
    प्रत्यागम परिणाम;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
स्थिर
M3_परिणाम Put_Ref_मानम् (M3SnapshotSave* s, u8 i_type, शून्य* i_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u64      word   = 0;

_   (NameReference(s, i_type, i_reference, &word));

    यदि (not s->writing) {
        प्रत्यागम m3Err_none;
    }

    यदि (word == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } अन्यथा {
        u8 विधा = 0;
        यदि (i_type == c_m3Type_funcref) {
            विधा = 0x01;
        } अन्यथा यदि (i_type == c_m3Type_externref) {
            विधा = 0x02;
        }
#  यदि d_m3HasExceptionHandling
        अन्यथा यदि (i_type == c_m3Type_exnref) {
            विधा = 0x03;
        }
#  endif
        अन्यथा यदि (i_type == c_m3Type_contref) {
            विधा = 0x04;
        }

_       (PutU8(s, विधा));
_       (PutLEB_u64(s, word));
    }

_catch:
    प्रत्यागम परिणाम;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
स्थिर
M3_परिणाम PutLE32 (M3SnapshotSave* s, u32 i_मानम्)
{
    u8 अष्टकानि[4];

    प्रति (u32 i = 0; i < परिमाण(अष्टकानि); ++i) {
        अष्टकानि[i] = (u8)(i_मानम् >> (8 * i));
    }

    प्रत्यागम Put(s, अष्टकानि, परिमाण(अष्टकानि));
}

स्थिर
M3_परिणाम PutLE64 (M3SnapshotSave* s, u64 i_मानम्)
{
    u8 अष्टकानि[8];

    प्रति (u32 i = 0; i < परिमाण(अष्टकानि); ++i) {
        अष्टकानि[i] = (u8)(i_मानम् >> (8 * i));
    }

    प्रत्यागम Put(s, अष्टकानि, परिमाण(अष्टकानि));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
स्थिर
M3_परिणाम Put_मानम्_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_द्वयङ्काः)
{
    यदि (IsRefType(i_type)) {
        प्रत्यागम Put_Ref_मानम्(s, आधारः_Type_सम्बन्धि(i_type), (शून्य*)(uintptr_t)i_द्वयङ्काः);
    }

    विकल्प (आधारः_Type_सम्बन्धि(i_type)) {
    पक्ष c_m3Type_i32: प्रत्यागम PutLEB_i64(s, (i32)(u32)i_द्वयङ्काः);
    पक्ष c_m3Type_i64: प्रत्यागम PutLEB_i64(s, (i64)i_द्वयङ्काः);
    पक्ष c_m3Type_f32: प्रत्यागम PutLE32(s, (u32)i_द्वयङ्काः);
    पक्ष c_m3Type_f64: प्रत्यागम PutLE64(s, i_द्वयङ्काः);
    स्वभाव: प्रत्यागम "a snapshot cannot hold a value of this type as bits";
    }
}

स्थिर
M3_परिणाम Put_मानम्_चिह्नकम् (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_गन्तव्यम्_Val_Type(i_type);

    यदि (not valtype) {
        प्रत्यागम "a snapshot has no tag for a value of this type";
    }

    प्रत्यागम PutU8(s, valtype);
}

स्थिर
M3_परिणाम Put_मानम्_द्वयङ्काः (M3SnapshotSave* s, m3type_t i_type, u64 i_द्वयङ्काः)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (Put_मानम्_चिह्नकम्(s, i_type));
_   (Put_मानम्_Body(s, i_type, i_द्वयङ्काः));

_catch:
    प्रत्यागम परिणाम;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
स्थिर
M3_परिणाम Put_मानम् (M3SnapshotSave* s, m3type_t i_type, ध्रुव शून्य* i_where)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (Put_मानम्_चिह्नकम्(s, i_type));

    यदि (IsRefType(i_type)) {
        शून्य* reference;
        स्मृतिं_प्रतिलिख(&reference, i_where, परिमाण(reference));
_       (Put_मानम्_Body(s, i_type, (u64)(uintptr_t)reference));
    } अन्यथा यदि (आधारः_Type_सम्बन्धि(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 अष्टकानि[16];
        स्मृतिं_प्रतिलिख(अष्टकानि, i_where, परिमाण(अष्टकानि));
_       (Put(s, अष्टकानि, परिमाण(अष्टकानि)));
    } अन्यथा यदि (Is64BitType(i_type)) {
        u64 द्वयङ्काः;
        स्मृतिं_प्रतिलिख(&द्वयङ्काः, i_where, परिमाण(द्वयङ्काः));
_       (Put_मानम्_Body(s, i_type, द्वयङ्काः));
    } अन्यथा {
        u32 narrow;
        स्मृतिं_प्रतिलिख(&narrow, i_where, परिमाण(narrow));
_       (Put_मानम्_Body(s, i_type, narrow));
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Put_स्मृतिः_Chunks (M3SnapshotSave* s, ध्रुव u8* i_अष्टकानि, size_t i_आकार)
{
    M3_परिणाम परिणाम = m3Err_none;
    size_t   cursor = 0;

    यावत् (cursor < i_आकार) {
        u8     byte = i_अष्टकानि[cursor];
        size_t सततम्  = 1;

        यावत् (cursor + सततम् < i_आकार and i_अष्टकानि[cursor + सततम्] == byte) {
            सततम्++;
        }

        यदि ((byte == 0x00 or byte == 0xFF) and सततम् >= d_m3_Snapshot_सततम्_देहली) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            यदि (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)सततम्));
            }

            cursor += सततम्;
            अनुवर्त;
        }

        size_t rawStart = cursor;

        यावत् (cursor < i_आकार) {
            u8     b      = i_अष्टकानि[cursor];
            size_t दीर्घता = 1;

            यावत् (cursor + दीर्घता < i_आकार and i_अष्टकानि[cursor + दीर्घता] == b) {
                दीर्घता++;
            }

            यदि ((b == 0x00 or b == 0xFF) and दीर्घता >= d_m3_Snapshot_सततम्_देहली) {
                भङ्ग;
            }

            cursor += दीर्घता;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)rawStart));
_       (PutLEB_u32(s, (u32)(cursor - rawStart)));
_       (Put(s, i_अष्टकानि + rawStart, cursor - rawStart));
    }

_   (PutU8(s, d_m3_Chunk_अन्तः));

_catch:
    प्रत्यागम परिणाम;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
स्थिर
u32 स्मृतिः_First_सूचकांक (IM3Module module, u32 index)
{
    u32 j = 0;
    यावत् (module->memories[j] != module->memories[index]) {
        ++j;
    }
    प्रत्यागम j;
}

स्थिर
u32 सारणी_First_सूचकांक (IM3Module module, u32 index)
{
    u32 j = 0;
    यावत् (module->tables[j] != module->tables[index]) {
        ++j;
    }
    प्रत्यागम j;
}

स्थिर
M3_परिणाम SaveMemories (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_Memories));

    प्रति (u32 m = 0; m < module->अङ्कः_Memories; ++m) {
        IM3_स्मृतिः स्मृतिः   = module->memories[m];
        u32       first    = स्मृतिः_First_सूचकांक(module, m);
        bool      has_दत्त  = स्मृतिः and स्मृतिः->mallocated;
        u32       page_आकार = स्मृतिः ? स्मृतिः_Page_आकार(स्मृतिः) : d_m3_Default_Mem_Page_आकार;
        u64       अष्टकानि    = स्मृतिः ? स्मृतिः->अङ्कः_पृष्ठानि * (u64)page_आकार : 0;
        u32       page_द्वयङ्काः = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, first));

        यदि (first != m) {
            अनुवर्त;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", स्मृतिः and स्मृतिः->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", अष्टकानि > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 स्मृतिः and स्मृतिः->mallocated and स्मृतिः->mallocated->दीर्घता < अष्टकानि);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        यावत् ((1u << page_द्वयङ्काः) < page_आकार) {
            ++page_द्वयङ्काः;
        }

_       (PutLEB_u32(s, page_द्वयङ्काः));
_       (PutLEB_u32(s, स्मृतिः ? (u32)स्मृतिः->अङ्कः_पृष्ठानि : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        यदि (not s->writing) {
            अनुवर्त;
        }

        यदि (has_दत्त) {
_           (Put_स्मृतिः_Chunks(s, m3_Mem_दत्त(स्मृतिः->mallocated), (size_t)अष्टकानि));
        } अन्यथा {
_           (PutU8(s, d_m3_Chunk_अन्तः));
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम SaveGlobals (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_Globals));

    प्रति (u32 g = 0; g < module->अङ्कः_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_मानम्(s, global->type, &cell->i64_मानम्));
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम SaveTables (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_Tables));

    प्रति (u32 t = 0; t < module->अङ्कः_Tables; ++t) {
        IM3_सारणी सारणी_2 = module->tables[t];
        u32      first = सारणी_First_सूचकांक(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, first));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        यदि (first != t) {
            अनुवर्त;
        }

_       (PutU8(s, M3_Type_गन्तव्यम्_Val_Type(सारणी_2->type)));
_       (PutLEB_u32(s, सारणी_2->आकार));

        प्रति (u32 e = 0; e < सारणी_2->आकार; ++e) {
_           (Put_Ref_मानम्(s, आधारः_Type_सम्बन्धि(सारणी_2->type), सारणी_2->elements[e]));
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम SaveSegments (M3SnapshotSave* s)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->अङ्कः_दत्त_Segments));

    प्रति (u32 i = 0; i < module->अङ्कः_दत्त_Segments; ++i) {
_       (PutU8(s, module->दत्त_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->अङ्कः_Element_Segments));

    प्रति (u32 i = 0; i < module->अङ्कः_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    प्रत्यागम परिणाम;
}

#  यदि d_m3HasExceptionHandling

स्थिर
M3_परिणाम SaveExceptions (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.गणनम्));

    प्रति (u32 i = 0; i < s->exceptions.गणनम्; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.वस्तूनि[i];
        IM3FuncType  type = exn->चिह्नकम्->type;

        _throwif(m3Err_wasmMalformed, exn->अङ्कः_तर्काः != Get_Func_Type_अङ्कः_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->अङ्कः_तर्काः));

        प्रति (u32 a = 0; a < exn->अङ्कः_तर्काः; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_मानम्_द्वयङ्काः(s, argType, exn->तर्काः[a]));
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
प्रकारनिर्देश संरचना M3Activation {
    IM3Function        function;
    ध्रुव M3SafePoint* point;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 विधा;            // M3SafePointKind
    M3FrameRegisters   registers;       // the ones it goes on with
    ध्रुव M3Frame*     resume;          // the resume it waits in, for safepoint_resume
    i32                firstFrame;      // its frames, outermost first: firstFrame down to lastFrame
    i32                चरमः_Frame;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
स्थिर
ध्रुव M3SafePoint* FindFrameSafePoint (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_विधा)
{
    ध्रुव M3SafePoint* point = *io_function ? FindSafePoint(*io_function, i_pc, i_विधा) : रिक्त;

    प्रति (u32 i = 0; not point and i < i_module->अङ्कः_Functions; ++i) {
        point = FindSafePoint(&i_module->functions[i], i_pc, i_विधा);
        यदि (point) {
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
M3_परिणाम अग्र_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_first, M3Activation* o_act)
{
    M3_परिणाम परिणाम = m3Err_none;
    i32      i      = i_first;

    स्मृतिं_पूरय(o_act, 0, परिमाण(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->firstFrame = i_first;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    प्रति (; i >= 0; --i) {
        ध्रुव M3Frame* frame = &i_cont->चौकटाः[i];

        _throwif("a suspended frame runs against another module's memory", frame->स्मृतिः != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", frame->sp != i_sp);

        यदि (frame->विधा == frame_call) {
            o_act->pc           = frame->pc;
            o_act->विधा         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&frame->call.r0;
#  यदि d_m3HasFloat
            o_act->registers.fp0 = (f64*)&frame->call.fp0;
#  endif
            भङ्ग;
        } अन्यथा यदि (frame->विधा == frame_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = frame->pc;
            o_act->विधा   = safepoint_resume;
            o_act->resume = frame;
            भङ्ग;
        }
    }

    यदि (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->विधा         = i_cont->suspendPoint;
        o_act->registers.r0 = &i_cont->r0;
#  यदि d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->चरमः_Frame = i;
    o_act->point     = FindFrameSafePoint(i_module, &o_act->function, o_act->pc, o_act->विधा);

    _throwif("a suspended frame is not at a safepoint", not o_act->point);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  यदि d_m3_प्रविष्टिः_Keeps_Frame
    प्रति (i32 k = i_first; k > o_act->चरमः_Frame; --k) {
        ध्रुव M3Frame* frame = &i_cont->चौकटाः[k];
        _throwif("a suspended frame is not the function it is entered with",
                 frame->विधा == frame_प्रविष्टिः and frame->प्रविष्टिः.function != o_act->function);
    }
#  endif

_catch:
    प्रत्यागम परिणाम;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
स्थिर
M3_परिणाम SaveActivation (M3SnapshotSave* s, m3slot_t* i_आधारः, u32 i_अङ्कः_Slots, ध्रुव M3Activation* i_act)
{
    M3_परिणाम             परिणाम        = m3Err_none;
    IM3Function          function      = i_act->function;
    ध्रुव M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_सूचकांक = Function_सूचकांक(s->module, function);
    ध्रुव M3SafePoint*   point         = i_act->point;
    u32                  अङ्कः_मानानि;

    _throwif("a suspended frame belongs to another module", function_सूचकांक == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_सूचकांक));
_   (PutU8(s, point->विधा));
_   (PutLEB_u32(s, point->wasm_विस्थापनम्));

    यदि (point->विधा == safepoint_op) {
_       (PutLEB_u32(s, map->खण्डानि[point->aux].wasm_विस्थापनम्));
    }

    अङ्कः_मानानि = map->अङ्कः_Locals + point->अङ्कः_मानानि;

_   (PutLEB_u32(s, अङ्कः_मानानि));

    प्रति (u32 v = 0; v < अङ्कः_मानानि; ++v) {
        ध्रुव M3_Slot_मानम्* मानम् = Frame_मानम्(map, point, v);
        u8*                where;
        u64                word;
        शून्य*              reference;

_       (Locate_मानम्(i_आधारः, i_अङ्कः_Slots, i_act->sp, मानम्, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        यदि (आधारः_Type_सम्बन्धि(मानम्->type) == c_m3Type_v128) {
_           (Put_मानम्_चिह्नकम्(s, मानम्->type));
_           (Put(s, where, 16));
            अनुवर्त;
        }

_       (Read_मानम्(where, &i_act->registers, मानम्, &word, &reference));

_       (Put_मानम्_द्वयङ्काः(s, मानम्->type, IsRefType(मानम्->type) ? (u64)(uintptr_t)reference : word));
    }

    यदि (point->विधा == safepoint_resume) {
_       (Put_Ref_मानम्(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    प्रत्यागम परिणाम;
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
M3_परिणाम SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_परिणाम      परिणाम   = m3Err_none;
    M3Activation* acts     = रिक्त;
    u32           अङ्कः_Acts  = 0;
    IM3Function   function = i_cont->प्रविष्टिः_Function;
    i32           first    = (i32)i_cont->अङ्कः_चौकटाः - 1;
    m3slot_t*     आधारः;
    u32           अङ्कः_Slots;
    m3stack_t     sp;

_   (Continuation_राशिः(s->runtime, i_cont, &आधारः, &अङ्कः_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->अङ्कः_चौकटाः + 1);
    _throwifnull(acts);

    sp = आधारः;

    यावत् (true) {
        M3Activation* act = &acts[अङ्कः_Acts];

_       (अग्र_Activation(s->module, i_cont, function, sp, first, act));

        यदि (act->विधा != safepoint_call) {
            अङ्कः_Acts++;
            भङ्ग;
        }

        _throwif("a suspended continuation stops at a call", act->चरमः_Frame < 0);

        यदि (not (act->point->flags & d_m3_Safe_Point_पुच्छ_Call)) {
            अङ्कः_Acts++;
        }

        function = i_cont->चौकटाः[act->चरमः_Frame].call.function;
        sp       = act->sp + act->point->aux;
        first    = act->चरमः_Frame - 1;
    }

_   (PutLEB_u32(s, अङ्कः_Acts));

    प्रति (u32 a = 0; a < अङ्कः_Acts; ++a) {
_       (SaveActivation(s, आधारः, अङ्कः_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_परिणाम  परिणाम    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_मूलम्    = (i_cont == s->runtime->मूलम्_Continuation);
    u8        state     = snapshot_cont_परिसमाप्तम्;
    u32       type_सूचकांक = is_मूलम् ? d_m3SnapshotNone : Func_Type_सूचकांक(module, i_cont->type);

    यदि (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } अन्यथा यदि (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } अन्यथा यदि (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->प्रविष्टिः_Function);
    _throwif("a continuation belongs to another module", Function_सूचकांक(module, i_cont->प्रविष्टिः_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_मूलम् and type_सूचकांक == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_मूलम्));
_   (PutLEB_u32(s, type_सूचकांक));
_   (PutLEB_u32(s, Function_सूचकांक(module, i_cont->प्रविष्टिः_Function)));
#  यदि d_m3HasExceptionHandling
_   (Put_Ref_मानम्(s, c_m3Type_exnref, i_cont->resumeThrow));
#  अन्यथा
_   (Put_Ref_मानम्(s, c_m3Type_exnref, रिक्त));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    यदि (s->postmortem) {
        गच्छ _catch;
    }

    यदि (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->प्रविष्टिः_Function ? i_cont->प्रविष्टिः_Function->funcType : रिक्त;
        u16         अङ्कः_Rets = inner ? Get_Func_Type_अङ्कः_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_तर्काः_गणनम् > Get_Func_Type_अङ्कः_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_तर्काः_गणनम्));

        प्रति (u32 i = 0; i < i_cont->bound_तर्काः_गणनम्; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_मानम्(s, type, i_cont->val_राशिः + (अङ्कः_Rets + i) * c_io_Slot_गणनम्));
        }
    } अन्यथा यदि (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_तर्काः_गणनम्));
_       (SaveSuspendedBody(s, i_cont));
    } अन्यथा {
_       (PutLEB_u32(s, 0));
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम SaveContinuations (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (PutLEB_u32(s, s->continuations.गणनम्));

    प्रति (u32 i = 0; i < s->continuations.गणनम्; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.वस्तूनि[i]));
    }

_catch:
    प्रत्यागम परिणाम;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
स्थिर
M3_परिणाम SaveHostState (M3SnapshotSave* s)
{
    M3_परिणाम               परिणाम = m3Err_none;
    ध्रुव M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_बफर_Writer         bw     = { रिक्त, 0, 0 };

    यदि (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, बफर_Writer_Write, &bw));
    }

    यदि (bw.आकार > 0) {
_       (Put(s, bw.बफर, bw.आकार));
    }

_catch:
    m3_Free(bw.बफर);

    प्रत्यागम परिणाम;
}

स्थिर
शून्य SnapshotResourceTotals (IM3Module module, u64* स्मृतिः_अष्टकानि, u64* सारणी_Elements)
{
    *स्मृतिः_अष्टकानि = *सारणी_Elements = 0;
    प्रति (u32 i = 0; i < module->अङ्कः_Memories; ++i) {
        IM3_स्मृतिः स्मृतिः = module->memories[i];
        यदि (स्मृतिः_First_सूचकांक(module, i) == i and स्मृतिः) {
            *स्मृतिः_अष्टकानि += स्मृतिः->अङ्कः_पृष्ठानि * (u64)स्मृतिः_Page_आकार(स्मृतिः);
        }
    }
    प्रति (u32 i = 0; i < module->अङ्कः_Tables; ++i) {
        IM3_सारणी सारणी_2 = module->tables[i];
        यदि (सारणी_First_सूचकांक(module, i) == i and सारणी_2) {
            *सारणी_Elements += सारणी_2->आकार;
        }
    }
}

स्थिर
M3_परिणाम SaveMeta (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;
    u64      hash   = 0;
    u64      स्मृतिः_अष्टकानि, सारणी_Elements;
    u32      सक्रिय_Stacks = 0;
    प्रति (u32 i = 0; i < s->continuations.गणनम्; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.वस्तूनि[i];
        यदि (cont != s->runtime->मूलम्_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            सक्रिय_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &स्मृतिः_अष्टकानि, &सारणी_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_HostTimeMs()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.गणनम्));
_   (PutLEB_u32(s, s->exceptions.गणनम्));
_   (PutLEB_u64(s, स्मृतिः_अष्टकानि));
_   (PutLEB_u64(s, सारणी_Elements));
_   (PutLEB_u32(s, सक्रिय_Stacks));

#  यदि d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    प्रति (u32 i = 0; i < s->exceptions.गणनम्; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.वस्तूनि[i];
        u32          चिह्नकम्_सूचकांक  = चिह्नकम्_सूचकांक_2(s->module, exception->चिह्नकम्);

        _throwif("an exception was thrown with another module's tag", चिह्नकम्_सूचकांक == d_m3SnapshotNone);

_       (PutLEB_u32(s, चिह्नकम्_सूचकांक));
    }
#  endif

_catch:
    प्रत्यागम परिणाम;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
स्थिर
M3_परिणाम DiscoverReferences (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम            = m3Err_none;
    u32      समाप्तम्_Continuations = 0;
    u32      समाप्तम्_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    यावत् (समाप्तम्_Continuations < s->continuations.गणनम् or समाप्तम्_Exceptions < s->exceptions.गणनम्) {
#  यदि d_m3HasExceptionHandling
        यावत् (समाप्तम्_Exceptions < s->exceptions.गणनम्) {
            M3Exception* exception = (M3Exception*)s->exceptions.वस्तूनि[समाप्तम्_Exceptions++];
            IM3FuncType  type      = exception->चिह्नकम्->type;

            _throwif(m3Err_wasmMalformed, exception->अङ्कः_तर्काः != Get_Func_Type_अङ्कः_Params(type));

            प्रति (u32 a = 0; a < exception->अङ्कः_तर्काः; ++a) {
_               (Put_मानम्_द्वयङ्काः(s, GetFuncTypeParamType(type, (u16)a), exception->तर्काः[a]));
            }
        }
#  endif
        यावत् (समाप्तम्_Continuations < s->continuations.गणनम् and समाप्तम्_Exceptions == s->exceptions.गणनम्) {
            u32 id = समाप्तम्_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.वस्तूनि[id]));
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम SaveSections (M3SnapshotSave* s)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (not s->writing) {
        प्रत्यागम DiscoverReferences(s);
    }

    यदि (not s->embedded) {
_       (Put(s, c_snapshotMagic, परिमाण(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_संस्करणम्));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    यदि (s->module->अङ्कः_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_स्मृतिः, SaveMemories));
    }
    यदि (s->module->अङ्कः_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_सारणी, SaveTables));
    }
    यदि (s->module->अङ्कः_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    यदि (s->module->अङ्कः_दत्त_Segments or s->module->अङ्कः_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  यदि d_m3HasExceptionHandling
    यदि (s->exceptions.गणनम्) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    यदि (s->continuations.गणनम्) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    प्रत्यागम परिणाम;
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

प्रकारनिर्देश संरचना M3_Snapshot_आरोपय {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    शून्य*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_राशिः;
    bool      परिवर्तितम्;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    अङ्कः_Continuations;
    u64                    required_स्मृतिः_अष्टकानि;
    u64                    required_सारणी_Elements;
    u64                    loaded_स्मृतिः_अष्टकानि;
    u64                    loaded_सारणी_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  यदि d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 अङ्कः_Exceptions;

    M3Frame* चौकटाः;           // a suspended continuation's frames as they are put together, outermost first

    अष्टकानि_t pos;               // bounded section payload, read using the Wasm parser's helpers
    अष्टकानि_t अन्तः;
    bool    अन्तर्_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    आलोकितम्_Sections[256];
} M3_Snapshot_आरोपय;

स्थिर
M3_परिणाम Get (M3_Snapshot_आरोपय* l, शून्य* o_दत्त, size_t i_आकार)
{
    यदि (not l->अन्तर्_Section) {
        प्रत्यागम l->reader(o_दत्त, i_आकार, l->userdata);
    }
    यदि (i_आकार > (size_t)(l->अन्तः - l->pos)) {
        प्रत्यागम m3Err_wasmUnderrun;
    }
    यदि (i_आकार) {
        स्मृतिं_प्रतिलिख(o_दत्त, l->pos, i_आकार);
        l->pos += i_आकार;
    }
    प्रत्यागम m3Err_none;
}

स्थिर
M3_परिणाम GetU8 (M3_Snapshot_आरोपय* l, u8* o_मानम्)
{
    प्रत्यागम Get(l, o_मानम्, परिमाण(*o_मानम्));
}

स्थिर
M3_परिणाम GetBool (M3_Snapshot_आरोपय* l, u8* o_मानम्)
{
    M3_परिणाम परिणाम = GetU8(l, o_मानम्);
    प्रत्यागम परिणाम ? परिणाम : (*o_मानम् <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

स्थिर
M3_परिणाम GetLEB_u32 (M3_Snapshot_आरोपय* l, u32* o_मानम्)
{
    यदि (l->अन्तर्_Section) {
        प्रत्यागम ReadLEB_u32(o_मानम्, &l->pos, l->अन्तः);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    प्रति (u32 i = 0; i < परिमाण(encoded); ++i) {
        M3_परिणाम परिणाम = GetU8(l, &encoded[i]);
        यदि (परिणाम) {
            प्रत्यागम परिणाम;
        }
        यदि (not (encoded[i] & 0x80)) {
            अष्टकानि_t pos = encoded;
            प्रत्यागम ReadLEB_u32(o_मानम्, &pos, encoded + i + 1);
        }
    }
    प्रत्यागम m3_Err_leb_अतिप्रवाहः;
}

स्थिर
M3_परिणाम GetLEB_u64 (M3_Snapshot_आरोपय* l, u64* o_मानम्)
{
    प्रत्यागम ReadLebUnsigned(o_मानम्, 64, &l->pos, l->अन्तः);
}

स्थिर
M3_परिणाम GetLEB_i64 (M3_Snapshot_आरोपय* l, i64* o_मानम्)
{
    प्रत्यागम ReadLEB_i64(o_मानम्, &l->pos, l->अन्तः);
}

स्थिर
M3_परिणाम Skip_अष्टकानि (M3_Snapshot_आरोपय* l, size_t i_आकार)
{
    यदि (i_आकार > (size_t)(l->अन्तः - l->pos)) {
        प्रत्यागम m3Err_wasmUnderrun;
    }
    l->pos += i_आकार;
    प्रत्यागम m3Err_none;
}


स्थिर
M3_परिणाम Get_Ref_मानम् (M3_Snapshot_आरोपय* l, m3type_t i_type, शून्य** o_reference)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       विधा   = 0;
    u64      id     = 0;

    *o_reference = रिक्त;

_   (GetU8(l, &विधा));
    यदि (विधा == 0x00) {
        प्रत्यागम (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    यदि (विधा == 0x01) {
        _throwif(m3Err_wasmMalformed, आधारः_Type_सम्बन्धि(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->अङ्कः_Functions);
#  यदि d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_सम्बन्धि(Ref_Type_सम्बन्धि_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } अन्यथा यदि (विधा == 0x02) {
        _throwif(m3Err_wasmMalformed, आधारः_Type_सम्बन्धि(i_type) != c_m3Type_externref);
        ध्रुव M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        यदि (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  यदि d_m3HasExceptionHandling
    अन्यथा यदि (विधा == 0x03) {
        _throwif(m3Err_wasmMalformed, आधारः_Type_सम्बन्धि(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->अङ्कः_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    अन्यथा यदि (विधा == 0x04) {
        _throwif(m3Err_wasmMalformed, आधारः_Type_सम्बन्धि(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->अङ्कः_Continuations);
        *o_reference = l->continuations[id];
#  यदि d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        यदि (not l->checking and Heap_Type_सम्बन्धि(i_type) != d_m3Type_heapAbstract) {
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
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम GetLE32 (M3_Snapshot_आरोपय* l, u32* o_मानम्)
{
    u8       अष्टकानि[4];
    M3_परिणाम परिणाम = Get(l, अष्टकानि, परिमाण(अष्टकानि));

    *o_मानम् = 0;

    प्रति (u32 i = 0; i < परिमाण(अष्टकानि); ++i) {
        *o_मानम् |= (u32)अष्टकानि[i] << (8 * i);
    }

    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम GetLE64 (M3_Snapshot_आरोपय* l, u64* o_मानम्)
{
    u8       अष्टकानि[8];
    M3_परिणाम परिणाम = Get(l, अष्टकानि, परिमाण(अष्टकानि));

    *o_मानम् = 0;

    प्रति (u32 i = 0; i < परिमाण(अष्टकानि); ++i) {
        *o_मानम् |= (u64)अष्टकानि[i] << (8 * i);
    }

    प्रत्यागम परिणाम;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
स्थिर
M3_परिणाम Get_मानम्_चिह्नकम् (M3_Snapshot_आरोपय* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_परिणाम परिणाम  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_गन्तव्यम्_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and आधारः_Type_सम्बन्धि(*o_type) != आधारः_Type_सम्बन्धि(i_expectedType));
    यदि (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    प्रत्यागम परिणाम;
}

// The mirror of PutValueBody: narrow types come back in the low bits
स्थिर
M3_परिणाम Get_मानम्_Body (M3_Snapshot_आरोपय* l, m3type_t i_type, u64* o_द्वयङ्काः)
{
    M3_परिणाम परिणाम = m3Err_none;

    *o_द्वयङ्काः = 0;

    यदि (IsRefType(i_type)) {
        शून्य* reference = रिक्त;
_       (Get_Ref_मानम्(l, i_type, &reference));
        *o_द्वयङ्काः = (u64)(uintptr_t)reference;
    } अन्यथा यदि (आधारः_Type_सम्बन्धि(i_type) == c_m3Type_i32) {
        i32 मानम् = 0;
_       (ReadLEB_i32(&मानम्, &l->pos, l->अन्तः));
        *o_द्वयङ्काः = (u32)मानम्;
    } अन्यथा यदि (आधारः_Type_सम्बन्धि(i_type) == c_m3Type_i64) {
        i64 मानम् = 0;
_       (GetLEB_i64(l, &मानम्));
        *o_द्वयङ्काः = (u64)मानम्;
    } अन्यथा यदि (आधारः_Type_सम्बन्धि(i_type) == c_m3Type_f32) {
        u32 द्वयङ्काः = 0;
_       (GetLE32(l, &द्वयङ्काः));
        *o_द्वयङ्काः = द्वयङ्काः;
    } अन्यथा यदि (आधारः_Type_सम्बन्धि(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_द्वयङ्काः));
    } अन्यथा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Get_मानम्_द्वयङ्काः (M3_Snapshot_आरोपय* l, m3type_t i_expectedType, u64* o_द्वयङ्काः)
{
    M3_परिणाम परिणाम = m3Err_none;
    m3type_t type;

_   (Get_मानम्_चिह्नकम्(l, i_expectedType, &type));
_   (Get_मानम्_Body(l, type, o_द्वयङ्काः));

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम Get_मानम् (M3_Snapshot_आरोपय* l, m3type_t i_expectedType, शून्य* o_where)
{
    M3_परिणाम परिणाम = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    यदि (l->checking) {
        o_where = scratch;
    }

_   (Get_मानम्_चिह्नकम्(l, i_expectedType, &type));

    यदि (IsRefType(type)) {
        शून्य* reference = रिक्त;
        u64   द्वयङ्काः      = 0;
_       (Get_मानम्_Body(l, type, &द्वयङ्काः));
        reference = (शून्य*)(uintptr_t)द्वयङ्काः;
        स्मृतिं_प्रतिलिख(o_where, &reference, परिमाण(reference));
    } अन्यथा यदि (आधारः_Type_सम्बन्धि(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } अन्यथा {
        u64 द्वयङ्काः = 0;
_       (Get_मानम्_Body(l, type, &द्वयङ्काः));

        यदि (Is64BitType(type)) {
            स्मृतिं_प्रतिलिख(o_where, &द्वयङ्काः, परिमाण(द्वयङ्काः));
        } अन्यथा {
            u32 narrow = (u32)द्वयङ्काः;
            स्मृतिं_प्रतिलिख(o_where, &narrow, परिमाण(narrow));
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम GetFunction (M3_Snapshot_आरोपय* l, IM3Function* o_function)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->अङ्कः_Functions);
    *o_function = &l->module->functions[index];

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_स्मृतिः_Chunks (M3_Snapshot_आरोपय* l, u8* o_अष्टकानि, size_t i_आकार)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रति (;;) {
        u8  chunkType = 0;
        u32 offset = 0, दीर्घता = 0;

_       (GetU8(l, &chunkType));

        यदि (chunkType == d_m3_Chunk_अन्तः) {
            भङ्ग;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &दीर्घता));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_आकार or दीर्घता > i_आकार - (size_t)offset);

        यदि (chunkType == d_m3ChunkRaw) {
            यदि (l->checking or दीर्घता == 0) {
_               (Skip_अष्टकानि(l, दीर्घता));
            } अन्यथा {
_               (Get(l, o_अष्टकानि + offset, दीर्घता));
            }
        } अन्यथा यदि (chunkType == d_m3ChunkFillFF) {
            यदि (not l->checking and दीर्घता) {
                स्मृतिं_पूरय(o_अष्टकानि + offset, 0xFF, दीर्घता);
            }
        } अन्यथा {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Memories (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम  परिणाम      = m3Err_none;
    IM3Module module      = l->module;
    u32       अङ्कः_Memories = 0;
    l->loaded_स्मृतिः_अष्टकानि  = 0;

_   (GetLEB_u32(l, &अङ्कः_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, अङ्कः_Memories == 0 or अङ्कः_Memories != module->अङ्कः_Memories);

    प्रति (u32 m = 0; m < अङ्कः_Memories; ++m) {
        u32       mem_सूचकांक = 0, first_सूचकांक = 0, page_द्वयङ्काः = 0, अङ्कः_पृष्ठानि = 0;
        IM3_स्मृतिः स्मृतिः;

_       (GetLEB_u32(l, &mem_सूचकांक));
_       (GetLEB_u32(l, &first_सूचकांक));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_सूचकांक != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 first_सूचकांक != स्मृतिः_First_सूचकांक(module, m));
        यदि (first_सूचकांक != m) {
            अनुवर्त;
        }

_       (GetLEB_u32(l, &page_द्वयङ्काः));
_       (GetLEB_u32(l, &अङ्कः_पृष्ठानि));

        स्मृतिः = module->memories[mem_सूचकांक];
        _throwif("a shared memory cannot be restored from a snapshot", स्मृतिः and स्मृतिः->isShared);

        size_t अष्टकानि = 0;
        u8*    दत्त  = रिक्त;

        यदि (स्मृतिः) {
            _throwif(m3Err_wasmMalformed, page_द्वयङ्काः >= 32 or (1u << page_द्वयङ्काः) != स्मृतिः_Page_आकार(स्मृतिः));
            _throwif(m3Err_wasmMalformed, अङ्कः_पृष्ठानि < स्मृतिः->अङ्कः_पृष्ठानि or अङ्कः_पृष्ठानि > स्मृतिः->max_पृष्ठानि);
            _throwif(m3Err_wasmMalformed, अङ्कः_पृष्ठानि > SIZE_MAX / स्मृतिः_Page_आकार(स्मृतिः));
            अष्टकानि = (size_t)अङ्कः_पृष्ठानि * स्मृतिः_Page_आकार(स्मृतिः);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->स्मृतिः_सीमा and अष्टकानि > (size_t)l->runtime->स्मृतिः_सीमा);

            यदि (not l->checking) {
                यदि (स्मृतिः->अङ्कः_पृष्ठानि < अङ्कः_पृष्ठानि) {
_                   (Resize_स्मृतिः(l->runtime, स्मृतिः, अङ्कः_पृष्ठानि));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         स्मृतिः->mallocated and स्मृतिः->mallocated->दीर्घता < अष्टकानि);
            }

            l->loaded_स्मृतिः_अष्टकानि += अष्टकानि;

            यदि (अष्टकानि and स्मृतिः->mallocated) {
                दत्त = m3_Mem_दत्त(स्मृतिः->mallocated);
            }
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, अङ्कः_पृष्ठानि != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        यदि (दत्त and not l->checking) {
            स्मृतिं_पूरय(दत्त, 0, अष्टकानि);
        }

_       (आरोपय_स्मृतिः_Chunks(l, दत्त, अष्टकानि));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_स्मृतिः_अष्टकानि != l->required_स्मृतिः_अष्टकानि);

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Globals (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम  परिणाम     = m3Err_none;
    IM3Module module     = l->module;
    u32       अङ्कः_Globals = 0;

_   (GetLEB_u32(l, &अङ्कः_Globals));
    _throwif(m3Err_wasmMalformed, अङ्कः_Globals == 0 or अङ्कः_Globals != module->अङ्कः_Globals);

    प्रति (u32 g = 0; g < अङ्कः_Globals; ++g) {
        u32       global_सूचकांक = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_सूचकांक));
        _throwif(m3Err_wasmMalformed, global_सूचकांक != g);

        global = &module->globals[global_सूचकांक];
        cell   = global->resolved ? global->resolved : global;

_       (Get_मानम्(l, global->type, &cell->i64_मानम्));
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Tables (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम  परिणाम       = m3Err_none;
    IM3Module module       = l->module;
    u32       अङ्कः_Tables    = 0;
    l->loaded_सारणी_Elements = 0;

_   (GetLEB_u32(l, &अङ्कः_Tables));
    _throwif(m3Err_wasmMalformed, अङ्कः_Tables == 0 or अङ्कः_Tables != module->अङ्कः_Tables);

    प्रति (u32 t = 0; t < अङ्कः_Tables; ++t) {
        u32      सारणी_सूचकांक = 0, first_सूचकांक = 0, आकार = 0;
        u8       valtype = 0;
        IM3_सारणी सारणी_2;

_       (GetLEB_u32(l, &सारणी_सूचकांक));
_       (GetLEB_u32(l, &first_सूचकांक));

        _throwif(m3Err_wasmMalformed, सारणी_सूचकांक != t);
        _throwif("the snapshot shares tables between imports differently",
                 first_सूचकांक != सारणी_First_सूचकांक(module, t));
        यदि (first_सूचकांक != t) {
            अनुवर्त;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &आकार));

        सारणी_2 = module->tables[सारणी_सूचकांक];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_गन्तव्यम्_Val_Type(सारणी_2->type));
        _throwif(m3Err_wasmMalformed, आकार > (सारणी_2->hasMax ? सारणी_2->max_आकार : d_m3_Max_Sane_सारणी_आकार));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, आकार < सारणी_2->आकार);

        l->loaded_सारणी_Elements += आकार;

        यदि (not l->checking and आकार != सारणी_2->आकार) {
            शून्य** elements;

            _throwif(m3_Err_सारणी_सीमा_Exceeded, l->runtime->सारणी_Elements_सीमा and
                                                 आकार - सारणी_2->आकार > l->runtime->सारणी_Elements_सीमा - l->runtime->सारणी_Elements_प्रयुक्तम्);
            elements = m3_ReallocArray(शून्य*, सारणी_2->elements, आकार, सारणी_2->आकार);
            _throwifnull(elements);

            l->runtime->सारणी_Elements_प्रयुक्तम् += आकार - सारणी_2->आकार;
            सारणी_2->elements = elements;
            सारणी_2->आकार     = आकार;
        }

        प्रति (u32 e = 0; e < आकार; ++e) {
            शून्य* reference = रिक्त;
_           (Get_Ref_मानम्(l, सारणी_2->type, &reference));
            यदि (not l->checking) {
                सारणी_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_सारणी_Elements != l->required_सारणी_Elements);

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Segments (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम  परिणाम = m3Err_none;
    IM3Module module = l->module;
    u32       गणनम्  = 0;

_   (GetLEB_u32(l, &गणनम्));
    _throwif(m3Err_wasmMalformed, गणनम् != module->अङ्कः_दत्त_Segments);
    _throwif(m3Err_wasmMalformed, module->अङ्कः_दत्त_Segments == 0 and module->अङ्कः_Element_Segments == 0);

    प्रति (u32 i = 0; i < गणनम्; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        यदि (not l->checking) {
            module->दत्त_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &गणनम्));
    _throwif(m3Err_wasmMalformed, गणनम् != module->अङ्कः_Element_Segments);

    प्रति (u32 i = 0; i < गणनम्; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        यदि (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

// A frame for the continuation being put together, outermost first
स्थिर
M3_परिणाम सङ्कलय_Frame (M3_Snapshot_आरोपय* l, u32* io_अङ्कः_चौकटाः, M3_Frame_विधा i_विधा, m3stack_t i_sp, M3Frame** o_frame)
{
    यदि (*io_अङ्कः_चौकटाः >= d_m3_Continuation_Max_चौकटाः) {
        प्रत्यागम "the snapshot's frames do not fit this build's";
    }

    M3Frame* frame = &l->चौकटाः[(*io_अङ्कः_चौकटाः)++];

    स्मृतिं_पूरय(frame, 0, परिमाण(*frame));
    frame->विधा   = (u8)i_विधा;
    frame->sp     = i_sp;
    frame->स्मृतिः = Module_Memory0(l->module);

    *o_frame = frame;

    प्रत्यागम m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
स्थिर
M3_परिणाम PrepareFunction (IM3Function i_function)
{
    M3_परिणाम परिणाम = m3Err_none;

    यदि (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    प्रत्यागम परिणाम;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
स्थिर
M3_परिणाम सङ्कलय_खण्डम्_चौकटाः (M3_Snapshot_आरोपय* l, ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, m3stack_t i_sp,
                         u32* io_अङ्कः_चौकटाः)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रति (u32 b = 0; b < i_map->अङ्कः_खण्डानि; ++b) {
        ध्रुव M3_खण्डम्_Start* खण्डम् = &i_map->खण्डानि[b];
        M3Frame*            frame;

        यदि (not Encloses_विस्थापनम्(खण्डम्, i_point->wasm_विस्थापनम्)) {
            अनुवर्त;
        }

        यदि (खण्डम्->opcode == c_waOp_loop) {
_           (सङ्कलय_Frame(l, io_अङ्कः_चौकटाः, frame_loop, i_sp, &frame));
            frame->pc = खण्डम्->pc;
        }
#  यदि d_m3HasExceptionHandling
        अन्यथा {
_           (सङ्कलय_Frame(l, io_अङ्कः_चौकटाः, frame_try, i_sp, &frame));
            frame->pc                = खण्डम्->pc;
            frame->try_.अङ्कः_Clauses   = खण्डम्->अङ्कः_Clauses;
            frame->try_.handlers_जीवितम् = Handlers_जीवितम्_At(i_map, i_point, खण्डम्);
        }
#  endif
    }

_catch:
    प्रत्यागम परिणाम;
}

// Puts a function's values where this build keeps them at the safepoint
स्थिर
M3_परिणाम आरोपय_मानानि (M3_Snapshot_आरोपय* l, m3slot_t* i_आधारः, u32 i_अङ्कः_Slots, m3stack_t i_sp,
                     ध्रुव M3SnapshotMap* i_map, ध्रुव M3SafePoint* i_point, ध्रुव M3FrameRegisters* i_registers)
{
    M3_परिणाम परिणाम    = m3Err_none;
    u32      अङ्कः_मानानि = 0;

_   (GetLEB_u32(l, &अङ्कः_मानानि));
    _throwif("the snapshot's frame does not match what this build compiled", अङ्कः_मानानि != i_map->अङ्कः_Locals + i_point->अङ्कः_मानानि);

    प्रति (u32 v = 0; v < अङ्कः_मानानि; ++v) {
        ध्रुव M3_Slot_मानम्* मानम्     = Frame_मानम्(i_map, i_point, v);
        u64                word      = 0;
        शून्य*              reference = रिक्त;
        u8*                where;

_       (Locate_मानम्(i_आधारः, i_अङ्कः_Slots, i_sp, मानम्, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        यदि (आधारः_Type_सम्बन्धि(मानम्->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_मानम्_चिह्नकम्(l, मानम्->type, &type));
            यदि (l->checking) {
_               (Skip_अष्टकानि(l, 16));
            } अन्यथा {
_               (Get(l, where, 16));
            }
            अनुवर्त;
        }

        l->allowUnfilledRef = i_point->विधा == safepoint_suspend and
                              v >= अङ्कः_मानानि - i_point->अङ्कः_Results;
_       (Get_मानम्_द्वयङ्काः(l, मानम्->type, &word));
        l->allowUnfilledRef = false;

        यदि (IsRefType(मानम्->type)) {
            reference = (शून्य*)(uintptr_t)word;
        }

        यदि (not l->checking) {
_           (Write_मानम्(where, i_registers, मानम्, word, reference));
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
स्थिर
M3_परिणाम आरोपय_Suspended_Body (M3_Snapshot_आरोपय* l, IM3Continuation io_cont, u32 i_id)
{
    M3_परिणाम    परिणाम    = m3Err_none;
    u32         अङ्कः_चौकटाः = 0;
    M3Frame*    callFrame = रिक्त;
    IM3Function function  = रिक्त;
    m3slot_t*   आधारः;
    u32         अङ्कः_Slots;
    u32         अङ्कः_Activations = 0;
    IM3FuncType expected       = io_cont->प्रविष्टिः_Function->funcType;
    m3stack_t   sp;

_   (Continuation_राशिः(l->runtime, io_cont, &आधारः, &अङ्कः_Slots));

_   (GetLEB_u32(l, &अङ्कः_Activations));
    _throwif(m3Err_wasmMalformed, अङ्कः_Activations == 0 or अङ्कः_Activations > d_m3_Continuation_Max_चौकटाः);

    io_cont->pc                = रिक्त;
    io_cont->r0                = 0;
    io_cont->अङ्कः_Suspend_Results = 0;
    io_cont->suspendPoint      = safepoint_op;
#  यदि d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = आधारः;

    प्रति (u32 a = 0; a < अङ्कः_Activations; ++a) {
        bool                 isInnermost = (a + 1 == अङ्कः_Activations);
        ध्रुव M3SafePoint*   point;
        M3FrameRegisters     registers;
        u8                   विधा       = 0;
        u32                  wasm_विस्थापनम् = 0, targetLoop = 0;
        ध्रुव M3SnapshotMap* map;

        स्मृतिं_पूरय(&registers, 0, परिमाण(registers));

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
                 (u32)(sp - आधारः) + function->max_राशिः_Slots > अङ्कः_Slots);

        // the call the frame outside this one is waiting on
        यदि (callFrame) {
            callFrame->call.function = function;
        }

#  यदि d_m3_प्रविष्टिः_Keeps_Frame
        {
            M3Frame* प्रविष्टिः;
_           (सङ्कलय_Frame(l, &अङ्कः_चौकटाः, frame_प्रविष्टिः, sp, &प्रविष्टिः));
            प्रविष्टिः->pc             = function->compiled;
            प्रविष्टिः->प्रविष्टिः.function = function;
        }
#  endif

_       (GetU8(l, &विधा));
_       (GetLEB_u32(l, &wasm_विस्थापनम्));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        यदि (विधा == safepoint_op) {
            ध्रुव M3_खण्डम्_Start* loop;

_           (GetLEB_u32(l, &targetLoop));

            loop = Find_खण्डम्_At(map, c_waOp_loop, targetLoop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_विस्थापनम्(loop, wasm_विस्थापनम्));
        }

        point = FindSafePointAt(map, wasm_विस्थापनम्, विधा, targetLoop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not point);

_       (सङ्कलय_खण्डम्_चौकटाः(l, map, point, sp, &अङ्कः_चौकटाः));

        यदि (not isInnermost) {
            _throwif(m3Err_wasmMalformed, विधा != safepoint_call);

_           (सङ्कलय_Frame(l, &अङ्कः_चौकटाः, frame_call, sp, &callFrame));
            callFrame->pc = point->pc;
            registers.r0  = &callFrame->call.r0;
#  यदि d_m3HasFloat
            registers.fp0 = &callFrame->call.fp0;
#  endif
        } अन्यथा यदि (विधा == safepoint_resume) {
            M3Frame* frame;
            u32      अङ्कः_Results = point->अङ्कः_Results;
            pc_t     resultsPC  = point->pc - 2 * अङ्कः_Results;

_           (सङ्कलय_Frame(l, &अङ्कः_चौकटाः, frame_resume, sp, &frame));
            frame->pc                 = point->pc;
            frame->resume.resultsPC   = resultsPC;
            frame->resume.अङ्कः_Results  = अङ्कः_Results;
            frame->resume.अङ्कः_Handlers = point->aux;
            frame->resume.handlersPC  = resultsPC - 1 - 3 * point->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            callFrame = frame;
        } अन्यथा {
            _throwif(m3Err_wasmMalformed, विधा != safepoint_op and विधा != safepoint_प्रविष्टिः and विधा != safepoint_suspend);

            io_cont->pc           = point->pc;
            io_cont->suspendPoint = विधा;
            registers.r0          = &io_cont->r0;
#  यदि d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (आरोपय_मानानि(l, आधारः, अङ्कः_Slots, sp, map, point, &registers));

        यदि (not l->checking) {
            RestoreConstants(function, sp);
        }

        यदि (not isInnermost) {
            sp = sp + point->aux;
_           (CallSiteType(function, point, &expected));
        } अन्यथा यदि (विधा == safepoint_resume) {
            शून्य* cont = रिक्त;
_           (Get_Ref_मानम्(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == रिक्त);

            callFrame->resume.cont = (IM3Continuation)cont;
            यदि (not l->checking) {
                l->suspendSites[i_id].point = point;
                प्रति (u32 i = 1; i < l->अङ्कः_Continuations; ++i) {
                    यदि (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].target = i;
                        l->suspendSites[i].resumed   = true;
                        भङ्ग;
                    }
                }
            }
        } अन्यथा {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            यदि (point->flags & d_m3_Safe_Point_गृहीतम्_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            यदि (विधा == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, point->अङ्कः_Results > point->अङ्कः_मानानि or
                                                point->अङ्कः_Results > d_m3MaxContinuationPayload);

                io_cont->अङ्कः_Suspend_Results = point->अङ्कः_Results;
                यदि (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].point = point;
                }

                प्रति (u32 i = 0; i < point->अङ्कः_Results; ++i) {
                    ध्रुव M3_Slot_मानम्* मानम् = &map->मानानि[point->first_मानम् + point->अङ्कः_मानानि - point->अङ्कः_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_पञ्जिकाकोशः_Slot_Alias(मानम्->slot));

                    io_cont->suspend_परिणाम_Offsets[i] = मानम्->slot;
                    io_cont->suspend_परिणाम_Is64[i]    = Is64BitType(मानम्->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    यदि (l->checking) {
        प्रत्यागम m3Err_none;
    }

    // held innermost first
    यदि (अङ्कः_चौकटाः > io_cont->चौकटाः_Cap) {
        M3Frame* चौकटाः = m3_ReallocArray(M3Frame, io_cont->चौकटाः, अङ्कः_चौकटाः, io_cont->चौकटाः_Cap);
        _throwifnull(चौकटाः);

        io_cont->चौकटाः    = चौकटाः;
        io_cont->चौकटाः_Cap = अङ्कः_चौकटाः;
    }

    प्रति (u32 k = 0; k < अङ्कः_चौकटाः; ++k) {
        io_cont->चौकटाः[अङ्कः_चौकटाः - 1 - k] = l->चौकटाः[k];
    }

    io_cont->अङ्कः_चौकटाः = अङ्कः_चौकटाः;

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Continuation (M3_Snapshot_आरोपय* l, u32 i_id)
{
    M3_परिणाम        परिणाम = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    यदि (l->checking) {
        scratch          = *cont;
        scratch.val_राशिः = l->check_राशिः;
        यदि (i_id == 0) {
            scratch.val_राशिः = (m3slot_t*)l->runtime->origin_राशिः;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_सूचकांक = 0, प्रविष्टिः_सूचकांक = 0, bound_तर्काः_गणनम् = 0;
    u8  state = 0, is_मूलम् = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_मूलम्));
_   (GetLEB_u32(l, &type_सूचकांक));
_   (GetLEB_u32(l, &प्रविष्टिः_सूचकांक));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_मूलम् != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_मूलम् and state != snapshot_contSuspended);
    यदि (not is_मूलम् and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_सूचकांक != d_m3SnapshotNone and type_सूचकांक >= module->अङ्कः_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, प्रविष्टिः_सूचकांक >= module->अङ्कः_Functions);
    _throwif(m3Err_wasmMalformed, is_मूलम् ? type_सूचकांक != d_m3SnapshotNone : (type_सूचकांक == d_m3SnapshotNone or not module->funcTypes[type_सूचकांक]->isContinuation));

    यदि (not l->checking and not is_मूलम् and state != snapshot_cont_परिसमाप्तम्) {
_       (Continuation_Acquire_राशिः(l->runtime, cont));
    }
    cont->type          = (type_सूचकांक != d_m3SnapshotNone) ? module->funcTypes[type_सूचकांक] : रिक्त;
    cont->प्रविष्टिः_Function = &module->functions[प्रविष्टिः_सूचकांक];
    cont->अङ्कः_Handlers   = 0;
    cont->handlersPC    = रिक्त;
    cont->parent        = रिक्त;
    cont->अङ्कः_चौकटाः     = 0;

#  यदि d_m3HasExceptionHandling
    {
        शून्य* ref = रिक्त;
_       (Get_Ref_मानम्(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_मूलम् or state == snapshot_cont_परिसमाप्तम्) and cont->resumeThrow != रिक्त);
#  अन्यथा
    {
        शून्य* ref = रिक्त;
_       (Get_Ref_मानम्(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != रिक्त);
    }
#  endif

    यदि (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         अङ्कः_Rets;

_       (GetLEB_u32(l, &bound_तर्काः_गणनम्));
        cont->bound_तर्काः_गणनम् = bound_तर्काः_गणनम्;
        inner                = cont->प्रविष्टिः_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_तर्काः_गणनम् > Get_Func_Type_अङ्कः_Params(inner));

        {
            IM3FuncType remaining = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not remaining or
                                            Get_Func_Type_अङ्कः_Params(remaining) != Get_Func_Type_अङ्कः_Params(inner) - bound_तर्काः_गणनम् or
                                            Get_Func_Type_अङ्कः_Results(remaining) != Get_Func_Type_अङ्कः_Results(inner));
            प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_परिणाम_Type(inner, i) != Get_Func_Type_परिणाम_Type(remaining, i));
            }
            प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Params(remaining); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_तर्काः_गणनम् + i)) != GetFuncTypeParamType(remaining, i));
            }
        }

        अङ्कः_Rets = Get_Func_Type_अङ्कः_Results(inner);

        _throwif(m3Err_wasmMalformed, (अङ्कः_Rets + bound_तर्काः_गणनम्) * c_io_Slot_गणनम् > cont->अङ्कः_राशिः_Slots);

        प्रति (u32 i = 0; i < bound_तर्काः_गणनम्; ++i) {
_           (Get_मानम्(l, GetFuncTypeParamType(inner, (u16)i), cont->val_राशिः + (अङ्कः_Rets + i) * c_io_Slot_गणनम्));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_राशिः;
        cont->pc    = cont->प्रविष्टिः_Function->compiled;
    } अन्यथा यदि (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_तर्काः_गणनम्));
        _throwif(m3Err_wasmMalformed, is_मूलम् and bound_तर्काः_गणनम् != 0);
        cont->bound_तर्काः_गणनम् = bound_तर्काः_गणनम्;

_       (आरोपय_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } अन्यथा यदि (state == snapshot_cont_परिसमाप्तम्) {
_       (GetLEB_u32(l, &bound_तर्काः_गणनम्));
        _throwif(m3Err_wasmMalformed, bound_तर्काः_गणनम् != 0);
        cont->bound_तर्काः_गणनम् = 0;
        cont->state          = cont_consumed;
    } अन्यथा {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    यदि (परिणाम) {
        cont->अङ्कः_चौकटाः = 0;
    }

    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Continuations (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      गणनम्  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &गणनम्));
    _throwif(m3Err_wasmMalformed, गणनम् == 0 or गणनम् != l->अङ्कः_Continuations);

    प्रति (u32 i = 0; i < गणनम्; ++i) {
_       (आरोपय_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    प्रत्यागम परिणाम;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
स्थिर
M3_परिणाम CheckContinuationReferences (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रति (u32 id = 0; id < l->अङ्कः_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             पर्णम्  = id;
        u32             गभीरता = 0;
#  यदि d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_सम्बन्धि(Ref_Type_सम्बन्धि_Func_Type(cont->type, true), expectedType)));
#  endif
        यदि (l->suspendSites[id].target) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_सम्बन्धि_Ref(l->module, l->suspendSites[id].point->resumeType);
            IM3Continuation child    = l->continuations[l->suspendSites[id].target];
            IM3FuncType     actual   = child->प्रविष्टिः_Function ? child->प्रविष्टिः_Function->funcType : रिक्त;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_अङ्कः_Results(actual) != Get_Func_Type_अङ्कः_Results(expected));
            प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_परिणाम_Type(actual, i) != Get_Func_Type_परिणाम_Type(expected, i));
            }
        }
        यावत् (l->suspendSites[पर्णम्].target) {
            पर्णम् = l->suspendSites[पर्णम्].target;
            _throwif(m3Err_wasmMalformed, ++गभीरता >= l->अङ्कः_Continuations or
                                            l->continuations[पर्णम्]->state != cont_suspended);
        }
        यदि (cont->state != cont_suspended or id == 0) {
            अनुवर्त;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        यदि (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_तर्काः_गणनम् != 0);
            अनुवर्त;
        }

        ध्रुव M3SnapshotSuspendSite* site      = &l->suspendSites[पर्णम्];
        IM3Continuation              inner     = l->continuations[पर्णम्];
        IM3FuncType                  remaining = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->point or not remaining);
        _throwif("a captured continuation has the wrong binding count", cont->bound_तर्काः_गणनम् > inner->अङ्कः_Suspend_Results or
                                                                          Get_Func_Type_अङ्कः_Params(remaining) != inner->अङ्कः_Suspend_Results - cont->bound_तर्काः_गणनम्);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_अङ्कः_Results(remaining) != Get_Func_Type_अङ्कः_Results(cont->प्रविष्टिः_Function->funcType));
        प्रति (u16 i = 0; i < Get_Func_Type_अङ्कः_Results(remaining); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_परिणाम_Type(remaining, i) !=
                                            Get_Func_Type_परिणाम_Type(cont->प्रविष्टिः_Function->funcType, i));
        }

        प्रति (u32 i = 0; i < inner->अङ्कः_Suspend_Results; ++i) {
            ध्रुव M3_Slot_मानम्* मानम् = &site->map->मानानि[site->point->first_मानम् +
                                                          site->point->अङ्कः_मानानि - site->point->अङ्कः_Results + i];
            यदि (i < cont->bound_तर्काः_गणनम्) {
                यदि (IsRefType(मानम्->type) and not IsNullableRef(मानम्->type)) {
                    शून्य* ref = रिक्त;
                    स्मृतिं_प्रतिलिख(&ref, inner->sp + मानम्->slot, परिमाण(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } अन्यथा {
                _throwif("a captured continuation has the wrong parameter type", मानम्->type !=
                                                                                   GetFuncTypeParamType(remaining, (u16)(i - cont->bound_तर्काः_गणनम्)));
            }
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

#  यदि d_m3HasExceptionHandling

स्थिर
M3_परिणाम आरोपय_Exceptions (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम परिणाम        = m3Err_none;
    u32      अङ्कः_Exceptions = 0;

_   (GetLEB_u32(l, &अङ्कः_Exceptions));
    _throwif(m3Err_wasmMalformed, अङ्कः_Exceptions == 0 or अङ्कः_Exceptions != l->अङ्कः_Exceptions);

    प्रति (u32 i = 0; i < अङ्कः_Exceptions; ++i) {
        u32          exnId = 0, अङ्कः_तर्काः = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &अङ्कः_तर्काः));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->चिह्नकम्->type;

        _throwif(m3Err_wasmMalformed, अङ्कः_तर्काः != exn->अङ्कः_तर्काः);

        प्रति (u32 a = 0; a < अङ्कः_तर्काः; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      द्वयङ्काः    = 0;
_           (Get_मानम्_द्वयङ्काः(l, argType, &द्वयङ्काः));
            यदि (not l->checking) {
                exn->तर्काः[a] = द्वयङ्काः;
            }
        }
    }

_catch:
    प्रत्यागम परिणाम;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
प्रकारनिर्देश संरचना M3BoundedReader {
    M3_Snapshot_आरोपय* आरोपय;
    u64             remaining;
    M3_परिणाम        error;
} M3BoundedReader;

स्थिर
M3_परिणाम BoundedReader_Read (शून्य* o_बफर, size_t i_आकार, शून्य* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    यदि (i_आकार > br->remaining) {
        br->error = "the host read past the end of its state";
        प्रत्यागम br->error;
    }

    br->remaining -= i_आकार;

    प्रत्यागम Get(br->आरोपय, o_बफर, i_आकार);
}

स्थिर
M3_परिणाम आरोपय_Host_State (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम               परिणाम = m3Err_none;
    ध्रुव M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 आकार = (size_t)(l->अन्तः - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->आरोपय_Host_State_2);

    यदि (l->checking) {
        प्रत्यागम Skip_अष्टकानि(l, आकार);
    }
    reader.आरोपय      = l;
    reader.remaining = आकार;
    reader.error     = m3Err_none;

_   (hooks->आरोपय_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, आकार));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.remaining != 0);

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_शीर्षकम् (M3_Snapshot_आरोपय* l)
{
    M3_परिणाम परिणाम = m3Err_none;
    u8       magic[परिमाण(c_snapshotMagic)];
    u32      संस्करणम् = 0;

_   (Get(l, magic, परिमाण(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, परिमाण(magic)) != 0);

_   (GetLE32(l, &संस्करणम्));
    _throwif(m3Err_wasmMalformed, संस्करणम् != c_snapshot_संस्करणम्);

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Meta (M3_Snapshot_आरोपय* l, u32* o_flags)
{
    M3_परिणाम परिणाम      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      स्मृतिः_अष्टकानि = 0, सारणी_Elements = 0;
    u32      सक्रिय_Stacks = 0;
    u64      current_स्मृतिः, currentTables;
    SnapshotResourceTotals(l->module, &current_स्मृतिः, &currentTables);

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
_   (GetLEB_u64(l, &स्मृतिः_अष्टकानि));
_   (GetLEB_u64(l, &सारणी_Elements));
_   (GetLEB_u32(l, &सक्रिय_Stacks));
    l->requiredStacks        = सक्रिय_Stacks;
    l->required_स्मृतिः_अष्टकानि   = स्मृतिः_अष्टकानि;
    l->required_सारणी_Elements = सारणी_Elements;
    _throwif(m3Err_wasmMalformed, स्मृतिः_अष्टकानि < current_स्मृतिः or सारणी_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->अङ्कः_Memories and स्मृतिः_अष्टकानि != 0);
    _throwif(m3Err_wasmMalformed, not l->module->अङ्कः_Tables and सारणी_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    यदि (*o_flags & d_m3SnapshotFlagPostmortem) {
        प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
            u32 चिह्नकम्_सूचकांक = 0;
_           (GetLEB_u32(l, &चिह्नकम्_सूचकांक));
        }
        प्रत्यागम m3Err_none;
    }

    _throwif(m3_Err_स्मृतिः_सीमा_Exceeded, l->runtime->स्मृतिः_अष्टकानि_सीमा and
                                          स्मृतिः_अष्टकानि - current_स्मृतिः > l->runtime->स्मृतिः_अष्टकानि_सीमा - l->runtime->स्मृतिः_अष्टकानि_प्रयुक्तम्);
    _throwif(m3_Err_सारणी_सीमा_Exceeded, l->runtime->सारणी_Elements_सीमा and
                                         सारणी_Elements - currentTables > l->runtime->सारणी_Elements_सीमा - l->runtime->सारणी_Elements_प्रयुक्तम्);
    _throwif(m3_Err_continuation_सीमा_Exceeded, l->runtime->continuations_सीमा and
                                                सक्रिय_Stacks > l->runtime->continuations_सीमा - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->अङ्कः_Continuations == 0 or सक्रिय_Stacks >= l->अङ्कः_Continuations);
#  यदि !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->अङ्कः_Exceptions != 0);
#  endif

    यदि (l->checking) {
        प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
            u32 चिह्नकम्_सूचकांक = 0;
_           (GetLEB_u32(l, &चिह्नकम्_सूचकांक));
#  यदि d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, चिह्नकम्_सूचकांक >= l->module->अङ्कः_चिह्नकानि);
#  endif
        }
        प्रत्यागम m3Err_none;
    }

    l->check_राशिः = m3_AllocArray(m3slot_t, d_m3_Continuation_राशिः_Slots + 4);
    _throwifnull(l->check_राशिः);
    l->चौकटाः = m3_AllocArray(M3Frame, d_m3_Continuation_Max_चौकटाः);
    _throwifnull(l->चौकटाः);

    l->continuations = m3_AllocArray(IM3Continuation, l->अङ्कः_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->अङ्कः_Continuations);
    _throwifnull(l->suspendSites);

    यदि (not l->runtime->मूलम्_Continuation) {
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

#  यदि d_m3HasExceptionHandling
    यदि (l->अङ्कः_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->अङ्कः_Exceptions);
        _throwifnull(l->exceptions);
    }

    प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
        u32    चिह्नकम्_सूचकांक = 0;
        IM3_चिह्नकम् चिह्नकम्;
        u32    अङ्कः_तर्काः;

_       (GetLEB_u32(l, &चिह्नकम्_सूचकांक));
        _throwif(m3Err_wasmMalformed, चिह्नकम्_सूचकांक >= l->module->अङ्कः_चिह्नकानि);

        चिह्नकम्     = चिह्नकम्_सम्बन्धि_सूचकांक(l->module, चिह्नकम्_सूचकांक);
        अङ्कः_तर्काः = Get_Func_Type_अङ्कः_Params(चिह्नकम्->type);

        l->exceptions[i] = NewException(l->runtime, चिह्नकम्, अङ्कः_तर्काः);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  अन्यथा
    प्रति (u32 i = 0; i < l->अङ्कः_Exceptions; ++i) {
        u32 चिह्नकम्_सूचकांक = 0;
_       (GetLEB_u32(l, &चिह्नकम्_सूचकांक));
    }
#  endif

_catch:
    प्रत्यागम परिणाम;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
स्थिर
M3_परिणाम आरोपय_Section (M3_Snapshot_आरोपय* l, u8 i_sectionId)
{
    M3_परिणाम परिणाम = m3Err_none;
    u32      flags  = 0;
    विकल्प (i_sectionId) {
    पक्ष d_m3SnapshotSection_Meta:
_       (आरोपय_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        भङ्ग;

    पक्ष d_m3_Snapshot_Section_स्मृतिः:
_       (आरोपय_Memories(l));
        भङ्ग;

    पक्ष d_m3_Snapshot_Section_सारणी:
_       (आरोपय_Tables(l));
        भङ्ग;

    पक्ष d_m3SnapshotSection_Global:
_       (आरोपय_Globals(l));
        भङ्ग;

    पक्ष d_m3SnapshotSection_Segment:
_       (आरोपय_Segments(l));
        भङ्ग;

#  यदि d_m3HasExceptionHandling
    पक्ष d_m3SnapshotSection_Exception:
_       (आरोपय_Exceptions(l));
        भङ्ग;
#  endif

    पक्ष d_m3SnapshotSection_Continuation:
_       (आरोपय_Continuations(l));
        भङ्ग;

    पक्ष d_m3SnapshotSection_HostState:
_       (आरोपय_Host_State(l));
        भङ्ग;

    स्वभाव:
_       (Skip_अष्टकानि(l, (size_t)(l->अन्तः - l->pos)));
        भङ्ग;
    }

_catch:
    प्रत्यागम परिणाम;
}

स्थिर
M3_परिणाम आरोपय_Sections (M3_Snapshot_आरोपय* l, bool i_embedded)
{
    M3_परिणाम परिणाम  = m3Err_none;
    u8*      payload = रिक्त;

    यदि (not i_embedded) {
_       (आरोपय_शीर्षकम्(l));
    }

    यावत् (true) {
        u8  sectionId   = 0;
        u32 section_आकार = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        यदि (GetU8(l, &sectionId)) {
            भङ्ग;
        }

_       (GetLEB_u32(l, &section_आकार));

        _throwif("the snapshot holds a section twice", l->आलोकितम्_Sections[sectionId]);
        l->आलोकितम्_Sections[sectionId] = true;
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
        l->अन्तः       = payload + section_आकार;
        l->अन्तर्_Section = true;

        {
            M3_Snapshot_आरोपय check = *l;
            check.checking       = true;
_           (आरोपय_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.अन्तः);
        }
        l->परिवर्तितम् = l->परिवर्तितम् or sectionId != d_m3SnapshotSection_Meta;
_       (आरोपय_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->अन्तः);
        l->अन्तर्_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->आलोकितम्_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->अङ्कः_Exceptions and not l->आलोकितम्_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->अङ्कः_Memories and not l->आलोकितम्_Sections[d_m3_Snapshot_Section_स्मृतिः]);
    _throwif("the snapshot has no table section",
             l->module->अङ्कः_Tables and not l->आलोकितम्_Sections[d_m3_Snapshot_Section_सारणी]);
    _throwif("the snapshot has no global section",
             l->module->अङ्कः_Globals and not l->आलोकितम्_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->अङ्कः_दत्त_Segments or l->module->अङ्कः_Element_Segments) and
               not l->आलोकितम्_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#यदि d_m3HasSnapshots

स्थिर
M3_परिणाम SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, शून्य* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_परिणाम       परिणाम = m3Err_none;
    M3SnapshotSave save;

    स्मृतिं_पूरय(&save, 0, परिमाण(save));

    IM3Continuation मूलम् = io_runtime->मूलम्_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not मूलम् or मूलम्->state != cont_suspended;

    IM3Function प्रविष्टिः = save.postmortem ? io_runtime->entered : मूलम्->प्रविष्टिः_Function;

    यदि (o_module) {
        *o_module = रिक्त;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not प्रविष्टिः);

    save.module = प्रविष्टिः->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    यदि (o_module) {
        *o_module = save.module;
    }

    प्रति (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->अग्र) {
        u32 id;
_       (Pointer_Ids_सङ्कलय(&save.जीवितम्_Continuations, cont, &id));
    }

#  यदि d_m3HasExceptionHandling
    प्रति (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->अग्र) {
        u32 id;
_       (Pointer_Ids_सङ्कलय(&save.जीवितम्_Exceptions, exception, &id));
    }
#  endif

    यदि (not save.postmortem) {
        u32 id;
_       (Pointer_Ids_सङ्कलय(&save.continuations, मूलम्, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    PointerIds_Free(&save.जीवितम्_Continuations);
    PointerIds_Free(&save.जीवितम्_Exceptions);
    PointerIds_Free(&save.continuations);
    PointerIds_Free(&save.exceptions);

    प्रत्यागम परिणाम;
}

#यदि_अन्तः

M3_परिणाम m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, शून्य* i_userdata)
{
    यदि (not io_runtime or not i_writer) {
        प्रत्यागम m3Err_mallocFailed;
    }

#यदि d_m3HasSnapshots
    प्रत्यागम SaveSnapshot(io_runtime, i_writer, i_userdata, रिक्त, false);
#अन्यथा
    (शून्य)i_userdata;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#यदि_अन्तः
}


स्थिर
M3_परिणाम आरोपय_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, शून्य* i_userdata, bool i_embedded)
{
    M3_परिणाम परिणाम = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#यदि d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_सततम्);

    {
        M3_Snapshot_आरोपय आरोपय;
        स्मृतिं_पूरय(&आरोपय, 0, परिमाण(आरोपय));

        आरोपय.runtime  = io_runtime;
        आरोपय.module   = i_module;
        आरोपय.reader   = i_reader;
        आरोपय.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        परिणाम = आरोपय_Sections(&आरोपय, i_embedded);

        IM3Continuation मूलम् = io_runtime->मूलम्_Continuation;

        यदि (परिणाम) {
            यदि (आरोपय.परिवर्तितम्) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                यदि (मूलम्) {
                    मूलम्->state     = cont_returned;
                    मूलम्->अङ्कः_चौकटाः = 0;
                }
            }
        } अन्यथा {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->startFunction = -1;
            i_module->has_सततम्        = true;
        }

        Runtime_Place_Call_राशिः(io_runtime);

        m3_Free(आरोपय.check_राशिः);
        m3_Free(आरोपय.चौकटाः);
        m3_Free(आरोपय.suspendSites);
        m3_Free(आरोपय.continuations);
#  यदि d_m3HasExceptionHandling
        m3_Free(आरोपय.exceptions);
#  endif
    }
#अन्यथा
    (शून्य)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#यदि_अन्तः

_catch:
    प्रत्यागम परिणाम;
}


M3_परिणाम m3_आरोपय_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, शून्य* i_userdata)
{
    प्रत्यागम आरोपय_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


शून्य m3_समुच्चयः_Snapshot_Hooks (IM3Runtime io_runtime, ध्रुव M3SnapshotHooks* i_hooks)
{
#यदि d_m3HasSnapshots
    यदि (io_runtime) {
        यदि (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } अन्यथा {
            स्मृतिं_पूरय(&io_runtime->snapshotHooks, 0, परिमाण(io_runtime->snapshotHooks));
        }
    }
#अन्यथा
    (शून्य)io_runtime;
    (शून्य)i_hooks;
#यदि_अन्तः
}


M3_परिणाम m3_Save_Snapshot_गन्तव्यम्_बफर (IM3Runtime io_runtime, शून्य** o_अष्टकानि, size_t* o_आकार)
{
    यदि (!io_runtime || !o_अष्टकानि || !o_आकार) {
        प्रत्यागम m3Err_mallocFailed;
    }
#यदि d_m3HasSnapshots
    M3_बफर_Writer bw     = { रिक्त, 0, 0 };
    M3_परिणाम       परिणाम = m3_SaveSnapshot(io_runtime, बफर_Writer_Write, &bw);
    यदि (परिणाम) {
        m3_Free(bw.बफर);
        *o_अष्टकानि = रिक्त;
        *o_आकार  = 0;
        प्रत्यागम परिणाम;
    }
    *o_अष्टकानि = bw.बफर;
    *o_आकार  = bw.आकार;
    प्रत्यागम m3Err_none;
#अन्यथा
    *o_अष्टकानि = रिक्त;
    *o_आकार  = 0;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#यदि_अन्तः
}

#यदि d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
स्थिर
u8* EmitLEB_u32 (u8* o_where, u32 i_मानम्)
{
    कुरु {
        u8 byte = i_मानम् & 0x7F;
        i_मानम् >>= 7;
        यदि (i_मानम् != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } यावत् (i_मानम् != 0);

    प्रत्यागम o_where;
}

स्थिर
u32 आकार_सम्बन्धि_LEB_u32 (u32 i_मानम्)
{
    u32 आकार = 0;

    कुरु {
        i_मानम् >>= 7;
        ++आकार;
    } यावत् (i_मानम् != 0);

    प्रत्यागम आकार;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
स्थिर
M3_परिणाम FindEmbeddedSnapshot (ध्रुव u8* i_start, ध्रुव u8* i_अन्तः, cstr_t i_secName,
                               ध्रुव u8** o_secStart, ध्रुव u8** o_sec_अन्तः,
                               ध्रुव u8** o_दत्त, u32* o_आकार)
{
    M3_परिणाम परिणाम = m3Err_none;
    अष्टकानि_t  pos    = i_start;
    bool     प्राप्तम्  = false;
    cstr_t   name   = रिक्त;

    यदि (o_secStart) {
        *o_secStart = रिक्त;
    }
    यदि (o_sec_अन्तः) {
        *o_sec_अन्तः = रिक्त;
    }
    यदि (o_दत्त) {
        *o_दत्त = रिक्त;
    }
    यदि (o_आकार) {
        *o_आकार = 0;
    }

    _throwif(m3Err_wasmMalformed, i_अन्तः - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    यावत् (pos < i_अन्तः) {
        अष्टकानि_t secStart  = pos;
        u8      sectionId = *pos++;
        u32     आकार      = 0;
_       (ReadLEB_u32(&आकार, &pos, i_अन्तः));
        _throwif(m3Err_wasmUnderrun, आकार > (size_t)(i_अन्तः - pos));
        {
            अष्टकानि_t payload = pos;
            pos += आकार;
            यदि (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                यदि (वाक्यं_तुलय(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", प्राप्तम्);
                    प्राप्तम् = true;
                    यदि (o_secStart) {
                        *o_secStart = secStart;
                    }
                    यदि (o_sec_अन्तः) {
                        *o_sec_अन्तः = pos;
                    }
                    यदि (o_दत्त) {
                        *o_दत्त = payload;
                    }
                    यदि (o_आकार) {
                        *o_आकार = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    प्रत्यागम परिणाम;
}

// Builds the custom section name a snapshot of i_name lives under
स्थिर
M3_परिणाम SnapshotSectionName (cstr_t i_name, वर्ण* o_secName, size_t i_आकार)
{
    यदि (i_name and *i_name) {
        यदि (वाक्यदैर्घ्यम्(i_name) > d_m3_Max_Snapshot_Name_दीर्घता) {
            प्रत्यागम "the snapshot name is too long";
        }
        snprintf(o_secName, i_आकार, "snapshot.%s", i_name);
    } अन्यथा {
        snprintf(o_secName, i_आकार, "snapshot");
    }

    प्रत्यागम m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
स्थिर
M3_परिणाम ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_परिणाम परिणाम = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = रिक्त;

    प्रति (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->अग्र) {
        यदि (वाक्यं_तुलय(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    प्रत्यागम परिणाम;
}
#यदि_अन्तः

M3_परिणाम m3_आरोपय_Snapshot_From_बफर (IM3Runtime io_runtime, IM3Module i_module, ध्रुव शून्य* i_अष्टकानि, size_t i_आकार)
{
    यदि (!io_runtime || !i_module || !i_अष्टकानि) {
        प्रत्यागम m3Err_mallocFailed;
    }
#यदि d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    यदि (i_आकार >= 8 && memcmp(i_अष्टकानि, "\0asm", 4) == 0) {
        ध्रुव u8* दत्त   = रिक्त;
        u32       आकार   = 0;
        M3_परिणाम  परिणाम = FindEmbeddedSnapshot((ध्रुव u8*)i_अष्टकानि, (ध्रुव u8*)i_अष्टकानि + i_आकार,
                                                "snapshot", रिक्त, रिक्त, &दत्त, &आकार);
        यदि (परिणाम) {
            प्रत्यागम परिणाम;
        }
        यदि (not दत्त) {
            प्रत्यागम "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_बफर_Reader br = { दत्त, आकार, 0 };
            प्रत्यागम आरोपय_Snapshot(io_runtime, i_module, बफर_Reader_Read, &br, true);
        }
    }
    {
        M3_बफर_Reader br = { (ध्रुव u8*)i_अष्टकानि, i_आकार, 0 };
        प्रत्यागम आरोपय_Snapshot(io_runtime, i_module, बफर_Reader_Read, &br,
                            i_आकार < 4 or memcmp(i_अष्टकानि, c_snapshotMagic, 4) != 0);
    }
#अन्यथा
    (शून्य)i_आकार;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#यदि_अन्तः
}

bool m3_HasSnapshot (IM3Module i_module, ध्रुव वर्ण* i_name)
{
    यदि (not i_module) {
        प्रत्यागम false;
    }
#यदि d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = रिक्त;
    (शून्य)ModuleSnapshot(i_module, i_name, &snap);

    प्रत्यागम snap != रिक्त;
#अन्यथा
    (शून्य)i_name;
    प्रत्यागम false;
#यदि_अन्तः
}

M3_परिणाम m3_GetEmbeddedSnapshot (IM3Module i_module, ध्रुव वर्ण* i_name,
                                 ध्रुव शून्य** o_अष्टकानि, size_t* o_आकार)
{
    यदि (not i_module or not o_अष्टकानि or not o_आकार) {
        प्रत्यागम m3Err_mallocFailed;
    }

    *o_अष्टकानि = रिक्त;
    *o_आकार  = 0;

#यदि d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = रिक्त;
        M3_परिणाम            परिणाम = ModuleSnapshot(i_module, i_name, &snap);

        यदि (परिणाम) {
            प्रत्यागम परिणाम;
        }
        यदि (not snap) {
            प्रत्यागम "the module holds no embedded snapshot of that name";
        }

        *o_अष्टकानि = snap->दत्त;
        *o_आकार  = snap->आकार;

        प्रत्यागम m3Err_none;
    }
#अन्यथा
    (शून्य)i_name;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#यदि_अन्तः
}

M3_परिणाम m3_आरोपय_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ध्रुव वर्ण* i_name)
{
    यदि (not io_runtime or not i_module) {
        प्रत्यागम m3Err_mallocFailed;
    }
#यदि d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = रिक्त;
        M3_परिणाम            परिणाम = ModuleSnapshot(i_module, i_name, &snap);
        M3_बफर_Reader      br;

        यदि (परिणाम) {
            प्रत्यागम परिणाम;
        }
        यदि (not snap) {
            प्रत्यागम "the module holds no embedded snapshot of that name";
        }

        br.बफर = snap->दत्त;
        br.आकार   = snap->आकार;
        br.cursor = 0;

        प्रत्यागम आरोपय_Snapshot(io_runtime, i_module, बफर_Reader_Read, &br, true);
    }
#अन्यथा
    (शून्य)i_name;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#यदि_अन्तः
}


M3_परिणाम m3_Save_Snapshot_गन्तव्यम्_Module (IM3Runtime io_runtime, IM3Module i_module, ध्रुव वर्ण* i_name,
                                  शून्य** o_अष्टकानि, size_t* o_आकार)
{
#यदि d_m3HasSnapshots
    M3_परिणाम       परिणाम = m3Err_none;
    M3_बफर_Writer bw     = { रिक्त, 0, 0 };
    u8*            बहिर्गमनम्    = रिक्त;

    वर्ण      secName[d_m3_Max_Snapshot_Name_दीर्घता + परिमाण("snapshot.")];
    u32       nameLen, name_Len_Leb_आकार, payload_आकार, payload_आकार_Leb_आकार;
    size_t    orig_आकार, योग_आकार;
    ध्रुव u8* dropStart = रिक्त;
    ध्रुव u8* drop_अन्तः   = रिक्त;
    IM3Module saved     = रिक्त;
    u8*       p;

    यदि (not io_runtime or not i_module or not o_अष्टकानि or not o_आकार) {
        प्रत्यागम m3Err_mallocFailed;
    }

    *o_अष्टकानि = रिक्त;
    *o_आकार  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasmStart or i_module->wasm_अन्तः <= i_module->wasmStart);

_   (SnapshotSectionName(i_name, secName, परिमाण(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->मूलम्_Continuation or io_runtime->मूलम्_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, बफर_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.आकार);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.आकार > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (FindEmbeddedSnapshot(i_module->wasmStart, i_module->wasm_अन्तः, secName, &dropStart, &drop_अन्तः, रिक्त, रिक्त));

    orig_आकार           = (size_t)(i_module->wasm_अन्तः - i_module->wasmStart) - (dropStart ? (size_t)(drop_अन्तः - dropStart) : 0);
    nameLen            = (u32)वाक्यदैर्घ्यम्(secName);
    name_Len_Leb_आकार     = आकार_सम्बन्धि_LEB_u32(nameLen);
    payload_आकार        = name_Len_Leb_आकार + nameLen + (u32)bw.आकार;
    payload_आकार_Leb_आकार = आकार_सम्बन्धि_LEB_u32(payload_आकार);
    योग_आकार          = orig_आकार + 1 + payload_आकार_Leb_आकार + payload_आकार;

    बहिर्गमनम् = (u8*)m3_Malloc("wasm_snapshot_module", योग_आकार);
    _throwifnull(बहिर्गमनम्);

    p = बहिर्गमनम्;

    यदि (dropStart) {
        size_t before = (size_t)(dropStart - i_module->wasmStart);
        size_t after  = (size_t)(i_module->wasm_अन्तः - drop_अन्तः);

        स्मृतिं_प्रतिलिख(p, i_module->wasmStart, before);
        p += before;
        स्मृतिं_प्रतिलिख(p, drop_अन्तः, after);
        p += after;
    } अन्यथा {
        स्मृतिं_प्रतिलिख(p, i_module->wasmStart, orig_आकार);
        p += orig_आकार;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_आकार);
    p    = EmitLEB_u32(p, nameLen);
    स्मृतिं_प्रतिलिख(p, secName, nameLen);
    p += nameLen;
    स्मृतिं_प्रतिलिख(p, bw.बफर, bw.आकार);

    *o_अष्टकानि = बहिर्गमनम्;
    *o_आकार  = योग_आकार;
    बहिर्गमनम्      = रिक्त;

_catch:
    m3_Free(bw.बफर);
    m3_Free(बहिर्गमनम्);

    प्रत्यागम परिणाम;
#अन्यथा
    (शून्य)io_runtime;
    (शून्य)i_module;
    (शून्य)i_name;
    (शून्य)o_अष्टकानि;
    (शून्य)o_आकार;
    प्रत्यागम "snapshots are not available in this build of Wasm3";
#यदि_अन्तः
}
