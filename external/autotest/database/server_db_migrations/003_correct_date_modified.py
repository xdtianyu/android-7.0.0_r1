UP_SQL = """
ALTER TABLE servers
MODIFY date_modified timestamp DEFAULT current_timestamp on update current_timestamp;
"""

DOWN_SQL = """
ALTER TABLE servers
MODIFY date_modified datetime DEFAULT NULL;
"""
