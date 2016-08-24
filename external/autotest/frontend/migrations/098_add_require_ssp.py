UP_SQL = """
ALTER TABLE afe_jobs ADD COLUMN require_ssp tinyint(1) NULL;
"""

DOWN_SQL = """
ALTER TABLE afe_jobs DROP COLUMN require_ssp;
"""
