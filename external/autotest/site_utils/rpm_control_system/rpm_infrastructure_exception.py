# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

class RPMInfrastructureException(Exception):
    """
    Exception used to indicate infrastructure failures in the RPM control
    system.
    """
    pass


class RPMLoggingSetupError(RPMInfrastructureException):
    """Rasied when setup logging fails."""
    pass
