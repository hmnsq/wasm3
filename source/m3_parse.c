//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_compile.h"
#అన్తర్భావ "m3_exception.h"
#అన్తర్భావ "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#ఒకవేళ d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_ఫలితం Parse_కుప్ప_Type (IM3Module i_module, m3type_t* o_కుప్ప_బిట్లు, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    i64 కుప్ప;
_   (ReadLebSigned(&కుప్ప, 33, io_బైట్లు, i_చివర));

    ఒకవేళ (కుప్ప == -d_waType_funcref) {
        *o_కుప్ప_బిట్లు = d_m3_Type_కుప్ప_Abstract;
    } లేకపోతే ఒకవేళ (కుప్ప == -d_waType_externref) {
        *o_కుప్ప_బిట్లు = d_m3Type_refExtern | d_m3_Type_కుప్ప_Abstract;
#  ఒకవేళ d_m3_Has_దొంతర_Switching
    } లేకపోతే ఒకవేళ (కుప్ప == -d_wa_Type_కుప్ప_cont) {
        *o_కుప్ప_బిట్లు = d_m3Type_refCont | d_m3_Type_కుప్ప_Abstract;
    } లేకపోతే ఒకవేళ (కుప్ప == -d_wa_Type_కుప్ప_nocont) {
        *o_కుప్ప_బిట్లు = d_m3Type_refCont | d_m3_Type_కుప్ప_None;
#  endif
    } లేకపోతే ఒకవేళ (కుప్ప >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) కుప్ప >= i_module->అంకె_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[కుప్ప];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_కుప్ప_బిట్లు = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_సూచీ;
    } లేకపోతే {
        _throw(m3Err_invalidTypeId);
    }

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_ఫలితం Parse_విలువ_Type (IM3Module i_module, m3type_t* o_type, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_బైట్లు >= i_చివర);

#ఒకవేళ d_m3HasTypedRefs
    ఒకవేళ (**io_బైట్లు == d_waEncode_ref or **io_బైట్లు == d_waEncode_refNull) {
        u8 lead = *(*io_బైట్లు)++;

        m3type_t కుప్ప_బిట్లు = d_m3_Type_కుప్ప_Abstract;
_       (Parse_కుప్ప_Type(i_module, &కుప్ప_బిట్లు, io_బైట్లు, i_చివర));

        *o_type = d_m3Type_ref | కుప్ప_బిట్లు | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        తిరిగి ఫలితం;
    }
#అయితే_ముగింపు

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_బైట్లు, i_చివర));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: తిరిగి ఫలితం;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
స్థిర
M3_ఫలితం Read_Type_పట్టిక_Type (IM3Module i_module, M3_పట్టిక_Info* o_info, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_కొలత = 0, max_కొలత = 0;

    // the width of the limits, and of every index into the table
    u32 addr_బిట్లు = 32;

_   (Parse_విలువ_Type(i_module, &elemType, io_బైట్లు, i_చివర));
#ఒకవేళ d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(పునాది_Type_యొక్క(elemType)));
#లేకపోతే
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#అయితే_ముగింపు

_   (ReadLEB_u7(&flag, io_బైట్లు, i_చివర));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#ఒకవేళ d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#లేకపోతే
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#అయితే_ముగింపు

    ఒకవేళ (flag & 0x04u) {
        addr_బిట్లు = 64;
    }

_   (ReadLebUnsigned(&init_కొలత, addr_బిట్లు, io_బైట్లు, i_చివర));
    _throwif("table overflow", init_కొలత > d_m3_Max_Sane_పట్టిక_కొలత);

    ఒకవేళ (flag & 0x01u) {
_       (ReadLebUnsigned(&max_కొలత, addr_బిట్లు, io_బైట్లు, i_చివర));
        _throwif(m3Err_wasmMalformed, max_కొలత < init_కొలత);
        _throwif("table overflow", max_కొలత > d_m3_Max_Sane_పట్టిక_కొలత);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_కొలత  = (u32)init_కొలత;
    o_info->max_కొలత   = (u32)max_కొలత;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    తిరిగి ఫలితం;
}


స్థిర
M3_ఫలితం Parse_Type_పట్టిక_Type (IM3Module io_module, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3_పట్టిక_Info info;

_   (Read_Type_పట్టిక_Type(io_module, &info, io_బైట్లు, i_చివర));
_   (Module_కూడు_పట్టిక(io_module, ఖాళీ, &info, false /* isImport */));

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Init_సమాసం_Typed (M3Module* io_module, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర, m3type_t* o_type);

M3_ఫలితం Parse_Type_పట్టిక (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 అంకె_Tables;
_   (ReadLEB_u32(&అంకె_Tables, &i_బైట్లు, i_చివర));                          m3log (parse, "** Table [%d]", అంకె_Tables);

    _throwif("too many tables", (u64)అంకె_Tables + io_module->అంకె_Tables > d_m3_Max_Sane_పట్టిక_లెక్క);

    కొరకు (u32 i = 0; i < అంకె_Tables; ++i) {
#ఒకవేళ d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_సమాసం = (i_బైట్లు < i_చివర) and (*i_బైట్లు == 0x40);
        u32  పట్టిక_సూచీ  = io_module->అంకె_Tables;

        ఒకవేళ (has_Init_సమాసం) {
            u8 reserved;
            ++i_బైట్లు;
_           (Read_u8(&reserved, &i_బైట్లు, i_చివర));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_పట్టిక_Type(io_module, &i_బైట్లు, i_చివర));

        ఒకవేళ (has_Init_సమాసం) {
            IM3_పట్టిక పట్టిక_2 = io_module->tables[పట్టిక_సూచీ];

            m3type_t initType;

            పట్టిక_2->init_సమాసం = i_బైట్లు;
_           (Parse_Init_సమాసం_Typed(io_module, &i_బైట్లు, i_చివర, &initType));
            పట్టిక_2->init_సమాసం_కొలత = (u32)(i_బైట్లు - పట్టిక_2->init_సమాసం);
            _throwif(m3_Err_wasm_Missing_Init_సమాసం, పట్టిక_2->init_సమాసం_కొలత <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_యొక్క(initType, పట్టిక_2->type));
        } లేకపోతే {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[పట్టిక_సూచీ]->type));
        }
#లేకపోతే
_       (Parse_Type_పట్టిక_Type(io_module, &i_బైట్లు, i_చివర));
#అయితే_ముగింపు
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Type_జ్ఞాపకం (M3_జ్ఞాపకం_Info* o_జ్ఞాపకం, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_పుట_కొలత = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_బిట్లు = 32;

_   (ReadLEB_u7(&flag, io_బైట్లు, i_చివర));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 తెలిసిన = 0x09u;
#ఒకవేళ d_m3HasAtomics
        తెలిసిన |= 0x02u;
#అయితే_ముగింపు
#ఒకవేళ d_m3HasMemory64
        తెలిసిన |= 0x04u;
#అయితే_ముగింపు
        _throwif(m3Err_wasmMalformed, flag & ~తెలిసిన);
    }

    o_జ్ఞాపకం->isShared = (flag & (1u << 1)) != 0;

    o_జ్ఞాపకం->isMemory64 = (flag & (1u << 2)) != 0;
    ఒకవేళ (o_జ్ఞాపకం->isMemory64) {
        addr_బిట్లు = 64;
    }

_   (ReadLebUnsigned(&o_జ్ఞాపకం->init_పుటలు, addr_బిట్లు, io_బైట్లు, i_చివర));

    o_జ్ఞాపకం->max_పుటలు = 0;
    o_జ్ఞాపకం->hasMax   = (flag & (1u << 0)) != 0;
    ఒకవేళ (o_జ్ఞాపకం->hasMax) {
_       (ReadLebUnsigned(&o_జ్ఞాపకం->max_పుటలు, addr_బిట్లు, io_బైట్లు, i_చివర));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_జ్ఞాపకం->max_పుటలు < o_జ్ఞాపకం->init_పుటలు);
    }

    _throwif("shared memory must have maximum", o_జ్ఞాపకం->isShared and not o_జ్ఞాపకం->hasMax);

    o_జ్ఞాపకం->పుట_కొలత = 0;
    ఒకవేళ (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_పుట_కొలత, io_బైట్లు, i_చివర));

        _throwif("invalid custom page size", log_పుట_కొలత > 16);

        o_జ్ఞాపకం->పుట_కొలత = 1u << log_పుట_కొలత;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_పుటలు_Allowed = (log_పుట_కొలత or addr_బిట్లు < 64)
                                ? (UINT64_C(1) << (addr_బిట్లు - log_పుట_కొలత))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_జ్ఞాపకం->init_పుటలు > max_పుటలు_Allowed);
        ఒకవేళ (o_జ్ఞాపకం->hasMax) {
            _throwif(m3Err_wasmMalformed, o_జ్ఞాపకం->max_పుటలు > max_పుటలు_Allowed);
        }
    }

    _catch: తిరిగి ఫలితం;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
స్థిర
M3_ఫలితం ParseFuncType (IM3Module io_module, IM3FuncType* o_type, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    IM3FuncType ftype = ఖాళీ;
_try {
    u32 అంకె_వాదనలు;
_   (ReadLEB_u32(&అంకె_వాదనలు, io_బైట్లు, i_చివర));

    _throwif(m3_Err_too_Many_వాదనలు_Rets, అంకె_వాదనలు > d_m3_Max_Sane_Function_Arg_Ret_లెక్క);
#ఒకవేళ M3_HAS_VLA
    m3type_t argTypes[అంకె_వాదనలు + 1]; // make ubsan happy
#లేకపోతే
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_లెక్క];
#అయితే_ముగింపు
    కొరకు (u32 a = 0; a < అంకె_వాదనలు; ++a) {
_       (Parse_విలువ_Type(io_module, &argTypes[a], io_బైట్లు, i_చివర));
    }

    u32 అంకె_Rets;
_   (ReadLEB_u32(&అంకె_Rets, io_బైట్లు, i_చివర));
    _throwif(m3_Err_too_Many_వాదనలు_Rets, (u64)(అంకె_Rets) + అంకె_వాదనలు > d_m3_Max_Sane_Function_Arg_Ret_లెక్క);

_   (AllocFuncType(&ftype, అంకె_Rets + అంకె_వాదనలు));
    ftype->అంకె_వాదనలు = అంకె_వాదనలు;
    ftype->అంకె_Rets = అంకె_Rets;

    కొరకు (u32 r = 0; r < అంకె_Rets; ++r) {
_       (Parse_విలువ_Type(io_module, &ftype->types[r], io_బైట్లు, i_చివర));
    }
    జ్ఞాపకం_నకలు(ftype->types + అంకె_Rets, argTypes, అంకె_వాదనలు * పరిమాణం(m3type_t));

    *o_type = ftype;
    ftype   = ఖాళీ;

} _catch:
    m3_Free(ftype);
    తిరిగి ఫలితం;
}


#ఒకవేళ d_m3_Has_దొంతర_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
స్థిర
M3_ఫలితం ParseContType (IM3Module io_module, IM3FuncType* o_type, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    IM3FuncType ftype = ఖాళీ;
_try {
    i64 ft_సూచీ;
_   (ReadLebSigned(&ft_సూచీ, 33, io_బైట్లు, i_చివర));
    _throwif(m3Err_wasmMalformed, ft_సూచీ < 0 or (u64) ft_సూచీ >= io_module->అంకె_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_సూచీ];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 మొత్తం = underlying->అంకె_Rets + underlying->అంకె_వాదనలు;
_   (AllocFuncType(&ftype, మొత్తం));
    ftype->అంకె_Rets        = underlying->అంకె_Rets;
    ftype->అంకె_వాదనలు        = underlying->అంకె_వాదనలు;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    ఒకవేళ (మొత్తం) {
        జ్ఞాపకం_నకలు(ftype->types, underlying->types, మొత్తం * పరిమాణం(m3type_t));
    }

    *o_type = ftype;
    ftype   = ఖాళీ;

} _catch:
    m3_Free(ftype);
    తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasStackSwitching


స్థిర
M3_ఫలితం ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
#ఒకవేళ d_m3_Has_దొంతర_Switching
    ఒకవేళ (i_form == -35) {
        తిరిగి ParseContType(io_module, o_type, io_బైట్లు, i_చివర);
    }
#అయితే_ముగింపు
    ఒకవేళ (i_form != -32) { // for Wasm MVP
        తిరిగి m3Err_wasmMalformed;
    }

    తిరిగి ParseFuncType(io_module, o_type, io_బైట్లు, i_చివర);
}


#ఒకవేళ d_m3_Has_దొంతర_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
స్థిర
M3_ఫలితం SkipTypeDefinition (i8 i_form, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
_try {
    ఒకవేళ (i_form == -35) {
        i64 ft_సూచీ;
_       (ReadLebSigned(&ft_సూచీ, 33, io_బైట్లు, i_చివర));
        తిరిగి ఫలితం;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    కొరకు (u32 half = 0; half < 2; ++half) {
        u32 లెక్క;
_       (ReadLEB_u32(&లెక్క, io_బైట్లు, i_చివర));
        _throwif(m3_Err_too_Many_వాదనలు_Rets, లెక్క > d_m3_Max_Sane_Function_Arg_Ret_లెక్క);

        కొరకు (u32 v = 0; v < లెక్క; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_బైట్లు, i_చివర));

            ఒకవేళ (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 కుప్ప;
_               (ReadLebSigned(&కుప్ప, 33, io_బైట్లు, i_చివర));
            }
        }
    }

} _catch: తిరిగి ఫలితం;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
స్థిర
M3_ఫలితం Parse_Rec_సమూహం (IM3Module io_module, u32 i_లెక్క, u32 i_మొదటి_సూచీ,
                        బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    IM3FuncType* placeholders = ఖాళీ;
_try {
    బైట్లు_t సమూహం_ప్రారంభం = *io_బైట్లు;

    placeholders = m3_AllocArray(IM3FuncType, i_లెక్క);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    కొరకు (u32 j = 0; j < i_లెక్క; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_బైట్లు, i_చివర));
_       (SkipTypeDefinition(form, io_బైట్లు, i_చివర));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 మొదటి_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_లెక్క, &మొదటి_Canonical));

    కొరకు (u32 j = 0; j < i_లెక్క; ++j) {
        placeholders[j]->canonical_సూచీ        = (u16)(మొదటి_Canonical + j);
        io_module->funcTypes[i_మొదటి_సూచీ + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    కొరకు (u32 pass = 0; pass < 2; ++pass) {
        బైట్లు_t బైట్లు = సమూహం_ప్రారంభం;

        కొరకు (u32 j = 0; j < i_లెక్క; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &బైట్లు, i_చివర));

            ఒకవేళ ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &బైట్లు, i_చివర));
                కొనసాగు;
            }

            IM3FuncType ftype = ఖాళీ;
_           (ParseTypeDefinition(io_module, form, &ftype, &బైట్లు, i_చివర));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_సూచీ);

            m3_Free(placeholders[j]);
            placeholders[j]                        = ఖాళీ;
            io_module->funcTypes[i_మొదటి_సూచీ + j] = ftype;
        }

        ఒకవేళ (pass == 1) {
            *io_బైట్లు = బైట్లు;
        }
    }

} _catch:
    ఒకవేళ (placeholders) {
        కొరకు (u32 j = 0; j < i_లెక్క; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasStackSwitching


M3_ఫలితం ParseSection_Type (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    IM3FuncType ftype = ఖాళీ;

_try {
    u32 అంకె_Types;
_   (ReadLEB_u32(&అంకె_Types, &i_బైట్లు, i_చివర));                                      m3log (parse, "** Type [%d]", అంకె_Types);

    _throwif("too many types", అంకె_Types > d_m3_Max_Sane_Types_లెక్క);

    ఒకవేళ (అంకె_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, అంకె_Types);
        _throwifnull(io_module->funcTypes);
        io_module->అంకె_Func_Types = అంకె_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#ఒకవేళ d_m3_Has_దొంతర_Switching
        u32 సామర్థ్యం = అంకె_Types;
#అయితే_ముగింపు
        u32 index = 0;

        కొరకు (u32 i = 0; i < అంకె_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_బైట్లు, i_చివర));

#ఒకవేళ d_m3_Has_దొంతర_Switching
            ఒకవేళ (form == -50) { // rec
                u32 లెక్క;
_               (ReadLEB_u32(&లెక్క, &i_బైట్లు, i_చివర));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + లెక్క + (అంకె_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_లెక్క);

                ఒకవేళ (needed > సామర్థ్యం) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, సామర్థ్యం);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    సామర్థ్యం                = grown;
                    io_module->అంకె_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                ఒకవేళ (లెక్క == 1) {
_                   (ReadLEB_i7(&form, &i_బైట్లు, i_చివర));
                } లేకపోతే {
_                   (Parse_Rec_సమూహం(io_module, లెక్క, index, &i_బైట్లు, i_చివర));
                    index += లెక్క;
                    కొనసాగు;
                }
            }
#అయితే_ముగింపు

_           (ParseTypeDefinition(io_module, form, &ftype, &i_బైట్లు, i_చివర));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_కూడు_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = ఖాళీ; // ownership transferred to environment
        }

        io_module->అంకె_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

} _catch:

    ఒకవేళ (ఫలితం) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->అంకె_Func_Types = 0;
    }

    తిరిగి ఫలితం;
}


M3_ఫలితం ParseSection_Function (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 అంకె_Functions;
_   (ReadLEB_u32(&అంకె_Functions, &i_బైట్లు, i_చివర));                                  m3log (parse, "** Function [%d]", అంకె_Functions);

    _throwif("too many functions", అంకె_Functions > d_m3_Max_Sane_Functions_లెక్క);

_   (Module_PreallocFunctions(io_module, io_module->అంకె_Functions + అంకె_Functions));

    కొరకు (u32 i = 0; i < అంకె_Functions; ++i) {
        u32 func_Type_సూచీ;
_       (ReadLEB_u32(&func_Type_సూచీ, &i_బైట్లు, i_చివర));

_       (Module_కూడు_Function(io_module, func_Type_సూచీ, ఖాళీ /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch: తిరిగి ఫలితం;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
రకనిర్వచనం నిర్మాణం M3ImportDesc {
    u8           రకం;
    u32          type_సూచీ;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_పట్టిక_Info  పట్టిక_2;
    M3_జ్ఞాపకం_Info జ్ఞాపకం;
} M3ImportDesc;


#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
స్థిర
M3_ఫలితం Read_ట్యాగ్_Type (IM3Module i_module, u32* o_type_సూచీ, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_బైట్లు, i_చివర));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_సూచీ, io_బైట్లు, i_చివర));
    _throwif(m3Err_unknownType, not i_module or *o_type_సూచీ >= i_module->అంకె_Func_Types);

#  ఒకవేళ !d_m3_Has_దొంతర_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_అంకె_Results(i_module->funcTypes[*o_type_సూచీ]) != 0);
#  endif

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
స్థిర
M3_ఫలితం ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (Read_u8(&o_desc->రకం, io_బైట్లు, i_చివర));

    ఎంపిక (o_desc->రకం) {
    స్థితి d_external_రకం_function:
_       (ReadLEB_u32(&o_desc->type_సూచీ, io_బైట్లు, i_చివర));
        ఆపు;

    స్థితి d_external_రకం_పట్టిక:
_       (Read_Type_పట్టిక_Type(i_module, &o_desc->పట్టిక_2, io_బైట్లు, i_చివర));
        ఆపు;

    స్థితి d_external_రకం_జ్ఞాపకం:
_       (Parse_Type_జ్ఞాపకం(&o_desc->జ్ఞాపకం, io_బైట్లు, i_చివర));
        ఆపు;

    స్థితి d_external_రకం_global:
_       (Parse_విలువ_Type(i_module, &o_desc->type, io_బైట్లు, i_చివర));
_       (ReadLEB_u7(&o_desc->isMutable, io_బైట్లు, i_చివర));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [పునాది_Type_యొక్క(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        ఆపు;

#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
    స్థితి d_external_రకం_ట్యాగ్:
_       (Read_ట్యాగ్_Type(i_module, &o_desc->type_సూచీ, io_బైట్లు, i_చివర));
        ఆపు;
#అయితే_ముగింపు

    అప్రమేయ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: తిరిగి ఫలితం;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
స్థిర
M3_ఫలితం ApplyImportDesc (IM3Module io_module, స్థిరరాశి M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3ImportInfo clearImport = { ఖాళీ, ఖాళీ };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->రకం, io_import->moduleUtf8, io_import->క్షేత్రం_Utf8);
    ఎంపిక (i_desc->రకం) {
    స్థితి d_external_రకం_function: {
_       (Module_కూడు_Function(io_module, i_desc->type_సూచీ, io_import))
        *io_import = clearImport;

        io_module->అంకె_Func_Imports++;
    } ఆపు;

    స్థితి d_external_రకం_పట్టిక: {
        IM3_పట్టిక పట్టిక_2;
_       (Module_కూడు_పట్టిక(io_module, &పట్టిక_2, &i_desc->పట్టిక_2, true /* isImport */));
        పట్టిక_2->import = *io_import;
        *io_import    = clearImport;
    } ఆపు;

    స్థితి d_external_రకం_జ్ఞాపకం: {
        IM3_జ్ఞాపకం జ్ఞాపకం;

#ఒకవేళ d_m3_Has_Multi_జ్ఞాపకం
        _throwif(m3_Err_too_Many_జ్ఞాపకం_Sections, io_module->అంకె_Memories >= d_m3_Max_Sane_Memories_లెక్క);
#లేకపోతే
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_జ్ఞాపకం_Sections, io_module->అంకె_Memories >= 1);
#అయితే_ముగింపు

_       (Module_కూడు_జ్ఞాపకం(io_module, &జ్ఞాపకం, &i_desc->జ్ఞాపకం, true /* isImport */));
        జ్ఞాపకం->import = *io_import;
        *io_import     = clearImport;
    } ఆపు;

    స్థితి d_external_రకం_global: {
        IM3Global global;
_       (Module_కూడు_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } ఆపు;

#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
    స్థితి d_external_రకం_ట్యాగ్: {
        IM3_ట్యాగ్ ట్యాగ్;
_       (Module_కూడు_ట్యాగ్(io_module, &ట్యాగ్, io_module->funcTypes[i_desc->type_సూచీ], true /* isImport */));
        ట్యాగ్->import = *io_import;
        *io_import  = clearImport;
    } ఆపు;
#అయితే_ముగింపు

    అప్రమేయ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: తిరిగి ఫలితం;
}


#ఒకవేళ d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
స్థిర
bool Is_ఖాళీలు_Name (బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    u32 నిడివి;
    తిరిగి (not ReadLEB_u32(&నిడివి, &i_బైట్లు, i_చివర)) and నిడివి == 0;
}

#అయితే_ముగింపు // d_m3HasCompactImports


#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching

M3_ఫలితం Parse_Section_ట్యాగ్ (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 అంకె_ట్యాగులు;
_   (ReadLEB_u32(&అంకె_ట్యాగులు, &i_బైట్లు, i_చివర));                                       m3log (parse, "** Tag [%d]", అంకె_ట్యాగులు);

    _throwif("too many tags", అంకె_ట్యాగులు > d_m3_Max_Sane_ట్యాగులు_లెక్క);

    కొరకు (u32 i = 0; i < అంకె_ట్యాగులు; ++i) {
        u32 type_సూచీ;
_       (Read_ట్యాగ్_Type(io_module, &type_సూచీ, &i_బైట్లు, i_చివర));                      m3log (parse, "    tag: [%d] type: %d", i, type_సూచీ);

_       (Module_కూడు_ట్యాగ్(io_module, ఖాళీ, io_module->funcTypes[type_సూచీ], false));
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch: తిరిగి ఫలితం;
}

#అయితే_ముగింపు // d_m3HasExceptionHandling


M3_ఫలితం ParseSection_Import (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    M3ImportInfo import = { ఖాళీ, ఖాళీ };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 అంకె_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 అంకె_Entries;
_   (ReadLEB_u32(&అంకె_Entries, &i_బైట్లు, i_చివర));                                    m3log (parse, "** Import [%d]", అంకె_Entries);

    _throwif("too many imports", అంకె_Entries > d_m3_Max_Sane_Imports_లెక్క);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->అంకె_Functions + అంకె_Entries));

    కొరకు (u32 i = 0; i < అంకె_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_బైట్లు, i_చివర));

#ఒకవేళ d_m3HasCompactImports
        బైట్లు_t క్షేత్రం_ప్రారంభం = i_బైట్లు;
#అయితే_ముగింపు
_       (Read_utf8(&import.క్షేత్రం_Utf8, &i_బైట్లు, i_చివర));

#ఒకవేళ d_m3HasCompactImports
        u8 compact = (i_బైట్లు < i_చివర) ? *i_బైట్లు : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        ఒకవేళ ((compact == d_compact_Imports_చొప్పున_అంశం_Type or compact == d_compactImports_sharedType) and
            Is_ఖాళీలు_Name(క్షేత్రం_ప్రారంభం, i_చివర)) {
            ++i_బైట్లు;

            m3_Free(import.క్షేత్రం_Utf8);         // it only marked the compact form
            import.క్షేత్రం_Utf8 = ఖాళీ;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            ఒకవేళ (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_బైట్లు, i_చివర));
            }

            u32 అంకె_వస్తువులు;
_           (ReadLEB_u32(&అంకె_వస్తువులు, &i_బైట్లు, i_చివర));                              m3log (parse, "    compact: %d import(s) of '%s'", అంకె_వస్తువులు, import.moduleUtf8);

            _throwif("too many imports", (u64)అంకె_Imports + అంకె_వస్తువులు > d_m3_Max_Sane_Imports_లెక్క);
            అంకె_Imports += అంకె_వస్తువులు;

_           (Module_PreallocFunctions(io_module, io_module->అంకె_Functions + అంకె_వస్తువులు));

            కొరకు (u32 j = 0; j < అంకె_వస్తువులు; ++j) {
                M3ImportInfo అంశం = { ఖాళీ, ఖాళీ };

                అంశం.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, అక్షరశ్రేణి_పొడవు(import.moduleUtf8) + 1);

                ఒకవేళ (అంశం.moduleUtf8) {
                    ఫలితం = Read_utf8(&అంశం.క్షేత్రం_Utf8, &i_బైట్లు, i_చివర);

                    // the shared form reuses the externtype read above
                    ఒకవేళ (not ఫలితం and compact == d_compact_Imports_చొప్పున_అంశం_Type) {
                        ఫలితం = ReadImportDesc(io_module, &desc, &i_బైట్లు, i_చివర);
                    }

                    ఒకవేళ (not ఫలితం) {
                        ఫలితం = ApplyImportDesc(io_module, &desc, &అంశం);
                    }
                } లేకపోతే {
                    ఫలితం = m3Err_mallocFailed;
                }

                FreeImportInfo(&అంశం);
                _throwif(ఫలితం, ఫలితం);
            }

            FreeImportInfo(&import);
            కొనసాగు;
        }
#అయితే_ముగింపు // d_m3HasCompactImports

        _throwif("too many imports", అంకె_Imports >= d_m3_Max_Sane_Imports_లెక్క);
        ++అంకె_Imports;

_       (ReadImportDesc(io_module, &desc, &i_బైట్లు, i_చివర));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    తిరిగి ఫలితం;
}


M3_ఫలితం ParseSection_Export (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం    ఫలితం = m3Err_none;
    స్థిరరాశి అక్షరం* utf8   = ఖాళీ;

    u32 అంకె_Exports;
_   (ReadLEB_u32(&అంకె_Exports, &i_బైట్లు, i_చివర));                                    m3log (parse, "** Export [%d]", అంకె_Exports);

    _throwif("too many exports", అంకె_Exports > d_m3_Max_Sane_Exports_లెక్క);

    // the section order check lets this section through once, so this is
    // every export the module has
    ఒకవేళ (అంకె_Exports) {
        io_module->exports = m3_AllocArray(M3Export, అంకె_Exports);
        _throwifnull(io_module->exports);
    }

    కొరకు (u32 i = 0; i < అంకె_Exports; ++i) {
        u8  export_రకం;
        u32 index;
        u32 name_నిడివి;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            బైట్లు_t నిడివి_బైట్లు = i_బైట్లు;
_           (ReadLEB_u32(&name_నిడివి, &నిడివి_బైట్లు, i_చివర));
        }

_       (Read_utf8(&utf8, &i_బైట్లు, i_చివర));
_       (Read_u8(&export_రకం, &i_బైట్లు, i_చివర));
_       (ReadLEB_u32(&index, &i_బైట్లు, i_చివర));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_రకం, utf8);

#ఒకవేళ d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        కొరకు (u32 j = 0; j < i; ++j) {
            స్థిరరాశి M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_నిడివి == name_నిడివి and memcmp(other->name, utf8, name_నిడివి) == 0);
        }
#అయితే_ముగింపు

        ఒకవేళ (export_రకం == d_external_రకం_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->అంకె_Functions);
            IM3Function func = &(io_module->functions[index]);
            ఒకవేళ (func->అంకె_పేర్లు < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_నిడివి + 1);
                _throwifnull(name);
                func->పేర్లు[func->అంకె_పేర్లు++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } లేకపోతే ఒకవేళ (export_రకం == d_external_రకం_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->అంకె_Globals);
        } లేకపోతే ఒకవేళ (export_రకం == d_external_రకం_జ్ఞాపకం) {
            _throwif(m3Err_wasmMalformed, index >= io_module->అంకె_Memories);
        } లేకపోతే ఒకవేళ (export_రకం == d_external_రకం_పట్టిక) {
            _throwif(m3Err_wasmMalformed, index >= io_module->అంకె_Tables);
        }
#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
        లేకపోతే ఒకవేళ (export_రకం == d_external_రకం_ట్యాగ్) {
            _throwif(m3Err_wasmMalformed, index >= io_module->అంకె_ట్యాగులు);
        }
#అయితే_ముగింపు

        M3Export* నమోదు_2   = &io_module->exports[io_module->అంకె_Exports++];
        నమోదు_2->name       = utf8;
        నమోదు_2->name_నిడివి = name_నిడివి;
        నమోదు_2->index      = index;
        నమోదు_2->రకం       = export_రకం;

        utf8 = ఖాళీ; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

_catch:
    m3_Free(utf8);
    తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Section_ప్రారంభం (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 ప్రారంభం_Func_సూచీ;
_   (ReadLEB_u32(&ప్రారంభం_Func_సూచీ, &i_బైట్లు, i_చివర));                                  m3log (parse, "** Start Function: %d", ప్రారంభం_Func_సూచీ);

    ఒకవేళ (ప్రారంభం_Func_సూచీ < io_module->అంకె_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[ప్రారంభం_Func_సూచీ];
        ఒకవేళ (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->అంకె_వాదనలు != 0 || func->funcType->అంకె_Rets != 0);
        }

        io_module->ప్రారంభం_Function = ప్రారంభం_Func_సూచీ;
    } లేకపోతే {
        ఫలితం = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch: తిరిగి ఫలితం;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_ఫలితం Parse_Init_సమాసం_Typed (M3Module* io_module, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర, m3type_t* o_type)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#ఒకవేళ defined(d_m3PreferStaticAlloc)
    స్థిర M3Compilation compilation;
#లేకపోతే
    M3Compilation compilation;
#అయితే_ముగింపు
    compilation = (M3Compilation){ .runtime = ఖాళీ, .module = io_module, .wasm = *io_బైట్లు, .wasm_చివర = i_చివర, .is_Init_సమాసం = true };

    ఫలితం = Compile_ఖండం_Statements(&compilation);

    *io_బైట్లు = compilation.wasm;

    ఒకవేళ (o_type) {
        *o_type = compilation.దొంతర_సూచీ ? compilation.type_దొంతర[compilation.దొంతర_సూచీ - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Init_సమాసం (M3Module* io_module, బైట్లు_t* io_బైట్లు, cbytes_t i_చివర)
{
    తిరిగి Parse_Init_సమాసం_Typed(io_module, io_బైట్లు, i_చివర, ఖాళీ);
}


M3_ఫలితం ParseSection_Element (IM3Module io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32     అంకె_Segments;
    బైట్లు_t pos;
_   (ReadLEB_u32(&అంకె_Segments, &i_బైట్లు, i_చివర));                            m3log (parse, "** Element [%d]", అంకె_Segments);

    _throwif("too many element segments", అంకె_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, అంకె_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->అంకె_Element_Segments = అంకె_Segments;
    io_module->element_Section_చివర  = i_చివర;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_బైట్లు;
    కొరకు (u32 i = 0; i < అంకె_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_చివర));
#ఒకవేళ d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#లేకపోతే
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_చురుకైన);
#అయితే_ముగింపు

        segment->బహుళకం   = flags & 0x3;
        segment->is_సమాసం = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        ఒకవేళ (segment->బహుళకం == c_m3_Elem_చురుకైన_Idx) {
_           (ReadLEB_u32(&segment->పట్టిక_సూచీ, &pos, i_చివర));
        }

        bool is_చురుకైన = (segment->బహుళకం == c_m3_Elem_చురుకైన or segment->బహుళకం == c_m3_Elem_చురుకైన_Idx);

        ఒకవేళ (is_చురుకైన) {
            _throwif(m3Err_wasmMalformed, segment->పట్టిక_సూచీ >= io_module->అంకె_Tables);

            segment->init_సమాసం = pos;
_           (Parse_Init_సమాసం(io_module, &pos, i_చివర));
            segment->init_సమాసం_కొలత = (u32)(pos - segment->init_సమాసం);
            _throwif(m3_Err_wasm_Missing_Init_సమాసం, segment->init_సమాసం_కొలత <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        ఒకవేళ (segment->బహుళకం != c_m3_Elem_చురుకైన) {
            ఒకవేళ (segment->is_సమాసం) {
_               (Parse_విలువ_Type(io_module, &segment->type, &pos, i_చివర));
                _throwif(m3Err_wasmMalformed, not IsRefType(పునాది_Type_యొక్క(segment->type)));
            } లేకపోతే {
                u8 elem_రకం;
_               (Read_u8(&elem_రకం, &pos, i_చివర));
                _throwif(m3Err_wasmMalformed, elem_రకం != 0x00);   // funcref
            }
        }

        ఒకవేళ (is_చురుకైన) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->పట్టిక_సూచీ]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->అంకె_Elements, &pos, i_చివర));
        _throwif("table overflow", segment->అంకె_Elements > d_m3_Max_Sane_పట్టిక_కొలత);

        segment->elements = pos;

        కొరకు (u32 e = 0; e < segment->అంకె_Elements; ++e) {
            ఒకవేళ (segment->is_సమాసం) {
_               (Parse_Init_సమాసం(io_module, &pos, i_చివర))
            } లేకపోతే {
                u32 func_సూచీ;
_               (ReadLEB_u32(&func_సూచీ, &pos, i_చివర));
                _throwif("function index out of range", func_సూచీ >= io_module->అంకె_Functions);
_               (Module_DeclareFunction(io_module, func_సూచీ));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_చివర);           // section size mismatch

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Section_సంకేతం (M3Module* io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం;

    u32 అంకె_Functions;
_   (ReadLEB_u32(&అంకె_Functions, &i_బైట్లు, i_చివర));                                  m3log (parse, "** Code [%d]", అంకె_Functions);

    ఒకవేళ (అంకె_Functions != io_module->అంకె_Functions - io_module->అంకె_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    కొరకు (u32 f = 0; f < అంకె_Functions; ++f) {
        స్థిరరాశి u8* ప్రారంభం = i_బైట్లు;

        u32 కొలత;
_       (ReadLEB_u32(&కొలత, &i_బైట్లు, i_చివర));

        ఒకవేళ (కొలత) {
            i_బైట్లు += కొలత;

            ఒకవేళ (i_బైట్లు <= i_చివర) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->అంకె_Func_Imports);

                func->module  = io_module;
                func->wasm    = ప్రారంభం;
                func->wasm_చివర = i_బైట్లు;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } లేకపోతే {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    ఒకవేళ (not ఫలితం and i_బైట్లు != i_చివర) {
        ఫలితం = m3Err_wasmSectionUnderrun;
    }

    తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Section_దత్తాంశం (M3Module* io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 అంకె_దత్తాంశం_Segments;
_   (ReadLEB_u32(&అంకె_దత్తాంశం_Segments, &i_బైట్లు, i_చివర));                               m3log (parse, "** Data [%d]", అంకె_దత్తాంశం_Segments);

    _throwif("too many data segments", అంకె_దత్తాంశం_Segments > d_m3_Max_Sane_దత్తాంశం_Segments);

    io_module->దత్తాంశం_Segments = m3_AllocArray(M3_దత్తాంశం_Segment, అంకె_దత్తాంశం_Segments);
    _throwifnull(io_module->దత్తాంశం_Segments);
    io_module->అంకె_దత్తాంశం_Segments = అంకె_దత్తాంశం_Segments;

    కొరకు (u32 i = 0; i < అంకె_దత్తాంశం_Segments; ++i) {
        M3_దత్తాంశం_Segment* segment = &io_module->దత్తాంశం_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_బైట్లు, i_చివర));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        ఒకవేళ (flags == 2) {
_           (ReadLEB_u32(&segment->జ్ఞాపకం_Region, &i_బైట్లు, i_చివర));
        }

        ఒకవేళ (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#ఒకవేళ !d_m3_Has_Multi_జ్ఞాపకం
            _throwif(m3Err_wasmMalformed, segment->జ్ఞాపకం_Region != 0);
#అయితే_ముగింపు
            _throwif(m3Err_wasmMalformed, segment->జ్ఞాపకం_Region >= io_module->అంకె_Memories);

            segment->init_సమాసం = i_బైట్లు;
_           (Parse_Init_సమాసం(io_module, &i_బైట్లు, i_చివర));
            segment->init_సమాసం_కొలత = (u32)(i_బైట్లు - segment->init_సమాసం);

            _throwif(m3_Err_wasm_Missing_Init_సమాసం, segment->init_సమాసం_కొలత <= 1);
        }

_       (ReadLEB_u32(&segment->కొలత, &i_బైట్లు, i_చివర));
        segment->దత్తాంశం = i_బైట్లు;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->జ్ఞాపకం_Region, segment->init_సమాసం_కొలత, segment->కొలత);
        i_బైట్లు += segment->కొలత;

        _throwif("data segment underflow", i_బైట్లు > i_చివర);
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch:

    తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Section_దత్తాంశం_లెక్క (M3Module* io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

_   (ReadLEB_u32(&io_module->దత్తాంశం_లెక్క, &i_బైట్లు, i_చివర));                       m3log (parse, "** DataCount [%d]", io_module->దత్తాంశం_లెక్క);

    io_module->has_దత్తాంశం_లెక్క = true;

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం Parse_Section_జ్ఞాపకం (M3Module* io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 అంకె_Memories;
_   (ReadLEB_u32(&అంకె_Memories, &i_బైట్లు, i_చివర));                                m3log (parse, "** Memory [%d]", అంకె_Memories);

#ఒకవేళ d_m3_Has_Multi_జ్ఞాపకం
    _throwif(m3_Err_too_Many_జ్ఞాపకం_Sections,
             (u64)అంకె_Memories + io_module->అంకె_Memories > d_m3_Max_Sane_Memories_లెక్క);
#లేకపోతే
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_జ్ఞాపకం_Sections, అంకె_Memories > 1);
    _throwif(m3_Err_too_Many_జ్ఞాపకం_Sections, (u64)అంకె_Memories + io_module->అంకె_Memories > 1);
#అయితే_ముగింపు

    కొరకు (u32 i = 0; i < అంకె_Memories; ++i) {
        M3_జ్ఞాపకం_Info info;
_       (Parse_Type_జ్ఞాపకం(&info, &i_బైట్లు, i_చివర));
_       (Module_కూడు_జ్ఞాపకం(io_module, ఖాళీ, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం ParseSection_Global (M3Module* io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    u32 అంకె_Globals;
_   (ReadLEB_u32(&అంకె_Globals, &i_బైట్లు, i_చివర));                                    m3log (parse, "** Global [%d]", అంకె_Globals);

    _throwif("too many globals", అంకె_Globals > d_m3_Max_Sane_Globals_లెక్క);

    కొరకు (u32 i = 0; i < అంకె_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_విలువ_Type(io_module, &type, &i_బైట్లు, i_చివర));
_       (ReadLEB_u7(&isMutable, &i_బైట్లు, i_చివర));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [పునాది_Type_యొక్క(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_కూడు_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_సమాసం = i_బైట్లు;
_       (Parse_Init_సమాసం(io_module, &i_బైట్లు, i_చివర));
        global->init_సమాసం_కొలత = (u32)(i_బైట్లు - global->init_సమాసం);

        _throwif(m3_Err_wasm_Missing_Init_సమాసం, global->init_సమాసం_కొలత <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_బైట్లు != i_చివర);      // section size mismatch

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం ParseSection_Name (M3Module* io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    cstr_t name = ఖాళీ;

    అయితే (i_బైట్లు < i_చివర) {
        u8  nameType;
        u32 payload_నిడివి;

_       (ReadLEB_u7(&nameType, &i_బైట్లు, i_చివర));
_       (ReadLEB_u32(&payload_నిడివి, &i_బైట్లు, i_చివర));

        బైట్లు_t ప్రారంభం = i_బైట్లు;
        _throwif(m3Err_wasmSectionOverrun, payload_నిడివి > (size_t)(i_చివర - ప్రారంభం));

        ఒకవేళ (nameType == 1) {
            u32 అంకె_పేర్లు;
_           (ReadLEB_u32(&అంకె_పేర్లు, &i_బైట్లు, i_చివర));

            _throwif("too many names", అంకె_పేర్లు > d_m3_Max_Sane_Functions_లెక్క);

            కొరకు (u32 i = 0; i < అంకె_పేర్లు; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_బైట్లు, i_చివర));
_               (Read_utf8(&name, &i_బైట్లు, i_చివర));

                ఒకవేళ (index < io_module->అంకె_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    ఒకవేళ (func->అంకె_పేర్లు == 0) {
                        func->పేర్లు[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->అంకె_పేర్లు = 1;

                        name = ఖాళీ; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_బైట్లు = ప్రారంభం + payload_నిడివి;
    }

    _catch: తిరిగి ఫలితం;
}


M3_ఫలితం ParseSection_Custom (M3Module* io_module, బైట్లు_t i_బైట్లు, cbytes_t i_చివర)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    cstr_t name = ఖాళీ;
_   (Read_utf8(&name, &i_బైట్లు, i_చివర));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    ఒకవేళ (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_బైట్లు, i_చివర));
#ఒకవేళ d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } లేకపోతే ఒకవేళ (strcmp(name, "snapshot") == 0 || (అక్షరశ్రేణి_పొడవు(name) > 9 && అక్షరశ్రేణి_పోలిక(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, అక్షరశ్రేణి_పొడవు(snapName) + 1);
        ఒకవేళ (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->దత్తాంశం           = i_బైట్లు;
        snap->కొలత           = (u32)(i_చివర - i_బైట్లు);
        snap->తదుపరి           = io_module->snapshots;
        io_module->snapshots = snap;
#అయితే_ముగింపు
    } లేకపోతే ఒకవేళ (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_బైట్లు, i_చివర));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    తిరిగి ఫలితం;
}


M3_ఫలితం ParseModuleSection (M3Module* o_module, u8 i_sectionType, బైట్లు_t i_బైట్లు, u32 i_అంకె_బైట్లు)
{
    M3_ఫలితం ఫలితం = m3Err_none;

    రకనిర్వచనం M3_ఫలితం (*M3Parser)(M3Module*, బైట్లు_t, cbytes_t);

    స్థిర M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_పట్టిక,        // 4
        Parse_Section_జ్ఞాపకం,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_ప్రారంభం,     // 8
        ParseSection_Element,   // 9
        Parse_Section_సంకేతం,      // 10
        Parse_Section_దత్తాంశం,      // 11
        Parse_Section_దత్తాంశం_లెక్క, // 12
#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
        Parse_Section_ట్యాగ్,       // 13
#అయితే_ముగింపు
    };

    M3Parser parser = ఖాళీ;

    ఒకవేళ (i_sectionType < M3_లెక్క_యొక్క(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    ఒకవేళ (parser) {
        cbytes_t చివర = i_బైట్లు + i_అంకె_బైట్లు;
        ఫలితం       = parser(o_module, i_బైట్లు, చివర);
    } లేకపోతే {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    తిరిగి ఫలితం;
}


M3_ఫలితం m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_బైట్లు, u32 i_అంకె_బైట్లు)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_అంకె_బైట్లు);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_అంకె_బైట్లు);
    module->ప్రారంభం_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    స్థిరరాశి u8* pos = i_బైట్లు;
    స్థిరరాశి u8* చివర = pos + i_అంకె_బైట్లు;

    module->wasm_ప్రారంభం = pos;
    module->wasm_చివర   = చివర;

    u32 magic, సంస్కరణ;
_   (Read_u32(&magic, &pos, చివర));
_   (Read_u32(&సంస్కరణ, &pos, చివర));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_సంస్కరణ, సంస్కరణ != 1);

#ఒకవేళ d_m3HasExceptionHandling || d_m3_Has_దొంతర_Switching
    // the tag section sits between memory (5) and global (6)
    స్థిర స్థిరరాశి u8 sections_క్రమం[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#లేకపోతే
    స్థిర స్థిరరాశి u8 sections_క్రమం[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#అయితే_ముగింపు
    u8 expectedSection = 0;

    అయితే (pos < చివర) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, చివర));

        ఒకవేళ (section != 0) {
            // Ensure sections appear only once and in order
            అయితే (sections_క్రమం[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_లెక్క_యొక్క(sections_క్రమం) - 1);
            }
        }

        u32 section_నిడివి;
_       (ReadLEB_u32(&section_నిడివి, &pos, చివర));
        _throwif(m3Err_wasmMalformed, pos + section_నిడివి > చివర);

_       (ParseModuleSection(module, section, pos, section_నిడివి));

        pos += section_నిడివి;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    ఒకవేళ (module->అంకె_Functions > module->అంకె_Func_Imports) {
        IM3Function మొదటి_Non_Import = &module->functions[module->అంకె_Func_Imports];
        _throwif(m3Err_wasmMalformed, మొదటి_Non_Import->wasm == ఖాళీ);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_దత్తాంశం_లెక్క and module->దత్తాంశం_లెక్క != module->అంకె_దత్తాంశం_Segments);

} _catch:

    ఒకవేళ (ఫలితం) {
        m3_FreeModule(module);
        module = ఖాళీ;
    }

    *o_module = module;

    తిరిగి ఫలితం;
}
