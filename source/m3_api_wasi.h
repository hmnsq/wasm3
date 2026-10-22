//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#నిర్వచితం_కాకపోతే m3_api_wasi_h
#నిర్వచించు m3_api_wasi_h

#అన్తర్భావ "m3_core.h"

// What every WASI backend hands the guest as its environment. A Wasm module gets
// no host environment of its own, and a few programs want these set; WASM3 and
// WASM3_ARCH are how a guest tells it is running under Wasm3 (see print_version).
#ఒకవేళ !defined(d_m3WasiDefaultEnvironment)
#  define d_m3WasiDefaultEnvironment          \
      "TERM=xterm-256color",                  \
      "COLORTERM=truecolor",                  \
      "LANG=en_US.UTF-8",                     \
      "PWD=/",                                \
      "HOME=/",                               \
      "PATH=/",                               \
      "WASM3=1",                              \
      "WASM3_ARCH=" M3_ARCH
#అయితే_ముగింపు

#ఒకవేళ defined(d_m3HasUVWASI)
#  include "uvwasi.h"
#అయితే_ముగింపు

d_m3BeginExternC

రకనిర్వచనం నిర్మాణం m3_wasi_context_t {
    i32      exit_సంకేతం;
    u32      argc;
    ccstr_t* argv;

#ఒకవేళ defined(d_m3HasUVWASI)
    uvwasi_t uvwasi;
#అయితే_ముగింపు
} m3_wasi_context_t;

#నిర్వచించు M3WASI m3_wasi_context_t*

// ----------------------------------------------------------------------
// Free-standing WASI
// ----------------------------------------------------------------------

M3_ఫలితం m3_NewCustomWASI (M3WASI*);
శూన్య     m3_FreeCustomWASI(M3WASI);

M3_ఫలితం m3_LinkCustomWASI(IM3Module, M3WASI);

#ఒకవేళ defined(d_m3HasUVWASI)
M3_ఫలితం m3_LinkCustomWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#అయితే_ముగింపు


// ----------------------------------------------------------------------
// Global WASI
// ----------------------------------------------------------------------
M3_ఫలితం m3_LinkWASI (IM3Module io_module);

#ఒకవేళ defined(d_m3HasUVWASI)
M3_ఫలితం m3_LinkWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#అయితే_ముగింపు

m3_wasi_context_t* m3_GetWasiContext (శూన్య);


d_m3_చివర_Extern_C

#అయితే_ముగింపు // m3_api_wasi_h
