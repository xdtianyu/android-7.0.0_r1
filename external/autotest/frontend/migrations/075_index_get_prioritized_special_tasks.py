UP_SQL = """
CREATE INDEX host_queue_entries_host_active ON afe_host_queue_entries (host_id, active);
CREATE INDEX special_tasks_active_complete ON afe_special_tasks (is_active, is_complete);
"""

DOWN_SQL = """
DROP INDEX host_queue_entries_host_active ON afe_host_queue_entries;
DROP INDEX special_tasks_active_complete ON afe_special_tasks;
"""

