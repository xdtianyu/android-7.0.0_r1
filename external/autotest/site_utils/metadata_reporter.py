# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module is designed to report metadata in a separated thread to avoid the
performance overhead of sending data to Elasticsearch using HTTP.

"""

import logging
import Queue
import time
import threading

import common
from autotest_lib.client.common_lib.cros.graphite import autotest_es
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.scheduler import email_manager
# The metadata_reporter thread runs inside scheduler process, thus it doesn't
# need to setup django, otherwise, following import is needed:
# from autotest_lib.frontend import setup_django_environment
from autotest_lib.site_utils import server_manager_utils


# Number of seconds to wait before checking queue again for uploading data.
_REPORT_INTERVAL_SECONDS = 5

_MAX_METADATA_QUEUE_SIZE = 1000000
_MAX_UPLOAD_SIZE = 50000
# The number of seconds for upload to fail continuously. After that, upload will
# be limited to 1 entry.
_MAX_UPLOAD_FAIL_DURATION = 600
# Number of entries to retry when the previous upload failed continueously for
# the duration of _MAX_UPLOAD_FAIL_DURATION.
_MIN_RETRY_ENTRIES = 10
# Queue to buffer metadata to be reported.
metadata_queue = Queue.Queue(_MAX_METADATA_QUEUE_SIZE)

_report_lock = threading.Lock()
_abort = threading.Event()
_queue_full = threading.Event()

def queue(data):
    """Queue metadata to be uploaded in reporter thread.

    If the queue is full, an error will be logged for the first time the queue
    becomes full. The call does not wait or raise Queue.Full exception, so
    there is no overhead on the performance of caller, e.g., scheduler.

    @param data: A metadata entry, which should be a dictionary.
    """
    try:
        metadata_queue.put_nowait(data)
        if _queue_full.is_set():
            logging.info('Metadata queue is available to receive new data '
                         'again.')
            _queue_full.clear()
    except Queue.Full:
        if not _queue_full.is_set():
            _queue_full.set()
            logging.error('Metadata queue is full, cannot report data. '
                          'Consider increasing the value of '
                          '_MAX_METADATA_QUEUE_SIZE. Its current value is set '
                          'to %d.', _MAX_METADATA_QUEUE_SIZE)


def _email_alert():
    """
    """
    if not server_manager_utils.use_server_db():
        logging.debug('Server database not emailed, email alert is skipped.')
        return
    try:
        server_manager_utils.confirm_server_has_role(hostname='localhost',
                                                     role='scheduler')
    except server_manager_utils.ServerActionError:
        # Only email alert if the server is a scheduler, not shard.
        return
    subject = ('Metadata upload has been failing for %d seconds' %
               _MAX_UPLOAD_FAIL_DURATION)
    email_manager.manager.enqueue_notify_email(subject, '')
    email_manager.manager.send_queued_emails()


def _run():
    """Report metadata in the queue until being aborted.
    """
    # Time when the first time upload failed. None if the last upload succeeded.
    first_failed_upload = None
    # True if email alert was sent when upload has been failing continuously
    # for _MAX_UPLOAD_FAIL_DURATION seconds.
    email_alert = False
    upload_size = _MIN_RETRY_ENTRIES
    try:
        while True:
            start_time = time.time()
            data_list = []
            if (first_failed_upload and
                time.time() - first_failed_upload > _MAX_UPLOAD_FAIL_DURATION):
                upload_size = _MIN_RETRY_ENTRIES
                if not email_alert:
                    _email_alert()
                    email_alert = True
            else:
                upload_size = min(upload_size*2, _MAX_UPLOAD_SIZE)
            while (not metadata_queue.empty() and len(data_list) < upload_size):
                data_list.append(metadata_queue.get_nowait())
            if data_list:
                if autotest_es.bulk_post(data_list=data_list):
                    time_used = time.time() - start_time
                    logging.info('%d entries of metadata uploaded in %s '
                                 'seconds.', len(data_list), time_used)
                    autotest_stats.Timer('metadata_reporter').send(
                            'time_used', time_used)
                    autotest_stats.Gauge('metadata_reporter').send(
                            'entries_uploaded', len(data_list))
                    first_failed_upload = None
                    email_alert = False
                else:
                    logging.warn('Failed to upload %d entries of metadata, '
                                 'they will be retried later.', len(data_list))
                    autotest_stats.Gauge('metadata_reporter').send(
                            'entries_failed', len(data_list))
                    for data in data_list:
                        queue(data)
                    if not first_failed_upload:
                        first_failed_upload = time.time()
            sleep_time = _REPORT_INTERVAL_SECONDS - time.time() + start_time
            if sleep_time < 0:
                sleep_time = 0.5
            _abort.wait(timeout=sleep_time)
    except Exception as e:
        logging.error('Metadata reporter thread failed with error: %s', e)
        raise
    finally:
        logging.info('Metadata reporting thread is exiting.')
        _abort.clear()
        _report_lock.release()


def start():
    """Start the thread to report metadata.
    """
    # The lock makes sure there is only one reporting thread working.
    if _report_lock.locked():
        logging.error('There is already a metadata reporter thread.')
        return

    _report_lock.acquire()
    reporting_thread = threading.Thread(target=_run)
    # Make it a daemon thread so it doesn't need to be closed explicitly.
    reporting_thread.setDaemon(True)
    reporting_thread.start()
    logging.info('Metadata reporting thread is started.')


def abort():
    """Abort the thread to report metadata.

    The call will wait up to 5 seconds for existing data to be uploaded.
    """
    if  not _report_lock.locked():
        logging.error('The metadata reporting thread has already exited.')
        return

    _abort.set()
    logging.info('Waiting up to %s seconds for metadata reporting thread to '
                 'complete.', _REPORT_INTERVAL_SECONDS)
    _abort.wait(_REPORT_INTERVAL_SECONDS)
