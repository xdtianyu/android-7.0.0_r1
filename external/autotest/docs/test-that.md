## Introduction

`test_that` is the supported mechanism to run autotests against Chrome OS
devices at your desk.  `test_that` replaces an older script, `run_remote_tests`.

Features:
  - CTRL+C kills `test_that` and all its autoserv children. Orphaned processes
    are no longer left behind.
  - Tests that require binary autotest dependencies will just work, because
    test_that always runs from the sysroot location.
  - Running emerge after python-only test changes is no longer necessary.
    test_that uses autotest_quickmerge to copy your python changes to the
    sysroot.
  - Tests are generally specified to `test_that` by the NAME field of their
    control file. Matching tests by filename is supported using f:[file
    pattern]

### Example uses (inside the chroot)

Run the test(s) named dummy\_Pass:

```
$ test_that -b ${board} ${host} dummy_Pass
```

Run the test(s) named dummy\_Pass.suspend:

```
$ test_that -b ${board} ${host} dummy_Pass.suspend
```

Run the smoke suite against dut:

```
$ test_that -b ${board} ${host} suite:smoke
```

Run all tests whose names match the regular expression `^login_.*$`. Note that
even though these tests have binary dependencies, there is no longer a need to
specify extra flags.

```
$ test_that -b ${board} ${host} e:login_.*
```

Run all tests whose control file filename matches the regular expression
`^.*control.dummy$`:

```
$ test_that -b ${board} ${host} f:.*control.dummy
```

### Running jobs in the lab

`test_that` now allows you to run jobs in the test lab. The usage is similar to
running tests against a specified host. The keyword :lab: is used as
test\_that's REMOTE argument, and the -i/--build argument is required, and takes
a trybot, paladin, or canary build number. To learn how to build a trybot image
with a new test that you're iterating on, see "dynamic suite" codelab.

For instance:

```
$ test_that -b lumpy -i lumpy-paladin/R38-6009.0.0-rc4 :lab: dummy_Pass
```

This will kick off a suite in the lab that consists of just 1 job, dummy\_Pass,
to run in this case on board lumpy using the image
lumpy-paladin/R38-6009.0.0-rc4. The lab's scheduler will take responsibility
for finding a suitable set of hosts, provisioning them to the correct image,
and running the tests. `test_that` will return after the suite finishes running,
with a suite run report.

You can specify multiple tests or test-matching expressions in the same way as
before:

```
$ test_that -b lumpy -i ${latest_image} :lab: dummy_Pass dummy_Fail
$ test_that -b lumpy -i ${latest_image} :lab: e:login_.*
```

Kicking off a run in the lab should be useful whenever you need to run a
particular test on a board or image that you do not have readily available
locally.For occasional runs of ad-hoc suites in the lab, this will also avoid
the need to create a suite control file and wait for it to end up in an image.

You can also kick off a suite, for example with:

```
test_that -b peach_pit :lab: suite:pyauto_perf -i 'peach_pit-release/R32-4763.0.0'
```

That told me that my job ID was 5196037. I could follow along by going to
http://cautotest/afe/#tab_id=view_job&object_id=5195962.

### Things to note:

This will only work with images newer than Sept 20, 2013 (specifically, builds
that contain Ifa73d7de7aac9c6efebd5f559708623804ad3691). Jobs will be scheduled
in the pool:try-bot machine pool.
