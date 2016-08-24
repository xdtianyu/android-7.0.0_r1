UP_SQL = """
ALTER TABLE afe_special_tasks ADD COLUMN is_aborted TINYINT(1) NOT NULL DEFAULT '0';
"""

DOWN_SQL = """
ALTER TABLE afe_special_tasks DROP COLUMN is_aborted;
"""
