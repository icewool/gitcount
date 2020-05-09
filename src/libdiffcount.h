/* Common Head File for DIFFCOUNT CORE LIB
   
   You can embed diffcount core lib (libdiffcount.a)
              into your own customized applications.

   DIFF part comes from GNU DIFF
   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1998, 2001,
   2002 Free Software Foundation, Inc.

   This file is part of DIFFCOUNT.
   Count Part developed by C.YANG. 2006 

   DIFFCOUNT is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;  */

#ifndef _LIB_DIFFCOUNT_H_
#define _LIB_DIFFCOUNT_H_

#include <stdio.h>

/*define a boolean type: bool */
typedef enum {false = 0, true = 1} bool;


/* store the diff count result based on individual language type */
/*  ----------------------- NOTES ---------------------------
   counting only(1) and diffcount(2) outputs share 
   the SAME DATA STRUCTRURE -- "diffcount_result"
   
   You can get the counting_only result of ONE package
   or file using this way:("==>" means "data from")
   
   Total Code Lines ===> diffcount_result.total_added_lines
   Total Blank Lines ==> diffcount_result.changed_blank_lines
   Total Comment Lines ==> diffcount_result.changed_comment_lines
   Total NBNC lines ==> diffcount_result.changed_nbnc_lines
   NBNC Convert to standard C Lines 
              ==> diffcount_result.standard_c_nbnc_lines 
 ---------------------------------------------------------------*/
 
struct diffcount_result 
{
    char language_name [25];  /* language name eg:PASCAL/DELPHI */    
    unsigned int total_added_lines;
    unsigned int total_deleted_lines;
    unsigned int total_modified_lines;  
        /* data of diff result for one kind of language */
    unsigned int changed_blank_lines;
    unsigned int changed_comment_lines;
    unsigned int changed_nbnc_lines; 	/*all changed nbnc */
	float standard_c_rate; 				/*convert rate to standard c */
	float standard_c_nbnc_lines; /*Convert to standard C nbnc*/
	
};


/* init diffcount parameters as default setting 
   if the diffcount core module was invoked by 
   third party program, the diffcount_init() should
   be executed first in initionalization process
*/
void diffcount_init();

/*switch option: ignore the file name case or not? */
void diffcount_set_ignore_case(bool b);

/*switch option:force the parsing all process or not ? */
void diffcount_set_parse_all(bool b);

/* DO NOT USE return value of diffcount_xxxx() 
   It just return the mid data of process  */
/*diff and count two packages ,invoked normally*/
int diffcount_compare_packages(char * left, char * right);


/* count only one package or file , 
	invoked by --counting-only or -c options */
int diffcount_count_package(char * name);


/*
construct final diffcount result and return the pointer 
NOTES:the three parameters should be pointer or pointer of pointer
*/
void diffcount_establish_result(struct diffcount_result * *result, 
									int * lang_number, float * total_standard_c_nbnc_lines);

#endif

