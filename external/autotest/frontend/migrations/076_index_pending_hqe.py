UP_SQL = """
CREATE INDEX host_queue_entry_status ON afe_host_queue_entries (status);
"""

DOWN_SQL = """
DROP INDEX host_queue_entry_status ON afe_host_queue_entries;
"""
