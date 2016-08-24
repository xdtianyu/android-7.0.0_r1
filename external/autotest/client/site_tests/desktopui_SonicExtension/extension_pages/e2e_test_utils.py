# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The extensions e2e test utils page."""

from extension_pages import ExtensionPages


class E2ETestUtilsPage(ExtensionPages):
    """Contains all the functions of the options page of the extension."""

    def __init__(self, driver, extension_id):
        """Constructor."""
        self._test_utils_url = ('chrome-extension://%s/e2e_test_utils.html' %
                                extension_id)
        ExtensionPages.__init__(self, driver, self._test_utils_url)


    def stop_activity_id_text_box(self):
        """The text box for entering the stop activity ID."""
        return self._get_text_box('activityIdToStop', 'stop_activity_id')


    def receiver_ip_or_name_text_box(self):
        """The text box for entering the receiver IP address/name."""
        return self._get_text_box('receiverIpAddress', 'receiver_ip_name')


    def url_to_open_text_box(self):
      """The text box for entering the URL to mirror."""
      return self._get_text_box('urlToOpen', 'url_to_open')


    def receiver_ip_or_name_v2_text_box(self):
        """The text box for entering the receiver IP address/name."""
        return self._get_text_box('receiverIpAddressV2', 'receiver_ip_name_v2')


    def url_to_open_v2_text_box(self):
        """The text box for entering the URL to mirror."""
        return self._get_text_box('urlToOpenV2', 'url_to_open_v2')


    def udp_proxy_server_text_box(self):
        """The text box for entering the udp proxy server address."""
        return self._get_text_box('udpProxyServer', 'udp_proxy_server')


    def network_profile_text_box(self):
        """The text box for entering the network profile."""
        return self._get_text_box('networkProfile', 'network_profile')


    def get_webrtc_stats_button(self):
        """The get webrtc stats button."""
        return self._get_button('getWebRtcStats', 'get_webrtc_stats')


    def get_mirror_id_button(self):
        """The get mirror id button."""
        return self._get_button('getMirrorId', 'get_mirror_id')


    def stop_all_activities_button(self):
      """The stop all activities button."""
      return self._get_button('stopAllActivities', 'stop_all_activities')


    def stop_activity_button(self):
        """The stop activity button."""
        return self._get_button('stopActivityById', 'stop_activity')


    def open_then_mirror_button(self):
        """The open then mirror button for v1 mirroring."""
        return self._get_button('mirrorUrl', 'open_then_mirror')


    def open_then_mirror_v2_button(self):
        """The open then mirror button for v2 mirroring."""
        return self._get_button('mirrorUrlV2', 'open_then_mirror_v2')


    def start_desktop_mirror_button(self):
        """The start desktop mirror button."""
        return self._get_button('mirrorDesktop','start_desktop_mirror')


    def webrtc_stats_scroll_box(self):
        """The scroll box that shows all the webrtc stats log."""
        return self._get_scroll_box('webrtc_stats', 'webrtc_stats')


    def mirror_id_web_element_box(self):
        """The box that shows the activity ID if there is any."""
        return self._get_web_element_box('mirrorId', 'mirror_id')


    def stop_v2_mirroring_button(self):
        """The button to stop v2 mirroring."""
        return self._get_button('stopV2Mirroring', 'stop_v2_mirroring')


    def get_v2_stats_button(self):
        """The button to get v2 mirroring stats."""
        return self._get_button(
                'getV2MirroringStats', 'get_v2_mirroring_stats')


    def upload_v2_mirroring_logs_button(self):
        """The button to upload v2 mirroring logs."""
        return self._get_button(
                'uploadV2MirroringLogs', 'upload_v2_mirroring_logs')


    def v2_mirroring_stats_scroll_box(self):
        """The scroll box that shows the v2 mirroring stats."""
        return self._get_scroll_box(
                'v2_mirroring_stats', 'v2_mirroring_stats')


    def v2_mirroring_logs_scroll_box(self):
        """The scroll box that shows v2 mirroring logs data url."""
        return self._get_scroll_box(
                'v2_mirroring_logs_report_id', 'v2_mirroring_logs_report_id')
