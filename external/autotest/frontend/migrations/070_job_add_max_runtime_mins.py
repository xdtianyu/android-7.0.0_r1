UP_SQL = """
ALTER TABLE afe_jobs ADD COLUMN max_runtime_mins integer NOT NULL;
UPDATE afe_jobs SET max_runtime_mins = max_runtime_hrs * 60;
"""

DOWN_SQL = """
ALTER TABLE afe_jobs DROP COLUMN max_runtime_mins;
"""
