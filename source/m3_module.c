//
//  m3_module.c
//
//  Created by Steven Massey on 5/7/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_exception.h"


शून्य Module_FreeFunctions (IM3Module i_module)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Functions; ++i) {
        IM3Function func = &i_module->functions[i];
        Function_Release(func);
    }
}


शून्य m3_FreeModule (IM3Module i_module)
{
    जइ (i_module) {
        m3log(module, "freeing module: %s (funcs: %d; segments: %d)",
              i_module->name, i_module->अङ्कः_Functions, i_module->अङ्कः_Data_Segments);

        Module_FreeFunctions(i_module);

        m3_Free(i_module->functions);
        //m3_Free (i_module->imports);
        m3_Free(i_module->funcTypes);
        m3_Free(i_module->dataSegments);

        प्रति (u32 i = 0; i < i_module->अङ्कः_Tables; ++i) {
            IM3Table table = i_module->tables[i];

            // a slot may point at a table another module owns; that module
            // frees it
            जइ (not table or table->owner != i_module) {
                अनुवर्त;
            }

            जइ (table->elements and i_module->runtime) {
                i_module->runtime->tableElementsUsed -= table->size;
            }
            m3_Free(table->elements);
            FreeImportInfo(&table->import);
            m3_Free(table);
        }
        m3_Free(i_module->tables);

        प्रति (u32 i = 0; i < i_module->अङ्कः_Element_Segments; ++i) {
            m3_Free(i_module->elementSegments[i].resolved);
        }
        m3_Free(i_module->elementSegments);
        m3_Free(i_module->declaredFuncs);

        प्रति (u32 i = 0; i < i_module->अङ्कः_Globals; ++i) {
            FreeImportInfo(&(i_module->globals[i].import));
        }
        m3_Free(i_module->globals);

#जइ d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching
        प्रति (u32 i = 0; i < i_module->अङ्कः_Tags; ++i) {
            FreeImportInfo(&(i_module->tags[i].import));
        }
        m3_Free(i_module->tags);
#endif

        प्रति (u32 i = 0; i < i_module->अङ्कः_Memories; ++i) {
            IM3Memory memory = i_module->memories[i];

            // a slot may point at a memory another module owns; that module
            // frees it
            जइ (not memory or memory->owner != i_module) {
                अनुवर्त;
            }

            FreeMemoryBlock(memory);
            FreeImportInfo(&memory->import);
            m3_Free(memory);
        }
        m3_Free(i_module->memories);
        FreeMemoryBlock(&i_module->emptyMemory);

        प्रति (u32 i = 0; i < i_module->अङ्कः_Exports; ++i) {
            m3_Free(i_module->exports[i].name);
        }
        m3_Free(i_module->exports);

#जइ d_m3HasSnapshots
        M3EmbeddedSnapshot* snap = i_module->snapshots;
        जाव (snap) {
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
                        ध्रुव वर्ण* ध्रुव    i_moduleName,
                        ध्रुव वर्ण* ध्रुव    i_globalName,
                        ध्रुव IM3_Tagged_मानम् i_मानम्)
{
    M3Result result = m3Err_globalLookupFailed;

    प्रति (u32 i = 0; i < io_module->अङ्कः_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        जइ (not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            अनुवर्त;
        }

        जइ (strcmp(g->import.moduleUtf8, i_moduleName) != 0 or
            strcmp(g->import.fieldUtf8, i_globalName) != 0) {
            अनुवर्त;
        }

        जइ (g->type != i_मानम्->type) {
            प्रत्यागम m3Err_globalTypeMismatch;
        }

        विकल्प (i_मानम्->type) {
        पक्ष c_m3Type_i32: g->i32_मानम् = i_मानम्->मानम्.i32; भङ्ग;
        पक्ष c_m3Type_i64: g->i64_मानम् = i_मानम्->मानम्.i64; भङ्ग;
#जइ d_m3HasFloat
        पक्ष c_m3Type_f32: g->f32_मानम् = i_मानम्->मानम्.f32; भङ्ग;
        पक्ष c_m3Type_f64: g->f64_मानम् = i_मानम्->मानम्.f64; भङ्ग;
#endif
        स्वभाव: प्रत्यागम m3Err_invalidTypeId;
        }

        result = m3Err_none;
    }

    प्रत्यागम result;
}


// The function count is final by the time anything can declare a reference:
// imports and the function section both precede the global, export and element
// sections, and those in turn precede the code section.
M3Result Module_DeclareFunction (IM3Module io_module, u32 i_index)
{
    M3Result result = m3Err_none;

    जइ (i_index >= io_module->अङ्कः_Functions) {
        प्रत्यागम "function index out of range";
    }

    जइ (not io_module->declaredFuncs) {
        io_module->declaredFuncs = m3_AllocArray(u8, (io_module->अङ्कः_Functions + 7) / 8);
        _throwifnull(io_module->declaredFuncs);
    }

    io_module->declaredFuncs[i_index / 8] |= (u8)(1u << (i_index % 8));

    _catch: प्रत्यागम result;
}


bool Module_IsFunctionDeclared (IM3Module i_module, u32 i_index)
{
    जइ (not i_module->declaredFuncs or i_index >= i_module->अङ्कः_Functions) {
        प्रत्यागम false;
    }

    प्रत्यागम (i_module->declaredFuncs[i_index / 8] & (1u << (i_index % 8))) != 0;
}


// Appends an entry to the module's memory index space. The M3Memory is
// allocated separately from the index so that another module importing it can
// hold the same pointer.
M3Result Module_AddMemory (IM3Module io_module, IM3Memory* o_memory, ध्रुव M3MemoryInfo* i_info, bool i_isImported)
{
    IM3Memory memory = रिक्त;
_try {
    u32 index = io_module->अङ्कः_Memories;

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
    io_module->अङ्कः_Memories     = index + 1;

    जइ (o_memory) {
        *o_memory = memory;
    }

    प्रत्यागम result;

} _catch:
    m3_Free(memory);
    प्रत्यागम result;
}


// Appends an entry to the module's table index space. Allocated separately from
// the index, like a memory, so another module importing it can hold the same
// pointer.
M3Result Module_AddTable (IM3Module io_module, IM3Table* o_table, ध्रुव M3TableInfo* i_info, bool i_isImported)
{
    IM3Table table = रिक्त;
_try {
    u32 index = io_module->अङ्कः_Tables;

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
    io_module->अङ्कः_Tables     = index + 1;

    जइ (o_table) {
        *o_table = table;
    }

    प्रत्यागम result;

} _catch:
    m3_Free(table);
    प्रत्यागम result;
}


#जइ d_m3HasExceptionHandling || d_m3_Has_राशिः_Switching

M3Result Module_AddTag (IM3Module io_module, IM3Tag* o_tag, IM3FuncType i_type, bool i_isImported)
{
_try {
    u32 index = io_module->अङ्कः_Tags++;

    io_module->tags = m3_ReallocArray(M3Tag, io_module->tags, io_module->अङ्कः_Tags, index);
    _throwifnull(io_module->tags);
    M3Tag* tag = &io_module->tags[index];

    tag->type     = i_type;
    tag->imported = i_isImported;
    tag->resolved = रिक्त;
    memset(&tag->import, 0, परिमाण(tag->import));

    जइ (o_tag) {
        *o_tag = tag;
    }

} _catch:
    प्रत्यागम result;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#जइ d_m3_Has_राशिः_Switching

// The continuation type a spelled-out reference names. Reference types carry
// the canonical index, and the module's own type space is where a name for it
// can be found again - which switch needs, because the type of the peer it
// suspends into is only written down as the last parameter of the type it
// switches to.
IM3FuncType Module_ContTypeOfRef (IM3Module i_module, m3type_t i_refType)
{
    जइ (not IsSpelledRefType(i_refType)) {
        प्रत्यागम रिक्त;
    }

    u32 heap = HeapTypeOf(i_refType);

    जइ (heap == d_m3Type_heapAbstract) {
        प्रत्यागम रिक्त;
    }

    प्रति (u32 i = 0; i < i_module->अङ्कः_Func_Types; ++i) {
        IM3FuncType ftype = i_module->funcTypes[i];

        जइ (ftype and ftype->isContinuation and ftype->canonicalIndex == heap) {
            प्रत्यागम ftype;
        }
    }

    प्रत्यागम रिक्त;
}

#endif // d_m3HasStackSwitching


M3Result Module_AddGlobal (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported)
{
_try {
    u32 index = io_module->अङ्कः_Globals++;

    io_module->globals = m3_ReallocArray(M3Global, io_module->globals, io_module->अङ्कः_Globals, index);
    _throwifnull(io_module->globals);
    M3Global* global = &io_module->globals[index];

    global->type      = i_type;
    global->imported  = i_isImported;
    global->isMutable = i_mutable;

    जइ (o_global) {
        *o_global = global;
    }

} _catch:
    प्रत्यागम result;
}

M3Result Module_PreallocFunctions (IM3Module io_module, u32 i_totalFunctions)
{
_try {
    जइ (i_totalFunctions > io_module->allFunctions) {
        io_module->functions    = m3_ReallocArray(M3Function, io_module->functions, i_totalFunctions, io_module->allFunctions);
        io_module->allFunctions = i_totalFunctions;
        _throwifnull(io_module->functions);
    }
} _catch:
    प्रत्यागम result;
}

M3Result Module_AddFunction (IM3Module io_module, u32 i_typeIndex, IM3ImportInfo i_importInfo)
{
_try {

    u32 index = io_module->अङ्कः_Functions++;
_   (Module_PreallocFunctions(io_module, io_module->अङ्कः_Functions));

    _throwif("type sig index out of bounds", i_typeIndex >= io_module->अङ्कः_Func_Types);

    IM3FuncType ft = io_module->funcTypes[i_typeIndex];

    IM3Function func = Module_GetFunction(io_module, index);
    func->funcType   = ft;
    func->module     = io_module;       // an import has no body, but still belongs to this module

#ifdef DEBUG
    func->index = index;
#endif

    जइ (i_importInfo and func->अङ्कः_Names == 0) {
        func->import   = *i_importInfo;
        func->names[0] = i_importInfo->fieldUtf8;
        func->अङ्कः_Names = 1;
    }

    m3log(module, "   added function: %3d; sig: %d", index, i_typeIndex);

} _catch:
    प्रत्यागम result;
}

#ifdef DEBUG
शून्य Module_GenerateNames (IM3Module i_module)
{
    प्रति (u32 i = 0; i < i_module->अङ्कः_Functions; ++i) {
        IM3Function func = &i_module->functions[i];

        जइ (func->अङ्कः_Names == 0) {
            वर्ण* buff = m3_AllocArray(वर्ण, 16);
            snprintf(buff, 16, "$func%d", i);
            func->names[0] = buff;
            func->अङ्कः_Names = 1;
        }
    }
}
#endif

IM3Function Module_GetFunction (IM3Module i_module, u32 i_functionIndex)
{
    IM3Function func = रिक्त;

    जइ (i_functionIndex < i_module->अङ्कः_Functions) {
        func = &i_module->functions[i_functionIndex];
        //func->module = i_module;
    }

    प्रत्यागम func;
}


ध्रुव वर्ण* m3_GetModuleName (IM3Module i_module)
{
    जइ (!i_module || !i_module->name) {
        प्रत्यागम ".unnamed";
    }

    प्रत्यागम i_module->name;
}

शून्य m3_SetModuleName (IM3Module i_module, ध्रुव वर्ण* name)
{
    जइ (i_module) {
        i_module->name = name;
    }
}

IM3Runtime m3_GetModuleRuntime (IM3Module i_module)
{
    प्रत्यागम i_module ? i_module->runtime : रिक्त;
}
