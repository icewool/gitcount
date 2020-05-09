/* Support routines for DIFFCOUNT.

   DIFF part comes from GNU DIFF
   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1998, 2001,
   2002 Free Software Foundation, Inc.

   This file is part of DIFFCOUNT.
   Count Part developed by C.YANG. 2006 

   DIFFCOUNT is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;  */
   

#include "diffcount.h"
#include <dirname.h>
#include <error.h>
#include <quotesys.h>
#include <regex.h>
#include <xalloc.h>


#include <ctype.h>
#include <string.h>
#include <stdlib.h>


/* Queue up one-line messages to be printed at the end,
   when -l is specified.  Each message is recorded with a `struct msg'.  */

#define TAB_WIDTH 8

struct msg
{
  struct msg *next;
  char args[1]; /* Format + 4 args, each '\0' terminated, concatenated.  */
};

/* Head of the chain of queues messages.  */

static struct msg *msg_chain;

/* Tail of the chain of queues messages.  */

static struct msg **msg_chain_end = &msg_chain;

/* Use when a system call returns non-zero status.
   NAME should normally be the file name.  */

void perror_with_name (char const *name)
{
  error (0, errno, "%s", name);
}

/* Use when a system call returns non-zero status and that is fatal.  */

void pfatal_with_name (char const *name)
{
  int e = errno;
  print_message_queue ();
  error (EXIT_TROUBLE, e, "%s", name);
  abort ();
}

/* Print an error message containing MSGID, then exit.  */

void fatal (char const *msgid)
{
  print_message_queue ();
  error (EXIT_TROUBLE, 0, "%s", _(msgid));
  abort ();
}


/* Like printf, except if -l in effect then save the message and print later.
   This is used for things like "Only in ...".  */

void message (char const *format_msgid, char const *arg1, char const *arg2)
{
  message5 (format_msgid, arg1, arg2, 0, 0);
}

void message5 (char const *format_msgid, char const *arg1, char const *arg2,
	  char const *arg3, char const *arg4)
{
      printf (_(format_msgid), arg1, arg2, arg3, arg4); 
}

/* Output all the messages that were saved up by calls to `message'.  */

void print_message_queue (void)
{
  char const *arg[5];
  int i;
  struct msg *m = msg_chain;

  while (m)
    {
      struct msg *next = m->next;
      arg[0] = m->args;
      for (i = 0;  i < 4;  i++)
	arg[i + 1] = arg[i] + strlen (arg[i]) + 1;
      printf (_(arg[0]), arg[1], arg[2], arg[3], arg[4]);
      free (m);
      m = next;
    }
}


/* Call before outputting the results of comparing files NAME0 and NAME1
   to set up OUTFILE, the stdio stream for the output to go to.

   Usually, OUTFILE is just stdout.  But when -l was specified
   we fork off a `pr' and make OUTFILE a pipe to it.
   `pr' then outputs to our stdout.  */

static char const *current_name0;
static char const *current_name1;
static bool currently_recursive;


/* Compare two lines (typically one from each input file)
   according to the command line options.
   For efficiency, this is invoked only when the lines do not match exactly
   but an option like -i might cause us to ignore the difference.
   Return nonzero if the lines differ.  */

bool lines_differ (char const *s1, char const *s2)
{
  register unsigned char const *t1 = (unsigned char const *) s1;
  register unsigned char const *t2 = (unsigned char const *) s2;
  size_t column = 0;
  
	trace3("enter the lines-differ function...\n");
  while (1)
    {
      register unsigned char c1 = *t1++;
      register unsigned char c2 = *t2++;

	   /* Test for exact char equality first, since it's a common case.  */
      if (c1 != c2)
	  {
		  if (ignore_white_space)
		   	{
			  trace3("skip space charactor in line differ...\n");
		      while (isspace (c1) && c1 != '\n') c1 = *t1++;
		      while (isspace (c2) && c2 != '\n') c2 = *t2++;
		  	}

		  if (c1 != c2)
		    break;
	   }
	  
      if (c1 == '\n')
			return 0;

      column += c1 == '\t' ? TAB_WIDTH - column % TAB_WIDTH : 1;
    }

  return 1;
}


/* Find the consecutive changes at the start of the script START.
   Return the last link before the first gap.  */

struct change * find_change (struct change *start)
{
  return start;
}

struct change * find_reverse_change (struct change *start)
{
  return start;
}

/* Divide SCRIPT into pieces by calling HUNKFUN and
   print each piece with PRINTFUN.
   Both functions take one arg, an edit script.

   HUNKFUN is called with the tail of the script
   and returns the last link that belongs together with the start
   of the tail.

   PRINTFUN takes a subscript which belongs together (with a null
   link at the end) and prints it.  */

void process_script (struct change *script,
	      struct change * (*hunkfun) (struct change *),
	      void (*printfun) (struct change *))
{
  struct change *next = script;
  while (next)
    {
      struct change *this, *end;

      /* Find a set of changes that belong together.  */
      this = next;
      end = (*hunkfun) (next);

      /* Disconnect them from the rest of the changes,
	 making them a hunk, and remember the rest for next iteration.  */
      next = end->link;
      end->link = 0;


      /* Print this hunk.  */
      (*printfun) (this);

      /* Reconnect the script so it will all be freed properly.  */
      end->link = next;
    }

  
}



/* Translate an internal line number (an index into diff's table of lines)
   into an actual line number in the input file.
   The internal line number is I.  FILE points to the data on the file.

   Internal line numbers count from 0 starting after the prefix.
   Actual line numbers count from 1 within the entire file.  */

lin translate_line_number (struct file_data const *file, lin i)
{
  return i + file->prefix_lines + 1;
}

/* Translate a line number range.  This is always done for printing,
   so for convenience translate to long rather than lin, so that the
   caller can use printf with "%ld" without casting.  */

void translate_range (struct file_data const *file,
		 lin a, lin b,
		 long *aptr, long *bptr)
{
  *aptr = translate_line_number (file, a - 1) + 1;
  *bptr = translate_line_number (file, b + 1) - 1;
}



/* Look at a hunk of edit script and report the range of lines in each file
   that it applies to.  HUNK is the start of the hunk, which is a chain
   of `struct change'.  The first and last line numbers of file 0 are stored in
   *FIRST0 and *LAST0, and likewise for file 1 in *FIRST1 and *LAST1.
   Note that these are internal line numbers that count from 0.

   If no lines from file 0 are deleted, then FIRST0 is LAST0+1.

   Return UNCHANGED if only ignorable lines are inserted or deleted,
   OLD if lines of file 0 are deleted,
   NEW if lines of file 1 are inserted,
   and CHANGED if both kinds of changes are found. */

enum changes analyze_hunk (struct change *hunk,
	      lin *first0, lin *last0,
	      lin *first1, lin *last1)
{
  struct change *next;
  lin l0, l1;
  lin show_from, show_to;
  lin i;
  bool trivial = ignore_blank_lines || ignore_regexp.fastmap;
  size_t trivial_length = (int) ignore_blank_lines - 1;
    /* If 0, ignore zero-length lines;
       if SIZE_MAX, do not ignore lines just because of their length.  */

  char const * const *linbuf0 = files[0].linbuf;  /* Help the compiler.  */
  char const * const *linbuf1 = files[1].linbuf;

  show_from = show_to = 0;

  *first0 = hunk->line0;
  *first1 = hunk->line1;

  next = hunk;
  do
    {
      l0 = next->line0 + next->deleted - 1;
      l1 = next->line1 + next->inserted - 1;
      show_from += next->deleted;
      show_to += next->inserted;

      for (i = next->line0; i <= l0 && trivial; i++)
	{
	  char const *line = linbuf0[i];
	  size_t len = linbuf0[i + 1] - line - 1;
	  if (len != trivial_length
	      && (! ignore_regexp.fastmap
		  || re_search (&ignore_regexp, line, len, 0, len, 0) < 0))
	    trivial = 0;
	}

      for (i = next->line1; i <= l1 && trivial; i++)
	{
	  char const *line = linbuf1[i];
	  size_t len = linbuf1[i + 1] - line - 1;
	  if (len != trivial_length
	      && (! ignore_regexp.fastmap
		  || re_search (&ignore_regexp, line, len, 0, len, 0) < 0))
	    trivial = 0;
	}
    }
  while ((next = next->link) != 0);

  *last0 = l0;
  *last1 = l1;

  /* If all inserted or deleted lines are ignorable,
     tell the caller to ignore this hunk.  */

  if (trivial)
    return UNCHANGED;

  return (show_from ? OLD : UNCHANGED) | (show_to ? NEW : UNCHANGED);
}


/* Concatenate three strings, returning a newly malloc'd string.  */

char *
concat (char const *s1, char const *s2, char const *s3)
{
  char *new = xmalloc (strlen (s1) + strlen (s2) + strlen (s3) + 1);
  sprintf (new, "%s%s%s", s1, s2, s3);
  return new;
}

/* Yield a new block of SIZE bytes, initialized to zero.  */

void *
zalloc (size_t size)
{
  void *p = xmalloc (size);
  memset (p, 0, size);
  return p;
}

/* Yield the newly malloc'd pathname
   of the file in DIR whose filename is FILE.  */

char *
dir_file_pathname (char const *dir, char const *file)
{
  #ifdef _DIFFCOUNT_WIN32_  /* make MINGW fun .Add by caoyang*/
  char const *base = base_name (dir);
  bool omit_slash = !*base || base[strlen (base) - 1] == '\\';
  return concat (dir, "\\" + omit_slash, file);
  #else
  char const *base = base_name (dir);
  bool omit_slash = !*base || base[strlen (base) - 1] == '/';
  return concat (dir, "/" + omit_slash, file);
  #endif
}



/* -- 在字符数组中进行模式匹配 
   -- 注意字符数组靠长度进行定位，没有'\0'符号 
   -- 使用KMP模式匹配算法         
*/
int find_string_in_array(char *S, char *T, int s_array_len)
{
	int next[50]; 
	int T_len = strlen(T);
	int S_len = s_array_len;
	int next_i=0;
	int next_j=-1;
	int kmp_i=0;
	int kmp_j=0;
	
	if (*T == '\0') 
			return -1;
	if (S_len == 0)
			return -1;
	
	/* -- Get Next Array ---*/
	next[0] = -1; 
	while(next_i < T_len)
	{
		if((next_j == -1) || (T[next_i] == T[next_j])) 
		{
			next_i++;
			next_j++;
			next[next_i] = next_j;
		}
		else 
		{ 
			next_j = next[next_j]; 
		} 
		
	}
	
	
	/*-- Kmp Search string  --*/	
	while((kmp_i < S_len) && (kmp_j < T_len)) 
	{ 
		if((kmp_j == -1) || (S[kmp_i] == T[kmp_j])) 
		{ 
			kmp_i++; 
			kmp_j++; 
		} 
		else 
		{ 
			kmp_j = next[kmp_j]; 
		} 
	} 
	
	if(kmp_j >= T_len) 
	{ 
		return (kmp_i-T_len); 
	} 
	else 
	{ 
		return -1; 
	} 
}


int find_string(char * S, char * T)
{
	return find_string_in_array(S,T,strlen(S));
}









int find_string_in_array_ignore_case(char *S, char *T, int s_array_len)
{
	int next[50]; 
	int T_len = strlen(T);
	int S_len = s_array_len;
	int next_i=0;
	int next_j=-1;
	int kmp_i=0;
	int kmp_j=0;

	int case_i = 0;

	
	if (*T == '\0') 
			return -1;
	if (S_len == 0)
			return -1;

    /************* force trans all strings to lower case ********/
	for (case_i=0; case_i<T_len; case_i++)
	{		
		if( isupper(T[case_i]) )
				T[case_i]=tolower(T[case_i]);
	}

	
	for (case_i=0; case_i<S_len; case_i++)
	{		
		if( isupper(S[case_i]) )
				S[case_i]=tolower(S[case_i]);
	}

	/************************************************************/

	
	/* -- Get Next Array ---*/
	next[0] = -1; 
	while(next_i < T_len)
	{
		if((next_j == -1) || (T[next_i] == T[next_j])) 
		{
			next_i++;
			next_j++;
			next[next_i] = next_j;
		}
		else 
		{ 
			next_j = next[next_j]; 
		} 
		
	}
	
	
	/*-- Kmp Search string  --*/	
	while((kmp_i < S_len) && (kmp_j < T_len)) 
	{ 
		if((kmp_j == -1) || (S[kmp_i] == T[kmp_j])) 
		{ 
			kmp_i++; 
			kmp_j++; 
		} 
		else 
		{ 
			kmp_j = next[kmp_j]; 
		} 
	} 
	
	if(kmp_j >= T_len) 
	{ 
		return (kmp_i-T_len); 
	} 
	else 
	{ 
		return -1; 
	} 
}


int find_string_ignore_case(char * S, char * T)
{
	return find_string_in_array_ignore_case(S,T,strlen(S));
}


