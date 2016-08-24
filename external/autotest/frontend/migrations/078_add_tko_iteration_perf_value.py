UP_SQL = """
CREATE TABLE tko_iteration_perf_value (
  test_idx INT(10) UNSIGNED NOT NULL,
  iteration INT(11) DEFAULT NULL,
  description VARCHAR(256) DEFAULT NULL,
  value FLOAT DEFAULT NULL,
  stddev FLOAT DEFAULT NULL,
  units VARCHAR(32) DEFAULT NULL,
  higher_is_better BOOLEAN NOT NULL DEFAULT TRUE,
  KEY test_idx (test_idx),
  KEY description (description),
  KEY value (value),
  CONSTRAINT tko_iteration_perf_value_ibfk FOREIGN KEY (test_idx)
      REFERENCES tko_tests (test_idx) ON DELETE CASCADE
) ENGINE = InnoDB;
"""

DOWN_SQL = """
DROP TABLE tko_iteration_perf_value;
"""
