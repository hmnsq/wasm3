//
//  m3_code.h
//
//  Created by Steven Massey on 4/19/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//

#جيڪڏهن_وصف_نه_ٿيل m3_ڪوڊ_h
#وصف_ڪريو m3_ڪوڊ_h

#ضمن "m3_core.h"

d_m3BeginExternC

تعريفنوع بنية M3_ڪوڊ_صفحو {
    M3_ڪوڊ_صفحو_سرو info;
    ڪوڊ_t           ڪوڊ[1];
} M3_ڪوڊ_صفحو;

تعريفنوع M3_ڪوڊ_صفحو* IM3_ڪوڊ_صفحو;


IM3_ڪوڊ_صفحو         New_ڪوڊ_صفحو (IM3Runtime i_runtime, u32 i_min_انگ_سٽون);

فراغ                Free_ڪوڊ_صفحا (IM3_ڪوڊ_صفحو* io_list);

u32                 انگ_Free_سٽون (IM3_ڪوڊ_صفحو i_صفحو);
pc_t                Get_صفحو_شروعات_PC (IM3_ڪوڊ_صفحو i_صفحو);
pc_t                Get_صفحو_PC (IM3_ڪوڊ_صفحو i_صفحو);
فراغ                Emit_لفظ_impl (IM3_ڪوڊ_صفحو i_صفحو, فراغ* i_لفظ);
فراغ                EmitWord32 (IM3_ڪوڊ_صفحو i_صفحو, u32 i_لفظ);
فراغ                EmitWord64 (IM3_ڪوڊ_صفحو i_صفحو, u64 i_لفظ);
#إذا d_m3_رڪارڊ_Backtraces
فراغ Emit_Mapping_داخلا (IM3_ڪوڊ_صفحو i_صفحو, u32 i_module_وٿي);
#جيڪڏهن_پورو // d_m3RecordBacktraces

فراغ        ڌڪو_ڪوڊ_صفحو (IM3_ڪوڊ_صفحو* io_list, IM3_ڪوڊ_صفحو i_ڪوڊ_صفحو);
IM3_ڪوڊ_صفحو ڪڍو_ڪوڊ_صفحو (IM3_ڪوڊ_صفحو* io_list);

IM3_ڪوڊ_صفحو Get_پڇاڙي_ڪوڊ_صفحو (IM3_ڪوڊ_صفحو i_list); // i_list = NULL is valid
u32         ڳڻپ_ڪوڊ_صفحا (IM3_ڪوڊ_صفحو i_list); // i_list = NULL is valid

#إذا d_m3_رڪارڊ_Backtraces
bool شامل_آهي_PC (IM3_ڪوڊ_صفحو i_صفحو, pc_t i_pc);
bool Map_PCTo_وٿي (IM3_ڪوڊ_صفحو i_صفحو, pc_t i_pc, u32* o_module_وٿي);
#جيڪڏهن_پورو // d_m3RecordBacktraces

#جيڪڏهن_وصف_ٿيل DEBUG
فراغ dump_ڪوڊ_صفحو (IM3_ڪوڊ_صفحو i_ڪوڊ_صفحو, pc_t i_شروعات_PC);
#جيڪڏهن_پورو

#وصف_ڪريو Emit_لفظ(صفحو, val) Emit_لفظ_impl(صفحو, (فراغ*)(val))

//---------------------------------------------------------------------------------------------------------------------------------

#إذا d_m3_رڪارڊ_Backtraces

تعريفنوع بنية M3_ڪوڊ_Map_داخلا {
    u32 pc_وٿي;
    u32 module_وٿي;
} M3_ڪوڊ_Map_داخلا;

تعريفنوع بنية M3_ڪوڊ_Mapping_صفحو {
    pc_t           بنياد_PC;
    u32            سائيز;
    u32            صلاحيت;
    M3_ڪوڊ_Map_داخلا entries[];
} M3_ڪوڊ_Mapping_صفحو;

#جيڪڏهن_پورو // d_m3RecordBacktraces

d_m3_پڇاڙي_Extern_C

#جيڪڏهن_پورو // m3_code_h
