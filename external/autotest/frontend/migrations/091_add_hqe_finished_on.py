UP_SQL = """
ALTER TABLE afe_host_queue_entries ADD COLUMN finished_on datetime NULL;
"""

DOWN_SQL = """
ALTER TABLE afe_host_queue_entries DROP COLUMN finished_on;
"""
