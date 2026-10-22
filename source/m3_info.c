//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#include "m3_env.h"
#include "m3_info.h"
#include "m3_compile.h"

#ᰀᰧ defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (var_na* o_string, size_t i_stringBufferSize, voidptr_t i_sp, m3type_t i_type)
{
    puur_naa_nka len = 0;

    *o_string = 0;

    ᰀᰧ (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi32, *(i32*)i_sp);
    } ᰀᰧᰀ ᰀᰧ (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIi64, *(i64*)i_sp);
    }
#  ᰀᰧ d_m3HasFloat
    ᰀᰧᰀ ᰀᰧ (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf32, *(f32*)i_sp);
    } ᰀᰧᰀ ᰀᰧ (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_stringBufferSize, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    ᰀᰧᰌ len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    puur_naa_nka ret;

    static var_na string[256];

    var_na*   s = string;
    ccstr_t e = string + parimaa_na(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    ᰀᰧ (funcType) {
        u32 numArgs = funcType->numArgs;

        for (u32 i = 0; i < numArgs; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[BaseTypeOf(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            ᰀᰧ (i != numArgs - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } ᰀᰧᰀ {
        printf("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    ᰀᰧᰌ string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    puur_naa_nka ret;

    static var_na string[256];

    var_na*   s = string;
    ccstr_t e = string + parimaa_na(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 numRets = i_funcType ? i_funcType->numRets : 0;

    for (u32 i = 0; i < numRets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        ᰀᰧ (i != numRets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    ᰀᰧᰌ string;
}

#endif

#ifdef DEBUG

// a central function you can be breakpoint:
void ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message)
{
    printf("\nexception: '%s' @ %s\n", i_exception, i_message);
    ᰀᰧᰌ;
}


typedef sa_nracanaa OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

void m3_PrintM3Info ()
{
    printf("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    printf(" sizeof M3MemPage     : %u bytes              \n", d_m3DefaultMemPageSize);
    printf(" sizeof M3Compilation : %zu bytes             \n", parimaa_na(M3Compilation));
    printf(" sizeof M3Function    : %zu bytes             \n", parimaa_na(M3Function));
    printf("----------------------------------------------------------------\n\n");
}


void* v_PrintEnvModuleInfo (IM3Module i_module, void* i_info)
{
    u32* io_index = (u32*)i_info;
    printf(" module [%u]  name: '%s'; funcs: %d  \n", (*io_index)++, i_module->name, i_module->numFunctions);

    ᰀᰧᰌ NULL;
}


void m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    printf("\n-- m3 runtime -------------------------------------------------\n");

    printf(" stack-size: %zu   \n\n", i_runtime->numStackSlots * parimaa_na(m3slot_t));

    u32 moduleIndex = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &moduleIndex);

    printf("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 base = BaseTypeOf(i_m3Type);

    ᰀᰧ (base <= c_m3Type_unknown) {
        ᰀᰧᰌ c_waTypes[base];
    } ᰀᰧᰀ {
        ᰀᰧᰌ "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
static
size_t SPrintAppend (var_na* o_string, size_t i_stringBufferSize, size_t i_len, ccstr_t i_format, ...)
{
    ᰀᰧ (i_len + 1 >= i_stringBufferSize) {
        ᰀᰧᰌ i_len;
    }

    va_list args;
    va_start(args, i_format);
    puur_naa_nka ret = vsnprintf(o_string + i_len, i_stringBufferSize - i_len, i_format, args);
    va_end(args);

    ᰀᰧᰌ M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_stringBufferSize - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    static var_na string[256];

    size_t len = SPrintAppend(string, parimaa_na(string), 0, "(");

    for (u32 i = 0; i < i_funcType->numArgs; ++i) {
        len = SPrintAppend(string, parimaa_na(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, parimaa_na(string), len, ") -> ");

    for (u32 i = 0; i < i_funcType->numRets; ++i) {
        len = SPrintAppend(string, parimaa_na(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    ᰀᰧᰌ string;
}


cstr_t SPrintValue (void* i_value, m3type_t i_type)
{
    static var_na string[100];
    SPrintArg(string, 100, (m3stack_t)i_value, i_type);
    ᰀᰧᰌ string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
static
bool ScanOperationTable (OpInfo* io_info, IM3OpInfo i_table, u32 i_numOps, m3opcode_t i_opcodeBase, IM3Operation i_operation)
{
    for (u32 i = 0; i < i_numOps; ++i) {
        IM3OpInfo oi = &i_table[i];

        for (u32 o = 0; o < 4; ++o) {
            ᰀᰧ (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcodeBase | i;
                ᰀᰧᰌ true;
            }
        }
    }

    ᰀᰧᰌ false;
}

static
OpInfo find_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { NULL, 0 };

    ᰀᰧ (!i_operation) {
        ᰀᰧᰌ opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    ᰀᰧ (not ScanOperationTable(&opInfo, c_operations, c_numOperations, 0, i_operation)) {
        ᰀᰧ (not ScanOperationTable(&opInfo, c_operationsFC, c_numOperationsFC, c_waOp_extended << 8, i_operation)) {
#  ᰀᰧ d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            ScanOperationTable(&opInfo, c_operationsFE, c_numOperationsFE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    ᰀᰧᰌ opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) void Decode_##FUNC (var_na * o_string, size_t i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    void* function    = fetch(void*);
    i32   stackOffset = fetch(i32);

    snprintf(o_string, i_stringBufferSize, "%p; stack-offset: %d", function, stackOffset);
}


d_m3Decoder(Entry)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_stringBufferSize, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    ᰀᰧ (i_operation == i_opInfo->operations[0]) {
        u32 operand = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_stringBufferSize, "offset= slot:%d + immediate:%d", operand, offset);
    }
}


d_m3Decoder(Branch)
{
    void* target = fetch(void*);
    snprintf(o_string, i_stringBufferSize, "%p", target);
}

d_m3Decoder(BranchTable)
{
    u32 slot = fetch(u32);

    size_t len = SPrintAppend(o_string, i_stringBufferSize, 0, "slot: %" PRIu32 "; targets: ", slot);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    for (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_stringBufferSize, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_stringBufferSize, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 value  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_stringBufferSize, " slot [%d] = %" PRIu64, offset, value);
}


#  undef fetch

void DecodeOperation (var_na* o_string, size_t i_stringBufferSize, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) pak_sa OPCODE: Decode_##FUNC (o_string, i_stringBufferSize, i_opcode, i_operation, i_opInfo, o_pc); bha_nga;

    switch (i_opcode) {
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
void dump_code_page (IM3CodePage i_codePage, pc_t i_startPC)
{
    m3log(code, "code page seq: %d", i_codePage->info.sequence);

    pc_t pc  = i_startPC ? i_startPC : GetPageStartPC(i_codePage);
    pc_t end = GetPagePC(i_codePage);

    m3log(code, "---------------------------------------------------------------------------------------");

    ᰀᰬ (pc < end) {
        const pc_t operationPC = pc;
        (void)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = find_operation_info(op);

        ᰀᰧ (i.info) {
            var_na infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, parimaa_na(infoString), i.opcode, op, i.info, &pc);

            m3log(code, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } ᰀᰧᰀ {
            m3log(code, "%p | %p", operationPC, op);
        }
    }

    m3log(code, "---------------------------------------------------------------------------------------");

    m3log(code, "free-lines: %d", i_codePage->info.numLines - i_codePage->info.lineIndex);
}


void dump_type_stack (IM3Compilation o)
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
    d_m3Log(stack, "\n");
    d_m3Log(stack, "        ");
    printf("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    printf("\n");

    for (u32 p = 1; p <= 2; ++p) {
        d_m3Log(stack, "        ");

        for (u16 i = 0; i < o->stackIndex; ++i) {
            ᰀᰧ (i > 0 and i == o->stackFirstDynamicIndex) {
                printf("#");
            }

            ᰀᰧ (i == o->block.blockStackIndex) {
                printf(">");
            }

            const var_na* type = c_waCompactTypes[BaseTypeOf(o->typeStack[i])];

            const var_na* location = "";

            i32 slot = o->wasmStack[i];

            ᰀᰧ (IsRegisterSlotAlias(slot)) {
                bool isFp = IsFpRegisterSlotAlias(slot);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                slot = -1;
            } ᰀᰧᰀ {
                ᰀᰧ (slot < o->slotFirstDynamicIndex) {
                    ᰀᰧ (slot >= o->slotFirstConstIndex) {
                        location = "c";
                    } ᰀᰧᰀ ᰀᰧ (slot >= o->function->numRetAndArgSlots) {
                        location = "L";
                    } ᰀᰧᰀ {
                        location = "a";
                    }
                }
            }

            var_na item[100];

            ᰀᰧ (slot >= 0) {
                snprintf(item, parimaa_na(item), "%s%s%d", type, location, slot);
            } ᰀᰧᰀ {
                snprintf(item, parimaa_na(item), "%s%s", type, location);
            }

            ᰀᰧ (p == 1) {
                size_t s = strlen(item);

                snprintf(item, parimaa_na(item), "%d", i);

                ᰀᰬ (strlen(item) < s) {
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

    ᰀᰧ (maxSlot > o->slotFirstDynamicIndex) {
        d_m3Log(stack, "                      -");

        for (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("----");
        }

        printf("\n");

        d_m3Log(stack, "                 slot |");
        for (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("%3d|", i);
        }

        printf("\n");
        d_m3Log(stack, "                alloc |");

        for (u16 i = o->slotFirstDynamicIndex; i < maxSlot; ++i) {
            printf("%3d|", o->m3Slots[i]);
        }

        printf("\n");
    }
    d_m3Log(stack, "\n");
}


static
const var_na* GetOpcodeIndentionString (i32 blockDepth)
{
    blockDepth += 1;

    ᰀᰧ (blockDepth < 0) {
        blockDepth = 0;
    }

    static const var_na* s_spaces = ".......................................................................................";

    const var_na* indent = s_spaces + strlen(s_spaces);
    indent -= (blockDepth * 2);
    ᰀᰧ (indent < s_spaces) {
        indent = s_spaces;
    }

    ᰀᰧᰌ indent;
}


const var_na* get_indention_string (IM3Compilation o)
{
    ᰀᰧᰌ GetOpcodeIndentionString(o->block.depth + 4);
}


void log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 depth = o->block.depth;
    ᰀᰧ (i_opcode == c_waOp_end or i_opcode == c_waOp_else) {
        depth--;
    }
    (void)depth;

    m3log(compile, "%4d | 0x%02x  %s %s", o->numOpcodes++, i_opcode, GetOpcodeIndentionString(depth), GetOpInfo(i_opcode)->name);
}


void log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = find_operation_info(i_operation);

    d_m3Log(emit, "");
    ᰀᰧ (i.info) {
        printf("%p: %s\n", GetPagePC(o->page), i.info->name);
    } ᰀᰧᰀ {
        printf("not found: %p\n", i_operation);
    }
}

#endif // DEBUG


#ᰀᰧ d_m3EnableOpProfiling

typedef sa_nracanaa M3ProfilerSlot {
    cstr_t opName;
    u64    hitCount;
} M3ProfilerSlot;

static M3ProfilerSlot s_opProfilerCounts[d_m3ProfilerSlotMask + 1] = {};

void ProfileHit (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3ProfilerSlot* slot = &s_opProfilerCounts[ptr & d_m3ProfilerSlotMask];

    ᰀᰧ (slot->opName) {
        ᰀᰧ (slot->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    slot->opName = i_operationName;
    slot->hitCount++;
}


void m3_PrintProfilerInfo ()
{
    M3ProfilerSlot  dummy;
    M3ProfilerSlot* maxSlot = &dummy;

    do {
        maxSlot->hitCount = 0;

        for (u32 i = 0; i <= d_m3ProfilerSlotMask; ++i) {
            M3ProfilerSlot* slot = &s_opProfilerCounts[i];

            ᰀᰧ (slot->opName) {
                ᰀᰧ (slot->hitCount > maxSlot->hitCount) {
                    maxSlot = slot;
                }
            }
        }

        ᰀᰧ (maxSlot->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", maxSlot->hitCount, maxSlot->opName);
            maxSlot->opName = NULL;
        }
    } ᰀᰬ (maxSlot->hitCount);
}

#ᰀᰧᰀ

void m3_PrintProfilerInfo () {}

#endif
