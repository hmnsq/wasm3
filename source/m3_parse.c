//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#અન્તર્ભાવ "m3_env.h"
#અન્તર્ભાવ "m3_compile.h"
#અન્તર્ભાવ "m3_exception.h"
#અન્તર્ભાવ "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#જો d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3Result ParseHeapType (IM3Module i_module, m3type_t* o_heapBits, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    i64 heap;
_   (ReadLebSigned(&heap, 33, io_bytes, i_end));

    જો (heap == -d_waType_funcref) {
        *o_heapBits = d_m3Type_heapAbstract;
    } નહીંતર જો (heap == -d_waType_externref) {
        *o_heapBits = d_m3Type_refExtern | d_m3Type_heapAbstract;
#  જો d_m3_Has_ઢગલો_Switching
    } નહીંતર જો (heap == -d_waType_heap_cont) {
        *o_heapBits = d_m3Type_refCont | d_m3Type_heapAbstract;
    } નહીંતર જો (heap == -d_waType_heap_nocont) {
        *o_heapBits = d_m3Type_refCont | d_m3Type_heapNone;
#  endif
    } નહીંતર જો (heap >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) heap >= i_module->અંક_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[heap];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_heapBits = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonicalIndex;
    } નહીંતર {
        _throw(m3Err_invalidTypeId);
    }

    _catch: પાછા result;
}

#endif // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3Result Parse_કિંમત_Type (IM3Module i_module, m3type_t* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_bytes >= i_end);

#જો d_m3HasTypedRefs
    જો (**io_bytes == d_waEncode_ref or **io_bytes == d_waEncode_refNull) {
        u8 lead = *(*io_bytes)++;

        m3type_t heapBits = d_m3Type_heapAbstract;
_       (ParseHeapType(i_module, &heapBits, io_bytes, i_end));

        *o_type = d_m3Type_ref | heapBits | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        પાછા result;
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

    _catch: પાછા result;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
સ્થિર
M3Result ReadType_TableType (IM3Module i_module, M3TableInfo* o_info, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      initSize = 0, maxSize = 0;

    // the width of the limits, and of every index into the table
    u32 addrBits = 32;

_   (Parse_કિંમત_Type(i_module, &elemType, io_bytes, i_end));
#જો d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(BaseTypeOf(elemType)));
#નહીંતર
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#endif

_   (ReadLEB_u7(&flag, io_bytes, i_end));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#જો d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#નહીંતર
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#endif

    જો (flag & 0x04u) {
        addrBits = 64;
    }

_   (ReadLebUnsigned(&initSize, addrBits, io_bytes, i_end));
    _throwif("table overflow", initSize > d_m3MaxSaneTableSize);

    જો (flag & 0x01u) {
_       (ReadLebUnsigned(&maxSize, addrBits, io_bytes, i_end));
        _throwif(m3Err_wasmMalformed, maxSize < initSize);
        _throwif("table overflow", maxSize > d_m3MaxSaneTableSize);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->initSize  = (u32)initSize;
    o_info->maxSize   = (u32)maxSize;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    પાછા result;
}


સ્થિર
M3Result ParseType_TableType (IM3Module io_module, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    M3TableInfo info;

_   (ReadType_TableType(io_module, &info, io_bytes, i_end));
_   (Module_AddTable(io_module, ખાલી, &info, false /* isImport */));

    _catch: પાછા result;
}


M3Result Parse_Init_પદાવલિ_Typed (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end, m3type_t* o_type);

M3Result ParseType_Table (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 અંક_Tables;
_   (ReadLEB_u32(&અંક_Tables, &i_bytes, i_end));                          m3log (parse, "** Table [%d]", અંક_Tables);

    _throwif("too many tables", (u64)અંક_Tables + io_module->અંક_Tables > d_m3_Max_Sane_Table_ગણતરી);

    માટે (u32 i = 0; i < અંક_Tables; ++i) {
#જો d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_પદાવલિ = (i_bytes < i_end) and (*i_bytes == 0x40);
        u32  tableIndex  = io_module->અંક_Tables;

        જો (has_Init_પદાવલિ) {
            u8 reserved;
            ++i_bytes;
_           (Read_u8(&reserved, &i_bytes, i_end));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (ParseType_TableType(io_module, &i_bytes, i_end));

        જો (has_Init_પદાવલિ) {
            IM3Table table = io_module->tables[tableIndex];

            m3type_t initType;

            table->init_પદાવલિ = i_bytes;
_           (Parse_Init_પદાવલિ_Typed(io_module, &i_bytes, i_end, &initType));
            table->init_પદાવલિ_Size = (u32)(i_bytes - table->init_પદાવલિ);
            _throwif(m3_Err_wasm_Missing_Init_પદાવલિ, table->init_પદાવલિ_Size <= 1);

            _throwif(m3Err_typeMismatch, not IsSubTypeOf(initType, table->type));
        } નહીંતર {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[tableIndex]->type));
        }
#નહીંતર
_       (ParseType_TableType(io_module, &i_bytes, i_end));
#endif
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: પાછા result;
}


M3Result ParseType_Memory (M3MemoryInfo* o_memory, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 logPageSize = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addrBits = 32;

_   (ReadLEB_u7(&flag, io_bytes, i_end));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 known = 0x09u;
#જો d_m3HasAtomics
        known |= 0x02u;
#endif
#જો d_m3HasMemory64
        known |= 0x04u;
#endif
        _throwif(m3Err_wasmMalformed, flag & ~known);
    }

    o_memory->isShared = (flag & (1u << 1)) != 0;

    o_memory->isMemory64 = (flag & (1u << 2)) != 0;
    જો (o_memory->isMemory64) {
        addrBits = 64;
    }

_   (ReadLebUnsigned(&o_memory->initPages, addrBits, io_bytes, i_end));

    o_memory->maxPages = 0;
    o_memory->hasMax   = (flag & (1u << 0)) != 0;
    જો (o_memory->hasMax) {
_       (ReadLebUnsigned(&o_memory->maxPages, addrBits, io_bytes, i_end));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_memory->maxPages < o_memory->initPages);
    }

    _throwif("shared memory must have maximum", o_memory->isShared and not o_memory->hasMax);

    o_memory->pageSize = 0;
    જો (flag & (1u << 3)) {
_       (ReadLEB_u32(&logPageSize, io_bytes, i_end));

        _throwif("invalid custom page size", logPageSize > 16);

        o_memory->pageSize = 1u << logPageSize;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 maxPagesAllowed = (logPageSize or addrBits < 64)
                                ? (UINT64_C(1) << (addrBits - logPageSize))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_memory->initPages > maxPagesAllowed);
        જો (o_memory->hasMax) {
            _throwif(m3Err_wasmMalformed, o_memory->maxPages > maxPagesAllowed);
        }
    }

    _catch: પાછા result;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
સ્થિર
M3Result ParseFuncType (IM3Module io_module, IM3FuncType* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = ખાલી;
_try {
    u32 અંક_Args;
_   (ReadLEB_u32(&અંક_Args, io_bytes, i_end));

    _throwif(m3Err_tooManyArgsRets, અંક_Args > d_m3_Max_Sane_Function_Arg_Ret_ગણતરી);
#જો M3_HAS_VLA
    m3type_t argTypes[અંક_Args + 1]; // make ubsan happy
#નહીંતર
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_ગણતરી];
#endif
    માટે (u32 a = 0; a < અંક_Args; ++a) {
_       (Parse_કિંમત_Type(io_module, &argTypes[a], io_bytes, i_end));
    }

    u32 અંક_Rets;
_   (ReadLEB_u32(&અંક_Rets, io_bytes, i_end));
    _throwif(m3Err_tooManyArgsRets, (u64)(અંક_Rets) + અંક_Args > d_m3_Max_Sane_Function_Arg_Ret_ગણતરી);

_   (AllocFuncType(&ftype, અંક_Rets + અંક_Args));
    ftype->અંક_Args = અંક_Args;
    ftype->અંક_Rets = અંક_Rets;

    માટે (u32 r = 0; r < અંક_Rets; ++r) {
_       (Parse_કિંમત_Type(io_module, &ftype->types[r], io_bytes, i_end));
    }
    memcpy(ftype->types + અંક_Rets, argTypes, અંક_Args * કદ(m3type_t));

    *o_type = ftype;
    ftype   = ખાલી;

} _catch:
    m3_Free(ftype);
    પાછા result;
}


#જો d_m3_Has_ઢગલો_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
સ્થિર
M3Result ParseContType (IM3Module io_module, IM3FuncType* o_type, bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = ખાલી;
_try {
    i64 ftIndex;
_   (ReadLebSigned(&ftIndex, 33, io_bytes, i_end));
    _throwif(m3Err_wasmMalformed, ftIndex < 0 or (u64) ftIndex >= io_module->અંક_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ftIndex];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 total = underlying->અંક_Rets + underlying->અંક_Args;
_   (AllocFuncType(&ftype, total));
    ftype->અંક_Rets        = underlying->અંક_Rets;
    ftype->અંક_Args        = underlying->અંક_Args;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    જો (total) {
        memcpy(ftype->types, underlying->types, total * કદ(m3type_t));
    }

    *o_type = ftype;
    ftype   = ખાલી;

} _catch:
    m3_Free(ftype);
    પાછા result;
}

#endif // d_m3HasStackSwitching


સ્થિર
M3Result ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              bytes_t* io_bytes, cbytes_t i_end)
{
#જો d_m3_Has_ઢગલો_Switching
    જો (i_form == -35) {
        પાછા ParseContType(io_module, o_type, io_bytes, i_end);
    }
#endif
    જો (i_form != -32) { // for Wasm MVP
        પાછા m3Err_wasmMalformed;
    }

    પાછા ParseFuncType(io_module, o_type, io_bytes, i_end);
}


#જો d_m3_Has_ઢગલો_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
સ્થિર
M3Result SkipTypeDefinition (i8 i_form, bytes_t* io_bytes, cbytes_t i_end)
{
_try {
    જો (i_form == -35) {
        i64 ftIndex;
_       (ReadLebSigned(&ftIndex, 33, io_bytes, i_end));
        પાછા result;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    માટે (u32 half = 0; half < 2; ++half) {
        u32 ગણતરી;
_       (ReadLEB_u32(&ગણતરી, io_bytes, i_end));
        _throwif(m3Err_tooManyArgsRets, ગણતરી > d_m3_Max_Sane_Function_Arg_Ret_ગણતરી);

        માટે (u32 v = 0; v < ગણતરી; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_bytes, i_end));

            જો (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 heap;
_               (ReadLebSigned(&heap, 33, io_bytes, i_end));
            }
        }
    }

} _catch: પાછા result;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
સ્થિર
M3Result ParseRecGroup (IM3Module io_module, u32 i_ગણતરી, u32 i_firstIndex,
                        bytes_t* io_bytes, cbytes_t i_end)
{
    IM3FuncType* placeholders = ખાલી;
_try {
    bytes_t groupStart = *io_bytes;

    placeholders = m3_AllocArray(IM3FuncType, i_ગણતરી);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    માટે (u32 j = 0; j < i_ગણતરી; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_bytes, i_end));
_       (SkipTypeDefinition(form, io_bytes, i_end));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 firstCanonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_ગણતરી, &firstCanonical));

    માટે (u32 j = 0; j < i_ગણતરી; ++j) {
        placeholders[j]->canonicalIndex        = (u16)(firstCanonical + j);
        io_module->funcTypes[i_firstIndex + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    માટે (u32 pass = 0; pass < 2; ++pass) {
        bytes_t bytes = groupStart;

        માટે (u32 j = 0; j < i_ગણતરી; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &bytes, i_end));

            જો ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &bytes, i_end));
                ચાલુ;
            }

            IM3FuncType ftype = ખાલી;
_           (ParseTypeDefinition(io_module, form, &ftype, &bytes, i_end));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonicalIndex);

            m3_Free(placeholders[j]);
            placeholders[j]                        = ખાલી;
            io_module->funcTypes[i_firstIndex + j] = ftype;
        }

        જો (pass == 1) {
            *io_bytes = bytes;
        }
    }

} _catch:
    જો (placeholders) {
        માટે (u32 j = 0; j < i_ગણતરી; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    પાછા result;
}

#endif // d_m3HasStackSwitching


M3Result ParseSection_Type (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    IM3FuncType ftype = ખાલી;

_try {
    u32 અંક_Types;
_   (ReadLEB_u32(&અંક_Types, &i_bytes, i_end));                                      m3log (parse, "** Type [%d]", અંક_Types);

    _throwif("too many types", અંક_Types > d_m3_Max_Sane_Types_ગણતરી);

    જો (અંક_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, અંક_Types);
        _throwifnull(io_module->funcTypes);
        io_module->અંક_Func_Types = અંક_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#જો d_m3_Has_ઢગલો_Switching
        u32 capacity = અંક_Types;
#endif
        u32 index = 0;

        માટે (u32 i = 0; i < અંક_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_bytes, i_end));

#જો d_m3_Has_ઢગલો_Switching
            જો (form == -50) { // rec
                u32 ગણતરી;
_               (ReadLEB_u32(&ગણતરી, &i_bytes, i_end));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + ગણતરી + (અંક_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_ગણતરી);

                જો (needed > capacity) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, capacity);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    capacity                = grown;
                    io_module->અંક_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                જો (ગણતરી == 1) {
_                   (ReadLEB_i7(&form, &i_bytes, i_end));
                } નહીંતર {
_                   (ParseRecGroup(io_module, ગણતરી, index, &i_bytes, i_end));
                    index += ગણતરી;
                    ચાલુ;
                }
            }
#endif

_           (ParseTypeDefinition(io_module, form, &ftype, &i_bytes, i_end));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_AddFuncType(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = ખાલી; // ownership transferred to environment
        }

        io_module->અંક_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

} _catch:

    જો (result) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->અંક_Func_Types = 0;
    }

    પાછા result;
}


M3Result ParseSection_Function (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 અંક_Functions;
_   (ReadLEB_u32(&અંક_Functions, &i_bytes, i_end));                                  m3log (parse, "** Function [%d]", અંક_Functions);

    _throwif("too many functions", અંક_Functions > d_m3_Max_Sane_Functions_ગણતરી);

_   (Module_PreallocFunctions(io_module, io_module->અંક_Functions + અંક_Functions));

    માટે (u32 i = 0; i < અંક_Functions; ++i) {
        u32 funcTypeIndex;
_       (ReadLEB_u32(&funcTypeIndex, &i_bytes, i_end));

_       (Module_AddFunction(io_module, funcTypeIndex, ખાલી /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: પાછા result;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
પ્રકારવ્યાખ્યા સંરચના M3ImportDesc {
    u8           kind;
    u32          typeIndex;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3TableInfo  table;
    M3MemoryInfo memory;
} M3ImportDesc;


#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
સ્થિર
M3Result ReadTagType (IM3Module i_module, u32* o_typeIndex, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_bytes, i_end));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_typeIndex, io_bytes, i_end));
    _throwif(m3Err_unknownType, not i_module or *o_typeIndex >= i_module->અંક_Func_Types);

#  જો !d_m3_Has_ઢગલો_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_અંક_Results(i_module->funcTypes[*o_typeIndex]) != 0);
#  endif

    _catch: પાછા result;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
સ્થિર
M3Result ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, bytes_t* io_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

_   (Read_u8(&o_desc->kind, io_bytes, i_end));

    પસંદગી (o_desc->kind) {
    સ્થિતિ d_externalKind_function:
_       (ReadLEB_u32(&o_desc->typeIndex, io_bytes, i_end));
        તોડો;

    સ્થિતિ d_externalKind_table:
_       (ReadType_TableType(i_module, &o_desc->table, io_bytes, i_end));
        તોડો;

    સ્થિતિ d_externalKind_memory:
_       (ParseType_Memory(&o_desc->memory, io_bytes, i_end));
        તોડો;

    સ્થિતિ d_externalKind_global:
_       (Parse_કિંમત_Type(i_module, &o_desc->type, io_bytes, i_end));
_       (ReadLEB_u7(&o_desc->isMutable, io_bytes, i_end));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [BaseTypeOf(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        તોડો;

#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
    સ્થિતિ d_externalKind_tag:
_       (ReadTagType(i_module, &o_desc->typeIndex, io_bytes, i_end));
        તોડો;
#endif

    મૂળભૂત:
        _throw(m3Err_wasmMalformed);
    }

    _catch: પાછા result;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
સ્થિર
M3Result ApplyImportDesc (IM3Module io_module, અચલ M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3Result result = m3Err_none;

    M3ImportInfo clearImport = { ખાલી, ખાલી };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->kind, io_import->moduleUtf8, io_import->fieldUtf8);
    પસંદગી (i_desc->kind) {
    સ્થિતિ d_externalKind_function: {
_       (Module_AddFunction(io_module, i_desc->typeIndex, io_import))
        *io_import = clearImport;

        io_module->અંક_Func_Imports++;
    } તોડો;

    સ્થિતિ d_externalKind_table: {
        IM3Table table;
_       (Module_AddTable(io_module, &table, &i_desc->table, true /* isImport */));
        table->import = *io_import;
        *io_import    = clearImport;
    } તોડો;

    સ્થિતિ d_externalKind_memory: {
        IM3Memory memory;

#જો d_m3HasMultiMemory
        _throwif(m3Err_tooManyMemorySections, io_module->અંક_Memories >= d_m3_Max_Sane_Memories_ગણતરી);
#નહીંતર
        // MVP: one memory per module, imported or declared
        _throwif(m3Err_tooManyMemorySections, io_module->અંક_Memories >= 1);
#endif

_       (Module_AddMemory(io_module, &memory, &i_desc->memory, true /* isImport */));
        memory->import = *io_import;
        *io_import     = clearImport;
    } તોડો;

    સ્થિતિ d_externalKind_global: {
        IM3Global global;
_       (Module_AddGlobal(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } તોડો;

#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
    સ્થિતિ d_externalKind_tag: {
        IM3Tag tag;
_       (Module_AddTag(io_module, &tag, io_module->funcTypes[i_desc->typeIndex], true /* isImport */));
        tag->import = *io_import;
        *io_import  = clearImport;
    } તોડો;
#endif

    મૂળભૂત:
        _throw(m3Err_wasmMalformed);
    }

    _catch: પાછા result;
}


#જો d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
સ્થિર
bool IsEmptyName (bytes_t i_bytes, cbytes_t i_end)
{
    u32 length;
    પાછા (not ReadLEB_u32(&length, &i_bytes, i_end)) and length == 0;
}

#endif // d_m3HasCompactImports


#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching

M3Result ParseSection_Tag (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 અંક_Tags;
_   (ReadLEB_u32(&અંક_Tags, &i_bytes, i_end));                                       m3log (parse, "** Tag [%d]", અંક_Tags);

    _throwif("too many tags", અંક_Tags > d_m3_Max_Sane_Tags_ગણતરી);

    માટે (u32 i = 0; i < અંક_Tags; ++i) {
        u32 typeIndex;
_       (ReadTagType(io_module, &typeIndex, &i_bytes, i_end));                      m3log (parse, "    tag: [%d] type: %d", i, typeIndex);

_       (Module_AddTag(io_module, ખાલી, io_module->funcTypes[typeIndex], false));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: પાછા result;
}

#endif // d_m3HasExceptionHandling


M3Result ParseSection_Import (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    M3ImportInfo import = { ખાલી, ખાલી };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 અંક_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 અંક_Entries;
_   (ReadLEB_u32(&અંક_Entries, &i_bytes, i_end));                                    m3log (parse, "** Import [%d]", અંક_Entries);

    _throwif("too many imports", અંક_Entries > d_m3_Max_Sane_Imports_ગણતરી);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->અંક_Functions + અંક_Entries));

    માટે (u32 i = 0; i < અંક_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_bytes, i_end));

#જો d_m3HasCompactImports
        bytes_t fieldStart = i_bytes;
#endif
_       (Read_utf8(&import.fieldUtf8, &i_bytes, i_end));

#જો d_m3HasCompactImports
        u8 compact = (i_bytes < i_end) ? *i_bytes : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        જો ((compact == d_compactImports_perItemType or compact == d_compactImports_sharedType) and
            IsEmptyName(fieldStart, i_end)) {
            ++i_bytes;

            m3_Free(import.fieldUtf8);         // it only marked the compact form
            import.fieldUtf8 = ખાલી;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            જો (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_bytes, i_end));
            }

            u32 અંક_વસ્તુઓ;
_           (ReadLEB_u32(&અંક_વસ્તુઓ, &i_bytes, i_end));                              m3log (parse, "    compact: %d import(s) of '%s'", અંક_વસ્તુઓ, import.moduleUtf8);

            _throwif("too many imports", (u64)અંક_Imports + અંક_વસ્તુઓ > d_m3_Max_Sane_Imports_ગણતરી);
            અંક_Imports += અંક_વસ્તુઓ;

_           (Module_PreallocFunctions(io_module, io_module->અંક_Functions + અંક_વસ્તુઓ));

            માટે (u32 j = 0; j < અંક_વસ્તુઓ; ++j) {
                M3ImportInfo item = { ખાલી, ખાલી };

                item.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, strlen(import.moduleUtf8) + 1);

                જો (item.moduleUtf8) {
                    result = Read_utf8(&item.fieldUtf8, &i_bytes, i_end);

                    // the shared form reuses the externtype read above
                    જો (not result and compact == d_compactImports_perItemType) {
                        result = ReadImportDesc(io_module, &desc, &i_bytes, i_end);
                    }

                    જો (not result) {
                        result = ApplyImportDesc(io_module, &desc, &item);
                    }
                } નહીંતર {
                    result = m3Err_mallocFailed;
                }

                FreeImportInfo(&item);
                _throwif(result, result);
            }

            FreeImportInfo(&import);
            ચાલુ;
        }
#endif // d_m3HasCompactImports

        _throwif("too many imports", અંક_Imports >= d_m3_Max_Sane_Imports_ગણતરી);
        ++અંક_Imports;

_       (ReadImportDesc(io_module, &desc, &i_bytes, i_end));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    પાછા result;
}


M3Result ParseSection_Export (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result    result = m3Err_none;
    અચલ અક્ષર* utf8   = ખાલી;

    u32 અંક_Exports;
_   (ReadLEB_u32(&અંક_Exports, &i_bytes, i_end));                                    m3log (parse, "** Export [%d]", અંક_Exports);

    _throwif("too many exports", અંક_Exports > d_m3_Max_Sane_Exports_ગણતરી);

    // the section order check lets this section through once, so this is
    // every export the module has
    જો (અંક_Exports) {
        io_module->exports = m3_AllocArray(M3Export, અંક_Exports);
        _throwifnull(io_module->exports);
    }

    માટે (u32 i = 0; i < અંક_Exports; ++i) {
        u8  exportKind;
        u32 index;
        u32 nameLength;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            bytes_t lengthBytes = i_bytes;
_           (ReadLEB_u32(&nameLength, &lengthBytes, i_end));
        }

_       (Read_utf8(&utf8, &i_bytes, i_end));
_       (Read_u8(&exportKind, &i_bytes, i_end));
_       (ReadLEB_u32(&index, &i_bytes, i_end));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) exportKind, utf8);

#જો d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        માટે (u32 j = 0; j < i; ++j) {
            અચલ M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->nameLength == nameLength and memcmp(other->name, utf8, nameLength) == 0);
        }
#endif

        જો (exportKind == d_externalKind_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->અંક_Functions);
            IM3Function func = &(io_module->functions[index]);
            જો (func->અંક_Names < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, nameLength + 1);
                _throwifnull(name);
                func->names[func->અંક_Names++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } નહીંતર જો (exportKind == d_externalKind_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->અંક_Globals);
        } નહીંતર જો (exportKind == d_externalKind_memory) {
            _throwif(m3Err_wasmMalformed, index >= io_module->અંક_Memories);
        } નહીંતર જો (exportKind == d_externalKind_table) {
            _throwif(m3Err_wasmMalformed, index >= io_module->અંક_Tables);
        }
#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
        નહીંતર જો (exportKind == d_externalKind_tag) {
            _throwif(m3Err_wasmMalformed, index >= io_module->અંક_Tags);
        }
#endif

        M3Export* entry   = &io_module->exports[io_module->અંક_Exports++];
        entry->name       = utf8;
        entry->nameLength = nameLength;
        entry->index      = index;
        entry->kind       = exportKind;

        utf8 = ખાલી; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

_catch:
    m3_Free(utf8);
    પાછા result;
}


M3Result ParseSection_Start (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 startFuncIndex;
_   (ReadLEB_u32(&startFuncIndex, &i_bytes, i_end));                                  m3log (parse, "** Start Function: %d", startFuncIndex);

    જો (startFuncIndex < io_module->અંક_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[startFuncIndex];
        જો (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->અંક_Args != 0 || func->funcType->અંક_Rets != 0);
        }

        io_module->startFunction = startFuncIndex;
    } નહીંતર {
        result = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: પાછા result;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3Result Parse_Init_પદાવલિ_Typed (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end, m3type_t* o_type)
{
    M3Result result = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#જો defined(d_m3PreferStaticAlloc)
    સ્થિર M3Compilation compilation;
#નહીંતર
    M3Compilation compilation;
#endif
    compilation = (M3Compilation){ .runtime = ખાલી, .module = io_module, .wasm = *io_bytes, .wasmEnd = i_end, .is_Init_પદાવલિ = true };

    result = CompileBlockStatements(&compilation);

    *io_bytes = compilation.wasm;

    જો (o_type) {
        *o_type = compilation.ઢગલો_Index ? compilation.type_ઢગલો[compilation.ઢગલો_Index - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    પાછા result;
}


M3Result Parse_Init_પદાવલિ (M3Module* io_module, bytes_t* io_bytes, cbytes_t i_end)
{
    પાછા Parse_Init_પદાવલિ_Typed(io_module, io_bytes, i_end, ખાલી);
}


M3Result ParseSection_Element (IM3Module io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32     અંક_Segments;
    bytes_t pos;
_   (ReadLEB_u32(&અંક_Segments, &i_bytes, i_end));                            m3log (parse, "** Element [%d]", અંક_Segments);

    _throwif("too many element segments", અંક_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, અંક_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->અંક_Element_Segments = અંક_Segments;
    io_module->elementSectionEnd  = i_end;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_bytes;
    માટે (u32 i = 0; i < અંક_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_end));
#જો d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#નહીંતર
        _throwif(m3Err_wasmMalformed, flags != c_m3Elem_active);
#endif

        segment->mode   = flags & 0x3;
        segment->is_પદાવલિ = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        જો (segment->mode == c_m3Elem_activeIdx) {
_           (ReadLEB_u32(&segment->tableIndex, &pos, i_end));
        }

        bool isActive = (segment->mode == c_m3Elem_active or segment->mode == c_m3Elem_activeIdx);

        જો (isActive) {
            _throwif(m3Err_wasmMalformed, segment->tableIndex >= io_module->અંક_Tables);

            segment->init_પદાવલિ = pos;
_           (Parse_Init_પદાવલિ(io_module, &pos, i_end));
            segment->init_પદાવલિ_Size = (u32)(pos - segment->init_પદાવલિ);
            _throwif(m3_Err_wasm_Missing_Init_પદાવલિ, segment->init_પદાવલિ_Size <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        જો (segment->mode != c_m3Elem_active) {
            જો (segment->is_પદાવલિ) {
_               (Parse_કિંમત_Type(io_module, &segment->type, &pos, i_end));
                _throwif(m3Err_wasmMalformed, not IsRefType(BaseTypeOf(segment->type)));
            } નહીંતર {
                u8 elemKind;
_               (Read_u8(&elemKind, &pos, i_end));
                _throwif(m3Err_wasmMalformed, elemKind != 0x00);   // funcref
            }
        }

        જો (isActive) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->tableIndex]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->અંક_Elements, &pos, i_end));
        _throwif("table overflow", segment->અંક_Elements > d_m3MaxSaneTableSize);

        segment->elements = pos;

        માટે (u32 e = 0; e < segment->અંક_Elements; ++e) {
            જો (segment->is_પદાવલિ) {
_               (Parse_Init_પદાવલિ(io_module, &pos, i_end))
            } નહીંતર {
                u32 funcIndex;
_               (ReadLEB_u32(&funcIndex, &pos, i_end));
                _throwif("function index out of range", funcIndex >= io_module->અંક_Functions);
_               (Module_DeclareFunction(io_module, funcIndex));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_end);           // section size mismatch

    _catch: પાછા result;
}


M3Result ParseSection_Code (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result;

    u32 અંક_Functions;
_   (ReadLEB_u32(&અંક_Functions, &i_bytes, i_end));                                  m3log (parse, "** Code [%d]", અંક_Functions);

    જો (અંક_Functions != io_module->અંક_Functions - io_module->અંક_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    માટે (u32 f = 0; f < અંક_Functions; ++f) {
        અચલ u8* start = i_bytes;

        u32 size;
_       (ReadLEB_u32(&size, &i_bytes, i_end));

        જો (size) {
            i_bytes += size;

            જો (i_bytes <= i_end) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->અંક_Func_Imports);

                func->module  = io_module;
                func->wasm    = start;
                func->wasmEnd = i_bytes;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } નહીંતર {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    જો (not result and i_bytes != i_end) {
        result = m3Err_wasmSectionUnderrun;
    }

    પાછા result;
}


M3Result ParseSection_Data (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 અંક_Data_Segments;
_   (ReadLEB_u32(&અંક_Data_Segments, &i_bytes, i_end));                               m3log (parse, "** Data [%d]", અંક_Data_Segments);

    _throwif("too many data segments", અંક_Data_Segments > d_m3MaxSaneDataSegments);

    io_module->dataSegments = m3_AllocArray(M3DataSegment, અંક_Data_Segments);
    _throwifnull(io_module->dataSegments);
    io_module->અંક_Data_Segments = અંક_Data_Segments;

    માટે (u32 i = 0; i < અંક_Data_Segments; ++i) {
        M3DataSegment* segment = &io_module->dataSegments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_bytes, i_end));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        જો (flags == 2) {
_           (ReadLEB_u32(&segment->memoryRegion, &i_bytes, i_end));
        }

        જો (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#જો !d_m3HasMultiMemory
            _throwif(m3Err_wasmMalformed, segment->memoryRegion != 0);
#endif
            _throwif(m3Err_wasmMalformed, segment->memoryRegion >= io_module->અંક_Memories);

            segment->init_પદાવલિ = i_bytes;
_           (Parse_Init_પદાવલિ(io_module, &i_bytes, i_end));
            segment->init_પદાવલિ_Size = (u32)(i_bytes - segment->init_પદાવલિ);

            _throwif(m3_Err_wasm_Missing_Init_પદાવલિ, segment->init_પદાવલિ_Size <= 1);
        }

_       (ReadLEB_u32(&segment->size, &i_bytes, i_end));
        segment->data = i_bytes;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->memoryRegion, segment->init_પદાવલિ_Size, segment->size);
        i_bytes += segment->size;

        _throwif("data segment underflow", i_bytes > i_end);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch:

    પાછા result;
}


M3Result Parse_Section_Data_ગણતરી (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

_   (ReadLEB_u32(&io_module->data_ગણતરી, &i_bytes, i_end));                       m3log (parse, "** DataCount [%d]", io_module->data_ગણતરી);

    io_module->has_Data_ગણતરી = true;

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: પાછા result;
}


M3Result ParseSection_Memory (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 અંક_Memories;
_   (ReadLEB_u32(&અંક_Memories, &i_bytes, i_end));                                m3log (parse, "** Memory [%d]", અંક_Memories);

#જો d_m3HasMultiMemory
    _throwif(m3Err_tooManyMemorySections,
             (u64)અંક_Memories + io_module->અંક_Memories > d_m3_Max_Sane_Memories_ગણતરી);
#નહીંતર
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3Err_tooManyMemorySections, અંક_Memories > 1);
    _throwif(m3Err_tooManyMemorySections, (u64)અંક_Memories + io_module->અંક_Memories > 1);
#endif

    માટે (u32 i = 0; i < અંક_Memories; ++i) {
        M3MemoryInfo info;
_       (ParseType_Memory(&info, &i_bytes, i_end));
_       (Module_AddMemory(io_module, ખાલી, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: પાછા result;
}


M3Result ParseSection_Global (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    u32 અંક_Globals;
_   (ReadLEB_u32(&અંક_Globals, &i_bytes, i_end));                                    m3log (parse, "** Global [%d]", અંક_Globals);

    _throwif("too many globals", અંક_Globals > d_m3_Max_Sane_Globals_ગણતરી);

    માટે (u32 i = 0; i < અંક_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_કિંમત_Type(io_module, &type, &i_bytes, i_end));
_       (ReadLEB_u7(&isMutable, &i_bytes, i_end));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [BaseTypeOf(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_AddGlobal(io_module, &global, type, isMutable, false /* isImport */));

        global->init_પદાવલિ = i_bytes;
_       (Parse_Init_પદાવલિ(io_module, &i_bytes, i_end));
        global->init_પદાવલિ_Size = (u32)(i_bytes - global->init_પદાવલિ);

        _throwif(m3_Err_wasm_Missing_Init_પદાવલિ, global->init_પદાવલિ_Size <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_bytes != i_end);      // section size mismatch

    _catch: પાછા result;
}


M3Result ParseSection_Name (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    cstr_t name = ખાલી;

    જ્યારે (i_bytes < i_end) {
        u8  nameType;
        u32 payloadLength;

_       (ReadLEB_u7(&nameType, &i_bytes, i_end));
_       (ReadLEB_u32(&payloadLength, &i_bytes, i_end));

        bytes_t start = i_bytes;
        _throwif(m3Err_wasmSectionOverrun, payloadLength > (size_t)(i_end - start));

        જો (nameType == 1) {
            u32 અંક_Names;
_           (ReadLEB_u32(&અંક_Names, &i_bytes, i_end));

            _throwif("too many names", અંક_Names > d_m3_Max_Sane_Functions_ગણતરી);

            માટે (u32 i = 0; i < અંક_Names; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_bytes, i_end));
_               (Read_utf8(&name, &i_bytes, i_end));

                જો (index < io_module->અંક_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    જો (func->અંક_Names == 0) {
                        func->names[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->અંક_Names = 1;

                        name = ખાલી; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_bytes = start + payloadLength;
    }

    _catch: પાછા result;
}


M3Result ParseSection_Custom (M3Module* io_module, bytes_t i_bytes, cbytes_t i_end)
{
    M3Result result = m3Err_none;

    cstr_t name = ખાલી;
_   (Read_utf8(&name, &i_bytes, i_end));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    જો (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_bytes, i_end));
#જો d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } નહીંતર જો (strcmp(name, "snapshot") == 0 || (strlen(name) > 9 && strncmp(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, strlen(snapName) + 1);
        જો (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->data           = i_bytes;
        snap->size           = (u32)(i_end - i_bytes);
        snap->next           = io_module->snapshots;
        io_module->snapshots = snap;
#endif
    } નહીંતર જો (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_bytes, i_end));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    પાછા result;
}


M3Result ParseModuleSection (M3Module* o_module, u8 i_sectionType, bytes_t i_bytes, u32 i_અંક_Bytes)
{
    M3Result result = m3Err_none;

    પ્રકારવ્યાખ્યા M3Result (*M3Parser)(M3Module*, bytes_t, cbytes_t);

    સ્થિર M3Parser s_parsers[] = {
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
        ParseSection_Data,      // 11
        Parse_Section_Data_ગણતરી, // 12
#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
        ParseSection_Tag,       // 13
#endif
    };

    M3Parser parser = ખાલી;

    જો (i_sectionType < M3_ગણતરી_OF(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    જો (parser) {
        cbytes_t end = i_bytes + i_અંક_Bytes;
        result       = parser(o_module, i_bytes, end);
    } નહીંતર {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    પાછા result;
}


M3Result m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_bytes, u32 i_અંક_Bytes)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_અંક_Bytes);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_અંક_Bytes);
    module->startFunction = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    અચલ u8* pos = i_bytes;
    અચલ u8* end = pos + i_અંક_Bytes;

    module->wasmStart = pos;
    module->wasmEnd   = end;

    u32 magic, version;
_   (Read_u32(&magic, &pos, end));
_   (Read_u32(&version, &pos, end));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3Err_incompatibleWasmVersion, version != 1);

#જો d_m3HasExceptionHandling || d_m3_Has_ઢગલો_Switching
    // the tag section sits between memory (5) and global (6)
    સ્થિર અચલ u8 sectionsOrder[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#નહીંતર
    સ્થિર અચલ u8 sectionsOrder[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#endif
    u8 expectedSection = 0;

    જ્યારે (pos < end) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, end));

        જો (section != 0) {
            // Ensure sections appear only once and in order
            જ્યારે (sectionsOrder[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_ગણતરી_OF(sectionsOrder) - 1);
            }
        }

        u32 sectionLength;
_       (ReadLEB_u32(&sectionLength, &pos, end));
        _throwif(m3Err_wasmMalformed, pos + sectionLength > end);

_       (ParseModuleSection(module, section, pos, sectionLength));

        pos += sectionLength;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    જો (module->અંક_Functions > module->અંક_Func_Imports) {
        IM3Function firstNonImport = &module->functions[module->અંક_Func_Imports];
        _throwif(m3Err_wasmMalformed, firstNonImport->wasm == ખાલી);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_Data_ગણતરી and module->data_ગણતરી != module->અંક_Data_Segments);

} _catch:

    જો (result) {
        m3_FreeModule(module);
        module = ખાલી;
    }

    *o_module = module;

    પાછા result;
}
