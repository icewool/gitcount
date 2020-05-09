/* LANG defination head file for DIFFCOUNT Utility.

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
   
#ifndef _LANG_H_
#define _LANG_H_

#ifdef DIFFCOUNT_CORE

/* initialize the language information 
   you can add new language type here.*/
   
/* 每一个语言信息，从第一列到最后一列的信息依次是:

 序列号(顺序增加);是否被统计(初始为0);语言名称;行注释符号;块注释开始符;块注释结束符
 可选的块注释开始符;可选的块注释结束符;字符串分割符;扩展名列表(以分号分割，以分号结束);
 和标准C的折算比例;后边六个初始值均为0 */

/* 扩展名格式: ";扩展名1;扩展名2;....;" */

struct language_type languages[]=
{{0,0, "C", "//", "/*", "*/", "", "","\"","'","\\",";c;h;cc;","", ".c", 1,0, 0, 0, 0, 0, 0},
 {1,0, "C++", "//", "/*", "*/", "", "", "\"","'","\\", ";cpp;hpp;","",".cpp",0.54,0, 0, 0, 0, 0, 0},
 {2,0, "Pascal", "//", "(*", "*)", "{", "}", "\'","","\\",";pas;pp;","",".pp",0.23, 0, 0, 0, 0, 0, 0},
 {3,0, "Java", "//", "/*", "*/", "", "", "\"","'","\\",";jav;java;sqlj;","",".java",0.34, 0, 0, 0, 0, 0, 0},
 //{4,0, "SQL", "--", "/*", "*/", "", "", "\'",";sql;tql;",0.04, 0, 0, 0, 0, 0, 0},
 {4,0, "ASM", ";", "/*", "*/", "", "", "","","",";asm;s;uc;","",".asm", 2.82, 0, 0, 0, 0, 0, 0},
 {5,0, "C#", "//", "/*", "*/", "", "","\"","'","\\", ";cs;", "",".cs", 0.2, 0, 0, 0, 0, 0, 0},
 {6,0, "Basic", "'", "", "", "", "", "\"","'","\\",";bas;vbs;", "",".vbs", 0.21, 0, 0, 0, 0, 0, 0},
 {7,0, "Perl", "#", "", "", "", "", "\"","'","\\",";pl;plx;pm;", "",".pl", 0.16, 0, 0, 0, 0, 0, 0},
 {8,0, "TCL/TK", "#", "", "", "", "", "\"","'","\\",";tcl;tk;", "", ".tk",0.5, 0, 0, 0, 0, 0, 0},
 {9,0, "Python", "#", "", "", "", "", "\"","'","\\",";py;pyw;", "",".py", 0.5, 0, 0, 0, 0, 0, 0},
 //{11,0, "Config", "", "", "", "", "", "",";ini;conf;config;cfg;properties;", 0.07, 0, 0, 0, 0, 0, 0},
 //{12,0, "XML", "", "", "", "", "", "", ";xml;xul;mxml;wsdl;opk;",0.09, 0, 0, 0, 0, 0, 0},
 {10,0, "Schema", "", "", "", "", "", "","","", ";xsd;","", ".xsd",0.09, 0, 0, 0, 0, 0, 0},
 {11,0, "FPL", "//", "/*", "*/", "", "","","","", ";fpl;", "", ".fpl",10, 0, 0, 0, 0, 0, 0},
 {12,0, "ASL", "//", "/*", "*/", "", "", "","","",";asl;", "", ".asl",1,0, 0, 0, 0, 0, 0},
 {13,0, "Verilog", "//", "/*", "*/", "", "", "\"","'","\\",";v;sv;svh;", "",".v", 2.5,0, 0, 0, 0, 0, 0},
 {14,0, "HTML", "", "<!--", "-->", "", "", "\"","'","\\",";html;htm;css;scss;sass;less;hbs;", "",".html", 0.06,0, 0, 0, 0, 0, 0},
 {15,0, "Flex", "//", "/*", "*/", "", "", "\"","'","\\",";as;","",".as", 0.6, 0, 0, 0, 0, 0, 0},
 {16,0, "Make", "#", "", "", "", "", "","","",";mak;mk;make;", ";makefile;make;",".mk", 0.12, 0, 0, 0, 0, 0, 0},
 {17,0, "JScript", "//", "/*", "*/", "", "", "\"","'","\\",";js;jsx;vue;","", ".js",0.41, 0, 0, 0, 0, 0, 0},
 {18,0, "Shell", "#", "", "", "", "", "\"","'","\\",";sh;", "",".sh", 0.12, 0, 0, 0, 0, 0, 0},
 {19,0, "Lua", "--", "--[[", "]]", "", "", "\"","'","\\",";lua;","",".lua",0.41, 0, 0, 0, 0, 0, 0},
 {20,0, "Ruby", "#", "=begin", "=end", "", "", "\"","'","\\",";rb;","",".rb",0.5, 0, 0, 0, 0, 0, 0},
 {21,0, "Objective-C", "//", "/*", "*/", "", "", "\"","'","\\", ";mm;m;","",".mm",0.54,0, 0, 0, 0, 0, 0},
 {22,0, "PHP", "//", "/*", "*/", "", "", "\"","'","\\", ";php;","",".php",0.34,0, 0, 0, 0, 0, 0},
 {23,0, "ASP", "'", "", "", "", "", "\"","'","\\",";asp;aspx;", "",".asp",0.06,0, 0, 0, 0, 0, 0},
 {24,0, "JSP", "//", "<%--", "--%>", "/*", "*/", "\"","'","\\",";jsp;","",".jsp", 0.06,0, 0, 0, 0, 0, 0},
 {25,0, "Scala", "//", "/*", "*/", "", "", "\"","'","\\",";scala;","",".scala",0.34, 0, 0, 0, 0, 0, 0},
 {26,0, "Swift", "//", "/*", "*/", "", "", "\"","'","\\",";swift;","",".swift",0.34, 0, 0, 0, 0, 0, 0},
 {27,0, "Kotlin", "//", "/*", "*/", "", "", "\"","'","\\",";kt;","",".kt",0.34, 0, 0, 0, 0, 0, 0},
 {28,0, "Go", "//", "/*", "*/", "", "", "\"","'","\\",";go;","",".go",0.34, 0, 0, 0, 0, 0, 0},
 {29,0, "QML", "//", "/*", "*/", "", "", "\"","'","\\",";qml;","",".qml",0.34, 0, 0, 0, 0, 0, 0},
 {30,0, "CAPL", "//", "/*", "*/", "", "", "\"","'","\\",";can;cin;","",".can",1, 0, 0, 0, 0, 0, 0},
 {31,0, "Dart", "//", "/*", "*/", "", "", "\"","'","\\",";dart;","",".dart",1, 0, 0, 0, 0, 0, 0},
 {32,0, "MLU", "//", "/*", "*/", "", "", "\"","'","\\",";mlu;","",".mlu",0.54, 0, 0, 0, 0, 0, 0},
 /* End Flag */
 {-1,-1, "", "", "", "", "", "", "","","",0, 0, 0, 0, 0, 0, 0}
};


#else
/* for use globle variaty */
extern struct language_type languages[];
#endif

#endif

