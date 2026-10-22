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

size_t SPrintArg (حرف* o_string, size_t i_string_بفر_اندازه, voidptr_t i_sp, m3type_t i_type)
{
    صحيح len = 0;

    *o_string = 0;

    إذا (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_بفر_اندازه, "%" PRIi32, *(i32*)i_sp);
    } وإلا إذا (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_بفر_اندازه, "%" PRIi64, *(i64*)i_sp);
    }
#  إذا d_m3HasFloat
    وإلا إذا (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_بفر_اندازه, "%" PRIf32, *(f32*)i_sp);
    } وإلا إذا (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_بفر_اندازه, "%" PRIf64, *(f64*)i_sp);
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
        u32 عدد_دلیلونه = funcType->عدد_دلیلونه;

        لكل (u32 i = 0; i < عدد_دلیلونه; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[بنسټ_Type_د(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            إذا (i != عدد_دلیلونه - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } وإلا {
        چاپ_کړه("null signature");
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

    u32 عدد_Rets = i_funcType ? i_funcType->عدد_Rets : 0;

    لكل (u32 i = 0; i < عدد_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        إذا (i != عدد_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    أرجع string;
}

#د_که_پای

#که_تعریف_شوی DEBUG

// a central function you can be breakpoint:
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پیغام)
{
    چاپ_کړه("\nexception: '%s' @ %s\n", i_exception, i_پیغام);
    أرجع;
}


تعريفنوع بنية OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

فراغ m3_PrintM3Info ()
{
    چاپ_کړه("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    چاپ_کړه(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_پاڼه_اندازه);
    چاپ_کړه(" sizeof M3Compilation : %zu bytes             \n", حجم(M3Compilation));
    چاپ_کړه(" sizeof M3Function    : %zu bytes             \n", حجم(M3Function));
    چاپ_کړه("----------------------------------------------------------------\n\n");
}


فراغ* v_PrintEnvModuleInfo (IM3Module i_module, فراغ* i_info)
{
    u32* io_شاخص = (u32*)i_info;
    چاپ_کړه(" module [%u]  name: '%s'; funcs: %d  \n", (*io_شاخص)++, i_module->name, i_module->عدد_Functions);

    أرجع عدم;
}


فراغ m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    چاپ_کړه("\n-- m3 runtime -------------------------------------------------\n");

    چاپ_کړه(" stack-size: %zu   \n\n", i_runtime->عدد_ډېری_Slots * حجم(m3slot_t));

    u32 module_شاخص = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_شاخص);

    چاپ_کړه("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 بنسټ = بنسټ_Type_د(i_m3Type);

    إذا (بنسټ <= c_m3Type_unknown) {
        أرجع c_waTypes[بنسټ];
    } وإلا {
        أرجع "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
ساكن
size_t SPrintAppend (حرف* o_string, size_t i_string_بفر_اندازه, size_t i_len, ccstr_t i_format, ...)
{
    إذا (i_len + 1 >= i_string_بفر_اندازه) {
        أرجع i_len;
    }

    va_list دلیلونه;
    va_start(دلیلونه, i_format);
    صحيح ret = vsnprintf(o_string + i_len, i_string_بفر_اندازه - i_len, i_format, دلیلونه);
    va_end(دلیلونه);

    أرجع M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_بفر_اندازه - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    ساكن حرف string[256];

    size_t len = SPrintAppend(string, حجم(string), 0, "(");

    لكل (u32 i = 0; i < i_funcType->عدد_دلیلونه; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, حجم(string), len, ") -> ");

    لكل (u32 i = 0; i < i_funcType->عدد_Rets; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    أرجع string;
}


cstr_t SPrint_ارزښت (فراغ* i_ارزښت, m3type_t i_type)
{
    ساكن حرف string[100];
    SPrintArg(string, 100, (m3stack_t)i_ارزښت, i_type);
    أرجع string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
ساكن
bool لټون_Operation_جدول (OpInfo* io_info, IM3OpInfo i_جدول, u32 i_عدد_Ops, m3opcode_t i_opcode_بنسټ, IM3Operation i_operation)
{
    لكل (u32 i = 0; i < i_عدد_Ops; ++i) {
        IM3OpInfo oi = &i_جدول[i];

        لكل (u32 o = 0; o < 4; ++o) {
            إذا (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_بنسټ | i;
                أرجع true;
            }
        }
    }

    أرجع false;
}

ساكن
OpInfo پیدا_کړه_operation_info (IM3Operation i_operation)
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
    إذا (not لټون_Operation_جدول(&opInfo, c_operations, c_عدد_Operations, 0, i_operation)) {
        إذا (not لټون_Operation_جدول(&opInfo, c_operationsFC, c_عدد_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  إذا d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            لټون_Operation_جدول(&opInfo, c_operationsFE, c_عدد_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    أرجع opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) فراغ Decode_##FUNC (حرف * o_string, size_t i_string_بفر_اندازه, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    فراغ* function    = fetch(فراغ*);
    i32   ډېری_دننه_واټن = fetch(i32);

    snprintf(o_string, i_string_بفر_اندازه, "%p; stack-offset: %d", function, ډېری_دننه_واټن);
}


d_m3Decoder(ثبت_2)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_بفر_اندازه, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    إذا (i_operation == i_opInfo->operations[0]) {
        u32 عملوند = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_بفر_اندازه, "offset= slot:%d + immediate:%d", عملوند, offset);
    }
}


d_m3Decoder(Branch)
{
    فراغ* موخه = fetch(فراغ*);
    snprintf(o_string, i_string_بفر_اندازه, "%p", موخه);
}

d_m3Decoder(Branch_جدول)
{
    u32 خونه = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_بفر_اندازه, 0, "slot: %" PRIu32 "; targets: ", خونه);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    لكل (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_بفر_اندازه, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_بفر_اندازه, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 ارزښت  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_بفر_اندازه, " slot [%d] = %" PRIu64, offset, ارزښت);
}


#  undef fetch

فراغ DecodeOperation (حرف* o_string, size_t i_string_بفر_اندازه, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) حالة OPCODE: Decode_##FUNC (o_string, i_string_بفر_اندازه, i_opcode, i_operation, i_opInfo, o_pc); اكسر;

    بدل (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, ثبت_2)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_جدول, Branch_جدول)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
فراغ dump_کوډ_پاڼه (IM3_کوډ_پاڼه i_کوډ_پاڼه, pc_t i_پیل_PC)
{
    m3log(کوډ, "code page seq: %d", i_کوډ_پاڼه->info.sequence);

    pc_t pc  = i_پیل_PC ? i_پیل_PC : Get_پاڼه_پیل_PC(i_کوډ_پاڼه);
    pc_t پای_ټکی = Get_پاڼه_PC(i_کوډ_پاڼه);

    m3log(کوډ, "---------------------------------------------------------------------------------------");

    طالما (pc < پای_ټکی) {
        ثابت pc_t operationPC = pc;
        (فراغ)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = پیدا_کړه_operation_info(op);

        إذا (i.info) {
            حرف infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, حجم(infoString), i.opcode, op, i.info, &pc);

            m3log(کوډ, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } وإلا {
            m3log(کوډ, "%p | %p", operationPC, op);
        }
    }

    m3log(کوډ, "---------------------------------------------------------------------------------------");

    m3log(کوډ, "free-lines: %d", i_کوډ_پاڼه->info.عدد_کرښې - i_کوډ_پاڼه->info.کرښه_شاخص);
}


فراغ dump_type_ډېری (IM3Compilation o)
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
    i32 regAllocated[2] = { (i32)Is_ثبتوونکی_Allocated(o, 0), (i32)Is_ثبتوونکی_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(ډېری, "\n");
    d_m3Log(ډېری, "        ");
    چاپ_کړه("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    چاپ_کړه("\n");

    لكل (u32 p = 1; p <= 2; ++p) {
        d_m3Log(ډېری, "        ");

        لكل (u16 i = 0; i < o->ډېری_شاخص; ++i) {
            إذا (i > 0 and i == o->ډېری_لومړی_Dynamic_شاخص) {
                چاپ_کړه("#");
            }

            إذا (i == o->بلاک.بلاک_ډېری_شاخص) {
                چاپ_کړه(">");
            }

            ثابت حرف* type = c_waCompactTypes[بنسټ_Type_د(o->type_ډېری[i])];

            ثابت حرف* location = "";

            i32 خونه = o->wasm_ډېری[i];

            إذا (Is_ثبتوونکی_خونه_Alias(خونه)) {
                bool isFp = Is_Fp_ثبتوونکی_خونه_Alias(خونه);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                خونه = -1;
            } وإلا {
                إذا (خونه < o->خونه_لومړی_Dynamic_شاخص) {
                    إذا (خونه >= o->خونه_لومړی_Const_شاخص) {
                        location = "c";
                    } وإلا إذا (خونه >= o->function->عدد_Ret_And_Arg_Slots) {
                        location = "L";
                    } وإلا {
                        location = "a";
                    }
                }
            }

            حرف توکی[100];

            إذا (خونه >= 0) {
                snprintf(توکی, حجم(توکی), "%s%s%d", type, location, خونه);
            } وإلا {
                snprintf(توکی, حجم(توکی), "%s%s", type, location);
            }

            إذا (p == 1) {
                size_t s = مزي_اوږدوالی(توکی);

                snprintf(توکی, حجم(توکی), "%d", i);

                طالما (مزي_اوږدوالی(توکی) < s) {
                    strcat(توکی, " ");
                }
            }

            چاپ_کړه("|%s ", توکی);
        }
        چاپ_کړه("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 max_خونه = Get_Max_کارول_شوی_خونه_Plus_One(o);

    إذا (max_خونه > o->خونه_لومړی_Dynamic_شاخص) {
        d_m3Log(ډېری, "                      -");

        لكل (u16 i = o->خونه_لومړی_Dynamic_شاخص; i < max_خونه; ++i) {
            چاپ_کړه("----");
        }

        چاپ_کړه("\n");

        d_m3Log(ډېری, "                 slot |");
        لكل (u16 i = o->خونه_لومړی_Dynamic_شاخص; i < max_خونه; ++i) {
            چاپ_کړه("%3d|", i);
        }

        چاپ_کړه("\n");
        d_m3Log(ډېری, "                alloc |");

        لكل (u16 i = o->خونه_لومړی_Dynamic_شاخص; i < max_خونه; ++i) {
            چاپ_کړه("%3d|", o->m3Slots[i]);
        }

        چاپ_کړه("\n");
    }
    d_m3Log(ډېری, "\n");
}


ساكن
ثابت حرف* GetOpcodeIndentionString (i32 بلاک_ژوروالی)
{
    بلاک_ژوروالی += 1;

    إذا (بلاک_ژوروالی < 0) {
        بلاک_ژوروالی = 0;
    }

    ساكن ثابت حرف* s_spaces = ".......................................................................................";

    ثابت حرف* indent = s_spaces + مزي_اوږدوالی(s_spaces);
    indent -= (بلاک_ژوروالی * 2);
    إذا (indent < s_spaces) {
        indent = s_spaces;
    }

    أرجع indent;
}


ثابت حرف* get_indention_string (IM3Compilation o)
{
    أرجع GetOpcodeIndentionString(o->بلاک.ژوروالی + 4);
}


فراغ log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 ژوروالی = o->بلاک.ژوروالی;
    إذا (i_opcode == c_wa_Op_پای_ټکی or i_opcode == c_waOp_else) {
        ژوروالی--;
    }
    (فراغ)ژوروالی;

    m3log(compile, "%4d | 0x%02x  %s %s", o->عدد_Opcodes++, i_opcode, GetOpcodeIndentionString(ژوروالی), GetOpInfo(i_opcode)->name);
}


فراغ log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = پیدا_کړه_operation_info(i_operation);

    d_m3Log(emit, "");
    إذا (i.info) {
        چاپ_کړه("%p: %s\n", Get_پاڼه_PC(o->پاڼه), i.info->name);
    } وإلا {
        چاپ_کړه("not found: %p\n", i_operation);
    }
}

#د_که_پای // DEBUG


#إذا d_m3EnableOpProfiling

تعريفنوع بنية M3_Profiler_خونه {
    cstr_t opName;
    u64    موندنه_شمېر;
} M3_Profiler_خونه;

ساكن M3_Profiler_خونه s_opProfilerCounts[d_m3_Profiler_خونه_ماسک + 1] = {};

فراغ Profile_موندنه (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3_Profiler_خونه* خونه = &s_opProfilerCounts[ptr & d_m3_Profiler_خونه_ماسک];

    إذا (خونه->opName) {
        إذا (خونه->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    خونه->opName = i_operationName;
    خونه->موندنه_شمېر++;
}


فراغ m3_PrintProfilerInfo ()
{
    M3_Profiler_خونه  dummy;
    M3_Profiler_خونه* max_خونه = &dummy;

    افعل {
        max_خونه->موندنه_شمېر = 0;

        لكل (u32 i = 0; i <= d_m3_Profiler_خونه_ماسک; ++i) {
            M3_Profiler_خونه* خونه = &s_opProfilerCounts[i];

            إذا (خونه->opName) {
                إذا (خونه->موندنه_شمېر > max_خونه->موندنه_شمېر) {
                    max_خونه = خونه;
                }
            }
        }

        إذا (max_خونه->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", max_خونه->موندنه_شمېر, max_خونه->opName);
            max_خونه->opName = عدم;
        }
    } طالما (max_خونه->موندنه_شمېر);
}

#وإلا

فراغ m3_PrintProfilerInfo () {}

#د_که_پای
