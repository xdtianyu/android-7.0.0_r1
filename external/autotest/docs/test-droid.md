# test\_droid: Quick Primer

## References
[Autotest Developer Guide](https://www.chromium.org/chromium-os/testing/autotest-user-doc)

[test\_that Basic Usage](docs/test-that.md)

## Objective
This document contains instructions for Brillo/Android developers interested in
running basic automated integration tests at their desk.  Developers can run
existing autotest tests as well as write their own.  Testing on Brillo/Android
is currently limited to server-side tests, which run on an autotest server and
control a Brillo/Android DUT (device under test) via remote command execution.
Running client-side autotest tests requires Python on the DUT and isn’t
currently supported.  `test_droid` does not support the autoupdate end-to-end
test, for instructions on how to run this test please refer to the Running
Brillo/Android Autoupdate End-to-End Test doc.

## Usage
The autotest repository is checked out in both AOSP and internal manifests at
external/autotest.

### Running tests against a single local device under test
Once you have a local copy of the autotest source, you can easily run tests
against a DUT connected directly to your workstation via a USB cable. Please
note your first time running `test_droid` it will download and install a number
of required packages locally into your autotest checkout.

First lookup the device serial number:

```
 $ adb devices
* daemon started successfully *
List of devices attached
7d52318 device
```

Run site\_utils/test\_droid.py from your autotest checkout to launch a test
against a given DUT:

```
 $ ./site_utils/test_droid.py <DUT Serial Number> <Test Name>
```

For example, to run the brillo\_WhitelistedGtests test:

```
 $ ./site_utils/test_droid.py 7d52318 brillo_WhitelistedGtests
```

`test_droid` can run multiple tests at once:

```
 $ ./site_utils/test_droid.py 7d52318 \
      brillo_WhitelistedGtests brillo_KernelVersionTest
```

As well as test suites:

```
 $ ./site_utils/test_droid.py 7d52318 suite:brillo-bvt
```


### Running tests that require multiple devices under test
Autotest now supports the concept of testbeds, which are multiple devices being
controlled by a single test. `test_droid` supports running these tests
by specifying a comma separated list of serials as the test device:

```
 $ adb devices
List of devices attached
emulator-5554 device
7d52318 device

 $ ./site_utils/test_droid.py emulator-5554,7d52318 testbed_DummyTest
```

### Running tests against a remote device under test
`test_droid` can run tests against devices connected to a remote server.  This
requires passwordless SSH access from the workstation to the remote server.
If no username is specified, `test_droid` will try the root and adb users.
If using the adb user, make sure it has passwordless sudo
rights to run the adb and fastboot commands. You can specify a
different user in the remote host name (the same passwordless requirement
applies).

The easiest way to set this up is to use the
[Chrome OS testing keys](https://www.chromium.org/chromium-os/testing/autotest-developer-faq/ssh-test-keys-setup).
Add to your SSH config an entry that looks like the following:

```
HostName <Remote Server IP or Hostname>
  Port 9222
  User root
  CheckHostIP no
  StrictHostKeyChecking no
  IdentityFile ~/.ssh/testing_rsa
  Protocol 2
```

To run the test:

```
 $ ./site_utils/test_droid.py \
       -r <Remote Server IP or Hostname> <Serial Number> \
       <Test Name>

 $ ./site_utils/test_droid.py \
       -r <User>@<Remote Server IP or Hostname> \
       <Serial Number> <Test Name>

 $ ./site_utils/test_droid.py -r 100.96.48.119 7d52318 suite:brillo-bvt

```

### Advanced: Uploading Commits for Review
Currently Autotest in AOSP is read-only, so you cannot use repo upload to
upload code changes. If you do edit or add a new test, make a commit and upload
it to https://chromium-review.googlesource.com.

Be sure to run pylint on every file you touch:

```
$ ./utils/run_pylint.py <file name>
```

Then upload your commit for review:

```
 $ git push https://chromium.googlesource.com/chromiumos/third_party/autotest \
     <local branch name>:refs/for/master
```
