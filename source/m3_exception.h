//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#నిర్వచితం_కాకపోతే m3_exception_h
#నిర్వచించు m3_exception_h

#అన్తర్భావ "m3_config.h"

#ఒకవేళ d_m3EnableExceptionBreakpoint

// declared in m3_info.c
శూన్య ExceptionBreakpoint (cstr_t i_exception, cstr_t i_సందేశం);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#లేకపోతే
#  define EXCEPTION_PRINT(...)
#అయితే_ముగింపు


#నిర్వచించు _try                                M3_ఫలితం ఫలితం = m3Err_none;
#నిర్వచించు _(TRY)                              { ఫలితం = TRY; ఒకవేళ (M3_UNLIKELY(ఫలితం)) { EXCEPTION_PRINT (ఫలితం); వెళ్ళు _catch; } }
#నిర్వచించు _throw(ERROR)                       { ఫలితం = ERROR; EXCEPTION_PRINT (ఫలితం); వెళ్ళు _catch; }
#నిర్వచించు _throwif(ERROR, COND)               ఒకవేళ (M3_UNLIKELY(COND)) { _throw(ERROR); }

#నిర్వచించు _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#అయితే_ముగింపు // m3_exception_h
