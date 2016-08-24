UP_SQL = """
CREATE INDEX host_queue_entries_abort_incomplete ON afe_host_queue_entries (aborted, complete);
"""

DOWN_SQL = """
DROP INDEX host_queue_entries_abort_incomplete ON afe_host_queue_entries;
"""

