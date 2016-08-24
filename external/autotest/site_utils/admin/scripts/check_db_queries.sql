-- Check Autotest database.

-- To run: time chromedbread < /tmp/check_db_queries.sql

-- Y - indicates a table count that will be checked.

-- AFE Tables
--   | afe_aborted_host_queue_entries            |
-- Y | afe_acl_groups                            |
-- Y | afe_acl_groups_hosts                      |
-- Y | afe_acl_groups_users                      |
--   | afe_atomic_groups                         |
-- Y | afe_autotests                             |
--   | afe_autotests_dependency_labels           |
--   | afe_drone_sets                            |
--   | afe_drone_sets_drones                     |
--   | afe_drones                                |
--   | afe_host_attributes                       |
--   | afe_host_queue_entries                    |
-- Y | afe_hosts                                 |
-- Y | afe_hosts_labels                          |
--   | afe_ineligible_host_queues                |
--   | afe_job_keyvals                           |
-- Y | afe_jobs                                  |
--   | afe_jobs_dependency_labels                |
--   | afe_kernels                               |
-- Y | afe_labels                                |
--   | afe_parameterized_job_parameters          |
--   | afe_parameterized_job_profiler_parameters |
--   | afe_parameterized_jobs                    |
--   | afe_parameterized_jobs_kernels            |
--   | afe_parameterized_jobs_profilers          |
--   | afe_profilers                             |
--   | afe_recurring_run                         |
--   | afe_special_tasks                         |
--   | afe_test_parameters                       |
-- Y | afe_users                                 |

select count(*) as count_afe_acl_groups from afe_acl_groups;
select count(*) as count_afe_acl_groups_hosts from afe_acl_groups_hosts;
select count(*) as count_afe_acl_groups_users from afe_acl_groups_users;
select count(*) as count_afe_autotests from afe_autotests;
select count(*) as count_afe_hosts from afe_hosts;
select count(*) as count_afe_hosts_labels from afe_hosts_labels;
select count(*) as count_afe_jobs from afe_jobs;
select count(*) as count_afe_labels from afe_labels;
select count(*) as count_afe_users from afe_users;

-- TKO Tables
--   | tko_embedded_graphing_queries         |
--   | tko_iteration_attributes              |
--   | tko_iteration_result                  |
-- Y | tko_job_keyvals                       |
-- Y | tko_jobs                              |
--   | tko_kernels                           |
-- Y | tko_machines                          |
--   | tko_patches                           |
--   | tko_perf_view                         |
-- Y | tko_perf_view_2                       |
--   | tko_query_history                     |
--   | tko_saved_queries                     |
-- Y | tko_status                            |
-- Y | tko_test_attributes                   |
--   | tko_test_labels                       |
--   | tko_test_labels_tests                 |
--   | tko_test_view                         |
-- Y | tko_test_view_2                       |
--   | tko_test_view_outer_joins             |
-- Y | tko_tests                             |

select count(*) as count_tko_job_keyvals from tko_job_keyvals;
select count(*) as count_tko_jobs from tko_jobs;
select count(*) as count_tko_machines from tko_machines;
select count(*) as count_tko_perf_view_2 from tko_perf_view_2;
select count(*) as count_tko_status from tko_status;
select count(*) as count_tko_test_attributes from tko_test_attributes;
select count(*) as count_tko_test_view_2 from tko_test_view_2;
select count(*) as count_tko_tests from tko_tests;

-- Now check for a few details.

select count(*) as jobs_per_board, left(name,instr(name,'-0')-1) as board from afe_jobs where name like 'x86%' group by board order by board;
select count(*) as platform_count from afe_labels where platform=true;
select `key`, count(*) as job_keyval_count from tko_job_keyvals group by `key`;
select month(queued_time), count(*) as tko_jobs_per_month from tko_jobs group by month(queued_time);
select status, count(*) from tko_test_view_2 group by status;
select left(test_name, 5) as test_name_prefix, count(*) from tko_test_view_2 group by test_name_prefix;
select count(*) as values_per_board, left(job_name,instr(job_name,'-0')-1) as board from tko_perf_view_2 where job_name like 'x86%' group by board order by board;
select left(iteration_key, 5) as key_name_prefix, count(*) from tko_perf_view_2 group by key_name_prefix;
