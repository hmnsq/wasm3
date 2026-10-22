//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#ifndef m3_exception_h
#define m3_exception_h

#அந்தர்பாவ "m3_config.h"

#எனில் d_m3EnableExceptionBreakpoint

// declared in m3_info.c
வெறுமை ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#இல்லையெனில்
#  define EXCEPTION_PRINT(...)
#endif


#define _try                                M3Result result = m3Err_none;
#define _(TRY)                              { result = TRY; எனில் (M3_UNLIKELY(result)) { EXCEPTION_PRINT (result); செல் _catch; } }
#define _throw(ERROR)                       { result = ERROR; EXCEPTION_PRINT (result); செல் _catch; }
#define _throwif(ERROR, COND)               எனில் (M3_UNLIKELY(COND)) { _throw(ERROR); }

#define _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#endif // m3_exception_h
