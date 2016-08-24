# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging


def map_afe_board_to_servo_board(afe_board):
    """Map a board we get from the AFE to a servo appropriate value.

    Many boards are identical to other boards for servo's purposes.
    This function makes that mapping.

    @param afe_board string board name received from AFE.
    @return board we expect servo to have.

    """
    KNOWN_SUFFIXES = ['_freon', '_moblab', '-cheets']
    BOARD_MAP = {'gizmo': 'panther'}
    mapped_board = afe_board
    if afe_board in BOARD_MAP:
        mapped_board = BOARD_MAP[afe_board]
    else:
        for suffix in KNOWN_SUFFIXES:
            if afe_board.endswith(suffix):
                mapped_board = afe_board[0:-len(suffix)]
                break
    if mapped_board != afe_board:
        logging.info('Mapping AFE board=%s to %s', afe_board, mapped_board)
    return mapped_board
