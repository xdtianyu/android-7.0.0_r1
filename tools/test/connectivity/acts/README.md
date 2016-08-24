# Android Comms Test Suite
ACTS is a python-based test framework that is designed to be lightweight,
pluggable, and easy to use. It initializes equipment and services associated
to a test run, provides those resources to test classes, executes test cases,
and generates test reports.

ACTS follows the Google Open-source
[Python Style Guide](https://google.github.io/styleguide/pyguide.html), and
it is recommended for all new test cases.

## ACTS Execution Flow Overview
Below is a high level view of the ACTS flow:
1. Read configuration files
2. Create controllers
3. Sequentially execute test classes
```
FooTest.setup_class()
FooTest.setup_test()
FooTest.test_A()
FooTest.teardown_test()
FooTest.setup_test()
FooTest.test_B()
FooTest.teardown_test()
....
FooTest.teardown_class()
BarTest.setup_class()
....
```
4. Destroy controllers

## Preparing an Android Device
### Allow USB Debugging
USB debugging must be enabled before a device can take commands from adb.
To enable USB debugging, first enable developer mode.
1. Go to Settings->About phone
2. Tap Build number repeatedly until "You're a developer now" is displayed.

In developer mode:
1. Plug the device into a computer (host)
2. Run `$adb devices`
- A pop-up asking to allow the host to access the android device may be
displayed. Check the "Always" box and click "Yes".

## ACTS Setup
1. ACTS requires three python dependencies:
- Python3.4
- The setuptools package
- The pyserial package
2. From the ACTS directory, run setup
- `$ sudo python3 setup.py develop`

After installation, `act.py` and `flashutil.py` will be in usr/bin and can be
called as command line utilities. Components in ACTS are importable under the
package "acts." in Python3.4, for example:
```
$ python3
>>> from acts.controllers import android_device
>>> device_list = android_device.get_all_instances()
```

## Verifying Setup
To verify the host and device are ready, from the frameworks folder run:
- `$ act.py -c sample_config.json -tb SampleTestBed -tc SampleTest`

If the above command executed successfully, the ouput should look something
similar to following:
```
[SampleTestBed] 07-22 15:23:50.323 INFO ==========> SampleTest <==========
[SampleTestBed] 07-22 15:23:50.327 INFO [Test Case] test_make_toast
[SampleTestBed] 07-22 15:23:50.334 INFO [Test Case] test_make_toast PASS
[SampleTestBed] 07-22 15:23:50.338 INFO Summary for test class SampleTest:
Requested 1, Executed 1, Passed 1, Failed 0, Skipped 0
[SampleTestBed] 07-22 15:23:50.338 INFO Summary for test run
SampleTestBed@07-22-2015_1-23-44-096: Requested 1, Executed 1, Passed 1,
Failed 0, Skipped 0
```
By default, all logs are saved in `/tmp/logs`

## Breaking Down the Example
Below are the components of the command run for the SampleTest:
- `acts.py`: is the script that runs the test
-  -c sample_config: is the flag and name of the configuration file to be used
in the test
-  -tb StampleTestBed: is the flag and name of the test bed to be used
-  -tc SampleTest: is the name of the test case

### Configuration Files
To run tests, required information must be provided via a json-formatted
text file. The required information includes a list of “testbed” configs.
Each specifies the hardware, services, the path to the logs directory, and
a list of paths where the python test case files are located. Below are the
contents of a sample configuration file:
```
{   "_description": "This is an example skeleton test configuration file.",
    "testbed":
    [
        {
            "_description": "Sample testbed with no devices",
            "name": "SampleTestBed"
        }
    ],
    "logpath": "/tmp/logs",
    "testpaths": ["../tests/sample"],
    "custom_param1": {"favorite_food": "Icecream!"}
}
```

### Test Class
Test classes are instantiated with a dictionary of “controllers”. The
controllers dictionary contains all resources provided to the test class
and are created based on the provided configuration file.

Test classes must also contain an iterable member self.tests that lists the
test case names within the class.  More on this is discussed after the following
code snippet.
```
from acts.base_test import BaseTestClass

class SampleTest(BaseTestClass):

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_make_toast",
        )

    """Tests"""
    def test_make_toast(self):
        for ad in self.android_devices:
            ad.droid.makeToast("Hello World.")
        return True
```
By default all test cases listed in a Test Class\'s self.tests will be run.
Using the syntax below will override the default behavior by executing only
specific tests within a test class.

The following will run a single test, test_make_toast:
`$ act.py -c sample_config.txt -tb SampleTestBed -tc SampleTest:test_make_toast`

Multiple tests may be specified with a comma-delimited list. The following
will execute test_make_toast and test_make_bagel:
- `$ act.py -c sample_config.txt -tb SampleTestBed -tc
SampleTest:test_make_toast,test_make_bagel`
