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

һ��ʹ�÷�����

��Ҫʹ�øù��ܵ�git������������git_server�б���¼�������޷�ʹ��diffcount��
git diffcountʹ�÷���
diffcount --git-post-receive �Cprocess �Cupdate �Cp �Cg �Cr �Cb -n
 
    --git-post-recieve 
       git post recieve diff count
	����һ�δ���ͳ��
   --process 
       show the git count process
   --update 
       repair git codecount
	����ģʽ������һЩ�쳣���ݵ��޸�
   -p (used with --git-post-recieve)
       repository's path
	�汾��·��(������������·��)
   -g (used with--git-post-recieve)
       count git grandpa revision
	��һ��ͳ��ʱ�İ汾��
   -r (used with--git-post-recieve)
       count git revision
	��ͳ�Ƶİ汾
   -b (used with--git-post-recieve)
       count git branch
	��ͳ�Ƶķ�֧
   -n (used with--git-post-recieve)
       count git repo ID
	��ͳ�Ƶ�repo ID

����ִ��ͳ�Ƴ��򴥷�ͳ��
/gitcount/core/diffcount --git-post-recieve  -p /var/opt/gitlab/git-data/repositories/root/count-test.git -n 105 -b master -g b899a6da8a8ce9c9e5cccffa8cf9f6d827fb8760 -r c371f50fd41eaf5321c17aafc1d3b243bfe88beb

ע���ó��򲢲�ʹ��gitlab�Դ�git�ͻ��ˣ���ʹ��git�ٷ��ͻ��ˣ�����/usr/binĿ¼�°�װ��git�ٷ��ͻ��˹��ߡ�
