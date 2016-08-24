UP_SQL = """
INSERT INTO afe_jobs_dependency_labels (job_id, label_id)
SELECT job_id, meta_host FROM afe_host_queue_entries
WHERE NOT complete AND NOT active AND status="Queued" AND NOT aborted;
"""

DOWN_SQL="""
"""
