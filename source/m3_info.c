//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"

#ಯದಿ defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (ವರ್ಣ* o_string, size_t i_stringBufferSize, voidptr_t i_sp, m3type_t i_type)
{
    ಪೂರ್ಣಾಙ್ಕ len = 0;

    *o_string = 0;

    ಯದಿ (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi32, *(i32*)i_sp);
    } ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi64, *(i64*)i_sp);
    }
#  ಯದಿ d_m3HasFloat
    ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf32, *(f32*)i_sp);
    } ಅನ್ಯಥಾ ಯದಿ (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    ಪ್ರತ್ಯಾಗಮ len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    ಪೂರ್ಣಾಙ್ಕ ret;

    ಸ್ಥಿರ ವರ್ಣ string[256];

    ವರ್ಣ*   s = string;
    ccstr_t e = string + ಪರಿಮಾಣ(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    ಯದಿ (funcType) {
        u32 ಅಂಕೆ_Args = funcType->ಅಂಕೆ_Args;

        ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Args; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[BaseTypeOf(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            ಯದಿ (i != ಅಂಕೆ_Args - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } ಅನ್ಯಥಾ {
        printf("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    ಪ್ರತ್ಯಾಗಮ string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    ಪೂರ್ಣಾಙ್ಕ ret;

    ಸ್ಥಿರ ವರ್ಣ string[256];

    ವರ್ಣ*   s = string;
    ccstr_t e = string + ಪರಿಮಾಣ(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 ಅಂಕೆ_Rets = i_funcType ? i_funcType->ಅಂಕೆ_Rets : 0;

    ಪ್ರತಿ (u32 i = 0; i < ಅಂಕೆ_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        ಯದಿ (i != ಅಂಕೆ_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    ಪ್ರತ್ಯಾಗಮ string;
}

#endif

#ifdef DEBUG

// a central function you can be breakpoint:
ಶೂನ್ಯ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message)
{
    printf("\nexception: '%s' @ %s\n", i_exception, i_message);
    ಪ್ರತ್ಯಾಗಮ;
}


ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

ಶೂನ್ಯ m3_PrintM3Info ()
{
    printf("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    printf(" sizeof M3MemPage     : %u bytes              \n", d_m3DefaultMemPageSize);
    printf(" sizeof M3Compilation : %zu bytes             \n", ಪರಿಮಾಣ(M3Compilation));
    printf(" sizeof M3Function    : %zu bytes             \n", ಪರಿಮಾಣ(M3Function));
    printf("----------------------------------------------------------------\n\n");
}


ಶೂನ್ಯ* v_PrintEnvModuleInfo (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    u32* io_index = (u32*)i_info;
    printf(" module [%u]  name: '%s'; funcs: %d  \n", (*io_index)++, i_module->name, i_module->ಅಂಕೆ_Functions);

    ಪ್ರತ್ಯಾಗಮ ರಿಕ್ತ;
}


ಶೂನ್ಯ m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    printf("\n-- m3 runtime -------------------------------------------------\n");

    printf(" stack-size: %zu   \n\n", i_runtime->ಅಂಕೆ_ರಾಶಿ_Slots * ಪರಿಮಾಣ(m3slot_t));

    u32 moduleIndex = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &moduleIndex);

    printf("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 base = BaseTypeOf(i_m3Type);

    ಯದಿ (base <= c_m3Type_unknown) {
        ಪ್ರತ್ಯಾಗಮ c_waTypes[base];
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
ಸ್ಥಿರ
size_t SPrintAppend (ವರ್ಣ* o_string, size_t i_stringBufferSize, size_t i_len, ccstr_t i_format, ...)
{
    ಯದಿ (i_len + 1 >= i_stringBufferSize) {
        ಪ್ರತ್ಯಾಗಮ i_len;
    }

    va_list args;
    va_start(args, i_format);
    ಪೂರ್ಣಾಙ್ಕ ret = vsnprintf(o_string + i_len, i_stringBufferSize - i_len, i_format, args);
    va_end(args);

    ಪ್ರತ್ಯಾಗಮ M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_stringBufferSize - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    ಸ್ಥಿರ ವರ್ಣ string[256];

    size_t len = SPrintAppend(string, ಪರಿಮಾಣ(string), 0, "(");

    ಪ್ರತಿ (u32 i = 0; i < i_funcType->ಅಂಕೆ_Args; ++i) {
        len = SPrintAppend(string, ಪರಿಮಾಣ(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, ಪರಿಮಾಣ(string), len, ") -> ");

    ಪ್ರತಿ (u32 i = 0; i < i_funcType->ಅಂಕೆ_Rets; ++i) {
        len = SPrintAppend(string, ಪರಿಮಾಣ(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    ಪ್ರತ್ಯಾಗಮ string;
}


cstr_t SPrint_ಮೌಲ್ಯ (ಶೂನ್ಯ* i_ಮೌಲ್ಯ, m3type_t i_type)
{
    ಸ್ಥಿರ ವರ್ಣ string[100];
    SPrintArg(string, 100, (m3stack_t)i_ಮೌಲ್ಯ, i_type);
    ಪ್ರತ್ಯಾಗಮ string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
ಸ್ಥಿರ
bool ScanOperationTable (OpInfo* io_info, IM3OpInfo i_table, u32 i_ಅಂಕೆ_Ops, m3opcode_t i_opcodeBase, IM3Operation i_operation)
{
    ಪ್ರತಿ (u32 i = 0; i < i_ಅಂಕೆ_Ops; ++i) {
        IM3OpInfo oi = &i_table[i];

        ಪ್ರತಿ (u32 o = 0; o < 4; ++o) {
            ಯದಿ (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcodeBase | i;
                ಪ್ರತ್ಯಾಗಮ true;
            }
        }
    }

    ಪ್ರತ್ಯಾಗಮ false;
}

ಸ್ಥಿರ
OpInfo find_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { ರಿಕ್ತ, 0 };

    ಯದಿ (!i_operation) {
        ಪ್ರತ್ಯಾಗಮ opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    ಯದಿ (not ScanOperationTable(&opInfo, c_operations, c_ಅಂಕೆ_Operations, 0, i_operation)) {
        ಯದಿ (not ScanOperationTable(&opInfo, c_operationsFC, c_ಅಂಕೆ_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  ಯದಿ d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            ScanOperationTable(&opInfo, c_operationsFE, c_ಅಂಕೆ_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    ಪ್ರತ್ಯಾಗಮ opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) ಶೂನ್ಯ Decode_##FUNC (ವರ್ಣ * o_string, size_t i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    ಶೂನ್ಯ* function    = fetch(ಶೂನ್ಯ*);
    i32   ರಾಶಿ_Offset = fetch(i32);

    snprintf(o_string, i_stringBufferSize, "%p; stack-offset: %d", function, ರಾಶಿ_Offset);
}


d_m3Decoder(Entry)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_stringBufferSize, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    ಯದಿ (i_operation == i_opInfo->operations[0]) {
        u32 operand = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_stringBufferSize, "offset= slot:%d + immediate:%d", operand, offset);
    }
}


d_m3Decoder(Branch)
{
    ಶೂನ್ಯ* target = fetch(ಶೂನ್ಯ*);
    snprintf(o_string, i_stringBufferSize, "%p", target);
}

d_m3Decoder(BranchTable)
{
    u32 slot = fetch(u32);

    size_t len = SPrintAppend(o_string, i_stringBufferSize, 0, "slot: %" PRIu32 "; targets: ", slot);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    ಪ್ರತಿ (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_stringBufferSize, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_stringBufferSize, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 ಮೌಲ್ಯ  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_stringBufferSize, " slot [%d] = %" PRIu64, offset, ಮೌಲ್ಯ);
}


#  undef fetch

ಶೂನ್ಯ DecodeOperation (ವರ್ಣ* o_string, size_t i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) ಪಕ್ಷ OPCODE: Decode_##FUNC (o_string, i_stringBufferSize, i_opcode, i_operation, i_opInfo, o_pc); ಭಙ್ಗ;

    ವಿಕಲ್ಪ (i_opcode) {
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
ಶೂನ್ಯ dump_code_page (IM3CodePage i_codePage, pc_t i_startPC)
{
    m3log(code, "code page seq: %d", i_codePage->info.sequence);

    pc_t pc  = i_startPC ? i_startPC : GetPageStartPC(i_codePage);
    pc_t end = GetPagePC(i_codePage);

    m3log(code, "---------------------------------------------------------------------------------------");

    ಯಾವತ್ (pc < end) {
        ಧ್ರುವ pc_t operationPC = pc;
        (ಶೂನ್ಯ)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = find_operation_info(op);

        ಯದಿ (i.info) {
            ವರ್ಣ infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, ಪರಿಮಾಣ(infoString), i.opcode, op, i.info, &pc);

            m3log(code, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } ಅನ್ಯಥಾ {
            m3log(code, "%p | %p", operationPC, op);
        }
    }

    m3log(code, "---------------------------------------------------------------------------------------");

    m3log(code, "free-lines: %d", i_codePage->info.ಅಂಕೆ_ಸಾಲುಗಳು - i_codePage->info.ಸಾಲು_Index);
}


ಶೂನ್ಯ dump_type_ರಾಶಿ (IM3Compilation o)
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
    d_m3Log(ರಾಶಿ, "\n");
    d_m3Log(ರಾಶಿ, "        ");
    printf("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    printf("\n");

    ಪ್ರತಿ (u32 p = 1; p <= 2; ++p) {
        d_m3Log(ರಾಶಿ, "        ");

        ಪ್ರತಿ (u16 i = 0; i < o->ರಾಶಿ_Index; ++i) {
            ಯದಿ (i > 0 and i == o->ರಾಶಿ_First_Dynamic_Index) {
                printf("#");
            }

            ಯದಿ (i == o->block.block_ರಾಶಿ_Index) {
                printf(">");
            }

            ಧ್ರುವ ವರ್ಣ* type = c_waCompactTypes[BaseTypeOf(o->type_ರಾಶಿ[i])];

            ಧ್ರುವ ವರ್ಣ* location = "";

            i32 slot = o->wasm_ರಾಶಿ[i];

            ಯದಿ (IsRegisterSlotAlias(slot)) {
                bool isFp = IsFpRegisterSlotAlias(slot);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                slot = -1;
            } ಅನ್ಯಥಾ {
                ಯದಿ (slot < o->slotFirstDynamicIndex) {
                    ಯದಿ (slot >= o->slotFirstConstIndex) {
                        location = "c";
                    } ಅನ್ಯಥಾ ಯದಿ (slot >= o->function->ಅಂಕೆ_Ret_And_Arg_Slots) {
                        location = "L";
                    } ಅನ್ಯಥಾ {
                        location = "a";
                    }
                }
            }

            ವರ್ಣ item[100];

            ಯದಿ (slot >= 0) {
                snprintf(item, ಪರಿಮಾಣ(item), "%s%s%d", type, location, slot);
            } ಅನ್ಯಥಾ {
                snprintf(item, ಪರಿಮಾಣ(item), "%s%s", type, location);
            }

            ಯದಿ (p == 1) {
                size_t s = strlen(item);

                snprintf(item, ಪರಿಮಾಣ(item), "%d", i);

                ಯಾವತ್ (strlen(item) < s) {
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

    ಯದಿ (maxSlot > o->slotFirstDynamicIndex) {
        d_m3Log(ರಾಶಿ, "                      -");

        ಪ್ರತಿ (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("----");
        }

        printf("\n");

        d_m3Log(ರಾಶಿ, "                 slot |");
        ಪ್ರತಿ (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("%3d|", i);
        }

        printf("\n");
        d_m3Log(ರಾಶಿ, "                alloc |");

        ಪ್ರತಿ (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("%3d|", o->m3Slots[i]);
        }

        printf("\n");
    }
    d_m3Log(ರಾಶಿ, "\n");
}


ಸ್ಥಿರ
ಧ್ರುವ ವರ್ಣ* GetOpcodeIndentionString (i32 blockDepth)
{
    blockDepth += 1;

    ಯದಿ (blockDepth < 0) {
        blockDepth = 0;
    }

    ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* s_spaces = ".......................................................................................";

    ಧ್ರುವ ವರ್ಣ* indent = s_spaces + strlen(s_spaces);
    indent -= (blockDepth * 2);
    ಯದಿ (indent < s_spaces) {
        indent = s_spaces;
    }

    ಪ್ರತ್ಯಾಗಮ indent;
}


ಧ್ರುವ ವರ್ಣ* get_indention_string (IM3Compilation o)
{
    ಪ್ರತ್ಯಾಗಮ GetOpcodeIndentionString(o->block.depth + 4);
}


ಶೂನ್ಯ log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 depth = o->block.depth;
    ಯದಿ (i_opcode == c_waOp_end or i_opcode == c_waOp_else) {
        depth--;
    }
    (ಶೂನ್ಯ)depth;

    m3log(compile, "%4d | 0x%02x  %s %s", o->ಅಂಕೆ_Opcodes++, i_opcode, GetOpcodeIndentionString(depth), GetOpInfo(i_opcode)->name);
}


ಶೂನ್ಯ log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = find_operation_info(i_operation);

    d_m3Log(emit, "");
    ಯದಿ (i.info) {
        printf("%p: %s\n", GetPagePC(o->page), i.info->name);
    } ಅನ್ಯಥಾ {
        printf("not found: %p\n", i_operation);
    }
}

#endif // DEBUG


#ಯದಿ d_m3EnableOpProfiling

ಪ್ರಕಾರನಿರ್ದೇಶ ಸಂರಚನಾ M3ProfilerSlot {
    cstr_t opName;
    u64    hit_ಎಣಿಕೆ;
} M3ProfilerSlot;

ಸ್ಥಿರ M3ProfilerSlot s_opProfilerCounts[d_m3ProfilerSlotMask + 1] = {};

ಶೂನ್ಯ ProfileHit (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3ProfilerSlot* slot = &s_opProfilerCounts[ptr & d_m3ProfilerSlotMask];

    ಯದಿ (slot->opName) {
        ಯದಿ (slot->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    slot->opName = i_operationName;
    slot->hit_ಎಣಿಕೆ++;
}


ಶೂನ್ಯ m3_PrintProfilerInfo ()
{
    M3ProfilerSlot  dummy;
    M3ProfilerSlot* maxSlot = &dummy;

    ಕುರು {
        maxSlot->hit_ಎಣಿಕೆ = 0;

        ಪ್ರತಿ (u32 i = 0; i <= d_m3ProfilerSlotMask; ++i) {
            M3ProfilerSlot* slot = &s_opProfilerCounts[i];

            ಯದಿ (slot->opName) {
                ಯದಿ (slot->hit_ಎಣಿಕೆ > maxSlot->hit_ಎಣಿಕೆ) {
                    maxSlot = slot;
                }
            }
        }

        ಯದಿ (maxSlot->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", maxSlot->hit_ಎಣಿಕೆ, maxSlot->opName);
            maxSlot->opName = ರಿಕ್ತ;
        }
    } ಯಾವತ್ (maxSlot->hit_ಎಣಿಕೆ);
}

#ಅನ್ಯಥಾ

ಶೂನ್ಯ m3_PrintProfilerInfo () {}

#endif
