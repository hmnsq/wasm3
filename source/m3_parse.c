//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_exception.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#ਜੇ d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_ਨਤੀਜਾ Parse_ਢੇਰ_Type (IM3Module i_module, m3type_t* o_ਢੇਰ_ਬਿੱਟ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    i64 ਢੇਰ_2;
_   (ReadLebSigned(&ਢੇਰ_2, 33, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

    ਜੇ (ਢੇਰ_2 == -d_waType_funcref) {
        *o_ਢੇਰ_ਬਿੱਟ = d_m3_Type_ਢੇਰ_Abstract;
    } ਨਹੀਂਤਾਂ ਜੇ (ਢੇਰ_2 == -d_waType_externref) {
        *o_ਢੇਰ_ਬਿੱਟ = d_m3Type_refExtern | d_m3_Type_ਢੇਰ_Abstract;
#  ਜੇ d_m3_Has_ਢੇਰ_Switching
    } ਨਹੀਂਤਾਂ ਜੇ (ਢੇਰ_2 == -d_wa_Type_ਢੇਰ_cont) {
        *o_ਢੇਰ_ਬਿੱਟ = d_m3Type_refCont | d_m3_Type_ਢੇਰ_Abstract;
    } ਨਹੀਂਤਾਂ ਜੇ (ਢੇਰ_2 == -d_wa_Type_ਢੇਰ_nocont) {
        *o_ਢੇਰ_ਬਿੱਟ = d_m3Type_refCont | d_m3_Type_ਢੇਰ_None;
#  endif
    } ਨਹੀਂਤਾਂ ਜੇ (ਢੇਰ_2 >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) ਢੇਰ_2 >= i_module->ਅੰਕ_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[ਢੇਰ_2];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_ਢੇਰ_ਬਿੱਟ = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_ਸੂਚਕਾਂਕ;
    } ਨਹੀਂਤਾਂ {
        _throw(m3Err_invalidTypeId);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_ਨਤੀਜਾ Parse_ਮੁੱਲ_Type (IM3Module i_module, m3type_t* o_type, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_ਬਾਈਟਾਂ >= i_ਸਿਰਾ);

#ਜੇ d_m3HasTypedRefs
    ਜੇ (**io_ਬਾਈਟਾਂ == d_waEncode_ref or **io_ਬਾਈਟਾਂ == d_waEncode_refNull) {
        u8 lead = *(*io_ਬਾਈਟਾਂ)++;

        m3type_t ਢੇਰ_ਬਿੱਟ = d_m3_Type_ਢੇਰ_Abstract;
_       (Parse_ਢੇਰ_Type(i_module, &ਢੇਰ_ਬਿੱਟ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

        *o_type = d_m3Type_ref | ਢੇਰ_ਬਿੱਟ | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        ਵਾਪਸ ਨਤੀਜਾ;
    }
#ਜੇ_ਅੰਤ

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
ਸਥਿਰ
M3_ਨਤੀਜਾ Read_Type_ਸਾਰਣੀ_Type (IM3Module i_module, M3_ਸਾਰਣੀ_Info* o_info, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_ਮਾਪ = 0, max_ਮਾਪ = 0;

    // the width of the limits, and of every index into the table
    u32 addr_ਬਿੱਟ = 32;

_   (Parse_ਮੁੱਲ_Type(i_module, &elemType, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
#ਜੇ d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(ਅਧਾਰ_Type_ਦਾ(elemType)));
#ਨਹੀਂਤਾਂ
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#ਜੇ_ਅੰਤ

_   (ReadLEB_u7(&flag, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#ਜੇ d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#ਨਹੀਂਤਾਂ
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#ਜੇ_ਅੰਤ

    ਜੇ (flag & 0x04u) {
        addr_ਬਿੱਟ = 64;
    }

_   (ReadLebUnsigned(&init_ਮਾਪ, addr_ਬਿੱਟ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    _throwif("table overflow", init_ਮਾਪ > d_m3_Max_Sane_ਸਾਰਣੀ_ਮਾਪ);

    ਜੇ (flag & 0x01u) {
_       (ReadLebUnsigned(&max_ਮਾਪ, addr_ਬਿੱਟ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        _throwif(m3Err_wasmMalformed, max_ਮਾਪ < init_ਮਾਪ);
        _throwif("table overflow", max_ਮਾਪ > d_m3_Max_Sane_ਸਾਰਣੀ_ਮਾਪ);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_ਮਾਪ  = (u32)init_ਮਾਪ;
    o_info->max_ਮਾਪ   = (u32)max_ਮਾਪ;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    ਵਾਪਸ ਨਤੀਜਾ;
}


ਸਥਿਰ
M3_ਨਤੀਜਾ Parse_Type_ਸਾਰਣੀ_Type (IM3Module io_module, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3_ਸਾਰਣੀ_Info info;

_   (Read_Type_ਸਾਰਣੀ_Type(io_module, &info, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_   (Module_ਜੋੜੋ_ਸਾਰਣੀ(io_module, ਨਿਰਾ, &info, false /* isImport */));

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Init_ਵਿਅੰਜਕ_Typed (M3Module* io_module, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ, m3type_t* o_type);

M3_ਨਤੀਜਾ Parse_Type_ਸਾਰਣੀ (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਅੰਕ_Tables;
_   (ReadLEB_u32(&ਅੰਕ_Tables, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                          m3log (parse, "** Table [%d]", ਅੰਕ_Tables);

    _throwif("too many tables", (u64)ਅੰਕ_Tables + io_module->ਅੰਕ_Tables > d_m3_Max_Sane_ਸਾਰਣੀ_ਗਿਣਤੀ);

    ਲਈ (u32 i = 0; i < ਅੰਕ_Tables; ++i) {
#ਜੇ d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_ਵਿਅੰਜਕ = (i_ਬਾਈਟਾਂ < i_ਸਿਰਾ) and (*i_ਬਾਈਟਾਂ == 0x40);
        u32  ਸਾਰਣੀ_ਸੂਚਕਾਂਕ  = io_module->ਅੰਕ_Tables;

        ਜੇ (has_Init_ਵਿਅੰਜਕ) {
            u8 reserved;
            ++i_ਬਾਈਟਾਂ;
_           (Read_u8(&reserved, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_ਸਾਰਣੀ_Type(io_module, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

        ਜੇ (has_Init_ਵਿਅੰਜਕ) {
            IM3_ਸਾਰਣੀ ਸਾਰਣੀ_2 = io_module->tables[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ];

            m3type_t initType;

            ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ = i_ਬਾਈਟਾਂ;
_           (Parse_Init_ਵਿਅੰਜਕ_Typed(io_module, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ, &initType));
            ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ_ਮਾਪ = (u32)(i_ਬਾਈਟਾਂ - ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ);
            _throwif(m3_Err_wasm_Missing_Init_ਵਿਅੰਜਕ, ਸਾਰਣੀ_2->init_ਵਿਅੰਜਕ_ਮਾਪ <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ਦਾ(initType, ਸਾਰਣੀ_2->type));
        } ਨਹੀਂਤਾਂ {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[ਸਾਰਣੀ_ਸੂਚਕਾਂਕ]->type));
        }
#ਨਹੀਂਤਾਂ
_       (Parse_Type_ਸਾਰਣੀ_Type(io_module, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
#ਜੇ_ਅੰਤ
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Type_ਸਮ੍ਰਿਤੀ (M3_ਸਮ੍ਰਿਤੀ_Info* o_ਸਮ੍ਰਿਤੀ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_ਪੰਨਾ_ਮਾਪ = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_ਬਿੱਟ = 32;

_   (ReadLEB_u7(&flag, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 ਗਿਆਤ = 0x09u;
#ਜੇ d_m3HasAtomics
        ਗਿਆਤ |= 0x02u;
#ਜੇ_ਅੰਤ
#ਜੇ d_m3HasMemory64
        ਗਿਆਤ |= 0x04u;
#ਜੇ_ਅੰਤ
        _throwif(m3Err_wasmMalformed, flag & ~ਗਿਆਤ);
    }

    o_ਸਮ੍ਰਿਤੀ->isShared = (flag & (1u << 1)) != 0;

    o_ਸਮ੍ਰਿਤੀ->isMemory64 = (flag & (1u << 2)) != 0;
    ਜੇ (o_ਸਮ੍ਰਿਤੀ->isMemory64) {
        addr_ਬਿੱਟ = 64;
    }

_   (ReadLebUnsigned(&o_ਸਮ੍ਰਿਤੀ->init_ਪੰਨੇ, addr_ਬਿੱਟ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

    o_ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ = 0;
    o_ਸਮ੍ਰਿਤੀ->hasMax   = (flag & (1u << 0)) != 0;
    ਜੇ (o_ਸਮ੍ਰਿਤੀ->hasMax) {
_       (ReadLebUnsigned(&o_ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ, addr_ਬਿੱਟ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ < o_ਸਮ੍ਰਿਤੀ->init_ਪੰਨੇ);
    }

    _throwif("shared memory must have maximum", o_ਸਮ੍ਰਿਤੀ->isShared and not o_ਸਮ੍ਰਿਤੀ->hasMax);

    o_ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ = 0;
    ਜੇ (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_ਪੰਨਾ_ਮਾਪ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

        _throwif("invalid custom page size", log_ਪੰਨਾ_ਮਾਪ > 16);

        o_ਸਮ੍ਰਿਤੀ->ਪੰਨਾ_ਮਾਪ = 1u << log_ਪੰਨਾ_ਮਾਪ;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_ਪੰਨੇ_Allowed = (log_ਪੰਨਾ_ਮਾਪ or addr_ਬਿੱਟ < 64)
                                ? (UINT64_C(1) << (addr_ਬਿੱਟ - log_ਪੰਨਾ_ਮਾਪ))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_ਸਮ੍ਰਿਤੀ->init_ਪੰਨੇ > max_ਪੰਨੇ_Allowed);
        ਜੇ (o_ਸਮ੍ਰਿਤੀ->hasMax) {
            _throwif(m3Err_wasmMalformed, o_ਸਮ੍ਰਿਤੀ->max_ਪੰਨੇ > max_ਪੰਨੇ_Allowed);
        }
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
ਸਥਿਰ
M3_ਨਤੀਜਾ ParseFuncType (IM3Module io_module, IM3FuncType* o_type, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    IM3FuncType ftype = ਨਿਰਾ;
_try {
    u32 ਅੰਕ_ਦਲੀਲਾਂ;
_   (ReadLEB_u32(&ਅੰਕ_ਦਲੀਲਾਂ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

    _throwif(m3_Err_too_Many_ਦਲੀਲਾਂ_Rets, ਅੰਕ_ਦਲੀਲਾਂ > d_m3_Max_Sane_Function_Arg_Ret_ਗਿਣਤੀ);
#ਜੇ M3_HAS_VLA
    m3type_t argTypes[ਅੰਕ_ਦਲੀਲਾਂ + 1]; // make ubsan happy
#ਨਹੀਂਤਾਂ
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_ਗਿਣਤੀ];
#ਜੇ_ਅੰਤ
    ਲਈ (u32 a = 0; a < ਅੰਕ_ਦਲੀਲਾਂ; ++a) {
_       (Parse_ਮੁੱਲ_Type(io_module, &argTypes[a], io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    }

    u32 ਅੰਕ_Rets;
_   (ReadLEB_u32(&ਅੰਕ_Rets, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    _throwif(m3_Err_too_Many_ਦਲੀਲਾਂ_Rets, (u64)(ਅੰਕ_Rets) + ਅੰਕ_ਦਲੀਲਾਂ > d_m3_Max_Sane_Function_Arg_Ret_ਗਿਣਤੀ);

_   (AllocFuncType(&ftype, ਅੰਕ_Rets + ਅੰਕ_ਦਲੀਲਾਂ));
    ftype->ਅੰਕ_ਦਲੀਲਾਂ = ਅੰਕ_ਦਲੀਲਾਂ;
    ftype->ਅੰਕ_Rets = ਅੰਕ_Rets;

    ਲਈ (u32 r = 0; r < ਅੰਕ_Rets; ++r) {
_       (Parse_ਮੁੱਲ_Type(io_module, &ftype->types[r], io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    }
    ਸਮ੍ਰਿਤੀ_ਨਕਲ(ftype->types + ਅੰਕ_Rets, argTypes, ਅੰਕ_ਦਲੀਲਾਂ * ਆਕਾਰ(m3type_t));

    *o_type = ftype;
    ftype   = ਨਿਰਾ;

} _catch:
    m3_Free(ftype);
    ਵਾਪਸ ਨਤੀਜਾ;
}


#ਜੇ d_m3_Has_ਢੇਰ_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
ਸਥਿਰ
M3_ਨਤੀਜਾ ParseContType (IM3Module io_module, IM3FuncType* o_type, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    IM3FuncType ftype = ਨਿਰਾ;
_try {
    i64 ft_ਸੂਚਕਾਂਕ;
_   (ReadLebSigned(&ft_ਸੂਚਕਾਂਕ, 33, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    _throwif(m3Err_wasmMalformed, ft_ਸੂਚਕਾਂਕ < 0 or (u64) ft_ਸੂਚਕਾਂਕ >= io_module->ਅੰਕ_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_ਸੂਚਕਾਂਕ];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 ਕੁੱਲ = underlying->ਅੰਕ_Rets + underlying->ਅੰਕ_ਦਲੀਲਾਂ;
_   (AllocFuncType(&ftype, ਕੁੱਲ));
    ftype->ਅੰਕ_Rets        = underlying->ਅੰਕ_Rets;
    ftype->ਅੰਕ_ਦਲੀਲਾਂ        = underlying->ਅੰਕ_ਦਲੀਲਾਂ;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    ਜੇ (ਕੁੱਲ) {
        ਸਮ੍ਰਿਤੀ_ਨਕਲ(ftype->types, underlying->types, ਕੁੱਲ * ਆਕਾਰ(m3type_t));
    }

    *o_type = ftype;
    ftype   = ਨਿਰਾ;

} _catch:
    m3_Free(ftype);
    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


ਸਥਿਰ
M3_ਨਤੀਜਾ ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
#ਜੇ d_m3_Has_ਢੇਰ_Switching
    ਜੇ (i_form == -35) {
        ਵਾਪਸ ParseContType(io_module, o_type, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);
    }
#ਜੇ_ਅੰਤ
    ਜੇ (i_form != -32) { // for Wasm MVP
        ਵਾਪਸ m3Err_wasmMalformed;
    }

    ਵਾਪਸ ParseFuncType(io_module, o_type, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ);
}


#ਜੇ d_m3_Has_ਢੇਰ_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
ਸਥਿਰ
M3_ਨਤੀਜਾ SkipTypeDefinition (i8 i_form, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
_try {
    ਜੇ (i_form == -35) {
        i64 ft_ਸੂਚਕਾਂਕ;
_       (ReadLebSigned(&ft_ਸੂਚਕਾਂਕ, 33, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    ਲਈ (u32 half = 0; half < 2; ++half) {
        u32 ਗਿਣਤੀ_2;
_       (ReadLEB_u32(&ਗਿਣਤੀ_2, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        _throwif(m3_Err_too_Many_ਦਲੀਲਾਂ_Rets, ਗਿਣਤੀ_2 > d_m3_Max_Sane_Function_Arg_Ret_ਗਿਣਤੀ);

        ਲਈ (u32 v = 0; v < ਗਿਣਤੀ_2; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

            ਜੇ (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 ਢੇਰ_2;
_               (ReadLebSigned(&ਢੇਰ_2, 33, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
            }
        }
    }

} _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
ਸਥਿਰ
M3_ਨਤੀਜਾ Parse_Rec_ਸਮੂਹ (IM3Module io_module, u32 i_ਗਿਣਤੀ, u32 i_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ,
                        ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    IM3FuncType* placeholders = ਨਿਰਾ;
_try {
    ਬਾਈਟਾਂ_t ਸਮੂਹ_ਸ਼ੁਰੂ = *io_ਬਾਈਟਾਂ;

    placeholders = m3_AllocArray(IM3FuncType, i_ਗਿਣਤੀ);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    ਲਈ (u32 j = 0; j < i_ਗਿਣਤੀ; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (SkipTypeDefinition(form, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 ਪਹਿਲਾ_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_ਗਿਣਤੀ, &ਪਹਿਲਾ_Canonical));

    ਲਈ (u32 j = 0; j < i_ਗਿਣਤੀ; ++j) {
        placeholders[j]->canonical_ਸੂਚਕਾਂਕ        = (u16)(ਪਹਿਲਾ_Canonical + j);
        io_module->funcTypes[i_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    ਲਈ (u32 pass = 0; pass < 2; ++pass) {
        ਬਾਈਟਾਂ_t ਬਾਈਟਾਂ = ਸਮੂਹ_ਸ਼ੁਰੂ;

        ਲਈ (u32 j = 0; j < i_ਗਿਣਤੀ; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &ਬਾਈਟਾਂ, i_ਸਿਰਾ));

            ਜੇ ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &ਬਾਈਟਾਂ, i_ਸਿਰਾ));
                ਜਾਰੀ;
            }

            IM3FuncType ftype = ਨਿਰਾ;
_           (ParseTypeDefinition(io_module, form, &ftype, &ਬਾਈਟਾਂ, i_ਸਿਰਾ));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_ਸੂਚਕਾਂਕ);

            m3_Free(placeholders[j]);
            placeholders[j]                        = ਨਿਰਾ;
            io_module->funcTypes[i_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ + j] = ftype;
        }

        ਜੇ (pass == 1) {
            *io_ਬਾਈਟਾਂ = ਬਾਈਟਾਂ;
        }
    }

} _catch:
    ਜੇ (placeholders) {
        ਲਈ (u32 j = 0; j < i_ਗਿਣਤੀ; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasStackSwitching


M3_ਨਤੀਜਾ ParseSection_Type (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    IM3FuncType ftype = ਨਿਰਾ;

_try {
    u32 ਅੰਕ_Types;
_   (ReadLEB_u32(&ਅੰਕ_Types, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                      m3log (parse, "** Type [%d]", ਅੰਕ_Types);

    _throwif("too many types", ਅੰਕ_Types > d_m3_Max_Sane_Types_ਗਿਣਤੀ);

    ਜੇ (ਅੰਕ_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, ਅੰਕ_Types);
        _throwifnull(io_module->funcTypes);
        io_module->ਅੰਕ_Func_Types = ਅੰਕ_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#ਜੇ d_m3_Has_ਢੇਰ_Switching
        u32 ਸਮਰੱਥਾ = ਅੰਕ_Types;
#ਜੇ_ਅੰਤ
        u32 index = 0;

        ਲਈ (u32 i = 0; i < ਅੰਕ_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

#ਜੇ d_m3_Has_ਢੇਰ_Switching
            ਜੇ (form == -50) { // rec
                u32 ਗਿਣਤੀ_2;
_               (ReadLEB_u32(&ਗਿਣਤੀ_2, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + ਗਿਣਤੀ_2 + (ਅੰਕ_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_ਗਿਣਤੀ);

                ਜੇ (needed > ਸਮਰੱਥਾ) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, ਸਮਰੱਥਾ);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    ਸਮਰੱਥਾ                = grown;
                    io_module->ਅੰਕ_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                ਜੇ (ਗਿਣਤੀ_2 == 1) {
_                   (ReadLEB_i7(&form, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
                } ਨਹੀਂਤਾਂ {
_                   (Parse_Rec_ਸਮੂਹ(io_module, ਗਿਣਤੀ_2, index, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
                    index += ਗਿਣਤੀ_2;
                    ਜਾਰੀ;
                }
            }
#ਜੇ_ਅੰਤ

_           (ParseTypeDefinition(io_module, form, &ftype, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_ਜੋੜੋ_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = ਨਿਰਾ; // ownership transferred to environment
        }

        io_module->ਅੰਕ_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

} _catch:

    ਜੇ (ਨਤੀਜਾ) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->ਅੰਕ_Func_Types = 0;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ParseSection_Function (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਅੰਕ_Functions;
_   (ReadLEB_u32(&ਅੰਕ_Functions, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                  m3log (parse, "** Function [%d]", ਅੰਕ_Functions);

    _throwif("too many functions", ਅੰਕ_Functions > d_m3_Max_Sane_Functions_ਗਿਣਤੀ);

_   (Module_PreallocFunctions(io_module, io_module->ਅੰਕ_Functions + ਅੰਕ_Functions));

    ਲਈ (u32 i = 0; i < ਅੰਕ_Functions; ++i) {
        u32 func_Type_ਸੂਚਕਾਂਕ;
_       (ReadLEB_u32(&func_Type_ਸੂਚਕਾਂਕ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

_       (Module_ਜੋੜੋ_Function(io_module, func_Type_ਸੂਚਕਾਂਕ, ਨਿਰਾ /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3ImportDesc {
    u8           ਕਿਸਮ;
    u32          type_ਸੂਚਕਾਂਕ;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_ਸਾਰਣੀ_Info  ਸਾਰਣੀ_2;
    M3_ਸਮ੍ਰਿਤੀ_Info ਸਮ੍ਰਿਤੀ;
} M3ImportDesc;


#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
ਸਥਿਰ
M3_ਨਤੀਜਾ Read_ਟੈਗ_Type (IM3Module i_module, u32* o_type_ਸੂਚਕਾਂਕ, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_ਸੂਚਕਾਂਕ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    _throwif(m3Err_unknownType, not i_module or *o_type_ਸੂਚਕਾਂਕ >= i_module->ਅੰਕ_Func_Types);

#  ਜੇ !d_m3_Has_ਢੇਰ_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_ਅੰਕ_Results(i_module->funcTypes[*o_type_ਸੂਚਕਾਂਕ]) != 0);
#  endif

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
ਸਥਿਰ
M3_ਨਤੀਜਾ ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (Read_u8(&o_desc->ਕਿਸਮ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

    ਚੋਣ (o_desc->ਕਿਸਮ) {
    ਹਾਲਤ d_external_ਕਿਸਮ_function:
_       (ReadLEB_u32(&o_desc->type_ਸੂਚਕਾਂਕ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        ਰੋਕੋ;

    ਹਾਲਤ d_external_ਕਿਸਮ_ਸਾਰਣੀ:
_       (Read_Type_ਸਾਰਣੀ_Type(i_module, &o_desc->ਸਾਰਣੀ_2, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        ਰੋਕੋ;

    ਹਾਲਤ d_external_ਕਿਸਮ_ਸਮ੍ਰਿਤੀ:
_       (Parse_Type_ਸਮ੍ਰਿਤੀ(&o_desc->ਸਮ੍ਰਿਤੀ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        ਰੋਕੋ;

    ਹਾਲਤ d_external_ਕਿਸਮ_global:
_       (Parse_ਮੁੱਲ_Type(i_module, &o_desc->type, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (ReadLEB_u7(&o_desc->isMutable, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [ਅਧਾਰ_Type_ਦਾ(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        ਰੋਕੋ;

#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
    ਹਾਲਤ d_external_ਕਿਸਮ_ਟੈਗ:
_       (Read_ਟੈਗ_Type(i_module, &o_desc->type_ਸੂਚਕਾਂਕ, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        ਰੋਕੋ;
#ਜੇ_ਅੰਤ

    ਮੂਲ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
ਸਥਿਰ
M3_ਨਤੀਜਾ ApplyImportDesc (IM3Module io_module, ਅਚਲ M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3ImportInfo clearImport = { ਨਿਰਾ, ਨਿਰਾ };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->ਕਿਸਮ, io_import->moduleUtf8, io_import->ਖੇਤਰ_Utf8);
    ਚੋਣ (i_desc->ਕਿਸਮ) {
    ਹਾਲਤ d_external_ਕਿਸਮ_function: {
_       (Module_ਜੋੜੋ_Function(io_module, i_desc->type_ਸੂਚਕਾਂਕ, io_import))
        *io_import = clearImport;

        io_module->ਅੰਕ_Func_Imports++;
    } ਰੋਕੋ;

    ਹਾਲਤ d_external_ਕਿਸਮ_ਸਾਰਣੀ: {
        IM3_ਸਾਰਣੀ ਸਾਰਣੀ_2;
_       (Module_ਜੋੜੋ_ਸਾਰਣੀ(io_module, &ਸਾਰਣੀ_2, &i_desc->ਸਾਰਣੀ_2, true /* isImport */));
        ਸਾਰਣੀ_2->import = *io_import;
        *io_import    = clearImport;
    } ਰੋਕੋ;

    ਹਾਲਤ d_external_ਕਿਸਮ_ਸਮ੍ਰਿਤੀ: {
        IM3_ਸਮ੍ਰਿਤੀ ਸਮ੍ਰਿਤੀ;

#ਜੇ d_m3_Has_Multi_ਸਮ੍ਰਿਤੀ
        _throwif(m3_Err_too_Many_ਸਮ੍ਰਿਤੀ_Sections, io_module->ਅੰਕ_Memories >= d_m3_Max_Sane_Memories_ਗਿਣਤੀ);
#ਨਹੀਂਤਾਂ
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_ਸਮ੍ਰਿਤੀ_Sections, io_module->ਅੰਕ_Memories >= 1);
#ਜੇ_ਅੰਤ

_       (Module_ਜੋੜੋ_ਸਮ੍ਰਿਤੀ(io_module, &ਸਮ੍ਰਿਤੀ, &i_desc->ਸਮ੍ਰਿਤੀ, true /* isImport */));
        ਸਮ੍ਰਿਤੀ->import = *io_import;
        *io_import     = clearImport;
    } ਰੋਕੋ;

    ਹਾਲਤ d_external_ਕਿਸਮ_global: {
        IM3Global global;
_       (Module_ਜੋੜੋ_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } ਰੋਕੋ;

#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
    ਹਾਲਤ d_external_ਕਿਸਮ_ਟੈਗ: {
        IM3_ਟੈਗ ਟੈਗ;
_       (Module_ਜੋੜੋ_ਟੈਗ(io_module, &ਟੈਗ, io_module->funcTypes[i_desc->type_ਸੂਚਕਾਂਕ], true /* isImport */));
        ਟੈਗ->import = *io_import;
        *io_import  = clearImport;
    } ਰੋਕੋ;
#ਜੇ_ਅੰਤ

    ਮੂਲ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


#ਜੇ d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
ਸਥਿਰ
bool Is_ਖਾਲੀ_ਥਾਵਾਂ_Name (ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    u32 ਲੰਮਾਈ;
    ਵਾਪਸ (not ReadLEB_u32(&ਲੰਮਾਈ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ)) and ਲੰਮਾਈ == 0;
}

#ਜੇ_ਅੰਤ // d_m3HasCompactImports


#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching

M3_ਨਤੀਜਾ Parse_Section_ਟੈਗ (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਅੰਕ_ਟੈਗ_ਸੂਚੀ;
_   (ReadLEB_u32(&ਅੰਕ_ਟੈਗ_ਸੂਚੀ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                       m3log (parse, "** Tag [%d]", ਅੰਕ_ਟੈਗ_ਸੂਚੀ);

    _throwif("too many tags", ਅੰਕ_ਟੈਗ_ਸੂਚੀ > d_m3_Max_Sane_ਟੈਗ_ਸੂਚੀ_ਗਿਣਤੀ);

    ਲਈ (u32 i = 0; i < ਅੰਕ_ਟੈਗ_ਸੂਚੀ; ++i) {
        u32 type_ਸੂਚਕਾਂਕ;
_       (Read_ਟੈਗ_Type(io_module, &type_ਸੂਚਕਾਂਕ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                      m3log (parse, "    tag: [%d] type: %d", i, type_ਸੂਚਕਾਂਕ);

_       (Module_ਜੋੜੋ_ਟੈਗ(io_module, ਨਿਰਾ, io_module->funcTypes[type_ਸੂਚਕਾਂਕ], false));
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਅੰਤ // d_m3HasExceptionHandling


M3_ਨਤੀਜਾ ParseSection_Import (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    M3ImportInfo import = { ਨਿਰਾ, ਨਿਰਾ };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 ਅੰਕ_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 ਅੰਕ_Entries;
_   (ReadLEB_u32(&ਅੰਕ_Entries, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                    m3log (parse, "** Import [%d]", ਅੰਕ_Entries);

    _throwif("too many imports", ਅੰਕ_Entries > d_m3_Max_Sane_Imports_ਗਿਣਤੀ);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->ਅੰਕ_Functions + ਅੰਕ_Entries));

    ਲਈ (u32 i = 0; i < ਅੰਕ_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

#ਜੇ d_m3HasCompactImports
        ਬਾਈਟਾਂ_t ਖੇਤਰ_ਸ਼ੁਰੂ = i_ਬਾਈਟਾਂ;
#ਜੇ_ਅੰਤ
_       (Read_utf8(&import.ਖੇਤਰ_Utf8, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

#ਜੇ d_m3HasCompactImports
        u8 compact = (i_ਬਾਈਟਾਂ < i_ਸਿਰਾ) ? *i_ਬਾਈਟਾਂ : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        ਜੇ ((compact == d_compact_Imports_ਪ੍ਰਤੀ_ਵਸਤੂ_Type or compact == d_compactImports_sharedType) and
            Is_ਖਾਲੀ_ਥਾਵਾਂ_Name(ਖੇਤਰ_ਸ਼ੁਰੂ, i_ਸਿਰਾ)) {
            ++i_ਬਾਈਟਾਂ;

            m3_Free(import.ਖੇਤਰ_Utf8);         // it only marked the compact form
            import.ਖੇਤਰ_Utf8 = ਨਿਰਾ;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            ਜੇ (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
            }

            u32 ਅੰਕ_ਚੀਜ਼ਾਂ;
_           (ReadLEB_u32(&ਅੰਕ_ਚੀਜ਼ਾਂ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                              m3log (parse, "    compact: %d import(s) of '%s'", ਅੰਕ_ਚੀਜ਼ਾਂ, import.moduleUtf8);

            _throwif("too many imports", (u64)ਅੰਕ_Imports + ਅੰਕ_ਚੀਜ਼ਾਂ > d_m3_Max_Sane_Imports_ਗਿਣਤੀ);
            ਅੰਕ_Imports += ਅੰਕ_ਚੀਜ਼ਾਂ;

_           (Module_PreallocFunctions(io_module, io_module->ਅੰਕ_Functions + ਅੰਕ_ਚੀਜ਼ਾਂ));

            ਲਈ (u32 j = 0; j < ਅੰਕ_ਚੀਜ਼ਾਂ; ++j) {
                M3ImportInfo ਵਸਤੂ = { ਨਿਰਾ, ਨਿਰਾ };

                ਵਸਤੂ.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, ਲੜੀ_ਲੰਬਾਈ(import.moduleUtf8) + 1);

                ਜੇ (ਵਸਤੂ.moduleUtf8) {
                    ਨਤੀਜਾ = Read_utf8(&ਵਸਤੂ.ਖੇਤਰ_Utf8, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ);

                    // the shared form reuses the externtype read above
                    ਜੇ (not ਨਤੀਜਾ and compact == d_compact_Imports_ਪ੍ਰਤੀ_ਵਸਤੂ_Type) {
                        ਨਤੀਜਾ = ReadImportDesc(io_module, &desc, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ);
                    }

                    ਜੇ (not ਨਤੀਜਾ) {
                        ਨਤੀਜਾ = ApplyImportDesc(io_module, &desc, &ਵਸਤੂ);
                    }
                } ਨਹੀਂਤਾਂ {
                    ਨਤੀਜਾ = m3Err_mallocFailed;
                }

                FreeImportInfo(&ਵਸਤੂ);
                _throwif(ਨਤੀਜਾ, ਨਤੀਜਾ);
            }

            FreeImportInfo(&import);
            ਜਾਰੀ;
        }
#ਜੇ_ਅੰਤ // d_m3HasCompactImports

        _throwif("too many imports", ਅੰਕ_Imports >= d_m3_Max_Sane_Imports_ਗਿਣਤੀ);
        ++ਅੰਕ_Imports;

_       (ReadImportDesc(io_module, &desc, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ParseSection_Export (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ    ਨਤੀਜਾ = m3Err_none;
    ਅਚਲ ਅੱਖਰ* utf8   = ਨਿਰਾ;

    u32 ਅੰਕ_Exports;
_   (ReadLEB_u32(&ਅੰਕ_Exports, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                    m3log (parse, "** Export [%d]", ਅੰਕ_Exports);

    _throwif("too many exports", ਅੰਕ_Exports > d_m3_Max_Sane_Exports_ਗਿਣਤੀ);

    // the section order check lets this section through once, so this is
    // every export the module has
    ਜੇ (ਅੰਕ_Exports) {
        io_module->exports = m3_AllocArray(M3Export, ਅੰਕ_Exports);
        _throwifnull(io_module->exports);
    }

    ਲਈ (u32 i = 0; i < ਅੰਕ_Exports; ++i) {
        u8  export_ਕਿਸਮ;
        u32 index;
        u32 name_ਲੰਮਾਈ;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            ਬਾਈਟਾਂ_t ਲੰਮਾਈ_ਬਾਈਟਾਂ = i_ਬਾਈਟਾਂ;
_           (ReadLEB_u32(&name_ਲੰਮਾਈ, &ਲੰਮਾਈ_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        }

_       (Read_utf8(&utf8, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (Read_u8(&export_ਕਿਸਮ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (ReadLEB_u32(&index, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_ਕਿਸਮ, utf8);

#ਜੇ d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        ਲਈ (u32 j = 0; j < i; ++j) {
            ਅਚਲ M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_ਲੰਮਾਈ == name_ਲੰਮਾਈ and memcmp(other->name, utf8, name_ਲੰਮਾਈ) == 0);
        }
#ਜੇ_ਅੰਤ

        ਜੇ (export_ਕਿਸਮ == d_external_ਕਿਸਮ_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ਅੰਕ_Functions);
            IM3Function func = &(io_module->functions[index]);
            ਜੇ (func->ਅੰਕ_ਨਾਂ < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_ਲੰਮਾਈ + 1);
                _throwifnull(name);
                func->ਨਾਂ[func->ਅੰਕ_ਨਾਂ++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } ਨਹੀਂਤਾਂ ਜੇ (export_ਕਿਸਮ == d_external_ਕਿਸਮ_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ਅੰਕ_Globals);
        } ਨਹੀਂਤਾਂ ਜੇ (export_ਕਿਸਮ == d_external_ਕਿਸਮ_ਸਮ੍ਰਿਤੀ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ਅੰਕ_Memories);
        } ਨਹੀਂਤਾਂ ਜੇ (export_ਕਿਸਮ == d_external_ਕਿਸਮ_ਸਾਰਣੀ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ਅੰਕ_Tables);
        }
#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
        ਨਹੀਂਤਾਂ ਜੇ (export_ਕਿਸਮ == d_external_ਕਿਸਮ_ਟੈਗ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ਅੰਕ_ਟੈਗ_ਸੂਚੀ);
        }
#ਜੇ_ਅੰਤ

        M3Export* ਇੰਦਰਾਜ   = &io_module->exports[io_module->ਅੰਕ_Exports++];
        ਇੰਦਰਾਜ->name       = utf8;
        ਇੰਦਰਾਜ->name_ਲੰਮਾਈ = name_ਲੰਮਾਈ;
        ਇੰਦਰਾਜ->index      = index;
        ਇੰਦਰਾਜ->ਕਿਸਮ       = export_ਕਿਸਮ;

        utf8 = ਨਿਰਾ; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

_catch:
    m3_Free(utf8);
    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Section_ਸ਼ੁਰੂ (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਸ਼ੁਰੂ_Func_ਸੂਚਕਾਂਕ;
_   (ReadLEB_u32(&ਸ਼ੁਰੂ_Func_ਸੂਚਕਾਂਕ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                  m3log (parse, "** Start Function: %d", ਸ਼ੁਰੂ_Func_ਸੂਚਕਾਂਕ);

    ਜੇ (ਸ਼ੁਰੂ_Func_ਸੂਚਕਾਂਕ < io_module->ਅੰਕ_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[ਸ਼ੁਰੂ_Func_ਸੂਚਕਾਂਕ];
        ਜੇ (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->ਅੰਕ_ਦਲੀਲਾਂ != 0 || func->funcType->ਅੰਕ_Rets != 0);
        }

        io_module->ਸ਼ੁਰੂ_Function = ਸ਼ੁਰੂ_Func_ਸੂਚਕਾਂਕ;
    } ਨਹੀਂਤਾਂ {
        ਨਤੀਜਾ = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_ਨਤੀਜਾ Parse_Init_ਵਿਅੰਜਕ_Typed (M3Module* io_module, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ, m3type_t* o_type)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#ਜੇ defined(d_m3PreferStaticAlloc)
    ਸਥਿਰ M3Compilation compilation;
#ਨਹੀਂਤਾਂ
    M3Compilation compilation;
#ਜੇ_ਅੰਤ
    compilation = (M3Compilation){ .runtime = ਨਿਰਾ, .module = io_module, .wasm = *io_ਬਾਈਟਾਂ, .wasm_ਸਿਰਾ = i_ਸਿਰਾ, .is_Init_ਵਿਅੰਜਕ = true };

    ਨਤੀਜਾ = Compile_ਖੰਡ_Statements(&compilation);

    *io_ਬਾਈਟਾਂ = compilation.wasm;

    ਜੇ (o_type) {
        *o_type = compilation.ਢੇਰ_ਸੂਚਕਾਂਕ ? compilation.type_ਢੇਰ[compilation.ਢੇਰ_ਸੂਚਕਾਂਕ - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Init_ਵਿਅੰਜਕ (M3Module* io_module, ਬਾਈਟਾਂ_t* io_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    ਵਾਪਸ Parse_Init_ਵਿਅੰਜਕ_Typed(io_module, io_ਬਾਈਟਾਂ, i_ਸਿਰਾ, ਨਿਰਾ);
}


M3_ਨਤੀਜਾ ParseSection_Element (IM3Module io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32     ਅੰਕ_Segments;
    ਬਾਈਟਾਂ_t pos;
_   (ReadLEB_u32(&ਅੰਕ_Segments, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                            m3log (parse, "** Element [%d]", ਅੰਕ_Segments);

    _throwif("too many element segments", ਅੰਕ_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, ਅੰਕ_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->ਅੰਕ_Element_Segments = ਅੰਕ_Segments;
    io_module->element_Section_ਸਿਰਾ  = i_ਸਿਰਾ;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_ਬਾਈਟਾਂ;
    ਲਈ (u32 i = 0; i < ਅੰਕ_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_ਸਿਰਾ));
#ਜੇ d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#ਨਹੀਂਤਾਂ
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_ਸਰਗਰਮ);
#ਜੇ_ਅੰਤ

        segment->ਬਹੁਲਕ   = flags & 0x3;
        segment->is_ਵਿਅੰਜਕ = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        ਜੇ (segment->ਬਹੁਲਕ == c_m3_Elem_ਸਰਗਰਮ_Idx) {
_           (ReadLEB_u32(&segment->ਸਾਰਣੀ_ਸੂਚਕਾਂਕ, &pos, i_ਸਿਰਾ));
        }

        bool is_ਸਰਗਰਮ = (segment->ਬਹੁਲਕ == c_m3_Elem_ਸਰਗਰਮ or segment->ਬਹੁਲਕ == c_m3_Elem_ਸਰਗਰਮ_Idx);

        ਜੇ (is_ਸਰਗਰਮ) {
            _throwif(m3Err_wasmMalformed, segment->ਸਾਰਣੀ_ਸੂਚਕਾਂਕ >= io_module->ਅੰਕ_Tables);

            segment->init_ਵਿਅੰਜਕ = pos;
_           (Parse_Init_ਵਿਅੰਜਕ(io_module, &pos, i_ਸਿਰਾ));
            segment->init_ਵਿਅੰਜਕ_ਮਾਪ = (u32)(pos - segment->init_ਵਿਅੰਜਕ);
            _throwif(m3_Err_wasm_Missing_Init_ਵਿਅੰਜਕ, segment->init_ਵਿਅੰਜਕ_ਮਾਪ <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        ਜੇ (segment->ਬਹੁਲਕ != c_m3_Elem_ਸਰਗਰਮ) {
            ਜੇ (segment->is_ਵਿਅੰਜਕ) {
_               (Parse_ਮੁੱਲ_Type(io_module, &segment->type, &pos, i_ਸਿਰਾ));
                _throwif(m3Err_wasmMalformed, not IsRefType(ਅਧਾਰ_Type_ਦਾ(segment->type)));
            } ਨਹੀਂਤਾਂ {
                u8 elem_ਕਿਸਮ;
_               (Read_u8(&elem_ਕਿਸਮ, &pos, i_ਸਿਰਾ));
                _throwif(m3Err_wasmMalformed, elem_ਕਿਸਮ != 0x00);   // funcref
            }
        }

        ਜੇ (is_ਸਰਗਰਮ) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->ਸਾਰਣੀ_ਸੂਚਕਾਂਕ]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->ਅੰਕ_Elements, &pos, i_ਸਿਰਾ));
        _throwif("table overflow", segment->ਅੰਕ_Elements > d_m3_Max_Sane_ਸਾਰਣੀ_ਮਾਪ);

        segment->elements = pos;

        ਲਈ (u32 e = 0; e < segment->ਅੰਕ_Elements; ++e) {
            ਜੇ (segment->is_ਵਿਅੰਜਕ) {
_               (Parse_Init_ਵਿਅੰਜਕ(io_module, &pos, i_ਸਿਰਾ))
            } ਨਹੀਂਤਾਂ {
                u32 func_ਸੂਚਕਾਂਕ;
_               (ReadLEB_u32(&func_ਸੂਚਕਾਂਕ, &pos, i_ਸਿਰਾ));
                _throwif("function index out of range", func_ਸੂਚਕਾਂਕ >= io_module->ਅੰਕ_Functions);
_               (Module_DeclareFunction(io_module, func_ਸੂਚਕਾਂਕ));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_ਸਿਰਾ);           // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Section_ਕੋਡ (M3Module* io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ;

    u32 ਅੰਕ_Functions;
_   (ReadLEB_u32(&ਅੰਕ_Functions, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                  m3log (parse, "** Code [%d]", ਅੰਕ_Functions);

    ਜੇ (ਅੰਕ_Functions != io_module->ਅੰਕ_Functions - io_module->ਅੰਕ_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    ਲਈ (u32 f = 0; f < ਅੰਕ_Functions; ++f) {
        ਅਚਲ u8* ਸ਼ੁਰੂ = i_ਬਾਈਟਾਂ;

        u32 ਮਾਪ;
_       (ReadLEB_u32(&ਮਾਪ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

        ਜੇ (ਮਾਪ) {
            i_ਬਾਈਟਾਂ += ਮਾਪ;

            ਜੇ (i_ਬਾਈਟਾਂ <= i_ਸਿਰਾ) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->ਅੰਕ_Func_Imports);

                func->module  = io_module;
                func->wasm    = ਸ਼ੁਰੂ;
                func->wasm_ਸਿਰਾ = i_ਬਾਈਟਾਂ;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } ਨਹੀਂਤਾਂ {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    ਜੇ (not ਨਤੀਜਾ and i_ਬਾਈਟਾਂ != i_ਸਿਰਾ) {
        ਨਤੀਜਾ = m3Err_wasmSectionUnderrun;
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Section_ਅੰਕੜੇ (M3Module* io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਅੰਕ_ਅੰਕੜੇ_Segments;
_   (ReadLEB_u32(&ਅੰਕ_ਅੰਕੜੇ_Segments, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                               m3log (parse, "** Data [%d]", ਅੰਕ_ਅੰਕੜੇ_Segments);

    _throwif("too many data segments", ਅੰਕ_ਅੰਕੜੇ_Segments > d_m3_Max_Sane_ਅੰਕੜੇ_Segments);

    io_module->ਅੰਕੜੇ_Segments = m3_AllocArray(M3_ਅੰਕੜੇ_Segment, ਅੰਕ_ਅੰਕੜੇ_Segments);
    _throwifnull(io_module->ਅੰਕੜੇ_Segments);
    io_module->ਅੰਕ_ਅੰਕੜੇ_Segments = ਅੰਕ_ਅੰਕੜੇ_Segments;

    ਲਈ (u32 i = 0; i < ਅੰਕ_ਅੰਕੜੇ_Segments; ++i) {
        M3_ਅੰਕੜੇ_Segment* segment = &io_module->ਅੰਕੜੇ_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        ਜੇ (flags == 2) {
_           (ReadLEB_u32(&segment->ਸਮ੍ਰਿਤੀ_Region, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        }

        ਜੇ (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#ਜੇ !d_m3_Has_Multi_ਸਮ੍ਰਿਤੀ
            _throwif(m3Err_wasmMalformed, segment->ਸਮ੍ਰਿਤੀ_Region != 0);
#ਜੇ_ਅੰਤ
            _throwif(m3Err_wasmMalformed, segment->ਸਮ੍ਰਿਤੀ_Region >= io_module->ਅੰਕ_Memories);

            segment->init_ਵਿਅੰਜਕ = i_ਬਾਈਟਾਂ;
_           (Parse_Init_ਵਿਅੰਜਕ(io_module, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
            segment->init_ਵਿਅੰਜਕ_ਮਾਪ = (u32)(i_ਬਾਈਟਾਂ - segment->init_ਵਿਅੰਜਕ);

            _throwif(m3_Err_wasm_Missing_Init_ਵਿਅੰਜਕ, segment->init_ਵਿਅੰਜਕ_ਮਾਪ <= 1);
        }

_       (ReadLEB_u32(&segment->ਮਾਪ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        segment->ਅੰਕੜੇ = i_ਬਾਈਟਾਂ;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->ਸਮ੍ਰਿਤੀ_Region, segment->init_ਵਿਅੰਜਕ_ਮਾਪ, segment->ਮਾਪ);
        i_ਬਾਈਟਾਂ += segment->ਮਾਪ;

        _throwif("data segment underflow", i_ਬਾਈਟਾਂ > i_ਸਿਰਾ);
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch:

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Section_ਅੰਕੜੇ_ਗਿਣਤੀ (M3Module* io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

_   (ReadLEB_u32(&io_module->ਅੰਕੜੇ_ਗਿਣਤੀ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                       m3log (parse, "** DataCount [%d]", io_module->ਅੰਕੜੇ_ਗਿਣਤੀ);

    io_module->has_ਅੰਕੜੇ_ਗਿਣਤੀ = true;

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ Parse_Section_ਸਮ੍ਰਿਤੀ (M3Module* io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਅੰਕ_Memories;
_   (ReadLEB_u32(&ਅੰਕ_Memories, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                m3log (parse, "** Memory [%d]", ਅੰਕ_Memories);

#ਜੇ d_m3_Has_Multi_ਸਮ੍ਰਿਤੀ
    _throwif(m3_Err_too_Many_ਸਮ੍ਰਿਤੀ_Sections,
             (u64)ਅੰਕ_Memories + io_module->ਅੰਕ_Memories > d_m3_Max_Sane_Memories_ਗਿਣਤੀ);
#ਨਹੀਂਤਾਂ
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_ਸਮ੍ਰਿਤੀ_Sections, ਅੰਕ_Memories > 1);
    _throwif(m3_Err_too_Many_ਸਮ੍ਰਿਤੀ_Sections, (u64)ਅੰਕ_Memories + io_module->ਅੰਕ_Memories > 1);
#ਜੇ_ਅੰਤ

    ਲਈ (u32 i = 0; i < ਅੰਕ_Memories; ++i) {
        M3_ਸਮ੍ਰਿਤੀ_Info info;
_       (Parse_Type_ਸਮ੍ਰਿਤੀ(&info, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (Module_ਜੋੜੋ_ਸਮ੍ਰਿਤੀ(io_module, ਨਿਰਾ, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ParseSection_Global (M3Module* io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    u32 ਅੰਕ_Globals;
_   (ReadLEB_u32(&ਅੰਕ_Globals, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                    m3log (parse, "** Global [%d]", ਅੰਕ_Globals);

    _throwif("too many globals", ਅੰਕ_Globals > d_m3_Max_Sane_Globals_ਗਿਣਤੀ);

    ਲਈ (u32 i = 0; i < ਅੰਕ_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_ਮੁੱਲ_Type(io_module, &type, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (ReadLEB_u7(&isMutable, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [ਅਧਾਰ_Type_ਦਾ(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_ਜੋੜੋ_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_ਵਿਅੰਜਕ = i_ਬਾਈਟਾਂ;
_       (Parse_Init_ਵਿਅੰਜਕ(io_module, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
        global->init_ਵਿਅੰਜਕ_ਮਾਪ = (u32)(i_ਬਾਈਟਾਂ - global->init_ਵਿਅੰਜਕ);

        _throwif(m3_Err_wasm_Missing_Init_ਵਿਅੰਜਕ, global->init_ਵਿਅੰਜਕ_ਮਾਪ <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_ਬਾਈਟਾਂ != i_ਸਿਰਾ);      // section size mismatch

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ParseSection_Name (M3Module* io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    cstr_t name = ਨਿਰਾ;

    ਜਦੋਂ (i_ਬਾਈਟਾਂ < i_ਸਿਰਾ) {
        u8  nameType;
        u32 payload_ਲੰਮਾਈ;

_       (ReadLEB_u7(&nameType, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_       (ReadLEB_u32(&payload_ਲੰਮਾਈ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

        ਬਾਈਟਾਂ_t ਸ਼ੁਰੂ = i_ਬਾਈਟਾਂ;
        _throwif(m3Err_wasmSectionOverrun, payload_ਲੰਮਾਈ > (size_t)(i_ਸਿਰਾ - ਸ਼ੁਰੂ));

        ਜੇ (nameType == 1) {
            u32 ਅੰਕ_ਨਾਂ;
_           (ReadLEB_u32(&ਅੰਕ_ਨਾਂ, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

            _throwif("too many names", ਅੰਕ_ਨਾਂ > d_m3_Max_Sane_Functions_ਗਿਣਤੀ);

            ਲਈ (u32 i = 0; i < ਅੰਕ_ਨਾਂ; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
_               (Read_utf8(&name, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));

                ਜੇ (index < io_module->ਅੰਕ_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    ਜੇ (func->ਅੰਕ_ਨਾਂ == 0) {
                        func->ਨਾਂ[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->ਅੰਕ_ਨਾਂ = 1;

                        name = ਨਿਰਾ; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_ਬਾਈਟਾਂ = ਸ਼ੁਰੂ + payload_ਲੰਮਾਈ;
    }

    _catch: ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ParseSection_Custom (M3Module* io_module, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, cbytes_t i_ਸਿਰਾ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    cstr_t name = ਨਿਰਾ;
_   (Read_utf8(&name, &i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    ਜੇ (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
#ਜੇ d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } ਨਹੀਂਤਾਂ ਜੇ (strcmp(name, "snapshot") == 0 || (ਲੜੀ_ਲੰਬਾਈ(name) > 9 && ਲੜੀ_ਤੁਲਨਾ(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, ਲੜੀ_ਲੰਬਾਈ(snapName) + 1);
        ਜੇ (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->ਅੰਕੜੇ           = i_ਬਾਈਟਾਂ;
        snap->ਮਾਪ           = (u32)(i_ਸਿਰਾ - i_ਬਾਈਟਾਂ);
        snap->ਅਗਲਾ           = io_module->snapshots;
        io_module->snapshots = snap;
#ਜੇ_ਅੰਤ
    } ਨਹੀਂਤਾਂ ਜੇ (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_ਬਾਈਟਾਂ, i_ਸਿਰਾ));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ ParseModuleSection (M3Module* o_module, u8 i_sectionType, ਬਾਈਟਾਂ_t i_ਬਾਈਟਾਂ, u32 i_ਅੰਕ_ਬਾਈਟਾਂ)
{
    M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;

    ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਨਤੀਜਾ (*M3Parser)(M3Module*, ਬਾਈਟਾਂ_t, cbytes_t);

    ਸਥਿਰ M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_ਸਾਰਣੀ,        // 4
        Parse_Section_ਸਮ੍ਰਿਤੀ,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_ਸ਼ੁਰੂ,     // 8
        ParseSection_Element,   // 9
        Parse_Section_ਕੋਡ,      // 10
        Parse_Section_ਅੰਕੜੇ,      // 11
        Parse_Section_ਅੰਕੜੇ_ਗਿਣਤੀ, // 12
#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
        Parse_Section_ਟੈਗ,       // 13
#ਜੇ_ਅੰਤ
    };

    M3Parser parser = ਨਿਰਾ;

    ਜੇ (i_sectionType < M3_ਗਿਣਤੀ_ਦਾ(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    ਜੇ (parser) {
        cbytes_t ਸਿਰਾ = i_ਬਾਈਟਾਂ + i_ਅੰਕ_ਬਾਈਟਾਂ;
        ਨਤੀਜਾ       = parser(o_module, i_ਬਾਈਟਾਂ, ਸਿਰਾ);
    } ਨਹੀਂਤਾਂ {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    ਵਾਪਸ ਨਤੀਜਾ;
}


M3_ਨਤੀਜਾ m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_ਬਾਈਟਾਂ, u32 i_ਅੰਕ_ਬਾਈਟਾਂ)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_ਅੰਕ_ਬਾਈਟਾਂ);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_ਅੰਕ_ਬਾਈਟਾਂ);
    module->ਸ਼ੁਰੂ_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ਅਚਲ u8* pos = i_ਬਾਈਟਾਂ;
    ਅਚਲ u8* ਸਿਰਾ = pos + i_ਅੰਕ_ਬਾਈਟਾਂ;

    module->wasm_ਸ਼ੁਰੂ = pos;
    module->wasm_ਸਿਰਾ   = ਸਿਰਾ;

    u32 magic, ਸੰਸਕਰਣ;
_   (Read_u32(&magic, &pos, ਸਿਰਾ));
_   (Read_u32(&ਸੰਸਕਰਣ, &pos, ਸਿਰਾ));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_ਸੰਸਕਰਣ, ਸੰਸਕਰਣ != 1);

#ਜੇ d_m3HasExceptionHandling || d_m3_Has_ਢੇਰ_Switching
    // the tag section sits between memory (5) and global (6)
    ਸਥਿਰ ਅਚਲ u8 sections_ਕ੍ਰਮ[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#ਨਹੀਂਤਾਂ
    ਸਥਿਰ ਅਚਲ u8 sections_ਕ੍ਰਮ[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#ਜੇ_ਅੰਤ
    u8 expectedSection = 0;

    ਜਦੋਂ (pos < ਸਿਰਾ) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, ਸਿਰਾ));

        ਜੇ (section != 0) {
            // Ensure sections appear only once and in order
            ਜਦੋਂ (sections_ਕ੍ਰਮ[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_ਗਿਣਤੀ_ਦਾ(sections_ਕ੍ਰਮ) - 1);
            }
        }

        u32 section_ਲੰਮਾਈ;
_       (ReadLEB_u32(&section_ਲੰਮਾਈ, &pos, ਸਿਰਾ));
        _throwif(m3Err_wasmMalformed, pos + section_ਲੰਮਾਈ > ਸਿਰਾ);

_       (ParseModuleSection(module, section, pos, section_ਲੰਮਾਈ));

        pos += section_ਲੰਮਾਈ;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    ਜੇ (module->ਅੰਕ_Functions > module->ਅੰਕ_Func_Imports) {
        IM3Function ਪਹਿਲਾ_Non_Import = &module->functions[module->ਅੰਕ_Func_Imports];
        _throwif(m3Err_wasmMalformed, ਪਹਿਲਾ_Non_Import->wasm == ਨਿਰਾ);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_ਅੰਕੜੇ_ਗਿਣਤੀ and module->ਅੰਕੜੇ_ਗਿਣਤੀ != module->ਅੰਕ_ਅੰਕੜੇ_Segments);

} _catch:

    ਜੇ (ਨਤੀਜਾ) {
        m3_FreeModule(module);
        module = ਨਿਰਾ;
    }

    *o_module = module;

    ਵਾਪਸ ਨਤੀਜਾ;
}
