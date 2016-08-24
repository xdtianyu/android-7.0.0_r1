UP_SQL = """
ALTER TABLE afe_hosts ADD COLUMN lock_reason TEXT DEFAULT NULL
"""

DOWN_SQL = """
ALTER TABLE afe_hosts DROP COLUMN lock_reason
"""
