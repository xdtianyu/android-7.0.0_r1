#!/usr/bin/python -u
"""
Wrapper to run 'suite_scheduler.py --sanity' before uploading a patch.
This script is invoked through PRESUBMIT.cfg from repohooks, and expects a
list of commit files in the environment variable.
"""

import os, re, sys
import common
from autotest_lib.client.common_lib import utils


def _commit_contains_ini_or_control():
    """
    Checks if commit contains suite_scheduler.ini or a control file.

    @return: True if one of the files in the commit is suite_scheduler.ini.
    """
    file_list = os.environ.get('PRESUBMIT_FILES')
    if file_list is None:
        print 'Expected a list of presubmit files in the environment variable.'
        sys.exit(1)

    pattern = re.compile(r'.*files/suite_scheduler.ini$|.*/control(?:\.\w+)?$')
    return any (pattern.search(file_path)
                for file_path in file_list.split('\n'))


def main():
    """
    Main function, invokes suite scheduler's sanity checker if the
    commit contains either suite_scheduler.ini or a control file.
    """
    if _commit_contains_ini_or_control():
        site_utils_dir = os.path.dirname(
                             os.path.dirname(os.path.abspath(__file__)))
        suite_scheduler = os.path.join(site_utils_dir,
                                       'suite_scheduler/suite_scheduler.py')
        output = utils.system_output(suite_scheduler + ' --sanity')
        if output:
            print output
            sys.exit(1)


if __name__ == '__main__':
    main()
