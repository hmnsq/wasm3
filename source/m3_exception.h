//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#यदि_अपरिभाषितम् m3_exception_h
#परिभाषय m3_exception_h

#अन्तर्भाव "m3_config.h"

#यदि d_m3EnableExceptionBreakpoint

// declared in m3_info.c
शून्य ExceptionBreakpoint (cstr_t i_exception, cstr_t i_सन्देशः);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__FILE__ ":" M3_STR(__LINE__)))

#अन्यथा
#  define EXCEPTION_PRINT(...)
#यदि_अन्तः


#परिभाषय _try                                M3_परिणाम परिणाम = m3Err_none;
#परिभाषय _(TRY)                              { परिणाम = TRY; यदि (M3_UNLIKELY(परिणाम)) { EXCEPTION_PRINT (परिणाम); गच्छ _catch; } }
#परिभाषय _throw(ERROR)                       { परिणाम = ERROR; EXCEPTION_PRINT (परिणाम); गच्छ _catch; }
#परिभाषय _throwif(ERROR, COND)               यदि (M3_UNLIKELY(COND)) { _throw(ERROR); }

#परिभाषय _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#यदि_अन्तः // m3_exception_h
