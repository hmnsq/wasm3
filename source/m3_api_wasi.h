//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#अगर_अघोषित m3_api_wasi_h
#घोषणा m3_api_wasi_h

#समावेश "m3_core.h"

// What every WASI backend hands the guest as its environment. A Wasm module gets
// no host environment of its own, and a few programs want these set; WASM3 and
// WASM3_ARCH are how a guest tells it is running under Wasm3 (see print_version).
#अगर !defined(d_m3WasiDefaultEnvironment)
#  define d_m3WasiDefaultEnvironment          \
      "TERM=xterm-256color",                  \
      "COLORTERM=truecolor",                  \
      "LANG=en_US.UTF-8",                     \
      "PWD=/",                                \
      "HOME=/",                               \
      "PATH=/",                               \
      "WASM3=1",                              \
      "WASM3_ARCH=" M3_ARCH
#पूर्ण_अगर

#अगर defined(d_m3HasUVWASI)
#  include "uvwasi.h"
#पूर्ण_अगर

d_m3BeginExternC

प्रकार काष्ठा m3_wasi_context_t {
    i32      exit_code;
    u32      argc;
    ccstr_t* argv;

#अगर defined(d_m3HasUVWASI)
    uvwasi_t uvwasi;
#पूर्ण_अगर
} m3_wasi_context_t;

#घोषणा M3WASI m3_wasi_context_t*

// ----------------------------------------------------------------------
// Free-standing WASI
// ----------------------------------------------------------------------

M3_परिणाम m3_NewCustomWASI (M3WASI*);
व्योम     m3_FreeCustomWASI(M3WASI);

M3_परिणाम m3_LinkCustomWASI(IM3Module, M3WASI);

#अगर defined(d_m3HasUVWASI)
M3_परिणाम m3_LinkCustomWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#पूर्ण_अगर


// ----------------------------------------------------------------------
// Global WASI
// ----------------------------------------------------------------------
M3_परिणाम m3_LinkWASI (IM3Module io_module);

#अगर defined(d_m3HasUVWASI)
M3_परिणाम m3_LinkWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#पूर्ण_अगर

m3_wasi_context_t* m3_GetWasiContext (व्योम);


d_m3EndExternC

#पूर्ण_अगर // m3_api_wasi_h
