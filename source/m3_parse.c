//
//  m3_parse.c
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"


// elem type + limits, shared by the table section and by table imports.
// The declared minimum is the table's initial size; table.grow can raise it up
// to the maximum, so an element segment reaching past the current size is out
// of bounds.
#ಆದರೆ d_m3HasTypedRefs

// Reads a heap type: 'func', 'extern', or the index of a function type the
// module has already defined. Yields the heap-type bits of an m3type_t, which
// carry the canonical index rather than the module's own, so that structurally
// equal types compare equal across modules.
M3_ಫಲಿತಾಂಶ Parse_ರಾಶಿ_Type (IM3Module i_module, m3type_t* o_ರಾಶಿ_ಬಿಟ್ಟುಗಳು, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    i64 ರಾಶಿ_2;
_   (ReadLebSigned(&ರಾಶಿ_2, 33, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

    ಆದರೆ (ರಾಶಿ_2 == -d_waType_funcref) {
        *o_ರಾಶಿ_ಬಿಟ್ಟುಗಳು = d_m3_Type_ರಾಶಿ_Abstract;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ರಾಶಿ_2 == -d_waType_externref) {
        *o_ರಾಶಿ_ಬಿಟ್ಟುಗಳು = d_m3Type_refExtern | d_m3_Type_ರಾಶಿ_Abstract;
#  ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ರಾಶಿ_2 == -d_wa_Type_ರಾಶಿ_cont) {
        *o_ರಾಶಿ_ಬಿಟ್ಟುಗಳು = d_m3Type_refCont | d_m3_Type_ರಾಶಿ_Abstract;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ರಾಶಿ_2 == -d_wa_Type_ರಾಶಿ_nocont) {
        *o_ರಾಶಿ_ಬಿಟ್ಟುಗಳು = d_m3Type_refCont | d_m3_Type_ರಾಶಿ_None;
#  endif
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ರಾಶಿ_2 >= 0) {
        _throwif(m3Err_wasmMalformed, not i_module or (u64) ರಾಶಿ_2 >= i_module->ಅಂಕೆ_Func_Types);

        IM3FuncType ftype = i_module->funcTypes[ರಾಶಿ_2];

        // null while the type section is still being read: a type may only name
        // one that precedes it, recursion belongs to a later proposal
        _throwif(m3Err_wasmMalformed, not ftype);

        *o_ರಾಶಿ_ಬಿಟ್ಟುಗಳು = (ftype->isContinuation ? d_m3Type_refCont : 0) | ftype->canonical_ಸೂಚ್ಯಂಕ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        _throw(m3Err_invalidTypeId);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasTypedRefs


// Reads a value type, including the reference types the function references
// proposal spells out in full: (ref ht) and (ref null ht).
M3_ಫಲಿತಾಂಶ Parse_ಮೌಲ್ಯ_Type (IM3Module i_module, m3type_t* o_type, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    _throwif(m3Err_wasmUnderrun, *io_ಬೈಟುಗಳು >= i_ಕೊನೆ);

#ಆದರೆ d_m3HasTypedRefs
    ಆದರೆ (**io_ಬೈಟುಗಳು == d_waEncode_ref or **io_ಬೈಟುಗಳು == d_waEncode_refNull) {
        u8 lead = *(*io_ಬೈಟುಗಳು)++;

        m3type_t ರಾಶಿ_ಬಿಟ್ಟುಗಳು = d_m3_Type_ರಾಶಿ_Abstract;
_       (Parse_ರಾಶಿ_Type(i_module, &ರಾಶಿ_ಬಿಟ್ಟುಗಳು, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

        *o_type = d_m3Type_ref | ರಾಶಿ_ಬಿಟ್ಟುಗಳು | ((lead == d_waEncode_ref) ? d_m3Type_refNonNull : 0);

        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }
#ಆದರೆ_ಅಂತ್ಯ

    i8 wasmType;
    u8 plainType;
_   (ReadLEB_i7(&wasmType, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
_   (NormalizeType(&plainType, wasmType));

    // 0x40 is the empty block type, not a value type. NormalizeType has to
    // accept it for the block forms, so every valtype position turns it away here.
    _throwif(m3Err_invalidTypeId, plainType == c_m3Type_none);

    *o_type = plainType;

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// Reads an elem type and limits without registering anything: an import
// descriptor holds on to the fields and only adds the table once it has a name.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Read_Type_ಕೋಷ್ಟಕ_Type (IM3Module i_module, M3_ಕೋಷ್ಟಕ_Info* o_info, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    m3type_t elemType = c_m3Type_none;
    u8       flag     = 0;
    u64      init_ಗಾತ್ರ = 0, max_ಗಾತ್ರ = 0;

    // the width of the limits, and of every index into the table
    u32 addr_ಬಿಟ್ಟುಗಳು = 32;

_   (Parse_ಮೌಲ್ಯ_Type(i_module, &elemType, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
#ಆದರೆ d_m3HasRefTypes
    _throwif(m3Err_wasmMalformed, not IsRefType(ತಳ_Type_ದ(elemType)));
#ಇಲ್ಲದಿದ್ದರೆ
    _throwif(m3Err_wasmMalformed, elemType != c_m3Type_funcref);
#ಆದರೆ_ಅಂತ್ಯ

_   (ReadLEB_u7(&flag, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

    // bit 0: has max, bit 2: table64 - the same flag memory64 gives a memory
#ಆದರೆ d_m3HasMemory64
    _throwif(m3Err_wasmMalformed, flag & ~0x05u);
#ಇಲ್ಲದಿದ್ದರೆ
    _throwif(m3Err_wasmMalformed, flag & ~0x01u);
#ಆದರೆ_ಅಂತ್ಯ

    ಆದರೆ (flag & 0x04u) {
        addr_ಬಿಟ್ಟುಗಳು = 64;
    }

_   (ReadLebUnsigned(&init_ಗಾತ್ರ, addr_ಬಿಟ್ಟುಗಳು, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
    _throwif("table overflow", init_ಗಾತ್ರ > d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಗಾತ್ರ);

    ಆದರೆ (flag & 0x01u) {
_       (ReadLebUnsigned(&max_ಗಾತ್ರ, addr_ಬಿಟ್ಟುಗಳು, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
        _throwif(m3Err_wasmMalformed, max_ಗಾತ್ರ < init_ಗಾತ್ರ);
        _throwif("table overflow", max_ಗಾತ್ರ > d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಗಾತ್ರ);
    }

    _catch:

    o_info->elemType  = elemType;
    o_info->init_ಗಾತ್ರ  = (u32)init_ಗಾತ್ರ;
    o_info->max_ಗಾತ್ರ   = (u32)max_ಗಾತ್ರ;
    o_info->hasMax    = (flag & 0x01u) != 0;
    o_info->isTable64 = (flag & 0x04u) != 0;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Parse_Type_ಕೋಷ್ಟಕ_Type (IM3Module io_module, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3_ಕೋಷ್ಟಕ_Info info;

_   (Read_Type_ಕೋಷ್ಟಕ_Type(io_module, &info, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
_   (Module_ಕೂಡಿಸು_ಕೋಷ್ಟಕ(io_module, ಖಾಲಿ, &info, false /* isImport */));

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Init_ಅಭಿವ್ಯಕ್ತಿ_Typed (M3Module* io_module, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ, m3type_t* o_type);

M3_ಫಲಿತಾಂಶ Parse_Type_ಕೋಷ್ಟಕ (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಅಂಕೆ_Tables;
_   (ReadLEB_u32(&ಅಂಕೆ_Tables, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                          m3log (parse, "** Table [%d]", ಅಂಕೆ_Tables);

    _throwif("too many tables", (u64)ಅಂಕೆ_Tables + io_module->ಅಂಕೆ_Tables > d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಎಣಿಕೆ);

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Tables; ++i) {
#ಆದರೆ d_m3HasTypedRefs
        // function-references: 0x40 0x00 introduces a table with an explicit
        // initializer. A reference type never starts with 0x40, so the two
        // forms are told apart by the first byte; the second is reserved.
        bool has_Init_ಅಭಿವ್ಯಕ್ತಿ = (i_ಬೈಟುಗಳು < i_ಕೊನೆ) and (*i_ಬೈಟುಗಳು == 0x40);
        u32  ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ  = io_module->ಅಂಕೆ_Tables;

        ಆದರೆ (has_Init_ಅಭಿವ್ಯಕ್ತಿ) {
            u8 reserved;
            ++i_ಬೈಟುಗಳು;
_           (Read_u8(&reserved, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
            _throwif(m3Err_wasmMalformed, reserved != 0x00);
        }

_       (Parse_Type_ಕೋಷ್ಟಕ_Type(io_module, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

        ಆದರೆ (has_Init_ಅಭಿವ್ಯಕ್ತಿ) {
            IM3_ಕೋಷ್ಟಕ ಕೋಷ್ಟಕ_2 = io_module->tables[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ];

            m3type_t initType;

            ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ = i_ಬೈಟುಗಳು;
_           (Parse_Init_ಅಭಿವ್ಯಕ್ತಿ_Typed(io_module, &i_ಬೈಟುಗಳು, i_ಕೊನೆ, &initType));
            ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ = (u32)(i_ಬೈಟುಗಳು - ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ);
            _throwif(m3_Err_wasm_Missing_Init_ಅಭಿವ್ಯಕ್ತಿ, ಕೋಷ್ಟಕ_2->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ <= 1);

            _throwif(m3Err_typeMismatch, not Is_Sub_Type_ದ(initType, ಕೋಷ್ಟಕ_2->type));
        } ಇಲ್ಲದಿದ್ದರೆ {
            // no initializer means null, which only a nullable type can hold
            _throwif("table of non-nullable type requires an initializer",
                     not IsNullableRef(io_module->tables[ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ]->type));
        }
#ಇಲ್ಲದಿದ್ದರೆ
_       (Parse_Type_ಕೋಷ್ಟಕ_Type(io_module, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
#ಆದರೆ_ಅಂತ್ಯ
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Type_ಸ್ಮರಣೆ (M3_ಸ್ಮರಣೆ_Info* o_ಸ್ಮರಣೆ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u8 flag;

    // The custom page sizes proposal admits only 1 and 65536, but nothing in
    // the engine cares which power of two it is, so any of them is accepted.
    // Declared up here so the throws below don't jump over its initialization.
    u32 log_ಪುಟ_ಗಾತ್ರ = 16;

    // The width of the limits, and of everything that addresses the memory:
    // 32 unless bit 2 says memory64.
    u32 addr_ಬಿಟ್ಟುಗಳು = 32;

_   (ReadLEB_u7(&flag, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

    // bit 0: has max, bit 1: shared, bit 2: memory64, bit 3: custom page size
    {
        u32 ತಿಳಿದ = 0x09u;
#ಆದರೆ d_m3HasAtomics
        ತಿಳಿದ |= 0x02u;
#ಆದರೆ_ಅಂತ್ಯ
#ಆದರೆ d_m3HasMemory64
        ತಿಳಿದ |= 0x04u;
#ಆದರೆ_ಅಂತ್ಯ
        _throwif(m3Err_wasmMalformed, flag & ~ತಿಳಿದ);
    }

    o_ಸ್ಮರಣೆ->isShared = (flag & (1u << 1)) != 0;

    o_ಸ್ಮರಣೆ->isMemory64 = (flag & (1u << 2)) != 0;
    ಆದರೆ (o_ಸ್ಮರಣೆ->isMemory64) {
        addr_ಬಿಟ್ಟುಗಳು = 64;
    }

_   (ReadLebUnsigned(&o_ಸ್ಮರಣೆ->init_ಪುಟಗಳು, addr_ಬಿಟ್ಟುಗಳು, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

    o_ಸ್ಮರಣೆ->max_ಪುಟಗಳು = 0;
    o_ಸ್ಮರಣೆ->hasMax   = (flag & (1u << 0)) != 0;
    ಆದರೆ (o_ಸ್ಮರಣೆ->hasMax) {
_       (ReadLebUnsigned(&o_ಸ್ಮರಣೆ->max_ಪುಟಗಳು, addr_ಬಿಟ್ಟುಗಳು, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

        // Spec: memory limits validation - max must not be less than init
        _throwif(m3Err_wasmMalformed, o_ಸ್ಮರಣೆ->max_ಪುಟಗಳು < o_ಸ್ಮರಣೆ->init_ಪುಟಗಳು);
    }

    _throwif("shared memory must have maximum", o_ಸ್ಮರಣೆ->isShared and not o_ಸ್ಮರಣೆ->hasMax);

    o_ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ = 0;
    ಆದರೆ (flag & (1u << 3)) {
_       (ReadLEB_u32(&log_ಪುಟ_ಗಾತ್ರ, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

        _throwif("invalid custom page size", log_ಪುಟ_ಗಾತ್ರ > 16);

        o_ಸ್ಮರಣೆ->ಪುಟ_ಗಾತ್ರ = 1u << log_ಪುಟ_ಗಾತ್ರ;
    }

    // Spec: memory limits must be valid within range 2^|addrtype|/pagesize.
    // That is 65536 pages at the default page size, 2^48 for a 64-bit memory,
    // and a whole address space of them when a page is a single byte.
    {
        u64 max_ಪುಟಗಳು_Allowed = (log_ಪುಟ_ಗಾತ್ರ or addr_ಬಿಟ್ಟುಗಳು < 64)
                                ? (UINT64_C(1) << (addr_ಬಿಟ್ಟುಗಳು - log_ಪುಟ_ಗಾತ್ರ))
                                : UINT64_MAX;

        _throwif(m3Err_wasmMalformed, o_ಸ್ಮರಣೆ->init_ಪುಟಗಳು > max_ಪುಟಗಳು_Allowed);
        ಆದರೆ (o_ಸ್ಮರಣೆ->hasMax) {
            _throwif(m3Err_wasmMalformed, o_ಸ್ಮರಣೆ->max_ಪುಟಗಳು > max_ಪುಟಗಳು_Allowed);
        }
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// One func type: [numArgs] argtypes [numRets] rettypes, with the results
// stored ahead of the arguments the way M3FuncType wants them.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ParseFuncType (IM3Module io_module, IM3FuncType* o_type, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    IM3FuncType ftype = ಖಾಲಿ;
_try {
    u32 ಅಂಕೆ_ವಾದಗಳು;
_   (ReadLEB_u32(&ಅಂಕೆ_ವಾದಗಳು, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

    _throwif(m3_Err_too_Many_ವಾದಗಳು_Rets, ಅಂಕೆ_ವಾದಗಳು > d_m3_Max_Sane_Function_Arg_Ret_ಎಣಿಕೆ);
#ಆದರೆ M3_HAS_VLA
    m3type_t argTypes[ಅಂಕೆ_ವಾದಗಳು + 1]; // make ubsan happy
#ಇಲ್ಲದಿದ್ದರೆ
    m3type_t argTypes[d_m3_Max_Sane_Function_Arg_Ret_ಎಣಿಕೆ];
#ಆದರೆ_ಅಂತ್ಯ
    ಗಾಗಿ (u32 a = 0; a < ಅಂಕೆ_ವಾದಗಳು; ++a) {
_       (Parse_ಮೌಲ್ಯ_Type(io_module, &argTypes[a], io_ಬೈಟುಗಳು, i_ಕೊನೆ));
    }

    u32 ಅಂಕೆ_Rets;
_   (ReadLEB_u32(&ಅಂಕೆ_Rets, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
    _throwif(m3_Err_too_Many_ವಾದಗಳು_Rets, (u64)(ಅಂಕೆ_Rets) + ಅಂಕೆ_ವಾದಗಳು > d_m3_Max_Sane_Function_Arg_Ret_ಎಣಿಕೆ);

_   (AllocFuncType(&ftype, ಅಂಕೆ_Rets + ಅಂಕೆ_ವಾದಗಳು));
    ftype->ಅಂಕೆ_ವಾದಗಳು = ಅಂಕೆ_ವಾದಗಳು;
    ftype->ಅಂಕೆ_Rets = ಅಂಕೆ_Rets;

    ಗಾಗಿ (u32 r = 0; r < ಅಂಕೆ_Rets; ++r) {
_       (Parse_ಮೌಲ್ಯ_Type(io_module, &ftype->types[r], io_ಬೈಟುಗಳು, i_ಕೊನೆ));
    }
    ಸ್ಮರಣೆ_ನಕಲು(ftype->types + ಅಂಕೆ_Rets, argTypes, ಅಂಕೆ_ವಾದಗಳು * ಗಾತ್ರ(m3type_t));

    *o_type = ftype;
    ftype   = ಖಾಲಿ;

} _catch:
    m3_Free(ftype);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

// cont $ft. A continuation carries the shape of the function it runs, so that
// resume and the rest can read the parameters and results straight off it.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ParseContType (IM3Module io_module, IM3FuncType* o_type, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    IM3FuncType ftype = ಖಾಲಿ;
_try {
    i64 ft_ಸೂಚ್ಯಂಕ;
_   (ReadLebSigned(&ft_ಸೂಚ್ಯಂಕ, 33, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
    _throwif(m3Err_wasmMalformed, ft_ಸೂಚ್ಯಂಕ < 0 or (u64) ft_ಸೂಚ್ಯಂಕ >= io_module->ಅಂಕೆ_Func_Types);

    IM3FuncType underlying = io_module->funcTypes[ft_ಸೂಚ್ಯಂಕ];
    _throwif(m3Err_wasmMalformed, not underlying or underlying->isContinuation);

    u32 ಒಟ್ಟು = underlying->ಅಂಕೆ_Rets + underlying->ಅಂಕೆ_ವಾದಗಳು;
_   (AllocFuncType(&ftype, ಒಟ್ಟು));
    ftype->ಅಂಕೆ_Rets        = underlying->ಅಂಕೆ_Rets;
    ftype->ಅಂಕೆ_ವಾದಗಳು        = underlying->ಅಂಕೆ_ವಾದಗಳು;
    ftype->isContinuation = true;
    ftype->contFuncType   = underlying;

    ಆದರೆ (ಒಟ್ಟು) {
        ಸ್ಮರಣೆ_ನಕಲು(ftype->types, underlying->types, ಒಟ್ಟು * ಗಾತ್ರ(m3type_t));
    }

    *o_type = ftype;
    ftype   = ಖಾಲಿ;

} _catch:
    m3_Free(ftype);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ParseTypeDefinition (IM3Module io_module, i8 i_form, IM3FuncType* o_type,
                              ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
    ಆದರೆ (i_form == -35) {
        ಹಿಂತಿರುಗಿ ParseContType(io_module, o_type, io_ಬೈಟುಗಳು, i_ಕೊನೆ);
    }
#ಆದರೆ_ಅಂತ್ಯ
    ಆದರೆ (i_form != -32) { // for Wasm MVP
        ಹಿಂತಿರುಗಿ m3Err_wasmMalformed;
    }

    ಹಿಂತಿರುಗಿ ParseFuncType(io_module, o_type, io_ಬೈಟುಗಳು, i_ಕೊನೆ);
}


#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching

// Walks one type definition without resolving anything, which is how the first
// pass over a recursive group learns what each member is before any of them can
// be built: the members are free to name each other, so nothing can be resolved
// until every index in the group is spoken for.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ SkipTypeDefinition (i8 i_form, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
_try {
    ಆದರೆ (i_form == -35) {
        i64 ft_ಸೂಚ್ಯಂಕ;
_       (ReadLebSigned(&ft_ಸೂಚ್ಯಂಕ, 33, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    _throwif(m3Err_wasmMalformed, i_form != -32);

    ಗಾಗಿ (u32 half = 0; half < 2; ++half) {
        u32 ಎಣಿಕೆ_2;
_       (ReadLEB_u32(&ಎಣಿಕೆ_2, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
        _throwif(m3_Err_too_Many_ವಾದಗಳು_Rets, ಎಣಿಕೆ_2 > d_m3_Max_Sane_Function_Arg_Ret_ಎಣಿಕೆ);

        ಗಾಗಿ (u32 v = 0; v < ಎಣಿಕೆ_2; ++v) {
            u8 encoding;
_           (Read_u8(&encoding, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

            ಆದರೆ (encoding == d_waEncode_ref or encoding == d_waEncode_refNull) {
                i64 ರಾಶಿ_2;
_               (ReadLebSigned(&ರಾಶಿ_2, 33, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
            }
        }
    }

} _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// A recursive type group. Its members may name each other, in either order, so
// the indices they will occupy have to exist before any body is read: the group
// is scanned once for shapes, its indices are reserved, and only then are the
// definitions built - func types first, because a cont type is spelled in terms
// of one.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Parse_Rec_ಗುಂಪು (IM3Module io_module, u32 i_ಎಣಿಕೆ, u32 i_ಮೊದಲ_ಸೂಚ್ಯಂಕ,
                        ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    IM3FuncType* placeholders = ಖಾಲಿ;
_try {
    ಬೈಟುಗಳು_t ಗುಂಪು_ಆರಂಭ = *io_ಬೈಟುಗಳು;

    placeholders = m3_AllocArray(IM3FuncType, i_ಎಣಿಕೆ);
    _throwifnull(placeholders);

    // pass one: what each member is, without looking at what it refers to
    ಗಾಗಿ (u32 j = 0; j < i_ಎಣಿಕೆ; ++j) {
        i8 form;
_       (ReadLEB_i7(&form, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (SkipTypeDefinition(form, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

_       (AllocFuncType(&placeholders[j], 0));
        placeholders[j]->isContinuation = (form == -35);
    }

    u16 ಮೊದಲ_Canonical;
_   (Environment_ReserveFuncTypes(io_module->environment, i_ಎಣಿಕೆ, &ಮೊದಲ_Canonical));

    ಗಾಗಿ (u32 j = 0; j < i_ಎಣಿಕೆ; ++j) {
        placeholders[j]->canonical_ಸೂಚ್ಯಂಕ        = (u16)(ಮೊದಲ_Canonical + j);
        io_module->funcTypes[i_ಮೊದಲ_ಸೂಚ್ಯಂಕ + j] = placeholders[j];
    }

    // pass two: the definitions themselves. A cont type copies the shape of the
    // func type it names, so every func type in the group has to be standing
    // before any cont type in it is read.
    ಗಾಗಿ (u32 pass = 0; pass < 2; ++pass) {
        ಬೈಟುಗಳು_t ಬೈಟುಗಳು = ಗುಂಪು_ಆರಂಭ;

        ಗಾಗಿ (u32 j = 0; j < i_ಎಣಿಕೆ; ++j) {
            i8 form;
_           (ReadLEB_i7(&form, &ಬೈಟುಗಳು, i_ಕೊನೆ));

            ಆದರೆ ((form == -35) != (pass == 1)) {
_               (SkipTypeDefinition(form, &ಬೈಟುಗಳು, i_ಕೊನೆ));
                ಮುಂದುವರಿ;
            }

            IM3FuncType ftype = ಖಾಲಿ;
_           (ParseTypeDefinition(io_module, form, &ftype, &ಬೈಟುಗಳು, i_ಕೊನೆ));

            Environment_AdoptFuncType(io_module->environment, ftype, placeholders[j]->canonical_ಸೂಚ್ಯಂಕ);

            m3_Free(placeholders[j]);
            placeholders[j]                        = ಖಾಲಿ;
            io_module->funcTypes[i_ಮೊದಲ_ಸೂಚ್ಯಂಕ + j] = ftype;
        }

        ಆದರೆ (pass == 1) {
            *io_ಬೈಟುಗಳು = ಬೈಟುಗಳು;
        }
    }

} _catch:
    ಆದರೆ (placeholders) {
        ಗಾಗಿ (u32 j = 0; j < i_ಎಣಿಕೆ; ++j) {
            m3_Free(placeholders[j]);
        }
        m3_Free(placeholders);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasStackSwitching


M3_ಫಲಿತಾಂಶ ParseSection_Type (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    IM3FuncType ftype = ಖಾಲಿ;

_try {
    u32 ಅಂಕೆ_Types;
_   (ReadLEB_u32(&ಅಂಕೆ_Types, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                      m3log (parse, "** Type [%d]", ಅಂಕೆ_Types);

    _throwif("too many types", ಅಂಕೆ_Types > d_m3_Max_Sane_Types_ಎಣಿಕೆ);

    ಆದರೆ (ಅಂಕೆ_Types) {
        // table of IM3FuncType (that point to the actual M3FuncType struct in the Environment)
        io_module->funcTypes = m3_AllocArray(IM3FuncType, ಅಂಕೆ_Types);
        _throwifnull(io_module->funcTypes);
        io_module->ಅಂಕೆ_Func_Types = ಅಂಕೆ_Types;

        // a recursive group is one entry of the section but any number of type
        // indices, so the two counts part ways as soon as one appears
#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
        u32 ಸಾಮರ್ಥ್ಯ = ಅಂಕೆ_Types;
#ಆದರೆ_ಅಂತ್ಯ
        u32 index = 0;

        ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Types; ++i) {
            i8 form;
_           (ReadLEB_i7(&form, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

#ಆದರೆ d_m3_Has_ರಾಶಿ_Switching
            ಆದರೆ (form == -50) { // rec
                u32 ಎಣಿಕೆ_2;
_               (ReadLEB_u32(&ಎಣಿಕೆ_2, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

                // room for this group, and for one more entry per section
                // entry still to come
                u64 needed = (u64)index + ಎಣಿಕೆ_2 + (ಅಂಕೆ_Types - 1 - i);

                _throwif("too many types", needed > d_m3_Max_Sane_Types_ಎಣಿಕೆ);

                ಆದರೆ (needed > ಸಾಮರ್ಥ್ಯ) {
                    u32 grown = (u32)needed;

                    IM3FuncType* types =
                      m3_ReallocArray(IM3FuncType, io_module->funcTypes, grown, ಸಾಮರ್ಥ್ಯ);
                    _throwifnull(types);

                    io_module->funcTypes    = types;
                    ಸಾಮರ್ಥ್ಯ                = grown;
                    io_module->ಅಂಕೆ_Func_Types = grown;
                }

                // a group of one says nothing a plain definition does not, and
                // has to be able to share an entry with one
                ಆದರೆ (ಎಣಿಕೆ_2 == 1) {
_                   (ReadLEB_i7(&form, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
                } ಇಲ್ಲದಿದ್ದರೆ {
_                   (Parse_Rec_ಗುಂಪು(io_module, ಎಣಿಕೆ_2, index, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
                    index += ಎಣಿಕೆ_2;
                    ಮುಂದುವರಿ;
                }
            }
#ಆದರೆ_ಅಂತ್ಯ

_           (ParseTypeDefinition(io_module, form, &ftype, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));       m3log (parse, "    type %2d: %s", index, SPrintFuncTypeSignature (ftype));

            Environment_ಕೂಡಿಸು_Func_Type(io_module->environment, &ftype);
            io_module->funcTypes[index++] = ftype;
            ftype                         = ಖಾಲಿ; // ownership transferred to environment
        }

        io_module->ಅಂಕೆ_Func_Types = index;
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

} _catch:

    ಆದರೆ (ಫಲಿತಾಂಶ) {
        m3_Free(ftype);
        // FIX: M3FuncTypes in the table are leaked
        m3_Free(io_module->funcTypes);
        io_module->ಅಂಕೆ_Func_Types = 0;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ParseSection_Function (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಅಂಕೆ_Functions;
_   (ReadLEB_u32(&ಅಂಕೆ_Functions, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                  m3log (parse, "** Function [%d]", ಅಂಕೆ_Functions);

    _throwif("too many functions", ಅಂಕೆ_Functions > d_m3_Max_Sane_Functions_ಎಣಿಕೆ);

_   (Module_PreallocFunctions(io_module, io_module->ಅಂಕೆ_Functions + ಅಂಕೆ_Functions));

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Functions; ++i) {
        u32 func_Type_ಸೂಚ್ಯಂಕ;
_       (ReadLEB_u32(&func_Type_ಸೂಚ್ಯಂಕ, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

_       (Module_ಕೂಡಿಸು_Function(io_module, func_Type_ಸೂಚ್ಯಂಕ, ಖಾಲಿ /* import info */));
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// One externtype, read but not yet registered. The compact encoding that shares
// a type across a run needs the two halves apart: the type sits ahead of the
// item names, so it is read once and then registered for each of them.
ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3ImportDesc {
    u8           ಬಗೆ;
    u32          type_ಸೂಚ್ಯಂಕ;                  // function
    m3type_t     type;                       // global type
    u8           isMutable;                  // global
    M3_ಕೋಷ್ಟಕ_Info  ಕೋಷ್ಟಕ_2;
    M3_ಸ್ಮರಣೆ_Info ಸ್ಮರಣೆ;
} M3ImportDesc;


#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching

// A tag_type: a reserved attribute byte that must be 0 (the exception
// attribute) and the index of the function type giving the payload. The
// proposal reserves the byte so later kinds of tag can share the encoding.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ Read_ಟ್ಯಾಗ್_Type (IM3Module i_module, u32* o_type_ಸೂಚ್ಯಂಕ, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u8 attribute;
_   (Read_u8(&attribute, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
    _throwif(m3Err_wasmMalformed, attribute != 0);

_   (ReadLEB_u32(o_type_ಸೂಚ್ಯಂಕ, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
    _throwif(m3Err_unknownType, not i_module or *o_type_ಸೂಚ್ಯಂಕ >= i_module->ಅಂಕೆ_Func_Types);

#  ಆದರೆ !d_m3_Has_ರಾಶಿ_Switching
    // an exception tag's type describes its payload, so it yields nothing
    _throwif(m3Err_wasmMalformed, Get_Func_Type_ಅಂಕೆ_Results(i_module->funcTypes[*o_type_ಸೂಚ್ಯಂಕ]) != 0);
#  endif

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling || d_m3HasStackSwitching


// Reads and validates one externtype. Mutates nothing in the module, so an
// externtype is checked even when the compact run that shares it is empty.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ReadImportDesc (IM3Module i_module, M3ImportDesc* o_desc, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (Read_u8(&o_desc->ಬಗೆ, io_ಬೈಟುಗಳು, i_ಕೊನೆ));

    ಆಯ್ಕೆ (o_desc->ಬಗೆ) {
    ಸ್ಥಿತಿ d_external_ಬಗೆ_function:
_       (ReadLEB_u32(&o_desc->type_ಸೂಚ್ಯಂಕ, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_external_ಬಗೆ_ಕೋಷ್ಟಕ:
_       (Read_Type_ಕೋಷ್ಟಕ_Type(i_module, &o_desc->ಕೋಷ್ಟಕ_2, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_external_ಬಗೆ_ಸ್ಮರಣೆ:
_       (Parse_Type_ಸ್ಮರಣೆ(&o_desc->ಸ್ಮರಣೆ, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
        ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_external_ಬಗೆ_global:
_       (Parse_ಮೌಲ್ಯ_Type(i_module, &o_desc->type, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (ReadLEB_u7(&o_desc->isMutable, io_ಬೈಟುಗಳು, i_ಕೊನೆ));                          m3log (parse, "     global: %s mutable=%d", c_waTypes [ತಳ_Type_ದ(o_desc->type)], (u32) o_desc->isMutable);
        _throwif(m3Err_wasmMalformed, o_desc->isMutable > 1);
        ನಿಲ್ಲಿಸು;

#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    ಸ್ಥಿತಿ d_external_ಬಗೆ_ಟ್ಯಾಗ್:
_       (Read_ಟ್ಯಾಗ್_Type(i_module, &o_desc->type_ಸೂಚ್ಯಂಕ, io_ಬೈಟುಗಳು, i_ಕೊನೆ));
        ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ

    ಪೂರ್ವನಿಯೋಜಿತ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// Registers the import that i_desc describes. Takes ownership of *io_import
// wherever the module keeps the strings, clearing the struct so the caller's
// FreeImportInfo() doesn't free what was handed over.
ಸ್ಥಿರ
M3_ಫಲಿತಾಂಶ ApplyImportDesc (IM3Module io_module, ಸ್ಥಿರಾಂಕ M3ImportDesc* i_desc, M3ImportInfo* io_import)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3ImportInfo clearImport = { ಖಾಲಿ, ಖಾಲಿ };                                      m3log (parse, "    kind: %d '%s.%s' ",
                                                                                            (u32) i_desc->ಬಗೆ, io_import->moduleUtf8, io_import->ಕ್ಷೇತ್ರ_Utf8);
    ಆಯ್ಕೆ (i_desc->ಬಗೆ) {
    ಸ್ಥಿತಿ d_external_ಬಗೆ_function: {
_       (Module_ಕೂಡಿಸು_Function(io_module, i_desc->type_ಸೂಚ್ಯಂಕ, io_import))
        *io_import = clearImport;

        io_module->ಅಂಕೆ_Func_Imports++;
    } ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_external_ಬಗೆ_ಕೋಷ್ಟಕ: {
        IM3_ಕೋಷ್ಟಕ ಕೋಷ್ಟಕ_2;
_       (Module_ಕೂಡಿಸು_ಕೋಷ್ಟಕ(io_module, &ಕೋಷ್ಟಕ_2, &i_desc->ಕೋಷ್ಟಕ_2, true /* isImport */));
        ಕೋಷ್ಟಕ_2->import = *io_import;
        *io_import    = clearImport;
    } ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_external_ಬಗೆ_ಸ್ಮರಣೆ: {
        IM3_ಸ್ಮರಣೆ ಸ್ಮರಣೆ;

#ಆದರೆ d_m3_Has_Multi_ಸ್ಮರಣೆ
        _throwif(m3_Err_too_Many_ಸ್ಮರಣೆ_Sections, io_module->ಅಂಕೆ_Memories >= d_m3_Max_Sane_Memories_ಎಣಿಕೆ);
#ಇಲ್ಲದಿದ್ದರೆ
        // MVP: one memory per module, imported or declared
        _throwif(m3_Err_too_Many_ಸ್ಮರಣೆ_Sections, io_module->ಅಂಕೆ_Memories >= 1);
#ಆದರೆ_ಅಂತ್ಯ

_       (Module_ಕೂಡಿಸು_ಸ್ಮರಣೆ(io_module, &ಸ್ಮರಣೆ, &i_desc->ಸ್ಮರಣೆ, true /* isImport */));
        ಸ್ಮರಣೆ->import = *io_import;
        *io_import     = clearImport;
    } ನಿಲ್ಲಿಸು;

    ಸ್ಥಿತಿ d_external_ಬಗೆ_global: {
        IM3Global global;
_       (Module_ಕೂಡಿಸು_Global(io_module, &global, i_desc->type, i_desc->isMutable, true /* isImport */));
        global->import = *io_import;
        *io_import     = clearImport;
    } ನಿಲ್ಲಿಸು;

#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    ಸ್ಥಿತಿ d_external_ಬಗೆ_ಟ್ಯಾಗ್: {
        IM3_ಟ್ಯಾಗ್ ಟ್ಯಾಗ್;
_       (Module_ಕೂಡಿಸು_ಟ್ಯಾಗ್(io_module, &ಟ್ಯಾಗ್, io_module->funcTypes[i_desc->type_ಸೂಚ್ಯಂಕ], true /* isImport */));
        ಟ್ಯಾಗ್->import = *io_import;
        *io_import  = clearImport;
    } ನಿಲ್ಲಿಸು;
#ಆದರೆ_ಅಂತ್ಯ

    ಪೂರ್ವನಿಯೋಜಿತ:
        _throw(m3Err_wasmMalformed);
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


#ಆದರೆ d_m3HasCompactImports

// The compact encodings only apply when the item name is empty. Read_utf8()
// hands back a C string, so a name that merely begins with U+0000 would look
// empty; the length prefix is what has to be tested.
ಸ್ಥಿರ
bool Is_ಖಾಲಿಗಳು_Name (ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    u32 ಉದ್ದಳತೆ;
    ಹಿಂತಿರುಗಿ (not ReadLEB_u32(&ಉದ್ದಳತೆ, &i_ಬೈಟುಗಳು, i_ಕೊನೆ)) and ಉದ್ದಳತೆ == 0;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasCompactImports


#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching

M3_ಫಲಿತಾಂಶ Parse_Section_ಟ್ಯಾಗ್ (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಅಂಕೆ_ಟ್ಯಾಗುಗಳು;
_   (ReadLEB_u32(&ಅಂಕೆ_ಟ್ಯಾಗುಗಳು, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                       m3log (parse, "** Tag [%d]", ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);

    _throwif("too many tags", ಅಂಕೆ_ಟ್ಯಾಗುಗಳು > d_m3_Max_Sane_ಟ್ಯಾಗುಗಳು_ಎಣಿಕೆ);

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ಟ್ಯಾಗುಗಳು; ++i) {
        u32 type_ಸೂಚ್ಯಂಕ;
_       (Read_ಟ್ಯಾಗ್_Type(io_module, &type_ಸೂಚ್ಯಂಕ, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                      m3log (parse, "    tag: [%d] type: %d", i, type_ಸೂಚ್ಯಂಕ);

_       (Module_ಕೂಡಿಸು_ಟ್ಯಾಗ್(io_module, ಖಾಲಿ, io_module->funcTypes[type_ಸೂಚ್ಯಂಕ], false));
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ಆದರೆ_ಅಂತ್ಯ // d_m3HasExceptionHandling


M3_ಫಲಿತಾಂಶ ParseSection_Import (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    M3ImportInfo import = { ಖಾಲಿ, ಖಾಲಿ };

    // the limit is on the imports the section declares, which a compact run
    // multiplies well past the entry count. Declared up here so the throws
    // below don't jump over its initialization.
    u32 ಅಂಕೆ_Imports = 0;

    // A count of entries, not of imports: under the compact encodings a single
    // entry stands for a whole run of them.
    u32 ಅಂಕೆ_Entries;
_   (ReadLEB_u32(&ಅಂಕೆ_Entries, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                    m3log (parse, "** Import [%d]", ಅಂಕೆ_Entries);

    _throwif("too many imports", ಅಂಕೆ_Entries > d_m3_Max_Sane_Imports_ಎಣಿಕೆ);

    // Most imports are functions, so we won't waste much space anyway (if any)
_   (Module_PreallocFunctions(io_module, io_module->ಅಂಕೆ_Functions + ಅಂಕೆ_Entries));

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Entries; ++i) {
        M3ImportDesc desc;

_       (Read_utf8(&import.moduleUtf8, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

#ಆದರೆ d_m3HasCompactImports
        ಬೈಟುಗಳು_t ಕ್ಷೇತ್ರ_ಆರಂಭ = i_ಬೈಟುಗಳು;
#ಆದರೆ_ಅಂತ್ಯ
_       (Read_utf8(&import.ಕ್ಷೇತ್ರ_Utf8, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

#ಆದರೆ d_m3HasCompactImports
        u8 compact = (i_ಬೈಟುಗಳು < i_ಕೊನೆ) ? *i_ಬೈಟುಗಳು : 0;

        // An empty item name where an externtype should start marks a compact
        // run sharing this module name. Neither marker is a valid externtype,
        // so nothing that used to parse changes meaning.
        ಆದರೆ ((compact == d_compact_Imports_ಪ್ರತಿ_ಅಂಶ_Type or compact == d_compactImports_sharedType) and
            Is_ಖಾಲಿಗಳು_Name(ಕ್ಷೇತ್ರ_ಆರಂಭ, i_ಕೊನೆ)) {
            ++i_ಬೈಟುಗಳು;

            m3_Free(import.ಕ್ಷೇತ್ರ_Utf8);         // it only marked the compact form
            import.ಕ್ಷೇತ್ರ_Utf8 = ಖಾಲಿ;

            // the shared form puts its one externtype ahead of the item names,
            // so reading it here is also what validates it
            ಆದರೆ (compact == d_compactImports_sharedType) {
_               (ReadImportDesc(io_module, &desc, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
            }

            u32 ಅಂಕೆ_ವಸ್ತುಗಳು;
_           (ReadLEB_u32(&ಅಂಕೆ_ವಸ್ತುಗಳು, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                              m3log (parse, "    compact: %d import(s) of '%s'", ಅಂಕೆ_ವಸ್ತುಗಳು, import.moduleUtf8);

            _throwif("too many imports", (u64)ಅಂಕೆ_Imports + ಅಂಕೆ_ವಸ್ತುಗಳು > d_m3_Max_Sane_Imports_ಎಣಿಕೆ);
            ಅಂಕೆ_Imports += ಅಂಕೆ_ವಸ್ತುಗಳು;

_           (Module_PreallocFunctions(io_module, io_module->ಅಂಕೆ_Functions + ಅಂಕೆ_ವಸ್ತುಗಳು));

            ಗಾಗಿ (u32 j = 0; j < ಅಂಕೆ_ವಸ್ತುಗಳು; ++j) {
                M3ImportInfo ಅಂಶ = { ಖಾಲಿ, ಖಾಲಿ };

                ಅಂಶ.moduleUtf8 = (cstr_t)m3_CopyMem(import.moduleUtf8, ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(import.moduleUtf8) + 1);

                ಆದರೆ (ಅಂಶ.moduleUtf8) {
                    ಫಲಿತಾಂಶ = Read_utf8(&ಅಂಶ.ಕ್ಷೇತ್ರ_Utf8, &i_ಬೈಟುಗಳು, i_ಕೊನೆ);

                    // the shared form reuses the externtype read above
                    ಆದರೆ (not ಫಲಿತಾಂಶ and compact == d_compact_Imports_ಪ್ರತಿ_ಅಂಶ_Type) {
                        ಫಲಿತಾಂಶ = ReadImportDesc(io_module, &desc, &i_ಬೈಟುಗಳು, i_ಕೊನೆ);
                    }

                    ಆದರೆ (not ಫಲಿತಾಂಶ) {
                        ಫಲಿತಾಂಶ = ApplyImportDesc(io_module, &desc, &ಅಂಶ);
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    ಫಲಿತಾಂಶ = m3Err_mallocFailed;
                }

                FreeImportInfo(&ಅಂಶ);
                _throwif(ಫಲಿತಾಂಶ, ಫಲಿತಾಂಶ);
            }

            FreeImportInfo(&import);
            ಮುಂದುವರಿ;
        }
#ಆದರೆ_ಅಂತ್ಯ // d_m3HasCompactImports

        _throwif("too many imports", ಅಂಕೆ_Imports >= d_m3_Max_Sane_Imports_ಎಣಿಕೆ);
        ++ಅಂಕೆ_Imports;

_       (ReadImportDesc(io_module, &desc, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (ApplyImportDesc(io_module, &desc, &import));

        FreeImportInfo(&import);
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch:

    FreeImportInfo(&import);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ParseSection_Export (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ    ಫಲಿತಾಂಶ = m3Err_none;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* utf8   = ಖಾಲಿ;

    u32 ಅಂಕೆ_Exports;
_   (ReadLEB_u32(&ಅಂಕೆ_Exports, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                    m3log (parse, "** Export [%d]", ಅಂಕೆ_Exports);

    _throwif("too many exports", ಅಂಕೆ_Exports > d_m3_Max_Sane_Exports_ಎಣಿಕೆ);

    // the section order check lets this section through once, so this is
    // every export the module has
    ಆದರೆ (ಅಂಕೆ_Exports) {
        io_module->exports = m3_AllocArray(M3Export, ಅಂಕೆ_Exports);
        _throwifnull(io_module->exports);
    }

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Exports; ++i) {
        u8  export_ಬಗೆ;
        u32 index;
        u32 name_ಉದ್ದಳತೆ;

        // Read_utf8 takes the length along with the name, so read it ahead
        {
            ಬೈಟುಗಳು_t ಉದ್ದಳತೆ_ಬೈಟುಗಳು = i_ಬೈಟುಗಳು;
_           (ReadLEB_u32(&name_ಉದ್ದಳತೆ, &ಉದ್ದಳತೆ_ಬೈಟುಗಳು, i_ಕೊನೆ));
        }

_       (Read_utf8(&utf8, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (Read_u8(&export_ಬಗೆ, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (ReadLEB_u32(&index, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                     m3log (parse, "    index: %3d; kind: %d; export: '%s'; ", index, (u32) export_ಬಗೆ, utf8);

#ಆದರೆ d_m3EnableValidation
        // Spec: all export names must be different - compared by length, since
        // a name may hold a NUL
        ಗಾಗಿ (u32 j = 0; j < i; ++j) {
            ಸ್ಥಿರಾಂಕ M3Export* other = &io_module->exports[j];

            _throwif(m3Err_wasmMalformed, other->name_ಉದ್ದಳತೆ == name_ಉದ್ದಳತೆ and memcmp(other->name, utf8, name_ಉದ್ದಳತೆ) == 0);
        }
#ಆದರೆ_ಅಂತ್ಯ

        ಆದರೆ (export_ಬಗೆ == d_external_ಬಗೆ_function) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ಅಂಕೆ_Functions);
            IM3Function func = &(io_module->functions[index]);
            ಆದರೆ (func->ಅಂಕೆ_ಹೆಸರುಗಳು < d_m3MaxDuplicateFunctionImpl) {
                // a name of its own: names are freed with the function
                cstr_t name = (cstr_t)m3_CopyMem(utf8, name_ಉದ್ದಳತೆ + 1);
                _throwifnull(name);
                func->ಹೆಸರುಗಳು[func->ಅಂಕೆ_ಹೆಸರುಗಳು++] = name;
            }
_           (Module_DeclareFunction(io_module, index));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (export_ಬಗೆ == d_external_ಬಗೆ_global) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ಅಂಕೆ_Globals);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (export_ಬಗೆ == d_external_ಬಗೆ_ಸ್ಮರಣೆ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ಅಂಕೆ_Memories);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (export_ಬಗೆ == d_external_ಬಗೆ_ಕೋಷ್ಟಕ) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ಅಂಕೆ_Tables);
        }
#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (export_ಬಗೆ == d_external_ಬಗೆ_ಟ್ಯಾಗ್) {
            _throwif(m3Err_wasmMalformed, index >= io_module->ಅಂಕೆ_ಟ್ಯಾಗುಗಳು);
        }
#ಆದರೆ_ಅಂತ್ಯ

        M3Export* ನಮೂದು   = &io_module->exports[io_module->ಅಂಕೆ_Exports++];
        ನಮೂದು->name       = utf8;
        ನಮೂದು->name_ಉದ್ದಳತೆ = name_ಉದ್ದಳತೆ;
        ನಮೂದು->index      = index;
        ನಮೂದು->ಬಗೆ       = export_ಬಗೆ;

        utf8 = ಖಾಲಿ; // ownership transferred to M3Module
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

_catch:
    m3_Free(utf8);
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Section_ಆರಂಭ (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಆರಂಭ_Func_ಸೂಚ್ಯಂಕ;
_   (ReadLEB_u32(&ಆರಂಭ_Func_ಸೂಚ್ಯಂಕ, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                  m3log (parse, "** Start Function: %d", ಆರಂಭ_Func_ಸೂಚ್ಯಂಕ);

    ಆದರೆ (ಆರಂಭ_Func_ಸೂಚ್ಯಂಕ < io_module->ಅಂಕೆ_Functions) {
        // Spec: start function type must be [] -> []
        IM3Function func = &io_module->functions[ಆರಂಭ_Func_ಸೂಚ್ಯಂಕ];
        ಆದರೆ (func->funcType) {
            _throwif(m3Err_wasmMalformed,
                     func->funcType->ಅಂಕೆ_ವಾದಗಳು != 0 || func->funcType->ಅಂಕೆ_Rets != 0);
        }

        io_module->ಆರಂಭ_Function = ಆರಂಭ_Func_ಸೂಚ್ಯಂಕ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಫಲಿತಾಂಶ = "start function index out of bounds";
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


// o_type, when asked for, reports what the expression leaves on the stack, so
// the caller can check it against the type the expression is initializing.
M3_ಫಲಿತಾಂಶ Parse_Init_ಅಭಿವ್ಯಕ್ತಿ_Typed (M3Module* io_module, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ, m3type_t* o_type)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    // this doesn't generate code pages. just walks the wasm bytecode to find the end

#ಆದರೆ defined(d_m3PreferStaticAlloc)
    ಸ್ಥಿರ M3Compilation compilation;
#ಇಲ್ಲದಿದ್ದರೆ
    M3Compilation compilation;
#ಆದರೆ_ಅಂತ್ಯ
    compilation = (M3Compilation){ .runtime = ಖಾಲಿ, .module = io_module, .wasm = *io_ಬೈಟುಗಳು, .wasm_ಕೊನೆ = i_ಕೊನೆ, .is_Init_ಅಭಿವ್ಯಕ್ತಿ = true };

    ಫಲಿತಾಂಶ = Compile_ಖಂಡ_Statements(&compilation);

    *io_ಬೈಟುಗಳು = compilation.wasm;

    ಆದರೆ (o_type) {
        *o_type = compilation.ರಾಶಿ_ಸೂಚ್ಯಂಕ ? compilation.type_ರಾಶಿ[compilation.ರಾಶಿ_ಸೂಚ್ಯಂಕ - 1]
                                         : (m3type_t)c_m3Type_none;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Init_ಅಭಿವ್ಯಕ್ತಿ (M3Module* io_module, ಬೈಟುಗಳು_t* io_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    ಹಿಂತಿರುಗಿ Parse_Init_ಅಭಿವ್ಯಕ್ತಿ_Typed(io_module, io_ಬೈಟುಗಳು, i_ಕೊನೆ, ಖಾಲಿ);
}


M3_ಫಲಿತಾಂಶ ParseSection_Element (IM3Module io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32     ಅಂಕೆ_Segments;
    ಬೈಟುಗಳು_t pos;
_   (ReadLEB_u32(&ಅಂಕೆ_Segments, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                            m3log (parse, "** Element [%d]", ಅಂಕೆ_Segments);

    _throwif("too many element segments", ಅಂಕೆ_Segments > d_m3MaxSaneElementSegments);

    io_module->elementSegments = m3_AllocArray(M3ElementSegment, ಅಂಕೆ_Segments);
    _throwifnull(io_module->elementSegments);
    io_module->ಅಂಕೆ_Element_Segments = ಅಂಕೆ_Segments;
    io_module->element_Section_ಕೊನೆ  = i_ಕೊನೆ;

    // Records where each segment's parts live; the elements are resolved later,
    // in InitTableAndElements.
    pos = i_ಬೈಟುಗಳು;
    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Segments; ++i) {
        M3ElementSegment* segment = &io_module->elementSegments[i];

        u32 flags;
_       (ReadLEB_u32(&flags, &pos, i_ಕೊನೆ));
#ಆದರೆ d_m3HasRefTypes
        _throwif(m3Err_wasmMalformed, flags > 7);
#ಇಲ್ಲದಿದ್ದರೆ
        _throwif(m3Err_wasmMalformed, flags != c_m3_Elem_ಸಕ್ರಿಯ);
#ಆದರೆ_ಅಂತ್ಯ

        segment->ಬಹುಲಕ   = flags & 0x3;
        segment->is_ಅಭಿವ್ಯಕ್ತಿ = (flags & 0x4) != 0;
        segment->type   = c_m3Type_funcref;

        ಆದರೆ (segment->ಬಹುಲಕ == c_m3_Elem_ಸಕ್ರಿಯ_Idx) {
_           (ReadLEB_u32(&segment->ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ, &pos, i_ಕೊನೆ));
        }

        bool is_ಸಕ್ರಿಯ = (segment->ಬಹುಲಕ == c_m3_Elem_ಸಕ್ರಿಯ or segment->ಬಹುಲಕ == c_m3_Elem_ಸಕ್ರಿಯ_Idx);

        ಆದರೆ (is_ಸಕ್ರಿಯ) {
            _throwif(m3Err_wasmMalformed, segment->ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ >= io_module->ಅಂಕೆ_Tables);

            segment->init_ಅಭಿವ್ಯಕ್ತಿ = pos;
_           (Parse_Init_ಅಭಿವ್ಯಕ್ತಿ(io_module, &pos, i_ಕೊನೆ));
            segment->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ = (u32)(pos - segment->init_ಅಭಿವ್ಯಕ್ತಿ);
            _throwif(m3_Err_wasm_Missing_Init_ಅಭಿವ್ಯಕ್ತಿ, segment->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ <= 1);
        }

        // Only the table-0 active form leaves the element type implicit
        ಆದರೆ (segment->ಬಹುಲಕ != c_m3_Elem_ಸಕ್ರಿಯ) {
            ಆದರೆ (segment->is_ಅಭಿವ್ಯಕ್ತಿ) {
_               (Parse_ಮೌಲ್ಯ_Type(io_module, &segment->type, &pos, i_ಕೊನೆ));
                _throwif(m3Err_wasmMalformed, not IsRefType(ತಳ_Type_ದ(segment->type)));
            } ಇಲ್ಲದಿದ್ದರೆ {
                u8 elem_ಬಗೆ;
_               (Read_u8(&elem_ಬಗೆ, &pos, i_ಕೊನೆ));
                _throwif(m3Err_wasmMalformed, elem_ಬಗೆ != 0x00);   // funcref
            }
        }

        ಆದರೆ (is_ಸಕ್ರಿಯ) {
            _throwif(m3Err_typeMismatch, io_module->tables[segment->ಕೋಷ್ಟಕ_ಸೂಚ್ಯಂಕ]->type != segment->type);
        }

_       (ReadLEB_u32(&segment->ಅಂಕೆ_Elements, &pos, i_ಕೊನೆ));
        _throwif("table overflow", segment->ಅಂಕೆ_Elements > d_m3_Max_Sane_ಕೋಷ್ಟಕ_ಗಾತ್ರ);

        segment->elements = pos;

        ಗಾಗಿ (u32 e = 0; e < segment->ಅಂಕೆ_Elements; ++e) {
            ಆದರೆ (segment->is_ಅಭಿವ್ಯಕ್ತಿ) {
_               (Parse_Init_ಅಭಿವ್ಯಕ್ತಿ(io_module, &pos, i_ಕೊನೆ))
            } ಇಲ್ಲದಿದ್ದರೆ {
                u32 func_ಸೂಚ್ಯಂಕ;
_               (ReadLEB_u32(&func_ಸೂಚ್ಯಂಕ, &pos, i_ಕೊನೆ));
                _throwif("function index out of range", func_ಸೂಚ್ಯಂಕ >= io_module->ಅಂಕೆ_Functions);
_               (Module_DeclareFunction(io_module, func_ಸೂಚ್ಯಂಕ));
            }
        }
    }

    _throwif(m3Err_wasmMalformed, pos != i_ಕೊನೆ);           // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Section_ಸಂಕೇತ (M3Module* io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ;

    u32 ಅಂಕೆ_Functions;
_   (ReadLEB_u32(&ಅಂಕೆ_Functions, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                  m3log (parse, "** Code [%d]", ಅಂಕೆ_Functions);

    ಆದರೆ (ಅಂಕೆ_Functions != io_module->ಅಂಕೆ_Functions - io_module->ಅಂಕೆ_Func_Imports) {
        _throw("mismatched function count in code section");
    }

    ಗಾಗಿ (u32 f = 0; f < ಅಂಕೆ_Functions; ++f) {
        ಸ್ಥಿರಾಂಕ u8* ಆರಂಭ = i_ಬೈಟುಗಳು;

        u32 ಗಾತ್ರ_2;
_       (ReadLEB_u32(&ಗಾತ್ರ_2, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

        ಆದರೆ (ಗಾತ್ರ_2) {
            i_ಬೈಟುಗಳು += ಗಾತ್ರ_2;

            ಆದರೆ (i_ಬೈಟುಗಳು <= i_ಕೊನೆ) {
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

                IM3Function func = Module_GetFunction(io_module, f + io_module->ಅಂಕೆ_Func_Imports);

                func->module  = io_module;
                func->wasm    = ಆರಂಭ;
                func->wasm_ಕೊನೆ = i_ಬೈಟುಗಳು;
                //func->ownsWasmCode = io_module->hasWasmCodeCopy;
                //                func->numLocals = numLocals;
            } ಇಲ್ಲದಿದ್ದರೆ {
                _throw(m3Err_wasmSectionOverrun);
            }
        }
    }

    _catch:

    ಆದರೆ (not ಫಲಿತಾಂಶ and i_ಬೈಟುಗಳು != i_ಕೊನೆ) {
        ಫಲಿತಾಂಶ = m3Err_wasmSectionUnderrun;
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Section_ದತ್ತಾಂಶ (M3Module* io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಅಂಕೆ_ದತ್ತಾಂಶ_Segments;
_   (ReadLEB_u32(&ಅಂಕೆ_ದತ್ತಾಂಶ_Segments, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                               m3log (parse, "** Data [%d]", ಅಂಕೆ_ದತ್ತಾಂಶ_Segments);

    _throwif("too many data segments", ಅಂಕೆ_ದತ್ತಾಂಶ_Segments > d_m3_Max_Sane_ದತ್ತಾಂಶ_Segments);

    io_module->ದತ್ತಾಂಶ_Segments = m3_AllocArray(M3_ದತ್ತಾಂಶ_Segment, ಅಂಕೆ_ದತ್ತಾಂಶ_Segments);
    _throwifnull(io_module->ದತ್ತಾಂಶ_Segments);
    io_module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments = ಅಂಕೆ_ದತ್ತಾಂಶ_Segments;

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ದತ್ತಾಂಶ_Segments; ++i) {
        M3_ದತ್ತಾಂಶ_Segment* segment = &io_module->ದತ್ತಾಂಶ_Segments[i];

        // 0: active in memory 0, 1: passive, 2: active with an explicit memory index
        u32 flags;
_       (ReadLEB_u32(&flags, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
        _throwif(m3Err_wasmMalformed, flags > 2);

        segment->isPassive = (flags == 1);

        ಆದರೆ (flags == 2) {
_           (ReadLEB_u32(&segment->ಸ್ಮರಣೆ_Region, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
        }

        ಆದರೆ (not segment->isPassive) {
            // The segment names the memory it initializes; only multi-memory
            // lets that be anything other than 0.
#ಆದರೆ !d_m3_Has_Multi_ಸ್ಮರಣೆ
            _throwif(m3Err_wasmMalformed, segment->ಸ್ಮರಣೆ_Region != 0);
#ಆದರೆ_ಅಂತ್ಯ
            _throwif(m3Err_wasmMalformed, segment->ಸ್ಮರಣೆ_Region >= io_module->ಅಂಕೆ_Memories);

            segment->init_ಅಭಿವ್ಯಕ್ತಿ = i_ಬೈಟುಗಳು;
_           (Parse_Init_ಅಭಿವ್ಯಕ್ತಿ(io_module, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
            segment->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ = (u32)(i_ಬೈಟುಗಳು - segment->init_ಅಭಿವ್ಯಕ್ತಿ);

            _throwif(m3_Err_wasm_Missing_Init_ಅಭಿವ್ಯಕ್ತಿ, segment->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ <= 1);
        }

_       (ReadLEB_u32(&segment->ಗಾತ್ರ_2, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
        segment->ದತ್ತಾಂಶ = i_ಬೈಟುಗಳು;                                                    m3log (parse, "    segment [%u]  memory: %u;  expr-size: %d;  size: %d",
                                                                                       i, segment->ಸ್ಮರಣೆ_Region, segment->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ, segment->ಗಾತ್ರ_2);
        i_ಬೈಟುಗಳು += segment->ಗಾತ್ರ_2;

        _throwif("data segment underflow", i_ಬೈಟುಗಳು > i_ಕೊನೆ);
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch:

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Section_ದತ್ತಾಂಶ_ಎಣಿಕೆ (M3Module* io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

_   (ReadLEB_u32(&io_module->ದತ್ತಾಂಶ_ಎಣಿಕೆ, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                       m3log (parse, "** DataCount [%d]", io_module->ದತ್ತಾಂಶ_ಎಣಿಕೆ);

    io_module->has_ದತ್ತಾಂಶ_ಎಣಿಕೆ = true;

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ Parse_Section_ಸ್ಮರಣೆ (M3Module* io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಅಂಕೆ_Memories;
_   (ReadLEB_u32(&ಅಂಕೆ_Memories, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                m3log (parse, "** Memory [%d]", ಅಂಕೆ_Memories);

#ಆದರೆ d_m3_Has_Multi_ಸ್ಮರಣೆ
    _throwif(m3_Err_too_Many_ಸ್ಮರಣೆ_Sections,
             (u64)ಅಂಕೆ_Memories + io_module->ಅಂಕೆ_Memories > d_m3_Max_Sane_Memories_ಎಣಿಕೆ);
#ಇಲ್ಲದಿದ್ದರೆ
    // MVP: at most one memory, counting any that was already imported
    _throwif(m3_Err_too_Many_ಸ್ಮರಣೆ_Sections, ಅಂಕೆ_Memories > 1);
    _throwif(m3_Err_too_Many_ಸ್ಮರಣೆ_Sections, (u64)ಅಂಕೆ_Memories + io_module->ಅಂಕೆ_Memories > 1);
#ಆದರೆ_ಅಂತ್ಯ

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Memories; ++i) {
        M3_ಸ್ಮರಣೆ_Info info;
_       (Parse_Type_ಸ್ಮರಣೆ(&info, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (Module_ಕೂಡಿಸು_ಸ್ಮರಣೆ(io_module, ಖಾಲಿ, &info, false /* isImport */));
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ParseSection_Global (M3Module* io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    u32 ಅಂಕೆ_Globals;
_   (ReadLEB_u32(&ಅಂಕೆ_Globals, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                    m3log (parse, "** Global [%d]", ಅಂಕೆ_Globals);

    _throwif("too many globals", ಅಂಕೆ_Globals > d_m3_Max_Sane_Globals_ಎಣಿಕೆ);

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Globals; ++i) {
        m3type_t type = c_m3Type_none;
        u8       isMutable;

_       (Parse_ಮೌಲ್ಯ_Type(io_module, &type, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (ReadLEB_u7(&isMutable, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));                                    m3log (parse, "    global: [%d] %s mutable: %d", i, c_waTypes [ತಳ_Type_ದ(type)],   (u32) isMutable);
        _throwif(m3Err_wasmMalformed, isMutable > 1);

        IM3Global global;
_       (Module_ಕೂಡಿಸು_Global(io_module, &global, type, isMutable, false /* isImport */));

        global->init_ಅಭಿವ್ಯಕ್ತಿ = i_ಬೈಟುಗಳು;
_       (Parse_Init_ಅಭಿವ್ಯಕ್ತಿ(io_module, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
        global->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ = (u32)(i_ಬೈಟುಗಳು - global->init_ಅಭಿವ್ಯಕ್ತಿ);

        _throwif(m3_Err_wasm_Missing_Init_ಅಭಿವ್ಯಕ್ತಿ, global->init_ಅಭಿವ್ಯಕ್ತಿ_ಗಾತ್ರ <= 1);
    }

    _throwif(m3Err_wasmMalformed, i_ಬೈಟುಗಳು != i_ಕೊನೆ);      // section size mismatch

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ParseSection_Name (M3Module* io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    cstr_t name = ಖಾಲಿ;

    ಆಗ (i_ಬೈಟುಗಳು < i_ಕೊನೆ) {
        u8  nameType;
        u32 payload_ಉದ್ದಳತೆ;

_       (ReadLEB_u7(&nameType, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
_       (ReadLEB_u32(&payload_ಉದ್ದಳತೆ, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

        ಬೈಟುಗಳು_t ಆರಂಭ = i_ಬೈಟುಗಳು;
        _throwif(m3Err_wasmSectionOverrun, payload_ಉದ್ದಳತೆ > (size_t)(i_ಕೊನೆ - ಆರಂಭ));

        ಆದರೆ (nameType == 1) {
            u32 ಅಂಕೆ_ಹೆಸರುಗಳು;
_           (ReadLEB_u32(&ಅಂಕೆ_ಹೆಸರುಗಳು, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

            _throwif("too many names", ಅಂಕೆ_ಹೆಸರುಗಳು > d_m3_Max_Sane_Functions_ಎಣಿಕೆ);

            ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ಹೆಸರುಗಳು; ++i) {
                u32 index;
_               (ReadLEB_u32(&index, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
_               (Read_utf8(&name, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));

                ಆದರೆ (index < io_module->ಅಂಕೆ_Functions) {
                    IM3Function func = &(io_module->functions[index]);
                    ಆದರೆ (func->ಅಂಕೆ_ಹೆಸರುಗಳು == 0) {
                        func->ಹೆಸರುಗಳು[0] = name;        m3log (parse, "    naming function%5d:  %s", index, name);
                        func->ಅಂಕೆ_ಹೆಸರುಗಳು = 1;

                        name = ಖಾಲಿ; // transfer ownership
                    }
                    //                          else m3log (parse, "prenamed: %s", io_module->functions [index].name);
                }

                m3_Free(name);
            }
        }

        i_ಬೈಟುಗಳು = ಆರಂಭ + payload_ಉದ್ದಳತೆ;
    }

    _catch: ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ParseSection_Custom (M3Module* io_module, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, cbytes_t i_ಕೊನೆ)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    cstr_t name = ಖಾಲಿ;
_   (Read_utf8(&name, &i_ಬೈಟುಗಳು, i_ಕೊನೆ));
                                                                                    m3log (parse, "** Custom: '%s'", name);
    ಆದರೆ (strcmp(name, "name") == 0) {
_       (ParseSection_Name(io_module, i_ಬೈಟುಗಳು, i_ಕೊನೆ));
#ಆದರೆ d_m3HasSnapshots
        // "snapshot" is the unnamed default, "snapshot.<name>" a named one. A bare
        // "snapshot." names nothing and is not either of them, so it is left to the
        // embedder's handler.
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (strcmp(name, "snapshot") == 0 || (ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(name) > 9 && ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(name, "snapshot.", 9) == 0)) {
        // Two of the same name are kept, not refused: a custom section does not
        // decide whether the module is valid. Selecting that name is what fails.
        cstr_t              snapName = (name[8] == '.') ? name + 9 : "";
        M3EmbeddedSnapshot* snap     = m3_AllocStruct(M3EmbeddedSnapshot);
        _throwifnull(snap);
        snap->name = (cstr_t)m3_CopyMem(snapName, ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(snapName) + 1);
        ಆದರೆ (!snap->name) {
            m3_Free(snap);
            _throw(m3Err_mallocFailed);
        }
        snap->ದತ್ತಾಂಶ           = i_ಬೈಟುಗಳು;
        snap->ಗಾತ್ರ_2           = (u32)(i_ಕೊನೆ - i_ಬೈಟುಗಳು);
        snap->ಮುಂದಿನ           = io_module->snapshots;
        io_module->snapshots = snap;
#ಆದರೆ_ಅಂತ್ಯ
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (io_module->environment->customSectionHandler) {
_       (io_module->environment->customSectionHandler(io_module, name, i_ಬೈಟುಗಳು, i_ಕೊನೆ));
    }

    // the section name is freed on the error path too: a malformed "name"
    // section, or a handler that rejects the payload, still throws past here
    _catch:

    m3_Free(name);

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ ParseModuleSection (M3Module* o_module, u8 i_sectionType, ಬೈಟುಗಳು_t i_ಬೈಟುಗಳು, u32 i_ಅಂಕೆ_ಬೈಟುಗಳು)
{
    M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;

    ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ M3_ಫಲಿತಾಂಶ (*M3Parser)(M3Module*, ಬೈಟುಗಳು_t, cbytes_t);

    ಸ್ಥಿರ M3Parser s_parsers[] = {
        ParseSection_Custom,    // 0
        ParseSection_Type,      // 1
        ParseSection_Import,    // 2
        ParseSection_Function,  // 3
        Parse_Type_ಕೋಷ್ಟಕ,        // 4
        Parse_Section_ಸ್ಮರಣೆ,    // 5
        ParseSection_Global,    // 6
        ParseSection_Export,    // 7
        Parse_Section_ಆರಂಭ,     // 8
        ParseSection_Element,   // 9
        Parse_Section_ಸಂಕೇತ,      // 10
        Parse_Section_ದತ್ತಾಂಶ,      // 11
        Parse_Section_ದತ್ತಾಂಶ_ಎಣಿಕೆ, // 12
#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
        Parse_Section_ಟ್ಯಾಗ್,       // 13
#ಆದರೆ_ಅಂತ್ಯ
    };

    M3Parser parser = ಖಾಲಿ;

    ಆದರೆ (i_sectionType < M3_ಎಣಿಕೆ_ದ(s_parsers)) {
        parser = s_parsers[i_sectionType];
    }

    ಆದರೆ (parser) {
        cbytes_t ಕೊನೆ = i_ಬೈಟುಗಳು + i_ಅಂಕೆ_ಬೈಟುಗಳು;
        ಫಲಿತಾಂಶ       = parser(o_module, i_ಬೈಟುಗಳು, ಕೊನೆ);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m3log(parse, " skipped section type: %d", (u32)i_sectionType);
    }

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}


M3_ಫಲಿತಾಂಶ m3_ParseModule (IM3Environment i_environment, IM3Module* o_module, cbytes_t i_ಬೈಟುಗಳು, u32 i_ಅಂಕೆ_ಬೈಟುಗಳು)
{
    IM3Module module;                                                               m3log (parse, "load module: %d bytes", i_ಅಂಕೆ_ಬೈಟುಗಳು);
_try {
    module = m3_AllocStruct(M3Module);
    _throwifnull(module);
    module->name          = ".unnamed";                                             m3log (parse, "load module: %d bytes", i_ಅಂಕೆ_ಬೈಟುಗಳು);
    module->ಆರಂಭ_Function = -1;
    //module->hasWasmCodeCopy = false;
    module->environment = i_environment;

    ಸ್ಥಿರಾಂಕ u8* pos = i_ಬೈಟುಗಳು;
    ಸ್ಥಿರಾಂಕ u8* ಕೊನೆ = pos + i_ಅಂಕೆ_ಬೈಟುಗಳು;

    module->wasm_ಆರಂಭ = pos;
    module->wasm_ಕೊನೆ   = ಕೊನೆ;

    u32 magic, ಆವೃತ್ತಿ;
_   (Read_u32(&magic, &pos, ಕೊನೆ));
_   (Read_u32(&ಆವೃತ್ತಿ, &pos, ಕೊನೆ));

    _throwif(m3Err_wasmMalformed, magic != 0x6d736100);
    _throwif(m3_Err_incompatible_Wasm_ಆವೃತ್ತಿ, ಆವೃತ್ತಿ != 1);

#ಆದರೆ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
    // the tag section sits between memory (5) and global (6)
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u8 sections_ಕ್ರಮ[] = { 1, 2, 3, 4, 5, 13, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#ಇಲ್ಲದಿದ್ದರೆ
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ u8 sections_ಕ್ರಮ[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 10, 11, 0 }; // 0 is a placeholder
#ಆದರೆ_ಅಂತ್ಯ
    u8 expectedSection = 0;

    ಆಗ (pos < ಕೊನೆ) {
        u8 section;
_       (ReadLEB_u7(&section, &pos, ಕೊನೆ));

        ಆದರೆ (section != 0) {
            // Ensure sections appear only once and in order
            ಆಗ (sections_ಕ್ರಮ[expectedSection++] != section) {
                _throwif(m3Err_misorderedWasmSection, expectedSection >= M3_ಎಣಿಕೆ_ದ(sections_ಕ್ರಮ) - 1);
            }
        }

        u32 section_ಉದ್ದಳತೆ;
_       (ReadLEB_u32(&section_ಉದ್ದಳತೆ, &pos, ಕೊನೆ));
        _throwif(m3Err_wasmMalformed, pos + section_ಉದ್ದಳತೆ > ಕೊನೆ);

_       (ParseModuleSection(module, section, pos, section_ಉದ್ದಳತೆ));

        pos += section_ಉದ್ದಳತೆ;
    }

    // Spec: if a function section exists, a code section must also exist with
    // matching count (and vice versa). ParseSection_Code checks the other
    // direction; this covers the case where the code section is missing entirely.
    ಆದರೆ (module->ಅಂಕೆ_Functions > module->ಅಂಕೆ_Func_Imports) {
        IM3Function ಮೊದಲ_Non_Import = &module->functions[module->ಅಂಕೆ_Func_Imports];
        _throwif(m3Err_wasmMalformed, ಮೊದಲ_Non_Import->wasm == ಖಾಲಿ);
    }

    // Spec: the data count section must agree with the data section, which may be absent
    _throwif(m3Err_wasmMalformed, module->has_ದತ್ತಾಂಶ_ಎಣಿಕೆ and module->ದತ್ತಾಂಶ_ಎಣಿಕೆ != module->ಅಂಕೆ_ದತ್ತಾಂಶ_Segments);

} _catch:

    ಆದರೆ (ಫಲಿತಾಂಶ) {
        m3_FreeModule(module);
        module = ಖಾಲಿ;
    }

    *o_module = module;

    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
