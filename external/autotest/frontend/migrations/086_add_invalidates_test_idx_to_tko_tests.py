ADD_COLUMN = """
ALTER TABLE tko_tests
ADD COLUMN `invalidates_test_idx` int(10) unsigned DEFAULT NULL;
"""
ADD_INDEX = """ALTER TABLE tko_tests ADD INDEX(invalidates_test_idx);"""
ADD_FOREIGN_KEY = """
ALTER TABLE tko_tests
ADD CONSTRAINT invalidates_test_idx_fk FOREIGN KEY
(`invalidates_test_idx`) REFERENCES `tko_tests`(`test_idx`)
ON DELETE NO ACTION;
"""
DROP_FOREIGN_KEY = """
ALTER TABLE tko_tests DROP FOREIGN KEY `invalidates_test_idx_fk`;
"""
DROP_COLUMN = """ALTER TABLE tko_tests DROP `invalidates_test_idx`; """

def migrate_up(manager):
    """Pick up the changes.

    @param manager: A MigrationManager object.

    """
    manager.execute(ADD_COLUMN)
    manager.execute(ADD_INDEX)
    manager.execute(ADD_FOREIGN_KEY)


def migrate_down(manager):
    """Drop the changes.

    @param manager: A MigrationManager object.

    """
    manager.execute(DROP_FOREIGN_KEY)
    manager.execute(DROP_COLUMN)
