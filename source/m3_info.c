//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ضمن "m3_env.h"
#ضمن "m3_info.h"
#ضمن "m3_compile.h"

#إذا defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (حرف* o_string, size_t i_string_بفر_سائيز, voidptr_t i_sp, m3type_t i_type)
{
    صحيح len = 0;

    *o_string = 0;

    إذا (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_بفر_سائيز, "%" PRIi32, *(i32*)i_sp);
    } وإلا إذا (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_بفر_سائيز, "%" PRIi64, *(i64*)i_sp);
    }
#  إذا d_m3HasFloat
    وإلا إذا (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_بفر_سائيز, "%" PRIf32, *(f32*)i_sp);
    } وإلا إذا (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_بفر_سائيز, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    أرجع len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    صحيح ret;

    ساكن حرف string[256];

    حرف*   s = string;
    ccstr_t e = string + حجم(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    إذا (funcType) {
        u32 انگ_دليلون = funcType->انگ_دليلون;

        لكل (u32 i = 0; i < انگ_دليلون; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[بنياد_Type_جو(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            إذا (i != انگ_دليلون - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } وإلا {
        لکو("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    أرجع string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    صحيح ret;

    ساكن حرف string[256];

    حرف*   s = string;
    ccstr_t e = string + حجم(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 انگ_Rets = i_funcType ? i_funcType->انگ_Rets : 0;

    لكل (u32 i = 0; i < انگ_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        إذا (i != انگ_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    أرجع string;
}

#جيڪڏهن_پورو

#جيڪڏهن_وصف_ٿيل DEBUG

// a central function you can be breakpoint:
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پيغام)
{
    لکو("\nexception: '%s' @ %s\n", i_exception, i_پيغام);
    أرجع;
}


تعريفنوع بنية OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

فراغ m3_PrintM3Info ()
{
    لکو("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    لکو(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_صفحو_سائيز);
    لکو(" sizeof M3Compilation : %zu bytes             \n", حجم(M3Compilation));
    لکو(" sizeof M3Function    : %zu bytes             \n", حجم(M3Function));
    لکو("----------------------------------------------------------------\n\n");
}


فراغ* v_PrintEnvModuleInfo (IM3Module i_module, فراغ* i_info)
{
    u32* io_اشاريو = (u32*)i_info;
    لکو(" module [%u]  name: '%s'; funcs: %d  \n", (*io_اشاريو)++, i_module->name, i_module->انگ_Functions);

    أرجع عدم;
}


فراغ m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    لکو("\n-- m3 runtime -------------------------------------------------\n");

    لکو(" stack-size: %zu   \n\n", i_runtime->انگ_ڍير_Slots * حجم(m3slot_t));

    u32 module_اشاريو = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_اشاريو);

    لکو("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 بنياد = بنياد_Type_جو(i_m3Type);

    إذا (بنياد <= c_m3Type_unknown) {
        أرجع c_waTypes[بنياد];
    } وإلا {
        أرجع "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
ساكن
size_t SPrintAppend (حرف* o_string, size_t i_string_بفر_سائيز, size_t i_len, ccstr_t i_format, ...)
{
    إذا (i_len + 1 >= i_string_بفر_سائيز) {
        أرجع i_len;
    }

    va_list دليلون;
    va_start(دليلون, i_format);
    صحيح ret = vsnprintf(o_string + i_len, i_string_بفر_سائيز - i_len, i_format, دليلون);
    va_end(دليلون);

    أرجع M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_بفر_سائيز - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    ساكن حرف string[256];

    size_t len = SPrintAppend(string, حجم(string), 0, "(");

    لكل (u32 i = 0; i < i_funcType->انگ_دليلون; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, حجم(string), len, ") -> ");

    لكل (u32 i = 0; i < i_funcType->انگ_Rets; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    أرجع string;
}


cstr_t SPrint_قدر (فراغ* i_قدر, m3type_t i_type)
{
    ساكن حرف string[100];
    SPrintArg(string, 100, (m3stack_t)i_قدر, i_type);
    أرجع string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
ساكن
bool جاچ_Operation_جدول (OpInfo* io_info, IM3OpInfo i_جدول, u32 i_انگ_Ops, m3opcode_t i_opcode_بنياد, IM3Operation i_operation)
{
    لكل (u32 i = 0; i < i_انگ_Ops; ++i) {
        IM3OpInfo oi = &i_جدول[i];

        لكل (u32 o = 0; o < 4; ++o) {
            إذا (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_بنياد | i;
                أرجع true;
            }
        }
    }

    أرجع false;
}

ساكن
OpInfo ڳوليو_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { عدم, 0 };

    إذا (!i_operation) {
        أرجع opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    إذا (not جاچ_Operation_جدول(&opInfo, c_operations, c_انگ_Operations, 0, i_operation)) {
        إذا (not جاچ_Operation_جدول(&opInfo, c_operationsFC, c_انگ_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  إذا d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            جاچ_Operation_جدول(&opInfo, c_operationsFE, c_انگ_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    أرجع opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) فراغ Decode_##FUNC (حرف * o_string, size_t i_string_بفر_سائيز, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    فراغ* function    = fetch(فراغ*);
    i32   ڍير_وٿي = fetch(i32);

    snprintf(o_string, i_string_بفر_سائيز, "%p; stack-offset: %d", function, ڍير_وٿي);
}


d_m3Decoder(داخلا_2)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_بفر_سائيز, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    إذا (i_operation == i_opInfo->operations[0]) {
        u32 عمليو = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_بفر_سائيز, "offset= slot:%d + immediate:%d", عمليو, offset);
    }
}


d_m3Decoder(Branch)
{
    فراغ* هدف = fetch(فراغ*);
    snprintf(o_string, i_string_بفر_سائيز, "%p", هدف);
}

d_m3Decoder(Branch_جدول)
{
    u32 جڳهه = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_بفر_سائيز, 0, "slot: %" PRIu32 "; targets: ", جڳهه);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    لكل (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_بفر_سائيز, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_بفر_سائيز, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 قدر  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_بفر_سائيز, " slot [%d] = %" PRIu64, offset, قدر);
}


#  undef fetch

فراغ DecodeOperation (حرف* o_string, size_t i_string_بفر_سائيز, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) حالة OPCODE: Decode_##FUNC (o_string, i_string_بفر_سائيز, i_opcode, i_operation, i_opInfo, o_pc); اكسر;

    بدل (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, داخلا_2)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_جدول, Branch_جدول)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
فراغ dump_ڪوڊ_صفحو (IM3_ڪوڊ_صفحو i_ڪوڊ_صفحو, pc_t i_شروعات_PC)
{
    m3log(ڪوڊ, "code page seq: %d", i_ڪوڊ_صفحو->info.sequence);

    pc_t pc  = i_شروعات_PC ? i_شروعات_PC : Get_صفحو_شروعات_PC(i_ڪوڊ_صفحو);
    pc_t پڇاڙي = Get_صفحو_PC(i_ڪوڊ_صفحو);

    m3log(ڪوڊ, "---------------------------------------------------------------------------------------");

    طالما (pc < پڇاڙي) {
        ثابت pc_t operationPC = pc;
        (فراغ)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = ڳوليو_operation_info(op);

        إذا (i.info) {
            حرف infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, حجم(infoString), i.opcode, op, i.info, &pc);

            m3log(ڪوڊ, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } وإلا {
            m3log(ڪوڊ, "%p | %p", operationPC, op);
        }
    }

    m3log(ڪوڊ, "---------------------------------------------------------------------------------------");

    m3log(ڪوڊ, "free-lines: %d", i_ڪوڊ_صفحو->info.انگ_سٽون - i_ڪوڊ_صفحو->info.سٽ_اشاريو);
}


فراغ dump_type_ڍير (IM3Compilation o)
{
    /* Reminders about how the stack works! :)
     -- args & locals remain on the type stack for duration of the function. Denoted with a constant 'A' and 'L' in this dump.
     -- the initial stack dumps originate from the CompileLocals () function, so these identifiers won't/can't be
     applied until this compilation stage is finished
     -- constants are not statically represented in the type stack (like args & constants) since they don't have/need
     write counts

     -- the number shown for static args and locals (value in wasmStack [i]) represents the write count for the variable

     -- (does Wasm ever write to an arg? I dunno/don't remember.)
     -- the number for the dynamic stack values represents the slot number.
     -- if the slot index points to arg, local or constant it's denoted with a lowercase 'a', 'l' or 'c'

     */

    // for the assert at end of dump:
    i32 regAllocated[2] = { (i32)Is_رجسٽر_Allocated(o, 0), (i32)Is_رجسٽر_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(ڍير, "\n");
    d_m3Log(ڍير, "        ");
    لکو("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    لکو("\n");

    لكل (u32 p = 1; p <= 2; ++p) {
        d_m3Log(ڍير, "        ");

        لكل (u16 i = 0; i < o->ڍير_اشاريو; ++i) {
            إذا (i > 0 and i == o->ڍير_پهريون_Dynamic_اشاريو) {
                لکو("#");
            }

            إذا (i == o->بلاڪ.بلاڪ_ڍير_اشاريو) {
                لکو(">");
            }

            ثابت حرف* type = c_waCompactTypes[بنياد_Type_جو(o->type_ڍير[i])];

            ثابت حرف* location = "";

            i32 جڳهه = o->wasm_ڍير[i];

            إذا (Is_رجسٽر_جڳهه_Alias(جڳهه)) {
                bool isFp = Is_Fp_رجسٽر_جڳهه_Alias(جڳهه);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                جڳهه = -1;
            } وإلا {
                إذا (جڳهه < o->جڳهه_پهريون_Dynamic_اشاريو) {
                    إذا (جڳهه >= o->جڳهه_پهريون_Const_اشاريو) {
                        location = "c";
                    } وإلا إذا (جڳهه >= o->function->انگ_Ret_And_Arg_Slots) {
                        location = "L";
                    } وإلا {
                        location = "a";
                    }
                }
            }

            حرف شي[100];

            إذا (جڳهه >= 0) {
                snprintf(شي, حجم(شي), "%s%s%d", type, location, جڳهه);
            } وإلا {
                snprintf(شي, حجم(شي), "%s%s", type, location);
            }

            إذا (p == 1) {
                size_t s = لڙي_ڊيگهه(شي);

                snprintf(شي, حجم(شي), "%d", i);

                طالما (لڙي_ڊيگهه(شي) < s) {
                    strcat(شي, " ");
                }
            }

            لکو("|%s ", شي);
        }
        لکو("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 max_جڳهه = Get_Max_استعمال_ٿيل_جڳهه_Plus_One(o);

    إذا (max_جڳهه > o->جڳهه_پهريون_Dynamic_اشاريو) {
        d_m3Log(ڍير, "                      -");

        لكل (u16 i = o->جڳهه_پهريون_Dynamic_اشاريو; i < max_جڳهه; ++i) {
            لکو("----");
        }

        لکو("\n");

        d_m3Log(ڍير, "                 slot |");
        لكل (u16 i = o->جڳهه_پهريون_Dynamic_اشاريو; i < max_جڳهه; ++i) {
            لکو("%3d|", i);
        }

        لکو("\n");
        d_m3Log(ڍير, "                alloc |");

        لكل (u16 i = o->جڳهه_پهريون_Dynamic_اشاريو; i < max_جڳهه; ++i) {
            لکو("%3d|", o->m3Slots[i]);
        }

        لکو("\n");
    }
    d_m3Log(ڍير, "\n");
}


ساكن
ثابت حرف* GetOpcodeIndentionString (i32 بلاڪ_اونهائي)
{
    بلاڪ_اونهائي += 1;

    إذا (بلاڪ_اونهائي < 0) {
        بلاڪ_اونهائي = 0;
    }

    ساكن ثابت حرف* s_spaces = ".......................................................................................";

    ثابت حرف* indent = s_spaces + لڙي_ڊيگهه(s_spaces);
    indent -= (بلاڪ_اونهائي * 2);
    إذا (indent < s_spaces) {
        indent = s_spaces;
    }

    أرجع indent;
}


ثابت حرف* get_indention_string (IM3Compilation o)
{
    أرجع GetOpcodeIndentionString(o->بلاڪ.اونهائي + 4);
}


فراغ log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 اونهائي = o->بلاڪ.اونهائي;
    إذا (i_opcode == c_wa_Op_پڇاڙي or i_opcode == c_waOp_else) {
        اونهائي--;
    }
    (فراغ)اونهائي;

    m3log(compile, "%4d | 0x%02x  %s %s", o->انگ_Opcodes++, i_opcode, GetOpcodeIndentionString(اونهائي), GetOpInfo(i_opcode)->name);
}


فراغ log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = ڳوليو_operation_info(i_operation);

    d_m3Log(emit, "");
    إذا (i.info) {
        لکو("%p: %s\n", Get_صفحو_PC(o->صفحو), i.info->name);
    } وإلا {
        لکو("not found: %p\n", i_operation);
    }
}

#جيڪڏهن_پورو // DEBUG


#إذا d_m3EnableOpProfiling

تعريفنوع بنية M3_Profiler_جڳهه {
    cstr_t opName;
    u64    ڪاميابي_ڳڻپ;
} M3_Profiler_جڳهه;

ساكن M3_Profiler_جڳهه s_opProfilerCounts[d_m3_Profiler_جڳهه_نقاب + 1] = {};

فراغ Profile_ڪاميابي (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3_Profiler_جڳهه* جڳهه = &s_opProfilerCounts[ptr & d_m3_Profiler_جڳهه_نقاب];

    إذا (جڳهه->opName) {
        إذا (جڳهه->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    جڳهه->opName = i_operationName;
    جڳهه->ڪاميابي_ڳڻپ++;
}


فراغ m3_PrintProfilerInfo ()
{
    M3_Profiler_جڳهه  dummy;
    M3_Profiler_جڳهه* max_جڳهه = &dummy;

    افعل {
        max_جڳهه->ڪاميابي_ڳڻپ = 0;

        لكل (u32 i = 0; i <= d_m3_Profiler_جڳهه_نقاب; ++i) {
            M3_Profiler_جڳهه* جڳهه = &s_opProfilerCounts[i];

            إذا (جڳهه->opName) {
                إذا (جڳهه->ڪاميابي_ڳڻپ > max_جڳهه->ڪاميابي_ڳڻپ) {
                    max_جڳهه = جڳهه;
                }
            }
        }

        إذا (max_جڳهه->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", max_جڳهه->ڪاميابي_ڳڻپ, max_جڳهه->opName);
            max_جڳهه->opName = عدم;
        }
    } طالما (max_جڳهه->ڪاميابي_ڳڻپ);
}

#وإلا

فراغ m3_PrintProfilerInfo () {}

#جيڪڏهن_پورو
