# Autotest Best Practices
When the Chrome OS team started using autotest, we tried our best to figure out
how to fit our code and our tests into the upstream style with little guidance
and poor documentation.  This went poorly.  With the benefit of hindsight,
we’re going to lay out some best-practices that we’d like to enforce going
forward.  In many cases, there is legacy code that contradicts this style; we
should go through and refactor that code to fit these guidelines as time
allows.

## Upstream Documentation

There is a sizeable volume of general Autotest documentation available on
github:
https://github.com/autotest/autotest/wiki

## Coding style

Basically PEP-8.  See [docs/coding-style.md](docs/coding-style.md)

## Where should my code live?

| Type of Code              | Relative Path           |
|---------------------------|-------------------------|
| client-side tests         | client/site_tests/      |
| server-side tests         | server/site_tests       |
| common library code       | client/common_lib/cros/ |
| server-only library code  | server/cros             |


## Writing tests

An autotest is really defined by its control file.  A control file contains
important metadata about the test (name, author, description, duration, what
suite it’s in, etc) and then pulls in and executes the actual test code.  This
test code can be shared among multiple distinct test cases by parameterizing it
and passing those parameters in from separate control files.

Autotests *must*:

 * Be self-contained: assume nothing about the condition of the device
 * Be hermetic: requiring the Internet to be reachable in order for your test
   to succeed is unacceptable.
 * Be automatic: avoid user interaction and run-time specification of input
   values.
 * Be integration tests: if you can test the feature in a unit test (or a
   chrome browser test), do so.
 * Prefer object composition to inheritance: avoid subclassing test.test to
   implement common functionality for multiple tests.  Instead, create a class
   that your tests can instantiate to perform common operations.  This enables
   us to write tests that use both PyAuto and Servo without dealing with
   multiple inheritance, for example.
 * Be deterministic: a test should not validate the timing of some operation.
   Instead, write a test that records the timing in performance keyvals so that
   we can track the numbers over time.

Autotests *must not*:

 * Put significant logic in the control file: control files are really just
   python, so one can put arbitrary logic in there.  Don’t.  Run your test
   code, perhaps with some parameters.

Autotests *may*:

 * Share parameterized fixtures: a test is defined by a control file.  Control
   files import and run test code, and can pass simple parameters to the code
   they run through a well-specified interface.

Autotest has a notion of both client-side tests and server-side tests.  Code in
a client-side test runs only on the device under test (DUT), and as such isn’t
capable of maintaining state across reboots or handling a failed suspend/resume
and the like.  If possible, an autotest should be written as a client-side
test.  A ‘server’ test runs on the autotest server, but gets assigned a DUT
just like a client-side test.  It can use various autotest primitives (and
library code written by the CrOS team) to manipulate that device.  Most, if not
all, tests that use Servo or remote power management should be server-side
tests, as an example.

Adding a test involves putting a control file and a properly-written test
wrapper in the right place in the source tree.  There are conventions that must
be followed, and a variety of primitives available for use.  When writing any
code, whether client-side test, server-side test, or library, have a strong
bias towards using autotest utility code.  This keeps the codebase consistent.


## Writing a test

This section explains considerations and requirements for any autotest, whether
client or server.

### Control files

Upstream documentation
Our local conventions for autotest control files deviate from the above a bit,
but the indication about which fields are mandatory still holds.

| Variable     | Required | Value                                                                                                                                                                                                                                                                                                                                    |
|--------------|----------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| AUTHOR       | Yes      | A comma-delimited string of at least one responsible engineer and a backup engineer -- or at worst a backup mailing list. i.e. AUTHOR = ‘msb, snanda’                                                                                                                                                                                    |
| DEPENDENCIES | No       | list of tags known to the HW test lab.                                                                                                                                                                                                                                                                                                   |
| DOC          | Yes      | Long description of the test, pass/fail criteria                                                                                                                                                                                                                                                                                         |
| NAME         | Yes      | Display name of the test. Generally this is the directory where your test lives e.g. hardware_TPMCheck. If you are using multiple run_test calls in the same control file or multiple control files with one test wrapper in the same suite, problems arise with the displaying of your test name. crosbug.com/35795. When in doubt ask. |
| SYNC\_COUNT  | No       | Integer >= 1.  Number of simultaneous devices needed for a test run.                                                                                                                                                                                                                                                                     |
| TIME         | Yes      | Test duration: 'FAST' (<1m), 'MEDIUM' (<10m), 'LONG' (<20m), 'LENGTHY' (>30m)                                                                                                                                                                                                                                                            |
| TEST\_TYPE   | Yes      | Client or Server                                                                                                                                                                                                                                                                                                                         |
| SUITE        | No       | A comma-delimited string of suite names that this test should be a part of.                                                                                                                                                                                                                                                              |

### Choosing a Suite

Currently existing suites are defined in the test\_suites/ subdirectory at the
top level of the autotest repo.  Read the docstrings there to see if your new
test fits into one that’s already defined.

When first adding a test, it should not go into the BVT suite.   A test should
only be added to the BVT after it has been running in some non-BVT suite long
enough to establish a track record showing that the test does not fail when run
against working software.  A suite named experimental exists for tests intended
for the BVT, and for which there is no more convenient home.

### Pure python 

Lie, cheat and steal to keep your tests in pure python.  It will be easier to
debug failures, it will be easier to generate meaningful error output, it will
be simpler to get your tests installed and run, and it will be simpler for the
lab team to build tools that allow you to quickly iterate.

Shelling out to existing command-line tools is done fairly often, and isn’t a
terrible thing.  The test author can wind up having to do a lot of output
parsing, which is often brittle, but this can be a decent tradeoff in lieu of
having to reimplement large pieces of functionality in python.

Note that you will need to be sure that any commands you use are installed on
the host.  For a client-side test, “the host” means “the DUT”.  For a
server-side test, “the host” typically means “the system running autoserv”;
however, if you use SiteHost.run(), the command will run on the DUT.  On the
server, your tests will have access to all tools common to both a typical CrOS
chroot environment and standard Goobuntu.

If you want to use a tool on the DUT, it may be appropriate to include it as a
dependency of the chromeos-base/chromeos-test package.  This ensures that the
tool is pre-installed on every test image for every device, and will always be
available for use.  Otherwise, the tool must be installed as an autotest “dep”.

_Never install your own shell scripts and call them._  Anything you can do in
shell, you can do in python.

### Reporting failures

Autotest supports several kinds of failure statuses:

| Status   | Exception         | Reason                                                                                                                                                                                                                                                                                                                   |
|----------|-------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| WARN     | error.TestWarn    | error.TestWarn should be used when side effects to the test running are encountered but are not directly related to the test running. For example, if you are testing Wifi and powerd crashes. *Currently* there are not any clear usecases for this and error.TestWarn should be generally avoided until further notice. |
| TEST\_NA | error.TestNAError | This test does not apply in the current environment.                                                                                                                                                                                                                                                                     |
| ERROR    | error.TestError   | The test was unable to validate the desired behavior.                                                                                                                                                                                                                                                                    |
| FAIL     | error.TestFail    | The test determined the desired behavior failed to occur.                                                                                                                                                                                                                                                                |


### Considerations when writing client-side tests

All client-side tests authored at Google must live in the client/site\_tests sub-directory of the autotest source tree.

###Compiling and executing binaries

It is possible to compile source that’s included with your test and use the
products at test runtime.  The build infrastructure will compile this code for
the appropriate target architecture and package it up along with the rest of
your test’s resources, but this increases your development iteration time as
you need to actually re-build and re-package your test to deploy it to the
device.  While we hope to improve tooling support for this use case in the
future, avoiding this issue is the ideal.

If you can’t avoid this, here’s how to get your code compiled and installed as
a part of your test:
1. Create a src/ directory next to your control file.
2. Put your source, including its Makefile, in src/
3. define a method in your test class called “setup(self)” that takes no arguments.
4. setup(self) should perform all tasks necessary to build your tool.  There are some helpful utility functions in client/common_lib/base_utils.py.  Trivial example:

```
    def setup(self):
        os.chdir(self.srcdir)
        utils.make('OUT_DIR=.')
```

### Reusing code (“fixtures”)

Any autotest is, essentially, a single usage of a re-usable test fixture.  This
is because run\_once() in your test wrapper can take any arguments you want.  As
such, multiple control files can re-use the same wrapper -- and should, where
it makes sense.

### Considerations when writing server-side tests

All server-side tests authored at Google must live in the server/site\_tests
sub-directory of the autotest source tree.

It should be even easier to keep the server-side of a test in pure python, as
you should simply be driving the DUT and verifying state.

### When/why to write a server-side test

Server-side tests are appropriate when some operation in the test can't be
executed on the DUT.  The prototypical example is rebooting the DUT.  Other
examples include tests that manipulate the network around the DUT (e.g. WiFi
tests), tests that power off the DUT, and tests that rely on a Servo attached
to the DUT.

One simple criterion for whether to write a server-side test is this:  Is the
DUT an object that the test must manipulate?  If the answer is “yes”, then a
server-side test makes sense.

### Control files for server-side tests

Server-side tests commonly operate on the DUT as an object.  Autotest
represents the DUT with an instance of class Host; the instance is constructed
and passed to the test from the control file.  Creating the host object in the
control file can be done using certain definitions present in the global
environment of every control file:

 * Function hosts.create\_host() will create a host object from a string with
   the name of the host (an IP address as a string is also acceptable).
 * Variable machines is a list of the host names available to the test.

Below is a sample fragment for a control file that runs a simple server side test in parallel on all the hosts specified for the test.  The fragment is a complete control file, except for the missing boilerplate comments and documentation definitions required in all control files.

```
def run(machine):
    host = hosts.create_host(machine)
    job.run_test("platform_ServerTest", host=host)

parallel_simple(run, machines)
```

Note:  The sample above relies on a common convention that the run\_once()
method of a server-side test defines an argument named host with a default
value, e.g.

```
def run_once(self, host=None):
    # … test code goes here.
```

### Operations on Host objects

A Host object supports various methods to operate on a DUT.  Below is a short list of important methods supported by instances of Host:

 * run(command) - run a shell command on the host
 * reboot() - reboot the host, and wait for it to be back on the network
 * wait_up() - wait for the host to be active on the network
 * wait_down() - wait until the host is no longer on the network, or until it is known to have rebooted.

More details, including a longer list of available methods, and more about how
they work can be found in the Autotest documentation for autoserv and Autotest
documentation for Host.

### Servo-based tests

For server-side tests that use a servo-attached DUT, the host object has a
servo attribute.  If Autotest determines that the DUT has a Servo attached, the
servo attribute will be a valid instance of a Servo client object; otherwise
the attribute will be None.

For a DUT in the lab, Autotest will automatically determine whether there is a
servo available; however, if a test requires Servo, its control file must have
additional code to guarantee a properly initialized servo object on the host.

Below is a code snippet outlining the requirements; portions of the control file have been omitted for brevity:

```
# ... Standard boilerplate variable assignments...
DEPENDENCIES = "servo"
# ... more standard boilerplate...

args_dict = utils.args_to_dict(args)
servo_args = hosts.SiteHost.get_servo_arguments(args_dict)

def run(machine):
    host = hosts.create_host(machine, servo_args=servo_args)
    job.run_test("platform_SampleServoTest", host=host)

parallel_simple(run, machines)
```

The `DEPENDENCIES` setting guarantees that if the test is scheduled in the lab,
it will be assigned to a DUT that has a servo.

The setting of `servo_args` guarantees two distinct things:  First, it forces
checks that will make sure that the Servo is functioning properly; this
guarantees that the host's `servo` attribute will not be None.  Second, the code
allows you to pass necessary servo specific command-line arguments to
`test_that`.

If the test control file follows the formula above, the test can be reliably called in a variety of ways:
 * When used for hosts in the lab, the host’s servo object will use the servo attached to the host, and the test can assume that the servo object is not None.
 * If you start servod manually on your desktop using the default port, you can use test_that without any special options.
 * If you need to specify a non-default host or port number (e.g. because servod is remote, or because you have more than one servo board), you can specify them with commands like these:

```
test_that --args=”servo_host=...” …
test_that --args=”servo_port=...” …
test_that --args=”servo_host=... servo_port=...” ...
```

### Calling client-side tests from a server-side test

Commonly, server-side tests need to do more on the DUT than simply run short
shell commands.  In those cases, a client-side test should be written and
invoked from the server-side test.  In particular, a client side test allows
the client side code to be written in Python that uses standard Autotest
infrastructure, such as various utility modules or the logging infrastructure.

Below is a short snippet showing the standard form for calling a client-side
test from server-side code:

```
from autotest_lib.server import autotest

    # ... inside some function, e.g. in run_once()
    client_at = autotest.Autotest(host)
    client_at.run_test("platform_ClientTest")
```

### Writing library code

There is a large quantity of Chromium OS specific code in the autotest
codebase.  Much of this exists to provide re-usable modules that enable tests
to talk to system services.  The guidelines from above apply here as well.
This code should be as pure python as possible, though it is reasonable to
shell out to command line tools from time to time.  In some cases we’ve done
this where we could (now) use the service’s DBus APIs directly.  If you’re
adding code to allow tests to communicate with your service, it is strongly
recommended that you use DBus where possible, instead of munging config files
directly or using command-line tools.

Currently, our library code lives in a concerning variety of places in the
autotest tree.  This is due to a poor initial understanding of how to do
things, and new code should follow the following conventions instead:

 * Used only in server-side tests: server/cros
 * Used in both server- and client-side tests, or only client:
   client/common\_lib/cros

### Adding test deps

This does not refer to the optional `DEPENDENCIES` field in test control files.
Rather, this section discusses how and when to use code/data/tools that are not
pre-installed on test images, and should (or can) not be included right in with
the test source.

Unfortunately, there is no hard-and-fast rule here.  Generally, if this is some
small tool or blob of data you need for a single test, you should include it as
discussed above in Writing client-side tests.  If you’re writing the tool, and
it has use for developers as well as in one or more tests that you’re writing,
then make it a first-class CrOS project.  Write an ebuild, write unit tests,
and then add it to the test image by default.  This can be done by RDEPENDing
on your new test package from the chromeos-test ebuild.

If your code/data falls in the middle (useful to several tests, not to devs),
and/or is large (hundreds of megabytes as opposed to tens) then using an
autotest ‘dep’ may be the right choice.  Conceptually, an autotest test dep is
simply another kind of archive that the autotest infrastructure knows how to
fetch and unpack.  There are two components to including a dependency from an
autotest test -- setup during build time and installing it on your DUT when
running a test.  The setup phase must be run from your tests setup() method
like so:

```
def setup(self):
  self.job.setup_dep([‘mydep’])
  logging.debug(‘mydep is at %s’ % (os.path.join(self.autodir,
                                                 ‘deps/mydep’))
```

The above gets run when you “build” the test.

The other half of this equation is actually installing the dependency so you
can use it while running a test.  To do this, add the following to either your
run\_once or initialize methods:

```
        dep = dep_name
        dep_dir = os.path.join(self.autodir, 'deps', dep=dep)
        self.job.install_pkg(dep, 'dep', dep_dir)
```


You can now reference the content of your dep using dep_dir.

Now that you know how to include a dep, the next question is how to write one.
Before you read further, you should check out client/deps/\* for many examples
of deps in our autotest tree.

### Create a dep from a third-party package

There are many examples of how to do this in the client/deps directory already.
The key component is to check in a tarball of the version of the dependency
you’d like to include under client/deps/your\_dep.

All deps require a control file and an actual python module by the same name.
They will also need a copy of common.py to import utils.update\_version. Both
the control and common are straightforward, the python module does all the
magic.

The deps python module follows a standard convention: a setup function and a
call to utils.update\_version.  update\_version is used instead of directly
calling setup as it maintains additional versioning logic ensuring setup is
only done 1x per dep. The following is its method signature:

```
def update_version(srcdir, preserve_srcdir, new_version, install, 
                   *args, **dargs)
```


Notably, install should be a pointer to your setup function and `*args` should
be filled in with params to said setup function.

If you are using a tarball, your setup function should look something like:

```
def setup(tarball, my_dir)
    utils.extract_tarball_to_dir(tarball, my_dir)
    os.chdir(my_dir)
    utils.make() # this assumes your tarball has a Makefile.
```

And you would invoke this with:

```
utils.update_version(os.getcwd(), True, version, setup, tarball_path,
                     os.getcwd())
```


Note: The developer needs to call this because def setup is a function they are
defining that can take any number of arguments or install the dep in any way
they see fit. The above example uses tarballs but some are distributed as
straight source under the src dir so their setup function only takes a top
level path. We could avoid this by forcing a convention but that would be
artificially constraining the deps mechanism. 

Once you’ve created the dep, you will also have to add the dep to the
autotest-deps package in chromiumos-overlay/chromeos-base/autotest-deps,
‘cros\_workon start’ it, and re-emerge it.

### Create a dep from other chrome-os packages

One can also create autotest deps from code that lives in other CrOS packages,
or from build products generated by other packages.  This is similar as above
but you can reference code using the `CHROMEOS_ROOT` env var that points to the
root of the CrOS source checkout, or the SYSROOT env var (which points to
/build/<board>) to refer to build products.  Again, read the above. Here’s an
example of the former with the files I want in
chromeos\_tree/chromite/my\_dep/\* where this will be the python code in
autotest/files/client/deps/my\_dep/my\_dep.py module.

```
import common, os, shutil
from autotest_lib.client.bin import utils

version = 1

def setup(setup_dir):
    my_dep_dir = os.path.join(os.environ['CHROMEOS_ROOT'], 'chromite',
                              'buildbot')
    shutil.copytree(my_dep_dir, setup_dir)


work_dir = os.path.join(os.getcwd(), 'src')
utils.update_version(os.getcwd(), True, version, setup, work_dir)
```
