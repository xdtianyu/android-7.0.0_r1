# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Sequenced feedback request."""

from __future__ import print_function

import input_handlers
import request
import textwrap


class _RequestAction(object):
    """An interface of a single action in a sequenced feedback request."""

    def execute(self):
        """Performs the action."""
        raise NotImplementedError


class _QuestionRequestAction(_RequestAction):
    """A question to be presented to a user."""

    def __init__(self, blurb, input_handler, prompt=None):
        self.input_handler = input_handler
        blurb_supp, prompt_supp = input_handler.get_choices_supplements()

        # Initialize the question blurb string.
        self.blurb = self._format_text(blurb)
        if blurb_supp:
            self.blurb += '\n' + blurb_supp

        # Initialize the input prompt string.
        if prompt is None:
            prompt = ''
        if prompt_supp:
            if prompt:
                prompt += ' '
            prompt += prompt_supp
        self.prompt = self._format_text(prompt)
        if self.prompt:
            self.prompt += ' '

    def _format_text(self, text):
        """Formats a blob of text for writing to standard output."""
        return textwrap.fill(text.strip())


    def execute(self):
        if self.blurb:
            print(self.blurb, end=('\n' if self.prompt else ' '))
        while True:
            try:
                return self.input_handler.process(raw_input(self.prompt))
            except input_handlers.InputError:
                print('Invalid input, try again')


class SequencedFeedbackRequest(request.FeedbackRequest):
    """A request consisting of a sequence of interactive actions."""

    def __init__(self, *args):
        super(SequencedFeedbackRequest, self).__init__(*args)
        self._actions = []


    def _append_action(self, action):
        self._actions.append(action)


    def append_question(self, blurb, input_handler, prompt=None):
        """Appends a question to the request sequence.

        @param blurb: The text to print prior to asking for input.
        @param input_handler: The input handler object.
        @param prompt: String to print when polling for input.
        """
        attrs = {'test': self.test, 'dut': self.dut}
        blurb = blurb or ''
        self._append_action(_QuestionRequestAction(
                blurb % attrs, input_handler,
                prompt=(prompt and prompt % attrs)))


    def execute(self):
        """Executes the sequence of request actions.

        @return: The return value of the last action.

        @raise request.FeedbackRequestError: Failed during sequence execution.
        """
        ret = None
        for action in self._actions:
            ret = action.execute()
        return ret
