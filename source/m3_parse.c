//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ضمن "m3_env.h"
#ضمن "m3_compile.h"
#ضمن "m3_exception.h"
#ضمن "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#إذا d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_نتیجہ Parse_ڈھیر_Type (IM3Module i_module, m3type_t* o_ڈھیر_بٹ, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    i64 ڈھیر_2;
_   (ReadLebSigned(&ڈھیر_2, 33, io_بائٹاں, i_سرا));

    إذا (ڈھیر_2 == -d_waType_funcref) {
        *o_ڈھیر_بٹ = d_m3_Type_ڈھیر_Abstract;
    } وإلا إذا (ڈھیر_2 == -d_waType_externref) {
        *o_ڈھیر_بٹ = d_m3Type_refExtern | d_m3_Type_ڈھیر_Abstract;
#  إذا d_m3_Has_ڈھیر_Switching
    } وإلا إذا (ڈھیر_2 == -d_wa_Type_ڈھیر_cont) {
        *o_ڈھیر_بٹ = d_m3Type_refCont | d_m3_Type_ڈھیر_Abstract;
    } وإلا إذا (ڈھیر_2 == -d_wa_Type_ڈھیر_nocont) {
        *o_ڈھیر_بٹ = d_m3Type_refCont | d_m3_Type_ڈھیر_None;
#  endif
    } وإلا إذا (ڈھیر_2 >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) ڈھیر_2 >= i_module->عدد_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[ڈھیر_2];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_ڈھیر_بٹ = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_اشاریہ;
    } وإلا {
        _throw(m3Err_invalidTypeId);
    }

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_نتیجہ Parse_قدر_Type (IM3Module i_module, m3type_t* o_type, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_بائٹاں >= i_سرا);

#إذا d_m3HasTypedRefs
    إذا (**io_بائٹاں == d_waEncode_ref or **io_بائٹاں == d_waEncode_refNull) {
        u8 lead = *(*io_بائٹاں)++;

        m3type_t ڈھیر_بٹ = d_m3_Type_ڈھیر_Abstract;
_       (Parse_ڈھیر_Type(i_module, &ڈھیر_بٹ, io_بائٹاں, i_سرا));

        *o_type = d_m3Type_ref | ڈھیر_بٹ | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        أرجع نتیجہ;
    }
#جے_انت

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_بائٹاں, i_سرا));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: أرجع نتیجہ;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
ساكن
M3_نتیجہ Read_Type_جدول_Type (IM3Module i_module, M3_جدول_Info* o_info, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_ناپ = 0, max_ناپ = 0;

    // the width of the limits, and of every index into the table
    u32 addr_بٹ = 32;

_   (Parse_قدر_Type(i_module, &elemType, io_بائٹاں, i_سرا));
#إذا d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(ادھار_Type_دا(elemType)));
#وإلا
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#جے_انت

_   (ReadLEB_u7(&flag, io_بائٹاں, i_سرا));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#إذا d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#وإلا
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#جے_انت

    إذا (flag & 0x04u) {
        addr_بٹ = 64;
    }

_   (ReadLebUnsigned(&init_ناپ, addr_بٹ, io_بائٹاں, i_سرا));
    _throwif("table overflow", init_ناپ > d_m3_Max_Sane_جدول_ناپ);

    إذا (flag & 0x01u) {
_       (ReadLebUnsigned(&max_ناپ, addr_بٹ, io_بائٹاں, i_سرا));
        _throwif(m3Err_wasmMalformed, max_ناپ < init_ناپ);
        _throwif("table overflow", max_ناپ > d_m3_Max_Sane_جدول_ناپ);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_ناپ  = (u32)init_ناپ;
    o_info->max_ناپ   = (u32)max_ناپ;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    أرجع نتیجہ;
}


ساكن
M3_نتیجہ Parse_Type_جدول_Type (IM3Module io_module, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول_Info info;

_   (Read_Type_جدول_Type(io_module, &info, io_بائٹاں, i_سرا));
_   (Module_جوڑو_جدول(io_module, عدم, &info, false /* isImport */));

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Parse_Init_اظہاریہ_Typed (M3Module* io_module, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا, m3type_t* o_type);

M3_نتیجہ Parse_Type_جدول (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Tables;
_   (ReadLEB_u32(&عدد_Tables, &i_بائٹاں, i_سرا));                          m3log (parse, "** Table [%d]", عدد_Tables);

    _throwif("too many tables", (u64)عدد_Tables + io_module->عدد_Tables > d_m3_Max_Sane_جدول_گنتی);

    لكل (u32 i = 0; i < عدد_Tables; ++i) {
#إذا d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_اظہاریہ = (i_بائٹاں < i_سرا) and (*i_بائٹاں == 0x40);
        u32  جدول_اشاریہ  = io_module->عدد_Tables;

        إذا (has_Init_اظہاریہ) {
            u8 reserved;
            ++i_بائٹاں;
_           (Read_u8(&reserved, &i_بائٹاں, i_سرا));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_جدول_Type(io_module, &i_بائٹاں, i_سرا));

        إذا (has_Init_اظہاریہ) {
            IM3_جدول جدول_2 = io_module->tables[جدول_اشاریہ];

            m3type_t initType;

            جدول_2->init_اظہاریہ = i_بائٹاں;
_           (Parse_Init_اظہاریہ_Typed(io_module, &i_بائٹاں, i_سرا, &initType));
            جدول_2->init_اظہاریہ_ناپ = (u32)(i_بائٹاں - جدول_2->init_اظہاریہ);
            _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, جدول_2->init_اظہاریہ_ناپ <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_دا(initType, جدول_2->type));
        } وإلا {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[جدول_اشاریہ]->type));
        }
#وإلا
_       (Parse_Type_جدول_Type(io_module, &i_بائٹاں, i_سرا));
#جے_انت
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Parse_Type_حافظہ (M3_حافظہ_Info* o_حافظہ, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_ورقہ_ناپ = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_بٹ = 32;

_   (ReadLEB_u7(&flag, io_بائٹاں, i_سرا));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 معلوم = 0x09u;
#إذا d_m3HasAtomics
        معلوم |= 0x02u;
#جے_انت
#إذا d_m3HasMemory64
        معلوم |= 0x04u;
#جے_انت
        _throwif(m3Err_wasmMalformed, flag & ~معلوم);
    }

    o_حافظہ->isShared = (flag & (1u << 1)) != 0;

    o_حافظہ->isMemory64 = (flag & (1u << 2)) != 0;
    إذا (o_حافظہ->isMemory64) {
        addr_بٹ = 64;
    }

_   (ReadLebUnsigned(&o_حافظہ->init_ورقے, addr_بٹ, io_بائٹاں, i_سرا));

    o_حافظہ->max_ورقے = 0;
    o_حافظہ->hasMax   = (flag & (1u << 0)) != 0;
    إذا (o_حافظہ->hasMax) {
_       (ReadLebUnsigned(&o_حافظہ->max_ورقے, addr_بٹ, io_بائٹاں, i_سرا));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_حافظہ->max_ورقے < o_حافظہ->init_ورقے);
    }

    _throwif("shared memory must have maximum", o_حافظہ->isShared and not o_حافظہ->hasMax);

    o_حافظہ->ورقہ_ناپ = 0;
    إذا (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_ورقہ_ناپ, io_بائٹاں, i_سرا));

        _throwif("invalid custom page size", log_ورقہ_ناپ > 16);

        o_حافظہ->ورقہ_ناپ = 1u << log_ورقہ_ناپ;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_ورقے_Allowed = (log_ورقہ_ناپ or addr_بٹ < 64)
                                ? (UINT64_C(1) << (addr_بٹ - log_ورقہ_ناپ))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_حافظہ->init_ورقے > max_ورقے_Allowed);
        إذا (o_حافظہ->hasMax) {
            _throwif(m3Err_wasmMalformed, o_حافظہ->max_ورقے > max_ورقے_Allowed);
        }
    }

    _catch: أرجع نتیجہ;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
ساكن
M3_نتیجہ ParseFuncType (IM3Module io_module, IM3FuncType* o_type, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    IM3FuncType ftype = عدم;
_try {
    u32 عدد_دلیلاں;
_   (ReadLEB_u32(&عدد_دلیلاں, io_بائٹاں, i_سرا));

    _throwif(m3_Err_too_Many_دلیلاں_Rets, عدد_دلیلاں > d_m3_Max_Sane_Function_Arg_Ret_گنتی);
#إذا M3_HAS_VLA
    m3type_t argTypes[عدد_دلیلاں + 1]; // make ubsan happy
#وإلا
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_گنتی];
#جے_انت
    لكل (u32 a = 0; a < عدد_دلیلاں; ++a) {
_       (Parse_قدر_Type(io_module, &argTypes[a], io_بائٹاں, i_سرا));
    }

    u32 عدد_Rets;
_   (ReadLEB_u32(&عدد_Rets, io_بائٹاں, i_سرا));
    _throwif(m3_Err_too_Many_دلیلاں_Rets, (u64)(عدد_Rets) + عدد_دلیلاں > d_m3_Max_Sane_Function_Arg_Ret_گنتی);

_   (AllocFuncType(&ftype, عدد_Rets + عدد_دلیلاں));
    ftype->عدد_دلیلاں = عدد_دلیلاں;
    ftype->عدد_Rets = عدد_Rets;

    لكل (u32 r = 0; r < عدد_Rets; ++r) {
_       (Parse_قدر_Type(io_module, &ftype->types[r], io_بائٹاں, i_سرا));
    }
    حافظہ_نقل(ftype->types + عدد_Rets, argTypes, عدد_دلیلاں * حجم(m3type_t));

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع نتیجہ;
}


#إذا d_m3_Has_ڈھیر_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
ساكن
M3_نتیجہ ParseContType (IM3Module io_module, IM3FuncType* o_type, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    IM3FuncType ftype = عدم;
_try {
    i64 ft_اشاریہ;
_   (ReadLebSigned(&ft_اشاریہ, 33, io_بائٹاں, i_سرا));
    _throwif(m3Err_wasmMalformed, ft_اشاریہ < 0 or (u64) ft_اشاریہ >= io_module->عدد_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_اشاریہ];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 کل = underlying->عدد_Rets + underlying->عدد_دلیلاں;
_   (AllocFuncType(&ftype, کل));
    ftype->عدد_Rets        = underlying->عدد_Rets;
    ftype->عدد_دلیلاں        = underlying->عدد_دلیلاں;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    إذا (کل) {
        حافظہ_نقل(ftype->types, underlying->types, کل * حجم(m3type_t));
    }

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع نتیجہ;
}

#جے_انت // d_m3HasStackSwitching


ساكن
M3_نتیجہ ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
#إذا d_m3_Has_ڈھیر_Switching
    إذا (i_form == -35) {
        أرجع ParseContType(io_module, o_type, io_بائٹاں, i_سرا);
    }
#جے_انت
    إذا (i_form != -32) { // for Wasm MVP
        أرجع m3Err_wasmMalformed;
    }

    أرجع ParseFuncType(io_module, o_type, io_بائٹاں, i_سرا);
}


#إذا d_m3_Has_ڈھیر_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
ساكن
M3_نتیجہ SkipTypeDefinition (i8 i_form, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
_try {
    إذا (i_form == -35) {
        i64 ft_اشاریہ;
_       (ReadLebSigned(&ft_اشاریہ, 33, io_بائٹاں, i_سرا));
        أرجع نتیجہ;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    لكل (u32 half = 0; half < 2; ++half) {
        u32 گنتی;
_       (ReadLEB_u32(&گنتی, io_بائٹاں, i_سرا));
        _throwif(m3_Err_too_Many_دلیلاں_Rets, گنتی > d_m3_Max_Sane_Function_Arg_Ret_گنتی);

        لكل (u32 v = 0; v < گنتی; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_بائٹاں, i_سرا));

            إذا (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 ڈھیر_2;
_               (ReadLebSigned(&ڈھیر_2, 33, io_بائٹاں, i_سرا));
            }
        }
    }

} _catch: أرجع نتیجہ;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
ساكن
M3_نتیجہ Parse_Rec_گروہ (IM3Module io_module, u32 i_گنتی, u32 i_پہلا_اشاریہ,
                        بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    IM3FuncType* placeholders = عدم;
_try {
    بائٹاں_t گروہ_شروع = *io_بائٹاں;

    placeholders = m3_AllocArray(IM3FuncType, i_گنتی);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    لكل (u32 j = 0; j < i_گنتی; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_بائٹاں, i_سرا));
_       (SkipTypeDefinition(form, io_بائٹاں, i_سرا));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 پہلا_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_گنتی, &پہلا_Canonical));

    لكل (u32 j = 0; j < i_گنتی; ++j) {
        placeholders[j]->canonical_اشاریہ        = (u16)(پہلا_Canonical + j);
        io_module->funcTypes[i_پہلا_اشاریہ + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    لكل (u32 pass = 0; pass < 2; ++pass) {
        بائٹاں_t بائٹاں = گروہ_شروع;

        لكل (u32 j = 0; j < i_گنتی; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &بائٹاں, i_سرا));

            إذا ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &بائٹاں, i_سرا));
                استمر;
            }

            IM3FuncType ftype = عدم;
_           (ParseTypeDefinition(io_module, form, &ftype, &بائٹاں, i_سرا));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_اشاریہ);

            m3_Free(placeholders[j]);
            placeholders[j]                        = عدم;
            io_module->funcTypes[i_پہلا_اشاریہ + j] = ftype;
        }

        إذا (pass == 1) {
            *io_بائٹاں = بائٹاں;
        }
    }

} _catch:
    إذا (placeholders) {
        لكل (u32 j = 0; j < i_گنتی; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    أرجع نتیجہ;
}

#جے_انت // d_m3HasStackSwitching


M3_نتیجہ ParseSection_Type (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    IM3FuncType ftype = عدم;

_try {
    u32 عدد_Types;
_   (ReadLEB_u32(&عدد_Types, &i_بائٹاں, i_سرا));                                      m3log (parse, "** Type [%d]", عدد_Types);

    _throwif("too many types", عدد_Types > d_m3_Max_Sane_Types_گنتی);

    إذا (عدد_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, عدد_Types);
        _throwifnull(io_module->funcTypes);
        io_module->عدد_Func_Types = عدد_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#إذا d_m3_Has_ڈھیر_Switching
        u32 سمرتھا = عدد_Types;
#جے_انت
        u32 index = 0;

        لكل (u32 i = 0; i < عدد_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_بائٹاں, i_سرا));

#إذا d_m3_Has_ڈھیر_Switching
            إذا (form == -50) { // rec
                u32 گنتی;
_               (ReadLEB_u32(&گنتی, &i_بائٹاں, i_سرا));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + گنتی + (عدد_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_گنتی);

                إذا (needed > سمرتھا) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, سمرتھا);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    سمرتھا                = grown;
                    io_module->عدد_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                إذا (گنتی == 1) {
_                   (ReadLEB_i7(&form, &i_بائٹاں, i_سرا));
                } وإلا {
_                   (Parse_Rec_گروہ(io_module, گنتی, index, &i_بائٹاں, i_سرا));
                    index += گنتی;
                    استمر;
                }
            }
#جے_انت

_           (ParseTypeDefinition(io_module, form, &ftype, &i_بائٹاں, i_سرا));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_جوڑو_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = عدم; // ownership transferred to environment
        }

        io_module->عدد_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

} _catch:

    إذا (نتیجہ) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->عدد_Func_Types = 0;
    }

    أرجع نتیجہ;
}


M3_نتیجہ ParseSection_Function (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Functions;
_   (ReadLEB_u32(&عدد_Functions, &i_بائٹاں, i_سرا));                                  m3log (parse, "** Function [%d]", عدد_Functions);

    _throwif("too many functions", عدد_Functions > d_m3_Max_Sane_Functions_گنتی);

_   (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_Functions));

    لكل (u32 i = 0; i < عدد_Functions; ++i) {
        u32 func_Type_اشاریہ;
_       (ReadLEB_u32(&func_Type_اشاریہ, &i_بائٹاں, i_سرا));

_       (Module_جوڑو_Function(io_module, func_Type_اشاریہ, عدم /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch: أرجع نتیجہ;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
تعريفنوع بنية M3ImportDesc {
    u8           قسم;
    u32          type_اشاریہ;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_جدول_Info  جدول_2;
    M3_حافظہ_Info حافظہ;
} M3ImportDesc;


#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
ساكن
M3_نتیجہ Read_ٹیگ_Type (IM3Module i_module, u32* o_type_اشاریہ, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_بائٹاں, i_سرا));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_اشاریہ, io_بائٹاں, i_سرا));
    _throwif(m3Err_unknownType, not i_module or *o_type_اشاریہ >= i_module->عدد_Func_Types);

#  إذا !d_m3_Has_ڈھیر_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_عدد_Results(i_module->funcTypes[*o_type_اشاریہ]) != 0);
#  endif

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
ساكن
M3_نتیجہ ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Read_u8(&o_desc->قسم, io_بائٹاں, i_سرا));

    بدل (o_desc->قسم) {
    حالة d_external_قسم_function:
_       (ReadLEB_u32(&o_desc->type_اشاریہ, io_بائٹاں, i_سرا));
        اكسر;

    حالة d_external_قسم_جدول:
_       (Read_Type_جدول_Type(i_module, &o_desc->جدول_2, io_بائٹاں, i_سرا));
        اكسر;

    حالة d_external_قسم_حافظہ:
_       (Parse_Type_حافظہ(&o_desc->حافظہ, io_بائٹاں, i_سرا));
        اكسر;

    حالة d_external_قسم_global:
_       (Parse_قدر_Type(i_module, &o_desc->type, io_بائٹاں, i_سرا));
_       (ReadLEB_u7(&o_desc->isMutable, io_بائٹاں, i_سرا));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [ادھار_Type_دا(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    حالة d_external_قسم_ٹیگ:
_       (Read_ٹیگ_Type(i_module, &o_desc->type_اشاریہ, io_بائٹاں, i_سرا));
        اكسر;
#جے_انت

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع نتیجہ;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
ساكن
M3_نتیجہ ApplyImportDesc (IM3Module io_module, ثابت M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3ImportInfo clearImport = { عدم, عدم };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->قسم, io_import->moduleUtf8, io_import->میدان_Utf8);
    بدل (i_desc->قسم) {
    حالة d_external_قسم_function: {
_       (Module_جوڑو_Function(io_module, i_desc->type_اشاریہ, io_import))
        *io_import = clearImport;

        io_module->عدد_Func_Imports++;
    } اكسر;

    حالة d_external_قسم_جدول: {
        IM3_جدول جدول_2;
_       (Module_جوڑو_جدول(io_module, &جدول_2, &i_desc->جدول_2, true /* isImport */));
        جدول_2->import = *io_import;
        *io_import    = clearImport;
    } اكسر;

    حالة d_external_قسم_حافظہ: {
        IM3_حافظہ حافظہ;

#إذا d_m3_Has_Multi_حافظہ
        _throwif(m3_Err_too_Many_حافظہ_Sections, io_module->عدد_Memories >= d_m3_Max_Sane_Memories_گنتی);
#وإلا
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_حافظہ_Sections, io_module->عدد_Memories >= 1);
#جے_انت

_       (Module_جوڑو_حافظہ(io_module, &حافظہ, &i_desc->حافظہ, true /* isImport */));
        حافظہ->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

    حالة d_external_قسم_global: {
        IM3Global global;
_       (Module_جوڑو_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    حالة d_external_قسم_ٹیگ: {
        IM3_ٹیگ ٹیگ;
_       (Module_جوڑو_ٹیگ(io_module, &ٹیگ, io_module->funcTypes[i_desc->type_اشاریہ], true /* isImport */));
        ٹیگ->import = *io_import;
        *io_import  = clearImport;
    } اكسر;
#جے_انت

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع نتیجہ;
}


#إذا d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
ساكن
bool Is_خالی_تھاواں_Name (بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    u32 لمائی;
    أرجع (not ReadLEB_u32(&لمائی, &i_بائٹاں, i_سرا)) and لمائی == 0;
}

#جے_انت // d_m3HasCompactImports


#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching

M3_نتیجہ Parse_Section_ٹیگ (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_ٹیگ_سوچی;
_   (ReadLEB_u32(&عدد_ٹیگ_سوچی, &i_بائٹاں, i_سرا));                                       m3log (parse, "** Tag [%d]", عدد_ٹیگ_سوچی);

    _throwif("too many tags", عدد_ٹیگ_سوچی > d_m3_Max_Sane_ٹیگ_سوچی_گنتی);

    لكل (u32 i = 0; i < عدد_ٹیگ_سوچی; ++i) {
        u32 type_اشاریہ;
_       (Read_ٹیگ_Type(io_module, &type_اشاریہ, &i_بائٹاں, i_سرا));                      m3log (parse, "    tag: [%d] type: %d", i, type_اشاریہ);

_       (Module_جوڑو_ٹیگ(io_module, عدم, io_module->funcTypes[type_اشاریہ], false));
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch: أرجع نتیجہ;
}

#جے_انت // d_m3HasExceptionHandling


M3_نتیجہ ParseSection_Import (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3ImportInfo import = { عدم, عدم };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 عدد_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 عدد_Entries;
_   (ReadLEB_u32(&عدد_Entries, &i_بائٹاں, i_سرا));                                    m3log (parse, "** Import [%d]", عدد_Entries);

    _throwif("too many imports", عدد_Entries > d_m3_Max_Sane_Imports_گنتی);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_Entries));

    لكل (u32 i = 0; i < عدد_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_بائٹاں, i_سرا));

#إذا d_m3HasCompactImports
        بائٹاں_t میدان_شروع = i_بائٹاں;
#جے_انت
_       (Read_utf8(&import.میدان_Utf8, &i_بائٹاں, i_سرا));

#إذا d_m3HasCompactImports
        u8 compact = (i_بائٹاں < i_سرا) ? *i_بائٹاں : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        إذا ((compact == d_compact_Imports_فی_شے_Type or compact == d_compactImports_sharedType) and
            Is_خالی_تھاواں_Name(میدان_شروع, i_سرا)) {
            ++i_بائٹاں;

            m3_Free(import.میدان_Utf8);         // it only marked the compact form
            import.میدان_Utf8 = عدم;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            إذا (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_بائٹاں, i_سرا));
            }

            u32 عدد_اشیاء;
_           (ReadLEB_u32(&عدد_اشیاء, &i_بائٹاں, i_سرا));                              m3log (parse, "    compact: %d import(s) of '%s'", عدد_اشیاء, import.moduleUtf8);

            _throwif("too many imports", (u64)عدد_Imports + عدد_اشیاء > d_m3_Max_Sane_Imports_گنتی);
            عدد_Imports += عدد_اشیاء;

_           (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_اشیاء));

            لكل (u32 j = 0; j < عدد_اشیاء; ++j) {
                M3ImportInfo شے = { عدم, عدم };

                شے.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, لڑی_لمبائی(import.moduleUtf8) + 1);

                إذا (شے.moduleUtf8) {
                    نتیجہ = Read_utf8(&شے.میدان_Utf8, &i_بائٹاں, i_سرا);

                    // the shared form reuses the externtype read above
                    إذا (not نتیجہ and compact == d_compact_Imports_فی_شے_Type) {
                        نتیجہ = ReadImportDesc(io_module, &desc, &i_بائٹاں, i_سرا);
                    }

                    إذا (not نتیجہ) {
                        نتیجہ = ApplyImportDesc(io_module, &desc, &شے);
                    }
                } وإلا {
                    نتیجہ = m3Err_mallocFailed;
                }

                FreeImportInfo(&شے);
                _throwif(نتیجہ, نتیجہ);
            }

            FreeImportInfo(&import);
            استمر;
        }
#جے_انت // d_m3HasCompactImports

        _throwif("too many imports", عدد_Imports >= d_m3_Max_Sane_Imports_گنتی);
        ++عدد_Imports;

_       (ReadImportDesc(io_module, &desc, &i_بائٹاں, i_سرا));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    أرجع نتیجہ;
}


M3_نتیجہ ParseSection_Export (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ    نتیجہ = m3Err_none;
    ثابت حرف* utf8   = عدم;

    u32 عدد_Exports;
_   (ReadLEB_u32(&عدد_Exports, &i_بائٹاں, i_سرا));                                    m3log (parse, "** Export [%d]", عدد_Exports);

    _throwif("too many exports", عدد_Exports > d_m3_Max_Sane_Exports_گنتی);

    // the section order check lets this section through once, so this is
    // every export the module has
    إذا (عدد_Exports) {
        io_module->exports = m3_AllocArray(M3Export, عدد_Exports);
        _throwifnull(io_module->exports);
    }

    لكل (u32 i = 0; i < عدد_Exports; ++i) {
        u8  export_قسم;
        u32 index;
        u32 name_لمائی;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            بائٹاں_t لمائی_بائٹاں = i_بائٹاں;
_           (ReadLEB_u32(&name_لمائی, &لمائی_بائٹاں, i_سرا));
        }

_       (Read_utf8(&utf8, &i_بائٹاں, i_سرا));
_       (Read_u8(&export_قسم, &i_بائٹاں, i_سرا));
_       (ReadLEB_u32(&index, &i_بائٹاں, i_سرا));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_قسم, utf8);

#إذا d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        لكل (u32 j = 0; j < i; ++j) {
            ثابت M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_لمائی == name_لمائی and memcmp(other->name, utf8, name_لمائی) == 0);
        }
#جے_انت

        إذا (export_قسم == d_external_قسم_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Functions);
            IM3Function func = &(io_module->functions[index]);
            إذا (func->عدد_ناں < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_لمائی + 1);
                _throwifnull(name);
                func->ناں[func->عدد_ناں++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } وإلا إذا (export_قسم == d_external_قسم_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Globals);
        } وإلا إذا (export_قسم == d_external_قسم_حافظہ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Memories);
        } وإلا إذا (export_قسم == d_external_قسم_جدول) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Tables);
        }
#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
        وإلا إذا (export_قسم == d_external_قسم_ٹیگ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_ٹیگ_سوچی);
        }
#جے_انت

        M3Export* اندراج   = &io_module->exports[io_module->عدد_Exports++];
        اندراج->name       = utf8;
        اندراج->name_لمائی = name_لمائی;
        اندراج->index      = index;
        اندراج->قسم       = export_قسم;

        utf8 = عدم; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

_catch:
    m3_Free(utf8);
    أرجع نتیجہ;
}


M3_نتیجہ Parse_Section_شروع (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 شروع_Func_اشاریہ;
_   (ReadLEB_u32(&شروع_Func_اشاریہ, &i_بائٹاں, i_سرا));                                  m3log (parse, "** Start Function: %d", شروع_Func_اشاریہ);

    إذا (شروع_Func_اشاریہ < io_module->عدد_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[شروع_Func_اشاریہ];
        إذا (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->عدد_دلیلاں != 0 || func->funcType->عدد_Rets != 0);
        }

        io_module->شروع_Function = شروع_Func_اشاریہ;
    } وإلا {
        نتیجہ = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch: أرجع نتیجہ;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_نتیجہ Parse_Init_اظہاریہ_Typed (M3Module* io_module, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا, m3type_t* o_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Compilation compilation;
#وإلا
    M3Compilation compilation;
#جے_انت
    compilation = (M3Compilation){ .runtime = عدم, .module = io_module, .wasm = *io_بائٹاں, .wasm_سرا = i_سرا, .is_Init_اظہاریہ = true };

    نتیجہ = Compile_بلاک_Statements(&compilation);

    *io_بائٹاں = compilation.wasm;

    إذا (o_type) {
        *o_type = compilation.ڈھیر_اشاریہ ? compilation.type_ڈھیر[compilation.ڈھیر_اشاریہ - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    أرجع نتیجہ;
}


M3_نتیجہ Parse_Init_اظہاریہ (M3Module* io_module, بائٹاں_t* io_بائٹاں, cbytes_t i_سرا)
{
    أرجع Parse_Init_اظہاریہ_Typed(io_module, io_بائٹاں, i_سرا, عدم);
}


M3_نتیجہ ParseSection_Element (IM3Module io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32     عدد_Segments;
    بائٹاں_t pos;
_   (ReadLEB_u32(&عدد_Segments, &i_بائٹاں, i_سرا));                            m3log (parse, "** Element [%d]", عدد_Segments);

    _throwif("too many element segments", عدد_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, عدد_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->عدد_Element_Segments = عدد_Segments;
    io_module->element_Section_سرا  = i_سرا;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_بائٹاں;
    لكل (u32 i = 0; i < عدد_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_سرا));
#إذا d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#وإلا
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_سرگرم);
#جے_انت

        segment->منوال   = flags & 0x3;
        segment->is_اظہاریہ = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        إذا (segment->منوال == c_m3_Elem_سرگرم_Idx) {
_           (ReadLEB_u32(&segment->جدول_اشاریہ, &pos, i_سرا));
        }

        bool is_سرگرم = (segment->منوال == c_m3_Elem_سرگرم or segment->منوال == c_m3_Elem_سرگرم_Idx);

        إذا (is_سرگرم) {
            _throwif(m3Err_wasmMalformed, segment->جدول_اشاریہ >= io_module->عدد_Tables);

            segment->init_اظہاریہ = pos;
_           (Parse_Init_اظہاریہ(io_module, &pos, i_سرا));
            segment->init_اظہاریہ_ناپ = (u32)(pos - segment->init_اظہاریہ);
            _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, segment->init_اظہاریہ_ناپ <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        إذا (segment->منوال != c_m3_Elem_سرگرم) {
            إذا (segment->is_اظہاریہ) {
_               (Parse_قدر_Type(io_module, &segment->type, &pos, i_سرا));
                _throwif(m3Err_wasmMalformed, not IsRefType(ادھار_Type_دا(segment->type)));
            } وإلا {
                u8 elem_قسم;
_               (Read_u8(&elem_قسم, &pos, i_سرا));
                _throwif(m3Err_wasmMalformed, elem_قسم != 0x00);   // funcref
            }
        }

        إذا (is_سرگرم) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->جدول_اشاریہ]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->عدد_Elements, &pos, i_سرا));
        _throwif("table overflow", segment->عدد_Elements > d_m3_Max_Sane_جدول_ناپ);

        segment->elements = pos;

        لكل (u32 e = 0; e < segment->عدد_Elements; ++e) {
            إذا (segment->is_اظہاریہ) {
_               (Parse_Init_اظہاریہ(io_module, &pos, i_سرا))
            } وإلا {
                u32 func_اشاریہ;
_               (ReadLEB_u32(&func_اشاریہ, &pos, i_سرا));
                _throwif("function index out of range", func_اشاریہ >= io_module->عدد_Functions);
_               (Module_DeclareFunction(io_module, func_اشاریہ));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_سرا);           // section size mismatch

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Parse_Section_کوڈ (M3Module* io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ;

    u32 عدد_Functions;
_   (ReadLEB_u32(&عدد_Functions, &i_بائٹاں, i_سرا));                                  m3log (parse, "** Code [%d]", عدد_Functions);

    إذا (عدد_Functions != io_module->عدد_Functions - io_module->عدد_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    لكل (u32 f = 0; f < عدد_Functions; ++f) {
        ثابت u8* شروع = i_بائٹاں;

        u32 ناپ;
_       (ReadLEB_u32(&ناپ, &i_بائٹاں, i_سرا));

        إذا (ناپ) {
            i_بائٹاں += ناپ;

            إذا (i_بائٹاں <= i_سرا) {
                /*
                const u8 * ptr = i_bytes - size;

                u32 numLocalBlocks;
_               (ReadLEB_u32 (& numLocalBlocks, & ptr, i_end));                                      m3log (parse, "    code size: %-4d", size);

                u32 numLocals = 0;

                for (u32 l = 0; l < numLocalBlocks; ++l)
                {
                    u32 varCount;
                    i8 wasmType;
                    u8 normalType;

_                   (ReadLEB_u32 (& varCount, & ptr, i_end));
_                   (ReadLEB_i7 (& wasmType, & ptr, i_end));
_                   (NormalizeType (& normalType, wasmType));

                    numLocals += varCount;                                                      m3log (parse, "      %2d locals; type: '%s'", varCount, c_waTypes [normalType]);
                }
                 */

                IM3Function func = Module_GetFunction(io_module, f + io_module->عدد_Func_Imports);

                func->module  = io_module;
                func->wasm    = شروع;
                func->wasm_سرا = i_بائٹاں;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } وإلا {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    إذا (not نتیجہ and i_بائٹاں != i_سرا) {
        نتیجہ = m3Err_wasmSectionUnderrun;
    }

    أرجع نتیجہ;
}


M3_نتیجہ Parse_Section_انکڑے (M3Module* io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_انکڑے_Segments;
_   (ReadLEB_u32(&عدد_انکڑے_Segments, &i_بائٹاں, i_سرا));                               m3log (parse, "** Data [%d]", عدد_انکڑے_Segments);

    _throwif("too many data segments", عدد_انکڑے_Segments > d_m3_Max_Sane_انکڑے_Segments);

    io_module->انکڑے_Segments = m3_AllocArray(M3_انکڑے_Segment, عدد_انکڑے_Segments);
    _throwifnull(io_module->انکڑے_Segments);
    io_module->عدد_انکڑے_Segments = عدد_انکڑے_Segments;

    لكل (u32 i = 0; i < عدد_انکڑے_Segments; ++i) {
        M3_انکڑے_Segment* segment = &io_module->انکڑے_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_بائٹاں, i_سرا));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        إذا (flags == 2) {
_           (ReadLEB_u32(&segment->حافظہ_Region, &i_بائٹاں, i_سرا));
        }

        إذا (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#إذا !d_m3_Has_Multi_حافظہ
            _throwif(m3Err_wasmMalformed, segment->حافظہ_Region != 0);
#جے_انت
            _throwif(m3Err_wasmMalformed, segment->حافظہ_Region >= io_module->عدد_Memories);

            segment->init_اظہاریہ = i_بائٹاں;
_           (Parse_Init_اظہاریہ(io_module, &i_بائٹاں, i_سرا));
            segment->init_اظہاریہ_ناپ = (u32)(i_بائٹاں - segment->init_اظہاریہ);

            _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, segment->init_اظہاریہ_ناپ <= 1);
        }

_       (ReadLEB_u32(&segment->ناپ, &i_بائٹاں, i_سرا));
        segment->انکڑے = i_بائٹاں;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->حافظہ_Region, segment->init_اظہاریہ_ناپ, segment->ناپ);
        i_بائٹاں += segment->ناپ;

        _throwif("data segment underflow", i_بائٹاں > i_سرا);
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch:

    أرجع نتیجہ;
}


M3_نتیجہ Parse_Section_انکڑے_گنتی (M3Module* io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (ReadLEB_u32(&io_module->انکڑے_گنتی, &i_بائٹاں, i_سرا));                       m3log (parse, "** DataCount [%d]", io_module->انکڑے_گنتی);

    io_module->has_انکڑے_گنتی = true;

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch: أرجع نتیجہ;
}


M3_نتیجہ Parse_Section_حافظہ (M3Module* io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Memories;
_   (ReadLEB_u32(&عدد_Memories, &i_بائٹاں, i_سرا));                                m3log (parse, "** Memory [%d]", عدد_Memories);

#إذا d_m3_Has_Multi_حافظہ
    _throwif(m3_Err_too_Many_حافظہ_Sections,
             (u64)عدد_Memories + io_module->عدد_Memories > d_m3_Max_Sane_Memories_گنتی);
#وإلا
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_حافظہ_Sections, عدد_Memories > 1);
    _throwif(m3_Err_too_Many_حافظہ_Sections, (u64)عدد_Memories + io_module->عدد_Memories > 1);
#جے_انت

    لكل (u32 i = 0; i < عدد_Memories; ++i) {
        M3_حافظہ_Info info;
_       (Parse_Type_حافظہ(&info, &i_بائٹاں, i_سرا));
_       (Module_جوڑو_حافظہ(io_module, عدم, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch: أرجع نتیجہ;
}


M3_نتیجہ ParseSection_Global (M3Module* io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Globals;
_   (ReadLEB_u32(&عدد_Globals, &i_بائٹاں, i_سرا));                                    m3log (parse, "** Global [%d]", عدد_Globals);

    _throwif("too many globals", عدد_Globals > d_m3_Max_Sane_Globals_گنتی);

    لكل (u32 i = 0; i < عدد_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_قدر_Type(io_module, &type, &i_بائٹاں, i_سرا));
_       (ReadLEB_u7(&isMutable, &i_بائٹاں, i_سرا));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [ادھار_Type_دا(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_جوڑو_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_اظہاریہ = i_بائٹاں;
_       (Parse_Init_اظہاریہ(io_module, &i_بائٹاں, i_سرا));
        global->init_اظہاریہ_ناپ = (u32)(i_بائٹاں - global->init_اظہاریہ);

        _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, global->init_اظہاریہ_ناپ <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_بائٹاں != i_سرا);      // section size mismatch

    _catch: أرجع نتیجہ;
}


M3_نتیجہ ParseSection_Name (M3Module* io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    cstr_t name = عدم;

    طالما (i_بائٹاں < i_سرا) {
        u8  nameType;
        u32 payload_لمائی;

_       (ReadLEB_u7(&nameType, &i_بائٹاں, i_سرا));
_       (ReadLEB_u32(&payload_لمائی, &i_بائٹاں, i_سرا));

        بائٹاں_t شروع = i_بائٹاں;
        _throwif(m3Err_wasmSectionOverrun, payload_لمائی > (size_t)(i_سرا - شروع));

        إذا (nameType == 1) {
            u32 عدد_ناں;
_           (ReadLEB_u32(&عدد_ناں, &i_بائٹاں, i_سرا));

            _throwif("too many names", عدد_ناں > d_m3_Max_Sane_Functions_گنتی);

            لكل (u32 i = 0; i < عدد_ناں; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_بائٹاں, i_سرا));
_               (Read_utf8(&name, &i_بائٹاں, i_سرا));

                إذا (index < io_module->عدد_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    إذا (func->عدد_ناں == 0) {
                        func->ناں[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->عدد_ناں = 1;

                        name = عدم; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_بائٹاں = شروع + payload_لمائی;
    }

    _catch: أرجع نتیجہ;
}


M3_نتیجہ ParseSection_Custom (M3Module* io_module, بائٹاں_t i_بائٹاں, cbytes_t i_سرا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    cstr_t name = عدم;
_   (Read_utf8(&name, &i_بائٹاں, i_سرا));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    إذا (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_بائٹاں, i_سرا));
#إذا d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } وإلا إذا (strcmp(name, "snapshot") == 0 || (لڑی_لمبائی(name) > 9 && لڑی_مقابلہ(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, لڑی_لمبائی(snapName) + 1);
        إذا (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->انکڑے           = i_بائٹاں;
        snap->ناپ           = (u32)(i_سرا - i_بائٹاں);
        snap->اگلا           = io_module->snapshots;
        io_module->snapshots = snap;
#جے_انت
    } وإلا إذا (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_بائٹاں, i_سرا));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    أرجع نتیجہ;
}


M3_نتیجہ ParseModuleSection (M3Module* o_module, u8 i_sectionType, بائٹاں_t i_بائٹاں, u32 i_عدد_بائٹاں)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    تعريفنوع M3_نتیجہ (*M3Parser)(M3Module*, بائٹاں_t, cbytes_t);

    ساكن M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_جدول,        // 4
        Parse_Section_حافظہ,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_شروع,     // 8
        ParseSection_Element,   // 9
        Parse_Section_کوڈ,      // 10
        Parse_Section_انکڑے,      // 11
        Parse_Section_انکڑے_گنتی, // 12
#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
        Parse_Section_ٹیگ,       // 13
#جے_انت
    };

    M3Parser parser = عدم;

    إذا (i_sectionType < M3_گنتی_دا(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    إذا (parser) {
        cbytes_t سرا = i_بائٹاں + i_عدد_بائٹاں;
        نتیجہ       = parser(o_module, i_بائٹاں, سرا);
    } وإلا {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    أرجع نتیجہ;
}


M3_نتیجہ m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_بائٹاں, u32 i_عدد_بائٹاں)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_عدد_بائٹاں);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_عدد_بائٹاں);
    module->شروع_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ثابت u8* pos = i_بائٹاں;
    ثابت u8* سرا = pos + i_عدد_بائٹاں;

    module->wasm_شروع = pos;
    module->wasm_سرا   = سرا;

    u32 magic, نسخہ;
_   (Read_u32(&magic, &pos, سرا));
_   (Read_u32(&نسخہ, &pos, سرا));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_نسخہ, نسخہ != 1);

#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    // the tag section sits between memory (5) and global (6)
    ساكن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#وإلا
    ساكن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#جے_انت
    u8 expectedSection = 0;

    طالما (pos < سرا) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, سرا));

        إذا (section != 0) {
            // Ensure sections appear only once and in order
            طالما (sections_ترتیب[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_گنتی_دا(sections_ترتیب) - 1);
            }
        }

        u32 section_لمائی;
_       (ReadLEB_u32(&section_لمائی, &pos, سرا));
        _throwif(m3Err_wasmMalformed, pos + section_لمائی > سرا);

_       (ParseModuleSection(module, section, pos, section_لمائی));

        pos += section_لمائی;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    إذا (module->عدد_Functions > module->عدد_Func_Imports) {
        IM3Function پہلا_Non_Import = &module->functions[module->عدد_Func_Imports];
        _throwif(m3Err_wasmMalformed, پہلا_Non_Import->wasm == عدم);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_انکڑے_گنتی and module->انکڑے_گنتی != module->عدد_انکڑے_Segments);

} _catch:

    إذا (نتیجہ) {
        m3_FreeModule(module);
        module = عدم;
    }

    *o_module = module;

    أرجع نتیجہ;
}
