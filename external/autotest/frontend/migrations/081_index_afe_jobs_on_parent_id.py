UP_SQL = """
CREATE INDEX parent_job_id_index ON afe_jobs (parent_job_id);
"""

DOWN_SQL = """
DROP INDEX parent_job_id_index ON afe_jobs;
"""

