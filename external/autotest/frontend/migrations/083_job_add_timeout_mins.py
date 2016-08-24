UP_SQL = """
ALTER TABLE afe_jobs ADD COLUMN timeout_mins integer NOT NULL;
UPDATE afe_jobs SET timeout_mins = timeout * 60;
"""

DOWN_SQL = """
ALTER TABLE afe_jobs DROP COLUMN timeout_mins;
"""