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
ساكن ثابت u32 c_snapshot_نسخہ  = 1;

تعداد {
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
#  define d_m3_Max_Snapshot_Name_لمائی   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
تعداد {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_مکمل  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_سرا                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//پچھان_تت
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ساكن
M3_نتیجہ ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_شروع or i_module->wasm_سرا - i_module->wasm_شروع < 8);

    {
        بائٹاں_t  pos = i_module->wasm_شروع + 8;
        cbytes_t سرا = i_module->wasm_سرا;

        Xxh64_Init(&hash, 0);

        طالما (pos < سرا) {
            بائٹاں_t شروع     = pos;
            u8      sectionId = *pos++;
            u32     ناپ      = 0;

_           (ReadLEB_u32(&ناپ, &pos, سرا));
            _throwif(m3Err_wasmUnderrun, ناپ > (size_t)(سرا - pos));

            pos += ناپ;
            إذا (sectionId != 0) {
                Xxh64_Update(&hash, شروع, (size_t)(pos - شروع));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    أرجع نتیجہ;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ساكن
ثابت M3_محفوظ_نقطہ* لبھو_محفوظ_نقطہ (IM3Function i_function, pc_t i_pc, u8 i_قسم)
{
    ثابت M3SnapshotMap* map = i_function->snapshotMap;

    إذا (map) {
        لكل (u32 i = 0; i < map->عدد_محفوظ_Points; ++i) {
            إذا (map->محفوظ_Points[i].pc == i_pc and map->محفوظ_Points[i].قسم == i_قسم) {
                أرجع &map->محفوظ_Points[i];
            }
        }
    }

    أرجع عدم;
}

// The safepoint of i_kind at the instruction at i_wasmOffset, and for a back
// edge, the one that goes to the loop at i_targetLoop. Two clauses of one
// instruction can go to the same loop; they leave the same frame, so either of
// their safepoints does.
ساكن
ثابت M3_محفوظ_نقطہ* لبھو_محفوظ_نقطہ_At (ثابت M3SnapshotMap* i_map, u32 i_wasm_وتھ, u8 i_قسم, u32 i_نشانہ_Loop)
{
    u32 کم  = 0;
    u32 بلند = i_map->عدد_محفوظ_Points;

    طالما (کم < بلند) {
        u32 درمیان = کم + (بلند - کم) / 2;

        إذا (i_map->محفوظ_Points[درمیان].wasm_وتھ < i_wasm_وتھ) {
            کم = درمیان + 1;
        } وإلا {
            بلند = درمیان;
        }
    }

    لكل (u32 i = کم; i < i_map->عدد_محفوظ_Points and i_map->محفوظ_Points[i].wasm_وتھ == i_wasm_وتھ; ++i) {
        ثابت M3_محفوظ_نقطہ* نقطہ = &i_map->محفوظ_Points[i];

        إذا (نقطہ->قسم != i_قسم or (نقطہ->flags & d_m3_محفوظ_نقطہ_پوچھ_Call)) {
            استمر;
        }
        إذا (i_قسم != safepoint_op or i_map->بلاکاں[نقطہ->aux].wasm_وتھ == i_نشانہ_Loop) {
            أرجع نقطہ;
        }
    }

    أرجع عدم;
}

ساكن
ثابت M3_بلاک_شروع* لبھو_بلاک_At (ثابت M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_وتھ)
{
    لكل (u32 i = 0; i < i_map->عدد_بلاکاں; ++i) {
        إذا (i_map->بلاکاں[i].opcode == i_opcode and i_map->بلاکاں[i].wasm_وتھ == i_wasm_وتھ) {
            أرجع &i_map->بلاکاں[i];
        }
    }

    أرجع عدم;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ساكن مضمن
bool Encloses_وتھ (ثابت M3_بلاک_شروع* i_بلاک, u32 i_wasm_وتھ)
{
    أرجع i_بلاک->wasm_وتھ < i_wasm_وتھ and i_wasm_وتھ < i_بلاک->wasm_سرا;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ساكن مضمن
bool Handlers_جیوندا_At (ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, ثابت M3_بلاک_شروع* i_try)
{
    أرجع not (i_نقطہ->قسم == safepoint_op and i_try->wasm_وتھ > i_map->بلاکاں[i_نقطہ->aux].wasm_وتھ);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ساكن
M3_نتیجہ CallSiteType (IM3Function i_caller, ثابت M3_محفوظ_نقطہ* i_نقطہ, IM3FuncType* o_type)
{
    M3_نتیجہ   نتیجہ = m3Err_none;
    IM3Module  module = i_caller->module;
    بائٹاں_t    pos    = i_caller->wasm;
    cbytes_t   سرا    = i_caller->wasm_سرا;
    u32        ناپ   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = عدم;

_   (ReadLEB_u32(&ناپ, &pos, سرا));
    _throwif(m3Err_wasmUnderrun, i_نقطہ->wasm_وتھ >= (size_t)(سرا - pos));
    pos += i_نقطہ->wasm_وتھ;

_   (Read_opcode(&opcode, &pos, سرا));
_   (ReadLEB_u32(&index, &pos, سرا));

    إذا (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->عدد_Functions);
        *o_type = module->functions[index].funcType;
    } وإلا {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_اسدھا and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->عدد_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    أرجع نتیجہ;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ساكن
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 عدد_Results = Get_Func_Type_عدد_Results(i_expected);

    إذا (not i_type or Get_Func_Type_عدد_Results(i_type) != عدد_Results) {
        أرجع false;
    }

    لكل (u16 i = 0; i < عدد_Results; ++i) {
        إذا (not Is_Sub_Type_دا(Get_Func_Type_نتیجہ_Type(i_type, i), Get_Func_Type_نتیجہ_Type(i_expected, i))) {
            أرجع false;
        }
    }

    أرجع true;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  values in a frame
//---------------------------------------------------------------------------------------------------------------------------------

// The registers a frame goes on with. A frame that goes on with clear ones - a
// resume - has none, and a value the map puts in one cannot be there.
تعريفنوع بنية M3_چوکھٹا_Registers {
    m3reg_t* r0;
#  إذا d_m3HasFloat
    f64* fp0;
#  endif
} M3_چوکھٹا_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
ساكن
M3_نتیجہ Continuation_ڈھیر (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_ادھار, u32* o_عدد_Slots)
{
    إذا (i_cont == i_runtime->جڑ_Continuation) {
        *o_ادھار     = (m3slot_t*)i_runtime->origin_ڈھیر;
        *o_عدد_Slots = i_runtime->عدد_ڈھیر_Slots;
    } وإلا {
        *o_ادھار     = i_cont->val_ڈھیر;
        *o_عدد_Slots = i_cont->عدد_ڈھیر_Slots;
    }

    أرجع (*o_ادھار) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ساكن
M3_نتیجہ Locate_قدر (m3slot_t* i_ادھار, u32 i_عدد_Slots, m3stack_t i_sp, ثابت M3_تھاں_قدر* i_قدر, u8** o_where)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       type   = ادھار_Type_دا(i_قدر->type);

    *o_where = عدم;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_رجسٹر_تھاں_Alias(i_قدر->تھاں));

    إذا (not Is_رجسٹر_تھاں_Alias(i_قدر->تھاں)) {
        size_t ناپ   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? حجم(فراغ*)
                                                                   : (Is64_ثنائی_ہندسہ_Type(type) ? حجم(u64) : حجم(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_ادھار + i_قدر->تھاں) * حجم(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + ناپ > (size_t)i_عدد_Slots * حجم(m3slot_t));

        *o_where = (u8*)i_ادھار + offset;
    }

_catch:
    أرجع نتیجہ;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ساكن
M3_نتیجہ Read_قدر (u8* i_where, ثابت M3_چوکھٹا_Registers* i_registers, ثابت M3_تھاں_قدر* i_قدر, u64* o_بٹ,
                    فراغ** o_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       type   = ادھار_Type_دا(i_قدر->type);

    *o_بٹ      = 0;
    *o_reference = عدم;

    إذا (i_where) {
        إذا (IsRefType(type)) {
            حافظہ_نقل(o_reference, i_where, حجم(فراغ*));
        } وإلا إذا (Is64_ثنائی_ہندسہ_Type(type)) {
            حافظہ_نقل(o_بٹ, i_where, حجم(u64));
        } وإلا {
            u32 narrow;
            حافظہ_نقل(&narrow, i_where, حجم(narrow));
            *o_بٹ = narrow;
        }
    } وإلا إذا (Is_Fp_رجسٹر_تھاں_Alias(i_قدر->تھاں)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 بٹ;
            حافظہ_نقل(&بٹ, &narrow, حجم(بٹ));
            *o_بٹ = بٹ;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            حافظہ_نقل(o_بٹ, i_registers->fp0, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        إذا (IsRefType(type)) {
            *o_reference = (فراغ*)(uintptr_t)r0;
        } وإلا إذا (Is64_ثنائی_ہندسہ_Type(type)) {
            *o_بٹ = (u64)r0;
        } وإلا {
            *o_بٹ = (u32)r0;
        }
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Write_قدر (u8* o_where, ثابت M3_چوکھٹا_Registers* i_registers, ثابت M3_تھاں_قدر* i_قدر, u64 i_بٹ,
                     فراغ* i_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       type   = ادھار_Type_دا(i_قدر->type);

    إذا (o_where) {
        إذا (IsRefType(type)) {
            حافظہ_نقل(o_where, &i_reference, حجم(فراغ*));
        } وإلا إذا (Is64_ثنائی_ہندسہ_Type(type)) {
            حافظہ_نقل(o_where, &i_بٹ, حجم(u64));
        } وإلا {
            u32 narrow = (u32)i_بٹ;
            حافظہ_نقل(o_where, &narrow, حجم(narrow));
        }
    } وإلا إذا (Is_Fp_رجسٹر_تھاں_Alias(i_قدر->تھاں)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            u32 بٹ = (u32)i_بٹ;
            f32 narrow;
            حافظہ_نقل(&narrow, &بٹ, حجم(narrow));
            *i_registers->fp0 = narrow;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            حافظہ_نقل(i_registers->fp0, &i_بٹ, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        إذا (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } وإلا {
            *i_registers->r0 = (m3reg_t)i_بٹ;
        }
    }

_catch:
    أرجع نتیجہ;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ساكن
ثابت M3_تھاں_قدر* چوکھٹا_قدر (ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, u32 i_اشاریہ)
{
    إذا (i_اشاریہ < i_map->عدد_Locals) {
        أرجع i_نقطہ->پہلا_Local_قدر == UINT32_MAX ? &i_map->locals[i_اشاریہ]
                                                      : &i_map->اقدار[i_نقطہ->پہلا_Local_قدر + i_اشاریہ];
    }
    أرجع &i_map->اقدار[i_نقطہ->پہلا_قدر + i_اشاریہ - i_map->عدد_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ساكن
فراغ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    إذا (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->عدد_Ret_And_Arg_Slots) + i_function->عدد_Local_بائٹاں;

        حافظہ_نقل(where, i_function->constants, i_function->عدد_Constant_بائٹاں);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
تعريفنوع بنية M3_اشارہ_گر_Ids {
    فراغ** اشیاء;
    u32    گنتی;
    u32    سمرتھا;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  اشاریہ_سمرتھا;       // a power of two, kept at least twice count
} M3_اشارہ_گر_Ids;

ساكن
u32 اشارہ_گر_بالٹی (فراغ* i_اشارہ_گر, u32 i_سمرتھا)
{
    u64 بٹ = (u64)(uintptr_t)i_اشارہ_گر;

    أرجع (u32)((بٹ * 0x9E3779B97F4A7C15ULL) >> 32) & (i_سمرتھا - 1);
}

ساكن
u32 اشارہ_گر_Ids_لبھو (ثابت M3_اشارہ_گر_Ids* i_ids, فراغ* i_اشارہ_گر)
{
    إذا (i_ids->اشاریہ_سمرتھا) {
        u32 بالٹی = اشارہ_گر_بالٹی(i_اشارہ_گر, i_ids->اشاریہ_سمرتھا);

        طالما (i_ids->index[بالٹی]) {
            u32 id = i_ids->index[بالٹی] - 1;

            إذا (i_ids->اشیاء[id] == i_اشارہ_گر) {
                أرجع id;
            }

            بالٹی = (بالٹی + 1) & (i_ids->اشاریہ_سمرتھا - 1);
        }
    }

    أرجع d_m3SnapshotNone;
}

ساكن
M3_نتیجہ اشارہ_گر_Ids_جوڑو (M3_اشارہ_گر_Ids* io_ids, فراغ* i_اشارہ_گر, u32* o_id)
{
    u32 id = اشارہ_گر_Ids_لبھو(io_ids, i_اشارہ_گر);

    إذا (id != d_m3SnapshotNone) {
        *o_id = id;
        أرجع m3Err_none;
    }

    إذا (io_ids->گنتی == io_ids->سمرتھا) {
        u32    سمرتھا = io_ids->سمرتھا ? io_ids->سمرتھا * 2 : 16;
        فراغ** اشیاء    = m3_ReallocArray(فراغ*, io_ids->اشیاء, سمرتھا, io_ids->سمرتھا);

        إذا (not اشیاء) {
            أرجع m3Err_mallocFailed;
        }

        io_ids->اشیاء    = اشیاء;
        io_ids->سمرتھا = سمرتھا;
    }

    إذا ((io_ids->گنتی + 1) * 2 > io_ids->اشاریہ_سمرتھا) {
        u32  سمرتھا = io_ids->اشاریہ_سمرتھا ? io_ids->اشاریہ_سمرتھا * 2 : 32;
        u32* index    = m3_AllocArray(u32, سمرتھا);

        إذا (not index) {
            أرجع m3Err_mallocFailed;
        }

        لكل (u32 i = 0; i < io_ids->گنتی; ++i) {
            u32 بالٹی = اشارہ_گر_بالٹی(io_ids->اشیاء[i], سمرتھا);

            طالما (index[بالٹی]) {
                بالٹی = (بالٹی + 1) & (سمرتھا - 1);
            }

            index[بالٹی] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->اشاریہ_سمرتھا = سمرتھا;
    }

    id                = io_ids->گنتی++;
    io_ids->اشیاء[id] = i_اشارہ_گر;

    u32 بالٹی = اشارہ_گر_بالٹی(i_اشارہ_گر, io_ids->اشاریہ_سمرتھا);

    طالما (io_ids->index[بالٹی]) {
        بالٹی = (بالٹی + 1) & (io_ids->اشاریہ_سمرتھا - 1);
    }

    io_ids->index[بالٹی] = id + 1;

    *o_id = id;

    أرجع m3Err_none;
}

ساكن
فراغ اشارہ_گر_Ids_Free (M3_اشارہ_گر_Ids* io_ids)
{
    m3_Free(io_ids->اشیاء);
    m3_Free(io_ids->index);
    memset(io_ids, 0, حجم(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3_بفر_Writer {
    u8*    بفر;
    size_t ناپ;
    size_t سمرتھا;
} M3_بفر_Writer;

ساكن
M3_نتیجہ بفر_Writer_Write (ثابت فراغ* i_انکڑے, size_t i_ناپ, فراغ* i_userdata)
{
    M3_بفر_Writer* bw = (M3_بفر_Writer*)i_userdata;

    إذا (bw->ناپ + i_ناپ > bw->سمرتھا) {
        size_t newCap = bw->سمرتھا ? bw->سمرتھا * 2 : 4096;
        طالما (bw->ناپ + i_ناپ > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->بفر, newCap, bw->سمرتھا);
        إذا (!newBuf) {
            أرجع m3Err_mallocFailed;
        }
        bw->بفر   = newBuf;
        bw->سمرتھا = newCap;
    }

    إذا (i_ناپ) {
        حافظہ_نقل(bw->بفر + bw->ناپ, i_انکڑے, i_ناپ);
        bw->ناپ += i_ناپ;
    }

    أرجع m3Err_none;
}

تعريفنوع بنية M3_بفر_Reader {
    ثابت u8* بفر;
    size_t    ناپ;
    size_t    cursor;
} M3_بفر_Reader;

ساكن
M3_نتیجہ بفر_Reader_Read (فراغ* o_بفر, size_t i_ناپ, فراغ* i_userdata)
{
    M3_بفر_Reader* br = (M3_بفر_Reader*)i_userdata;

    إذا (i_ناپ > br->ناپ - br->cursor) {
        أرجع m3Err_wasmMalformed;
    }

    إذا (i_ناپ) {
        حافظہ_نقل(o_بفر, br->بفر + br->cursor, i_ناپ);
        br->cursor += i_ناپ;
    }

    أرجع m3Err_none;
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

    M3_اشارہ_گر_Ids جیوندا_Continuations;     // everything the runtime still holds, to check
    M3_اشارہ_گر_Ids جیوندا_Exceptions;        //   a reference against before following it
    M3_اشارہ_گر_Ids continuations;         // what the snapshot carries, by id
    M3_اشارہ_گر_Ids exceptions;
} M3SnapshotSave;

ساكن مضمن
u8 M3_Type_منزل_Val_Type (m3type_t i_type)
{
    بدل (ادھار_Type_دا(i_type)) {
    حالة c_m3Type_i32: أرجع 0x7F;
    حالة c_m3Type_i64: أرجع 0x7E;
    حالة c_m3Type_f32: أرجع 0x7D;
    حالة c_m3Type_f64: أرجع 0x7C;
    حالة c_m3Type_v128: أرجع 0x7B;
    حالة c_m3Type_funcref: أرجع 0x70;
    حالة c_m3Type_externref: أرجع 0x6F;
    حالة c_m3Type_exnref: أرجع 0x69;
    حالة c_m3Type_contref: أرجع 0x68;
    افتراضي: أرجع 0x00;
    }
}

ساكن مضمن
m3type_t Val_Type_منزل_M3_Type (u8 i_valtype)
{
    بدل (i_valtype) {
    حالة 0x7F: أرجع c_m3Type_i32;
    حالة 0x7E: أرجع c_m3Type_i64;
    حالة 0x7D: أرجع c_m3Type_f32;
    حالة 0x7C: أرجع c_m3Type_f64;
    حالة 0x7B: أرجع c_m3Type_v128;
    حالة 0x70: أرجع c_m3Type_funcref;
    حالة 0x6F: أرجع c_m3Type_externref;
    حالة 0x69: أرجع c_m3Type_exnref;
    حالة 0x68: أرجع c_m3Type_contref;
    افتراضي: أرجع c_m3Type_none;
    }
}

ساكن
M3_نتیجہ Put (M3SnapshotSave* s, ثابت فراغ* i_انکڑے, size_t i_ناپ)
{
    أرجع s->writing ? s->writer(i_انکڑے, i_ناپ, s->userdata) : m3Err_none;
}

ساكن
M3_نتیجہ PutU8 (M3SnapshotSave* s, u8 i_قدر)
{
    أرجع Put(s, &i_قدر, حجم(i_قدر));
}


ساكن
M3_نتیجہ PutLEB_u32 (M3SnapshotSave* s, u32 i_قدر)
{
    u8  بائٹاں[5];
    u32 len = 0;

    افعل {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        إذا (i_قدر != 0) {
            byte |= 0x80;
        }
        بائٹاں[len++] = byte;
    } طالما (i_قدر != 0);

    أرجع Put(s, بائٹاں, len);
}

ساكن
M3_نتیجہ PutLEB_u64 (M3SnapshotSave* s, u64 i_قدر)
{
    u8  بائٹاں[10];
    u32 len = 0;

    افعل {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        إذا (i_قدر != 0) {
            byte |= 0x80;
        }
        بائٹاں[len++] = byte;
    } طالما (i_قدر != 0);

    أرجع Put(s, بائٹاں, len);
}

ساكن
M3_نتیجہ PutLEB_i64 (M3SnapshotSave* s, i64 i_قدر)
{
    u8   بائٹاں[10];
    u32  len  = 0;
    bool ہور = true;

    طالما (ہور) {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        إذا ((i_قدر == 0 && !(byte & 0x40)) || (i_قدر == -1 && (byte & 0x40))) {
            ہور = false;
        } وإلا {
            byte |= 0x80;
        }
        بائٹاں[len++] = byte;
    }

    أرجع Put(s, بائٹاں, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
تعريفنوع M3_نتیجہ (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ساكن
M3_نتیجہ SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_نتیجہ         نتیجہ   = m3Err_none;
    M3_بفر_Writer   bw       = { عدم, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    فراغ*            userdata = s->userdata;

    s->writer   = بفر_Writer_Write;
    s->userdata = &bw;

    نتیجہ = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    إذا (نتیجہ) {
        اذهب _catch;
    }

    // a section with nothing in it is left out
    إذا (bw.ناپ) {
        _throwif("snapshot section is too large", bw.ناپ > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.ناپ));
_       (Put(s, bw.بفر, bw.ناپ));
    }

_catch:
    m3_Free(bw.بفر);

    أرجع نتیجہ;
}

ساكن
u32 Function_اشاریہ (IM3Module i_module, IM3Function i_function)
{
    إذا (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->عدد_Functions) {
        أرجع (u32)(i_function - i_module->functions);
    }

    أرجع d_m3SnapshotNone;
}

ساكن
u32 Func_Type_اشاریہ (IM3Module i_module, IM3FuncType i_type)
{
    لكل (u32 i = 0; i < i_module->عدد_Func_Types; ++i) {
        إذا (i_module->funcTypes[i] == i_type) {
            أرجع i;
        }
    }

    أرجع d_m3SnapshotNone;
}

#  إذا d_m3HasExceptionHandling

// Code refers to a tag as whatever it resolved to, so that is what an
// exception holds
ساكن
IM3_ٹیگ ٹیگ_دا_اشاریہ (IM3Module i_module, u32 i_اشاریہ)
{
    IM3_ٹیگ ٹیگ = &i_module->ٹیگ_سوچی[i_اشاریہ];

    أرجع ٹیگ->resolved ? ٹیگ->resolved : ٹیگ;
}

ساكن
u32 ٹیگ_اشاریہ_2 (IM3Module i_module, IM3_ٹیگ i_ٹیگ)
{
    لكل (u32 i = 0; i < i_module->عدد_ٹیگ_سوچی; ++i) {
        إذا (ٹیگ_دا_اشاریہ(i_module, i) == i_ٹیگ) {
            أرجع i;
        }
    }

    أرجع d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ساكن
M3_نتیجہ NameReference (M3SnapshotSave* s, u8 i_type, فراغ* i_reference, u64* o_لفظ)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_لفظ = d_m3SnapshotNullRef;

    إذا (not i_reference) {
        أرجع m3Err_none;
    }

    إذا (i_type == c_m3Type_funcref) {
        id = Function_اشاریہ(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } وإلا إذا (i_type == c_m3Type_externref) {
        ثابت M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_لفظ = name;
        اذهب _catch;
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 اشارہ_گر_Ids_لبھو(&s->جیوندا_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 گنتی = s->exceptions.گنتی;
_       (اشارہ_گر_Ids_جوڑو(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.گنتی != گنتی);
    }
#  endif
    وإلا إذا (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 اشارہ_گر_Ids_لبھو(&s->جیوندا_Continuations, i_reference) == d_m3SnapshotNone);

        u32 گنتی = s->continuations.گنتی;
_       (اشارہ_گر_Ids_جوڑو(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.گنتی != گنتی);
    } وإلا {
        _throw("a reference of an unknown type");
    }

    *o_لفظ = id;

_catch:
    أرجع نتیجہ;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ساكن
M3_نتیجہ Put_Ref_قدر (M3SnapshotSave* s, u8 i_type, فراغ* i_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u64      لفظ   = 0;

_   (NameReference(s, i_type, i_reference, &لفظ));

    إذا (not s->writing) {
        أرجع m3Err_none;
    }

    إذا (لفظ == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } وإلا {
        u8 قسم = 0;
        إذا (i_type == c_m3Type_funcref) {
            قسم = 0x01;
        } وإلا إذا (i_type == c_m3Type_externref) {
            قسم = 0x02;
        }
#  إذا d_m3HasExceptionHandling
        وإلا إذا (i_type == c_m3Type_exnref) {
            قسم = 0x03;
        }
#  endif
        وإلا إذا (i_type == c_m3Type_contref) {
            قسم = 0x04;
        }

_       (PutU8(s, قسم));
_       (PutLEB_u64(s, لفظ));
    }

_catch:
    أرجع نتیجہ;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ساكن
M3_نتیجہ PutLE32 (M3SnapshotSave* s, u32 i_قدر)
{
    u8 بائٹاں[4];

    لكل (u32 i = 0; i < حجم(بائٹاں); ++i) {
        بائٹاں[i] = (u8)(i_قدر >> (8 * i));
    }

    أرجع Put(s, بائٹاں, حجم(بائٹاں));
}

ساكن
M3_نتیجہ PutLE64 (M3SnapshotSave* s, u64 i_قدر)
{
    u8 بائٹاں[8];

    لكل (u32 i = 0; i < حجم(بائٹاں); ++i) {
        بائٹاں[i] = (u8)(i_قدر >> (8 * i));
    }

    أرجع Put(s, بائٹاں, حجم(بائٹاں));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ساكن
M3_نتیجہ Put_قدر_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_بٹ)
{
    إذا (IsRefType(i_type)) {
        أرجع Put_Ref_قدر(s, ادھار_Type_دا(i_type), (فراغ*)(uintptr_t)i_بٹ);
    }

    بدل (ادھار_Type_دا(i_type)) {
    حالة c_m3Type_i32: أرجع PutLEB_i64(s, (i32)(u32)i_بٹ);
    حالة c_m3Type_i64: أرجع PutLEB_i64(s, (i64)i_بٹ);
    حالة c_m3Type_f32: أرجع PutLE32(s, (u32)i_بٹ);
    حالة c_m3Type_f64: أرجع PutLE64(s, i_بٹ);
    افتراضي: أرجع "a snapshot cannot hold a value of this type as bits";
    }
}

ساكن
M3_نتیجہ Put_قدر_ٹیگ (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_منزل_Val_Type(i_type);

    إذا (not valtype) {
        أرجع "a snapshot has no tag for a value of this type";
    }

    أرجع PutU8(s, valtype);
}

ساكن
M3_نتیجہ Put_قدر_بٹ (M3SnapshotSave* s, m3type_t i_type, u64 i_بٹ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Put_قدر_ٹیگ(s, i_type));
_   (Put_قدر_Body(s, i_type, i_بٹ));

_catch:
    أرجع نتیجہ;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ساكن
M3_نتیجہ Put_قدر (M3SnapshotSave* s, m3type_t i_type, ثابت فراغ* i_where)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Put_قدر_ٹیگ(s, i_type));

    إذا (IsRefType(i_type)) {
        فراغ* reference;
        حافظہ_نقل(&reference, i_where, حجم(reference));
_       (Put_قدر_Body(s, i_type, (u64)(uintptr_t)reference));
    } وإلا إذا (ادھار_Type_دا(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 بائٹاں[16];
        حافظہ_نقل(بائٹاں, i_where, حجم(بائٹاں));
_       (Put(s, بائٹاں, حجم(بائٹاں)));
    } وإلا إذا (Is64_ثنائی_ہندسہ_Type(i_type)) {
        u64 بٹ;
        حافظہ_نقل(&بٹ, i_where, حجم(بٹ));
_       (Put_قدر_Body(s, i_type, بٹ));
    } وإلا {
        u32 narrow;
        حافظہ_نقل(&narrow, i_where, حجم(narrow));
_       (Put_قدر_Body(s, i_type, narrow));
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Put_حافظہ_Chunks (M3SnapshotSave* s, ثابت u8* i_بائٹاں, size_t i_ناپ)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    size_t   cursor = 0;

    طالما (cursor < i_ناپ) {
        u8     byte = i_بائٹاں[cursor];
        size_t لگاتار  = 1;

        طالما (cursor + لگاتار < i_ناپ and i_بائٹاں[cursor + لگاتار] == byte) {
            لگاتار++;
        }

        إذا ((byte == 0x00 or byte == 0xFF) and لگاتار >= d_m3_Snapshot_لگاتار_حد) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            إذا (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)لگاتار));
            }

            cursor += لگاتار;
            استمر;
        }

        size_t raw_شروع = cursor;

        طالما (cursor < i_ناپ) {
            u8     b      = i_بائٹاں[cursor];
            size_t لمائی = 1;

            طالما (cursor + لمائی < i_ناپ and i_بائٹاں[cursor + لمائی] == b) {
                لمائی++;
            }

            إذا ((b == 0x00 or b == 0xFF) and لمائی >= d_m3_Snapshot_لگاتار_حد) {
                اكسر;
            }

            cursor += لمائی;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_شروع));
_       (PutLEB_u32(s, (u32)(cursor - raw_شروع)));
_       (Put(s, i_بائٹاں + raw_شروع, cursor - raw_شروع));
    }

_   (PutU8(s, d_m3_Chunk_سرا));

_catch:
    أرجع نتیجہ;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ساكن
u32 حافظہ_پہلا_اشاریہ (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->memories[j] != module->memories[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
u32 جدول_پہلا_اشاریہ (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->tables[j] != module->tables[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
M3_نتیجہ SaveMemories (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Memories));

    لكل (u32 m = 0; m < module->عدد_Memories; ++m) {
        IM3_حافظہ حافظہ   = module->memories[m];
        u32       پہلا    = حافظہ_پہلا_اشاریہ(module, m);
        bool      has_انکڑے  = حافظہ and حافظہ->mallocated;
        u32       ورقہ_ناپ = حافظہ ? حافظہ_ورقہ_ناپ(حافظہ) : d_m3_Default_Mem_ورقہ_ناپ;
        u64       بائٹاں    = حافظہ ? حافظہ->عدد_ورقے * (u64)ورقہ_ناپ : 0;
        u32       ورقہ_بٹ = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, پہلا));

        إذا (پہلا != m) {
            استمر;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", حافظہ and حافظہ->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", بائٹاں > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 حافظہ and حافظہ->mallocated and حافظہ->mallocated->لمائی < بائٹاں);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        طالما ((1u << ورقہ_بٹ) < ورقہ_ناپ) {
            ++ورقہ_بٹ;
        }

_       (PutLEB_u32(s, ورقہ_بٹ));
_       (PutLEB_u32(s, حافظہ ? (u32)حافظہ->عدد_ورقے : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        إذا (not s->writing) {
            استمر;
        }

        إذا (has_انکڑے) {
_           (Put_حافظہ_Chunks(s, m3_Mem_انکڑے(حافظہ->mallocated), (size_t)بائٹاں));
        } وإلا {
_           (PutU8(s, d_m3_Chunk_سرا));
        }
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ SaveGlobals (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Globals));

    لكل (u32 g = 0; g < module->عدد_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_قدر(s, global->type, &cell->i64_قدر));
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ SaveTables (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Tables));

    لكل (u32 t = 0; t < module->عدد_Tables; ++t) {
        IM3_جدول جدول_2 = module->tables[t];
        u32      پہلا = جدول_پہلا_اشاریہ(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, پہلا));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        إذا (پہلا != t) {
            استمر;
        }

_       (PutU8(s, M3_Type_منزل_Val_Type(جدول_2->type)));
_       (PutLEB_u32(s, جدول_2->ناپ));

        لكل (u32 e = 0; e < جدول_2->ناپ; ++e) {
_           (Put_Ref_قدر(s, ادھار_Type_دا(جدول_2->type), جدول_2->elements[e]));
        }
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ SaveSegments (M3SnapshotSave* s)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_انکڑے_Segments));

    لكل (u32 i = 0; i < module->عدد_انکڑے_Segments; ++i) {
_       (PutU8(s, module->انکڑے_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->عدد_Element_Segments));

    لكل (u32 i = 0; i < module->عدد_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    أرجع نتیجہ;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_نتیجہ SaveExceptions (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.گنتی));

    لكل (u32 i = 0; i < s->exceptions.گنتی; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.اشیاء[i];
        IM3FuncType  type = exn->ٹیگ->type;

        _throwif(m3Err_wasmMalformed, exn->عدد_دلیلاں != Get_Func_Type_عدد_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->عدد_دلیلاں));

        لكل (u32 a = 0; a < exn->عدد_دلیلاں; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_قدر_بٹ(s, argType, exn->دلیلاں[a]));
        }
    }

_catch:
    أرجع نتیجہ;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
تعريفنوع بنية M3Activation {
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
ساكن
ثابت M3_محفوظ_نقطہ* لبھو_چوکھٹا_محفوظ_نقطہ (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_قسم)
{
    ثابت M3_محفوظ_نقطہ* نقطہ = *io_function ? لبھو_محفوظ_نقطہ(*io_function, i_pc, i_قسم) : عدم;

    لكل (u32 i = 0; not نقطہ and i < i_module->عدد_Functions; ++i) {
        نقطہ = لبھو_محفوظ_نقطہ(&i_module->functions[i], i_pc, i_قسم);
        إذا (نقطہ) {
            *io_function = &i_module->functions[i];
        }
    }

    أرجع نقطہ;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ساكن
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

    لكل (; i >= 0; --i) {
        ثابت M3_چوکھٹا* چوکھٹا = &i_cont->چوکھٹے[i];

        _throwif("a suspended frame runs against another module's memory", چوکھٹا->حافظہ != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", چوکھٹا->sp != i_sp);

        إذا (چوکھٹا->قسم == چوکھٹا_call) {
            o_act->pc           = چوکھٹا->pc;
            o_act->قسم         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&چوکھٹا->call.r0;
#  إذا d_m3HasFloat
            o_act->registers.fp0 = (f64*)&چوکھٹا->call.fp0;
#  endif
            اكسر;
        } وإلا إذا (چوکھٹا->قسم == چوکھٹا_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = چوکھٹا->pc;
            o_act->قسم   = safepoint_resume;
            o_act->resume = چوکھٹا;
            اكسر;
        }
    }

    إذا (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->قسم         = i_cont->suspend_نقطہ;
        o_act->registers.r0 = &i_cont->r0;
#  إذا d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->آخری_چوکھٹا = i;
    o_act->نقطہ     = لبھو_چوکھٹا_محفوظ_نقطہ(i_module, &o_act->function, o_act->pc, o_act->قسم);

    _throwif("a suspended frame is not at a safepoint", not o_act->نقطہ);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  إذا d_m3_اندراج_Keeps_چوکھٹا
    لكل (i32 k = i_پہلا; k > o_act->آخری_چوکھٹا; --k) {
        ثابت M3_چوکھٹا* چوکھٹا = &i_cont->چوکھٹے[k];
        _throwif("a suspended frame is not the function it is entered with",
                 چوکھٹا->قسم == چوکھٹا_اندراج and چوکھٹا->اندراج.function != o_act->function);
    }
#  endif

_catch:
    أرجع نتیجہ;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ساكن
M3_نتیجہ SaveActivation (M3SnapshotSave* s, m3slot_t* i_ادھار, u32 i_عدد_Slots, ثابت M3Activation* i_act)
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
_   (PutLEB_u32(s, نقطہ->wasm_وتھ));

    إذا (نقطہ->قسم == safepoint_op) {
_       (PutLEB_u32(s, map->بلاکاں[نقطہ->aux].wasm_وتھ));
    }

    عدد_اقدار = map->عدد_Locals + نقطہ->عدد_اقدار;

_   (PutLEB_u32(s, عدد_اقدار));

    لكل (u32 v = 0; v < عدد_اقدار; ++v) {
        ثابت M3_تھاں_قدر* قدر = چوکھٹا_قدر(map, نقطہ, v);
        u8*                where;
        u64                لفظ;
        فراغ*              reference;

_       (Locate_قدر(i_ادھار, i_عدد_Slots, i_act->sp, قدر, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (ادھار_Type_دا(قدر->type) == c_m3Type_v128) {
_           (Put_قدر_ٹیگ(s, قدر->type));
_           (Put(s, where, 16));
            استمر;
        }

_       (Read_قدر(where, &i_act->registers, قدر, &لفظ, &reference));

_       (Put_قدر_بٹ(s, قدر->type, IsRefType(قدر->type) ? (u64)(uintptr_t)reference : لفظ));
    }

    إذا (نقطہ->قسم == safepoint_resume) {
_       (Put_Ref_قدر(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    أرجع نتیجہ;
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
M3_نتیجہ SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_نتیجہ      نتیجہ   = m3Err_none;
    M3Activation* acts     = عدم;
    u32           عدد_Acts  = 0;
    IM3Function   function = i_cont->اندراج_Function;
    i32           پہلا    = (i32)i_cont->عدد_چوکھٹے - 1;
    m3slot_t*     ادھار;
    u32           عدد_Slots;
    m3stack_t     sp;

_   (Continuation_ڈھیر(s->runtime, i_cont, &ادھار, &عدد_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->عدد_چوکھٹے + 1);
    _throwifnull(acts);

    sp = ادھار;

    طالما (true) {
        M3Activation* act = &acts[عدد_Acts];

_       (اگلا_Activation(s->module, i_cont, function, sp, پہلا, act));

        إذا (act->قسم != safepoint_call) {
            عدد_Acts++;
            اكسر;
        }

        _throwif("a suspended continuation stops at a call", act->آخری_چوکھٹا < 0);

        إذا (not (act->نقطہ->flags & d_m3_محفوظ_نقطہ_پوچھ_Call)) {
            عدد_Acts++;
        }

        function = i_cont->چوکھٹے[act->آخری_چوکھٹا].call.function;
        sp       = act->sp + act->نقطہ->aux;
        پہلا    = act->آخری_چوکھٹا - 1;
    }

_   (PutLEB_u32(s, عدد_Acts));

    لكل (u32 a = 0; a < عدد_Acts; ++a) {
_       (SaveActivation(s, ادھار, عدد_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_نتیجہ  نتیجہ    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_جڑ    = (i_cont == s->runtime->جڑ_Continuation);
    u8        state     = snapshot_cont_مکمل;
    u32       type_اشاریہ = is_جڑ ? d_m3SnapshotNone : Func_Type_اشاریہ(module, i_cont->type);

    إذا (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } وإلا إذا (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } وإلا إذا (i_cont->state == cont_running) {
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
#  إذا d_m3HasExceptionHandling
_   (Put_Ref_قدر(s, c_m3Type_exnref, i_cont->resumeThrow));
#  وإلا
_   (Put_Ref_قدر(s, c_m3Type_exnref, عدم));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    إذا (s->postmortem) {
        اذهب _catch;
    }

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->اندراج_Function ? i_cont->اندراج_Function->funcType : عدم;
        u16         عدد_Rets = inner ? Get_Func_Type_عدد_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_دلیلاں_گنتی > Get_Func_Type_عدد_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_دلیلاں_گنتی));

        لكل (u32 i = 0; i < i_cont->bound_دلیلاں_گنتی; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_قدر(s, type, i_cont->val_ڈھیر + (عدد_Rets + i) * c_io_تھاں_گنتی));
        }
    } وإلا إذا (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_دلیلاں_گنتی));
_       (SaveSuspendedBody(s, i_cont));
    } وإلا {
_       (PutLEB_u32(s, 0));
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ SaveContinuations (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (PutLEB_u32(s, s->continuations.گنتی));

    لكل (u32 i = 0; i < s->continuations.گنتی; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.اشیاء[i]));
    }

_catch:
    أرجع نتیجہ;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ساكن
M3_نتیجہ SaveHostState (M3SnapshotSave* s)
{
    M3_نتیجہ               نتیجہ = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_بفر_Writer         bw     = { عدم, 0, 0 };

    إذا (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, بفر_Writer_Write, &bw));
    }

    إذا (bw.ناپ > 0) {
_       (Put(s, bw.بفر, bw.ناپ));
    }

_catch:
    m3_Free(bw.بفر);

    أرجع نتیجہ;
}

ساكن
فراغ SnapshotResourceTotals (IM3Module module, u64* حافظہ_بائٹاں, u64* جدول_Elements)
{
    *حافظہ_بائٹاں = *جدول_Elements = 0;
    لكل (u32 i = 0; i < module->عدد_Memories; ++i) {
        IM3_حافظہ حافظہ = module->memories[i];
        إذا (حافظہ_پہلا_اشاریہ(module, i) == i and حافظہ) {
            *حافظہ_بائٹاں += حافظہ->عدد_ورقے * (u64)حافظہ_ورقہ_ناپ(حافظہ);
        }
    }
    لكل (u32 i = 0; i < module->عدد_Tables; ++i) {
        IM3_جدول جدول_2 = module->tables[i];
        إذا (جدول_پہلا_اشاریہ(module, i) == i and جدول_2) {
            *جدول_Elements += جدول_2->ناپ;
        }
    }
}

ساكن
M3_نتیجہ SaveMeta (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u64      hash   = 0;
    u64      حافظہ_بائٹاں, جدول_Elements;
    u32      سرگرم_Stacks = 0;
    لكل (u32 i = 0; i < s->continuations.گنتی; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.اشیاء[i];
        إذا (cont != s->runtime->جڑ_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            سرگرم_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &حافظہ_بائٹاں, &جدول_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_ویلا_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.گنتی));
_   (PutLEB_u32(s, s->exceptions.گنتی));
_   (PutLEB_u64(s, حافظہ_بائٹاں));
_   (PutLEB_u64(s, جدول_Elements));
_   (PutLEB_u32(s, سرگرم_Stacks));

#  إذا d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    لكل (u32 i = 0; i < s->exceptions.گنتی; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.اشیاء[i];
        u32          ٹیگ_اشاریہ  = ٹیگ_اشاریہ_2(s->module, exception->ٹیگ);

        _throwif("an exception was thrown with another module's tag", ٹیگ_اشاریہ == d_m3SnapshotNone);

_       (PutLEB_u32(s, ٹیگ_اشاریہ));
    }
#  endif

_catch:
    أرجع نتیجہ;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ساكن
M3_نتیجہ DiscoverReferences (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ            = m3Err_none;
    u32      پورا_Continuations = 0;
    u32      پورا_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    طالما (پورا_Continuations < s->continuations.گنتی or پورا_Exceptions < s->exceptions.گنتی) {
#  إذا d_m3HasExceptionHandling
        طالما (پورا_Exceptions < s->exceptions.گنتی) {
            M3Exception* exception = (M3Exception*)s->exceptions.اشیاء[پورا_Exceptions++];
            IM3FuncType  type      = exception->ٹیگ->type;

            _throwif(m3Err_wasmMalformed, exception->عدد_دلیلاں != Get_Func_Type_عدد_Params(type));

            لكل (u32 a = 0; a < exception->عدد_دلیلاں; ++a) {
_               (Put_قدر_بٹ(s, GetFuncTypeParamType(type, (u16)a), exception->دلیلاں[a]));
            }
        }
#  endif
        طالما (پورا_Continuations < s->continuations.گنتی and پورا_Exceptions == s->exceptions.گنتی) {
            u32 id = پورا_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.اشیاء[id]));
        }
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ SaveSections (M3SnapshotSave* s)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (not s->writing) {
        أرجع DiscoverReferences(s);
    }

    إذا (not s->embedded) {
_       (Put(s, c_snapshotMagic, حجم(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_نسخہ));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    إذا (s->module->عدد_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_حافظہ, SaveMemories));
    }
    إذا (s->module->عدد_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_جدول, SaveTables));
    }
    إذا (s->module->عدد_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    إذا (s->module->عدد_انکڑے_Segments or s->module->عدد_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  إذا d_m3HasExceptionHandling
    إذا (s->exceptions.گنتی) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    إذا (s->continuations.گنتی) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    أرجع نتیجہ;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3SnapshotSuspendSite {
    ثابت M3SnapshotMap* map;
    ثابت M3_محفوظ_نقطہ*   نقطہ;
    u32                  نشانہ;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

تعريفنوع بنية M3_Snapshot_لدو {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    فراغ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ڈھیر;
    bool      بدلیا;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    عدد_Continuations;
    u64                    required_حافظہ_بائٹاں;
    u64                    required_جدول_Elements;
    u64                    loaded_حافظہ_بائٹاں;
    u64                    loaded_جدول_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  إذا d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 عدد_Exceptions;

    M3_چوکھٹا* چوکھٹے;           // a suspended continuation's frames as they are put together, outermost first

    بائٹاں_t pos;               // bounded section payload, read using the Wasm parser's helpers
    بائٹاں_t سرا;
    bool    وچ_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    تکیا_Sections[256];
} M3_Snapshot_لدو;

ساكن
M3_نتیجہ Get (M3_Snapshot_لدو* l, فراغ* o_انکڑے, size_t i_ناپ)
{
    إذا (not l->وچ_Section) {
        أرجع l->reader(o_انکڑے, i_ناپ, l->userdata);
    }
    إذا (i_ناپ > (size_t)(l->سرا - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    إذا (i_ناپ) {
        حافظہ_نقل(o_انکڑے, l->pos, i_ناپ);
        l->pos += i_ناپ;
    }
    أرجع m3Err_none;
}

ساكن
M3_نتیجہ GetU8 (M3_Snapshot_لدو* l, u8* o_قدر)
{
    أرجع Get(l, o_قدر, حجم(*o_قدر));
}

ساكن
M3_نتیجہ GetBool (M3_Snapshot_لدو* l, u8* o_قدر)
{
    M3_نتیجہ نتیجہ = GetU8(l, o_قدر);
    أرجع نتیجہ ? نتیجہ : (*o_قدر <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ساكن
M3_نتیجہ GetLEB_u32 (M3_Snapshot_لدو* l, u32* o_قدر)
{
    إذا (l->وچ_Section) {
        أرجع ReadLEB_u32(o_قدر, &l->pos, l->سرا);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    لكل (u32 i = 0; i < حجم(encoded); ++i) {
        M3_نتیجہ نتیجہ = GetU8(l, &encoded[i]);
        إذا (نتیجہ) {
            أرجع نتیجہ;
        }
        إذا (not (encoded[i] & 0x80)) {
            بائٹاں_t pos = encoded;
            أرجع ReadLEB_u32(o_قدر, &pos, encoded + i + 1);
        }
    }
    أرجع m3_Err_leb_حد_توں_ودھ;
}

ساكن
M3_نتیجہ GetLEB_u64 (M3_Snapshot_لدو* l, u64* o_قدر)
{
    أرجع ReadLebUnsigned(o_قدر, 64, &l->pos, l->سرا);
}

ساكن
M3_نتیجہ GetLEB_i64 (M3_Snapshot_لدو* l, i64* o_قدر)
{
    أرجع ReadLEB_i64(o_قدر, &l->pos, l->سرا);
}

ساكن
M3_نتیجہ Skip_بائٹاں (M3_Snapshot_لدو* l, size_t i_ناپ)
{
    إذا (i_ناپ > (size_t)(l->سرا - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    l->pos += i_ناپ;
    أرجع m3Err_none;
}


ساكن
M3_نتیجہ Get_Ref_قدر (M3_Snapshot_لدو* l, m3type_t i_type, فراغ** o_reference)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       قسم   = 0;
    u64      id     = 0;

    *o_reference = عدم;

_   (GetU8(l, &قسم));
    إذا (قسم == 0x00) {
        أرجع (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    إذا (قسم == 0x01) {
        _throwif(m3Err_wasmMalformed, ادھار_Type_دا(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->عدد_Functions);
#  إذا d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_دا(Ref_Type_دا_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } وإلا إذا (قسم == 0x02) {
        _throwif(m3Err_wasmMalformed, ادھار_Type_دا(i_type) != c_m3Type_externref);
        ثابت M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        إذا (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (قسم == 0x03) {
        _throwif(m3Err_wasmMalformed, ادھار_Type_دا(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->عدد_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    وإلا إذا (قسم == 0x04) {
        _throwif(m3Err_wasmMalformed, ادھار_Type_دا(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->عدد_Continuations);
        *o_reference = l->continuations[id];
#  إذا d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        إذا (not l->checking and ڈھیر_Type_دا(i_type) != d_m3_Type_ڈھیر_Abstract) {
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
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ GetLE32 (M3_Snapshot_لدو* l, u32* o_قدر)
{
    u8       بائٹاں[4];
    M3_نتیجہ نتیجہ = Get(l, بائٹاں, حجم(بائٹاں));

    *o_قدر = 0;

    لكل (u32 i = 0; i < حجم(بائٹاں); ++i) {
        *o_قدر |= (u32)بائٹاں[i] << (8 * i);
    }

    أرجع نتیجہ;
}

ساكن
M3_نتیجہ GetLE64 (M3_Snapshot_لدو* l, u64* o_قدر)
{
    u8       بائٹاں[8];
    M3_نتیجہ نتیجہ = Get(l, بائٹاں, حجم(بائٹاں));

    *o_قدر = 0;

    لكل (u32 i = 0; i < حجم(بائٹاں); ++i) {
        *o_قدر |= (u64)بائٹاں[i] << (8 * i);
    }

    أرجع نتیجہ;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ساكن
M3_نتیجہ Get_قدر_ٹیگ (M3_Snapshot_لدو* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_نتیجہ نتیجہ  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_منزل_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and ادھار_Type_دا(*o_type) != ادھار_Type_دا(i_expectedType));
    إذا (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    أرجع نتیجہ;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ساكن
M3_نتیجہ Get_قدر_Body (M3_Snapshot_لدو* l, m3type_t i_type, u64* o_بٹ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    *o_بٹ = 0;

    إذا (IsRefType(i_type)) {
        فراغ* reference = عدم;
_       (Get_Ref_قدر(l, i_type, &reference));
        *o_بٹ = (u64)(uintptr_t)reference;
    } وإلا إذا (ادھار_Type_دا(i_type) == c_m3Type_i32) {
        i32 قدر = 0;
_       (ReadLEB_i32(&قدر, &l->pos, l->سرا));
        *o_بٹ = (u32)قدر;
    } وإلا إذا (ادھار_Type_دا(i_type) == c_m3Type_i64) {
        i64 قدر = 0;
_       (GetLEB_i64(l, &قدر));
        *o_بٹ = (u64)قدر;
    } وإلا إذا (ادھار_Type_دا(i_type) == c_m3Type_f32) {
        u32 بٹ = 0;
_       (GetLE32(l, &بٹ));
        *o_بٹ = بٹ;
    } وإلا إذا (ادھار_Type_دا(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_بٹ));
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Get_قدر_بٹ (M3_Snapshot_لدو* l, m3type_t i_expectedType, u64* o_بٹ)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    m3type_t type;

_   (Get_قدر_ٹیگ(l, i_expectedType, &type));
_   (Get_قدر_Body(l, type, o_بٹ));

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ Get_قدر (M3_Snapshot_لدو* l, m3type_t i_expectedType, فراغ* o_where)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    إذا (l->checking) {
        o_where = scratch;
    }

_   (Get_قدر_ٹیگ(l, i_expectedType, &type));

    إذا (IsRefType(type)) {
        فراغ* reference = عدم;
        u64   بٹ      = 0;
_       (Get_قدر_Body(l, type, &بٹ));
        reference = (فراغ*)(uintptr_t)بٹ;
        حافظہ_نقل(o_where, &reference, حجم(reference));
    } وإلا إذا (ادھار_Type_دا(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } وإلا {
        u64 بٹ = 0;
_       (Get_قدر_Body(l, type, &بٹ));

        إذا (Is64_ثنائی_ہندسہ_Type(type)) {
            حافظہ_نقل(o_where, &بٹ, حجم(بٹ));
        } وإلا {
            u32 narrow = (u32)بٹ;
            حافظہ_نقل(o_where, &narrow, حجم(narrow));
        }
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ GetFunction (M3_Snapshot_لدو* l, IM3Function* o_function)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->عدد_Functions);
    *o_function = &l->module->functions[index];

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_حافظہ_Chunks (M3_Snapshot_لدو* l, u8* o_بائٹاں, size_t i_ناپ)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    لكل (;;) {
        u8  chunkType = 0;
        u32 offset = 0, لمائی = 0;

_       (GetU8(l, &chunkType));

        إذا (chunkType == d_m3_Chunk_سرا) {
            اكسر;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &لمائی));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_ناپ or لمائی > i_ناپ - (size_t)offset);

        إذا (chunkType == d_m3ChunkRaw) {
            إذا (l->checking or لمائی == 0) {
_               (Skip_بائٹاں(l, لمائی));
            } وإلا {
_               (Get(l, o_بائٹاں + offset, لمائی));
            }
        } وإلا إذا (chunkType == d_m3ChunkFillFF) {
            إذا (not l->checking and لمائی) {
                memset(o_بائٹاں + offset, 0xFF, لمائی);
            }
        } وإلا {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Memories (M3_Snapshot_لدو* l)
{
    M3_نتیجہ  نتیجہ      = m3Err_none;
    IM3Module module      = l->module;
    u32       عدد_Memories = 0;
    l->loaded_حافظہ_بائٹاں  = 0;

_   (GetLEB_u32(l, &عدد_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, عدد_Memories == 0 or عدد_Memories != module->عدد_Memories);

    لكل (u32 m = 0; m < عدد_Memories; ++m) {
        u32       mem_اشاریہ = 0, پہلا_اشاریہ = 0, ورقہ_بٹ = 0, عدد_ورقے = 0;
        IM3_حافظہ حافظہ;

_       (GetLEB_u32(l, &mem_اشاریہ));
_       (GetLEB_u32(l, &پہلا_اشاریہ));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_اشاریہ != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 پہلا_اشاریہ != حافظہ_پہلا_اشاریہ(module, m));
        إذا (پہلا_اشاریہ != m) {
            استمر;
        }

_       (GetLEB_u32(l, &ورقہ_بٹ));
_       (GetLEB_u32(l, &عدد_ورقے));

        حافظہ = module->memories[mem_اشاریہ];
        _throwif("a shared memory cannot be restored from a snapshot", حافظہ and حافظہ->isShared);

        size_t بائٹاں = 0;
        u8*    انکڑے  = عدم;

        إذا (حافظہ) {
            _throwif(m3Err_wasmMalformed, ورقہ_بٹ >= 32 or (1u << ورقہ_بٹ) != حافظہ_ورقہ_ناپ(حافظہ));
            _throwif(m3Err_wasmMalformed, عدد_ورقے < حافظہ->عدد_ورقے or عدد_ورقے > حافظہ->max_ورقے);
            _throwif(m3Err_wasmMalformed, عدد_ورقے > SIZE_MAX / حافظہ_ورقہ_ناپ(حافظہ));
            بائٹاں = (size_t)عدد_ورقے * حافظہ_ورقہ_ناپ(حافظہ);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->حافظہ_حد_بندی and بائٹاں > (size_t)l->runtime->حافظہ_حد_بندی);

            إذا (not l->checking) {
                إذا (حافظہ->عدد_ورقے < عدد_ورقے) {
_                   (Resize_حافظہ(l->runtime, حافظہ, عدد_ورقے));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         حافظہ->mallocated and حافظہ->mallocated->لمائی < بائٹاں);
            }

            l->loaded_حافظہ_بائٹاں += بائٹاں;

            إذا (بائٹاں and حافظہ->mallocated) {
                انکڑے = m3_Mem_انکڑے(حافظہ->mallocated);
            }
        } وإلا {
            _throwif(m3Err_wasmMalformed, عدد_ورقے != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        إذا (انکڑے and not l->checking) {
            memset(انکڑے, 0, بائٹاں);
        }

_       (لدو_حافظہ_Chunks(l, انکڑے, بائٹاں));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_حافظہ_بائٹاں != l->required_حافظہ_بائٹاں);

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Globals (M3_Snapshot_لدو* l)
{
    M3_نتیجہ  نتیجہ     = m3Err_none;
    IM3Module module     = l->module;
    u32       عدد_Globals = 0;

_   (GetLEB_u32(l, &عدد_Globals));
    _throwif(m3Err_wasmMalformed, عدد_Globals == 0 or عدد_Globals != module->عدد_Globals);

    لكل (u32 g = 0; g < عدد_Globals; ++g) {
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
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Tables (M3_Snapshot_لدو* l)
{
    M3_نتیجہ  نتیجہ       = m3Err_none;
    IM3Module module       = l->module;
    u32       عدد_Tables    = 0;
    l->loaded_جدول_Elements = 0;

_   (GetLEB_u32(l, &عدد_Tables));
    _throwif(m3Err_wasmMalformed, عدد_Tables == 0 or عدد_Tables != module->عدد_Tables);

    لكل (u32 t = 0; t < عدد_Tables; ++t) {
        u32      جدول_اشاریہ = 0, پہلا_اشاریہ = 0, ناپ = 0;
        u8       valtype = 0;
        IM3_جدول جدول_2;

_       (GetLEB_u32(l, &جدول_اشاریہ));
_       (GetLEB_u32(l, &پہلا_اشاریہ));

        _throwif(m3Err_wasmMalformed, جدول_اشاریہ != t);
        _throwif("the snapshot shares tables between imports differently",
                 پہلا_اشاریہ != جدول_پہلا_اشاریہ(module, t));
        إذا (پہلا_اشاریہ != t) {
            استمر;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &ناپ));

        جدول_2 = module->tables[جدول_اشاریہ];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_منزل_Val_Type(جدول_2->type));
        _throwif(m3Err_wasmMalformed, ناپ > (جدول_2->hasMax ? جدول_2->max_ناپ : d_m3_Max_Sane_جدول_ناپ));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, ناپ < جدول_2->ناپ);

        l->loaded_جدول_Elements += ناپ;

        إذا (not l->checking and ناپ != جدول_2->ناپ) {
            فراغ** elements;

            _throwif(m3_Err_جدول_حد_بندی_Exceeded, l->runtime->جدول_Elements_حد_بندی and
                                                 ناپ - جدول_2->ناپ > l->runtime->جدول_Elements_حد_بندی - l->runtime->جدول_Elements_ورتیا);
            elements = m3_ReallocArray(فراغ*, جدول_2->elements, ناپ, جدول_2->ناپ);
            _throwifnull(elements);

            l->runtime->جدول_Elements_ورتیا += ناپ - جدول_2->ناپ;
            جدول_2->elements = elements;
            جدول_2->ناپ     = ناپ;
        }

        لكل (u32 e = 0; e < ناپ; ++e) {
            فراغ* reference = عدم;
_           (Get_Ref_قدر(l, جدول_2->type, &reference));
            إذا (not l->checking) {
                جدول_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_جدول_Elements != l->required_جدول_Elements);

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Segments (M3_Snapshot_لدو* l)
{
    M3_نتیجہ  نتیجہ = m3Err_none;
    IM3Module module = l->module;
    u32       گنتی  = 0;

_   (GetLEB_u32(l, &گنتی));
    _throwif(m3Err_wasmMalformed, گنتی != module->عدد_انکڑے_Segments);
    _throwif(m3Err_wasmMalformed, module->عدد_انکڑے_Segments == 0 and module->عدد_Element_Segments == 0);

    لكل (u32 i = 0; i < گنتی; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->انکڑے_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &گنتی));
    _throwif(m3Err_wasmMalformed, گنتی != module->عدد_Element_Segments);

    لكل (u32 i = 0; i < گنتی; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    أرجع نتیجہ;
}

// A frame for the continuation being put together, outermost first
ساكن
M3_نتیجہ جوڑو_چوکھٹا (M3_Snapshot_لدو* l, u32* io_عدد_چوکھٹے, M3_چوکھٹا_قسم i_قسم, m3stack_t i_sp, M3_چوکھٹا** o_چوکھٹا)
{
    إذا (*io_عدد_چوکھٹے >= d_m3_Continuation_Max_چوکھٹے) {
        أرجع "the snapshot's frames do not fit this build's";
    }

    M3_چوکھٹا* چوکھٹا = &l->چوکھٹے[(*io_عدد_چوکھٹے)++];

    memset(چوکھٹا, 0, حجم(*چوکھٹا));
    چوکھٹا->قسم   = (u8)i_قسم;
    چوکھٹا->sp     = i_sp;
    چوکھٹا->حافظہ = Module_Memory0(l->module);

    *o_چوکھٹا = چوکھٹا;

    أرجع m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ساكن
M3_نتیجہ PrepareFunction (IM3Function i_function)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    أرجع نتیجہ;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ساكن
M3_نتیجہ جوڑو_بلاک_چوکھٹے (M3_Snapshot_لدو* l, ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, m3stack_t i_sp,
                         u32* io_عدد_چوکھٹے)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    لكل (u32 b = 0; b < i_map->عدد_بلاکاں; ++b) {
        ثابت M3_بلاک_شروع* بلاک = &i_map->بلاکاں[b];
        M3_چوکھٹا*            چوکھٹا;

        إذا (not Encloses_وتھ(بلاک, i_نقطہ->wasm_وتھ)) {
            استمر;
        }

        إذا (بلاک->opcode == c_waOp_loop) {
_           (جوڑو_چوکھٹا(l, io_عدد_چوکھٹے, چوکھٹا_loop, i_sp, &چوکھٹا));
            چوکھٹا->pc = بلاک->pc;
        }
#  إذا d_m3HasExceptionHandling
        وإلا {
_           (جوڑو_چوکھٹا(l, io_عدد_چوکھٹے, چوکھٹا_try, i_sp, &چوکھٹا));
            چوکھٹا->pc                = بلاک->pc;
            چوکھٹا->try_.عدد_Clauses   = بلاک->عدد_Clauses;
            چوکھٹا->try_.handlers_جیوندا = Handlers_جیوندا_At(i_map, i_نقطہ, بلاک);
        }
#  endif
    }

_catch:
    أرجع نتیجہ;
}

// Puts a function's values where this build keeps them at the safepoint
ساكن
M3_نتیجہ لدو_اقدار (M3_Snapshot_لدو* l, m3slot_t* i_ادھار, u32 i_عدد_Slots, m3stack_t i_sp,
                     ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطہ* i_نقطہ, ثابت M3_چوکھٹا_Registers* i_registers)
{
    M3_نتیجہ نتیجہ    = m3Err_none;
    u32      عدد_اقدار = 0;

_   (GetLEB_u32(l, &عدد_اقدار));
    _throwif("the snapshot's frame does not match what this build compiled", عدد_اقدار != i_map->عدد_Locals + i_نقطہ->عدد_اقدار);

    لكل (u32 v = 0; v < عدد_اقدار; ++v) {
        ثابت M3_تھاں_قدر* قدر     = چوکھٹا_قدر(i_map, i_نقطہ, v);
        u64                لفظ      = 0;
        فراغ*              reference = عدم;
        u8*                where;

_       (Locate_قدر(i_ادھار, i_عدد_Slots, i_sp, قدر, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (ادھار_Type_دا(قدر->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_قدر_ٹیگ(l, قدر->type, &type));
            إذا (l->checking) {
_               (Skip_بائٹاں(l, 16));
            } وإلا {
_               (Get(l, where, 16));
            }
            استمر;
        }

        l->allowUnfilledRef = i_نقطہ->قسم == safepoint_suspend and
                              v >= عدد_اقدار - i_نقطہ->عدد_Results;
_       (Get_قدر_بٹ(l, قدر->type, &لفظ));
        l->allowUnfilledRef = false;

        إذا (IsRefType(قدر->type)) {
            reference = (فراغ*)(uintptr_t)لفظ;
        }

        إذا (not l->checking) {
_           (Write_قدر(where, i_registers, قدر, لفظ, reference));
        }
    }

_catch:
    أرجع نتیجہ;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ساكن
M3_نتیجہ لدو_Suspended_Body (M3_Snapshot_لدو* l, IM3Continuation io_cont, u32 i_id)
{
    M3_نتیجہ    نتیجہ    = m3Err_none;
    u32         عدد_چوکھٹے = 0;
    M3_چوکھٹا*    call_چوکھٹا = عدم;
    IM3Function function  = عدم;
    m3slot_t*   ادھار;
    u32         عدد_Slots;
    u32         عدد_Activations = 0;
    IM3FuncType expected       = io_cont->اندراج_Function->funcType;
    m3stack_t   sp;

_   (Continuation_ڈھیر(l->runtime, io_cont, &ادھار, &عدد_Slots));

_   (GetLEB_u32(l, &عدد_Activations));
    _throwif(m3Err_wasmMalformed, عدد_Activations == 0 or عدد_Activations > d_m3_Continuation_Max_چوکھٹے);

    io_cont->pc                = عدم;
    io_cont->r0                = 0;
    io_cont->عدد_Suspend_Results = 0;
    io_cont->suspend_نقطہ      = safepoint_op;
#  إذا d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = ادھار;

    لكل (u32 a = 0; a < عدد_Activations; ++a) {
        bool                 isInnermost = (a + 1 == عدد_Activations);
        ثابت M3_محفوظ_نقطہ*   نقطہ;
        M3_چوکھٹا_Registers     registers;
        u8                   قسم       = 0;
        u32                  wasm_وتھ = 0, نشانہ_Loop = 0;
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
                 (u32)(sp - ادھار) + function->max_ڈھیر_Slots > عدد_Slots);

        // the call the frame outside this one is waiting on
        إذا (call_چوکھٹا) {
            call_چوکھٹا->call.function = function;
        }

#  إذا d_m3_اندراج_Keeps_چوکھٹا
        {
            M3_چوکھٹا* اندراج;
_           (جوڑو_چوکھٹا(l, &عدد_چوکھٹے, چوکھٹا_اندراج, sp, &اندراج));
            اندراج->pc             = function->compiled;
            اندراج->اندراج.function = function;
        }
#  endif

_       (GetU8(l, &قسم));
_       (GetLEB_u32(l, &wasm_وتھ));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        إذا (قسم == safepoint_op) {
            ثابت M3_بلاک_شروع* loop;

_           (GetLEB_u32(l, &نشانہ_Loop));

            loop = لبھو_بلاک_At(map, c_waOp_loop, نشانہ_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_وتھ(loop, wasm_وتھ));
        }

        نقطہ = لبھو_محفوظ_نقطہ_At(map, wasm_وتھ, قسم, نشانہ_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not نقطہ);

_       (جوڑو_بلاک_چوکھٹے(l, map, نقطہ, sp, &عدد_چوکھٹے));

        إذا (not isInnermost) {
            _throwif(m3Err_wasmMalformed, قسم != safepoint_call);

_           (جوڑو_چوکھٹا(l, &عدد_چوکھٹے, چوکھٹا_call, sp, &call_چوکھٹا));
            call_چوکھٹا->pc = نقطہ->pc;
            registers.r0  = &call_چوکھٹا->call.r0;
#  إذا d_m3HasFloat
            registers.fp0 = &call_چوکھٹا->call.fp0;
#  endif
        } وإلا إذا (قسم == safepoint_resume) {
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
        } وإلا {
            _throwif(m3Err_wasmMalformed, قسم != safepoint_op and قسم != safepoint_اندراج and قسم != safepoint_suspend);

            io_cont->pc           = نقطہ->pc;
            io_cont->suspend_نقطہ = قسم;
            registers.r0          = &io_cont->r0;
#  إذا d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (لدو_اقدار(l, ادھار, عدد_Slots, sp, map, نقطہ, &registers));

        إذا (not l->checking) {
            RestoreConstants(function, sp);
        }

        إذا (not isInnermost) {
            sp = sp + نقطہ->aux;
_           (CallSiteType(function, نقطہ, &expected));
        } وإلا إذا (قسم == safepoint_resume) {
            فراغ* cont = عدم;
_           (Get_Ref_قدر(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == عدم);

            call_چوکھٹا->resume.cont = (IM3Continuation)cont;
            إذا (not l->checking) {
                l->suspendSites[i_id].نقطہ = نقطہ;
                لكل (u32 i = 1; i < l->عدد_Continuations; ++i) {
                    إذا (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].نشانہ = i;
                        l->suspendSites[i].resumed   = true;
                        اكسر;
                    }
                }
            }
        } وإلا {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            إذا (نقطہ->flags & d_m3_محفوظ_نقطہ_لیا_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            إذا (قسم == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, نقطہ->عدد_Results > نقطہ->عدد_اقدار or
                                                نقطہ->عدد_Results > d_m3MaxContinuationPayload);

                io_cont->عدد_Suspend_Results = نقطہ->عدد_Results;
                إذا (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].نقطہ = نقطہ;
                }

                لكل (u32 i = 0; i < نقطہ->عدد_Results; ++i) {
                    ثابت M3_تھاں_قدر* قدر = &map->اقدار[نقطہ->پہلا_قدر + نقطہ->عدد_اقدار - نقطہ->عدد_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_رجسٹر_تھاں_Alias(قدر->تھاں));

                    io_cont->suspend_نتیجہ_Offsets[i] = قدر->تھاں;
                    io_cont->suspend_نتیجہ_Is64[i]    = Is64_ثنائی_ہندسہ_Type(قدر->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    إذا (l->checking) {
        أرجع m3Err_none;
    }

    // held innermost first
    إذا (عدد_چوکھٹے > io_cont->چوکھٹے_Cap) {
        M3_چوکھٹا* چوکھٹے = m3_ReallocArray(M3_چوکھٹا, io_cont->چوکھٹے, عدد_چوکھٹے, io_cont->چوکھٹے_Cap);
        _throwifnull(چوکھٹے);

        io_cont->چوکھٹے    = چوکھٹے;
        io_cont->چوکھٹے_Cap = عدد_چوکھٹے;
    }

    لكل (u32 k = 0; k < عدد_چوکھٹے; ++k) {
        io_cont->چوکھٹے[عدد_چوکھٹے - 1 - k] = l->چوکھٹے[k];
    }

    io_cont->عدد_چوکھٹے = عدد_چوکھٹے;

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Continuation (M3_Snapshot_لدو* l, u32 i_id)
{
    M3_نتیجہ        نتیجہ = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    إذا (l->checking) {
        scratch          = *cont;
        scratch.val_ڈھیر = l->check_ڈھیر;
        إذا (i_id == 0) {
            scratch.val_ڈھیر = (m3slot_t*)l->runtime->origin_ڈھیر;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_اشاریہ = 0, اندراج_اشاریہ = 0, bound_دلیلاں_گنتی = 0;
    u8  state = 0, is_جڑ = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_جڑ));
_   (GetLEB_u32(l, &type_اشاریہ));
_   (GetLEB_u32(l, &اندراج_اشاریہ));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_جڑ != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_جڑ and state != snapshot_contSuspended);
    إذا (not is_جڑ and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_اشاریہ != d_m3SnapshotNone and type_اشاریہ >= module->عدد_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, اندراج_اشاریہ >= module->عدد_Functions);
    _throwif(m3Err_wasmMalformed, is_جڑ ? type_اشاریہ != d_m3SnapshotNone : (type_اشاریہ == d_m3SnapshotNone or not module->funcTypes[type_اشاریہ]->isContinuation));

    إذا (not l->checking and not is_جڑ and state != snapshot_cont_مکمل) {
_       (Continuation_Acquire_ڈھیر(l->runtime, cont));
    }
    cont->type          = (type_اشاریہ != d_m3SnapshotNone) ? module->funcTypes[type_اشاریہ] : عدم;
    cont->اندراج_Function = &module->functions[اندراج_اشاریہ];
    cont->عدد_Handlers   = 0;
    cont->handlersPC    = عدم;
    cont->ماپا        = عدم;
    cont->عدد_چوکھٹے     = 0;

#  إذا d_m3HasExceptionHandling
    {
        فراغ* ref = عدم;
_       (Get_Ref_قدر(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_جڑ or state == snapshot_cont_مکمل) and cont->resumeThrow != عدم);
#  وإلا
    {
        فراغ* ref = عدم;
_       (Get_Ref_قدر(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != عدم);
    }
#  endif

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         عدد_Rets;

_       (GetLEB_u32(l, &bound_دلیلاں_گنتی));
        cont->bound_دلیلاں_گنتی = bound_دلیلاں_گنتی;
        inner                = cont->اندراج_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_دلیلاں_گنتی > Get_Func_Type_عدد_Params(inner));

        {
            IM3FuncType بچیا = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not بچیا or
                                            Get_Func_Type_عدد_Params(بچیا) != Get_Func_Type_عدد_Params(inner) - bound_دلیلاں_گنتی or
                                            Get_Func_Type_عدد_Results(بچیا) != Get_Func_Type_عدد_Results(inner));
            لكل (u16 i = 0; i < Get_Func_Type_عدد_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجہ_Type(inner, i) != Get_Func_Type_نتیجہ_Type(بچیا, i));
            }
            لكل (u16 i = 0; i < Get_Func_Type_عدد_Params(بچیا); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_دلیلاں_گنتی + i)) != GetFuncTypeParamType(بچیا, i));
            }
        }

        عدد_Rets = Get_Func_Type_عدد_Results(inner);

        _throwif(m3Err_wasmMalformed, (عدد_Rets + bound_دلیلاں_گنتی) * c_io_تھاں_گنتی > cont->عدد_ڈھیر_Slots);

        لكل (u32 i = 0; i < bound_دلیلاں_گنتی; ++i) {
_           (Get_قدر(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ڈھیر + (عدد_Rets + i) * c_io_تھاں_گنتی));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ڈھیر;
        cont->pc    = cont->اندراج_Function->compiled;
    } وإلا إذا (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_دلیلاں_گنتی));
        _throwif(m3Err_wasmMalformed, is_جڑ and bound_دلیلاں_گنتی != 0);
        cont->bound_دلیلاں_گنتی = bound_دلیلاں_گنتی;

_       (لدو_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } وإلا إذا (state == snapshot_cont_مکمل) {
_       (GetLEB_u32(l, &bound_دلیلاں_گنتی));
        _throwif(m3Err_wasmMalformed, bound_دلیلاں_گنتی != 0);
        cont->bound_دلیلاں_گنتی = 0;
        cont->state          = cont_consumed;
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    إذا (نتیجہ) {
        cont->عدد_چوکھٹے = 0;
    }

    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Continuations (M3_Snapshot_لدو* l)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      گنتی  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &گنتی));
    _throwif(m3Err_wasmMalformed, گنتی == 0 or گنتی != l->عدد_Continuations);

    لكل (u32 i = 0; i < گنتی; ++i) {
_       (لدو_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    أرجع نتیجہ;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ساكن
M3_نتیجہ CheckContinuationReferences (M3_Snapshot_لدو* l)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    لكل (u32 id = 0; id < l->عدد_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             پتی  = id;
        u32             ڈونگھائی = 0;
#  إذا d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_دا(Ref_Type_دا_Func_Type(cont->type, true), expectedType)));
#  endif
        إذا (l->suspendSites[id].نشانہ) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_دا_Ref(l->module, l->suspendSites[id].نقطہ->resumeType);
            IM3Continuation بچہ    = l->continuations[l->suspendSites[id].نشانہ];
            IM3FuncType     actual   = بچہ->اندراج_Function ? بچہ->اندراج_Function->funcType : عدم;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_عدد_Results(actual) != Get_Func_Type_عدد_Results(expected));
            لكل (u16 i = 0; i < Get_Func_Type_عدد_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجہ_Type(actual, i) != Get_Func_Type_نتیجہ_Type(expected, i));
            }
        }
        طالما (l->suspendSites[پتی].نشانہ) {
            پتی = l->suspendSites[پتی].نشانہ;
            _throwif(m3Err_wasmMalformed, ++ڈونگھائی >= l->عدد_Continuations or
                                            l->continuations[پتی]->state != cont_suspended);
        }
        إذا (cont->state != cont_suspended or id == 0) {
            استمر;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        إذا (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_دلیلاں_گنتی != 0);
            استمر;
        }

        ثابت M3SnapshotSuspendSite* site      = &l->suspendSites[پتی];
        IM3Continuation              inner     = l->continuations[پتی];
        IM3FuncType                  بچیا = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->نقطہ or not بچیا);
        _throwif("a captured continuation has the wrong binding count", cont->bound_دلیلاں_گنتی > inner->عدد_Suspend_Results or
                                                                          Get_Func_Type_عدد_Params(بچیا) != inner->عدد_Suspend_Results - cont->bound_دلیلاں_گنتی);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_عدد_Results(بچیا) != Get_Func_Type_عدد_Results(cont->اندراج_Function->funcType));
        لكل (u16 i = 0; i < Get_Func_Type_عدد_Results(بچیا); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجہ_Type(بچیا, i) !=
                                            Get_Func_Type_نتیجہ_Type(cont->اندراج_Function->funcType, i));
        }

        لكل (u32 i = 0; i < inner->عدد_Suspend_Results; ++i) {
            ثابت M3_تھاں_قدر* قدر = &site->map->اقدار[site->نقطہ->پہلا_قدر +
                                                          site->نقطہ->عدد_اقدار - site->نقطہ->عدد_Results + i];
            إذا (i < cont->bound_دلیلاں_گنتی) {
                إذا (IsRefType(قدر->type) and not IsNullableRef(قدر->type)) {
                    فراغ* ref = عدم;
                    حافظہ_نقل(&ref, inner->sp + قدر->تھاں, حجم(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } وإلا {
                _throwif("a captured continuation has the wrong parameter type", قدر->type !=
                                                                                   GetFuncTypeParamType(بچیا, (u16)(i - cont->bound_دلیلاں_گنتی)));
            }
        }
    }

_catch:
    أرجع نتیجہ;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_نتیجہ لدو_Exceptions (M3_Snapshot_لدو* l)
{
    M3_نتیجہ نتیجہ        = m3Err_none;
    u32      عدد_Exceptions = 0;

_   (GetLEB_u32(l, &عدد_Exceptions));
    _throwif(m3Err_wasmMalformed, عدد_Exceptions == 0 or عدد_Exceptions != l->عدد_Exceptions);

    لكل (u32 i = 0; i < عدد_Exceptions; ++i) {
        u32          exnId = 0, عدد_دلیلاں = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &عدد_دلیلاں));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->ٹیگ->type;

        _throwif(m3Err_wasmMalformed, عدد_دلیلاں != exn->عدد_دلیلاں);

        لكل (u32 a = 0; a < عدد_دلیلاں; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      بٹ    = 0;
_           (Get_قدر_بٹ(l, argType, &بٹ));
            إذا (not l->checking) {
                exn->دلیلاں[a] = بٹ;
            }
        }
    }

_catch:
    أرجع نتیجہ;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
تعريفنوع بنية M3BoundedReader {
    M3_Snapshot_لدو* لدو;
    u64             بچیا;
    M3_نتیجہ        error;
} M3BoundedReader;

ساكن
M3_نتیجہ BoundedReader_Read (فراغ* o_بفر, size_t i_ناپ, فراغ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    إذا (i_ناپ > br->بچیا) {
        br->error = "the host read past the end of its state";
        أرجع br->error;
    }

    br->بچیا -= i_ناپ;

    أرجع Get(br->لدو, o_بفر, i_ناپ);
}

ساكن
M3_نتیجہ لدو_Host_State (M3_Snapshot_لدو* l)
{
    M3_نتیجہ               نتیجہ = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 ناپ = (size_t)(l->سرا - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->لدو_Host_State_2);

    إذا (l->checking) {
        أرجع Skip_بائٹاں(l, ناپ);
    }
    reader.لدو      = l;
    reader.بچیا = ناپ;
    reader.error     = m3Err_none;

_   (hooks->لدو_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, ناپ));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.بچیا != 0);

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_سرخی (M3_Snapshot_لدو* l)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u8       magic[حجم(c_snapshotMagic)];
    u32      نسخہ = 0;

_   (Get(l, magic, حجم(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, حجم(magic)) != 0);

_   (GetLE32(l, &نسخہ));
    _throwif(m3Err_wasmMalformed, نسخہ != c_snapshot_نسخہ);

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Meta (M3_Snapshot_لدو* l, u32* o_flags)
{
    M3_نتیجہ نتیجہ      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      حافظہ_بائٹاں = 0, جدول_Elements = 0;
    u32      سرگرم_Stacks = 0;
    u64      current_حافظہ, currentTables;
    SnapshotResourceTotals(l->module, &current_حافظہ, &currentTables);

_   (GetLEB_u32(l, o_flags));
_   (GetLEB_u64(l, &timestamp));
_   (GetLEB_u64(l, &moduleHash));

    (فراغ)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->عدد_Continuations));
_   (GetLEB_u32(l, &l->عدد_Exceptions));
_   (GetLEB_u64(l, &حافظہ_بائٹاں));
_   (GetLEB_u64(l, &جدول_Elements));
_   (GetLEB_u32(l, &سرگرم_Stacks));
    l->requiredStacks        = سرگرم_Stacks;
    l->required_حافظہ_بائٹاں   = حافظہ_بائٹاں;
    l->required_جدول_Elements = جدول_Elements;
    _throwif(m3Err_wasmMalformed, حافظہ_بائٹاں < current_حافظہ or جدول_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->عدد_Memories and حافظہ_بائٹاں != 0);
    _throwif(m3Err_wasmMalformed, not l->module->عدد_Tables and جدول_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    إذا (*o_flags & d_m3SnapshotFlagPostmortem) {
        لكل (u32 i = 0; i < l->عدد_Exceptions; ++i) {
            u32 ٹیگ_اشاریہ = 0;
_           (GetLEB_u32(l, &ٹیگ_اشاریہ));
        }
        أرجع m3Err_none;
    }

    _throwif(m3_Err_حافظہ_حد_بندی_Exceeded, l->runtime->حافظہ_بائٹاں_حد_بندی and
                                          حافظہ_بائٹاں - current_حافظہ > l->runtime->حافظہ_بائٹاں_حد_بندی - l->runtime->حافظہ_بائٹاں_ورتیا);
    _throwif(m3_Err_جدول_حد_بندی_Exceeded, l->runtime->جدول_Elements_حد_بندی and
                                         جدول_Elements - currentTables > l->runtime->جدول_Elements_حد_بندی - l->runtime->جدول_Elements_ورتیا);
    _throwif(m3_Err_continuation_حد_بندی_Exceeded, l->runtime->continuations_حد_بندی and
                                                سرگرم_Stacks > l->runtime->continuations_حد_بندی - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->عدد_Continuations == 0 or سرگرم_Stacks >= l->عدد_Continuations);
#  إذا !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->عدد_Exceptions != 0);
#  endif

    إذا (l->checking) {
        لكل (u32 i = 0; i < l->عدد_Exceptions; ++i) {
            u32 ٹیگ_اشاریہ = 0;
_           (GetLEB_u32(l, &ٹیگ_اشاریہ));
#  إذا d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, ٹیگ_اشاریہ >= l->module->عدد_ٹیگ_سوچی);
#  endif
        }
        أرجع m3Err_none;
    }

    l->check_ڈھیر = m3_AllocArray(m3slot_t, d_m3_Continuation_ڈھیر_Slots + 4);
    _throwifnull(l->check_ڈھیر);
    l->چوکھٹے = m3_AllocArray(M3_چوکھٹا, d_m3_Continuation_Max_چوکھٹے);
    _throwifnull(l->چوکھٹے);

    l->continuations = m3_AllocArray(IM3Continuation, l->عدد_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->عدد_Continuations);
    _throwifnull(l->suspendSites);

    إذا (not l->runtime->جڑ_Continuation) {
        IM3Continuation جڑ = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(جڑ);

        // the root borrows the runtime's stack, so it holds none of its own
        جڑ->عدد_ڈھیر_Slots          = l->runtime->عدد_ڈھیر_Slots;
        l->runtime->جڑ_Continuation = جڑ;
    }

    l->continuations[0] = l->runtime->جڑ_Continuation;

    لكل (u32 i = 1; i < l->عدد_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(l->continuations[i]);
    }

#  إذا d_m3HasExceptionHandling
    إذا (l->عدد_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->عدد_Exceptions);
        _throwifnull(l->exceptions);
    }

    لكل (u32 i = 0; i < l->عدد_Exceptions; ++i) {
        u32    ٹیگ_اشاریہ = 0;
        IM3_ٹیگ ٹیگ;
        u32    عدد_دلیلاں;

_       (GetLEB_u32(l, &ٹیگ_اشاریہ));
        _throwif(m3Err_wasmMalformed, ٹیگ_اشاریہ >= l->module->عدد_ٹیگ_سوچی);

        ٹیگ     = ٹیگ_دا_اشاریہ(l->module, ٹیگ_اشاریہ);
        عدد_دلیلاں = Get_Func_Type_عدد_Params(ٹیگ->type);

        l->exceptions[i] = NewException(l->runtime, ٹیگ, عدد_دلیلاں);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  وإلا
    لكل (u32 i = 0; i < l->عدد_Exceptions; ++i) {
        u32 ٹیگ_اشاریہ = 0;
_       (GetLEB_u32(l, &ٹیگ_اشاریہ));
    }
#  endif

_catch:
    أرجع نتیجہ;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ساكن
M3_نتیجہ لدو_Section (M3_Snapshot_لدو* l, u8 i_sectionId)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    u32      flags  = 0;
    بدل (i_sectionId) {
    حالة d_m3SnapshotSection_Meta:
_       (لدو_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        اكسر;

    حالة d_m3_Snapshot_Section_حافظہ:
_       (لدو_Memories(l));
        اكسر;

    حالة d_m3_Snapshot_Section_جدول:
_       (لدو_Tables(l));
        اكسر;

    حالة d_m3SnapshotSection_Global:
_       (لدو_Globals(l));
        اكسر;

    حالة d_m3SnapshotSection_Segment:
_       (لدو_Segments(l));
        اكسر;

#  إذا d_m3HasExceptionHandling
    حالة d_m3SnapshotSection_Exception:
_       (لدو_Exceptions(l));
        اكسر;
#  endif

    حالة d_m3SnapshotSection_Continuation:
_       (لدو_Continuations(l));
        اكسر;

    حالة d_m3SnapshotSection_HostState:
_       (لدو_Host_State(l));
        اكسر;

    افتراضي:
_       (Skip_بائٹاں(l, (size_t)(l->سرا - l->pos)));
        اكسر;
    }

_catch:
    أرجع نتیجہ;
}

ساكن
M3_نتیجہ لدو_Sections (M3_Snapshot_لدو* l, bool i_embedded)
{
    M3_نتیجہ نتیجہ  = m3Err_none;
    u8*      payload = عدم;

    إذا (not i_embedded) {
_       (لدو_سرخی(l));
    }

    طالما (true) {
        u8  sectionId   = 0;
        u32 section_ناپ = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        إذا (GetU8(l, &sectionId)) {
            اكسر;
        }

_       (GetLEB_u32(l, &section_ناپ));

        _throwif("the snapshot holds a section twice", l->تکیا_Sections[sectionId]);
        l->تکیا_Sections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_ناپ == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_ناپ);
        _throwifnull(payload);
_       (Get(l, payload, section_ناپ));
        l->pos       = payload;
        l->سرا       = payload + section_ناپ;
        l->وچ_Section = true;

        {
            M3_Snapshot_لدو check = *l;
            check.checking       = true;
_           (لدو_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.سرا);
        }
        l->بدلیا = l->بدلیا or sectionId != d_m3SnapshotSection_Meta;
_       (لدو_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->سرا);
        l->وچ_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->تکیا_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->عدد_Exceptions and not l->تکیا_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->عدد_Memories and not l->تکیا_Sections[d_m3_Snapshot_Section_حافظہ]);
    _throwif("the snapshot has no table section",
             l->module->عدد_Tables and not l->تکیا_Sections[d_m3_Snapshot_Section_جدول]);
    _throwif("the snapshot has no global section",
             l->module->عدد_Globals and not l->تکیا_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->عدد_انکڑے_Segments or l->module->عدد_Element_Segments) and
               not l->تکیا_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    أرجع نتیجہ;
}

#جے_انت // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3HasSnapshots

ساكن
M3_نتیجہ SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata,
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

    IM3Function اندراج = save.postmortem ? io_runtime->entered : جڑ->اندراج_Function;

    إذا (o_module) {
        *o_module = عدم;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not اندراج);

    save.module = اندراج->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    إذا (o_module) {
        *o_module = save.module;
    }

    لكل (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->اگلا) {
        u32 id;
_       (اشارہ_گر_Ids_جوڑو(&save.جیوندا_Continuations, cont, &id));
    }

#  إذا d_m3HasExceptionHandling
    لكل (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->اگلا) {
        u32 id;
_       (اشارہ_گر_Ids_جوڑو(&save.جیوندا_Exceptions, exception, &id));
    }
#  endif

    إذا (not save.postmortem) {
        u32 id;
_       (اشارہ_گر_Ids_جوڑو(&save.continuations, جڑ, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    اشارہ_گر_Ids_Free(&save.جیوندا_Continuations);
    اشارہ_گر_Ids_Free(&save.جیوندا_Exceptions);
    اشارہ_گر_Ids_Free(&save.continuations);
    اشارہ_گر_Ids_Free(&save.exceptions);

    أرجع نتیجہ;
}

#جے_انت

M3_نتیجہ m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata)
{
    إذا (not io_runtime or not i_writer) {
        أرجع m3Err_mallocFailed;
    }

#إذا d_m3HasSnapshots
    أرجع SaveSnapshot(io_runtime, i_writer, i_userdata, عدم, false);
#وإلا
    (فراغ)i_userdata;
    أرجع "snapshots are not available in this build of Wasm3";
#جے_انت
}


ساكن
M3_نتیجہ لدو_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata, bool i_embedded)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#إذا d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_لگاتار);

    {
        M3_Snapshot_لدو لدو;
        memset(&لدو, 0, حجم(لدو));

        لدو.runtime  = io_runtime;
        لدو.module   = i_module;
        لدو.reader   = i_reader;
        لدو.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        نتیجہ = لدو_Sections(&لدو, i_embedded);

        IM3Continuation جڑ = io_runtime->جڑ_Continuation;

        إذا (نتیجہ) {
            إذا (لدو.بدلیا) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                إذا (جڑ) {
                    جڑ->state     = cont_returned;
                    جڑ->عدد_چوکھٹے = 0;
                }
            }
        } وإلا {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->شروع_Function = -1;
            i_module->has_لگاتار        = true;
        }

        Runtime_رکھو_Call_ڈھیر(io_runtime);

        m3_Free(لدو.check_ڈھیر);
        m3_Free(لدو.چوکھٹے);
        m3_Free(لدو.suspendSites);
        m3_Free(لدو.continuations);
#  إذا d_m3HasExceptionHandling
        m3_Free(لدو.exceptions);
#  endif
    }
#وإلا
    (فراغ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#جے_انت

_catch:
    أرجع نتیجہ;
}


M3_نتیجہ m3_لدو_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata)
{
    أرجع لدو_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


فراغ m3_سیٹ_Snapshot_Hooks (IM3Runtime io_runtime, ثابت M3SnapshotHooks* i_hooks)
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
#جے_انت
}


M3_نتیجہ m3_Save_Snapshot_منزل_بفر (IM3Runtime io_runtime, فراغ** o_بائٹاں, size_t* o_ناپ)
{
    إذا (!io_runtime || !o_بائٹاں || !o_ناپ) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    M3_بفر_Writer bw     = { عدم, 0, 0 };
    M3_نتیجہ       نتیجہ = m3_SaveSnapshot(io_runtime, بفر_Writer_Write, &bw);
    إذا (نتیجہ) {
        m3_Free(bw.بفر);
        *o_بائٹاں = عدم;
        *o_ناپ  = 0;
        أرجع نتیجہ;
    }
    *o_بائٹاں = bw.بفر;
    *o_ناپ  = bw.ناپ;
    أرجع m3Err_none;
#وإلا
    *o_بائٹاں = عدم;
    *o_ناپ  = 0;
    أرجع "snapshots are not available in this build of Wasm3";
#جے_انت
}

#إذا d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ساكن
u8* EmitLEB_u32 (u8* o_where, u32 i_قدر)
{
    افعل {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        إذا (i_قدر != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } طالما (i_قدر != 0);

    أرجع o_where;
}

ساكن
u32 ناپ_دا_LEB_u32 (u32 i_قدر)
{
    u32 ناپ = 0;

    افعل {
        i_قدر >>= 7;
        ++ناپ;
    } طالما (i_قدر != 0);

    أرجع ناپ;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ساكن
M3_نتیجہ لبھو_Embedded_Snapshot (ثابت u8* i_شروع, ثابت u8* i_سرا, cstr_t i_secName,
                               ثابت u8** o_sec_شروع, ثابت u8** o_sec_سرا,
                               ثابت u8** o_انکڑے, u32* o_ناپ)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    بائٹاں_t  pos    = i_شروع;
    bool     لبھیا  = false;
    cstr_t   name   = عدم;

    إذا (o_sec_شروع) {
        *o_sec_شروع = عدم;
    }
    إذا (o_sec_سرا) {
        *o_sec_سرا = عدم;
    }
    إذا (o_انکڑے) {
        *o_انکڑے = عدم;
    }
    إذا (o_ناپ) {
        *o_ناپ = 0;
    }

    _throwif(m3Err_wasmMalformed, i_سرا - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    طالما (pos < i_سرا) {
        بائٹاں_t sec_شروع  = pos;
        u8      sectionId = *pos++;
        u32     ناپ      = 0;
_       (ReadLEB_u32(&ناپ, &pos, i_سرا));
        _throwif(m3Err_wasmUnderrun, ناپ > (size_t)(i_سرا - pos));
        {
            بائٹاں_t payload = pos;
            pos += ناپ;
            إذا (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                إذا (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", لبھیا);
                    لبھیا = true;
                    إذا (o_sec_شروع) {
                        *o_sec_شروع = sec_شروع;
                    }
                    إذا (o_sec_سرا) {
                        *o_sec_سرا = pos;
                    }
                    إذا (o_انکڑے) {
                        *o_انکڑے = payload;
                    }
                    إذا (o_ناپ) {
                        *o_ناپ = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    أرجع نتیجہ;
}

// Builds the custom section name a snapshot of i_name lives under
ساكن
M3_نتیجہ SnapshotSectionName (cstr_t i_name, حرف* o_secName, size_t i_ناپ)
{
    إذا (i_name and *i_name) {
        إذا (لڑی_لمبائی(i_name) > d_m3_Max_Snapshot_Name_لمائی) {
            أرجع "the snapshot name is too long";
        }
        snprintf(o_secName, i_ناپ, "snapshot.%s", i_name);
    } وإلا {
        snprintf(o_secName, i_ناپ, "snapshot");
    }

    أرجع m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ساكن
M3_نتیجہ ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_نتیجہ نتیجہ = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = عدم;

    لكل (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->اگلا) {
        إذا (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    أرجع نتیجہ;
}
#جے_انت

M3_نتیجہ m3_لدو_Snapshot_From_بفر (IM3Runtime io_runtime, IM3Module i_module, ثابت فراغ* i_بائٹاں, size_t i_ناپ)
{
    إذا (!io_runtime || !i_module || !i_بائٹاں) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    إذا (i_ناپ >= 8 && memcmp(i_بائٹاں, "\0asm", 4) == 0) {
        ثابت u8* انکڑے   = عدم;
        u32       ناپ   = 0;
        M3_نتیجہ  نتیجہ = لبھو_Embedded_Snapshot((ثابت u8*)i_بائٹاں, (ثابت u8*)i_بائٹاں + i_ناپ,
                                                "snapshot", عدم, عدم, &انکڑے, &ناپ);
        إذا (نتیجہ) {
            أرجع نتیجہ;
        }
        إذا (not انکڑے) {
            أرجع "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_بفر_Reader br = { انکڑے, ناپ, 0 };
            أرجع لدو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
        }
    }
    {
        M3_بفر_Reader br = { (ثابت u8*)i_بائٹاں, i_ناپ, 0 };
        أرجع لدو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br,
                            i_ناپ < 4 or memcmp(i_بائٹاں, c_snapshotMagic, 4) != 0);
    }
#وإلا
    (فراغ)i_ناپ;
    أرجع "snapshots are not available in this build of Wasm3";
#جے_انت
}

bool m3_HasSnapshot (IM3Module i_module, ثابت حرف* i_name)
{
    إذا (not i_module) {
        أرجع false;
    }
#إذا d_m3HasSnapshots
    // two of that name are still one it carries: loading it is what fails
    M3EmbeddedSnapshot* snap = عدم;
    (فراغ)ModuleSnapshot(i_module, i_name, &snap);

    أرجع snap != عدم;
#وإلا
    (فراغ)i_name;
    أرجع false;
#جے_انت
}

M3_نتیجہ m3_GetEmbeddedSnapshot (IM3Module i_module, ثابت حرف* i_name,
                                 ثابت فراغ** o_بائٹاں, size_t* o_ناپ)
{
    إذا (not i_module or not o_بائٹاں or not o_ناپ) {
        أرجع m3Err_mallocFailed;
    }

    *o_بائٹاں = عدم;
    *o_ناپ  = 0;

#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_نتیجہ            نتیجہ = ModuleSnapshot(i_module, i_name, &snap);

        إذا (نتیجہ) {
            أرجع نتیجہ;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        *o_بائٹاں = snap->انکڑے;
        *o_ناپ  = snap->ناپ;

        أرجع m3Err_none;
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#جے_انت
}

M3_نتیجہ m3_لدو_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name)
{
    إذا (not io_runtime or not i_module) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_نتیجہ            نتیجہ = ModuleSnapshot(i_module, i_name, &snap);
        M3_بفر_Reader      br;

        إذا (نتیجہ) {
            أرجع نتیجہ;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        br.بفر = snap->انکڑے;
        br.ناپ   = snap->ناپ;
        br.cursor = 0;

        أرجع لدو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#جے_انت
}


M3_نتیجہ m3_Save_Snapshot_منزل_Module (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name,
                                  فراغ** o_بائٹاں, size_t* o_ناپ)
{
#إذا d_m3HasSnapshots
    M3_نتیجہ       نتیجہ = m3Err_none;
    M3_بفر_Writer bw     = { عدم, 0, 0 };
    u8*            باہرلا    = عدم;

    حرف      secName[d_m3_Max_Snapshot_Name_لمائی + حجم("snapshot.")];
    u32       nameLen, name_Len_Leb_ناپ, payload_ناپ, payload_ناپ_Leb_ناپ;
    size_t    orig_ناپ, کل_ناپ;
    ثابت u8* drop_شروع = عدم;
    ثابت u8* drop_سرا   = عدم;
    IM3Module saved     = عدم;
    u8*       p;

    إذا (not io_runtime or not i_module or not o_بائٹاں or not o_ناپ) {
        أرجع m3Err_mallocFailed;
    }

    *o_بائٹاں = عدم;
    *o_ناپ  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_شروع or i_module->wasm_سرا <= i_module->wasm_شروع);

_   (SnapshotSectionName(i_name, secName, حجم(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->جڑ_Continuation or io_runtime->جڑ_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, بفر_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.ناپ);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.ناپ > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (لبھو_Embedded_Snapshot(i_module->wasm_شروع, i_module->wasm_سرا, secName, &drop_شروع, &drop_سرا, عدم, عدم));

    orig_ناپ           = (size_t)(i_module->wasm_سرا - i_module->wasm_شروع) - (drop_شروع ? (size_t)(drop_سرا - drop_شروع) : 0);
    nameLen            = (u32)لڑی_لمبائی(secName);
    name_Len_Leb_ناپ     = ناپ_دا_LEB_u32(nameLen);
    payload_ناپ        = name_Len_Leb_ناپ + nameLen + (u32)bw.ناپ;
    payload_ناپ_Leb_ناپ = ناپ_دا_LEB_u32(payload_ناپ);
    کل_ناپ          = orig_ناپ + 1 + payload_ناپ_Leb_ناپ + payload_ناپ;

    باہرلا = (u8*)m3_Malloc("wasm_snapshot_module", کل_ناپ);
    _throwifnull(باہرلا);

    p = باہرلا;

    إذا (drop_شروع) {
        size_t before = (size_t)(drop_شروع - i_module->wasm_شروع);
        size_t after  = (size_t)(i_module->wasm_سرا - drop_سرا);

        حافظہ_نقل(p, i_module->wasm_شروع, before);
        p += before;
        حافظہ_نقل(p, drop_سرا, after);
        p += after;
    } وإلا {
        حافظہ_نقل(p, i_module->wasm_شروع, orig_ناپ);
        p += orig_ناپ;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_ناپ);
    p    = EmitLEB_u32(p, nameLen);
    حافظہ_نقل(p, secName, nameLen);
    p += nameLen;
    حافظہ_نقل(p, bw.بفر, bw.ناپ);

    *o_بائٹاں = باہرلا;
    *o_ناپ  = کل_ناپ;
    باہرلا      = عدم;

_catch:
    m3_Free(bw.بفر);
    m3_Free(باہرلا);

    أرجع نتیجہ;
#وإلا
    (فراغ)io_runtime;
    (فراغ)i_module;
    (فراغ)i_name;
    (فراغ)o_بائٹاں;
    (فراغ)o_ناپ;
    أرجع "snapshots are not available in this build of Wasm3";
#جے_انت
}
