UP_SQL = """
CREATE INDEX owner_index ON afe_jobs (owner);
"""

DOWN_SQL = """
DROP INDEX owner_index ON afe_jobs;
"""
