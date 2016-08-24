# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""User input handlers."""


class InputError(Exception):
    """An error with a user provided input."""


class _InputHandler(object):
    """An input handler base class."""

    def get_choices_supplements(self):
        """Returns a pair of supplement strings representing input choices.

        @return: A pair consisting of a detailed representation of available
                 choices, and a corresponding concise descriptor of available
                 input choices with optional default.
        """
        input_choices, default = self._get_input_choices_and_default()
        if input_choices:
            input_choices = '[%s]' % input_choices
            if default is not None:
                input_choices += ' (default: %s)' % default

        return self._get_choices_details(), input_choices


    def _get_input_choices_and_default(self):
        """Returns an input choices descriptor and a default (if any)."""
        raise NotImplementedError


    def _get_choices_details(self):
        """Returns a detailed description (string) of input choices."""
        raise NotImplementedError


    def process(self, input_str):
        """Returns the result of processing the user input.

        @param input_str: The user input.

        @return: The result of processing the input.

        @raise InputError: Provided input is invalid.
        """
        raise NotImplementedError


class PauseInputHandler(_InputHandler):
    """A quiet input handler that just returns on any input."""

    # Interface overrides.
    #
    def _get_input_choices_and_default(self):
        return None, None


    def _get_choices_details(self):
        return None


    def process(self, input_str):
        pass


class YesNoInputHandler(_InputHandler):
    "A yes/no input handler with optional default."""

    def __init__(self, default=None):
        """Initializes the input handler.

        @param default: The Boolean value to return by default.
        """
        self._default = default
        self._input_choices = '%s/%s' % ('Y' if default is True else 'y',
                                         'N' if default is False else 'n')


    # Interface overrides.
    #
    def _get_input_choices_and_default(self):
        # We highlight the default by uppercasing the corresponding choice
        # directly, so no need to return a default separately.
        return self._input_choices, None


    def _get_choices_details(self):
        return None


    def process(self, input_str):
        input_str = input_str.lower().strip()
        if input_str == 'y':
            return True
        if input_str == 'n':
            return False
        if not input_str and self._default is not None:
            return self._default
        raise InputError


class MultipleChoiceInputHandler(_InputHandler):
    """A multiple choice input handler with optional default."""

    def __init__(self, choices, default=None):
        """Initializes the input handler.

        @param choices: An iterable of input choices.
        @param default: Index of default choice (integer).
        """
        max_idx = len(choices)
        if not (default is None or default in range(1, max_idx + 1)):
            raise ValueError('Default choice is not a valid index')
        self._choices = choices
        self._idx_range = '1-%d' % max_idx if max_idx > 1 else str(max_idx)
        self._default = None if default is None else str(default)


    # Interface overrides.
    #
    def _get_input_choices_and_default(self):
        return self._idx_range, self._default


    def _get_choices_details(self):
        return '\n'.join(['%d) %s' % (idx, choice)
                          for idx, choice in enumerate(self._choices, 1)])


    def process(self, input_str):
        """Returns the index (zero-based) and value of the chosen option."""
        input_str = input_str or self._default
        if input_str:
            try:
                input_idx = int(input_str) - 1
                if input_idx in range(len(self._choices)):
                    return input_idx, self._choices[input_idx]
            except ValueError:
                pass

        raise InputError
