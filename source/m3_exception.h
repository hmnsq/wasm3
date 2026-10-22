//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#که_نه_تعریف_شوی m3_exception_h
#تعریف_کړه m3_exception_h

#ضمن "m3_config.h"

#إذا d_m3EnableExceptionBreakpoint

// declared in m3_info.c
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پیغام);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#وإلا
#  define EXCEPTION_PRINT(...)
#د_که_پای


#تعریف_کړه _try                                M3_پایله پایله = m3Err_none;
#تعریف_کړه _(TRY)                              { پایله = TRY; إذا (M3_UNLIKELY(پایله)) { EXCEPTION_PRINT (پایله); اذهب _catch; } }
#تعریف_کړه _throw(ERROR)                       { پایله = ERROR; EXCEPTION_PRINT (پایله); اذهب _catch; }
#تعریف_کړه _throwif(ERROR, COND)               إذا (M3_UNLIKELY(COND)) { _throw(ERROR); }

#تعریف_کړه _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#د_که_پای // m3_exception_h
