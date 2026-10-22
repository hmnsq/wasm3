//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_compile.h"
#अन्तर्भाव "m3_exception.h"
#अन्तर्भाव "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#यदि d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_परिणाम ParseHeapType (IM3Module i_module, m3type_t* o_heap_द्वयङ्काः, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    i64 heap;
_   (ReadLebSigned(&heap, 33, io_अष्टकानि, i_अन्तः));

    यदि (heap == -d_waType_funcref) {
        *o_heap_द्वयङ्काः = d_m3Type_heapAbstract;
    } अन्यथा यदि (heap == -d_waType_externref) {
        *o_heap_द्वयङ्काः = d_m3Type_refExtern | d_m3Type_heapAbstract;
#  यदि d_m3_Has_राशिः_Switching
    } अन्यथा यदि (heap == -d_waType_heap_cont) {
        *o_heap_द्वयङ्काः = d_m3Type_refCont | d_m3Type_heapAbstract;
    } अन्यथा यदि (heap == -d_waType_heap_nocont) {
        *o_heap_द्वयङ्काः = d_m3Type_refCont | d_m3Type_heapNone;
#  endif
    } अन्यथा यदि (heap >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) heap >= i_module->अङ्कः_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[heap];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_heap_द्वयङ्काः = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_सूचकांक;
    } अन्यथा {
        _throw(m3Err_invalidTypeId);
    }

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_परिणाम Parse_मानम्_Type (IM3Module i_module, m3type_t* o_type, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_अष्टकानि >= i_अन्तः);

#यदि d_m3HasTypedRefs
    यदि (**io_अष्टकानि == d_waEncode_ref or **io_अष्टकानि == d_waEncode_refNull) {
        u8 lead = *(*io_अष्टकानि)++;

        m3type_t heap_द्वयङ्काः = d_m3Type_heapAbstract;
_       (ParseHeapType(i_module, &heap_द्वयङ्काः, io_अष्टकानि, i_अन्तः));

        *o_type = d_m3Type_ref | heap_द्वयङ्काः | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        प्रत्यागम परिणाम;
    }
#यदि_अन्तः

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_अष्टकानि, i_अन्तः));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: प्रत्यागम परिणाम;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
स्थिर
M3_परिणाम Read_Type_सारणी_Type (IM3Module i_module, M3_सारणी_Info* o_info, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_आकार = 0, max_आकार = 0;

    // the width of the limits, and of every index into the table
    u32 addr_द्वयङ्काः = 32;

_   (Parse_मानम्_Type(i_module, &elemType, io_अष्टकानि, i_अन्तः));
#यदि d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(आधारः_Type_सम्बन्धि(elemType)));
#अन्यथा
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#यदि_अन्तः

_   (ReadLEB_u7(&flag, io_अष्टकानि, i_अन्तः));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#यदि d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#अन्यथा
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#यदि_अन्तः

    यदि (flag & 0x04u) {
        addr_द्वयङ्काः = 64;
    }

_   (ReadLebUnsigned(&init_आकार, addr_द्वयङ्काः, io_अष्टकानि, i_अन्तः));
    _throwif("table overflow", init_आकार > d_m3_Max_Sane_सारणी_आकार);

    यदि (flag & 0x01u) {
_       (ReadLebUnsigned(&max_आकार, addr_द्वयङ्काः, io_अष्टकानि, i_अन्तः));
        _throwif(m3Err_wasmMalformed, max_आकार < init_आकार);
        _throwif("table overflow", max_आकार > d_m3_Max_Sane_सारणी_आकार);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_आकार  = (u32)init_आकार;
    o_info->max_आकार   = (u32)max_आकार;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    प्रत्यागम परिणाम;
}


स्थिर
M3_परिणाम Parse_Type_सारणी_Type (IM3Module io_module, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3_सारणी_Info info;

_   (Read_Type_सारणी_Type(io_module, &info, io_अष्टकानि, i_अन्तः));
_   (Module_सङ्कलय_सारणी(io_module, रिक्त, &info, false /* isImport */));

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Parse_Init_व्यञ्जकम्_Typed (M3Module* io_module, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः, m3type_t* o_type);

M3_परिणाम Parse_Type_सारणी (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अङ्कः_Tables;
_   (ReadLEB_u32(&अङ्कः_Tables, &i_अष्टकानि, i_अन्तः));                          m3log (parse, "** Table [%d]", अङ्कः_Tables);

    _throwif("too many tables", (u64)अङ्कः_Tables + io_module->अङ्कः_Tables > d_m3_Max_Sane_सारणी_गणनम्);

    प्रति (u32 i = 0; i < अङ्कः_Tables; ++i) {
#यदि d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_व्यञ्जकम् = (i_अष्टकानि < i_अन्तः) and (*i_अष्टकानि == 0x40);
        u32  सारणी_सूचकांक  = io_module->अङ्कः_Tables;

        यदि (has_Init_व्यञ्जकम्) {
            u8 reserved;
            ++i_अष्टकानि;
_           (Read_u8(&reserved, &i_अष्टकानि, i_अन्तः));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_सारणी_Type(io_module, &i_अष्टकानि, i_अन्तः));

        यदि (has_Init_व्यञ्जकम्) {
            IM3_सारणी सारणी_2 = io_module->tables[सारणी_सूचकांक];

            m3type_t initType;

            सारणी_2->init_व्यञ्जकम् = i_अष्टकानि;
_           (Parse_Init_व्यञ्जकम्_Typed(io_module, &i_अष्टकानि, i_अन्तः, &initType));
            सारणी_2->init_व्यञ्जकम्_आकार = (u32)(i_अष्टकानि - सारणी_2->init_व्यञ्जकम्);
            _throwif(m3_Err_wasm_Missing_Init_व्यञ्जकम्, सारणी_2->init_व्यञ्जकम्_आकार <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_सम्बन्धि(initType, सारणी_2->type));
        } अन्यथा {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[सारणी_सूचकांक]->type));
        }
#अन्यथा
_       (Parse_Type_सारणी_Type(io_module, &i_अष्टकानि, i_अन्तः));
#यदि_अन्तः
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Parse_Type_स्मृतिः (M3_स्मृतिः_Info* o_स्मृतिः, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_Page_आकार = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_द्वयङ्काः = 32;

_   (ReadLEB_u7(&flag, io_अष्टकानि, i_अन्तः));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 ज्ञातम् = 0x09u;
#यदि d_m3HasAtomics
        ज्ञातम् |= 0x02u;
#यदि_अन्तः
#यदि d_m3HasMemory64
        ज्ञातम् |= 0x04u;
#यदि_अन्तः
        _throwif(m3Err_wasmMalformed, flag & ~ज्ञातम्);
    }

    o_स्मृतिः->isShared = (flag & (1u << 1)) != 0;

    o_स्मृतिः->isMemory64 = (flag & (1u << 2)) != 0;
    यदि (o_स्मृतिः->isMemory64) {
        addr_द्वयङ्काः = 64;
    }

_   (ReadLebUnsigned(&o_स्मृतिः->init_पृष्ठानि, addr_द्वयङ्काः, io_अष्टकानि, i_अन्तः));

    o_स्मृतिः->max_पृष्ठानि = 0;
    o_स्मृतिः->hasMax   = (flag & (1u << 0)) != 0;
    यदि (o_स्मृतिः->hasMax) {
_       (ReadLebUnsigned(&o_स्मृतिः->max_पृष्ठानि, addr_द्वयङ्काः, io_अष्टकानि, i_अन्तः));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_स्मृतिः->max_पृष्ठानि < o_स्मृतिः->init_पृष्ठानि);
    }

    _throwif("shared memory must have maximum", o_स्मृतिः->isShared and not o_स्मृतिः->hasMax);

    o_स्मृतिः->page_आकार = 0;
    यदि (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_Page_आकार, io_अष्टकानि, i_अन्तः));

        _throwif("invalid custom page size", log_Page_आकार > 16);

        o_स्मृतिः->page_आकार = 1u << log_Page_आकार;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_पृष्ठानि_Allowed = (log_Page_आकार or addr_द्वयङ्काः < 64)
                                ? (UINT64_C(1) << (addr_द्वयङ्काः - log_Page_आकार))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_स्मृतिः->init_पृष्ठानि > max_पृष्ठानि_Allowed);
        यदि (o_स्मृतिः->hasMax) {
            _throwif(m3Err_wasmMalformed, o_स्मृतिः->max_पृष्ठानि > max_पृष्ठानि_Allowed);
        }
    }

    _catch: प्रत्यागम परिणाम;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
स्थिर
M3_परिणाम ParseFuncType (IM3Module io_module, IM3FuncType* o_type, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    IM3FuncType ftype = रिक्त;
_try {
    u32 अङ्कः_तर्काः;
_   (ReadLEB_u32(&अङ्कः_तर्काः, io_अष्टकानि, i_अन्तः));

    _throwif(m3_Err_too_Many_तर्काः_Rets, अङ्कः_तर्काः > d_m3_Max_Sane_Function_Arg_Ret_गणनम्);
#यदि M3_HAS_VLA
    m3type_t argTypes[अङ्कः_तर्काः + 1]; // make ubsan happy
#अन्यथा
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_गणनम्];
#यदि_अन्तः
    प्रति (u32 a = 0; a < अङ्कः_तर्काः; ++a) {
_       (Parse_मानम्_Type(io_module, &argTypes[a], io_अष्टकानि, i_अन्तः));
    }

    u32 अङ्कः_Rets;
_   (ReadLEB_u32(&अङ्कः_Rets, io_अष्टकानि, i_अन्तः));
    _throwif(m3_Err_too_Many_तर्काः_Rets, (u64)(अङ्कः_Rets) + अङ्कः_तर्काः > d_m3_Max_Sane_Function_Arg_Ret_गणनम्);

_   (AllocFuncType(&ftype, अङ्कः_Rets + अङ्कः_तर्काः));
    ftype->अङ्कः_तर्काः = अङ्कः_तर्काः;
    ftype->अङ्कः_Rets = अङ्कः_Rets;

    प्रति (u32 r = 0; r < अङ्कः_Rets; ++r) {
_       (Parse_मानम्_Type(io_module, &ftype->types[r], io_अष्टकानि, i_अन्तः));
    }
    स्मृतिं_प्रतिलिख(ftype->types + अङ्कः_Rets, argTypes, अङ्कः_तर्काः * परिमाण(m3type_t));

    *o_type = ftype;
    ftype   = रिक्त;

} _catch:
    m3_Free(ftype);
    प्रत्यागम परिणाम;
}


#यदि d_m3_Has_राशिः_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
स्थिर
M3_परिणाम ParseContType (IM3Module io_module, IM3FuncType* o_type, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    IM3FuncType ftype = रिक्त;
_try {
    i64 ft_सूचकांक;
_   (ReadLebSigned(&ft_सूचकांक, 33, io_अष्टकानि, i_अन्तः));
    _throwif(m3Err_wasmMalformed, ft_सूचकांक < 0 or (u64) ft_सूचकांक >= io_module->अङ्कः_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_सूचकांक];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 योग = underlying->अङ्कः_Rets + underlying->अङ्कः_तर्काः;
_   (AllocFuncType(&ftype, योग));
    ftype->अङ्कः_Rets        = underlying->अङ्कः_Rets;
    ftype->अङ्कः_तर्काः        = underlying->अङ्कः_तर्काः;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    यदि (योग) {
        स्मृतिं_प्रतिलिख(ftype->types, underlying->types, योग * परिमाण(m3type_t));
    }

    *o_type = ftype;
    ftype   = रिक्त;

} _catch:
    m3_Free(ftype);
    प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasStackSwitching


स्थिर
M3_परिणाम ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
#यदि d_m3_Has_राशिः_Switching
    यदि (i_form == -35) {
        प्रत्यागम ParseContType(io_module, o_type, io_अष्टकानि, i_अन्तः);
    }
#यदि_अन्तः
    यदि (i_form != -32) { // for Wasm MVP
        प्रत्यागम m3Err_wasmMalformed;
    }

    प्रत्यागम ParseFuncType(io_module, o_type, io_अष्टकानि, i_अन्तः);
}


#यदि d_m3_Has_राशिः_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
स्थिर
M3_परिणाम SkipTypeDefinition (i8 i_form, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
_try {
    यदि (i_form == -35) {
        i64 ft_सूचकांक;
_       (ReadLebSigned(&ft_सूचकांक, 33, io_अष्टकानि, i_अन्तः));
        प्रत्यागम परिणाम;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    प्रति (u32 half = 0; half < 2; ++half) {
        u32 गणनम्;
_       (ReadLEB_u32(&गणनम्, io_अष्टकानि, i_अन्तः));
        _throwif(m3_Err_too_Many_तर्काः_Rets, गणनम् > d_m3_Max_Sane_Function_Arg_Ret_गणनम्);

        प्रति (u32 v = 0; v < गणनम्; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_अष्टकानि, i_अन्तः));

            यदि (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 heap;
_               (ReadLebSigned(&heap, 33, io_अष्टकानि, i_अन्तः));
            }
        }
    }

} _catch: प्रत्यागम परिणाम;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
स्थिर
M3_परिणाम Parse_Rec_गणः (IM3Module io_module, u32 i_गणनम्, u32 i_first_सूचकांक,
                        अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    IM3FuncType* placeholders = रिक्त;
_try {
    अष्टकानि_t गणः_Start = *io_अष्टकानि;

    placeholders = m3_AllocArray(IM3FuncType, i_गणनम्);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    प्रति (u32 j = 0; j < i_गणनम्; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_अष्टकानि, i_अन्तः));
_       (SkipTypeDefinition(form, io_अष्टकानि, i_अन्तः));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 firstCanonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_गणनम्, &firstCanonical));

    प्रति (u32 j = 0; j < i_गणनम्; ++j) {
        placeholders[j]->canonical_सूचकांक        = (u16)(firstCanonical + j);
        io_module->funcTypes[i_first_सूचकांक + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    प्रति (u32 pass = 0; pass < 2; ++pass) {
        अष्टकानि_t अष्टकानि = गणः_Start;

        प्रति (u32 j = 0; j < i_गणनम्; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &अष्टकानि, i_अन्तः));

            यदि ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &अष्टकानि, i_अन्तः));
                अनुवर्त;
            }

            IM3FuncType ftype = रिक्त;
_           (ParseTypeDefinition(io_module, form, &ftype, &अष्टकानि, i_अन्तः));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_सूचकांक);

            m3_Free(placeholders[j]);
            placeholders[j]                        = रिक्त;
            io_module->funcTypes[i_first_सूचकांक + j] = ftype;
        }

        यदि (pass == 1) {
            *io_अष्टकानि = अष्टकानि;
        }
    }

} _catch:
    यदि (placeholders) {
        प्रति (u32 j = 0; j < i_गणनम्; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasStackSwitching


M3_परिणाम ParseSection_Type (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    IM3FuncType ftype = रिक्त;

_try {
    u32 अङ्कः_Types;
_   (ReadLEB_u32(&अङ्कः_Types, &i_अष्टकानि, i_अन्तः));                                      m3log (parse, "** Type [%d]", अङ्कः_Types);

    _throwif("too many types", अङ्कः_Types > d_m3_Max_Sane_Types_गणनम्);

    यदि (अङ्कः_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, अङ्कः_Types);
        _throwifnull(io_module->funcTypes);
        io_module->अङ्कः_Func_Types = अङ्कः_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#यदि d_m3_Has_राशिः_Switching
        u32 क्षमता = अङ्कः_Types;
#यदि_अन्तः
        u32 index = 0;

        प्रति (u32 i = 0; i < अङ्कः_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_अष्टकानि, i_अन्तः));

#यदि d_m3_Has_राशिः_Switching
            यदि (form == -50) { // rec
                u32 गणनम्;
_               (ReadLEB_u32(&गणनम्, &i_अष्टकानि, i_अन्तः));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + गणनम् + (अङ्कः_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_गणनम्);

                यदि (needed > क्षमता) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, क्षमता);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    क्षमता                = grown;
                    io_module->अङ्कः_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                यदि (गणनम् == 1) {
_                   (ReadLEB_i7(&form, &i_अष्टकानि, i_अन्तः));
                } अन्यथा {
_                   (Parse_Rec_गणः(io_module, गणनम्, index, &i_अष्टकानि, i_अन्तः));
                    index += गणनम्;
                    अनुवर्त;
                }
            }
#यदि_अन्तः

_           (ParseTypeDefinition(io_module, form, &ftype, &i_अष्टकानि, i_अन्तः));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_सङ्कलय_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = रिक्त; // ownership transferred to environment
        }

        io_module->अङ्कः_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

} _catch:

    यदि (परिणाम) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->अङ्कः_Func_Types = 0;
    }

    प्रत्यागम परिणाम;
}


M3_परिणाम ParseSection_Function (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अङ्कः_Functions;
_   (ReadLEB_u32(&अङ्कः_Functions, &i_अष्टकानि, i_अन्तः));                                  m3log (parse, "** Function [%d]", अङ्कः_Functions);

    _throwif("too many functions", अङ्कः_Functions > d_m3_Max_Sane_Functions_गणनम्);

_   (Module_PreallocFunctions(io_module, io_module->अङ्कः_Functions + अङ्कः_Functions));

    प्रति (u32 i = 0; i < अङ्कः_Functions; ++i) {
        u32 func_Type_सूचकांक;
_       (ReadLEB_u32(&func_Type_सूचकांक, &i_अष्टकानि, i_अन्तः));

_       (Module_सङ्कलय_Function(io_module, func_Type_सूचकांक, रिक्त /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch: प्रत्यागम परिणाम;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
प्रकारनिर्देश संरचना M3ImportDesc {
    u8           विधा;
    u32          type_सूचकांक;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_सारणी_Info  सारणी_2;
    M3_स्मृतिः_Info स्मृतिः;
} M3ImportDesc;


#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
स्थिर
M3_परिणाम Read_चिह्नकम्_Type (IM3Module i_module, u32* o_type_सूचकांक, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_अष्टकानि, i_अन्तः));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_सूचकांक, io_अष्टकानि, i_अन्तः));
    _throwif(m3Err_unknownType, not i_module or *o_type_सूचकांक >= i_module->अङ्कः_Func_Types);

#  यदि !d_m3_Has_राशिः_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_अङ्कः_Results(i_module->funcTypes[*o_type_सूचकांक]) != 0);
#  endif

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
स्थिर
M3_परिणाम ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (Read_u8(&o_desc->विधा, io_अष्टकानि, i_अन्तः));

    विकल्प (o_desc->विधा) {
    पक्ष d_external_विधा_function:
_       (ReadLEB_u32(&o_desc->type_सूचकांक, io_अष्टकानि, i_अन्तः));
        भङ्ग;

    पक्ष d_external_विधा_सारणी:
_       (Read_Type_सारणी_Type(i_module, &o_desc->सारणी_2, io_अष्टकानि, i_अन्तः));
        भङ्ग;

    पक्ष d_external_विधा_स्मृतिः:
_       (Parse_Type_स्मृतिः(&o_desc->स्मृतिः, io_अष्टकानि, i_अन्तः));
        भङ्ग;

    पक्ष d_external_विधा_global:
_       (Parse_मानम्_Type(i_module, &o_desc->type, io_अष्टकानि, i_अन्तः));
_       (ReadLEB_u7(&o_desc->isMutable, io_अष्टकानि, i_अन्तः));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [आधारः_Type_सम्बन्धि(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        भङ्ग;

#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    पक्ष d_external_विधा_चिह्नकम्:
_       (Read_चिह्नकम्_Type(i_module, &o_desc->type_सूचकांक, io_अष्टकानि, i_अन्तः));
        भङ्ग;
#यदि_अन्तः

    स्वभाव:
        _throw(m3Err_wasmMalformed);
    }

    _catch: प्रत्यागम परिणाम;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
स्थिर
M3_परिणाम ApplyImportDesc (IM3Module io_module, ध्रुव M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3ImportInfo clearImport = { रिक्त, रिक्त };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->विधा, io_import->moduleUtf8, io_import->क्षेत्रम्_Utf8);
    विकल्प (i_desc->विधा) {
    पक्ष d_external_विधा_function: {
_       (Module_सङ्कलय_Function(io_module, i_desc->type_सूचकांक, io_import))
        *io_import = clearImport;

        io_module->अङ्कः_Func_Imports++;
    } भङ्ग;

    पक्ष d_external_विधा_सारणी: {
        IM3_सारणी सारणी_2;
_       (Module_सङ्कलय_सारणी(io_module, &सारणी_2, &i_desc->सारणी_2, true /* isImport */));
        सारणी_2->import = *io_import;
        *io_import    = clearImport;
    } भङ्ग;

    पक्ष d_external_विधा_स्मृतिः: {
        IM3_स्मृतिः स्मृतिः;

#यदि d_m3_Has_Multi_स्मृतिः
        _throwif(m3_Err_too_Many_स्मृतिः_Sections, io_module->अङ्कः_Memories >= d_m3_Max_Sane_Memories_गणनम्);
#अन्यथा
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_स्मृतिः_Sections, io_module->अङ्कः_Memories >= 1);
#यदि_अन्तः

_       (Module_सङ्कलय_स्मृतिः(io_module, &स्मृतिः, &i_desc->स्मृतिः, true /* isImport */));
        स्मृतिः->import = *io_import;
        *io_import     = clearImport;
    } भङ्ग;

    पक्ष d_external_विधा_global: {
        IM3Global global;
_       (Module_सङ्कलय_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } भङ्ग;

#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    पक्ष d_external_विधा_चिह्नकम्: {
        IM3_चिह्नकम् चिह्नकम्;
_       (Module_सङ्कलय_चिह्नकम्(io_module, &चिह्नकम्, io_module->funcTypes[i_desc->type_सूचकांक], true /* isImport */));
        चिह्नकम्->import = *io_import;
        *io_import  = clearImport;
    } भङ्ग;
#यदि_अन्तः

    स्वभाव:
        _throw(m3Err_wasmMalformed);
    }

    _catch: प्रत्यागम परिणाम;
}


#यदि d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
स्थिर
bool Is_रिक्तानि_Name (अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    u32 दीर्घता;
    प्रत्यागम (not ReadLEB_u32(&दीर्घता, &i_अष्टकानि, i_अन्तः)) and दीर्घता == 0;
}

#यदि_अन्तः // d_m3HasCompactImports


#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching

M3_परिणाम Parse_Section_चिह्नकम् (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अङ्कः_चिह्नकानि;
_   (ReadLEB_u32(&अङ्कः_चिह्नकानि, &i_अष्टकानि, i_अन्तः));                                       m3log (parse, "** Tag [%d]", अङ्कः_चिह्नकानि);

    _throwif("too many tags", अङ्कः_चिह्नकानि > d_m3_Max_Sane_चिह्नकानि_गणनम्);

    प्रति (u32 i = 0; i < अङ्कः_चिह्नकानि; ++i) {
        u32 type_सूचकांक;
_       (Read_चिह्नकम्_Type(io_module, &type_सूचकांक, &i_अष्टकानि, i_अन्तः));                      m3log (parse, "    tag: [%d] type: %d", i, type_सूचकांक);

_       (Module_सङ्कलय_चिह्नकम्(io_module, रिक्त, io_module->funcTypes[type_सूचकांक], false));
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch: प्रत्यागम परिणाम;
}

#यदि_अन्तः // d_m3HasExceptionHandling


M3_परिणाम ParseSection_Import (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3ImportInfo import = { रिक्त, रिक्त };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 अङ्कः_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 अङ्कः_Entries;
_   (ReadLEB_u32(&अङ्कः_Entries, &i_अष्टकानि, i_अन्तः));                                    m3log (parse, "** Import [%d]", अङ्कः_Entries);

    _throwif("too many imports", अङ्कः_Entries > d_m3_Max_Sane_Imports_गणनम्);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->अङ्कः_Functions + अङ्कः_Entries));

    प्रति (u32 i = 0; i < अङ्कः_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_अष्टकानि, i_अन्तः));

#यदि d_m3HasCompactImports
        अष्टकानि_t क्षेत्रम्_Start = i_अष्टकानि;
#यदि_अन्तः
_       (Read_utf8(&import.क्षेत्रम्_Utf8, &i_अष्टकानि, i_अन्तः));

#यदि d_m3HasCompactImports
        u8 compact = (i_अष्टकानि < i_अन्तः) ? *i_अष्टकानि : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        यदि ((compact == d_compact_Imports_प्रत्येकम्_Item_Type or compact == d_compactImports_sharedType) and
            Is_रिक्तानि_Name(क्षेत्रम्_Start, i_अन्तः)) {
            ++i_अष्टकानि;

            m3_Free(import.क्षेत्रम्_Utf8);         // it only marked the compact form
            import.क्षेत्रम्_Utf8 = रिक्त;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            यदि (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_अष्टकानि, i_अन्तः));
            }

            u32 अङ्कः_वस्तूनि;
_           (ReadLEB_u32(&अङ्कः_वस्तूनि, &i_अष्टकानि, i_अन्तः));                              m3log (parse, "    compact: %d import(s) of '%s'", अङ्कः_वस्तूनि, import.moduleUtf8);

            _throwif("too many imports", (u64)अङ्कः_Imports + अङ्कः_वस्तूनि > d_m3_Max_Sane_Imports_गणनम्);
            अङ्कः_Imports += अङ्कः_वस्तूनि;

_           (Module_PreallocFunctions(io_module, io_module->अङ्कः_Functions + अङ्कः_वस्तूनि));

            प्रति (u32 j = 0; j < अङ्कः_वस्तूनि; ++j) {
                M3ImportInfo item = { रिक्त, रिक्त };

                item.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, वाक्यदैर्घ्यम्(import.moduleUtf8) + 1);

                यदि (item.moduleUtf8) {
                    परिणाम = Read_utf8(&item.क्षेत्रम्_Utf8, &i_अष्टकानि, i_अन्तः);

                    // the shared form reuses the externtype read above
                    यदि (not परिणाम and compact == d_compact_Imports_प्रत्येकम्_Item_Type) {
                        परिणाम = ReadImportDesc(io_module, &desc, &i_अष्टकानि, i_अन्तः);
                    }

                    यदि (not परिणाम) {
                        परिणाम = ApplyImportDesc(io_module, &desc, &item);
                    }
                } अन्यथा {
                    परिणाम = m3Err_mallocFailed;
                }

                FreeImportInfo(&item);
                _throwif(परिणाम, परिणाम);
            }

            FreeImportInfo(&import);
            अनुवर्त;
        }
#यदि_अन्तः // d_m3HasCompactImports

        _throwif("too many imports", अङ्कः_Imports >= d_m3_Max_Sane_Imports_गणनम्);
        ++अङ्कः_Imports;

_       (ReadImportDesc(io_module, &desc, &i_अष्टकानि, i_अन्तः));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    प्रत्यागम परिणाम;
}


M3_परिणाम ParseSection_Export (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम    परिणाम = m3Err_none;
    ध्रुव वर्ण* utf8   = रिक्त;

    u32 अङ्कः_Exports;
_   (ReadLEB_u32(&अङ्कः_Exports, &i_अष्टकानि, i_अन्तः));                                    m3log (parse, "** Export [%d]", अङ्कः_Exports);

    _throwif("too many exports", अङ्कः_Exports > d_m3_Max_Sane_Exports_गणनम्);

    // the section order check lets this section through once, so this is
    // every export the module has
    यदि (अङ्कः_Exports) {
        io_module->exports = m3_AllocArray(M3Export, अङ्कः_Exports);
        _throwifnull(io_module->exports);
    }

    प्रति (u32 i = 0; i < अङ्कः_Exports; ++i) {
        u8  export_विधा;
        u32 index;
        u32 name_दीर्घता;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            अष्टकानि_t दीर्घता_अष्टकानि = i_अष्टकानि;
_           (ReadLEB_u32(&name_दीर्घता, &दीर्घता_अष्टकानि, i_अन्तः));
        }

_       (Read_utf8(&utf8, &i_अष्टकानि, i_अन्तः));
_       (Read_u8(&export_विधा, &i_अष्टकानि, i_अन्तः));
_       (ReadLEB_u32(&index, &i_अष्टकानि, i_अन्तः));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_विधा, utf8);

#यदि d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        प्रति (u32 j = 0; j < i; ++j) {
            ध्रुव M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_दीर्घता == name_दीर्घता and memcmp(other->name, utf8, name_दीर्घता) == 0);
        }
#यदि_अन्तः

        यदि (export_विधा == d_external_विधा_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अङ्कः_Functions);
            IM3Function func = &(io_module->functions[index]);
            यदि (func->अङ्कः_नामानि < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_दीर्घता + 1);
                _throwifnull(name);
                func->नामानि[func->अङ्कः_नामानि++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } अन्यथा यदि (export_विधा == d_external_विधा_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अङ्कः_Globals);
        } अन्यथा यदि (export_विधा == d_external_विधा_स्मृतिः) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अङ्कः_Memories);
        } अन्यथा यदि (export_विधा == d_external_विधा_सारणी) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अङ्कः_Tables);
        }
#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
        अन्यथा यदि (export_विधा == d_external_विधा_चिह्नकम्) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अङ्कः_चिह्नकानि);
        }
#यदि_अन्तः

        M3Export* प्रविष्टिः   = &io_module->exports[io_module->अङ्कः_Exports++];
        प्रविष्टिः->name       = utf8;
        प्रविष्टिः->name_दीर्घता = name_दीर्घता;
        प्रविष्टिः->index      = index;
        प्रविष्टिः->विधा       = export_विधा;

        utf8 = रिक्त; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

_catch:
    m3_Free(utf8);
    प्रत्यागम परिणाम;
}


M3_परिणाम ParseSection_Start (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 start_Func_सूचकांक;
_   (ReadLEB_u32(&start_Func_सूचकांक, &i_अष्टकानि, i_अन्तः));                                  m3log (parse, "** Start Function: %d", start_Func_सूचकांक);

    यदि (start_Func_सूचकांक < io_module->अङ्कः_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[start_Func_सूचकांक];
        यदि (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->अङ्कः_तर्काः != 0 || func->funcType->अङ्कः_Rets != 0);
        }

        io_module->startFunction = start_Func_सूचकांक;
    } अन्यथा {
        परिणाम = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch: प्रत्यागम परिणाम;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_परिणाम Parse_Init_व्यञ्जकम्_Typed (M3Module* io_module, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः, m3type_t* o_type)
{
    M3_परिणाम परिणाम = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#यदि defined(d_m3PreferStaticAlloc)
    स्थिर M3Compilation compilation;
#अन्यथा
    M3Compilation compilation;
#यदि_अन्तः
    compilation = (M3Compilation){ .runtime = रिक्त, .module = io_module, .wasm = *io_अष्टकानि, .wasm_अन्तः = i_अन्तः, .is_Init_व्यञ्जकम् = true };

    परिणाम = Compile_खण्डम्_Statements(&compilation);

    *io_अष्टकानि = compilation.wasm;

    यदि (o_type) {
        *o_type = compilation.राशिः_सूचकांक ? compilation.type_राशिः[compilation.राशिः_सूचकांक - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    प्रत्यागम परिणाम;
}


M3_परिणाम Parse_Init_व्यञ्जकम् (M3Module* io_module, अष्टकानि_t* io_अष्टकानि, cbytes_t i_अन्तः)
{
    प्रत्यागम Parse_Init_व्यञ्जकम्_Typed(io_module, io_अष्टकानि, i_अन्तः, रिक्त);
}


M3_परिणाम ParseSection_Element (IM3Module io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32     अङ्कः_Segments;
    अष्टकानि_t pos;
_   (ReadLEB_u32(&अङ्कः_Segments, &i_अष्टकानि, i_अन्तः));                            m3log (parse, "** Element [%d]", अङ्कः_Segments);

    _throwif("too many element segments", अङ्कः_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, अङ्कः_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->अङ्कः_Element_Segments = अङ्कः_Segments;
    io_module->element_Section_अन्तः  = i_अन्तः;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_अष्टकानि;
    प्रति (u32 i = 0; i < अङ्कः_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_अन्तः));
#यदि d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#अन्यथा
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_सक्रिय);
#यदि_अन्तः

        segment->बहुलकम्   = flags & 0x3;
        segment->is_व्यञ्जकम् = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        यदि (segment->बहुलकम् == c_m3_Elem_सक्रिय_Idx) {
_           (ReadLEB_u32(&segment->सारणी_सूचकांक, &pos, i_अन्तः));
        }

        bool is_सक्रिय = (segment->बहुलकम् == c_m3_Elem_सक्रिय or segment->बहुलकम् == c_m3_Elem_सक्रिय_Idx);

        यदि (is_सक्रिय) {
            _throwif(m3Err_wasmMalformed, segment->सारणी_सूचकांक >= io_module->अङ्कः_Tables);

            segment->init_व्यञ्जकम् = pos;
_           (Parse_Init_व्यञ्जकम्(io_module, &pos, i_अन्तः));
            segment->init_व्यञ्जकम्_आकार = (u32)(pos - segment->init_व्यञ्जकम्);
            _throwif(m3_Err_wasm_Missing_Init_व्यञ्जकम्, segment->init_व्यञ्जकम्_आकार <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        यदि (segment->बहुलकम् != c_m3_Elem_सक्रिय) {
            यदि (segment->is_व्यञ्जकम्) {
_               (Parse_मानम्_Type(io_module, &segment->type, &pos, i_अन्तः));
                _throwif(m3Err_wasmMalformed, not IsRefType(आधारः_Type_सम्बन्धि(segment->type)));
            } अन्यथा {
                u8 elem_विधा;
_               (Read_u8(&elem_विधा, &pos, i_अन्तः));
                _throwif(m3Err_wasmMalformed, elem_विधा != 0x00);   // funcref
            }
        }

        यदि (is_सक्रिय) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->सारणी_सूचकांक]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->अङ्कः_Elements, &pos, i_अन्तः));
        _throwif("table overflow", segment->अङ्कः_Elements > d_m3_Max_Sane_सारणी_आकार);

        segment->elements = pos;

        प्रति (u32 e = 0; e < segment->अङ्कः_Elements; ++e) {
            यदि (segment->is_व्यञ्जकम्) {
_               (Parse_Init_व्यञ्जकम्(io_module, &pos, i_अन्तः))
            } अन्यथा {
                u32 func_सूचकांक;
_               (ReadLEB_u32(&func_सूचकांक, &pos, i_अन्तः));
                _throwif("function index out of range", func_सूचकांक >= io_module->अङ्कः_Functions);
_               (Module_DeclareFunction(io_module, func_सूचकांक));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_अन्तः);           // section size mismatch

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Parse_Section_संहिता (M3Module* io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम;

    u32 अङ्कः_Functions;
_   (ReadLEB_u32(&अङ्कः_Functions, &i_अष्टकानि, i_अन्तः));                                  m3log (parse, "** Code [%d]", अङ्कः_Functions);

    यदि (अङ्कः_Functions != io_module->अङ्कः_Functions - io_module->अङ्कः_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    प्रति (u32 f = 0; f < अङ्कः_Functions; ++f) {
        ध्रुव u8* start = i_अष्टकानि;

        u32 आकार;
_       (ReadLEB_u32(&आकार, &i_अष्टकानि, i_अन्तः));

        यदि (आकार) {
            i_अष्टकानि += आकार;

            यदि (i_अष्टकानि <= i_अन्तः) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->अङ्कः_Func_Imports);

                func->module  = io_module;
                func->wasm    = start;
                func->wasm_अन्तः = i_अष्टकानि;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } अन्यथा {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    यदि (not परिणाम and i_अष्टकानि != i_अन्तः) {
        परिणाम = m3Err_wasmSectionUnderrun;
    }

    प्रत्यागम परिणाम;
}


M3_परिणाम Parse_Section_दत्त (M3Module* io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अङ्कः_दत्त_Segments;
_   (ReadLEB_u32(&अङ्कः_दत्त_Segments, &i_अष्टकानि, i_अन्तः));                               m3log (parse, "** Data [%d]", अङ्कः_दत्त_Segments);

    _throwif("too many data segments", अङ्कः_दत्त_Segments > d_m3_Max_Sane_दत्त_Segments);

    io_module->दत्त_Segments = m3_AllocArray(M3_दत्त_Segment, अङ्कः_दत्त_Segments);
    _throwifnull(io_module->दत्त_Segments);
    io_module->अङ्कः_दत्त_Segments = अङ्कः_दत्त_Segments;

    प्रति (u32 i = 0; i < अङ्कः_दत्त_Segments; ++i) {
        M3_दत्त_Segment* segment = &io_module->दत्त_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_अष्टकानि, i_अन्तः));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        यदि (flags == 2) {
_           (ReadLEB_u32(&segment->स्मृतिः_Region, &i_अष्टकानि, i_अन्तः));
        }

        यदि (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#यदि !d_m3_Has_Multi_स्मृतिः
            _throwif(m3Err_wasmMalformed, segment->स्मृतिः_Region != 0);
#यदि_अन्तः
            _throwif(m3Err_wasmMalformed, segment->स्मृतिः_Region >= io_module->अङ्कः_Memories);

            segment->init_व्यञ्जकम् = i_अष्टकानि;
_           (Parse_Init_व्यञ्जकम्(io_module, &i_अष्टकानि, i_अन्तः));
            segment->init_व्यञ्जकम्_आकार = (u32)(i_अष्टकानि - segment->init_व्यञ्जकम्);

            _throwif(m3_Err_wasm_Missing_Init_व्यञ्जकम्, segment->init_व्यञ्जकम्_आकार <= 1);
        }

_       (ReadLEB_u32(&segment->आकार, &i_अष्टकानि, i_अन्तः));
        segment->दत्त = i_अष्टकानि;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->स्मृतिः_Region, segment->init_व्यञ्जकम्_आकार, segment->आकार);
        i_अष्टकानि += segment->आकार;

        _throwif("data segment underflow", i_अष्टकानि > i_अन्तः);
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch:

    प्रत्यागम परिणाम;
}


M3_परिणाम Parse_Section_दत्त_गणनम् (M3Module* io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (ReadLEB_u32(&io_module->दत्त_गणनम्, &i_अष्टकानि, i_अन्तः));                       m3log (parse, "** DataCount [%d]", io_module->दत्त_गणनम्);

    io_module->has_दत्त_गणनम् = true;

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम Parse_Section_स्मृतिः (M3Module* io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अङ्कः_Memories;
_   (ReadLEB_u32(&अङ्कः_Memories, &i_अष्टकानि, i_अन्तः));                                m3log (parse, "** Memory [%d]", अङ्कः_Memories);

#यदि d_m3_Has_Multi_स्मृतिः
    _throwif(m3_Err_too_Many_स्मृतिः_Sections,
             (u64)अङ्कः_Memories + io_module->अङ्कः_Memories > d_m3_Max_Sane_Memories_गणनम्);
#अन्यथा
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_स्मृतिः_Sections, अङ्कः_Memories > 1);
    _throwif(m3_Err_too_Many_स्मृतिः_Sections, (u64)अङ्कः_Memories + io_module->अङ्कः_Memories > 1);
#यदि_अन्तः

    प्रति (u32 i = 0; i < अङ्कः_Memories; ++i) {
        M3_स्मृतिः_Info info;
_       (Parse_Type_स्मृतिः(&info, &i_अष्टकानि, i_अन्तः));
_       (Module_सङ्कलय_स्मृतिः(io_module, रिक्त, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम ParseSection_Global (M3Module* io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अङ्कः_Globals;
_   (ReadLEB_u32(&अङ्कः_Globals, &i_अष्टकानि, i_अन्तः));                                    m3log (parse, "** Global [%d]", अङ्कः_Globals);

    _throwif("too many globals", अङ्कः_Globals > d_m3_Max_Sane_Globals_गणनम्);

    प्रति (u32 i = 0; i < अङ्कः_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_मानम्_Type(io_module, &type, &i_अष्टकानि, i_अन्तः));
_       (ReadLEB_u7(&isMutable, &i_अष्टकानि, i_अन्तः));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [आधारः_Type_सम्बन्धि(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_सङ्कलय_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_व्यञ्जकम् = i_अष्टकानि;
_       (Parse_Init_व्यञ्जकम्(io_module, &i_अष्टकानि, i_अन्तः));
        global->init_व्यञ्जकम्_आकार = (u32)(i_अष्टकानि - global->init_व्यञ्जकम्);

        _throwif(m3_Err_wasm_Missing_Init_व्यञ्जकम्, global->init_व्यञ्जकम्_आकार <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_अष्टकानि != i_अन्तः);      // section size mismatch

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम ParseSection_Name (M3Module* io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    cstr_t name = रिक्त;

    यावत् (i_अष्टकानि < i_अन्तः) {
        u8  nameType;
        u32 payload_दीर्घता;

_       (ReadLEB_u7(&nameType, &i_अष्टकानि, i_अन्तः));
_       (ReadLEB_u32(&payload_दीर्घता, &i_अष्टकानि, i_अन्तः));

        अष्टकानि_t start = i_अष्टकानि;
        _throwif(m3Err_wasmSectionOverrun, payload_दीर्घता > (size_t)(i_अन्तः - start));

        यदि (nameType == 1) {
            u32 अङ्कः_नामानि;
_           (ReadLEB_u32(&अङ्कः_नामानि, &i_अष्टकानि, i_अन्तः));

            _throwif("too many names", अङ्कः_नामानि > d_m3_Max_Sane_Functions_गणनम्);

            प्रति (u32 i = 0; i < अङ्कः_नामानि; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_अष्टकानि, i_अन्तः));
_               (Read_utf8(&name, &i_अष्टकानि, i_अन्तः));

                यदि (index < io_module->अङ्कः_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    यदि (func->अङ्कः_नामानि == 0) {
                        func->नामानि[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->अङ्कः_नामानि = 1;

                        name = रिक्त; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_अष्टकानि = start + payload_दीर्घता;
    }

    _catch: प्रत्यागम परिणाम;
}


M3_परिणाम ParseSection_Custom (M3Module* io_module, अष्टकानि_t i_अष्टकानि, cbytes_t i_अन्तः)
{
    M3_परिणाम परिणाम = m3Err_none;

    cstr_t name = रिक्त;
_   (Read_utf8(&name, &i_अष्टकानि, i_अन्तः));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    यदि (वाक्यं_तुलय(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_अष्टकानि, i_अन्तः));
#यदि d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } अन्यथा यदि (वाक्यं_तुलय(name, "snapshot") == 0 || (वाक्यदैर्घ्यम्(name) > 9 && वाक्ये_तुलय(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, वाक्यदैर्घ्यम्(snapName) + 1);
        यदि (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->दत्त           = i_अष्टकानि;
        snap->आकार           = (u32)(i_अन्तः - i_अष्टकानि);
        snap->अग्र           = io_module->snapshots;
        io_module->snapshots = snap;
#यदि_अन्तः
    } अन्यथा यदि (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_अष्टकानि, i_अन्तः));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    प्रत्यागम परिणाम;
}


M3_परिणाम ParseModuleSection (M3Module* o_module, u8 i_sectionType, अष्टकानि_t i_अष्टकानि, u32 i_अङ्कः_अष्टकानि)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रकारनिर्देश M3_परिणाम (*M3Parser)(M3Module*, अष्टकानि_t, cbytes_t);

    स्थिर M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_सारणी,        // 4
        Parse_Section_स्मृतिः,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        ParseSection_Start,     // 8
        ParseSection_Element,   // 9
        Parse_Section_संहिता,      // 10
        Parse_Section_दत्त,      // 11
        Parse_Section_दत्त_गणनम्, // 12
#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
        Parse_Section_चिह्नकम्,       // 13
#यदि_अन्तः
    };

    M3Parser parser = रिक्त;

    यदि (i_sectionType < M3_गणनम्_सम्बन्धि(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    यदि (parser) {
        cbytes_t अन्तः = i_अष्टकानि + i_अङ्कः_अष्टकानि;
        परिणाम       = parser(o_module, i_अष्टकानि, अन्तः);
    } अन्यथा {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    प्रत्यागम परिणाम;
}


M3_परिणाम m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_अष्टकानि, u32 i_अङ्कः_अष्टकानि)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_अङ्कः_अष्टकानि);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_अङ्कः_अष्टकानि);
    module->startFunction = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ध्रुव u8* pos = i_अष्टकानि;
    ध्रुव u8* अन्तः = pos + i_अङ्कः_अष्टकानि;

    module->wasmStart = pos;
    module->wasm_अन्तः   = अन्तः;

    u32 magic, संस्करणम्;
_   (Read_u32(&magic, &pos, अन्तः));
_   (Read_u32(&संस्करणम्, &pos, अन्तः));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_संस्करणम्, संस्करणम् != 1);

#यदि d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
    // the tag section sits between memory (5) and global (6)
    स्थिर ध्रुव u8 sections_क्रमः[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#अन्यथा
    स्थिर ध्रुव u8 sections_क्रमः[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#यदि_अन्तः
    u8 expectedSection = 0;

    यावत् (pos < अन्तः) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, अन्तः));

        यदि (section != 0) {
            // Ensure sections appear only once and in order
            यावत् (sections_क्रमः[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_गणनम्_सम्बन्धि(sections_क्रमः) - 1);
            }
        }

        u32 section_दीर्घता;
_       (ReadLEB_u32(&section_दीर्घता, &pos, अन्तः));
        _throwif(m3Err_wasmMalformed, pos + section_दीर्घता > अन्तः);

_       (ParseModuleSection(module, section, pos, section_दीर्घता));

        pos += section_दीर्घता;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    यदि (module->अङ्कः_Functions > module->अङ्कः_Func_Imports) {
        IM3Function firstNonImport = &module->functions[module->अङ्कः_Func_Imports];
        _throwif(m3Err_wasmMalformed, firstNonImport->wasm == रिक्त);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_दत्त_गणनम् and module->दत्त_गणनम् != module->अङ्कः_दत्त_Segments);

} _catch:

    यदि (परिणाम) {
        m3_FreeModule(module);
        module = रिक्त;
    }

    *o_module = module;

    प्रत्यागम परिणाम;
}
