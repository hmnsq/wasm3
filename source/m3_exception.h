//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ m3_exception_h
#ವ್ಯಾಖ್ಯಾನಿಸು m3_exception_h

#ಅನ್ತರ್ಭಾವ "m3_config.h"

#ಆದರೆ d_m3EnableExceptionBreakpoint

// declared in m3_info.c
ಶೂನ್ಯ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_ಸಂದೇಶ);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#ಇಲ್ಲದಿದ್ದರೆ
#  define EXCEPTION_PRINT(...)
#ಆದರೆ_ಅಂತ್ಯ


#ವ್ಯಾಖ್ಯಾನಿಸು _try                                M3_ಫಲಿತಾಂಶ ಫಲಿತಾಂಶ = m3Err_none;
#ವ್ಯಾಖ್ಯಾನಿಸು _(TRY)                              { ಫಲಿತಾಂಶ = TRY; ಆದರೆ (M3_UNLIKELY(ಫಲಿತಾಂಶ)) { EXCEPTION_PRINT (ಫಲಿತಾಂಶ); ಹೋಗು _catch; } }
#ವ್ಯಾಖ್ಯಾನಿಸು _throw(ERROR)                       { ಫಲಿತಾಂಶ = ERROR; EXCEPTION_PRINT (ಫಲಿತಾಂಶ); ಹೋಗು _catch; }
#ವ್ಯಾಖ್ಯಾನಿಸು _throwif(ERROR, COND)               ಆದರೆ (M3_UNLIKELY(COND)) { _throw(ERROR); }

#ವ್ಯಾಖ್ಯಾನಿಸು _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#ಆದರೆ_ಅಂತ್ಯ // m3_exception_h
