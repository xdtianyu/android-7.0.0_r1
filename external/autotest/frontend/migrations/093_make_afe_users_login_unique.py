UP_SQL = """
CREATE UNIQUE INDEX login_unique ON afe_users (login);
"""

DOWN_SQL = """
DROP INDEX login_unique ON afe_users;
"""