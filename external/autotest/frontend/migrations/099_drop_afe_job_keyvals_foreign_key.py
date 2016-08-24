UP_SQL = """
ALTER TABLE afe_job_keyvals DROP FOREIGN KEY `afe_job_keyvals_ibfk_1`;
"""

# Before syncing down, you must cleanup the rows that do not meet
# the foreign key constraint by:
#
# DELETE t1 FROM afe_job_keyvals t1
# LEFT OUTER JOIN afe_jobs t2
# ON (t1.job_id = t2.id) WHERE t2.id is NULL;
#
# Execute with care!

DOWN_SQL = """
ALTER TABLE afe_job_keyvals
ADD CONSTRAINT afe_job_keyvals_ibfk_1 FOREIGN KEY
(`job_id`) REFERENCES `afe_jobs`(`id`)
ON DELETE NO ACTION;
"""
