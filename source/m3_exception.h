//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ m3_exception_h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ m3_exception_h

#ਅਨ੍ਤਰ੍ਭਾਵ "m3_config.h"

#ਜੇ d_m3EnableExceptionBreakpoint

// declared in m3_info.c
ਖਾਲੀ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_ਸੁਨੇਹਾ);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#ਨਹੀਂਤਾਂ
#  define EXCEPTION_PRINT(...)
#ਜੇ_ਅੰਤ


#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ _try                                M3_ਨਤੀਜਾ ਨਤੀਜਾ = m3Err_none;
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ _(TRY)                              { ਨਤੀਜਾ = TRY; ਜੇ (M3_UNLIKELY(ਨਤੀਜਾ)) { EXCEPTION_PRINT (ਨਤੀਜਾ); ਜਾਓ _catch; } }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ _throw(ERROR)                       { ਨਤੀਜਾ = ERROR; EXCEPTION_PRINT (ਨਤੀਜਾ); ਜਾਓ _catch; }
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ _throwif(ERROR, COND)               ਜੇ (M3_UNLIKELY(COND)) { _throw(ERROR); }

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#ਜੇ_ਅੰਤ // m3_exception_h
