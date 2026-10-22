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

size_t SPrintArg (حرف* o_string, size_t i_string_میانگیر_اندازه, voidptr_t i_sp, m3type_t i_type)
{
    صحيح len = 0;

    *o_string = 0;

    إذا (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_میانگیر_اندازه, "%" PRIi32, *(i32*)i_sp);
    } وإلا إذا (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_میانگیر_اندازه, "%" PRIi64, *(i64*)i_sp);
    }
#  إذا d_m3HasFloat
    وإلا إذا (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_میانگیر_اندازه, "%" PRIf32, *(f32*)i_sp);
    } وإلا إذا (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_میانگیر_اندازه, "%" PRIf64, *(f64*)i_sp);
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
        u32 رقم_آرگومانها = funcType->رقم_آرگومانها;

        لكل (u32 i = 0; i < رقم_آرگومانها; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[پایه_Type_از(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            إذا (i != رقم_آرگومانها - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } وإلا {
        چاپ_کن("null signature");
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

    u32 رقم_Rets = i_funcType ? i_funcType->رقم_Rets : 0;

    لكل (u32 i = 0; i < رقم_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        إذا (i != رقم_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    أرجع string;
}

#پایان_اگر

#اگر_تعریف_شده DEBUG

// a central function you can be breakpoint:
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پیام)
{
    چاپ_کن("\nexception: '%s' @ %s\n", i_exception, i_پیام);
    أرجع;
}


تعريفنوع بنية OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

فراغ m3_PrintM3Info ()
{
    چاپ_کن("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    چاپ_کن(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_صفحه_اندازه);
    چاپ_کن(" sizeof M3Compilation : %zu bytes             \n", حجم(M3Compilation));
    چاپ_کن(" sizeof M3Function    : %zu bytes             \n", حجم(M3Function));
    چاپ_کن("----------------------------------------------------------------\n\n");
}


فراغ* v_PrintEnvModuleInfo (IM3Module i_module, فراغ* i_info)
{
    u32* io_شاخص = (u32*)i_info;
    چاپ_کن(" module [%u]  name: '%s'; funcs: %d  \n", (*io_شاخص)++, i_module->name, i_module->رقم_Functions);

    أرجع عدم;
}


فراغ m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    چاپ_کن("\n-- m3 runtime -------------------------------------------------\n");

    چاپ_کن(" stack-size: %zu   \n\n", i_runtime->رقم_پشته_Slots * حجم(m3slot_t));

    u32 module_شاخص = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_شاخص);

    چاپ_کن("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 پایه = پایه_Type_از(i_m3Type);

    إذا (پایه <= c_m3Type_unknown) {
        أرجع c_waTypes[پایه];
    } وإلا {
        أرجع "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
ساكن
size_t SPrintAppend (حرف* o_string, size_t i_string_میانگیر_اندازه, size_t i_len, ccstr_t i_format, ...)
{
    إذا (i_len + 1 >= i_string_میانگیر_اندازه) {
        أرجع i_len;
    }

    va_list آرگومانها;
    va_start(آرگومانها, i_format);
    صحيح ret = vsnprintf(o_string + i_len, i_string_میانگیر_اندازه - i_len, i_format, آرگومانها);
    va_end(آرگومانها);

    أرجع M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_میانگیر_اندازه - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    ساكن حرف string[256];

    size_t len = SPrintAppend(string, حجم(string), 0, "(");

    لكل (u32 i = 0; i < i_funcType->رقم_آرگومانها; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, حجم(string), len, ") -> ");

    لكل (u32 i = 0; i < i_funcType->رقم_Rets; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    أرجع string;
}


cstr_t SPrint_مقدار (فراغ* i_مقدار, m3type_t i_type)
{
    ساكن حرف string[100];
    SPrintArg(string, 100, (m3stack_t)i_مقدار, i_type);
    أرجع string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
ساكن
bool پیمایش_Operation_جدول (OpInfo* io_info, IM3OpInfo i_جدول, u32 i_رقم_Ops, m3opcode_t i_opcode_پایه, IM3Operation i_operation)
{
    لكل (u32 i = 0; i < i_رقم_Ops; ++i) {
        IM3OpInfo oi = &i_جدول[i];

        لكل (u32 o = 0; o < 4; ++o) {
            إذا (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_پایه | i;
                أرجع true;
            }
        }
    }

    أرجع false;
}

ساكن
OpInfo پیدا_کن_operation_info (IM3Operation i_operation)
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
    إذا (not پیمایش_Operation_جدول(&opInfo, c_operations, c_رقم_Operations, 0, i_operation)) {
        إذا (not پیمایش_Operation_جدول(&opInfo, c_operationsFC, c_رقم_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  إذا d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            پیمایش_Operation_جدول(&opInfo, c_operationsFE, c_رقم_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    أرجع opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) فراغ Decode_##FUNC (حرف * o_string, size_t i_string_میانگیر_اندازه, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    فراغ* function    = fetch(فراغ*);
    i32   پشته_جابه_جایی = fetch(i32);

    snprintf(o_string, i_string_میانگیر_اندازه, "%p; stack-offset: %d", function, پشته_جابه_جایی);
}


d_m3Decoder(مدخل_2)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_میانگیر_اندازه, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    إذا (i_operation == i_opInfo->operations[0]) {
        u32 عملوند = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_میانگیر_اندازه, "offset= slot:%d + immediate:%d", عملوند, offset);
    }
}


d_m3Decoder(Branch)
{
    فراغ* هدف = fetch(فراغ*);
    snprintf(o_string, i_string_میانگیر_اندازه, "%p", هدف);
}

d_m3Decoder(Branch_جدول)
{
    u32 جایگاه = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_میانگیر_اندازه, 0, "slot: %" PRIu32 "; targets: ", جایگاه);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    لكل (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_میانگیر_اندازه, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_میانگیر_اندازه, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 مقدار  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_میانگیر_اندازه, " slot [%d] = %" PRIu64, offset, مقدار);
}


#  undef fetch

فراغ DecodeOperation (حرف* o_string, size_t i_string_میانگیر_اندازه, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) حالة OPCODE: Decode_##FUNC (o_string, i_string_میانگیر_اندازه, i_opcode, i_operation, i_opInfo, o_pc); اكسر;

    بدل (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, مدخل_2)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_جدول, Branch_جدول)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
فراغ dump_کد_صفحه (IM3_کد_صفحه i_کد_صفحه, pc_t i_شروع_PC)
{
    m3log(کد, "code page seq: %d", i_کد_صفحه->info.sequence);

    pc_t pc  = i_شروع_PC ? i_شروع_PC : Get_صفحه_شروع_PC(i_کد_صفحه);
    pc_t انتها = Get_صفحه_PC(i_کد_صفحه);

    m3log(کد, "---------------------------------------------------------------------------------------");

    طالما (pc < انتها) {
        ثابت pc_t operationPC = pc;
        (فراغ)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = پیدا_کن_operation_info(op);

        إذا (i.info) {
            حرف infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, حجم(infoString), i.opcode, op, i.info, &pc);

            m3log(کد, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } وإلا {
            m3log(کد, "%p | %p", operationPC, op);
        }
    }

    m3log(کد, "---------------------------------------------------------------------------------------");

    m3log(کد, "free-lines: %d", i_کد_صفحه->info.رقم_سطرها - i_کد_صفحه->info.سطر_شاخص);
}


فراغ dump_type_پشته (IM3Compilation o)
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
    i32 regAllocated[2] = { (i32)Is_ثبات_Allocated(o, 0), (i32)Is_ثبات_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(پشته, "\n");
    d_m3Log(پشته, "        ");
    چاپ_کن("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    چاپ_کن("\n");

    لكل (u32 p = 1; p <= 2; ++p) {
        d_m3Log(پشته, "        ");

        لكل (u16 i = 0; i < o->پشته_شاخص; ++i) {
            إذا (i > 0 and i == o->پشته_اولی_Dynamic_شاخص) {
                چاپ_کن("#");
            }

            إذا (i == o->بلوک.بلوک_پشته_شاخص) {
                چاپ_کن(">");
            }

            ثابت حرف* type = c_waCompactTypes[پایه_Type_از(o->type_پشته[i])];

            ثابت حرف* location = "";

            i32 جایگاه = o->wasm_پشته[i];

            إذا (Is_ثبات_جایگاه_Alias(جایگاه)) {
                bool isFp = Is_Fp_ثبات_جایگاه_Alias(جایگاه);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                جایگاه = -1;
            } وإلا {
                إذا (جایگاه < o->جایگاه_اولی_Dynamic_شاخص) {
                    إذا (جایگاه >= o->جایگاه_اولی_Const_شاخص) {
                        location = "c";
                    } وإلا إذا (جایگاه >= o->function->رقم_Ret_And_Arg_Slots) {
                        location = "L";
                    } وإلا {
                        location = "a";
                    }
                }
            }

            حرف قلم[100];

            إذا (جایگاه >= 0) {
                snprintf(قلم, حجم(قلم), "%s%s%d", type, location, جایگاه);
            } وإلا {
                snprintf(قلم, حجم(قلم), "%s%s", type, location);
            }

            إذا (p == 1) {
                size_t s = طول_رشته(قلم);

                snprintf(قلم, حجم(قلم), "%d", i);

                طالما (طول_رشته(قلم) < s) {
                    strcat(قلم, " ");
                }
            }

            چاپ_کن("|%s ", قلم);
        }
        چاپ_کن("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 max_جایگاه = Get_Max_استفاده_شده_جایگاه_Plus_One(o);

    إذا (max_جایگاه > o->جایگاه_اولی_Dynamic_شاخص) {
        d_m3Log(پشته, "                      -");

        لكل (u16 i = o->جایگاه_اولی_Dynamic_شاخص; i < max_جایگاه; ++i) {
            چاپ_کن("----");
        }

        چاپ_کن("\n");

        d_m3Log(پشته, "                 slot |");
        لكل (u16 i = o->جایگاه_اولی_Dynamic_شاخص; i < max_جایگاه; ++i) {
            چاپ_کن("%3d|", i);
        }

        چاپ_کن("\n");
        d_m3Log(پشته, "                alloc |");

        لكل (u16 i = o->جایگاه_اولی_Dynamic_شاخص; i < max_جایگاه; ++i) {
            چاپ_کن("%3d|", o->m3Slots[i]);
        }

        چاپ_کن("\n");
    }
    d_m3Log(پشته, "\n");
}


ساكن
ثابت حرف* GetOpcodeIndentionString (i32 بلوک_عمق)
{
    بلوک_عمق += 1;

    إذا (بلوک_عمق < 0) {
        بلوک_عمق = 0;
    }

    ساكن ثابت حرف* s_spaces = ".......................................................................................";

    ثابت حرف* indent = s_spaces + طول_رشته(s_spaces);
    indent -= (بلوک_عمق * 2);
    إذا (indent < s_spaces) {
        indent = s_spaces;
    }

    أرجع indent;
}


ثابت حرف* get_indention_string (IM3Compilation o)
{
    أرجع GetOpcodeIndentionString(o->بلوک.عمق + 4);
}


فراغ log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 عمق = o->بلوک.عمق;
    إذا (i_opcode == c_wa_Op_انتها or i_opcode == c_waOp_else) {
        عمق--;
    }
    (فراغ)عمق;

    m3log(compile, "%4d | 0x%02x  %s %s", o->رقم_Opcodes++, i_opcode, GetOpcodeIndentionString(عمق), GetOpInfo(i_opcode)->name);
}


فراغ log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = پیدا_کن_operation_info(i_operation);

    d_m3Log(emit, "");
    إذا (i.info) {
        چاپ_کن("%p: %s\n", Get_صفحه_PC(o->صفحه), i.info->name);
    } وإلا {
        چاپ_کن("not found: %p\n", i_operation);
    }
}

#پایان_اگر // DEBUG


#إذا d_m3EnableOpProfiling

تعريفنوع بنية M3_Profiler_جایگاه {
    cstr_t opName;
    u64    اصابت_شمار;
} M3_Profiler_جایگاه;

ساكن M3_Profiler_جایگاه s_opProfilerCounts[d_m3_Profiler_جایگاه_ماسک + 1] = {};

فراغ Profile_اصابت (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3_Profiler_جایگاه* جایگاه = &s_opProfilerCounts[ptr & d_m3_Profiler_جایگاه_ماسک];

    إذا (جایگاه->opName) {
        إذا (جایگاه->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    جایگاه->opName = i_operationName;
    جایگاه->اصابت_شمار++;
}


فراغ m3_PrintProfilerInfo ()
{
    M3_Profiler_جایگاه  dummy;
    M3_Profiler_جایگاه* max_جایگاه = &dummy;

    افعل {
        max_جایگاه->اصابت_شمار = 0;

        لكل (u32 i = 0; i <= d_m3_Profiler_جایگاه_ماسک; ++i) {
            M3_Profiler_جایگاه* جایگاه = &s_opProfilerCounts[i];

            إذا (جایگاه->opName) {
                إذا (جایگاه->اصابت_شمار > max_جایگاه->اصابت_شمار) {
                    max_جایگاه = جایگاه;
                }
            }
        }

        إذا (max_جایگاه->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", max_جایگاه->اصابت_شمار, max_جایگاه->opName);
            max_جایگاه->opName = عدم;
        }
    } طالما (max_جایگاه->اصابت_شمار);
}

#وإلا

فراغ m3_PrintProfilerInfo () {}

#پایان_اگر
