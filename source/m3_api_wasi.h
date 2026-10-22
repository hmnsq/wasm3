//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_api_wasi_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_api_wasi_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_core.h"

// What every WASI backend hands the guest as its environment. A Wasm module gets
// no host environment of its own, and a few programs want these set; WASM3 and
// WASM3_ARCH are how a guest tells it is running under Wasm3 (see print_version).
#ਜੇ !defined(d_m3WasiDefaultEnvironment)
#  define d_m3WasiDefaultEnvironment          \
      "TERM=xterm-256color",                  \
      "COLORTERM=truecolor",                  \
      "LANG=en_US.UTF-8",                     \
      "PWD=/",                                \
      "HOME=/",                               \
      "PATH=/",                               \
      "WASM3=1",                              \
      "WASM3_ARCH=" M3_ARCH
#ਜੇ_ਅੰਤ

#ਜੇ defined(d_m3HasUVWASI)
#  include "uvwasi.h"
#ਜੇ_ਅੰਤ

d_m3BeginExternC

ਕਿਸਮਪਰਿਭਾਸ਼ਾ ਬਣਤਰ m3_wasi_context_t {
    i32      exit_ਕੋਡ;
    u32      argc;
    ccstr_t* argv;

#ਜੇ defined(d_m3HasUVWASI)
    uvwasi_t uvwasi;
#ਜੇ_ਅੰਤ
} m3_wasi_context_t;

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ M3WASI m3_wasi_context_t*

// ----------------------------------------------------------------------
// Free-standing WASI
// ----------------------------------------------------------------------

M3_ਨਤੀਜਾ m3_NewCustomWASI (M3WASI*);
ਖਾਲੀ     m3_FreeCustomWASI(M3WASI);

M3_ਨਤੀਜਾ m3_LinkCustomWASI(IM3Module, M3WASI);

#ਜੇ defined(d_m3HasUVWASI)
M3_ਨਤੀਜਾ m3_LinkCustomWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#ਜੇ_ਅੰਤ


// ----------------------------------------------------------------------
// Global WASI
// ----------------------------------------------------------------------
M3_ਨਤੀਜਾ m3_LinkWASI (IM3Module io_module);

#ਜੇ defined(d_m3HasUVWASI)
M3_ਨਤੀਜਾ m3_LinkWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#ਜੇ_ਅੰਤ

m3_wasi_context_t* m3_GetWasiContext (ਖਾਲੀ);


d_m3_ਸਿਰਾ_Extern_C

#ਜੇ_ਅੰਤ // m3_api_wasi_h
