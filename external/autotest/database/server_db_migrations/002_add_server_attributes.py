UP_SQL = """
CREATE TABLE `server_attributes` (
  `id` int(11) NOT NULL auto_increment,
  `server_id` int(11) DEFAULT NULL,
  `attribute` varchar(128) DEFAULT NULL,
  `value` text,
  `date_modified` timestamp DEFAULT current_timestamp on update current_timestamp,
  PRIMARY KEY (`id`),
  KEY `fk_server_attributes_server_idx` (`server_id`),
  CONSTRAINT `fk_server_attributes_server_server_id` FOREIGN KEY (`server_id`) REFERENCES `servers` (`id`) ON DELETE CASCADE ON UPDATE NO ACTION
);
"""

DOWN_SQL = """
DROP TABLE server_attributes;
"""