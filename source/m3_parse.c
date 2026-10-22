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
M3_نتيجو Parse_ڍير_Type (IM3Module i_module, m3type_t* o_ڍير_بٽ, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    i64 ڍير_2;
_   (ReadLebSigned(&ڍير_2, 33, io_بائيٽون, i_پڇاڙي));

    إذا (ڍير_2 == -d_waType_funcref) {
        *o_ڍير_بٽ = d_m3_Type_ڍير_Abstract;
    } وإلا إذا (ڍير_2 == -d_waType_externref) {
        *o_ڍير_بٽ = d_m3Type_refExtern | d_m3_Type_ڍير_Abstract;
#  إذا d_m3_Has_ڍير_Switching
    } وإلا إذا (ڍير_2 == -d_wa_Type_ڍير_cont) {
        *o_ڍير_بٽ = d_m3Type_refCont | d_m3_Type_ڍير_Abstract;
    } وإلا إذا (ڍير_2 == -d_wa_Type_ڍير_nocont) {
        *o_ڍير_بٽ = d_m3Type_refCont | d_m3_Type_ڍير_None;
#  endif
    } وإلا إذا (ڍير_2 >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) ڍير_2 >= i_module->انگ_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[ڍير_2];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_ڍير_بٽ = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_اشاريو;
    } وإلا {
        _throw(m3Err_invalidTypeId);
    }

    _catch: أرجع نتيجو;
}

#جيڪڏهن_پورو // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_نتيجو Parse_قدر_Type (IM3Module i_module, m3type_t* o_type, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_بائيٽون >= i_پڇاڙي);

#إذا d_m3HasTypedRefs
    إذا (**io_بائيٽون == d_waEncode_ref or **io_بائيٽون == d_waEncode_refNull) {
        u8 lead = *(*io_بائيٽون)++;

        m3type_t ڍير_بٽ = d_m3_Type_ڍير_Abstract;
_       (Parse_ڍير_Type(i_module, &ڍير_بٽ, io_بائيٽون, i_پڇاڙي));

        *o_type = d_m3Type_ref | ڍير_بٽ | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        أرجع نتيجو;
    }
#جيڪڏهن_پورو

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_بائيٽون, i_پڇاڙي));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: أرجع نتيجو;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
ساكن
M3_نتيجو Read_Type_جدول_Type (IM3Module i_module, M3_جدول_Info* o_info, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_سائيز = 0, max_سائيز = 0;

    // the width of the limits, and of every index into the table
    u32 addr_بٽ = 32;

_   (Parse_قدر_Type(i_module, &elemType, io_بائيٽون, i_پڇاڙي));
#إذا d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(بنياد_Type_جو(elemType)));
#وإلا
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#جيڪڏهن_پورو

_   (ReadLEB_u7(&flag, io_بائيٽون, i_پڇاڙي));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#إذا d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#وإلا
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#جيڪڏهن_پورو

    إذا (flag & 0x04u) {
        addr_بٽ = 64;
    }

_   (ReadLebUnsigned(&init_سائيز, addr_بٽ, io_بائيٽون, i_پڇاڙي));
    _throwif("table overflow", init_سائيز > d_m3_Max_Sane_جدول_سائيز);

    إذا (flag & 0x01u) {
_       (ReadLebUnsigned(&max_سائيز, addr_بٽ, io_بائيٽون, i_پڇاڙي));
        _throwif(m3Err_wasmMalformed, max_سائيز < init_سائيز);
        _throwif("table overflow", max_سائيز > d_m3_Max_Sane_جدول_سائيز);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_سائيز  = (u32)init_سائيز;
    o_info->max_سائيز   = (u32)max_سائيز;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    أرجع نتيجو;
}


ساكن
M3_نتيجو Parse_Type_جدول_Type (IM3Module io_module, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    M3_جدول_Info info;

_   (Read_Type_جدول_Type(io_module, &info, io_بائيٽون, i_پڇاڙي));
_   (Module_جوڙيو_جدول(io_module, عدم, &info, false /* isImport */));

    _catch: أرجع نتيجو;
}


M3_نتيجو Parse_Init_اظهار_Typed (M3Module* io_module, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي, m3type_t* o_type);

M3_نتيجو Parse_Type_جدول (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32 انگ_Tables;
_   (ReadLEB_u32(&انگ_Tables, &i_بائيٽون, i_پڇاڙي));                          m3log (parse, "** Table [%d]", انگ_Tables);

    _throwif("too many tables", (u64)انگ_Tables + io_module->انگ_Tables > d_m3_Max_Sane_جدول_ڳڻپ);

    لكل (u32 i = 0; i < انگ_Tables; ++i) {
#إذا d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_اظهار = (i_بائيٽون < i_پڇاڙي) and (*i_بائيٽون == 0x40);
        u32  جدول_اشاريو  = io_module->انگ_Tables;

        إذا (has_Init_اظهار) {
            u8 reserved;
            ++i_بائيٽون;
_           (Read_u8(&reserved, &i_بائيٽون, i_پڇاڙي));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_جدول_Type(io_module, &i_بائيٽون, i_پڇاڙي));

        إذا (has_Init_اظهار) {
            IM3_جدول جدول_2 = io_module->tables[جدول_اشاريو];

            m3type_t initType;

            جدول_2->init_اظهار = i_بائيٽون;
_           (Parse_Init_اظهار_Typed(io_module, &i_بائيٽون, i_پڇاڙي, &initType));
            جدول_2->init_اظهار_سائيز = (u32)(i_بائيٽون - جدول_2->init_اظهار);
            _throwif(m3_Err_wasm_Missing_Init_اظهار, جدول_2->init_اظهار_سائيز <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_جو(initType, جدول_2->type));
        } وإلا {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[جدول_اشاريو]->type));
        }
#وإلا
_       (Parse_Type_جدول_Type(io_module, &i_بائيٽون, i_پڇاڙي));
#جيڪڏهن_پورو
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch: أرجع نتيجو;
}


M3_نتيجو Parse_Type_يادگيري (M3_يادگيري_Info* o_يادگيري, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_صفحو_سائيز = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_بٽ = 32;

_   (ReadLEB_u7(&flag, io_بائيٽون, i_پڇاڙي));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 معلوم = 0x09u;
#إذا d_m3HasAtomics
        معلوم |= 0x02u;
#جيڪڏهن_پورو
#إذا d_m3HasMemory64
        معلوم |= 0x04u;
#جيڪڏهن_پورو
        _throwif(m3Err_wasmMalformed, flag & ~معلوم);
    }

    o_يادگيري->isShared = (flag & (1u << 1)) != 0;

    o_يادگيري->isMemory64 = (flag & (1u << 2)) != 0;
    إذا (o_يادگيري->isMemory64) {
        addr_بٽ = 64;
    }

_   (ReadLebUnsigned(&o_يادگيري->init_صفحا, addr_بٽ, io_بائيٽون, i_پڇاڙي));

    o_يادگيري->max_صفحا = 0;
    o_يادگيري->hasMax   = (flag & (1u << 0)) != 0;
    إذا (o_يادگيري->hasMax) {
_       (ReadLebUnsigned(&o_يادگيري->max_صفحا, addr_بٽ, io_بائيٽون, i_پڇاڙي));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_يادگيري->max_صفحا < o_يادگيري->init_صفحا);
    }

    _throwif("shared memory must have maximum", o_يادگيري->isShared and not o_يادگيري->hasMax);

    o_يادگيري->صفحو_سائيز = 0;
    إذا (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_صفحو_سائيز, io_بائيٽون, i_پڇاڙي));

        _throwif("invalid custom page size", log_صفحو_سائيز > 16);

        o_يادگيري->صفحو_سائيز = 1u << log_صفحو_سائيز;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_صفحا_Allowed = (log_صفحو_سائيز or addr_بٽ < 64)
                                ? (UINT64_C(1) << (addr_بٽ - log_صفحو_سائيز))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_يادگيري->init_صفحا > max_صفحا_Allowed);
        إذا (o_يادگيري->hasMax) {
            _throwif(m3Err_wasmMalformed, o_يادگيري->max_صفحا > max_صفحا_Allowed);
        }
    }

    _catch: أرجع نتيجو;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
ساكن
M3_نتيجو ParseFuncType (IM3Module io_module, IM3FuncType* o_type, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    IM3FuncType ftype = عدم;
_try {
    u32 انگ_دليلون;
_   (ReadLEB_u32(&انگ_دليلون, io_بائيٽون, i_پڇاڙي));

    _throwif(m3_Err_too_Many_دليلون_Rets, انگ_دليلون > d_m3_Max_Sane_Function_Arg_Ret_ڳڻپ);
#إذا M3_HAS_VLA
    m3type_t argTypes[انگ_دليلون + 1]; // make ubsan happy
#وإلا
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_ڳڻپ];
#جيڪڏهن_پورو
    لكل (u32 a = 0; a < انگ_دليلون; ++a) {
_       (Parse_قدر_Type(io_module, &argTypes[a], io_بائيٽون, i_پڇاڙي));
    }

    u32 انگ_Rets;
_   (ReadLEB_u32(&انگ_Rets, io_بائيٽون, i_پڇاڙي));
    _throwif(m3_Err_too_Many_دليلون_Rets, (u64)(انگ_Rets) + انگ_دليلون > d_m3_Max_Sane_Function_Arg_Ret_ڳڻپ);

_   (AllocFuncType(&ftype, انگ_Rets + انگ_دليلون));
    ftype->انگ_دليلون = انگ_دليلون;
    ftype->انگ_Rets = انگ_Rets;

    لكل (u32 r = 0; r < انگ_Rets; ++r) {
_       (Parse_قدر_Type(io_module, &ftype->types[r], io_بائيٽون, i_پڇاڙي));
    }
    يادگيري_نقل(ftype->types + انگ_Rets, argTypes, انگ_دليلون * حجم(m3type_t));

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع نتيجو;
}


#إذا d_m3_Has_ڍير_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
ساكن
M3_نتيجو ParseContType (IM3Module io_module, IM3FuncType* o_type, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    IM3FuncType ftype = عدم;
_try {
    i64 ft_اشاريو;
_   (ReadLebSigned(&ft_اشاريو, 33, io_بائيٽون, i_پڇاڙي));
    _throwif(m3Err_wasmMalformed, ft_اشاريو < 0 or (u64) ft_اشاريو >= io_module->انگ_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_اشاريو];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 ڪل = underlying->انگ_Rets + underlying->انگ_دليلون;
_   (AllocFuncType(&ftype, ڪل));
    ftype->انگ_Rets        = underlying->انگ_Rets;
    ftype->انگ_دليلون        = underlying->انگ_دليلون;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    إذا (ڪل) {
        يادگيري_نقل(ftype->types, underlying->types, ڪل * حجم(m3type_t));
    }

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع نتيجو;
}

#جيڪڏهن_پورو // d_m3HasStackSwitching


ساكن
M3_نتيجو ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
#إذا d_m3_Has_ڍير_Switching
    إذا (i_form == -35) {
        أرجع ParseContType(io_module, o_type, io_بائيٽون, i_پڇاڙي);
    }
#جيڪڏهن_پورو
    إذا (i_form != -32) { // for Wasm MVP
        أرجع m3Err_wasmMalformed;
    }

    أرجع ParseFuncType(io_module, o_type, io_بائيٽون, i_پڇاڙي);
}


#إذا d_m3_Has_ڍير_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
ساكن
M3_نتيجو SkipTypeDefinition (i8 i_form, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
_try {
    إذا (i_form == -35) {
        i64 ft_اشاريو;
_       (ReadLebSigned(&ft_اشاريو, 33, io_بائيٽون, i_پڇاڙي));
        أرجع نتيجو;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    لكل (u32 half = 0; half < 2; ++half) {
        u32 ڳڻپ;
_       (ReadLEB_u32(&ڳڻپ, io_بائيٽون, i_پڇاڙي));
        _throwif(m3_Err_too_Many_دليلون_Rets, ڳڻپ > d_m3_Max_Sane_Function_Arg_Ret_ڳڻپ);

        لكل (u32 v = 0; v < ڳڻپ; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_بائيٽون, i_پڇاڙي));

            إذا (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 ڍير_2;
_               (ReadLebSigned(&ڍير_2, 33, io_بائيٽون, i_پڇاڙي));
            }
        }
    }

} _catch: أرجع نتيجو;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
ساكن
M3_نتيجو Parse_Rec_گروهه (IM3Module io_module, u32 i_ڳڻپ, u32 i_پهريون_اشاريو,
                        بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    IM3FuncType* placeholders = عدم;
_try {
    بائيٽون_t گروهه_شروعات = *io_بائيٽون;

    placeholders = m3_AllocArray(IM3FuncType, i_ڳڻپ);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    لكل (u32 j = 0; j < i_ڳڻپ; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_بائيٽون, i_پڇاڙي));
_       (SkipTypeDefinition(form, io_بائيٽون, i_پڇاڙي));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 پهريون_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_ڳڻپ, &پهريون_Canonical));

    لكل (u32 j = 0; j < i_ڳڻپ; ++j) {
        placeholders[j]->canonical_اشاريو        = (u16)(پهريون_Canonical + j);
        io_module->funcTypes[i_پهريون_اشاريو + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    لكل (u32 pass = 0; pass < 2; ++pass) {
        بائيٽون_t بائيٽون = گروهه_شروعات;

        لكل (u32 j = 0; j < i_ڳڻپ; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &بائيٽون, i_پڇاڙي));

            إذا ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &بائيٽون, i_پڇاڙي));
                استمر;
            }

            IM3FuncType ftype = عدم;
_           (ParseTypeDefinition(io_module, form, &ftype, &بائيٽون, i_پڇاڙي));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_اشاريو);

            m3_Free(placeholders[j]);
            placeholders[j]                        = عدم;
            io_module->funcTypes[i_پهريون_اشاريو + j] = ftype;
        }

        إذا (pass == 1) {
            *io_بائيٽون = بائيٽون;
        }
    }

} _catch:
    إذا (placeholders) {
        لكل (u32 j = 0; j < i_ڳڻپ; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    أرجع نتيجو;
}

#جيڪڏهن_پورو // d_m3HasStackSwitching


M3_نتيجو ParseSection_Type (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    IM3FuncType ftype = عدم;

_try {
    u32 انگ_Types;
_   (ReadLEB_u32(&انگ_Types, &i_بائيٽون, i_پڇاڙي));                                      m3log (parse, "** Type [%d]", انگ_Types);

    _throwif("too many types", انگ_Types > d_m3_Max_Sane_Types_ڳڻپ);

    إذا (انگ_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, انگ_Types);
        _throwifnull(io_module->funcTypes);
        io_module->انگ_Func_Types = انگ_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#إذا d_m3_Has_ڍير_Switching
        u32 صلاحيت = انگ_Types;
#جيڪڏهن_پورو
        u32 index = 0;

        لكل (u32 i = 0; i < انگ_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_بائيٽون, i_پڇاڙي));

#إذا d_m3_Has_ڍير_Switching
            إذا (form == -50) { // rec
                u32 ڳڻپ;
_               (ReadLEB_u32(&ڳڻپ, &i_بائيٽون, i_پڇاڙي));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + ڳڻپ + (انگ_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_ڳڻپ);

                إذا (needed > صلاحيت) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, صلاحيت);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    صلاحيت                = grown;
                    io_module->انگ_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                إذا (ڳڻپ == 1) {
_                   (ReadLEB_i7(&form, &i_بائيٽون, i_پڇاڙي));
                } وإلا {
_                   (Parse_Rec_گروهه(io_module, ڳڻپ, index, &i_بائيٽون, i_پڇاڙي));
                    index += ڳڻپ;
                    استمر;
                }
            }
#جيڪڏهن_پورو

_           (ParseTypeDefinition(io_module, form, &ftype, &i_بائيٽون, i_پڇاڙي));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_جوڙيو_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = عدم; // ownership transferred to environment
        }

        io_module->انگ_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

} _catch:

    إذا (نتيجو) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->انگ_Func_Types = 0;
    }

    أرجع نتيجو;
}


M3_نتيجو ParseSection_Function (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32 انگ_Functions;
_   (ReadLEB_u32(&انگ_Functions, &i_بائيٽون, i_پڇاڙي));                                  m3log (parse, "** Function [%d]", انگ_Functions);

    _throwif("too many functions", انگ_Functions > d_m3_Max_Sane_Functions_ڳڻپ);

_   (Module_PreallocFunctions(io_module, io_module->انگ_Functions + انگ_Functions));

    لكل (u32 i = 0; i < انگ_Functions; ++i) {
        u32 func_Type_اشاريو;
_       (ReadLEB_u32(&func_Type_اشاريو, &i_بائيٽون, i_پڇاڙي));

_       (Module_جوڙيو_Function(io_module, func_Type_اشاريو, عدم /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch: أرجع نتيجو;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
تعريفنوع بنية M3ImportDesc {
    u8           قسم;
    u32          type_اشاريو;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_جدول_Info  جدول_2;
    M3_يادگيري_Info يادگيري;
} M3ImportDesc;


#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
ساكن
M3_نتيجو Read_ٽيگ_Type (IM3Module i_module, u32* o_type_اشاريو, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_بائيٽون, i_پڇاڙي));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_اشاريو, io_بائيٽون, i_پڇاڙي));
    _throwif(m3Err_unknownType, not i_module or *o_type_اشاريو >= i_module->انگ_Func_Types);

#  إذا !d_m3_Has_ڍير_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_انگ_Results(i_module->funcTypes[*o_type_اشاريو]) != 0);
#  endif

    _catch: أرجع نتيجو;
}

#جيڪڏهن_پورو // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
ساكن
M3_نتيجو ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

_   (Read_u8(&o_desc->قسم, io_بائيٽون, i_پڇاڙي));

    بدل (o_desc->قسم) {
    حالة d_external_قسم_function:
_       (ReadLEB_u32(&o_desc->type_اشاريو, io_بائيٽون, i_پڇاڙي));
        اكسر;

    حالة d_external_قسم_جدول:
_       (Read_Type_جدول_Type(i_module, &o_desc->جدول_2, io_بائيٽون, i_پڇاڙي));
        اكسر;

    حالة d_external_قسم_يادگيري:
_       (Parse_Type_يادگيري(&o_desc->يادگيري, io_بائيٽون, i_پڇاڙي));
        اكسر;

    حالة d_external_قسم_global:
_       (Parse_قدر_Type(i_module, &o_desc->type, io_بائيٽون, i_پڇاڙي));
_       (ReadLEB_u7(&o_desc->isMutable, io_بائيٽون, i_پڇاڙي));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [بنياد_Type_جو(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
    حالة d_external_قسم_ٽيگ:
_       (Read_ٽيگ_Type(i_module, &o_desc->type_اشاريو, io_بائيٽون, i_پڇاڙي));
        اكسر;
#جيڪڏهن_پورو

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع نتيجو;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
ساكن
M3_نتيجو ApplyImportDesc (IM3Module io_module, ثابت M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_نتيجو نتيجو = m3Err_none;

    M3ImportInfo clearImport = { عدم, عدم };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->قسم, io_import->moduleUtf8, io_import->ميدان_Utf8);
    بدل (i_desc->قسم) {
    حالة d_external_قسم_function: {
_       (Module_جوڙيو_Function(io_module, i_desc->type_اشاريو, io_import))
        *io_import = clearImport;

        io_module->انگ_Func_Imports++;
    } اكسر;

    حالة d_external_قسم_جدول: {
        IM3_جدول جدول_2;
_       (Module_جوڙيو_جدول(io_module, &جدول_2, &i_desc->جدول_2, true /* isImport */));
        جدول_2->import = *io_import;
        *io_import    = clearImport;
    } اكسر;

    حالة d_external_قسم_يادگيري: {
        IM3_يادگيري يادگيري;

#إذا d_m3_Has_Multi_يادگيري
        _throwif(m3_Err_too_Many_يادگيري_Sections, io_module->انگ_Memories >= d_m3_Max_Sane_Memories_ڳڻپ);
#وإلا
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_يادگيري_Sections, io_module->انگ_Memories >= 1);
#جيڪڏهن_پورو

_       (Module_جوڙيو_يادگيري(io_module, &يادگيري, &i_desc->يادگيري, true /* isImport */));
        يادگيري->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

    حالة d_external_قسم_global: {
        IM3Global global;
_       (Module_جوڙيو_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
    حالة d_external_قسم_ٽيگ: {
        IM3_ٽيگ ٽيگ;
_       (Module_جوڙيو_ٽيگ(io_module, &ٽيگ, io_module->funcTypes[i_desc->type_اشاريو], true /* isImport */));
        ٽيگ->import = *io_import;
        *io_import  = clearImport;
    } اكسر;
#جيڪڏهن_پورو

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع نتيجو;
}


#إذا d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
ساكن
bool Is_خالي_جايون_Name (بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    u32 ڊيگهائي;
    أرجع (not ReadLEB_u32(&ڊيگهائي, &i_بائيٽون, i_پڇاڙي)) and ڊيگهائي == 0;
}

#جيڪڏهن_پورو // d_m3HasCompactImports


#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching

M3_نتيجو Parse_Section_ٽيگ (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32 انگ_ٽيگ_فهرست;
_   (ReadLEB_u32(&انگ_ٽيگ_فهرست, &i_بائيٽون, i_پڇاڙي));                                       m3log (parse, "** Tag [%d]", انگ_ٽيگ_فهرست);

    _throwif("too many tags", انگ_ٽيگ_فهرست > d_m3_Max_Sane_ٽيگ_فهرست_ڳڻپ);

    لكل (u32 i = 0; i < انگ_ٽيگ_فهرست; ++i) {
        u32 type_اشاريو;
_       (Read_ٽيگ_Type(io_module, &type_اشاريو, &i_بائيٽون, i_پڇاڙي));                      m3log (parse, "    tag: [%d] type: %d", i, type_اشاريو);

_       (Module_جوڙيو_ٽيگ(io_module, عدم, io_module->funcTypes[type_اشاريو], false));
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch: أرجع نتيجو;
}

#جيڪڏهن_پورو // d_m3HasExceptionHandling


M3_نتيجو ParseSection_Import (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    M3ImportInfo import = { عدم, عدم };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 انگ_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 انگ_Entries;
_   (ReadLEB_u32(&انگ_Entries, &i_بائيٽون, i_پڇاڙي));                                    m3log (parse, "** Import [%d]", انگ_Entries);

    _throwif("too many imports", انگ_Entries > d_m3_Max_Sane_Imports_ڳڻپ);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->انگ_Functions + انگ_Entries));

    لكل (u32 i = 0; i < انگ_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_بائيٽون, i_پڇاڙي));

#إذا d_m3HasCompactImports
        بائيٽون_t ميدان_شروعات = i_بائيٽون;
#جيڪڏهن_پورو
_       (Read_utf8(&import.ميدان_Utf8, &i_بائيٽون, i_پڇاڙي));

#إذا d_m3HasCompactImports
        u8 compact = (i_بائيٽون < i_پڇاڙي) ? *i_بائيٽون : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        إذا ((compact == d_compact_Imports_في_شي_Type or compact == d_compactImports_sharedType) and
            Is_خالي_جايون_Name(ميدان_شروعات, i_پڇاڙي)) {
            ++i_بائيٽون;

            m3_Free(import.ميدان_Utf8);         // it only marked the compact form
            import.ميدان_Utf8 = عدم;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            إذا (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_بائيٽون, i_پڇاڙي));
            }

            u32 انگ_شيون;
_           (ReadLEB_u32(&انگ_شيون, &i_بائيٽون, i_پڇاڙي));                              m3log (parse, "    compact: %d import(s) of '%s'", انگ_شيون, import.moduleUtf8);

            _throwif("too many imports", (u64)انگ_Imports + انگ_شيون > d_m3_Max_Sane_Imports_ڳڻپ);
            انگ_Imports += انگ_شيون;

_           (Module_PreallocFunctions(io_module, io_module->انگ_Functions + انگ_شيون));

            لكل (u32 j = 0; j < انگ_شيون; ++j) {
                M3ImportInfo شي = { عدم, عدم };

                شي.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, لڙي_ڊيگهه(import.moduleUtf8) + 1);

                إذا (شي.moduleUtf8) {
                    نتيجو = Read_utf8(&شي.ميدان_Utf8, &i_بائيٽون, i_پڇاڙي);

                    // the shared form reuses the externtype read above
                    إذا (not نتيجو and compact == d_compact_Imports_في_شي_Type) {
                        نتيجو = ReadImportDesc(io_module, &desc, &i_بائيٽون, i_پڇاڙي);
                    }

                    إذا (not نتيجو) {
                        نتيجو = ApplyImportDesc(io_module, &desc, &شي);
                    }
                } وإلا {
                    نتيجو = m3Err_mallocFailed;
                }

                FreeImportInfo(&شي);
                _throwif(نتيجو, نتيجو);
            }

            FreeImportInfo(&import);
            استمر;
        }
#جيڪڏهن_پورو // d_m3HasCompactImports

        _throwif("too many imports", انگ_Imports >= d_m3_Max_Sane_Imports_ڳڻپ);
        ++انگ_Imports;

_       (ReadImportDesc(io_module, &desc, &i_بائيٽون, i_پڇاڙي));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    أرجع نتيجو;
}


M3_نتيجو ParseSection_Export (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو    نتيجو = m3Err_none;
    ثابت حرف* utf8   = عدم;

    u32 انگ_Exports;
_   (ReadLEB_u32(&انگ_Exports, &i_بائيٽون, i_پڇاڙي));                                    m3log (parse, "** Export [%d]", انگ_Exports);

    _throwif("too many exports", انگ_Exports > d_m3_Max_Sane_Exports_ڳڻپ);

    // the section order check lets this section through once, so this is
    // every export the module has
    إذا (انگ_Exports) {
        io_module->exports = m3_AllocArray(M3Export, انگ_Exports);
        _throwifnull(io_module->exports);
    }

    لكل (u32 i = 0; i < انگ_Exports; ++i) {
        u8  export_قسم;
        u32 index;
        u32 name_ڊيگهائي;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            بائيٽون_t ڊيگهائي_بائيٽون = i_بائيٽون;
_           (ReadLEB_u32(&name_ڊيگهائي, &ڊيگهائي_بائيٽون, i_پڇاڙي));
        }

_       (Read_utf8(&utf8, &i_بائيٽون, i_پڇاڙي));
_       (Read_u8(&export_قسم, &i_بائيٽون, i_پڇاڙي));
_       (ReadLEB_u32(&index, &i_بائيٽون, i_پڇاڙي));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_قسم, utf8);

#إذا d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        لكل (u32 j = 0; j < i; ++j) {
            ثابت M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_ڊيگهائي == name_ڊيگهائي and memcmp(other->name, utf8, name_ڊيگهائي) == 0);
        }
#جيڪڏهن_پورو

        إذا (export_قسم == d_external_قسم_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->انگ_Functions);
            IM3Function func = &(io_module->functions[index]);
            إذا (func->انگ_نالا < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_ڊيگهائي + 1);
                _throwifnull(name);
                func->نالا[func->انگ_نالا++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } وإلا إذا (export_قسم == d_external_قسم_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->انگ_Globals);
        } وإلا إذا (export_قسم == d_external_قسم_يادگيري) {
            _throwif(m3Err_wasmMalformed, index >= io_module->انگ_Memories);
        } وإلا إذا (export_قسم == d_external_قسم_جدول) {
            _throwif(m3Err_wasmMalformed, index >= io_module->انگ_Tables);
        }
#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
        وإلا إذا (export_قسم == d_external_قسم_ٽيگ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->انگ_ٽيگ_فهرست);
        }
#جيڪڏهن_پورو

        M3Export* داخلا   = &io_module->exports[io_module->انگ_Exports++];
        داخلا->name       = utf8;
        داخلا->name_ڊيگهائي = name_ڊيگهائي;
        داخلا->index      = index;
        داخلا->قسم       = export_قسم;

        utf8 = عدم; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

_catch:
    m3_Free(utf8);
    أرجع نتيجو;
}


M3_نتيجو Parse_Section_شروعات (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32 شروعات_Func_اشاريو;
_   (ReadLEB_u32(&شروعات_Func_اشاريو, &i_بائيٽون, i_پڇاڙي));                                  m3log (parse, "** Start Function: %d", شروعات_Func_اشاريو);

    إذا (شروعات_Func_اشاريو < io_module->انگ_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[شروعات_Func_اشاريو];
        إذا (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->انگ_دليلون != 0 || func->funcType->انگ_Rets != 0);
        }

        io_module->شروعات_Function = شروعات_Func_اشاريو;
    } وإلا {
        نتيجو = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch: أرجع نتيجو;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_نتيجو Parse_Init_اظهار_Typed (M3Module* io_module, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي, m3type_t* o_type)
{
    M3_نتيجو نتيجو = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Compilation compilation;
#وإلا
    M3Compilation compilation;
#جيڪڏهن_پورو
    compilation = (M3Compilation){ .runtime = عدم, .module = io_module, .wasm = *io_بائيٽون, .wasm_پڇاڙي = i_پڇاڙي, .is_Init_اظهار = true };

    نتيجو = Compile_بلاڪ_Statements(&compilation);

    *io_بائيٽون = compilation.wasm;

    إذا (o_type) {
        *o_type = compilation.ڍير_اشاريو ? compilation.type_ڍير[compilation.ڍير_اشاريو - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    أرجع نتيجو;
}


M3_نتيجو Parse_Init_اظهار (M3Module* io_module, بائيٽون_t* io_بائيٽون, cbytes_t i_پڇاڙي)
{
    أرجع Parse_Init_اظهار_Typed(io_module, io_بائيٽون, i_پڇاڙي, عدم);
}


M3_نتيجو ParseSection_Element (IM3Module io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32     انگ_Segments;
    بائيٽون_t pos;
_   (ReadLEB_u32(&انگ_Segments, &i_بائيٽون, i_پڇاڙي));                            m3log (parse, "** Element [%d]", انگ_Segments);

    _throwif("too many element segments", انگ_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, انگ_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->انگ_Element_Segments = انگ_Segments;
    io_module->element_Section_پڇاڙي  = i_پڇاڙي;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_بائيٽون;
    لكل (u32 i = 0; i < انگ_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_پڇاڙي));
#إذا d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#وإلا
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_سرگرم);
#جيڪڏهن_پورو

        segment->گهڻائي   = flags & 0x3;
        segment->is_اظهار = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        إذا (segment->گهڻائي == c_m3_Elem_سرگرم_Idx) {
_           (ReadLEB_u32(&segment->جدول_اشاريو, &pos, i_پڇاڙي));
        }

        bool is_سرگرم = (segment->گهڻائي == c_m3_Elem_سرگرم or segment->گهڻائي == c_m3_Elem_سرگرم_Idx);

        إذا (is_سرگرم) {
            _throwif(m3Err_wasmMalformed, segment->جدول_اشاريو >= io_module->انگ_Tables);

            segment->init_اظهار = pos;
_           (Parse_Init_اظهار(io_module, &pos, i_پڇاڙي));
            segment->init_اظهار_سائيز = (u32)(pos - segment->init_اظهار);
            _throwif(m3_Err_wasm_Missing_Init_اظهار, segment->init_اظهار_سائيز <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        إذا (segment->گهڻائي != c_m3_Elem_سرگرم) {
            إذا (segment->is_اظهار) {
_               (Parse_قدر_Type(io_module, &segment->type, &pos, i_پڇاڙي));
                _throwif(m3Err_wasmMalformed, not IsRefType(بنياد_Type_جو(segment->type)));
            } وإلا {
                u8 elem_قسم;
_               (Read_u8(&elem_قسم, &pos, i_پڇاڙي));
                _throwif(m3Err_wasmMalformed, elem_قسم != 0x00);   // funcref
            }
        }

        إذا (is_سرگرم) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->جدول_اشاريو]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->انگ_Elements, &pos, i_پڇاڙي));
        _throwif("table overflow", segment->انگ_Elements > d_m3_Max_Sane_جدول_سائيز);

        segment->elements = pos;

        لكل (u32 e = 0; e < segment->انگ_Elements; ++e) {
            إذا (segment->is_اظهار) {
_               (Parse_Init_اظهار(io_module, &pos, i_پڇاڙي))
            } وإلا {
                u32 func_اشاريو;
_               (ReadLEB_u32(&func_اشاريو, &pos, i_پڇاڙي));
                _throwif("function index out of range", func_اشاريو >= io_module->انگ_Functions);
_               (Module_DeclareFunction(io_module, func_اشاريو));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_پڇاڙي);           // section size mismatch

    _catch: أرجع نتيجو;
}


M3_نتيجو Parse_Section_ڪوڊ (M3Module* io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو;

    u32 انگ_Functions;
_   (ReadLEB_u32(&انگ_Functions, &i_بائيٽون, i_پڇاڙي));                                  m3log (parse, "** Code [%d]", انگ_Functions);

    إذا (انگ_Functions != io_module->انگ_Functions - io_module->انگ_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    لكل (u32 f = 0; f < انگ_Functions; ++f) {
        ثابت u8* شروعات = i_بائيٽون;

        u32 سائيز;
_       (ReadLEB_u32(&سائيز, &i_بائيٽون, i_پڇاڙي));

        إذا (سائيز) {
            i_بائيٽون += سائيز;

            إذا (i_بائيٽون <= i_پڇاڙي) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->انگ_Func_Imports);

                func->module  = io_module;
                func->wasm    = شروعات;
                func->wasm_پڇاڙي = i_بائيٽون;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } وإلا {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    إذا (not نتيجو and i_بائيٽون != i_پڇاڙي) {
        نتيجو = m3Err_wasmSectionUnderrun;
    }

    أرجع نتيجو;
}


M3_نتيجو Parse_Section_معلومات (M3Module* io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32 انگ_معلومات_Segments;
_   (ReadLEB_u32(&انگ_معلومات_Segments, &i_بائيٽون, i_پڇاڙي));                               m3log (parse, "** Data [%d]", انگ_معلومات_Segments);

    _throwif("too many data segments", انگ_معلومات_Segments > d_m3_Max_Sane_معلومات_Segments);

    io_module->معلومات_Segments = m3_AllocArray(M3_معلومات_Segment, انگ_معلومات_Segments);
    _throwifnull(io_module->معلومات_Segments);
    io_module->انگ_معلومات_Segments = انگ_معلومات_Segments;

    لكل (u32 i = 0; i < انگ_معلومات_Segments; ++i) {
        M3_معلومات_Segment* segment = &io_module->معلومات_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_بائيٽون, i_پڇاڙي));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        إذا (flags == 2) {
_           (ReadLEB_u32(&segment->يادگيري_Region, &i_بائيٽون, i_پڇاڙي));
        }

        إذا (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#إذا !d_m3_Has_Multi_يادگيري
            _throwif(m3Err_wasmMalformed, segment->يادگيري_Region != 0);
#جيڪڏهن_پورو
            _throwif(m3Err_wasmMalformed, segment->يادگيري_Region >= io_module->انگ_Memories);

            segment->init_اظهار = i_بائيٽون;
_           (Parse_Init_اظهار(io_module, &i_بائيٽون, i_پڇاڙي));
            segment->init_اظهار_سائيز = (u32)(i_بائيٽون - segment->init_اظهار);

            _throwif(m3_Err_wasm_Missing_Init_اظهار, segment->init_اظهار_سائيز <= 1);
        }

_       (ReadLEB_u32(&segment->سائيز, &i_بائيٽون, i_پڇاڙي));
        segment->معلومات = i_بائيٽون;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->يادگيري_Region, segment->init_اظهار_سائيز, segment->سائيز);
        i_بائيٽون += segment->سائيز;

        _throwif("data segment underflow", i_بائيٽون > i_پڇاڙي);
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch:

    أرجع نتيجو;
}


M3_نتيجو Parse_Section_معلومات_ڳڻپ (M3Module* io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

_   (ReadLEB_u32(&io_module->معلومات_ڳڻپ, &i_بائيٽون, i_پڇاڙي));                       m3log (parse, "** DataCount [%d]", io_module->معلومات_ڳڻپ);

    io_module->has_معلومات_ڳڻپ = true;

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch: أرجع نتيجو;
}


M3_نتيجو Parse_Section_يادگيري (M3Module* io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32 انگ_Memories;
_   (ReadLEB_u32(&انگ_Memories, &i_بائيٽون, i_پڇاڙي));                                m3log (parse, "** Memory [%d]", انگ_Memories);

#إذا d_m3_Has_Multi_يادگيري
    _throwif(m3_Err_too_Many_يادگيري_Sections,
             (u64)انگ_Memories + io_module->انگ_Memories > d_m3_Max_Sane_Memories_ڳڻپ);
#وإلا
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_يادگيري_Sections, انگ_Memories > 1);
    _throwif(m3_Err_too_Many_يادگيري_Sections, (u64)انگ_Memories + io_module->انگ_Memories > 1);
#جيڪڏهن_پورو

    لكل (u32 i = 0; i < انگ_Memories; ++i) {
        M3_يادگيري_Info info;
_       (Parse_Type_يادگيري(&info, &i_بائيٽون, i_پڇاڙي));
_       (Module_جوڙيو_يادگيري(io_module, عدم, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch: أرجع نتيجو;
}


M3_نتيجو ParseSection_Global (M3Module* io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    u32 انگ_Globals;
_   (ReadLEB_u32(&انگ_Globals, &i_بائيٽون, i_پڇاڙي));                                    m3log (parse, "** Global [%d]", انگ_Globals);

    _throwif("too many globals", انگ_Globals > d_m3_Max_Sane_Globals_ڳڻپ);

    لكل (u32 i = 0; i < انگ_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_قدر_Type(io_module, &type, &i_بائيٽون, i_پڇاڙي));
_       (ReadLEB_u7(&isMutable, &i_بائيٽون, i_پڇاڙي));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [بنياد_Type_جو(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_جوڙيو_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_اظهار = i_بائيٽون;
_       (Parse_Init_اظهار(io_module, &i_بائيٽون, i_پڇاڙي));
        global->init_اظهار_سائيز = (u32)(i_بائيٽون - global->init_اظهار);

        _throwif(m3_Err_wasm_Missing_Init_اظهار, global->init_اظهار_سائيز <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_بائيٽون != i_پڇاڙي);      // section size mismatch

    _catch: أرجع نتيجو;
}


M3_نتيجو ParseSection_Name (M3Module* io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    cstr_t name = عدم;

    طالما (i_بائيٽون < i_پڇاڙي) {
        u8  nameType;
        u32 payload_ڊيگهائي;

_       (ReadLEB_u7(&nameType, &i_بائيٽون, i_پڇاڙي));
_       (ReadLEB_u32(&payload_ڊيگهائي, &i_بائيٽون, i_پڇاڙي));

        بائيٽون_t شروعات = i_بائيٽون;
        _throwif(m3Err_wasmSectionOverrun, payload_ڊيگهائي > (size_t)(i_پڇاڙي - شروعات));

        إذا (nameType == 1) {
            u32 انگ_نالا;
_           (ReadLEB_u32(&انگ_نالا, &i_بائيٽون, i_پڇاڙي));

            _throwif("too many names", انگ_نالا > d_m3_Max_Sane_Functions_ڳڻپ);

            لكل (u32 i = 0; i < انگ_نالا; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_بائيٽون, i_پڇاڙي));
_               (Read_utf8(&name, &i_بائيٽون, i_پڇاڙي));

                إذا (index < io_module->انگ_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    إذا (func->انگ_نالا == 0) {
                        func->نالا[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->انگ_نالا = 1;

                        name = عدم; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_بائيٽون = شروعات + payload_ڊيگهائي;
    }

    _catch: أرجع نتيجو;
}


M3_نتيجو ParseSection_Custom (M3Module* io_module, بائيٽون_t i_بائيٽون, cbytes_t i_پڇاڙي)
{
    M3_نتيجو نتيجو = m3Err_none;

    cstr_t name = عدم;
_   (Read_utf8(&name, &i_بائيٽون, i_پڇاڙي));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    إذا (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_بائيٽون, i_پڇاڙي));
#إذا d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } وإلا إذا (strcmp(name, "snapshot") == 0 || (لڙي_ڊيگهه(name) > 9 && لڙي_ڀيٽ(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, لڙي_ڊيگهه(snapName) + 1);
        إذا (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->معلومات           = i_بائيٽون;
        snap->سائيز           = (u32)(i_پڇاڙي - i_بائيٽون);
        snap->ايندڙ           = io_module->snapshots;
        io_module->snapshots = snap;
#جيڪڏهن_پورو
    } وإلا إذا (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_بائيٽون, i_پڇاڙي));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    أرجع نتيجو;
}


M3_نتيجو ParseModuleSection (M3Module* o_module, u8 i_sectionType, بائيٽون_t i_بائيٽون, u32 i_انگ_بائيٽون)
{
    M3_نتيجو نتيجو = m3Err_none;

    تعريفنوع M3_نتيجو (*M3Parser)(M3Module*, بائيٽون_t, cbytes_t);

    ساكن M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_جدول,        // 4
        Parse_Section_يادگيري,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_شروعات,     // 8
        ParseSection_Element,   // 9
        Parse_Section_ڪوڊ,      // 10
        Parse_Section_معلومات,      // 11
        Parse_Section_معلومات_ڳڻپ, // 12
#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
        Parse_Section_ٽيگ,       // 13
#جيڪڏهن_پورو
    };

    M3Parser parser = عدم;

    إذا (i_sectionType < M3_ڳڻپ_جو(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    إذا (parser) {
        cbytes_t پڇاڙي = i_بائيٽون + i_انگ_بائيٽون;
        نتيجو       = parser(o_module, i_بائيٽون, پڇاڙي);
    } وإلا {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    أرجع نتيجو;
}


M3_نتيجو m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_بائيٽون, u32 i_انگ_بائيٽون)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_انگ_بائيٽون);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_انگ_بائيٽون);
    module->شروعات_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ثابت u8* pos = i_بائيٽون;
    ثابت u8* پڇاڙي = pos + i_انگ_بائيٽون;

    module->wasm_شروعات = pos;
    module->wasm_پڇاڙي   = پڇاڙي;

    u32 magic, نسخو;
_   (Read_u32(&magic, &pos, پڇاڙي));
_   (Read_u32(&نسخو, &pos, پڇاڙي));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_نسخو, نسخو != 1);

#إذا d_m3HasExceptionHandling || d_m3_Has_ڍير_Switching
    // the tag section sits between memory (5) and global (6)
    ساكن ثابت u8 sections_ترتيب[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#وإلا
    ساكن ثابت u8 sections_ترتيب[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#جيڪڏهن_پورو
    u8 expectedSection = 0;

    طالما (pos < پڇاڙي) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, پڇاڙي));

        إذا (section != 0) {
            // Ensure sections appear only once and in order
            طالما (sections_ترتيب[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_ڳڻپ_جو(sections_ترتيب) - 1);
            }
        }

        u32 section_ڊيگهائي;
_       (ReadLEB_u32(&section_ڊيگهائي, &pos, پڇاڙي));
        _throwif(m3Err_wasmMalformed, pos + section_ڊيگهائي > پڇاڙي);

_       (ParseModuleSection(module, section, pos, section_ڊيگهائي));

        pos += section_ڊيگهائي;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    إذا (module->انگ_Functions > module->انگ_Func_Imports) {
        IM3Function پهريون_Non_Import = &module->functions[module->انگ_Func_Imports];
        _throwif(m3Err_wasmMalformed, پهريون_Non_Import->wasm == عدم);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_معلومات_ڳڻپ and module->معلومات_ڳڻپ != module->انگ_معلومات_Segments);

} _catch:

    إذا (نتيجو) {
        m3_FreeModule(module);
        module = عدم;
    }

    *o_module = module;

    أرجع نتيجو;
}
