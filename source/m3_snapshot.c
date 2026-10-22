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
ساكن ثابت u32 c_snapshot_نسخو  = 1;

تعداد {
    d_m3SnapshotSection_Meta         = 0,
    d_m3_Snapshot_Section_يادگيري       = 1,
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
#  define d_m3_Max_Snapshot_Name_ڊيگهائي   200

// no index, and no reference
#  define d_m3SnapshotNone            UINT32_MAX
#  define d_m3SnapshotNullRef         UINT64_MAX

// A continuation's state, as the file spells it. A running continuation can
// only be written into a postmortem, where it is as finished as the rest.
تعداد {
    snapshot_contAllocated = 0,
    snapshot_contSuspended = 1,
    snapshot_cont_ختم  = 2
};

// Memory contents go out as runs, so the long stretches of zero or 0xFF that
// most memories are made of cost a few bytes rather than their length
#  define d_m3_Chunk_پڇاڙي                0x00
#  define d_m3ChunkRaw                0x01
#  define d_m3ChunkFillFF             0x02


//---------------------------------------------------------------------------------------------------------------------------------
//سڃاڻپ_عنصر
//---------------------------------------------------------------------------------------------------------------------------------

// Only standard sections affect the module's state. Custom sections (including
// names, debug information and snapshots) can change without changing its identity.
// A module whose bytes are gone has no identity to check, and a snapshot is
// neither saved from one nor loaded into one.
ساكن
M3_نتيجو ModuleFingerprint (IM3Module i_module, u64* o_hash)
{
    M3_نتيجو نتيجو = m3Err_none;
    Xxh64    hash;

    *o_hash = 0;

    _throwif("the module's bytes are gone, so a snapshot cannot tell it from another",
             not i_module->wasm_شروعات or i_module->wasm_پڇاڙي - i_module->wasm_شروعات < 8);

    {
        بائيٽون_t  pos = i_module->wasm_شروعات + 8;
        cbytes_t پڇاڙي = i_module->wasm_پڇاڙي;

        Xxh64_Init(&hash, 0);

        طالما (pos < پڇاڙي) {
            بائيٽون_t شروعات     = pos;
            u8      sectionId = *pos++;
            u32     سائيز      = 0;

_           (ReadLEB_u32(&سائيز, &pos, پڇاڙي));
            _throwif(m3Err_wasmUnderrun, سائيز > (size_t)(پڇاڙي - pos));

            pos += سائيز;
            إذا (sectionId != 0) {
                Xxh64_Update(&hash, شروعات, (size_t)(pos - شروعات));
            }
        }
    }

    *o_hash = Xxh64_Digest(&hash);

_catch:
    أرجع نتيجو;
}


//---------------------------------------------------------------------------------------------------------------------------------
//  safepoints
//---------------------------------------------------------------------------------------------------------------------------------

ساكن
ثابت M3_محفوظ_نقطو* ڳوليو_محفوظ_نقطو (IM3Function i_function, pc_t i_pc, u8 i_قسم)
{
    ثابت M3SnapshotMap* map = i_function->snapshotMap;

    إذا (map) {
        لكل (u32 i = 0; i < map->انگ_محفوظ_Points; ++i) {
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
ثابت M3_محفوظ_نقطو* ڳوليو_محفوظ_نقطو_At (ثابت M3SnapshotMap* i_map, u32 i_wasm_وٿي, u8 i_قسم, u32 i_هدف_Loop)
{
    u32 هيٺيون  = 0;
    u32 مٿاهون = i_map->انگ_محفوظ_Points;

    طالما (هيٺيون < مٿاهون) {
        u32 وچ = هيٺيون + (مٿاهون - هيٺيون) / 2;

        إذا (i_map->محفوظ_Points[وچ].wasm_وٿي < i_wasm_وٿي) {
            هيٺيون = وچ + 1;
        } وإلا {
            مٿاهون = وچ;
        }
    }

    لكل (u32 i = هيٺيون; i < i_map->انگ_محفوظ_Points and i_map->محفوظ_Points[i].wasm_وٿي == i_wasm_وٿي; ++i) {
        ثابت M3_محفوظ_نقطو* نقطو = &i_map->محفوظ_Points[i];

        إذا (نقطو->قسم != i_قسم or (نقطو->flags & d_m3_محفوظ_نقطو_پڇ_Call)) {
            استمر;
        }
        إذا (i_قسم != safepoint_op or i_map->بلاڪون[نقطو->aux].wasm_وٿي == i_هدف_Loop) {
            أرجع نقطو;
        }
    }

    أرجع عدم;
}

ساكن
ثابت M3_بلاڪ_شروعات* ڳوليو_بلاڪ_At (ثابت M3SnapshotMap* i_map, u8 i_opcode, u32 i_wasm_وٿي)
{
    لكل (u32 i = 0; i < i_map->انگ_بلاڪون; ++i) {
        إذا (i_map->بلاڪون[i].opcode == i_opcode and i_map->بلاڪون[i].wasm_وٿي == i_wasm_وٿي) {
            أرجع &i_map->بلاڪون[i];
        }
    }

    أرجع عدم;
}

// Whether a loop or try_table encloses the instruction at i_wasmOffset (see
// M3BlockStart)
ساكن مضمن
bool Encloses_وٿي (ثابت M3_بلاڪ_شروعات* i_بلاڪ, u32 i_wasm_وٿي)
{
    أرجع i_بلاڪ->wasm_وٿي < i_wasm_وٿي and i_wasm_وٿي < i_بلاڪ->wasm_پڇاڙي;
}

// Whether the handlers of a try_table around a safepoint are in force there.
// A back edge has retired those of the try_tables it leaves: the ones inside
// the loop it goes to.
ساكن مضمن
bool Handlers_جيئرو_At (ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطو* i_نقطو, ثابت M3_بلاڪ_شروعات* i_try)
{
    أرجع not (i_نقطو->قسم == safepoint_op and i_try->wasm_وٿي > i_map->بلاڪون[i_نقطو->aux].wasm_وٿي);
}

// The type a call safepoint's instruction calls through, read back out of the
// caller's body: what its callee has to return
ساكن
M3_نتيجو CallSiteType (IM3Function i_caller, ثابت M3_محفوظ_نقطو* i_نقطو, IM3FuncType* o_type)
{
    M3_نتيجو   نتيجو = m3Err_none;
    IM3Module  module = i_caller->module;
    بائيٽون_t    pos    = i_caller->wasm;
    cbytes_t   پڇاڙي    = i_caller->wasm_پڇاڙي;
    u32        سائيز   = 0;
    u32        index  = 0;
    m3opcode_t opcode = 0;

    *o_type = عدم;

_   (ReadLEB_u32(&سائيز, &pos, پڇاڙي));
    _throwif(m3Err_wasmUnderrun, i_نقطو->wasm_وٿي >= (size_t)(پڇاڙي - pos));
    pos += i_نقطو->wasm_وٿي;

_   (Read_opcode(&opcode, &pos, پڇاڙي));
_   (ReadLEB_u32(&index, &pos, پڇاڙي));

    إذا (opcode == c_waOp_call) {
        _throwif(m3Err_wasmMalformed, index >= module->انگ_Functions);
        *o_type = module->functions[index].funcType;
    } وإلا {
        _throwif(m3Err_wasmMalformed, opcode != c_wa_Op_call_اڻسڌو and opcode != c_waOp_callRef);
        _throwif(m3Err_wasmMalformed, index >= module->انگ_Func_Types);
        *o_type = module->funcTypes[index];
    }

_catch:
    أرجع نتيجو;
}

// Whether a function can stand where i_expected was called: it returns as
// many values, each a subtype of the one expected - which is what a
// return_call asks of the function it hands its frame to
ساكن
bool ReturnsWhat (IM3FuncType i_type, IM3FuncType i_expected)
{
    u16 انگ_Results = Get_Func_Type_انگ_Results(i_expected);

    إذا (not i_type or Get_Func_Type_انگ_Results(i_type) != انگ_Results) {
        أرجع false;
    }

    لكل (u16 i = 0; i < انگ_Results; ++i) {
        إذا (not Is_Sub_Type_جو(Get_Func_Type_نتيجو_Type(i_type, i), Get_Func_Type_نتيجو_Type(i_expected, i))) {
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
تعريفنوع بنية M3_چوکٺو_Registers {
    m3reg_t* r0;
#  إذا d_m3HasFloat
    f64* fp0;
#  endif
} M3_چوکٺو_Registers;

// The stack a continuation's frames are on: the runtime's own for the root
ساكن
M3_نتيجو Continuation_ڍير (IM3Runtime i_runtime, IM3Continuation i_cont, m3slot_t** o_بنياد, u32* o_انگ_Slots)
{
    إذا (i_cont == i_runtime->پاڙ_Continuation) {
        *o_بنياد     = (m3slot_t*)i_runtime->origin_ڍير;
        *o_انگ_Slots = i_runtime->انگ_ڍير_Slots;
    } وإلا {
        *o_بنياد     = i_cont->val_ڍير;
        *o_انگ_Slots = i_cont->انگ_ڍير_Slots;
    }

    أرجع (*o_بنياد) ? m3Err_none : "a suspended continuation has no stack";
}

// Where a value is kept, or NULL for a register; o_bytes is how much of the
// frame it takes, which has to be inside the stack the frame is on
ساكن
M3_نتيجو Locate_قدر (m3slot_t* i_بنياد, u32 i_انگ_Slots, m3stack_t i_sp, ثابت M3_جڳهه_قدر* i_قدر, u8** o_where)
{
    M3_نتيجو نتيجو = m3Err_none;
    u8       type   = بنياد_Type_جو(i_قدر->type);

    *o_where = عدم;

    _throwif(m3Err_wasmMalformed, type == c_m3Type_none or type > c_m3Type_contref);

    // no operation reads or writes one, so a v128 only ever sits in slots
    _throwif("a v128 value is kept in a register", type == c_m3Type_v128 and Is_رجسٽر_جڳهه_Alias(i_قدر->جڳهه));

    إذا (not Is_رجسٽر_جڳهه_Alias(i_قدر->جڳهه)) {
        size_t سائيز   = (type == c_m3Type_v128) ? 16
                                                : (IsRefType(type) ? حجم(فراغ*)
                                                                   : (Is64_ٻٽو_انگ_Type(type) ? حجم(u64) : حجم(u32)));
        size_t offset = (size_t)((m3slot_t*)i_sp - i_بنياد + i_قدر->جڳهه) * حجم(m3slot_t);

        _throwif("a value is outside the stack its frame is on", offset + سائيز > (size_t)i_انگ_Slots * حجم(m3slot_t));

        *o_where = (u8*)i_بنياد + offset;
    }

_catch:
    أرجع نتيجو;
}

// A value as the file holds it before a reference is named: a number's bits,
// zero-extended to 64, or the reference itself
ساكن
M3_نتيجو Read_قدر (u8* i_where, ثابت M3_چوکٺو_Registers* i_registers, ثابت M3_جڳهه_قدر* i_قدر, u64* o_بٽ,
                    فراغ** o_reference)
{
    M3_نتيجو نتيجو = m3Err_none;
    u8       type   = بنياد_Type_جو(i_قدر->type);

    *o_بٽ      = 0;
    *o_reference = عدم;

    إذا (i_where) {
        إذا (IsRefType(type)) {
            يادگيري_نقل(o_reference, i_where, حجم(فراغ*));
        } وإلا إذا (Is64_ٻٽو_انگ_Type(type)) {
            يادگيري_نقل(o_بٽ, i_where, حجم(u64));
        } وإلا {
            u32 narrow;
            يادگيري_نقل(&narrow, i_where, حجم(narrow));
            *o_بٽ = narrow;
        }
    } وإلا إذا (Is_Fp_رجسٽر_جڳهه_Alias(i_قدر->جڳهه)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            f32 narrow = (f32)*i_registers->fp0;
            u32 بٽ;
            يادگيري_نقل(&بٽ, &narrow, حجم(بٽ));
            *o_بٽ = بٽ;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            يادگيري_نقل(o_بٽ, i_registers->fp0, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        m3reg_t r0 = *i_registers->r0;

        إذا (IsRefType(type)) {
            *o_reference = (فراغ*)(uintptr_t)r0;
        } وإلا إذا (Is64_ٻٽو_انگ_Type(type)) {
            *o_بٽ = (u64)r0;
        } وإلا {
            *o_بٽ = (u32)r0;
        }
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو Write_قدر (u8* o_where, ثابت M3_چوکٺو_Registers* i_registers, ثابت M3_جڳهه_قدر* i_قدر, u64 i_بٽ,
                     فراغ* i_reference)
{
    M3_نتيجو نتيجو = m3Err_none;
    u8       type   = بنياد_Type_جو(i_قدر->type);

    إذا (o_where) {
        إذا (IsRefType(type)) {
            يادگيري_نقل(o_where, &i_reference, حجم(فراغ*));
        } وإلا إذا (Is64_ٻٽو_انگ_Type(type)) {
            يادگيري_نقل(o_where, &i_بٽ, حجم(u64));
        } وإلا {
            u32 narrow = (u32)i_بٽ;
            يادگيري_نقل(o_where, &narrow, حجم(narrow));
        }
    } وإلا إذا (Is_Fp_رجسٽر_جڳهه_Alias(i_قدر->جڳهه)) {
#  إذا d_m3HasFloat
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->fp0);

        إذا (type == c_m3Type_f32) {
            u32 بٽ = (u32)i_بٽ;
            f32 narrow;
            يادگيري_نقل(&narrow, &بٽ, حجم(narrow));
            *i_registers->fp0 = narrow;
        } وإلا {
            _throwif(m3Err_wasmMalformed, type != c_m3Type_f64);
            يادگيري_نقل(i_registers->fp0, &i_بٽ, حجم(u64));
        }
#  وإلا
        _throw(m3Err_wasmMalformed);
#  endif
    } وإلا {
        _throwif("a value is kept in a register the frame does not hold", not i_registers or not i_registers->r0);

        إذا (IsRefType(type)) {
            *i_registers->r0 = (m3reg_t)(uintptr_t)i_reference;
        } وإلا {
            *i_registers->r0 = (m3reg_t)i_بٽ;
        }
    }

_catch:
    أرجع نتيجو;
}

// The value a safepoint lists at i_index: the function's locals first, then
// the operand stack
ساكن
ثابت M3_جڳهه_قدر* چوکٺو_قدر (ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطو* i_نقطو, u32 i_اشاريو)
{
    إذا (i_اشاريو < i_map->انگ_Locals) {
        أرجع i_نقطو->پهريون_Local_قدر == UINT32_MAX ? &i_map->locals[i_اشاريو]
                                                      : &i_map->قدرون[i_نقطو->پهريون_Local_قدر + i_اشاريو];
    }
    أرجع &i_map->قدرون[i_نقطو->پهريون_قدر + i_اشاريو - i_map->انگ_Locals];
}

// A frame's constants are copied in by op_Entry, and op_Entry already ran
ساكن
فراغ RestoreConstants (IM3Function i_function, m3stack_t i_sp)
{
    إذا (i_function->constants) {
        u8* where = (u8*)((m3slot_t*)i_sp + i_function->انگ_Ret_And_Arg_Slots) + i_function->انگ_Local_بائيٽون;

        يادگيري_نقل(where, i_function->constants, i_function->انگ_Constant_بائيٽون);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------
//  pointer ids
//---------------------------------------------------------------------------------------------------------------------------------

// Pointers numbered in the order they were first added, with a hash index so
// the lookup does not have to walk them
تعريفنوع بنية M3_اشارو_Ids {
    فراغ** شيون;
    u32    ڳڻپ;
    u32    صلاحيت;

    u32* index;               // id + 1, or 0 for an empty bucket
    u32  اشاريو_صلاحيت;       // a power of two, kept at least twice count
} M3_اشارو_Ids;

ساكن
u32 اشارو_ٻالٽي (فراغ* i_اشارو, u32 i_صلاحيت)
{
    u64 بٽ = (u64)(uintptr_t)i_اشارو;

    أرجع (u32)((بٽ * 0x9E3779B97F4A7C15ULL) >> 32) & (i_صلاحيت - 1);
}

ساكن
u32 اشارو_Ids_ڳوليو (ثابت M3_اشارو_Ids* i_ids, فراغ* i_اشارو)
{
    إذا (i_ids->اشاريو_صلاحيت) {
        u32 ٻالٽي = اشارو_ٻالٽي(i_اشارو, i_ids->اشاريو_صلاحيت);

        طالما (i_ids->index[ٻالٽي]) {
            u32 id = i_ids->index[ٻالٽي] - 1;

            إذا (i_ids->شيون[id] == i_اشارو) {
                أرجع id;
            }

            ٻالٽي = (ٻالٽي + 1) & (i_ids->اشاريو_صلاحيت - 1);
        }
    }

    أرجع d_m3SnapshotNone;
}

ساكن
M3_نتيجو اشارو_Ids_جوڙيو (M3_اشارو_Ids* io_ids, فراغ* i_اشارو, u32* o_id)
{
    u32 id = اشارو_Ids_ڳوليو(io_ids, i_اشارو);

    إذا (id != d_m3SnapshotNone) {
        *o_id = id;
        أرجع m3Err_none;
    }

    إذا (io_ids->ڳڻپ == io_ids->صلاحيت) {
        u32    صلاحيت = io_ids->صلاحيت ? io_ids->صلاحيت * 2 : 16;
        فراغ** شيون    = m3_ReallocArray(فراغ*, io_ids->شيون, صلاحيت, io_ids->صلاحيت);

        إذا (not شيون) {
            أرجع m3Err_mallocFailed;
        }

        io_ids->شيون    = شيون;
        io_ids->صلاحيت = صلاحيت;
    }

    إذا ((io_ids->ڳڻپ + 1) * 2 > io_ids->اشاريو_صلاحيت) {
        u32  صلاحيت = io_ids->اشاريو_صلاحيت ? io_ids->اشاريو_صلاحيت * 2 : 32;
        u32* index    = m3_AllocArray(u32, صلاحيت);

        إذا (not index) {
            أرجع m3Err_mallocFailed;
        }

        لكل (u32 i = 0; i < io_ids->ڳڻپ; ++i) {
            u32 ٻالٽي = اشارو_ٻالٽي(io_ids->شيون[i], صلاحيت);

            طالما (index[ٻالٽي]) {
                ٻالٽي = (ٻالٽي + 1) & (صلاحيت - 1);
            }

            index[ٻالٽي] = i + 1;
        }

        m3_Free(io_ids->index);
        io_ids->index         = index;
        io_ids->اشاريو_صلاحيت = صلاحيت;
    }

    id                = io_ids->ڳڻپ++;
    io_ids->شيون[id] = i_اشارو;

    u32 ٻالٽي = اشارو_ٻالٽي(i_اشارو, io_ids->اشاريو_صلاحيت);

    طالما (io_ids->index[ٻالٽي]) {
        ٻالٽي = (ٻالٽي + 1) & (io_ids->اشاريو_صلاحيت - 1);
    }

    io_ids->index[ٻالٽي] = id + 1;

    *o_id = id;

    أرجع m3Err_none;
}

ساكن
فراغ اشارو_Ids_Free (M3_اشارو_Ids* io_ids)
{
    m3_Free(io_ids->شيون);
    m3_Free(io_ids->index);
    memset(io_ids, 0, حجم(*io_ids));
}


//---------------------------------------------------------------------------------------------------------------------------------
//  buffers
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3_بفر_Writer {
    u8*    بفر;
    size_t سائيز;
    size_t صلاحيت;
} M3_بفر_Writer;

ساكن
M3_نتيجو بفر_Writer_Write (ثابت فراغ* i_معلومات, size_t i_سائيز, فراغ* i_userdata)
{
    M3_بفر_Writer* bw = (M3_بفر_Writer*)i_userdata;

    إذا (bw->سائيز + i_سائيز > bw->صلاحيت) {
        size_t newCap = bw->صلاحيت ? bw->صلاحيت * 2 : 4096;
        طالما (bw->سائيز + i_سائيز > newCap) {
            newCap *= 2;
        }
        u8* newBuf = m3_ReallocArray(u8, bw->بفر, newCap, bw->صلاحيت);
        إذا (!newBuf) {
            أرجع m3Err_mallocFailed;
        }
        bw->بفر   = newBuf;
        bw->صلاحيت = newCap;
    }

    إذا (i_سائيز) {
        يادگيري_نقل(bw->بفر + bw->سائيز, i_معلومات, i_سائيز);
        bw->سائيز += i_سائيز;
    }

    أرجع m3Err_none;
}

تعريفنوع بنية M3_بفر_Reader {
    ثابت u8* بفر;
    size_t    سائيز;
    size_t    cursor;
} M3_بفر_Reader;

ساكن
M3_نتيجو بفر_Reader_Read (فراغ* o_بفر, size_t i_سائيز, فراغ* i_userdata)
{
    M3_بفر_Reader* br = (M3_بفر_Reader*)i_userdata;

    إذا (i_سائيز > br->سائيز - br->cursor) {
        أرجع m3Err_wasmMalformed;
    }

    إذا (i_سائيز) {
        يادگيري_نقل(o_بفر, br->بفر + br->cursor, i_سائيز);
        br->cursor += i_سائيز;
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

    M3_اشارو_Ids جيئرو_Continuations;     // everything the runtime still holds, to check
    M3_اشارو_Ids جيئرو_Exceptions;        //   a reference against before following it
    M3_اشارو_Ids continuations;         // what the snapshot carries, by id
    M3_اشارو_Ids exceptions;
} M3SnapshotSave;

ساكن مضمن
u8 M3_Type_منزل_Val_Type (m3type_t i_type)
{
    بدل (بنياد_Type_جو(i_type)) {
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
M3_نتيجو Put (M3SnapshotSave* s, ثابت فراغ* i_معلومات, size_t i_سائيز)
{
    أرجع s->writing ? s->writer(i_معلومات, i_سائيز, s->userdata) : m3Err_none;
}

ساكن
M3_نتيجو PutU8 (M3SnapshotSave* s, u8 i_قدر)
{
    أرجع Put(s, &i_قدر, حجم(i_قدر));
}


ساكن
M3_نتيجو PutLEB_u32 (M3SnapshotSave* s, u32 i_قدر)
{
    u8  بائيٽون[5];
    u32 len = 0;

    افعل {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        إذا (i_قدر != 0) {
            byte |= 0x80;
        }
        بائيٽون[len++] = byte;
    } طالما (i_قدر != 0);

    أرجع Put(s, بائيٽون, len);
}

ساكن
M3_نتيجو PutLEB_u64 (M3SnapshotSave* s, u64 i_قدر)
{
    u8  بائيٽون[10];
    u32 len = 0;

    افعل {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        إذا (i_قدر != 0) {
            byte |= 0x80;
        }
        بائيٽون[len++] = byte;
    } طالما (i_قدر != 0);

    أرجع Put(s, بائيٽون, len);
}

ساكن
M3_نتيجو PutLEB_i64 (M3SnapshotSave* s, i64 i_قدر)
{
    u8   بائيٽون[10];
    u32  len  = 0;
    bool وڌيڪ = true;

    طالما (وڌيڪ) {
        u8 byte = i_قدر & 0x7F;
        i_قدر >>= 7;
        إذا ((i_قدر == 0 && !(byte & 0x40)) || (i_قدر == -1 && (byte & 0x40))) {
            وڌيڪ = false;
        } وإلا {
            byte |= 0x80;
        }
        بائيٽون[len++] = byte;
    }

    أرجع Put(s, بائيٽون, len);
}

// What one section writes. Every section body takes the save state and nothing
// else, so SaveSection can drive them all the same way.
تعريفنوع M3_نتيجو (*M3SnapshotSectionFn)(M3SnapshotSave* s);

// Writes one section: the body into a buffer first, so its length can go in
// front of it, then the whole thing to the real writer. The buffer writer is
// swapped onto s itself rather than onto a copy of it - a body may name a
// reference, and the ids that name one belong to s, not to a copy that is
// about to be thrown away.
ساكن
M3_نتيجو SaveSection (M3SnapshotSave* s, u8 i_sectionId, M3SnapshotSectionFn i_body)
{
    M3_نتيجو         نتيجو   = m3Err_none;
    M3_بفر_Writer   bw       = { عدم, 0, 0 };
    M3SnapshotWriter writer   = s->writer;
    فراغ*            userdata = s->userdata;

    s->writer   = بفر_Writer_Write;
    s->userdata = &bw;

    نتيجو = i_body(s);

    s->writer   = writer;
    s->userdata = userdata;

    إذا (نتيجو) {
        اذهب _catch;
    }

    // a section with nothing in it is left out
    إذا (bw.سائيز) {
        _throwif("snapshot section is too large", bw.سائيز > UINT32_MAX);
_       (PutU8(s, i_sectionId));
_       (PutLEB_u32(s, (u32)bw.سائيز));
_       (Put(s, bw.بفر, bw.سائيز));
    }

_catch:
    m3_Free(bw.بفر);

    أرجع نتيجو;
}

ساكن
u32 Function_اشاريو (IM3Module i_module, IM3Function i_function)
{
    إذا (i_function and i_function >= i_module->functions and i_function < i_module->functions + i_module->انگ_Functions) {
        أرجع (u32)(i_function - i_module->functions);
    }

    أرجع d_m3SnapshotNone;
}

ساكن
u32 Func_Type_اشاريو (IM3Module i_module, IM3FuncType i_type)
{
    لكل (u32 i = 0; i < i_module->انگ_Func_Types; ++i) {
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
IM3_ٽيگ ٽيگ_جو_اشاريو (IM3Module i_module, u32 i_اشاريو)
{
    IM3_ٽيگ ٽيگ = &i_module->ٽيگ_فهرست[i_اشاريو];

    أرجع ٽيگ->resolved ? ٽيگ->resolved : ٽيگ;
}

ساكن
u32 ٽيگ_اشاريو_2 (IM3Module i_module, IM3_ٽيگ i_ٽيگ)
{
    لكل (u32 i = 0; i < i_module->انگ_ٽيگ_فهرست; ++i) {
        إذا (ٽيگ_جو_اشاريو(i_module, i) == i_ٽيگ) {
            أرجع i;
        }
    }

    أرجع d_m3SnapshotNone;
}

#  endif

// The word a reference is written as: a function index, an object id, the
// embedder's name for an externref, or d_m3SnapshotNullRef
ساكن
M3_نتيجو NameReference (M3SnapshotSave* s, u8 i_type, فراغ* i_reference, u64* o_لفظ)
{
    M3_نتيجو نتيجو = m3Err_none;
    u32      id     = d_m3SnapshotNone;

    *o_لفظ = d_m3SnapshotNullRef;

    إذا (not i_reference) {
        أرجع m3Err_none;
    }

    إذا (i_type == c_m3Type_funcref) {
        id = Function_اشاريو(s->module, (IM3Function)i_reference);
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
                 اشارو_Ids_ڳوليو(&s->جيئرو_Exceptions, i_reference) == d_m3SnapshotNone);

        u32 ڳڻپ = s->exceptions.ڳڻپ;
_       (اشارو_Ids_جوڙيو(&s->exceptions, i_reference, &id));
        _throwif("internal: an exception was found only while writing", s->writing and s->exceptions.ڳڻپ != ڳڻپ);
    }
#  endif
    وإلا إذا (i_type == c_m3Type_contref) {
        _throwif("a contref names a continuation that no longer exists",
                 اشارو_Ids_ڳوليو(&s->جيئرو_Continuations, i_reference) == d_m3SnapshotNone);

        u32 ڳڻپ = s->continuations.ڳڻپ;
_       (اشارو_Ids_جوڙيو(&s->continuations, i_reference, &id));
        _throwif("internal: a continuation was found only while writing", s->writing and s->continuations.ڳڻپ != ڳڻپ);
    } وإلا {
        _throw("a reference of an unknown type");
    }

    *o_لفظ = id;

_catch:
    أرجع نتيجو;
}

//// A value held in memory - a global's cell, a bound argument - as the file
// holds it: a reference by name, a 64-bit value whole, anything narrower
// zero-extended
ساكن
M3_نتيجو Put_Ref_قدر (M3SnapshotSave* s, u8 i_type, فراغ* i_reference)
{
    M3_نتيجو نتيجو = m3Err_none;
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
    أرجع نتيجو;
}

// The fixed-width quantities: the container's version, and a float's bit
// pattern. Little endian whatever the host is. Everything else in the file is
// a LEB128, but a float has to survive as an exact 32- or 64-bit pattern, NaN
// payloads and all, and the version has to sit where a reader can find it
// before it knows anything else about the file.
ساكن
M3_نتيجو PutLE32 (M3SnapshotSave* s, u32 i_قدر)
{
    u8 بائيٽون[4];

    لكل (u32 i = 0; i < حجم(بائيٽون); ++i) {
        بائيٽون[i] = (u8)(i_قدر >> (8 * i));
    }

    أرجع Put(s, بائيٽون, حجم(بائيٽون));
}

ساكن
M3_نتيجو PutLE64 (M3SnapshotSave* s, u64 i_قدر)
{
    u8 بائيٽون[8];

    لكل (u32 i = 0; i < حجم(بائيٽون); ++i) {
        بائيٽون[i] = (u8)(i_قدر >> (8 * i));
    }

    أرجع Put(s, بائيٽون, حجم(بائيٽون));
}

// The body of a value the caller already holds as bits, with a narrow type in
// the low bits - what ReadValue hands back for a frame slot, and what an
// exception payload is. Bits rather than a location: on a big-endian host the
// low half of a u64 is not the half its address points at.
ساكن
M3_نتيجو Put_قدر_Body (M3SnapshotSave* s, m3type_t i_type, u64 i_بٽ)
{
    إذا (IsRefType(i_type)) {
        أرجع Put_Ref_قدر(s, بنياد_Type_جو(i_type), (فراغ*)(uintptr_t)i_بٽ);
    }

    بدل (بنياد_Type_جو(i_type)) {
    حالة c_m3Type_i32: أرجع PutLEB_i64(s, (i32)(u32)i_بٽ);
    حالة c_m3Type_i64: أرجع PutLEB_i64(s, (i64)i_بٽ);
    حالة c_m3Type_f32: أرجع PutLE32(s, (u32)i_بٽ);
    حالة c_m3Type_f64: أرجع PutLE64(s, i_بٽ);
    افتراضي: أرجع "a snapshot cannot hold a value of this type as bits";
    }
}

ساكن
M3_نتيجو Put_قدر_ٽيگ (M3SnapshotSave* s, m3type_t i_type)
{
    u8 valtype = M3_Type_منزل_Val_Type(i_type);

    إذا (not valtype) {
        أرجع "a snapshot has no tag for a value of this type";
    }

    أرجع PutU8(s, valtype);
}

ساكن
M3_نتيجو Put_قدر_بٽ (M3SnapshotSave* s, m3type_t i_type, u64 i_بٽ)
{
    M3_نتيجو نتيجو = m3Err_none;

_   (Put_قدر_ٽيگ(s, i_type));
_   (Put_قدر_Body(s, i_type, i_بٽ));

_catch:
    أرجع نتيجو;
}

// A value as a memory location holds it: a global's cell, a bound argument, a
// table element. Narrow types are read at their own width first, so what goes
// on the wire is the value and not the bytes around it.
ساكن
M3_نتيجو Put_قدر (M3SnapshotSave* s, m3type_t i_type, ثابت فراغ* i_where)
{
    M3_نتيجو نتيجو = m3Err_none;

_   (Put_قدر_ٽيگ(s, i_type));

    إذا (IsRefType(i_type)) {
        فراغ* reference;
        يادگيري_نقل(&reference, i_where, حجم(reference));
_       (Put_قدر_Body(s, i_type, (u64)(uintptr_t)reference));
    } وإلا إذا (بنياد_Type_جو(i_type) == c_m3Type_v128) {
        // a v128 is a vector of bytes, and has no byte order of its own
        u8 بائيٽون[16];
        يادگيري_نقل(بائيٽون, i_where, حجم(بائيٽون));
_       (Put(s, بائيٽون, حجم(بائيٽون)));
    } وإلا إذا (Is64_ٻٽو_انگ_Type(i_type)) {
        u64 بٽ;
        يادگيري_نقل(&بٽ, i_where, حجم(بٽ));
_       (Put_قدر_Body(s, i_type, بٽ));
    } وإلا {
        u32 narrow;
        يادگيري_نقل(&narrow, i_where, حجم(narrow));
_       (Put_قدر_Body(s, i_type, narrow));
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو Put_يادگيري_Chunks (M3SnapshotSave* s, ثابت u8* i_بائيٽون, size_t i_سائيز)
{
    M3_نتيجو نتيجو = m3Err_none;
    size_t   cursor = 0;

    طالما (cursor < i_سائيز) {
        u8     byte = i_بائيٽون[cursor];
        size_t لاڳيتو  = 1;

        طالما (cursor + لاڳيتو < i_سائيز and i_بائيٽون[cursor + لاڳيتو] == byte) {
            لاڳيتو++;
        }

        إذا ((byte == 0x00 or byte == 0xFF) and لاڳيتو >= d_m3_Snapshot_لاڳيتو_حد) {
            // the memory is zeroed before it is filled in, so a run of zeros
            // is written as nothing at all
            إذا (byte == 0xFF) {
_               (PutU8(s, d_m3ChunkFillFF));
_               (PutLEB_u32(s, (u32)cursor));
_               (PutLEB_u32(s, (u32)لاڳيتو));
            }

            cursor += لاڳيتو;
            استمر;
        }

        size_t raw_شروعات = cursor;

        طالما (cursor < i_سائيز) {
            u8     b      = i_بائيٽون[cursor];
            size_t ڊيگهائي = 1;

            طالما (cursor + ڊيگهائي < i_سائيز and i_بائيٽون[cursor + ڊيگهائي] == b) {
                ڊيگهائي++;
            }

            إذا ((b == 0x00 or b == 0xFF) and ڊيگهائي >= d_m3_Snapshot_لاڳيتو_حد) {
                اكسر;
            }

            cursor += ڊيگهائي;
        }

_       (PutU8(s, d_m3ChunkRaw));
_       (PutLEB_u32(s, (u32)raw_شروعات));
_       (PutLEB_u32(s, (u32)(cursor - raw_شروعات)));
_       (Put(s, i_بائيٽون + raw_شروعات, cursor - raw_شروعات));
    }

_   (PutU8(s, d_m3_Chunk_پڇاڙي));

_catch:
    أرجع نتيجو;
}

// Two imports can name the same memory or table, so one can stand at several
// indices. The file stores it once, at the first of them, and the record at
// any other index names that first one instead.
ساكن
u32 يادگيري_پهريون_اشاريو (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->memories[j] != module->memories[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
u32 جدول_پهريون_اشاريو (IM3Module module, u32 index)
{
    u32 j = 0;
    طالما (module->tables[j] != module->tables[index]) {
        ++j;
    }
    أرجع j;
}

ساكن
M3_نتيجو SaveMemories (M3SnapshotSave* s)
{
    M3_نتيجو  نتيجو = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->انگ_Memories));

    لكل (u32 m = 0; m < module->انگ_Memories; ++m) {
        IM3_يادگيري يادگيري   = module->memories[m];
        u32       پهريون    = يادگيري_پهريون_اشاريو(module, m);
        bool      has_معلومات  = يادگيري and يادگيري->mallocated;
        u32       صفحو_سائيز = يادگيري ? يادگيري_صفحو_سائيز(يادگيري) : d_m3_Default_Mem_صفحو_سائيز;
        u64       بائيٽون    = يادگيري ? يادگيري->انگ_صفحا * (u64)صفحو_سائيز : 0;
        u32       صفحو_بٽ = 0;

_       (PutLEB_u32(s, m));
_       (PutLEB_u32(s, پهريون));

        إذا (پهريون != m) {
            استمر;
        }

        // a shared memory belongs to every runtime that maps it, and the file
        // holds the state of one
        _throwif("a shared memory cannot be saved in a snapshot", يادگيري and يادگيري->isShared);

        // the file counts pages and addresses bytes in 32 bits, whatever size
        // the memory's pages are, so one this large has no encoding here
        _throwif("the memory is too large for a snapshot", بائيٽون > UINT32_MAX);
        _throwif("shrunken memory cannot be saved in a snapshot",
                 يادگيري and يادگيري->mallocated and يادگيري->mallocated->ڊيگهائي < بائيٽون);

        // a page size is a power of two, written the way custom page sizes
        // writes one: as that power
        طالما ((1u << صفحو_بٽ) < صفحو_سائيز) {
            ++صفحو_بٽ;
        }

_       (PutLEB_u32(s, صفحو_بٽ));
_       (PutLEB_u32(s, يادگيري ? (u32)يادگيري->انگ_صفحا : 0));

        // linear memory is little endian whatever the host is, and nothing in
        // it is a reference, so the discovery pass has no reason to read it
        إذا (not s->writing) {
            استمر;
        }

        إذا (has_معلومات) {
_           (Put_يادگيري_Chunks(s, m3_Mem_معلومات(يادگيري->mallocated), (size_t)بائيٽون));
        } وإلا {
_           (PutU8(s, d_m3_Chunk_پڇاڙي));
        }
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو SaveGlobals (M3SnapshotSave* s)
{
    M3_نتيجو  نتيجو = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->انگ_Globals));

    لكل (u32 g = 0; g < module->انگ_Globals; ++g) {
        M3Global* global = &module->globals[g];
        M3Global* cell   = global->resolved ? global->resolved : global;

_       (PutLEB_u32(s, g));
_       (Put_قدر(s, global->type, &cell->i64_قدر));
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو SaveTables (M3SnapshotSave* s)
{
    M3_نتيجو  نتيجو = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->انگ_Tables));

    لكل (u32 t = 0; t < module->انگ_Tables; ++t) {
        IM3_جدول جدول_2 = module->tables[t];
        u32      پهريون = جدول_پهريون_اشاريو(module, t);

_       (PutLEB_u32(s, t));
_       (PutLEB_u32(s, پهريون));

        // the first record holds the elements, and the discovery pass has
        // followed their references there
        إذا (پهريون != t) {
            استمر;
        }

_       (PutU8(s, M3_Type_منزل_Val_Type(جدول_2->type)));
_       (PutLEB_u32(s, جدول_2->سائيز));

        لكل (u32 e = 0; e < جدول_2->سائيز; ++e) {
_           (Put_Ref_قدر(s, بنياد_Type_جو(جدول_2->type), جدول_2->elements[e]));
        }
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو SaveSegments (M3SnapshotSave* s)
{
    M3_نتيجو  نتيجو = m3Err_none;
    IM3Module module = s->module;

_   (PutLEB_u32(s, module->انگ_معلومات_Segments));

    لكل (u32 i = 0; i < module->انگ_معلومات_Segments; ++i) {
_       (PutU8(s, module->معلومات_Segments[i].dropped));
    }

_   (PutLEB_u32(s, module->انگ_Element_Segments));

    لكل (u32 i = 0; i < module->انگ_Element_Segments; ++i) {
_       (PutU8(s, module->elementSegments[i].dropped));
    }

_catch:
    أرجع نتيجو;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_نتيجو SaveExceptions (M3SnapshotSave* s)
{
    M3_نتيجو نتيجو = m3Err_none;

_   (PutLEB_u32(s, s->exceptions.ڳڻپ));

    لكل (u32 i = 0; i < s->exceptions.ڳڻپ; ++i) {
        M3Exception* exn  = (M3Exception*)s->exceptions.شيون[i];
        IM3FuncType  type = exn->ٽيگ->type;

        _throwif(m3Err_wasmMalformed, exn->انگ_دليلون != Get_Func_Type_انگ_Params(type));

_       (PutLEB_u32(s, i));
_       (PutLEB_u32(s, exn->انگ_دليلون));

        لكل (u32 a = 0; a < exn->انگ_دليلون; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
_           (Put_قدر_بٽ(s, argType, exn->دليلون[a]));
        }
    }

_catch:
    أرجع نتيجو;
}

#  endif

// One function's frame within a suspended continuation, as the native frames
// the interpreter recorded describe it
تعريفنوع بنية M3Activation {
    IM3Function        function;
    ثابت M3_محفوظ_نقطو* نقطو;           // the safepoint it stands at
    m3stack_t          sp;
    pc_t               pc;
    u8                 قسم;            // M3SafePointKind
    M3_چوکٺو_Registers   registers;       // the ones it goes on with
    ثابت M3_چوکٺو*     resume;          // the resume it waits in, for safepoint_resume
    i32                پهريون_چوکٺو;      // its frames, outermost first: firstFrame down to lastFrame
    i32                آخري_چوکٺو;
} M3Activation;

// The safepoint a frame stands at, and so the function it stands in. That is
// the function the call into it named, io_function, unless a tail call has
// since replaced it; then it is whichever function has a safepoint at i_pc.
ساكن
ثابت M3_محفوظ_نقطو* ڳوليو_چوکٺو_محفوظ_نقطو (IM3Module i_module, IM3Function* io_function, pc_t i_pc, u8 i_قسم)
{
    ثابت M3_محفوظ_نقطو* نقطو = *io_function ? ڳوليو_محفوظ_نقطو(*io_function, i_pc, i_قسم) : عدم;

    لكل (u32 i = 0; not نقطو and i < i_module->انگ_Functions; ++i) {
        نقطو = ڳوليو_محفوظ_نقطو(&i_module->functions[i], i_pc, i_قسم);
        إذا (نقطو) {
            *io_function = &i_module->functions[i];
        }
    }

    أرجع نقطو;
}

// The activation that starts at frame i_first, walking inward: the loops, try
// regions and entry frame standing in the function, up to the call or resume it
// waits on - or, for the innermost, up to the continuation's own suspension
// point. i_function is the function the call into it named.
ساكن
M3_نتيجو ايندڙ_Activation (IM3Module i_module, IM3Continuation i_cont, IM3Function i_function, m3stack_t i_sp,
                         i32 i_پهريون, M3Activation* o_act)
{
    M3_نتيجو نتيجو = m3Err_none;
    i32      i      = i_پهريون;

    memset(o_act, 0, حجم(*o_act));

    o_act->function   = i_function;
    o_act->sp         = i_sp;
    o_act->پهريون_چوکٺو = i_پهريون;

    _throwif("a suspended call does not say what it called", not i_function);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

    لكل (; i >= 0; --i) {
        ثابت M3_چوکٺو* چوکٺو = &i_cont->چوکٺا[i];

        _throwif("a suspended frame runs against another module's memory", چوکٺو->يادگيري != Module_Memory0(i_module));
        _throwif("a suspended frame is not where its function's frame is", چوکٺو->sp != i_sp);

        إذا (چوکٺو->قسم == چوکٺو_call) {
            o_act->pc           = چوکٺو->pc;
            o_act->قسم         = safepoint_call;
            o_act->registers.r0 = (m3reg_t*)&چوکٺو->call.r0;
#  إذا d_m3HasFloat
            o_act->registers.fp0 = (f64*)&چوکٺو->call.fp0;
#  endif
            اكسر;
        } وإلا إذا (چوکٺو->قسم == چوکٺو_resume) {
            _throwif("a suspended resume is not the innermost frame", i != 0);

            o_act->pc     = چوکٺو->pc;
            o_act->قسم   = safepoint_resume;
            o_act->resume = چوکٺو;
            اكسر;
        }
    }

    إذا (i < 0) {
        _throwif("a suspended frame is not where its function's frame is", i_cont->sp != i_sp);

        o_act->pc           = i_cont->pc;
        o_act->قسم         = i_cont->suspend_نقطو;
        o_act->registers.r0 = &i_cont->r0;
#  إذا d_m3HasFloat
        o_act->registers.fp0 = &i_cont->fp0;
#  endif
    }

    o_act->آخري_چوکٺو = i;
    o_act->نقطو     = ڳوليو_چوکٺو_محفوظ_نقطو(i_module, &o_act->function, o_act->pc, o_act->قسم);

    _throwif("a suspended frame is not at a safepoint", not o_act->نقطو);
    _throwif("a function in the snapshot was compiled before the runtime was made suspendable",
             not o_act->function->snapshotMap);

#  إذا d_m3_داخلا_Keeps_چوکٺو
    لكل (i32 k = i_پهريون; k > o_act->آخري_چوکٺو; --k) {
        ثابت M3_چوکٺو* چوکٺو = &i_cont->چوکٺا[k];
        _throwif("a suspended frame is not the function it is entered with",
                 چوکٺو->قسم == چوکٺو_داخلا and چوکٺو->داخلا.function != o_act->function);
    }
#  endif

_catch:
    أرجع نتيجو;
}

// A function's frame: which function, the safepoint it waits at, and every
// value it holds there
ساكن
M3_نتيجو SaveActivation (M3SnapshotSave* s, m3slot_t* i_بنياد, u32 i_انگ_Slots, ثابت M3Activation* i_act)
{
    M3_نتيجو             نتيجو        = m3Err_none;
    IM3Function          function      = i_act->function;
    ثابت M3SnapshotMap* map           = function->snapshotMap;
    u32                  function_اشاريو = Function_اشاريو(s->module, function);
    ثابت M3_محفوظ_نقطو*   نقطو         = i_act->نقطو;
    u32                  انگ_قدرون;

    _throwif("a suspended frame belongs to another module", function_اشاريو == d_m3SnapshotNone);

_   (PutLEB_u32(s, function_اشاريو));
_   (PutU8(s, نقطو->قسم));
_   (PutLEB_u32(s, نقطو->wasm_وٿي));

    إذا (نقطو->قسم == safepoint_op) {
_       (PutLEB_u32(s, map->بلاڪون[نقطو->aux].wasm_وٿي));
    }

    انگ_قدرون = map->انگ_Locals + نقطو->انگ_قدرون;

_   (PutLEB_u32(s, انگ_قدرون));

    لكل (u32 v = 0; v < انگ_قدرون; ++v) {
        ثابت M3_جڳهه_قدر* قدر = چوکٺو_قدر(map, نقطو, v);
        u8*                where;
        u64                لفظ;
        فراغ*              reference;

_       (Locate_قدر(i_بنياد, i_انگ_Slots, i_act->sp, قدر, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (بنياد_Type_جو(قدر->type) == c_m3Type_v128) {
_           (Put_قدر_ٽيگ(s, قدر->type));
_           (Put(s, where, 16));
            استمر;
        }

_       (Read_قدر(where, &i_act->registers, قدر, &لفظ, &reference));

_       (Put_قدر_بٽ(s, قدر->type, IsRefType(قدر->type) ? (u64)(uintptr_t)reference : لفظ));
    }

    إذا (نقطو->قسم == safepoint_resume) {
_       (Put_Ref_قدر(s, c_m3Type_contref, i_act->resume->resume.cont));
    }

_catch:
    أرجع نتيجو;
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
M3_نتيجو SaveSuspendedBody (M3SnapshotSave* s, IM3Continuation i_cont)
{
    M3_نتيجو      نتيجو   = m3Err_none;
    M3Activation* acts     = عدم;
    u32           انگ_Acts  = 0;
    IM3Function   function = i_cont->داخلا_Function;
    i32           پهريون    = (i32)i_cont->انگ_چوکٺا - 1;
    m3slot_t*     بنياد;
    u32           انگ_Slots;
    m3stack_t     sp;

_   (Continuation_ڍير(s->runtime, i_cont, &بنياد, &انگ_Slots));

    // one activation for each call frame at most, and the innermost
    acts = m3_AllocArray(M3Activation, i_cont->انگ_چوکٺا + 1);
    _throwifnull(acts);

    sp = بنياد;

    طالما (true) {
        M3Activation* act = &acts[انگ_Acts];

_       (ايندڙ_Activation(s->module, i_cont, function, sp, پهريون, act));

        إذا (act->قسم != safepoint_call) {
            انگ_Acts++;
            اكسر;
        }

        _throwif("a suspended continuation stops at a call", act->آخري_چوکٺو < 0);

        إذا (not (act->نقطو->flags & d_m3_محفوظ_نقطو_پڇ_Call)) {
            انگ_Acts++;
        }

        function = i_cont->چوکٺا[act->آخري_چوکٺو].call.function;
        sp       = act->sp + act->نقطو->aux;
        پهريون    = act->آخري_چوکٺو - 1;
    }

_   (PutLEB_u32(s, انگ_Acts));

    لكل (u32 a = 0; a < انگ_Acts; ++a) {
_       (SaveActivation(s, بنياد, انگ_Slots, &acts[a]));
    }

_catch:
    m3_Free(acts);
    أرجع نتيجو;
}

ساكن
M3_نتيجو SaveContinuation (M3SnapshotSave* s, u32 i_id, IM3Continuation i_cont)
{
    M3_نتيجو  نتيجو    = m3Err_none;
    IM3Module module    = s->module;
    bool      is_پاڙ    = (i_cont == s->runtime->پاڙ_Continuation);
    u8        state     = snapshot_cont_ختم;
    u32       type_اشاريو = is_پاڙ ? d_m3SnapshotNone : Func_Type_اشاريو(module, i_cont->type);

    إذا (i_cont->state == cont_allocated) {
        state = snapshot_contAllocated;
    } وإلا إذا (i_cont->state == cont_suspended) {
        state = snapshot_contSuspended;
    } وإلا إذا (i_cont->state == cont_running) {
        _throwif("a continuation is running", not s->postmortem);
    }

    _throwif("a continuation was made with no function", not i_cont->داخلا_Function);
    _throwif("a continuation belongs to another module", Function_اشاريو(module, i_cont->داخلا_Function) == d_m3SnapshotNone);

    _throwif("a continuation has a type of another module", not is_پاڙ and type_اشاريو == d_m3SnapshotNone);

_   (PutLEB_u32(s, i_id));
_   (PutU8(s, state));
_   (PutU8(s, is_پاڙ));
_   (PutLEB_u32(s, type_اشاريو));
_   (PutLEB_u32(s, Function_اشاريو(module, i_cont->داخلا_Function)));
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
        IM3FuncType inner   = i_cont->داخلا_Function ? i_cont->داخلا_Function->funcType : عدم;
        u16         انگ_Rets = inner ? Get_Func_Type_انگ_Results(inner) : 0;

        _throwif(m3Err_wasmMalformed, not inner or i_cont->bound_دليلون_ڳڻپ > Get_Func_Type_انگ_Params(inner));

_       (PutLEB_u32(s, i_cont->bound_دليلون_ڳڻپ));

        لكل (u32 i = 0; i < i_cont->bound_دليلون_ڳڻپ; ++i) {
            m3type_t type = GetFuncTypeParamType(inner, (u16)i);

_           (Put_قدر(s, type, i_cont->val_ڍير + (انگ_Rets + i) * c_io_جڳهه_ڳڻپ));
        }
    } وإلا إذا (state == snapshot_contSuspended) {
_       (PutLEB_u32(s, i_cont->bound_دليلون_ڳڻپ));
_       (SaveSuspendedBody(s, i_cont));
    } وإلا {
_       (PutLEB_u32(s, 0));
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو SaveContinuations (M3SnapshotSave* s)
{
    M3_نتيجو نتيجو = m3Err_none;

_   (PutLEB_u32(s, s->continuations.ڳڻپ));

    لكل (u32 i = 0; i < s->continuations.ڳڻپ; ++i) {
_       (SaveContinuation(s, i, (IM3Continuation)s->continuations.شيون[i]));
    }

_catch:
    أرجع نتيجو;
}

// The embedder's own state: the whole section, whose length is where it ends.
// Nothing saved is no section at all. Only the writing pass asks for it: it
// holds no references to follow.
ساكن
M3_نتيجو SaveHostState (M3SnapshotSave* s)
{
    M3_نتيجو               نتيجو = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &s->runtime->snapshotHooks;
    M3_بفر_Writer         bw     = { عدم, 0, 0 };

    إذا (s->writing and hooks->saveHostState and not s->postmortem) {
_       (hooks->saveHostState(hooks->userdata, بفر_Writer_Write, &bw));
    }

    إذا (bw.سائيز > 0) {
_       (Put(s, bw.بفر, bw.سائيز));
    }

_catch:
    m3_Free(bw.بفر);

    أرجع نتيجو;
}

ساكن
فراغ SnapshotResourceTotals (IM3Module module, u64* يادگيري_بائيٽون, u64* جدول_Elements)
{
    *يادگيري_بائيٽون = *جدول_Elements = 0;
    لكل (u32 i = 0; i < module->انگ_Memories; ++i) {
        IM3_يادگيري يادگيري = module->memories[i];
        إذا (يادگيري_پهريون_اشاريو(module, i) == i and يادگيري) {
            *يادگيري_بائيٽون += يادگيري->انگ_صفحا * (u64)يادگيري_صفحو_سائيز(يادگيري);
        }
    }
    لكل (u32 i = 0; i < module->انگ_Tables; ++i) {
        IM3_جدول جدول_2 = module->tables[i];
        إذا (جدول_پهريون_اشاريو(module, i) == i and جدول_2) {
            *جدول_Elements += جدول_2->سائيز;
        }
    }
}

ساكن
M3_نتيجو SaveMeta (M3SnapshotSave* s)
{
    M3_نتيجو نتيجو = m3Err_none;
    u64      hash   = 0;
    u64      يادگيري_بائيٽون, جدول_Elements;
    u32      سرگرم_Stacks = 0;
    لكل (u32 i = 0; i < s->continuations.ڳڻپ; ++i) {
        IM3Continuation cont = (IM3Continuation)s->continuations.شيون[i];
        إذا (cont != s->runtime->پاڙ_Continuation and
            (cont->state == cont_allocated or cont->state == cont_suspended)) {
            سرگرم_Stacks++;
        }
    }
    SnapshotResourceTotals(s->module, &يادگيري_بائيٽون, &جدول_Elements);

_   (ModuleFingerprint(s->module, &hash));

_   (PutLEB_u32(s, s->postmortem ? d_m3SnapshotFlagPostmortem : 0));
_   (PutLEB_u64(s, m3_Host_وقت_Ms()));
_   (PutLEB_u64(s, hash));
_   (PutLEB_u32(s, s->continuations.ڳڻپ));
_   (PutLEB_u32(s, s->exceptions.ڳڻپ));
_   (PutLEB_u64(s, يادگيري_بائيٽون));
_   (PutLEB_u64(s, جدول_Elements));
_   (PutLEB_u32(s, سرگرم_Stacks));

#  إذا d_m3HasExceptionHandling
    // each exception's tag before any of their payloads, so a reader can make
    // all of them before resolving a reference to one
    لكل (u32 i = 0; i < s->exceptions.ڳڻپ; ++i) {
        M3Exception* exception = (M3Exception*)s->exceptions.شيون[i];
        u32          ٽيگ_اشاريو  = ٽيگ_اشاريو_2(s->module, exception->ٽيگ);

        _throwif("an exception was thrown with another module's tag", ٽيگ_اشاريو == d_m3SnapshotNone);

_       (PutLEB_u32(s, ٽيگ_اشاريو));
    }
#  endif

_catch:
    أرجع نتيجو;
}

// The discovery pass. Reaching a continuation or an exception can find more of
// either, so it goes round until neither list grows; the writing pass then
// finds nothing new and can lay the sections out in order.
ساكن
M3_نتيجو DiscoverReferences (M3SnapshotSave* s)
{
    M3_نتيجو نتيجو            = m3Err_none;
    u32      پورو_Continuations = 0;
    u32      پورو_Exceptions    = 0;

_   (SaveMemories(s));
_   (SaveGlobals(s));
_   (SaveTables(s));
_   (SaveSegments(s));

    طالما (پورو_Continuations < s->continuations.ڳڻپ or پورو_Exceptions < s->exceptions.ڳڻپ) {
#  إذا d_m3HasExceptionHandling
        طالما (پورو_Exceptions < s->exceptions.ڳڻپ) {
            M3Exception* exception = (M3Exception*)s->exceptions.شيون[پورو_Exceptions++];
            IM3FuncType  type      = exception->ٽيگ->type;

            _throwif(m3Err_wasmMalformed, exception->انگ_دليلون != Get_Func_Type_انگ_Params(type));

            لكل (u32 a = 0; a < exception->انگ_دليلون; ++a) {
_               (Put_قدر_بٽ(s, GetFuncTypeParamType(type, (u16)a), exception->دليلون[a]));
            }
        }
#  endif
        طالما (پورو_Continuations < s->continuations.ڳڻپ and پورو_Exceptions == s->exceptions.ڳڻپ) {
            u32 id = پورو_Continuations++;
_           (SaveContinuation(s, id, (IM3Continuation)s->continuations.شيون[id]));
        }
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو SaveSections (M3SnapshotSave* s)
{
    M3_نتيجو نتيجو = m3Err_none;

    إذا (not s->writing) {
        أرجع DiscoverReferences(s);
    }

    إذا (not s->embedded) {
_       (Put(s, c_snapshotMagic, حجم(c_snapshotMagic)));
_       (PutLE32(s, c_snapshot_نسخو));
    }

    // a section with nothing to say is left out: a module with no memory has
    // no Memory section, rather than one that lists none
_   (SaveSection(s, d_m3SnapshotSection_Meta, SaveMeta));

    إذا (s->module->انگ_Memories) {
_       (SaveSection(s, d_m3_Snapshot_Section_يادگيري, SaveMemories));
    }
    إذا (s->module->انگ_Tables) {
_       (SaveSection(s, d_m3_Snapshot_Section_جدول, SaveTables));
    }
    إذا (s->module->انگ_Globals) {
_       (SaveSection(s, d_m3SnapshotSection_Global, SaveGlobals));
    }
    إذا (s->module->انگ_معلومات_Segments or s->module->انگ_Element_Segments) {
_       (SaveSection(s, d_m3SnapshotSection_Segment, SaveSegments));
    }

#  إذا d_m3HasExceptionHandling
    إذا (s->exceptions.ڳڻپ) {
_       (SaveSection(s, d_m3SnapshotSection_Exception, SaveExceptions));
    }
#  endif

    إذا (s->continuations.ڳڻپ) {
_       (SaveSection(s, d_m3SnapshotSection_Continuation, SaveContinuations));
    }

    // left out by SaveSection when the embedder has nothing to add
_   (SaveSection(s, d_m3SnapshotSection_HostState, SaveHostState));

_catch:
    أرجع نتيجو;
}

//---------------------------------------------------------------------------------------------------------------------------------
//  loading
//---------------------------------------------------------------------------------------------------------------------------------

تعريفنوع بنية M3SnapshotSuspendSite {
    ثابت M3SnapshotMap* map;
    ثابت M3_محفوظ_نقطو*   نقطو;
    u32                  هدف;             // child at a resume site, or zero
    bool                 resumed;
    m3type_t             expectedType; // concrete heap type required by references to this record
} M3SnapshotSuspendSite;

تعريفنوع بنية M3_Snapshot_ڀريو {
    IM3Runtime       runtime;
    IM3Module        module;
    M3SnapshotReader reader;
    فراغ*            userdata;

    bool      started;              // past the header, and so changing the runtime
    m3slot_t* check_ڍير;
    bool      بدليو;              // past Meta: a section has been applied to the module

    IM3Continuation*       continuations;
    u32                    انگ_Continuations;
    u64                    required_يادگيري_بائيٽون;
    u64                    required_جدول_Elements;
    u64                    loaded_يادگيري_بائيٽون;
    u64                    loaded_جدول_Elements;
    u32                    requiredStacks;
    u32                    loadedStacks;
    M3SnapshotSuspendSite* suspendSites;

#  إذا d_m3HasExceptionHandling
    M3Exception** exceptions;
#  endif
    u32 انگ_Exceptions;

    M3_چوکٺو* چوکٺا;           // a suspended continuation's frames as they are put together, outermost first

    بائيٽون_t pos;               // bounded section payload, read using the Wasm parser's helpers
    بائيٽون_t پڇاڙي;
    bool    اندريون_Section;
    bool    checking;
    bool    allowUnfilledRef;  // a suspend's not-yet-bound result slots can contain null
    bool    ڏٺل_Sections[256];
} M3_Snapshot_ڀريو;

ساكن
M3_نتيجو Get (M3_Snapshot_ڀريو* l, فراغ* o_معلومات, size_t i_سائيز)
{
    إذا (not l->اندريون_Section) {
        أرجع l->reader(o_معلومات, i_سائيز, l->userdata);
    }
    إذا (i_سائيز > (size_t)(l->پڇاڙي - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    إذا (i_سائيز) {
        يادگيري_نقل(o_معلومات, l->pos, i_سائيز);
        l->pos += i_سائيز;
    }
    أرجع m3Err_none;
}

ساكن
M3_نتيجو GetU8 (M3_Snapshot_ڀريو* l, u8* o_قدر)
{
    أرجع Get(l, o_قدر, حجم(*o_قدر));
}

ساكن
M3_نتيجو GetBool (M3_Snapshot_ڀريو* l, u8* o_قدر)
{
    M3_نتيجو نتيجو = GetU8(l, o_قدر);
    أرجع نتيجو ? نتيجو : (*o_قدر <= 1 ? m3Err_none : m3Err_wasmMalformed);
}

ساكن
M3_نتيجو GetLEB_u32 (M3_Snapshot_ڀريو* l, u32* o_قدر)
{
    إذا (l->اندريون_Section) {
        أرجع ReadLEB_u32(o_قدر, &l->pos, l->پڇاڙي);
    }

    // Only the section envelope is streamed. Collect its length and let the
    // same decoder used for Wasm modules check its width and terminal bits.
    u8 encoded[5];
    لكل (u32 i = 0; i < حجم(encoded); ++i) {
        M3_نتيجو نتيجو = GetU8(l, &encoded[i]);
        إذا (نتيجو) {
            أرجع نتيجو;
        }
        إذا (not (encoded[i] & 0x80)) {
            بائيٽون_t pos = encoded;
            أرجع ReadLEB_u32(o_قدر, &pos, encoded + i + 1);
        }
    }
    أرجع m3_Err_leb_حد_کان_مٿي;
}

ساكن
M3_نتيجو GetLEB_u64 (M3_Snapshot_ڀريو* l, u64* o_قدر)
{
    أرجع ReadLebUnsigned(o_قدر, 64, &l->pos, l->پڇاڙي);
}

ساكن
M3_نتيجو GetLEB_i64 (M3_Snapshot_ڀريو* l, i64* o_قدر)
{
    أرجع ReadLEB_i64(o_قدر, &l->pos, l->پڇاڙي);
}

ساكن
M3_نتيجو Skip_بائيٽون (M3_Snapshot_ڀريو* l, size_t i_سائيز)
{
    إذا (i_سائيز > (size_t)(l->پڇاڙي - l->pos)) {
        أرجع m3Err_wasmUnderrun;
    }
    l->pos += i_سائيز;
    أرجع m3Err_none;
}


ساكن
M3_نتيجو Get_Ref_قدر (M3_Snapshot_ڀريو* l, m3type_t i_type, فراغ** o_reference)
{
    M3_نتيجو نتيجو = m3Err_none;
    u8       قسم   = 0;
    u64      id     = 0;

    *o_reference = عدم;

_   (GetU8(l, &قسم));
    إذا (قسم == 0x00) {
        أرجع (IsNullableRef(i_type) or l->allowUnfilledRef) ? m3Err_none : "null in a non-nullable snapshot reference";
    }

_   (GetLEB_u64(l, &id));

    إذا (قسم == 0x01) {
        _throwif(m3Err_wasmMalformed, بنياد_Type_جو(i_type) != c_m3Type_funcref);
        _throwif(m3Err_wasmMalformed, id >= l->module->انگ_Functions);
#  إذا d_m3HasTypedRefs
        _throwif("a snapshot function reference has the wrong type", not Is_Sub_Type_جو(Ref_Type_جو_Func_Type(l->module->functions[id].funcType, true), i_type));
#  endif
        *o_reference = &l->module->functions[id];
    } وإلا إذا (قسم == 0x02) {
        _throwif(m3Err_wasmMalformed, بنياد_Type_جو(i_type) != c_m3Type_externref);
        ثابت M3SnapshotHooks* hooks = &l->runtime->snapshotHooks;
        _throwif("the snapshot holds an externref, and nothing here can bind one", not hooks->bindExternRef);
        إذا (not l->checking) {
_           (hooks->bindExternRef(hooks->userdata, id, o_reference));
            _throwif(m3Err_wasmMalformed, not *o_reference and not IsNullableRef(i_type));
        }
    }
#  إذا d_m3HasExceptionHandling
    وإلا إذا (قسم == 0x03) {
        _throwif(m3Err_wasmMalformed, بنياد_Type_جو(i_type) != c_m3Type_exnref);
        _throwif(m3Err_wasmMalformed, id >= l->انگ_Exceptions);
        *o_reference = l->exceptions[id];
    }
#  endif
    وإلا إذا (قسم == 0x04) {
        _throwif(m3Err_wasmMalformed, بنياد_Type_جو(i_type) != c_m3Type_contref);
        _throwif(m3Err_wasmMalformed, id == 0 or id >= l->انگ_Continuations);
        *o_reference = l->continuations[id];
#  إذا d_m3HasTypedRefs
        // The record may appear later, even in another section. Check its
        // concrete heap type once all continuation records have been read.
        إذا (not l->checking and ڍير_Type_جو(i_type) != d_m3_Type_ڍير_Abstract) {
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
    أرجع نتيجو;
}

ساكن
M3_نتيجو GetLE32 (M3_Snapshot_ڀريو* l, u32* o_قدر)
{
    u8       بائيٽون[4];
    M3_نتيجو نتيجو = Get(l, بائيٽون, حجم(بائيٽون));

    *o_قدر = 0;

    لكل (u32 i = 0; i < حجم(بائيٽون); ++i) {
        *o_قدر |= (u32)بائيٽون[i] << (8 * i);
    }

    أرجع نتيجو;
}

ساكن
M3_نتيجو GetLE64 (M3_Snapshot_ڀريو* l, u64* o_قدر)
{
    u8       بائيٽون[8];
    M3_نتيجو نتيجو = Get(l, بائيٽون, حجم(بائيٽون));

    *o_قدر = 0;

    لكل (u32 i = 0; i < حجم(بائيٽون); ++i) {
        *o_قدر |= (u64)بائيٽون[i] << (8 * i);
    }

    أرجع نتيجو;
}

// Reads the tag of a value and says what it holds, refusing one the file's
// type does not agree with. c_m3Type_none expects anything.
ساكن
M3_نتيجو Get_قدر_ٽيگ (M3_Snapshot_ڀريو* l, m3type_t i_expectedType, m3type_t* o_type)
{
    M3_نتيجو نتيجو  = m3Err_none;
    u8       valtype = 0;

_   (GetU8(l, &valtype));

    *o_type = Val_Type_منزل_M3_Type(valtype);

    _throwif(m3Err_wasmMalformed, *o_type == c_m3Type_none);
    _throwif(m3Err_wasmMalformed,
             i_expectedType != c_m3Type_none and بنياد_Type_جو(*o_type) != بنياد_Type_جو(i_expectedType));
    إذا (i_expectedType != c_m3Type_none) {
        *o_type = i_expectedType;
    }

_catch:
    أرجع نتيجو;
}

// The mirror of PutValueBody: narrow types come back in the low bits
ساكن
M3_نتيجو Get_قدر_Body (M3_Snapshot_ڀريو* l, m3type_t i_type, u64* o_بٽ)
{
    M3_نتيجو نتيجو = m3Err_none;

    *o_بٽ = 0;

    إذا (IsRefType(i_type)) {
        فراغ* reference = عدم;
_       (Get_Ref_قدر(l, i_type, &reference));
        *o_بٽ = (u64)(uintptr_t)reference;
    } وإلا إذا (بنياد_Type_جو(i_type) == c_m3Type_i32) {
        i32 قدر = 0;
_       (ReadLEB_i32(&قدر, &l->pos, l->پڇاڙي));
        *o_بٽ = (u32)قدر;
    } وإلا إذا (بنياد_Type_جو(i_type) == c_m3Type_i64) {
        i64 قدر = 0;
_       (GetLEB_i64(l, &قدر));
        *o_بٽ = (u64)قدر;
    } وإلا إذا (بنياد_Type_جو(i_type) == c_m3Type_f32) {
        u32 بٽ = 0;
_       (GetLE32(l, &بٽ));
        *o_بٽ = بٽ;
    } وإلا إذا (بنياد_Type_جو(i_type) == c_m3Type_f64) {
_       (GetLE64(l, o_بٽ));
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو Get_قدر_بٽ (M3_Snapshot_ڀريو* l, m3type_t i_expectedType, u64* o_بٽ)
{
    M3_نتيجو نتيجو = m3Err_none;
    m3type_t type;

_   (Get_قدر_ٽيگ(l, i_expectedType, &type));
_   (Get_قدر_Body(l, type, o_بٽ));

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو Get_قدر (M3_Snapshot_ڀريو* l, m3type_t i_expectedType, فراغ* o_where)
{
    M3_نتيجو نتيجو = m3Err_none;
    m3type_t type;
    u8       scratch[16];
    إذا (l->checking) {
        o_where = scratch;
    }

_   (Get_قدر_ٽيگ(l, i_expectedType, &type));

    إذا (IsRefType(type)) {
        فراغ* reference = عدم;
        u64   بٽ      = 0;
_       (Get_قدر_Body(l, type, &بٽ));
        reference = (فراغ*)(uintptr_t)بٽ;
        يادگيري_نقل(o_where, &reference, حجم(reference));
    } وإلا إذا (بنياد_Type_جو(type) == c_m3Type_v128) {
_       (Get(l, o_where, 16));
    } وإلا {
        u64 بٽ = 0;
_       (Get_قدر_Body(l, type, &بٽ));

        إذا (Is64_ٻٽو_انگ_Type(type)) {
            يادگيري_نقل(o_where, &بٽ, حجم(بٽ));
        } وإلا {
            u32 narrow = (u32)بٽ;
            يادگيري_نقل(o_where, &narrow, حجم(narrow));
        }
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو GetFunction (M3_Snapshot_ڀريو* l, IM3Function* o_function)
{
    M3_نتيجو نتيجو = m3Err_none;
    u32      index  = 0;

_   (GetLEB_u32(l, &index));

    _throwif(m3Err_wasmMalformed, index >= l->module->انگ_Functions);
    *o_function = &l->module->functions[index];

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_يادگيري_Chunks (M3_Snapshot_ڀريو* l, u8* o_بائيٽون, size_t i_سائيز)
{
    M3_نتيجو نتيجو = m3Err_none;

    لكل (;;) {
        u8  chunkType = 0;
        u32 offset = 0, ڊيگهائي = 0;

_       (GetU8(l, &chunkType));

        إذا (chunkType == d_m3_Chunk_پڇاڙي) {
            اكسر;
        }

_       (GetLEB_u32(l, &offset));
_       (GetLEB_u32(l, &ڊيگهائي));

        _throwif(m3Err_wasmMalformed, (size_t)offset > i_سائيز or ڊيگهائي > i_سائيز - (size_t)offset);

        إذا (chunkType == d_m3ChunkRaw) {
            إذا (l->checking or ڊيگهائي == 0) {
_               (Skip_بائيٽون(l, ڊيگهائي));
            } وإلا {
_               (Get(l, o_بائيٽون + offset, ڊيگهائي));
            }
        } وإلا إذا (chunkType == d_m3ChunkFillFF) {
            إذا (not l->checking and ڊيگهائي) {
                memset(o_بائيٽون + offset, 0xFF, ڊيگهائي);
            }
        } وإلا {
            _throw(m3Err_wasmMalformed);
        }
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Memories (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو  نتيجو      = m3Err_none;
    IM3Module module      = l->module;
    u32       انگ_Memories = 0;
    l->loaded_يادگيري_بائيٽون  = 0;

_   (GetLEB_u32(l, &انگ_Memories));
    // a section is present only when it has something to say
    _throwif(m3Err_wasmMalformed, انگ_Memories == 0 or انگ_Memories != module->انگ_Memories);

    لكل (u32 m = 0; m < انگ_Memories; ++m) {
        u32       mem_اشاريو = 0, پهريون_اشاريو = 0, صفحو_بٽ = 0, انگ_صفحا = 0;
        IM3_يادگيري يادگيري;

_       (GetLEB_u32(l, &mem_اشاريو));
_       (GetLEB_u32(l, &پهريون_اشاريو));

        // every memory is listed, in index order, so the index is a check
        _throwif(m3Err_wasmMalformed, mem_اشاريو != m);

        // which imports share a memory is settled by linking, not by the
        // module, so the instance restored into has to share the same ones
        _throwif("the snapshot shares memories between imports differently",
                 پهريون_اشاريو != يادگيري_پهريون_اشاريو(module, m));
        إذا (پهريون_اشاريو != m) {
            استمر;
        }

_       (GetLEB_u32(l, &صفحو_بٽ));
_       (GetLEB_u32(l, &انگ_صفحا));

        يادگيري = module->memories[mem_اشاريو];
        _throwif("a shared memory cannot be restored from a snapshot", يادگيري and يادگيري->isShared);

        size_t بائيٽون = 0;
        u8*    معلومات  = عدم;

        إذا (يادگيري) {
            _throwif(m3Err_wasmMalformed, صفحو_بٽ >= 32 or (1u << صفحو_بٽ) != يادگيري_صفحو_سائيز(يادگيري));
            _throwif(m3Err_wasmMalformed, انگ_صفحا < يادگيري->انگ_صفحا or انگ_صفحا > يادگيري->max_صفحا);
            _throwif(m3Err_wasmMalformed, انگ_صفحا > SIZE_MAX / يادگيري_صفحو_سائيز(يادگيري));
            بائيٽون = (size_t)انگ_صفحا * يادگيري_صفحو_سائيز(يادگيري);

            _throwif("shrunken memory cannot be restored from a snapshot",
                     l->runtime->يادگيري_حد_بندي and بائيٽون > (size_t)l->runtime->يادگيري_حد_بندي);

            إذا (not l->checking) {
                إذا (يادگيري->انگ_صفحا < انگ_صفحا) {
_                   (Resize_يادگيري(l->runtime, يادگيري, انگ_صفحا));
                }
                _throwif("shrunken memory cannot be restored from a snapshot",
                         يادگيري->mallocated and يادگيري->mallocated->ڊيگهائي < بائيٽون);
            }

            l->loaded_يادگيري_بائيٽون += بائيٽون;

            إذا (بائيٽون and يادگيري->mallocated) {
                معلومات = m3_Mem_معلومات(يادگيري->mallocated);
            }
        } وإلا {
            _throwif(m3Err_wasmMalformed, انگ_صفحا != 0);
        }

        // every memory carries a chunk stream, an unbacked one an empty stream,
        // so a memory this runtime does not hold still has to be read past
        إذا (معلومات and not l->checking) {
            memset(معلومات, 0, بائيٽون);
        }

_       (ڀريو_يادگيري_Chunks(l, معلومات, بائيٽون));
    }
    _throwif(m3Err_wasmMalformed, l->loaded_يادگيري_بائيٽون != l->required_يادگيري_بائيٽون);

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Globals (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو  نتيجو     = m3Err_none;
    IM3Module module     = l->module;
    u32       انگ_Globals = 0;

_   (GetLEB_u32(l, &انگ_Globals));
    _throwif(m3Err_wasmMalformed, انگ_Globals == 0 or انگ_Globals != module->انگ_Globals);

    لكل (u32 g = 0; g < انگ_Globals; ++g) {
        u32       global_اشاريو = 0;
        M3Global* global;
        M3Global* cell;

_       (GetLEB_u32(l, &global_اشاريو));
        _throwif(m3Err_wasmMalformed, global_اشاريو != g);

        global = &module->globals[global_اشاريو];
        cell   = global->resolved ? global->resolved : global;

_       (Get_قدر(l, global->type, &cell->i64_قدر));
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Tables (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو  نتيجو       = m3Err_none;
    IM3Module module       = l->module;
    u32       انگ_Tables    = 0;
    l->loaded_جدول_Elements = 0;

_   (GetLEB_u32(l, &انگ_Tables));
    _throwif(m3Err_wasmMalformed, انگ_Tables == 0 or انگ_Tables != module->انگ_Tables);

    لكل (u32 t = 0; t < انگ_Tables; ++t) {
        u32      جدول_اشاريو = 0, پهريون_اشاريو = 0, سائيز = 0;
        u8       valtype = 0;
        IM3_جدول جدول_2;

_       (GetLEB_u32(l, &جدول_اشاريو));
_       (GetLEB_u32(l, &پهريون_اشاريو));

        _throwif(m3Err_wasmMalformed, جدول_اشاريو != t);
        _throwif("the snapshot shares tables between imports differently",
                 پهريون_اشاريو != جدول_پهريون_اشاريو(module, t));
        إذا (پهريون_اشاريو != t) {
            استمر;
        }

_       (GetU8(l, &valtype));
_       (GetLEB_u32(l, &سائيز));

        جدول_2 = module->tables[جدول_اشاريو];

        _throwif(m3Err_wasmMalformed, valtype != M3_Type_منزل_Val_Type(جدول_2->type));
        _throwif(m3Err_wasmMalformed, سائيز > (جدول_2->hasMax ? جدول_2->max_سائيز : d_m3_Max_Sane_جدول_سائيز));

        // the table can only have grown since it was instantiated
        _throwif(m3Err_wasmMalformed, سائيز < جدول_2->سائيز);

        l->loaded_جدول_Elements += سائيز;

        إذا (not l->checking and سائيز != جدول_2->سائيز) {
            فراغ** elements;

            _throwif(m3_Err_جدول_حد_بندي_Exceeded, l->runtime->جدول_Elements_حد_بندي and
                                                 سائيز - جدول_2->سائيز > l->runtime->جدول_Elements_حد_بندي - l->runtime->جدول_Elements_استعمال_ٿيل);
            elements = m3_ReallocArray(فراغ*, جدول_2->elements, سائيز, جدول_2->سائيز);
            _throwifnull(elements);

            l->runtime->جدول_Elements_استعمال_ٿيل += سائيز - جدول_2->سائيز;
            جدول_2->elements = elements;
            جدول_2->سائيز     = سائيز;
        }

        لكل (u32 e = 0; e < سائيز; ++e) {
            فراغ* reference = عدم;
_           (Get_Ref_قدر(l, جدول_2->type, &reference));
            إذا (not l->checking) {
                جدول_2->elements[e] = reference;
            }
        }
    }
    _throwif(m3Err_wasmMalformed, l->loaded_جدول_Elements != l->required_جدول_Elements);

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Segments (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو  نتيجو = m3Err_none;
    IM3Module module = l->module;
    u32       ڳڻپ  = 0;

_   (GetLEB_u32(l, &ڳڻپ));
    _throwif(m3Err_wasmMalformed, ڳڻپ != module->انگ_معلومات_Segments);
    _throwif(m3Err_wasmMalformed, module->انگ_معلومات_Segments == 0 and module->انگ_Element_Segments == 0);

    لكل (u32 i = 0; i < ڳڻپ; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->معلومات_Segments[i].dropped = dropped;
        }
    }

_   (GetLEB_u32(l, &ڳڻپ));
    _throwif(m3Err_wasmMalformed, ڳڻپ != module->انگ_Element_Segments);

    لكل (u32 i = 0; i < ڳڻپ; ++i) {
        u8 dropped = 0;
_       (GetBool(l, &dropped));
        إذا (not l->checking) {
            module->elementSegments[i].dropped = dropped;
        }
    }

_catch:
    أرجع نتيجو;
}

// A frame for the continuation being put together, outermost first
ساكن
M3_نتيجو جوڙيو_چوکٺو (M3_Snapshot_ڀريو* l, u32* io_انگ_چوکٺا, M3_چوکٺو_قسم i_قسم, m3stack_t i_sp, M3_چوکٺو** o_چوکٺو)
{
    إذا (*io_انگ_چوکٺا >= d_m3_Continuation_Max_چوکٺا) {
        أرجع "the snapshot's frames do not fit this build's";
    }

    M3_چوکٺو* چوکٺو = &l->چوکٺا[(*io_انگ_چوکٺا)++];

    memset(چوکٺو, 0, حجم(*چوکٺو));
    چوکٺو->قسم   = (u8)i_قسم;
    چوکٺو->sp     = i_sp;
    چوکٺو->يادگيري = Module_Memory0(l->module);

    *o_چوکٺو = چوکٺو;

    أرجع m3Err_none;
}

// Compiled, and with a snapshot map: where a function's frame in a snapshot is
// found in this build's code
ساكن
M3_نتيجو PrepareFunction (IM3Function i_function)
{
    M3_نتيجو نتيجو = m3Err_none;

    إذا (not i_function->compiled) {
_       (CompileFunction(i_function));
    }

    _throwif("a function in the snapshot was compiled before the runtime was made suspendable", not i_function->snapshotMap);

_catch:
    أرجع نتيجو;
}

// The native frames the interpreter keeps for the loops and try_tables around
// a safepoint, outermost first. They are fixed by where the frame stands, so
// the snapshot does not list them: they come from this build's own map.
ساكن
M3_نتيجو جوڙيو_بلاڪ_چوکٺا (M3_Snapshot_ڀريو* l, ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطو* i_نقطو, m3stack_t i_sp,
                         u32* io_انگ_چوکٺا)
{
    M3_نتيجو نتيجو = m3Err_none;

    لكل (u32 b = 0; b < i_map->انگ_بلاڪون; ++b) {
        ثابت M3_بلاڪ_شروعات* بلاڪ = &i_map->بلاڪون[b];
        M3_چوکٺو*            چوکٺو;

        إذا (not Encloses_وٿي(بلاڪ, i_نقطو->wasm_وٿي)) {
            استمر;
        }

        إذا (بلاڪ->opcode == c_waOp_loop) {
_           (جوڙيو_چوکٺو(l, io_انگ_چوکٺا, چوکٺو_loop, i_sp, &چوکٺو));
            چوکٺو->pc = بلاڪ->pc;
        }
#  إذا d_m3HasExceptionHandling
        وإلا {
_           (جوڙيو_چوکٺو(l, io_انگ_چوکٺا, چوکٺو_try, i_sp, &چوکٺو));
            چوکٺو->pc                = بلاڪ->pc;
            چوکٺو->try_.انگ_Clauses   = بلاڪ->انگ_Clauses;
            چوکٺو->try_.handlers_جيئرو = Handlers_جيئرو_At(i_map, i_نقطو, بلاڪ);
        }
#  endif
    }

_catch:
    أرجع نتيجو;
}

// Puts a function's values where this build keeps them at the safepoint
ساكن
M3_نتيجو ڀريو_قدرون (M3_Snapshot_ڀريو* l, m3slot_t* i_بنياد, u32 i_انگ_Slots, m3stack_t i_sp,
                     ثابت M3SnapshotMap* i_map, ثابت M3_محفوظ_نقطو* i_نقطو, ثابت M3_چوکٺو_Registers* i_registers)
{
    M3_نتيجو نتيجو    = m3Err_none;
    u32      انگ_قدرون = 0;

_   (GetLEB_u32(l, &انگ_قدرون));
    _throwif("the snapshot's frame does not match what this build compiled", انگ_قدرون != i_map->انگ_Locals + i_نقطو->انگ_قدرون);

    لكل (u32 v = 0; v < انگ_قدرون; ++v) {
        ثابت M3_جڳهه_قدر* قدر     = چوکٺو_قدر(i_map, i_نقطو, v);
        u64                لفظ      = 0;
        فراغ*              reference = عدم;
        u8*                where;

_       (Locate_قدر(i_بنياد, i_انگ_Slots, i_sp, قدر, &where));

        // a v128 has no byte order of its own, and no bits path can carry it
        إذا (بنياد_Type_جو(قدر->type) == c_m3Type_v128) {
            m3type_t type;
_           (Get_قدر_ٽيگ(l, قدر->type, &type));
            إذا (l->checking) {
_               (Skip_بائيٽون(l, 16));
            } وإلا {
_               (Get(l, where, 16));
            }
            استمر;
        }

        l->allowUnfilledRef = i_نقطو->قسم == safepoint_suspend and
                              v >= انگ_قدرون - i_نقطو->انگ_Results;
_       (Get_قدر_بٽ(l, قدر->type, &لفظ));
        l->allowUnfilledRef = false;

        إذا (IsRefType(قدر->type)) {
            reference = (فراغ*)(uintptr_t)لفظ;
        }

        إذا (not l->checking) {
_           (Write_قدر(where, i_registers, قدر, لفظ, reference));
        }
    }

_catch:
    أرجع نتيجو;
}

// The functions a suspended continuation is in the middle of, outermost first.
// Each one's frame is found in this build's code, its values put where this
// build keeps them, and the native frames the interpreter needs to stand it
// back up are made from what its own maps say about them.
ساكن
M3_نتيجو ڀريو_Suspended_Body (M3_Snapshot_ڀريو* l, IM3Continuation io_cont, u32 i_id)
{
    M3_نتيجو    نتيجو    = m3Err_none;
    u32         انگ_چوکٺا = 0;
    M3_چوکٺو*    call_چوکٺو = عدم;
    IM3Function function  = عدم;
    m3slot_t*   بنياد;
    u32         انگ_Slots;
    u32         انگ_Activations = 0;
    IM3FuncType expected       = io_cont->داخلا_Function->funcType;
    m3stack_t   sp;

_   (Continuation_ڍير(l->runtime, io_cont, &بنياد, &انگ_Slots));

_   (GetLEB_u32(l, &انگ_Activations));
    _throwif(m3Err_wasmMalformed, انگ_Activations == 0 or انگ_Activations > d_m3_Continuation_Max_چوکٺا);

    io_cont->pc                = عدم;
    io_cont->r0                = 0;
    io_cont->انگ_Suspend_Results = 0;
    io_cont->suspend_نقطو      = safepoint_op;
#  إذا d_m3HasFloat
    io_cont->fp0 = 0.;
#  endif

    sp = بنياد;

    لكل (u32 a = 0; a < انگ_Activations; ++a) {
        bool                 isInnermost = (a + 1 == انگ_Activations);
        ثابت M3_محفوظ_نقطو*   نقطو;
        M3_چوکٺو_Registers     registers;
        u8                   قسم       = 0;
        u32                  wasm_وٿي = 0, هدف_Loop = 0;
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
                 (u32)(sp - بنياد) + function->max_ڍير_Slots > انگ_Slots);

        // the call the frame outside this one is waiting on
        إذا (call_چوکٺو) {
            call_چوکٺو->call.function = function;
        }

#  إذا d_m3_داخلا_Keeps_چوکٺو
        {
            M3_چوکٺو* داخلا;
_           (جوڙيو_چوکٺو(l, &انگ_چوکٺا, چوکٺو_داخلا, sp, &داخلا));
            داخلا->pc             = function->compiled;
            داخلا->داخلا.function = function;
        }
#  endif

_       (GetU8(l, &قسم));
_       (GetLEB_u32(l, &wasm_وٿي));

        // A back edge says which loop it goes to. That has to be a loop around
        // the instruction; one the instruction does not branch to has no
        // safepoint to match.
        إذا (قسم == safepoint_op) {
            ثابت M3_بلاڪ_شروعات* loop;

_           (GetLEB_u32(l, &هدف_Loop));

            loop = ڳوليو_بلاڪ_At(map, c_waOp_loop, هدف_Loop);
            _throwif("a back edge in the snapshot does not go to a loop of its function", not loop);
            _throwif("a back edge in the snapshot goes to a loop that is not around it",
                     not Encloses_وٿي(loop, wasm_وٿي));
        }

        نقطو = ڳوليو_محفوظ_نقطو_At(map, wasm_وٿي, قسم, هدف_Loop);
        _throwif("a frame in the snapshot is not at a safepoint of this build's", not نقطو);

_       (جوڙيو_بلاڪ_چوکٺا(l, map, نقطو, sp, &انگ_چوکٺا));

        إذا (not isInnermost) {
            _throwif(m3Err_wasmMalformed, قسم != safepoint_call);

_           (جوڙيو_چوکٺو(l, &انگ_چوکٺا, چوکٺو_call, sp, &call_چوکٺو));
            call_چوکٺو->pc = نقطو->pc;
            registers.r0  = &call_چوکٺو->call.r0;
#  إذا d_m3HasFloat
            registers.fp0 = &call_چوکٺو->call.fp0;
#  endif
        } وإلا إذا (قسم == safepoint_resume) {
            M3_چوکٺو* چوکٺو;
            u32      انگ_Results = نقطو->انگ_Results;
            pc_t     resultsPC  = نقطو->pc - 2 * انگ_Results;

_           (جوڙيو_چوکٺو(l, &انگ_چوکٺا, چوکٺو_resume, sp, &چوکٺو));
            چوکٺو->pc                 = نقطو->pc;
            چوکٺو->resume.resultsPC   = resultsPC;
            چوکٺو->resume.انگ_Results  = انگ_Results;
            چوکٺو->resume.انگ_Handlers = نقطو->aux;
            چوکٺو->resume.handlersPC  = resultsPC - 1 - 3 * نقطو->aux;

            // a replayed resume carries on with clear registers, so the frame
            // keeps nothing in them
            call_چوکٺو = چوکٺو;
        } وإلا {
            _throwif(m3Err_wasmMalformed, قسم != safepoint_op and قسم != safepoint_داخلا and قسم != safepoint_suspend);

            io_cont->pc           = نقطو->pc;
            io_cont->suspend_نقطو = قسم;
            registers.r0          = &io_cont->r0;
#  إذا d_m3HasFloat
            registers.fp0 = &io_cont->fp0;
#  endif
        }

_       (ڀريو_قدرون(l, بنياد, انگ_Slots, sp, map, نقطو, &registers));

        إذا (not l->checking) {
            RestoreConstants(function, sp);
        }

        إذا (not isInnermost) {
            sp = sp + نقطو->aux;
_           (CallSiteType(function, نقطو, &expected));
        } وإلا إذا (قسم == safepoint_resume) {
            فراغ* cont = عدم;
_           (Get_Ref_قدر(l, c_m3Type_contref, &cont));
            _throwif(m3Err_wasmMalformed, cont == عدم);

            call_چوکٺو->resume.cont = (IM3Continuation)cont;
            إذا (not l->checking) {
                l->suspendSites[i_id].نقطو = نقطو;
                لكل (u32 i = 1; i < l->انگ_Continuations; ++i) {
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
            إذا (نقطو->flags & d_m3_محفوظ_نقطو_ورتل_Branch) {
                io_cont->r0 = 1;
            }

            // a suspend waits for its results in the slots at the top of what
            // it holds, which is where whatever resumes it writes them
            إذا (قسم == safepoint_suspend) {
                _throwif(m3Err_wasmMalformed, نقطو->انگ_Results > نقطو->انگ_قدرون or
                                                نقطو->انگ_Results > d_m3MaxContinuationPayload);

                io_cont->انگ_Suspend_Results = نقطو->انگ_Results;
                إذا (not l->checking) {
                    l->suspendSites[i_id].map   = map;
                    l->suspendSites[i_id].نقطو = نقطو;
                }

                لكل (u32 i = 0; i < نقطو->انگ_Results; ++i) {
                    ثابت M3_جڳهه_قدر* قدر = &map->قدرون[نقطو->پهريون_قدر + نقطو->انگ_قدرون - نقطو->انگ_Results + i];

                    _throwif(m3Err_wasmMalformed, Is_رجسٽر_جڳهه_Alias(قدر->جڳهه));

                    io_cont->suspend_نتيجو_Offsets[i] = قدر->جڳهه;
                    io_cont->suspend_نتيجو_Is64[i]    = Is64_ٻٽو_انگ_Type(قدر->type);
                }
            }
        }
    }

    io_cont->sp = sp;
    إذا (l->checking) {
        أرجع m3Err_none;
    }

    // held innermost first
    إذا (انگ_چوکٺا > io_cont->چوکٺا_Cap) {
        M3_چوکٺو* چوکٺا = m3_ReallocArray(M3_چوکٺو, io_cont->چوکٺا, انگ_چوکٺا, io_cont->چوکٺا_Cap);
        _throwifnull(چوکٺا);

        io_cont->چوکٺا    = چوکٺا;
        io_cont->چوکٺا_Cap = انگ_چوکٺا;
    }

    لكل (u32 k = 0; k < انگ_چوکٺا; ++k) {
        io_cont->چوکٺا[انگ_چوکٺا - 1 - k] = l->چوکٺا[k];
    }

    io_cont->انگ_چوکٺا = انگ_چوکٺا;

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Continuation (M3_Snapshot_ڀريو* l, u32 i_id)
{
    M3_نتيجو        نتيجو = m3Err_none;
    IM3Module       module = l->module;
    IM3Continuation cont   = l->continuations[i_id];
    M3Continuation  scratch;
    إذا (l->checking) {
        scratch          = *cont;
        scratch.val_ڍير = l->check_ڍير;
        إذا (i_id == 0) {
            scratch.val_ڍير = (m3slot_t*)l->runtime->origin_ڍير;
        }
        cont = &scratch;
    }

    u32 contId = 0, type_اشاريو = 0, داخلا_اشاريو = 0, bound_دليلون_ڳڻپ = 0;
    u8  state = 0, is_پاڙ = 0;

_   (GetLEB_u32(l, &contId));
_   (GetU8(l, &state));
_   (GetBool(l, &is_پاڙ));
_   (GetLEB_u32(l, &type_اشاريو));
_   (GetLEB_u32(l, &داخلا_اشاريو));

    _throwif(m3Err_wasmMalformed, contId != i_id);
    _throwif(m3Err_wasmMalformed, (bool)is_پاڙ != (i_id == 0));
    _throwif(m3Err_wasmMalformed, is_پاڙ and state != snapshot_contSuspended);
    إذا (not is_پاڙ and (state == snapshot_contAllocated or state == snapshot_contSuspended)) {
        l->loadedStacks++;
    }
    _throwif(m3Err_wasmMalformed, type_اشاريو != d_m3SnapshotNone and type_اشاريو >= module->انگ_Func_Types);
    // every continuation, the root included, was made with a function
    _throwif(m3Err_wasmMalformed, داخلا_اشاريو >= module->انگ_Functions);
    _throwif(m3Err_wasmMalformed, is_پاڙ ? type_اشاريو != d_m3SnapshotNone : (type_اشاريو == d_m3SnapshotNone or not module->funcTypes[type_اشاريو]->isContinuation));

    إذا (not l->checking and not is_پاڙ and state != snapshot_cont_ختم) {
_       (Continuation_Acquire_ڍير(l->runtime, cont));
    }
    cont->type          = (type_اشاريو != d_m3SnapshotNone) ? module->funcTypes[type_اشاريو] : عدم;
    cont->داخلا_Function = &module->functions[داخلا_اشاريو];
    cont->انگ_Handlers   = 0;
    cont->handlersPC    = عدم;
    cont->والد        = عدم;
    cont->انگ_چوکٺا     = 0;

#  إذا d_m3HasExceptionHandling
    {
        فراغ* ref = عدم;
_       (Get_Ref_قدر(l, c_m3Type_exnref, &ref));
        cont->resumeThrow = (M3Exception*)ref;
    }
    // the root was not resumed by anything, and a finished continuation has
    // nowhere left to raise an exception
    _throwif(m3Err_wasmMalformed, (is_پاڙ or state == snapshot_cont_ختم) and cont->resumeThrow != عدم);
#  وإلا
    {
        فراغ* ref = عدم;
_       (Get_Ref_قدر(l, c_m3Type_exnref, &ref));
        _throwif(m3Err_wasmMalformed, ref != عدم);
    }
#  endif

    إذا (state == snapshot_contAllocated) {
        IM3FuncType inner;
        u16         انگ_Rets;

_       (GetLEB_u32(l, &bound_دليلون_ڳڻپ));
        cont->bound_دليلون_ڳڻپ = bound_دليلون_ڳڻپ;
        inner                = cont->داخلا_Function->funcType;

        _throwif(m3Err_wasmMalformed, bound_دليلون_ڳڻپ > Get_Func_Type_انگ_Params(inner));

        {
            IM3FuncType بچيل = cont->type->contFuncType;
            _throwif(m3Err_wasmMalformed, not بچيل or
                                            Get_Func_Type_انگ_Params(بچيل) != Get_Func_Type_انگ_Params(inner) - bound_دليلون_ڳڻپ or
                                            Get_Func_Type_انگ_Results(بچيل) != Get_Func_Type_انگ_Results(inner));
            لكل (u16 i = 0; i < Get_Func_Type_انگ_Results(inner); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتيجو_Type(inner, i) != Get_Func_Type_نتيجو_Type(بچيل, i));
            }
            لكل (u16 i = 0; i < Get_Func_Type_انگ_Params(بچيل); ++i) {
                _throwif(m3Err_wasmMalformed, GetFuncTypeParamType(inner, (u16)(bound_دليلون_ڳڻپ + i)) != GetFuncTypeParamType(بچيل, i));
            }
        }

        انگ_Rets = Get_Func_Type_انگ_Results(inner);

        _throwif(m3Err_wasmMalformed, (انگ_Rets + bound_دليلون_ڳڻپ) * c_io_جڳهه_ڳڻپ > cont->انگ_ڍير_Slots);

        لكل (u32 i = 0; i < bound_دليلون_ڳڻپ; ++i) {
_           (Get_قدر(l, GetFuncTypeParamType(inner, (u16)i), cont->val_ڍير + (انگ_Rets + i) * c_io_جڳهه_ڳڻپ));
        }

        cont->state = cont_allocated;
        cont->sp    = cont->val_ڍير;
        cont->pc    = cont->داخلا_Function->compiled;
    } وإلا إذا (state == snapshot_contSuspended) {
_       (GetLEB_u32(l, &bound_دليلون_ڳڻپ));
        _throwif(m3Err_wasmMalformed, is_پاڙ and bound_دليلون_ڳڻپ != 0);
        cont->bound_دليلون_ڳڻپ = bound_دليلون_ڳڻپ;

_       (ڀريو_Suspended_Body(l, cont, i_id));

        cont->state = cont_suspended;
    } وإلا إذا (state == snapshot_cont_ختم) {
_       (GetLEB_u32(l, &bound_دليلون_ڳڻپ));
        _throwif(m3Err_wasmMalformed, bound_دليلون_ڳڻپ != 0);
        cont->bound_دليلون_ڳڻپ = 0;
        cont->state          = cont_consumed;
    } وإلا {
        _throw(m3Err_wasmMalformed);
    }

_catch:
    إذا (نتيجو) {
        cont->انگ_چوکٺا = 0;
    }

    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Continuations (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو نتيجو = m3Err_none;
    u32      ڳڻپ  = 0;
    l->loadedStacks = 0;

_   (GetLEB_u32(l, &ڳڻپ));
    _throwif(m3Err_wasmMalformed, ڳڻپ == 0 or ڳڻپ != l->انگ_Continuations);

    لكل (u32 i = 0; i < ڳڻپ; ++i) {
_       (ڀريو_Continuation(l, i));
    }
    _throwif(m3Err_wasmMalformed, l->loadedStacks != l->requiredStacks);

_catch:
    أرجع نتيجو;
}

// Cross-record constraints cannot be checked while reading a reference: both
// its target and a nested suspension point may occur later in the stream.
ساكن
M3_نتيجو CheckContinuationReferences (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو نتيجو = m3Err_none;

    لكل (u32 id = 0; id < l->انگ_Continuations; ++id) {
        IM3Continuation cont  = l->continuations[id];
        u32             پن  = id;
        u32             اونهائي = 0;
#  إذا d_m3HasTypedRefs
        m3type_t expectedType = l->suspendSites[id].expectedType;
        _throwif("a snapshot continuation reference has the wrong type", expectedType and
                                                                           (not cont->type or not Is_Sub_Type_جو(Ref_Type_جو_Func_Type(cont->type, true), expectedType)));
#  endif
        إذا (l->suspendSites[id].هدف) {
            // The child may have switched to a different parameter signature
            // after this resume started. Its results must still fit this site.
            IM3FuncType     expected = Module_Cont_Type_جو_Ref(l->module, l->suspendSites[id].نقطو->resumeType);
            IM3Continuation ٻار    = l->continuations[l->suspendSites[id].هدف];
            IM3FuncType     actual   = ٻار->داخلا_Function ? ٻار->داخلا_Function->funcType : عدم;
            _throwif(m3Err_wasmMalformed, not expected or not actual);
            expected = expected->contFuncType;
            _throwif(m3Err_wasmMalformed, Get_Func_Type_انگ_Results(actual) != Get_Func_Type_انگ_Results(expected));
            لكل (u16 i = 0; i < Get_Func_Type_انگ_Results(expected); ++i) {
                _throwif(m3Err_wasmMalformed, Get_Func_Type_نتيجو_Type(actual, i) != Get_Func_Type_نتيجو_Type(expected, i));
            }
        }
        طالما (l->suspendSites[پن].هدف) {
            پن = l->suspendSites[پن].هدف;
            _throwif(m3Err_wasmMalformed, ++اونهائي >= l->انگ_Continuations or
                                            l->continuations[پن]->state != cont_suspended);
        }
        إذا (cont->state != cont_suspended or id == 0) {
            استمر;
        }

        // A continuation currently executing under a saved resume already
        // received its arguments. Only an independently captured suspension
        // (possibly containing nested resumes) still expects the tag's results.
        إذا (l->suspendSites[id].resumed) {
            _throwif(m3Err_wasmMalformed, cont->bound_دليلون_ڳڻپ != 0);
            استمر;
        }

        ثابت M3SnapshotSuspendSite* site      = &l->suspendSites[پن];
        IM3Continuation              inner     = l->continuations[پن];
        IM3FuncType                  بچيل = cont->type->contFuncType;
        _throwif("a captured continuation has no suspension point", not site->نقطو or not بچيل);
        _throwif("a captured continuation has the wrong binding count", cont->bound_دليلون_ڳڻپ > inner->انگ_Suspend_Results or
                                                                          Get_Func_Type_انگ_Params(بچيل) != inner->انگ_Suspend_Results - cont->bound_دليلون_ڳڻپ);
        _throwif(m3Err_wasmMalformed,
                 Get_Func_Type_انگ_Results(بچيل) != Get_Func_Type_انگ_Results(cont->داخلا_Function->funcType));
        لكل (u16 i = 0; i < Get_Func_Type_انگ_Results(بچيل); ++i) {
            _throwif(m3Err_wasmMalformed, Get_Func_Type_نتيجو_Type(بچيل, i) !=
                                            Get_Func_Type_نتيجو_Type(cont->داخلا_Function->funcType, i));
        }

        لكل (u32 i = 0; i < inner->انگ_Suspend_Results; ++i) {
            ثابت M3_جڳهه_قدر* قدر = &site->map->قدرون[site->نقطو->پهريون_قدر +
                                                          site->نقطو->انگ_قدرون - site->نقطو->انگ_Results + i];
            إذا (i < cont->bound_دليلون_ڳڻپ) {
                إذا (IsRefType(قدر->type) and not IsNullableRef(قدر->type)) {
                    فراغ* ref = عدم;
                    يادگيري_نقل(&ref, inner->sp + قدر->جڳهه, حجم(ref));
                    _throwif(m3Err_wasmMalformed, not ref);
                }
            } وإلا {
                _throwif("a captured continuation has the wrong parameter type", قدر->type !=
                                                                                   GetFuncTypeParamType(بچيل, (u16)(i - cont->bound_دليلون_ڳڻپ)));
            }
        }
    }

_catch:
    أرجع نتيجو;
}

#  إذا d_m3HasExceptionHandling

ساكن
M3_نتيجو ڀريو_Exceptions (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو نتيجو        = m3Err_none;
    u32      انگ_Exceptions = 0;

_   (GetLEB_u32(l, &انگ_Exceptions));
    _throwif(m3Err_wasmMalformed, انگ_Exceptions == 0 or انگ_Exceptions != l->انگ_Exceptions);

    لكل (u32 i = 0; i < انگ_Exceptions; ++i) {
        u32          exnId = 0, انگ_دليلون = 0;
        M3Exception* exn;
        IM3FuncType  type;

_       (GetLEB_u32(l, &exnId));
_       (GetLEB_u32(l, &انگ_دليلون));

        _throwif(m3Err_wasmMalformed, exnId != i);

        exn  = l->exceptions[i];
        type = exn->ٽيگ->type;

        _throwif(m3Err_wasmMalformed, انگ_دليلون != exn->انگ_دليلون);

        لكل (u32 a = 0; a < انگ_دليلون; ++a) {
            m3type_t argType = GetFuncTypeParamType(type, (u16)a);
            u64      بٽ    = 0;
_           (Get_قدر_بٽ(l, argType, &بٽ));
            إذا (not l->checking) {
                exn->دليلون[a] = بٽ;
            }
        }
    }

_catch:
    أرجع نتيجو;
}

#  endif

// Reads the embedder's state through a reader that stops where it ends
تعريفنوع بنية M3BoundedReader {
    M3_Snapshot_ڀريو* ڀريو;
    u64             بچيل;
    M3_نتيجو        error;
} M3BoundedReader;

ساكن
M3_نتيجو BoundedReader_Read (فراغ* o_بفر, size_t i_سائيز, فراغ* i_userdata)
{
    M3BoundedReader* br = (M3BoundedReader*)i_userdata;

    إذا (i_سائيز > br->بچيل) {
        br->error = "the host read past the end of its state";
        أرجع br->error;
    }

    br->بچيل -= i_سائيز;

    أرجع Get(br->ڀريو, o_بفر, i_سائيز);
}

ساكن
M3_نتيجو ڀريو_Host_State (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو               نتيجو = m3Err_none;
    ثابت M3SnapshotHooks* hooks  = &l->runtime->snapshotHooks;
    M3BoundedReader        reader;
    size_t                 سائيز = (size_t)(l->پڇاڙي - l->pos);     // the whole section, which is never empty

    _throwif("the snapshot carries host state, and nothing here restores it", not hooks->ڀريو_Host_State_2);

    إذا (l->checking) {
        أرجع Skip_بائيٽون(l, سائيز);
    }
    reader.ڀريو      = l;
    reader.بچيل = سائيز;
    reader.error     = m3Err_none;

_   (hooks->ڀريو_Host_State_2(hooks->userdata, BoundedReader_Read, &reader, سائيز));
_   (reader.error);
    _throwif("the host did not read all of its state", reader.بچيل != 0);

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_سرو (M3_Snapshot_ڀريو* l)
{
    M3_نتيجو نتيجو = m3Err_none;
    u8       magic[حجم(c_snapshotMagic)];
    u32      نسخو = 0;

_   (Get(l, magic, حجم(magic)));
    _throwif(m3Err_wasmMalformed, memcmp(magic, c_snapshotMagic, حجم(magic)) != 0);

_   (GetLE32(l, &نسخو));
    _throwif(m3Err_wasmMalformed, نسخو != c_snapshot_نسخو);

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Meta (M3_Snapshot_ڀريو* l, u32* o_flags)
{
    M3_نتيجو نتيجو      = m3Err_none;
    u64      timestamp   = 0;
    u64      moduleHash  = 0;
    u64      ownHash     = 0;
    u64      يادگيري_بائيٽون = 0, جدول_Elements = 0;
    u32      سرگرم_Stacks = 0;
    u64      current_يادگيري, currentTables;
    SnapshotResourceTotals(l->module, &current_يادگيري, &currentTables);

_   (GetLEB_u32(l, o_flags));
_   (GetLEB_u64(l, &timestamp));
_   (GetLEB_u64(l, &moduleHash));

    (فراغ)timestamp;

_   (ModuleFingerprint(l->module, &ownHash));
    _throwif("the snapshot was saved from a different module", moduleHash != ownHash);

    // a flag this build does not know changes what the rest of the file means
    _throwif("the snapshot sets a flag this build does not know", *o_flags & ~(u32)d_m3SnapshotFlagPostmortem);

_   (GetLEB_u32(l, &l->انگ_Continuations));
_   (GetLEB_u32(l, &l->انگ_Exceptions));
_   (GetLEB_u64(l, &يادگيري_بائيٽون));
_   (GetLEB_u64(l, &جدول_Elements));
_   (GetLEB_u32(l, &سرگرم_Stacks));
    l->requiredStacks        = سرگرم_Stacks;
    l->required_يادگيري_بائيٽون   = يادگيري_بائيٽون;
    l->required_جدول_Elements = جدول_Elements;
    _throwif(m3Err_wasmMalformed, يادگيري_بائيٽون < current_يادگيري or جدول_Elements < currentTables);
    _throwif(m3Err_wasmMalformed, not l->module->انگ_Memories and يادگيري_بائيٽون != 0);
    _throwif(m3Err_wasmMalformed, not l->module->انگ_Tables and جدول_Elements != 0);

    // the rest of Meta means the same thing in a postmortem, so it is read
    // whole and the refusal to resume comes after the section, not inside it
    إذا (*o_flags & d_m3SnapshotFlagPostmortem) {
        لكل (u32 i = 0; i < l->انگ_Exceptions; ++i) {
            u32 ٽيگ_اشاريو = 0;
_           (GetLEB_u32(l, &ٽيگ_اشاريو));
        }
        أرجع m3Err_none;
    }

    _throwif(m3_Err_يادگيري_حد_بندي_Exceeded, l->runtime->يادگيري_بائيٽون_حد_بندي and
                                          يادگيري_بائيٽون - current_يادگيري > l->runtime->يادگيري_بائيٽون_حد_بندي - l->runtime->يادگيري_بائيٽون_استعمال_ٿيل);
    _throwif(m3_Err_جدول_حد_بندي_Exceeded, l->runtime->جدول_Elements_حد_بندي and
                                         جدول_Elements - currentTables > l->runtime->جدول_Elements_حد_بندي - l->runtime->جدول_Elements_استعمال_ٿيل);
    _throwif(m3_Err_continuation_حد_بندي_Exceeded, l->runtime->continuations_حد_بندي and
                                                سرگرم_Stacks > l->runtime->continuations_حد_بندي - l->runtime->continuationsAllocated);

    _throwif(m3Err_wasmMalformed, l->انگ_Continuations == 0 or سرگرم_Stacks >= l->انگ_Continuations);
#  إذا !d_m3HasExceptionHandling
    _throwif("the snapshot holds exceptions, and this build has none", l->انگ_Exceptions != 0);
#  endif

    إذا (l->checking) {
        لكل (u32 i = 0; i < l->انگ_Exceptions; ++i) {
            u32 ٽيگ_اشاريو = 0;
_           (GetLEB_u32(l, &ٽيگ_اشاريو));
#  إذا d_m3HasExceptionHandling
            _throwif(m3Err_wasmMalformed, ٽيگ_اشاريو >= l->module->انگ_ٽيگ_فهرست);
#  endif
        }
        أرجع m3Err_none;
    }

    l->check_ڍير = m3_AllocArray(m3slot_t, d_m3_Continuation_ڍير_Slots + 4);
    _throwifnull(l->check_ڍير);
    l->چوکٺا = m3_AllocArray(M3_چوکٺو, d_m3_Continuation_Max_چوکٺا);
    _throwifnull(l->چوکٺا);

    l->continuations = m3_AllocArray(IM3Continuation, l->انگ_Continuations);
    _throwifnull(l->continuations);
    l->suspendSites = m3_AllocArray(M3SnapshotSuspendSite, l->انگ_Continuations);
    _throwifnull(l->suspendSites);

    إذا (not l->runtime->پاڙ_Continuation) {
        IM3Continuation پاڙ = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(پاڙ);

        // the root borrows the runtime's stack, so it holds none of its own
        پاڙ->انگ_ڍير_Slots          = l->runtime->انگ_ڍير_Slots;
        l->runtime->پاڙ_Continuation = پاڙ;
    }

    l->continuations[0] = l->runtime->پاڙ_Continuation;

    لكل (u32 i = 1; i < l->انگ_Continuations; ++i) {
        l->continuations[i] = Continuation_New(l->runtime, عدم, عدم);
        _throwifnull(l->continuations[i]);
    }

#  إذا d_m3HasExceptionHandling
    إذا (l->انگ_Exceptions) {
        l->exceptions = m3_AllocArray(M3Exception*, l->انگ_Exceptions);
        _throwifnull(l->exceptions);
    }

    لكل (u32 i = 0; i < l->انگ_Exceptions; ++i) {
        u32    ٽيگ_اشاريو = 0;
        IM3_ٽيگ ٽيگ;
        u32    انگ_دليلون;

_       (GetLEB_u32(l, &ٽيگ_اشاريو));
        _throwif(m3Err_wasmMalformed, ٽيگ_اشاريو >= l->module->انگ_ٽيگ_فهرست);

        ٽيگ     = ٽيگ_جو_اشاريو(l->module, ٽيگ_اشاريو);
        انگ_دليلون = Get_Func_Type_انگ_Params(ٽيگ->type);

        l->exceptions[i] = NewException(l->runtime, ٽيگ, انگ_دليلون);
        _throwifnull(l->exceptions[i]);

        l->exceptions[i]->reified = true;
    }
#  وإلا
    لكل (u32 i = 0; i < l->انگ_Exceptions; ++i) {
        u32 ٽيگ_اشاريو = 0;
_       (GetLEB_u32(l, &ٽيگ_اشاريو));
    }
#  endif

_catch:
    أرجع نتيجو;
}

// Each section is decoded once without changing guest state or calling the host,
// then applied with the same decoder. This checks its complete structure before
// any of its values or opaque host bytes are installed.
ساكن
M3_نتيجو ڀريو_Section (M3_Snapshot_ڀريو* l, u8 i_sectionId)
{
    M3_نتيجو نتيجو = m3Err_none;
    u32      flags  = 0;
    بدل (i_sectionId) {
    حالة d_m3SnapshotSection_Meta:
_       (ڀريو_Meta(l, &flags));
        _throwif("postmortem snapshots cannot be resumed", flags & d_m3SnapshotFlagPostmortem);
        l->started = true;
        اكسر;

    حالة d_m3_Snapshot_Section_يادگيري:
_       (ڀريو_Memories(l));
        اكسر;

    حالة d_m3_Snapshot_Section_جدول:
_       (ڀريو_Tables(l));
        اكسر;

    حالة d_m3SnapshotSection_Global:
_       (ڀريو_Globals(l));
        اكسر;

    حالة d_m3SnapshotSection_Segment:
_       (ڀريو_Segments(l));
        اكسر;

#  إذا d_m3HasExceptionHandling
    حالة d_m3SnapshotSection_Exception:
_       (ڀريو_Exceptions(l));
        اكسر;
#  endif

    حالة d_m3SnapshotSection_Continuation:
_       (ڀريو_Continuations(l));
        اكسر;

    حالة d_m3SnapshotSection_HostState:
_       (ڀريو_Host_State(l));
        اكسر;

    افتراضي:
_       (Skip_بائيٽون(l, (size_t)(l->پڇاڙي - l->pos)));
        اكسر;
    }

_catch:
    أرجع نتيجو;
}

ساكن
M3_نتيجو ڀريو_Sections (M3_Snapshot_ڀريو* l, bool i_embedded)
{
    M3_نتيجو نتيجو  = m3Err_none;
    u8*      payload = عدم;

    إذا (not i_embedded) {
_       (ڀريو_سرو(l));
    }

    طالما (true) {
        u8  sectionId   = 0;
        u32 section_سائيز = 0;

        // the last section ends the file, so the reader running out here is
        // the end of the snapshot rather than a truncated one
        إذا (GetU8(l, &sectionId)) {
            اكسر;
        }

_       (GetLEB_u32(l, &section_سائيز));

        _throwif("the snapshot holds a section twice", l->ڏٺل_Sections[sectionId]);
        l->ڏٺل_Sections[sectionId] = true;
        _throwif("the snapshot holds an empty section", section_سائيز == 0);

        // the Meta section says what the runtime has to be made ready for, so
        // nothing may come before it
        _throwif(m3Err_wasmMalformed, not l->started and sectionId != d_m3SnapshotSection_Meta);

        // Read the entire payload before letting a loader or host callback
        // touch the instance. Every body decoder is then bounded by this buffer.
        payload = (u8*)m3_Malloc("snapshot_section", section_سائيز);
        _throwifnull(payload);
_       (Get(l, payload, section_سائيز));
        l->pos       = payload;
        l->پڇاڙي       = payload + section_سائيز;
        l->اندريون_Section = true;

        {
            M3_Snapshot_ڀريو check = *l;
            check.checking       = true;
_           (ڀريو_Section(&check, sectionId));
            _throwif("a section in the snapshot does not match its declared length", check.pos != check.پڇاڙي);
        }
        l->بدليو = l->بدليو or sectionId != d_m3SnapshotSection_Meta;
_       (ڀريو_Section(l, sectionId));

        // a section that reads past its own length has desynced the stream,
        // and one that stops short would leave the next section misread
        _throwif("a section in the snapshot does not match its declared length",
                 l->pos != l->پڇاڙي);
        l->اندريون_Section = false;
        m3_Free(payload);
    }

    _throwif("the snapshot has no meta section", not l->started);
    _throwif("the snapshot has no continuation section",
             not l->ڏٺل_Sections[d_m3SnapshotSection_Continuation]);
    _throwif("the snapshot names exceptions it does not hold",
             l->انگ_Exceptions and not l->ڏٺل_Sections[d_m3SnapshotSection_Exception]);

    // a store section left out is not "unchanged": it would resume against
    // whatever instantiation happened to leave behind
    _throwif("the snapshot has no memory section",
             l->module->انگ_Memories and not l->ڏٺل_Sections[d_m3_Snapshot_Section_يادگيري]);
    _throwif("the snapshot has no table section",
             l->module->انگ_Tables and not l->ڏٺل_Sections[d_m3_Snapshot_Section_جدول]);
    _throwif("the snapshot has no global section",
             l->module->انگ_Globals and not l->ڏٺل_Sections[d_m3SnapshotSection_Global]);
    _throwif("the snapshot has no segment section",
             (l->module->انگ_معلومات_Segments or l->module->انگ_Element_Segments) and
               not l->ڏٺل_Sections[d_m3SnapshotSection_Segment]);

_   (CheckContinuationReferences(l));

_catch:
    m3_Free(payload);
    أرجع نتيجو;
}

#جيڪڏهن_پورو // d_m3HasSnapshots


//---------------------------------------------------------------------------------------------------------------------------------
//  API
//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3HasSnapshots

ساكن
M3_نتيجو SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata,
                       IM3Module* o_module, bool i_embedded)
{
    M3_نتيجو       نتيجو = m3Err_none;
    M3SnapshotSave save;

    memset(&save, 0, حجم(save));

    IM3Continuation پاڙ = io_runtime->پاڙ_Continuation;

    // Anything but a paused invocation is saved as a postmortem: the state of
    // the module the last call entered, which needs no continuation and so no
    // suspendable runtime
    save.runtime    = io_runtime;
    save.writer     = i_writer;
    save.userdata   = i_userdata;
    save.embedded   = i_embedded;
    save.postmortem = not پاڙ or پاڙ->state != cont_suspended;

    IM3Function داخلا = save.postmortem ? io_runtime->entered : پاڙ->داخلا_Function;

    إذا (o_module) {
        *o_module = عدم;
    }

    // no call has entered anything, so there is no module to describe
    _throwif("there is nothing to snapshot", not داخلا);

    save.module = داخلا->module;

    _throwif("a snapshot failed to restore into the module, so it cannot be saved", save.module->isUnusable);

    إذا (o_module) {
        *o_module = save.module;
    }

    لكل (IM3Continuation cont = io_runtime->continuations; cont; cont = cont->ايندڙ) {
        u32 id;
_       (اشارو_Ids_جوڙيو(&save.جيئرو_Continuations, cont, &id));
    }

#  إذا d_m3HasExceptionHandling
    لكل (M3Exception* exception = io_runtime->exceptions; exception; exception = exception->ايندڙ) {
        u32 id;
_       (اشارو_Ids_جوڙيو(&save.جيئرو_Exceptions, exception, &id));
    }
#  endif

    إذا (not save.postmortem) {
        u32 id;
_       (اشارو_Ids_جوڙيو(&save.continuations, پاڙ, &id));
    }

_   (SaveSections(&save));

    save.writing = true;

_   (SaveSections(&save));

_catch:
    اشارو_Ids_Free(&save.جيئرو_Continuations);
    اشارو_Ids_Free(&save.جيئرو_Exceptions);
    اشارو_Ids_Free(&save.continuations);
    اشارو_Ids_Free(&save.exceptions);

    أرجع نتيجو;
}

#جيڪڏهن_پورو

M3_نتيجو m3_SaveSnapshot (IM3Runtime io_runtime, M3SnapshotWriter i_writer, فراغ* i_userdata)
{
    إذا (not io_runtime or not i_writer) {
        أرجع m3Err_mallocFailed;
    }

#إذا d_m3HasSnapshots
    أرجع SaveSnapshot(io_runtime, i_writer, i_userdata, عدم, false);
#وإلا
    (فراغ)i_userdata;
    أرجع "snapshots are not available in this build of Wasm3";
#جيڪڏهن_پورو
}


ساكن
M3_نتيجو ڀريو_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata, bool i_embedded)
{
    M3_نتيجو نتيجو = m3Err_none;
    _throwifnull(io_runtime);
    _throwifnull(i_module);
    _throwifnull(i_reader);

#إذا d_m3HasSnapshots
    // A snapshot is the whole of the module's state, and is checked against
    // what instantiation made: memories and tables no smaller than they were
    // declared, and nothing the module did since to account for
    _throwif("a snapshot failed to restore into the module, so it cannot be used", i_module->isUnusable);
    _throwif("a snapshot restores only into a freshly instantiated module, and this one has run", i_module->has_لاڳيتو);

    {
        M3_Snapshot_ڀريو ڀريو;
        memset(&ڀريو, 0, حجم(ڀريو));

        ڀريو.runtime  = io_runtime;
        ڀريو.module   = i_module;
        ڀريو.reader   = i_reader;
        ڀريو.userdata = i_userdata;

        // Before anything here compiles a function: what comes out of a
        // snapshot has to be able to suspend again, and a body compiled while
        // this is still false has neither the back edges nor the maps for it
        io_runtime->isSuspendable = true;

        نتيجو = ڀريو_Sections(&ڀريو, i_embedded);

        IM3Continuation پاڙ = io_runtime->پاڙ_Continuation;

        إذا (نتيجو) {
            إذا (ڀريو.بدليو) {
                // Part of the snapshot is in the module and part is not: a
                // state the program was never in. It is not resumed, run or
                // saved again; the embedder has to throw it away.
                i_module->isUnusable = true;

                إذا (پاڙ) {
                    پاڙ->state     = cont_returned;
                    پاڙ->انگ_چوکٺا = 0;
                }
            }
        } وإلا {
            // A suspension is only ever reached with the start function run,
            // or under way in the invocation being restored, so the memories
            // and globals already hold what it did
            i_module->شروعات_Function = -1;
            i_module->has_لاڳيتو        = true;
        }

        Runtime_رکو_Call_ڍير(io_runtime);

        m3_Free(ڀريو.check_ڍير);
        m3_Free(ڀريو.چوکٺا);
        m3_Free(ڀريو.suspendSites);
        m3_Free(ڀريو.continuations);
#  إذا d_m3HasExceptionHandling
        m3_Free(ڀريو.exceptions);
#  endif
    }
#وإلا
    (فراغ)i_userdata;
    _throw("snapshots are not available in this build of Wasm3");
#جيڪڏهن_پورو

_catch:
    أرجع نتيجو;
}


M3_نتيجو m3_ڀريو_Snapshot (IM3Runtime io_runtime, IM3Module i_module, M3SnapshotReader i_reader, فراغ* i_userdata)
{
    أرجع ڀريو_Snapshot(io_runtime, i_module, i_reader, i_userdata, false);
}


فراغ m3_سيٽ_Snapshot_Hooks (IM3Runtime io_runtime, ثابت M3SnapshotHooks* i_hooks)
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
#جيڪڏهن_پورو
}


M3_نتيجو m3_Save_Snapshot_منزل_بفر (IM3Runtime io_runtime, فراغ** o_بائيٽون, size_t* o_سائيز)
{
    إذا (!io_runtime || !o_بائيٽون || !o_سائيز) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    M3_بفر_Writer bw     = { عدم, 0, 0 };
    M3_نتيجو       نتيجو = m3_SaveSnapshot(io_runtime, بفر_Writer_Write, &bw);
    إذا (نتيجو) {
        m3_Free(bw.بفر);
        *o_بائيٽون = عدم;
        *o_سائيز  = 0;
        أرجع نتيجو;
    }
    *o_بائيٽون = bw.بفر;
    *o_سائيز  = bw.سائيز;
    أرجع m3Err_none;
#وإلا
    *o_بائيٽون = عدم;
    *o_سائيز  = 0;
    أرجع "snapshots are not available in this build of Wasm3";
#جيڪڏهن_پورو
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
u32 سائيز_جو_LEB_u32 (u32 i_قدر)
{
    u32 سائيز = 0;

    افعل {
        i_قدر >>= 7;
        ++سائيز;
    } طالما (i_قدر != 0);

    أرجع سائيز;
}

// Walks the custom sections of a Wasm binary and finds the one named
// i_secName. o_secStart/o_secEnd frame the whole section, so it can be dropped
// when a new one replaces it; o_data/o_size frame its snapshot payload. Any of
// the four may be NULL. Not finding one is not an error: o_secStart and o_data
// are left NULL.
ساكن
M3_نتيجو ڳوليو_Embedded_Snapshot (ثابت u8* i_شروعات, ثابت u8* i_پڇاڙي, cstr_t i_secName,
                               ثابت u8** o_sec_شروعات, ثابت u8** o_sec_پڇاڙي,
                               ثابت u8** o_معلومات, u32* o_سائيز)
{
    M3_نتيجو نتيجو = m3Err_none;
    بائيٽون_t  pos    = i_شروعات;
    bool     مليو  = false;
    cstr_t   name   = عدم;

    إذا (o_sec_شروعات) {
        *o_sec_شروعات = عدم;
    }
    إذا (o_sec_پڇاڙي) {
        *o_sec_پڇاڙي = عدم;
    }
    إذا (o_معلومات) {
        *o_معلومات = عدم;
    }
    إذا (o_سائيز) {
        *o_سائيز = 0;
    }

    _throwif(m3Err_wasmMalformed, i_پڇاڙي - pos < 8 or memcmp(pos, "\0asm\1\0\0\0", 8) != 0);
    pos += 8;

    طالما (pos < i_پڇاڙي) {
        بائيٽون_t sec_شروعات  = pos;
        u8      sectionId = *pos++;
        u32     سائيز      = 0;
_       (ReadLEB_u32(&سائيز, &pos, i_پڇاڙي));
        _throwif(m3Err_wasmUnderrun, سائيز > (size_t)(i_پڇاڙي - pos));
        {
            بائيٽون_t payload = pos;
            pos += سائيز;
            إذا (sectionId == 0) {
_               (Read_utf8(&name, &payload, pos));
                إذا (strcmp(name, i_secName) == 0) {
                    _throwif("duplicate embedded snapshot name", مليو);
                    مليو = true;
                    إذا (o_sec_شروعات) {
                        *o_sec_شروعات = sec_شروعات;
                    }
                    إذا (o_sec_پڇاڙي) {
                        *o_sec_پڇاڙي = pos;
                    }
                    إذا (o_معلومات) {
                        *o_معلومات = payload;
                    }
                    إذا (o_سائيز) {
                        *o_سائيز = (u32)(pos - payload);
                    }
                }
                m3_Free(name);
            }
        }
    }
_catch:
    m3_Free(name);
    أرجع نتيجو;
}

// Builds the custom section name a snapshot of i_name lives under
ساكن
M3_نتيجو SnapshotSectionName (cstr_t i_name, حرف* o_secName, size_t i_سائيز)
{
    إذا (i_name and *i_name) {
        إذا (لڙي_ڊيگهه(i_name) > d_m3_Max_Snapshot_Name_ڊيگهائي) {
            أرجع "the snapshot name is too long";
        }
        snprintf(o_secName, i_سائيز, "snapshot.%s", i_name);
    } وإلا {
        snprintf(o_secName, i_سائيز, "snapshot");
    }

    أرجع m3Err_none;
}

// The snapshot of this name the module was parsed with, or NULL. Parsing keeps
// two of the same name, and selecting that name is where they are refused.
ساكن
M3_نتيجو ModuleSnapshot (IM3Module i_module, cstr_t i_name, M3EmbeddedSnapshot** o_snapshot)
{
    M3_نتيجو نتيجو = m3Err_none;
    cstr_t   name   = i_name ? i_name : "";

    *o_snapshot = عدم;

    لكل (M3EmbeddedSnapshot* snap = i_module->snapshots; snap; snap = snap->ايندڙ) {
        إذا (strcmp(snap->name, name) == 0) {
            _throwif("duplicate embedded snapshot name", *o_snapshot);
            *o_snapshot = snap;
        }
    }

_catch:
    أرجع نتيجو;
}
#جيڪڏهن_پورو

M3_نتيجو m3_ڀريو_Snapshot_From_بفر (IM3Runtime io_runtime, IM3Module i_module, ثابت فراغ* i_بائيٽون, size_t i_سائيز)
{
    إذا (!io_runtime || !i_module || !i_بائيٽون) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    // A Wasm binary carries its snapshot in a custom section. It is taken from
    // the buffer that was handed over, which need not be the one i_module was
    // parsed from - the snapshot still restores into i_module.
    إذا (i_سائيز >= 8 && memcmp(i_بائيٽون, "\0asm", 4) == 0) {
        ثابت u8* معلومات   = عدم;
        u32       سائيز   = 0;
        M3_نتيجو  نتيجو = ڳوليو_Embedded_Snapshot((ثابت u8*)i_بائيٽون, (ثابت u8*)i_بائيٽون + i_سائيز,
                                                "snapshot", عدم, عدم, &معلومات, &سائيز);
        إذا (نتيجو) {
            أرجع نتيجو;
        }
        إذا (not معلومات) {
            أرجع "the Wasm binary holds no embedded snapshot";
        }

        {
            M3_بفر_Reader br = { معلومات, سائيز, 0 };
            أرجع ڀريو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
        }
    }
    {
        M3_بفر_Reader br = { (ثابت u8*)i_بائيٽون, i_سائيز, 0 };
        أرجع ڀريو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br,
                            i_سائيز < 4 or memcmp(i_بائيٽون, c_snapshotMagic, 4) != 0);
    }
#وإلا
    (فراغ)i_سائيز;
    أرجع "snapshots are not available in this build of Wasm3";
#جيڪڏهن_پورو
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
#جيڪڏهن_پورو
}

M3_نتيجو m3_GetEmbeddedSnapshot (IM3Module i_module, ثابت حرف* i_name,
                                 ثابت فراغ** o_بائيٽون, size_t* o_سائيز)
{
    إذا (not i_module or not o_بائيٽون or not o_سائيز) {
        أرجع m3Err_mallocFailed;
    }

    *o_بائيٽون = عدم;
    *o_سائيز  = 0;

#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_نتيجو            نتيجو = ModuleSnapshot(i_module, i_name, &snap);

        إذا (نتيجو) {
            أرجع نتيجو;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        *o_بائيٽون = snap->معلومات;
        *o_سائيز  = snap->سائيز;

        أرجع m3Err_none;
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#جيڪڏهن_پورو
}

M3_نتيجو m3_ڀريو_Embedded_Snapshot (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name)
{
    إذا (not io_runtime or not i_module) {
        أرجع m3Err_mallocFailed;
    }
#إذا d_m3HasSnapshots
    {
        M3EmbeddedSnapshot* snap   = عدم;
        M3_نتيجو            نتيجو = ModuleSnapshot(i_module, i_name, &snap);
        M3_بفر_Reader      br;

        إذا (نتيجو) {
            أرجع نتيجو;
        }
        إذا (not snap) {
            أرجع "the module holds no embedded snapshot of that name";
        }

        br.بفر = snap->معلومات;
        br.سائيز   = snap->سائيز;
        br.cursor = 0;

        أرجع ڀريو_Snapshot(io_runtime, i_module, بفر_Reader_Read, &br, true);
    }
#وإلا
    (فراغ)i_name;
    أرجع "snapshots are not available in this build of Wasm3";
#جيڪڏهن_پورو
}


M3_نتيجو m3_Save_Snapshot_منزل_Module (IM3Runtime io_runtime, IM3Module i_module, ثابت حرف* i_name,
                                  فراغ** o_بائيٽون, size_t* o_سائيز)
{
#إذا d_m3HasSnapshots
    M3_نتيجو       نتيجو = m3Err_none;
    M3_بفر_Writer bw     = { عدم, 0, 0 };
    u8*            ٻاهريون    = عدم;

    حرف      secName[d_m3_Max_Snapshot_Name_ڊيگهائي + حجم("snapshot.")];
    u32       nameLen, name_Len_Leb_سائيز, payload_سائيز, payload_سائيز_Leb_سائيز;
    size_t    orig_سائيز, ڪل_سائيز;
    ثابت u8* drop_شروعات = عدم;
    ثابت u8* drop_پڇاڙي   = عدم;
    IM3Module saved     = عدم;
    u8*       p;

    إذا (not io_runtime or not i_module or not o_بائيٽون or not o_سائيز) {
        أرجع m3Err_mallocFailed;
    }

    *o_بائيٽون = عدم;
    *o_سائيز  = 0;

    _throwif("the module carries no Wasm bytecode to embed a snapshot in",
             not i_module->wasm_شروعات or i_module->wasm_پڇاڙي <= i_module->wasm_شروعات);

_   (SnapshotSectionName(i_name, secName, حجم(secName)));

    // An embedded snapshot is there to be resumed - the unnamed one is resumed
    // just by running the module - and a postmortem cannot be. With nothing
    // paused, a save is a postmortem.
    _throwif("a postmortem cannot be embedded in a module",
             not io_runtime->پاڙ_Continuation or io_runtime->پاڙ_Continuation->state != cont_suspended);

_   (SaveSnapshot(io_runtime, بفر_Writer_Write, &bw, &saved, true));

    _throwif("there is nothing to snapshot", not bw.سائيز);

    // the snapshot is of the module the paused call is in: embedding it in any
    // other leaves a binary whose fingerprint can never match
    _throwif("the snapshot belongs to another module", saved != i_module);

    _throwif("the snapshot is too large to embed in a Wasm module", bw.سائيز > 0xF0000000u);

    // a snapshot of this name already in the module is replaced, not added to
_   (ڳوليو_Embedded_Snapshot(i_module->wasm_شروعات, i_module->wasm_پڇاڙي, secName, &drop_شروعات, &drop_پڇاڙي, عدم, عدم));

    orig_سائيز           = (size_t)(i_module->wasm_پڇاڙي - i_module->wasm_شروعات) - (drop_شروعات ? (size_t)(drop_پڇاڙي - drop_شروعات) : 0);
    nameLen            = (u32)لڙي_ڊيگهه(secName);
    name_Len_Leb_سائيز     = سائيز_جو_LEB_u32(nameLen);
    payload_سائيز        = name_Len_Leb_سائيز + nameLen + (u32)bw.سائيز;
    payload_سائيز_Leb_سائيز = سائيز_جو_LEB_u32(payload_سائيز);
    ڪل_سائيز          = orig_سائيز + 1 + payload_سائيز_Leb_سائيز + payload_سائيز;

    ٻاهريون = (u8*)m3_Malloc("wasm_snapshot_module", ڪل_سائيز);
    _throwifnull(ٻاهريون);

    p = ٻاهريون;

    إذا (drop_شروعات) {
        size_t before = (size_t)(drop_شروعات - i_module->wasm_شروعات);
        size_t after  = (size_t)(i_module->wasm_پڇاڙي - drop_پڇاڙي);

        يادگيري_نقل(p, i_module->wasm_شروعات, before);
        p += before;
        يادگيري_نقل(p, drop_پڇاڙي, after);
        p += after;
    } وإلا {
        يادگيري_نقل(p, i_module->wasm_شروعات, orig_سائيز);
        p += orig_سائيز;
    }

    *p++ = 0x00; // custom section
    p    = EmitLEB_u32(p, payload_سائيز);
    p    = EmitLEB_u32(p, nameLen);
    يادگيري_نقل(p, secName, nameLen);
    p += nameLen;
    يادگيري_نقل(p, bw.بفر, bw.سائيز);

    *o_بائيٽون = ٻاهريون;
    *o_سائيز  = ڪل_سائيز;
    ٻاهريون      = عدم;

_catch:
    m3_Free(bw.بفر);
    m3_Free(ٻاهريون);

    أرجع نتيجو;
#وإلا
    (فراغ)io_runtime;
    (فراغ)i_module;
    (فراغ)i_name;
    (فراغ)o_بائيٽون;
    (فراغ)o_سائيز;
    أرجع "snapshots are not available in this build of Wasm3";
#جيڪڏهن_پورو
}
