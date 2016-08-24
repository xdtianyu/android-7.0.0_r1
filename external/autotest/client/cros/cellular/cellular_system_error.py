# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class CellularSystemError(Exception):
    """
    """
    pass


class BadState(CellularSystemError):
    """
    Indicates the state of the test system is unexpected. For example:
    when the call drops unexpectedly; or setting a call-box to Band 13, but
    a Band? query produces 14.
    """
    pass


class BadScpiCommand(CellularSystemError):
    """
    Indicates the SCPI command was rejected by the instrument, or that the
    SCPI query did not return a valid result.
    """
    pass

class ConnectionFailure(CellularSystemError):
    """
    Indicates a connection failure with the cellular network used for the test.
    For example, the cellular network was not found, an unexpected connection
    drop happened, etc.
    """
    pass

class SocketTimeout(CellularSystemError):
    """
    Indicates the socket component of a connection exceeded a time limit
    without a response. This happens when the socket is closed unexpectedly,
    if if there are network problems with reaching the remote. This usually
    does not provide information about the instrument, this failure happens
    before the instrument gets the command.
    """
    pass


class InstrumentTimeout(CellularSystemError):
    """
    Indicates a working communication channel to the instrument, and a valid
    command, but the command took too long to run.
    """
    pass


class ChromebookHardwareTimeout(CellularSystemError):
    """
    Indicates an operation took longer then the time allowed. This can
    be used for cellular GPIB instruments or cellular hardware on the
    Chromebook.
    """
    pass
