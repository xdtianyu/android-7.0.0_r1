UP_SQL = """
CREATE TABLE afe_shards (
  id INT NOT NULL AUTO_INCREMENT PRIMARY KEY
) ENGINE=innodb;

ALTER TABLE afe_jobs ADD COLUMN shard_id INT NULL;
ALTER TABLE afe_jobs ADD CONSTRAINT jobs_to_shard_ibfk
    FOREIGN KEY (shard_id) REFERENCES afe_shards(id);

CREATE TABLE afe_shards_labels (
    id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    shard_id INT NOT NULL,
    label_id INT NOT NULL
) ENGINE=InnoDB;

ALTER TABLE `afe_shards_labels` ADD CONSTRAINT shard_shard_id_fk
    FOREIGN KEY (`shard_id`) REFERENCES `afe_shards` (`id`);
ALTER TABLE `afe_shards_labels` ADD CONSTRAINT shard_label_id_fk
    FOREIGN KEY (`label_id`) REFERENCES `afe_labels` (`id`);
"""

DOWN_SQL = """
ALTER TABLE afe_jobs DROP FOREIGN KEY jobs_to_shard_ibfk;
ALTER TABLE afe_jobs DROP COLUMN shard_id;

ALTER TABLE afe_shards_labels DROP FOREIGN KEY shard_label_id_fk;
ALTER TABLE afe_shards_labels DROP FOREIGN KEY shard_shard_id_fk;
DROP TABLE afe_shards_labels;

DROP TABLE afe_shards;
"""
