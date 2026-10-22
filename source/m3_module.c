//
//  m3_module.c
//
//  Created by Steven Massey on 5/7/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ضمن "m3_env.h"
#ضمن "m3_exception.h"


فراغ Module_FreeFunctions (IM3Module i_module)
{
    لكل (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function func = &i_module->functions[i];
        Function_Release(func);
    }
}


فراغ m3_FreeModule (IM3Module i_module)
{
    إذا (i_module) {
        m3log(module, "freeing module: %s (funcs: %d; segments: %d)",
              i_module->name, i_module->عدد_Functions, i_module->عدد_Data_Segments);

        Module_FreeFunctions(i_module);

        m3_Free(i_module->functions);
        //m3_Free (i_module->imports);
        m3_Free(i_module->funcTypes);
        m3_Free(i_module->dataSegments);

        لكل (u32 i = 0; i < i_module->عدد_Tables; ++i) {
            IM3Table table = i_module->tables[i];

            // a slot may point at a table another module owns; that module
            // frees it
            إذا (not table or table->owner != i_module) {
                استمر;
            }

            إذا (table->elements and i_module->runtime) {
                i_module->runtime->tableElementsUsed -= table->size;
            }
            m3_Free(table->elements);
            FreeImportInfo(&table->import);
            m3_Free(table);
        }
        m3_Free(i_module->tables);

        لكل (u32 i = 0; i < i_module->عدد_Element_Segments; ++i) {
            m3_Free(i_module->elementSegments[i].resolved);
        }
        m3_Free(i_module->elementSegments);
        m3_Free(i_module->declaredFuncs);

        لكل (u32 i = 0; i < i_module->عدد_Globals; ++i) {
            FreeImportInfo(&(i_module->globals[i].import));
        }
        m3_Free(i_module->globals);

#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching
        لكل (u32 i = 0; i < i_module->عدد_Tags; ++i) {
            FreeImportInfo(&(i_module->tags[i].import));
        }
        m3_Free(i_module->tags);
#endif

        لكل (u32 i = 0; i < i_module->عدد_Memories; ++i) {
            IM3Memory memory = i_module->memories[i];

            // a slot may point at a memory another module owns; that module
            // frees it
            إذا (not memory or memory->owner != i_module) {
                استمر;
            }

            FreeMemoryBlock(memory);
            FreeImportInfo(&memory->import);
            m3_Free(memory);
        }
        m3_Free(i_module->memories);
        FreeMemoryBlock(&i_module->emptyMemory);

        لكل (u32 i = 0; i < i_module->عدد_Exports; ++i) {
            m3_Free(i_module->exports[i].name);
        }
        m3_Free(i_module->exports);

#إذا d_m3HasSnapshots
        M3EmbeddedSnapshot* snap = i_module->snapshots;
        طالما (snap) {
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
                        ثابت حرف* ثابت    i_moduleName,
                        ثابت حرف* ثابت    i_globalName,
                        ثابت IM3_Tagged_قدر i_قدر)
{
    M3Result result = m3Err_globalLookupFailed;

    لكل (u32 i = 0; i < io_module->عدد_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            استمر;
        }

        إذا (strcmp(g->import.moduleUtf8, i_moduleName) != 0 or
            strcmp(g->import.fieldUtf8, i_globalName) != 0) {
            استمر;
        }

        إذا (g->type != i_قدر->type) {
            أرجع m3Err_globalTypeMismatch;
        }

        بدل (i_قدر->type) {
        حالة c_m3Type_i32: g->i32_قدر = i_قدر->قدر.i32; اكسر;
        حالة c_m3Type_i64: g->i64_قدر = i_قدر->قدر.i64; اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32: g->f32_قدر = i_قدر->قدر.f32; اكسر;
        حالة c_m3Type_f64: g->f64_قدر = i_قدر->قدر.f64; اكسر;
#endif
        افتراضي: أرجع m3Err_invalidTypeId;
        }

        result = m3Err_none;
    }

    أرجع result;
}


// The function count is final by the time anything can declare a reference:
// imports and the function section both precede the global, export and element
// sections, and those in turn precede the code section.
M3Result Module_DeclareFunction (IM3Module io_module, u32 i_index)
{
    M3Result result = m3Err_none;

    إذا (i_index >= io_module->عدد_Functions) {
        أرجع "function index out of range";
    }

    إذا (not io_module->declaredFuncs) {
        io_module->declaredFuncs = m3_AllocArray(u8, (io_module->عدد_Functions + 7) / 8);
        _throwifnull(io_module->declaredFuncs);
    }

    io_module->declaredFuncs[i_index / 8] |= (u8)(1u << (i_index % 8));

    _catch: أرجع result;
}


bool Module_IsFunctionDeclared (IM3Module i_module, u32 i_index)
{
    إذا (not i_module->declaredFuncs or i_index >= i_module->عدد_Functions) {
        أرجع false;
    }

    أرجع (i_module->declaredFuncs[i_index / 8] & (1u << (i_index % 8))) != 0;
}


// Appends an entry to the module's memory index space. The M3Memory is
// allocated separately from the index so that another module importing it can
// hold the same pointer.
M3Result Module_AddMemory (IM3Module io_module, IM3Memory* o_memory, ثابت M3MemoryInfo* i_info, bool i_isImported)
{
    IM3Memory memory = عدم;
_try {
    u32 index = io_module->عدد_Memories;

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
    io_module->عدد_Memories     = index + 1;

    إذا (o_memory) {
        *o_memory = memory;
    }

    أرجع result;

} _catch:
    m3_Free(memory);
    أرجع result;
}


// Appends an entry to the module's table index space. Allocated separately from
// the index, like a memory, so another module importing it can hold the same
// pointer.
M3Result Module_AddTable (IM3Module io_module, IM3Table* o_table, ثابت M3TableInfo* i_info, bool i_isImported)
{
    IM3Table table = عدم;
_try {
    u32 index = io_module->عدد_Tables;

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
    io_module->عدد_Tables     = index + 1;

    إذا (o_table) {
        *o_table = table;
    }

    أرجع result;

} _catch:
    m3_Free(table);
    أرجع result;
}


#إذا d_m3HasExceptionHandling || d_m3_Has_ڈھیر_Switching

M3Result Module_AddTag (IM3Module io_module, IM3Tag* o_tag, IM3FuncType i_type, bool i_isImported)
{
_try {
    u32 index = io_module->عدد_Tags++;

    io_module->tags = m3_ReallocArray(M3Tag, io_module->tags, io_module->عدد_Tags, index);
    _throwifnull(io_module->tags);
    M3Tag* tag = &io_module->tags[index];

    tag->type     = i_type;
    tag->imported = i_isImported;
    tag->resolved = عدم;
    memset(&tag->import, 0, حجم(tag->import));

    إذا (o_tag) {
        *o_tag = tag;
    }

} _catch:
    أرجع result;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3_Has_ڈھیر_Switching

// The continuation type a spelled-out reference names. Reference types carry
// the canonical index, and the module's own type space is where a name for it
// can be found again - which switch needs, because the type of the peer it
// suspends into is only written down as the last parameter of the type it
// switches to.
IM3FuncType Module_ContTypeOfRef (IM3Module i_module, m3type_t i_refType)
{
    إذا (not IsSpelledRefType(i_refType)) {
        أرجع عدم;
    }

    u32 heap = HeapTypeOf(i_refType);

    إذا (heap == d_m3Type_heapAbstract) {
        أرجع عدم;
    }

    لكل (u32 i = 0; i < i_module->عدد_Func_Types; ++i) {
        IM3FuncType ftype = i_module->funcTypes[i];

        إذا (ftype and ftype->isContinuation and ftype->canonicalIndex == heap) {
            أرجع ftype;
        }
    }

    أرجع عدم;
}

#endif // d_m3HasStackSwitching


M3Result Module_AddGlobal (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported)
{
_try {
    u32 index = io_module->عدد_Globals++;

    io_module->globals = m3_ReallocArray(M3Global, io_module->globals, io_module->عدد_Globals, index);
    _throwifnull(io_module->globals);
    M3Global* global = &io_module->globals[index];

    global->type      = i_type;
    global->imported  = i_isImported;
    global->isMutable = i_mutable;

    إذا (o_global) {
        *o_global = global;
    }

} _catch:
    أرجع result;
}

M3Result Module_PreallocFunctions (IM3Module io_module, u32 i_totalFunctions)
{
_try {
    إذا (i_totalFunctions > io_module->allFunctions) {
        io_module->functions    = m3_ReallocArray(M3Function, io_module->functions, i_totalFunctions, io_module->allFunctions);
        io_module->allFunctions = i_totalFunctions;
        _throwifnull(io_module->functions);
    }
} _catch:
    أرجع result;
}

M3Result Module_AddFunction (IM3Module io_module, u32 i_typeIndex, IM3ImportInfo i_importInfo)
{
_try {

    u32 index = io_module->عدد_Functions++;
_   (Module_PreallocFunctions(io_module, io_module->عدد_Functions));

    _throwif("type sig index out of bounds", i_typeIndex >= io_module->عدد_Func_Types);

    IM3FuncType ft = io_module->funcTypes[i_typeIndex];

    IM3Function func = Module_GetFunction(io_module, index);
    func->funcType   = ft;
    func->module     = io_module;       // an import has no body, but still belongs to this module

#ifdef DEBUG
    func->index = index;
#endif

    إذا (i_importInfo and func->عدد_Names == 0) {
        func->import   = *i_importInfo;
        func->names[0] = i_importInfo->fieldUtf8;
        func->عدد_Names = 1;
    }

    m3log(module, "   added function: %3d; sig: %d", index, i_typeIndex);

} _catch:
    أرجع result;
}

#ifdef DEBUG
فراغ Module_GenerateNames (IM3Module i_module)
{
    لكل (u32 i = 0; i < i_module->عدد_Functions; ++i) {
        IM3Function func = &i_module->functions[i];

        إذا (func->عدد_Names == 0) {
            حرف* buff = m3_AllocArray(حرف, 16);
            snprintf(buff, 16, "$func%d", i);
            func->names[0] = buff;
            func->عدد_Names = 1;
        }
    }
}
#endif

IM3Function Module_GetFunction (IM3Module i_module, u32 i_functionIndex)
{
    IM3Function func = عدم;

    إذا (i_functionIndex < i_module->عدد_Functions) {
        func = &i_module->functions[i_functionIndex];
        //func->module = i_module;
    }

    أرجع func;
}


ثابت حرف* m3_GetModuleName (IM3Module i_module)
{
    إذا (!i_module || !i_module->name) {
        أرجع ".unnamed";
    }

    أرجع i_module->name;
}

فراغ m3_SetModuleName (IM3Module i_module, ثابت حرف* name)
{
    إذا (i_module) {
        i_module->name = name;
    }
}

IM3Runtime m3_GetModuleRuntime (IM3Module i_module)
{
    أرجع i_module ? i_module->runtime : عدم;
}
