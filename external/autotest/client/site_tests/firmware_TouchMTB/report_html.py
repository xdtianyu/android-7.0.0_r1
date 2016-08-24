# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Touch firmware test report in html format."""

import os
import urllib

import common_util
import firmware_log
import test_conf as conf

from firmware_utils import get_fw_and_date
from string import Template
from validators import get_base_name_and_segment


class TemplateHtml:
    """An html Template."""

    def __init__(self, image_width, image_height, score_colors):
        self.score_colors = score_colors

        # Define the template of the doc
        self.doc = Template('$head $test_version $logs $tail')
        self.table = Template('<table border="3" width="100%"> $gestures '
                              '</table>')
        self.gestures = []

        # Define a template to show a gesture information including
        # the gesture name, variation, prompt, image, and test results.
        self.gesture_template = Template('''
            <tr>
                <td><table>
                    <tr>
                        <h3> $gesture_name.$variation </h3>
                        <h5> $prompt </h5>
                    </tr>
                    <tr>
                        <img src="data:image/png;base64,\n$image"
                            alt="$filename" width="%d" height="%d" />
                    </tr>
                </table></td>
                <td><table>
                    $vlogs
                </table></td>
            </tr>
        ''' % (image_width, image_height))

        self.criteria_string = '  criteria: %s'
        self.validator_template =  Template('''
            <tr>
<pre><span style="color:$color"><b>$name</b></span>
$details
$criteria
</pre>
            </tr>
        ''')

        self.detail_template =  Template('<tr><h5> $detail </h5></tr>')
        self._fill_doc()

    def _html_head(self):
        """Fill the head of an html document."""
        head = '\n'.join(['<!DOCTYPE html>', '<html>', '<body>'])
        return head

    def _html_tail(self):
        """Fill the tail of an html document."""
        tail = '\n'.join(['</body>', '</html>'])
        return tail

    def _fill_doc(self):
        """Fill in fields into the doc."""
        self.doc = Template(self.doc.safe_substitute(head=self._html_head(),
                                                     tail=self._html_tail()))

    def get_score_color(self, score):
        """Present the score in different colors."""
        for s, c in self.score_colors:
            if score >= s:
                return c

    def _insert_details(self, details):
        details_content = []
        for detail in details:
            details_content.append(' ' * 2 + detail.strip())
        return '<br>'.join(details_content)

    def _insert_vlog(self, vlog):
        """Insert a single vlog."""
        base_name, _ = get_base_name_and_segment(vlog.name)
        criteria_string = self.criteria_string % vlog.criteria
        vlog_content = self.validator_template.safe_substitute(
                name=vlog.name,
                details=self._insert_details(vlog.details),
                criteria=criteria_string,
                color='blue',
                score=vlog.score)
        return vlog_content

    def _insert_vlogs(self, vlogs):
        """Insert multiple vlogs."""
        vlogs_content = []
        for vlog in vlogs:
            vlogs_content.append(self._insert_vlog(vlog))
        return '<hr>'.join(vlogs_content)

    def insert_gesture(self, glog, image, image_filename):
        """Insert glog, image, and vlogs."""
        vlogs_content = self._insert_vlogs(glog.vlogs)
        gesture = self.gesture_template.safe_substitute(
                gesture_name=glog.name,
                variation=glog.variation,
                prompt=glog.prompt,
                image=image,
                filename=image_filename,
                vlogs=vlogs_content)
        self.gestures.append(gesture)

    def get_doc(self, test_version):
        gestures = ''.join(self.gestures)
        new_table = self.table.safe_substitute(gestures=gestures)
        new_doc = self.doc.safe_substitute(test_version=test_version,
                                           logs=new_table)
        return new_doc


class ReportHtml:
    """Firmware Report in html format."""

    def __init__(self, filename, screen_size, touch_device_window_size,
                 score_colors, test_version):
        self.html_filename = filename
        self.screen_size = screen_size
        self.image_width = self.screen_size[0] * 0.5
        touch_width, touch_height = touch_device_window_size
        self.image_height = self.image_width / touch_width * touch_height
        self.doc = TemplateHtml(self.image_width, self.image_height,
                                score_colors)
        self._reset_content()
        self.test_version = test_version
        fw_and_date = get_fw_and_date(filename)
        self.rlog = firmware_log.RoundLog(test_version, *fw_and_date)

    def __del__(self):
        self.stop()

    def stop(self):
        """Close the file."""
        with open(self.html_filename, 'w') as report_file:
            report_file.write(self.doc.get_doc(self.test_version))
        # Make a copy to /tmp so that it could be viewed in Chrome.
        tmp_copy = os.path.join(conf.docroot,
                                os.path.basename(self.html_filename))
        copy_cmd = 'cp %s %s' % (self.html_filename, tmp_copy)
        common_util.simple_system(copy_cmd)

        # Dump the logs to a byte stream file
        log_file_root = os.path.splitext(self.html_filename)[0]
        log_filename = os.extsep.join([log_file_root, 'log'])
        self.rlog.dump(log_filename)

    def _reset_content(self):
        self.glog = firmware_log.GestureLog()
        self.encoded_image=''
        self.image_filename=''

    def _get_content(self):
        return [self.glog, self.encoded_image, self.image_filename]

    def _encode_base64(self, filename):
        """Encode a file in base 64 format."""
        if (filename is None) or (not os.path.isfile(filename)):
            return None
        encoded = urllib.quote(open(filename, "rb").read().encode("base64"))
        return encoded

    def flush(self):
        """Flush the current gesture including gesture log, image and
        validator logs.
        """
        content = self._get_content()
        # It is ok to flush the gesture log even when there are no mtplot images
        if self.glog:
            # Write the content to the html file.
            self.doc.insert_gesture(*content)
            # Write the logs to the round log.
            self.rlog.insert_glog(self.glog)
            self._reset_content()

    def insert_image(self, filename):
        """Insert an image into the document."""
        self.encoded_image = self._encode_base64(filename)
        self.image_filename = filename

    def insert_result(self, text):
        """Insert the text into the document."""
        self.result += text

    def insert_gesture_log(self, glog):
        """Update the gesture log."""
        self.glog = glog

    def insert_validator_logs(self, vlogs):
        """Update the validator logs."""
        self.glog.vlogs = vlogs
