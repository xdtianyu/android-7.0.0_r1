# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


def  verify(cr, histogram_name, histogram_bucket_value):
     """
     Verifies histogram string and success rate in a parsed histogram bucket.

     Full histogram URL is used to load histogram. Example Histogram URL is :
     chrome://histograms/Media.GpuVideoDecoderInitializeStatus
     @param cr: object, the Chrome instance
     @param histogram_name: string, name of the histogram
     @param histogram_bucket_value: int, required bucket number to look for
     @raises error.TestError if histogram is not successful

     """
     bucket_pattern = '\n'+ str(histogram_bucket_value) +'.*100\.0%.*'
     error_msg_format = ('{} not loaded or bucket not found '
                         'or bucket found at < 100%')
     tab = cr.browser.tabs.New()

     def loaded():
          """
          Checks if the histogram page has been fully loaded.

          """
          docEle = 'document.documentElement'
          tab.Navigate('chrome://histograms/%s' % histogram_name)
          tab.WaitForDocumentReadyStateToBeComplete()
          raw_text = tab.EvaluateJavaScript(
                  "{0} && {0}.innerText".format(docEle))
          return re.search(bucket_pattern, raw_text)

     msg = error_msg_format.format(histogram_name)
     utils.poll_for_condition(loaded,
                              exception=error.TestError(msg),
                              sleep_interval=1)


def is_bucket_present(cr,histogram_name, histogram_bucket_value):
     """
     This returns histogram succes or fail to called function

     @param cr: object, the Chrome instance
     @param histogram_name: string, name of the histogram
     @param histogram_bucket_value: int, required bucket number to look for
     @returns True if histogram page was loaded and the bucket was found.
              False otherwise

     """
     try:
          verify(cr,histogram_name, histogram_bucket_value)
     except error.TestError:
          return False
     else:
          return True
