UP_SQL = """
ALTER TABLE afe_autotests ADD COLUMN run_reset SMALLINT NOT NULL DEFAULT '1';
ALTER TABLE afe_jobs ADD COLUMN run_reset SMALLINT NOT NULL DEFAULT '1';
"""

DOWN_SQL = """
ALTER TABLE afe_autotests DROP COLUMN run_reset;
ALTER TABLE afe_jobs DROP COLUMN run_reset;
"""