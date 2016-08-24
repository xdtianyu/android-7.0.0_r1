UP_SQL = """
ALTER TABLE tko_iteration_perf_value ADD COLUMN graph VARCHAR(256) DEFAULT NULL;
"""

DOWN_SQL = """
ALTER TABLE tko_iteration_perf_value DROP COLUMN graph;
"""
