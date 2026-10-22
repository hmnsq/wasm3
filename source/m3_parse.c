//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#অন্তর্ভাব "m3_env.h"
#অন্তর্ভাব "m3_compile.h"
#অন্তর্ভাব "m3_exception.h"
#অন্তর্ভাব "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#যদি d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_ফলাফল ParseHeapType (IM3Module i_module, m3type_t* o_heapBits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    i64 heap;
_   (ReadLebSigned(&heap, 33, io_bytes, i_end));

    যদি (heap == -d_waType_funcref) {
        *o_heapBits = d_m3Type_heapAbstract;
    } অন্যথা যদি (heap == -d_waType_externref) {
        *o_heapBits = d_m3Type_refExtern | d_m3Type_heapAbstract;
#  যদি d_m3_Has_স্তূপ_Switching
    } অন্যথা যদি (heap == -d_waType_heap_cont) {
        *o_heapBits = d_m3Type_refCont | d_m3Type_heapAbstract;
    } অন্যথা যদি (heap == -d_waType_heap_nocont) {
        *o_heapBits = d_m3Type_refCont | d_m3Type_heapNone;
#  endif
    } অন্যথা যদি (heap >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) heap >= i_module->অঙ্ক_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[heap];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_heapBits = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_সূচক;
    } অন্যথা {
        _throw(m3Err_invalidTypeId);
    }

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_ফলাফল Parse_মান_Type (IM3Module i_module, m3type_t* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_bytes >= i_end);

#যদি d_m3HasTypedRefs
    যদি (**io_bytes == d_waEncode_ref or **io_bytes == d_waEncode_refNull) {
        u8 lead = *(*io_bytes)++;

        m3type_t heapBits = d_m3Type_heapAbstract;
_       (ParseHeapType(i_module, &heapBits, io_bytes, i_end));

        *o_type = d_m3Type_ref | heapBits | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        ফেরত ফলাফল;
    }
#endif

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_bytes, i_end));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: ফেরত ফলাফল;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
স্থির
M3_ফলাফল ReadType_TableType (IM3Module i_module, M3TableInfo* o_info, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_আকার = 0, max_আকার = 0;

    // the width of the limits, and of every index into the table
    u32 addrBits = 32;

_   (Parse_মান_Type(i_module, &elemType, io_bytes, i_end));
#যদি d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(BaseTypeOf(elemType)));
#অন্যথা
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#endif

_   (ReadLEB_u7(&flag, io_bytes, i_end));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#যদি d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#অন্যথা
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#endif

    যদি (flag & 0x04u) {
        addrBits = 64;
    }

_   (ReadLebUnsigned(&init_আকার, addrBits, io_bytes, i_end));
    _throwif("table overflow", init_আকার > d_m3_Max_Sane_Table_আকার);

    যদি (flag & 0x01u) {
_       (ReadLebUnsigned(&max_আকার, addrBits, io_bytes, i_end));
        _throwif(m3Err_wasmMalformed, max_আকার < init_আকার);
        _throwif("table overflow", max_আকার > d_m3_Max_Sane_Table_আকার);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_আকার  = (u32)init_আকার;
    o_info->max_আকার   = (u32)max_আকার;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    ফেরত ফলাফল;
}


স্থির
M3_ফলাফল ParseType_TableType (IM3Module io_module, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3TableInfo info;

_   (ReadType_TableType(io_module, &info, io_bytes, i_end));
_   (Module_AddTable(io_module, ফাঁকা, &info, false /* isImport */));

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল Parse_Init_রাশিমালা_Typed (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end, m3type_t* o_type);

M3_ফলাফল ParseType_Table (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 অঙ্ক_Tables;
_   (ReadLEB_u32(&অঙ্ক_Tables, &i_bytes, i_end));                          m3log (parse, "** Table [%d]", অঙ্ক_Tables);

    _throwif("too many tables", (u64)অঙ্ক_Tables + io_module->অঙ্ক_Tables > d_m3_Max_Sane_Table_গুনতি);

    জন্য (u32 i = 0; i < অঙ্ক_Tables; ++i) {
#যদি d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_রাশিমালা = (i_bytes < i_end) and (*i_bytes == 0x40);
        u32  table_সূচক  = io_module->অঙ্ক_Tables;

        যদি (has_Init_রাশিমালা) {
            u8 reserved;
            ++i_bytes;
_           (Read_u8(&reserved, &i_bytes, i_end));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (ParseType_TableType(io_module, &i_bytes, i_end));

        যদি (has_Init_রাশিমালা) {
            IM3Table table = io_module->tables[table_সূচক];

            m3type_t initType;

            table->init_রাশিমালা = i_bytes;
_           (Parse_Init_রাশিমালা_Typed(io_module, &i_bytes, i_end, &initType));
            table->init_রাশিমালা_আকার = (u32)(i_bytes - table->init_রাশিমালা);
            _throwif(m3_Err_wasm_Missing_Init_রাশিমালা, table->init_রাশিমালা_আকার <= 1);

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(initType, table->type));
        } অন্যথা {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[table_সূচক]->type));
        }
#অন্যথা
_       (ParseType_TableType(io_module, &i_bytes, i_end));
#endif
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল ParseType_Memory (M3MemoryInfo* o_memory, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_Page_আকার = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addrBits = 32;

_   (ReadLEB_u7(&flag, io_bytes, i_end));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 known = 0x09u;
#যদি d_m3HasAtomics
        known |= 0x02u;
#endif
#যদি d_m3HasMemory64
        known |= 0x04u;
#endif
        _throwif(m3Err_wasmMalformed, flag & ~known);
    }

    o_memory->isShared = (flag & (1u << 1)) != 0;

    o_memory->isMemory64 = (flag & (1u << 2)) != 0;
    যদি (o_memory->isMemory64) {
        addrBits = 64;
    }

_   (ReadLebUnsigned(&o_memory->initPages, addrBits, io_bytes, i_end));

    o_memory->maxPages = 0;
    o_memory->hasMax   = (flag & (1u << 0)) != 0;
    যদি (o_memory->hasMax) {
_       (ReadLebUnsigned(&o_memory->maxPages, addrBits, io_bytes, i_end));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_memory->maxPages < o_memory->initPages);
    }

    _throwif("shared memory must have maximum", o_memory->isShared and not o_memory->hasMax);

    o_memory->page_আকার = 0;
    যদি (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_Page_আকার, io_bytes, i_end));

        _throwif("invalid custom page size", log_Page_আকার > 16);

        o_memory->page_আকার = 1u << log_Page_আকার;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 maxPagesAllowed = (log_Page_আকার or addrBits < 64)
                                ? (UINT64_C(1) << (addrBits - log_Page_আকার))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_memory->initPages > maxPagesAllowed);
        যদি (o_memory->hasMax) {
            _throwif(m3Err_wasmMalformed, o_memory->maxPages > maxPagesAllowed);
        }
    }

    _catch: ফেরত ফলাফল;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
স্থির
M3_ফলাফল ParseFuncType (IM3Module io_module, IM3FuncType* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = ফাঁকা;
_try {
    u32 অঙ্ক_Args;
_   (ReadLEB_u32(&অঙ্ক_Args, io_bytes, i_end));

    _throwif(m3Err_tooManyArgsRets, অঙ্ক_Args > d_m3_Max_Sane_Function_Arg_Ret_গুনতি);
#যদি M3_HAS_VLA
    m3type_t argTypes[অঙ্ক_Args + 1]; // make ubsan happy
#অন্যথা
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_গুনতি];
#endif
    জন্য (u32 a = 0; a < অঙ্ক_Args; ++a) {
_       (Parse_মান_Type(io_module, &argTypes[a], io_bytes, i_end));
    }

    u32 অঙ্ক_Rets;
_   (ReadLEB_u32(&অঙ্ক_Rets, io_bytes, i_end));
    _throwif(m3Err_tooManyArgsRets, (u64)(অঙ্ক_Rets) + অঙ্ক_Args > d_m3_Max_Sane_Function_Arg_Ret_গুনতি);

_   (AllocFuncType(&ftype, অঙ্ক_Rets + অঙ্ক_Args));
    ftype->অঙ্ক_Args = অঙ্ক_Args;
    ftype->অঙ্ক_Rets = অঙ্ক_Rets;

    জন্য (u32 r = 0; r < অঙ্ক_Rets; ++r) {
_       (Parse_মান_Type(io_module, &ftype->types[r], io_bytes, i_end));
    }
    memcpy(ftype->types + অঙ্ক_Rets, argTypes, অঙ্ক_Args * আকার(m3type_t));

    *o_type = ftype;
    ftype   = ফাঁকা;

} _catch:
    m3_Free(ftype);
    ফেরত ফলাফল;
}


#যদি d_m3_Has_স্তূপ_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
স্থির
M3_ফলাফল ParseContType (IM3Module io_module, IM3FuncType* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = ফাঁকা;
_try {
    i64 ft_সূচক;
_   (ReadLebSigned(&ft_সূচক, 33, io_bytes, i_end));
    _throwif(m3Err_wasmMalformed, ft_সূচক < 0 or (u64) ft_সূচক >= io_module->অঙ্ক_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_সূচক];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 যোগ = underlying->অঙ্ক_Rets + underlying->অঙ্ক_Args;
_   (AllocFuncType(&ftype, যোগ));
    ftype->অঙ্ক_Rets        = underlying->অঙ্ক_Rets;
    ftype->অঙ্ক_Args        = underlying->অঙ্ক_Args;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    যদি (যোগ) {
        memcpy(ftype->types, underlying->types, যোগ * আকার(m3type_t));
    }

    *o_type = ftype;
    ftype   = ফাঁকা;

} _catch:
    m3_Free(ftype);
    ফেরত ফলাফল;
}

#endif // d_m3HasStackSwitching


স্থির
M3_ফলাফল ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              bytes_t* io_bytes, cbytes_t i_end)
{
#যদি d_m3_Has_স্তূপ_Switching
    যদি (i_form == -35) {
        ফেরত ParseContType(io_module, o_type, io_bytes, i_end);
    }
#endif
    যদি (i_form != -32) { // for Wasm MVP
        ফেরত m3Err_wasmMalformed;
    }

    ফেরত ParseFuncType(io_module, o_type, io_bytes, i_end);
}


#যদি d_m3_Has_স্তূপ_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
স্থির
M3_ফলাফল SkipTypeDefinition (i8 i_form, bytes_t* io_bytes, cbytes_t i_end)
{
_try {
    যদি (i_form == -35) {
        i64 ft_সূচক;
_       (ReadLebSigned(&ft_সূচক, 33, io_bytes, i_end));
        ফেরত ফলাফল;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    জন্য (u32 half = 0; half < 2; ++half) {
        u32 গুনতি;
_       (ReadLEB_u32(&গুনতি, io_bytes, i_end));
        _throwif(m3Err_tooManyArgsRets, গুনতি > d_m3_Max_Sane_Function_Arg_Ret_গুনতি);

        জন্য (u32 v = 0; v < গুনতি; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_bytes, i_end));

            যদি (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 heap;
_               (ReadLebSigned(&heap, 33, io_bytes, i_end));
            }
        }
    }

} _catch: ফেরত ফলাফল;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
স্থির
M3_ফলাফল ParseRecGroup (IM3Module io_module, u32 i_গুনতি, u32 i_first_সূচক,
                        bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType* placeholders = ফাঁকা;
_try {
    bytes_t groupStart = *io_bytes;

    placeholders = m3_AllocArray(IM3FuncType, i_গুনতি);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    জন্য (u32 j = 0; j < i_গুনতি; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_bytes, i_end));
_       (SkipTypeDefinition(form, io_bytes, i_end));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 firstCanonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_গুনতি, &firstCanonical));

    জন্য (u32 j = 0; j < i_গুনতি; ++j) {
        placeholders[j]->canonical_সূচক        = (u16)(firstCanonical + j);
        io_module->funcTypes[i_first_সূচক + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    জন্য (u32 pass = 0; pass < 2; ++pass) {
        bytes_t bytes = groupStart;

        জন্য (u32 j = 0; j < i_গুনতি; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &bytes, i_end));

            যদি ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &bytes, i_end));
                চালু;
            }

            IM3FuncType ftype = ফাঁকা;
_           (ParseTypeDefinition(io_module, form, &ftype, &bytes, i_end));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_সূচক);

            m3_Free(placeholders[j]);
            placeholders[j]                        = ফাঁকা;
            io_module->funcTypes[i_first_সূচক + j] = ftype;
        }

        যদি (pass == 1) {
            *io_bytes = bytes;
        }
    }

} _catch:
    যদি (placeholders) {
        জন্য (u32 j = 0; j < i_গুনতি; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    ফেরত ফলাফল;
}

#endif // d_m3HasStackSwitching


M3_ফলাফল ParseSection_Type (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = ফাঁকা;

_try {
    u32 অঙ্ক_Types;
_   (ReadLEB_u32(&অঙ্ক_Types, &i_bytes, i_end));                                      m3log (parse, "** Type [%d]", অঙ্ক_Types);

    _throwif("too many types", অঙ্ক_Types > d_m3_Max_Sane_Types_গুনতি);

    যদি (অঙ্ক_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, অঙ্ক_Types);
        _throwifnull(io_module->funcTypes);
        io_module->অঙ্ক_Func_Types = অঙ্ক_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#যদি d_m3_Has_স্তূপ_Switching
        u32 capacity = অঙ্ক_Types;
#endif
        u32 index = 0;

        জন্য (u32 i = 0; i < অঙ্ক_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_bytes, i_end));

#যদি d_m3_Has_স্তূপ_Switching
            যদি (form == -50) { // rec
                u32 গুনতি;
_               (ReadLEB_u32(&গুনতি, &i_bytes, i_end));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + গুনতি + (অঙ্ক_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_গুনতি);

                যদি (needed > capacity) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, capacity);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    capacity                = grown;
                    io_module->অঙ্ক_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                যদি (গুনতি == 1) {
_                   (ReadLEB_i7(&form, &i_bytes, i_end));
                } অন্যথা {
_                   (ParseRecGroup(io_module, গুনতি, index, &i_bytes, i_end));
                    index += গুনতি;
                    চালু;
                }
            }
#endif

_           (ParseTypeDefinition(io_module, form, &ftype, &i_bytes, i_end));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_AddFuncType(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = ফাঁকা; // ownership transferred to environment
        }

        io_module->অঙ্ক_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

} _catch:

    যদি (ফলাফল) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->অঙ্ক_Func_Types = 0;
    }

    ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Function (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 অঙ্ক_Functions;
_   (ReadLEB_u32(&অঙ্ক_Functions, &i_bytes, i_end));                                  m3log (parse, "** Function [%d]", অঙ্ক_Functions);

    _throwif("too many functions", অঙ্ক_Functions > d_m3_Max_Sane_Functions_গুনতি);

_   (Module_PreallocFunctions(io_module, io_module->অঙ্ক_Functions + অঙ্ক_Functions));

    জন্য (u32 i = 0; i < অঙ্ক_Functions; ++i) {
        u32 func_Type_সূচক;
_       (ReadLEB_u32(&func_Type_সূচক, &i_bytes, i_end));

_       (Module_AddFunction(io_module, func_Type_সূচক, ফাঁকা /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: ফেরত ফলাফল;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
প্রকারলেখ কাঠামো M3ImportDesc {
    u8           kind;
    u32          type_সূচক;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3TableInfo  table;
    M3MemoryInfo memory;
} M3ImportDesc;


#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
স্থির
M3_ফলাফল ReadTagType (IM3Module i_module, u32* o_type_সূচক, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_bytes, i_end));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_সূচক, io_bytes, i_end));
    _throwif(m3Err_unknownType, not i_module or *o_type_সূচক >= i_module->অঙ্ক_Func_Types);

#  যদি !d_m3_Has_স্তূপ_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_অঙ্ক_Results(i_module->funcTypes[*o_type_সূচক]) != 0);
#  endif

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
স্থির
M3_ফলাফল ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, bytes_t* io_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (Read_u8(&o_desc->kind, io_bytes, i_end));

    নির্বাচন (o_desc->kind) {
    ক্ষেত্র d_externalKind_function:
_       (ReadLEB_u32(&o_desc->type_সূচক, io_bytes, i_end));
        বিরতি;

    ক্ষেত্র d_externalKind_table:
_       (ReadType_TableType(i_module, &o_desc->table, io_bytes, i_end));
        বিরতি;

    ক্ষেত্র d_externalKind_memory:
_       (ParseType_Memory(&o_desc->memory, io_bytes, i_end));
        বিরতি;

    ক্ষেত্র d_externalKind_global:
_       (Parse_মান_Type(i_module, &o_desc->type, io_bytes, i_end));
_       (ReadLEB_u7(&o_desc->isMutable, io_bytes, i_end));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [BaseTypeOf(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        বিরতি;

#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
    ক্ষেত্র d_externalKind_tag:
_       (ReadTagType(i_module, &o_desc->type_সূচক, io_bytes, i_end));
        বিরতি;
#endif

    পূর্বনির্ধারিত:
        _throw(m3Err_wasmMalformed);
    }

    _catch: ফেরত ফলাফল;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
স্থির
M3_ফলাফল ApplyImportDesc (IM3Module io_module, ধ্রুবক M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3ImportInfo clearImport = { ফাঁকা, ফাঁকা };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->kind, io_import->moduleUtf8, io_import->fieldUtf8);
    নির্বাচন (i_desc->kind) {
    ক্ষেত্র d_externalKind_function: {
_       (Module_AddFunction(io_module, i_desc->type_সূচক, io_import))
        *io_import = clearImport;

        io_module->অঙ্ক_Func_Imports++;
    } বিরতি;

    ক্ষেত্র d_externalKind_table: {
        IM3Table table;
_       (Module_AddTable(io_module, &table, &i_desc->table, true /* isImport */));
        table->import = *io_import;
        *io_import    = clearImport;
    } বিরতি;

    ক্ষেত্র d_externalKind_memory: {
        IM3Memory memory;

#যদি d_m3HasMultiMemory
        _throwif(m3Err_tooManyMemorySections, io_module->অঙ্ক_Memories >= d_m3_Max_Sane_Memories_গুনতি);
#অন্যথা
        // MVP: one memory per module, imported or declared
        _throwif(m3Err_tooManyMemorySections, io_module->অঙ্ক_Memories >= 1);
#endif

_       (Module_AddMemory(io_module, &memory, &i_desc->memory, true /* isImport */));
        memory->import = *io_import;
        *io_import     = clearImport;
    } বিরতি;

    ক্ষেত্র d_externalKind_global: {
        IM3Global global;
_       (Module_AddGlobal(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } বিরতি;

#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
    ক্ষেত্র d_externalKind_tag: {
        IM3Tag tag;
_       (Module_AddTag(io_module, &tag, io_module->funcTypes[i_desc->type_সূচক], true /* isImport */));
        tag->import = *io_import;
        *io_import  = clearImport;
    } বিরতি;
#endif

    পূর্বনির্ধারিত:
        _throw(m3Err_wasmMalformed);
    }

    _catch: ফেরত ফলাফল;
}


#যদি d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
স্থির
bool IsEmptyName (bytes_t i_bytes, cbytes_t i_end)
{
    u32 দৈর্ঘ্য;
    ফেরত (not ReadLEB_u32(&দৈর্ঘ্য, &i_bytes, i_end)) and দৈর্ঘ্য == 0;
}

#endif // d_m3HasCompactImports


#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching

M3_ফলাফল ParseSection_Tag (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 অঙ্ক_Tags;
_   (ReadLEB_u32(&অঙ্ক_Tags, &i_bytes, i_end));                                       m3log (parse, "** Tag [%d]", অঙ্ক_Tags);

    _throwif("too many tags", অঙ্ক_Tags > d_m3_Max_Sane_Tags_গুনতি);

    জন্য (u32 i = 0; i < অঙ্ক_Tags; ++i) {
        u32 type_সূচক;
_       (ReadTagType(io_module, &type_সূচক, &i_bytes, i_end));                      m3log (parse, "    tag: [%d] type: %d", i, type_সূচক);

_       (Module_AddTag(io_module, ফাঁকা, io_module->funcTypes[type_সূচক], false));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: ফেরত ফলাফল;
}

#endif // d_m3HasExceptionHandling


M3_ফলাফল ParseSection_Import (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    M3ImportInfo import = { ফাঁকা, ফাঁকা };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 অঙ্ক_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 অঙ্ক_Entries;
_   (ReadLEB_u32(&অঙ্ক_Entries, &i_bytes, i_end));                                    m3log (parse, "** Import [%d]", অঙ্ক_Entries);

    _throwif("too many imports", অঙ্ক_Entries > d_m3_Max_Sane_Imports_গুনতি);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->অঙ্ক_Functions + অঙ্ক_Entries));

    জন্য (u32 i = 0; i < অঙ্ক_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_bytes, i_end));

#যদি d_m3HasCompactImports
        bytes_t fieldStart = i_bytes;
#endif
_       (Read_utf8(&import.fieldUtf8, &i_bytes, i_end));

#যদি d_m3HasCompactImports
        u8 compact = (i_bytes < i_end) ? *i_bytes : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        যদি ((compact == d_compactImports_perItemType or compact == d_compactImports_sharedType) and
            IsEmptyName(fieldStart, i_end)) {
            ++i_bytes;

            m3_Free(import.fieldUtf8);         // it only marked the compact form
            import.fieldUtf8 = ফাঁকা;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            যদি (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_bytes, i_end));
            }

            u32 অঙ্ক_বস্তুসমূহ;
_           (ReadLEB_u32(&অঙ্ক_বস্তুসমূহ, &i_bytes, i_end));                              m3log (parse, "    compact: %d import(s) of '%s'", অঙ্ক_বস্তুসমূহ, import.moduleUtf8);

            _throwif("too many imports", (u64)অঙ্ক_Imports + অঙ্ক_বস্তুসমূহ > d_m3_Max_Sane_Imports_গুনতি);
            অঙ্ক_Imports += অঙ্ক_বস্তুসমূহ;

_           (Module_PreallocFunctions(io_module, io_module->অঙ্ক_Functions + অঙ্ক_বস্তুসমূহ));

            জন্য (u32 j = 0; j < অঙ্ক_বস্তুসমূহ; ++j) {
                M3ImportInfo item = { ফাঁকা, ফাঁকা };

                item.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, strlen(import.moduleUtf8) + 1);

                যদি (item.moduleUtf8) {
                    ফলাফল = Read_utf8(&item.fieldUtf8, &i_bytes, i_end);

                    // the shared form reuses the externtype read above
                    যদি (not ফলাফল and compact == d_compactImports_perItemType) {
                        ফলাফল = ReadImportDesc(io_module, &desc, &i_bytes, i_end);
                    }

                    যদি (not ফলাফল) {
                        ফলাফল = ApplyImportDesc(io_module, &desc, &item);
                    }
                } অন্যথা {
                    ফলাফল = m3Err_mallocFailed;
                }

                FreeImportInfo(&item);
                _throwif(ফলাফল, ফলাফল);
            }

            FreeImportInfo(&import);
            চালু;
        }
#endif // d_m3HasCompactImports

        _throwif("too many imports", অঙ্ক_Imports >= d_m3_Max_Sane_Imports_গুনতি);
        ++অঙ্ক_Imports;

_       (ReadImportDesc(io_module, &desc, &i_bytes, i_end));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Export (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল    ফলাফল = m3Err_none;
    ধ্রুবক অক্ষর* utf8   = ফাঁকা;

    u32 অঙ্ক_Exports;
_   (ReadLEB_u32(&অঙ্ক_Exports, &i_bytes, i_end));                                    m3log (parse, "** Export [%d]", অঙ্ক_Exports);

    _throwif("too many exports", অঙ্ক_Exports > d_m3_Max_Sane_Exports_গুনতি);

    // the section order check lets this section through once, so this is
    // every export the module has
    যদি (অঙ্ক_Exports) {
        io_module->exports = m3_AllocArray(M3Export, অঙ্ক_Exports);
        _throwifnull(io_module->exports);
    }

    জন্য (u32 i = 0; i < অঙ্ক_Exports; ++i) {
        u8  exportKind;
        u32 index;
        u32 name_দৈর্ঘ্য;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            bytes_t দৈর্ঘ্য_Bytes = i_bytes;
_           (ReadLEB_u32(&name_দৈর্ঘ্য, &দৈর্ঘ্য_Bytes, i_end));
        }

_       (Read_utf8(&utf8, &i_bytes, i_end));
_       (Read_u8(&exportKind, &i_bytes, i_end));
_       (ReadLEB_u32(&index, &i_bytes, i_end));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) exportKind, utf8);

#যদি d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        জন্য (u32 j = 0; j < i; ++j) {
            ধ্রুবক M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_দৈর্ঘ্য == name_দৈর্ঘ্য and memcmp(other->name, utf8, name_দৈর্ঘ্য) == 0);
        }
#endif

        যদি (exportKind == d_externalKind_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->অঙ্ক_Functions);
            IM3Function func = &(io_module->functions[index]);
            যদি (func->অঙ্ক_Names < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_দৈর্ঘ্য + 1);
                _throwifnull(name);
                func->names[func->অঙ্ক_Names++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } অন্যথা যদি (exportKind == d_externalKind_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->অঙ্ক_Globals);
        } অন্যথা যদি (exportKind == d_externalKind_memory) {
            _throwif(m3Err_wasmMalformed, index >= io_module->অঙ্ক_Memories);
        } অন্যথা যদি (exportKind == d_externalKind_table) {
            _throwif(m3Err_wasmMalformed, index >= io_module->অঙ্ক_Tables);
        }
#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
        অন্যথা যদি (exportKind == d_externalKind_tag) {
            _throwif(m3Err_wasmMalformed, index >= io_module->অঙ্ক_Tags);
        }
#endif

        M3Export* entry   = &io_module->exports[io_module->অঙ্ক_Exports++];
        entry->name       = utf8;
        entry->name_দৈর্ঘ্য = name_দৈর্ঘ্য;
        entry->index      = index;
        entry->kind       = exportKind;

        utf8 = ফাঁকা; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

_catch:
    m3_Free(utf8);
    ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Start (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 start_Func_সূচক;
_   (ReadLEB_u32(&start_Func_সূচক, &i_bytes, i_end));                                  m3log (parse, "** Start Function: %d", start_Func_সূচক);

    যদি (start_Func_সূচক < io_module->অঙ্ক_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[start_Func_সূচক];
        যদি (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->অঙ্ক_Args != 0 || func->funcType->অঙ্ক_Rets != 0);
        }

        io_module->startFunction = start_Func_সূচক;
    } অন্যথা {
        ফলাফল = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: ফেরত ফলাফল;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_ফলাফল Parse_Init_রাশিমালা_Typed (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end, m3type_t* o_type)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#যদি defined(d_m3PreferStaticAlloc)
    স্থির M3Compilation compilation;
#অন্যথা
    M3Compilation compilation;
#endif
    compilation = (M3Compilation){ .runtime = ফাঁকা, .module = io_module, .wasm = *io_bytes, .wasmEnd = i_end, .is_Init_রাশিমালা = true };

    ফলাফল = CompileBlockStatements(&compilation);

    *io_bytes = compilation.wasm;

    যদি (o_type) {
        *o_type = compilation.স্তূপ_সূচক ? compilation.type_স্তূপ[compilation.স্তূপ_সূচক - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    ফেরত ফলাফল;
}


M3_ফলাফল Parse_Init_রাশিমালা (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end)
{
    ফেরত Parse_Init_রাশিমালা_Typed(io_module, io_bytes, i_end, ফাঁকা);
}


M3_ফলাফল ParseSection_Element (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32     অঙ্ক_Segments;
    bytes_t pos;
_   (ReadLEB_u32(&অঙ্ক_Segments, &i_bytes, i_end));                            m3log (parse, "** Element [%d]", অঙ্ক_Segments);

    _throwif("too many element segments", অঙ্ক_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, অঙ্ক_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->অঙ্ক_Element_Segments = অঙ্ক_Segments;
    io_module->elementSectionEnd  = i_end;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_bytes;
    জন্য (u32 i = 0; i < অঙ্ক_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_end));
#যদি d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#অন্যথা
        _throwif(m3Err_wasmMalformed, flags != c_m3Elem_active);
#endif

        segment->mode   = flags & 0x3;
        segment->is_রাশিমালা = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        যদি (segment->mode == c_m3Elem_activeIdx) {
_           (ReadLEB_u32(&segment->table_সূচক, &pos, i_end));
        }

        bool isActive = (segment->mode == c_m3Elem_active or segment->mode == c_m3Elem_activeIdx);

        যদি (isActive) {
            _throwif(m3Err_wasmMalformed, segment->table_সূচক >= io_module->অঙ্ক_Tables);

            segment->init_রাশিমালা = pos;
_           (Parse_Init_রাশিমালা(io_module, &pos, i_end));
            segment->init_রাশিমালা_আকার = (u32)(pos - segment->init_রাশিমালা);
            _throwif(m3_Err_wasm_Missing_Init_রাশিমালা, segment->init_রাশিমালা_আকার <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        যদি (segment->mode != c_m3Elem_active) {
            যদি (segment->is_রাশিমালা) {
_               (Parse_মান_Type(io_module, &segment->type, &pos, i_end));
                _throwif(m3Err_wasmMalformed, not IsRefType(BaseTypeOf(segment->type)));
            } অন্যথা {
                u8 elemKind;
_               (Read_u8(&elemKind, &pos, i_end));
                _throwif(m3Err_wasmMalformed, elemKind != 0x00);   // funcref
            }
        }

        যদি (isActive) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->table_সূচক]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->অঙ্ক_Elements, &pos, i_end));
        _throwif("table overflow", segment->অঙ্ক_Elements > d_m3_Max_Sane_Table_আকার);

        segment->elements = pos;

        জন্য (u32 e = 0; e < segment->অঙ্ক_Elements; ++e) {
            যদি (segment->is_রাশিমালা) {
_               (Parse_Init_রাশিমালা(io_module, &pos, i_end))
            } অন্যথা {
                u32 func_সূচক;
_               (ReadLEB_u32(&func_সূচক, &pos, i_end));
                _throwif("function index out of range", func_সূচক >= io_module->অঙ্ক_Functions);
_               (Module_DeclareFunction(io_module, func_সূচক));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_end);           // section size mismatch

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Code (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল;

    u32 অঙ্ক_Functions;
_   (ReadLEB_u32(&অঙ্ক_Functions, &i_bytes, i_end));                                  m3log (parse, "** Code [%d]", অঙ্ক_Functions);

    যদি (অঙ্ক_Functions != io_module->অঙ্ক_Functions - io_module->অঙ্ক_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    জন্য (u32 f = 0; f < অঙ্ক_Functions; ++f) {
        ধ্রুবক u8* start = i_bytes;

        u32 আকার_2;
_       (ReadLEB_u32(&আকার_2, &i_bytes, i_end));

        যদি (আকার_2) {
            i_bytes += আকার_2;

            যদি (i_bytes <= i_end) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->অঙ্ক_Func_Imports);

                func->module  = io_module;
                func->wasm    = start;
                func->wasmEnd = i_bytes;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } অন্যথা {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    যদি (not ফলাফল and i_bytes != i_end) {
        ফলাফল = m3Err_wasmSectionUnderrun;
    }

    ফেরত ফলাফল;
}


M3_ফলাফল Parse_Section_তথ্য (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 অঙ্ক_তথ্য_Segments;
_   (ReadLEB_u32(&অঙ্ক_তথ্য_Segments, &i_bytes, i_end));                               m3log (parse, "** Data [%d]", অঙ্ক_তথ্য_Segments);

    _throwif("too many data segments", অঙ্ক_তথ্য_Segments > d_m3_Max_Sane_তথ্য_Segments);

    io_module->তথ্য_Segments = m3_AllocArray(M3_তথ্য_Segment, অঙ্ক_তথ্য_Segments);
    _throwifnull(io_module->তথ্য_Segments);
    io_module->অঙ্ক_তথ্য_Segments = অঙ্ক_তথ্য_Segments;

    জন্য (u32 i = 0; i < অঙ্ক_তথ্য_Segments; ++i) {
        M3_তথ্য_Segment* segment = &io_module->তথ্য_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_bytes, i_end));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        যদি (flags == 2) {
_           (ReadLEB_u32(&segment->memoryRegion, &i_bytes, i_end));
        }

        যদি (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#যদি !d_m3HasMultiMemory
            _throwif(m3Err_wasmMalformed, segment->memoryRegion != 0);
#endif
            _throwif(m3Err_wasmMalformed, segment->memoryRegion >= io_module->অঙ্ক_Memories);

            segment->init_রাশিমালা = i_bytes;
_           (Parse_Init_রাশিমালা(io_module, &i_bytes, i_end));
            segment->init_রাশিমালা_আকার = (u32)(i_bytes - segment->init_রাশিমালা);

            _throwif(m3_Err_wasm_Missing_Init_রাশিমালা, segment->init_রাশিমালা_আকার <= 1);
        }

_       (ReadLEB_u32(&segment->আকার_2, &i_bytes, i_end));
        segment->তথ্য = i_bytes;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->memoryRegion, segment->init_রাশিমালা_আকার, segment->আকার_2);
        i_bytes += segment->আকার_2;

        _throwif("data segment underflow", i_bytes > i_end);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch:

    ফেরত ফলাফল;
}


M3_ফলাফল Parse_Section_তথ্য_গুনতি (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

_   (ReadLEB_u32(&io_module->তথ্য_গুনতি, &i_bytes, i_end));                       m3log (parse, "** DataCount [%d]", io_module->তথ্য_গুনতি);

    io_module->has_তথ্য_গুনতি = true;

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Memory (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 অঙ্ক_Memories;
_   (ReadLEB_u32(&অঙ্ক_Memories, &i_bytes, i_end));                                m3log (parse, "** Memory [%d]", অঙ্ক_Memories);

#যদি d_m3HasMultiMemory
    _throwif(m3Err_tooManyMemorySections,
             (u64)অঙ্ক_Memories + io_module->অঙ্ক_Memories > d_m3_Max_Sane_Memories_গুনতি);
#অন্যথা
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3Err_tooManyMemorySections, অঙ্ক_Memories > 1);
    _throwif(m3Err_tooManyMemorySections, (u64)অঙ্ক_Memories + io_module->অঙ্ক_Memories > 1);
#endif

    জন্য (u32 i = 0; i < অঙ্ক_Memories; ++i) {
        M3MemoryInfo info;
_       (ParseType_Memory(&info, &i_bytes, i_end));
_       (Module_AddMemory(io_module, ফাঁকা, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Global (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    u32 অঙ্ক_Globals;
_   (ReadLEB_u32(&অঙ্ক_Globals, &i_bytes, i_end));                                    m3log (parse, "** Global [%d]", অঙ্ক_Globals);

    _throwif("too many globals", অঙ্ক_Globals > d_m3_Max_Sane_Globals_গুনতি);

    জন্য (u32 i = 0; i < অঙ্ক_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_মান_Type(io_module, &type, &i_bytes, i_end));
_       (ReadLEB_u7(&isMutable, &i_bytes, i_end));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [BaseTypeOf(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_AddGlobal(io_module, &global, type, isMutable, false /* isImport */));

        global->init_রাশিমালা = i_bytes;
_       (Parse_Init_রাশিমালা(io_module, &i_bytes, i_end));
        global->init_রাশিমালা_আকার = (u32)(i_bytes - global->init_রাশিমালা);

        _throwif(m3_Err_wasm_Missing_Init_রাশিমালা, global->init_রাশিমালা_আকার <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Name (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    cstr_t name = ফাঁকা;

    যখন (i_bytes < i_end) {
        u8  nameType;
        u32 payload_দৈর্ঘ্য;

_       (ReadLEB_u7(&nameType, &i_bytes, i_end));
_       (ReadLEB_u32(&payload_দৈর্ঘ্য, &i_bytes, i_end));

        bytes_t start = i_bytes;
        _throwif(m3Err_wasmSectionOverrun, payload_দৈর্ঘ্য > (size_t)(i_end - start));

        যদি (nameType == 1) {
            u32 অঙ্ক_Names;
_           (ReadLEB_u32(&অঙ্ক_Names, &i_bytes, i_end));

            _throwif("too many names", অঙ্ক_Names > d_m3_Max_Sane_Functions_গুনতি);

            জন্য (u32 i = 0; i < অঙ্ক_Names; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_bytes, i_end));
_               (Read_utf8(&name, &i_bytes, i_end));

                যদি (index < io_module->অঙ্ক_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    যদি (func->অঙ্ক_Names == 0) {
                        func->names[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->অঙ্ক_Names = 1;

                        name = ফাঁকা; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_bytes = start + payload_দৈর্ঘ্য;
    }

    _catch: ফেরত ফলাফল;
}


M3_ফলাফল ParseSection_Custom (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    cstr_t name = ফাঁকা;
_   (Read_utf8(&name, &i_bytes, i_end));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    যদি (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_bytes, i_end));
#যদি d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } অন্যথা যদি (strcmp(name, "snapshot") == 0 || (strlen(name) > 9 && strncmp(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, strlen(snapName) + 1);
        যদি (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->তথ্য           = i_bytes;
        snap->আকার_2           = (u32)(i_end - i_bytes);
        snap->পরবর্তী           = io_module->snapshots;
        io_module->snapshots = snap;
#endif
    } অন্যথা যদি (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_bytes, i_end));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    ফেরত ফলাফল;
}


M3_ফলাফল ParseModuleSection (M3Module* o_module, u8 i_sectionType, bytes_t i_bytes, u32 i_অঙ্ক_Bytes)
{
    M3_ফলাফল ফলাফল = m3Err_none;

    প্রকারলেখ M3_ফলাফল (*M3Parser)(M3Module*, bytes_t, cbytes_t);

    স্থির M3Parser s_parsers[] = {
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
        Parse_Section_তথ্য,      // 11
        Parse_Section_তথ্য_গুনতি, // 12
#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
        ParseSection_Tag,       // 13
#endif
    };

    M3Parser parser = ফাঁকা;

    যদি (i_sectionType < M3_গুনতি_OF(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    যদি (parser) {
        cbytes_t end = i_bytes + i_অঙ্ক_Bytes;
        ফলাফল       = parser(o_module, i_bytes, end);
    } অন্যথা {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    ফেরত ফলাফল;
}


M3_ফলাফল m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_bytes, u32 i_অঙ্ক_Bytes)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_অঙ্ক_Bytes);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_অঙ্ক_Bytes);
    module->startFunction = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ধ্রুবক u8* pos = i_bytes;
    ধ্রুবক u8* end = pos + i_অঙ্ক_Bytes;

    module->wasmStart = pos;
    module->wasmEnd   = end;

    u32 magic, version;
_   (Read_u32(&magic, &pos, end));
_   (Read_u32(&version, &pos, end));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3Err_incompatibleWasmVersion, version != 1);

#যদি d_m3HasExceptionHandling || d_m3_Has_স্তূপ_Switching
    // the tag section sits between memory (5) and global (6)
    স্থির ধ্রুবক u8 sectionsOrder[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#অন্যথা
    স্থির ধ্রুবক u8 sectionsOrder[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#endif
    u8 expectedSection = 0;

    যখন (pos < end) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, end));

        যদি (section != 0) {
            // Ensure sections appear only once and in order
            যখন (sectionsOrder[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_গুনতি_OF(sectionsOrder) - 1);
            }
        }

        u32 section_দৈর্ঘ্য;
_       (ReadLEB_u32(&section_দৈর্ঘ্য, &pos, end));
        _throwif(m3Err_wasmMalformed, pos + section_দৈর্ঘ্য > end);

_       (ParseModuleSection(module, section, pos, section_দৈর্ঘ্য));

        pos += section_দৈর্ঘ্য;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    যদি (module->অঙ্ক_Functions > module->অঙ্ক_Func_Imports) {
        IM3Function firstNonImport = &module->functions[module->অঙ্ক_Func_Imports];
        _throwif(m3Err_wasmMalformed, firstNonImport->wasm == ফাঁকা);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_তথ্য_গুনতি and module->তথ্য_গুনতি != module->অঙ্ক_তথ্য_Segments);

} _catch:

    যদি (ফলাফল) {
        m3_FreeModule(module);
        module = ফাঁকা;
    }

    *o_module = module;

    ফেরত ফলাফল;
}
