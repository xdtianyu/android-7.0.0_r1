UP_SQL = """
ALTER VIEW tko_test_view_2 AS
SELECT  tko_tests.test_idx,
        tko_tests.job_idx,
        tko_tests.test AS test_name,
        tko_tests.subdir,
        tko_tests.kernel_idx,
        tko_tests.status AS status_idx,
        tko_tests.reason,
        tko_tests.machine_idx,
        tko_tests.invalid,
        tko_tests.invalidates_test_idx,
        tko_tests.started_time AS test_started_time,
        tko_tests.finished_time AS test_finished_time,
        tko_jobs.tag AS job_tag,
        tko_jobs.label AS job_name,
        tko_jobs.username AS job_owner,
        tko_jobs.queued_time AS job_queued_time,
        tko_jobs.started_time AS job_started_time,
        tko_jobs.finished_time AS job_finished_time,
        tko_jobs.afe_job_id AS afe_job_id,
        tko_machines.hostname AS hostname,
        tko_machines.machine_group AS platform,
        tko_machines.owner AS machine_owner,
        tko_kernels.kernel_hash,
        tko_kernels.base AS kernel_base,
        tko_kernels.printable AS kernel,
        tko_status.word AS status
FROM tko_tests
INNER JOIN tko_jobs ON tko_jobs.job_idx = tko_tests.job_idx
INNER JOIN tko_machines ON tko_machines.machine_idx = tko_jobs.machine_idx
INNER JOIN tko_kernels ON tko_kernels.kernel_idx = tko_tests.kernel_idx
INNER JOIN tko_status ON tko_status.status_idx = tko_tests.status;
"""


DOWN_SQL = """
ALTER VIEW tko_test_view_2 AS
SELECT  tko_tests.test_idx,
        tko_tests.job_idx,
        tko_tests.test AS test_name,
        tko_tests.subdir,
        tko_tests.kernel_idx,
        tko_tests.status AS status_idx,
        tko_tests.reason,
        tko_tests.machine_idx,
        tko_tests.started_time AS test_started_time,
        tko_tests.finished_time AS test_finished_time,
        tko_jobs.tag AS job_tag,
        tko_jobs.label AS job_name,
        tko_jobs.username AS job_owner,
        tko_jobs.queued_time AS job_queued_time,
        tko_jobs.started_time AS job_started_time,
        tko_jobs.finished_time AS job_finished_time,
        tko_jobs.afe_job_id AS afe_job_id,
        tko_machines.hostname AS hostname,
        tko_machines.machine_group AS platform,
        tko_machines.owner AS machine_owner,
        tko_kernels.kernel_hash,
        tko_kernels.base AS kernel_base,
        tko_kernels.printable AS kernel,
        tko_status.word AS status
FROM tko_tests
INNER JOIN tko_jobs ON tko_jobs.job_idx = tko_tests.job_idx
INNER JOIN tko_machines ON tko_machines.machine_idx = tko_jobs.machine_idx
INNER JOIN tko_kernels ON tko_kernels.kernel_idx = tko_tests.kernel_idx
INNER JOIN tko_status ON tko_status.status_idx = tko_tests.status;
"""
