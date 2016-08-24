# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This implements the APIs defined at
# https://github.com/luci/luci-py/blob/master/appengine/
# swarming/swarming_bot/bot_config.py


"""This file is meant to be overriden by the server's specific copy.

You can upload a new version via /restricted/upload/bot_config.

There's 3 types of functions in this file:
  - get_*() to return properties to describe this bot.
  - on_*() as hooks based on events happening on the bot.
  - setup_*() to setup global state on the host.

This file shouldn't import from other scripts in this directory except
os_utilities which is guaranteed to be usable as an API. It's fine to import
from stdlib.

Set the environment variable SWARMING_LOAD_TEST=1 to disable the use of
server-provided bot_config.py. This permits safe load testing.

TODO(fdeng):
    Restrict the command to run_suite/abort_suite
"""

import json
import re
import os

from api import os_utilities

# Unused argument 'bot' - pylint: disable=W0613


CMD_WHITELIST = {'/usr/local/autotest/site_utils/run_suite.py',
                 '/usr/local/autotest/site_utils/abort_suite.py'}


def get_dimensions(bot=None):
    """Returns dict with the bot's dimensions.

    The dimensions are what are used to select the bot that can run each task.

    By default, the bot id will be automatically selected based on
    the hostname with os_utilities.get_dimensions(). This method
    overrides the default id returned by os_utilities.get_dimensions().

    Assume the bot's working directory is like BOT_ROOT/bot_23/
    we will parse the id "23" from the directory name and append it to the
    hostname to form the bot id. so the bot id would look like
    chromeos-server31-23

    See https://github.com/luci/luci-py/blob/master/appengine/
    swarming/doc/Magic-Values.md

    @returns: Dict with the bot's dimentions.

    """
    d = os_utilities.get_dimensions()
    m = re.match('.*/bot_([\d]+).*', os.getcwd())
    suffix = ''
    if m:
        suffix = '-'+ m.group(1)
    d[u'id'] = [os_utilities.get_hostname_short() + suffix]
    return d


def get_state(bot=None):
    """Returns dict with a state of the bot reported to the server with each poll.

    It is only for dynamic state that changes while bot is running for information
    for the sysadmins.

    The server can not use this state for immediate scheduling purposes (use
    'dimensions' for that), but it can use it for maintenance and bookkeeping
    tasks.

    See https://github.com/luci/luci-py/blob/master/appengine/
    swarming/doc/Magic-Values.md

    """
    return os_utilities.get_state()


### Hooks


def on_before_task(bot):
    """Hook function called before running a task.

    It shouldn't do much, since it can't cancel the task so it shouldn't do
    anything too fancy.

    @param bot: bot.Bot instance.
    """
    # TODO(fdeng): it is possible that the format gets updated
    # without warning. It would be better to find a long term solution.
    work_dir = os.path.join(bot.base_dir, 'work')
    path = os.path.join(work_dir, 'task_runner_in.json')
    manifest = {}
    with open(path) as f:
        manifest = json.load(f)
    full_command = manifest.get('command')
    if full_command and not full_command[0] in CMD_WHITELIST:
        # override the command with a safe "echo"
        manifest['command'] = ['echo', '"Command not allowed"']
        manifest['data'] = []
        with open(path, 'wb') as f:
            f.write(json.dumps(manifest))
        raise Exception('Command not allowed: %s' % full_command)


### Setup


def setup_bot(bot):
    """Does one time initialization for this bot.

    Returns True if it's fine to start the bot right away. Otherwise, the calling
    script should exit.

    Example: making this script starts automatically on user login via
    os_utilities.set_auto_startup_win() or os_utilities.set_auto_startup_osx().

    @param bot: bot.Bot instance.

    @returns: Boolean. See above.

    """
    return True
