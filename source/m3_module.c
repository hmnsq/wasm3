//
//  m3_module.c
//
//  Created by Steven Massey on 5/7/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_exception.h"


ಶೂನ್ಯ Module_FreeFunctions (IM3Module i_module)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function func = &i_module->functions[i];
        Function_Release(func);
    }
}


ಶೂನ್ಯ m3_FreeModule (IM3Module i_module)
{
    ಯದಿ (i_module) {
        m3log(module, "freeing module: %s (funcs: %d; segments: %d)",
              i_module->name, i_module->ಅಂಕೆ_Functions, i_module->ಅಂಕೆ_Data_Segments);

        Module_FreeFunctions(i_module);

        m3_Free(i_module->functions);
        //m3_Free (i_module->imports);
        m3_Free(i_module->funcTypes);
        m3_Free(i_module->dataSegments);

        ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Tables; ++i) {
            IM3Table table = i_module->tables[i];

            // a slot may point at a table another module owns; that module
            // frees it
            ಯದಿ (not table or table->owner != i_module) {
                ಅನುವರ್ತ;
            }

            ಯದಿ (table->elements and i_module->runtime) {
                i_module->runtime->tableElementsUsed -= table->size;
            }
            m3_Free(table->elements);
            FreeImportInfo(&table->import);
            m3_Free(table);
        }
        m3_Free(i_module->tables);

        ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Element_Segments; ++i) {
            m3_Free(i_module->elementSegments[i].resolved);
        }
        m3_Free(i_module->elementSegments);
        m3_Free(i_module->declaredFuncs);

        ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Globals; ++i) {
            FreeImportInfo(&(i_module->globals[i].import));
        }
        m3_Free(i_module->globals);

#ಯದಿ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching
        ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Tags; ++i) {
            FreeImportInfo(&(i_module->tags[i].import));
        }
        m3_Free(i_module->tags);
#endif

        ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Memories; ++i) {
            IM3Memory memory = i_module->memories[i];

            // a slot may point at a memory another module owns; that module
            // frees it
            ಯದಿ (not memory or memory->owner != i_module) {
                ಅನುವರ್ತ;
            }

            FreeMemoryBlock(memory);
            FreeImportInfo(&memory->import);
            m3_Free(memory);
        }
        m3_Free(i_module->memories);
        FreeMemoryBlock(&i_module->emptyMemory);

        ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Exports; ++i) {
            m3_Free(i_module->exports[i].name);
        }
        m3_Free(i_module->exports);

#ಯದಿ d_m3HasSnapshots
        M3EmbeddedSnapshot* snap = i_module->snapshots;
        ಯಾವತ್ (snap) {
            M3EmbeddedSnapshot* next = snap->next;
            m3_Free(snap->name);
            m3_Free(snap);
            snap = next;
        }
#endif

        m3_Free(i_module);
    }
}


// Supplies the value of an imported global. Unlike m3_SetGlobal this ignores
// mutability: the import's mutability governs what the wasm code may do with it,
// not whether the host may provide it in the first place.
M3Result m3_LinkGlobal (IM3Module            io_module,
                        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ    i_moduleName,
                        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ    i_globalName,
                        ಧ್ರುವ IM3_Tagged_ಮೌಲ್ಯ i_ಮೌಲ್ಯ)
{
    M3Result result = m3Err_globalLookupFailed;

    ಪ್ರತಿ (u32 i = 0; i < io_module->ಅಂಕೆ_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        ಯದಿ (not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (strcmp(g->import.moduleUtf8, i_moduleName) != 0 or
            strcmp(g->import.fieldUtf8, i_globalName) != 0) {
            ಅನುವರ್ತ;
        }

        ಯದಿ (g->type != i_ಮೌಲ್ಯ->type) {
            ಪ್ರತ್ಯಾಗಮ m3Err_globalTypeMismatch;
        }

        ವಿಕಲ್ಪ (i_ಮೌಲ್ಯ->type) {
        ಪಕ್ಷ c_m3Type_i32: g->i32_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i32; ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_i64: g->i64_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.i64; ಭಙ್ಗ;
#ಯದಿ d_m3HasFloat
        ಪಕ್ಷ c_m3Type_f32: g->f32_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f32; ಭಙ್ಗ;
        ಪಕ್ಷ c_m3Type_f64: g->f64_ಮೌಲ್ಯ = i_ಮೌಲ್ಯ->ಮೌಲ್ಯ.f64; ಭಙ್ಗ;
#endif
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ m3Err_invalidTypeId;
        }

        result = m3Err_none;
    }

    ಪ್ರತ್ಯಾಗಮ result;
}


// The function count is final by the time anything can declare a reference:
// imports and the function section both precede the global, export and element
// sections, and those in turn precede the code section.
M3Result Module_DeclareFunction (IM3Module io_module, u32 i_index)
{
    M3Result result = m3Err_none;

    ಯದಿ (i_index >= io_module->ಅಂಕೆ_Functions) {
        ಪ್ರತ್ಯಾಗಮ "function index out of range";
    }

    ಯದಿ (not io_module->declaredFuncs) {
        io_module->declaredFuncs = m3_AllocArray(u8, (io_module->ಅಂಕೆ_Functions + 7) / 8);
        _throwifnull(io_module->declaredFuncs);
    }

    io_module->declaredFuncs[i_index / 8] |= (u8)(1u << (i_index % 8));

    _catch: ಪ್ರತ್ಯಾಗಮ result;
}


bool Module_IsFunctionDeclared (IM3Module i_module, u32 i_index)
{
    ಯದಿ (not i_module->declaredFuncs or i_index >= i_module->ಅಂಕೆ_Functions) {
        ಪ್ರತ್ಯಾಗಮ false;
    }

    ಪ್ರತ್ಯಾಗಮ (i_module->declaredFuncs[i_index / 8] & (1u << (i_index % 8))) != 0;
}


// Appends an entry to the module's memory index space. The M3Memory is
// allocated separately from the index so that another module importing it can
// hold the same pointer.
M3Result Module_AddMemory (IM3Module io_module, IM3Memory* o_memory, ಧ್ರುವ M3MemoryInfo* i_info, bool i_isImported)
{
    IM3Memory memory = ರಿಕ್ತ;
_try {
    u32 index = io_module->ಅಂಕೆ_Memories;

    memory = m3_AllocStruct(M3Memory);
    _throwifnull(memory);

    io_module->memories = m3_ReallocArray(IM3Memory, io_module->memories, index + 1, index);
    _throwifnull(io_module->memories);

    memory->owner      = io_module;
    memory->imported   = i_isImported;
    memory->initPages  = i_info->initPages;
    memory->maxPages   = i_info->maxPages;
    memory->pageSize   = i_info->pageSize;
    memory->hasMax     = i_info->hasMax;
    memory->isMemory64 = i_info->isMemory64;
    memory->isShared   = i_info->isShared;

    io_module->memories[index] = memory;
    io_module->ಅಂಕೆ_Memories     = index + 1;

    ಯದಿ (o_memory) {
        *o_memory = memory;
    }

    ಪ್ರತ್ಯಾಗಮ result;

} _catch:
    m3_Free(memory);
    ಪ್ರತ್ಯಾಗಮ result;
}


// Appends an entry to the module's table index space. Allocated separately from
// the index, like a memory, so another module importing it can hold the same
// pointer.
M3Result Module_AddTable (IM3Module io_module, IM3Table* o_table, ಧ್ರುವ M3TableInfo* i_info, bool i_isImported)
{
    IM3Table table = ರಿಕ್ತ;
_try {
    u32 index = io_module->ಅಂಕೆ_Tables;

    table = m3_AllocStruct(M3Table);
    _throwifnull(table);

    io_module->tables = m3_ReallocArray(IM3Table, io_module->tables, index + 1, index);
    _throwifnull(io_module->tables);

    table->owner     = io_module;
    table->imported  = i_isImported;
    table->type      = i_info->elemType;
    table->size      = i_info->initSize;
    table->initSize  = i_info->initSize;
    table->maxSize   = i_info->maxSize;
    table->hasMax    = i_info->hasMax;
    table->isTable64 = i_info->isTable64;

    io_module->tables[index] = table;
    io_module->ಅಂಕೆ_Tables     = index + 1;

    ಯದಿ (o_table) {
        *o_table = table;
    }

    ಪ್ರತ್ಯಾಗಮ result;

} _catch:
    m3_Free(table);
    ಪ್ರತ್ಯಾಗಮ result;
}


#ಯದಿ d_m3HasExceptionHandling || d_m3_Has_ರಾಶಿ_Switching

M3Result Module_AddTag (IM3Module io_module, IM3Tag* o_tag, IM3FuncType i_type, bool i_isImported)
{
_try {
    u32 index = io_module->ಅಂಕೆ_Tags++;

    io_module->tags = m3_ReallocArray(M3Tag, io_module->tags, io_module->ಅಂಕೆ_Tags, index);
    _throwifnull(io_module->tags);
    M3Tag* tag = &io_module->tags[index];

    tag->type     = i_type;
    tag->imported = i_isImported;
    tag->resolved = ರಿಕ್ತ;
    memset(&tag->import, 0, ಪರಿಮಾಣ(tag->import));

    ಯದಿ (o_tag) {
        *o_tag = tag;
    }

} _catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#ಯದಿ d_m3_Has_ರಾಶಿ_Switching

// The continuation type a spelled-out reference names. Reference types carry
// the canonical index, and the module's own type space is where a name for it
// can be found again - which switch needs, because the type of the peer it
// suspends into is only written down as the last parameter of the type it
// switches to.
IM3FuncType Module_ContTypeOfRef (IM3Module i_module, m3type_t i_refType)
{
    ಯದಿ (not IsSpelledRefType(i_refType)) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    u32 heap = HeapTypeOf(i_refType);

    ಯದಿ (heap == d_m3Type_heapAbstract) {
        ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
    }

    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Func_Types; ++i) {
        IM3FuncType ftype = i_module->funcTypes[i];

        ಯದಿ (ftype and ftype->isContinuation and ftype->canonicalIndex == heap) {
            ಪ್ರತ್ಯಾಗಮ ftype;
        }
    }

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}

#endif // d_m3HasStackSwitching


M3Result Module_AddGlobal (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported)
{
_try {
    u32 index = io_module->ಅಂಕೆ_Globals++;

    io_module->globals = m3_ReallocArray(M3Global, io_module->globals, io_module->ಅಂಕೆ_Globals, index);
    _throwifnull(io_module->globals);
    M3Global* global = &io_module->globals[index];

    global->type      = i_type;
    global->imported  = i_isImported;
    global->isMutable = i_mutable;

    ಯದಿ (o_global) {
        *o_global = global;
    }

} _catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

M3Result Module_PreallocFunctions (IM3Module io_module, u32 i_totalFunctions)
{
_try {
    ಯದಿ (i_totalFunctions > io_module->allFunctions) {
        io_module->functions    = m3_ReallocArray(M3Function, io_module->functions, i_totalFunctions, io_module->allFunctions);
        io_module->allFunctions = i_totalFunctions;
        _throwifnull(io_module->functions);
    }
} _catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

M3Result Module_AddFunction (IM3Module io_module, u32 i_typeIndex, IM3ImportInfo i_importInfo)
{
_try {

    u32 index = io_module->ಅಂಕೆ_Functions++;
_   (Module_PreallocFunctions(io_module, io_module->ಅಂಕೆ_Functions));

    _throwif("type sig index out of bounds", i_typeIndex >= io_module->ಅಂಕೆ_Func_Types);

    IM3FuncType ft = io_module->funcTypes[i_typeIndex];

    IM3Function func = Module_GetFunction(io_module, index);
    func->funcType   = ft;
    func->module     = io_module;       // an import has no body, but still belongs to this module

#ifdef DEBUG
    func->index = index;
#endif

    ಯದಿ (i_importInfo and func->ಅಂಕೆ_Names == 0) {
        func->import   = *i_importInfo;
        func->names[0] = i_importInfo->fieldUtf8;
        func->ಅಂಕೆ_Names = 1;
    }

    m3log(module, "   added function: %3d; sig: %d", index, i_typeIndex);

} _catch:
    ಪ್ರತ್ಯಾಗಮ result;
}

#ifdef DEBUG
ಶೂನ್ಯ Module_GenerateNames (IM3Module i_module)
{
    ಪ್ರತಿ (u32 i = 0; i < i_module->ಅಂಕೆ_Functions; ++i) {
        IM3Function func = &i_module->functions[i];

        ಯದಿ (func->ಅಂಕೆ_Names == 0) {
            ವರ್ಣ* buff = m3_AllocArray(ವರ್ಣ, 16);
            snprintf(buff, 16, "$func%d", i);
            func->names[0] = buff;
            func->ಅಂಕೆ_Names = 1;
        }
    }
}
#endif

IM3Function Module_GetFunction (IM3Module i_module, u32 i_functionIndex)
{
    IM3Function func = ರಿಕ್ತ;

    ಯದಿ (i_functionIndex < i_module->ಅಂಕೆ_Functions) {
        func = &i_module->functions[i_functionIndex];
        //func->module = i_module;
    }

    ಪ್ರತ್ಯಾಗಮ func;
}


ಧ್ರುವ ವರ್ಣ* m3_GetModuleName (IM3Module i_module)
{
    ಯದಿ (!i_module || !i_module->name) {
        ಪ್ರತ್ಯಾಗಮ ".unnamed";
    }

    ಪ್ರತ್ಯಾಗಮ i_module->name;
}

ಶೂನ್ಯ m3_SetModuleName (IM3Module i_module, ಧ್ರುವ ವರ್ಣ* name)
{
    ಯದಿ (i_module) {
        i_module->name = name;
    }
}

IM3Runtime m3_GetModuleRuntime (IM3Module i_module)
{
    ಪ್ರತ್ಯಾಗಮ i_module ? i_module->runtime : ರಿಕ್ತ;
}
