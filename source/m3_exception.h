//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#جيڪڏهن_وصف_نه_ٿيل m3_exception_h
#وصف_ڪريو m3_exception_h

#ضمن "m3_config.h"

#إذا d_m3EnableExceptionBreakpoint

// declared in m3_info.c
فراغ ExceptionBreakpoint (cstr_t i_exception, cstr_t i_پيغام);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#وإلا
#  define EXCEPTION_PRINT(...)
#جيڪڏهن_پورو


#وصف_ڪريو _try                                M3_نتيجو نتيجو = m3Err_none;
#وصف_ڪريو _(TRY)                              { نتيجو = TRY; إذا (M3_UNLIKELY(نتيجو)) { EXCEPTION_PRINT (نتيجو); اذهب _catch; } }
#وصف_ڪريو _throw(ERROR)                       { نتيجو = ERROR; EXCEPTION_PRINT (نتيجو); اذهب _catch; }
#وصف_ڪريو _throwif(ERROR, COND)               إذا (M3_UNLIKELY(COND)) { _throw(ERROR); }

#وصف_ڪريو _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#جيڪڏهن_پورو // m3_exception_h
