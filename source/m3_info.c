//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#അന്തര്ഭാവ "m3_env.h"
#അന്തര്ഭാവ "m3_info.h"
#അന്തര്ഭാവ "m3_compile.h"

#എങ്കിൽ defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (അക്ഷരം* o_string, size_t i_stringBufferSize, voidptr_t i_sp, m3type_t i_type)
{
    പൂർണ്ണസംഖ്യ len = 0;

    *o_string = 0;

    എങ്കിൽ (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi32, *(i32*)i_sp);
    } അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi64, *(i64*)i_sp);
    }
#  എങ്കിൽ d_m3HasFloat
    അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf32, *(f32*)i_sp);
    } അല്ലെങ്കിൽ എങ്കിൽ (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    തിരികെ len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    പൂർണ്ണസംഖ്യ ret;

    സ്ഥിതം അക്ഷരം string[256];

    അക്ഷരം*   s = string;
    ccstr_t e = string + വലുപ്പം(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    എങ്കിൽ (funcType) {
        u32 അക്കം_Args = funcType->അക്കം_Args;

        വേണ്ടി (u32 i = 0; i < അക്കം_Args; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[BaseTypeOf(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            എങ്കിൽ (i != അക്കം_Args - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } അല്ലെങ്കിൽ {
        printf("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    തിരികെ string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    പൂർണ്ണസംഖ്യ ret;

    സ്ഥിതം അക്ഷരം string[256];

    അക്ഷരം*   s = string;
    ccstr_t e = string + വലുപ്പം(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 അക്കം_Rets = i_funcType ? i_funcType->അക്കം_Rets : 0;

    വേണ്ടി (u32 i = 0; i < അക്കം_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        എങ്കിൽ (i != അക്കം_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    തിരികെ string;
}

#endif

#ifdef DEBUG

// a central function you can be breakpoint:
ശൂന്യം ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message)
{
    printf("\nexception: '%s' @ %s\n", i_exception, i_message);
    തിരികെ;
}


തരനിർവചനം ഘടന OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

ശൂന്യം m3_PrintM3Info ()
{
    printf("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    printf(" sizeof M3MemPage     : %u bytes              \n", d_m3DefaultMemPageSize);
    printf(" sizeof M3Compilation : %zu bytes             \n", വലുപ്പം(M3Compilation));
    printf(" sizeof M3Function    : %zu bytes             \n", വലുപ്പം(M3Function));
    printf("----------------------------------------------------------------\n\n");
}


ശൂന്യം* v_PrintEnvModuleInfo (IM3Module i_module, ശൂന്യം* i_info)
{
    u32* io_index = (u32*)i_info;
    printf(" module [%u]  name: '%s'; funcs: %d  \n", (*io_index)++, i_module->name, i_module->അക്കം_Functions);

    തിരികെ ശൂന്യ;
}


ശൂന്യം m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    printf("\n-- m3 runtime -------------------------------------------------\n");

    printf(" stack-size: %zu   \n\n", i_runtime->അക്കം_അട്ടി_Slots * വലുപ്പം(m3slot_t));

    u32 moduleIndex = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &moduleIndex);

    printf("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 base = BaseTypeOf(i_m3Type);

    എങ്കിൽ (base <= c_m3Type_unknown) {
        തിരികെ c_waTypes[base];
    } അല്ലെങ്കിൽ {
        തിരികെ "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
സ്ഥിതം
size_t SPrintAppend (അക്ഷരം* o_string, size_t i_stringBufferSize, size_t i_len, ccstr_t i_format, ...)
{
    എങ്കിൽ (i_len + 1 >= i_stringBufferSize) {
        തിരികെ i_len;
    }

    va_list args;
    va_start(args, i_format);
    പൂർണ്ണസംഖ്യ ret = vsnprintf(o_string + i_len, i_stringBufferSize - i_len, i_format, args);
    va_end(args);

    തിരികെ M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_stringBufferSize - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    സ്ഥിതം അക്ഷരം string[256];

    size_t len = SPrintAppend(string, വലുപ്പം(string), 0, "(");

    വേണ്ടി (u32 i = 0; i < i_funcType->അക്കം_Args; ++i) {
        len = SPrintAppend(string, വലുപ്പം(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, വലുപ്പം(string), len, ") -> ");

    വേണ്ടി (u32 i = 0; i < i_funcType->അക്കം_Rets; ++i) {
        len = SPrintAppend(string, വലുപ്പം(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    തിരികെ string;
}


cstr_t SPrint_മൂല്യം (ശൂന്യം* i_മൂല്യം, m3type_t i_type)
{
    സ്ഥിതം അക്ഷരം string[100];
    SPrintArg(string, 100, (m3stack_t)i_മൂല്യം, i_type);
    തിരികെ string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
സ്ഥിതം
bool ScanOperationTable (OpInfo* io_info, IM3OpInfo i_table, u32 i_അക്കം_Ops, m3opcode_t i_opcodeBase, IM3Operation i_operation)
{
    വേണ്ടി (u32 i = 0; i < i_അക്കം_Ops; ++i) {
        IM3OpInfo oi = &i_table[i];

        വേണ്ടി (u32 o = 0; o < 4; ++o) {
            എങ്കിൽ (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcodeBase | i;
                തിരികെ true;
            }
        }
    }

    തിരികെ false;
}

സ്ഥിതം
OpInfo find_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { ശൂന്യ, 0 };

    എങ്കിൽ (!i_operation) {
        തിരികെ opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    എങ്കിൽ (not ScanOperationTable(&opInfo, c_operations, c_അക്കം_Operations, 0, i_operation)) {
        എങ്കിൽ (not ScanOperationTable(&opInfo, c_operationsFC, c_അക്കം_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  എങ്കിൽ d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            ScanOperationTable(&opInfo, c_operationsFE, c_അക്കം_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    തിരികെ opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) ശൂന്യം Decode_##FUNC (അക്ഷരം * o_string, size_t i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    ശൂന്യം* function    = fetch(ശൂന്യം*);
    i32   അട്ടി_Offset = fetch(i32);

    snprintf(o_string, i_stringBufferSize, "%p; stack-offset: %d", function, അട്ടി_Offset);
}


d_m3Decoder(Entry)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_stringBufferSize, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    എങ്കിൽ (i_operation == i_opInfo->operations[0]) {
        u32 operand = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_stringBufferSize, "offset= slot:%d + immediate:%d", operand, offset);
    }
}


d_m3Decoder(Branch)
{
    ശൂന്യം* target = fetch(ശൂന്യം*);
    snprintf(o_string, i_stringBufferSize, "%p", target);
}

d_m3Decoder(BranchTable)
{
    u32 slot = fetch(u32);

    size_t len = SPrintAppend(o_string, i_stringBufferSize, 0, "slot: %" PRIu32 "; targets: ", slot);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    വേണ്ടി (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_stringBufferSize, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_stringBufferSize, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 മൂല്യം  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_stringBufferSize, " slot [%d] = %" PRIu64, offset, മൂല്യം);
}


#  undef fetch

ശൂന്യം DecodeOperation (അക്ഷരം* o_string, size_t i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) അവസ്ഥ OPCODE: Decode_##FUNC (o_string, i_stringBufferSize, i_opcode, i_operation, i_opInfo, o_pc); നിർത്തുക;

    തിരഞ്ഞെടുപ്പ് (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, Entry)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_waOp_branchTable, BranchTable)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
ശൂന്യം dump_code_page (IM3CodePage i_codePage, pc_t i_startPC)
{
    m3log(code, "code page seq: %d", i_codePage->info.sequence);

    pc_t pc  = i_startPC ? i_startPC : GetPageStartPC(i_codePage);
    pc_t end = GetPagePC(i_codePage);

    m3log(code, "---------------------------------------------------------------------------------------");

    ആകുമ്പോൾ (pc < end) {
        സ്ഥിരം pc_t operationPC = pc;
        (ശൂന്യം)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = find_operation_info(op);

        എങ്കിൽ (i.info) {
            അക്ഷരം infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, വലുപ്പം(infoString), i.opcode, op, i.info, &pc);

            m3log(code, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } അല്ലെങ്കിൽ {
            m3log(code, "%p | %p", operationPC, op);
        }
    }

    m3log(code, "---------------------------------------------------------------------------------------");

    m3log(code, "free-lines: %d", i_codePage->info.അക്കം_വരികൾ - i_codePage->info.വരി_Index);
}


ശൂന്യം dump_type_അട്ടി (IM3Compilation o)
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
    i32 regAllocated[2] = { (i32)IsRegisterAllocated(o, 0), (i32)IsRegisterAllocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(അട്ടി, "\n");
    d_m3Log(അട്ടി, "        ");
    printf("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    printf("\n");

    വേണ്ടി (u32 p = 1; p <= 2; ++p) {
        d_m3Log(അട്ടി, "        ");

        വേണ്ടി (u16 i = 0; i < o->അട്ടി_Index; ++i) {
            എങ്കിൽ (i > 0 and i == o->അട്ടി_First_Dynamic_Index) {
                printf("#");
            }

            എങ്കിൽ (i == o->block.block_അട്ടി_Index) {
                printf(">");
            }

            സ്ഥിരം അക്ഷരം* type = c_waCompactTypes[BaseTypeOf(o->type_അട്ടി[i])];

            സ്ഥിരം അക്ഷരം* location = "";

            i32 slot = o->wasm_അട്ടി[i];

            എങ്കിൽ (IsRegisterSlotAlias(slot)) {
                bool isFp = IsFpRegisterSlotAlias(slot);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                slot = -1;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (slot < o->slotFirstDynamicIndex) {
                    എങ്കിൽ (slot >= o->slotFirstConstIndex) {
                        location = "c";
                    } അല്ലെങ്കിൽ എങ്കിൽ (slot >= o->function->അക്കം_Ret_And_Arg_Slots) {
                        location = "L";
                    } അല്ലെങ്കിൽ {
                        location = "a";
                    }
                }
            }

            അക്ഷരം item[100];

            എങ്കിൽ (slot >= 0) {
                snprintf(item, വലുപ്പം(item), "%s%s%d", type, location, slot);
            } അല്ലെങ്കിൽ {
                snprintf(item, വലുപ്പം(item), "%s%s", type, location);
            }

            എങ്കിൽ (p == 1) {
                size_t s = strlen(item);

                snprintf(item, വലുപ്പം(item), "%d", i);

                ആകുമ്പോൾ (strlen(item) < s) {
                    strcat(item, " ");
                }
            }

            printf("|%s ", item);
        }
        printf("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 maxSlot = GetMaxUsedSlotPlusOne(o);

    എങ്കിൽ (maxSlot > o->slotFirstDynamicIndex) {
        d_m3Log(അട്ടി, "                      -");

        വേണ്ടി (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("----");
        }

        printf("\n");

        d_m3Log(അട്ടി, "                 slot |");
        വേണ്ടി (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("%3d|", i);
        }

        printf("\n");
        d_m3Log(അട്ടി, "                alloc |");

        വേണ്ടി (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("%3d|", o->m3Slots[i]);
        }

        printf("\n");
    }
    d_m3Log(അട്ടി, "\n");
}


സ്ഥിതം
സ്ഥിരം അക്ഷരം* GetOpcodeIndentionString (i32 blockDepth)
{
    blockDepth += 1;

    എങ്കിൽ (blockDepth < 0) {
        blockDepth = 0;
    }

    സ്ഥിതം സ്ഥിരം അക്ഷരം* s_spaces = ".......................................................................................";

    സ്ഥിരം അക്ഷരം* indent = s_spaces + strlen(s_spaces);
    indent -= (blockDepth * 2);
    എങ്കിൽ (indent < s_spaces) {
        indent = s_spaces;
    }

    തിരികെ indent;
}


സ്ഥിരം അക്ഷരം* get_indention_string (IM3Compilation o)
{
    തിരികെ GetOpcodeIndentionString(o->block.depth + 4);
}


ശൂന്യം log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 depth = o->block.depth;
    എങ്കിൽ (i_opcode == c_waOp_end or i_opcode == c_waOp_else) {
        depth--;
    }
    (ശൂന്യം)depth;

    m3log(compile, "%4d | 0x%02x  %s %s", o->അക്കം_Opcodes++, i_opcode, GetOpcodeIndentionString(depth), GetOpInfo(i_opcode)->name);
}


ശൂന്യം log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = find_operation_info(i_operation);

    d_m3Log(emit, "");
    എങ്കിൽ (i.info) {
        printf("%p: %s\n", GetPagePC(o->page), i.info->name);
    } അല്ലെങ്കിൽ {
        printf("not found: %p\n", i_operation);
    }
}

#endif // DEBUG


#എങ്കിൽ d_m3EnableOpProfiling

തരനിർവചനം ഘടന M3ProfilerSlot {
    cstr_t opName;
    u64    hit_എണ്ണം;
} M3ProfilerSlot;

സ്ഥിതം M3ProfilerSlot s_opProfilerCounts[d_m3ProfilerSlotMask + 1] = {};

ശൂന്യം ProfileHit (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3ProfilerSlot* slot = &s_opProfilerCounts[ptr & d_m3ProfilerSlotMask];

    എങ്കിൽ (slot->opName) {
        എങ്കിൽ (slot->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    slot->opName = i_operationName;
    slot->hit_എണ്ണം++;
}


ശൂന്യം m3_PrintProfilerInfo ()
{
    M3ProfilerSlot  dummy;
    M3ProfilerSlot* maxSlot = &dummy;

    ചെയ്യുക {
        maxSlot->hit_എണ്ണം = 0;

        വേണ്ടി (u32 i = 0; i <= d_m3ProfilerSlotMask; ++i) {
            M3ProfilerSlot* slot = &s_opProfilerCounts[i];

            എങ്കിൽ (slot->opName) {
                എങ്കിൽ (slot->hit_എണ്ണം > maxSlot->hit_എണ്ണം) {
                    maxSlot = slot;
                }
            }
        }

        എങ്കിൽ (maxSlot->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", maxSlot->hit_എണ്ണം, maxSlot->opName);
            maxSlot->opName = ശൂന്യ;
        }
    } ആകുമ്പോൾ (maxSlot->hit_എണ്ണം);
}

#അല്ലെങ്കിൽ

ശൂന്യം m3_PrintProfilerInfo () {}

#endif
