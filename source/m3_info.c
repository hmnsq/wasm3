//
//  m3_info.c
//
//  Created by Steven Massey on 4/27/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ಅನ್ತರ್ಭಾವ "m3_env.h"
#ಅನ್ತರ್ಭಾವ "m3_info.h"
#ಅನ್ತರ್ಭಾವ "m3_compile.h"

#ಆದರೆ defined(DEBUG) || (d_m3EnableStrace >= 1)

size_t SPrintArg (ಅಕ್ಷರ* o_string, size_t i_string_ಬಫರ್_ಗಾತ್ರ, voidptr_t i_sp, m3type_t i_type)
{
    ಪೂರ್ಣಾಂಕ len = 0;

    *o_string = 0;

    ಆದರೆ (i_type == c_m3Type_i32) {
        len = snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "%" PRIi32, *(i32*)i_sp);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_i64) {
        len = snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "%" PRIi64, *(i64*)i_sp);
    }
#  ಆದರೆ d_m3HasFloat
    ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_f32) {
        len = snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "%" PRIf32, *(f32*)i_sp);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (i_type == c_m3Type_f64) {
        len = snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "%" PRIf64, *(f64*)i_sp);
    }
#  endif

    len = M3_MAX(0, len);

    ಹಿಂತಿರುಗಿ len;
}


cstr_t SPrintFunctionArgList (IM3Function i_function, m3stack_t i_sp)
{
    ಪೂರ್ಣಾಂಕ ret;

    ಸ್ಥಿರ ಅಕ್ಷರ string[256];

    ಅಕ್ಷರ*   s = string;
    ccstr_t e = string + ಗಾತ್ರ(string) - 1;

    ret = snprintf(s, e - s, "(");
    s += M3_MAX(0, ret);

    u64* argSp = (u64*)i_sp;

    IM3FuncType funcType = i_function->funcType;
    ಆದರೆ (funcType) {
        u32 ಅಂಕೆ_ವಾದಗಳು = funcType->ಅಂಕೆ_ವಾದಗಳು;

        ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_ವಾದಗಳು; ++i) {
            m3type_t type = d_FuncArgType(funcType, i);

            ret = snprintf(s, e - s, "%s: ", c_waTypes[ತಳ_Type_ದ(type)]);
            s += M3_MAX(0, ret);

            s += SPrintArg(s, e - s, argSp + i, type);

            ಆದರೆ (i != ಅಂಕೆ_ವಾದಗಳು - 1) {
                ret = snprintf(s, e - s, ", ");
                s += M3_MAX(0, ret);
            }
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಬರೆ("null signature");
    }

    ret = snprintf(s, e - s, ")");
    s += M3_MAX(0, ret);

    ಹಿಂತಿರುಗಿ string;
}

// The results of a call, comma separated, for the strace return line. Empty when
// the function returns nothing, so the caller can leave the " = " off entirely.
// Every result occupies a full u64 in the I/O area, whatever its type - the same
// layout m3_GetResults reads.
cstr_t SPrintFunctionRetList (IM3FuncType i_funcType, m3stack_t i_sp)
{
    ಪೂರ್ಣಾಂಕ ret;

    ಸ್ಥಿರ ಅಕ್ಷರ string[256];

    ಅಕ್ಷರ*   s = string;
    ccstr_t e = string + ಗಾತ್ರ(string) - 1;

    *s = 0;

    u64* retSp = (u64*)i_sp;

    u32 ಅಂಕೆ_Rets = i_funcType ? i_funcType->ಅಂಕೆ_Rets : 0;

    ಗಾಗಿ (u32 i = 0; i < ಅಂಕೆ_Rets; ++i) {
        s += SPrintArg(s, e - s, retSp + i, d_FuncRetType(i_funcType, i));

        ಆದರೆ (i != ಅಂಕೆ_Rets - 1) {
            ret = snprintf(s, e - s, ", ");
            s += M3_MAX(0, ret);
        }
    }

    ಹಿಂತಿರುಗಿ string;
}

#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ DEBUG

// a central function you can be breakpoint:
ಶೂನ್ಯ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_ಸಂದೇಶ)
{
    ಬರೆ("\nexception: '%s' @ %s\n", i_exception, i_ಸಂದೇಶ);
    ಹಿಂತಿರುಗಿ;
}


ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ OpInfo {
    IM3OpInfo  info;
    m3opcode_t opcode;
} OpInfo;

ಶೂನ್ಯ m3_PrintM3Info ()
{
    ಬರೆ("\n-- m3 configuration --------------------------------------------\n");
    //  printf (" sizeof M3CodePage    : %zu bytes  (%d slots) \n", sizeof (M3CodePage), c_m3CodePageNumSlots);
    ಬರೆ(" sizeof M3MemPage     : %u bytes              \n", d_m3_Default_Mem_ಪುಟ_ಗಾತ್ರ);
    ಬರೆ(" sizeof M3Compilation : %zu bytes             \n", ಗಾತ್ರ(M3Compilation));
    ಬರೆ(" sizeof M3Function    : %zu bytes             \n", ಗಾತ್ರ(M3Function));
    ಬರೆ("----------------------------------------------------------------\n\n");
}


ಶೂನ್ಯ* v_PrintEnvModuleInfo (IM3Module i_module, ಶೂನ್ಯ* i_info)
{
    u32* io_ಸೂಚ್ಯಂಕ = (u32*)i_info;
    ಬರೆ(" module [%u]  name: '%s'; funcs: %d  \n", (*io_ಸೂಚ್ಯಂಕ)++, i_module->name, i_module->ಅಂಕೆ_Functions);

    ಹಿಂತಿರುಗಿ ಖಾಲಿ;
}


ಶೂನ್ಯ m3_PrintRuntimeInfo (IM3Runtime i_runtime)
{
    ಬರೆ("\n-- m3 runtime -------------------------------------------------\n");

    ಬರೆ(" stack-size: %zu   \n\n", i_runtime->ಅಂಕೆ_ರಾಶಿ_Slots * ಗಾತ್ರ(m3slot_t));

    u32 module_ಸೂಚ್ಯಂಕ = 0;
    ForEachModule(i_runtime, v_PrintEnvModuleInfo, &module_ಸೂಚ್ಯಂಕ);

    ಬರೆ("----------------------------------------------------------------\n\n");
}


cstr_t GetTypeName (m3type_t i_m3Type)
{
    u8 ತಳ = ತಳ_Type_ದ(i_m3Type);

    ಆದರೆ (ತಳ <= c_m3Type_unknown) {
        ಹಿಂತಿರುಗಿ c_waTypes[ತಳ];
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ "?";
    }
}


// Appends to a string that is built up over several calls, and returns the length
// reached. snprintf reports the length it wanted to write rather than the length
// it wrote, so an append that truncates has to leave the cursor inside the buffer.
ಸ್ಥಿರ
size_t SPrintAppend (ಅಕ್ಷರ* o_string, size_t i_string_ಬಫರ್_ಗಾತ್ರ, size_t i_len, ccstr_t i_format, ...)
{
    ಆದರೆ (i_len + 1 >= i_string_ಬಫರ್_ಗಾತ್ರ) {
        ಹಿಂತಿರುಗಿ i_len;
    }

    va_list ವಾದಗಳು;
    va_start(ವಾದಗಳು, i_format);
    ಪೂರ್ಣಾಂಕ ret = vsnprintf(o_string + i_len, i_string_ಬಫರ್_ಗಾತ್ರ - i_len, i_format, ವಾದಗಳು);
    va_end(ವಾದಗಳು);

    ಹಿಂತಿರುಗಿ M3_MIN(i_len + (size_t)M3_MAX(0, ret), i_string_ಬಫರ್_ಗಾತ್ರ - 1);
}


// TODO: these 'static char string []' aren't thread-friendly.  though these functions are
// mainly for simple diagnostics during development, it'd be nice if they were fully reliable.

cstr_t SPrintFuncTypeSignature (IM3FuncType i_funcType)
{
    ಸ್ಥಿರ ಅಕ್ಷರ string[256];

    size_t len = SPrintAppend(string, ಗಾತ್ರ(string), 0, "(");

    ಗಾಗಿ (u32 i = 0; i < i_funcType->ಅಂಕೆ_ವಾದಗಳು; ++i) {
        len = SPrintAppend(string, ಗಾತ್ರ(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncArgType(i_funcType, i)));
    }

    len = SPrintAppend(string, ಗಾತ್ರ(string), len, ") -> ");

    ಗಾಗಿ (u32 i = 0; i < i_funcType->ಅಂಕೆ_Rets; ++i) {
        len = SPrintAppend(string, ಗಾತ್ರ(string), len, "%s%s", i ? ", " : "",
                           GetTypeName(d_FuncRetType(i_funcType, i)));
    }

    ಹಿಂತಿರುಗಿ string;
}


cstr_t SPrint_ಮೌಲ್ಯ (ಶೂನ್ಯ* i_ಮೌಲ್ಯ, m3type_t i_type)
{
    ಸ್ಥಿರ ಅಕ್ಷರ string[100];
    SPrintArg(string, 100, (m3stack_t)i_ಮೌಲ್ಯ, i_type);
    ಹಿಂತಿರುಗಿ string;
}

// Finds i_operation in one operation table. i_opcodeBase tags the result, so an
// extended entry comes back as the 0xFCxx opcode the rest of the code speaks.
ಸ್ಥಿರ
bool ಪರಿಶೀಲನೆ_Operation_ಕೋಷ್ಟಕ (OpInfo* io_info, IM3OpInfo i_ಕೋಷ್ಟಕ, u32 i_ಅಂಕೆ_Ops, m3opcode_t i_opcode_ತಳ, IM3Operation i_operation)
{
    ಗಾಗಿ (u32 i = 0; i < i_ಅಂಕೆ_Ops; ++i) {
        IM3OpInfo oi = &i_ಕೋಷ್ಟಕ[i];

        ಗಾಗಿ (u32 o = 0; o < 4; ++o) {
            ಆದರೆ (oi->operations[o] == i_operation) {
                io_info->info   = oi;
                io_info->opcode = i_opcode_ತಳ | i;
                ಹಿಂತಿರುಗಿ true;
            }
        }
    }

    ಹಿಂತಿರುಗಿ false;
}

ಸ್ಥಿರ
OpInfo ಕಂಡುಹಿಡಿ_operation_info (IM3Operation i_operation)
{
    OpInfo opInfo = { ಖಾಲಿ, 0 };

    ಆದರೆ (!i_operation) {
        ಹಿಂತಿರುಗಿ opInfo;
    }

    // Scans the tables directly rather than through GetOpInfo (), which rejects
    // the reserved slots and the internal operations kept past the last opcode.
    // Reserved slots are zeroed, so they never match a real operation here.
    // c_operationsHigh is not scanned: every entry in it is compiler-driven with
    // an empty operation list, so nothing there could match.
    ಆದರೆ (not ಪರಿಶೀಲನೆ_Operation_ಕೋಷ್ಟಕ(&opInfo, c_operations, c_ಅಂಕೆ_Operations, 0, i_operation)) {
        ಆದರೆ (not ಪರಿಶೀಲನೆ_Operation_ಕೋಷ್ಟಕ(&opInfo, c_operationsFC, c_ಅಂಕೆ_Operations_FC, c_waOp_extended << 8, i_operation)) {
#  ಆದರೆ d_m3HasAtomics
            // one entry serves a whole kind, so the opcode it comes back with is only the entry's index
            ಪರಿಶೀಲನೆ_Operation_ಕೋಷ್ಟಕ(&opInfo, c_operationsFE, c_ಅಂಕೆ_Operations_FE, c_waOp_atomic << 8, i_operation);
#  endif
        }
    }

    ಹಿಂತಿರುಗಿ opInfo;
}


#  undef fetch
#  define fetch(TYPE) (* (TYPE *) ((*o_pc)++))

#  define d_m3Decoder(FUNC) ಶೂನ್ಯ Decode_##FUNC (ಅಕ್ಷರ * o_string, size_t i_string_ಬಫರ್_ಗಾತ್ರ, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t * o_pc)

d_m3Decoder(Call)
{
    ಶೂನ್ಯ* function    = fetch(ಶೂನ್ಯ*);
    i32   ರಾಶಿ_ಸ್ಥಾನಾಂತರ = fetch(i32);

    snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "%p; stack-offset: %d", function, ರಾಶಿ_ಸ್ಥಾನಾಂತರ);
}


d_m3Decoder(ನಮೂದು_2)
{
    IM3Function function = fetch(IM3Function);

    // only prints out the first registered name for the function
    snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "%s", m3_GetFunctionName(function));
}


d_m3Decoder(f64_Store)
{
    ಆದರೆ (i_operation == i_opInfo->operations[0]) {
        u32 ಕಾರ್ಯಾಂಶ = fetch(u32);
        u32 offset  = fetch(u32);

        snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "offset= slot:%d + immediate:%d", ಕಾರ್ಯಾಂಶ, offset);
    }
}


d_m3Decoder(Branch)
{
    ಶೂನ್ಯ* ಗುರಿ = fetch(ಶೂನ್ಯ*);
    snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, "%p", ಗುರಿ);
}

d_m3Decoder(Branch_ಕೋಷ್ಟಕ)
{
    u32 ಸ್ಥಾನ = fetch(u32);

    size_t len = SPrintAppend(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, 0, "slot: %" PRIu32 "; targets: ", ಸ್ಥಾನ);

    //    IM3Function function = fetch2 (IM3Function);

    i32 targets = fetch(i32);

    ಗಾಗಿ (i32 i = 0; i < targets; ++i) {
        pc_t addr = fetch(pc_t);
        len       = SPrintAppend(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, len, "%" PRIi32 "=%p, ", i, addr);
    }

    pc_t addr = fetch(pc_t);
    SPrintAppend(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, len, "def=%p ", addr);
}


d_m3Decoder(Const)
{
    u64 ಮೌಲ್ಯ  = fetch(u64);
    i32 offset = fetch(i32);
    snprintf(o_string, i_string_ಬಫರ್_ಗಾತ್ರ, " slot [%d] = %" PRIu64, offset, ಮೌಲ್ಯ);
}


#  undef fetch

ಶೂನ್ಯ DecodeOperation (ಅಕ್ಷರ* o_string, size_t i_string_ಬಫರ್_ಗಾತ್ರ, m3opcode_t i_opcode, IM3Operation i_operation, IM3OpInfo i_opInfo, pc_t* o_pc)
{
#  define d_m3Decode(OPCODE, FUNC) ಸ್ಥಿತಿ OPCODE: Decode_##FUNC (o_string, i_string_ಬಫರ್_ಗಾತ್ರ, i_opcode, i_operation, i_opInfo, o_pc); ನಿಲ್ಲಿಸು;

    ಆಯ್ಕೆ (i_opcode) {
        //d_m3Decode (0xc0,                  Const)
        d_m3Decode(0xc5, ನಮೂದು_2)
        d_m3Decode(c_waOp_call, Call)
        d_m3Decode(c_waOp_branch, Branch)
        d_m3Decode(c_wa_Op_branch_ಕೋಷ್ಟಕ, Branch_ಕೋಷ್ಟಕ)
        d_m3Decode(0x39, f64_Store)
    }
}

// TODO: this isn't fully implemented. it blindly assumes each word is a Operation pointer
// and, if an operation happens to missing from the c_operations table it won't be recognized here
ಶೂನ್ಯ dump_ಸಂಕೇತ_ಪುಟ (IM3_ಸಂಕೇತ_ಪುಟ i_ಸಂಕೇತ_ಪುಟ, pc_t i_ಆರಂಭ_PC)
{
    m3log(ಸಂಕೇತ_2, "code page seq: %d", i_ಸಂಕೇತ_ಪುಟ->info.sequence);

    pc_t pc  = i_ಆರಂಭ_PC ? i_ಆರಂಭ_PC : Get_ಪುಟ_ಆರಂಭ_PC(i_ಸಂಕೇತ_ಪುಟ);
    pc_t ಕೊನೆ = Get_ಪುಟ_PC(i_ಸಂಕೇತ_ಪುಟ);

    m3log(ಸಂಕೇತ_2, "---------------------------------------------------------------------------------------");

    ಆಗ (pc < ಕೊನೆ) {
        ಸ್ಥಿರಾಂಕ pc_t operationPC = pc;
        (ಶೂನ್ಯ)operationPC;  // avoid unused variable warning if DEBUG is not defined
        IM3Operation op = (IM3Operation)(*pc++);

        OpInfo i = ಕಂಡುಹಿಡಿ_operation_info(op);

        ಆದರೆ (i.info) {
            ಅಕ್ಷರ infoString[8 * 1024] = { 0 };

            DecodeOperation(infoString, ಗಾತ್ರ(infoString), i.opcode, op, i.info, &pc);

            m3log(ಸಂಕೇತ_2, "%p | %20s  %s", operationPC, i.info->name, infoString);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m3log(ಸಂಕೇತ_2, "%p | %p", operationPC, op);
        }
    }

    m3log(ಸಂಕೇತ_2, "---------------------------------------------------------------------------------------");

    m3log(ಸಂಕೇತ_2, "free-lines: %d", i_ಸಂಕೇತ_ಪುಟ->info.ಅಂಕೆ_ಸಾಲುಗಳು - i_ಸಂಕೇತ_ಪುಟ->info.ಸಾಲು_ಸೂಚ್ಯಂಕ);
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
    i32 regAllocated[2] = { (i32)Is_ರಿಜಿಸ್ಟರ್_Allocated(o, 0), (i32)Is_ರಿಜಿಸ್ಟರ್_Allocated(o, 1) };

    // display whether r0 or fp0 is allocated. these should then also be reflected somewhere in the stack too.
    d_m3Log(ರಾಶಿ, "\n");
    d_m3Log(ರಾಶಿ, "        ");
    ಬರೆ("%s %s    ", regAllocated[0] ? "(r0)" : "    ", regAllocated[1] ? "(fp0)" : "     ");
    ಬರೆ("\n");

    ಗಾಗಿ (u32 p = 1; p <= 2; ++p) {
        d_m3Log(ರಾಶಿ, "        ");

        ಗಾಗಿ (u16 i = 0; i < o->ರಾಶಿ_ಸೂಚ್ಯಂಕ; ++i) {
            ಆದರೆ (i > 0 and i == o->ರಾಶಿ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
                ಬರೆ("#");
            }

            ಆದರೆ (i == o->ಖಂಡ.ಖಂಡ_ರಾಶಿ_ಸೂಚ್ಯಂಕ) {
                ಬರೆ(">");
            }

            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* type = c_waCompactTypes[ತಳ_Type_ದ(o->type_ರಾಶಿ[i])];

            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* location = "";

            i32 ಸ್ಥಾನ = o->wasm_ರಾಶಿ[i];

            ಆದರೆ (Is_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(ಸ್ಥಾನ)) {
                bool isFp = Is_Fp_ರಿಜಿಸ್ಟರ್_ಸ್ಥಾನ_Alias(ಸ್ಥಾನ);
                location  = isFp ? "/f" : "/r";

                regAllocated[isFp]--;
                ಸ್ಥಾನ = -1;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (ಸ್ಥಾನ < o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
                    ಆದರೆ (ಸ್ಥಾನ >= o->ಸ್ಥಾನ_ಮೊದಲ_Const_ಸೂಚ್ಯಂಕ) {
                        location = "c";
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಾನ >= o->function->ಅಂಕೆ_Ret_And_Arg_Slots) {
                        location = "L";
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        location = "a";
                    }
                }
            }

            ಅಕ್ಷರ ಅಂಶ[100];

            ಆದರೆ (ಸ್ಥಾನ >= 0) {
                snprintf(ಅಂಶ, ಗಾತ್ರ(ಅಂಶ), "%s%s%d", type, location, ಸ್ಥಾನ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                snprintf(ಅಂಶ, ಗಾತ್ರ(ಅಂಶ), "%s%s", type, location);
            }

            ಆದರೆ (p == 1) {
                size_t s = ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(ಅಂಶ);

                snprintf(ಅಂಶ, ಗಾತ್ರ(ಅಂಶ), "%d", i);

                ಆಗ (ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(ಅಂಶ) < s) {
                    strcat(ಅಂಶ, " ");
                }
            }

            ಬರೆ("|%s ", ಅಂಶ);
        }
        ಬರೆ("\n");
    }

    //    for (u32 r = 0; r < 2; ++r)
    //    d_m3Assert (regAllocated [r] == 0);         // reg allocation & stack out of sync

    u16 max_ಸ್ಥಾನ = Get_Max_ಬಳಸಿದ_ಸ್ಥಾನ_Plus_One(o);

    ಆದರೆ (max_ಸ್ಥಾನ > o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ) {
        d_m3Log(ರಾಶಿ, "                      -");

        ಗಾಗಿ (u16 i = o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ; i < max_ಸ್ಥಾನ; ++i) {
            ಬರೆ("----");
        }

        ಬರೆ("\n");

        d_m3Log(ರಾಶಿ, "                 slot |");
        ಗಾಗಿ (u16 i = o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ; i < max_ಸ್ಥಾನ; ++i) {
            ಬರೆ("%3d|", i);
        }

        ಬರೆ("\n");
        d_m3Log(ರಾಶಿ, "                alloc |");

        ಗಾಗಿ (u16 i = o->ಸ್ಥಾನ_ಮೊದಲ_Dynamic_ಸೂಚ್ಯಂಕ; i < max_ಸ್ಥಾನ; ++i) {
            ಬರೆ("%3d|", o->m3Slots[i]);
        }

        ಬರೆ("\n");
    }
    d_m3Log(ರಾಶಿ, "\n");
}


ಸ್ಥಿರ
ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* GetOpcodeIndentionString (i32 ಖಂಡ_ಆಳ)
{
    ಖಂಡ_ಆಳ += 1;

    ಆದರೆ (ಖಂಡ_ಆಳ < 0) {
        ಖಂಡ_ಆಳ = 0;
    }

    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* s_spaces = ".......................................................................................";

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* indent = s_spaces + ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(s_spaces);
    indent -= (ಖಂಡ_ಆಳ * 2);
    ಆದರೆ (indent < s_spaces) {
        indent = s_spaces;
    }

    ಹಿಂತಿರುಗಿ indent;
}


ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* get_indention_string (IM3Compilation o)
{
    ಹಿಂತಿರುಗಿ GetOpcodeIndentionString(o->ಖಂಡ.ಆಳ + 4);
}


ಶೂನ್ಯ log_opcode (IM3Compilation o, m3opcode_t i_opcode)
{
    i32 ಆಳ = o->ಖಂಡ.ಆಳ;
    ಆದರೆ (i_opcode == c_wa_Op_ಕೊನೆ or i_opcode == c_waOp_else) {
        ಆಳ--;
    }
    (ಶೂನ್ಯ)ಆಳ;

    m3log(compile, "%4d | 0x%02x  %s %s", o->ಅಂಕೆ_Opcodes++, i_opcode, GetOpcodeIndentionString(ಆಳ), GetOpInfo(i_opcode)->name);
}


ಶೂನ್ಯ log_emit (IM3Compilation o, IM3Operation i_operation)
{
    OpInfo i = ಕಂಡುಹಿಡಿ_operation_info(i_operation);

    d_m3Log(emit, "");
    ಆದರೆ (i.info) {
        ಬರೆ("%p: %s\n", Get_ಪುಟ_PC(o->ಪುಟ), i.info->name);
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಬರೆ("not found: %p\n", i_operation);
    }
}

#ಆದರೆ_ಅಂತ್ಯ // DEBUG


#ಆದರೆ d_m3EnableOpProfiling

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ M3_Profiler_ಸ್ಥಾನ {
    cstr_t opName;
    u64    ದೊರಕಿತು_ಎಣಿಕೆ;
} M3_Profiler_ಸ್ಥಾನ;

ಸ್ಥಿರ M3_Profiler_ಸ್ಥಾನ s_opProfilerCounts[d_m3_Profiler_ಸ್ಥಾನ_ಮುಖವಾಡ + 1] = {};

ಶೂನ್ಯ Profile_ದೊರಕಿತು (cstr_t i_operationName)
{
    u64 ptr = (u64)i_operationName;

    M3_Profiler_ಸ್ಥಾನ* ಸ್ಥಾನ = &s_opProfilerCounts[ptr & d_m3_Profiler_ಸ್ಥಾನ_ಮುಖವಾಡ];

    ಆದರೆ (ಸ್ಥಾನ->opName) {
        ಆದರೆ (ಸ್ಥಾನ->opName != i_operationName) {
            m3_Abort("profiler slot collision; increase d_m3ProfilerSlotMask");
        }
    }

    ಸ್ಥಾನ->opName = i_operationName;
    ಸ್ಥಾನ->ದೊರಕಿತು_ಎಣಿಕೆ++;
}


ಶೂನ್ಯ m3_PrintProfilerInfo ()
{
    M3_Profiler_ಸ್ಥಾನ  dummy;
    M3_Profiler_ಸ್ಥಾನ* max_ಸ್ಥಾನ = &dummy;

    ಮಾಡು {
        max_ಸ್ಥಾನ->ದೊರಕಿತು_ಎಣಿಕೆ = 0;

        ಗಾಗಿ (u32 i = 0; i <= d_m3_Profiler_ಸ್ಥಾನ_ಮುಖವಾಡ; ++i) {
            M3_Profiler_ಸ್ಥಾನ* ಸ್ಥಾನ = &s_opProfilerCounts[i];

            ಆದರೆ (ಸ್ಥಾನ->opName) {
                ಆದರೆ (ಸ್ಥಾನ->ದೊರಕಿತು_ಎಣಿಕೆ > max_ಸ್ಥಾನ->ದೊರಕಿತು_ಎಣಿಕೆ) {
                    max_ಸ್ಥಾನ = ಸ್ಥಾನ;
                }
            }
        }

        ಆದರೆ (max_ಸ್ಥಾನ->opName) {
            fprintf(stderr, "%13" PRIu64 "  %s\n", max_ಸ್ಥಾನ->ದೊರಕಿತು_ಎಣಿಕೆ, max_ಸ್ಥಾನ->opName);
            max_ಸ್ಥಾನ->opName = ಖಾಲಿ;
        }
    } ಆಗ (max_ಸ್ಥಾನ->ದೊರಕಿತು_ಎಣಿಕೆ);
}

#ಇಲ್ಲದಿದ್ದರೆ

ಶೂನ್ಯ m3_PrintProfilerInfo () {}

#ಆದರೆ_ಅಂತ್ಯ
