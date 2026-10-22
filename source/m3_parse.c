//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#شامل "m3_env.h"
#شامل "m3_compile.h"
#شامل "m3_exception.h"
#شامل "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#اگر d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_نتیجہ Parse_ڈھیر_Type (IM3Module i_module, m3type_t* o_ڈھیر_بٹ, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    i64 ڈھیر_2;
_   (ReadLebSigned(&ڈھیر_2, 33, io_بائٹیں, i_انتہا));

    اگر (ڈھیر_2 == -d_waType_funcref) {
        *o_ڈھیر_بٹ = d_m3_Type_ڈھیر_Abstract;
    } ورنہ اگر (ڈھیر_2 == -d_waType_externref) {
        *o_ڈھیر_بٹ = d_m3Type_refExtern | d_m3_Type_ڈھیر_Abstract;
#  اگر d_m3_Has_ڈھیر_Switching
    } ورنہ اگر (ڈھیر_2 == -d_wa_Type_ڈھیر_cont) {
        *o_ڈھیر_بٹ = d_m3Type_refCont | d_m3_Type_ڈھیر_Abstract;
    } ورنہ اگر (ڈھیر_2 == -d_wa_Type_ڈھیر_nocont) {
        *o_ڈھیر_بٹ = d_m3Type_refCont | d_m3_Type_ڈھیر_None;
#  endif
    } ورنہ اگر (ڈھیر_2 >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) ڈھیر_2 >= i_module->عدد_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[ڈھیر_2];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_ڈھیر_بٹ = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_اشاریہ;
    } ورنہ {
        _throw(m3Err_invalidTypeId);
    }

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_نتیجہ Parse_قدر_Type (IM3Module i_module, m3type_t* o_type, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_بائٹیں >= i_انتہا);

#اگر d_m3HasTypedRefs
    اگر (**io_بائٹیں == d_waEncode_ref or **io_بائٹیں == d_waEncode_refNull) {
        u8 lead = *(*io_بائٹیں)++;

        m3type_t ڈھیر_بٹ = d_m3_Type_ڈھیر_Abstract;
_       (Parse_ڈھیر_Type(i_module, &ڈھیر_بٹ, io_بائٹیں, i_انتہا));

        *o_type = d_m3Type_ref | ڈھیر_بٹ | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        واپس نتیجہ;
    }
#اگر_ختم

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_بائٹیں, i_انتہا));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: واپس نتیجہ;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
ساکن
M3_نتیجہ Read_Type_جدول_Type (IM3Module i_module, M3_جدول_Info* o_info, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_سائز = 0, max_سائز = 0;

    // the width of the limits, and of every index into the table
    u32 addr_بٹ = 32;

_   (Parse_قدر_Type(i_module, &elemType, io_بائٹیں, i_انتہا));
#اگر d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(بنیاد_Type_کا(elemType)));
#ورنہ
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#اگر_ختم

_   (ReadLEB_u7(&flag, io_بائٹیں, i_انتہا));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#اگر d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#ورنہ
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#اگر_ختم

    اگر (flag & 0x04u) {
        addr_بٹ = 64;
    }

_   (ReadLebUnsigned(&init_سائز, addr_بٹ, io_بائٹیں, i_انتہا));
    _throwif("table overflow", init_سائز > d_m3_Max_Sane_جدول_سائز);

    اگر (flag & 0x01u) {
_       (ReadLebUnsigned(&max_سائز, addr_بٹ, io_بائٹیں, i_انتہا));
        _throwif(m3Err_wasmMalformed, max_سائز < init_سائز);
        _throwif("table overflow", max_سائز > d_m3_Max_Sane_جدول_سائز);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_سائز  = (u32)init_سائز;
    o_info->max_سائز   = (u32)max_سائز;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    واپس نتیجہ;
}


ساکن
M3_نتیجہ Parse_Type_جدول_Type (IM3Module io_module, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3_جدول_Info info;

_   (Read_Type_جدول_Type(io_module, &info, io_بائٹیں, i_انتہا));
_   (Module_جوڑو_جدول(io_module, تہی, &info, false /* isImport */));

    _catch: واپس نتیجہ;
}


M3_نتیجہ Parse_Init_اظہاریہ_Typed (M3Module* io_module, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا, m3type_t* o_type);

M3_نتیجہ Parse_Type_جدول (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Tables;
_   (ReadLEB_u32(&عدد_Tables, &i_بائٹیں, i_انتہا));                          m3log (parse, "** Table [%d]", عدد_Tables);

    _throwif("too many tables", (u64)عدد_Tables + io_module->عدد_Tables > d_m3_Max_Sane_جدول_گنتی);

    برائے (u32 i = 0; i < عدد_Tables; ++i) {
#اگر d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_اظہاریہ = (i_بائٹیں < i_انتہا) and (*i_بائٹیں == 0x40);
        u32  جدول_اشاریہ  = io_module->عدد_Tables;

        اگر (has_Init_اظہاریہ) {
            u8 reserved;
            ++i_بائٹیں;
_           (Read_u8(&reserved, &i_بائٹیں, i_انتہا));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_جدول_Type(io_module, &i_بائٹیں, i_انتہا));

        اگر (has_Init_اظہاریہ) {
            IM3_جدول جدول_2 = io_module->tables[جدول_اشاریہ];

            m3type_t initType;

            جدول_2->init_اظہاریہ = i_بائٹیں;
_           (Parse_Init_اظہاریہ_Typed(io_module, &i_بائٹیں, i_انتہا, &initType));
            جدول_2->init_اظہاریہ_سائز = (u32)(i_بائٹیں - جدول_2->init_اظہاریہ);
            _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, جدول_2->init_اظہاریہ_سائز <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_کا(initType, جدول_2->type));
        } ورنہ {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[جدول_اشاریہ]->type));
        }
#ورنہ
_       (Parse_Type_جدول_Type(io_module, &i_بائٹیں, i_انتہا));
#اگر_ختم
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch: واپس نتیجہ;
}


M3_نتیجہ Parse_Type_حافظہ (M3_حافظہ_Info* o_حافظہ, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_صفحہ_سائز = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_بٹ = 32;

_   (ReadLEB_u7(&flag, io_بائٹیں, i_انتہا));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 معلوم = 0x09u;
#اگر d_m3HasAtomics
        معلوم |= 0x02u;
#اگر_ختم
#اگر d_m3HasMemory64
        معلوم |= 0x04u;
#اگر_ختم
        _throwif(m3Err_wasmMalformed, flag & ~معلوم);
    }

    o_حافظہ->isShared = (flag & (1u << 1)) != 0;

    o_حافظہ->isMemory64 = (flag & (1u << 2)) != 0;
    اگر (o_حافظہ->isMemory64) {
        addr_بٹ = 64;
    }

_   (ReadLebUnsigned(&o_حافظہ->init_صفحات, addr_بٹ, io_بائٹیں, i_انتہا));

    o_حافظہ->max_صفحات = 0;
    o_حافظہ->hasMax   = (flag & (1u << 0)) != 0;
    اگر (o_حافظہ->hasMax) {
_       (ReadLebUnsigned(&o_حافظہ->max_صفحات, addr_بٹ, io_بائٹیں, i_انتہا));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_حافظہ->max_صفحات < o_حافظہ->init_صفحات);
    }

    _throwif("shared memory must have maximum", o_حافظہ->isShared and not o_حافظہ->hasMax);

    o_حافظہ->صفحہ_سائز = 0;
    اگر (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_صفحہ_سائز, io_بائٹیں, i_انتہا));

        _throwif("invalid custom page size", log_صفحہ_سائز > 16);

        o_حافظہ->صفحہ_سائز = 1u << log_صفحہ_سائز;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_صفحات_Allowed = (log_صفحہ_سائز or addr_بٹ < 64)
                                ? (UINT64_C(1) << (addr_بٹ - log_صفحہ_سائز))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_حافظہ->init_صفحات > max_صفحات_Allowed);
        اگر (o_حافظہ->hasMax) {
            _throwif(m3Err_wasmMalformed, o_حافظہ->max_صفحات > max_صفحات_Allowed);
        }
    }

    _catch: واپس نتیجہ;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
ساکن
M3_نتیجہ ParseFuncType (IM3Module io_module, IM3FuncType* o_type, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    IM3FuncType ftype = تہی;
_try {
    u32 عدد_دلائل;
_   (ReadLEB_u32(&عدد_دلائل, io_بائٹیں, i_انتہا));

    _throwif(m3_Err_too_Many_دلائل_Rets, عدد_دلائل > d_m3_Max_Sane_Function_Arg_Ret_گنتی);
#اگر M3_HAS_VLA
    m3type_t argTypes[عدد_دلائل + 1]; // make ubsan happy
#ورنہ
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_گنتی];
#اگر_ختم
    برائے (u32 a = 0; a < عدد_دلائل; ++a) {
_       (Parse_قدر_Type(io_module, &argTypes[a], io_بائٹیں, i_انتہا));
    }

    u32 عدد_Rets;
_   (ReadLEB_u32(&عدد_Rets, io_بائٹیں, i_انتہا));
    _throwif(m3_Err_too_Many_دلائل_Rets, (u64)(عدد_Rets) + عدد_دلائل > d_m3_Max_Sane_Function_Arg_Ret_گنتی);

_   (AllocFuncType(&ftype, عدد_Rets + عدد_دلائل));
    ftype->عدد_دلائل = عدد_دلائل;
    ftype->عدد_Rets = عدد_Rets;

    برائے (u32 r = 0; r < عدد_Rets; ++r) {
_       (Parse_قدر_Type(io_module, &ftype->types[r], io_بائٹیں, i_انتہا));
    }
    حافظہ_نقل(ftype->types + عدد_Rets, argTypes, عدد_دلائل * حجم(m3type_t));

    *o_type = ftype;
    ftype   = تہی;

} _catch:
    m3_Free(ftype);
    واپس نتیجہ;
}


#اگر d_m3_Has_ڈھیر_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
ساکن
M3_نتیجہ ParseContType (IM3Module io_module, IM3FuncType* o_type, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    IM3FuncType ftype = تہی;
_try {
    i64 ft_اشاریہ;
_   (ReadLebSigned(&ft_اشاریہ, 33, io_بائٹیں, i_انتہا));
    _throwif(m3Err_wasmMalformed, ft_اشاریہ < 0 or (u64) ft_اشاریہ >= io_module->عدد_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_اشاریہ];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 کل = underlying->عدد_Rets + underlying->عدد_دلائل;
_   (AllocFuncType(&ftype, کل));
    ftype->عدد_Rets        = underlying->عدد_Rets;
    ftype->عدد_دلائل        = underlying->عدد_دلائل;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    اگر (کل) {
        حافظہ_نقل(ftype->types, underlying->types, کل * حجم(m3type_t));
    }

    *o_type = ftype;
    ftype   = تہی;

} _catch:
    m3_Free(ftype);
    واپس نتیجہ;
}

#اگر_ختم // d_m3HasStackSwitching


ساکن
M3_نتیجہ ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
#اگر d_m3_Has_ڈھیر_Switching
    اگر (i_form == -35) {
        واپس ParseContType(io_module, o_type, io_بائٹیں, i_انتہا);
    }
#اگر_ختم
    اگر (i_form != -32) { // for Wasm MVP
        واپس m3Err_wasmMalformed;
    }

    واپس ParseFuncType(io_module, o_type, io_بائٹیں, i_انتہا);
}


#اگر d_m3_Has_ڈھیر_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
ساکن
M3_نتیجہ SkipTypeDefinition (i8 i_form, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
_try {
    اگر (i_form == -35) {
        i64 ft_اشاریہ;
_       (ReadLebSigned(&ft_اشاریہ, 33, io_بائٹیں, i_انتہا));
        واپس نتیجہ;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    برائے (u32 half = 0; half < 2; ++half) {
        u32 گنتی;
_       (ReadLEB_u32(&گنتی, io_بائٹیں, i_انتہا));
        _throwif(m3_Err_too_Many_دلائل_Rets, گنتی > d_m3_Max_Sane_Function_Arg_Ret_گنتی);

        برائے (u32 v = 0; v < گنتی; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_بائٹیں, i_انتہا));

            اگر (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 ڈھیر_2;
_               (ReadLebSigned(&ڈھیر_2, 33, io_بائٹیں, i_انتہا));
            }
        }
    }

} _catch: واپس نتیجہ;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
ساکن
M3_نتیجہ Parse_Rec_گروہ (IM3Module io_module, u32 i_گنتی, u32 i_پہلا_اشاریہ,
                        بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    IM3FuncType* placeholders = تہی;
_try {
    بائٹیں_t گروہ_آغاز = *io_بائٹیں;

    placeholders = m3_AllocArray(IM3FuncType, i_گنتی);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    برائے (u32 j = 0; j < i_گنتی; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_بائٹیں, i_انتہا));
_       (SkipTypeDefinition(form, io_بائٹیں, i_انتہا));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 پہلا_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_گنتی, &پہلا_Canonical));

    برائے (u32 j = 0; j < i_گنتی; ++j) {
        placeholders[j]->canonical_اشاریہ        = (u16)(پہلا_Canonical + j);
        io_module->funcTypes[i_پہلا_اشاریہ + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    برائے (u32 pass = 0; pass < 2; ++pass) {
        بائٹیں_t بائٹیں = گروہ_آغاز;

        برائے (u32 j = 0; j < i_گنتی; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &بائٹیں, i_انتہا));

            اگر ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &بائٹیں, i_انتہا));
                جاری;
            }

            IM3FuncType ftype = تہی;
_           (ParseTypeDefinition(io_module, form, &ftype, &بائٹیں, i_انتہا));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_اشاریہ);

            m3_Free(placeholders[j]);
            placeholders[j]                        = تہی;
            io_module->funcTypes[i_پہلا_اشاریہ + j] = ftype;
        }

        اگر (pass == 1) {
            *io_بائٹیں = بائٹیں;
        }
    }

} _catch:
    اگر (placeholders) {
        برائے (u32 j = 0; j < i_گنتی; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    واپس نتیجہ;
}

#اگر_ختم // d_m3HasStackSwitching


M3_نتیجہ ParseSection_Type (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    IM3FuncType ftype = تہی;

_try {
    u32 عدد_Types;
_   (ReadLEB_u32(&عدد_Types, &i_بائٹیں, i_انتہا));                                      m3log (parse, "** Type [%d]", عدد_Types);

    _throwif("too many types", عدد_Types > d_m3_Max_Sane_Types_گنتی);

    اگر (عدد_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, عدد_Types);
        _throwifnull(io_module->funcTypes);
        io_module->عدد_Func_Types = عدد_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#اگر d_m3_Has_ڈھیر_Switching
        u32 استعداد = عدد_Types;
#اگر_ختم
        u32 index = 0;

        برائے (u32 i = 0; i < عدد_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_بائٹیں, i_انتہا));

#اگر d_m3_Has_ڈھیر_Switching
            اگر (form == -50) { // rec
                u32 گنتی;
_               (ReadLEB_u32(&گنتی, &i_بائٹیں, i_انتہا));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + گنتی + (عدد_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_گنتی);

                اگر (needed > استعداد) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, استعداد);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    استعداد                = grown;
                    io_module->عدد_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                اگر (گنتی == 1) {
_                   (ReadLEB_i7(&form, &i_بائٹیں, i_انتہا));
                } ورنہ {
_                   (Parse_Rec_گروہ(io_module, گنتی, index, &i_بائٹیں, i_انتہا));
                    index += گنتی;
                    جاری;
                }
            }
#اگر_ختم

_           (ParseTypeDefinition(io_module, form, &ftype, &i_بائٹیں, i_انتہا));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_جوڑو_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = تہی; // ownership transferred to environment
        }

        io_module->عدد_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

} _catch:

    اگر (نتیجہ) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->عدد_Func_Types = 0;
    }

    واپس نتیجہ;
}


M3_نتیجہ ParseSection_Function (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Functions;
_   (ReadLEB_u32(&عدد_Functions, &i_بائٹیں, i_انتہا));                                  m3log (parse, "** Function [%d]", عدد_Functions);

    _throwif("too many functions", عدد_Functions > d_m3_Max_Sane_Functions_گنتی);

_   (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_Functions));

    برائے (u32 i = 0; i < عدد_Functions; ++i) {
        u32 func_Type_اشاریہ;
_       (ReadLEB_u32(&func_Type_اشاریہ, &i_بائٹیں, i_انتہا));

_       (Module_جوڑو_Function(io_module, func_Type_اشاریہ, تہی /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch: واپس نتیجہ;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
قسمتعریف ساخت M3ImportDesc {
    u8           قسم;
    u32          type_اشاریہ;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_جدول_Info  جدول_2;
    M3_حافظہ_Info حافظہ;
} M3ImportDesc;


#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
ساکن
M3_نتیجہ Read_لیبل_Type (IM3Module i_module, u32* o_type_اشاریہ, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_بائٹیں, i_انتہا));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_اشاریہ, io_بائٹیں, i_انتہا));
    _throwif(m3Err_unknownType, not i_module or *o_type_اشاریہ >= i_module->عدد_Func_Types);

#  اگر !d_m3_Has_ڈھیر_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_عدد_Results(i_module->funcTypes[*o_type_اشاریہ]) != 0);
#  endif

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
ساکن
M3_نتیجہ ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (Read_u8(&o_desc->قسم, io_بائٹیں, i_انتہا));

    انتخاب (o_desc->قسم) {
    حالت d_external_قسم_function:
_       (ReadLEB_u32(&o_desc->type_اشاریہ, io_بائٹیں, i_انتہا));
        توڑو;

    حالت d_external_قسم_جدول:
_       (Read_Type_جدول_Type(i_module, &o_desc->جدول_2, io_بائٹیں, i_انتہا));
        توڑو;

    حالت d_external_قسم_حافظہ:
_       (Parse_Type_حافظہ(&o_desc->حافظہ, io_بائٹیں, i_انتہا));
        توڑو;

    حالت d_external_قسم_global:
_       (Parse_قدر_Type(i_module, &o_desc->type, io_بائٹیں, i_انتہا));
_       (ReadLEB_u7(&o_desc->isMutable, io_بائٹیں, i_انتہا));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [بنیاد_Type_کا(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        توڑو;

#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    حالت d_external_قسم_لیبل:
_       (Read_لیبل_Type(i_module, &o_desc->type_اشاریہ, io_بائٹیں, i_انتہا));
        توڑو;
#اگر_ختم

    طےشدہ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: واپس نتیجہ;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
ساکن
M3_نتیجہ ApplyImportDesc (IM3Module io_module, ثابت M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3ImportInfo clearImport = { تہی, تہی };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->قسم, io_import->moduleUtf8, io_import->میدان_Utf8);
    انتخاب (i_desc->قسم) {
    حالت d_external_قسم_function: {
_       (Module_جوڑو_Function(io_module, i_desc->type_اشاریہ, io_import))
        *io_import = clearImport;

        io_module->عدد_Func_Imports++;
    } توڑو;

    حالت d_external_قسم_جدول: {
        IM3_جدول جدول_2;
_       (Module_جوڑو_جدول(io_module, &جدول_2, &i_desc->جدول_2, true /* isImport */));
        جدول_2->import = *io_import;
        *io_import    = clearImport;
    } توڑو;

    حالت d_external_قسم_حافظہ: {
        IM3_حافظہ حافظہ;

#اگر d_m3_Has_Multi_حافظہ
        _throwif(m3_Err_too_Many_حافظہ_Sections, io_module->عدد_Memories >= d_m3_Max_Sane_Memories_گنتی);
#ورنہ
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_حافظہ_Sections, io_module->عدد_Memories >= 1);
#اگر_ختم

_       (Module_جوڑو_حافظہ(io_module, &حافظہ, &i_desc->حافظہ, true /* isImport */));
        حافظہ->import = *io_import;
        *io_import     = clearImport;
    } توڑو;

    حالت d_external_قسم_global: {
        IM3Global global;
_       (Module_جوڑو_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } توڑو;

#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    حالت d_external_قسم_لیبل: {
        IM3_لیبل لیبل;
_       (Module_جوڑو_لیبل(io_module, &لیبل, io_module->funcTypes[i_desc->type_اشاریہ], true /* isImport */));
        لیبل->import = *io_import;
        *io_import  = clearImport;
    } توڑو;
#اگر_ختم

    طےشدہ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: واپس نتیجہ;
}


#اگر d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
ساکن
bool Is_خالی_خانے_Name (بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    u32 لمبائی;
    واپس (not ReadLEB_u32(&لمبائی, &i_بائٹیں, i_انتہا)) and لمبائی == 0;
}

#اگر_ختم // d_m3HasCompactImports


#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching

M3_نتیجہ Parse_Section_لیبل (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_لیبل_فہرست;
_   (ReadLEB_u32(&عدد_لیبل_فہرست, &i_بائٹیں, i_انتہا));                                       m3log (parse, "** Tag [%d]", عدد_لیبل_فہرست);

    _throwif("too many tags", عدد_لیبل_فہرست > d_m3_Max_Sane_لیبل_فہرست_گنتی);

    برائے (u32 i = 0; i < عدد_لیبل_فہرست; ++i) {
        u32 type_اشاریہ;
_       (Read_لیبل_Type(io_module, &type_اشاریہ, &i_بائٹیں, i_انتہا));                      m3log (parse, "    tag: [%d] type: %d", i, type_اشاریہ);

_       (Module_جوڑو_لیبل(io_module, تہی, io_module->funcTypes[type_اشاریہ], false));
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch: واپس نتیجہ;
}

#اگر_ختم // d_m3HasExceptionHandling


M3_نتیجہ ParseSection_Import (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    M3ImportInfo import = { تہی, تہی };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 عدد_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 عدد_Entries;
_   (ReadLEB_u32(&عدد_Entries, &i_بائٹیں, i_انتہا));                                    m3log (parse, "** Import [%d]", عدد_Entries);

    _throwif("too many imports", عدد_Entries > d_m3_Max_Sane_Imports_گنتی);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_Entries));

    برائے (u32 i = 0; i < عدد_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_بائٹیں, i_انتہا));

#اگر d_m3HasCompactImports
        بائٹیں_t میدان_آغاز = i_بائٹیں;
#اگر_ختم
_       (Read_utf8(&import.میدان_Utf8, &i_بائٹیں, i_انتہا));

#اگر d_m3HasCompactImports
        u8 compact = (i_بائٹیں < i_انتہا) ? *i_بائٹیں : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        اگر ((compact == d_compact_Imports_فی_شے_Type or compact == d_compactImports_sharedType) and
            Is_خالی_خانے_Name(میدان_آغاز, i_انتہا)) {
            ++i_بائٹیں;

            m3_Free(import.میدان_Utf8);         // it only marked the compact form
            import.میدان_Utf8 = تہی;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            اگر (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_بائٹیں, i_انتہا));
            }

            u32 عدد_اشیاء;
_           (ReadLEB_u32(&عدد_اشیاء, &i_بائٹیں, i_انتہا));                              m3log (parse, "    compact: %d import(s) of '%s'", عدد_اشیاء, import.moduleUtf8);

            _throwif("too many imports", (u64)عدد_Imports + عدد_اشیاء > d_m3_Max_Sane_Imports_گنتی);
            عدد_Imports += عدد_اشیاء;

_           (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_اشیاء));

            برائے (u32 j = 0; j < عدد_اشیاء; ++j) {
                M3ImportInfo شے = { تہی, تہی };

                شے.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, لڑی_لمبائی(import.moduleUtf8) + 1);

                اگر (شے.moduleUtf8) {
                    نتیجہ = Read_utf8(&شے.میدان_Utf8, &i_بائٹیں, i_انتہا);

                    // the shared form reuses the externtype read above
                    اگر (not نتیجہ and compact == d_compact_Imports_فی_شے_Type) {
                        نتیجہ = ReadImportDesc(io_module, &desc, &i_بائٹیں, i_انتہا);
                    }

                    اگر (not نتیجہ) {
                        نتیجہ = ApplyImportDesc(io_module, &desc, &شے);
                    }
                } ورنہ {
                    نتیجہ = m3Err_mallocFailed;
                }

                FreeImportInfo(&شے);
                _throwif(نتیجہ, نتیجہ);
            }

            FreeImportInfo(&import);
            جاری;
        }
#اگر_ختم // d_m3HasCompactImports

        _throwif("too many imports", عدد_Imports >= d_m3_Max_Sane_Imports_گنتی);
        ++عدد_Imports;

_       (ReadImportDesc(io_module, &desc, &i_بائٹیں, i_انتہا));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    واپس نتیجہ;
}


M3_نتیجہ ParseSection_Export (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ    نتیجہ = m3Err_none;
    ثابت حرف* utf8   = تہی;

    u32 عدد_Exports;
_   (ReadLEB_u32(&عدد_Exports, &i_بائٹیں, i_انتہا));                                    m3log (parse, "** Export [%d]", عدد_Exports);

    _throwif("too many exports", عدد_Exports > d_m3_Max_Sane_Exports_گنتی);

    // the section order check lets this section through once, so this is
    // every export the module has
    اگر (عدد_Exports) {
        io_module->exports = m3_AllocArray(M3Export, عدد_Exports);
        _throwifnull(io_module->exports);
    }

    برائے (u32 i = 0; i < عدد_Exports; ++i) {
        u8  export_قسم;
        u32 index;
        u32 name_لمبائی;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            بائٹیں_t لمبائی_بائٹیں = i_بائٹیں;
_           (ReadLEB_u32(&name_لمبائی, &لمبائی_بائٹیں, i_انتہا));
        }

_       (Read_utf8(&utf8, &i_بائٹیں, i_انتہا));
_       (Read_u8(&export_قسم, &i_بائٹیں, i_انتہا));
_       (ReadLEB_u32(&index, &i_بائٹیں, i_انتہا));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_قسم, utf8);

#اگر d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        برائے (u32 j = 0; j < i; ++j) {
            ثابت M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_لمبائی == name_لمبائی and memcmp(other->name, utf8, name_لمبائی) == 0);
        }
#اگر_ختم

        اگر (export_قسم == d_external_قسم_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Functions);
            IM3Function func = &(io_module->functions[index]);
            اگر (func->عدد_نام < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_لمبائی + 1);
                _throwifnull(name);
                func->نام[func->عدد_نام++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } ورنہ اگر (export_قسم == d_external_قسم_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Globals);
        } ورنہ اگر (export_قسم == d_external_قسم_حافظہ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Memories);
        } ورنہ اگر (export_قسم == d_external_قسم_جدول) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Tables);
        }
#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
        ورنہ اگر (export_قسم == d_external_قسم_لیبل) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_لیبل_فہرست);
        }
#اگر_ختم

        M3Export* اندراج_2   = &io_module->exports[io_module->عدد_Exports++];
        اندراج_2->name       = utf8;
        اندراج_2->name_لمبائی = name_لمبائی;
        اندراج_2->index      = index;
        اندراج_2->قسم       = export_قسم;

        utf8 = تہی; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

_catch:
    m3_Free(utf8);
    واپس نتیجہ;
}


M3_نتیجہ Parse_Section_آغاز (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 آغاز_Func_اشاریہ;
_   (ReadLEB_u32(&آغاز_Func_اشاریہ, &i_بائٹیں, i_انتہا));                                  m3log (parse, "** Start Function: %d", آغاز_Func_اشاریہ);

    اگر (آغاز_Func_اشاریہ < io_module->عدد_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[آغاز_Func_اشاریہ];
        اگر (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->عدد_دلائل != 0 || func->funcType->عدد_Rets != 0);
        }

        io_module->آغاز_Function = آغاز_Func_اشاریہ;
    } ورنہ {
        نتیجہ = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch: واپس نتیجہ;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_نتیجہ Parse_Init_اظہاریہ_Typed (M3Module* io_module, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا, m3type_t* o_type)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#اگر defined(d_m3PreferStaticAlloc)
    ساکن M3Compilation compilation;
#ورنہ
    M3Compilation compilation;
#اگر_ختم
    compilation = (M3Compilation){ .runtime = تہی, .module = io_module, .wasm = *io_بائٹیں, .wasm_انتہا = i_انتہا, .is_Init_اظہاریہ = true };

    نتیجہ = Compile_بلاک_Statements(&compilation);

    *io_بائٹیں = compilation.wasm;

    اگر (o_type) {
        *o_type = compilation.ڈھیر_اشاریہ ? compilation.type_ڈھیر[compilation.ڈھیر_اشاریہ - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    واپس نتیجہ;
}


M3_نتیجہ Parse_Init_اظہاریہ (M3Module* io_module, بائٹیں_t* io_بائٹیں, cbytes_t i_انتہا)
{
    واپس Parse_Init_اظہاریہ_Typed(io_module, io_بائٹیں, i_انتہا, تہی);
}


M3_نتیجہ ParseSection_Element (IM3Module io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32     عدد_Segments;
    بائٹیں_t pos;
_   (ReadLEB_u32(&عدد_Segments, &i_بائٹیں, i_انتہا));                            m3log (parse, "** Element [%d]", عدد_Segments);

    _throwif("too many element segments", عدد_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, عدد_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->عدد_Element_Segments = عدد_Segments;
    io_module->element_Section_انتہا  = i_انتہا;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_بائٹیں;
    برائے (u32 i = 0; i < عدد_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_انتہا));
#اگر d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#ورنہ
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_فعال);
#اگر_ختم

        segment->منوال   = flags & 0x3;
        segment->is_اظہاریہ = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        اگر (segment->منوال == c_m3_Elem_فعال_Idx) {
_           (ReadLEB_u32(&segment->جدول_اشاریہ, &pos, i_انتہا));
        }

        bool is_فعال = (segment->منوال == c_m3_Elem_فعال or segment->منوال == c_m3_Elem_فعال_Idx);

        اگر (is_فعال) {
            _throwif(m3Err_wasmMalformed, segment->جدول_اشاریہ >= io_module->عدد_Tables);

            segment->init_اظہاریہ = pos;
_           (Parse_Init_اظہاریہ(io_module, &pos, i_انتہا));
            segment->init_اظہاریہ_سائز = (u32)(pos - segment->init_اظہاریہ);
            _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, segment->init_اظہاریہ_سائز <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        اگر (segment->منوال != c_m3_Elem_فعال) {
            اگر (segment->is_اظہاریہ) {
_               (Parse_قدر_Type(io_module, &segment->type, &pos, i_انتہا));
                _throwif(m3Err_wasmMalformed, not IsRefType(بنیاد_Type_کا(segment->type)));
            } ورنہ {
                u8 elem_قسم;
_               (Read_u8(&elem_قسم, &pos, i_انتہا));
                _throwif(m3Err_wasmMalformed, elem_قسم != 0x00);   // funcref
            }
        }

        اگر (is_فعال) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->جدول_اشاریہ]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->عدد_Elements, &pos, i_انتہا));
        _throwif("table overflow", segment->عدد_Elements > d_m3_Max_Sane_جدول_سائز);

        segment->elements = pos;

        برائے (u32 e = 0; e < segment->عدد_Elements; ++e) {
            اگر (segment->is_اظہاریہ) {
_               (Parse_Init_اظہاریہ(io_module, &pos, i_انتہا))
            } ورنہ {
                u32 func_اشاریہ;
_               (ReadLEB_u32(&func_اشاریہ, &pos, i_انتہا));
                _throwif("function index out of range", func_اشاریہ >= io_module->عدد_Functions);
_               (Module_DeclareFunction(io_module, func_اشاریہ));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_انتہا);           // section size mismatch

    _catch: واپس نتیجہ;
}


M3_نتیجہ Parse_Section_رمز (M3Module* io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ;

    u32 عدد_Functions;
_   (ReadLEB_u32(&عدد_Functions, &i_بائٹیں, i_انتہا));                                  m3log (parse, "** Code [%d]", عدد_Functions);

    اگر (عدد_Functions != io_module->عدد_Functions - io_module->عدد_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    برائے (u32 f = 0; f < عدد_Functions; ++f) {
        ثابت u8* آغاز = i_بائٹیں;

        u32 سائز;
_       (ReadLEB_u32(&سائز, &i_بائٹیں, i_انتہا));

        اگر (سائز) {
            i_بائٹیں += سائز;

            اگر (i_بائٹیں <= i_انتہا) {
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
                func->wasm    = آغاز;
                func->wasm_انتہا = i_بائٹیں;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } ورنہ {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    اگر (not نتیجہ and i_بائٹیں != i_انتہا) {
        نتیجہ = m3Err_wasmSectionUnderrun;
    }

    واپس نتیجہ;
}


M3_نتیجہ Parse_Section_ڈیٹا (M3Module* io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_ڈیٹا_Segments;
_   (ReadLEB_u32(&عدد_ڈیٹا_Segments, &i_بائٹیں, i_انتہا));                               m3log (parse, "** Data [%d]", عدد_ڈیٹا_Segments);

    _throwif("too many data segments", عدد_ڈیٹا_Segments > d_m3_Max_Sane_ڈیٹا_Segments);

    io_module->ڈیٹا_Segments = m3_AllocArray(M3_ڈیٹا_Segment, عدد_ڈیٹا_Segments);
    _throwifnull(io_module->ڈیٹا_Segments);
    io_module->عدد_ڈیٹا_Segments = عدد_ڈیٹا_Segments;

    برائے (u32 i = 0; i < عدد_ڈیٹا_Segments; ++i) {
        M3_ڈیٹا_Segment* segment = &io_module->ڈیٹا_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_بائٹیں, i_انتہا));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        اگر (flags == 2) {
_           (ReadLEB_u32(&segment->حافظہ_Region, &i_بائٹیں, i_انتہا));
        }

        اگر (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#اگر !d_m3_Has_Multi_حافظہ
            _throwif(m3Err_wasmMalformed, segment->حافظہ_Region != 0);
#اگر_ختم
            _throwif(m3Err_wasmMalformed, segment->حافظہ_Region >= io_module->عدد_Memories);

            segment->init_اظہاریہ = i_بائٹیں;
_           (Parse_Init_اظہاریہ(io_module, &i_بائٹیں, i_انتہا));
            segment->init_اظہاریہ_سائز = (u32)(i_بائٹیں - segment->init_اظہاریہ);

            _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, segment->init_اظہاریہ_سائز <= 1);
        }

_       (ReadLEB_u32(&segment->سائز, &i_بائٹیں, i_انتہا));
        segment->ڈیٹا = i_بائٹیں;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->حافظہ_Region, segment->init_اظہاریہ_سائز, segment->سائز);
        i_بائٹیں += segment->سائز;

        _throwif("data segment underflow", i_بائٹیں > i_انتہا);
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch:

    واپس نتیجہ;
}


M3_نتیجہ Parse_Section_ڈیٹا_گنتی (M3Module* io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

_   (ReadLEB_u32(&io_module->ڈیٹا_گنتی, &i_بائٹیں, i_انتہا));                       m3log (parse, "** DataCount [%d]", io_module->ڈیٹا_گنتی);

    io_module->has_ڈیٹا_گنتی = true;

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch: واپس نتیجہ;
}


M3_نتیجہ Parse_Section_حافظہ (M3Module* io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Memories;
_   (ReadLEB_u32(&عدد_Memories, &i_بائٹیں, i_انتہا));                                m3log (parse, "** Memory [%d]", عدد_Memories);

#اگر d_m3_Has_Multi_حافظہ
    _throwif(m3_Err_too_Many_حافظہ_Sections,
             (u64)عدد_Memories + io_module->عدد_Memories > d_m3_Max_Sane_Memories_گنتی);
#ورنہ
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_حافظہ_Sections, عدد_Memories > 1);
    _throwif(m3_Err_too_Many_حافظہ_Sections, (u64)عدد_Memories + io_module->عدد_Memories > 1);
#اگر_ختم

    برائے (u32 i = 0; i < عدد_Memories; ++i) {
        M3_حافظہ_Info info;
_       (Parse_Type_حافظہ(&info, &i_بائٹیں, i_انتہا));
_       (Module_جوڑو_حافظہ(io_module, تہی, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch: واپس نتیجہ;
}


M3_نتیجہ ParseSection_Global (M3Module* io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    u32 عدد_Globals;
_   (ReadLEB_u32(&عدد_Globals, &i_بائٹیں, i_انتہا));                                    m3log (parse, "** Global [%d]", عدد_Globals);

    _throwif("too many globals", عدد_Globals > d_m3_Max_Sane_Globals_گنتی);

    برائے (u32 i = 0; i < عدد_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_قدر_Type(io_module, &type, &i_بائٹیں, i_انتہا));
_       (ReadLEB_u7(&isMutable, &i_بائٹیں, i_انتہا));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [بنیاد_Type_کا(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_جوڑو_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_اظہاریہ = i_بائٹیں;
_       (Parse_Init_اظہاریہ(io_module, &i_بائٹیں, i_انتہا));
        global->init_اظہاریہ_سائز = (u32)(i_بائٹیں - global->init_اظہاریہ);

        _throwif(m3_Err_wasm_Missing_Init_اظہاریہ, global->init_اظہاریہ_سائز <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_بائٹیں != i_انتہا);      // section size mismatch

    _catch: واپس نتیجہ;
}


M3_نتیجہ ParseSection_Name (M3Module* io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    cstr_t name = تہی;

    جب (i_بائٹیں < i_انتہا) {
        u8  nameType;
        u32 payload_لمبائی;

_       (ReadLEB_u7(&nameType, &i_بائٹیں, i_انتہا));
_       (ReadLEB_u32(&payload_لمبائی, &i_بائٹیں, i_انتہا));

        بائٹیں_t آغاز = i_بائٹیں;
        _throwif(m3Err_wasmSectionOverrun, payload_لمبائی > (size_t)(i_انتہا - آغاز));

        اگر (nameType == 1) {
            u32 عدد_نام;
_           (ReadLEB_u32(&عدد_نام, &i_بائٹیں, i_انتہا));

            _throwif("too many names", عدد_نام > d_m3_Max_Sane_Functions_گنتی);

            برائے (u32 i = 0; i < عدد_نام; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_بائٹیں, i_انتہا));
_               (Read_utf8(&name, &i_بائٹیں, i_انتہا));

                اگر (index < io_module->عدد_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    اگر (func->عدد_نام == 0) {
                        func->نام[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->عدد_نام = 1;

                        name = تہی; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_بائٹیں = آغاز + payload_لمبائی;
    }

    _catch: واپس نتیجہ;
}


M3_نتیجہ ParseSection_Custom (M3Module* io_module, بائٹیں_t i_بائٹیں, cbytes_t i_انتہا)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    cstr_t name = تہی;
_   (Read_utf8(&name, &i_بائٹیں, i_انتہا));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    اگر (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_بائٹیں, i_انتہا));
#اگر d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } ورنہ اگر (strcmp(name, "snapshot") == 0 || (لڑی_لمبائی(name) > 9 && لڑی_موازنہ(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, لڑی_لمبائی(snapName) + 1);
        اگر (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->ڈیٹا           = i_بائٹیں;
        snap->سائز           = (u32)(i_انتہا - i_بائٹیں);
        snap->اگلا           = io_module->snapshots;
        io_module->snapshots = snap;
#اگر_ختم
    } ورنہ اگر (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_بائٹیں, i_انتہا));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    واپس نتیجہ;
}


M3_نتیجہ ParseModuleSection (M3Module* o_module, u8 i_sectionType, بائٹیں_t i_بائٹیں, u32 i_عدد_بائٹیں)
{
    M3_نتیجہ نتیجہ = m3Err_none;

    قسمتعریف M3_نتیجہ (*M3Parser)(M3Module*, بائٹیں_t, cbytes_t);

    ساکن M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_جدول,        // 4
        Parse_Section_حافظہ,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_آغاز,     // 8
        ParseSection_Element,   // 9
        Parse_Section_رمز,      // 10
        Parse_Section_ڈیٹا,      // 11
        Parse_Section_ڈیٹا_گنتی, // 12
#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
        Parse_Section_لیبل,       // 13
#اگر_ختم
    };

    M3Parser parser = تہی;

    اگر (i_sectionType < M3_گنتی_کا(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    اگر (parser) {
        cbytes_t انتہا = i_بائٹیں + i_عدد_بائٹیں;
        نتیجہ       = parser(o_module, i_بائٹیں, انتہا);
    } ورنہ {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    واپس نتیجہ;
}


M3_نتیجہ m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_بائٹیں, u32 i_عدد_بائٹیں)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_عدد_بائٹیں);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_عدد_بائٹیں);
    module->آغاز_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ثابت u8* pos = i_بائٹیں;
    ثابت u8* انتہا = pos + i_عدد_بائٹیں;

    module->wasm_آغاز = pos;
    module->wasm_انتہا   = انتہا;

    u32 magic, نسخہ;
_   (Read_u32(&magic, &pos, انتہا));
_   (Read_u32(&نسخہ, &pos, انتہا));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_نسخہ, نسخہ != 1);

#اگر d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
    // the tag section sits between memory (5) and global (6)
    ساکن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#ورنہ
    ساکن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#اگر_ختم
    u8 expectedSection = 0;

    جب (pos < انتہا) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, انتہا));

        اگر (section != 0) {
            // Ensure sections appear only once and in order
            جب (sections_ترتیب[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_گنتی_کا(sections_ترتیب) - 1);
            }
        }

        u32 section_لمبائی;
_       (ReadLEB_u32(&section_لمبائی, &pos, انتہا));
        _throwif(m3Err_wasmMalformed, pos + section_لمبائی > انتہا);

_       (ParseModuleSection(module, section, pos, section_لمبائی));

        pos += section_لمبائی;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    اگر (module->عدد_Functions > module->عدد_Func_Imports) {
        IM3Function پہلا_Non_Import = &module->functions[module->عدد_Func_Imports];
        _throwif(m3Err_wasmMalformed, پہلا_Non_Import->wasm == تہی);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_ڈیٹا_گنتی and module->ڈیٹا_گنتی != module->عدد_ڈیٹا_Segments);

} _catch:

    اگر (نتیجہ) {
        m3_FreeModule(module);
        module = تہی;
    }

    *o_module = module;

    واپس نتیجہ;
}
