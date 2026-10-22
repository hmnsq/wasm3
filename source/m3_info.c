//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_info.h"
#अन्तर्भाव "m3_compile.h"

#यदि defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (वर्ण* o_string, size_t i_string_बफर_आकार, voidptr_t i_sp, m3type_t i_type)
{
    पूर्णाङ्क len = 0;

    *o_string = 0;

    यदि (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_बफर_आकार, "%" PRIi32, *(i32*)i_sp);
    } अन्यथा यदि (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_बफर_आकार, "%" PRIi64, *(i64*)i_sp);
    }
#  यदि d_m3HasFloat
    अन्यथा यदि (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_बफर_आकार, "%" PRIf32, *(f32*)i_sp);
    } अन्यथा यदि (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_बफर_आकार, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    प्रत्यागम len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    पूर्णाङ्क ret;

    स्थिर वर्ण string[256];

    वर्ण*   s = string;
    ccstr_t e = string + परिमाण(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    यदि (funcType) {
        u32 अङ्कः_तर्काः = funcType->अङ्कः_तर्काः;

        प्रति (u32 i = 0; i < अङ्कः_तर्काः; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[आधारः_Type_सम्बन्धि(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            यदि (i != अङ्कः_तर्काः - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } अन्यथा {
        प्रारूप_लिख("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    प्रत्यागम string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    पूर्णाङ्क ret;

    स्थिर वर्ण string[256];

    वर्ण*   s = string;
    ccstr_t e = string + परिमाण(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 अङ्कः_Rets = i_funcType ? i_funcType->अङ्कः_Rets : 0;

    प्रति (u32 i = 0; i < अङ्कः_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        यदि (i != अङ्कः_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    प्रत्यागम string;
}

#यदि_अन्तः

#यदि_परिभाषितम् DEBUG

// a central function you can be breakpoint:
शून्य ExceptionBreakpoint (cstr_t i_exception, cstr_t i_सन्देशः)
{
    प्रारूप_लिख("\nexception: '%s' @ %s\n", i_exception, i_सन्देशः);
    प्रत्यागम;
}


प्रकारनिर्देश संरचना OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

शून्य m3_PrintM3Info ()
{
    प्रारूप_लिख("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    प्रारूप_लिख(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_Page_आकार);
    प्रारूप_लिख(" sizeof M3Compilation : %zu bytes             \n", परिमाण(M3Compilation));
    प्रारूप_लिख(" sizeof M3Function    : %zu bytes             \n", परिमाण(M3Function));
    प्रारूप_लिख("----------------------------------------------------------------\n\n");
}


शून्य* v_PrintEnvModuleInfo (IM3Module i_module, शून्य* i_info)
{
    u32* io_सूचकांक = (u32*)i_info;
    प्रारूप_लिख(" module [%u]  name: '%s'; funcs: %d  \n", (*io_सूचकांक)++, i_module->name, i_module->अङ्कः_Functions);

    प्रत्यागम रिक्त;
}


शून्य m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    प्रारूप_लिख("\n-- m3 runtime -------------------------------------------------\n");

    प्रारूप_लिख(" stack-size: %zu   \n\n", i_runtime->अङ्कः_राशिः_Slots * परिमाण(m3slot_t));

    u32 module_सूचकांक = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_सूचकांक);

    प्रारूप_लिख("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 आधारः = आधारः_Type_सम्बन्धि(i_m3Type);

    यदि (आधारः <= c_m3Type_unknown) {
        प्रत्यागम c_waTypes[आधारः];
    } अन्यथा {
        प्रत्यागम "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
स्थिर
size_t SPrintAppend (वर्ण* o_string, size_t i_string_बफर_आकार, size_t i_len, ccstr_t i_format, ...)
{
    यदि (i_len + 1 >= i_string_बफर_आकार) {
        प्रत्यागम i_len;
    }

    va_list तर्काः;
    va_start(तर्काः, i_format);
    पूर्णाङ्क ret = vsnprintf(o_string + i_len, i_string_बफर_आकार - i_len, i_format, तर्काः);
    va_end(तर्काः);

    प्रत्यागम M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_बफर_आकार - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    स्थिर वर्ण string[256];

    size_t len = SPrintAppend(string, परिमाण(string), 0, "(");

    प्रति (u32 i = 0; i < i_funcType->अङ्कः_तर्काः; ++i) {
        len = SPrintAppend(string, परिमाण(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, परिमाण(string), len, ") -> ");

    प्रति (u32 i = 0; i < i_funcType->अङ्कः_Rets; ++i) {
        len = SPrintAppend(string, परिमाण(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    प्रत्यागम string;
}


cstr_t SPrint_मानम् (शून्य* i_मानम्, m3type_t i_type)
{
    स्थिर वर्ण string[100];
    SPrintArg(string, 100, (m3stack_t)i_मानम्, i_type);
    प्रत्यागम string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
स्थिर
bool अवलोकनम्_Operation_सारणी (OpInfo* io_info, IM3OpInfo i_सारणी, u32 i_अङ्कः_Ops, m3opcode_t i_opcode_आधारः, IM3Operation i_operation)
{
    प्रति (u32 i = 0; i < i_अङ्कः_Ops; ++i) {
        IM3OpInfo oi = &i_सारणी[i];

        प्रति (u32 o = 0; o < 4; ++o) {
            यदि (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_आधारः | i;
                प्रत्यागम true;
            }
        }
    }

    प्रत्यागम false;
}

स्थिर
OpInfo find_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { रिक्त, 0 };

    यदि (!i_operation) {
        प्रत्यागम opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    यदि (not अवलोकनम्_Operation_सारणी(&opInfo, c_operations, c_अङ्कः_Operations, 0, i_operation)) {
        यदि (not अवलोकनम्_Operation_सारणी(&opInfo, c_operationsFC, c_अङ्कः_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  यदि d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            अवलोकनम्_Operation_सारणी(&opInfo, c_operationsFE, c_अङ्कः_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    प्रत्यागम opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) शून्य Decode_##FUNC (वर्ण * o_string, size_t i_string_बफर_आकार, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    शून्य* function    = fetch(शून्य*);
    i32   राशिः_विस्थापनम् = fetch(i32);

    snprintf(o_string, i_string_बफर_आकार, "%p; stack-offset: %d", function, राशिः_विस्थापनम्);
}


d_m3Decoder(प्रविष्टिः_2)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_बफर_आकार, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    यदि (i_operation == i_opInfo->operations[0]) {
        u32 सङ्कार्यम् = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_बफर_आकार, "offset= slot:%d + immediate:%d", सङ्कार्यम्, offset);
    }
}


d_m3Decoder(Branch)
{
    शून्य* target = fetch(शून्य*);
    snprintf(o_string, i_string_बफर_आकार, "%p", target);
}

d_m3Decoder(Branch_सारणी)
{
    u32 slot = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_बफर_आकार, 0, "slot: %" PRIu32 "; targets: ", slot);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    प्रति (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_बफर_आकार, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_बफर_आकार, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 मानम्  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_बफर_आकार, " slot [%d] = %" PRIu64, offset, मानम्);
}


#  undef fetch

शून्य DecodeOperation (वर्ण* o_string, size_t i_string_बफर_आकार, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) पक्ष OPCODE: Decode_##FUNC (o_string, i_string_बफर_आकार, i_opcode, i_operation, i_opInfo, o_pc); भङ्ग;

    विकल्प (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, प्रविष्टिः_2)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_सारणी, Branch_सारणी)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
शून्य dump_संहिता_page (IM3_संहिता_Page i_संहिता_Page, pc_t i_startPC)
{
    m3log(संहिता, "code page seq: %d", i_संहिता_Page->info.sequence);

    pc_t pc  = i_startPC ? i_startPC : GetPageStartPC(i_संहिता_Page);
    pc_t अन्तः = GetPagePC(i_संहिता_Page);

    m3log(संहिता, "---------------------------------------------------------------------------------------");

    यावत् (pc < अन्तः) {
        ध्रुव pc_t operationPC = pc;
        (शून्य)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = find_operation_info(op);

        यदि (i.info) {
            वर्ण infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, परिमाण(infoString), i.opcode, op, i.info, &pc);

            m3log(संहिता, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } अन्यथा {
            m3log(संहिता, "%p | %p", operationPC, op);
        }
    }

    m3log(संहिता, "---------------------------------------------------------------------------------------");

    m3log(संहिता, "free-lines: %d", i_संहिता_Page->info.अङ्कः_पङ्क्तयः - i_संहिता_Page->info.पङ्क्तिः_सूचकांक);
}


शून्य dump_type_राशिः (IM3Compilation o)
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
    i32 regAllocated[2] = { (i32)Is_पञ्जिकाकोशः_Allocated(o, 0), (i32)Is_पञ्जिकाकोशः_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(राशिः, "\n");
    d_m3Log(राशिः, "        ");
    प्रारूप_लिख("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    प्रारूप_लिख("\n");

    प्रति (u32 p = 1; p <= 2; ++p) {
        d_m3Log(राशिः, "        ");

        प्रति (u16 i = 0; i < o->राशिः_सूचकांक; ++i) {
            यदि (i > 0 and i == o->राशिः_First_Dynamic_सूचकांक) {
                प्रारूप_लिख("#");
            }

            यदि (i == o->खण्डम्.खण्डम्_राशिः_सूचकांक) {
                प्रारूप_लिख(">");
            }

            ध्रुव वर्ण* type = c_waCompactTypes[आधारः_Type_सम्बन्धि(o->type_राशिः[i])];

            ध्रुव वर्ण* location = "";

            i32 slot = o->wasm_राशिः[i];

            यदि (Is_पञ्जिकाकोशः_Slot_Alias(slot)) {
                bool isFp = Is_Fp_पञ्जिकाकोशः_Slot_Alias(slot);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                slot = -1;
            } अन्यथा {
                यदि (slot < o->slot_First_Dynamic_सूचकांक) {
                    यदि (slot >= o->slot_First_Const_सूचकांक) {
                        location = "c";
                    } अन्यथा यदि (slot >= o->function->अङ्कः_Ret_And_Arg_Slots) {
                        location = "L";
                    } अन्यथा {
                        location = "a";
                    }
                }
            }

            वर्ण item[100];

            यदि (slot >= 0) {
                snprintf(item, परिमाण(item), "%s%s%d", type, location, slot);
            } अन्यथा {
                snprintf(item, परिमाण(item), "%s%s", type, location);
            }

            यदि (p == 1) {
                size_t s = वाक्यदैर्घ्यम्(item);

                snprintf(item, परिमाण(item), "%d", i);

                यावत् (वाक्यदैर्घ्यम्(item) < s) {
                    वाक्यं_योजय(item, " ");
                }
            }

            प्रारूप_लिख("|%s ", item);
        }
        प्रारूप_लिख("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 maxSlot = Get_Max_प्रयुक्तम्_Slot_Plus_One(o);

    यदि (maxSlot > o->slot_First_Dynamic_सूचकांक) {
        d_m3Log(राशिः, "                      -");

        प्रति (u16 i = o->slot_First_Dynamic_सूचकांक; i < maxSlot; ++i) {
            प्रारूप_लिख("----");
        }

        प्रारूप_लिख("\n");

        d_m3Log(राशिः, "                 slot |");
        प्रति (u16 i = o->slot_First_Dynamic_सूचकांक; i < maxSlot; ++i) {
            प्रारूप_लिख("%3d|", i);
        }

        प्रारूप_लिख("\n");
        d_m3Log(राशिः, "                alloc |");

        प्रति (u16 i = o->slot_First_Dynamic_सूचकांक; i < maxSlot; ++i) {
            प्रारूप_लिख("%3d|", o->m3Slots[i]);
        }

        प्रारूप_लिख("\n");
    }
    d_m3Log(राशिः, "\n");
}


स्थिर
ध्रुव वर्ण* GetOpcodeIndentionString (i32 खण्डम्_गभीरता)
{
    खण्डम्_गभीरता += 1;

    यदि (खण्डम्_गभीरता < 0) {
        खण्डम्_गभीरता = 0;
    }

    स्थिर ध्रुव वर्ण* s_spaces = ".......................................................................................";

    ध्रुव वर्ण* indent = s_spaces + वाक्यदैर्घ्यम्(s_spaces);
    indent -= (खण्डम्_गभीरता * 2);
    यदि (indent < s_spaces) {
        indent = s_spaces;
    }

    प्रत्यागम indent;
}


ध्रुव वर्ण* get_indention_string (IM3Compilation o)
{
    प्रत्यागम GetOpcodeIndentionString(o->खण्डम्.गभीरता + 4);
}


शून्य log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 गभीरता = o->खण्डम्.गभीरता;
    यदि (i_opcode == c_wa_Op_अन्तः or i_opcode == c_waOp_else) {
        गभीरता--;
    }
    (शून्य)गभीरता;

    m3log(compile, "%4d | 0x%02x  %s %s", o->अङ्कः_Opcodes++, i_opcode, GetOpcodeIndentionString(गभीरता), GetOpInfo(i_opcode)->name);
}


शून्य log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = find_operation_info(i_operation);

    d_m3Log(emit, "");
    यदि (i.info) {
        प्रारूप_लिख("%p: %s\n", GetPagePC(o->page), i.info->name);
    } अन्यथा {
        प्रारूप_लिख("not found: %p\n", i_operation);
    }
}

#यदि_अन्तः // DEBUG


#यदि d_m3EnableOpProfiling

प्रकारनिर्देश संरचना M3ProfilerSlot {
    cstr_t opName;
    u64    लब्धम्_गणनम्;
} M3ProfilerSlot;

स्थिर M3ProfilerSlot s_opProfilerCounts[d_m3_Profiler_Slot_आवरणम् + 1] = {};

शून्य Profile_लब्धम् (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3ProfilerSlot* slot = &s_opProfilerCounts[ptr & d_m3_Profiler_Slot_आवरणम्];

    यदि (slot->opName) {
        यदि (slot->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    slot->opName = i_operationName;
    slot->लब्धम्_गणनम्++;
}


शून्य m3_PrintProfilerInfo ()
{
    M3ProfilerSlot  dummy;
    M3ProfilerSlot* maxSlot = &dummy;

    कुरु {
        maxSlot->लब्धम्_गणनम् = 0;

        प्रति (u32 i = 0; i <= d_m3_Profiler_Slot_आवरणम्; ++i) {
            M3ProfilerSlot* slot = &s_opProfilerCounts[i];

            यदि (slot->opName) {
                यदि (slot->लब्धम्_गणनम् > maxSlot->लब्धम्_गणनम्) {
                    maxSlot = slot;
                }
            }
        }

        यदि (maxSlot->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", maxSlot->लब्धम्_गणनम्, maxSlot->opName);
            maxSlot->opName = रिक्त;
        }
    } यावत् (maxSlot->लब्धम्_गणनम्);
}

#अन्यथा

शून्य m3_PrintProfilerInfo () {}

#यदि_अन्तः
