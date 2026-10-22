//
//  m3_module.c
//
//  Created by Steven Massey on 5/7/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#அந்தர்பாவ "m3_env.h"
#அந்தர்பாவ "m3_exception.h"


வெறுமை Module_FreeFunctions (IM3Module i_module)
{
    க்கு (u32 i = 0; i < i_module->இலக்கம்_Functions; ++i) {
        IM3Function func = &i_module->functions[i];
        Function_Release(func);
    }
}


வெறுமை m3_FreeModule (IM3Module i_module)
{
    எனில் (i_module) {
        m3log(module, "freeing module: %s (funcs: %d; segments: %d)",
              i_module->name, i_module->இலக்கம்_Functions, i_module->இலக்கம்_Data_Segments);

        Module_FreeFunctions(i_module);

        m3_Free(i_module->functions);
        //m3_Free (i_module->imports);
        m3_Free(i_module->funcTypes);
        m3_Free(i_module->dataSegments);

        க்கு (u32 i = 0; i < i_module->இலக்கம்_Tables; ++i) {
            IM3Table table = i_module->tables[i];

            // a slot may point at a table another module owns; that module
            // frees it
            எனில் (not table or table->owner != i_module) {
                தொடர்;
            }

            எனில் (table->elements and i_module->runtime) {
                i_module->runtime->tableElementsUsed -= table->size;
            }
            m3_Free(table->elements);
            FreeImportInfo(&table->import);
            m3_Free(table);
        }
        m3_Free(i_module->tables);

        க்கு (u32 i = 0; i < i_module->இலக்கம்_Element_Segments; ++i) {
            m3_Free(i_module->elementSegments[i].resolved);
        }
        m3_Free(i_module->elementSegments);
        m3_Free(i_module->declaredFuncs);

        க்கு (u32 i = 0; i < i_module->இலக்கம்_Globals; ++i) {
            FreeImportInfo(&(i_module->globals[i].import));
        }
        m3_Free(i_module->globals);

#எனில் d_m3HasExceptionHandling || d_m3_Has_அடுக்கு_Switching
        க்கு (u32 i = 0; i < i_module->இலக்கம்_Tags; ++i) {
            FreeImportInfo(&(i_module->tags[i].import));
        }
        m3_Free(i_module->tags);
#endif

        க்கு (u32 i = 0; i < i_module->இலக்கம்_Memories; ++i) {
            IM3Memory memory = i_module->memories[i];

            // a slot may point at a memory another module owns; that module
            // frees it
            எனில் (not memory or memory->owner != i_module) {
                தொடர்;
            }

            FreeMemoryBlock(memory);
            FreeImportInfo(&memory->import);
            m3_Free(memory);
        }
        m3_Free(i_module->memories);
        FreeMemoryBlock(&i_module->emptyMemory);

        க்கு (u32 i = 0; i < i_module->இலக்கம்_Exports; ++i) {
            m3_Free(i_module->exports[i].name);
        }
        m3_Free(i_module->exports);

#எனில் d_m3HasSnapshots
        M3EmbeddedSnapshot* snap = i_module->snapshots;
        போது (snap) {
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
                        மாறிலி எழுத்து* மாறிலி    i_moduleName,
                        மாறிலி எழுத்து* மாறிலி    i_globalName,
                        மாறிலி IM3_Tagged_மதிப்பு i_மதிப்பு)
{
    M3Result result = m3Err_globalLookupFailed;

    க்கு (u32 i = 0; i < io_module->இலக்கம்_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        எனில் (not (g->import.moduleUtf8 and g->import.fieldUtf8)) {
            தொடர்;
        }

        எனில் (strcmp(g->import.moduleUtf8, i_moduleName) != 0 or
            strcmp(g->import.fieldUtf8, i_globalName) != 0) {
            தொடர்;
        }

        எனில் (g->type != i_மதிப்பு->type) {
            திரும்பு m3Err_globalTypeMismatch;
        }

        தேர்வு (i_மதிப்பு->type) {
        நிலை c_m3Type_i32: g->i32_மதிப்பு = i_மதிப்பு->மதிப்பு.i32; நிறுத்து;
        நிலை c_m3Type_i64: g->i64_மதிப்பு = i_மதிப்பு->மதிப்பு.i64; நிறுத்து;
#எனில் d_m3HasFloat
        நிலை c_m3Type_f32: g->f32_மதிப்பு = i_மதிப்பு->மதிப்பு.f32; நிறுத்து;
        நிலை c_m3Type_f64: g->f64_மதிப்பு = i_மதிப்பு->மதிப்பு.f64; நிறுத்து;
#endif
        இயல்பு: திரும்பு m3Err_invalidTypeId;
        }

        result = m3Err_none;
    }

    திரும்பு result;
}


// The function count is final by the time anything can declare a reference:
// imports and the function section both precede the global, export and element
// sections, and those in turn precede the code section.
M3Result Module_DeclareFunction (IM3Module io_module, u32 i_index)
{
    M3Result result = m3Err_none;

    எனில் (i_index >= io_module->இலக்கம்_Functions) {
        திரும்பு "function index out of range";
    }

    எனில் (not io_module->declaredFuncs) {
        io_module->declaredFuncs = m3_AllocArray(u8, (io_module->இலக்கம்_Functions + 7) / 8);
        _throwifnull(io_module->declaredFuncs);
    }

    io_module->declaredFuncs[i_index / 8] |= (u8)(1u << (i_index % 8));

    _catch: திரும்பு result;
}


bool Module_IsFunctionDeclared (IM3Module i_module, u32 i_index)
{
    எனில் (not i_module->declaredFuncs or i_index >= i_module->இலக்கம்_Functions) {
        திரும்பு false;
    }

    திரும்பு (i_module->declaredFuncs[i_index / 8] & (1u << (i_index % 8))) != 0;
}


// Appends an entry to the module's memory index space. The M3Memory is
// allocated separately from the index so that another module importing it can
// hold the same pointer.
M3Result Module_AddMemory (IM3Module io_module, IM3Memory* o_memory, மாறிலி M3MemoryInfo* i_info, bool i_isImported)
{
    IM3Memory memory = வெற்று;
_try {
    u32 index = io_module->இலக்கம்_Memories;

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
    io_module->இலக்கம்_Memories     = index + 1;

    எனில் (o_memory) {
        *o_memory = memory;
    }

    திரும்பு result;

} _catch:
    m3_Free(memory);
    திரும்பு result;
}


// Appends an entry to the module's table index space. Allocated separately from
// the index, like a memory, so another module importing it can hold the same
// pointer.
M3Result Module_AddTable (IM3Module io_module, IM3Table* o_table, மாறிலி M3TableInfo* i_info, bool i_isImported)
{
    IM3Table table = வெற்று;
_try {
    u32 index = io_module->இலக்கம்_Tables;

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
    io_module->இலக்கம்_Tables     = index + 1;

    எனில் (o_table) {
        *o_table = table;
    }

    திரும்பு result;

} _catch:
    m3_Free(table);
    திரும்பு result;
}


#எனில் d_m3HasExceptionHandling || d_m3_Has_அடுக்கு_Switching

M3Result Module_AddTag (IM3Module io_module, IM3Tag* o_tag, IM3FuncType i_type, bool i_isImported)
{
_try {
    u32 index = io_module->இலக்கம்_Tags++;

    io_module->tags = m3_ReallocArray(M3Tag, io_module->tags, io_module->இலக்கம்_Tags, index);
    _throwifnull(io_module->tags);
    M3Tag* tag = &io_module->tags[index];

    tag->type     = i_type;
    tag->imported = i_isImported;
    tag->resolved = வெற்று;
    memset(&tag->import, 0, அளவு(tag->import));

    எனில் (o_tag) {
        *o_tag = tag;
    }

} _catch:
    திரும்பு result;
}

#endif // d_m3HasExceptionHandling || d_m3HasStackSwitching


#எனில் d_m3_Has_அடுக்கு_Switching

// The continuation type a spelled-out reference names. Reference types carry
// the canonical index, and the module's own type space is where a name for it
// can be found again - which switch needs, because the type of the peer it
// suspends into is only written down as the last parameter of the type it
// switches to.
IM3FuncType Module_ContTypeOfRef (IM3Module i_module, m3type_t i_refType)
{
    எனில் (not IsSpelledRefType(i_refType)) {
        திரும்பு வெற்று;
    }

    u32 heap = HeapTypeOf(i_refType);

    எனில் (heap == d_m3Type_heapAbstract) {
        திரும்பு வெற்று;
    }

    க்கு (u32 i = 0; i < i_module->இலக்கம்_Func_Types; ++i) {
        IM3FuncType ftype = i_module->funcTypes[i];

        எனில் (ftype and ftype->isContinuation and ftype->canonicalIndex == heap) {
            திரும்பு ftype;
        }
    }

    திரும்பு வெற்று;
}

#endif // d_m3HasStackSwitching


M3Result Module_AddGlobal (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported)
{
_try {
    u32 index = io_module->இலக்கம்_Globals++;

    io_module->globals = m3_ReallocArray(M3Global, io_module->globals, io_module->இலக்கம்_Globals, index);
    _throwifnull(io_module->globals);
    M3Global* global = &io_module->globals[index];

    global->type      = i_type;
    global->imported  = i_isImported;
    global->isMutable = i_mutable;

    எனில் (o_global) {
        *o_global = global;
    }

} _catch:
    திரும்பு result;
}

M3Result Module_PreallocFunctions (IM3Module io_module, u32 i_totalFunctions)
{
_try {
    எனில் (i_totalFunctions > io_module->allFunctions) {
        io_module->functions    = m3_ReallocArray(M3Function, io_module->functions, i_totalFunctions, io_module->allFunctions);
        io_module->allFunctions = i_totalFunctions;
        _throwifnull(io_module->functions);
    }
} _catch:
    திரும்பு result;
}

M3Result Module_AddFunction (IM3Module io_module, u32 i_typeIndex, IM3ImportInfo i_importInfo)
{
_try {

    u32 index = io_module->இலக்கம்_Functions++;
_   (Module_PreallocFunctions(io_module, io_module->இலக்கம்_Functions));

    _throwif("type sig index out of bounds", i_typeIndex >= io_module->இலக்கம்_Func_Types);

    IM3FuncType ft = io_module->funcTypes[i_typeIndex];

    IM3Function func = Module_GetFunction(io_module, index);
    func->funcType   = ft;
    func->module     = io_module;       // an import has no body, but still belongs to this module

#ifdef DEBUG
    func->index = index;
#endif

    எனில் (i_importInfo and func->இலக்கம்_Names == 0) {
        func->import   = *i_importInfo;
        func->names[0] = i_importInfo->fieldUtf8;
        func->இலக்கம்_Names = 1;
    }

    m3log(module, "   added function: %3d; sig: %d", index, i_typeIndex);

} _catch:
    திரும்பு result;
}

#ifdef DEBUG
வெறுமை Module_GenerateNames (IM3Module i_module)
{
    க்கு (u32 i = 0; i < i_module->இலக்கம்_Functions; ++i) {
        IM3Function func = &i_module->functions[i];

        எனில் (func->இலக்கம்_Names == 0) {
            எழுத்து* buff = m3_AllocArray(எழுத்து, 16);
            snprintf(buff, 16, "$func%d", i);
            func->names[0] = buff;
            func->இலக்கம்_Names = 1;
        }
    }
}
#endif

IM3Function Module_GetFunction (IM3Module i_module, u32 i_functionIndex)
{
    IM3Function func = வெற்று;

    எனில் (i_functionIndex < i_module->இலக்கம்_Functions) {
        func = &i_module->functions[i_functionIndex];
        //func->module = i_module;
    }

    திரும்பு func;
}


மாறிலி எழுத்து* m3_GetModuleName (IM3Module i_module)
{
    எனில் (!i_module || !i_module->name) {
        திரும்பு ".unnamed";
    }

    திரும்பு i_module->name;
}

வெறுமை m3_SetModuleName (IM3Module i_module, மாறிலி எழுத்து* name)
{
    எனில் (i_module) {
        i_module->name = name;
    }
}

IM3Runtime m3_GetModuleRuntime (IM3Module i_module)
{
    திரும்பு i_module ? i_module->runtime : வெற்று;
}
