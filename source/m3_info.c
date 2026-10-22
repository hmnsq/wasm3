//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_env.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_info.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "m3_compile.h"

#ਜੇ defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (ਅੱਖਰ* o_string, size_t i_string_ਬਫਰ_ਮਾਪ, voidptr_t i_sp, m3type_t i_type)
{
    ਪੂਰਨ len = 0;

    *o_string = 0;

    ਜੇ (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "%" PRIi32, *(i32*)i_sp);
    } ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "%" PRIi64, *(i64*)i_sp);
    }
#  ਜੇ d_m3HasFloat
    ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "%" PRIf32, *(f32*)i_sp);
    } ਨਹੀਂਤਾਂ ਜੇ (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    ਵਾਪਸ len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    ਪੂਰਨ ret;

    ਸਥਿਰ ਅੱਖਰ string[256];

    ਅੱਖਰ*   s = string;
    ccstr_t e = string + ਆਕਾਰ(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    ਜੇ (funcType) {
        u32 ਅੰਕ_ਦਲੀਲਾਂ = funcType->ਅੰਕ_ਦਲੀਲਾਂ;

        ਲਈ (u32 i = 0; i < ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[ਅਧਾਰ_Type_ਦਾ(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            ਜੇ (i != ਅੰਕ_ਦਲੀਲਾਂ - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } ਨਹੀਂਤਾਂ {
        ਲਿਖੋ("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    ਵਾਪਸ string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    ਪੂਰਨ ret;

    ਸਥਿਰ ਅੱਖਰ string[256];

    ਅੱਖਰ*   s = string;
    ccstr_t e = string + ਆਕਾਰ(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 ਅੰਕ_Rets = i_funcType ? i_funcType->ਅੰਕ_Rets : 0;

    ਲਈ (u32 i = 0; i < ਅੰਕ_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        ਜੇ (i != ਅੰਕ_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    ਵਾਪਸ string;
}

#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG

// a central function you can be breakpoint:
ਖਾਲੀ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_ਸੁਨੇਹਾ)
{
    ਲਿਖੋ("\nexception: '%s' @ %s\n", i_exception, i_ਸੁਨੇਹਾ);
    ਵਾਪਸ;
}


ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

ਖਾਲੀ m3_PrintM3Info ()
{
    ਲਿਖੋ("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    ਲਿਖੋ(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_ਪੰਨਾ_ਮਾਪ);
    ਲਿਖੋ(" sizeof M3Compilation : %zu bytes             \n", ਆਕਾਰ(M3Compilation));
    ਲਿਖੋ(" sizeof M3Function    : %zu bytes             \n", ਆਕਾਰ(M3Function));
    ਲਿਖੋ("----------------------------------------------------------------\n\n");
}


ਖਾਲੀ* v_PrintEnvModuleInfo (IM3Module i_module, ਖਾਲੀ* i_info)
{
    u32* io_ਸੂਚਕਾਂਕ = (u32*)i_info;
    ਲਿਖੋ(" module [%u]  name: '%s'; funcs: %d  \n", (*io_ਸੂਚਕਾਂਕ)++, i_module->name, i_module->ਅੰਕ_Functions);

    ਵਾਪਸ ਨਿਰਾ;
}


ਖਾਲੀ m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    ਲਿਖੋ("\n-- m3 runtime -------------------------------------------------\n");

    ਲਿਖੋ(" stack-size: %zu   \n\n", i_runtime->ਅੰਕ_ਢੇਰ_Slots * ਆਕਾਰ(m3slot_t));

    u32 module_ਸੂਚਕਾਂਕ = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_ਸੂਚਕਾਂਕ);

    ਲਿਖੋ("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 ਅਧਾਰ = ਅਧਾਰ_Type_ਦਾ(i_m3Type);

    ਜੇ (ਅਧਾਰ <= c_m3Type_unknown) {
        ਵਾਪਸ c_waTypes[ਅਧਾਰ];
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
ਸਥਿਰ
size_t SPrintAppend (ਅੱਖਰ* o_string, size_t i_string_ਬਫਰ_ਮਾਪ, size_t i_len, ccstr_t i_format, ...)
{
    ਜੇ (i_len + 1 >= i_string_ਬਫਰ_ਮਾਪ) {
        ਵਾਪਸ i_len;
    }

    va_list ਦਲੀਲਾਂ;
    va_start(ਦਲੀਲਾਂ, i_format);
    ਪੂਰਨ ret = vsnprintf(o_string + i_len, i_string_ਬਫਰ_ਮਾਪ - i_len, i_format, ਦਲੀਲਾਂ);
    va_end(ਦਲੀਲਾਂ);

    ਵਾਪਸ M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_ਬਫਰ_ਮਾਪ - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    ਸਥਿਰ ਅੱਖਰ string[256];

    size_t len = SPrintAppend(string, ਆਕਾਰ(string), 0, "(");

    ਲਈ (u32 i = 0; i < i_funcType->ਅੰਕ_ਦਲੀਲਾਂ; ++i) {
        len = SPrintAppend(string, ਆਕਾਰ(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, ਆਕਾਰ(string), len, ") -> ");

    ਲਈ (u32 i = 0; i < i_funcType->ਅੰਕ_Rets; ++i) {
        len = SPrintAppend(string, ਆਕਾਰ(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    ਵਾਪਸ string;
}


cstr_t SPrint_ਮੁੱਲ (ਖਾਲੀ* i_ਮੁੱਲ, m3type_t i_type)
{
    ਸਥਿਰ ਅੱਖਰ string[100];
    SPrintArg(string, 100, (m3stack_t)i_ਮੁੱਲ, i_type);
    ਵਾਪਸ string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
ਸਥਿਰ
bool ਜਾਂਚ_Operation_ਸਾਰਣੀ (OpInfo* io_info, IM3OpInfo i_ਸਾਰਣੀ, u32 i_ਅੰਕ_Ops, m3opcode_t i_opcode_ਅਧਾਰ, IM3Operation i_operation)
{
    ਲਈ (u32 i = 0; i < i_ਅੰਕ_Ops; ++i) {
        IM3OpInfo oi = &i_ਸਾਰਣੀ[i];

        ਲਈ (u32 o = 0; o < 4; ++o) {
            ਜੇ (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_ਅਧਾਰ | i;
                ਵਾਪਸ true;
            }
        }
    }

    ਵਾਪਸ false;
}

ਸਥਿਰ
OpInfo ਲੱਭੋ_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { ਨਿਰਾ, 0 };

    ਜੇ (!i_operation) {
        ਵਾਪਸ opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    ਜੇ (not ਜਾਂਚ_Operation_ਸਾਰਣੀ(&opInfo, c_operations, c_ਅੰਕ_Operations, 0, i_operation)) {
        ਜੇ (not ਜਾਂਚ_Operation_ਸਾਰਣੀ(&opInfo, c_operationsFC, c_ਅੰਕ_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  ਜੇ d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            ਜਾਂਚ_Operation_ਸਾਰਣੀ(&opInfo, c_operationsFE, c_ਅੰਕ_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    ਵਾਪਸ opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) ਖਾਲੀ Decode_##FUNC (ਅੱਖਰ * o_string, size_t i_string_ਬਫਰ_ਮਾਪ, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    ਖਾਲੀ* function    = fetch(ਖਾਲੀ*);
    i32   ਢੇਰ_ਵਿੱਥ = fetch(i32);

    snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "%p; stack-offset: %d", function, ਢੇਰ_ਵਿੱਥ);
}


d_m3Decoder(ਇੰਦਰਾਜ_2)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    ਜੇ (i_operation == i_opInfo->operations[0]) {
        u32 ਸੰਕਾਰਜ = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "offset= slot:%d + immediate:%d", ਸੰਕਾਰਜ, offset);
    }
}


d_m3Decoder(Branch)
{
    ਖਾਲੀ* ਨਿਸ਼ਾਨਾ = fetch(ਖਾਲੀ*);
    snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, "%p", ਨਿਸ਼ਾਨਾ);
}

d_m3Decoder(Branch_ਸਾਰਣੀ)
{
    u32 ਥਾਂ = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_ਬਫਰ_ਮਾਪ, 0, "slot: %" PRIu32 "; targets: ", ਥਾਂ);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    ਲਈ (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_ਬਫਰ_ਮਾਪ, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_ਬਫਰ_ਮਾਪ, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 ਮੁੱਲ  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_ਬਫਰ_ਮਾਪ, " slot [%d] = %" PRIu64, offset, ਮੁੱਲ);
}


#  undef fetch

ਖਾਲੀ DecodeOperation (ਅੱਖਰ* o_string, size_t i_string_ਬਫਰ_ਮਾਪ, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) ਹਾਲਤ OPCODE: Decode_##FUNC (o_string, i_string_ਬਫਰ_ਮਾਪ, i_opcode, i_operation, i_opInfo, o_pc); ਰੋਕੋ;

    ਚੋਣ (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, ਇੰਦਰਾਜ_2)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_ਸਾਰਣੀ, Branch_ਸਾਰਣੀ)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
ਖਾਲੀ dump_ਕੋਡ_ਪੰਨਾ (IM3_ਕੋਡ_ਪੰਨਾ i_ਕੋਡ_ਪੰਨਾ, pc_t i_ਸ਼ੁਰੂ_PC)
{
    m3log(ਕੋਡ, "code page seq: %d", i_ਕੋਡ_ਪੰਨਾ->info.sequence);

    pc_t pc  = i_ਸ਼ੁਰੂ_PC ? i_ਸ਼ੁਰੂ_PC : Get_ਪੰਨਾ_ਸ਼ੁਰੂ_PC(i_ਕੋਡ_ਪੰਨਾ);
    pc_t ਸਿਰਾ = Get_ਪੰਨਾ_PC(i_ਕੋਡ_ਪੰਨਾ);

    m3log(ਕੋਡ, "---------------------------------------------------------------------------------------");

    ਜਦੋਂ (pc < ਸਿਰਾ) {
        ਅਚਲ pc_t operationPC = pc;
        (ਖਾਲੀ)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = ਲੱਭੋ_operation_info(op);

        ਜੇ (i.info) {
            ਅੱਖਰ infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, ਆਕਾਰ(infoString), i.opcode, op, i.info, &pc);

            m3log(ਕੋਡ, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } ਨਹੀਂਤਾਂ {
            m3log(ਕੋਡ, "%p | %p", operationPC, op);
        }
    }

    m3log(ਕੋਡ, "---------------------------------------------------------------------------------------");

    m3log(ਕੋਡ, "free-lines: %d", i_ਕੋਡ_ਪੰਨਾ->info.ਅੰਕ_ਸਤਰਾਂ - i_ਕੋਡ_ਪੰਨਾ->info.ਸਤਰ_ਸੂਚਕਾਂਕ);
}


ਖਾਲੀ dump_type_ਢੇਰ (IM3Compilation o)
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
    i32 regAllocated[2] = { (i32)Is_ਰਜਿਸਟਰ_Allocated(o, 0), (i32)Is_ਰਜਿਸਟਰ_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(ਢੇਰ, "\n");
    d_m3Log(ਢੇਰ, "        ");
    ਲਿਖੋ("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    ਲਿਖੋ("\n");

    ਲਈ (u32 p = 1; p <= 2; ++p) {
        d_m3Log(ਢੇਰ, "        ");

        ਲਈ (u16 i = 0; i < o->ਢੇਰ_ਸੂਚਕਾਂਕ; ++i) {
            ਜੇ (i > 0 and i == o->ਢੇਰ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
                ਲਿਖੋ("#");
            }

            ਜੇ (i == o->ਖੰਡ.ਖੰਡ_ਢੇਰ_ਸੂਚਕਾਂਕ) {
                ਲਿਖੋ(">");
            }

            ਅਚਲ ਅੱਖਰ* type = c_waCompactTypes[ਅਧਾਰ_Type_ਦਾ(o->type_ਢੇਰ[i])];

            ਅਚਲ ਅੱਖਰ* location = "";

            i32 ਥਾਂ = o->wasm_ਢੇਰ[i];

            ਜੇ (Is_ਰਜਿਸਟਰ_ਥਾਂ_Alias(ਥਾਂ)) {
                bool isFp = Is_Fp_ਰਜਿਸਟਰ_ਥਾਂ_Alias(ਥਾਂ);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                ਥਾਂ = -1;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (ਥਾਂ < o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
                    ਜੇ (ਥਾਂ >= o->ਥਾਂ_ਪਹਿਲਾ_Const_ਸੂਚਕਾਂਕ) {
                        location = "c";
                    } ਨਹੀਂਤਾਂ ਜੇ (ਥਾਂ >= o->function->ਅੰਕ_Ret_And_Arg_Slots) {
                        location = "L";
                    } ਨਹੀਂਤਾਂ {
                        location = "a";
                    }
                }
            }

            ਅੱਖਰ ਵਸਤੂ[100];

            ਜੇ (ਥਾਂ >= 0) {
                snprintf(ਵਸਤੂ, ਆਕਾਰ(ਵਸਤੂ), "%s%s%d", type, location, ਥਾਂ);
            } ਨਹੀਂਤਾਂ {
                snprintf(ਵਸਤੂ, ਆਕਾਰ(ਵਸਤੂ), "%s%s", type, location);
            }

            ਜੇ (p == 1) {
                size_t s = ਲੜੀ_ਲੰਬਾਈ(ਵਸਤੂ);

                snprintf(ਵਸਤੂ, ਆਕਾਰ(ਵਸਤੂ), "%d", i);

                ਜਦੋਂ (ਲੜੀ_ਲੰਬਾਈ(ਵਸਤੂ) < s) {
                    strcat(ਵਸਤੂ, " ");
                }
            }

            ਲਿਖੋ("|%s ", ਵਸਤੂ);
        }
        ਲਿਖੋ("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 max_ਥਾਂ = Get_Max_ਵਰਤਿਆ_ਥਾਂ_Plus_One(o);

    ਜੇ (max_ਥਾਂ > o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ) {
        d_m3Log(ਢੇਰ, "                      -");

        ਲਈ (u16 i = o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ; i < max_ਥਾਂ; ++i) {
            ਲਿਖੋ("----");
        }

        ਲਿਖੋ("\n");

        d_m3Log(ਢੇਰ, "                 slot |");
        ਲਈ (u16 i = o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ; i < max_ਥਾਂ; ++i) {
            ਲਿਖੋ("%3d|", i);
        }

        ਲਿਖੋ("\n");
        d_m3Log(ਢੇਰ, "                alloc |");

        ਲਈ (u16 i = o->ਥਾਂ_ਪਹਿਲਾ_Dynamic_ਸੂਚਕਾਂਕ; i < max_ਥਾਂ; ++i) {
            ਲਿਖੋ("%3d|", o->m3Slots[i]);
        }

        ਲਿਖੋ("\n");
    }
    d_m3Log(ਢੇਰ, "\n");
}


ਸਥਿਰ
ਅਚਲ ਅੱਖਰ* GetOpcodeIndentionString (i32 ਖੰਡ_ਡੂੰਘਾਈ)
{
    ਖੰਡ_ਡੂੰਘਾਈ += 1;

    ਜੇ (ਖੰਡ_ਡੂੰਘਾਈ < 0) {
        ਖੰਡ_ਡੂੰਘਾਈ = 0;
    }

    ਸਥਿਰ ਅਚਲ ਅੱਖਰ* s_spaces = ".......................................................................................";

    ਅਚਲ ਅੱਖਰ* indent = s_spaces + ਲੜੀ_ਲੰਬਾਈ(s_spaces);
    indent -= (ਖੰਡ_ਡੂੰਘਾਈ * 2);
    ਜੇ (indent < s_spaces) {
        indent = s_spaces;
    }

    ਵਾਪਸ indent;
}


ਅਚਲ ਅੱਖਰ* get_indention_string (IM3Compilation o)
{
    ਵਾਪਸ GetOpcodeIndentionString(o->ਖੰਡ.ਡੂੰਘਾਈ + 4);
}


ਖਾਲੀ log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 ਡੂੰਘਾਈ = o->ਖੰਡ.ਡੂੰਘਾਈ;
    ਜੇ (i_opcode == c_wa_Op_ਸਿਰਾ or i_opcode == c_waOp_else) {
        ਡੂੰਘਾਈ--;
    }
    (ਖਾਲੀ)ਡੂੰਘਾਈ;

    m3log(compile, "%4d | 0x%02x  %s %s", o->ਅੰਕ_Opcodes++, i_opcode, GetOpcodeIndentionString(ਡੂੰਘਾਈ), GetOpInfo(i_opcode)->name);
}


ਖਾਲੀ log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = ਲੱਭੋ_operation_info(i_operation);

    d_m3Log(emit, "");
    ਜੇ (i.info) {
        ਲਿਖੋ("%p: %s\n", Get_ਪੰਨਾ_PC(o->ਪੰਨਾ), i.info->name);
    } ਨਹੀਂਤਾਂ {
        ਲਿਖੋ("not found: %p\n", i_operation);
    }
}

#ਜੇ_ਅੰਤ // DEBUG


#ਜੇ d_m3EnableOpProfiling

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_Profiler_ਥਾਂ {
    cstr_t opName;
    u64    ਪ੍ਰਾਪਤੀ_ਗਿਣਤੀ;
} M3_Profiler_ਥਾਂ;

ਸਥਿਰ M3_Profiler_ਥਾਂ s_opProfilerCounts[d_m3_Profiler_ਥਾਂ_ਨਕਾਬ + 1] = {};

ਖਾਲੀ Profile_ਪ੍ਰਾਪਤੀ (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3_Profiler_ਥਾਂ* ਥਾਂ = &s_opProfilerCounts[ptr & d_m3_Profiler_ਥਾਂ_ਨਕਾਬ];

    ਜੇ (ਥਾਂ->opName) {
        ਜੇ (ਥਾਂ->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    ਥਾਂ->opName = i_operationName;
    ਥਾਂ->ਪ੍ਰਾਪਤੀ_ਗਿਣਤੀ++;
}


ਖਾਲੀ m3_PrintProfilerInfo ()
{
    M3_Profiler_ਥਾਂ  dummy;
    M3_Profiler_ਥਾਂ* max_ਥਾਂ = &dummy;

    ਕਰੋ {
        max_ਥਾਂ->ਪ੍ਰਾਪਤੀ_ਗਿਣਤੀ = 0;

        ਲਈ (u32 i = 0; i <= d_m3_Profiler_ਥਾਂ_ਨਕਾਬ; ++i) {
            M3_Profiler_ਥਾਂ* ਥਾਂ = &s_opProfilerCounts[i];

            ਜੇ (ਥਾਂ->opName) {
                ਜੇ (ਥਾਂ->ਪ੍ਰਾਪਤੀ_ਗਿਣਤੀ > max_ਥਾਂ->ਪ੍ਰਾਪਤੀ_ਗਿਣਤੀ) {
                    max_ਥਾਂ = ਥਾਂ;
                }
            }
        }

        ਜੇ (max_ਥਾਂ->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", max_ਥਾਂ->ਪ੍ਰਾਪਤੀ_ਗਿਣਤੀ, max_ਥਾਂ->opName);
            max_ਥਾਂ->opName = ਨਿਰਾ;
        }
    } ਜਦੋਂ (max_ਥਾਂ->ਪ੍ਰਾਪਤੀ_ਗਿਣਤੀ);
}

#ਨਹੀਂਤਾਂ

ਖਾਲੀ m3_PrintProfilerInfo () {}

#ਜੇ_ਅੰਤ
