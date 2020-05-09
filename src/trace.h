/* Tracing Tools for DIFFCOUNT

   This file is part of DIFFCOUNT.
   Print the trace for program debuging
   Developed by C.YANG. 2006 

   DIFFCOUNT is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;  */


/***************************************************
 * function List :
 * trace()
 * trace1()
 * trace2()
 * trace3()
 * debugprint()
 * *************************************************/

#ifndef _DIFFCOUNT_DEBUG_H
#define _DIFFCOUNT_DEBUG_H      1
#include<stdio.h>

/* comment|Uncomment the line for change the macro DEBUG for Mode */
//#define DIFFCOUNT_DEBUG

/*flag the trace level, [0..3], 3 open all trace message , 0 only open "trace()" */
#define TRACE_LEVEL 3
 

#ifdef DIFFCOUNT_DEBUG

/* common printf  */
#define debugprint(str,args...) printf(str,##args)

#define trace(str,args...) printf("<%s>:<%d>:<%s>: ",__FILE__,__LINE__,__FUNCTION__);printf(str,##args)
  #if TRACE_LEVEL < 1
     #define trace1(str,args...)                                                                                      
  #else                                                                
      #define trace1(str,args...) printf("<%s>:<%d>:<%s>: ",__FILE__,__LINE__,__FUNCTION__);printf(str,##args)
  #endif
  
   #if TRACE_LEVEL < 2
     #define trace2(str,args...)                                                                                      
  #else                                                                
      #define trace2(str,args...) printf("<%s>:<%d>:<%s>: ",__FILE__,__LINE__,__FUNCTION__);printf(str,##args)
  #endif
  
   #if TRACE_LEVEL < 3
     #define trace3(str,args...)                                                                                      
  #else                                                                
      #define trace3(str,args...) printf("<%s>:<%d>:<%s>: ",__FILE__,__LINE__,__FUNCTION__);printf(str,##args)
  #endif

#else
#define trace(str,args...)
#define trace1(str,args...)
#define trace2(str,args...)
#define trace3(str,args...)
#define debugprint(str,args...)


#endif /*DIFFCOUNT_DEBUG*/
#endif /*_DIFFCOUNT_DEBUG_H*/

