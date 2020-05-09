CREATE TABLE `git_domain` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `domain_name` varchar(64) NOT NULL,
  `repository_dir` varchar(128) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8
CREATE TABLE `git_server` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `domain_id` int(11) NOT NULL,
  `server_ip` varchar(64) DEFAULT NULL,
  `uuid` varchar(36) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `FK_domain_id` (`domain_id`),
  CONSTRAINT `FK_domain_id` FOREIGN KEY (`domain_id`) REFERENCES `git_domain` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8
CREATE TABLE `git_code_count` (
  `server_id` int(11) NOT NULL,
  `repos_id` int(11) NOT NULL,
  `revision` char(40) NOT NULL,
  `path` varchar(330) NOT NULL,
  `user_id` int(11) NOT NULL,
  `date` datetime NOT NULL,
  `diff_state` enum('DEL','NEW','MOD') DEFAULT NULL,
  `added_lines` int(11) DEFAULT '0',
  `modified_lines` int(11) DEFAULT '0',
  `deleted_lines` int(11) DEFAULT '0',
  `changed_blank_lines` int(11) DEFAULT '0',
  `changed_comment_lines` int(11) DEFAULT '0',
  `changed_nbnc_lines` int(11) DEFAULT '0',
  `language_type` varchar(32) DEFAULT NULL,
  `commit_level` enum('Replace','ImportII','ImportI','Warning','Normal') DEFAULT NULL,
  `branch` varchar(30) DEFAULT NULL,
  `grandpa_revision` varchar(40) NOT NULL,
  PRIMARY KEY (`server_id`,`repos_id`,`revision`,`path`),
  CONSTRAINT `FK_server_id` FOREIGN KEY (`server_id`) REFERENCES `git_server` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8
CREATE TABLE `git_code_count_error` (
  `id` int(11) NOT NULL,
  `server_id` int(11) DEFAULT NULL,
  `repos_id` int(11) NOT NULL,
  `revision` varchar(40) DEFAULT NULL,
  `update_time` datetime DEFAULT NULL,
  `grandpa_revision` varchar(40) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_index_key` (`server_id`,`revision`,`repos_id`),
  CONSTRAINT `server_id` FOREIGN KEY (`server_id`) REFERENCES `git_server` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8
CREATE TABLE `git_code_count_process` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `server_id` int(11) NOT NULL,
  `repos_id` int(11) NOT NULL,
  `revision` varchar(40) DEFAULT NULL,
  `update_time` datetime DEFAULT NULL,
  `grandpa_revision` varchar(40) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `unique_index_key` (`server_id`,`repos_id`,`revision`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8
CREATE TABLE `git_audit_code_import` (
  `server_id` int(11) NOT NULL,
  `repos_id` int(11) NOT NULL,
  `user_id` int(11) NOT NULL,
  `revesion` varchar(40) NOT NULL DEFAULT '',
  `counts` int(11) NOT NULL,
  `time` datetime NOT NULL,
  `repos_name` varchar(64) DEFAULT NULL,
  `server_ip` varchar(32) DEFAULT NULL,
  `log_message` varchar(256) DEFAULT NULL,
  PRIMARY KEY (`repos_id`,`revesion`,`server_id`),
  KEY `index_user_id` (`user_id`) USING BTREE,
  KEY `index_server_id` (`server_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8

## SVN”√ªß±Ì
CREATE TABLE `svn_user` (
  `user_id` int(11) NOT NULL AUTO_INCREMENT,
  `user_name` varchar(32) NOT NULL,
  `full_name` varchar(32) DEFAULT NULL,
  `staff_no` varchar(16) DEFAULT NULL,
  `email` varchar(64) DEFAULT NULL,
  `getpwd_date` datetime DEFAULT NULL,
  `getpwd_times` int(11) DEFAULT '0',
  `creation_date` datetime DEFAULT NULL,
  `disable` bit(1) DEFAULT b'0',
  `disable_date` datetime DEFAULT NULL,
  `remark` varchar(256) DEFAULT NULL,
  `deptcode` varchar(32) DEFAULT NULL,
  PRIMARY KEY (`user_id`) USING BTREE,
  UNIQUE KEY `index_user_name` (`user_name`),
  KEY `index_full_name` (`full_name`),
  KEY `index_disable` (`disable`),
  KEY `index_staff_no` (`staff_no`),
  KEY `index_deptcode` (`deptcode`) USING BTREE
) ENGINE=MyISAM AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;


