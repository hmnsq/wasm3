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
    لكل (u32 i = 0; i < i_module->رقم_Functions; ++i) {
        IM3Function func = &i_module->functions[i];
        Function_Release(func);
    }
}


فراغ m3_FreeModule (IM3Module i_module)
{
    إذا (i_module) {
        m3log(module, "freeing module: %s (funcs: %d; segments: %d)",
              i_module->name, i_module->رقم_Functions, i_module->رقم_داده_Segments);

        Module_FreeFunctions(i_module);

        m3_Free(i_module->functions);
        //m3_Free (i_module->imports);
        m3_Free(i_module->funcTypes);
        m3_Free(i_module->داده_Segments);

        لكل (u32 i = 0; i < i_module->رقم_Tables; ++i) {
            IM3_جدول جدول_2 = i_module->tables[i];

            // a slot may point at a table another module owns; that module
            // frees it
            إذا (not جدول_2 or جدول_2->owner != i_module) {
                استمر;
            }

            إذا (جدول_2->elements and i_module->runtime) {
                i_module->runtime->جدول_Elements_استفاده_شده -= جدول_2->اندازه;
            }
            m3_Free(جدول_2->elements);
            FreeImportInfo(&جدول_2->import);
            m3_Free(جدول_2);
        }
        m3_Free(i_module->tables);

        لكل (u32 i = 0; i < i_module->رقم_Element_Segments; ++i) {
            m3_Free(i_module->elementSegments[i].resolved);
        }
        m3_Free(i_module->elementSegments);
        m3_Free(i_module->declaredFuncs);

        لكل (u32 i = 0; i < i_module->رقم_Globals; ++i) {
            FreeImportInfo(&(i_module->globals[i].import));
        }
        m3_Free(i_module->globals);

#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching
        لكل (u32 i = 0; i < i_module->رقم_برچسبها; ++i) {
            FreeImportInfo(&(i_module->برچسبها[i].import));
        }
        m3_Free(i_module->برچسبها);
#پایان_اگر

        لكل (u32 i = 0; i < i_module->رقم_Memories; ++i) {
            IM3_حافظه حافظه = i_module->memories[i];

            // a slot may point at a memory another module owns; that module
            // frees it
            إذا (not حافظه or حافظه->owner != i_module) {
                استمر;
            }

            Free_حافظه_بلوک(حافظه);
            FreeImportInfo(&حافظه->import);
            m3_Free(حافظه);
        }
        m3_Free(i_module->memories);
        Free_حافظه_بلوک(&i_module->خالی_حافظه);

        لكل (u32 i = 0; i < i_module->رقم_Exports; ++i) {
            m3_Free(i_module->exports[i].name);
        }
        m3_Free(i_module->exports);

#إذا d_m3HasSnapshots
        M3EmbeddedSnapshot* snap = i_module->snapshots;
        طالما (snap) {
            M3EmbeddedSnapshot* بعدی = snap->بعدی;
            m3_Free(snap->name);
            m3_Free(snap);
            snap = بعدی;
        }
#پایان_اگر

        m3_Free(i_module);
    }
}


// Supplies the value of an imported global. Unlike m3_SetGlobal this ignores
// mutability: the import's mutability governs what the wasm code may do with it,
// not whether the host may provide it in the first place.
M3_نتیجه m3_LinkGlobal (IM3Module            io_module,
                        ثابت حرف* ثابت    i_moduleName,
                        ثابت حرف* ثابت    i_globalName,
                        ثابت IM3_Tagged_مقدار i_مقدار)
{
    M3_نتیجه نتیجه = m3Err_globalLookupFailed;

    لكل (u32 i = 0; i < io_module->رقم_Globals; ++i) {
        IM3Global g = &io_module->globals[i];

        إذا (not (g->import.moduleUtf8 and g->import.فیلد_Utf8)) {
            استمر;
        }

        إذا (strcmp(g->import.moduleUtf8, i_moduleName) != 0 or
            strcmp(g->import.فیلد_Utf8, i_globalName) != 0) {
            استمر;
        }

        إذا (g->type != i_مقدار->type) {
            أرجع m3Err_globalTypeMismatch;
        }

        بدل (i_مقدار->type) {
        حالة c_m3Type_i32: g->i32_مقدار = i_مقدار->مقدار.i32; اكسر;
        حالة c_m3Type_i64: g->i64_مقدار = i_مقدار->مقدار.i64; اكسر;
#إذا d_m3HasFloat
        حالة c_m3Type_f32: g->f32_مقدار = i_مقدار->مقدار.f32; اكسر;
        حالة c_m3Type_f64: g->f64_مقدار = i_مقدار->مقدار.f64; اكسر;
#پایان_اگر
        افتراضي: أرجع m3Err_invalidTypeId;
        }

        نتیجه = m3Err_none;
    }

    أرجع نتیجه;
}


// The function count is final by the time anything can declare a reference:
// imports and the function section both precede the global, export and element
// sections, and those in turn precede the code section.
M3_نتیجه Module_DeclareFunction (IM3Module io_module, u32 i_شاخص)
{
    M3_نتیجه نتیجه = m3Err_none;

    إذا (i_شاخص >= io_module->رقم_Functions) {
        أرجع "function index out of range";
    }

    إذا (not io_module->declaredFuncs) {
        io_module->declaredFuncs = m3_AllocArray(u8, (io_module->رقم_Functions + 7) / 8);
        _throwifnull(io_module->declaredFuncs);
    }

    io_module->declaredFuncs[i_شاخص / 8] |= (u8)(1u << (i_شاخص % 8));

    _catch: أرجع نتیجه;
}


bool Module_IsFunctionDeclared (IM3Module i_module, u32 i_شاخص)
{
    إذا (not i_module->declaredFuncs or i_شاخص >= i_module->رقم_Functions) {
        أرجع false;
    }

    أرجع (i_module->declaredFuncs[i_شاخص / 8] & (1u << (i_شاخص % 8))) != 0;
}


// Appends an entry to the module's memory index space. The M3Memory is
// allocated separately from the index so that another module importing it can
// hold the same pointer.
M3_نتیجه Module_جمع_کن_حافظه (IM3Module io_module, IM3_حافظه* o_حافظه, ثابت M3_حافظه_Info* i_info, bool i_isImported)
{
    IM3_حافظه حافظه = عدم;
_try {
    u32 index = io_module->رقم_Memories;

    حافظه = m3_AllocStruct(M3_حافظه);
    _throwifnull(حافظه);

    io_module->memories = m3_ReallocArray(IM3_حافظه, io_module->memories, index + 1, index);
    _throwifnull(io_module->memories);

    حافظه->owner      = io_module;
    حافظه->imported   = i_isImported;
    حافظه->init_صفحات  = i_info->init_صفحات;
    حافظه->max_صفحات   = i_info->max_صفحات;
    حافظه->صفحه_اندازه   = i_info->صفحه_اندازه;
    حافظه->hasMax     = i_info->hasMax;
    حافظه->isMemory64 = i_info->isMemory64;
    حافظه->isShared   = i_info->isShared;

    io_module->memories[index] = حافظه;
    io_module->رقم_Memories     = index + 1;

    إذا (o_حافظه) {
        *o_حافظه = حافظه;
    }

    أرجع نتیجه;

} _catch:
    m3_Free(حافظه);
    أرجع نتیجه;
}


// Appends an entry to the module's table index space. Allocated separately from
// the index, like a memory, so another module importing it can hold the same
// pointer.
M3_نتیجه Module_جمع_کن_جدول (IM3Module io_module, IM3_جدول* o_جدول, ثابت M3_جدول_Info* i_info, bool i_isImported)
{
    IM3_جدول جدول_2 = عدم;
_try {
    u32 index = io_module->رقم_Tables;

    جدول_2 = m3_AllocStruct(M3_جدول);
    _throwifnull(جدول_2);

    io_module->tables = m3_ReallocArray(IM3_جدول, io_module->tables, index + 1, index);
    _throwifnull(io_module->tables);

    جدول_2->owner     = io_module;
    جدول_2->imported  = i_isImported;
    جدول_2->type      = i_info->elemType;
    جدول_2->اندازه      = i_info->init_اندازه;
    جدول_2->init_اندازه  = i_info->init_اندازه;
    جدول_2->max_اندازه   = i_info->max_اندازه;
    جدول_2->hasMax    = i_info->hasMax;
    جدول_2->isTable64 = i_info->isTable64;

    io_module->tables[index] = جدول_2;
    io_module->رقم_Tables     = index + 1;

    إذا (o_جدول) {
        *o_جدول = جدول_2;
    }

    أرجع نتیجه;

} _catch:
    m3_Free(جدول_2);
    أرجع نتیجه;
}


#إذا d_m3HasExceptionHandling || d_m3_Has_پشته_Switching

M3_نتیجه Module_جمع_کن_برچسب (IM3Module io_module, IM3_برچسب* o_برچسب, IM3FuncType i_type, bool i_isImported)
{
_try {
    u32 index = io_module->رقم_برچسبها++;

    io_module->برچسبها = m3_ReallocArray(M3_برچسب, io_module->برچسبها, io_module->رقم_برچسبها, index);
    _throwifnull(io_module->برچسبها);
    M3_برچسب* برچسب = &io_module->برچسبها[index];

    برچسب->type     = i_type;
    برچسب->imported = i_isImported;
    برچسب->resolved = عدم;
    memset(&برچسب->import, 0, حجم(برچسب->import));

    إذا (o_برچسب) {
        *o_برچسب = برچسب;
    }

} _catch:
    أرجع نتیجه;
}

#پایان_اگر // d_m3HasExceptionHandling || d_m3HasStackSwitching


#إذا d_m3_Has_پشته_Switching

// The continuation type a spelled-out reference names. Reference types carry
// the canonical index, and the module's own type space is where a name for it
// can be found again - which switch needs, because the type of the peer it
// suspends into is only written down as the last parameter of the type it
// switches to.
IM3FuncType Module_Cont_Type_از_Ref (IM3Module i_module, m3type_t i_refType)
{
    إذا (not IsSpelledRefType(i_refType)) {
        أرجع عدم;
    }

    u32 انباشته = انباشته_Type_از(i_refType);

    إذا (انباشته == d_m3_Type_انباشته_Abstract) {
        أرجع عدم;
    }

    لكل (u32 i = 0; i < i_module->رقم_Func_Types; ++i) {
        IM3FuncType ftype = i_module->funcTypes[i];

        إذا (ftype and ftype->isContinuation and ftype->canonical_شاخص == انباشته) {
            أرجع ftype;
        }
    }

    أرجع عدم;
}

#پایان_اگر // d_m3HasStackSwitching


M3_نتیجه Module_جمع_کن_Global (IM3Module io_module, IM3Global* o_global, m3type_t i_type, bool i_mutable, bool i_isImported)
{
_try {
    u32 index = io_module->رقم_Globals++;

    io_module->globals = m3_ReallocArray(M3Global, io_module->globals, io_module->رقم_Globals, index);
    _throwifnull(io_module->globals);
    M3Global* global = &io_module->globals[index];

    global->type      = i_type;
    global->imported  = i_isImported;
    global->isMutable = i_mutable;

    إذا (o_global) {
        *o_global = global;
    }

} _catch:
    أرجع نتیجه;
}

M3_نتیجه Module_PreallocFunctions (IM3Module io_module, u32 i_مجموع_Functions)
{
_try {
    إذا (i_مجموع_Functions > io_module->allFunctions) {
        io_module->functions    = m3_ReallocArray(M3Function, io_module->functions, i_مجموع_Functions, io_module->allFunctions);
        io_module->allFunctions = i_مجموع_Functions;
        _throwifnull(io_module->functions);
    }
} _catch:
    أرجع نتیجه;
}

M3_نتیجه Module_جمع_کن_Function (IM3Module io_module, u32 i_type_شاخص, IM3ImportInfo i_importInfo)
{
_try {

    u32 index = io_module->رقم_Functions++;
_   (Module_PreallocFunctions(io_module, io_module->رقم_Functions));

    _throwif("type sig index out of bounds", i_type_شاخص >= io_module->رقم_Func_Types);

    IM3FuncType ft = io_module->funcTypes[i_type_شاخص];

    IM3Function func = Module_GetFunction(io_module, index);
    func->funcType   = ft;
    func->module     = io_module;       // an import has no body, but still belongs to this module

#اگر_تعریف_شده DEBUG
    func->index = index;
#پایان_اگر

    إذا (i_importInfo and func->رقم_نامها == 0) {
        func->import   = *i_importInfo;
        func->نامها[0] = i_importInfo->فیلد_Utf8;
        func->رقم_نامها = 1;
    }

    m3log(module, "   added function: %3d; sig: %d", index, i_type_شاخص);

} _catch:
    أرجع نتیجه;
}

#اگر_تعریف_شده DEBUG
فراغ Module_Generate_نامها (IM3Module i_module)
{
    لكل (u32 i = 0; i < i_module->رقم_Functions; ++i) {
        IM3Function func = &i_module->functions[i];

        إذا (func->رقم_نامها == 0) {
            حرف* buff = m3_AllocArray(حرف, 16);
            snprintf(buff, 16, "$func%d", i);
            func->نامها[0] = buff;
            func->رقم_نامها = 1;
        }
    }
}
#پایان_اگر

IM3Function Module_GetFunction (IM3Module i_module, u32 i_function_شاخص)
{
    IM3Function func = عدم;

    إذا (i_function_شاخص < i_module->رقم_Functions) {
        func = &i_module->functions[i_function_شاخص];
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

فراغ m3_مجموعه_Module_Name (IM3Module i_module, ثابت حرف* name)
{
    إذا (i_module) {
        i_module->name = name;
    }
}

IM3Runtime m3_GetModuleRuntime (IM3Module i_module)
{
    أرجع i_module ? i_module->runtime : عدم;
}
