//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#اگر_تعریف_نشده m3_exception_h
#تعریف_کن m3_exception_h

#ضمن "m3_config.h"

#إذا d_m3EnableExceptionBreakpoint

// declared in m3_info.c
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پیام);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#وإلا
#  define EXCEPTION_PRINT(...)
#پایان_اگر


#تعریف_کن _try                                M3_نتیجه نتیجه = m3Err_none;
#تعریف_کن _(TRY)                              { نتیجه = TRY; إذا (M3_UNLIKELY(نتیجه)) { EXCEPTION_PRINT (نتیجه); اذهب _catch; } }
#تعریف_کن _throw(ERROR)                       { نتیجه = ERROR; EXCEPTION_PRINT (نتیجه); اذهب _catch; }
#تعریف_کن _throwif(ERROR, COND)               إذا (M3_UNLIKELY(COND)) { _throw(ERROR); }

#تعریف_کن _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#پایان_اگر // m3_exception_h
