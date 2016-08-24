UP_SQL = """
ALTER TABLE afe_shards ADD COLUMN hostname VARCHAR(255) NOT NULL;
ALTER TABLE afe_hosts ADD COLUMN shard_id INT NULL;
ALTER TABLE afe_hosts ADD CONSTRAINT hosts_to_shard_ibfk
    FOREIGN KEY (shard_id) REFERENCES afe_shards(id);
"""

DOWN_SQL = """
ALTER TABLE afe_hosts DROP FOREIGN KEY hosts_to_shard_ibfk;
ALTER TABLE afe_hosts DROP COLUMN shard_id;

ALTER TABLE afe_shards DROP COLUMN hostname;
"""
