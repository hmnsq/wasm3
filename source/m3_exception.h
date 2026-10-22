//
//  m3_exception.h
//
//  Created by Steven Massey on 7/5/19.
//  Copyright © 2019 Steven Massey. All rights reserved.
//
//  some macros to emulate try/catch

#अगर_अघोषित m3_exception_h
#घोषणा m3_exception_h

#समावेश "m3_config.h"

#अगर d_m3EnableExceptionBreakpoint

// declared in m3_info.c
व्योम ExceptionBreakpoint (cstr_t i_exception, cstr_t i_message);

#  define EXCEPTION_PRINT(ERROR) ExceptionBreakpoint (ERROR, (__खाता__ ":" M3_STR(__LINE__)))

#अन्यथा
#  define EXCEPTION_PRINT(...)
#पूर्ण_अगर


#घोषणा _try                                M3_परिणाम परिणाम = m3Err_none;
#घोषणा _(TRY)                              { परिणाम = TRY; अगर (M3_UNLIKELY(परिणाम)) { EXCEPTION_PRINT (परिणाम); जाओ _catch; } }
#घोषणा _throw(ERROR)                       { परिणाम = ERROR; EXCEPTION_PRINT (परिणाम); जाओ _catch; }
#घोषणा _throwif(ERROR, COND)               अगर (M3_UNLIKELY(COND)) { _throw(ERROR); }

#घोषणा _throwifnull(PTR)                   _throwif (m3Err_mallocFailed, !(PTR))

#पूर्ण_अगर // m3_exception_h
