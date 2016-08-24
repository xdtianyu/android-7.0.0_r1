UP_SQL = """
ALTER TABLE afe_hosts ADD COLUMN leased TINYINT(1) NOT NULL DEFAULT '1';
CREATE INDEX leased_hosts ON afe_hosts (leased, locked);
"""

DOWN_SQL = """
DROP INDEX leased_hosts ON afe_hosts;
ALTER TABLE afe_hosts DROP COLUMN leased;
"""
