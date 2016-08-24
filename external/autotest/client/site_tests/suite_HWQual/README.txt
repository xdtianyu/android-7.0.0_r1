Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.


This document describes the steps to go through in order to run Chrome OS
hardware qualification on a device under test.

================================================================================
Glossary
================================================================================

- $: command line prompt
- $HOME: home directory of current user.
- AC: alternating current, implies device is not powered by battery.
- DUT: device under test
- Semi-Automated Test: test that runs with automation but requires manual
  intervention.

================================================================================
Test Setup
================================================================================

- Setup a Linux machine to serve as the Autotest server. The Autotest
  server requires Python, Wireless access to the DUT and basic Linux
  shell utilities. The setup has been tested on Ubuntu 9.10 available
  for download at http://www.ubuntu.com/getubuntu/download/.


- Create an installation directory on the Autotest server for the
  Chrome OS hardware qualification package. The rest of the
  instructions assume that you're installing the package in the
  current user home directory ($HOME/).


- Contact your Google technical support person and download the Chrome
  OS hardware qualification package chromeos-hwqual-TAG.tar.bz2 for
  your device in $HOME/.


- Install the package on the server:

  $ cd $HOME/ && tar xjf chromeos-hwqual-TAG.tar.bz2


- Install the Chrome OS test image on the DUT. The USB test image is
  available in:

  $HOME/chromeos-hwqual-TAG/chromeos-hwqual-usb.img

  Here are sample steps to install the test image.

  - Plug a USB storage device into the Autotest server. Note that all
    data on your USB stick will be destroyed.

  - Unmount any mounts on the USB device.

  - Copy the USB image to a USB storage device by executing:

    $ sudo dd if=$HOME/chromeos-hwqual-TAG/chromeos-hwqual-usb.img \
              of=/dev/sdX

  - where /dev/sdX is your USB device.

  - Plug the USB device into the DUT and boot from it.

  - Log in to Chrome OS.  Start the Chrome OS shell by pressing Ctrl-Alt-T.
    Install Chrome OS on the DUT:

    crosh> install


- Cold boot the DUT -- turn the DUT off and then back on. This ensures
  a consistent starting point for the qualification tests and allows
  the system to collect cold boot performance metrics. Make sure you
  don't boot from USB.


- Connect the DUT to the network and note its IP address <DUT_IP>. The
  IP address is displayed at the bottom of the network selection
  menu. Unless specified explicitly, the test setup works correctly
  through either wireless or wired network connections.


- Add the DUT root private key to ssh-agent on the Autotest server:

  $ ssh-add $HOME/chromeos-hwqual-TAG/testing_rsa

- If ssh-add fails saying that it cannot connect to your authentication agent,
  retry the command after running:

  $ eval `ssh-agent -s`

- These commands allow the Autotest server to connect and login as root on the
  DUT.


- Make sure you can ssh as root to the DUT from the Autotest
  server. The command below should print 0.

  $ ssh root@<DUT_IP> true; echo $?

================================================================================
Automated and Semi-Automated Test Runs
================================================================================

- Unless otherwise noted, all tests can be performed on an AC-powered DUT.

- Go to the Autotest server directory and clean up previous test results.

  $ cd $HOME/chromeos-hwqual-TAG/autotest/
  $ rm -rf results.*


- Run the fully automated client-side tests:

  $ ./server/autoserv -r results.auto -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.auto


- Plug high-speed high-capacity storage devices in all USB and SD Card
  slots and run the external storage test:

  $ ./server/autoserv -r results.external_devices -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.external_drives


- Run the system suspend/resume stability test:

  $ ./server/autoserv -r results.suspend_resume -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.suspend_resume


- If the DUT has video out ports, run the Video Out semi-automated
  test by following the instructions specified in the control file
  (control.video_out) and then executing:

  $ ./server/autoserv -r results.video_out.${PORT} -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.video_out

- Where PORT is the name of each video port you are testing.  For
  example, if the DUT has one HDMI and one VGA out port, run:

  $ ./server/autoserv -r results.video_out.hdmi1 -m <DUT_IP> \
                -c client/site_tests/suite_HWQual/control.video_out

  $ ./server/autoserv -r results.video_out.vga1 -m <DUT_IP> \
                -c client/site_tests/suite_HWQual/control.video_out


- Run the graphics tearing test:

  $ ./server/autoserv -r results.teartest -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.teartest

- Run audio test, with built-in speakers and microphone

  $ ./server/autoserv -r results.audio -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.audio

- Plug-in headphone and microphone, run audio test again

  $ ./server/autoserv -r results.audio_ext -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.audio

- Run the Keyboard test : 
  (Wait several seconds after running the test. Then strike the "Search" key,
   e.g. the key above Left Shift and below Tab)

  $ ./server/autoserv -r result.keyboard -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.keyboard

- Run the DUT on AC. Probe the AC driver:

  $ ./server/autoserv -r result.probe_ac -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.probe_ac

- Run the DUT on battery. Probe the battery driver:
  (If you just unplugged AC, please wait for a second before running
   the test for kernel updating power status.)

  $ ./server/autoserv -r result.probe_bat -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.probe_bat

- Run the DUT on AC. Plug a power draw USB dongle in each USB port.
  Run the max power draw test:

  $ ./server/autoserv -r results.max_power_draw.ac -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.max_power_draw

- Run the DUT on battery. Plug a power draw USB dongle in each USB
  port. Run the max power draw test:

  $ ./server/autoserv -r results.max_power_draw.batt -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.max_power_draw

- Run the DUT on AC. Run the power settings test:

  $ ./server/autoserv -r results.power_x86_setting.ac -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.power_x86_settings

- Run the DUT on battery. Run the power settings test:

  $ ./server/autoserv -r results.power_x86_setting.batt -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.power_x86_settings

- Make sure the remaining battery charge is less than 5%. Note that the test
  will check and fail quickly if the initial battery charge is more than 5%.
  Run the DUT on AC. Run the battery charge test:

  $ ./server/autoserv -r results.battery_charge_time -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.battery_charge_time

- Make sure that the battery is fully charged. Note that the test will not
  check if the battery is fully charged before running. Run the DUT on
  battery. Run the battery load test by first following the manual
  instructions specified in the control file (control.battery_load)
  and then executing:

  $ ./server/autoserv -r results.battery_load -m <DUT_IP> \
                  -c client/site_tests/suite_HWQual/control.battery_load

================================================================================
Reviewing Automated and Semi-Automated Test Results
================================================================================

- Autotest logs progress and performance data in results.* directories
  specified through the '-r' autoserv option. The easy way to see a summary
  of your test results is to use the 'generate_test_report'
  script installed under $HOME/chromeos-hwqual-TAG/:

  $ ../generate_test_report results.*

  This will display a table with test status and perfomance data for
  all result directories.

- If deeper investigation into a failure is required, you can review
  the debug information stored in results:

  $ ls */*/debug/.

================================================================================
Manual Test Runs
================================================================================

- Perform the manual tests specified in
  $HOME/chromeos-hwqual-TAG/manual/testcases.csv.

- Please note that some tests cannot be tested as they rely on
  features not yet implemented.  They are being included as a preview for
  manual tests that will be required.  Such tests will have
  "NotImplemented" in their "LABELS" column.

- Update this spreadsheet with a column of pass/fail results with any notes
  which may be useful.

================================================================================
Reporting Results
================================================================================

- Upon completing automatic, semi-automatic, and manual test runs, you should
  report your results to a Google technical support contact to verify the
  tests were run correctly and to help diagnose failures.  Verify you have
  updated your manual test spreadsheet as described above and copy it into
  your autotest output directory:

  $ cd $HOME/chromeos-hwqual-TAG/autotest/
  $ cp ../manual/testcases.csv .

- Package all results into a tar file:

  $ tar cjf chromeos-hwqual-results-TAG-DATE.tar.bz2 results.* testcases.csv

- Send the resulting chromeos-hwqual-results file to your technical support
  contact.
