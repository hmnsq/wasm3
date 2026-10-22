//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#اگر_غیر_تعریف_شدہ m3_exception_h
#تعریف_کرو m3_exception_h

#شامل "m3_config.h"

#اگر d_m3EnableExceptionBreakpoint

// declared in m3_info.c
خالی ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پیغام);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#ورنہ
#  define EXCEPTION_PRINT(...)
#اگر_ختم


#تعریف_کرو _try                                M3_نتیجہ نتیجہ = m3Err_none;
#تعریف_کرو _(TRY)                              { نتیجہ = TRY; اگر (M3_UNLIKELY(نتیجہ)) { EXCEPTION_PRINT (نتیجہ); جاؤ _catch; } }
#تعریف_کرو _throw(ERROR)                       { نتیجہ = ERROR; EXCEPTION_PRINT (نتیجہ); جاؤ _catch; }
#تعریف_کرو _throwif(ERROR, COND)               اگر (M3_UNLIKELY(COND)) { _throw(ERROR); }

#تعریف_کرو _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#اگر_ختم // m3_exception_h
