UP_SQL = """
ALTER TABLE afe_special_tasks ADD COLUMN time_finished DATETIME;
"""

DOWN_SQL = """
ALTER TABLE afe_special_tasks DROP COLUMN time_finished;
"""
