UP_SQL = """
ALTER TABLE tko_tests MODIFY reason VARCHAR(4096);
"""

DOWN_SQL = """
ALTER TABLE tko_tests MODIFY reason VARCHAR(1024);
"""
