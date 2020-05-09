/* diffcount - compare and count source code line by line
               compare two source package and get NBNC changes
               cmd.c: command line interface

   DIFF part comes from GNU DIFF
   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1998, 2001,
   2002 Free Software Foundation, Inc.

   This file is part of DIFFCOUNT.
   Count Part developed by C.YANG. 2006

   This file is part of HikSvnCount.
   SVN Count Part developed by Haiyuan.Qian 2013
   Software Configuration Management of HANGZHOU HIKVISION DIGITAL TECHNOLOGY CO.,LTD.
   email:qianhaiyuan@hikvision.com

   HikSvnCount is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation;
   */


#define DIFFCOUNT_CMD

#include "diffcount.h"
#include <error.h>
#include <exclude.h>
#include <exitfail.h>
#include <getopt.h>
#include <sys/time.h>

long getCurrentTime()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static char const shortopts[] =
"0123456789a:b:cC:dD:e:EfF:g:hH:lL:n:p:Pqr:sS:tTuU:vwW:x:X:y";

/* Values for long options that do not have single-letter equivalents.  */
/* -- 对于只有长参数的参数解析，在这里添加映射关系 ----*/
enum
{
  HELP_OPTION=CHAR_MAX + 1,
  HELP_OPTION_CN,
  IGNORE_CASE_OPTION,
  PRINT_LINES_INFO_OPTION,
  PRINT_FILES_INFO_OPTION,
  FORCE_PARSE_ALL_OPTION,
  FOR_PROGRAM_READING_OPTION,
  IGNORE_WHITE_SPACE_OPTION,
  LANGS_OPTION,
  GIT_POST_RECIEVE_OPTION,
  GIT_END_REVISION_OPTION,
  GIT_COUNT_PROCESS,
  GIT_COUNT_UPDATE
};

char server_ip[32];
char server_id[11];

#define IFNAMSIZ   16

/*-- Adapt short opts and long opts -----
  the relationship list as below
  e.g:
	--print-lines-info
	--help
	-v <---> --version
------------------------------------------*/
static struct option const longopts[] =
{
  {"ignore-filename-case", 0, 0, IGNORE_CASE_OPTION},
  {"print-lines-info", 0, 0, PRINT_LINES_INFO_OPTION},
  {"print-files-info", 0, 0, PRINT_FILES_INFO_OPTION},
  {"force-parse-all", 0, 0, FORCE_PARSE_ALL_OPTION},
  {"for-program-reading", 0, 0, FOR_PROGRAM_READING_OPTION},
  {"ignore-white-space", 0, 0, IGNORE_WHITE_SPACE_OPTION},
  {"help", 0, 0, HELP_OPTION},
  {"cn-help", 0, 0, HELP_OPTION_CN},
  {"count-only", 0, 0, 'c'},
  {"version", 0, 0, 'v'},
  {"langs", 0, 0, LANGS_OPTION},
  {"git-post-recieve", 0, 0, GIT_POST_RECIEVE_OPTION},
  {"end-revision", 1, 0, GIT_END_REVISION_OPTION},
  {"process", 0, 0, GIT_COUNT_PROCESS},
  {"update", 0, 0, GIT_COUNT_UPDATE},
  /* end flag */
  {0, 0, 0, 0}
};



static void check_stdout (void)
{
  if (ferror (stdout))
    fatal ("write failed");
  else if (fclose (stdout) != 0)
    pfatal_with_name (_("standard output"));
}



/*----DIFF COUNT HELP---------------*/
static void try_help (char const *reason_msgid, char const *operand)
{
  if (reason_msgid)
    error (0, 0, _(reason_msgid), operand);
  error (EXIT_TROUBLE, 0, _("Try `%s --help or --cn-help' for more information."),
	 program_name);
  abort ();
}


//中文的帮助说明
static char const * const option_help_msgid_cn[] = {
  N_("功能：对两个代码包/文件进行差异统计、或者对一个代码包/文件进行代码行统计"),
  "",
  N_("参数说明："),
  "",
  N_(" --git-post-recieve "),
  N_("     GIT提交(post recieve)代码行统计"),
  "",
  N_(" --process "),
  N_("     阻塞方式，显示统计过程，如无该参数，本程序调用后创建独立进程后立即返回结果"),
  "",
  N_(" --update "),
  N_("     更新模式，用于一些异常数据的修复"),
  "",
  N_(" -p (仅在“GIT提交”时有效）"),
  N_("     版本库路径(服务器端数据路径)"),
  "",
  N_(" -g (仅在“GIT提交”时有效）"),
  N_("     待统计的版本的祖先版本"),
  N_(" -r (仅在“GIT提交”时有效）"),
  N_("     待统计的版本"),
  N_(" -b (仅在“GIT提交”时有效）"),
  N_("     待统计的分支"),
  N_(" -n (仅在“GIT提交”时有效）"),
  N_("     待统计的repo ID"),
  "",
  N_(" --end-revision (可选参数,仅在“GIT提交”时有效）"),
  N_("     同用于带统计至结束版本之间的版本批量统计"),
  N_("     请注意，该值必须小于待统计版本"),
  "",
  N_("  -c --count-only  FILE/DIR"),
  N_("    仅仅对一个文件或者目录完成统计代码行的功能，没有比较对象 "),
  N_("    这样代码包/文件参数的数量只能有一个（文件或目录） "),
  "",
  N_("  --ignore-filename-case  (仅在差异统计时使用)"),
  N_("    在比较的时候，忽略文件名的大小写差异(缺省情况会作为两个完全不同的文件）"),
  N_("    (仅在“差异统计”时有效）"),
  "",
  N_("  --ignore-white-space  (仅在差异统计时使用)"),
  N_("    在比较的时候，忽略代码行中仅仅由于空格(包括制表符)造成的差异"),
  N_("    (仅在“差异统计”时有效）"),
  "",
  N_("  --print-lines-info  (仅在调试时使用) "),
  N_("    输出每个文件差异比较后的差异结果和初步统计信息（逐行）,主要用于分析错误 "),
  N_("    注意不要针对大型代码包使用，否则逐行打印时间消耗惊人"),
  "",
  N_("  --print-files-info"),
  N_("    使用这个开关，当每个文件差异比较和统计以后，输出该文件差异统计的结果信息"),
  N_("    缺省不打印每个文件的信息，只打印最后的结果"),
  "",
  N_("  --force-parse-all (仅在差异统计时使用) "),
  N_("    对可能相同的文件不进行分析处理，缺省对完全相同的文件会使用比较功能"),
  N_("    仅在“差异统计”时有效，代码行统计(count-only)时所有文件都要处理"),
  "",
  N_("  --for-program-reading"),
  N_("    改变输出方式，以格式化文本形式输出，便于其他程序读取结果信息"),
  N_("    在由第三方程序调用diffcount，并需要读取统计结果的时候使用"),
  N_("    该选项会屏蔽 --print-lines-info 选项"),
  "",
  N_("  -v  --version  "),
  N_("    输出当前的版本信息"),
  "",
  N_("  --help "),
  N_("    输出缺省帮助信息"),
  "",
  N_("  --cn-help "),
  N_("    输出中文帮助信息"),

  N_("  --langs "),
  N_("	  输出支持的编程语言列表"),
  "",
  N_("关于代码行的说明:"),
  N_("  本程序的有效代码行使用\"非空非注释行\"(NBNC)的概念"),
  N_("  最后进行代码行折算的时候,只使用NBNC行数"),
  "",
  N_("Report bugs to <qianhaiyuan@hikvision.com>."),
  0
};


static char const * const option_help_msgid[] = {
  N_("Compare and diff two packages or only couting one code package"),
  "",
  N_("OPTIONS: "),
  N_(" --git-post-recieve "),
  N_("     git post recieve diff count"),
  N_(" --process "),
  N_("     show the git count process"),
  N_(" --update "),
  N_("     repair git codecount"),
  N_(" -p (used with --git-post-recieve)"),
  N_("     repository's path"),
  N_(" -g (used with--git-post-recieve)"),
  N_("     count git grandpa revision"),
  N_(" -r (used with--git-post-recieve)"),
  N_("     count git revision"),
  N_(" -b (used with--git-post-recieve)"),
  N_("     count git branch"),
  N_(" -n (used with--git-post-recieve)"),
  N_("     count git repo ID"),
  N_(" --end-revision (used with --git-post-recieve)"),
  N_("     end revision ,start with count revision"),
  N_(" -c --count-only  "),
  N_("     Only counting one code package"),
  N_(" --ignore-filename-case    "),
  N_("     Ignore the difference of file name case "),
  N_(" --ignore-white-space    "),
  N_("     Ignore the white SPACE and TAB  changes in code lines "),
  N_(" --print-lines-info "),
  N_("     Print detailed information of every diffed lines "),
  N_("     Slow, Debug use only "),
  N_(" --print-files-info  "),
  N_("     Print every diffed file result information."),
  N_(" --force-parse-all "),
  N_("     Parsing all diffed files(default: Compare same file)"),
  N_(" --for-program-reading"),
  N_("    Change result output style for Third-party program reading "),
  N_("    Will disable --print-lines-info automaticly "),
  N_(" -v  --version "),
  N_("     Output current version."),
  N_(" --help "),
  N_("     Show this page"),
  N_(" --cn-help "),
  N_("    Show Chinese help page"),
  N_(" --langs "),
  N_("	  Show supported languages list"),
   "",
  N_("Report bugs to <qianhaiyuan@hikvision.com>."),
  0
};




static void usage (void)
{
  char const * const *p;
  printf (_("\nUsage: %s [OPTION]... (Baseline) Target\n\n"), program_name);
  for (p = option_help_msgid;  *p;  p++)
    {
      if (!**p)
	putchar ('\n');
      else
	{
	  char const *msg = _(*p);
	  char const *nl;
	  while ((nl = strchr (msg, '\n')))
	    {
	      int msglen = nl + 1 - msg;
	      printf ("  %.*s", msglen, msg);
	      msg = nl + 1;
	    }

	  printf ("  %s\n" + 2 * (*msg != ' ' && *msg != '-'), msg);
	}
    }
}



static void usage_cn (void)
{
  char const * const *p;
  printf (_("\n使用方法: %s [参数选项]... (基线代码包/文件) 目标代码包/文件\n\n"), program_name);
  for (p = option_help_msgid_cn;  *p;  p++)
    {
      if (!**p)
	putchar ('\n');
      else
	{
	  char const *msg = _(*p);
	  char const *nl;
	  while ((nl = strchr (msg, '\n')))
	    {
	      int msglen = nl + 1 - msg;
	      printf ("  %.*s", msglen, msg);
	      msg = nl + 1;
	    }

	  printf ("  %s\n" + 2 * (*msg != ' ' && *msg != '-'), msg);
	}
    }
}




static void print_langs_list()
{
	int it_language = 0;
	printf("Diffcount support languages list below:\n");
	printf("LANG\tLCMT\tBCMT1S\tBCMT1E\tBCMT2S\tBCMT2E\tSTRSB\tEXTS\t\tRATE\n");
	while ( languages[it_language].id != -1)
	{
		printf("%s",languages[it_language].language_name);
		printf("\t%s",languages[it_language].line_comment);
		printf("\t%s",languages[it_language].block_comment_on);
		printf("\t%s",languages[it_language].block_comment_off);
		printf("\t%s",languages[it_language].block_comment_on_alt);
		printf("\t%s",languages[it_language].block_comment_off_alt);
		printf("\t%s",languages[it_language].string_symbol);
		printf("\t%s",languages[it_language].file_extensions);
		printf("\t\t%.2f\n",languages[it_language].standard_c_rate);
		it_language ++;
	}
}






/*  Output the final counting result in console (command line mode)  */
static void print_diffcount_result(int cmd_counting_only, char * left ,char * right,
								      struct diffcount_result * result,
								      int lang_number, float total_standard_c_nbnc_lines)
{
	int i;

	if (cmd_counting_only)
	{
		if (!for_program_reading)
		{
			printf("\nCounting package [%s] result:\n\n",right);
			printf("LANG\tTOTAL\tBLK\tCMT\tNBNC\tRATE\n");
			printf("-----------------------------------------------------------------------\n");
		}
		else if (print_files_info && !for_program_reading)
		{
			printf("$$$$$$$\n"); /* 如果程序还需要读取详细清单，7个$用作详单和结果的分割符*/
		}

		for (i=0; i<lang_number; i++)
		{
			printf("%s",result[i].language_name);
			printf("\t%d",result[i].total_added_lines);
			printf("\t%d",result[i].changed_blank_lines);
			printf("\t%d",result[i].changed_comment_lines);
			printf("\t%d",result[i].changed_nbnc_lines);
			printf("\t%.2f\n",result[i].standard_c_rate);
		}
	}
	else
	{
		if (git_post_commit_counting)
		{
			printf("LANG\tADD\tMOD\tDEL\tA&M\tBLK\tCMT\tNBNC\n");
			printf("-------------------------------------------------------------\n");
		}
		else if (!for_program_reading)
		{
			printf("\nDiffcount [%s] and [%s] result:\n\n",left,right);
			printf("LANG\tADD\tMOD\tDEL\tA&M\tBLK\tCMT\tNBNC\tRATE\n");
			printf("-----------------------------------------------------------------------\n");
		}
		else if(print_files_info && !for_program_reading)
		{
			printf("$$$$$$$\n"); /* 如果程序还需要读取详细清单，7个$用作详单和结果的分割符*/
		}
		for (i=0; i<lang_number; i++)
		{
			printf("%s",result[i].language_name);
			printf("\t%d",result[i].total_added_lines);
			printf("\t%d",result[i].total_modified_lines);
			printf("\t%d",result[i].total_deleted_lines);
			printf("\t%d" ,result[i].total_added_lines + result[i].total_modified_lines);
			printf("\t%d",result[i].changed_blank_lines);
			printf("\t%d",result[i].changed_comment_lines);
			printf("\t%d",result[i].changed_nbnc_lines);
			if(!git_post_commit_counting)
				printf("\t%.2f\n",result[i].standard_c_rate);
			else
				printf("\n");

		}
	}


	if(git_post_commit_counting)
	{
		printf("-------------------------------------------------------------\n");
	}
	else if (!for_program_reading)
	{
		printf("-----------------------------------------------------------------------\n");
		printf("  Convert all NBNC lines to standard C \n");
		printf("      Total: %.2f  (standard C lines)\n\n",total_standard_c_nbnc_lines);
	}


}


bool connect_mysql(MYSQL *p_mysql_conn,mysql_connection *connect_info,bool b_init)
{
	if(b_init)
	{
		if(mysql_init(p_mysql_conn) == NULL)
		{
			printf("mysql init fail\n");
			return false;
		}
	}
	int  timeout_  =  8;
	mysql_options(p_mysql_conn,MYSQL_OPT_CONNECT_TIMEOUT,(const char *)&timeout_);
	mysql_options(p_mysql_conn,MYSQL_OPT_READ_TIMEOUT,(const char *)&timeout_);
	mysql_options(p_mysql_conn,MYSQL_OPT_WRITE_TIMEOUT,(const char *)&timeout_);
	if(mysql_real_connect(p_mysql_conn, connect_info->host, connect_info->user, connect_info->passwd,"svnadmin", connect_info->port, NULL, 0) == NULL)
	{
		SVN_COUNT_DEBUG( "mysql_conn链接错误 : %s\n",mysql_error(p_mysql_conn));
		return false;
	}
	mysql_query(p_mysql_conn, "set names utf8");
	return true;
}

bool getdata_mysql(MYSQL *p_mysql_conn,char * execsql,char *getstr,int getlen)
{
	MYSQL_RES *result;
	MYSQL_ROW row;
	int n_count = 5;
start:
	if (mysql_query(p_mysql_conn, execsql)!=0)
	{
		char *Err = (char *)mysql_error(p_mysql_conn);
		if(strstr(Err,"MySQL server has gone away") && n_count>0)
		{
			mysql_close(p_mysql_conn);
			//初始化数据库链接
			n_count -- ;
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

	if(!(result=mysql_use_result(p_mysql_conn)))
	{
		SVN_COUNT_DEBUG( " select error no result \n","");
		return false;
	}

	if(row=mysql_fetch_row(result))
	{
		//SVN_MULTISITE_DEBUG( " sqlsel: %s\n",row[0]);
		if(!row[0])
		{
			goto err_end_fun;
		}
		if(strlen(row[0]) > getlen || strlen(row[0]) == 0)
		{
			goto err_end_fun;
		}

		strncpy(getstr,row[0],getlen);
	}
	else
		goto err_end_fun;

	mysql_free_result(result);
	return true;

err_end_fun:
	mysql_free_result(result);
	return false;

}


static int get_line(char **buffer,char **line_str)
{
	char *lpstop = NULL;

	if(*buffer == NULL)
		return COUNT_ERR;
	lpstop = strstr(*buffer,"\n");
	if(NULL != lpstop )
	{
		*line_str = *buffer;
		*buffer = lpstop+1;
		*lpstop = '\0';
	}
	else
	{
		*line_str = *buffer;
		*buffer = NULL;
	}
	return COUNT_OK;
}
static int get_exec_popen_str(char **buffer,char *execsql)
{
	FILE* fp = NULL;
	if(NULL==(fp=popen(execsql,"r")))
    {
        pclose(fp);
		return COUNT_ERR;
    }
	*buffer = (char *)malloc(4096);
	memset(*buffer,0,4096);
	int bufsize = 4096;
	int len = 0;

	while(1)
	{
		if(!fgets(*buffer+len, bufsize-len , fp))
			break;
		len = strlen(*buffer);
		if(len >= bufsize-1)
		{
			bufsize *= 2;
			*buffer = (char *)realloc(*buffer,bufsize);
			//printf("bufsize:%d",bufsize);
		}
	}
	pclose(fp);
	return COUNT_OK;
}

static int write_file_str(char *filepath,char *out_str)
{
	FILE *fd = NULL;
	if (!(fd = fopen( filepath, "w+")))
	{
		return 0;
	}
	fwrite(out_str ,strlen(out_str),1,fd );
	fclose(fd);
	return 1;
}

static int
svn_count_do_get_command(char * execsql,char * buf,int buf_length)
{
	FILE* fp = NULL;

	SVN_COUNT_DEBUG("svn command :\n %s \n",execsql);
	if(NULL==(fp=popen(execsql,"r")))
	{
		pclose(fp);
		SVN_COUNT_DEBUG( "Error: popen: %s\n ",   strerror(errno));
		return COUNT_ERR;
	}
	int n = 0;
	int len = 0;
	memset(buf,0,buf_length);
	while(n<20 && 4000-len>0 &&  fgets(buf+len, 4000-len , fp))
	{
		len = strlen(buf);
		n++;
	}
	pclose(fp);
	fp = NULL;
	SVN_COUNT_DEBUG("svn command result(len:%d):\n %s \n", strlen(buf),buf);
	return COUNT_OK;
}


static void rand_str(int in_len,char *OutStr)
{
	char Str[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	int len = strlen(Str);
	memset(OutStr,0,strlen(OutStr));
	srand((unsigned int)time(NULL) + rand());    //初始化随机数的种子
	int i = 0;
	for (; i  < in_len; i++)
	{
		OutStr[i] = Str[rand()%len];
	}
	OutStr[i] = '\0';
}

int svn_post_commit_count(char *repos_path,char * revision, char * parent_commitid)
{
	char *left_path = (char*)malloc(256);
	char *right_path= (char*)malloc(256);
	char *replace_partent= (char*)malloc(256);
	struct diffcount_result *result;
	long time = getCurrentTime();
	int exit_status = EXIT_SUCCESS;
	int lang_number = 0;
	float total_standard_c_nbnc_lines = 0;
	memset(left_path ,0,256);
	memset(right_path,0,256);
	memset(replace_partent,0,256);
	char execsql[4096] = {0};
	char error_commitid[41] = "0000000000000000000000000000000000000000";
	/*char *parent_commitid= (char*)malloc(82);
	memset(parent_commitid,0,82);*/
	char rev_codecount[16] = {0};
	int len = 0;
	char *changed_buffer;
	int file_mod[4] = {0,0,0,0};

	svn_info.revision = revision;
    svn_info.grandpa_revision = parent_commitid;
	char *author = NULL;
	char *git_info = NULL;

	char *date = NULL;

	bool h_only = true;
	SVN_COUNT_DEBUG("repos_path : %s \n",repos_path);

	/* 设置统计开始标记位 */
	sprintf(execsql,"insert into git_code_count_process(server_id,repos_id,revision,update_time,grandpa_revision) values(%s,%s,\"%s\",now(),\"%s\");",svn_info.server_id,svn_info.repos_id,revision,svn_info.grandpa_revision);
	SVN_COUNT_DEBUG("execsql : %s \n",execsql);
	svn_count_mysql_query(&mysql_conn,execsql);

	memset(execsql,0,4096);

	/* 获取版本author和修改时间*/

	sprintf(execsql,"/usr/bin/git log --pretty=format:\"%%an\" \"%s\" -1",revision);
	SVN_COUNT_DEBUG("execsql : %s \n",execsql);

	if(get_exec_popen_str(&author,execsql) == COUNT_ERR )
	{
		printf("get author error\n");
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}

	if(strlen(author) == 0)
	{
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}

	sprintf(execsql,"/usr/bin/git log --pretty=format:\"%%ci\" \"%s\" -1 | cut -b 1-20",revision);
	SVN_COUNT_DEBUG("execsql : %s \n",execsql);

	if(get_exec_popen_str(&(svn_info.date),execsql) == COUNT_ERR )
	{
		printf("get commit-date error\n");
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}

	if(strlen((svn_info.date)) == 0)
	{
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}

    sprintf(execsql,"/usr/bin/git log --pretty=format:\"%%s\" \"%s\" -1",revision);
	SVN_COUNT_DEBUG("execsql : %s \n",execsql);

	if(get_exec_popen_str(&(svn_info.log),execsql) == COUNT_ERR )
	{
		printf("get commit-log error\n");
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}

	if(strlen((svn_info.log)) == 0)
	{
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}


	SVN_COUNT_DEBUG("author : %s   date: %s \n",author,svn_info.date );


	//查询用户ID
	sprintf(execsql,"select user_id from svn_user where user_name=\"%s\" or full_name=\"%s\"",author,author);
	SVN_COUNT_DEBUG("execsql : %s \n",execsql);

	if(!getdata_mysql(&mysql_conn,execsql,svn_info.user_id,11))
	{
		free(left_path);
		free(right_path);
		free(replace_partent);
		SVN_COUNT_DEBUG("get user_id error : %s \n",svn_info.user_id);
		return COUNT_OK;
	}
	SVN_COUNT_DEBUG("user_id : %s \n",svn_info.user_id);

	//获取当前版本文件修改列表
	//1.获取当前提交的父提交
	/*sprintf(execsql,"/usr/bin/git log --pretty=format:\"%P\" \"%s\" -1",revision);

	SVN_COUNT_DEBUG("execsql : %s \n",execsql);
    if(get_exec_popen_str(&parent_commitid,execsql) == COUNT_ERR )
	{
		printf("get parent-commit error\n");
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}

	if(strlen(parent_commitid) == 0)
	{
		free(left_path);
		free(right_path);
		free(replace_partent);
		return COUNT_ERR;
	}*/
    if(strcmp(parent_commitid, error_commitid)!=0){
        changed_buffer = NULL;
        sprintf(execsql,"/usr/bin/git diff --name-status '%s' %s",parent_commitid,revision);
        SVN_COUNT_DEBUG("execsql : %s \n",execsql);

	if(get_exec_popen_str(&changed_buffer,execsql) == COUNT_ERR || NULL == changed_buffer)
	{
		free(left_path);
		free(right_path);
		free(replace_partent);
		printf("something wrong!!\n");
		return COUNT_ERR;
	}
    }
	else{
        char *test_buffer = NULL;
        sprintf(execsql,"/usr/bin/git show --name-status '%s'",revision);
        SVN_COUNT_DEBUG("execsql : %s \n",execsql);

	if(get_exec_popen_str(&test_buffer,execsql) == COUNT_ERR || NULL == test_buffer)
	{
		free(left_path);
		free(right_path);
		free(replace_partent);
		printf("something wrong!!\n");
		return COUNT_ERR;
	}
		char *achanged_buffer = strstr(test_buffer, "\n\nA");
        changed_buffer = strstr(achanged_buffer, "A");

		SVN_COUNT_DEBUG("changed_buffer:%s", changed_buffer);
	}

	if(*changed_buffer == 'A')
		file_mod[0]++;
	else if (*changed_buffer == 'D')
		file_mod[1]++;
	else
		file_mod[2]++;
	char * lpfind = strstr(changed_buffer,"\nA");
	while(lpfind != NULL)
	{
		file_mod[0]++;
		lpfind = strstr(lpfind+2,"\nA");
	}
	lpfind = strstr(changed_buffer,"\nC");
	while(lpfind != NULL)
	{
		file_mod[0]++;
		lpfind = strstr(lpfind+2,"\nC");
	}

	lpfind = strstr(changed_buffer,"\nU");
	while(lpfind != NULL)
	{
		file_mod[2]++;
		lpfind = strstr(lpfind+2,"\nU");
	}

	lpfind = strstr(changed_buffer,"\nR");
	while(lpfind != NULL)
	{
		file_mod[2]++;
		lpfind = strstr(lpfind+2,"\nR");
	}

	lpfind = strstr(changed_buffer,"\nD");
	while(lpfind != NULL)
	{
		file_mod[1]++;
		lpfind = strstr(lpfind+2,"\nD");
	}

	SVN_COUNT_DEBUG("file_mod count: add(%d) del(%d) chg(%d) \n",file_mod[0],file_mod[1],file_mod[2]);
	// 二、使用svnlook history 和cat 获取父文件和当前文件的文件内容实体
	char *lprightstr = changed_buffer;
	char *lplinestr = NULL;
	if(file_mod[0] >= 150 && file_mod[2] == 0)
	{
		//如果导入的文件大于150 个每次，直接过滤本次提交，删除统计数据
		SVN_COUNT_DEBUG("audit import that file add more than 150 files: counts(%d) \n",file_mod[0]);

		strcpy(execsql,"insert into git_audit_code_import(repos_id,user_id,revesion,counts,time,repos_name,server_ip,log_message) value (");
		sprintf(execsql+strlen(execsql),"%s,%s,'%s',%d,'%s','%s','%s','",svn_info.repos_id,svn_info.user_id,revision,file_mod[0],svn_info.date,svn_info.repos_name,svn_info.server_ip);

		if(strlen(svn_info.log) >0)
		{
			char *lpexecsql = execsql+strlen(execsql);
			int n = 254;
			while(*(svn_info.log) && n > 0)
			{
				if(*(svn_info.log) == '\'')
				{
					*lpexecsql = '\\';
					lpexecsql++;
				}

				*lpexecsql = *(svn_info.log);
				svn_info.log++;
				lpexecsql++;
				n--;
			}
			if(*(lpexecsql-1) == '\n')
			{
				*(lpexecsql-1) = '\'';
				*(lpexecsql) = ')';
				*(lpexecsql+1) = '\0';
			}
			else
			{
				*(lpexecsql) = '\'';
				*(lpexecsql+1) = ')';
				*(lpexecsql+2) = '\0';
			}
		}
		else
		{
			strcat(execsql,"')");
		}


		svn_count_mysql_query(&mysql_conn,execsql);

		goto end_fun;
	}
	file_mod[0] = 0;
	file_mod[1] = 0;
	file_mod[2] = 0;

	while(get_line(&lprightstr,&lplinestr) == COUNT_OK)
	{
		int change_type = 0;

		if(NULL == lplinestr )
		{
			break;
		}

		SVN_COUNT_DEBUG("lplinestr : %s \n",lplinestr);

		/*if(strlen(lplinestr) <= 8 || strlen(lplinestr) > 512)
		{
			continue;
		}*/
		svn_info.svn_url_path = lplinestr+2;

		SVN_COUNT_DEBUG("svn_url_path : %s \n",svn_info.svn_url_path);

		/* 文件状态识别，主要分为ADU，但是部分merge的会显示_U等，故需要删除前面的_*/
		while(*lplinestr == '_')
			lplinestr++;

		switch(*lplinestr)
		{
			case 'A':
				{
					//检查文件是否为Replace ，这种Replace 是由目录的Replace 引起
					if(NULL != replace_partent  && strlen(replace_partent)>0 && strncmp(svn_info.svn_url_path,replace_partent,strlen(replace_partent)) ==0)
					{
						change_type = REPLACE;
						break;
					}
					change_type = ADD;
					break;
				}
			case 'U':
				change_type = MOD;
				break;
            case 'M':
                change_type = MOD;
				break;
			case 'D':
				{
					//检查文件是否为Replace

					if(NULL != lprightstr)
					{
						char *str = lprightstr;
						char *str_new_line = NULL;

						if(get_line(&str,&str_new_line) == COUNT_OK)
						{

							if( strlen(str_new_line) <= 8 || strlen(str_new_line) > 512)
							{
								lprightstr = str;
								change_type = DEL;
							}
							else
							{
								SVN_COUNT_DEBUG("Replace check : %s \n",str_new_line);
								//失败的话，需要把刚刚取行删除的回车加回去
								//成功的话，需要把指针slprightstr 指到新的行
								char *cmp_str = str_new_line+4;

								while(*str_new_line == '_')
									str_new_line++;
								if(*str_new_line == 'A')
								{
									if(strcmp(svn_info.svn_url_path,cmp_str) == 0)
									{
										SVN_COUNT_DEBUG("Find Replace \n","");
										lprightstr = str;
										change_type = REPLACE;
										memset(replace_partent,0,256);
										strncpy(replace_partent,svn_info.svn_url_path,256);
									}
								}
								if(NULL != str && change_type == 0)
								{
									*(str-1) = '\n';
								}
							}
						}
					}
					if(change_type == 0)
						change_type = DEL;
					break;
				}
			default:

				break;
		}

		//如果是文件夹，直接返回
		if(*(lplinestr+strlen(lplinestr)) == '/')
		{
			//printf("not a file \n");
			continue;
		}

		/* 语言识别*/
		/* 如果没有识别到语言，检查一些常见的全文件名匹配 */
		if(!(current_language = get_current_language(lplinestr)))
		{
			//printf("not a language file \n");
			if(!(current_language = get_current_language_with_filename(lplinestr)))
			{
				continue;
			}

			SVN_COUNT_DEBUG("filename ext patten : %s \n",current_language->default_extensions);
		}

		char str_rand[9] = {0};
		rand_str(8,str_rand);
		sprintf(right_path,"/tmp/new_%s_%s_%s%s",str_rand,svn_info.repos_id,revision,current_language->default_extensions);
		SVN_COUNT_DEBUG("rightpack : %s \n",right_path);
		sprintf(left_path,"/tmp/old_%s_%s_%s%s",str_rand,svn_info.repos_id,revision,current_language->default_extensions);

		if(change_type == ADD || change_type == MOD || change_type == REPLACE)
		{
			sprintf(execsql,"/usr/bin/git show '%s':'%s' > \"%s\"",revision,svn_info.svn_url_path,right_path);
			SVN_COUNT_DEBUG("execsql : %s \n",execsql);
			system(execsql);
		}

		svn_info.commit_level = CODE_COUNT_NORMAL;

		if(change_type == ADD )
		{
			file_mod[0]++;
			/* 文件添加，直接统计*/
			exit_status = compare_files((struct comparison *) 0,"", right_path);
			remove(right_path);
		}
		else if(change_type == DEL)
		{
			file_mod[1]++;
			char *buffer_lines = NULL;
			char revision_del[12] = {0};
			/* 删除的文件没法直接获取文件流，直接从上一版本中获取，该方法存在缺陷
			如果文件是分支过来且删除的，会导致文件流获取失败
			但是TortoiseSVN中也存在相同bug，故不做修复，出现概率低，且对实际影响很小*/
			sprintf(execsql,"/usr/bin/git show '%s':'%s' | wc -l",parent_commitid,svn_info.svn_url_path);
			if(get_exec_popen_str(&buffer_lines,execsql) == COUNT_ERR)
			{
				//发生错误
				continue;
			}
			SVN_COUNT_DEBUG("buffer_lines : %s \n",buffer_lines);
			if(atoi(buffer_lines) == 0)
			{
				free(buffer_lines);
				continue;
			}
			/* 将查询到的行数直接插入到current_language->total_deleted_lines
			并在前台打印文件详细信息*/
			current_language->total_deleted_lines += atoi(buffer_lines);
			if(print_files_info)
			{
				printf("%s",current_language->language_name);
				printf("\t%d",0);
				printf("\t%d",0);
				printf("\t%d",atoi(buffer_lines));
				printf("\t%d" ,0 );
				printf("\t%d",0);
				printf("\t%d",0);
				printf("\t%d",0);
				printf("\t%s","DEL");
				printf("\t%s",svn_info.svn_url_path);
				printf("\t%s\n","");
			}
			insert_code_count(svn_info.repos_id,svn_info.revision,svn_info.svn_url_path,svn_info.user_id,svn_info.date,current_language->language_name,"DEL",0,0,atoi(buffer_lines),0,0,0);
			free(buffer_lines);
			buffer_lines = NULL;
		}
		else if ( change_type == MOD )
		{
			file_mod[2]++;
			char *buffer_history = NULL;
			char *repos_path_old = NULL;
			char *revision_old = NULL;

			//查询上一版本路径和版本号


			//获取上一版本文件流

			sprintf(execsql,"/usr/bin/git show '%s':'%s' >'%s'",parent_commitid,svn_info.svn_url_path,left_path);
			SVN_COUNT_DEBUG("execsql : %s  \n",execsql);
			system(execsql);

			//对比新老文件
			exit_status = diffcount_compare_packages (left_path,right_path);

			remove(right_path);
			remove(left_path);
			free(buffer_history);

		}
		else if(change_type = REPLACE)
		{
			file_mod[3]++;
			svn_info.commit_level = CODE_COUNT_REPLACE;
			//获取上一版本文件流
			//SVN_COUNT_DEBUG("Repalce file \n","");
			sprintf(execsql,"/usr/bin/git show '%s':'%s' >'%s'",parent_commitid,svn_info.svn_url_path,left_path);
			SVN_COUNT_DEBUG("execsql: %s \n",execsql);

			int ret = system(execsql);
			//对比新老文件
			SVN_COUNT_DEBUG("system return: %d \n",ret);
			//在SVN程序没有输出时调用，可能出现返回结果为-1的情况，但程序是正常运行的
			//没能找出具体出错的原因
			if(ret==-1)
			{
				ret = access(left_path,F_OK);
			}

			if(ret==0)
			{
				file_mod[2]++;
				exit_status = diffcount_compare_packages (left_path,right_path);
			}
			else
			{
				file_mod[0]++;
				exit_status = compare_files((struct comparison *) 0,"", right_path);
			}

			remove(right_path);
			remove(left_path);
		}

	}

	memset(execsql,0,4096);
	if(file_mod[0] >= 150 && file_mod[2] == 0)
	{
		//如果导入的文件大于150 个每次，直接过滤本次提交，删除统计数据
		strcpy(execsql,"delete from git_code_count where repos_id=");
		strcat(execsql,svn_info.repos_id);
		strcat(execsql," and server_id=");
		strcat(execsql,svn_info.server_id);
		strcat(execsql," and revision='");
		strcat(execsql,svn_info.revision);
		strcat(execsql,"'");
		svn_count_mysql_query(&mysql_conn,execsql);

		SVN_COUNT_DEBUG("audit import that file add more than 150 files: counts(%d) \n",file_mod[0]);

		strcpy(execsql,"insert into git_audit_code_import(server_id,repos_id,user_id,revesion,counts,time,repos_name,server_ip,log_message) value (");
		sprintf(execsql+strlen(execsql),"%s,%s,%s, '%s', %d,'%s','%s','%s',",svn_info.server_id,svn_info.repos_id,svn_info.user_id,revision,file_mod[0],svn_info.date,svn_info.repos_name,svn_info.server_ip);

		if(strlen(svn_info.log) >0)
		{
			char *lpexecsql = execsql+strlen(execsql);
			int n = 254;
			while(*(svn_info.log) && n > 0)
			{
				if(*(svn_info.log) == '\'')
				{
					*lpexecsql = '\\';
					lpexecsql++;
				}

				*lpexecsql = *(svn_info.log);
				svn_info.log++;
				lpexecsql++;
				n--;
			}
			if(*(lpexecsql-1) == '\n')
			{
				*(lpexecsql-1) = '\'';
				*(lpexecsql) = ')';
				*(lpexecsql+1) = '\0';
			}
			else
			{
				*(lpexecsql) = '\'';
				*(lpexecsql+1) = ')';
				*(lpexecsql+2) = '\0';
			}
		}
		else
		{
			strcat(execsql,"')");
		}

		svn_count_mysql_query(&mysql_conn,execsql);

	}
	else
	{
		if(file_mod[0] >= 50 && file_mod[2] <= 5 )
		{
			sprintf(execsql,"update git_code_count set commit_level=\"%s\" ",(file_mod[0] >= 150)?(CODE_COUNT_IMPORT_II):(CODE_COUNT_IMPORT_I));
		}
		else if (file_mod[0] >= 150)
		{
			sprintf(execsql,"update git_code_count set commit_level=\"%s\" ",CODE_COUNT_IMPORT_I);
		}
		else
        {
            sprintf(execsql,"select sum(ROUND(changed_nbnc_lines+deleted_lines*0.05)) as total_nbnc_code \
					from git_code_count where repos_id=%s and revision=\"%s\" group by repos_id,revision",svn_info.repos_id,svn_info.revision);
            SVN_COUNT_DEBUG("execsql: %s \n",execsql);

			if(strlen(execsql )>0 )
			{
				getdata_mysql(&mysql_conn,execsql,rev_codecount,16);
				memset(execsql,0,4096);
				SVN_COUNT_DEBUG("rev_codecount: %s \n",rev_codecount);
				if(atoi(rev_codecount)> 10000)
				{
					sprintf(execsql,"update git_code_count set commit_level=\"%s\" ",CODE_COUNT_WARNING);
					SVN_COUNT_DEBUG("revision's codecount is more than 10000, make commit level to %s \n",CODE_COUNT_WARNING);
				}
			}
        }
		if(strlen(execsql) > 0)
		{
			strcat(execsql," where repos_id=");
			strcat(execsql,svn_info.repos_id);
			strcat(execsql," and server_id=");
			strcat(execsql,svn_info.server_id);
			strcat(execsql," and revision='");
			strcat(execsql,svn_info.revision);
			strcat(execsql,"'");
			SVN_COUNT_DEBUG("execsql: %s \n",execsql);
			svn_count_mysql_query(&mysql_conn,execsql);
		}



	}
end_fun:
	/* 设置统计开始标记位 */
	memset(execsql,0,4096);
	sprintf(execsql,"delete from git_code_count_process where server_id=%s and repos_id=%s and revision = \"%s\";",svn_info.server_id,svn_info.repos_id,revision);
	SVN_COUNT_DEBUG("execsql : %s \n",execsql);
	svn_count_mysql_query(&mysql_conn,execsql);

	//如果存在项目编号，则修改提交记录并标记项目编号，同理代码重用率使用系统方法计算，默认代码重用率为0%
	/*if (strlen(project) > 0 && strlen (project) <= 256 && !strstr(project,"\"") && !strstr(project,"'"))
	{
		SVN_COUNT_DEBUG("project : %s \n",project);
		//查找是否有多个项目，如果有，逐个叠加
		memset(execsql,0,4096);
		strcpy(execsql,"insert into code_count_project(repos_id,revision,project_number) values");
		char * sign = project;
		char * signend = NULL;
		while(sign)
		{
			while(*sign == ';')
				sign++;

			if(sign)
			{
				signend = strstr(sign,";");
				strcat(execsql,"(");
				strcat(execsql,svn_info.repos_id);
				strcat(execsql,",");
				strcat(execsql,svn_info.revision);
				strcat(execsql,",\"");
			}
			else
			{
				int len = strlen(execsql);
				if(execsql[len-1] == ',')
					execsql[len-1] = ';';
				else
					memset(execsql,0,4096);
				break;
			}

			if(!signend)
			{
				strcat(execsql,sign);
				strcat(execsql,"\");");
				break;
			}
			else
			{
				strncat(execsql,sign,signend-sign);
				strcat(execsql,"\"),");
			}

			sign = signend ;
		}

		if(strlen(execsql) > 0)
			svn_count_mysql_query(&mysql_conn,execsql);
	}*/

	for_program_reading = true;
	diffcount_establish_result(&result,&lang_number,&total_standard_c_nbnc_lines);
	print_diffcount_result(false,left_path, right_path,
					 result,lang_number,total_standard_c_nbnc_lines);

	/*for (int i=0; i<lang_number; i++)
	{
		//查找文件修改状态，如果全部是.h修改，自动忽略
		printf("%s",result[i].language_name);
		printf("\t%d",result[i].total_added_lines);
		printf("\t%d",result[i].changed_blank_lines);
		printf("\t%d",result[i].changed_comment_lines);
		printf("\t%d",result[i].changed_nbnc_lines);
		printf("\t%.2f\n",result[i].standard_c_rate);
		if(result[i].default_extensions)
		{
			h_only = false;
			break;
		}
	}*/

	free(result);
	free(left_path);
	free(right_path);
	//free(changed_buffer);
	free(replace_partent);
	time = getCurrentTime() - time;
	printf("time:%ld\n" ,time);
	return COUNT_OK;
}


int main (int argc, char **argv)
{
  int c;
  int prev = -1;
  bool cmd_counting_only = false;

  char * leftpack;
  char * rightpack;
  char repos_path[256] = {0};
  char revision[41] = {0};
  char repo_id[12] = {0};
  char start_revision[41] = {0};
  char end_revision[41] = {0};
  char branch[256] = {0};
  char parent_commitid[41] = {0};
  memset(server_ip,0,32);
  int exit_status;

  struct diffcount_result *result;

  int lang_number = 0;
  float total_standard_c_nbnc_lines = 0;

  /*init diffcount */
  diffcount_init();

  /* Program name ---> 'diffcount' */
  program_name = argv[0];
  git_post_commit_counting = false;
  git_count_debug_on = false;
  git_count_process = false;
  git_count_update = false;

  /* Decode the options.  */
int err = 0;
  while ((c = getopt_long (argc, argv, shortopts, longopts, 0)) != -1)
  {

    switch (c)
	{
		case 0:
	 	 break;

		 case 'v':
		  	  printf ("\n\tDiffcount Utility  %s\n%s\n",
			  version_string, copyright_string);
			  check_stdout ();
		  return EXIT_SUCCESS;

		  case 'c':
		  	  trace2("Get Option: counting_only \n");
		  	  cmd_counting_only = true;
		  	  break;
		  case 'd':
			  git_count_debug_on = true;
			  break;

          case 'p':
		  	if(optarg && strlen(optarg) <256 && strlen(optarg) > 2)
	        {
	            strcpy(repos_path,optarg);
                chdir(repos_path);
	        }
			else
				  err = 1;
		  	  trace2("Get Option: repos path \n");
		  	  break;
          case 'n':
            if(optarg && strlen(optarg) <256)
	        {
	            strcpy(repo_id,optarg);
	        }
			else
				  err = 1;
		  	  trace2("Get Option: repos id \n");
		  	  break;
          case 'b':
              if(optarg && strlen(optarg) <256 && strlen(optarg) > 0)
	        {
	            strcpy(branch,optarg);
	        }
	        else
                    err = 1;
                trace2("Get Option: branch \n");
		  	    break;
          case 'g':
            if(optarg && strlen(optarg) <41 && strlen(optarg) >= 40)
                strcpy(parent_commitid, optarg);
            else
                err = 1;
                trace2("Get Option: grandpa revision");
                break;
          case 'r':
		  	if(optarg && strlen(optarg) <41 && strlen(optarg) >= 40)
		  	{
		  		strcpy(start_revision, optarg);
	            strcpy(revision,optarg);
		  	}
			else
                  err = 1;
		  	  trace2("Get Option: revision \n");
		  	  break;
		 case IGNORE_CASE_OPTION:
		 	  trace2("Get Option: Ignore file name case \n");
		 	  diffcount_set_ignore_case(true);
			  break;
		 case GIT_END_REVISION_OPTION:
		 	if(optarg && strlen(optarg) <41 && strlen(optarg) >= 40)
  				  strcpy(end_revision, optarg);
			else
				  err = 1;
		  	  trace2("Get Option: end_revision \n");
		  	  break;
		 	break;

   	     case PRINT_LINES_INFO_OPTION:
   	     	  trace2("Get Option: Print Lines detailed information \n");
			  if (!for_program_reading)
   	     	  	print_lines_info = true;
   	     	  break;
		 case GIT_COUNT_PROCESS:
		 	  git_count_process = true;
		 	  break;

		 case GIT_POST_RECIEVE_OPTION:
		 	  git_post_commit_counting = true;
			  diffcount_set_ignore_white_space(true);
			  break;

		 case GIT_COUNT_UPDATE:
		 	  git_count_update = true;
			  break;

   	     case PRINT_FILES_INFO_OPTION:
   	     	  trace2("Get Option: Print Files detailed information \n");
   	     	  	print_files_info = true;
   	     	  break;

   	     case FORCE_PARSE_ALL_OPTION:
   	     	  trace2("Get Option: Force parsing all files \n");
   	     	  diffcount_set_parse_all(false);
   	     	  break;

		 case FOR_PROGRAM_READING_OPTION:
			  trace2("Get Option: For program reading \n");
			  diffcount_set_program_reading(true);
			  break;

		 case IGNORE_WHITE_SPACE_OPTION:
			  trace2("Get Option: ignore white spaces \n");
			  diffcount_set_ignore_white_space(true);
			  break;

		 case HELP_OPTION:
			  usage();
			  check_stdout ();
			  return EXIT_SUCCESS;

		 case HELP_OPTION_CN:
			  usage_cn ();
			  check_stdout ();
			  return EXIT_SUCCESS;

		  case LANGS_OPTION:
			  print_langs_list();
			  check_stdout ();
			  return EXIT_SUCCESS;

		default:
		  try_help (0, 0);
	}
      prev = c;
    }

	/*printf("svn_post_commit_counting:%s \n",svn_post_commit_counting?"true":"false");
	printf("repos_path:%s \n",repos_path);
	printf("revision:%s \n",revision);*/

	if(err ==1)
	{
		usage();//try_help ("DIFFCOUNT: missing operand after `%s'", argv[argc - 1]);
		exit(0);
	}

  if(git_post_commit_counting)
	{
		  if(strlen(repos_path) == 0 || strlen(revision) == 0)
		  {
			  usage();
			  exit(0);
		  }

	}
  else if (cmd_counting_only) /* counting_only : only one filename parameter */
  {
  	if (argc - optind != 1)
  	{
		  if (argc - optind < 1)
			try_help ("DIFFCOUNT: missing operand after `%s'", argv[argc - 1]);
	  	else
			try_help ("DIFFCOUNT: extra operand `%s'", argv[optind + 1]);
  	}
  }
  else /* Normal Status */
  {
	if (argc - optind != 2)
  	{
		  if (argc - optind < 2)
			try_help ("DIFFCOUNT: missing operand after `%s'", argv[argc - 1]);
	  	else
			try_help ("DIFFCOUNT: extra operand `%s'", argv[optind + 2]);
  	}
  }


  /* Get left file name / right file name */
  if(git_post_commit_counting)
  {
  		if (!git_count_debug_on && !git_count_process && !git_count_update )
  		{
	  		int pid;
			if(pid=fork())
				exit(0);
			else if(pid< 0)
				exit(1);

			setsid();
			chdir("/");
			close(0);
		    close(1);
		    close(2);
			signal(SIGCHLD, SIG_IGN);
  		}

		//git post recieve 差异比较
		char execsql[512]={0};
		char uuid[64] = {0};
		memset(&svn_info,0,sizeof(svn_info_struct));
		svn_info.repos_id = (char *)malloc(12);
		svn_info.user_id = (char *)malloc(12);

		//初始化数据库链接
		if(!connect_mysql(&mysql_conn,&svn_count_connection,true))
		{
			printf("init mysql error\n");
			return COUNT_ERR;
		}

		//memset(server_ip,0,32);
        //获权IP
		svn_count_do_get_command("cat /var/cmcenter/uuid | grep -E -o \"[a-f0-9]{8}-([a-f0-9]{4}-){3}[a-f0-9]{12}\" | tr -d \"\\n\" ",&uuid[0],63);

		if(strlen(uuid) != 36 )
		{
			printf("uuid error: %s  %d",uuid,strlen(uuid));
			return COUNT_ERR;
		}

		sprintf(execsql,"select server_ip from git_server where uuid=\"%s\"",uuid);
		memset(server_ip,0,32);
		getdata_mysql(&mysql_conn,execsql,&server_ip[0],31);
		sprintf(execsql,"select id from git_server where uuid=\"%s\"",uuid);
		memset(server_id,0,11);
		getdata_mysql(&mysql_conn,execsql,&server_id[0],11);
		if(strlen(server_ip) == 0  || strlen(server_id) == 0)
		{
			printf("无法获取服务器IP，程序无法启动!\n");
	        return COUNT_ERR;
		}
		svn_info.server_ip = &server_ip[0];
		svn_info.server_id = &server_id[0];
		if(strlen(branch) == 0 )
        {
            printf("无法获取分支，程序无法启动!\n");
        }
		svn_info.branch = &branch[0];

		svn_info.repos_name = repos_path + strlen(repos_path)-1;
		if(*(svn_info.repos_name) == '/')
			*(svn_info.repos_name) = '\0';
		while(*(svn_info.repos_name) != '/')
			svn_info.repos_name--;
		svn_info.repos_name++;
		SVN_COUNT_DEBUG("reposname : %s \n",svn_info.repos_name);

		//查询版本库ID
		strcpy(svn_info.repos_id, repo_id);

		SVN_COUNT_DEBUG("repos_id : %s \n",svn_info.repos_id);
		if(end_revision == 0)
            printf("");
			svn_post_commit_count(repos_path, revision, parent_commitid);


		free(svn_info.repos_id);
		free(svn_info.user_id);
		exit(0);

  }
  else if (cmd_counting_only)
  {
  	if (print_files_info && (!for_program_reading))
	{
		printf("\nLANG\tTOTAL\tBLK\tCMT\tNBNC\tTARGET FILE\n");
  	}
  	leftpack = NULL;
   	rightpack = argv[optind];
   	exit_status = diffcount_count_package (rightpack);
  }
  else
  {
  	/* Get the left and right to be counted... */

  	if (print_files_info && (!for_program_reading))
	{
		printf("\nLANG\tADD\tMOD\tDEL\tA&M\tBLK\tCMT\tNBNC\tSTATE\tBASELINE FILE\tTARGET FILE\n");
  	}
  	leftpack = argv[optind];
  	rightpack = argv[optind + 1];
  	exit_status = diffcount_compare_packages (leftpack,rightpack);
  }


 	/*--- 处理并输出最后的结果-----*/
  diffcount_establish_result(&result,&lang_number,&total_standard_c_nbnc_lines);
  print_diffcount_result(cmd_counting_only,leftpack, rightpack,
  						 result,lang_number,total_standard_c_nbnc_lines);
  free(result);


  check_stdout ();


  exit (exit_status);
  return exit_status;
}





