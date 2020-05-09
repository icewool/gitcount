/* diffcount - compare and count source code line by line
               compare two source package and get NBNC changes
               diffcount.lib core file

   DIFF part comes from GNU DIFF
   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1998, 2001,
   2002 Free Software Foundation, Inc.

   This file is part of DIFFCOUNT.
   Count Part developed by C.YANG. 2006

   DIFFCOUNT is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;

   This file is part of HikSvnCount.
   SVN Count Part developed by Haiyuan.Qian 2013
   Software Configuration Management of HANGZHOU HIKVISION DIGITAL TECHNOLOGY CO.,LTD.
   email:qianhaiyuan@hikvision.com

   HikSvnCount is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;
   */

#define DIFFCOUNT_CORE


#include "diffcount.h"
#include <c-stack.h>
#include <dirname.h>
#include <error.h>
#include <exclude.h>
#include <exitfail.h>
#include <fnmatch.h>

#include <getopt.h>
#include <hard-locale.h>
#include <prepargs.h>
#include <quotesys.h>
#include <regex.h>
#include <setmode.h>
#include <xalloc.h>



int compare_files (struct comparison const *,
							char const *,char const *);



/*------------------Extern Interface for diffcount core --------------------*/

/* init diffcount parameters as default setting
   if the diffcount core module was invoked by
   third party program, the diffcount_init() should
   be executed first in initionalize process
*/
void diffcount_init()
{

  /*Default Initilization :

      IGNORE the difference of file name case.
      diff and count TWO packages
      SKIP the same file
      Do NOT print file result information.
      DO NOT print every diffed line information
  */
  ignore_file_name_case = false;
  counting_only = false;
  skip_identical_file = false;
  print_files_info = false;
  print_lines_info = false;
  for_program_reading = false;

  ignore_white_space = false;

  excluded = new_exclude ();

  if (horizon_lines < context)    horizon_lines = context;
}


/*switch option: ignore the file name case or not? */
void diffcount_set_ignore_case(bool b)
{
	if (b)
		ignore_file_name_case = true;
	else
		ignore_file_name_case = false;
}

void diffcount_set_program_reading(bool b)
{
	if (b)
	{
		for_program_reading = true;
	}
	else
		for_program_reading = false;
}

/*switch option:force the parsing all process or not ? */
void diffcount_set_parse_all(bool b)
{
	if (b)
		skip_identical_file = false;
	else
		skip_identical_file = true;
}


void diffcount_set_ignore_white_space(bool b)
{
	if (!counting_only)
	{
		if (b)
			ignore_white_space = true;
		else
			ignore_white_space = false;
	}
}



/*diff and count two packages ,invoked normally*/
int diffcount_compare_packages(char * left, char * right)
{
	return compare_files((struct comparison *) 0,left, right);
}

/* count only one package or file , invoked by --counting-only /-c options */
int diffcount_count_package(char * name)
{
	char * left = "_#_diffcount_temp_blank_#_";
	char * right = name;
	counting_only = true;
	return compare_files((struct comparison *) 0,left, right);
}


/*construct final diffcount result and return the pointer */
void diffcount_establish_result(struct diffcount_result * *result,
									int * lang_number, float * total_standard_c_nbnc_lines)
{
	int i=0,j=0,k=0;
	int result_lang_number = 0;
	float result_total_nbnc_lines = 0.0f;
	struct diffcount_result * return_result;

	while (languages[i].id != -1)
	{
		if (languages[i].constructed)
			result_lang_number ++;
		i++;
	}

	return_result = (struct diffcount_result *)\
					malloc(result_lang_number * sizeof(struct diffcount_result));

	while (languages[j].id != -1)
	{
		if (languages[j].constructed)
		{
			if (k < result_lang_number)
			{
				strcpy(return_result[k].language_name,languages[j].language_name);
				return_result[k].total_added_lines = languages[j].total_added_lines;
				return_result[k].total_deleted_lines = languages[j].total_deleted_lines;
				return_result[k].total_modified_lines = languages[j].total_modified_lines;
				return_result[k].changed_blank_lines = languages[j].changed_blank_lines;
				return_result[k].changed_comment_lines = languages[j].changed_comment_lines;
				return_result[k].changed_nbnc_lines = languages[j].changed_nbnc_lines;

				return_result[k].standard_c_rate = languages[j].standard_c_rate;
				return_result[k].standard_c_nbnc_lines = languages[j].changed_nbnc_lines * languages[j].standard_c_rate;

				result_total_nbnc_lines += return_result[k].standard_c_nbnc_lines;

				k ++;
			}
		}
		j++;
	}

	/*return data */
	*lang_number = result_lang_number;
	*total_standard_c_nbnc_lines = result_total_nbnc_lines;
	*result = return_result;

}



/*-----------------------------------------------------------------*/





/* before diff two actual file ,init the diff list and language pointer */
 void initialize_diff_list()
{
    /* construct the head of diffed data list */
	 line_node_head = line_node_tail = NULL;
	 current_language = NULL;
}


/*-- After diff processing, release the diff list and free all mem -- */
 void destroy_diff_list()
{
	struct diffed_line_node * p1 = line_node_head;
	struct diffed_line_node * p2;
	while (p1)
	{
		p2 = p1->next;
		free (p1);
		p1 = p2;
	}
	line_node_head = line_node_tail = NULL;
}

void svn_count_debug(const char *file, int line, const char *fmt, ...)
{
        va_list ap;

        if (!git_count_debug_on)
                return;

        fprintf(stderr, "(%s:%d:%d)", file, line, getpid());
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
}
/*
void
svn_count_debug(const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	int argno = 0;
	char tmp[4096] = {0};
	char chg_time[32] = {0};
	char log_path[256] = {0};
	FILE *fd = NULL;

	time_t t = time(0);
	memset(chg_time,0,32);
	strftime( chg_time, sizeof(chg_time), "%Y-%m-%d %X",localtime(&t) );

	if (!(fd = fopen( SVN_COUNT_LOG_PATH, "a+")))
	{
		printf("failed to open log file(%s).",log_path);
		return ;
	}

	if (svn_count_debug_on)
		fprintf(stderr, "(%s:%d:%d)", file, line, getpid());

	snprintf(tmp,4095,"%s (%s:%d:%d) ",chg_time,file, line, getpid());
	fwrite(tmp ,strlen(tmp),1,fd );

	va_start(ap, fmt);
	vsnprintf(tmp,4095,fmt,ap);
	va_end(ap);

	if (svn_count_debug_on)
		fprintf(stderr, tmp);

	fwrite(tmp ,strlen(tmp),1,fd );

	fclose(fd);
}*/


bool svn_count_mysql_query(MYSQL *p_mysql_conn,char * execsql)
{
	if(NULL == p_mysql_conn || NULL == execsql)
		return false;
	SVN_COUNT_DEBUG("execsql: %s \n",execsql);
	int n_count = 5;
start:

	if(mysql_query(p_mysql_conn,execsql) !=0)
	{
		SVN_COUNT_DEBUG("update failed: %s\n", execsql);
		char *Err = (char *)mysql_error(p_mysql_conn);
		if(strstr(Err,"MySQL server has gone away") && n_count>0)
		{
			mysql_close(p_mysql_conn);
			//初始化数据库链接
			n_count --;
			if(!connect_mysql(p_mysql_conn,&svn_count_connection,false))
			{
				printf("init mysql error\n");
				return false;
			}
			goto start;
		}
		SVN_COUNT_DEBUG("myql_error %s \n",  Err);

		return false;
	}

	if(mysql_affected_rows(p_mysql_conn)<=0)
	{
		SVN_COUNT_DEBUG("update failed: %s\n", execsql);
		return false;
	}
	return true;
}

/* Set the last-modified time of *ST to be the current time.  */

static void set_mtime_to_now (struct stat *st)
{
#ifdef ST_MTIM_NSEC

# if HAVE_CLOCK_GETTIME && defined CLOCK_REALTIME
  if (clock_gettime (CLOCK_REALTIME, &st->st_mtim) == 0)
    return;
# endif

# if HAVE_GETTIMEOFDAY
  {
    struct timeval timeval;
    if (gettimeofday (&timeval, NULL) == 0)
      {
	st->st_mtime = timeval.tv_sec;
	st->st_mtim.ST_MTIM_NSEC = timeval.tv_usec * 1000;
	return;
      }
  }
# endif

#endif /* ST_MTIM_NSEC */

  time (&st->st_mtime);
}





/* Compare and Count two files (or dirs) with parent comparison PARENT
   and names NAME0 and NAME1.
   (If PARENT is 0, then the first name is just NAME0, etc.)
   This is self-contained; it opens the files and closes them.

   Value is EXIT_SUCCESS if files are the same, EXIT_FAILURE if
   different, EXIT_TROUBLE if there is a problem opening them.  */

int compare_files (struct comparison const *parent,
	                        char const *name0,
	                        char const *name1 )
{
  struct comparison cmp;
  #define DIR_P(f) (S_ISDIR (cmp.file[f].stat.st_mode) != 0)
  register int f;
  int status = EXIT_SUCCESS;
  bool same_files;
  char *free0, *free1;

  trace3("-->Enter compare_files():name0 %s, name1 %s\n",name0,name1);

  /*construct the cmp stucture object */
  memset (cmp.file, 0, sizeof cmp.file);

  /* 对目录的比较采用树型结构进行遍历,初次遍历的根节点是0 */
  cmp.parent = parent;

  /* cmp.file[f].desc markers */
#define NONEXISTENT (-1) /* nonexistent file */
#define UNOPENED (-2) /* unopened file (e.g. directory) */
#define ERRNO_ENCODE(errno) (-3 - (errno)) /* encoded errno value */
#define ERRNO_DECODE(desc) (-3 - (desc)) /* inverse of ERRNO_ENCODE */




/* 如果遍历到了树叶，则标识为NONEXISTENT，否则就标识为UNOPENED
   但是UNOPENED本身分为两种情况，未打开的目录或者文件都是*/
  cmp.file[0].desc = name0 == 0 ? NONEXISTENT : UNOPENED;
  cmp.file[1].desc = name1 == 0 ? NONEXISTENT : UNOPENED;

  /* Now record the full name of each file, including nonexistent ones.  */
  if (name0 == 0)
  	{
  	  /*这时标明右边取到的文件，左边不存在，所以在左边"虚构"一个同名空文件进行DIFF*/
      name0 = name1;
      trace3("name0 == 0 ---> name0 = name1 :%s\n",name1);
    }
  if (name1 == 0)
  	{
  	/*这时标明左边取到的文件，右边不存在，所以在右边"虚构"一个同名空文件进行DIFF*/
    name1 = name0;
    trace3("name1 == 0  --> name1 = name0:%s\n",name0);
  	}

  if (!parent)
    {
      /* 目录树的根节点*/
      free0 = 0;
      free1 = 0;
      cmp.file[0].name = name0;
      cmp.file[1].name = name1;
      trace3("Now,At Root: file[0].name is %s, file[1].name is %s \n",name0,name1);
    }
  else
    {
      /* 目录树的下层节点 */
      cmp.file[0].name = free0
			= dir_file_pathname (parent->file[0].name, name0);
      cmp.file[1].name = free1
			= dir_file_pathname (parent->file[1].name, name1);
      trace3("Now, At Children: file[0].name is %s, file[1].name is %s \n",cmp.file[0].name,cmp.file[1].name);
    }


  /* Stat the files.(在没有遍历到树叶的情况下)*/
  for (f = 0; f < 2; f++)
    {
      if (cmp.file[f].desc != NONEXISTENT)
	{
	  if (f && file_name_cmp (cmp.file[f].name, cmp.file[0].name) == 0)
	    {
	      cmp.file[f].desc = cmp.file[0].desc;
	      cmp.file[f].stat = cmp.file[0].stat;
	    }
	  else if (strcmp (cmp.file[f].name, "-") == 0)
	    {
	      cmp.file[f].desc = STDIN_FILENO;
	      if (fstat (STDIN_FILENO, &cmp.file[f].stat) != 0)
		cmp.file[f].desc = ERRNO_ENCODE (errno);
	      else
		{
		  if (S_ISREG (cmp.file[f].stat.st_mode))
		    {
		      off_t pos = lseek (STDIN_FILENO, (off_t) 0, SEEK_CUR);
		      if (pos < 0)
			cmp.file[f].desc = ERRNO_ENCODE (errno);
		      else
			cmp.file[f].stat.st_size =
			  MAX (0, cmp.file[f].stat.st_size - pos);
		    }

		  /* POSIX 1003.1-2001 requires current time for
		     stdin.  */
		  set_mtime_to_now (&cmp.file[f].stat);
		}
	    }
	  else if (stat (cmp.file[f].name, &cmp.file[f].stat) != 0)
	    cmp.file[f].desc = ERRNO_ENCODE (errno);
	}
    }

  /* Mark files as nonexistent at the top level */
  if (! parent)
    {
      if (  cmp.file[0].desc == ERRNO_ENCODE (ENOENT)
	 		 && cmp.file[1].desc == UNOPENED)
	 			 cmp.file[0].desc = NONEXISTENT;

      if (cmp.file[0].desc == UNOPENED
	 	 && cmp.file[1].desc == ERRNO_ENCODE (ENOENT))
				cmp.file[1].desc = NONEXISTENT;
    }


  /* 叶子节点 */
  for (f = 0; f < 2; f++)
    if (cmp.file[f].desc == NONEXISTENT)
      cmp.file[f].stat.st_mode = cmp.file[1 - f].stat.st_mode;


  for (f = 0; f < 2; f++)
    {
      int e = ERRNO_DECODE (cmp.file[f].desc);
     if (0 <= e)
	 {
		  errno = e;
		  perror_with_name (cmp.file[f].name);
		  status = EXIT_TROUBLE;
	 }
    }


  if (status == EXIT_SUCCESS && ! parent && DIR_P (0) != DIR_P (1))
    {
      /* If one is a directory, and it was specified in the command line,
	 use the file in that dir with the other file's basename.  */

      int fnm_arg = DIR_P (0);
      int dir_arg = 1 - fnm_arg;
      char const *fnm = cmp.file[fnm_arg].name;
      char const *dir = cmp.file[dir_arg].name;
      char const *filename = cmp.file[dir_arg].name = free0
			= dir_file_pathname (dir, base_name (fnm));

      if (strcmp (fnm, "-") == 0)
	fatal ("cannot compare `-' to a directory");


      if (stat (filename, &cmp.file[dir_arg].stat) != 0)
	{
	  perror_with_name (filename);
	  status = EXIT_TROUBLE;
	}
    }


  if (status != EXIT_SUCCESS)
    {
      /* One of the files should exist but does not.  */
    }

  else if (same_files = (cmp.file[0].desc != NONEXISTENT
   				&& cmp.file[1].desc != NONEXISTENT
			   	&& 0 < same_file (&cmp.file[0].stat, &cmp.file[1].stat)
   				&& same_file_attributes (&cmp.file[0].stat,	&cmp.file[1].stat)))
   	{
   			/* The two named files are actually the same physical file.
   			We know they are identical without actually reading them.  */
   	}
  else if (DIR_P (0) & DIR_P (1))
    {

    /* If both are directories, compare the files in them.  */
    /*用递归的方式来进行目录树的遍历*/
	status = diff_dirs (&cmp, compare_files);
    }

  else if ((DIR_P (0) | DIR_P (1))
	   || (parent
	       && (! S_ISREG (cmp.file[0].stat.st_mode)
		   || ! S_ISREG (cmp.file[1].stat.st_mode))))
    {
      if (cmp.file[0].desc == NONEXISTENT || cmp.file[1].desc == NONEXISTENT)
	{
	  /* We have a subdirectory that exists only in one directory.  */

	  if ((DIR_P (0) | DIR_P (1))
	      && ( ( cmp.file[0].desc == NONEXISTENT)))
	    	status = diff_dirs (&cmp, compare_files);
	  else
	    {
	      char const *dir
			= parent->file[cmp.file[0].desc == NONEXISTENT].name;

	      /* See POSIX 1003.1-2001 for this format.  */
	      trace3 ("Only in %s: %s\n", dir, name0);
	      status = EXIT_FAILURE;
	    }
	}
    else
	{
	  /* We have two files that are not to be compared.  */

	  /* This is a difference.  */
	  status = EXIT_FAILURE;
	}
    }

  else
    {
      /* Both exist and neither is a directory.  */

      /* Open the files and record their descriptors.  */

      if (cmp.file[0].desc == UNOPENED)
	  if ((cmp.file[0].desc = open (cmp.file[0].name, O_RDONLY, 0)) < 0)
	  {
	    perror_with_name (cmp.file[0].name);
	    status = EXIT_TROUBLE;
	  }
      if (cmp.file[1].desc == UNOPENED)
	  {
	  if (same_files)
	    cmp.file[1].desc = cmp.file[0].desc;
	  else if ((cmp.file[1].desc = open (cmp.file[1].name, O_RDONLY, 0))
		   < 0)
	    {
	      perror_with_name (cmp.file[1].name);
	      status = EXIT_TROUBLE;
	    }
	}

      /* Compare and count the files, if no error was found.  */

      if (status == EXIT_SUCCESS)
      {
     	/*为了提高效率，对于可能相同的文件，尽量不进行比较,
      	   判断的依据是根据文件的属性*/
       	 same_files = same_file_attributes (&cmp.file[0].stat, &cmp.file[1].stat);

       	 if ((same_files) && skip_identical_file)
       	 {
			/*如果文件是相同的，就跳过，如果是debug状态，使用参数可以打印信息*/
		 	#ifdef DIFFCOUNT_DEBUG
		 	if (print_files_info )
			{
				printf("Escape SAME Files: %s and %s \n",
									   cmp.file[0].name, cmp.file[1].name);
			}//end if print_file_info
			#endif

         }
         else
         {
    	 	/* construct the head of diffed data list */
		    initialize_diff_list();
      	 	status = diff_2_files (&cmp);
		 	/* destroy the list head by line_node_head */
		 	destroy_diff_list();
         }
      }
      /* Close the file descriptors.  */
    if (0 <= cmp.file[0].desc && close (cmp.file[0].desc) != 0)
	{
	  perror_with_name (cmp.file[0].name);
	  status = EXIT_TROUBLE;
	}
    if (0 <= cmp.file[1].desc && cmp.file[0].desc != cmp.file[1].desc
	  && close (cmp.file[1].desc) != 0)
	{
	  perror_with_name (cmp.file[1].name);
	  status = EXIT_TROUBLE;
	}
    }

  /* Now the comparison has been done, if no error prevented it,
     and STATUS is the value this function will return.  */


  if (free0)
    free (free0);
  if (free1)
    free (free1);

  trace3("<--Quit compare_diff_files function .. with Status: %d\n",status);
  return status;
}




