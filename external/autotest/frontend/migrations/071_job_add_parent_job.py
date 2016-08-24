UP_SQL = """
ALTER TABLE afe_jobs ADD COLUMN parent_job_id integer NULL;
UPDATE afe_jobs SET parent_job_id = NULL;
"""

DOWN_SQL = """
ALTER TABLE afe_jobs DROP COLUMN parent_job_id;
"""
