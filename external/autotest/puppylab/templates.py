# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Templates for results a test can emit.
"""

# TODO(beeps): The right way to create these status logs is by creating a job
# object and invoking job.record on it. However we perform this template
# hack instead for the following reasons:
#   * The templates are a lot easier to understand at first glance, which
#     is really what one wants from a testing interface built for a
#     framework like autotest.
#   * Creating the job is wedged into core autotest code, so it has
#     unintended consequences like checking for hosts/labels etc.
#   * We are guaranteed to create the bare minimum by hand specifying the file
#     to write, and their contents. Job.record ends up checking and creating
#     several non-essential directoris in the process or recording status.

success_test_template = (
        "\tSTART\t%(test_name)s\t%(test_name)s"
        "\ttimestamp=%(timestamp)s\tlocaltime=%(date)s\n"
        "\t\tGOOD\t%(test_name)s\t%(test_name)s\ttimestamp=%(timestamp)s\t"
        "localtime=%(date)s\tcompleted successfully\n"
        "\tEND GOOD\t%(test_name)s\t%(test_name)s\ttimestamp=%(timestamp)s\t"
        "localtime=%(date)s")


success_job_template = (
        "START\t----\t----\ttimestamp=%(timestamp)s\tlocaltime=%(date)s"
        "\n\tSTART\t%(test_name)s\t%(test_name)s\ttimestamp=%(timestamp)s\t"
        "localtime=%(date)s\n\t\tGOOD\t%(test_name)s\t%(test_name)s"
        "\ttimestamp=%(timestamp)s\tlocaltime=%(date)s\tcompleted "
        "successfully\n\tEND GOOD\t%(test_name)s\t%(test_name)s"
        "\ttimestamp=%(timestamp)s\tlocaltime=%(date)s\n"
        "END GOOD\t----\t----\ttimestamp=%(timestamp)s\tlocaltime=%(date)s")


job_keyvals_template = "hostname=%(hostname)s\nstatus_version=1\n"

