UP_SQL = """
CREATE TABLE afe_stable_versions (
  id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  board VARCHAR(255) NOT NULL,
  version VARCHAR(255) NOT NULL,
  UNIQUE KEY `board_UNIQUE` (`board`)
) ENGINE=innodb;
"""

DOWN_SQL = """
DROP TABLE afe_stable_versions;
"""
