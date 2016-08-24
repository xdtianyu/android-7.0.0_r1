UP_SQL = """
ALTER TABLE tko_iteration_result MODIFY attribute varchar(256) default NULL;
"""

DOWN_SQL = """
ALTER TABLE tko_iteration_result MODIFY attribute varchar(30) default NULL;
"""
