UP_SQL = """
ALTER TABLE tko_tests
DROP FOREIGN KEY `invalidates_test_idx_fk`,
ADD CONSTRAINT `invalidates_test_idx_fk_1`
FOREIGN KEY (`invalidates_test_idx`)
REFERENCES `tko_tests`(`test_idx`) ON DELETE CASCADE;
"""

DOWN_SQL = """
ALTER TABLE tko_tests
DROP FOREIGN KEY `invalidates_test_idx_fk_1`,
ADD CONSTRAINT `invalidates_test_idx_fk`
FOREIGN KEY (`invalidates_test_idx`)
REFERENCES `tko_tests`(`test_idx`) ON DELETE NO ACTION;
"""
