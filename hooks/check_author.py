#!/usr/bin/env python
# -*- coding:utf-8 -*--
#   Analyze file differences for DIFFCOUNT.
# 
#   DIFF part comes from GNU DIFF
#   Copyright (C) 1988, 1989, 1991, 1992, 1993, 1994, 1995, 1998, 2001,
#   2002 Free Software Foundation, Inc.
#
#   This file is part of DIFFCOUNT.
#   Count Part developed by C.YANG. 2006
#
#   DIFFCOUNT is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation;
#
#   This file is part of HikSvnCount.
#   SVN Count Part developed by Haiyuan.Qian 2013
#   Software Configuration Management of HANGZHOU HIKVISION DIGITAL TECHNOLOGY CO.,LTD.
#   email:qianhaiyuan@hikvision.com
#
#   HikSvnCount is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation;
# author      :hikvision
# create time :2020/4/10 15:26
# description :
# IDE         :PyCharm

import os
import pymysql
import sys
import subprocess


class Checker:

    def __init__(self):
        self.connector = pymysql.connect(host="IP",port=3306,user="USERNAME",password="PASSWORD",database="svnadmin")
        self.cursor = self.connector.cursor(cursor=pymysql.cursors.DictCursor)

    def check_author(self, author:str)->bool:
        check_statement = "select user_id from svn_user where user_name='%s' or full_name='%s'" %(author,author)
        self.cursor.execute(check_statement)
        user_id = self.cursor.fetchone()
        if user_id:
            sys.exit(0)
        else:
            print(author)
            sys.exit(1)

    def get_author(self, commit_id, repos_path):
        os.chdir(repos_path)
        author_cmd = "/usr/bin/git log --pretty=format:\"%an\" \"{commit_id}\" -1".format(commit_id=commit_id)
        res = subprocess.Popen(author_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = res.communicate()
        return_code = res.poll()
        if return_code != 0:
            return False
        else:
            author = str(stdout, encoding="utf-8")
            return author


if __name__=="__main__":
    if len(sys.argv) != 3:
        print(u"error parameter!")
        sys.exit(1)
    args = sys.argv[1:]
    commit_id = args[0]
    repos_path = args[1]
    checker = Checker()
    author = checker.get_author(commit_id, repos_path=repos_path)
    if author is False:
        sys.exit(1)
    checker.check_author(author)

