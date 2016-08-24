# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import error, logging, os, serial, shutil, threading, time

_power_play_data_file = '/tmp/power_play_data'

class PowerPlay(object):
    """Class to record serial over USB data from Power Play (go/powerplay).

    It detects if powerplay is connected to the DUT over USB and opens the
    serial port to start receiving powerplay data. It also opens a text file to
    save this data after some formatting.
    """

    version = 1

    def __init__(self, test_obj, record_interval=0):
        """Initialize PowerPlay.

        @param test_obj: test object.
        @param record_interval: Power play data recording interval in seconds.
        """
        self.test = test_obj
        self.ser = None
        self.recording_interval = record_interval
        self.momentary_curr_list = list()
        self.record_thread = None

    def extract_current(self, pp_data):
        """Extract momentary current value from each line of powerplay data.

        @param pp_data: Single line of powerplay data with eight comma separated
                        values.
        @return list containing momentary current values.
        """
        if pp_data[0].isdigit():
            self.momentary_curr_list.append(float(pp_data[pp_data.index(',')+1:]
                    [:pp_data[pp_data.index(',')+1:].index(',')]))
        return self.momentary_curr_list

    def start_recording_power_play_data(self):
        """Starts a new thread to record power play data."""
        self.record_thread = threading.Thread(target=self.start_record_thread)
        self.record_thread.daemon = True
        self.record_thread.start()

    def start_record_thread(self):
        """Start recording power play data.

        Get a list of connected USB devices and try to establish a serial
        connection. Once the connection is established, open a text file and
        start reading serial data and write it to the text file after some
        formatting.
        """
        devices = [x for x in os.listdir('/dev/') if x.startswith('ttyUSB')]

        for device in devices:
            device_link = '/dev/' + device
            try:
                if self.ser == None:
                    logging.info('Trying ... %s', device_link)
                    self.ser = serial.Serial(device_link, 115200)
                    logging.info('Successfully connected to %s', device_link)
                    break
            except serial.SerialException, e:
                raise error.TestError('Failed to connect to %s becuase of %s' %
                                     (device_link, str(e)))

        self.text_file = open(_power_play_data_file, 'w')

        if self.ser != None:
            title_row = ('time,powerplay_timestamp,momentary_current (A),' +
                    'momentary_charge (AH),average_current (A),' +
                    'total_standby_time,total_wake_time,num_wakes,is_awake?\n')
            self.text_file.write(title_row)
            start_time = time.time()
            while self.ser.readline():
                current_timestamp = (('{:>10.3f}'.
                        format(time.time() - start_time)).replace(' ', ''))
                pp_data = (self.ser.readline().replace('\00', '').
                        replace(' ', ',').replace('\r', ''))
                if (not pp_data.startswith('#') and (len(pp_data) > 30) and
                        not self.text_file.closed):
                    self.text_file.write(current_timestamp + ',' + pp_data)
                    self.momentary_curr_list = self.extract_current(pp_data)
                time.sleep(self.recording_interval)
                self.ser.flushInput()
        else:
            self.text_file.write('No data from powerplay. Check connection.')

    def stop_recording_power_play_data(self):
        """Stop recording power play data.

        Close the text file and copy it to the test log results directory. Also
        report current data to the performance dashboard.
        """
        if not self.text_file.closed:
            self.text_file.close()
        shutil.copy(_power_play_data_file, self.test.resultsdir)
        self.test.output_perf_value(description='momentary_current_draw',
                               value=self.momentary_curr_list,
                               units='Amps', higher_is_better=False)
