# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.   1
# Use of this source code is governed by a BSD-style license that can be   2
# found in the LICENSE file.

"Module containing common utilities for server-side autoupdate tests."

import logging

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import autoupdater, dev_server
from autotest_lib.server.cros.dynamic_suite import tools


def get_updater_from_repo_url(host, job_repo_url=None):
    """Returns the autoupdater instance to use for a given autoupdate test.

    All server-side tests that run in the lab have an associated job_repo_url
    assigned to their host that is associated with the version of the build that
    is currently installed on them. Given most autoupdate tests need to
    update to some build as part of the test, we conveniently re-update to the
    same version installed. This method serves as a helper to get the
    instantiated autoupdater instance for that build.

    This method guarantees that the devserver associated with the autoupdater
    has already staged the necessary files for autoupdate.

    @param host: The host for the DUT of the server-side test.
    @param job_repo_url: If set, the job_repo_url to use.

    @raise error.TestError: If we fail to get a job_repo_url.
    """
    # Get the job_repo_url -- if not present, attempt to use the one
    # specified in the host attributes for the host.
    if not job_repo_url:
        try:
            job_repo_url = host.lookup_job_repo_url()
        except KeyError:
            logging.fatal('Could not lookup job_repo_url from afe.')

        if not job_repo_url:
            raise error.TestError(
                    'Could not find a job_repo_url for the given host.')

    # Get the devserver url and build (image) from the repo url e.g.
    # 'http://mydevserver:8080', 'x86-alex-release/R27-123.0.0'
    ds_url, build = tools.get_devserver_build_from_package_url(job_repo_url)
    devserver = dev_server.ImageServer(ds_url)
    devserver.stage_artifacts(build, ['full_payload', 'stateful'])

    # We only need to update stateful to do this test.
    return autoupdater.ChromiumOSUpdater(devserver.get_update_url(build),
                                         host=host)
