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
M3_نتیجه Parse_انباشته_Type (IM3Module i_module, m3type_t* o_انباشته_بیتها, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    i64 انباشته;
_   (ReadLebSigned(&انباشته, 33, io_بایتها, i_انتها));

    إذا (انباشته == -d_waType_funcref) {
        *o_انباشته_بیتها = d_m3_Type_انباشته_Abstract;
    } وإلا إذا (انباشته == -d_waType_externref) {
        *o_انباشته_بیتها = d_m3Type_refExtern | d_m3_Type_انباشته_Abstract;
#  إذا d_m3_Has_پشته_Switching
    } وإلا إذا (انباشته == -d_wa_Type_انباشته_cont) {
        *o_انباشته_بیتها = d_m3Type_refCont | d_m3_Type_انباشته_Abstract;
    } وإلا إذا (انباشته == -d_wa_Type_انباشته_nocont) {
        *o_انباشته_بیتها = d_m3Type_refCont | d_m3_Type_انباشته_None;
#  endif
    } وإلا إذا (انباشته >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) انباشته >= i_module->رقم_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[انباشته];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_انباشته_بیتها = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_شاخص;
    } وإلا {
        _throw(m3Err_invalidTypeId);
    }

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_نتیجه Parse_مقدار_Type (IM3Module i_module, m3type_t* o_type, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_بایتها >= i_انتها);

#إذا d_m3HasTypedRefs
    إذا (**io_بایتها == d_waEncode_ref or **io_بایتها == d_waEncode_refNull) {
        u8 lead = *(*io_بایتها)++;

        m3type_t انباشته_بیتها = d_m3_Type_انباشته_Abstract;
_       (Parse_انباشته_Type(i_module, &انباشته_بیتها, io_بایتها, i_انتها));

        *o_type = d_m3Type_ref | انباشته_بیتها | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        أرجع نتیجه;
    }
#پایان_اگر

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_بایتها, i_انتها));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: أرجع نتیجه;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
ساكن
M3_نتیجه Read_Type_جدول_Type (IM3Module i_module, M3_جدول_Info* o_info, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_اندازه = 0, max_اندازه = 0;

    // the width of the limits, and of every index into the table
    u32 addr_بیتها = 32;

_   (Parse_مقدار_Type(i_module, &elemType, io_بایتها, i_انتها));
#إذا d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(پایه_Type_از(elemType)));
#وإلا
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#پایان_اگر

_   (ReadLEB_u7(&flag, io_بایتها, i_انتها));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#إذا d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#وإلا
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#پایان_اگر

    إذا (flag & 0x04u) {
        addr_بیتها = 64;
    }

_   (ReadLebUnsigned(&init_اندازه, addr_بیتها, io_بایتها, i_انتها));
    _throwif("table overflow", init_اندازه > d_m3_Max_Sane_جدول_اندازه);

    إذا (flag & 0x01u) {
_       (ReadLebUnsigned(&max_اندازه, addr_بیتها, io_بایتها, i_انتها));
        _throwif(m3Err_wasmMalformed, max_اندازه < init_اندازه);
        _throwif("table overflow", max_اندازه > d_m3_Max_Sane_جدول_اندازه);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_اندازه  = (u32)init_اندازه;
    o_info->max_اندازه   = (u32)max_اندازه;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    أرجع نتیجه;
}


ساكن
M3_نتیجه Parse_Type_جدول_Type (IM3Module io_module, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3_جدول_Info info;

_   (Read_Type_جدول_Type(io_module, &info, io_بایتها, i_انتها));
_   (Module_جمع_کن_جدول(io_module, عدم, &info, false /* isImport */));

    _catch: أرجع نتیجه;
}


M3_نتیجه Parse_Init_عبارت_Typed (M3Module* io_module, بایتها_t* io_بایتها, cbytes_t i_انتها, m3type_t* o_type);

M3_نتیجه Parse_Type_جدول (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 رقم_Tables;
_   (ReadLEB_u32(&رقم_Tables, &i_بایتها, i_انتها));                          m3log (parse, "** Table [%d]", رقم_Tables);

    _throwif("too many tables", (u64)رقم_Tables + io_module->رقم_Tables > d_m3_Max_Sane_جدول_شمار);

    لكل (u32 i = 0; i < رقم_Tables; ++i) {
#إذا d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_عبارت = (i_بایتها < i_انتها) and (*i_بایتها == 0x40);
        u32  جدول_شاخص  = io_module->رقم_Tables;

        إذا (has_Init_عبارت) {
            u8 reserved;
            ++i_بایتها;
_           (Read_u8(&reserved, &i_بایتها, i_انتها));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_جدول_Type(io_module, &i_بایتها, i_انتها));

        إذا (has_Init_عبارت) {
            IM3_جدول جدول_2 = io_module->tables[جدول_شاخص];

            m3type_t initType;

            جدول_2->init_عبارت = i_بایتها;
_           (Parse_Init_عبارت_Typed(io_module, &i_بایتها, i_انتها, &initType));
            جدول_2->init_عبارت_اندازه = (u32)(i_بایتها - جدول_2->init_عبارت);
            _throwif(m3_Err_wasm_Missing_Init_عبارت, جدول_2->init_عبارت_اندازه <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_از(initType, جدول_2->type));
        } وإلا {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[جدول_شاخص]->type));
        }
#وإلا
_       (Parse_Type_جدول_Type(io_module, &i_بایتها, i_انتها));
#پایان_اگر
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch: أرجع نتیجه;
}


M3_نتیجه Parse_Type_حافظه (M3_حافظه_Info* o_حافظه, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_صفحه_اندازه = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_بیتها = 32;

_   (ReadLEB_u7(&flag, io_بایتها, i_انتها));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 معلوم = 0x09u;
#إذا d_m3HasAtomics
        معلوم |= 0x02u;
#پایان_اگر
#إذا d_m3HasMemory64
        معلوم |= 0x04u;
#پایان_اگر
        _throwif(m3Err_wasmMalformed, flag & ~معلوم);
    }

    o_حافظه->isShared = (flag & (1u << 1)) != 0;

    o_حافظه->isMemory64 = (flag & (1u << 2)) != 0;
    إذا (o_حافظه->isMemory64) {
        addr_بیتها = 64;
    }

_   (ReadLebUnsigned(&o_حافظه->init_صفحات, addr_بیتها, io_بایتها, i_انتها));

    o_حافظه->max_صفحات = 0;
    o_حافظه->hasMax   = (flag & (1u << 0)) != 0;
    إذا (o_حافظه->hasMax) {
_       (ReadLebUnsigned(&o_حافظه->max_صفحات, addr_بیتها, io_بایتها, i_انتها));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_حافظه->max_صفحات < o_حافظه->init_صفحات);
    }

    _throwif("shared memory must have maximum", o_حافظه->isShared and not o_حافظه->hasMax);

    o_حافظه->صفحه_اندازه = 0;
    إذا (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_صفحه_اندازه, io_بایتها, i_انتها));

        _throwif("invalid custom page size", log_صفحه_اندازه > 16);

        o_حافظه->صفحه_اندازه = 1u << log_صفحه_اندازه;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_صفحات_Allowed = (log_صفحه_اندازه or addr_بیتها < 64)
                                ? (UINT64_C(1) << (addr_بیتها - log_صفحه_اندازه))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_حافظه->init_صفحات > max_صفحات_Allowed);
        إذا (o_حافظه->hasMax) {
            _throwif(m3Err_wasmMalformed, o_حافظه->max_صفحات > max_صفحات_Allowed);
        }
    }

    _catch: أرجع نتیجه;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
ساكن
M3_نتیجه ParseFuncType (IM3Module io_module, IM3FuncType* o_type, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    IM3FuncType ftype = عدم;
_try {
    u32 رقم_آرگومانها;
_   (ReadLEB_u32(&رقم_آرگومانها, io_بایتها, i_انتها));

    _throwif(m3_Err_too_Many_آرگومانها_Rets, رقم_آرگومانها > d_m3_Max_Sane_Function_Arg_Ret_شمار);
#إذا M3_HAS_VLA
    m3type_t argTypes[رقم_آرگومانها + 1]; // make ubsan happy
#وإلا
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_شمار];
#پایان_اگر
    لكل (u32 a = 0; a < رقم_آرگومانها; ++a) {
_       (Parse_مقدار_Type(io_module, &argTypes[a], io_بایتها, i_انتها));
    }

    u32 رقم_Rets;
_   (ReadLEB_u32(&رقم_Rets, io_بایتها, i_انتها));
    _throwif(m3_Err_too_Many_آرگومانها_Rets, (u64)(رقم_Rets) + رقم_آرگومانها > d_m3_Max_Sane_Function_Arg_Ret_شمار);

_   (AllocFuncType(&ftype, رقم_Rets + رقم_آرگومانها));
    ftype->رقم_آرگومانها = رقم_آرگومانها;
    ftype->رقم_Rets = رقم_Rets;

    لكل (u32 r = 0; r < رقم_Rets; ++r) {
_       (Parse_مقدار_Type(io_module, &ftype->types[r], io_بایتها, i_انتها));
    }
    کپی_حافظه(ftype->types + رقم_Rets, argTypes, رقم_آرگومانها * حجم(m3type_t));

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع نتیجه;
}


#إذا d_m3_Has_پشته_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
ساكن
M3_نتیجه ParseContType (IM3Module io_module, IM3FuncType* o_type, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    IM3FuncType ftype = عدم;
_try {
    i64 ft_شاخص;
_   (ReadLebSigned(&ft_شاخص, 33, io_بایتها, i_انتها));
    _throwif(m3Err_wasmMalformed, ft_شاخص < 0 or (u64) ft_شاخص >= io_module->رقم_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_شاخص];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 مجموع = underlying->رقم_Rets + underlying->رقم_آرگومانها;
_   (AllocFuncType(&ftype, مجموع));
    ftype->رقم_Rets        = underlying->رقم_Rets;
    ftype->رقم_آرگومانها        = underlying->رقم_آرگومانها;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    إذا (مجموع) {
        کپی_حافظه(ftype->types, underlying->types, مجموع * حجم(m3type_t));
    }

    *o_type = ftype;
    ftype   = عدم;

} _catch:
    m3_Free(ftype);
    أرجع نتیجه;
}

#پایان_اگر // d_m3HasStackSwitching


ساكن
M3_نتیجه ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              بایتها_t* io_بایتها, cbytes_t i_انتها)
{
#إذا d_m3_Has_پشته_Switching
    إذا (i_form == -35) {
        أرجع ParseContType(io_module, o_type, io_بایتها, i_انتها);
    }
#پایان_اگر
    إذا (i_form != -32) { // for Wasm MVP
        أرجع m3Err_wasmMalformed;
    }

    أرجع ParseFuncType(io_module, o_type, io_بایتها, i_انتها);
}


#إذا d_m3_Has_پشته_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
ساكن
M3_نتیجه SkipTypeDefinition (i8 i_form, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
_try {
    إذا (i_form == -35) {
        i64 ft_شاخص;
_       (ReadLebSigned(&ft_شاخص, 33, io_بایتها, i_انتها));
        أرجع نتیجه;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    لكل (u32 half = 0; half < 2; ++half) {
        u32 شمار;
_       (ReadLEB_u32(&شمار, io_بایتها, i_انتها));
        _throwif(m3_Err_too_Many_آرگومانها_Rets, شمار > d_m3_Max_Sane_Function_Arg_Ret_شمار);

        لكل (u32 v = 0; v < شمار; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_بایتها, i_انتها));

            إذا (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 انباشته;
_               (ReadLebSigned(&انباشته, 33, io_بایتها, i_انتها));
            }
        }
    }

} _catch: أرجع نتیجه;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
ساكن
M3_نتیجه Parse_Rec_گروه (IM3Module io_module, u32 i_شمار, u32 i_اولی_شاخص,
                        بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    IM3FuncType* placeholders = عدم;
_try {
    بایتها_t گروه_شروع = *io_بایتها;

    placeholders = m3_AllocArray(IM3FuncType, i_شمار);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    لكل (u32 j = 0; j < i_شمار; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_بایتها, i_انتها));
_       (SkipTypeDefinition(form, io_بایتها, i_انتها));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 اولی_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_شمار, &اولی_Canonical));

    لكل (u32 j = 0; j < i_شمار; ++j) {
        placeholders[j]->canonical_شاخص        = (u16)(اولی_Canonical + j);
        io_module->funcTypes[i_اولی_شاخص + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    لكل (u32 pass = 0; pass < 2; ++pass) {
        بایتها_t بایتها = گروه_شروع;

        لكل (u32 j = 0; j < i_شمار; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &بایتها, i_انتها));

            إذا ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &بایتها, i_انتها));
                استمر;
            }

            IM3FuncType ftype = عدم;
_           (ParseTypeDefinition(io_module, form, &ftype, &بایتها, i_انتها));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_شاخص);

            m3_Free(placeholders[j]);
            placeholders[j]                        = عدم;
            io_module->funcTypes[i_اولی_شاخص + j] = ftype;
        }

        إذا (pass == 1) {
            *io_بایتها = بایتها;
        }
    }

} _catch:
    إذا (placeholders) {
        لكل (u32 j = 0; j < i_شمار; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    أرجع نتیجه;
}

#پایان_اگر // d_m3HasStackSwitching


M3_نتیجه ParseSection_Type (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    IM3FuncType ftype = عدم;

_try {
    u32 رقم_Types;
_   (ReadLEB_u32(&رقم_Types, &i_بایتها, i_انتها));                                      m3log (parse, "** Type [%d]", رقم_Types);

    _throwif("too many types", رقم_Types > d_m3_Max_Sane_Types_شمار);

    إذا (رقم_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, رقم_Types);
        _throwifnull(io_module->funcTypes);
        io_module->رقم_Func_Types = رقم_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#إذا d_m3_Has_پشته_Switching
        u32 ظرفیت = رقم_Types;
#پایان_اگر
        u32 index = 0;

        لكل (u32 i = 0; i < رقم_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_بایتها, i_انتها));

#إذا d_m3_Has_پشته_Switching
            إذا (form == -50) { // rec
                u32 شمار;
_               (ReadLEB_u32(&شمار, &i_بایتها, i_انتها));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + شمار + (رقم_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_شمار);

                إذا (needed > ظرفیت) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, ظرفیت);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    ظرفیت                = grown;
                    io_module->رقم_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                إذا (شمار == 1) {
_                   (ReadLEB_i7(&form, &i_بایتها, i_انتها));
                } وإلا {
_                   (Parse_Rec_گروه(io_module, شمار, index, &i_بایتها, i_انتها));
                    index += شمار;
                    استمر;
                }
            }
#پایان_اگر

_           (ParseTypeDefinition(io_module, form, &ftype, &i_بایتها, i_انتها));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_جمع_کن_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = عدم; // ownership transferred to environment
        }

        io_module->رقم_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

} _catch:

    إذا (نتیجه) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->رقم_Func_Types = 0;
    }

    أرجع نتیجه;
}


M3_نتیجه ParseSection_Function (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 رقم_Functions;
_   (ReadLEB_u32(&رقم_Functions, &i_بایتها, i_انتها));                                  m3log (parse, "** Function [%d]", رقم_Functions);

    _throwif("too many functions", رقم_Functions > d_m3_Max_Sane_Functions_شمار);

_   (Module_PreallocFunctions(io_module, io_module->رقم_Functions + رقم_Functions));

    لكل (u32 i = 0; i < رقم_Functions; ++i) {
        u32 func_Type_شاخص;
_       (ReadLEB_u32(&func_Type_شاخص, &i_بایتها, i_انتها));

_       (Module_جمع_کن_Function(io_module, func_Type_شاخص, عدم /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch: أرجع نتیجه;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
تعريفنوع بنية M3ImportDesc {
    u8           نوع;
    u32          type_شاخص;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_جدول_Info  جدول_2;
    M3_حافظه_Info حافظه;
} M3ImportDesc;


#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
ساكن
M3_نتیجه Read_برچسب_Type (IM3Module i_module, u32* o_type_شاخص, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_بایتها, i_انتها));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_شاخص, io_بایتها, i_انتها));
    _throwif(m3Err_unknownType, not i_module or *o_type_شاخص >= i_module->رقم_Func_Types);

#  إذا !d_m3_Has_پشته_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_رقم_Results(i_module->funcTypes[*o_type_شاخص]) != 0);
#  endif

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
ساكن
M3_نتیجه ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (Read_u8(&o_desc->نوع, io_بایتها, i_انتها));

    بدل (o_desc->نوع) {
    حالة d_external_نوع_function:
_       (ReadLEB_u32(&o_desc->type_شاخص, io_بایتها, i_انتها));
        اكسر;

    حالة d_external_نوع_جدول:
_       (Read_Type_جدول_Type(i_module, &o_desc->جدول_2, io_بایتها, i_انتها));
        اكسر;

    حالة d_external_نوع_حافظه:
_       (Parse_Type_حافظه(&o_desc->حافظه, io_بایتها, i_انتها));
        اكسر;

    حالة d_external_نوع_global:
_       (Parse_مقدار_Type(i_module, &o_desc->type, io_بایتها, i_انتها));
_       (ReadLEB_u7(&o_desc->isMutable, io_بایتها, i_انتها));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [پایه_Type_از(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
    حالة d_external_نوع_برچسب:
_       (Read_برچسب_Type(i_module, &o_desc->type_شاخص, io_بایتها, i_انتها));
        اكسر;
#پایان_اگر

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع نتیجه;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
ساكن
M3_نتیجه ApplyImportDesc (IM3Module io_module, ثابت M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3ImportInfo clearImport = { عدم, عدم };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->نوع, io_import->moduleUtf8, io_import->فیلد_Utf8);
    بدل (i_desc->نوع) {
    حالة d_external_نوع_function: {
_       (Module_جمع_کن_Function(io_module, i_desc->type_شاخص, io_import))
        *io_import = clearImport;

        io_module->رقم_Func_Imports++;
    } اكسر;

    حالة d_external_نوع_جدول: {
        IM3_جدول جدول_2;
_       (Module_جمع_کن_جدول(io_module, &جدول_2, &i_desc->جدول_2, true /* isImport */));
        جدول_2->import = *io_import;
        *io_import    = clearImport;
    } اكسر;

    حالة d_external_نوع_حافظه: {
        IM3_حافظه حافظه;

#إذا d_m3_Has_Multi_حافظه
        _throwif(m3_Err_too_Many_حافظه_Sections, io_module->رقم_Memories >= d_m3_Max_Sane_Memories_شمار);
#وإلا
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_حافظه_Sections, io_module->رقم_Memories >= 1);
#پایان_اگر

_       (Module_جمع_کن_حافظه(io_module, &حافظه, &i_desc->حافظه, true /* isImport */));
        حافظه->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

    حالة d_external_نوع_global: {
        IM3Global global;
_       (Module_جمع_کن_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } اكسر;

#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
    حالة d_external_نوع_برچسب: {
        IM3_برچسب برچسب;
_       (Module_جمع_کن_برچسب(io_module, &برچسب, io_module->funcTypes[i_desc->type_شاخص], true /* isImport */));
        برچسب->import = *io_import;
        *io_import  = clearImport;
    } اكسر;
#پایان_اگر

    افتراضي:
        _throw(m3Err_wasmMalformed);
    }

    _catch: أرجع نتیجه;
}


#إذا d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
ساكن
bool Is_خالی_Name (بایتها_t i_بایتها, cbytes_t i_انتها)
{
    u32 درازی;
    أرجع (not ReadLEB_u32(&درازی, &i_بایتها, i_انتها)) and درازی == 0;
}

#پایان_اگر // d_m3HasCompactImports


#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching

M3_نتیجه Parse_Section_برچسب (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 رقم_برچسبها;
_   (ReadLEB_u32(&رقم_برچسبها, &i_بایتها, i_انتها));                                       m3log (parse, "** Tag [%d]", رقم_برچسبها);

    _throwif("too many tags", رقم_برچسبها > d_m3_Max_Sane_برچسبها_شمار);

    لكل (u32 i = 0; i < رقم_برچسبها; ++i) {
        u32 type_شاخص;
_       (Read_برچسب_Type(io_module, &type_شاخص, &i_بایتها, i_انتها));                      m3log (parse, "    tag: [%d] type: %d", i, type_شاخص);

_       (Module_جمع_کن_برچسب(io_module, عدم, io_module->funcTypes[type_شاخص], false));
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch: أرجع نتیجه;
}

#پایان_اگر // d_m3HasExceptionHandling


M3_نتیجه ParseSection_Import (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    M3ImportInfo import = { عدم, عدم };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 رقم_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 رقم_Entries;
_   (ReadLEB_u32(&رقم_Entries, &i_بایتها, i_انتها));                                    m3log (parse, "** Import [%d]", رقم_Entries);

    _throwif("too many imports", رقم_Entries > d_m3_Max_Sane_Imports_شمار);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->رقم_Functions + رقم_Entries));

    لكل (u32 i = 0; i < رقم_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_بایتها, i_انتها));

#إذا d_m3HasCompactImports
        بایتها_t فیلد_شروع = i_بایتها;
#پایان_اگر
_       (Read_utf8(&import.فیلد_Utf8, &i_بایتها, i_انتها));

#إذا d_m3HasCompactImports
        u8 compact = (i_بایتها < i_انتها) ? *i_بایتها : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        إذا ((compact == d_compact_Imports_در_هر_قلم_Type or compact == d_compactImports_sharedType) and
            Is_خالی_Name(فیلد_شروع, i_انتها)) {
            ++i_بایتها;

            m3_Free(import.فیلد_Utf8);         // it only marked the compact form
            import.فیلد_Utf8 = عدم;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            إذا (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_بایتها, i_انتها));
            }

            u32 رقم_اقلام;
_           (ReadLEB_u32(&رقم_اقلام, &i_بایتها, i_انتها));                              m3log (parse, "    compact: %d import(s) of '%s'", رقم_اقلام, import.moduleUtf8);

            _throwif("too many imports", (u64)رقم_Imports + رقم_اقلام > d_m3_Max_Sane_Imports_شمار);
            رقم_Imports += رقم_اقلام;

_           (Module_PreallocFunctions(io_module, io_module->رقم_Functions + رقم_اقلام));

            لكل (u32 j = 0; j < رقم_اقلام; ++j) {
                M3ImportInfo قلم = { عدم, عدم };

                قلم.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, طول_رشته(import.moduleUtf8) + 1);

                إذا (قلم.moduleUtf8) {
                    نتیجه = Read_utf8(&قلم.فیلد_Utf8, &i_بایتها, i_انتها);

                    // the shared form reuses the externtype read above
                    إذا (not نتیجه and compact == d_compact_Imports_در_هر_قلم_Type) {
                        نتیجه = ReadImportDesc(io_module, &desc, &i_بایتها, i_انتها);
                    }

                    إذا (not نتیجه) {
                        نتیجه = ApplyImportDesc(io_module, &desc, &قلم);
                    }
                } وإلا {
                    نتیجه = m3Err_mallocFailed;
                }

                FreeImportInfo(&قلم);
                _throwif(نتیجه, نتیجه);
            }

            FreeImportInfo(&import);
            استمر;
        }
#پایان_اگر // d_m3HasCompactImports

        _throwif("too many imports", رقم_Imports >= d_m3_Max_Sane_Imports_شمار);
        ++رقم_Imports;

_       (ReadImportDesc(io_module, &desc, &i_بایتها, i_انتها));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    أرجع نتیجه;
}


M3_نتیجه ParseSection_Export (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه    نتیجه = m3Err_none;
    ثابت حرف* utf8   = عدم;

    u32 رقم_Exports;
_   (ReadLEB_u32(&رقم_Exports, &i_بایتها, i_انتها));                                    m3log (parse, "** Export [%d]", رقم_Exports);

    _throwif("too many exports", رقم_Exports > d_m3_Max_Sane_Exports_شمار);

    // the section order check lets this section through once, so this is
    // every export the module has
    إذا (رقم_Exports) {
        io_module->exports = m3_AllocArray(M3Export, رقم_Exports);
        _throwifnull(io_module->exports);
    }

    لكل (u32 i = 0; i < رقم_Exports; ++i) {
        u8  export_نوع;
        u32 index;
        u32 name_درازی;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            بایتها_t درازی_بایتها = i_بایتها;
_           (ReadLEB_u32(&name_درازی, &درازی_بایتها, i_انتها));
        }

_       (Read_utf8(&utf8, &i_بایتها, i_انتها));
_       (Read_u8(&export_نوع, &i_بایتها, i_انتها));
_       (ReadLEB_u32(&index, &i_بایتها, i_انتها));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_نوع, utf8);

#إذا d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        لكل (u32 j = 0; j < i; ++j) {
            ثابت M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_درازی == name_درازی and memcmp(other->name, utf8, name_درازی) == 0);
        }
#پایان_اگر

        إذا (export_نوع == d_external_نوع_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->رقم_Functions);
            IM3Function func = &(io_module->functions[index]);
            إذا (func->رقم_نامها < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_درازی + 1);
                _throwifnull(name);
                func->نامها[func->رقم_نامها++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } وإلا إذا (export_نوع == d_external_نوع_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->رقم_Globals);
        } وإلا إذا (export_نوع == d_external_نوع_حافظه) {
            _throwif(m3Err_wasmMalformed, index >= io_module->رقم_Memories);
        } وإلا إذا (export_نوع == d_external_نوع_جدول) {
            _throwif(m3Err_wasmMalformed, index >= io_module->رقم_Tables);
        }
#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
        وإلا إذا (export_نوع == d_external_نوع_برچسب) {
            _throwif(m3Err_wasmMalformed, index >= io_module->رقم_برچسبها);
        }
#پایان_اگر

        M3Export* مدخل   = &io_module->exports[io_module->رقم_Exports++];
        مدخل->name       = utf8;
        مدخل->name_درازی = name_درازی;
        مدخل->index      = index;
        مدخل->نوع       = export_نوع;

        utf8 = عدم; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

_catch:
    m3_Free(utf8);
    أرجع نتیجه;
}


M3_نتیجه Parse_Section_شروع (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 شروع_Func_شاخص;
_   (ReadLEB_u32(&شروع_Func_شاخص, &i_بایتها, i_انتها));                                  m3log (parse, "** Start Function: %d", شروع_Func_شاخص);

    إذا (شروع_Func_شاخص < io_module->رقم_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[شروع_Func_شاخص];
        إذا (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->رقم_آرگومانها != 0 || func->funcType->رقم_Rets != 0);
        }

        io_module->شروع_Function = شروع_Func_شاخص;
    } وإلا {
        نتیجه = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch: أرجع نتیجه;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_نتیجه Parse_Init_عبارت_Typed (M3Module* io_module, بایتها_t* io_بایتها, cbytes_t i_انتها, m3type_t* o_type)
{
    M3_نتیجه نتیجه = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#إذا defined(d_m3PreferStaticAlloc)
    ساكن M3Compilation compilation;
#وإلا
    M3Compilation compilation;
#پایان_اگر
    compilation = (M3Compilation){ .runtime = عدم, .module = io_module, .wasm = *io_بایتها, .wasm_انتها = i_انتها, .is_Init_عبارت = true };

    نتیجه = Compile_بلوک_Statements(&compilation);

    *io_بایتها = compilation.wasm;

    إذا (o_type) {
        *o_type = compilation.پشته_شاخص ? compilation.type_پشته[compilation.پشته_شاخص - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    أرجع نتیجه;
}


M3_نتیجه Parse_Init_عبارت (M3Module* io_module, بایتها_t* io_بایتها, cbytes_t i_انتها)
{
    أرجع Parse_Init_عبارت_Typed(io_module, io_بایتها, i_انتها, عدم);
}


M3_نتیجه ParseSection_Element (IM3Module io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32     رقم_Segments;
    بایتها_t pos;
_   (ReadLEB_u32(&رقم_Segments, &i_بایتها, i_انتها));                            m3log (parse, "** Element [%d]", رقم_Segments);

    _throwif("too many element segments", رقم_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, رقم_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->رقم_Element_Segments = رقم_Segments;
    io_module->element_Section_انتها  = i_انتها;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_بایتها;
    لكل (u32 i = 0; i < رقم_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_انتها));
#إذا d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#وإلا
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_فعال);
#پایان_اگر

        segment->نما   = flags & 0x3;
        segment->is_عبارت = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        إذا (segment->نما == c_m3_Elem_فعال_Idx) {
_           (ReadLEB_u32(&segment->جدول_شاخص, &pos, i_انتها));
        }

        bool is_فعال = (segment->نما == c_m3_Elem_فعال or segment->نما == c_m3_Elem_فعال_Idx);

        إذا (is_فعال) {
            _throwif(m3Err_wasmMalformed, segment->جدول_شاخص >= io_module->رقم_Tables);

            segment->init_عبارت = pos;
_           (Parse_Init_عبارت(io_module, &pos, i_انتها));
            segment->init_عبارت_اندازه = (u32)(pos - segment->init_عبارت);
            _throwif(m3_Err_wasm_Missing_Init_عبارت, segment->init_عبارت_اندازه <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        إذا (segment->نما != c_m3_Elem_فعال) {
            إذا (segment->is_عبارت) {
_               (Parse_مقدار_Type(io_module, &segment->type, &pos, i_انتها));
                _throwif(m3Err_wasmMalformed, not IsRefType(پایه_Type_از(segment->type)));
            } وإلا {
                u8 elem_نوع;
_               (Read_u8(&elem_نوع, &pos, i_انتها));
                _throwif(m3Err_wasmMalformed, elem_نوع != 0x00);   // funcref
            }
        }

        إذا (is_فعال) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->جدول_شاخص]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->رقم_Elements, &pos, i_انتها));
        _throwif("table overflow", segment->رقم_Elements > d_m3_Max_Sane_جدول_اندازه);

        segment->elements = pos;

        لكل (u32 e = 0; e < segment->رقم_Elements; ++e) {
            إذا (segment->is_عبارت) {
_               (Parse_Init_عبارت(io_module, &pos, i_انتها))
            } وإلا {
                u32 func_شاخص;
_               (ReadLEB_u32(&func_شاخص, &pos, i_انتها));
                _throwif("function index out of range", func_شاخص >= io_module->رقم_Functions);
_               (Module_DeclareFunction(io_module, func_شاخص));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_انتها);           // section size mismatch

    _catch: أرجع نتیجه;
}


M3_نتیجه Parse_Section_کد (M3Module* io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه;

    u32 رقم_Functions;
_   (ReadLEB_u32(&رقم_Functions, &i_بایتها, i_انتها));                                  m3log (parse, "** Code [%d]", رقم_Functions);

    إذا (رقم_Functions != io_module->رقم_Functions - io_module->رقم_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    لكل (u32 f = 0; f < رقم_Functions; ++f) {
        ثابت u8* شروع = i_بایتها;

        u32 اندازه;
_       (ReadLEB_u32(&اندازه, &i_بایتها, i_انتها));

        إذا (اندازه) {
            i_بایتها += اندازه;

            إذا (i_بایتها <= i_انتها) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->رقم_Func_Imports);

                func->module  = io_module;
                func->wasm    = شروع;
                func->wasm_انتها = i_بایتها;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } وإلا {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    إذا (not نتیجه and i_بایتها != i_انتها) {
        نتیجه = m3Err_wasmSectionUnderrun;
    }

    أرجع نتیجه;
}


M3_نتیجه Parse_Section_داده (M3Module* io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 رقم_داده_Segments;
_   (ReadLEB_u32(&رقم_داده_Segments, &i_بایتها, i_انتها));                               m3log (parse, "** Data [%d]", رقم_داده_Segments);

    _throwif("too many data segments", رقم_داده_Segments > d_m3_Max_Sane_داده_Segments);

    io_module->داده_Segments = m3_AllocArray(M3_داده_Segment, رقم_داده_Segments);
    _throwifnull(io_module->داده_Segments);
    io_module->رقم_داده_Segments = رقم_داده_Segments;

    لكل (u32 i = 0; i < رقم_داده_Segments; ++i) {
        M3_داده_Segment* segment = &io_module->داده_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_بایتها, i_انتها));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        إذا (flags == 2) {
_           (ReadLEB_u32(&segment->حافظه_Region, &i_بایتها, i_انتها));
        }

        إذا (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#إذا !d_m3_Has_Multi_حافظه
            _throwif(m3Err_wasmMalformed, segment->حافظه_Region != 0);
#پایان_اگر
            _throwif(m3Err_wasmMalformed, segment->حافظه_Region >= io_module->رقم_Memories);

            segment->init_عبارت = i_بایتها;
_           (Parse_Init_عبارت(io_module, &i_بایتها, i_انتها));
            segment->init_عبارت_اندازه = (u32)(i_بایتها - segment->init_عبارت);

            _throwif(m3_Err_wasm_Missing_Init_عبارت, segment->init_عبارت_اندازه <= 1);
        }

_       (ReadLEB_u32(&segment->اندازه, &i_بایتها, i_انتها));
        segment->داده = i_بایتها;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->حافظه_Region, segment->init_عبارت_اندازه, segment->اندازه);
        i_بایتها += segment->اندازه;

        _throwif("data segment underflow", i_بایتها > i_انتها);
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch:

    أرجع نتیجه;
}


M3_نتیجه Parse_Section_داده_شمار (M3Module* io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

_   (ReadLEB_u32(&io_module->داده_شمار, &i_بایتها, i_انتها));                       m3log (parse, "** DataCount [%d]", io_module->داده_شمار);

    io_module->has_داده_شمار = true;

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch: أرجع نتیجه;
}


M3_نتیجه Parse_Section_حافظه (M3Module* io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 رقم_Memories;
_   (ReadLEB_u32(&رقم_Memories, &i_بایتها, i_انتها));                                m3log (parse, "** Memory [%d]", رقم_Memories);

#إذا d_m3_Has_Multi_حافظه
    _throwif(m3_Err_too_Many_حافظه_Sections,
             (u64)رقم_Memories + io_module->رقم_Memories > d_m3_Max_Sane_Memories_شمار);
#وإلا
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_حافظه_Sections, رقم_Memories > 1);
    _throwif(m3_Err_too_Many_حافظه_Sections, (u64)رقم_Memories + io_module->رقم_Memories > 1);
#پایان_اگر

    لكل (u32 i = 0; i < رقم_Memories; ++i) {
        M3_حافظه_Info info;
_       (Parse_Type_حافظه(&info, &i_بایتها, i_انتها));
_       (Module_جمع_کن_حافظه(io_module, عدم, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch: أرجع نتیجه;
}


M3_نتیجه ParseSection_Global (M3Module* io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    u32 رقم_Globals;
_   (ReadLEB_u32(&رقم_Globals, &i_بایتها, i_انتها));                                    m3log (parse, "** Global [%d]", رقم_Globals);

    _throwif("too many globals", رقم_Globals > d_m3_Max_Sane_Globals_شمار);

    لكل (u32 i = 0; i < رقم_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_مقدار_Type(io_module, &type, &i_بایتها, i_انتها));
_       (ReadLEB_u7(&isMutable, &i_بایتها, i_انتها));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [پایه_Type_از(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_جمع_کن_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_عبارت = i_بایتها;
_       (Parse_Init_عبارت(io_module, &i_بایتها, i_انتها));
        global->init_عبارت_اندازه = (u32)(i_بایتها - global->init_عبارت);

        _throwif(m3_Err_wasm_Missing_Init_عبارت, global->init_عبارت_اندازه <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_بایتها != i_انتها);      // section size mismatch

    _catch: أرجع نتیجه;
}


M3_نتیجه ParseSection_Name (M3Module* io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    cstr_t name = عدم;

    طالما (i_بایتها < i_انتها) {
        u8  nameType;
        u32 payload_درازی;

_       (ReadLEB_u7(&nameType, &i_بایتها, i_انتها));
_       (ReadLEB_u32(&payload_درازی, &i_بایتها, i_انتها));

        بایتها_t شروع = i_بایتها;
        _throwif(m3Err_wasmSectionOverrun, payload_درازی > (size_t)(i_انتها - شروع));

        إذا (nameType == 1) {
            u32 رقم_نامها;
_           (ReadLEB_u32(&رقم_نامها, &i_بایتها, i_انتها));

            _throwif("too many names", رقم_نامها > d_m3_Max_Sane_Functions_شمار);

            لكل (u32 i = 0; i < رقم_نامها; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_بایتها, i_انتها));
_               (Read_utf8(&name, &i_بایتها, i_انتها));

                إذا (index < io_module->رقم_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    إذا (func->رقم_نامها == 0) {
                        func->نامها[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->رقم_نامها = 1;

                        name = عدم; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_بایتها = شروع + payload_درازی;
    }

    _catch: أرجع نتیجه;
}


M3_نتیجه ParseSection_Custom (M3Module* io_module, بایتها_t i_بایتها, cbytes_t i_انتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    cstr_t name = عدم;
_   (Read_utf8(&name, &i_بایتها, i_انتها));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    إذا (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_بایتها, i_انتها));
#إذا d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } وإلا إذا (strcmp(name, "snapshot") == 0 || (طول_رشته(name) > 9 && مقایسه_رشته(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, طول_رشته(snapName) + 1);
        إذا (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->داده           = i_بایتها;
        snap->اندازه           = (u32)(i_انتها - i_بایتها);
        snap->بعدی           = io_module->snapshots;
        io_module->snapshots = snap;
#پایان_اگر
    } وإلا إذا (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_بایتها, i_انتها));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    أرجع نتیجه;
}


M3_نتیجه ParseModuleSection (M3Module* o_module, u8 i_sectionType, بایتها_t i_بایتها, u32 i_رقم_بایتها)
{
    M3_نتیجه نتیجه = m3Err_none;

    تعريفنوع M3_نتیجه (*M3Parser)(M3Module*, بایتها_t, cbytes_t);

    ساكن M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_جدول,        // 4
        Parse_Section_حافظه,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_شروع,     // 8
        ParseSection_Element,   // 9
        Parse_Section_کد,      // 10
        Parse_Section_داده,      // 11
        Parse_Section_داده_شمار, // 12
#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
        Parse_Section_برچسب,       // 13
#پایان_اگر
    };

    M3Parser parser = عدم;

    إذا (i_sectionType < M3_شمار_از(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    إذا (parser) {
        cbytes_t انتها = i_بایتها + i_رقم_بایتها;
        نتیجه       = parser(o_module, i_بایتها, انتها);
    } وإلا {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    أرجع نتیجه;
}


M3_نتیجه m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_بایتها, u32 i_رقم_بایتها)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_رقم_بایتها);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_رقم_بایتها);
    module->شروع_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ثابت u8* pos = i_بایتها;
    ثابت u8* انتها = pos + i_رقم_بایتها;

    module->wasm_شروع = pos;
    module->wasm_انتها   = انتها;

    u32 magic, نسخه;
_   (Read_u32(&magic, &pos, انتها));
_   (Read_u32(&نسخه, &pos, انتها));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_نسخه, نسخه != 1);

#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
    // the tag section sits between memory (5) and global (6)
    ساكن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#وإلا
    ساكن ثابت u8 sections_ترتیب[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#پایان_اگر
    u8 expectedSection = 0;

    طالما (pos < انتها) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, انتها));

        إذا (section != 0) {
            // Ensure sections appear only once and in order
            طالما (sections_ترتیب[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_شمار_از(sections_ترتیب) - 1);
            }
        }

        u32 section_درازی;
_       (ReadLEB_u32(&section_درازی, &pos, انتها));
        _throwif(m3Err_wasmMalformed, pos + section_درازی > انتها);

_       (ParseModuleSection(module, section, pos, section_درازی));

        pos += section_درازی;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    إذا (module->رقم_Functions > module->رقم_Func_Imports) {
        IM3Function اولی_Non_Import = &module->functions[module->رقم_Func_Imports];
        _throwif(m3Err_wasmMalformed, اولی_Non_Import->wasm == عدم);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_داده_شمار and module->داده_شمار != module->رقم_داده_Segments);

} _catch:

    إذا (نتیجه) {
        m3_FreeModule(module);
        module = عدم;
    }

    *o_module = module;

    أرجع نتیجه;
}
