"""
This module contains the status enums for use by HostQueueEntrys in the
database.  It is a stand alone module as these status strings are needed
from various disconnected pieces of code that should not depend on everything
that frontend.afe.models depends on such as RPC clients.
"""

from autotest_lib.client.common_lib import enum

Status_list = ['Queued', 'Starting', 'Resetting', 'Verifying', 'Provisioning',
               'Pending', 'Waiting', 'Running', 'Gathering', 'Parsing',
               'Archiving', 'Aborted', 'Completed', 'Failed', 'Stopped',
               'Cleaning', 'Template']

Status = enum.Enum(*Status_list, string_values=True)
ACTIVE_STATUSES = (Status.STARTING, Status.RESETTING, Status.VERIFYING,
                   Status.PROVISIONING, Status.PENDING, Status.RUNNING,
                   Status.GATHERING, Status.CLEANING)
COMPLETE_STATUSES = (Status.ABORTED, Status.COMPLETED, Status.FAILED,
                     Status.STOPPED, Status.TEMPLATE)

IntStatus = enum.Enum(*Status_list)
