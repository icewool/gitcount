#!/usr/bin/env python
# -*- coding:utf-8 -*-
import sys
import os
import fileinput
import socket
import subprocess

def main():
    # 获取repo_path
    repo_path = os.getcwd()
    # 获取repo对象
    # 获取repo_id
    repo_id = os.environ['GL_REPOSITORY'].split('-')[1]
    diffcount = "/gitcount/core/diffcount"
    # 获取推送前后的commitid
    for line in fileinput.input():
        oldrev, newrev, refname = tuple(line.split(" "))
        branch = refname.split("/")[-1].strip("\n")
        count_cmd = diffcount + " --git-post-recieve "  " -p " + repo_path + " -n " + repo_id + " -b " + branch + " -g " + oldrev + " -r " + newrev + " -d"
        print(count_cmd)
        res = subprocess.Popen(count_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
        stdout, stderr = res.communicate()

if __name__=="__main__":
    main()
