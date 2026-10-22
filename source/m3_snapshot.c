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
ساكن ثابت u32 c_snapshot_نسخه  = 1;

تعداد {
    d_m3SnapshotSection_Meta         = 0,
    d_m3_Snapshot_Section_حافظه       = 1,
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
#  define d_m3_Max_Snapshot_Name_درازی   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
تعداد {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_پایانیافته  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_انتها                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//عنصر_خنثی
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ساكن
M3_نتیجه ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_نتیجه نتیجه = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_شروع or i_module->wasm_انتها - i_module->wasm_شروع < 8);

    {
        بایتها_t  pos = i_module->wasm_شروع + 8;
        cbytes_t انتها = i_module->wasm_انتها;

        Xxh64_Init(&hash, 0);

        طالما (pos < انتها) {
            بایتها_t شروع     = pos;
            u8      sectionId = *pos++;
            u32     اندازه      = 0;

_           (ReadLEB_u32(&اندازه, &pos, انتها));
            _throwif(m3Err_wasmUnderrun, اندازه > (size_t)(انتها - pos));

            pos += اندازه;
            إذا (sectionId != 0) {
                Xxh64_Update(&hash, شروع, (size_t)(pos - شروع));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    أرجع نتیجه;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ساكن
ثابت M3_امن_نقطه* پیدا_کن_امن_نقطه (IM3Function i_function, pc_t i_pc, u8 i_نوع)
{
    ثابت M3SnapshotMap* map = i_function->snapshotMap;

    إذا (map) {
        لكل (u32 i = 0; i < map->رقم_امن_Points; ++i) {
            إذا (map->امن_Points[i].pc == i_pc and map->امن_Points[i].نوع == i_نوع) {
                أرجع &map->امن_Points[i];
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
ثابت M3_امن_نقطه* پیدا_کن_امن_نقطه_At (ثابت M3SnapshotMap* i_map, u32 i_wasm_جابه_جایی, u8 i_نوع, u32 i_هدف_Loop)
{
    u32 کم  = 0;
    u32 بالا = i_map->رقم_امن_Points;

    طالما (کم < بالا) {
        u32 میانه = کم + (بالا - کم) / 2;

        إذا (i_map->امن_Points[میانه].wasm_جابه_جایی < i_wasm_جابه_جایی) {
            کم = میانه + 1;
        } وإلا {
            بالا = میانه;
        }
    }

    لكل (u32 i = کم; i < i_map->رقم_امن_Points and i_map->امن_Points[i].wasm_جابه_جایی == i_wasm_جابه_جایی; ++i) {
        ثابت M3_امن_نقطه* نقطه = &i_map->امن_Points[i];

        إذا (نقطه->نوع != i_نوع or (نقطه->flags & d_m3_امن_نقطه_دم_Call)) {
            استمر;
        }
        إذا (i_نوع != safepoint_op or i_map->بلوکها[نقطه->aux].wasm_جابه_جایی == i_هدف_Loop) {
            أرجع نقطه;
        }
    }

    أرجع عدم;
}

ساكن
ثابت M3_بلوک_شروع* پیدا_کن_بلوک_At (ثابت M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_جابه_جایی)
{
    لكل (u32 i = 0; i < i_map->رقم_بلوکها; ++i) {
        إذا (i_map->بلوکها[i].opcode == i_opcode and i_map->بلوکها[i].wasm_جابه_جایی == i_wasm_جابه_جایی) {
            أرجع &i_map->بلوکها[i];
        }
    }

    أرجع عدم;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ساكن مضمن
bool Encloses_جابه_جایی (ثابت M3_بلوک_شروع* i_بلوک, u32 i_wasm_جابه_جایی)
{
    أرجع i_بلوک->wasm_جابه_جایی < i_wasm_جابه_جایی and i_wasm_جابه_جایی < i_بلوک->wasm_انتها;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ساكن مضمن
bool Handlers_زنده_At (ثابت M3SnapshotMap* i_map, ثابت M3_امن_نقطه* i_نقطه, ثابت M3_بلوک_شروع* i_try)
{
    أرجع not (i_نقطه->نوع == safepoint_op and i_try->wasm_جابه_جایی > i_map->بلوکها[i_نقطه->aux].wasm_جابه_جایی);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ساكن
M3_نتیجه CallSiteType (IM3Function i_caller, ثابت M3_امن_نقطه* i_نقطه, IM3FuncType* o_type)
{
    M3_نتیجه   نتیجه = m3Err_none;
    IM3Module  module = i_caller->module;
    بایتها_t    pos    = i_caller->wasm;
    cbytes_t   انتها    = i_caller->wasm_انتها;
    u32        اندازه   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = عدم;

_   (ReadLEB_u32(&اندازه, &pos, انتها));
    _throwif(m3Err_wasmUnderrun, i_نقطه->wasm_جابه_جایی >= (size_t)(انتها - pos));
    pos += i_نقطه->wasm_جابه_جایی;

_   (Read_opcode(&opcode, &pos, انتها));
_   (ReadLEB_u32(&index, &pos, انتها));

    إذا (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->رقم_Functions);
        *o_type = module->functions[index].funcType;
    } وإلا {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_غیرمستقیم and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->رقم_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    أرجع نتیجه;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ساكن
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 رقم_Results = Get_Func_Type_رقم_Results(i_expected);

    إذا (not i_type or Get_Func_Type_رقم_Results(i_type) != رقم_Results) {
        أرجع false;
    }

    لكل (u16 i = 0; i < رقم_Results; ++i) {
        إذا (not Is_Sub_Type_از(Get_Func_Type_نتیجه_Type(i_type, i), Get_Func_Type_نتیجه_Type(i_expected, i))) {
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
تعريفنوع بنية M3_قاب_Registers {
    m3reg_t* r0;
#  إذا d_m3HasFloat
    f64* fp0;
#  endif
} M3_قاب_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
ساكن
M3_نتیجه Continuation_پشته (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_پایه, u32* o_رقم_Slots)
{
    إذا (i_cont == i_runtime->ریشه_Continuation) {
        *o_پایه     = (m3slot_t*)i_runtime->origin_پشته;
        *o_رقم_Slots = i_runtime->رقم_پشته_Slots;
    } وإلا {
        *o_پایه     = i_cont->val_پشته;
        *o_رقم_Slots = i_cont->رقم_پشته_Slots;
    }

    أرجع (*o_پایه) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ساكن
M3_نتیجه Locate_مقدار (m3slot_t* i_پایه, u32 i_رقم_Slots, m3stack_t i_sp, ثابت M3_جایگاه_مقدار* i_مقدار, u8** o_where)
{
    M3_نتیجه نتیجه = m3Err_none;
    u8       type   = پایه_Type_از(i_مقدار->type);

    *o_where = عدم;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_ثبات_جایگاه_Alias(i_مقدار->جایگاه));

    إذا (not Is_ثبات_جایگاه_Alias(i_مقدار->جایگاه)) {
        size_t اندازه   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? حجم(فراغ*)
                                                                   : (Is64_بیت_Type(type) ? حجم(u64) : حجم(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_پایه + i_مقدار->جایگاه) * حجم(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + اندازه > (size_t)i_رقم_Slots * حجم(m3slot_t));

        *o_where = (u8*)i_پایه + offset;
    }

_catch:
    أرجع نتیجه;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ساكن
M3_نتیجه Read_مقدار (u8* i_where, ثابت M3_قاب_Registers* i_registers, ثابت M3_جایگاه_مقدار* i_مقدار, u64* o_بیتها,
                    فراغ** o_reference)
{
    M3_نتیجه نتیجه = m3Err_none;
    u8       type   = پایه_Type_از(i_مقدار->type);

    *o_بیتها      = 0;
    *o_reference = عدم;

    إذا (i_where) {
        إذا (IsRefType(type)) {
            کپی_حافظه(o_reference, i_where, حجم(فراغ*));
        } وإلا إذا (Is64_بیت_Type(type)) {
            کپی_حافظه(o_بیتها, i_where, حجم(u64));
        } وإلا {
            u32 narrow;
            کپی_حافظه(&narrow, i_where, حجم(narrow));
            *o_بیتها = narrow;
        }
    } وإلا إذا (Is_Fp_ثبات_جایگاه_Alias(i_مقدار->جایگاه)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 بیتها;
            کپی_حافظه(&بیتها, &narrow, حجم(بیتها));
            *o_بیتها = بیتها;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            کپی_حافظه(o_بیتها, i_registers->fp0, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        إذا (IsRefType(type)) {
            *o_reference = (فراغ*)(uintptr_t)r0;
        } وإلا إذا (Is64_بیت_Type(type)) {
            *o_بیتها = (u64)r0;
        } وإلا {
            *o_بیتها = (u32)r0;
        }
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه Write_مقدار (u8* o_where, ثابت M3_قاب_Registers* i_registers, ثابت M3_جایگاه_مقدار* i_مقدار, u64 i_بیتها,
                     فراغ* i_reference)
{
    M3_نتیجه نتیجه = m3Err_none;
    u8       type   = پایه_Type_از(i_مقدار->type);

    إذا (o_where) {
        إذا (IsRefType(type)) {
            کپی_حافظه(o_where, &i_reference, حجم(فراغ*));
        } وإلا إذا (Is64_بیت_Type(type)) {
            کپی_حافظه(o_where, &i_بیتها, حجم(u64));
        } وإلا {
            u32 narrow = (u32)i_بیتها;
            کپی_حافظه(o_where, &narrow, حجم(narrow));
        }
    } وإلا إذا (Is_Fp_ثبات_جایگاه_Alias(i_مقدار->جایگاه)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            u32 بیتها = (u32)i_بیتها;
            f32 narrow;
            کپی_حافظه(&narrow, &بیتها, حجم(narrow));
            *i_registers->fp0 = narrow;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            کپی_حافظه(i_registers->fp0, &i_بیتها, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        إذا (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } وإلا {
            *i_registers->r0 = (m3reg_t)i_بیتها;
        }
    }

_catch:
    أرجع نتیجه;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ساكن
ثابت M3_جایگاه_مقدار* قاب_مقدار (ثابت M3SnapshotMap* i_map, ثابت M3_امن_نقطه* i_نقطه, u32 i_شاخص)
{
    إذا (i_شاخص < i_map->رقم_Locals) {
        أرجع i_نقطه->اولی_Local_مقدار == UINT32_MAX ? &i_map->locals[i_شاخص]
                                                      : &i_map->مقادیر[i_نقطه->اولی_Local_مقدار + i_شاخص];
    }
    أرجع &i_map->مقادیر[i_نقطه->اولی_مقدار + i_شاخص - i_map->رقم_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ساكن
فراغ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    إذا (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->رقم_Ret_And_Arg_Slots) + i_function->رقم_Local_بایتها;

        کپی_حافظه(where, i_function->constants, i_function->رقم_Constant_بایتها);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
تعريفنوع بنية M3_اشاره_گر_Ids {
    فراغ** اقلام;
    u32    شمار;
    u32    ظرفیت;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  شاخص_ظرفیت;       // a power of two, kept at least twice count
} M3_اشاره_گر_Ids;

ساكن
u32 اشاره_گر_سطل (فراغ* i_اشاره_گر, u32 i_ظرفیت)
{
    u64 بیتها = (u64)(uintptr_t)i_اشاره_گر;

    أرجع (u32)((بیتها * 0x9E3779B97F4A7C15ULL) >> 32) & (i_ظرفیت - 1);
}

ساكن
u32 اشاره_گر_Ids_پیدا_کن (ثابت M3_اشاره_گر_Ids* i_ids, فراغ* i_اشاره_گر)
{
    إذا (i_ids->شاخص_ظرفیت) {
        u32 سطل = اشاره_گر_سطل(i_اشاره_گر, i_ids->شاخص_ظرفیت);

        طالما (i_ids->index[سطل]) {
            u32 id = i_ids->index[سطل] - 1;

            إذا (i_ids->اقلام[id] == i_اشاره_گر) {
                أرجع id;
            }

            سطل = (سطل + 1) & (i_ids->شاخص_ظرفیت - 1);
        }
    }

    أرجع d_m3SnapshotNone;
}

ساكن
M3_نتیجه اشاره_گر_Ids_جمع_کن (M3_اشاره_گر_Ids* io_ids, فراغ* i_اشاره_گر, u32* o_id)
{
    u32 id = اشاره_گر_Ids_پیدا_کن(io_ids, i_اشاره_گر);

    إذا (id != d_m3SnapshotNone) {
        *o_id = id;
        أرجع m3Err_none;
    }

    إذا (io_ids->شمار == io_ids->ظرفیت) {
        u32    ظرفیت = io_ids->ظرفیت ? io_ids->ظرفیت * 2 : 16;
        فراغ** اقلام    = m3_ReallocArray(فراغ*, io_ids->اقلام, ظرفیت, io_ids->ظرفیت);

        إذا (not اقلام) {
            أرجع m3Err_mallocFailed;
        }

        io_ids->اقلام    = اقلام;
        io_ids->ظرفیت = ظرفیت;
    }

    إذا ((io_ids->شمار + 1) * 2 > io_ids->شاخص_ظرفیت) {
        u32  ظرفیت = io_ids->شاخص_ظرفیت ? io_ids->شاخص_ظرفیت * 2 : 32;
        u32* index    = m3_AllocArray(u32, ظرفیت);

        إذا (not index) {
            أرجع m3Err_mallocFailed;
        }

        لكل (u32 i = 0; i < io_ids->شمار; ++i) {
            u32 سطل = اشاره_گر_سطل(io_ids->اقلام[i], ظرفیت);

            طالما (index[سطل]) {
                سطل = (سطل + 1) & (ظرفیت - 1);
            }

            index[سطل] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->شاخص_ظرفیت = ظرفیت;
    }

    id                = io_ids->شمار++;
    io_ids->اقلام[id] = i_اشاره_گر;

    u32 سطل = اشاره_گر_سطل(i_اشاره_گر, io_ids->شاخص_ظرفیت);

    طالما (io_ids->index[سطل]) {
        سطل = (سطل + 1) & (io_ids->شاخص_ظرفیت - 1);
    }

    io_ids->index[سطل] = id + 1;

    *o_id = id;

    أرجع m3Err_none;
}

ساكن
فراغ اشاره_گر_Ids_Free (M3_اشاره_گر_Ids* io_ids)
{
    m3_Free(io_ids->اقلام);
    m3_Free(io_ids->index);
    memset(io_ids, 0, حجم(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3_میانگیر_Writer {
    u8*    میانگیر;
    size_t اندازه;
    size_t ظرفیت;
} M3_میانگیر_Writer;

ساكن
M3_نتیجه میانگیر_Writer_Write (ثابت فراغ* i_داده, size_t i_اندازه, فراغ* i_userdata)
{
    M3_میانگیر_Writer* bw = (M3_میانگیر_Writer*)i_userdata;

    إذا (bw->اندازه + i_اندازه > bw->ظرفیت) {
        size_t newCap = bw->ظرفیت ? bw->ظرفیت * 2 : 4096;
        طالما (bw->اندازه + i_اندازه > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->میانگیر, newCap, bw->ظرفیت);
        إذا (!newBuf) {
            أرجع m3Err_mallocFailed;
        }
        bw->میانگیر   = newBuf;
        bw->ظرفیت = newCap;
    }

    إذا (i_اندازه) {
        کپی_حافظه(bw->میانگیر + bw->اندازه, i_داده, i_اندازه);
        bw->اندازه += i_اندازه;
    }

    أرجع m3Err_none;
}

تعريفنوع بنية M3_میانگیر_Reader {
    ثابت u8* میانگیر;
    size_t    اندازه;
    size_t    cursor;
} M3_میانگیر_Reader;

ساكن
M3_نتیجه میانگیر_Reader_Read (فراغ* o_میانگیر, size_t i_اندازه, فراغ* i_userdata)
{
    M3_میانگیر_Reader* br = (M3_میانگیر_Reader*)i_userdata;

    إذا (i_اندازه > br->اندازه - br->cursor) {
        أرجع m3Err_wasmMalformed;
    }

    إذا (i_اندازه) {
        کپی_حافظه(o_میانگیر, br->میانگیر + br->cursor, i_اندازه);
        br->cursor += i_اندازه;
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

    M3_اشاره_گر_Ids زنده_Continuations;     // everything the runtime still holds, to check
    M3_اشاره_گر_Ids زنده_Exceptions;        //   a reference against before following it
    M3_اشاره_گر_Ids continuations;         // what the snapshot carries, by id
    M3_اشاره_گر_Ids exceptions;
} M3SnapshotSave;

ساكن مضمن
u8 M3_Type_مقصد_Val_Type (m3type_t i_type)
{
    بدل (پایه_Type_از(i_type)) {
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
m3type_t Val_Type_مقصد_M3_Type (u8 i_valtype)
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
M3_نتیجه Put (M3SnapshotSave* s, ثابت فراغ* i_داده, size_t i_اندازه)
{
    أرجع s->writing ? s->writer(i_داده, i_اندازه, s->userdata) : m3Err_none;
}

ساكن
M3_نتیجه PutU8 (M3SnapshotSave* s, u8 i_مقدار)
{
    أرجع Put(s, &i_مقدار, حجم(i_مقدار));
}


ساكن
M3_نتیجه PutLEB_u32 (M3SnapshotSave* s, u32 i_مقدار)
{
    u8  بایتها[5];
    u32 len = 0;

    افعل {
        u8 byte = i_مقدار & 0x7F;
        i_مقدار >>= 7;
        إذا (i_مقدار != 0) {
            byte |= 0x80;
        }
        بایتها[len++] = byte;
    } طالما (i_مقدار != 0);

    أرجع Put(s, بایتها, len);
}

ساكن
M3_نتیجه PutLEB_u64 (M3SnapshotSave* s, u64 i_مقدار)
{
    u8  بایتها[10];
    u32 len = 0;

    افعل {
        u8 byte = i_مقدار & 0x7F;
        i_مقدار >>= 7;
        إذا (i_مقدار != 0) {
            byte |= 0x80;
        }
        بایتها[len++] = byte;
    } طالما (i_مقدار != 0);

    أرجع Put(s, بایتها, len);
}

ساكن
M3_نتیجه PutLEB_i64 (M3SnapshotSave* s, i64 i_مقدار)
{
    u8   بایتها[10];
    u32  len  = 0;
    bool بیشتر = true;

    طالما (بیشتر) {
        u8 byte = i_مقدار & 0x7F;
        i_مقدار >>= 7;
        إذا ((i_مقدار == 0 && !(byte & 0x40)) || (i_مقدار == -1 && (byte & 0x40))) {
            بیشتر = false;
        } وإلا {
            byte |= 0x80;
        }
        بایتها[len++] = byte;
    }

    أرجع Put(s, بایتها, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
تعريفنوع M3_نتیجه (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ساكن
M3_نتیجه SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_نتیجه         نتیجه   = m3Err_none;
    M3_میانگیر_Writer   bw       = { عدم, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    فراغ*            userdata = s->userdata;

    s->writer   = میانگیر_Writer_Write;
    s->userdata = &bw;

    نتیجه = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    إذا (نتیجه) {
        اذهب _catch;
    }

    // a section with nothing in it is left out
    إذا (bw.اندازه) {
        _throwif("snapshot section is too large", bw.اندازه > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.اندازه));
_       (Put(s, bw.میانگیر, bw.اندازه));
    }

_catch:
    m3_Free(bw.میانگیر);

    أرجع نتیجه;
}

ساكن
u32 Function_شاخص (IM3Module i_module, IM3Function i_function)
{
    إذا (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->رقم_Functions) {
        أرجع (u32)(i_function - i_module->functions);
    }

    أرجع d_m3SnapshotNone;
}

ساكن
u32 Func_Type_شاخص (IM3Module i_module, IM3FuncType i_type)
{
    لكل (u32 i = 0; i < i_module->رقم_Func_Types; ++i) {
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
IM3_برچسب برچسب_از_شاخص (IM3Module i_module, u32 i_شاخص)
{
    IM3_برچسب برچسب = &i_module->برچسبها[i_شاخص];

    أرجع برچسب->resolved ? برچسب->resolved : برچسب;
}

ساكن
u32 برچسب_شاخص_2 (IM3Module i_module, IM3_برچسب i_برچسب)
{
    لكل (u32 i = 0; i < i_module->رقم_برچسبها; ++i) {
        إذا (برچسب_از_شاخص(i_module, i) == i_برچسب) {
            أرجع i;
        }
    }

    أرجع d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ساكن
M3_نتیجه NameReference (M3SnapshotSave* s, u8 i_type, فراغ* i_reference, u64* o_کلمه)
{
    M3_نتیجه نتیجه = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_کلمه = d_m3SnapshotNullRef;

    إذا (not i_reference) {
        أرجع m3Err_none;
    }

    إذا (i_type == c_m3Type_funcref) {
        id = Function_شاخص(s->module, (IM3Function)i_reference);
        _throwif("a funcref names a function of another module", id == d_m3SnapshotNone);
    } وإلا إذا (i_type == c_m3Type_externref) {
        ثابت M3SnapshotHooks* hooks = &s->runtime->snapshotHooks;

        _throwif("an externref belongs to the host, and cannot be saved", not hooks->nameExternRef);

        u64 name = d_m3SnapshotNullRef;
_       (hooks->nameExternRef(hooks->userdata, i_reference, &name));
        _throwif("the host named an externref as null", name == d_m3SnapshotNullRef);

        *o_کلمه = name;
        اذهب _catch;
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (i_type == c_m3Type_exnref) {
        _throwif("an exnref names an exception that no longer exists",
                 اشاره_گر_Ids_پیدا_کن(&s->زنده_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 شمار = s->exceptions.شمار;
_       (اشاره_گر_Ids_جمع_کن(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.شمار != شمار);
    }
#  endif
    وإلا إذا (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 اشاره_گر_Ids_پیدا_کن(&s->زنده_Continuations, i_reference) == d_m3SnapshotNone);

        u32 شمار = s->continuations.شمار;
_       (اشاره_گر_Ids_جمع_کن(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.شمار != شمار);
    } وإلا {
        _throw("a reference of an unknown type");
    }

    *o_کلمه = id;

_catch:
    أرجع نتیجه;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ساكن
M3_نتیجه Put_Ref_مقدار (M3SnapshotSave* s, u8 i_type, فراغ* i_reference)
{
    M3_نتیجه نتیجه = m3Err_none;
    u64      کلمه   = 0;

_   (NameReference(s, i_type, i_reference, &کلمه));

    إذا (not s->writing) {
        أرجع m3Err_none;
    }

    إذا (کلمه == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } وإلا {
        u8 نوع = 0;
        إذا (i_type == c_m3Type_funcref) {
            نوع = 0x01;
        } وإلا إذا (i_type == c_m3Type_externref) {
            نوع = 0x02;
        }
#  إذا d_m3HasExceptionHandling
        وإلا إذا (i_type == c_m3Type_exnref) {
            نوع = 0x03;
        }
#  endif
        وإلا إذا (i_type == c_m3Type_contref) {
            نوع = 0x04;
        }

_       (PutU8(s, نوع));
_       (PutLEB_u64(s, کلمه));
    }

_catch:
    أرجع نتیجه;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ساكن
M3_نتیجه PutLE32 (M3SnapshotSave* s, u32 i_مقدار)
{
    u8 بایتها[4];

    لكل (u32 i = 0; i < حجم(بایتها); ++i) {
        بایتها[i] = (u8)(i_مقدار >> (8 * i));
    }

    أرجع Put(s, بایتها, حجم(بایتها));
}

ساكن
M3_نتیجه PutLE64 (M3SnapshotSave* s, u64 i_مقدار)
{
    u8 بایتها[8];

    لكل (u32 i = 0; i < حجم(بایتها); ++i) {
        بایتها[i] = (u8)(i_مقدار >> (8 * i));
    }

    أرجع Put(s, بایتها, حجم(بایتها));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ساكن
M3_نتیجه Put_مقدار_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_بیتها)
{
    إذا (IsRefType(i_type)) {
        أرجع Put_Ref_مقدار(s, پایه_Type_از(i_type), (فراغ*)(uintptr_t)i_بیتها);
    }

    بدل (پایه_Type_از(i_type)) {
    حالة c_m3Type_i32: أرجع PutLEB_i64(s, (i32)(u32)i_بیتها);
    حالة c_m3Type_i64: أرجع PutLEB_i64(s, (i64)i_بیتها);
    حالة c_m3Type_f32: أرجع PutLE32(s, (u32)i_بیتها);
    حالة c_m3Type_f64: أرجع PutLE64(s, i_بیتها);
    افتراضي: أرجع "a snapshot cannot hold a value of this type as bits";
    }
}

ساكن
M3_نتیجه Put_مقدار_برچسب (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_مقصد_Val_Type(i_type);

    إذا (not valtype) {
        أرجع "a snapshot has no tag for a value of this type";
    }

    أرجع PutU8(s, valtype);
}

ساكن
M3_نتیجه Put_مقدار_بیتها (M3SnapshotSave* s, m3type_t i_type, u64 i_بیتها)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (Put_مقدار_برچسب(s, i_type));
_   (Put_مقدار_Body(s, i_type, i_بیتها));

_catch:
    أرجع نتیجه;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ساكن
M3_نتیجه Put_مقدار (M3SnapshotSave* s, m3type_t i_type, ثابت فراغ* i_where)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (Put_مقدار_برچسب(s, i_type));

    إذا (IsRefType(i_type)) {
        فراغ* reference;
        کپی_حافظه(&reference, i_where, حجم(reference));
_       (Put_مقدار_Body(s, i_type, (u64)(uintptr_t)reference));
    } وإلا إذا (پایه_Type_از(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 بایتها[16];
        کپی_حافظه(بایتها, i_where, حجم(بایتها));
_       (Put(s, بایتها, حجم(بایتها)));
    } وإلا إذا (Is64_بیت_Type(i_type)) {
        u64 بیتها;
        کپی_حافظه(&بیتها, i_where, حجم(بیتها));
_       (Put_مقدار_Body(s, i_type, بیتها));
    } وإلا {
        u32 narrow;
        کپی_حافظه(&narrow, i_where, حجم(narrow));
_       (Put_مقدار_Body(s, i_type, narrow));
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه Put_حافظه_Chunks (M3SnapshotSave* s, ثابت u8* i_بایتها, size_t i_اندازه)
{
    M3_نتیجه نتیجه = m3Err_none;
    size_t   cursor = 0;

    طالما (cursor < i_اندازه) {
        u8     byte = i_بایتها[cursor];
        size_t پیاپی  = 1;

        طالما (cursor + پیاپی < i_اندازه and i_بایتها[cursor + پیاپی] == byte) {
            پیاپی++;
        }

        إذا ((byte == 0x00 or byte == 0xFF) and پیاپی >= d_m3_Snapshot_پیاپی_آستانه) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            إذا (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)پیاپی));
            }

            cursor += پیاپی;
            استمر;
        }

        size_t raw_شروع = cursor;

        طالما (cursor < i_اندازه) {
            u8     b      = i_بایتها[cursor];
            size_t درازی = 1;

            طالما (cursor + درازی < i_اندازه and i_بایتها[cursor + درازی] == b) {
                درازی++;
            }

            إذا ((b == 0x00 or b == 0xFF) and درازی >= d_m3_Snapshot_پیاپی_آستانه) {
                اكسر;
            }

            cursor += درازی;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_شروع));
_       (PutLEB_u32(s, (u32)(cursor - raw_شروع)));
_       (Put(s, i_بایتها + raw_شروع, cursor - raw_شروع));
    }

_   (PutU8(s, d_m3_Chunk_انتها));

_catch:
    أرجع نتیجه;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ساكن
u32 حافظه_اولی_شاخص (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->memories[j] != module->memories[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
u32 جدول_اولی_شاخص (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->tables[j] != module->tables[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
M3_نتیجه SaveMemories (M3SnapshotSave* s)
{
    M3_نتیجه  نتیجه = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_Memories));

    لكل (u32 m = 0; m < module->رقم_Memories; ++m) {
        IM3_حافظه حافظه   = module->memories[m];
        u32       اولی    = حافظه_اولی_شاخص(module, m);
        bool      has_داده  = حافظه and حافظه->mallocated;
        u32       صفحه_اندازه = حافظه ? حافظه_صفحه_اندازه(حافظه) : d_m3_Default_Mem_صفحه_اندازه;
        u64       بایتها    = حافظه ? حافظه->رقم_صفحات * (u64)صفحه_اندازه : 0;
        u32       صفحه_بیتها = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, اولی));

        إذا (اولی != m) {
            استمر;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", حافظه and حافظه->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", بایتها > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 حافظه and حافظه->mallocated and حافظه->mallocated->درازی < بایتها);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        طالما ((1u << صفحه_بیتها) < صفحه_اندازه) {
            ++صفحه_بیتها;
        }

_       (PutLEB_u32(s, صفحه_بیتها));
_       (PutLEB_u32(s, حافظه ? (u32)حافظه->رقم_صفحات : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        إذا (not s->writing) {
            استمر;
        }

        إذا (has_داده) {
_           (Put_حافظه_Chunks(s, m3_Mem_داده(حافظه->mallocated), (size_t)بایتها));
        } وإلا {
_           (PutU8(s, d_m3_Chunk_انتها));
        }
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه SaveGlobals (M3SnapshotSave* s)
{
    M3_نتیجه  نتیجه = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_Globals));

    لكل (u32 g = 0; g < module->رقم_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_مقدار(s, global->type, &cell->i64_مقدار));
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه SaveTables (M3SnapshotSave* s)
{
    M3_نتیجه  نتیجه = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_Tables));

    لكل (u32 t = 0; t < module->رقم_Tables; ++t) {
        IM3_جدول جدول_2 = module->tables[t];
        u32      اولی = جدول_اولی_شاخص(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, اولی));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        إذا (اولی != t) {
            استمر;
        }

_       (PutU8(s, M3_Type_مقصد_Val_Type(جدول_2->type)));
_       (PutLEB_u32(s, جدول_2->اندازه));

        لكل (u32 e = 0; e < جدول_2->اندازه; ++e) {
_           (Put_Ref_مقدار(s, پایه_Type_از(جدول_2->type), جدول_2->elements[e]));
        }
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه SaveSegments (M3SnapshotSave* s)
{
    M3_نتیجه  نتیجه = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->رقم_داده_Segments));

    لكل (u32 i = 0; i < module->رقم_داده_Segments; ++i) {
_       (PutU8(s, module->داده_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->رقم_Element_Segments));

    لكل (u32 i = 0; i < module->رقم_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    أرجع نتیجه;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_نتیجه SaveExceptions (M3SnapshotSave* s)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.شمار));

    لكل (u32 i = 0; i < s->exceptions.شمار; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.اقلام[i];
        IM3FuncType  type = exn->برچسب->type;

        _throwif(m3Err_wasmMalformed, exn->رقم_آرگومانها != Get_Func_Type_رقم_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->رقم_آرگومانها));

        لكل (u32 a = 0; a < exn->رقم_آرگومانها; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_مقدار_بیتها(s, argType, exn->آرگومانها[a]));
        }
    }

_catch:
    أرجع نتیجه;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
تعريفنوع بنية M3Activation {
    IM3Function        function;
    ثابت M3_امن_نقطه* نقطه;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 نوع;            // M3SafePointKind
    M3_قاب_Registers   registers;       // the ones it goes on with
    ثابت M3_قاب*     resume;          // the resume it waits in, for safepoint_resume
    i32                اولی_قاب;      // its frames, outermost first: firstFrame down to lastFrame
    i32                آخرین_قاب;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ساكن
ثابت M3_امن_نقطه* پیدا_کن_قاب_امن_نقطه (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_نوع)
{
    ثابت M3_امن_نقطه* نقطه = *io_function ? پیدا_کن_امن_نقطه(*io_function, i_pc, i_نوع) : عدم;

    لكل (u32 i = 0; not نقطه and i < i_module->رقم_Functions; ++i) {
        نقطه = پیدا_کن_امن_نقطه(&i_module->functions[i], i_pc, i_نوع);
        إذا (نقطه) {
            *io_function = &i_module->functions[i];
        }
    }

    أرجع نقطه;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ساكن
M3_نتیجه بعدی_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_اولی, M3Activation* o_act)
{
    M3_نتیجه نتیجه = m3Err_none;
    i32      i      = i_اولی;

    memset(o_act, 0, حجم(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->اولی_قاب = i_اولی;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    لكل (; i >= 0; --i) {
        ثابت M3_قاب* قاب = &i_cont->قابها[i];

        _throwif("a suspended frame runs against another module's memory", قاب->حافظه != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", قاب->sp != i_sp);

        إذا (قاب->نوع == قاب_call) {
            o_act->pc           = قاب->pc;
            o_act->نوع         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&قاب->call.r0;
#  إذا d_m3HasFloat
            o_act->registers.fp0 = (f64*)&قاب->call.fp0;
#  endif
            اكسر;
        } وإلا إذا (قاب->نوع == قاب_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = قاب->pc;
            o_act->نوع   = safepoint_resume;
            o_act->resume = قاب;
            اكسر;
        }
    }

    إذا (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->نوع         = i_cont->suspend_نقطه;
        o_act->registers.r0 = &i_cont->r0;
#  إذا d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->آخرین_قاب = i;
    o_act->نقطه     = پیدا_کن_قاب_امن_نقطه(i_module, &o_act->function, o_act->pc, o_act->نوع);

    _throwif("a suspended frame is not at a safepoint", not o_act->نقطه);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  إذا d_m3_مدخل_Keeps_قاب
    لكل (i32 k = i_اولی; k > o_act->آخرین_قاب; --k) {
        ثابت M3_قاب* قاب = &i_cont->قابها[k];
        _throwif("a suspended frame is not the function it is entered with",
                 قاب->نوع == قاب_مدخل and قاب->مدخل.function != o_act->function);
    }
#  endif

_catch:
    أرجع نتیجه;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ساكن
M3_نتیجه SaveActivation (M3SnapshotSave* s, m3slot_t* i_پایه, u32 i_رقم_Slots, ثابت M3Activation* i_act)
{
    M3_نتیجه             نتیجه        = m3Err_none;
    IM3Function          function      = i_act->function;
    ثابت M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_شاخص = Function_شاخص(s->module, function);
    ثابت M3_امن_نقطه*   نقطه         = i_act->نقطه;
    u32                  رقم_مقادیر;

    _throwif("a suspended frame belongs to another module", function_شاخص == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_شاخص));
_   (PutU8(s, نقطه->نوع));
_   (PutLEB_u32(s, نقطه->wasm_جابه_جایی));

    إذا (نقطه->نوع == safepoint_op) {
_       (PutLEB_u32(s, map->بلوکها[نقطه->aux].wasm_جابه_جایی));
    }

    رقم_مقادیر = map->رقم_Locals + نقطه->رقم_مقادیر;

_   (PutLEB_u32(s, رقم_مقادیر));

    لكل (u32 v = 0; v < رقم_مقادیر; ++v) {
        ثابت M3_جایگاه_مقدار* مقدار = قاب_مقدار(map, نقطه, v);
        u8*                where;
        u64                کلمه;
        فراغ*              reference;

_       (Locate_مقدار(i_پایه, i_رقم_Slots, i_act->sp, مقدار, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (پایه_Type_از(مقدار->type) == c_m3Type_v128) {
_           (Put_مقدار_برچسب(s, مقدار->type));
_           (Put(s, where, 16));
            استمر;
        }

_       (Read_مقدار(where, &i_act->registers, مقدار, &کلمه, &reference));

_       (Put_مقدار_بیتها(s, مقدار->type, IsRefType(مقدار->type) ? (u64)(uintptr_t)reference : کلمه));
    }

    إذا (نقطه->نوع == safepoint_resume) {
_       (Put_Ref_مقدار(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    أرجع نتیجه;
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
M3_نتیجه SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_نتیجه      نتیجه   = m3Err_none;
    M3Activation* acts     = عدم;
    u32           رقم_Acts  = 0;
    IM3Function   function = i_cont->مدخل_Function;
    i32           اولی    = (i32)i_cont->رقم_قابها - 1;
    m3slot_t*     پایه;
    u32           رقم_Slots;
    m3stack_t     sp;

_   (Continuation_پشته(s->runtime, i_cont, &پایه, &رقم_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->رقم_قابها + 1);
    _throwifnull(acts);

    sp = پایه;

    طالما (true) {
        M3Activation* act = &acts[رقم_Acts];

_       (بعدی_Activation(s->module, i_cont, function, sp, اولی, act));

        إذا (act->نوع != safepoint_call) {
            رقم_Acts++;
            اكسر;
        }

        _throwif("a suspended continuation stops at a call", act->آخرین_قاب < 0);

        إذا (not (act->نقطه->flags & d_m3_امن_نقطه_دم_Call)) {
            رقم_Acts++;
        }

        function = i_cont->قابها[act->آخرین_قاب].call.function;
        sp       = act->sp + act->نقطه->aux;
        اولی    = act->آخرین_قاب - 1;
    }

_   (PutLEB_u32(s, رقم_Acts));

    لكل (u32 a = 0; a < رقم_Acts; ++a) {
_       (SaveActivation(s, پایه, رقم_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    أرجع نتیجه;
}

ساكن
M3_نتیجه SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_نتیجه  نتیجه    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_ریشه    = (i_cont == s->runtime->ریشه_Continuation);
    u8        state     = snapshot_cont_پایانیافته;
    u32       type_شاخص = is_ریشه ? d_m3SnapshotNone : Func_Type_شاخص(module, i_cont->type);

    إذا (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } وإلا إذا (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } وإلا إذا (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->مدخل_Function);
    _throwif("a continuation belongs to another module", Function_شاخص(module, i_cont->مدخل_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_ریشه and type_شاخص == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_ریشه));
_   (PutLEB_u32(s, type_شاخص));
_   (PutLEB_u32(s, Function_شاخص(module, i_cont->مدخل_Function)));
#  إذا d_m3HasExceptionHandling
_   (Put_Ref_مقدار(s, c_m3Type_exnref, i_cont->resumeThrow));
#  وإلا
_   (Put_Ref_مقدار(s, c_m3Type_exnref, عدم));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    إذا (s->postmortem) {
        اذهب _catch;
    }

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->مدخل_Function ? i_cont->مدخل_Function->funcType : عدم;
        u16         رقم_Rets = inner ? Get_Func_Type_رقم_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_آرگومانها_شمار > Get_Func_Type_رقم_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_آرگومانها_شمار));

        لكل (u32 i = 0; i < i_cont->bound_آرگومانها_شمار; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_مقدار(s, type, i_cont->val_پشته + (رقم_Rets + i) * c_io_جایگاه_شمار));
        }
    } وإلا إذا (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_آرگومانها_شمار));
_       (SaveSuspendedBody(s, i_cont));
    } وإلا {
_       (PutLEB_u32(s, 0));
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه SaveContinuations (M3SnapshotSave* s)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (PutLEB_u32(s, s->continuations.شمار));

    لكل (u32 i = 0; i < s->continuations.شمار; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.اقلام[i]));
    }

_catch:
    أرجع نتیجه;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ساكن
M3_نتیجه SaveHostState (M3SnapshotSave* s)
{
    M3_نتیجه               نتیجه = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_میانگیر_Writer         bw     = { عدم, 0, 0 };

    إذا (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, میانگیر_Writer_Write, &bw));
    }

    إذا (bw.اندازه > 0) {
_       (Put(s, bw.میانگیر, bw.اندازه));
    }

_catch:
    m3_Free(bw.میانگیر);

    أرجع نتیجه;
}

ساكن
فراغ SnapshotResourceTotals (IM3Module module, u64* حافظه_بایتها, u64* جدول_Elements)
{
    *حافظه_بایتها = *جدول_Elements = 0;
    لكل (u32 i = 0; i < module->رقم_Memories; ++i) {
        IM3_حافظه حافظه = module->memories[i];
        إذا (حافظه_اولی_شاخص(module, i) == i and حافظه) {
            *حافظه_بایتها += حافظه->رقم_صفحات * (u64)حافظه_صفحه_اندازه(حافظه);
        }
    }
    لكل (u32 i = 0; i < module->رقم_Tables; ++i) {
        IM3_جدول جدول_2 = module->tables[i];
        إذا (جدول_اولی_شاخص(module, i) == i and جدول_2) {
            *جدول_Elements += جدول_2->اندازه;
        }
    }
}

ساكن
M3_نتیجه SaveMeta (M3SnapshotSave* s)
{
    M3_نتیجه نتیجه = m3Err_none;
    u64      hash   = 0;
    u64      حافظه_بایتها, جدول_Elements;
    u32      فعال_Stacks = 0;
    لكل (u32 i = 0; i < s->continuations.شمار; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.اقلام[i];
        إذا (cont != s->runtime->ریشه_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            فعال_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &حافظه_بایتها, &جدول_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_زمان_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.شمار));
_   (PutLEB_u32(s, s->exceptions.شمار));
_   (PutLEB_u64(s, حافظه_بایتها));
_   (PutLEB_u64(s, جدول_Elements));
_   (PutLEB_u32(s, فعال_Stacks));

#  إذا d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    لكل (u32 i = 0; i < s->exceptions.شمار; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.اقلام[i];
        u32          برچسب_شاخص  = برچسب_شاخص_2(s->module, exception->برچسب);

        _throwif("an exception was thrown with another module's tag", برچسب_شاخص == d_m3SnapshotNone);

_       (PutLEB_u32(s, برچسب_شاخص));
    }
#  endif

_catch:
    أرجع نتیجه;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ساكن
M3_نتیجه DiscoverReferences (M3SnapshotSave* s)
{
    M3_نتیجه نتیجه            = m3Err_none;
    u32      انجامشده_Continuations = 0;
    u32      انجامشده_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    طالما (انجامشده_Continuations < s->continuations.شمار or انجامشده_Exceptions < s->exceptions.شمار) {
#  إذا d_m3HasExceptionHandling
        طالما (انجامشده_Exceptions < s->exceptions.شمار) {
            M3Exception* exception = (M3Exception*)s->exceptions.اقلام[انجامشده_Exceptions++];
            IM3FuncType  type      = exception->برچسب->type;

            _throwif(m3Err_wasmMalformed, exception->رقم_آرگومانها != Get_Func_Type_رقم_Params(type));

            لكل (u32 a = 0; a < exception->رقم_آرگومانها; ++a) {
_               (Put_مقدار_بیتها(s, GetFuncTypeParamType(type, (u16)a), exception->آرگومانها[a]));
            }
        }
#  endif
        طالما (انجامشده_Continuations < s->continuations.شمار and انجامشده_Exceptions == s->exceptions.شمار) {
            u32 id = انجامشده_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.اقلام[id]));
        }
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه SaveSections (M3SnapshotSave* s)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (not s->writing) {
        أرجع DiscoverReferences(s);
    }

    إذا (not s->embedded) {
_       (Put(s, c_snapshotMagic, حجم(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_نسخه));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    إذا (s->module->رقم_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_حافظه, SaveMemories));
    }
    إذا (s->module->رقم_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_جدول, SaveTables));
    }
    إذا (s->module->رقم_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    إذا (s->module->رقم_داده_Segments or s->module->رقم_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  إذا d_m3HasExceptionHandling
    إذا (s->exceptions.شمار) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    إذا (s->continuations.شمار) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    أرجع نتیجه;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3SnapshotSuspendSite {
    ثابت M3SnapshotMap* map;
    ثابت M3_امن_نقطه*   نقطه;
    u32                  هدف;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

تعريفنوع بنية M3_Snapshot_بارگیری {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    فراغ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_پشته;
    bool      تغییریافته;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    رقم_Continuations;
    u64                    required_حافظه_بایتها;
    u64                    required_جدول_Elements;
    u64                    loaded_حافظه_بایتها;
    u64                    loaded_جدول_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  إذا d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 رقم_Exceptions;

    M3_قاب* قابها;           // a suspended continuation's frames as they are put together, outermost first

    بایتها_t pos;               // bounded section payload, read using the Wasm parser's helpers
    بایتها_t انتها;
    bool    درون_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    دیده_Sections[256];
} M3_Snapshot_بارگیری;

ساكن
M3_نتیجه Get (M3_Snapshot_بارگیری* l, فراغ* o_داده, size_t i_اندازه)
{
    إذا (not l->درون_Section) {
        أرجع l->reader(o_داده, i_اندازه, l->userdata);
    }
    إذا (i_اندازه > (size_t)(l->انتها - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    إذا (i_اندازه) {
        کپی_حافظه(o_داده, l->pos, i_اندازه);
        l->pos += i_اندازه;
    }
    أرجع m3Err_none;
}

ساكن
M3_نتیجه GetU8 (M3_Snapshot_بارگیری* l, u8* o_مقدار)
{
    أرجع Get(l, o_مقدار, حجم(*o_مقدار));
}

ساكن
M3_نتیجه GetBool (M3_Snapshot_بارگیری* l, u8* o_مقدار)
{
    M3_نتیجه نتیجه = GetU8(l, o_مقدار);
    أرجع نتیجه ? نتیجه : (*o_مقدار <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ساكن
M3_نتیجه GetLEB_u32 (M3_Snapshot_بارگیری* l, u32* o_مقدار)
{
    إذا (l->درون_Section) {
        أرجع ReadLEB_u32(o_مقدار, &l->pos, l->انتها);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    لكل (u32 i = 0; i < حجم(encoded); ++i) {
        M3_نتیجه نتیجه = GetU8(l, &encoded[i]);
        إذا (نتیجه) {
            أرجع نتیجه;
        }
        إذا (not (encoded[i] & 0x80)) {
            بایتها_t pos = encoded;
            أرجع ReadLEB_u32(o_مقدار, &pos, encoded + i + 1);
        }
    }
    أرجع m3_Err_leb_سرریز;
}

ساكن
M3_نتیجه GetLEB_u64 (M3_Snapshot_بارگیری* l, u64* o_مقدار)
{
    أرجع ReadLebUnsigned(o_مقدار, 64, &l->pos, l->انتها);
}

ساكن
M3_نتیجه GetLEB_i64 (M3_Snapshot_بارگیری* l, i64* o_مقدار)
{
    أرجع ReadLEB_i64(o_مقدار, &l->pos, l->انتها);
}

ساكن
M3_نتیجه Skip_بایتها (M3_Snapshot_بارگیری* l, size_t i_اندازه)
{
    إذا (i_اندازه > (size_t)(l->انتها - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    l->pos += i_اندازه;
    أرجع m3Err_none;
}


ساكن
M3_نتیجه Get_Ref_مقدار (M3_Snapshot_بارگیری* l, m3type_t i_type, فراغ** o_reference)
{
    M3_نتیجه نتیجه = m3Err_none;
    u8       نوع   = 0;
    u64      id     = 0;

    *o_reference = عدم;

_   (GetU8(l, &نوع));
    إذا (نوع == 0x00) {
        أرجع (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    إذا (نوع == 0x01) {
        _throwif(m3Err_wasmMalformed, پایه_Type_از(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->رقم_Functions);
#  إذا d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_از(Ref_Type_از_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } وإلا إذا (نوع == 0x02) {
        _throwif(m3Err_wasmMalformed, پایه_Type_از(i_type) != c_m3Type_externref);
        ثابت M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        إذا (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (نوع == 0x03) {
        _throwif(m3Err_wasmMalformed, پایه_Type_از(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->رقم_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    وإلا إذا (نوع == 0x04) {
        _throwif(m3Err_wasmMalformed, پایه_Type_از(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->رقم_Continuations);
        *o_reference = l->continuations[id];
#  إذا d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        إذا (not l->checking and انباشته_Type_از(i_type) != d_m3_Type_انباشته_Abstract) {
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
    أرجع نتیجه;
}

ساكن
M3_نتیجه GetLE32 (M3_Snapshot_بارگیری* l, u32* o_مقدار)
{
    u8       بایتها[4];
    M3_نتیجه نتیجه = Get(l, بایتها, حجم(بایتها));

    *o_مقدار = 0;

    لكل (u32 i = 0; i < حجم(بایتها); ++i) {
        *o_مقدار |= (u32)بایتها[i] << (8 * i);
    }

    أرجع نتیجه;
}

ساكن
M3_نتیجه GetLE64 (M3_Snapshot_بارگیری* l, u64* o_مقدار)
{
    u8       بایتها[8];
    M3_نتیجه نتیجه = Get(l, بایتها, حجم(بایتها));

    *o_مقدار = 0;

    لكل (u32 i = 0; i < حجم(بایتها); ++i) {
        *o_مقدار |= (u64)بایتها[i] << (8 * i);
    }

    أرجع نتیجه;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ساكن
M3_نتیجه Get_مقدار_برچسب (M3_Snapshot_بارگیری* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_نتیجه نتیجه  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_مقصد_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and پایه_Type_از(*o_type) != پایه_Type_از(i_expectedType));
    إذا (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    أرجع نتیجه;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ساكن
M3_نتیجه Get_مقدار_Body (M3_Snapshot_بارگیری* l, m3type_t i_type, u64* o_بیتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    *o_بیتها = 0;

    إذا (IsRefType(i_type)) {
        فراغ* reference = عدم;
_       (Get_Ref_مقدار(l, i_type, &reference));
        *o_بیتها = (u64)(uintptr_t)reference;
    } وإلا إذا (پایه_Type_از(i_type) == c_m3Type_i32) {
        i32 مقدار = 0;
_       (ReadLEB_i32(&مقدار, &l->pos, l->انتها));
        *o_بیتها = (u32)مقدار;
    } وإلا إذا (پایه_Type_از(i_type) == c_m3Type_i64) {
        i64 مقدار = 0;
_       (GetLEB_i64(l, &مقدار));
        *o_بیتها = (u64)مقدار;
    } وإلا إذا (پایه_Type_از(i_type) == c_m3Type_f32) {
        u32 بیتها = 0;
_       (GetLE32(l, &بیتها));
        *o_بیتها = بیتها;
    } وإلا إذا (پایه_Type_از(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_بیتها));
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه Get_مقدار_بیتها (M3_Snapshot_بارگیری* l, m3type_t i_expectedType, u64* o_بیتها)
{
    M3_نتیجه نتیجه = m3Err_none;
    m3type_t type;

_   (Get_مقدار_برچسب(l, i_expectedType, &type));
_   (Get_مقدار_Body(l, type, o_بیتها));

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه Get_مقدار (M3_Snapshot_بارگیری* l, m3type_t i_expectedType, فراغ* o_where)
{
    M3_نتیجه نتیجه = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    إذا (l->checking) {
        o_where = scratch;
    }

_   (Get_مقدار_برچسب(l, i_expectedType, &type));

    إذا (IsRefType(type)) {
        فراغ* reference = عدم;
        u64   بیتها      = 0;
_       (Get_مقدار_Body(l, type, &بیتها));
        reference = (فراغ*)(uintptr_t)بیتها;
        کپی_حافظه(o_where, &reference, حجم(reference));
    } وإلا إذا (پایه_Type_از(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } وإلا {
        u64 بیتها = 0;
_       (Get_مقدار_Body(l, type, &بیتها));

        إذا (Is64_بیت_Type(type)) {
            کپی_حافظه(o_where, &بیتها, حجم(بیتها));
        } وإلا {
            u32 narrow = (u32)بیتها;
            کپی_حافظه(o_where, &narrow, حجم(narrow));
        }
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه GetFunction (M3_Snapshot_بارگیری* l, IM3Function* o_function)
{
    M3_نتیجه نتیجه = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->رقم_Functions);
    *o_function = &l->module->functions[index];

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_حافظه_Chunks (M3_Snapshot_بارگیری* l, u8* o_بایتها, size_t i_اندازه)
{
    M3_نتیجه نتیجه = m3Err_none;

    لكل (;;) {
        u8  chunkType = 0;
        u32 offset = 0, درازی = 0;

_       (GetU8(l, &chunkType));

        إذا (chunkType == d_m3_Chunk_انتها) {
            اكسر;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &درازی));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_اندازه or درازی > i_اندازه - (size_t)offset);

        إذا (chunkType == d_m3ChunkRaw) {
            إذا (l->checking or درازی == 0) {
_               (Skip_بایتها(l, درازی));
            } وإلا {
_               (Get(l, o_بایتها + offset, درازی));
            }
        } وإلا إذا (chunkType == d_m3ChunkFillFF) {
            إذا (not l->checking and درازی) {
                memset(o_بایتها + offset, 0xFF, درازی);
            }
        } وإلا {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Memories (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه  نتیجه      = m3Err_none;
    IM3Module module      = l->module;
    u32       رقم_Memories = 0;
    l->loaded_حافظه_بایتها  = 0;

_   (GetLEB_u32(l, &رقم_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, رقم_Memories == 0 or رقم_Memories != module->رقم_Memories);

    لكل (u32 m = 0; m < رقم_Memories; ++m) {
        u32       mem_شاخص = 0, اولی_شاخص = 0, صفحه_بیتها = 0, رقم_صفحات = 0;
        IM3_حافظه حافظه;

_       (GetLEB_u32(l, &mem_شاخص));
_       (GetLEB_u32(l, &اولی_شاخص));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_شاخص != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 اولی_شاخص != حافظه_اولی_شاخص(module, m));
        إذا (اولی_شاخص != m) {
            استمر;
        }

_       (GetLEB_u32(l, &صفحه_بیتها));
_       (GetLEB_u32(l, &رقم_صفحات));

        حافظه = module->memories[mem_شاخص];
        _throwif("a shared memory cannot be restored from a snapshot", حافظه and حافظه->isShared);

        size_t بایتها = 0;
        u8*    داده  = عدم;

        إذا (حافظه) {
            _throwif(m3Err_wasmMalformed, صفحه_بیتها >= 32 or (1u << صفحه_بیتها) != حافظه_صفحه_اندازه(حافظه));
            _throwif(m3Err_wasmMalformed, رقم_صفحات < حافظه->رقم_صفحات or رقم_صفحات > حافظه->max_صفحات);
            _throwif(m3Err_wasmMalformed, رقم_صفحات > SIZE_MAX / حافظه_صفحه_اندازه(حافظه));
            بایتها = (size_t)رقم_صفحات * حافظه_صفحه_اندازه(حافظه);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->حافظه_حد and بایتها > (size_t)l->runtime->حافظه_حد);

            إذا (not l->checking) {
                إذا (حافظه->رقم_صفحات < رقم_صفحات) {
_                   (Resize_حافظه(l->runtime, حافظه, رقم_صفحات));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         حافظه->mallocated and حافظه->mallocated->درازی < بایتها);
            }

            l->loaded_حافظه_بایتها += بایتها;

            إذا (بایتها and حافظه->mallocated) {
                داده = m3_Mem_داده(حافظه->mallocated);
            }
        } وإلا {
            _throwif(m3Err_wasmMalformed, رقم_صفحات != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        إذا (داده and not l->checking) {
            memset(داده, 0, بایتها);
        }

_       (بارگیری_حافظه_Chunks(l, داده, بایتها));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_حافظه_بایتها != l->required_حافظه_بایتها);

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Globals (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه  نتیجه     = m3Err_none;
    IM3Module module     = l->module;
    u32       رقم_Globals = 0;

_   (GetLEB_u32(l, &رقم_Globals));
    _throwif(m3Err_wasmMalformed, رقم_Globals == 0 or رقم_Globals != module->رقم_Globals);

    لكل (u32 g = 0; g < رقم_Globals; ++g) {
        u32       global_شاخص = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_شاخص));
        _throwif(m3Err_wasmMalformed, global_شاخص != g);

        global = &module->globals[global_شاخص];
        cell   = global->resolved ? global->resolved : global;

_       (Get_مقدار(l, global->type, &cell->i64_مقدار));
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Tables (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه  نتیجه       = m3Err_none;
    IM3Module module       = l->module;
    u32       رقم_Tables    = 0;
    l->loaded_جدول_Elements = 0;

_   (GetLEB_u32(l, &رقم_Tables));
    _throwif(m3Err_wasmMalformed, رقم_Tables == 0 or رقم_Tables != module->رقم_Tables);

    لكل (u32 t = 0; t < رقم_Tables; ++t) {
        u32      جدول_شاخص = 0, اولی_شاخص = 0, اندازه = 0;
        u8       valtype = 0;
        IM3_جدول جدول_2;

_       (GetLEB_u32(l, &جدول_شاخص));
_       (GetLEB_u32(l, &اولی_شاخص));

        _throwif(m3Err_wasmMalformed, جدول_شاخص != t);
        _throwif("the snapshot shares tables between imports differently",
                 اولی_شاخص != جدول_اولی_شاخص(module, t));
        إذا (اولی_شاخص != t) {
            استمر;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &اندازه));

        جدول_2 = module->tables[جدول_شاخص];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_مقصد_Val_Type(جدول_2->type));
        _throwif(m3Err_wasmMalformed, اندازه > (جدول_2->hasMax ? جدول_2->max_اندازه : d_m3_Max_Sane_جدول_اندازه));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, اندازه < جدول_2->اندازه);

        l->loaded_جدول_Elements += اندازه;

        إذا (not l->checking and اندازه != جدول_2->اندازه) {
            فراغ** elements;

            _throwif(m3_Err_جدول_حد_Exceeded, l->runtime->جدول_Elements_حد and
                                                 اندازه - جدول_2->اندازه > l->runtime->جدول_Elements_حد - l->runtime->جدول_Elements_استفاده_شده);
            elements = m3_ReallocArray(فراغ*, جدول_2->elements, اندازه, جدول_2->اندازه);
            _throwifnull(elements);

            l->runtime->جدول_Elements_استفاده_شده += اندازه - جدول_2->اندازه;
            جدول_2->elements = elements;
            جدول_2->اندازه     = اندازه;
        }

        لكل (u32 e = 0; e < اندازه; ++e) {
            فراغ* reference = عدم;
_           (Get_Ref_مقدار(l, جدول_2->type, &reference));
            إذا (not l->checking) {
                جدول_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_جدول_Elements != l->required_جدول_Elements);

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Segments (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه  نتیجه = m3Err_none;
    IM3Module module = l->module;
    u32       شمار  = 0;

_   (GetLEB_u32(l, &شمار));
    _throwif(m3Err_wasmMalformed, شمار != module->رقم_داده_Segments);
    _throwif(m3Err_wasmMalformed, module->رقم_داده_Segments == 0 and module->رقم_Element_Segments == 0);

    لكل (u32 i = 0; i < شمار; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->داده_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &شمار));
    _throwif(m3Err_wasmMalformed, شمار != module->رقم_Element_Segments);

    لكل (u32 i = 0; i < شمار; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    أرجع نتیجه;
}

// A frame for the continuation being put together, outermost first
ساكن
M3_نتیجه جمع_کن_قاب (M3_Snapshot_بارگیری* l, u32* io_رقم_قابها, M3_قاب_نوع i_نوع, m3stack_t i_sp, M3_قاب** o_قاب)
{
    إذا (*io_رقم_قابها >= d_m3_Continuation_Max_قابها) {
        أرجع "the snapshot's frames do not fit this build's";
    }

    M3_قاب* قاب = &l->قابها[(*io_رقم_قابها)++];

    memset(قاب, 0, حجم(*قاب));
    قاب->نوع   = (u8)i_نوع;
    قاب->sp     = i_sp;
    قاب->حافظه = Module_Memory0(l->module);

    *o_قاب = قاب;

    أرجع m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ساكن
M3_نتیجه PrepareFunction (IM3Function i_function)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    أرجع نتیجه;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ساكن
M3_نتیجه جمع_کن_بلوک_قابها (M3_Snapshot_بارگیری* l, ثابت M3SnapshotMap* i_map, ثابت M3_امن_نقطه* i_نقطه, m3stack_t i_sp,
                         u32* io_رقم_قابها)
{
    M3_نتیجه نتیجه = m3Err_none;

    لكل (u32 b = 0; b < i_map->رقم_بلوکها; ++b) {
        ثابت M3_بلوک_شروع* بلوک = &i_map->بلوکها[b];
        M3_قاب*            قاب;

        إذا (not Encloses_جابه_جایی(بلوک, i_نقطه->wasm_جابه_جایی)) {
            استمر;
        }

        إذا (بلوک->opcode == c_waOp_loop) {
_           (جمع_کن_قاب(l, io_رقم_قابها, قاب_loop, i_sp, &قاب));
            قاب->pc = بلوک->pc;
        }
#  إذا d_m3HasExceptionHandling
        وإلا {
_           (جمع_کن_قاب(l, io_رقم_قابها, قاب_try, i_sp, &قاب));
            قاب->pc                = بلوک->pc;
            قاب->try_.رقم_Clauses   = بلوک->رقم_Clauses;
            قاب->try_.handlers_زنده = Handlers_زنده_At(i_map, i_نقطه, بلوک);
        }
#  endif
    }

_catch:
    أرجع نتیجه;
}

// Puts a function's values where this build keeps them at the safepoint
ساكن
M3_نتیجه بارگیری_مقادیر (M3_Snapshot_بارگیری* l, m3slot_t* i_پایه, u32 i_رقم_Slots, m3stack_t i_sp,
                     ثابت M3SnapshotMap* i_map, ثابت M3_امن_نقطه* i_نقطه, ثابت M3_قاب_Registers* i_registers)
{
    M3_نتیجه نتیجه    = m3Err_none;
    u32      رقم_مقادیر = 0;

_   (GetLEB_u32(l, &رقم_مقادیر));
    _throwif("the snapshot's frame does not match what this build compiled", رقم_مقادیر != i_map->رقم_Locals + i_نقطه->رقم_مقادیر);

    لكل (u32 v = 0; v < رقم_مقادیر; ++v) {
        ثابت M3_جایگاه_مقدار* مقدار     = قاب_مقدار(i_map, i_نقطه, v);
        u64                کلمه      = 0;
        فراغ*              reference = عدم;
        u8*                where;

_       (Locate_مقدار(i_پایه, i_رقم_Slots, i_sp, مقدار, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (پایه_Type_از(مقدار->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_مقدار_برچسب(l, مقدار->type, &type));
            إذا (l->checking) {
_               (Skip_بایتها(l, 16));
            } وإلا {
_               (Get(l, where, 16));
            }
            استمر;
        }

        l->allowUnfilledRef = i_نقطه->نوع == safepoint_suspend and
                              v >= رقم_مقادیر - i_نقطه->رقم_Results;
_       (Get_مقدار_بیتها(l, مقدار->type, &کلمه));
        l->allowUnfilledRef = false;

        إذا (IsRefType(مقدار->type)) {
            reference = (فراغ*)(uintptr_t)کلمه;
        }

        إذا (not l->checking) {
_           (Write_مقدار(where, i_registers, مقدار, کلمه, reference));
        }
    }

_catch:
    أرجع نتیجه;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ساكن
M3_نتیجه بارگیری_Suspended_Body (M3_Snapshot_بارگیری* l, IM3Continuation io_cont, u32 i_id)
{
    M3_نتیجه    نتیجه    = m3Err_none;
    u32         رقم_قابها = 0;
    M3_قاب*    call_قاب = عدم;
    IM3Function function  = عدم;
    m3slot_t*   پایه;
    u32         رقم_Slots;
    u32         رقم_Activations = 0;
    IM3FuncType expected       = io_cont->مدخل_Function->funcType;
    m3stack_t   sp;

_   (Continuation_پشته(l->runtime, io_cont, &پایه, &رقم_Slots));

_   (GetLEB_u32(l, &رقم_Activations));
    _throwif(m3Err_wasmMalformed, رقم_Activations == 0 or رقم_Activations > d_m3_Continuation_Max_قابها);

    io_cont->pc                = عدم;
    io_cont->r0                = 0;
    io_cont->رقم_Suspend_Results = 0;
    io_cont->suspend_نقطه      = safepoint_op;
#  إذا d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = پایه;

    لكل (u32 a = 0; a < رقم_Activations; ++a) {
        bool                 isInnermost = (a + 1 == رقم_Activations);
        ثابت M3_امن_نقطه*   نقطه;
        M3_قاب_Registers     registers;
        u8                   نوع       = 0;
        u32                  wasm_جابه_جایی = 0, هدف_Loop = 0;
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
                 (u32)(sp - پایه) + function->max_پشته_Slots > رقم_Slots);

        // the call the frame outside this one is waiting on
        إذا (call_قاب) {
            call_قاب->call.function = function;
        }

#  إذا d_m3_مدخل_Keeps_قاب
        {
            M3_قاب* مدخل;
_           (جمع_کن_قاب(l, &رقم_قابها, قاب_مدخل, sp, &مدخل));
            مدخل->pc             = function->compiled;
            مدخل->مدخل.function = function;
        }
#  endif

_       (GetU8(l, &نوع));
_       (GetLEB_u32(l, &wasm_جابه_جایی));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        إذا (نوع == safepoint_op) {
            ثابت M3_بلوک_شروع* loop;

_           (GetLEB_u32(l, &هدف_Loop));

            loop = پیدا_کن_بلوک_At(map, c_waOp_loop, هدف_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_جابه_جایی(loop, wasm_جابه_جایی));
        }

        نقطه = پیدا_کن_امن_نقطه_At(map, wasm_جابه_جایی, نوع, هدف_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not نقطه);

_       (جمع_کن_بلوک_قابها(l, map, نقطه, sp, &رقم_قابها));

        إذا (not isInnermost) {
            _throwif(m3Err_wasmMalformed, نوع != safepoint_call);

_           (جمع_کن_قاب(l, &رقم_قابها, قاب_call, sp, &call_قاب));
            call_قاب->pc = نقطه->pc;
            registers.r0  = &call_قاب->call.r0;
#  إذا d_m3HasFloat
            registers.fp0 = &call_قاب->call.fp0;
#  endif
        } وإلا إذا (نوع == safepoint_resume) {
            M3_قاب* قاب;
            u32      رقم_Results = نقطه->رقم_Results;
            pc_t     resultsPC  = نقطه->pc - 2 * رقم_Results;

_           (جمع_کن_قاب(l, &رقم_قابها, قاب_resume, sp, &قاب));
            قاب->pc                 = نقطه->pc;
            قاب->resume.resultsPC   = resultsPC;
            قاب->resume.رقم_Results  = رقم_Results;
            قاب->resume.رقم_Handlers = نقطه->aux;
            قاب->resume.handlersPC  = resultsPC - 1 - 3 * نقطه->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            call_قاب = قاب;
        } وإلا {
            _throwif(m3Err_wasmMalformed, نوع != safepoint_op and نوع != safepoint_مدخل and نوع != safepoint_suspend);

            io_cont->pc           = نقطه->pc;
            io_cont->suspend_نقطه = نوع;
            registers.r0          = &io_cont->r0;
#  إذا d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (بارگیری_مقادیر(l, پایه, رقم_Slots, sp, map, نقطه, &registers));

        إذا (not l->checking) {
            RestoreConstants(function, sp);
        }

        إذا (not isInnermost) {
            sp = sp + نقطه->aux;
_           (CallSiteType(function, نقطه, &expected));
        } وإلا إذا (نوع == safepoint_resume) {
            فراغ* cont = عدم;
_           (Get_Ref_مقدار(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == عدم);

            call_قاب->resume.cont = (IM3Continuation)cont;
            إذا (not l->checking) {
                l->suspendSites[i_id].نقطه = نقطه;
                لكل (u32 i = 1; i < l->رقم_Continuations; ++i) {
                    إذا (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].هدف = i;
                        l->suspendSites[i].resumed   = true;
                        اكسر;
                    }
                }
            }
        } وإلا {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            إذا (نقطه->flags & d_m3_امن_نقطه_گرفته_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            إذا (نوع == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, نقطه->رقم_Results > نقطه->رقم_مقادیر or
                                                نقطه->رقم_Results > d_m3MaxContinuationPayload);

                io_cont->رقم_Suspend_Results = نقطه->رقم_Results;
                إذا (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].نقطه = نقطه;
                }

                لكل (u32 i = 0; i < نقطه->رقم_Results; ++i) {
                    ثابت M3_جایگاه_مقدار* مقدار = &map->مقادیر[نقطه->اولی_مقدار + نقطه->رقم_مقادیر - نقطه->رقم_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_ثبات_جایگاه_Alias(مقدار->جایگاه));

                    io_cont->suspend_نتیجه_Offsets[i] = مقدار->جایگاه;
                    io_cont->suspend_نتیجه_Is64[i]    = Is64_بیت_Type(مقدار->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    إذا (l->checking) {
        أرجع m3Err_none;
    }

    // held innermost first
    إذا (رقم_قابها > io_cont->قابها_Cap) {
        M3_قاب* قابها = m3_ReallocArray(M3_قاب, io_cont->قابها, رقم_قابها, io_cont->قابها_Cap);
        _throwifnull(قابها);

        io_cont->قابها    = قابها;
        io_cont->قابها_Cap = رقم_قابها;
    }

    لكل (u32 k = 0; k < رقم_قابها; ++k) {
        io_cont->قابها[رقم_قابها - 1 - k] = l->قابها[k];
    }

    io_cont->رقم_قابها = رقم_قابها;

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Continuation (M3_Snapshot_بارگیری* l, u32 i_id)
{
    M3_نتیجه        نتیجه = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    إذا (l->checking) {
        scratch          = *cont;
        scratch.val_پشته = l->check_پشته;
        إذا (i_id == 0) {
            scratch.val_پشته = (m3slot_t*)l->runtime->origin_پشته;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_شاخص = 0, مدخل_شاخص = 0, bound_آرگومانها_شمار = 0;
    u8  state = 0, is_ریشه = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_ریشه));
_   (GetLEB_u32(l, &type_شاخص));
_   (GetLEB_u32(l, &مدخل_شاخص));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_ریشه != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_ریشه and state != snapshot_contSuspended);
    إذا (not is_ریشه and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_شاخص != d_m3SnapshotNone and type_شاخص >= module->رقم_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, مدخل_شاخص >= module->رقم_Functions);
    _throwif(m3Err_wasmMalformed, is_ریشه ? type_شاخص != d_m3SnapshotNone : (type_شاخص == d_m3SnapshotNone or not module->funcTypes[type_شاخص]->isContinuation));

    إذا (not l->checking and not is_ریشه and state != snapshot_cont_پایانیافته) {
_       (Continuation_Acquire_پشته(l->runtime, cont));
    }
    cont->type          = (type_شاخص != d_m3SnapshotNone) ? module->funcTypes[type_شاخص] : عدم;
    cont->مدخل_Function = &module->functions[مدخل_شاخص];
    cont->رقم_Handlers   = 0;
    cont->handlersPC    = عدم;
    cont->والد        = عدم;
    cont->رقم_قابها     = 0;

#  إذا d_m3HasExceptionHandling
    {
        فراغ* ref = عدم;
_       (Get_Ref_مقدار(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_ریشه or state == snapshot_cont_پایانیافته) and cont->resumeThrow != عدم);
#  وإلا
    {
        فراغ* ref = عدم;
_       (Get_Ref_مقدار(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != عدم);
    }
#  endif

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         رقم_Rets;

_       (GetLEB_u32(l, &bound_آرگومانها_شمار));
        cont->bound_آرگومانها_شمار = bound_آرگومانها_شمار;
        inner                = cont->مدخل_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_آرگومانها_شمار > Get_Func_Type_رقم_Params(inner));

        {
            IM3FuncType مانده = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not مانده or
                                            Get_Func_Type_رقم_Params(مانده) != Get_Func_Type_رقم_Params(inner) - bound_آرگومانها_شمار or
                                            Get_Func_Type_رقم_Results(مانده) != Get_Func_Type_رقم_Results(inner));
            لكل (u16 i = 0; i < Get_Func_Type_رقم_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجه_Type(inner, i) != Get_Func_Type_نتیجه_Type(مانده, i));
            }
            لكل (u16 i = 0; i < Get_Func_Type_رقم_Params(مانده); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_آرگومانها_شمار + i)) != GetFuncTypeParamType(مانده, i));
            }
        }

        رقم_Rets = Get_Func_Type_رقم_Results(inner);

        _throwif(m3Err_wasmMalformed, (رقم_Rets + bound_آرگومانها_شمار) * c_io_جایگاه_شمار > cont->رقم_پشته_Slots);

        لكل (u32 i = 0; i < bound_آرگومانها_شمار; ++i) {
_           (Get_مقدار(l, GetFuncTypeParamType(inner, (u16)i), cont->val_پشته + (رقم_Rets + i) * c_io_جایگاه_شمار));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_پشته;
        cont->pc    = cont->مدخل_Function->compiled;
    } وإلا إذا (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_آرگومانها_شمار));
        _throwif(m3Err_wasmMalformed, is_ریشه and bound_آرگومانها_شمار != 0);
        cont->bound_آرگومانها_شمار = bound_آرگومانها_شمار;

_       (بارگیری_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } وإلا إذا (state == snapshot_cont_پایانیافته) {
_       (GetLEB_u32(l, &bound_آرگومانها_شمار));
        _throwif(m3Err_wasmMalformed, bound_آرگومانها_شمار != 0);
        cont->bound_آرگومانها_شمار = 0;
        cont->state          = cont_consumed;
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    إذا (نتیجه) {
        cont->رقم_قابها = 0;
    }

    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Continuations (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه نتیجه = m3Err_none;
    u32      شمار  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &شمار));
    _throwif(m3Err_wasmMalformed, شمار == 0 or شمار != l->رقم_Continuations);

    لكل (u32 i = 0; i < شمار; ++i) {
_       (بارگیری_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    أرجع نتیجه;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ساكن
M3_نتیجه CheckContinuationReferences (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه نتیجه = m3Err_none;

    لكل (u32 id = 0; id < l->رقم_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             برگ  = id;
        u32             عمق = 0;
#  إذا d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_از(Ref_Type_از_Func_Type(cont->type, true), expectedType)));
#  endif
        إذا (l->suspendSites[id].هدف) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_از_Ref(l->module, l->suspendSites[id].نقطه->resumeType);
            IM3Continuation فرزند    = l->continuations[l->suspendSites[id].هدف];
            IM3FuncType     actual   = فرزند->مدخل_Function ? فرزند->مدخل_Function->funcType : عدم;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_رقم_Results(actual) != Get_Func_Type_رقم_Results(expected));
            لكل (u16 i = 0; i < Get_Func_Type_رقم_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجه_Type(actual, i) != Get_Func_Type_نتیجه_Type(expected, i));
            }
        }
        طالما (l->suspendSites[برگ].هدف) {
            برگ = l->suspendSites[برگ].هدف;
            _throwif(m3Err_wasmMalformed, ++عمق >= l->رقم_Continuations or
                                            l->continuations[برگ]->state != cont_suspended);
        }
        إذا (cont->state != cont_suspended or id == 0) {
            استمر;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        إذا (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_آرگومانها_شمار != 0);
            استمر;
        }

        ثابت M3SnapshotSuspendSite* site      = &l->suspendSites[برگ];
        IM3Continuation              inner     = l->continuations[برگ];
        IM3FuncType                  مانده = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->نقطه or not مانده);
        _throwif("a captured continuation has the wrong binding count", cont->bound_آرگومانها_شمار > inner->رقم_Suspend_Results or
                                                                          Get_Func_Type_رقم_Params(مانده) != inner->رقم_Suspend_Results - cont->bound_آرگومانها_شمار);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_رقم_Results(مانده) != Get_Func_Type_رقم_Results(cont->مدخل_Function->funcType));
        لكل (u16 i = 0; i < Get_Func_Type_رقم_Results(مانده); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_نتیجه_Type(مانده, i) !=
                                            Get_Func_Type_نتیجه_Type(cont->مدخل_Function->funcType, i));
        }

        لكل (u32 i = 0; i < inner->رقم_Suspend_Results; ++i) {
            ثابت M3_جایگاه_مقدار* مقدار = &site->map->مقادیر[site->نقطه->اولی_مقدار +
                                                          site->نقطه->رقم_مقادیر - site->نقطه->رقم_Results + i];
            إذا (i < cont->bound_آرگومانها_شمار) {
                إذا (IsRefType(مقدار->type) and not IsNullableRef(مقدار->type)) {
                    فراغ* ref = عدم;
                    کپی_حافظه(&ref, inner->sp + مقدار->جایگاه, حجم(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } وإلا {
                _throwif("a captured continuation has the wrong parameter type", مقدار->type !=
                                                                                   GetFuncTypeParamType(مانده, (u16)(i - cont->bound_آرگومانها_شمار)));
            }
        }
    }

_catch:
    أرجع نتیجه;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_نتیجه بارگیری_Exceptions (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه نتیجه        = m3Err_none;
    u32      رقم_Exceptions = 0;

_   (GetLEB_u32(l, &رقم_Exceptions));
    _throwif(m3Err_wasmMalformed, رقم_Exceptions == 0 or رقم_Exceptions != l->رقم_Exceptions);

    لكل (u32 i = 0; i < رقم_Exceptions; ++i) {
        u32          exnId = 0, رقم_آرگومانها = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &رقم_آرگومانها));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->برچسب->type;

        _throwif(m3Err_wasmMalformed, رقم_آرگومانها != exn->رقم_آرگومانها);

        لكل (u32 a = 0; a < رقم_آرگومانها; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      بیتها    = 0;
_           (Get_مقدار_بیتها(l, argType, &بیتها));
            إذا (not l->checking) {
                exn->آرگومانها[a] = بیتها;
            }
        }
    }

_catch:
    أرجع نتیجه;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
تعريفنوع بنية M3BoundedReader {
    M3_Snapshot_بارگیری* بارگیری;
    u64             مانده;
    M3_نتیجه        error;
} M3BoundedReader;

ساكن
M3_نتیجه BoundedReader_Read (فراغ* o_میانگیر, size_t i_اندازه, فراغ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    إذا (i_اندازه > br->مانده) {
        br->error = "the host read past the end of its state";
        أرجع br->error;
    }

    br->مانده -= i_اندازه;

    أرجع Get(br->بارگیری, o_میانگیر, i_اندازه);
}

ساكن
M3_نتیجه بارگیری_Host_State (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه               نتیجه = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 اندازه = (size_t)(l->انتها - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->بارگیری_Host_State_2);

    إذا (l->checking) {
        أرجع Skip_بایتها(l, اندازه);
    }
    reader.بارگیری      = l;
    reader.مانده = اندازه;
    reader.error     = m3Err_none;

_   (hooks->بارگیری_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, اندازه));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.مانده != 0);

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_سرآیند (M3_Snapshot_بارگیری* l)
{
    M3_نتیجه نتیجه = m3Err_none;
    u8       magic[حجم(c_snapshotMagic)];
    u32      نسخه = 0;

_   (Get(l, magic, حجم(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, حجم(magic)) != 0);

_   (GetLE32(l, &نسخه));
    _throwif(m3Err_wasmMalformed, نسخه != c_snapshot_نسخه);

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Meta (M3_Snapshot_بارگیری* l, u32* o_flags)
{
    M3_نتیجه نتیجه      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      حافظه_بایتها = 0, جدول_Elements = 0;
    u32      فعال_Stacks = 0;
    u64      current_حافظه, currentTables;
    SnapshotResourceTotals(l->module, &current_حافظه, &currentTables);

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
_   (GetLEB_u64(l, &حافظه_بایتها));
_   (GetLEB_u64(l, &جدول_Elements));
_   (GetLEB_u32(l, &فعال_Stacks));
    l->requiredStacks        = فعال_Stacks;
    l->required_حافظه_بایتها   = حافظه_بایتها;
    l->required_جدول_Elements = جدول_Elements;
    _throwif(m3Err_wasmMalformed, حافظه_بایتها < current_حافظه or جدول_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->رقم_Memories and حافظه_بایتها != 0);
    _throwif(m3Err_wasmMalformed, not l->module->رقم_Tables and جدول_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    إذا (*o_flags & d_m3SnapshotFlagPostmortem) {
        لكل (u32 i = 0; i < l->رقم_Exceptions; ++i) {
            u32 برچسب_شاخص = 0;
_           (GetLEB_u32(l, &برچسب_شاخص));
        }
        أرجع m3Err_none;
    }

    _throwif(m3_Err_حافظه_حد_Exceeded, l->runtime->حافظه_بایتها_حد and
                                          حافظه_بایتها - current_حافظه > l->runtime->حافظه_بایتها_حد - l->runtime->حافظه_بایتها_استفاده_شده);
    _throwif(m3_Err_جدول_حد_Exceeded, l->runtime->جدول_Elements_حد and
                                         جدول_Elements - currentTables > l->runtime->جدول_Elements_حد - l->runtime->جدول_Elements_استفاده_شده);
    _throwif(m3_Err_continuation_حد_Exceeded, l->runtime->continuations_حد and
                                                فعال_Stacks > l->runtime->continuations_حد - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->رقم_Continuations == 0 or فعال_Stacks >= l->رقم_Continuations);
#  إذا !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->رقم_Exceptions != 0);
#  endif

    إذا (l->checking) {
        لكل (u32 i = 0; i < l->رقم_Exceptions; ++i) {
            u32 برچسب_شاخص = 0;
_           (GetLEB_u32(l, &برچسب_شاخص));
#  إذا d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, برچسب_شاخص >= l->module->رقم_برچسبها);
#  endif
        }
        أرجع m3Err_none;
    }

    l->check_پشته = m3_AllocArray(m3slot_t, d_m3_Continuation_پشته_Slots + 4);
    _throwifnull(l->check_پشته);
    l->قابها = m3_AllocArray(M3_قاب, d_m3_Continuation_Max_قابها);
    _throwifnull(l->قابها);

    l->continuations = m3_AllocArray(IM3Continuation, l->رقم_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->رقم_Continuations);
    _throwifnull(l->suspendSites);

    إذا (not l->runtime->ریشه_Continuation) {
        IM3Continuation ریشه = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(ریشه);

        // the root borrows the runtime's stack, so it holds none of its own
        ریشه->رقم_پشته_Slots          = l->runtime->رقم_پشته_Slots;
        l->runtime->ریشه_Continuation = ریشه;
    }

    l->continuations[0] = l->runtime->ریشه_Continuation;

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
        u32    برچسب_شاخص = 0;
        IM3_برچسب برچسب;
        u32    رقم_آرگومانها;

_       (GetLEB_u32(l, &برچسب_شاخص));
        _throwif(m3Err_wasmMalformed, برچسب_شاخص >= l->module->رقم_برچسبها);

        برچسب     = برچسب_از_شاخص(l->module, برچسب_شاخص);
        رقم_آرگومانها = Get_Func_Type_رقم_Params(برچسب->type);

        l->exceptions[i] = NewException(l->runtime, برچسب, رقم_آرگومانها);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  وإلا
    لكل (u32 i = 0; i < l->رقم_Exceptions; ++i) {
        u32 برچسب_شاخص = 0;
_       (GetLEB_u32(l, &برچسب_شاخص));
    }
#  endif

_catch:
    أرجع نتیجه;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ساكن
M3_نتیجه بارگیری_Section (M3_Snapshot_بارگیری* l, u8 i_sectionId)
{
    M3_نتیجه نتیجه = m3Err_none;
    u32      flags  = 0;
    بدل (i_sectionId) {
    حالة d_m3SnapshotSection_Meta:
_       (بارگیری_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        اكسر;

    حالة d_m3_Snapshot_Section_حافظه:
_       (بارگیری_Memories(l));
        اكسر;

    حالة d_m3_Snapshot_Section_جدول:
_       (بارگیری_Tables(l));
        اكسر;

    حالة d_m3SnapshotSection_Global:
_       (بارگیری_Globals(l));
        اكسر;

    حالة d_m3SnapshotSection_Segment:
_       (بارگیری_Segments(l));
        اكسر;

#  إذا d_m3HasExceptionHandling
    حالة d_m3SnapshotSection_Exception:
_       (بارگیری_Exceptions(l));
        اكسر;
#  endif

    حالة d_m3SnapshotSection_Continuation:
_       (بارگیری_Continuations(l));
        اكسر;

    حالة d_m3SnapshotSection_HostState:
_       (بارگیری_Host_State(l));
        اكسر;

    افتراضي:
_       (Skip_بایتها(l, (size_t)(l->انتها - l->pos)));
        اكسر;
    }

_catch:
    أرجع نتیجه;
}

ساكن
M3_نتیجه بارگیری_Sections (M3_Snapshot_بارگیری* l, bool i_embedded)
{
    M3_نتیجه نتیجه  = m3Err_none;
    u8*      payload = عدم;

    إذا (not i_embedded) {
_       (بارگیری_سرآیند(l));
    }

    طالما (true) {
        u8  sectionId   = 0;
        u32 section_اندازه = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        إذا (GetU8(l, &sectionId)) {
            اكسر;
        }

_       (GetLEB_u32(l, &section_اندازه));

        _throwif("the snapshot holds a section twice", l->دیده_Sections[sectionId]);
        l->دیده_Sections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_اندازه == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_اندازه);
        _throwifnull(payload);
_       (Get(l, payload, section_اندازه));
        l->pos       = payload;
        l->انتها       = payload + section_اندازه;
        l->درون_Section = true;

        {
            M3_Snapshot_بارگیری check = *l;
            check.checking       = true;
_           (بارگیری_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.انتها);
        }
        l->تغییریافته = l->تغییریافته or sectionId != d_m3SnapshotSection_Meta;
_       (بارگیری_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->انتها);
        l->درون_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->دیده_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->رقم_Exceptions and not l->دیده_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->رقم_Memories and not l->دیده_Sections[d_m3_Snapshot_Section_حافظه]);
    _throwif("the snapshot has no table section",
             l->module->رقم_Tables and not l->دیده_Sections[d_m3_Snapshot_Section_جدول]);
    _throwif("the snapshot has no global section",
             l->module->رقم_Globals and not l->دیده_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->رقم_داده_Segments or l->module->رقم_Element_Segments) and
               not l->دیده_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    أرجع نتیجه;
}

#پایان_اگر // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3HasSnapshots

ساكن
M3_نتیجه SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_نتیجه       نتیجه = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, حجم(save));

    IM3Continuation ریشه = io_runtime->ریشه_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not ریشه or ریشه->state != cont_suspended;

    IM3Function مدخل = save.postmortem ? io_runtime->entered : ریشه->مدخل_Function;

    إذا (o_module) {
        *o_module = عدم;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not مدخل);

    save.module = مدخل->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    إذا (o_module) {
        *o_module = save.module;
    }

    لكل (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->بعدی) {
        u32 id;
_       (اشاره_گر_Ids_جمع_کن(&save.زنده_Continuations, cont, &id));
    }

#  إذا d_m3HasExceptionHandling
    لكل (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->بعدی) {
        u32 id;
_       (اشاره_گر_Ids_جمع_کن(&save.زنده_Exceptions, exception, &id));
    }
#  endif

    إذا (not save.postmortem) {
        u32 id;
_       (اشاره_گر_Ids_جمع_کن(&save.continuations, ریشه, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    اشاره_گر_Ids_Free(&save.زنده_Continuations);
    اشاره_گر_Ids_Free(&save.زنده_Exceptions);
    اشاره_گر_Ids_Free(&save.continuations);
    اشاره_گر_Ids_Free(&save.exceptions);

    أرجع نتیجه;
}

#پایان_اگر

M3_نتیجه m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata)
{
    إذا (not io_runtime or not i_writer) {
        أرجع m3Err_mallocFailed;
    }

#إذا d_m3HasSnapshots
    أرجع SaveSnapshot(io_runtime, i_writer, i_userdata, عدم, false);
#وإلا
    (فراغ)i_userdata;
    أرجع "snapshots are not available in this build of Wasm3";
#پایان_اگر
}


ساكن
M3_نتیجه بارگیری_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata, bool i_embedded)
{
    M3_نتیجه نتیجه = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#إذا d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_پیاپی);

    {
        M3_Snapshot_بارگیری بارگیری;
        memset(&بارگیری, 0, حجم(بارگیری));

        بارگیری.runtime  = io_runtime;
        بارگیری.module   = i_module;
        بارگیری.reader   = i_reader;
        بارگیری.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        نتیجه = بارگیری_Sections(&بارگیری, i_embedded);

        IM3Continuation ریشه = io_runtime->ریشه_Continuation;

        إذا (نتیجه) {
            إذا (بارگیری.تغییریافته) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                إذا (ریشه) {
                    ریشه->state     = cont_returned;
                    ریشه->رقم_قابها = 0;
                }
            }
        } وإلا {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->شروع_Function = -1;
            i_module->has_پیاپی        = true;
        }

        Runtime_قراربده_Call_پشته(io_runtime);

        m3_Free(بارگیری.check_پشته);
        m3_Free(بارگیری.قابها);
        m3_Free(بارگیری.suspendSites);
        m3_Free(بارگیری.continuations);
#  إذا d_m3HasExceptionHandling
        m3_Free(بارگیری.exceptions);
#  endif
    }
#وإلا
    (فراغ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#پایان_اگر

_catch:
    أرجع نتیجه;
}


M3_نتیجه m3_بارگیری_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata)
{
    أرجع بارگیری_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


فراغ m3_مجموعه_Snapshot_Hooks (IM3Runtime io_runtime, ثابت M3SnapshotHooks* i_hooks)
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
#پایان_اگر
}


M3_نتیجه m3_Save_Snapshot_مقصد_میانگیر (IM3Runtime io_runtime, فراغ** o_بایتها, size_t* o_اندازه)
{
    إذا (!io_runtime || !o_بایتها || !o_اندازه) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    M3_میانگیر_Writer bw     = { عدم, 0, 0 };
    M3_نتیجه       نتیجه = m3_SaveSnapshot(io_runtime, میانگیر_Writer_Write, &bw);
    إذا (نتیجه) {
        m3_Free(bw.میانگیر);
        *o_بایتها = عدم;
        *o_اندازه  = 0;
        أرجع نتیجه;
    }
    *o_بایتها = bw.میانگیر;
    *o_اندازه  = bw.اندازه;
    أرجع m3Err_none;
#وإلا
    *o_بایتها = عدم;
    *o_اندازه  = 0;
    أرجع "snapshots are not available in this build of Wasm3";
#پایان_اگر
}

#إذا d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ساكن
u8* EmitLEB_u32 (u8* o_where, u32 i_مقدار)
{
    افعل {
        u8 byte = i_مقدار & 0x7F;
        i_مقدار >>= 7;
        إذا (i_مقدار != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } طالما (i_مقدار != 0);

    أرجع o_where;
}

ساكن
u32 اندازه_از_LEB_u32 (u32 i_مقدار)
{
    u32 اندازه = 0;

    افعل {
        i_مقدار >>= 7;
        ++اندازه;
    } طالما (i_مقدار != 0);

    أرجع اندازه;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ساكن
M3_نتیجه پیدا_کن_Embedded_Snapshot (ثابت u8* i_شروع, ثابت u8* i_انتها, cstr_t i_secName,
                               ثابت u8** o_sec_شروع, ثابت u8** o_sec_انتها,
                               ثابت u8** o_داده, u32* o_اندازه)
{
    M3_نتیجه نتیجه = m3Err_none;
    بایتها_t  pos    = i_شروع;
    bool     یافت  = false;
    cstr_t   name   = عدم;

    إذا (o_sec_شروع) {
        *o_sec_شروع = عدم;
    }
    إذا (o_sec_انتها) {
        *o_sec_انتها = عدم;
    }
    إذا (o_داده) {
        *o_داده = عدم;
    }
    إذا (o_اندازه) {
        *o_اندازه = 0;
    }

    _throwif(m3Err_wasmMalformed, i_انتها - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    طالما (pos < i_انتها) {
        بایتها_t sec_شروع  = pos;
        u8      sectionId = *pos++;
        u32     اندازه      = 0;
_       (ReadLEB_u32(&اندازه, &pos, i_انتها));
        _throwif(m3Err_wasmUnderrun, اندازه > (size_t)(i_انتها - pos));
        {
            بایتها_t payload = pos;
            pos += اندازه;
            إذا (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                إذا (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", یافت);
                    یافت = true;
                    إذا (o_sec_شروع) {
                        *o_sec_شروع = sec_شروع;
                    }
                    إذا (o_sec_انتها) {
                        *o_sec_انتها = pos;
                    }
                    إذا (o_داده) {
                        *o_داده = payload;
                    }
                    إذا (o_اندازه) {
                        *o_اندازه = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    أرجع نتیجه;
}

// Builds the custom section name a snapshot of i_name lives under
ساكن
M3_نتیجه SnapshotSectionName (cstr_t i_name, حرف* o_secName, size_t i_اندازه)
{
    إذا (i_name and *i_name) {
        إذا (طول_رشته(i_name) > d_m3_Max_Snapshot_Name_درازی) {
            أرجع "the snapshot name is too long";
        }
        snprintf(o_secName, i_اندازه, "snapshot.%s", i_name);
    } وإلا {
        snprintf(o_secName, i_اندازه, "snapshot");
    }

    أرجع m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ساكن
M3_نتیجه ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_نتیجه نتیجه = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = عدم;

    لكل (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->بعدی) {
        إذا (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    أرجع نتیجه;
}
#پایان_اگر

M3_نتیجه m3_بارگیری_Snapshot_From_میانگیر (IM3Runtime io_runtime, IM3Module i_module, ثابت فراغ* i_بایتها, size_t i_اندازه)
{
    إذا (!io_runtime || !i_module || !i_بایتها) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    إذا (i_اندازه >= 8 && memcmp(i_بایتها, "\0asm", 4) == 0) {
        ثابت u8* داده   = عدم;
        u32       اندازه   = 0;
        M3_نتیجه  نتیجه = پیدا_کن_Embedded_Snapshot((ثابت u8*)i_بایتها, (ثابت u8*)i_بایتها + i_اندازه,
                                                "snapshot", عدم, عدم, &داده, &اندازه);
        إذا (نتیجه) {
            أرجع نتیجه;
        }
        إذا (not داده) {
            أرجع "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_میانگیر_Reader br = { داده, اندازه, 0 };
            أرجع بارگیری_Snapshot(io_runtime, i_module, میانگیر_Reader_Read, &br, true);
        }
    }
    {
        M3_میانگیر_Reader br = { (ثابت u8*)i_بایتها, i_اندازه, 0 };
        أرجع بارگیری_Snapshot(io_runtime, i_module, میانگیر_Reader_Read, &br,
                            i_اندازه < 4 or memcmp(i_بایتها, c_snapshotMagic, 4) != 0);
    }
#وإلا
    (فراغ)i_اندازه;
    أرجع "snapshots are not available in this build of Wasm3";
#پایان_اگر
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
#پایان_اگر
}

M3_نتیجه m3_GetEmbeddedSnapshot (IM3Module i_module, ثابت حرف* i_name,
                                 ثابت فراغ** o_بایتها, size_t* o_اندازه)
{
    إذا (not i_module or not o_بایتها or not o_اندازه) {
        أرجع m3Err_mallocFailed;
    }

    *o_بایتها = عدم;
    *o_اندازه  = 0;

#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_نتیجه            نتیجه = ModuleSnapshot(i_module, i_name, &snap);

        إذا (نتیجه) {
            أرجع نتیجه;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        *o_بایتها = snap->داده;
        *o_اندازه  = snap->اندازه;

        أرجع m3Err_none;
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#پایان_اگر
}

M3_نتیجه m3_بارگیری_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name)
{
    إذا (not io_runtime or not i_module) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_نتیجه            نتیجه = ModuleSnapshot(i_module, i_name, &snap);
        M3_میانگیر_Reader      br;

        إذا (نتیجه) {
            أرجع نتیجه;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        br.میانگیر = snap->داده;
        br.اندازه   = snap->اندازه;
        br.cursor = 0;

        أرجع بارگیری_Snapshot(io_runtime, i_module, میانگیر_Reader_Read, &br, true);
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#پایان_اگر
}


M3_نتیجه m3_Save_Snapshot_مقصد_Module (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name,
                                  فراغ** o_بایتها, size_t* o_اندازه)
{
#إذا d_m3HasSnapshots
    M3_نتیجه       نتیجه = m3Err_none;
    M3_میانگیر_Writer bw     = { عدم, 0, 0 };
    u8*            بیرون    = عدم;

    حرف      secName[d_m3_Max_Snapshot_Name_درازی + حجم("snapshot.")];
    u32       nameLen, name_Len_Leb_اندازه, payload_اندازه, payload_اندازه_Leb_اندازه;
    size_t    orig_اندازه, مجموع_اندازه;
    ثابت u8* drop_شروع = عدم;
    ثابت u8* drop_انتها   = عدم;
    IM3Module saved     = عدم;
    u8*       p;

    إذا (not io_runtime or not i_module or not o_بایتها or not o_اندازه) {
        أرجع m3Err_mallocFailed;
    }

    *o_بایتها = عدم;
    *o_اندازه  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_شروع or i_module->wasm_انتها <= i_module->wasm_شروع);

_   (SnapshotSectionName(i_name, secName, حجم(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->ریشه_Continuation or io_runtime->ریشه_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, میانگیر_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.اندازه);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.اندازه > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (پیدا_کن_Embedded_Snapshot(i_module->wasm_شروع, i_module->wasm_انتها, secName, &drop_شروع, &drop_انتها, عدم, عدم));

    orig_اندازه           = (size_t)(i_module->wasm_انتها - i_module->wasm_شروع) - (drop_شروع ? (size_t)(drop_انتها - drop_شروع) : 0);
    nameLen            = (u32)طول_رشته(secName);
    name_Len_Leb_اندازه     = اندازه_از_LEB_u32(nameLen);
    payload_اندازه        = name_Len_Leb_اندازه + nameLen + (u32)bw.اندازه;
    payload_اندازه_Leb_اندازه = اندازه_از_LEB_u32(payload_اندازه);
    مجموع_اندازه          = orig_اندازه + 1 + payload_اندازه_Leb_اندازه + payload_اندازه;

    بیرون = (u8*)m3_Malloc("wasm_snapshot_module", مجموع_اندازه);
    _throwifnull(بیرون);

    p = بیرون;

    إذا (drop_شروع) {
        size_t before = (size_t)(drop_شروع - i_module->wasm_شروع);
        size_t after  = (size_t)(i_module->wasm_انتها - drop_انتها);

        کپی_حافظه(p, i_module->wasm_شروع, before);
        p += before;
        کپی_حافظه(p, drop_انتها, after);
        p += after;
    } وإلا {
        کپی_حافظه(p, i_module->wasm_شروع, orig_اندازه);
        p += orig_اندازه;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_اندازه);
    p    = EmitLEB_u32(p, nameLen);
    کپی_حافظه(p, secName, nameLen);
    p += nameLen;
    کپی_حافظه(p, bw.میانگیر, bw.اندازه);

    *o_بایتها = بیرون;
    *o_اندازه  = مجموع_اندازه;
    بیرون      = عدم;

_catch:
    m3_Free(bw.میانگیر);
    m3_Free(بیرون);

    أرجع نتیجه;
#وإلا
    (فراغ)io_runtime;
    (فراغ)i_module;
    (فراغ)i_name;
    (فراغ)o_بایتها;
    (فراغ)o_اندازه;
    أرجع "snapshots are not available in this build of Wasm3";
#پایان_اگر
}
