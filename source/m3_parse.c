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
M3_پایله Parse_ډېرۍ_Type (IM3Module i_module, m3type_t* o_ډېرۍ_بټونه, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    i64 ډېرۍ;
_   (ReadLebSigned(&ډېرۍ, 33, io_بایټونه, i_پای_ټکی));

    إذا (ډېرۍ == -d_waType_funcref) {
        *o_ډېرۍ_بټونه = d_m3_Type_ډېرۍ_Abstract;
    } وإلا إذا (ډېرۍ == -d_waType_externref) {
        *o_ډېرۍ_بټونه = d_m3Type_refExtern | d_m3_Type_ډېرۍ_Abstract;
#  إذا d_m3_Has_ډېری_Switching
    } وإلا إذا (ډېرۍ == -d_wa_Type_ډېرۍ_cont) {
        *o_ډېرۍ_بټونه = d_m3Type_refCont | d_m3_Type_ډېرۍ_Abstract;
    } وإلا إذا (ډېرۍ == -d_wa_Type_ډېرۍ_nocont) {
        *o_ډېرۍ_بټونه = d_m3Type_refCont | d_m3_Type_ډېرۍ_None;
#  endif
    } وإلا إذا (ډېرۍ >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) ډېرۍ >= i_module->عدد_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[ډېرۍ];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_ډېرۍ_بټونه = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_شاخص;
    } وإلا {
        _throw(m3Err_invalidTypeId);
    }

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_پایله Parse_ارزښت_Type (IM3Module i_module, m3type_t* o_type, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_بایټونه >= i_پای_ټکی);

#إذا d_m3HasTypedRefs
    إذا (**io_بایټونه == d_waEncode_ref or **io_بایټونه == d_waEncode_refNull) {
        u8 lead = *(*io_بایټونه)++;

        m3type_t ډېرۍ_بټونه = d_m3_Type_ډېرۍ_Abstract;
_       (Parse_ډېرۍ_Type(i_module, &ډېرۍ_بټونه, io_بایټونه, i_پای_ټکی));

        *o_type = d_m3Type_ref | ډېرۍ_بټونه | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        أرجع پایله;
    }
#د_که_پای

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_بایټونه, i_پای_ټکی));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: أرجع پایله;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
ساكن
M3_پایله Read_Type_جدول_Type (IM3Module i_module, M3_جدول_Info* o_info, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_اندازه = 0, max_اندازه = 0;

    // the width of the limits, and of every index into the table
    u32 addr_بټونه = 32;

_   (Parse_ارزښت_Type(i_module, &elemType, io_بایټونه, i_پای_ټکی));
#إذا d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(بنسټ_Type_د(elemType)));
#وإلا
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#د_که_پای

_   (ReadLEB_u7(&flag, io_بایټونه, i_پای_ټکی));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#إذا d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#وإلا
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#د_که_پای

    إذا (flag & 0x04u) {
        addr_بټونه = 64;
    }

_   (ReadLebUnsigned(&init_اندازه, addr_بټونه, io_بایټونه, i_پای_ټکی));
    _throwif("table overflow", init_اندازه > d_m3_Max_Sane_جدول_اندازه);

    إذا (flag & 0x01u) {
_       (ReadLebUnsigned(&max_اندازه, addr_بټونه, io_بایټونه, i_پای_ټکی));
        _throwif(m3Err_wasmMalformed, max_اندازه < init_اندازه);
        _throwif("table overflow", max_اندازه > d_m3_Max_Sane_جدول_اندازه);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_اندازه  = (u32)init_اندازه;
    o_info->max_اندازه   = (u32)max_اندازه;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    أرجع پایله;
}


ساكن
M3_پایله Parse_Type_جدول_Type (IM3Module io_module, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    M3_جدول_Info info;

_   (Read_Type_جدول_Type(io_module, &info, io_بایټونه, i_پای_ټکی));
_   (Module_جمع_کړه_جدول(io_module, عدم, &info, false /* isImport */));

    _catch: أرجع پایله;
}


M3_پایله Parse_Init_بیان_Typed (M3Module* io_module, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی, m3type_t* o_type);

M3_پایله Parse_Type_جدول (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32 عدد_Tables;
_   (ReadLEB_u32(&عدد_Tables, &i_بایټونه, i_پای_ټکی));                          m3log (parse, "** Table [%d]", عدد_Tables);

    _throwif("too many tables", (u64)عدد_Tables + io_module->عدد_Tables > d_m3_Max_Sane_جدول_شمېر);

    لكل (u32 i = 0; i < عدد_Tables; ++i) {
#إذا d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_بیان = (i_بایټونه < i_پای_ټکی) and (*i_بایټونه == 0x40);
        u32  جدول_شاخص  = io_module->عدد_Tables;

        إذا (has_Init_بیان) {
            u8 reserved;
            ++i_بایټونه;
_           (Read_u8(&reserved, &i_بایټونه, i_پای_ټکی));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_جدول_Type(io_module, &i_بایټونه, i_پای_ټکی));

        إذا (has_Init_بیان) {
            IM3_جدول جدول_2 = io_module->tables[جدول_شاخص];

            m3type_t initType;

            جدول_2->init_بیان = i_بایټونه;
_           (Parse_Init_بیان_Typed(io_module, &i_بایټونه, i_پای_ټکی, &initType));
            جدول_2->init_بیان_اندازه = (u32)(i_بایټونه - جدول_2->init_بیان);
            _throwif(m3_Err_wasm_Missing_Init_بیان, جدول_2->init_بیان_اندازه <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_د(initType, جدول_2->type));
        } وإلا {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[جدول_شاخص]->type));
        }
#وإلا
_       (Parse_Type_جدول_Type(io_module, &i_بایټونه, i_پای_ټکی));
#د_که_پای
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch: أرجع پایله;
}


M3_پایله Parse_Type_حافظه (M3_حافظه_Info* o_حافظه, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_پاڼه_اندازه = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_بټونه = 32;

_   (ReadLEB_u7(&flag, io_بایټونه, i_پای_ټکی));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 معلوم = 0x09u;
#إذا d_m3HasAtomics
        معلوم |= 0x02u;
#د_که_پای
#إذا d_m3HasMemory64
        معلوم |= 0x04u;
#د_که_پای
        _throwif(m3Err_wasmMalformed, flag & ~معلوم);
    }

    o_حافظه->isShared = (flag & (1u << 1)) != 0;

    o_حافظه->isMemory64 = (flag & (1u << 2)) != 0;
    إذا (o_حافظه->isMemory64) {
        addr_بټونه = 64;
    }

_   (ReadLebUnsigned(&o_حافظه->init_پاڼې, addr_بټونه, io_بایټونه, i_پای_ټکی));

    o_حافظه->max_پاڼې = 0;
    o_حافظه->hasMax   = (flag & (1u << 0)) != 0;
    إذا (o_حافظه->hasMax) {
_       (ReadLebUnsigned(&o_حافظه->max_پاڼې, addr_بټونه, io_بایټونه, i_پای_ټکی));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_حافظه->max_پاڼې < o_حافظه->init_پاڼې);
    }

    _throwif("shared memory must have maximum", o_حافظه->isShared and not o_حافظه->hasMax);

    o_حافظه->پاڼه_اندازه = 0;
    إذا (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_پاڼه_اندازه, io_بایټونه, i_پای_ټکی));

        _throwif("invalid custom page size", log_پاڼه_اندازه > 16);

        o_حافظه->پاڼه_اندازه = 1u << log_پاڼه_اندازه;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_پاڼې_Allowed = (log_پاڼه_اندازه or addr_بټونه < 64)
                                ? (UINT64_C(1) << (addr_بټونه - log_پاڼه_اندازه))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_حافظه->init_پاڼې > max_پاڼې_Allowed);
        إذا (o_حافظه->hasMax) {
            _throwif(m3Err_wasmMalformed, o_حافظه->max_پاڼې > max_پاڼې_Allowed);
        }
    }

    _catch: أرجع پایله;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
ساكن
M3_پایله ParseFuncType (IM3Module io_module, IM3FuncType* o_type, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    IM3FuncType ftype = عدم;
_try {
    u32 عدد_دلیلونه;
_   (ReadLEB_u32(&عدد_دلیلونه, io_بایټونه, i_پای_ټکی));

    _throwif(m3_Err_too_Many_دلیلونه_Rets, عدد_دلیلونه > d_m3_Max_Sane_Function_Arg_Ret_شمېر);
#إذا M3_HAS_VLA
    m3type_t argTypes[عدد_دلیلونه + 1]; // make ubsan happy
#وإلا
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_شمېر];
#د_که_پای
    لكل (u32 a = 0; a < عدد_دلیلونه; ++a) {
_       (Parse_ارزښت_Type(io_module, &argTypes[a], io_بایټونه, i_پای_ټکی));
    }

    u32 عدد_Rets;
_   (ReadLEB_u32(&عدد_Rets, io_بایټونه, i_پای_ټکی));
    _throwif(m3_Err_too_Many_دلیلونه_Rets, (u64)(عدد_Rets) + عدد_دلیلونه > d_m3_Max_Sane_Function_Arg_Ret_شمېر);

_   (AllocFuncType(&ftype, عدد_Rets + عدد_دلیلونه));
    ftype->عدد_دلیلونه = عدد_دلیلونه;
    ftype->عدد_Rets = عدد_Rets;

    لكل (u32 r = 0; r < عدد_Rets; ++r) {
_       (Parse_ارزښت_Type(io_module, &ftype->types[r], io_بایټونه, i_پای_ټکی));
    }
    حافظه_نقل(ftype->types + عدد_Rets, argTypes, عدد_دلیلونه * حجم(m3type_t));

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع پایله;
}


#إذا d_m3_Has_ډېری_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
ساكن
M3_پایله ParseContType (IM3Module io_module, IM3FuncType* o_type, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    IM3FuncType ftype = عدم;
_try {
    i64 ft_شاخص;
_   (ReadLebSigned(&ft_شاخص, 33, io_بایټونه, i_پای_ټکی));
    _throwif(m3Err_wasmMalformed, ft_شاخص < 0 or (u64) ft_شاخص >= io_module->عدد_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_شاخص];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 ټول = underlying->عدد_Rets + underlying->عدد_دلیلونه;
_   (AllocFuncType(&ftype, ټول));
    ftype->عدد_Rets        = underlying->عدد_Rets;
    ftype->عدد_دلیلونه        = underlying->عدد_دلیلونه;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    إذا (ټول) {
        حافظه_نقل(ftype->types, underlying->types, ټول * حجم(m3type_t));
    }

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع پایله;
}

#د_که_پای // d_m3HasStackSwitching


ساكن
M3_پایله ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
#إذا d_m3_Has_ډېری_Switching
    إذا (i_form == -35) {
        أرجع ParseContType(io_module, o_type, io_بایټونه, i_پای_ټکی);
    }
#د_که_پای
    إذا (i_form != -32) { // for Wasm MVP
        أرجع m3Err_wasmMalformed;
    }

    أرجع ParseFuncType(io_module, o_type, io_بایټونه, i_پای_ټکی);
}


#إذا d_m3_Has_ډېری_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
ساكن
M3_پایله SkipTypeDefinition (i8 i_form, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
_try {
    إذا (i_form == -35) {
        i64 ft_شاخص;
_       (ReadLebSigned(&ft_شاخص, 33, io_بایټونه, i_پای_ټکی));
        أرجع پایله;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    لكل (u32 half = 0; half < 2; ++half) {
        u32 شمېر;
_       (ReadLEB_u32(&شمېر, io_بایټونه, i_پای_ټکی));
        _throwif(m3_Err_too_Many_دلیلونه_Rets, شمېر > d_m3_Max_Sane_Function_Arg_Ret_شمېر);

        لكل (u32 v = 0; v < شمېر; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_بایټونه, i_پای_ټکی));

            إذا (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 ډېرۍ;
_               (ReadLebSigned(&ډېرۍ, 33, io_بایټونه, i_پای_ټکی));
            }
        }
    }

} _catch: أرجع پایله;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
ساكن
M3_پایله Parse_Rec_ډله (IM3Module io_module, u32 i_شمېر, u32 i_لومړی_شاخص,
                        بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    IM3FuncType* placeholders = عدم;
_try {
    بایټونه_t ډله_پیل = *io_بایټونه;

    placeholders = m3_AllocArray(IM3FuncType, i_شمېر);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    لكل (u32 j = 0; j < i_شمېر; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_بایټونه, i_پای_ټکی));
_       (SkipTypeDefinition(form, io_بایټونه, i_پای_ټکی));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 لومړی_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_شمېر, &لومړی_Canonical));

    لكل (u32 j = 0; j < i_شمېر; ++j) {
        placeholders[j]->canonical_شاخص        = (u16)(لومړی_Canonical + j);
        io_module->funcTypes[i_لومړی_شاخص + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    لكل (u32 pass = 0; pass < 2; ++pass) {
        بایټونه_t بایټونه = ډله_پیل;

        لكل (u32 j = 0; j < i_شمېر; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &بایټونه, i_پای_ټکی));

            إذا ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &بایټونه, i_پای_ټکی));
                استمر;
            }

            IM3FuncType ftype = عدم;
_           (ParseTypeDefinition(io_module, form, &ftype, &بایټونه, i_پای_ټکی));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_شاخص);

            m3_Free(placeholders[j]);
            placeholders[j]                        = عدم;
            io_module->funcTypes[i_لومړی_شاخص + j] = ftype;
        }

        إذا (pass == 1) {
            *io_بایټونه = بایټونه;
        }
    }

} _catch:
    إذا (placeholders) {
        لكل (u32 j = 0; j < i_شمېر; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    أرجع پایله;
}

#د_که_پای // d_m3HasStackSwitching


M3_پایله ParseSection_Type (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    IM3FuncType ftype = عدم;

_try {
    u32 عدد_Types;
_   (ReadLEB_u32(&عدد_Types, &i_بایټونه, i_پای_ټکی));                                      m3log (parse, "** Type [%d]", عدد_Types);

    _throwif("too many types", عدد_Types > d_m3_Max_Sane_Types_شمېر);

    إذا (عدد_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, عدد_Types);
        _throwifnull(io_module->funcTypes);
        io_module->عدد_Func_Types = عدد_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#إذا d_m3_Has_ډېری_Switching
        u32 ظرفیت = عدد_Types;
#د_که_پای
        u32 index = 0;

        لكل (u32 i = 0; i < عدد_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_بایټونه, i_پای_ټکی));

#إذا d_m3_Has_ډېری_Switching
            إذا (form == -50) { // rec
                u32 شمېر;
_               (ReadLEB_u32(&شمېر, &i_بایټونه, i_پای_ټکی));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + شمېر + (عدد_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_شمېر);

                إذا (needed > ظرفیت) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, ظرفیت);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    ظرفیت                = grown;
                    io_module->عدد_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                إذا (شمېر == 1) {
_                   (ReadLEB_i7(&form, &i_بایټونه, i_پای_ټکی));
                } وإلا {
_                   (Parse_Rec_ډله(io_module, شمېر, index, &i_بایټونه, i_پای_ټکی));
                    index += شمېر;
                    استمر;
                }
            }
#د_که_پای

_           (ParseTypeDefinition(io_module, form, &ftype, &i_بایټونه, i_پای_ټکی));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_جمع_کړه_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = عدم; // ownership transferred to environment
        }

        io_module->عدد_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

} _catch:

    إذا (پایله) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->عدد_Func_Types = 0;
    }

    أرجع پایله;
}


M3_پایله ParseSection_Function (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32 عدد_Functions;
_   (ReadLEB_u32(&عدد_Functions, &i_بایټونه, i_پای_ټکی));                                  m3log (parse, "** Function [%d]", عدد_Functions);

    _throwif("too many functions", عدد_Functions > d_m3_Max_Sane_Functions_شمېر);

_   (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_Functions));

    لكل (u32 i = 0; i < عدد_Functions; ++i) {
        u32 func_Type_شاخص;
_       (ReadLEB_u32(&func_Type_شاخص, &i_بایټونه, i_پای_ټکی));

_       (Module_جمع_کړه_Function(io_module, func_Type_شاخص, عدم /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch: أرجع پایله;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
تعريفنوع بنية M3ImportDesc {
    u8           ډول;
    u32          type_شاخص;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_جدول_Info  جدول_2;
    M3_حافظه_Info حافظه;
} M3ImportDesc;


#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
ساكن
M3_پایله Read_نښان_Type (IM3Module i_module, u32* o_type_شاخص, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_بایټونه, i_پای_ټکی));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_شاخص, io_بایټونه, i_پای_ټکی));
    _throwif(m3Err_unknownType, not i_module or *o_type_شاخص >= i_module->عدد_Func_Types);

#  إذا !d_m3_Has_ډېری_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_عدد_Results(i_module->funcTypes[*o_type_شاخص]) != 0);
#  endif

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
ساكن
M3_پایله ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

_   (Read_u8(&o_desc->ډول, io_بایټونه, i_پای_ټکی));

    بدل (o_desc->ډول) {
    حالة d_external_ډول_function:
_       (ReadLEB_u32(&o_desc->type_شاخص, io_بایټونه, i_پای_ټکی));
        اكسر;

    حالة d_external_ډول_جدول:
_       (Read_Type_جدول_Type(i_module, &o_desc->جدول_2, io_بایټونه, i_پای_ټکی));
        اكسر;

    حالة d_external_ډول_حافظه:
_       (Parse_Type_حافظه(&o_desc->حافظه, io_بایټونه, i_پای_ټکی));
        اكسر;

    حالة d_external_ډول_global:
_       (Parse_ارزښت_Type(i_module, &o_desc->type, io_بایټونه, i_پای_ټکی));
_       (ReadLEB_u7(&o_desc->isMutable, io_بایټونه, i_پای_ټکی));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [بنسټ_Type_د(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
    حالة d_external_ډول_نښان:
_       (Read_نښان_Type(i_module, &o_desc->type_شاخص, io_بایټونه, i_پای_ټکی));
        اكسر;
#د_که_پای

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع پایله;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
ساكن
M3_پایله ApplyImportDesc (IM3Module io_module, ثابت M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_پایله پایله = m3Err_none;

    M3ImportInfo clearImport = { عدم, عدم };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->ډول, io_import->moduleUtf8, io_import->ډګر_Utf8);
    بدل (i_desc->ډول) {
    حالة d_external_ډول_function: {
_       (Module_جمع_کړه_Function(io_module, i_desc->type_شاخص, io_import))
        *io_import = clearImport;

        io_module->عدد_Func_Imports++;
    } اكسر;

    حالة d_external_ډول_جدول: {
        IM3_جدول جدول_2;
_       (Module_جمع_کړه_جدول(io_module, &جدول_2, &i_desc->جدول_2, true /* isImport */));
        جدول_2->import = *io_import;
        *io_import    = clearImport;
    } اكسر;

    حالة d_external_ډول_حافظه: {
        IM3_حافظه حافظه;

#إذا d_m3_Has_Multi_حافظه
        _throwif(m3_Err_too_Many_حافظه_Sections, io_module->عدد_Memories >= d_m3_Max_Sane_Memories_شمېر);
#وإلا
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_حافظه_Sections, io_module->عدد_Memories >= 1);
#د_که_پای

_       (Module_جمع_کړه_حافظه(io_module, &حافظه, &i_desc->حافظه, true /* isImport */));
        حافظه->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

    حالة d_external_ډول_global: {
        IM3Global global;
_       (Module_جمع_کړه_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
    حالة d_external_ډول_نښان: {
        IM3_نښان نښان;
_       (Module_جمع_کړه_نښان(io_module, &نښان, io_module->funcTypes[i_desc->type_شاخص], true /* isImport */));
        نښان->import = *io_import;
        *io_import  = clearImport;
    } اكسر;
#د_که_پای

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع پایله;
}


#إذا d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
ساكن
bool Is_تشه_Name (بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    u32 اوږدتیا;
    أرجع (not ReadLEB_u32(&اوږدتیا, &i_بایټونه, i_پای_ټکی)) and اوږدتیا == 0;
}

#د_که_پای // d_m3HasCompactImports


#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching

M3_پایله Parse_Section_نښان (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32 عدد_نښانونه;
_   (ReadLEB_u32(&عدد_نښانونه, &i_بایټونه, i_پای_ټکی));                                       m3log (parse, "** Tag [%d]", عدد_نښانونه);

    _throwif("too many tags", عدد_نښانونه > d_m3_Max_Sane_نښانونه_شمېر);

    لكل (u32 i = 0; i < عدد_نښانونه; ++i) {
        u32 type_شاخص;
_       (Read_نښان_Type(io_module, &type_شاخص, &i_بایټونه, i_پای_ټکی));                      m3log (parse, "    tag: [%d] type: %d", i, type_شاخص);

_       (Module_جمع_کړه_نښان(io_module, عدم, io_module->funcTypes[type_شاخص], false));
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch: أرجع پایله;
}

#د_که_پای // d_m3HasExceptionHandling


M3_پایله ParseSection_Import (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    M3ImportInfo import = { عدم, عدم };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 عدد_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 عدد_Entries;
_   (ReadLEB_u32(&عدد_Entries, &i_بایټونه, i_پای_ټکی));                                    m3log (parse, "** Import [%d]", عدد_Entries);

    _throwif("too many imports", عدد_Entries > d_m3_Max_Sane_Imports_شمېر);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_Entries));

    لكل (u32 i = 0; i < عدد_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_بایټونه, i_پای_ټکی));

#إذا d_m3HasCompactImports
        بایټونه_t ډګر_پیل = i_بایټونه;
#د_که_پای
_       (Read_utf8(&import.ډګر_Utf8, &i_بایټونه, i_پای_ټکی));

#إذا d_m3HasCompactImports
        u8 compact = (i_بایټونه < i_پای_ټکی) ? *i_بایټونه : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        إذا ((compact == d_compact_Imports_فی_توکی_Type or compact == d_compactImports_sharedType) and
            Is_تشه_Name(ډګر_پیل, i_پای_ټکی)) {
            ++i_بایټونه;

            m3_Free(import.ډګر_Utf8);         // it only marked the compact form
            import.ډګر_Utf8 = عدم;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            إذا (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_بایټونه, i_پای_ټکی));
            }

            u32 عدد_توکي;
_           (ReadLEB_u32(&عدد_توکي, &i_بایټونه, i_پای_ټکی));                              m3log (parse, "    compact: %d import(s) of '%s'", عدد_توکي, import.moduleUtf8);

            _throwif("too many imports", (u64)عدد_Imports + عدد_توکي > d_m3_Max_Sane_Imports_شمېر);
            عدد_Imports += عدد_توکي;

_           (Module_PreallocFunctions(io_module, io_module->عدد_Functions + عدد_توکي));

            لكل (u32 j = 0; j < عدد_توکي; ++j) {
                M3ImportInfo توکی = { عدم, عدم };

                توکی.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, مزي_اوږدوالی(import.moduleUtf8) + 1);

                إذا (توکی.moduleUtf8) {
                    پایله = Read_utf8(&توکی.ډګر_Utf8, &i_بایټونه, i_پای_ټکی);

                    // the shared form reuses the externtype read above
                    إذا (not پایله and compact == d_compact_Imports_فی_توکی_Type) {
                        پایله = ReadImportDesc(io_module, &desc, &i_بایټونه, i_پای_ټکی);
                    }

                    إذا (not پایله) {
                        پایله = ApplyImportDesc(io_module, &desc, &توکی);
                    }
                } وإلا {
                    پایله = m3Err_mallocFailed;
                }

                FreeImportInfo(&توکی);
                _throwif(پایله, پایله);
            }

            FreeImportInfo(&import);
            استمر;
        }
#د_که_پای // d_m3HasCompactImports

        _throwif("too many imports", عدد_Imports >= d_m3_Max_Sane_Imports_شمېر);
        ++عدد_Imports;

_       (ReadImportDesc(io_module, &desc, &i_بایټونه, i_پای_ټکی));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    أرجع پایله;
}


M3_پایله ParseSection_Export (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله    پایله = m3Err_none;
    ثابت حرف* utf8   = عدم;

    u32 عدد_Exports;
_   (ReadLEB_u32(&عدد_Exports, &i_بایټونه, i_پای_ټکی));                                    m3log (parse, "** Export [%d]", عدد_Exports);

    _throwif("too many exports", عدد_Exports > d_m3_Max_Sane_Exports_شمېر);

    // the section order check lets this section through once, so this is
    // every export the module has
    إذا (عدد_Exports) {
        io_module->exports = m3_AllocArray(M3Export, عدد_Exports);
        _throwifnull(io_module->exports);
    }

    لكل (u32 i = 0; i < عدد_Exports; ++i) {
        u8  export_ډول;
        u32 index;
        u32 name_اوږدتیا;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            بایټونه_t اوږدتیا_بایټونه = i_بایټونه;
_           (ReadLEB_u32(&name_اوږدتیا, &اوږدتیا_بایټونه, i_پای_ټکی));
        }

_       (Read_utf8(&utf8, &i_بایټونه, i_پای_ټکی));
_       (Read_u8(&export_ډول, &i_بایټونه, i_پای_ټکی));
_       (ReadLEB_u32(&index, &i_بایټونه, i_پای_ټکی));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_ډول, utf8);

#إذا d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        لكل (u32 j = 0; j < i; ++j) {
            ثابت M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_اوږدتیا == name_اوږدتیا and memcmp(other->name, utf8, name_اوږدتیا) == 0);
        }
#د_که_پای

        إذا (export_ډول == d_external_ډول_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Functions);
            IM3Function func = &(io_module->functions[index]);
            إذا (func->عدد_نومونه < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_اوږدتیا + 1);
                _throwifnull(name);
                func->نومونه[func->عدد_نومونه++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } وإلا إذا (export_ډول == d_external_ډول_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Globals);
        } وإلا إذا (export_ډول == d_external_ډول_حافظه) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Memories);
        } وإلا إذا (export_ډول == d_external_ډول_جدول) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_Tables);
        }
#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
        وإلا إذا (export_ډول == d_external_ډول_نښان) {
            _throwif(m3Err_wasmMalformed, index >= io_module->عدد_نښانونه);
        }
#د_که_پای

        M3Export* ثبت   = &io_module->exports[io_module->عدد_Exports++];
        ثبت->name       = utf8;
        ثبت->name_اوږدتیا = name_اوږدتیا;
        ثبت->index      = index;
        ثبت->ډول       = export_ډول;

        utf8 = عدم; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

_catch:
    m3_Free(utf8);
    أرجع پایله;
}


M3_پایله Parse_Section_پیل (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32 پیل_Func_شاخص;
_   (ReadLEB_u32(&پیل_Func_شاخص, &i_بایټونه, i_پای_ټکی));                                  m3log (parse, "** Start Function: %d", پیل_Func_شاخص);

    إذا (پیل_Func_شاخص < io_module->عدد_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[پیل_Func_شاخص];
        إذا (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->عدد_دلیلونه != 0 || func->funcType->عدد_Rets != 0);
        }

        io_module->پیل_Function = پیل_Func_شاخص;
    } وإلا {
        پایله = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch: أرجع پایله;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_پایله Parse_Init_بیان_Typed (M3Module* io_module, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی, m3type_t* o_type)
{
    M3_پایله پایله = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Compilation compilation;
#وإلا
    M3Compilation compilation;
#د_که_پای
    compilation = (M3Compilation){ .runtime = عدم, .module = io_module, .wasm = *io_بایټونه, .wasm_پای_ټکی = i_پای_ټکی, .is_Init_بیان = true };

    پایله = Compile_بلاک_Statements(&compilation);

    *io_بایټونه = compilation.wasm;

    إذا (o_type) {
        *o_type = compilation.ډېری_شاخص ? compilation.type_ډېری[compilation.ډېری_شاخص - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    أرجع پایله;
}


M3_پایله Parse_Init_بیان (M3Module* io_module, بایټونه_t* io_بایټونه, cbytes_t i_پای_ټکی)
{
    أرجع Parse_Init_بیان_Typed(io_module, io_بایټونه, i_پای_ټکی, عدم);
}


M3_پایله ParseSection_Element (IM3Module io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32     عدد_Segments;
    بایټونه_t pos;
_   (ReadLEB_u32(&عدد_Segments, &i_بایټونه, i_پای_ټکی));                            m3log (parse, "** Element [%d]", عدد_Segments);

    _throwif("too many element segments", عدد_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, عدد_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->عدد_Element_Segments = عدد_Segments;
    io_module->element_Section_پای_ټکی  = i_پای_ټکی;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_بایټونه;
    لكل (u32 i = 0; i < عدد_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_پای_ټکی));
#إذا d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#وإلا
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_فعال);
#د_که_پای

        segment->ډېرپېښ   = flags & 0x3;
        segment->is_بیان = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        إذا (segment->ډېرپېښ == c_m3_Elem_فعال_Idx) {
_           (ReadLEB_u32(&segment->جدول_شاخص, &pos, i_پای_ټکی));
        }

        bool is_فعال = (segment->ډېرپېښ == c_m3_Elem_فعال or segment->ډېرپېښ == c_m3_Elem_فعال_Idx);

        إذا (is_فعال) {
            _throwif(m3Err_wasmMalformed, segment->جدول_شاخص >= io_module->عدد_Tables);

            segment->init_بیان = pos;
_           (Parse_Init_بیان(io_module, &pos, i_پای_ټکی));
            segment->init_بیان_اندازه = (u32)(pos - segment->init_بیان);
            _throwif(m3_Err_wasm_Missing_Init_بیان, segment->init_بیان_اندازه <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        إذا (segment->ډېرپېښ != c_m3_Elem_فعال) {
            إذا (segment->is_بیان) {
_               (Parse_ارزښت_Type(io_module, &segment->type, &pos, i_پای_ټکی));
                _throwif(m3Err_wasmMalformed, not IsRefType(بنسټ_Type_د(segment->type)));
            } وإلا {
                u8 elem_ډول;
_               (Read_u8(&elem_ډول, &pos, i_پای_ټکی));
                _throwif(m3Err_wasmMalformed, elem_ډول != 0x00);   // funcref
            }
        }

        إذا (is_فعال) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->جدول_شاخص]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->عدد_Elements, &pos, i_پای_ټکی));
        _throwif("table overflow", segment->عدد_Elements > d_m3_Max_Sane_جدول_اندازه);

        segment->elements = pos;

        لكل (u32 e = 0; e < segment->عدد_Elements; ++e) {
            إذا (segment->is_بیان) {
_               (Parse_Init_بیان(io_module, &pos, i_پای_ټکی))
            } وإلا {
                u32 func_شاخص;
_               (ReadLEB_u32(&func_شاخص, &pos, i_پای_ټکی));
                _throwif("function index out of range", func_شاخص >= io_module->عدد_Functions);
_               (Module_DeclareFunction(io_module, func_شاخص));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_پای_ټکی);           // section size mismatch

    _catch: أرجع پایله;
}


M3_پایله Parse_Section_کوډ (M3Module* io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله;

    u32 عدد_Functions;
_   (ReadLEB_u32(&عدد_Functions, &i_بایټونه, i_پای_ټکی));                                  m3log (parse, "** Code [%d]", عدد_Functions);

    إذا (عدد_Functions != io_module->عدد_Functions - io_module->عدد_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    لكل (u32 f = 0; f < عدد_Functions; ++f) {
        ثابت u8* پیل = i_بایټونه;

        u32 اندازه;
_       (ReadLEB_u32(&اندازه, &i_بایټونه, i_پای_ټکی));

        إذا (اندازه) {
            i_بایټونه += اندازه;

            إذا (i_بایټونه <= i_پای_ټکی) {
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
                func->wasm    = پیل;
                func->wasm_پای_ټکی = i_بایټونه;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } وإلا {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    إذا (not پایله and i_بایټونه != i_پای_ټکی) {
        پایله = m3Err_wasmSectionUnderrun;
    }

    أرجع پایله;
}


M3_پایله Parse_Section_معلومات (M3Module* io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32 عدد_معلومات_Segments;
_   (ReadLEB_u32(&عدد_معلومات_Segments, &i_بایټونه, i_پای_ټکی));                               m3log (parse, "** Data [%d]", عدد_معلومات_Segments);

    _throwif("too many data segments", عدد_معلومات_Segments > d_m3_Max_Sane_معلومات_Segments);

    io_module->معلومات_Segments = m3_AllocArray(M3_معلومات_Segment, عدد_معلومات_Segments);
    _throwifnull(io_module->معلومات_Segments);
    io_module->عدد_معلومات_Segments = عدد_معلومات_Segments;

    لكل (u32 i = 0; i < عدد_معلومات_Segments; ++i) {
        M3_معلومات_Segment* segment = &io_module->معلومات_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_بایټونه, i_پای_ټکی));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        إذا (flags == 2) {
_           (ReadLEB_u32(&segment->حافظه_Region, &i_بایټونه, i_پای_ټکی));
        }

        إذا (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#إذا !d_m3_Has_Multi_حافظه
            _throwif(m3Err_wasmMalformed, segment->حافظه_Region != 0);
#د_که_پای
            _throwif(m3Err_wasmMalformed, segment->حافظه_Region >= io_module->عدد_Memories);

            segment->init_بیان = i_بایټونه;
_           (Parse_Init_بیان(io_module, &i_بایټونه, i_پای_ټکی));
            segment->init_بیان_اندازه = (u32)(i_بایټونه - segment->init_بیان);

            _throwif(m3_Err_wasm_Missing_Init_بیان, segment->init_بیان_اندازه <= 1);
        }

_       (ReadLEB_u32(&segment->اندازه, &i_بایټونه, i_پای_ټکی));
        segment->معلومات = i_بایټونه;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->حافظه_Region, segment->init_بیان_اندازه, segment->اندازه);
        i_بایټونه += segment->اندازه;

        _throwif("data segment underflow", i_بایټونه > i_پای_ټکی);
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch:

    أرجع پایله;
}


M3_پایله Parse_Section_معلومات_شمېر (M3Module* io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

_   (ReadLEB_u32(&io_module->معلومات_شمېر, &i_بایټونه, i_پای_ټکی));                       m3log (parse, "** DataCount [%d]", io_module->معلومات_شمېر);

    io_module->has_معلومات_شمېر = true;

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch: أرجع پایله;
}


M3_پایله Parse_Section_حافظه (M3Module* io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32 عدد_Memories;
_   (ReadLEB_u32(&عدد_Memories, &i_بایټونه, i_پای_ټکی));                                m3log (parse, "** Memory [%d]", عدد_Memories);

#إذا d_m3_Has_Multi_حافظه
    _throwif(m3_Err_too_Many_حافظه_Sections,
             (u64)عدد_Memories + io_module->عدد_Memories > d_m3_Max_Sane_Memories_شمېر);
#وإلا
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_حافظه_Sections, عدد_Memories > 1);
    _throwif(m3_Err_too_Many_حافظه_Sections, (u64)عدد_Memories + io_module->عدد_Memories > 1);
#د_که_پای

    لكل (u32 i = 0; i < عدد_Memories; ++i) {
        M3_حافظه_Info info;
_       (Parse_Type_حافظه(&info, &i_بایټونه, i_پای_ټکی));
_       (Module_جمع_کړه_حافظه(io_module, عدم, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch: أرجع پایله;
}


M3_پایله ParseSection_Global (M3Module* io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    u32 عدد_Globals;
_   (ReadLEB_u32(&عدد_Globals, &i_بایټونه, i_پای_ټکی));                                    m3log (parse, "** Global [%d]", عدد_Globals);

    _throwif("too many globals", عدد_Globals > d_m3_Max_Sane_Globals_شمېر);

    لكل (u32 i = 0; i < عدد_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_ارزښت_Type(io_module, &type, &i_بایټونه, i_پای_ټکی));
_       (ReadLEB_u7(&isMutable, &i_بایټونه, i_پای_ټکی));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [بنسټ_Type_د(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_جمع_کړه_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_بیان = i_بایټونه;
_       (Parse_Init_بیان(io_module, &i_بایټونه, i_پای_ټکی));
        global->init_بیان_اندازه = (u32)(i_بایټونه - global->init_بیان);

        _throwif(m3_Err_wasm_Missing_Init_بیان, global->init_بیان_اندازه <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_بایټونه != i_پای_ټکی);      // section size mismatch

    _catch: أرجع پایله;
}


M3_پایله ParseSection_Name (M3Module* io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    cstr_t name = عدم;

    طالما (i_بایټونه < i_پای_ټکی) {
        u8  nameType;
        u32 payload_اوږدتیا;

_       (ReadLEB_u7(&nameType, &i_بایټونه, i_پای_ټکی));
_       (ReadLEB_u32(&payload_اوږدتیا, &i_بایټونه, i_پای_ټکی));

        بایټونه_t پیل = i_بایټونه;
        _throwif(m3Err_wasmSectionOverrun, payload_اوږدتیا > (size_t)(i_پای_ټکی - پیل));

        إذا (nameType == 1) {
            u32 عدد_نومونه;
_           (ReadLEB_u32(&عدد_نومونه, &i_بایټونه, i_پای_ټکی));

            _throwif("too many names", عدد_نومونه > d_m3_Max_Sane_Functions_شمېر);

            لكل (u32 i = 0; i < عدد_نومونه; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_بایټونه, i_پای_ټکی));
_               (Read_utf8(&name, &i_بایټونه, i_پای_ټکی));

                إذا (index < io_module->عدد_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    إذا (func->عدد_نومونه == 0) {
                        func->نومونه[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->عدد_نومونه = 1;

                        name = عدم; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_بایټونه = پیل + payload_اوږدتیا;
    }

    _catch: أرجع پایله;
}


M3_پایله ParseSection_Custom (M3Module* io_module, بایټونه_t i_بایټونه, cbytes_t i_پای_ټکی)
{
    M3_پایله پایله = m3Err_none;

    cstr_t name = عدم;
_   (Read_utf8(&name, &i_بایټونه, i_پای_ټکی));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    إذا (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_بایټونه, i_پای_ټکی));
#إذا d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } وإلا إذا (strcmp(name, "snapshot") == 0 || (مزي_اوږدوالی(name) > 9 && مزي_پرتله(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, مزي_اوږدوالی(snapName) + 1);
        إذا (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->معلومات           = i_بایټونه;
        snap->اندازه           = (u32)(i_پای_ټکی - i_بایټونه);
        snap->بل           = io_module->snapshots;
        io_module->snapshots = snap;
#د_که_پای
    } وإلا إذا (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_بایټونه, i_پای_ټکی));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    أرجع پایله;
}


M3_پایله ParseModuleSection (M3Module* o_module, u8 i_sectionType, بایټونه_t i_بایټونه, u32 i_عدد_بایټونه)
{
    M3_پایله پایله = m3Err_none;

    تعريفنوع M3_پایله (*M3Parser)(M3Module*, بایټونه_t, cbytes_t);

    ساكن M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_جدول,        // 4
        Parse_Section_حافظه,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_پیل,     // 8
        ParseSection_Element,   // 9
        Parse_Section_کوډ,      // 10
        Parse_Section_معلومات,      // 11
        Parse_Section_معلومات_شمېر, // 12
#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
        Parse_Section_نښان,       // 13
#د_که_پای
    };

    M3Parser parser = عدم;

    إذا (i_sectionType < M3_شمېر_د(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    إذا (parser) {
        cbytes_t پای_ټکی = i_بایټونه + i_عدد_بایټونه;
        پایله       = parser(o_module, i_بایټونه, پای_ټکی);
    } وإلا {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    أرجع پایله;
}


M3_پایله m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_بایټونه, u32 i_عدد_بایټونه)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_عدد_بایټونه);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_عدد_بایټونه);
    module->پیل_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ثابت u8* pos = i_بایټونه;
    ثابت u8* پای_ټکی = pos + i_عدد_بایټونه;

    module->wasm_پیل = pos;
    module->wasm_پای_ټکی   = پای_ټکی;

    u32 magic, نسخه;
_   (Read_u32(&magic, &pos, پای_ټکی));
_   (Read_u32(&نسخه, &pos, پای_ټکی));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_نسخه, نسخه != 1);

#إذا d_m3HasExceptionHandling || d_m3_Has_ډېری_Switching
    // the tag section sits between memory (5) and global (6)
    ساكن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#وإلا
    ساكن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#د_که_پای
    u8 expectedSection = 0;

    طالما (pos < پای_ټکی) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, پای_ټکی));

        إذا (section != 0) {
            // Ensure sections appear only once and in order
            طالما (sections_ترتیب[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_شمېر_د(sections_ترتیب) - 1);
            }
        }

        u32 section_اوږدتیا;
_       (ReadLEB_u32(&section_اوږدتیا, &pos, پای_ټکی));
        _throwif(m3Err_wasmMalformed, pos + section_اوږدتیا > پای_ټکی);

_       (ParseModuleSection(module, section, pos, section_اوږدتیا));

        pos += section_اوږدتیا;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    إذا (module->عدد_Functions > module->عدد_Func_Imports) {
        IM3Function لومړی_Non_Import = &module->functions[module->عدد_Func_Imports];
        _throwif(m3Err_wasmMalformed, لومړی_Non_Import->wasm == عدم);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_معلومات_شمېر and module->معلومات_شمېر != module->عدد_معلومات_Segments);

} _catch:

    إذا (پایله) {
        m3_FreeModule(module);
        module = عدم;
    }

    *o_module = module;

    أرجع پایله;
}
