//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#جے_بنا_تعریف m3_exception_h
#تعریف_کرو m3_exception_h

#ضمن "m3_config.h"

#إذا d_m3EnableExceptionBreakpoint

// declared in m3_info.c
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_سنیہا);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#وإلا
#  define EXCEPTION_PRINT(...)
#جے_انت


#تعریف_کرو _try                                M3_نتیجہ نتیجہ = m3Err_none;
#تعریف_کرو _(TRY)                              { نتیجہ = TRY; إذا (M3_UNLIKELY(نتیجہ)) { EXCEPTION_PRINT (نتیجہ); اذهب _catch; } }
#تعریف_کرو _throw(ERROR)                       { نتیجہ = ERROR; EXCEPTION_PRINT (نتیجہ); اذهب _catch; }
#تعریف_کرو _throwif(ERROR, COND)               إذا (M3_UNLIKELY(COND)) { _throw(ERROR); }

#تعریف_کرو _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#جے_انت // m3_exception_h
