#-----------------------------------------------------
# Makefile for DiffCount (CORE and CLI)
# for Linux and Cygwin (share this Makefile) 
# C.YANG  2006   Licensed under GPL
#------------------------------------------------------
#  This file is part of HikSvnCount.
#  SVN Count Part developed by Haiyuan.Qian 2013
#  Software Configuration Management of HANGZHOU HIKVISION DIGITAL TECHNOLOGY CO.,LTD. 
#  email:qianhaiyuan@hikvision.com
#
#  HikSvnCount is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; 
#------------------------------------------------------

DELETE = @rm -f 

GNU_LIB=lib
CORE_SRC=src

CORE_LIB=$(CORE_SRC)/libdiffcount.a

CLI_EXE=./diffcount

CC=gcc
CFLAGS = -DHAVE_CONFIG_H -I. -I$(GNU_LIB) -I$(CORE_SRC) -w -O2 -c -I/usr/include/mysql
LFLAG = -g -O2 -o 
MYSQLFLAG = -L/usr/lib/x86_64-linux-gnu -lmysqlclient -lpthread -lz -lm -lrt -ldl
AR = ar
ARFLAG = cru
RANLIB = ranlib



#diffcount and static library objects
GNU_OBJS = $(GNU_LIB)/basename.o $(GNU_LIB)/c-stack.o $(GNU_LIB)/cmpbuf.o $(GNU_LIB)/error.o  $(GNU_LIB)/fnmatch.o\
          $(GNU_LIB)/exclude.o $(GNU_LIB)/exitfail.o $(GNU_LIB)/freesoft.o $(GNU_LIB)/getopt.o \
          $(GNU_LIB)/getopt1.o $(GNU_LIB)/hard-locale.o $(GNU_LIB)/imaxtostr.o $(GNU_LIB)/offtostr.o \
          $(GNU_LIB)/prepargs.o $(GNU_LIB)/posixver.o $(GNU_LIB)/quotesys.o $(GNU_LIB)/setmode.o \
          $(GNU_LIB)/strftime.o $(GNU_LIB)/umaxtostr.o $(GNU_LIB)/xmalloc.o $(GNU_LIB)/xstrtoumax.o \
	      $(GNU_LIB)/strtoumax.o $(GNU_LIB)/regex.o
	      
CORE_OBJS = $(CORE_SRC)/analyze.o $(CORE_SRC)/diffcount.o $(CORE_SRC)/dir.o $(CORE_SRC)/io.o \
            $(CORE_SRC)/count.o $(CORE_SRC)/util.o $(CORE_SRC)/version.o 
           
EXE_OBJS = $(CORE_SRC)/cmd.o 

all:$(CLI_EXE)

#build final EXE
$(CLI_EXE): $(CORE_LIB) $(EXE_OBJS)
	$(DELETE) $(CLI_EXE)
	$(CC) $(LFLAG) $(CLI_EXE) $(EXE_OBJS) $(CORE_LIB) $(MYSQLFLAG)
	@echo " ----------------------------------------------------- "
	@echo " diffcount command compiled successful in ./src"
	@echo " libdiffcount core and head file in ./src "
	@echo " ----------------------------------------------------- "



#build static support library
$(CORE_LIB): $(GNU_OBJS) $(CORE_OBJS)
	$(DELETE) $(CORE_LIB)
	$(AR) $(ARFLAG) $(CORE_LIB) $(GNU_OBJS) $(CORE_OBJS) 
	$(RANLIB) $(CORE_LIB)
	

clean:
	$(DELETE) $(GNU_OBJS) $(CORE_OBJS) $(EXE_OBJS) $(CORE_LIB) $(CLI_EXE)


$(GNU_LIB)/%.o:$(GNU_LIB)/%.c
	$(CC) $(CFLAGS) $< -o $@
	
$(CORE_SRC)/%.o:$(CORE_SRC)/%.c
	$(CC) $(CFLAGS) $< -o $@


include	make.deps.linux

install:
	@cp -f $(CLI_EXE) /_cmcenter/svncount
	@echo " ----------------------------------------------------- "
	@echo " diffcount command installed in /_cmcenter/svncount "
	@echo " ----------------------------------------------------- "

