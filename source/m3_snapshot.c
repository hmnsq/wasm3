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

#شامل "m3_env.h"
#شامل "m3_compile.h"
#شامل "m3_exception.h"
#شامل "m3_host.h"

#شامل <limits.h>

#اگر d_m3HasSnapshots

//---------------------------------------------------------------------------------------------------------------------------------
//  format
//---------------------------------------------------------------------------------------------------------------------------------

// The container opens the way a Wasm binary does: four bytes of magic and a
// fixed 32-bit version, little endian. Eight bytes, at a fixed place, whatever
// the version turns out to say.
ساکن ثابت u8  c_snapshotMagic[4] = { 0x00, 'd', 'm', 'p' };
ساکن ثابت u32 c_snapshot_نسخہ  = 1;

شمار {
    d_m3SnapshotSection_Meta         = 0,
    d_m3_Snapshot_Section_حافظہ       = 1,
    d_m3_Snapshot_Section_جدول        = 2,
    d_m3SnapshotSection_Global       = 3,
    d_m3SnapshotSection_Segment      = 4,
    d_m3SnapshotSection_Exception    = 5,
    d_m3SnapshotSection_Continuation = 6,
    d_m3SnapshotSection_HostState    = 7,
};

#  define d_m3SnapshotFlagPostmortem  0x1

// The longest <name> in a "snapshot.<name>" custom section. A name is a label
// the embedder picks, not something the format needs room to grow.
#  define d_m3_Max_Snapshot_Name_لمبائی   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
شمار {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_ختم_شدہ  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_انتہا                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//شناختی_عنصر
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ساکن
M3_نتیجہ ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_آغاز or i_module->wasm_انتہا - i_module->wasm_آغاز < 8);

    {
        بائٹیں_t  pos = i_module->wasm_آغاز + 8;
        cbytes_t انتہا = i_module->wasm_انتہا;

        Xxh64_Init(&hash, 0);

        جب (pos < انتہا) {
            بائٹیں_t آغاز     = pos;
            u8      sectionId = *pos++;
            u32     سائز      = 0;

_           (ReadLEB_u32(&سائز, &pos, انتہا));
            _throwif(m3Err_wasmUnderrun, سائز > (size_t)(انتہا - pos));

            pos += سائز;
            اگر (sectionId != 0) {
                Xxh64_Update(&hash, آغاز, (size_t)(pos - آغاز));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    واپس نتیجہ;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ساکن
ثابت M3_محفوظ_نقطہ* ڈھونڈو_محفوظ_نقطہ (IM3Function i_function, pc_t i_pc, u8 i_قسم)
{
    ثابت M3SnapshotMap* map = i_function->snapshotMap;

    اگر (map) {
        برائے (u32 i = 0; i < map->عدد_محفوظ_Points; ++i) {
            اگر (map->محفوظ_Points[i].pc == i_pc and map->محفوظ_Points[i].قسم == i_قسم) {
                واپس &map->محفوظ_Points[i];
            }
        }
    }

    واپس تہی;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
ساکن
ثابت M3_محفوظ_نقطہ* ڈھونڈو_محفوظ_نقطہ_At (ثابت M3SnapshotMap* i_map, u32 i_wasm_انحراف, u8 i_قسم, u32 i_ہدف_Loop)
{
    u32 کم  = 0;
    u32 بلند = i_map->عدد_محفوظ_Points;

    جب (کم < بلند) {
        u32 درمیان = کم + (بلند - کم) / 2;

        اگر (i_map->محفوظ_Points[درمیان].wasm_انحراف < i_wasm_انحراف) {
            کم = درمیان + 1;
        } ورنہ {
            بلند = درمیان;
        }
    }

    برائے (u32 i = کم; i < i_map->عدد_محفوظ_Points and i_map->محفوظ_Points[i].wasm_انحراف == i_wasm_انحراف; ++i) {
        ثابت M3_محفوظ_نقطہ* نقطہ = &i_map->محفوظ_Points[i];

        اگر (نقطہ->قسم != i_قسم or (نقطہ->flags & d_m3_محفوظ_نقطہ_دم_Call)) {
            جاری;
        }
        اگر (i_قسم != safepoint_op or i_map->بلاکیں[نقطہ->aux].wasm_انحراف == i_ہدف_Loop) {
            واپس نقطہ;
        }
    }

    واپس تہی;
}

ساکن
ثابت M3_بلاک_آغاز* ڈھونڈو_بلاک_At (ثابت M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_انحراف)
{
    برائے (u32 i = 0; i < i_map->عدد_بلاکیں; ++i) {
        اگر (i_map->بلاکیں[i].opcode == i_opcode and i_map->بلاکیں[i].wasm_انحراف == i_wasm_انحراف) {
            واپس &i_map->بلاکیں[i];
        }
    }

    واپس تہی;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ساکن اندرونی
bool Encloses_انحراف (ثابت M3_بلاک_آغاز* i_بلاک, u32 i_wasm_انحراف)
{
    واپس i_بلاک->wasm_انحراف < i_wasm_انحراف and i_wasm_انحراف < i_بلاک->wasm_انتہا;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ساکن اندرونی
bool Handlers_زندہ_At (ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, ثابت M3_بلاک_آغاز* i_try)
{
    واپس not (i_نقطہ->قسم == safepoint_op and i_try->wasm_انحراف > i_map->بلاکیں[i_نقطہ->aux].wasm_انحراف);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ساکن
M3_نتیجہ CallSiteType (IM3Function i_caller, ثابت M3_محفوظ_نقطہ* i_نقطہ, IM3FuncType* o_type)
{
    M3_نتیجہ   نتیجہ = m3Err_none;
    IM3Module  module = i_caller->module;
    بائٹیں_t    pos    = i_caller->wasm;
    cbytes_t   انتہا    = i_caller->wasm_انتہا;
    u32        سائز   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = تہی;

_   (ReadLEB_u32(&سائز, &pos, انتہا));
    _throwif(m3Err_wasmUnderrun, i_نقطہ->wasm_انحراف >= (size_t)(انتہا - pos));
    pos += i_نقطہ->wasm_انحراف;

_   (Read_opcode(&opcode, &pos, انتہا));
_   (ReadLEB_u32(&index, &pos, انتہا));

    اگر (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->عدد_Functions);
        *o_type = module->functions[index].funcType;
    } ورنہ {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_بالواسطہ and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->عدد_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    واپس نتیجہ;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ساکن
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 عدد_Results = Get_Func_Type_عدد_Results(i_expected);

    اگر (not i_type or Get_Func_Type_عدد_Results(i_type) != عدد_Results) {
        واپس false;
    }

    برائے (u16 i = 0; i < عدد_Results; ++i) {
        اگر (not Is_Sub_Type_کا(Get_Func_Type_نتیجہ_Type(i_type, i), Get_Func_Type_نتیجہ_Type(i_expected, i))) {
            واپس false;
        }
    }

    واپس true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
قسمتعریف ساخت M3_چوکھٹا_Registers {
    m3reg_t* r0;
#  اگر d_m3HasFloat
    f64* fp0;
#  endif
} M3_چوکھٹا_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
ساکن
M3_نتیجہ Continuation_ڈھیر (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_بنیاد, u32* o_عدد_Slots)
{
    اگر (i_cont == i_runtime->جڑ_Continuation) {
        *o_بنیاد     = (m3slot_t*)i_runtime->origin_ڈھیر;
        *o_عدد_Slots = i_runtime->عدد_ڈھیر_Slots;
    } ورنہ {
        *o_بنیاد     = i_cont->val_ڈھیر;
        *o_عدد_Slots = i_cont->عدد_ڈھیر_Slots;
    }

    واپس (*o_بنیاد) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ساکن
M3_نتیجہ Locate_قدر (m3slot_t* i_بنیاد, u32 i_عدد_Slots, m3stack_t i_sp, ثابت M3_خانہ_قدر* i_قدر, u8** o_where)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       type   = بنیاد_Type_کا(i_قدر->type);

    *o_where = تہی;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_رجسٹر_خانہ_Alias(i_قدر->خانہ));

    اگر (not Is_رجسٹر_خانہ_Alias(i_قدر->خانہ)) {
        size_t سائز   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? حجم(خالی*)
                                                                   : (Is64_ثنائی_ہندسہ_Type(type) ? حجم(u64) : حجم(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_بنیاد + i_قدر->خانہ) * حجم(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + سائز > (size_t)i_عدد_Slots * حجم(m3slot_t));

        *o_where = (u8*)i_بنیاد + offset;
    }

_catch:
    واپس نتیجہ;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ساکن
M3_نتیجہ Read_قدر (u8* i_where, ثابت M3_چوکھٹا_Registers* i_registers, ثابت M3_خانہ_قدر* i_قدر, u64* o_بٹ,
                    خالی** o_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       type   = بنیاد_Type_کا(i_قدر->type);

    *o_بٹ      = 0;
    *o_reference = تہی;

    اگر (i_where) {
        اگر (IsRefType(type)) {
            حافظہ_نقل(o_reference, i_where, حجم(خالی*));
        } ورنہ اگر (Is64_ثنائی_ہندسہ_Type(type)) {
            حافظہ_نقل(o_بٹ, i_where, حجم(u64));
        } ورنہ {
            u32 narrow;
            حافظہ_نقل(&narrow, i_where, حجم(narrow));
            *o_بٹ = narrow;
        }
    } ورنہ اگر (Is_Fp_رجسٹر_خانہ_Alias(i_قدر->خانہ)) {
#  اگر d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        اگر (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 بٹ;
            حافظہ_نقل(&بٹ, &narrow, حجم(بٹ));
            *o_بٹ = بٹ;
        } ورنہ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            حافظہ_نقل(o_بٹ, i_registers->fp0, حجم(u64));
        }
#  ورنہ
        _throw(m3Err_wasmMalformed);
#  endif
    } ورنہ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        اگر (IsRefType(type)) {
            *o_reference = (خالی*)(uintptr_t)r0;
        } ورنہ اگر (Is64_ثنائی_ہندسہ_Type(type)) {
            *o_بٹ = (u64)r0;
        } ورنہ {
            *o_بٹ = (u32)r0;
        }
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Write_قدر (u8* o_where, ثابت M3_چوکھٹا_Registers* i_registers, ثابت M3_خانہ_قدر* i_قدر, u64 i_بٹ,
                     خالی* i_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       type   = بنیاد_Type_کا(i_قدر->type);

    اگر (o_where) {
        اگر (IsRefType(type)) {
            حافظہ_نقل(o_where, &i_reference, حجم(خالی*));
        } ورنہ اگر (Is64_ثنائی_ہندسہ_Type(type)) {
            حافظہ_نقل(o_where, &i_بٹ, حجم(u64));
        } ورنہ {
            u32 narrow = (u32)i_بٹ;
            حافظہ_نقل(o_where, &narrow, حجم(narrow));
        }
    } ورنہ اگر (Is_Fp_رجسٹر_خانہ_Alias(i_قدر->خانہ)) {
#  اگر d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        اگر (type == c_m3Type_f32) {
            u32 بٹ = (u32)i_بٹ;
            f32 narrow;
            حافظہ_نقل(&narrow, &بٹ, حجم(narrow));
            *i_registers->fp0 = narrow;
        } ورنہ {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            حافظہ_نقل(i_registers->fp0, &i_بٹ, حجم(u64));
        }
#  ورنہ
        _throw(m3Err_wasmMalformed);
#  endif
    } ورنہ {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        اگر (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } ورنہ {
            *i_registers->r0 = (m3reg_t)i_بٹ;
        }
    }

_catch:
    واپس نتیجہ;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ساکن
ثابت M3_خانہ_قدر* چوکھٹا_قدر (ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, u32 i_اشاریہ)
{
    اگر (i_اشاریہ < i_map->عدد_Locals) {
        واپس i_نقطہ->پہلا_Local_قدر == UINT32_MAX ? &i_map->locals[i_اشاریہ]
                                                      : &i_map->اقدار[i_نقطہ->پہلا_Local_قدر + i_اشاریہ];
    }
    واپس &i_map->اقدار[i_نقطہ->پہلا_قدر + i_اشاریہ - i_map->عدد_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ساکن
خالی RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    اگر (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->عدد_Ret_And_Arg_Slots) + i_function->عدد_Local_بائٹیں;

        حافظہ_نقل(where, i_function->constants, i_function->عدد_Constant_بائٹیں);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
قسمتعریف ساخت M3_اشارہ_گر_Ids {
    خالی** اشیاء;
    u32    گنتی;
    u32    استعداد;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  اشاریہ_استعداد;       // a power of two, kept at least twice count
} M3_اشارہ_گر_Ids;

ساکن
u32 اشارہ_گر_بالٹی (خالی* i_اشارہ_گر, u32 i_استعداد)
{
    u64 بٹ = (u64)(uintptr_t)i_اشارہ_گر;

    واپس (u32)((بٹ * 0x9E3779B97F4A7C15ULL) >> 32) & (i_استعداد - 1);
}

ساکن
u32 اشارہ_گر_Ids_ڈھونڈو (ثابت M3_اشارہ_گر_Ids* i_ids, خالی* i_اشارہ_گر)
{
    اگر (i_ids->اشاریہ_استعداد) {
        u32 بالٹی = اشارہ_گر_بالٹی(i_اشارہ_گر, i_ids->اشاریہ_استعداد);

        جب (i_ids->index[بالٹی]) {
            u32 id = i_ids->index[بالٹی] - 1;

            اگر (i_ids->اشیاء[id] == i_اشارہ_گر) {
                واپس id;
            }

            بالٹی = (بالٹی + 1) & (i_ids->اشاریہ_استعداد - 1);
        }
    }

    واپس d_m3SnapshotNone;
}

ساکن
M3_نتیجہ اشارہ_گر_Ids_جوڑو (M3_اشارہ_گر_Ids* io_ids, خالی* i_اشارہ_گر, u32* o_id)
{
    u32 id = اشارہ_گر_Ids_ڈھونڈو(io_ids, i_اشارہ_گر);

    اگر (id != d_m3SnapshotNone) {
        *o_id = id;
        واپس m3Err_none;
    }

    اگر (io_ids->گنتی == io_ids->استعداد) {
        u32    استعداد = io_ids->استعداد ? io_ids->استعداد * 2 : 16;
        خالی** اشیاء    = m3_ReallocArray(خالی*, io_ids->اشیاء, استعداد, io_ids->استعداد);

        اگر (not اشیاء) {
            واپس m3Err_mallocFailed;
        }

        io_ids->اشیاء    = اشیاء;
        io_ids->استعداد = استعداد;
    }

    اگر ((io_ids->گنتی + 1) * 2 > io_ids->اشاریہ_استعداد) {
        u32  استعداد = io_ids->اشاریہ_استعداد ? io_ids->اشاریہ_استعداد * 2 : 32;
        u32* index    = m3_AllocArray(u32, استعداد);

        اگر (not index) {
            واپس m3Err_mallocFailed;
        }

        برائے (u32 i = 0; i < io_ids->گنتی; ++i) {
            u32 بالٹی = اشارہ_گر_بالٹی(io_ids->اشیاء[i], استعداد);

            جب (index[بالٹی]) {
                بالٹی = (بالٹی + 1) & (استعداد - 1);
            }

            index[بالٹی] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->اشاریہ_استعداد = استعداد;
    }

    id                = io_ids->گنتی++;
    io_ids->اشیاء[id] = i_اشارہ_گر;

    u32 بالٹی = اشارہ_گر_بالٹی(i_اشارہ_گر, io_ids->اشاریہ_استعداد);

    جب (io_ids->index[بالٹی]) {
        بالٹی = (بالٹی + 1) & (io_ids->اشاریہ_استعداد - 1);
    }

    io_ids->index[بالٹی] = id + 1;

    *o_id = id;

    واپس m3Err_none;
}

ساکن
خالی اشارہ_گر_Ids_Free (M3_اشارہ_گر_Ids* io_ids)
{
    m3_Free(io_ids->اشیاء);
    m3_Free(io_ids->index);
    memset(io_ids, 0, حجم(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

قسمتعریف ساخت M3_بفر_Writer {
    u8*    بفر;
    size_t سائز;
    size_t استعداد;
} M3_بفر_Writer;

ساکن
M3_نتیجہ بفر_Writer_Write (ثابت خالی* i_ڈیٹا, size_t i_سائز, خالی* i_userdata)
{
    M3_بفر_Writer* bw = (M3_بفر_Writer*)i_userdata;

    اگر (bw->سائز + i_سائز > bw->استعداد) {
        size_t newCap = bw->استعداد ? bw->استعداد * 2 : 4096;
        جب (bw->سائز + i_سائز > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->بفر, newCap, bw->استعداد);
        اگر (!newBuf) {
            واپس m3Err_mallocFailed;
        }
        bw->بفر   = newBuf;
        bw->استعداد = newCap;
    }

    اگر (i_سائز) {
        حافظہ_نقل(bw->بفر + bw->سائز, i_ڈیٹا, i_سائز);
        bw->سائز += i_سائز;
    }

    واپس m3Err_none;
}

قسمتعریف ساخت M3_بفر_Reader {
    ثابت u8* بفر;
    size_t    سائز;
    size_t    cursor;
} M3_بفر_Reader;

ساکن
M3_نتیجہ بفر_Reader_Read (خالی* o_بفر, size_t i_سائز, خالی* i_userdata)
{
    M3_بفر_Reader* br = (M3_بفر_Reader*)i_userdata;

    اگر (i_سائز > br->سائز - br->cursor) {
        واپس m3Err_wasmMalformed;
    }

    اگر (i_سائز) {
        حافظہ_نقل(o_بفر, br->بفر + br->cursor, i_سائز);
        br->cursor += i_سائز;
    }

    واپس m3Err_none;
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
قسمتعریف ساخت M3SnapshotSave {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotWriter writer;
    خالی*            userdata;

    bool postmortem;
    bool embedded;
    bool writing;

    M3_اشارہ_گر_Ids زندہ_Continuations;     // everything the runtime still holds, to check
    M3_اشارہ_گر_Ids زندہ_Exceptions;        //   a reference against before following it
    M3_اشارہ_گر_Ids continuations;         // what the snapshot carries, by id
    M3_اشارہ_گر_Ids exceptions;
} M3SnapshotSave;

ساکن اندرونی
u8 M3_Type_مقصد_Val_Type (m3type_t i_type)
{
    انتخاب (بنیاد_Type_کا(i_type)) {
    حالت c_m3Type_i32: واپس 0x7F;
    حالت c_m3Type_i64: واپس 0x7E;
    حالت c_m3Type_f32: واپس 0x7D;
    حالت c_m3Type_f64: واپس 0x7C;
    حالت c_m3Type_v128: واپس 0x7B;
    حالت c_m3Type_funcref: واپس 0x70;
    حالت c_m3Type_externref: واپس 0x6F;
    حالت c_m3Type_exnref: واپس 0x69;
    حالت c_m3Type_contref: واپس 0x68;
    طےشدہ: واپس 0x00;
    }
}

ساکن اندرونی
m3type_t Val_Type_مقصد_M3_Type (u8 i_valtype)
{
    انتخاب (i_valtype) {
    حالت 0x7F: واپس c_m3Type_i32;
    حالت 0x7E: واپس c_m3Type_i64;
    حالت 0x7D: واپس c_m3Type_f32;
    حالت 0x7C: واپس c_m3Type_f64;
    حالت 0x7B: واپس c_m3Type_v128;
    حالت 0x70: واپس c_m3Type_funcref;
    حالت 0x6F: واپس c_m3Type_externref;
    حالت 0x69: واپس c_m3Type_exnref;
    حالت 0x68: واپس c_m3Type_contref;
    طےشدہ: واپس c_m3Type_none;
    }
}

ساکن
M3_نتیجہ Put (M3SnapshotSave* s, ثابت خالی* i_ڈیٹا, size_t i_سائز)
{
    واپس s->writing ? s->writer(i_ڈیٹا, i_سائز, s->userdata) : m3Err_none;
}

ساکن
M3_نتیجہ PutU8 (M3SnapshotSave* s, u8 i_قدر)
{
    واپس Put(s, &i_قدر, حجم(i_قدر));
}


ساکن
M3_نتیجہ PutLEB_u32 (M3SnapshotSave* s, u32 i_قدر)
{
    u8  بائٹیں[5];
    u32 len = 0;

    کرو {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        اگر (i_قدر != 0) {
            byte |= 0x80;
        }
        بائٹیں[len++] = byte;
    } جب (i_قدر != 0);

    واپس Put(s, بائٹیں, len);
}

ساکن
M3_نتیجہ PutLEB_u64 (M3SnapshotSave* s, u64 i_قدر)
{
    u8  بائٹیں[10];
    u32 len = 0;

    کرو {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        اگر (i_قدر != 0) {
            byte |= 0x80;
        }
        بائٹیں[len++] = byte;
    } جب (i_قدر != 0);

    واپس Put(s, بائٹیں, len);
}

ساکن
M3_نتیجہ PutLEB_i64 (M3SnapshotSave* s, i64 i_قدر)
{
    u8   بائٹیں[10];
    u32  len  = 0;
    bool مزید = true;

    جب (مزید) {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        اگر ((i_قدر == 0 && !(byte & 0x40)) || (i_قدر == -1 && (byte & 0x40))) {
            مزید = false;
        } ورنہ {
            byte |= 0x80;
        }
        بائٹیں[len++] = byte;
    }

    واپس Put(s, بائٹیں, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
قسمتعریف M3_نتیجہ (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ساکن
M3_نتیجہ SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_نتیجہ         نتیجہ   = m3Err_none;
    M3_بفر_Writer   bw       = { تہی, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    خالی*            userdata = s->userdata;

    s->writer   = بفر_Writer_Write;
    s->userdata = &bw;

    نتیجہ = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    اگر (نتیجہ) {
        جاؤ _catch;
    }

    // a section with nothing in it is left out
    اگر (bw.سائز) {
        _throwif("snapshot section is too large", bw.سائز > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.سائز));
_       (Put(s, bw.بفر, bw.سائز));
    }

_catch:
    m3_Free(bw.بفر);

    واپس نتیجہ;
}

ساکن
u32 Function_اشاریہ (IM3Module i_module, IM3Function i_function)
{
    اگر (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->عدد_Functions) {
        واپس (u32)(i_function - i_module->functions);
    }

    واپس d_m3SnapshotNone;
}

ساکن
u32 Func_Type_اشاریہ (IM3Module i_module, IM3FuncType i_type)
{
    برائے (u32 i = 0; i < i_module->عدد_Func_Types; ++i) {
        اگر (i_module->funcTypes[i] == i_type) {
            واپس i;
        }
    }

    واپس d_m3SnapshotNone;
}

#  اگر d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
ساکن
IM3_لیبل لیبل_کا_اشاریہ (IM3Module i_module, u32 i_اشاریہ)
{
    IM3_لیبل لیبل = &i_module->لیبل_فہرست[i_اشاریہ];

    واپس لیبل->resolved ? لیبل->resolved : لیبل;
}

ساکن
u32 لیبل_اشاریہ_2 (IM3Module i_module, IM3_لیبل i_لیبل)
{
    برائے (u32 i = 0; i < i_module->عدد_لیبل_فہرست; ++i) {
        اگر (لیبل_کا_اشاریہ(i_module, i) == i_لیبل) {
            واپس i;
        }
    }

    واپس d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ساکن
M3_نتیجہ NameReference (M3SnapshotSave* s, u8 i_type, خالی* i_reference, u64* o_لفظ)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_لفظ = d_m3SnapshotNullRef;

    اگر (not i_reference) {
        واپس m3Err_none;
    }

    اگر (i_type == c_m3Type_funcref) {
        id = Function_اشاریہ(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } ورنہ اگر (i_type == c_m3Type_externref) {
        ثابت M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_لفظ = name;
        جاؤ _catch;
    }
#  اگر d_m3HasExceptionHandling
    ورنہ اگر (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 اشارہ_گر_Ids_ڈھونڈو(&s->زندہ_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 گنتی = s->exceptions.گنتی;
_       (اشارہ_گر_Ids_جوڑو(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.گنتی != گنتی);
    }
#  endif
    ورنہ اگر (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 اشارہ_گر_Ids_ڈھونڈو(&s->زندہ_Continuations, i_reference) == d_m3SnapshotNone);

        u32 گنتی = s->continuations.گنتی;
_       (اشارہ_گر_Ids_جوڑو(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.گنتی != گنتی);
    } ورنہ {
        _throw("a reference of an unknown type");
    }

    *o_لفظ = id;

_catch:
    واپس نتیجہ;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ساکن
M3_نتیجہ Put_Ref_قدر (M3SnapshotSave* s, u8 i_type, خالی* i_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u64      لفظ   = 0;

_   (NameReference(s, i_type, i_reference, &لفظ));

    اگر (not s->writing) {
        واپس m3Err_none;
    }

    اگر (لفظ == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } ورنہ {
        u8 قسم = 0;
        اگر (i_type == c_m3Type_funcref) {
            قسم = 0x01;
        } ورنہ اگر (i_type == c_m3Type_externref) {
            قسم = 0x02;
        }
#  اگر d_m3HasExceptionHandling
        ورنہ اگر (i_type == c_m3Type_exnref) {
            قسم = 0x03;
        }
#  endif
        ورنہ اگر (i_type == c_m3Type_contref) {
            قسم = 0x04;
        }

_       (PutU8(s, قسم));
_       (PutLEB_u64(s, لفظ));
    }

_catch:
    واپس نتیجہ;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ساکن
M3_نتیجہ PutLE32 (M3SnapshotSave* s, u32 i_قدر)
{
    u8 بائٹیں[4];

    برائے (u32 i = 0; i < حجم(بائٹیں); ++i) {
        بائٹیں[i] = (u8)(i_قدر >> (8 * i));
    }

    واپس Put(s, بائٹیں, حجم(بائٹیں));
}

ساکن
M3_نتیجہ PutLE64 (M3SnapshotSave* s, u64 i_قدر)
{
    u8 بائٹیں[8];

    برائے (u32 i = 0; i < حجم(بائٹیں); ++i) {
        بائٹیں[i] = (u8)(i_قدر >> (8 * i));
    }

    واپس Put(s, بائٹیں, حجم(بائٹیں));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ساکن
M3_نتیجہ Put_قدر_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_بٹ)
{
    اگر (IsRefType(i_type)) {
        واپس Put_Ref_قدر(s, بنیاد_Type_کا(i_type), (خالی*)(uintptr_t)i_بٹ);
    }

    انتخاب (بنیاد_Type_کا(i_type)) {
    حالت c_m3Type_i32: واپس PutLEB_i64(s, (i32)(u32)i_بٹ);
    حالت c_m3Type_i64: واپس PutLEB_i64(s, (i64)i_بٹ);
    حالت c_m3Type_f32: واپس PutLE32(s, (u32)i_بٹ);
    حالت c_m3Type_f64: واپس PutLE64(s, i_بٹ);
    طےشدہ: واپس "a snapshot cannot hold a value of this type as bits";
    }
}

ساکن
M3_نتیجہ Put_قدر_لیبل (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_مقصد_Val_Type(i_type);

    اگر (not valtype) {
        واپس "a snapshot has no tag for a value of this type";
    }

    واپس PutU8(s, valtype);
}

ساکن
M3_نتیجہ Put_قدر_بٹ (M3SnapshotSave* s, m3type_t i_type, u64 i_بٹ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Put_قدر_لیبل(s, i_type));
_   (Put_قدر_Body(s, i_type, i_بٹ));

_catch:
    واپس نتیجہ;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ساکن
M3_نتیجہ Put_قدر (M3SnapshotSave* s, m3type_t i_type, ثابت خالی* i_where)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Put_قدر_لیبل(s, i_type));

    اگر (IsRefType(i_type)) {
        خالی* reference;
        حافظہ_نقل(&reference, i_where, حجم(reference));
_       (Put_قدر_Body(s, i_type, (u64)(uintptr_t)reference));
    } ورنہ اگر (بنیاد_Type_کا(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 بائٹیں[16];
        حافظہ_نقل(بائٹیں, i_where, حجم(بائٹیں));
_       (Put(s, بائٹیں, حجم(بائٹیں)));
    } ورنہ اگر (Is64_ثنائی_ہندسہ_Type(i_type)) {
        u64 بٹ;
        حافظہ_نقل(&بٹ, i_where, حجم(بٹ));
_       (Put_قدر_Body(s, i_type, بٹ));
    } ورنہ {
        u32 narrow;
        حافظہ_نقل(&narrow, i_where, حجم(narrow));
_       (Put_قدر_Body(s, i_type, narrow));
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Put_حافظہ_Chunks (M3SnapshotSave* s, ثابت u8* i_بائٹیں, size_t i_سائز)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    size_t   cursor = 0;

    جب (cursor < i_سائز) {
        u8     byte = i_بائٹیں[cursor];
        size_t تسلسل  = 1;

        جب (cursor + تسلسل < i_سائز and i_بائٹیں[cursor + تسلسل] == byte) {
            تسلسل++;
        }

        اگر ((byte == 0x00 or byte == 0xFF) and تسلسل >= d_m3_Snapshot_تسلسل_حد) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            اگر (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)تسلسل));
            }

            cursor += تسلسل;
            جاری;
        }

        size_t raw_آغاز = cursor;

        جب (cursor < i_سائز) {
            u8     b      = i_بائٹیں[cursor];
            size_t لمبائی = 1;

            جب (cursor + لمبائی < i_سائز and i_بائٹیں[cursor + لمبائی] == b) {
                لمبائی++;
            }

            اگر ((b == 0x00 or b == 0xFF) and لمبائی >= d_m3_Snapshot_تسلسل_حد) {
                توڑو;
            }

            cursor += لمبائی;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_آغاز));
_       (PutLEB_u32(s, (u32)(cursor - raw_آغاز)));
_       (Put(s, i_بائٹیں + raw_آغاز, cursor - raw_آغاز));
    }

_   (PutU8(s, d_m3_Chunk_انتہا));

_catch:
    واپس نتیجہ;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ساکن
u32 حافظہ_پہلا_اشاریہ (IM3Module module, u32 index)
{
    u32 j = 0;
    جب (module->memories[j] != module->memories[index]) {
        ++j;
    }
    واپس j;
}

ساکن
u32 جدول_پہلا_اشاریہ (IM3Module module, u32 index)
{
    u32 j = 0;
    جب (module->tables[j] != module->tables[index]) {
        ++j;
    }
    واپس j;
}

ساکن
M3_نتیجہ SaveMemories (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Memories));

    برائے (u32 m = 0; m < module->عدد_Memories; ++m) {
        IM3_حافظہ حافظہ   = module->memories[m];
        u32       پہلا    = حافظہ_پہلا_اشاریہ(module, m);
        bool      has_ڈیٹا  = حافظہ and حافظہ->mallocated;
        u32       صفحہ_سائز = حافظہ ? حافظہ_صفحہ_سائز(حافظہ) : d_m3_Default_Mem_صفحہ_سائز;
        u64       بائٹیں    = حافظہ ? حافظہ->عدد_صفحات * (u64)صفحہ_سائز : 0;
        u32       صفحہ_بٹ = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, پہلا));

        اگر (پہلا != m) {
            جاری;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", حافظہ and حافظہ->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", بائٹیں > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 حافظہ and حافظہ->mallocated and حافظہ->mallocated->لمبائی < بائٹیں);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        جب ((1u << صفحہ_بٹ) < صفحہ_سائز) {
            ++صفحہ_بٹ;
        }

_       (PutLEB_u32(s, صفحہ_بٹ));
_       (PutLEB_u32(s, حافظہ ? (u32)حافظہ->عدد_صفحات : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        اگر (not s->writing) {
            جاری;
        }

        اگر (has_ڈیٹا) {
_           (Put_حافظہ_Chunks(s, m3_Mem_ڈیٹا(حافظہ->mallocated), (size_t)بائٹیں));
        } ورنہ {
_           (PutU8(s, d_m3_Chunk_انتہا));
        }
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ SaveGlobals (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Globals));

    برائے (u32 g = 0; g < module->عدد_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_قدر(s, global->type, &cell->i64_قدر));
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ SaveTables (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Tables));

    برائے (u32 t = 0; t < module->عدد_Tables; ++t) {
        IM3_جدول جدول_2 = module->tables[t];
        u32      پہلا = جدول_پہلا_اشاریہ(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, پہلا));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        اگر (پہلا != t) {
            جاری;
        }

_       (PutU8(s, M3_Type_مقصد_Val_Type(جدول_2->type)));
_       (PutLEB_u32(s, جدول_2->سائز));

        برائے (u32 e = 0; e < جدول_2->سائز; ++e) {
_           (Put_Ref_قدر(s, بنیاد_Type_کا(جدول_2->type), جدول_2->elements[e]));
        }
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ SaveSegments (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_ڈیٹا_Segments));

    برائے (u32 i = 0; i < module->عدد_ڈیٹا_Segments; ++i) {
_       (PutU8(s, module->ڈیٹا_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->عدد_Element_Segments));

    برائے (u32 i = 0; i < module->عدد_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    واپس نتیجہ;
}

#  اگر d_m3HasExceptionHandling

ساکن
M3_نتیجہ SaveExceptions (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.گنتی));

    برائے (u32 i = 0; i < s->exceptions.گنتی; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.اشیاء[i];
        IM3FuncType  type = exn->لیبل->type;

        _throwif(m3Err_wasmMalformed, exn->عدد_دلائل != Get_Func_Type_عدد_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->عدد_دلائل));

        برائے (u32 a = 0; a < exn->عدد_دلائل; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_قدر_بٹ(s, argType, exn->دلائل[a]));
        }
    }

_catch:
    واپس نتیجہ;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
قسمتعریف ساخت M3Activation {
    IM3Function        function;
    ثابت M3_محفوظ_نقطہ* نقطہ;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 قسم;            // M3SafePointKind
    M3_چوکھٹا_Registers   registers;       // the ones it goes on with
    ثابت M3_چوکھٹا*     resume;          // the resume it waits in, for safepoint_resume
    i32                پہلا_چوکھٹا;      // its frames, outermost first: firstFrame down to lastFrame
    i32                آخری_چوکھٹا;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ساکن
ثابت M3_محفوظ_نقطہ* ڈھونڈو_چوکھٹا_محفوظ_نقطہ (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_قسم)
{
    ثابت M3_محفوظ_نقطہ* نقطہ = *io_function ? ڈھونڈو_محفوظ_نقطہ(*io_function, i_pc, i_قسم) : تہی;

    برائے (u32 i = 0; not نقطہ and i < i_module->عدد_Functions; ++i) {
        نقطہ = ڈھونڈو_محفوظ_نقطہ(&i_module->functions[i], i_pc, i_قسم);
        اگر (نقطہ) {
            *io_function = &i_module->functions[i];
        }
    }

    واپس نقطہ;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ساکن
M3_نتیجہ اگلا_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_پہلا, M3Activation* o_act)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    i32      i      = i_پہلا;

    memset(o_act, 0, حجم(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->پہلا_چوکھٹا = i_پہلا;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    برائے (; i >= 0; --i) {
        ثابت M3_چوکھٹا* چوکھٹا = &i_cont->چوکھٹے[i];

        _throwif("a suspended frame runs against another module's memory", چوکھٹا->حافظہ != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", چوکھٹا->sp != i_sp);

        اگر (چوکھٹا->قسم == چوکھٹا_call) {
            o_act->pc           = چوکھٹا->pc;
            o_act->قسم         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&چوکھٹا->call.r0;
#  اگر d_m3HasFloat
            o_act->registers.fp0 = (f64*)&چوکھٹا->call.fp0;
#  endif
            توڑو;
        } ورنہ اگر (چوکھٹا->قسم == چوکھٹا_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = چوکھٹا->pc;
            o_act->قسم   = safepoint_resume;
            o_act->resume = چوکھٹا;
            توڑو;
        }
    }

    اگر (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->قسم         = i_cont->suspend_نقطہ;
        o_act->registers.r0 = &i_cont->r0;
#  اگر d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->آخری_چوکھٹا = i;
    o_act->نقطہ     = ڈھونڈو_چوکھٹا_محفوظ_نقطہ(i_module, &o_act->function, o_act->pc, o_act->قسم);

    _throwif("a suspended frame is not at a safepoint", not o_act->نقطہ);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  اگر d_m3_اندراج_Keeps_چوکھٹا
    برائے (i32 k = i_پہلا; k > o_act->آخری_چوکھٹا; --k) {
        ثابت M3_چوکھٹا* چوکھٹا = &i_cont->چوکھٹے[k];
        _throwif("a suspended frame is not the function it is entered with",
                 چوکھٹا->قسم == چوکھٹا_اندراج and چوکھٹا->اندراج_2.function != o_act->function);
    }
#  endif

_catch:
    واپس نتیجہ;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ساکن
M3_نتیجہ SaveActivation (M3SnapshotSave* s, m3slot_t* i_بنیاد, u32 i_عدد_Slots, ثابت M3Activation* i_act)
{
    M3_نتیجہ             نتیجہ        = m3Err_none;
    IM3Function          function      = i_act->function;
    ثابت M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_اشاریہ = Function_اشاریہ(s->module, function);
    ثابت M3_محفوظ_نقطہ*   نقطہ         = i_act->نقطہ;
    u32                  عدد_اقدار;

    _throwif("a suspended frame belongs to another module", function_اشاریہ == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_اشاریہ));
_   (PutU8(s, نقطہ->قسم));
_   (PutLEB_u32(s, نقطہ->wasm_انحراف));

    اگر (نقطہ->قسم == safepoint_op) {
_       (PutLEB_u32(s, map->بلاکیں[نقطہ->aux].wasm_انحراف));
    }

    عدد_اقدار = map->عدد_Locals + نقطہ->عدد_اقدار;

_   (PutLEB_u32(s, عدد_اقدار));

    برائے (u32 v = 0; v < عدد_اقدار; ++v) {
        ثابت M3_خانہ_قدر* قدر = چوکھٹا_قدر(map, نقطہ, v);
        u8*                where;
        u64                لفظ;
        خالی*              reference;

_       (Locate_قدر(i_بنیاد, i_عدد_Slots, i_act->sp, قدر, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        اگر (بنیاد_Type_کا(قدر->type) == c_m3Type_v128) {
_           (Put_قدر_لیبل(s, قدر->type));
_           (Put(s, where, 16));
            جاری;
        }

_       (Read_قدر(where, &i_act->registers, قدر, &لفظ, &reference));

_       (Put_قدر_بٹ(s, قدر->type, IsRefType(قدر->type) ? (u64)(uintptr_t)reference : لفظ));
    }

    اگر (نقطہ->قسم == safepoint_resume) {
_       (Put_Ref_قدر(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    واپس نتیجہ;
}

// A suspended continuation, as the functions it is in the middle of, outermost
// first. A call leaves the caller waiting and the callee next in; a resume the
// suspension passed through ends the list, since the rest of it belongs to the
// continuation that resume was running.
//
// A caller left waiting at a return_call compiled as a call is walked past and
// not written: in Wasm its callee has taken its place, and a build that reuses
// the frame has nothing there to write.
ساکن
M3_نتیجہ SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_نتیجہ      نتیجہ   = m3Err_none;
    M3Activation* acts     = تہی;
    u32           عدد_Acts  = 0;
    IM3Function   function = i_cont->اندراج_Function;
    i32           پہلا    = (i32)i_cont->عدد_چوکھٹے - 1;
    m3slot_t*     بنیاد;
    u32           عدد_Slots;
    m3stack_t     sp;

_   (Continuation_ڈھیر(s->runtime, i_cont, &بنیاد, &عدد_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->عدد_چوکھٹے + 1);
    _throwifnull(acts);

    sp = بنیاد;

    جب (true) {
        M3Activation* act = &acts[عدد_Acts];

_       (اگلا_Activation(s->module, i_cont, function, sp, پہلا, act));

        اگر (act->قسم != safepoint_call) {
            عدد_Acts++;
            توڑو;
        }

        _throwif("a suspended continuation stops at a call", act->آخری_چوکھٹا < 0);

        اگر (not (act->نقطہ->flags & d_m3_محفوظ_نقطہ_دم_Call)) {
            عدد_Acts++;
        }

        function = i_cont->چوکھٹے[act->آخری_چوکھٹا].call.function;
        sp       = act->sp + act->نقطہ->aux;
        پہلا    = act->آخری_چوکھٹا - 1;
    }

_   (PutLEB_u32(s, عدد_Acts));

    برائے (u32 a = 0; a < عدد_Acts; ++a) {
_       (SaveActivation(s, بنیاد, عدد_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    واپس نتیجہ;
}

ساکن
M3_نتیجہ SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_نتیجہ  نتیجہ    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_جڑ    = (i_cont == s->runtime->جڑ_Continuation);
    u8        state     = snapshot_cont_ختم_شدہ;
    u32       type_اشاریہ = is_جڑ ? d_m3SnapshotNone : Func_Type_اشاریہ(module, i_cont->type);

    اگر (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } ورنہ اگر (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } ورنہ اگر (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->اندراج_Function);
    _throwif("a continuation belongs to another module", Function_اشاریہ(module, i_cont->اندراج_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_جڑ and type_اشاریہ == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_جڑ));
_   (PutLEB_u32(s, type_اشاریہ));
_   (PutLEB_u32(s, Function_اشاریہ(module, i_cont->اندراج_Function)));
#  اگر d_m3HasExceptionHandling
_   (Put_Ref_قدر(s, c_m3Type_exnref, i_cont->resumeThrow));
#  ورنہ
_   (Put_Ref_قدر(s, c_m3Type_exnref, تہی));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    اگر (s->postmortem) {
        جاؤ _catch;
    }

    اگر (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->اندراج_Function ? i_cont->اندراج_Function->funcType : تہی;
        u16         عدد_Rets = inner ? Get_Func_Type_عدد_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_دلائل_گنتی > Get_Func_Type_عدد_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_دلائل_گنتی));

        برائے (u32 i = 0; i < i_cont->bound_دلائل_گنتی; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_قدر(s, type, i_cont->val_ڈھیر + (عدد_Rets + i) * c_io_خانہ_گنتی));
        }
    } ورنہ اگر (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_دلائل_گنتی));
_       (SaveSuspendedBody(s, i_cont));
    } ورنہ {
_       (PutLEB_u32(s, 0));
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ SaveContinuations (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (PutLEB_u32(s, s->continuations.گنتی));

    برائے (u32 i = 0; i < s->continuations.گنتی; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.اشیاء[i]));
    }

_catch:
    واپس نتیجہ;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ساکن
M3_نتیجہ SaveHostState (M3SnapshotSave* s)
{
    M3_نتیجہ               نتیجہ = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_بفر_Writer         bw     = { تہی, 0, 0 };

    اگر (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, بفر_Writer_Write, &bw));
    }

    اگر (bw.سائز > 0) {
_       (Put(s, bw.بفر, bw.سائز));
    }

_catch:
    m3_Free(bw.بفر);

    واپس نتیجہ;
}

ساکن
خالی SnapshotResourceTotals (IM3Module module, u64* حافظہ_بائٹیں, u64* جدول_Elements)
{
    *حافظہ_بائٹیں = *جدول_Elements = 0;
    برائے (u32 i = 0; i < module->عدد_Memories; ++i) {
        IM3_حافظہ حافظہ = module->memories[i];
        اگر (حافظہ_پہلا_اشاریہ(module, i) == i and حافظہ) {
            *حافظہ_بائٹیں += حافظہ->عدد_صفحات * (u64)حافظہ_صفحہ_سائز(حافظہ);
        }
    }
    برائے (u32 i = 0; i < module->عدد_Tables; ++i) {
        IM3_جدول جدول_2 = module->tables[i];
        اگر (جدول_پہلا_اشاریہ(module, i) == i and جدول_2) {
            *جدول_Elements += جدول_2->سائز;
        }
    }
}

ساکن
M3_نتیجہ SaveMeta (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u64      hash   = 0;
    u64      حافظہ_بائٹیں, جدول_Elements;
    u32      فعال_Stacks = 0;
    برائے (u32 i = 0; i < s->continuations.گنتی; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.اشیاء[i];
        اگر (cont != s->runtime->جڑ_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            فعال_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &حافظہ_بائٹیں, &جدول_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_وقت_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.گنتی));
_   (PutLEB_u32(s, s->exceptions.گنتی));
_   (PutLEB_u64(s, حافظہ_بائٹیں));
_   (PutLEB_u64(s, جدول_Elements));
_   (PutLEB_u32(s, فعال_Stacks));

#  اگر d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    برائے (u32 i = 0; i < s->exceptions.گنتی; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.اشیاء[i];
        u32          لیبل_اشاریہ  = لیبل_اشاریہ_2(s->module, exception->لیبل);

        _throwif("an exception was thrown with another module's tag", لیبل_اشاریہ == d_m3SnapshotNone);

_       (PutLEB_u32(s, لیبل_اشاریہ));
    }
#  endif

_catch:
    واپس نتیجہ;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ساکن
M3_نتیجہ DiscoverReferences (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ            = m3Err_none;
    u32      مکمل_Continuations = 0;
    u32      مکمل_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    جب (مکمل_Continuations < s->continuations.گنتی or مکمل_Exceptions < s->exceptions.گنتی) {
#  اگر d_m3HasExceptionHandling
        جب (مکمل_Exceptions < s->exceptions.گنتی) {
            M3Exception* exception = (M3Exception*)s->exceptions.اشیاء[مکمل_Exceptions++];
            IM3FuncType  type      = exception->لیبل->type;

            _throwif(m3Err_wasmMalformed, exception->عدد_دلائل != Get_Func_Type_عدد_Params(type));

            برائے (u32 a = 0; a < exception->عدد_دلائل; ++a) {
_               (Put_قدر_بٹ(s, GetFuncTypeParamType(type, (u16)a), exception->دلائل[a]));
            }
        }
#  endif
        جب (مکمل_Continuations < s->continuations.گنتی and مکمل_Exceptions == s->exceptions.گنتی) {
            u32 id = مکمل_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.اشیاء[id]));
        }
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ SaveSections (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (not s->writing) {
        واپس DiscoverReferences(s);
    }

    اگر (not s->embedded) {
_       (Put(s, c_snapshotMagic, حجم(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_نسخہ));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    اگر (s->module->عدد_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_حافظہ, SaveMemories));
    }
    اگر (s->module->عدد_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_جدول, SaveTables));
    }
    اگر (s->module->عدد_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    اگر (s->module->عدد_ڈیٹا_Segments or s->module->عدد_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  اگر d_m3HasExceptionHandling
    اگر (s->exceptions.گنتی) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    اگر (s->continuations.گنتی) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    واپس نتیجہ;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

قسمتعریف ساخت M3SnapshotSuspendSite {
    ثابت M3SnapshotMap* map;
    ثابت M3_محفوظ_نقطہ*   نقطہ;
    u32                  ہدف;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

قسمتعریف ساخت M3_Snapshot_بھرو {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    خالی*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ڈھیر;
    bool      بدلا;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    عدد_Continuations;
    u64                    required_حافظہ_بائٹیں;
    u64                    required_جدول_Elements;
    u64                    loaded_حافظہ_بائٹیں;
    u64                    loaded_جدول_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  اگر d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 عدد_Exceptions;

    M3_چوکھٹا* چوکھٹے;           // a suspended continuation's frames as they are put together, outermost first

    بائٹیں_t pos;               // bounded section payload, read using the Wasm parser's helpers
    بائٹیں_t انتہا;
    bool    اندر_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    نظر_آیا_Sections[256];
} M3_Snapshot_بھرو;

ساکن
M3_نتیجہ Get (M3_Snapshot_بھرو* l, خالی* o_ڈیٹا, size_t i_سائز)
{
    اگر (not l->اندر_Section) {
        واپس l->reader(o_ڈیٹا, i_سائز, l->userdata);
    }
    اگر (i_سائز > (size_t)(l->انتہا - l->pos)) {
        واپس m3Err_wasmUnderrun;
    }
    اگر (i_سائز) {
        حافظہ_نقل(o_ڈیٹا, l->pos, i_سائز);
        l->pos += i_سائز;
    }
    واپس m3Err_none;
}

ساکن
M3_نتیجہ GetU8 (M3_Snapshot_بھرو* l, u8* o_قدر)
{
    واپس Get(l, o_قدر, حجم(*o_قدر));
}

ساکن
M3_نتیجہ GetBool (M3_Snapshot_بھرو* l, u8* o_قدر)
{
    M3_نتیجہ نتیجہ = GetU8(l, o_قدر);
    واپس نتیجہ ? نتیجہ : (*o_قدر <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ساکن
M3_نتیجہ GetLEB_u32 (M3_Snapshot_بھرو* l, u32* o_قدر)
{
    اگر (l->اندر_Section) {
        واپس ReadLEB_u32(o_قدر, &l->pos, l->انتہا);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    برائے (u32 i = 0; i < حجم(encoded); ++i) {
        M3_نتیجہ نتیجہ = GetU8(l, &encoded[i]);
        اگر (نتیجہ) {
            واپس نتیجہ;
        }
        اگر (not (encoded[i] & 0x80)) {
            بائٹیں_t pos = encoded;
            واپس ReadLEB_u32(o_قدر, &pos, encoded + i + 1);
        }
    }
    واپس m3_Err_leb_حد_سے_تجاوز;
}

ساکن
M3_نتیجہ GetLEB_u64 (M3_Snapshot_بھرو* l, u64* o_قدر)
{
    واپس ReadLebUnsigned(o_قدر, 64, &l->pos, l->انتہا);
}

ساکن
M3_نتیجہ GetLEB_i64 (M3_Snapshot_بھرو* l, i64* o_قدر)
{
    واپس ReadLEB_i64(o_قدر, &l->pos, l->انتہا);
}

ساکن
M3_نتیجہ Skip_بائٹیں (M3_Snapshot_بھرو* l, size_t i_سائز)
{
    اگر (i_سائز > (size_t)(l->انتہا - l->pos)) {
        واپس m3Err_wasmUnderrun;
    }
    l->pos += i_سائز;
    واپس m3Err_none;
}


ساکن
M3_نتیجہ Get_Ref_قدر (M3_Snapshot_بھرو* l, m3type_t i_type, خالی** o_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       قسم   = 0;
    u64      id     = 0;

    *o_reference = تہی;

_   (GetU8(l, &قسم));
    اگر (قسم == 0x00) {
        واپس (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    اگر (قسم == 0x01) {
        _throwif(m3Err_wasmMalformed, بنیاد_Type_کا(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->عدد_Functions);
#  اگر d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_کا(Ref_Type_کا_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } ورنہ اگر (قسم == 0x02) {
        _throwif(m3Err_wasmMalformed, بنیاد_Type_کا(i_type) != c_m3Type_externref);
        ثابت M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        اگر (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  اگر d_m3HasExceptionHandling
    ورنہ اگر (قسم == 0x03) {
        _throwif(m3Err_wasmMalformed, بنیاد_Type_کا(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->عدد_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    ورنہ اگر (قسم == 0x04) {
        _throwif(m3Err_wasmMalformed, بنیاد_Type_کا(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->عدد_Continuations);
        *o_reference = l->continuations[id];
#  اگر d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        اگر (not l->checking and ڈھیر_Type_کا(i_type) != d_m3_Type_ڈھیر_Abstract) {
            m3type_t expected = i_type | d_m3Type_refNonNull;
            _throwif(m3Err_wasmMalformed, l->suspendSites[id].expectedType and
                                            l->suspendSites[id].expectedType != expected);
            l->suspendSites[id].expectedType = expected;
        }
#  endif
    } ورنہ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ GetLE32 (M3_Snapshot_بھرو* l, u32* o_قدر)
{
    u8       بائٹیں[4];
    M3_نتیجہ نتیجہ = Get(l, بائٹیں, حجم(بائٹیں));

    *o_قدر = 0;

    برائے (u32 i = 0; i < حجم(بائٹیں); ++i) {
        *o_قدر |= (u32)بائٹیں[i] << (8 * i);
    }

    واپس نتیجہ;
}

ساکن
M3_نتیجہ GetLE64 (M3_Snapshot_بھرو* l, u64* o_قدر)
{
    u8       بائٹیں[8];
    M3_نتیجہ نتیجہ = Get(l, بائٹیں, حجم(بائٹیں));

    *o_قدر = 0;

    برائے (u32 i = 0; i < حجم(بائٹیں); ++i) {
        *o_قدر |= (u64)بائٹیں[i] << (8 * i);
    }

    واپس نتیجہ;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ساکن
M3_نتیجہ Get_قدر_لیبل (M3_Snapshot_بھرو* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_نتیجہ نتیجہ  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_مقصد_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and بنیاد_Type_کا(*o_type) != بنیاد_Type_کا(i_expectedType));
    اگر (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    واپس نتیجہ;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ساکن
M3_نتیجہ Get_قدر_Body (M3_Snapshot_بھرو* l, m3type_t i_type, u64* o_بٹ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    *o_بٹ = 0;

    اگر (IsRefType(i_type)) {
        خالی* reference = تہی;
_       (Get_Ref_قدر(l, i_type, &reference));
        *o_بٹ = (u64)(uintptr_t)reference;
    } ورنہ اگر (بنیاد_Type_کا(i_type) == c_m3Type_i32) {
        i32 قدر = 0;
_       (ReadLEB_i32(&قدر, &l->pos, l->انتہا));
        *o_بٹ = (u32)قدر;
    } ورنہ اگر (بنیاد_Type_کا(i_type) == c_m3Type_i64) {
        i64 قدر = 0;
_       (GetLEB_i64(l, &قدر));
        *o_بٹ = (u64)قدر;
    } ورنہ اگر (بنیاد_Type_کا(i_type) == c_m3Type_f32) {
        u32 بٹ = 0;
_       (GetLE32(l, &بٹ));
        *o_بٹ = بٹ;
    } ورنہ اگر (بنیاد_Type_کا(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_بٹ));
    } ورنہ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Get_قدر_بٹ (M3_Snapshot_بھرو* l, m3type_t i_expectedType, u64* o_بٹ)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    m3type_t type;

_   (Get_قدر_لیبل(l, i_expectedType, &type));
_   (Get_قدر_Body(l, type, o_بٹ));

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ Get_قدر (M3_Snapshot_بھرو* l, m3type_t i_expectedType, خالی* o_where)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    اگر (l->checking) {
        o_where = scratch;
    }

_   (Get_قدر_لیبل(l, i_expectedType, &type));

    اگر (IsRefType(type)) {
        خالی* reference = تہی;
        u64   بٹ      = 0;
_       (Get_قدر_Body(l, type, &بٹ));
        reference = (خالی*)(uintptr_t)بٹ;
        حافظہ_نقل(o_where, &reference, حجم(reference));
    } ورنہ اگر (بنیاد_Type_کا(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } ورنہ {
        u64 بٹ = 0;
_       (Get_قدر_Body(l, type, &بٹ));

        اگر (Is64_ثنائی_ہندسہ_Type(type)) {
            حافظہ_نقل(o_where, &بٹ, حجم(بٹ));
        } ورنہ {
            u32 narrow = (u32)بٹ;
            حافظہ_نقل(o_where, &narrow, حجم(narrow));
        }
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ GetFunction (M3_Snapshot_بھرو* l, IM3Function* o_function)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->عدد_Functions);
    *o_function = &l->module->functions[index];

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_حافظہ_Chunks (M3_Snapshot_بھرو* l, u8* o_بائٹیں, size_t i_سائز)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    برائے (;;) {
        u8  chunkType = 0;
        u32 offset = 0, لمبائی = 0;

_       (GetU8(l, &chunkType));

        اگر (chunkType == d_m3_Chunk_انتہا) {
            توڑو;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &لمبائی));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_سائز or لمبائی > i_سائز - (size_t)offset);

        اگر (chunkType == d_m3ChunkRaw) {
            اگر (l->checking or لمبائی == 0) {
_               (Skip_بائٹیں(l, لمبائی));
            } ورنہ {
_               (Get(l, o_بائٹیں + offset, لمبائی));
            }
        } ورنہ اگر (chunkType == d_m3ChunkFillFF) {
            اگر (not l->checking and لمبائی) {
                memset(o_بائٹیں + offset, 0xFF, لمبائی);
            }
        } ورنہ {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Memories (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ  نتیجہ      = m3Err_none;
    IM3Module module      = l->module;
    u32       عدد_Memories = 0;
    l->loaded_حافظہ_بائٹیں  = 0;

_   (GetLEB_u32(l, &عدد_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, عدد_Memories == 0 or عدد_Memories != module->عدد_Memories);

    برائے (u32 m = 0; m < عدد_Memories; ++m) {
        u32       mem_اشاریہ = 0, پہلا_اشاریہ = 0, صفحہ_بٹ = 0, عدد_صفحات = 0;
        IM3_حافظہ حافظہ;

_       (GetLEB_u32(l, &mem_اشاریہ));
_       (GetLEB_u32(l, &پہلا_اشاریہ));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_اشاریہ != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 پہلا_اشاریہ != حافظہ_پہلا_اشاریہ(module, m));
        اگر (پہلا_اشاریہ != m) {
            جاری;
        }

_       (GetLEB_u32(l, &صفحہ_بٹ));
_       (GetLEB_u32(l, &عدد_صفحات));

        حافظہ = module->memories[mem_اشاریہ];
        _throwif("a shared memory cannot be restored from a snapshot", حافظہ and حافظہ->isShared);

        size_t بائٹیں = 0;
        u8*    ڈیٹا  = تہی;

        اگر (حافظہ) {
            _throwif(m3Err_wasmMalformed, صفحہ_بٹ >= 32 or (1u << صفحہ_بٹ) != حافظہ_صفحہ_سائز(حافظہ));
            _throwif(m3Err_wasmMalformed, عدد_صفحات < حافظہ->عدد_صفحات or عدد_صفحات > حافظہ->max_صفحات);
            _throwif(m3Err_wasmMalformed, عدد_صفحات > SIZE_MAX / حافظہ_صفحہ_سائز(حافظہ));
            بائٹیں = (size_t)عدد_صفحات * حافظہ_صفحہ_سائز(حافظہ);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->حافظہ_حد_بندی and بائٹیں > (size_t)l->runtime->حافظہ_حد_بندی);

            اگر (not l->checking) {
                اگر (حافظہ->عدد_صفحات < عدد_صفحات) {
_                   (Resize_حافظہ(l->runtime, حافظہ, عدد_صفحات));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         حافظہ->mallocated and حافظہ->mallocated->لمبائی < بائٹیں);
            }

            l->loaded_حافظہ_بائٹیں += بائٹیں;

            اگر (بائٹیں and حافظہ->mallocated) {
                ڈیٹا = m3_Mem_ڈیٹا(حافظہ->mallocated);
            }
        } ورنہ {
            _throwif(m3Err_wasmMalformed, عدد_صفحات != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        اگر (ڈیٹا and not l->checking) {
            memset(ڈیٹا, 0, بائٹیں);
        }

_       (بھرو_حافظہ_Chunks(l, ڈیٹا, بائٹیں));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_حافظہ_بائٹیں != l->required_حافظہ_بائٹیں);

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Globals (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ  نتیجہ     = m3Err_none;
    IM3Module module     = l->module;
    u32       عدد_Globals = 0;

_   (GetLEB_u32(l, &عدد_Globals));
    _throwif(m3Err_wasmMalformed, عدد_Globals == 0 or عدد_Globals != module->عدد_Globals);

    برائے (u32 g = 0; g < عدد_Globals; ++g) {
        u32       global_اشاریہ = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_اشاریہ));
        _throwif(m3Err_wasmMalformed, global_اشاریہ != g);

        global = &module->globals[global_اشاریہ];
        cell   = global->resolved ? global->resolved : global;

_       (Get_قدر(l, global->type, &cell->i64_قدر));
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Tables (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ  نتیجہ       = m3Err_none;
    IM3Module module       = l->module;
    u32       عدد_Tables    = 0;
    l->loaded_جدول_Elements = 0;

_   (GetLEB_u32(l, &عدد_Tables));
    _throwif(m3Err_wasmMalformed, عدد_Tables == 0 or عدد_Tables != module->عدد_Tables);

    برائے (u32 t = 0; t < عدد_Tables; ++t) {
        u32      جدول_اشاریہ = 0, پہلا_اشاریہ = 0, سائز = 0;
        u8       valtype = 0;
        IM3_جدول جدول_2;

_       (GetLEB_u32(l, &جدول_اشاریہ));
_       (GetLEB_u32(l, &پہلا_اشاریہ));

        _throwif(m3Err_wasmMalformed, جدول_اشاریہ != t);
        _throwif("the snapshot shares tables between imports differently",
                 پہلا_اشاریہ != جدول_پہلا_اشاریہ(module, t));
        اگر (پہلا_اشاریہ != t) {
            جاری;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &سائز));

        جدول_2 = module->tables[جدول_اشاریہ];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_مقصد_Val_Type(جدول_2->type));
        _throwif(m3Err_wasmMalformed, سائز > (جدول_2->hasMax ? جدول_2->max_سائز : d_m3_Max_Sane_جدول_سائز));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, سائز < جدول_2->سائز);

        l->loaded_جدول_Elements += سائز;

        اگر (not l->checking and سائز != جدول_2->سائز) {
            خالی** elements;

            _throwif(m3_Err_جدول_حد_بندی_Exceeded, l->runtime->جدول_Elements_حد_بندی and
                                                 سائز - جدول_2->سائز > l->runtime->جدول_Elements_حد_بندی - l->runtime->جدول_Elements_مستعمل);
            elements = m3_ReallocArray(خالی*, جدول_2->elements, سائز, جدول_2->سائز);
            _throwifnull(elements);

            l->runtime->جدول_Elements_مستعمل += سائز - جدول_2->سائز;
            جدول_2->elements = elements;
            جدول_2->سائز     = سائز;
        }

        برائے (u32 e = 0; e < سائز; ++e) {
            خالی* reference = تہی;
_           (Get_Ref_قدر(l, جدول_2->type, &reference));
            اگر (not l->checking) {
                جدول_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_جدول_Elements != l->required_جدول_Elements);

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Segments (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = l->module;
    u32       گنتی  = 0;

_   (GetLEB_u32(l, &گنتی));
    _throwif(m3Err_wasmMalformed, گنتی != module->عدد_ڈیٹا_Segments);
    _throwif(m3Err_wasmMalformed, module->عدد_ڈیٹا_Segments == 0 and module->عدد_Element_Segments == 0);

    برائے (u32 i = 0; i < گنتی; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        اگر (not l->checking) {
            module->ڈیٹا_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &گنتی));
    _throwif(m3Err_wasmMalformed, گنتی != module->عدد_Element_Segments);

    برائے (u32 i = 0; i < گنتی; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        اگر (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    واپس نتیجہ;
}

// A frame for the continuation being put together, outermost first
ساکن
M3_نتیجہ جوڑو_چوکھٹا (M3_Snapshot_بھرو* l, u32* io_عدد_چوکھٹے, M3_چوکھٹا_قسم i_قسم, m3stack_t i_sp, M3_چوکھٹا** o_چوکھٹا)
{
    اگر (*io_عدد_چوکھٹے >= d_m3_Continuation_Max_چوکھٹے) {
        واپس "the snapshot's frames do not fit this build's";
    }

    M3_چوکھٹا* چوکھٹا = &l->چوکھٹے[(*io_عدد_چوکھٹے)++];

    memset(چوکھٹا, 0, حجم(*چوکھٹا));
    چوکھٹا->قسم   = (u8)i_قسم;
    چوکھٹا->sp     = i_sp;
    چوکھٹا->حافظہ = Module_Memory0(l->module);

    *o_چوکھٹا = چوکھٹا;

    واپس m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ساکن
M3_نتیجہ PrepareFunction (IM3Function i_function)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    اگر (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    واپس نتیجہ;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ساکن
M3_نتیجہ جوڑو_بلاک_چوکھٹے (M3_Snapshot_بھرو* l, ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, m3stack_t i_sp,
                         u32* io_عدد_چوکھٹے)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    برائے (u32 b = 0; b < i_map->عدد_بلاکیں; ++b) {
        ثابت M3_بلاک_آغاز* بلاک = &i_map->بلاکیں[b];
        M3_چوکھٹا*            چوکھٹا;

        اگر (not Encloses_انحراف(بلاک, i_نقطہ->wasm_انحراف)) {
            جاری;
        }

        اگر (بلاک->opcode == c_waOp_loop) {
_           (جوڑو_چوکھٹا(l, io_عدد_چوکھٹے, چوکھٹا_loop, i_sp, &چوکھٹا));
            چوکھٹا->pc = بلاک->pc;
        }
#  اگر d_m3HasExceptionHandling
        ورنہ {
_           (جوڑو_چوکھٹا(l, io_عدد_چوکھٹے, چوکھٹا_try, i_sp, &چوکھٹا));
            چوکھٹا->pc                = بلاک->pc;
            چوکھٹا->try_.عدد_Clauses   = بلاک->عدد_Clauses;
            چوکھٹا->try_.handlers_زندہ = Handlers_زندہ_At(i_map, i_نقطہ, بلاک);
        }
#  endif
    }

_catch:
    واپس نتیجہ;
}

// Puts a function's values where this build keeps them at the safepoint
ساکن
M3_نتیجہ بھرو_اقدار (M3_Snapshot_بھرو* l, m3slot_t* i_بنیاد, u32 i_عدد_Slots, m3stack_t i_sp,
                     ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, ثابت M3_چوکھٹا_Registers* i_registers)
{
    M3_نتیجہ نتیجہ    = m3Err_none;
    u32      عدد_اقدار = 0;

_   (GetLEB_u32(l, &عدد_اقدار));
    _throwif("the snapshot's frame does not match what this build compiled", عدد_اقدار != i_map->عدد_Locals + i_نقطہ->عدد_اقدار);

    برائے (u32 v = 0; v < عدد_اقدار; ++v) {
        ثابت M3_خانہ_قدر* قدر     = چوکھٹا_قدر(i_map, i_نقطہ, v);
        u64                لفظ      = 0;
        خالی*              reference = تہی;
        u8*                where;

_       (Locate_قدر(i_بنیاد, i_عدد_Slots, i_sp, قدر, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        اگر (بنیاد_Type_کا(قدر->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_قدر_لیبل(l, قدر->type, &type));
            اگر (l->checking) {
_               (Skip_بائٹیں(l, 16));
            } ورنہ {
_               (Get(l, where, 16));
            }
            جاری;
        }

        l->allowUnfilledRef = i_نقطہ->قسم == safepoint_suspend and
                              v >= عدد_اقدار - i_نقطہ->عدد_Results;
_       (Get_قدر_بٹ(l, قدر->type, &لفظ));
        l->allowUnfilledRef = false;

        اگر (IsRefType(قدر->type)) {
            reference = (خالی*)(uintptr_t)لفظ;
        }

        اگر (not l->checking) {
_           (Write_قدر(where, i_registers, قدر, لفظ, reference));
        }
    }

_catch:
    واپس نتیجہ;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ساکن
M3_نتیجہ بھرو_Suspended_Body (M3_Snapshot_بھرو* l, IM3Continuation io_cont, u32 i_id)
{
    M3_نتیجہ    نتیجہ    = m3Err_none;
    u32         عدد_چوکھٹے = 0;
    M3_چوکھٹا*    call_چوکھٹا = تہی;
    IM3Function function  = تہی;
    m3slot_t*   بنیاد;
    u32         عدد_Slots;
    u32         عدد_Activations = 0;
    IM3FuncType expected       = io_cont->اندراج_Function->funcType;
    m3stack_t   sp;

_   (Continuation_ڈھیر(l->runtime, io_cont, &بنیاد, &عدد_Slots));

_   (GetLEB_u32(l, &عدد_Activations));
    _throwif(m3Err_wasmMalformed, عدد_Activations == 0 or عدد_Activations > d_m3_Continuation_Max_چوکھٹے);

    io_cont->pc                = تہی;
    io_cont->r0                = 0;
    io_cont->عدد_Suspend_Results = 0;
    io_cont->suspend_نقطہ      = safepoint_op;
#  اگر d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = بنیاد;

    برائے (u32 a = 0; a < عدد_Activations; ++a) {
        bool                 isInnermost = (a + 1 == عدد_Activations);
        ثابت M3_محفوظ_نقطہ*   نقطہ;
        M3_چوکھٹا_Registers     registers;
        u8                   قسم       = 0;
        u32                  wasm_انحراف = 0, ہدف_Loop = 0;
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
                 (u32)(sp - بنیاد) + function->max_ڈھیر_Slots > عدد_Slots);

        // the call the frame outside this one is waiting on
        اگر (call_چوکھٹا) {
            call_چوکھٹا->call.function = function;
        }

#  اگر d_m3_اندراج_Keeps_چوکھٹا
        {
            M3_چوکھٹا* اندراج_2;
_           (جوڑو_چوکھٹا(l, &عدد_چوکھٹے, چوکھٹا_اندراج, sp, &اندراج_2));
            اندراج_2->pc             = function->compiled;
            اندراج_2->اندراج_2.function = function;
        }
#  endif

_       (GetU8(l, &قسم));
_       (GetLEB_u32(l, &wasm_انحراف));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        اگر (قسم == safepoint_op) {
            ثابت M3_بلاک_آغاز* loop;

_           (GetLEB_u32(l, &ہدف_Loop));

            loop = ڈھونڈو_بلاک_At(map, c_waOp_loop, ہدف_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_انحراف(loop, wasm_انحراف));
        }

        نقطہ = ڈھونڈو_محفوظ_نقطہ_At(map, wasm_انحراف, قسم, ہدف_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not نقطہ);

_       (جوڑو_بلاک_چوکھٹے(l, map, نقطہ, sp, &عدد_چوکھٹے));

        اگر (not isInnermost) {
            _throwif(m3Err_wasmMalformed, قسم != safepoint_call);

_           (جوڑو_چوکھٹا(l, &عدد_چوکھٹے, چوکھٹا_call, sp, &call_چوکھٹا));
            call_چوکھٹا->pc = نقطہ->pc;
            registers.r0  = &call_چوکھٹا->call.r0;
#  اگر d_m3HasFloat
            registers.fp0 = &call_چوکھٹا->call.fp0;
#  endif
        } ورنہ اگر (قسم == safepoint_resume) {
            M3_چوکھٹا* چوکھٹا;
            u32      عدد_Results = نقطہ->عدد_Results;
            pc_t     resultsPC  = نقطہ->pc - 2 * عدد_Results;

_           (جوڑو_چوکھٹا(l, &عدد_چوکھٹے, چوکھٹا_resume, sp, &چوکھٹا));
            چوکھٹا->pc                 = نقطہ->pc;
            چوکھٹا->resume.resultsPC   = resultsPC;
            چوکھٹا->resume.عدد_Results  = عدد_Results;
            چوکھٹا->resume.عدد_Handlers = نقطہ->aux;
            چوکھٹا->resume.handlersPC  = resultsPC - 1 - 3 * نقطہ->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            call_چوکھٹا = چوکھٹا;
        } ورنہ {
            _throwif(m3Err_wasmMalformed, قسم != safepoint_op and قسم != safepoint_اندراج and قسم != safepoint_suspend);

            io_cont->pc           = نقطہ->pc;
            io_cont->suspend_نقطہ = قسم;
            registers.r0          = &io_cont->r0;
#  اگر d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (بھرو_اقدار(l, بنیاد, عدد_Slots, sp, map, نقطہ, &registers));

        اگر (not l->checking) {
            RestoreConstants(function, sp);
        }

        اگر (not isInnermost) {
            sp = sp + نقطہ->aux;
_           (CallSiteType(function, نقطہ, &expected));
        } ورنہ اگر (قسم == safepoint_resume) {
            خالی* cont = تہی;
_           (Get_Ref_قدر(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == تہی);

            call_چوکھٹا->resume.cont = (IM3Continuation)cont;
            اگر (not l->checking) {
                l->suspendSites[i_id].نقطہ = نقطہ;
                برائے (u32 i = 1; i < l->عدد_Continuations; ++i) {
                    اگر (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].ہدف = i;
                        l->suspendSites[i].resumed   = true;
                        توڑو;
                    }
                }
            }
        } ورنہ {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            اگر (نقطہ->flags & d_m3_محفوظ_نقطہ_لیا_گیا_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            اگر (قسم == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, نقطہ->عدد_Results > نقطہ->عدد_اقدار or
                                                نقطہ->عدد_Results > d_m3MaxContinuationPayload);

                io_cont->عدد_Suspend_Results = نقطہ->عدد_Results;
                اگر (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].نقطہ = نقطہ;
                }

                برائے (u32 i = 0; i < نقطہ->عدد_Results; ++i) {
                    ثابت M3_خانہ_قدر* قدر = &map->اقدار[نقطہ->پہلا_قدر + نقطہ->عدد_اقدار - نقطہ->عدد_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_رجسٹر_خانہ_Alias(قدر->خانہ));

                    io_cont->suspend_نتیجہ_Offsets[i] = قدر->خانہ;
                    io_cont->suspend_نتیجہ_Is64[i]    = Is64_ثنائی_ہندسہ_Type(قدر->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    اگر (l->checking) {
        واپس m3Err_none;
    }

    // held innermost first
    اگر (عدد_چوکھٹے > io_cont->چوکھٹے_Cap) {
        M3_چوکھٹا* چوکھٹے = m3_ReallocArray(M3_چوکھٹا, io_cont->چوکھٹے, عدد_چوکھٹے, io_cont->چوکھٹے_Cap);
        _throwifnull(چوکھٹے);

        io_cont->چوکھٹے    = چوکھٹے;
        io_cont->چوکھٹے_Cap = عدد_چوکھٹے;
    }

    برائے (u32 k = 0; k < عدد_چوکھٹے; ++k) {
        io_cont->چوکھٹے[عدد_چوکھٹے - 1 - k] = l->چوکھٹے[k];
    }

    io_cont->عدد_چوکھٹے = عدد_چوکھٹے;

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Continuation (M3_Snapshot_بھرو* l, u32 i_id)
{
    M3_نتیجہ        نتیجہ = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    اگر (l->checking) {
        scratch          = *cont;
        scratch.val_ڈھیر = l->check_ڈھیر;
        اگر (i_id == 0) {
            scratch.val_ڈھیر = (m3slot_t*)l->runtime->origin_ڈھیر;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_اشاریہ = 0, اندراج_اشاریہ = 0, bound_دلائل_گنتی = 0;
    u8  state = 0, is_جڑ = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_جڑ));
_   (GetLEB_u32(l, &type_اشاریہ));
_   (GetLEB_u32(l, &اندراج_اشاریہ));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_جڑ != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_جڑ and state != snapshot_contSuspended);
    اگر (not is_جڑ and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_اشاریہ != d_m3SnapshotNone and type_اشاریہ >= module->عدد_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, اندراج_اشاریہ >= module->عدد_Functions);
    _throwif(m3Err_wasmMalformed, is_جڑ ? type_اشاریہ != d_m3SnapshotNone : (type_اشاریہ == d_m3SnapshotNone or not module->funcTypes[type_اشاریہ]->isContinuation));

    اگر (not l->checking and not is_جڑ and state != snapshot_cont_ختم_شدہ) {
_       (Continuation_Acquire_ڈھیر(l->runtime, cont));
    }
    cont->type          = (type_اشاریہ != d_m3SnapshotNone) ? module->funcTypes[type_اشاریہ] : تہی;
    cont->اندراج_Function = &module->functions[اندراج_اشاریہ];
    cont->عدد_Handlers   = 0;
    cont->handlersPC    = تہی;
    cont->والد        = تہی;
    cont->عدد_چوکھٹے     = 0;

#  اگر d_m3HasExceptionHandling
    {
        خالی* ref = تہی;
_       (Get_Ref_قدر(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_جڑ or state == snapshot_cont_ختم_شدہ) and cont->resumeThrow != تہی);
#  ورنہ
    {
        خالی* ref = تہی;
_       (Get_Ref_قدر(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != تہی);
    }
#  endif

    اگر (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         عدد_Rets;

_       (GetLEB_u32(l, &bound_دلائل_گنتی));
        cont->bound_دلائل_گنتی = bound_دلائل_گنتی;
        inner                = cont->اندراج_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_دلائل_گنتی > Get_Func_Type_عدد_Params(inner));

        {
            IM3FuncType بقایا = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not بقایا or
                                            Get_Func_Type_عدد_Params(بقایا) != Get_Func_Type_عدد_Params(inner) - bound_دلائل_گنتی or
                                            Get_Func_Type_عدد_Results(بقایا) != Get_Func_Type_عدد_Results(inner));
            برائے (u16 i = 0; i < Get_Func_Type_عدد_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجہ_Type(inner, i) != Get_Func_Type_نتیجہ_Type(بقایا, i));
            }
            برائے (u16 i = 0; i < Get_Func_Type_عدد_Params(بقایا); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_دلائل_گنتی + i)) != GetFuncTypeParamType(بقایا, i));
            }
        }

        عدد_Rets = Get_Func_Type_عدد_Results(inner);

        _throwif(m3Err_wasmMalformed, (عدد_Rets + bound_دلائل_گنتی) * c_io_خانہ_گنتی > cont->عدد_ڈھیر_Slots);

        برائے (u32 i = 0; i < bound_دلائل_گنتی; ++i) {
_           (Get_قدر(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ڈھیر + (عدد_Rets + i) * c_io_خانہ_گنتی));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ڈھیر;
        cont->pc    = cont->اندراج_Function->compiled;
    } ورنہ اگر (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_دلائل_گنتی));
        _throwif(m3Err_wasmMalformed, is_جڑ and bound_دلائل_گنتی != 0);
        cont->bound_دلائل_گنتی = bound_دلائل_گنتی;

_       (بھرو_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } ورنہ اگر (state == snapshot_cont_ختم_شدہ) {
_       (GetLEB_u32(l, &bound_دلائل_گنتی));
        _throwif(m3Err_wasmMalformed, bound_دلائل_گنتی != 0);
        cont->bound_دلائل_گنتی = 0;
        cont->state          = cont_consumed;
    } ورنہ {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    اگر (نتیجہ) {
        cont->عدد_چوکھٹے = 0;
    }

    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Continuations (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      گنتی  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &گنتی));
    _throwif(m3Err_wasmMalformed, گنتی == 0 or گنتی != l->عدد_Continuations);

    برائے (u32 i = 0; i < گنتی; ++i) {
_       (بھرو_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    واپس نتیجہ;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ساکن
M3_نتیجہ CheckContinuationReferences (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    برائے (u32 id = 0; id < l->عدد_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             برگ  = id;
        u32             گہرائی = 0;
#  اگر d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_کا(Ref_Type_کا_Func_Type(cont->type, true), expectedType)));
#  endif
        اگر (l->suspendSites[id].ہدف) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_کا_Ref(l->module, l->suspendSites[id].نقطہ->resumeType);
            IM3Continuation بچہ    = l->continuations[l->suspendSites[id].ہدف];
            IM3FuncType     actual   = بچہ->اندراج_Function ? بچہ->اندراج_Function->funcType : تہی;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_عدد_Results(actual) != Get_Func_Type_عدد_Results(expected));
            برائے (u16 i = 0; i < Get_Func_Type_عدد_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجہ_Type(actual, i) != Get_Func_Type_نتیجہ_Type(expected, i));
            }
        }
        جب (l->suspendSites[برگ].ہدف) {
            برگ = l->suspendSites[برگ].ہدف;
            _throwif(m3Err_wasmMalformed, ++گہرائی >= l->عدد_Continuations or
                                            l->continuations[برگ]->state != cont_suspended);
        }
        اگر (cont->state != cont_suspended or id == 0) {
            جاری;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        اگر (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_دلائل_گنتی != 0);
            جاری;
        }

        ثابت M3SnapshotSuspendSite* site      = &l->suspendSites[برگ];
        IM3Continuation              inner     = l->continuations[برگ];
        IM3FuncType                  بقایا = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->نقطہ or not بقایا);
        _throwif("a captured continuation has the wrong binding count", cont->bound_دلائل_گنتی > inner->عدد_Suspend_Results or
                                                                          Get_Func_Type_عدد_Params(بقایا) != inner->عدد_Suspend_Results - cont->bound_دلائل_گنتی);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_عدد_Results(بقایا) != Get_Func_Type_عدد_Results(cont->اندراج_Function->funcType));
        برائے (u16 i = 0; i < Get_Func_Type_عدد_Results(بقایا); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجہ_Type(بقایا, i) !=
                                            Get_Func_Type_نتیجہ_Type(cont->اندراج_Function->funcType, i));
        }

        برائے (u32 i = 0; i < inner->عدد_Suspend_Results; ++i) {
            ثابت M3_خانہ_قدر* قدر = &site->map->اقدار[site->نقطہ->پہلا_قدر +
                                                          site->نقطہ->عدد_اقدار - site->نقطہ->عدد_Results + i];
            اگر (i < cont->bound_دلائل_گنتی) {
                اگر (IsRefType(قدر->type) and not IsNullableRef(قدر->type)) {
                    خالی* ref = تہی;
                    حافظہ_نقل(&ref, inner->sp + قدر->خانہ, حجم(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } ورنہ {
                _throwif("a captured continuation has the wrong parameter type", قدر->type !=
                                                                                   GetFuncTypeParamType(بقایا, (u16)(i - cont->bound_دلائل_گنتی)));
            }
        }
    }

_catch:
    واپس نتیجہ;
}

#  اگر d_m3HasExceptionHandling

ساکن
M3_نتیجہ بھرو_Exceptions (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ نتیجہ        = m3Err_none;
    u32      عدد_Exceptions = 0;

_   (GetLEB_u32(l, &عدد_Exceptions));
    _throwif(m3Err_wasmMalformed, عدد_Exceptions == 0 or عدد_Exceptions != l->عدد_Exceptions);

    برائے (u32 i = 0; i < عدد_Exceptions; ++i) {
        u32          exnId = 0, عدد_دلائل = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &عدد_دلائل));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->لیبل->type;

        _throwif(m3Err_wasmMalformed, عدد_دلائل != exn->عدد_دلائل);

        برائے (u32 a = 0; a < عدد_دلائل; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      بٹ    = 0;
_           (Get_قدر_بٹ(l, argType, &بٹ));
            اگر (not l->checking) {
                exn->دلائل[a] = بٹ;
            }
        }
    }

_catch:
    واپس نتیجہ;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
قسمتعریف ساخت M3BoundedReader {
    M3_Snapshot_بھرو* بھرو;
    u64             بقایا;
    M3_نتیجہ        error;
} M3BoundedReader;

ساکن
M3_نتیجہ BoundedReader_Read (خالی* o_بفر, size_t i_سائز, خالی* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    اگر (i_سائز > br->بقایا) {
        br->error = "the host read past the end of its state";
        واپس br->error;
    }

    br->بقایا -= i_سائز;

    واپس Get(br->بھرو, o_بفر, i_سائز);
}

ساکن
M3_نتیجہ بھرو_Host_State (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ               نتیجہ = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 سائز = (size_t)(l->انتہا - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->بھرو_Host_State_2);

    اگر (l->checking) {
        واپس Skip_بائٹیں(l, سائز);
    }
    reader.بھرو      = l;
    reader.بقایا = سائز;
    reader.error     = m3Err_none;

_   (hooks->بھرو_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, سائز));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.بقایا != 0);

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_سرخی (M3_Snapshot_بھرو* l)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       magic[حجم(c_snapshotMagic)];
    u32      نسخہ = 0;

_   (Get(l, magic, حجم(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, حجم(magic)) != 0);

_   (GetLE32(l, &نسخہ));
    _throwif(m3Err_wasmMalformed, نسخہ != c_snapshot_نسخہ);

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Meta (M3_Snapshot_بھرو* l, u32* o_flags)
{
    M3_نتیجہ نتیجہ      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      حافظہ_بائٹیں = 0, جدول_Elements = 0;
    u32      فعال_Stacks = 0;
    u64      current_حافظہ, currentTables;
    SnapshotResourceTotals(l->module, &current_حافظہ, &currentTables);

_   (GetLEB_u32(l, o_flags));
_   (GetLEB_u64(l, &timestamp));
_   (GetLEB_u64(l, &moduleHash));

    (خالی)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->عدد_Continuations));
_   (GetLEB_u32(l, &l->عدد_Exceptions));
_   (GetLEB_u64(l, &حافظہ_بائٹیں));
_   (GetLEB_u64(l, &جدول_Elements));
_   (GetLEB_u32(l, &فعال_Stacks));
    l->requiredStacks        = فعال_Stacks;
    l->required_حافظہ_بائٹیں   = حافظہ_بائٹیں;
    l->required_جدول_Elements = جدول_Elements;
    _throwif(m3Err_wasmMalformed, حافظہ_بائٹیں < current_حافظہ or جدول_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->عدد_Memories and حافظہ_بائٹیں != 0);
    _throwif(m3Err_wasmMalformed, not l->module->عدد_Tables and جدول_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    اگر (*o_flags & d_m3SnapshotFlagPostmortem) {
        برائے (u32 i = 0; i < l->عدد_Exceptions; ++i) {
            u32 لیبل_اشاریہ = 0;
_           (GetLEB_u32(l, &لیبل_اشاریہ));
        }
        واپس m3Err_none;
    }

    _throwif(m3_Err_حافظہ_حد_بندی_Exceeded, l->runtime->حافظہ_بائٹیں_حد_بندی and
                                          حافظہ_بائٹیں - current_حافظہ > l->runtime->حافظہ_بائٹیں_حد_بندی - l->runtime->حافظہ_بائٹیں_مستعمل);
    _throwif(m3_Err_جدول_حد_بندی_Exceeded, l->runtime->جدول_Elements_حد_بندی and
                                         جدول_Elements - currentTables > l->runtime->جدول_Elements_حد_بندی - l->runtime->جدول_Elements_مستعمل);
    _throwif(m3_Err_continuation_حد_بندی_Exceeded, l->runtime->continuations_حد_بندی and
                                                فعال_Stacks > l->runtime->continuations_حد_بندی - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->عدد_Continuations == 0 or فعال_Stacks >= l->عدد_Continuations);
#  اگر !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->عدد_Exceptions != 0);
#  endif

    اگر (l->checking) {
        برائے (u32 i = 0; i < l->عدد_Exceptions; ++i) {
            u32 لیبل_اشاریہ = 0;
_           (GetLEB_u32(l, &لیبل_اشاریہ));
#  اگر d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, لیبل_اشاریہ >= l->module->عدد_لیبل_فہرست);
#  endif
        }
        واپس m3Err_none;
    }

    l->check_ڈھیر = m3_AllocArray(m3slot_t, d_m3_Continuation_ڈھیر_Slots + 4);
    _throwifnull(l->check_ڈھیر);
    l->چوکھٹے = m3_AllocArray(M3_چوکھٹا, d_m3_Continuation_Max_چوکھٹے);
    _throwifnull(l->چوکھٹے);

    l->continuations = m3_AllocArray(IM3Continuation, l->عدد_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->عدد_Continuations);
    _throwifnull(l->suspendSites);

    اگر (not l->runtime->جڑ_Continuation) {
        IM3Continuation جڑ = Continuation_New(l->runtime, تہی, تہی);
        _throwifnull(جڑ);

        // the root borrows the runtime's stack, so it holds none of its own
        جڑ->عدد_ڈھیر_Slots          = l->runtime->عدد_ڈھیر_Slots;
        l->runtime->جڑ_Continuation = جڑ;
    }

    l->continuations[0] = l->runtime->جڑ_Continuation;

    برائے (u32 i = 1; i < l->عدد_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, تہی, تہی);
        _throwifnull(l->continuations[i]);
    }

#  اگر d_m3HasExceptionHandling
    اگر (l->عدد_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->عدد_Exceptions);
        _throwifnull(l->exceptions);
    }

    برائے (u32 i = 0; i < l->عدد_Exceptions; ++i) {
        u32    لیبل_اشاریہ = 0;
        IM3_لیبل لیبل;
        u32    عدد_دلائل;

_       (GetLEB_u32(l, &لیبل_اشاریہ));
        _throwif(m3Err_wasmMalformed, لیبل_اشاریہ >= l->module->عدد_لیبل_فہرست);

        لیبل     = لیبل_کا_اشاریہ(l->module, لیبل_اشاریہ);
        عدد_دلائل = Get_Func_Type_عدد_Params(لیبل->type);

        l->exceptions[i] = NewException(l->runtime, لیبل, عدد_دلائل);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  ورنہ
    برائے (u32 i = 0; i < l->عدد_Exceptions; ++i) {
        u32 لیبل_اشاریہ = 0;
_       (GetLEB_u32(l, &لیبل_اشاریہ));
    }
#  endif

_catch:
    واپس نتیجہ;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ساکن
M3_نتیجہ بھرو_Section (M3_Snapshot_بھرو* l, u8 i_sectionId)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      flags  = 0;
    انتخاب (i_sectionId) {
    حالت d_m3SnapshotSection_Meta:
_       (بھرو_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        توڑو;

    حالت d_m3_Snapshot_Section_حافظہ:
_       (بھرو_Memories(l));
        توڑو;

    حالت d_m3_Snapshot_Section_جدول:
_       (بھرو_Tables(l));
        توڑو;

    حالت d_m3SnapshotSection_Global:
_       (بھرو_Globals(l));
        توڑو;

    حالت d_m3SnapshotSection_Segment:
_       (بھرو_Segments(l));
        توڑو;

#  اگر d_m3HasExceptionHandling
    حالت d_m3SnapshotSection_Exception:
_       (بھرو_Exceptions(l));
        توڑو;
#  endif

    حالت d_m3SnapshotSection_Continuation:
_       (بھرو_Continuations(l));
        توڑو;

    حالت d_m3SnapshotSection_HostState:
_       (بھرو_Host_State(l));
        توڑو;

    طےشدہ:
_       (Skip_بائٹیں(l, (size_t)(l->انتہا - l->pos)));
        توڑو;
    }

_catch:
    واپس نتیجہ;
}

ساکن
M3_نتیجہ بھرو_Sections (M3_Snapshot_بھرو* l, bool i_embedded)
{
    M3_نتیجہ نتیجہ  = m3Err_none;
    u8*      payload = تہی;

    اگر (not i_embedded) {
_       (بھرو_سرخی(l));
    }

    جب (true) {
        u8  sectionId   = 0;
        u32 section_سائز = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        اگر (GetU8(l, &sectionId)) {
            توڑو;
        }

_       (GetLEB_u32(l, &section_سائز));

        _throwif("the snapshot holds a section twice", l->نظر_آیا_Sections[sectionId]);
        l->نظر_آیا_Sections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_سائز == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_سائز);
        _throwifnull(payload);
_       (Get(l, payload, section_سائز));
        l->pos       = payload;
        l->انتہا       = payload + section_سائز;
        l->اندر_Section = true;

        {
            M3_Snapshot_بھرو check = *l;
            check.checking       = true;
_           (بھرو_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.انتہا);
        }
        l->بدلا = l->بدلا or sectionId != d_m3SnapshotSection_Meta;
_       (بھرو_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->انتہا);
        l->اندر_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->نظر_آیا_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->عدد_Exceptions and not l->نظر_آیا_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->عدد_Memories and not l->نظر_آیا_Sections[d_m3_Snapshot_Section_حافظہ]);
    _throwif("the snapshot has no table section",
             l->module->عدد_Tables and not l->نظر_آیا_Sections[d_m3_Snapshot_Section_جدول]);
    _throwif("the snapshot has no global section",
             l->module->عدد_Globals and not l->نظر_آیا_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->عدد_ڈیٹا_Segments or l->module->عدد_Element_Segments) and
               not l->نظر_آیا_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    واپس نتیجہ;
}

#اگر_ختم // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#اگر d_m3HasSnapshots

ساکن
M3_نتیجہ SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, خالی* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_نتیجہ       نتیجہ = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, حجم(save));

    IM3Continuation جڑ = io_runtime->جڑ_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not جڑ or جڑ->state != cont_suspended;

    IM3Function اندراج_2 = save.postmortem ? io_runtime->entered : جڑ->اندراج_Function;

    اگر (o_module) {
        *o_module = تہی;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not اندراج_2);

    save.module = اندراج_2->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    اگر (o_module) {
        *o_module = save.module;
    }

    برائے (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->اگلا) {
        u32 id;
_       (اشارہ_گر_Ids_جوڑو(&save.زندہ_Continuations, cont, &id));
    }

#  اگر d_m3HasExceptionHandling
    برائے (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->اگلا) {
        u32 id;
_       (اشارہ_گر_Ids_جوڑو(&save.زندہ_Exceptions, exception, &id));
    }
#  endif

    اگر (not save.postmortem) {
        u32 id;
_       (اشارہ_گر_Ids_جوڑو(&save.continuations, جڑ, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    اشارہ_گر_Ids_Free(&save.زندہ_Continuations);
    اشارہ_گر_Ids_Free(&save.زندہ_Exceptions);
    اشارہ_گر_Ids_Free(&save.continuations);
    اشارہ_گر_Ids_Free(&save.exceptions);

    واپس نتیجہ;
}

#اگر_ختم

M3_نتیجہ m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, خالی* i_userdata)
{
    اگر (not io_runtime or not i_writer) {
        واپس m3Err_mallocFailed;
    }

#اگر d_m3HasSnapshots
    واپس SaveSnapshot(io_runtime, i_writer, i_userdata, تہی, false);
#ورنہ
    (خالی)i_userdata;
    واپس "snapshots are not available in this build of Wasm3";
#اگر_ختم
}


ساکن
M3_نتیجہ بھرو_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, خالی* i_userdata, bool i_embedded)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#اگر d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_تسلسل);

    {
        M3_Snapshot_بھرو بھرو;
        memset(&بھرو, 0, حجم(بھرو));

        بھرو.runtime  = io_runtime;
        بھرو.module   = i_module;
        بھرو.reader   = i_reader;
        بھرو.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        نتیجہ = بھرو_Sections(&بھرو, i_embedded);

        IM3Continuation جڑ = io_runtime->جڑ_Continuation;

        اگر (نتیجہ) {
            اگر (بھرو.بدلا) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                اگر (جڑ) {
                    جڑ->state     = cont_returned;
                    جڑ->عدد_چوکھٹے = 0;
                }
            }
        } ورنہ {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->آغاز_Function = -1;
            i_module->has_تسلسل        = true;
        }

        Runtime_رکھو_Call_ڈھیر(io_runtime);

        m3_Free(بھرو.check_ڈھیر);
        m3_Free(بھرو.چوکھٹے);
        m3_Free(بھرو.suspendSites);
        m3_Free(بھرو.continuations);
#  اگر d_m3HasExceptionHandling
        m3_Free(بھرو.exceptions);
#  endif
    }
#ورنہ
    (خالی)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#اگر_ختم

_catch:
    واپس نتیجہ;
}


M3_نتیجہ m3_بھرو_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, خالی* i_userdata)
{
    واپس بھرو_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


خالی m3_سیٹ_Snapshot_Hooks (IM3Runtime io_runtime, ثابت M3SnapshotHooks* i_hooks)
{
#اگر d_m3HasSnapshots
    اگر (io_runtime) {
        اگر (i_hooks) {
            io_runtime->snapshotHooks = *i_hooks;
        } ورنہ {
            memset(&io_runtime->snapshotHooks, 0, حجم(io_runtime->snapshotHooks));
        }
    }
#ورنہ
    (خالی)io_runtime;
    (خالی)i_hooks;
#اگر_ختم
}


M3_نتیجہ m3_Save_Snapshot_مقصد_بفر (IM3Runtime io_runtime, خالی** o_بائٹیں, size_t* o_سائز)
{
    اگر (!io_runtime || !o_بائٹیں || !o_سائز) {
        واپس m3Err_mallocFailed;
    }
#اگر d_m3HasSnapshots
    M3_بفر_Writer bw     = { تہی, 0, 0 };
    M3_نتیجہ       نتیجہ = m3_SaveSnapshot(io_runtime, بفر_Writer_Write, &bw);
    اگر (نتیجہ) {
        m3_Free(bw.بفر);
        *o_بائٹیں = تہی;
        *o_سائز  = 0;
        واپس نتیجہ;
    }
    *o_بائٹیں = bw.بفر;
    *o_سائز  = bw.سائز;
    واپس m3Err_none;
#ورنہ
    *o_بائٹیں = تہی;
    *o_سائز  = 0;
    واپس "snapshots are not available in this build of Wasm3";
#اگر_ختم
}

#اگر d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ساکن
u8* EmitLEB_u32 (u8* o_where, u32 i_قدر)
{
    کرو {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        اگر (i_قدر != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } جب (i_قدر != 0);

    واپس o_where;
}

ساکن
u32 سائز_کا_LEB_u32 (u32 i_قدر)
{
    u32 سائز = 0;

    کرو {
        i_قدر >>= 7;
        ++سائز;
    } جب (i_قدر != 0);

    واپس سائز;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ساکن
M3_نتیجہ ڈھونڈو_Embedded_Snapshot (ثابت u8* i_آغاز, ثابت u8* i_انتہا, cstr_t i_secName,
                               ثابت u8** o_sec_آغاز, ثابت u8** o_sec_انتہا,
                               ثابت u8** o_ڈیٹا, u32* o_سائز)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    بائٹیں_t  pos    = i_آغاز;
    bool     ملا  = false;
    cstr_t   name   = تہی;

    اگر (o_sec_آغاز) {
        *o_sec_آغاز = تہی;
    }
    اگر (o_sec_انتہا) {
        *o_sec_انتہا = تہی;
    }
    اگر (o_ڈیٹا) {
        *o_ڈیٹا = تہی;
    }
    اگر (o_سائز) {
        *o_سائز = 0;
    }

    _throwif(m3Err_wasmMalformed, i_انتہا - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    جب (pos < i_انتہا) {
        بائٹیں_t sec_آغاز  = pos;
        u8      sectionId = *pos++;
        u32     سائز      = 0;
_       (ReadLEB_u32(&سائز, &pos, i_انتہا));
        _throwif(m3Err_wasmUnderrun, سائز > (size_t)(i_انتہا - pos));
        {
            بائٹیں_t payload = pos;
            pos += سائز;
            اگر (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                اگر (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", ملا);
                    ملا = true;
                    اگر (o_sec_آغاز) {
                        *o_sec_آغاز = sec_آغاز;
                    }
                    اگر (o_sec_انتہا) {
                        *o_sec_انتہا = pos;
                    }
                    اگر (o_ڈیٹا) {
                        *o_ڈیٹا = payload;
                    }
                    اگر (o_سائز) {
                        *o_سائز = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    واپس نتیجہ;
}

// Builds the custom section name a snapshot of i_name lives under
ساکن
M3_نتیجہ SnapshotSectionName (cstr_t i_name, حرف* o_secName, size_t i_سائز)
{
    اگر (i_name and *i_name) {
        اگر (لڑی_لمبائی(i_name) > d_m3_Max_Snapshot_Name_لمبائی) {
            واپس "the snapshot name is too long";
        }
        snprintf(o_secName, i_سائز, "snapshot.%s", i_name);
    } ورنہ {
        snprintf(o_secName, i_سائز, "snapshot");
    }

    واپس m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ساکن
M3_نتیجہ ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = تہی;

    برائے (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->اگلا) {
        اگر (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    واپس نتیجہ;
}
#اگر_ختم

M3_نتیجہ m3_بھرو_Snapshot_From_بفر (IM3Runtime io_runtime, IM3Module i_module, ثابت خالی* i_بائٹیں, size_t i_سائز)
{
    اگر (!io_runtime || !i_module || !i_بائٹیں) {
        واپس m3Err_mallocFailed;
    }
#اگر d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    اگر (i_سائز >= 8 && memcmp(i_بائٹیں, "\0asm", 4) == 0) {
        ثابت u8* ڈیٹا   = تہی;
        u32       سائز   = 0;
        M3_نتیجہ  نتیجہ = ڈھونڈو_Embedded_Snapshot((ثابت u8*)i_بائٹیں, (ثابت u8*)i_بائٹیں + i_سائز,
                                                "snapshot", تہی, تہی, &ڈیٹا, &سائز);
        اگر (نتیجہ) {
            واپس نتیجہ;
        }
        اگر (not ڈیٹا) {
            واپس "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_بفر_Reader br = { ڈیٹا, سائز, 0 };
            واپس بھرو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
        }
    }
    {
        M3_بفر_Reader br = { (ثابت u8*)i_بائٹیں, i_سائز, 0 };
        واپس بھرو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br,
                            i_سائز < 4 or memcmp(i_بائٹیں, c_snapshotMagic, 4) != 0);
    }
#ورنہ
    (خالی)i_سائز;
    واپس "snapshots are not available in this build of Wasm3";
#اگر_ختم
}

bool m3_HasSnapshot (IM3Module i_module, ثابت حرف* i_name)
{
    اگر (not i_module) {
        واپس false;
    }
#اگر d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = تہی;
    (خالی)ModuleSnapshot(i_module, i_name, &snap);

    واپس snap != تہی;
#ورنہ
    (خالی)i_name;
    واپس false;
#اگر_ختم
}

M3_نتیجہ m3_GetEmbeddedSnapshot (IM3Module i_module, ثابت حرف* i_name,
                                 ثابت خالی** o_بائٹیں, size_t* o_سائز)
{
    اگر (not i_module or not o_بائٹیں or not o_سائز) {
        واپس m3Err_mallocFailed;
    }

    *o_بائٹیں = تہی;
    *o_سائز  = 0;

#اگر d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = تہی;
        M3_نتیجہ            نتیجہ = ModuleSnapshot(i_module, i_name, &snap);

        اگر (نتیجہ) {
            واپس نتیجہ;
        }
        اگر (not snap) {
            واپس "the module holds no embedded snapshot of that name";
        }

        *o_بائٹیں = snap->ڈیٹا;
        *o_سائز  = snap->سائز;

        واپس m3Err_none;
    }
#ورنہ
    (خالی)i_name;
    واپس "snapshots are not available in this build of Wasm3";
#اگر_ختم
}

M3_نتیجہ m3_بھرو_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name)
{
    اگر (not io_runtime or not i_module) {
        واپس m3Err_mallocFailed;
    }
#اگر d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = تہی;
        M3_نتیجہ            نتیجہ = ModuleSnapshot(i_module, i_name, &snap);
        M3_بفر_Reader      br;

        اگر (نتیجہ) {
            واپس نتیجہ;
        }
        اگر (not snap) {
            واپس "the module holds no embedded snapshot of that name";
        }

        br.بفر = snap->ڈیٹا;
        br.سائز   = snap->سائز;
        br.cursor = 0;

        واپس بھرو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
    }
#ورنہ
    (خالی)i_name;
    واپس "snapshots are not available in this build of Wasm3";
#اگر_ختم
}


M3_نتیجہ m3_Save_Snapshot_مقصد_Module (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name,
                                  خالی** o_بائٹیں, size_t* o_سائز)
{
#اگر d_m3HasSnapshots
    M3_نتیجہ       نتیجہ = m3Err_none;
    M3_بفر_Writer bw     = { تہی, 0, 0 };
    u8*            برآمد    = تہی;

    حرف      secName[d_m3_Max_Snapshot_Name_لمبائی + حجم("snapshot.")];
    u32       nameLen, name_Len_Leb_سائز, payload_سائز, payload_سائز_Leb_سائز;
    size_t    orig_سائز, کل_سائز;
    ثابت u8* drop_آغاز = تہی;
    ثابت u8* drop_انتہا   = تہی;
    IM3Module saved     = تہی;
    u8*       p;

    اگر (not io_runtime or not i_module or not o_بائٹیں or not o_سائز) {
        واپس m3Err_mallocFailed;
    }

    *o_بائٹیں = تہی;
    *o_سائز  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_آغاز or i_module->wasm_انتہا <= i_module->wasm_آغاز);

_   (SnapshotSectionName(i_name, secName, حجم(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->جڑ_Continuation or io_runtime->جڑ_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, بفر_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.سائز);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.سائز > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (ڈھونڈو_Embedded_Snapshot(i_module->wasm_آغاز, i_module->wasm_انتہا, secName, &drop_آغاز, &drop_انتہا, تہی, تہی));

    orig_سائز           = (size_t)(i_module->wasm_انتہا - i_module->wasm_آغاز) - (drop_آغاز ? (size_t)(drop_انتہا - drop_آغاز) : 0);
    nameLen            = (u32)لڑی_لمبائی(secName);
    name_Len_Leb_سائز     = سائز_کا_LEB_u32(nameLen);
    payload_سائز        = name_Len_Leb_سائز + nameLen + (u32)bw.سائز;
    payload_سائز_Leb_سائز = سائز_کا_LEB_u32(payload_سائز);
    کل_سائز          = orig_سائز + 1 + payload_سائز_Leb_سائز + payload_سائز;

    برآمد = (u8*)m3_Malloc("wasm_snapshot_module", کل_سائز);
    _throwifnull(برآمد);

    p = برآمد;

    اگر (drop_آغاز) {
        size_t before = (size_t)(drop_آغاز - i_module->wasm_آغاز);
        size_t after  = (size_t)(i_module->wasm_انتہا - drop_انتہا);

        حافظہ_نقل(p, i_module->wasm_آغاز, before);
        p += before;
        حافظہ_نقل(p, drop_انتہا, after);
        p += after;
    } ورنہ {
        حافظہ_نقل(p, i_module->wasm_آغاز, orig_سائز);
        p += orig_سائز;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_سائز);
    p    = EmitLEB_u32(p, nameLen);
    حافظہ_نقل(p, secName, nameLen);
    p += nameLen;
    حافظہ_نقل(p, bw.بفر, bw.سائز);

    *o_بائٹیں = برآمد;
    *o_سائز  = کل_سائز;
    برآمد      = تہی;

_catch:
    m3_Free(bw.بفر);
    m3_Free(برآمد);

    واپس نتیجہ;
#ورنہ
    (خالی)io_runtime;
    (خالی)i_module;
    (خالی)i_name;
    (خالی)o_بائٹیں;
    (خالی)o_سائز;
    واپس "snapshots are not available in this build of Wasm3";
#اگر_ختم
}
