//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#అన్తర్భావ "m3_env.h"
#అన్తర్భావ "m3_info.h"
#అన్తర్భావ "m3_compile.h"

#ఒకవేళ defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (అక్షరం* o_string, size_t i_string_బఫరు_కొలత, voidptr_t i_sp, m3type_t i_type)
{
    పూర్ణాంకం len = 0;

    *o_string = 0;

    ఒకవేళ (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_బఫరు_కొలత, "%" PRIi32, *(i32*)i_sp);
    } లేకపోతే ఒకవేళ (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_బఫరు_కొలత, "%" PRIi64, *(i64*)i_sp);
    }
#  ఒకవేళ d_m3HasFloat
    లేకపోతే ఒకవేళ (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_బఫరు_కొలత, "%" PRIf32, *(f32*)i_sp);
    } లేకపోతే ఒకవేళ (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_బఫరు_కొలత, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    తిరిగి len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    పూర్ణాంకం ret;

    స్థిర అక్షరం string[256];

    అక్షరం*   s = string;
    ccstr_t e = string + పరిమాణం(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    ఒకవేళ (funcType) {
        u32 అంకె_వాదనలు = funcType->అంకె_వాదనలు;

        కొరకు (u32 i = 0; i < అంకె_వాదనలు; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[పునాది_Type_యొక్క(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            ఒకవేళ (i != అంకె_వాదనలు - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } లేకపోతే {
        రాయి("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    తిరిగి string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    పూర్ణాంకం ret;

    స్థిర అక్షరం string[256];

    అక్షరం*   s = string;
    ccstr_t e = string + పరిమాణం(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 అంకె_Rets = i_funcType ? i_funcType->అంకె_Rets : 0;

    కొరకు (u32 i = 0; i < అంకె_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        ఒకవేళ (i != అంకె_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    తిరిగి string;
}

#అయితే_ముగింపు

#నిర్వచితమైతే DEBUG

// a central function you can be breakpoint:
శూన్య ExceptionBreakpoint (cstr_t i_exception, cstr_t i_సందేశం)
{
    రాయి("\nexception: '%s' @ %s\n", i_exception, i_సందేశం);
    తిరిగి;
}


రకనిర్వచనం నిర్మాణం OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

శూన్య m3_PrintM3Info ()
{
    రాయి("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    రాయి(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_పుట_కొలత);
    రాయి(" sizeof M3Compilation : %zu bytes             \n", పరిమాణం(M3Compilation));
    రాయి(" sizeof M3Function    : %zu bytes             \n", పరిమాణం(M3Function));
    రాయి("----------------------------------------------------------------\n\n");
}


శూన్య* v_PrintEnvModuleInfo (IM3Module i_module, శూన్య* i_info)
{
    u32* io_సూచీ = (u32*)i_info;
    రాయి(" module [%u]  name: '%s'; funcs: %d  \n", (*io_సూచీ)++, i_module->name, i_module->అంకె_Functions);

    తిరిగి ఖాళీ;
}


శూన్య m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    రాయి("\n-- m3 runtime -------------------------------------------------\n");

    రాయి(" stack-size: %zu   \n\n", i_runtime->అంకె_దొంతర_Slots * పరిమాణం(m3slot_t));

    u32 module_సూచీ = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_సూచీ);

    రాయి("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 పునాది = పునాది_Type_యొక్క(i_m3Type);

    ఒకవేళ (పునాది <= c_m3Type_unknown) {
        తిరిగి c_waTypes[పునాది];
    } లేకపోతే {
        తిరిగి "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
స్థిర
size_t SPrintAppend (అక్షరం* o_string, size_t i_string_బఫరు_కొలత, size_t i_len, ccstr_t i_format, ...)
{
    ఒకవేళ (i_len + 1 >= i_string_బఫరు_కొలత) {
        తిరిగి i_len;
    }

    va_list వాదనలు;
    va_start(వాదనలు, i_format);
    పూర్ణాంకం ret = vsnprintf(o_string + i_len, i_string_బఫరు_కొలత - i_len, i_format, వాదనలు);
    va_end(వాదనలు);

    తిరిగి M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_బఫరు_కొలత - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    స్థిర అక్షరం string[256];

    size_t len = SPrintAppend(string, పరిమాణం(string), 0, "(");

    కొరకు (u32 i = 0; i < i_funcType->అంకె_వాదనలు; ++i) {
        len = SPrintAppend(string, పరిమాణం(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, పరిమాణం(string), len, ") -> ");

    కొరకు (u32 i = 0; i < i_funcType->అంకె_Rets; ++i) {
        len = SPrintAppend(string, పరిమాణం(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    తిరిగి string;
}


cstr_t SPrint_విలువ (శూన్య* i_విలువ, m3type_t i_type)
{
    స్థిర అక్షరం string[100];
    SPrintArg(string, 100, (m3stack_t)i_విలువ, i_type);
    తిరిగి string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
స్థిర
bool పరిశీలన_Operation_పట్టిక (OpInfo* io_info, IM3OpInfo i_పట్టిక, u32 i_అంకె_Ops, m3opcode_t i_opcode_పునాది, IM3Operation i_operation)
{
    కొరకు (u32 i = 0; i < i_అంకె_Ops; ++i) {
        IM3OpInfo oi = &i_పట్టిక[i];

        కొరకు (u32 o = 0; o < 4; ++o) {
            ఒకవేళ (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_పునాది | i;
                తిరిగి true;
            }
        }
    }

    తిరిగి false;
}

స్థిర
OpInfo కనుగొను_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { ఖాళీ, 0 };

    ఒకవేళ (!i_operation) {
        తిరిగి opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    ఒకవేళ (not పరిశీలన_Operation_పట్టిక(&opInfo, c_operations, c_అంకె_Operations, 0, i_operation)) {
        ఒకవేళ (not పరిశీలన_Operation_పట్టిక(&opInfo, c_operationsFC, c_అంకె_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  ఒకవేళ d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            పరిశీలన_Operation_పట్టిక(&opInfo, c_operationsFE, c_అంకె_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    తిరిగి opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) శూన్య Decode_##FUNC (అక్షరం * o_string, size_t i_string_బఫరు_కొలత, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    శూన్య* function    = fetch(శూన్య*);
    i32   దొంతర_స్థానభ్రంశం = fetch(i32);

    snprintf(o_string, i_string_బఫరు_కొలత, "%p; stack-offset: %d", function, దొంతర_స్థానభ్రంశం);
}


d_m3Decoder(నమోదు_3)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_బఫరు_కొలత, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    ఒకవేళ (i_operation == i_opInfo->operations[0]) {
        u32 నిర్వాహ్యం = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_బఫరు_కొలత, "offset= slot:%d + immediate:%d", నిర్వాహ్యం, offset);
    }
}


d_m3Decoder(Branch)
{
    శూన్య* లక్ష్యం = fetch(శూన్య*);
    snprintf(o_string, i_string_బఫరు_కొలత, "%p", లక్ష్యం);
}

d_m3Decoder(Branch_పట్టిక)
{
    u32 స్థానం = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_బఫరు_కొలత, 0, "slot: %" PRIu32 "; targets: ", స్థానం);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    కొరకు (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_బఫరు_కొలత, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_బఫరు_కొలత, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 విలువ  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_బఫరు_కొలత, " slot [%d] = %" PRIu64, offset, విలువ);
}


#  undef fetch

శూన్య DecodeOperation (అక్షరం* o_string, size_t i_string_బఫరు_కొలత, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) స్థితి OPCODE: Decode_##FUNC (o_string, i_string_బఫరు_కొలత, i_opcode, i_operation, i_opInfo, o_pc); ఆపు;

    ఎంపిక (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, నమోదు_3)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_పట్టిక, Branch_పట్టిక)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
శూన్య dump_సంకేతం_పుట (IM3_సంకేతం_పుట i_సంకేతం_పుట, pc_t i_ప్రారంభం_PC)
{
    m3log(సంకేతం_2, "code page seq: %d", i_సంకేతం_పుట->info.sequence);

    pc_t pc  = i_ప్రారంభం_PC ? i_ప్రారంభం_PC : Get_పుట_ప్రారంభం_PC(i_సంకేతం_పుట);
    pc_t చివర = Get_పుట_PC(i_సంకేతం_పుట);

    m3log(సంకేతం_2, "---------------------------------------------------------------------------------------");

    అయితే (pc < చివర) {
        స్థిరరాశి pc_t operationPC = pc;
        (శూన్య)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = కనుగొను_operation_info(op);

        ఒకవేళ (i.info) {
            అక్షరం infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, పరిమాణం(infoString), i.opcode, op, i.info, &pc);

            m3log(సంకేతం_2, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } లేకపోతే {
            m3log(సంకేతం_2, "%p | %p", operationPC, op);
        }
    }

    m3log(సంకేతం_2, "---------------------------------------------------------------------------------------");

    m3log(సంకేతం_2, "free-lines: %d", i_సంకేతం_పుట->info.అంకె_పంక్తులు - i_సంకేతం_పుట->info.పంక్తి_సూచీ);
}


శూన్య dump_type_దొంతర (IM3Compilation o)
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
    i32 regAllocated[2] = { (i32)Is_రిజిస్టరు_Allocated(o, 0), (i32)Is_రిజిస్టరు_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(దొంతర, "\n");
    d_m3Log(దొంతర, "        ");
    రాయి("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    రాయి("\n");

    కొరకు (u32 p = 1; p <= 2; ++p) {
        d_m3Log(దొంతర, "        ");

        కొరకు (u16 i = 0; i < o->దొంతర_సూచీ; ++i) {
            ఒకవేళ (i > 0 and i == o->దొంతర_మొదటి_Dynamic_సూచీ) {
                రాయి("#");
            }

            ఒకవేళ (i == o->ఖండం.ఖండం_దొంతర_సూచీ) {
                రాయి(">");
            }

            స్థిరరాశి అక్షరం* type = c_waCompactTypes[పునాది_Type_యొక్క(o->type_దొంతర[i])];

            స్థిరరాశి అక్షరం* location = "";

            i32 స్థానం = o->wasm_దొంతర[i];

            ఒకవేళ (Is_రిజిస్టరు_స్థానం_Alias(స్థానం)) {
                bool isFp = Is_Fp_రిజిస్టరు_స్థానం_Alias(స్థానం);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                స్థానం = -1;
            } లేకపోతే {
                ఒకవేళ (స్థానం < o->స్థానం_మొదటి_Dynamic_సూచీ) {
                    ఒకవేళ (స్థానం >= o->స్థానం_మొదటి_Const_సూచీ) {
                        location = "c";
                    } లేకపోతే ఒకవేళ (స్థానం >= o->function->అంకె_Ret_And_Arg_Slots) {
                        location = "L";
                    } లేకపోతే {
                        location = "a";
                    }
                }
            }

            అక్షరం అంశం[100];

            ఒకవేళ (స్థానం >= 0) {
                snprintf(అంశం, పరిమాణం(అంశం), "%s%s%d", type, location, స్థానం);
            } లేకపోతే {
                snprintf(అంశం, పరిమాణం(అంశం), "%s%s", type, location);
            }

            ఒకవేళ (p == 1) {
                size_t s = అక్షరశ్రేణి_పొడవు(అంశం);

                snprintf(అంశం, పరిమాణం(అంశం), "%d", i);

                అయితే (అక్షరశ్రేణి_పొడవు(అంశం) < s) {
                    strcat(అంశం, " ");
                }
            }

            రాయి("|%s ", అంశం);
        }
        రాయి("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 max_స్థానం = Get_Max_వాడిన_స్థానం_Plus_One(o);

    ఒకవేళ (max_స్థానం > o->స్థానం_మొదటి_Dynamic_సూచీ) {
        d_m3Log(దొంతర, "                      -");

        కొరకు (u16 i = o->స్థానం_మొదటి_Dynamic_సూచీ; i < max_స్థానం; ++i) {
            రాయి("----");
        }

        రాయి("\n");

        d_m3Log(దొంతర, "                 slot |");
        కొరకు (u16 i = o->స్థానం_మొదటి_Dynamic_సూచీ; i < max_స్థానం; ++i) {
            రాయి("%3d|", i);
        }

        రాయి("\n");
        d_m3Log(దొంతర, "                alloc |");

        కొరకు (u16 i = o->స్థానం_మొదటి_Dynamic_సూచీ; i < max_స్థానం; ++i) {
            రాయి("%3d|", o->m3Slots[i]);
        }

        రాయి("\n");
    }
    d_m3Log(దొంతర, "\n");
}


స్థిర
స్థిరరాశి అక్షరం* GetOpcodeIndentionString (i32 ఖండం_లోతు)
{
    ఖండం_లోతు += 1;

    ఒకవేళ (ఖండం_లోతు < 0) {
        ఖండం_లోతు = 0;
    }

    స్థిర స్థిరరాశి అక్షరం* s_spaces = ".......................................................................................";

    స్థిరరాశి అక్షరం* indent = s_spaces + అక్షరశ్రేణి_పొడవు(s_spaces);
    indent -= (ఖండం_లోతు * 2);
    ఒకవేళ (indent < s_spaces) {
        indent = s_spaces;
    }

    తిరిగి indent;
}


స్థిరరాశి అక్షరం* get_indention_string (IM3Compilation o)
{
    తిరిగి GetOpcodeIndentionString(o->ఖండం.లోతు + 4);
}


శూన్య log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 లోతు = o->ఖండం.లోతు;
    ఒకవేళ (i_opcode == c_wa_Op_చివర or i_opcode == c_waOp_else) {
        లోతు--;
    }
    (శూన్య)లోతు;

    m3log(compile, "%4d | 0x%02x  %s %s", o->అంకె_Opcodes++, i_opcode, GetOpcodeIndentionString(లోతు), GetOpInfo(i_opcode)->name);
}


శూన్య log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = కనుగొను_operation_info(i_operation);

    d_m3Log(emit, "");
    ఒకవేళ (i.info) {
        రాయి("%p: %s\n", Get_పుట_PC(o->పుట), i.info->name);
    } లేకపోతే {
        రాయి("not found: %p\n", i_operation);
    }
}

#అయితే_ముగింపు // DEBUG


#ఒకవేళ d_m3EnableOpProfiling

రకనిర్వచనం నిర్మాణం M3_Profiler_స్థానం {
    cstr_t opName;
    u64    తగిలింది_లెక్క;
} M3_Profiler_స్థానం;

స్థిర M3_Profiler_స్థానం s_opProfilerCounts[d_m3_Profiler_స్థానం_ముసుగు + 1] = {};

శూన్య Profile_తగిలింది (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3_Profiler_స్థానం* స్థానం = &s_opProfilerCounts[ptr & d_m3_Profiler_స్థానం_ముసుగు];

    ఒకవేళ (స్థానం->opName) {
        ఒకవేళ (స్థానం->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    స్థానం->opName = i_operationName;
    స్థానం->తగిలింది_లెక్క++;
}


శూన్య m3_PrintProfilerInfo ()
{
    M3_Profiler_స్థానం  dummy;
    M3_Profiler_స్థానం* max_స్థానం = &dummy;

    చేయి {
        max_స్థానం->తగిలింది_లెక్క = 0;

        కొరకు (u32 i = 0; i <= d_m3_Profiler_స్థానం_ముసుగు; ++i) {
            M3_Profiler_స్థానం* స్థానం = &s_opProfilerCounts[i];

            ఒకవేళ (స్థానం->opName) {
                ఒకవేళ (స్థానం->తగిలింది_లెక్క > max_స్థానం->తగిలింది_లెక్క) {
                    max_స్థానం = స్థానం;
                }
            }
        }

        ఒకవేళ (max_స్థానం->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", max_స్థానం->తగిలింది_లెక్క, max_స్థానం->opName);
            max_స్థానం->opName = ఖాళీ;
        }
    } అయితే (max_స్థానం->తగిలింది_లెక్క);
}

#లేకపోతే

శూన్య m3_PrintProfilerInfo () {}

#అయితే_ముగింపు
