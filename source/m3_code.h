//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_ਕੋਡ_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_ਕੋਡ_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

d_m3BeginExternC

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਕੋਡ_ਪੰਨਾ {
    M3_ਕੋਡ_ਪੰਨਾ_ਸਿਰਲੇਖ info;
    ਕੋਡ_t           ਕੋਡ[1];
} M3_ਕੋਡ_ਪੰਨਾ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ M3_ਕੋਡ_ਪੰਨਾ* IM3_ਕੋਡ_ਪੰਨਾ;


IM3_ਕੋਡ_ਪੰਨਾ         New_ਕੋਡ_ਪੰਨਾ (IM3Runtime i_runtime, u32 i_min_ਅੰਕ_ਸਤਰਾਂ);

ਖਾਲੀ                Free_ਕੋਡ_ਪੰਨੇ (IM3_ਕੋਡ_ਪੰਨਾ* io_list);

u32                 ਅੰਕ_Free_ਸਤਰਾਂ (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ);
pc_t                Get_ਪੰਨਾ_ਸ਼ੁਰੂ_PC (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ);
pc_t                Get_ਪੰਨਾ_PC (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ);
ਖਾਲੀ                Emit_ਸ਼ਬਦ_impl (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ, ਖਾਲੀ* i_ਸ਼ਬਦ);
ਖਾਲੀ                EmitWord32 (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ, u32 i_ਸ਼ਬਦ);
ਖਾਲੀ                EmitWord64 (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ, u64 i_ਸ਼ਬਦ);
#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
ਖਾਲੀ Emit_Mapping_ਇੰਦਰਾਜ (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ, u32 i_module_ਵਿੱਥ);
#ਜੇ_ਅੰਤ // d_m3RecordBacktraces

ਖਾਲੀ        ਧੱਕੋ_ਕੋਡ_ਪੰਨਾ (IM3_ਕੋਡ_ਪੰਨਾ* io_list, IM3_ਕੋਡ_ਪੰਨਾ i_ਕੋਡ_ਪੰਨਾ);
IM3_ਕੋਡ_ਪੰਨਾ ਕੱਢੋ_ਕੋਡ_ਪੰਨਾ (IM3_ਕੋਡ_ਪੰਨਾ* io_list);

IM3_ਕੋਡ_ਪੰਨਾ Get_ਸਿਰਾ_ਕੋਡ_ਪੰਨਾ (IM3_ਕੋਡ_ਪੰਨਾ i_list); // i_list = NULL is valid
u32         ਗਿਣਤੀ_ਕੋਡ_ਪੰਨੇ (IM3_ਕੋਡ_ਪੰਨਾ i_list); // i_list = NULL is valid

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces
bool ਸ਼ਾਮਲ_ਹੈ_PC (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ, pc_t i_pc);
bool Map_PCTo_ਵਿੱਥ (IM3_ਕੋਡ_ਪੰਨਾ i_ਪੰਨਾ, pc_t i_pc, u32* o_module_ਵਿੱਥ);
#ਜੇ_ਅੰਤ // d_m3RecordBacktraces

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ DEBUG
ਖਾਲੀ dump_ਕੋਡ_ਪੰਨਾ (IM3_ਕੋਡ_ਪੰਨਾ i_ਕੋਡ_ਪੰਨਾ, pc_t i_ਸ਼ੁਰੂ_PC);
#ਜੇ_ਅੰਤ

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ Emit_ਸ਼ਬਦ(ਪੰਨਾ, val) Emit_ਸ਼ਬਦ_impl(ਪੰਨਾ, (ਖਾਲੀ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#ਜੇ d_m3_ਰਿਕਾਰਡ_Backtraces

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਕੋਡ_Map_ਇੰਦਰਾਜ {
    u32 pc_ਵਿੱਥ;
    u32 module_ਵਿੱਥ;
} M3_ਕੋਡ_Map_ਇੰਦਰਾਜ;

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ M3_ਕੋਡ_Mapping_ਪੰਨਾ {
    pc_t           ਅਧਾਰ_PC;
    u32            ਮਾਪ;
    u32            ਸਮਰੱਥਾ;
    M3_ਕੋਡ_Map_ਇੰਦਰਾਜ entries[];
} M3_ਕੋਡ_Mapping_ਪੰਨਾ;

#ਜੇ_ਅੰਤ // d_m3RecordBacktraces

d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_code_h
