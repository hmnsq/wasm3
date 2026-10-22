//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#यदि_अपरिभाषितम् m3_api_wasi_h
#परिभाषय m3_api_wasi_h

#अन्तर्भाव "m3_core.h"

// What every WASI backend hands the guest as its environment. A Wasm module gets
// no host environment of its own, and a few programs want these set; WASM3 and
// WASM3_ARCH are how a guest tells it is running under Wasm3 (see print_version).
#यदि !defined(d_m3WasiDefaultEnvironment)
#  define d_m3WasiDefaultEnvironment          \
      "TERM=xterm-256color",                  \
      "COLORTERM=truecolor",                  \
      "LANG=en_US.UTF-8",                     \
      "PWD=/",                                \
      "HOME=/",                               \
      "PATH=/",                               \
      "WASM3=1",                              \
      "WASM3_ARCH=" M3_ARCH
#यदि_अन्तः

#यदि defined(d_m3HasUVWASI)
#  include "uvwasi.h"
#यदि_अन्तः

d_m3BeginExternC

प्रकारनिर्देश संरचना m3_wasi_context_t {
    i32      exit_संहिता;
    u32      argc;
    ccstr_t* argv;

#यदि defined(d_m3HasUVWASI)
    uvwasi_t uvwasi;
#यदि_अन्तः
} m3_wasi_context_t;

#परिभाषय M3WASI m3_wasi_context_t*

// ----------------------------------------------------------------------
// Free-standing WASI
// ----------------------------------------------------------------------

M3_परिणाम m3_NewCustomWASI (M3WASI*);
शून्य     m3_FreeCustomWASI(M3WASI);

M3_परिणाम m3_LinkCustomWASI(IM3Module, M3WASI);

#यदि defined(d_m3HasUVWASI)
M3_परिणाम m3_LinkCustomWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#यदि_अन्तः


// ----------------------------------------------------------------------
// Global WASI
// ----------------------------------------------------------------------
M3_परिणाम m3_LinkWASI (IM3Module io_module);

#यदि defined(d_m3HasUVWASI)
M3_परिणाम m3_LinkWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#यदि_अन्तः

m3_wasi_context_t* m3_GetWasiContext (शून्य);


d_m3_अन्तः_Extern_C

#यदि_अन्तः // m3_api_wasi_h
