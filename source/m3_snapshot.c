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
#  define d_m3_Max_Snapshot_Name_اوږدتیا   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
تعداد {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_پای_ته_رسېدلی  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_پای_ټکی                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//بې_اغېزه_عنصر
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ساكن
M3_پایله ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_پایله پایله = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_پیل or i_module->wasm_پای_ټکی - i_module->wasm_پیل < 8);

    {
        بایټونه_t  pos = i_module->wasm_پیل + 8;
        cbytes_t پای_ټکی = i_module->wasm_پای_ټکی;

        Xxh64_Init(&hash, 0);

        طالما (pos < پای_ټکی) {
            بایټونه_t پیل     = pos;
            u8      sectionId = *pos++;
            u32     اندازه      = 0;

_           (ReadLEB_u32(&اندازه, &pos, پای_ټکی));
            _throwif(m3Err_wasmUnderrun, اندازه > (size_t)(پای_ټکی - pos));

            pos += اندازه;
            إذا (sectionId != 0) {
                Xxh64_Update(&hash, پیل, (size_t)(pos - پیل));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    أرجع پایله;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ساكن
ثابت M3_خوندي_ټکی* پیدا_کړه_خوندي_ټکی (IM3Function i_function, pc_t i_pc, u8 i_ډول)
{
    ثابت M3SnapshotMap* map = i_function->snapshotMap;

    إذا (map) {
        لكل (u32 i = 0; i < map->عدد_خوندي_Points; ++i) {
            إذا (map->خوندي_Points[i].pc == i_pc and map->خوندي_Points[i].ډول == i_ډول) {
                أرجع &map->خوندي_Points[i];
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
ثابت M3_خوندي_ټکی* پیدا_کړه_خوندي_ټکی_At (ثابت M3SnapshotMap* i_map, u32 i_wasm_دننه_واټن, u8 i_ډول, u32 i_موخه_Loop)
{
    u32 ټیټ  = 0;
    u32 لوړ = i_map->عدد_خوندي_Points;

    طالما (ټیټ < لوړ) {
        u32 منځ = ټیټ + (لوړ - ټیټ) / 2;

        إذا (i_map->خوندي_Points[منځ].wasm_دننه_واټن < i_wasm_دننه_واټن) {
            ټیټ = منځ + 1;
        } وإلا {
            لوړ = منځ;
        }
    }

    لكل (u32 i = ټیټ; i < i_map->عدد_خوندي_Points and i_map->خوندي_Points[i].wasm_دننه_واټن == i_wasm_دننه_واټن; ++i) {
        ثابت M3_خوندي_ټکی* ټکی = &i_map->خوندي_Points[i];

        إذا (ټکی->ډول != i_ډول or (ټکی->flags & d_m3_خوندي_ټکی_لکۍ_Call)) {
            استمر;
        }
        إذا (i_ډول != safepoint_op or i_map->بلاکونه[ټکی->aux].wasm_دننه_واټن == i_موخه_Loop) {
            أرجع ټکی;
        }
    }

    أرجع عدم;
}

ساكن
ثابت M3_بلاک_پیل* پیدا_کړه_بلاک_At (ثابت M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_دننه_واټن)
{
    لكل (u32 i = 0; i < i_map->عدد_بلاکونه; ++i) {
        إذا (i_map->بلاکونه[i].opcode == i_opcode and i_map->بلاکونه[i].wasm_دننه_واټن == i_wasm_دننه_واټن) {
            أرجع &i_map->بلاکونه[i];
        }
    }

    أرجع عدم;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ساكن مضمن
bool Encloses_دننه_واټن (ثابت M3_بلاک_پیل* i_بلاک, u32 i_wasm_دننه_واټن)
{
    أرجع i_بلاک->wasm_دننه_واټن < i_wasm_دننه_واټن and i_wasm_دننه_واټن < i_بلاک->wasm_پای_ټکی;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ساكن مضمن
bool Handlers_ژوندی_At (ثابت M3SnapshotMap* i_map, ثابت M3_خوندي_ټکی* i_ټکی, ثابت M3_بلاک_پیل* i_try)
{
    أرجع not (i_ټکی->ډول == safepoint_op and i_try->wasm_دننه_واټن > i_map->بلاکونه[i_ټکی->aux].wasm_دننه_واټن);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ساكن
M3_پایله CallSiteType (IM3Function i_caller, ثابت M3_خوندي_ټکی* i_ټکی, IM3FuncType* o_type)
{
    M3_پایله   پایله = m3Err_none;
    IM3Module  module = i_caller->module;
    بایټونه_t    pos    = i_caller->wasm;
    cbytes_t   پای_ټکی    = i_caller->wasm_پای_ټکی;
    u32        اندازه   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = عدم;

_   (ReadLEB_u32(&اندازه, &pos, پای_ټکی));
    _throwif(m3Err_wasmUnderrun, i_ټکی->wasm_دننه_واټن >= (size_t)(پای_ټکی - pos));
    pos += i_ټکی->wasm_دننه_واټن;

_   (Read_opcode(&opcode, &pos, پای_ټکی));
_   (ReadLEB_u32(&index, &pos, پای_ټکی));

    إذا (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->عدد_Functions);
        *o_type = module->functions[index].funcType;
    } وإلا {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_غیرمستقیم and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->عدد_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    أرجع پایله;
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
        إذا (not Is_Sub_Type_د(Get_Func_Type_پایله_Type(i_type, i), Get_Func_Type_پایله_Type(i_expected, i))) {
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
تعريفنوع بنية M3_چوکاټ_Registers {
    m3reg_t* r0;
#  إذا d_m3HasFloat
    f64* fp0;
#  endif
} M3_چوکاټ_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
ساكن
M3_پایله Continuation_ډېری (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_بنسټ, u32* o_عدد_Slots)
{
    إذا (i_cont == i_runtime->ریښه_Continuation) {
        *o_بنسټ     = (m3slot_t*)i_runtime->origin_ډېری;
        *o_عدد_Slots = i_runtime->عدد_ډېری_Slots;
    } وإلا {
        *o_بنسټ     = i_cont->val_ډېری;
        *o_عدد_Slots = i_cont->عدد_ډېری_Slots;
    }

    أرجع (*o_بنسټ) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ساكن
M3_پایله Locate_ارزښت (m3slot_t* i_بنسټ, u32 i_عدد_Slots, m3stack_t i_sp, ثابت M3_خونه_ارزښت* i_ارزښت, u8** o_where)
{
    M3_پایله پایله = m3Err_none;
    u8       type   = بنسټ_Type_د(i_ارزښت->type);

    *o_where = عدم;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_ثبتوونکی_خونه_Alias(i_ارزښت->خونه));

    إذا (not Is_ثبتوونکی_خونه_Alias(i_ارزښت->خونه)) {
        size_t اندازه   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? حجم(فراغ*)
                                                                   : (Is64_بټ_Type(type) ? حجم(u64) : حجم(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_بنسټ + i_ارزښت->خونه) * حجم(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + اندازه > (size_t)i_عدد_Slots * حجم(m3slot_t));

        *o_where = (u8*)i_بنسټ + offset;
    }

_catch:
    أرجع پایله;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ساكن
M3_پایله Read_ارزښت (u8* i_where, ثابت M3_چوکاټ_Registers* i_registers, ثابت M3_خونه_ارزښت* i_ارزښت, u64* o_بټونه,
                    فراغ** o_reference)
{
    M3_پایله پایله = m3Err_none;
    u8       type   = بنسټ_Type_د(i_ارزښت->type);

    *o_بټونه      = 0;
    *o_reference = عدم;

    إذا (i_where) {
        إذا (IsRefType(type)) {
            حافظه_نقل(o_reference, i_where, حجم(فراغ*));
        } وإلا إذا (Is64_بټ_Type(type)) {
            حافظه_نقل(o_بټونه, i_where, حجم(u64));
        } وإلا {
            u32 narrow;
            حافظه_نقل(&narrow, i_where, حجم(narrow));
            *o_بټونه = narrow;
        }
    } وإلا إذا (Is_Fp_ثبتوونکی_خونه_Alias(i_ارزښت->خونه)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 بټونه;
            حافظه_نقل(&بټونه, &narrow, حجم(بټونه));
            *o_بټونه = بټونه;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            حافظه_نقل(o_بټونه, i_registers->fp0, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        إذا (IsRefType(type)) {
            *o_reference = (فراغ*)(uintptr_t)r0;
        } وإلا إذا (Is64_بټ_Type(type)) {
            *o_بټونه = (u64)r0;
        } وإلا {
            *o_بټونه = (u32)r0;
        }
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله Write_ارزښت (u8* o_where, ثابت M3_چوکاټ_Registers* i_registers, ثابت M3_خونه_ارزښت* i_ارزښت, u64 i_بټونه,
                     فراغ* i_reference)
{
    M3_پایله پایله = m3Err_none;
    u8       type   = بنسټ_Type_د(i_ارزښت->type);

    إذا (o_where) {
        إذا (IsRefType(type)) {
            حافظه_نقل(o_where, &i_reference, حجم(فراغ*));
        } وإلا إذا (Is64_بټ_Type(type)) {
            حافظه_نقل(o_where, &i_بټونه, حجم(u64));
        } وإلا {
            u32 narrow = (u32)i_بټونه;
            حافظه_نقل(o_where, &narrow, حجم(narrow));
        }
    } وإلا إذا (Is_Fp_ثبتوونکی_خونه_Alias(i_ارزښت->خونه)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            u32 بټونه = (u32)i_بټونه;
            f32 narrow;
            حافظه_نقل(&narrow, &بټونه, حجم(narrow));
            *i_registers->fp0 = narrow;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            حافظه_نقل(i_registers->fp0, &i_بټونه, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        إذا (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } وإلا {
            *i_registers->r0 = (m3reg_t)i_بټونه;
        }
    }

_catch:
    أرجع پایله;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ساكن
ثابت M3_خونه_ارزښت* چوکاټ_ارزښت (ثابت M3SnapshotMap* i_map, ثابت M3_خوندي_ټکی* i_ټکی, u32 i_شاخص)
{
    إذا (i_شاخص < i_map->عدد_Locals) {
        أرجع i_ټکی->لومړی_Local_ارزښت == UINT32_MAX ? &i_map->locals[i_شاخص]
                                                      : &i_map->ارزښتونه[i_ټکی->لومړی_Local_ارزښت + i_شاخص];
    }
    أرجع &i_map->ارزښتونه[i_ټکی->لومړی_ارزښت + i_شاخص - i_map->عدد_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ساكن
فراغ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    إذا (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->عدد_Ret_And_Arg_Slots) + i_function->عدد_Local_بایټونه;

        حافظه_نقل(where, i_function->constants, i_function->عدد_Constant_بایټونه);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
تعريفنوع بنية M3_اشاره_ګر_Ids {
    فراغ** توکي;
    u32    شمېر;
    u32    ظرفیت;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  شاخص_ظرفیت;       // a power of two, kept at least twice count
} M3_اشاره_ګر_Ids;

ساكن
u32 اشاره_ګر_لوښی (فراغ* i_اشاره_ګر, u32 i_ظرفیت)
{
    u64 بټونه = (u64)(uintptr_t)i_اشاره_ګر;

    أرجع (u32)((بټونه * 0x9E3779B97F4A7C15ULL) >> 32) & (i_ظرفیت - 1);
}

ساكن
u32 اشاره_ګر_Ids_پیدا_کړه (ثابت M3_اشاره_ګر_Ids* i_ids, فراغ* i_اشاره_ګر)
{
    إذا (i_ids->شاخص_ظرفیت) {
        u32 لوښی = اشاره_ګر_لوښی(i_اشاره_ګر, i_ids->شاخص_ظرفیت);

        طالما (i_ids->index[لوښی]) {
            u32 id = i_ids->index[لوښی] - 1;

            إذا (i_ids->توکي[id] == i_اشاره_ګر) {
                أرجع id;
            }

            لوښی = (لوښی + 1) & (i_ids->شاخص_ظرفیت - 1);
        }
    }

    أرجع d_m3SnapshotNone;
}

ساكن
M3_پایله اشاره_ګر_Ids_جمع_کړه (M3_اشاره_ګر_Ids* io_ids, فراغ* i_اشاره_ګر, u32* o_id)
{
    u32 id = اشاره_ګر_Ids_پیدا_کړه(io_ids, i_اشاره_ګر);

    إذا (id != d_m3SnapshotNone) {
        *o_id = id;
        أرجع m3Err_none;
    }

    إذا (io_ids->شمېر == io_ids->ظرفیت) {
        u32    ظرفیت = io_ids->ظرفیت ? io_ids->ظرفیت * 2 : 16;
        فراغ** توکي    = m3_ReallocArray(فراغ*, io_ids->توکي, ظرفیت, io_ids->ظرفیت);

        إذا (not توکي) {
            أرجع m3Err_mallocFailed;
        }

        io_ids->توکي    = توکي;
        io_ids->ظرفیت = ظرفیت;
    }

    إذا ((io_ids->شمېر + 1) * 2 > io_ids->شاخص_ظرفیت) {
        u32  ظرفیت = io_ids->شاخص_ظرفیت ? io_ids->شاخص_ظرفیت * 2 : 32;
        u32* index    = m3_AllocArray(u32, ظرفیت);

        إذا (not index) {
            أرجع m3Err_mallocFailed;
        }

        لكل (u32 i = 0; i < io_ids->شمېر; ++i) {
            u32 لوښی = اشاره_ګر_لوښی(io_ids->توکي[i], ظرفیت);

            طالما (index[لوښی]) {
                لوښی = (لوښی + 1) & (ظرفیت - 1);
            }

            index[لوښی] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->شاخص_ظرفیت = ظرفیت;
    }

    id                = io_ids->شمېر++;
    io_ids->توکي[id] = i_اشاره_ګر;

    u32 لوښی = اشاره_ګر_لوښی(i_اشاره_ګر, io_ids->شاخص_ظرفیت);

    طالما (io_ids->index[لوښی]) {
        لوښی = (لوښی + 1) & (io_ids->شاخص_ظرفیت - 1);
    }

    io_ids->index[لوښی] = id + 1;

    *o_id = id;

    أرجع m3Err_none;
}

ساكن
فراغ اشاره_ګر_Ids_Free (M3_اشاره_ګر_Ids* io_ids)
{
    m3_Free(io_ids->توکي);
    m3_Free(io_ids->index);
    memset(io_ids, 0, حجم(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3_بفر_Writer {
    u8*    بفر;
    size_t اندازه;
    size_t ظرفیت;
} M3_بفر_Writer;

ساكن
M3_پایله بفر_Writer_Write (ثابت فراغ* i_معلومات, size_t i_اندازه, فراغ* i_userdata)
{
    M3_بفر_Writer* bw = (M3_بفر_Writer*)i_userdata;

    إذا (bw->اندازه + i_اندازه > bw->ظرفیت) {
        size_t newCap = bw->ظرفیت ? bw->ظرفیت * 2 : 4096;
        طالما (bw->اندازه + i_اندازه > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->بفر, newCap, bw->ظرفیت);
        إذا (!newBuf) {
            أرجع m3Err_mallocFailed;
        }
        bw->بفر   = newBuf;
        bw->ظرفیت = newCap;
    }

    إذا (i_اندازه) {
        حافظه_نقل(bw->بفر + bw->اندازه, i_معلومات, i_اندازه);
        bw->اندازه += i_اندازه;
    }

    أرجع m3Err_none;
}

تعريفنوع بنية M3_بفر_Reader {
    ثابت u8* بفر;
    size_t    اندازه;
    size_t    cursor;
} M3_بفر_Reader;

ساكن
M3_پایله بفر_Reader_Read (فراغ* o_بفر, size_t i_اندازه, فراغ* i_userdata)
{
    M3_بفر_Reader* br = (M3_بفر_Reader*)i_userdata;

    إذا (i_اندازه > br->اندازه - br->cursor) {
        أرجع m3Err_wasmMalformed;
    }

    إذا (i_اندازه) {
        حافظه_نقل(o_بفر, br->بفر + br->cursor, i_اندازه);
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

    M3_اشاره_ګر_Ids ژوندی_Continuations;     // everything the runtime still holds, to check
    M3_اشاره_ګر_Ids ژوندی_Exceptions;        //   a reference against before following it
    M3_اشاره_ګر_Ids continuations;         // what the snapshot carries, by id
    M3_اشاره_ګر_Ids exceptions;
} M3SnapshotSave;

ساكن مضمن
u8 M3_Type_منزل_Val_Type (m3type_t i_type)
{
    بدل (بنسټ_Type_د(i_type)) {
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
M3_پایله Put (M3SnapshotSave* s, ثابت فراغ* i_معلومات, size_t i_اندازه)
{
    أرجع s->writing ? s->writer(i_معلومات, i_اندازه, s->userdata) : m3Err_none;
}

ساكن
M3_پایله PutU8 (M3SnapshotSave* s, u8 i_ارزښت)
{
    أرجع Put(s, &i_ارزښت, حجم(i_ارزښت));
}


ساكن
M3_پایله PutLEB_u32 (M3SnapshotSave* s, u32 i_ارزښت)
{
    u8  بایټونه[5];
    u32 len = 0;

    افعل {
        u8 byte = i_ارزښت & 0x7F;
        i_ارزښت >>= 7;
        إذا (i_ارزښت != 0) {
            byte |= 0x80;
        }
        بایټونه[len++] = byte;
    } طالما (i_ارزښت != 0);

    أرجع Put(s, بایټونه, len);
}

ساكن
M3_پایله PutLEB_u64 (M3SnapshotSave* s, u64 i_ارزښت)
{
    u8  بایټونه[10];
    u32 len = 0;

    افعل {
        u8 byte = i_ارزښت & 0x7F;
        i_ارزښت >>= 7;
        إذا (i_ارزښت != 0) {
            byte |= 0x80;
        }
        بایټونه[len++] = byte;
    } طالما (i_ارزښت != 0);

    أرجع Put(s, بایټونه, len);
}

ساكن
M3_پایله PutLEB_i64 (M3SnapshotSave* s, i64 i_ارزښت)
{
    u8   بایټونه[10];
    u32  len  = 0;
    bool نور = true;

    طالما (نور) {
        u8 byte = i_ارزښت & 0x7F;
        i_ارزښت >>= 7;
        إذا ((i_ارزښت == 0 && !(byte & 0x40)) || (i_ارزښت == -1 && (byte & 0x40))) {
            نور = false;
        } وإلا {
            byte |= 0x80;
        }
        بایټونه[len++] = byte;
    }

    أرجع Put(s, بایټونه, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
تعريفنوع M3_پایله (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ساكن
M3_پایله SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_پایله         پایله   = m3Err_none;
    M3_بفر_Writer   bw       = { عدم, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    فراغ*            userdata = s->userdata;

    s->writer   = بفر_Writer_Write;
    s->userdata = &bw;

    پایله = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    إذا (پایله) {
        اذهب _catch;
    }

    // a section with nothing in it is left out
    إذا (bw.اندازه) {
        _throwif("snapshot section is too large", bw.اندازه > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.اندازه));
_       (Put(s, bw.بفر, bw.اندازه));
    }

_catch:
    m3_Free(bw.بفر);

    أرجع پایله;
}

ساكن
u32 Function_شاخص (IM3Module i_module, IM3Function i_function)
{
    إذا (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->عدد_Functions) {
        أرجع (u32)(i_function - i_module->functions);
    }

    أرجع d_m3SnapshotNone;
}

ساكن
u32 Func_Type_شاخص (IM3Module i_module, IM3FuncType i_type)
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
IM3_نښان نښان_د_شاخص (IM3Module i_module, u32 i_شاخص)
{
    IM3_نښان نښان = &i_module->نښانونه[i_شاخص];

    أرجع نښان->resolved ? نښان->resolved : نښان;
}

ساكن
u32 نښان_شاخص_2 (IM3Module i_module, IM3_نښان i_نښان)
{
    لكل (u32 i = 0; i < i_module->عدد_نښانونه; ++i) {
        إذا (نښان_د_شاخص(i_module, i) == i_نښان) {
            أرجع i;
        }
    }

    أرجع d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ساكن
M3_پایله NameReference (M3SnapshotSave* s, u8 i_type, فراغ* i_reference, u64* o_کلمه)
{
    M3_پایله پایله = m3Err_none;
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
                 اشاره_ګر_Ids_پیدا_کړه(&s->ژوندی_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 شمېر = s->exceptions.شمېر;
_       (اشاره_ګر_Ids_جمع_کړه(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.شمېر != شمېر);
    }
#  endif
    وإلا إذا (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 اشاره_ګر_Ids_پیدا_کړه(&s->ژوندی_Continuations, i_reference) == d_m3SnapshotNone);

        u32 شمېر = s->continuations.شمېر;
_       (اشاره_ګر_Ids_جمع_کړه(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.شمېر != شمېر);
    } وإلا {
        _throw("a reference of an unknown type");
    }

    *o_کلمه = id;

_catch:
    أرجع پایله;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ساكن
M3_پایله Put_Ref_ارزښت (M3SnapshotSave* s, u8 i_type, فراغ* i_reference)
{
    M3_پایله پایله = m3Err_none;
    u64      کلمه   = 0;

_   (NameReference(s, i_type, i_reference, &کلمه));

    إذا (not s->writing) {
        أرجع m3Err_none;
    }

    إذا (کلمه == d_m3SnapshotNullRef) {
_       (PutU8(s, 0x00));
    } وإلا {
        u8 ډول = 0;
        إذا (i_type == c_m3Type_funcref) {
            ډول = 0x01;
        } وإلا إذا (i_type == c_m3Type_externref) {
            ډول = 0x02;
        }
#  إذا d_m3HasExceptionHandling
        وإلا إذا (i_type == c_m3Type_exnref) {
            ډول = 0x03;
        }
#  endif
        وإلا إذا (i_type == c_m3Type_contref) {
            ډول = 0x04;
        }

_       (PutU8(s, ډول));
_       (PutLEB_u64(s, کلمه));
    }

_catch:
    أرجع پایله;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ساكن
M3_پایله PutLE32 (M3SnapshotSave* s, u32 i_ارزښت)
{
    u8 بایټونه[4];

    لكل (u32 i = 0; i < حجم(بایټونه); ++i) {
        بایټونه[i] = (u8)(i_ارزښت >> (8 * i));
    }

    أرجع Put(s, بایټونه, حجم(بایټونه));
}

ساكن
M3_پایله PutLE64 (M3SnapshotSave* s, u64 i_ارزښت)
{
    u8 بایټونه[8];

    لكل (u32 i = 0; i < حجم(بایټونه); ++i) {
        بایټونه[i] = (u8)(i_ارزښت >> (8 * i));
    }

    أرجع Put(s, بایټونه, حجم(بایټونه));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ساكن
M3_پایله Put_ارزښت_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_بټونه)
{
    إذا (IsRefType(i_type)) {
        أرجع Put_Ref_ارزښت(s, بنسټ_Type_د(i_type), (فراغ*)(uintptr_t)i_بټونه);
    }

    بدل (بنسټ_Type_د(i_type)) {
    حالة c_m3Type_i32: أرجع PutLEB_i64(s, (i32)(u32)i_بټونه);
    حالة c_m3Type_i64: أرجع PutLEB_i64(s, (i64)i_بټونه);
    حالة c_m3Type_f32: أرجع PutLE32(s, (u32)i_بټونه);
    حالة c_m3Type_f64: أرجع PutLE64(s, i_بټونه);
    افتراضي: أرجع "a snapshot cannot hold a value of this type as bits";
    }
}

ساكن
M3_پایله Put_ارزښت_نښان (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_منزل_Val_Type(i_type);

    إذا (not valtype) {
        أرجع "a snapshot has no tag for a value of this type";
    }

    أرجع PutU8(s, valtype);
}

ساكن
M3_پایله Put_ارزښت_بټونه (M3SnapshotSave* s, m3type_t i_type, u64 i_بټونه)
{
    M3_پایله پایله = m3Err_none;

_   (Put_ارزښت_نښان(s, i_type));
_   (Put_ارزښت_Body(s, i_type, i_بټونه));

_catch:
    أرجع پایله;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ساكن
M3_پایله Put_ارزښت (M3SnapshotSave* s, m3type_t i_type, ثابت فراغ* i_where)
{
    M3_پایله پایله = m3Err_none;

_   (Put_ارزښت_نښان(s, i_type));

    إذا (IsRefType(i_type)) {
        فراغ* reference;
        حافظه_نقل(&reference, i_where, حجم(reference));
_       (Put_ارزښت_Body(s, i_type, (u64)(uintptr_t)reference));
    } وإلا إذا (بنسټ_Type_د(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 بایټونه[16];
        حافظه_نقل(بایټونه, i_where, حجم(بایټونه));
_       (Put(s, بایټونه, حجم(بایټونه)));
    } وإلا إذا (Is64_بټ_Type(i_type)) {
        u64 بټونه;
        حافظه_نقل(&بټونه, i_where, حجم(بټونه));
_       (Put_ارزښت_Body(s, i_type, بټونه));
    } وإلا {
        u32 narrow;
        حافظه_نقل(&narrow, i_where, حجم(narrow));
_       (Put_ارزښت_Body(s, i_type, narrow));
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله Put_حافظه_Chunks (M3SnapshotSave* s, ثابت u8* i_بایټونه, size_t i_اندازه)
{
    M3_پایله پایله = m3Err_none;
    size_t   cursor = 0;

    طالما (cursor < i_اندازه) {
        u8     byte = i_بایټونه[cursor];
        size_t پرلپسې  = 1;

        طالما (cursor + پرلپسې < i_اندازه and i_بایټونه[cursor + پرلپسې] == byte) {
            پرلپسې++;
        }

        إذا ((byte == 0x00 or byte == 0xFF) and پرلپسې >= d_m3_Snapshot_پرلپسې_پوله) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            إذا (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)پرلپسې));
            }

            cursor += پرلپسې;
            استمر;
        }

        size_t raw_پیل = cursor;

        طالما (cursor < i_اندازه) {
            u8     b      = i_بایټونه[cursor];
            size_t اوږدتیا = 1;

            طالما (cursor + اوږدتیا < i_اندازه and i_بایټونه[cursor + اوږدتیا] == b) {
                اوږدتیا++;
            }

            إذا ((b == 0x00 or b == 0xFF) and اوږدتیا >= d_m3_Snapshot_پرلپسې_پوله) {
                اكسر;
            }

            cursor += اوږدتیا;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_پیل));
_       (PutLEB_u32(s, (u32)(cursor - raw_پیل)));
_       (Put(s, i_بایټونه + raw_پیل, cursor - raw_پیل));
    }

_   (PutU8(s, d_m3_Chunk_پای_ټکی));

_catch:
    أرجع پایله;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ساكن
u32 حافظه_لومړی_شاخص (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->memories[j] != module->memories[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
u32 جدول_لومړی_شاخص (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->tables[j] != module->tables[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
M3_پایله SaveMemories (M3SnapshotSave* s)
{
    M3_پایله  پایله = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Memories));

    لكل (u32 m = 0; m < module->عدد_Memories; ++m) {
        IM3_حافظه حافظه   = module->memories[m];
        u32       لومړی    = حافظه_لومړی_شاخص(module, m);
        bool      has_معلومات  = حافظه and حافظه->mallocated;
        u32       پاڼه_اندازه = حافظه ? حافظه_پاڼه_اندازه(حافظه) : d_m3_Default_Mem_پاڼه_اندازه;
        u64       بایټونه    = حافظه ? حافظه->عدد_پاڼې * (u64)پاڼه_اندازه : 0;
        u32       پاڼه_بټونه = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, لومړی));

        إذا (لومړی != m) {
            استمر;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", حافظه and حافظه->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", بایټونه > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 حافظه and حافظه->mallocated and حافظه->mallocated->اوږدتیا < بایټونه);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        طالما ((1u << پاڼه_بټونه) < پاڼه_اندازه) {
            ++پاڼه_بټونه;
        }

_       (PutLEB_u32(s, پاڼه_بټونه));
_       (PutLEB_u32(s, حافظه ? (u32)حافظه->عدد_پاڼې : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        إذا (not s->writing) {
            استمر;
        }

        إذا (has_معلومات) {
_           (Put_حافظه_Chunks(s, m3_Mem_معلومات(حافظه->mallocated), (size_t)بایټونه));
        } وإلا {
_           (PutU8(s, d_m3_Chunk_پای_ټکی));
        }
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله SaveGlobals (M3SnapshotSave* s)
{
    M3_پایله  پایله = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Globals));

    لكل (u32 g = 0; g < module->عدد_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_ارزښت(s, global->type, &cell->i64_ارزښت));
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله SaveTables (M3SnapshotSave* s)
{
    M3_پایله  پایله = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_Tables));

    لكل (u32 t = 0; t < module->عدد_Tables; ++t) {
        IM3_جدول جدول_2 = module->tables[t];
        u32      لومړی = جدول_لومړی_شاخص(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, لومړی));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        إذا (لومړی != t) {
            استمر;
        }

_       (PutU8(s, M3_Type_منزل_Val_Type(جدول_2->type)));
_       (PutLEB_u32(s, جدول_2->اندازه));

        لكل (u32 e = 0; e < جدول_2->اندازه; ++e) {
_           (Put_Ref_ارزښت(s, بنسټ_Type_د(جدول_2->type), جدول_2->elements[e]));
        }
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله SaveSegments (M3SnapshotSave* s)
{
    M3_پایله  پایله = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->عدد_معلومات_Segments));

    لكل (u32 i = 0; i < module->عدد_معلومات_Segments; ++i) {
_       (PutU8(s, module->معلومات_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->عدد_Element_Segments));

    لكل (u32 i = 0; i < module->عدد_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    أرجع پایله;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_پایله SaveExceptions (M3SnapshotSave* s)
{
    M3_پایله پایله = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.شمېر));

    لكل (u32 i = 0; i < s->exceptions.شمېر; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.توکي[i];
        IM3FuncType  type = exn->نښان->type;

        _throwif(m3Err_wasmMalformed, exn->عدد_دلیلونه != Get_Func_Type_عدد_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->عدد_دلیلونه));

        لكل (u32 a = 0; a < exn->عدد_دلیلونه; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_ارزښت_بټونه(s, argType, exn->دلیلونه[a]));
        }
    }

_catch:
    أرجع پایله;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
تعريفنوع بنية M3Activation {
    IM3Function        function;
    ثابت M3_خوندي_ټکی* ټکی;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 ډول;            // M3SafePointKind
    M3_چوکاټ_Registers   registers;       // the ones it goes on with
    ثابت M3_چوکاټ*     resume;          // the resume it waits in, for safepoint_resume
    i32                لومړی_چوکاټ;      // its frames, outermost first: firstFrame down to lastFrame
    i32                وروستی_چوکاټ;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ساكن
ثابت M3_خوندي_ټکی* پیدا_کړه_چوکاټ_خوندي_ټکی (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_ډول)
{
    ثابت M3_خوندي_ټکی* ټکی = *io_function ? پیدا_کړه_خوندي_ټکی(*io_function, i_pc, i_ډول) : عدم;

    لكل (u32 i = 0; not ټکی and i < i_module->عدد_Functions; ++i) {
        ټکی = پیدا_کړه_خوندي_ټکی(&i_module->functions[i], i_pc, i_ډول);
        إذا (ټکی) {
            *io_function = &i_module->functions[i];
        }
    }

    أرجع ټکی;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ساكن
M3_پایله بل_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_لومړی, M3Activation* o_act)
{
    M3_پایله پایله = m3Err_none;
    i32      i      = i_لومړی;

    memset(o_act, 0, حجم(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->لومړی_چوکاټ = i_لومړی;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    لكل (; i >= 0; --i) {
        ثابت M3_چوکاټ* چوکاټ = &i_cont->چوکاټونه[i];

        _throwif("a suspended frame runs against another module's memory", چوکاټ->حافظه != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", چوکاټ->sp != i_sp);

        إذا (چوکاټ->ډول == چوکاټ_call) {
            o_act->pc           = چوکاټ->pc;
            o_act->ډول         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&چوکاټ->call.r0;
#  إذا d_m3HasFloat
            o_act->registers.fp0 = (f64*)&چوکاټ->call.fp0;
#  endif
            اكسر;
        } وإلا إذا (چوکاټ->ډول == چوکاټ_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = چوکاټ->pc;
            o_act->ډول   = safepoint_resume;
            o_act->resume = چوکاټ;
            اكسر;
        }
    }

    إذا (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->ډول         = i_cont->suspend_ټکی;
        o_act->registers.r0 = &i_cont->r0;
#  إذا d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->وروستی_چوکاټ = i;
    o_act->ټکی     = پیدا_کړه_چوکاټ_خوندي_ټکی(i_module, &o_act->function, o_act->pc, o_act->ډول);

    _throwif("a suspended frame is not at a safepoint", not o_act->ټکی);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  إذا d_m3_ثبت_Keeps_چوکاټ
    لكل (i32 k = i_لومړی; k > o_act->وروستی_چوکاټ; --k) {
        ثابت M3_چوکاټ* چوکاټ = &i_cont->چوکاټونه[k];
        _throwif("a suspended frame is not the function it is entered with",
                 چوکاټ->ډول == چوکاټ_ثبت and چوکاټ->ثبت.function != o_act->function);
    }
#  endif

_catch:
    أرجع پایله;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ساكن
M3_پایله SaveActivation (M3SnapshotSave* s, m3slot_t* i_بنسټ, u32 i_عدد_Slots, ثابت M3Activation* i_act)
{
    M3_پایله             پایله        = m3Err_none;
    IM3Function          function      = i_act->function;
    ثابت M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_شاخص = Function_شاخص(s->module, function);
    ثابت M3_خوندي_ټکی*   ټکی         = i_act->ټکی;
    u32                  عدد_ارزښتونه;

    _throwif("a suspended frame belongs to another module", function_شاخص == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_شاخص));
_   (PutU8(s, ټکی->ډول));
_   (PutLEB_u32(s, ټکی->wasm_دننه_واټن));

    إذا (ټکی->ډول == safepoint_op) {
_       (PutLEB_u32(s, map->بلاکونه[ټکی->aux].wasm_دننه_واټن));
    }

    عدد_ارزښتونه = map->عدد_Locals + ټکی->عدد_ارزښتونه;

_   (PutLEB_u32(s, عدد_ارزښتونه));

    لكل (u32 v = 0; v < عدد_ارزښتونه; ++v) {
        ثابت M3_خونه_ارزښت* ارزښت = چوکاټ_ارزښت(map, ټکی, v);
        u8*                where;
        u64                کلمه;
        فراغ*              reference;

_       (Locate_ارزښت(i_بنسټ, i_عدد_Slots, i_act->sp, ارزښت, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (بنسټ_Type_د(ارزښت->type) == c_m3Type_v128) {
_           (Put_ارزښت_نښان(s, ارزښت->type));
_           (Put(s, where, 16));
            استمر;
        }

_       (Read_ارزښت(where, &i_act->registers, ارزښت, &کلمه, &reference));

_       (Put_ارزښت_بټونه(s, ارزښت->type, IsRefType(ارزښت->type) ? (u64)(uintptr_t)reference : کلمه));
    }

    إذا (ټکی->ډول == safepoint_resume) {
_       (Put_Ref_ارزښت(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    أرجع پایله;
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
M3_پایله SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_پایله      پایله   = m3Err_none;
    M3Activation* acts     = عدم;
    u32           عدد_Acts  = 0;
    IM3Function   function = i_cont->ثبت_Function;
    i32           لومړی    = (i32)i_cont->عدد_چوکاټونه - 1;
    m3slot_t*     بنسټ;
    u32           عدد_Slots;
    m3stack_t     sp;

_   (Continuation_ډېری(s->runtime, i_cont, &بنسټ, &عدد_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->عدد_چوکاټونه + 1);
    _throwifnull(acts);

    sp = بنسټ;

    طالما (true) {
        M3Activation* act = &acts[عدد_Acts];

_       (بل_Activation(s->module, i_cont, function, sp, لومړی, act));

        إذا (act->ډول != safepoint_call) {
            عدد_Acts++;
            اكسر;
        }

        _throwif("a suspended continuation stops at a call", act->وروستی_چوکاټ < 0);

        إذا (not (act->ټکی->flags & d_m3_خوندي_ټکی_لکۍ_Call)) {
            عدد_Acts++;
        }

        function = i_cont->چوکاټونه[act->وروستی_چوکاټ].call.function;
        sp       = act->sp + act->ټکی->aux;
        لومړی    = act->وروستی_چوکاټ - 1;
    }

_   (PutLEB_u32(s, عدد_Acts));

    لكل (u32 a = 0; a < عدد_Acts; ++a) {
_       (SaveActivation(s, بنسټ, عدد_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    أرجع پایله;
}

ساكن
M3_پایله SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_پایله  پایله    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_ریښه    = (i_cont == s->runtime->ریښه_Continuation);
    u8        state     = snapshot_cont_پای_ته_رسېدلی;
    u32       type_شاخص = is_ریښه ? d_m3SnapshotNone : Func_Type_شاخص(module, i_cont->type);

    إذا (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } وإلا إذا (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } وإلا إذا (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->ثبت_Function);
    _throwif("a continuation belongs to another module", Function_شاخص(module, i_cont->ثبت_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_ریښه and type_شاخص == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_ریښه));
_   (PutLEB_u32(s, type_شاخص));
_   (PutLEB_u32(s, Function_شاخص(module, i_cont->ثبت_Function)));
#  إذا d_m3HasExceptionHandling
_   (Put_Ref_ارزښت(s, c_m3Type_exnref, i_cont->resumeThrow));
#  وإلا
_   (Put_Ref_ارزښت(s, c_m3Type_exnref, عدم));
#  endif

    // a postmortem has nothing to resume, so it does not say how
    إذا (s->postmortem) {
        اذهب _catch;
    }

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner   = i_cont->ثبت_Function ? i_cont->ثبت_Function->funcType : عدم;
        u16         عدد_Rets = inner ? Get_Func_Type_عدد_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_دلیلونه_شمېر > Get_Func_Type_عدد_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_دلیلونه_شمېر));

        لكل (u32 i = 0; i < i_cont->bound_دلیلونه_شمېر; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_ارزښت(s, type, i_cont->val_ډېری + (عدد_Rets + i) * c_io_خونه_شمېر));
        }
    } وإلا إذا (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_دلیلونه_شمېر));
_       (SaveSuspendedBody(s, i_cont));
    } وإلا {
_       (PutLEB_u32(s, 0));
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله SaveContinuations (M3SnapshotSave* s)
{
    M3_پایله پایله = m3Err_none;

_   (PutLEB_u32(s, s->continuations.شمېر));

    لكل (u32 i = 0; i < s->continuations.شمېر; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.توکي[i]));
    }

_catch:
    أرجع پایله;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ساكن
M3_پایله SaveHostState (M3SnapshotSave* s)
{
    M3_پایله               پایله = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_بفر_Writer         bw     = { عدم, 0, 0 };

    إذا (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, بفر_Writer_Write, &bw));
    }

    إذا (bw.اندازه > 0) {
_       (Put(s, bw.بفر, bw.اندازه));
    }

_catch:
    m3_Free(bw.بفر);

    أرجع پایله;
}

ساكن
فراغ SnapshotResourceTotals (IM3Module module, u64* حافظه_بایټونه, u64* جدول_Elements)
{
    *حافظه_بایټونه = *جدول_Elements = 0;
    لكل (u32 i = 0; i < module->عدد_Memories; ++i) {
        IM3_حافظه حافظه = module->memories[i];
        إذا (حافظه_لومړی_شاخص(module, i) == i and حافظه) {
            *حافظه_بایټونه += حافظه->عدد_پاڼې * (u64)حافظه_پاڼه_اندازه(حافظه);
        }
    }
    لكل (u32 i = 0; i < module->عدد_Tables; ++i) {
        IM3_جدول جدول_2 = module->tables[i];
        إذا (جدول_لومړی_شاخص(module, i) == i and جدول_2) {
            *جدول_Elements += جدول_2->اندازه;
        }
    }
}

ساكن
M3_پایله SaveMeta (M3SnapshotSave* s)
{
    M3_پایله پایله = m3Err_none;
    u64      hash   = 0;
    u64      حافظه_بایټونه, جدول_Elements;
    u32      فعال_Stacks = 0;
    لكل (u32 i = 0; i < s->continuations.شمېر; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.توکي[i];
        إذا (cont != s->runtime->ریښه_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            فعال_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &حافظه_بایټونه, &جدول_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_وخت_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.شمېر));
_   (PutLEB_u32(s, s->exceptions.شمېر));
_   (PutLEB_u64(s, حافظه_بایټونه));
_   (PutLEB_u64(s, جدول_Elements));
_   (PutLEB_u32(s, فعال_Stacks));

#  إذا d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    لكل (u32 i = 0; i < s->exceptions.شمېر; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.توکي[i];
        u32          نښان_شاخص  = نښان_شاخص_2(s->module, exception->نښان);

        _throwif("an exception was thrown with another module's tag", نښان_شاخص == d_m3SnapshotNone);

_       (PutLEB_u32(s, نښان_شاخص));
    }
#  endif

_catch:
    أرجع پایله;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ساكن
M3_پایله DiscoverReferences (M3SnapshotSave* s)
{
    M3_پایله پایله            = m3Err_none;
    u32      بشپړ_Continuations = 0;
    u32      بشپړ_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    طالما (بشپړ_Continuations < s->continuations.شمېر or بشپړ_Exceptions < s->exceptions.شمېر) {
#  إذا d_m3HasExceptionHandling
        طالما (بشپړ_Exceptions < s->exceptions.شمېر) {
            M3Exception* exception = (M3Exception*)s->exceptions.توکي[بشپړ_Exceptions++];
            IM3FuncType  type      = exception->نښان->type;

            _throwif(m3Err_wasmMalformed, exception->عدد_دلیلونه != Get_Func_Type_عدد_Params(type));

            لكل (u32 a = 0; a < exception->عدد_دلیلونه; ++a) {
_               (Put_ارزښت_بټونه(s, GetFuncTypeParamType(type, (u16)a), exception->دلیلونه[a]));
            }
        }
#  endif
        طالما (بشپړ_Continuations < s->continuations.شمېر and بشپړ_Exceptions == s->exceptions.شمېر) {
            u32 id = بشپړ_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.توکي[id]));
        }
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله SaveSections (M3SnapshotSave* s)
{
    M3_پایله پایله = m3Err_none;

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

    إذا (s->module->عدد_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_حافظه, SaveMemories));
    }
    إذا (s->module->عدد_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_جدول, SaveTables));
    }
    إذا (s->module->عدد_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    إذا (s->module->عدد_معلومات_Segments or s->module->عدد_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  إذا d_m3HasExceptionHandling
    إذا (s->exceptions.شمېر) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    إذا (s->continuations.شمېر) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    أرجع پایله;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3SnapshotSuspendSite {
    ثابت M3SnapshotMap* map;
    ثابت M3_خوندي_ټکی*   ټکی;
    u32                  موخه;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

تعريفنوع بنية M3_Snapshot_بارول {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    فراغ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ډېری;
    bool      بدل_شوی;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    عدد_Continuations;
    u64                    required_حافظه_بایټونه;
    u64                    required_جدول_Elements;
    u64                    loaded_حافظه_بایټونه;
    u64                    loaded_جدول_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  إذا d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 عدد_Exceptions;

    M3_چوکاټ* چوکاټونه;           // a suspended continuation's frames as they are put together, outermost first

    بایټونه_t pos;               // bounded section payload, read using the Wasm parser's helpers
    بایټونه_t پای_ټکی;
    bool    دننه_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    کتل_شوی_Sections[256];
} M3_Snapshot_بارول;

ساكن
M3_پایله Get (M3_Snapshot_بارول* l, فراغ* o_معلومات, size_t i_اندازه)
{
    إذا (not l->دننه_Section) {
        أرجع l->reader(o_معلومات, i_اندازه, l->userdata);
    }
    إذا (i_اندازه > (size_t)(l->پای_ټکی - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    إذا (i_اندازه) {
        حافظه_نقل(o_معلومات, l->pos, i_اندازه);
        l->pos += i_اندازه;
    }
    أرجع m3Err_none;
}

ساكن
M3_پایله GetU8 (M3_Snapshot_بارول* l, u8* o_ارزښت)
{
    أرجع Get(l, o_ارزښت, حجم(*o_ارزښت));
}

ساكن
M3_پایله GetBool (M3_Snapshot_بارول* l, u8* o_ارزښت)
{
    M3_پایله پایله = GetU8(l, o_ارزښت);
    أرجع پایله ? پایله : (*o_ارزښت <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ساكن
M3_پایله GetLEB_u32 (M3_Snapshot_بارول* l, u32* o_ارزښت)
{
    إذا (l->دننه_Section) {
        أرجع ReadLEB_u32(o_ارزښت, &l->pos, l->پای_ټکی);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    لكل (u32 i = 0; i < حجم(encoded); ++i) {
        M3_پایله پایله = GetU8(l, &encoded[i]);
        إذا (پایله) {
            أرجع پایله;
        }
        إذا (not (encoded[i] & 0x80)) {
            بایټونه_t pos = encoded;
            أرجع ReadLEB_u32(o_ارزښت, &pos, encoded + i + 1);
        }
    }
    أرجع m3_Err_leb_ډکېدنه;
}

ساكن
M3_پایله GetLEB_u64 (M3_Snapshot_بارول* l, u64* o_ارزښت)
{
    أرجع ReadLebUnsigned(o_ارزښت, 64, &l->pos, l->پای_ټکی);
}

ساكن
M3_پایله GetLEB_i64 (M3_Snapshot_بارول* l, i64* o_ارزښت)
{
    أرجع ReadLEB_i64(o_ارزښت, &l->pos, l->پای_ټکی);
}

ساكن
M3_پایله Skip_بایټونه (M3_Snapshot_بارول* l, size_t i_اندازه)
{
    إذا (i_اندازه > (size_t)(l->پای_ټکی - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    l->pos += i_اندازه;
    أرجع m3Err_none;
}


ساكن
M3_پایله Get_Ref_ارزښت (M3_Snapshot_بارول* l, m3type_t i_type, فراغ** o_reference)
{
    M3_پایله پایله = m3Err_none;
    u8       ډول   = 0;
    u64      id     = 0;

    *o_reference = عدم;

_   (GetU8(l, &ډول));
    إذا (ډول == 0x00) {
        أرجع (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    إذا (ډول == 0x01) {
        _throwif(m3Err_wasmMalformed, بنسټ_Type_د(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->عدد_Functions);
#  إذا d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_د(Ref_Type_د_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } وإلا إذا (ډول == 0x02) {
        _throwif(m3Err_wasmMalformed, بنسټ_Type_د(i_type) != c_m3Type_externref);
        ثابت M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        إذا (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (ډول == 0x03) {
        _throwif(m3Err_wasmMalformed, بنسټ_Type_د(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->عدد_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    وإلا إذا (ډول == 0x04) {
        _throwif(m3Err_wasmMalformed, بنسټ_Type_د(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->عدد_Continuations);
        *o_reference = l->continuations[id];
#  إذا d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        إذا (not l->checking and ډېرۍ_Type_د(i_type) != d_m3_Type_ډېرۍ_Abstract) {
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
    أرجع پایله;
}

ساكن
M3_پایله GetLE32 (M3_Snapshot_بارول* l, u32* o_ارزښت)
{
    u8       بایټونه[4];
    M3_پایله پایله = Get(l, بایټونه, حجم(بایټونه));

    *o_ارزښت = 0;

    لكل (u32 i = 0; i < حجم(بایټونه); ++i) {
        *o_ارزښت |= (u32)بایټونه[i] << (8 * i);
    }

    أرجع پایله;
}

ساكن
M3_پایله GetLE64 (M3_Snapshot_بارول* l, u64* o_ارزښت)
{
    u8       بایټونه[8];
    M3_پایله پایله = Get(l, بایټونه, حجم(بایټونه));

    *o_ارزښت = 0;

    لكل (u32 i = 0; i < حجم(بایټونه); ++i) {
        *o_ارزښت |= (u64)بایټونه[i] << (8 * i);
    }

    أرجع پایله;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ساكن
M3_پایله Get_ارزښت_نښان (M3_Snapshot_بارول* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_پایله پایله  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_منزل_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and بنسټ_Type_د(*o_type) != بنسټ_Type_د(i_expectedType));
    إذا (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    أرجع پایله;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ساكن
M3_پایله Get_ارزښت_Body (M3_Snapshot_بارول* l, m3type_t i_type, u64* o_بټونه)
{
    M3_پایله پایله = m3Err_none;

    *o_بټونه = 0;

    إذا (IsRefType(i_type)) {
        فراغ* reference = عدم;
_       (Get_Ref_ارزښت(l, i_type, &reference));
        *o_بټونه = (u64)(uintptr_t)reference;
    } وإلا إذا (بنسټ_Type_د(i_type) == c_m3Type_i32) {
        i32 ارزښت = 0;
_       (ReadLEB_i32(&ارزښت, &l->pos, l->پای_ټکی));
        *o_بټونه = (u32)ارزښت;
    } وإلا إذا (بنسټ_Type_د(i_type) == c_m3Type_i64) {
        i64 ارزښت = 0;
_       (GetLEB_i64(l, &ارزښت));
        *o_بټونه = (u64)ارزښت;
    } وإلا إذا (بنسټ_Type_د(i_type) == c_m3Type_f32) {
        u32 بټونه = 0;
_       (GetLE32(l, &بټونه));
        *o_بټونه = بټونه;
    } وإلا إذا (بنسټ_Type_د(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_بټونه));
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله Get_ارزښت_بټونه (M3_Snapshot_بارول* l, m3type_t i_expectedType, u64* o_بټونه)
{
    M3_پایله پایله = m3Err_none;
    m3type_t type;

_   (Get_ارزښت_نښان(l, i_expectedType, &type));
_   (Get_ارزښت_Body(l, type, o_بټونه));

_catch:
    أرجع پایله;
}

ساكن
M3_پایله Get_ارزښت (M3_Snapshot_بارول* l, m3type_t i_expectedType, فراغ* o_where)
{
    M3_پایله پایله = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    إذا (l->checking) {
        o_where = scratch;
    }

_   (Get_ارزښت_نښان(l, i_expectedType, &type));

    إذا (IsRefType(type)) {
        فراغ* reference = عدم;
        u64   بټونه      = 0;
_       (Get_ارزښت_Body(l, type, &بټونه));
        reference = (فراغ*)(uintptr_t)بټونه;
        حافظه_نقل(o_where, &reference, حجم(reference));
    } وإلا إذا (بنسټ_Type_د(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } وإلا {
        u64 بټونه = 0;
_       (Get_ارزښت_Body(l, type, &بټونه));

        إذا (Is64_بټ_Type(type)) {
            حافظه_نقل(o_where, &بټونه, حجم(بټونه));
        } وإلا {
            u32 narrow = (u32)بټونه;
            حافظه_نقل(o_where, &narrow, حجم(narrow));
        }
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله GetFunction (M3_Snapshot_بارول* l, IM3Function* o_function)
{
    M3_پایله پایله = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->عدد_Functions);
    *o_function = &l->module->functions[index];

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_حافظه_Chunks (M3_Snapshot_بارول* l, u8* o_بایټونه, size_t i_اندازه)
{
    M3_پایله پایله = m3Err_none;

    لكل (;;) {
        u8  chunkType = 0;
        u32 offset = 0, اوږدتیا = 0;

_       (GetU8(l, &chunkType));

        إذا (chunkType == d_m3_Chunk_پای_ټکی) {
            اكسر;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &اوږدتیا));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_اندازه or اوږدتیا > i_اندازه - (size_t)offset);

        إذا (chunkType == d_m3ChunkRaw) {
            إذا (l->checking or اوږدتیا == 0) {
_               (Skip_بایټونه(l, اوږدتیا));
            } وإلا {
_               (Get(l, o_بایټونه + offset, اوږدتیا));
            }
        } وإلا إذا (chunkType == d_m3ChunkFillFF) {
            إذا (not l->checking and اوږدتیا) {
                memset(o_بایټونه + offset, 0xFF, اوږدتیا);
            }
        } وإلا {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_Memories (M3_Snapshot_بارول* l)
{
    M3_پایله  پایله      = m3Err_none;
    IM3Module module      = l->module;
    u32       عدد_Memories = 0;
    l->loaded_حافظه_بایټونه  = 0;

_   (GetLEB_u32(l, &عدد_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, عدد_Memories == 0 or عدد_Memories != module->عدد_Memories);

    لكل (u32 m = 0; m < عدد_Memories; ++m) {
        u32       mem_شاخص = 0, لومړی_شاخص = 0, پاڼه_بټونه = 0, عدد_پاڼې = 0;
        IM3_حافظه حافظه;

_       (GetLEB_u32(l, &mem_شاخص));
_       (GetLEB_u32(l, &لومړی_شاخص));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_شاخص != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 لومړی_شاخص != حافظه_لومړی_شاخص(module, m));
        إذا (لومړی_شاخص != m) {
            استمر;
        }

_       (GetLEB_u32(l, &پاڼه_بټونه));
_       (GetLEB_u32(l, &عدد_پاڼې));

        حافظه = module->memories[mem_شاخص];
        _throwif("a shared memory cannot be restored from a snapshot", حافظه and حافظه->isShared);

        size_t بایټونه = 0;
        u8*    معلومات  = عدم;

        إذا (حافظه) {
            _throwif(m3Err_wasmMalformed, پاڼه_بټونه >= 32 or (1u << پاڼه_بټونه) != حافظه_پاڼه_اندازه(حافظه));
            _throwif(m3Err_wasmMalformed, عدد_پاڼې < حافظه->عدد_پاڼې or عدد_پاڼې > حافظه->max_پاڼې);
            _throwif(m3Err_wasmMalformed, عدد_پاڼې > SIZE_MAX / حافظه_پاڼه_اندازه(حافظه));
            بایټونه = (size_t)عدد_پاڼې * حافظه_پاڼه_اندازه(حافظه);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->حافظه_حد and بایټونه > (size_t)l->runtime->حافظه_حد);

            إذا (not l->checking) {
                إذا (حافظه->عدد_پاڼې < عدد_پاڼې) {
_                   (Resize_حافظه(l->runtime, حافظه, عدد_پاڼې));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         حافظه->mallocated and حافظه->mallocated->اوږدتیا < بایټونه);
            }

            l->loaded_حافظه_بایټونه += بایټونه;

            إذا (بایټونه and حافظه->mallocated) {
                معلومات = m3_Mem_معلومات(حافظه->mallocated);
            }
        } وإلا {
            _throwif(m3Err_wasmMalformed, عدد_پاڼې != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        إذا (معلومات and not l->checking) {
            memset(معلومات, 0, بایټونه);
        }

_       (بارول_حافظه_Chunks(l, معلومات, بایټونه));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_حافظه_بایټونه != l->required_حافظه_بایټونه);

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_Globals (M3_Snapshot_بارول* l)
{
    M3_پایله  پایله     = m3Err_none;
    IM3Module module     = l->module;
    u32       عدد_Globals = 0;

_   (GetLEB_u32(l, &عدد_Globals));
    _throwif(m3Err_wasmMalformed, عدد_Globals == 0 or عدد_Globals != module->عدد_Globals);

    لكل (u32 g = 0; g < عدد_Globals; ++g) {
        u32       global_شاخص = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_شاخص));
        _throwif(m3Err_wasmMalformed, global_شاخص != g);

        global = &module->globals[global_شاخص];
        cell   = global->resolved ? global->resolved : global;

_       (Get_ارزښت(l, global->type, &cell->i64_ارزښت));
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_Tables (M3_Snapshot_بارول* l)
{
    M3_پایله  پایله       = m3Err_none;
    IM3Module module       = l->module;
    u32       عدد_Tables    = 0;
    l->loaded_جدول_Elements = 0;

_   (GetLEB_u32(l, &عدد_Tables));
    _throwif(m3Err_wasmMalformed, عدد_Tables == 0 or عدد_Tables != module->عدد_Tables);

    لكل (u32 t = 0; t < عدد_Tables; ++t) {
        u32      جدول_شاخص = 0, لومړی_شاخص = 0, اندازه = 0;
        u8       valtype = 0;
        IM3_جدول جدول_2;

_       (GetLEB_u32(l, &جدول_شاخص));
_       (GetLEB_u32(l, &لومړی_شاخص));

        _throwif(m3Err_wasmMalformed, جدول_شاخص != t);
        _throwif("the snapshot shares tables between imports differently",
                 لومړی_شاخص != جدول_لومړی_شاخص(module, t));
        إذا (لومړی_شاخص != t) {
            استمر;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &اندازه));

        جدول_2 = module->tables[جدول_شاخص];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_منزل_Val_Type(جدول_2->type));
        _throwif(m3Err_wasmMalformed, اندازه > (جدول_2->hasMax ? جدول_2->max_اندازه : d_m3_Max_Sane_جدول_اندازه));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, اندازه < جدول_2->اندازه);

        l->loaded_جدول_Elements += اندازه;

        إذا (not l->checking and اندازه != جدول_2->اندازه) {
            فراغ** elements;

            _throwif(m3_Err_جدول_حد_Exceeded, l->runtime->جدول_Elements_حد and
                                                 اندازه - جدول_2->اندازه > l->runtime->جدول_Elements_حد - l->runtime->جدول_Elements_کارول_شوی);
            elements = m3_ReallocArray(فراغ*, جدول_2->elements, اندازه, جدول_2->اندازه);
            _throwifnull(elements);

            l->runtime->جدول_Elements_کارول_شوی += اندازه - جدول_2->اندازه;
            جدول_2->elements = elements;
            جدول_2->اندازه     = اندازه;
        }

        لكل (u32 e = 0; e < اندازه; ++e) {
            فراغ* reference = عدم;
_           (Get_Ref_ارزښت(l, جدول_2->type, &reference));
            إذا (not l->checking) {
                جدول_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_جدول_Elements != l->required_جدول_Elements);

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_Segments (M3_Snapshot_بارول* l)
{
    M3_پایله  پایله = m3Err_none;
    IM3Module module = l->module;
    u32       شمېر  = 0;

_   (GetLEB_u32(l, &شمېر));
    _throwif(m3Err_wasmMalformed, شمېر != module->عدد_معلومات_Segments);
    _throwif(m3Err_wasmMalformed, module->عدد_معلومات_Segments == 0 and module->عدد_Element_Segments == 0);

    لكل (u32 i = 0; i < شمېر; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->معلومات_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &شمېر));
    _throwif(m3Err_wasmMalformed, شمېر != module->عدد_Element_Segments);

    لكل (u32 i = 0; i < شمېر; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    أرجع پایله;
}

// A frame for the continuation being put together, outermost first
ساكن
M3_پایله جمع_کړه_چوکاټ (M3_Snapshot_بارول* l, u32* io_عدد_چوکاټونه, M3_چوکاټ_ډول i_ډول, m3stack_t i_sp, M3_چوکاټ** o_چوکاټ)
{
    إذا (*io_عدد_چوکاټونه >= d_m3_Continuation_Max_چوکاټونه) {
        أرجع "the snapshot's frames do not fit this build's";
    }

    M3_چوکاټ* چوکاټ = &l->چوکاټونه[(*io_عدد_چوکاټونه)++];

    memset(چوکاټ, 0, حجم(*چوکاټ));
    چوکاټ->ډول   = (u8)i_ډول;
    چوکاټ->sp     = i_sp;
    چوکاټ->حافظه = Module_Memory0(l->module);

    *o_چوکاټ = چوکاټ;

    أرجع m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ساكن
M3_پایله PrepareFunction (IM3Function i_function)
{
    M3_پایله پایله = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    أرجع پایله;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ساكن
M3_پایله جمع_کړه_بلاک_چوکاټونه (M3_Snapshot_بارول* l, ثابت M3SnapshotMap* i_map, ثابت M3_خوندي_ټکی* i_ټکی, m3stack_t i_sp,
                         u32* io_عدد_چوکاټونه)
{
    M3_پایله پایله = m3Err_none;

    لكل (u32 b = 0; b < i_map->عدد_بلاکونه; ++b) {
        ثابت M3_بلاک_پیل* بلاک = &i_map->بلاکونه[b];
        M3_چوکاټ*            چوکاټ;

        إذا (not Encloses_دننه_واټن(بلاک, i_ټکی->wasm_دننه_واټن)) {
            استمر;
        }

        إذا (بلاک->opcode == c_waOp_loop) {
_           (جمع_کړه_چوکاټ(l, io_عدد_چوکاټونه, چوکاټ_loop, i_sp, &چوکاټ));
            چوکاټ->pc = بلاک->pc;
        }
#  إذا d_m3HasExceptionHandling
        وإلا {
_           (جمع_کړه_چوکاټ(l, io_عدد_چوکاټونه, چوکاټ_try, i_sp, &چوکاټ));
            چوکاټ->pc                = بلاک->pc;
            چوکاټ->try_.عدد_Clauses   = بلاک->عدد_Clauses;
            چوکاټ->try_.handlers_ژوندی = Handlers_ژوندی_At(i_map, i_ټکی, بلاک);
        }
#  endif
    }

_catch:
    أرجع پایله;
}

// Puts a function's values where this build keeps them at the safepoint
ساكن
M3_پایله بارول_ارزښتونه (M3_Snapshot_بارول* l, m3slot_t* i_بنسټ, u32 i_عدد_Slots, m3stack_t i_sp,
                     ثابت M3SnapshotMap* i_map, ثابت M3_خوندي_ټکی* i_ټکی, ثابت M3_چوکاټ_Registers* i_registers)
{
    M3_پایله پایله    = m3Err_none;
    u32      عدد_ارزښتونه = 0;

_   (GetLEB_u32(l, &عدد_ارزښتونه));
    _throwif("the snapshot's frame does not match what this build compiled", عدد_ارزښتونه != i_map->عدد_Locals + i_ټکی->عدد_ارزښتونه);

    لكل (u32 v = 0; v < عدد_ارزښتونه; ++v) {
        ثابت M3_خونه_ارزښت* ارزښت     = چوکاټ_ارزښت(i_map, i_ټکی, v);
        u64                کلمه      = 0;
        فراغ*              reference = عدم;
        u8*                where;

_       (Locate_ارزښت(i_بنسټ, i_عدد_Slots, i_sp, ارزښت, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (بنسټ_Type_د(ارزښت->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_ارزښت_نښان(l, ارزښت->type, &type));
            إذا (l->checking) {
_               (Skip_بایټونه(l, 16));
            } وإلا {
_               (Get(l, where, 16));
            }
            استمر;
        }

        l->allowUnfilledRef = i_ټکی->ډول == safepoint_suspend and
                              v >= عدد_ارزښتونه - i_ټکی->عدد_Results;
_       (Get_ارزښت_بټونه(l, ارزښت->type, &کلمه));
        l->allowUnfilledRef = false;

        إذا (IsRefType(ارزښت->type)) {
            reference = (فراغ*)(uintptr_t)کلمه;
        }

        إذا (not l->checking) {
_           (Write_ارزښت(where, i_registers, ارزښت, کلمه, reference));
        }
    }

_catch:
    أرجع پایله;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ساكن
M3_پایله بارول_Suspended_Body (M3_Snapshot_بارول* l, IM3Continuation io_cont, u32 i_id)
{
    M3_پایله    پایله    = m3Err_none;
    u32         عدد_چوکاټونه = 0;
    M3_چوکاټ*    call_چوکاټ = عدم;
    IM3Function function  = عدم;
    m3slot_t*   بنسټ;
    u32         عدد_Slots;
    u32         عدد_Activations = 0;
    IM3FuncType expected       = io_cont->ثبت_Function->funcType;
    m3stack_t   sp;

_   (Continuation_ډېری(l->runtime, io_cont, &بنسټ, &عدد_Slots));

_   (GetLEB_u32(l, &عدد_Activations));
    _throwif(m3Err_wasmMalformed, عدد_Activations == 0 or عدد_Activations > d_m3_Continuation_Max_چوکاټونه);

    io_cont->pc                = عدم;
    io_cont->r0                = 0;
    io_cont->عدد_Suspend_Results = 0;
    io_cont->suspend_ټکی      = safepoint_op;
#  إذا d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = بنسټ;

    لكل (u32 a = 0; a < عدد_Activations; ++a) {
        bool                 isInnermost = (a + 1 == عدد_Activations);
        ثابت M3_خوندي_ټکی*   ټکی;
        M3_چوکاټ_Registers     registers;
        u8                   ډول       = 0;
        u32                  wasm_دننه_واټن = 0, موخه_Loop = 0;
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
                 (u32)(sp - بنسټ) + function->max_ډېری_Slots > عدد_Slots);

        // the call the frame outside this one is waiting on
        إذا (call_چوکاټ) {
            call_چوکاټ->call.function = function;
        }

#  إذا d_m3_ثبت_Keeps_چوکاټ
        {
            M3_چوکاټ* ثبت;
_           (جمع_کړه_چوکاټ(l, &عدد_چوکاټونه, چوکاټ_ثبت, sp, &ثبت));
            ثبت->pc             = function->compiled;
            ثبت->ثبت.function = function;
        }
#  endif

_       (GetU8(l, &ډول));
_       (GetLEB_u32(l, &wasm_دننه_واټن));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        إذا (ډول == safepoint_op) {
            ثابت M3_بلاک_پیل* loop;

_           (GetLEB_u32(l, &موخه_Loop));

            loop = پیدا_کړه_بلاک_At(map, c_waOp_loop, موخه_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_دننه_واټن(loop, wasm_دننه_واټن));
        }

        ټکی = پیدا_کړه_خوندي_ټکی_At(map, wasm_دننه_واټن, ډول, موخه_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not ټکی);

_       (جمع_کړه_بلاک_چوکاټونه(l, map, ټکی, sp, &عدد_چوکاټونه));

        إذا (not isInnermost) {
            _throwif(m3Err_wasmMalformed, ډول != safepoint_call);

_           (جمع_کړه_چوکاټ(l, &عدد_چوکاټونه, چوکاټ_call, sp, &call_چوکاټ));
            call_چوکاټ->pc = ټکی->pc;
            registers.r0  = &call_چوکاټ->call.r0;
#  إذا d_m3HasFloat
            registers.fp0 = &call_چوکاټ->call.fp0;
#  endif
        } وإلا إذا (ډول == safepoint_resume) {
            M3_چوکاټ* چوکاټ;
            u32      عدد_Results = ټکی->عدد_Results;
            pc_t     resultsPC  = ټکی->pc - 2 * عدد_Results;

_           (جمع_کړه_چوکاټ(l, &عدد_چوکاټونه, چوکاټ_resume, sp, &چوکاټ));
            چوکاټ->pc                 = ټکی->pc;
            چوکاټ->resume.resultsPC   = resultsPC;
            چوکاټ->resume.عدد_Results  = عدد_Results;
            چوکاټ->resume.عدد_Handlers = ټکی->aux;
            چوکاټ->resume.handlersPC  = resultsPC - 1 - 3 * ټکی->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            call_چوکاټ = چوکاټ;
        } وإلا {
            _throwif(m3Err_wasmMalformed, ډول != safepoint_op and ډول != safepoint_ثبت and ډول != safepoint_suspend);

            io_cont->pc           = ټکی->pc;
            io_cont->suspend_ټکی = ډول;
            registers.r0          = &io_cont->r0;
#  إذا d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (بارول_ارزښتونه(l, بنسټ, عدد_Slots, sp, map, ټکی, &registers));

        إذا (not l->checking) {
            RestoreConstants(function, sp);
        }

        إذا (not isInnermost) {
            sp = sp + ټکی->aux;
_           (CallSiteType(function, ټکی, &expected));
        } وإلا إذا (ډول == safepoint_resume) {
            فراغ* cont = عدم;
_           (Get_Ref_ارزښت(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == عدم);

            call_چوکاټ->resume.cont = (IM3Continuation)cont;
            إذا (not l->checking) {
                l->suspendSites[i_id].ټکی = ټکی;
                لكل (u32 i = 1; i < l->عدد_Continuations; ++i) {
                    إذا (l->continuations[i] == cont) {
                        // a continuation runs under one resume at a time
                        _throwif("a snapshot continuation is resumed by two frames", l->suspendSites[i].resumed);

                        l->suspendSites[i_id].موخه = i;
                        l->suspendSites[i].resumed   = true;
                        اكسر;
                    }
                }
            }
        } وإلا {
            // op_ContinueLoopIf suspends before it has branched, and goes on
            // by reading the condition again
            إذا (ټکی->flags & d_m3_خوندي_ټکی_اخیستل_شوی_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            إذا (ډول == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, ټکی->عدد_Results > ټکی->عدد_ارزښتونه or
                                                ټکی->عدد_Results > d_m3MaxContinuationPayload);

                io_cont->عدد_Suspend_Results = ټکی->عدد_Results;
                إذا (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].ټکی = ټکی;
                }

                لكل (u32 i = 0; i < ټکی->عدد_Results; ++i) {
                    ثابت M3_خونه_ارزښت* ارزښت = &map->ارزښتونه[ټکی->لومړی_ارزښت + ټکی->عدد_ارزښتونه - ټکی->عدد_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_ثبتوونکی_خونه_Alias(ارزښت->خونه));

                    io_cont->suspend_پایله_Offsets[i] = ارزښت->خونه;
                    io_cont->suspend_پایله_Is64[i]    = Is64_بټ_Type(ارزښت->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    إذا (l->checking) {
        أرجع m3Err_none;
    }

    // held innermost first
    إذا (عدد_چوکاټونه > io_cont->چوکاټونه_Cap) {
        M3_چوکاټ* چوکاټونه = m3_ReallocArray(M3_چوکاټ, io_cont->چوکاټونه, عدد_چوکاټونه, io_cont->چوکاټونه_Cap);
        _throwifnull(چوکاټونه);

        io_cont->چوکاټونه    = چوکاټونه;
        io_cont->چوکاټونه_Cap = عدد_چوکاټونه;
    }

    لكل (u32 k = 0; k < عدد_چوکاټونه; ++k) {
        io_cont->چوکاټونه[عدد_چوکاټونه - 1 - k] = l->چوکاټونه[k];
    }

    io_cont->عدد_چوکاټونه = عدد_چوکاټونه;

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_Continuation (M3_Snapshot_بارول* l, u32 i_id)
{
    M3_پایله        پایله = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    إذا (l->checking) {
        scratch          = *cont;
        scratch.val_ډېری = l->check_ډېری;
        إذا (i_id == 0) {
            scratch.val_ډېری = (m3slot_t*)l->runtime->origin_ډېری;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_شاخص = 0, ثبت_شاخص = 0, bound_دلیلونه_شمېر = 0;
    u8  state = 0, is_ریښه = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_ریښه));
_   (GetLEB_u32(l, &type_شاخص));
_   (GetLEB_u32(l, &ثبت_شاخص));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_ریښه != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_ریښه and state != snapshot_contSuspended);
    إذا (not is_ریښه and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_شاخص != d_m3SnapshotNone and type_شاخص >= module->عدد_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, ثبت_شاخص >= module->عدد_Functions);
    _throwif(m3Err_wasmMalformed, is_ریښه ? type_شاخص != d_m3SnapshotNone : (type_شاخص == d_m3SnapshotNone or not module->funcTypes[type_شاخص]->isContinuation));

    إذا (not l->checking and not is_ریښه and state != snapshot_cont_پای_ته_رسېدلی) {
_       (Continuation_Acquire_ډېری(l->runtime, cont));
    }
    cont->type          = (type_شاخص != d_m3SnapshotNone) ? module->funcTypes[type_شاخص] : عدم;
    cont->ثبت_Function = &module->functions[ثبت_شاخص];
    cont->عدد_Handlers   = 0;
    cont->handlersPC    = عدم;
    cont->پلار        = عدم;
    cont->عدد_چوکاټونه     = 0;

#  إذا d_m3HasExceptionHandling
    {
        فراغ* ref = عدم;
_       (Get_Ref_ارزښت(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_ریښه or state == snapshot_cont_پای_ته_رسېدلی) and cont->resumeThrow != عدم);
#  وإلا
    {
        فراغ* ref = عدم;
_       (Get_Ref_ارزښت(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != عدم);
    }
#  endif

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         عدد_Rets;

_       (GetLEB_u32(l, &bound_دلیلونه_شمېر));
        cont->bound_دلیلونه_شمېر = bound_دلیلونه_شمېر;
        inner                = cont->ثبت_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_دلیلونه_شمېر > Get_Func_Type_عدد_Params(inner));

        {
            IM3FuncType پاتې_شوی = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not پاتې_شوی or
                                            Get_Func_Type_عدد_Params(پاتې_شوی) != Get_Func_Type_عدد_Params(inner) - bound_دلیلونه_شمېر or
                                            Get_Func_Type_عدد_Results(پاتې_شوی) != Get_Func_Type_عدد_Results(inner));
            لكل (u16 i = 0; i < Get_Func_Type_عدد_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_پایله_Type(inner, i) != Get_Func_Type_پایله_Type(پاتې_شوی, i));
            }
            لكل (u16 i = 0; i < Get_Func_Type_عدد_Params(پاتې_شوی); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_دلیلونه_شمېر + i)) != GetFuncTypeParamType(پاتې_شوی, i));
            }
        }

        عدد_Rets = Get_Func_Type_عدد_Results(inner);

        _throwif(m3Err_wasmMalformed, (عدد_Rets + bound_دلیلونه_شمېر) * c_io_خونه_شمېر > cont->عدد_ډېری_Slots);

        لكل (u32 i = 0; i < bound_دلیلونه_شمېر; ++i) {
_           (Get_ارزښت(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ډېری + (عدد_Rets + i) * c_io_خونه_شمېر));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ډېری;
        cont->pc    = cont->ثبت_Function->compiled;
    } وإلا إذا (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_دلیلونه_شمېر));
        _throwif(m3Err_wasmMalformed, is_ریښه and bound_دلیلونه_شمېر != 0);
        cont->bound_دلیلونه_شمېر = bound_دلیلونه_شمېر;

_       (بارول_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } وإلا إذا (state == snapshot_cont_پای_ته_رسېدلی) {
_       (GetLEB_u32(l, &bound_دلیلونه_شمېر));
        _throwif(m3Err_wasmMalformed, bound_دلیلونه_شمېر != 0);
        cont->bound_دلیلونه_شمېر = 0;
        cont->state          = cont_consumed;
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    إذا (پایله) {
        cont->عدد_چوکاټونه = 0;
    }

    أرجع پایله;
}

ساكن
M3_پایله بارول_Continuations (M3_Snapshot_بارول* l)
{
    M3_پایله پایله = m3Err_none;
    u32      شمېر  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &شمېر));
    _throwif(m3Err_wasmMalformed, شمېر == 0 or شمېر != l->عدد_Continuations);

    لكل (u32 i = 0; i < شمېر; ++i) {
_       (بارول_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    أرجع پایله;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ساكن
M3_پایله CheckContinuationReferences (M3_Snapshot_بارول* l)
{
    M3_پایله پایله = m3Err_none;

    لكل (u32 id = 0; id < l->عدد_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             ونې_پاڼه  = id;
        u32             ژوروالی = 0;
#  إذا d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_د(Ref_Type_د_Func_Type(cont->type, true), expectedType)));
#  endif
        إذا (l->suspendSites[id].موخه) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_د_Ref(l->module, l->suspendSites[id].ټکی->resumeType);
            IM3Continuation ماشوم    = l->continuations[l->suspendSites[id].موخه];
            IM3FuncType     actual   = ماشوم->ثبت_Function ? ماشوم->ثبت_Function->funcType : عدم;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_عدد_Results(actual) != Get_Func_Type_عدد_Results(expected));
            لكل (u16 i = 0; i < Get_Func_Type_عدد_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_پایله_Type(actual, i) != Get_Func_Type_پایله_Type(expected, i));
            }
        }
        طالما (l->suspendSites[ونې_پاڼه].موخه) {
            ونې_پاڼه = l->suspendSites[ونې_پاڼه].موخه;
            _throwif(m3Err_wasmMalformed, ++ژوروالی >= l->عدد_Continuations or
                                            l->continuations[ونې_پاڼه]->state != cont_suspended);
        }
        إذا (cont->state != cont_suspended or id == 0) {
            استمر;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        إذا (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_دلیلونه_شمېر != 0);
            استمر;
        }

        ثابت M3SnapshotSuspendSite* site      = &l->suspendSites[ونې_پاڼه];
        IM3Continuation              inner     = l->continuations[ونې_پاڼه];
        IM3FuncType                  پاتې_شوی = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->ټکی or not پاتې_شوی);
        _throwif("a captured continuation has the wrong binding count", cont->bound_دلیلونه_شمېر > inner->عدد_Suspend_Results or
                                                                          Get_Func_Type_عدد_Params(پاتې_شوی) != inner->عدد_Suspend_Results - cont->bound_دلیلونه_شمېر);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_عدد_Results(پاتې_شوی) != Get_Func_Type_عدد_Results(cont->ثبت_Function->funcType));
        لكل (u16 i = 0; i < Get_Func_Type_عدد_Results(پاتې_شوی); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_پایله_Type(پاتې_شوی, i) !=
                                            Get_Func_Type_پایله_Type(cont->ثبت_Function->funcType, i));
        }

        لكل (u32 i = 0; i < inner->عدد_Suspend_Results; ++i) {
            ثابت M3_خونه_ارزښت* ارزښت = &site->map->ارزښتونه[site->ټکی->لومړی_ارزښت +
                                                          site->ټکی->عدد_ارزښتونه - site->ټکی->عدد_Results + i];
            إذا (i < cont->bound_دلیلونه_شمېر) {
                إذا (IsRefType(ارزښت->type) and not IsNullableRef(ارزښت->type)) {
                    فراغ* ref = عدم;
                    حافظه_نقل(&ref, inner->sp + ارزښت->خونه, حجم(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } وإلا {
                _throwif("a captured continuation has the wrong parameter type", ارزښت->type !=
                                                                                   GetFuncTypeParamType(پاتې_شوی, (u16)(i - cont->bound_دلیلونه_شمېر)));
            }
        }
    }

_catch:
    أرجع پایله;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_پایله بارول_Exceptions (M3_Snapshot_بارول* l)
{
    M3_پایله پایله        = m3Err_none;
    u32      عدد_Exceptions = 0;

_   (GetLEB_u32(l, &عدد_Exceptions));
    _throwif(m3Err_wasmMalformed, عدد_Exceptions == 0 or عدد_Exceptions != l->عدد_Exceptions);

    لكل (u32 i = 0; i < عدد_Exceptions; ++i) {
        u32          exnId = 0, عدد_دلیلونه = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &عدد_دلیلونه));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->نښان->type;

        _throwif(m3Err_wasmMalformed, عدد_دلیلونه != exn->عدد_دلیلونه);

        لكل (u32 a = 0; a < عدد_دلیلونه; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      بټونه    = 0;
_           (Get_ارزښت_بټونه(l, argType, &بټونه));
            إذا (not l->checking) {
                exn->دلیلونه[a] = بټونه;
            }
        }
    }

_catch:
    أرجع پایله;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
تعريفنوع بنية M3BoundedReader {
    M3_Snapshot_بارول* بارول;
    u64             پاتې_شوی;
    M3_پایله        error;
} M3BoundedReader;

ساكن
M3_پایله BoundedReader_Read (فراغ* o_بفر, size_t i_اندازه, فراغ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    إذا (i_اندازه > br->پاتې_شوی) {
        br->error = "the host read past the end of its state";
        أرجع br->error;
    }

    br->پاتې_شوی -= i_اندازه;

    أرجع Get(br->بارول, o_بفر, i_اندازه);
}

ساكن
M3_پایله بارول_Host_State (M3_Snapshot_بارول* l)
{
    M3_پایله               پایله = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 اندازه = (size_t)(l->پای_ټکی - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->بارول_Host_State_2);

    إذا (l->checking) {
        أرجع Skip_بایټونه(l, اندازه);
    }
    reader.بارول      = l;
    reader.پاتې_شوی = اندازه;
    reader.error     = m3Err_none;

_   (hooks->بارول_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, اندازه));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.پاتې_شوی != 0);

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_سرلیک (M3_Snapshot_بارول* l)
{
    M3_پایله پایله = m3Err_none;
    u8       magic[حجم(c_snapshotMagic)];
    u32      نسخه = 0;

_   (Get(l, magic, حجم(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, حجم(magic)) != 0);

_   (GetLE32(l, &نسخه));
    _throwif(m3Err_wasmMalformed, نسخه != c_snapshot_نسخه);

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_Meta (M3_Snapshot_بارول* l, u32* o_flags)
{
    M3_پایله پایله      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      حافظه_بایټونه = 0, جدول_Elements = 0;
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

_   (GetLEB_u32(l, &l->عدد_Continuations));
_   (GetLEB_u32(l, &l->عدد_Exceptions));
_   (GetLEB_u64(l, &حافظه_بایټونه));
_   (GetLEB_u64(l, &جدول_Elements));
_   (GetLEB_u32(l, &فعال_Stacks));
    l->requiredStacks        = فعال_Stacks;
    l->required_حافظه_بایټونه   = حافظه_بایټونه;
    l->required_جدول_Elements = جدول_Elements;
    _throwif(m3Err_wasmMalformed, حافظه_بایټونه < current_حافظه or جدول_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->عدد_Memories and حافظه_بایټونه != 0);
    _throwif(m3Err_wasmMalformed, not l->module->عدد_Tables and جدول_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    إذا (*o_flags & d_m3SnapshotFlagPostmortem) {
        لكل (u32 i = 0; i < l->عدد_Exceptions; ++i) {
            u32 نښان_شاخص = 0;
_           (GetLEB_u32(l, &نښان_شاخص));
        }
        أرجع m3Err_none;
    }

    _throwif(m3_Err_حافظه_حد_Exceeded, l->runtime->حافظه_بایټونه_حد and
                                          حافظه_بایټونه - current_حافظه > l->runtime->حافظه_بایټونه_حد - l->runtime->حافظه_بایټونه_کارول_شوی);
    _throwif(m3_Err_جدول_حد_Exceeded, l->runtime->جدول_Elements_حد and
                                         جدول_Elements - currentTables > l->runtime->جدول_Elements_حد - l->runtime->جدول_Elements_کارول_شوی);
    _throwif(m3_Err_continuation_حد_Exceeded, l->runtime->continuations_حد and
                                                فعال_Stacks > l->runtime->continuations_حد - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->عدد_Continuations == 0 or فعال_Stacks >= l->عدد_Continuations);
#  إذا !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->عدد_Exceptions != 0);
#  endif

    إذا (l->checking) {
        لكل (u32 i = 0; i < l->عدد_Exceptions; ++i) {
            u32 نښان_شاخص = 0;
_           (GetLEB_u32(l, &نښان_شاخص));
#  إذا d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, نښان_شاخص >= l->module->عدد_نښانونه);
#  endif
        }
        أرجع m3Err_none;
    }

    l->check_ډېری = m3_AllocArray(m3slot_t, d_m3_Continuation_ډېری_Slots + 4);
    _throwifnull(l->check_ډېری);
    l->چوکاټونه = m3_AllocArray(M3_چوکاټ, d_m3_Continuation_Max_چوکاټونه);
    _throwifnull(l->چوکاټونه);

    l->continuations = m3_AllocArray(IM3Continuation, l->عدد_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->عدد_Continuations);
    _throwifnull(l->suspendSites);

    إذا (not l->runtime->ریښه_Continuation) {
        IM3Continuation ریښه = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(ریښه);

        // the root borrows the runtime's stack, so it holds none of its own
        ریښه->عدد_ډېری_Slots          = l->runtime->عدد_ډېری_Slots;
        l->runtime->ریښه_Continuation = ریښه;
    }

    l->continuations[0] = l->runtime->ریښه_Continuation;

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
        u32    نښان_شاخص = 0;
        IM3_نښان نښان;
        u32    عدد_دلیلونه;

_       (GetLEB_u32(l, &نښان_شاخص));
        _throwif(m3Err_wasmMalformed, نښان_شاخص >= l->module->عدد_نښانونه);

        نښان     = نښان_د_شاخص(l->module, نښان_شاخص);
        عدد_دلیلونه = Get_Func_Type_عدد_Params(نښان->type);

        l->exceptions[i] = NewException(l->runtime, نښان, عدد_دلیلونه);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  وإلا
    لكل (u32 i = 0; i < l->عدد_Exceptions; ++i) {
        u32 نښان_شاخص = 0;
_       (GetLEB_u32(l, &نښان_شاخص));
    }
#  endif

_catch:
    أرجع پایله;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ساكن
M3_پایله بارول_Section (M3_Snapshot_بارول* l, u8 i_sectionId)
{
    M3_پایله پایله = m3Err_none;
    u32      flags  = 0;
    بدل (i_sectionId) {
    حالة d_m3SnapshotSection_Meta:
_       (بارول_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        اكسر;

    حالة d_m3_Snapshot_Section_حافظه:
_       (بارول_Memories(l));
        اكسر;

    حالة d_m3_Snapshot_Section_جدول:
_       (بارول_Tables(l));
        اكسر;

    حالة d_m3SnapshotSection_Global:
_       (بارول_Globals(l));
        اكسر;

    حالة d_m3SnapshotSection_Segment:
_       (بارول_Segments(l));
        اكسر;

#  إذا d_m3HasExceptionHandling
    حالة d_m3SnapshotSection_Exception:
_       (بارول_Exceptions(l));
        اكسر;
#  endif

    حالة d_m3SnapshotSection_Continuation:
_       (بارول_Continuations(l));
        اكسر;

    حالة d_m3SnapshotSection_HostState:
_       (بارول_Host_State(l));
        اكسر;

    افتراضي:
_       (Skip_بایټونه(l, (size_t)(l->پای_ټکی - l->pos)));
        اكسر;
    }

_catch:
    أرجع پایله;
}

ساكن
M3_پایله بارول_Sections (M3_Snapshot_بارول* l, bool i_embedded)
{
    M3_پایله پایله  = m3Err_none;
    u8*      payload = عدم;

    إذا (not i_embedded) {
_       (بارول_سرلیک(l));
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

        _throwif("the snapshot holds a section twice", l->کتل_شوی_Sections[sectionId]);
        l->کتل_شوی_Sections[sectionId] = true;
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
        l->پای_ټکی       = payload + section_اندازه;
        l->دننه_Section = true;

        {
            M3_Snapshot_بارول check = *l;
            check.checking       = true;
_           (بارول_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.پای_ټکی);
        }
        l->بدل_شوی = l->بدل_شوی or sectionId != d_m3SnapshotSection_Meta;
_       (بارول_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->پای_ټکی);
        l->دننه_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->کتل_شوی_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->عدد_Exceptions and not l->کتل_شوی_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->عدد_Memories and not l->کتل_شوی_Sections[d_m3_Snapshot_Section_حافظه]);
    _throwif("the snapshot has no table section",
             l->module->عدد_Tables and not l->کتل_شوی_Sections[d_m3_Snapshot_Section_جدول]);
    _throwif("the snapshot has no global section",
             l->module->عدد_Globals and not l->کتل_شوی_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->عدد_معلومات_Segments or l->module->عدد_Element_Segments) and
               not l->کتل_شوی_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    أرجع پایله;
}

#د_که_پای // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3HasSnapshots

ساكن
M3_پایله SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_پایله       پایله = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, حجم(save));

    IM3Continuation ریښه = io_runtime->ریښه_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not ریښه or ریښه->state != cont_suspended;

    IM3Function ثبت = save.postmortem ? io_runtime->entered : ریښه->ثبت_Function;

    إذا (o_module) {
        *o_module = عدم;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not ثبت);

    save.module = ثبت->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    إذا (o_module) {
        *o_module = save.module;
    }

    لكل (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->بل) {
        u32 id;
_       (اشاره_ګر_Ids_جمع_کړه(&save.ژوندی_Continuations, cont, &id));
    }

#  إذا d_m3HasExceptionHandling
    لكل (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->بل) {
        u32 id;
_       (اشاره_ګر_Ids_جمع_کړه(&save.ژوندی_Exceptions, exception, &id));
    }
#  endif

    إذا (not save.postmortem) {
        u32 id;
_       (اشاره_ګر_Ids_جمع_کړه(&save.continuations, ریښه, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    اشاره_ګر_Ids_Free(&save.ژوندی_Continuations);
    اشاره_ګر_Ids_Free(&save.ژوندی_Exceptions);
    اشاره_ګر_Ids_Free(&save.continuations);
    اشاره_ګر_Ids_Free(&save.exceptions);

    أرجع پایله;
}

#د_که_پای

M3_پایله m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata)
{
    إذا (not io_runtime or not i_writer) {
        أرجع m3Err_mallocFailed;
    }

#إذا d_m3HasSnapshots
    أرجع SaveSnapshot(io_runtime, i_writer, i_userdata, عدم, false);
#وإلا
    (فراغ)i_userdata;
    أرجع "snapshots are not available in this build of Wasm3";
#د_که_پای
}


ساكن
M3_پایله بارول_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata, bool i_embedded)
{
    M3_پایله پایله = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#إذا d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_پرلپسې);

    {
        M3_Snapshot_بارول بارول;
        memset(&بارول, 0, حجم(بارول));

        بارول.runtime  = io_runtime;
        بارول.module   = i_module;
        بارول.reader   = i_reader;
        بارول.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        پایله = بارول_Sections(&بارول, i_embedded);

        IM3Continuation ریښه = io_runtime->ریښه_Continuation;

        إذا (پایله) {
            إذا (بارول.بدل_شوی) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                إذا (ریښه) {
                    ریښه->state     = cont_returned;
                    ریښه->عدد_چوکاټونه = 0;
                }
            }
        } وإلا {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->پیل_Function = -1;
            i_module->has_پرلپسې        = true;
        }

        Runtime_ځای_پرځای_کړه_Call_ډېری(io_runtime);

        m3_Free(بارول.check_ډېری);
        m3_Free(بارول.چوکاټونه);
        m3_Free(بارول.suspendSites);
        m3_Free(بارول.continuations);
#  إذا d_m3HasExceptionHandling
        m3_Free(بارول.exceptions);
#  endif
    }
#وإلا
    (فراغ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#د_که_پای

_catch:
    أرجع پایله;
}


M3_پایله m3_بارول_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata)
{
    أرجع بارول_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


فراغ m3_سېټ_Snapshot_Hooks (IM3Runtime io_runtime, ثابت M3SnapshotHooks* i_hooks)
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
#د_که_پای
}


M3_پایله m3_Save_Snapshot_منزل_بفر (IM3Runtime io_runtime, فراغ** o_بایټونه, size_t* o_اندازه)
{
    إذا (!io_runtime || !o_بایټونه || !o_اندازه) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    M3_بفر_Writer bw     = { عدم, 0, 0 };
    M3_پایله       پایله = m3_SaveSnapshot(io_runtime, بفر_Writer_Write, &bw);
    إذا (پایله) {
        m3_Free(bw.بفر);
        *o_بایټونه = عدم;
        *o_اندازه  = 0;
        أرجع پایله;
    }
    *o_بایټونه = bw.بفر;
    *o_اندازه  = bw.اندازه;
    أرجع m3Err_none;
#وإلا
    *o_بایټونه = عدم;
    *o_اندازه  = 0;
    أرجع "snapshots are not available in this build of Wasm3";
#د_که_پای
}

#إذا d_m3HasSnapshots

// Appends one LEB128-encoded u32 to a plain byte cursor
ساكن
u8* EmitLEB_u32 (u8* o_where, u32 i_ارزښت)
{
    افعل {
        u8 byte = i_ارزښت & 0x7F;
        i_ارزښت >>= 7;
        إذا (i_ارزښت != 0) {
            byte |= 0x80;
        }
        *o_where++ = byte;
    } طالما (i_ارزښت != 0);

    أرجع o_where;
}

ساكن
u32 اندازه_د_LEB_u32 (u32 i_ارزښت)
{
    u32 اندازه = 0;

    افعل {
        i_ارزښت >>= 7;
        ++اندازه;
    } طالما (i_ارزښت != 0);

    أرجع اندازه;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ساكن
M3_پایله پیدا_کړه_Embedded_Snapshot (ثابت u8* i_پیل, ثابت u8* i_پای_ټکی, cstr_t i_secName,
                               ثابت u8** o_sec_پیل, ثابت u8** o_sec_پای_ټکی,
                               ثابت u8** o_معلومات, u32* o_اندازه)
{
    M3_پایله پایله = m3Err_none;
    بایټونه_t  pos    = i_پیل;
    bool     وموندل  = false;
    cstr_t   name   = عدم;

    إذا (o_sec_پیل) {
        *o_sec_پیل = عدم;
    }
    إذا (o_sec_پای_ټکی) {
        *o_sec_پای_ټکی = عدم;
    }
    إذا (o_معلومات) {
        *o_معلومات = عدم;
    }
    إذا (o_اندازه) {
        *o_اندازه = 0;
    }

    _throwif(m3Err_wasmMalformed, i_پای_ټکی - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    طالما (pos < i_پای_ټکی) {
        بایټونه_t sec_پیل  = pos;
        u8      sectionId = *pos++;
        u32     اندازه      = 0;
_       (ReadLEB_u32(&اندازه, &pos, i_پای_ټکی));
        _throwif(m3Err_wasmUnderrun, اندازه > (size_t)(i_پای_ټکی - pos));
        {
            بایټونه_t payload = pos;
            pos += اندازه;
            إذا (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                إذا (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", وموندل);
                    وموندل = true;
                    إذا (o_sec_پیل) {
                        *o_sec_پیل = sec_پیل;
                    }
                    إذا (o_sec_پای_ټکی) {
                        *o_sec_پای_ټکی = pos;
                    }
                    إذا (o_معلومات) {
                        *o_معلومات = payload;
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
    أرجع پایله;
}

// Builds the custom section name a snapshot of i_name lives under
ساكن
M3_پایله SnapshotSectionName (cstr_t i_name, حرف* o_secName, size_t i_اندازه)
{
    إذا (i_name and *i_name) {
        إذا (مزي_اوږدوالی(i_name) > d_m3_Max_Snapshot_Name_اوږدتیا) {
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
M3_پایله ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_پایله پایله = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = عدم;

    لكل (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->بل) {
        إذا (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    أرجع پایله;
}
#د_که_پای

M3_پایله m3_بارول_Snapshot_From_بفر (IM3Runtime io_runtime, IM3Module i_module, ثابت فراغ* i_بایټونه, size_t i_اندازه)
{
    إذا (!io_runtime || !i_module || !i_بایټونه) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    إذا (i_اندازه >= 8 && memcmp(i_بایټونه, "\0asm", 4) == 0) {
        ثابت u8* معلومات   = عدم;
        u32       اندازه   = 0;
        M3_پایله  پایله = پیدا_کړه_Embedded_Snapshot((ثابت u8*)i_بایټونه, (ثابت u8*)i_بایټونه + i_اندازه,
                                                "snapshot", عدم, عدم, &معلومات, &اندازه);
        إذا (پایله) {
            أرجع پایله;
        }
        إذا (not معلومات) {
            أرجع "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_بفر_Reader br = { معلومات, اندازه, 0 };
            أرجع بارول_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
        }
    }
    {
        M3_بفر_Reader br = { (ثابت u8*)i_بایټونه, i_اندازه, 0 };
        أرجع بارول_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br,
                            i_اندازه < 4 or memcmp(i_بایټونه, c_snapshotMagic, 4) != 0);
    }
#وإلا
    (فراغ)i_اندازه;
    أرجع "snapshots are not available in this build of Wasm3";
#د_که_پای
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
#د_که_پای
}

M3_پایله m3_GetEmbeddedSnapshot (IM3Module i_module, ثابت حرف* i_name,
                                 ثابت فراغ** o_بایټونه, size_t* o_اندازه)
{
    إذا (not i_module or not o_بایټونه or not o_اندازه) {
        أرجع m3Err_mallocFailed;
    }

    *o_بایټونه = عدم;
    *o_اندازه  = 0;

#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_پایله            پایله = ModuleSnapshot(i_module, i_name, &snap);

        إذا (پایله) {
            أرجع پایله;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        *o_بایټونه = snap->معلومات;
        *o_اندازه  = snap->اندازه;

        أرجع m3Err_none;
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#د_که_پای
}

M3_پایله m3_بارول_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name)
{
    إذا (not io_runtime or not i_module) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_پایله            پایله = ModuleSnapshot(i_module, i_name, &snap);
        M3_بفر_Reader      br;

        إذا (پایله) {
            أرجع پایله;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        br.بفر = snap->معلومات;
        br.اندازه   = snap->اندازه;
        br.cursor = 0;

        أرجع بارول_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#د_که_پای
}


M3_پایله m3_Save_Snapshot_منزل_Module (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name,
                                  فراغ** o_بایټونه, size_t* o_اندازه)
{
#إذا d_m3HasSnapshots
    M3_پایله       پایله = m3Err_none;
    M3_بفر_Writer bw     = { عدم, 0, 0 };
    u8*            بهر    = عدم;

    حرف      secName[d_m3_Max_Snapshot_Name_اوږدتیا + حجم("snapshot.")];
    u32       nameLen, name_Len_Leb_اندازه, payload_اندازه, payload_اندازه_Leb_اندازه;
    size_t    orig_اندازه, ټول_اندازه;
    ثابت u8* drop_پیل = عدم;
    ثابت u8* drop_پای_ټکی   = عدم;
    IM3Module saved     = عدم;
    u8*       p;

    إذا (not io_runtime or not i_module or not o_بایټونه or not o_اندازه) {
        أرجع m3Err_mallocFailed;
    }

    *o_بایټونه = عدم;
    *o_اندازه  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_پیل or i_module->wasm_پای_ټکی <= i_module->wasm_پیل);

_   (SnapshotSectionName(i_name, secName, حجم(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->ریښه_Continuation or io_runtime->ریښه_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, بفر_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.اندازه);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.اندازه > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (پیدا_کړه_Embedded_Snapshot(i_module->wasm_پیل, i_module->wasm_پای_ټکی, secName, &drop_پیل, &drop_پای_ټکی, عدم, عدم));

    orig_اندازه           = (size_t)(i_module->wasm_پای_ټکی - i_module->wasm_پیل) - (drop_پیل ? (size_t)(drop_پای_ټکی - drop_پیل) : 0);
    nameLen            = (u32)مزي_اوږدوالی(secName);
    name_Len_Leb_اندازه     = اندازه_د_LEB_u32(nameLen);
    payload_اندازه        = name_Len_Leb_اندازه + nameLen + (u32)bw.اندازه;
    payload_اندازه_Leb_اندازه = اندازه_د_LEB_u32(payload_اندازه);
    ټول_اندازه          = orig_اندازه + 1 + payload_اندازه_Leb_اندازه + payload_اندازه;

    بهر = (u8*)m3_Malloc("wasm_snapshot_module", ټول_اندازه);
    _throwifnull(بهر);

    p = بهر;

    إذا (drop_پیل) {
        size_t before = (size_t)(drop_پیل - i_module->wasm_پیل);
        size_t after  = (size_t)(i_module->wasm_پای_ټکی - drop_پای_ټکی);

        حافظه_نقل(p, i_module->wasm_پیل, before);
        p += before;
        حافظه_نقل(p, drop_پای_ټکی, after);
        p += after;
    } وإلا {
        حافظه_نقل(p, i_module->wasm_پیل, orig_اندازه);
        p += orig_اندازه;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_اندازه);
    p    = EmitLEB_u32(p, nameLen);
    حافظه_نقل(p, secName, nameLen);
    p += nameLen;
    حافظه_نقل(p, bw.بفر, bw.اندازه);

    *o_بایټونه = بهر;
    *o_اندازه  = ټول_اندازه;
    بهر      = عدم;

_catch:
    m3_Free(bw.بفر);
    m3_Free(بهر);

    أرجع پایله;
#وإلا
    (فراغ)io_runtime;
    (فراغ)i_module;
    (فراغ)i_name;
    (فراغ)o_بایټونه;
    (فراغ)o_اندازه;
    أرجع "snapshots are not available in this build of Wasm3";
#د_که_پای
}
