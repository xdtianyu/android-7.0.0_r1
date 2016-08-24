#!/usr/bin/python -u
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Check if a json file is valid.

This wrapper is invoked through autotest's PRESUBMIT.cfg for every commit
that edits a json file.
"""

import json
import os


class InvalidJsonFile(Exception):
    """Exception to raise when a json file can't be parsed."""


def main():
    """Check if all json files that are a part of this commit are valid."""
    file_list = os.environ.get('PRESUBMIT_FILES')
    if file_list is None:
        raise InvalidJsonFile('Expected a list of presubmit files in '
                              'the PRESUBMIT_FILES environment variable.')

    for f in file_list.split():
        if f.lower().endswith('.json'):
            try:
                with open(f) as json_file:
                    json.load(json_file)
            except ValueError:
                # Re-raise the error to include the file path.
                print ('Presubmit check `check_json_file` failed. If the file '
                       'is meant to be malformated, please do not name it as a '
                       'json file, or you will have to upload the CL using '
                       '--no-verify')
                raise InvalidJsonFile('Invalid json file: %s' % f)


if __name__ == '__main__':
    main()
