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

一、使用方法：

需要使用该功能的git服务器必须在git_server中被记录，否则无法使用diffcount。
git diffcount使用方法
diffcount --git-post-receive –process –update –p –g –r –b -n
 
    --git-post-recieve 
       git post recieve diff count
	发起一次代码统计
   --process 
       show the git count process
   --update 
       repair git codecount
	更新模式，用于一些异常数据的修复
   -p (used with --git-post-recieve)
       repository's path
	版本库路径(服务器端数据路径)
   -g (used with--git-post-recieve)
       count git grandpa revision
	上一次统计时的版本号
   -r (used with--git-post-recieve)
       count git revision
	待统计的版本
   -b (used with--git-post-recieve)
       count git branch
	待统计的分支
   -n (used with--git-post-recieve)
       count git repo ID
	待统计的repo ID

二、执行统计程序触发统计
/gitcount/core/diffcount --git-post-recieve  -p /var/opt/gitlab/git-data/repositories/root/count-test.git -n 105 -b master -g b899a6da8a8ce9c9e5cccffa8cf9f6d827fb8760 -r c371f50fd41eaf5321c17aafc1d3b243bfe88beb

注：该程序并不使用gitlab自带git客户端，而使用git官方客户端，需在/usr/bin目录下安装有git官方客户端工具。
