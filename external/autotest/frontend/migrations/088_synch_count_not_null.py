UP_SQL = """
UPDATE afe_jobs SET synch_count=0 WHERE synch_count IS NULL;
ALTER TABLE afe_jobs MODIFY synch_count int(11) NOT NULL;
"""

DOWN_SQL = """
ALTER TABLE afe_jobs MODIFY synch_count int(11) NULL;
"""