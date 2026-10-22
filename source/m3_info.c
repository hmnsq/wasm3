//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#شامل "m3_env.h"
#شامل "m3_info.h"
#شامل "m3_compile.h"

#اگر defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (حرف* o_string, size_t i_string_بفر_سائز, voidptr_t i_sp, m3type_t i_type)
{
    صحیح len = 0;

    *o_string = 0;

    اگر (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_بفر_سائز, "%" PRIi32, *(i32*)i_sp);
    } ورنہ اگر (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_بفر_سائز, "%" PRIi64, *(i64*)i_sp);
    }
#  اگر d_m3HasFloat
    ورنہ اگر (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_بفر_سائز, "%" PRIf32, *(f32*)i_sp);
    } ورنہ اگر (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_بفر_سائز, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    واپس len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    صحیح ret;

    ساکن حرف string[256];

    حرف*   s = string;
    ccstr_t e = string + حجم(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    اگر (funcType) {
        u32 عدد_دلائل = funcType->عدد_دلائل;

        برائے (u32 i = 0; i < عدد_دلائل; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[بنیاد_Type_کا(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            اگر (i != عدد_دلائل - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } ورنہ {
        لکھو("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    واپس string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    صحیح ret;

    ساکن حرف string[256];

    حرف*   s = string;
    ccstr_t e = string + حجم(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 عدد_Rets = i_funcType ? i_funcType->عدد_Rets : 0;

    برائے (u32 i = 0; i < عدد_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        اگر (i != عدد_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    واپس string;
}

#اگر_ختم

#اگر_تعریف_شدہ DEBUG

// a central function you can be breakpoint:
خالی ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پیغام)
{
    لکھو("\nexception: '%s' @ %s\n", i_exception, i_پیغام);
    واپس;
}


قسمتعریف ساخت OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

خالی m3_PrintM3Info ()
{
    لکھو("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    لکھو(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_صفحہ_سائز);
    لکھو(" sizeof M3Compilation : %zu bytes             \n", حجم(M3Compilation));
    لکھو(" sizeof M3Function    : %zu bytes             \n", حجم(M3Function));
    لکھو("----------------------------------------------------------------\n\n");
}


خالی* v_PrintEnvModuleInfo (IM3Module i_module, خالی* i_info)
{
    u32* io_اشاریہ = (u32*)i_info;
    لکھو(" module [%u]  name: '%s'; funcs: %d  \n", (*io_اشاریہ)++, i_module->name, i_module->عدد_Functions);

    واپس تہی;
}


خالی m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    لکھو("\n-- m3 runtime -------------------------------------------------\n");

    لکھو(" stack-size: %zu   \n\n", i_runtime->عدد_ڈھیر_Slots * حجم(m3slot_t));

    u32 module_اشاریہ = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_اشاریہ);

    لکھو("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 بنیاد = بنیاد_Type_کا(i_m3Type);

    اگر (بنیاد <= c_m3Type_unknown) {
        واپس c_waTypes[بنیاد];
    } ورنہ {
        واپس "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
ساکن
size_t SPrintAppend (حرف* o_string, size_t i_string_بفر_سائز, size_t i_len, ccstr_t i_format, ...)
{
    اگر (i_len + 1 >= i_string_بفر_سائز) {
        واپس i_len;
    }

    va_list دلائل;
    va_start(دلائل, i_format);
    صحیح ret = vsnprintf(o_string + i_len, i_string_بفر_سائز - i_len, i_format, دلائل);
    va_end(دلائل);

    واپس M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_بفر_سائز - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    ساکن حرف string[256];

    size_t len = SPrintAppend(string, حجم(string), 0, "(");

    برائے (u32 i = 0; i < i_funcType->عدد_دلائل; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, حجم(string), len, ") -> ");

    برائے (u32 i = 0; i < i_funcType->عدد_Rets; ++i) {
        len = SPrintAppend(string, حجم(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    واپس string;
}


cstr_t SPrint_قدر (خالی* i_قدر, m3type_t i_type)
{
    ساکن حرف string[100];
    SPrintArg(string, 100, (m3stack_t)i_قدر, i_type);
    واپس string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
ساکن
bool جائزہ_Operation_جدول (OpInfo* io_info, IM3OpInfo i_جدول, u32 i_عدد_Ops, m3opcode_t i_opcode_بنیاد, IM3Operation i_operation)
{
    برائے (u32 i = 0; i < i_عدد_Ops; ++i) {
        IM3OpInfo oi = &i_جدول[i];

        برائے (u32 o = 0; o < 4; ++o) {
            اگر (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_بنیاد | i;
                واپس true;
            }
        }
    }

    واپس false;
}

ساکن
OpInfo ڈھونڈو_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { تہی, 0 };

    اگر (!i_operation) {
        واپس opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    اگر (not جائزہ_Operation_جدول(&opInfo, c_operations, c_عدد_Operations, 0, i_operation)) {
        اگر (not جائزہ_Operation_جدول(&opInfo, c_operationsFC, c_عدد_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  اگر d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            جائزہ_Operation_جدول(&opInfo, c_operationsFE, c_عدد_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    واپس opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) خالی Decode_##FUNC (حرف * o_string, size_t i_string_بفر_سائز, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    خالی* function    = fetch(خالی*);
    i32   ڈھیر_انحراف = fetch(i32);

    snprintf(o_string, i_string_بفر_سائز, "%p; stack-offset: %d", function, ڈھیر_انحراف);
}


d_m3Decoder(اندراج_3)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_بفر_سائز, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    اگر (i_operation == i_opInfo->operations[0]) {
        u32 عملیہ = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_بفر_سائز, "offset= slot:%d + immediate:%d", عملیہ, offset);
    }
}


d_m3Decoder(Branch)
{
    خالی* ہدف = fetch(خالی*);
    snprintf(o_string, i_string_بفر_سائز, "%p", ہدف);
}

d_m3Decoder(Branch_جدول)
{
    u32 خانہ = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_بفر_سائز, 0, "slot: %" PRIu32 "; targets: ", خانہ);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    برائے (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_بفر_سائز, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_بفر_سائز, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 قدر  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_بفر_سائز, " slot [%d] = %" PRIu64, offset, قدر);
}


#  undef fetch

خالی DecodeOperation (حرف* o_string, size_t i_string_بفر_سائز, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) حالت OPCODE: Decode_##FUNC (o_string, i_string_بفر_سائز, i_opcode, i_operation, i_opInfo, o_pc); توڑو;

    انتخاب (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, اندراج_3)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_جدول, Branch_جدول)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
خالی dump_رمز_صفحہ (IM3_رمز_صفحہ i_رمز_صفحہ, pc_t i_آغاز_PC)
{
    m3log(رمز, "code page seq: %d", i_رمز_صفحہ->info.sequence);

    pc_t pc  = i_آغاز_PC ? i_آغاز_PC : Get_صفحہ_آغاز_PC(i_رمز_صفحہ);
    pc_t انتہا = Get_صفحہ_PC(i_رمز_صفحہ);

    m3log(رمز, "---------------------------------------------------------------------------------------");

    جب (pc < انتہا) {
        ثابت pc_t operationPC = pc;
        (خالی)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = ڈھونڈو_operation_info(op);

        اگر (i.info) {
            حرف infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, حجم(infoString), i.opcode, op, i.info, &pc);

            m3log(رمز, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } ورنہ {
            m3log(رمز, "%p | %p", operationPC, op);
        }
    }

    m3log(رمز, "---------------------------------------------------------------------------------------");

    m3log(رمز, "free-lines: %d", i_رمز_صفحہ->info.عدد_سطریں - i_رمز_صفحہ->info.سطر_اشاریہ);
}


خالی dump_type_ڈھیر (IM3Compilation o)
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
    i32 regAllocated[2] = { (i32)Is_رجسٹر_Allocated(o, 0), (i32)Is_رجسٹر_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(ڈھیر, "\n");
    d_m3Log(ڈھیر, "        ");
    لکھو("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    لکھو("\n");

    برائے (u32 p = 1; p <= 2; ++p) {
        d_m3Log(ڈھیر, "        ");

        برائے (u16 i = 0; i < o->ڈھیر_اشاریہ; ++i) {
            اگر (i > 0 and i == o->ڈھیر_پہلا_Dynamic_اشاریہ) {
                لکھو("#");
            }

            اگر (i == o->بلاک.بلاک_ڈھیر_اشاریہ) {
                لکھو(">");
            }

            ثابت حرف* type = c_waCompactTypes[بنیاد_Type_کا(o->type_ڈھیر[i])];

            ثابت حرف* location = "";

            i32 خانہ = o->wasm_ڈھیر[i];

            اگر (Is_رجسٹر_خانہ_Alias(خانہ)) {
                bool isFp = Is_Fp_رجسٹر_خانہ_Alias(خانہ);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                خانہ = -1;
            } ورنہ {
                اگر (خانہ < o->خانہ_پہلا_Dynamic_اشاریہ) {
                    اگر (خانہ >= o->خانہ_پہلا_Const_اشاریہ) {
                        location = "c";
                    } ورنہ اگر (خانہ >= o->function->عدد_Ret_And_Arg_Slots) {
                        location = "L";
                    } ورنہ {
                        location = "a";
                    }
                }
            }

            حرف شے[100];

            اگر (خانہ >= 0) {
                snprintf(شے, حجم(شے), "%s%s%d", type, location, خانہ);
            } ورنہ {
                snprintf(شے, حجم(شے), "%s%s", type, location);
            }

            اگر (p == 1) {
                size_t s = لڑی_لمبائی(شے);

                snprintf(شے, حجم(شے), "%d", i);

                جب (لڑی_لمبائی(شے) < s) {
                    strcat(شے, " ");
                }
            }

            لکھو("|%s ", شے);
        }
        لکھو("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 max_خانہ = Get_Max_مستعمل_خانہ_Plus_One(o);

    اگر (max_خانہ > o->خانہ_پہلا_Dynamic_اشاریہ) {
        d_m3Log(ڈھیر, "                      -");

        برائے (u16 i = o->خانہ_پہلا_Dynamic_اشاریہ; i < max_خانہ; ++i) {
            لکھو("----");
        }

        لکھو("\n");

        d_m3Log(ڈھیر, "                 slot |");
        برائے (u16 i = o->خانہ_پہلا_Dynamic_اشاریہ; i < max_خانہ; ++i) {
            لکھو("%3d|", i);
        }

        لکھو("\n");
        d_m3Log(ڈھیر, "                alloc |");

        برائے (u16 i = o->خانہ_پہلا_Dynamic_اشاریہ; i < max_خانہ; ++i) {
            لکھو("%3d|", o->m3Slots[i]);
        }

        لکھو("\n");
    }
    d_m3Log(ڈھیر, "\n");
}


ساکن
ثابت حرف* GetOpcodeIndentionString (i32 بلاک_گہرائی)
{
    بلاک_گہرائی += 1;

    اگر (بلاک_گہرائی < 0) {
        بلاک_گہرائی = 0;
    }

    ساکن ثابت حرف* s_spaces = ".......................................................................................";

    ثابت حرف* indent = s_spaces + لڑی_لمبائی(s_spaces);
    indent -= (بلاک_گہرائی * 2);
    اگر (indent < s_spaces) {
        indent = s_spaces;
    }

    واپس indent;
}


ثابت حرف* get_indention_string (IM3Compilation o)
{
    واپس GetOpcodeIndentionString(o->بلاک.گہرائی + 4);
}


خالی log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 گہرائی = o->بلاک.گہرائی;
    اگر (i_opcode == c_wa_Op_انتہا or i_opcode == c_waOp_else) {
        گہرائی--;
    }
    (خالی)گہرائی;

    m3log(compile, "%4d | 0x%02x  %s %s", o->عدد_Opcodes++, i_opcode, GetOpcodeIndentionString(گہرائی), GetOpInfo(i_opcode)->name);
}


خالی log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = ڈھونڈو_operation_info(i_operation);

    d_m3Log(emit, "");
    اگر (i.info) {
        لکھو("%p: %s\n", Get_صفحہ_PC(o->صفحہ), i.info->name);
    } ورنہ {
        لکھو("not found: %p\n", i_operation);
    }
}

#اگر_ختم // DEBUG


#اگر d_m3EnableOpProfiling

قسمتعریف ساخت M3_Profiler_خانہ {
    cstr_t opName;
    u64    کامیابی_گنتی;
} M3_Profiler_خانہ;

ساکن M3_Profiler_خانہ s_opProfilerCounts[d_m3_Profiler_خانہ_نقاب + 1] = {};

خالی Profile_کامیابی (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3_Profiler_خانہ* خانہ = &s_opProfilerCounts[ptr & d_m3_Profiler_خانہ_نقاب];

    اگر (خانہ->opName) {
        اگر (خانہ->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    خانہ->opName = i_operationName;
    خانہ->کامیابی_گنتی++;
}


خالی m3_PrintProfilerInfo ()
{
    M3_Profiler_خانہ  dummy;
    M3_Profiler_خانہ* max_خانہ = &dummy;

    کرو {
        max_خانہ->کامیابی_گنتی = 0;

        برائے (u32 i = 0; i <= d_m3_Profiler_خانہ_نقاب; ++i) {
            M3_Profiler_خانہ* خانہ = &s_opProfilerCounts[i];

            اگر (خانہ->opName) {
                اگر (خانہ->کامیابی_گنتی > max_خانہ->کامیابی_گنتی) {
                    max_خانہ = خانہ;
                }
            }
        }

        اگر (max_خانہ->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", max_خانہ->کامیابی_گنتی, max_خانہ->opName);
            max_خانہ->opName = تہی;
        }
    } جب (max_خانہ->کامیابی_گنتی);
}

#ورنہ

خالی m3_PrintProfilerInfo () {}

#اگر_ختم
