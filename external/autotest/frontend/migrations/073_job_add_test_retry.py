UP_SQL = """
ALTER TABLE afe_jobs ADD COLUMN test_retry integer NOT NULL DEFAULT '0';
"""

DOWN_SQL = """
ALTER TABLE afe_jobs DROP COLUMN test_retry;
"""
