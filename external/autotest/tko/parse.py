#!/usr/bin/python -u

import datetime
import json
import os, sys, optparse, fcntl, errno, traceback, socket

import common
from autotest_lib.client.common_lib import mail, pidfile
from autotest_lib.client.common_lib import utils
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.site_utils import job_overhead
from autotest_lib.tko import db as tko_db, utils as tko_utils
from autotest_lib.tko import models, status_lib
from autotest_lib.tko.perf_upload import perf_uploader


def parse_args():
    """Parse args."""
    # build up our options parser and parse sys.argv
    parser = optparse.OptionParser()
    parser.add_option("-m", help="Send mail for FAILED tests",
                      dest="mailit", action="store_true")
    parser.add_option("-r", help="Reparse the results of a job",
                      dest="reparse", action="store_true")
    parser.add_option("-o", help="Parse a single results directory",
                      dest="singledir", action="store_true")
    parser.add_option("-l", help=("Levels of subdirectories to include "
                                  "in the job name"),
                      type="int", dest="level", default=1)
    parser.add_option("-n", help="No blocking on an existing parse",
                      dest="noblock", action="store_true")
    parser.add_option("-s", help="Database server hostname",
                      dest="db_host", action="store")
    parser.add_option("-u", help="Database username", dest="db_user",
                      action="store")
    parser.add_option("-p", help="Database password", dest="db_pass",
                      action="store")
    parser.add_option("-d", help="Database name", dest="db_name",
                      action="store")
    parser.add_option("--write-pidfile",
                      help="write pidfile (.parser_execute)",
                      dest="write_pidfile", action="store_true",
                      default=False)
    parser.add_option("--record-duration",
                      help="Record timing to metadata db",
                      dest="record_duration", action="store_true",
                      default=False)
    options, args = parser.parse_args()

    # we need a results directory
    if len(args) == 0:
        tko_utils.dprint("ERROR: at least one results directory must "
                         "be provided")
        parser.print_help()
        sys.exit(1)

    # pass the options back
    return options, args


def format_failure_message(jobname, kernel, testname, status, reason):
    """Format failure message with the given information.

    @param jobname: String representing the job name.
    @param kernel: String representing the kernel.
    @param testname: String representing the test name.
    @param status: String representing the test status.
    @param reason: String representing the reason.

    @return: Failure message as a string.
    """
    format_string = "%-12s %-20s %-12s %-10s %s"
    return format_string % (jobname, kernel, testname, status, reason)


def mailfailure(jobname, job, message):
    """Send an email about the failure.

    @param jobname: String representing the job name.
    @param job: A job object.
    @param message: The message to mail.
    """
    message_lines = [""]
    message_lines.append("The following tests FAILED for this job")
    message_lines.append("http://%s/results/%s" %
                         (socket.gethostname(), jobname))
    message_lines.append("")
    message_lines.append(format_failure_message("Job name", "Kernel",
                                                "Test name", "FAIL/WARN",
                                                "Failure reason"))
    message_lines.append(format_failure_message("=" * 8, "=" * 6, "=" * 8,
                                                "=" * 8, "=" * 14))
    message_header = "\n".join(message_lines)

    subject = "AUTOTEST: FAILED tests from job %s" % jobname
    mail.send("", job.user, "", subject, message_header + message)


def _invalidate_original_tests(orig_job_idx, retry_job_idx):
    """Retry tests invalidates original tests.

    Whenever a retry job is complete, we want to invalidate the original
    job's test results, such that the consumers of the tko database
    (e.g. tko frontend, wmatrix) could figure out which results are the latest.

    When a retry job is parsed, we retrieve the original job's afe_job_id
    from the retry job's keyvals, which is then converted to tko job_idx and
    passed into this method as |orig_job_idx|.

    In this method, we are going to invalidate the rows in tko_tests that are
    associated with the original job by flipping their 'invalid' bit to True.
    In addition, in tko_tests, we also maintain a pointer from the retry results
    to the original results, so that later we can always know which rows in
    tko_tests are retries and which are the corresponding original results.
    This is done by setting the field 'invalidates_test_idx' of the tests
    associated with the retry job.

    For example, assume Job(job_idx=105) are retried by Job(job_idx=108), after
    this method is run, their tko_tests rows will look like:
    __________________________________________________________________________
    test_idx| job_idx | test            | ... | invalid | invalidates_test_idx
    10      | 105     | dummy_Fail.Error| ... | 1       | NULL
    11      | 105     | dummy_Fail.Fail | ... | 1       | NULL
    ...
    20      | 108     | dummy_Fail.Error| ... | 0       | 10
    21      | 108     | dummy_Fail.Fail | ... | 0       | 11
    __________________________________________________________________________
    Note the invalid bits of the rows for Job(job_idx=105) are set to '1'.
    And the 'invalidates_test_idx' fields of the rows for Job(job_idx=108)
    are set to 10 and 11 (the test_idx of the rows for the original job).

    @param orig_job_idx: An integer representing the original job's
                         tko job_idx. Tests associated with this job will
                         be marked as 'invalid'.
    @param retry_job_idx: An integer representing the retry job's
                          tko job_idx. The field 'invalidates_test_idx'
                          of the tests associated with this job will be updated.

    """
    msg = 'orig_job_idx: %s, retry_job_idx: %s' % (orig_job_idx, retry_job_idx)
    if not orig_job_idx or not retry_job_idx:
        tko_utils.dprint('ERROR: Could not invalidate tests: ' + msg)
    # Using django models here makes things easier, but make sure that
    # before this method is called, all other relevant transactions have been
    # committed to avoid race condition. In the long run, we might consider
    # to make the rest of parser use django models.
    orig_tests = tko_models.Test.objects.filter(job__job_idx=orig_job_idx)
    retry_tests = tko_models.Test.objects.filter(job__job_idx=retry_job_idx)

    # Invalidate original tests.
    orig_tests.update(invalid=True)

    # Maintain a dictionary that maps (test, subdir) to original tests.
    # Note that within the scope of a job, (test, subdir) uniquelly
    # identifies a test run, but 'test' does not.
    # In a control file, one could run the same test with different
    # 'subdir_tag', for example,
    #     job.run_test('dummy_Fail', tag='Error', subdir_tag='subdir_1')
    #     job.run_test('dummy_Fail', tag='Error', subdir_tag='subdir_2')
    # In tko, we will get
    #    (test='dummy_Fail.Error', subdir='dummy_Fail.Error.subdir_1')
    #    (test='dummy_Fail.Error', subdir='dummy_Fail.Error.subdir_2')
    invalidated_tests = {(orig_test.test, orig_test.subdir): orig_test
                         for orig_test in orig_tests}
    for retry in retry_tests:
        # It is possible that (retry.test, retry.subdir) doesn't exist
        # in invalidated_tests. This could happen when the original job
        # didn't run some of its tests. For example, a dut goes offline
        # since the beginning of the job, in which case invalidated_tests
        # will only have one entry for 'SERVER_JOB'.
        orig_test = invalidated_tests.get((retry.test, retry.subdir), None)
        if orig_test:
            retry.invalidates_test = orig_test
            retry.save()
    tko_utils.dprint('DEBUG: Invalidated tests associated to job: ' + msg)


def parse_one(db, jobname, path, reparse, mail_on_failure):
    """Parse a single job. Optionally send email on failure.

    @param db: database object.
    @param jobname: the tag used to search for existing job in db,
                    e.g. '1234-chromeos-test/host1'
    @param path: The path to the results to be parsed.
    @param reparse: True/False, whether this is reparsing of the job.
    @param mail_on_failure: whether to send email on FAILED test.


    """
    tko_utils.dprint("\nScanning %s (%s)" % (jobname, path))
    old_job_idx = db.find_job(jobname)
    # old tests is a dict from tuple (test_name, subdir) to test_idx
    old_tests = {}
    if old_job_idx is not None:
        if not reparse:
            tko_utils.dprint("! Job is already parsed, done")
            return

        raw_old_tests = db.select("test_idx,subdir,test", "tko_tests",
                                  {"job_idx": old_job_idx})
        if raw_old_tests:
            old_tests = dict(((test, subdir), test_idx)
                             for test_idx, subdir, test in raw_old_tests)

    # look up the status version
    job_keyval = models.job.read_keyval(path)
    status_version = job_keyval.get("status_version", 0)

    # parse out the job
    parser = status_lib.parser(status_version)
    job = parser.make_job(path)
    status_log = os.path.join(path, "status.log")
    if not os.path.exists(status_log):
        status_log = os.path.join(path, "status")
    if not os.path.exists(status_log):
        tko_utils.dprint("! Unable to parse job, no status file")
        return

    # parse the status logs
    tko_utils.dprint("+ Parsing dir=%s, jobname=%s" % (path, jobname))
    status_lines = open(status_log).readlines()
    parser.start(job)
    tests = parser.end(status_lines)

    # parser.end can return the same object multiple times, so filter out dups
    job.tests = []
    already_added = set()
    for test in tests:
        if test not in already_added:
            already_added.add(test)
            job.tests.append(test)

    # try and port test_idx over from the old tests, but if old tests stop
    # matching up with new ones just give up
    if reparse and old_job_idx is not None:
        job.index = old_job_idx
        for test in job.tests:
            test_idx = old_tests.pop((test.testname, test.subdir), None)
            if test_idx is not None:
                test.test_idx = test_idx
            else:
                tko_utils.dprint("! Reparse returned new test "
                                 "testname=%r subdir=%r" %
                                 (test.testname, test.subdir))
        for test_idx in old_tests.itervalues():
            where = {'test_idx' : test_idx}
            db.delete('tko_iteration_result', where)
            db.delete('tko_iteration_perf_value', where)
            db.delete('tko_iteration_attributes', where)
            db.delete('tko_test_attributes', where)
            db.delete('tko_test_labels_tests', {'test_id': test_idx})
            db.delete('tko_tests', where)

    # check for failures
    message_lines = [""]
    job_successful = True
    for test in job.tests:
        if not test.subdir:
            continue
        tko_utils.dprint("* testname, status, reason: %s %s %s"
                         % (test.subdir, test.status, test.reason))
        if test.status != 'GOOD':
            job_successful = False
            message_lines.append(format_failure_message(
                jobname, test.kernel.base, test.subdir,
                test.status, test.reason))

    message = "\n".join(message_lines)

    # send out a email report of failure
    if len(message) > 2 and mail_on_failure:
        tko_utils.dprint("Sending email report of failure on %s to %s"
                         % (jobname, job.user))
        mailfailure(jobname, job, message)

    # write the job into the database.
    db.insert_job(jobname, job,
                  parent_job_id=job_keyval.get(constants.PARENT_JOB_ID, None))

    # Upload perf values to the perf dashboard, if applicable.
    for test in job.tests:
        perf_uploader.upload_test(job, test)

    # Although the cursor has autocommit, we still need to force it to commit
    # existing changes before we can use django models, otherwise it
    # will go into deadlock when django models try to start a new trasaction
    # while the current one has not finished yet.
    db.commit()

    # Handle retry job.
    orig_afe_job_id = job_keyval.get(constants.RETRY_ORIGINAL_JOB_ID, None)
    if orig_afe_job_id:
        orig_job_idx = tko_models.Job.objects.get(
                afe_job_id=orig_afe_job_id).job_idx
        _invalidate_original_tests(orig_job_idx, job.index)

    # Serializing job into a binary file
    try:
        from autotest_lib.tko import tko_pb2
        from autotest_lib.tko import job_serializer

        serializer = job_serializer.JobSerializer()
        binary_file_name = os.path.join(path, "job.serialize")
        serializer.serialize_to_binary(job, jobname, binary_file_name)

        if reparse:
            site_export_file = "autotest_lib.tko.site_export"
            site_export = utils.import_site_function(__file__,
                                                     site_export_file,
                                                     "site_export",
                                                     _site_export_dummy)
            site_export(binary_file_name)

    except ImportError:
        tko_utils.dprint("DEBUG: tko_pb2.py doesn't exist. Create by "
                         "compiling tko/tko.proto.")

    db.commit()

    # Mark GS_OFFLOADER_NO_OFFLOAD in gs_offloader_instructions at the end of
    # the function, so any failure, e.g., db connection error, will stop
    # gs_offloader_instructions being updated, and logs can be uploaded for
    # troubleshooting.
    if job_successful:
        # Check if we should not offload this test's results.
        if job_keyval.get(constants.JOB_OFFLOAD_FAILURES_KEY, False):
            # Update the gs_offloader_instructions json file.
            gs_instructions_file = os.path.join(
                    path, constants.GS_OFFLOADER_INSTRUCTIONS)
            gs_offloader_instructions = {}
            if os.path.exists(gs_instructions_file):
                with open(gs_instructions_file, 'r') as f:
                    gs_offloader_instructions = json.load(f)

            gs_offloader_instructions[constants.GS_OFFLOADER_NO_OFFLOAD] = True
            with open(gs_instructions_file, 'w') as f:
                json.dump(gs_offloader_instructions, f)


def _site_export_dummy(binary_file_name):
    pass


def _get_job_subdirs(path):
    """
    Returns a list of job subdirectories at path. Returns None if the test
    is itself a job directory. Does not recurse into the subdirs.
    """
    # if there's a .machines file, use it to get the subdirs
    machine_list = os.path.join(path, ".machines")
    if os.path.exists(machine_list):
        subdirs = set(line.strip() for line in file(machine_list))
        existing_subdirs = set(subdir for subdir in subdirs
                               if os.path.exists(os.path.join(path, subdir)))
        if len(existing_subdirs) != 0:
            return existing_subdirs

    # if this dir contains ONLY subdirectories, return them
    contents = set(os.listdir(path))
    contents.discard(".parse.lock")
    subdirs = set(sub for sub in contents if
                  os.path.isdir(os.path.join(path, sub)))
    if len(contents) == len(subdirs) != 0:
        return subdirs

    # this is a job directory, or something else we don't understand
    return None


def parse_leaf_path(db, path, level, reparse, mail_on_failure):
    """Parse a leaf path.

    @param db: database handle.
    @param path: The path to the results to be parsed.
    @param level: Integer, level of subdirectories to include in the job name.
    @param reparse: True/False, whether this is reparsing of the job.
    @param mail_on_failure: whether to send email on FAILED test.

    @returns: The job name of the parsed job, e.g. '123-chromeos-test/host1'
    """
    job_elements = path.split("/")[-level:]
    jobname = "/".join(job_elements)
    try:
        db.run_with_retry(parse_one, db, jobname, path, reparse,
                          mail_on_failure)
    except Exception:
        traceback.print_exc()
    return jobname


def parse_path(db, path, level, reparse, mail_on_failure):
    """Parse a path

    @param db: database handle.
    @param path: The path to the results to be parsed.
    @param level: Integer, level of subdirectories to include in the job name.
    @param reparse: True/False, whether this is reparsing of the job.
    @param mail_on_failure: whether to send email on FAILED test.

    @returns: A set of job names of the parsed jobs.
              set(['123-chromeos-test/host1', '123-chromeos-test/host2'])
    """
    processed_jobs = set()
    job_subdirs = _get_job_subdirs(path)
    if job_subdirs is not None:
        # parse status.log in current directory, if it exists. multi-machine
        # synchronous server side tests record output in this directory. without
        # this check, we do not parse these results.
        if os.path.exists(os.path.join(path, 'status.log')):
            new_job = parse_leaf_path(db, path, level, reparse, mail_on_failure)
            processed_jobs.add(new_job)
        # multi-machine job
        for subdir in job_subdirs:
            jobpath = os.path.join(path, subdir)
            new_jobs = parse_path(db, jobpath, level + 1, reparse, mail_on_failure)
            processed_jobs.update(new_jobs)
    else:
        # single machine job
        new_job = parse_leaf_path(db, path, level, reparse, mail_on_failure)
        processed_jobs.add(new_job)
    return processed_jobs


def record_parsing(processed_jobs, duration_secs):
    """Record the time spent on parsing to metadata db.

    @param processed_jobs: A set of job names of the parsed jobs.
              set(['123-chromeos-test/host1', '123-chromeos-test/host2'])
    @param duration_secs: Total time spent on parsing, in seconds.
    """

    for job_name in processed_jobs:
        job_id, hostname = tko_utils.get_afe_job_id_and_hostname(job_name)
        if not job_id or not hostname:
            tko_utils.dprint('ERROR: can not parse job name %s, '
                             'will not send duration to metadata db.'
                             % job_name)
            continue
        else:
            job_overhead.record_state_duration(
                    job_id, hostname, job_overhead.STATUS.PARSING,
                    duration_secs)


def main():
    """Main entrance."""
    start_time = datetime.datetime.now()
    # Record the processed jobs so that
    # we can send the duration of parsing to metadata db.
    processed_jobs = set()

    options, args = parse_args()
    results_dir = os.path.abspath(args[0])
    assert os.path.exists(results_dir)

    pid_file_manager = pidfile.PidFileManager("parser", results_dir)

    if options.write_pidfile:
        pid_file_manager.open_file()

    try:
        # build up the list of job dirs to parse
        if options.singledir:
            jobs_list = [results_dir]
        else:
            jobs_list = [os.path.join(results_dir, subdir)
                         for subdir in os.listdir(results_dir)]

        # build up the database
        db = tko_db.db(autocommit=False, host=options.db_host,
                       user=options.db_user, password=options.db_pass,
                       database=options.db_name)

        # parse all the jobs
        for path in jobs_list:
            lockfile = open(os.path.join(path, ".parse.lock"), "w")
            flags = fcntl.LOCK_EX
            if options.noblock:
                flags |= fcntl.LOCK_NB
            try:
                fcntl.flock(lockfile, flags)
            except IOError, e:
                # lock is not available and nonblock has been requested
                if e.errno == errno.EWOULDBLOCK:
                    lockfile.close()
                    continue
                else:
                    raise # something unexpected happened
            try:
                new_jobs = parse_path(db, path, options.level, options.reparse,
                           options.mailit)
                processed_jobs.update(new_jobs)

            finally:
                fcntl.flock(lockfile, fcntl.LOCK_UN)
                lockfile.close()

    except:
        pid_file_manager.close_file(1)
        raise
    else:
        pid_file_manager.close_file(0)
    duration_secs = (datetime.datetime.now() - start_time).total_seconds()
    if options.record_duration:
        record_parsing(processed_jobs, duration_secs)


if __name__ == "__main__":
    main()
