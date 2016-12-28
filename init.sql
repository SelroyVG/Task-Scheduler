CREATE DATABASE `tasks_scheduler`;
USE `tasks_scheduler`;
DROP TABLE IF EXISTS `daemons_log`;
CREATE TABLE `daemons_log` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `command` varchar(1024) NOT NULL,
  `status` varchar(32) NOT NULL,
  `launch_time` datetime NOT NULL,
  PRIMARY KEY(`id`)
) CHARSET='utf8';