//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#अन्तर्भाव "m3_env.h"
#अन्तर्भाव "m3_info.h"
#अन्तर्भाव "m3_compile.h"

#जर defined(DEBUG) || (d_m3EnableStrace >= 1)

माप_प्रकार SPrintArg (अक्षर* o_string, माप_प्रकार i_stringBufferSize, voidptr_t i_sp, m3type_t i_type)
{
    पूर्णांक len = 0;

    *o_string = 0;

    जर (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi32, *(i32*)i_sp);
    } अन्यथा जर (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi64, *(i64*)i_sp);
    }
#  जर d_m3HasFloat
    अन्यथा जर (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf32, *(f32*)i_sp);
    } अन्यथा जर (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    परत len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    पूर्णांक ret;

    स्थिर अक्षर string[256];

    अक्षर*   s = string;
    ccstr_t e = string + आकार(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    जर (funcType) {
        u32 अंक_Args = funcType->अंक_Args;

        साठी (u32 i = 0; i < अंक_Args; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[BaseTypeOf(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            जर (i != अंक_Args - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } अन्यथा {
        म_लिखो("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    परत string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    पूर्णांक ret;

    स्थिर अक्षर string[256];

    अक्षर*   s = string;
    ccstr_t e = string + आकार(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 अंक_Rets = i_funcType ? i_funcType->अंक_Rets : 0;

    साठी (u32 i = 0; i < अंक_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        जर (i != अंक_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    परत string;
}

#endif

#ifdef DEBUG

// a central function you can be breakpoint:
शून्य ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message)
{
    म_लिखो("\nexception: '%s' @ %s\n", i_exception, i_message);
    परत;
}


प्रकारव्याख्या संरचना OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

शून्य m3_PrintM3Info ()
{
    म_लिखो("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    म_लिखो(" sizeof M3MemPage     : %u bytes              \n", d_m3DefaultMemPageSize);
    म_लिखो(" sizeof M3Compilation : %zu bytes             \n", आकार(M3Compilation));
    म_लिखो(" sizeof M3Function    : %zu bytes             \n", आकार(M3Function));
    म_लिखो("----------------------------------------------------------------\n\n");
}


शून्य* v_PrintEnvModuleInfo (IM3Module i_module, शून्य* i_info)
{
    u32* io_index = (u32*)i_info;
    म_लिखो(" module [%u]  name: '%s'; funcs: %d  \n", (*io_index)++, i_module->name, i_module->अंक_Functions);

    परत रिक्त;
}


शून्य m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    म_लिखो("\n-- m3 runtime -------------------------------------------------\n");

    म_लिखो(" stack-size: %zu   \n\n", i_runtime->अंक_चळत_Slots * आकार(m3slot_t));

    u32 moduleIndex = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &moduleIndex);

    म_लिखो("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 base = BaseTypeOf(i_m3Type);

    जर (base <= c_m3Type_unknown) {
        परत c_waTypes[base];
    } अन्यथा {
        परत "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
स्थिर
माप_प्रकार SPrintAppend (अक्षर* o_string, माप_प्रकार i_stringBufferSize, माप_प्रकार i_len, ccstr_t i_format, ...)
{
    जर (i_len + 1 >= i_stringBufferSize) {
        परत i_len;
    }

    बहु_सूची args;
    बहु_शुरू(args, i_format);
    पूर्णांक ret = vsnprintf(o_string + i_len, i_stringBufferSize - i_len, i_format, args);
    बहु_पूर्ण(args);

    परत M3_MIN(i_len + (माप_प्रकार)M3_MAX(0, ret), i_stringBufferSize - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    स्थिर अक्षर string[256];

    माप_प्रकार len = SPrintAppend(string, आकार(string), 0, "(");

    साठी (u32 i = 0; i < i_funcType->अंक_Args; ++i) {
        len = SPrintAppend(string, आकार(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, आकार(string), len, ") -> ");

    साठी (u32 i = 0; i < i_funcType->अंक_Rets; ++i) {
        len = SPrintAppend(string, आकार(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    परत string;
}


cstr_t SPrint_मूल्य (शून्य* i_मूल्य, m3type_t i_type)
{
    स्थिर अक्षर string[100];
    SPrintArg(string, 100, (m3stack_t)i_मूल्य, i_type);
    परत string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
स्थिर
bool ScanOperationTable (OpInfo* io_info, IM3OpInfo i_table, u32 i_अंक_Ops, m3opcode_t i_opcodeBase, IM3Operation i_operation)
{
    साठी (u32 i = 0; i < i_अंक_Ops; ++i) {
        IM3OpInfo oi = &i_table[i];

        साठी (u32 o = 0; o < 4; ++o) {
            जर (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcodeBase | i;
                परत true;
            }
        }
    }

    परत false;
}

स्थिर
OpInfo find_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { रिक्त, 0 };

    जर (!i_operation) {
        परत opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    जर (not ScanOperationTable(&opInfo, c_operations, c_अंक_Operations, 0, i_operation)) {
        जर (not ScanOperationTable(&opInfo, c_operationsFC, c_अंक_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  जर d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            ScanOperationTable(&opInfo, c_operationsFE, c_अंक_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    परत opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) शून्य Decode_##FUNC (अक्षर * o_string, माप_प्रकार i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    शून्य* function    = fetch(शून्य*);
    i32   चळत_Offset = fetch(i32);

    snprintf(o_string, i_stringBufferSize, "%p; stack-offset: %d", function, चळत_Offset);
}


d_m3Decoder(Entry)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_stringBufferSize, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    जर (i_operation == i_opInfo->operations[0]) {
        u32 operand = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_stringBufferSize, "offset= slot:%d + immediate:%d", operand, offset);
    }
}


d_m3Decoder(Branch)
{
    शून्य* target = fetch(शून्य*);
    snprintf(o_string, i_stringBufferSize, "%p", target);
}

d_m3Decoder(BranchTable)
{
    u32 slot = fetch(u32);

    माप_प्रकार len = SPrintAppend(o_string, i_stringBufferSize, 0, "slot: %" PRIu32 "; targets: ", slot);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    साठी (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_stringBufferSize, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_stringBufferSize, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 मूल्य  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_stringBufferSize, " slot [%d] = %" PRIu64, offset, मूल्य);
}


#  undef fetch

शून्य DecodeOperation (अक्षर* o_string, माप_प्रकार i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) स्थिती OPCODE: Decode_##FUNC (o_string, i_stringBufferSize, i_opcode, i_operation, i_opInfo, o_pc); थांबा;

    निवड (i_opcode) {
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
शून्य dump_code_page (IM3CodePage i_codePage, pc_t i_startPC)
{
    m3log(code, "code page seq: %d", i_codePage->info.sequence);

    pc_t pc  = i_startPC ? i_startPC : GetPageStartPC(i_codePage);
    pc_t end = GetPagePC(i_codePage);

    m3log(code, "---------------------------------------------------------------------------------------");

    पर्यंत (pc < end) {
        स्थिरांक pc_t operationPC = pc;
        (शून्य)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = find_operation_info(op);

        जर (i.info) {
            अक्षर infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, आकार(infoString), i.opcode, op, i.info, &pc);

            m3log(code, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } अन्यथा {
            m3log(code, "%p | %p", operationPC, op);
        }
    }

    m3log(code, "---------------------------------------------------------------------------------------");

    m3log(code, "free-lines: %d", i_codePage->info.अंक_ओळी - i_codePage->info.ओळ_Index);
}


शून्य dump_type_चळत (IM3Compilation o)
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
    d_m3Log(चळत, "\n");
    d_m3Log(चळत, "        ");
    म_लिखो("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    म_लिखो("\n");

    साठी (u32 p = 1; p <= 2; ++p) {
        d_m3Log(चळत, "        ");

        साठी (u16 i = 0; i < o->चळत_Index; ++i) {
            जर (i > 0 and i == o->चळत_First_Dynamic_Index) {
                म_लिखो("#");
            }

            जर (i == o->block.block_चळत_Index) {
                म_लिखो(">");
            }

            स्थिरांक अक्षर* type = c_waCompactTypes[BaseTypeOf(o->type_चळत[i])];

            स्थिरांक अक्षर* location = "";

            i32 slot = o->wasm_चळत[i];

            जर (IsRegisterSlotAlias(slot)) {
                bool isFp = IsFpRegisterSlotAlias(slot);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                slot = -1;
            } अन्यथा {
                जर (slot < o->slotFirstDynamicIndex) {
                    जर (slot >= o->slotFirstConstIndex) {
                        location = "c";
                    } अन्यथा जर (slot >= o->function->अंक_Ret_And_Arg_Slots) {
                        location = "L";
                    } अन्यथा {
                        location = "a";
                    }
                }
            }

            अक्षर item[100];

            जर (slot >= 0) {
                snprintf(item, आकार(item), "%s%s%d", type, location, slot);
            } अन्यथा {
                snprintf(item, आकार(item), "%s%s", type, location);
            }

            जर (p == 1) {
                माप_प्रकार s = म_माप(item);

                snprintf(item, आकार(item), "%d", i);

                पर्यंत (म_माप(item) < s) {
                    म_जोड़ो(item, " ");
                }
            }

            म_लिखो("|%s ", item);
        }
        म_लिखो("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 maxSlot = GetMaxUsedSlotPlusOne(o);

    जर (maxSlot > o->slotFirstDynamicIndex) {
        d_m3Log(चळत, "                      -");

        साठी (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            म_लिखो("----");
        }

        म_लिखो("\n");

        d_m3Log(चळत, "                 slot |");
        साठी (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            म_लिखो("%3d|", i);
        }

        म_लिखो("\n");
        d_m3Log(चळत, "                alloc |");

        साठी (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            म_लिखो("%3d|", o->m3Slots[i]);
        }

        म_लिखो("\n");
    }
    d_m3Log(चळत, "\n");
}


स्थिर
स्थिरांक अक्षर* GetOpcodeIndentionString (i32 blockDepth)
{
    blockDepth += 1;

    जर (blockDepth < 0) {
        blockDepth = 0;
    }

    स्थिर स्थिरांक अक्षर* s_spaces = ".......................................................................................";

    स्थिरांक अक्षर* indent = s_spaces + म_माप(s_spaces);
    indent -= (blockDepth * 2);
    जर (indent < s_spaces) {
        indent = s_spaces;
    }

    परत indent;
}


स्थिरांक अक्षर* get_indention_string (IM3Compilation o)
{
    परत GetOpcodeIndentionString(o->block.depth + 4);
}


शून्य log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 depth = o->block.depth;
    जर (i_opcode == c_waOp_end or i_opcode == c_waOp_else) {
        depth--;
    }
    (शून्य)depth;

    m3log(compile, "%4d | 0x%02x  %s %s", o->अंक_Opcodes++, i_opcode, GetOpcodeIndentionString(depth), GetOpInfo(i_opcode)->name);
}


शून्य log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = find_operation_info(i_operation);

    d_m3Log(emit, "");
    जर (i.info) {
        म_लिखो("%p: %s\n", GetPagePC(o->page), i.info->name);
    } अन्यथा {
        म_लिखो("not found: %p\n", i_operation);
    }
}

#endif // DEBUG


#जर d_m3EnableOpProfiling

प्रकारव्याख्या संरचना M3ProfilerSlot {
    cstr_t opName;
    u64    hit_मोजणी;
} M3ProfilerSlot;

स्थिर M3ProfilerSlot s_opProfilerCounts[d_m3ProfilerSlotMask + 1] = {};

शून्य ProfileHit (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3ProfilerSlot* slot = &s_opProfilerCounts[ptr & d_m3ProfilerSlotMask];

    जर (slot->opName) {
        जर (slot->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    slot->opName = i_operationName;
    slot->hit_मोजणी++;
}


शून्य m3_PrintProfilerInfo ()
{
    M3ProfilerSlot  dummy;
    M3ProfilerSlot* maxSlot = &dummy;

    करा {
        maxSlot->hit_मोजणी = 0;

        साठी (u32 i = 0; i <= d_m3ProfilerSlotMask; ++i) {
            M3ProfilerSlot* slot = &s_opProfilerCounts[i];

            जर (slot->opName) {
                जर (slot->hit_मोजणी > maxSlot->hit_मोजणी) {
                    maxSlot = slot;
                }
            }
        }

        जर (maxSlot->opName) {
            ख_लिखो(मानक_त्रुटि, "%13" PRIu64 "  %s\n", maxSlot->hit_मोजणी, maxSlot->opName);
            maxSlot->opName = रिक्त;
        }
    } पर्यंत (maxSlot->hit_मोजणी);
}

#अन्यथा

शून्य m3_PrintProfilerInfo () {}

#endif
