UP_SQL = """
ALTER TABLE afe_autotests ADD COLUMN test_retry integer NOT NULL DEFAULT '0';
"""

DOWN_SQL = """
ALTER TABLE afe_autotests DROP COLUMN test_retry;
"""
