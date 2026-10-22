//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#ifndef m3_exception_h
#define m3_exception_h

#ضمن "m3_config.h"

#إذا d_m3EnableExceptionBreakpoint

// declared in m3_info.c
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message);

#  define EXCEPTION_بغ(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#وإلا
#  define EXCEPTION_بغ(...)
#endif


#define _try                                M3Result result = m3Err_none;
#define _(TRY)                              { result = TRY; إذا (M3_UNLIKELY(result)) { EXCEPTION_بغ (result); اذهب _catch; } }
#define _throw(ERROR)                       { result = ERROR; EXCEPTION_بغ (result); اذهب _catch; }
#define _throwif(ERROR, COND)               إذا (M3_UNLIKELY(COND)) { _throw(ERROR); }

#define _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#endif // m3_exception_h
