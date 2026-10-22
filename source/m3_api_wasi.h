//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_api_wasi_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_api_wasi_h

#ಅನ್ತರ್ಭಾವ "m3_core.h"

// What every WASI backend hands the guest as its environment. A Wasm module gets
// no host environment of its own, and a few programs want these set; WASM3 and
// WASM3_ARCH are how a guest tells it is running under Wasm3 (see print_version).
#ಆದರೆ !defined(d_m3WasiDefaultEnvironment)
#  define d_m3WasiDefaultEnvironment          \
      "TERM=xterm-256color",                  \
      "COLORTERM=truecolor",                  \
      "LANG=en_US.UTF-8",                     \
      "PWD=/",                                \
      "HOME=/",                               \
      "PATH=/",                               \
      "WASM3=1",                              \
      "WASM3_ARCH=" M3_ARCH
#ಆದರೆ_ಅಂತ್ಯ

#ಆದರೆ defined(d_m3HasUVWASI)
#  include "uvwasi.h"
#ಆದರೆ_ಅಂತ್ಯ

d_m3BeginExternC

ಪ್ರಕಾರವ್ಯಾಖ್ಯೆ ರಚನೆ m3_wasi_context_t {
    i32      exit_ಸಂಕೇತ;
    u32      argc;
    ccstr_t* argv;

#ಆದರೆ defined(d_m3HasUVWASI)
    uvwasi_t uvwasi;
#ಆದರೆ_ಅಂತ್ಯ
} m3_wasi_context_t;

#ವ್ಯಾಖ್ಯಾನಿಸು M3WASI m3_wasi_context_t*

// ----------------------------------------------------------------------
// Free-standing WASI
// ----------------------------------------------------------------------

M3_ಫಲಿತಾಂಶ m3_NewCustomWASI (M3WASI*);
ಶೂನ್ಯ     m3_FreeCustomWASI(M3WASI);

M3_ಫಲಿತಾಂಶ m3_LinkCustomWASI(IM3Module, M3WASI);

#ಆದರೆ defined(d_m3HasUVWASI)
M3_ಫಲಿತಾಂಶ m3_LinkCustomWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#ಆದರೆ_ಅಂತ್ಯ


// ----------------------------------------------------------------------
// Global WASI
// ----------------------------------------------------------------------
M3_ಫಲಿತಾಂಶ m3_LinkWASI (IM3Module io_module);

#ಆದರೆ defined(d_m3HasUVWASI)
M3_ಫಲಿತಾಂಶ m3_LinkWASIWithOptions (IM3Module io_module, uvwasi_options_t uvwasiOptions);
#ಆದರೆ_ಅಂತ್ಯ

m3_wasi_context_t* m3_GetWasiContext (ಶೂನ್ಯ);


d_m3_ಕೊನೆ_Extern_C

#ಆದರೆ_ಅಂತ್ಯ // m3_api_wasi_h
