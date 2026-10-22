//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#جيڪڏهن_وصف_نه_ٿيل m3_api_wasi_h
#وصف_ڪريو m3_api_wasi_h

#ضمن "m3_core.h"

// What every WASI backend hands the guest as its environment. A Wasm module gets
// no host environment of its own, and a few programs want these set; WASM3 and
// WASM3_ARCH are how a guest tells it is running under Wasm3 (see print_version).
#إذا !defined(d_m3WasiDefaultEnvironment)
#  define d_m3WasiDefaultEnvironment          \
      "TERM=xterm-256color",                  \
      "COLORTERM=truecolor",                  \
      "LANG=en_US.UTF-8",                     \
      "PWD=/",                                \
      "HOME=/",                               \
      "PATH=/",                               \
      "WASM3=1",                              \
      "WASM3_ARCH=" M3_ARCH
#جيڪڏهن_پورو

#إذا defined(d_m3HasUVWASI)
#  include "uvwasi.h"
#جيڪڏهن_پورو

d_m3BeginExternC

تعريفنوع بنية m3_wasi_context_t {
    i32      exit_ڪوڊ;
    u32      argc;
    ccstr_t* argv;

#إذا defined(d_m3HasUVWASI)
    uvwasi_t uvwasi;
#جيڪڏهن_پورو
} m3_wasi_context_t;

#وصف_ڪريو M3WASI m3_wasi_context_t*

// ----------------------------------------------------------------------
// Free-standing WASI
// ----------------------------------------------------------------------

M3_نتيجو m3_NewCustomWASI (M3WASI*);
فراغ     m3_FreeCustomWASI(M3WASI);

M3_نتيجو m3_LinkCustomWASI(IM3Module, M3WASI);

#إذا defined(d_m3HasUVWASI)
M3_نتيجو m3_LinkCustomWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#جيڪڏهن_پورو


// ----------------------------------------------------------------------
// Global WASI
// ----------------------------------------------------------------------
M3_نتيجو m3_LinkWASI (IM3Module io_module);

#إذا defined(d_m3HasUVWASI)
M3_نتيجو m3_LinkWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#جيڪڏهن_پورو

m3_wasi_context_t* m3_GetWasiContext (فراغ);


d_m3_پڇاڙي_Extern_C

#جيڪڏهن_پورو // m3_api_wasi_h
