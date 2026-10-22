//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#समावेश "m3_env.h"
#समावेश "m3_compile.h"
#समावेश "m3_exception.h"
#समावेश "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#अगर d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_परिणाम ParseHeapType (IM3Module i_module, m3type_t* o_heapBits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    i64 heap;
_   (ReadLebSigned(&heap, 33, io_bytes, i_end));

    अगर (heap == -d_waType_funcref) {
        *o_heapBits = d_m3Type_heapAbstract;
    } अन्यथा अगर (heap == -d_waType_externref) {
        *o_heapBits = d_m3Type_refExtern | d_m3Type_heapAbstract;
#  अगर d_m3_Has_ढेर_Switching
    } अन्यथा अगर (heap == -d_waType_heap_cont) {
        *o_heapBits = d_m3Type_refCont | d_m3Type_heapAbstract;
    } अन्यथा अगर (heap == -d_waType_heap_nocont) {
        *o_heapBits = d_m3Type_refCont | d_m3Type_heapNone;
#  endif
    } अन्यथा अगर (heap >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) heap >= i_module->अंक_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[heap];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_heapBits = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_सूचकांक;
    } अन्यथा {
        _throw(m3Err_invalidTypeId);
    }

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_परिणाम Parse_मूल्य_Type (IM3Module i_module, m3type_t* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_bytes >= i_end);

#अगर d_m3HasTypedRefs
    अगर (**io_bytes == d_waEncode_ref or **io_bytes == d_waEncode_refNull) {
        u8 lead = *(*io_bytes)++;

        m3type_t heapBits = d_m3Type_heapAbstract;
_       (ParseHeapType(i_module, &heapBits, io_bytes, i_end));

        *o_type = d_m3Type_ref | heapBits | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        वापस परिणाम;
    }
#पूर्ण_अगर

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_bytes, i_end));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: वापस परिणाम;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
अटल
M3_परिणाम ReadType_TableType (IM3Module i_module, M3TableInfo* o_info, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_आकार = 0, max_आकार = 0;

    // the width of the limits, and of every index into the table
    u32 addrBits = 32;

_   (Parse_मूल्य_Type(i_module, &elemType, io_bytes, i_end));
#अगर d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(BaseTypeOf(elemType)));
#अन्यथा
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#पूर्ण_अगर

_   (ReadLEB_u7(&flag, io_bytes, i_end));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#अगर d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#अन्यथा
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#पूर्ण_अगर

    अगर (flag & 0x04u) {
        addrBits = 64;
    }

_   (ReadLebUnsigned(&init_आकार, addrBits, io_bytes, i_end));
    _throwif("table overflow", init_आकार > d_m3_Max_Sane_Table_आकार);

    अगर (flag & 0x01u) {
_       (ReadLebUnsigned(&max_आकार, addrBits, io_bytes, i_end));
        _throwif(m3Err_wasmMalformed, max_आकार < init_आकार);
        _throwif("table overflow", max_आकार > d_m3_Max_Sane_Table_आकार);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_आकार  = (u32)init_आकार;
    o_info->max_आकार   = (u32)max_आकार;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    वापस परिणाम;
}


अटल
M3_परिणाम ParseType_TableType (IM3Module io_module, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3TableInfo info;

_   (ReadType_TableType(io_module, &info, io_bytes, i_end));
_   (Module_AddTable(io_module, शून्य, &info, false /* isImport */));

    _catch: वापस परिणाम;
}


M3_परिणाम Parse_Init_व्यंजक_Typed (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end, m3type_t* o_type);

M3_परिणाम ParseType_Table (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अंक_Tables;
_   (ReadLEB_u32(&अंक_Tables, &i_bytes, i_end));                          m3log (parse, "** Table [%d]", अंक_Tables);

    _throwif("too many tables", (u64)अंक_Tables + io_module->अंक_Tables > d_m3_Max_Sane_Table_गिनती);

    क्रम (u32 i = 0; i < अंक_Tables; ++i) {
#अगर d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_व्यंजक = (i_bytes < i_end) and (*i_bytes == 0x40);
        u32  table_सूचकांक  = io_module->अंक_Tables;

        अगर (has_Init_व्यंजक) {
            u8 reserved;
            ++i_bytes;
_           (Read_u8(&reserved, &i_bytes, i_end));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (ParseType_TableType(io_module, &i_bytes, i_end));

        अगर (has_Init_व्यंजक) {
            IM3Table table = io_module->tables[table_सूचकांक];

            m3type_t initType;

            table->init_व्यंजक = i_bytes;
_           (Parse_Init_व्यंजक_Typed(io_module, &i_bytes, i_end, &initType));
            table->init_व्यंजक_आकार = (u32)(i_bytes - table->init_व्यंजक);
            _throwif(m3_Err_wasm_Missing_Init_व्यंजक, table->init_व्यंजक_आकार <= 1);

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(initType, table->type));
        } अन्यथा {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[table_सूचकांक]->type));
        }
#अन्यथा
_       (ParseType_TableType(io_module, &i_bytes, i_end));
#पूर्ण_अगर
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: वापस परिणाम;
}


M3_परिणाम ParseType_Memory (M3MemoryInfo* o_memory, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_Page_आकार = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addrBits = 32;

_   (ReadLEB_u7(&flag, io_bytes, i_end));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 known = 0x09u;
#अगर d_m3HasAtomics
        known |= 0x02u;
#पूर्ण_अगर
#अगर d_m3HasMemory64
        known |= 0x04u;
#पूर्ण_अगर
        _throwif(m3Err_wasmMalformed, flag & ~known);
    }

    o_memory->isShared = (flag & (1u << 1)) != 0;

    o_memory->isMemory64 = (flag & (1u << 2)) != 0;
    अगर (o_memory->isMemory64) {
        addrBits = 64;
    }

_   (ReadLebUnsigned(&o_memory->initPages, addrBits, io_bytes, i_end));

    o_memory->maxPages = 0;
    o_memory->hasMax   = (flag & (1u << 0)) != 0;
    अगर (o_memory->hasMax) {
_       (ReadLebUnsigned(&o_memory->maxPages, addrBits, io_bytes, i_end));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_memory->maxPages < o_memory->initPages);
    }

    _throwif("shared memory must have maximum", o_memory->isShared and not o_memory->hasMax);

    o_memory->page_आकार = 0;
    अगर (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_Page_आकार, io_bytes, i_end));

        _throwif("invalid custom page size", log_Page_आकार > 16);

        o_memory->page_आकार = 1u << log_Page_आकार;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 maxPagesAllowed = (log_Page_आकार or addrBits < 64)
                                ? (UINT64_C(1) << (addrBits - log_Page_आकार))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_memory->initPages > maxPagesAllowed);
        अगर (o_memory->hasMax) {
            _throwif(m3Err_wasmMalformed, o_memory->maxPages > maxPagesAllowed);
        }
    }

    _catch: वापस परिणाम;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
अटल
M3_परिणाम ParseFuncType (IM3Module io_module, IM3FuncType* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = शून्य;
_try {
    u32 अंक_Args;
_   (ReadLEB_u32(&अंक_Args, io_bytes, i_end));

    _throwif(m3Err_tooManyArgsRets, अंक_Args > d_m3_Max_Sane_Function_Arg_Ret_गिनती);
#अगर M3_HAS_VLA
    m3type_t argTypes[अंक_Args + 1]; // make ubsan happy
#अन्यथा
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_गिनती];
#पूर्ण_अगर
    क्रम (u32 a = 0; a < अंक_Args; ++a) {
_       (Parse_मूल्य_Type(io_module, &argTypes[a], io_bytes, i_end));
    }

    u32 अंक_Rets;
_   (ReadLEB_u32(&अंक_Rets, io_bytes, i_end));
    _throwif(m3Err_tooManyArgsRets, (u64)(अंक_Rets) + अंक_Args > d_m3_Max_Sane_Function_Arg_Ret_गिनती);

_   (AllocFuncType(&ftype, अंक_Rets + अंक_Args));
    ftype->अंक_Args = अंक_Args;
    ftype->अंक_Rets = अंक_Rets;

    क्रम (u32 r = 0; r < अंक_Rets; ++r) {
_       (Parse_मूल्य_Type(io_module, &ftype->types[r], io_bytes, i_end));
    }
    स_नकल(ftype->types + अंक_Rets, argTypes, अंक_Args * माप(m3type_t));

    *o_type = ftype;
    ftype   = शून्य;

} _catch:
    m3_Free(ftype);
    वापस परिणाम;
}


#अगर d_m3_Has_ढेर_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
अटल
M3_परिणाम ParseContType (IM3Module io_module, IM3FuncType* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = शून्य;
_try {
    i64 ft_सूचकांक;
_   (ReadLebSigned(&ft_सूचकांक, 33, io_bytes, i_end));
    _throwif(m3Err_wasmMalformed, ft_सूचकांक < 0 or (u64) ft_सूचकांक >= io_module->अंक_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_सूचकांक];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 योग = underlying->अंक_Rets + underlying->अंक_Args;
_   (AllocFuncType(&ftype, योग));
    ftype->अंक_Rets        = underlying->अंक_Rets;
    ftype->अंक_Args        = underlying->अंक_Args;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    अगर (योग) {
        स_नकल(ftype->types, underlying->types, योग * माप(m3type_t));
    }

    *o_type = ftype;
    ftype   = शून्य;

} _catch:
    m3_Free(ftype);
    वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasStackSwitching


अटल
M3_परिणाम ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              bytes_t* io_bytes, cbytes_t i_end)
{
#अगर d_m3_Has_ढेर_Switching
    अगर (i_form == -35) {
        वापस ParseContType(io_module, o_type, io_bytes, i_end);
    }
#पूर्ण_अगर
    अगर (i_form != -32) { // for Wasm MVP
        वापस m3Err_wasmMalformed;
    }

    वापस ParseFuncType(io_module, o_type, io_bytes, i_end);
}


#अगर d_m3_Has_ढेर_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
अटल
M3_परिणाम SkipTypeDefinition (i8 i_form, bytes_t* io_bytes, cbytes_t i_end)
{
_try {
    अगर (i_form == -35) {
        i64 ft_सूचकांक;
_       (ReadLebSigned(&ft_सूचकांक, 33, io_bytes, i_end));
        वापस परिणाम;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    क्रम (u32 half = 0; half < 2; ++half) {
        u32 गिनती;
_       (ReadLEB_u32(&गिनती, io_bytes, i_end));
        _throwif(m3Err_tooManyArgsRets, गिनती > d_m3_Max_Sane_Function_Arg_Ret_गिनती);

        क्रम (u32 v = 0; v < गिनती; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_bytes, i_end));

            अगर (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 heap;
_               (ReadLebSigned(&heap, 33, io_bytes, i_end));
            }
        }
    }

} _catch: वापस परिणाम;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
अटल
M3_परिणाम ParseRecGroup (IM3Module io_module, u32 i_गिनती, u32 i_first_सूचकांक,
                        bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType* placeholders = शून्य;
_try {
    bytes_t groupStart = *io_bytes;

    placeholders = m3_AllocArray(IM3FuncType, i_गिनती);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    क्रम (u32 j = 0; j < i_गिनती; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_bytes, i_end));
_       (SkipTypeDefinition(form, io_bytes, i_end));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 firstCanonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_गिनती, &firstCanonical));

    क्रम (u32 j = 0; j < i_गिनती; ++j) {
        placeholders[j]->canonical_सूचकांक        = (u16)(firstCanonical + j);
        io_module->funcTypes[i_first_सूचकांक + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    क्रम (u32 pass = 0; pass < 2; ++pass) {
        bytes_t bytes = groupStart;

        क्रम (u32 j = 0; j < i_गिनती; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &bytes, i_end));

            अगर ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &bytes, i_end));
                जारी;
            }

            IM3FuncType ftype = शून्य;
_           (ParseTypeDefinition(io_module, form, &ftype, &bytes, i_end));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_सूचकांक);

            m3_Free(placeholders[j]);
            placeholders[j]                        = शून्य;
            io_module->funcTypes[i_first_सूचकांक + j] = ftype;
        }

        अगर (pass == 1) {
            *io_bytes = bytes;
        }
    }

} _catch:
    अगर (placeholders) {
        क्रम (u32 j = 0; j < i_गिनती; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasStackSwitching


M3_परिणाम ParseSection_Type (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = शून्य;

_try {
    u32 अंक_Types;
_   (ReadLEB_u32(&अंक_Types, &i_bytes, i_end));                                      m3log (parse, "** Type [%d]", अंक_Types);

    _throwif("too many types", अंक_Types > d_m3_Max_Sane_Types_गिनती);

    अगर (अंक_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, अंक_Types);
        _throwifnull(io_module->funcTypes);
        io_module->अंक_Func_Types = अंक_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#अगर d_m3_Has_ढेर_Switching
        u32 capacity = अंक_Types;
#पूर्ण_अगर
        u32 index = 0;

        क्रम (u32 i = 0; i < अंक_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_bytes, i_end));

#अगर d_m3_Has_ढेर_Switching
            अगर (form == -50) { // rec
                u32 गिनती;
_               (ReadLEB_u32(&गिनती, &i_bytes, i_end));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + गिनती + (अंक_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_गिनती);

                अगर (needed > capacity) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, capacity);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    capacity                = grown;
                    io_module->अंक_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                अगर (गिनती == 1) {
_                   (ReadLEB_i7(&form, &i_bytes, i_end));
                } अन्यथा {
_                   (ParseRecGroup(io_module, गिनती, index, &i_bytes, i_end));
                    index += गिनती;
                    जारी;
                }
            }
#पूर्ण_अगर

_           (ParseTypeDefinition(io_module, form, &ftype, &i_bytes, i_end));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_AddFuncType(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = शून्य; // ownership transferred to environment
        }

        io_module->अंक_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

} _catch:

    अगर (परिणाम) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->अंक_Func_Types = 0;
    }

    वापस परिणाम;
}


M3_परिणाम ParseSection_Function (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अंक_Functions;
_   (ReadLEB_u32(&अंक_Functions, &i_bytes, i_end));                                  m3log (parse, "** Function [%d]", अंक_Functions);

    _throwif("too many functions", अंक_Functions > d_m3_Max_Sane_Functions_गिनती);

_   (Module_PreallocFunctions(io_module, io_module->अंक_Functions + अंक_Functions));

    क्रम (u32 i = 0; i < अंक_Functions; ++i) {
        u32 func_Type_सूचकांक;
_       (ReadLEB_u32(&func_Type_सूचकांक, &i_bytes, i_end));

_       (Module_AddFunction(io_module, func_Type_सूचकांक, शून्य /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: वापस परिणाम;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
प्रकार काष्ठा M3ImportDesc {
    u8           kind;
    u32          type_सूचकांक;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3TableInfo  table;
    M3MemoryInfo memory;
} M3ImportDesc;


#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
अटल
M3_परिणाम ReadTagType (IM3Module i_module, u32* o_type_सूचकांक, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_bytes, i_end));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_सूचकांक, io_bytes, i_end));
    _throwif(m3Err_unknownType, not i_module or *o_type_सूचकांक >= i_module->अंक_Func_Types);

#  अगर !d_m3_Has_ढेर_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_अंक_Results(i_module->funcTypes[*o_type_सूचकांक]) != 0);
#  endif

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
अटल
M3_परिणाम ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (Read_u8(&o_desc->kind, io_bytes, i_end));

    चयन (o_desc->kind) {
    हाल d_externalKind_function:
_       (ReadLEB_u32(&o_desc->type_सूचकांक, io_bytes, i_end));
        अवरोध;

    हाल d_externalKind_table:
_       (ReadType_TableType(i_module, &o_desc->table, io_bytes, i_end));
        अवरोध;

    हाल d_externalKind_memory:
_       (ParseType_Memory(&o_desc->memory, io_bytes, i_end));
        अवरोध;

    हाल d_externalKind_global:
_       (Parse_मूल्य_Type(i_module, &o_desc->type, io_bytes, i_end));
_       (ReadLEB_u7(&o_desc->isMutable, io_bytes, i_end));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [BaseTypeOf(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        अवरोध;

#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
    हाल d_externalKind_tag:
_       (ReadTagType(i_module, &o_desc->type_सूचकांक, io_bytes, i_end));
        अवरोध;
#पूर्ण_अगर

    शेष:
        _throw(m3Err_wasmMalformed);
    }

    _catch: वापस परिणाम;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
अटल
M3_परिणाम ApplyImportDesc (IM3Module io_module, स्थिर M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3ImportInfo clearImport = { शून्य, शून्य };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->kind, io_import->moduleUtf8, io_import->fieldUtf8);
    चयन (i_desc->kind) {
    हाल d_externalKind_function: {
_       (Module_AddFunction(io_module, i_desc->type_सूचकांक, io_import))
        *io_import = clearImport;

        io_module->अंक_Func_Imports++;
    } अवरोध;

    हाल d_externalKind_table: {
        IM3Table table;
_       (Module_AddTable(io_module, &table, &i_desc->table, true /* isImport */));
        table->import = *io_import;
        *io_import    = clearImport;
    } अवरोध;

    हाल d_externalKind_memory: {
        IM3Memory memory;

#अगर d_m3HasMultiMemory
        _throwif(m3Err_tooManyMemorySections, io_module->अंक_Memories >= d_m3_Max_Sane_Memories_गिनती);
#अन्यथा
        // MVP: one memory per module, imported or declared
        _throwif(m3Err_tooManyMemorySections, io_module->अंक_Memories >= 1);
#पूर्ण_अगर

_       (Module_AddMemory(io_module, &memory, &i_desc->memory, true /* isImport */));
        memory->import = *io_import;
        *io_import     = clearImport;
    } अवरोध;

    हाल d_externalKind_global: {
        IM3Global global;
_       (Module_AddGlobal(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } अवरोध;

#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
    हाल d_externalKind_tag: {
        IM3Tag tag;
_       (Module_AddTag(io_module, &tag, io_module->funcTypes[i_desc->type_सूचकांक], true /* isImport */));
        tag->import = *io_import;
        *io_import  = clearImport;
    } अवरोध;
#पूर्ण_अगर

    शेष:
        _throw(m3Err_wasmMalformed);
    }

    _catch: वापस परिणाम;
}


#अगर d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
अटल
bool IsEmptyName (bytes_t i_bytes, cbytes_t i_end)
{
    u32 लंबाई;
    वापस (not ReadLEB_u32(&लंबाई, &i_bytes, i_end)) and लंबाई == 0;
}

#पूर्ण_अगर // d_m3HasCompactImports


#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching

M3_परिणाम ParseSection_Tag (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अंक_Tags;
_   (ReadLEB_u32(&अंक_Tags, &i_bytes, i_end));                                       m3log (parse, "** Tag [%d]", अंक_Tags);

    _throwif("too many tags", अंक_Tags > d_m3_Max_Sane_Tags_गिनती);

    क्रम (u32 i = 0; i < अंक_Tags; ++i) {
        u32 type_सूचकांक;
_       (ReadTagType(io_module, &type_सूचकांक, &i_bytes, i_end));                      m3log (parse, "    tag: [%d] type: %d", i, type_सूचकांक);

_       (Module_AddTag(io_module, शून्य, io_module->funcTypes[type_सूचकांक], false));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: वापस परिणाम;
}

#पूर्ण_अगर // d_m3HasExceptionHandling


M3_परिणाम ParseSection_Import (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    M3ImportInfo import = { शून्य, शून्य };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 अंक_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 अंक_Entries;
_   (ReadLEB_u32(&अंक_Entries, &i_bytes, i_end));                                    m3log (parse, "** Import [%d]", अंक_Entries);

    _throwif("too many imports", अंक_Entries > d_m3_Max_Sane_Imports_गिनती);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->अंक_Functions + अंक_Entries));

    क्रम (u32 i = 0; i < अंक_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_bytes, i_end));

#अगर d_m3HasCompactImports
        bytes_t fieldStart = i_bytes;
#पूर्ण_अगर
_       (Read_utf8(&import.fieldUtf8, &i_bytes, i_end));

#अगर d_m3HasCompactImports
        u8 compact = (i_bytes < i_end) ? *i_bytes : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        अगर ((compact == d_compactImports_perItemType or compact == d_compactImports_sharedType) and
            IsEmptyName(fieldStart, i_end)) {
            ++i_bytes;

            m3_Free(import.fieldUtf8);         // it only marked the compact form
            import.fieldUtf8 = शून्य;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            अगर (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_bytes, i_end));
            }

            u32 अंक_वस्तुएँ;
_           (ReadLEB_u32(&अंक_वस्तुएँ, &i_bytes, i_end));                              m3log (parse, "    compact: %d import(s) of '%s'", अंक_वस्तुएँ, import.moduleUtf8);

            _throwif("too many imports", (u64)अंक_Imports + अंक_वस्तुएँ > d_m3_Max_Sane_Imports_गिनती);
            अंक_Imports += अंक_वस्तुएँ;

_           (Module_PreallocFunctions(io_module, io_module->अंक_Functions + अंक_वस्तुएँ));

            क्रम (u32 j = 0; j < अंक_वस्तुएँ; ++j) {
                M3ImportInfo item = { शून्य, शून्य };

                item.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, म_माप(import.moduleUtf8) + 1);

                अगर (item.moduleUtf8) {
                    परिणाम = Read_utf8(&item.fieldUtf8, &i_bytes, i_end);

                    // the shared form reuses the externtype read above
                    अगर (not परिणाम and compact == d_compactImports_perItemType) {
                        परिणाम = ReadImportDesc(io_module, &desc, &i_bytes, i_end);
                    }

                    अगर (not परिणाम) {
                        परिणाम = ApplyImportDesc(io_module, &desc, &item);
                    }
                } अन्यथा {
                    परिणाम = m3Err_mallocFailed;
                }

                FreeImportInfo(&item);
                _throwif(परिणाम, परिणाम);
            }

            FreeImportInfo(&import);
            जारी;
        }
#पूर्ण_अगर // d_m3HasCompactImports

        _throwif("too many imports", अंक_Imports >= d_m3_Max_Sane_Imports_गिनती);
        ++अंक_Imports;

_       (ReadImportDesc(io_module, &desc, &i_bytes, i_end));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    वापस परिणाम;
}


M3_परिणाम ParseSection_Export (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम    परिणाम = m3Err_none;
    स्थिर अक्षर* utf8   = शून्य;

    u32 अंक_Exports;
_   (ReadLEB_u32(&अंक_Exports, &i_bytes, i_end));                                    m3log (parse, "** Export [%d]", अंक_Exports);

    _throwif("too many exports", अंक_Exports > d_m3_Max_Sane_Exports_गिनती);

    // the section order check lets this section through once, so this is
    // every export the module has
    अगर (अंक_Exports) {
        io_module->exports = m3_AllocArray(M3Export, अंक_Exports);
        _throwifnull(io_module->exports);
    }

    क्रम (u32 i = 0; i < अंक_Exports; ++i) {
        u8  exportKind;
        u32 index;
        u32 name_लंबाई;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            bytes_t लंबाई_Bytes = i_bytes;
_           (ReadLEB_u32(&name_लंबाई, &लंबाई_Bytes, i_end));
        }

_       (Read_utf8(&utf8, &i_bytes, i_end));
_       (Read_u8(&exportKind, &i_bytes, i_end));
_       (ReadLEB_u32(&index, &i_bytes, i_end));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) exportKind, utf8);

#अगर d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        क्रम (u32 j = 0; j < i; ++j) {
            स्थिर M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_लंबाई == name_लंबाई and स_भेद(other->name, utf8, name_लंबाई) == 0);
        }
#पूर्ण_अगर

        अगर (exportKind == d_externalKind_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अंक_Functions);
            IM3Function func = &(io_module->functions[index]);
            अगर (func->अंक_Names < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_लंबाई + 1);
                _throwifnull(name);
                func->names[func->अंक_Names++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } अन्यथा अगर (exportKind == d_externalKind_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अंक_Globals);
        } अन्यथा अगर (exportKind == d_externalKind_memory) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अंक_Memories);
        } अन्यथा अगर (exportKind == d_externalKind_table) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अंक_Tables);
        }
#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
        अन्यथा अगर (exportKind == d_externalKind_tag) {
            _throwif(m3Err_wasmMalformed, index >= io_module->अंक_Tags);
        }
#पूर्ण_अगर

        M3Export* entry   = &io_module->exports[io_module->अंक_Exports++];
        entry->name       = utf8;
        entry->name_लंबाई = name_लंबाई;
        entry->index      = index;
        entry->kind       = exportKind;

        utf8 = शून्य; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

_catch:
    m3_Free(utf8);
    वापस परिणाम;
}


M3_परिणाम ParseSection_Start (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 start_Func_सूचकांक;
_   (ReadLEB_u32(&start_Func_सूचकांक, &i_bytes, i_end));                                  m3log (parse, "** Start Function: %d", start_Func_सूचकांक);

    अगर (start_Func_सूचकांक < io_module->अंक_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[start_Func_सूचकांक];
        अगर (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->अंक_Args != 0 || func->funcType->अंक_Rets != 0);
        }

        io_module->startFunction = start_Func_सूचकांक;
    } अन्यथा {
        परिणाम = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: वापस परिणाम;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_परिणाम Parse_Init_व्यंजक_Typed (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end, m3type_t* o_type)
{
    M3_परिणाम परिणाम = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#अगर defined(d_m3PreferStaticAlloc)
    अटल M3Compilation compilation;
#अन्यथा
    M3Compilation compilation;
#पूर्ण_अगर
    compilation = (M3Compilation){ .runtime = शून्य, .module = io_module, .wasm = *io_bytes, .wasmEnd = i_end, .is_Init_व्यंजक = true };

    परिणाम = CompileBlockStatements(&compilation);

    *io_bytes = compilation.wasm;

    अगर (o_type) {
        *o_type = compilation.ढेर_सूचकांक ? compilation.type_ढेर[compilation.ढेर_सूचकांक - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    वापस परिणाम;
}


M3_परिणाम Parse_Init_व्यंजक (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end)
{
    वापस Parse_Init_व्यंजक_Typed(io_module, io_bytes, i_end, शून्य);
}


M3_परिणाम ParseSection_Element (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32     अंक_Segments;
    bytes_t pos;
_   (ReadLEB_u32(&अंक_Segments, &i_bytes, i_end));                            m3log (parse, "** Element [%d]", अंक_Segments);

    _throwif("too many element segments", अंक_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, अंक_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->अंक_Element_Segments = अंक_Segments;
    io_module->elementSectionEnd  = i_end;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_bytes;
    क्रम (u32 i = 0; i < अंक_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_end));
#अगर d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#अन्यथा
        _throwif(m3Err_wasmMalformed, flags != c_m3Elem_active);
#पूर्ण_अगर

        segment->mode   = flags & 0x3;
        segment->is_व्यंजक = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        अगर (segment->mode == c_m3Elem_activeIdx) {
_           (ReadLEB_u32(&segment->table_सूचकांक, &pos, i_end));
        }

        bool isActive = (segment->mode == c_m3Elem_active or segment->mode == c_m3Elem_activeIdx);

        अगर (isActive) {
            _throwif(m3Err_wasmMalformed, segment->table_सूचकांक >= io_module->अंक_Tables);

            segment->init_व्यंजक = pos;
_           (Parse_Init_व्यंजक(io_module, &pos, i_end));
            segment->init_व्यंजक_आकार = (u32)(pos - segment->init_व्यंजक);
            _throwif(m3_Err_wasm_Missing_Init_व्यंजक, segment->init_व्यंजक_आकार <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        अगर (segment->mode != c_m3Elem_active) {
            अगर (segment->is_व्यंजक) {
_               (Parse_मूल्य_Type(io_module, &segment->type, &pos, i_end));
                _throwif(m3Err_wasmMalformed, not IsRefType(BaseTypeOf(segment->type)));
            } अन्यथा {
                u8 elemKind;
_               (Read_u8(&elemKind, &pos, i_end));
                _throwif(m3Err_wasmMalformed, elemKind != 0x00);   // funcref
            }
        }

        अगर (isActive) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->table_सूचकांक]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->अंक_Elements, &pos, i_end));
        _throwif("table overflow", segment->अंक_Elements > d_m3_Max_Sane_Table_आकार);

        segment->elements = pos;

        क्रम (u32 e = 0; e < segment->अंक_Elements; ++e) {
            अगर (segment->is_व्यंजक) {
_               (Parse_Init_व्यंजक(io_module, &pos, i_end))
            } अन्यथा {
                u32 func_सूचकांक;
_               (ReadLEB_u32(&func_सूचकांक, &pos, i_end));
                _throwif("function index out of range", func_सूचकांक >= io_module->अंक_Functions);
_               (Module_DeclareFunction(io_module, func_सूचकांक));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_end);           // section size mismatch

    _catch: वापस परिणाम;
}


M3_परिणाम ParseSection_Code (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम;

    u32 अंक_Functions;
_   (ReadLEB_u32(&अंक_Functions, &i_bytes, i_end));                                  m3log (parse, "** Code [%d]", अंक_Functions);

    अगर (अंक_Functions != io_module->अंक_Functions - io_module->अंक_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    क्रम (u32 f = 0; f < अंक_Functions; ++f) {
        स्थिर u8* start = i_bytes;

        u32 आकार;
_       (ReadLEB_u32(&आकार, &i_bytes, i_end));

        अगर (आकार) {
            i_bytes += आकार;

            अगर (i_bytes <= i_end) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->अंक_Func_Imports);

                func->module  = io_module;
                func->wasm    = start;
                func->wasmEnd = i_bytes;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } अन्यथा {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    अगर (not परिणाम and i_bytes != i_end) {
        परिणाम = m3Err_wasmSectionUnderrun;
    }

    वापस परिणाम;
}


M3_परिणाम Parse_Section_तथ्य (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अंक_तथ्य_Segments;
_   (ReadLEB_u32(&अंक_तथ्य_Segments, &i_bytes, i_end));                               m3log (parse, "** Data [%d]", अंक_तथ्य_Segments);

    _throwif("too many data segments", अंक_तथ्य_Segments > d_m3_Max_Sane_तथ्य_Segments);

    io_module->तथ्य_Segments = m3_AllocArray(M3_तथ्य_Segment, अंक_तथ्य_Segments);
    _throwifnull(io_module->तथ्य_Segments);
    io_module->अंक_तथ्य_Segments = अंक_तथ्य_Segments;

    क्रम (u32 i = 0; i < अंक_तथ्य_Segments; ++i) {
        M3_तथ्य_Segment* segment = &io_module->तथ्य_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_bytes, i_end));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        अगर (flags == 2) {
_           (ReadLEB_u32(&segment->memoryRegion, &i_bytes, i_end));
        }

        अगर (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#अगर !d_m3HasMultiMemory
            _throwif(m3Err_wasmMalformed, segment->memoryRegion != 0);
#पूर्ण_अगर
            _throwif(m3Err_wasmMalformed, segment->memoryRegion >= io_module->अंक_Memories);

            segment->init_व्यंजक = i_bytes;
_           (Parse_Init_व्यंजक(io_module, &i_bytes, i_end));
            segment->init_व्यंजक_आकार = (u32)(i_bytes - segment->init_व्यंजक);

            _throwif(m3_Err_wasm_Missing_Init_व्यंजक, segment->init_व्यंजक_आकार <= 1);
        }

_       (ReadLEB_u32(&segment->आकार, &i_bytes, i_end));
        segment->तथ्य = i_bytes;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->memoryRegion, segment->init_व्यंजक_आकार, segment->आकार);
        i_bytes += segment->आकार;

        _throwif("data segment underflow", i_bytes > i_end);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch:

    वापस परिणाम;
}


M3_परिणाम Parse_Section_तथ्य_गिनती (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

_   (ReadLEB_u32(&io_module->तथ्य_गिनती, &i_bytes, i_end));                       m3log (parse, "** DataCount [%d]", io_module->तथ्य_गिनती);

    io_module->has_तथ्य_गिनती = true;

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: वापस परिणाम;
}


M3_परिणाम ParseSection_Memory (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अंक_Memories;
_   (ReadLEB_u32(&अंक_Memories, &i_bytes, i_end));                                m3log (parse, "** Memory [%d]", अंक_Memories);

#अगर d_m3HasMultiMemory
    _throwif(m3Err_tooManyMemorySections,
             (u64)अंक_Memories + io_module->अंक_Memories > d_m3_Max_Sane_Memories_गिनती);
#अन्यथा
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3Err_tooManyMemorySections, अंक_Memories > 1);
    _throwif(m3Err_tooManyMemorySections, (u64)अंक_Memories + io_module->अंक_Memories > 1);
#पूर्ण_अगर

    क्रम (u32 i = 0; i < अंक_Memories; ++i) {
        M3MemoryInfo info;
_       (ParseType_Memory(&info, &i_bytes, i_end));
_       (Module_AddMemory(io_module, शून्य, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: वापस परिणाम;
}


M3_परिणाम ParseSection_Global (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    u32 अंक_Globals;
_   (ReadLEB_u32(&अंक_Globals, &i_bytes, i_end));                                    m3log (parse, "** Global [%d]", अंक_Globals);

    _throwif("too many globals", अंक_Globals > d_m3_Max_Sane_Globals_गिनती);

    क्रम (u32 i = 0; i < अंक_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_मूल्य_Type(io_module, &type, &i_bytes, i_end));
_       (ReadLEB_u7(&isMutable, &i_bytes, i_end));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [BaseTypeOf(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_AddGlobal(io_module, &global, type, isMutable, false /* isImport */));

        global->init_व्यंजक = i_bytes;
_       (Parse_Init_व्यंजक(io_module, &i_bytes, i_end));
        global->init_व्यंजक_आकार = (u32)(i_bytes - global->init_व्यंजक);

        _throwif(m3_Err_wasm_Missing_Init_व्यंजक, global->init_व्यंजक_आकार <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: वापस परिणाम;
}


M3_परिणाम ParseSection_Name (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    cstr_t name = शून्य;

    जबतक (i_bytes < i_end) {
        u8  nameType;
        u32 payload_लंबाई;

_       (ReadLEB_u7(&nameType, &i_bytes, i_end));
_       (ReadLEB_u32(&payload_लंबाई, &i_bytes, i_end));

        bytes_t start = i_bytes;
        _throwif(m3Err_wasmSectionOverrun, payload_लंबाई > (माप_प्रकार)(i_end - start));

        अगर (nameType == 1) {
            u32 अंक_Names;
_           (ReadLEB_u32(&अंक_Names, &i_bytes, i_end));

            _throwif("too many names", अंक_Names > d_m3_Max_Sane_Functions_गिनती);

            क्रम (u32 i = 0; i < अंक_Names; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_bytes, i_end));
_               (Read_utf8(&name, &i_bytes, i_end));

                अगर (index < io_module->अंक_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    अगर (func->अंक_Names == 0) {
                        func->names[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->अंक_Names = 1;

                        name = शून्य; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_bytes = start + payload_लंबाई;
    }

    _catch: वापस परिणाम;
}


M3_परिणाम ParseSection_Custom (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_परिणाम परिणाम = m3Err_none;

    cstr_t name = शून्य;
_   (Read_utf8(&name, &i_bytes, i_end));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    अगर (म_भेद(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_bytes, i_end));
#अगर d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } अन्यथा अगर (म_भेद(name, "snapshot") == 0 || (म_माप(name) > 9 && म_भेदन(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, म_माप(snapName) + 1);
        अगर (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->तथ्य           = i_bytes;
        snap->आकार           = (u32)(i_end - i_bytes);
        snap->अग्र           = io_module->snapshots;
        io_module->snapshots = snap;
#पूर्ण_अगर
    } अन्यथा अगर (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_bytes, i_end));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    वापस परिणाम;
}


M3_परिणाम ParseModuleSection (M3Module* o_module, u8 i_sectionType, bytes_t i_bytes, u32 i_अंक_Bytes)
{
    M3_परिणाम परिणाम = m3Err_none;

    प्रकार M3_परिणाम (*M3Parser)(M3Module*, bytes_t, cbytes_t);

    अटल M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        ParseType_Table,        // 4
        ParseSection_Memory,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        ParseSection_Start,     // 8
        ParseSection_Element,   // 9
        ParseSection_Code,      // 10
        Parse_Section_तथ्य,      // 11
        Parse_Section_तथ्य_गिनती, // 12
#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
        ParseSection_Tag,       // 13
#पूर्ण_अगर
    };

    M3Parser parser = शून्य;

    अगर (i_sectionType < M3_गिनती_OF(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    अगर (parser) {
        cbytes_t end = i_bytes + i_अंक_Bytes;
        परिणाम       = parser(o_module, i_bytes, end);
    } अन्यथा {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    वापस परिणाम;
}


M3_परिणाम m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_bytes, u32 i_अंक_Bytes)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_अंक_Bytes);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_अंक_Bytes);
    module->startFunction = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    स्थिर u8* pos = i_bytes;
    स्थिर u8* end = pos + i_अंक_Bytes;

    module->wasmStart = pos;
    module->wasmEnd   = end;

    u32 magic, version;
_   (Read_u32(&magic, &pos, end));
_   (Read_u32(&version, &pos, end));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3Err_incompatibleWasmVersion, version != 1);

#अगर d_m3HasExceptionHandling || d_m3_Has_ढेर_Switching
    // the tag section sits between memory (5) and global (6)
    अटल स्थिर u8 sectionsOrder[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#अन्यथा
    अटल स्थिर u8 sectionsOrder[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#पूर्ण_अगर
    u8 expectedSection = 0;

    जबतक (pos < end) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, end));

        अगर (section != 0) {
            // Ensure sections appear only once and in order
            जबतक (sectionsOrder[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_गिनती_OF(sectionsOrder) - 1);
            }
        }

        u32 section_लंबाई;
_       (ReadLEB_u32(&section_लंबाई, &pos, end));
        _throwif(m3Err_wasmMalformed, pos + section_लंबाई > end);

_       (ParseModuleSection(module, section, pos, section_लंबाई));

        pos += section_लंबाई;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    अगर (module->अंक_Functions > module->अंक_Func_Imports) {
        IM3Function firstNonImport = &module->functions[module->अंक_Func_Imports];
        _throwif(m3Err_wasmMalformed, firstNonImport->wasm == शून्य);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_तथ्य_गिनती and module->तथ्य_गिनती != module->अंक_तथ्य_Segments);

} _catch:

    अगर (परिणाम) {
        m3_FreeModule(module);
        module = शून्य;
    }

    *o_module = module;

    वापस परिणाम;
}
