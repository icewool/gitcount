/* Shared definitions for DIFFCOUNT

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


#include <stdio.h>
#include "system.h"
#include "libdiffcount.h"
#include <mysql.h>
#include <stdarg.h>

#define ADD 1
#define MOD 2
#define DEL 3
#define REPLACE 4
#define CODE_COUNT_REPLACE "Replace"
#define CODE_COUNT_IMPORT_II "ImportII"
#define CODE_COUNT_IMPORT_I "ImportI"
#define CODE_COUNT_WARNING "Warning"
#define CODE_COUNT_NORMAL "Normal"
#define COUNT_ERR -1
#define COUNT_OK 0
#define SVN_COUNT_LOG_PATH "/var/log/svncount.log"


#define TAB_WIDTH 8

/* What kind of changes a hunk contains.  */
enum changes
{
  /* No changes: lines common to both files.  */
  UNCHANGED,
  /* Deletes only: lines taken from just the first file.  */
  OLD,
  /* Inserts only: lines taken from just the second file.  */
  NEW,
  /* Both deletes and inserts: a hunk containing both old and new lines.  */
  CHANGED
};


/* the comment kind of line type of diffed result */
enum line_comment_type
{
   /*Block Comment Begin eg. { (* .... */
   BlockCommentOn,
   /*Block Comment end eg. } *) ......*/
   BlockCommentOff,
   /*Line Comment .. eg // or the combined blockon and off eg {xxx}..*/
   LineComment,
   /*Normal Code line*/
   NormalLine,
   /* Blank Code line */
   BlankLine,
   /*One code line with a comment line */
   NormalWithComment,
   /* one code line with BlockCommentOn */
   NormalWithBlockOn,
   /*Blockoff with one code */
   BlockOffWithNormal,
};

/*store diffed file information for constructing list */
struct diffed_line_node
{
    enum changes line_status;
    enum line_comment_type line_comment;
    struct diffed_line_node * next;
};



struct language_type
{
	int id;
	bool constructed; /* existence flage in this diff process */
	char * language_name;
	char * line_comment;
	char * block_comment_on;
	char * block_comment_off;
	char * block_comment_on_alt;
	char * block_comment_off_alt;
	char * string_symbol;
	char * string_symbol_alt;
	char * string_escaped;

	char * file_extensions;
	char * file_names;
	char * default_extensions;

	float standard_c_rate;

	unsigned int total_added_lines;
	unsigned int total_deleted_lines;
	unsigned int total_modified_lines;

	    /* data of diff result for one kind of language */
	    unsigned int changed_blank_lines;
	    unsigned int changed_comment_lines;
        unsigned int changed_nbnc_lines; /*all changed nbnc */
};

/* used for globe */
#ifndef DIFFCOUNT_CORE
# define XTERN extern
#else
# define XTERN
#endif


typedef struct {
	char host [32];
	char user [32];
	char passwd [64];
	int port;
	}mysql_connection;
MYSQL mysql_conn;

typedef struct
  {
    char *repos_id;
    char *svn_url_path;
	char *user_id;
	char *revision;
	char *date;
	char *commit_level;
	char *log;
	char *server_ip;
	char *server_id;
	char *repos_name;
	char *branch;
	char *grandpa_revision;
  }svn_info_struct;

svn_info_struct svn_info;

static mysql_connection svn_count_connection = {"IP", "USERNAME", "PASSWORD",3306};

/*language type and information
  You can add or customize new language in lang.h */
#include "lang.h"


/* list for one diffed file ,record the comment analyze result */
XTERN struct diffed_line_node * line_node_head;
/* ¿é×¢ÊÍ·û±ê¼Ç*/
 static int block_comment = 0;
/* ×Ö·û´®±ê¼Ç*/
 static int string_process = 0;

XTERN struct diffed_line_node * line_node_tail;
XTERN bool git_count_debug_on ;
XTERN bool git_count_process ;
XTERN bool git_count_update;

/*globe pointer to language of current file */
XTERN struct language_type * current_language;

/* print every diffed lines information
   controlled by option --print-lines-info */
XTERN bool print_lines_info;

/* print every diffed files information
   controlled by option --print-files-info */
XTERN bool print_files_info;

/* Ignore differences in case of letters in file names.  */
XTERN bool ignore_file_name_case;

XTERN bool counting_only;
XTERN bool git_post_commit_counting;
XTERN bool git_count_test;

XTERN bool skip_identical_file;

XTERN bool for_program_reading;

/* add by caoyang for ignore white space in lines */
XTERN bool ignore_white_space;


/*----------------------------------*/


/* Ignore changes that affect only blank lines (-B).  */
XTERN bool ignore_blank_lines;

XTERN struct re_pattern_buffer ignore_regexp;

/* Number of lines of context to show in each set of diffs.
   This is zero when context is not to be shown.  */
XTERN lin context;

/* Number of lines to keep in identical prefix and suffix.  */
XTERN lin horizon_lines;

/* Don't discard lines.  This makes things slower (sometimes much
   slower) but will find a guaranteed minimal set of changes.  */
XTERN bool minimal;

/* Patterns that match file names to be excluded.  */
XTERN struct exclude *excluded;

/* Name of program the user invoked (for error messages).  */
XTERN char *program_name;

/* The strftime format to use for time strings.  */
XTERN char const *time_format;

/* The result of comparison is an "edit script": a chain of `struct change'.
   Each `struct change' represents one place where some lines are deleted
   and some are inserted.

   LINE0 and LINE1 are the first affected lines in the two files (origin 0).
   DELETED is the number of lines deleted here from file 0.
   INSERTED is the number of lines inserted here in file 1.

   If DELETED is 0 then LINE0 is the number of the line before
   which the insertion was done; vice versa for INSERTED and LINE1.  */
#define SVN_COUNT_DEBUG(fmt, ...)        svn_count_debug(__FILE__, __LINE__, fmt, __VA_ARGS__)


struct change
{
  struct change *link;		/* Previous or next edit command  */
  lin inserted;			/* # lines of file 1 changed here.  */
  lin deleted;			/* # lines of file 0 changed here.  */
  lin line0;			/* Line number of 1st deleted line.  */
  lin line1;			/* Line number of 1st inserted line.  */
  bool ignore;			/* Flag used in context.c.  */
};

/* Structures that describe the input files.  */



/* Data on one input file being compared.  */
struct file_data {
    int             desc;	/* File descriptor  */
    char const      *name;	/* File name  */
    struct stat     stat;	/* File status */

    /* Buffer in which text of file is read.  */
    word *buffer;

    /* Allocated size of buffer, in bytes.  Always a multiple of
       sizeof *buffer.  */
    size_t bufsize;

    /* Number of valid bytes now in the buffer.  */
    size_t buffered;

    /* Array of pointers to lines in the file.  */
    char const **linbuf;

    /* linbuf_base <= buffered_lines <= valid_lines <= alloc_lines.
       linebuf[linbuf_base ... buffered_lines - 1] are possibly differing.
       linebuf[linbuf_base ... valid_lines - 1] contain valid data.
       linebuf[linbuf_base ... alloc_lines - 1] are allocated.  */
    lin linbuf_base, buffered_lines, valid_lines, alloc_lines;

    /* Pointer to end of prefix of this file to ignore when hashing.  */
    char const *prefix_end;

    /* Count of lines in the prefix.
       There are this many lines in the file before linbuf[0].  */
    lin prefix_lines;

    /* Pointer to start of suffix of this file to ignore when hashing.  */
    char const *suffix_begin;

    /* Vector, indexed by line number, containing an equivalence code for
       each line.  It is this vector that is actually compared with that
       of another file to generate differences.  */
    lin *equivs;

    /* Vector, like the previous one except that
       the elements for discarded lines have been squeezed out.  */
    lin *undiscarded;

    /* Vector mapping virtual line numbers (not counting discarded lines)
       to real ones (counting those lines).  Both are origin-0.  */
    lin *realindexes;

    /* Total number of nondiscarded lines.  */
    lin nondiscarded_lines;

    /* Vector, indexed by real origin-0 line number,
       containing TRUE for a line that is an insertion or a deletion.
       The results of comparison are stored here.  */
    bool *changed;

    /* 1 if file ends in a line with no final newline.  */
    bool missing_newline;

    /* 1 if at end of file.  */
    bool eof;

    /* 1 more than the maximum equivalence value used for this or its
       sibling file.  */
    lin equiv_max;
};



/* The file buffer, considered as an array of bytes rather than
   as an array of words.  */
#define FILE_BUFFER(f) ((char *) (f)->buffer)

/* Data on two input files being compared.  */

struct comparison
  {
    struct file_data file[2];
    struct comparison const *parent;  /* parent, if a recursive comparison */
  };


/* Describe the two files currently being compared.  */

XTERN struct file_data files[2];

void svn_count_debug(const char *file, int line, const char *fmt, ...);

bool svn_count_mysql_query(MYSQL *p_mysql_conn,char * execsql);

/* Declare various functions.  */

 void initialize_diff_list();
 void destroy_diff_list();


/* analyze.c */
int diff_2_files (struct comparison *);
bool insert_code_count(char *,char *,char *,char *,char *,char *,char *,int ,int ,int ,int ,int ,int );

//int diff_2_files_memery (struct comparison *);

int diff_2_files_memery (struct comparison *,char *,char *);

/* io.c */
void file_block_read (struct file_data *, size_t);
bool read_files (struct file_data[]);
bool read_files_memery (struct file_data[],char *,char *);



/* dir.c */
int diff_dirs (struct comparison const *, int (*) (struct comparison const *, char const *, char const *));



/* util.c */
char *concat (char const *, char const *, char const *);
char *dir_file_pathname (char const *, char const *);
bool lines_differ (char const *, char const *);
lin translate_line_number (struct file_data const *, lin);
struct change *find_change (struct change *);
struct change *find_reverse_change (struct change *);
void *zalloc (size_t);
enum changes analyze_hunk (struct change *, lin *, lin *, lin *, lin *);
void debug_script (struct change *);

void process_script (struct change *, struct change * (*) (struct change *), void (*) (struct change *));



int find_string_in_array(char *S, char *T, int s_array_len);
int find_string(char * S, char * T);


void fatal (char const *) __attribute__((noreturn));
void message (char const *, char const *, char const *);
void message5 (char const *, char const *, char const *, char const *, char const *);
void perror_with_name (char const *);
void pfatal_with_name (char const *) __attribute__((noreturn));
void print_message_queue (void);

void translate_range (struct file_data const *, lin, lin, long *, long *);




/* version.c */

extern char const copyright_string[];
extern char const version_string[];










